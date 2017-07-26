/***************************************************************************
 *   Copyright (C) 2003, 2004, 2013 by Thomas Sailer                       *
 *   t.sailer@alumni.ethz.ch                                               *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 *   This program is distributed in the hope that it will be useful,       *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *   GNU General Public License for more details.                          *
 *                                                                         *
 *   You should have received a copy of the GNU General Public License     *
 *   along with this program; if not, write to the                         *
 *   Free Software Foundation, Inc.,                                       *
 *   59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.             *
 ***************************************************************************/

#include "sysdeps.h"

#include <sys/time.h>
#include <sstream>
#include <iomanip>
#include <limits>
#include <glibmm.h>
#include <giomm.h>

#include "bitmapmaps.h"

#ifdef HAVE_EIGEN3
#include <Eigen/Dense>
#endif

const struct BitmapMaps::Ellipsoid::ellipsoids BitmapMaps::Ellipsoid::ellipsoids[] = {
	{ "wgs84", 6378137, 1.0 / 298.257223563, 0, 0, 0 },
	{ "grs80", 6378137, 1.0 / 298.257222101, 0, 0, 0 },
	{ "bessel1841", 6377397.155, 1.0 / 299.15281285, 0, 0, 0 },
	{ 0, 0, 0, 0, 0, 0 }
};

BitmapMaps::Ellipsoid::Ellipsoid(void)
{
	set(ellipsoids);
}

bool BitmapMaps::Ellipsoid::set(const struct ellipsoids *ell)
{
	if (!ell)
		return false;
	if (!ell->name)
		return false;
        m_a = ell->a;
        m_f = ell->f;
        m_dx = ell->dx;
        m_dy = ell->dy;
        m_dz = ell->dz;
        m_b = m_a * (1 - m_f);
        m_e2 = m_f * (2.0 - m_f);
        m_da = 6378137 - m_a;
        m_df = (1.0 / 298.257223563) - m_f;
	return true;
}

bool BitmapMaps::Ellipsoid::set(const std::string& name)
{
	const struct ellipsoids *ell(ellipsoids);
	while (ell->name) {
		if (ell->name == name)
			return set(ell);
		++ell;
	}
	return false;
}

void BitmapMaps::Ellipsoid::diffwgs84(double& dlat, double& dlon, double& dh, double lat, double lon, double h)
{
#if defined(_GNU_SOURCE)
	double sinlat, coslat, sinlon, coslon;
	sincos(lat, &sinlat, &coslat);
	sincos(lon, &sinlon, &coslon);
#else
	double sinlat(sin(lat)), coslat(cos(lat)), sinlon(sin(lon)), coslon(cos(lon));
#endif
	double t1(1 - m_e2 * sinlat * sinlat);
	double t2(1.0 / sqrt(t1));
        double rn(m_a * t2);
	double rm(m_a * (1 - m_e2) * t2 * t2 * t2);
        if (&dlat)
                dlat = (- m_dx*sinlat*coslon - m_dy*sinlat*sinlon+m_dz*coslat
			+ m_da*(rn*m_e2*sinlat*coslat)/m_a
			+ m_df*(rm*m_a/m_b+rn*m_b/m_a)*sinlat*coslat) /
                        (rm+h);
        if (&dlon)
                dlon = (- m_dx*sinlon + m_dy*coslon) / ((rn+h)*coslat);
        if (&dh)
                dh = m_dx*coslat*coslon + m_dy*coslat*sinlon + m_dz*sinlat
                        - m_da*m_a/rn + m_df*m_b/m_a*rn*sinlat*sinlat;
}

void BitmapMaps::Ellipsoid::diffwgs84(double& dlat, double& dlon, double& dh, const Point& pt, double h)
{
	return diffwgs84(dlat, dlon, dh, pt.get_lat_rad_dbl(), pt.get_lon_rad_dbl(), h);
}

void BitmapMaps::Ellipsoid::diffwgs84(Point& dpt, double& dh, const Point& pt, double h)
{
	double dlat, dlon;
	diffwgs84(dlat, dlon, dh, pt, h);
	dpt.set_lat_rad_dbl(dlat);
	dpt.set_lon_rad_dbl(dlon);
}

BitmapMaps::GeoRefPoint::GeoRefPoint(unsigned int x, unsigned int y, const Point& pt, const std::string& name)
	: m_name(name), m_point(pt), m_x(x), m_y(y)
{
}

