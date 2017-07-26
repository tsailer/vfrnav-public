/*****************************************************************************/

/*
 *      aircraft.cc  --  Aircraft Model.
 *
 *      Copyright (C) 2012, 2013, 2014, 2015, 2016  Thomas Sailer (t.sailer@alumni.ethz.ch)
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

#include "aircraft.h"
#include "airdata.h"
#include "baro.h"
#include "geom.h"
#include "fplan.h"

#include <cmath>
#include <stdexcept>
#include <limits>
#include <algorithm>
#include <fstream>
#include <iostream>
#include <iomanip>
#include <unistd.h>
#include <iterator>

#ifdef HAVE_EIGEN3
#include <Eigen/Dense>
#include <Eigen/Cholesky>
#include <unsupported/Eigen/Polynomials>
#endif

#ifdef HAVE_ICONV
#include <iconv.h>
#endif

#ifdef HAVE_JSONCPP
#include <json/json.h>
#endif

template <typename T> T Aircraft::Poly1D<T>::eval(T x) const
{
	if (this->empty())
		return std::numeric_limits<T>::quiet_NaN();
	T ret(0);
	for (typename Poly1D<T>::const_reverse_iterator pi(this->rbegin()), pe(this->rend()); pi != pe; ++pi)
		ret = ret * x + *pi;
	return ret;
}

template <typename T> T Aircraft::Poly1D<T>::evalderiv(T x) const
{
	if (this->empty())
		return std::numeric_limits<T>::quiet_NaN();
	T ret(0), x1(1);
	for (typename Poly1D<T>::size_type i(1); i < this->size(); ++i, x1 *= x)
		ret += i * x1 * (*this)[i];
	return ret;
}

template <typename T> T Aircraft::Poly1D<T>::evalderiv2(T x) const
{
	if (this->empty())
		return std::numeric_limits<T>::quiet_NaN();
	T ret(0), x1(1);
	for (typename Poly1D<T>::size_type i(2); i < this->size(); ++i, x1 *= x)
		ret += i * (i - 1) * x1 * (*this)[i];
	return ret;
}

template <typename T> T Aircraft::Poly1D<T>::inveval(T x, T tol, unsigned int maxiter, T t) const
{
	for (unsigned int i = 0; i < maxiter; ++i) {
		T x0(this->eval(t));
		if (std::fabs(x - x0) <= tol)
			break;
		T xd(this->evalderiv(t));
		if (xd == 0)
			break;
		if (false)
			std::cerr << "inveval: y " << x << " yest " << x0 << " yderiv " << xd
				  << " x " << t << " tol " << tol << " iter " << i << std::endl;
		t += (x - x0) / xd;
	}
	return t;
}

template <typename T> T Aircraft::Poly1D<T>::inveval(T x, T tol, unsigned int maxiter) const
{
	if (this->size() < 2 || (*this)[1] == 0)
		return std::numeric_limits<T>::quiet_NaN();
	return this->inveval(x, tol, maxiter, (x - (*this)[0]) / (*this)[1]);
}

template <typename T> T Aircraft::Poly1D<T>::inveval(T x) const
{
	static const T abstol(1e-8);
	static const T reltol(1e-4);
	return this->inveval(x, std::max(abstol, std::fabs(x) * reltol));
}

template <typename T> T Aircraft::Poly1D<T>::boundedinveval(T x, T blo, T bhi, T tol, unsigned int maxiter) const
{
	if (std::isnan(blo) || std::isnan(bhi) || bhi <= blo)
		return std::numeric_limits<T>::quiet_NaN();
	T t;
	{
		T xlo(this->eval(blo));
		T xhi(this->eval(bhi));
		if (std::isnan(xlo) || std::isnan(xhi) || fabs(xhi - xlo) < 1e-10) {
			t = 0.5 * (blo + bhi);
		} else {
			t = (x - xlo) / (xhi - xlo) * (bhi - blo) + blo;
			t = std::max(blo, std::min(bhi, t));
		}
	}
	for (unsigned int i = 0; i < maxiter; ++i) {
		T x0(this->eval(t));
		if (std::fabs(x - x0) <= tol)
			break;
		T xd(this->evalderiv(t));
		if (xd == 0)
			break;
		if (false)
			std::cerr << "inveval: y " << x << " yest " << x0 << " yderiv " << xd
				  << " x " << t << " tol " << tol << " iter " << i
				  << " bounds " << blo << "..." << bhi << std::endl;
		t += (x - x0) / xd;
		t = std::max(blo, std::min(bhi, t));
	}
	return t;
}

template <typename T> T Aircraft::Poly1D<T>::boundedinveval(T x, T blo, T bhi) const
{
	static const T abstol(1e-8);
	static const T reltol(1e-4);
	return this->boundedinveval(x, blo, bhi, std::max(abstol, std::fabs(x) * reltol));
}

template <typename T> void Aircraft::Poly1D<T>::eval(XY& x) const
{
	x.set_y(eval(x.get_x()));
}

template <typename T> void Aircraft::Poly1D<T>::evalderiv(XY& x) const
{
	x.set_y(evalderiv(x.get_x()));
}

template <typename T> void Aircraft::Poly1D<T>::evalderiv2(XY& x) const
{
	x.set_y(evalderiv2(x.get_x()));
}

template <typename T> void Aircraft::Poly1D<T>::inveval(XY& x, T tol, unsigned int maxiter, T t) const
{
	x.set_y(inveval(x.get_x(), tol, maxiter, t));
}

template <typename T> void Aircraft::Poly1D<T>::inveval(XY& x, T tol, unsigned int maxiter) const
{
	x.set_y(inveval(x.get_x(), tol, maxiter));
}

template <typename T> void Aircraft::Poly1D<T>::inveval(XY& x) const
{
	x.set_y(inveval(x.get_x()));
}

template <typename T> void Aircraft::Poly1D<T>::boundedinveval(XY& x, T blo, T bhi, T tol, unsigned int maxiter) const
{
	x.set_y(boundedinveval(x.get_x(), blo, bhi, tol, maxiter));
}

template <typename T> void Aircraft::Poly1D<T>::boundedinveval(XY& x, T blo, T bhi) const
{
	x.set_y(boundedinveval(x.get_x(), blo, bhi));
}

template <typename T> void Aircraft::Poly1D<T>::reduce(void)
{
	typename Poly1D<T>::size_type n(this->size());
	while (n > 1) {
		typename Poly1D<T>::size_type n1(n - 1);
		if ((*this)[n1] != (T)0)
			break;
		n = n1;
	}
	this->resize(n);
}

template <typename T> Aircraft::Poly1D<T>& Aircraft::Poly1D<T>::operator+=(T x)
{
	this->resize(std::max(this->size(), (typename Poly1D<T>::size_type)1U), (T)0);
	(*this)[0] += x;
	reduce();
	return *this;
}

template <typename T> Aircraft::Poly1D<T>& Aircraft::Poly1D<T>::operator-=(T x)
{
	this->resize(std::max(this->size(), (typename Poly1D<T>::size_type)1U), (T)0);
	(*this)[0] -= x;
	reduce();
	return *this;
}

template <typename T> Aircraft::Poly1D<T>& Aircraft::Poly1D<T>::operator*=(T x)
{
	for (typename Poly1D<T>::size_type i(0); i < this->size(); ++i)
		(*this)[i] *= x;
	reduce();
	return *this;
}

template <typename T> Aircraft::Poly1D<T>& Aircraft::Poly1D<T>::operator/=(T x)
{
	for (typename Poly1D<T>::size_type i(0); i < this->size(); ++i)
		(*this)[i] /= x;
	reduce();
	return *this;
}

template <typename T> Aircraft::Poly1D<T>& Aircraft::Poly1D<T>::operator+=(const Poly1D<T>& x)
{
	this->resize(std::max(this->size(), x.size()), (T)0);
	for (typename Poly1D<T>::size_type i(0); i < x.size(); ++i)
		(*this)[i] += x[i];
	reduce();
	return *this;
}

template <typename T> Aircraft::Poly1D<T>& Aircraft::Poly1D<T>::operator-=(const Poly1D<T>& x)
{
	this->resize(std::max(this->size(), x.size()), (T)0);
	for (typename Poly1D<T>::size_type i(0); i < x.size(); ++i)
		(*this)[i] -= x[i];
	reduce();
	return *this;
}

template <typename T> Aircraft::Poly1D<T> Aircraft::Poly1D<T>::operator*(const Poly1D<T>& x) const
{
	Poly1D<T> r;
	{
		typename Poly1D<T>::size_type n(this->size() + x.size());
		if (!n)
			return r;
		--n;
		r.resize(n, (T)0);
	}
	for (typename Poly1D<T>::size_type i(0); i < this->size(); ++i)
		for (typename Poly1D<T>::size_type j(0); j < x.size(); ++j)
			r[i + j] += (*this)[i] * x[j];
	r.reduce();
	return r;
}

template <typename T> Aircraft::Poly1D<T> Aircraft::Poly1D<T>::differentiate(void) const
{
	if (this->size() < 2U)
		return Poly1D<T>();
	Poly1D<T> r(this->size() - 1);
	for (typename Poly1D<T>::size_type i(1); i < this->size(); ++i)
		r[i - 1] = i * (*this)[i];
	r.reduce();
	return r;
}

template <typename T> Aircraft::Poly1D<T> Aircraft::Poly1D<T>::integrate(void) const
{
	if (!this->size())
		return Poly1D<T>();
	Poly1D<T> r(this->size() + 1);
	for (typename Poly1D<T>::size_type i(1); i <= this->size(); ++i)
		r[i] = (*this)[i - 1] / (T)i;
	r.reduce();
	return r;
}

template <typename T> Aircraft::Poly1D<T> Aircraft::Poly1D<T>::convert(unit_t ufrom, unit_t uto) const
{
	Poly1D<T> r(*this);
	r *= Aircraft::convert(ufrom, uto, 1);
	return r;
}

template <typename T> Aircraft::Poly1D<T> Aircraft::Poly1D<T>::simplify(double absacc, double relacc, double invmaxmag) const
{
	if (this->empty() || std::isnan((*this)[0]))
		return *this;
	if (std::isnan(absacc) || std::isnan(relacc) || std::isnan(invmaxmag))
		return *this;
	invmaxmag = fabs(invmaxmag);
	absacc = std::max(fabs(absacc), fabs((*this)[0] * relacc));
	Poly1D<T> r(*this);
	for (typename Poly1D<T>::size_type i(0); i < r.size(); ++i, absacc *= invmaxmag) {
		if (fabs(r[i]) < absacc)
			r[i] = 0;
	}
	r.reduce();
	return r;
}

template <typename T> typename Aircraft::Poly1D<T>::MinMax Aircraft::Poly1D<T>::get_minmax(T dmin, T dmax) const
{
	MinMax r;
	r.m_glbmin.set_x(dmin);
	r.m_glbmax.set_x(dmax);
	eval(r.m_glbmin);
	eval(r.m_glbmax);
	if (r.m_glbmin.get_y() > r.m_glbmax.get_y())
		std::swap(r.m_glbmin, r.m_glbmax);
	typedef typename std::vector<T> rroots_t;
	rroots_t rroots;
	{
		Poly1D<T> pd(this->differentiate());
		Eigen::Map<const Eigen::Matrix<T,Eigen::Dynamic,1> > poly(&pd[0], pd.size());
		Eigen::PolynomialSolver<T,Eigen::Dynamic> roots(poly);
		roots.realRoots(rroots);
	}
	for (typename rroots_t::const_iterator ri(rroots.begin()), re(rroots.end()); ri != re; ++ri) {
		XY rt(*ri);
		eval(rt);
		if (rt.get_y() > r.m_glbmax.get_y())
			r.m_glbmax = rt;
		if (rt.get_y() < r.m_glbmin.get_y())
			r.m_glbmin = rt;
		double d2(evalderiv2(*ri));
		if (d2 > 0)
			r.m_lclmax.insert(rt);
		else if (d2 < 0)
			r.m_lclmin.insert(rt);
	}
	return r;
}

template <typename T> void Aircraft::Poly1D<T>::load_xml(const xmlpp::Element *el)
{
	if (!el)
		return;
	const xmlpp::TextNode* txt(el->get_child_text());
	if (!txt)
		return;
	this->clear();
	std::istringstream iss(txt->get_content());
	while (!iss.eof()) {
		double v;
		iss >> v;
		if (iss.fail()) {
			std::cerr << "Aircraft::Poly1D::load_xml: poly parse error: "
				  << txt->get_content() << " order " << this->size() << std::endl;
			break;
		}
		this->push_back(v);
		if (iss.eof())
			break;
		char ch;
		iss >> ch;
		if (iss.fail()) {
			std::cerr << "Aircraft::Poly1D::load_xml: poly parse error: "
				  << txt->get_content() << " order " << this->size() << std::endl;
			break;
		}
		if (ch != ',') {
			std::cerr << "Aircraft::Poly1D::load_xml: poly delimiter error: "
				  << txt->get_content() << " order " << this->size() << std::endl;
			break;
		}
	}
}

template <typename T> void Aircraft::Poly1D<T>::save_xml(xmlpp::Element *el) const
{
	if (!el)
		return;
	std::ostringstream oss;
	oss << std::setprecision(10);
	bool subseq(false);
	for (typename Poly1D<T>::const_iterator pi(this->begin()), pe(this->end()); pi != pe; ++pi) {
		if (subseq)
			oss << ',';
		subseq = true;
		oss << *pi;
	}
	el->set_child_text(oss.str());
}

template <typename T> std::ostream& Aircraft::Poly1D<T>::save_octave(std::ostream& os, const Glib::ustring& name) const
{
	os << "# name: " << name << std::endl
	   << "# type: matrix" << std::endl
	   << "# rows: " << this->size() << std::endl
	   << "# columns: 1" << std::endl;
	for (typename Poly1D<T>::const_iterator i(this->begin()), e(this->end()); i != e; ++i) {
		std::ostringstream oss;
		oss << std::setprecision(10) << *i;
		os << oss.str() << std::endl;
	}
	return os;
}

template <typename T> std::ostream& Aircraft::Poly1D<T>::print(std::ostream& os) const
{
	bool subseq(false);
	for (typename Poly1D<T>::const_reverse_iterator i(this->rbegin()), e(this->rend()); i != e; ++i) {
		if (subseq)
			os << ' ';
		else
			subseq = true;
		std::ostringstream oss;
		oss << std::setprecision(10) << *i;
		os << oss.str();
	}
	return os;
}

template class Aircraft::Poly1D<double>;

Aircraft::OtherInfo::OtherInfo(const Glib::ustring& category, const Glib::ustring& text)
	: m_category(category), m_text(text)
{
}

bool Aircraft::OtherInfo::is_category_valid(const Glib::ustring& cat)
{
	Glib::ustring::const_iterator si(cat.begin()), se(cat.end());
	if (si == se)
		return false;
	if (!std::isalpha(*si))
		return false;
	for (++si; si != se; ++si)
		if (!std::isalnum(*si))
			return false;
	return true;
}

std::string Aircraft::OtherInfo::get_valid_text(const std::string& txt)
{
	typedef enum {
		spc_begin,
		spc_no,
		spc_yes
	} spc_t;
	spc_t spc(spc_begin);
	std::string r;
	r.reserve(txt.size());
	for (std::string::const_iterator si(txt.begin()), se(txt.end()); si != se; ++si) {
		if (std::isspace(*si) || *si == '/' || *si == '-') {
			if (spc != spc_begin)
				spc = spc_yes;
			continue;
		}
		if (*si < ' ' || *si >= 127)
			continue;
		if (spc == spc_yes)
			r.push_back(' ');
		spc = spc_no;
		r.push_back(*si);
	}
	return r;
}

Glib::ustring Aircraft::OtherInfo::get_valid_text(const Glib::ustring& txt)
{
	typedef enum {
		spc_begin,
		spc_no,
		spc_yes
	} spc_t;
	spc_t spc(spc_begin);
	Glib::ustring r;
	r.reserve(txt.size());
	for (Glib::ustring::const_iterator si(txt.begin()), se(txt.end()); si != se; ++si) {
		if (std::isspace(*si) || *si == '/' || *si == '-') {
			if (spc != spc_begin)
				spc = spc_yes;
			continue;
		}
		if (*si < ' ' || *si >= 127)
			continue;
		if (spc == spc_yes)
			r.push_back(' ');
		spc = spc_no;
		r.push_back(*si);
	}
	return r;
}

Aircraft::WeightBalance::Element::Unit::Unit(const Glib::ustring& name, double factor, double offset)
	: m_name(name), m_factor(factor), m_offset(offset)
{
}

void Aircraft::WeightBalance::Element::Unit::load_xml(const xmlpp::Element *el)
{
	if (!el)
		return;
	m_name.clear();
	m_factor = 0;
	m_offset = 0;
	xmlpp::Attribute *attr;
 	if ((attr = el->get_attribute("name")))
		m_name = attr->get_value();
	if ((attr = el->get_attribute("factor")))
		m_factor = Glib::Ascii::strtod(attr->get_value());
	if ((attr = el->get_attribute("offset")))
		m_offset = Glib::Ascii::strtod(attr->get_value());
}

void Aircraft::WeightBalance::Element::Unit::save_xml(xmlpp::Element *el) const
{
	if (!el)
		return;
	if (m_name.empty())
		el->remove_attribute("name");
	else
		el->set_attribute("name", m_name);
	if (m_factor == 0) {
		el->remove_attribute("factor");
	} else {
		std::ostringstream oss;
		oss << m_factor;
		el->set_attribute("factor", oss.str());
	}
	if (m_offset == 0) {
		el->remove_attribute("offset");
	} else {
		std::ostringstream oss;
		oss << m_offset;
		el->set_attribute("offset", oss.str());
	}
}

const Aircraft::WeightBalance::Element::FlagName Aircraft::WeightBalance::Element::flagnames[] =
{
	{ "fixed", flag_fixed },
	{ "binary", flag_binary },
	{ "avgas", flag_avgas },
	{ "jeta", flag_jeta },
	{ "diesel", flag_diesel },
	{ "oil", flag_oil },
	{ "gear", flag_gear },
	{ "consumable", flag_consumable },
	{ 0, flag_none }
};

Aircraft::WeightBalance::Element::Element(const Glib::ustring& name, double massmin, double massmax, double arm, flags_t flags)
	: m_name(name), m_massmin(massmin), m_massmax(massmax), m_flags(flags)
{
	if (!std::isnan(arm))
		m_arms[0.0] = arm;
}

void Aircraft::WeightBalance::Element::load_xml(const xmlpp::Element *el)
{
	if (!el)
		return;
	m_units.clear();
	m_name.clear();
	m_massmin = 0;
	m_massmax = 0;
	m_arms.clear();
	m_flags = flag_none;
	xmlpp::Attribute *attr;
 	if ((attr = el->get_attribute("name")))
		m_name = attr->get_value();
	if ((attr = el->get_attribute("massmin")))
		m_massmin = Glib::Ascii::strtod(attr->get_value());
	if ((attr = el->get_attribute("massmax")))
		m_massmax = Glib::Ascii::strtod(attr->get_value());
	if ((attr = el->get_attribute("arm")))
		m_arms[0.0] = Glib::Ascii::strtod(attr->get_value());
	if ((attr = el->get_attribute("flags"))) {
		Glib::ustring val(attr->get_value());
		Glib::ustring::size_type vn(0);
		while (vn < val.size()) {
			Glib::ustring::size_type vn2(val.find('|', vn));
			Glib::ustring v1(val.substr(vn, vn2 - vn));
			vn = vn2;
			if (vn < val.size())
				++vn;
			if (v1.empty())
				continue;
			if (*std::min_element(v1.begin(), v1.end()) >= (Glib::ustring::value_type)'0' &&
			    *std::max_element(v1.begin(), v1.end()) <= (Glib::ustring::value_type)'9') {
				m_flags |= (flags_t)strtol(v1.c_str(), 0, 10);
				continue;
			}
			const FlagName *fn = flagnames;
			for (; fn->m_name; ++fn)
				if (fn->m_name == v1)
					break;
			if (!fn->m_name) {
				std::cerr << "WeightBalance::Element::load_xml: invalid flag " << v1 << std::endl;
				continue;
			}
			m_flags |= fn->m_flag;
		}
	}
	{
		xmlpp::Node::NodeList nl(el->get_children("arms"));
		for (xmlpp::Node::NodeList::const_iterator ni(nl.begin()), ne(nl.end()); ni != ne; ++ni) {
			const xmlpp::Element *el2(static_cast<xmlpp::Element *>(*ni));
			double m(std::numeric_limits<double>::quiet_NaN()), a(std::numeric_limits<double>::quiet_NaN());
			if ((attr = el2->get_attribute("mass")))
				m = Glib::Ascii::strtod(attr->get_value());
			if ((attr = el2->get_attribute("arm")))
				a = Glib::Ascii::strtod(attr->get_value());
			if (!std::isnan(m) && !std::isnan(a))
				m_arms[m] = a;
		}
	}
	{
		xmlpp::Node::NodeList nl(el->get_children("unit"));
		for (xmlpp::Node::NodeList::const_iterator ni(nl.begin()), ne(nl.end()); ni != ne; ++ni) {
			m_units.push_back(Unit());
			m_units.back().load_xml(static_cast<xmlpp::Element *>(*ni));
		}
	}
}

void Aircraft::WeightBalance::Element::save_xml(xmlpp::Element *el) const
{
	if (!el)
		return;
	{
		xmlpp::Node::NodeList nl(el->get_children());
		for (xmlpp::Node::NodeList::const_iterator ni(nl.begin()), ne(nl.end()); ni != ne; ++ni)
			el->remove_child(*ni);
	}
	if (m_name.empty())
		el->remove_attribute("name");
	else
		el->set_attribute("name", m_name);
	if (m_massmin == 0) {
		el->remove_attribute("massmin");
	} else {
		std::ostringstream oss;
		oss << m_massmin;
		el->set_attribute("massmin", oss.str());
	}
	if (m_massmax == 0) {
		el->remove_attribute("massmax");
	} else {
		std::ostringstream oss;
		oss << m_massmax;
		el->set_attribute("massmax", oss.str());
	}
	{
		arms_t::const_iterator i(m_arms.begin());
		if (i != m_arms.end() && i->first == 0.0)
			++i;
		if (i == m_arms.end() && i != m_arms.begin()) {
			std::ostringstream oss;
			oss << m_arms.begin()->second;
			el->set_attribute("arm", oss.str());
		} else {
			el->remove_attribute("arm");
			for (arms_t::const_iterator ai(m_arms.begin()), ae(m_arms.end()); ai != ae; ++ai) {
				xmlpp::Element *el2(el->add_child("arms"));
				{
					std::ostringstream oss;
					oss << ai->first;
					el2->set_attribute("mass", oss.str());
				}
				{
					std::ostringstream oss;
					oss << ai->second;
					el2->set_attribute("arm", oss.str());
				}
			}
		}
	}
	{
		Glib::ustring flg;
		flags_t flags(m_flags);
		for (const FlagName *fn = flagnames; fn->m_name; ++fn) {
			if (fn->m_flag & ~flags)
				continue;
			flags &= ~fn->m_flag;
			flg += "|";
			flg += fn->m_name;
		}
		if (flags) {
			std::ostringstream oss;
			oss << '|' << (unsigned int)flags;
			flg += oss.str();
		}
		if (flg.empty())
			el->remove_attribute("flags");
		else
			el->set_attribute("flags", flg.substr(1));
	}
	for (units_t::const_iterator ui(m_units.begin()), ue(m_units.end()); ui != ue; ++ui)
		ui->save_xml(el->add_child("unit"));
}

double Aircraft::WeightBalance::Element::get_arm(double mass) const
{
	if (std::isnan(mass) || m_arms.empty())
		return std::numeric_limits<double>::quiet_NaN();
	arms_t::const_iterator iu(m_arms.lower_bound(mass));
	if (iu == m_arms.begin())
		return iu->second;
	arms_t::const_iterator il(iu);
	--il;
	if (iu == m_arms.end())
		return il->second;
	double t(mass - il->first);
	t /= (iu->first - il->first);
	return t * iu->second + (1 - t) * il->second;
}

std::pair<double,double> Aircraft::WeightBalance::Element::get_piecelimits(double mass, bool rounddown) const
{
	std::pair<double,double> r(std::numeric_limits<double>::quiet_NaN(), std::numeric_limits<double>::quiet_NaN());
	if (std::isnan(mass) || m_arms.empty())
		return r;
	arms_t::const_iterator iu(m_arms.lower_bound(mass));
	if (!rounddown && iu != m_arms.end() && mass == iu->first)
		++iu;
	if (iu == m_arms.begin()) {
		r.first = 0;
		r.second = iu->first;
		return r;
	}
	arms_t::const_iterator il(iu);
	--il;
	if (iu == m_arms.end()) {
		r.first = il->first;
		r.second = std::numeric_limits<double>::max();
		return r;
	}
	r.first = il->first;
	r.second = iu->first;
	return r;
}

void Aircraft::WeightBalance::Element::clear_units(void)
{
	m_units.clear();
}

void Aircraft::WeightBalance::Element::add_unit(const Glib::ustring& name, double factor, double offset)
{
	m_units.push_back(Unit(name, factor, offset));
	if (m_units.back().is_fixed())
		m_flags |= flag_fixed;
}

void Aircraft::WeightBalance::Element::add_auto_units(unit_t massunit, unit_t fuelunit, unit_t oilunit)
{
	if (!((1 << massunit) & unitmask_mass))
		return;
	flags_t flags(get_flags() & (flag_fuel | flag_oil));
	unit_t un(unit_invalid);
	unitmask_t um(unitmask_mass);
	switch (flags) {
	case flag_avgas:
	case flag_jeta:
	case flag_diesel:
		un = fuelunit;
		um |= unitmask_volume;
		break;

	case flag_oil:
		un = oilunit;
		um |= unitmask_volume;
		break;

	default:
		break;
	}
	add_unit(to_str(massunit, true), 1.0, 0);
	if ((1 << un) & unitmask_volume)
		add_unit(to_str(un, true), convert(un, massunit, 1.0, flags), 0);
	else
		un = unit_invalid;
	for (unit_t u(unit_kg); u < unit_invalid; u = (unit_t)(u + 1))
		if (u != un && u != massunit && ((1 << u) & um))
			add_unit(to_str(u, true), convert(u, massunit, 1.0, flags), 0);
}

Aircraft::WeightBalance::Envelope::Point::Point(double arm, double mass)
	: m_arm(arm), m_mass(mass)
{
}

/* twice the signed area of the triangle p0, p1, p2 */
double Aircraft::WeightBalance::Envelope::Point::area2(const Point& p0, const Point& p1, const Point& p2)
{
        double p2x = p2.get_arm();
        double p2y = p2.get_mass();
        double p0x = p0.get_arm() - p2x;
        double p0y = p0.get_mass() - p2y;
        double p1x = p1.get_arm() - p2x;
        double p1y = p1.get_mass() - p2y;
        return p0x * (p1y - p0y) - p0y * (p1x - p0x);
}

void Aircraft::WeightBalance::Envelope::Point::load_xml(const xmlpp::Element *el)
{
	if (!el)
		return;
	m_arm = 0;
	m_mass = 0;
	xmlpp::Attribute *attr;
 	if ((attr = el->get_attribute("arm")))
		m_arm = Glib::Ascii::strtod(attr->get_value());
 	if ((attr = el->get_attribute("mass")))
		m_mass = Glib::Ascii::strtod(attr->get_value());
}

void Aircraft::WeightBalance::Envelope::Point::save_xml(xmlpp::Element *el) const
{
	if (!el)
		return;
	if (m_arm == 0) {
		el->remove_attribute("arm");
	} else {
		std::ostringstream oss;
		oss << m_arm;
		el->set_attribute("arm", oss.str());
	}
	if (m_mass == 0) {
		el->remove_attribute("mass");
	} else {
		std::ostringstream oss;
		oss << m_mass;
		el->set_attribute("mass", oss.str());
	}
}

Aircraft::WeightBalance::Envelope::Envelope(const Glib::ustring& name)
	: m_name(name)
{
}

int Aircraft::WeightBalance::Envelope::windingnumber(const Point& pt) const
{
        int wn = 0; /* the winding number counter */

        /* loop through all edges of the polygon */
        unsigned int j = size() - 1;
        for (unsigned int i = 0; i < size(); j = i, i++) {
		const Point& pj(operator[](j));
 		const Point& pi(operator[](i));
                double lon = pt.area2(pj, pi);
		/* edge from V[j] to V[i] */
                if (pj.get_mass() <= pt.get_mass()) {
                        if (pi.get_mass() > pt.get_mass())
                                /* an upward crossing */
                                if (lon > 0)
                                        /* P left of edge: have a valid up intersect */
                                        wn++;
                } else {
                        if (pi.get_mass() <= pt.get_mass())
                                /* a downward crossing */
                                if (lon < 0)
                                        /* P right of edge: have a valid down intersect */
                                        wn--;
                }
        }
        return wn;
}

void Aircraft::WeightBalance::Envelope::get_bounds(double& minarm, double& maxarm, double& minmass, double& maxmass) const
{
	double mina(std::numeric_limits<double>::max());
	double maxa(std::numeric_limits<double>::min());
	double minm(mina);
	double maxm(maxa);
	for (env_t::const_iterator ei(m_env.begin()), ee(m_env.end()); ei != ee; ++ei) {
		const Point& el(*ei);
		mina = std::min(mina, el.get_arm());
		maxa = std::max(maxa, el.get_arm());
		minm = std::min(minm, el.get_mass());
		maxm = std::max(maxm, el.get_mass());
	}
	minarm = mina;
	maxarm = maxa;
	minmass = minm;
	maxmass = maxm;
}

void Aircraft::WeightBalance::Envelope::load_xml(const xmlpp::Element *el)
{
	if (!el)
		return;
	m_env.clear();
	m_name.clear();
	xmlpp::Attribute *attr;
 	if ((attr = el->get_attribute("name")))
		m_name = attr->get_value();
	{
		xmlpp::Node::NodeList nl(el->get_children("point"));
		for (xmlpp::Node::NodeList::const_iterator ni(nl.begin()), ne(nl.end()); ni != ne; ++ni) {
			m_env.push_back(Point());
			m_env.back().load_xml(static_cast<xmlpp::Element *>(*ni));
		}
	}
}

void Aircraft::WeightBalance::Envelope::save_xml(xmlpp::Element *el) const
{
	if (!el)
		return;
	if (m_name.empty())
		el->remove_attribute("name");
	else
		el->set_attribute("name", m_name);
	for (env_t::const_iterator ei(m_env.begin()), ee(m_env.end()); ei != ee; ++ei)
		ei->save_xml(el->add_child("point"));
}

void Aircraft::WeightBalance::Envelope::add_point(const Point& pt)
{
	m_env.push_back(pt);
}

void Aircraft::WeightBalance::Envelope::add_point(double arm, double mass)
{
	add_point(Point(arm, mass));
}

Aircraft::WeightBalance::WeightBalance(void)
	: m_armunit(unit_in), m_massunit(unit_lb)
{
	// Default: P28R-200
	m_elements.push_back(Element("Pilot and Front Pax", 0, 600, 85.5, Element::flag_none));
	m_elements.back().add_auto_units(m_massunit, unit_usgal, unit_quart);
	m_elements.push_back(Element("Rear Pax", 0, 600, 118.1, Element::flag_none));
	m_elements.back().add_auto_units(m_massunit, unit_usgal, unit_quart);
	m_elements.push_back(Element("Cargo", 0, 200, 142.8, Element::flag_none));
	m_elements.back().add_auto_units(m_massunit, unit_usgal, unit_quart);
	m_elements.push_back(Element("Fuel", 0, 50 * usgal_to_liter * avgas_density * kg_to_lb, 95, Element::flag_avgas));
	m_elements.back().add_auto_units(m_massunit, unit_usgal, unit_quart);
	m_elements.push_back(Element("Oil", 0, 15, 29.5, Element::flag_oil));
	m_elements.back().add_auto_units(m_massunit, unit_usgal, unit_quart);
	m_elements.push_back(Element("Gear", 0, 0.1, 8190, Element::flag_gear));
	m_elements.back().add_unit("Extended", 0, 0);
	m_elements.back().add_unit("Retracted", 0, 0.1);
	m_elements.push_back(Element("Basic Empty Mass", 1653.0, 1653.0, 84.69, Element::flag_fixed));
	m_envelopes.push_back(Envelope("Normal"));
	m_envelopes.back().add_point(81.0, 1600);
	m_envelopes.back().add_point(95.9, 1600);
	m_envelopes.back().add_point(95.9, 2600);
	m_envelopes.back().add_point(90.0, 2600);
	m_envelopes.back().add_point(81.0, 1925);
}

void Aircraft::WeightBalance::load_xml(const xmlpp::Element *el)
{
	if (!el)
		return;
	m_elements.clear();
	m_envelopes.clear();
	m_remark.clear();
	m_armunit = unit_invalid;
	m_massunit = unit_invalid;
	xmlpp::Attribute *attr;
 	if ((attr = el->get_attribute("armunitname")))
		m_armunit = parse_unit(attr->get_value());
 	if ((attr = el->get_attribute("massunitname")))
		m_massunit = parse_unit(attr->get_value());
	if (!((1 << m_armunit) & unitmask_length))
		m_armunit = unit_in;
	if (!((1 << m_massunit) & unitmask_mass))
		m_massunit = unit_lb;
	if ((attr = el->get_attribute("remark")))
		m_remark = attr->get_value();
	{
		xmlpp::Node::NodeList nl(el->get_children("element"));
		for (xmlpp::Node::NodeList::const_iterator ni(nl.begin()), ne(nl.end()); ni != ne; ++ni) {
			m_elements.push_back(Element());
			m_elements.back().load_xml(static_cast<xmlpp::Element *>(*ni));
		}
	}
	{
		xmlpp::Node::NodeList nl(el->get_children("envelope"));
		for (xmlpp::Node::NodeList::const_iterator ni(nl.begin()), ne(nl.end()); ni != ne; ++ni) {
			m_envelopes.push_back(Envelope());
			m_envelopes.back().load_xml(static_cast<xmlpp::Element *>(*ni));
		}
	}
}

void Aircraft::WeightBalance::save_xml(xmlpp::Element *el) const
{
	if (!el)
		return;
	if (m_armunit == unit_invalid)
		el->remove_attribute("armunitname");
	else
		el->set_attribute("armunitname", to_str(m_armunit));
	if (m_massunit == unit_invalid)
		el->remove_attribute("massunitname");
	else
		el->set_attribute("massunitname", to_str(m_massunit));
	if (m_remark.empty())
		el->remove_attribute("remark");
	else
		el->set_attribute("remark", m_remark);
	for (elements_t::const_iterator ei(m_elements.begin()), ee(m_elements.end()); ei != ee; ++ei)
		ei->save_xml(el->add_child("element"));
	for (envelopes_t::const_iterator ei(m_envelopes.begin()), ee(m_envelopes.end()); ei != ee; ++ei)
		ei->save_xml(el->add_child("envelope"));
}

void Aircraft::WeightBalance::clear_elements(void)
{
	m_elements.clear();
}

void Aircraft::WeightBalance::add_element(const Element& el)
{
	m_elements.push_back(el);
}

void Aircraft::WeightBalance::clear_envelopes(void)
{
	m_envelopes.clear();
}

void Aircraft::WeightBalance::add_envelope(const Envelope& el)
{
	m_envelopes.push_back(el);
}

const std::string& Aircraft::WeightBalance::get_armunitname(void) const
{
	return to_str(get_armunit());
}

const std::string& Aircraft::WeightBalance::get_massunitname(void) const
{
	return to_str(get_massunit());
}

double Aircraft::WeightBalance::get_useable_fuelmass(void) const
{
	double m(0);
	for (elements_t::const_iterator i(m_elements.begin()), e(m_elements.end()); i != e; ++i) {
		const Aircraft::WeightBalance::Element& el(*i);
		if (!(el.get_flags() & Aircraft::WeightBalance::Element::flag_fuel))
			continue;
		m += el.get_massmax() - el.get_massmin();
	}
	return m;
}

double Aircraft::WeightBalance::get_total_fuelmass(void) const
{
	double m(0);
	for (elements_t::const_iterator i(m_elements.begin()), e(m_elements.end()); i != e; ++i) {
		const Aircraft::WeightBalance::Element& el(*i);
		if (!(el.get_flags() & Aircraft::WeightBalance::Element::flag_fuel))
			continue;
		m += el.get_massmax();
	}
	return m;
}

double Aircraft::WeightBalance::get_useable_oilmass(void) const
{
	double m(0);
	for (elements_t::const_iterator i(m_elements.begin()), e(m_elements.end()); i != e; ++i) {
		const Aircraft::WeightBalance::Element& el(*i);
		if (!(el.get_flags() & Aircraft::WeightBalance::Element::flag_oil))
			continue;
		m += el.get_massmax() - el.get_massmin();
	}
	return m;
}

double Aircraft::WeightBalance::get_total_oilmass(void) const
{
	double m(0);
	for (elements_t::const_iterator i(m_elements.begin()), e(m_elements.end()); i != e; ++i) {
		const Aircraft::WeightBalance::Element& el(*i);
		if (!(el.get_flags() & Aircraft::WeightBalance::Element::flag_oil))
			continue;
		m += el.get_massmax();
	}
	return m;
}

double Aircraft::WeightBalance::get_min_mass(void) const
{
	double m(0);
	for (elements_t::const_iterator i(m_elements.begin()), e(m_elements.end()); i != e; ++i) {
		const Aircraft::WeightBalance::Element& el(*i);
		m += el.get_massmin();
	}
	return m;
}

Aircraft::WeightBalance::Element::flags_t Aircraft::WeightBalance::get_element_flags(void) const
{
	Element::flags_t flg(Element::flag_none);
	for (elements_t::const_iterator i(m_elements.begin()), e(m_elements.end()); i != e; ++i) {
		const Aircraft::WeightBalance::Element& el(*i);
		flg |= el.get_flags();
	}
	return flg;
}

bool Aircraft::WeightBalance::add_auto_units(unit_t fuelunit, unit_t oilunit)
{
	bool work(false);
	for (elements_t::iterator i(m_elements.begin()), e(m_elements.end()); i != e; ++i) {
		if (i->get_nrunits())
			continue;
		if (i->get_flags() & Element::flag_fixed) {
			i->add_unit(to_str(m_massunit), 1, 0);
			work = true;
			continue;
		}
		i->add_auto_units(m_massunit, fuelunit, oilunit);
		work = true;
	}
	return work;
}

void Aircraft::WeightBalance::set_units(unit_t armunit, unit_t massunit)
{
	bool armok(!!((1 << armunit) & unitmask_length));
	bool massok(!!((1 << massunit) & unitmask_mass));
	if (armok)
		m_armunit = armunit;
	if (massok)
		m_massunit = massunit;
	if (armok && massok)
		return;
	std::ostringstream oss;
	oss << "Aircraft::WeightBalance::set_units: invalid units for ";
	if (!armok)
		oss << "arm";
	if (!(armok || massok))
		oss << ", ";
	if (!massok)
		oss << "mass";
	throw std::runtime_error(oss.str());
}

void Aircraft::WeightBalance::limit(elementvalues_t& ev) const
{
	for (elements_t::size_type i(0), n(std::min(m_elements.size(), ev.size())); i < n; ++i) {
		const Element& el(m_elements[i]);
		ElementValue& evs(ev[i]);
		if (evs.get_unit() >= el.get_nrunits()) {
			evs.set_unit(0);
			if (!el.get_nrunits())
				continue;
		}
		const Element::Unit& unit(el.get_unit(evs.get_unit()));
		if (std::isnan(unit.get_factor()) || std::isnan(unit.get_offset()) || unit.get_factor() == 0)
			continue;
		double mass(evs.get_value() * unit.get_factor() + unit.get_offset());
		if (mass < el.get_massmin())
			mass = el.get_massmin();
		if (mass > el.get_massmax())
			mass = el.get_massmax();
		evs.set_value((mass - unit.get_offset()) / unit.get_factor());
	}
}

Aircraft::Distances::Distance::Point::Point(double da, double pa, double temp, double gnddist, double obstdist)
	: m_da(da), m_pa(pa), m_temp(temp), m_gnddist(gnddist), m_obstdist(obstdist)
{
}

void Aircraft::Distances::Distance::Point::load_xml(const xmlpp::Element *el, double altfactor, double tempfactor, double tempoffset, double distfactor)
{
	if (!el)
		return;
	m_da = m_pa = m_temp = m_gnddist = m_obstdist = std::numeric_limits<double>::quiet_NaN();
	xmlpp::Attribute *attr;
	if ((attr = el->get_attribute("da")))
		m_da = Glib::Ascii::strtod(attr->get_value()) * altfactor;
	if ((attr = el->get_attribute("pa")))
		m_pa = Glib::Ascii::strtod(attr->get_value()) * altfactor;
	if ((attr = el->get_attribute("temp")))
		m_temp = Glib::Ascii::strtod(attr->get_value()) * tempfactor + tempoffset;
	if ((attr = el->get_attribute("gnddist")))
		m_gnddist = Glib::Ascii::strtod(attr->get_value()) * distfactor;
	if ((attr = el->get_attribute("obstdist")))
		m_obstdist = Glib::Ascii::strtod(attr->get_value()) * distfactor;

}

void Aircraft::Distances::Distance::Point::save_xml(xmlpp::Element *el, double altfactor, double tempfactor, double tempoffset, double distfactor) const
{
	if (!el)
		return;
	if (std::isnan(m_da) || altfactor == 0) {
		el->remove_attribute("da");
	} else {
		std::ostringstream oss;
		oss << (m_da / altfactor);
		el->set_attribute("da", oss.str());
	}
	if (std::isnan(m_pa) || altfactor == 0) {
		el->remove_attribute("pa");
	} else {
		std::ostringstream oss;
		oss << (m_pa / altfactor);
		el->set_attribute("pa", oss.str());
	}
	if (std::isnan(m_temp) || tempfactor == 0) {
		el->remove_attribute("temp");
	} else {
		std::ostringstream oss;
		oss << ((m_temp - tempoffset) / tempfactor);
		el->set_attribute("temp", oss.str());
	}
	if (std::isnan(m_gnddist) || distfactor == 0) {
		el->remove_attribute("gnddist");
	} else {
		std::ostringstream oss;
		oss << (m_gnddist / distfactor);
		el->set_attribute("gnddist", oss.str());
	}
	if (std::isnan(m_obstdist) || distfactor == 0) {
		el->remove_attribute("obstdist");
	} else {
		std::ostringstream oss;
		oss << (m_obstdist / distfactor);
		el->set_attribute("obstdist", oss.str());
	}
}

double Aircraft::Distances::Distance::Point::get_densityalt(void) const
{
	if (!std::isnan(m_da))
		return m_da;
	if (std::isnan(m_pa))
		return std::numeric_limits<double>::quiet_NaN();
	float temp, press;
	IcaoAtmosphere<float>::std_altitude_to_pressure(&press, &temp, m_pa);
	if (!std::isnan(m_temp))
		temp = m_temp;
	return IcaoAtmosphere<float>::std_density_to_altitude(IcaoAtmosphere<float>::pressure_to_density(press, temp));
}

Aircraft::Distances::Distance::Distance(const Glib::ustring& name, double vrotate, double mass)
	: m_name(name), m_vrotate(vrotate), m_mass(mass)
{
}

#ifdef HAVE_EIGEN3

namespace {

#ifdef __WIN32__
__attribute__((force_align_arg_pointer))
#endif

Eigen::VectorXd least_squares_solution(const Eigen::MatrixXd& A, const Eigen::VectorXd& b)
{
	if (false) {
		// traditional
		return (A.transpose() * A).inverse() * A.transpose() * b;
	} else if (true) {
		// cholesky
		return (A.transpose() * A).ldlt().solve(A.transpose() * b);
	} else if (false) {
		// householder QR
		return A.colPivHouseholderQr().solve(b);
	} else if (false) {
		// jacobi SVD
		return A.jacobiSvd(Eigen::ComputeThinU | Eigen::ComputeThinV).solve(b);
	} else {
		typedef Eigen::JacobiSVD<Eigen::MatrixXd, Eigen::FullPivHouseholderQRPreconditioner> jacobi_t;
		jacobi_t jacobi(A, Eigen::ComputeFullU | Eigen::ComputeFullV);
		return jacobi.solve(b);
	}
}

};

#endif

#ifdef __WIN32__
__attribute__((force_align_arg_pointer))
#endif
bool Aircraft::Distances::Distance::recalculatepoly(bool force)
{
#ifdef HAVE_EIGEN3
	if (!force && !m_gndpoly.empty() && !m_obstpoly.empty())
		return false;
	if (m_points.empty()) {
		m_gndpoly.clear();
		m_obstpoly.clear();
		return false;
	}
	unsigned int polyorder(4);
	unsigned int ptgnd(0);
	unsigned int ptobst(0);
	Eigen::MatrixXd mg(m_points.size(), polyorder);
	Eigen::MatrixXd mo(m_points.size(), polyorder);
	Eigen::VectorXd vg(m_points.size());
	Eigen::VectorXd vo(m_points.size());
	for (points_t::const_iterator pi(m_points.begin()), pe(m_points.end()); pi != pe; ++pi) {
		double da(pi->get_densityalt());
		if (std::isnan(da))
			continue;
		double d(pi->get_gnddist());
		if (!std::isnan(d)) {
			double da1(1);
			for (unsigned int i = 0; i < polyorder; ++i, da1 *= da)
				mg(ptgnd, i) = da1;
			vg(ptgnd) = d;
			++ptgnd;
		}
		d = pi->get_obstdist();
		if (!std::isnan(d)) {
			double da1(1);
			for (unsigned int i = 0; i < polyorder; ++i, da1 *= da)
				mo(ptobst, i) = da1;
			vo(ptobst) = d;
			++ptobst;
		}
	}
	bool ret(false);
	if (force || m_gndpoly.empty()) {
		m_gndpoly.clear();
		if (ptgnd) {
			ret = true;
			mg.conservativeResize(ptgnd, std::min(ptgnd, polyorder));
			vg.conservativeResize(ptgnd);
			Eigen::VectorXd x(least_squares_solution(mg, vg));
			poly_t p(x.data(), x.data() + x.size());
			m_gndpoly.swap(p);
		}
	}
	if (force || m_obstpoly.empty()) {
		m_obstpoly.clear();
		if (ptobst) {
			ret = true;
			mo.conservativeResize(ptobst, std::min(ptobst, polyorder));
			vo.conservativeResize(ptobst);
			Eigen::VectorXd x(least_squares_solution(mo, vo));
			poly_t p(x.data(), x.data() + x.size());
			m_obstpoly.swap(p);
		}
	}
	return ret;
#else
	return false;
#endif
}

void Aircraft::Distances::Distance::add_point_da(double gnddist, double obstdist, double da)
{
	m_points.push_back(Point(da, std::numeric_limits<double>::quiet_NaN(), std::numeric_limits<double>::quiet_NaN(), gnddist, obstdist));
}

void Aircraft::Distances::Distance::add_point_pa(double gnddist, double obstdist, double pa, double temp)
{
	m_points.push_back(Point(std::numeric_limits<double>::quiet_NaN(), pa, temp, gnddist, obstdist));
}

void Aircraft::Distances::Distance::load_xml(const xmlpp::Element *el, double altfactor, double tempfactor, double tempoffset, double distfactor)
{
	if (!el)
		return;
	m_name.clear();
	m_vrotate = 0;
	m_gndpoly.clear();
	m_obstpoly.clear();
	m_points.clear();
	xmlpp::Attribute *attr;
 	if ((attr = el->get_attribute("name")))
		m_name = attr->get_value();
 	if ((attr = el->get_attribute("vrotate")))
		m_vrotate = Glib::Ascii::strtod(attr->get_value());
 	if ((attr = el->get_attribute("mass")))
		m_mass = Glib::Ascii::strtod(attr->get_value());
	{
		xmlpp::Node::NodeList nl(el->get_children("point"));
		for (xmlpp::Node::NodeList::const_iterator ni(nl.begin()), ne(nl.end()); ni != ne; ++ni) {
			m_points.push_back(Point());
			m_points.back().load_xml(static_cast<xmlpp::Element *>(*ni), altfactor, tempfactor, tempoffset, distfactor);
		}
	}
	{
		xmlpp::Node::NodeList nl(el->get_children("gndpoly"));
		for (xmlpp::Node::NodeList::const_iterator ni(nl.begin()), ne(nl.end()); ni != ne; ++ni)
			m_gndpoly.load_xml(static_cast<xmlpp::Element *>(*ni));
 	}
	{
		xmlpp::Node::NodeList nl(el->get_children("obstpoly"));
		for (xmlpp::Node::NodeList::const_iterator ni(nl.begin()), ne(nl.end()); ni != ne; ++ni)
			m_obstpoly.load_xml(static_cast<xmlpp::Element *>(*ni));
	}
}

void Aircraft::Distances::Distance::save_xml(xmlpp::Element *el, double altfactor, double tempfactor, double tempoffset, double distfactor) const
{
	if (!el)
		return;
	if (m_name.empty())
		el->remove_attribute("name");
	else
		el->set_attribute("name", m_name);
	if (m_vrotate == 0) {
		el->remove_attribute("vrotate");
	} else {
		std::ostringstream oss;
		oss << m_vrotate;
		el->set_attribute("vrotate", oss.str());
	}
	if (m_mass == 0) {
		el->remove_attribute("mass");
	} else {
		std::ostringstream oss;
		oss << m_mass;
		el->set_attribute("mass", oss.str());
	}
	for (points_t::const_iterator di(m_points.begin()), de(m_points.end()); di != de; ++di)
		di->save_xml(el->add_child("point"), altfactor, tempfactor, tempoffset, distfactor);
	{
		xmlpp::Node::NodeList nl(el->get_children("gndpoly"));
		for (xmlpp::Node::NodeList::const_iterator ni(nl.begin()), ne(nl.end()); ni != ne; ++ni)
			el->remove_child(*ni);
	}
	if (!m_gndpoly.empty())
		m_gndpoly.save_xml(el->add_child("gndpoly"));
 	{
		xmlpp::Node::NodeList nl(el->get_children("obstpoly"));
		for (xmlpp::Node::NodeList::const_iterator ni(nl.begin()), ne(nl.end()); ni != ne; ++ni)
			el->remove_child(*ni);
	}
	if (!m_obstpoly.empty())
		m_obstpoly.save_xml(el->add_child("obstpoly"));
}

double Aircraft::Distances::Distance::calculate_gnddist(double da) const
{
	return m_gndpoly.eval(da);
}

double Aircraft::Distances::Distance::calculate_obstdist(double da) const
{
	return m_obstpoly.eval(da);
}

void Aircraft::Distances::Distance::calculate(double& gnddist, double& obstdist, double da, double headwind) const
{
	calculate(gnddist, obstdist, da, headwind, get_mass());
}

void Aircraft::Distances::Distance::calculate(double& gnddist, double& obstdist, double da, double headwind, double mass) const
{
	double mmul(1);
	if (get_mass() > 0)
		mmul = std::max(0.1, std::min(10.0, mass / get_mass()));
	double mul(1);
	if (get_vrotate() > 0)
		mul = std::max(0.5, (get_vrotate() - headwind) / get_vrotate());
	double mul2(mul * mul);
	double gd(calculate_gnddist(da));
	double od(calculate_obstdist(da));
	if (std::isnan(gd)) {
		gnddist = gd;
		obstdist = od;
		return;
	}
	if (false)
		std::cerr << "Aircraft: Distance: " << m_name << ": da " << da << " MTOM gnd: " << gd << " obst " << od << std::endl;
	od -= gd;
	od *= mul;
	gd *= mul2 * mmul;
	od += gd;
	gnddist = gd;
	obstdist = od;
	if (false)
		std::cerr << "Aircraft: Distance: " << m_name << ": da " << da << " mass: " << mass << " headwind: " << headwind
			  << " mmul: " << mmul << " mul: " << mul << " gnd: " << gd << " obst " << od << std::endl;
}

Aircraft::Distances::Distances(unit_t distunit, double altfactor, double tempfactor, double tempoffset, double distfactor)
	: m_altfactor(altfactor), m_tempfactor(tempfactor), m_tempoffset(tempoffset), m_distfactor(distfactor), m_distunit(distunit)
{
	// Default: P28R-200
	static const int16_t p28r200_tkoffdist[][4] = {
		{  756, 1022, 1578, 1800 },
		{  867, 1156, 1800, 2022 },
		{  956, 1289, 2000, 2267 },
		{ 1067, 1422, 2244, 2533 },
		{ 1200, 1600, 2533, 2867 },
		{ 1311, 1755, 2889, 3244 },
		{ 1489, 1978, 3311, 3689 },
		{ 1667, 2200, 3867, 4244 },
		{ 1867, 2467, 4556, 5000 }
	};
	static const int16_t p28r200_ldgdist[][2] = {
		{ 773, 1351 },
		{ 800, 1369 },
		{ 827, 1396 },
		{ 853, 1431 },
		{ 880, 1449 },
		{ 898, 1476 },
		{ 924, 1502 },
		{ 951, 1520 }
	};
	if (!((1 << m_distunit) & unitmask_length))
		m_distunit = unit_m;
	m_takeoffdist.push_back(Distance("Flaps 25", 56, 2600));
	for (unsigned int i = 0; i < sizeof(p28r200_tkoffdist)/sizeof(p28r200_tkoffdist[0]); ++i)
		m_takeoffdist.back().add_point_da(convert(unit_ft, m_distunit, p28r200_tkoffdist[i][0]), convert(unit_ft, m_distunit, p28r200_tkoffdist[i][2]), i * 1000);
	m_takeoffdist.push_back(Distance("Flaps 0", 56, 2600));
	for (unsigned int i = 0; i < sizeof(p28r200_tkoffdist)/sizeof(p28r200_tkoffdist[0]); ++i)
		m_takeoffdist.back().add_point_da(convert(unit_ft, m_distunit, p28r200_tkoffdist[i][1]), convert(unit_ft, m_distunit, p28r200_tkoffdist[i][3]), i * 1000);
	m_ldgdist.push_back(Distance("Flaps 40", 56, 2600));
	for (unsigned int i = 0; i < sizeof(p28r200_ldgdist)/sizeof(p28r200_ldgdist[0]); ++i)
		m_ldgdist.back().add_point_da(convert(unit_ft, m_distunit, p28r200_ldgdist[i][0]), convert(unit_ft, m_distunit, p28r200_ldgdist[i][1]), i * 1000);
	recalculatepoly(true);
}

bool Aircraft::Distances::recalculatepoly(bool force)
{
	bool ret(false);
	for (distances_t::iterator di(m_takeoffdist.begin()), de(m_takeoffdist.end()); di != de; ++di)
		ret = di->recalculatepoly(force) || ret;
	for (distances_t::iterator di(m_ldgdist.begin()), de(m_ldgdist.end()); di != de; ++di)
		ret = di->recalculatepoly(force) || ret;
	return ret;
}

void Aircraft::Distances::clear_takeoffdists(void)
{
	m_takeoffdist.clear();
}

void Aircraft::Distances::add_takeoffdist(const Distance& d)
{
	m_takeoffdist.push_back(d);
}

void Aircraft::Distances::clear_ldgdists(void)
{
	m_ldgdist.clear();
}

void Aircraft::Distances::add_ldgdist(const Distance& d)
{
	m_ldgdist.push_back(d);
}

void Aircraft::Distances::load_xml(const xmlpp::Element *el)
{
	if (!el)
		return;
	m_takeoffdist.clear();
	m_ldgdist.clear();
	m_remark.clear();
	m_altfactor = 1;
	m_tempfactor = 1;
	m_tempoffset = 273.15;
	m_distfactor = std::numeric_limits<double>::quiet_NaN();
	m_distunit = unit_m;
	xmlpp::Attribute *attr;
 	if ((attr = el->get_attribute("altfactor")))
		m_altfactor = Glib::Ascii::strtod(attr->get_value());
 	if ((attr = el->get_attribute("tempfactor")))
		m_tempfactor = Glib::Ascii::strtod(attr->get_value());
 	if ((attr = el->get_attribute("tempoffset")))
		m_tempoffset = Glib::Ascii::strtod(attr->get_value());
 	if ((attr = el->get_attribute("distfactor")))
		m_distfactor = Glib::Ascii::strtod(attr->get_value());
	if ((attr = el->get_attribute("unitname")))
		m_distunit = parse_unit(attr->get_value());
	if (!((1 << m_distunit) & unitmask_length))
		m_distunit = unit_m;
	if (std::isnan(m_distfactor))
		m_distfactor = convert(m_distunit, unit_m, 1.0);
	if ((attr = el->get_attribute("remark")))
		m_remark = attr->get_value();
	{
		xmlpp::Node::NodeList nl(el->get_children("tkoffdist"));
		for (xmlpp::Node::NodeList::const_iterator ni(nl.begin()), ne(nl.end()); ni != ne; ++ni) {
			m_takeoffdist.push_back(Distance());
			m_takeoffdist.back().load_xml(static_cast<xmlpp::Element *>(*ni), m_altfactor, m_tempfactor, m_tempoffset, m_distfactor);
		}
	}
	{
		xmlpp::Node::NodeList nl(el->get_children("ldgdist"));
		for (xmlpp::Node::NodeList::const_iterator ni(nl.begin()), ne(nl.end()); ni != ne; ++ni) {
			m_ldgdist.push_back(Distance());
			m_ldgdist.back().load_xml(static_cast<xmlpp::Element *>(*ni), m_altfactor, m_tempfactor, m_tempoffset, m_distfactor);
		}
	}
	if ((attr = el->get_attribute("vrotate"))) {
		double vrot(Glib::Ascii::strtod(attr->get_value()));
		if (!std::isnan(vrot) && vrot > 0) {
			for (distances_t::iterator i(m_takeoffdist.begin()), e(m_takeoffdist.end()); i != e; ++i)
				if (std::isnan(i->get_vrotate()) || i->get_vrotate() <= 0)
					i->set_vrotate(vrot);
			for (distances_t::iterator i(m_ldgdist.begin()), e(m_ldgdist.end()); i != e; ++i)
				if (std::isnan(i->get_vrotate()) || i->get_vrotate() <= 0)
					i->set_vrotate(vrot);
		}
	}
	if ((attr = el->get_attribute("mass"))) {
		double m(Glib::Ascii::strtod(attr->get_value()));
		if (!std::isnan(m) && m > 0) {
			for (distances_t::iterator i(m_takeoffdist.begin()), e(m_takeoffdist.end()); i != e; ++i)
				if (std::isnan(i->get_mass()) || i->get_mass() <= 0)
					i->set_mass(m);
			for (distances_t::iterator i(m_ldgdist.begin()), e(m_ldgdist.end()); i != e; ++i)
				if (std::isnan(i->get_mass()) || i->get_mass() <= 0)
					i->set_mass(m);
		}
	}
}

void Aircraft::Distances::save_xml(xmlpp::Element *el) const
{
	if (!el)
		return;
	if (m_altfactor == 1) {
		el->remove_attribute("altfactor");
	} else {
		std::ostringstream oss;
		oss << m_altfactor;
		el->set_attribute("altfactor", oss.str());
	}
	if (m_tempfactor == 1) {
		el->remove_attribute("tempfactor");
	} else {
		std::ostringstream oss;
		oss << m_tempfactor;
		el->set_attribute("tempfactor", oss.str());
	}
	if (m_tempoffset == 273.15) {
		el->remove_attribute("tempoffset");
	} else {
		std::ostringstream oss;
		oss << m_tempoffset;
		el->set_attribute("tempoffset", oss.str());
	}
	{
		bool dfltdistf(m_distfactor == 1);
		if (m_distunit != unit_invalid) {
			double distf(convert(m_distunit, unit_m, 1.0));
			dfltdistf = fabs(m_distfactor - distf) < 1e-6 * distf;
		}
		if (dfltdistf) {
			el->remove_attribute("distfactor");
		} else {
			std::ostringstream oss;
			oss << m_distfactor;
			el->set_attribute("distfactor", oss.str());
		}
	}
	if (m_distunit == unit_invalid)
		el->remove_attribute("unitname");
	else
		el->set_attribute("unitname", to_str(m_distunit));
	if (m_remark.empty())
		el->remove_attribute("remark");
	else
		el->set_attribute("remark", m_remark);
	for (distances_t::const_iterator di(m_takeoffdist.begin()), de(m_takeoffdist.end()); di != de; ++di)
		di->save_xml(el->add_child("tkoffdist"), m_altfactor, m_tempfactor, m_tempoffset, m_distfactor);
	for (distances_t::const_iterator di(m_ldgdist.begin()), de(m_ldgdist.end()); di != de; ++di)
		di->save_xml(el->add_child("ldgdist"), m_altfactor, m_tempfactor, m_tempoffset, m_distfactor);
}

Aircraft::ClimbDescent::Point::Point(double pa, double rate, double fuelflow, double cas)
	: m_pa(pa), m_rate(rate), m_fuelflow(fuelflow), m_cas(cas)
{
}
			
void Aircraft::ClimbDescent::Point::load_xml(const xmlpp::Element *el, mode_t& mode, double altfactor,
					     double ratefactor, double fuelflowfactor, double casfactor,
					     double timefactor, double fuelfactor, double distfactor)
{
	if (!el)
		return;
	m_pa = m_rate = m_fuelflow = std::numeric_limits<double>::quiet_NaN();
	xmlpp::Attribute *attr;
	if ((attr = el->get_attribute("pa")) || (attr = el->get_attribute("da")))
		m_pa = Glib::Ascii::strtod(attr->get_value()) * altfactor;
	if (mode == mode_rateofclimb || mode == mode_invalid) {
		if ((attr = el->get_attribute("rate"))) {
			m_rate = Glib::Ascii::strtod(attr->get_value()) * ratefactor;
			mode = mode_rateofclimb;
		}
		if ((attr = el->get_attribute("fuelflow"))) {
			m_fuelflow = Glib::Ascii::strtod(attr->get_value()) * fuelflowfactor;
			mode = mode_rateofclimb;
		}
		if ((attr = el->get_attribute("cas"))) {
			m_cas = Glib::Ascii::strtod(attr->get_value()) * casfactor;
			mode = mode_rateofclimb;
		}
	}
	if (mode == mode_timetoaltitude || mode == mode_invalid) {
		if ((attr = el->get_attribute("time"))) {
			m_rate = Glib::Ascii::strtod(attr->get_value()) * timefactor;
			mode = mode_timetoaltitude;
		}
		if ((attr = el->get_attribute("fuel"))) {
			m_fuelflow = Glib::Ascii::strtod(attr->get_value()) * fuelfactor;
			mode = mode_timetoaltitude;
		}
		if ((attr = el->get_attribute("dist"))) {
			m_cas = Glib::Ascii::strtod(attr->get_value()) * distfactor;
			mode = mode_timetoaltitude;
		} else if ((attr = el->get_attribute("distance"))) {
			m_cas = Glib::Ascii::strtod(attr->get_value()) * distfactor;
			mode = mode_timetoaltitude;
		}
	}
}

void Aircraft::ClimbDescent::Point::save_xml(xmlpp::Element *el, mode_t mode, double altfactor,
					     double ratefactor, double fuelflowfactor, double casfactor,
					     double timefactor, double fuelfactor, double distfactor) const
{
	if (!el)
		return;
	if (std::isnan(m_pa) || altfactor == 0) {
		el->remove_attribute("pa");
	} else {
		std::ostringstream oss;
		oss << (m_pa / altfactor);
		el->set_attribute("pa", oss.str());
	}
	switch (mode) {
	case mode_rateofclimb:
		if (std::isnan(m_rate) || ratefactor == 0) {
			el->remove_attribute("rate");
		} else {
			std::ostringstream oss;
			oss << (m_rate / ratefactor);
			el->set_attribute("rate", oss.str());
		}
		if (std::isnan(m_fuelflow) || fuelflowfactor == 0) {
			el->remove_attribute("fuelflow");
		} else {
			std::ostringstream oss;
			oss << (m_fuelflow / fuelflowfactor);
			el->set_attribute("fuelflow", oss.str());
		}
		if (std::isnan(m_cas) || casfactor == 0) {
			el->remove_attribute("cas");
		} else {
			std::ostringstream oss;
			oss << (m_cas / casfactor);
			el->set_attribute("cas", oss.str());
		}
		break;

	case mode_timetoaltitude:
		if (std::isnan(m_rate) || timefactor == 0) {
			el->remove_attribute("time");
		} else {
			std::ostringstream oss;
			oss << (m_rate / timefactor);
			el->set_attribute("time", oss.str());
		}
		if (std::isnan(m_fuelflow) || fuelfactor == 0) {
			el->remove_attribute("fuel");
		} else {
			std::ostringstream oss;
			oss << (m_fuelflow / fuelfactor);
			el->set_attribute("fuel", oss.str());
		}
		if (std::isnan(m_cas) || distfactor == 0) {
			el->remove_attribute("dist");
		} else {
			std::ostringstream oss;
			oss << (m_cas / distfactor);
			el->set_attribute("dist", oss.str());
		}
		break;

	default:
		break;
	}
}

std::ostream& Aircraft::ClimbDescent::Point::print(std::ostream& os, mode_t mode) const
{
	switch (mode) {
	case mode_rateofclimb:
		return os << "rate " << get_rate() << " ff " << get_fuelflow() << " cas " << get_cas();

	case mode_timetoaltitude:
		return os << "time " << get_time() << " fuel " << get_fuel() << " dist " << get_dist();

	default:
		return os << "?";
	}
}

const std::string& to_str(Aircraft::ClimbDescent::mode_t m)
{
	switch (m) {
	case Aircraft::ClimbDescent::mode_rateofclimb:
	{
		static const std::string r("rateofclimb");
		return r;
	}

	case Aircraft::ClimbDescent::mode_timetoaltitude:
	{
		static const std::string r("timetoaltitude");
		return r;
	}

	default:
	case Aircraft::ClimbDescent::mode_invalid:
	{
		static const std::string r("");
		return r;
	}
	}
}

Aircraft::ClimbDescent::ClimbDescent(const std::string& name, double mass, double isaoffs)
	: m_name(name), m_mass(mass), m_isaoffset(isaoffs),
	  m_ceiling(18000), m_climbtime(3600), m_mode(mode_rateofclimb)
{
	// PA28R-200
	m_points.push_back(Point(0, 910, 14, 95 * 0.86897624));
	m_points.push_back(Point(18000, 0, 8, 95 * 0.86897624));
	recalculatepoly(false);
}

void Aircraft::ClimbDescent::calculate(double& rate, double& fuelflow, double& cas, double da) const
{
	rate = m_ratepoly.eval(da);
	fuelflow = m_fuelflowpoly.eval(da);
	cas = m_caspoly.eval(da);
}

double Aircraft::ClimbDescent::time_to_altitude(double t) const
{
	return m_climbaltpoly.eval(t);
}

double Aircraft::ClimbDescent::time_to_distance(double t) const
{
	return m_climbdistpoly.eval(t);
}

double Aircraft::ClimbDescent::time_to_fuel(double t) const
{
	return m_climbfuelpoly.eval(t);
}

double Aircraft::ClimbDescent::time_to_climbrate(double t) const
{
	return m_climbaltpoly.evalderiv(t) * 60.0;
}

double Aircraft::ClimbDescent::time_to_tas(double t) const
{
	return m_climbdistpoly.evalderiv(t) * 3600.0;
}

double Aircraft::ClimbDescent::time_to_fuelflow(double t) const
{
	return m_climbfuelpoly.evalderiv(t) * 3600.0;
}

double Aircraft::ClimbDescent::altitude_to_time(double a) const
{
	return m_climbaltpoly.boundedinveval(a, 0, get_climbtime());
}

double Aircraft::ClimbDescent::distance_to_time(double d) const
{
	return m_climbdistpoly.boundedinveval(d, 0, get_climbtime());
}

#ifdef __WIN32__
__attribute__((force_align_arg_pointer))
#endif
bool Aircraft::ClimbDescent::recalculateratepoly(bool force)
{
	if (std::isnan(m_ceiling) || m_ceiling <= 0) {
		m_ceiling = 0;
		for (points_t::const_iterator pi(m_points.begin()), pe(m_points.end()); pi != pe; ++pi) {
			double pa(pi->get_pressurealt());
			if (std::isnan(pa))
				continue;
			m_ceiling = std::max(m_ceiling, pa);
		}
	}
	m_ceiling = std::min(m_ceiling, 50000.);
#ifdef HAVE_EIGEN3
	if (!force && !m_ratepoly.empty() && !m_fuelflowpoly.empty() && !m_caspoly.empty())
		return false;
	if (m_points.empty()) {
		m_ratepoly.clear();
		m_fuelflowpoly.clear();
		m_caspoly.clear();
		return false;
	}
	unsigned int polyorder(4);
	unsigned int ptrate(0);
	unsigned int ptfuel(0);
	unsigned int ptcas(0);
	Eigen::MatrixXd mr(m_points.size(), polyorder);
	Eigen::MatrixXd mf(m_points.size(), polyorder);
	Eigen::MatrixXd mc(m_points.size(), polyorder);
	Eigen::VectorXd vr(m_points.size());
	Eigen::VectorXd vf(m_points.size());
	Eigen::VectorXd vc(m_points.size());
	{
		typedef std::map<double,double> dmap_t;
		dmap_t rmap, fmap, cmap;
		for (points_t::const_iterator pi(m_points.begin()), pe(m_points.end()); pi != pe; ++pi) {
			if (false)
				std::cerr << "climbpoint: pa " << pi->get_pressurealt() << " rate " << pi->get_rate()
					  << " ff " << pi->get_fuelflow() << " cas " << pi->get_cas() << std::endl;
			double pa(pi->get_pressurealt());
			if (std::isnan(pa))
				continue;
			double d(pi->get_rate());
			if (!std::isnan(d)) {
				std::pair<dmap_t::iterator,bool> ins(rmap.insert(dmap_t::value_type(pa, d)));
				if (!ins.second) {
					std::cerr << "Duplicate entry for PA " << pa << " new rate value " << d
						  << " previous " << ins.first->second << std::endl;
					ins.first->second = (ins.first->second + d) * 0.5;
				}
			}
			d = pi->get_fuelflow();
			if (!std::isnan(d)) {
				std::pair<dmap_t::iterator,bool> ins(fmap.insert(dmap_t::value_type(pa, d)));
				if (!ins.second) {
					std::cerr << "Duplicate entry for PA " << pa << " new ff value " << d
						  << " previous " << ins.first->second << std::endl;
					ins.first->second = (ins.first->second + d) * 0.5;
				}
			}
			d = pi->get_cas();
			if (!std::isnan(d)) {
				std::pair<dmap_t::iterator,bool> ins(cmap.insert(dmap_t::value_type(pa, d)));
				if (!ins.second) {
					std::cerr << "Duplicate entry for PA " << pa << " new cas value " << d
						  << " previous " << ins.first->second << std::endl;
					ins.first->second = (ins.first->second + d) * 0.5;
				}
			}
		}
		for (dmap_t::const_iterator i(rmap.begin()), e(rmap.end()); i != e; ++i) {
			double pa1(1), pa(i->first);
			for (unsigned int i = 0; i < polyorder; ++i, pa1 *= pa)
				mr(ptrate, i) = pa1;
			vr(ptrate) = i->second;
			++ptrate;
		}
		for (dmap_t::const_iterator i(fmap.begin()), e(fmap.end()); i != e; ++i) {
			double pa1(1), pa(i->first);
			for (unsigned int i = 0; i < polyorder; ++i, pa1 *= pa)
				mf(ptfuel, i) = pa1;
			vf(ptfuel) = i->second;
			++ptfuel;
		}
		for (dmap_t::const_iterator i(cmap.begin()), e(cmap.end()); i != e; ++i) {
			double pa1(1), pa(i->first);
			for (unsigned int i = 0; i < polyorder; ++i, pa1 *= pa)
				mc(ptcas, i) = pa1;
			vc(ptcas) = i->second;
			++ptcas;
		}
	}
	bool ret(false);
	if (force || m_ratepoly.empty()) {
		m_ratepoly.clear();
		if (ptrate) {
			ret = true;
			//mr.conservativeResize(ptrate, std::min(ptrate, polyorder));
			//vr.conservativeResize(ptrate);
			mr = mr.topLeftCorner(ptrate, std::min(ptrate, polyorder)).eval();
			vr = vr.head(ptrate).eval();
			Eigen::VectorXd x(least_squares_solution(mr, vr));
			if (false)
				std::cerr << "mr [ " << mr << " ]; vr [ " << vr << " ]" << std::endl;
			poly_t p(x.data(), x.data() + x.size());
			m_ratepoly.swap(p);
			m_ratepoly = m_ratepoly.simplify(0.1, 0.001, 1./50000);
		}
	}
	if (force || m_fuelflowpoly.empty()) {
		m_fuelflowpoly.clear();
		if (ptfuel) {
			ret = true;
			//mf.conservativeResize(ptfuel, std::min(ptfuel, polyorder));
			//vf.conservativeResize(ptfuel);
			mf = mf.topLeftCorner(ptfuel, std::min(ptfuel, polyorder)).eval();
			vf = vf.head(ptfuel).eval();
			Eigen::VectorXd x(least_squares_solution(mf, vf));
			if (false)
				std::cerr << "mf [ " << mr << " ]; vf [ " << vr << " ]" << std::endl;
			poly_t p(x.data(), x.data() + x.size());
			m_fuelflowpoly.swap(p);
			m_fuelflowpoly = m_fuelflowpoly.simplify(0.01, 0.001, 1./50000);
		}
	}
	if (force || m_caspoly.empty()) {
		m_caspoly.clear();
		if (ptcas) {
			ret = true;
			//mc.conservativeResize(ptcas, std::min(ptcas, polyorder));
			//vc.conservativeResize(ptcas);
			mc = mc.topLeftCorner(ptcas, std::min(ptcas, polyorder)).eval();
			vc = vc.head(ptcas).eval();
			Eigen::VectorXd x(least_squares_solution(mc, vc));
			if (false)
				std::cerr << "mc [ " << mc << " ]; vc [ " << vc << " ]" << std::endl;
			poly_t p(x.data(), x.data() + x.size());
			m_caspoly.swap(p);
			m_caspoly = m_caspoly.simplify(0.1, 0.001, 1./50000);
		}
	}
	if (false)
		m_fuelflowpoly.print(m_caspoly.print(m_ratepoly.print(std::cerr << "recalculateratepoly: ratepoly ")
						     << std::endl << "recalculateratepoly: caspoly ") << std::endl
				     << "recalculateratepoly: fuelflowpoly ") << std::endl;
	return ret;
#else
	return false;
#endif
}

#ifdef __WIN32__
__attribute__((force_align_arg_pointer))
#endif
void Aircraft::ClimbDescent::recalculateclimbpoly(double pastart)
{
	// recompute climb profile polynomials
#ifdef HAVE_EIGEN3
	Poly1D<double> altv, distv, fuelv;
	double dist(0), fuel(0);
	AirData<float> ad;
	ad.set_qnh_temp(); // standard atmosphere
	ad.set_pressure_altitude(pastart);
	static const unsigned int tinc = 1;
	if (false)
		m_fuelflowpoly.print(m_caspoly.print(m_ratepoly.print(std::cerr << "recalculateclimbpoly: ratepoly ")
						     << std::endl << "recalculateclimbpoly: caspoly ") << std::endl
				     << "recalculateclimbpoly: fuelflowpoly ") << std::endl;
	{
		bool lastalt(false);
		for (unsigned int i = 0; ; i += tinc) {
			double cas(m_caspoly.eval(ad.get_pressure_altitude()));
			if (std::isnan(cas) || cas < 1)
				break;
			ad.set_cas(cas);
			altv.push_back(ad.get_pressure_altitude());
			distv.push_back(dist);
			fuelv.push_back(fuel);
			if (false)
				std::cerr << "recalculateclimbpoly: t " << i << " PA " << ad.get_pressure_altitude()
					  << " dist " << dist << " fuel " << fuel << " CAS " << ad.get_cas() << " TAS " << ad.get_tas()
					  << " rate " << m_ratepoly.eval(ad.get_pressure_altitude()) << std::endl;
			// limit simulated climb to one hour
			if (lastalt || i >= 3600)
				break;
			fuel += m_fuelflowpoly.eval(ad.get_pressure_altitude()) * (1.0 / 3600.0);
			dist += ad.get_tas() * (1.0 / 3600.0);
			{
				double altr(m_ratepoly.eval(ad.get_pressure_altitude()));
				if (altr <= 0 || std::isnan(altr))
					break;
				double altn(ad.get_pressure_altitude() + altr * (1.0 / 60.0));
				if (std::isnan(altn) || altn < pastart)
					break;
				if (altn > m_ceiling) {
					altn = m_ceiling;
					lastalt = true;
				}
				ad.set_pressure_altitude(altn);
			}
		}
	}
	if (!altv.empty()) {
		m_ceiling = altv.back();
		m_climbtime = altv.size() * tinc;
	}
	if (altv.size() > 10) {
		const bool force_origin(!pastart);
		{
			const unsigned int polyorder(5 + !force_origin);
			Eigen::MatrixXd m(altv.size(), polyorder);
			Eigen::VectorXd v(altv.size());
			for (std::vector<double>::size_type i(0); i < altv.size(); ++i) {
				v(i) = altv[i];
				double t(1);
				if (force_origin)
					t *= i * tinc;
				for (unsigned int j = 0; j < polyorder; ++j, t *= i * tinc)
					m(i, j) = t;
			}
			Eigen::VectorXd x(least_squares_solution(m, v));
			poly_t p(x.data(), x.data() + x.size());
			if (force_origin)
				p.insert(p.begin(), 0);
			m_climbaltpoly.swap(p);
			m_climbaltpoly = m_climbaltpoly.simplify(0.1, 0.001, 1./3600);
		}
		{
			const unsigned int polyorder(5 + !force_origin);
			Eigen::MatrixXd m(distv.size(), polyorder);
			Eigen::VectorXd v(distv.size());
			for (std::vector<double>::size_type i(0); i < distv.size(); ++i) {
				v(i) = distv[i];
				double t(1);
				if (force_origin)
					t *= i * tinc;
				for (unsigned int j = 0; j < polyorder; ++j, t *= i * tinc)
					m(i, j) = t;
			}
			Eigen::VectorXd x(least_squares_solution(m, v));
			poly_t p(x.data(), x.data() + x.size());
			if (force_origin)
				p.insert(p.begin(), 0);
			m_climbdistpoly.swap(p);
			m_climbdistpoly = m_climbdistpoly.simplify(0.01, 0.001, 1./3600);
		}
		{
			const unsigned int polyorder(5 + !force_origin);
			Eigen::MatrixXd m(fuelv.size(), polyorder);
			Eigen::VectorXd v(fuelv.size());
			for (std::vector<double>::size_type i(0); i < fuelv.size(); ++i) {
				v(i) = fuelv[i];
				double t(1);
				if (force_origin)
					t *= i * tinc;
				for (unsigned int j = 0; j < polyorder; ++j, t *= i * tinc)
					m(i, j) = t;
			}
			Eigen::VectorXd x(least_squares_solution(m, v));
			poly_t p(x.data(), x.data() + x.size());
			if (force_origin)
				p.insert(p.begin(), 0);
			m_climbfuelpoly.swap(p);
			m_climbfuelpoly = m_climbfuelpoly.simplify(0.01, 0.001, 1./3600);
		}
		if (false) {
			char filename[] = "/tmp/climbdescentXXXXXX";
			int fd(mkstemp(filename));
			if (fd != -1) {
				close(fd);
				std::ofstream os(filename);
				altv.save_octave(os, "altv");
				distv.save_octave(os, "distv");
				fuelv.save_octave(os, "fuelv");
				m_ratepoly.save_octave(os, "ratepoly");
				m_fuelflowpoly.save_octave(os, "fuelflowpoly");
				m_caspoly.save_octave(os, "caspoly");
				m_climbaltpoly.save_octave(os, "climbaltpoly");
				m_climbdistpoly.save_octave(os, "climbdistpoly");
				m_climbfuelpoly.save_octave(os, "climbfuelpoly");
				std::cerr << "recalculateclimbpoly: data saved to " << filename << std::endl;
			}
		}
	}
	// do not extrapolate climb
	{
		double maxpa(max_point_pa());
		if (!std::isnan(maxpa) && maxpa >= 1000 && maxpa < m_ceiling)
			m_ceiling = maxpa;
	}
	{
		double t(altitude_to_time(m_ceiling));
		if (!std::isnan(t) && t > 0 && t < m_climbtime)
			m_climbtime = t;
	}
	if (false)
		m_climbfuelpoly.print(m_climbdistpoly.print(m_climbaltpoly.print(std::cerr << "recalculateclimbpoly: climbaltpoly ") << std::endl
							    << "recalculateclimbpoly: climbdistpoly ") << std::endl
				      << "recalculateclimbpoly: climbfuelpoly ") << std::endl;
#endif
}

namespace {
	class PointSorterTime {
	public:
		bool operator()(const Aircraft::ClimbDescent::Point& a, const Aircraft::ClimbDescent::Point& b) const {
			if (std::isnan(b.get_time()))
				return false;
			if (std::isnan(a.get_time()))
				return true;
			return a.get_time() < b.get_time();
		}
	};
};

#ifdef __WIN32__
__attribute__((force_align_arg_pointer))
#endif
bool Aircraft::ClimbDescent::recalculatetoclimbpoly(bool force)
{
	// recalculate climb profile polynomials
#ifdef HAVE_EIGEN3
	static const double enforce_points_altdist = 5000;
	if (!force && !m_climbaltpoly.empty() && !m_climbfuelpoly.empty() && !m_climbdistpoly.empty())
		return false;
	if (m_points.empty()) {
		m_climbaltpoly.clear();
		m_climbfuelpoly.clear();
		m_climbdistpoly.clear();
		return false;
	}
	points_t points(m_points);
	std::sort(points.begin(), points.end(), PointSorterTime());
	for (points_t::size_type i(0), n(points.size()); i < n; ) {
		Point& b(points[i]);
		if (std::isnan(b.get_time()) || std::isnan(b.get_pressurealt()) ||
		    std::isnan(b.get_fuel()) || std::isnan(b.get_dist()) ||
		    b.get_time() < 0) {
			points.erase(points.begin() + i);
			n = points.size();
			continue;
		}
		++i;
		static const Point nullpoint(0, 0, 0, 0);
		const Point& a((i < 2) ? nullpoint : points[i-2]);
		if (b.get_pressurealt() < a.get_pressurealt() + enforce_points_altdist)
			continue;
		unsigned int m(std::ceil((b.get_pressurealt() - a.get_pressurealt()) * (1.0 / enforce_points_altdist)));
		if (m < 2)
			continue;
		double t0(a.get_time()), td((b.get_time() - a.get_time()) / m);
		double a0(a.get_pressurealt()), ad((b.get_pressurealt() - a.get_pressurealt()) / m);
		double f0(a.get_fuel()), fd((b.get_fuel() - a.get_fuel()) / m);
		double d0(a.get_dist()), dd((b.get_dist() - a.get_dist()) / m);
		for (unsigned int j = 1; j < m; ++j, ++i)
			points.insert(points.begin() + (i - 1), Point(a0 + ad * j, t0 + td * j, f0 + fd * j, d0 + dd * j));
		n = points.size();
	}
	m_ceiling = 0;
	m_climbtime = 0;
	unsigned int polyorder(5);
	unsigned int ptalt(0);
	unsigned int ptfuel(0);
	unsigned int ptdist(0);
	Eigen::MatrixXd ma(points.size() + 1, polyorder);
	Eigen::MatrixXd mf(points.size() + 1, polyorder);
	Eigen::MatrixXd md(points.size() + 1, polyorder);
	Eigen::VectorXd va(points.size() + 1);
	Eigen::VectorXd vf(points.size() + 1);
	Eigen::VectorXd vd(points.size() + 1);
	{
		typedef std::map<double,double> dmap_t;
		dmap_t pmap, fmap, dmap;
		for (points_t::const_iterator pi(points.begin()), pe(points.end()); pi != pe; ++pi) {
			if (false)
				std::cerr << "climbpoint: time " << pi->get_time() << " pa " << pi->get_pressurealt()
					  << " fuel " << pi->get_fuel() << " dist " << pi->get_dist() << std::endl;
			double tm(pi->get_time());
			if (std::isnan(tm))
				continue;
			tm = std::max(tm, 0.);
			m_climbtime = std::max(m_climbtime, tm);
			double d(pi->get_pressurealt());
			if (!std::isnan(d)) {
				std::pair<dmap_t::iterator,bool> ins(pmap.insert(dmap_t::value_type(tm, d)));
				if (!ins.second) {
					std::cerr << "Duplicate entry for time " << tm << " new PA value " << d
						  << " previous " << ins.first->second << std::endl;
					ins.first->second = (ins.first->second + d) * 0.5;
				}
			}
			d = pi->get_fuel();	
			if (!std::isnan(d)) {
				std::pair<dmap_t::iterator,bool> ins(fmap.insert(dmap_t::value_type(tm, d)));
				if (!ins.second) {
					std::cerr << "Duplicate entry for time " << tm << " new fuel value " << d
						  << " previous " << ins.first->second << std::endl;
					ins.first->second = (ins.first->second + d) * 0.5;
				}
			}
			d = pi->get_dist();
			if (!std::isnan(d)) {
				std::pair<dmap_t::iterator,bool> ins(dmap.insert(dmap_t::value_type(tm, d)));
				if (!ins.second) {
					std::cerr << "Duplicate entry for time " << tm << " new dist value " << d
						  << " previous " << ins.first->second << std::endl;
					ins.first->second = (ins.first->second + d) * 0.5;
				}
			}
		}
		if (pmap.empty() || fabs(pmap.begin()->first) >= 1)
			pmap.insert(dmap_t::value_type(0, 0));
		if (fmap.empty() || fabs(fmap.begin()->first) >= 1)
			fmap.insert(dmap_t::value_type(0, 0));
		if (dmap.empty() || fabs(dmap.begin()->first) >= 1)
			dmap.insert(dmap_t::value_type(0, 0));
		for (dmap_t::const_iterator i(pmap.begin()), e(pmap.end()); i != e; ++i) {
			double tm1(1), tm(i->first);
			m_ceiling = std::max(m_ceiling, i->second);
			for (unsigned int i = 0; i < polyorder; ++i, tm1 *= tm)
				ma(ptalt, i) = tm1;
			va(ptalt) = i->second;
			++ptalt;
		}
		for (dmap_t::const_iterator i(fmap.begin()), e(fmap.end()); i != e; ++i) {
			double tm1(1), tm(i->first);
			for (unsigned int i = 0; i < polyorder; ++i, tm1 *= tm)
				mf(ptfuel, i) = tm1;
			vf(ptfuel) = i->second;
			++ptfuel;
		}
		for (dmap_t::const_iterator i(dmap.begin()), e(dmap.end()); i != e; ++i) {
			double tm1(1), tm(i->first);
			for (unsigned int i = 0; i < polyorder; ++i, tm1 *= tm)
				md(ptdist, i) = tm1;
			vd(ptdist) = i->second;
			++ptdist;
		}
	}
	bool ret(false);
	if (force || m_climbaltpoly.empty()) {
		m_climbaltpoly.clear();
		if (ptalt) {
			ret = true;
			//ma.conservativeResize(ptalt, std::min(ptalt, polyorder));
			//va.conservativeResize(ptalt);
			ma = ma.topLeftCorner(ptalt, std::min(ptalt, polyorder)).eval();
			va = va.head(ptalt).eval();
			Eigen::VectorXd x(least_squares_solution(ma, va));
			if (false)
				std::cerr << "ma [ " << ma << " ]; va [ " << va << " ]" << std::endl;
			poly_t p(x.data(), x.data() + x.size());
			m_climbaltpoly.swap(p);
			m_climbaltpoly = m_climbaltpoly.simplify(0.1, 0.001, 1./3600);
		}
	}
	if (force || m_climbfuelpoly.empty()) {
		m_climbfuelpoly.clear();
		if (ptfuel) {
			ret = true;
			//mf.conservativeResize(ptfuel, std::min(ptfuel, polyorder));
			//vf.conservativeResize(ptfuel);
			mf = mf.topLeftCorner(ptfuel, std::min(ptfuel, polyorder)).eval();
			vf = vf.head(ptfuel).eval();
			Eigen::VectorXd x(least_squares_solution(mf, vf));
			if (false)
				std::cerr << "mf [ " << mf << " ]; vf [ " << vf << " ]" << std::endl;
			poly_t p(x.data(), x.data() + x.size());
			m_climbfuelpoly.swap(p);
			m_climbfuelpoly = m_climbfuelpoly.simplify(0.01, 0.001, 1./3600);
		}
	}
	if (force || m_climbdistpoly.empty()) {
		m_climbdistpoly.clear();
		if (ptdist) {
			ret = true;
			//md.conservativeResize(ptdist, std::min(ptdist, polyorder));
			//vd.conservativeResize(ptdist);
			md = md.topLeftCorner(ptdist, std::min(ptdist, polyorder)).eval();
			vd = vd.head(ptdist).eval();
			Eigen::VectorXd x(least_squares_solution(md, vd));
			if (false)
				std::cerr << "md [ " << md << " ]; vd [ " << vd << " ]" << std::endl;
			poly_t p(x.data(), x.data() + x.size());
			m_climbdistpoly.swap(p);
			m_climbdistpoly = m_climbdistpoly.simplify(0.01, 0.001, 1./3600);
		}
	}
	if (false)
		m_climbfuelpoly.print(m_climbdistpoly.print(m_climbaltpoly.print(std::cerr << "recalculatetoclimbpoly: climbaltpoly ") << std::endl
							    << "recalculatetoclimbpoly: climbdistpoly ") << std::endl
				      << "recalculatetoclimbpoly: climbfuelpoly ") << std::endl;
	return ret;
#else
	return false;
#endif
}

#ifdef __WIN32__
__attribute__((force_align_arg_pointer))
#endif
void Aircraft::ClimbDescent::recalculatetoratepoly(void)
{
	// recompute rate polynomials
#ifdef HAVE_EIGEN3
	m_ratepoly.clear();
	m_fuelflowpoly.clear();
	m_caspoly.clear();
	unsigned int polyorder(4);
	unsigned int pt(0);
	const unsigned int nrpt(m_ceiling * 0.01 + 1);
	Eigen::MatrixXd m(nrpt, polyorder);
	Eigen::VectorXd vr(nrpt);
	Eigen::VectorXd vf(nrpt);
	Eigen::VectorXd vc(nrpt);
	AirData<float> ad;
	ad.set_qnh_temp(); // standard atmosphere
	ad.set_pressure_altitude(0);
	for (; pt < nrpt; ++pt) {
		double pa(pt * 100);
		ad.set_pressure_altitude(pa);
		if (pa > m_ceiling + 50)
			break;
		{
			double pa1(1);
			for (unsigned int i = 0; i < polyorder; ++i, pa1 *= pa)
				m(pt, i) = pa1;
		}
		double t(altitude_to_time(pa));
		vr(pt) = time_to_climbrate(t);
		vf(pt) = time_to_fuelflow(t);
		double tas(time_to_tas(t));
		if (tas > 1) {
			ad.set_tas(tas);
			vc(pt) = ad.get_cas();
		} else {
			vc(pt) = 1;
		}
		if (false)
			std::cerr << "recalculatetoratepoly: pa " << m(pt, 1) << " rate " << vr(pt) << " ff " << vf(pt)
				  << " cas " << vc(pt) << " tas " << tas << std::endl;
	}
	if (pt < 1)
		return;
	//m.conservativeResize(pt, std::min(pt, polyorder));
	m = m.topLeftCorner(pt, std::min(pt, polyorder)).eval();
	//vr.conservativeResize(pt);
	vr = vr.head(pt).eval();
	//vf.conservativeResize(pt);
	vf = vf.head(pt).eval();
	//vc.conservativeResize(pt);
	vc = vc.head(pt).eval();
	{
		Eigen::VectorXd x(least_squares_solution(m, vr));
		if (false)
			std::cerr << "m [ " << m << " ]; vr [ " << vr << " ]" << std::endl;
		poly_t p(x.data(), x.data() + x.size());
		m_ratepoly.swap(p);
		m_ratepoly = m_ratepoly.simplify(0.1, 0.001, 1./50000);
	}
	{
		Eigen::VectorXd x(least_squares_solution(m, vf));
		if (false)
			std::cerr << "m [ " << m << " ]; vf [ " << vr << " ]" << std::endl;
		poly_t p(x.data(), x.data() + x.size());
		m_fuelflowpoly.swap(p);
		m_fuelflowpoly = m_fuelflowpoly.simplify(0.01, 0.001, 1./50000);
	}
	{
		Eigen::VectorXd x(least_squares_solution(m, vc));
		if (false)
			std::cerr << "m [ " << m << " ]; vc [ " << vc << " ]" << std::endl;
		poly_t p(x.data(), x.data() + x.size());
		m_caspoly.swap(p);
		m_caspoly = m_caspoly.simplify(0.1, 0.001, 1./50000);
	}
	if (false)
		m_fuelflowpoly.print(m_caspoly.print(m_ratepoly.print(std::cerr << "recalculatetoratepoly: ratepoly ")
						     << std::endl << "recalculatetoratepoly: caspoly ") << std::endl
				     << "recalculatetoratepoly: fuelflowpoly ") << std::endl;
#endif
}

bool Aircraft::ClimbDescent::recalculatepoly(bool force)
{
	switch (get_mode()) {
	case mode_timetoaltitude:
		if (!recalculatetoclimbpoly(force))
			return false;
		recalculatetoratepoly();
		break;

	default:
		if (!recalculateratepoly(force))
			return false;	
		recalculateclimbpoly(0);
		break;
	}
	return true;
}

void Aircraft::ClimbDescent::recalculateformass(double newmass)
{
	if (fabs(get_mass() - newmass) < 0.01)
		return;
	m_ratepoly *= get_mass() / newmass;
	m_ratepoly = m_ratepoly.simplify(0.1, 0.001, 1./50000);
	double oldceil(m_ceiling), oldmass(m_mass);
	m_mass = newmass;
	recalculateclimbpoly(time_to_altitude(0));
	if (false)
		std::cerr << "recalculateformass: mass " << oldmass << " -> " << m_mass
			  << " ceil " << oldceil << " -> " << m_ceiling << std::endl;
	clear_points();
	//recalc_points();
	if (false)
		print(std::cerr << "ClimbDescent::recalculateformass:" << std::endl, "  ");
}

#ifdef __WIN32__
__attribute__((force_align_arg_pointer))
#endif
void Aircraft::ClimbDescent::recalculateforatmo(double isaoffs, double qnh)
{
	// recompute climb alt & time polynomials
	if (fabs(qnh - IcaoAtmosphere<double>::std_sealevel_pressure) < 0.1 &&
	    fabs(isaoffs - m_isaoffset) < 0.1)
		return;
#ifdef HAVE_EIGEN3
	static const unsigned int timestep = 10;
	Poly1D<double> altv;
	{
		IcaoAtmosphere<double> atmoo(IcaoAtmosphere<double>::std_sealevel_pressure, IcaoAtmosphere<double>::std_sealevel_temperature + m_isaoffset);
		IcaoAtmosphere<double> atmon(qnh, IcaoAtmosphere<double>::std_sealevel_temperature + isaoffs);
		for (unsigned int i = 0; i <= m_climbtime; i += timestep) {
			double alt(time_to_altitude(i));
			if (alt > m_ceiling)
				break;
			double density(atmoo.altitude_to_density(alt * ::Point::ft_to_m_dbl));
			double altn(atmon.density_to_altitude(density) * ::Point::m_to_ft_dbl);
			altv.push_back(altn);
			if (false)
				std::cerr << "recalculateforatmo: time " << i << " alt old " << alt << " new " << altn << " density " << density << std::endl;
		}
		double newceil(atmon.density_to_altitude(atmoo.altitude_to_density(m_ceiling)));
		if (false)
			std::cerr << "recalculateforatmo: isaoffs " << (atmoo.get_temp() - IcaoAtmosphere<double>::std_sealevel_temperature)
				  << " -> " << (atmon.get_temp() - IcaoAtmosphere<double>::std_sealevel_temperature) << " qnh "
				  << atmoo.get_qnh() << " -> " << atmon.get_qnh() << " ceil " << m_ceiling << " -> " << newceil << std::endl;
		m_ceiling = newceil;
	}
	if (altv.size() < 10)
		return;
	const bool force_origin(false);
	{
		const unsigned int polyorder(5);
		Eigen::MatrixXd m(altv.size(), polyorder);
		Eigen::VectorXd v(altv.size());
		for (std::vector<double>::size_type i(0); i < altv.size(); ++i) {
			v(i) = altv[i];
			double t(1);
			if (force_origin)
				t *= i * timestep;
			for (unsigned int j = 0; j < polyorder; ++j, t *= i * timestep)
				m(i, j) = t;
		}
		Eigen::VectorXd x(least_squares_solution(m, v));
		poly_t p(x.data(), x.data() + x.size());
		if (force_origin)
			p.insert(p.begin(), 0);
		m_climbaltpoly.swap(p);
		if (false)
			m_climbaltpoly.print(std::cerr << "recalculateforatmo: climbaltpoly (1): ") << std::endl;
		m_climbaltpoly = m_climbaltpoly.simplify(0.1, 0.001, 1./3600);
		if (false)
			m_climbaltpoly.print(std::cerr << "recalculateforatmo: climbaltpoly: ") << std::endl;
	}
	m_isaoffset = isaoffs;
#endif
	clear_points();
	//recalc_points();
	if (false)
		print(std::cerr << "ClimbDescent::recalculateforatmo:" << std::endl, "  ");
}

Aircraft::ClimbDescent Aircraft::ClimbDescent::interpolate(double t, const ClimbDescent& cd) const
{
	ClimbDescent r(*this);
	if (std::isnan(t))
		return r;
	t = std::max(std::min(t, 1.), 0.);
	r.m_ratepoly *= t;
	r.m_fuelflowpoly *= t;
	r.m_caspoly *= t;
	r.m_climbaltpoly *= t;
	r.m_climbdistpoly *= t;
	r.m_climbfuelpoly *= t;
	r.m_mass *= t;
	r.m_isaoffset *= t;
	r.m_ceiling *= t;
	r.m_climbtime *= t;
	t = 1 - t;
	r.m_ratepoly += cd.m_ratepoly * t;
	r.m_fuelflowpoly += cd.m_fuelflowpoly * t;
	r.m_caspoly += cd.m_caspoly * t;
	r.m_climbaltpoly += cd.m_climbaltpoly * t;
	r.m_climbdistpoly += cd.m_climbdistpoly * t;
	r.m_climbfuelpoly += cd.m_climbfuelpoly * t;
	r.m_mass += cd.m_mass * t;
	r.m_isaoffset += cd.m_isaoffset * t;
	r.m_ceiling += cd.m_ceiling * t;
	r.m_climbtime += cd.m_climbtime * t;
	r.clear_points();
	//r.recalc_points();
	return r;
}

void Aircraft::ClimbDescent::load_xml(const xmlpp::Element *el, double altfactor, double ratefactor, double fuelflowfactor, double casfactor,
				      double timefactor, double fuelfactor, double distfactor)
{
	if (!el)
		return;
	m_name.clear();
	m_mass = std::numeric_limits<double>::quiet_NaN();
	m_isaoffset = 0;
	m_ceiling = 10000;
	m_climbtime = 3600;
	m_mode = mode_invalid;
	m_ratepoly.clear();
	m_fuelflowpoly.clear();
	m_caspoly.clear();
	m_climbaltpoly.clear();
	m_climbdistpoly.clear();
	m_climbfuelpoly.clear();
	m_points.clear();
	xmlpp::Attribute *attr;
	if ((attr = el->get_attribute("name")))
		m_name = attr->get_value();
	if ((attr = el->get_attribute("mass")))
		m_mass = Glib::Ascii::strtod(attr->get_value());
	if ((attr = el->get_attribute("isaoffset")))
		m_isaoffset = Glib::Ascii::strtod(attr->get_value());
	if ((attr = el->get_attribute("ceiling")))
		m_ceiling = Glib::Ascii::strtod(attr->get_value());
	if ((attr = el->get_attribute("climbtime")))
		m_climbtime = Glib::Ascii::strtod(attr->get_value());
	if ((attr = el->get_attribute("mode"))) {
		if (attr->get_value() == "rateofdescent") {
			m_mode = mode_rateofclimb;
		} else if (attr->get_value() == "timefromaltitude") {
			m_mode = mode_timetoaltitude;
		} else {
			for (mode_t m(mode_rateofclimb); m < mode_invalid; m = (mode_t)(m + 1)) {
				if (to_str(m) != attr->get_value())
					continue;
				m_mode = m;
				break;
			}
		}
	}
	{
		xmlpp::Node::NodeList nl(el->get_children("point"));
		for (xmlpp::Node::NodeList::const_iterator ni(nl.begin()), ne(nl.end()); ni != ne; ++ni) {
			m_points.push_back(Point());
			m_points.back().load_xml(static_cast<xmlpp::Element *>(*ni), m_mode, altfactor, ratefactor, fuelflowfactor, casfactor,
						 timefactor, fuelfactor, distfactor);
		}
	}
	{
		xmlpp::Node::NodeList nl(el->get_children("ratepoly"));
		for (xmlpp::Node::NodeList::const_iterator ni(nl.begin()), ne(nl.end()); ni != ne; ++ni)
			m_ratepoly.load_xml(static_cast<xmlpp::Element *>(*ni));
 	}
	{
		xmlpp::Node::NodeList nl(el->get_children("fuelflowpoly"));
		for (xmlpp::Node::NodeList::const_iterator ni(nl.begin()), ne(nl.end()); ni != ne; ++ni)
			m_fuelflowpoly.load_xml(static_cast<xmlpp::Element *>(*ni));
	}
	{
		xmlpp::Node::NodeList nl(el->get_children("caspoly"));
		for (xmlpp::Node::NodeList::const_iterator ni(nl.begin()), ne(nl.end()); ni != ne; ++ni)
			m_caspoly.load_xml(static_cast<xmlpp::Element *>(*ni));
	}
	{
		xmlpp::Node::NodeList nl(el->get_children("climbaltpoly"));
		for (xmlpp::Node::NodeList::const_iterator ni(nl.begin()), ne(nl.end()); ni != ne; ++ni)
			m_climbaltpoly.load_xml(static_cast<xmlpp::Element *>(*ni));
	}
	{
		xmlpp::Node::NodeList nl(el->get_children("climbdistpoly"));
		for (xmlpp::Node::NodeList::const_iterator ni(nl.begin()), ne(nl.end()); ni != ne; ++ni)
			m_climbdistpoly.load_xml(static_cast<xmlpp::Element *>(*ni));
	}
	{
		xmlpp::Node::NodeList nl(el->get_children("climbfuelpoly"));
		for (xmlpp::Node::NodeList::const_iterator ni(nl.begin()), ne(nl.end()); ni != ne; ++ni)
			m_climbfuelpoly.load_xml(static_cast<xmlpp::Element *>(*ni));
	}
}

void Aircraft::ClimbDescent::save_xml(xmlpp::Element *el, double altfactor, double ratefactor, double fuelflowfactor, double casfactor,
				      double timefactor, double fuelfactor, double distfactor) const
{
	if (!el)
		return;
	if (m_name.empty())
		el->remove_attribute("name");
        else
		el->set_attribute("name", m_name);
	if (std::isnan(m_mass)) {
		el->remove_attribute("mass");
	} else {
		std::ostringstream oss;
		oss << m_mass;
		el->set_attribute("mass", oss.str());
	}
	if (m_isaoffset == 0) {
		el->remove_attribute("isaoffset");
	} else {
		std::ostringstream oss;
		oss << m_isaoffset;
		el->set_attribute("isaoffset", oss.str());
	}
	if (m_ceiling == 1) {
		el->remove_attribute("ceiling");
	} else {
		std::ostringstream oss;
		oss << m_ceiling;
		el->set_attribute("ceiling", oss.str());
	}
	if (m_climbtime == 1) {
		el->remove_attribute("climbtime");
	} else {
		std::ostringstream oss;
		oss << m_climbtime;
		el->set_attribute("climbtime", oss.str());
	}
	if (m_mode == mode_invalid)
		el->remove_attribute("mode");
	else
		el->set_attribute("mode", to_str(m_mode));
	for (points_t::const_iterator di(m_points.begin()), de(m_points.end()); di != de; ++di)
		di->save_xml(el->add_child("point"), m_mode, altfactor, ratefactor, fuelflowfactor, casfactor,
			     timefactor, fuelfactor, distfactor);
	{
		xmlpp::Node::NodeList nl(el->get_children("ratepoly"));
		for (xmlpp::Node::NodeList::const_iterator ni(nl.begin()), ne(nl.end()); ni != ne; ++ni)
			el->remove_child(*ni);
	}
	if (!m_ratepoly.empty())
		m_ratepoly.save_xml(el->add_child("ratepoly"));
	{
		xmlpp::Node::NodeList nl(el->get_children("fuelflowpoly"));
		for (xmlpp::Node::NodeList::const_iterator ni(nl.begin()), ne(nl.end()); ni != ne; ++ni)
			el->remove_child(*ni);
	}
	if (!m_fuelflowpoly.empty())
		m_fuelflowpoly.save_xml(el->add_child("fuelflowpoly"));
	{
		xmlpp::Node::NodeList nl(el->get_children("caspoly"));
		for (xmlpp::Node::NodeList::const_iterator ni(nl.begin()), ne(nl.end()); ni != ne; ++ni)
			el->remove_child(*ni);
	}
	if (!m_caspoly.empty())
		m_caspoly.save_xml(el->add_child("caspoly"));
	{
		xmlpp::Node::NodeList nl(el->get_children("climbtimepoly"));
		for (xmlpp::Node::NodeList::const_iterator ni(nl.begin()), ne(nl.end()); ni != ne; ++ni)
			el->remove_child(*ni);
	}
	{
		xmlpp::Node::NodeList nl(el->get_children("climbaltpoly"));
		for (xmlpp::Node::NodeList::const_iterator ni(nl.begin()), ne(nl.end()); ni != ne; ++ni)
			el->remove_child(*ni);
	}
	if (!m_climbaltpoly.empty())
		m_climbaltpoly.save_xml(el->add_child("climbaltpoly"));
 	{
		xmlpp::Node::NodeList nl(el->get_children("climbdistpoly"));
		for (xmlpp::Node::NodeList::const_iterator ni(nl.begin()), ne(nl.end()); ni != ne; ++ni)
			el->remove_child(*ni);
	}
	if (!m_climbdistpoly.empty())
		m_climbdistpoly.save_xml(el->add_child("climbdistpoly"));
	{
		xmlpp::Node::NodeList nl(el->get_children("climbfuelpoly"));
		for (xmlpp::Node::NodeList::const_iterator ni(nl.begin()), ne(nl.end()); ni != ne; ++ni)
			el->remove_child(*ni);
	}
	if (!m_climbfuelpoly.empty())
		m_climbfuelpoly.save_xml(el->add_child("climbfuelpoly"));
}

void Aircraft::ClimbDescent::add_point(const Point& pt)
{
	m_points.push_back(pt);
}

void Aircraft::ClimbDescent::clear_points(void)
{
	m_points.clear();
}

void Aircraft::ClimbDescent::recalc_points(mode_t m)
{
	clear_points();
	switch (m) {
	case mode_timetoaltitude:
		m_mode = mode_timetoaltitude;
		if (std::isnan(get_ceiling()) || get_ceiling() <= 0 || get_ceiling() >= 100000)
			break;
		{
			for (double alt = 0;; alt += 500) {
				bool last(alt >= get_ceiling());
				if (last)
					alt = get_ceiling();
				double t(altitude_to_time(alt));
				if (t >= 0 && t <= get_climbtime())
					m_points.push_back(Point(alt, t, time_to_fuel(t), time_to_distance(t)));
				if (last)
					break;
			}
		}
		break;

	default:
		m_mode = mode_rateofclimb;
		if (std::isnan(get_ceiling()) || get_ceiling() <= 0 || get_ceiling() >= 100000)
			break;
		{
			for (double alt = 0;; alt += 500) {
				bool last(alt >= get_ceiling());
				if (last)
					alt = get_ceiling();
				double rate, fuelflow, cas;
				calculate(rate, fuelflow, cas, alt);
				m_points.push_back(Point(alt, rate, fuelflow, cas));
				if (last)
					break;
			}
		}
		break;
	}
}

void Aircraft::ClimbDescent::limit_ceiling(double lim)
{
	if (std::isnan(lim))
		return;
	lim = std::max(0., lim);
	m_ceiling = std::min(m_ceiling, lim);
}

void Aircraft::ClimbDescent::set_ceiling(double ceil)
{
	if (std::isnan(ceil) || ceil < 0)
		return;
	m_ceiling = ceil;
}

double Aircraft::ClimbDescent::max_point_pa(void) const
{
	double r(std::numeric_limits<double>::min());
	for (points_t::const_iterator pi(m_points.begin()), pe(m_points.end()); pi != pe; ++pi) {
		double pa(pi->get_pressurealt());
		if (std::isnan(pa))
			continue;
		r = std::max(r, pa);
	}
	return r;
}

void Aircraft::ClimbDescent::set_point_vy(double vy)
{
	if (std::isnan(vy) || vy <= 0 || m_mode != mode_rateofclimb)
		return;
	for (points_t::iterator pi(m_points.begin()), pe(m_points.end()); pi != pe; ++pi) {
		if (!std::isnan(pi->get_cas()) && pi->get_cas() > 0)
			continue;
		pi->set_cas(vy);
	}
}

void Aircraft::ClimbDescent::set_mass(double mass)
{
	if (std::isnan(mass) || mass <= 0)
		return;
	m_mass = mass;
}

void Aircraft::ClimbDescent::set_descent_rate(double rate)
{
	m_ratepoly.clear();
	m_fuelflowpoly.clear();
	m_caspoly.clear();
	m_climbaltpoly.clear();
	m_climbdistpoly.clear();
	m_climbfuelpoly.clear();
	m_points.clear();
	m_name.clear();
	m_mass = std::numeric_limits<double>::quiet_NaN();
	m_isaoffset = 0;
	m_ceiling = 18000;
	m_climbtime = 3600;
	m_mode = mode_rateofclimb;
	m_ratepoly.push_back(rate);
	m_fuelflowpoly.push_back(0);
	m_caspoly.push_back(0);
}

Aircraft::CheckErrors Aircraft::ClimbDescent::check(double minmass, double maxmass, bool descent) const
{
	CheckErrors r;
	if (get_mass() < minmass * 0.9)
		r.add(*this, descent, CheckError::severity_warning) << "mass less than empty mass - 10%";
	if (get_mass() > maxmass)
		r.add(*this, descent, CheckError::severity_warning) << "mass greater than maximum takeoff mass";
	if (std::isnan(get_ceiling()) || get_ceiling() <= 0) {
		r.add(*this, descent, CheckError::severity_error) << "invalid ceiling";
		return r;
	}
	if (std::isnan(get_climbtime()) || get_climbtime() <= 0) {
		r.add(*this, descent, CheckError::severity_error) << "invalid climb time";
		return r;
	}
	if (get_ceiling() < 7000)
		r.add(*this, descent, CheckError::severity_error) << "unrealistically low ceiling";
	if (get_climbtime() < 90)
		r.add(*this, descent, CheckError::severity_error) << "unrealistically low climb time";
	int tratemin(std::numeric_limits<int>::max()), tratemax(std::numeric_limits<int>::min());
	int taltmin(tratemin), taltmax(tratemax), tdistmin(tratemin), tdistmax(tratemax), tfuelmin(tratemin), tfuelmax(tratemax);
	int ttamin(tratemin), ttamax(tratemax), ttdmin(tratemin), ttdmax(tratemax);
	double tadiff(0), tddiff(0), aadiff(0), addiff(0);
	for (int t = 0; t <= get_climbtime(); t += 10) {
		if (get_ratepoly().eval(t) < 0) {
			tratemin = std::min(tratemin, t);
			tratemax = std::max(tratemax, t);
		}
		if (get_climbaltpoly().evalderiv(t) < 0) {
			taltmin = std::min(taltmin, t);
			taltmax = std::max(taltmax, t);
		}
		if (get_climbdistpoly().evalderiv(t) < 0) {
			tdistmin = std::min(tdistmin, t);
			tdistmax = std::max(tdistmax, t);
		}
		if (get_climbfuelpoly().evalderiv(t) < 0) {
			tfuelmin = std::min(tfuelmin, t);
			tfuelmax = std::max(tfuelmax, t);
		}
                double t0(altitude_to_time(time_to_altitude(t)) - t);
		if (fabs(t0) > 30) {
			double a0(time_to_altitude(t0 + t) - time_to_altitude(t));
			if (fabs(a0) > 100) {
				ttamin = std::min(ttamin, t);
				ttamax = std::max(ttamax, t);
				if (fabs(t0) > fabs(tadiff))
					tadiff = t0;
				if (fabs(a0) > fabs(aadiff))
					aadiff = a0;
			}
		}
		t0 = distance_to_time(time_to_distance(t)) - t;
		if (fabs(t0) > 30) {
			double a0(time_to_altitude(t0 + t) - time_to_altitude(t));
			if (fabs(a0) > 100) {
				ttdmin = std::min(ttdmin, t);
				ttdmax = std::max(ttdmax, t);
				if (fabs(t0) > fabs(tddiff))
					tddiff = t0;
				if (fabs(a0) > fabs(addiff))
					addiff = a0;
			}
		}
	}
	static const char * const climbdescentstr[2] = { "climb", "descent" };
	if (tratemin <= tratemax)
		r.add(*this, descent, CheckError::severity_error, tratemin, tratemax) << "rate of "
										      << climbdescentstr[descent] << " less than zero";
	if (taltmin <= taltmax)
		r.add(*this, descent, CheckError::severity_error, taltmin, taltmax) << "altitude derivative (rate of "
										    << climbdescentstr[descent] << ") less than zero";
	if (tdistmin <= tdistmax)
		r.add(*this, descent, CheckError::severity_error, tdistmin, tdistmax) << "distance derivative less than zero (distance goes backwards)";
	if (tfuelmin <= tfuelmax)
		r.add(*this, descent, CheckError::severity_error, tfuelmin, tfuelmax) << "fuel derivative less than zero (negative fuel flow)";
	if (ttamin <= ttamax)
		r.add(*this, descent, CheckError::severity_error, ttamin, ttamax) << "excessive time/altitude forward/backward error: time error "
										  << tadiff << " altitude error " << aadiff;
	if (ttdmin <= ttdmax)
		r.add(*this, descent, CheckError::severity_error, ttdmin, ttdmax) << "excessive time/distance forward/backward error: time error "
										  << tddiff << " altitude error " << addiff;
	if (false && ttamin <= ttamax)
		m_climbaltpoly.print(std::cerr << "ClimbDescent name " << get_name() << " mass " << get_mass()
				     << " isaoffs " << get_isaoffset() << std::endl << "  climbaltpoly ") << std::endl;
	return r;
}

std::ostream& Aircraft::ClimbDescent::print(std::ostream& os, const std::string& indent) const
{
	os << indent << "name = \"" << get_name() << "\"" << std::endl
	   << indent << "mode = \"" << to_str(get_mode()) << "\"" << std::endl
	   << indent << "mass = " << get_mass() << std::endl
	   << indent << "isaoffset = " << get_isaoffset() << std::endl
	   << indent << "ceiling = " << get_ceiling() << std::endl
	   << indent << "climbtime = " << get_climbtime() << std::endl;
	get_ratepoly().print(os << indent << "ratepoly = [") << "]" << std::endl;
	get_fuelflowpoly().print(os << indent << "fuelflowpoly = [") << "]" << std::endl;
	get_caspoly().print(os << indent << "caspoly = [") << "]" << std::endl;
	get_climbaltpoly().print(os << indent << "climbaltpoly = [") << "]" << std::endl;
	get_climbdistpoly().print(os << indent << "climbdistpoly = [") << "]" << std::endl;
	get_climbfuelpoly().print(os << indent << "climbfuelpoly = [") << "]" << std::endl;
	for (points_t::const_iterator i(m_points.begin()), e(m_points.end()); i != e; ++i)
		i->print(os << indent << "  ", get_mode()) << std::endl;
	return os;
}

Aircraft::Climb::Climb(double altfactor, double ratefactor, double fuelflowfactor, double casfactor,
		       double timefactor, double fuelfactor, double distfactor)
	: m_altfactor(altfactor), m_ratefactor(ratefactor), m_fuelflowfactor(fuelflowfactor), m_casfactor(casfactor),
	  m_timefactor(timefactor), m_fuelfactor(fuelfactor), m_distfactor(distfactor)
{
	add(ClimbDescent("", 2600));
}

Aircraft::ClimbDescent Aircraft::Climb::calculate(const std::string& name, double mass, double isaoffs, double qnh) const
{
	curves_t::const_iterator ci(m_curves.find(name));
	if (ci == m_curves.end()) {
		ci = m_curves.begin();
		if (ci == m_curves.end())
			return ClimbDescent("", 2600);
	}
	return calculate(ci->second, mass, isaoffs, qnh);
}

Aircraft::ClimbDescent Aircraft::Climb::calculate(std::string& name, const std::string& dfltname, double mass, double isaoffs, double qnh) const
{
	curves_t::const_iterator ci(m_curves.find(name));
	if (ci == m_curves.end()) {
		ci = m_curves.find(dfltname);
		if (ci == m_curves.end()) {
			ci = m_curves.begin();
			if (ci == m_curves.end())
				return ClimbDescent("", 2600);
		} else {
			name = dfltname;
		}
	}
	return calculate(ci->second, mass, isaoffs, qnh);
}

Aircraft::ClimbDescent Aircraft::Climb::calculate(const massmap_t& ci, double mass, double isaoffs, double qnh) const
{
	massmap_t::const_iterator miu(ci.lower_bound(mass));
	if (miu == ci.end()) {
		if (miu == ci.begin())
			return ClimbDescent("", 2600);
		--miu;
		if (miu != ci.begin())
			return calculate(miu->second, isaoffs, qnh);
		ClimbDescent r(calculate(miu->second, isaoffs, qnh));
		r.recalculateformass(mass);
		return r;
	}
	massmap_t::const_iterator mil(miu);
	if (miu == ci.begin()) {
		++miu;
		if (miu == ci.end()) {
			ClimbDescent r(calculate(mil->second, isaoffs, qnh));
			r.recalculateformass(mass);
			return r;
		}
	} else {
		--mil;
	}
	double t((mass - mil->first) / (miu->first - mil->first));
	if (t <= 0)
		return calculate(mil->second, isaoffs, qnh);
	if (t >= 1)
		return calculate(miu->second, isaoffs, qnh);
	return calculate(miu->second, isaoffs, qnh).interpolate(t, calculate(mil->second, isaoffs, qnh));
}

Aircraft::ClimbDescent Aircraft::Climb::calculate(const isamap_t& mi, double isaoffs, double qnh) const
{
	isamap_t::const_iterator iiu(mi.lower_bound(isaoffs));
	if (iiu == mi.end()) {
		if (iiu == mi.begin())
			return ClimbDescent("", 2600);
		--iiu;
		if (iiu != mi.begin())
			return iiu->second;
		ClimbDescent r(iiu->second);
		r.recalculateforatmo(isaoffs, qnh);
		return r;
	}
	isamap_t::const_iterator iil(iiu);
	if (iiu == mi.begin()) {
		++iiu;
		if (iiu == mi.end()) {
			ClimbDescent r(iil->second);
			r.recalculateforatmo(isaoffs, qnh);
			return r;
		}
	} else {
		--iil;
	}
	double t((isaoffs - iil->first) / (iiu->first - iil->first));
	if (t <= 0)
		return iil->second;
	if (t >= 1)
		return iiu->second;
	return iiu->second.interpolate(t, iil->second);
}

unsigned int Aircraft::Climb::count_curves(void) const
{
	unsigned int ret(0);
	for (curves_t::const_iterator ci(m_curves.begin()), ce(m_curves.end()); ci != ce; ++ci)
		for (massmap_t::const_iterator mi(ci->second.begin()), me(ci->second.end()); mi != me; ++mi)
			ret += mi->second.size();
	return ret;
}

std::set<std::string> Aircraft::Climb::get_curves(void) const
{
	std::set<std::string> r;
	for (curves_t::const_iterator i(m_curves.begin()), e(m_curves.end()); i != e; ++i)
		r.insert(i->first);
	return r;
}

double Aircraft::Climb::get_ceiling(void) const
{
	double c(0);
	for (curves_t::const_iterator ci(m_curves.begin()), ce(m_curves.end()); ci != ce; ++ci)
		for (massmap_t::const_iterator mi(ci->second.begin()), me(ci->second.end()); mi != me; ++mi)
			for (isamap_t::const_iterator ii(mi->second.begin()), ie(mi->second.end()); ii != ie; ++ii)
				c = std::max(c, ii->second.get_ceiling());
	return c;
}

void Aircraft::Climb::load_xml(const xmlpp::Element *el)
{
	if (!el)
		return;
	m_curves.clear();
	m_remark.clear();
	m_altfactor = 1;
	m_ratefactor = 1;
	m_fuelflowfactor = 1;
	m_casfactor = 1;
	m_timefactor = 1;
	m_fuelfactor = 1;
	m_distfactor = 1;
	xmlpp::Attribute *attr;
	if ((attr = el->get_attribute("altfactor")))
		m_altfactor = Glib::Ascii::strtod(attr->get_value());
	if ((attr = el->get_attribute("ratefactor")))
		m_ratefactor = Glib::Ascii::strtod(attr->get_value());
	if ((attr = el->get_attribute("fuelfactor")))
		m_fuelflowfactor = m_fuelfactor = Glib::Ascii::strtod(attr->get_value());
	if ((attr = el->get_attribute("fuelflowfactor")))
		m_fuelflowfactor = Glib::Ascii::strtod(attr->get_value());
	if ((attr = el->get_attribute("casfactor")))
		m_casfactor = Glib::Ascii::strtod(attr->get_value());
	if ((attr = el->get_attribute("timefactor")))
		m_timefactor = Glib::Ascii::strtod(attr->get_value());
	if ((attr = el->get_attribute("distfactor")))
		m_distfactor = Glib::Ascii::strtod(attr->get_value());
	if ((attr = el->get_attribute("remark")))
		m_remark = attr->get_value();
	{
		xmlpp::Node::NodeList nl(el->get_children("curve"));
		for (xmlpp::Node::NodeList::const_iterator ni(nl.begin()), ne(nl.end()); ni != ne; ++ni) {
			ClimbDescent cd;
			cd.load_xml(static_cast<xmlpp::Element *>(*ni), m_altfactor, m_ratefactor, m_fuelflowfactor, m_casfactor,
				    m_timefactor, m_fuelfactor, m_distfactor);
			add(cd);
		}
		if (nl.empty()) {
			ClimbDescent cd;
			cd.load_xml(el, m_altfactor, m_ratefactor, m_fuelflowfactor, m_casfactor,
				    m_timefactor, m_fuelfactor, m_distfactor);
			if (cd.has_points()) {
				if (cd.get_name().empty())
					cd.set_name("Normal");
				add(cd);
			}
		}
	}
}

void Aircraft::Climb::save_xml(xmlpp::Element *el) const
{
	if (m_altfactor == 1) {
		el->remove_attribute("altfactor");
	} else {
		std::ostringstream oss;
		oss << m_altfactor;
		el->set_attribute("altfactor", oss.str());
	}
	if (m_ratefactor == 1) {
		el->remove_attribute("ratefactor");
	} else {
		std::ostringstream oss;
		oss << m_ratefactor;
		el->set_attribute("ratefactor", oss.str());
	}
	if (m_fuelflowfactor == 1) {
		el->remove_attribute("fuelflowfactor");
	} else {
		std::ostringstream oss;
		oss << m_fuelflowfactor;
		el->set_attribute("fuelflowfactor", oss.str());
	}
	if (m_casfactor == 1) {
		el->remove_attribute("casfactor");
	} else {
		std::ostringstream oss;
		oss << m_casfactor;
		el->set_attribute("casfactor", oss.str());
	}
	if (m_timefactor == 1) {
		el->remove_attribute("timefactor");
	} else {
		std::ostringstream oss;
		oss << m_timefactor;
		el->set_attribute("timefactor", oss.str());
	}
	if (m_fuelfactor == 1) {
		el->remove_attribute("fuelfactor");
	} else {
		std::ostringstream oss;
		oss << m_fuelfactor;
		el->set_attribute("fuelfactor", oss.str());
	}
	if (m_distfactor == 1) {
		el->remove_attribute("distfactor");
	} else {
		std::ostringstream oss;
		oss << m_distfactor;
		el->set_attribute("distfactor", oss.str());
	}
	if (m_remark.empty())
		el->remove_attribute("remark");
	else
		el->set_attribute("remark", m_remark);
	for (curves_t::const_iterator ci(m_curves.begin()), ce(m_curves.end()); ci != ce; ++ci)
		for (massmap_t::const_iterator mi(ci->second.begin()), me(ci->second.end()); mi != me; ++mi)
			for (isamap_t::const_iterator ii(mi->second.begin()), ie(mi->second.end()); ii != ie; ++ii)
				ii->second.save_xml(el->add_child("curve"), m_altfactor, m_ratefactor, m_fuelflowfactor, m_casfactor,
						    m_timefactor, m_fuelfactor, m_distfactor);
}

void Aircraft::Climb::clear(void)
{
	m_curves.clear();
}

void Aircraft::Climb::add(const ClimbDescent& cd)
{
	double m(cd.get_mass());
	if (std::isnan(m) || m <= 0)
		m = 0;
	m_curves[cd.get_name()][m][cd.get_isaoffset()] = cd;
}

bool Aircraft::Climb::recalculatepoly(bool force)
{
	bool work(false);
	for (curves_t::iterator ci(m_curves.begin()), ce(m_curves.end()); ci != ce; ++ci)
		for (massmap_t::iterator mi(ci->second.begin()), me(ci->second.end()); mi != me; ++mi)
			for (isamap_t::iterator ii(mi->second.begin()), ie(mi->second.end()); ii != ie; ++ii)
				work = const_cast<ClimbDescent&>(ii->second).recalculatepoly(force) || work;
	return work;
}

void Aircraft::Climb::set_point_vy(double vy)
{
	for (curves_t::iterator ci(m_curves.begin()), ce(m_curves.end()); ci != ce; ++ci)
		for (massmap_t::iterator mi(ci->second.begin()), me(ci->second.end()); mi != me; ++mi)
			for (isamap_t::iterator ii(mi->second.begin()), ie(mi->second.end()); ii != ie; ++ii)
				const_cast<ClimbDescent&>(ii->second).set_point_vy(vy);
}

void Aircraft::Climb::set_mass(double mass)
{
	if (std::isnan(mass) || mass <= 0)
		return;
	for (curves_t::iterator ci(m_curves.begin()), ce(m_curves.end()); ci != ce; ++ci) {
		massmap_t::iterator mi(ci->second.find(0));
		if (mi == ci->second.end())
			continue;
		for (isamap_t::const_iterator ii(mi->second.begin()), ie(mi->second.end()); ii != ie; ++ii) {
			ClimbDescent cd(ii->second);
			cd.set_mass(mass);
			add(cd);
		}
		ci->second.erase(mi);
	}
}

const Aircraft::ClimbDescent *Aircraft::Climb::find_single_curve(void) const
{
	if (m_curves.size() != 1)
		return 0;
	const massmap_t& m(m_curves.begin()->second);
	if (m.size() != 1)
		return 0;
	const isamap_t& i(m.begin()->second);
	if (i.size() != 1)
		return 0;
	return &i.begin()->second;
}

Aircraft::CheckErrors Aircraft::Climb::check(double minmass, double maxmass, bool descent) const
{
	CheckErrors r;
	typedef std::map<double,double> cmassmap_t;
	typedef std::map<double,cmassmap_t> cisamap_t;
	cisamap_t ceilings;
	for (curves_t::const_iterator ci(m_curves.begin()), ce(m_curves.end()); ci != ce; ++ci) {
		for (massmap_t::const_iterator mi(ci->second.begin()), me(ci->second.end()); mi != me; ++mi)
			for (isamap_t::const_iterator ii(mi->second.begin()), ie(mi->second.end()); ii != ie; ++ii) {
				r.add(ii->second.check(minmass, maxmass, descent));
				std::pair<cmassmap_t::iterator, bool> ins(ceilings[ii->second.get_isaoffset()].
									 insert(cmassmap_t::value_type(ii->second.get_mass(), ii->second.get_ceiling())));
				if (!ins.second)
					ins.first->second = std::max(ins.first->second, ii->second.get_ceiling());
			}
		for (cisamap_t::const_iterator ii(ceilings.begin()), ie(ceilings.end()); ii != ie; ++ii) {
			for (cmassmap_t::const_iterator mi(ii->second.begin()), me(ii->second.end()); mi != me; ) {
				cmassmap_t::const_iterator mi2(mi);
				++mi;
				if (mi == me)
					break;
				if (mi2->second >= mi->second)
					continue;
				r.add(descent ? CheckError::type_descent : CheckError::type_climb,
				      CheckError::severity_warning, ci->first, mi2->first, ii->first)
					<< "curve has lower ceiling than higher mass" << mi->first;
			}
		}
	}
	return r;
}

std::ostream& Aircraft::Climb::print(std::ostream& os, const std::string& indent) const
{
	for (curves_t::const_iterator ci(m_curves.begin()), ce(m_curves.end()); ci != ce; ++ci)
		for (massmap_t::const_iterator mi(ci->second.begin()), me(ci->second.end()); mi != me; ++mi)
			for (isamap_t::const_iterator ii(mi->second.begin()), ie(mi->second.end()); ii != ie; ++ii)
				ii->second.print(os << indent << "Profile Name \"" << ci->first << "\" mass " << mi->first
						 << " isaoffs " << ii->first << std::endl, indent + "  ");
	return os;	
}

Aircraft::Descent::Descent(double altfactor, double ratefactor, double fuelflowfactor, double casfactor,
			   double timefactor, double fuelfactor, double distfactor)
	: Climb(altfactor, ratefactor, fuelflowfactor, casfactor, timefactor, fuelfactor, distfactor)
{
	m_curves.clear();
	{
		ClimbDescent cd;
		// descent rate in ft/min
		cd.set_descent_rate(1000);
		add(cd);
	}
}

void Aircraft::Descent::load_xml(const xmlpp::Element *el)
{
	if (!el)
		return;
	Climb::load_xml(el);
	xmlpp::Attribute *attr;
	if ((attr = el->get_attribute("rate"))) {
		double rate(Glib::Ascii::strtod(attr->get_value()));
		std::set<std::string> cn(get_curves());
		if (cn.empty() && !std::isnan(rate) && (rate >= 100)) {
			ClimbDescent cd;
			cd.set_descent_rate(rate);
			add(cd);
		}
	}
}

void Aircraft::Descent::save_xml(xmlpp::Element *el) const
{
	if (!el)
		return;
	Climb::save_xml(el);
	{
		double rate(get_rate());
		if (std::isnan(rate)) {
			el->remove_attribute("rate");
		} else {
			std::ostringstream oss;
			oss << rate;
			el->set_attribute("rate", oss.str());
		}
	}
}

double Aircraft::Descent::get_rate(void) const
{
	double rate(std::numeric_limits<double>::quiet_NaN());
	for (curves_t::const_iterator ci(m_curves.begin()), ce(m_curves.end()); ci != ce; ++ci)
		for (massmap_t::const_iterator mi(ci->second.begin()), me(ci->second.end()); mi != me; ++mi)
			for (isamap_t::const_iterator ii(mi->second.begin()), ie(mi->second.end()); ii != ie; ++ii) {
				if (ii->second.get_ratepoly().size() != 1)
					return std::numeric_limits<double>::quiet_NaN();
				double r(ii->second.get_ratepoly().front());
				if (std::isnan(r))
					return std::numeric_limits<double>::quiet_NaN();
				if (!std::isnan(rate) && rate != r)
					return std::numeric_limits<double>::quiet_NaN();
				rate = r;
			}
	return rate;
}

void Aircraft::Descent::set_default(propulsion_t prop, const Cruise& cruise, const Climb& climb)
{
	double ceiling(climb.get_ceiling());
	if (std::isnan(ceiling) || ceiling < 1000)
		return;
	double rate(std::numeric_limits<double>::quiet_NaN()), mass(std::numeric_limits<double>::quiet_NaN()), isaoffs(0);
	{
		const ClimbDescent *cd(find_single_curve());
		if (!cd)
			return;
		if (cd->has_points())
			return;
		double ff, cas;
		cd->calculate(rate, ff, cas, 0);
		if (std::isnan(rate))
			return;
		mass = cd->get_mass();
		isaoffs = cd->get_isaoffset();
	}
	std::set<std::string> cn(cruise.get_curves());
	if (cn.empty())
		return;
	AirData<float> ad;
	ad.set_qnh_temp();
	bool first(true);
	for (std::set<std::string>::const_iterator ci(cn.begin()), ce(cn.end()); ci != ce; ++ci) {
		if (cruise.get_curve_flags(*ci) & Cruise::Curve::flags_hold)
			continue;
		ClimbDescent cd(*ci, mass, isaoffs);
		cd.clear_points();
		cd.set_ceiling(ceiling);
		typedef std::set<double> alts_t;
		alts_t alts;
		for (double pa = 0;; pa += 1000) {
			bool last(pa >= ceiling);
			if (last)
				pa = ceiling;
			double tas(0), fuelflow(0), pa1(pa), mass1(mass), isaoffs1(isaoffs);
			Cruise::CruiseEngineParams ep(*ci);
			cruise.calculate(prop, tas, fuelflow, pa1, mass1, isaoffs1, ep);
			if (false)
				std::cerr << "Descent: " << *ci << " tas " << tas << " ff " << fuelflow << " pa " << pa1 << " rate " << rate << std::endl;
			if (!std::isnan(tas) && !std::isnan(fuelflow) && !std::isnan(pa1) && tas >= 1 && pa1 >= 0) {
				if (alts.insert(pa1).second) {
					ad.set_pressure_altitude(pa1);
					ad.set_tas(tas);
					cd.add_point(ClimbDescent::Point(pa1, rate, fuelflow, ad.get_cas()));
				}
			}
			if (last)
				break;
		}
		cd.recalculatepoly(true);
		if (first) {
			m_curves.clear();
			first = false;
		}
		add(cd);
	}
}

Aircraft::ClimbDescent Aircraft::Descent::calculate(const std::string& name, double mass, double isaoffs, double qnh) const
{
	curves_t::const_iterator ci(m_curves.find(name));
	if (ci == m_curves.end()) {
		ci = m_curves.begin();
		if (ci == m_curves.end())
			return ClimbDescent("", 2600);
	}
	return calculate(ci->second, mass, isaoffs, qnh);
}

Aircraft::ClimbDescent Aircraft::Descent::calculate(std::string& name, const std::string& dfltname, double mass, double isaoffs, double qnh) const
{
	curves_t::const_iterator ci(m_curves.find(name));
	if (ci == m_curves.end()) {
		ci = m_curves.find(dfltname);
		if (ci == m_curves.end()) {
			ci = m_curves.begin();
			if (ci == m_curves.end())
				return ClimbDescent("", 2600);
		} else {
			name = dfltname;
		}
	}
	return calculate(ci->second, mass, isaoffs, qnh);
}

Aircraft::ClimbDescent Aircraft::Descent::calculate(const massmap_t& ci, double mass, double isaoffs, double qnh) const
{
	massmap_t::const_iterator miu(ci.lower_bound(mass));
	if (miu == ci.end()) {
		if (miu == ci.begin())
			return ClimbDescent("", 2600);
		--miu;
		if (miu != ci.begin())
			return calculate(miu->second, isaoffs, qnh);
		ClimbDescent r(calculate(miu->second, isaoffs, qnh));
		//r.recalculateformass(mass);
		return r;
	}
	massmap_t::const_iterator mil(miu);
	if (miu == ci.begin()) {
		++miu;
		if (miu == ci.end()) {
			ClimbDescent r(calculate(mil->second, isaoffs, qnh));
			//r.recalculateformass(mass);
			return r;
		}
	} else {
		--mil;
	}
	double t((mass - mil->first) / (miu->first - mil->first));
	if (t <= 0)
		return calculate(mil->second, isaoffs, qnh);
	if (t >= 1)
		return calculate(miu->second, isaoffs, qnh);
	return calculate(miu->second, isaoffs, qnh).interpolate(t, calculate(mil->second, isaoffs, qnh));
}

Aircraft::Cruise::Point::Point(double pa, double bhp, double tas, double fuelflow)
	: m_pa(pa), m_bhp(bhp), m_tas(tas), m_fuelflow(fuelflow)
{
}

void Aircraft::Cruise::Point::load_xml(const xmlpp::Element *el, PistonPower& pp, double altfactor, double tasfactor, double fuelfactor, double isaoffs)
{
	if (!el)
		return;
	m_pa = m_bhp = m_tas = m_fuelflow = std::numeric_limits<double>::quiet_NaN();
	double rpm(std::numeric_limits<double>::quiet_NaN());
	double mp(std::numeric_limits<double>::quiet_NaN());
	xmlpp::Attribute *attr;
	if ((attr = el->get_attribute("pa")))
		m_pa = Glib::Ascii::strtod(attr->get_value()) * altfactor;
	else if ((attr = el->get_attribute("da")))
		m_pa = Glib::Ascii::strtod(attr->get_value()) * altfactor;
	if ((attr = el->get_attribute("rpm")))
		rpm = Glib::Ascii::strtod(attr->get_value());
	if ((attr = el->get_attribute("mp")))
		mp = Glib::Ascii::strtod(attr->get_value());
	if ((attr = el->get_attribute("bhp")))
		m_bhp = Glib::Ascii::strtod(attr->get_value());
	if ((attr = el->get_attribute("tas")))
		m_tas = Glib::Ascii::strtod(attr->get_value()) * tasfactor;
	if ((attr = el->get_attribute("fuelflow")))
		m_fuelflow = Glib::Ascii::strtod(attr->get_value()) * fuelfactor;
	pp.add(m_pa, isaoffs, m_bhp, rpm, mp);
}

void Aircraft::Cruise::Point::save_xml(xmlpp::Element *el, double altfactor, double tasfactor, double fuelfactor) const
{
	if (!el)
		return;
	if (std::isnan(m_pa) || altfactor == 0) {
		el->remove_attribute("pa");
	} else {
		std::ostringstream oss;
		oss << (m_pa / altfactor);
		el->set_attribute("pa", oss.str());
	}
	if (std::isnan(m_bhp)) {
		el->remove_attribute("bhp");
	} else {
		std::ostringstream oss;
		oss << m_bhp;
		el->set_attribute("bhp", oss.str());
	}
	if (std::isnan(m_tas) || tasfactor == 0) {
		el->remove_attribute("tas");
	} else {
		std::ostringstream oss;
		oss << (m_tas / tasfactor);
		el->set_attribute("tas", oss.str());
	}
	if (std::isnan(m_fuelflow) || fuelfactor == 0) {
		el->remove_attribute("fuelflow");
	} else {
		std::ostringstream oss;
		oss << (m_fuelflow / fuelfactor);
		el->set_attribute("fuelflow", oss.str());
	}
}

int Aircraft::Cruise::Point::compare(const Point& x) const
{
	if (get_pressurealt() < x.get_pressurealt())
		return -1;
	if (x.get_pressurealt() < get_pressurealt())
		return 1;
	return 0;
}

Aircraft::Cruise::PointRPMMP::PointRPMMP(double pa, double rpm, double mp, double bhp, double tas, double fuelflow)
	: Point(pa, bhp, tas, fuelflow), m_rpm(rpm), m_mp(mp)
{
}

void Aircraft::Cruise::PointRPMMP::load_xml(const xmlpp::Element *el, PistonPower& pp, double altfactor, double tasfactor, double fuelfactor, double isaoffs)
{
	if (!el)
		return;
	m_pa = m_bhp = m_tas = m_fuelflow = m_rpm = m_mp = std::numeric_limits<double>::quiet_NaN();
	xmlpp::Attribute *attr;
	if ((attr = el->get_attribute("pa")))
		m_pa = Glib::Ascii::strtod(attr->get_value()) * altfactor;
	else if ((attr = el->get_attribute("da")))
		m_pa = Glib::Ascii::strtod(attr->get_value()) * altfactor;
	if ((attr = el->get_attribute("rpm")))
		m_rpm = Glib::Ascii::strtod(attr->get_value());
	if ((attr = el->get_attribute("mp")))
		m_mp = Glib::Ascii::strtod(attr->get_value());
	if ((attr = el->get_attribute("bhp")))
		m_bhp = Glib::Ascii::strtod(attr->get_value());
	if ((attr = el->get_attribute("tas")))
		m_tas = Glib::Ascii::strtod(attr->get_value()) * tasfactor;
	if ((attr = el->get_attribute("fuelflow")))
		m_fuelflow = Glib::Ascii::strtod(attr->get_value()) * fuelfactor;
	pp.add(m_pa, isaoffs, m_bhp, m_rpm, m_mp);
}

void Aircraft::Cruise::PointRPMMP::save_xml(xmlpp::Element *el, double altfactor, double tasfactor, double fuelfactor) const
{
	if (!el)
		return;
	if (std::isnan(m_pa) || altfactor == 0) {
		el->remove_attribute("pa");
	} else {
		std::ostringstream oss;
		oss << (m_pa / altfactor);
		el->set_attribute("pa", oss.str());
	}
	if (std::isnan(m_rpm)) {
		el->remove_attribute("rpm");
	} else {
		std::ostringstream oss;
		oss << m_rpm;
		el->set_attribute("rpm", oss.str());
	}
	if (std::isnan(m_mp)) {
		el->remove_attribute("mp");
	} else {
		std::ostringstream oss;
		oss << m_mp;
		el->set_attribute("mp", oss.str());
	}
	if (std::isnan(m_bhp)) {
		el->remove_attribute("bhp");
	} else {
		std::ostringstream oss;
		oss << m_bhp;
		el->set_attribute("bhp", oss.str());
	}
	if (std::isnan(m_tas) || tasfactor == 0) {
		el->remove_attribute("tas");
	} else {
		std::ostringstream oss;
		oss << (m_tas / tasfactor);
		el->set_attribute("tas", oss.str());
	}
	if (std::isnan(m_fuelflow) || fuelfactor == 0) {
		el->remove_attribute("fuelflow");
	} else {
		std::ostringstream oss;
		oss << (m_fuelflow / fuelfactor);
		el->set_attribute("fuelflow", oss.str());
	}
}

const Aircraft::Cruise::Curve::FlagName Aircraft::Cruise::Curve::flagnames[] =
{
	{ "interpolate", flags_interpolate },
	{ "hold", flags_hold },
	{ 0, flags_none }
};

Aircraft::Cruise::Curve::Curve(const std::string& name, flags_t flags, double mass, double isaoffs, double rpm)
	: m_name(name), m_mass(mass), m_isaoffset(isaoffs), m_rpm(rpm), m_flags(flags)
{
}

void Aircraft::Cruise::Curve::add_point(const Point& pt)
{
	insert(pt);
}

void Aircraft::Cruise::Curve::clear_points(void)
{
	clear();
}

void Aircraft::Cruise::Curve::load_xml(const xmlpp::Element *el, PistonPower& pp, double altfactor, double tasfactor, double fuelfactor)
{
	clear();
	m_name.clear();
	m_mass = std::numeric_limits<double>::quiet_NaN();
	m_isaoffset = 0;
	m_rpm = std::numeric_limits<double>::quiet_NaN();
	m_flags = flags_none;
	xmlpp::Attribute *attr;
	if ((attr = el->get_attribute("name")))
		m_name = attr->get_value();
	if ((attr = el->get_attribute("mass")))
		m_mass = Glib::Ascii::strtod(attr->get_value());
	if ((attr = el->get_attribute("isaoffset")))
		m_isaoffset = Glib::Ascii::strtod(attr->get_value());
	else if ((attr = el->get_attribute("isaoffs")))
		m_isaoffset = Glib::Ascii::strtod(attr->get_value());
	if ((attr = el->get_attribute("rpm")))
		m_rpm = Glib::Ascii::strtod(attr->get_value());
	if ((attr = el->get_attribute("flags"))) {
		Glib::ustring val(attr->get_value());
		if (false)
			std::cerr << "Curve::load_xml: name " << get_name() << " mass " << get_mass()
				  << " isaoffset " << get_isaoffset() << " flags \"" << val << "\"" << std::endl;
		Glib::ustring::size_type vn(0);
		while (vn < val.size()) {
			Glib::ustring::size_type vn2(val.find('|', vn));
			Glib::ustring v1(val.substr(vn, vn2 - vn));
			vn = vn2;
			if (vn < val.size())
				++vn;
			if (false)
				std::cerr << "Curve::load_xml: name " << get_name() << " mass " << get_mass()
					  << " isaoffset " << get_isaoffset() << " flag \"" << v1 << "\"" << std::endl;
			if (v1.empty())
				continue;
			if (*std::min_element(v1.begin(), v1.end()) >= (Glib::ustring::value_type)'0' &&
			    *std::max_element(v1.begin(), v1.end()) <= (Glib::ustring::value_type)'9') {
				m_flags |= (flags_t)strtol(v1.c_str(), 0, 10);
				continue;
			}
			const FlagName *fn = flagnames;
			for (; fn->m_name; ++fn)
				if (fn->m_name == v1)
					break;
			if (!fn->m_name) {
				std::cerr << "Curve::load_xml: name " << get_name() << " mass " << get_mass()
					  << " isaoffset " << get_isaoffset() << " invalid flag " << v1 << std::endl;
				continue;
			}
			m_flags |= fn->m_flag;
		}
	}
	{
		std::pair<double,double> rpmr(std::numeric_limits<double>::max(), -std::numeric_limits<double>::max());
		xmlpp::Node::NodeList nl(el->get_children("point"));
		for (xmlpp::Node::NodeList::const_iterator ni(nl.begin()), ne(nl.end()); ni != ne; ++ni) {
			PointRPMMP pt;
			pt.load_xml(static_cast<xmlpp::Element *>(*ni), pp, altfactor, tasfactor, fuelfactor);
			if (std::isnan(pt.get_pressurealt()))
				continue;
			if (std::isnan(pt.get_rpm())) {
				rpmr.first = rpmr.second = std::numeric_limits<double>::quiet_NaN();
			} else if (!std::isnan(rpmr.first) && !std::isnan(rpmr.second)) {
				rpmr.first = std::min(rpmr.first, pt.get_rpm());
				rpmr.second = std::max(rpmr.second, pt.get_rpm());
			}
			insert(pt);
		}
		if (!std::isnan(rpmr.first) && !std::isnan(rpmr.second) && std::isnan(m_rpm) && rpmr.first > 0 && rpmr.second <= rpmr.first * 1.01)
			m_rpm = rpmr.first;
	}
	{
		std::pair<double,double> bhp(get_bhp_range());
		if (std::isnan(bhp.first) || std::isnan(bhp.second))
			m_flags &= ~flags_interpolate;
	}
}

void Aircraft::Cruise::Curve::save_xml(xmlpp::Element *el, double altfactor, double tasfactor, double fuelfactor) const
{
	if (!el)
		return;
	{
		xmlpp::Node::NodeList nl(el->get_children());
		for (xmlpp::Node::NodeList::const_iterator ni(nl.begin()), ne(nl.end()); ni != ne; ++ni)
			el->remove_child(*ni);
	}
	if (m_name.empty())
		el->remove_attribute("name");
	else
		el->set_attribute("name", m_name);
	if (std::isnan(m_mass)) {
		el->remove_attribute("mass");
	} else {
		std::ostringstream oss;
		oss << m_mass;
		el->set_attribute("mass", oss.str());
	}
	if (m_isaoffset == 0) {
		el->remove_attribute("isaoffset");
	} else {
		std::ostringstream oss;
		oss << m_isaoffset;
		el->set_attribute("isaoffset", oss.str());
	}
	if (std::isnan(m_rpm)) {
		el->remove_attribute("rpm");
	} else {
		std::ostringstream oss;
		oss << m_rpm;
		el->set_attribute("rpm", oss.str());
	}
	{
		Glib::ustring flg;
		flags_t flags(m_flags);
		for (const FlagName *fn = flagnames; fn->m_name; ++fn) {
			if (fn->m_flag & ~flags)
				continue;
			flags &= ~fn->m_flag;
			flg += "|";
			flg += fn->m_name;
		}
		if (flags) {
			std::ostringstream oss;
			oss << '|' << (unsigned int)flags;
			flg += oss.str();
		}
		if (flg.empty())
			el->remove_attribute("flags");
		else
			el->set_attribute("flags", flg.substr(1));
	}
	for (const_iterator pi(begin()), pe(end()); pi != pe; ++pi)
		pi->save_xml(el->add_child("point"));
}

int Aircraft::Cruise::Curve::compare(const Curve& x) const
{
	return get_name().compare(x.get_name());
}

std::pair<double,double> Aircraft::Cruise::Curve::get_bhp_range(void) const
{
	std::pair<double,double> r(std::numeric_limits<double>::max(), -std::numeric_limits<double>::max());
	for (const_iterator i(begin()), e(end()); i != e; ++i) {
		if (std::isnan(i->get_bhp()))
			return std::pair<double,double>(std::numeric_limits<double>::quiet_NaN(),
							std::numeric_limits<double>::quiet_NaN());
		r.first = std::min(r.first, i->get_bhp());
		r.second = std::max(r.second, i->get_bhp());
	}
	if (r.first > r.second)
		return std::pair<double,double>(std::numeric_limits<double>::quiet_NaN(),
						std::numeric_limits<double>::quiet_NaN());
	return r;
}

bool Aircraft::Cruise::Curve::is_bhp_const(void) const
{
	std::pair<double,double> r(get_bhp_range());
	if (std::isnan(r.first) || r.first <= 0 || r.second > r.first * 1.01)
		return false;
	return true;
}

void Aircraft::Cruise::Curve::calculate(double& tas, double& fuelflow, double& pa, CruiseEngineParams& ep) const
{
	static const bool debug(false);
	ep.set_rpm(std::numeric_limits<double>::quiet_NaN());
	ep.set_mp(std::numeric_limits<double>::quiet_NaN());
	if (std::isnan(pa) || empty()) {
		tas = fuelflow = std::numeric_limits<double>::quiet_NaN();
		ep.set_bhp(std::numeric_limits<double>::quiet_NaN());
		if (debug)
			std::cerr << "Cruise::Curve::calculate: name " << get_name() << " mass " << get_mass() << " isaoffs " << get_isaoffset()
				  << " pa " << pa << " tas " << tas << " ff " << fuelflow << " bhp " << ep.get_bhp() << std::endl;
		return;
	}
	const_iterator i(lower_bound(Point(pa)));
	if (i == begin() || i == end()) {
		if (i == end())
			--i;
		tas = i->get_tas();
		fuelflow = i->get_fuelflow();
		pa = i->get_pressurealt();
		ep.set_bhp(i->get_bhp());
		ep.set_rpm(get_rpm());
		if (debug)
			std::cerr << "Cruise::Curve::calculate: name " << get_name() << " mass " << get_mass() << " isaoffs " << get_isaoffset()
				  << " pa " << pa << " tas " << tas << " ff " << fuelflow << " bhp " << ep.get_bhp() << std::endl;
		return;
	}
	const_iterator i0(i);
	--i0;
	double x((pa - i0->get_pressurealt()) / (i->get_pressurealt() - i0->get_pressurealt()));
	double x0(1.0 - x);
	tas = i->get_tas() * x + i0->get_tas() * x0;
	fuelflow = i->get_fuelflow() * x + i0->get_fuelflow() * x0;
	ep.set_bhp(i->get_bhp() * x + i0->get_bhp() * x0);
	ep.set_rpm(get_rpm());
	if (debug)
		std::cerr << "Cruise::Curve::calculate: name " << get_name() << " mass " << get_mass() << " isaoffs " << get_isaoffset()
			  << " pa " << pa << " tas " << tas << " ff " << fuelflow << " bhp " << ep.get_bhp()
			  << " pa0 " << i0->get_pressurealt() << " (" << x0 << ") pa1 " << i->get_pressurealt() << " (" << x << ')'
			  << " tas0 " << i0->get_tas() << " tas1 " << i->get_tas()
			  << " ff0 " << i0->get_fuelflow() << " ff1 " << i->get_fuelflow() << std::endl;
}

void Aircraft::Cruise::Curve::set_mass(double mass)
{
	if (std::isnan(mass) || mass <= 0)
		return;
	m_mass = mass;
}

Aircraft::CheckErrors Aircraft::Cruise::Curve::check(double minmass, double maxmass) const
{
	CheckErrors r;
	if (get_mass() < minmass * 0.9)
		r.add(*this, CheckError::severity_warning) << "mass less than empty mass - 10%";
	if (get_mass() > maxmass)
		r.add(*this, CheckError::severity_warning) << "mass more than maximum takeoff mass";
	if (empty()) {
		r.add(*this, CheckError::severity_error) << "no points";
		return r;
	}
	if (begin()->get_pressurealt() > 2000)
		r.add(*this, CheckError::severity_error) << "does not include a low level point";
	return r;
}

Aircraft::Cruise::CurveRPMMP::CurveRPMMP(const std::string& name)
	: m_name(name)
{
}

void Aircraft::Cruise::CurveRPMMP::load_xml(const xmlpp::Element *el, PistonPower& pp, double altfactor, double tasfactor, double fuelfactor)
{
	clear();
	m_name.clear();
	xmlpp::Attribute *attr;
	if ((attr = el->get_attribute("name")))
		m_name = attr->get_value();
	xmlpp::Node::NodeList nl(el->get_children("point"));
	for (xmlpp::Node::NodeList::const_iterator ni(nl.begin()), ne(nl.end()); ni != ne; ++ni) {
		PointRPMMP pt;
		pt.load_xml(static_cast<xmlpp::Element *>(*ni), pp, altfactor, tasfactor, fuelfactor);
		if (std::isnan(pt.get_pressurealt()))
			continue;
		insert(pt);
	}
}

void Aircraft::Cruise::CurveRPMMP::save_xml(xmlpp::Element *el, double altfactor, double tasfactor, double fuelfactor) const
{
	if (!el)
		return;
	{
		xmlpp::Node::NodeList nl(el->get_children());
		for (xmlpp::Node::NodeList::const_iterator ni(nl.begin()), ne(nl.end()); ni != ne; ++ni)
			el->remove_child(*ni);
	}
	if (m_name.empty())
		el->remove_attribute("name");
	else
		el->set_attribute("name", m_name);
	for (const_iterator pi(begin()), pe(end()); pi != pe; ++pi)
		pi->save_xml(el->add_child("point"));
}

bool Aircraft::Cruise::CurveRPMMP::is_mp(void) const
{
	for (const_iterator i(begin()), e(end()); i != e; ++i)
		if (std::isnan(i->get_mp()))
			return false;
	return !empty();
}

std::pair<double,double> Aircraft::Cruise::CurveRPMMP::get_bhp_range(void) const
{
	std::pair<double,double> r(std::numeric_limits<double>::max(), -std::numeric_limits<double>::max());
	for (const_iterator i(begin()), e(end()); i != e; ++i) {
		if (std::isnan(i->get_bhp()))
			return std::pair<double,double>(std::numeric_limits<double>::quiet_NaN(),
							std::numeric_limits<double>::quiet_NaN());
		r.first = std::min(r.first, i->get_bhp());
		r.second = std::max(r.second, i->get_bhp());
	}
	if (r.first > r.second)
		return std::pair<double,double>(std::numeric_limits<double>::quiet_NaN(),
						std::numeric_limits<double>::quiet_NaN());
	return r;
}

std::pair<double,double> Aircraft::Cruise::CurveRPMMP::get_rpm_range(void) const
{
	std::pair<double,double> r(std::numeric_limits<double>::max(), -std::numeric_limits<double>::max());
	for (const_iterator i(begin()), e(end()); i != e; ++i) {
		if (std::isnan(i->get_rpm()))
			return std::pair<double,double>(std::numeric_limits<double>::quiet_NaN(),
							std::numeric_limits<double>::quiet_NaN());
		r.first = std::min(r.first, i->get_rpm());
		r.second = std::max(r.second, i->get_rpm());
	}
	if (r.first > r.second)
		return std::pair<double,double>(std::numeric_limits<double>::quiet_NaN(),
						std::numeric_limits<double>::quiet_NaN());
	return r;
}

bool Aircraft::Cruise::CurveRPMMP::is_bhp_const(void) const
{
	std::pair<double,double> r(get_bhp_range());
	if (std::isnan(r.first) || r.first <= 0 || r.second > r.first * 1.01)
		return false;
	return true;
}

bool Aircraft::Cruise::CurveRPMMP::is_rpm_const(void) const
{
	std::pair<double,double> r(get_rpm_range());
	if (std::isnan(r.first) || r.first <= 0 || r.second > r.first * 1.01)
		return false;
	return true;
}

Aircraft::Cruise::CurveRPMMP::operator Curve(void) const
{
	double rpm(std::numeric_limits<double>::quiet_NaN());
	if (is_rpm_const())
		rpm = get_rpm_range().first;
	Curve c(get_name(), Curve::flags_interpolate,
		std::numeric_limits<double>::quiet_NaN(), 0, rpm);
	for (const_iterator i(begin()), e(end()); i != e; ++i)
		c.insert(*i);
	return c;
}

Aircraft::Cruise::CruiseEngineParams::CruiseEngineParams(double bhp, double rpm, double mp, const std::string& name, Curve::flags_t flags)
	: m_name(name), m_bhp(bhp), m_rpm(rpm), m_mp(mp), m_flags(flags)
{
}

Aircraft::Cruise::CruiseEngineParams::CruiseEngineParams(const std::string& name, Curve::flags_t flags)
	: m_name(name), m_bhp(std::numeric_limits<double>::quiet_NaN()), m_rpm(std::numeric_limits<double>::quiet_NaN()),
	  m_mp(std::numeric_limits<double>::quiet_NaN()), m_flags(flags)
{
}
	     
bool Aircraft::Cruise::CruiseEngineParams::is_unset(void) const
{
	return std::isnan(get_bhp()) && std::isnan(get_rpm()) && std::isnan(get_mp()) &&
		get_name().empty() && (get_flags() & ~Curve::flags_interpolate) == Curve::flags_none;
}

int Aircraft::Cruise::CruiseEngineParams::compare(const CruiseEngineParams& x) const
{
	int c(get_name().compare(x.get_name()));
	if (c)
		return c;
	if (std::isnan(get_bhp())) {
		if (!std::isnan(x.get_bhp()))
			return -1;
	} else {
		if (std::isnan(x.get_bhp()))
			return 1;
		if (get_bhp() < x.get_bhp())
			return -1;
		if (x.get_bhp() < get_bhp())
			return 1;
	}
	if (std::isnan(get_rpm())) {
		if (!std::isnan(x.get_rpm()))
			return -1;
	} else {
		if (std::isnan(x.get_rpm()))
			return 1;
		if (get_rpm() < x.get_rpm())
			return -1;
		if (x.get_rpm() < get_rpm())
			return 1;
	}
	if (std::isnan(get_mp())) {
		if (!std::isnan(x.get_mp()))
			return -1;
	} else {
		if (std::isnan(x.get_mp()))
			return 1;
		if (get_mp() < x.get_mp())
			return -1;
		if (x.get_mp() < get_mp())
			return 1;
	}
	if (get_flags() < x.get_flags())
		return -1;
	if (x.get_flags() < get_flags())
		return 1;
	return 0;
}

std::ostream& Aircraft::Cruise::CruiseEngineParams::print(std::ostream& os) const
{
	bool any(false);
	if (!std::isnan(get_bhp())) {
		os << "bhp " << get_bhp();
		any = true;
	}
	if (!std::isnan(get_rpm())) {
		if (any)
			os << ' ';
		os << "rpm " << get_rpm();
		any = true;
	}
	if (!std::isnan(get_mp())) {
		if (any)
			os << ' ';
		os << "mp " << get_mp();
		any = true;
	}
	if (!get_name().empty()) {
		if (any)
			os << ' ';
		os << "name " << get_name();
		any = true;
	}
	if (!any)
		os << '-';
	if (Curve::flags_interpolate & get_flags())
		os << " interpolate";
	if (Curve::flags_hold & get_flags())
		os << " hold";
	return os;
}



Aircraft::Cruise::EngineParams::EngineParams(double bhp, double rpm, double mp, const std::string& name, Curve::flags_t flags,
					     const std::string& climbname, const std::string& descentname)
	: CruiseEngineParams(bhp, rpm, mp, name, flags), m_climbname(climbname), m_descentname(descentname)
{
}

Aircraft::Cruise::EngineParams::EngineParams(const std::string& name, Curve::flags_t flags,
					     const std::string& climbname, const std::string& descentname)
	: CruiseEngineParams(name, flags), m_climbname(climbname), m_descentname(descentname)
{
}
	     
bool Aircraft::Cruise::EngineParams::is_unset(void) const
{
	return get_climbname().empty() && get_descentname().empty() && CruiseEngineParams::is_unset();
}

int Aircraft::Cruise::EngineParams::compare(const EngineParams& x) const
{
	int c(get_name().compare(x.get_name()));
	if (c)
		return c;
	c = get_climbname().compare(x.get_climbname());
	if (c)
		return c;
	c = get_descentname().compare(x.get_descentname());
	if (c)
		return c;
	if (std::isnan(get_bhp())) {
		if (!std::isnan(x.get_bhp()))
			return -1;
	} else {
		if (std::isnan(x.get_bhp()))
			return 1;
		if (get_bhp() < x.get_bhp())
			return -1;
		if (x.get_bhp() < get_bhp())
			return 1;
	}
	if (std::isnan(get_rpm())) {
		if (!std::isnan(x.get_rpm()))
			return -1;
	} else {
		if (std::isnan(x.get_rpm()))
			return 1;
		if (get_rpm() < x.get_rpm())
			return -1;
		if (x.get_rpm() < get_rpm())
			return 1;
	}
	if (std::isnan(get_mp())) {
		if (!std::isnan(x.get_mp()))
			return -1;
	} else {
		if (std::isnan(x.get_mp()))
			return 1;
		if (get_mp() < x.get_mp())
			return -1;
		if (x.get_mp() < get_mp())
			return 1;
	}
	if (get_flags() < x.get_flags())
		return -1;
	if (x.get_flags() < get_flags())
		return 1;
	return 0;
}

std::ostream& Aircraft::Cruise::EngineParams::print(std::ostream& os) const
{
	bool any(false);
	if (!std::isnan(get_bhp())) {
		os << "bhp " << get_bhp();
		any = true;
	}
	if (!std::isnan(get_rpm())) {
		if (any)
			os << ' ';
		os << "rpm " << get_rpm();
		any = true;
	}
	if (!std::isnan(get_mp())) {
		if (any)
			os << ' ';
		os << "mp " << get_mp();
		any = true;
	}
	if (!get_name().empty()) {
		if (any)
			os << ' ';
		os << "name " << get_name();
		any = true;
	}
	if (!get_climbname().empty()) {
		if (any)
			os << ' ';
		os << "climb " << get_climbname();
		any = true;
	}
	if (!get_descentname().empty()) {
		if (any)
			os << ' ';
		os << "descent " << get_descentname();
		any = true;
	}
	if (!any)
		os << '-';
	if (Curve::flags_interpolate & get_flags())
		os << " interpolate";
	if (Curve::flags_hold & get_flags())
		os << " hold";
	return os;
}


Aircraft::Cruise::PistonPowerBHPRPM::PistonPowerBHPRPM(double bhp, double rpm)
	: m_bhp(bhp), m_rpm(rpm)
{
}

int Aircraft::Cruise::PistonPowerBHPRPM::compare(const PistonPowerBHPRPM& x) const
{
	if (get_bhp() < x.get_bhp())
		return -1;
	if (x.get_bhp() < get_bhp())
		return 1;
	if (get_rpm() < x.get_rpm())
		return -1;
	if (x.get_rpm() < get_rpm())
		return 1;
	return 0;
}

void Aircraft::Cruise::PistonPowerBHPRPM::save_xml(xmlpp::Element *el, double alt, double isaoffs, double bhpfactor) const
{
	if (!el)
		return;
	el = el->add_child("point");
	if (std::isnan(alt)) {
		el->remove_attribute("pa");
	} else {
		std::ostringstream oss;
		oss << alt;
		el->set_attribute("pa", oss.str());
	}
	if (std::isnan(isaoffs)) {
		el->remove_attribute("isaoffs");
	} else {
		std::ostringstream oss;
		oss << isaoffs;
		el->set_attribute("isaoffs", oss.str());
	}
	if (std::isnan(get_bhp()) || bhpfactor == 0) {
		el->remove_attribute("bhp");
	} else {
		std::ostringstream oss;
		oss << (get_bhp() / bhpfactor);
		el->set_attribute("bhp", oss.str());
	}
	if (std::isnan(get_rpm())) {
		el->remove_attribute("rpm");
	} else {
		std::ostringstream oss;
		oss << get_rpm();
		el->set_attribute("rpm", oss.str());
	}
}

Aircraft::Cruise::PistonPowerBHPMP::PistonPowerBHPMP(double bhp, double mp)
	: m_bhp(bhp), m_mp(mp)
{
}

int Aircraft::Cruise::PistonPowerBHPMP::compare(const PistonPowerBHPMP& x) const
{
	if (get_bhp() < x.get_bhp())
		return -1;
	if (x.get_bhp() < get_bhp())
		return 1;
	if (get_mp() < x.get_mp())
		return -1;
	if (x.get_mp() < get_mp())
		return 1;
	return 0;
}

void Aircraft::Cruise::PistonPowerBHPMP::save_xml(xmlpp::Element *el, double alt, double isaoffs, double rpm, double bhpfactor) const
{
	if (!el)
		return;
	el = el->add_child("point");
	if (std::isnan(alt)) {
		el->remove_attribute("pa");
	} else {
		std::ostringstream oss;
		oss << alt;
		el->set_attribute("pa", oss.str());
	}
	if (std::isnan(isaoffs)) {
		el->remove_attribute("isaoffs");
	} else {
		std::ostringstream oss;
		oss << isaoffs;
		el->set_attribute("isaoffs", oss.str());
	}
	if (std::isnan(get_bhp()) || bhpfactor == 0) {
		el->remove_attribute("bhp");
	} else {
		std::ostringstream oss;
		oss << (get_bhp() / bhpfactor);
		el->set_attribute("bhp", oss.str());
	}
	if (std::isnan(rpm)) {
		el->remove_attribute("rpm");
	} else {
		std::ostringstream oss;
		oss << rpm;
		el->set_attribute("rpm", oss.str());
	}
	if (std::isnan(get_mp())) {
		el->remove_attribute("mp");
	} else {
		std::ostringstream oss;
		oss << get_mp();
		el->set_attribute("mp", oss.str());
	}
}

Aircraft::Cruise::PistonPowerRPM::PistonPowerRPM(double rpm)
	: m_rpm(rpm)
{
}

int Aircraft::Cruise::PistonPowerRPM::compare(const PistonPowerRPM& x) const
{
	if (get_rpm() < x.get_rpm())
		return -1;
	if (x.get_rpm() < get_rpm())
		return 1;
	return 0;
}

void Aircraft::Cruise::PistonPowerRPM::calculate(double& bhp, double& mp) const
{
	if (!std::isnan(bhp)) {
		const_iterator iu(lower_bound(PistonPowerBHPMP(bhp, 0)));
		if (iu == end()) {
			if (iu == begin()) {
				bhp = mp = std::numeric_limits<double>::quiet_NaN();
				return;
			}
			--iu;
			bhp = iu->get_bhp();
			mp = iu->get_mp();
			return;
		}
		const_iterator il(iu);
		if (iu == begin()) {
			++iu;
			if (iu == end()) {
				bhp = il->get_bhp();
				mp = il->get_mp();
				return;
			}
		} else {
			--il;
		}
		double t((bhp - il->get_bhp()) / (iu->get_bhp() - il->get_bhp()));
		if (t <= 0) {
			bhp = il->get_bhp();
			mp = il->get_mp();
			return;
		}
		if (t >= 1) {
			bhp = iu->get_bhp();
			mp = iu->get_mp();
			return;
		}
		bhp = iu->get_bhp() * t + il->get_bhp() * (1 - t);
		mp = iu->get_mp() * t + il->get_mp() * (1 - t);
		return;
	}
	if (std::isnan(mp)) {
		bhp = mp = std::numeric_limits<double>::quiet_NaN();
		return;
	}
	const_iterator iu(begin());
	while (iu != end() && iu->get_mp() < mp)
		++iu;
	if (iu == end()) {
		if (iu == begin()) {
			bhp = mp = std::numeric_limits<double>::quiet_NaN();
			return;
		}
		--iu;
		bhp = iu->get_bhp();
		mp = iu->get_mp();
		return;
	}
	const_iterator il(iu);
	if (iu == begin()) {
		++iu;
		if (iu == end()) {
			bhp = il->get_bhp();
			mp = il->get_mp();
			return;
		}
	} else {
		--il;
	}
	double t((mp - il->get_mp()) / (iu->get_mp() - il->get_mp()));
	if (t <= 0) {
		bhp = il->get_bhp();
		mp = il->get_mp();
		return;
	}
	if (t >= 1) {
		bhp = iu->get_bhp();
		mp = iu->get_mp();
		return;
	}
	bhp = iu->get_bhp() * t + il->get_bhp() * (1 - t);
	mp = iu->get_mp() * t + il->get_mp() * (1 - t);
}

void Aircraft::Cruise::PistonPowerRPM::save_xml(xmlpp::Element *el, double alt, double isaoffs, double bhpfactor) const
{
	for (const_iterator i(begin()), e(end()); i != e; ++i)
		i->save_xml(el, alt, isaoffs, get_rpm(), bhpfactor);
}

Aircraft::Cruise::PistonPowerTemp::PistonPowerTemp(double isaoffs)
	: m_isaoffs(isaoffs)
{
}

int Aircraft::Cruise::PistonPowerTemp::compare(const PistonPowerTemp& x) const
{
	if (get_isaoffs() < x.get_isaoffs())
		return -1;
	if (x.get_isaoffs() < get_isaoffs())
		return 1;
	return 0;
}

void Aircraft::Cruise::PistonPowerTemp::calculate(double& bhp, double& rpm, double& mp) const
{
	if (!std::isnan(rpm) && !get_variablepitch().empty()) {
		double xbhp(bhp), xrpm(rpm), xmp(mp);
		variablepitch_t::const_iterator iu(get_variablepitch().lower_bound(PistonPowerRPM(xrpm)));
		if (iu == get_variablepitch().end()) {
			if (iu == get_variablepitch().begin()) {
				xbhp = xrpm = xmp = std::numeric_limits<double>::quiet_NaN();
			} else {
				--iu;
				xrpm = iu->get_rpm();
				iu->calculate(xbhp, xmp);
			}
		} else {
			bool doit(true);
			variablepitch_t::const_iterator il(iu);
			if (iu == get_variablepitch().begin()) {
				++iu;
				if (iu == get_variablepitch().end()) {
					xrpm = il->get_rpm();
					il->calculate(xbhp, xmp);
					doit = false;
				}
			} else {
				--il;
			}
			if (doit) {
				double t((xrpm - il->get_rpm()) / (iu->get_rpm() - il->get_rpm()));
				if (t <= 0) {
					il->calculate(xbhp, xmp);
				} else if (t >= 1) {
					iu->calculate(xbhp, xmp);
				} else {
					double lbhp(xbhp), lmp(xmp);
					il->calculate(lbhp, lmp);
					double ubhp(xbhp), ump(xmp);
					iu->calculate(ubhp, ump);
					xbhp = ubhp * t + lbhp * (1 - t);
					xmp = ump * t + lmp * (1 - t);
				}
			}
		}
		if (std::isnan(bhp) || xbhp >= 0.99 * bhp) {
			bhp = xbhp;
			rpm = xrpm;
			mp = xmp;
			return;
		}
		for (variablepitch_t::const_iterator i(get_variablepitch().begin()), e(get_variablepitch().end()); i != e; ++i) {
			double zbhp(bhp), zmp(mp);
			i->calculate(zbhp, zmp);
			if (zbhp >= 0.99 * bhp) {
				bhp = zbhp;
				rpm = i->get_rpm();
				mp = zmp;
				return;
			}
		}
		bhp = xbhp;
		rpm = xrpm;
		mp = xmp;
		return;	
	}
	mp = std::numeric_limits<double>::quiet_NaN();
	if (get_fixedpitch().empty()) {
		bhp = rpm = std::numeric_limits<double>::quiet_NaN();
		return;
	}
	if (!std::isnan(bhp)) {
		fixedpitch_t::const_iterator iu(get_fixedpitch().lower_bound(PistonPowerBHPRPM(bhp, 0)));
		if (iu == get_fixedpitch().end()) {
			if (iu == get_fixedpitch().begin()) {
				bhp = rpm = std::numeric_limits<double>::quiet_NaN();
				return;
			}
			--iu;
			bhp = iu->get_bhp();
			rpm = iu->get_rpm();
			return;
		}
		fixedpitch_t::const_iterator il(iu);
		if (iu == get_fixedpitch().begin()) {
			++iu;
			if (iu == get_fixedpitch().end()) {
				bhp = il->get_bhp();
				rpm = il->get_rpm();
				return;
			}
		} else {
			--il;
		}
		double t((bhp - il->get_bhp()) / (iu->get_bhp() - il->get_bhp()));
		if (t <= 0) {
			bhp = il->get_bhp();
			rpm = il->get_rpm();
			return;
		}
		if (t >= 1) {
			bhp = iu->get_bhp();
			rpm = iu->get_rpm();
			return;
		}
		bhp = iu->get_bhp() * t + il->get_bhp() * (1 - t);
		rpm = iu->get_rpm() * t + il->get_rpm() * (1 - t);
		return;
	}
	if (std::isnan(rpm)) {
		bhp = rpm = std::numeric_limits<double>::quiet_NaN();
		return;
	}
	fixedpitch_t::const_iterator iu(get_fixedpitch().begin());
	while (iu != get_fixedpitch().end() && iu->get_rpm() < rpm)
		++iu;
	if (iu == get_fixedpitch().end()) {
		if (iu == get_fixedpitch().begin()) {
			bhp = rpm = std::numeric_limits<double>::quiet_NaN();
			return;
		}
		--iu;
		bhp = iu->get_bhp();
		rpm = iu->get_rpm();
		return;
	}
	fixedpitch_t::const_iterator il(iu);
	if (iu == get_fixedpitch().begin()) {
		++iu;
		if (iu == get_fixedpitch().end()) {
			bhp = il->get_bhp();
			rpm = il->get_rpm();
			return;
		}
	} else {
		--il;
	}
	double t((rpm - il->get_rpm()) / (iu->get_rpm() - il->get_rpm()));
	if (t <= 0) {
		bhp = il->get_bhp();
		rpm = il->get_rpm();
		return;
	}
	if (t >= 1) {
		bhp = iu->get_bhp();
		rpm = iu->get_rpm();
		return;
	}
	bhp = iu->get_bhp() * t + il->get_bhp() * (1 - t);
	rpm = iu->get_rpm() * t + il->get_rpm() * (1 - t);
}

void Aircraft::Cruise::PistonPowerTemp::save_xml(xmlpp::Element *el, double alt, double tempfactor, double bhpfactor) const
{
	if (std::isnan(tempfactor) || tempfactor == 0)
		tempfactor = std::numeric_limits<double>::quiet_NaN();
	else
		tempfactor = get_isaoffs() / tempfactor;
	for (fixedpitch_t::const_iterator i(get_fixedpitch().begin()), e(get_fixedpitch().end()); i != e; ++i)
		i->save_xml(el, alt, tempfactor, bhpfactor);
	for (variablepitch_t::const_iterator i(get_variablepitch().begin()), e(get_variablepitch().end()); i != e; ++i)
		i->save_xml(el, alt, tempfactor, bhpfactor);
}

Aircraft::Cruise::PistonPowerPA::PistonPowerPA(double pa)
	: m_pa(pa)
{
}

int Aircraft::Cruise::PistonPowerPA::compare(const PistonPowerPA& x) const
{
	if (get_pa() < x.get_pa())
		return -1;
	if (x.get_pa() < get_pa())
		return 1;
	return 0;
}

void Aircraft::Cruise::PistonPowerPA::calculate(double& isaoffs, double& bhp, double& rpm, double& mp) const
{
	if (std::isnan(isaoffs)) {
		isaoffs = bhp = rpm = mp = std::numeric_limits<double>::quiet_NaN();
		return;
	}
	const_iterator iu(lower_bound(PistonPowerTemp(isaoffs)));
	if (iu == end()) {
		if (iu == begin()) {
			isaoffs = bhp = rpm = mp = std::numeric_limits<double>::quiet_NaN();
			return;
		}
		--iu;
		isaoffs = iu->get_isaoffs();
		iu->calculate(bhp, rpm, mp);
		return;
	}
	const_iterator il(iu);
	if (iu == begin()) {
		++iu;
		if (iu == end()) {
			isaoffs = il->get_isaoffs();
			il->calculate(bhp, rpm, mp);
			return;
		}
	} else {
		--il;
	}
	double t((isaoffs - il->get_isaoffs()) / (iu->get_isaoffs() - il->get_isaoffs()));
	if (t <= 0) {
		il->calculate(bhp, rpm, mp);
		return;
	}
	if (t >= 1) {
		iu->calculate(bhp, rpm, mp);
		return;
	}
	double lbhp(bhp), lrpm(rpm), lmp(mp);
	il->calculate(lbhp, lrpm, lmp);
	double ubhp(bhp), urpm(rpm), ump(mp);
	iu->calculate(ubhp, urpm, ump);
	bhp = ubhp * t + lbhp * (1 - t);
	rpm = urpm * t + lrpm * (1 - t);
	mp = ump * t + lmp * (1 - t);
}

void Aircraft::Cruise::PistonPowerPA::save_xml(xmlpp::Element *el, double altfactor, double tempfactor, double bhpfactor) const
{
	if (std::isnan(altfactor) || altfactor == 0)
		altfactor = std::numeric_limits<double>::quiet_NaN();
	else
		altfactor = get_pa() / altfactor;
	for (const_iterator i(begin()), e(end()); i != e; ++i)
		i->save_xml(el, altfactor, tempfactor, bhpfactor);
}

bool Aircraft::Cruise::PistonPowerPA::has_fixedpitch(void) const
{
	for (const_iterator i(begin()), e(end()); i != e; ++i)
		if (i->has_fixedpitch())
			return true;
	return false;
}

bool Aircraft::Cruise::PistonPowerPA::has_variablepitch(void) const
{
	for (const_iterator i(begin()), e(end()); i != e; ++i)
		if (i->has_variablepitch())
			return true;
	return false;
}

Aircraft::Cruise::PistonPower::PistonPower(void)
{
}

void Aircraft::Cruise::PistonPower::add(double pa, double isaoffs, double bhp, double rpm, double mp)
{
	if (std::isnan(pa) || std::isnan(isaoffs) || std::isnan(bhp) || std::isnan(rpm))
		return;
	iterator ip(insert(PistonPowerPA(pa)).first);
	PistonPowerPA::iterator it(const_cast<PistonPowerPA&>(*ip).insert(PistonPowerTemp(isaoffs)).first);
	if (std::isnan(mp)) {
		const_cast<PistonPowerTemp::fixedpitch_t&>(it->get_fixedpitch()).insert(PistonPowerBHPRPM(bhp, rpm));
	} else {
		PistonPowerTemp::variablepitch_t::iterator ir(const_cast<PistonPowerTemp::variablepitch_t&>(it->get_variablepitch()).insert(PistonPowerRPM(rpm)).first);
		const_cast<PistonPowerRPM&>(*ir).insert(PistonPowerBHPMP(bhp, mp));
	}	
}

void Aircraft::Cruise::PistonPower::calculate(double& pa, double& isaoffs, double& bhp, double& rpm, double& mp) const
{
	if (std::isnan(pa) || std::isnan(isaoffs)) {
		pa = isaoffs = bhp = rpm = mp = std::numeric_limits<double>::quiet_NaN();
		return;
	}
	const_iterator iu(lower_bound(PistonPowerPA(pa)));
	if (iu == end()) {
		if (iu == begin()) {
			pa = isaoffs = bhp = rpm = mp = std::numeric_limits<double>::quiet_NaN();
			return;
		}
		--iu;
		pa = iu->get_pa();
		return iu->calculate(isaoffs, bhp, rpm, mp);
	}
	const_iterator il(iu);
	if (iu == begin()) {
		++iu;
		if (iu == end()) {
			pa = il->get_pa();
			il->calculate(isaoffs, bhp, rpm, mp);
			return;
		}
	} else {
		--il;
	}
	double t((pa - il->get_pa()) / (iu->get_pa() - il->get_pa()));
	if (t <= 0) {
		il->calculate(isaoffs, bhp, rpm, mp);
		return;
	}
	if (t >= 1) {
		iu->calculate(isaoffs, bhp, rpm, mp);
		return;
	}
	double lisaoffs(isaoffs), lbhp(bhp), lrpm(rpm), lmp(mp);
	il->calculate(lisaoffs, lbhp, lrpm, lmp);
	double uisaoffs(isaoffs), ubhp(bhp), urpm(rpm), ump(mp);
	iu->calculate(uisaoffs, ubhp, urpm, ump);
	isaoffs = uisaoffs * t + lisaoffs * (1 - t);
	bhp = ubhp * t + lbhp * (1 - t);
	rpm = urpm * t + lrpm * (1 - t);
	mp = ump * t + lmp * (1 - t);
}

void Aircraft::Cruise::PistonPower::load_xml(const xmlpp::Element *el, double altfactor, double tempfactor, double bhpfactor)
{
	xmlpp::Node::NodeList nl(el->get_children("point"));
	for (xmlpp::Node::NodeList::const_iterator ni(nl.begin()), ne(nl.end()); ni != ne; ++ni) {
		const xmlpp::Element *el2(static_cast<xmlpp::Element *>(*ni));
		double pa(std::numeric_limits<double>::quiet_NaN());
		double isaoffs(std::numeric_limits<double>::quiet_NaN());
		double bhp(std::numeric_limits<double>::quiet_NaN());
		double rpm(std::numeric_limits<double>::quiet_NaN());
		double mp(std::numeric_limits<double>::quiet_NaN());
		xmlpp::Attribute *attr;
		if ((attr = el2->get_attribute("pa")))
			pa = Glib::Ascii::strtod(attr->get_value()) * altfactor;
		if ((attr = el2->get_attribute("isaoffs")))
			isaoffs = Glib::Ascii::strtod(attr->get_value()) * tempfactor;
		if ((attr = el2->get_attribute("bhp")))
			bhp = Glib::Ascii::strtod(attr->get_value()) * bhpfactor;
		if ((attr = el2->get_attribute("rpm")))
			rpm = Glib::Ascii::strtod(attr->get_value());
		if ((attr = el2->get_attribute("mp")))
			mp = Glib::Ascii::strtod(attr->get_value());
		add(pa, isaoffs, bhp, rpm, mp);
	}
}

void Aircraft::Cruise::PistonPower::save_xml(xmlpp::Element *el, double altfactor, double tempfactor, double bhpfactor) const
{
	if (!el)
		return;
	if (empty())
		return;
	for (const_iterator i(begin()), e(end()); i != e; ++i)
		i->save_xml(el, altfactor, tempfactor, bhpfactor);
}

void Aircraft::Cruise::PistonPower::set_default(void)
{
	static const double tbl[][4] = {
		{     0, 110, 2100, 22.9 },
		{  1000, 110, 2100, 22.7 },
		{  2000, 110, 2100, 22.4 },
		{  3000, 110, 2100, 22.2 },
		{  4000, 110, 2100, 21.9 },
		{  5000, 110, 2100, 21.7 },
		{  6000, 110, 2100, 21.4 },
		{  7000, 110, 2100, 21.2 },
		{  8000, 110, 2100, 21.0 },
		{  9000, 110, 2100, 20.8 },
		{     0, 110, 2400, 20.4 },
		{  1000, 110, 2400, 20.2 },
		{  2000, 110, 2400, 20.0 },
		{  3000, 110, 2400, 19.8 },
		{  4000, 110, 2400, 19.5 },
		{  5000, 110, 2400, 19.3 },
		{  6000, 110, 2400, 19.1 },
		{  7000, 110, 2400, 18.9 },
		{  8000, 110, 2400, 18.7 },
		{  9000, 110, 2400, 18.5 },
		{ 10000, 110, 2400, 18.3 },
		{ 11000, 110, 2400, 18.1 },
		{ 12000, 110, 2400, 17.8 },
		{ 13000, 110, 2400, 17.6 },
		{ 14000, 110, 2400, 17.4 },
		{     0, 130, 2100, 25.9 },
		{  1000, 130, 2100, 25.6 },
		{  2000, 130, 2100, 25.4 },
		{  3000, 130, 2100, 25.1 },
		{  4000, 130, 2100, 24.8 },
		{  5000, 130, 2100, 24.6 },
		{     0, 130, 2400, 22.9 },
		{  1000, 130, 2400, 22.7 },
		{  2000, 130, 2400, 22.5 },
		{  3000, 130, 2400, 22.2 },
		{  4000, 130, 2400, 22.0 },
		{  5000, 130, 2400, 21.7 },
		{  6000, 130, 2400, 21.5 },
		{  7000, 130, 2400, 21.3 },
		{  8000, 130, 2400, 21.0 },
		{  9000, 130, 2400, 20.8 },
		{     0, 150, 2400, 25.5 },
		{  1000, 150, 2400, 25.2 },
		{  2000, 150, 2400, 25.0 },
		{  3000, 150, 2400, 24.7 },
		{  4000, 150, 2400, 24.4 },
		{  5000, 150, 2400, 24.2 },
		// extrapolated
		{     0,  90, 2400, 17.9 },
		{  1000,  90, 2400, 17.7 },
		{  2000,  90, 2400, 17.5 },
		{  3000,  90, 2400, 17.3 },
		{  4000,  90, 2400, 17.0 },
		{  5000,  90, 2400, 16.8 },
		{  6000,  90, 2400, 16.6 },
		{  7000,  90, 2400, 16.4 },
		{  8000,  90, 2400, 16.2 },
		{  9000,  90, 2400, 16.0 },
		{ 10000,  90, 2400, 15.8 },
		{ 11000,  90, 2400, 15.6 },
		{ 12000,  90, 2400, 15.3 },
		{ 13000,  90, 2400, 15.1 },
		{ 14000,  90, 2400, 14.9 },
		{ 15000,  90, 2400, 14.7 },
		{ 16000,  90, 2400, 14.5 },
		{ 17000,  90, 2400, 14.3 }
	};

	clear();
	for (unsigned int i = 0; i < sizeof(tbl)/sizeof(tbl[0]); ++i)
		for (int t = -2; t <= 2; ++t)
			add(tbl[i][0], t * (50./9.), tbl[i][1], tbl[i][2], tbl[i][3] + t * 0.16);
}

bool Aircraft::Cruise::PistonPower::has_fixedpitch(void) const
{
	for (const_iterator i(begin()), e(end()); i != e; ++i)
		if (i->has_fixedpitch())
			return true;
	return false;
}

bool Aircraft::Cruise::PistonPower::has_variablepitch(void) const
{
	for (const_iterator i(begin()), e(end()); i != e; ++i)
		if (i->has_variablepitch())
			return true;
	return false;
}

Aircraft::Cruise::Cruise(double altfactor, double tasfactor, double fuelfactor, double tempfactor, double bhpfactor)
	: m_altfactor(altfactor), m_tasfactor(tasfactor), m_fuelfactor(fuelfactor), m_tempfactor(tempfactor), m_bhpfactor(bhpfactor)
{
	// PA28R-200
	{
		Curve c("55%, 2100 RPM", Curve::flags_interpolate, std::numeric_limits<double>::quiet_NaN(), 0, 2100);
		c.insert(Point(    0, 110, 131.5 * 0.86897624, 54.0/6));
		c.insert(Point( 1000, 110, 133.0 * 0.86897624, 54.0/6));
		c.insert(Point( 2000, 110, 134.5 * 0.86897624, 54.0/6));
		c.insert(Point( 3000, 110, 136.0 * 0.86897624, 54.0/6));
		c.insert(Point( 4000, 110, 137.5 * 0.86897624, 54.0/6));
		c.insert(Point( 5000, 110, 139.0 * 0.86897624, 54.0/6));
		c.insert(Point( 6000, 110, 140.5 * 0.86897624, 54.0/6));
		c.insert(Point( 7000, 110, 142.0 * 0.86897624, 54.0/6));
		c.insert(Point( 8000, 110, 143.5 * 0.86897624, 54.0/6));
		c.insert(Point( 9000, 110, 145.0 * 0.86897624, 54.0/6));
		add_curve(c);
	}
	{
		Curve c("55%, 2400 RPM", Curve::flags_interpolate, std::numeric_limits<double>::quiet_NaN(), 0, 2400);
		c.insert(Point(    0, 110, 131.5 * 0.86897624, 57.0/6));
		c.insert(Point( 1000, 110, 133.0 * 0.86897624, 57.0/6));
		c.insert(Point( 2000, 110, 134.5 * 0.86897624, 57.0/6));
		c.insert(Point( 3000, 110, 136.0 * 0.86897624, 57.0/6));
		c.insert(Point( 4000, 110, 137.5 * 0.86897624, 57.0/6));
		c.insert(Point( 5000, 110, 139.0 * 0.86897624, 57.0/6));
		c.insert(Point( 6000, 110, 140.5 * 0.86897624, 57.0/6));
		c.insert(Point( 7000, 110, 142.0 * 0.86897624, 57.0/6));
		c.insert(Point( 8000, 110, 143.5 * 0.86897624, 57.0/6));
		c.insert(Point( 9000, 110, 145.0 * 0.86897624, 57.0/6));
		c.insert(Point(10000, 110, 146.5 * 0.86897624, 57.0/6));
		c.insert(Point(11000, 110, 148.0 * 0.86897624, 57.0/6));
		c.insert(Point(12000, 110, 149.5 * 0.86897624, 57.0/6));
		c.insert(Point(13000, 110, 151.0 * 0.86897624, 57.0/6));
		c.insert(Point(14000, 110, 152.5 * 0.86897624, 57.0/6));
		add_curve(c);
	}
	{
		Curve c("65%, 2100 RPM", Curve::flags_interpolate, std::numeric_limits<double>::quiet_NaN(), 0, 2100);
		c.insert(Point(    0, 130, 143.0 * 0.86897624, 61.0/6));
		c.insert(Point( 1000, 130, 144.8 * 0.86897624, 61.0/6));
		c.insert(Point( 2000, 130, 146.6 * 0.86897624, 61.0/6));
		c.insert(Point( 3000, 130, 148.3 * 0.86897624, 61.0/6));
		c.insert(Point( 4000, 130, 150.1 * 0.86897624, 61.0/6));
		c.insert(Point( 5000, 130, 151.9 * 0.86897624, 61.0/6));
		add_curve(c);
	}
	{
		Curve c("65%, 2400 RPM", Curve::flags_interpolate, std::numeric_limits<double>::quiet_NaN(), 0, 2400);
		c.insert(Point(    0, 130, 143.0 * 0.86897624, 64.0/6));
		c.insert(Point( 1000, 130, 144.8 * 0.86897624, 64.0/6));
		c.insert(Point( 2000, 130, 146.6 * 0.86897624, 64.0/6));
		c.insert(Point( 3000, 130, 148.3 * 0.86897624, 64.0/6));
		c.insert(Point( 4000, 130, 150.1 * 0.86897624, 64.0/6));
		c.insert(Point( 5000, 130, 151.9 * 0.86897624, 64.0/6));
		c.insert(Point( 6000, 130, 153.7 * 0.86897624, 64.0/6));
		c.insert(Point( 7000, 130, 155.4 * 0.86897624, 64.0/6));
		c.insert(Point( 8000, 130, 157.2 * 0.86897624, 64.0/6));
		c.insert(Point( 9000, 130, 159.0 * 0.86897624, 64.0/6));
		add_curve(c);
	}
	{
		Curve c("75%, 2400 RPM", Curve::flags_interpolate, std::numeric_limits<double>::quiet_NaN(), 0, 2400);
		c.insert(Point(    0, 150, 153.0 * 0.86897624, 72.0/6));
		c.insert(Point( 1000, 150, 155.0 * 0.86897624, 72.0/6));
		c.insert(Point( 2000, 150, 157.0 * 0.86897624, 72.0/6));
		c.insert(Point( 3000, 150, 159.0 * 0.86897624, 72.0/6));
		c.insert(Point( 4000, 150, 161.0 * 0.86897624, 72.0/6));
		c.insert(Point( 5000, 150, 163.0 * 0.86897624, 72.0/6));
		add_curve(c);
	}
	// Extrapolated
	{
		Curve c("45%, 2400 RPM", Curve::flags_interpolate, std::numeric_limits<double>::quiet_NaN(), 0, 2400);
		c.insert(Point(    0,  90, 120.5 * 0.86897624, 50.0/6));
		c.insert(Point( 1000,  90, 122.0 * 0.86897624, 50.0/6));
		c.insert(Point( 2000,  90, 123.5 * 0.86897624, 50.0/6));
		c.insert(Point( 3000,  90, 125.0 * 0.86897624, 50.0/6));
		c.insert(Point( 4000,  90, 126.5 * 0.86897624, 50.0/6));
		c.insert(Point( 5000,  90, 128.0 * 0.86897624, 50.0/6));
		c.insert(Point( 6000,  90, 129.5 * 0.86897624, 50.0/6));
		c.insert(Point( 7000,  90, 131.0 * 0.86897624, 50.0/6));
		c.insert(Point( 8000,  90, 132.5 * 0.86897624, 50.0/6));
		c.insert(Point( 9000,  90, 134.0 * 0.86897624, 50.0/6));
		c.insert(Point(10000,  90, 135.5 * 0.86897624, 50.0/6));
		c.insert(Point(11000,  90, 137.0 * 0.86897624, 50.0/6));
		c.insert(Point(12000,  90, 138.5 * 0.86897624, 50.0/6));
		c.insert(Point(13000,  90, 140.0 * 0.86897624, 50.0/6));
		c.insert(Point(14000,  90, 141.5 * 0.86897624, 50.0/6));
		c.insert(Point(15000,  90, 143.0 * 0.86897624, 50.0/6));
		c.insert(Point(16000,  90, 144.5 * 0.86897624, 50.0/6));
		c.insert(Point(17000,  90, 145.0 * 0.86897624, 50.0/6));
		add_curve(c);
	}
	m_pistonpower.set_default();
}

void Aircraft::Cruise::set_points(const rpmmppoints_t& pts, double maxbhp)
{
	typedef std::map<int,CurveRPMMP> intcurve_t;
	typedef std::map<int,intcurve_t> intintcurve_t;
	CurveRPMMP c0("Normal");
	intcurve_t c1;
	intcurve_t c2;
	intintcurve_t c3;
	unsigned int missing[4] = { 0, 0, 0, 0 };
	unsigned int nrcurves[4] = { 1, 0, 0, 0 };
	m_curves.clear();
	double bhpmul(100.0 / maxbhp);
	for (rpmmppoints_t::const_iterator pi(pts.begin()), pe(pts.end()); pi != pe; ++pi) {
		m_pistonpower.add(pi->get_pressurealt(), 0, pi->get_bhp(), pi->get_rpm(), pi->get_mp());
		int bhp(-1), rpm(-1);
		{
			double x(bhpmul * pi->get_bhp());
			if (!std::isnan(x))
				bhp = ::Point::round<int,double>(x);
		}
		{
			double x(pi->get_rpm());
			if (!std::isnan(x))
				rpm = ::Point::round<int,double>(x);
		}
		{
			std::pair<CurveRPMMP::iterator,bool> itp(c0.insert(*pi));
			missing[0] += !itp.second;
		}
		if (rpm >= 0) {
			std::pair<intcurve_t::iterator,bool> it(c1.insert(intcurve_t::value_type(rpm, CurveRPMMP(""))));
			CurveRPMMP& c(it.first->second);
			if (it.second) {
				std::ostringstream oss;
				oss << rpm << " RPM";
				c.set_name(oss.str());
				++nrcurves[1];
			}
			std::pair<CurveRPMMP::iterator,bool> itp(c.insert(*pi));
			missing[1] += !itp.second;
		} else {
			++missing[1];
		}
		if (bhp >= 0) {
			std::pair<intcurve_t::iterator,bool> it(c2.insert(intcurve_t::value_type(bhp, CurveRPMMP(""))));
			CurveRPMMP& c(it.first->second);
			if (it.second) {
				std::ostringstream oss;
				oss << bhp << '%';
				c.set_name(oss.str());
				++nrcurves[2];
			}
			std::pair<CurveRPMMP::iterator,bool> itp(c.insert(*pi));
			missing[2] += !itp.second;
		} else {
			++missing[2];
		}
		if (rpm >= 0 && bhp >= 0) {
			std::pair<intintcurve_t::iterator,bool> it1(c3.insert(intintcurve_t::value_type(rpm, intcurve_t())));
			std::pair<intcurve_t::iterator,bool> it2(it1.first->second.insert(intcurve_t::value_type(bhp, CurveRPMMP(""))));
			CurveRPMMP& c(it2.first->second);
			if (it2.second) {
				std::ostringstream oss;
				oss << bhp << "%, " << rpm << " RPM";
				c.set_name(oss.str());
				++nrcurves[3];
			}
			std::pair<CurveRPMMP::iterator,bool> itp(c.insert(*pi));
			missing[3] += !itp.second;
		} else {
			++missing[3];
		}
	}
	unsigned int bidx(~0U);
	for (unsigned int i = 0; i < 4; ++i) {
		if (missing[i])
			continue;
		if (bidx >= 4) {
			bidx = i;
			continue;
		}
		if (nrcurves[i] >= nrcurves[bidx])
			continue;
		bidx = i;
	}
	if (bidx >= 4U) {
		bidx = 0;
		for (unsigned int i = 1; i < 4; ++i) {
			if (missing[i] > missing[bidx])
				continue;
			if (missing[i] == missing[bidx] && nrcurves[i] >= nrcurves[bidx])
				continue;
			bidx = i;
		}
	}
	switch (bidx) {
	default:
	case 0:
		add_curve(c0);
		break;

	case 1:
		for (intcurve_t::const_iterator ci(c1.begin()), ce(c1.end()); ci != ce; ++ci)
			add_curve(ci->second);
		break;

	case 2:
		for (intcurve_t::const_iterator ci(c2.begin()), ce(c2.end()); ci != ce; ++ci)
			add_curve(ci->second);
		break;

	case 3:
		for (intintcurve_t::const_iterator xi(c3.begin()), xe(c3.end()); xi != xe; ++xi)
			for (intcurve_t::const_iterator ci(xi->second.begin()), ce(xi->second.end()); ci != ce; ++ci)
				add_curve(ci->second);
		break;
	}
}

void Aircraft::Cruise::load_xml(const xmlpp::Element *el, double maxbhp)
{
	if (!el)
		return;
	m_remark.clear();
	m_altfactor = 1;
	m_tasfactor = 1;
	m_fuelfactor = 1;
	clear_curves();
	m_pistonpower.set_default();
	xmlpp::Attribute *attr;
	if ((attr = el->get_attribute("altfactor")))
		m_altfactor = Glib::Ascii::strtod(attr->get_value());
	if ((attr = el->get_attribute("tasfactor")))
		m_tasfactor = Glib::Ascii::strtod(attr->get_value());
	if ((attr = el->get_attribute("fuelfactor")))
		m_fuelfactor = Glib::Ascii::strtod(attr->get_value());
	if ((attr = el->get_attribute("tempfactor")))
		m_tempfactor = Glib::Ascii::strtod(attr->get_value());
	if ((attr = el->get_attribute("bhpfactor")))
		m_bhpfactor = Glib::Ascii::strtod(attr->get_value());
	if ((attr = el->get_attribute("remark")))
		m_remark = attr->get_value();
	{
		PistonPower pp;
		xmlpp::Node::NodeList nl(el->get_children("curve"));
		for (xmlpp::Node::NodeList::const_iterator ni(nl.begin()), ne(nl.end()); ni != ne; ++ni) {
			Curve c;
			c.load_xml(static_cast<xmlpp::Element *>(*ni), pp, m_altfactor, m_tasfactor, m_fuelfactor);
			add_curve(c);
		}
		if (m_curves.empty()) {
			rpmmppoints_t pts;
			xmlpp::Node::NodeList nl(el->get_children("point"));
			for (xmlpp::Node::NodeList::const_iterator ni(nl.begin()), ne(nl.end()); ni != ne; ++ni) {
				PointRPMMP pt;
				pt.load_xml(static_cast<xmlpp::Element *>(*ni), pp, m_altfactor, m_tasfactor, m_fuelfactor);
				if (std::isnan(pt.get_pressurealt()))
					continue;
				pts.push_back(pt);
			}
			set_points(pts, maxbhp);
		}
		if (!pp.empty())
			m_pistonpower.swap(pp);
	}
	{
		xmlpp::Node::NodeList nl(el->get_children("PistonPower"));
		if (!nl.empty()) {
			PistonPower pp;
			for (xmlpp::Node::NodeList::const_iterator ni(nl.begin()), ne(nl.end()); ni != ne; ++ni) {
				const xmlpp::Element *el2(static_cast<xmlpp::Element *>(*ni));
				pp.load_xml(el2, m_altfactor, m_tempfactor, m_bhpfactor);
			}
			m_pistonpower.swap(pp);
		}
	}
}

void Aircraft::Cruise::save_xml(xmlpp::Element *el, double maxbhp) const
{
	if (!el)
		return;
	if (m_altfactor == 1) {
		el->remove_attribute("altfactor");
	} else {
		std::ostringstream oss;
		oss << m_altfactor;
		el->set_attribute("altfactor", oss.str());
	}
	if (m_tasfactor == 1) {
		el->remove_attribute("tasfactor");
	} else {
		std::ostringstream oss;
		oss << m_tasfactor;
		el->set_attribute("tasfactor", oss.str());
	}
	if (m_fuelfactor == 1) {
		el->remove_attribute("fuelfactor");
	} else {
		std::ostringstream oss;
		oss << m_fuelfactor;
		el->set_attribute("fuelfactor", oss.str());
	}
	if (m_tempfactor == 1) {
		el->remove_attribute("tempfactor");
	} else {
		std::ostringstream oss;
		oss << m_tempfactor;
		el->set_attribute("tempfactor", oss.str());
	}
	if (m_bhpfactor == 1) {
		el->remove_attribute("bhpfactor");
	} else {
		std::ostringstream oss;
		oss << m_bhpfactor;
		el->set_attribute("bhpfactor", oss.str());
	}
	if (m_remark.empty())
		el->remove_attribute("remark");
	else
		el->set_attribute("remark", m_remark);
	for (curves_t::const_iterator ci(m_curves.begin()), ce(m_curves.end()); ci != ce; ++ci)
		for (massmap_t::const_iterator mi(ci->second.begin()), me(ci->second.end()); mi != me; ++mi)
			for (isamap_t::const_iterator ii(mi->second.begin()), ie(mi->second.end()); ii != ie; ++ii)
				ii->second.save_xml(el->add_child("curve"), m_altfactor, m_tasfactor, m_fuelfactor);
	m_pistonpower.save_xml(el->add_child("PistonPower"), m_altfactor, m_tempfactor, m_bhpfactor);
}

void Aircraft::Cruise::add_curve(const Curve& c)
{
	double m(c.get_mass());
	if (std::isnan(m) || m <= 0)
		m = 0;
	m_curves[c.get_name()][m][c.get_isaoffset()] = c;
}

void Aircraft::Cruise::clear_curves(void)
{
	m_curves.clear();
}

unsigned int Aircraft::Cruise::count_curves(void) const
{
	unsigned int ret(0);
	for (curves_t::const_iterator ci(m_curves.begin()), ce(m_curves.end()); ci != ce; ++ci)
		for (massmap_t::const_iterator mi(ci->second.begin()), me(ci->second.end()); mi != me; ++mi)
			ret += mi->second.size();
	return ret;
}

bool Aircraft::Cruise::has_hold(void) const
{
	for (curves_t::const_iterator ci(m_curves.begin()), ce(m_curves.end()); ci != ce; ++ci)
		for (massmap_t::const_iterator mi(ci->second.begin()), me(ci->second.end()); mi != me; ++mi)
			for (isamap_t::const_iterator ii(mi->second.begin()), ie(mi->second.end()); ii != ie; ++ii)
				if (ii->second.get_flags() & Curve::flags_hold)
					return true;
	return false;
}

std::set<std::string> Aircraft::Cruise::get_curves(void) const
{
	std::set<std::string> r;
	for (curves_t::const_iterator i(m_curves.begin()), e(m_curves.end()); i != e; ++i)
		r.insert(i->first);
	return r;
}

Aircraft::Cruise::Curve::flags_t Aircraft::Cruise::get_curve_flags(const std::string& name) const
{
	Curve::flags_t r(Curve::flags_none);
	curves_t::const_iterator ci(m_curves.find(name));
	if (ci == m_curves.end())
		return Curve::flags_none;
	for (massmap_t::const_iterator mi(ci->second.begin()), me(ci->second.end()); mi != me; ++mi)
		for (isamap_t::const_iterator ii(mi->second.begin()), ie(mi->second.end()); ii != ie; ++ii)
			r |= ii->second.get_flags();
	return r;
}

void Aircraft::Cruise::build_altmap(propulsion_t prop)
{
	if (prop != propulsion_fixedpitch && prop != propulsion_constantspeed)
		m_pistonpower.clear();
	m_bhpmap.clear();
	for (curves_t::const_iterator ci(m_curves.begin()), ce(m_curves.end()); ci != ce; ++ci) {
		for (massmap_t::const_iterator mi(ci->second.begin()), me(ci->second.end()); mi != me; ++mi) {
			for (isamap_t::const_iterator ii(mi->second.begin()), ie(mi->second.end()); ii != ie; ++ii) {
				if (!(ii->second.get_flags() & Curve::flags_interpolate))
					continue;
				for (Curve::const_iterator pi(ii->second.begin()), pe(ii->second.end()); pi != pe; ++pi) {
					if (std::isnan(pi->get_pressurealt()) || std::isnan(pi->get_bhp()))
						continue;
					m_bhpmap[ii->second.get_mass()][ii->second.get_isaoffset()][pi->get_pressurealt()][pi->get_bhp()] = Point1(pi->get_tas(), pi->get_fuelflow());
				}
			}
		}
	}	
	if (false) {
		for (bhpmap_t::const_iterator i(m_bhpmap.begin()), e(m_bhpmap.end()); i != e; ++i) {
			std::cout << "Mass = " << i->first << std::endl;
			for (bhpisamap_t::const_iterator i2(i->second.begin()), e2(i->second.end()); i2 != e2; ++i2) {
				std::cout << "  ISA = " << i2->first << std::endl;
				for (bhpaltmap_t::const_iterator i3(i2->second.begin()), e3(i2->second.end()); i3 != e3; ++i3) {
					std::cout << "    PA = " << i3->first;
					for (bhpbhpmap_t::const_iterator i4(i3->second.begin()), e4(i3->second.end()); i4 != e4; ++i4) {
						std::cout << "    BHP = " << i4->first << " TAS = " << i4->second.get_tas()
							  << " FF = " << i4->second.get_fuelflow() << std::endl;
					}
				}
			}
		}
	}
}

void Aircraft::Cruise::calculate(double& mass, double& isaoffs, double& pa, double& bhp, double& tas, double& ff) const
{
	if (std::isnan(mass)) {
		mass = isaoffs = pa = bhp = tas = ff = std::numeric_limits<double>::quiet_NaN();
		return;
	}
	bhpmap_t::const_iterator iu(m_bhpmap.lower_bound(mass));
	if (iu == m_bhpmap.end()) {
		if (iu == m_bhpmap.begin()) {
			mass = isaoffs = pa = bhp = tas = ff = std::numeric_limits<double>::quiet_NaN();
			return;
		}
		--iu;
		mass = iu->first;
		calculate(iu, isaoffs, pa, bhp, tas, ff);
		return;
	}
	bhpmap_t::const_iterator il(iu);
	if (iu == m_bhpmap.begin()) {
		++iu;
		if (iu == m_bhpmap.end()) {
			mass = il->first;
			calculate(il, isaoffs, pa, bhp, tas, ff);
			return;
		}
	} else {
		--il;
	}
	double t((mass - il->first) / (iu->first - il->first));
	if (t <= 0) {
		calculate(il, isaoffs, pa, bhp, tas, ff);
		return;
	}
	if (t >= 1) {
		calculate(iu, isaoffs, pa, bhp, tas, ff);
		return;
	}
	double lisaoffs(isaoffs), lpa(pa), lbhp(bhp), ltas(tas), lff(ff);
	calculate(il, lisaoffs, lpa, lbhp, ltas, lff);
	double uisaoffs(isaoffs), upa(pa), ubhp(bhp), utas(tas), uff(ff);
	calculate(iu, uisaoffs, upa, ubhp, utas, uff);
	isaoffs = uisaoffs * t + lisaoffs * (1 - t);
	pa = upa * t + lpa * (1 - t);
	bhp = ubhp * t + lbhp * (1 - t);
	tas = utas * t + ltas * (1 - t);
	ff = uff * t + lff * (1 - t);
}

void Aircraft::Cruise::calculate(bhpmap_t::const_iterator it, double& isaoffs, double& pa, double& bhp, double& tas, double& ff) const
{
	if (std::isnan(isaoffs)) {
		isaoffs = pa = bhp = tas = ff = std::numeric_limits<double>::quiet_NaN();
		return;
	}
	bhpisamap_t::const_iterator iu(it->second.lower_bound(isaoffs));
	if (iu == it->second.end()) {
		if (iu == it->second.begin()) {
			isaoffs = pa = bhp = tas = ff = std::numeric_limits<double>::quiet_NaN();
			return;
		}
		--iu;
		isaoffs = iu->first;
		calculate(iu, pa, bhp, tas, ff);
		return;
	}
	bhpisamap_t::const_iterator il(iu);
	if (iu == it->second.begin()) {
		++iu;
		if (iu == it->second.end()) {
			isaoffs = il->first;
			calculate(il, pa, bhp, tas, ff);
			return;
		}
	} else {
		--il;
	}
	double t((isaoffs - il->first) / (iu->first - il->first));
	if (t <= 0) {
		calculate(il, pa, bhp, tas, ff);
		return;
	}
	if (t >= 1) {
		calculate(iu, pa, bhp, tas, ff);
		return;
	}
	double lpa(pa), lbhp(bhp), ltas(tas), lff(ff);
	calculate(il, lpa, lbhp, ltas, lff);
	double upa(pa), ubhp(bhp), utas(tas), uff(ff);
	calculate(iu, upa, ubhp, utas, uff);
	pa = upa * t + lpa * (1 - t);
	bhp = ubhp * t + lbhp * (1 - t);
	tas = utas * t + ltas * (1 - t);
	ff = uff * t + lff * (1 - t);
}

void Aircraft::Cruise::calculate(bhpisamap_t::const_iterator it, double& pa, double& bhp, double& tas, double& ff) const
{
	if (std::isnan(pa)) {
		pa = bhp = tas = ff = std::numeric_limits<double>::quiet_NaN();
		return;
	}
	bhpaltmap_t::const_iterator iu(it->second.lower_bound(pa));
	if (iu == it->second.end()) {
		if (iu == it->second.begin()) {
			pa = bhp = tas = ff = std::numeric_limits<double>::quiet_NaN();
			return;
		}
		--iu;
		pa = iu->first;
		calculate(iu, bhp, tas, ff);
		return;
	}
	bhpaltmap_t::const_iterator il(iu);
	if (iu == it->second.begin()) {
		++iu;
		if (iu == it->second.end()) {
			pa = il->first;
			calculate(il, bhp, tas, ff);
			return;
		}
	} else {
		--il;
	}
	double t((pa - il->first) / (iu->first - il->first));
	if (t <= 0) {
		calculate(il, bhp, tas, ff);
		return;
	}
	if (t >= 1) {
		calculate(iu, bhp, tas, ff);
		return;
	}
	double lbhp(bhp), ltas(tas), lff(ff);
	calculate(il, lbhp, ltas, lff);
	double ubhp(bhp), utas(tas), uff(ff);
	calculate(iu, ubhp, utas, uff);
	bhp = ubhp * t + lbhp * (1 - t);
	tas = utas * t + ltas * (1 - t);
	ff = uff * t + lff * (1 - t);
}

void Aircraft::Cruise::calculate(bhpaltmap_t::const_iterator it, double& bhp, double& tas, double& ff) const
{
	if (std::isnan(bhp)) {
		bhp = tas = ff = std::numeric_limits<double>::quiet_NaN();
		return;
	}
	bhpbhpmap_t::const_iterator iu(it->second.lower_bound(bhp));
	if (iu == it->second.end()) {
		if (iu == it->second.begin()) {
			bhp = tas = ff = std::numeric_limits<double>::quiet_NaN();
			return;
		}
		--iu;
		bhp = iu->first;
		tas = iu->second.get_tas();
		ff = iu->second.get_fuelflow();
		return;
	}
	bhpbhpmap_t::const_iterator il(iu);
	if (iu == it->second.begin()) {
		++iu;
		if (iu == it->second.end()) {
			bhp = il->first;
			tas = il->second.get_tas();
			ff = il->second.get_fuelflow();
			return;
		}
	} else {
		--il;
	}
	double t((bhp - il->first) / (iu->first - il->first));
	if (t <= 0) {
		tas = il->second.get_tas();
		ff = il->second.get_fuelflow();
		return;
	}
	if (t >= 1) {
		tas = iu->second.get_tas();
		ff = iu->second.get_fuelflow();
		return;
	}
	tas = iu->second.get_tas() * t + il->second.get_tas() * (1 - t);
	ff = iu->second.get_fuelflow() * t + il->second.get_fuelflow() * (1 - t);
}

Aircraft::Cruise::Curve::flags_t Aircraft::Cruise::calculate(curves_t::const_iterator it, double& tas, double& ff, double& pa, double& mass, double& isaoffs, CruiseEngineParams& ep) const
{
	static const bool debug(false);
	ep.set_name(it->first);
	if (std::isnan(mass)) {
		mass = isaoffs = pa = tas = ff = std::numeric_limits<double>::quiet_NaN();
		ep.set_bhp(std::numeric_limits<double>::quiet_NaN());
		if (debug)
			std::cerr << "Cruise::calculate: name " << ep.get_name() << " mass " << mass << " isaoffs " << isaoffs
				  << " pa " << pa << " tas " << tas << " ff " << ff << " bhp " << ep.get_bhp() << std::endl;
		return Curve::flags_none;
	}
	massmap_t::const_iterator iu(it->second.lower_bound(mass));
	if (iu == it->second.end()) {
		if (iu == it->second.begin()) {
			mass = isaoffs = pa = tas = ff = std::numeric_limits<double>::quiet_NaN();
			ep.set_bhp(std::numeric_limits<double>::quiet_NaN());
			if (debug)
				std::cerr << "Cruise::calculate: name " << ep.get_name() << " mass " << mass << " isaoffs " << isaoffs
					  << " pa " << pa << " tas " << tas << " ff " << ff << " bhp " << ep.get_bhp() << std::endl;
			return Curve::flags_none;
		}
		--iu;
		mass = iu->first;
		return calculate(iu, tas, ff, pa, isaoffs, ep);
	}
	massmap_t::const_iterator il(iu);
	if (iu == it->second.begin()) {
		++iu;
		if (iu == it->second.end()) {
			mass = il->first;
			return calculate(il, tas, ff, pa, isaoffs, ep);
		}
	} else {
		--il;
	}
	double t((mass - il->first) / (iu->first - il->first));
	if (t <= 0)
		return calculate(il, tas, ff, pa, isaoffs, ep);
	if (t >= 1)
		return calculate(iu, tas, ff, pa, isaoffs, ep);
	CruiseEngineParams lep(ep);
	double lisaoffs(isaoffs), lpa(pa), ltas(tas), lff(ff);
	Curve::flags_t lflags(calculate(il, ltas, lff, lpa, lisaoffs, lep));
	CruiseEngineParams uep(ep);
	double uisaoffs(isaoffs), upa(pa), utas(tas), uff(ff);
	Curve::flags_t uflags(calculate(iu, utas, uff, upa, uisaoffs, uep));
	isaoffs = uisaoffs * t + lisaoffs * (1 - t);
	pa = upa * t + lpa * (1 - t);
	ep.set_bhp(uep.get_bhp() * t + lep.get_bhp() * (1 - t));
	ep.set_rpm(uep.get_rpm() * t + lep.get_rpm() * (1 - t));
	tas = utas * t + ltas * (1 - t);
	ff = uff * t + lff * (1 - t);
	if (debug)
		std::cerr << "Cruise::calculate: name " << ep.get_name() << " mass " << mass << " isaoffs " << isaoffs
			  << " pa " << pa << " tas " << tas << " ff " << ff << " bhp " << ep.get_bhp() << " t " << t << std::endl;
	return lflags | uflags;	
}

Aircraft::Cruise::Curve::flags_t Aircraft::Cruise::calculate(massmap_t::const_iterator it, double& tas, double& ff, double& pa, double& isaoffs, CruiseEngineParams& ep) const
{
	static const bool debug(false);
	if (std::isnan(isaoffs)) {
		isaoffs = pa = tas = ff = std::numeric_limits<double>::quiet_NaN();
		ep.set_bhp(std::numeric_limits<double>::quiet_NaN());
		if (debug)
			std::cerr << "Cruise::calculate: name " << ep.get_name() << " mass " << it->first << " isaoffs " << isaoffs
				  << " pa " << pa << " tas " << tas << " ff " << ff << " bhp " << ep.get_bhp() << " (1)" << std::endl;
		return Curve::flags_none;
	}
	isamap_t::const_iterator iu(it->second.lower_bound(isaoffs));
	if (iu == it->second.end()) {
		if (iu == it->second.begin()) {
			isaoffs = pa = tas = ff = std::numeric_limits<double>::quiet_NaN();
			ep.set_bhp(std::numeric_limits<double>::quiet_NaN());
			if (debug)
				std::cerr << "Cruise::calculate: name " << ep.get_name() << " mass " << it->first << " isaoffs " << isaoffs
					  << " pa " << pa << " tas " << tas << " ff " << ff << " bhp " << ep.get_bhp() << " (2)" << std::endl;
			return Curve::flags_none;
		}
		--iu;
		isaoffs = iu->first;
		iu->second.calculate(tas, ff, pa, ep);
		if (debug)
			std::cerr << "Cruise::calculate: name " << ep.get_name() << " mass " << it->first << " isaoffs " << isaoffs
				  << " pa " << pa << " tas " << tas << " ff " << ff << " bhp " << ep.get_bhp() << " (3)" << std::endl;
		return iu->second.get_flags();
	}
	isamap_t::const_iterator il(iu);
	if (iu == it->second.begin()) {
		++iu;
		if (iu == it->second.end()) {
			isaoffs = il->first;
			il->second.calculate(tas, ff, pa, ep);
			if (debug)
				std::cerr << "Cruise::calculate: name " << ep.get_name() << " mass " << it->first << " isaoffs " << isaoffs
					  << " pa " << pa << " tas " << tas << " ff " << ff << " bhp " << ep.get_bhp() << " (4)" << std::endl;
			return il->second.get_flags();
		}
	} else {
		--il;
	}
	double t((isaoffs - il->first) / (iu->first - il->first));
	if (t <= 0) {
		il->second.calculate(tas, ff, pa, ep);
		if (debug)
			std::cerr << "Cruise::calculate: name " << ep.get_name() << " mass " << it->first << " isaoffs " << isaoffs
				  << " pa " << pa << " tas " << tas << " ff " << ff << " bhp " << ep.get_bhp() << " t " << t << std::endl;
		return il->second.get_flags();
	}
	if (t >= 1) {
		iu->second.calculate(tas, ff, pa, ep);
		if (debug)
			std::cerr << "Cruise::calculate: name " << ep.get_name() << " mass " << it->first << " isaoffs " << isaoffs
				  << " pa " << pa << " tas " << tas << " ff " << ff << " bhp " << ep.get_bhp() << " t " << t << std::endl;
		return iu->second.get_flags();
	}
	CruiseEngineParams lep(ep);
	double lpa(pa), ltas(tas), lff(ff);
	il->second.calculate(ltas, lff, lpa, lep);
	CruiseEngineParams uep(ep);
	double upa(pa), utas(tas), uff(ff);
	iu->second.calculate(utas, uff, upa, uep);
	pa = upa * t + lpa * (1 - t);
	ep.set_bhp(uep.get_bhp() * t + lep.get_bhp() * (1 - t));
	ep.set_rpm(uep.get_rpm() * t + lep.get_rpm() * (1 - t));
	tas = utas * t + ltas * (1 - t);
	ff = uff * t + lff * (1 - t);
	if (debug)
		std::cerr << "Cruise::calculate: name " << ep.get_name() << " mass " << it->first << " isaoffs " << isaoffs
			  << " pa " << pa << " tas " << tas << " ff " << ff << " bhp " << ep.get_bhp() << " t " << t << std::endl;
	return il->second.get_flags() | iu->second.get_flags();
}

std::pair<double,double> Aircraft::Cruise::get_bhp_range(curves_t::const_iterator it) const
{
	std::pair<double,double> r(std::numeric_limits<double>::max(), -std::numeric_limits<double>::max());
	for (massmap_t::const_iterator i(it->second.begin()), e(it->second.end()); i != e; ++i) {
		std::pair<double,double> r1(get_bhp_range(i));
		if (std::isnan(r1.first) || std::isnan(r1.second))
			return std::pair<double,double>(std::numeric_limits<double>::quiet_NaN(),
							std::numeric_limits<double>::quiet_NaN());
		r.first = std::min(r.first, r1.first);
		r.second = std::max(r.second, r1.second);
	}
	if (r.first > r.second)
		return std::pair<double,double>(std::numeric_limits<double>::quiet_NaN(),
						std::numeric_limits<double>::quiet_NaN());
	return r;
}

std::pair<double,double> Aircraft::Cruise::get_bhp_range(massmap_t::const_iterator it) const
{
	std::pair<double,double> r(std::numeric_limits<double>::max(), -std::numeric_limits<double>::max());
	for (isamap_t::const_iterator i(it->second.begin()), e(it->second.end()); i != e; ++i) {
		std::pair<double,double> r1(i->second.get_bhp_range());
		if (std::isnan(r1.first) || std::isnan(r1.second))
			return std::pair<double,double>(std::numeric_limits<double>::quiet_NaN(),
							std::numeric_limits<double>::quiet_NaN());
		r.first = std::min(r.first, r1.first);
		r.second = std::max(r.second, r1.second);
	}
	if (r.first > r.second)
		return std::pair<double,double>(std::numeric_limits<double>::quiet_NaN(),
						std::numeric_limits<double>::quiet_NaN());
	return r;
}

void Aircraft::Cruise::calculate(propulsion_t prop, double& tas, double& fuelflow, double& pa, double& mass, double& isaoffs, CruiseEngineParams& ep) const
{
	if (std::isnan(pa) || std::isnan(mass) || std::isnan(isaoffs)) {
		tas = fuelflow = pa = mass = isaoffs = std::numeric_limits<double>::quiet_NaN();
		return;
	}
	{
		double bhp(ep.get_bhp()), rpm(ep.get_rpm()), mp(ep.get_mp());
		if (std::isnan(bhp) && !std::isnan(rpm))
			m_pistonpower.calculate(pa, isaoffs, bhp, rpm, mp);
		if (!std::isnan(bhp) && !m_bhpmap.empty()) {
			calculate(mass, isaoffs, pa, bhp, tas, fuelflow);
			double pa1(pa), isaoffs1(isaoffs), bhp1(bhp);
			m_pistonpower.calculate(pa1, isaoffs1, bhp1, rpm, mp);
			ep.set_bhp(bhp);
			ep.set_rpm(rpm);
			ep.set_mp(mp);
			return;
		}
	}
	double pax(pa);
	double tas1(std::numeric_limits<double>::quiet_NaN());
	double fuelflow1(std::numeric_limits<double>::quiet_NaN());
	double pa1(std::numeric_limits<double>::quiet_NaN());
	double mass1(std::numeric_limits<double>::quiet_NaN());
	double isaoffs1(std::numeric_limits<double>::quiet_NaN());
	CruiseEngineParams ep1(ep);
	ep1.set_bhp(std::numeric_limits<double>::quiet_NaN());
	ep1.set_rpm(std::numeric_limits<double>::quiet_NaN());
	ep1.set_mp(std::numeric_limits<double>::quiet_NaN());
	if (ep.get_flags() & Curve::flags_hold) {
		bool ishold(false);
		for (curves_t::const_iterator ci(m_curves.begin()), ce(m_curves.end()); ci != ce; ++ci) {
			double tas2, fuelflow2, pa2(pax), mass2(mass), isaoffs2(isaoffs);
			CruiseEngineParams ep2(ep);
			Curve::flags_t flags(calculate(ci, tas2, fuelflow2, pa2, mass2, isaoffs2, ep2));
			if (std::isnan(pa2))
				continue;
			if (false)
				ep2.print(std::cerr << "holdcalc: " << ci->first << ' ') << " mass " << mass2
						    << " isaoffs " << isaoffs2 << " pa " << pa2
						    << " tas " << tas2 << " ff " << fuelflow2 << std::endl;
			if (((flags & Curve::flags_hold) && !ishold) ||
			    std::isnan(pa1) || (fabs(pa1 - pax) > 1 && fabs(pa2 - pax) < fabs(pa1 - pax)) ||
			    (fabs(pa2 - pax) <= 1 && fuelflow2 < fuelflow1)) {
				tas1 = tas2;
				fuelflow1 = fuelflow2;
				pa1 = pa2;
				mass1 = mass2;
				isaoffs1 = isaoffs2;
				ep1 = ep2;
				ishold = !!(flags & Curve::flags_hold);
			}
		}
		tas = tas1;
		fuelflow = fuelflow1;
		pa = pa1;
		mass = mass1;
		double isaoffspp(isaoffs);
		isaoffs = isaoffs1;
		{
			double bhp1(ep1.get_bhp()), rpm1(ep1.get_rpm()), mp1(ep1.get_mp());
			if (std::isnan(rpm1))
				rpm1 = ep.get_rpm();
			if (std::isnan(mp1))
				mp1 = ep.get_mp();
			m_pistonpower.calculate(pa1, isaoffspp, bhp1, rpm1, mp1);
			ep1.set_rpm(rpm1);
			ep1.set_mp(mp1);
		}
		ep = ep1;
		return;
	}
	double cbhp(std::numeric_limits<double>::quiet_NaN());
	{
		double tas2, fuelflow2, pa2, mass2, isaoffs2;
		CruiseEngineParams ep2;
		curves_t::const_iterator ci(m_curves.begin()), ce(m_curves.end());
		for (; ci != ce; ++ci) {
			if (ci->first != ep1.get_name())
				continue;
			pa2 = pax;
			mass2 = mass;
			isaoffs2 = isaoffs;
			ep2 = ep;
			if (calculate(ci, tas2, fuelflow2, pa2, mass2, isaoffs2, ep2) & Curve::flags_hold)
				continue;
			break;
		}
		if (ci != ce) {
			if (!std::isnan(pa2) && fabs(pa2 - pax) < 1) {
				tas = tas2;
				fuelflow = fuelflow2;
				pa = pa2;
				mass = mass2;
				double isaoffspp(isaoffs);
				isaoffs = isaoffs2;
				{
					double bhp1(ep2.get_bhp()), rpm1(ep2.get_rpm()), mp1(ep2.get_mp());
					if (std::isnan(rpm1))
						rpm1 = ep.get_rpm();
					if (std::isnan(mp1))
						mp1 = ep.get_mp();
					m_pistonpower.calculate(pa2, isaoffspp, bhp1, rpm1, mp1);
					ep2.set_rpm(rpm1);
					ep2.set_mp(mp1);
				}
				ep = ep2;
				return;
			}
			std::pair<double,double> br(get_bhp_range(ci));
			if (!std::isnan(br.first) && br.first > 0 && br.second <= 1.01 * br.first)
				cbhp = 0.5 * (br.first + br.second);
		}
	}
	for (curves_t::const_iterator ci(m_curves.begin()), ce(m_curves.end()); ci != ce; ++ci) {
		double tas2, fuelflow2, pa2(pax), mass2(mass), isaoffs2(isaoffs);
		CruiseEngineParams ep2(ep);
		if (calculate(ci, tas2, fuelflow2, pa2, mass2, isaoffs2, ep2) & Curve::flags_hold)
			continue;
		if (std::isnan(pa2))
			continue;
		if (std::isnan(pa1) || (fabs(pa1 - pax) > 1 && fabs(pa2 - pax) < fabs(pa1 - pax)) ||
		    (!std::isnan(cbhp) && fabs(pa2 - pax) <= 1 && fabs(ep1.get_bhp() - cbhp) > cbhp * 0.01 &&
		     fabs(ep2.get_bhp() - cbhp) < fabs(ep1.get_bhp() - cbhp))) {
			tas1 = tas2;
			fuelflow1 = fuelflow2;
			pa1 = pa2;
			mass1 = mass2;
			isaoffs1 = isaoffs2;
			ep1 = ep2;
		}
	}
	tas = tas1;
	fuelflow = fuelflow1;
	pa = pa1;
	mass = mass1;
	double isaoffspp(isaoffs);
	isaoffs = isaoffs1;
	{
		double bhp1(ep1.get_bhp()), rpm1(ep1.get_rpm()), mp1(ep1.get_mp());
		if (std::isnan(rpm1))
			rpm1 = ep.get_rpm();
		if (std::isnan(mp1))
			mp1 = ep.get_mp();
		m_pistonpower.calculate(pa1, isaoffspp, bhp1, rpm1, mp1);
		ep1.set_rpm(rpm1);
		ep1.set_mp(mp1);
	}
	ep = ep1;
}

void Aircraft::Cruise::set_mass(double mass)
{
	if (std::isnan(mass) || mass <= 0)
		return;
	for (curves_t::iterator ci(m_curves.begin()), ce(m_curves.end()); ci != ce; ++ci) {
		massmap_t::iterator mi(ci->second.find(0));
		if (mi == ci->second.end())
			continue;
		for (isamap_t::const_iterator ii(mi->second.begin()), ie(mi->second.end()); ii != ie; ++ii) {
			Curve c(ii->second);
			c.set_mass(mass);
			add_curve(c);
		}
		ci->second.erase(mi);
	}
}

Aircraft::CheckErrors Aircraft::Cruise::check(double minmass, double maxmass) const
{
	CheckErrors r;
	typedef std::map<double,double> cmassmap_t;
	typedef std::map<double,cmassmap_t> cisamap_t;
	cisamap_t ceilingsg;
	bool hascurve(false);
	for (curves_t::const_iterator ci(m_curves.begin()), ce(m_curves.end()); ci != ce; ++ci) {
		bool hold(false);
		cisamap_t ceilings;
		for (massmap_t::const_iterator mi(ci->second.begin()), me(ci->second.end()); mi != me; ++mi)
			for (isamap_t::const_iterator ii(mi->second.begin()), ie(mi->second.end()); ii != ie; ++ii) {
				r.add(ii->second.check(minmass, maxmass));
				if (ii->second.get_flags() & Curve::flags_hold) {
					hold = true;
					continue;
				}
				std::pair<cmassmap_t::iterator, bool> ins(((ii->second.get_flags() & Curve::flags_interpolate) ? ceilingsg : ceilings)[ii->second.get_isaoffset()].
									 insert(cmassmap_t::value_type(ii->second.get_mass(), ii->second.rbegin()->get_pressurealt())));
				if (!ins.second)
					ins.first->second = std::max(ins.first->second, ii->second.rbegin()->get_pressurealt());
			}
		if (hold)
			continue;
		hascurve = hascurve || !ceilings.empty();
		for (cisamap_t::const_iterator ii(ceilings.begin()), ie(ceilings.end()); ii != ie; ++ii) {
			for (cmassmap_t::const_iterator mi(ii->second.begin()), me(ii->second.end()); mi != me; ) {
				if (mi->second < 7000)
					r.add(CheckError::type_cruise, CheckError::severity_warning, ci->first, mi->first, ii->first)
						<< "curve has unrealistically low ceiling";
				cmassmap_t::const_iterator mi2(mi);
				++mi;
				if (mi == me)
					break;
				if (mi2->second >= mi->second)
					continue;
				r.add(CheckError::type_cruise, CheckError::severity_warning, ci->first, mi2->first, ii->first)
					<< "curve has lower ceiling than higher mass " << mi->first;
			}
		}
	}
	hascurve = hascurve || !ceilingsg.empty();
	for (cisamap_t::const_iterator ii(ceilingsg.begin()), ie(ceilingsg.end()); ii != ie; ++ii) {
		for (cmassmap_t::const_iterator mi(ii->second.begin()), me(ii->second.end()); mi != me; ) {
			if (mi->second < 7000)
				r.add(CheckError::type_cruise, CheckError::severity_warning, "*interpolated*", mi->first, ii->first)
					<< "curve has unrealistically low ceiling";
			cmassmap_t::const_iterator mi2(mi);
			++mi;
			if (mi == me)
				break;
			if (mi2->second >= mi->second)
				continue;
			r.add(CheckError::type_cruise, CheckError::severity_warning, "*interpolated*", mi2->first, ii->first)
				<< "curve has lower ceiling than higher mass " << mi->first;
		}
	}
	return r;
}

const std::string& to_str(Aircraft::CheckError::type_t t)
{
	switch (t) {
	case Aircraft::CheckError::type_climb:
	{
		static const std::string r("climb");
		return r;
	}

	case Aircraft::CheckError::type_descent:
	{
		static const std::string r("descent");
		return r;
	}

	case Aircraft::CheckError::type_cruise:
	{
		static const std::string r("cruise");
		return r;
	}

	case Aircraft::CheckError::type_unknown:
	default:
	{
		static const std::string r("unknown");
		return r;
	}
	}
}

const std::string& to_str(Aircraft::CheckError::severity_t s)
{
	switch (s) {
	case Aircraft::CheckError::severity_warning:
	{
		static const std::string r("warning");
		return r;
	}

	case Aircraft::CheckError::severity_error:
	{
		static const std::string r("error");
		return r;
	}

	default:
	{
		static const std::string r("invalid");
		return r;
	}
	}
}

Aircraft::CheckError::CheckError(type_t typ, severity_t sev, const std::string& n, double mass, double isaoffs, const std::string& msg)
	: m_name(n), m_message(msg), m_mass(mass), m_isaoffset(isaoffs),
	  m_timeinterval(timeinterval_t(std::numeric_limits<double>::quiet_NaN(), std::numeric_limits<double>::quiet_NaN())),
	  m_type(typ), m_severity(sev)
{
}

Aircraft::CheckError::CheckError(const ClimbDescent& cd, bool descent, severity_t sev, double tmin, double tmax)
	: m_name(cd.get_name()), m_mode(::to_str(cd.get_mode())), m_mass(cd.get_mass()), m_isaoffset(cd.get_isaoffset()),
	  m_timeinterval(timeinterval_t(tmin, tmax)), m_type(descent ? type_descent : type_climb), m_severity(sev)
{
}

Aircraft::CheckError::CheckError(const Cruise::Curve& c, severity_t sev)
	: m_name(c.get_name()), m_message(""), m_mass(c.get_mass()), m_isaoffset(c.get_isaoffset()),
	  m_timeinterval(timeinterval_t(std::numeric_limits<double>::quiet_NaN(), std::numeric_limits<double>::quiet_NaN())),
	  m_type(type_cruise), m_severity(sev)
{
}

std::string Aircraft::CheckError::to_str(void) const
{
	std::ostringstream oss;
	switch (get_severity()) {
	case severity_warning:
		oss << "[W]";
		break;

	case severity_error:
		oss << "[E]";
		break;

	default:
		oss << "[?]";
		break;
	}
	oss << ' ' << ::to_str(get_type());
	if (!get_name().empty())
		oss << " name " << get_name();
	if (!std::isnan(get_mass()))
		oss << " mass " << get_mass();
	if (!std::isnan(get_isaoffset()))
		oss << " isaoffset " << get_isaoffset();
	if (!get_mode().empty())
		oss << " mode " << get_mode();
	if (!std::isnan(get_mintime()) || !std::isnan(get_maxtime())) {
		oss << " time ";
		if (!std::isnan(get_mintime()))
			oss << get_mintime();
		oss << "...";
		if (!std::isnan(get_maxtime()))
			oss << get_maxtime();
	}
	oss << ' ' << get_message();
	return oss.str();
}

#ifdef HAVE_JSONCPP
Json::Value Aircraft::CheckError::to_json(void) const
{
	Json::Value r;
	r["type"] = ::to_str(get_type());
	r["severity"] = ::to_str(get_severity());
	if (!get_name().empty())
		r["name"] = get_name();
	if (!get_mode().empty())
		r["mode"] = get_mode();
	if (!std::isnan(get_mass()))
		r["mass"] = get_mass();
	if (!std::isnan(get_isaoffset()))
		r["isaoffset"] = get_isaoffset();
	if (!std::isnan(get_mintime()))
		r["mintime"] = get_mintime();
	if (!std::isnan(get_maxtime()))
		r["maxtime"] = get_maxtime();
	r["message"] = get_message();
	return r;
}
#endif

void Aircraft::CheckErrors::set_name(const std::string& n)
{
	for (iterator i(begin()), e(end()); i != e; ++i)
		i->set_name(n);
}

void Aircraft::CheckErrors::set_mode(const std::string& m)
{
	for (iterator i(begin()), e(end()); i != e; ++i)
		i->set_mode(m);
}

void Aircraft::CheckErrors::set_mass(double m)
{
	for (iterator i(begin()), e(end()); i != e; ++i)
		i->set_mass(m);
}

void Aircraft::CheckErrors::set_isaoffset(double io)
{
	for (iterator i(begin()), e(end()); i != e; ++i)
		i->set_isaoffset(io);
}

void Aircraft::CheckErrors::set_type(CheckError::type_t t)
{
	for (iterator i(begin()), e(end()); i != e; ++i)
		i->set_type(t);
}

void Aircraft::CheckErrors::set_severity(CheckError::severity_t s)
{
	for (iterator i(begin()), e(end()); i != e; ++i)
		i->set_severity(s);
}

Aircraft::CheckErrors::iterator Aircraft::CheckErrors::add(const CheckErrors& ce)
{
	return insert(end(), ce.begin(), ce.end());
}

Aircraft::CheckError::MessageOStream Aircraft::CheckErrors::add(const CheckError& ce)
{
	push_back(ce);
	return back().get_messageostream();
}

std::ostream& Aircraft::CheckErrors::print(std::ostream& os, const std::string& indent) const
{
	if (empty())
		return os;
	os << indent << "Aircraft Check Errors:" << std::endl;
	for (const_iterator i(begin()), e(end()); i != e; ++i)
		os << indent << i->to_str() << std::endl;
	return os;
}

#ifdef HAVE_JSONCPP
Json::Value Aircraft::CheckErrors::to_json(void) const
{
	Json::Value r(Json::arrayValue);
	for (const_iterator i(begin()), e(end()); i != e; ++i)
		r.append(i->to_json());
	return r;
}
#endif

constexpr double Aircraft::avgas_density;
constexpr double Aircraft::jeta_density;
constexpr double Aircraft::diesel_density;
constexpr double Aircraft::oil_density;
constexpr double Aircraft::quart_to_liter;
constexpr double Aircraft::usgal_to_liter;
constexpr double Aircraft::lb_to_kg;
constexpr double Aircraft::kg_to_lb;

const std::string& to_str(Aircraft::unit_t u, bool lng)
{
	switch (u) {
	case Aircraft::unit_m:
	{
		static const std::string r("m");
		return r;
	}

	case Aircraft::unit_cm:
	{
		static const std::string r("cm");
		return r;
	}

	case Aircraft::unit_km:
	{
		static const std::string r("km");
		return r;
	}

	case Aircraft::unit_in:
	{
		static const std::string r("in");
		return r;
	}

	case Aircraft::unit_ft:
	{
		static const std::string r("ft");
		return r;
	}

	case Aircraft::unit_nmi:
	{
		static const std::string r("nmi");
		return r;
	}

	case Aircraft::unit_kg:
	{
		static const std::string r("kg");
		return r;
	}

	case Aircraft::unit_lb:
	{
		static const std::string r("lb");
		return r;
	}

	case Aircraft::unit_liter:
	{
		if (lng) {
			static const std::string r("Liter");
			return r;
		}
		static const std::string r("l");
		return r;
	}

	case Aircraft::unit_usgal:
	{
		if (lng) {
			static const std::string r("USGal");
			return r;
		}
		static const std::string r("usg");
		return r;
	}

	case Aircraft::unit_quart:
	{
		if (lng) {
			static const std::string r("Quart");
			return r;
		}
		static const std::string r("qt");
		return r;
	}

	case Aircraft::unit_invalid:
	default:
	{
		static const std::string r("invalid");
		return r;
	}
	}
}

std::string to_str(Aircraft::unitmask_t um, bool lng)
{
	std::string r;
	for (Aircraft::unit_t u(Aircraft::unit_m); u <= Aircraft::unit_invalid; u = (Aircraft::unit_t)(u + 1)) {
		if (!(um & (Aircraft::unitmask_t)(1 << u)))
			continue;
		if (!r.empty())
			r += ",";
		r += to_str(u, lng);
	}
	return r;
}

const std::string& to_str(Aircraft::propulsion_t prop)
{
	switch (prop) {
	case Aircraft::propulsion_fixedpitch:
	{
		static const std::string r("fixedpitch");
		return r;
	}

	case Aircraft::propulsion_constantspeed:
	{
		static const std::string r("constantspeed");
		return r;
	}

	case Aircraft::propulsion_turboprop:
	{
		static const std::string r("turboprop");
		return r;
	}

	case Aircraft::propulsion_turbojet:
	{
		static const std::string r("turbojet");
		return r;
	}

	default:
	{
		static const std::string r("?");
		return r;
	}
	}
}

const std::string& to_str(Aircraft::opsrules_t r)
{
	switch (r) {
	case Aircraft::opsrules_easacat:
	{
		static const std::string r("easacat");
		return r;
	}

	case Aircraft::opsrules_easanco:
	{
		static const std::string r("easanco");
		return r;
	}

	case Aircraft::opsrules_auto:
	default:
	{
		static const std::string r("auto");
		return r;
	}
	}
}

Aircraft::Aircraft(void)
	: m_callsign("ZZZZZ"), m_manufacturer("Piper"), m_model("PA28R-200"), m_year("1971"), m_icaotype("P28R"),
	  m_equipment("SDFGBRY"), m_transponder("S"), m_colormarking("WHITE BLUE STRIPES"), m_emergencyradio("E"),
	  m_survival(""), m_lifejackets(""), m_dinghies(""), m_picname(""), m_crewcontact(""), m_homebase(""),
	  m_mrm(2600), m_mtom(2600), m_mlm(2600), m_mzfm(2600),
	  m_vr0(62 * 0.86897624), m_vr1(68 * 0.86897624), m_vx0(80 * 0.86897624), m_vx1(80 * 0.86897624),
	  m_vy(95 * 0.86897624), m_vs0(64 * 0.86897624), m_vs1(70 * 0.86897624), m_vno(170 * 0.86897624), m_vne(214 * 0.86897624),
	  m_vref0(64 * 1.4 * 0.86897624), m_vref1(70 * 1.4 * 0.86897624), m_va(134 * 0.86897624), m_vfe(125 * 0.86897624),
	  m_vgearext(150 * 0.86897624), m_vgearret(125 * 0.86897624), m_vbestglide(105 * 0.86897624), m_glideslope(8.1086),
	  m_vdescent(150 * 0.86897624), m_fuelmass(usgal_to_liter * avgas_density * kg_to_lb), m_taxifuel(0.5), m_taxifuelflow(0.),
	  m_maxbhp(200), m_contingencyfuel(5), m_pbn(pbn_b2 | pbn_d2), m_gnssflags(gnssflags_gps | gnssflags_sbas), m_fuelunit(unit_usgal),
	  m_propulsion(propulsion_constantspeed), m_opsrules(opsrules_auto), m_freecirculation(false)
{
	get_cruise().set_mass(get_mtom());
	get_cruise().build_altmap(get_propulsion());
	get_climb().set_point_vy(get_vy());
	get_climb().set_mass(get_mtom());
	get_descent().set_point_vy(get_vdescent());
	get_descent().set_mass(get_mtom());
	get_descent().set_default(get_propulsion(), get_cruise(), get_climb());
	//check_aircraft_type_class();
}

void Aircraft::load_xml(const xmlpp::Element *el)
{
	if (!el)
		return;
	m_otherinfo.clear();
	m_callsign.clear();
	m_manufacturer.clear();
	m_model.clear();
	m_year.clear();
	m_icaotype.clear();
	m_equipment.clear();
	m_transponder.clear();
	m_pbn = pbn_none;
	m_gnssflags = gnssflags_none;
	m_colormarking.clear();
	m_emergencyradio.clear();
	m_survival.clear();
	m_lifejackets.clear();
	m_dinghies.clear();
	m_picname.clear();
	m_crewcontact.clear();
	m_homebase.clear();
	m_remark.clear();
	m_fuelunit = unit_invalid;
	m_propulsion = propulsion_fixedpitch;
	m_opsrules = opsrules_auto;
	m_mrm = m_mtom = m_mlm = m_mzfm = m_vr0 = m_vr1 = m_vx0 = m_vx1 = m_vy = m_vs0 = m_vs1 = m_vno = m_vne =
		m_vref0 = m_vref1 = m_va = m_vfe = m_vgearext = m_vgearret = m_vbestglide = m_glideslope = m_vdescent =
		m_fuelmass = m_taxifuel = m_taxifuelflow = m_maxbhp = m_contingencyfuel = std::numeric_limits<double>::quiet_NaN();
	m_freecirculation = false;
	bool haspbn(false), hasgnssflags(false);
	xmlpp::Attribute *attr;
	if ((attr = el->get_attribute("callsign")))
		m_callsign = attr->get_value();
	if ((attr = el->get_attribute("manufacturer")))
		m_manufacturer = attr->get_value();
	if ((attr = el->get_attribute("model")))
		m_model = attr->get_value();
	else if ((attr = el->get_attribute("aircrafttype")))
		m_model = attr->get_value();
	if ((attr = el->get_attribute("year")))
		m_year = attr->get_value();
	if ((attr = el->get_attribute("icaotype")))
		m_icaotype = attr->get_value();
	if ((attr = el->get_attribute("equipment")))
		m_equipment = attr->get_value();
	if ((attr = el->get_attribute("transponder")))
		m_transponder = attr->get_value();
	haspbn = (attr = el->get_attribute("pbn"));
	if (haspbn)
		set_pbn(attr->get_value());
	hasgnssflags = (attr = el->get_attribute("gnssflags"));
	if (hasgnssflags)
		set_gnssflags(attr->get_value());
	if ((attr = el->get_attribute("colormarking")))
		m_colormarking = attr->get_value();
	if ((attr = el->get_attribute("emergencyradio")))
		m_emergencyradio = attr->get_value();
	if ((attr = el->get_attribute("survival")))
		m_survival = attr->get_value();
	if ((attr = el->get_attribute("picname")))
		m_picname = attr->get_value();
	if ((attr = el->get_attribute("crewcontact")))
		m_crewcontact = attr->get_value();
	if ((attr = el->get_attribute("homebase")))
		m_homebase = attr->get_value();
	if ((attr = el->get_attribute("lifejackets")))
		m_lifejackets = attr->get_value();
	if ((attr = el->get_attribute("dinghies")))
		m_dinghies = attr->get_value();
	if ((attr = el->get_attribute("mrm")))
		m_mrm = Glib::Ascii::strtod(attr->get_value());
	if ((attr = el->get_attribute("mtom")))
		m_mtom = Glib::Ascii::strtod(attr->get_value());
	if ((attr = el->get_attribute("mlm")))
		m_mlm = Glib::Ascii::strtod(attr->get_value());
	if ((attr = el->get_attribute("mzfm")))
		m_mzfm = Glib::Ascii::strtod(attr->get_value());
	if ((attr = el->get_attribute("vr0")))
		m_vr0 = Glib::Ascii::strtod(attr->get_value());
	if ((attr = el->get_attribute("vr1")))
		m_vr1 = Glib::Ascii::strtod(attr->get_value());
	if ((attr = el->get_attribute("vx0")))
		m_vx0 = Glib::Ascii::strtod(attr->get_value());
	if ((attr = el->get_attribute("vx1")))
		m_vx1 = Glib::Ascii::strtod(attr->get_value());
	if ((attr = el->get_attribute("vy")))
		m_vy = Glib::Ascii::strtod(attr->get_value());
	if ((attr = el->get_attribute("vs0")))
		m_vs0 = Glib::Ascii::strtod(attr->get_value());
	if ((attr = el->get_attribute("vs1")))
		m_vs1 = Glib::Ascii::strtod(attr->get_value());
	if ((attr = el->get_attribute("vno")))
		m_vno = Glib::Ascii::strtod(attr->get_value());
	if ((attr = el->get_attribute("vne")))
		m_vne = Glib::Ascii::strtod(attr->get_value());
	if ((attr = el->get_attribute("vref0")))
		m_vref0 = Glib::Ascii::strtod(attr->get_value());
	if ((attr = el->get_attribute("vref1")))
		m_vref1 = Glib::Ascii::strtod(attr->get_value());
	if ((attr = el->get_attribute("va")))
		m_va = Glib::Ascii::strtod(attr->get_value());
	if ((attr = el->get_attribute("vfe")))
		m_vfe = Glib::Ascii::strtod(attr->get_value());
	if ((attr = el->get_attribute("vgearext")))
		m_vgearext = Glib::Ascii::strtod(attr->get_value());
	if ((attr = el->get_attribute("vgearret")))
		m_vgearret = Glib::Ascii::strtod(attr->get_value());
	if ((attr = el->get_attribute("vbestglide")))
		m_vbestglide = Glib::Ascii::strtod(attr->get_value());
	if ((attr = el->get_attribute("glideslope")))
		m_glideslope = Glib::Ascii::strtod(attr->get_value());
	if ((attr = el->get_attribute("vdescent")))
		m_vdescent = Glib::Ascii::strtod(attr->get_value());
	if ((attr = el->get_attribute("fuelmass")))
		m_fuelmass = Glib::Ascii::strtod(attr->get_value());
	if ((attr = el->get_attribute("taxifuel")))
		m_taxifuel = Glib::Ascii::strtod(attr->get_value());
	if ((attr = el->get_attribute("taxifuelflow")))
		m_taxifuelflow = Glib::Ascii::strtod(attr->get_value());
	if ((attr = el->get_attribute("maxbhp")))
		m_maxbhp = Glib::Ascii::strtod(attr->get_value());
	if ((attr = el->get_attribute("contingencyfuel")))
		m_contingencyfuel = Glib::Ascii::strtod(attr->get_value());
	if ((attr = el->get_attribute("fuelunitname")))
		m_fuelunit = parse_unit(attr->get_value());
	if (!((1 << m_fuelunit) & (unitmask_mass | unitmask_volume)))
		m_fuelunit = unit_usgal;
	if ((attr = el->get_attribute("remark")))
		m_remark = attr->get_value();
	attr = el->get_attribute("propulsion");
	if (!attr) {
		xmlpp::Node::NodeList nl(el->get_children("cruise"));
		for (xmlpp::Node::NodeList::const_iterator ni(nl.begin()), ne(nl.end()); ni != ne; ++ni) {
			attr = static_cast<const xmlpp::Element *>(*ni)->get_attribute("propulsion");
			if (attr)
				break;
		}
	}
	bool hasprop(false);
	if (attr) {
		static const propulsion_t propval[] = {
			propulsion_fixedpitch,
			propulsion_constantspeed,
			propulsion_turboprop,
			propulsion_turbojet
		};
		unsigned int i;
		for (i = 0; i < sizeof(propval)/sizeof(propval[0]); ++i) {
			if (attr->get_value() == to_str(propval[i]))
				break;
		}
		if (i < sizeof(propval)/sizeof(propval[0])) {
			m_propulsion = propval[i];
			hasprop = true;
		}
	}
	if ((attr = el->get_attribute("opsrules"))) {
		for (opsrules_t r(opsrules_auto); r <= opsrules_last; r = (opsrules_t)(r + 1)) {
			if (attr->get_value() == to_str(r)) {
				m_opsrules = r;
				break;
			}
		}
	}
	if ((attr = el->get_attribute("freecirculation")))
		m_freecirculation = attr->get_value() == std::string("Y") || attr->get_value() == std::string("y") || attr->get_value() == std::string("1");
	xmlpp::Node::NodeList nl(el->get_children("wb"));
	for (xmlpp::Node::NodeList::const_iterator ni(nl.begin()), ne(nl.end()); ni != ne; ++ni)
		m_wb.load_xml(static_cast<const xmlpp::Element *>(*ni));
	nl = el->get_children("distances");
	for (xmlpp::Node::NodeList::const_iterator ni(nl.begin()), ne(nl.end()); ni != ne; ++ni)
		m_dist.load_xml(static_cast<const xmlpp::Element *>(*ni));
	nl = el->get_children("climb");
	for (xmlpp::Node::NodeList::const_iterator ni(nl.begin()), ne(nl.end()); ni != ne; ++ni)
		m_climb.load_xml(static_cast<const xmlpp::Element *>(*ni));
	nl = el->get_children("descent");
	for (xmlpp::Node::NodeList::const_iterator ni(nl.begin()), ne(nl.end()); ni != ne; ++ni)
		m_descent.load_xml(static_cast<const xmlpp::Element *>(*ni));
	nl = el->get_children("cruise");
	for (xmlpp::Node::NodeList::const_iterator ni(nl.begin()), ne(nl.end()); ni != ne; ++ni)
		m_cruise.load_xml(static_cast<const xmlpp::Element *>(*ni), get_maxbhp());
	nl = el->get_children("otherinfo");
	for (xmlpp::Node::NodeList::const_iterator ni(nl.begin()), ne(nl.end()); ni != ne; ++ni) {
		const xmlpp::Element *el2(static_cast<const xmlpp::Element *>(*ni));
		Glib::ustring category, text;
		if ((attr = el2->get_attribute("category")))
			category = attr->get_value();
		if (category.empty())
			continue;
		if ((attr = el2->get_attribute("text")))
			text = attr->get_value();
		otherinfo_add(category, text);
	}
	// fix propulsion
	{
		const Cruise& cruise(get_cruise());
		if (!hasprop && cruise.has_variablepitch())
			m_propulsion = propulsion_constantspeed;
		if (hasprop && m_propulsion == propulsion_constantspeed &&
		    !cruise.has_variablepitch() && cruise.has_fixedpitch())
			m_propulsion = propulsion_fixedpitch;
	}
	get_cruise().set_mass(get_mtom());
	get_cruise().build_altmap(get_propulsion());
	get_climb().set_point_vy(get_vy());
	get_climb().set_mass(get_mtom());
	get_descent().set_point_vy(get_vdescent());
	get_descent().set_mass(get_mtom());
	get_descent().set_default(get_propulsion(), get_cruise(), get_climb());
	// Fix up equipment
	m_equipment = m_equipment.uppercase();
	m_transponder = m_transponder.uppercase();
	pbn_fix_equipment();
	if (haspbn && !hasgnssflags) {
		if (get_pbn() & pbn_gnss) {
			m_gnssflags |= gnssflags_gps;
			if (get_pbn() & (pbn_d1 | pbn_d2 | pbn_o1 | pbn_o2 | pbn_s1 | pbn_s2 | pbn_t1 | pbn_t2))
				m_gnssflags |= gnssflags_sbas;
			if (get_pbn() & pbn_s2)
				m_gnssflags |= gnssflags_baroaid;
		}
	}
	// try to load from FS xml file
	nl = el->get_children("aircraft_setup_info");
	if (nl.begin() != nl.end())
		load_xml_fs(el);
	if (std::isnan(m_fuelmass)) {
		WeightBalance::Element::flags_t flags(get_wb().get_element_flags());
		flags &= WeightBalance::Element::flag_fuel;
		switch (flags) {
		case WeightBalance::Element::flag_avgas:
		case WeightBalance::Element::flag_jeta:
		case WeightBalance::Element::flag_diesel:
		default:
			break;
		}
	}
}

void Aircraft::save_xml(xmlpp::Element *el) const
{
	if (!el)
		return;
	if (m_callsign.empty())
		el->remove_attribute("callsign");
	else
		el->set_attribute("callsign", m_callsign);
	if (m_manufacturer.empty())
		el->remove_attribute("manufacturer");
	else
		el->set_attribute("manufacturer", m_manufacturer);
	if (m_model.empty())
		el->remove_attribute("model");
	else
		el->set_attribute("model", m_model);
	if (m_year.empty())
		el->remove_attribute("year");
	else
		el->set_attribute("year", m_year);
	if (m_icaotype.empty())
		el->remove_attribute("icaotype");
	else
		el->set_attribute("icaotype", m_icaotype);
	if (m_equipment.empty())
		el->remove_attribute("equipment");
	else
		el->set_attribute("equipment", m_equipment);
	if (m_transponder.empty())
		el->remove_attribute("transponder");
	else
		el->set_attribute("transponder", m_transponder);
	if (m_pbn == pbn_none)
		el->remove_attribute("pbn");
	else
		el->set_attribute("pbn", get_pbn_string());
	if (m_gnssflags == gnssflags_none)
		el->remove_attribute("gnssflags");
	else
		el->set_attribute("gnssflags", get_gnssflags_string());
	if (m_colormarking.empty())
		el->remove_attribute("colormarking");
	else
		el->set_attribute("colormarking", m_colormarking);
	if (m_emergencyradio.empty())
		el->remove_attribute("emergencyradio");
	else
		el->set_attribute("emergencyradio", m_emergencyradio);
	if (m_survival.empty())
		el->remove_attribute("survival");
	else
		el->set_attribute("survival", m_survival);
	if (m_lifejackets.empty())
		el->remove_attribute("lifejackets");
	else
		el->set_attribute("lifejackets", m_lifejackets);
	if (m_dinghies.empty())
		el->remove_attribute("dinghies");
	else
		el->set_attribute("dinghies", m_dinghies);
	if (m_picname.empty())
		el->remove_attribute("picname");
	else
		el->set_attribute("picname", m_picname);
	if (m_crewcontact.empty())
		el->remove_attribute("crewcontact");
	else
		el->set_attribute("crewcontact", m_crewcontact);
	if (m_homebase.empty())
		el->remove_attribute("homebase");
	else
		el->set_attribute("homebase", m_homebase);
	if (std::isnan(m_mrm)) {
		el->remove_attribute("mrm");
	} else {
		std::ostringstream oss;
		oss << m_mrm;
		el->set_attribute("mrm", oss.str());
	}
	if (std::isnan(m_mtom)) {
		el->remove_attribute("mtom");
	} else {
		std::ostringstream oss;
		oss << m_mtom;
		el->set_attribute("mtom", oss.str());
	}
	if (std::isnan(m_mlm)) {
		el->remove_attribute("mlm");
	} else {
		std::ostringstream oss;
		oss << m_mlm;
		el->set_attribute("mlm", oss.str());
	}
	if (std::isnan(m_mzfm)) {
		el->remove_attribute("mzfm");
	} else {
		std::ostringstream oss;
		oss << m_mzfm;
		el->set_attribute("mzfm", oss.str());
	}
	if (std::isnan(m_vr0)) {
		el->remove_attribute("vr0");
	} else {
		std::ostringstream oss;
		oss << m_vr0;
		el->set_attribute("vr0", oss.str());
	}
	if (std::isnan(m_vr1)) {
		el->remove_attribute("vr1");
	} else {
		std::ostringstream oss;
		oss << m_vr1;
		el->set_attribute("vr1", oss.str());
	}
	if (std::isnan(m_vx0)) {
		el->remove_attribute("vx0");
	} else {
		std::ostringstream oss;
		oss << m_vx0;
		el->set_attribute("vx0", oss.str());
	}
	if (std::isnan(m_vx1)) {
		el->remove_attribute("vx1");
	} else {
		std::ostringstream oss;
		oss << m_vx1;
		el->set_attribute("vx1", oss.str());
	}
	if (std::isnan(m_vy)) {
		el->remove_attribute("vy");
	} else {
		std::ostringstream oss;
		oss << m_vy;
		el->set_attribute("vy", oss.str());
	}
	if (std::isnan(m_vs0)) {
		el->remove_attribute("vs0");
	} else {
		std::ostringstream oss;
		oss << m_vs0;
		el->set_attribute("vs0", oss.str());
	}
	if (std::isnan(m_vs1)) {
		el->remove_attribute("vs1");
	} else {
		std::ostringstream oss;
		oss << m_vs1;
		el->set_attribute("vs1", oss.str());
	}
	if (std::isnan(m_vno)) {
		el->remove_attribute("vno");
	} else {
		std::ostringstream oss;
		oss << m_vno;
		el->set_attribute("vno", oss.str());
	}
	if (std::isnan(m_vne)) {
		el->remove_attribute("vne");
	} else {
		std::ostringstream oss;
		oss << m_vne;
		el->set_attribute("vne", oss.str());
	}
	if (std::isnan(m_vref0)) {
		el->remove_attribute("vref0");
	} else {
		std::ostringstream oss;
		oss << m_vref0;
		el->set_attribute("vref0", oss.str());
	}
	if (std::isnan(m_vref1)) {
		el->remove_attribute("vref1");
	} else {
		std::ostringstream oss;
		oss << m_vref1;
		el->set_attribute("vref1", oss.str());
	}
	if (std::isnan(m_va)) {
		el->remove_attribute("va");
	} else {
		std::ostringstream oss;
		oss << m_va;
		el->set_attribute("va", oss.str());
	}
	if (std::isnan(m_vfe)) {
		el->remove_attribute("vfe");
	} else {
		std::ostringstream oss;
		oss << m_vfe;
		el->set_attribute("vfe", oss.str());
	}
	if (std::isnan(m_vgearext)) {
		el->remove_attribute("vgearext");
	} else {
		std::ostringstream oss;
		oss << m_vgearext;
		el->set_attribute("vgearext", oss.str());
	}
	if (std::isnan(m_vgearret)) {
		el->remove_attribute("vgearret");
	} else {
		std::ostringstream oss;
		oss << m_vgearret;
		el->set_attribute("vgearret", oss.str());
	}
	if (std::isnan(m_vbestglide)) {
		el->remove_attribute("vbestglide");
	} else {
		std::ostringstream oss;
		oss << m_vbestglide;
		el->set_attribute("vbestglide", oss.str());
	}
	if (std::isnan(m_glideslope)) {
		el->remove_attribute("glideslope");
	} else {
		std::ostringstream oss;
		oss << m_glideslope;
		el->set_attribute("glideslope", oss.str());
	}
	if (std::isnan(m_vdescent)) {
		el->remove_attribute("vdescent");
	} else {
		std::ostringstream oss;
		oss << m_vdescent;
		el->set_attribute("vdescent", oss.str());
	}
	if (m_fuelunit == unit_invalid)
		el->remove_attribute("fuelunitname");
	else
		el->set_attribute("fuelunitname", to_str(m_fuelunit));
	if (std::isnan(m_fuelmass)) {
		el->remove_attribute("fuelmass");
	} else {
		std::ostringstream oss;
		oss << m_fuelmass;
		el->set_attribute("fuelmass", oss.str());
	}
	if (std::isnan(m_taxifuel)) {
		el->remove_attribute("taxifuel");
	} else {
		std::ostringstream oss;
		oss << m_taxifuel;
		el->set_attribute("taxifuel", oss.str());
	}
	if (std::isnan(m_taxifuelflow)) {
		el->remove_attribute("taxifuelflow");
	} else {
		std::ostringstream oss;
		oss << m_taxifuelflow;
		el->set_attribute("taxifuelflow", oss.str());
	}
	if (std::isnan(m_maxbhp)) {
		el->remove_attribute("maxbhp");
	} else {
		std::ostringstream oss;
		oss << m_maxbhp;
		el->set_attribute("maxbhp", oss.str());
	}
	if (std::isnan(m_contingencyfuel)) {
		el->remove_attribute("contingencyfuel");
	} else {
		std::ostringstream oss;
		oss << m_contingencyfuel;
		el->set_attribute("contingencyfuel", oss.str());
	}
	if (m_remark.empty())
		el->remove_attribute("remark");
	else
		el->set_attribute("remark", m_remark);
	el->set_attribute("propulsion", to_str(m_propulsion));
	el->set_attribute("opsrules", to_str(m_opsrules));
	el->set_attribute("freecirculation", is_freecirculation() ? "Y" : "N");
	m_wb.save_xml(el->add_child("wb"));
	m_dist.save_xml(el->add_child("distances"));
	m_climb.save_xml(el->add_child("climb"));
	m_descent.save_xml(el->add_child("descent"));
	m_cruise.save_xml(el->add_child("cruise"), get_maxbhp());
	for (otherinfo_const_iterator_t oii(otherinfo_begin()), oie(otherinfo_end()); oii != oie; ++oii) {
		if (!oii->is_valid())
			continue;
		xmlpp::Element *el2(el->add_child("otherinfo"));
		el2->set_attribute("category", oii->get_category());
		if (!oii->get_text().empty())
			el2->set_attribute("text", oii->get_text());
	}
}

std::string Aircraft::get_text(const xmlpp::Node *n)
{
	const xmlpp::Element *el(static_cast<const xmlpp::Element *>(n));
	if (!el)
		return "";
	const xmlpp::TextNode *t(el->get_child_text());
	if (!t)
		return "";
	return t->get_content();
}

void Aircraft::load_xml_fs(const xmlpp::Element *el)
{
	m_propulsion = propulsion_fixedpitch;
	double speedfactor(1); // kts
	double fuelfactor(1); // USgal
	double massfactor(0.45359237); // lb->kg
	double armfactor(1); // In
	xmlpp::Node::NodeList nl(el->get_children("aircraft_setup_info"));
	for (xmlpp::Node::NodeList::const_iterator ni(nl.begin()), ne(nl.end()); ni != ne; ++ni) {
		const xmlpp::Element *el2(static_cast<const xmlpp::Element *>(*ni));
		xmlpp::Node::NodeList nl2(el2->get_children("aircraft_type"));
		for (xmlpp::Node::NodeList::const_iterator ni2(nl2.begin()), ne2(nl2.end()); ni2 != ne2; ++ni2) {
			std::string txt(get_text(*ni2));
			if (txt.empty()) {
				continue;
			} else if (txt == "Piston fixed pitch") {
				m_propulsion = propulsion_fixedpitch;
			} else if (txt == "Piston variable pitch") {
				m_propulsion = propulsion_constantspeed;
			} else {
				std::cerr << "load_xml_fs: unknown aircraft_type " << txt << std::endl;
			}
		}
		nl2 = el2->get_children("speed_distance_units");
		for (xmlpp::Node::NodeList::const_iterator ni2(nl2.begin()), ne2(nl2.end()); ni2 != ne2; ++ni2) {
			std::string txt(get_text(*ni2));
			if (txt.empty()) {
				continue;
			} else if (txt == "Knots") {
				speedfactor = 1;
			} else if (txt == "MPH") {
				speedfactor = 0.86897624;
			} else {
				std::cerr << "load_xml_fs: unknown speed_distance_units " << txt << std::endl;
			}
		}
		nl2 = el2->get_children("fuel_units");
		for (xmlpp::Node::NodeList::const_iterator ni2(nl2.begin()), ne2(nl2.end()); ni2 != ne2; ++ni2) {
			std::string txt(get_text(*ni2));
			if (txt.empty()) {
				continue;
			} else if (txt == "Gallons") {
				fuelfactor = 1;
			} else if (txt == "Liters") {
				fuelfactor = 0.26417152;
			} else {
				std::cerr << "load_xml_fs: unknown fuel_units " << txt << std::endl;
			}
		}
		nl2 = el2->get_children("weight_units");
		for (xmlpp::Node::NodeList::const_iterator ni2(nl2.begin()), ne2(nl2.end()); ni2 != ne2; ++ni2) {
			std::string txt(get_text(*ni2));
			if (txt.empty()) {
				continue;
			} else if (txt == "Pounds") {
				massfactor = 0.45359237;
			} else if (txt == "kg") {
				massfactor = 1;
			} else {
				std::cerr << "load_xml_fs: unknown aircraft_type " << txt << std::endl;
			}
		}
		nl2 = el2->get_children("cg_units");
		for (xmlpp::Node::NodeList::const_iterator ni2(nl2.begin()), ne2(nl2.end()); ni2 != ne2; ++ni2) {
			std::string txt(get_text(*ni2));
			if (txt.empty()) {
				continue;
			} else if (txt == "Inches") {
				armfactor = 1;
			} else {
				std::cerr << "load_xml_fs: unknown cg_units " << txt << std::endl;
			}
		}
	}
	double maxfuel(0);
	{
		double servceiling(0), servceilingroc(0), servceilingff(0), slroc(0), slff(0);
		nl = el->get_children("basic_info");
		for (xmlpp::Node::NodeList::const_iterator ni(nl.begin()), ne(nl.end()); ni != ne; ++ni) {
			const xmlpp::Element *el2(static_cast<const xmlpp::Element *>(*ni));
			xmlpp::Node::NodeList nl2(el2->get_children("make"));
			for (xmlpp::Node::NodeList::const_iterator ni2(nl2.begin()), ne2(nl2.end()); ni2 != ne2; ++ni2) {
				std::string txt(get_text(*ni2));
				if (txt.empty()) {
					continue;
				}
				m_model = txt;
			}
			nl2 = el2->get_children("model");
			for (xmlpp::Node::NodeList::const_iterator ni2(nl2.begin()), ne2(nl2.end()); ni2 != ne2; ++ni2) {
				std::string txt(get_text(*ni2));
				if (txt.empty()) {
					continue;
				}
				m_icaotype = txt;
			}
			nl2 = el2->get_children("registration");
			for (xmlpp::Node::NodeList::const_iterator ni2(nl2.begin()), ne2(nl2.end()); ni2 != ne2; ++ni2) {
				std::string txt(get_text(*ni2));
				if (txt.empty()) {
					continue;
				}
				m_callsign = txt;
			}
			nl2 = el2->get_children("color");
			for (xmlpp::Node::NodeList::const_iterator ni2(nl2.begin()), ne2(nl2.end()); ni2 != ne2; ++ni2) {
				std::string txt(get_text(*ni2));
				if (txt.empty()) {
					continue;
				}
				m_colormarking = txt;
			}
			nl2 = el2->get_children("equipment_icao");
			for (xmlpp::Node::NodeList::const_iterator ni2(nl2.begin()), ne2(nl2.end()); ni2 != ne2; ++ni2) {
				std::string txt(get_text(*ni2));
				if (txt.empty()) {
					continue;
				}
				m_equipment = txt;
			}
			nl2 = el2->get_children("fuel_capacity");
			for (xmlpp::Node::NodeList::const_iterator ni2(nl2.begin()), ne2(nl2.end()); ni2 != ne2; ++ni2) {
				std::string txt(get_text(*ni2));
				if (txt.empty()) {
					continue;
				}
				maxfuel = Glib::Ascii::strtod(txt) * fuelfactor;
			}
			nl2 = el2->get_children("service_ceiling");
			for (xmlpp::Node::NodeList::const_iterator ni2(nl2.begin()), ne2(nl2.end()); ni2 != ne2; ++ni2) {
				std::string txt(get_text(*ni2));
				if (txt.empty()) {
					continue;
				}
				servceiling = Glib::Ascii::strtod(txt);
			}
			nl2 = el2->get_children("service_ceiling_roc");
			for (xmlpp::Node::NodeList::const_iterator ni2(nl2.begin()), ne2(nl2.end()); ni2 != ne2; ++ni2) {
				std::string txt(get_text(*ni2));
				if (txt.empty()) {
					continue;
				}
				servceilingroc = Glib::Ascii::strtod(txt);
			}
			nl2 = el2->get_children("sea_level_roc");
			for (xmlpp::Node::NodeList::const_iterator ni2(nl2.begin()), ne2(nl2.end()); ni2 != ne2; ++ni2) {
				std::string txt(get_text(*ni2));
				if (txt.empty()) {
					continue;
				}
				slroc = Glib::Ascii::strtod(txt);
			}
			nl2 = el2->get_children("indicated_climb_speed");
			for (xmlpp::Node::NodeList::const_iterator ni2(nl2.begin()), ne2(nl2.end()); ni2 != ne2; ++ni2) {
				std::string txt(get_text(*ni2));
				if (txt.empty()) {
					continue;
				}
				m_vy = Glib::Ascii::strtod(txt) * speedfactor;
			}
			nl2 = el2->get_children("sea_level_climb_fuel_rate");
			for (xmlpp::Node::NodeList::const_iterator ni2(nl2.begin()), ne2(nl2.end()); ni2 != ne2; ++ni2) {
				std::string txt(get_text(*ni2));
				if (txt.empty()) {
					continue;
				}
				slff = Glib::Ascii::strtod(txt) * fuelfactor;
			}
			nl2 = el2->get_children("service_ceiling_climb_fuel_rate");
			for (xmlpp::Node::NodeList::const_iterator ni2(nl2.begin()), ne2(nl2.end()); ni2 != ne2; ++ni2) {
				std::string txt(get_text(*ni2));
				if (txt.empty()) {
					continue;
				}
				servceilingff = Glib::Ascii::strtod(txt) * fuelfactor;
			}
		}
		if (servceiling > 0 && slroc > 0) {
			ClimbDescent cd;
			cd.clear_points();
			cd.add_point(ClimbDescent::Point(0, slroc, slff));
			cd.add_point(ClimbDescent::Point(servceiling, servceilingroc, servceilingff));
			m_climb.clear();
			m_climb.add(cd);
		}
	}
	nl = el->get_children("weight_balance_info");
	for (xmlpp::Node::NodeList::const_iterator ni(nl.begin()), ne(nl.end()); ni != ne; ++ni) {
		const xmlpp::Element *el2(static_cast<const xmlpp::Element *>(*ni));
		xmlpp::Node::NodeList nl2(el2->get_children("fuel_density"));
		for (xmlpp::Node::NodeList::const_iterator ni2(nl2.begin()), ne2(nl2.end()); ni2 != ne2; ++ni2) {
			std::string txt(get_text(*ni2));
			if (txt.empty()) {
				continue;
			}
			m_fuelmass = Glib::Ascii::strtod(txt) / fuelfactor;
		}
		nl2 = el2->get_children("main_envelope");
		for (xmlpp::Node::NodeList::const_iterator ni2(nl2.begin()), ne2(nl2.end()); ni2 != ne2; ++ni2) {
			const xmlpp::Element *el3(static_cast<const xmlpp::Element *>(*ni2));
			{
				WeightBalance::Envelope ev;
				xmlpp::Node::NodeList nl3(el3->get_children("point"));
				for (xmlpp::Node::NodeList::const_iterator ni3(nl3.begin()), ne3(nl3.end()); ni3 != ne3; ++ni3) {
					const xmlpp::Element *el4(static_cast<const xmlpp::Element *>(*ni3));
					double w, a;
					xmlpp::Attribute *attr;
					if ((attr = el4->get_attribute("weight")))
						w = Glib::Ascii::strtod(attr->get_value());
					else
						continue;
					if ((attr = el4->get_attribute("arm")))
						a = Glib::Ascii::strtod(attr->get_value());
					else
						continue;
					w *= massfactor;
					a *= armfactor;
					ev.add_point(a, w);
				}
				if (ev.size()) {
					double minarm, maxarm, minmass, maxmass;
					ev.get_bounds(minarm, maxarm, minmass, maxmass);
					m_mtom = maxmass;
					m_wb.add_envelope(ev);
				}
			}
		}
		nl2 = el2->get_children("moment_item_list");
		for (xmlpp::Node::NodeList::const_iterator ni2(nl2.begin()), ne2(nl2.end()); ni2 != ne2; ++ni2) {
			const xmlpp::Element *el3(static_cast<const xmlpp::Element *>(*ni2));
			xmlpp::Node::NodeList nl3(el3->get_children("moment_item"));
			for (xmlpp::Node::NodeList::const_iterator ni3(nl3.begin()), ne3(nl3.end()); ni3 != ne3; ++ni3) {
				const xmlpp::Element *el4(static_cast<const xmlpp::Element *>(*ni3));
				Glib::ustring name;
				double arm(std::numeric_limits<double>::quiet_NaN());
				double massmin(arm), massmax(arm), massdflt(arm);
				WeightBalance::Element::flags_t flg(WeightBalance::Element::flag_none);
				xmlpp::Node::NodeList nl4(el4->get_children("type"));
				for (xmlpp::Node::NodeList::const_iterator ni4(nl4.begin()), ne4(nl4.end()); ni4 != ne4; ++ni4) {
					std::string txt(get_text(*ni2));
					if (txt.empty()) {
						continue;
					}
					if (txt == "Fixed")
						flg |= WeightBalance::Element::flag_fixed;
				}
				nl4 = el4->get_children("desc");
				for (xmlpp::Node::NodeList::const_iterator ni4(nl4.begin()), ne4(nl4.end()); ni4 != ne4; ++ni4) {
					std::string txt(get_text(*ni2));
					if (txt.empty()) {
						continue;
					}
					name = txt;
				}
				nl4 = el4->get_children("default_weight");
				for (xmlpp::Node::NodeList::const_iterator ni4(nl4.begin()), ne4(nl4.end()); ni4 != ne4; ++ni4) {
					std::string txt(get_text(*ni2));
					if (txt.empty()) {
						continue;
					}
				        massdflt = Glib::Ascii::strtod(txt) * massfactor;
				}
				nl4 = el4->get_children("min_weight");
				for (xmlpp::Node::NodeList::const_iterator ni4(nl4.begin()), ne4(nl4.end()); ni4 != ne4; ++ni4) {
					std::string txt(get_text(*ni2));
					if (txt.empty()) {
						continue;
					}
				        massmin = Glib::Ascii::strtod(txt) * massfactor;
				}
				nl4 = el4->get_children("max_weight");
				for (xmlpp::Node::NodeList::const_iterator ni4(nl4.begin()), ne4(nl4.end()); ni4 != ne4; ++ni4) {
					std::string txt(get_text(*ni2));
					if (txt.empty()) {
						continue;
					}
				        massmax = Glib::Ascii::strtod(txt) * massfactor;
				}
				nl4 = el4->get_children("arm");
				for (xmlpp::Node::NodeList::const_iterator ni4(nl4.begin()), ne4(nl4.end()); ni4 != ne4; ++ni4) {
					std::string txt(get_text(*ni2));
					if (txt.empty()) {
						continue;
					}
				        arm = Glib::Ascii::strtod(txt) * armfactor;
				}
				if (flg & WeightBalance::Element::flag_fixed)
					massmin = massmax = massdflt;
				if (!std::isnan(massmin) && !std::isnan(massmax) && !std::isnan(arm)) {
					WeightBalance::Element el(name, massmin, massmax, arm, flg);
					el.add_auto_units(m_wb.get_massunit(), get_fuelunit(), unit_quart);
					m_wb.add_element(el);
				}
			}
		}
	}
	nl = el->get_children("performance_info");
	for (xmlpp::Node::NodeList::const_iterator ni(nl.begin()), ne(nl.end()); ni != ne; ++ni) {
		m_cruise = Cruise();
		const xmlpp::Element *el2(static_cast<const xmlpp::Element *>(*ni));
		xmlpp::Node::NodeList nl2(el2->get_children("constant_power_model"));
		for (xmlpp::Node::NodeList::const_iterator ni2(nl2.begin()), ne2(nl2.end()); ni2 != ne2; ++ni2) {
			const xmlpp::Element *el3(static_cast<const xmlpp::Element *>(*ni2));
			xmlpp::Node::NodeList nl3(el3->get_children("power_table"));
			for (xmlpp::Node::NodeList::const_iterator ni3(nl3.begin()), ne3(nl3.end()); ni3 != ne3; ++ni3) {
				const xmlpp::Element *el4(static_cast<const xmlpp::Element *>(*ni3));
				double pwr;
				xmlpp::Attribute *attr;
				if ((attr = el4->get_attribute("power")))
					pwr = Glib::Ascii::strtod(attr->get_value());
				else
					continue;
				xmlpp::Node::NodeList nl4(el4->get_children("power_entry"));
				for (xmlpp::Node::NodeList::const_iterator ni4(nl4.begin()), ne4(nl4.end()); ni4 != ne4; ++ni4) {
					const xmlpp::Element *el5(static_cast<const xmlpp::Element *>(*ni4));
					double alt, rpm;
					if ((attr = el5->get_attribute("altitude")))
						alt = Glib::Ascii::strtod(attr->get_value());
					else
						continue;
					if ((attr = el5->get_attribute("rpm")))
						rpm = Glib::Ascii::strtod(attr->get_value());
					else
						continue;
					Cruise::rpmmppoints_t pts;
					xmlpp::Node::NodeList nl5(el5->get_children("isa_standard"));
					for (xmlpp::Node::NodeList::const_iterator ni5(nl5.begin()), ne5(nl5.end()); ni5 != ne5; ++ni5) {
						const xmlpp::Element *el6(static_cast<const xmlpp::Element *>(*ni5));
						double mp(std::numeric_limits<double>::quiet_NaN()), fuel, tas;
						if ((attr = el6->get_attribute("mp")))
							mp = Glib::Ascii::strtod(attr->get_value());
						if ((attr = el6->get_attribute("fuel")))
							fuel = Glib::Ascii::strtod(attr->get_value()) * fuelfactor;
						else
							continue;
						if ((attr = el6->get_attribute("tas")))
							tas = Glib::Ascii::strtod(attr->get_value()) * speedfactor;
						else
							continue;
						m_maxbhp = 200;
						pts.push_back(Cruise::PointRPMMP(alt, rpm, mp, m_maxbhp * pwr * 0.01, tas, fuel));
					}
					m_cruise.set_points(pts, get_maxbhp());					
				}
			}
		}
	}
	get_cruise().set_mass(get_mtom());
	get_cruise().build_altmap(get_propulsion());
	get_climb().set_point_vy(get_vy());
	get_climb().set_mass(get_mtom());
	get_descent().set_point_vy(get_vdescent());
	get_descent().set_mass(get_mtom());
	get_descent().set_default(get_propulsion(), get_cruise(), get_climb());
	pbn_fix_equipment();
}

bool Aircraft::load_file(const Glib::ustring& filename)
{
	xmlpp::DomParser p(filename);
	if (!p)
		return false;
	xmlpp::Document *doc(p.get_document());
	xmlpp::Element *el(doc->get_root_node());
	if (el->get_name() != "aircraft")
		return false;
	load_xml(el);
	return true;
}

void Aircraft::save_file(const Glib::ustring& filename) const
{
	std::unique_ptr<xmlpp::Document> doc(new xmlpp::Document());
	save_xml(doc->create_root_node("aircraft"));
	doc->write_to_file_formatted(filename);
}

bool Aircraft::load_string(const Glib::ustring& data)
{
	xmlpp::DomParser p;
	p.parse_memory(data);
	if (!p)
		return false;
	xmlpp::Document *doc(p.get_document());
	xmlpp::Element *el(doc->get_root_node());
	if (el->get_name() != "aircraft")
		return false;
	load_xml(el);
	return true;
}

Glib::ustring Aircraft::save_string(void) const
{
	std::unique_ptr<xmlpp::Document> doc(new xmlpp::Document());
	save_xml(doc->create_root_node("aircraft"));
	return doc->write_to_string_formatted();
}


bool Aircraft::load_opsperf(const OperationsPerformance::Aircraft& acft)
{
	if (!acft.is_valid())
		return false;
	set_callsign("ZZZZZ");
	set_year("2015");
	set_equipment("SDFGLORY");
	set_transponder("S");
	set_pbn(pbn_b2 | pbn_d2);
	set_gnssflags(gnssflags_gps | gnssflags_sbas);
	set_colormarking("");
	set_emergencyradio("E");
	set_survival("");
	set_lifejackets("");
	set_dinghies("");
	set_picname("");
	set_crewcontact("");
	set_manufacturer(acft.get_description());
	set_model(acft.get_acfttype());
	set_icaotype(""); // must be set by synonym
	set_mrm(acft.get_massmax());
	set_mtom(acft.get_massmax());
	set_mlm(acft.get_massmax());
	set_mzfm(acft.get_massmax());
	get_wb().set_units(unit_in, unit_lb);
	get_wb().clear_elements();
	get_wb().clear_envelopes();
	get_dist() = Distances();
	get_dist().clear_takeoffdists();
	get_dist().clear_ldgdists();
	{
		const OperationsPerformance::Aircraft::Config& cfg(acft.get_config("LD"));
		set_vs0(cfg.get_vstall());
		set_vr0(0.97 * cfg.get_vstall());
		set_vx0(1.25 * cfg.get_vstall());
		set_vref0(1.4 * cfg.get_vstall());
	}
	{
		const OperationsPerformance::Aircraft::Config& cfg(acft.get_config("CR"));
		set_vs1(cfg.get_vstall());
		set_vr1(0.97 * cfg.get_vstall());
		set_vx1(1.25 * cfg.get_vstall());
		set_vref1(1.4 * cfg.get_vstall());
		set_va(1.8 * cfg.get_vstall());
		set_vbestglide(1.8 * cfg.get_vstall());
	}
	{
		const OperationsPerformance::Aircraft::Procedure& proc(acft.get_proc(1));
		set_vy(0.5 * (proc.get_climbcaslo() + proc.get_climbcashi()));
	}
	set_vno(acft.get_vmo());
	set_vne(acft.get_vmo());
	set_vfe(acft.get_vmo());
	set_vgearext(acft.get_vmo());
	set_vgearret(acft.get_vmo());
	set_glideslope(10);
	set_fuelmass((acft.get_propulsion() == OperationsPerformance::Aircraft::propulsion_piston) ?
		     (usgal_to_liter * avgas_density * kg_to_lb) :
		     (usgal_to_liter * jeta_density * kg_to_lb));
	set_maxbhp(0.14328 * acft.get_thrustclimb(0));
	double ffmul(60 * kg_to_lb / get_fuelmass());
	{
		ClimbDescent cd("", acft.get_massref());
		cd.clear_points();
		for (int alt = 0; alt < 66000; alt += 1000) {
			OperationsPerformance::Aircraft::ComputeResult res(acft.get_massref());
			AirData<float>& ad(res);
			ad.set_qnh_temp(); // ISA
			ad.set_pressure_altitude(alt);
			if (!acft.compute(res, OperationsPerformance::Aircraft::compute_climb))
				break;
			cd.add_point(Aircraft::ClimbDescent::Point(alt, res.get_rocd(), res.get_fuelflow() * ffmul));
		}
		cd.recalculatepoly(true);
		cd.limit_ceiling(acft.get_maxalt());
		get_climb().clear();
		get_climb().add(cd);
	}
	{
		ClimbDescent cd("", acft.get_massref());
		cd.clear_points();
		for (int alt = 0; alt < 66000; alt += 1000) {
			OperationsPerformance::Aircraft::ComputeResult res(acft.get_massref());
			AirData<float>& ad(res);
			ad.set_qnh_temp(); // ISA
			ad.set_pressure_altitude(alt);
			if (!acft.compute(res, OperationsPerformance::Aircraft::compute_descent))
				break;
			cd.add_point(Aircraft::ClimbDescent::Point(alt, res.get_rocd(), res.get_fuelflow() * ffmul));
		}
		cd.recalculatepoly(true);
		cd.limit_ceiling(acft.get_maxalt());
		get_descent().clear();
		get_descent().add(cd);
	}
	switch (acft.get_propulsion()) {
	case OperationsPerformance::Aircraft::propulsion_piston:
		m_propulsion = propulsion_fixedpitch;
		break;

	case OperationsPerformance::Aircraft::propulsion_turboprop:
		m_propulsion = propulsion_turboprop;
		break;

	case OperationsPerformance::Aircraft::propulsion_jet:
		m_propulsion = propulsion_turbojet;
		break;

	default:
		m_propulsion = propulsion_fixedpitch;
		break;
	}
	{
		Cruise::Curve c("", Cruise::Curve::flags_interpolate, acft.get_massref(),
				0, std::numeric_limits<double>::quiet_NaN());
		for (int alt = 0; alt < 66000; alt += 1000) {
			OperationsPerformance::Aircraft::ComputeResult res(acft.get_massref());
			AirData<float>& ad(res);
			ad.set_qnh_temp(); // ISA
			ad.set_pressure_altitude(alt);
			if (!acft.compute(res, OperationsPerformance::Aircraft::compute_cruise))
				break;
			c.insert(Aircraft::Cruise::Point(alt, 0.14328 * res.get_thrust(),
							 res.get_tas(), res.get_fuelflow() * ffmul));
		}
		get_cruise().add_curve(c);
	}
	get_cruise().set_mass(get_mtom());
	get_cruise().build_altmap(get_propulsion());
	return true;
}

Glib::ustring Aircraft::get_description(void) const
{
	Glib::ustring r(get_manufacturer());
	if (!r.empty() || !get_model().empty())
		r += " " + get_model();
	if (!get_year().empty())
		r += " (" + get_year() + ")";
	return r;
}

char Aircraft::get_wakecategory(void) const
{
	double mtom(get_mtom_kg());
	if (mtom <= 7000)
		return 'L';
	if (mtom <= 136000)
		return 'M';
	if (mtom <= 500000)
		return 'H';
	return 'J';
}

double Aircraft::get_mrm_kg(void) const
{
	return convert(get_wb().get_massunit(), unit_kg, get_mrm());
}

double Aircraft::get_mtom_kg(void) const
{
	return convert(get_wb().get_massunit(), unit_kg, get_mtom());
}

double Aircraft::get_mlm_kg(void) const
{
	return convert(get_wb().get_massunit(), unit_kg, get_mlm());
}

double Aircraft::get_mzfm_kg(void) const
{
	return convert(get_wb().get_massunit(), unit_kg, get_mzfm());
}

void Aircraft::otherinfo_add(const OtherInfo& oi)
{
	if (!oi.is_valid())
		return;
	std::pair<otherinfo_t::iterator,bool> ins(m_otherinfo.insert(oi));
	if (ins.second)
		return;
	Glib::ustring t(ins.first->get_text());
	if (!t.empty() || !oi.get_text().empty())
		t += " ";
	t += oi.get_text();
	OtherInfo oi2(ins.first->get_category(), t);
	m_otherinfo.erase(ins.first);
	m_otherinfo.insert(oi2);
}

const Aircraft::OtherInfo& Aircraft::otherinfo_find(const Glib::ustring& category) const
{
	static const OtherInfo notfound("", "");
	otherinfo_t::const_iterator i(m_otherinfo.find(OtherInfo(category)));
	if (i == m_otherinfo.end())
		return notfound;
	return *i;
}

// PBN Classification

Glib::ustring Aircraft::get_pbn_string(pbn_t pbn)
{
	static const char pbnstr[][4] = {
		"A1",
		"B1",
		"B2",
		"B3",
		"B4",
		"B5",
		"B6",
		"C1",
		"C2",
		"C3",
		"C4",
		"D1",
		"D2",
		"D3",
		"D4",
		"L1",
		"O1",
		"O2",
		"O3",
		"O4",
		"S1",
		"S2",
		"T1",
		"T2"
	};
	Glib::ustring ret;
	for (unsigned int i = 0; i < sizeof(pbnstr)/sizeof(pbnstr[0]); ++i)
		if (pbn & (pbn_t)(1U << i))
			ret += (const char *)pbnstr[i];
	return ret;
}

Aircraft::pbn_t Aircraft::parse_pbn(const Glib::ustring& x)
{
	pbn_t pbn(pbn_none);
	for (Glib::ustring::const_iterator i(x.begin()), e(x.end()); i != e;) {
		char c1(*i++);
		if (i == e)
			break;
		char c2(*i);
		switch (c1) {
		case 'a':
		case 'A':
			if (c2 != '1')
				break;
			++i;
			pbn |= pbn_a1;
			break;

		case 'l':
		case 'L':
			if (c2 != '1')
				break;
			++i;
			pbn |= pbn_l1;
			break;

		case 'b':
		case 'B':
			if (c2 < '1' || c2 > '6')
				break;
			++i;
			pbn |= (pbn_t)(pbn_b1 << (c2 - '1'));
			break;

		case 'c':
		case 'C':
			if (c2 < '1' || c2 > '4')
				break;
			++i;
			pbn |= (pbn_t)(pbn_c1 << (c2 - '1'));
			break;

		case 'd':
		case 'D':
			if (c2 < '1' || c2 > '4')
				break;
			++i;
			pbn |= (pbn_t)(pbn_d1 << (c2 - '1'));
			break;

		case 'o':
		case 'O':
			if (c2 < '1' || c2 > '4')
				break;
			++i;
			pbn |= (pbn_t)(pbn_o1 << (c2 - '1'));
			break;

		case 's':
		case 'S':
			if (c2 < '1' || c2 > '2')
				break;
			++i;
			pbn |= (pbn_t)(pbn_s1 << (c2 - '1'));
			break;

		case 't':
		case 'T':
			if (c2 < '1' || c2 > '2')
				break;
			++i;
			pbn |= (pbn_t)(pbn_t1 << (c2 - '1'));
			break;

		default:
			break;
		}
	}
	return pbn;
}

void Aircraft::pbn_fix_equipment(Glib::ustring& equipment, pbn_t pbn)
{
	equipment = equipment.uppercase();
	{
		Glib::ustring::size_type n(equipment.find('R'));
		if (n == Glib::ustring::npos) {
			if (pbn != pbn_none)
				equipment += 'R';
		} else {
			if (pbn == pbn_none)
				equipment.erase(n, 1);
		}
	}
	if (pbn & pbn_gnss) {
		Glib::ustring::size_type n(equipment.find('G'));
		if (n == Glib::ustring::npos)
			equipment += 'G';
	}
	if (pbn & pbn_dme) {
		Glib::ustring::size_type n(equipment.find('D'));
		if (n == Glib::ustring::npos)
			equipment += 'D';
	}
	if (pbn & pbn_dmedmeiru) {
		Glib::ustring::size_type n(equipment.find('I'));
		if (n == Glib::ustring::npos)
			equipment += 'I';
	}
	if (pbn & pbn_vordme) {
		Glib::ustring::size_type n(equipment.find_first_of("SO"));
		if (n == Glib::ustring::npos)
			equipment += 'O';
	}
}

void Aircraft::pbn_fix_equipment(std::string& equipment, pbn_t pbn)
{
	Glib::ustring e(equipment);
	pbn_fix_equipment(e, pbn);
	equipment = e;
}

Glib::ustring Aircraft::get_gnssflags_string(gnssflags_t gnssflags)
{
	static const char * const gnssflagsstr[] = {		
		"GPS",
		"SBAS",
		"GLONASS",
		"GALILEO",
		"QZSS",
		"BEIDOU",
		"BAROAID",
		0
	};
	Glib::ustring ret;
	for (unsigned int i = 0; gnssflagsstr[i]; ++i) {
		if (!(gnssflags & (gnssflags_t)(1U << i)))
			continue;
		if (!ret.empty())
			ret += ",";
		ret += gnssflagsstr[i];
	}
	return ret;
}

Aircraft::gnssflags_t Aircraft::parse_gnssflags(const Glib::ustring& x)
{
	gnssflags_t gnssflags(gnssflags_none);
	for (Glib::ustring::size_type i(0), n(x.size()); i < n;) {
		Glib::ustring::size_type j(x.find(',', i));
		if (j == Glib::ustring::npos)
			j = n;
		j -= i;
		if (!j) {
			++i;
			continue;
		}
		Glib::ustring xs(x, i, j);
		i += j + 1;
		for (unsigned int s = 0;; ++s) {
			Glib::ustring gs(get_gnssflags_string((gnssflags_t)(1 << s)));
			if (gs.empty())
				break;
			if (gs != xs)
				continue;
			gnssflags |= (gnssflags_t)(1 << s);
			break;
		}
	}
	return gnssflags;
}

double Aircraft::get_useable_fuel(void) const
{
	double d(get_fuelmass());
	if (std::isnan(d) || d <= 0)
		return std::numeric_limits<double>::quiet_NaN();
	return get_useable_fuelmass() / d;
}

double Aircraft::get_total_fuel(void) const
{
	double d(get_fuelmass());
	if (std::isnan(d) || d <= 0)
		return std::numeric_limits<double>::quiet_NaN();
	return get_total_fuelmass() / d;
}

Aircraft::unit_t Aircraft::parse_unit(const std::string& s)
{
	static const struct unittable {
		const char *name;
		unit_t unit;
	} unittable[] = {
		// short names
		{ "m", unit_m },
		{ "cm", unit_cm },
		{ "km", unit_km },
		{ "in", unit_in },
		{ "ft", unit_ft },
		{ "nmi", unit_nmi },
		{ "kg", unit_kg },
		{ "lb", unit_lb },
		{ "l", unit_liter },
		{ "usg", unit_usgal },
		{ "qt", unit_quart },
		// long names
		{ "Liter", unit_liter },
		{ "USGal", unit_usgal },
		{ "Quart", unit_quart },
		// other
		{ "USG", unit_usgal },
		{ "usg", unit_usgal },
		{ "usgal", unit_usgal },
		// end
		{ 0, unit_invalid }
	};
	for (const struct unittable *u = unittable;; ++u)
		if (!u->name || s == u->name)
			return u->unit;
	return unit_invalid;
}

double Aircraft::convert(unit_t fromunit, unit_t tounit, double val,
			 WeightBalance::Element::flags_t flags)
{
	if (std::isnan(val) || fromunit == tounit)
		return val;
	if (((1 << fromunit) & unitmask_length) &&
	    ((1 << tounit) & unitmask_length)) {
		switch (fromunit) {
		case unit_m:
			break;

		case unit_cm:
			val *= 0.01;
			break;

		case unit_km:
			val *= 1000.0;
			break;

		case unit_in:
			val *= 0.0254;
			break;

		case unit_ft:
			val *= Point::ft_to_m_dbl;
			break;

		case unit_nmi:
			val *= 1000.0 / Point::km_to_nmi_dbl;
			break;

		default:
			return std::numeric_limits<double>::quiet_NaN();
		}
		switch (tounit) {
		case unit_m:
			break;

		case unit_cm:
			val *= 100.0;
			break;

		case unit_km:
			val *= 0.001;
			break;

		case unit_in:
			val *= 1.0 / 0.0254;
			break;

		case unit_ft:
			val *= Point::m_to_ft_dbl;
			break;

		case unit_nmi:
			val *= Point::km_to_nmi_dbl / 1000.0;
			break;

		default:
			return std::numeric_limits<double>::quiet_NaN();
		}
		return val;
	}
	if (((1 << fromunit) & (unitmask_mass | unitmask_volume)) &&
	    ((1 << tounit) & (unitmask_mass | unitmask_volume))) {
		switch (fromunit) {
		case unit_kg:
			break;

		case unit_lb:
			val *= lb_to_kg;
			break;

		case unit_liter:
			break;

		case unit_usgal:
			val *= usgal_to_liter;
			break;

		case unit_quart:
			val *= quart_to_liter;
			break;

		default:
			return std::numeric_limits<double>::quiet_NaN();
		}
		flags &= WeightBalance::Element::flag_fuel | WeightBalance::Element::flag_oil;
		if (((1 << fromunit) & unitmask_volume) &&
		    ((1 << tounit) & unitmask_mass)) {
			switch (flags) {
			case WeightBalance::Element::flag_avgas:
				val *= avgas_density;
				break;

			case WeightBalance::Element::flag_jeta:
				val *= jeta_density;
				break;

			case WeightBalance::Element::flag_diesel:
				val *= diesel_density;
				break;

			case WeightBalance::Element::flag_oil:
				val *= oil_density;
				break;

			default:
				return std::numeric_limits<double>::quiet_NaN();
			}
		}
		if (((1 << fromunit) & unitmask_mass) &&
		    ((1 << tounit) & unitmask_volume)) {
			switch (flags) {
			case WeightBalance::Element::flag_avgas:
				val *= 1.0 / avgas_density;
				break;

			case WeightBalance::Element::flag_jeta:
				val *= 1.0 / jeta_density;
				break;

			case WeightBalance::Element::flag_diesel:
				val *= 1.0 / diesel_density;
				break;

			case WeightBalance::Element::flag_oil:
				val *= 1.0 / oil_density;
				break;

			default:
				return std::numeric_limits<double>::quiet_NaN();
			}
		}
		switch (tounit) {
		case unit_kg:
			break;

		case unit_lb:
			val *= kg_to_lb;
			break;

		case unit_liter:
			break;

		case unit_usgal:
			val *= 1.0 / usgal_to_liter;
			break;

		case unit_quart:
			val *= 1.0 / quart_to_liter;
			break;

		default:
			return std::numeric_limits<double>::quiet_NaN();
		}
		return val;
	}
	return std::numeric_limits<double>::quiet_NaN();
}

double Aircraft::convert_fuel(unit_t fromunit, unit_t tounit, double val) const
{
	if (fromunit == tounit)
		return val;
	if ((((1 << fromunit) & unitmask_length) &&
	     ((1 << tounit) & unitmask_length)) ||
	    (((1 << fromunit) & unitmask_mass) &&
	     ((1 << tounit) & unitmask_mass)) ||
	    (((1 << fromunit) & unitmask_volume) &&
	     ((1 << tounit) & unitmask_volume)))
		return convert(fromunit, tounit, val, WeightBalance::Element::flag_none);
	WeightBalance::Element::flags_t flags(get_wb().get_element_flags() & WeightBalance::Element::flag_fuel);
	if (flags == WeightBalance::Element::flag_avgas ||
	    flags == WeightBalance::Element::flag_jeta ||
	    flags == WeightBalance::Element::flag_diesel)
		return convert(fromunit, tounit, val, flags);
	if (((1 << fromunit) & unitmask_volume) &&
	    ((1 << tounit) & unitmask_mass)) {
		val = convert(fromunit, get_fuelunit(), val, WeightBalance::Element::flag_none);
		val *= get_fuelmass();
		val = convert(get_wb().get_massunit(), tounit, val, WeightBalance::Element::flag_none);
		return val;
	}
	if (((1 << fromunit) & unitmask_mass) &&
	    ((1 << tounit) & unitmask_volume)) {
		val = convert(fromunit, get_wb().get_massunit(), val, WeightBalance::Element::flag_none);
		val /= get_fuelmass();
		val = convert(get_fuelunit(), tounit, val, WeightBalance::Element::flag_none);
		return val;
	}
	return convert(fromunit, tounit, val, WeightBalance::Element::flag_none);
}

bool Aircraft::add_auto_units(void)
{
	return get_wb().add_auto_units(get_fuelunit(), unit_quart);
}

const std::string& Aircraft::get_fuelname(void) const
{
	static const std::string avgas("AVGAS");
	static const std::string jeta("Jet-A");
	static const std::string diesel("Diesel");
	static const std::string unknown("Unknown");
	{
		WeightBalance::Element::flags_t flags(get_wb().get_element_flags() & WeightBalance::Element::flag_fuel);
		if (flags == WeightBalance::Element::flag_avgas)
			return avgas;
		if (flags == WeightBalance::Element::flag_jeta)
			return jeta;
		if (flags == WeightBalance::Element::flag_diesel)
			return diesel;
	}
	{
		double density(convert(unit_liter, get_fuelunit(), convert(get_wb().get_massunit(), unit_kg, get_fuelmass(), WeightBalance::Element::flag_none), WeightBalance::Element::flag_none));
		if (!std::isnan(density)) {
			double x(density * (1.0 / avgas_density));
			if (x >= 0.92 && x <= 1.02)
				return avgas;
			x = density * (1.0 / jeta_density);
			if (x >= 0.92 && x <= 1.02)
				return jeta;
		}
	}
	return unknown;
}

Aircraft::ClimbDescent Aircraft::calculate_climb(const std::string& name, double mass, double isaoffs, double qnh) const
{
	if (std::isnan(mass) || mass <= 0)
		mass = get_mtom();
	if (std::isnan(isaoffs))
		isaoffs = 0;
	if (std::isnan(qnh))
		qnh = IcaoAtmosphere<double>::std_sealevel_pressure;
	return get_climb().calculate(name, mass, isaoffs, qnh);
}

Aircraft::ClimbDescent Aircraft::calculate_descent(const std::string& name, double mass, double isaoffs, double qnh) const
{
	if (std::isnan(mass) || mass <= 0) {
		mass = get_mlm();
		if (std::isnan(mass) || mass <= 0)
			mass = get_mtom();
	}
	if (std::isnan(isaoffs))
		isaoffs = 0;
	if (std::isnan(qnh))
		qnh = IcaoAtmosphere<double>::std_sealevel_pressure;
	return get_descent().calculate(name, mass, isaoffs, qnh);
}

Aircraft::ClimbDescent Aircraft::calculate_climb(Cruise::EngineParams& ep, double mass, double isaoffs, double qnh) const
{
	if (std::isnan(mass) || mass <= 0)
		mass = get_mtom();
	if (std::isnan(isaoffs))
		isaoffs = 0;
	if (std::isnan(qnh))
		qnh = IcaoAtmosphere<double>::std_sealevel_pressure;
	ClimbDescent cd(get_climb().calculate(ep.get_climbname(), ep.get_name(), mass, isaoffs, qnh));
	ep.set_climbname(cd.get_name());
	return cd;
}

Aircraft::ClimbDescent Aircraft::calculate_descent(Cruise::EngineParams& ep, double mass, double isaoffs, double qnh) const
{
	if (std::isnan(mass) || mass <= 0) {
		mass = get_mlm();
		if (std::isnan(mass) || mass <= 0)
			mass = get_mtom();
	}
	if (std::isnan(isaoffs))
		isaoffs = 0;
	if (std::isnan(qnh))
		qnh = IcaoAtmosphere<double>::std_sealevel_pressure;
	ClimbDescent cd(get_descent().calculate(ep.get_descentname(), ep.get_name(), mass, isaoffs, qnh));
	ep.set_descentname(cd.get_name());
	return cd;
}

void Aircraft::calculate_cruise(double& tas, double& fuelflow, double& pa, double& mass, double& isaoffs, double& qnh, Cruise::CruiseEngineParams& ep) const
{
	static const bool debug(false);
	if (debug)
		ep.print(std::cerr << "calculate_cruise: >> pa " << pa << " mass " << mass << " isaoffs " << isaoffs << " qnh " << qnh << " ep ") << std::endl;
	if (ep.get_name().empty() && !(ep.get_flags() & Cruise::Curve::flags_hold) &&
	    (get_propulsion() == propulsion_constantspeed || get_propulsion() == propulsion_fixedpitch)) {
		if (std::isnan(is_constantspeed() ? ep.get_mp() : ep.get_rpm())) {
			if (std::isnan(ep.get_bhp()))
				ep.set_bhp(0.65 * get_maxbhp());
			if (is_constantspeed() && std::isnan(ep.get_rpm()))
				ep.set_rpm(2400);
		}
	}
	if (std::isnan(mass) || mass <= 0)
		mass = get_mtom();
	if (std::isnan(isaoffs))
		isaoffs = 0;
	if (std::isnan(qnh))
		qnh = IcaoAtmosphere<double>::std_sealevel_pressure;
	get_cruise().calculate(get_propulsion(), tas, fuelflow, pa, mass, isaoffs, ep);
	if (debug)
		ep.print(std::cerr << "calculate_cruise: << tas " << tas << " ff " << fuelflow << " pa " << pa << " mass " << mass << " isaoffs " << isaoffs << " qnh " << qnh << " ep ") << std::endl;
}

time_t Aircraft::get_opsrules_holdtime(opsrules_t r, propulsion_t p)
{
	switch (r) {
	case opsrules_easacat:
		// 30min/45min holding at alternate aerodrome altitude + 1500'
		// ICAO Annex 6 Part I
		switch (p) {
		case Aircraft::propulsion_fixedpitch:
		case Aircraft::propulsion_constantspeed:
			return 45 * 60;

		default:
			return 30 * 60;
		}
		break;

	default:
	case opsrules_easanco:
		// 45min holding at "normal cruise altitude"
		// ICAO Annex 6 Part II
		return 45 * 60;
	}
}

bool Aircraft::is_opsrules_holdcruisealt(opsrules_t r)
{
	switch (r) {
	default:
	case opsrules_easacat:
		// 30min/45min holding at alternate aerodrome altitude + 1500'
		// ICAO Annex 6 Part I
		return false;

	case opsrules_easanco:
		// 45min holding at "normal cruise altitude"
		// ICAO Annex 6 Part II
		return true;
	}
}

Aircraft::CheckErrors Aircraft::check(void) const
{
	double minmass(get_wb().get_min_mass());
	CheckErrors r(m_climb.check(minmass, get_mtom()));
	r.add(m_descent.check(minmass, get_mtom()));
	r.add(m_cruise.check(minmass, get_mtom()));
	return r;
}

Aircraft::Endurance::Endurance(void)
	: m_dist(std::numeric_limits<double>::quiet_NaN()),
	  m_fuel(std::numeric_limits<double>::quiet_NaN()),
	  m_tas(std::numeric_limits<double>::quiet_NaN()),
	  m_fuelflow(std::numeric_limits<double>::quiet_NaN()),
	  m_pa(std::numeric_limits<double>::quiet_NaN()),
	  m_tkoffelev(std::numeric_limits<double>::quiet_NaN()),
	  m_endurance(0)
{
}

class Aircraft::EnduranceW : public Aircraft::Endurance {
public:
	void set_engineparams(const Cruise::CruiseEngineParams& ep) { m_ep = ep; }
	void set_dist(double x) { m_dist = x; }
	void set_fuel(double x) { m_fuel = x; }
	void set_tas(double x) { m_tas = x; }
	void set_fuelflow(double x) { m_fuelflow = x; }
	void set_pa(double x) { m_pa = x; }
	void set_tkoffelev(double x) { m_tkoffelev = x; }
	void set_endurance(time_t x) { m_endurance = x; }
	void add_dist(double x) { m_dist += x; }
	void add_endurance(time_t x) { m_endurance += x; }
};

Aircraft::Endurance Aircraft::compute_endurance(double pa, const Cruise::CruiseEngineParams& ep, double tkoffelev)
{
	EnduranceW e;
	double fuel(get_useable_fuel());
	e.set_fuel(fuel);
	if (std::isnan(fuel) || fuel <= 0)
		return e;
	if (std::isnan(pa))
		return e;
	if (pa < 0)
		pa = 0;
	double mass(get_mtom()), isaoffs(0), qnh(IcaoAtmosphere<double>::std_sealevel_pressure);
	ClimbDescent climb(calculate_climb("", mass, isaoffs, qnh));
	if (pa > climb.get_ceiling())
		pa = climb.get_ceiling();
	double tas(0), fuelflow(0);
	Cruise::CruiseEngineParams ep1(ep);
	calculate_cruise(tas, fuelflow, pa, mass, isaoffs, qnh, ep1);
	e.set_tas(tas);
	e.set_fuelflow(fuelflow);
	e.set_engineparams(ep1);
	e.set_pa(pa);
	if (std::isnan(pa) || std::isnan(fuelflow) || fuelflow <= 0)
		return e;
	e.set_dist(0);
	e.set_endurance(0);
	if (!std::isnan(tkoffelev)) {
		double t0(climb.altitude_to_time(tkoffelev));
		double t1(climb.altitude_to_time(pa));
		if (!std::isnan(t0) && !std::isnan(t1) && t1 >= t0) {
			e.set_tkoffelev(climb.time_to_altitude(t0));
			e.add_dist(climb.time_to_distance(t1) - climb.time_to_distance(t0));
			e.add_endurance(Point::round<time_t,double>(t1 - t0));
			fuel -= climb.time_to_fuel(t1) - climb.time_to_fuel(t0);
		}
	}
	{
		double t(fuel / fuelflow);
		e.add_endurance(Point::round<time_t,double>(t * 3600.0));
		e.add_dist(t * tas);
	}
	return e;
}


// Aircraft Type Classification "Database", ICAO Doc 8643
// First character:
// L = Landplane
// S = Seaplane
// A = Amphibian
// H = Helicopter
// G = Gyrocopter
// T = Tilt-wing aircraft
// Second character:
// 1,2,3,4,6, 8 = the number of engines
// C = 2 coupled engines driving 1 propeller
// Third character:
// P = Piston Engine
// T = Turbo Engine
// J = Jet Engine
// Fourth Character:
// Approach Category
// Fifth Character:
// Wake Turbulence Category

// get this data out of Achim's icao8653.db:
// select substr(icao||'    ',1,4)||substr(description,1,1)||enginecount||substr(enginetype,1,1)||'-'||wakecategory from types order by icao;


const Aircraft::aircraft_type_class_t Aircraft::aircraft_type_class_db[] = {
	"A002G1P-L",
	"A1  L1P-M",
	"A10 L2J-M",
	"A109H2T-L",
	"A119H1T-L",
	"A122L1P-L",
	"A124L4J-H",
	"A129H2T-L",
	"A139H2T-L",
	"A140L2T-M",
	"A148L2J-M",
	"A149H2T-M",
	"A158L2J-M",
	"A16 L1P-L",
	"A169H2T-L",
	"A189H2T-M",
	"A19 L1P-L",
	"A20 L2P-M",
	"A205G1P-L",
	"A21 L1P-L",
	"A210L1P-L",
	"A211L1P-L",
	"A22 L1P-L",
	"A223L1P-L",
	"A225L6J-H",
	"A23 L1P-L",
	"A25 A1P-L",
	"A251A2P-L",
	"A27 L1P-L",
	"A270L1T-L",
	"A29 L1P-L",
	"A2RTH2T-L",
	"A3  L2J-M",
	"A306L2J-H",
	"A30BL2J-H",
	"A31 L1P-L",
	"A310L2J-H",
	"A318L2JCM",
	"A319L2JCM",
	"A320L2JCM",
	"A321L2JCM",
	"A33 L1P-L",
	"A332L2JCH",
	"A333L2JCH",
	"A342L4JCH",
	"A343L4JCH",
	"A345L4JCH",
	"A346L4JCH",
	"A35 L1P-L",
	"A358L2J-H",
	"A359L2J-H",
	"A35KL2J-H",
	"A37 L2J-L",
	"A388L4JCH",
	"A3STL2J-H",
	"A4  L1J-M",
	"A400L4T-H",
	"A5  A1P-L",
	"A50 L4J-H",
	"A500L2P-L",
	"A504L1P-L",
	"A6  L2J-M",
	"A600H1P-L",
	"A660L1T-L",
	"A7  L1J-M",
	"A700L2J-L",
	"A743L2J-M",
	"A748L2T-M",
	"A890L1P-L",
	"A9  L1P-L",
	"A900L1P-L",
	"A910L1P-L",
	"AA1 L1P-L",
	"AA37L2P-L",
	"AA5 L1P-L",
	"AAT3L1P-L",
	"AAT4L1P-L",
	"AB11L1P-L",
	"AB15L1P-L",
	"AB18L1P-L",
	"AB95L1P-L",
	"AC10G1P-L",
	"AC11L1P-L",
	"AC4 L1P-L",
	"AC50L2P-L",
	"AC52L2P-L",
	"AC56L2P-L",
	"AC5AL1P-L",
	"AC5ML1P-L",
	"AC68L2P-L",
	"AC6LL2P-L",
	"AC72L2P-L",
	"AC80L2T-L",
	"AC90L2T-L",
	"AC95L2T-L",
	"ACAML2P-L",
	"ACARL1P-L",
	"ACEDL1P-L",
	"ACJRA1P-L",
	"ACPLL1P-L",
	"ACR2L1P-L",
	"ACRDL2P-L",
	"ACROL1P-L",
	"ACSRL1P-L",
	"AD20L1P-L",
	"ADELG1P-L",
	"ADVEL1P-L",
	"ADVNA1P-L",
	"AE45L2P-L",
	"AEA1L1P-L",
	"AESTL2P-L",
	"AFOXL1P-L",
	"AG02L1P-L",
	"AGSHA1P-L",
	"AI10L1P-L",
	"AIGTL1P-L",
	"AIRDL1P-L",
	"AIRLL2P-L",
	"AJ27L2J-M",
	"AJETL2J-M",
	"AK1 L1P-L",
	"AKOYA1P-L",
	"AKROL1P-L",
	"ALBUL1P-L",
	"ALC1L1P-L",
	"ALGRL1P-L",
	"ALH H2T-L",
	"ALIGL1P-L",
	"ALIZL1T-M",
	"ALO2H1T-L",
	"ALO3H1T-L",
	"ALPIL1P-L",
	"ALSLL1P-L",
	"ALTOL1P-L",
	"AM3 L1P-L",
	"AMX L1J-M",
	"AN12L4T-M",
	"AN2 L1PAL",
	"AN22L4T-H",
	"AN24L2T-M",
	"AN26L2T-M",
	"AN28L2T-L",
	"AN3 L1T-L",
	"AN30L2T-M",
	"AN32L2T-M",
	"AN38L2T-M",
	"AN70L4T-M",
	"AN72L2J-M",
	"AN8 L2T-M",
	"ANGLL2P-L",
	"ANSNL2P-L",
	"ANSTH2T-L",
	"AP20L1P-L",
	"AP22L1P-L",
	"AP24A1P-L",
	"AP26L2P-L",
	"AP28L2P-L",
	"AP36L2P-L",
	"APM2L1P-L",
	"APM3L1P-L",
	"APM4L1P-L",
	"APUPL1P-L",
	"AR11L1P-L",
	"AR15L1P-L",
	"AR79L1P-L",
	"ARCEL1E-L",
	"ARCPL1P-L",
	"ARESL1J-L",
	"ARKSL1P-L",
	"ARONL1P-L",
	"ARV1L1P-L",
	"ARVAL2T-L",
	"ARWFL1P-L",
	"AS02L1P-L",
	"AS14L1P-L",
	"AS16L1P-L",
	"AS20L1P-L",
	"AS21L1P-L",
	"AS22L1P-L",
	"AS24L1P-L",
	"AS25L1P-L",
	"AS26L1P-L",
	"AS28L1P-L",
	"AS29L1P-L",
	"AS2TL1T-L",
	"AS30L1P-L",
	"AS31L1P-L",
	"AS32H2T-M",
	"AS3BH2T-M",
	"AS50H1T-L",
	"AS55H2T-L",
	"AS65H2T-L",
	"AS80L1P-L",
	"ASO4L1P-L",
	"ASTOL1P-L",
	"ASTRL2J-M",
	"AT2PL1P-L",
	"AT3 L2J-M",
	"AT3PL1P-L",
	"AT3TL1T-L",
	"AT43L2T-M",
	"AT44L2T-M",
	"AT45L2T-M",
	"AT46L2T-M",
	"AT5PL1P-L",
	"AT5TL1T-L",
	"AT6TL1T-L",
	"AT72L2T-M",
	"AT73L2T-M",
	"AT75L2T-M",
	"AT76L2T-M",
	"AT8TL1T-L",
	"ATACL1P-L",
	"ATG1L2J-L",
	"ATISL1P-L",
	"ATL L1P-L",
	"ATLAL2T-M",
	"ATP L2T-M",
	"ATTLL1P-L",
	"AU11L1P-L",
	"AUJ2L1P-L",
	"AUJ4L1P-L",
	"AUS3L1P-L",
	"AUS4L1P-L",
	"AUS5L1P-L",
	"AUS6L1P-L",
	"AUS7L1P-L",
	"AUS9L1P-L",
	"AV68L1P-L",
	"AVAMA1P-L",
	"AVIDL1P-L",
	"AVINL1P-L",
	"AVK4L2T-L",
	"AVLNA1T-L",
	"AVTRA1P-L",
	"B06 H1T-L",
	"B06TH2T-L",
	"B1  L4J-H",
	"B103A2P-L",
	"B105H2T-L",
	"B13 L1P-L",
	"B14AL1P-L",
	"B14BL1P-L",
	"B14CL1P-L",
	"B150H1T-L",
	"B17 L4P-M",
	"B18TL2T-L",
	"B190L2T-M",
	"B2  L4J-H",
	"B209L1P-L",
	"B212H2T-L",
	"B214H1T-M",
	"B222H2T-L",
	"B23 L2P-M",
	"B230H2T-L",
	"B24 L4P-M",
	"B25 L2P-M",
	"B26 L2P-M",
	"B26ML2P-M",
	"B29 L4P-M",
	"B305H1P-L",
	"B350L2T-L",
	"B360L1P-L",
	"B36TL1T-L",
	"B407H1T-L",
	"B412H2T-L",
	"B427H2T-L",
	"B429H2T-L",
	"B430H2T-L",
	"B461L4JCM",
	"B462L4JCM",
	"B463L4JCM",
	"B47GH1P-L",
	"B47JH1P-L",
	"B47TH1T-L",
	"B52 L8J-H",
	"B58TL2P-L",
	"B60 L1P-L",
	"B609T2T-M",
	"B60TL2T-L",
	"B701L4J-M",
	"B703L4J-H",
	"B712L2J-M",
	"B720L4J-M",
	"B721L3J-M",
	"B722L3J-M",
	"B731L2JCM",
	"B732L2JCM",
	"B733L2JCM",
	"B734L2JCM",
	"B735L2JCM",
	"B736L2JCM",
	"B737L2JCM",
	"B738L2JCM",
	"B739L2JCM",
	"B741L4JDH",
	"B742L4JDH",
	"B743L4JDH",
	"B744L4JDH",
	"B748L4JCH",
	"B74DL4J-H",
	"B74RL4J-H",
	"B74SL4J-H",
	"B752L2JCM",
	"B753L2JCM",
	"B762L2JDH",
	"B763L2JDH",
	"B764L2JDH",
	"B772L2J-H",
	"B773L2J-H",
	"B77LL2J-H",
	"B77WL2J-H",
	"B788L2J-H",
	"B789L2J-H",
	"B78XL2J-H",
	"BA11L2J-M",
	"BABYH1P-L",
	"BAR6L1P-L",
	"BARCL1P-L",
	"BASSL2P-L",
	"BBATL1P-L",
	"BBIRL1P-L",
	"BCA3L1P-L",
	"BCATL1P-L",
	"BCS1L2J-M",
	"BCS3L2J-M",
	"BD10L1J-L",
	"BD12L1P-L",
	"BD17L1P-L",
	"BD4 L1P-L",
	"BD5 L1P-L",
	"BD5JL1J-L",
	"BD5TL1T-L",
	"BDOGL1P-L",
	"BE10L2T-L",
	"BE12A2T-M",
	"BE17L1P-L",
	"BE18L2P-L",
	"BE19L1P-L",
	"BE20L2T-L",
	"BE23L1P-L",
	"BE24L1P-L",
	"BE30L2T-L",
	"BE32L2T-M",
	"BE33L1P-L",
	"BE35L1P-L",
	"BE36L1P-L",
	"BE40L2JBM",
	"BE45L2J-M",
	"BE50L2P-L",
	"BE55L2P-L",
	"BE56L2P-L",
	"BE58L2P-L",
	"BE60L2P-L",
	"BE65L2P-L",
	"BE70L2P-L",
	"BE76L2P-L",
	"BE77L1P-L",
	"BE80L2P-L",
	"BE88L2P-L",
	"BE95L2P-L",
	"BE99L2T-L",
	"BE9LL2T-L",
	"BE9TL2T-L",
	"BEARL1P-L",
	"BELFL4T-M",
	"BER2A2J-M",
	"BER4A2J-M",
	"BETAL1P-L",
	"BEVRL1P-L",
	"BF19L1P-L",
	"BFITL1P-L",
	"BILOL1P-L",
	"BIPLL1P-L",
	"BIRDL1P-L",
	"BISCL1P-L",
	"BK17H2T-L",
	"BKUTL1P-L",
	"BL11L1P-L",
	"BL17L1P-L",
	"BL19L1P-L",
	"BL8 L1P-L",
	"BLBUL1P-L",
	"BLCFL4J-H",
	"BLENL2P-L",
	"BLKSL1P-L",
	"BM6 L1P-L",
	"BMANL1P-L",
	"BN2PL2P-L",
	"BN2TL2T-L",
	"BOLTL1P-L",
	"BOOML2P-L",
	"BPATL1P-L",
	"BPODL4E-L",
	"BR14L1P-L",
	"BR54G1P-L",
	"BR60L1P-L",
	"BR61L1P-L",
	"BRAVL1P-L",
	"BRB2H1P-L",
	"BREZL1P-L",
	"BROUL1P-L",
	"BS60L2T-L",
	"BSCAL4J-H",
	"BSTPH2T-M",
	"BSTRL1P-L",
	"BT36L1P-L",
	"BTUBL1P-L",
	"BU20L3P-L",
	"BU31L1P-L",
	"BU33L1P-L",
	"BU81L1P-L",
	"BUC L2J-M",
	"BUCAA1P-L",
	"BULTL1P-L",
	"BX2 L1P-L",
	"C02TL2T-L",
	"C04TL2T-L",
	"C06TL1T-L",
	"C07TL1T-L",
	"C1  L2J-M",
	"C101L1J-L",
	"C10TL1T-L",
	"C119L2P-M",
	"C120L1P-L",
	"C123L2P-M",
	"C125L3P-M",
	"C130L4T-M",
	"C133L4T-M",
	"C135L4J-H",
	"C140L1P-L",
	"C141L4J-H",
	"C14TL2T-L",
	"C15 L4J-M",
	"C150L1PAL",
	"C152L1PAL",
	"C160L2T-M",
	"C162L1P-L",
	"C17 L4JBH",
	"C170L1PAL",
	"C172L1PAL",
	"C175L1P-L",
	"C177L1PAL",
	"C180L1PAL",
	"C182L1PAL",
	"C185L1P-L",
	"C188L1P-L",
	"C190L1P-L",
	"C195L1P-L",
	"C2  L2T-M",
	"C205L1P-L",
	"C206L1P-L",
	"C207L1P-L",
	"C208L1T-L",
	"C210L1P-L",
	"C212L2T-M",
	"C21TL2T-L",
	"C22JL2J-L",
	"C240L1P-L",
	"C25AL2JBL",
	"C25BL2JBL",
	"C25CL2J-M",
	"C270L1P-L",
	"C27JL2T-M",
	"C295L2T-M",
	"C303L2P-L",
	"C309L1P-L",
	"C30JL4T-M",
	"C310L2P-L",
	"C320L2P-L",
	"C335L2P-L",
	"C336L2P-L",
	"C337L2P-L",
	"C340L2P-L",
	"C365L1T-L",
	"C402L2P-L",
	"C404L2P-L",
	"C411L2P-L",
	"C414L2P-L",
	"C42 L1P-L",
	"C421L2P-L",
	"C425L2T-L",
	"C441L2T-L",
	"C46 L2P-M",
	"C5  L4JCH",
	"C500L2J-L",
	"C501L2JBL",
	"C510L2JBL",
	"C525L2JBL",
	"C526L2J-L",
	"C550L2JBL",
	"C551L2JBL",
	"C55BL2J-L",
	"C560L2JBM",
	"C56XL2JBM",
	"C5M L4J-H",
	"C650L2JBM",
	"C680L2JBM",
	"C72RL1PAL",
	"C750L2J-M",
	"C77RL1PAL",
	"C82 L2P-M",
	"C82RL1PAL",
	"C82SL1PAL",
	"C82TL1PAL",
	"C97 L4P-M",
	"CA12L1T-L",
	"CA19L1P-L",
	"CA1PL1P-L",
	"CA1TL1T-L",
	"CA25L1P-L",
	"CA3 L1P-L",
	"CA4 L1P-L",
	"CA41L1P-L",
	"CA6 L1P-L",
	"CA61L1P-L",
	"CA65L1P-L",
	"CA7PL1P-L",
	"CA7TL1T-L",
	"CA8 L1T-L",
	"CA9 L1T-L",
	"CABIL1T-L",
	"CABNL1P-L",
	"CAD2L1P-L",
	"CAD4L1P-L",
	"CAJ L1J-L",
	"CAMLL1P-L",
	"CAMPL1P-L",
	"CAN4L1P-L",
	"CAPLL1P-L",
	"CAR L1P-L",
	"CARVL4P-M",
	"CASSL1P-L",
	"CAT A2P-M",
	"CAT1L1P-L",
	"CAT2L2P-L",
	"CAW L1P-L",
	"CB1 L1P-L",
	"CD2 A2T-L",
	"CDC6L1P-L",
	"CDUSG1P-L",
	"CDW1L1P-L",
	"CE15S1P-L",
	"CE22A2P-L",
	"CE23A1P-L",
	"CE25A2P-L",
	"CE27A2P-L",
	"CE43L1P-L",
	"CEGLL1P-L",
	"CELRL1P-L",
	"CENTL1P-L",
	"CFREA1P-L",
	"CG3 L1P-L",
	"CGANS1P-L",
	"CH1 L1T-L",
	"CH10L1P-L",
	"CH12H1P-L",
	"CH14H1T-L",
	"CH15L1P-L",
	"CH18L1P-L",
	"CH20L1P-L",
	"CH25L1P-L",
	"CH2TL1P-L",
	"CH30L1P-L",
	"CH40L2P-L",
	"CH50L1P-L",
	"CH60L1P-L",
	"CH62L2P-L",
	"CH64L1P-L",
	"CH65L1P-L",
	"CH7 H1P-L",
	"CH70L1P-L",
	"CH75L1P-L",
	"CH7AL1P-L",
	"CH7BL1P-L",
	"CH80L1P-L",
	"CHANL1P-L",
	"CHCSL1P-L",
	"CHGOL1P-L",
	"CHICL1P-L",
	"CHIFH1P-L",
	"CHINL1P-L",
	"CHIPL1P-L",
	"CHR1L1P-L",
	"CHR4L1P-L",
	"CHSYG1P-L",
	"CICAL2P-L",
	"CJ1 L1P-L",
	"CJ6 L1P-L",
	"CKUOL2J-M",
	"CL2PA2P-M",
	"CL2TA2T-M",
	"CL30L2JCM",
	"CL35L2J-M",
	"CL41L1J-L",
	"CL44L4T-M",
	"CL4GL4T-M",
	"CL60L2JCM",
	"CL8 L1P-L",
	"CLA L1P-L",
	"CLB1L1P-L",
	"CLBRL1P-L",
	"CLD2G1P-L",
	"CLDSL1P-L",
	"CLONG1P-L",
	"CMA3L1P-L",
	"CMASL1P-L",
	"CMD1G1P-L",
	"CMDEG1P-L",
	"CMDTG1P-L",
	"CN12L1P-L",
	"CN35L2T-M",
	"CNBRL2J-M",
	"CNDRL2T-L",
	"CNGPL1P-L",
	"CNUKL1P-L",
	"COARL1P-L",
	"COBRL1P-L",
	"COL3L1P-L",
	"COL4L1P-L",
	"COMUH1P-L",
	"CONIL4P-M",
	"COOTA1P-L",
	"COROL1P-L",
	"CORRL1P-L",
	"CORSL1P-M",
	"CORVL2P-L",
	"COUGL1P-L",
	"COURL1P-L",
	"COY2L1P-L",
	"COZYL1P-L",
	"CP10L1P-L",
	"CP13L1P-L",
	"CP20L1P-L",
	"CP21L1P-L",
	"CP22L1P-L",
	"CP23L1P-L",
	"CP30L1P-L",
	"CP32L1P-L",
	"CP60L1P-L",
	"CP65L1P-L",
	"CP75L1P-L",
	"CP80L1P-L",
	"CP90L1P-L",
	"CPNAL1P-L",
	"CPUPL1P-L",
	"CR10L1P-L",
	"CRA1L1P-L",
	"CRACL1P-L",
	"CRERL1P-L",
	"CRESL1T-L",
	"CRIOA1P-L",
	"CRJ1L2JCM",
	"CRJ2L2JCM",
	"CRJ7L2JCM",
	"CRJ9L2JCM",
	"CRJXL2J-M",
	"CRUZL1P-L",
	"CT4 L1P-L",
	"CTAHL1P-L",
	"CTLNA1P-L",
	"CUB2L1P-L",
	"CUCAL1P-L",
	"CULPL1P-L",
	"CULVL1P-L",
	"CULXL2P-L",
	"CVLPL2P-M",
	"CVLTL2T-M",
	"CX5 L1P-L",
	"CYCLL1P-L",
	"CYGTL1P-L",
	"D1  L2P-L",
	"D11 L1P-L",
	"D139L1P-L",
	"D140L1P-L",
	"D150L1P-L",
	"D18 L1P-L",
	"D201L1P-L",
	"D228L2T-L",
	"D25 L1P-L",
	"D250L1P-L",
	"D253L1P-L",
	"D28DL2P-L",
	"D28TL2T-L",
	"D31 L1P-L",
	"D328L2TBM",
	"D39 L1P-L",
	"D4  L1P-L",
	"D5  L1P-L",
	"D5TUL1P-L",
	"D6  L1P-L",
	"D6CRL1P-L",
	"D6SLL1P-L",
	"D7  L1P-L",
	"D8  L1P-L",
	"DA2 L1P-L",
	"DA36L1E-L",
	"DA40L1P-L",
	"DA42L2P-L",
	"DA5 L1P-L",
	"DA50L1P-L",
	"DAHUL1P-L",
	"DAKHL1P-L",
	"DAL1L1P-L",
	"DAL4L1P-L",
	"DAL5L1P-L",
	"DARTL1P-L",
	"DC10L3J-H",
	"DC2 L2P-M",
	"DC3 L2P-M",
	"DC3SL2P-M",
	"DC3TL2T-M",
	"DC4 L4P-M",
	"DC6 L4P-M",
	"DC7 L4P-M",
	"DC85L4J-H",
	"DC86L4J-H",
	"DC87L4J-H",
	"DC91L2J-M",
	"DC92L2J-M",
	"DC93L2J-M",
	"DC94L2J-M",
	"DC95L2J-M",
	"DEAGG1P-L",
	"DEFIL2P-L",
	"DELFL1P-L",
	"DFL6L1P-L",
	"DFLYL1P-L",
	"DG15L1P-L",
	"DG1TL1P-L",
	"DG40L1P-L",
	"DG50L1P-L",
	"DG60L1P-L",
	"DG80L1P-L",
	"DH2TL1T-L",
	"DH3TL1T-L",
	"DH60L1P-L",
	"DH80L1P-L",
	"DH82L1P-L",
	"DH83L1P-L",
	"DH85L1P-L",
	"DH87L1P-L",
	"DH88L2P-L",
	"DH89L2P-L",
	"DH8AL2T-M",
	"DH8BL2T-M",
	"DH8CL2T-M",
	"DH8DL2TCM",
	"DH90L2P-L",
	"DH94L1P-L",
	"DHA3L3P-L",
	"DHC1L1P-L",
	"DHC2L1P-L",
	"DHC3L1P-L",
	"DHC4L2P-M",
	"DHC5L2T-M",
	"DHC6L2T-L",
	"DHC7L4T-M",
	"DIESL1P-L",
	"DIJ3L1P-L",
	"DIJ4A1P-L",
	"DIMOL1P-L",
	"DINOL1P-L",
	"DIPRA1P-L",
	"DISCL1P-L",
	"DJETL1J-L",
	"DJINH1T-L",
	"DLH2L1E-L",
	"DLTAL1P-L",
	"DNGOA1T-L",
	"DO27L1P-L",
	"DO28L2P-L",
	"DOCXL1P-L",
	"DON L1P-L",
	"DOVEL2P-L",
	"DR1 L1P-L",
	"DR10L1P-L",
	"DR22L1P-L",
	"DR30L1P-L",
	"DR40L1P-L",
	"DRAGH1P-L",
	"DRIFL1P-L",
	"DRTGL1P-L",
	"DSA1L1P-L",
	"DSK L1P-L",
	"DSLKL1P-L",
	"DUB2L1P-L",
	"DUCEL1P-L",
	"DUODL1P-L",
	"DUR5L1P-L",
	"DV1 L1P-L",
	"DV2 L1P-L",
	"DV20L1P-L",
	"DW1 L1P-L",
	"DWD2L1P-L",
	"DYH2H1P-L",
	"E110L2T-L",
	"E120L2T-M",
	"E121L2T-L",
	"E135L2JCM",
	"E145L2JCM",
	"E170L2JCM",
	"E190L2JCM",
	"E2  L2T-M",
	"E200L1P-L",
	"E230L1P-L",
	"E2CBL1P-L",
	"E300L1P-L",
	"E314L1T-L",
	"E35LL2J-M",
	"E3CFL4J-H",
	"E3TFL4J-H",
	"E400L1P-L",
	"E45XL2J-M",
	"E500L1T-L",
	"E50PL2JBM",
	"E545L2J-M",
	"E550L2J-M",
	"E55PL2JBM",
	"E6  L4J-H",
	"E737L2J-M",
	"E767L2J-H",
	"E7BHL1P-L",
	"EA40L1J-L",
	"EA50L2J-L",
	"EAEAL1P-L",
	"EAGLL1P-L",
	"EAGTL1P-L",
	"EAGXL1P-L",
	"EC20H1T-L",
	"EC25H2T-M",
	"EC30H1T-L",
	"EC35H2T-L",
	"EC45H2T-L",
	"EC55H2T-L",
	"EC6 L1P-L",
	"EC75H2T-L",
	"ECHOL1P-L",
	"EDGEL1P-L",
	"EDGTL1P-L",
	"EFOXL1P-L",
	"EGL3H1T-L",
	"EGRTL1T-L",
	"EH10H3T-M",
	"EL20L1P-L",
	"ELA7G1P-L",
	"ELF L1P-L",
	"ELITL2J-L",
	"ELPSL1P-L",
	"ELSTL1P-L",
	"ELTOH1P-L",
	"ELTRL1P-L",
	"EM10L1J-L",
	"EM11L2P-L",
	"EN28H1P-L",
	"EN48H1T-L",
	"EP9 L1P-L",
	"EPERL1P-L",
	"EPICL1T-L",
	"EPX1L1P-L",
	"ERACL1P-L",
	"ERCOL1P-L",
	"ES11H1T-L",
	"ES13L1P-L",
	"ESCAL1T-L",
	"ESCPL1P-L",
	"ESQLL1P-L",
	"ETARL1J-M",
	"EUFIL2J-M",
	"EUPAL1P-L",
	"EURTL1P-L",
	"EV55L2P-L",
	"EV97L1P-L",
	"EVANL2P-L",
	"EVICL1J-L",
	"EVOPL1P-L",
	"EVOTL1P-L",
	"EVSSL1P-L",
	"EX5TL1T-L",
	"EXECH1P-L",
	"EXEJH1T-L",
	"EXPLH2T-L",
	"EXPRL1P-L",
	"EZFLL1P-L",
	"EZFTL2P-L",
	"EZHVL1P-L",
	"EZIKL1P-L",
	"EZKCL1P-L",
	"F1  L2J-M",
	"F100L2JCM",
	"F104L1J-M",
	"F106L1J-M",
	"F111L2J-M",
	"F117L2J-M",
	"F14 L2J-M",
	"F15 L2J-M",
	"F156L1P-L",
	"F16 L1J-M",
	"F16XL1J-M",
	"F18 L2J-M",
	"F1FVL1P-L",
	"F2  L1J-M",
	"F22 L2J-M",
	"F260L1P-L",
	"F26TL1T-L",
	"F27 L2T-M",
	"F28 L2J-M",
	"F2THL2JCM",
	"F35 L1J-M",
	"F3F L1P-L",
	"F4  L2J-M",
	"F406L2T-L",
	"F41EL2P-L",
	"F5  L2J-M",
	"F50 L2T-M",
	"F5SAL2J-M",
	"F60 L2T-M",
	"F600L2T-L",
	"F70 L2JBM",
	"F8  L1J-M",
	"F86 L1J-M",
	"F8L L1P-L",
	"F900L3JBM",
	"F9F L1J-L",
	"FA01L1P-L",
	"FA02L1P-L",
	"FA03L1P-L",
	"FA04L1P-L",
	"FA10L2J-M",
	"FA11L1P-L",
	"FA20L2JBM",
	"FA24L1P-L",
	"FA50L3JCM",
	"FA5XL2J-M",
	"FA62L1P-L",
	"FA7XL3J-M",
	"FA8XL3J-M",
	"FAETL1P-L",
	"FALCL1P-L",
	"FALML1P-L",
	"FANLL1P-L",
	"FANTL1P-L",
	"FB1AL1P-L",
	"FB1BL1P-L",
	"FB5 L1P-L",
	"FBA2L1P-L",
	"FBIRL1P-L",
	"FC1 L1J-M",
	"FDCTL1P-L",
	"FDMCL1P-L",
	"FE51L1P-L",
	"FESTL1P-L",
	"FFLYL1P-L",
	"FG01L1P-L",
	"FGT L1P-L",
	"FH11H1T-L",
	"FIBOL1P-L",
	"FIKDL1P-L",
	"FIKEL1P-L",
	"FINCL1P-L",
	"FJ10L2J-L",
	"FJR3L1P-L",
	"FK12L1P-L",
	"FK14L1P-L",
	"FK9 L1P-L",
	"FL3 L1P-L",
	"FL53L1P-L",
	"FL54L1P-L",
	"FL55L1P-L",
	"FLAML2P-L",
	"FLE2L1P-L",
	"FLE7L1P-L",
	"FLIZL1P-L",
	"FLSHL1P-L",
	"FLSSL1P-L",
	"FM25L1P-L",
	"FN33A1P-L",
	"FNKBL1P-L",
	"FOOFL1P-L",
	"FORTL1P-L",
	"FOUGL2J-L",
	"FOX L1P-L",
	"FOXTL1P-L",
	"FREEL1P-L",
	"FRELH3T-M",
	"FRNTL1P-L",
	"FRONL1P-L",
	"FS51L1P-L",
	"FT30L1P-L",
	"FU24L1P-L",
	"FURYL1P-L",
	"FW19L1P-L",
	"FW21L1P-L",
	"FW44L1P-L",
	"FW90L1P-L",
	"G1  L1P-L",
	"G103L1P-L",
	"G109L1P-L",
	"G115L1P-L",
	"G120L1P-L",
	"G12TL1T-L",
	"G140L1T-L",
	"G150L2J-M",
	"G159L2T-M",
	"G15TL1P-L",
	"G160L1T-L",
	"G164L1P-L",
	"G180L1P-L",
	"G200L1P-L",
	"G202L1P-L",
	"G21 A2P-L",
	"G21MA4P-L",
	"G21TA2T-L",
	"G222L2T-M",
	"G250L2J-M",
	"G280L2J-M",
	"G2CAH1P-L",
	"G2GLL1J-L",
	"G2T1L1P-L",
	"G3  L1P-L",
	"G300L1P-L",
	"G44 A2P-L",
	"G46 L1P-L",
	"G4SGL1J-L",
	"G59 L1P-L",
	"G64TL1T-L",
	"G73 A2P-L",
	"G73TA2T-L",
	"G800L1P-L",
	"G850L2P-M",
	"G96 L2P-M",
	"G97 L1P-L",
	"GA10L1T-L",
	"GA20L1P-L",
	"GA5CL2J-M",
	"GA6CL2J-M",
	"GA7 L2P-L",
	"GA8 L1P-L",
	"GALXL2J-M",
	"GAUNL1P-L",
	"GAVIL1P-L",
	"GAZLH1T-L",
	"GBSPL1P-L",
	"GC1 L1P-L",
	"GDUKA2P-L",
	"GEMIL2P-L",
	"GENIL1E-L",
	"GEPEL1P-L",
	"GF20L1P-L",
	"GFLYL1J-M",
	"GL5TL2J-M",
	"GLADL1P-L",
	"GLASL1P-L",
	"GLEXL2JCM",
	"GLF2L2J-M",
	"GLF3L2J-M",
	"GLF4L2JCM",
	"GLF5L2JCM",
	"GLF6L2J-M",
	"GLSPL1P-L",
	"GLSTL1P-L",
	"GLTUL1T-L",
	"GM01L1P-L",
	"GM17L1T-L",
	"GMGCL1P-L",
	"GNATL1J-L",
	"GOBUG1P-L",
	"GOLFL1P-L",
	"GOOSA1P-L",
	"GOTRL1P-L",
	"GP3 A1P-L",
	"GP4 L1P-L",
	"GR51L1T-L",
	"GRAFL1P-L",
	"GRFNL1P-L",
	"GRIFL1P-L",
	"GRIZL1T-L",
	"GSISL1P-L",
	"GSPNL2J-L",
	"GUEPL1P-L",
	"GURIL1P-L",
	"GX  L1P-L",
	"GY10L1P-L",
	"GY20L1P-L",
	"GY30L1P-L",
	"GY80L1P-L",
	"H111L2P-M",
	"H12TH1T-L",
	"H2  H2T-L",
	"H202L1P-L",
	"H204L1P-L",
	"H207L1P-L",
	"H21 H1P-L",
	"H25AL2J-M",
	"H25BL2JCM",
	"H25CL2J-M",
	"H269H1P-L",
	"H40 L1P-L",
	"H43BH1T-L",
	"H46 H2T-M",
	"H47 H2T-M",
	"H500H1T-L",
	"H53 H2T-M",
	"H53SH3T-M",
	"H60 H2T-M",
	"H64 H2T-M",
	"HA2 G1P-L",
	"HA31L1P-L",
	"HA4TL2J-M",
	"HAHUL1P-L",
	"HAR L1J-M",
	"HAW3G1P-L",
	"HAWKL1J-M",
	"HB21L1P-L",
	"HB23L1P-L",
	"HB3 L1P-L",
	"HCATL1P-L",
	"HD34L2P-M",
	"HDJTL2J-L",
	"HEADL1P-L",
	"HERNL4P-L",
	"HF20L2J-M",
	"HI27L1P-L",
	"HIGHL1P-L",
	"HINDL1P-L",
	"HL2 L1P-L",
	"HLD4L1P-L",
	"HM38L1P-L",
	"HN70L1P-L",
	"HORNL1P-L",
	"HORZL1P-L",
	"HPTRA1P-L",
	"HPZLL1P-L",
	"HR10L1P-L",
	"HR20L1P-L",
	"HRNTL1P-L",
	"HROCL1P-L",
	"HRYAL1P-L",
	"HT16L1J-L",
	"HT2 L1P-L",
	"HT32L1P-L",
	"HT34L1T-L",
	"HT36L1J-L",
	"HU1 L1P-L",
	"HU2 L1P-L",
	"HUCOH1T-L",
	"HUMLL1P-L",
	"HUMML1P-L",
	"HUNTL1J-M",
	"HURIL1P-L",
	"HUSKL1P-L",
	"HW4PG1P-L",
	"HW4TG1T-L",
	"HX2 H1T-L",
	"I103L1P-L",
	"I114L2T-M",
	"I115L1P-L",
	"I11BL1P-L",
	"I153L1P-L",
	"I15BL1P-L",
	"I16 L1P-L",
	"I22 L1J-M",
	"I23 L1P-L",
	"I3  L1P-L",
	"I66 L1P-L",
	"I828L1T-L",
	"IA46L1P-L",
	"IA50L2T-M",
	"IA51L1P-L",
	"IA58L2T-L",
	"IA63L1J-L",
	"IFURL1P-L",
	"IL14L2P-M",
	"IL18L4T-M",
	"IL28L2J-M",
	"IL38L4T-M",
	"IL62L4J-H",
	"IL76L4J-H",
	"IL86L4J-H",
	"IL96L4J-H",
	"IMPUL1P-L",
	"INCQL1P-L",
	"INECL1P-L",
	"INEXL1P-L",
	"ION L1P-L",
	"IP06L1P-L",
	"IP10L1P-L",
	"IP26L1P-L",
	"IP6AL1P-L",
	"IPANL1P-L",
	"IR21L1P-L",
	"IR22L1P-L",
	"IR23L1P-L",
	"IR24L1P-L",
	"IR25L1T-L",
	"IR27L1P-L",
	"IR28L1P-L",
	"IR31L1P-L",
	"IR46L1P-L",
	"IR99L1J-L",
	"IS2 H1P-L",
	"IS28L1P-L",
	"ISATL1P-L",
	"ISPTL1P-L",
	"J1  L1P-L",
	"J10 L1J-M",
	"J177L1P-L",
	"J2  L1P-L",
	"J3  L1P-L",
	"J300L1P-L",
	"J328L2JCM",
	"J4  L1P-L",
	"J4B2G1P-L",
	"J5  L1P-L",
	"J728L2J-M",
	"J8A L2J-M",
	"J8B L2J-M",
	"JAB2L1P-L",
	"JAB4L1P-L",
	"JABIL1P-L",
	"JACEL1P-L",
	"JAG2H1T-L",
	"JAGRL2J-M",
	"JAJ5L1P-L",
	"JAJ6L1P-L",
	"JANUL1P-L",
	"JAROL1P-L",
	"JASTL1J-L",
	"JB1 A1P-L",
	"JB15L1P-L",
	"JC01L1P-L",
	"JC02L1P-L",
	"JCOML2J-M",
	"JCRUL1T-L",
	"JD2 L1P-L",
	"JDOEL1P-L",
	"JE2 G1P-L",
	"JFOXL1P-L",
	"JH7 L2J-M",
	"JK05L1P-L",
	"JN76L1P-L",
	"JPM1L1P-L",
	"JPROL1J-L",
	"JRC1L1P-L",
	"JS1 L2T-L",
	"JS20L2T-L",
	"JS3 L2T-L",
	"JS31L2T-L",
	"JS32L2T-M",
	"JS41L2T-M",
	"JSQAL1J-L",
	"JT2 L1P-L",
	"JU52L3P-M",
	"JUN1L1P-L",
	"JUN2L1P-L",
	"JUNRL1P-L",
	"JUPIL1P-L",
	"K126H1T-L",
	"K209H1T-L",
	"K226H2T-L",
	"K250L1P-L",
	"K35EL4J-H",
	"K35RL4J-H",
	"K50 L1J-M",
	"K51 L1P-L",
	"K8  L1J-L",
	"KA25H2T-M",
	"KA26H2P-L",
	"KA27H2T-M",
	"KA50H2T-M",
	"KA52H2T-M",
	"KA62H2T-L",
	"KAFIL1P-L",
	"KAK1L1P-L",
	"KAK3L1P-L",
	"KAT3L1T-L",
	"KATBL2P-L",
	"KATRL1P-L",
	"KC2 L2J-M",
	"KE3 L4J-H",
	"KEHAL1P-L",
	"KELAL1P-L",
	"KELDL1P-L",
	"KEROL1P-L",
	"KESTL1T-L",
	"KFIRL1J-M",
	"KFISA1P-L",
	"KH4 H1P-L",
	"KIS2L1P-L",
	"KIS4L1P-L",
	"KITIL1P-L",
	"KIWIL1P-L",
	"KK60L1P-L",
	"KL07L1P-L",
	"KL25L1P-L",
	"KL35L1P-L",
	"KLBRL1P-L",
	"KM2 L1P-L",
	"KMAXH1T-L",
	"KNTWL1P-L",
	"KODIL1T-L",
	"KOLLL1P-L",
	"KP2 L1P-L",
	"KP5 L1P-L",
	"KR1 L1P-L",
	"KR2 L1P-L",
	"KR21L1P-L",
	"KR31L1P-L",
	"KR34L1P-L",
	"KRAGL1P-L",
	"KRAHL1P-L",
	"KRICL1P-L",
	"KSTKL2P-L",
	"KT1 L1T-L",
	"KTOOL1P-L",
	"KZ2 L1P-L",
	"KZ3 L1P-L",
	"KZ4 L2P-L",
	"KZ7 L1P-L",
	"KZ8 L1P-L",
	"L10 L2P-L",
	"L101L3J-H",
	"L11 L1P-L",
	"L11EL1P-L",
	"L12 L2P-L",
	"L13 L1P-L",
	"L13ML1P-L",
	"L13SL1P-L",
	"L14 L2P-M",
	"L15 L2J-M",
	"L159L1J-M",
	"L18 L2P-M",
	"L181L1P-L",
	"L188L4T-M",
	"L200L2P-L",
	"L29 L1J-L",
	"L29AL4J-M",
	"L29BL4J-M",
	"L37 L2P-M",
	"L380L1P-L",
	"L39 L1J-L",
	"L4  A2P-L",
	"L40 L1P-L",
	"L410L2T-L",
	"L5  L1P-L",
	"L59 L1J-L",
	"L6  A2P-L",
	"L60 L1P-L",
	"L610L2T-M",
	"L70 L1P-L",
	"L8  L1P-L",
	"L90 L1T-L",
	"LA25A1P-L",
	"LA4 A1P-L",
	"LA60L1P-L",
	"LA6TL1T-L",
	"LA8 A2P-L",
	"LACOL1P-L",
	"LAE1L1E-L",
	"LAKRL1P-L",
	"LAKXL1P-L",
	"LAMAH1T-L",
	"LANCL4P-M",
	"LARKL1P-L",
	"LASTL1P-L",
	"LBUGL1P-L",
	"LCA L1J-M",
	"LCB L1P-L",
	"LCR L1P-L",
	"LEG2L1P-L",
	"LEGDL1P-L",
	"LEOPL2J-L",
	"LEVIL1P-L",
	"LGEZL1P-L",
	"LH10L1P-L",
	"LIBEL1P-L",
	"LIONL1P-L",
	"LJ23L2J-L",
	"LJ24L2J-L",
	"LJ25L2J-L",
	"LJ28L2J-L",
	"LJ31L2JBM",
	"LJ35L2JCM",
	"LJ40L2JCM",
	"LJ45L2JCM",
	"LJ55L2JCM",
	"LJ60L2JCM",
	"LJ70L2J-M",
	"LJ75L2J-M",
	"LJ85L2J-M",
	"LK17L1P-L",
	"LK19L1P-L",
	"LK20L1P-L",
	"LM5 L1P-L",
	"LM5XL1P-L",
	"LM7 L1P-L",
	"LMK1L1P-L",
	"LN27L1P-L",
	"LNC2L1P-L",
	"LNC4L1P-L",
	"LNCEL1P-L",
	"LNP4L1T-L",
	"LNT4L1T-L",
	"LOCAL1P-L",
	"LOVEL1P-L",
	"LP1 L1P-L",
	"LR2TH1T-L",
	"LS10L1P-L",
	"LS2 L1P-L",
	"LS8 L1P-L",
	"LS9 L1P-L",
	"LSTRL1P-L",
	"LTNGL2J-M",
	"LUL5L1P-L",
	"LUL6L1P-L",
	"LUL7L1P-L",
	"LUL8L1P-L",
	"LV51L1P-L",
	"LW20L1P-L",
	"LWINL1P-L",
	"LX32L1P-L",
	"LX34L1P-L",
	"LYNXH2T-L",
	"LYSAL1P-L",
	"M10 L1P-L",
	"M101L1T-L",
	"M106L1P-L",
	"M108L1P-L",
	"M110L1P-L",
	"M15 L1J-L",
	"M17 L1J-M",
	"M18 L1P-L",
	"M18TL1T-L",
	"M1SCL1P-L",
	"M2  L1P-L",
	"M200L1P-L",
	"M203L1P-L",
	"M20LL1PAL",
	"M20PL1PAL",
	"M20TL1PAL",
	"M21 L1P-L",
	"M212L1P-L",
	"M22 L1P-L",
	"M24 L1P-L",
	"M26 L1P-L",
	"M28 L2T-L",
	"M2HKL1P-L",
	"M308L1P-L",
	"M326L1J-L",
	"M339L1J-L",
	"M346L2J-M",
	"M360L1P-L",
	"M4  L1P-L",
	"M404L2P-M",
	"M5  L1P-L",
	"M55 L2J-M",
	"M6  L1P-L",
	"M7  L1P-L",
	"M74 H1P-L",
	"M7T L1T-L",
	"M8  L1P-L",
	"M9  L1P-L",
	"MA1 L1P-L",
	"MA5 L1P-L",
	"MA60L2T-M",
	"MA6HL2T-M",
	"MAGCL1P-L",
	"MAGIL1P-L",
	"MAGNL1P-L",
	"MAJRL1P-L",
	"MAMBL1P-L",
	"MAMEL1P-L",
	"MARSS4P-M",
	"MAVRL1P-L",
	"MC10L2P-L",
	"MC45L1P-L",
	"MC90L1P-L",
	"MCOUL1P-L",
	"MCOYL1P-L",
	"MCR1L1P-L",
	"MCR4L1P-L",
	"MCRRL1P-L",
	"MCULL1P-L",
	"MD11L3J-H",
	"MD3 L1P-L",
	"MD3RL1P-L",
	"MD52H1T-L",
	"MD60H1T-L",
	"MD81L2J-M",
	"MD82L2J-M",
	"MD83L2J-M",
	"MD87L2J-M",
	"MD88L2J-M",
	"MD90L2J-M",
	"ME08L1P-L",
	"ME09L1P-L",
	"ME62L2J-L",
	"MEADL1P-L",
	"MEL2L1P-L",
	"MERKL2P-L",
	"MESSL1P-L",
	"METRL2J-M",
	"MEXPL1P-L",
	"MF10L1P-L",
	"MF17L1P-L",
	"MG15L1J-L",
	"MG17L1J-L",
	"MG19L2J-M",
	"MG21L1J-M",
	"MG23L1J-M",
	"MG25L2J-M",
	"MG29L2J-M",
	"MG31L2J-M",
	"MG44L2J-M",
	"MGATL1J-L",
	"MGICL1P-L",
	"MGNML1P-L",
	"MH20H2T-L",
	"MH46L1P-L",
	"MI10H2T-M",
	"MI14H2T-M",
	"MI2 H2T-L",
	"MI24H2T-M",
	"MI26H2T-M",
	"MI28H2T-M",
	"MI34H1P-L",
	"MI38H2T-M",
	"MI4 H1P-L",
	"MI6 H2T-M",
	"MI8 H2T-M",
	"MICOL2J-L",
	"MIDRL1P-L",
	"MIMPL1P-L",
	"MIMUL1P-L",
	"MIR2L1J-M",
	"MIRAL1J-M",
	"MITEL1P-L",
	"MJ2 L1P-L",
	"MJ7 L1P-L",
	"MLERL1P-L",
	"MM14G1P-L",
	"MM16G1P-L",
	"MM19G1P-L",
	"MM21G1P-L",
	"MM22G1P-L",
	"MM24G1P-L",
	"MMACL1P-L",
	"MMAXG1P-L",
	"MMUTL1P-L",
	"MNEXL1P-L",
	"MOCUL1P-L",
	"MOGOL1P-L",
	"MOL1L1P-L",
	"MONAL1P-L",
	"MONIL1P-L",
	"MOR2L1P-L",
	"MOSPL1P-L",
	"MOSQL2P-M",
	"MOTOL1P-L",
	"MP02L1P-L",
	"MP20L1P-L",
	"MR25L1P-L",
	"MR35L1P-L",
	"MR3TL1T-L",
	"MRAIL1P-L",
	"MRAML1P-L",
	"MRF1L1J-M",
	"MRMDA1P-L",
	"MRTNL1P-L",
	"MS1 L1P-L",
	"MS18L1P-L",
	"MS23L1P-L",
	"MS25L1P-L",
	"MS30L1P-L",
	"MS31L1P-L",
	"MS73L1P-L",
	"MS76L2J-L",
	"MSAIL1P-L",
	"MSQ2L1P-L",
	"MT  G1P-L",
	"MT2 L2J-M",
	"MU2 L2T-L",
	"MU23L1P-L",
	"MU30L2J-M",
	"MUS2L1P-L",
	"MVN1L1P-L",
	"MVRKL1P-L",
	"MX10L1P-L",
	"MX1TL1P-L",
	"MX2 L1P-L",
	"MX65L1P-L",
	"MX80L1P-L",
	"MXS L1P-L",
	"MY12L1P-L",
	"MY13L1P-L",
	"MYA4L4J-H",
	"MYS4L1J-M",
	"N110L1P-L",
	"N120L1P-L",
	"N250L2T-M",
	"N260L2T-M",
	"N262L2T-M",
	"N3  L1P-L",
	"N320L1P-L",
	"N340L1P-L",
	"N3N L1P-L",
	"N5  L1P-L",
	"NA40H2T-L",
	"NAL2L1P-L",
	"NAVIL1P-L",
	"NC85L1P-L",
	"ND1TL1T-L",
	"NDACL1P-L",
	"NDATL1P-L",
	"NDICL1P-L",
	"NG4 L1P-L",
	"NG5 L1P-L",
	"NH90H2T-M",
	"NHCOL1P-L",
	"NI28L1P-L",
	"NIBBL1P-L",
	"NIMBL1P-L",
	"NIPRL1P-L",
	"NM5 L1P-L",
	"NMCUL1P-L",
	"NNJAL1P-L",
	"NOMAL2T-L",
	"NORAL2P-M",
	"NORSL1P-L",
	"NPORL1P-L",
	"NSTRL1P-L",
	"NT10L1P-L",
	"NXT L1P-L",
	"O1  L1P-L",
	"O3  L1P-L",
	"OH1 H2T-L",
	"OKHOG1P-L",
	"OM1 L1P-L",
	"OMAGL1P-L",
	"OMGAL1P-L",
	"OMLAL1T-L",
	"ONE L1P-L",
	"ONEXL1P-L",
	"OPCAL1P-L",
	"OR10A1P-L",
	"OR12A2P-L",
	"OSCRL1P-L",
	"OUDEL1P-L",
	"OVODL1P-L",
	"OZZIL1P-L",
	"P06TL2P-L",
	"P1  L4J-M",
	"P100L1P-L",
	"P130L1P-L",
	"P136A2P-L",
	"P148L1P-L",
	"P149L1P-L",
	"P180L2T-L",
	"P18TL1T-L",
	"P19 L1P-L",
	"P2  L2P-M",
	"P208L1P-L",
	"P210L1P-L",
	"P212L2P-L",
	"P220L1P-L",
	"P230L1P-L",
	"P27 L1P-L",
	"P270L1P-L",
	"P28AL1PAL",
	"P28BL1PAL",
	"P28RL1PAL",
	"P28SL1P-L",
	"P28TL1PAL",
	"P28UL1P-L",
	"P3  L4T-M",
	"P32RL1PAL",
	"P32TL1PAL",
	"P337L2P-L",
	"P38 L2P-M",
	"P39 L1P-L",
	"P40 L1P-L",
	"P46TL1T-L",
	"P47 L1P-M",
	"P4Y L4P-M",
	"P50 L1P-L",
	"P51 L1P-L",
	"P57 L1P-L",
	"P60 L1P-L",
	"P61 L2P-M",
	"P63 L1P-L",
	"P66PL2P-L",
	"P66TL2T-L",
	"P68 L2P-L",
	"P68TL2T-L",
	"P70 L1P-L",
	"P750L1T-L",
	"P8  L2J-M",
	"P80 L1P-L",
	"P82 L2P-M",
	"PA11L1P-L",
	"PA12L1P-L",
	"PA14L1P-L",
	"PA15L1P-L",
	"PA16L1P-L",
	"PA17L1P-L",
	"PA18L1PAL",
	"PA20L1P-L",
	"PA22L1P-L",
	"PA23L2P-L",
	"PA24L1P-L",
	"PA25L1P-L",
	"PA27L2PAL",
	"PA30L2P-L",
	"PA31L2PAL",
	"PA32L1PAL",
	"PA34L2PAL",
	"PA36L1P-L",
	"PA38L1PAL",
	"PA44L2PAL",
	"PA46L1PAL",
	"PA47L1JAL",
	"PAGOL1P-L",
	"PANTL1P-L",
	"PAR1L1P-L",
	"PAR4L1P-L",
	"PARLL1P-L",
	"PAT2L1P-L",
	"PAT4L2T-L",
	"PAULL1P-L",
	"PAV4G1P-L",
	"PAY1L2T-L",
	"PAY2L2T-L",
	"PAY3L2T-L",
	"PAY4L2T-L",
	"PC12L1TAL",
	"PC21L1T-L",
	"PC24L2J-M",
	"PC6PL1P-L",
	"PC6TL1TAL",
	"PC7 L1TAL",
	"PC9 L1TAL",
	"PCA2G1P-L",
	"PDIGL2P-L",
	"PECRL1P-L",
	"PEGAL2P-L",
	"PEGZL1P-L",
	"PELIL1P-L",
	"PEMBL2P-L",
	"PETLL1P-L",
	"PETRA1P-L",
	"PGEEL1P-L",
	"PGK1L1P-L",
	"PHILH1P-L",
	"PHIXG1P-L",
	"PHNXL1P-L",
	"PIATL1P-L",
	"PICOL1P-L",
	"PILLL1P-L",
	"PINOL1P-L",
	"PIPAL1P-L",
	"PISIL1P-L",
	"PITAL1P-L",
	"PITEL1E-L",
	"PIVIL1P-L",
	"PK11L1P-L",
	"PK15L1P-L",
	"PK18L1P-L",
	"PK19L1P-L",
	"PK20L1P-L",
	"PK21L1P-L",
	"PK23L1P-L",
	"PK25L1P-L",
	"PKANL1P-L",
	"PL1 L1P-L",
	"PL12L1P-L",
	"PL2 L1P-L",
	"PL4 L1P-L",
	"PL9 L1P-L",
	"PLUSL1P-L",
	"PNR2L1P-L",
	"PNR3L1P-L",
	"PNR4L1P-L",
	"PO2 L1P-L",
	"PO60L1P-L",
	"POLIL1P-L",
	"PONYA1P-L",
	"PP2 L1P-L",
	"PP3 L1P-L",
	"PPROL1P-L",
	"PRBPL1P-L",
	"PRBRL1P-L",
	"PRCEL2P-L",
	"PRENL1P-L",
	"PRETL1P-L",
	"PREXL1P-L",
     	"PRM1L2JBL",
	"PROCL1P-L",
	"PROTL1P-L",
	"PROWL1P-L",
	"PRPRL1P-L",
	"PRTSL2J-L",
	"PRXTL1T-L",
	"PSTML1P-L",
	"PSW4H1T-L",
	"PT21L1P-L",
	"PT22L1P-L",
	"PT70L1P-L",
	"PT80L1P-L",
	"PTMSL1P-L",
	"PTRLL1P-L",
	"PTS1L1P-L",
	"PTS2L1P-L",
	"PTSSL1P-L",
	"PUL6L1P-L",
	"PULSL1P-L",
	"PUMAH2T-L",
	"PUP L1P-L",
	"PURSL1P-L",
	"PUSHL1P-L",
	"PW4 L1P-L",
	"PZ01L1P-L",
	"PZ02L1P-L",
	"PZ04L1P-L",
	"PZ05L1P-L",
	"PZ06L1P-L",
	"PZ12L1P-L",
	"PZ26L1P-L",
	"PZ3TL1T-L",
	"PZ4ML1P-L",
	"PZ6TL1T-L",
	"Q5  L2J-M",
	"QAILL1P-L",
	"QALTL1P-L",
	"QESTL1P-L",
	"QIC2L1P-L",
	"QR01L1P-L",
	"QUASL1P-L",
	"QUICL1P-L",
	"R100L1P-L",
	"R109L1P-L",
	"R11 L1P-L",
	"R135L4J-H",
	"R185L1P-L",
	"R2  S1P-L",
	"R200L1P-L",
	"R22 H1P-L",
	"R300L1P-L",
	"R4  H1P-L",
	"R44 H1P-L",
	"R66 H1T-L",
	"R721L3J-M",
	"R722L3J-M",
	"R90FL1P-L",
	"R90RL1P-L",
	"R90TL1T-L",
	"RA14L1P-L",
	"RA17L1P-L",
	"RAF2G1P-L",
	"RAIDL1P-L",
	"RAILL1P-L",
	"RALLL1P-L",
	"RANGL1P-L",
	"RAROL1P-L",
	"RAV3L1P-L",
	"RAV5L1P-L",
	"RAZML1P-L",
	"RBELL1P-L",
	"RC3 A1P-L",
	"RC70L2P-L",
	"RD03L1P-L",
	"RD20L1P-L",
	"RDH2L1P-L",
	"RELIL1P-L",
	"RENEL1P-L",
	"REV6G1T-L",
	"RF10L1P-L",
	"RF3 L1P-L",
	"RF4 L1P-L",
	"RF47L1P-L",
	"RF5 L1P-L",
	"RF6 L1P-L",
	"RF9 L1P-L",
	"RFALL2J-M",
	"RGNTL1P-L",
	"RJ03L1P-L",
	"RJ1HL4JCM",
	"RJ70L4JCM",
	"RJ85L4JCM",
	"RK5 L1P-L",
	"RLU1L1P-L",
	"RMOUH1T-L",
	"RNGRL1P-L",
	"ROARL1-L",
	"RODSL1P-L",
	"RONDL1P-L",
	"ROSEL1P-L",
	"RP1 H2T-L",
	"RPUPG1P-L",
	"RS12L1P-L",
	"RS18L1P-L",
	"RTA4L1P-L",
	"RUBIL1P-L",
	"RV4TL1T-L",
	"RV6 L1P-L",
	"RVALH2T-M",
	"RW19L1P-L",
	"RW20L1P-L",
	"RW22L1P-L",
	"RW26L1P-L",
	"RW3 L1P-L",
	"RYSTL1P-L",
	"S05FL1P-L",
	"S05RL1P-L",
	"S1  L1P-L",
	"S10 L1P-L",
	"S107S1P-L",
	"S108L1P-L",
	"S10SL1P-L",
	"S11 L1P-L",
	"S12 A1P-L",
	"S200L1J-L",
	"S202A2P-L",
	"S208L1P-L",
	"S21 L1P-L",
	"S210L2J-M",
	"S211L1J-L",
	"S223L1P-L",
	"S22TL1P-L",
	"S274H1T-L",
	"S278H1T-L",
	"S285H1T-L",
	"S2P L2P-M",
	"S2T L2T-M",
	"S3  L2J-M",
	"S330H1T-L",
	"S360H1T-L",
	"S37 L2J-M",
	"S38 A2P-L",
	"S39 A1P-L",
	"S4  L1P-L",
	"S400S2P-L",
	"S434H1T-L",
	"S45 L1P-L",
	"S51 H1P-L",
	"S51DL1P-L",
	"S52 H1P-L",
	"S55PH1P-L",
	"S55TH1T-L",
	"S58PH1P-L",
	"S58TH1T-L",
	"S6  L1P-L",
	"S601L2J-L",
	"S61 H2T-M",
	"S61RH2T-M",
	"S62 H1T-L",
	"S64 H2T-M",
	"S65CH2T-L",
	"S76 H2T-L",
	"S900L1P-L",
	"S92 H2T-M",
	"SA02L1P-L",
	"SA03L1P-L",
	"SA04L1P-L",
	"SA05L1P-L",
	"SA10L1P-L",
	"SA11L1P-L",
	"SA2 L1P-L",
	"SA20A1P-L",
	"SA3 L1P-L",
	"SA30L1P-L",
	"SA37L1P-L",
	"SA38L2P-L",
	"SA50L1P-L",
	"SA6 L1P-L",
	"SA6EL1T-L",
	"SA7 L1P-L",
	"SA70L1P-L",
	"SA75L1P-L",
	"SA8TL2T-L",
	"SAB2L1P-L",
	"SABAL1P-L",
	"SACEL1P-L",
	"SACRL1P-L",
	"SAFFL1P-L",
	"SAH1L1P-L",
	"SAKOL1P-L",
	"SALBL1P-L",
	"SAM L1P-L",
	"SANDS4P-M",
	"SAPHL1P-L",
	"SARAL2T-L",
	"SASHL2P-L",
	"SASPL1P-L",
	"SASYL1P-L",
	"SATAL2J-L",
	"SAVAL1P-L",
	"SAVGL1P-L",
	"SB05L2J-L",
	"SB20L2TBM",
	"SB29L1J-M",
	"SB32L1J-M",
	"SB35L1J-M",
	"SB37L1J-M",
	"SB39L1J-M",
	"SB7 L1P-L",
	"SB91L1P-L",
	"SBD L1P-L",
	"SBLSL2P-L",
	"SBM3L1P-L",
	"SBOYL1P-L",
	"SBR1L2J-M",
	"SBR2L2J-M",
	"SC01L1P-L",
	"SC7 L2T-L",
	"SCAML1P-L",
	"SCEPL1P-L",
	"SCIIG1P-L",
	"SCOML1P-L",
	"SCORH1P-L",
	"SCOUH1T-L",
	"SCROL1P-L",
	"SCTRL1P-L",
	"SCUBL1P-L",
	"SCW1L1P-L",
	"SD26L1P-L",
	"SD4 L1P-L",
	"SDUSL1P-L",
	"SE5AL1P-L",
	"SE5RL1P-L",
	"SEATA1T-L",
	"SEAWA1P-L",
	"SERAL1P-L",
	"SESTA1T-L",
	"SF2 L1P-L",
	"SF23L1P-L",
	"SF24L1P-L",
	"SF25L1P-L",
	"SF27L1P-L",
	"SF28L1P-L",
	"SF31L1P-L",
	"SF32L1P-L",
	"SF34L2T-M",
	"SF35L1P-L",
	"SF36L1P-L",
	"SF50L1J-L",
	"SG37L1P-L",
	"SG92L1T-L",
	"SGRAL1P-L",
	"SGUPL4T-M",
	"SH33L2T-M",
	"SH36L2T-M",
	"SH4 H1P-L",
	"SH5 S4T-M",
	"SHACL4P-M",
	"SHAKA1P-L",
	"SHAWL1J-M",
	"SHEAA1P-L",
	"SHEKL1P-L",
	"SHERL1P-L",
	"SHOEL1P-L",
	"SHOPL1P-L",
	"SHORL1P-L",
	"SHRKL1P-L",
	"SHRTL1T-L",
	"SIDEL1P-L",
	"SIGML1P-L",
	"SILHL1P-L",
	"SIR2L1P-L",
	"SIRAL1P-L",
	"SJ30L2J-L",
	"SJETL1J-L",
	"SK10L1P-L",
	"SK70L2P-L",
	"SKARL1P-L",
	"SKIFA1P-L",
	"SKIMA1P-L",
	"SKRAL1P-L",
	"SKYCL2P-L",
	"SKYOL1P-L",
	"SKYRL1P-L",
	"SL1 L1P-L",
	"SL39L1P-L",
	"SL90L1P-L",
	"SLK3L1P-L",
	"SLK5L1P-L",
	"SM01L1P-L",
	"SM19L1T-L",
	"SM20L1T-L",
	"SM60L3P-L",
	"SM92L1P-L",
	"SMAXA1P-L",
	"SMB2L1J-M",
	"SNADG1P-L",
	"SNAPL1P-L",
	"SNGYL1P-L",
	"SNOSL1P-L",
	"SNS2L1P-L",
	"SNS7L1P-L",
	"SNS9L1P-L",
	"SNTAL1P-L",
	"SOK2L1P-L",
	"SOKLL1P-L",
	"SOL1L4E-L",
	"SOL2L4E-L",
	"SOLIL1P-L",
	"SONXL1P-L",
	"SORAL1P-L",
	"SP20L1P-L",
	"SP33L2J-L",
	"SP55L1P-L",
	"SP6EL1P-L",
	"SP7 L1P-L",
	"SP91L1P-L",
	"SP95L1P-L",
	"SPA2L1P-L",
	"SPARL1P-L",
	"SPC2L1P-L",
	"SPDRL1P-L",
	"SPELL1P-L",
	"SPGYG1P-L",
	"SPHAG1P-L",
	"SPIRL1P-L",
	"SPITL1P-L",
	"SPORL1P-L",
	"SPR2L1P-L",
	"SPRTL1P-L",
	"SPSTL1P-L",
	"SPT1L1P-L",
	"SQ2TL1P-L",
	"SQESL2P-L",
	"SR01L1P-L",
	"SR20L1P-L",
	"SR22L1P-L",
	"SR71L2J-M",
	"SRACL1P-L",
	"SRAIL1P-L",
	"SRASL1P-L",
	"SRAYA1P-L",
	"SREYA1P-L",
	"SS2 L1-M",
	"SS2PL1P-L",
	"SS2TL1T-L",
	"SSABL1J-M",
	"SSC G1P-L",
	"SSTMA1P-L",
	"ST1 L2P-L",
	"ST10L1P-L",
	"ST30L1P-L",
	"ST50L1T-L",
	"ST60L1P-L",
	"ST75L1P-L",
	"ST87L1P-L",
	"STALL1P-L",
	"STARL2T-L",
	"STATL1T-L",
	"STCHL1P-L",
	"STFFL1P-L",
	"STILL1P-L",
	"STLNL1T-L",
	"STORL1P-L",
	"STR2L1P-L",
	"STRAL1P-L",
	"STRIL1P-L",
	"STRML1P-L",
	"STSTL1P-L",
	"SU15L2J-M",
	"SU17L1J-M",
	"SU24L2J-M",
	"SU25L2J-M",
	"SU26L1P-L",
	"SU27L2J-M",
	"SU29L1P-L",
	"SU31L1P-L",
	"SU38L1P-L",
	"SU7 L1J-M",
	"SU80L2T-M",
	"SU95L2J-M",
	"SUBAL1P-L",
	"SUCOH2T-M",
	"SUNBL1P-L",
	"SUNVL1P-L",
	"SURNH2T-M",
	"SURUL1P-L",
	"SUSOL1P-L",
	"SV4 L1P-L",
	"SVNHL1P-L",
	"SW18L1P-L",
	"SW2 L2T-L",
	"SW3 L2T-L",
	"SW4 L2T-L",
	"SWAKL1P-L",
	"SWATL1P-L",
	"SWIFL1J-M",
	"SWINL1P-L",
	"SWORL1P-L",
	"SX30L1P-L",
	"SYCAH1P-L",
	"SYMPL1P-L",
	"SYNCL1P-L",
	"SZ45L1P-L",
	"SZ9ML1P-L",
	"T1  L1J-L",
	"T10 L1P-L",
	"T101L1T-L",
	"T134L2J-M",
	"T144L4J-H",
	"T154L3J-M",
	"T160L4J-H",
	"T18 L1P-L",
	"T2  L2J-L",
	"T204L2J-M",
	"T206L1P-L",
	"T210L1P-L",
	"T211L1P-L",
	"T22ML2J-M",
	"T250L1P-L",
	"T28 L1P-L",
	"T30 L1P-L",
	"T33 L1J-M",
	"T334L2J-M",
	"T34PL1P-L",
	"T34TL1T-L",
	"T35 L1P-L",
	"T37 L2J-L",
	"T38 L2J-L",
	"T4  L2J-M",
	"T40 L1P-L",
	"T411L1P-L",
	"T415L1P-L",
	"T419L1P-L",
	"T5  L1T-L",
	"T50 L2P-L",
	"T50SL2J-M",
	"T51 L1P-L",
	"T6  L1P-L",
	"T7  L1T-L",
	"TA15L1P-L",
	"TA16A1P-L",
	"TA20L1P-L",
	"TAA1L1P-L",
	"TAGOL1P-L",
	"TAILL1P-L",
	"TAMPL1P-L",
	"TAROL1P-L",
	"TAYAL1P-L",
	"TAYBL1P-L",
	"TAYDL1P-L",
	"TB05L1P-L",
	"TB20L1P-L",
	"TB21L1P-L",
	"TB30L1P-L",
	"TB31L1T-L",
	"TBEEA2P-L",
	"TBM L1P-M",
	"TBM7L1T-L",
	"TBM8L1T-L",
	"TBM9L1T-L",
	"TBR3L1P-L",
	"TC2 L1P-L",
	"TCATL2P-M",
	"TCOUL2P-L",
	"TD1 L1P-L",
	"TD2 L1T-L",
	"TD3 L1P-L",
	"TEALA1P-L",
	"TERML1P-L",
	"TERRL1P-L",
	"TEX2L1T-L",
	"TEXAL1P-L",
	"TF19L1P-L",
	"TF21L1P-L",
	"TF22L1P-L",
	"TFK2L1P-L",
	"TFOCL1P-L",
	"TFUNL1P-L",
	"TGRSL1P-L",
	"TIGRH2T-L",
	"TIJUL1P-L",
	"TIPBL1P-L",
	"TJETL2J-L",
	"TL20L1P-L",
	"TL30L1P-L",
	"TLEGL1T-L",
	"TM5 L1P-L",
	"TMOTL1P-L",
	"TMUSL1P-L",
	"TNAVL2P-L",
	"TNDRL1P-L",
	"TOBAL1P-L",
	"TOOTL1P-L",
	"TOR L2J-M",
	"TOURL1P-L",
	"TOXOL1P-L",
	"TP40L1P-L",
	"TPILL1T-L",
	"TPINL2P-L",
	"TR1 A1P-L",
	"TR20L1P-L",
	"TR26L1P-L",
	"TR55L1P-L",
	"TRALL1P-L",
	"TRAPL1P-L",
	"TRBAL1P-L",
	"TRDOL1P-L",
	"TRF1L1P-L",
	"TRIML3P-L",
	"TRISL3P-L",
	"TRMAL3P-L",
	"TRWNL2P-L",
	"TS11L1J-L",
	"TS14L1P-L",
	"TS1JL1J-L",
	"TS8 L1P-L",
	"TSPTL1P-L",
	"TSTNL1P-L",
	"TSTRG1P-L",
	"TT62L2P-L",
	"TTRSL1P-L",
	"TTWOL1P-L",
	"TU16L2J-M",
	"TU22L2J-M",
	"TU4 L4T-M",
	"TU95L4T-H",
	"TUCAL1T-L",
	"TUL3L1P-L",
	"TUTRL1P-L",
	"TVL4L1P-L",
	"TVLBL1P-L",
	"TWENL1P-L",
	"TWSPL1P-L",
	"TZRVL1J-L",
	"U16 A2P-M",
	"U2  L1J-M",
	"U21 L2T-L",
	"U22 L1P-L",
	"UBATL1P-L",
	"UF10L1P-L",
	"UF13L1P-L",
	"UFHTG1P-L",
	"UH1 H1T-L",
	"UH12H1P-L",
	"UH1YH2T-M",
	"UL10L1P-L",
	"UL20L1P-L",
	"UL2FL1P-L",
	"UL45L1P-L",
	"ULPAL1P-L",
	"ULTSH1P-L",
	"UM18G1P-L",
	"UNIVL1P-L",
	"URRAL1P-L",
	"US1 A4T-M",
	"US2 A4T-M",
	"UT60L1P-L",
	"UT65L1P-L",
	"UT66L1P-L",
	"UT75L1P-L",
	"UU12L1P-L",
	"V1  L2T-M",
	"V10 L2T-L",
	"V22 T2T-M",
	"V252L1P-L",
	"V322L1P-L",
	"V351L1P-L",
	"V452L1T-L",
	"V500H1P-L",
	"V8SPL1P-L",
	"VALIL1P-L",
	"VAMPL1J-L",
	"VANTL1J-L",
	"VAUTL2J-M",
	"VC10L4J-H",
	"VELOL1P-L",
	"VENTL1P-L",
	"VEZEL1P-L",
	"VF14L2J-M",
	"VF2 L1P-L",
	"VF35L1J-M",
	"VF60L1T-L",
	"VG3TL1P-L",
	"VGULL1P-L",
	"VIMAL1P-L",
	"VIMYL2P-L",
	"VIPJL1J-L",
	"VIPRL1P-L",
	"VISCL4T-M",
	"VISIL1P-L",
	"VIVAL1P-L",
	"VIX L1P-L",
	"VIXNL1P-L",
	"VJ22A1P-L",
	"VK3PL1P-L",
	"VL3 L1P-L",
	"VM1 L1P-L",
	"VMT L4J-H",
	"VNOML1J-M",
	"VO10L1P-L",
	"VOL2L1P-L",
	"VP2 L1P-L",
	"VR20L1P-L",
	"VSONL1P-L",
	"VTORL2T-L",
	"VTRAL1P-L",
	"VTURL1P-L",
	"VULCL4J-M",
	"VUT1L1P-L",
	"VVIGL1P-L",
	"VW10L1P-L",
	"VWITL1P-L",
	"W11 L1P-L",
	"W201L1P-L",
	"W3  H2T-L",
	"W62TL1T-L",
	"WA40L1P-L",
	"WA41L1P-L",
	"WA42L1P-L",
	"WA50L1P-L",
	"WA80L1P-L",
	"WAC9L1P-L",
	"WACAL1P-L",
	"WACCL1P-L",
	"WACDL1P-L",
	"WACEL1P-L",
	"WACFL1P-L",
	"WACGL1P-L",
	"WACML1P-L",
	"WACNL1P-L",
	"WACOL1P-L",
	"WACTL1P-L",
	"WAIXL1P-L",
	"WASPH1T-L",
	"WB57L2J-M",
	"WBOOL1P-L",
	"WCATL1P-L",
	"WDEXL1P-L",
	"WESXH2T-L",
	"WF4UL1P-L",
	"WFOCL1P-L",
	"WFURL1P-L",
	"WG30H2T-L",
	"WH1 L1P-L",
	"WHATL1P-L",
	"WHILL1P-L",
	"WHISL1P-L",
	"WHITL1P-L",
	"WHK2L4J-M",
	"WHKNL2J-M",
	"WICHL1P-L",
	"WILTL1P-L",
	"WINDL1P-L",
	"WINEL1P-L",
	"WIRRL1P-L",
	"WM2 L1P-L",
	"WOPUL1P-L",
	"WP40L1P-L",
	"WP47L1P-L",
	"WS22L1P-L",
	"WSP L1P-L",
	"WT9 L1P-L",
	"WUSHA1P-L",
	"WW1 L1P-L",
	"WW23L2J-M",
	"WW24L2J-M",
	"WZ10H2T-L",
	"WZERL1P-L",
	"X2  H1T-L",
	"X29 L1J-M",
	"X3  H2T-L",
	"X32 L1J-M",
	"X4  L1P-L",
	"X49 H2T-M",
	"X55 L2J-M",
	"XA41L1P-L",
	"XA42L1P-L",
	"XA85L1P-L",
	"XAIRL1P-L",
	"XL2 L1P-L",
	"XNONG1P-L",
	"XNOSL1P-L",
	"XV15T2T-L",
	"Y11 L2P-L",
	"Y112L1P-L",
	"Y12 L2T-L",
	"Y12FL2T-M",
	"Y130L2J-M",
	"Y141L3J-M",
	"Y18TL1P-L",
	"YAK3L1P-L",
	"YAK9L1P-L",
	"YALEL1P-L",
	"YARRL1P-L",
	"YASTL1P-L",
	"YC12L1P-L",
	"YK11L1P-L",
	"YK12L1P-L",
	"YK18L1P-L",
	"YK28L2J-M",
	"YK30L1J-L",
	"YK38L3J-M",
	"YK40L3J-M",
	"YK42L3J-M",
	"YK50L1P-L",
	"YK52L1P-L",
	"YK53L1P-L",
	"YK54L1P-L",
	"YK55L1P-L",
	"YK58L1P-L",
	"YNHLH2P-L",
	"YS11L2T-M",
	"YUNOL1P-L",
	"Z22 L1P-L",
	"Z26 L1P-L",
	"Z37PL1P-L",
	"Z37TL1T-L",
	"Z42 L1P-L",
	"Z43 L1P-L",
	"Z50 L1P-L",
	"ZA6 H1P-L",
	"ZEP2L1P-L",
	"ZEPHL1P-L",
	"ZEROL1P-L",
	"ZIA L1P-L",
	"ZIU L1P-L",
	"ZULUL1P-L"
};

#if 0

std::string Aircraft::get_aircrafttypeclass(const Glib::ustring& acfttype)
{
	std::string r("L1---");
	std::string at(acfttype);
	at.resize(4, ' ');
	AircraftTypeClassDbCompare cmp;
	const aircraft_type_class_t *it(std::lower_bound(&aircraft_type_class_db[0],
							 &aircraft_type_class_db[sizeof(aircraft_type_class_db)/sizeof(aircraft_type_class_db[0])],
							 at.c_str(), cmp));
	if (it != &aircraft_type_class_db[sizeof(aircraft_type_class_db)/sizeof(aircraft_type_class_db[0])] &&
	    !cmp(*it, acfttype.c_str()) && !cmp(acfttype.c_str(), *it))
		r = (*it) + 4;
	if (r.size() != 5)
		r.resize(5, '-');
	return r;
}

#else

std::string Aircraft::get_aircrafttypeclass(const Glib::ustring& acfttype)
{
	std::string r("L1---");
	std::string at(acfttype);
	at.resize(4, ' ');
	AircraftTypeClassDbCompare cmp;
	std::pair<const aircraft_type_class_t *, const aircraft_type_class_t *> p(std::equal_range(&aircraft_type_class_db[0],
												   &aircraft_type_class_db[sizeof(aircraft_type_class_db)/sizeof(aircraft_type_class_db[0])],
												   at.c_str(), cmp));
	if (p.first != p.second)
		r = (*p.first) + 4;
	if (r.size() != 5)
		r.resize(5, '-');
	return r;
}

#endif

std::string Aircraft::get_aircrafttypeclass(void) const
{
	std::string r(get_aircrafttypeclass(get_icaotype()));
	if (r.size() != 5)
		r.resize(5, '-');
	if (r[2] == '-') {
		switch (get_propulsion()) {
		default:
			r[2] = 'P';
			break;

		case propulsion_turboprop:
			r[2] = 'T';
			break;

		case propulsion_turbojet:
			r[2] = 'J';
			break;
		}
	}
	if (r[3] == '-') {
		static const double apprcat[] = { 90 / 1.3, 120 / 1.3, 140 / 1.3, 165 / 1.3 };
		unsigned int i(0);
		for (; i < sizeof(apprcat)/sizeof(apprcat[0]); ++i)
			if (get_vs0() <= apprcat[i])
				break;
		r[3] = 'A' + i;
	}
	if (r[4] == '-')
		r[4] = get_wakecategory();
	return r;
}

void Aircraft::check_aircraft_type_class(void)
{
	for (unsigned int i = 1; i < sizeof(aircraft_type_class_db)/sizeof(aircraft_type_class_db[0]); ++i) {
		AircraftTypeClassDbCompare cmp;
		if (cmp(aircraft_type_class_db[i - 1], aircraft_type_class_db[i]))
			continue;
		std::ostringstream oss;
		oss << "Aircraft Type/Class Database not ascending: " << aircraft_type_class_db[i - 1] << ", " << aircraft_type_class_db[i];
		std::cerr << oss.str() << std::endl;
		throw std::runtime_error(oss.str());
	}
}

#ifdef HAVE_ICONV

bool Aircraft::to_ascii_iconv(std::string& r, const Glib::ustring& x)
{
	r.clear();
	iconv_t ic(iconv_open("US-ASCII//TRANSLIT", "UTF-8"));
	if (ic == (iconv_t)-1)
		return false;
	size_t insz(x.bytes());
	char *inbuf(const_cast<char *>(x.data()));
	size_t outsz(insz * 4);
	size_t outbufsz(outsz);
	char outbuf[outsz];
	char *outbufp(outbuf);
	size_t sz(iconv(ic, &inbuf, &insz, &outbufp, &outsz));
	iconv_close(ic);
	if (outsz < outbufsz) {
		r = std::string(outbuf, outbuf + (outbufsz - outsz));
		return true;
	}
	return false;
}

#else

bool Aircraft::to_ascii_iconv(std::string& r, const Glib::ustring& x)
{
	return false;
}

#endif

const Aircraft::UnicodeMapping Aircraft::unicodemapping[] = {
	{ 0x00C0, "A" },
	{ 0x00C1, "A" },
	{ 0x00C2, "A" },
	{ 0x00C3, "A" },
	{ 0x00C4, "AE" },
	{ 0x00C5, "A" },
	{ 0x00C6, "AE" },
	{ 0x00C7, "C" },
	{ 0x00C8, "E" },
	{ 0x00C9, "E" },
	{ 0x00CA, "E" },
	{ 0x00CB, "E" },
	{ 0x00CC, "I" },
	{ 0x00CD, "I" },
	{ 0x00CE, "I" },
	{ 0x00CF, "I" },
	{ 0x00D0, "D" },
	{ 0x00D1, "N" },
	{ 0x00D2, "O" },
	{ 0x00D3, "O" },
	{ 0x00D4, "O" },
	{ 0x00D5, "O" },
	{ 0x00D6, "OE" },
	{ 0x00D8, "O" },
	{ 0x00D9, "U" },
	{ 0x00DA, "U" },
	{ 0x00DB, "U" },
	{ 0x00DC, "UE" },
	{ 0x00DD, "Y" },
	{ 0x00DF, "SS" },
	{ 0x00E0, "a" },
	{ 0x00E1, "a" },
	{ 0x00E2, "a" },
	{ 0x00E3, "a" },
	{ 0x00E4, "ae" },
	{ 0x00E5, "a" },
	{ 0x00E6, "ae" },
	{ 0x00E7, "c" },
	{ 0x00E8, "e" },
	{ 0x00E9, "e" },
	{ 0x00EA, "e" },
	{ 0x00EB, "e" },
	{ 0x00EC, "i" },
	{ 0x00ED, "i" },
	{ 0x00EE, "i" },
	{ 0x00EF, "i" },
	{ 0x00F1, "n" },
	{ 0x00F2, "o" },
	{ 0x00F3, "o" },
	{ 0x00F4, "o" },
	{ 0x00F5, "o" },
	{ 0x00F6, "oe" },
	{ 0x00F8, "o" },
	{ 0x00F9, "u" },
	{ 0x00FA, "u" },
	{ 0x00FB, "u" },
	{ 0x00FC, "ue" },
	{ 0x00FD, "y" },
	{ 0x00FF, "y" },
	{ 0x0100, "A" },
	{ 0x0101, "a" },
	{ 0x0102, "A" },
	{ 0x0103, "a" },
	{ 0x0104, "A" },
	{ 0x0105, "a" },
	{ 0x0106, "C" },
	{ 0x0107, "c" },
	{ 0x0108, "C" },
	{ 0x0109, "c" },
	{ 0x010A, "C" },
	{ 0x010B, "c" },
	{ 0x010C, "C" },
	{ 0x010D, "c" },
	{ 0x010E, "D" },
	{ 0x010F, "d" },
	{ 0x0110, "D" },
	{ 0x0111, "d" },
	{ 0x0112, "E" },
	{ 0x0113, "e" },
	{ 0x0114, "E" },
	{ 0x0115, "e" },
	{ 0x0116, "E" },
	{ 0x0117, "e" },
	{ 0x0118, "E" },
	{ 0x0119, "e" },
	{ 0x011A, "E" },
	{ 0x011B, "e" },
	{ 0x011C, "G" },
	{ 0x011D, "g" },
	{ 0x011E, "G" },
	{ 0x011F, "g" },
	{ 0x0120, "G" },
	{ 0x0121, "g" },
	{ 0x0122, "G" },
	{ 0x0123, "g" },
	{ 0x0124, "H" },
	{ 0x0125, "h" },
	{ 0x0126, "H" },
	{ 0x0127, "h" },
	{ 0x0128, "I" },
	{ 0x0129, "i" },
	{ 0x012A, "I" },
	{ 0x012B, "i" },
	{ 0x012C, "I" },
	{ 0x012D, "i" },
	{ 0x012E, "I" },
	{ 0x012F, "i" },
	{ 0x0130, "I" },
	{ 0x0131, "i" },
	{ 0x0132, "IJ" },
	{ 0x0133, "ij" },
	{ 0x0134, "J" },
	{ 0x0135, "j" },
	{ 0x0136, "K" },
	{ 0x0137, "k" },
	{ 0x0139, "L" },
	{ 0x013A, "l" },
	{ 0x013B, "L" },
	{ 0x013C, "l" },
	{ 0x013D, "L" },
	{ 0x013E, "l" },
	{ 0x013F, "L" },
	{ 0x0140, "l" },
	{ 0x0141, "L" },
	{ 0x0142, "l" },
	{ 0x0143, "N" },
	{ 0x0144, "n" },
	{ 0x0145, "N" },
	{ 0x0146, "n" },
	{ 0x0147, "N" },
	{ 0x0148, "n" },
	{ 0x0149, "'n" },
	{ 0x014C, "O" },
	{ 0x014D, "o" },
	{ 0x014E, "O" },
	{ 0x014F, "o" },
	{ 0x0150, "O" },
	{ 0x0151, "o" },
	{ 0x0152, "OE" },
	{ 0x0153, "oe" },
	{ 0x0154, "R" },
	{ 0x0155, "r" },
	{ 0x0156, "R" },
	{ 0x0157, "r" },
	{ 0x0158, "R" },
	{ 0x0159, "r" },
	{ 0x015A, "S" },
	{ 0x015B, "s" },
	{ 0x015C, "S" },
	{ 0x015D, "s" },
	{ 0x015E, "S" },
	{ 0x015F, "s" },
	{ 0x0160, "S" },
	{ 0x0161, "s" },
	{ 0x0162, "T" },
	{ 0x0163, "t" },
	{ 0x0164, "T" },
	{ 0x0165, "t" },
	{ 0x0166, "T" },
	{ 0x0167, "t" },
	{ 0x0168, "U" },
	{ 0x0169, "u" },
	{ 0x016A, "U" },
	{ 0x016B, "u" },
	{ 0x016C, "U" },
	{ 0x016D, "u" },
	{ 0x016E, "U" },
	{ 0x016F, "u" },
	{ 0x0170, "U" },
	{ 0x0171, "u" },
	{ 0x0172, "U" },
	{ 0x0173, "u" },
	{ 0x0174, "W" },
	{ 0x0175, "w" },
	{ 0x0176, "Y" },
	{ 0x0177, "y" },
	{ 0x0178, "Y" },
	{ 0x0179, "Z" },
	{ 0x017A, "z" },
	{ 0x017B, "Z" },
	{ 0x017C, "z" },
	{ 0x017D, "Z" },
	{ 0x017E, "z" },
	{ 0x0180, "b" },
	{ 0x0181, "B" },
	{ 0x0182, "b" },
	{ 0x0183, "B" },
	{ 0x0189, "D" },
	{ 0x018A, "D" },
	{ 0x018B, "D" },
	{ 0x018C, "d" },
	{ 0x01C4, "DZ" },
	{ 0x01C5, "Dz" },
	{ 0x01C6, "dz" },
	{ 0x01C7, "LJ" },
	{ 0x01C8, "Lj" },
	{ 0x01C9, "lj" },
	{ 0x01CA, "NJ" },
	{ 0x01CB, "Nj" },
	{ 0x01CC, "nj" },
	{ 0x01CD, "A" },
	{ 0x01CE, "a" },
	{ 0x01CF, "I" },
	{ 0x01D0, "i" },
	{ 0x01D1, "O" },
	{ 0x01D2, "o" },
	{ 0x01D3, "U" },
	{ 0x01D4, "u" },
	{ 0x01D5, "U" },
	{ 0x01D6, "u" },
	{ 0x01D7, "U" },
	{ 0x01D8, "u" },
	{ 0x01D9, "U" },
	{ 0x01DA, "u" },
	{ 0x01DB, "U" },
	{ 0x01DC, "u" },
	{ 0x01DE, "A" },
	{ 0x01DF, "a" },
	{ 0x01E0, "A" },
	{ 0x01E1, "a" },
	{ 0x01E2, "AE" },
	{ 0x01E3, "ae" },
	{ 0x01E4, "G" },
	{ 0x01E5, "g" },
	{ 0x01E6, "G" },
	{ 0x01E7, "g" },
	{ 0x01E8, "K" },
	{ 0x01E9, "k" },
	{ 0x01EA, "O" },
	{ 0x01EB, "o" },
	{ 0x01EC, "O" },
	{ 0x01ED, "o" },
	{ 0x01F0, "j" },
	{ 0x01F1, "DZ" },
	{ 0x01F2, "Dz" },
	{ 0x01F3, "dz" },
	{ 0x01F4, "G" },
	{ 0x01F5, "g" },
	{ 0x01F8, "N" },
	{ 0x01F9, "n" },
	{ 0x01FA, "A" },
	{ 0x01FB, "a" },
	{ 0x01FC, "AE" },
	{ 0x01FD, "ae" },
	{ 0x01FE, "O" },
	{ 0x01FF, "o" },
	{ 0x0200, "A" },
	{ 0x0201, "a" },
	{ 0x0202, "A" },
	{ 0x0203, "a" },
	{ 0x0204, "E" },
	{ 0x0205, "e" },
	{ 0x0206, "E" },
	{ 0x0207, "e" },
	{ 0x0208, "I" },
	{ 0x0209, "i" },
	{ 0x020A, "I" },
	{ 0x020B, "i" },
	{ 0x020C, "O" },
	{ 0x020D, "o" },
	{ 0x020E, "O" },
	{ 0x020F, "o" },
	{ 0x0210, "R" },
	{ 0x0211, "r" },
	{ 0x0212, "R" },
	{ 0x0213, "r" },
	{ 0x0214, "U" },
	{ 0x0215, "u" },
	{ 0x0216, "U" },
	{ 0x0217, "u" },
	{ 0x0218, "S" },
	{ 0x0219, "s" },
	{ 0x021A, "T" },
	{ 0x021B, "t" },
	{ 0x021E, "H" },
	{ 0x021F, "h" },
	{ 0x0224, "Z" },
	{ 0x0225, "z" },
	{ 0x0226, "A" },
	{ 0x0227, "a" },
	{ 0x0228, "E" },
	{ 0x0229, "e" },
	{ 0x022A, "O" },
	{ 0x022B, "o" },
	{ 0x022C, "O" },
	{ 0x022D, "o" },
	{ 0x022E, "O" },
	{ 0x022F, "o" },
	{ 0x0230, "O" },
	{ 0x0231, "o" },
	{ 0x0232, "Y" },
	{ 0x0233, "y" },
	{ 0x023A, "A" },
	{ 0x023B, "C" },
	{ 0x023C, "c" },
	{ 0x023D, "L" },
	{ 0x023E, "T" },
	{ 0x023F, "s" },
	{ 0x0243, "B" },
	{ 0x0244, "U" },
	{ 0x0246, "E" },
	{ 0x0247, "e" },
	{ 0x0248, "J" },
	{ 0x0249, "j" },
	{ 0x024C, "R" },
	{ 0x024D, "r" },
	{ 0x024E, "Y" },
	{ 0x024F, "y" },
	{ 0x0260, "g" },
	{ 0x0260, "g" },
	{ 0x0260, "G" },
	{ 0x0266, "h" },
	{ 0x0268, "i" },
	{ 0x026A, "I" },
	{ 0x026B, "l" },
	{ 0x026C, "l" },
	{ 0x026D, "l" },
	{ 0x0274, "N" },
	{ 0x0276, "OE" },
	{ 0x0280, "R" },
	{ 0x0284, "j" },
	{ 0x028F, "Y" },
	{ 0x0290, "z" },
	{ 0x0291, "z" },
	{ 0x0299, "B" },
	{ 0x029B, "G" },
	{ 0x029C, "H" },
	{ 0x029D, "j" },
	{ 0x029F, "L" },
	{ 0x02A3, "dz" },
	{ 0x02A5, "dz" },
	{ 0x02A6, "ts" },
	{ 0x02A8, "tc" },
	{ 0x02AA, "ls" },
	{ 0x02AB, "lz" }
};

std::string Aircraft::to_ascii(const Glib::ustring& x)
{

	std::string r;
	bool tryiconv(false);
	for (Glib::ustring::const_iterator i(x.begin()), e(x.end()); i != e; ++i) {
		if (*i >= ' ' && *i < 127) {
			r.push_back(*i);
			continue;
		}
		std::pair<const UnicodeMapping *, const UnicodeMapping *> p(std::equal_range(&unicodemapping[0],
											     &unicodemapping[sizeof(unicodemapping)/sizeof(unicodemapping[0])],
											     *i, UnicodeMappingCompare()));
		if (p.first != p.second) {
			r += p.first->txt;
			continue;
		}
		tryiconv = true;
	}
	if (tryiconv) {
		std::string r2;
		if (to_ascii_iconv(r2, x))
			return r2;
	}
	return r;
}

// http://www.aircraftguru.com/aircraft/aircraft-information.php?craftid=127
// VR (no flaps, rotation): 45-55 kias (83-102 km/h)
// VR (25° flaps, rotation): 40-52 kias (74-96 km/h)
// VX (no flaps, best angle of climb): 63 kias (117 km/h)
// VX (25° flaps, best angle of climb): 44-57 kias (82-106 km/h)
// VY (best rate of climb): 79 kias (146 km/h)
// VA (maneuvering): 88-111 kias (163-206 km/h)
// VNO (max cruise): 126 kias (233 km/h)
// VNE (never exceed): 160 kias (296 km/h)
// VFE (flaps extended): 103 kias (191 km/h)
// VREF (no flaps, approach): 70 kias (130 km/h)
// VREF (40° flaps, approach): 63 kias (117 km/h)
// VS (stall, clean): 50 kias (93 km/h)
// VS0 (stall, dirty): 44 kias (82 km/h)
