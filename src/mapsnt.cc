/***************************************************************************
 *   Copyright (C) 2007, 2008, 2009, 2011, 2012, 2013, 2015, 2016, 2017    *
 *     by Thomas Sailer                                                    *
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

#ifndef HAVE_MULTITHREADED_MAPS

#include <sys/time.h>
#include <sstream>
#include <iomanip>
#include <gdk/gdkkeysyms.h>
#include <set>

#ifdef HAVE_EIGEN3
#include <Eigen/Dense>
#include <Eigen/Cholesky>
#include <unsupported/Eigen/Polynomials>
#endif

#include "mapsnt.h"
#include "wind.h"
#include "grib2.h"
#include "baro.h"

template <typename T> const typename MapRenderer::ImageBuffer<T>::float_t MapRenderer::ImageBuffer<T>::min_scale;
template <typename T> const typename MapRenderer::ImageBuffer<T>::float_t MapRenderer::ImageBuffer<T>::max_scale;
template <typename T> const typename MapRenderer::ImageBuffer<T>::float_t MapRenderer::ImageBuffer<T>::arpt_min_scale;
template <typename T> const typename MapRenderer::ImageBuffer<T>::float_t MapRenderer::ImageBuffer<T>::arpt_max_scale;

template <typename T> MapRenderer::ImageBuffer<T>::ImageBuffer(void)
	: m_surface(), m_imgsize(16, 16), m_imgcenter(), m_scale(0.1), m_upangle(0)
{
	reset();
}

template <typename T> MapRenderer::ImageBuffer<T>::ImageBuffer(const ScreenCoord& imgsize, const Point& center, float_t nmi_per_pixel, uint16_t upangle,
							       int alt, int maxalt, int64_t time, const GlideModel& glidemodel, float_t winddir, float_t windspeed,
							       DrawFlags flags, const ScreenCoord& offset)
	: m_surface(), m_imgsize(imgsize), m_center(center), m_imgcenter(center), m_glidemodel(glidemodel), m_scale(nmi_per_pixel),
	  m_winddir(winddir), m_windspeed(windspeed), m_alt(alt), m_maxalt(maxalt), m_time(time), m_upangle(upangle), m_drawflags(flags)
{
	reset(imgsize, center, nmi_per_pixel, upangle, alt, maxalt, time, glidemodel, winddir, windspeed, flags, offset);
}

template <typename T> void MapRenderer::ImageBuffer<T>::reset(void)
{
	m_surface.clear();
	memset(m_invmatrix, 0, sizeof(m_invmatrix));
	memset(m_invmatrix, 0, sizeof(m_invmatrix));
}

template <typename T> void MapRenderer::ImageBuffer<T>::reset(const ScreenCoord& imgsize, const Point& center, float_t nmi_per_pixel, uint16_t upangle,
							      int alt, int maxalt, int64_t time, const GlideModel& glidemodel, float_t winddir, float_t windspeed,
							      DrawFlags flags, const ScreenCoord& offset)
{
	m_imgsize = imgsize;
	m_center = m_imgcenter = center;
	m_scale = std::max(arpt_min_scale, std::min(max_scale, nmi_per_pixel));
	m_upangle = upangle;
	m_alt = alt;
	m_maxalt = maxalt;
	m_time = time;
	m_glidemodel = glidemodel;
	m_winddir = winddir;
	m_windspeed = windspeed;
	m_drawflags = flags;
	// initialize forward matrix
	m_matrix[0] = m_matrix[3] = 0;
	m_matrix[1] = (float_t)(60.0 / Point::from_deg) / get_scale();
	m_matrix[2] = -m_matrix[1];
	{
		/*
		 * mercator lat formula: ln(tan(pi/4+lat/2))
		 * mercator lat scale factor (1/2+1/2*tan(1/2*lat+1/4*pi)^2)*tan(1/2*lat+1/4*pi)^(-1)
		 * lon scale factor approx: 1 - 1/2*lat^2 + 1/24*lat^4 - 1/720 * lat^6 ...
		 */
		/*
		 * alternative mercator lat formula: asinh(tan(lat))
		 * mercator lat scale factor sqrt(1+tan(lat)^2)
		 * lon scale factor approximation: 1 - 1/2*lat^2 + 1/24*lat^4 - 1/720*lat^6 + 1/40320*lat^8 - ...
		 */
		float_t l = get_imagecenter().get_lat_rad();
		l *= l;
		float_t c = 1 - l * (float_t)(1.0f / 2.0f);
		l *= l;
		c += l * (float_t)(1.0f / 24.0f);
		m_matrix[1] *= c;
	}
	if (get_upangle()) {
		float s, c;
		sincosf(get_upangle() * MapRenderer::to_rad_16bit, &s, &c);
		// compute [c s ; -s c ] * [m0 m1 ; m2 m3 ]
		float_t r[4];
		r[0] = c * m_matrix[0] + s * m_matrix[2];
		r[1] = c * m_matrix[1] + s * m_matrix[3];
		r[2] = c * m_matrix[2] - s * m_matrix[0];
		r[3] = c * m_matrix[3] - s * m_matrix[1];
		for (int i = 0; i < 4; ++i)
			m_matrix[i] = r[i];
	}
	// compute inverse transform
	{
		float_t invdetA((float_t)(1.) / (m_matrix[0] * m_matrix[3] - m_matrix[1] * m_matrix[2]));
		m_invmatrix[0] = invdetA * m_matrix[3];
		m_invmatrix[1] = -invdetA * m_matrix[1];
		m_invmatrix[2] = -invdetA * m_matrix[2];
		m_invmatrix[3] = invdetA * m_matrix[0];
	}
	m_imgcenter += transform_distance(offset.getx(), offset.gety());
	// allocate surface
	m_surface = Cairo::ImageSurface::create(/*Cairo::FORMAT_ARGB32*/Cairo::FORMAT_RGB24, get_imagesize().getx(), get_imagesize().gety());
	if (true)
		std::cerr << "ImageBuffer: new image: size " << get_imagesize() << " center " << get_imagecenter()
			  << " scale " << get_scale() << " north " << get_upangle() << " alt " << get_altitude()
			  << " maxalt " << get_maxalt() << " time " << get_time() << " flags " << get_drawflags() << " offset " << offset << " requested center " << center
			  << " matrix [ " << m_matrix[0] << ' ' << m_matrix[1] << " ; " << m_matrix[2] << ' ' << m_matrix[3]
			  << " ] invmatrix [ " << m_invmatrix[0] << ' ' << m_invmatrix[1] << " ; " << m_invmatrix[2] << ' ' << m_invmatrix[3]
			  << " ]" << std::endl;
}

template <typename T> void MapRenderer::ImageBuffer<T>::flush_surface(void)
{
	if (!m_surface)
		return;
	m_surface->flush();
}

template <typename T> Cairo::RefPtr<Cairo::Context> MapRenderer::ImageBuffer<T>::create_context(void)
{
	if (m_surface)
		return Cairo::Context::create(m_surface);
	return Cairo::RefPtr<Cairo::Context>();
}

template <typename T> Cairo::RefPtr<Cairo::SurfacePattern> MapRenderer::ImageBuffer<T>::create_pattern(void)
{
	if (m_surface)
		return Cairo::SurfacePattern::create(m_surface);
	return Cairo::RefPtr<Cairo::SurfacePattern>();
}

template <typename T> Rect MapRenderer::ImageBuffer<T>::coverrect(float_t xmul, float_t ymul) const
{
	float_t x(get_imagesize().getx() * 0.5 * xmul);
	float_t y(get_imagesize().gety() * 0.5 * ymul);
	Point pt1(transform_distance(x, y));
	Point pt2(transform_distance(x, -y));
	pt1.set_lat(std::max(abs(pt1.get_lat()), abs(pt2.get_lat())));
	pt1.set_lon(std::max(abs(pt1.get_lon()), abs(pt2.get_lon())));
	return Rect(-pt1, pt1) + get_imagecenter();
}

template <typename T> Cairo::Matrix MapRenderer::ImageBuffer<T>::cairo_matrix(void) const
{
	Cairo::Matrix m;
	cairo_matrix_init(&m, m_matrix[0], m_matrix[2], m_matrix[1], m_matrix[3],
			  get_imagesize().getx() * 0.5, get_imagesize().gety() * 0.5);
	return m;
}

template <typename T> Cairo::Matrix MapRenderer::ImageBuffer<T>::cairo_matrix(const Point& origin) const
{
	ScreenCoordTemplate<T> offs((*this)(origin));
	Cairo::Matrix m;
	cairo_matrix_init(&m, m_matrix[0], m_matrix[2], m_matrix[1], m_matrix[3], offs.getx(), offs.gety());
	return m;
}

template <typename T> Cairo::Matrix MapRenderer::ImageBuffer<T>::cairo_matrix_inverse(void) const
{
	float_t tx(get_imagesize().getx() * -0.5);
	float_t ty(get_imagesize().gety() * -0.5);
	Cairo::Matrix m;
	cairo_matrix_init(&m, m_invmatrix[0], m_invmatrix[2], m_invmatrix[1], m_invmatrix[3],
			  tx * m_invmatrix[0] + ty * m_invmatrix[2], tx * m_invmatrix[1] + ty * m_invmatrix[3]);
	return m;
}

template <typename T> typename MapRenderer::ScreenCoordTemplate<T> MapRenderer::ImageBuffer<T>::operator()(const Point& pt) const
{
	return transform_distance(pt - get_imagecenter()) + ScreenCoordTemplate<float_t>(get_imagesize().getx() * 0.5, get_imagesize().gety() * 0.5);
}

template <typename T> Point MapRenderer::ImageBuffer<T>::operator()(const ScreenCoord& sc) const
{
	return (*this)(sc.getx(), sc.gety());
}

template <typename T> Point MapRenderer::ImageBuffer<T>::operator()(const ScreenCoordFloat& sc) const
{
	return (*this)(sc.getx(), sc.gety());
}

template <typename T> Point MapRenderer::ImageBuffer<T>::operator()(float_t x, float_t y) const
{
	return transform_inv(x, y) + get_imagecenter();
}

template <typename T> Point MapRenderer::ImageBuffer<T>::transform_inv(float_t x, float_t y) const
{
	return transform_distance(x - get_imagesize().getx() * 0.5, y - get_imagesize().gety() * 0.5);
}

template <typename T> typename MapRenderer::ScreenCoordTemplate<T> MapRenderer::ImageBuffer<T>::transform_distance(const Point& pt) const
{
	float_t x(pt.get_lat() * m_matrix[0] + pt.get_lon() * m_matrix[1]);
	float_t y(pt.get_lat() * m_matrix[2] + pt.get_lon() * m_matrix[3]);
	return ScreenCoordTemplate<float_t>(x, y);
}

template <typename T> Point MapRenderer::ImageBuffer<T>::transform_distance(const ScreenCoord& sc) const
{
	return transform_distance(sc.getx(), sc.gety());
}

template <typename T> Point MapRenderer::ImageBuffer<T>::transform_distance(const ScreenCoordFloat& sc) const
{
	return transform_distance(sc.getx(), sc.gety());
}

template <typename T> Point MapRenderer::ImageBuffer<T>::transform_distance(float_t x, float_t y) const
{
	float_t xx(x * m_invmatrix[0] + y * m_invmatrix[1]);
	float_t yy(x * m_invmatrix[2] + y * m_invmatrix[3]);
	return Point(Point::round<Point::coord_t,float_t>(yy), Point::round<Point::coord_t,float_t>(xx));
}

template class MapRenderer::ScreenCoordTemplate<int>;
template class MapRenderer::ScreenCoordTemplate<float>;
template class MapRenderer::ImageBuffer<float>;

constexpr float MapRenderer::to_deg_16bit;
constexpr float MapRenderer::to_rad_16bit;
constexpr float MapRenderer::from_deg_16bit;
constexpr float MapRenderer::from_rad_16bit;

constexpr double MapRenderer::to_deg_16bit_dbl;
constexpr double MapRenderer::to_rad_16bit_dbl;
constexpr double MapRenderer::from_deg_16bit_dbl;
constexpr double MapRenderer::from_rad_16bit_dbl;

MapRenderer::GlideModel::GlideModel(void)
	: m_ceiling(std::numeric_limits<double>::quiet_NaN()),
	  m_climbtime(std::numeric_limits<double>::quiet_NaN())
{
}

MapRenderer::GlideModel::GlideModel(const Aircraft::ClimbDescent& cd)
	: m_climbaltpoly(cd.get_climbaltpoly()), m_climbdistpoly(cd.get_climbdistpoly()),
	  m_ceiling(cd.get_ceiling()), m_climbtime(cd.get_climbtime())
{
}

bool MapRenderer::GlideModel::is_invalid(void) const
{
	return std::isnan(get_ceiling()) || std::isnan(get_climbtime()) ||
		get_ceiling() <= 0 || get_climbtime() <= 0 ||
		get_climbaltpoly().empty() || get_climbdistpoly().empty() ||
		std::isnan(time_to_altitude(0)) || std::isnan(time_to_distance(0));
}

bool MapRenderer::GlideModel::is_approxequal(const GlideModel& x) const
{
	{
		bool vx(x.is_invalid()), v(is_invalid());
		if (vx != v)
			return false;
		if (v)
			return true;
	}
	constexpr double tolerance = 0.05;
	if (fabs(x.get_ceiling() - get_ceiling()) > tolerance * get_ceiling())
		return false;
	if (fabs(x.get_climbtime() - get_climbtime()) > tolerance * get_climbtime())
		return false;
	{
		double vx(x.time_to_altitude(get_climbtime())), v(time_to_altitude(get_climbtime()));
		if (fabs(vx - v) > tolerance * v)
			return false;
	}
	{
		double vx(x.time_to_distance(get_climbtime())), v(time_to_distance(get_climbtime()));
		if (fabs(vx - v) > tolerance * v)
			return false;
	}
	return true;
}

double MapRenderer::GlideModel::compute_newalt(Wind<double>& wind, const Point& p0, const Point& p1, double alt0) const
{
	constexpr double nmi_to_ft = 1.0 / Point::km_to_nmi_dbl * 1e3 * Point::m_to_ft_dbl;
	constexpr double maxglide = 0;
	std::pair<Point,double> gcnav(p0.get_gcnav(p1, 0.5));
	if (std::isnan(alt0) || alt0 >= get_ceiling())
		return std::numeric_limits<double>::quiet_NaN();
	double t0(altitude_to_time(alt0));
	wind.set_crs_tas(gcnav.second, time_to_tas(t0));
	double d0(time_to_distance(t0));
	double dist(p0.spheric_distance_nmi_dbl(p1));
	double d1(d0 - dist * wind.get_tas() / wind.get_gs());
	double t1(distance_to_time(d1));
	double alt1(time_to_altitude(t1));
	if (maxglide > 0)
		alt1 = std::min(alt1, alt0 - dist * (nmi_to_ft / maxglide));
	if (std::isnan(alt1) || alt1 >= get_ceiling())
		return std::numeric_limits<double>::quiet_NaN();
	alt1 = std::min(alt1, alt0);
	return alt1;
}

double MapRenderer::GlideModel::time_to_altitude(double t) const
{
	return m_climbaltpoly.eval(t);
}

double MapRenderer::GlideModel::time_to_distance(double t) const
{
	return m_climbdistpoly.eval(t);
}

double MapRenderer::GlideModel::time_to_tas(double t) const
{
	return m_climbdistpoly.evalderiv(t) * 3600.0;
}

double MapRenderer::GlideModel::altitude_to_time(double a) const
{
	return m_climbaltpoly.boundedinveval(a, 0, get_climbtime());
}

double MapRenderer::GlideModel::distance_to_time(double d) const
{
	return m_climbdistpoly.boundedinveval(d, 0, get_climbtime());
}

const MapRenderer::GlideModel MapRenderer::invalid_glidemodel;

MapRenderer::MapRenderer(Engine& eng, const ScreenCoord& scrsize, const Point& center, int alt, uint16_t upangle, int64_t time)
        : m_center(center), m_time(time), m_altitude(alt), m_upangle(upangle), m_engine(eng), m_route(0), m_track(0), m_screensize(scrsize)
{
}

MapRenderer::ScreenCoordFloat MapRenderer::rotate(const ScreenCoordFloat& sc, uint16_t angle)
{
	if (!angle)
		return sc;
	float s, c;
	sincosf(to_rad_16bit * angle, &s, &c);
	return ScreenCoordFloat(sc.getx() * c - sc.gety() * s, sc.gety() * c + sc.getx() * s);
}

void MapRenderer::draw(const Cairo::RefPtr<Cairo::Context>& cr, ScreenCoord offs)
{
	cr->save();
	cr->set_source_rgb(1.0, 1.0, 1.0);
	cr->paint();
	cr->restore();
}

void MapRenderer::pan_up(void)
{
        ScreenCoord sc(get_screensize());
        sc.setx(sc.getx() / 2);
        sc.sety(sc.gety() * 3 / 4);
        set_center(screen_coord(sc), m_altitude, m_upangle, m_time);
}

void MapRenderer::pan_down(void)
{
        ScreenCoord sc(get_screensize());
        sc.setx(sc.getx() / 2);
        sc.sety(sc.gety() / 4);
        set_center(screen_coord(sc), m_altitude, m_upangle, m_time);
}

void MapRenderer::pan_left(void)
{
        ScreenCoord sc(get_screensize());
        sc.setx(sc.getx() * 3 / 4);
        sc.sety(sc.gety() / 2);
        set_center(screen_coord(sc), m_altitude, m_upangle, m_time);
}

void MapRenderer::pan_right(void)
{
        ScreenCoord sc(get_screensize());
        sc.setx(sc.getx() / 4);
        sc.sety(sc.gety() / 2);
        set_center(screen_coord(sc), m_altitude, m_upangle, m_time);
}

VectorMapRenderer::TimeMeasurement::TimeMeasurement(bool start)
        : m_running(start)
{
        m_start.assign_current_time();
        m_stop = m_start;
}

void VectorMapRenderer::TimeMeasurement::start(void)
{
        if (m_running)
                return;
        m_start.assign_current_time();
        m_stop = m_start;
        m_running = true;
}

void VectorMapRenderer::TimeMeasurement::stop(void)
{
        if (!m_running)
                return;
        m_stop.assign_current_time();
        m_running = false;
}

VectorMapRenderer::TimeMeasurement::operator unsigned int()
{
        stop();
        Glib::TimeVal v(m_stop);
        v -= m_start;
        return v.tv_sec * 1000 + v.tv_usec / 1000;
}

std::ostream & VectorMapRenderer::TimeMeasurement::print(std::ostream& os)
{
        os << (unsigned int)(*this) << "ms";
        return os;
}

VectorMapRenderer::GRIB2Layer::GRIB2Layer()
        : m_refcount(1), m_valmin(0), m_valmax(0)
{
}

VectorMapRenderer::GRIB2Layer::~GRIB2Layer()
{
}

void VectorMapRenderer::GRIB2Layer::reference(void) const
{
        g_atomic_int_inc(&m_refcount);
}

void VectorMapRenderer::GRIB2Layer::unreference(void) const
{
        if (!g_atomic_int_dec_and_test(&m_refcount))
                return;
        delete this;
}

void VectorMapRenderer::GRIB2Layer::jet(uint8_t d[3], float x)
{
	if (std::isnan(x)) {
		d[0] = d[1] = d[2] = 0;
		return;
	}
	x = std::min(std::max(x, 0.f), 1.f);
	if (x >= 7.f / 8.f)
		d[0] = std::max(0, std::min(255, Point::round<int,float>(-1024.f * x + 9.f * 128.f)));
	else if (x >= 5.f / 8.f)
		d[0] = 255;
	else if (x >= 3.f / 8.f)
		d[0] = std::max(0, std::min(255, Point::round<int,float>(1024.f * x - 3.f * 128.f)));
	else
		d[0] = 0;
	if (x >= 7.f / 8.f)
		d[1] = 0;
	else if (x >= 5.f / 8.f)
		d[1] = std::max(0, std::min(255, Point::round<int,float>(-1024.f * x + 7.f * 128.f)));
	else if (x >= 3.f / 8.f)
		d[1] = 255;
	else if (x >= 1.f / 8.f)
		d[1] = std::max(0, std::min(255, Point::round<int,float>(1024.f * x - 1.f * 128.f)));
	else
		d[1] = 0;
	if (x >= 5.f / 8.f)
		d[2] = 0;
	else if (x >= 3.f / 8.f)
		d[2] = std::max(0, std::min(255, Point::round<int,float>(-1024.f * x + 5.f * 128.f)));
	else if (x >= 1.f / 8.f)
		d[2] = 255;
	else
		d[2] = std::max(0, std::min(255, Point::round<int,float>(1024.f * x + 1.f * 128.f)));
}

bool VectorMapRenderer::GRIB2Layer::compute(Glib::RefPtr<GRIB2::Layer> layer, const Rect& bbox)
{
	m_layer.reset();
	m_surface.clear();
	m_bbox = Rect();
	m_valmin = m_valmax = std::numeric_limits<float>::quiet_NaN();
	if (!layer) {
		if (true)
			std::cerr << "VectorMapRenderer: no GRIB2 layer" << std::endl;
		return false;
	}
	Glib::RefPtr<GRIB2::LayerResult> lr(layer->get_results(bbox));
	if (!lr) {
		if (true)
			std::cerr << "VectorMapRenderer: cannot get GRIB2 layer data" << std::endl;
		return false;
	}
	float vmin(std::numeric_limits<float>::max());
	float vmax(std::numeric_limits<float>::min());
	unsigned int sz(lr->get_size());
	for (unsigned int i = 0; i < sz; ++i) {
		float v(lr->operator[](i));
		if (std::isnan(v))
			continue;
		vmin = std::min(vmin, v);
		vmax = std::max(vmax, v);
	}
	unsigned int w(lr->get_width());
	unsigned int h(lr->get_height());
	if (vmax <= vmin) {
		if (true)
			std::cerr << "VectorMapRenderer: GRIB2 layer (" << w << 'x' << h << ") has no data" << std::endl;
		return false;
	}
	Cairo::RefPtr<Cairo::ImageSurface> surface = Cairo::ImageSurface::create(/*Cairo::FORMAT_ARGB32*/Cairo::FORMAT_RGB24, w, h);
	if (!surface) {
		if (true)
			std::cerr << "VectorMapRenderer: cannot create GRIB2 surface (" << w << 'x' << h << ')' << std::endl;
		return false;
	}
	unsigned int stride(surface->get_stride());
	unsigned char *data(surface->get_data());
	float vscale(1.f / (vmax - vmin));
	float voffs(vmin);
	const GRIB2::Parameter *param(layer->get_parameter());
	std::string unit(param->get_unit_nonnull());
	if (unit == "%") {
		vscale = 0.01;
		voffs = 0;
	}
	for (unsigned int y = 0; y < h; y++) {
		for (unsigned int x = 0; x < w; x++) {
			unsigned char *d = data + 4 * x + stride * y;
			float v(lr->operator()(x, y));
			jet(d, vscale * (v - voffs));
		}
	}
	m_bbox = lr->get_bbox();
	m_layer = layer;
	m_surface = surface;
	m_valmin = voffs;
	m_valmax = voffs + 1.f / vscale;
	return true;
}

void VectorMapRenderer::GRIB2Layer::draw_scale(Cairo::RefPtr<Cairo::Context> cr)
{
	if (!m_layer || std::isnan(m_valmin) || std::isnan(m_valmax) || m_valmax <= m_valmin) {
		if (true)
			std::cerr << "GRIB2Layer::draw_scale: invalid surface" << std::endl;
		return;
	}
	// draw scale on the left lower edge
	const GRIB2::Parameter *param(m_layer->get_parameter());
	cr->save();
	cr->set_line_width(1.0);
	for (int y = 0; y < 100; ++y) {
		uint8_t d[3];
		jet(d, y * (1.f / 100));
		cr->set_source_rgb(d[0] * (1.f / 255.f), d[1] * (1.f / 255.f), d[2] * (1.f / 255.f));
		cr->rectangle(-10, 100 - y, 10, 1);
		cr->fill();
	}
        cr->set_source_rgb(0, 0, 0);
	cr->rectangle(-10, 0, 10, 100);
        cr->stroke();
	float vscale((1.f / 100) * (m_valmax - m_valmin));
	float voffs(m_valmin);
	std::string unit(param->get_unit_nonnull());
      	if (unit == "K") {
		voffs -= IcaoAtmosphere<float>::degc_to_kelvin;
		unit = "degC";
	}
	if (true)
		std::cerr << "GRIB2Layer::draw_scale: offs " << voffs << " scale " << vscale << std::endl;
	double textx(0);
	for (int y = 0; y <= 100; y += 20) {
		cr->set_source_rgb(0, 0, 0);
		cr->move_to(-10, 100 - y);
		cr->line_to(-15, 100 - y);
		cr->stroke();
		std::ostringstream oss;
		oss << (voffs + vscale * y);
	        Cairo::TextExtents ext;
		cr->get_text_extents(oss.str(), ext);
		textx = std::max(textx, ext.width);
		double ty(100 - y + 0.5 * ext.height);
		double tx(-17 - ext.width - ext.x_bearing);
		cr->set_source_rgb(1.0, 1.0, 1.0);
		cr->move_to(tx - 1, ty);
		cr->show_text(oss.str());
		cr->move_to(tx + 1, ty);
		cr->show_text(oss.str());
		cr->move_to(tx, ty - 1);
		cr->show_text(oss.str());
		cr->move_to(tx, ty + 1);
		cr->show_text(oss.str());
		cr->move_to(tx, ty);
		cr->set_source_rgb(0, 0, 0);
		cr->show_text(oss.str());
	}
	if (!unit.empty()) {
	        Cairo::TextExtents ext;
		cr->get_text_extents(unit, ext);
		double ty(120 + 0.5 * ext.height);
		double tx(-10 - ext.width - ext.x_bearing);
		cr->set_source_rgb(1.0, 1.0, 1.0);
		cr->move_to(tx - 1, ty);
		cr->show_text(unit);
		cr->move_to(tx + 1, ty);
		cr->show_text(unit);
		cr->move_to(tx, ty - 1);
		cr->show_text(unit);
		cr->move_to(tx, ty + 1);
		cr->show_text(unit);
		cr->move_to(tx, ty);
		cr->set_source_rgb(0, 0, 0);
		cr->show_text(unit);
	}

	double texty, baselineskip;
	{
	        Cairo::TextExtents ext;
		cr->get_text_extents("0", ext);
		texty = ext.height;
		baselineskip = 1.5 * ext.height;
	}
	textx = -30 - textx;
	for (unsigned int i = 0; i < 4; ++i) {
		std::ostringstream oss;
		switch (i) {
		case 0:
			oss << param->get_abbrev_nonnull();
			{
				const char *cp(param->get_unit());
				if (cp)
					oss << " (" << cp << ")";
				oss << ' ';
			}
			oss << param->get_str_nonnull();
			break;

		case 1:
		{
			Glib::DateTime dt(Glib::DateTime::create_now_utc(m_layer->get_efftime()));
			oss << "Effective " << dt.format("%H:%M:%S %F");
			dt = Glib::DateTime::create_now_utc(m_layer->get_reftime());
			oss << " Issue " << dt.format("%H:%M:%S %F");
			break;
		}

		case 2:
			if (m_layer->get_surface1type() == GRIB2::surface_missing)
				continue;
			oss << GRIB2::find_surfacetype_str(m_layer->get_surface1type(), "")
			    << ' ' << m_layer->get_surface1value()
			    << GRIB2::find_surfaceunit_str(m_layer->get_surface1type(), "");
			if (m_layer->get_surface1type() == GRIB2::surface_isobaric_surface) {
				float alt(0);
				IcaoAtmosphere<float>::std_pressure_to_altitude(&alt, 0, m_layer->get_surface1value() * 0.01);
				oss << " (F" << std::setw(3) << std::setfill('0') << Point::round<int,float>(alt * (Point::m_to_ft * 0.01f)) << ")";
			}
			break;

		case 3:
			if (m_layer->get_surface2type() == GRIB2::surface_missing)
				continue;
			oss << GRIB2::find_surfacetype_str(m_layer->get_surface2type(), "")
			    << ' ' << m_layer->get_surface2value()
			    << GRIB2::find_surfaceunit_str(m_layer->get_surface2type(), "");
			if (m_layer->get_surface2type() == GRIB2::surface_isobaric_surface) {
				float alt(0);
				IcaoAtmosphere<float>::std_pressure_to_altitude(&alt, 0, m_layer->get_surface2value() * 0.01);
				oss << " (F" << std::setw(3) << std::setfill('0') << Point::round<int,float>(alt * (Point::m_to_ft * 0.01f)) << ")";
			}
			break;

		default:
			break;
		}
		if (oss.str().empty())
			continue;
		if (true)
			std::cerr << "GRIB2Layer::draw_scale: text " << i << " x " << textx << " y " << texty << ": " << oss.str() << std::endl;
		Cairo::TextExtents ext;
		cr->get_text_extents(oss.str(), ext);
		double tx(textx - ext.width - ext.x_bearing);
		cr->set_source_rgb(1.0, 1.0, 1.0);
		cr->move_to(tx - 1, texty);
		cr->show_text(oss.str());
		cr->move_to(tx + 1, texty);
		cr->show_text(oss.str());
		cr->move_to(tx, texty - 1);
		cr->show_text(oss.str());
		cr->move_to(tx, texty + 1);
		cr->show_text(oss.str());
		cr->move_to(tx, texty);
		cr->set_source_rgb(0, 0, 0);
		cr->show_text(oss.str());
		texty += baselineskip;
	}
	cr->restore();
}

VectorMapRenderer::TileDownloader::TileDownloader(renderer_t renderer)
	: DrawThreadTMS::TileCache(renderer - renderer_openstreetmap), m_renderer(renderer)
{
}

void VectorMapRenderer::TileDownloader::queue_rect(unsigned int zoom, const Rect& bbox)
{
	if (m_renderer < renderer_openstreetmap  || m_renderer > renderer_vfr_germany)
		return;
	DrawThreadTMS::TileCache::queue_rect(zoom, bbox);
}

unsigned int VectorMapRenderer::TileDownloader::rect_tiles(unsigned int zoom, const Rect& bbox, bool download_only)
{
	if (m_renderer < renderer_openstreetmap  || m_renderer > renderer_vfr_germany)
		return 0;
	return DrawThreadTMS::TileCache::rect_tiles(zoom, bbox, download_only);
}

VectorMapRenderer::VectorMapRenderer(Engine& eng, Glib::Dispatcher& dispatch, int depth, const ScreenCoord& scrsize, const Point& center, renderer_t renderer, int alt, uint16_t upangle, int64_t time)
        : MapRenderer(eng, scrsize, center, upangle, time), m_thread(0), m_drawflags(drawflags_none)
{
        switch (renderer) {
	default:
	case renderer_vmap:
		m_thread = new DrawThreadVMap(eng, dispatch, scrsize, center, alt, upangle, time);
		break;

	case renderer_terrain:
		m_thread = new DrawThreadTerrain(eng, dispatch, scrsize, center, alt, upangle, time);
		break;

	case renderer_airportdiagram:
		m_thread = new DrawThreadAirportDiagram(eng, dispatch, scrsize, center, alt, upangle, time);
		break;

	case renderer_bitmap:
		m_thread = new DrawThreadBitmap(eng, dispatch, Glib::RefPtr<BitmapMaps::Map>(), scrsize, center, alt, upangle, time);
		break;

	case renderer_openstreetmap:
	case renderer_opencyclemap:
	case renderer_osm_public_transport:
	case renderer_osmc_trails:
	case renderer_maps_for_free:
	case renderer_google_street:
	case renderer_google_satellite:
	case renderer_google_hybrid:
	case renderer_virtual_earth_street:
	case renderer_virtual_earth_satellite:
	case renderer_virtual_earth_hybrid:
	case renderer_skyvector_worldvfr:
	case renderer_skyvector_worldifrlow:
	case renderer_skyvector_worldifrhigh:
	case renderer_cartabossy:
	case renderer_cartabossy_weekend:
	case renderer_ign:
	case renderer_openaip:
	case renderer_icao_switzerland:
	case renderer_glider_switzerland:
	case renderer_obstacle_switzerland:
	case renderer_icao_germany:
	case renderer_vfr_germany:
		m_thread = new DrawThreadTMS(eng, dispatch, renderer - renderer_openstreetmap, scrsize, center, alt, upangle, time);
		break;
	}
        set_map_scale((renderer == renderer_airportdiagram) ? 0.002 : 0.1);
}

VectorMapRenderer::~VectorMapRenderer()
{
        delete m_thread;
}


void VectorMapRenderer::stop(void)
{
}

void VectorMapRenderer::set_center(const Point& pt, int alt, uint16_t upangle, int64_t time)
{
	if (false)
		std::cerr << "VectorMapRenderer::set_center: " << pt << " alt " << alt << " upangle " << upangle << " time " << time << std::endl;
        MapRenderer::set_center(pt, alt, upangle, time);
        if (m_thread)
                m_thread->set_center(pt, alt, upangle, time);
}

void VectorMapRenderer::set_screensize(const ScreenCoord& scrsize)
{
        MapRenderer::set_screensize(scrsize);
        if (m_thread)
                m_thread->set_screensize(scrsize);
}

MapRenderer& VectorMapRenderer::operator=(DrawFlags f)
{
        if (m_thread)
                m_thread->set_drawflags(m_drawflags = f);
	if (false)
		std::cerr << "drawflags: " << m_drawflags << std::endl;
        return *this;
}

MapRenderer& VectorMapRenderer::operator&=(DrawFlags f)
{
        if (m_thread)
                m_thread->set_drawflags(m_drawflags &= f);
        return *this;
}

MapRenderer& VectorMapRenderer::operator|=(DrawFlags f)
{
        if (m_thread)
                m_thread->set_drawflags(m_drawflags |= f);
        return *this;
}

MapRenderer& VectorMapRenderer::operator^=(DrawFlags f)
{
        if (m_thread)
                m_thread->set_drawflags(m_drawflags ^= f);
        return *this;
}

void VectorMapRenderer::set_grib2(const Glib::RefPtr<GRIB2::Layer>& layer)
{
	if (m_thread)
                m_thread->set_grib2(layer);
}

void VectorMapRenderer::set_bitmap(const Glib::RefPtr<BitmapMaps::Map>& map)
{
	if (m_thread)
                m_thread->set_bitmap(map);
}

void VectorMapRenderer::airspaces_changed(void)
{
        if (m_thread)
                m_thread->airspaces_changed();
}

void VectorMapRenderer::airports_changed(void)
{
        if (m_thread)
                m_thread->airports_changed();
}

void VectorMapRenderer::navaids_changed(void)
{
        if (m_thread)
                m_thread->navaids_changed();
}

void VectorMapRenderer::waypoints_changed(void)
{
        if (m_thread)
                m_thread->waypoints_changed();
}

void VectorMapRenderer::airways_changed(void)
{
        if (m_thread)
                m_thread->airways_changed();
}

void VectorMapRenderer::draw(const Cairo::RefPtr<Cairo::Context>& cr, ScreenCoord offs)
{
        if (m_thread) {
		m_thread->set_offset(offs);
                m_thread->draw(cr);
		cr->save();
                 // draw route
                if ((m_drawflags & drawflags_route) && m_route)
                        m_thread->draw_route(cr.operator->(), m_route);
                // draw track
                if ((m_drawflags & drawflags_track) && m_track)
                        m_thread->draw_track(cr.operator->(), m_track);
		// draw scale
		cr->translate(10, m_screensize.gety() - 10);
		cr->set_font_size(12);
		m_thread->draw_scale(cr.operator->(), std::min(100, get_screensize().getx() / 4));
		cr->restore();
		// draw north indicator
		if (m_drawflags & drawflags_northpointer) {
			cr->save();
			cr->translate(25, 25);
			cr->scale(10, 10);
			m_thread->draw_north(cr.operator->());
			cr->restore();
		}
        }
}

void VectorMapRenderer::redraw_route(void)
{
}

void VectorMapRenderer::redraw_track(void)
{
}

template<typename T> std::ostream& operator<<(std::ostream& os, const MapRenderer::ScreenCoordTemplate<T>& sc)
{
        return os << '(' << sc.getx() << ',' << sc.gety() << ')';
}

template std::ostream& operator<< <int>(std::ostream & os, const MapRenderer::ScreenCoordTemplate<int>& sc);
template std::ostream& operator<< <float>(std::ostream & os, const MapRenderer::ScreenCoordTemplate<float>& sc);

const std::string& to_str(VectorMapArea::renderer_t rnd)
{
	switch (rnd) {
	case VectorMapArea::renderer_vmap:
	{
		static const std::string r("Vector Map (DCW)");
		return r;
	}

	case VectorMapArea::renderer_terrain:
	{
		static const std::string r("Terrain (SRTM)");
		return r;
	}

	case VectorMapArea::renderer_airportdiagram:
	{
		static const std::string r("Airport Diagrams");
		return r;
	}

	case VectorMapArea::renderer_bitmap:
	{
		static const std::string r("Bitmap");
		return r;
	}

	case VectorMapArea::renderer_openstreetmap:
	{
		static const std::string r("Open Street Map");
		return r;
	}

	case VectorMapArea::renderer_maps_for_free:
	{
		static const std::string r("Maps For Free");
		return r;
	}

	case VectorMapArea::renderer_opencyclemap:
	{
		static const std::string r("Open Cycle Map");
		return r;
	}

	case VectorMapArea::renderer_osm_public_transport:
	{
		static const std::string r("Open Street Map Public Transport");
		return r;
	}

	case VectorMapArea::renderer_osmc_trails:
	{
		static const std::string r("OSMC Trails");
		return r;
	}

	case VectorMapArea::renderer_google_street:
	{
		static const std::string r("Google Street");
		return r;
	}

	case VectorMapArea::renderer_google_satellite:
	{
		static const std::string r("Google Satellite");
		return r;
	}

	case VectorMapArea::renderer_google_hybrid:
	{
		static const std::string r("Google Hybrid");
		return r;
	}

	case VectorMapArea::renderer_virtual_earth_street:
	{
		static const std::string r("Virtual Earth Street");
		return r;
	}

	case VectorMapArea::renderer_virtual_earth_satellite:
	{
		static const std::string r("Virtual Earth Satellite");
		return r;
	}

	case VectorMapArea::renderer_virtual_earth_hybrid:
	{
		static const std::string r("Virtual Earth Hybrid");
		return r;
	}

	case VectorMapArea::renderer_skyvector_worldvfr:
	{
		static const std::string r("SkyVector World VFR");
		return r;
	}

	case VectorMapArea::renderer_skyvector_worldifrlow:
	{
		static const std::string r("SkyVector World IFR Low");
		return r;
	}

	case VectorMapArea::renderer_skyvector_worldifrhigh:
	{
		static const std::string r("SkyVector World IFR High");
		return r;
	}

	case VectorMapArea::renderer_cartabossy:
	{
		static const std::string r("Cartabossy");
		return r;
	}

	case VectorMapArea::renderer_cartabossy_weekend:
	{
		static const std::string r("Cartabossy Weekend");
		return r;
	}

	case VectorMapArea::renderer_ign:
	{
		static const std::string r("IGN");
		return r;
	}

	case VectorMapArea::renderer_openaip:
	{
		static const std::string r("OpenAIP");
		return r;
	}

	case VectorMapArea::renderer_icao_switzerland:
	{
		static const std::string r("ICAO Switzerland");
		return r;
	}

	case VectorMapArea::renderer_glider_switzerland:
	{
		static const std::string r("Glider Switzerland");
		return r;
	}

	case VectorMapArea::renderer_obstacle_switzerland:
	{
		static const std::string r("Obstacle Switzerland");
		return r;
	}

	case VectorMapArea::renderer_icao_germany:
	{
		static const std::string r("ICAO Germany");
		return r;
	}

	case VectorMapArea::renderer_vfr_germany:
	{
		static const std::string r("VFR Germany");
		return r;
	}

	default:
	case VectorMapArea::renderer_none:
	{
		static const std::string r("None");
		return r;
	}
	}
}

const std::string& to_short_str(VectorMapArea::renderer_t rnd)
{
	switch (rnd) {
	case VectorMapArea::renderer_vmap:
	{
		static const std::string r("VM");
		return r;
	}

	case VectorMapArea::renderer_terrain:
	{
		static const std::string r("T");
		return r;
	}

	case VectorMapArea::renderer_airportdiagram:
	{
		static const std::string r("AD");
		return r;
	}

	case VectorMapArea::renderer_bitmap:
	{
		static const std::string r("BMP");
		return r;
	}

	case VectorMapArea::renderer_openstreetmap:
	{
		static const std::string r("OSM");
		return r;
	}

	case VectorMapArea::renderer_maps_for_free:
	{
		static const std::string r("MFF");
		return r;
	}

	case VectorMapArea::renderer_opencyclemap:
	{
		static const std::string r("OCM");
		return r;
	}

	case VectorMapArea::renderer_osm_public_transport:
	{
		static const std::string r("OPT");
		return r;
	}

	case VectorMapArea::renderer_osmc_trails:
	{
		static const std::string r("TR");
		return r;
	}

	case VectorMapArea::renderer_google_street:
	{
		static const std::string r("G");
		return r;
	}

	case VectorMapArea::renderer_google_satellite:
	{
		static const std::string r("GS");
		return r;
	}

	case VectorMapArea::renderer_google_hybrid:
	{
		static const std::string r("GH");
		return r;
	}

	case VectorMapArea::renderer_virtual_earth_street:
	{
		static const std::string r("VE");
		return r;
	}

	case VectorMapArea::renderer_virtual_earth_satellite:
	{
		static const std::string r("VES");
		return r;
	}

	case VectorMapArea::renderer_virtual_earth_hybrid:
	{
		static const std::string r("VEH");
		return r;
	}

	case VectorMapArea::renderer_skyvector_worldvfr:
	{
		static const std::string r("SVV");
		return r;
	}

	case VectorMapArea::renderer_skyvector_worldifrlow:
	{
		static const std::string r("SVIL");
		return r;
	}

	case VectorMapArea::renderer_skyvector_worldifrhigh:
	{
		static const std::string r("SVIH");
		return r;
	}

	case VectorMapArea::renderer_cartabossy:
	{
		static const std::string r("CB");
		return r;
	}

	case VectorMapArea::renderer_cartabossy_weekend:
	{
		static const std::string r("CBW");
		return r;
	}

	case VectorMapArea::renderer_ign:
	{
		static const std::string r("IGN");
		return r;
	}

	case VectorMapArea::renderer_openaip:
	{
		static const std::string r("OAIP");
		return r;
	}

	case VectorMapArea::renderer_icao_switzerland:
	{
		static const std::string r("LSI");
		return r;
	}

	case VectorMapArea::renderer_glider_switzerland:
	{
		static const std::string r("LSG");
		return r;
	}

	case VectorMapArea::renderer_obstacle_switzerland:
	{
		static const std::string r("LSO");
		return r;
	}

	case VectorMapArea::renderer_icao_germany:
	{
		static const std::string r("EDI");
		return r;
	}

	case VectorMapArea::renderer_vfr_germany:
	{
		static const std::string r("EDV");
		return r;
	}

	default:
	case VectorMapArea::renderer_none:
	{
		static const std::string r("?");
		return r;
	}
	}
}

VectorMapArea::VectorMapArea(BaseObjectType *castitem, const Glib::RefPtr<Builder>& refxml)
        : Gtk::DrawingArea(castitem), m_engine(0), m_renderer(0), m_rendertype(renderer_none),
          m_dragbutton(2), m_dragstart(0, 0), m_dragoffset(0, 0), m_draginprogress(false),
          m_cursorvalid(false), m_cursoranglevalid(false), m_cursorangle(0),
          m_mapscale(0.1), m_winddir(0), m_windspeed(0), m_time(0), m_altitude(0), m_upangle(0),
	  m_route(0), m_track(0), m_drawflags(MapRenderer::drawflags_none)
{
        set_map_scale(0.1);
#ifndef GLIBMM_DEFAULT_SIGNAL_HANDLERS_ENABLED
#ifdef HAVE_GTKMM2
        signal_expose_event().connect(sigc::mem_fun(*this, &VectorMapArea::on_expose_event));
#endif
#ifdef HAVE_GTKMM3
        signal_draw().connect(sigc::mem_fun(*this, &VectorMapArea::on_draw));
#endif
        signal_size_allocate().connect(sigc::mem_fun(*this, &VectorMapArea::on_size_allocate));
        signal_show().connect(sigc::mem_fun(*this, &VectorMapArea::on_show));
        signal_hide().connect(sigc::mem_fun(*this, &VectorMapArea::on_hide));
        signal_button_press_event().connect(sigc::mem_fun(*this, &VectorMapArea::on_button_press_event));
        signal_button_release_event().connect(sigc::mem_fun(*this, &VectorMapArea::on_button_release_event));
        signal_motion_notify_event().connect(sigc::mem_fun(*this, &VectorMapArea::on_motion_notify_event));
        signal_scroll_event().connect(sigc::mem_fun(*this, &VectorMapArea::on_scroll_event));
#endif //GLIBMM_DEFAULT_SIGNAL_HANDLERS_ENABLED
}

VectorMapArea::~VectorMapArea()
{
	m_connupdate.disconnect();
        delete m_renderer;
}

void VectorMapArea::set_engine(Engine& eng)
{
        m_engine = &eng;
        renderer_alloc();
}

void VectorMapArea::set_renderer(renderer_t render)
{
        m_rendertype = render;
        renderer_alloc();
}

VectorMapArea::renderer_t VectorMapArea::get_renderer(void) const
{
        return m_rendertype;
}

bool VectorMapArea::is_renderer_enabled(renderer_t rnd)
{
	switch (rnd) {
	case renderer_vmap:
	case renderer_terrain:
	case renderer_airportdiagram:
	case renderer_bitmap:
	case renderer_openstreetmap:
	case renderer_maps_for_free:
	case renderer_opencyclemap:
	case renderer_osm_public_transport:
	case renderer_google_street:
	case renderer_google_satellite:
	case renderer_google_hybrid:
	case renderer_virtual_earth_street:
	case renderer_virtual_earth_satellite:
	case renderer_virtual_earth_hybrid:
	case renderer_osmc_trails:
	case renderer_skyvector_worldvfr:
	case renderer_skyvector_worldifrlow:
	case renderer_skyvector_worldifrhigh:
	case renderer_cartabossy:
	case renderer_cartabossy_weekend:
	case renderer_ign:
	//case renderer_openaip:
	case renderer_icao_switzerland:
	case renderer_glider_switzerland:
	//case renderer_obstacle_switzerland:
	case renderer_icao_germany:
	case renderer_vfr_germany:
		return true;

	default:
	case renderer_none:
		return false;
	}
}

VectorMapRenderer::renderer_t VectorMapArea::convert_renderer(renderer_t render)
{
	static const struct renderer_maptable {
		renderer_t rnd;
		VectorMapRenderer::renderer_t rnd2;
	} renderer_maptable[] = {
		{ renderer_vmap, VectorMapRenderer::renderer_vmap },
		{ renderer_terrain, VectorMapRenderer::renderer_terrain },
		{ renderer_airportdiagram, VectorMapRenderer::renderer_airportdiagram },
		{ renderer_bitmap, VectorMapRenderer::renderer_bitmap },
		{ renderer_openstreetmap, VectorMapRenderer::renderer_openstreetmap },
		{ renderer_maps_for_free, VectorMapRenderer::renderer_maps_for_free },
		{ renderer_opencyclemap, VectorMapRenderer::renderer_opencyclemap },
		{ renderer_osm_public_transport, VectorMapRenderer::renderer_osm_public_transport },
		{ renderer_osmc_trails, VectorMapRenderer::renderer_osmc_trails },
		{ renderer_google_street, VectorMapRenderer::renderer_google_street },
		{ renderer_google_satellite, VectorMapRenderer::renderer_google_satellite },
		{ renderer_google_hybrid, VectorMapRenderer::renderer_google_hybrid },
		{ renderer_virtual_earth_street, VectorMapRenderer::renderer_virtual_earth_street },
		{ renderer_virtual_earth_satellite, VectorMapRenderer::renderer_virtual_earth_satellite },
		{ renderer_virtual_earth_hybrid, VectorMapRenderer::renderer_virtual_earth_hybrid },
		{ renderer_skyvector_worldvfr, VectorMapRenderer::renderer_skyvector_worldvfr },
		{ renderer_skyvector_worldifrlow, VectorMapRenderer::renderer_skyvector_worldifrlow },
		{ renderer_skyvector_worldifrhigh, VectorMapRenderer::renderer_skyvector_worldifrhigh },
		{ renderer_cartabossy, VectorMapRenderer::renderer_cartabossy },
		{ renderer_cartabossy_weekend, VectorMapRenderer::renderer_cartabossy_weekend },
		{ renderer_ign, VectorMapRenderer::renderer_ign },
		{ renderer_openaip, VectorMapRenderer::renderer_openaip },
		{ renderer_icao_switzerland, VectorMapRenderer::renderer_icao_switzerland },
		{ renderer_glider_switzerland, VectorMapRenderer::renderer_glider_switzerland },
		{ renderer_obstacle_switzerland, VectorMapRenderer::renderer_obstacle_switzerland },
		{ renderer_icao_germany, VectorMapRenderer::renderer_icao_germany },
		{ renderer_vfr_germany, VectorMapRenderer::renderer_vfr_germany },
		{ renderer_none, VectorMapRenderer::renderer_vmap }
	};
	for (const struct renderer_maptable *m = renderer_maptable; m->rnd != renderer_none; ++m)
		if (m->rnd == render)
			return m->rnd2;
	return VectorMapRenderer::renderer_vmap;
}

void VectorMapArea::renderer_alloc(void)
{
	m_connupdate.disconnect();
        delete m_renderer;
        m_renderer = 0;
        if (!m_engine)
                return;
        Glib::RefPtr<Gdk::Window> window = Gtk::Widget::get_root_window();
        if (!window)
                return;
	int depth(32);
#ifdef HAVE_GTKMM2
	depth = window->get_depth();
#endif
#ifdef HAVE_GTKMM3
	depth = window->get_visual()->get_depth();
#endif
	if (m_rendertype != renderer_none)
		m_renderer = new VectorMapRenderer(*m_engine, m_dispatch, depth, VectorMapRenderer::ScreenCoord(0, 0), Point(), convert_renderer(m_rendertype), m_altitude, m_upangle, m_time);
        if (m_renderer) {
                m_connupdate = m_dispatch.connect(sigc::mem_fun(*this, &VectorMapArea::renderer_update));
                m_renderer->set_maxalt(m_engine->get_prefs().maxalt * 100);
                m_renderer->set_route(m_route);
                m_renderer->set_track(m_track);
                m_renderer->set_center(m_center, m_altitude, m_upangle, m_time);
                m_renderer->set_map_scale(m_mapscale);
		m_renderer->set_glidemodel((m_rendertype == renderer_terrain) ? m_glidemodel : MapRenderer::GlideModel());
		m_renderer->set_wind(m_winddir, m_windspeed);
                *m_renderer = m_drawflags;
                m_renderer->set_screensize(MapRenderer::ScreenCoord(get_width(), get_height()));
		m_renderer->set_grib2(m_grib2layer);
                m_renderer->set_bitmap(m_map);
        }
}

void VectorMapArea::set_route(const FPlanRoute& route)
{
        m_route = &route;
        if (!m_renderer)
                return;
        m_renderer->set_route(m_route);
}

void VectorMapArea::set_track(const TracksDb::Track *track)
{
        m_track = track;
        if (!m_renderer)
                return;
        m_renderer->set_track(m_track);
}

void VectorMapArea::renderer_update(void)
{
	if (get_is_drawable())
		queue_draw();
}

void VectorMapArea::redraw_route(void)
{
        if (!m_renderer)
                return;
        m_renderer->redraw_route();
	if (get_is_drawable())
		queue_draw();
}

void VectorMapArea::redraw_track(void)
{
        if (!m_renderer)
                return;
        m_renderer->redraw_track();
	if (get_is_drawable())
		queue_draw();
}

#ifdef HAVE_GTKMM2
bool VectorMapArea::on_expose_event(GdkEventExpose *event)
{
        Glib::RefPtr<Gdk::Window> wnd(get_window());
        if (!wnd)
                return false;
        Cairo::RefPtr<Cairo::Context> cr(wnd->create_cairo_context());
	if (event) {
		GdkRectangle *rects;
		int n_rects;
		gdk_region_get_rectangles(event->region, &rects, &n_rects);
		for (int i = 0; i < n_rects; i++)
			cr->rectangle(rects[i].x, rects[i].y, rects[i].width, rects[i].height);
		cr->clip();
		g_free(rects);
	}
        return on_draw(cr);
}
#endif

bool VectorMapArea::on_draw(const Cairo::RefPtr<Cairo::Context>& cr)
{
        if (!m_renderer || !cr)
                return false;
        m_renderer->draw(cr, m_dragoffset);
        if (m_cursorvalid) {
                MapRenderer::ScreenCoord sc(m_renderer->coord_screen(m_cursor));
		cr->save();
		cr->translate(sc.getx(), sc.gety());
                if (m_cursoranglevalid) {
			cr->rotate((m_cursorangle - m_upangle) * MapRenderer::to_rad_16bit_dbl);
			cr->set_line_width(3);
			cr->set_source_rgb(1.0, 1.0, 1.0);
			cr->begin_new_sub_path();
			cr->arc(0.0, 0.0, 15.0, 0.0, 2.0 * M_PI);
			cr->stroke();
			cr->set_line_width(1);
			cr->set_source_rgb(1.0, 0.0, 0.0);
			cr->move_to(0.0, 0.0);
			cr->arc(0.0, 0.0, 15.0, (90.0 - 30.0) * (M_PI / 180.0), (90.0 + 30.0) * (M_PI / 180.0));
			cr->close_path();
			cr->fill();
			cr->arc(0.0, 0.0, 15.0, 0.0, 2.0 * M_PI);
			cr->stroke();
                } else {
			// make lines fall onto exact pixels
			cr->translate(0.5, 0.5);
			cr->move_to(-16.0, 0.0);
			cr->line_to(16.0, 0.0);
			cr->move_to(0.0, -16.0);
			cr->line_to(0.0, 16.0);
			cr->set_line_width(3);
			cr->set_source_rgb(1.0, 1.0, 1.0);
			cr->stroke();
			cr->move_to(-15.0, 0.0);
			cr->line_to(15.0, 0.0);
			cr->move_to(0.0, -15.0);
			cr->line_to(0.0, 15.0);
			cr->set_line_width(1);
			cr->set_source_rgb(1.0, 0.0, 0.0);
			cr->stroke();
                }
		cr->restore();
        }
        return true;
}

void VectorMapArea::on_size_allocate(Gtk::Allocation& allocation)
{
#ifdef GLIBMM_DEFAULT_SIGNAL_HANDLERS_ENABLED
	Gtk::DrawingArea::on_size_allocate(allocation);
#endif //GLIBMM_DEFAULT_SIGNAL_HANDLERS_ENABLED
        if (m_renderer)
                m_renderer->set_screensize(MapRenderer::ScreenCoord(allocation.get_width(), allocation.get_height()));
        //std::cerr << "map size allocate" << std::endl;
}

void VectorMapArea::on_show(void)
{
#ifdef GLIBMM_DEFAULT_SIGNAL_HANDLERS_ENABLED
	Gtk::DrawingArea::on_show();
#endif //GLIBMM_DEFAULT_SIGNAL_HANDLERS_ENABLED
        std::cerr << "vmaparea: show" << std::endl;
        if (m_renderer)
                m_renderer->show();
}

void VectorMapArea::on_hide(void)
{
#ifdef GLIBMM_DEFAULT_SIGNAL_HANDLERS_ENABLED
	Gtk::DrawingArea::on_hide();
#endif //GLIBMM_DEFAULT_SIGNAL_HANDLERS_ENABLED
        std::cerr << "vmaparea: hide" << std::endl;
        if (m_renderer)
                m_renderer->hide();
}

unsigned int VectorMapArea::button_translate(GdkEventButton *event)
{
        unsigned int button = event->button;
        if (button == 1) {
                guint state = event->state;
                if (state & GDK_SHIFT_MASK)
                        button = 2;
                else if (state & GDK_CONTROL_MASK)
                        button = 3;
        }
        return button;
}

bool VectorMapArea::on_button_press_event(GdkEventButton *event)
{
        MapRenderer::ScreenCoordFloat sc(event->x, event->y);
        unsigned int button = button_translate(event);
        if (button == m_dragbutton) {
                m_dragstart = sc;
                m_dragoffset = MapRenderer::ScreenCoord(0, 0);
                m_draginprogress = true;
        } else if (button == 1) {
                if (m_renderer) {
                        m_cursor = m_renderer->screen_coord(sc);
                        m_cursorvalid = false;
                        signal_cursor()(m_cursor, cursor_invalid);
                        redraw();
                }
	}
	if (true)
		std::cerr << "map button_press " << sc << ' ' << button << std::endl;
#ifdef GLIBMM_DEFAULT_SIGNAL_HANDLERS_ENABLED
	if (Gtk::DrawingArea::on_button_press_event(event))
		return true;
#endif //GLIBMM_DEFAULT_SIGNAL_HANDLERS_ENABLED
        return false;
}

bool VectorMapArea::on_button_release_event(GdkEventButton *event)
{
        MapRenderer::ScreenCoordFloat sc(event->x, event->y);
        unsigned int button = button_translate(event);
        if (m_draginprogress && (button == m_dragbutton)) {
                if (m_renderer) {
                        Point pt = m_renderer->screen_coord(m_dragstart) - m_renderer->screen_coord(sc);
                        m_renderer->set_center(m_renderer->get_center() + pt, m_renderer->get_altitude(), m_renderer->get_upangle(), m_renderer->get_time());
                }
                m_dragoffset = MapRenderer::ScreenCoord(0, 0);
                m_draginprogress = false;
                redraw();
        } else if (button == 1) {
                if (m_renderer) {
                        m_cursor = m_renderer->screen_coord(sc);
                        m_cursorvalid = true;
                        signal_cursor()(m_cursor, cursor_mouse);
                        redraw();
                }
        }
	if (true)
		std::cerr << "map button_release " << sc << ' ' << button << std::endl;
#ifdef GLIBMM_DEFAULT_SIGNAL_HANDLERS_ENABLED
	if (Gtk::DrawingArea::on_button_release_event(event))
		return true;
#endif //GLIBMM_DEFAULT_SIGNAL_HANDLERS_ENABLED
        return false;
}

bool VectorMapArea::on_motion_notify_event(GdkEventMotion *event)
{
        VectorMapRenderer::ScreenCoordFloat sc(event->x, event->y);
        unsigned int button = 0 /* event->button */;
        Point pt;
        if (m_renderer) {
                pt = m_renderer->screen_coord(sc);
                signal_pointer()(pt);
        }
        if (m_draginprogress) {
                if (m_renderer) {
                        MapRenderer::ScreenCoordFloat sc1 = m_dragstart - sc;
                        m_dragoffset = MapRenderer::ScreenCoord((int)sc1.getx(), (int)sc1.gety());
                        redraw();
                }
        }
	if (false)
		std::cerr << "map motion: screen " << sc << " coord " << pt << std::endl;
#ifdef GLIBMM_DEFAULT_SIGNAL_HANDLERS_ENABLED
	if (Gtk::DrawingArea::on_motion_notify_event(event))
		return true;
#endif //GLIBMM_DEFAULT_SIGNAL_HANDLERS_ENABLED
        return false;
}

bool VectorMapArea::on_scroll_event(GdkEventScroll *event)
{
        VectorMapRenderer::ScreenCoordFloat sc(event->x, event->y);
	guint state = event->state;
	GdkScrollDirection dir = event->direction;
	// MOD1 is Alt
	switch (state & (GDK_SHIFT_MASK | GDK_CONTROL_MASK | GDK_MOD1_MASK)) {
	case 0:
		switch (dir) {
		case GDK_SCROLL_UP:
			pan_up();
			break;

		case GDK_SCROLL_DOWN:
			pan_down();
			break;

		case GDK_SCROLL_LEFT:
			pan_left();
			break;

		case GDK_SCROLL_RIGHT:
			pan_right();
			break;

		default:
			break;
		}
		break;

	case GDK_SHIFT_MASK:
		switch (dir) {
		case GDK_SCROLL_UP:
			set_center(get_center(), get_altitude(), get_upangle() + (1 << 11));
			break;

		case GDK_SCROLL_DOWN:
			set_center(get_center(), get_altitude(), get_upangle() - (1 << 11));
			break;

		default:
			break;
		}
		break;

	case GDK_CONTROL_MASK:
		switch (dir) {
		case GDK_SCROLL_UP:
			zoom_in();
			break;

		case GDK_SCROLL_DOWN:
			zoom_out();
			break;

		default:
			break;
		}
		break;

	case GDK_MOD1_MASK:
		switch (dir) {
		case GDK_SCROLL_UP:
			pan_left();
			break;

		case GDK_SCROLL_DOWN:
			pan_right();
			break;

		default:
			break;
		}
		break;

	default:
		break;
	}
	if (true)
		std::cerr << "map scroll " << sc << ' ' << (int)event->direction << std::endl;
#ifdef GLIBMM_DEFAULT_SIGNAL_HANDLERS_ENABLED
	if (Gtk::DrawingArea::on_scroll_event(event))
		return true;
#endif //GLIBMM_DEFAULT_SIGNAL_HANDLERS_ENABLED
	return false;
}

void VectorMapArea::pan_up(void)
{
        if (m_renderer) {
                m_renderer->pan_up();
                m_center = m_renderer->get_center();
        }
        redraw();
}

void VectorMapArea::pan_down(void)
{
        if (m_renderer) {
                m_renderer->pan_down();
                m_center = m_renderer->get_center();
        }
        redraw();
}

void VectorMapArea::pan_left(void)
{
        if (m_renderer) {
                m_renderer->pan_left();
                m_center = m_renderer->get_center();
        }
        redraw();
}

void VectorMapArea::pan_right(void)
{
        if (m_renderer) {
                m_renderer->pan_right();
                m_center = m_renderer->get_center();
        }
        redraw();
}

void VectorMapArea::set_map_scale(float nmi_per_pixel)
{
        m_mapscale = nmi_per_pixel;
        if (!m_renderer)
                return;
        m_renderer->set_map_scale(m_mapscale);
        m_mapscale = m_renderer->get_map_scale();
        redraw();
}

float VectorMapArea::get_map_scale(void) const
{
        if (!m_renderer)
                return m_mapscale;
        return m_renderer->get_map_scale();
}

void VectorMapArea::set_glidemodel(const MapRenderer::GlideModel& gm)
{
	m_glidemodel = gm;
	if (m_renderer && m_rendertype == renderer_terrain)
		m_renderer->set_glidemodel(m_glidemodel);
        redraw();
}

const MapRenderer::GlideModel& VectorMapArea::get_glidemodel(void) const
{
	if (m_renderer && m_rendertype == renderer_terrain)
		return m_renderer->get_glidemodel();
	return m_glidemodel;
}

void VectorMapArea::set_wind(float dir, float speed)
{
	if (std::isnan(dir))
		dir = 0;
	if (std::isnan(speed))
		speed = 0;
	if (speed < 0) {
		speed = -speed;
		dir += 180;
	}
	if (speed == 0) {
		dir = 0;
	} else {
		float x;
		dir = modff(dir * (1.f / 360.f), &x) * 360.f;
	}
	m_winddir = dir;
	m_windspeed = speed;
	if (m_renderer)
		m_renderer->set_wind(m_winddir, m_windspeed);
        redraw();
}

float VectorMapArea::get_winddir(void) const
{
	if (m_renderer)
		return m_renderer->get_winddir();
	return m_winddir;
}

float VectorMapArea::get_windspeed(void) const
{
	if (m_renderer)
		return m_renderer->get_windspeed();
	return m_windspeed;
}

void VectorMapArea::set_center(const Point& pt, int alt, uint16_t upangle, int64_t time)
{
        m_center = pt;
        m_altitude = alt;
	m_upangle = upangle;
	m_time = time;
        if (!m_renderer)
                return;
        if (m_engine)
                m_renderer->set_maxalt(m_engine->get_prefs().maxalt * 100);
        m_renderer->set_center(m_center, m_altitude, m_upangle, m_time);
        redraw();
        if (!get_visible())
                m_renderer->hide();
}

Point VectorMapArea::get_center(void) const
{
        if (!m_renderer)
                return m_center;
        return m_renderer->get_center();
}

int VectorMapArea::get_altitude(void) const
{
        if (!m_renderer)
                return m_altitude;
        return m_renderer->get_altitude();
}

uint16_t VectorMapArea::get_upangle(void) const
{
        if (!m_renderer)
                return m_upangle;
        return m_renderer->get_upangle();
}

int64_t VectorMapArea::get_time(void) const
{
        if (!m_renderer)
                return m_time;
        return m_renderer->get_time();
}

void VectorMapArea::set_dragbutton(unsigned int dragbutton)
{
        m_dragbutton = dragbutton;
        m_dragoffset = MapRenderer::ScreenCoord(0, 0);
        m_draginprogress = false;
}

VectorMapArea::operator MapRenderer::DrawFlags() const
{
        if (!m_renderer)
                return m_drawflags;
        return *m_renderer;
}

VectorMapArea & VectorMapArea::operator=(MapRenderer::DrawFlags f)
{
        m_drawflags = f;
        if (m_renderer)
                *m_renderer = f;
        return *this;
}

VectorMapArea & VectorMapArea::operator&=(MapRenderer::DrawFlags f)
{
        m_drawflags &= f;
        if (m_renderer)
                *m_renderer &= f;
        return *this;
}

VectorMapArea & VectorMapArea::operator|=(MapRenderer::DrawFlags f)
{
        m_drawflags |= f;
        if (m_renderer)
                *m_renderer |= f;
        return *this;
}

VectorMapArea & VectorMapArea::operator^=(MapRenderer::DrawFlags f)
{
        m_drawflags ^= f;
        if (m_renderer)
                *m_renderer ^= f;
        return *this;
}

void VectorMapArea::set_grib2(const Glib::RefPtr<GRIB2::Layer>& layer)
{
	m_grib2layer = layer;
	if (m_renderer)
		m_renderer->set_grib2(m_grib2layer);
}

void VectorMapArea::set_bitmap(const Glib::RefPtr<BitmapMaps::Map>& map)
{
	m_map = map;
	if (m_renderer)
                m_renderer->set_bitmap(m_map);
}

void VectorMapArea::airspaces_changed(void)
{
        if (m_renderer)
                m_renderer->airspaces_changed();
}

void VectorMapArea::airports_changed(void)
{
        if (m_renderer)
                m_renderer->airports_changed();
}

void VectorMapArea::navaids_changed(void)
{
        if (m_renderer)
                m_renderer->navaids_changed();
}

void VectorMapArea::waypoints_changed(void)
{
        if (m_renderer)
                m_renderer->waypoints_changed();
}

void VectorMapArea::airways_changed(void)
{
        if (m_renderer)
                m_renderer->airways_changed();
}

void VectorMapArea::invalidate_cursor(void)
{
        if (m_cursorvalid) {
                m_cursorvalid = false;
                redraw();
                signal_cursor()(m_cursor, cursor_invalid);
        }
}

void VectorMapArea::set_cursor(const Point& pt)
{
	if (m_cursorvalid && m_cursor == pt)
		return;
	m_cursor = pt;
	m_cursorvalid = true;
	redraw();
	signal_cursor()(m_cursor, cursor_set);
}

Rect VectorMapArea::get_cursor_rect(void) const
{
        if (!m_cursorvalid || !m_renderer)
                return Rect();
        MapRenderer::ScreenCoord sc(m_renderer->coord_screen(m_cursor));
        return Rect(m_renderer->screen_coord(sc + MapRenderer::ScreenCoord(-5, 5)),
                    m_renderer->screen_coord(sc + MapRenderer::ScreenCoord(5, -5)));
}

void VectorMapArea::set_cursor_angle(float angle)
{
	uint16_t curangle(Point::round<uint16_t,float>(angle * MapRenderer::from_deg_16bit));
	if (m_cursoranglevalid && m_cursorangle == curangle)
		return;
        m_cursorangle = curangle;
        m_cursoranglevalid = true;
        redraw();
}

void VectorMapArea::invalidate_cursor_angle(void)
{
        m_cursorangle = 0;
        if (m_cursoranglevalid) {
                m_cursoranglevalid = false;
                redraw();
        }
}

void VectorMapArea::zoom_out(void)
{
        if (m_renderer) {
                m_renderer->zoom_out();
                m_mapscale = m_renderer->get_map_scale();
        }
        redraw();
}

void VectorMapArea::zoom_in(void)
{
        if (m_renderer) {
                m_renderer->zoom_in();
                m_mapscale = m_renderer->get_map_scale();
        }
        redraw();
}

VectorMapRenderer::Corner::Corner(void)
{
}

VectorMapRenderer::Corner::operator Rect() const
{
	Rect r;
	r.set_east(((*this)[1].get_lon() + (*this)[3].get_lon()) >> 1);
	r.set_west(((*this)[0].get_lon() + (*this)[2].get_lon()) >> 1);
	r.set_south(((*this)[2].get_lat() + (*this)[3].get_lat()) >> 1);
	r.set_north(((*this)[0].get_lat() + (*this)[1].get_lat()) >> 1);
	return r;
}

const Rect VectorMapRenderer::DrawThread::empty_rect(Point(0, Point::pole_lat), Point(0, Point::pole_lat));

VectorMapRenderer::DrawThread::DrawThread(Engine& eng, Glib::Dispatcher& dispatch, const ScreenCoord& scrsize, const Point& center, int alt, uint16_t upangle, int64_t time)
        : m_engine(eng), m_dispatch(dispatch), m_dbrectangle(empty_rect),
	  m_center(center), m_screensize(scrsize), m_offset(0, 0), m_drawoffset(0, 0), m_time(time), m_scale(0.1), m_winddir(0), m_windspeed(0),
	  m_altitude(alt), m_maxalt(std::numeric_limits<int>::max()),
	  m_drawflags(drawflags_none), m_upangle(upangle), m_threadstate(thread_idle), m_hidden(false)
{
	m_dbdispatch.connect(sigc::mem_fun(*this, &DrawThread::dbdone));
}

VectorMapRenderer::DrawThread::~DrawThread()
{
}

void VectorMapRenderer::DrawThread::set_center(const Point& pt)
{
	m_center = pt;
	check_restart();
	update_drawoffset();
}

void VectorMapRenderer::DrawThread::set_center(const Point& pt, int alt)
{
	m_center = pt;
	m_altitude = alt;
	check_restart();
	update_drawoffset();
}

void VectorMapRenderer::DrawThread::set_center(const Point& pt, int alt, uint16_t upangle)
{
	m_center = pt;
	m_altitude = alt;
	m_upangle = upangle;
	check_restart();
	update_drawoffset();
}

void VectorMapRenderer::DrawThread::set_center(const Point& pt, int alt, uint16_t upangle, int64_t time)
{
	m_center = pt;
	m_altitude = alt;
	m_upangle = upangle;
	m_time = time;
	check_restart();
	update_drawoffset();
}

void VectorMapRenderer::DrawThread::set_screensize(const ScreenCoord& scrsize)
{
	m_screensize = scrsize;
	check_restart();
	update_drawoffset();
}

void VectorMapRenderer::DrawThread::set_offset(const ScreenCoord& offs)
{
	m_offset = offs;
	check_restart();
	update_drawoffset();
}

void VectorMapRenderer::DrawThread::set_upangle(uint16_t upangle)
{
	m_upangle = upangle;
	check_restart();
}

void VectorMapRenderer::DrawThread::set_time(int64_t time)
{
	m_time = time;
	check_restart();
}

void VectorMapRenderer::DrawThread::set_map_scale(float nmi_per_pixel)
{
	std::pair<float,float> bnds(get_map_scale_bounds());
 	nmi_per_pixel = std::max(bnds.first, std::min(bnds.second, nmi_per_pixel));
 	m_scale = nmi_per_pixel;
	check_restart();
}

void VectorMapRenderer::DrawThread::set_altitude(int alt)
{
	m_altitude = alt;
	check_restart();
}

void VectorMapRenderer::DrawThread::set_glidemodel(const GlideModel& gm)
{
	m_glidemodel = gm;
	check_restart();
}

void VectorMapRenderer::DrawThread::set_wind(float winddir, float windspeed)
{
	if (std::isnan(winddir))
		winddir = 0;
	if (std::isnan(windspeed))
		windspeed = 0;
	if (windspeed < 0) {
		windspeed = -windspeed;
		winddir += 180;
	}
	if (windspeed == 0) {
		winddir = 0;
	} else {
		float x;
		winddir = modff(winddir * (1.f / 360.f), &x) * 360.f;
	}
	m_winddir = winddir;
	m_windspeed = windspeed;
	check_restart();
}

MapRenderer::DrawFlags VectorMapRenderer::DrawThread::set_drawflags(DrawFlags df)
{
	m_drawflags = df;
	check_restart();
	return m_drawflags;
}

void VectorMapRenderer::DrawThread::set_hidden(bool hidden)
{
	bool unhide(m_hidden && !hidden);
	bool hide(hidden && !m_hidden);
	m_hidden = hidden;
	if (unhide)
		restart();
}

void VectorMapRenderer::DrawThread::set_grib2(const Glib::RefPtr<GRIB2::Layer>& layer)
{
	bool samelayer((!m_grib2layer && !layer) ||
		       (m_grib2layer && layer && m_grib2layer == layer));
	m_grib2layer = layer;
	if (!samelayer && !is_hidden())
		restart_db();
}

void VectorMapRenderer::DrawThread::check_restart(void)
{
	static const int alt_tolerance = 10;
	static const int upangle_tolerance = 5 * from_deg_16bit_dbl;
	static const float map_tolerance = 0.001;
	static const float tas_tolerance = 0.01;
	static const float winddir_tolerance = 5;
	static const float windspeed_tolerance = 0.01;
	static const float windspeed_abstolerance = 0.1;
	static const DrawFlags drawflags_mask = (DrawFlags)(~0) & ~(drawflags_route | drawflags_track | drawflags_track_points);

	if (false) {
		ScreenCoord scscr(calc_image_drawoffset(m_screenbuf));
		ScreenCoord scdraw(calc_image_drawoffset(m_drawbuf));
		Rect gsrect;
		{
			Point pd(TopoDb30::TopoCoordinate::get_pointsize());
			pd.set_lon(pd.get_lon() / 2);
			pd.set_lat(pd.get_lat() / 2);
			gsrect = Rect(get_center() - pd, get_center() + pd);
		}
		std::cerr << "ThreadImageBuffer::check_restart: hidden " << is_hidden()
			  << " threadstate " << m_threadstate
			  << " screensize " << get_screensize() << " center " << get_center()
			  << " offset " << get_offset() << " scale " << get_map_scale()
			  << " north " << get_upangle() << " alt " << get_altitude() << " maxalt " << get_maxalt()
			  << " time " << get_time() << " wind " << get_winddir() << '/' << get_windspeed()
			  << " flags " << get_drawflags() << " gsrect " << gsrect << std::endl
			  << "  screenbuf: imagesize " << m_screenbuf.get_imagesize()
			  << " center " << m_screenbuf.get_imagecenter() << " scale " << m_screenbuf.get_scale()
			  << " north " << m_screenbuf.get_upangle() << " alt " << m_screenbuf.get_altitude() << " maxalt " << m_screenbuf.get_maxalt()
			  << " time " << m_screenbuf.get_time() << " wind " << m_screenbuf.get_winddir() << '/' << m_screenbuf.get_windspeed()
			  << " flags " << m_screenbuf.get_drawflags()
			  << " window " << scscr << ' ' << (scscr + get_screensize()) << std::endl
			  << "    flags " << (!((get_drawflags() ^ m_screenbuf.get_drawflags()) & drawflags_mask))
			  << " alt " << (abs(get_altitude() - m_screenbuf.get_altitude()) <= alt_tolerance)
			  << " north " << (abs((int16_t)(get_upangle() - m_screenbuf.get_upangle())) <= upangle_tolerance)
			  << " scale " << check_window((float)(get_map_scale() / m_screenbuf.get_scale()), (float)(1. / (1 + map_tolerance)), (float)(1 + map_tolerance))
			  << " inside " << check_inside(m_screenbuf) << " gsinside " << gsrect.is_inside(m_screenbuf.get_center()) << std::endl
			  << "  drawbuf: imagesize " << m_drawbuf.get_imagesize()
			  << " center " << m_drawbuf.get_imagecenter() << " scale " << m_drawbuf.get_scale()
			  << " north " << m_drawbuf.get_upangle() << " alt " << m_drawbuf.get_altitude() << " maxalt " << m_drawbuf.get_maxalt()
			  << " time " << m_drawbuf.get_time() << " wind " << m_drawbuf.get_winddir() << '/' << m_drawbuf.get_windspeed()
			  << " flags " << m_drawbuf.get_drawflags()
			  << " window " << scdraw << ' ' << (scdraw + get_screensize()) << std::endl
			  << "    flags " << (!((get_drawflags() ^ m_drawbuf.get_drawflags()) & drawflags_mask))
			  << " alt " << (abs(get_altitude() - m_drawbuf.get_altitude()) <= alt_tolerance)
			  << " north " << (abs((int16_t)(get_upangle() - m_drawbuf.get_upangle())) <= upangle_tolerance)
			  << " scale " << check_window((float)(get_map_scale() / m_drawbuf.get_scale()), (float)(1. / (1 + map_tolerance)), (float)(1 + map_tolerance))
			  << " inside " << check_inside(m_drawbuf) << " gsinside " << gsrect.is_inside(m_drawbuf.get_center()) << std::endl;
	}
	if (is_hidden())
		return;
	Rect gsrect;
	{
		Point pd(TopoDb30::TopoCoordinate::get_pointsize());
		pd.set_lon(pd.get_lon() / 2);
		pd.set_lat(pd.get_lat() / 2);
		gsrect = Rect(get_center() - pd, get_center() + pd);
	}
	if (!((get_drawflags() ^ m_screenbuf.get_drawflags()) & drawflags_mask) &&
	    (!need_altitude() || abs(get_altitude() - m_screenbuf.get_altitude()) <= alt_tolerance) &&
	    abs((int16_t)(get_upangle() - m_screenbuf.get_upangle())) <= upangle_tolerance &&
	    check_window_relative((float)(get_map_scale() / m_screenbuf.get_scale()), map_tolerance) &&
	    check_inside(m_screenbuf) &&
	    (!need_glidemodel() || m_glidemodel.is_approxequal(m_screenbuf.get_glidemodel())) &&
	    (!need_glidemodel() || get_glidemodel().is_invalid() ||
	     (gsrect.is_inside(m_screenbuf.get_center()) &&
	      (fabsf(get_winddir() - m_screenbuf.get_winddir()) <= winddir_tolerance &&
		(fabsf(get_windspeed() - m_screenbuf.get_windspeed()) <= windspeed_abstolerance ||
		 check_window_relative(get_windspeed() / m_screenbuf.get_windspeed(), windspeed_tolerance))))))
		return;
	if (m_threadstate == thread_busy &&
	    !((get_drawflags() ^ m_drawbuf.get_drawflags()) & drawflags_mask) &&
	    (!need_altitude() || abs(get_altitude() - m_drawbuf.get_altitude()) <= alt_tolerance) &&
	    abs((int16_t)(get_upangle() - m_drawbuf.get_upangle())) <= upangle_tolerance &&
	    check_window_relative((float)(get_map_scale() / m_drawbuf.get_scale()), map_tolerance) &&
	    check_inside(m_drawbuf) &&
	    (!need_glidemodel() || m_glidemodel.is_approxequal(m_drawbuf.get_glidemodel())) &&
	    (!need_glidemodel() || get_glidemodel().is_invalid() ||
	     (gsrect.is_inside(m_drawbuf.get_center()) &&
	      (fabsf(get_winddir() - m_drawbuf.get_winddir()) <= winddir_tolerance &&
		(fabsf(get_windspeed() - m_drawbuf.get_windspeed()) <= windspeed_abstolerance ||
		 check_window_relative(get_windspeed() / m_drawbuf.get_windspeed(), windspeed_tolerance))))))
		return;
	if (true)
		std::cerr << "ThreadImageBuffer::check_restart: do restart" << std::endl;
	restart();
}

MapRenderer::ScreenCoord VectorMapRenderer::DrawThread::calc_image_drawoffset(const ImageBuffer<float>& imgbuf) const
{
	return to_screencoord(imgbuf(get_center()) - get_screencenter()) + get_offset();
}

void VectorMapRenderer::DrawThread::update_drawoffset(void)
{
	m_drawoffset = calc_image_drawoffset(m_screenbuf);
}

MapRenderer::ScreenCoordFloat VectorMapRenderer::DrawThread::operator()(const Point& pt) const
{
	return m_screenbuf(pt) - to_screencoordfloat(m_drawoffset);
}

Point VectorMapRenderer::DrawThread::operator()(float_t x, float_t y) const
{
	return m_screenbuf(x + m_drawoffset.getx(), y + m_drawoffset.gety());
}

Point VectorMapRenderer::DrawThread::operator()(const ScreenCoord& sc) const
{
	return (*this)(sc.getx(), sc.gety());
}

Point VectorMapRenderer::DrawThread::operator()(const ScreenCoordFloat& sc) const
{
	return (*this)(sc.getx(), sc.gety());
}

bool VectorMapRenderer::DrawThread::check_inside(const ImageBuffer<float>& imgbuf) const
{
	ScreenCoord sc(calc_image_drawoffset(imgbuf));
	if (sc.getx() < 0 || sc.gety() < 0)
		return false;
	sc += get_screensize();
	return (sc.getx() <= imgbuf.get_imagesize().getx() && sc.gety() <= imgbuf.get_imagesize().gety());
}

void VectorMapRenderer::DrawThread::draw(const Cairo::RefPtr<Cairo::Context>& cr)
{
	if (false)
		std::cerr << "ThreadImageBuffer::draw: imagesize " << m_screenbuf.get_imagesize()
			  << " center " << m_screenbuf.get_imagecenter() << " scale " << m_screenbuf.get_scale()
			  << " north " << m_screenbuf.get_upangle() << " alt " << m_screenbuf.get_altitude()
			  << " maxalt " << m_screenbuf.get_maxalt() << " time " << m_screenbuf.get_time()
			  << " flags " << m_screenbuf.get_drawflags() << std::endl;
	{
		Cairo::RefPtr<Cairo::SurfacePattern> pattern(m_screenbuf.create_pattern());
		if (pattern) {
			// set surface as pattern
			pattern->set_extend(Cairo::EXTEND_NONE);
			pattern->set_filter(Cairo::FILTER_GOOD);
			cr->save();
			// move origin
			// residual rotation
			int16_t upanglediff(get_upangle() - m_screenbuf.get_upangle());
			if (upanglediff) {
				int hx(m_screensize.getx()), hy(m_screensize.gety());
				cr->translate(hx - m_drawoffset.getx(), hy - m_drawoffset.gety());
				cr->rotate(upanglediff * MapRenderer::to_rad_16bit);
				cr->translate(-hx, -hy);
			} else {
				cr->translate(-m_drawoffset.getx(), -m_drawoffset.gety());
			}
			//cr->reset_clip();
			cr->set_source(pattern);
			cr->paint();
			cr->restore();
			// draw grib2 scale
			if (m_grib2surface) {
				cr->save();
				cr->select_font_face("Sans", Cairo::FONT_SLANT_NORMAL, Cairo::FONT_WEIGHT_NORMAL);
				cr->set_font_size(12);
				cr->translate(m_screensize.getx() - 10, 10);
				m_grib2surface->draw_scale(cr);
				cr->restore();
			}
			if (m_screenbuf.get_drawflags() & drawflags_weather) {
				cr->save();
				cr->select_font_face("Sans", Cairo::FONT_SLANT_NORMAL, Cairo::FONT_WEIGHT_NORMAL);
				cr->set_font_size(12);
				cr->translate(m_screensize.getx() - 10, m_screensize.gety() - 10);
				draw_wxinfo(cr.operator->());
				cr->restore();
			}
			if (false)
				std::cerr << "draw: surface " << m_screenbuf.get_imagesize() << " drawoffset " << m_drawoffset
					  << " center " << get_center() << " pixmap center " << m_screenbuf.get_imagecenter() << std::endl;
			return;
		}
        }
	cr->save();
	cr->set_source_rgb(1.0, 1.0, 1.0);
	cr->paint();
	cr->restore();
}

void VectorMapRenderer::DrawThread::terminate(void)
{
	m_threadstate = thread_terminate;
}


bool VectorMapRenderer::DrawThread::draw_checkabort(void)
{
	if (false)
		std::cerr << "draw_checkabort: threadstate: " << m_threadstate << std::endl;
	return m_threadstate == thread_terminate || m_hidden;
}

void VectorMapRenderer::DrawThread::draw_done(void)
{
	if (m_threadstate != thread_busy)
		return;
	m_drawbuf.flush_surface();
	m_screenbuf = m_drawbuf;
	m_drawbuf.reset();
	m_threadstate = thread_idle;
	update_drawoffset();
	m_dispatch();
	if (true)
		std::cerr << "update: new screen image: size " << m_screenbuf.get_imagesize() << " center " << m_screenbuf.get_imagecenter()
			  << " scale " << get_map_scale() << " north " << m_screenbuf.get_upangle() << " alt " << m_screenbuf.get_altitude()
			  << " maxalt " << get_maxalt() << " flags " << m_screenbuf.get_drawflags() << std::endl;
}

void VectorMapRenderer::DrawThread::restart(void)
{
	if (m_threadstate == thread_terminate)
		return;
	m_threadstate = thread_busy;
	m_drawbuf.reset(ScreenCoord(m_screensize.getx() * 2, m_screensize.gety() * 2), m_center, m_scale, m_upangle, m_altitude, m_maxalt,
			m_time, m_glidemodel, m_winddir, m_windspeed, m_drawflags, m_offset);
	bool dbquery(false);
	{
		Rect rect(draw_coverrect(2, 2));
		dbquery = !m_dbrectangle.is_inside(rect);
		if (dbquery)
			recompute_dbrectangle();
	}
	if (true)
		std::cerr << "draw thread: action: dbquery " << dbquery << " hidden " << is_hidden() << std::endl;
        draw_restart(dbquery);
	draw_iterate();
}

void VectorMapRenderer::DrawThread::recompute_dbrectangle(void)
{
	m_dbrectangle = draw_coverrect(3, 3);
}

void VectorMapRenderer::DrawThread::restart_db(void)
{
	clear_dbrectangle();
        restart();
}

void VectorMapRenderer::DrawThread::dbdone(void)
{
	if (m_threadstate != thread_busy)
		return;
	draw_iterate();
}

void VectorMapRenderer::DrawThread::airspaces_changed(void)
{
        if (get_drawflags() & drawflags_airspaces)
		restart_db();
}

void VectorMapRenderer::DrawThread::airports_changed(void)
{
        if (get_drawflags() & drawflags_airports)
		restart_db();
}

void VectorMapRenderer::DrawThread::navaids_changed(void)
{
        if (get_drawflags() & drawflags_navaids)
		restart_db();
}

void VectorMapRenderer::DrawThread::waypoints_changed(void)
{
        if (get_drawflags() & drawflags_waypoints)
		restart_db();
}

void VectorMapRenderer::DrawThread::airways_changed(void)
{
        if (get_drawflags() & (drawflags_airways_low | drawflags_airways_high))
		restart_db();
}

void VectorMapRenderer::DrawThread::async_done(void)
{
	if (false)
		std::cerr << "async_done" << std::endl;
	m_dbdispatch();
}

float VectorMapRenderer::DrawThread::nmi_to_pixel(float nmi) const
{
        return nmi / draw_get_scale();
}

Glib::ustring VectorMapRenderer::DrawThread::freqstr(uint32_t f)
{
        if (f < 1000)
                return Glib::ustring();
        char buf[16];
        f /= 1000;
        if (f < 1000) {
                snprintf(buf, sizeof(buf), "%u", f);
                return buf;
        }
        snprintf(buf, sizeof(buf), "%.3f", f * (1.0 / 1000));
        return buf;
}

void VectorMapRenderer::DrawThread::polypath(Cairo::Context *cr, const PolygonSimple& poly, bool closeit)
{
        if (poly.empty())
                return;
	draw_path(cr, poly.begin(), poly.end());
        if (closeit)
                cr->close_path();
}

void VectorMapRenderer::DrawThread::polypath(Cairo::Context *cr, const PolygonHole& poly, bool closeit)
{
        polypath(cr, poly.get_exterior(), closeit);
        for (unsigned int i = 0; i < poly.get_nrinterior(); i++)
                polypath(cr, poly[i], closeit);
}

void VectorMapRenderer::DrawThread::polypath(Cairo::Context *cr, const MultiPolygonHole& poly, bool closeit)
{
	for (MultiPolygonHole::const_iterator pi(poly.begin()), pe(poly.end()); pi != pe; ++pi)
		polypath(cr, *pi, closeit);
}

void VectorMapRenderer::DrawThread::draw_surface(Cairo::Context *cr, const Rect& cbbox, const Cairo::RefPtr<Cairo::ImageSurface>& pbuf, double alpha)
{
	if (!pbuf || std::isnan(alpha) || alpha <= 0)
		return;
        int pbufx = pbuf->get_width(), pbufy = pbuf->get_height();
        if (pbufx <= 0 || pbufy <= 0)
                return;
	Point psz(cbbox.get_northwest());
	Cairo::Matrix matrix(draw_cairo_matrix(psz));
	psz = cbbox.get_southeast() - psz;
	{
		if (false)
			std::cerr << "draw_surface: matrix [" << matrix.xx << ' ' << matrix.xy << ' ' << matrix.x0 << " ; "
				  << matrix.yx << ' ' << matrix.yy << ' ' << matrix.y0 << " ] size " << pbufx << " / " << pbufy << std::endl;
		double x(psz.get_lat()), y(psz.get_lon());
		cairo_matrix_transform_point(&matrix, &x, &y);
		if (false)
			std::cerr << "draw_surface: diff transform 1: (" << x << ',' << y << ')' << std::endl;
	}
	// "x" is usually latitude, "y" longitude - swap inputs
	std::swap(matrix.xx, matrix.xy);
	std::swap(matrix.yx, matrix.yy);
	{
		double x(psz.get_lon()), y(psz.get_lat());
		cairo_matrix_transform_point(&matrix, &x, &y);
		if (false)
			std::cerr << "draw_surface: diff transform 2: (" << x << ',' << y << ')' << std::endl;
	}
	cairo_matrix_scale(&matrix, psz.get_lon() / (double)pbufx, psz.get_lat() / (double)pbufy);
        cr->save();
	cr->transform(matrix);
	if (false)
		std::cerr << "draw_surface: matrix [" << matrix.xx << ' ' << matrix.xy << ' ' << matrix.x0 << " ; "
			  << matrix.yx << ' ' << matrix.yy << ' ' << matrix.y0 << " ] size " << pbufx << " / " << pbufy << std::endl;
	if (false)
		std::cerr << "draw_surface: cbbox " << cbbox << " center " << draw_get_imagecenter() << std::endl;
 	if (false)
		std::cerr << "draw_surface: " << cbbox.get_northwest() << " -> " << draw_transform(cbbox.get_northwest()) << std::endl
			  << "    " << cbbox.get_southeast() << " -> " << draw_transform(cbbox.get_southeast()) << std::endl;
 	if (false) {
		double x0(0), y0(0), x1(pbufx), y1(pbufy);
		cairo_matrix_transform_point(&matrix, &x0, &y0);
		cairo_matrix_transform_point(&matrix, &x1, &y1);
		std::cerr << "draw_surface: (0,0) -> (" << x0 << ',' << y0 << ')' << std::endl
			  << "    (" << pbufx << ',' << pbufy << ") -> (" << x1 << ',' << y1 << ')' << std::endl;
	}
        Cairo::RefPtr<Cairo::SurfacePattern> pat(Cairo::SurfacePattern::create(pbuf));
        cr->set_source(pat);
	if (alpha >= 1)
		cr->paint();
	else
		cr->paint_with_alpha(alpha);
        cr->restore();
}

void VectorMapRenderer::DrawThread::draw_pixbuf(const Cairo::RefPtr<Cairo::Context>& cr, const Rect& cbbox, const Glib::RefPtr<Gdk::Pixbuf>& pbuf, double alpha)
{
	if (!pbuf || std::isnan(alpha) || alpha <= 0)
		return;
        int pbufx = pbuf->get_width(), pbufy = pbuf->get_height();
        if (pbufx <= 0 || pbufy <= 0)
                return;
	Point psz(cbbox.get_northwest());
	Cairo::Matrix matrix(draw_cairo_matrix(psz));
	psz = cbbox.get_southeast() - psz;
	{
		if (false)
			std::cerr << "draw_pixbuf: matrix [" << matrix.xx << ' ' << matrix.xy << ' ' << matrix.x0 << " ; "
				  << matrix.yx << ' ' << matrix.yy << ' ' << matrix.y0 << " ] size " << pbufx << " / " << pbufy << std::endl;
		double x(psz.get_lat()), y(psz.get_lon());
		cairo_matrix_transform_point(&matrix, &x, &y);
		if (false)
			std::cerr << "draw_pixbuf: diff transform 1: (" << x << ',' << y << ')' << std::endl;
	}
	// "x" is usually latitude, "y" longitude - swap inputs
	std::swap(matrix.xx, matrix.xy);
	std::swap(matrix.yx, matrix.yy);
	{
		double x(psz.get_lon()), y(psz.get_lat());
		cairo_matrix_transform_point(&matrix, &x, &y);
		if (false)
			std::cerr << "draw_pixbuf: diff transform 2: (" << x << ',' << y << ')' << std::endl;
	}
	cairo_matrix_scale(&matrix, psz.get_lon() / (double)pbufx, psz.get_lat() / (double)pbufy);
        cr->save();
	cr->transform(matrix);
	if (false)
		std::cerr << "draw_pixbuf: matrix [" << matrix.xx << ' ' << matrix.xy << ' ' << matrix.x0 << " ; "
			  << matrix.yx << ' ' << matrix.yy << ' ' << matrix.y0 << " ] size " << pbufx << " / " << pbufy << std::endl;
	if (false)
		std::cerr << "draw_pixbuf: cbbox " << cbbox << " center " << draw_get_imagecenter() << std::endl;
 	if (false)
		std::cerr << "draw_pixbuf: " << cbbox.get_northwest() << " -> " << draw_transform(cbbox.get_northwest()) << std::endl
			  << "    " << cbbox.get_southeast() << " -> " << draw_transform(cbbox.get_southeast()) << std::endl;
 	if (false) {
		double x0(0), y0(0), x1(pbufx), y1(pbufy);
		cairo_matrix_transform_point(&matrix, &x0, &y0);
		cairo_matrix_transform_point(&matrix, &x1, &y1);
		std::cerr << "draw_pixbuf: (0,0) -> (" << x0 << ',' << y0 << ')' << std::endl
			  << "    (" << pbufx << ',' << pbufy << ") -> (" << x1 << ',' << y1 << ')' << std::endl;
	}
	Gdk::Cairo::set_source_pixbuf(cr, pbuf, 0, 0);
	if (alpha >= 1)
		cr->paint();
	else
		cr->paint_with_alpha(alpha);
	cr->restore();
}

void VectorMapRenderer::DrawThread::draw_pixbuf(const Cairo::RefPtr<Cairo::Context>& cr, const Corner& corner, const Glib::RefPtr<Gdk::Pixbuf>& pbuf, double alpha)
{
	if (true) {
		if (!pbuf || std::isnan(alpha) || alpha <= 0)
			return;
		int pbufx = pbuf->get_width(), pbufy = pbuf->get_height();
		if (pbufx <= 0 || pbufy <= 0)
			return;
		// wxmaxima:
		// A: matrix(
		//  [0,0,1],
		//  [x,0,1],
		//  [0,y,1],
		//  [x,y,1]
		// );
		// invert(transpose(A).A).transpose(A);
		Cairo::Matrix matrix;
		matrix.xx = matrix.xy = matrix.yx = matrix.yy = matrix.x0 = matrix.y0 = 0;
		{
			const double px = 0.5 / pbufx, py = 0.5 / pbufy;
			const double A[3][4] = {
				{ -px, px, -px, px },
				{ -py, -py, py, py },
				{ 0.75, 0.25, 0.25, -0.25 }
			};
			for (unsigned int i = 0; i < 4; ++i) {
				ScreenCoordFloat sc(draw_transform(corner[i]));
				matrix.xx += A[0][i] * sc.getx();
				matrix.xy += A[1][i] * sc.getx();
				matrix.x0 += A[2][i] * sc.getx();
				matrix.yx += A[0][i] * sc.gety();
				matrix.yy += A[1][i] * sc.gety();
				matrix.y0 += A[2][i] * sc.gety();
			}
		}
		cr->save();
		cr->transform(matrix);
		if (false)
			std::cerr << "draw_pixbuf: matrix [" << matrix.xx << ' ' << matrix.xy << ' ' << matrix.x0 << " ; "
				  << matrix.yx << ' ' << matrix.yy << ' ' << matrix.y0 << " ] size " << pbufx << " / " << pbufy << std::endl;
		if (false)
			std::cerr << "draw_pixbuf: " << corner[0] << " -> " << draw_transform(corner[0]) << std::endl
				  << "    " << corner[1] << " -> " << draw_transform(corner[1])
				  << "    " << corner[2] << " -> " << draw_transform(corner[2])
				  << "    " << corner[3] << " -> " << draw_transform(corner[3]) << std::endl;
		if (false) {
			double x0(0), y0(0), x1(pbufx), y1(pbufy);
			cairo_matrix_transform_point(&matrix, &x0, &y0);
			cairo_matrix_transform_point(&matrix, &x1, &y1);
			std::cerr << "draw_pixbuf: (0,0) -> (" << x0 << ',' << y0 << ')' << std::endl
				  << "    (" << pbufx << ',' << pbufy << ") -> (" << x1 << ',' << y1 << ')' << std::endl;
		}
		if (false) {
			for (unsigned int i = 0; i < 4; ++i) {
				ScreenCoordFloat sc(draw_transform(corner[i]));
				double x((i & 1) * pbufx), y(((i >> 1) & 1) * pbufy);
				std::cerr << "draw_pixbuf: " << x << ',' << y << ": exact " << sc.getx() << ',' << sc.gety() << " transform ";
				cairo_matrix_transform_point(&matrix, &x, &y);
				std::cerr << x << ',' << y << " error " << (x - sc.getx()) << ',' << (y - sc.gety()) << std::endl;
			}
		}
		Gdk::Cairo::set_source_pixbuf(cr, pbuf, 0, 0);
		if (alpha >= 1)
			cr->paint();
		else
			cr->paint_with_alpha(alpha);
		cr->restore();
	}
	draw_pixbuf(cr, (Rect)corner, pbuf, alpha);
}

void VectorMapRenderer::DrawThread::draw_scale(Cairo::Context *cr, float target_pixel)
{
	float scale(draw_get_scale());
        float nmi = target_pixel * scale;
        nmi = powf(10, floorf(log10f(nmi)));
        float pix1 = nmi / scale;
        if (pix1 < target_pixel * (1.0 / 2.0)) {
                pix1 *= 5;
                nmi *= 5;
        } else if (pix1 < target_pixel) {
                pix1 *= 2;
                nmi *= 2;
        }
        int pix = (int)pix1;
        //std::cerr << "draw_scale: nmi " << nmi << " pix " << pix << std::endl;
        std::ostringstream oss;
        oss << nmi;
        cr->save();
        cr->move_to(0, -5);
        cr->rel_line_to(0, 5);
        cr->rel_line_to(pix, 0);
        cr->rel_line_to(0, -5);
        cr->set_line_width(4.0);
        cr->set_source_rgb(1.0, 1.0, 1.0);
        cr->stroke_preserve();
        cr->set_line_width(2.0);
        cr->set_source_rgb(0, 0, 0);
        cr->stroke();
        Cairo::TextExtents ext;
        cr->get_text_extents(oss.str(), ext);
#if 0
        cr->move_to((pix - ext.width) / 2, -3);
        cr->show_text(oss.str());
#else
        cr->set_source_rgb(1.0, 1.0, 1.0);
        cr->move_to((pix - ext.width) / 2, -2);
        cr->show_text(oss.str());
        cr->move_to((pix - ext.width) / 2, -4);
        cr->show_text(oss.str());
        cr->move_to((pix - ext.width) / 2 - 1, -3);
        cr->show_text(oss.str());
        cr->move_to((pix - ext.width) / 2 + 1, -3);
        cr->show_text(oss.str());
        cr->move_to((pix - ext.width) / 2, -3);
        cr->set_source_rgb(0, 0, 0);
        cr->show_text(oss.str());
#endif
        cr->stroke();
        cr->restore();
}

void VectorMapRenderer::DrawThread::draw_north(Cairo::Context *cr)
{
	cr->rotate(screen_get_upangle() * -to_rad_16bit_dbl);
	cr->move_to(-0.5, 1);
	cr->line_to(-0.5, -1);
	cr->line_to(0.5, 1);
	cr->line_to(0.5, -1);
	cr->move_to(0, 1);
	cr->line_to(0, -2);
	cr->move_to(-0.5, -1.5);
	cr->line_to(0, -2);
	cr->line_to(0.5, -1.5);
        cr->set_line_width(0.2);
        cr->set_source_rgb(1.0, 1.0, 1.0);
        cr->stroke_preserve();
        cr->set_line_width(0.1);
        cr->set_source_rgb(0, 0, 0);
        cr->stroke();
}

void VectorMapRenderer::DrawThread::draw_route(Cairo::Context *cr, const FPlanRoute *route)
{
        if (!route)
                return;
        if (route->get_nrwpt() < 2)
                return;
        cr->save();
        cr->set_line_width(2.0);
        cr->set_source_rgb(0xff / 255.0, 0x00 / 255.0, 0xc8 / 255.0);
	std::vector<ScreenCoordFloat> coords(route->get_nrwpt());
	for (unsigned int i = 0; i < coords.size(); ++i)
		coords[i] = (*this)((*route)[i].get_coord());
	{
		std::vector<ScreenCoordFloat>::const_iterator ci(coords.begin()), ce(coords.end());
		if (ci != ce) {
			cr->move_to(ci->getx(), ci->gety());
			++ci;
		}
		for (; ci != ce; ++ci)
			cr->line_to(ci->getx(), ci->gety());
	}
        cr->stroke();
	if (get_drawflags() & drawflags_route_labels) {
		cr->select_font_face("Sans", Cairo::FONT_SLANT_NORMAL, Cairo::FONT_WEIGHT_BOLD);
		cr->set_font_size(12);
	        for (unsigned int i = 0; i < coords.size(); ++i) {
			Glib::ustring txt((*route)[i].get_icao());
			if (txt.empty())
				txt = (*route)[i].get_name();
			if (txt.empty())
				continue;
			Cairo::TextExtents ext;
			cr->get_text_extents(txt, ext);
			unsigned int mask(0);
			if (i > 0) {
				ScreenCoordFloat scf(coords[i-1] - coords[i]);
				mask |= 1 << ((scf.getx() < 0) | ((scf.gety() < 0) << 1));
			}
			if (i + 1 < coords.size()) {
				ScreenCoordFloat scf(coords[i+1] - coords[i]);
				mask |= 1 << ((scf.getx() < 0) | ((scf.gety() < 0) << 1));
			}
			ScreenCoordFloat scf(coords[i]);
			scf -= ScreenCoordFloat(ext.x_bearing, ext.y_bearing);
			// scf now points to top left corner of text
			if (!(mask & 0xC))
				scf -= ScreenCoordFloat(ext.width * 0.5, ext.height + 2);
			else if (!(mask & 0x3))
				scf -= ScreenCoordFloat(ext.width * 0.5, -2);
			else if (!(mask & 0xA))
				scf -= ScreenCoordFloat(ext.width + 2, ext.height * 0.5);
			else if (!(mask & 0x5))
				scf -= ScreenCoordFloat(-2, ext.height * 0.5);
			else if (!(mask & 0x1))
				scf += ScreenCoordFloat(2, 2);
			else if (!(mask & 0x2))
				scf -= ScreenCoordFloat(ext.width + 2, -2);
			else if (!(mask & 0x4))
				scf -= ScreenCoordFloat(-2, ext.height + 2);
			else if (!(mask & 0x4))
				scf -= ScreenCoordFloat(ext.width + 2, ext.height + 2);
			else
				scf -= ScreenCoordFloat(ext.width * 0.5, ext.height * 0.5);
			cr->move_to(scf.getx(), scf.gety());
			cr->show_text(txt);
		}
	}
        cr->restore();
}

void VectorMapRenderer::DrawThread::draw_track(Cairo::Context *cr, const TracksDb::Track *track)
{
        if (!track)
                return;
        if (get_drawflags() & drawflags_track_points) {
                cr->save();
                Cairo::RefPtr<Cairo::ImageSurface> pats(Cairo::ImageSurface::create(Cairo::FORMAT_ARGB32, 4, 4));
		int path(pats->get_height()), patw(pats->get_width());
                {
                        Cairo::RefPtr<Cairo::Context> crp(Cairo::Context::create(pats));
                        crp->set_source_rgba(1.0, 1.0, 1.0, 0.0);
                        crp->paint();
                        crp->set_source_rgba(255.0/255.0, 128.0/255.0, 0.0/255.0, 1.0);
                        crp->arc(patw / 2, path / 2, 2.0, 0.0, 2.0 * M_PI);
                        crp->fill();
                        //crp->paint();
                }
                Cairo::RefPtr<Cairo::SurfacePattern> pat(Cairo::SurfacePattern::create(pats));
                pat->set_extend(Cairo::EXTEND_NONE);
                pat->set_filter(Cairo::FILTER_FAST);
                cr->set_source(pat);
		Cairo::Matrix matrix;
		pat->get_matrix(matrix);
		cairo_matrix_translate(&matrix, patw/2, path/2);
                for (unsigned int i = 0; i < track->size(); i++) {
			ScreenCoordFloat px((*this)((*track)[i].get_coord()));
			Cairo::Matrix matrix2(matrix);
			cairo_matrix_translate(&matrix2, -px.getx(), -px.gety());
                        pat->set_matrix(matrix2);
			if (false)
				std::cerr << "Translate: " << px.getx() << ", " << px.gety() << std::endl;
                        cr->paint();
                }
                cr->restore();
                return;
        }
        if (track->size() < 2)
                return;
        cr->save();
        cr->set_line_width(2.0);
        cr->set_source_rgb(255.0/255.0, 128.0/255.0, 0.0/255.0);
	move_to(cr, (*track)[0].get_coord());
        for (unsigned int i = 1; i < track->size(); ++i)
		line_to(cr, (*track)[i].get_coord());
        cr->stroke();
        cr->restore();
}

void VectorMapRenderer::DrawThread::draw_label(Cairo::Context *cr, const Glib::ustring& text, const ScreenCoordFloat& pos, NavaidsDb::Navaid::label_placement_t lp, int dist, uint16_t upangle)
{
        if (text.empty() || lp == NavaidsDb::Navaid::label_off)
                return;
	{
		uint16_t pinc(upangle + (1 << 13));
		pinc >>= 14;
		pinc &= 3;
		if (pinc) {
			static const NavaidsDb::Navaid::label_placement_t placements[4] = {
				NavaidsDb::Navaid::label_n,
				NavaidsDb::Navaid::label_w,
				NavaidsDb::Navaid::label_s,
				NavaidsDb::Navaid::label_e
			};
			unsigned int i;
			for (i = 0; i < 4; ++i)
				if (placements[i] == lp)
					break;
			if (i < 4)
				lp = placements[(i + pinc) & 3];
		}
	}
        ScreenCoordFloat px(pos);
        cr->save();
        Cairo::TextExtents ext;
        cr->get_text_extents(text, ext);
        ScreenCoord tdim((int)ext.width, (int)ext.height);
	px -= ScreenCoordFloat(ext.x_bearing, ext.y_bearing);
        switch (lp) {
                default:
                        cr->restore();
                        return;

                case NavaidsDb::Navaid::label_n:
                        px -= ScreenCoordFloat(tdim.getx() * 0.5, dist + tdim.gety());
                        break;

                case NavaidsDb::Navaid::label_e:
                        px -= ScreenCoordFloat(-dist, tdim.gety() * 0.5);
                        break;

                case NavaidsDb::Navaid::label_s:
                        px -= ScreenCoordFloat(tdim.getx() * 0.5, -dist);
                        break;

                case NavaidsDb::Navaid::label_w:
                        px -= ScreenCoordFloat(tdim.getx() + dist, tdim.gety() * 0.5);
                        break;

                case NavaidsDb::Navaid::label_center:
                case NavaidsDb::Navaid::label_any:
                        px -= ScreenCoordFloat(tdim.getx() * 0.5, tdim.gety() * 0.5);
                        break;
        }
        cr->move_to(px.getx(), px.gety());
        cr->show_text(text);
        //std::cerr << "drawing text " << text << " at " << px << std::endl;
        cr->restore();
}

void VectorMapRenderer::DrawThread::draw_label(Cairo::Context *cr, const Glib::ustring& text, const Point& pt, NavaidsDb::Navaid::label_placement_t lp, int dist)
{
        if (text.empty() || lp == NavaidsDb::Navaid::label_off)
                return;
        draw_label(cr, text, draw_transform(pt), lp, dist, draw_get_upangle());
}

void VectorMapRenderer::DrawThread::draw_waypoint(Cairo::Context *cr, const Point& pt, const Glib::ustring& str, bool mandatory, NavaidsDb::Navaid::label_placement_t lp, bool setcolor)
{
        cr->save();
	if (setcolor)
		cr->set_source_rgb(0.0, 0.0, 0.0);
        cr->set_line_width(2.0);
        cr->set_fill_rule(Cairo::FILL_RULE_EVEN_ODD);
        ScreenCoordFloat px(draw_transform(pt));
        cr->move_to(px.getx() + 0.000 * 7, px.gety() - 1.000 * 7);
        cr->line_to(px.getx() - 0.866 * 7, px.gety() + 0.500 * 7);
        cr->line_to(px.getx() + 0.866 * 7, px.gety() + 0.500 * 7);
        cr->close_path();
        cr->arc(px.getx(), px.gety(), 2, 0, 2.0*M_PI);
        cr->close_path();
        if (mandatory)
                cr->fill();
        else
                cr->stroke();
        if (!str.empty())
                draw_label(cr, str, pt, lp, 8);
        cr->restore();
}

VectorMapRenderer::DrawThread::AirportRoutePoint::AirportRoutePoint(const Glib::ustring& name, const Point& coord, AirportsDb::Airport::label_placement_t lp)
        : m_name(name), m_coord(coord), m_labelplacement(lp)
{
}

VectorMapRenderer::DrawThread::AirportRoutePoint::AirportRoutePoint(const AirportsDb::Airport::VFRRoute::VFRRoutePoint & pt)
        : m_name(pt.get_name()), m_coord(pt.get_coord()), m_labelplacement(pt.get_label_placement())
{
}

bool VectorMapRenderer::DrawThread::AirportRoutePoint::operator<(const AirportRoutePoint & p) const
{
        if (m_coord.get_lon() < p.m_coord.get_lon())
                return true;
        if (m_coord.get_lon() > p.m_coord.get_lon())
                return false;
        if (m_coord.get_lat() < p.m_coord.get_lat())
                return true;
        if (m_coord.get_lat() > p.m_coord.get_lat())
                return false;
        return m_name < p.m_name;
}

void VectorMapRenderer::DrawThread::draw_airport_vfrreppt(Cairo::Context *cr, const AirportsDb::Airport& arpt)
{
        typedef std::map<AirportRoutePoint,bool> points_t;
        points_t pts;
        for (unsigned int i = 0; i < arpt.get_nrvfrrte(); i++) {
                const AirportsDb::Airport::VFRRoute& vfrrte(arpt.get_vfrrte(i));
                for (unsigned int j = 0; j < vfrrte.size(); j++) {
                        const AirportsDb::Airport::VFRRoute::VFRRoutePoint& vfrrtept(vfrrte[j]);
                        if (vfrrtept.is_at_airport())
                                continue;
                        Glib::ustring name(vfrrtept.get_name());
                        if (name.empty())
                                continue;
                        std::pair<points_t::iterator,bool> ins(pts.insert(points_t::value_type(AirportRoutePoint(vfrrtept), vfrrtept.is_compulsory())));
                        if (vfrrtept.is_compulsory() && ins.first != pts.end())
                                ins.first->second = true;
                }
        }
        for (points_t::const_iterator pi(pts.begin()), pe(pts.end()); pi != pe; pi++)
                draw_waypoint(cr, pi->first.get_coord(), pi->first.get_name(), pi->second, pi->first.get_label_placement());
}

void VectorMapRenderer::DrawThread::draw_navaid(Cairo::Context *cr, const NavaidsDb::Navaid& n, const ScreenCoordFloat& pos)
{
        int dist = 2;
        NavaidsDb::Navaid::navaid_type_t t(n.get_navaid_type());
        switch (t) {
                default:
                        return;

                case NavaidsDb::Navaid::navaid_ndb:
                case NavaidsDb::Navaid::navaid_ndbdme:
                {
                        cr->arc(pos.getx(), pos.gety(), 1, 0, 2.0*M_PI);
                        cr->fill();
                        cr->arc(pos.getx(), pos.gety(), 5, 0, 2.0*M_PI);
                        cr->stroke();
                        std::valarray<double> dash(1);
                        dash[0] = 1;
                        cr->set_dash(dash, 0);
                        for (double r = 7; r < 12; r += 2) {
                                cr->arc(pos.getx(), pos.gety(), r, 0, 2.0*M_PI);
                                cr->stroke();
                        }
                        std::valarray<double> nodash(0);
                        cr->set_dash(nodash, 0);
                        dist = 11;
                        if (t == NavaidsDb::Navaid::navaid_ndbdme) {
                                cr->move_to(pos.getx() + 1.000 * 6, pos.gety() + 0.866 * 6);
                                cr->line_to(pos.getx() + 1.000 * 6, pos.gety() - 0.866 * 6);
                                cr->line_to(pos.getx() - 1.000 * 6, pos.gety() - 0.866 * 6);
                                cr->line_to(pos.getx() - 1.000 * 6, pos.gety() + 0.866 * 6);
                                cr->close_path();
                                cr->stroke();
                        }
                        break;
                }

                case NavaidsDb::Navaid::navaid_vor:
                case NavaidsDb::Navaid::navaid_vordme:
                case NavaidsDb::Navaid::navaid_dme:
                case NavaidsDb::Navaid::navaid_vortac:
                case NavaidsDb::Navaid::navaid_tacan:
		{
                        cr->arc(pos.getx(), pos.gety(), 2, 0, 2.0*M_PI);
                        cr->fill();
			if (t == NavaidsDb::Navaid::navaid_vordme || t == NavaidsDb::Navaid::navaid_vor || t == NavaidsDb::Navaid::navaid_dme) {
				cr->move_to(pos.getx() + 1.000 * 6, pos.gety() + 0.000 * 6);
				cr->line_to(pos.getx() + 0.500 * 6, pos.gety() + 0.866 * 6);
				cr->line_to(pos.getx() - 0.500 * 6, pos.gety() + 0.866 * 6);
				cr->line_to(pos.getx() - 1.000 * 6, pos.gety() + 0.000 * 6);
				cr->line_to(pos.getx() - 0.500 * 6, pos.gety() - 0.866 * 6);
				cr->line_to(pos.getx() + 0.500 * 6, pos.gety() - 0.866 * 6);
				cr->close_path();
				cr->stroke();
			}
                        if (t == NavaidsDb::Navaid::navaid_vordme || t == NavaidsDb::Navaid::navaid_dme) {
                                cr->move_to(pos.getx() + 1.000 * 6, pos.gety() + 0.866 * 6);
                                cr->line_to(pos.getx() + 1.000 * 6, pos.gety() - 0.866 * 6);
                                cr->line_to(pos.getx() - 1.000 * 6, pos.gety() - 0.866 * 6);
                                cr->line_to(pos.getx() - 1.000 * 6, pos.gety() + 0.866 * 6);
                                cr->close_path();
                                cr->stroke();
                        }
			if (t == NavaidsDb::Navaid::navaid_vortac || t == NavaidsDb::Navaid::navaid_tacan) {
				// inner points: t1=exp(i*(120:60:450)/180*pi)
				// outer points: t2=t1+reshape([1;1]*exp(i*(150:120:450)/180*pi),1,6)
				// correct order: [t1(1) t2(1) t2(2) t1(2) t1(3) t2(3) t2(4) t1(4) t1(5) t2(5) t2(6) t1(6) t1(1)]
				cr->move_to(pos.getx() - 0.500 * 6, pos.gety() + 0.866 * 6);
				cr->line_to(pos.getx() - 1.366 * 6, pos.gety() + 1.366 * 6);
				cr->line_to(pos.getx() - 1.866 * 6, pos.gety() + 0.500 * 6);
				cr->line_to(pos.getx() - 1.000 * 6, pos.gety() + 0.000 * 6);
				cr->line_to(pos.getx() - 0.500 * 6, pos.gety() - 0.866 * 6);
				cr->line_to(pos.getx() - 0.500 * 6, pos.gety() - 1.866 * 6);
				cr->line_to(pos.getx() + 0.500 * 6, pos.gety() - 1.866 * 6);
				cr->line_to(pos.getx() + 0.500 * 6, pos.gety() - 0.866 * 6);
				cr->line_to(pos.getx() + 1.000 * 6, pos.gety() - 0.000 * 6);
				cr->line_to(pos.getx() + 1.866 * 6, pos.gety() + 0.500 * 6);
				cr->line_to(pos.getx() + 1.366 * 6, pos.gety() + 1.366 * 6);
				cr->line_to(pos.getx() + 0.500 * 6, pos.gety() + 0.866 * 6);
				cr->close_path();
                                cr->stroke();
			}
                        if (t == NavaidsDb::Navaid::navaid_vor || t == NavaidsDb::Navaid::navaid_vordme ||
			    t == NavaidsDb::Navaid::navaid_vortac) {
				cr->save();
				cr->translate(pos.getx(), pos.gety());
				cr->rotate(draw_get_upangle() * -to_rad_16bit_dbl);
                                cr->arc(0, 0, 20, 0, 2.0*M_PI);
                                cr->stroke();
                                cr->move_to(0 + 10, 0);
                                cr->line_to(0 + 20, 0);
                                cr->stroke();
                                cr->move_to(0 + 15, 0 + 2);
                                cr->line_to(0 + 20, 0);
                                cr->line_to(0 + 15, 0 - 2);
                                cr->stroke();
                                cr->move_to(0 - 10, 0);
                                cr->line_to(0 - 20, 0);
                                cr->stroke();
                                cr->move_to(0 - 15, 0 + 2);
                                cr->line_to(0 - 20, 0);
                                cr->line_to(0 - 15, 0 - 2);
                                cr->stroke();
                                cr->move_to(0, 0 + 10);
                                cr->line_to(0, 0 + 20);
                                cr->stroke();
                                cr->move_to(0 + 2, 0 + 15);
                                cr->line_to(0, 0 + 20);
                                cr->line_to(0 - 2, 0 + 15);
                                cr->stroke();
                                cr->move_to(0, 0 - 10);
                                cr->line_to(0, 0 - 20);
                                cr->stroke();
                                cr->move_to(0 + 2, 0 - 15);
                                cr->line_to(0, 0 - 20);
                                cr->line_to(0 - 2, 0 - 15);
                                cr->stroke();
				cr->restore();
                        }
                        dist = 20;
                        break;
                }
        }
        draw_label(cr, n.get_icao() + ' ' + freqstr(n.get_frequency()), pos, n.get_label_placement(), dist, draw_get_upangle());
}

constexpr float VectorMapRenderer::DrawThreadOverlay::airport_loccone_centerlength;
constexpr float VectorMapRenderer::DrawThreadOverlay::airport_loccone_sidelength;
constexpr float VectorMapRenderer::DrawThreadOverlay::airport_loccone_sideangle;

VectorMapRenderer::DrawThreadOverlay::DrawThreadOverlay(Engine& eng, Glib::Dispatcher& dispatch, const ScreenCoord& scrsize, const Point& center, int alt, uint16_t upangle, int64_t time)
        : DrawThread(eng, dispatch, scrsize, center, alt, upangle, time), m_wxreftime(0), m_wxefftime(0), m_wxaltitude(0)
{
}

VectorMapRenderer::DrawThreadOverlay::~DrawThreadOverlay()
{
	async_cancel();
}

void VectorMapRenderer::DrawThreadOverlay::airspaces_changed(void)
{
        Engine::AirspaceResult::cancel(m_airspacequery);
        DrawThread::airspaces_changed();
}

void VectorMapRenderer::DrawThreadOverlay::airports_changed(void)
{
        Engine::AirportResult::cancel(m_airportquery);
        DrawThread::airports_changed();
}

void VectorMapRenderer::DrawThreadOverlay::navaids_changed(void)
{
        Engine::NavaidResult::cancel(m_navaidquery);
        DrawThread::navaids_changed();
}

void VectorMapRenderer::DrawThreadOverlay::waypoints_changed(void)
{
        Engine::WaypointResult::cancel(m_waypointquery);
        DrawThread::waypoints_changed();
}

void VectorMapRenderer::DrawThreadOverlay::airways_changed(void)
{
        Engine::AirwayResult::cancel(m_airwayquery);
        DrawThread::airways_changed();
}

void VectorMapRenderer::DrawThreadOverlay::async_cancel(void)
{
        Engine::NavaidResult::cancel(m_navaidquery);
        Engine::WaypointResult::cancel(m_waypointquery);
        Engine::AirportResult::cancel(m_airportquery);
        Engine::AirspaceResult::cancel(m_airspacequery);
        Engine::AirwayResult::cancel(m_airwayquery);
	m_wxwindu.reset();
	m_wxwindv.reset();
	m_wxtemp.reset();
	m_wxqff.reset();
	m_wxrh.reset();
	m_wxprecip.reset();
	m_wxcldbdrycover.reset();
	m_wxcldlowcover.reset();
	m_wxcldlowbase.reset();
	m_wxcldlowtop.reset();
	m_wxcldmidcover.reset();
	m_wxcldmidbase.reset();
	m_wxcldmidtop.reset();
	m_wxcldhighcover.reset();
	m_wxcldhighbase.reset();
	m_wxcldhightop.reset();
	m_wxcldconvcover.reset();
	m_wxcldconvbase.reset();
	m_wxcldconvtop.reset();
}

bool VectorMapRenderer::DrawThreadOverlay::need_dbquery(void)
{
	return ((draw_get_drawflags() & drawflags_navaids) && (!m_navaidquery || m_navaidquery->is_error()))
		|| ((draw_get_drawflags() & drawflags_waypoints) && (!m_waypointquery || m_waypointquery->is_error()))
		|| ((draw_get_drawflags() & drawflags_airports) && (!m_airportquery || m_airportquery->is_error()))
		|| ((draw_get_drawflags() & drawflags_airspaces) && (!m_airspacequery || m_airspacequery->is_error()))
		|| ((draw_get_drawflags() & (drawflags_airways_low | drawflags_airways_high)) && (!m_airwayquery || m_airwayquery->is_error()));
}

void VectorMapRenderer::DrawThreadOverlay::db_restart(void)
{
	async_cancel();
	if (draw_get_drawflags() & drawflags_navaids)
		m_navaidquery = m_engine.async_navaid_find_bbox(get_dbrectangle(), ~0, NavaidsDb::element_t::subtables_all);
	if (draw_get_drawflags() & drawflags_waypoints)
		m_waypointquery = m_engine.async_waypoint_find_bbox(get_dbrectangle(), ~0, WaypointsDb::element_t::subtables_all);
	if (draw_get_drawflags() & drawflags_airports)
		m_airportquery = m_engine.async_airport_find_bbox(get_dbrectangle(), ~0, AirportsDb::element_t::subtables_runways |
								  AirportsDb::element_t::subtables_vfrroutes |
								  AirportsDb::element_t::subtables_fas);
	if (draw_get_drawflags() & drawflags_airspaces)
		m_airspacequery = m_engine.async_airspace_find_bbox(get_dbrectangle(), ~0, AirspacesDb::element_t::subtables_none);
	if (draw_get_drawflags() & (drawflags_airways_low | drawflags_airways_high))
		m_airwayquery = m_engine.async_airway_find_bbox(get_dbrectangle(), ~0, AirwaysDb::element_t::subtables_none);
	if (m_navaidquery)
		m_navaidquery->connect(sigc::mem_fun(*this, &VectorMapRenderer::DrawThreadVMap::async_done));
	if (m_waypointquery)
		m_waypointquery->connect(sigc::mem_fun(*this, &VectorMapRenderer::DrawThreadVMap::async_done));
	if (m_airportquery)
		m_airportquery->connect(sigc::mem_fun(*this, &VectorMapRenderer::DrawThreadVMap::async_done));
	if (m_airspacequery)
		m_airspacequery->connect(sigc::mem_fun(*this, &VectorMapRenderer::DrawThreadVMap::async_done));
	if (m_airwayquery)
		m_airwayquery->connect(sigc::mem_fun(*this, &VectorMapRenderer::DrawThreadVMap::async_done));
	if (draw_get_drawflags() & drawflags_weather) {
		m_wxwindu.reset();
		m_wxwindv.reset();
		m_wxtemp.reset();
		m_wxqff.reset();
		m_wxrh.reset();
		m_wxprecip.reset();
		m_wxcldbdrycover.reset();
		m_wxcldlowcover.reset();
		m_wxcldlowbase.reset();
		m_wxcldlowtop.reset();
		m_wxcldmidcover.reset();
		m_wxcldmidbase.reset();
		m_wxcldmidtop.reset();
		m_wxcldhighcover.reset();
		m_wxcldhighbase.reset();
		m_wxcldhightop.reset();
		m_wxcldconvcover.reset();
		m_wxcldconvbase.reset();
		m_wxcldconvtop.reset();
		float press(0);
		IcaoAtmosphere<float>::std_altitude_to_pressure(&press, 0, draw_get_altitude() * Point::ft_to_m);
		press *= 100;
		{
			GRIB2::layerlist_t ll(m_engine.get_grib2_db().find_layers(GRIB2::find_parameter(GRIB2::param_meteorology_momentum_ugrd),
										  draw_get_time(), GRIB2::surface_isobaric_surface, press));
			Glib::RefPtr<GRIB2::LayerInterpolateResult> x(GRIB2::interpolate_results(get_dbrectangle(), ll, draw_get_time(), press));
			if (x)
				m_wxwindu = x->get_results(draw_get_time(), press);
		}
		{
			GRIB2::layerlist_t ll(m_engine.get_grib2_db().find_layers(GRIB2::find_parameter(GRIB2::param_meteorology_momentum_vgrd),
										  draw_get_time(), GRIB2::surface_isobaric_surface, press));
			Glib::RefPtr<GRIB2::LayerInterpolateResult> x(GRIB2::interpolate_results(get_dbrectangle(), ll, draw_get_time(), press));
			if (x)
				m_wxwindv = x->get_results(draw_get_time(), press);
		}
		{
			GRIB2::layerlist_t ll(m_engine.get_grib2_db().find_layers(GRIB2::find_parameter(GRIB2::param_meteorology_temperature_tmp),
										  draw_get_time(), GRIB2::surface_isobaric_surface, press));
			std::cerr << "WX: Temp EffTime " << Glib::DateTime::create_now_utc(draw_get_time()).format("%F %H:%M:%S")
				  << " press " << press << "Pa" << std::endl;
			Glib::RefPtr<GRIB2::LayerInterpolateResult> x(GRIB2::interpolate_results(get_dbrectangle(), ll, draw_get_time(), press));
			if (x)
				m_wxtemp = x->get_results(draw_get_time(), press);
			if (m_wxtemp)
				std::cerr << "WX: Temp Interpolate EffTime " << Glib::DateTime::create_now_utc(m_wxtemp->get_efftime()).format("%F %H:%M:%S")
					  << " press " << m_wxtemp->get_surface1value() << "Pa" << std::endl;
		}
		{
			GRIB2::layerlist_t ll(m_engine.get_grib2_db().find_layers(GRIB2::find_parameter(GRIB2::param_meteorology_mass_prmsl),
										  draw_get_time()));
			Glib::RefPtr<GRIB2::LayerInterpolateResult> x(GRIB2::interpolate_results(get_dbrectangle(), ll, draw_get_time()));
			if (x)
				m_wxqff = x->get_results(draw_get_time());
		}
		{
			GRIB2::layerlist_t ll(m_engine.get_grib2_db().find_layers(GRIB2::find_parameter(GRIB2::param_meteorology_moisture_rh),
										  draw_get_time(), GRIB2::surface_isobaric_surface, press));
			Glib::RefPtr<GRIB2::LayerInterpolateResult> x(GRIB2::interpolate_results(get_dbrectangle(), ll, draw_get_time(), press));
			if (x)
				m_wxrh = x->get_results(draw_get_time(), press);
		}
		{
			GRIB2::layerlist_t ll(m_engine.get_grib2_db().find_layers(GRIB2::find_parameter(GRIB2::param_meteorology_moisture_apcp),
										  draw_get_time()));
			Glib::RefPtr<GRIB2::LayerInterpolateResult> x(GRIB2::interpolate_results(get_dbrectangle(), ll, draw_get_time()));
			if (x)
				m_wxprecip = x->get_results(draw_get_time());
		}
		{
			GRIB2::layerlist_t ll(m_engine.get_grib2_db().find_layers(GRIB2::find_parameter(GRIB2::param_meteorology_cloud_tcdc),
										  draw_get_time(), GRIB2::surface_boundary_layer_cloud, 0));
			Glib::RefPtr<GRIB2::LayerInterpolateResult> x(GRIB2::interpolate_results(get_dbrectangle(), ll, draw_get_time(), press));
			if (x)
				m_wxcldbdrycover = x->get_results(draw_get_time(), press);
		}
		{
			GRIB2::layerlist_t ll(m_engine.get_grib2_db().find_layers(GRIB2::find_parameter(GRIB2::param_meteorology_cloud_tcdc),
										  draw_get_time(), GRIB2::surface_low_cloud, 0));
			Glib::RefPtr<GRIB2::LayerInterpolateResult> x(GRIB2::interpolate_results(get_dbrectangle(), ll, draw_get_time(), press));
			if (x)
				m_wxcldlowcover = x->get_results(draw_get_time(), press);
		}
		{
			GRIB2::layerlist_t ll(m_engine.get_grib2_db().find_layers(GRIB2::find_parameter(GRIB2::param_meteorology_mass_pres),
										  draw_get_time(), GRIB2::surface_low_cloud_bottom, 0));
			Glib::RefPtr<GRIB2::LayerInterpolateResult> x(GRIB2::interpolate_results(get_dbrectangle(), ll, draw_get_time(), press));
			if (x)
				m_wxcldlowbase = x->get_results(draw_get_time(), press);
		}
		{
			GRIB2::layerlist_t ll(m_engine.get_grib2_db().find_layers(GRIB2::find_parameter(GRIB2::param_meteorology_mass_pres),
										  draw_get_time(), GRIB2::surface_low_cloud_top, 0));
			Glib::RefPtr<GRIB2::LayerInterpolateResult> x(GRIB2::interpolate_results(get_dbrectangle(), ll, draw_get_time(), press));
			if (x)
				m_wxcldlowtop = x->get_results(draw_get_time(), press);
		}
		{
			GRIB2::layerlist_t ll(m_engine.get_grib2_db().find_layers(GRIB2::find_parameter(GRIB2::param_meteorology_cloud_tcdc),
										  draw_get_time(), GRIB2::surface_middle_cloud, 0));
			Glib::RefPtr<GRIB2::LayerInterpolateResult> x(GRIB2::interpolate_results(get_dbrectangle(), ll, draw_get_time(), press));
			if (x)
				m_wxcldmidcover = x->get_results(draw_get_time(), press);
		}
		{
			GRIB2::layerlist_t ll(m_engine.get_grib2_db().find_layers(GRIB2::find_parameter(GRIB2::param_meteorology_mass_pres),
										  draw_get_time(), GRIB2::surface_middle_cloud_bottom, 0));
			Glib::RefPtr<GRIB2::LayerInterpolateResult> x(GRIB2::interpolate_results(get_dbrectangle(), ll, draw_get_time(), press));
			if (x)
				m_wxcldmidbase = x->get_results(draw_get_time(), press);
		}
		{
			GRIB2::layerlist_t ll(m_engine.get_grib2_db().find_layers(GRIB2::find_parameter(GRIB2::param_meteorology_mass_pres),
										  draw_get_time(), GRIB2::surface_middle_cloud_top, 0));
			Glib::RefPtr<GRIB2::LayerInterpolateResult> x(GRIB2::interpolate_results(get_dbrectangle(), ll, draw_get_time(), press));
			if (x)
				m_wxcldmidtop = x->get_results(draw_get_time(), press);
		}
		{
			GRIB2::layerlist_t ll(m_engine.get_grib2_db().find_layers(GRIB2::find_parameter(GRIB2::param_meteorology_cloud_tcdc),
										  draw_get_time(), GRIB2::surface_top_cloud, 0));
			Glib::RefPtr<GRIB2::LayerInterpolateResult> x(GRIB2::interpolate_results(get_dbrectangle(), ll, draw_get_time(), press));
			if (x)
				m_wxcldhighcover = x->get_results(draw_get_time(), press);
		}
		{
			GRIB2::layerlist_t ll(m_engine.get_grib2_db().find_layers(GRIB2::find_parameter(GRIB2::param_meteorology_mass_pres),
										  draw_get_time(), GRIB2::surface_top_cloud_bottom, 0));
			Glib::RefPtr<GRIB2::LayerInterpolateResult> x(GRIB2::interpolate_results(get_dbrectangle(), ll, draw_get_time(), press));
			if (x)
				m_wxcldhighbase = x->get_results(draw_get_time(), press);
		}
		{
			GRIB2::layerlist_t ll(m_engine.get_grib2_db().find_layers(GRIB2::find_parameter(GRIB2::param_meteorology_mass_pres),
										  draw_get_time(), GRIB2::surface_top_cloud_top, 0));
			Glib::RefPtr<GRIB2::LayerInterpolateResult> x(GRIB2::interpolate_results(get_dbrectangle(), ll, draw_get_time(), press));
			if (x)
				m_wxcldhightop = x->get_results(draw_get_time(), press);
		}
		{
			GRIB2::layerlist_t ll(m_engine.get_grib2_db().find_layers(GRIB2::find_parameter(GRIB2::param_meteorology_cloud_tcdc),
										  draw_get_time(), GRIB2::surface_convective_cloud, 0));
			Glib::RefPtr<GRIB2::LayerInterpolateResult> x(GRIB2::interpolate_results(get_dbrectangle(), ll, draw_get_time(), press));
			if (x)
				m_wxcldconvcover = x->get_results(draw_get_time(), press);
		}
		{
			GRIB2::layerlist_t ll(m_engine.get_grib2_db().find_layers(GRIB2::find_parameter(GRIB2::param_meteorology_mass_pres),
										  draw_get_time(), GRIB2::surface_convective_cloud_bottom, 0));
			Glib::RefPtr<GRIB2::LayerInterpolateResult> x(GRIB2::interpolate_results(get_dbrectangle(), ll, draw_get_time(), press));
			if (x)
				m_wxcldconvbase = x->get_results(draw_get_time(), press);
		}
		{
			GRIB2::layerlist_t ll(m_engine.get_grib2_db().find_layers(GRIB2::find_parameter(GRIB2::param_meteorology_mass_pres),
										  draw_get_time(), GRIB2::surface_convective_cloud_top, 0));
			Glib::RefPtr<GRIB2::LayerInterpolateResult> x(GRIB2::interpolate_results(get_dbrectangle(), ll, draw_get_time(), press));
			if (x)
				m_wxcldconvtop = x->get_results(draw_get_time(), press);
		}
	}
	m_grib2surface.reset();
	if (m_grib2layer) {
		if (false)
			std::cerr << "VMAP: has GRIB2 Layer" << std::endl;
		Glib::RefPtr<GRIB2Layer> sfc(new GRIB2Layer());
		if (sfc->compute(m_grib2layer, get_dbrectangle())) {
			m_grib2surface = sfc;
			if (false)
				std::cerr << "VMAP: GRIB2 Layer computed successfully" << std::endl;
		}
	}
}

bool VectorMapRenderer::DrawThreadOverlay::do_draw_navaids(Cairo::Context *cr)
{
	if ((draw_get_drawflags() & drawflags_navaids) && m_navaidquery) {
		if (!m_navaidquery->is_done())
			return false;
		if (!m_navaidquery->is_error()) {
			TimeMeasurement tm;
			draw(cr, m_navaidquery->get_result());
			std::cerr << "navaids drawing time " << tm << std::endl;
		}
	}
	return true;
}

bool VectorMapRenderer::DrawThreadOverlay::do_draw_waypoints(Cairo::Context *cr)
{
	if ((draw_get_drawflags() & drawflags_waypoints) && m_waypointquery) {
		if (!m_waypointquery->is_done())
			return false;
		if (!m_waypointquery->is_error()) {
			TimeMeasurement tm;
			draw(cr, m_waypointquery->get_result());
			std::cerr << "waypoints: " << m_waypointquery->get_result().size() << " drawing time " << tm << std::endl;
		}
	}
	return true;
}

bool VectorMapRenderer::DrawThreadOverlay::do_draw_airports(Cairo::Context *cr)
{
	if ((draw_get_drawflags() & drawflags_airports) && m_airportquery) {
		if (!m_airportquery->is_done())
			return false;
		if (!m_airportquery->is_error()) {
			TimeMeasurement tm;
			draw(cr, m_airportquery->get_result());
			std::cerr << "airports: " << m_airportquery->get_result().size() << " drawing time " << tm << std::endl;
		}
	}
	return true;
}

bool VectorMapRenderer::DrawThreadOverlay::do_draw_airspaces(Cairo::Context *cr)
{
	if ((draw_get_drawflags() & drawflags_airspaces) && m_airspacequery) {
		if (!m_airspacequery->is_done())
			return false;
		if (!m_airspacequery->is_error()) {
			TimeMeasurement tm;
			draw(cr, m_airspacequery->get_result());
			std::cerr << "airspaces: " << m_airspacequery->get_result().size() << " drawing time " << tm << std::endl;
		}
	}
	return true;
}

bool VectorMapRenderer::DrawThreadOverlay::do_draw_airways(Cairo::Context *cr)
{
	if ((draw_get_drawflags() & (drawflags_airways_low | drawflags_airways_high)) && !draw_checkabort() && m_airwayquery) {
		if (!m_airwayquery->is_done())
			return false;
		if (!m_airwayquery->is_error()) {
			TimeMeasurement tm;
			draw(cr, m_airwayquery->get_result());
			std::cerr << "airways: " << m_airwayquery->get_result().size() << " drawing time " << tm << std::endl;
		}
	}
	return true;
}

void VectorMapRenderer::DrawThreadOverlay::do_draw_weather(Cairo::Context *cr)
{
	if (draw_get_drawflags() & drawflags_weather && m_wxwindu && m_wxwindv) {
		TimeMeasurement tm;
		draw_weather(cr);
		m_wxreftime = m_wxwindu->get_layer()->get_reftime();
		m_wxefftime = m_wxwindu->get_efftime();
		{
			float alt(0);
			IcaoAtmosphere<float>::std_pressure_to_altitude(&alt, 0, m_wxwindu->get_surface1value() * 0.01);
			m_wxaltitude = Point::round<int,float>(alt * Point::m_to_ft);
		}
		std::cerr << "weather: " << m_wxwindu->get_width() << 'x' << m_wxwindu->get_height() << " scale " << draw_get_scale() << " drawing time " << tm << std::endl;
	} else {
		m_wxreftime = m_wxefftime = 0;
		m_wxaltitude = 0;
	}
}

void VectorMapRenderer::DrawThreadOverlay::do_draw_grib2layer(Cairo::Context *cr)
{
	if (m_grib2surface) {
		TimeMeasurement tm;
		cr->save();
		cr->set_source_rgba(0, 0, 0, 0.5);
		cr->paint();
		draw_surface(cr, m_grib2surface->get_bbox(), m_grib2surface->get_surface(), 0.5);
		cr->restore();
		std::cerr << "GRIB2 surface drawing time " << tm << std::endl;
	}
}

void VectorMapRenderer::DrawThreadOverlay::draw(Cairo::Context *cr, const std::vector<NavaidsDb::Navaid>& navaids)
{
        cr->save();
        cr->set_source_rgb(0.0, 0.0, 0.0);
        cr->set_line_width(1.0);
        cr->set_fill_rule(Cairo::FILL_RULE_EVEN_ODD);
        for (std::vector<NavaidsDb::Navaid>::const_iterator i1(navaids.begin()), ie(navaids.end()); i1 != ie && !draw_checkabort(); i1++) {
                const NavaidsDb::Navaid& n(*i1);
                NavaidsDb::Navaid::navaid_type_t t(n.get_navaid_type());
		{
			bool ok(false);
			ok = ok || (draw_get_drawflags() & drawflags_navaids_ndb && NavaidsDb::Navaid::has_ndb(t));
			ok = ok || (draw_get_drawflags() & drawflags_navaids_vor && NavaidsDb::Navaid::has_vor(t));
			ok = ok || (draw_get_drawflags() & drawflags_navaids_dme && NavaidsDb::Navaid::has_dme(t));
			if (!ok)
				continue;
                }
                draw_navaid(cr, n, draw_transform(n.get_coord()));
        }
        cr->restore();
}

bool VectorMapRenderer::DrawThreadOverlay::is_ground(const AirspacesDb::Airspace& a)
{
        uint8_t fl(a.get_altlwrflags());
        int32_t alt(a.get_altlwr());
        uint8_t tc(a.get_typecode());
        char bc(a.get_bdryclass());

        // do not fill class E, F or G airspaces
        if (tc != 255 && bc >= 'E' && bc <= 'G')
                return false;
        if (tc == 255 && (bc == 'A' || bc == 'D' || bc == 'M' || bc == 'P' || bc == 'R' || bc == 'T' || bc == 'W'))
                return false;
        if (fl & (AirspacesDb::Airspace::altflag_sfc | AirspacesDb::Airspace::altflag_gnd))
                return true;
        if (fl & AirspacesDb::Airspace::altflag_agl)
                return (alt < 500);
        return (a.get_altlwr_corr() <= a.get_gndelevmax());
}

void VectorMapRenderer::DrawThreadOverlay::draw(Cairo::Context *cr, const std::vector<AirspacesDb::Airspace>& airspaces)
{
        cr->save();
        cr->set_line_width(2.0);
        cr->set_fill_rule(Cairo::FILL_RULE_EVEN_ODD);
        //std::cerr << "Airspaces: draw flags: "
        //          << ((draw_get_drawflags() & drawflags_airspaces_specialuse) ? "SUAS " : "")
        //          << ((draw_get_drawflags() & drawflags_airspaces_ab) ? "AB" : "")
        //          << ((draw_get_drawflags() & drawflags_airspaces_cd) ? "CD" : "")
        //          << ((draw_get_drawflags() & drawflags_airspaces_efg) ? "EFG" : "") << std::endl;
        for (std::vector<AirspacesDb::Airspace>::const_iterator i1(airspaces.begin()), ie(airspaces.end()); i1 != ie && !draw_checkabort(); i1++) {
                const AirspacesDb::Airspace& a(*i1);
                // check for limit
                if (a.get_altlwr_corr() >= get_maxalt())
                        continue;
                if (a.get_typecode() == 255) {
                        // special use airspace
                        if (!(draw_get_drawflags() & drawflags_airspaces_specialuse))
                                continue;
                } else if (a.get_bdryclass() <= 'B') {
                        if (!(draw_get_drawflags() & drawflags_airspaces_ab))
                                continue;
                } else if (a.get_bdryclass() <= 'D') {
                        if (!(draw_get_drawflags() & drawflags_airspaces_cd))
                                continue;
                } else {
                        if (!(draw_get_drawflags() & drawflags_airspaces_efg))
                                continue;
                }
                Gdk::Color col = a.get_color();
                polypath(cr, a.get_polygon(), true);
                if ((draw_get_drawflags() & drawflags_airspaces_fill_ground) && is_ground(a)) {
                        cr->set_source_rgba(col.get_red_p(), col.get_green_p(), col.get_blue_p(), 0.5);
                        cr->fill_preserve();
                }
                cr->set_source_rgb(col.get_red_p(), col.get_green_p(), col.get_blue_p());
                cr->stroke();
                Glib::ustring buf1(a.get_name());
                Glib::ustring buf2(a.get_altlwr_string());
                Glib::ustring buf3(a.get_commname());
                Glib::ustring tmp1(a.get_class_string());
                if (!buf1.empty() && !tmp1.empty())
                        buf1 += ' ';
                buf1 += tmp1;
                tmp1 = a.get_altupr_string();
                if (!buf2.empty() && !tmp1.empty())
                        buf2 += ' ';
                buf2 += tmp1;
                tmp1 = freqstr(a.get_commfreq());
                if (!buf3.empty() && !tmp1.empty())
                        buf3 += ' ';
                buf3 += tmp1;
                Cairo::TextExtents ext1, ext2, ext3;
                cr->get_text_extents(buf1, ext1);
                cr->get_text_extents(buf2, ext2);
                cr->get_text_extents(buf3, ext3);
                double width = std::max(std::max(ext1.width, ext2.width), ext3.width);
                double height = ext1.height + ext2.height + ext3.height + 4;
                ScreenCoordFloat px(draw_transform(a.get_labelcoord()));
                cr->move_to(px.getx() - width * 0.5, px.gety() + height * 0.5);
                cr->show_text(buf3);
                cr->move_to(px.getx() - width * 0.5, px.gety() + height * 0.5 - ext3.height - 2);
                cr->show_text(buf2);
                cr->move_to(px.getx() - width * 0.5, px.gety() + height * 0.5 - ext3.height - ext2.height - 4);
                cr->show_text(buf1);
        }
        cr->restore();
}

void VectorMapRenderer::DrawThreadOverlay::draw(Cairo::Context *cr, const std::vector<AirportsDb::Airport>& airports)
{
        cr->save();
        cr->set_line_width(1.0);
	bool simplified(false);
	{
		unsigned int acnt(0);
		ScreenCoordFloat topleft(0, 0);
		ScreenCoordFloat bottomright(to_screencoordfloat(draw_get_imagesize()));
		for (std::vector<AirportsDb::Airport>::const_iterator i1(airports.begin()), ie(airports.end()); i1 != ie && !draw_checkabort(); i1++) {
			const AirportsDb::Airport& a(*i1);
			bool ismil(false);
			switch (a.get_typecode()) {
                        case 'A':
                                if (!(draw_get_drawflags() & drawflags_airports_civ))
                                        continue;
                                break;

                        case 'C':
                                if (!(draw_get_drawflags() & drawflags_airports_mil))
                                        continue;
                                ismil = true;
                                break;

                        default:
                                break;
			}
			ScreenCoordFloat px(draw_transform(a.get_coord()));
			if (!(px >= topleft && px <= bottomright))
				continue;
			++acnt;
			if (acnt < 50)
				continue;
			simplified = true;
			break;
		}
	}
        for (std::vector<AirportsDb::Airport>::const_iterator i1(airports.begin()), ie(airports.end()); i1 != ie && !draw_checkabort(); i1++) {
                const AirportsDb::Airport& a(*i1);
                bool ismil(false);
                switch (a.get_typecode()) {
                        case 'A':
                                if (!(draw_get_drawflags() & drawflags_airports_civ))
                                        continue;
                                break;

                        case 'C':
                                if (!(draw_get_drawflags() & drawflags_airports_mil))
                                        continue;
                                ismil = true;
                                break;

                        default:
                                break;
                }
		if (simplified) {
			if (ismil)
				cr->set_source_rgb(167.0 / 255, 101.0 / 255, 160.0 / 255);
			else
				cr->set_source_rgb(0.0, 0.0, 0.0);
			draw_waypoint(cr, a.get_coord(), a.get_icao(), true, a.get_label_placement(), false);
			continue;
		}
                int rwydir = -1, rwylen = -1;
                bool rwyasp = false;
                for (unsigned int i = 0; i < a.get_nrrwy(); i++) {
                        const AirportsDb::Airport::Runway& rwy(a.get_rwy(i));
                        bool thisrwyasp = rwy.is_concrete();
                        if (rwy.get_length() > rwylen || (thisrwyasp && !rwyasp)) {
                                rwylen = rwy.get_length();
                                rwydir = rwy.get_he_hdg();
                                rwyasp |= thisrwyasp;
                        }
                        // draw localizer cones
                        if (!(draw_get_drawflags() & drawflags_airports_localizercone) || a.get_nrfinalapproachsegments())
                                continue;
                        if (rwy.is_he_usable()) {
                                //std::cerr << a.get_icao() << " RWY " << rwy.get_ident_he() << std::endl;
                                Point pt(rwy.get_he_coord());
                                float ang((rwy.get_he_hdg() + 0x8000) * FPlanLeg::to_deg);
                                Point ptc(pt.spheric_course_distance_nmi(ang, airport_loccone_centerlength));
                                Point pts0(pt.spheric_course_distance_nmi(ang - airport_loccone_sideangle, airport_loccone_sidelength));
                                Point pts1(pt.spheric_course_distance_nmi(ang + airport_loccone_sideangle, airport_loccone_sidelength));
                                ScreenCoordFloat px(draw_transform(pt));
                                ScreenCoordFloat pxc(draw_transform(ptc));
                                ScreenCoordFloat pxs0(draw_transform(pts0));
                                ScreenCoordFloat pxs1(draw_transform(pts1));
                                ScreenCoordFloat pxd = (pxc - px);
                                if (pxd.getx() * pxd.getx() + pxd.gety() * pxd.gety() >= 100.0f) {
                                        cr->set_source_rgb(0.0 / 255, 148.0 / 255, 0.0 / 255);
                                        cr->move_to(px.getx(), px.gety());
                                        cr->line_to(pxs0.getx(), pxs0.gety());
                                        cr->line_to(pxc.getx(), pxc.gety());
                                        cr->line_to(pxs1.getx(), pxs1.gety());
                                        cr->line_to(px.getx(), px.gety());
                                        cr->line_to(pxc.getx(), pxc.gety());
                                        cr->stroke();
                                }
                        }
                        if (rwy.is_le_usable()) {
                                //std::cerr << a.get_icao() << " RWY " << rwy.get_ident_le() << std::endl;
                                Point pt(rwy.get_le_coord());
                                float ang((rwy.get_le_hdg() + 0x8000) * FPlanLeg::to_deg);
                                Point ptc(pt.spheric_course_distance_nmi(ang, airport_loccone_centerlength));
                                Point pts0(pt.spheric_course_distance_nmi(ang - airport_loccone_sideangle, airport_loccone_sidelength));
                                Point pts1(pt.spheric_course_distance_nmi(ang + airport_loccone_sideangle, airport_loccone_sidelength));
                                ScreenCoordFloat px(draw_transform(pt));
                                ScreenCoordFloat pxc(draw_transform(ptc));
                                ScreenCoordFloat pxs0(draw_transform(pts0));
                                ScreenCoordFloat pxs1(draw_transform(pts1));
                                ScreenCoordFloat pxd = (pxc - px);
                                if (pxd.getx() * pxd.getx() + pxd.gety() * pxd.gety() >= 100.0f) {
                                        cr->set_source_rgb(0.0 / 255, 148.0 / 255, 0.0 / 255);
                                        cr->move_to(px.getx(), px.gety());
                                        cr->line_to(pxs0.getx(), pxs0.gety());
                                        cr->line_to(pxc.getx(), pxc.gety());
                                        cr->line_to(pxs1.getx(), pxs1.gety());
                                        cr->line_to(px.getx(), px.gety());
                                        cr->line_to(pxc.getx(), pxc.gety());
                                        cr->stroke();
                                }
                        }
                }
		if ((draw_get_drawflags() & drawflags_airports_localizercone) && a.get_nrfinalapproachsegments()) {
			cr->set_source_rgb(0.0 / 255, 180.0 / 255, 0.0 / 255);
			for (unsigned int i = 0; i < a.get_nrfinalapproachsegments(); ++i) {
				const AirportsDb::Airport::MinimalFAS& mfas(a.get_finalapproachsegment(i));
				AirportsDb::Airport::FASNavigate nfas(mfas.navigate());
				if (!nfas.is_valid())
					continue;
				Point pt(nfas.get_fpap());
                                float ang(nfas.get_course_deg() + 180.0);
                                Point ptc(pt.spheric_course_distance_nmi(ang, airport_loccone_centerlength));
                                Point pts0(pt.spheric_course_distance_nmi(ang - nfas.get_coursedefl_deg(), airport_loccone_sidelength));
                                Point pts1(pt.spheric_course_distance_nmi(ang + nfas.get_coursedefl_deg(), airport_loccone_sidelength));
                                ScreenCoordFloat px(draw_transform(pt));
                                ScreenCoordFloat pxc(draw_transform(ptc));
                                ScreenCoordFloat pxs0(draw_transform(pts0));
                                ScreenCoordFloat pxs1(draw_transform(pts1));
                                ScreenCoordFloat pxd = (pxc - px);
                                if (pxd.getx() * pxd.getx() + pxd.gety() * pxd.gety() >= 100.0f) {
					cr->move_to(px.getx(), px.gety());
                                        cr->line_to(pxs0.getx(), pxs0.gety());
                                        cr->line_to(pxc.getx(), pxc.gety());
                                        cr->line_to(pxs1.getx(), pxs1.gety());
                                        cr->line_to(px.getx(), px.gety());
                                        cr->line_to(pxc.getx(), pxc.gety());
                                        cr->stroke();
                                }
			}
		}
                if (ismil)
                        cr->set_source_rgb(167.0 / 255, 101.0 / 255, 160.0 / 255);
                else
                        cr->set_source_rgb(0.0, 0.0, 0.0);
                ScreenCoordFloat px(draw_transform(a.get_coord()));
                if (rwydir == -1) {
			cr->begin_new_sub_path();
                        cr->arc(px.getx(), px.gety(), 15, 0, 2.0*M_PI);
                        cr->stroke();
                } else {
                        //if (a.get_icao() == "LSZK")
                        //        std::cerr << a.get_icao() << " rwydir " << rwydir << std::endl;
                        double phi((rwydir - draw_get_upangle()) * FPlanLeg::to_rad);
                        double c(cos(phi)), s(sin(phi));
                        ScreenCoordFloat p0(-2 * c + 20 * s, -2 * s - 20 * c);
                        ScreenCoordFloat p1(2 * c + 20 * s, 2 * s - 20 * c);
                        cr->move_to(px.getx() + p0.getx(), px.gety() + p0.gety());
                        cr->line_to(px.getx() + p1.getx(), px.gety() + p1.gety());
                        cr->line_to(px.getx() - p0.getx(), px.gety() - p0.gety());
                        cr->line_to(px.getx() - p1.getx(), px.gety() - p1.gety());
                        cr->close_path();
                        if (rwyasp)
                                cr->fill_preserve();
                        cr->stroke();
                        cr->arc(px.getx(), px.gety(), 15, phi + (0.5 * M_PI + 0.133), phi + (1.5 * M_PI - 0.133));
                        cr->stroke();
                        cr->arc(px.getx(), px.gety(), 15, phi + (1.5 * M_PI + 0.133), phi + (2.5 * M_PI - 0.133));
                        cr->stroke();
                }
                Glib::ustring buf1(a.get_icao());
                Glib::ustring buf2(a.get_name());
                if (!buf1.empty() && !buf2.empty())
                        buf1 += ' ';
                buf1 += buf2;
                draw_label(cr, buf1, a.get_coord(), a.get_label_placement(), 20);
                draw_airport_vfrreppt(cr, a);
        }
        cr->restore();
}

void VectorMapRenderer::DrawThreadOverlay::draw(Cairo::Context *cr, const std::vector<WaypointsDb::Waypoint>& waypoints)
{
        for (std::vector<WaypointsDb::Waypoint>::const_iterator i1(waypoints.begin()), ie(waypoints.end()); i1 != ie && !draw_checkabort(); i1++) {
                const WaypointsDb::Waypoint& w(*i1);
                switch (w.get_usage()) {
                        case WaypointsDb::Waypoint::usage_highlevel:
                                if (!(draw_get_drawflags() & drawflags_waypoints_high))
                                        continue;
                                break;

                        case WaypointsDb::Waypoint::usage_lowlevel:
                                if (!(draw_get_drawflags() & drawflags_waypoints_low))
                                        continue;
                                break;

                        case WaypointsDb::Waypoint::usage_bothlevel:
                                if (!(draw_get_drawflags() & (drawflags_waypoints_low | drawflags_waypoints_high)))
                                        continue;
                                break;

                        case WaypointsDb::Waypoint::usage_rnav:
                                if (!(draw_get_drawflags() & drawflags_waypoints_rnav))
                                        continue;
                                break;

                        case WaypointsDb::Waypoint::usage_terminal:
                                if (!(draw_get_drawflags() & drawflags_waypoints_terminal))
                                        continue;
                                break;

                        case WaypointsDb::Waypoint::usage_vfr:
                                if (!(draw_get_drawflags() & drawflags_waypoints_vfr))
                                        continue;
                                break;

                        default:
                        case WaypointsDb::Waypoint::usage_user:
                                if (!(draw_get_drawflags() & drawflags_waypoints_user))
                                        continue;
                                break;
                }
                draw_waypoint(cr, w.get_coord(), w.get_name(), true, w.get_label_placement());
        }
}

void VectorMapRenderer::DrawThreadOverlay::draw(Cairo::Context *cr, const std::vector<AirwaysDb::Airway>& airways)
{
        cr->save();
        cr->set_line_width(1.0);
        cr->set_source_rgb(0.0 / 255, 0.0 / 255, 0.0 / 255);
        for (std::vector<AirwaysDb::Airway>::const_iterator i1(airways.begin()), ie(airways.end()); i1 != ie && !draw_checkabort(); i1++) {
                const AirwaysDb::Airway& awy(*i1);
                switch (awy.get_type()) {
                        case AirwaysDb::Airway::airway_high:
                                if (!(draw_get_drawflags() & drawflags_airways_high))
                                        continue;
                                break;

                        case AirwaysDb::Airway::airway_low:
                                if (!(draw_get_drawflags() & drawflags_airways_low))
                                        continue;
                                break;

                        case AirwaysDb::Airway::airway_invalid:
                                continue;

                        default:
                                break;
                }
                {
                        ScreenCoordFloat pb(draw_transform(awy.get_begin_coord()));
                        ScreenCoordFloat pe(draw_transform(awy.get_end_coord()));
                        cr->move_to(pb.getx(), pb.gety());
                        cr->line_to(pe.getx(), pe.gety());
                        cr->stroke();
                        draw_label(cr, awy.get_name(), awy.get_labelcoord(), awy.get_label_placement(), 20);
                }
        }
        cr->restore();
}

void VectorMapRenderer::DrawThreadOverlay::draw_weather(Cairo::Context *cr)
{
	if (!m_wxwindu || !m_wxwindv)
		return;
	bool line1, line1simple, line2;
	{
		float sc(draw_get_scale());
		line2 = sc < 0.2;
		line1 = sc < 0.8;
		line1simple = sc >= 0.2;
	}
	cr->save();
	//cr->select_font_face("helvetica", Cairo::FONT_SLANT_NORMAL, Cairo::FONT_WEIGHT_NORMAL);
	cr->select_font_face("DejaVu Sans", Cairo::FONT_SLANT_NORMAL, Cairo::FONT_WEIGHT_NORMAL);
	cr->set_font_size(10);
	for (unsigned int u = 0, umax = m_wxwindu->get_width(); u < umax; ++u) {
		for (unsigned int v = 0, vmax = m_wxwindu->get_height(); v < vmax; ++v) {
			Point pt(m_wxwindu->get_center(u, v));
			ScreenCoordFloat px(draw_transform(pt));
			if (false)
				std::cerr << "WX: " << u << ' ' << v << " screen " << px.getx() << ' ' << px.gety() << std::endl;
			cr->set_source_rgb(0.9, 0.0, 0.1);
			cr->set_line_width(2.0);
			bool txtabove(true);
			{
				float windu(m_wxwindu->operator()(u, v));
				float windv(m_wxwindv->operator()(pt));
				{
					std::pair<float,float> w(m_wxwindu->get_layer()->get_grid()->transform_axes(windu, windv));
					windu = w.first;
					windv = w.second;
				}
				// convert from m/s to kts
				windu *= (-1e-3f * Point::km_to_nmi * 3600);
				windv *= (-1e-3f * Point::km_to_nmi * 3600);
				float windspeed(sqrtf(windu * windu + windv * windv));
				cr->begin_new_sub_path();
				cr->arc(px.getx(), px.gety(), 2.0, 0.0, 2.0 * M_PI);
				cr->close_path();
				cr->fill();
				// speed is wind speed in 5 knots increments
				int speed(Point::round<int,double>(windspeed * 0.2));
				if (speed > 0) {
					txtabove = windv >= 0;
					{
						float invwind(1.f / windspeed);
						windu *= invwind;
						windv *= invwind;
					}
					double y(34);
					cr->move_to(px.getx(), px.gety());
					cr->rel_line_to(y * windu, y * windv);
					cr->stroke();
					if (speed < 10)
						y += 4;
					while (speed >= 10) {
						speed -= 10;
						cr->move_to(px.getx() + y * windu, px.gety() + y * windv);
						cr->rel_line_to(-4 * windu + 8 * windv, -8 * windu - 4 * windv);
						cr->rel_line_to(-4 * windu - 8 * windv, 8 * windu - 4 * windv);
						cr->fill();
						y -= 8;
					}
					while (speed >= 2) {
						speed -= 2;
						y -= 4;
						cr->move_to(px.getx() + y * windu, px.gety() + y * windv);
						cr->rel_line_to(4 * windu + 8 * windv, -8 * windu + 4 * windv);
						cr->stroke();
					}
					if (speed) {
						y -= 4;
						cr->move_to(px.getx() + y * windu, px.gety() + y * windv);
						cr->rel_line_to(2 * windu + 4 * windv, -4 * windu + 2 * windv);
						cr->stroke();
					}
				}
				double y(px.gety());
				double baselineskip;
				{
					Cairo::TextExtents ext;
					cr->get_text_extents("8", ext);
					baselineskip = 1.2 * ext.height;
					y -= ext.y_bearing;
					if (txtabove) {
						y -= 10 + ext.height;
						baselineskip = -baselineskip;
					} else {
						y += 10;
					}
				}
				cr->set_source_rgb(0.0, 0.3, 0.0);
				for (unsigned int linenr = 0; linenr < 2; ++linenr) {
					unsigned int reallinenr(linenr);
					if (txtabove)
						reallinenr = 1 - linenr;
					switch (reallinenr) {
					case 0:
					{
						if (!line1)
							break;
						float temp(std::numeric_limits<float>::quiet_NaN());
						float qff(std::numeric_limits<float>::quiet_NaN());
						float rh(std::numeric_limits<float>::quiet_NaN());
						float precip(std::numeric_limits<float>::quiet_NaN());
						if (m_wxtemp)
							temp = m_wxtemp->operator()(pt);
						if (m_wxqff)
							qff = m_wxqff->operator()(pt);
						if (m_wxrh)
							rh = m_wxrh->operator()(pt);
						if (m_wxprecip)
							precip = m_wxprecip->operator()(pt); // kg/m^2 or mm
						std::ostringstream oss;
						if (line1simple) {
							if (!std::isnan(temp)) {
								float isa(0);
								IcaoAtmosphere<float>::std_altitude_to_pressure(0, &isa, draw_get_altitude() * Point::ft_to_m);
								oss << Point::round<int,float>(temp - IcaoAtmosphere<float>::degc_to_kelvin);
							}
						} else {
							if (!std::isnan(temp)) {
								float isa(0);
								IcaoAtmosphere<float>::std_altitude_to_pressure(0, &isa, draw_get_altitude() * Point::ft_to_m);
								oss << 'T' << Point::round<int,float>(temp - IcaoAtmosphere<float>::degc_to_kelvin)
								    << " (ISA" << std::showpos << Point::round<int,float>(temp  - isa) << ')' << std::noshowpos;
							}
							if (!std::isnan(rh)) {
								if (!oss.str().empty())
									oss << ' ';
								oss << "RH" << Point::round<int,float>(rh) << '%';
							}
							if (!std::isnan(qff)) {
								if (!oss.str().empty())
									oss << ' ';
								oss << 'Q' << ((Point::round<int,float>(qff) + 50) / 100);
							}
							if (!std::isnan(precip) && precip >= 0.5) {
								if (!oss.str().empty())
									oss << ' ';
								oss << "\342\230\224" << Point::round<int,float>(precip) << "mm";
							}
						}
						Cairo::TextExtents ext;
						cr->get_text_extents(oss.str(), ext);
						cr->move_to(px.getx() - ext.x_bearing - ext.width * 0.5, y);
						cr->show_text(oss.str());
						y += baselineskip;
						break;
					}

					case 1:
					{
						if (!line2)
							break;
						if (!m_wxcldbdrycover && !m_wxcldlowcover && !m_wxcldmidcover && !m_wxcldhighcover && !m_wxcldconvcover)
							break;
						std::ostringstream oss;
						if (m_wxcldbdrycover) {
							float cov(m_wxcldbdrycover->operator()(pt));
							if (!std::isnan(cov)) {
								if (cov <= 2)
									oss << "\342\227\213";
								else if (cov <= 10)
									oss << "\342\246\266";
								else if (cov <= 27.5)
									oss << "\342\227\224";
								else if (cov <= 62.5)
									oss << "\342\227\221";
								else if (cov <= 87.5)
									oss << "\342\227\225";
								else
									oss << "\342\227\217";
							}
						}
						oss << '/';
						if (m_wxcldlowcover) {
							float cov(m_wxcldlowcover->operator()(pt));
							float base(std::numeric_limits<float>::quiet_NaN());
							float top(std::numeric_limits<float>::quiet_NaN());
							if (m_wxcldlowbase)
								base = m_wxcldlowbase->operator()(pt);
							if (m_wxcldlowtop)
								top = m_wxcldlowtop->operator()(pt);
							if (!std::isnan(cov)) {
								if (cov <= 2)
									oss << "\342\227\213";
								else if (cov <= 10)
									oss << "\342\246\266";
								else if (cov <= 27.5)
									oss << "\342\227\224";
								else if (cov <= 62.5)
									oss << "\342\227\221";
								else if (cov <= 87.5)
									oss << "\342\227\225";
								else
									oss << "\342\227\217";
							}
							if (!std::isnan(base)) {
								float alt(0);
								IcaoAtmosphere<float>::std_pressure_to_altitude(&alt, 0, base * 0.01);
								oss << std::setw(3) << std::setfill('0') << Point::round<int,float>(alt * (Point::m_to_ft * 0.01f));
							}
							if (!std::isnan(base) || !std::isnan(top))
								oss << '-';
							if (!std::isnan(top)) {
								float alt(0);
								IcaoAtmosphere<float>::std_pressure_to_altitude(&alt, 0, top * 0.01);
								oss << std::setw(3) << std::setfill('0') << Point::round<int,float>(alt * (Point::m_to_ft * 0.01f));
							}
						}
						oss << '/';
						if (m_wxcldmidcover) {
							float cov(m_wxcldmidcover->operator()(pt));
							float base(std::numeric_limits<float>::quiet_NaN());
							float top(std::numeric_limits<float>::quiet_NaN());
							if (m_wxcldmidbase)
								base = m_wxcldmidbase->operator()(pt);
							if (m_wxcldmidtop)
								top = m_wxcldmidtop->operator()(pt);
							if (!std::isnan(cov)) {
								if (cov <= 2)
									oss << "\342\227\213";
								else if (cov <= 10)
									oss << "\342\246\266";
								else if (cov <= 27.5)
									oss << "\342\227\224";
								else if (cov <= 62.5)
									oss << "\342\227\221";
								else if (cov <= 87.5)
									oss << "\342\227\225";
								else
									oss << "\342\227\217";
							}
							if (!std::isnan(base)) {
								float alt(0);
								IcaoAtmosphere<float>::std_pressure_to_altitude(&alt, 0, base * 0.01);
								oss << std::setw(3) << std::setfill('0') << Point::round<int,float>(alt * (Point::m_to_ft * 0.01f));
							}
							if (!std::isnan(base) || !std::isnan(top))
								oss << '-';
							if (!std::isnan(top)) {
								float alt(0);
								IcaoAtmosphere<float>::std_pressure_to_altitude(&alt, 0, top * 0.01);
								oss << std::setw(3) << std::setfill('0') << Point::round<int,float>(alt * (Point::m_to_ft * 0.01f));
							}
						}
						oss << '/';
						if (m_wxcldhighcover) {
							float cov(m_wxcldhighcover->operator()(pt));
							float base(std::numeric_limits<float>::quiet_NaN());
							float top(std::numeric_limits<float>::quiet_NaN());
							if (m_wxcldhighbase)
								base = m_wxcldhighbase->operator()(pt);
							if (m_wxcldhightop)
								top = m_wxcldhightop->operator()(pt);
							if (!std::isnan(cov)) {
								if (cov <= 2)
									oss << "\342\227\213";
								else if (cov <= 10)
									oss << "\342\246\266";
								else if (cov <= 27.5)
									oss << "\342\227\224";
								else if (cov <= 62.5)
									oss << "\342\227\221";
								else if (cov <= 87.5)
									oss << "\342\227\225";
								else
									oss << "\342\227\217";
							}
							if (!std::isnan(base)) {
								float alt(0);
								IcaoAtmosphere<float>::std_pressure_to_altitude(&alt, 0, base * 0.01);
								oss << std::setw(3) << std::setfill('0') << Point::round<int,float>(alt * (Point::m_to_ft * 0.01f));
							}
							if (!std::isnan(base) || !std::isnan(top))
								oss << '-';
							if (!std::isnan(top)) {
								float alt(0);
								IcaoAtmosphere<float>::std_pressure_to_altitude(&alt, 0, top * 0.01);
								oss << std::setw(3) << std::setfill('0') << Point::round<int,float>(alt * (Point::m_to_ft * 0.01f));
							}
						}
						oss << '/';
						if (m_wxcldconvcover) {
							float cov(m_wxcldconvcover->operator()(pt));
							float base(std::numeric_limits<float>::quiet_NaN());
							float top(std::numeric_limits<float>::quiet_NaN());
							if (m_wxcldconvbase)
								base = m_wxcldconvbase->operator()(pt);
							if (m_wxcldconvtop)
								top = m_wxcldconvtop->operator()(pt);
							if (!std::isnan(cov)) {
								if (cov <= 2)
									oss << "\342\227\213";
								else if (cov <= 10)
									oss << "\342\246\266";
								else if (cov <= 27.5)
									oss << "\342\227\224";
								else if (cov <= 62.5)
									oss << "\342\227\221";
								else if (cov <= 87.5)
									oss << "\342\227\225";
								else
									oss << "\342\227\217";
							}
							if (!std::isnan(base)) {
								float alt(0);
								IcaoAtmosphere<float>::std_pressure_to_altitude(&alt, 0, base * 0.01);
								oss << std::setw(3) << std::setfill('0') << Point::round<int,float>(alt * (Point::m_to_ft * 0.01f));
							}
							if (!std::isnan(base) || !std::isnan(top))
								oss << '-';
							if (!std::isnan(top)) {
								float alt(0);
								IcaoAtmosphere<float>::std_pressure_to_altitude(&alt, 0, top * 0.01);
								oss << std::setw(3) << std::setfill('0') << Point::round<int,float>(alt * (Point::m_to_ft * 0.01f));
							}
						}
						Cairo::TextExtents ext;
						cr->get_text_extents(oss.str(), ext);
						cr->move_to(px.getx() - ext.x_bearing - ext.width * 0.5, y);
						cr->show_text(oss.str());
						y += baselineskip;
						break;
					}

					default:
						break;
					}
				}

			}
		}
	}
	cr->restore();
}

void VectorMapRenderer::DrawThreadOverlay::draw_wxinfo(Cairo::Context *cr)
{
	double y(0);
	{
		std::ostringstream oss;
		oss << 'F' << std::setfill('0') << std::setw(3) << ((m_wxaltitude + 50) / 100)
		    << " Eff " << Glib::DateTime::create_now_utc(m_wxefftime).format("%F %H:%M:%S");
		Cairo::TextExtents ext;
                cr->get_text_extents(oss.str(), ext);
		double ty(y - ext.y_bearing - ext.height);
		double tx(- ext.x_bearing - ext.width);
		cr->set_source_rgb(1.0, 1.0, 1.0);
		cr->move_to(tx - 1, ty);
		cr->show_text(oss.str());
		cr->move_to(tx + 1, ty);
		cr->show_text(oss.str());
		cr->move_to(tx, ty - 1);
		cr->show_text(oss.str());
		cr->move_to(tx, ty + 1);
		cr->show_text(oss.str());
		cr->set_source_rgb(0.0, 0.0, 0.0);
		cr->move_to(tx, ty);
		cr->show_text(oss.str());
		y -= 1.2 * ext.height;
	}
	{
		std::ostringstream oss;
		oss << "Ref " << Glib::DateTime::create_now_utc(m_wxreftime).format("%F %H:%M:%S");
		Cairo::TextExtents ext;
                cr->get_text_extents(oss.str(), ext);
		double ty(y - ext.y_bearing - ext.height);
		double tx(- ext.x_bearing - ext.width);
		cr->set_source_rgb(1.0, 1.0, 1.0);
		cr->move_to(tx - 1, ty);
		cr->show_text(oss.str());
		cr->move_to(tx + 1, ty);
		cr->show_text(oss.str());
		cr->move_to(tx, ty - 1);
		cr->show_text(oss.str());
		cr->move_to(tx, ty + 1);
		cr->show_text(oss.str());
		cr->set_source_rgb(0.0, 0.0, 0.0);
		cr->move_to(tx, ty);
		cr->show_text(oss.str());
	}
}

VectorMapRenderer::DrawThreadVMap::DrawThreadVMap(Engine& eng, Glib::Dispatcher& dispatch, const ScreenCoord& scrsize, const Point& center, int alt, uint16_t upangle, int64_t time)
        : DrawThreadOverlay(eng, dispatch, scrsize, center, alt, upangle, time), m_drawstate(drawstate_done)
{
}

VectorMapRenderer::DrawThreadVMap::~DrawThreadVMap()
{
        async_cancel();
}

void VectorMapRenderer::DrawThreadVMap::async_cancel(void)
{
        Engine::ElevationMapCairoResult::cancel(m_topocairoquery);
        Engine::MapelementResult::cancel(m_mapelementquery);
	DrawThreadOverlay::async_cancel();
}

void VectorMapRenderer::DrawThreadVMap::draw_restart(bool dbquery)
{
	m_drawstate = drawstate_topo;
	dbquery = dbquery
		|| ((draw_get_drawflags() & drawflags_topo) && (!m_topocairoquery || m_topocairoquery->is_error()))
		|| ((draw_get_drawflags() & drawflags_terrain_all) && (!m_mapelementquery || m_mapelementquery->is_error()))
		|| need_dbquery();
        if (dbquery) {
		recompute_dbrectangle();
		std::cerr << "bbox: " << get_dbrectangle() << " (center " << draw_get_imagecenter() << ')' << std::endl;
                async_cancel();
                if (draw_get_drawflags() & drawflags_topo)
                        m_topocairoquery = m_engine.async_elevation_map_cairo(get_dbrectangle());
                if (draw_get_drawflags() & drawflags_terrain_all)
                        m_mapelementquery = m_engine.async_mapelement_find_bbox(get_dbrectangle(), ~0, MapelementsDb::element_t::subtables_all);
                if (m_topocairoquery)
                        m_topocairoquery->connect(sigc::mem_fun(*this, &VectorMapRenderer::DrawThreadVMap::async_done));
                if (m_mapelementquery)
                        m_mapelementquery->connect(sigc::mem_fun(*this, &VectorMapRenderer::DrawThreadVMap::async_done));
		db_restart();
        }
        // start drawing
        Cairo::RefPtr<Cairo::Context> cr = draw_create_context();
        cr->rectangle(0, 0, draw_get_imagesize().getx(), draw_get_imagesize().gety());
        cr->set_source_rgb(1.0, 1.0, 1.0);
        cr->fill();
}

void VectorMapRenderer::DrawThreadVMap::draw_iterate(void)
{
	Cairo::RefPtr<Cairo::Context> cr = draw_create_context();
        cr->rectangle(0, 0, draw_get_imagesize().getx(), draw_get_imagesize().gety());
        cr->clip();
        //cr->set_tolerance(1.0);

	switch (m_drawstate) {
	case drawstate_topo:
		// draw topography
		if ((draw_get_drawflags() & drawflags_topo) && m_topocairoquery) {
			if (!m_topocairoquery->is_done())
				return;
			if (!m_topocairoquery->is_error()) {
				TimeMeasurement tm;
				tm.start();
				draw_surface(cr.operator->(), m_topocairoquery->get_bbox(), m_topocairoquery->get_surface(), 1);
				std::cerr << "topo cairo: drawing time " << tm << std::endl;
			}
		}
		m_drawstate = drawstate_mapelements;
		// fall through

	case drawstate_mapelements:
		if ((draw_get_drawflags() & drawflags_terrain_all) && m_mapelementquery) {
			if (!m_mapelementquery->is_done())
				return;
			if (!m_mapelementquery->is_error()) {
				mapel_render_comp comp;
				TimeMeasurement tm;
				sort(m_mapelementquery->get_result().begin(), m_mapelementquery->get_result().end(), comp);
				std::cerr << "mapel sort: " << m_mapelementquery->get_result().size() << " time " << tm << std::endl;
				tm.start();
				unsigned int sz(0);
				for (MapelementsDb::elementvector_t::const_iterator i1(m_mapelementquery->get_result().begin()), ie(m_mapelementquery->get_result().end()); i1 != ie; i1++) {
					if (draw_checkabort())
						break;
					if (false && !(*i1).get_name().casefold().compare(0, 4, Glib::ustring("Biel").casefold()))
						std::cerr << "Mapel: " << (*i1).get_name() << " bbox " << (*i1).get_bbox() << std::endl;
					draw(cr.operator->(), *i1);
					if (!i1->get_name().empty())
						sz++;
				}
				if ((draw_get_drawflags() & drawflags_terrain_names) && (sz < 100) && !draw_checkabort()) {
					cr->save();
					cr->select_font_face("Sans", Cairo::FONT_SLANT_NORMAL, Cairo::FONT_WEIGHT_NORMAL);
					cr->set_font_size(12);
					cr->set_source_rgb(0.0, 0.0, 0.0);
					for (MapelementsDb::elementvector_t::const_iterator i1(m_mapelementquery->get_result().begin()), ie(m_mapelementquery->get_result().end()); i1 != ie; i1++) {
						if (draw_checkabort())
							break;
						draw_text(cr.operator->(), *i1);
					}
					cr->restore();
				}
				std::cerr << "mapel drawing time " << tm << std::endl;
			}
		}
		m_drawstate = drawstate_navaids;
		// fall through

	case drawstate_navaids:
		if (!do_draw_navaids(cr.operator->()))
			return;
		m_drawstate = drawstate_waypoints;
		// fall through

	case drawstate_waypoints:
		if (!do_draw_waypoints(cr.operator->()))
			return;
		m_drawstate = drawstate_airports;
		// fall through

	case drawstate_airports:
		if (!do_draw_airports(cr.operator->()))
			return;
		m_drawstate = drawstate_airspaces;
		// fall through

	case drawstate_airspaces:
		if (!do_draw_airspaces(cr.operator->()))
			return;
		m_drawstate = drawstate_airways;
		// fall through

	case drawstate_airways:
		if (!do_draw_airways(cr.operator->()))
			return;
		m_drawstate = drawstate_weather;
		// fall through

	case drawstate_weather:
		do_draw_weather(cr.operator->());
		m_drawstate = drawstate_grib2layer;
		// fall through

	case drawstate_grib2layer:
		do_draw_grib2layer(cr.operator->());
		m_drawstate = drawstate_done;
		// fall through

	case drawstate_done:
	default:
		draw_done();
		break;
	}
}

void VectorMapRenderer::DrawThreadVMap::draw(Cairo::Context *cr, const MapelementsDb::Mapelement& mapel)
{
        if (false && !mapel.get_name().casefold().compare(0, 4, Glib::ustring("Biel").casefold())) {
                std::cerr << "Mapel: " << mapel.get_name() << " bbox " << mapel.get_bbox() << " type " << mapel.get_typecode() << std::endl;
                const PolygonHole& ph(mapel.get_polygon());
                const PolygonSimple& poly(ph.get_exterior());
                std::cerr << "  Exterior:" << std::endl;
                for (PolygonSimple::const_iterator pi(poly.begin()), pe(poly.end()); pi != pe; ++pi)
                        std::cerr << "    " << pi->get_lat_str() << ", " << pi->get_lon_str() << std::endl;
                for (unsigned int i = 0; i < ph.get_nrinterior(); ++i) {
                        const PolygonSimple& poly(ph[i]);
                        std::cerr << "  Interior: " << i << std::endl;
                        for (PolygonSimple::const_iterator pi(poly.begin()), pe(poly.end()); pi != pe; ++pi)
                                std::cerr << "    " << pi->get_lat_str() << ", " << pi->get_lon_str() << std::endl;
                }
        }
        switch (mapel.get_typecode()) {
                default:
                case MapelementsDb::Mapelement::maptyp_invalid:
                        break;

                case MapelementsDb::Mapelement::maptyp_railway:
                        if (!(draw_get_drawflags() & drawflags_terrain))
                                break;
                        cr->save();
                        cr->set_source_rgb(0.0, 0.0, 0.0);
                        cr->set_line_width(2.0);
                        polypath(cr, mapel.get_polygon(), false);
                        cr->stroke();
                        cr->restore();
                        break;

                case MapelementsDb::Mapelement::maptyp_railway_d:
                        if (!(draw_get_drawflags() & drawflags_terrain))
                                break;
                        cr->save();
                        cr->set_source_rgb(0.0, 0.0, 0.0);
                        cr->set_line_width(3.0);
                        polypath(cr, mapel.get_polygon(), false);
                        cr->stroke();
                        cr->restore();
                        break;

                case MapelementsDb::Mapelement::maptyp_aerial_cable:
                        if (!(draw_get_drawflags() & drawflags_terrain))
                                break;
                        cr->save();
                        cr->set_source_rgb(0.0, 0.0, 0.0);
                        cr->set_line_width(1.0);
                        polypath(cr, mapel.get_polygon(), false);
                        cr->stroke();
                        cr->restore();
                        break;

                case MapelementsDb::Mapelement::maptyp_highway:
                        if (!(draw_get_drawflags() & drawflags_terrain))
                                break;
                        cr->save();
                        cr->set_source_rgb(1.0, 0.0, 0.0);
                        cr->set_line_width(3.0);
                        polypath(cr, mapel.get_polygon(), false);
                        cr->stroke_preserve();
                        cr->set_source_rgb(1.0, 1.0, 1.0);
                        cr->set_line_width(1.0);
                        cr->stroke();
                        cr->restore();
                        break;

                case MapelementsDb::Mapelement::maptyp_road:
                        if (!(draw_get_drawflags() & drawflags_terrain))
                                break;
                        cr->save();
                        cr->set_source_rgb(1.0, 0.0, 0.0);
                        cr->set_line_width(2.0);
                        polypath(cr, mapel.get_polygon(), false);
                        cr->stroke();
                        cr->restore();
                        break;

                case MapelementsDb::Mapelement::maptyp_trail:
                        if (!(draw_get_drawflags() & drawflags_terrain))
                                break;
                        cr->save();
                        cr->set_source_rgb(1.0, 0.0, 0.0);
                        cr->set_line_width(1.0);
                        polypath(cr, mapel.get_polygon(), false);
                        cr->stroke();
                        cr->restore();
                        break;

                case MapelementsDb::Mapelement::maptyp_river:
                case MapelementsDb::Mapelement::maptyp_river_t:
                        if (!(draw_get_drawflags() & drawflags_terrain))
                                break;
                        cr->save();
                        cr->set_source_rgb(0.0, 0.0, 1.0);
                        cr->set_line_width(3.0);
                        polypath(cr, mapel.get_polygon(), false);
                        cr->stroke();
                        cr->restore();
                        break;

                case MapelementsDb::Mapelement::maptyp_canal:
                        if (!(draw_get_drawflags() & drawflags_terrain))
                                break;
                        cr->save();
                        cr->set_source_rgb(0.0, 0.0, 1.0);
                        cr->set_line_width(2.0);
                        polypath(cr, mapel.get_polygon(), false);
                        cr->stroke();
                        cr->restore();
                        break;

                case MapelementsDb::Mapelement::maptyp_lake:
                case MapelementsDb::Mapelement::maptyp_lake_t:
			if (!(draw_get_drawflags() & drawflags_terrain))
                                break;
			cr->save();
                        cr->set_source_rgb(0.0, 0.0, 1.0);
                        polypath(cr, mapel.get_polygon(), true);
                        cr->fill();
                        cr->restore();
                        break;

                case MapelementsDb::Mapelement::maptyp_forest:
                        if (!(draw_get_drawflags() & drawflags_terrain))
                                break;
                        cr->save();
                        cr->set_source_rgba(0.0, 1.0, 0.0, 0.5);
                        polypath(cr, mapel.get_polygon(), true);
                        cr->fill();
                        cr->restore();
                        break;

                case MapelementsDb::Mapelement::maptyp_pack_ice:
                case MapelementsDb::Mapelement::maptyp_glacier:
                        if (!(draw_get_drawflags() & drawflags_terrain))
                                break;
                        cr->save();
                        cr->set_source_rgba(1.0, 1.0, 1.0, 0.5);
                        polypath(cr, mapel.get_polygon(), true);
                        cr->fill();
                        cr->restore();
                        break;

                case MapelementsDb::Mapelement::maptyp_city:
                        if (!(draw_get_drawflags() & drawflags_terrain))
                                break;
                        cr->save();
                        cr->set_source_rgb(1.0, 1.0, 0.0);
                        polypath(cr, mapel.get_polygon(), true);
                        cr->fill();
                        cr->restore();
                        break;

                case MapelementsDb::Mapelement::maptyp_village:
                case MapelementsDb::Mapelement::maptyp_spot:
                case MapelementsDb::Mapelement::maptyp_landmark:
                        break;

                case MapelementsDb::Mapelement::maptyp_powerline:
                        if (!(draw_get_drawflags() & drawflags_terrain))
                                break;
                        cr->save();
                        cr->set_source_rgba(1.0, 0.0, 0.0, 0.5);
                        cr->set_line_width(2.0);
                        polypath(cr, mapel.get_polygon(), false);
                        cr->stroke();
                        cr->restore();
                        break;

                case MapelementsDb::Mapelement::maptyp_border:
                        if (!(draw_get_drawflags() & drawflags_terrain_borders))
                                break;
                        cr->save();
                        cr->set_source_rgb(0.5, 0.22, 0.0);
                        cr->set_line_width(2.0);
                        polypath(cr, mapel.get_polygon(), false);
                        cr->stroke();
                        cr->restore();
                        break;
        }
}

void VectorMapRenderer::DrawThreadVMap::draw_text(Cairo::Context *cr, const MapelementsDb::Mapelement& mapel)
{
        if (mapel.get_name().empty())
                return;
#if 0
        Rect bbox(mapel.get_bbox());
        ScreenCoord dim(draw_transform(bbox.get_northeast()) - draw_transform(bbox.get_southwest()));
        if (dim.getx() < 10 || dim.gety() < 10)
        return;
#endif
        draw_label(cr, mapel.get_name(), mapel.get_labelcoord(), NavaidsDb::Navaid::label_center);
}

bool VectorMapRenderer::DrawThreadVMap::mapel_render_comp::operator()(const MapelementsDb::Mapelement& m1, const MapelementsDb::Mapelement& m2) const
{
        if (m1.get_typecode() == MapelementsDb::Mapelement::maptyp_invalid && m2.get_typecode() != MapelementsDb::Mapelement::maptyp_invalid)
                return false;
        if (m1.get_typecode() != MapelementsDb::Mapelement::maptyp_invalid && m2.get_typecode() == MapelementsDb::Mapelement::maptyp_invalid)
                return true;
        if (m1.get_typecode() == MapelementsDb::Mapelement::maptyp_pack_ice && m2.get_typecode() != MapelementsDb::Mapelement::maptyp_pack_ice)
                return true;
        if (m1.get_typecode() != MapelementsDb::Mapelement::maptyp_pack_ice && m2.get_typecode() == MapelementsDb::Mapelement::maptyp_pack_ice)
                return false;
        if (m1.get_typecode() == MapelementsDb::Mapelement::maptyp_glacier && m2.get_typecode() != MapelementsDb::Mapelement::maptyp_glacier)
                return true;
        if (m1.get_typecode() != MapelementsDb::Mapelement::maptyp_glacier && m2.get_typecode() == MapelementsDb::Mapelement::maptyp_glacier)
                return false;
        if (m1.get_typecode() == MapelementsDb::Mapelement::maptyp_forest && m2.get_typecode() != MapelementsDb::Mapelement::maptyp_forest)
                return true;
        if (m1.get_typecode() != MapelementsDb::Mapelement::maptyp_forest && m2.get_typecode() == MapelementsDb::Mapelement::maptyp_forest)
                return false;
        if ((m1.get_typecode() == MapelementsDb::Mapelement::maptyp_lake || m1.get_typecode() == MapelementsDb::Mapelement::maptyp_lake_t) &&
             (m2.get_typecode() != MapelementsDb::Mapelement::maptyp_lake && m2.get_typecode() != MapelementsDb::Mapelement::maptyp_lake_t))
                return true;
        if ((m1.get_typecode() != MapelementsDb::Mapelement::maptyp_lake && m1.get_typecode() != MapelementsDb::Mapelement::maptyp_lake_t) &&
             (m2.get_typecode() == MapelementsDb::Mapelement::maptyp_lake || m2.get_typecode() == MapelementsDb::Mapelement::maptyp_lake_t))
                return false;
        if (m1.get_typecode() == MapelementsDb::Mapelement::maptyp_city && m2.get_typecode() != MapelementsDb::Mapelement::maptyp_city)
                return true;
        if (m1.get_typecode() != MapelementsDb::Mapelement::maptyp_city && m2.get_typecode() == MapelementsDb::Mapelement::maptyp_city)
                return false;
        return false;
}

VectorMapRenderer::DrawThreadTerrain::DrawThreadTerrain(Engine& eng, Glib::Dispatcher& dispatch, const ScreenCoord& scrsize, const Point& center, int alt, uint16_t upangle, int64_t time)
        : DrawThread(eng, dispatch, scrsize, center, alt, upangle, time), m_drawstate(drawstate_done)
{
}

VectorMapRenderer::DrawThreadTerrain::~DrawThreadTerrain()
{
        async_cancel();
}

void VectorMapRenderer::DrawThreadTerrain::airspaces_changed(void)
{
}

void VectorMapRenderer::DrawThreadTerrain::airports_changed(void)
{
        Engine::AirportResult::cancel(m_airportquery);
        DrawThread::airports_changed();
}

void VectorMapRenderer::DrawThreadTerrain::navaids_changed(void)
{
        Engine::NavaidResult::cancel(m_navaidquery);
        DrawThread::navaids_changed();
}

void VectorMapRenderer::DrawThreadTerrain::waypoints_changed(void)
{
        Engine::WaypointResult::cancel(m_waypointquery);
        DrawThread::waypoints_changed();
}

void VectorMapRenderer::DrawThreadTerrain::airways_changed(void)
{
}

void VectorMapRenderer::DrawThreadTerrain::async_cancel(void)
{
        Engine::ElevationMapResult::cancel(m_topoquery);
        Engine::NavaidResult::cancel(m_navaidquery);
        Engine::WaypointResult::cancel(m_waypointquery);
        Engine::AirportResult::cancel(m_airportquery);
}

void VectorMapRenderer::DrawThreadTerrain::draw_restart(bool dbquery)
{
	m_drawstate = drawstate_topo;
	dbquery = dbquery
		|| (!m_topoquery || m_topoquery->is_error())
		|| ((draw_get_drawflags() & drawflags_navaids) && (!m_navaidquery || m_navaidquery->is_error()))
		|| ((draw_get_drawflags() & drawflags_waypoints) && (!m_waypointquery || m_waypointquery->is_error()))
		|| ((draw_get_drawflags() & drawflags_airports) && (!m_airportquery || m_airportquery->is_error()));
        if (dbquery) {
		recompute_dbrectangle();
		std::cerr << "bbox: " << get_dbrectangle() << " (center " << draw_get_imagecenter() << ')' << std::endl;
                async_cancel();
                m_topoquery = m_engine.async_elevation_map(get_dbrectangle());
                if (draw_get_drawflags() & drawflags_navaids)
                        m_navaidquery = m_engine.async_navaid_find_bbox(get_dbrectangle(), ~0, NavaidsDb::element_t::subtables_none);
                if (draw_get_drawflags() & drawflags_waypoints)
                        m_waypointquery = m_engine.async_waypoint_find_bbox(get_dbrectangle(), ~0, WaypointsDb::element_t::subtables_none);
                if (draw_get_drawflags() & drawflags_airports)
                        m_airportquery = m_engine.async_airport_find_bbox(get_dbrectangle(), ~0, AirportsDb::element_t::subtables_none);
                if (m_topoquery)
                        m_topoquery->connect(sigc::mem_fun(*this, &VectorMapRenderer::DrawThreadTerrain::async_done));
                if (m_navaidquery)
                        m_navaidquery->connect(sigc::mem_fun(*this, &VectorMapRenderer::DrawThreadTerrain::async_done));
                if (m_waypointquery)
                        m_waypointquery->connect(sigc::mem_fun(*this, &VectorMapRenderer::DrawThreadTerrain::async_done));
                if (m_airportquery)
                        m_airportquery->connect(sigc::mem_fun(*this, &VectorMapRenderer::DrawThreadTerrain::async_done));
        }
        // start drawing
        Cairo::RefPtr<Cairo::Context> cr = draw_create_context();
        cr->rectangle(0, 0, draw_get_imagesize().getx(), draw_get_imagesize().gety());
        cr->set_source_rgb(1.0, 1.0, 1.0);
        cr->fill();
}

void VectorMapRenderer::DrawThreadTerrain::draw_iterate(void)
{
	Cairo::RefPtr<Cairo::Context> cr = draw_create_context();
        cr->rectangle(0, 0, draw_get_imagesize().getx(), draw_get_imagesize().gety());
	cr->clip();
        //cr->set_tolerance(1.0);

	switch (m_drawstate) {
	case drawstate_topo:
		// draw topography
		if (m_topoquery) {
			if (!m_topoquery->is_done())
				return;
			if (!m_topoquery->is_error()) {
				TimeMeasurement tm;
				unsigned int width(m_topoquery->get_width()), height(m_topoquery->get_height());
				Cairo::RefPtr<Cairo::ImageSurface> surface = Cairo::ImageSurface::create(/*Cairo::FORMAT_ARGB32*/Cairo::FORMAT_RGB24, width, height);
				if (surface) {
					int currentalt(draw_get_altitude());
					unsigned int stride(surface->get_stride());
					unsigned char *data(surface->get_data());
					std::cerr << "W " << width << " H " << height << " stride " << stride << std::endl;
					for (unsigned int y = 0; y < height; y++) {
						for (unsigned int x = 0; x < width; x++) {
							unsigned char *d = data + 4 * x + stride * y;
							TopoDb30::elev_t gndelev(m_topoquery.operator->()->operator()(x, y));
							//std::cerr << "x " << x << " y " << y << " gndelev " << gndelev << " alt " << currentalt << std::endl;
							d[3] = 0;
							if (gndelev == TopoDb30::nodata) {
								d[2] = 64; // red
								d[1] = 0; // green
								d[0] = 0; // blue
								continue;
							}
							if (gndelev == TopoDb30::ocean)
								gndelev = 0;
							int elev = currentalt - TopoDb30::m_to_ft(gndelev);
							//std::cerr << "x " << x << " y " << y << " gndelev " << gndelev << " alt " << currentalt << " elev " << elev << std::endl;
							if (elev <= 0) {
								d[2] = 255; // red
								d[1] = 0; // green
								d[0] = 0; // blue
							} else if (elev < 200) {
								d[2] = 255; // red
								d[1] = 96; // green
								d[0] = 0; // blue
							} else if (elev < 500) {
								d[2] = 255; // red
								d[1] = 192; // green
								d[0] = 0; // blue
							} else if (elev < 1000) {
								d[2] = 255; // red
								d[1] = 255; // green
								d[0] = 0; // blue
							} else if (elev < 2000) {
								d[2] = 0; // red
								d[1] = 255; // green
								d[0] = 0; // blue
							} else {
								d[2] = 0; // red
								d[1] = 0; // green
								d[0] = 0; // blue
							}
						}
					}
					std::cerr << "topo: draw time " << tm << std::endl;
					tm.start();
					draw_glidearea(surface);
					draw_surface(cr.operator->(), m_topoquery->get_bbox(), surface, 1);
					std::cerr << "glide area: draw time " << tm << std::endl;
				}
			}
		}
		m_drawstate = drawstate_navaids;
		// fall through

	case drawstate_navaids:
		if ((draw_get_drawflags() & drawflags_navaids) && m_navaidquery) {
			if (!m_navaidquery->is_done())
				return;
			if (!m_navaidquery->is_error()) {
				TimeMeasurement tm;
				draw(cr.operator->(), m_navaidquery->get_result());
				std::cerr << "navaids: " << m_navaidquery->get_result().size() << " drawing time " << tm << std::endl;
			}
       		}
		m_drawstate = drawstate_waypoints;
		// fall through

	case drawstate_waypoints:
		if ((draw_get_drawflags() & drawflags_waypoints) && m_waypointquery) {
			if (!m_waypointquery->is_done())
				return;
			if (!m_waypointquery->is_error()) {
				TimeMeasurement tm;
				draw(cr.operator->(), m_waypointquery->get_result());
				std::cerr << "waypoints: " << m_waypointquery->get_result().size() << " drawing time " << tm << std::endl;
			}
		}
		m_drawstate = drawstate_airports;
		// fall through

	case drawstate_airports:
		if ((draw_get_drawflags() & drawflags_airports) && m_airportquery) {
			if (!m_airportquery->is_done())
				return;
			if (!m_airportquery->is_error()) {
				TimeMeasurement tm;
				draw(cr.operator->(), m_airportquery->get_result());
				std::cerr << "airports: " << m_airportquery->get_result().size() << " drawing time " << tm << std::endl;
			}
		}
		m_drawstate = drawstate_done;
		// fall through

	case drawstate_done:
	default:
		draw_done();
		break;
	}
}

void VectorMapRenderer::DrawThreadTerrain::draw(Cairo::Context *cr, const std::vector<NavaidsDb::Navaid>& navaids)
{
        cr->save();
        cr->set_source_rgb(0.0, 1.0, 1.0);
        for (std::vector<NavaidsDb::Navaid>::const_iterator i1(navaids.begin()), ie(navaids.end()); i1 != ie && !draw_checkabort(); i1++) {
                const NavaidsDb::Navaid& n(*i1);
                NavaidsDb::Navaid::navaid_type_t t(n.get_navaid_type());
		{
			bool ok(false);
			ok = ok || (draw_get_drawflags() & drawflags_navaids_ndb && NavaidsDb::Navaid::has_ndb(t));
			ok = ok || (draw_get_drawflags() & drawflags_navaids_vor && NavaidsDb::Navaid::has_vor(t));
			ok = ok || (draw_get_drawflags() & drawflags_navaids_dme && NavaidsDb::Navaid::has_dme(t));
			if (!ok)
				continue;
                }
                draw_waypoint(cr, n.get_coord(), n.get_icao(), false, n.get_label_placement(), false);
        }
        cr->restore();
}

void VectorMapRenderer::DrawThreadTerrain::draw(Cairo::Context *cr, const std::vector<AirportsDb::Airport>& airports)
{
        cr->save();
        cr->set_source_rgb(0.0, 1.0, 1.0);
        for (std::vector<AirportsDb::Airport>::const_iterator i1(airports.begin()), ie(airports.end()); i1 != ie && !draw_checkabort(); i1++) {
                const AirportsDb::Airport& a(*i1);
                switch (a.get_typecode()) {
                        case 'A':
                                if (!(draw_get_drawflags() & drawflags_airports_civ))
                                        continue;
                                break;

                        case 'C':
                                if (!(draw_get_drawflags() & drawflags_airports_mil))
                                        continue;
				break;

                        default:
                                break;
                }
                draw_waypoint(cr, a.get_coord(), a.get_icao(), true, a.get_label_placement(), false);
        }
        cr->restore();
}

void VectorMapRenderer::DrawThreadTerrain::draw(Cairo::Context *cr, const std::vector<WaypointsDb::Waypoint>& waypoints)
{
        cr->save();
        cr->set_source_rgb(0.0, 1.0, 1.0);
        for (std::vector<WaypointsDb::Waypoint>::const_iterator i1(waypoints.begin()), ie(waypoints.end()); i1 != ie && !draw_checkabort(); i1++) {
                const WaypointsDb::Waypoint& w(*i1);
                switch (w.get_usage()) {
                        case WaypointsDb::Waypoint::usage_highlevel:
                                if (!(draw_get_drawflags() & drawflags_waypoints_high))
                                        continue;
                                break;

                        case WaypointsDb::Waypoint::usage_lowlevel:
                                if (!(draw_get_drawflags() & drawflags_waypoints_low))
                                        continue;
                                break;

                        case WaypointsDb::Waypoint::usage_bothlevel:
                                if (!(draw_get_drawflags() & (drawflags_waypoints_low | drawflags_waypoints_high)))
                                        continue;
                                break;

                        case WaypointsDb::Waypoint::usage_rnav:
                                if (!(draw_get_drawflags() & drawflags_waypoints_rnav))
                                        continue;
                                break;

                        case WaypointsDb::Waypoint::usage_terminal:
                                if (!(draw_get_drawflags() & drawflags_waypoints_terminal))
                                        continue;
                                break;

                        case WaypointsDb::Waypoint::usage_vfr:
                                if (!(draw_get_drawflags() & drawflags_waypoints_vfr))
                                        continue;
                                break;

                        default:
                        case WaypointsDb::Waypoint::usage_user:
                                if (!(draw_get_drawflags() & drawflags_waypoints_user))
                                        continue;
                                break;
                }
                draw_waypoint(cr, w.get_coord(), w.get_name(), false, w.get_label_placement(), false);
        }
        cr->restore();
}

namespace {

#ifdef HAVE_EIGEN3

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

#ifdef __WIN32__
__attribute__((force_align_arg_pointer))
#endif
MapRenderer::GlideModel::poly_t calculate_altloss(const MapRenderer::GlideModel& gm, const Point& pt0, const Point& pt1, Wind<double>& wind, double& maxalt)
{
	static constexpr int32_t maxpalt = 60000;
	static constexpr int32_t altinc = 500;
	static constexpr unsigned int polyorder = 5;

	unsigned int mp(maxalt / altinc + 1);
	Eigen::MatrixXd m(mp, polyorder);
	Eigen::VectorXd v(mp);
	int32_t ma(std::numeric_limits<int32_t>::min());
	mp = 0;
	for (int32_t alt = 0; alt <= maxpalt; alt += altinc, ++mp) {
		double alt1(gm.compute_newalt(wind, pt0, pt1, alt));
		if (std::isnan(alt1))
			break;
		double a1(1);
		for (unsigned int i = 0; i < polyorder; ++i, a1 *= alt)
			m(mp, i) = a1 * Point::ft_to_m_dbl;
		v(mp) = alt1 * Point::ft_to_m_dbl;
		ma = alt;
	}
	maxalt = std::min(maxalt, (double)ma);
	if (!mp)
		return MapRenderer::GlideModel::poly_t();
	m.conservativeResize(mp, polyorder);
	v.conservativeResize(mp);
	Eigen::VectorXd x(least_squares_solution(m, v));
        return MapRenderer::GlideModel::poly_t(x.data(), x.data() + x.size());
}

#else

MapRenderer::GlideModel::poly_t calculate_altloss(const GlideModel& gm, const Point& pt0, const Point& pt1, Wind<double>& wind, double& maxalt) const
{
	maxalt = std::min(maxalt, 0.0);
	return MapRenderer::GlideModel::poly_t();
}

#endif

class WQElement {
public:
	WQElement(int x, int y, float alt) : m_x(x), m_y(y), m_alt(alt) {}
	int get_x(void) const { return m_x; }
	int get_y(void) const { return m_y; }
	float get_alt(void) const { return m_alt; }
	int compare(const WQElement& x) const {
		if (get_alt() < x.get_alt())
			return 1;
		if (x.get_alt() < get_alt())
			return -1;
		if (get_x() < x.get_x())
			return -1;
		if (x.get_x() < get_x())
			return 1;
		if (get_y() < x.get_y())
			return -1;
		if (x.get_y() < get_y())
			return 1;
		return 0;
	}
	bool operator==(const WQElement& x) const { return compare(x) == 0; }
	bool operator!=(const WQElement& x) const { return compare(x) != 0; }
	bool operator<(const WQElement& x) const { return compare(x) < 0; }
	bool operator<=(const WQElement& x) const { return compare(x) <= 0; }
	bool operator>(const WQElement& x) const { return compare(x) > 0; }
	bool operator>=(const WQElement& x) const { return compare(x) >= 0; }

protected:
	int m_x;
	int m_y;
	float m_alt;
};

};

void VectorMapRenderer::DrawThreadTerrain::draw_glidearea(const Cairo::RefPtr<Cairo::ImageSurface> surface)
{
	static const Point dirs[8] = {
		Point(0, -1),
		Point(1, -1),
		Point(1, 0),
		Point(1, 1),
		Point(0, 1),
		Point(-1, 1),
		Point(-1, 0),
		Point(-1, -1)
	};
	GlideModel::poly_t altloss[8]; // in meter!
	double maxalt(std::numeric_limits<double>::max());

	const GlideModel& glidemodel(draw_get_glidemodel());
	if (glidemodel.is_invalid() || !surface || !m_topoquery)
		return;
	int width(surface->get_width()), height(surface->get_height());
	if (width <= 0 || height <= 0)
		return;
	typedef std::set<WQElement> workqueue_t;
	workqueue_t workqueue;
	std::vector<float> alts(width * height, std::numeric_limits<float>::min());
	{
		const Rect& bbox(m_topoquery->get_bbox());
		Point ptdiff(bbox.get_southeast() - bbox.get_northwest());
		ptdiff.set_lon(ptdiff.get_lon() / width);
		ptdiff.set_lat(ptdiff.get_lat() / height);
		{
			Point pt(bbox.get_northwest().halfway(bbox.get_southeast()));
			if (false)
				std::cerr << "draw_glidearea: bbox " << bbox << " pt " << pt << " ptdiff " << ptdiff << std::endl;
			Wind<double> wind;
			wind.set_wind(draw_get_winddir(), draw_get_windspeed());
			for (unsigned int i = 0; i < sizeof(dirs)/sizeof(dirs[0]); ++i) {
				Point pt1(ptdiff.get_lon() * dirs[i].get_lon(), ptdiff.get_lat() * dirs[i].get_lat());
				pt1 += pt;
				altloss[i] = calculate_altloss(glidemodel, pt, pt1, wind, maxalt);
				if (false)
					altloss[i].print(std::cerr << "draw_glidearea: dir " << i << " (" << dirs[i].get_lon() << ','
							 << dirs[i].get_lat() << ") altloss ") << ' ' << pt << "->" << pt1 << std::endl;
			}
		}
		{
			Point pt(draw_get_center() - bbox.get_northwest());
			pt.set_lon(pt.get_lon() / ptdiff.get_lon());
			pt.set_lat(pt.get_lat() / ptdiff.get_lat());
			WQElement wqe(pt.get_lon(), pt.get_lat(), draw_get_altitude() * Point::ft_to_m);
			unsigned int altidx = wqe.get_x() * height + wqe.get_y();
			alts[altidx] = wqe.get_alt();
			workqueue.insert(wqe);
			if (false)
				std::cerr << "draw_glidearea: start x " << wqe.get_x() << " y " << wqe.get_y()
					  << " alt " << wqe.get_alt() << " w " << width << " h " << height << std::endl;
		}
	}
	unsigned int stride(surface->get_stride());
	unsigned char *data(surface->get_data());
	constexpr TopoDb30::elev_t elevnil(std::numeric_limits<TopoDb30::elev_t>::min() + 2);
	std::vector<TopoDb30::elev_t> gndelev(width * height, elevnil);
	std::vector<bool> visited(width * height, false);
	while (!workqueue.empty()) {
		WQElement wqe(*workqueue.begin());
		workqueue.erase(workqueue.begin());
		if (wqe.get_x() < 0 || wqe.get_y() < 0 || wqe.get_x() >= width || wqe.get_y() >= height || std::isnan(wqe.get_alt()))
			continue;
		unsigned int altidx = wqe.get_x() * height + wqe.get_y();
		TopoDb30::elev_t& gndelevc(gndelev[altidx]);
		if (gndelevc == elevnil) {
		        gndelevc = m_topoquery.operator->()->operator()(wqe.get_x(), wqe.get_y());
			if (gndelevc == TopoDb30::ocean)
				gndelevc = 0;
		}
		if (gndelevc == TopoDb30::nodata)
			continue;
		if (wqe.get_alt() <= gndelevc)
			continue;
		if (!visited[altidx]) {
			unsigned char *d = data + 4 * wqe.get_x() + stride * wqe.get_y();
			d[2] = d[2] - (d[2] >> 1) + 127;
			d[1] = d[1] - (d[1] >> 1) + 127;
			d[0] = d[0] - (d[0] >> 1) + 127;
			visited[altidx] = true;
		}
		for (unsigned int i = 0; i < sizeof(dirs)/sizeof(dirs[0]); ++i) {
			WQElement wqen(wqe.get_x() + dirs[i].get_lon(), wqe.get_y() + dirs[i].get_lat(), altloss[i].eval(wqe.get_alt()));
			if (std::isnan(wqen.get_alt()))
				continue;
			unsigned int altidxn = wqen.get_x() * height + wqen.get_y();
			float& altsn(alts[altidxn]);
			if (wqen.get_alt() <= altsn)
				continue;
			workqueue_t::iterator it(workqueue.find(WQElement(wqen.get_x(), wqen.get_y(), altsn)));
			if (it != workqueue.end())
				workqueue.erase(it);
			altsn = wqen.get_alt();
			workqueue.insert(wqen);
		}
	}
}

VectorMapRenderer::DrawThreadAirportDiagram::DrawThreadAirportDiagram(Engine & eng, Glib::Dispatcher& dispatch, const ScreenCoord & scrsize, const Point & center, int alt, uint16_t upangle, int64_t time)
        : DrawThread(eng, dispatch, scrsize, center, alt, upangle, time), m_drawstate(drawstate_done)
{
}

VectorMapRenderer::DrawThreadAirportDiagram::~DrawThreadAirportDiagram()
{
        async_cancel();
}

void VectorMapRenderer::DrawThreadAirportDiagram::airspaces_changed(void)
{
}

void VectorMapRenderer::DrawThreadAirportDiagram::airports_changed(void)
{
        Engine::AirportResult::cancel(m_airportquery);
	DrawThread::airports_changed();
}

void VectorMapRenderer::DrawThreadAirportDiagram::navaids_changed(void)
{
}

void VectorMapRenderer::DrawThreadAirportDiagram::waypoints_changed(void)
{
}

void VectorMapRenderer::DrawThreadAirportDiagram::airways_changed(void)
{
}

void VectorMapRenderer::DrawThreadAirportDiagram::draw_restart(bool dbquery)
{
	m_drawstate = drawstate_airports;
	dbquery = dbquery
		|| (!m_airportquery || m_airportquery->is_error());
        if (dbquery) {
		recompute_dbrectangle();
		std::cerr << "bbox: " << get_dbrectangle() << " (center " << draw_get_imagecenter() << ')' << std::endl;
                async_cancel();
                m_airportquery = m_engine.async_airport_find_bbox(get_dbrectangle(), ~0, AirportsDb::element_t::subtables_all);
                m_navaidquery = m_engine.async_navaid_find_bbox(get_dbrectangle(), ~0, NavaidsDb::element_t::subtables_all);
                m_waypointquery = m_engine.async_waypoint_find_bbox(get_dbrectangle(), ~0, WaypointsDb::element_t::subtables_all);
                if (m_airportquery)
                        m_airportquery->connect(sigc::mem_fun(*this, &VectorMapRenderer::DrawThreadAirportDiagram::async_done));
                if (m_navaidquery)
                        m_navaidquery->connect(sigc::mem_fun(*this, &VectorMapRenderer::DrawThreadAirportDiagram::async_done));
                if (m_waypointquery)
                        m_waypointquery->connect(sigc::mem_fun(*this, &VectorMapRenderer::DrawThreadAirportDiagram::async_done));
        }
        // start drawing
        Cairo::RefPtr<Cairo::Context> cr = draw_create_context();
        cr->rectangle(0, 0, draw_get_imagesize().getx(), draw_get_imagesize().gety());
        cr->set_source_rgb(1.0, 1.0, 1.0);
        cr->fill();
}

void VectorMapRenderer::DrawThreadAirportDiagram::draw_iterate(void)
{
        Cairo::RefPtr<Cairo::Context> cr = draw_create_context();
        cr->rectangle(0, 0, draw_get_imagesize().getx(), draw_get_imagesize().gety());
        cr->set_source_rgb(1.0, 1.0, 1.0);
        cr->clip();
        //cr->set_tolerance(1.0);

	switch (m_drawstate) {
	case drawstate_airports:
                if (m_airportquery) {
			if (!m_airportquery->is_done())
				return;
			if (!m_airportquery->is_error()) {
				TimeMeasurement tm;
				const AirportsDb::elementvector_t& ev(m_airportquery->get_result());
				for (AirportsDb::elementvector_t::const_iterator ei(ev.begin()), ee(ev.end()); ei != ee && !draw_checkabort(); ++ei)
					draw(cr.operator->(), *ei);
				std::cerr << "airports: " << m_airportquery->get_result().size() << ": drawing time " << tm << std::endl;
			}
      		}
		m_drawstate = drawstate_navaids;
		// fall through

	case drawstate_navaids:
		if (m_navaidquery) {
			if (!m_navaidquery->is_done())
				return;
			if (!m_navaidquery->is_error()) {
				TimeMeasurement tm;
				const NavaidsDb::elementvector_t& ev(m_navaidquery->get_result());
				for (NavaidsDb::elementvector_t::const_iterator ei(ev.begin()), ee(ev.end()); ei != ee && !draw_checkabort(); ++ei)
					draw(cr.operator->(), *ei);
				std::cerr << "navaids: " << m_navaidquery->get_result().size() << " drawing time " << tm << std::endl;
			}
     		}
		m_drawstate = drawstate_waypoints;
		// fall through

	case drawstate_waypoints:
		if (m_waypointquery) {
			if (!m_waypointquery->is_done())
				return;
			if (!m_waypointquery->is_error()) {
				TimeMeasurement tm;
				const WaypointsDb::elementvector_t& ev(m_waypointquery->get_result());
				for (WaypointsDb::elementvector_t::const_iterator ei(ev.begin()), ee(ev.end()); ei != ee && !draw_checkabort(); ++ei)
					draw(cr.operator->(), *ei);
				std::cerr << "waypoints: " << m_waypointquery->get_result().size() << " drawing time " << tm << std::endl;
			}
		}
		m_drawstate = drawstate_done;
		// fall through

	case drawstate_done:
	default:
		draw_done();
		break;
	}
}

void VectorMapRenderer::DrawThreadAirportDiagram::async_cancel(void)
{
        Engine::AirportResult::cancel(m_airportquery);
        Engine::NavaidResult::cancel(m_navaidquery);
        Engine::WaypointResult::cancel(m_waypointquery);
}

void VectorMapRenderer::DrawThreadAirportDiagram::polyline_to_path(Cairo::Context *cr, const AirportsDb::Airport::Polyline& pl)
{
        if (pl.empty())
                return;
        const AirportsDb::Airport::PolylineNode *pnfirst = 0;
        ScreenCoordFloat sc0, sc1, scfirst;
        for (AirportsDb::Airport::Polyline::const_iterator ni(pl.begin()), ni0, ne(pl.end()); ni != ne; ni0 = ni, sc0 = sc1, ++ni) {
                const AirportsDb::Airport::PolylineNode& pn1(*ni);
                sc1 = draw_transform(pn1.get_point());
                if (!pnfirst) {
                        pnfirst = &pn1;
                        scfirst = sc1;
                        cr->move_to(sc1.getx(), sc1.gety());
                        continue;
                }
                const AirportsDb::Airport::PolylineNode& pn0(*ni0);
                if (pn0.is_bezier_next() || pn1.is_bezier_prev()) {
                        ScreenCoordFloat scc0(sc0);
                        if (pn0.is_bezier_next())
                                scc0 = draw_transform(pn0.get_next_controlpoint());
                        ScreenCoordFloat scc1(sc1);
                        if (pn1.is_bezier_prev())
                                scc1 = draw_transform(pn1.get_prev_controlpoint());
                        cr->curve_to(scc0.getx(), scc0.gety(), scc1.getx(), scc1.gety(), sc1.getx(), sc1.gety());
                } else {
                        cr->line_to(sc1.getx(), sc1.gety());
                }
                if (pn1.is_closepath()) {
                        if (pn0.is_bezier_next() || pn1.is_bezier_prev()) {
                                ScreenCoordFloat scc0(sc1);
                                if (pn1.is_bezier_next())
                                        scc0 = draw_transform(pn1.get_next_controlpoint());
                                ScreenCoordFloat scc1(scfirst);
                                if (pnfirst->is_bezier_prev())
                                        scc1 = draw_transform(pnfirst->get_prev_controlpoint());
                                cr->curve_to(scc0.getx(), scc0.gety(), scc1.getx(), scc1.gety(), scfirst.getx(), scfirst.gety());
                        } else {
                                cr->line_to(scfirst.getx(), scfirst.gety());
                        }
                        cr->close_path();
                }
                if (pn1.is_closepath() || pn1.is_endpath()) {
                        pnfirst = 0;
                }
        }
}

void VectorMapRenderer::DrawThreadAirportDiagram::draw(Cairo::Context *cr, const AirportsDb::Airport& arpt)
{
        float featurescale(nmi_to_pixel(Point::km_to_nmi * 0.002));
        cr->save();
        // draw line features
        cr->set_line_width(1.0);
        cr->set_fill_rule(Cairo::FILL_RULE_WINDING);
        // first pass: area features
        for (unsigned int r = 0; r < arpt.get_nrlinefeatures(); ++r) {
                const AirportsDb::Airport::Polyline& pl(arpt.get_linefeature(r));
                if (!pl.is_area())
                        continue;
                polyline_to_path(cr, pl);
                if (pl.is_concrete())
                        cr->set_source_rgba(192.0/256.0, 192.0/256.0, 192.0/256.0, 1.0);
                else
                        cr->set_source_rgba(0.0/256.0, 150.0/256.0, 0.0/256.0, 1.0);
                cr->fill_preserve();
                if (pl.is_airportboundary()) {
                        cr->set_source_rgb(0.0, 0.0, 0.0);
                        cr->stroke_preserve();
                }
                cr->begin_new_path();
        }
        cr->set_line_width(1.0);
        for (unsigned int r = 0; r < arpt.get_nrrwy(); ++r) {
                const AirportsDb::Airport::Runway& rwy(arpt.get_rwy(r));
                float halfwidth(rwy.get_width() * (0.5f * Point::ft_to_m * (1.0f/1000.f) * Point::km_to_nmi));
                Point rept0(rwy.get_le_coord().spheric_course_distance_nmi((rwy.get_le_hdg() - 0x4000) * FPlanLeg::to_deg, halfwidth));
                Point rept1(rwy.get_le_coord().spheric_course_distance_nmi((rwy.get_le_hdg() + 0x4000) * FPlanLeg::to_deg, halfwidth));
                Point rept2(rwy.get_he_coord().spheric_course_distance_nmi((rwy.get_he_hdg() - 0x4000) * FPlanLeg::to_deg, halfwidth));
                Point rept3(rwy.get_he_coord().spheric_course_distance_nmi((rwy.get_he_hdg() + 0x4000) * FPlanLeg::to_deg, halfwidth));
                ScreenCoordFloat repx0(draw_transform(rept0));
                ScreenCoordFloat repx1(draw_transform(rept1));
                ScreenCoordFloat repx2(draw_transform(rept2));
                ScreenCoordFloat repx3(draw_transform(rept3));
                if (rwy.is_concrete())
                        cr->set_source_rgba(192.0/256.0, 192.0/256.0, 192.0/256.0, 1.0);
                else
                        cr->set_source_rgba(0.0/256.0, 150.0/256.0, 0.0/256.0, 1.0);
                cr->move_to(repx0.getx(), repx0.gety());
                cr->line_to(repx1.getx(), repx1.gety());
                cr->line_to(repx2.getx(), repx2.gety());
                cr->line_to(repx3.getx(), repx3.gety());
                cr->line_to(repx0.getx(), repx0.gety());
                cr->fill();
                float rwyhalfwidth(nmi_to_pixel(rwy.get_width() * (Point::ft_to_m / 2000.0f * Point::km_to_nmi)));
                // LE headend
                {
                        cr->save();
                        {
                                Point ptcenter(rwy.get_le_coord());
                                if (rwy.get_le_disp())
                                        ptcenter = ptcenter.spheric_course_distance_nmi(rwy.get_le_hdg() * FPlanLeg::to_deg,
                                                                                        rwy.get_le_disp() * (Point::ft_to_m * (1.0f/1000.f) * Point::km_to_nmi));
                                ScreenCoordFloat pxcenter(draw_transform(ptcenter));
                                cr->translate(pxcenter.getx(), pxcenter.gety());
                        }
                        cr->rotate((rwy.get_le_hdg() + 0x8000 - draw_get_upangle()) * FPlanLeg::to_rad);
                        //std::cerr << "scale: " << rwyhalfwidth << " width(nmi): " << (rwy.get_width() * (Point::ft_to_m / 2000.0f * Point::km_to_nmi)) << std::endl;
                        cr->scale(rwyhalfwidth, rwyhalfwidth);
                        cr->set_line_width(0.2);
                        cr->set_source_rgb(0.0, 0.0, 0.0);
                        cr->move_to(-1.0, 0.0);
                        cr->line_to(1.0, 0.0);
                        cr->stroke();
                        if (rwy.is_le_usable()) {
                                for (unsigned int i = 0; i < 5; ++i) {
                                        cr->move_to((1.0/11.0) * (1 + 2 * i), 0.2);
                                        cr->rel_line_to((1.0/11.0), 0.0);
                                        cr->rel_line_to(0.0, 1.0);
                                        cr->rel_line_to(-(1.0/11.0), 0.0);
                                        cr->close_path();
                                        cr->fill();
                                        cr->move_to(-(1.0/11.0) * (1 + 2 * i), 0.2);
                                        cr->rel_line_to(-(1.0/11.0), 0.0);
                                        cr->rel_line_to(0.0, 1.0);
                                        cr->rel_line_to((1.0/11.0), 0.0);
                                        cr->close_path();
                                        cr->fill();
                                }
                        } else {
                                for (unsigned int i = 0; i < 4; ++i) {
                                        cr->move_to(-1.0 + 0.5 * i, 0.2);
                                        cr->rel_line_to(0.5, 0.5);
                                        cr->rel_move_to(-0.5, 0.0);
                                        cr->rel_line_to(0.5, -0.5);
                                        cr->stroke();
                                }
                        }
                        if (rwy.get_le_disp()) {
                                float rwyend(-nmi_to_pixel(rwy.get_le_disp() * (Point::ft_to_m / 1000.0f * Point::km_to_nmi)) / rwyhalfwidth);
                                cr->save();
                                cr->move_to(-1.0, rwyend);
                                cr->rel_line_to(2.0, 0.0);
                                cr->line_to(1.0, 0.0);
                                cr->rel_line_to(-2.0, 0.0);
                                cr->close_path();
                                cr->clip();
                                for (float curp = -0.4; curp >= rwyend; curp -= 1.4) {
                                        cr->move_to(-0.8, curp-1.6);
                                        cr->rel_line_to(0.8, 1.6);
                                        cr->rel_line_to(0.8, -1.6);
                                        cr->stroke();
                                }
                                cr->restore();
                        }
                        if (!rwy.get_ident_le().empty()) {
                                cr->select_font_face("Sans", Cairo::FONT_SLANT_NORMAL, Cairo::FONT_WEIGHT_NORMAL);
                                cr->set_font_size(1);
                                Cairo::TextExtents ext;
                                cr->get_text_extents(rwy.get_ident_le(), ext);
                                if (ext.width > 0.0) {
                                        cr->set_font_size(1.8 / ext.width);
                                        cr->save();
                                        cr->translate(0.9, 1.5);
                                        cr->rotate(M_PI);
                                        cr->move_to(0.0, 0.0);
                                        cr->show_text(rwy.get_ident_le());
                                        cr->restore();
                                }
                        }
                        cr->restore();
                }
                // HE headend
                {
                        cr->save();
                        {
                                Point ptcenter(rwy.get_he_coord());
                                if (rwy.get_he_disp())
                                        ptcenter = ptcenter.spheric_course_distance_nmi(rwy.get_he_hdg() * FPlanLeg::to_deg,
                                                                                        rwy.get_he_disp() * (Point::ft_to_m * (1.0f/1000.f) * Point::km_to_nmi));
                                ScreenCoordFloat pxcenter(draw_transform(ptcenter));
                                cr->translate(pxcenter.getx(), pxcenter.gety());
                        }
                        cr->rotate((rwy.get_he_hdg() + 0x8000 - draw_get_upangle()) * FPlanLeg::to_rad);
                        //std::cerr << "scale: " << rwyhalfwidth << " width(nmi): " << (rwy.get_width() * (Point::ft_to_m / 2000.0f * Point::km_to_nmi)) << std::endl;
                        cr->scale(rwyhalfwidth, rwyhalfwidth);
                        cr->set_line_width(0.2);
                        cr->set_source_rgb(0.0, 0.0, 0.0);
                        cr->move_to(-1.0, 0.0);
                        cr->line_to(1.0, 0.0);
                        cr->stroke();
                        if (rwy.is_he_usable()) {
                                for (unsigned int i = 0; i < 5; ++i) {
                                        cr->move_to((1.0/11.0) * (1 + 2 * i), 0.2);
                                        cr->rel_line_to((1.0/11.0), 0.0);
                                        cr->rel_line_to(0.0, 1.0);
                                        cr->rel_line_to(-(1.0/11.0), 0.0);
                                        cr->close_path();
                                        cr->fill();
                                        cr->move_to(-(1.0/11.0) * (1 + 2 * i), 0.2);
                                        cr->rel_line_to(-(1.0/11.0), 0.0);
                                        cr->rel_line_to(0.0, 1.0);
                                        cr->rel_line_to((1.0/11.0), 0.0);
                                        cr->close_path();
                                        cr->fill();
                                }
                        } else {
                                for (unsigned int i = 0; i < 4; ++i) {
                                        cr->move_to(-1.0 + 0.5 * i, 0.2);
                                        cr->rel_line_to(0.5, 0.5);
                                        cr->rel_move_to(-0.5, 0.0);
                                        cr->rel_line_to(0.5, -0.5);
                                        cr->stroke();
                                }
                        }
                        if (rwy.get_he_disp()) {
                                float rwyend(-nmi_to_pixel(rwy.get_he_disp() * (Point::ft_to_m / 1000.0f * Point::km_to_nmi)) / rwyhalfwidth);
                                cr->save();
                                cr->move_to(-1.0, rwyend);
                                cr->rel_line_to(2.0, 0.0);
                                cr->line_to(1.0, 0.0);
                                cr->rel_line_to(-2.0, 0.0);
                                cr->close_path();
                                cr->clip();
                                for (float curp = -0.4; curp >= rwyend; curp -= 1.4) {
                                        cr->move_to(-0.8, curp-1.6);
                                        cr->rel_line_to(0.8, 1.6);
                                        cr->rel_line_to(0.8, -1.6);
                                        cr->stroke();
                                }
                                cr->restore();
                        }
                        if (!rwy.get_ident_he().empty()) {
                                cr->select_font_face("Sans", Cairo::FONT_SLANT_NORMAL, Cairo::FONT_WEIGHT_NORMAL);
                                cr->set_font_size(1);
                                Cairo::TextExtents ext;
                                cr->get_text_extents(rwy.get_ident_he(), ext);
                                if (ext.width > 0.0) {
                                        cr->set_font_size(1.8 / ext.width);
                                        cr->save();
                                        cr->translate(0.9, 1.5);
                                        cr->rotate(M_PI);
                                        cr->move_to(0.0, 0.0);
                                        cr->show_text(rwy.get_ident_he());
                                        cr->restore();
                                }
                        }
                        cr->restore();
                }
                // draw centerline
                {
                        float halfwidth(rwy.get_width() * (0.5f * Point::ft_to_m * (1.0f/1000.f) * Point::km_to_nmi));
                        Point pt1(rwy.get_le_coord().spheric_course_distance_nmi(rwy.get_le_hdg() * FPlanLeg::to_deg,
                                  (rwy.get_le_disp() + 2 * rwy.get_width()) * (Point::ft_to_m * (1.0f/1000.f) * Point::km_to_nmi)));
                        Point pt2(rwy.get_he_coord().spheric_course_distance_nmi(rwy.get_he_hdg() * FPlanLeg::to_deg,
                                  (rwy.get_he_disp() + 2 * rwy.get_width()) * (Point::ft_to_m * (1.0f/1000.f) * Point::km_to_nmi)));
                        ScreenCoordFloat px1(draw_transform(pt1));
                        ScreenCoordFloat px2(draw_transform(pt2));
                        cr->set_line_width(0.05 * rwyhalfwidth);
                        std::vector<double> dashes;
                        dashes.push_back(rwyhalfwidth);
                        cr->set_dash(dashes, 0.0);
                        cr->set_source_rgb(0.0, 0.0, 0.0);
                        cr->move_to(px1.getx(), px1.gety());
                        cr->line_to(px2.getx(), px2.gety());
                        cr->stroke();
                        dashes.clear();
                        cr->set_dash(dashes, 0.0);
                }
        }
        // draw line features
        cr->set_line_width(1.0);
        // second pass: line features
        for (unsigned int r = 0; r < arpt.get_nrlinefeatures(); ++r) {
                const AirportsDb::Airport::Polyline& pl(arpt.get_linefeature(r));
                if (pl.is_area())
                        continue;
                if (pl.is_airportboundary()) {
                        polyline_to_path(cr, pl);
                        cr->set_source_rgb(0.0, 0.0, 0.0);
                        cr->stroke();
                }
        }
        // draw heliports
        cr->set_line_width(1.0);
        cr->set_source_rgb(0.0, 0.0, 0.0);
        for (unsigned int r = 0; r < arpt.get_nrhelipads(); ++r) {
                const AirportsDb::Airport::Helipad& hp(arpt.get_helipad(r));
                ScreenCoordFloat px(draw_transform(hp.get_coord()));
                double r1(nmi_to_pixel(hp.get_width() * (Point::ft_to_m * 0.0005f * Point::km_to_nmi)));
                double r2(nmi_to_pixel(hp.get_length() * (Point::ft_to_m * 0.0005f * Point::km_to_nmi)));
                cr->save();
                cr->translate(px.getx(), px.gety());
                cr->scale(r1, r2);
                cr->begin_new_path();
                cr->arc(0.0, 0.0, 1.0, 0.0, 2.0 * M_PI);
                if (hp.is_concrete())
                        cr->set_source_rgba(192.0/256.0, 192.0/256.0, 192.0/256.0, 1.0);
                else
                        cr->set_source_rgba(0.0/256.0, 150.0/256.0, 0.0/256.0, 1.0);
                cr->fill_preserve();
                cr->set_source_rgb(0.0, 0.0, 0.0);
                cr->set_line_width(0.1);
                cr->stroke();
                cr->restore();
                cr->select_font_face("Sans", Cairo::FONT_SLANT_NORMAL, Cairo::FONT_WEIGHT_NORMAL);
                cr->set_font_size(100.0);
                Cairo::TextExtents ext;
                cr->get_text_extents(hp.get_ident(), ext);
                double sc = std::min(r1 / ext.width, r2 / ext.height);
                //std::cerr << "Helipad: Font: r1 " << r1 << " r2 " << r2 << " W " << ext.width << " H " << ext.height << " sc " << sc << std::endl;
                cr->set_font_size(sc * 100.0);
                cr->get_text_extents(hp.get_ident(), ext);
                cr->move_to(px.getx() - 0.5 * ext.width, px.gety() + 0.5 * ext.height);
                cr->show_text(hp.get_ident());
        }
        // draw ARP
        {
                cr->save();
                ScreenCoordFloat px(draw_transform(arpt.get_coord()));
                cr->translate(px.getx(), px.gety());
                cr->scale(featurescale, featurescale);
                cr->set_line_width(1.0);
                cr->set_source_rgb(0.0, 0.0, 0.0);
                cr->move_to(-10, 0);
                cr->line_to(+10, 0);
                cr->move_to(0, -10);
                cr->line_to(0, +10);
                cr->stroke();
                cr->arc(0, 0, 5.0, 0.0, 2.0 * M_PI);
                cr->stroke();
                cr->restore();
        }
        // draw point features
        cr->set_line_width(1.0);
        cr->select_font_face("Sans", Cairo::FONT_SLANT_NORMAL, Cairo::FONT_WEIGHT_NORMAL);
        cr->set_font_size(12);
        for (unsigned int r = 0; r < arpt.get_nrpointfeatures(); ++r) {
                const AirportsDb::Airport::PointFeature& pf(arpt.get_pointfeature(r));
                switch (pf.get_feature()) {
                        case AirportsDb::Airport::PointFeature::feature_windsock:
                        {
                                ScreenCoordFloat px(draw_transform(pf.get_coord()));
                                cr->save();
                                cr->set_source_rgb(0.0, 0.0, 0.0);
                                cr->set_line_width(0.25);
                                cr->translate(px.getx(), px.gety());
                                cr->scale(featurescale, featurescale);
                                cr->move_to(0.0, -1.0);
                                cr->line_to(-2.0, 0.0);
                                cr->line_to(2.0, 0.0);
                                cr->close_path();
                                cr->move_to(0.0, -10.0);
                                cr->line_to(4.0, -11.0);
                                cr->line_to(4.0, -19.0);
                                cr->line_to(0.0, -20.0);
                                cr->close_path();
                                cr->move_to(8.0, -12.0);
                                cr->line_to(12.0, -13.0);
                                cr->line_to(12.0, -17.0);
                                cr->line_to(8.0, -18.0);
                                cr->close_path();
                                cr->fill();
                                cr->move_to(0.0, 0.0);
                                cr->line_to(0.0, -20.0);
                                cr->line_to(12.0, -17.0);
                                cr->line_to(12.0, -13.0);
                                cr->line_to(0.0, -10.0);
                                cr->stroke();
                                cr->restore();
                                break;
                        }

                        case AirportsDb::Airport::PointFeature::feature_vasi:
                        {
                                uint16_t hdg(0);
                                //std::cerr << "VASI: RWY Ident " << pf.get_rwyident() << std::endl;
                                for (unsigned int r = 0; r < arpt.get_nrrwy(); ++r) {
                                        const AirportsDb::Airport::Runway& rwy(arpt.get_rwy(r));
                                        if (rwy.get_ident_he() == pf.get_rwyident()) {
                                                hdg = rwy.get_he_hdg();
                                                break;
                                        }
                                        if (rwy.get_ident_le() == pf.get_rwyident()) {
                                                hdg = rwy.get_le_hdg();
                                                break;
                                        }
                                }
                                ScreenCoordFloat px(draw_transform(pf.get_coord()));
                                cr->save();
                                cr->set_source_rgb(0.0, 0.0, 0.0);
                                cr->translate(px.getx(), px.gety());
                                cr->rotate((hdg - draw_get_upangle()) * FPlanLeg::to_rad);
                                cr->scale(featurescale, featurescale);
                                cr->arc(-9.0, 0.0, 2.0, 0.0, 2.0 * M_PI);
                                cr->begin_new_sub_path();
                                cr->arc(-3.0, 0.0, 2.0, 0.0, 2.0 * M_PI);
                                cr->begin_new_sub_path();
                                cr->arc(3.0, 0.0, 2.0, 0.0, 2.0 * M_PI);
                                cr->begin_new_sub_path();
                                cr->arc(9.0, 0.0, 2.0, 0.0, 2.0 * M_PI);
                                cr->fill();
                                cr->restore();
                                break;
                        }

                        case AirportsDb::Airport::PointFeature::feature_tower:
                        {
                                static const float starx[5] = {
                                        0.0000,
                                        -9.5106,
                                        -5.8779,
                                        5.8779,
                                        9.5106
                                };
                                static const float stary[5] = {
                                        10.0000,
                                        3.0902,
                                        -8.0902,
                                        -8.0902,
                                        3.0902
                                };
                                ScreenCoordFloat px(draw_transform(pf.get_coord()));
                                cr->save();
                                cr->set_source_rgb(0.0, 0.0, 0.0);
                                cr->set_line_width(1.0);
                                cr->translate(px.getx(), px.gety());
                                cr->rotate((pf.get_hdg() - draw_get_upangle()) * FPlanLeg::to_rad);
                                cr->scale(featurescale, featurescale);
                                cr->arc(0.0, 0.0, 10.0, 0.0, 2.0 * M_PI);
                                cr->close_path();
                                cr->move_to(starx[0], -stary[0]);
                                for (unsigned int i = 0;;) {
                                        i += 2;
                                        if (i >= 5)
                                                i -= 5;
                                        cr->line_to(starx[i], -stary[i]);
                                        if (!i)
                                                break;
                                }
                                cr->close_path();
                                cr->stroke();
                                cr->restore();
                                break;
                        }

                        case AirportsDb::Airport::PointFeature::feature_taxiwaysign:
                        {
                                ScreenCoordFloat px(draw_transform(pf.get_coord()));
                                cr->save();
                                cr->set_source_rgb(0.0, 0.0, 0.0);
                                cr->translate(px.getx(), px.gety());
                                cr->rotate((pf.get_hdg() - draw_get_upangle()) * FPlanLeg::to_rad);
                                float sc(ldexpf(featurescale, -1));
                                cr->scale(sc, sc);
                                draw_taxiwaysign(cr, pf.get_name());
                                cr->restore();
                                break;
                        }

                        case AirportsDb::Airport::PointFeature::feature_startuploc:
                        {
                                ScreenCoordFloat px(draw_transform(pf.get_coord()));
                                cr->save();
                                cr->set_source_rgb(0.0, 0.0, 0.0);
                                cr->translate(px.getx(), px.gety());
                                cr->rotate((pf.get_hdg() - draw_get_upangle()) * FPlanLeg::to_rad);
                                float sc(ldexpf(featurescale, -1));
                                cr->scale(sc, sc);
                                draw_label(cr, pf.get_name(), ScreenCoordFloat(0, 0), AirportsDb::Airport::label_center, 0, 0);
                                cr->restore();
                                break;
                        }

                        default:
                                break;
                }
        }
        draw_airport_vfrreppt(cr, arpt);
        cr->restore();
}

void VectorMapRenderer::DrawThreadAirportDiagram::draw(Cairo::Context *cr, const NavaidsDb::Navaid& n)
{
        cr->save();
        cr->set_source_rgb(0.0, 0.0, 0.0);
        cr->set_line_width(1.0);
        cr->set_fill_rule(Cairo::FILL_RULE_EVEN_ODD);
        ScreenCoordFloat pos(draw_transform(n.get_coord()));
        cr->translate(pos.getx(), pos.gety());
        float featurescale(nmi_to_pixel(Point::km_to_nmi * 0.002));
        cr->scale(featurescale, featurescale);
        draw_navaid(cr, n, ScreenCoordFloat());
        cr->restore();
}

void VectorMapRenderer::DrawThreadAirportDiagram::draw(Cairo::Context *cr, const WaypointsDb::Waypoint& w)
{
        draw_waypoint(cr, w.get_coord(), w.get_name(), true, w.get_label_placement());
}

void VectorMapRenderer::DrawThreadAirportDiagram::draw_taxiwaysign(Cairo::Context *cr, const Glib::ustring& text)
{
        std::vector<LabelPart> lbl;
        lbl.push_back(LabelPart());
        for (Glib::ustring::size_type i = 0; i < text.size();) {
                {
                        Glib::ustring::value_type ch(text[i]);
                        if (ch != '{') {
                                lbl.back().push_back(ch);
                                ++i;
                                continue;
                        }
                }
                ++i;
                // in glyph mode, find glyphs separated by comma or terminated by right curly brace
                while (i < text.size()) {
                        Glib::ustring::size_type j = text.find_first_of(",}", i);
                        Glib::ustring glyph(text, i, (j == Glib::ustring::npos) ? j : (j - i));
                        i = j;
                        if (glyph == "@Y") {
                                // (black lettering on yellow - the most common taxiway sign).
                                if (!lbl.back().empty())
                                        lbl.push_back(LabelPart());
                                lbl.back().set_fg(0.0, 0.0, 0.0);
                                lbl.back().set_bg(1.0, 1.0, 0.0);
                        } else if (glyph == "@R") {
                                // (white lettering on a black background - usually a mandatory sign, such as warning of a runway crossing).
                                if (!lbl.back().empty())
                                        lbl.push_back(LabelPart());
                                lbl.back().set_fg(1.0, 1.0, 1.0);
                                lbl.back().set_bg(0.0, 0.0, 0.0);
                        } else if (glyph == "@L") {
                                // (yellow lettering on a black background - informs you of your current location).
                                if (!lbl.back().empty())
                                        lbl.push_back(LabelPart());
                                lbl.back().set_fg(1.0, 1.0, 0.0);
                                lbl.back().set_bg(0.0, 0.0, 0.0);
                        } else if (glyph == "@B") {
                                // (white lettering on a black background - used for runway 'distance-remaining' signs).
                                if (!lbl.back().empty())
                                        lbl.push_back(LabelPart());
                                lbl.back().set_fg(1.0, 1.0, 1.0);
                                lbl.back().set_bg(0.0, 0.0, 0.0);
                        } else if (glyph == "@@") {
                                // backside - just ignore
                        } else if (glyph == "_") {
                                lbl.back().push_back(' ');
                        } else if (glyph == "*") {
                                lbl.back().push_back('.');
                        } else if (glyph == "|") {
                                // frame
                                lbl.back().push_back('|');
                        } else if (glyph == "^u") {
                                // up arrow
                                lbl.back().push_back((gunichar)0x2191);
                        } else if (glyph == "^d") {
                                // down arrow
                                lbl.back().push_back((gunichar)0x2193);
                        } else if (glyph == "^l") {
                                // left arrow
                                lbl.back().push_back((gunichar)0x2190);
                        } else if (glyph == "^r") {
                                // right arrow
                                lbl.back().push_back((gunichar)0x2192);
                        } else if (glyph == "^lu") {
                                lbl.back().push_back((gunichar)0x2196);
                        } else if (glyph == "^ru") {
                                lbl.back().push_back((gunichar)0x2197);
                        } else if (glyph == "^ld") {
                                lbl.back().push_back((gunichar)0x2199);
                        } else if (glyph == "^rd") {
                                lbl.back().push_back((gunichar)0x2198);
                        } else if (glyph == "no-entry") {
                                lbl.back().append("no entry");
                        } else {
                                lbl.back().append(glyph);
                        }
                        if (i == Glib::ustring::npos || i >= text.size())
                                break;
                        if (text[i] == '}') {
                                ++i;
                                break;
                        }
                        ++i;
                }
        }
        double toth(0), totw(0);
        for (std::vector<LabelPart>::const_iterator li(lbl.begin()), le(lbl.end()); li != le; ++li) {
                Cairo::TextExtents ext;
                cr->get_text_extents(*li, ext);
                totw += ext.width + 1.0;
                toth = std::max(toth, ext.height);
        }
        double posw(-0.5 * totw), posh(-0.5 * toth);
        for (std::vector<LabelPart>::const_iterator li(lbl.begin()), le(lbl.end()); li != le; ++li) {
                Cairo::TextExtents ext;
                cr->get_text_extents(*li, ext);
                li->set_bg(cr);
                cr->rectangle(posw, posh - 1.0, ext.width + 1.0, toth + 2.0);
                cr->fill();
                li->set_fg(cr);
                cr->move_to(posw, -posh);
                cr->show_text(*li);
                posw += ext.width + 1.0;
        }
}

const VectorMapRenderer::DrawThreadTMS::AIRACDates VectorMapRenderer::DrawThreadTMS::airacdates[] = {
	{ 1389225600, "1401" },
	{ 1391644800, "1402" },
	{ 1394064000, "1403" },
	{ 1396483200, "1404" },
	{ 1398902400, "1405" },
	{ 1401321600, "1406" },
	{ 1403740800, "1407" },
	{ 1406160000, "1408" },
	{ 1408579200, "1409" },
	{ 1410998400, "1410" },
	{ 1413417600, "1411" },
	{ 1415836800, "1412" },
	{ 1418256000, "1413" },
	{ 1420675200, "1501" },
	{ 1423094400, "1502" },
	{ 1425513600, "1503" },
	{ 1427932800, "1504" },
	{ 1430352000, "1505" },
	{ 1432771200, "1506" },
	{ 1435190400, "1507" },
	{ 1437609600, "1508" },
	{ 1440028800, "1509" },
	{ 1442448000, "1510" },
	{ 1444867200, "1511" },
	{ 1447286400, "1512" },
	{ 1449705600, "1513" },
	{ 1452124800, "1601" },
	{ 1454544000, "1602" },
	{ 1456963200, "1603" },
	{ 1459382400, "1604" },
	{ 1461801600, "1605" },
	{ 1464220800, "1606" },
	{ 1466640000, "1607" },
	{ 1469059200, "1608" },
	{ 1471478400, "1609" },
	{ 1473897600, "1610" },
	{ 1476316800, "1611" },
	{ 1478736000, "1612" },
	{ 1481155200, "1613" },
	{ 1483574400, "1701" },
	{ 1485993600, "1702" },
	{ 1488412800, "1703" },
	{ 1490832000, "1704" },
	{ 1493251200, "1705" },
	{ 1495670400, "1706" },
	{ 1498089600, "1707" },
	{ 1500508800, "1708" },
	{ 1502928000, "1709" },
	{ 1505347200, "1710" },
	{ 1507766400, "1711" },
	{ 1510185600, "1712" },
	{ 1512604800, "1713" },
	{ 1515024000, "1801" },
	{ 1517443200, "1802" },
	{ 1519862400, "1803" },
	{ 1522281600, "1804" },
	{ 1524700800, "1805" },
	{ 1527120000, "1806" },
	{ 1529539200, "1807" },
	{ 1531958400, "1808" },
	{ 1534377600, "1809" },
	{ 1536796800, "1810" },
	{ 1539216000, "1811" },
	{ 1541635200, "1812" },
	{ 1544054400, "1813" },
	{ 1546473600, "1901" },
	{ 1548892800, "1902" },
	{ 1551312000, "1903" },
	{ 1553731200, "1904" },
	{ 1556150400, "1905" },
	{ 1558569600, "1906" },
	{ 1560988800, "1907" },
	{ 1563408000, "1908" },
	{ 1565827200, "1909" },
	{ 1568246400, "1910" },
	{ 1570665600, "1911" },
	{ 1573084800, "1912" },
	{ 1575504000, "1913" }
};

const VectorMapRenderer::DrawThreadTMS::TileSource VectorMapRenderer::DrawThreadTMS::tilesources[] = {
	{ 1, 18, TileSource::imgfmt_png, TileIndex::mode_tms, "OSM", "OpenStreetMap", "http://tile.openstreetmap.org/#Z/#X/#Y.png" },
	{ 1, 18, TileSource::imgfmt_png, TileIndex::mode_tms, "OCM", "OpenCycleMap", "http://b.tile.opencyclemap.org/cycle/#Z/#X/#Y.png" },
	{ 1, 18, TileSource::imgfmt_png, TileIndex::mode_tms, "PT", "Public Transport", "http://tile.xn--pnvkarte-m4a.de/tilegen/#Z/#X/#Y.png" },
	{ 1, 15, TileSource::imgfmt_png, TileIndex::mode_tms, "TR", "OSMC Trails", "http://topo.geofabrik.de/trails/#Z/#X/#Y.png" },
	{ 1, 11, TileSource::imgfmt_jpg, TileIndex::mode_tms, "MFF", "Maps-For-Free", "http://maps-for-free.com/layer/relief/z#Z/row#Y/#Z_#X-#Y.jpg" },
	{ 1, 17, TileSource::imgfmt_jpg, TileIndex::mode_tms, "GM", "Google Maps", "http://mt#R.google.com/vt/lyrs=m@146&hl=en&x=#X&s=&y=#Y&z=#Z" },
	{ 1, 18, TileSource::imgfmt_jpg, TileIndex::mode_tms, "GS", "Google Satellite", "http://khm#R.google.com/kh/v=128&src=app&x=#X&y=#Y&z=#Z" },
	{ 1, 17, TileSource::imgfmt_jpg, TileIndex::mode_tms, "GH", "Google Hybrid", "http://mt#R.google.com/vt/lyrs=h@146&hl=en&x=#X&s=&y=#Y&z=#Z" },
	{ 1, 17, TileSource::imgfmt_jpg, TileIndex::mode_tms, "VEM", "Virtual Earth", "http://a#R.ortho.tiles.virtualearth.net/tiles/r#W.jpeg?g=50" },
	{ 1, 17, TileSource::imgfmt_jpg, TileIndex::mode_tms, "VES", "Virtual Earth Satellite", "http://a#R.ortho.tiles.virtualearth.net/tiles/a#W.jpeg?g=50" },
	{ 1, 17, TileSource::imgfmt_jpg, TileIndex::mode_tms, "VEH", "Virtual Earth Hybrid", "http://a#R.ortho.tiles.virtualearth.net/tiles/h#W.jpeg?g=50" },
	{ 0, 23, TileSource::imgfmt_jpg, TileIndex::mode_svsm_vfr, "SVV", "Skyvector World VFR", "http://t#T.skyvector.net/tiles/301/#A/#Z/#X/#Y.jpg" },
	{ 0, 23, TileSource::imgfmt_jpg, TileIndex::mode_svsm_ifrl, "SVIL", "Skyvector World IFR Low", "http://t#T.skyvector.net/tiles/302/#A/#Z/#X/#Y.jpg" },
	{ 2, 23, TileSource::imgfmt_jpg, TileIndex::mode_svsm_ifrh, "SVIH", "Skyvector World IFR High", "http://t#T.skyvector.net/tiles/304/#A/#Z/#X/#Y.jpg" },
	{ 3, 11, TileSource::imgfmt_png, TileIndex::mode_tms, "CB", "Cartabossy", "http://carte.f-aero.fr/fr-v5/#Z/#X/#Y.png" },
	{ 3, 11, TileSource::imgfmt_png, TileIndex::mode_tms, "CBW", "Cartabossy Weekend", "http://carte.f-aero.fr/wk-v5/#Z/#X/#Y.png" },
	{ 3, 11, TileSource::imgfmt_png, TileIndex::mode_tms, "IGN", "IGN", "http://carte.f-aero.fr/8png/#Z/#X/#Y.png" },
	{ 3, 11, TileSource::imgfmt_png, TileIndex::mode_tms, "OAI", "OpenAIP", "http://1.tile.maps.openaip.net/geowebcache/service/tms/1.0.01.0.0/openaip_basemap@EPSG%3A900913@png/#Z/#X/#Y.png" },
	{ 14, 20, TileSource::imgfmt_png, TileIndex::mode_tmsch1903, "LSI", "ICAO LS", "http://wmts9.geo.admin.ch/1.0.0/ch.bazl.luftfahrtkarten-icao/default/20150305/21781/#Z/#Y/#X.png" },
	{ 14, 20, TileSource::imgfmt_png, TileIndex::mode_tmsch1903, "LSG", "Glider LS", "http://wmts6.geo.admin.ch/1.0.0/ch.bazl.segelflugkarte/default/20150305/21781/#Z/#Y/#X.png" },
	{ 14, 28, TileSource::imgfmt_png, TileIndex::mode_tmsch1903, "LSO", "Obstacle LS", "http://wmts7.geo.admin.ch/1.0.0/ch.swisstopo.pixelkarte-farbe/default/20151231/21781/#Z/#Y/#X.png" },
	{ 1, 10, TileSource::imgfmt_png, TileIndex::mode_tms, "EDI", "ICAO ED", "https://secais.dfs.de/static-maps/ICAO500-2014-DACH-Reprojected_01/tiles/#Z/#X/#Y.png" },
	{ 1, 8, TileSource::imgfmt_png, TileIndex::mode_tms, "EDV", "VFR ED", "https://secais.dfs.de/static-maps/vfr_20131114/tiles/#Z/#X/#Y.png" }
};

constexpr double VectorMapRenderer::DrawThreadTMS::TileIndex::sv_wgs_a;
constexpr double VectorMapRenderer::DrawThreadTMS::TileIndex::sv_x_0;
constexpr double VectorMapRenderer::DrawThreadTMS::TileIndex::sv_y_0;
constexpr double VectorMapRenderer::DrawThreadTMS::TileIndex::sv_xr_vfr;
constexpr double VectorMapRenderer::DrawThreadTMS::TileIndex::sv_yr_vfr;
constexpr double VectorMapRenderer::DrawThreadTMS::TileIndex::sv_xr_ifrl;
constexpr double VectorMapRenderer::DrawThreadTMS::TileIndex::sv_yr_ifrl;
constexpr double VectorMapRenderer::DrawThreadTMS::TileIndex::sv_xr_ifrh;
constexpr double VectorMapRenderer::DrawThreadTMS::TileIndex::sv_yr_ifrh;
const unsigned int VectorMapRenderer::DrawThreadTMS::TileIndex::sv_tilesize;
const unsigned int VectorMapRenderer::DrawThreadTMS::TileIndex::sv_chartwidth_vfr;
const unsigned int VectorMapRenderer::DrawThreadTMS::TileIndex::sv_chartheight_vfr;
const unsigned int VectorMapRenderer::DrawThreadTMS::TileIndex::sv_chartwidth_ifrl;
const unsigned int VectorMapRenderer::DrawThreadTMS::TileIndex::sv_chartheight_ifrl;
const unsigned int VectorMapRenderer::DrawThreadTMS::TileIndex::sv_chartwidth_ifrh;
const unsigned int VectorMapRenderer::DrawThreadTMS::TileIndex::sv_chartheight_ifrh;
constexpr double VectorMapRenderer::DrawThreadTMS::TileIndex::ch1903_origin_x;
constexpr double VectorMapRenderer::DrawThreadTMS::TileIndex::ch1903_origin_y;
const unsigned int VectorMapRenderer::DrawThreadTMS::TileIndex::ch1903_tilesize;

const VectorMapRenderer::DrawThreadTMS::TileIndex::TileLayerInfo VectorMapRenderer::DrawThreadTMS::TileIndex::ch1903_layerinfo[29] = {
	{ 1, 1, 4000 },
	{ 1, 1, 3750 },
	{ 1, 1, 3500 },
	{ 1, 1, 3250 },
	{ 1, 1, 3000 },
	{ 1, 1, 2750 },
	{ 1, 1, 2500 },
	{ 1, 1, 2250 },
	{ 1, 1, 2000 },
	{ 2, 1, 1750 },
	{ 2, 1, 1500 },
	{ 2, 1, 1250 },
	{ 2, 2, 1000 },
	{ 3, 2, 750 },
	{ 3, 2, 650 },
	{ 4, 3, 500 },
	{ 8, 5, 250 },
	{ 19, 13, 100 },
	{ 38, 25, 50 },
	{ 94, 63, 20 },
	{ 188, 125, 10 },
	{ 375, 250, 5 },
	{ 750, 500, 2.5 },
	{ 938, 625, 2 },
	{ 1250, 834, 1.5 },
	{ 1875, 1250, 1 },
	{ 3750, 2500, 0.5 },
	{ 7500, 5000, 0.25 },
	{ 18750, 12500, 0.1 }
};

VectorMapRenderer::DrawThreadTMS::TileIndex::TileIndex(mode_t mode, unsigned int zoom, unsigned int x, unsigned int y)
	: m_z(zoom), m_x(x), m_y(y), m_mode(mode)
{
	unsigned int mask(get_mask());
	m_x &= mask;
	m_y = std::min(m_y, std::max(mask, 1U) - 1U);
}

VectorMapRenderer::DrawThreadTMS::TileIndex::TileIndex(mode_t mode, unsigned int zoom, const Point& pt)
	: m_z(zoom), m_x(0), m_y(0), m_mode(mode)
{
	set_rounddown(pt);
}

double VectorMapRenderer::DrawThreadTMS::TileIndex::get_sv_scale_vfr(unsigned int z)
{
	if (!z)
		return 0.5;
	--z;
	return ldexp((z & 1) ? 4.0 / 3.0 : 1.0, (z >> 1));
}

unsigned int VectorMapRenderer::DrawThreadTMS::TileIndex::get_sv_xsize_vfr(unsigned int z)
{
	return Point::round<unsigned int,double>(ceil(sv_chartwidth_vfr / (get_sv_scale_vfr(z) * sv_tilesize)));
}

unsigned int VectorMapRenderer::DrawThreadTMS::TileIndex::get_sv_ysize_vfr(unsigned int z)
{
	return Point::round<unsigned int,double>(ceil(sv_chartheight_vfr / (get_sv_scale_vfr(z) * sv_tilesize)));
}

double VectorMapRenderer::DrawThreadTMS::TileIndex::get_sv_scale_ifr(unsigned int z)
{
	if (!z)
		return 0.5;
	--z;
	return ldexp((z & 1) ? 1.5 : 1.0, (z >> 1));
}

unsigned int VectorMapRenderer::DrawThreadTMS::TileIndex::get_sv_xsize_ifrl(unsigned int z)
{
	return Point::round<unsigned int,double>(ceil(sv_chartwidth_ifrl / (get_sv_scale_ifr(z) * sv_tilesize)));
}

unsigned int VectorMapRenderer::DrawThreadTMS::TileIndex::get_sv_ysize_ifrl(unsigned int z)
{
	return Point::round<unsigned int,double>(ceil(sv_chartheight_ifrl / (get_sv_scale_ifr(z) * sv_tilesize)));
}

unsigned int VectorMapRenderer::DrawThreadTMS::TileIndex::get_sv_xsize_ifrh(unsigned int z)
{
	return Point::round<unsigned int,double>(ceil(sv_chartwidth_ifrh / (get_sv_scale_ifr(z) * sv_tilesize)));
}

unsigned int VectorMapRenderer::DrawThreadTMS::TileIndex::get_sv_ysize_ifrh(unsigned int z)
{
	return Point::round<unsigned int,double>(ceil(sv_chartheight_ifrh / (get_sv_scale_ifr(z) * sv_tilesize)));
}

const VectorMapRenderer::DrawThreadTMS::TileIndex::TileLayerInfo& VectorMapRenderer::DrawThreadTMS::TileIndex::get_ch1903_info(unsigned int z)
{
	return ch1903_layerinfo[std::min(z, (unsigned int)(sizeof(ch1903_layerinfo)/sizeof(ch1903_layerinfo[0])-1U))];
}

unsigned int VectorMapRenderer::DrawThreadTMS::TileIndex::get_xsize(void) const
{
	switch (get_mode()) {
	case mode_svsm_vfr:
		return get_sv_xsize_vfr();

	case mode_svsm_ifrl:
		return get_sv_xsize_ifrl();

	case mode_svsm_ifrh:
		return get_sv_xsize_ifrh();

	case mode_tmsch1903:
	{
		const TileLayerInfo& info(get_ch1903_info());
		return info.width;
	}

	default:
		return get_mask() + 1U;
	}
}

unsigned int VectorMapRenderer::DrawThreadTMS::TileIndex::get_ysize(void) const
{
	switch (get_mode()) {
	case mode_svsm_vfr:
		return get_sv_ysize_vfr();

	case mode_svsm_ifrl:
		return get_sv_ysize_ifrl();

	case mode_svsm_ifrh:
		return get_sv_ysize_ifrh();

	case mode_tmsch1903:
	{
		const TileLayerInfo& info(get_ch1903_info());
		return info.height;
	}

	default:
		return get_mask() + 1U;
	}
}

void VectorMapRenderer::DrawThreadTMS::TileIndex::set_rounddown(const Point& pt)
{
	switch (get_mode()) {
	case mode_svsm_vfr:
	{
		Point pt1(pt);
		if (pt1.get_lat() > (Point::coord_t)(89.5 * Point::from_deg_dbl))
			pt1.set_lat((Point::coord_t)(89.5 * Point::from_deg_dbl));
		else if (pt1.get_lat() < (Point::coord_t)(-89.5 * Point::from_deg_dbl))
			pt1.set_lat((Point::coord_t)(-89.5 * Point::from_deg_dbl));
		double x(sv_wgs_a * pt1.get_lon_rad_dbl() - sv_x_0);
		double y(sv_wgs_a * log(tan(.25 * M_PI + .5 * pt1.get_lat_rad_dbl())) - sv_y_0);
		double sc(get_sv_scale_vfr());
		if (x < 0)
			x += sv_xr_vfr * sv_chartwidth_vfr;
		if (y > 0)
			y += sv_yr_vfr * sv_chartheight_vfr;
		x /= sv_xr_vfr * sv_tilesize * sc;
		y /= sv_yr_vfr * sv_tilesize * sc;
		m_x = Point::round<unsigned int,double>(floor(x));
		m_y = Point::round<unsigned int,double>(ceil(y));
		if (false)
			std::cerr << "set_rounddown: " << pt.get_lat_str2() << ' ' << pt.get_lon_str2()
				  << " x " << m_x << " y " << m_y << " sz " << get_sv_xsize_vfr() << ' ' << get_sv_ysize_vfr()
				  << " zoom " << get_zoom() << std::endl;
		m_x %= get_sv_xsize_vfr();
		m_y %= get_sv_ysize_vfr();
		break;
	}

	case mode_svsm_ifrl:
	{
		Point pt1(pt);
		if (pt1.get_lat() > (Point::coord_t)(89.5 * Point::from_deg_dbl))
			pt1.set_lat((Point::coord_t)(89.5 * Point::from_deg_dbl));
		else if (pt1.get_lat() < (Point::coord_t)(-89.5 * Point::from_deg_dbl))
			pt1.set_lat((Point::coord_t)(-89.5 * Point::from_deg_dbl));
		double x(sv_wgs_a * pt1.get_lon_rad_dbl() - sv_x_0);
		double y(sv_wgs_a * log(tan(.25 * M_PI + .5 * pt1.get_lat_rad_dbl())) - sv_y_0);
		double sc(get_sv_scale_ifr());
		if (x < 0)
			x += sv_xr_ifrl * sv_chartwidth_ifrl;
		if (y > 0)
			y += sv_yr_ifrl * sv_chartheight_ifrl;
		x /= sv_xr_ifrl * sv_tilesize * sc;
		y /= sv_yr_ifrl * sv_tilesize * sc;
		m_x = Point::round<unsigned int,double>(floor(x));
		m_y = Point::round<unsigned int,double>(ceil(y));
		if (false)
			std::cerr << "set_rounddown: " << pt.get_lat_str2() << ' ' << pt.get_lon_str2()
				  << " x " << m_x << " y " << m_y << " sz " << get_sv_xsize_ifrl() << ' ' << get_sv_ysize_ifrl()
				  << " zoom " << get_zoom() << std::endl;
		m_x %= get_sv_xsize_ifrl();
		m_y %= get_sv_ysize_ifrl();
		break;
	}

	case mode_svsm_ifrh:
	{
		Point pt1(pt);
		if (pt1.get_lat() > (Point::coord_t)(89.5 * Point::from_deg_dbl))
			pt1.set_lat((Point::coord_t)(89.5 * Point::from_deg_dbl));
		else if (pt1.get_lat() < (Point::coord_t)(-89.5 * Point::from_deg_dbl))
			pt1.set_lat((Point::coord_t)(-89.5 * Point::from_deg_dbl));
		double x(sv_wgs_a * pt1.get_lon_rad_dbl() - sv_x_0);
		double y(sv_wgs_a * log(tan(.25 * M_PI + .5 * pt1.get_lat_rad_dbl())) - sv_y_0);
		double sc(get_sv_scale_ifr());
		if (x < 0)
			x += sv_xr_ifrh * sv_chartwidth_ifrh;
		if (y > 0)
			y += sv_yr_ifrh * sv_chartheight_ifrh;
		x /= sv_xr_ifrh * sv_tilesize * sc;
		y /= sv_yr_ifrh * sv_tilesize * sc;
		m_x = Point::round<unsigned int,double>(floor(x));
		m_y = Point::round<unsigned int,double>(ceil(y));
		if (false)
			std::cerr << "set_rounddown: " << pt.get_lat_str2() << ' ' << pt.get_lon_str2()
				  << " x " << m_x << " y " << m_y << " sz " << get_sv_xsize_ifrh() << ' ' << get_sv_ysize_ifrh()
				  << " zoom " << get_zoom() << std::endl;
		m_x %= get_sv_xsize_ifrh();
		m_y %= get_sv_ysize_ifrh();
		break;
	}

	case mode_tmsch1903:
	{
		const TileLayerInfo& info(get_ch1903_info());
		double x, y;
		if (!pt.get_ch1903p(x, y)) {
			m_x = (y > 700000) ? info.width - 1 : 0;
			m_y = (x > 200000) ? info.height - 1 : 0;
			break;
		}
		x = ch1903_origin_x - x;
		y -= ch1903_origin_y;
		if (x < 0) {
			m_y = 0;
		} else {
			m_y = Point::round<unsigned int,double>(ceil(x / (info.resolution * ch1903_tilesize)));
			if (m_y >= info.height)
				m_y = info.height - 1;
		}
		if (y < 0) {
			m_x = 0;
		} else {
			m_x = Point::round<unsigned int,double>(floor(y / (info.resolution * ch1903_tilesize)));
			if (m_x >= info.width)
				m_x = info.width - 1;
		}
		break;
	}

	default:
	case mode_tms:
	{
		unsigned int mask(get_mask());
		unsigned int xsh(sizeof(Point::coord_t) * 8 - get_zoom());
		m_x = (pt.get_lon() ^ 0x80000000) >> xsh;
		m_x &= mask;
		double lat(pt.get_lat_rad_dbl());
		int y(ceil(ldexp(1.0 - log(tan(lat) + 1.0 / cos(lat)) * (1.0 / M_PI), get_zoom() - 1)));
		if (y < 0)
			y = 0;
		else if (y > mask)
			y = mask;
		m_y = y;
		break;
	}
	}
}

void VectorMapRenderer::DrawThreadTMS::TileIndex::set_roundup(const Point& pt)
{
	switch (get_mode()) {
	case mode_svsm_vfr:
	{
		Point pt1(pt);
		if (pt1.get_lat() > (Point::coord_t)(89.5 * Point::from_deg_dbl))
			pt1.set_lat((Point::coord_t)(89.5 * Point::from_deg_dbl));
		else if (pt1.get_lat() < (Point::coord_t)(-89.5 * Point::from_deg_dbl))
			pt1.set_lat((Point::coord_t)(-89.5 * Point::from_deg_dbl));
		double x(sv_wgs_a * pt1.get_lon_rad_dbl() - sv_x_0);
		double y(sv_wgs_a * log(tan(.25 * M_PI + .5 * pt1.get_lat_rad_dbl())) - sv_y_0);
		double sc(get_sv_scale_vfr());
		if (x < 0)
			x += sv_xr_vfr * sv_chartwidth_vfr;
		if (y > 0)
			y += sv_yr_vfr * sv_chartheight_vfr;
		x /= sv_xr_vfr * sv_tilesize * sc;
		y /= sv_yr_vfr * sv_tilesize * sc;
		m_x = Point::round<unsigned int,double>(ceil(x));
		m_y = Point::round<unsigned int,double>(floor(y));
		if (false)
			std::cerr << "set_roundup: " << pt.get_lat_str2() << ' ' << pt.get_lon_str2()
				  << " x " << m_x << " y " << m_y << " sz " << get_sv_xsize_vfr() << ' ' << get_sv_ysize_vfr()
				  << " zoom " << get_zoom() << std::endl;
		m_x %= get_sv_xsize_vfr();
		m_y %= get_sv_ysize_vfr();
		break;
	}

	case mode_svsm_ifrl:
	{
		Point pt1(pt);
		if (pt1.get_lat() > (Point::coord_t)(89.5 * Point::from_deg_dbl))
			pt1.set_lat((Point::coord_t)(89.5 * Point::from_deg_dbl));
		else if (pt1.get_lat() < (Point::coord_t)(-89.5 * Point::from_deg_dbl))
			pt1.set_lat((Point::coord_t)(-89.5 * Point::from_deg_dbl));
		double x(sv_wgs_a * pt1.get_lon_rad_dbl() - sv_x_0);
		double y(sv_wgs_a * log(tan(.25 * M_PI + .5 * pt1.get_lat_rad_dbl())) - sv_y_0);
		double sc(get_sv_scale_ifr());
		if (x < 0)
			x += sv_xr_ifrl * sv_chartwidth_ifrl;
		if (y > 0)
			y += sv_yr_ifrl * sv_chartheight_ifrl;
		x /= sv_xr_ifrl * sv_tilesize * sc;
		y /= sv_yr_ifrl * sv_tilesize * sc;
		m_x = Point::round<unsigned int,double>(ceil(x));
		m_y = Point::round<unsigned int,double>(floor(y));
		if (false)
			std::cerr << "set_roundup: " << pt.get_lat_str2() << ' ' << pt.get_lon_str2()
				  << " x " << m_x << " y " << m_y << " sz " << get_sv_xsize_ifrl() << ' ' << get_sv_ysize_ifrl()
				  << " zoom " << get_zoom() << std::endl;
		m_x %= get_sv_xsize_ifrl();
		m_y %= get_sv_ysize_ifrl();
		break;
	}

	case mode_svsm_ifrh:
	{
		Point pt1(pt);
		if (pt1.get_lat() > (Point::coord_t)(89.5 * Point::from_deg_dbl))
			pt1.set_lat((Point::coord_t)(89.5 * Point::from_deg_dbl));
		else if (pt1.get_lat() < (Point::coord_t)(-89.5 * Point::from_deg_dbl))
			pt1.set_lat((Point::coord_t)(-89.5 * Point::from_deg_dbl));
		double x(sv_wgs_a * pt1.get_lon_rad_dbl() - sv_x_0);
		double y(sv_wgs_a * log(tan(.25 * M_PI + .5 * pt1.get_lat_rad_dbl())) - sv_y_0);
		double sc(get_sv_scale_ifr());
		if (x < 0)
			x += sv_xr_ifrh * sv_chartwidth_ifrh;
		if (y > 0)
			y += sv_yr_ifrh * sv_chartheight_ifrh;
		x /= sv_xr_ifrh * sv_tilesize * sc;
		y /= sv_yr_ifrh * sv_tilesize * sc;
		m_x = Point::round<unsigned int,double>(ceil(x));
		m_y = Point::round<unsigned int,double>(floor(y));
		if (false)
			std::cerr << "set_roundup: " << pt.get_lat_str2() << ' ' << pt.get_lon_str2()
				  << " x " << m_x << " y " << m_y << " sz " << get_sv_xsize_ifrh() << ' ' << get_sv_ysize_ifrh()
				  << " zoom " << get_zoom() << std::endl;
		m_x %= get_sv_xsize_ifrh();
		m_y %= get_sv_ysize_ifrh();
		break;
	}

	case mode_tmsch1903:
	{
		const TileLayerInfo& info(get_ch1903_info());
		double x, y;
		if (!pt.get_ch1903p(x, y)) {
			m_x = (y > 700000) ? info.width - 1 : 0;
			m_y = (x > 200000) ? info.height - 1 : 0;
			break;
		}
		x = ch1903_origin_x - x;
		y -= ch1903_origin_y;
		if (x < 0) {
			m_y = 0;
		} else {
			m_y = Point::round<unsigned int,double>(floor(x / (info.resolution * ch1903_tilesize)));
			if (m_y >= info.height)
				m_y = info.height - 1;
		}
		if (y < 0) {
			m_x = 0;
		} else {
			m_x = Point::round<unsigned int,double>(ceil(y / (info.resolution * ch1903_tilesize)));
			if (m_x >= info.width)
				m_x = info.width - 1;
		}
		break;
	}

	default:
	case mode_tms:
	{
		unsigned int mask(get_mask());
		unsigned int xsh(sizeof(Point::coord_t) * 8 - get_zoom());
		m_x = ((pt.get_lon() ^ 0x80000000) + (1U << xsh) - 1U) >> xsh;
		m_x &= mask;
		double lat(pt.get_lat_rad_dbl());
		int y(floor(ldexp(1.0 - log(tan(lat) + 1.0 / cos(lat)) * (1.0 / M_PI), get_zoom() - 1)));
		if (y < 0)
			y = 0;
		else if (y > mask)
			y = mask;
		m_y = y;
		break;
	}
	}
}

unsigned int VectorMapRenderer::DrawThreadTMS::TileIndex::get_mask(void) const
{
	return (1U << get_zoom()) - 1U;
}

unsigned int VectorMapRenderer::DrawThreadTMS::TileIndex::get_invy(void) const
{
	return get_mask() - get_y();
}

VectorMapRenderer::DrawThreadTMS::TileIndex::operator Point() const
{
	switch (get_mode()) {
	case mode_svsm_vfr:
	{
		double sc(get_sv_scale_vfr());
		double x(get_x() * sv_xr_vfr * sv_tilesize * sc + sv_x_0);
		double y((get_y() + 1) * sv_yr_vfr * sv_tilesize * sc + sv_y_0);
		Point pt;
		pt.set_lon_rad_dbl(x / sv_wgs_a);
		pt.set_lat_rad_dbl(M_PI / 2. - 2 * atan(exp(-1 * y / sv_wgs_a)));
		return pt;
	}

	case mode_svsm_ifrl:
	{
		double sc(get_sv_scale_ifr());
		double x(get_x() * sv_xr_ifrl * sv_tilesize * sc + sv_x_0);
		double y((get_y() + 1) * sv_yr_ifrl * sv_tilesize * sc + sv_y_0);
		Point pt;
		pt.set_lon_rad_dbl(x / sv_wgs_a);
		pt.set_lat_rad_dbl(M_PI / 2. - 2 * atan(exp(-1 * y / sv_wgs_a)));
		return pt;
	}

	case mode_svsm_ifrh:
	{
		double sc(get_sv_scale_ifr());
		double x(get_x() * sv_xr_ifrh * sv_tilesize * sc + sv_x_0);
		double y((get_y() + 1) * sv_yr_ifrh * sv_tilesize * sc + sv_y_0);
		Point pt;
		pt.set_lon_rad_dbl(x / sv_wgs_a);
		pt.set_lat_rad_dbl(M_PI / 2. - 2 * atan(exp(-1 * y / sv_wgs_a)));
		return pt;
	}

	case mode_tmsch1903:
	{
		const TileLayerInfo& info(get_ch1903_info());
		double x((get_y() + 1) * (info.resolution * ch1903_tilesize));
		double y(get_x() * (info.resolution * ch1903_tilesize));
		x = ch1903_origin_x - x;
		y += ch1903_origin_y;
		Point pt;
		pt.set_ch1903p(x, y);
		return pt;
	}

	default:
	case mode_tms:
	{
		Point pt;
		pt.set_lon((get_x() << (sizeof(Point::coord_t) * 8 - get_zoom())) ^ 0x80000000);
		double n(M_PI - (2.0 * M_PI) * ldexp(get_y() + 1, -(int)get_zoom()));
		pt.set_lat_rad_dbl(atan(0.5 * (exp(n) - exp(-n))));
		pt.set_lat(pt.get_lat() + 1);
		return pt;
	}
	}
}

VectorMapRenderer::DrawThreadTMS::TileIndex::operator Rect() const
{
	switch (get_mode()) {
	case mode_svsm_vfr:
	{
		Point sw(*this);
		Point ne;
		double sc(get_sv_scale_vfr());
		double x((get_x() + 1) * sv_xr_vfr * sv_tilesize * sc + sv_x_0);
		double y(get_y() * sv_yr_vfr * sv_tilesize * sc + sv_y_0);
		ne.set_lon_rad_dbl(x / sv_wgs_a);
		ne.set_lat_rad_dbl(M_PI / 2. - 2 * atan(exp(-1 * y / sv_wgs_a)));
		return Rect(sw, ne);
	}

	case mode_svsm_ifrl:
	{
		Point sw(*this);
		Point ne;
		double sc(get_sv_scale_ifr());
		double x((get_x() + 1) * sv_xr_ifrl * sv_tilesize * sc + sv_x_0);
		double y(get_y() * sv_yr_ifrl * sv_tilesize * sc + sv_y_0);
		ne.set_lon_rad_dbl(x / sv_wgs_a);
		ne.set_lat_rad_dbl(M_PI / 2. - 2 * atan(exp(-1 * y / sv_wgs_a)));
		return Rect(sw, ne);
	}

	case mode_svsm_ifrh:
	{
		Point sw(*this);
		Point ne;
		double sc(get_sv_scale_ifr());
		double x((get_x() + 1) * sv_xr_ifrh * sv_tilesize * sc + sv_x_0);
		double y(get_y() * sv_yr_ifrh * sv_tilesize * sc + sv_y_0);
		ne.set_lon_rad_dbl(x / sv_wgs_a);
		ne.set_lat_rad_dbl(M_PI / 2. - 2 * atan(exp(-1 * y / sv_wgs_a)));
		return Rect(sw, ne);
	}

	case mode_tmsch1903:
	{
		if (true) {
			Corner pt(*this);
			return pt;
		}
		const TileLayerInfo& info(get_ch1903_info());
		Point sw(*this);
		Point ne;
		double x(get_y() * (info.resolution * ch1903_tilesize));
		double y((get_x() + 1) * (info.resolution * ch1903_tilesize));
		x = ch1903_origin_x - x;
		y += ch1903_origin_y;
		ne.set_ch1903p(x, y);
		return Rect(sw, ne);
	}

	default:
	case mode_tms:
	{
		Point sw(*this);
		Point ne;
		ne.set_lon((((get_x() + 1) << (sizeof(Point::coord_t) * 8 - get_zoom())) - 1) ^ 0x80000000);
		double n(M_PI - (2.0 * M_PI) * ldexp(get_y(), -(int)get_zoom()));
		ne.set_lat_rad_dbl(atan(0.5 * (exp(n) - exp(-n))));
		return Rect(sw, ne);
	}
	}
}

VectorMapRenderer::DrawThreadTMS::TileIndex::operator Corner() const
{
	switch (get_mode()) {
	case mode_svsm_vfr:
	{
		Corner pt;
		double sc(get_sv_scale_vfr());
		for (unsigned int i = 0; i < 4; ++i) {
			double x((get_x() + (i & 1)) * sv_xr_vfr * sv_tilesize * sc + sv_x_0);
			double y((get_y() + ((i >> 1) & 1)) * sv_yr_vfr * sv_tilesize * sc + sv_y_0);
			pt[i].set_lon_rad_dbl(x / sv_wgs_a);
			pt[i].set_lat_rad_dbl(M_PI / 2. - 2 * atan(exp(-1 * y / sv_wgs_a)));
		}
		return pt;
	}

	case mode_svsm_ifrl:
	{
		Corner pt;
		double sc(get_sv_scale_ifr());
		for (unsigned int i = 0; i < 4; ++i) {
			double x((get_x() + (i & 1)) * sv_xr_ifrl * sv_tilesize * sc + sv_x_0);
			double y((get_y() + ((i >> 1) & 1)) * sv_yr_ifrl * sv_tilesize * sc + sv_y_0);
			pt[i].set_lon_rad_dbl(x / sv_wgs_a);
			pt[i].set_lat_rad_dbl(M_PI / 2. - 2 * atan(exp(-1 * y / sv_wgs_a)));
		}
		return pt;
	}

	case mode_svsm_ifrh:
	{
		Corner pt;
		double sc(get_sv_scale_ifr());
		for (unsigned int i = 0; i < 4; ++i) {
			double x((get_x() + (i & 1)) * sv_xr_ifrh * sv_tilesize * sc + sv_x_0);
			double y((get_y() + ((i >> 1) & 1)) * sv_yr_ifrh * sv_tilesize * sc + sv_y_0);
			pt[i].set_lon_rad_dbl(x / sv_wgs_a);
			pt[i].set_lat_rad_dbl(M_PI / 2. - 2 * atan(exp(-1 * y / sv_wgs_a)));
		}
		return pt;
	}

	case mode_tmsch1903:
	{
		const TileLayerInfo& info(get_ch1903_info());
		Corner pt;
		for (unsigned int i = 0; i < 4; ++i) {
			double x((get_y() + ((i >> 1) & 1)) * (info.resolution * ch1903_tilesize));
			double y((get_x() + (i & 1)) * (info.resolution * ch1903_tilesize));
			x = ch1903_origin_x - x;
			y += ch1903_origin_y;
			pt[i].set_ch1903p(x, y);
		}
		return pt;
	}

	default:
	case mode_tms:
	{
		Corner pt;
		for (unsigned int i = 0; i < 4; ++i) {
			pt[i].set_lon((((get_x() + (i & 1)) << (sizeof(Point::coord_t) * 8 - get_zoom())) - 1) ^ 0x80000000);
			double n(M_PI - (2.0 * M_PI) * ldexp((get_y() + ((i >> 1) & 1)), -(int)get_zoom()));
			pt[i].set_lat_rad_dbl(atan(0.5 * (exp(n) - exp(-n))));
		}
		return pt;
	}
	}
}

void VectorMapRenderer::DrawThreadTMS::TileIndex::nextx(void)
{
	switch (get_mode()) {
	case mode_svsm_vfr:
	{
		unsigned int xsz(get_sv_xsize_vfr());
		++m_x;
		if (m_x >= xsz)
			m_x -= xsz;
		break;
	}

	case mode_svsm_ifrl:
	{
		unsigned int xsz(get_sv_xsize_ifrl());
		++m_x;
		if (m_x >= xsz)
			m_x -= xsz;
		break;
	}

	case mode_svsm_ifrh:
	{
		unsigned int xsz(get_sv_xsize_ifrh());
		++m_x;
		if (m_x >= xsz)
			m_x -= xsz;
		break;
	}

	case mode_tmsch1903:
	{
		const TileLayerInfo& info(get_ch1903_info());
		if (m_x + 1 < info.width)
			++m_x;
		break;
	}

	default:
	case mode_tms:
	{
		unsigned int mask(get_mask());
		m_x = (m_x + 1U) & mask;
		break;
	}
	}
}

void VectorMapRenderer::DrawThreadTMS::TileIndex::prevx(void)
{
	switch (get_mode()) {
	case mode_svsm_vfr:
		if (!m_x)
			m_x += get_sv_xsize_vfr();
		--m_x;
		break;

	case mode_svsm_ifrl:
		if (!m_x)
			m_x += get_sv_xsize_ifrl();
		--m_x;
		break;

	case mode_svsm_ifrh:
		if (!m_x)
			m_x += get_sv_xsize_ifrh();
		--m_x;
		break;

	case mode_tmsch1903:
		if (m_x)
			--m_x;
		break;

	default:
	case mode_tms:
	{
		unsigned int mask(get_mask());
		m_x = (m_x - 1U) & mask;
		break;
	}
	}
}

void VectorMapRenderer::DrawThreadTMS::TileIndex::nexty(void)
{
	switch (get_mode()) {
	case mode_svsm_vfr:
		if (m_y < get_sv_ysize_vfr())
			++m_y;
		break;

	case mode_svsm_ifrl:
		if (m_y < get_sv_ysize_ifrl())
			++m_y;
		break;

	case mode_svsm_ifrh:
		if (m_y < get_sv_ysize_ifrh())
			++m_y;
		break;

	case mode_tmsch1903:
	{
		const TileLayerInfo& info(get_ch1903_info());
		if (m_y + 1 < info.height)
			++m_y;
		break;
	}

	default:
	case mode_tms:
	{
		unsigned int mask(get_mask());
		if (m_y < mask)
			++m_y;
		break;
	}
	}
}

void VectorMapRenderer::DrawThreadTMS::TileIndex::prevy(void)
{
	switch (get_mode()) {
	case mode_svsm_vfr:
	case mode_svsm_ifrl:
	case mode_svsm_ifrh:
	case mode_tmsch1903:
		if (m_y)
			--m_y;
		break;

	default:
	case mode_tms:
		if (m_y)
			--m_y;
		break;
	}
}

double VectorMapRenderer::DrawThreadTMS::TileIndex::nmi_per_pixel(unsigned int zoom, const Point& pt)
{
	switch (get_mode()) {
	case mode_svsm_vfr:
	{
		double scale(Point::earth_radius_dbl * Point::km_to_nmi_dbl * (2.0 * M_PI / sv_chartwidth_vfr));
		scale *= get_sv_scale_vfr(zoom);
		scale *= cos(pt.get_lat_rad_dbl());
		return scale;
	}

	case mode_svsm_ifrl:
	{
		double scale(Point::earth_radius_dbl * Point::km_to_nmi_dbl * (2.0 * M_PI / sv_chartwidth_ifrl));
		scale *= get_sv_scale_ifr(zoom);
		scale *= cos(pt.get_lat_rad_dbl());
		return scale;
	}

	case mode_svsm_ifrh:
	{
		double scale(Point::earth_radius_dbl * Point::km_to_nmi_dbl * (2.0 * M_PI / sv_chartwidth_ifrh));
		scale *= get_sv_scale_ifr(zoom);
		scale *= cos(pt.get_lat_rad_dbl());
		return scale;
	}

	case mode_tmsch1903:
	{
		const TileLayerInfo& info(get_ch1903_info(zoom));
		return info.resolution * (1e-3 * Point::km_to_nmi_dbl);
	}

	default:
	case mode_tms:
	{
		double scale(ldexp(Point::earth_radius_dbl * Point::km_to_nmi_dbl * (2.0 * M_PI / 256.0), -(int)zoom));
		scale *= cos(pt.get_lat_rad_dbl());
		return scale;
	}
	}
}



unsigned int VectorMapRenderer::DrawThreadTMS::TileIndex::nearest_zoom(double nmiperpixel, const Point& pt)
{
	switch (get_mode()) {
	case mode_svsm_vfr:
	{
		// nmiperpixel = Point::earth_radius_dbl * Point::km_to_nmi_dbl * (2.0 * M_PI / sv_chartwidth_vfr) * cos(lat) * sv_scale(zoom)
		// sv_scale(zoom) = nmiperpixel / (Point::earth_radius_dbl * Point::km_to_nmi_dbl * (2.0 * M_PI / sv_chartwidth_vfr) * cos(lat))
		double scale(Point::earth_radius_dbl * Point::km_to_nmi_dbl * (2.0 * M_PI / sv_chartwidth_vfr));
		scale *= cos(pt.get_lat_rad_dbl());
		scale = nmiperpixel / scale;
		int z(0);
		if (scale > 1) {
			scale = frexp(scale, &z);
			if (z) {
				scale *= 2;
				--z;
			}
			z *= 2;
		}
		unsigned int zz(0);
		double m(scale / get_sv_scale_vfr(zz));
		if (m < 1 && m > 0)
			m = 1 / m;
		for (unsigned int z1 = 1; z1 <= 2; ++z1) {
			double m1(scale / get_sv_scale_vfr(z1));
			if (m1 < 1 && m1 > 0)
				m1 = 1 / m1;
			if (m1 >= m)
				continue;
			m = m1;
			zz = z1;
		}
		if (false)
			std::cerr << "nearest_zoom: " << (zz + z) << " nmiperpix req " << nmiperpixel
				  << " actual " << nmi_per_pixel(zz + z, pt) << std::endl;
		return zz + z;
	}

	case mode_svsm_ifrl:
	{
		// nmiperpixel = Point::earth_radius_dbl * Point::km_to_nmi_dbl * (2.0 * M_PI / sv_chartwidth_ifrl) * cos(lat) * sv_scale(zoom)
		// sv_scale(zoom) = nmiperpixel / (Point::earth_radius_dbl * Point::km_to_nmi_dbl * (2.0 * M_PI / sv_chartwidth_ifrl) * cos(lat))
		double scale(Point::earth_radius_dbl * Point::km_to_nmi_dbl * (2.0 * M_PI / sv_chartwidth_ifrl));
		scale *= cos(pt.get_lat_rad_dbl());
		scale = nmiperpixel / scale;
		int z(0);
		if (scale > 1) {
			scale = frexp(scale, &z);
			if (z) {
				scale *= 2;
				--z;
			}
			z *= 2;
		}
		unsigned int zz(0);
		double m(scale / get_sv_scale_ifr(zz));
		if (m < 1 && m > 0)
			m = 1 / m;
		for (unsigned int z1 = 1; z1 <= 2; ++z1) {
			double m1(scale / get_sv_scale_ifr(z1));
			if (m1 < 1 && m1 > 0)
				m1 = 1 / m1;
			if (m1 >= m)
				continue;
			m = m1;
			zz = z1;
		}
		if (false)
			std::cerr << "nearest_zoom: " << (zz + z) << " nmiperpix req " << nmiperpixel
				  << " actual " << nmi_per_pixel(zz + z, pt) << std::endl;
		return zz + z;
	}

	case mode_svsm_ifrh:
	{
		// nmiperpixel = Point::earth_radius_dbl * Point::km_to_nmi_dbl * (2.0 * M_PI / sv_chartwidth_ifrh) * cos(lat) * sv_scale(zoom)
		// sv_scale(zoom) = nmiperpixel / (Point::earth_radius_dbl * Point::km_to_nmi_dbl * (2.0 * M_PI / sv_chartwidth_ifrh) * cos(lat))
		double scale(Point::earth_radius_dbl * Point::km_to_nmi_dbl * (2.0 * M_PI / sv_chartwidth_ifrh));
		scale *= cos(pt.get_lat_rad_dbl());
		scale = nmiperpixel / scale;
		int z(0);
		if (scale > 1) {
			scale = frexp(scale, &z);
			if (z) {
				scale *= 2;
				--z;
			}
			z *= 2;
		}
		unsigned int zz(0);
		double m(scale / get_sv_scale_ifr(zz));
		if (m < 1 && m > 0)
			m = 1 / m;
		for (unsigned int z1 = 1; z1 <= 2; ++z1) {
			double m1(scale / get_sv_scale_ifr(z1));
			if (m1 < 1 && m1 > 0)
				m1 = 1 / m1;
			if (m1 >= m)
				continue;
			m = m1;
			zz = z1;
		}
		if (false)
			std::cerr << "nearest_zoom: " << (zz + z) << " nmiperpix req " << nmiperpixel
				  << " actual " << nmi_per_pixel(zz + z, pt) << std::endl;
		return zz + z;
	}

	case mode_tmsch1903:
	{
		double res(nmiperpixel * (1e3 / Point::km_to_nmi_dbl));
		unsigned int zb(0);
		double mb(res / ch1903_layerinfo[zb].resolution);
		if (mb < 1 && mb > 0)
			mb = 1.0 / mb;
		for (unsigned int z(1); z < sizeof(ch1903_layerinfo)/sizeof(ch1903_layerinfo[0]); ++z) {
			double m(res / ch1903_layerinfo[z].resolution);
			if (m < 1 && m > 0)
				m = 1.0 / m;
			if (m > mb)
				continue;
			mb = m;
			zb = z;
		}
		return zb;
	}

	default:
	case mode_tms:
	{
		// nmiperpixel = cos(lat) * Point::earth_radius_dbl * Point::km_to_nmi_dbl * (2.0 * M_PI / 256.0) * 2^-zoom
		// 2^-zoom = nmiperpixel / cos(lat) / (Point::earth_radius_dbl * Point::km_to_nmi_dbl) * (256.0 / 2.0 / M_PI)
		double c(cos(pt.get_lat_rad_dbl()));
		if (c < 1e-6)
			return 0;
		c = nmiperpixel / c * ((256.0 / 2.0 / M_PI) / (Point::earth_radius_dbl * Point::km_to_nmi_dbl));
		int exp;
		double f(frexp(c, &exp));
		exp = -exp;
		if (f >= 0.70711)
			++exp;
		if (exp < 0)
			exp = 0;
		return exp;
	}
	}
}

VectorMapRenderer::DrawThreadTMS::TileIndex::operator std::string() const
{
	std::ostringstream oss;
	oss << '(' << get_zoom() << ',' << get_x() << ',' << get_y() << ')';
	return oss.str();
}

bool VectorMapRenderer::DrawThreadTMS::TileIndex::operator<(const TileIndex& x) const
{
	if (get_zoom() < x.get_zoom())
		return true;
	if (get_zoom() > x.get_zoom())
		return false;
	if (get_x() < x.get_x())
		return true;
	if (get_x() > x.get_x())
		return false;
	return get_y() < x.get_y();
}

bool VectorMapRenderer::DrawThreadTMS::TileIndex::operator==(const TileIndex& x) const
{
	return (get_zoom() == x.get_zoom()) && (get_x() == x.get_x()) && (get_y() == x.get_y());
}

std::string VectorMapRenderer::DrawThreadTMS::TileIndex::quadtree_string(const char *quadrants) const
{
       	std::string r;
	if (!quadrants)
		return r;
	for (unsigned int n = get_zoom(); n;) {
		--n;
		unsigned int xbit = (get_x() >> n) & 1;
		unsigned int ybit = (get_y() >> n) & 1;
		r.push_back(quadrants[xbit + 2 * ybit]);
	}
	return r;
}

VectorMapRenderer::DrawThreadTMS::TileCache::TileCache(unsigned int source)
	: m_session(0), m_msg(0), m_source(source), m_dispatched(false)
{
	if (m_source >= sizeof(tilesources)/sizeof(tilesources[0]))
		m_source = sizeof(tilesources)/sizeof(tilesources[0])-1;
	m_dispatch.connect(sigc::mem_fun(*this, &TileCache::runqueue));
}

VectorMapRenderer::DrawThreadTMS::TileCache::TileCache(const TileCache& x)
	: m_session(0), m_msg(0), m_source(x.get_source()), m_dispatched(false)
{
	m_dispatch.connect(sigc::mem_fun(*this, &TileCache::runqueue));
	{
		Glib::Mutex::Lock lock(const_cast<Glib::Mutex&>(x.m_mutex));
		m_failed = x.m_failed;
		m_queuechange = x.m_queuechange;
	}
}

VectorMapRenderer::DrawThreadTMS::TileCache::~TileCache()
{
	if (m_msg)
		soup_session_cancel_message(m_session, m_msg, SOUP_STATUS_CANCELLED);
	if (m_session) {
		soup_session_abort(m_session);
		g_object_unref(m_session);
	}
	{
		Glib::Mutex::Lock lock(m_mutex);
		m_queuechange.clear();
		m_queue.clear();
		m_failed.clear();
	}
}

VectorMapRenderer::DrawThreadTMS::TileCache& VectorMapRenderer::DrawThreadTMS::TileCache::operator=(const TileCache& x)
{
	if (m_msg) {
		soup_session_cancel_message(m_session, m_msg, SOUP_STATUS_CANCELLED);
		m_msg = 0;
	}
	if (m_session) {
		soup_session_abort(m_session);
		g_object_unref(m_session);
		m_session = 0;
	}
	failed_t failed;
	sigc::signal<void> queuechange;
	{
		Glib::Mutex::Lock lock(const_cast<Glib::Mutex&>(x.m_mutex));
		failed = x.m_failed;
		queuechange = x.m_queuechange;
	}
	{
		Glib::Mutex::Lock lock(m_mutex);
		m_queuechange.clear();
		m_queue.clear();
		m_failed.clear();
		m_queuechange = queuechange;
		m_failed.swap(failed);
	}
	m_source = x.get_source();
	return *this;
}

sigc::connection VectorMapRenderer::DrawThreadTMS::TileCache::connect_queuechange(const sigc::slot<void>& slot)
{
	Glib::Mutex::Lock lock(m_mutex);
	return m_queuechange.connect(slot);
}

std::string VectorMapRenderer::DrawThreadTMS::TileCache::get_filename(const TileIndex& idx) const
{
	std::ostringstream src;
	std::ostringstream z;
	std::ostringstream x;
	std::ostringstream y;
	src << m_source;
	z << idx.get_zoom();
	x << idx.get_x();
	y << idx.get_y();
	switch (tilesources[m_source].imgfmt) {
	case TileSource::imgfmt_png:
		y << ".png";
		break;

	case TileSource::imgfmt_jpg:
		y << ".jpg";
		break;

	default:
		break;
	}
	return Glib::build_filename(FPlan::get_userdbpath(), "osm", src.str(), z.str(), x.str(), y.str());
}

std::string VectorMapRenderer::DrawThreadTMS::TileCache::get_uri(const TileIndex& idx)
{
	std::string uri(tilesources[m_source].uri);
	std::string::size_type n;
	while ((n = uri.find("#X")) != std::string::npos) {
		std::ostringstream oss;
		oss << idx.get_x();
		uri.replace(n, 2, oss.str());
	}
	while ((n = uri.find("#Y")) != std::string::npos) {
		std::ostringstream oss;
		oss << idx.get_y();
		uri.replace(n, 2, oss.str());
	}
	while ((n = uri.find("#Z")) != std::string::npos) {
		std::ostringstream oss;
		oss << idx.get_zoom();
		uri.replace(n, 2, oss.str());
	}
	while ((n = uri.find("#S")) != std::string::npos) {
		std::ostringstream oss;
		oss << (tilesources[m_source].maxzoom - idx.get_zoom());
		uri.replace(n, 2, oss.str());
	}
	while ((n = uri.find("#R")) != std::string::npos) {
		std::ostringstream oss;
		oss << (m_random.get_int() & 3);
		uri.replace(n, 2, oss.str());
	}
	while ((n = uri.find("#r")) != std::string::npos) {
		std::ostringstream oss;
		oss << (m_random.get_int() & 1);
		uri.replace(n, 2, oss.str());
	}
	while ((n = uri.find("#Q")) != std::string::npos) {
		uri.replace(n, 2, "t" + idx.quadtree_string("qrts"));
	}
	while ((n = uri.find("#W")) != std::string::npos) {
		uri.replace(n, 2, idx.quadtree_string("0123"));
	}
	while ((n = uri.find("#T")) != std::string::npos) {
		std::ostringstream oss;
		oss << ((idx.get_x() + idx.get_y()) & 1);
		uri.replace(n, 2, oss.str());
	}
	while ((n = uri.find("#A")) != std::string::npos) {
		const AIRACDates *airacdates_begin = airacdates;
		const AIRACDates *airacdates_end = airacdates + sizeof(airacdates)/sizeof(airacdates[0]);
		AIRACDates d = { time(0), 0 };
		const AIRACDates *it(std::lower_bound(airacdates_begin, airacdates_end, d));
		if (it == airacdates_end || (it != airacdates_begin && d < *it))
			--it;
		uri.replace(n, 2, it->airac);
	}
	return uri;
}

void VectorMapRenderer::DrawThreadTMS::TileCache::mkdir(const TileIndex& idx) const
{
	std::ostringstream src;
	std::ostringstream z;
	std::ostringstream x;
	src << m_source;
	z << idx.get_zoom();
	x << idx.get_x();
	std::string path(Glib::build_filename(FPlan::get_userdbpath(), "osm", src.str(), z.str(), x.str()));
	g_mkdir_with_parents(path.c_str(), 0755);
}

bool VectorMapRenderer::DrawThreadTMS::TileCache::exists(const TileIndex& idx) const
{
	std::string f(get_filename(idx));
	return Glib::file_test(f, Glib::FILE_TEST_EXISTS) && Glib::file_test(f, Glib::FILE_TEST_IS_REGULAR);
}

Glib::RefPtr<Gdk::Pixbuf> VectorMapRenderer::DrawThreadTMS::TileCache::get_pixbuf(const TileIndex& idx)
{
	std::string f(get_filename(idx));
	if (!(Glib::file_test(f, Glib::FILE_TEST_EXISTS) && Glib::file_test(f, Glib::FILE_TEST_IS_REGULAR))) {
		queue(idx);
		return Glib::RefPtr<Gdk::Pixbuf>();
	}
	{
		Glib::RefPtr<Gio::File> file(Gio::File::create_for_path(f));
		if (!file)
			return Glib::RefPtr<Gdk::Pixbuf>();
		Glib::RefPtr<Gio::FileInfo> fi(file->query_info("standard::*", Gio::FILE_QUERY_INFO_NONE ));
		if (!fi)
			return Glib::RefPtr<Gdk::Pixbuf>();
		if (fi->get_size() <= 0)
			return Glib::RefPtr<Gdk::Pixbuf>();
	}
	return Gdk::Pixbuf::create_from_file(f);
}

void VectorMapRenderer::DrawThreadTMS::TileCache::clear_queue(void)
{
	Glib::Mutex::Lock lock(m_mutex);
	if (!m_msg) {
		m_queue.clear();
		return;
	}
	m_queue.erase(m_queue.begin() + 1, m_queue.end());
}

void VectorMapRenderer::DrawThreadTMS::TileCache::queue_rect(unsigned int zoom, const Rect& bbox)
{
	TileIndex idx0(tilesources[m_source].idxmode, zoom);
	TileIndex idxe(tilesources[m_source].idxmode, zoom);
	idx0.set_rounddown(bbox.get_southwest());
	idxe.set_roundup(bbox.get_northeast());
	for (;;) {
		TileIndex idx1(idx0);
		for (;;) {
			if (!exists(idx1))
				queue(idx1);
			if (idx1.get_x() == idxe.get_x())
				break;
			idx1.nextx();
		}
		if (idx0.get_y() == idxe.get_y())
			break;
		idx0.prevy();
	}
}

unsigned int VectorMapRenderer::DrawThreadTMS::TileCache::rect_tiles(unsigned int zoom, const Rect& bbox, bool download_only)
{
	TileIndex idx0(tilesources[m_source].idxmode, zoom);
	TileIndex idxe(tilesources[m_source].idxmode, zoom);
	idx0.set_rounddown(bbox.get_southwest());
	idxe.set_roundup(bbox.get_northeast());
	if (!download_only) {
		unsigned int m(idxe.get_xsize());
		unsigned int sz(m + idxe.get_x() - idx0.get_x());
		sz %= m;
		sz += 1;
		sz *= (idx0.get_y() - idxe.get_y() + 1);
		return sz;
	}
	unsigned int sz(0);
	for (;;) {
		TileIndex idx1(idx0);
		for (;;) {
			if (!exists(idx1))
				++sz;
			if (idx1.get_x() == idxe.get_x())
				break;
			idx1.nextx();
		}
		if (idx0.get_y() == idxe.get_y())
			break;
		idx0.prevy();
	}
	return sz;
}

unsigned int VectorMapRenderer::DrawThreadTMS::TileCache::get_minzoom(void) const
{
	return tilesources[m_source].minzoom;
}

unsigned int VectorMapRenderer::DrawThreadTMS::TileCache::get_maxzoom(void) const
{
	return tilesources[m_source].maxzoom;
}

const char *VectorMapRenderer::DrawThreadTMS::TileCache::get_shortname(void) const
{
	return tilesources[m_source].shortname;
}

const char *VectorMapRenderer::DrawThreadTMS::TileCache::get_friendlyname(void) const
{
	return tilesources[m_source].friendlyname;
}

const char *VectorMapRenderer::DrawThreadTMS::TileCache::get_uri(void) const
{
	return tilesources[m_source].uri;
}

void VectorMapRenderer::DrawThreadTMS::TileCache::queue(const TileIndex& idx)
{
	bool d(false);
	{
		Glib::Mutex::Lock lock(m_mutex);
		if (m_failed.find(idx) != m_failed.end())
			return;
		m_queue.push_back(idx);
		d = m_dispatched;
		m_dispatched = true;
	}
	if (!d)
		m_dispatch();
	m_queuechange();
}

void VectorMapRenderer::DrawThreadTMS::TileCache::runqueue(void)
{
	static const char useragent[] = "Mozilla/5.0 (Windows; U; Windows NT 5.1; en-US; rv:1.8.1.11) Gecko/20071127 Firefox/2.0.0.11";
	if (m_msg)
		return;
	for (;;) {
		TileIndex idx;
		bool failed(false);
		{
			Glib::Mutex::Lock lock(m_mutex);
			m_dispatched = false;
			if (m_queue.empty())
				break;
			idx = m_queue.front();
			failed = m_failed.find(idx) != m_failed.end();
			if (failed)
				m_queue.erase(m_queue.begin());
		}
		if (failed) {
			if (true)
				std::cerr << "DrawThreadTMS::TileCache: skipping failed " << (std::string)idx << std::endl;
			continue;
		}
		if (exists(idx)) {
			{
				Glib::Mutex::Lock lock(m_mutex);
				m_queue.erase(m_queue.begin());
			}
			if (true)
				std::cerr << "DrawThreadTMS::TileCache: skipping already downloaded " << (std::string)idx << std::endl;
			continue;
		}
		if (!m_session)
			m_session = soup_session_async_new_with_options(SOUP_SESSION_USER_AGENT, useragent, NULL);
		std::string uri(get_uri(idx));
		m_msg = soup_message_new(SOUP_METHOD_GET, uri.c_str());
		if (!m_msg)
			break;
		if (uri.find("google.com") != std::string::npos)
			soup_message_headers_append(m_msg->request_headers, "Referer", "http://maps.google.com/");
		else if (uri.find("geo.admin.ch") != std::string::npos)
			soup_message_headers_append(m_msg->request_headers, "Referer", "http://map.geo.admin.ch/");
		soup_session_queue_message(m_session, m_msg, download_complete_1, this);
		if (true)
			std::cerr << "DrawThreadTMS::TileCache: downloading " << (std::string)idx << " uri " << uri << std::endl;
		break;
	}
}

void VectorMapRenderer::DrawThreadTMS::TileCache::download_complete_1(SoupSession *session, SoupMessage *msg, gpointer user_data)
{
	static_cast<TileCache *>(user_data)->download_complete(session, msg);
}

void VectorMapRenderer::DrawThreadTMS::TileCache::download_complete(SoupSession *session, SoupMessage *msg)
{
	if (true)
		std::cerr << "DrawThreadTMS::TileCache: download complete, status " << msg->status_code << std::endl;
	if (SOUP_STATUS_IS_SUCCESSFUL(msg->status_code)) {
		TileIndex idx;
		{
			Glib::Mutex::Lock lock(m_mutex);
			if (m_queue.empty())
				return;
			idx = m_queue.front();
			m_queue.erase(m_queue.begin());
		}
		mkdir(idx);
		{
			std::string fname(get_filename(idx));
			Glib::RefPtr<Gio::File> file(Gio::File::create_for_path(fname));
			gssize r;
			{
				Glib::RefPtr<Gio::FileOutputStream> outstream(file->create_file());
				r = outstream->write(msg->response_body->data, msg->response_body->length);
			}
			if (r < msg->response_body->length) {
				if (true)
					std::cerr << "DrawThreadTMS::TileCache: cannot write to " << fname
						  << " (written " << r << " size " << msg->response_body->length << ')' << std::endl;
				file->remove();
				Glib::Mutex::Lock lock(m_mutex);
				m_failed.insert(idx);
			}
		}
		m_queuechange();
		m_msg = 0;
		runqueue();
		return;
	}
        if ((msg->status_code == SOUP_STATUS_NOT_FOUND) || (msg->status_code == SOUP_STATUS_FORBIDDEN)) {
		TileIndex idx;
		{
			Glib::Mutex::Lock lock(m_mutex);
			if (m_queue.empty())
				return;
			idx = m_queue.front();
			m_queue.erase(m_queue.begin());
			m_failed.insert(idx);
		}
		std::cerr << "DrawThreadTMS: cannot download tile " << (std::string)idx << std::endl;
		m_queuechange();
		m_msg = 0;
		runqueue();
		return;
	}
	if (msg->status_code == SOUP_STATUS_CANCELLED) {
		TileIndex idx;
		{
			Glib::Mutex::Lock lock(m_mutex);
			if (m_queue.empty())
				return;
			idx = m_queue.front();
			m_queue.erase(m_queue.begin());
		}
		std::cerr << "DrawThreadTMS: cancelled download tile " << (std::string)idx << std::endl;
		m_queuechange();
		m_msg = 0;
		return;
	}
	soup_session_requeue_message(session, msg);
}

VectorMapRenderer::DrawThreadTMS::DrawThreadTMS(Engine& eng, Glib::Dispatcher& dispatch, unsigned int source, const ScreenCoord& scrsize, const Point& center, int alt, uint16_t upangle, int64_t time)
        : DrawThreadOverlay(eng, dispatch, scrsize, center, alt, upangle, time), m_cache(source), m_drawstate(drawstate_done)
{
	m_cache.connect_queuechange(sigc::mem_fun(*this, &DrawThreadTMS::tile_queue_changed));
}

VectorMapRenderer::DrawThreadTMS::~DrawThreadTMS()
{
        async_cancel();
}

void VectorMapRenderer::DrawThreadTMS::draw_restart(bool dbquery)
{
	m_drawstate = drawstate_ogm;
	m_cache.clear_queue();
	dbquery = dbquery || need_dbquery();
        if (dbquery) {
		recompute_dbrectangle();
		std::cerr << "bbox: " << get_dbrectangle() << " (center " << draw_get_imagecenter() << ')' << std::endl;
		db_restart();
        }
        // start drawing
        Cairo::RefPtr<Cairo::Context> cr = draw_create_context();
        cr->rectangle(0, 0, draw_get_imagesize().getx(), draw_get_imagesize().gety());
        cr->set_source_rgb(1.0, 1.0, 1.0);
        cr->fill();
}

void VectorMapRenderer::DrawThreadTMS::draw_iterate(void)
{
	Cairo::RefPtr<Cairo::Context> cr = draw_create_context();
        cr->rectangle(0, 0, draw_get_imagesize().getx(), draw_get_imagesize().gety());
        cr->clip();
        //cr->set_tolerance(1.0);

	switch (m_drawstate) {
	case drawstate_ogm:
	{
		m_cache.clear_queue();
		unsigned int zoom(TileIndex(tilesources[m_cache.get_source()].idxmode).nearest_zoom(draw_get_scale(), draw_get_imagecenter()));
		if (zoom < tilesources[m_cache.get_source()].minzoom)
			zoom = tilesources[m_cache.get_source()].minzoom;
		else if (zoom > tilesources[m_cache.get_source()].maxzoom)
			zoom = tilesources[m_cache.get_source()].maxzoom;
		TileIndex idx0(tilesources[m_cache.get_source()].idxmode, zoom);
		TileIndex idxe(tilesources[m_cache.get_source()].idxmode, zoom);
		Rect bbox(draw_coverrect(1.1, 1.1));
		idx0.set_rounddown(bbox.get_southwest());
		idxe.set_roundup(bbox.get_northeast());
		if (true)
			std::cerr << "DrawThreadTMS::draw_iterate: center " << draw_get_imagecenter()
				  << " scale " << draw_get_scale() << " tilescale "
				  << TileIndex(tilesources[m_cache.get_source()].idxmode).nmi_per_pixel(zoom, draw_get_imagecenter())
				  << " zoom " << zoom << " tile indices " << (std::string)idx0
				  << '-' << (std::string)idxe << " bbox " << bbox << std::endl;
		if (idx0.get_y() >= idxe.get_y()) {
			for (;;) {
				TileIndex idx1(idx0);
				for (;;) {
					Glib::RefPtr<Gdk::Pixbuf> pbuf(m_cache.get_pixbuf(idx1));
					if (pbuf) {
						if (false)
							std::cerr << "DrawThreadTMS::draw_iterate: tile " << (std::string)idx1
								  << ' ' << (Rect)idx1 << ' ' << pbuf->get_width()
								  << 'x' << pbuf->get_height() << std::endl;
						draw_pixbuf(cr, (Corner)idx1, pbuf, 1);
					}
					if (idx1.get_x() == idxe.get_x())
						break;
					idx1.nextx();
				}
				if (idx0.get_y() == idxe.get_y())
					break;
				idx0.prevy();
			}
		}
		m_drawstate = drawstate_navaids;
		// fall through
	}

	case drawstate_navaids:
		if (!do_draw_navaids(cr.operator->()))
			return;
		m_drawstate = drawstate_waypoints;
		// fall through

	case drawstate_waypoints:
		if (!do_draw_waypoints(cr.operator->()))
			return;
		m_drawstate = drawstate_airports;
		// fall through

	case drawstate_airports:
		if (!do_draw_airports(cr.operator->()))
			return;
		m_drawstate = drawstate_airspaces;
		// fall through

	case drawstate_airspaces:
		if (!do_draw_airspaces(cr.operator->()))
			return;
		m_drawstate = drawstate_airways;
		// fall through

	case drawstate_airways:
		if (!do_draw_airways(cr.operator->()))
			return;
		m_drawstate = drawstate_weather;
		// fall through

	case drawstate_weather:
		do_draw_weather(cr.operator->());
		m_drawstate = drawstate_grib2layer;
		// fall through

	case drawstate_grib2layer:
		do_draw_grib2layer(cr.operator->());
		m_drawstate = drawstate_done;
		// fall through

	case drawstate_done:
	default:
		draw_done();
		break;
	}
}

void VectorMapRenderer::DrawThreadTMS::tile_queue_changed(void)
{
	std::cerr << "DrawThreadTMS: tile queue " << m_cache.queue_size() << std::endl;
	if (!m_cache.queue_size())
		restart();
}

VectorMapRenderer::DrawThreadBitmap::DrawThreadBitmap(Engine& eng, Glib::Dispatcher& dispatch, const Glib::RefPtr<BitmapMaps::Map>& map, const ScreenCoord& scrsize, const Point& center, int alt, uint16_t upangle, int64_t time)
        : DrawThreadOverlay(eng, dispatch, scrsize, center, alt, upangle, time), m_map(map), m_drawstate(drawstate_done)
{
}

VectorMapRenderer::DrawThreadBitmap::~DrawThreadBitmap()
{
        async_cancel();
	if (m_map)
		m_map->flush_cache();
}

void VectorMapRenderer::DrawThreadBitmap::set_bitmap(const Glib::RefPtr<BitmapMaps::Map>& map)
{
	if (m_map == map)
		return;
	if (m_map)
		m_map->flush_cache();
	m_map = map;
	restart();
}

void VectorMapRenderer::DrawThreadBitmap::draw_restart(bool dbquery)
{
	m_drawstate = drawstate_bitmap;
	dbquery = dbquery || need_dbquery();
	dbquery = dbquery && get_map_scale() <= 0.1;
        if (dbquery) {
		recompute_dbrectangle();
		std::cerr << "bbox: " << get_dbrectangle() << " (center " << draw_get_imagecenter() << ')' << std::endl;
		db_restart();
        }
        // start drawing
        Cairo::RefPtr<Cairo::Context> cr = draw_create_context();
        cr->rectangle(0, 0, draw_get_imagesize().getx(), draw_get_imagesize().gety());
        cr->set_source_rgb(1.0, 1.0, 1.0);
        cr->fill();
}

void VectorMapRenderer::DrawThreadBitmap::draw_iterate(void)
{
	Cairo::RefPtr<Cairo::Context> cr = draw_create_context();
        cr->rectangle(0, 0, draw_get_imagesize().getx(), draw_get_imagesize().gety());
        cr->clip();
        //cr->set_tolerance(1.0);

	switch (m_drawstate) {
	case drawstate_bitmap:
		if (m_map) {
			Rect bbox(draw_coverrect(2, 2));
			for (unsigned int i = 0, sz = m_map->size(); i < sz; ++i) {
				BitmapMaps::Map::Tile::ptr_t tile(m_map->operator[](i));
				if (!tile)
					continue;
				if (!bbox.is_intersect(tile->get_bbox()))
					continue;
				Glib::RefPtr<Gdk::Pixbuf> pbuf(tile->get_pixbuf());
				if (!pbuf)
					continue;
				if (true)
					std::cerr << "bitmap: " << m_map->get_name() << " tile " << tile->get_file()
						  << ' ' << tile->get_width() << 'x' << tile->get_height() << ' '
						  << ' ' << tile->get_bbox().get_southwest().get_lat_str2()
						  << ' ' << tile->get_bbox().get_southwest().get_lon_str2()
						  << ' ' << tile->get_bbox().get_northeast().get_lat_str2()
						  << ' ' << tile->get_bbox().get_northeast().get_lon_str2()
						  << ' ' << tile->get_file() << " scale " << get_map_scale()
						  << ' ' << tile->get_nmi_per_pixel() << std::endl;
				draw_pixbuf(cr, tile->get_bbox(), pbuf, 1);
			}
		}
		if (get_map_scale() > 0.1) {
			m_drawstate = drawstate_weather;
			goto draw_weather;
		}
		m_drawstate = drawstate_navaids;
		// fall through

	case drawstate_navaids:
		if (!do_draw_navaids(cr.operator->()))
			return;
		m_drawstate = drawstate_waypoints;
		// fall through

	case drawstate_waypoints:
		if (!do_draw_waypoints(cr.operator->()))
			return;
		m_drawstate = drawstate_airports;
		// fall through

	case drawstate_airports:
		if (!do_draw_airports(cr.operator->()))
			return;
		m_drawstate = drawstate_airspaces;
		// fall through

	case drawstate_airspaces:
		if (!do_draw_airspaces(cr.operator->()))
			return;
		m_drawstate = drawstate_airways;
		// fall through

	case drawstate_airways:
		if (!do_draw_airways(cr.operator->()))
			return;
		m_drawstate = drawstate_weather;
		// fall through

	case drawstate_weather:
	draw_weather:
		do_draw_weather(cr.operator->());
		m_drawstate = drawstate_grib2layer;
		// fall through

	case drawstate_grib2layer:
		do_draw_grib2layer(cr.operator->());
		m_drawstate = drawstate_done;
		// fall through

	case drawstate_done:
	default:
		draw_done();
		break;
	}
}

#endif /* HAVE_MULTITHREADED_MAPS */