int BitmapMaps::GeoRefPoint::load_xml(const xmlpp::Element *el)
{
	if (!el)
		return -1;
	bool setlon(false), setlat(false), setx(false), sety(false);
        xmlpp::Attribute *attr;
        if ((attr = el->get_attribute("x"))) {
		const char *cp(attr->get_value().c_str());
		char *cp1;
		unsigned int x(strtoul(cp, &cp1, 0));
		if (cp1 != cp && cp1 && !*cp1) {
			m_x = x;
			setx = true;
		}
	}
	if ((attr = el->get_attribute("y"))) {
		const char *cp(attr->get_value().c_str());
		char *cp1;
		unsigned int y(strtoul(cp, &cp1, 0));
		if (cp1 != cp && cp1 && !*cp1) {
			m_y = y;
			sety = true;
		}
	}
        if ((attr = el->get_attribute("lon"))) {
		const char *cp(attr->get_value().c_str());
		char *cp1;
		double c(strtod(cp, &cp1));
		if (cp1 != cp && cp1 && !*cp1) {
			m_point.set_lon_deg_dbl(c);
			setlon = true;
		} else {
			Point pt;
			if (pt.set_str(attr->get_value()) & Point::setstr_lon) {
				m_point.set_lon(pt.get_lon());
				setlon = true;
			}
		}
	}
        if ((attr = el->get_attribute("lat"))) {
		const char *cp(attr->get_value().c_str());
		char *cp1;
		double c(strtod(cp, &cp1));
		if (cp1 != cp && cp1 && !*cp1) {
			m_point.set_lat_deg_dbl(c);
			setlat = true;
		} else {
			Point pt;
			if (pt.set_str(attr->get_value()) & Point::setstr_lat) {
				m_point.set_lat(pt.get_lat());
				setlat = true;
			}
		}
	}
	{
		const xmlpp::TextNode* txt(el->get_child_text());
		if (txt)
			m_name = txt->get_content();
	}
	if (!(setlon && setlat && setx && sety))
		return -1;
	return 0;
}

BitmapMaps::Projection::Projection(Point::coord_t reflon)
        : m_refcount(1), m_reflon(reflon)
{
	init_affine_transform();
}

BitmapMaps::Projection::~Projection()
{
}

void BitmapMaps::Projection::reference(void) const
{
        g_atomic_int_inc(&m_refcount);
}

void BitmapMaps::Projection::unreference(void) const
{
        if (!g_atomic_int_dec_and_test(&m_refcount))
                return;
        delete this;
}

int BitmapMaps::Projection::init_affine_transform(void)
{
	m_atfwd[0] = m_atfwd[4] = m_atrev[0] = m_atrev[3] = 1;
	m_atfwd[1] = m_atfwd[2] = m_atfwd[3] = m_atfwd[5] = m_atrev[1] = m_atrev[2] = 0;
	return 0;
}

int BitmapMaps::Projection::init_affine_transform(const georef_t& ref, int northx, int northy, double northdist)
{
#ifdef HAVE_EIGEN3
	bool hasnorth(northx && northy && !std::isnan(northdist) && northdist > 0);
	unsigned int matrixcol(2 * hasnorth);
	matrixcol += ref.size();
	if (matrixcol < 3) {
		init_affine_transform();
		return -1;
	}
	Eigen::MatrixXd C(matrixcol, 3);
	Eigen::VectorXd rx(matrixcol);
	Eigen::VectorXd ry(matrixcol);
	for (unsigned int i = 0, sz = ref.size(); i < sz; ++i) {
		const GeoRefPoint& refpt(ref[i]);
		double x1, y1;
		proj_normal(x1, y1, refpt.get_point().get_lat_rad_dbl(), refpt.get_point().get_lon_rad_dbl());
		rx(i) = refpt.get_x();
                ry(i) = refpt.get_y();
                C(i, 0) = x1;
                C(i, 1) = y1;
                C(i, 2) = 1;
	}
	if (hasnorth) {
		const GeoRefPoint& refpt(ref[0]);
		Point pt(refpt.get_point().spheric_course_distance_nmi(0, northdist));
		double x1, y1;
		proj_normal(x1, y1, pt.get_lat_rad_dbl(), pt.get_lon_rad_dbl());
		rx(matrixcol - 2) = refpt.get_x() + northx;
                ry(matrixcol - 2) = refpt.get_y() + northy;
                C(matrixcol - 2, 0) = x1;
                C(matrixcol - 2, 1) = y1;
                C(matrixcol - 2, 2) = 1;
		pt = refpt.get_point().spheric_course_distance_nmi(90, northdist);
		proj_normal(x1, y1, pt.get_lat_rad_dbl(), pt.get_lon_rad_dbl());
		rx(matrixcol - 1) = refpt.get_x() - northy;
                ry(matrixcol - 1) = refpt.get_y() + northx;
                C(matrixcol - 1, 0) = x1;
                C(matrixcol - 1, 1) = y1;
                C(matrixcol - 1, 2) = 1;

	}
	Eigen::MatrixXd CC((C.transpose() * C).inverse() * C.transpose());
	Eigen::VectorXd fx(CC * rx);
	Eigen::VectorXd fy(CC * ry);
	m_atfwd[0] = fx(0);
        m_atfwd[1] = fx(1);
        m_atfwd[2] = fx(2);
        m_atfwd[3] = fy(0);
        m_atfwd[4] = fy(1);
        m_atfwd[5] = fy(2);
        double detf(fx(0) * fy(1) - fx(1) * fy(0));
        if (detf == 0) {
		m_atrev[0] = m_atrev[1] = m_atrev[2] = m_atrev[3] = 0;
		return -1;
	}
        detf = 1.0 / detf;
        m_atrev[0] = fy(1) * detf;
        m_atrev[1] = -fx(1) * detf;
        m_atrev[2] = -fy(0) * detf;
        m_atrev[3] = fx(0) * detf;
	return 0;
#else
	init_affine_transform();
	return -1;
#endif
}

void BitmapMaps::Projection::proj_normal(double& x, double& y, double lat, double lon) const
{
        double x1, y1;
	proj_core_normal(x1, y1, lat, lon - m_reflon * Point::to_rad_dbl);
        if (&x)
                x = x1 * m_atfwd[0] + y1 * m_atfwd[1] + m_atfwd[2];
        if (&y)
                y = x1 * m_atfwd[3] + y1 * m_atfwd[4] + m_atfwd[5];
}

void BitmapMaps::Projection::proj_normal(double& x, double& y, const Point& pt) const
{
	proj_normal(x, y, pt.get_lat_rad_dbl(), pt.get_lon_rad_dbl());
}

void BitmapMaps::Projection::proj_reverse(double& lat, double& lon, double x, double y) const
{
        double lon1, lat1;
        x -= m_atfwd[2];
        y -= m_atfwd[5];
	proj_core_reverse(lat1, lon1, x * m_atrev[0] + y * m_atrev[1], x * m_atrev[2] + y * m_atrev[3]);
        if (&lat)
                lat = lat1;
        if (&lon)
                lon = lon1 + m_reflon * Point::to_rad_dbl;
}

void BitmapMaps::Projection::proj_reverse(Point& pt, double x, double y) const
{
	double lat, lon;
	proj_reverse(lat, lon, x, y);
	pt.set_lat_rad_dbl(lat);
	pt.set_lon_rad_dbl(lon);
}

BitmapMaps::ProjectionLinear::ProjectionLinear(Point::coord_t reflon)
	: Projection(reflon)
{
}

void BitmapMaps::ProjectionLinear::proj_core_normal(double& x, double& y, double lat, double lon) const
{
	x = lon;
	y = lat;
}

void BitmapMaps::ProjectionLinear::proj_core_reverse(double& lat, double& lon, double x, double y) const
{
	lon = x;
	lat = y;
}

// http://mathworld.wolfram.com/LambertConformalConicProjection.html

BitmapMaps::ProjectionLambert::ProjectionLambert(const Point& refpt, Point::coord_t stdpar1, Point::coord_t stdpar2)
	: Projection(refpt.get_lon()), m_n(1)
{
	double stdparrad1(stdpar1 * Point::to_rad_dbl);
	double stdparrad2(stdpar2 * Point::to_rad_dbl);
        if (fabs(stdparrad1 - stdparrad2) >= 0.000001)
                m_n = log(cos(stdparrad1) / cos(stdparrad2)) /
			log(tan((M_PI / 4) + 0.5 * stdparrad2) / tan((M_PI / 4) + 0.5 * stdparrad1));
        m_f = cos(stdparrad1) * pow(tan((M_PI / 4) + 0.5 * stdparrad1), m_n) / m_n;
        m_rho0 = m_f * pow(1 / tan((M_PI / 4) + 0.5 * refpt.get_lat_rad_dbl()), m_n);
}

void BitmapMaps::ProjectionLambert::proj_core_normal(double& x, double& y, double lat, double lon) const
{
	double rho(m_f * pow(1 / tan((M_PI / 4) + 0.5 * lat), m_n));
	x = rho * sin(m_n * lon);
	y = m_rho0 - rho * cos(m_n * lon);
}

void BitmapMaps::ProjectionLambert::proj_core_reverse(double& lat, double& lon, double x, double y) const
{
	double rho0my(m_rho0 - y);
	double rho(sqrt(x * x + rho0my * rho0my));
	if (m_n < 0)
		rho = -rho;
	double theta(atan2(x, rho0my));
	lon = theta / m_n;
	lat = 2 * atan(pow(m_f / rho, 1.0/m_n)) - 0.5*M_PI;
}

BitmapMaps::Map::Tile::Tile(void)
	: m_refcount(1), m_time(0), m_width(0), m_height(0), m_xmin(0), m_ymin(0),
	  m_xmax(std::numeric_limits<unsigned int>::max()),
	  m_ymax(std::numeric_limits<unsigned int>::max())
{
}

BitmapMaps::Map::Tile::~Tile()
{
}

void BitmapMaps::Map::Tile::reference(void) const
{
        g_atomic_int_inc(&m_refcount);
}

void BitmapMaps::Map::Tile::unreference(void) const
{
        if (!g_atomic_int_dec_and_test(&m_refcount))
                return;
        delete this;
}

Glib::RefPtr<Gdk::Pixbuf> BitmapMaps::Map::Tile::get_pixbuf(void)
{
	if (!m_pixbufcache) {
		try {
			m_pixbufcache = Gdk::Pixbuf::create_from_file(m_file);
			zap_border();
		} catch (...) {
			m_pixbufcache.clear();
		}
	}
	return m_pixbufcache;
}

void BitmapMaps::Map::Tile::zap_border(void)
{
	if (!m_pixbufcache)
		return;
	unsigned int rowstride(m_pixbufcache->get_rowstride());
	unsigned int bytes_per_pixel((m_pixbufcache->get_n_channels() * m_pixbufcache->get_bits_per_sample() + 7U) / 8U);
	guint8 *pix(m_pixbufcache->get_pixels());
	for (unsigned int i = 0; i < m_ymin; ++i)
		memset(pix + i * rowstride, 0xff, bytes_per_pixel * m_width);
	for (unsigned int i = m_ymax; i < m_height; ++i)
		memset(pix + i * rowstride, 0xff, bytes_per_pixel * m_width);
	for (unsigned int i = m_ymin; i < m_ymax; ++i) {
		if (m_xmin > 0)
			memset(pix + i * rowstride, 0xff, bytes_per_pixel * m_ymin);
		if (m_xmax < m_width)
			memset(pix + i * rowstride + bytes_per_pixel * m_xmax, 0xff, bytes_per_pixel * (m_width - m_xmax));
	}
}

double BitmapMaps::Map::Tile::get_nmi_per_pixel(void) const
{
	if (!m_projection)
		return std::numeric_limits<double>::quiet_NaN();
	double x(get_width() * 0.5), y(get_height() * 0.5);
	Point pt1, pt2;
	m_projection->proj_reverse(pt1, x, y);
	m_projection->proj_reverse(pt2, x + 1, y);
	return pt1.spheric_distance_nmi_dbl(pt2);
}

void BitmapMaps::Map::Tile::flush_cache(void)
{
	m_pixbufcache.clear();
}

bool BitmapMaps::Map::Tile::check_file(void) const
{
	return Glib::file_test(m_file, Glib::FILE_TEST_IS_REGULAR);
}

int BitmapMaps::Map::Tile::load_xml(const xmlpp::Element *el, const std::string& name)
{
	if (!el)
		return -1;
	bool hasnorth(false);
	int northx(0), northy(0);
	double northdist(0);
        xmlpp::Attribute *attr;
	if ((attr = el->get_attribute("file")))
                m_file = attr->get_value();
        if ((attr = el->get_attribute("time"))) {
		Glib::TimeVal tv;
		if (tv.assign_from_iso8601(attr->get_value())) {
			m_time = tv.tv_sec;
		} else {
			const char *cp(attr->get_value().c_str());
			char *cp1;
			unsigned int x(strtoul(cp, &cp1, 0));
			if (cp1 != cp && cp1 && !*cp1)
				m_time = x;
		}
	}
        if ((attr = el->get_attribute("xmin"))) {
		const char *cp(attr->get_value().c_str());
		char *cp1;
		unsigned int x(strtoul(cp, &cp1, 0));
		if (cp1 != cp && cp1 && !*cp1)
			m_xmin = x;
	}
	if ((attr = el->get_attribute("ymin"))) {
		const char *cp(attr->get_value().c_str());
		char *cp1;
		unsigned int x(strtoul(cp, &cp1, 0));
		if (cp1 != cp && cp1 && !*cp1)
			m_ymin = x;
	}
        if ((attr = el->get_attribute("xmax"))) {
		const char *cp(attr->get_value().c_str());
		char *cp1;
		unsigned int x(strtoul(cp, &cp1, 0));
		if (cp1 != cp && cp1 && !*cp1)
			m_xmax = x;
	}
	if ((attr = el->get_attribute("ymax"))) {
		const char *cp(attr->get_value().c_str());
		char *cp1;
		unsigned int x(strtoul(cp, &cp1, 0));
		if (cp1 != cp && cp1 && !*cp1)
			m_ymax = x;
	}
	{
                xmlpp::Node::NodeList nl(el->get_children("point"));
                for (xmlpp::Node::NodeList::const_iterator ni(nl.begin()), ne(nl.end()); ni != ne; ++ni) {
			GeoRefPoint p;
			int err1(p.load_xml(static_cast<xmlpp::Element *>(*ni)));
			if (err1)
				continue;
			m_georef.push_back(p);
		}
	}
	{
                xmlpp::Node::NodeList nl(el->get_children("north"));
                for (xmlpp::Node::NodeList::const_iterator ni(nl.begin()), ne(nl.end()); ni != ne; ++ni) {
			int nx(0), ny(0);
			double nd(0);
			if ((attr = el->get_attribute("headx"))) {
				const char *cp(attr->get_value().c_str());
				char *cp1;
				int x(strtol(cp, &cp1, 0));
				if (cp1 != cp && cp1 && !*cp1)
					nx += x;
				else
					continue;
			}
			if ((attr = el->get_attribute("heady"))) {
				const char *cp(attr->get_value().c_str());
				char *cp1;
				int y(strtol(cp, &cp1, 0));
				if (cp1 != cp && cp1 && !*cp1)
					ny += y;
				else
					continue;
			}
			if ((attr = el->get_attribute("tailx"))) {
				const char *cp(attr->get_value().c_str());
				char *cp1;
				int x(strtol(cp, &cp1, 0));
				if (cp1 != cp && cp1 && !*cp1)
					nx -= x;
				else
					continue;
			}
			if ((attr = el->get_attribute("taily"))) {
				const char *cp(attr->get_value().c_str());
				char *cp1;
				int y(strtol(cp, &cp1, 0));
				if (cp1 != cp && cp1 && !*cp1)
					ny -= y;
				else
					continue;
			}
			if ((attr = el->get_attribute("dist"))) {
				const char *cp(attr->get_value().c_str());
				char *cp1;
				double d(strtod(cp, &cp1));
				if (cp1 != cp && cp1 && !*cp1)
					nd = d;
				else
					continue;
			}
			hasnorth = true;
			northx = nx;
			northy = ny;
			northdist = nd;
		}
	}
	if (m_file.empty())
		return -1;
	if (m_georef.empty() || (!hasnorth && m_georef.size() < 3)) {
		if (true)
			std::cerr << "Bitmap " << name << " has not enough reference points " << m_georef.size() << (hasnorth ? " (north)" : "") << std::endl;
		return -1;
	}
	if (!check_file()) {
		if (true)
			std::cerr << "Bitmap " << name << " file " << m_file << " not found" << std::endl;
		return -1;
	}
	Ellipsoid ell;
	if ((attr = el->get_attribute("mapdatum"))) {
                if (!ell.set(attr->get_value())) {
			if (true)
				std::cerr << "Bitmap " << name << " file " << m_file << " invalid ellipsoid " << attr->get_value() << std::endl;
			return -1;
		}
	}
	attr = el->get_attribute("projection");
	if (!attr || attr->get_value() == "linear") {
		Point::coord_t reflon(m_georef[0].get_point().get_lon());
		if ((attr = el->get_attribute("reflon"))) {
			const char *cp(attr->get_value().c_str());
			char *cp1;
			double c(strtod(cp, &cp1));
			if (cp1 != cp && cp1 && !*cp1) {
				Point pt;
				pt.set_lon_deg_dbl(c);
				reflon = pt.get_lon();
			} else {
				Point pt;
				if (pt.set_str(attr->get_value()) & Point::setstr_lon)
					reflon = pt.get_lon();
			}
		}
		Glib::RefPtr<ProjectionLinear> p(new ProjectionLinear(reflon));
		m_projection = p;
	} else if (attr->get_value() == "lambert") {
		Point refpoint(m_georef[0].get_point());
		Point::coord_t stdpar1(Point::lon_min);
		Point::coord_t stdpar2(Point::lon_min);
		if ((attr = el->get_attribute("reflat"))) {
			const char *cp(attr->get_value().c_str());
			char *cp1;
			double c(strtod(cp, &cp1));
			if (cp1 != cp && cp1 && !*cp1) {
				Point pt;
				pt.set_lat_deg_dbl(c);
				refpoint.set_lat(pt.get_lat());
			} else {
				Point pt;
				if (pt.set_str(attr->get_value()) & Point::setstr_lat)
				        refpoint.set_lat(pt.get_lat());
			}
		}
		if ((attr = el->get_attribute("reflon"))) {
			const char *cp(attr->get_value().c_str());
			char *cp1;
			double c(strtod(cp, &cp1));
			if (cp1 != cp && cp1 && !*cp1) {
				Point pt;
				pt.set_lon_deg_dbl(c);
				refpoint.set_lon(pt.get_lon());
			} else {
				Point pt;
				if (pt.set_str(attr->get_value()) & Point::setstr_lon)
				        refpoint.set_lon(pt.get_lon());
			}
		}
		if ((attr = el->get_attribute("stdparallel"))) {
			const char *cp(attr->get_value().c_str());
			char *cp1;
			double c(strtod(cp, &cp1));
			if (cp1 != cp && cp1 && !*cp1) {
				Point pt;
				pt.set_lat_deg_dbl(c);
			        stdpar1 = stdpar2 = pt.get_lat();
			} else {
				Point pt;
				if (pt.set_str(attr->get_value()) & Point::setstr_lat)
				        stdpar1 = stdpar2 = pt.get_lat();
			}
		}
		if ((attr = el->get_attribute("stdparallel1"))) {
			const char *cp(attr->get_value().c_str());
			char *cp1;
			double c(strtod(cp, &cp1));
			if (cp1 != cp && cp1 && !*cp1) {
				Point pt;
				pt.set_lat_deg_dbl(c);
			        stdpar1 = pt.get_lat();
			} else {
				Point pt;
				if (pt.set_str(attr->get_value()) & Point::setstr_lat)
				        stdpar1 = pt.get_lat();
			}
		}
		if ((attr = el->get_attribute("stdparallel2"))) {
			const char *cp(attr->get_value().c_str());
			char *cp1;
			double c(strtod(cp, &cp1));
			if (cp1 != cp && cp1 && !*cp1) {
				Point pt;
				pt.set_lat_deg_dbl(c);
				stdpar2 = pt.get_lat();
			} else {
				Point pt;
				if (pt.set_str(attr->get_value()) & Point::setstr_lat)
					stdpar2 = pt.get_lat();
			}
		}
		if (stdpar1 > Point::lat_max || stdpar1 < Point::lat_min ||
		    stdpar2 > Point::lat_max || stdpar2 < Point::lat_min) {
			if (true)
				std::cerr << "Bitmap " << name << " Tile " << m_file << " invalid lambert standard parallels" << std::endl;
			return -1;
		}
		Glib::RefPtr<ProjectionLambert> p(new ProjectionLambert(refpoint, stdpar1, stdpar2));
		m_projection = p;
	} else {
		if (true)
			std::cerr << "Bitmap " << name << " Tile " << m_file << " invalid projection " << attr->get_value() << std::endl;
		return -1;
	}
	{
		int err(m_projection->init_affine_transform(m_georef, hasnorth ? northx : 0, hasnorth ? northy : 0, hasnorth ? northdist : 0));
		if (err) {
			if (true)
				std::cerr << "Bitmap " << name << " Tile " << m_file << " cannot compute affine transform" << std::endl;
			return err;
		}
	}
	try {
		m_pixbufcache = Gdk::Pixbuf::create_from_file(m_file);
	} catch (...) {
		m_pixbufcache.clear();
	}
	if (!m_pixbufcache) {
		if (true)
			std::cerr << "Bitmap " << name << " Tile " << m_file << " cannot load bitmap" << std::endl;
		return -1;
	}
	{
		m_width = m_pixbufcache->get_width();
		m_height = m_pixbufcache->get_height();
		m_xmin = std::min(m_xmin, m_width);
		m_ymin = std::min(m_ymin, m_height);
		m_xmax = std::min(m_xmax, m_width);
		m_ymax = std::min(m_ymax, m_height);
		if (m_xmin > m_xmax)
			std::swap(m_xmin, m_xmax);
		if (m_ymin > m_ymax)
			std::swap(m_ymin, m_ymax);
	}
	zap_border();
	if (!m_time) {
		try {
			Glib::RefPtr<Gio::File> f(Gio::File::create_for_path(m_file));
			Glib::RefPtr<Gio::FileInfo> info(f->query_info("*", Gio::FILE_QUERY_INFO_NONE));
			Glib::TimeVal tv(info->modification_time()); 
			m_time = tv.tv_sec;
		} catch (...) {
			if (true)
				std::cerr << "Bitmap " << name << " Tile " << m_file << " cannot get file time" << std::endl;
			return -1;
		}
	}
	{
		Point sw, ne;
		m_projection->proj_reverse(sw, 0, m_height);
		m_projection->proj_reverse(ne, m_width, 0);
		m_bbox = Rect(sw, ne);
	}
	// transform statistics
	if (true) {
		double pixerr(0), disterr(0);
		for (unsigned int i = 0, sz = m_georef.size(); i < sz; ++i) {
			const GeoRefPoint& refpt(m_georef[i]);
			double x, y;
			m_projection->proj_normal(x, y, refpt.get_point());
			x -= refpt.get_x();
			y -= refpt.get_y();
			double pixe(x * x + y * y);
			pixerr += pixe;
			Point pt;
			m_projection->proj_reverse(pt, refpt.get_x(), refpt.get_y());
			double dist(refpt.get_point().spheric_distance_nmi_dbl(pt));
			disterr += dist * dist;
			if (false) {
				std::cerr << "Bitmap " << name << " Tile " << m_file << " point ";
				if (refpt.get_name().empty())
					std::cerr << i;
				else
					refpt.get_name();
				std::cerr << " errors: pixel " << sqrt(pixe) << " dist " << dist << std::endl;
			}
		}
		{
			double x(1.0 / m_georef.size());
			pixerr *= x;
			disterr *= x;
		}
		pixerr = sqrt(pixerr);
		disterr = sqrt(pixerr);
		std::cerr << "Bitmap " << name << " Tile " << m_file << " RMS errors: pixel " << pixerr << " dist " << disterr << std::endl;
	}
	return 0;
}

BitmapMaps::Map::Map(void)
        : m_refcount(1), m_scale(0)
{
}

BitmapMaps::Map::~Map()
{
}

void BitmapMaps::Map::reference(void) const
{
        g_atomic_int_inc(&m_refcount);
}

void BitmapMaps::Map::unreference(void) const
{
        if (!g_atomic_int_dec_and_test(&m_refcount))
                return;
        delete this;
}

BitmapMaps::Map::Tile::ptr_t BitmapMaps::Map::operator[](unsigned int x)
{
	if (x >= size())
		return Tile::ptr_t();
	return m_tiles[x];
}

bool BitmapMaps::Map::check_file(void) const
{
	for (tiles_t::const_iterator i(m_tiles.begin()), e(m_tiles.end()); i != e; ++i)
		if ((*i) && (*i)->check_file())
			return true;
	return false;
}

int BitmapMaps::Map::load_xml(const xmlpp::Element *el)
{
	if (!el)
		return -1;
        xmlpp::Attribute *attr;
        if ((attr = el->get_attribute("name")))
                m_name = attr->get_value();
        if ((attr = el->get_attribute("scale"))) {
		const char *cp(attr->get_value().c_str());
		char *cp1;
		unsigned int x(strtoul(cp, &cp1, 0));
		if (cp1 != cp && cp1 && !*cp1)
			m_scale = x;
	}
	int err(0);
	{
                xmlpp::Node::NodeList nl(el->get_children("Tile"));
                for (xmlpp::Node::NodeList::const_iterator ni(nl.begin()), ne(nl.end()); ni != ne; ++ni) {
			Tile::ptr_t t(new Tile());
			int err1(t->load_xml(static_cast<xmlpp::Element *>(*ni), m_name));
			if (err1) {
				err = err1;
				continue;
			}
			m_tiles.push_back(t);
		}
		if (nl.empty()) {
			Tile::ptr_t t(new Tile());
			int err1(t->load_xml(el, m_name));
			if (err1)
				err = err1;
			else
				m_tiles.push_back(t);
		}
	}
	clear_nonexisting_tiles();
	if (m_name.empty() || m_tiles.empty() || err)
		return -1;
	if (!m_scale) {
		if (true)
			std::cerr << "Bitmap " << m_name << " has no scale" << std::endl;
		return -1;
	}
	if (!check_file()) {
		if (true)
			std::cerr << "Bitmap " << m_name << " has no tiles with existing map files" << std::endl;
		return -1;
	}
	return 0;
}

double BitmapMaps::Map::get_nmi_per_pixel(void) const
{
	if (m_tiles.empty())
		return std::numeric_limits<double>::quiet_NaN();
	double npp(std::numeric_limits<double>::quiet_NaN());
	unsigned int cnt(0);
	for (tiles_t::const_iterator i(m_tiles.begin()), e(m_tiles.end()); i != e; ++i) {
		if (!*i)
			continue;
		double npp1((*i)->get_nmi_per_pixel());
		if (std::isnan(npp1))
			continue;
		if (!cnt)
			npp = npp1;
		else
			npp += npp1;
		++cnt;
	}
	if (cnt)
		npp /= cnt;
	return npp;
}

time_t BitmapMaps::Map::get_time(void) const
{
	time_t tm(0);
	for (tiles_t::const_iterator i(m_tiles.begin()), e(m_tiles.end()); i != e; ++i)
		if (*i)
			tm = std::max(tm, (*i)->get_time());
	return tm;
}

Rect BitmapMaps::Map::get_bbox(void) const
{
	Rect bbox(Point(Point::lon_min, Point::lon_min), Point(Point::lon_min, Point::lon_min));
	bool first(true);
	for (tiles_t::const_iterator i(m_tiles.begin()), e(m_tiles.end()); i != e; ++i) {
		if (!*i)
			continue;
		const Rect& bbox1((*i)->get_bbox());
		if (first) {
			bbox = bbox1;
			first = false;
			continue;
		}
		bbox = bbox.add(bbox1.get_southwest());
		bbox = bbox.add(bbox1.get_northeast());
	}
	return bbox;
}

void BitmapMaps::Map::flush_cache(void)
{
	for (tiles_t::iterator i(m_tiles.begin()), e(m_tiles.end()); i != e; ++i)
		if (*i)
			(*i)->flush_cache();
}

void BitmapMaps::Map::clear_nonexisting_tiles(void)
{
	for (tiles_t::iterator i(m_tiles.begin()), e(m_tiles.end()); i != e;) {
		if ((*i) && (*i)->check_file()) {
			++i;
			continue;
		}
		i = m_tiles.erase(i);
		e = m_tiles.end();
	}
}

BitmapMaps::BitmapMaps(void)
{
}

unsigned int BitmapMaps::size(void) const
{
	unsigned int sz;
	{
		Glib::Mutex::Lock lock(m_mutex);
		sz = m_maps.size();
	}
	return sz;
}

bool BitmapMaps::empty(void) const
{
	bool e;
	{
		Glib::Mutex::Lock lock(m_mutex);
		e = m_maps.empty();
	}
	return e;
}

BitmapMaps::Map::ptr_t BitmapMaps::operator[](unsigned int x) const
{
	Map::ptr_t p;
	{
		Glib::Mutex::Lock lock(m_mutex);
		if (x < m_maps.size())
			p = m_maps[x];
	}
	return p;
}

unsigned int BitmapMaps::find_index(const std::string& nm) const
{
	if (nm.empty())
		return ~0U;
	Glib::Mutex::Lock lock(m_mutex);
	for (unsigned int i = 0, sz = m_maps.size(); i < sz; ++i) {
		const Map::ptr_t m(m_maps[i]);
		if (m && m->get_name() == nm)
			return i;
	}
	return ~0U;
}

unsigned int BitmapMaps::find_index(const Map::const_ptr_t& p) const
{
	if (!p)
		return ~0U;
	return find_index(p->get_name());
}

unsigned int BitmapMaps::find_index(const Map::ptr_t& p) const
{
	if (!p)
		return ~0U;
	return find_index(p->get_name());
}

int BitmapMaps::parse_file(const std::string& fn)
{
        if (!Glib::file_test(fn, Glib::FILE_TEST_IS_REGULAR))
		return -1;
	xmlpp::DomParser p;
	try {
		p.parse_file(fn);
	} catch (...) {
		return false;
	}
        if (!p)
                return false;
        xmlpp::Document *doc(p.get_document());
        xmlpp::Element *el(doc->get_root_node());
        if (el->get_name() != "MovingMap")
                return -1;
	int err(0);
	{
                xmlpp::Node::NodeList nl(el->get_children("map"));
                for (xmlpp::Node::NodeList::const_iterator ni(nl.begin()), ne(nl.end()); ni != ne; ++ni) {
			xmlpp::Element *el(static_cast<xmlpp::Element *>(*ni));
			if (!el)
				continue;
			{
				xmlpp::Attribute *attr;
				if (!(attr = el->get_attribute("name")))
					continue;
				std::string nm(attr->get_value());
				bool duplicate(false);
				Glib::Mutex::Lock lock(m_mutex);
				for (maps_t::const_iterator i(m_maps.begin()), e(m_maps.end()); i != e; ++i) {
					if ((*i) && (*i)->get_name() != nm)
						continue;
					duplicate = true;
					break;
				}
				if (duplicate)
					continue;
			}
			Map::ptr_t m(new Map());
			int err1(m->load_xml(el));
			if (err1) {
				err = err1;
				continue;
			}
			m->flush_cache();
			{
				Glib::Mutex::Lock lock(m_mutex);
				m_maps.push_back(m);
			}
		}
	}
	return err;
}

int BitmapMaps::parse_directory(const std::string& dir)
{
        int err(0), lay(0);
        Glib::Dir d(dir);
        for (;;) {
                std::string f(d.read_name());
                if (f.empty())
                        break;
                int r(parse(Glib::build_filename(dir, f)));
                if (r < 0)
                        err = r;
                else
                        lay += r;
        }
        if (lay)
                return lay;
        return err;
}

int BitmapMaps::parse(const std::string& p)
{
        if (Glib::file_test(p, Glib::FILE_TEST_IS_DIR))
                return parse_directory(p);
        if (Glib::file_test(p, Glib::FILE_TEST_IS_REGULAR))
                return parse_file(p);
        return -1;
}

namespace
{
        struct mapsort {
                int compare(const BitmapMaps::Map::const_ptr_t& a, const BitmapMaps::Map::const_ptr_t& b) const {
			int c(a->get_name().compare(b->get_name()));
			if (c)
				return c;
			if (a->get_scale() < b->get_scale())
				return -1;
			if (b->get_scale() < a->get_scale())
				return 1;
			return a->get_time() < b->get_time();
		}

                bool operator()(const BitmapMaps::Map::const_ptr_t& a, const BitmapMaps::Map::const_ptr_t& b) const {
                        return compare(a, b) < 0;
                }
        };
};

void BitmapMaps::sort(void)
{
	Glib::Mutex::Lock lock(m_mutex);
	std::sort(m_maps.begin(), m_maps.end(), mapsort());
}

void BitmapMaps::check_files(void)
{
	Glib::Mutex::Lock lock(m_mutex);
	for (maps_t::iterator i(m_maps.begin()), e(m_maps.end()); i != e;) {
		if ((*i) && (*i)->check_file()) {
			++i;
			continue;
		}
		i = m_maps.erase(i);
		e = m_maps.end();
	}
}
