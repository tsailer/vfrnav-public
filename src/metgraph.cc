/*****************************************************************************/

/*
 *      metgraph.cc  --  Meteo Graphs.
 *
 *      Copyright (C) 2014, 2015, 2016, 2017  Thomas Sailer (t.sailer@alumni.ethz.ch)
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

#include <iomanip>
#include <fstream>

#include <librsvg/rsvg.h>

#define __RSVG_RSVG_H_INSIDE__
#include <librsvg/rsvg-cairo.h>
#undef __RSVG_RSVG_H_INSIDE__

#include "metgraph.h"
#include "aircraft.h"
#include "baro.h"
#include "wind.h"

#ifdef HAVE_PQXX
#include <pqxx/connection.hxx>
#include <pqxx/transaction.hxx>
#include <pqxx/transactor.hxx>
#include <pqxx/result.hxx>
#include <pqxx/except.hxx>
#endif

MeteoProfile::DriftDownProfilePoint::DriftDownProfilePoint(double dist, double routedist, unsigned int routeindex,
							   const Point& pt, gint64 efftime, int32_t alt, int32_t galt,
							   const std::string& sitename)
	: m_sitename(sitename), m_pt(pt), m_efftime(efftime), m_dist(dist), m_routedist(routedist), m_routeindex(routeindex),
	  m_alt(alt), m_glidealt(galt)
{
}

bool MeteoProfile::DriftDownProfilePoint::add_glide(int32_t alt, const std::string& sitename)
{
	if (alt >= m_glidealt)
		return false;
	m_glidealt = alt;
	m_sitename = sitename;
	return true;
}

std::ostream& MeteoProfile::DriftDownProfilePoint::print(std::ostream& os, unsigned int indent) const
{
	std::ostringstream oss;
	oss << "R " << get_routeindex() << " RD" << std::fixed << std::setprecision(2) << get_routedist()
	    << " D" << std::setprecision(2) << get_dist() << ' ' << get_pt().get_lat_str3() << ' ' << get_pt().get_lon_str3()
	    << ' ' << Glib::TimeVal(get_efftime(), 0).as_iso8601().substr(11, 8) << ' ' << get_alt() << "ft";
	if (get_glidealt() != std::numeric_limits<int32_t>::max()) {
		oss << " G " << get_glidealt() << "ft";
		if (!get_sitename().empty())
			oss << ' ' << get_sitename();
	}
	return os << std::string(indent, ' ') << oss.str() << std::endl;
}

MeteoProfile::DriftDownProfile::DriftDownProfile(void)
{
}

double MeteoProfile::DriftDownProfile::get_dist(void) const
{
	if (empty())
		return 0;
	return back().get_routedist();
}

std::ostream& MeteoProfile::DriftDownProfile::print(std::ostream& os, unsigned int indent) const
{
	os << std::string(indent, ' ') << "Drift Down Profile:" << std::endl;
	for (const_iterator i(begin()), e(end()); i != e; ++i)
		i->print(os, indent + 2);
	return os;
}

int MeteoProfile::LinePoint::compare(const LinePoint& x) const
{
	if (get_x() < x.get_x())
		return -1;
	if (x.get_x() < get_x())
		return 1;
	if (get_y() < x.get_y())
		return -1;
	if (x.get_y() < get_y())
		return 1;
	if (is_dir() < x.is_dir())
		return -1;
	if (x.is_dir() < is_dir())
		return 1;
	return 0;
}

MeteoProfile::CloudIcons::CloudIcons(const std::string& basefile, const Cairo::RefPtr<Cairo::Surface> sfc)
	: m_ok(true)
{
	for (unsigned int i = 0; i < 4; ++i) {
		std::string fname;
		{
			std::ostringstream fn;
			fn << basefile << ((i + 1) * 25) << ".svg";
			fname = Glib::build_filename(PACKAGE_DATA_DIR, "gramet", fn.str());
		}
		GError *err(0);
		RsvgHandle *h(rsvg_handle_new_from_file(fname.c_str(), &err));
		if (!h) {
			std::cerr << "Cannot open " << fname;
			if (err)
				std::cerr << ": " << err->message;
			std::cerr << std::endl;
			g_error_free(err);
			err = 0;
			m_ok = false;
			break;
		}
		RsvgDimensionData dim;
		rsvg_handle_get_dimensions(h, &dim);
		if (true)
			std::cerr << "Icon " << fname << " WxH " << dim.width << ' ' << dim.height << std::endl;
		m_icons[i] = Cairo::Surface::create(sfc, Cairo::CONTENT_COLOR_ALPHA,
						    dim.width, dim.height);
		Cairo::RefPtr<Cairo::Context> ctx(Cairo::Context::create(m_icons[i]));
		m_ok = rsvg_handle_render_cairo(h, ctx->cobj());
		g_object_unref(G_OBJECT(h));
		if (!m_ok) {
			std::cerr << "Cannot render " << fname << std::endl;
			break;
		}
	}
	if (m_ok)
		return;
	for (unsigned int i = 0; i < 4; ++i)
		m_icons[i].clear();
}

Cairo::RefPtr<Cairo::Pattern> MeteoProfile::CloudIcons::operator[](unsigned int i) const
{
	Cairo::RefPtr<Cairo::SurfacePattern> p;
	if (!*this || i >= 4 || !m_icons[i])
		return p;
	if (false) {
		double cover((i + 1) * 0.25);
		double gray(1.0 - 0.25 * cover);
		return Cairo::SolidPattern::create_rgba(gray, gray, gray, 0.5 + 2.0 * cover);
	}
	p = Cairo::SurfacePattern::create(m_icons[i]);
	p->set_extend(Cairo::EXTEND_REPEAT);
	p->set_filter(Cairo::FILTER_GOOD);
	return p;
}

MeteoProfile::ConvCloudIcons::Icon::Icon(const Cairo::RefPtr<Cairo::Surface>& sfc, double width, double height)
	: m_surface(sfc), m_width(width), m_height(height), m_area(0)
{
	if (false && sfc) {
		std::cerr << sfc->get_type() << " surface" << std::endl;
		Cairo::RefPtr<Cairo::ImageSurface> imgsfc(Cairo::RefPtr<Cairo::ImageSurface>::cast_dynamic(sfc));
		if (imgsfc) {
			std::cerr << "Image surface format " << imgsfc->get_format() << std::endl;
		}
	}
	if (sfc && width > 1 && height > 1) {
		Cairo::RefPtr<Cairo::ImageSurface> imgsfc(Cairo::ImageSurface::create(Cairo::FORMAT_A8, std::ceil(width), std::ceil(height)));
		{
			Cairo::RefPtr<Cairo::Context> ctx(Cairo::Context::create(imgsfc));
			Cairo::RefPtr<Cairo::SurfacePattern> p(Cairo::SurfacePattern::create(sfc));
			p->set_extend(Cairo::EXTEND_NONE);
			p->set_filter(Cairo::FILTER_GOOD);
			ctx->set_source(p);
			ctx->paint();
		}
		unsigned int width1(imgsfc->get_width()), height1(imgsfc->get_height()), stride1(imgsfc->get_stride());
		const unsigned char *data1(imgsfc->get_data());
		uint64_t area1(0);
		for (unsigned int y = 0; y < height1; ++y) {
			const unsigned char *data2(data1 + y * stride1);
			for (unsigned int x = 0; x < width1; ++x)
				area1 += *data2++;
		}
		m_area = area1 * (1.0 / 255);
		if (false)
			std::cerr << "Tile area " << width * height << " area " << m_area << std::endl;
	}
}

MeteoProfile::ConvCloudIcons::ConvCloudIcons(const std::string& basefile, const Cairo::RefPtr<Cairo::Surface> sfc)
{
	for (unsigned int i = 1; i <= 1024; ++i) {
		std::string fname;
		{
			std::ostringstream fn;
			fn << basefile << i << ".svg";
			fname = Glib::build_filename(PACKAGE_DATA_DIR, "gramet", fn.str());
		}
		if (!Glib::file_test(fname, Glib::FILE_TEST_EXISTS) || !Glib::file_test(fname, Glib::FILE_TEST_IS_REGULAR))
			break;
		GError *err(0);
		RsvgHandle *h(rsvg_handle_new_from_file(fname.c_str(), &err));
		if (!h) {
			std::cerr << "Cannot open " << fname;
			if (err)
				std::cerr << ": " << err->message;
			std::cerr << std::endl;
			g_error_free(err);
			err = 0;
			m_icons.clear();
			break;
		}
		RsvgDimensionData dim;
		rsvg_handle_get_dimensions(h, &dim);
		if (true)
			std::cerr << "Icon " << fname << " WxH " << dim.width << ' ' << dim.height << std::endl;
		Cairo::RefPtr<Cairo::Surface>  sfc1(Cairo::Surface::create(sfc, Cairo::CONTENT_COLOR_ALPHA,
									   dim.width, dim.height));
		Cairo::RefPtr<Cairo::Context> ctx(Cairo::Context::create(sfc1));
		bool ok(rsvg_handle_render_cairo(h, ctx->cobj()));
		g_object_unref(G_OBJECT(h));
		if (!ok) {
			m_icons.clear();
			std::cerr << "Cannot render " << fname << std::endl;
			break;
		}
		m_icons.insert(Icon(sfc1, dim.width, dim.height));
	}
}

Cairo::RefPtr<Cairo::Pattern> MeteoProfile::ConvCloudIcons::get_cloud(double& width, double& height, double& area, int32_t top, int32_t tropopause) const
{
	if (top <= 0 || tropopause <= 0) {
		icons_t::const_iterator i(m_icons.lower_bound(Icon(Cairo::RefPtr<Cairo::Surface>(), 0, height)));
		if (i != m_icons.begin()) {
			icons_t::const_iterator i0(i);
			--i0;
			if (i == m_icons.end() || height - i0->get_height() < i->get_height() - height)
				i = i0;
		}
		if (i == m_icons.end()) {
			width = height = area = std::numeric_limits<double>::quiet_NaN();
			return Cairo::RefPtr<Cairo::Pattern>();
		}
		width = i->get_width();
		height = i->get_height();
		area = i->get_area();
		Cairo::RefPtr<Cairo::SurfacePattern> p(Cairo::SurfacePattern::create(i->get_surface()));
		p->set_extend(Cairo::EXTEND_NONE);
		p->set_filter(Cairo::FILTER_GOOD);
		return p;
	}
	icons_t::size_type n(m_icons.size());
	width = height = area = std::numeric_limits<double>::quiet_NaN();
	if (!n)
		return Cairo::RefPtr<Cairo::Pattern>();
	icons_t::size_type j(std::min((icons_t::size_type)((10 * top) / (8 * tropopause)), n - 1));
	icons_t::const_iterator i(m_icons.begin());
	while (j > 0) {
		icons_t::const_iterator i1(i);
		++i1;
		if (i1 == m_icons.end())
			break;
		i = i1;
		--j;
	}
	width = i->get_width();
	height = i->get_height();
	area = i->get_area();
	Cairo::RefPtr<Cairo::SurfacePattern> p(Cairo::SurfacePattern::create(i->get_surface()));
	p->set_extend(Cairo::EXTEND_NONE);
	p->set_filter(Cairo::FILTER_GOOD);
	return p;
}

MeteoProfile::StratusCloudBlueNoise::Group::Group(const Cairo::RefPtr<Cairo::Surface>& sfc,
						  const Cairo::RefPtr<Cairo::Surface>& alphasfc,
						  unsigned int nr, double x, double y)
	: m_surface(sfc), m_alphasurface(alphasfc), m_x(x), m_y(y), m_nr(nr)
{
	for (unsigned int idx = 0; idx < 16; ++idx)
		m_density[idx] = std::numeric_limits<double>::quiet_NaN();
}

Cairo::RefPtr<Cairo::Pattern> MeteoProfile::StratusCloudBlueNoise::Group::get_pattern(void) const
{
	Cairo::RefPtr<Cairo::SurfacePattern> p(Cairo::SurfacePattern::create(get_surface()));
	p->set_extend(Cairo::EXTEND_NONE);
	p->set_filter(Cairo::FILTER_GOOD);
	return p;
}

Cairo::RefPtr<Cairo::Pattern> MeteoProfile::StratusCloudBlueNoise::Group::get_alphapattern(void) const
{
	Cairo::RefPtr<Cairo::SurfacePattern> p(Cairo::SurfacePattern::create(get_alphasurface()));
	p->set_extend(Cairo::EXTEND_NONE);
	p->set_filter(Cairo::FILTER_GOOD);
	return p;
}

int MeteoProfile::StratusCloudBlueNoise::Group::compare(const Group& x) const
{
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

MeteoProfile::StratusCloudBlueNoise::StratusCloudBlueNoise(const std::string& filename, const Cairo::RefPtr<Cairo::Surface> sfc)
	: m_width(0), m_height(0), m_x(0), m_y(0)
{
	// load cloud file
	{
		std::string fname(Glib::build_filename(PACKAGE_DATA_DIR, "gramet", filename));
		GError *err(0);
		RsvgHandle *h(rsvg_handle_new_from_file(fname.c_str(), &err));
		if (!h) {
			std::cerr << "Cannot open " << fname;
			if (err)
				std::cerr << ": " << err->message;
			std::cerr << std::endl;
			g_error_free(err);
			err = 0;
			return;
		}
		RsvgDimensionData dim;
		rsvg_handle_get_dimensions(h, &dim);
		if (false)
			std::cerr << "Stratus Icon " << fname << " WxH " << dim.width << ' ' << dim.height << std::endl;
		RsvgDimensionData dimtile;
		RsvgPositionData postile;
		if (!rsvg_handle_get_dimensions_sub(h, &dimtile, "#tilearea") ||
		    !rsvg_handle_get_position_sub(h, &postile, "#tilearea")) {
			g_object_unref(G_OBJECT(h));
			return;
		}
		if (true)
			std::cerr << "Stratus Icon " << fname << " WxH " << dim.width << ' ' << dim.height
				  << " Tile WxH " << dimtile.width << ' ' << dimtile.height
				  << " XxY " << postile.x << ' ' << postile.y << std::endl;
		for (unsigned int gnr = 0; gnr < 10000; ++gnr) {
			std::ostringstream gnrstr;
			gnrstr << "#g" << std::setw(3) << std::setfill('0') << gnr;
			if (!rsvg_handle_has_sub(h, gnrstr.str().c_str()))
				break;
			Cairo::RefPtr<Cairo::Surface> sfcg(Cairo::Surface::create(sfc, Cairo::CONTENT_COLOR_ALPHA,
										  dim.width, dim.height));
			Cairo::RefPtr<Cairo::Context> ctx(Cairo::Context::create(sfcg));
			if (!rsvg_handle_render_cairo_sub(h, ctx->cobj(), gnrstr.str().c_str()))
				break;
			Cairo::RefPtr<Cairo::ImageSurface> alpha(Cairo::ImageSurface::create(Cairo::FORMAT_A8, dim.width, dim.height));
			ctx = Cairo::Context::create(alpha);
			Cairo::RefPtr<Cairo::SurfacePattern> p(Cairo::SurfacePattern::create(sfcg));
			p->set_extend(Cairo::EXTEND_NONE);
			p->set_filter(Cairo::FILTER_GOOD);
			ctx->set_source(p);
			ctx->paint();
			double mx, my;
			MeteoProfile::center_of_mass(mx, my, alpha);
			if (std::isnan(mx) || std::isnan(my))
				break;
			if (false)
				std::cerr << "Group " << gnrstr.str() << " center of gravity x " << mx << " y " << my << std::endl;
			if (false)
				std::cerr << "sfcg type " << sfcg->get_type() << " alpha type " << alpha->get_type() << std::endl;
			m_groups.push_back(Group(sfcg, alpha, gnr, mx, my));
		}
		g_object_unref(G_OBJECT(h));
		if (m_groups.empty())
			return;
		m_width = dimtile.width;
		m_height = dimtile.height;
		m_x = postile.x;
		m_y = postile.y;
	}
	// calculate blue noise masks
	for (unsigned int idx = 0; idx < 16; ++idx) {
		Cairo::RefPtr<Cairo::ImageSurface> sfc(Cairo::ImageSurface::create(Cairo::FORMAT_A8, 2 * get_width(), 2 * get_height()));
		Cairo::RefPtr<Cairo::Context> ctx(Cairo::Context::create(sfc));
		std::random_shuffle(m_groups.begin(), m_groups.end());
		int stride(sfc->get_stride());
		const uint8_t *data(sfc->get_data());
		double invmaxdensity(1.0 / (255.0 * get_width() * get_height()));
		for (groups_t::iterator gi(m_groups.begin()), ge(m_groups.end()); gi != ge; ++gi) {
			for (int x = 0; x < 2; ++x)
				for (int y = 0; y < 2; ++y) {
					ctx->save();
					ctx->translate(-x * get_width(), -y * get_height());
					ctx->set_source(gi->get_alphapattern());
					ctx->paint();
					ctx->restore();
				}
			sfc->flush();
			// compute density
			uint64_t dsum(0);
			for (int y = 0; y < get_height(); ++y) {
				const uint8_t *d(data + y * stride);
				for (int x = 0; x < get_width(); ++x)
					dsum += d[x];
			}
			double d(dsum * invmaxdensity);
			gi->set_density(idx, d);
			if (false)
				std::cerr << "Group " << gi->get_nr() << " x " << gi->get_x() << " y " << gi->get_y()
					  << " d " << gi->get_density(idx) << std::endl;
		}
	}
	std::sort(m_groups.begin(), m_groups.end());
}

MeteoProfile::MeteoProfile(const FPlanRoute& route, const TopoDb30::RouteProfile& routeprofile,
			   const GRIB2::WeatherProfile& wxprofile, const DriftDownProfile& ddprofile)
	: m_route(route), m_routeprofile(routeprofile), m_wxprofile(wxprofile), m_ddprofile(ddprofile)
{
}

void MeteoProfile::extract_remainder(std::vector<LinePoint>& ln, std::multiset<LinePoint>& lp, uint32_t maxydiff)
{
	bool dir(ln.back().is_dir());
	for (;;) {
		uint32_t x(ln.back().get_x() + 1);
		int32_t y(ln.back().get_y());
		std::multiset<LinePoint>::iterator li(lp.lower_bound(LinePoint(x, std::numeric_limits<int32_t>::min(), false)));
		std::multiset<LinePoint>::iterator le(lp.end());
		std::multiset<LinePoint>::iterator li2;
		bool li2set(false);
		uint32_t diff(std::numeric_limits<uint32_t>::max());
		for (; li != le && li->get_x() == x; ++li) {
			if (li->is_dir() != dir)
				continue;
			uint32_t d(abs(li->get_y() - y));
			if (d >= diff && li2set)
				continue;
			diff = d;
			li2set = true;
			li2 = li;
		}
		if (!li2set || diff > maxydiff)
			return;
		ln.push_back(*li2);
		lp.erase(li2);
	}
}

std::vector<MeteoProfile::LinePoint> MeteoProfile::extract_line(std::multiset<LinePoint>& lp, uint32_t maxydiff)
{
	for (;;) {
		if (lp.empty())
			return std::vector<LinePoint>();
		std::vector<LinePoint> ln;
		ln.push_back(*lp.begin());
		lp.erase(lp.begin());
		extract_remainder(ln, lp, maxydiff);
		return ln;
	}
}

std::vector<MeteoProfile::LinePoint> MeteoProfile::extract_area(std::multiset<LinePoint>& lp, uint32_t maxydiff)
{
	for (;;) {
		if (lp.empty())
			return std::vector<LinePoint>();
		std::vector<LinePoint> ln1;
		std::vector<LinePoint> ln2;
		{
			std::multiset<LinePoint>::iterator p2(lp.begin()), pe(lp.end());
			uint32_t x(p2->get_x());
			while (p2 != pe) {
				std::multiset<LinePoint>::iterator p1(p2);
				++p2;
				if (p2 == pe)
					break;
				if (p2->get_x() != x)
					break;
				if (p1->is_dir() == p2->is_dir())
					continue;
				ln1.push_back(*p1);
				ln2.push_back(*p2);
				lp.erase(p1);
				lp.erase(p2);
				break;
			}
			if (ln1.empty() || ln2.empty()) {
				lp.erase(lp.begin(), p2);
				continue;
			}
		}
		if (ln1.empty() || ln2.empty())
			continue;
		extract_remainder(ln1, lp, maxydiff);
		extract_remainder(ln2, lp, maxydiff);
		ln1.insert(ln1.end(), ln2.rbegin(), ln2.rend());
		return ln1;
	}
}

void MeteoProfile::draw_clouds_old(const Cairo::RefPtr<Cairo::Context>& cr, const std::vector<CloudPoint>& cldpt,
				   yaxis_t yaxis)
{
	std::vector<CloudPoint>::const_iterator ci(cldpt.begin()), ce(cldpt.end());
	if (ci == ce)
		return;
	for (;;) {
		std::vector<CloudPoint>::const_iterator cip(ci);
		++ci;
		if (ci == ce)
			break;
		cr->move_to(cip->get_dist(), yconvert(cip->get_base(), yaxis));
		cr->line_to(cip->get_dist(), yconvert(cip->get_top(), yaxis));
		cr->line_to(ci->get_dist(), yconvert(ci->get_top(), yaxis));
		cr->line_to(ci->get_dist(), yconvert(ci->get_base(), yaxis));
		cr->close_path();
		set_color_cloud(cr, cip->get_cover());
		cr->fill();
	}
}

void MeteoProfile::draw_clouds(const Cairo::RefPtr<Cairo::Context>& cr, const std::vector<CloudPoint>& cldpt,
			       yaxis_t yaxis, bool preservepath)
{
	if (cldpt.size() < 2)
		return;
	double invdist(cldpt.back().get_dist() - cldpt.front().get_dist());
	if (invdist <= 0)
		return;
	invdist = 1.0 / invdist;
	{
		std::vector<CloudPoint>::const_iterator ci(cldpt.begin()), ce(cldpt.end());
		cr->move_to(ci->get_dist(), yconvert(ci->get_base(), yaxis));
		for (++ci; ci != ce; ++ci)
			cr->line_to(ci->get_dist(), yconvert(ci->get_base(), yaxis));
	}
	for (std::vector<CloudPoint>::const_reverse_iterator ci(cldpt.rbegin()), ce(cldpt.rend()); ci != ce; ++ci)
		cr->line_to(ci->get_dist(), yconvert(ci->get_top(), yaxis));
	cr->close_path();
	Cairo::RefPtr<Cairo::LinearGradient> g(Cairo::LinearGradient::create(0, cldpt.front().get_dist(), 0, cldpt.back().get_dist()));
	for (std::vector<CloudPoint>::const_iterator ci(cldpt.begin()), ce(cldpt.end()); ci != ce; ++ci)
		set_color_cloud(g, (ci->get_dist() - cldpt.front().get_dist()) * invdist, ci->get_cover());
	cr->set_source(g);
	if (preservepath)
		cr->fill_preserve();
	else
		cr->fill();
}

void MeteoProfile::draw_clouds(const Cairo::RefPtr<Cairo::Context>& cr, const std::vector<CloudPoint>& cldpt,
			       yaxis_t yaxis, const CloudIcons& icons, double scale)
{
	static const double blursigma2 = 30;

	if (cldpt.size() < 2)
		return;
	double invdist(cldpt.back().get_dist() - cldpt.front().get_dist());
	if (invdist <= 0)
		return;
	invdist = 1.0 / invdist;
	int width(0), height(0);
	{
		double x0, y0, x1, y1;
		cr->get_clip_extents(x0, y0, x1, y1);
		cr->user_to_device(x0, y0);
		cr->user_to_device(x1, y1);
		if (false)
			std::cerr << "clip extents: " << x0 << ' ' << y0 << ' ' << x1 << ' ' << y1 << std::endl;
		width = std::ceil(std::max(x0, x1));
		height = std::ceil(std::max(y0, y1));
	}
	if (std::isnan(blursigma2) || blursigma2 <= 0) {
		for (unsigned int i = 0; i < 4; ++i) {
			double meandensity((i + 1) * (1.0 / 4));
			// create surface to serve as alpha channel for the mask operation
			Cairo::RefPtr<Cairo::Surface> sfcm(Cairo::Surface::create(cr->get_target(), Cairo::CONTENT_ALPHA, width, height));
			Cairo::RefPtr<Cairo::Context> crm(Cairo::Context::create(sfcm));
			crm->set_matrix(cr->get_matrix());
			// clip the mask channel surface to the cloud dimensions
			{
				std::vector<CloudPoint>::const_iterator ci(cldpt.begin()), ce(cldpt.end());
				crm->move_to(ci->get_dist(), yconvert(ci->get_base(), yaxis));
				for (++ci; ci != ce; ++ci)
					crm->line_to(ci->get_dist(), yconvert(ci->get_base(), yaxis));
			}
			for (std::vector<CloudPoint>::const_reverse_iterator ci(cldpt.rbegin()), ce(cldpt.rend()); ci != ce; ++ci)
				crm->line_to(ci->get_dist(), yconvert(ci->get_top(), yaxis));
			crm->close_path();
			crm->clip();
			// make linear gradient that records cloud density blending
			Cairo::RefPtr<Cairo::LinearGradient> g(Cairo::LinearGradient::create(0, cldpt.front().get_dist(), 0, cldpt.back().get_dist()));
			for (std::vector<CloudPoint>::const_iterator ci(cldpt.begin()), ce(cldpt.end()); ci != ce; ++ci)
				g->add_color_stop_rgba((ci->get_dist() - cldpt.front().get_dist()) * invdist,
						       1., 1., 1., std::max(0., 1. - fabs(ci->get_cover() - meandensity) * 4.));
			crm->set_source(g);
			crm->paint();
			crm.clear();
			// blur alpha channel
			if (true) {
				// convert mask surface to an image surface (A8)
				Cairo::RefPtr<Cairo::ImageSurface> imgsfc(Cairo::ImageSurface::create(Cairo::FORMAT_A8, width, height));
				{
					Cairo::RefPtr<Cairo::Context> ctx(Cairo::Context::create(imgsfc));
					Cairo::RefPtr<Cairo::SurfacePattern> p(Cairo::SurfacePattern::create(sfcm));
					p->set_extend(Cairo::EXTEND_NONE);
					p->set_filter(Cairo::FILTER_GOOD);
					ctx->set_source(p);
					ctx->paint();
				}
				// gaussian blur over the image surface
				gaussian_blur(imgsfc, 15.0);
				sfcm = imgsfc;
			}
			if (false) {
				std::cerr << sfcm->get_type() << " surface" << std::endl;
				Cairo::RefPtr<Cairo::ImageSurface> imgsfc(Cairo::RefPtr<Cairo::ImageSurface>::cast_dynamic(sfcm));
				if (imgsfc) {
					std::cerr << "Image surface format " << imgsfc->get_format() << std::endl;
				}
			}
			{
				cr->save();
				Cairo::Matrix tm;
				cr->get_matrix(tm);
				cr->set_identity_matrix();
				cr->scale(scale, scale);
				cr->set_source(icons[i]);
				cr->set_identity_matrix();
				cr->mask(sfcm, 0., 0.);
				cr->set_matrix(tm);
				cr->restore();
			}
		}
		return;
	}
	// create master alpha channel
	Cairo::RefPtr<Cairo::ImageSurface> masteralpha(Cairo::ImageSurface::create(Cairo::FORMAT_A8, width, height));
	{
		Cairo::RefPtr<Cairo::Context> crm(Cairo::Context::create(masteralpha));
		crm->set_matrix(cr->get_matrix());
		// clip the mask channel surface to the cloud dimensions
		{
			std::vector<CloudPoint>::const_iterator ci(cldpt.begin()), ce(cldpt.end());
			crm->move_to(ci->get_dist(), yconvert(ci->get_base(), yaxis));
			for (++ci; ci != ce; ++ci)
				crm->line_to(ci->get_dist(), yconvert(ci->get_base(), yaxis));
		}
		for (std::vector<CloudPoint>::const_reverse_iterator ci(cldpt.rbegin()), ce(cldpt.rend()); ci != ce; ++ci)
			crm->line_to(ci->get_dist(), yconvert(ci->get_top(), yaxis));
		crm->close_path();
		crm->clip();
		// make linear gradient that records cloud density blending
		Cairo::RefPtr<Cairo::LinearGradient> g(Cairo::LinearGradient::create(0, cldpt.front().get_dist(), 0, cldpt.back().get_dist()));
		for (std::vector<CloudPoint>::const_iterator ci(cldpt.begin()), ce(cldpt.end()); ci != ce; ++ci)
			g->add_color_stop_rgba((ci->get_dist() - cldpt.front().get_dist()) * invdist, 1., 1., 1., ci->get_cover());
		crm->set_source(g);
		crm->paint();
	}
	// gaussian blur over the image surface
	gaussian_blur(masteralpha, blursigma2);
	// check whether we have any significant cloud
	{
		masteralpha->flush();
		int mastride(masteralpha->get_stride());
		const uint8_t *madata(masteralpha->get_data());
		bool alltr(true);
		for (int y = 0; y < height && alltr; ++y) {
				const uint8_t *d(madata + y * mastride);
				for (int x = 0; x < width && alltr; ++x)
					alltr = alltr && (d[x] < 8);
		}
		if (alltr)
			return;
	}
	// paint textures
	for (unsigned int i = 0; i < 4; ++i) {
		masteralpha->flush();
		Cairo::RefPtr<Cairo::ImageSurface> texturealpha(Cairo::ImageSurface::create(Cairo::FORMAT_A8, width, height));
		texturealpha->flush();
		bool nontr(false);
        	{
			int meanalpha((i << 6) | 0x3f);
			int src_stride(masteralpha->get_stride());
			int dst_stride(texturealpha->get_stride());
			const uint8_t *src(masteralpha->get_data());
			uint8_t *dst(texturealpha->get_data());
			for (int y = 0; y < height; ++y) {
				const uint8_t *s(src + y * src_stride);
				uint8_t *d(dst + y * dst_stride);
				for (int x = 0; x < width; ++x) {
					uint8_t w(abs(s[x] - meanalpha));
					if (w > 0x3f)
						w = 0x3f;
					w <<= 2;
					w = 255 - w;
					if (w < 4)
						w = 0;
					else
						nontr = true;
					d[x] = w;
				}
			}
		}
		texturealpha->mark_dirty();
		if (!nontr)
			continue;
		cr->save();
		Cairo::Matrix tm;
		cr->get_matrix(tm);
		cr->set_identity_matrix();
		cr->scale(scale, scale);
		cr->set_source(icons[i]);
		cr->set_identity_matrix();
		cr->mask(texturealpha, 0., 0.);
		cr->set_matrix(tm);
		cr->restore();
	}
}

void MeteoProfile::draw_clouds(const Cairo::RefPtr<Cairo::Context>& cr, const std::vector<CloudPoint>& cldpt,
			       yaxis_t yaxis, const StratusCloudBlueNoise& icon, double scale)
{
	if (cldpt.size() < 2)
		return;
	double invdist(cldpt.back().get_dist() - cldpt.front().get_dist());
	if (invdist <= 0)
		return;
	invdist = 1.0 / invdist;
	int width(0), height(0);
	{
		double x0, y0, x1, y1;
		cr->get_clip_extents(x0, y0, x1, y1);
		cr->user_to_device(x0, y0);
		cr->user_to_device(x1, y1);
		if (false)
			std::cerr << "clip extents: " << x0 << ' ' << y0 << ' ' << x1 << ' ' << y1 << std::endl;
		width = std::ceil(std::max(x0, x1));
		height = std::ceil(std::max(y0, y1));
	}
	// create master alpha channel
	Cairo::RefPtr<Cairo::ImageSurface> masteralpha(Cairo::ImageSurface::create(Cairo::FORMAT_A8, width, height));
	{
		Cairo::RefPtr<Cairo::Context> crm(Cairo::Context::create(masteralpha));
		crm->set_matrix(cr->get_matrix());
		// clip the mask channel surface to the cloud dimensions
		{
			std::vector<CloudPoint>::const_iterator ci(cldpt.begin()), ce(cldpt.end());
			crm->move_to(ci->get_dist(), yconvert(ci->get_base(), yaxis));
			for (++ci; ci != ce; ++ci)
				crm->line_to(ci->get_dist(), yconvert(ci->get_base(), yaxis));
		}
		for (std::vector<CloudPoint>::const_reverse_iterator ci(cldpt.rbegin()), ce(cldpt.rend()); ci != ce; ++ci)
			crm->line_to(ci->get_dist(), yconvert(ci->get_top(), yaxis));
		crm->close_path();
		crm->clip();
		// make linear gradient that records cloud density blending
		Cairo::RefPtr<Cairo::LinearGradient> g(Cairo::LinearGradient::create(cldpt.front().get_dist(), 0, cldpt.back().get_dist(), 0));
		if (false)
			std::cerr << "draw_clouds: blue noise " << cldpt.front().get_dist() << "..." << cldpt.back().get_dist() << std::endl;
		for (std::vector<CloudPoint>::const_iterator ci(cldpt.begin()), ce(cldpt.end()); ci != ce; ++ci) {
			g->add_color_stop_rgba((ci->get_dist() - cldpt.front().get_dist()) * invdist, 1., 1., 1., ci->get_cover());
			if (false)
				std::cerr << "draw_clouds: blue noise col stop " << ((ci->get_dist() - cldpt.front().get_dist()) * invdist)
					  << " cover " << ci->get_cover() << std::endl;
		}
		crm->set_source(g);
		crm->paint();
	}
	masteralpha->flush();
	if (false) {
		static unsigned int bnmaskcount = 0;
		std::ostringstream oss;
		oss << "/tmp/bluenoisemask" << (bnmaskcount++) << ".png";
		masteralpha->write_to_png(oss.str());
		std::cerr << "draw_clouds: blue noise mask " << oss.str() << std::endl;
	}
	cr->save();
	cr->set_identity_matrix();
	cr->scale(scale, scale);
	{
		int mastride(masteralpha->get_stride());
		const uint8_t *madata(masteralpha->get_data());
		if (false) {
			for (int y = 0; y < height; ++y) {
				const uint8_t *d(madata + y * mastride);
				std::cerr << "master alpha y=" << y << ':';
				for (int x = 0; x < width; ++x)
					std::cerr << ' ' << (unsigned int)d[x];
				std::cerr << std::endl;
			}
		}
		int xx(-icon.get_width()), yy(-icon.get_height());
		for (;;) {
			{
				double xxd(xx), yyd(yy);
				cr->user_to_device(xxd, yyd);
				if (yyd >= height)
					break;
				if (xxd >= width) {
					xx = -icon.get_width();
					yy += icon.get_height();
					continue;
				}
			}
			unsigned int didx(rand() & 15);
			cr->save();
			cr->translate(xx, yy);
			for (unsigned int i(0), n(icon.size()); i < n; ++i) {
				const StratusCloudBlueNoise::Group& g(icon[i]);
				{
					double xg(g.get_x()), yg(g.get_y());
					cr->user_to_device(xg, yg);
					int gx(Point::round<int,double>(xg)), gy(Point::round<int,double>(yg));
					if (gx < 0 || gx >= width || gy < 0 || gy >= height)
						continue;
					if (false)
						std::cerr << "Group " << g.get_nr() << " x " << (xx + g.get_x())
							  << " y " << (yy + g.get_y()) << " xd " << gx << " yd " << gy
							  << " d " << (255.0 * g.get_density(didx))
							  << " actual " << (unsigned int)madata[gy * mastride + gx] << std::endl;
					if (madata[gy * mastride + gx] < 255.0 * g.get_density(didx))
						continue;
				}
				cr->set_source(g.get_pattern());
				cr->paint();
			}
			cr->restore();
			xx += icon.get_width();
		}
	}
	cr->restore();
}

void MeteoProfile::draw_clouds(const Cairo::RefPtr<Cairo::Context>& cr, const std::vector<CloudPoint>& cldpt,
			       yaxis_t yaxis, const ConvCloudIcons& icons, double scale, double aspectratio, int32_t tropopause)
{
	double areacount(0);
	for (std::vector<CloudPoint>::const_iterator ci(cldpt.begin()), ce(cldpt.end()); ci != ce; ++ci) {
		double ytop(yconvert(ci->get_top(), yaxis));
		double ybase(yconvert(ci->get_base(), yaxis));
		double x0(ci->get_dist()), y0(ytop), x1, y1(ybase);
		cr->user_to_device(x0, y0);
		{
			std::vector<CloudPoint>::const_iterator ci1(ci);
			++ci1;
			if (ci1 == ce)
				break;
			x1 = ci1->get_dist();
		}
		cr->user_to_device(x1, y1);
		double heightr(y1 - y0);
		double cldarea((x1 - x0) * heightr * ci->get_cover());
		if (false)
			std::cerr << "heightr " << heightr << " distr " << (x1 - x0) << " cover " << ci->get_cover()
				  << " cldarea " << cldarea << " areacount " << areacount << std::endl;
		areacount += cldarea;
		if (areacount < 0 || heightr <= 1)
			continue;
		double height(heightr / (scale * aspectratio)), width(0), area(0);
		Cairo::RefPtr<Cairo::Pattern> pat(icons.get_cloud(width, height, area, ci->get_top(), tropopause));
		height *= scale * aspectratio;
		width *= scale;
		area *= scale * scale * aspectratio;
		if (false)
			std::cerr << "cloud size requested: " << heightr << " returned: " << height << ' ' << width << ' ' << area
				  << " pos " << x0 << ' ' << y0 << ' ' << x1 << ' ' << y1 << " cover " << ci->get_cover() << std::endl;
		if (height <= 1 || !pat)
			continue;
		areacount -= area;
		cr->save();
		cr->set_identity_matrix();
		cr->translate(x0, y0);
		cr->scale(scale, scale * aspectratio * heightr / height);
		cr->set_source(pat);
		cr->paint();
		cr->restore();
	}
}

void MeteoProfile::gaussian_blur(const Cairo::RefPtr<Cairo::ImageSurface>& sfc, double sigma2)
{
	if (!sfc || sfc->get_format() != Cairo::FORMAT_A8)
		return;
	sfc->flush();
	int width(sfc->get_width());
	int height(sfc->get_height());
	int src_stride(sfc->get_stride());
	uint8_t *src(sfc->get_data());
	Cairo::RefPtr<Cairo::ImageSurface> sfc2(Cairo::ImageSurface::create(sfc->get_format(), width, height));
	int dst_stride(sfc2->get_stride());
	uint8_t *dst(sfc2->get_data());
	// compute kernel
	uint8_t kernel[31];
	const int ksize = sizeof(kernel) / sizeof(kernel[0]);
	const int khalf = (ksize - 1) / 2;
	uint32_t karea = 0;
	{
		double invsigma2(1.0 / sigma2);
		for (int i = 0; i < ksize; ++i) {
			double f(i - khalf);
			karea += kernel[i] = exp(-f * f * invsigma2) * 255.0;
		}
	}
	// horizontally blur sfc -> sfc2
	for (int i = 0; i < height; ++i) {
		const uint8_t *s = src + i * src_stride;
		uint8_t *d = dst + i * dst_stride;
		for (int j = 0; j < width; ++j) {
			uint32_t w = 0;
			for (int k = 0; k < ksize; ++k) {
				if (j - khalf + k < 0 || j - khalf + k >= width)
					continue;
				w += s[j - khalf + k] * kernel[k];
			}
			w += karea / 2;
			w /= karea;
			w = std::min(w, (uint32_t)255);
			d[j] = w;
		}
	}

	// vertically blur sfc2 -> sfc
	for (int i = 0; i < height; ++i) {
		uint8_t *d = src + i * src_stride;
		for (int j = 0; j < width; ++j) {
			uint32_t w = 0;
			for (int k = 0; k < ksize; ++k) {
				if (i - khalf + k < 0 || i - khalf + k >= height)
					continue;
				w += dst[(i - khalf + k) * dst_stride + j] * kernel[k];
			}
			w += karea / 2;
			w /= karea;
			w = std::min(w, (uint32_t)255);
			d[j] = w;
		}
	}
	sfc->mark_dirty();
}

void MeteoProfile::center_of_mass(double& x, double& y, const Cairo::RefPtr<Cairo::ImageSurface>& sfc)
{
	x = y = std::numeric_limits<double>::quiet_NaN();
	if (!sfc || sfc->get_format() != Cairo::FORMAT_A8)
		return;
	sfc->flush();
	int width(sfc->get_width());
	int height(sfc->get_height());
	int stride(sfc->get_stride());
	const uint8_t *d(sfc->get_data());
	uint64_t mx(0), my(0), m(0);
	for (int i = 0; i < height; ++i) {
		const uint8_t *dl = d + i * stride;
		for (int j = 0; j < width; ++j) {
			m += dl[j];
			mx += j * dl[j];
			my += i * dl[j];
		}
	}
	double im(m);
	im = 1.0 / im;
	x = im * mx;
	y = im * my;
}

double MeteoProfile::get_scaledist(const Cairo::RefPtr<Cairo::Context>& cr, int width, double dist) const
{
	if (dist <= 0 || std::isnan(dist))
		return 0;
	int alt_width;
	{
		cr->save();
		cr->select_font_face("sans", Cairo::FONT_SLANT_NORMAL, Cairo::FONT_WEIGHT_NORMAL);
		cr->set_font_size(12);
		Cairo::TextExtents ext;
		cr->get_text_extents("999", ext);
		alt_width = ext.width + 4;
		cr->restore();
	}
	return (width - alt_width) / dist;
}

double MeteoProfile::get_scaleelev(const Cairo::RefPtr<Cairo::Context>& cr, int height, double elev,
				   yaxis_t yaxis) const
{
	if (elev <= 0 || std::isnan(elev))
		return 0;
	elev = yconvert(elev, yaxis) - yconvert(0, yaxis);
	int dist_height;
	int dist_line_height;
	{
		cr->save();
		cr->select_font_face("sans", Cairo::FONT_SLANT_NORMAL, Cairo::FONT_WEIGHT_NORMAL);
		cr->set_font_size(12);
		Cairo::TextExtents ext;
		cr->get_text_extents("999", ext);
		dist_line_height = ext.height + 4;
		dist_height = 3 * dist_line_height + 5;
		cr->restore();
	}
	return (height - dist_height) / elev;
}

void MeteoProfile::errorpage(const Cairo::RefPtr<Cairo::Context>& cr, int width, int height, const std::string& errmsg) const
{
	cr->set_source_rgb(0.0, 0.0, 0.0);
	{
		Cairo::TextExtents ext;
		cr->get_text_extents(errmsg, ext);
		cr->move_to((width - ext.width) * 0.5 - ext.x_bearing, (height - ext.height) * 0.5 - ext.y_bearing);
		cr->show_text(errmsg);
	}
	if (true) {
		cr->set_font_size(8);
		Cairo::TextExtents ext;
		cr->get_text_extents("8888", ext);
		double y(ext.height * 4);
		for (unsigned int i(0), n(m_route.get_nrwpt()); i < n; ++i) {
			const FPlanWaypoint& wpt(m_route[i]);
			std::ostringstream oss;
			oss << '[' << i << '/' << n << "]: " << wpt.to_str()
			    << ' ' << std::setfill('0') << std::setw(3) << Point::round<int,float>(wpt.get_winddir_deg())
			    << '/' << std::setfill('0') << std::setw(2) << Point::round<int,float>(wpt.get_windspeed_kts())
			    << " ISA" << std::showpos << Point::round<int,float>(wpt.get_isaoffset_kelvin())
			    << " Q" << Point::round<int,float>(wpt.get_qff_hpa())
			    << " TA " << wpt.get_truealt();
			cr->move_to(ext.width, y);
			cr->show_text(oss.str());
			y += 1.4 * ext.height;
		}
	}
}

void MeteoProfile::draw(const Cairo::RefPtr<Cairo::Context>& cr, int width, int height,
			double origindist, double scaledist, double originelev, double scaleelev,
			yaxis_t yaxis, altflag_t altflags, const std::string& servicename) const
{
	cr->save();
	// background
	cr->set_source_rgb(1.0, 1.0, 1.0);
	cr->paint();
	// scaling
	cr->select_font_face("sans", Cairo::FONT_SLANT_NORMAL, Cairo::FONT_WEIGHT_NORMAL);
	cr->set_font_size(12);
	if (m_routeprofile.size() < 1) {
		errorpage(cr, width, height, "No Route Profile");
		cr->restore();
		return;
	}
	bool xaxistime(m_routeprofile.size() < 2 || m_routeprofile.front().get_routedist() >= m_routeprofile.back().get_routedist());
	if (m_route.get_nrwpt() < 2 - xaxistime) {
	        errorpage(cr, width, height, "No Flight Plan");
		cr->restore();
		return;
	}
	if (m_wxprofile.size() < 2) {
	        errorpage(cr, width, height, "No Weather Profile");
		cr->restore();
		return;
	}
	{
		unsigned int i(1), n(m_route.get_nrwpt());
		for (; i < n; ++i)
			if (m_route[i].get_flighttime() < m_route[i - 1].get_flighttime())
				break;
		if (i < n) {
			std::ostringstream oss;
			oss << "Nonmonotonous Route Time: waypoints " << (i-1) << ", " << i;
			errorpage(cr, width, height, oss.str());
			cr->restore();
			return;
		}
	}
	{
		gint64 tm0(m_route[0].get_time_unix());
		gint64 tm1(tm0 + m_route[m_route.get_nrwpt() - 1].get_flighttime());
		if (tm0 < m_wxprofile.get_minefftime() ||
		    tm1 > m_wxprofile.get_maxefftime()) {
			std::ostringstream oss;
			oss << "No Weather Data available for " << Glib::TimeVal(tm0, 0).as_iso8601()
			    << " to " << Glib::TimeVal(tm1, 0).as_iso8601();
			errorpage(cr, width, height, oss.str());
			cr->restore();
			return;
		}
	}
	int alt_width;
	int dist_height;
	int dist_line_height;
	double elevinc, elevstart;
	double distinc, diststart;
	{
		Cairo::TextExtents ext;
		cr->get_text_extents("999", ext);
		alt_width = ext.width + 4;
		dist_line_height = ext.height + 4;
		dist_height = 3 * dist_line_height + 5;
		elevinc = (scaleelev < 0) ? -1 : 1;
		for (unsigned int i = 0; i < 18; ++i) {
			if (elevinc * scaleelev >= 100)
				break;
			elevinc *= ((i % 3) == 1) ? 2.5 : 2;
		}
		elevstart = elevinc * ceil(originelev / elevinc);
		cr->get_text_extents("999", ext);
		distinc = 1;
		if (xaxistime) {
			static const double mul[] = {
				2,
				2.5,
				3,
				2,
				2,
				2,
				2.5,
				3,
				2,
				2,
				3,
				2,
				2,
				2,
				2,
				2,
				2
			};
			for (unsigned int i = 0; i < sizeof(mul)/sizeof(mul[0]); ++i) {
				if (distinc * scaledist >= 100)
					break;
				distinc *= mul[i];
			}
		} else {
			for (unsigned int i = 0; i < 12; ++i) {
				if (distinc * scaledist >= 100)
					break;
				distinc *= ((i % 3) == 1) ? 2.5 : 2;
			}
		}
		diststart = distinc * ceil(origindist / distinc);
	}
	// draw location labels
	if (!xaxistime) {
		// prioritize labels
		unsigned int prio[m_route.get_nrwpt()];
		for (unsigned int i = 0, n = m_route.get_nrwpt(); i < n; ++i) {
			if (!i || i + 1 >= n) {
				prio[i] = 0;
				continue;
			}
			bool routechg(i > 0 && (m_route[i - 1].get_pathcode() != m_route[i].get_pathcode() ||
						m_route[i - 1].get_pathname() != m_route[i].get_pathname()));
			bool rulechg(i > 0 && m_route[i - 1].is_ifr() != m_route[i].is_ifr());
			bool altchg(i + 1 < n && (m_route[i].get_altitude() != m_route[i + 1].get_altitude() ||
						  m_route[i].is_standard() != m_route[i + 1].is_standard()));
			bool turnpt(m_route[i].is_turnpoint());
			bool tocbod(m_route[i].get_type() >= FPlanWaypoint::type_generated_start && m_route[i].get_type() <= FPlanWaypoint::type_generated_end);
			prio[i] = 1 + (!turnpt) + (!(routechg || rulechg || altchg)) + (4*tocbod);
		}
		set_color_fplan(cr);
		typedef std::pair<double, double> lblext_t;
		typedef std::vector<lblext_t> lblexts_t;
		lblexts_t lblexts;
		for (unsigned int p = 0; p < 4; ++p) {
			for (unsigned int i = 0, n = m_route.get_nrwpt(); i < n; ++i) {
				if (prio[i] != p)
					continue;
				int x;
				{
					double dist(std::numeric_limits<double>::quiet_NaN());
					for (TopoDb30::RouteProfile::const_iterator pi(m_routeprofile.begin()), pe(m_routeprofile.end()); pi != pe; ++pi) {
						if (pi->get_routeindex() != i)
							continue;
						dist = pi->get_routedist();
						break;
					}
					if (std::isnan(dist))
						continue;
					x = alt_width + Point::round<int,double>((dist - origindist) * scaledist);
					if (false)
						std::cerr << "MeteoProfile::draw: wpt " << i << '/' << n << " dist " << dist
							  << " x " << x << " (" << alt_width << "..." << width << ')' << std::endl;
				}
				if (x < alt_width || x > width)
					continue;
				Glib::ustring txt(m_route[i].get_icao());
				if (txt.empty() || (m_route[i].get_type() == FPlanWaypoint::type_airport && AirportsDb::Airport::is_fpl_zzzz(txt)))
					txt = m_route[i].get_name();
				if (txt.empty())
					continue;
				Cairo::TextExtents ext;
				cr->get_text_extents(txt, ext);
				double lblx(x - ext.x_bearing);
				if (i + 1 >= n)
					lblx -= ext.width;
				else if (i)
					lblx -= ext.width / 2U;
				{
					lblext_t lble(lblx - 2, lblx + ext.width + 2);
					lblexts_t::const_iterator li(lblexts.begin()), le(lblexts.end());
					for (; li != le; ++li) {
						if (lble.second <= li->first ||
						    lble.first >= li->second)
							continue;
						break;
					}
					if (li != le)
						continue;
					lblexts.push_back(lble);
				}
				cr->move_to(lblx, height - 3U * dist_line_height + 2U - ext.y_bearing);
				cr->show_text(txt);
				cr->begin_new_path();
				cr->move_to(x - 2, height - 3 * dist_line_height);
				cr->rel_line_to(4, 0);
				cr->rel_line_to(-2, -4);
				cr->close_path();
				cr->fill();
			}
		}
	}
	// draw chart legend
	{
		static const char * const arrow[2] = { " \342\206\222 ", " -> " };
		static const char * const bullet[2] = { " \342\200\242 ", " * " };
		static const bool noutf8 = false;
		cr->set_source_rgb(0.0, 0.0, 0.0);
		std::ostringstream oss;
		if (m_route.get_nrwpt() > 0) {
			const FPlanWaypoint& wpt(m_route[0]);
			std::string dep;
			if (wpt.get_icao().empty() || (wpt.get_type() == FPlanWaypoint::type_airport && AirportsDb::Airport::is_fpl_zzzz(wpt.get_icao())))
				dep = wpt.get_name();
			else
				dep = wpt.get_icao();
			std::string tm0(Glib::TimeVal(wpt.get_time_unix(), 0).as_iso8601());
			oss << dep << ' ' << tm0.substr(0, 10) << ' ' << tm0.substr(11, 5) << 'Z';
			if (m_route.get_nrwpt() > 1) {
				const FPlanWaypoint& wpt(m_route[m_route.get_nrwpt() - 1]);
				oss << arrow[noutf8];
				std::string dest;
				if (wpt.get_icao().empty() || (wpt.get_type() == FPlanWaypoint::type_airport && AirportsDb::Airport::is_fpl_zzzz(wpt.get_icao())))
					dest = wpt.get_name();
				else
					dest = wpt.get_icao();
				std::string tm1(Glib::TimeVal(m_route[0].get_time_unix() + wpt.get_flighttime(), 0).as_iso8601());
				if (dep != dest)
					oss << dest << ' ';
				if (tm0.substr(0, 10) != tm1.substr(0, 10))
					oss << tm1.substr(0, 10) << ' ';
				oss << tm1.substr(11, 5) << 'Z';
			}
		}
		{
			std::string tm0(Glib::TimeVal(m_wxprofile.get_minreftime(), 0).as_iso8601());
			oss << bullet[noutf8] << "GFS RefTime " << tm0.substr(0, 10) << ' ' << tm0.substr(11, 5) << 'Z';
			if (m_wxprofile.get_maxreftime() != m_wxprofile.get_minreftime()) {
				std::string tm1(Glib::TimeVal(m_wxprofile.get_maxreftime(), 0).as_iso8601());
				oss << '-';
				if (tm0.substr(0, 10) != tm1.substr(0, 10))
					oss << tm1.substr(0, 10) << ' ';
				oss << tm1.substr(11, 5) << 'Z';
			}
		}
		std::ostringstream oss2;
		if (true) {
			std::string tm0(Glib::TimeVal(m_wxprofile.get_minefftime(), 0).as_iso8601());
			oss2 << bullet[noutf8] << "EffTime " << tm0.substr(0, 10) << ' ' << tm0.substr(11, 5) << 'Z';
			if (m_wxprofile.get_maxefftime() != m_wxprofile.get_minefftime()) {
				std::string tm1(Glib::TimeVal(m_wxprofile.get_maxefftime(), 0).as_iso8601());
				oss2 << '-';
				if (tm0.substr(0, 10) != tm1.substr(0, 10))
					oss2 << tm1.substr(0, 10) << ' ';
				oss2 << tm1.substr(11, 5) << 'Z';
			}
		}
		Cairo::TextExtents ext;
		cr->get_text_extents(oss.str() + oss2.str() + bullet[noutf8] + servicename, ext);
		if (ext.width <= width - alt_width)
			oss << oss2.str();
		oss << bullet[noutf8] << servicename;
		cr->get_text_extents(oss.str(), ext);
		cr->move_to(alt_width - ext.x_bearing, height - dist_line_height + 2U - ext.y_bearing);
		cr->show_text(oss.str());
		if (false) {
			std::string t(oss.str());
			std::ostringstream x;
			x << std::hex;
			for (std::string::const_iterator i(t.begin()), e(t.end()); i != e; ++i)
				x << ' ' << std::setw(2) << std::setfill('0') << (0xff & *i);
			std::cerr << "GRAMET: title line bearing x " << ext.x_bearing << " y " << ext.y_bearing
				  << " alt_width " << alt_width << " height " << height << " dist_line_height "
				  << dist_line_height << " (" << t.size() << ')' << x.str() << std::endl;
		}
	}
	// normalize and draw terrain
	cr->save();
	Cairo::Matrix mnorm;
	cr->get_matrix(mnorm);
	cr->rectangle(alt_width, 0, width - alt_width, height - dist_height);
	cr->clip();
	{
		double y0(height - dist_height - (yconvert(0, yaxis) - originelev) * scaleelev);
		double y1(height - dist_height - (yconvert(60000, yaxis) - originelev) * scaleelev);
		Cairo::RefPtr<Cairo::LinearGradient> g(Cairo::LinearGradient::create(0, y0, 0, y1));
		set_color_sky(g);
		cr->set_source(g);
		//set_color_sky(cr);
		cr->paint();
	}
	cr->set_line_width(2);
	cr->translate(alt_width, height - dist_height);
	cr->scale(scaledist, -scaleelev);
	cr->translate(-origindist, -originelev);
	Cairo::Matrix mscaled;
	cr->get_matrix(mscaled);
	// draw dusk/night/dawn
	{
		if (true) {
			int x(-1);
			for (GRIB2::WeatherProfile::const_iterator pi(m_wxprofile.begin()), pe(m_wxprofile.end()); pi != pe; ++pi) {
				int xx(pi->get_flags() & GRIB2::WeatherProfilePoint::flags_daymask);
				if (xx == x)
					continue;
				x = xx;
				switch (x) {
				case GRIB2::WeatherProfilePoint::flags_day:
					std::cerr << "Day";
					break;

				case GRIB2::WeatherProfilePoint::flags_dusk:
					std::cerr << "Dusk";
					break;

				case GRIB2::WeatherProfilePoint::flags_night:
					std::cerr << "Night";
					break;

				case GRIB2::WeatherProfilePoint::flags_dawn:
					std::cerr << "Dawn";
					break;

				default:
					std::cerr << "?""?";
					break;
				}
				std::cerr << " routedist " << pi->get_routedist() << " time "
					  << Glib::TimeVal(pi->get_efftime(), 0).as_iso8601() << std::endl;
			}
		}
		for (GRIB2::WeatherProfile::const_iterator pi(m_wxprofile.begin()), pe(m_wxprofile.end()); pi != pe; ) {
			if ((pi->get_flags() & GRIB2::WeatherProfilePoint::flags_daymask) == GRIB2::WeatherProfilePoint::flags_day) {
				++pi;
				continue;
			}
			GRIB2::WeatherProfile::const_iterator pi2(pi);
			for (++pi2; pi2 != pe; ++pi2)
				if ((pi->get_flags() ^ pi2->get_flags()) & GRIB2::WeatherProfilePoint::flags_daymask)
					break;
			double dist1(xaxistime ? (pi->get_efftime() - m_wxprofile.front().get_efftime()) : pi->get_routedist());
			double dist2;
			if (pi2 != pe)
				dist2 = xaxistime ? (pi2->get_efftime() - m_wxprofile.front().get_efftime()) : pi2->get_routedist();
			else
				dist2 = xaxistime ? (m_wxprofile.back().get_efftime() - m_wxprofile.front().get_efftime()) : m_wxprofile.get_dist();
			if ((pi->get_flags() & GRIB2::WeatherProfilePoint::flags_daymask) == GRIB2::WeatherProfilePoint::flags_night) {
				set_color_sky_night(cr);
			} else {
				int before(-1), after(-1);
				if (pi != m_wxprofile.begin()) {
					GRIB2::WeatherProfile::const_iterator pi2(pi);
					--pi2;
					switch (pi2->get_flags() & GRIB2::WeatherProfilePoint::flags_daymask) {
					case GRIB2::WeatherProfilePoint::flags_day:
						before = 1;
						break;

					case GRIB2::WeatherProfilePoint::flags_night:
						before = 0;
						break;

					default:
						break;
					}
				}
				if (pi2 != pe) {
					switch (pi2->get_flags() & GRIB2::WeatherProfilePoint::flags_daymask) {
					case GRIB2::WeatherProfilePoint::flags_day:
						after = 1;
						break;

					case GRIB2::WeatherProfilePoint::flags_night:
						after = 0;
						break;

					default:
						break;
					}
				}
				if (true)
					std::cerr << "Dusk/Dawn: dist: " << dist1 << "..." << dist2 << " before " << before
						  << " after " << after << std::endl;
				if ((before != 1 && after == 1) || (before == 0 && after != 0)) {
					Cairo::RefPtr<Cairo::LinearGradient> g(Cairo::LinearGradient::create(dist2, 0, dist1, 0));
					set_color_sky_duskdawn(g);
					cr->set_source(g);
				} else if ((before == 1 && after != 1) || (before != 0 && after == 0)) {
					Cairo::RefPtr<Cairo::LinearGradient> g(Cairo::LinearGradient::create(dist1, 0, dist2, 0));
					set_color_sky_duskdawn(g);
					cr->set_source(g);
				} else {
					set_color_sky_duskdawn(cr);
				}
			}
			double yg(yconvert(0, yaxis)), yt(yconvert(100000, yaxis));
			cr->rectangle(dist1, yg, dist2 - dist1, yt - yg);
			cr->fill();
			if (false)
				std::cerr << "dark sky: D" << dist1 << "-D" << dist2 << " state "
					  << (pi->get_flags() & GRIB2::WeatherProfilePoint::flags_daymask) << std::endl;
			pi = pi2;
		}
	}
	// draw max elev
	cr->set_line_width(1.0);
	if (xaxistime) {
		double yg(yconvert(0, yaxis));
		double ye(yconvert(m_routeprofile.front().get_maxelev() * Point::m_to_ft, yaxis));
		double dist(m_wxprofile.back().get_efftime() - m_wxprofile.front().get_efftime());
		cr->begin_new_path();
		cr->move_to(0, yg);
		cr->line_to(dist, yg);
		cr->line_to(dist, ye);
		cr->line_to(0, ye);
		cr->close_path();
	} else {
		double yg(yconvert(0, yaxis));
		cr->begin_new_path();
		cr->move_to(0, yg);
		double dist(0);
		for (TopoDb30::RouteProfile::const_iterator pi(m_routeprofile.begin()), pe(m_routeprofile.end()); pi != pe; ++pi)
			cr->line_to(dist = pi->get_routedist(), yconvert(pi->get_maxelev() * Point::m_to_ft, yaxis));
		cr->line_to(dist, yg);
		cr->close_path();
	}
	cr->set_matrix(mnorm);
	set_color_terrain_fill_maxelev(cr);
	cr->fill_preserve();
	set_color_terrain(cr);
	cr->stroke();
	cr->set_matrix(mscaled);
	// draw boundary clouds
	for (GRIB2::WeatherProfile::const_iterator pi(m_wxprofile.begin()), pe(m_wxprofile.end()); pi != pe; ) {
		if (std::isnan(pi->get_cldbdrycover()) || pi->get_cldbdrycover() <= 0.05 ||
		    pi->get_bdrylayerheight() == GRIB2::WeatherProfilePoint::invalidalt) {
			++pi;
			continue;
		}
		std::vector<CloudPoint> cldpt;
		GRIB2::WeatherProfile::const_iterator pi2(pi);
		for (;;) {
			TopoDb30::RouteProfilePoint rpp(m_routeprofile.interpolate(pi2->get_routedist()));
			if (rpp.get_elev() == TopoDb30::nodata)
				break;
			double elev(rpp.get_elev() * Point::m_to_ft);
			int32_t bdrybottom(0);
			if (!false) {
				if (false)
					std::cerr << "Boundary Layer: D" << pi2->get_routedist() << ' ' << pi2->get_bdrylayerbase()
						  << "..." << pi2->get_bdrylayerheight() << std::endl;
				bdrybottom = pi2->get_bdrylayerbase();
				if (bdrybottom == GRIB2::WeatherProfilePoint::invalidalt) {
					bdrybottom = 0;
				} else if (bdrybottom + 500 > pi2->get_bdrylayerheight())
					bdrybottom = pi2->get_bdrylayerheight() - 500;
			}
			cldpt.push_back(CloudPoint(xaxistime ? (pi2->get_efftime() - m_wxprofile.front().get_efftime()) : pi2->get_routedist(),
						   elev + bdrybottom, elev + pi2->get_bdrylayerheight(), pi2->get_cldbdrycover()));
			++pi2;
			if (pi2 == pe || std::isnan(pi2->get_cldbdrycover()) || pi2->get_cldbdrycover() <= 0.05 ||
			    pi2->get_bdrylayerheight() == GRIB2::WeatherProfilePoint::invalidalt)
				break;
		}
		if (pi == pi2) {
			++pi;
			continue;
		}
		pi = pi2;
		draw_clouds(cr, cldpt, yaxis, false);
	}
	// draw low clouds
	{
		CloudIcons icons("stratus", cr->get_target());
		StratusCloudBlueNoise bicon("stratus.svg", cr->get_target());
		for (GRIB2::WeatherProfile::const_iterator pi(m_wxprofile.begin()), pe(m_wxprofile.end()); pi != pe; ) {
			if (std::isnan(pi->get_cldlowcover()) || pi->get_cldlowcover() <= 0 ||
			    pi->get_cldlowbase() == GRIB2::WeatherProfilePoint::invalidalt ||
			    pi->get_cldlowtop() == GRIB2::WeatherProfilePoint::invalidalt) {
				++pi;
				continue;
			}
			std::vector<CloudPoint> cldpt;
			GRIB2::WeatherProfile::const_iterator pi2(pi);
			for (;;) {
				if (false)
					std::cerr << "Low Cld: D" << pi2->get_routedist() << " T"
						  << (pi2->get_efftime() - m_wxprofile.front().get_efftime()) << ' '
						  << pi2->get_cldlowbase() << "..." << pi2->get_cldlowtop()
						  << " C" << pi2->get_cldlowcover() << std::endl;
				cldpt.push_back(CloudPoint(xaxistime ? (pi2->get_efftime() - m_wxprofile.front().get_efftime()) : pi2->get_routedist(),
							   pi2->get_cldlowbase(), pi2->get_cldlowtop(), pi2->get_cldlowcover()));
				++pi2;
				if (pi2 == pe || std::isnan(pi2->get_cldlowcover()) || pi2->get_cldlowcover() <= 0 ||
				    pi2->get_cldlowbase() == GRIB2::WeatherProfilePoint::invalidalt ||
				    pi2->get_cldlowtop() == GRIB2::WeatherProfilePoint::invalidalt)
					break;
			}
			pi = pi2;
			if (bicon)
				draw_clouds(cr, cldpt, yaxis, bicon, 0.8);
			else if (icons)
				draw_clouds(cr, cldpt, yaxis, icons, 0.5);
			else
				draw_clouds(cr, cldpt, yaxis, false);
		}
	}
	// draw mid clouds
	{
		CloudIcons icons("altostratus", cr->get_target());
		StratusCloudBlueNoise bicon("stratus.svg", cr->get_target()); // FIXME
		for (GRIB2::WeatherProfile::const_iterator pi(m_wxprofile.begin()), pe(m_wxprofile.end()); pi != pe; ) {
			if (std::isnan(pi->get_cldmidcover()) || pi->get_cldmidcover() <= 0 ||
			    pi->get_cldmidbase() == GRIB2::WeatherProfilePoint::invalidalt ||
			    pi->get_cldmidtop() == GRIB2::WeatherProfilePoint::invalidalt) {
				++pi;
				continue;
			}
			std::vector<CloudPoint> cldpt;
			GRIB2::WeatherProfile::const_iterator pi2(pi);
			for (;;) {
				cldpt.push_back(CloudPoint(xaxistime ? (pi2->get_efftime() - m_wxprofile.front().get_efftime()) : pi2->get_routedist(),
							   pi2->get_cldmidbase(), pi2->get_cldmidtop(), pi2->get_cldmidcover()));
				++pi2;
				if (pi2 == pe || std::isnan(pi2->get_cldmidcover()) || pi2->get_cldmidcover() <= 0 ||
				    pi2->get_cldmidbase() == GRIB2::WeatherProfilePoint::invalidalt ||
				    pi2->get_cldmidtop() == GRIB2::WeatherProfilePoint::invalidalt)
					break;
			}
			pi = pi2;
			if (bicon)
				draw_clouds(cr, cldpt, yaxis, bicon, 0.8);
			else if (icons)
				draw_clouds(cr, cldpt, yaxis, icons, 0.5);
			else
				draw_clouds(cr, cldpt, yaxis, false);
		}
	}
	// draw high clouds
	{
		CloudIcons icons("cirrus", cr->get_target());
		for (GRIB2::WeatherProfile::const_iterator pi(m_wxprofile.begin()), pe(m_wxprofile.end()); pi != pe; ) {
			if (std::isnan(pi->get_cldhighcover()) || pi->get_cldhighcover() <= 0 ||
			    pi->get_cldhighbase() == GRIB2::WeatherProfilePoint::invalidalt ||
			    pi->get_cldhightop() == GRIB2::WeatherProfilePoint::invalidalt) {
				++pi;
				continue;
			}
			std::vector<CloudPoint> cldpt;
			GRIB2::WeatherProfile::const_iterator pi2(pi);
			for (;;) {
				cldpt.push_back(CloudPoint(xaxistime ? (pi2->get_efftime() - m_wxprofile.front().get_efftime()) : pi2->get_routedist(),
							   pi2->get_cldhighbase(), pi2->get_cldhightop(), pi2->get_cldhighcover()));
				++pi2;
				if (pi2 == pe || std::isnan(pi2->get_cldhighcover()) || pi2->get_cldhighcover() <= 0 ||
				    pi2->get_cldhighbase() == GRIB2::WeatherProfilePoint::invalidalt ||
				    pi2->get_cldhightop() == GRIB2::WeatherProfilePoint::invalidalt)
					break;
			}
			pi = pi2;
			if (icons)
				draw_clouds(cr, cldpt, yaxis, icons, 0.5);
			else
				draw_clouds(cr, cldpt, yaxis, false);
		}
	}
	// draw convective
	{
		ConvCloudIcons icons("cumulus_", cr->get_target());
		Cairo::RefPtr<Cairo::SurfacePattern> cbpattern;
		if (!icons) {
			Cairo::RefPtr<Cairo::Surface> patsurf(Cairo::Surface::create(cr->get_target(), Cairo::CONTENT_COLOR_ALPHA, 200, 200));
			Cairo::RefPtr<Cairo::Context> cr(Cairo::Context::create(patsurf));
			cr->translate(100, 100);
			for (unsigned int i = 0; i < 2; ++i) {
				cr->set_line_width(i ? 1.5 : 5);
				cr->set_source_rgba(!i, !i, !i, 1);
				cr->save();
				cr->arc(0, 40, 50, M_PI, 0);
				cr->close_path();
				cr->stroke();
				cr->arc(0, 40, 50, M_PI, 0);
				cr->line_to(100, 40);
				cr->line_to(100, -100);
				cr->line_to(-100, -100);
				cr->line_to(-100, 40);
				cr->close_path();
				cr->clip();
				if (true) {
					// TCU symbol
					cr->arc(0, 0, 20, M_PI, 0);
				} else {
					// CB symbol
					cr->arc(0, -30, 40, 0, M_PI);
				}
				cr->close_path();
				cr->stroke();
				cr->restore();
			}
			cbpattern = Cairo::SurfacePattern::create(patsurf);
			cbpattern->set_extend(Cairo::EXTEND_REPEAT);
			cbpattern->set_filter(Cairo::FILTER_GOOD);
		}
		for (GRIB2::WeatherProfile::const_iterator pi(m_wxprofile.begin()), pe(m_wxprofile.end()); pi != pe; ) {
			if (std::isnan(pi->get_cldconvcover()) || pi->get_cldconvcover() <= 0 ||
			    pi->get_cldconvbase() == GRIB2::WeatherProfilePoint::invalidalt ||
			    pi->get_cldconvtop() == GRIB2::WeatherProfilePoint::invalidalt) {
				++pi;
				continue;
			}
			std::vector<CloudPoint> cldpt;
			GRIB2::WeatherProfile::const_iterator pi2(pi);
			int32_t tropopause(std::numeric_limits<int32_t>::max());
			for (;;) {
				cldpt.push_back(CloudPoint(xaxistime ? (pi2->get_efftime() - m_wxprofile.front().get_efftime()) : pi2->get_routedist(),
							   pi2->get_cldconvbase(), pi2->get_cldconvtop(), pi2->get_cldconvcover()));
				tropopause = std::min(tropopause, pi2->get_tropopause());
				++pi2;
				if (pi2 == pe || std::isnan(pi2->get_cldconvcover()) || pi2->get_cldconvcover() <= 0 ||
				    pi2->get_cldconvbase() == GRIB2::WeatherProfilePoint::invalidalt ||
				    pi2->get_cldconvtop() == GRIB2::WeatherProfilePoint::invalidalt)
					break;
			}
			pi = pi2;
			if (icons) {
				draw_clouds(cr, cldpt, yaxis, icons, 0.5, 4, tropopause);
			} else {
				draw_clouds(cr, cldpt, yaxis, true);
				cr->save();
				cr->clip();
				cr->set_matrix(mnorm);
				cr->scale(0.1, 0.1);
				cr->set_source(cbpattern);
				cr->paint_with_alpha(0.5);
				cr->restore();
			}
		}
	}
	// rain, snow
	if (false) {
		// use categorical flags
		const double frequency(0.001 * scaledist * scaleelev);
		Glib::Rand rnd;
		if (false) {
			Glib::TimeVal tv;
			tv.assign_current_time();
			rnd.set_seed(tv.tv_usec ^ tv.tv_sec);
		}
		double yg(yconvert(0, yaxis));
		double cumprecip(0);
		static const unsigned int nrsfc(sizeof(GRIB2::WeatherProfilePoint::isobaric_levels)/
						sizeof(GRIB2::WeatherProfilePoint::isobaric_levels[0]));
		for (GRIB2::WeatherProfile::const_iterator pi(m_wxprofile.begin()), pe(m_wxprofile.end()); pi != pe;) {
			GRIB2::WeatherProfile::const_iterator pi0(pi);
			++pi;
			if (pi == pe)
				break;
			if (!(pi0->get_flags() & (GRIB2::WeatherProfilePoint::flags_rain |
						  GRIB2::WeatherProfilePoint::flags_freezingrain |
						  GRIB2::WeatherProfilePoint::flags_icepellets |
						  GRIB2::WeatherProfilePoint::flags_snow))) {
				cumprecip = 0;
				continue;
			}
			int32_t alt(-1);
			if (!std::isnan(pi->get_cldmidcover()) && pi->get_cldmidcover() >= 0.25 &&
			    pi->get_cldmidbase() != GRIB2::WeatherProfilePoint::invalidalt &&
			    pi->get_cldmidtop() != GRIB2::WeatherProfilePoint::invalidalt)
				alt = pi->get_cldmidbase();
			else if (!std::isnan(pi->get_cldlowcover()) && pi->get_cldlowcover() >= 0.25 &&
			    pi->get_cldlowbase() != GRIB2::WeatherProfilePoint::invalidalt &&
			    pi->get_cldlowtop() != GRIB2::WeatherProfilePoint::invalidalt)
				alt = pi->get_cldlowbase();
			else
				continue;
			double yc(yconvert(alt, yaxis));
			double dist0(xaxistime ? (pi0->get_efftime() - m_wxprofile.front().get_efftime()) : pi0->get_routedist());
			double dist1(xaxistime ? (pi->get_efftime() - m_wxprofile.front().get_efftime()) : pi->get_routedist());
			double precip((yc - yg) * (dist1 - dist0) * frequency * 2);
			cumprecip += precip;
			int nr(Point::round<int,double>(rnd.get_double_range(0, cumprecip)));
			if (false)
				std::cerr << "precip: D" << pi0->get_routedist() << ".." << pi->get_routedist()
					  << " nr " << nr << " precip " << precip <<  " cum " << cumprecip << std::endl;
			cumprecip -= nr;
			for (; nr > 0; --nr) {
				int32_t alt1(Point::round<int,double>(rnd.get_double_range(0, alt)));
				double temp(0);
				for (unsigned int i = 1; i < nrsfc; ++i) {
					if (GRIB2::WeatherProfilePoint::isobaric_levels[i - 1] < 0 ||
					    GRIB2::WeatherProfilePoint::isobaric_levels[i] < 0)
						continue;
					if (alt1 < GRIB2::WeatherProfilePoint::altitudes[i - 1] ||
					    alt1 >= GRIB2::WeatherProfilePoint::altitudes[i]);
					float t0(pi->operator[](i-1).get_temp());
					float t1(pi->operator[](i).get_temp());
					temp = t0 + (t1 - t0) * (alt1 - GRIB2::WeatherProfilePoint::altitudes[i - 1])
						/ (GRIB2::WeatherProfilePoint::altitudes[i] - GRIB2::WeatherProfilePoint::altitudes[i - 1]);
					break;
				}
				bool snowcryst(temp < IcaoAtmosphere<double>::degc_to_kelvin + 3);
				cr->move_to(rnd.get_double_range(dist0, dist1), yconvert(alt1, yaxis));
				cr->set_matrix(mnorm);
				if (snowcryst) {
					static const double snowflake[6][2] = {
						{  0.00000,   2.5000 },
						{  2.16506,   1.2500 },
						{  2.16506,  -1.2500 },
						{  0.00000,  -2.5000 },
						{ -2.16506,  -1.2500 },
						{ -2.16506,   1.2500 }
					};
					for (unsigned int i(0); i < 6; ++i) {
						cr->rel_line_to(snowflake[i][0], snowflake[i][1]);
						cr->rel_move_to(-snowflake[i][0], -snowflake[i][1]);
					}
					cr->set_line_width(1.5);
					cr->set_source_rgb(0, 0, 0);
					cr->stroke_preserve();
					cr->set_line_width(0.5);
					cr->set_source_rgb(1, 1, 1);
					cr->stroke();
				} else {
					cr->rel_move_to(-4, 4);
					cr->rel_line_to(4, -8);
					cr->rel_move_to(-2, 8);
					cr->rel_line_to(4, -8);
					cr->rel_move_to(-2, 8);
					cr->set_line_width(3);
					cr->set_source_rgb(1, 1, 1);
					cr->stroke_preserve();
					cr->set_line_width(1);
					cr->set_source_rgb(0, 0, 0);
					cr->stroke();
				}
				cr->set_matrix(mscaled);
			}
		}
	} else {
		// use precipitation rate
		const double frequency(0.0001 * 86400 * scaledist * scaleelev);
		Glib::Rand rnd;
		if (false) {
			Glib::TimeVal tv;
			tv.assign_current_time();
			rnd.set_seed(tv.tv_usec ^ tv.tv_sec);
		}
		double yg(yconvert(0, yaxis));
		double cumprecip1(0), cumprecip2(0);
		static const unsigned int nrsfc(sizeof(GRIB2::WeatherProfilePoint::isobaric_levels)/
						sizeof(GRIB2::WeatherProfilePoint::isobaric_levels[0]));
		for (GRIB2::WeatherProfile::const_iterator pi(m_wxprofile.begin()), pe(m_wxprofile.end()); pi != pe;) {
			GRIB2::WeatherProfile::const_iterator pi0(pi);
			++pi;
			if (pi == pe)
				break;
			double dist0(xaxistime ? (pi0->get_efftime() - m_wxprofile.front().get_efftime()) : pi0->get_routedist());
			double dist1(xaxistime ? (pi->get_efftime() - m_wxprofile.front().get_efftime()) : pi->get_routedist());
			int nr1(0), nr2(0);
			int32_t alt1(-1), alt2(-1);
			if (!std::isnan(pi->get_cldmidcover()) && pi->get_cldmidcover() >= 0.25 &&
			    pi->get_cldmidbase() != GRIB2::WeatherProfilePoint::invalidalt &&
			    pi->get_cldmidtop() != GRIB2::WeatherProfilePoint::invalidalt)
				alt1 = pi->get_cldmidbase();
			else if (!std::isnan(pi->get_cldlowcover()) && pi->get_cldlowcover() >= 0.25 &&
			    pi->get_cldlowbase() != GRIB2::WeatherProfilePoint::invalidalt &&
			    pi->get_cldlowtop() != GRIB2::WeatherProfilePoint::invalidalt)
				alt1 = pi->get_cldlowbase();
			if (!std::isnan(pi->get_cldconvcover()) && pi->get_cldconvcover() >= 0 &&
			    pi->get_cldconvbase() != GRIB2::WeatherProfilePoint::invalidalt &&
			    pi->get_cldconvtop() != GRIB2::WeatherProfilePoint::invalidalt)
				alt2 = pi->get_cldconvbase();
			cumprecip1 *= 0.95;
			cumprecip2 *= 0.95;
			if (alt1 >= 0) {
				double yc(yconvert(alt1, yaxis));
				double precip((yc - yg) * (dist1 - dist0) * pi->get_preciprate() * frequency);
				if (!std::isnan(precip) && precip > 0)
				cumprecip1 += precip;
				nr1 = Point::round<int,double>(rnd.get_double_range(0, cumprecip1));
				cumprecip1 -= nr1;
			}
			if (alt2 >= 0) {
				double yc(yconvert(alt2, yaxis));
				double precip((yc - yg) * (dist1 - dist0) * pi->get_convpreciprate() * frequency);
				if (!std::isnan(precip) && precip > 0)
					cumprecip2 += precip;
				nr2 = Point::round<int,double>(rnd.get_double_range(0, cumprecip2));
				cumprecip2 -= nr2;
			}
			while (nr1 > 0 || nr2 > 0) {
				int32_t altx;
				if (nr1 > 0) {
					altx = Point::round<int,double>(rnd.get_double_range(0, alt1));
					--nr1;
				} else if (nr2 > 0) {
					altx = Point::round<int,double>(rnd.get_double_range(0, alt2));
					--nr2;
				} else {
					break;
				}
				double temp(0);
				for (unsigned int i = 1; i < nrsfc; ++i) {
					if (GRIB2::WeatherProfilePoint::isobaric_levels[i - 1] < 0 ||
					    GRIB2::WeatherProfilePoint::isobaric_levels[i] < 0)
						continue;
					if (altx < GRIB2::WeatherProfilePoint::altitudes[i - 1] ||
					    altx >= GRIB2::WeatherProfilePoint::altitudes[i]);
					float t0(pi->operator[](i-1).get_temp());
					float t1(pi->operator[](i).get_temp());
					temp = t0 + (t1 - t0) * (altx - GRIB2::WeatherProfilePoint::altitudes[i - 1])
						/ (GRIB2::WeatherProfilePoint::altitudes[i] - GRIB2::WeatherProfilePoint::altitudes[i - 1]);
					break;
				}
				bool snowcryst(temp < IcaoAtmosphere<double>::degc_to_kelvin + 3);
				cr->move_to(rnd.get_double_range(dist0, dist1), yconvert(altx, yaxis));
				cr->set_matrix(mnorm);
				if (snowcryst) {
					static const double snowflake[6][2] = {
						{  0.00000,   2.5000 },
						{  2.16506,   1.2500 },
						{  2.16506,  -1.2500 },
						{  0.00000,  -2.5000 },
						{ -2.16506,  -1.2500 },
						{ -2.16506,   1.2500 }
					};
					for (unsigned int i(0); i < 6; ++i) {
						cr->rel_line_to(snowflake[i][0], snowflake[i][1]);
						cr->rel_move_to(-snowflake[i][0], -snowflake[i][1]);
					}
					cr->set_line_width(1.5);
					cr->set_source_rgb(0, 0, 0);
					cr->stroke_preserve();
					cr->set_line_width(0.5);
					cr->set_source_rgb(1, 1, 1);
					cr->stroke();
				} else {
					cr->rel_move_to(-4, 4);
					cr->rel_line_to(4, -8);
					cr->rel_move_to(-2, 8);
					cr->rel_line_to(4, -8);
					cr->rel_move_to(-2, 8);
					cr->set_line_width(3);
					cr->set_source_rgb(1, 1, 1);
					cr->stroke_preserve();
					cr->set_line_width(1);
					cr->set_source_rgb(0, 0, 0);
					cr->stroke();
				}
				cr->set_matrix(mscaled);
			}
		}
	}
	// draw isotherms
	for (int therm = -60; therm < 50; therm += 10) {
		static const unsigned int nrsfc(sizeof(GRIB2::WeatherProfilePoint::isobaric_levels)/
						sizeof(GRIB2::WeatherProfilePoint::isobaric_levels[0]));
		double temp(IcaoAtmosphere<double>::degc_to_kelvin + therm);
		std::multiset<LinePoint> isothermpoints;
		for (GRIB2::WeatherProfile::const_iterator pi(m_wxprofile.begin()), pe(m_wxprofile.end()); pi != pe; ++pi) {
			for (unsigned int i = 1; i < nrsfc; ++i) {
				if (GRIB2::WeatherProfilePoint::isobaric_levels[i - 1] < 0 ||
				    GRIB2::WeatherProfilePoint::isobaric_levels[i] < 0)
					continue;
				double t0(pi->operator[](i-1).get_temp());
				double t1(pi->operator[](i).get_temp());
				if (std::isnan(t0) || std::isnan(t1))
					continue;
				if ((t0 <= temp && t1 > temp) || (t0 > temp && t1 <= temp)) {
					int32_t alt0(GRIB2::WeatherProfilePoint::altitudes[i - 1]);
					int32_t alt1(GRIB2::WeatherProfilePoint::altitudes[i]);
					int32_t alt(alt0 + (alt1 - alt0) * (temp - t0) / (t1 - t0));
					isothermpoints.insert(LinePoint(pi - m_wxprofile.begin(), alt, t1 > t0));
				}
			}
		}
		{
			std::vector<double> dashes;
			dashes.push_back(2);
			dashes.push_back(2);
			cr->set_dash(dashes, 0);
		}
		cr->set_line_width(1.0);
		double labeldist(std::numeric_limits<double>::quiet_NaN()), labelalt(0);
		for (;;) {
			std::vector<LinePoint> isotherm(extract_line(isothermpoints, 1000));
			if (isotherm.empty())
				break;
			if (isotherm.size() < 2)
				continue;
			cr->begin_new_path();
			std::vector<MeteoProfile::LinePoint>::const_iterator li(isotherm.begin()), le(isotherm.end());
			{
				double dist(xaxistime ? (m_wxprofile[li->get_x()].get_efftime() - m_wxprofile.front().get_efftime()) : m_wxprofile[li->get_x()].get_routedist());
				cr->move_to(dist, yconvert(li->get_y(), yaxis));
			}
			for (++li; li != le; ++li) {
				double dist(xaxistime ? (m_wxprofile[li->get_x()].get_efftime() - m_wxprofile.front().get_efftime()) : m_wxprofile[li->get_x()].get_routedist());
				cr->line_to(dist, yconvert(li->get_y(), yaxis));
			}
			cr->set_matrix(mnorm);
			if (therm)
				set_color_otherisotherm(cr);
			else
				set_color_0degisotherm(cr);
			cr->stroke();
			cr->set_matrix(mscaled);
			for (std::vector<MeteoProfile::LinePoint>::const_iterator li(isotherm.begin()), le(isotherm.end()); li != le; ++li) {
				double dist(xaxistime ? (m_wxprofile[li->get_x()].get_efftime() - m_wxprofile.front().get_efftime()) : m_wxprofile[li->get_x()].get_routedist());
				if ((dist - origindist) * scaledist < 10)
					continue;
				if (std::isnan(labeldist) || dist < labeldist) {
					labeldist = dist;
					labelalt = li->get_y();
				}
			}
		}
		cr->unset_dash();
		if (!std::isnan(labeldist)) {
			std::ostringstream oss;
			oss << therm << "\302\260C";
			cr->move_to(labeldist, yconvert(labelalt, yaxis));
			cr->set_matrix(mnorm);
			cr->set_font_size(8);
			Cairo::TextExtents ext;
			cr->get_text_extents(oss.str(), ext);
			cr->rel_move_to(-ext.x_bearing, -ext.y_bearing - 0.5*ext.height);
			cr->set_source_rgb(1., 1., 1.);
			double x, y;
			cr->get_current_point(x, y);
			cr->set_line_width(2.0);
			cr->text_path(oss.str());
			cr->stroke();
			cr->move_to(x, y);
			if (therm)
				set_color_otherisotherm(cr);
			else
				set_color_0degisotherm(cr);
			cr->show_text(oss.str());
			cr->set_font_size(12);
			cr->set_matrix(mscaled);
		}
	}
	// draw 0degC isotherm
	cr->set_line_width(1.0);
	if (false) {
		double labeldist(std::numeric_limits<double>::quiet_NaN()), labelalt(0);
		for (GRIB2::WeatherProfile::const_iterator pi(m_wxprofile.begin()), pe(m_wxprofile.end()); pi != pe; ) {
			if (pi->get_zerodegisotherm() == GRIB2::WeatherProfilePoint::invalidalt) {
				++pi;
				continue;
			}
			cr->begin_new_path();
			{
				double dist(xaxistime ? (pi->get_efftime() - m_wxprofile.front().get_efftime()) : pi->get_routedist());
				cr->move_to(dist, yconvert(pi->get_zerodegisotherm(), yaxis));
				if ((dist - origindist) * scaledist >= 10 &&
				    (std::isnan(labeldist) || dist < labeldist)) {
					labeldist = dist;
					labelalt = pi->get_zerodegisotherm();
				}
			}
			GRIB2::WeatherProfile::const_iterator pi2(pi);
			++pi2;
			while (pi2 != pe && pi2->get_zerodegisotherm() != GRIB2::WeatherProfilePoint::invalidalt) {
				double dist(xaxistime ? (pi2->get_efftime() - m_wxprofile.front().get_efftime()) : pi2->get_routedist());
				cr->line_to(dist, yconvert(pi2->get_zerodegisotherm(), yaxis));
				if ((dist - origindist) * scaledist >= 10 &&
				    (std::isnan(labeldist) || dist < labeldist)) {
					labeldist = dist;
					labelalt = pi2->get_zerodegisotherm();
				}
				++pi2;
			}
			if (false && (pi2 - pi) <= 1) {
				pi = pi2;
				continue;
			}
			cr->set_matrix(mnorm);
			set_color_0degisotherm(cr);
			cr->stroke();
			cr->set_matrix(mscaled);
			pi = pi2;
		}
		if (!std::isnan(labeldist)) {
			static const char label[] = "0\302\260C";
			cr->move_to(labeldist, yconvert(labelalt, yaxis));
			cr->set_matrix(mnorm);
			cr->set_font_size(8);
			Cairo::TextExtents ext;
			cr->get_text_extents(label, ext);
			cr->rel_move_to(-ext.x_bearing, -ext.y_bearing - 0.5*ext.height);
			cr->set_source_rgb(1., 1., 1.);
			double x, y;
			cr->get_current_point(x, y);
			cr->set_line_width(2.0);
			cr->text_path(label);
			cr->stroke();
			cr->move_to(x, y);
			set_color_otherisotherm(cr);
			cr->show_text(label);
			cr->set_font_size(12);
			cr->set_matrix(mscaled);
		}
	}
	cr->unset_dash();
	// draw driftdown minimum altitude
	{
		cr->set_line_width(1.0);
		double labeldist(std::numeric_limits<double>::quiet_NaN()), labelalt(0);
		for (DriftDownProfile::const_iterator pi(m_ddprofile.begin()), pe(m_ddprofile.end()); pi != pe; ) {
			if (pi->get_glidealt() == std::numeric_limits<int32_t>::max()) {
				++pi;
				continue;
			}
			cr->begin_new_path();
			{
				double dist(xaxistime ? (pi->get_efftime() - m_ddprofile.front().get_efftime()) : pi->get_routedist());
				cr->move_to(dist, yconvert(pi->get_glidealt(), yaxis));
				if ((dist - origindist) * scaledist >= 10 &&
				    (std::isnan(labeldist) || pi->get_routedist() < labeldist)) {
					labeldist = dist;
					labelalt = pi->get_glidealt();
				}
			}
		        DriftDownProfile::const_iterator pi2(pi);
			++pi2;
			while (pi2 != pe && pi2->get_glidealt() != std::numeric_limits<int32_t>::max()) {
				double dist(xaxistime ? (pi2->get_efftime() - m_ddprofile.front().get_efftime()) : pi2->get_routedist());
				cr->line_to(dist, yconvert(pi2->get_glidealt(), yaxis));
				if ((dist - origindist) * scaledist >= 10 &&
				    (std::isnan(labeldist) || dist < labeldist)) {
					labeldist = dist;
					labelalt = pi2->get_glidealt();
				}
				++pi2;
			}
			if (false && (pi2 - pi) <= 1) {
				pi = pi2;
				continue;
			}
			cr->set_matrix(mnorm);
			cr->set_source_rgb(0.0, 0.0, 0.0);
			{
				std::vector<double> dashes;
				dashes.push_back(4);
				dashes.push_back(4);
				cr->set_dash(dashes, 0);
			}
			cr->stroke_preserve();
			cr->unset_dash();
			cr->set_source_rgb(0.5, 0.5, 0.5);
			{
				std::vector<double> dashes;
				dashes.push_back(4);
				dashes.push_back(4);
				cr->set_dash(dashes, 4);
			}
			cr->stroke();
			cr->unset_dash();
			cr->set_matrix(mscaled);
			pi = pi2;
		}
		if (!std::isnan(labeldist)) {
			static const char label[] = "Glide";
			cr->move_to(labeldist, yconvert(labelalt, yaxis));
			cr->set_matrix(mnorm);
			cr->set_font_size(8);
			Cairo::TextExtents ext;
			cr->get_text_extents(label, ext);
			cr->rel_move_to(-ext.x_bearing, -ext.y_bearing - 0.5*ext.height);
			cr->set_source_rgb(1., 1., 1.);
			double x, y;
			cr->get_current_point(x, y);
			cr->set_line_width(2.0);
			cr->text_path(label);
			cr->stroke();
			cr->move_to(x, y);
			cr->set_source_rgb(0.0, 0.0, 0.0);
			cr->show_text(label);
			cr->set_font_size(12);
			cr->set_matrix(mscaled);
		}
	}
	// draw tropopause
	{
		cr->set_line_width(1.0);
		double labeldist(std::numeric_limits<double>::quiet_NaN()), labelalt(0);
		for (GRIB2::WeatherProfile::const_iterator pi(m_wxprofile.begin()), pe(m_wxprofile.end()); pi != pe; ) {
			if (pi->get_tropopause() == GRIB2::WeatherProfilePoint::invalidalt) {
				++pi;
				continue;
			}
			cr->begin_new_path();
			{
				double dist(xaxistime ? (pi->get_efftime() - m_wxprofile.front().get_efftime()) : pi->get_routedist());
				cr->move_to(dist, yconvert(pi->get_tropopause(), yaxis));
				if ((dist - origindist) * scaledist >= 10 &&
				    (std::isnan(labeldist) || pi->get_routedist() < labeldist)) {
					labeldist = dist;
					labelalt = pi->get_tropopause();
				}
			}
			GRIB2::WeatherProfile::const_iterator pi2(pi);
			++pi2;
			while (pi2 != pe && pi2->get_tropopause() != GRIB2::WeatherProfilePoint::invalidalt) {
				double dist(xaxistime ? (pi2->get_efftime() - m_wxprofile.front().get_efftime()) : pi2->get_routedist());
				cr->line_to(dist, yconvert(pi2->get_tropopause(), yaxis));
				if ((dist - origindist) * scaledist >= 10 &&
				    (std::isnan(labeldist) || dist < labeldist)) {
					labeldist = dist;
					labelalt = pi2->get_tropopause();
				}
				++pi2;
			}
			if (false && (pi2 - pi) <= 1) {
				pi = pi2;
				continue;
			}
			cr->set_matrix(mnorm);
			cr->set_source_rgb(1.0, 0.0, 0.0);
			{
				std::vector<double> dashes;
				dashes.push_back(4);
				dashes.push_back(4);
				cr->set_dash(dashes, 0);
			}
			cr->stroke_preserve();
			cr->unset_dash();
			cr->set_source_rgb(1.0, 0.9, 0.0);
			{
				std::vector<double> dashes;
				dashes.push_back(4);
				dashes.push_back(4);
				cr->set_dash(dashes, 4);
			}
			cr->stroke();
			cr->unset_dash();
			cr->set_matrix(mscaled);
			pi = pi2;
		}
		if (!std::isnan(labeldist)) {
			static const char label[] = "Tropopause";
			cr->move_to(labeldist, yconvert(labelalt, yaxis));
			cr->set_matrix(mnorm);
			cr->set_font_size(8);
			Cairo::TextExtents ext;
			cr->get_text_extents(label, ext);
			cr->rel_move_to(-ext.x_bearing, -ext.y_bearing - 0.5*ext.height);
			cr->set_source_rgb(1., 1., 1.);
			double x, y;
			cr->get_current_point(x, y);
			cr->set_line_width(2.0);
			cr->text_path(label);
			cr->stroke();
			cr->move_to(x, y);
			cr->set_source_rgb(1.0, 0.0, 0.0);
			cr->show_text(label);
			cr->set_font_size(12);
			cr->set_matrix(mscaled);
		}
	}
	// draw isotachs
	cr->set_line_width(1.0);
	for (unsigned int tach = 25; tach <= 200; tach += 25) {
		static const unsigned int nrsfc(sizeof(GRIB2::WeatherProfilePoint::isobaric_levels)/
						sizeof(GRIB2::WeatherProfilePoint::isobaric_levels[0]));
		static const float knots_to_mps(1.0 / 3.6 / Point::km_to_nmi_dbl);
		float mps(tach * knots_to_mps);
		std::multiset<LinePoint> isotachpoints;
		for (GRIB2::WeatherProfile::const_iterator pi(m_wxprofile.begin()), pe(m_wxprofile.end()); pi != pe; ++pi) {
			int xcnt(0);
			for (unsigned int i = 1; i < nrsfc; ++i) {
				if (GRIB2::WeatherProfilePoint::isobaric_levels[i - 1] < 0 ||
				    GRIB2::WeatherProfilePoint::isobaric_levels[i] < 0)
					continue;
				float t0(pi->operator[](i-1).get_wind());
				float t1(pi->operator[](i).get_wind());
				if (std::isnan(t0) || std::isnan(t1))
					continue;
				if ((t0 <= mps && t1 > mps) || (t0 > mps && t1 <= mps)) {
					int32_t alt0(GRIB2::WeatherProfilePoint::altitudes[i - 1]);
					int32_t alt1(GRIB2::WeatherProfilePoint::altitudes[i]);
					int32_t alt(alt0 + (alt1 - alt0) * (mps - t0) / (t1 - t0));
					isotachpoints.insert(LinePoint(pi - m_wxprofile.begin(), alt, t1 > t0));
					xcnt += (t1 > t0) ? 1 : -1;
				}
			}
			while (xcnt) {
				isotachpoints.insert(LinePoint(pi - m_wxprofile.begin(),
							       (xcnt < 0) ? 0 : GRIB2::WeatherProfilePoint::altitudes[nrsfc - 1],
							       xcnt < 0));
				xcnt += (xcnt < 0) ? 1 : -1;
			}
		}
		for (;;) {
			std::vector<LinePoint> isotach(extract_area(isotachpoints, 1000));
			if (isotach.empty())
				break;
			if (isotach.size() < 3)
				continue;
			cr->begin_new_path();
			std::vector<LinePoint>::const_iterator li(isotach.begin()), le(isotach.end());
			{
				double dist(xaxistime ? (m_wxprofile[li->get_x()].get_efftime() - m_wxprofile.front().get_efftime()) : m_wxprofile[li->get_x()].get_routedist());
				cr->move_to(dist, yconvert(li->get_y(), yaxis));
			}
			for (++li; li != le; ++li) {
				double dist(xaxistime ? (m_wxprofile[li->get_x()].get_efftime() - m_wxprofile.front().get_efftime()) : m_wxprofile[li->get_x()].get_routedist());
				cr->line_to(dist, yconvert(li->get_y(), yaxis));
			}
			cr->close_path();
			cr->set_matrix(mnorm);
			set_color_isotach(cr);
			std::vector<double> dashes;
			dashes.push_back(10);
			dashes.push_back(2);
			for (unsigned int x(0); x < tach; x += 25) {
				dashes.push_back(2);
				dashes.push_back(2);
			}
			cr->set_dash(dashes, 0);
			cr->stroke();
			cr->set_matrix(mscaled);
		}
	}
	// draw turbulence
	{
		std::multiset<LinePoint> turb[2]; // moderate, severe
		static const unsigned int nrsfc(sizeof(GRIB2::WeatherProfilePoint::isobaric_levels)/
						sizeof(GRIB2::WeatherProfilePoint::isobaric_levels[0]));
		for (GRIB2::WeatherProfile::const_iterator pi(m_wxprofile.begin()), pe(m_wxprofile.end()); pi != pe; ++pi) {
			int xcnt[2];
			xcnt[0] = xcnt[1] = 0;
			for (unsigned int i = 1; i < nrsfc; ++i) {
				if (GRIB2::WeatherProfilePoint::isobaric_levels[i - 1] < 0 ||
				    GRIB2::WeatherProfilePoint::isobaric_levels[i] < 0)
					continue;
				float t0(pi->operator[](i-1).get_turbulenceindex());
				float t1(pi->operator[](i).get_turbulenceindex());
				if (std::isnan(t0) || std::isnan(t1))
					continue;
				for (unsigned int k = 0; k < 2; ++k) {
					static const float turblim[2] = { 80, 160 };
					float tlim(turblim[k]);
					if ((t0 <= tlim && t1 > tlim) || (t0 > tlim && t1 <= tlim)) {
						int32_t alt0(GRIB2::WeatherProfilePoint::altitudes[i - 1]);
						int32_t alt1(GRIB2::WeatherProfilePoint::altitudes[i]);
						int32_t alt(alt0 + (alt1 - alt0) * (tlim - t0) / (t1 - t0));
						turb[k].insert(LinePoint(pi - m_wxprofile.begin(), alt, t1 > t0));
						xcnt[k] += (t1 > t0) ? 1 : -1;
					}
				}
			}
			for (unsigned int k = 0; k < 2; ++k) {
				while (xcnt[k]) {
					turb[k].insert(LinePoint(pi - m_wxprofile.begin(),
								 (xcnt[k] < 0) ? 0 : GRIB2::WeatherProfilePoint::altitudes[nrsfc - 1],
								 xcnt[k] < 0));
					xcnt[k] += (xcnt[k] < 0) ? 1 : -1;
				}
			}
		}
		for (unsigned int k(0); k < 2;) {
			std::vector<LinePoint> turbarea(extract_area(turb[k], 1000));
			if (turbarea.empty()) {
				++k;
				continue;
			}
			if (turbarea.size() < 3)
				continue;
			cr->begin_new_path();
			std::vector<LinePoint>::const_iterator li(turbarea.begin()), le(turbarea.end());
			{
				double dist(xaxistime ? (m_wxprofile[li->get_x()].get_efftime() - m_wxprofile.front().get_efftime()) : m_wxprofile[li->get_x()].get_routedist());
				cr->move_to(dist, yconvert(li->get_y(), yaxis));
			}
			for (++li; li != le; ++li) {
				double dist(xaxistime ? (m_wxprofile[li->get_x()].get_efftime() - m_wxprofile.front().get_efftime()) : m_wxprofile[li->get_x()].get_routedist());
				cr->line_to(dist, yconvert(li->get_y(), yaxis));
			}
			cr->close_path();
			cr->set_matrix(mnorm);
			cr->set_source_rgb(156 / 256.0, 85 / 256.0, 27 / 256.0);
			std::vector<double> dashes;
			dashes.push_back(5);
			dashes.push_back(2);
			dashes.push_back(5);
			dashes.push_back(2);
			if (k) {
				dashes.push_back(2);
				dashes.push_back(2);
			}
			cr->set_dash(dashes, 0);
			cr->stroke_preserve();
			cr->set_source_rgba(156 / 256.0, 85 / 256.0, 27 / 256.0, 0.2);
			cr->fill();
			cr->set_matrix(mscaled);
		}
	}
	// draw icing
	{
		// IENG_CNV = 200 * (Lwc(bottom_of_cloud) - lwc) / lwc(293K)) * sqrt((T - 253.15)/20.0);  si 253.15<=T<= 273.15
		// IENG_LYR = 100 * t *(t + 14)/ 49; where t = (T - 273.15); when  -14 <= t <= 0.
		// where:
		// lwc(293K) = 0.017281 Kg / m3  (water vapor density at 20ºC in a satured air)
		// lwc = es(T) / (Rv * T) (water vapor density at air cell conditions)
		// es: saturation vapor pressure (SVP)
		// lwc: liquid water content
		std::multiset<LinePoint> icing[2]; // moderate, severe
		static const unsigned int nrsfc(sizeof(GRIB2::WeatherProfilePoint::isobaric_levels)/
						sizeof(GRIB2::WeatherProfilePoint::isobaric_levels[0]));
		for (GRIB2::WeatherProfile::const_iterator pi(m_wxprofile.begin()), pe(m_wxprofile.end()); pi != pe; ++pi) {
			int xcnt[2];
			xcnt[0] = xcnt[1] = 0;
			float convbasetemp(std::numeric_limits<float>::quiet_NaN());
			double convbaselwc(std::numeric_limits<double>::quiet_NaN());
			for (unsigned int i = 1; i < nrsfc; ++i) {
				if (GRIB2::WeatherProfilePoint::isobaric_levels[i - 1] < 0 ||
				    GRIB2::WeatherProfilePoint::isobaric_levels[i] < 0)
					continue;
				const GRIB2::WeatherProfilePoint::Surface& surf0(pi->operator[](i-1));
				const GRIB2::WeatherProfilePoint::Surface& surf1(pi->operator[](i));
				float ieng0(0), ieng1(0);
				if (!std::isnan(pi->get_cldlowcover())) {
					if (GRIB2::WeatherProfilePoint::altitudes[i-1] >= pi->get_cldlowbase() &&
					    GRIB2::WeatherProfilePoint::altitudes[i-1] <= pi->get_cldlowtop() &&
					    surf0.get_temp() >= IcaoAtmosphere<float>::degc_to_kelvin - 14 &&
					    surf0.get_temp() <= IcaoAtmosphere<float>::degc_to_kelvin) {
						float t(surf0.get_temp() - IcaoAtmosphere<float>::degc_to_kelvin);
						ieng0 += pi->get_cldlowcover() * (100. / 49) * -t * (t + 14);
					}
					if (GRIB2::WeatherProfilePoint::altitudes[i] >= pi->get_cldlowbase() &&
					    GRIB2::WeatherProfilePoint::altitudes[i] <= pi->get_cldlowtop() &&
					    surf1.get_temp() >= IcaoAtmosphere<float>::degc_to_kelvin - 14 &&
					    surf1.get_temp() <= IcaoAtmosphere<float>::degc_to_kelvin) {
						float t(surf1.get_temp() - IcaoAtmosphere<float>::degc_to_kelvin);
						ieng1 += pi->get_cldlowcover() * (100. / 49) * -t * (t + 14);
					}
				}
				if (!std::isnan(pi->get_cldmidcover())) {
					if (GRIB2::WeatherProfilePoint::altitudes[i-1] >= pi->get_cldmidbase() &&
					    GRIB2::WeatherProfilePoint::altitudes[i-1] <= pi->get_cldmidtop() &&
					    surf0.get_temp() >= IcaoAtmosphere<float>::degc_to_kelvin - 14 &&
					    surf0.get_temp() <= IcaoAtmosphere<float>::degc_to_kelvin) {
						float t(surf0.get_temp() - IcaoAtmosphere<float>::degc_to_kelvin);
						ieng0 += pi->get_cldmidcover() * (100. / 49) * -t * (t + 14);
					}
					if (GRIB2::WeatherProfilePoint::altitudes[i] >= pi->get_cldmidbase() &&
					    GRIB2::WeatherProfilePoint::altitudes[i] <= pi->get_cldmidtop() &&
					    surf1.get_temp() >= IcaoAtmosphere<float>::degc_to_kelvin - 14 &&
					    surf1.get_temp() <= IcaoAtmosphere<float>::degc_to_kelvin) {
						float t(surf1.get_temp() - IcaoAtmosphere<float>::degc_to_kelvin);
						ieng1 += pi->get_cldmidcover() * (100. / 49) * -t * (t + 14);
					}
				}
				if (!std::isnan(pi->get_cldhighcover())) {
					if (GRIB2::WeatherProfilePoint::altitudes[i-1] >= pi->get_cldhighbase() &&
					    GRIB2::WeatherProfilePoint::altitudes[i-1] <= pi->get_cldhightop() &&
					    surf0.get_temp() >= IcaoAtmosphere<float>::degc_to_kelvin - 14 &&
					    surf0.get_temp() <= IcaoAtmosphere<float>::degc_to_kelvin) {
						float t(surf0.get_temp() - IcaoAtmosphere<float>::degc_to_kelvin);
						ieng0 += pi->get_cldhighcover() * (100. / 49) * -t * (t + 14);
					}
					if (GRIB2::WeatherProfilePoint::altitudes[i] >= pi->get_cldhighbase() &&
					    GRIB2::WeatherProfilePoint::altitudes[i] <= pi->get_cldhightop() &&
					    surf1.get_temp() >= IcaoAtmosphere<float>::degc_to_kelvin - 14 &&
					    surf1.get_temp() <= IcaoAtmosphere<float>::degc_to_kelvin) {
						float t(surf1.get_temp() - IcaoAtmosphere<float>::degc_to_kelvin);
						ieng1 += pi->get_cldhighcover() * (100. / 49) * -t * (t + 14);
					}
				}
				if (!std::isnan(pi->get_cldconvcover())) {
					if (std::isnan(convbasetemp)) {
						if (pi->get_cldconvbase() <= GRIB2::WeatherProfilePoint::altitudes[i-1])
							convbasetemp = surf0.get_temp();
						else if (pi->get_cldconvbase() <= GRIB2::WeatherProfilePoint::altitudes[i])
							convbasetemp = surf0.get_temp() + (surf1.get_temp() - surf0.get_temp())
								* (pi->get_cldconvbase() - GRIB2::WeatherProfilePoint::altitudes[i-1])
								/ (GRIB2::WeatherProfilePoint::altitudes[i] - GRIB2::WeatherProfilePoint::altitudes[i-1]);
						static const double lwccldbasepoly[] = {
							1.4637e+00, 4.8000e-02, -7.5000e-04
						};
						if (!std::isnan(convbasetemp)) {
							double tt(convbasetemp - IcaoAtmosphere<double>::degc_to_kelvin), t(1);
							convbaselwc = 0;
							for (unsigned int i(0), n(sizeof(lwccldbasepoly)/sizeof(lwccldbasepoly[0])); i < n; ++i) {
								convbaselwc += lwccldbasepoly[i] * t;
								t *= tt;
							}
						}
					}
					static const double lwc293k(17.281);
					// mmHg->Pa: *133.322368 ; Pa = kg·m^−1·s^−2
					// R=8.3144621 m^2·kg·s^-2·K^-1·mol^-1
					// m=18.01528 g/mol
					static const double lwcc1(20.386+log(18.01528*133.322368/8.3144621));
					static const double lwcc2(-5132);
					if (GRIB2::WeatherProfilePoint::altitudes[i-1] >= pi->get_cldconvbase() &&
					    GRIB2::WeatherProfilePoint::altitudes[i-1] <= pi->get_cldconvtop() &&
					    surf0.get_temp() >= IcaoAtmosphere<float>::degc_to_kelvin - 20 &&
					    surf0.get_temp() <= IcaoAtmosphere<float>::degc_to_kelvin) {
						double x(1.0 / surf0.get_temp());
						x *= exp(lwcc1+lwcc2*x);
						x = (convbaselwc - x) * (1.0 / lwc293k);
						x *= 200 * sqrt((surf0.get_temp() + (20 - IcaoAtmosphere<float>::degc_to_kelvin))/20.0);
						if (x >= 0)
							ieng0 += pi->get_cldconvcover() * x;
					}
					if (GRIB2::WeatherProfilePoint::altitudes[i] >= pi->get_cldconvbase() &&
					    GRIB2::WeatherProfilePoint::altitudes[i] <= pi->get_cldconvtop() &&
					    surf1.get_temp() >= IcaoAtmosphere<float>::degc_to_kelvin - 20 &&
					    surf1.get_temp() <= IcaoAtmosphere<float>::degc_to_kelvin) {
						double x(1.0 / surf1.get_temp());
						x *= exp(lwcc1+lwcc2*x);
						x = (convbaselwc - x) * (1.0 / lwc293k);
						x *= 200 * sqrt((surf1.get_temp() + (20 - IcaoAtmosphere<float>::degc_to_kelvin))/20.0);
						if (x >= 0)
							ieng1 += pi->get_cldconvcover() * x;
					}
				}
				ieng0 *= 0.5;
				ieng1 *= 0.5;
				if (false)
					std::cerr << "icing: D" << pi->get_routedist() << ' ' << GRIB2::WeatherProfilePoint::isobaric_levels[i]
						  << "hPa ieng0 " << ieng0 << " ieng1 " << ieng1 << std::endl;
				for (unsigned int k = 0; k < 2; ++k) {
					static const float icelim[2] = { 30, 80 };
					float ilim(icelim[k]);
					if ((ieng0 <= ilim && ieng1 > ilim) || (ieng0 > ilim && ieng1 <= ilim)) {
						int32_t alt0(GRIB2::WeatherProfilePoint::altitudes[i - 1]);
						int32_t alt1(GRIB2::WeatherProfilePoint::altitudes[i]);
						int32_t alt(alt0 + (alt1 - alt0) * (ilim - ieng0) / (ieng1 - ieng0));
						icing[k].insert(LinePoint(pi - m_wxprofile.begin(), alt, ieng1 > ieng0));
						xcnt[k] += (ieng1 > ieng0) ? 1 : -1;
					}
				}
			}
			for (unsigned int k = 0; k < 2; ++k) {
				while (xcnt[k]) {
					icing[k].insert(LinePoint(pi - m_wxprofile.begin(),
								  (xcnt[k] < 0) ? 0 : GRIB2::WeatherProfilePoint::altitudes[nrsfc - 1],
								  xcnt[k] < 0));
					xcnt[k] += (xcnt[k] < 0) ? 1 : -1;
				}
			}
		}
		for (unsigned int k(0); k < 2;) {
			std::vector<LinePoint> icingarea(extract_area(icing[k], 1000));
			if (icingarea.empty()) {
				++k;
				continue;
			}
			if (icingarea.size() < 3)
				continue;
			cr->begin_new_path();
			std::vector<LinePoint>::const_iterator li(icingarea.begin()), le(icingarea.end());
			{
				double dist(xaxistime ? (m_wxprofile[li->get_x()].get_efftime() - m_wxprofile.front().get_efftime()) : m_wxprofile[li->get_x()].get_routedist());
				cr->move_to(dist, yconvert(li->get_y(), yaxis));
			}
			for (++li; li != le; ++li) {
				double dist(xaxistime ? (m_wxprofile[li->get_x()].get_efftime() - m_wxprofile.front().get_efftime()) : m_wxprofile[li->get_x()].get_routedist());
				cr->line_to(dist, yconvert(li->get_y(), yaxis));
			}
			cr->close_path();
			cr->set_matrix(mnorm);
			cr->set_source_rgb(0 / 256.0, 256 / 256.0, 160 / 256.0);
			std::vector<double> dashes;
			dashes.push_back(5);
			dashes.push_back(2);
			if (k) {
				dashes.push_back(2);
				dashes.push_back(2);
			}
			cr->set_dash(dashes, 0);
			cr->stroke_preserve();
			cr->set_source_rgba(0 / 256.0, 256 / 256.0, 160 / 256.0, 0.2);
			cr->fill();
			cr->set_matrix(mscaled);
		}
	}
	cr->unset_dash();
	// wind barbs
	if (true) {
		Cairo::LineCap lcap(cr->get_line_cap());
		Cairo::LineJoin ljoin(cr->get_line_join());
		cr->set_line_cap(Cairo::LINE_CAP_ROUND);
		cr->set_line_join(Cairo::LINE_JOIN_ROUND);
		double lastx(100);
		const unsigned int endlevels(sizeof(GRIB2::WeatherProfilePoint::isobaric_levels)/
					     sizeof(GRIB2::WeatherProfilePoint::isobaric_levels[0]));
		for (GRIB2::WeatherProfile::const_iterator pi(m_wxprofile.begin()), pe(m_wxprofile.end()); pi != pe; ++pi) {
			double dist(xaxistime ? (pi->get_efftime() - m_wxprofile.front().get_efftime()) : pi->get_routedist());
			{
				double x((dist - origindist) * scaledist);
				if (x < lastx)
					continue;
				lastx = x + 100;
			}
			double lasty(100);
			bool first(true);
			for (unsigned int idx(0); idx < endlevels; ++idx) {
				if (GRIB2::WeatherProfilePoint::isobaric_levels[idx] < 0)
					continue;
				const GRIB2::WeatherProfilePoint::Surface& surf(pi->operator[](idx));
				int32_t alt(GRIB2::WeatherProfilePoint::altitudes[idx]);
				{
					double y((yconvert(alt, yaxis) - originelev) * scaleelev);
					if (y < lasty)
						continue;
					lasty = y + 100;
				}
				// compute cloud cover
				double alpha(1);
				if (!std::isnan(pi->get_cldlowcover()) &&
				    pi->get_cldlowbase() != GRIB2::WeatherProfilePoint::invalidalt &&
				    pi->get_cldlowtop() != GRIB2::WeatherProfilePoint::invalidalt &&
				    pi->get_cldlowbase() <= alt && pi->get_cldlowtop() >= alt)
					alpha *= 1 - pi->get_cldlowcover();
				if (!std::isnan(pi->get_cldmidcover()) &&
				    pi->get_cldmidbase() != GRIB2::WeatherProfilePoint::invalidalt &&
				    pi->get_cldmidtop() != GRIB2::WeatherProfilePoint::invalidalt &&
				    pi->get_cldmidbase() <= alt && pi->get_cldmidtop() >= alt)
					alpha *= 1 - pi->get_cldmidcover();
				if (!std::isnan(pi->get_cldhighcover()) &&
				    pi->get_cldhighbase() != GRIB2::WeatherProfilePoint::invalidalt &&
				    pi->get_cldhightop() != GRIB2::WeatherProfilePoint::invalidalt &&
				    pi->get_cldhighbase() <= alt && pi->get_cldhightop() >= alt)
					alpha *= 1 - pi->get_cldhighcover();
				if (!std::isnan(pi->get_cldconvcover()) &&
				    pi->get_cldconvbase() != GRIB2::WeatherProfilePoint::invalidalt &&
				    pi->get_cldconvtop() != GRIB2::WeatherProfilePoint::invalidalt &&
				    pi->get_cldconvbase() <= alt && pi->get_cldconvtop() >= alt)
					alpha *= 1 - pi->get_cldconvcover();
				cr->save();
				cr->translate(dist, yconvert(alt, yaxis));
				cr->scale(10.0 / scaledist, -10.0 / scaleelev);
				double winddirrad(0);
				int windspeed5(0);
				{
					float wu(surf.get_uwind()), wv(surf.get_vwind());
					wu *= (-1e-3f * Point::km_to_nmi * 3600);
					wv *= (-1e-3f * Point::km_to_nmi * 3600);
					windspeed5 = Point::round<int,double>(sqrtf(wu * wu + wv * wv) * 0.2);
					winddirrad = atan2f(wu, wv);
				}
				// draw aircraft track
				static const double windradius = 1.5;
				if (!xaxistime && pi->get_routeindex() < m_route.get_nrwpt()) {
					cr->save();
					cr->rotate(m_route[pi->get_routeindex()].get_truetrack_rad());
					for (unsigned int i(0); i < 2; ++i) {
						cr->set_line_width(i ? 0.10 : 0.30);
						if (i)
							set_color_fplan(cr);
						else
							cr->set_source_rgb(1.0, 1.0, 1.0);
						cr->move_to(0, windradius);
						cr->line_to(0, -windradius);
						cr->move_to(-0.2, 0.4-windradius);
						cr->line_to(0, -windradius);
						cr->line_to(0.2, 0.4-windradius);
						cr->stroke();
					}
					cr->restore();
				}
				if (windspeed5) {
					cr->save();
					cr->rotate(M_PI - winddirrad);
					// draw twice, first white with bigger line width
					for (unsigned int i(0); i < 2; ++i) {
						cr->set_line_width(i ? 0.10 : 0.30);
						cr->set_source_rgb(i ? 0.0 : 1.0, i ? 0.0 : 1.0, i ? 0.0 : 1.0);
						double y(-windradius);
						cr->move_to(0, 0);
						cr->line_to(0, y);
						cr->stroke();
						int speed(windspeed5);
						if (speed < 10)
							y -= 0.2;
						while (speed >= 10) {
							speed -= 10;
							cr->move_to(0, y);
							cr->rel_line_to(0.4, 0.2);
							cr->rel_line_to(-0.4, 0.2);
							if (i)
								cr->fill();
							else
								cr->stroke();
							y += 0.4;
						}
						while (speed >= 2) {
							speed -= 2;
							y += 0.2;
							cr->move_to(0, y);
							cr->rel_line_to(0.4, -0.2);
							cr->stroke();
						}
						if (speed) {
							y += 0.2;
							cr->move_to(0, y);
							cr->rel_line_to(0.2, -0.1);
							cr->stroke();
						}
					}
					cr->restore();
				}
				// draw cloud circle
				static const double cldradius = 0.5;
				cr->begin_new_path();
				cr->arc(0.0, 0.0, cldradius, 0.0, 2.0 * M_PI);
				cr->close_path();
				cr->set_line_width(0.10);
				if (alpha >= 0.95) {
					cr->set_source_rgb(1.0, 1.0, 1.0);
					cr->fill_preserve();
					cr->set_source_rgb(0.0, 0.0, 0.0);
					cr->stroke();
				} else if (alpha >= 0.85) {
					cr->set_source_rgb(1.0, 1.0, 1.0);
					cr->fill_preserve();
					cr->set_source_rgb(0.0, 0.0, 0.0);
					cr->stroke();
					cr->move_to(0, cldradius);
					cr->line_to(0, -cldradius);
					cr->stroke();
				} else if (alpha >= 0.65) {
					cr->set_source_rgb(1.0, 1.0, 1.0);
					cr->fill_preserve();
					cr->set_source_rgb(0.0, 0.0, 0.0);
					cr->stroke();
					cr->begin_new_path();
					cr->arc(0.0, 0.0, cldradius, 1.5 * M_PI, 2.0 * M_PI);
					cr->line_to(0, 0);
					cr->close_path();
					cr->fill_preserve();
					cr->stroke();
				} else if (alpha >= 0.55) {
					cr->set_source_rgb(1.0, 1.0, 1.0);
					cr->fill_preserve();
					cr->set_source_rgb(0.0, 0.0, 0.0);
					cr->stroke();
					cr->begin_new_path();
					cr->arc(0.0, 0.0, cldradius, 1.5 * M_PI, 2.0 * M_PI);
					cr->line_to(0, 0);
					cr->close_path();
					cr->fill_preserve();
					cr->stroke();
					cr->move_to(0, cldradius);
					cr->line_to(0, -cldradius);
					cr->stroke();
				} else if (alpha >= 0.45) {
					cr->set_source_rgb(1.0, 1.0, 1.0);
					cr->fill_preserve();
					cr->set_source_rgb(0.0, 0.0, 0.0);
					cr->stroke();
					cr->begin_new_path();
					cr->arc(0.0, 0.0, cldradius, 1.5 * M_PI, 0.5 * M_PI);
					cr->close_path();
					cr->fill_preserve();
					cr->stroke();
				} else if (alpha >= 0.35) {
					cr->set_source_rgb(1.0, 1.0, 1.0);
					cr->fill_preserve();
					cr->set_source_rgb(0.0, 0.0, 0.0);
					cr->stroke();
					cr->begin_new_path();
					cr->arc(0.0, 0.0, cldradius, 1.5 * M_PI, 0.5 * M_PI);
					cr->close_path();
					cr->fill_preserve();
					cr->stroke();
					cr->move_to(-cldradius, 0);
					cr->line_to(cldradius, 0);
					cr->stroke();
				} else if (alpha >= 0.15) {
					cr->set_source_rgb(1.0, 1.0, 1.0);
					cr->fill_preserve();
					cr->set_source_rgb(0.0, 0.0, 0.0);
					cr->stroke();
					cr->begin_new_path();
					cr->arc(0.0, 0.0, cldradius, 1.5 * M_PI, 1.0 * M_PI);
					cr->line_to(0, 0);
					cr->close_path();
					cr->fill_preserve();
					cr->stroke();
				} else if (alpha >= 0.05) {
					cr->set_source_rgb(0.0, 0.0, 0.0);
					cr->fill_preserve();
					cr->stroke();
					cr->set_source_rgb(1.0, 1.0, 1.0);
					cr->move_to(0, -0.9 * cldradius);
					cr->line_to(0, 0.9 * cldradius);
					cr->stroke();
				} else {
					cr->set_source_rgb(0.0, 0.0, 0.0);
					cr->fill_preserve();
					cr->stroke();
				}
				// show temperature
				static const double textdist = windradius * 1.01;
				std::ostringstream oss;
				oss << Point::round<int,double>(surf.get_temp() - IcaoAtmosphere<float>::degc_to_kelvin) << "\302\260";
				cr->set_source_rgb(1.0, 1.0, 1.0);
				cr->set_font_size(1.0);
				Cairo::TextExtents ext;
				cr->get_text_extents(oss.str(), ext);
				cr->move_to(textdist, ext.height * 0.5);
				cr->set_line_width(0.15);
				cr->text_path(oss.str());
				cr->stroke();
				cr->set_source_rgb(1.0, 0.0, 0.0);
				cr->move_to(textdist, ext.height * 0.5);
				cr->show_text(oss.str());
				if (first && !std::isnan(pi->get_liftedindex()) && pi->get_liftedindex() <= -0.5) {
					cr->set_source_rgb(1.0, 1.0, 1.0);
					std::ostringstream oss;
			        	oss << "\342\230\210 " << std::fixed << std::setprecision(1) << pi->get_liftedindex();
					cr->move_to(textdist, ext.height * 1.7);
					cr->set_line_width(0.15);
					cr->text_path(oss.str());
					cr->stroke();
					if (pi->get_liftedindex() <= -6.5)
						// violent
						cr->set_source_rgb(1.0, 0.0, 1.0);
					else if (pi->get_liftedindex() <= -5.5)
						// severe
						cr->set_source_rgb(1.0, 0.0, 0.0);
					else if (pi->get_liftedindex() <= -3.5)
						// probable
						cr->set_source_rgb(1.0, 1.0, 0.0);
        				else if (pi->get_liftedindex() <= -0.5)
						// possible
						cr->set_source_rgb(0.0, 1.0, 0.0);
					else
						// unlikely
						cr->set_source_rgb(0.0, 0.0, 0.0);
					cr->move_to(textdist, ext.height * 1.7);
					cr->show_text(oss.str());
				}
				cr->restore();
				first = false;
			}
		}
    		cr->set_line_cap(lcap);
		cr->set_line_join(ljoin);
	}
	cr->set_matrix(mscaled);
	// draw elev
	if (xaxistime) {
		double yg(yconvert(0, yaxis));
		double ye(yconvert(m_routeprofile.front().get_elev() * Point::m_to_ft, yaxis));
		double dist(m_wxprofile.back().get_efftime() - m_wxprofile.front().get_efftime());
		cr->begin_new_path();
		cr->move_to(0, yg);
		cr->line_to(dist, yg);
		cr->line_to(dist, ye);
		cr->line_to(0, ye);
		cr->close_path();
	} else {
		double yg(yconvert(0, yaxis));
		cr->begin_new_path();
		cr->move_to(0, yg);
		double dist(0);
		for (TopoDb30::RouteProfile::const_iterator pi(m_routeprofile.begin()), pe(m_routeprofile.end()); pi != pe; ++pi)
			cr->line_to(dist = pi->get_routedist(), yconvert(pi->get_elev() * Point::m_to_ft, yaxis));
		cr->line_to(dist, yg);
		cr->close_path();
	}
	cr->set_matrix(mnorm);
	set_color_terrain_fill_elev(cr);
	cr->fill_preserve();
	set_color_terrain(cr);
	cr->stroke();
	cr->restore();
	// draw scales
	cr->set_line_width(0.5);
	{
		std::vector<double> dashes;
		dashes.push_back(2);
		dashes.push_back(2);
		cr->set_dash(dashes, 0);
 	}
	if (true && yaxis == yaxis_pressure) {
		typedef std::map<double,int32_t> grids_t;
		grids_t grids;
		{
			static const double ymindiff(60);
			double yheight(height - dist_height);
			int32_t altinc(10000), altmin(0), altmax(90000);
			for (unsigned int i = 0; altinc >= 10 && i < 20; ++i) {
				bool work(false);
				for (int32_t alt(altmin); alt <= altmax; alt += altinc) {
					double y(yheight - (yconvert(alt, yaxis) - originelev) * scaleelev);
					if (false)
						std::cerr << "alt " << alt << " y " << y << " yh " << yheight << std::endl;
					if (y < 0) {
						altmax = std::min(altmax, alt);
						continue;
					}
					if (y > yheight) {
						altmin = std::max(altmin, alt);
						continue;
					}
					grids_t::iterator gi(grids.lower_bound(y));
					if (gi != grids.end() && (gi->first - y) < ymindiff)
						continue;
					if (gi != grids.begin()) {
						grids_t::iterator gi2(gi);
						--gi2;
						if ((y - gi2->first) < ymindiff)
							continue;
					}
					work = true;
					grids.insert(gi, grids_t::value_type(y, alt));
				}
				if (!work && !grids.empty())
					break;
				if ((i % 3) == 1)
					altinc = (altinc << 1) / 5;
				else
					altinc >>= 1;
			}
		}
		for (grids_t::const_iterator gi(grids.begin()), ge(grids.end()); gi != ge; ++gi) {
			cr->move_to(alt_width, gi->first);
			cr->line_to(width, gi->first);
			set_color_grid(cr);
			cr->stroke();
			set_color_marking(cr);
			std::ostringstream oss;
			oss << std::fixed << std::setprecision(0) << (gi->second * 0.01);
			Cairo::TextExtents ext;
			cr->get_text_extents(oss.str(), ext);
			cr->move_to(alt_width - 2 - ext.width - ext.x_bearing, gi->first - ext.height - 2 - ext.y_bearing);
			cr->show_text(oss.str());
			std::ostringstream ossp;
			{
				float p;
				IcaoAtmosphere<float>::std_altitude_to_pressure(&p, 0, gi->second * Point::ft_to_m);
				ossp << std::fixed << std::setprecision(0) << p;
			}
			cr->set_font_size(8);
			cr->get_text_extents(ossp.str(), ext);
			cr->move_to(alt_width - 2 - ext.width - ext.x_bearing, gi->first + 2 - ext.y_bearing);
			cr->show_text(ossp.str());
			cr->set_font_size(12);
		}
	} else {
		for (;; elevstart += elevinc) {
			if (yaxis == yaxis_pressure && elevstart <= 10)
				break;
			double y(height - dist_height - (elevstart - originelev) * scaleelev);
			if (y < 0)
				break;
			cr->move_to(alt_width, y);
			cr->line_to(width, y);
			set_color_grid(cr);
			cr->stroke();
			set_color_marking(cr);
			std::ostringstream oss;
			switch (yaxis) {
			default:
				oss << std::fixed << std::setprecision(0) << (elevstart * 0.01);
				break;

			case yaxis_pressure:
			{
				float a;
				IcaoAtmosphere<float>::std_pressure_to_altitude(&a, 0, elevstart);
				oss << std::fixed << std::setprecision(0) << (a * (Point::m_to_ft_dbl * 0.01));
				break;
			}
			}
			Cairo::TextExtents ext;
			cr->get_text_extents(oss.str(), ext);
			cr->move_to(alt_width - 2 - ext.width - ext.x_bearing, y - ext.height - 2 - ext.y_bearing);
			cr->show_text(oss.str());
			std::ostringstream ossp;
			switch (yaxis) {
			default:
			{
				float p;
				IcaoAtmosphere<float>::std_altitude_to_pressure(&p, 0, elevstart * Point::ft_to_m);
				ossp << std::fixed << std::setprecision(0) << p;
				break;
			}

			case yaxis_pressure:
				ossp << std::fixed << std::setprecision(0) << elevstart;
				break;
			}
			cr->set_font_size(8);
			cr->get_text_extents(ossp.str(), ext);
			cr->move_to(alt_width - 2 - ext.width - ext.x_bearing, y + 2 - ext.y_bearing);
			cr->show_text(ossp.str());
			cr->set_font_size(12);
		}
	}
	for (;; diststart += distinc) {
		double x(alt_width + (diststart - origindist) * scaledist);
		if (x >= width)
			break;
		cr->move_to(x, 0);
		cr->line_to(x, height - dist_height);
		set_color_grid(cr);
		cr->stroke();
		set_color_marking(cr);
		if (xaxistime) {
			std::ostringstream oss;
			oss << Glib::TimeVal(m_wxprofile.front().get_efftime() + diststart, 0).as_iso8601().substr(11, 5);
			Cairo::TextExtents ext;
			cr->get_text_extents(oss.str(), ext);
			double lblx(x - ext.width / 2U - ext.x_bearing);
			cr->move_to(lblx, height - 2U * dist_line_height + 2U - ext.y_bearing);
			cr->show_text(oss.str());
		} else {
			std::ostringstream oss;
			oss << std::fixed << std::setprecision(0) << diststart;
			Cairo::TextExtents ext;
			cr->get_text_extents(oss.str(), ext);
			double lblx(x - ext.width / 2U - ext.x_bearing);
			cr->move_to(lblx, height - 2U * dist_line_height + 2U - ext.y_bearing);
			cr->show_text(oss.str());
			lblx += ext.width;
			gint64 efftime(-1);
			{
				double bd(std::numeric_limits<double>::max());
				for (GRIB2::WeatherProfile::const_iterator pi(m_wxprofile.begin()), pe(m_wxprofile.end()); pi != pe; ++pi) {
					double d(fabs(pi->get_routedist() - diststart));
					if (d > bd)
						continue;
					bd = d;
					efftime = pi->get_efftime();
				}
			}
			if (efftime < 0)
				continue;
			double h12(ext.height);
			std::string efftstr("  " + Glib::TimeVal(efftime, 0).as_iso8601().substr(11, 5));
			cr->set_font_size(8);
			cr->get_text_extents(efftstr, ext);
			cr->move_to(lblx, height - 2U * dist_line_height + 2U - ext.y_bearing + h12 - ext.height);
			cr->show_text(efftstr);
			cr->set_font_size(12);
		}
	}
        cr->unset_dash();
	cr->restore();
	// normalize and draw route
	if (!xaxistime) {
		for (bool usetruealt(false); ; ) {
			altflag_t curaltflags(altflags & (usetruealt ? altflag_calculated : altflag_planned));
			if (curaltflags) {
				cr->save();
				cr->rectangle(alt_width, 0, width - alt_width, height - dist_height);
				set_color_marking(cr);
				cr->set_line_width(1);
				cr->stroke_preserve();
				cr->clip();
				set_color_fplan(cr);
				cr->set_line_width((curaltflags & altflag_fat) ? 2 : 1);
				cr->set_matrix(mscaled);
				bool first(true);
				cr->begin_new_path();
				for (unsigned int i = 0, j = m_route.get_nrwpt(); i < j; ++i) {
					double dist(std::numeric_limits<double>::quiet_NaN());
					for (TopoDb30::RouteProfile::const_iterator pi(m_routeprofile.begin()), pe(m_routeprofile.end()); pi != pe; ++pi) {
						if (pi->get_routeindex() != i)
							continue;
						dist = pi->get_routedist();
						break;
					}
					if (std::isnan(dist))
						continue;
					double y((usetruealt && (xaxistime || (i > 0 && i + 1 < j) || m_route[i].get_type() != FPlanWaypoint::type_airport))
						 ? m_route[i].get_truealt() : m_route[i].get_true_altitude());
					y = yconvert(y, yaxis);
					if (first)
						cr->move_to(dist, y);
					else
						cr->line_to(dist, y);
					first = false;
				}
				cr->set_matrix(mnorm);
				cr->stroke();
				cr->restore();
			}
			if (usetruealt)
				break;
			usetruealt = true;
		}
	}
	// end
}

MeteoChart::AreaExtract::ScalarGridPt::ScalarGridPt()
	: m_val(std::numeric_limits<float>::min()), m_inside(false)
{
	m_coord.set_invalid();
}

MeteoChart::AreaExtract::ScalarGridPt::ScalarGridPt(const Glib::RefPtr<GRIB2::LayerInterpolateResult>& lay, unsigned int x, unsigned int y,
						    float lim, gint64 efftime, double sfc1value)
	: m_val(std::numeric_limits<float>::min()), m_inside(false)
{
	m_coord.set_invalid();
	if (!lay)
		return;
	m_coord = lay->get_center(x, y);
	float v(lay->operator()(x, y, efftime, sfc1value));
	if (std::isnan(v))
		return;
	m_val = v;
	m_inside = m_val >= lim;
}

MeteoChart::AreaExtract::ScalarGridPt::ScalarGridPt(const ScalarGridPt& x, const ScalarGridPt& y, float lim)
	: m_val((x.get_val() + y.get_val()) * 0.5), m_inside(false)
{
	m_coord = x.get_coord().halfway(y.get_coord());
	m_inside = m_val >= lim;
}

Point MeteoChart::AreaExtract::ScalarGridPt::contour_point(const ScalarGridPt& x, float lim) const
{
	if (is_inside() == x.is_inside())
		throw std::runtime_error("ScalarGridPt::contour_point");
	float t((lim - get_val()) / (x.get_val() - get_val()));
	t = std::max(0.f, std::min(1.f, t));
	Point pdiff(x.get_coord() - get_coord());
	return get_coord() + Point(pdiff.get_lon() * t, pdiff.get_lat() * t);
}

MeteoChart::AreaExtract::WindGridPt::WindGridPt()
{
	m_val = 0;
}

MeteoChart::AreaExtract::WindGridPt::WindGridPt(const Glib::RefPtr<GRIB2::LayerInterpolateResult>& layu,
						const Glib::RefPtr<GRIB2::LayerInterpolateResult>& layv,
						unsigned int x, unsigned int y, float lim, gint64 efftime, double sfc1value)
{
	m_val = 0;
	if (!layu || !layv)
		return;
	m_coord = layu->get_center(x, y);
	float u(layu->operator()(x, y, efftime, sfc1value));
	float v(layv->operator()(x, y, efftime, sfc1value));
	if (std::isnan(u) || std::isnan(v))
		return;
	m_val = sqrtf(u * u + v * v);
	m_inside = m_val >= lim;
}

MeteoChart::AreaExtract::WindGridPt::WindGridPt(const WindGridPt& x, const WindGridPt& y, float lim)
	: ScalarGridPt(x, y, lim)
{
}

constexpr float MeteoChart::AreaExtract::CloudGridPt::mincover;

MeteoChart::AreaExtract::CloudGridPt::CloudGridPt()
	: m_cover(0), m_base(0), m_top(200000), m_inside(false)
{
	m_coord.set_invalid();
}

MeteoChart::AreaExtract::CloudGridPt::CloudGridPt(const Glib::RefPtr<GRIB2::LayerInterpolateResult>& laycover,
						  const Glib::RefPtr<GRIB2::LayerInterpolateResult>& laybase,
						  const Glib::RefPtr<GRIB2::LayerInterpolateResult>& laytop,
						  unsigned int x, unsigned int y, gint64 efftime, float pressure)
	: m_cover(0), m_base(0), m_top(200000), m_inside(false)
{
	m_coord.set_invalid();
	if (!laycover || !laybase || !laytop)
		return;
	m_coord = laycover->get_center(x, y);
	float c(laycover->operator()(x, y, efftime, 0));
	float b(laybase->operator()(x, y, efftime, 0));
	float t(laytop->operator()(x, y, efftime, 0));
	if (std::isnan(c) || !GRIB2::WeatherProfilePoint::is_pressure_valid(b) || !GRIB2::WeatherProfilePoint::is_pressure_valid(t))
		return;
	m_cover = std::max(0.f, std::min(1.f, c * 0.01f));
	m_base = b;
	m_top = t;
	m_inside = m_cover >= mincover && pressure <= m_base && pressure >= m_top;
}

MeteoChart::AreaExtract::CloudGridPt::CloudGridPt(const CloudGridPt& x, const CloudGridPt& y, float pressure)
	: m_cover((x.get_cover() + y.get_cover()) * 0.5), m_base((x.get_base() + y.get_base()) * 0.5),
	  m_top((x.get_top() + y.get_top()) * 0.5), m_inside(false)
{
	m_coord = x.get_coord().halfway(y.get_coord());
	m_inside = m_cover >= mincover && pressure <= m_base && pressure >= m_top;
}

Point MeteoChart::AreaExtract::CloudGridPt::contour_point(const CloudGridPt& x, float pressure) const
{
	if (is_inside() == x.is_inside())
		throw std::runtime_error("CloudGridPt::contour_point");
	float tc((mincover - get_cover()) / (x.get_cover() - get_cover()));
	float tb((pressure - get_base()) / (x.get_base() - get_base()));
	float tt((pressure - get_top()) / (x.get_top() - get_top()));
	tc = std::max(0.f, std::min(1.f, tc));
	tb = std::max(0.f, std::min(1.f, tb));
	tt = std::max(0.f, std::min(1.f, tt));
	float t;
	if (is_inside())
		t = std::min(tc, std::min(tb, tt));
	else
		t = std::max(tc, std::max(tb, tt));
	Point pdiff(x.get_coord() - get_coord());
	return get_coord() + Point(pdiff.get_lon() * t, pdiff.get_lat() * t);
}

template <typename PT> MeteoChart::AreaExtract::Grid<PT>::Grid(unsigned int w, unsigned int h)
	: m_width(w), m_height(h)
{
	if (m_width < 4 || m_height < 4) {
		m_width = m_height = 0;
		return;
	}
	m_v.resize(m_width * m_height);
}

template <typename PT> PT& MeteoChart::AreaExtract::Grid<PT>::operator()(unsigned int x, unsigned int y)
{
	if (x >= get_width() || y >= get_height())
		return *(PT *)0;
	return m_v[y * get_width() + x];
}

template <typename PT> const PT& MeteoChart::AreaExtract::Grid<PT>::operator()(unsigned int x, unsigned int y) const
{
	if (x >= get_width() || y >= get_height())
		return *(PT *)0;
	return m_v[y * get_width() + x];
}

template <typename PT> void MeteoChart::AreaExtract::Grid<PT>::set_border(void)
{
	if (m_width < 4 || m_height < 4)
		return;
	Point pdiff0(operator()(2, 1).get_coord() - operator()(1, 1).get_coord());
	Point pdiff1(operator()(1, 2).get_coord() - operator()(1, 1).get_coord());
	for (unsigned int x = 2; x < get_width(); ++x) {
		operator()(x - 1, 0).set_coord(operator()(x - 1, 1).get_coord() - pdiff1);
		operator()(x - 1, get_height() - 1).set_coord(operator()(x - 1, get_height() - 2).get_coord() + pdiff1);
	}
	for (unsigned int y = 2; y < get_height(); ++y) {
		operator()(0, y - 1).set_coord(operator()(1, y - 1).get_coord() - pdiff0);
		operator()(get_width() - 1, y - 1).set_coord(operator()(get_width() - 2, y - 1).get_coord() + pdiff0);
	}
	operator()(0, 0).set_coord(operator()(1, 0).get_coord() - pdiff0);
	operator()(get_width() - 1, 0).set_coord(operator()(get_width() - 2, 0).get_coord() + pdiff0);
	operator()(0, get_height() - 1).set_coord(operator()(1, get_height() - 1).get_coord() - pdiff0);
	operator()(get_width() - 1, get_height() - 1).set_coord(operator()(get_width() - 2, get_height() - 1).get_coord() + pdiff0);
}

template <typename PT> unsigned int MeteoChart::AreaExtract::Grid<PT>::get_index(unsigned int x, unsigned int y) const
{
	if (x >= get_width() || y >= get_height())
		return ~0U;
	return y * get_width() + x;
}

template <typename PT> unsigned int MeteoChart::AreaExtract::Grid<PT>::get_edgenr(unsigned int x0, unsigned int y0, unsigned int x1, unsigned int y1) const
{
	if ((x0 != x1 && y0 != y1) ||
	    (x0 == x1 && y0 == y1))
		throw std::runtime_error("Grid::get_edgenr");
	if (x0 > x1)
		std::swap(x0, x1);
	if (y0 > y1)
		std::swap(y0, y1);
	if (x1 - x0 > 1 || y1 - y0 > 1)
		throw std::runtime_error("Grid::get_edgenr");
	return (get_index(x0, y0) << 1) | (y1 > y0);
}

int MeteoChart::AreaExtract::Edge::compare(const Edge& x) const
{
	if (get_startedge() < x.get_startedge())
		return -1;
	if (x.get_startedge() < get_startedge())
		return 1;
	if (get_endedge() < x.get_endedge())
		return -1;
	if (x.get_endedge() < get_endedge())
		return 1;
	return 0;
}

MeteoChart::AreaExtract::AreaExtract(const Glib::RefPtr<GRIB2::LayerInterpolateResult>& lay, float lim, gint64 efftime, double sfc1value)
{
	if (!lay)
		return;
	unsigned int width(lay->get_width());
	unsigned int height(lay->get_height());
	if (width < 2 || height < 2)
		return;

	Grid<ScalarGridPt> grid(width + 2, height + 2);
	for (unsigned int x(0); x < width; ++x)
		for (unsigned int y(0); y < height; ++y)
			grid(x + 1, y + 1) = ScalarGridPt(lay, x, y, lim, efftime, sfc1value);
	grid.set_border();
	compute(grid, lim);
}

MeteoChart::AreaExtract::AreaExtract(const Glib::RefPtr<GRIB2::LayerInterpolateResult>& layu,
				     const Glib::RefPtr<GRIB2::LayerInterpolateResult>& layv, float lim, gint64 efftime, double sfc1value)
{
	if (!layu || !layv)
		return;
	if (layu->get_bbox() != layv->get_bbox())
		return;
	unsigned int width(layu->get_width());
	unsigned int height(layu->get_height());
	if (layv->get_width() != width || layv->get_height() != height)
		return;
	if (width < 2 || height < 2)
		return;
	Grid<WindGridPt> grid(width + 2, height + 2);
	for (unsigned int x(0); x < width; ++x)
		for (unsigned int y(0); y < height; ++y)
			grid(x + 1, y + 1) = WindGridPt(layu, layv, x, y, lim, efftime, sfc1value);
	grid.set_border();
	compute(grid, lim);
}

MeteoChart::AreaExtract::AreaExtract(const Glib::RefPtr<GRIB2::LayerInterpolateResult>& laycover,
				     const Glib::RefPtr<GRIB2::LayerInterpolateResult>& laybase,
				     const Glib::RefPtr<GRIB2::LayerInterpolateResult>& laytop, gint64 efftime, double pressure)
{
	if (!laycover || !laybase || !laytop)
		return;
	if (laycover->get_bbox() != laybase->get_bbox() || laycover->get_bbox() != laytop->get_bbox())
		return;
	unsigned int width(laycover->get_width());
	unsigned int height(laycover->get_height());
	if (laybase->get_width() != width || laybase->get_height() != height ||
	    laytop->get_width() != width || laytop->get_height() != height)
		return;
	if (width < 2 || height < 2)
		return;
	Grid<CloudGridPt> grid(width + 2, height + 2);
	for (unsigned int x(0); x < width; ++x)
		for (unsigned int y(0); y < height; ++y)
			grid(x + 1, y + 1) = CloudGridPt(laycover, laybase, laytop, x, y, efftime, pressure);
	grid.set_border();
	compute(grid, pressure);
	if (false) {
		std::cerr << "Area Extract: pressure " << pressure << " efftime " << Glib::TimeVal(efftime, 0).as_iso8601() << std::endl
			  << "  Cover: " << laycover->get_layer()->get_parameter()->get_str_nonnull()
			  << " sfc1 " << GRIB2::find_surfacetype_str(laycover->get_layer()->get_surface1type(), "?") << ' '
			  << laycover->get_minsurface1value() << "..." << laycover->get_maxsurface1value() << std::endl
			  << "  Base: " << laybase->get_layer()->get_parameter()->get_str_nonnull()
			  << " sfc1 " << GRIB2::find_surfacetype_str(laybase->get_layer()->get_surface1type(), "?") << ' '
			  << laybase->get_minsurface1value() << "..." << laybase->get_maxsurface1value() << std::endl
			  << "  Top: " << GRIB2::find_surfacetype_str(laytop->get_layer()->get_surface1type(), "?") << ' '
			  << laytop->get_layer()->get_parameter()->get_str_nonnull()
			  << " sfc1 " << laytop->get_minsurface1value() << "..." << laytop->get_maxsurface1value() << std::endl;
		for (unsigned int x(0); x < width + 2; ++x)
			for (unsigned int y(0); y < height + 2; ++y) {
				const CloudGridPt& p(grid(x, y));
				float cc(laycover->operator()(p.get_coord(), efftime, 0) * 0.01f);
				float pb(laybase->operator()(p.get_coord(), efftime, 0));
				float pt(laytop->operator()(p.get_coord(), efftime, 0));
				std::cerr << "  x " << x << " y " << y << " c " << p.get_cover() << " p " << p.get_base() << "..." << p.get_top()
					  << ' ' << (p.is_inside() ? 'T' : 'F') << ' ' << " [c " << cc << " p " << pb << "..." << pt << "] "
					  << p.get_coord().get_lat_str2() << ' ' << p.get_coord().get_lon_str2() << std::endl;
				if (std::isnan(cc)) {
					GRIB2::LayerInterpolateResult::LinInterp intp(laycover->operator()(p.get_coord()));
					std::cerr << "  cover: " << intp[0] << ' ' << intp[1] << ' ' << intp[2] << ' ' << intp[3] << std::endl;
				}
				if (std::isnan(pb)) {
					GRIB2::LayerInterpolateResult::LinInterp intp(laybase->operator()(p.get_coord()));
					std::cerr << "  base: " << intp[0] << ' ' << intp[1] << ' ' << intp[2] << ' ' << intp[3] << std::endl;
				}
				if (std::isnan(pt)) {
					GRIB2::LayerInterpolateResult::LinInterp intp(laytop->operator()(p.get_coord()));
					std::cerr << "  top: " << intp[0] << ' ' << intp[1] << ' ' << intp[2] << ' ' << intp[3] << std::endl;
				}
			}
		std::cerr << std::endl;
	}
}

template <typename PT> void MeteoChart::AreaExtract::compute(Grid<PT>& grid, float lim)
{
	unsigned int width(grid.get_width()), height(grid.get_height());
	if (width < 2 || height < 2)
		return;
	width -= 2;
	height -= 2;
	for (unsigned int x(0); x <= width; ++x)
		for (unsigned int y(0); y <= height; ++y) {
			// Ordering of Points:
			// 0 1
			// 3 2
			unsigned int xx[4], yy[4];
			for (unsigned int i = 0; i < 4; ++i) {
				xx[i] = x + ((i ^ (i >> 1)) & 1);
				yy[i] = y + ((i >> 1) & 1);
			}
			for (unsigned int i = 0; i < 4; ++i) {
				PT& p0(grid(xx[i & 3], yy[i & 3]));
				PT& p1(grid(xx[(i + 1) & 3], yy[(i + 1) & 3]));
				PT& p2(grid(xx[(i + 2) & 3], yy[(i + 2) & 3]));
				PT& p3(grid(xx[(i + 3) & 3], yy[(i + 3) & 3]));
				if (i < 2 && p0.is_inside() && !p1.is_inside() && p2.is_inside() && !p3.is_inside()) {
					PT mid(p0, p2, lim);
					if (!mid.is_inside()) {
						m_edges.insert(Edge(p3.contour_point(p0, lim), grid.get_edgenr(xx[(i + 3) & 3], yy[(i + 3) & 3], xx[i & 3], yy[i & 3]),
								    p1.contour_point(p0, lim), grid.get_edgenr(xx[(i + 1) & 3], yy[(i + 1) & 3], xx[i & 3], yy[i & 3]),
								    mid.contour_point(p0, lim)));
						m_edges.insert(Edge(p1.contour_point(p2, lim), grid.get_edgenr(xx[(i + 1) & 3], yy[(i + 1) & 3], xx[(i + 2) & 3], yy[(i + 2) & 3]),
								    p3.contour_point(p2, lim), grid.get_edgenr(xx[(i + 3) & 3], yy[(i + 3) & 3], xx[(i + 2) & 3], yy[(i + 2) & 3]),
								    mid.contour_point(p2, lim)));
						break;
					}
					m_edges.insert(Edge(p1.contour_point(p2, lim), grid.get_edgenr(xx[(i + 1) & 3], yy[(i + 1) & 3], xx[(i + 2) & 3], yy[(i + 2) & 3]),
							    p1.contour_point(p0, lim), grid.get_edgenr(xx[(i + 1) & 3], yy[(i + 1) & 3], xx[i & 3], yy[i & 3]),
							    p1.contour_point(mid, lim)));
					m_edges.insert(Edge(p3.contour_point(p0, lim), grid.get_edgenr(xx[(i + 3) & 3], yy[(i + 3) & 3], xx[i & 3], yy[i & 3]),
							    p3.contour_point(p2, lim), grid.get_edgenr(xx[(i + 3) & 3], yy[(i + 3) & 3], xx[(i + 2) & 3], yy[(i + 2) & 3]),
							    p3.contour_point(mid, lim)));
					break;
				}
				if (p0.is_inside() && !p1.is_inside() && !p2.is_inside() && !p3.is_inside()) {
					m_edges.insert(Edge(p3.contour_point(p0, lim), grid.get_edgenr(xx[(i + 3) & 3], yy[(i + 3) & 3], xx[i & 3], yy[i & 3]),
							    p1.contour_point(p0, lim), grid.get_edgenr(xx[(i + 1) & 3], yy[(i + 1) & 3], xx[i & 3], yy[i & 3]),
							    p2.contour_point(p0, lim)));
					break;
				}
				if (p0.is_inside() && p1.is_inside() && !p2.is_inside() && !p3.is_inside()) {
					m_edges.insert(Edge(p3.contour_point(p0, lim), grid.get_edgenr(xx[(i + 3) & 3], yy[(i + 3) & 3], xx[i & 3], yy[i & 3]),
							    p2.contour_point(p1, lim), grid.get_edgenr(xx[(i + 2) & 3], yy[(i + 2) & 3], xx[(i + 1) & 3], yy[(i + 1) & 3])));
					break;
				}
				if (p0.is_inside() && p1.is_inside() && p2.is_inside() && !p3.is_inside()) {
					m_edges.insert(Edge(p0.contour_point(p3, lim), grid.get_edgenr(xx[i & 3], yy[i & 3], xx[(i + 3) & 3], yy[(i + 3) & 3]),
							    p2.contour_point(p3, lim), grid.get_edgenr(xx[(i + 2) & 3], yy[(i + 2) & 3], xx[(i + 3) & 3], yy[(i + 3) & 3]),
							    p1.contour_point(p3, lim)));
					break;
				}
			}
		}
}

std::pair<float,float> MeteoChart::AreaExtract::minmax(const Glib::RefPtr<GRIB2::LayerInterpolateResult>& lay, gint64 efftime, double sfc1value)
{
	std::pair<float,float> r(std::numeric_limits<float>::max(), std::numeric_limits<float>::min());
	if (!lay)
		return r;
	unsigned int width(lay->get_width());
	unsigned int height(lay->get_height());
	for (unsigned int x(0); x < width; ++x)
		for (unsigned int y(0); y < height; ++y) {
			float v(lay->operator()(x, y, efftime, sfc1value));
			if (std::isnan(v))
				continue;
			r.first = std::min(r.first, v);
			r.second = std::max(r.second, v);
		}
	return r;
}

std::pair<float,float> MeteoChart::AreaExtract::minmax(const Glib::RefPtr<GRIB2::LayerInterpolateResult>& layu,
						       const Glib::RefPtr<GRIB2::LayerInterpolateResult>& layv, gint64 efftime, double sfc1value)
{
	std::pair<float,float> r(std::numeric_limits<float>::max(), std::numeric_limits<float>::min());
	if (!layu || !layv)
		return r;
	if (layu->get_bbox() != layv->get_bbox())
		return r;
	unsigned int width(layu->get_width());
	unsigned int height(layu->get_height());
	if (layv->get_width() != width || layv->get_height() != height)
		return r;
	for (unsigned int x(0); x < width; ++x)
		for (unsigned int y(0); y < height; ++y) {
			float u(layu->operator()(x, y, efftime, sfc1value));
			float v(layv->operator()(x, y, efftime, sfc1value));
			if (std::isnan(u) || std::isnan(v))
				continue;
			float w(sqrtf(u * u + v * v));
			r.first = std::min(r.first, w);
			r.second = std::max(r.second, w);
		}
	return r;
}

std::vector<Point> MeteoChart::AreaExtract::extract_contour(void)
{
	if (m_edges.empty())
		return std::vector<Point>();
	std::vector<Edge> edges;
	edges.push_back(*m_edges.begin());
	m_edges.erase(m_edges.begin());
	if (false)
		std::cerr << "Edge: " << edges.back().get_startedge() << " -> " << edges.back().get_endedge()
			  << ' ' << edges.back().get_start().get_lat_str2() << ' ' << edges.back().get_start().get_lon_str2()
			  << " -> " << edges.back().get_end().get_lat_str2() << ' ' << edges.back().get_end().get_lon_str2()
			  << " (*)" << std::endl;
	for (;;) {
		edges_t::iterator i(m_edges.lower_bound(Edge(Point::invalid, edges.back().get_endedge(), Point::invalid, 0)));
		if (i == m_edges.end() || i->get_startedge() != edges.back().get_endedge())
			break;
		edges.push_back(*i);
		m_edges.erase(i);
		if (false)
			std::cerr << "Edge: " << edges.back().get_startedge() << " -> " << edges.back().get_endedge()
				  << ' ' << edges.back().get_start().get_lat_str2() << ' ' << edges.back().get_start().get_lon_str2()
				  << " -> " << edges.back().get_end().get_lat_str2() << ' ' << edges.back().get_end().get_lon_str2() << std::endl;
	}
	std::vector<Point> p;
	for (std::vector<Edge>::const_iterator i(edges.begin()), e(edges.end()); i != e; ++i) {
		p.push_back(i->get_start());
		if (!i->get_intermediate().is_invalid())
			p.push_back(i->get_intermediate());
	}
	if (edges.front().get_startedge() != edges.back().get_endedge()) {
		std::cerr << "AreaExtract::extract_contour: warning: non-closed contour" << std::endl;
		p.push_back(edges.back().get_end());
	}
	return p;
}

void MeteoChart::AreaExtract::extract_contours(const Cairo::RefPtr<Cairo::Context>& cr, const Point& center, double scalelon, double scalelat)
{
	if (!cr)
		return;
	for (;;) {
		std::vector<Point> pts(extract_contour());
		if (false) {
			std::cerr << "Contour: " << pts.size() << std::endl;
			if (false)
				for (std::vector<Point>::const_iterator pi(pts.begin()), pe(pts.end()); pi != pe; ++pi)
					std::cerr << "  " << pi->get_lat_str2() << ' ' << pi->get_lon_str2() << std::endl;
		}
		if (pts.size() < 3)
			break;
		{
			float d(pts.front().simple_distance_nmi(pts.back()));
			if (d > 50)
				std::cerr << "Contour distance: " << d << std::endl;
		}
		bool first(true);
		for (std::vector<Point>::const_iterator pi(pts.begin()), pe(pts.end()); pi != pe; ++pi) {
			Point pdiff(*pi - center);
			double x(pdiff.get_lon() * scalelon);
			double y(pdiff.get_lat() * scalelat);
			if (first) {
				cr->move_to(x, y);
				first = false;
			} else {
				cr->line_to(x, y);
			}
		}
		cr->close_path();
	}
}

MeteoChart::MeteoChart(const FPlanRoute& route, const alternates_t& altn, const altroutes_t& altnfpl, const GRIB2& wxdb)
	: m_route(route), m_altn(altn), m_altnroutes(altnfpl), m_wxdb(&wxdb),
	  m_minefftime(std::numeric_limits<gint64>::max()),
	  m_maxefftime(std::numeric_limits<gint64>::min()),
	  m_minreftime(std::numeric_limits<gint64>::max()),
	  m_maxreftime(std::numeric_limits<gint64>::min()),
	  m_gndchart(false)
{
}

void MeteoChart::clear(void)
{
	m_wxwindu = Glib::RefPtr<GRIB2::LayerInterpolateResult>();
	m_wxwindv = Glib::RefPtr<GRIB2::LayerInterpolateResult>();
	m_wxprmsl = Glib::RefPtr<GRIB2::LayerInterpolateResult>();
	m_wxtemperature = Glib::RefPtr<GRIB2::LayerInterpolateResult>();
	m_wxtropopause = Glib::RefPtr<GRIB2::LayerInterpolateResult>();
	m_wxcldbdrycover = Glib::RefPtr<GRIB2::LayerInterpolateResult>();
	m_wxcldlowcover = Glib::RefPtr<GRIB2::LayerInterpolateResult>();
	m_wxcldlowbase = Glib::RefPtr<GRIB2::LayerInterpolateResult>();
	m_wxcldlowtop = Glib::RefPtr<GRIB2::LayerInterpolateResult>();
	m_wxcldmidcover = Glib::RefPtr<GRIB2::LayerInterpolateResult>();
	m_wxcldmidbase = Glib::RefPtr<GRIB2::LayerInterpolateResult>();
	m_wxcldmidtop = Glib::RefPtr<GRIB2::LayerInterpolateResult>();
	m_wxcldhighcover = Glib::RefPtr<GRIB2::LayerInterpolateResult>();
	m_wxcldhighbase = Glib::RefPtr<GRIB2::LayerInterpolateResult>();
	m_wxcldhightop = Glib::RefPtr<GRIB2::LayerInterpolateResult>();
	m_wxcldconvcover = Glib::RefPtr<GRIB2::LayerInterpolateResult>();
	m_wxcldconvbase = Glib::RefPtr<GRIB2::LayerInterpolateResult>();
	m_wxcldconvtop = Glib::RefPtr<GRIB2::LayerInterpolateResult>();
	m_minefftime = std::numeric_limits<gint64>::max();
	m_maxefftime = std::numeric_limits<gint64>::min();
	m_minreftime = std::numeric_limits<gint64>::max();
	m_maxreftime = std::numeric_limits<gint64>::min();
}

void MeteoChart::loaddb(const Rect& bbox, gint64 efftime, double pressure)
{
	if (!m_wxdb) {
		clear();
		return;
	}
	m_minefftime = std::numeric_limits<gint64>::max();
	m_maxefftime = std::numeric_limits<gint64>::min();
	m_minreftime = std::numeric_limits<gint64>::max();
	m_maxreftime = std::numeric_limits<gint64>::min();
	Rect bbox1(bbox.oversize_nmi(200));
	bool gndchart(pressure <= 0);
	if (gndchart != m_gndchart) {
		m_gndchart = gndchart;
		m_wxwindu = m_wxwindv = m_wxtemperature = m_wxprmsl = Glib::RefPtr<GRIB2::LayerInterpolateResult>();
	}
	if (m_gndchart) {
		if (!m_wxwindu || efftime < m_wxwindu->get_minefftime() || efftime > m_wxwindu->get_maxefftime() ||
		    !m_wxwindu->get_bbox().is_inside(bbox)) {
			GRIB2::layerlist_t ll(const_cast<GRIB2 *>(m_wxdb)->find_layers(GRIB2::find_parameter(GRIB2::param_meteorology_momentum_ugrd),
										       efftime, GRIB2::surface_specific_height_gnd, 100));
			m_wxwindu = GRIB2::interpolate_results(bbox1, ll, efftime, 100);
		}
		if (!m_wxwindv || efftime < m_wxwindv->get_minefftime() || efftime > m_wxwindv->get_maxefftime() ||
		    !m_wxwindv->get_bbox().is_inside(bbox)) {
			GRIB2::layerlist_t ll(const_cast<GRIB2 *>(m_wxdb)->find_layers(GRIB2::find_parameter(GRIB2::param_meteorology_momentum_vgrd),
										       efftime, GRIB2::surface_specific_height_gnd, 100));
			m_wxwindv = GRIB2::interpolate_results(bbox1, ll, efftime, 100);
		}
		if (!m_wxtemperature || efftime < m_wxtemperature->get_minefftime() || efftime > m_wxtemperature->get_maxefftime() ||
		    !m_wxtemperature->get_bbox().is_inside(bbox)) {
			GRIB2::layerlist_t ll(const_cast<GRIB2 *>(m_wxdb)->find_layers(GRIB2::find_parameter(GRIB2::param_meteorology_temperature_tmp),
										       efftime, GRIB2::surface_ground_or_water, 0));
			m_wxtemperature = GRIB2::interpolate_results(bbox1, ll, efftime);
                }
		if (!m_wxprmsl || efftime < m_wxprmsl->get_minefftime() || efftime > m_wxprmsl->get_maxefftime() ||
		    !m_wxprmsl->get_bbox().is_inside(bbox)) {
			GRIB2::layerlist_t ll(const_cast<GRIB2 *>(m_wxdb)->find_layers(GRIB2::find_parameter(GRIB2::param_meteorology_mass_prmsl),
										       efftime));
			m_wxprmsl = GRIB2::interpolate_results(bbox1, ll, efftime);
                }
	} else {
		if (!m_wxwindu || efftime < m_wxwindu->get_minefftime() || efftime > m_wxwindu->get_maxefftime() ||
		    pressure < m_wxwindu->get_minsurface1value() || pressure > m_wxwindu->get_maxsurface1value() ||
		    !m_wxwindu->get_bbox().is_inside(bbox)) {
			GRIB2::layerlist_t ll(const_cast<GRIB2 *>(m_wxdb)->find_layers(GRIB2::find_parameter(GRIB2::param_meteorology_momentum_ugrd),
										       efftime, GRIB2::surface_isobaric_surface, pressure));
			m_wxwindu = GRIB2::interpolate_results(bbox1, ll, efftime, pressure);
		}
		if (!m_wxwindv || efftime < m_wxwindv->get_minefftime() || efftime > m_wxwindv->get_maxefftime() ||
		    pressure < m_wxwindv->get_minsurface1value() || pressure > m_wxwindv->get_maxsurface1value() ||
		    !m_wxwindv->get_bbox().is_inside(bbox)) {
			GRIB2::layerlist_t ll(const_cast<GRIB2 *>(m_wxdb)->find_layers(GRIB2::find_parameter(GRIB2::param_meteorology_momentum_vgrd),
										       efftime, GRIB2::surface_isobaric_surface, pressure));
			m_wxwindv = GRIB2::interpolate_results(bbox1, ll, efftime, pressure);
		}
		if (!m_wxtemperature || efftime < m_wxtemperature->get_minefftime() || efftime > m_wxtemperature->get_maxefftime() ||
		    pressure < m_wxtemperature->get_minsurface1value() || pressure > m_wxtemperature->get_maxsurface1value() ||
		    !m_wxtemperature->get_bbox().is_inside(bbox)) {
			GRIB2::layerlist_t ll(const_cast<GRIB2 *>(m_wxdb)->find_layers(GRIB2::find_parameter(GRIB2::param_meteorology_temperature_tmp),
										       efftime, GRIB2::surface_isobaric_surface, pressure));
			m_wxtemperature = GRIB2::interpolate_results(bbox1, ll, efftime, pressure);
                }
		if (!m_wxprmsl || efftime < m_wxprmsl->get_minefftime() || efftime > m_wxprmsl->get_maxefftime() ||
		    pressure < m_wxprmsl->get_minsurface1value() || pressure > m_wxprmsl->get_maxsurface1value() ||
		    !m_wxprmsl->get_bbox().is_inside(bbox)) {
			GRIB2::layerlist_t ll(const_cast<GRIB2 *>(m_wxdb)->find_layers(GRIB2::find_parameter(GRIB2::param_meteorology_mass_hgt),
										       efftime, GRIB2::surface_isobaric_surface, pressure));
			m_wxprmsl = GRIB2::interpolate_results(bbox1, ll, efftime, pressure);
                }
	}
	if (m_wxwindu) {
		m_minefftime = std::min(m_minefftime, m_wxwindu->get_minefftime());
		m_maxefftime = std::max(m_maxefftime, m_wxwindu->get_maxefftime());
		m_minreftime = std::min(m_minreftime, m_wxwindu->get_minreftime());
		m_maxreftime = std::max(m_maxreftime, m_wxwindu->get_maxreftime());
	}
	if (m_wxwindv) {
		m_minefftime = std::min(m_minefftime, m_wxwindv->get_minefftime());
		m_maxefftime = std::max(m_maxefftime, m_wxwindv->get_maxefftime());
		m_minreftime = std::min(m_minreftime, m_wxwindv->get_minreftime());
		m_maxreftime = std::max(m_maxreftime, m_wxwindv->get_maxreftime());
	}
	if (m_wxtemperature) {
		m_minefftime = std::min(m_minefftime, m_wxtemperature->get_minefftime());
		m_maxefftime = std::max(m_maxefftime, m_wxtemperature->get_maxefftime());
		m_minreftime = std::min(m_minreftime, m_wxtemperature->get_minreftime());
		m_maxreftime = std::max(m_maxreftime, m_wxtemperature->get_maxreftime());
	}
	if (m_wxprmsl) {
		m_minefftime = std::min(m_minefftime, m_wxprmsl->get_minefftime());
		m_maxefftime = std::max(m_maxefftime, m_wxprmsl->get_maxefftime());
		m_minreftime = std::min(m_minreftime, m_wxprmsl->get_minreftime());
		m_maxreftime = std::max(m_maxreftime, m_wxprmsl->get_maxreftime());
	}
	if (!m_wxtropopause || efftime < m_wxtropopause->get_minefftime() || efftime > m_wxtropopause->get_maxefftime() ||
	    !m_wxtropopause->get_bbox().is_inside(bbox)) {
		GRIB2::layerlist_t ll(const_cast<GRIB2 *>(m_wxdb)->find_layers(GRIB2::find_parameter(GRIB2::param_meteorology_mass_hgt),
									       efftime, GRIB2::surface_tropopause, 0));
		m_wxtropopause = GRIB2::interpolate_results(bbox1, ll, efftime);
	}
	if (m_wxtropopause) {
		m_minefftime = std::min(m_minefftime, m_wxtropopause->get_minefftime());
		m_maxefftime = std::max(m_maxefftime, m_wxtropopause->get_maxefftime());
		m_minreftime = std::min(m_minreftime, m_wxtropopause->get_minreftime());
		m_maxreftime = std::max(m_maxreftime, m_wxtropopause->get_maxreftime());
	}
	if (!m_wxcldbdrycover || efftime < m_wxcldbdrycover->get_minefftime() || efftime > m_wxcldbdrycover->get_maxefftime() ||
	    !m_wxcldbdrycover->get_bbox().is_inside(bbox)) {
		GRIB2::layerlist_t ll(const_cast<GRIB2 *>(m_wxdb)->find_layers(GRIB2::find_parameter(GRIB2::param_meteorology_cloud_tcdc),
									       efftime, GRIB2::surface_boundary_layer_cloud, 0));
		m_wxcldbdrycover = GRIB2::interpolate_results(bbox1, ll, efftime);
	}
	if (m_wxcldbdrycover) {
		m_minefftime = std::min(m_minefftime, m_wxcldbdrycover->get_minefftime());
		m_maxefftime = std::max(m_maxefftime, m_wxcldbdrycover->get_maxefftime());
		m_minreftime = std::min(m_minreftime, m_wxcldbdrycover->get_minreftime());
		m_maxreftime = std::max(m_maxreftime, m_wxcldbdrycover->get_maxreftime());
	}
	if (!m_wxcldlowcover || efftime < m_wxcldlowcover->get_minefftime() || efftime > m_wxcldlowcover->get_maxefftime() ||
	    !m_wxcldlowcover->get_bbox().is_inside(bbox)) {
		GRIB2::layerlist_t ll(const_cast<GRIB2 *>(m_wxdb)->find_layers(GRIB2::find_parameter(GRIB2::param_meteorology_cloud_tcdc),
									       efftime, GRIB2::surface_low_cloud, 0));
		m_wxcldlowcover = GRIB2::interpolate_results(bbox1, ll, efftime);
	}
	if (m_wxcldlowcover) {
		m_minefftime = std::min(m_minefftime, m_wxcldlowcover->get_minefftime());
		m_maxefftime = std::max(m_maxefftime, m_wxcldlowcover->get_maxefftime());
		m_minreftime = std::min(m_minreftime, m_wxcldlowcover->get_minreftime());
		m_maxreftime = std::max(m_maxreftime, m_wxcldlowcover->get_maxreftime());
	}
	if (!m_wxcldlowbase || efftime < m_wxcldlowbase->get_minefftime() || efftime > m_wxcldlowbase->get_maxefftime() ||
	    !m_wxcldlowbase->get_bbox().is_inside(bbox)) {
		GRIB2::layerlist_t ll(const_cast<GRIB2 *>(m_wxdb)->find_layers(GRIB2::find_parameter(GRIB2::param_meteorology_mass_pres),
									       efftime, GRIB2::surface_low_cloud_bottom, 0));
		m_wxcldlowbase = GRIB2::interpolate_results(bbox1, ll, efftime);
	}
	if (m_wxcldlowbase) {
		m_minefftime = std::min(m_minefftime, m_wxcldlowbase->get_minefftime());
		m_maxefftime = std::max(m_maxefftime, m_wxcldlowbase->get_maxefftime());
		m_minreftime = std::min(m_minreftime, m_wxcldlowbase->get_minreftime());
		m_maxreftime = std::max(m_maxreftime, m_wxcldlowbase->get_maxreftime());
	}
	if (!m_wxcldlowtop || efftime < m_wxcldlowtop->get_minefftime() || efftime > m_wxcldlowtop->get_maxefftime() ||
	    !m_wxcldlowtop->get_bbox().is_inside(bbox)) {
		GRIB2::layerlist_t ll(const_cast<GRIB2 *>(m_wxdb)->find_layers(GRIB2::find_parameter(GRIB2::param_meteorology_mass_pres),
									       efftime, GRIB2::surface_low_cloud_top, 0));
		m_wxcldlowtop = GRIB2::interpolate_results(bbox1, ll, efftime);
	}
	if (m_wxcldlowtop) {
		m_minefftime = std::min(m_minefftime, m_wxcldlowtop->get_minefftime());
		m_maxefftime = std::max(m_maxefftime, m_wxcldlowtop->get_maxefftime());
		m_minreftime = std::min(m_minreftime, m_wxcldlowtop->get_minreftime());
		m_maxreftime = std::max(m_maxreftime, m_wxcldlowtop->get_maxreftime());
	}
	if (!m_wxcldmidcover || efftime < m_wxcldmidcover->get_minefftime() || efftime > m_wxcldmidcover->get_maxefftime() ||
	    !m_wxcldmidcover->get_bbox().is_inside(bbox)) {
		GRIB2::layerlist_t ll(const_cast<GRIB2 *>(m_wxdb)->find_layers(GRIB2::find_parameter(GRIB2::param_meteorology_cloud_tcdc),
									       efftime, GRIB2::surface_middle_cloud, 0));
		m_wxcldmidcover = GRIB2::interpolate_results(bbox1, ll, efftime);
	}
	if (m_wxcldmidcover) {
		m_minefftime = std::min(m_minefftime, m_wxcldmidcover->get_minefftime());
		m_maxefftime = std::max(m_maxefftime, m_wxcldmidcover->get_maxefftime());
		m_minreftime = std::min(m_minreftime, m_wxcldmidcover->get_minreftime());
		m_maxreftime = std::max(m_maxreftime, m_wxcldmidcover->get_maxreftime());
	}
	if (!m_wxcldmidbase || efftime < m_wxcldmidbase->get_minefftime() || efftime > m_wxcldmidbase->get_maxefftime() ||
	    !m_wxcldmidbase->get_bbox().is_inside(bbox)) {
		GRIB2::layerlist_t ll(const_cast<GRIB2 *>(m_wxdb)->find_layers(GRIB2::find_parameter(GRIB2::param_meteorology_mass_pres),
									       efftime, GRIB2::surface_middle_cloud_bottom, 0));
		m_wxcldmidbase = GRIB2::interpolate_results(bbox1, ll, efftime);
	}
	if (m_wxcldmidbase) {
		m_minefftime = std::min(m_minefftime, m_wxcldmidbase->get_minefftime());
		m_maxefftime = std::max(m_maxefftime, m_wxcldmidbase->get_maxefftime());
		m_minreftime = std::min(m_minreftime, m_wxcldmidbase->get_minreftime());
		m_maxreftime = std::max(m_maxreftime, m_wxcldmidbase->get_maxreftime());
	}
	if (!m_wxcldmidtop || efftime < m_wxcldmidtop->get_minefftime() || efftime > m_wxcldmidtop->get_maxefftime() ||
	    !m_wxcldmidtop->get_bbox().is_inside(bbox)) {
		GRIB2::layerlist_t ll(const_cast<GRIB2 *>(m_wxdb)->find_layers(GRIB2::find_parameter(GRIB2::param_meteorology_mass_pres),
									       efftime, GRIB2::surface_middle_cloud_top, 0));
		m_wxcldmidtop = GRIB2::interpolate_results(bbox1, ll, efftime);
	}
	if (m_wxcldmidtop) {
		m_minefftime = std::min(m_minefftime, m_wxcldmidtop->get_minefftime());
		m_maxefftime = std::max(m_maxefftime, m_wxcldmidtop->get_maxefftime());
		m_minreftime = std::min(m_minreftime, m_wxcldmidtop->get_minreftime());
		m_maxreftime = std::max(m_maxreftime, m_wxcldmidtop->get_maxreftime());
	}
	if (!m_wxcldhighcover || efftime < m_wxcldhighcover->get_minefftime() || efftime > m_wxcldhighcover->get_maxefftime() ||
	    !m_wxcldhighcover->get_bbox().is_inside(bbox)) {
		GRIB2::layerlist_t ll(const_cast<GRIB2 *>(m_wxdb)->find_layers(GRIB2::find_parameter(GRIB2::param_meteorology_cloud_tcdc),
									       efftime, GRIB2::surface_top_cloud, 0));
		m_wxcldhighcover = GRIB2::interpolate_results(bbox1, ll, efftime);
	}
	if (m_wxcldhighcover) {
		m_minefftime = std::min(m_minefftime, m_wxcldhighcover->get_minefftime());
		m_maxefftime = std::max(m_maxefftime, m_wxcldhighcover->get_maxefftime());
		m_minreftime = std::min(m_minreftime, m_wxcldhighcover->get_minreftime());
		m_maxreftime = std::max(m_maxreftime, m_wxcldhighcover->get_maxreftime());
	}
	if (!m_wxcldhighbase || efftime < m_wxcldhighbase->get_minefftime() || efftime > m_wxcldhighbase->get_maxefftime() ||
	    !m_wxcldhighbase->get_bbox().is_inside(bbox)) {
		GRIB2::layerlist_t ll(const_cast<GRIB2 *>(m_wxdb)->find_layers(GRIB2::find_parameter(GRIB2::param_meteorology_mass_pres),
									       efftime, GRIB2::surface_top_cloud_bottom, 0));
		m_wxcldhighbase = GRIB2::interpolate_results(bbox1, ll, efftime);
	}
	if (m_wxcldhighbase) {
		m_minefftime = std::min(m_minefftime, m_wxcldhighbase->get_minefftime());
		m_maxefftime = std::max(m_maxefftime, m_wxcldhighbase->get_maxefftime());
		m_minreftime = std::min(m_minreftime, m_wxcldhighbase->get_minreftime());
		m_maxreftime = std::max(m_maxreftime, m_wxcldhighbase->get_maxreftime());
	}
	if (!m_wxcldhightop || efftime < m_wxcldhightop->get_minefftime() || efftime > m_wxcldhightop->get_maxefftime() ||
	    !m_wxcldhightop->get_bbox().is_inside(bbox)) {
		GRIB2::layerlist_t ll(const_cast<GRIB2 *>(m_wxdb)->find_layers(GRIB2::find_parameter(GRIB2::param_meteorology_mass_pres),
									       efftime, GRIB2::surface_top_cloud_top, 0));
		m_wxcldhightop = GRIB2::interpolate_results(bbox1, ll, efftime);
	}
	if (m_wxcldhightop) {
		m_minefftime = std::min(m_minefftime, m_wxcldhightop->get_minefftime());
		m_maxefftime = std::max(m_maxefftime, m_wxcldhightop->get_maxefftime());
		m_minreftime = std::min(m_minreftime, m_wxcldhightop->get_minreftime());
		m_maxreftime = std::max(m_maxreftime, m_wxcldhightop->get_maxreftime());
	}
	if (!m_wxcldconvcover || efftime < m_wxcldconvcover->get_minefftime() || efftime > m_wxcldconvcover->get_maxefftime() ||
	    !m_wxcldconvcover->get_bbox().is_inside(bbox)) {
		GRIB2::layerlist_t ll(const_cast<GRIB2 *>(m_wxdb)->find_layers(GRIB2::find_parameter(GRIB2::param_meteorology_cloud_tcdc),
									       efftime, GRIB2::surface_convective_cloud, 0));
		m_wxcldconvcover = GRIB2::interpolate_results(bbox1, ll, efftime);
	}
	if (m_wxcldconvcover) {
		m_minefftime = std::min(m_minefftime, m_wxcldconvcover->get_minefftime());
		m_maxefftime = std::max(m_maxefftime, m_wxcldconvcover->get_maxefftime());
		m_minreftime = std::min(m_minreftime, m_wxcldconvcover->get_minreftime());
		m_maxreftime = std::max(m_maxreftime, m_wxcldconvcover->get_maxreftime());
	}
	if (!m_wxcldconvbase || efftime < m_wxcldconvbase->get_minefftime() || efftime > m_wxcldconvbase->get_maxefftime() ||
	    !m_wxcldconvbase->get_bbox().is_inside(bbox)) {
		GRIB2::layerlist_t ll(const_cast<GRIB2 *>(m_wxdb)->find_layers(GRIB2::find_parameter(GRIB2::param_meteorology_mass_pres),
									       efftime, GRIB2::surface_convective_cloud_bottom, 0));
		m_wxcldconvbase = GRIB2::interpolate_results(bbox1, ll, efftime);
	}
	if (m_wxcldconvbase) {
		m_minefftime = std::min(m_minefftime, m_wxcldconvbase->get_minefftime());
		m_maxefftime = std::max(m_maxefftime, m_wxcldconvbase->get_maxefftime());
		m_minreftime = std::min(m_minreftime, m_wxcldconvbase->get_minreftime());
		m_maxreftime = std::max(m_maxreftime, m_wxcldconvbase->get_maxreftime());
	}
	if (!m_wxcldconvtop || efftime < m_wxcldconvtop->get_minefftime() || efftime > m_wxcldconvtop->get_maxefftime() ||
	    !m_wxcldconvtop->get_bbox().is_inside(bbox)) {
		GRIB2::layerlist_t ll(const_cast<GRIB2 *>(m_wxdb)->find_layers(GRIB2::find_parameter(GRIB2::param_meteorology_mass_pres),
									       efftime, GRIB2::surface_convective_cloud_top, 0));
		m_wxcldconvtop = GRIB2::interpolate_results(bbox1, ll, efftime);
	}
	if (m_wxcldconvtop) {
		m_minefftime = std::min(m_minefftime, m_wxcldconvtop->get_minefftime());
		m_maxefftime = std::max(m_maxefftime, m_wxcldconvtop->get_maxefftime());
		m_minreftime = std::min(m_minreftime, m_wxcldconvtop->get_minreftime());
		m_maxreftime = std::max(m_maxreftime, m_wxcldconvtop->get_maxreftime());
	}
}

void MeteoChart::get_fullscale(int width, int height, Point& center, double& scalelon, double& scalelat) const
{
	center = Point::invalid;
	scalelon = scalelat = std::numeric_limits<double>::quiet_NaN();
	if (width <= 20 || height <= 20 || m_route.get_nrwpt() < 2)
		return;
	width -= 20;
	height -= 20;
	Rect bbox(m_route.get_bbox());
	for (alternates_t::const_iterator ai(m_altn.begin()), ae(m_altn.end()); ai != ae; ++ai) {
		if (ai->get_coord().is_invalid())
			continue;
		bbox = bbox.add(ai->get_coord());
	}
	for (altroutes_t::const_iterator ai(m_altnroutes.begin()), ae(m_altnroutes.end()); ai != ae; ++ai) {
		Rect bbox2(ai->get_bbox());
		bbox = bbox.add(bbox2);
	}
	bbox = bbox.oversize_nmi(50);
	center = bbox.boxcenter();
	Point psz(bbox.get_northeast() - bbox.get_southwest());
	double c(std::max(0.01, cos(center.get_lat_rad())));
	double sc(std::min(width / c / (double)(uint32_t)psz.get_lon(),
			   height / (double)(uint32_t)psz.get_lat()));
	scalelon = sc * c;
	scalelat = -sc;
	if (false)
		std::cerr << "MeteoChart::get_fullscale: " << width << 'x' << height << ' '
			  << center.get_lat_str2() << ' ' << center.get_lon_str2() << ' '
			  << scalelat << ' ' << scalelon << std::endl;
}

gint64 MeteoChart::get_midefftime(void) const
{
	if (m_route.get_nrwpt() < 1)
		return 0;
	time_t tst(m_route[0].get_time_unix());
	time_t td(m_route[m_route.get_nrwpt()-1].get_time_unix());
	td -= tst;
	return tst + td / 2;
}

void MeteoChart::draw(const Cairo::RefPtr<Cairo::Context>& cr, int width, int height,
		      const Point& center, double scalelon, double scalelat, double pressure,
		      const std::string& servicename)
{
	if (false)
		std::cerr << "MeteoChart::draw: " << width << 'x' << height << ' '
			  << center.get_lat_str2() << ' ' << center.get_lon_str2() << ' '
			  << scalelat << ' ' << scalelon << ' ' << pressure << std::endl;
	cr->save();
	// background
	cr->set_source_rgb(1.0, 1.0, 1.0);
	cr->paint();
	// scaling
	cr->select_font_face("sans", Cairo::FONT_SLANT_NORMAL, Cairo::FONT_WEIGHT_NORMAL);
	cr->set_font_size(12);
	// check for database
	if (!m_wxdb) {
		cr->set_source_rgb(0.0, 0.0, 0.0);
		cr->move_to(width * 0.5, height * 0.5);
		cr->show_text("No Data");
		cr->restore();
		return;
	}
	if (m_route.get_nrwpt() < 2) {
		cr->set_source_rgb(0.0, 0.0, 0.0);
		cr->move_to(width * 0.5, height * 0.5);
		cr->show_text("No Flight Plan");
		cr->restore();
		return;
	}
	Rect bbox;
	{
		Point phsz(width * 0.5 / fabs(scalelat), height * 0.5 / fabs(scalelon));
		bbox = Rect(center - phsz, center + phsz).oversize_nmi(100);
	}
	gint64 efftime(get_midefftime());
	loaddb(bbox, efftime, pressure);
	// draw chart legend
	int32_t altitude(0);
	if (!m_gndchart) {
		float alt;
		IcaoAtmosphere<float>::std_pressure_to_altitude(&alt, 0, pressure * 0.01);
		altitude = Point::round<int32_t,float>(alt * Point::m_to_ft);
	}
	{
		cr->set_source_rgb(0.0, 0.0, 0.0);
		std::ostringstream oss;
		if (m_gndchart)
			oss << "GND";
		else
			oss << std::fixed << std::setprecision(0) << (pressure * 0.01) << "hPa (F" << std::setw(3)
			    << std::setfill('0') << ((altitude + 50) / 100) << ')';
		if (m_route.get_nrwpt() > 0) {
			oss << " \342\200\242 ";
			const FPlanWaypoint& wpt(m_route[0]);
			if (wpt.get_icao().empty() || (wpt.get_type() == FPlanWaypoint::type_airport && AirportsDb::Airport::is_fpl_zzzz(wpt.get_icao())))
				oss << wpt.get_name();
			else
				oss << wpt.get_icao();
			std::string tm0(Glib::TimeVal(wpt.get_time_unix(), 0).as_iso8601());
			oss << ' ' << tm0.substr(0, 10) << ' ' << tm0.substr(11, 5) << 'Z';
			if (m_route.get_nrwpt() > 1) {
				const FPlanWaypoint& wpt(m_route[m_route.get_nrwpt() - 1]);
				oss << " \342\206\222 ";
				if (wpt.get_icao().empty() || (wpt.get_type() == FPlanWaypoint::type_airport && AirportsDb::Airport::is_fpl_zzzz(wpt.get_icao())))
					oss << wpt.get_name();
				else
					oss << wpt.get_icao();
				std::string tm1(Glib::TimeVal(wpt.get_time_unix(), 0).as_iso8601());
				oss << ' ';
				if (tm0.substr(0, 10) != tm1.substr(0, 10))
					oss << tm1.substr(0, 10) << ' ';
				oss << tm1.substr(11, 5) << 'Z';
			}
		}
		if (get_minreftime() > get_maxreftime()) {
			oss << " \342\200\242 GFS Data not available";
		} else {
			std::string tm0(Glib::TimeVal(get_minreftime(), 0).as_iso8601());
			oss << " \342\200\242 GFS RefTime " << tm0.substr(0, 10) << ' ' << tm0.substr(11, 5) << 'Z';
			if (get_maxreftime() != get_minreftime()) {
				std::string tm1(Glib::TimeVal(get_maxreftime(), 0).as_iso8601());
				oss << '-';
				if (tm0.substr(0, 10) != tm1.substr(0, 10))
					oss << tm1.substr(0, 10) << ' ';
				oss << tm1.substr(11, 5) << 'Z';
			}
		}
		std::ostringstream oss2;
		if (get_minefftime() <= get_maxefftime()) {
			std::string tm0(Glib::TimeVal(get_minefftime(), 0).as_iso8601());
			oss2 << " * EffTime " << tm0.substr(0, 10) << ' ' << tm0.substr(11, 5) << 'Z';
			if (get_maxefftime() != get_minefftime()) {
				std::string tm1(Glib::TimeVal(get_maxefftime(), 0).as_iso8601());
				oss2 << '-';
				if (tm0.substr(0, 10) != tm1.substr(0, 10))
					oss2 << tm1.substr(0, 10) << ' ';
				oss2 << tm1.substr(11, 5) << 'Z';
			}
		}
		static const char arident[] = " \342\200\242 ";
		Cairo::TextExtents ext;
		cr->get_text_extents(oss.str() + oss2.str() + arident + servicename, ext);
		if (ext.width <= width)
			oss << oss2.str();
		oss << arident << servicename;
		cr->get_text_extents(oss.str(), ext);
		cr->move_to(0 - ext.x_bearing, height - ext.height - 2U - ext.y_bearing);
		cr->show_text(oss.str());
		height -= ext.height + 9U;
	}
	// border
	cr->rectangle(0, 0, width, height);
	set_color_marking(cr);
	cr->set_line_width(2);
	cr->stroke_preserve();
	cr->clip();
	cr->translate(width * 0.5, height * 0.5);
	// grid
	{
		Point pgrid;
		pgrid.set_lon_deg(0.5);
		pgrid.set_lat_deg(0.5);
		static const double minpixel(100);
		static const double coordpx(10);
		for (unsigned int i = 0; i < 7; ++i) {
			if (fabs(pgrid.get_lon() * scalelon) >= minpixel)
				break;
			if ((i - 2) % 3)
				pgrid.set_lon(pgrid.get_lon() << 1);
			else
				pgrid.set_lon((pgrid.get_lon() << 1) + (pgrid.get_lon() >> 1));
		}
		for (unsigned int i = 0; i < 7; ++i) {
			if (fabs(pgrid.get_lat() * scalelat) >= minpixel)
				break;
			if ((i - 2) % 3)
				pgrid.set_lat(pgrid.get_lat() << 1);
			else
				pgrid.set_lat((pgrid.get_lat() << 1) + (pgrid.get_lat() >> 1));
		}
		Point pst(bbox.get_southwest());
		pst.set_lon(pst.get_lon() / pgrid.get_lon() * pgrid.get_lon());
		pst.set_lat(pst.get_lat() / pgrid.get_lat() * pgrid.get_lat());
		Point plen(bbox.get_northeast() - pst);
		// Lon Grid
		for (Point::coord_t cd(0); cd < plen.get_lon(); cd += pgrid.get_lon()) {
			Point::coord_t coord(pst.get_lon() + cd);
			double xc((coord - center.get_lon()) * scalelon);
			std::ostringstream oss;
			{
				int min(Point::round<int,double>(Point(coord, coord).get_lon_deg_dbl() * 60));
				if (min < -180 * 60)
					min += 360 * 60;
				else if (min > 180 * 60)
					min -= 360 * 60;
				if (min < 0) {
					oss << 'W';
					min = -min;
				} else {
					oss << 'E';
				}
				unsigned int deg(min);
				deg /= 60;
				min -= deg * 60;
				oss << std::setw(3) << std::setfill('0') << deg;
				if (min)
					oss << ' ' << std::setw(2) << std::setfill('0') << min;
			}
			Cairo::TextExtents ext;
			cr->get_text_extents(oss.str(), ext);
			cr->set_source_rgb(0.0, 0.0, 0.0);
			cr->set_line_width(2);
			cr->move_to(xc, -0.5 * height);
			cr->rel_line_to(0, coordpx);
			cr->move_to(xc, 0.5 * height);
			cr->rel_line_to(0, -coordpx);
			cr->stroke();
			cr->move_to(xc - ext.width * 0.5 - ext.x_bearing, coordpx - 0.5 * height + 1 - ext.y_bearing);
			cr->show_text(oss.str());
			cr->move_to(xc - ext.width * 0.5 - ext.x_bearing, 0.5 * height - coordpx - 1 - ext.height - ext.y_bearing);
			cr->show_text(oss.str());
			if (false)
				std::cerr << "Lon Grid: " << Point(coord, coord).get_lon_str2() << " xc " << xc
					  << ' ' << oss.str() << std::endl;
		}
		// Lat Grid
		for (Point::coord_t cd(0); cd < plen.get_lat(); cd += pgrid.get_lat()) {
			Point::coord_t coord(pst.get_lat() + cd);
			double yc((coord - center.get_lat()) * scalelat);
			std::ostringstream oss;
			{
				int min(Point::round<int,double>(Point(coord, coord).get_lat_deg_dbl() * 60));
				if (min < 0) {
					oss << 'S';
					min = -min;
				} else {
					oss << 'N';
				}
				unsigned int deg(min);
				deg /= 60;
				min -= deg * 60;
				oss << std::setw(2) << std::setfill('0') << deg;
				if (min)
					oss << ' ' << std::setw(2) << std::setfill('0') << min;
			}
			Cairo::TextExtents ext;
			cr->get_text_extents(oss.str(), ext);
			cr->set_source_rgb(0.0, 0.0, 0.0);
			cr->set_line_width(2);
			cr->move_to(-0.5 * width, yc);
			cr->rel_line_to(coordpx, 0);
			cr->move_to(0.5 * width, yc);
			cr->rel_line_to(-coordpx, 0);
			cr->stroke();
			cr->move_to(coordpx - 0.5 * width + 1 - ext.x_bearing, yc - ext.height * 0.5 - ext.y_bearing);
			cr->show_text(oss.str());
			cr->move_to(0.5 * width - coordpx - 1 - ext.width - ext.x_bearing, yc - ext.height * 0.5 - ext.y_bearing);
			cr->show_text(oss.str());
			if (false)
				std::cerr << "Lat Grid: " << Point(coord, coord).get_lat_str2() << " yc " << yc
					  << ' ' << oss.str() << std::endl;
		}
		// wind barbs
		for (Point::coord_t cdlon(0); cdlon < plen.get_lon(); cdlon += pgrid.get_lon()) {
			for (Point::coord_t cdlat(0); cdlat < plen.get_lat(); cdlat += pgrid.get_lat()) {
				Point pt(pst.get_lon() + cdlon, pst.get_lat() + cdlat);
				double xc, yc;
				{
					Point pdiff(pt - center);
					xc = pdiff.get_lon() * scalelon;
					yc = pdiff.get_lat() * scalelat;
				}
				if (fabs(xc) > 0.5 * width - 30 || fabs(yc) > 0.5 * height - 30)
					continue;
				std::ostringstream ossflg;
				// compute cloud cover
				double alpha(1);
				if (m_gndchart) {
					if (m_wxcldbdrycover) {
						float v(m_wxcldbdrycover->operator()(pt, efftime, 0));
						if (!std::isnan(v))
							alpha = 1 - std::min(1.f, std::max(0.f, v * 0.01f));
					}
				} else {
					if (m_wxcldlowcover && m_wxcldlowbase && m_wxcldlowtop) {
						float cover(m_wxcldlowcover->operator()(pt, efftime, 0));
						float base(m_wxcldlowbase->operator()(pt, efftime, 0));
						float top(m_wxcldlowtop->operator()(pt, efftime, 0));
						if (!std::isnan(cover) &&
						    GRIB2::WeatherProfilePoint::is_pressure_valid(base) &&
						    GRIB2::WeatherProfilePoint::is_pressure_valid(top) &&
						    pressure <= base && pressure >= top)
						       	alpha *= 1 - std::min(1.f, std::max(0.f, cover * 0.01f));
					}
					if (m_wxcldmidcover && m_wxcldmidbase && m_wxcldmidtop) {
						float cover(m_wxcldmidcover->operator()(pt, efftime, 0));
						float base(m_wxcldmidbase->operator()(pt, efftime, 0));
						float top(m_wxcldmidtop->operator()(pt, efftime, 0));
						if (!std::isnan(cover) &&
						    GRIB2::WeatherProfilePoint::is_pressure_valid(base) &&
						    GRIB2::WeatherProfilePoint::is_pressure_valid(top) &&
						    pressure <= base && pressure >= top)
						       	alpha *= 1 - std::min(1.f, std::max(0.f, cover * 0.01f));
					}
					if (m_wxcldhighcover && m_wxcldhighbase && m_wxcldhightop) {
						float cover(m_wxcldhighcover->operator()(pt, efftime, 0));
						float base(m_wxcldhighbase->operator()(pt, efftime, 0));
						float top(m_wxcldhightop->operator()(pt, efftime, 0));
						if (!std::isnan(cover) &&
						    GRIB2::WeatherProfilePoint::is_pressure_valid(base) &&
						    GRIB2::WeatherProfilePoint::is_pressure_valid(top) &&
						    pressure <= base && pressure >= top)
						       	alpha *= 1 - std::min(1.f, std::max(0.f, cover * 0.01f));
					}
					if (m_wxcldconvcover && m_wxcldconvbase && m_wxcldconvtop) {
						float cover(m_wxcldconvcover->operator()(pt, efftime, 0));
						float base(m_wxcldconvbase->operator()(pt, efftime, 0));
						float top(m_wxcldconvtop->operator()(pt, efftime, 0));
						if (!std::isnan(cover) &&
						    GRIB2::WeatherProfilePoint::is_pressure_valid(base) &&
						    GRIB2::WeatherProfilePoint::is_pressure_valid(top) &&
						    pressure <= base && pressure >= top) {
							cover = std::min(1.f, std::max(0.f, cover * 0.01f));
						       	alpha *= 1 - cover;
							if (cover >= 0.25)
								ossflg << "CB";
							else if (cover >= 0.1)
								ossflg << "ISOL CB";
						}
					}
				}
				cr->save();
				cr->translate(xc, yc);
				cr->scale(10.0, 10.0);
				double winddirrad(0);
				int windspeed5(0);
				if (m_wxwindu && m_wxwindv) {
					float wu(m_wxwindu->operator()(pt, efftime, m_gndchart ? 100 : pressure));
					float wv(m_wxwindv->operator()(pt, efftime, m_gndchart ? 100 : pressure));
					{
						std::pair<float,float> wnd(m_wxwindu->get_layer()->get_grid()->transform_axes(wu, wv));
						wu = wnd.first;
						wv = wnd.second;
					}
					if (!std::isnan(wu) && !std::isnan(wv)) {
						wu *= (-1e-3f * Point::km_to_nmi * 3600);
						wv *= (-1e-3f * Point::km_to_nmi * 3600);
						windspeed5 = Point::round<int,double>(sqrtf(wu * wu + wv * wv) * 0.2);
						winddirrad = atan2f(wu, wv);
					}
				}
				static const double windradius = 1.5;
				if (windspeed5) {
					cr->save();
					cr->rotate(M_PI - winddirrad);
					// draw twice, first white with bigger line width
					for (unsigned int i(0); i < 2; ++i) {
						cr->set_line_width(i ? 0.10 : 0.30);
						cr->set_source_rgb(i ? 0.0 : 1.0, i ? 0.0 : 1.0, i ? 0.0 : 1.0);
						double y(-windradius);
						cr->move_to(0, 0);
						cr->line_to(0, y);
						cr->stroke();
						int speed(windspeed5);
						if (speed < 10)
							y -= 0.2;
						while (speed >= 10) {
							speed -= 10;
							cr->move_to(0, y);
							cr->rel_line_to(-0.4, 0.2);
							cr->rel_line_to(0.4, 0.2);
							cr->fill();
							y += 0.4;
						}
						while (speed >= 2) {
							speed -= 2;
							y += 0.2;
							cr->move_to(0, y);
							cr->rel_line_to(-0.4, -0.2);
							cr->stroke();
						}
						if (speed) {
							y += 0.2;
							cr->move_to(0, y);
							cr->rel_line_to(-0.2, -0.1);
							cr->stroke();
						}
					}
					cr->restore();
				}
				// draw cloud circle
				static const double cldradius = 0.5;
				cr->begin_new_path();
				cr->arc(0.0, 0.0, cldradius, 0.0, 2.0 * M_PI);
				cr->close_path();
				cr->set_line_width(0.10);
				if (alpha >= 0.95) {
					cr->set_source_rgb(1.0, 1.0, 1.0);
					cr->fill_preserve();
					cr->set_source_rgb(0.0, 0.0, 0.0);
					cr->stroke();
				} else if (alpha >= 0.85) {
					cr->set_source_rgb(1.0, 1.0, 1.0);
					cr->fill_preserve();
					cr->set_source_rgb(0.0, 0.0, 0.0);
					cr->stroke();
					cr->move_to(0, cldradius);
					cr->line_to(0, -cldradius);
					cr->stroke();
				} else if (alpha >= 0.65) {
					cr->set_source_rgb(1.0, 1.0, 1.0);
					cr->fill_preserve();
					cr->set_source_rgb(0.0, 0.0, 0.0);
					cr->stroke();
					cr->begin_new_path();
					cr->arc(0.0, 0.0, cldradius, 1.5 * M_PI, 2.0 * M_PI);
					cr->line_to(0, 0);
					cr->close_path();
					cr->fill_preserve();
					cr->stroke();
				} else if (alpha >= 0.55) {
					cr->set_source_rgb(1.0, 1.0, 1.0);
					cr->fill_preserve();
					cr->set_source_rgb(0.0, 0.0, 0.0);
					cr->stroke();
					cr->begin_new_path();
					cr->arc(0.0, 0.0, cldradius, 1.5 * M_PI, 2.0 * M_PI);
					cr->line_to(0, 0);
					cr->close_path();
					cr->fill_preserve();
					cr->stroke();
					cr->move_to(0, cldradius);
					cr->line_to(0, -cldradius);
					cr->stroke();
				} else if (alpha >= 0.45) {
					cr->set_source_rgb(1.0, 1.0, 1.0);
					cr->fill_preserve();
					cr->set_source_rgb(0.0, 0.0, 0.0);
					cr->stroke();
					cr->begin_new_path();
					cr->arc(0.0, 0.0, cldradius, 1.5 * M_PI, 0.5 * M_PI);
					cr->close_path();
					cr->fill_preserve();
					cr->stroke();
				} else if (alpha >= 0.35) {
					cr->set_source_rgb(1.0, 1.0, 1.0);
					cr->fill_preserve();
					cr->set_source_rgb(0.0, 0.0, 0.0);
					cr->stroke();
					cr->begin_new_path();
					cr->arc(0.0, 0.0, cldradius, 1.5 * M_PI, 0.5 * M_PI);
					cr->close_path();
					cr->fill_preserve();
					cr->stroke();
					cr->move_to(-cldradius, 0);
					cr->line_to(cldradius, 0);
					cr->stroke();
				} else if (alpha >= 0.15) {
					cr->set_source_rgb(1.0, 1.0, 1.0);
					cr->fill_preserve();
					cr->set_source_rgb(0.0, 0.0, 0.0);
					cr->stroke();
					cr->begin_new_path();
					cr->arc(0.0, 0.0, cldradius, 1.5 * M_PI, 1.0 * M_PI);
					cr->line_to(0, 0);
					cr->close_path();
					cr->fill_preserve();
					cr->stroke();
				} else if (alpha >= 0.05) {
					cr->set_source_rgb(0.0, 0.0, 0.0);
					cr->fill_preserve();
					cr->stroke();
					cr->set_source_rgb(1.0, 1.0, 1.0);
					cr->move_to(0, -0.9 * cldradius);
					cr->line_to(0, 0.9 * cldradius);
					cr->stroke();
				} else {
					cr->set_source_rgb(0.0, 0.0, 0.0);
					cr->fill_preserve();
					cr->stroke();
				}
				cr->restore();
				// show temperature
				static const double textdist = 15;
				if (m_wxtemperature) {
					float temp(m_wxtemperature->operator()(pt, efftime, m_gndchart ? 0 : pressure));
					if (!std::isnan(temp)) {
						std::ostringstream oss;
						oss << Point::round<int,double>(temp - IcaoAtmosphere<double>::degc_to_kelvin) << "\302\260";
						Cairo::TextExtents ext;
						cr->get_text_extents(oss.str(), ext);
						double xc1(xc + textdist - ext.x_bearing);
						double yc1(yc - 1.5 * ext.height - 4 - ext.y_bearing);
						cr->set_source_rgb(1.0, 1.0, 1.0);
						cr->move_to(xc1, yc1 + 1);
						cr->show_text(oss.str());
						cr->move_to(xc1, yc1 - 1);
						cr->show_text(oss.str());
						cr->move_to(xc1 + 1, yc1);
						cr->show_text(oss.str());
						cr->move_to(xc1 - 1, yc1);
						cr->show_text(oss.str());
						set_color_temp(cr);
						cr->move_to(xc1, yc1);
						cr->show_text(oss.str());
					}
				}
				if (m_wxprmsl) {
					float pr(m_wxprmsl->operator()(pt, efftime, m_gndchart ? 0 : pressure));
					if (!std::isnan(pr)) {
						std::ostringstream oss;
						if (m_gndchart)
							oss << Point::round<int,double>(pr * 0.01);
						else
							oss << Point::round<int,double>(pr * Point::m_to_ft_dbl) << '\'';
						Cairo::TextExtents ext;
						cr->get_text_extents(oss.str(), ext);
						double xc1(xc + textdist - ext.x_bearing);
						double yc1(yc - 0.5 * ext.height - ext.y_bearing);
						cr->set_source_rgb(1.0, 1.0, 1.0);
						cr->move_to(xc1, yc1 + 1);
						cr->show_text(oss.str());
						cr->move_to(xc1, yc1 - 1);
						cr->show_text(oss.str());
						cr->move_to(xc1 + 1, yc1);
						cr->show_text(oss.str());
						cr->move_to(xc1 - 1, yc1);
						cr->show_text(oss.str());
						set_color_qnh(cr);
						cr->move_to(xc1, yc1);
						cr->show_text(oss.str());
					}
				}
				if (!ossflg.str().empty()) {
					Cairo::TextExtents ext;
					cr->get_text_extents(ossflg.str(), ext);
					double xc1(xc + textdist - ext.x_bearing);
					double yc1(yc + 0.5 * ext.height + 4 - ext.y_bearing);
					cr->set_source_rgb(1.0, 1.0, 1.0);
					cr->move_to(xc1, yc1 + 1);
					cr->show_text(ossflg.str());
					cr->move_to(xc1, yc1 - 1);
					cr->show_text(ossflg.str());
					cr->move_to(xc1 + 1, yc1);
					cr->show_text(ossflg.str());
					cr->move_to(xc1 - 1, yc1);
					cr->show_text(ossflg.str());
					set_color_wxnote(cr);
					cr->move_to(xc1, yc1);
					cr->show_text(ossflg.str());
				}
			}
		}
	}
	// QNH / altitude contours
	if (m_wxprmsl) {
		std::pair<float,float> mm(AreaExtract::minmax(m_wxprmsl, efftime, m_gndchart ? 0 : pressure));
		if (mm.first < mm.second) {
			double pinc(m_gndchart ? 100 : 25 * Point::ft_to_m_dbl);
			double p0(std::floor(mm.first / pinc) * pinc);
			double p1(std::ceil(mm.second / pinc) * pinc);
			for (; p0 <= p1; p0 += pinc) {
				AreaExtract area(m_wxprmsl, p0, efftime, m_gndchart ? 0 : pressure);
				cr->begin_new_path();
				area.extract_contours(cr, center, scalelon, scalelat);
				set_color_qnh(cr);
				cr->set_line_width(1);
				cr->stroke();
			}
		}
	}
	// wind contours
	if (m_wxwindu && m_wxwindv) {
		std::pair<float,float> mm(AreaExtract::minmax(m_wxwindu, m_wxwindv, efftime, m_gndchart ? 100 : pressure));
		if (mm.first < mm.second) {
			double pinc(1e3f / Point::km_to_nmi / 3600.0 * 25);
			double p0(std::max(1., std::floor(mm.first / pinc)) * pinc);
			double p1(std::ceil(mm.second / pinc) * pinc);
			for (; p0 <= p1; p0 += pinc) {
				AreaExtract area(m_wxwindu, m_wxwindv, p0, efftime, m_gndchart ? 0 : pressure);
				cr->begin_new_path();
				area.extract_contours(cr, center, scalelon, scalelat);
				set_color_isotach(cr);
				cr->set_line_width(1);
				std::vector<double> dashes;
				dashes.push_back(10);
				dashes.push_back(2);
				for (unsigned int x(1), n(Point::round<int,double>(p0 / pinc)); x < n; ++x) {
					dashes.push_back(2);
					dashes.push_back(2);
				}
				cr->set_dash(dashes, 0);
				cr->stroke();
			}
		}
	}
	cr->unset_dash();
	if (m_gndchart) {
		// boundary clouds
		if (m_wxcldbdrycover) {
			AreaExtract area(m_wxcldbdrycover, 0.4, efftime, 0);
			cr->begin_new_path();
			area.extract_contours(cr, center, scalelon, scalelat);
			cr->set_source_rgba(0.5, 0.5, 0.5, 0.2);
			cr->fill();
		}
	} else {
		// low clouds
		if (m_wxcldlowcover && m_wxcldlowbase && m_wxcldlowtop) {
			AreaExtract area(m_wxcldlowcover, m_wxcldlowbase, m_wxcldlowtop, efftime, pressure);
			cr->begin_new_path();
			area.extract_contours(cr, center, scalelon, scalelat);
			cr->set_source_rgba(0.5, 0.5, 0.5, 0.2);
			cr->fill();
		}
		// mid clouds
		if (m_wxcldmidcover && m_wxcldmidbase && m_wxcldmidtop) {
			AreaExtract area(m_wxcldmidcover, m_wxcldmidbase, m_wxcldmidtop, efftime, pressure);
			cr->begin_new_path();
			area.extract_contours(cr, center, scalelon, scalelat);
			cr->set_source_rgba(0.5, 0.5, 0.5, 0.2);
			cr->fill();
		}
		// high clouds
		if (m_wxcldhighcover && m_wxcldhighbase && m_wxcldhightop) {
			AreaExtract area(m_wxcldhighcover, m_wxcldhighbase, m_wxcldhightop, efftime, pressure);
			cr->begin_new_path();
			area.extract_contours(cr, center, scalelon, scalelat);
			cr->set_source_rgba(0.5, 0.5, 0.5, 0.2);
			cr->fill();
		}
		// convective clouds
		if (m_wxcldconvcover && m_wxcldconvbase && m_wxcldconvtop) {
			AreaExtract area(m_wxcldconvcover, m_wxcldconvbase, m_wxcldconvtop, efftime, pressure);
			cr->begin_new_path();
			area.extract_contours(cr, center, scalelon, scalelat);
			cr->set_source_rgba(0.5, 0.5, 0.5, 0.2);
			cr->fill();
		}
	}
	// draw route
	{
		set_color_fplan(cr);
		cr->set_line_width(2);
		cr->begin_new_path();
		for (unsigned int i = 0, j = m_route.get_nrwpt(); i < j; ++i) {
			const FPlanWaypoint& wpt(m_route[i]);
			if (wpt.get_coord().is_invalid())
				continue;
			double xc, yc;
			{
				Point pdiff(wpt.get_coord() - center);
				xc = pdiff.get_lon() * scalelon;
				yc = pdiff.get_lat() * scalelat;
			}
			if (i)
				cr->line_to(xc, yc);
			else
				cr->move_to(xc, yc);
		}
		cr->stroke();
		{
			std::vector<double> dashes;
			dashes.push_back(2);
			dashes.push_back(2);
			cr->set_dash(dashes, 0);
		}
		for (alternates_t::const_iterator ai(m_altn.begin()), ae(m_altn.end()); ai != ae; ++ai) {
			if (ai->get_coord().is_invalid())
				continue;
			const FPlanWaypoint& wpte(m_route[m_route.get_nrwpt()-1]);
			if (wpte.get_coord().is_invalid())
				continue;
			const FPlanRoute *falt(0);
			for (altroutes_t::const_iterator di(m_altnroutes.begin()), de(m_altnroutes.end()); di != de; ++di) {
				if (di->get_nrwpt() <= 0 || (*di)[di->get_nrwpt()-1].get_coord().is_invalid() ||
				    (*di)[di->get_nrwpt()-1].get_coord().simple_distance_nmi(ai->get_coord()) >= 1)
					continue;
				falt = &*di;
				break;
			}
			if (falt) {
				for (unsigned int i = 0, j = falt->get_nrwpt(); i < j; ++i) {
					const FPlanWaypoint& wpt((*falt)[i]);
					if (wpt.get_coord().is_invalid())
						continue;
					double xc, yc;
					{
						Point pdiff(wpt.get_coord() - center);
						xc = pdiff.get_lon() * scalelon;
						yc = pdiff.get_lat() * scalelat;
					}
					if (i)
						cr->line_to(xc, yc);
					else
						cr->move_to(xc, yc);
				}
			} else {
				double xc, yc;
				{
					Point pdiff(wpte.get_coord() - center);
					xc = pdiff.get_lon() * scalelon;
					yc = pdiff.get_lat() * scalelat;
				}
				cr->move_to(xc, yc);
				{
					Point pdiff(ai->get_coord() - center);
					xc = pdiff.get_lon() * scalelon;
					yc = pdiff.get_lat() * scalelat;
				}
				cr->line_to(xc, yc);
			}
			cr->stroke();
		}
		cr->unset_dash();
		for (unsigned int i = 0, j = m_route.get_nrwpt(); i < j; ++i) {
			const FPlanWaypoint& wpt(m_route[i]);
			Glib::ustring txt(wpt.get_icao());
			if (txt.empty() || (wpt.get_type() == FPlanWaypoint::type_airport && AirportsDb::Airport::is_fpl_zzzz(txt)))
				txt = wpt.get_name();
			if (txt.empty())
				continue;
			if (wpt.get_type() >= FPlanWaypoint::type_generated_start && wpt.get_type() <= FPlanWaypoint::type_generated_end)
				continue;
			double xc, yc;
			{
				Point pdiff(wpt.get_coord() - center);
				xc = pdiff.get_lon() * scalelon;
				yc = pdiff.get_lat() * scalelat;
			}
			unsigned int quadrant(0);
			quadrant |= 1 << ((wpt.get_truetrack() >> 14) & 3);
			if (i)
				quadrant |= 1 << (((m_route[i-1].get_truetrack() + 0x8000) >> 14) & 3);
			Cairo::TextExtents ext;
			cr->get_text_extents(txt, ext);
			xc -= ext.x_bearing;
			yc -= ext.y_bearing;
			// place label
			if (!(quadrant & 0x3)) {
				xc += 3;
				yc -= ext.height * 0.5;
			} else if (!(quadrant & 0x6)) {
				xc -= ext.width * 0.5;
				yc += 3;
			} else if (!(quadrant & 0xC)) {
				xc -= 3 + ext.width;
				yc -= ext.height * 0.5;
			} else if (!(quadrant & 0x9)) {
				xc -= ext.width * 0.5;
				yc -= 3 + ext.height;
			} else if (!(quadrant & 0x1)) {
				xc += 3;
				yc -= 3 + ext.height;
			} else if (!(quadrant & 0x2)) {
				xc += 3;
				yc += 3;
			} else if (!(quadrant & 0x4)) {
				xc -= 3 + ext.width;
				yc += 3;
			} else if (!(quadrant & 0x8)) {
				xc -= 3 + ext.width;
				yc -= 3 + ext.height;
			} else {
				xc -= ext.width * 0.5;
				yc -= ext.height * 0.5;
			}
			cr->set_source_rgb(1.0, 1.0, 1.0);
			cr->move_to(xc, yc + 1);
			cr->show_text(txt);
			cr->move_to(xc, yc - 1);
			cr->show_text(txt);
			cr->move_to(xc + 1, yc);
			cr->show_text(txt);
			cr->move_to(xc - 1, yc);
			cr->show_text(txt);
			set_color_fplan(cr);
			cr->move_to(xc, yc);
			cr->show_text(txt);
		}
		for (alternates_t::const_iterator ai(m_altn.begin()), ae(m_altn.end()); ai != ae; ++ai) {
			if (ai->get_coord().is_invalid())
				continue;
			const FPlanWaypoint& wpt(m_route[m_route.get_nrwpt()-1]);
			Glib::ustring txt(ai->get_icao());
			if (txt.empty() || AirportsDb::Airport::is_fpl_zzzz(txt))
				txt = ai->get_name();
			if (txt.empty())
				continue;
			double xc, yc;
			{
				Point pdiff(ai->get_coord() - center);
				xc = pdiff.get_lon() * scalelon;
				yc = pdiff.get_lat() * scalelat;
			}
			unsigned int quadrant(wpt.get_coord().spheric_true_course_dbl(ai->get_coord()) * FPlanWaypoint::from_deg);
			quadrant = 1 << ((quadrant >> 14) & 3);
			Cairo::TextExtents ext;
			cr->get_text_extents(txt, ext);
			xc -= ext.x_bearing;
			yc -= ext.y_bearing;
			// place label
			if (!(quadrant & 0x3)) {
				xc += 3;
				yc -= ext.height * 0.5;
			} else if (!(quadrant & 0x6)) {
				xc -= ext.width * 0.5;
				yc += 3;
			} else if (!(quadrant & 0xC)) {
				xc -= 3 + ext.width;
				yc -= ext.height * 0.5;
			} else if (!(quadrant & 0x9)) {
				xc -= ext.width * 0.5;
				yc -= 3 + ext.height;
			} else if (!(quadrant & 0x1)) {
				xc += 3;
				yc -= 3 + ext.height;
			} else if (!(quadrant & 0x2)) {
				xc += 3;
				yc += 3;
			} else if (!(quadrant & 0x4)) {
				xc -= 3 + ext.width;
				yc += 3;
			} else if (!(quadrant & 0x8)) {
				xc -= 3 + ext.width;
				yc -= 3 + ext.height;
			} else {
				xc -= ext.width * 0.5;
				yc -= ext.height * 0.5;
			}
			cr->set_source_rgb(1.0, 1.0, 1.0);
			cr->move_to(xc, yc + 1);
			cr->show_text(txt);
			cr->move_to(xc, yc - 1);
			cr->show_text(txt);
			cr->move_to(xc + 1, yc);
			cr->show_text(txt);
			cr->move_to(xc - 1, yc);
			cr->show_text(txt);
			set_color_fplan(cr);
			cr->move_to(xc, yc);
			cr->show_text(txt);
		}
	}
	// end
	cr->restore();
	cr->save();
	cr->rectangle(0, 0, width, height);
	cr->set_source_rgb(0.0, 0.0, 0.0);
	cr->set_line_width(3);
	cr->stroke();
	cr->restore();
}

const std::string& to_str(Cairo::SurfaceType st)
{
	switch (st) {
	case Cairo::SURFACE_TYPE_IMAGE:
	{
		static const std::string r("IMAGE");
		return r;
	}

	case Cairo::SURFACE_TYPE_PDF:
	{
		static const std::string r("PDF");
		return r;
	}

	case Cairo::SURFACE_TYPE_PS:
	{
		static const std::string r("PS");
		return r;
	}

	case Cairo::SURFACE_TYPE_XLIB:
	{
		static const std::string r("XLIB");
		return r;
	}

	case Cairo::SURFACE_TYPE_XCB:
	{
		static const std::string r("XCB");
		return r;
	}

	case Cairo::SURFACE_TYPE_GLITZ:
	{
		static const std::string r("GLITZ");
		return r;
	}

	case Cairo::SURFACE_TYPE_QUARTZ:
	{
		static const std::string r("QUARTZ");
		return r;
	}

	case Cairo::SURFACE_TYPE_WIN32:
	{
		static const std::string r("WIN32");
		return r;
	}

	case Cairo::SURFACE_TYPE_BEOS:
	{
		static const std::string r("BEOS");
		return r;
	}

	case Cairo::SURFACE_TYPE_DIRECTFB:
	{
		static const std::string r("DIRECTFB");
		return r;
	}

	case Cairo::SURFACE_TYPE_SVG:
	{
		static const std::string r("SVG");
		return r;
	}

	case Cairo::SURFACE_TYPE_OS2:
	{
		static const std::string r("OS2");
		return r;
	}

	case Cairo::SURFACE_TYPE_WIN32_PRINTING:
	{
		static const std::string r("PRINTING");
		return r;
	}

	case Cairo::SURFACE_TYPE_QUARTZ_IMAGE:
	{
		static const std::string r("IMAGE");
		return r;
	}

#if (CAIRO_VERSION_MAJOR > 1) || (CAIRO_VERSION_MAJOR == 1 && CAIRO_VERSION_MINOR >= 10)
	case Cairo::SURFACE_TYPE_SCRIPT:
	{
		static const std::string r("SCRIPT");
		return r;
	}

	case Cairo::SURFACE_TYPE_QT:
	{
		static const std::string r("QT");
		return r;
	}

	case Cairo::SURFACE_TYPE_RECORDING:
	{
		static const std::string r("RECORDING");
		return r;
	}

	case Cairo::SURFACE_TYPE_VG:
	{
		static const std::string r("VG");
		return r;
	}

	case Cairo::SURFACE_TYPE_GL:
	{
		static const std::string r("GL");
		return r;
	}

	case Cairo::SURFACE_TYPE_DRM:
	{
		static const std::string r("DRM");
		return r;
	}

	case Cairo::SURFACE_TYPE_TEE:
	{
		static const std::string r("TEE");
		return r;
	}

	case Cairo::SURFACE_TYPE_XML:
	{
		static const std::string r("XML");
		return r;
	}

	case Cairo::SURFACE_TYPE_SKIA:
	{
		static const std::string r("SKIA");
		return r;
	}

	case Cairo::SURFACE_TYPE_SUBSURFACE:
	{
		static const std::string r("SUBSURFACE");
		return r;
	}
#endif

	default:
	{
		static const std::string r("?");
		return r;
	}
	}
}

const std::string& to_str(Cairo::Format f)
{
	switch (f) {
	case Cairo::FORMAT_ARGB32:
	{
		static const std::string r("ARGB32");
		return r;
	}

	case Cairo::FORMAT_RGB24:
	{
		static const std::string r("RGB32");
		return r;
	}

	case Cairo::FORMAT_A8:
	{
		static const std::string r("A8");
		return r;
	}

	case Cairo::FORMAT_A1:
	{
		static const std::string r("A1");
		return r;
	}

	case Cairo::FORMAT_RGB16_565:
	{
		static const std::string r("RGB16_565");
		return r;
	}

	default:
	{
		static const std::string r("Unknown Format");
		return r;
	}
	}
}

METARTAFChart::OrderFlightPath::OrderFlightPath(const Point& p0, const Point& p1)
	: m_pt0(p0), m_pt1(p1)
{
	m_lonscale2 = cos(m_pt0.halfway(m_pt1).get_lat_rad_dbl());
	m_lonscale2 *= m_lonscale2;
}

bool METARTAFChart::OrderFlightPath::operator()(const MetarTafSet::Station& a, const MetarTafSet::Station& b) const
{
	Point p(m_pt1 - m_pt0);
	Point pa(a.get_coord() - m_pt0);
	Point pb(b.get_coord() - m_pt0);
	double ma(p.get_lon() * (double)pa.get_lon() * m_lonscale2 + p.get_lat() * (double)pa.get_lat());
	double mb(p.get_lon() * (double)pb.get_lon() * m_lonscale2 + p.get_lat() * (double)pb.get_lat());
	return ma < mb;
}

void METARTAFChart::DbLoaderSqlite::load(MetarTafSet& mtset, const Rect& bbox, time_t tmin, time_t tmax,
					 unsigned int metarhistory, unsigned int tafhistory)
{
	mtset.loadstn_sqlite(m_db, bbox, metarhistory, tafhistory);
}

#ifdef HAVE_PQXX

void METARTAFChart::DbLoaderPG::load(MetarTafSet& mtset, const Rect& bbox, time_t tmin, time_t tmax,
				     unsigned int metarhistory, unsigned int tafhistory)
{
	mtset.loadstn_pg(m_conn, bbox, tmin, tmax, metarhistory, tafhistory);
}

#endif

METARTAFChart::Label::Label(const std::string& txt, const std::string& code, double x, double y,
			    placement_t placement, bool fixed, bool keep)
	: m_txt(txt), m_code(code), m_ax(x), m_ay(y), m_nx(x), m_ny(y), m_x(x), m_y(y),
	  m_placement(placement), m_fixed(fixed), m_keep(keep)
{
	switch (m_placement) {
	case placement_west:
		m_x += m_txt.size() * tex_char_width + tex_anchor_x;
		break;

	case placement_north:
		m_y -= 0.5 * tex_char_height + tex_anchor_y;
		break;

	case placement_east:
		m_x -= m_txt.size() * tex_char_width + tex_anchor_x;
		break;

	case placement_south:
		m_y += 0.5 * tex_char_height + tex_anchor_y;
		break;

	case placement_southwest:
		m_x += m_txt.size() * tex_char_width + tex_anchor_x;
		m_y += 0.5 * tex_char_height + tex_anchor_y;
		break;

	case placement_northwest:
		m_x += m_txt.size() * tex_char_width + tex_anchor_x;
		m_y -= 0.5 * tex_char_height + tex_anchor_y;
		break;

	case placement_northeast:
		m_x -= m_txt.size() * tex_char_width + tex_anchor_x;
		m_y -= 0.5 * tex_char_height + tex_anchor_y;
		break;

	case placement_southeast:
		m_x -= m_txt.size() * tex_char_width + tex_anchor_x;
		m_y += 0.5 * tex_char_height + tex_anchor_y;
		break;

	case placement_center:
	default:
		break;
	}
	m_nx = m_x;
	m_ny = m_y;
}

double METARTAFChart::Label::attractive_force(void) const
{
	if (is_fixed())
		return 0;
	double x(get_diffx()), y(get_diffy());
	return x * x + y * y;
}

double METARTAFChart::Label::repulsive_force(const Label& lbl, double maxx, double maxy) const
{
	if (maxx <= 0 || maxy <= 0)
		return 0;
	double dx(fabs(lbl.get_x() - get_x())), dy(fabs(lbl.get_y() - get_y()));
	if (dx >= maxx || dy >= maxy)
		return 0;
	dx /= maxx;
	dy /= maxy;
	double r(sqrt(dx * dx + dy * dy));
	r = std::max(r, 1e-30);
	return 1 / r;
}

void METARTAFChart::Label::move(double dx, double dy, double minx, double maxx, double miny, double maxy)
{
	m_x = std::max(std::min(m_x + dx, maxx), minx);
	m_y = std::max(std::min(m_y + dy, maxy), miny);
}

const std::string& METARTAFChart::Label::get_tikz_anchor(placement_t p)
{
	switch (p) {
	default:
	case placement_center:
	{
		static const std::string r("anchor=center");
		return r;
	}

	case placement_north:
	{
		static const std::string r("anchor=north");
		return r;
	}

	case placement_northeast:
	{
		static const std::string r("anchor=north east");
		return r;
	}

	case placement_east:
	{
		static const std::string r("anchor=east");
		return r;
	}

	case placement_southeast:
	{
		static const std::string r("anchor=south east");
		return r;
	}

	case placement_south:
	{
		static const std::string r("anchor=south");
		return r;
	}

	case placement_southwest:
	{
		static const std::string r("anchor=south west");
		return r;
	}

	case placement_west:
	{
		static const std::string r("anchor=west");
		return r;
	}

	case placement_northwest:
	{
		static const std::string r("anchor=north west");
		return r;
	}
	}
}

METARTAFChart::Label::placement_t METARTAFChart::Label::placement_from_quadrant(unsigned int quadrant)
{
	if (!(quadrant & 0x3))
		return placement_west;
	if (!(quadrant & 0x6))
		return placement_north;
	if (!(quadrant & 0xC))
		return placement_east;
	if (!(quadrant & 0x9))
		return placement_south;
	if (!(quadrant & 0x1))
		return placement_southwest;
	if (!(quadrant & 0x2))
		return placement_northwest;
	if (!(quadrant & 0x4))
		return placement_northeast;
	if (!(quadrant & 0x8))
		return placement_southeast;
	return placement_center;
}

METARTAFChart::LabelLayout::LabelLayout(double width, double height, double maxx, double maxy)
	: m_width(width), m_height(height), m_maxx(maxx), m_maxy(maxy), m_ka(1), m_kr(1)
{
}

void METARTAFChart::LabelLayout::setk(double kr)
{
	m_kr = m_width * m_height / std::max(size(), (size_type)1);
	m_ka = 1. / sqrt(m_kr);
	m_kr *= kr;
}

void METARTAFChart::LabelLayout::iterate(double temp)
{
	for (size_type i(0), n(size()); i < n; ++i) {
		Label& lbl(operator[](i));
		if (lbl.is_fixed())
			continue;
		double dx(0), dy(0);
		{
			double f(lbl.attractive_force());
			f *= m_ka * temp;
			dx = (lbl.get_nominalx() - lbl.get_x()) * f;
			dy = (lbl.get_nominaly() - lbl.get_y()) * f;
		}
		for (size_type j(0); j < n; ++j) {
			if (i == j)
				continue;
			const Label& lbl2(operator[](j));
			double f(lbl.repulsive_force(lbl2, m_maxx, m_maxy));
			f *= m_kr * temp;
			dx += (lbl.get_x() - lbl2.get_x()) * f;
			dy += (lbl.get_y() - lbl2.get_y()) * f;
		}
		lbl.move(dx, dy, 0, m_width, 0, m_height);
	}
}

double METARTAFChart::LabelLayout::total_repulsive_force(void) const
{
	double r(0);
	for (size_type i(0), n(size()); i < n; ++i)
		for (size_type j(i + 1); j < n; ++j)
			if (!(operator[](i).is_fixed() && operator[](j).is_fixed()))
			    r += operator[](i).repulsive_force(operator[](j), m_maxx, m_maxy);
	return m_kr * r;
}

double METARTAFChart::LabelLayout::total_attractive_force(void) const
{
	double r(0);
	for (size_type i(0), n(size()); i < n; ++i)
		r += operator[](i).attractive_force();
	return m_ka * r;
}

double METARTAFChart::LabelLayout::repulsive_force(size_type i) const
{
	double r(0);
	for (size_type j(0), n(size()); j < n; ++j)
		if (j != i)
			r += operator[](i).repulsive_force(operator[](j), m_maxx, m_maxy);
	return m_kr * r;
}

void METARTAFChart::LabelLayout::remove_overlapping_labels(void)
{
	for (unsigned int len = 5; len >= 2; --len) {
		for (size_type i(0), n(size()); i < n;) {
			if (operator[](i).is_keep()) {
				++i;
				continue;
			}
			if ((len == 5 && operator[](i).get_text().size() < 5) ||
			    (len == 2 && operator[](i).get_text().size() > 2) ||
			    (len > 2 && len < 5 && operator[](i).get_text().size() != len)) {
				++i;
				continue;
			}
			if (repulsive_force(i) <= 0) {
				++i;
				continue;
			}
			erase(begin() + i);
			n = size();
		}
	}
}

constexpr double METARTAFChart::max_dist_from_route;
constexpr double METARTAFChart::max_raob_dist_from_route;
constexpr double METARTAFChart::chart_strip_length;
const unsigned int METARTAFChart::metar_history_size;
const unsigned int METARTAFChart::metar_depdestalt_history_size;
const unsigned int METARTAFChart::taf_history_size;
constexpr double METARTAFChart::tex_char_width;
constexpr double METARTAFChart::tex_char_height;
constexpr double METARTAFChart::tex_anchor_x;
constexpr double METARTAFChart::tex_anchor_y;

METARTAFChart::METARTAFChart(const FPlanRoute& route, const alternates_t& altn, const altroutes_t& altnfpl,
			      const RadioSoundings& raobs, const std::string& tempdir)
	: m_raobs(raobs), m_route(route), m_altn(altn), m_altnroutes(altnfpl), m_tempdir(tempdir)
{
}

void METARTAFChart::set_db_sqlite(const std::string& db)
{
	m_dbloader = dbloader_t(new DbLoaderSqlite(db));
}

#ifdef HAVE_PQXX

void METARTAFChart::set_db_pg(pqxx::connection_base& conn)
{
	m_dbloader = dbloader_t(new DbLoaderPG(conn));
}

#endif

void METARTAFChart::add_fir(const std::string& id, const MultiPolygonHole& poly)
{
	m_set.add_fir(id, poly);
}

void METARTAFChart::polypath(const Cairo::RefPtr<Cairo::Context>& cr, const PolygonSimple& poly,
			     const Rect& bbox, const Point& center, double scalelon, double scalelat)
{
	if (poly.size() < 2 || !poly.get_bbox().is_intersect(bbox))
		return;
	bool first(true);
	cr->begin_new_sub_path();
	for (PolygonSimple::const_iterator pi(poly.begin()), pe(poly.end()); pi != pe; ++pi) {
		double xc, yc;
		{
			Point pdiff(*pi - center);
			xc = pdiff.get_lon() * scalelon;
			yc = pdiff.get_lat() * scalelat;
		}
		if (first) {
			cr->move_to(xc, yc);
			first = false;
		} else {
			cr->line_to(xc, yc);
		}
	}
	cr->close_path();
}

void METARTAFChart::polypath(const Cairo::RefPtr<Cairo::Context>& cr, const PolygonHole& poly,
			     const Rect& bbox, const Point& center, double scalelon, double scalelat)
{
	polypath(cr, poly.get_exterior(), bbox, center, scalelon, scalelat);
	for (unsigned int i(0), n(poly.get_nrinterior()); i < n; ++i)
		polypath(cr, poly[i], bbox, center, scalelon, scalelat);
}

void METARTAFChart::polypath(const Cairo::RefPtr<Cairo::Context>& cr, const MultiPolygonHole& poly,
			     const Rect& bbox, const Point& center, double scalelon, double scalelat)
{
	for (MultiPolygonHole::const_iterator phi(poly.begin()), phe(poly.end()); phi != phe; ++phi)
		polypath(cr, *phi, bbox, center, scalelon, scalelat);
}

void METARTAFChart::polypath(const Cairo::RefPtr<Cairo::Context>& cr, const LineString& line,
			     const Rect& bbox, const Point& center, double scalelon, double scalelat)
{
	if (line.size() < 2 || !line.get_bbox().is_intersect(bbox))
		return;
	bool first(true);
	cr->begin_new_sub_path();
	for (LineString::const_iterator pi(line.begin()), pe(line.end()); pi != pe; ++pi) {
		double xc, yc;
		{
			Point pdiff(*pi - center);
			xc = pdiff.get_lon() * scalelon;
			yc = pdiff.get_lat() * scalelat;
		}
		if (first) {
			cr->move_to(xc, yc);
			first = false;
		} else {
			cr->line_to(xc, yc);
		}
	}
}

void METARTAFChart::polypath(const Cairo::RefPtr<Cairo::Context>& cr, const MultiLineString& line,
			     const Rect& bbox, const Point& center, double scalelon, double scalelat)
{
	for (MultiLineString::const_iterator li(line.begin()), le(line.end()); li != le; ++li)
		polypath(cr, *li, bbox, center, scalelon, scalelat);
}

std::string METARTAFChart::pgfcoord(double x, double y)
{
	std::ostringstream oss;
	oss << '(' << std::fixed << x << ',' << std::fixed << y << ')';
	return oss.str();
}

std::string METARTAFChart::georefcoord(double x, double y)
{
	std::ostringstream oss;
	oss << '{' << std::fixed << x << "}{" << std::fixed << y << '}';
	return oss.str();
}

std::string METARTAFChart::georefcoord(const Point& pt, double x, double y)
{
	std::ostringstream oss;
	oss << '{' << std::fixed << pt.get_lat_deg_dbl()
	    << "}{" << std::fixed << pt.get_lon_deg_dbl()
	    << "}{" << std::fixed << x << "}{" << std::fixed << y << '}';
	return oss.str();
}

std::string METARTAFChart::georefcoord(const Point& pt, const Point& center, double width, double height, double scalelon, double scalelat)
{
	double xc, yc;
	{
		Point pdiff(pt - center);
		xc = pdiff.get_lon() * scalelon + 0.5 * width;
		yc = pdiff.get_lat() * scalelat + 0.5 * height;
	}
	return georefcoord(pt, xc, yc);
}

void METARTAFChart::polypath(std::ostream& os, const std::string& attr, const PolygonSimple& poly,
			     const Rect& bbox, const Point& center, double width, double height, double scalelon, double scalelat)
{
	if (poly.size() < 2)
		return;
	PolygonSimple::size_type i, n(poly.size()), j(n);
	std::vector<bool> inside(n, false);
	bool allinside(true), anyinside(false);;
	for (i = 0; i < n; ++i) {
		bool x(bbox.is_inside(poly[i]));
		inside[i] = x;
		allinside = x && allinside;
		anyinside = x || anyinside;
		if (!x && j >= n)
			j = i;
	}
	if (!anyinside)
		return;
	if (allinside) {
		os << "    \\path";
		if (!attr.empty())
			os << '[' << attr << ']';
		for (i = 0; i < n; ++i) {
			if (!(i & 15) && i)
				os << std::endl << "      ";
			double xc, yc;
			{
				Point pdiff(poly[i] - center);
				xc = pdiff.get_lon() * scalelon + 0.5 * width;
				yc = pdiff.get_lat() * scalelat + 0.5 * height;
			}
			if (i)
				os << " -- ";
			else
				os << ' ';
			os << pgfcoord(xc, yc);
		}
		os << " -- cycle;" << std::endl;
		return;
	}
	for (i = j;;) {
		while (!inside[i]) {
			++i;
			if (i >= n)
				i = 0;
			if (i == j)
				break;
		}
		if (i == j)
			break;
		os << "    \\path";
		if (!attr.empty())
			os << '[' << attr << ']';
		unsigned int cnt(0);
		for (;;) {
			if (!cnt) {
				os << ' ';
			} else if (inside[i]) {
				os << " -- ";
			} else {
				os << ';' << std::endl;
				break;
			}
			if (!(cnt & 15) && cnt)
				os << std::endl << "      ";
			double xc, yc;
			{
				Point pdiff(poly[i] - center);
				xc = pdiff.get_lon() * scalelon + 0.5 * width;
				yc = pdiff.get_lat() * scalelat + 0.5 * height;
			}
			os << pgfcoord(xc, yc);
			++cnt;
			++i;
			if (i >= n)
				i = 0;
			if (i == j) {
				os << ';' << std::endl;
				break;
			}
		}
		if (i == j)
			break;
	}
}

void METARTAFChart::polypath(std::ostream& os, const std::string& attr, const PolygonHole& poly,
			     const Rect& bbox, const Point& center, double width, double height, double scalelon, double scalelat)
{
	polypath(os, attr, poly.get_exterior(), bbox, center, width, height, scalelon, scalelat);
	for (unsigned int i(0), n(poly.get_nrinterior()); i < n; ++i)
		polypath(os, attr, poly[i], bbox, center, width, height, scalelon, scalelat);
}

void METARTAFChart::polypath(std::ostream& os, const std::string& attr, const MultiPolygonHole& poly,
			     const Rect& bbox, const Point& center, double width, double height, double scalelon, double scalelat)
{
	for (MultiPolygonHole::const_iterator phi(poly.begin()), phe(poly.end()); phi != phe; ++phi)
		polypath(os, attr, *phi, bbox, center, width, height, scalelon, scalelat);
}

void METARTAFChart::polypath(std::ostream& os, const std::string& attr, const LineString& line,
			     const Rect& bbox, const Point& center, double width, double height, double scalelon, double scalelat)
{
	if (line.size() < 2)
		return;
	PolygonSimple::size_type i, n(line.size()), j(n);
	std::vector<bool> inside(n, false);
	bool allinside(true), anyinside(false);;
	for (i = 0; i < n; ++i) {
		bool x(bbox.is_inside(line[i]));
		inside[i] = x;
		allinside = x && allinside;
		anyinside = x || anyinside;
		if (!x && j >= n)
			j = i;
	}
	if (!anyinside)
		return;
	if (allinside) {
		os << "    \\path";
		if (!attr.empty())
			os << '[' << attr << ']';
		for (i = 0; i < n; ++i) {
			if (!(i & 15) && i)
				os << std::endl << "      ";
			double xc, yc;
			{
				Point pdiff(line[i] - center);
				xc = pdiff.get_lon() * scalelon + 0.5 * width;
				yc = pdiff.get_lat() * scalelat + 0.5 * height;
			}
			if (i)
				os << " -- ";
			else
				os << ' ';
			os << pgfcoord(xc, yc);
		}
		os << ';' << std::endl;
		return;
	}
	for (i = j; i < n;) {
		while (i < n && !inside[i])
			++i;
		if (i >= n)
			break;
		os << "    \\path";
		if (!attr.empty())
			os << '[' << attr << ']';
		unsigned int cnt(0);
		for (;;) {
			if (!cnt) {
				os << ' ';
			} else if (inside[i]) {
				os << " -- ";
			} else {
				os << ';' << std::endl;
				break;
			}
			if (!(cnt & 15) && cnt)
				os << std::endl << "      ";
			double xc, yc;
			{
				Point pdiff(line[i] - center);
				xc = pdiff.get_lon() * scalelon + 0.5 * width;
				yc = pdiff.get_lat() * scalelat + 0.5 * height;
			}
			os << pgfcoord(xc, yc);
			++cnt;
			++i;
			if (i >= n) {
				os << ';' << std::endl;
				break;
			}
		}
	}
}

void METARTAFChart::polypath(std::ostream& os, const std::string& attr, const MultiLineString& line,
			     const Rect& bbox, const Point& center, double width, double height, double scalelon, double scalelat)
{
	for (MultiLineString::const_iterator li(line.begin()), le(line.end()); li != le; ++li)
		polypath(os, attr, *li, bbox, center, width, height, scalelon, scalelat);
}

void METARTAFChart::polypath_fill_helper(std::ostream& os, const PolygonSimple& poly,
					 const Point& center, double width, double height, double scalelon, double scalelat)
{
	if (poly.size() < 2)
		return;
	for (PolygonSimple::size_type i(0), n(poly.size()); i < n; ++i) {
		if (!(i & 15) && i)
			os << std::endl << "      ";
		double xc, yc;
		{
			Point pdiff(poly[i] - center);
			xc = pdiff.get_lon() * scalelon + 0.5 * width;
			yc = pdiff.get_lat() * scalelat + 0.5 * height;
		}
		if (i)
			os << " -- ";
		else
			os << ' ';
		os << pgfcoord(xc, yc);
	}
	os << " -- cycle";
}

void METARTAFChart::polypath_fill(std::ostream& os, const std::string& attr, const PolygonHole& poly,
				  const Point& center, double width, double height, double scalelon, double scalelat)
{
	if (poly.get_exterior().size() < 2)
		return;
	os << "    \\path";
	if (!attr.empty())
		os << '[' << attr << ']';
	polypath_fill_helper(os, poly.get_exterior(), center, width, height, scalelon, scalelat);
	for (unsigned int j(0), m(poly.get_nrinterior()); j < m; ++j)
		polypath_fill_helper(os, poly[j], center, width, height, scalelon, scalelat);
	os << ';' << std::endl;
}

void METARTAFChart::polypath_fill(std::ostream& os, const std::string& attr, const MultiPolygonHole& poly,
				  const Point& center, double width, double height, double scalelon, double scalelat)
{
	for (MultiPolygonHole::const_iterator phi(poly.begin()), phe(poly.end()); phi != phe; ++phi)
		polypath_fill(os, attr, *phi, center, width, height, scalelon, scalelat);
}

Rect METARTAFChart::get_bbox(unsigned int wptidx0, unsigned int wptidx1) const
{
	Rect bbox(get_route().get_bbox(wptidx0, wptidx1));
	if (wptidx1 < get_route().get_nrwpt())
		return bbox;
	for (alternates_t::const_iterator ai(m_altn.begin()), ae(m_altn.end()); ai != ae; ++ai) {
		if (ai->get_coord().is_invalid())
			continue;
		bbox = bbox.add(ai->get_coord());
	}
	for (altroutes_t::const_iterator ai(m_altnroutes.begin()), ae(m_altnroutes.end()); ai != ae; ++ai) {
		Rect bbox2(ai->get_bbox());
		bbox = bbox.add(bbox2);
	}
	return bbox;
}

Rect METARTAFChart::get_bbox(void) const
{
	return get_bbox(0, get_route().get_nrwpt());
}

Rect METARTAFChart::get_extended_bbox(void) const
{
	return get_bbox().oversize_nmi(max_dist_from_route);
}

Rect METARTAFChart::get_extended_bbox(unsigned int wptidx0, unsigned int wptidx1) const
{
	return get_bbox(wptidx0, wptidx1).oversize_nmi(max_dist_from_route);
}

std::pair<time_t,time_t> METARTAFChart::get_timebound(void) const
{
	if (!get_route().get_nrwpt())
		return std::pair<time_t,time_t>(std::numeric_limits<time_t>::max(), std::numeric_limits<time_t>::min());
	std::pair<time_t,time_t> r(get_route()[0].get_time_unix(), get_route()[get_route().get_nrwpt()-1].get_flighttime());
	for (alternates_t::const_iterator ai(m_altn.begin()), ae(m_altn.end()); ai != ae; ++ai) {
		if (ai->get_coord().is_invalid())
			continue;
		r.second = std::max(r.second, (time_t)ai->get_flighttime());
	}
	r.second += r.first;
	return r;
}

void METARTAFChart::loadstn(const Rect& bbox)
{
	loadstn_db(bbox);
	loadstn_raob(bbox);
	{
		std::set<std::string> filter;
		filter.insert(m_route[0].get_icao());
		filter.insert(m_route[m_route.get_nrwpt()-1].get_icao());
		for (alternates_t::const_iterator ai(m_altn.begin()), ae(m_altn.end()); ai != ae; ++ai)
			filter.insert(ai->get_icao());
		for (MetarTafSet::stations_t::iterator si(m_set.get_stations().begin()), se(m_set.get_stations().end()); si != se; ++si) {
			unsigned int nr(si->get_metar().size());
			{
				unsigned int i(filter.find(si->get_stationid()) == filter.end() ? metar_history_size : metar_depdestalt_history_size);
				if (nr <= i)
					continue;
				nr -= i;
			}
			for (; nr > 0; --nr)
				si->get_metar().erase(si->get_metar().begin());
		}
	}
}

void METARTAFChart::loadstn_db(const Rect& bbox)
{
	if (!m_dbloader)
		return;
	std::pair<time_t,time_t> tb(get_timebound());
	if (tb.first <= tb.second) {
		tb.first -= 2 * 60 * 60;
		tb.second += 2 * 60 * 60;
	}
	m_dbloader->load(m_set, bbox, tb.first, tb.second, std::max(metar_history_size, metar_depdestalt_history_size), taf_history_size);
}

void METARTAFChart::loadstn_raob(const Rect& bbox)
{
	static const double max_wmo_dist = 5;
	if (!&m_raobs)
		return;
	for (RadioSoundings::const_iterator ri(m_raobs.begin()), re(m_raobs.end()); ri != re; ) {
		unsigned int wmonr(ri->get_wmonr());
		++ri;
		while (ri != re && ri->get_wmonr() == wmonr)
			++ri;
		if (wmonr == ~0U)
			continue;
		WMOStation::const_iterator stn(WMOStation::find(wmonr));
		while (stn != WMOStation::end() && !stn->get_coord().is_invalid()) {
			MetarTafSet::stations_t::iterator se(m_set.get_stations().end()), si(se);
			double dist(max_wmo_dist);
			for (MetarTafSet::stations_t::iterator si2(m_set.get_stations().begin()); si2 != se; ++si2) {
				double d(si2->get_coord().spheric_distance_nmi_dbl(stn->get_coord()));
				if (si2->get_wmonr() == wmonr) {
					dist = d;
					si = si2;
					break;
				}
				if (d > dist)
					continue;
				dist = d;
				si = si2;
			}
			if (si != se) {
				if (!si->is_wmonr_valid()) {
					si->set_wmonr(wmonr);
					break;
				}
				if (si->get_wmonr() == wmonr)
					break;
				unsigned int wmonr2(si->get_wmonr());
				WMOStation::const_iterator stn2(WMOStation::find(wmonr2));
				if (stn2 == WMOStation::end() || stn2->get_coord().is_invalid()) {
					si->set_wmonr(wmonr);
					break;
				}
				double dist2(si->get_coord().spheric_distance_nmi_dbl(stn2->get_coord()));
				if (dist < dist2) {
					si->set_wmonr(wmonr);
					wmonr = wmonr2;
					stn = stn2;
					continue;
				}
			}
			{
				MetarTafSet::stations_t::iterator si2(se);
				if (stn->get_icao()) {
					for (si2 = m_set.get_stations().begin(); si2 != se; ++si2)
						if (si2->get_stationid() == stn->get_icao())
							break;
				}
				if (si2 != se)
					std::cerr << "raob: stn id " << si2->get_stationid() << " already used, distance "
						  << si2->get_coord().spheric_distance_nmi_dbl(stn->get_coord()) << std::endl;
				std::ostringstream stnid;
				if (stn->get_icao() && si2 == se)
					stnid << stn->get_icao();
				else
					stnid << std::setfill('0') << std::setw(5) << wmonr;
				double elevm(std::numeric_limits<double>::quiet_NaN());
				if (stn->get_elev() != TopoDb30::nodata)
					elevm = stn->get_elev();
				si = m_set.get_stations().insert(se, MetarTafSet::Station(stnid.str(), stn->get_coord(), elevm));
			}
			si->set_wmonr(wmonr);
			break;
		}
	}
}

void METARTAFChart::filterstn(void)
{
	for (MetarTafSet::stations_t::iterator si(m_set.get_stations().begin()), se(m_set.get_stations().end()); si != se; ) {
		const double maxroutedist(si->is_wmonr_valid() ? max_raob_dist_from_route : max_dist_from_route);
		unsigned int i(0), n(m_route.get_nrwpt());
		for (; i < n; ++i) {
			const Point& pt1(m_route[i].get_coord());
			if (si->get_coord().spheric_distance_nmi_dbl(pt1) < maxroutedist) {
				if (false)
					std::cerr << "Station ID " << si->get_stationid() << " close to waypoint " << i << std::endl;
				break;
			}
			if (!i)
				continue;
			const Point& pt0(m_route[i-1].get_coord());
			Point pt(si->get_coord().spheric_nearest_on_line(pt0, pt1));
			if (pt.is_invalid() || !pt.in_box(pt0, pt1))
				continue;
			if (si->get_coord().spheric_distance_nmi_dbl(pt) < maxroutedist) {
				if (false)
					std::cerr << "Station ID " << si->get_stationid() << " close to "
						  << pt.get_lat_str2() << ' ' << pt.get_lon_str2() << " leg " << i << std::endl;
				break;
			}
		}
		if (i == n) {
			for (alternates_t::const_iterator ai(m_altn.begin()), ae(m_altn.end()); ai != ae; ++ai) {
				if (ai->get_coord().is_invalid())
					continue;
				if (si->get_coord().spheric_distance_nmi_dbl(ai->get_coord()) < maxroutedist) {
					i = 0;
					n = 1;
					if (false)
						std::cerr << "Station ID " << si->get_stationid()
							  << " close to alternate " << ai->get_icao() << std::endl;
					break;
				}
				const FPlanRoute *falt(0);
				for (altroutes_t::const_iterator di(m_altnroutes.begin()), de(m_altnroutes.end()); di != de; ++di) {
					if (di->get_nrwpt() <= 0 || (*di)[di->get_nrwpt()-1].get_coord().is_invalid() ||
					    (*di)[di->get_nrwpt()-1].get_coord().simple_distance_nmi(ai->get_coord()) >= 1)
						continue;
					falt = &*di;
					break;
				}
				if (falt) {
					i = 0;
					n = falt->get_nrwpt();
					for (; i < n; ++i) {
						const Point& pt1((*falt)[i].get_coord());
						if (si->get_coord().spheric_distance_nmi_dbl(pt1) < maxroutedist) {
							if (false)
								std::cerr << "Station ID " << si->get_stationid() << " close to altn waypoint " << i << std::endl;
							break;
						}
						if (!i)
							continue;
						const Point& pt0((*falt)[i-1].get_coord());
						Point pt(si->get_coord().spheric_nearest_on_line(pt0, pt1));
						if (pt.is_invalid() || !pt.in_box(pt0, pt1))
							continue;
						if (si->get_coord().spheric_distance_nmi_dbl(pt) < maxroutedist) {
							if (false)
								std::cerr << "Station ID " << si->get_stationid() << " close to "
									  << pt.get_lat_str2() << ' ' << pt.get_lon_str2() << " altn leg " << i << std::endl;
							break;
						}
					}
					if (i < n)
						break;
					continue;
				}
				const Point& pt0(m_route[m_route.get_nrwpt()-1].get_coord());
				Point pt(si->get_coord().spheric_nearest_on_line(pt0, ai->get_coord()));
				if (pt.is_invalid() || !pt.in_box(pt0, ai->get_coord()))
					continue;
				if (si->get_coord().spheric_distance_nmi_dbl(pt) < maxroutedist) {
					i = 0;
					n = 1;
					if (false)
						std::cerr << "Station ID " << si->get_stationid() << " close to alternate "
							  << ai->get_icao() << std::endl;
					break;
				}
			}
		}
		if (i < n) {
			++si;
			continue;
		}
		si = m_set.get_stations().erase(si);
		se = m_set.get_stations().end();
	}
	if (false) {
		OrderFlightPath order(m_route[0].get_coord(), m_route[m_route.get_nrwpt()-1].get_coord());
		std::sort(m_set.get_stations().begin(), m_set.get_stations().end(), order);
	} else {
		std::sort(m_set.get_stations().begin(), m_set.get_stations().end(), MetarTafSet::OrderStadionId());
	}
}

#if 0
Glib::ustring METARTAFChart::latex_string_meta(const Glib::ustring& x)
{
	Glib::ustring r;
	for (Glib::ustring::const_iterator i(x.begin()), e(x.end()); i != e; ++i) {
		switch (*i) {
		case '\\':
			r += "\\textbackslash{}";
			break;

		case '~':
			r += "\\textasciitilde{}";
			break;

		case '^':
			r += "\\textasciicircum{}";
			break;

		case '<':
			r += "\\textless{}";
			break;

		case '>':
			r += "\\textgreater{}";
			break;

		case 0x2190:
			r += "$\\leftarrow$";
			break;

		case 0x2191:
			r += "$\\uparrow$";
			break;

		case 0x2192:
			r += "$\\rightarrow$";
			break;

		case 0x2193:
			r += "$\\downarrow$";
			break;

		case 0x2194:
			r += "$\\leftrightarrow$";
			break;

		case 0x2195:
			r += "$\\updownarrow$";
			break;

		case 0x2196:
			r += "$\\nwarrow$";
			break;

		case 0x2197:
			r += "$\\nearrow$";
			break;

		case 0x2198:
			r += "$\\searrow$";
			break;

		case 0x2199:
			r += "$\\swarrow$";
			break;

		case 0x21a9:
			r += "$\\hookleftarrow$";
			break;

		case 0x21aa:
			r += "$\\hookrightarrow$";
			break;

		case 0x21d0:
			r += "$\\Leftarrow$";
			break;

		case 0x21d1:
			r += "$\\Uparrow$";
			break;

		case 0x21d2:
			r += "$\\Rightarrow$";
			break;

		case 0x21d3:
			r += "$\\Downarrow$";
			break;

		case 0x21d4:
			r += "$\\Leftrightarrow$";
			break;

		case 0x21d5:
			r += "$\\Updownarrow$";
			break;

		case 0x2200:
			r += "$\\forall$";
			break;

		case 0x2203:
			r += "$\\exists$";
			break;

		case 0x2205:
			r += "$\\emtpyset$";
			break;

		case 0x2208:
			r += "$\\in$";
			break;

		case 0x2209:
			r += "$\\notin$";
			break;

		case 0x220b:
			r += "$\\ni$";
			break;

		case 0x220f:
			r += "$\\prod$";
			break;

		case 0x2211:
			r += "$\\sum$";
			break;

		case 0x2212:
			r += "$-$";
			break;

		case 0x2213:
			r += "$\\mp$";
			break;

		case 0x2215:
			r += "$/$";
			break;

		case 0x2216:
			r += "$\\setminus$";
			break;

		case 0x2217:
			r += "$\\ast$";
			break;

		case 0x2218:
			r += "$\\circ$";
			break;

		case 0x2219:
			r += "$\\bullet$";
			break;

		case 0x221a:
			r += "$\\surd$";
			break;

		case 0x221d:
			r += "$\\propto$";
			break;

		case 0x221e:
			r += "$\\infty$";
			break;

		case 0x2227:
			r += "$\\wedge$";
			break;

		case 0x2228:
			r += "$\\vee$";
			break;

		case 0x2229:
			r += "$\\cap$";
			break;

		case 0x222a:
			r += "$\\cup$";
			break;

		case 0x222b:
			r += "$\\int$";
			break;

		case 0x222e:
			r += "$\\oint$";
			break;

		case 0x223c:
			r += "$\\sim$";
			break;

		case 0x2241:
			r += "$\\not\\sim$";
			break;

		case 0x2243:
			r += "$\\simeq$";
			break;

		case 0x2244:
			r += "$\\not\\simeq$";
			break;

		case 0x2245:
			r += "$\\cong$";
			break;

		case 0x2247:
			r += "$\\not\\cong$";
			break;

		case 0x2248:
			r += "$\\approx$";
			break;

		case 0x2249:
			r += "$\\not\\approx$";
			break;

		case 0x224d:
			r += "$\\asymp$";
			break;

		case 0x2250:
			r += "$\\doteq$";
			break;

		case 0x2260:
			r += "$\\not=$";
			break;

		case 0x2261:
			r += "$\\equiv$";
			break;

		case 0x2262:
			r += "$\\not\\equiv$";
			break;

		case 0x2264:
			r += "$\\leq$";
			break;

		case 0x2265:
			r += "$\\geq$";
			break;

		case 0x226a:
			r += "$\\ll$";
			break;

		case 0x226b:
			r += "$\\gg$";
			break;

		case 0x226d:
			r += "$\\not\\asymp$";
			break;

		case 0x226e:
			r += "$\\not<$";
			break;

		case 0x226f:
			r += "$\\not>$";
			break;

		case 0x2270:
			r += "$\\not\\leq$";
			break;

		case 0x2271:
			r += "$\\not\\geq$";
			break;

		case 0x227a:
			r += "$\\prec$";
			break;

		case 0x227b:
			r += "$\\succ$";
			break;

		case 0x227c:
			r += "$\\preceq$";
			break;

		case 0x227d:
			r += "$\\succeq$";
			break;

		case 0x2280:
			r += "$\\not\\prec$";
			break;

		case 0x2281:
			r += "$\\not\\succ$";
			break;

		case 0x2282:
			r += "$\\subset$";
			break;

		case 0x2283:
			r += "$\\supset$";
			break;

		case 0x2284:
			r += "$\\not\\subset$";
			break;

		case 0x2285:
			r += "$\\not\\supset$";
			break;

		case 0x2286:
			r += "$\\subseteq$";
			break;

		case 0x2287:
			r += "$\\supseteq$";
			break;

		case 0x2288:
			r += "$\\not\\subseteq$";
			break;

		case 0x2289:
			r += "$\\not\\supseteq$";
			break;

		case 0x228e:
			r += "$\\uplas";
			break;

		case 0x228f:
			r += "$\\sqsubset$";
			break;

		case 0x2290:
			r += "$\\sqsupset$";
			break;

		case 0x2291:
			r += "$\\sqsubseteq$";
			break;

		case 0x2292:
			r += "$\\sqsupseteq$";
			break;

		case 0x2293:
			r += "$\\sqcap$";
			break;

		case 0x2294:
			r += "$\\sqcup$";
			break;

		case 0x2295:
			r += "$\\oplus$";
			break;

		case 0x2296:
			r += "$\\ominus$";
			break;

		case 0x2297:
			r += "$\\otimes$";
			break;

		case 0x2298:
			r += "$\\oslash$";
			break;

		case 0x2299:
			r += "$\\odot$";
			break;

		case 0x22a2:
			r += "$\\vdash$";
			break;

		case 0x22a3:
			r += "$\\dashv$";
			break;

		case 0x22a4:
			r += "$\\top$";
			break;

		case 0x22a5:
			r += "$\\bot$";
			break;

		case 0x22a8:
			r += "$\\models$";
			break;

		case 0x22b2:
			r += "$\\lhd$";
			break;

		case 0x22b3:
			r += "$\\rhd$";
			break;

		case 0x22b4:
			r += "$\\unlhd$";
			break;

		case 0x22b5:
			r += "$\\unrhd$";
			break;

		case 0x22c0:
			r += "$\\wedge$";
			break;

		case 0x22c1:
			r += "$\\vee$";
			break;

		case 0x22c2:
			r += "$\\cap$";
			break;

		case 0x22c3:
			r += "$\\cup$";
			break;

		case 0x22c4:
			r += "$\\diamond$";
			break;

		case 0x22c5:
			r += "$\\cdot$";
			break;

		case 0x22c6:
			r += "$\\star$";
			break;

		case 0x22c8:
			r += "$\\bowtie$";
			break;

		case 0x22e2:
			r += "$\\not\\sqsubseteq$";
			break;

		case 0x22e3:
			r += "$\\not\\sqsupseteq$";
			break;

		case '&':
		case '%':
		case '$':
		case '#':
		case '_':
		case '{':
		case '}':
			r.push_back('\\');
			// fall through

		default:
			r.push_back(*i);
			break;
		}
	}
	return r;
}
#endif

std::string METARTAFChart::latex_string_meta(const std::string& x)
{
	return latex_string_meta(Glib::ustring(x));
}

std::ostream& METARTAFChart::dump_latex(std::ostream& os, const stnfilter_t& filter, bool exclude)
{
	for (MetarTafSet::stations_t::const_iterator si(m_set.get_stations().begin()), se(m_set.get_stations().end()); si != se; ++si) {
		if ((filter.find(si->get_stationid()) == filter.end()) ^ exclude)
			continue;
		os << "% station " << si->get_stationid() << ' '
		   << si->get_coord().get_fpl_str();
		if (si->is_wmonr_valid())
			os << " wmo " << std::setw(5) << std::setfill('0') << si->get_wmonr();
		os << std::endl;
		if (si->get_metar().empty() && si->get_taf().empty())
			continue;
		os << "\\myhypertarget{" << si->get_stationid() << "}%" << std::endl;
		for (MetarTafSet::Station::metar_t::const_iterator i(si->get_metar().begin()), e(si->get_metar().end()); i != e; ++i) {
			os << "\\metar";
			switch (i->get_frules()) {
			case MetarTafSet::METAR::frules_lifr:
				os << "lifr";
				break;

			case MetarTafSet::METAR::frules_ifr:
				os << "ifr";
				break;

			case MetarTafSet::METAR::frules_mvfr:
				os << "mvfr";
				break;

			case MetarTafSet::METAR::frules_vfr:
				os << "vfr";
				break;

			default:
				break;
			}
			os << '{' << latex_string_meta(i->get_rawtext()) << '}' << std::endl;
		}
		for (MetarTafSet::Station::taf_t::const_iterator i(si->get_taf().begin()), e(si->get_taf().end()); i != e; ++i) {
			os << "\\taf{" << latex_string_meta(i->get_rawtext()) << '}' << std::endl;
		}
		if (si->is_wmonr_valid()) {
			os << "\\raob{raob" << si->get_stationid() << "}{" << si->get_stationid() << '}' << std::endl;
		}
	}
	return os;
}

std::string METARTAFChart::findstn(const std::string& icao, const Point& coord) const
{
	std::string nearest;
	double bdist(std::numeric_limits<double>::max());
	for (MetarTafSet::stations_t::const_iterator si(m_set.get_stations().begin()), se(m_set.get_stations().end()); si != se; ++si) {
		if (si->get_metar().empty() && si->get_taf().empty())
			continue;
		if (si->get_stationid() == icao)
			return icao;
		if (coord.is_invalid() || si->get_coord().is_invalid())
			continue;
		double dist(si->get_coord().spheric_distance_nmi_dbl(coord));
		if (dist >= bdist)
			continue;
		bdist = dist;
		nearest = si->get_stationid();
	}
	return nearest;
}

std::string METARTAFChart::findstn(const FPlanWaypoint& wpt) const
{
	return findstn(wpt.get_icao(), wpt.get_coord());
}

std::ostream& METARTAFChart::dump_latex_stnlist(std::ostream& os)
{
	stnfilter_t stnfilt;
	if (m_route.get_nrwpt() > 0) {
		std::string icao(findstn(m_route[0]));
		if (!icao.empty()) {
			os << "\\metartafdepsep" << std::endl;
			stnfilter_t filt;
			filt.insert(icao);
			stnfilt.insert(icao);
			dump_latex(os, filt, false);
		}
	}
	if (m_route.get_nrwpt() > 1) {
		std::string icao(findstn(m_route[m_route.get_nrwpt()-1]));
		if (!icao.empty() && stnfilt.find(icao) == stnfilt.end()) {
			os << "\\metartafdestsep" << std::endl;
			stnfilter_t filt;
			filt.insert(icao);
			stnfilt.insert(icao);
			dump_latex(os, filt, false);
		}
	}
	{
		stnfilter_t filt;
		for (alternates_t::const_iterator ai(m_altn.begin()), ae(m_altn.end()); ai != ae; ++ai) {
			std::string icao(findstn(*ai));
			if (icao.empty() || stnfilt.find(icao) != stnfilt.end())
				continue;
			filt.insert(icao);
			stnfilt.insert(icao);
		}
		if (!filt.empty()) {
			os << "\\metartafaltsep" << std::endl;
			dump_latex(os, filt, false);
		}
		os << "\\metartafrtesep" << std::endl;
		dump_latex(os, stnfilt, true);
	}
	return os;
}

std::ostream& METARTAFChart::dump_latex_sigmet(std::ostream& os)
{
	bool first(true);
	for (MetarTafSet::firs_t::const_iterator fi(m_set.get_firs().begin()), fe(m_set.get_firs().end()); fi != fe; ++fi) {
		const MetarTafSet::FIR& fir(*fi);
		const MetarTafSet::FIR::sigmet_t& sigmets(fir.get_sigmet());
		for (MetarTafSet::FIR::sigmet_t::const_iterator si(sigmets.begin()), se(sigmets.end()); si != se; ++si) {
			const MetarTafSet::SIGMET& sigmet(*si);
			if (first)
				os << "\\metartafsigmetsep" << std::endl;
			first = false;
			os << "\\myhypertarget{" << fir.get_ident() << ' ' << sigmet.get_sequence() << "}%" << std::endl;
			std::ostringstream wmohdr, firstline;
			// WMO header
			{
				std::string txdate(Glib::TimeVal(sigmet.get_transmissiontime(), 0).as_iso8601());
				wmohdr << sigmet.get_type_str() << sigmet.get_country()
				       << std::setw(2) << std::setfill('0') << sigmet.get_bulletinnr()
				       << ' ' << sigmet.get_dissemiator() << ' ' << txdate.substr(0, 10) << ' '
				       << txdate.substr(11, 5) << 'Z';
			}
			// first line
			{
				std::string datefrom(Glib::TimeVal(sigmet.get_validfrom(), 0).as_iso8601());
				std::string dateto(Glib::TimeVal(sigmet.get_validto(), 0).as_iso8601());
				std::ostringstream oss;
				firstline << fir.get_ident() << " SIGMET " << sigmet.get_sequence() << " VALID "
					  << datefrom.substr(0, 10) << ' ' << datefrom.substr(11, 5) << "Z/"
					  << dateto.substr(0, 10) << ' ' << dateto.substr(11, 5) << "Z "
					  << sigmet.get_originator();
			}
			os << "\\sigmet{" << latex_string_meta(wmohdr.str()) << "}{"
			   << latex_string_meta(firstline.str()) << "}{" << latex_string_meta(sigmet.get_msg()) << "}\n";
		}
	}
	return os;
}

std::ostream& METARTAFChart::dump_latex_raobs(std::ostream& os)
{
	if (!&m_raobs)
		return os;
	bool first(true);
	for (MetarTafSet::stations_t::const_iterator si(m_set.get_stations().begin()), se(m_set.get_stations().end()); si != se;) {
		if (si->get_wmonr() == ~0U) {
			++si;
			continue;
		}
		if (first) {
			os << "\\metartafraobsep" << std::endl;
			first = false;
		}
		os << "\\begin{tabular*}{\\textwidth}{@{}l@{\\extracolsep{\\fill}}r@{}}" << std::endl << "  ";
		for (unsigned int j = 0; j < 2; ++j) {
			if (j)
				os << " & ";
			if (si == se)
				continue;
			const RadioSounding *snd0(0), *snd1(0);
			unsigned int wmonr(si->get_wmonr());
			{
				RadioSoundings::const_iterator ri(m_raobs.lower_bound(RadioSounding(wmonr, 0))), re(m_raobs.end());
				for (; ri != re && ri->get_wmonr() <= wmonr; ++ri) {
					if (ri->get_wmonr() != wmonr)
						continue;
					snd1 = snd0;
					snd0 = &*ri;
				}
			}
			const std::string& stnid(si->get_stationid());
			bool stnempty(si->get_metar().empty() && si->get_taf().empty());
			++si;
			while (si != se && si->get_wmonr() == ~0U)
				++si;
			if (!snd0)
				continue;
			SkewTChartSounding ch(*snd0, snd1 ? *snd1 : RadioSounding(), std::numeric_limits<int32_t>::min());
			//ch.calc_stability();
			if (true && !m_tempdir.empty()) {
				os << "\\raisebox{11.5cm}{\\myhypertarget{raob" << stnid << '}';
				if (stnempty)
					os << "\\myhypertarget{" << stnid << '}';
				os << '}';
				std::string filename;
				{
					std::ostringstream oss;
					oss << "raob" << wmonr << ".pdf";
					filename = Glib::build_filename(m_tempdir, oss.str());
				}
				{
					double height((14 + 0.7) * 0.8 * (72 / 2.54)), width((10 + (2 * !j)) * 0.8 * (72 / 2.54) + 2);
					Cairo::RefPtr<Cairo::PdfSurface> pdfsurface(Cairo::PdfSurface::create(filename, width, height));
					Cairo::RefPtr<Cairo::Context> cr(Cairo::Context::create(pdfsurface));
					cr->translate(1, 5);
					ch.draw(cr, 10 * 0.8 * (72 / 2.54), 14 * 0.8 * (72 / 2.54), !j);
					cr->show_page();
				}
				os << "\\includegraphics{" << filename << '}';
			} else {
				os << '%' << std::endl;
				SkewTChart::define_latex_colors(os);
				SkewTChart::define_latex_styles(os << "  \\begin{tikzpicture}[scale=0.8,font=\\small,") << "]" << std::endl
								   << "    \\node[anchor=east] at (0,14) {\\myhypertarget{raob" << stnid << '}';
				if (stnempty)
					os << "\\myhypertarget{" << stnid << '}';
				ch.drawtikz(os << "};" << std::endl, 10, 14/*, "skewtgridlines"*/);
				ch.drawtikzwindbarbs(os, 9.5, 14);
				ch.drawtikzhlabels(os, 10, 14);
				if (!j)
					ch.drawtikzvlabels(os, 10, 14, true);
				os << "  \\end{tikzpicture}%" << std::endl;
			}
		}
		os << " \\\\" << std::endl
		   << "\\end{tabular*}" << std::endl;
	}
	return os;
}

void METARTAFChart::draw(const Cairo::RefPtr<Cairo::Context>& cr, int width, int height)
{
	draw(0, m_route.get_nrwpt(), cr, width, height);
}

void METARTAFChart::draw(unsigned int wptidx0, unsigned int wptidx1, const Cairo::RefPtr<Cairo::Context>& cr, int width, int height)
{
	if (wptidx1 < wptidx0 + 2 || m_route[wptidx0].get_coord().is_invalid() || m_route[wptidx1-1].get_coord().is_invalid())
		return;
	Rect bbox(get_extended_bbox(wptidx0, wptidx1));
	loadstn(bbox);
	filterstn();
	cr->save();
	// background
	cr->set_source_rgb(1.0, 1.0, 1.0);
	cr->paint();
	// clip
	cr->rectangle(0, 0, width, height);
	cr->clip();
	cr->translate(width * 0.5, height * 0.5);
	// scaling
	cr->select_font_face("sans", Cairo::FONT_SLANT_NORMAL, Cairo::FONT_WEIGHT_NORMAL);
	cr->set_font_size(12);
	//Point center(m_route[wptidx0].get_coord().halfway(m_route[wptidx1-1].get_coord()));
	Point center(bbox.boxcenter());
	double scalelon, scalelat;
	{
		Point p(bbox.get_northeast() - bbox.get_southwest());
		double lonscale(cos(center.get_lat_rad_dbl()));
		scalelat = std::max(p.get_lon() * lonscale / width, (double)p.get_lat() / height);
		scalelat = 1.0 / scalelat;
		scalelon = scalelat * lonscale;
		scalelat = -scalelat;
	}
	draw_bgndlayer(cr, bbox, center, scalelon, scalelat);
	// draw route
	{
		std::set<std::string> filter;
		set_color_fplan(cr);
		cr->set_line_width(2);
		cr->begin_new_path();
		{
			bool first(true);
			for (unsigned int i(wptidx0); i < wptidx1; ++i) {
				const FPlanWaypoint& wpt(m_route[i]);
				if (wpt.get_coord().is_invalid())
					continue;
				double xc, yc;
				{
					Point pdiff(wpt.get_coord() - center);
					xc = pdiff.get_lon() * scalelon;
					yc = pdiff.get_lat() * scalelat;
				}
				if (first) {
					first = false;
					cr->move_to(xc, yc);
				} else {
					cr->line_to(xc, yc);
				}
			}
			if (!first)
				cr->stroke();
		}
	        if (wptidx1 >= m_route.get_nrwpt()) {
			{
				std::vector<double> dashes;
				dashes.push_back(2);
				dashes.push_back(2);
				cr->set_dash(dashes, 0);
			}
			for (alternates_t::const_iterator ai(m_altn.begin()), ae(m_altn.end()); ai != ae; ++ai) {
				if (ai->get_coord().is_invalid())
					continue;
				const FPlanWaypoint& wpte(m_route[m_route.get_nrwpt()-1]);
				if (wpte.get_coord().is_invalid())
					break;
				const FPlanRoute *falt(0);
				for (altroutes_t::const_iterator di(m_altnroutes.begin()), de(m_altnroutes.end()); di != de; ++di) {
					if (di->get_nrwpt() <= 0 || (*di)[di->get_nrwpt()-1].get_coord().is_invalid() ||
					    (*di)[di->get_nrwpt()-1].get_coord().simple_distance_nmi(ai->get_coord()) >= 1)
						continue;
					falt = &*di;
					break;
				}
				if (falt) {
					bool first(true);
					for (unsigned int i = 0, j = falt->get_nrwpt(); i < j; ++i) {
						const FPlanWaypoint& wpt((*falt)[i]);
						if (wpt.get_coord().is_invalid())
							continue;
						double xc, yc;
						{
							Point pdiff(wpt.get_coord() - center);
							xc = pdiff.get_lon() * scalelon;
							yc = pdiff.get_lat() * scalelat;
						}
						if (first) {
							first = false;
							cr->move_to(xc, yc);
						} else {
							cr->line_to(xc, yc);
						}
					}
					if (!first)
						cr->stroke();
				} else {
					double xc, yc;
					{
						Point pdiff(wpte.get_coord() - center);
						xc = pdiff.get_lon() * scalelon;
						yc = pdiff.get_lat() * scalelat;
					}
					cr->move_to(xc, yc);
					{
						Point pdiff(ai->get_coord() - center);
						xc = pdiff.get_lon() * scalelon;
						yc = pdiff.get_lat() * scalelat;
					}
					cr->line_to(xc, yc);
					cr->stroke();
				}
			}
			cr->unset_dash();
		}
		// use latex overpic environment to place labels?
		for (unsigned int i(wptidx0); i < wptidx1; ++i) {
			const FPlanWaypoint& wpt(m_route[i]);
			if (wpt.get_coord().is_invalid())
				continue;
			if (i && wpt.get_pathcode() == FPlanWaypoint::pathcode_sid)
				continue;
			if (i > 0 && i + 1 != m_route.get_nrwpt() && m_route[i-1].get_pathcode() == FPlanWaypoint::pathcode_star)
				continue;
			if (wpt.get_type() >= FPlanWaypoint::type_generated_start && wpt.get_type() <= FPlanWaypoint::type_generated_end)
				continue;
			Glib::ustring txt(wpt.get_icao());
			if (txt.empty() || (wpt.get_type() == FPlanWaypoint::type_airport && AirportsDb::Airport::is_fpl_zzzz(txt)))
				txt = wpt.get_name();
			if (txt.empty())
				continue;
			double xc, yc;
			{
				Point pdiff(wpt.get_coord() - center);
				xc = pdiff.get_lon() * scalelon;
				yc = pdiff.get_lat() * scalelat;
			}
			unsigned int quadrant(0);
			if (i)
				quadrant |= 1 << (((m_route[i-1].get_truetrack() + 0x8000) >> 14) & 3);
			if (i + 1 == m_route.get_nrwpt()) {
				for (alternates_t::const_iterator ai(m_altn.begin()), ae(m_altn.end()); ai != ae; ++ai) {
					if (ai->get_coord().is_invalid())
						continue;
					unsigned int q(wpt.get_coord().spheric_true_course_dbl(ai->get_coord()) * FPlanWaypoint::from_deg + 0x8000);
					quadrant |= 1 << ((q >> 14) & 3);
				}
			} else {
				quadrant |= 1 << ((wpt.get_truetrack() >> 14) & 3);
			}
			Cairo::TextExtents ext;
			cr->get_text_extents(txt, ext);
			xc -= ext.x_bearing;
			yc -= ext.y_bearing;
			// place label
			if (!(quadrant & 0x3)) {
				xc += 3;
				yc -= ext.height * 0.5;
			} else if (!(quadrant & 0x6)) {
				xc -= ext.width * 0.5;
				yc += 3;
			} else if (!(quadrant & 0xC)) {
				xc -= 3 + ext.width;
				yc -= ext.height * 0.5;
			} else if (!(quadrant & 0x9)) {
				xc -= ext.width * 0.5;
				yc -= 3 + ext.height;
			} else if (!(quadrant & 0x1)) {
				xc += 3;
				yc -= 3 + ext.height;
			} else if (!(quadrant & 0x2)) {
				xc += 3;
				yc += 3;
			} else if (!(quadrant & 0x4)) {
				xc -= 3 + ext.width;
				yc += 3;
			} else if (!(quadrant & 0x8)) {
				xc -= 3 + ext.width;
				yc -= 3 + ext.height;
			} else {
				xc -= ext.width * 0.5;
				yc -= ext.height * 0.5;
			}
			cr->set_source_rgb(1.0, 1.0, 1.0);
			cr->move_to(xc, yc + 1);
			cr->show_text(txt);
			cr->move_to(xc, yc - 1);
			cr->show_text(txt);
			cr->move_to(xc + 1, yc);
			cr->show_text(txt);
			cr->move_to(xc - 1, yc);
			cr->show_text(txt);
			{
				bool set(false);
				MetarTafSet::METAR::frules_t frules(MetarTafSet::METAR::frules_unknown);
				if (wpt.get_type() == FPlanWaypoint::type_airport) {
					for (MetarTafSet::stations_t::const_iterator si(m_set.get_stations().begin()), se(m_set.get_stations().end()); si != se; ++si) {
						if (si->get_stationid() != wpt.get_icao())
							continue;
						set = true;
						filter.insert(wpt.get_icao());
						if (si->get_metar().empty())
							break;
						frules = si->get_metar().rbegin()->get_frules();
						break;
					}
				}
				if (set)
					set_color_frules(cr, frules);
				else
					set_color_fplan(cr);
			}
			cr->move_to(xc, yc);
			cr->show_text(txt);
		}
		if (wptidx1 >= m_route.get_nrwpt()) {
			for (alternates_t::const_iterator ai(m_altn.begin()), ae(m_altn.end()); ai != ae; ++ai) {
				if (ai->get_coord().is_invalid())
					continue;
				const FPlanWaypoint& wpt(m_route[m_route.get_nrwpt()-1]);
				const FPlanWaypoint& wptp(m_route[m_route.get_nrwpt()-2]);
				if (wpt.get_coord() == ai->get_coord())
					continue;
				Glib::ustring txt(ai->get_icao());
				if (txt.empty() || AirportsDb::Airport::is_fpl_zzzz(txt))
					txt = ai->get_name();
				if (txt.empty())
					continue;
				double xc, yc;
				{
					Point pdiff(ai->get_coord() - center);
					xc = pdiff.get_lon() * scalelon;
					yc = pdiff.get_lat() * scalelat;
				}
				unsigned int quadrant(wpt.get_coord().spheric_true_course_dbl(ai->get_coord()) * FPlanWaypoint::from_deg + 0x8000);
				quadrant = 1 << ((quadrant >> 14) & 3);
				quadrant |= 1 << (((wptp.get_truetrack() + 0x8000) >> 14) & 3);
				Cairo::TextExtents ext;
				cr->get_text_extents(txt, ext);
				xc -= ext.x_bearing;
				yc -= ext.y_bearing;
				// place label
				if (!(quadrant & 0x3)) {
					xc += 3;
					yc -= ext.height * 0.5;
				} else if (!(quadrant & 0x6)) {
					xc -= ext.width * 0.5;
					yc += 3;
				} else if (!(quadrant & 0xC)) {
					xc -= 3 + ext.width;
					yc -= ext.height * 0.5;
				} else if (!(quadrant & 0x9)) {
					xc -= ext.width * 0.5;
					yc -= 3 + ext.height;
				} else if (!(quadrant & 0x1)) {
					xc += 3;
					yc -= 3 + ext.height;
				} else if (!(quadrant & 0x2)) {
					xc += 3;
					yc += 3;
				} else if (!(quadrant & 0x4)) {
					xc -= 3 + ext.width;
					yc += 3;
				} else if (!(quadrant & 0x8)) {
					xc -= 3 + ext.width;
					yc -= 3 + ext.height;
				} else {
					xc -= ext.width * 0.5;
					yc -= ext.height * 0.5;
				}
				cr->set_source_rgb(1.0, 1.0, 1.0);
				cr->move_to(xc, yc + 1);
				cr->show_text(txt);
				cr->move_to(xc, yc - 1);
				cr->show_text(txt);
				cr->move_to(xc + 1, yc);
				cr->show_text(txt);
				cr->move_to(xc - 1, yc);
				cr->show_text(txt);
				{
					bool set(false);
					MetarTafSet::METAR::frules_t frules(MetarTafSet::METAR::frules_unknown);
					for (MetarTafSet::stations_t::const_iterator si(m_set.get_stations().begin()), se(m_set.get_stations().end()); si != se; ++si) {
						if (si->get_stationid() != ai->get_icao())
							continue;
						set = true;
						filter.insert(ai->get_icao());
						if (si->get_metar().empty())
							break;
						frules = si->get_metar().rbegin()->get_frules();
						break;
					}
					if (set)
						set_color_frules(cr, frules);
					else
						set_color_fplan(cr);
				}
				cr->move_to(xc, yc);
				cr->show_text(txt);
			}
		}
		// draw metar/taf stations
		for (MetarTafSet::stations_t::const_iterator si(m_set.get_stations().begin()), se(m_set.get_stations().end()); si != se; ++si) {
			if (filter.find(si->get_stationid()) != filter.end())
				continue;
			if (!bbox.is_inside(si->get_coord()))
				continue;
			Cairo::TextExtents ext;
			cr->get_text_extents(si->get_stationid(), ext);
			double xc(-ext.width * 0.5 - ext.x_bearing), yc(-ext.height * 0.5 - ext.y_bearing);
			{
				Point pdiff(si->get_coord() - center);
				xc += pdiff.get_lon() * scalelon;
				yc += pdiff.get_lat() * scalelat;
			}
			cr->set_source_rgb(1.0, 1.0, 1.0);
			cr->move_to(xc, yc + 1);
			cr->show_text(si->get_stationid());
			cr->move_to(xc, yc - 1);
			cr->show_text(si->get_stationid());
			cr->move_to(xc + 1, yc);
			cr->show_text(si->get_stationid());
			cr->move_to(xc - 1, yc);
			cr->show_text(si->get_stationid());
			{
				MetarTafSet::METAR::frules_t frules(MetarTafSet::METAR::frules_unknown);
				if (!si->get_metar().empty())
					frules = si->get_metar().rbegin()->get_frules();
				set_color_frules(cr, frules);
			}
			cr->move_to(xc, yc);
			cr->show_text(si->get_stationid());
		}
	}
	// end
	cr->restore();
}

void METARTAFChart::draw_firs(std::ostream& os, LabelLayout& lbl, const Rect& bbox, const Point& center,
			      double width, double height, double scalelon, double scalelat)
{
	for (MetarTafSet::firs_t::iterator fi(m_set.get_firs().begin()), fe(m_set.get_firs().end()); fi != fe; ++fi) {
		MetarTafSet::FIR& fir(const_cast<MetarTafSet::FIR&>(*fi));
		MetarTafSet::FIR::sigmet_t& sigmets(fir.get_sigmet());
		for (MetarTafSet::FIR::sigmet_t::iterator si(sigmets.begin()), se(sigmets.end()); si != se; ) {
			const MetarTafSet::SIGMET& sigmet(*si);
			MetarTafSet::FIR::sigmet_t::iterator si1(si);
			++si;
			MultiPolygonHole poly(sigmet.get_poly());
			if (poly.empty()) {
				sigmets.erase(si1);
				continue;
			}
			poly.geos_intersect(PolygonHole(bbox));
			if (poly.empty()) {
				sigmets.erase(si1);
				continue;
			}
			os << "  % FIR " << fir.get_ident() << " sigmet " << sigmet.get_msg() << std::endl;
			polypath_fill(os, "fill=red,fill opacity=0.2,draw=red,thick,draw opacity=1.0",
				      poly, center, width, height, scalelon, scalelat);
			double xc, yc;
			std::string txt(fir.get_ident() + " " + sigmet.get_sequence());
			{
				Point pdiff(poly[0].get_exterior().centroid());
				if (pdiff.is_invalid())
					continue;
				pdiff -= center;
				xc = pdiff.get_lon() * scalelon + 0.5 * width;
				yc = pdiff.get_lat() * scalelat + 0.5 * height;
			}
			std::ostringstream oss;
			oss << "\\sigmetlabellink{" << txt << '}';
			lbl.push_back(Label(txt, oss.str(), xc, yc, Label::placement_center, false, false));
		}
	}
}

std::ostream& METARTAFChart::dump_latex_legend(std::ostream& os)
{
	static const char legend[] =
		"\n\n"
		"\\setlength{\\extrarowheight}{2pt}\n"
		"\\small\n"
		"\\begin{center}\n"
		"  \\begin{tabular}{|l|lll|}\n"
		"    \\hline\n"
		"    \\multicolumn{1}{|c|}{\\textbf{Category}} & \\multicolumn{1}{c}{\\textbf{Ceiling}} & & \\multicolumn{1}{c|}{\\textbf{Visibility}} \\\\\n"
		"    \\hline\n"
		"    \\textcolor{colunknown}{Unspecified} & \\multicolumn{3}{c|}{no flight rule classification} \\\\\n"
		"    \\hline\n"
		"    \\textcolor{collifr}{Low Instrument Flight Rules LIFR} & below 500 feet AGL & and/or & less than 1 mile \\\\\n"
		"    \\hline\n"
		"    \\textcolor{colifr}{Instrument Flight Rules IFR} & 500 to below 1000 feet AGL & and/or & 1 mile to less than 3 miles \\\\\n"
		"    \\hline\n"
		"    \\textcolor{colmvfr}{Marginal Visual Flight Rules MVFR} & 1000 to 3000 feet AGL & and/or & 3 mile to less than 5 miles \\\\\n"
		"    \\hline\n"
		"    \\textcolor{colvfr}{Visual Flight Rules VFR} & greater than 3000 feet AGL & and & greater than 5 miles \\\\\n"
		"    \\hline\n"
		"  \\end{tabular}\n"
		"\\end{center}\n\n";
	return os << legend;
}

void METARTAFChart::draw(std::ostream& os, double width, double height)
{
	draw(0, m_route.get_nrwpt(), os, width, height);
}

unsigned int METARTAFChart::get_chart_strip(unsigned int wptstart) const
{
	const unsigned int n(m_route.get_nrwpt());
	if (wptstart >= n || chart_strip_length <= 0. || m_route[wptstart].get_coord().is_invalid())
		return n;
	unsigned int wptend(wptstart);
	for (unsigned int i(wptend + 1); i < n; ++i) {
		if (m_route[i].get_coord().is_invalid())
			continue;
		if (wptend == wptstart)
			wptend = i;
		if (m_route[wptstart].get_coord().spheric_distance_nmi_dbl(m_route[i].get_coord()) > chart_strip_length)
			break;
		wptend = i;
	}
	if (wptend == wptstart)
		return n;
	return wptend + 1U;
}

void METARTAFChart::draw(unsigned int wptidx0, unsigned int wptidx1, std::ostream& os, double width, double height)
{
	if (wptidx1 < wptidx0 + 2 || m_route[wptidx0].get_coord().is_invalid() || m_route[wptidx1-1].get_coord().is_invalid())
		return;
	Rect bbox(get_extended_bbox(wptidx0, wptidx1));
	loadstn(bbox);
	filterstn();
	//Point center(m_route[wptidx0].get_coord().halfway(m_route[wptidx1-1].get_coord()));
	Point center(bbox.boxcenter());
	double scalelon, scalelat;
	{
		Point p(bbox.get_northeast() - bbox.get_southwest());
		double lonscale(cos(center.get_lat_rad_dbl()));
		scalelat = std::max(p.get_lon() * lonscale / width, (double)p.get_lat() / height);
		scalelat = 1.0 / scalelat;
		scalelon = scalelat * lonscale;
		height = p.get_lat() * scalelat;
		width = p.get_lon() * scalelon;
	}
	os << "  \\begin{center}\\begin{tikzpicture}" << std::endl
	   << "    \\path[clip] (0,0) rectangle " << pgfcoord(width, height) << ';' << std::endl;
	if (true)
		os << "    \\georefstart{4326}" << std::endl
		   << "    \\georefbound{0}{0}" << std::endl
		   << "    \\georefbound" << georefcoord(width, 0) << std::endl
		   << "    \\georefbound" << georefcoord(width, height) << std::endl
		   << "    \\georefbound" << georefcoord(0, height) << std::endl
		   << "    \\georefmap" << georefcoord(bbox.get_southwest(), center, width, height, scalelon, scalelat) << std::endl
		   << "    \\georefmap" << georefcoord(bbox.get_southeast(), center, width, height, scalelon, scalelat) << std::endl
		   << "    \\georefmap" << georefcoord(bbox.get_northeast(), center, width, height, scalelon, scalelat) << std::endl
		   << "    \\georefmap" << georefcoord(bbox.get_northwest(), center, width, height, scalelon, scalelat) << std::endl
		   << "    \\georefend" << std::endl;
	draw_bgndlayer(os, bbox, center, width, height, scalelon, scalelat);
	LabelLayout lbl(width, height, 1.2 * 4 * tex_char_width, 1.2 * tex_char_height);
	draw_firs(os, lbl, bbox, center, width, height, scalelon, scalelat);
	// draw route
	{
		std::set<std::string> filter;
		bool first(true);
		for (unsigned int i(wptidx0); i < wptidx1; ++i) {
			const FPlanWaypoint& wpt(m_route[i]);
			if (wpt.get_coord().is_invalid())
				continue;
			double xc, yc;
			{
				Point pdiff(wpt.get_coord() - center);
				xc = pdiff.get_lon() * scalelon + 0.5 * width;
				yc = pdiff.get_lat() * scalelat + 0.5 * height;
			}
			if (first) {
				os << "    \\path[draw,thick,colfplan] ";
				first = false;
			} else {
				os << " -- ";
			}
			os << pgfcoord(xc, yc);
			if (i && wpt.get_pathcode() == FPlanWaypoint::pathcode_sid)
				continue;
			if (i > 0 && i + 1 != m_route.get_nrwpt() && m_route[i-1].get_pathcode() == FPlanWaypoint::pathcode_star)
				continue;
			if (wpt.get_type() >= FPlanWaypoint::type_generated_start && wpt.get_type() <= FPlanWaypoint::type_generated_end)
				continue;
			Glib::ustring txt(wpt.get_icao());
			if (txt.empty() || (wpt.get_type() == FPlanWaypoint::type_airport && AirportsDb::Airport::is_fpl_zzzz(txt)))
				txt = wpt.get_name();
			if (txt.empty())
				continue;
			unsigned int quadrant(0);
			if (i)
				quadrant |= 1 << (((m_route[i-1].get_truetrack() + 0x8000) >> 14) & 3);
			if (i + 1 == m_route.get_nrwpt()) {
				for (alternates_t::const_iterator ai(m_altn.begin()), ae(m_altn.end()); ai != ae; ++ai) {
					if (ai->get_coord().is_invalid())
						continue;
					unsigned int q(wpt.get_coord().spheric_true_course_dbl(ai->get_coord()) * FPlanWaypoint::from_deg + 0x8000);
					quadrant |= 1 << ((q >> 14) & 3);
				}
			} else {
				quadrant |= 1 << ((wpt.get_truetrack() >> 14) & 3);
			}
			// place label: quadrants
			// 8 1
			// 4 2
			Label::placement_t placement(Label::placement_from_quadrant(quadrant));
			{
				std::string col("colfplan");
				if (wpt.get_type() == FPlanWaypoint::type_airport) {
					for (MetarTafSet::stations_t::const_iterator si(m_set.get_stations().begin()), se(m_set.get_stations().end()); si != se; ++si) {
						if (si->get_stationid() != wpt.get_icao())
							continue;
						col = "colunknown";
						filter.insert(wpt.get_icao());
						if (si->get_metar().empty())
							break;
						col = si->get_metar().rbegin()->get_latex_col();
						break;
					}
				}
				std::ostringstream oss;
				oss << "\\stationlabel" << (col == "colfplan" ? "" : "link")
				    << '{' << txt << "}{" << col << '}';
				lbl.push_back(Label(txt, oss.str(), xc, yc, placement, true, i == wptidx0 || (i + 1) >= wptidx1));
			}
		}
		if (!first)
			os << ';' << std::endl;
		if (wptidx1 >= m_route.get_nrwpt()) {
			for (alternates_t::const_iterator ai(m_altn.begin()), ae(m_altn.end()); ai != ae; ++ai) {
				if (ai->get_coord().is_invalid())
					continue;
				const FPlanWaypoint& wpte(m_route[m_route.get_nrwpt()-1]);
				if (wpte.get_coord().is_invalid())
					break;
				const FPlanRoute *falt(0);
				for (altroutes_t::const_iterator di(m_altnroutes.begin()), de(m_altnroutes.end()); di != de; ++di) {
					if (di->get_nrwpt() <= 0 || (*di)[di->get_nrwpt()-1].get_coord().is_invalid() ||
					    (*di)[di->get_nrwpt()-1].get_coord().simple_distance_nmi(ai->get_coord()) >= 1)
						continue;
					falt = &*di;
					break;
				}
				double xc, yc;
				if (falt) {
					bool first(true);
					for (unsigned int i = 0, j = falt->get_nrwpt(); i < j; ++i) {
						const FPlanWaypoint& wpt((*falt)[i]);
						if (wpt.get_coord().is_invalid())
							continue;
						{
							Point pdiff(wpt.get_coord() - center);
							xc = pdiff.get_lon() * scalelon + 0.5 * width;
							yc = pdiff.get_lat() * scalelat + 0.5 * height;
						}
						if (first) {
							os << "    \\path[draw,thick,colfplan,dash pattern=on 2pt off 2pt] ";
							first = false;
						} else {
							os << " -- ";
						}
						os << pgfcoord(xc, yc);
					}
					if (first)
						continue;
				} else {
					{
						Point pdiff(wpte.get_coord() - center);
						xc = pdiff.get_lon() * scalelon + 0.5 * width;
						yc = pdiff.get_lat() * scalelat + 0.5 * height;
					}
					os << "    \\path[draw,thick,colfplan,dash pattern=on 2pt off 2pt] " << pgfcoord(xc, yc);
					{
						Point pdiff(ai->get_coord() - center);
						xc = pdiff.get_lon() * scalelon + 0.5 * width;
						yc = pdiff.get_lat() * scalelat + 0.5 * height;
					}
					os << " -- " << pgfcoord(xc, yc);
				}
				if (wpte.get_coord() == ai->get_coord()) {
					os << ';' << std::endl;
					continue;
				}
				Glib::ustring txt(ai->get_icao());
				if (txt.empty() || AirportsDb::Airport::is_fpl_zzzz(txt))
					txt = ai->get_name();
				os << ';' << std::endl;
				if (txt.empty())
					continue;
				const FPlanWaypoint& wptp(m_route[m_route.get_nrwpt()-2]);
				unsigned int quadrant(wpte.get_coord().spheric_true_course_dbl(ai->get_coord()) * FPlanWaypoint::from_deg + 0x8000);
				quadrant = 1 << ((quadrant >> 14) & 3);
				quadrant |= 1 << (((wptp.get_truetrack() + 0x8000) >> 14) & 3);
				// place label: quadrants
				// 8 1
				// 4 2
				Label::placement_t placement(Label::placement_from_quadrant(quadrant));
				{
					std::string col("colfplan");
					for (MetarTafSet::stations_t::const_iterator si(m_set.get_stations().begin()), se(m_set.get_stations().end()); si != se; ++si) {
						if (si->get_stationid() != ai->get_icao())
							continue;
						col = "colunknown";
						filter.insert(ai->get_icao());
						if (si->get_metar().empty())
							break;
						col = si->get_metar().rbegin()->get_latex_col();
						break;
					}
					std::ostringstream oss;
					oss << "\\stationlabel" << (col == "colfplan" ? "" : "link") << '{' << txt << "}{" << col << '}';
					lbl.push_back(Label(txt, oss.str(), xc, yc, placement, true, true));
				}
			}
		}
		lbl.remove_overlapping_labels();
		// draw metar/taf stations
		for (MetarTafSet::stations_t::const_iterator si(m_set.get_stations().begin()), se(m_set.get_stations().end()); si != se; ++si) {
			if (filter.find(si->get_stationid()) != filter.end())
				continue;
			if (!bbox.is_inside(si->get_coord()))
				continue;
			double xc, yc;
			{
				Point pdiff(si->get_coord() - center);
				xc = pdiff.get_lon() * scalelon + 0.5 * width;
				yc = pdiff.get_lat() * scalelat + 0.5 * height;
			}
			{
				MetarTafSet::METAR::frules_t frules(MetarTafSet::METAR::frules_unknown);
				if (!si->get_metar().empty())
					frules = si->get_metar().rbegin()->get_frules();
				std::ostringstream oss;
				oss << "\\stationlabellink{" << si->get_stationid() << "}{" << MetarTafSet::METAR::get_latex_col(frules) << '}';
				lbl.push_back(Label(si->get_stationid(), oss.str(), xc, yc, Label::placement_center, false, false));
			}
		}
	}
	// output labels
	lbl.setk(1);
	if (!false)
		std::cerr << "force directed layout: a " << lbl.total_attractive_force()
			  << " r " << lbl.total_repulsive_force() << std::endl;
	for (double temp = 0.05; temp > 0.001; temp *= 0.75) {
		lbl.iterate(temp);
		if (!false)
			std::cerr << "force directed layout: temp " << temp << " a " << lbl.total_attractive_force()
				  << " r " << lbl.total_repulsive_force() << std::endl;
	}
	for (LabelLayout::const_iterator li(lbl.begin()), le(lbl.end()); li != le; ++li) {
		if (!li->is_fixed())
			continue;
		os << "    \\node[" << li->get_tikz_anchor() << "] at " << pgfcoord(li->get_anchorx(), li->get_anchory())
		   << " {" << li->get_code() << "};" << std::endl;
	}
	for (LabelLayout::const_iterator li(lbl.begin()), le(lbl.end()); li != le; ++li) {
		if (li->is_fixed())
			continue;
		os << "    \\node[" << li->get_tikz_anchor() << "] at " << pgfcoord(li->get_x(), li->get_y())
		   << " {" << li->get_code() << "};" << std::endl;
	}
	// end
	os << "  \\end{tikzpicture}\\end{center}" << std::endl << std::endl;
}

#if 0
WMOStation::WMOStation(const char *icao, const char *name, const char *state, const char *country,
		       unsigned int wmonr, unsigned int wmoregion, const Point& coord,
		       TopoDb30::elev_t elev, bool basic)
	: m_icao(icao), m_name(name), m_state(state), m_country(country),
	  m_wmonr(wmonr), m_wmoregion(wmoregion), m_coord(coord), m_elev(elev), m_basic(basic)
{
}
#endif

WMOStation::const_iterator WMOStation::find(unsigned int wmonr)
{
	WMOStation w(0, 0, 0, 0, wmonr);
	const_iterator i(std::lower_bound(begin(), end(), w, WMONrSort()));
	if (i == end() || i->get_wmonr() != wmonr)
		return end();
	return i;
}

RadioSounding::Level::Level(uint16_t press, int16_t temp, int16_t dewpt, uint16_t winddir, uint16_t windspd)
	: m_press(press), m_temp(temp), m_dewpt(dewpt), m_winddir(winddir), m_windspeed(windspd), m_flags(0)
{
}

int RadioSounding::Level::compare(const Level& x) const
{
	if (get_press_dpa() < x.get_press_dpa())
		return 1;
	if (x.get_press_dpa() < get_press_dpa())
		return -1;
	return 0;
}

std::ostream& RadioSounding::Level::print(std::ostream& os) const
{
	std::ostringstream oss;
	oss << std::fixed << std::setw(6) << std::setprecision(1) << (get_press_dpa() * 0.1) << ' ';
	if (is_temp_valid())
		oss << std::setw(5) << std::setprecision(1) << (get_temp_degc10() * 0.1);
	else
		oss << "-----";
	oss << ' ';
	if (is_dewpt_valid())
		oss << std::setw(5) << std::setprecision(1) << (get_dewpt_degc10() * 0.1);
	else
		oss << "-----";
	oss << ' ';
	if (is_wind_valid())
		oss << std::setw(3) << get_winddir_deg() << '/'
		   << std::setw(2) << Point::round<int,float>(get_windspeed_kts16() * (1.f / 16));
	else
		oss << "---/--";
	if (is_surface())
		oss << " SFC";
	if (is_maxwind())
		oss << " MAXW";
	if (is_tropopause())
		oss << " TROPO";
	return os << oss.str();
}

RadioSounding::Level::operator GRIB2::WeatherProfilePoint::SoundingSurface(void) const
{
	GRIB2::WeatherProfilePoint::SoundingSurface s;
	s.set_press(get_press_dpa() * 0.1);
	if (is_temp_valid())
		s.set_temp(get_temp_degc10() * 0.1 + IcaoAtmosphere<double>::degc_to_kelvin);
	if (is_dewpt_valid())
		s.set_dewpt(get_dewpt_degc10() * 0.1 + IcaoAtmosphere<double>::degc_to_kelvin);
	if (is_wind_valid()) {
		s.set_winddir(get_winddir_deg());
		s.set_windspeed(get_windspeed_kts16() * (1.0f / 16));
	}
	return s;
}

RadioSounding::RadioSounding(unsigned int wmonr, time_t obstm)
	: m_obstime(obstm), m_wmonr(wmonr)
{
}

int RadioSounding::compare(const RadioSounding& x) const
{
	if (get_wmonr() < x.get_wmonr())
		return -1;
	if (x.get_wmonr() < get_wmonr())
		return 1;
	if (get_obstime() < x.get_obstime())
		return -1;
	if (x.get_obstime() < get_obstime())
		return 1;
	return 0;
}

RadioSounding::Level& RadioSounding::insert_level(uint16_t press)
{
	std::pair<levels_t::iterator,bool> i(m_levels.insert(RadioSounding::Level(press)));
	return const_cast<RadioSounding::Level&>(*(i.first));
}

std::ostream& RadioSounding::print(std::ostream& os) const
{
	os << "Station " << std::setw(5) << std::setfill('0') << get_wmonr();
	{
		WMOStation::const_iterator stn(WMOStation::find(get_wmonr()));
		if (stn != WMOStation::end()) {
			if (stn->get_icao())
				os << ' ' << stn->get_icao();
			if (!stn->get_coord().is_invalid())
				os << ' ' << stn->get_coord().get_fpl_str();
			if (stn->get_elev() != TopoDb30::nodata)
				os << " elev " << stn->get_elev();
		}
	}
	os << ' ' << Glib::TimeVal(get_obstime(), 0).as_iso8601() << std::endl;
	for (levels_t::const_iterator li(get_levels().begin()), le(get_levels().end()); li != le; ++li)
		li->print(os << "  ") << std::endl;
	return os;
}

RadioSounding::operator soundingsurfaces_t(void) const
{
	soundingsurfaces_t s;
	for (levels_t::const_iterator li(get_levels().begin()), le(get_levels().end()); li != le; ++li) {
		GRIB2::WeatherProfilePoint::SoundingSurface x(*li);
		if (!x.is_temp_valid())
			continue;
		s.insert(x);
	}
	return s;
}

std::ostream& RadioSoundings::print(std::ostream& os) const
{
	for (const_iterator i(begin()), e(end()); i != e; ++i)
		i->print(os) << std::endl;
	return os;
}

// http://geotest.tamu.edu/userfiles/ATMO251/UpperAirCode.pdf
// http://weather.unisys.com/wxp/Appendices/Formats/TEMP.html

unsigned int RadioSoundings::getint(const std::string& s, unsigned int i, unsigned int n)
{
	if (!n || i > s.size() || n > s.size() || (i + n) > s.size())
		return ~0U;
	unsigned int v(0);
	for (; n > 0; --n, ++i) {
		if (!std::isdigit(s[i]))
			return ~0U;
		v *= 10;
		v += s[i] - '0';
	}
	return v;
}

time_t RadioSoundings::getdate(bool& knots, const std::string& s, time_t filedate)
{
	knots = false;
	unsigned int day(getint(s, 0, 2)), hour(getint(s, 2, 2));
	if (day == ~0U || hour == ~0U)
		return 0;
	if (day >= 50) {
		knots = true;
		day -= 50;
	}
	struct tm tm;
	{
		time_t x(filedate);
		if (!gmtime_r(&x, &tm))
			return 0;
	}
	tm.tm_mday = day;
	tm.tm_hour = hour;
	tm.tm_min = 0;
	tm.tm_sec = 0;
	tm.tm_isdst = 0;
	time_t t(timegm(&tm));
	if (t - filedate > 15*24*60*60) {
		if (tm.tm_mon) {
			--tm.tm_mon;
		} else {
			tm.tm_mon = 11;
			--tm.tm_year;
		}
		t = timegm(&tm);
	}
	return t;
}

RadioSounding& RadioSoundings::parse_header(bool& knots, bool& newrs, const std::vector<std::string>& tok, time_t filedate)
{
	static const RadioSounding invalid(~0U, 0);
	knots = newrs = false;
	if (tok.size() < 2 || tok[0].size() != 5 || tok[1].size() != 5)
		return const_cast<RadioSounding&>(invalid);
	time_t obstime(getdate(knots, tok[0], filedate));
	unsigned int wmonr(getint(tok[1], 0, 5));
	if (wmonr == ~0U)
		return const_cast<RadioSounding&>(invalid);
	std::pair<iterator,bool> i(insert(RadioSounding(wmonr, obstime)));
	newrs = i.second;
	return const_cast<RadioSounding&>(*(i.first));
}

unsigned int RadioSoundings::parse_ttaa(const std::vector<std::string>& tok, time_t filedate, uint16_t pressmul)
{
	bool knots(false), newrs(false);
	RadioSounding& rs(parse_header(knots, newrs, tok, filedate));
	if (rs.get_wmonr() == ~0U)
		return 0;
	unsigned int lastwindpress(0);
	switch (tok[0][4]) {
	case '4':
		if (pressmul < 10)
			break;
		// fall through
	case '1':
	case '2':
	case '3':
	case '5':
	case '7':
		lastwindpress = (tok[0][4] - '0') * 10 * pressmul;
		break;

	case '8':
		if (pressmul < 10)
			break;
		lastwindpress = 850;
		break;

	case '0':
		if (pressmul < 10)
			break;
		lastwindpress = 1000;
		break;

	case '/':
		lastwindpress = ~0U;
		break;

	default:
		break;
	}
	std::vector<std::string>::size_type idx(4);
	for (; idx < tok.size(); idx += 3) {
		if (tok[idx-2].size() != 5 || tok[idx-1].size() != 5 || tok[idx].size() != 5)
			break;
		unsigned int x0(getint(tok[idx-2], 0, 2));
		if (x0 == ~0U)
			break;
		unsigned int x1(getint(tok[idx-2], 2, 3));
		if (x0 == 77 || x0 == 66) {
			if (x1 == 999) {
				idx += 3;
				break;
			}
			unsigned int y0(getint(tok[idx-1], 0, 2)), y1(getint(tok[idx-1], 2, 3));
			// disregard windshear info
			if (y0 != ~0U && y1 != ~0U) {
				RadioSounding::Level& lvl(rs.insert_level(x1 * pressmul));
				idx += 3;
				y0 *= 10;
				if (y1 >= 500) {
					y0 += 5;
					y1 -= 500;
				}
				lvl.set_winddir_deg(y0);
				if (knots) {
					lvl.set_windspeed_kts16(y1 * 16);
				} else {
					lvl.set_windspeed_kts16(Point::round<int,float>((Point::mps_to_kts * 16) * y1));
				}
				lvl.set_maxwind(true);
			}
			break;
		}
		unsigned int y0(getint(tok[idx-1], 0, 3)), y1(getint(tok[idx-1], 3, 2));
		unsigned int z0(getint(tok[idx-0], 0, 2)), z1(getint(tok[idx-0], 2, 3));
		uint16_t press(0);
		if (x0 == 99 || x0 == 88) {
			if (x1 == ~0U)
				break;
			press = x1;
			if (x0 == 99) {
				if (press < 100)
					press += 1000;
			} else {
				if (press == 999) {
					idx -= 2;
					continue;
				}
			}
		} else {
			press = x0 * 10;
			switch (x0 % 10) {
			case 2:
			case 7:
				press += 5;
				break;

			default:
				break;
			}
			if (press < 100)
				press += 1000;
			if (press < lastwindpress) {
				--idx;
				z0 = z1 = ~0U;
			}
		}
		RadioSounding::Level& lvl(rs.insert_level(press * pressmul));
		if (y0 != ~0U) {
			int16_t temp(y0);
			if (temp & 1)
				temp = -temp;
			lvl.set_temp_degc10(temp);
			if (y1 != ~0U) {
				if (y1 > 50)
					temp -= (y1 - 50) * 10;
				else
					temp -= y1;
				lvl.set_dewpt_degc10(temp);
			}
		}
		if (z0 != ~0U && z1 != ~0U) {
			z0 *= 10;
			if (z1 >= 500) {
				z0 += 5;
				z1 -= 500;
			}
			lvl.set_winddir_deg(z0);
			if (knots) {
				lvl.set_windspeed_kts16(z1 * 16);
			} else {
				lvl.set_windspeed_kts16(Point::round<int,float>((Point::mps_to_kts * 16) * z1));
			}
		}
		if (x0 == 99)
			lvl.set_surface(true);
		if (x0 == 88)
			lvl.set_tropopause(true);
	}
	return newrs;
}

unsigned int RadioSoundings::parse_ttbb(const std::vector<std::string>& tok, time_t filedate, uint16_t pressmul)
{
	bool knots(false), newrs(false);
	RadioSounding& rs(parse_header(knots, newrs, tok, filedate));
	if (rs.get_wmonr() == ~0U)
		return 0;
	std::vector<std::string>::size_type idx(3);
	for (; idx < tok.size(); idx += 2) {
		if (tok[idx-1].size() != 5 || tok[idx].size() != 5)
			break;
		unsigned int x0(getint(tok[idx-1], 0, 2)), x1(getint(tok[idx-1], 2, 3));
		unsigned int y0(getint(tok[idx-0], 0, 3)), y1(getint(tok[idx-0], 3, 2));
		if (x0 != 00 && x0 != 11 && x0 != 22 && x0 != 33 && x0 != 44 &&
		    x0 != 55 && x0 != 66 && x0 != 77 && x0 != 88 && x0 != 99)
			break;
		uint16_t press(x1);
		if (press < 100)
			press += 1000;
		RadioSounding::Level& lvl(rs.insert_level(press * pressmul));
		if (y0 != ~0U) {
			int16_t temp(y0);
			if (temp & 1)
				temp = -temp;
			lvl.set_temp_degc10(temp);
			if (y1 != ~0U) {
				if (y1 > 50)
					temp -= (y1 - 50) * 10;
				else
					temp -= y1;
				lvl.set_dewpt_degc10(temp);
			}
		}
	}
	return newrs;
}

unsigned int RadioSoundings::parse_wmo(std::istream& is, time_t filedate)
{
	if (!is.good())
		return 0;
	std::vector<std::string> toks;
	typedef enum {
		mode_search,
		mode_ttaa,
		mode_ttbb,
		mode_ttcc,
		mode_ttdd
	} mode_t;
	mode_t mode(mode_search);
	int ch(is.get());
	unsigned int ret(0);
	while (is.good()) {
		std::string tok;
		while (ch != EOF && !std::isalnum(ch) && ch != '/' && ch != '=')
			ch = is.get();
		if (ch == EOF)
			break;
		tok.push_back(ch);
		if (ch == '=') {
			ch = is.get();
		} else {
			for (;;) {
				ch = is.get();
				if (std::isalnum(ch) || ch == '/') {
					tok.push_back(ch);
					continue;
				}
				if (ch == '=')
					break;
				ch = is.get();
				break;
			}
		}
		if (tok.empty())
			break;
		if (false)
			std::cerr << "Token: " << tok << std::endl;
		if (tok == "=") {
			if (false) {
				static const char * const modenames[] = { "?""?", "TTAA", "TTBB", "TTCC", "TTDD" };
				std::cerr << modenames[mode];
				for (std::vector<std::string>::const_iterator ti(toks.begin()), te(toks.end()); ti != te; ++ti)
					std::cerr << ' ' << *ti;
				std::cerr << std::endl;
			}
			switch (mode) {
			case mode_ttaa:
				ret += parse_ttaa(toks, filedate, 10);
				break;

			case mode_ttbb:
				ret += parse_ttbb(toks, filedate, 10);
				break;

			case mode_ttcc:
				ret += parse_ttaa(toks, filedate, 1);
				break;

			case mode_ttdd:
				ret += parse_ttbb(toks, filedate, 1);
				break;

			default:
				break;
			};
			mode = mode_search;
			toks.clear();
			continue;
		}
		if (mode != mode_search) {
			toks.push_back(tok);
			continue;
		}
		if (tok == "TTAA") {
			mode = mode_ttaa;
			continue;
		}
		if (tok == "TTBB") {
			mode = mode_ttbb;
			continue;
		}
		if (tok == "TTCC") {
			mode = mode_ttcc;
			continue;
		}
		if (tok == "TTDD") {
			mode = mode_ttdd;
			continue;
		}
	}
	return ret;
}

unsigned int RadioSoundings::parse_wmo(const std::string& fn, time_t filedate)
{
	if (fn.empty())
		return 0;
	std::ifstream is(fn.c_str());
	if (!is) {
		std::cerr << "Cannot open " << fn << std::endl;
		return 0;
	}
	return parse_wmo(is, filedate);
}

unsigned int RadioSoundings::parse_wmo(const std::string& dir, const std::string& fn)
{
	if (fn.size() != 16 || fn.compare(8, 8, "_upa.wmo"))
		return 0;
	static const unsigned int xmin[4] = { 0, 1, 0, 0 };
	static const unsigned int xmax[4] = { 99, 12, 31, 23 };
	unsigned int x[4];
	for (unsigned int i = 0; i < 4; ++i) {
		if (!std::isdigit(fn[2*i]) || !std::isdigit(fn[2*i+1]))
			return 0;
		x[i] = fn[2*i] * 10 + fn[2*i+1] - '0' * 11;
		if (x[i] < xmin[i] || x[i] > xmax[i])
			return 0;
	}

	struct tm tm;
	tm.tm_sec = 0;
	tm.tm_min = 0;
	tm.tm_hour = x[3];
	tm.tm_mday = x[2];
	tm.tm_mon = x[1] - 1;
	tm.tm_year = x[0] + 100;
	tm.tm_isdst = 0;
	time_t t(timegm(&tm));
	if (false)
		std::cerr << "parse: dir " << dir << " fn " << fn << " time " << t << std::endl;
	return parse_wmo(Glib::build_filename(dir, fn), t);
}

unsigned int RadioSoundings::parse_dir(const std::string& dir)
{
	if (false)
		std::cerr << "checking dir " << dir << std::endl;
	if (!Glib::file_test(dir, Glib::FILE_TEST_IS_DIR))
		return 0;
	Glib::Dir d(dir);
	unsigned int r(0);
	for (;;) {
		std::string f(d.read_name());
		if (f.empty())
			break;
		r += parse_wmo(dir, f);
		if (false)
			std::cerr << "file " << f << " nr " << r << std::endl;
	}
	return r;
}

unsigned int RadioSoundings::parse_dirs(const std::string& maindir, const std::string& auxdir)
{
	return parse_dir(Glib::build_filename(auxdir.empty() ? std::string(PACKAGE_DATA_DIR) : auxdir, "raob")) +
		parse_dir(Glib::build_filename(maindir.empty() ? (std::string)FPlan::get_userdbpath() : maindir, "raob"));
}

// Log P Skew T
// http://derecho.math.uwm.edu/classes/SynI/FedMetHandbooks/SkewTDocumentation.pdf
// http://www.atmos.washington.edu/~houze/301/Miscellaneous/Skew-T.pdf
// http://kiwi.atmos.colostate.edu/group/todd/Extras_files/Skew-T-Manual.pdf
// http://badpets.net/Documents/Atmos_Thermodynamics.pdf

const double SkewTChart::draw_mixr[] = {
	0.1, 0.4, 1, 2, 4, 7, 10, 15, 25
};

SkewTChart::SkewTChart(int32_t alt)
	: m_alt(alt)
{
}

void SkewTChart::draw(const Cairo::RefPtr<Cairo::Context>& cr, double width, double height, bool vlabels)
{
	cr->save();
	cr->rectangle(0, 0, width, height);
        cr->clip();
	drawterrain(cr, width, height);
	drawgrid(cr, width, height);
	drawstability(cr, width, height);
	if (m_alt != std::numeric_limits<int32_t>::min()) {
		float press(0);
		IcaoAtmosphere<float>::std_altitude_to_pressure(&press, 0, m_alt * Point::ft_to_m);
		std::pair<double,double> r(transform(press, 0));
		set_style_fplan(cr);
		cr->move_to(0, -r.second * height);
		cr->line_to(width, -r.second * height);
		cr->stroke();
	}
	drawtraces(cr, width, height);
	drawwindbarbs(cr, 0.9 * width, height);
	drawlegend(cr, width, height);
	cr->restore();
	cr->save();
	cr->rectangle(0, 0, width, height);
	cr->set_source_rgb(0, 0, 0);
	cr->set_line_width(1);
	cr->stroke();
	drawhlabels(cr, width, height);
	if (vlabels)
		drawvlabels(cr, width, height);
	cr->restore();
}

std::ostream& SkewTChart::draw(std::ostream& os, double width, double height, const std::string& boxname)
{
	// default: height=11.5, weight=14
	os << "  \\begin{center}\\begin{tikzpicture}" << std::endl;
	drawtikz(os, width, height, boxname);
	drawtikzhlabels(os, width, height);
	drawtikzvlabels(os, width, height);
	return os << "  \\end{tikzpicture}\\end{center}" << std::endl << std::endl;
}

std::ostream& SkewTChart::drawtikzstability(std::ostream& os, double width, double height)
{
	if (!get_stability().get_tempcurve().empty()) {
		const GRIB2::WeatherProfilePoint::Stability::tempcurve_t& tc(get_stability().get_tempcurve());
		GRIB2::WeatherProfilePoint::Stability::tempcurve_t::const_iterator ti(tc.begin()), te(tc.end());
		--te;
		while (ti != te) {
			GRIB2::WeatherProfilePoint::Stability::tempcurve_t::const_iterator ti1(ti);
			++ti1;
			float diff(0);
			while (ti1 != te && ti1->get_temp() != ti1->get_dewpt()) {
				diff += ti1->get_dewpt() - ti1->get_temp();
				++ti1;
			}
			if (diff > 0)
				os << "      \\draw[styskewtcape]";
			else
				os << "      \\draw[styskewtcin]";
			GRIB2::WeatherProfilePoint::Stability::tempcurve_t::const_iterator ti2(ti);
			for (;;) {
				if (ti2 != ti)
					os << " --";
				os << ' ' << pgfcoord(ti2->get_press(), ti2->get_temp() - IcaoAtmosphere<double>::degc_to_kelvin, width, height);
				if (ti2 == ti1)
					break;
				++ti2;
			}
			for (;;) {
				os << " -- " << pgfcoord(ti2->get_press(), ti2->get_dewpt() - IcaoAtmosphere<double>::degc_to_kelvin, width, height);
				if (ti2 == ti)
					break;
				--ti2;
			}
			os << ';' << std::endl;
			ti = ti1;
			if (!std::isnan(get_stability().get_el_press()) && ti->get_press() <= get_stability().get_el_press())
				break;
		}
	}
	return os;
}

std::ostream& SkewTChart::drawtikzgrid(std::ostream& os, double width, double height)
{
	// pressure gridlines
	for (unsigned int p = 100; p <= 1000; p += 50) {
		std::pair<double,double> r(transform(p, 0));
		os << "      \\draw[styskewtgridpress] (0," << (r.second * height) << ") -- (" << width << ',' << (r.second * height) << ");" << std::endl;
	}
	// temperature gridlines
	for (int t = -150; t <= 40; t += 10) {
		for (unsigned int p = 1000; p >= (100 - (100 >> 5)); p -= p >> 5)
			os << ((p == 1000) ? (t ? "      \\draw[styskewtgridtemp] " : "      \\draw[styskewtgridtemp0] ") : " -- ")
			   << pgfcoord(p, t, width, height);
		os << ';' << std::endl;
	}
	// mixing ratios
	for (unsigned int i = 0, n = sizeof(draw_mixr)/sizeof(draw_mixr[0]); i < n; ++i) {
		double w(draw_mixr[i]);
		for (unsigned int p = 1000; p >= (100 - (100 >> 5)); p -= p >> 5) {
			double tmr(mixing_ratio(w, p) - IcaoAtmosphere<double>::degc_to_kelvin);
			os << ((p == 1000) ? "      \\draw[styskewtgridmixr] " : " -- ") << pgfcoord(p, tmr, width, height);
		}
		os << ';' << std::endl;
	}
	// dry adiabats
	for (int t = -50; t <= 200; t += 10) {
		for (unsigned int p = 1000; p >= (100 - (100 >> 5)); p -= p >> 5) {
			double tda(dry_adiabat(t + IcaoAtmosphere<double>::degc_to_kelvin, p) - IcaoAtmosphere<double>::degc_to_kelvin);
			os << ((p == 1000) ? "      \\draw[styskewtgriddryad] " : " -- ") << pgfcoord(p, tda, width, height);
		}
		os << ';' << std::endl;
	}
	// saturation adiabats
	for (int t = -50; t <= 200; t += 10) {
		if (false) {
			double tsa(sat_adiabat(t + IcaoAtmosphere<double>::degc_to_kelvin, 100));
			double tda(dry_adiabat(t + IcaoAtmosphere<double>::degc_to_kelvin, 100));
			double td(tsa / tda * (t + IcaoAtmosphere<double>::degc_to_kelvin) - IcaoAtmosphere<double>::degc_to_kelvin);
			std::cerr << "saturation adiabat theta=" << t << " corresponds to dry adiabat theta=" << td
				  << " tsa=" << tsa << " tda=" << tda << std::endl;
		}
		for (unsigned int p = 1000; p >= (100 - (100 >> 5)); p -= p >> 5) {
			double tsa(sat_adiabat(t + IcaoAtmosphere<double>::degc_to_kelvin, p) - IcaoAtmosphere<double>::degc_to_kelvin);
			os << ((p == 1000) ? "      \\draw[styskewtgridsatad] " : " -- ") << pgfcoord(p, tsa, width, height);
			if (tsa < -45)
				break;
		}
		os << ';' << std::endl;
	}
	// ICAO atmosphere
	for (unsigned int p = 1000; p >= (100 - (100 >> 5)); p -= p >> 5) {
		float alt(0), temp(0);
		IcaoAtmosphere<float>::std_pressure_to_altitude(&alt, &temp, p);
		os << ((p == 1000) ? "    \\draw[styskewttempicao] " : " -- ")
		   << pgfcoord(p, temp - IcaoAtmosphere<double>::degc_to_kelvin, width, height);
	}
	return os << ';' << std::endl;
}

std::ostream& SkewTChart::drawtikz(std::ostream& os, double width, double height, const std::string& boxname)
{
	os << "    \\begin{scope}" << std::endl
	   << "      \\path[clip] (0,0) rectangle (" << width << ',' << height << ");" << std::endl;
	// ground
	drawtikzterrain(os, width, height);
	// gridlines
	if (boxname.empty())
		drawtikzgrid(os, width, height);
	else
		os << "      \\node[anchor=south west,inner sep=0pt,outer sep=0pt] at (0,0) {\\usebox{\\" << boxname << "}};" << std::endl;
	// stability
	drawtikzstability(os, width, height);
	// altitude
	if (m_alt != std::numeric_limits<int32_t>::min()) {
		float press(0);
		IcaoAtmosphere<float>::std_altitude_to_pressure(&press, 0, m_alt * Point::ft_to_m);
		std::pair<double,double> r(transform(press, 0));
		os << "      \\draw[colfplan,thin] (0," << (r.second * height) << ") -- (" << width << ',' << (r.second * height) << ");" << std::endl;
	}
	os << "    \\end{scope}" << std::endl;
	// frame
	os << "    \\draw[black,thin] (0,0) rectangle (" << width << ',' << height << ");" << std::endl;
	// curves
	os << "    \\begin{scope}" << std::endl
	   << "      \\path[clip] (0,0) rectangle (" << width << ',' << height << ");" << std::endl;
	// temperature & dewpoint curve
	drawtikztraces(os, width, height);
	os << "    \\end{scope}" << std::endl;
	// label
	drawtikzlegend(os, width, height);
	return os;
}

std::ostream& SkewTChart::prerender(std::ostream& os, const std::string& boxname, const std::string& opt, double width, double height)
{
	define_latex_styles(os << "  \\newsavebox{\\" << boxname << "}" << std::endl
			    << "    \\sbox{\\" << boxname << "}{\\begin{tikzpicture}[" << opt << (opt.empty() ? "" : ",")) << ']' << std::endl
			    << "      \\path[clip] (0,0) rectangle (" << width << ',' << height << ");" << std::endl;
	// gridlines
	drawtikzgrid(os, width, height);
	return os << "    \\end{tikzpicture}}" << std::endl;
}

std::ostream& SkewTChart::drawtikzhlabels(std::ostream& os, double width, double height)
{
	for (int t = -50; t <= 40; t += 10) {
		std::pair<double,double> r(transform(1000, t));
		std::ostringstream oss;
		if (t < 0)
			oss << "--";
		oss << abs(t);
		os << "    \\node[anchor=north,align=center] at (" << (r.first * width) << ",0) {" << oss.str() << "};" << std::endl;
	}
	return os;
}

std::ostream& SkewTChart::drawtikzvlabels(std::ostream& os, double width, double height, bool right)
{
	for (unsigned int p = 100; p <= 1000; p += 100) {
		float alt(0), temp(0);
		IcaoAtmosphere<float>::std_pressure_to_altitude(&alt, &temp, p);
		alt *= Point::m_to_ft * 0.01;
		std::pair<double,double> r(transform(p, 0));
		std::ostringstream oss;
		oss << 'F' << std::setfill('0') << std::setw(3) << Point::round<int,float>(alt) << ' ' << p;
		if (right)
			os << "    \\node[anchor=west,align=left] at (" << width << ',';
		else
			os << "    \\node[anchor=east,align=right] at (0,";
		os  << (r.second * height) << ") {" << oss.str() << "};" << std::endl;
	}
	return os;
}

void SkewTChart::drawstability(const Cairo::RefPtr<Cairo::Context>& cr, double width, double height)
{
	if (get_stability().get_tempcurve().empty())
		return;
	const GRIB2::WeatherProfilePoint::Stability::tempcurve_t& tc(get_stability().get_tempcurve());
	if (false)
		for (GRIB2::WeatherProfilePoint::Stability::tempcurve_t::const_iterator ti(tc.begin()), te(tc.end()); ti != te; ++ti)
			std::cerr << "drawstability: press " << ti->get_press()
				  << " temp " << ti->get_temp() << ' ' << ti->get_dewpt() << std::endl;
	set_style_liftcurve(cr);
	for (GRIB2::WeatherProfilePoint::Stability::tempcurve_t::const_iterator ti(tc.begin()), te(tc.end()); ti != te; ++ti)
		cairopath(cr, ti == tc.begin(), ti->get_press(), ti->get_dewpt() - IcaoAtmosphere<double>::degc_to_kelvin, width, height);
	cr->stroke();
	if (std::isnan(get_stability().get_el_press()))
		return;
	GRIB2::WeatherProfilePoint::Stability::tempcurve_t::const_iterator ti(tc.begin()), te(tc.end());
	--te;
	while (ti != te) {
		GRIB2::WeatherProfilePoint::Stability::tempcurve_t::const_iterator ti1(ti);
		++ti1;
		float diff(0);
		while (ti1 != te && ti1->get_temp() != ti1->get_dewpt()) {
			diff += ti1->get_dewpt() - ti1->get_temp();
			++ti1;
		}
		if (diff > 0)
			set_color_cape(cr);
		else
			set_color_cin(cr);
		GRIB2::WeatherProfilePoint::Stability::tempcurve_t::const_iterator ti2(ti);
		for (;;) {
			cairopath(cr, ti2 == ti, ti2->get_press(), ti2->get_temp() - IcaoAtmosphere<double>::degc_to_kelvin, width, height);
			if (ti2 == ti1)
				break;
			++ti2;
		}
		for (;;) {
			cairopath(cr, false, ti2->get_press(), ti2->get_dewpt() - IcaoAtmosphere<double>::degc_to_kelvin, width, height);
			if (ti2 == ti)
				break;
			--ti2;
		}
		cr->close_path();
		cr->fill();
		ti = ti1;
		if (ti->get_press() <= get_stability().get_el_press())
			break;
	}
}

void SkewTChart::drawgrid(const Cairo::RefPtr<Cairo::Context>& cr, double width, double height)
{
	// pressure gridlines
	set_style_press(cr);
	for (unsigned int p = 100; p <= 1000; p += 50) {
		std::pair<double,double> r(transform(p, 0));
		cr->move_to(0, height - r.second * height);
		cr->line_to(width, height - r.second * height);
		cr->stroke();
	}
	// temperature gridlines
	set_style_temp(cr, false);
	for (int t = -150; t <= 40; t += 10) {
		if (!t)
			set_style_temp(cr, true);
		for (unsigned int p = 1000; p >= (100 - (100 >> 5)); p -= p >> 5)
			cairopath(cr, p == 1000, p, t, width, height);
		cr->stroke();
		if (!t)
			set_style_temp(cr, false);
	}
	// mixing ratios
	set_style_mixr(cr);
	for (unsigned int i = 0, n = sizeof(draw_mixr)/sizeof(draw_mixr[0]); i < n; ++i) {
		double w(draw_mixr[i]);
		for (unsigned int p = 1000; p >= (100 - (100 >> 5)); p -= p >> 5) {
			double tmr(mixing_ratio(w, p) - IcaoAtmosphere<double>::degc_to_kelvin);
		        cairopath(cr, p == 1000, p, tmr, width, height);
		}
		cr->stroke();
	}
	// dry adiabats
	set_style_dryad(cr);
	for (int t = -50; t <= 200; t += 10) {
		for (unsigned int p = 1000; p >= (100 - (100 >> 5)); p -= p >> 5) {
			double tda(dry_adiabat(t + IcaoAtmosphere<double>::degc_to_kelvin, p) - IcaoAtmosphere<double>::degc_to_kelvin);
		        cairopath(cr, p == 1000, p, tda, width, height);
		}
		cr->stroke();
	}
	// saturation adiabats
	set_style_satad(cr);
	for (int t = -50; t <= 200; t += 10) {
		if (false) {
			double tsa(sat_adiabat(t + IcaoAtmosphere<double>::degc_to_kelvin, 100));
			double tda(dry_adiabat(t + IcaoAtmosphere<double>::degc_to_kelvin, 100));
			double td(tsa / tda * (t + IcaoAtmosphere<double>::degc_to_kelvin) - IcaoAtmosphere<double>::degc_to_kelvin);
			std::cerr << "saturation adiabat theta=" << t << " corresponds to dry adiabat theta=" << td
				  << " tsa=" << tsa << " tda=" << tda << std::endl;
		}
		for (unsigned int p = 1000; p >= (100 - (100 >> 5)); p -= p >> 5) {
			double tsa(sat_adiabat(t + IcaoAtmosphere<double>::degc_to_kelvin, p) - IcaoAtmosphere<double>::degc_to_kelvin);
			cairopath(cr, p == 1000, p, tsa, width, height);
			if (tsa < -45)
				break;
		}
		cr->stroke();
	}
	// ICAO atmosphere
	set_style_tempicao(cr);
	for (unsigned int p = 1000; p >= (100 - (100 >> 5)); p -= p >> 5) {
		float alt(0), temp(0);
		IcaoAtmosphere<float>::std_pressure_to_altitude(&alt, &temp, p);
		cairopath(cr, p == 1000, p, temp - IcaoAtmosphere<double>::degc_to_kelvin, width, height);
	}
	cr->stroke();
}

void SkewTChart::drawhlabels(const Cairo::RefPtr<Cairo::Context>& cr, double width, double height)
{
	cr->save();
	cr->select_font_face("sans", Cairo::FONT_SLANT_NORMAL, Cairo::FONT_WEIGHT_NORMAL);
	cr->set_font_size(8);
	for (int t = -50; t <= 40; t += 10) {
		std::pair<double,double> r(transform(1000, t));
		std::ostringstream oss;
		if (t < 0)
			oss << "\342\210\222";
		oss << abs(t);
		Cairo::TextExtents ext;
		cr->get_text_extents(oss.str(), ext);
		cr->move_to(r.first * width - 0.5 * ext.width - ext.x_bearing,
			    height - ext.y_bearing + 0.4 * ext.height);
		cr->show_text(oss.str());
	}
	cr->restore();
}

void SkewTChart::drawvlabels(const Cairo::RefPtr<Cairo::Context>& cr, double width, double height)
{
	cr->save();
	cr->select_font_face("sans", Cairo::FONT_SLANT_NORMAL, Cairo::FONT_WEIGHT_NORMAL);
	cr->set_font_size(8);
	Cairo::TextExtents ext;
	cr->get_text_extents("F888 8888", ext);
	double totw(ext.width);
	for (unsigned int p = 100; p <= 1000; p += 100) {
		float alt(0), temp(0);
		IcaoAtmosphere<float>::std_pressure_to_altitude(&alt, &temp, p);
		alt *= Point::m_to_ft * 0.01;
		std::pair<double,double> r(transform(p, 0));
		std::ostringstream ossl, ossr;
		ossl << 'F' << std::setfill('0') << std::setw(3) << Point::round<int,float>(alt);
		ossr << p;
		cr->get_text_extents(ossl.str(), ext);
		cr->move_to(width - ext.x_bearing + 2,
			    height - r.second * height - 0.5 * ext.height - ext.y_bearing);
		cr->show_text(ossl.str());
		cr->get_text_extents(ossr.str(), ext);
		cr->move_to(width + totw - ext.width - ext.x_bearing + 2,
			    height - r.second * height - 0.5 * ext.height - ext.y_bearing);
		cr->show_text(ossr.str());
	}
	cr->restore();
}

std::ostream& SkewTChart::draw_windbarbuv(std::ostream& os, float wu, float wv, unsigned int mask)
{
	wu *= (-1e-3f * Point::km_to_nmi * 3600);
	wv *= (-1e-3f * Point::km_to_nmi * 3600);
	float wdirdeg(atan2f(wu, wv) * (180.0 / M_PI)), wspeedkts(sqrtf(wu * wu + wv * wv));
	return draw_windbarb(os, wdirdeg, wspeedkts, mask);
}

std::ostream& SkewTChart::draw_windbarb(std::ostream& os, float wdirdeg, float wspeedkts, unsigned int mask)
{
	static const double windradius = 1.5;
	int windspeed5(Point::round<int,float>(wspeedkts * 0.2f));
	if (mask & 1)
		os << "    \\draw[white,ultra thick] (0,0) arc[x radius=0.1,y radius=0.1,start angle=0,delta angle=360] -- cycle;" << std::endl;
	if (windspeed5) {
		os << "    \\begin{scope}[rotate=" << Point::round<int,float>(-wdirdeg) << ']' << std::endl;
		// draw twice, first white with bigger line width
		for (unsigned int i(0); i < 2; ++i) {
			if (!(mask & (1U << i)))
				continue;
			double y(windradius);
			os << "      \\draw[" << (i ? "black,thin" : "white,ultra thick")
			   << "] (0,0) -- (0," << y << ");" << std::endl;
			int speed(windspeed5);
			if (speed < 10)
				y += 0.2;
			while (speed >= 10) {
				speed -= 10;
				os << "      \\" << (i ? "fill[black,thin" : "draw[white,ultra thick")
				   << "] (0," << y << ") -- ++(0.4,-0.2) -- ++(-0.4,-0.2) -- cycle;" << std::endl;
				y -= 0.4;
			}
			while (speed >= 2) {
				speed -= 2;
				y -= 0.2;
				os << "      \\draw[" << (i ? "black,thin" : "white,ultra thick")
				   << "] (0," << y << ") -- ++(0.4,0.2);" << std::endl;
			}
			if (speed) {
				y -= 0.2;
				os << "      \\draw[" << (i ? "black,thin" : "white,ultra thick")
				   << "] (0," << y << ") -- ++(0.2,0.1);" << std::endl;
			}
		}
		os << "    \\end{scope}" << std::endl;
	}
	if (mask & 2)
		os << "    \\fill[black] (0,0) arc[x radius=0.1,y radius=0.1,start angle=0,delta angle=360] -- cycle;" << std::endl;
	return os;
}

void SkewTChart::draw_windbarbuv(const Cairo::RefPtr<Cairo::Context>& cr, float wu, float wv, unsigned int mask)
{
	wu *= (-1e-3f * Point::km_to_nmi * 3600);
	wv *= (-1e-3f * Point::km_to_nmi * 3600);
	float wdirdeg(atan2f(wu, wv) * (180.0 / M_PI)), wspeedkts(sqrtf(wu * wu + wv * wv));
	draw_windbarb(cr, wdirdeg, wspeedkts, mask);
}

void SkewTChart::draw_windbarb(const Cairo::RefPtr<Cairo::Context>& cr, float wdirdeg, float wspeedkts, unsigned int mask)
{
	static const double windradius = 1.5;
	int windspeed5(Point::round<int,float>(wspeedkts * 0.2f));
	if (mask & 1) {
		cr->begin_new_path();
		cr->arc(0.0, 0.0, 0.1, 0.0, 2.0 * M_PI);
		cr->close_path();
		cr->set_source_rgb(1, 1, 1);
		cr->set_line_width(0.30);
		cr->stroke();
	}
	if (windspeed5) {
		cr->save();
		cr->rotate(wdirdeg * (M_PI / 180));
		// draw twice, first white with bigger line width
		for (unsigned int i(0); i < 2; ++i) {
			if (!(mask & (1U << i)))
				continue;
			double y(-windradius);
			if (i) {
				cr->set_source_rgb(0, 0, 0);
				cr->set_line_width(0.10);
			} else {
				cr->set_source_rgb(1, 1, 1);
				cr->set_line_width(0.30);
			}
			cr->move_to(0, 0);
			cr->line_to(0, y);
			cr->stroke();
			int speed(windspeed5);
			if (speed < 10)
				y -= 0.2;
			while (speed >= 10) {
				speed -= 10;
				cr->move_to(0, y);
				cr->rel_line_to(0.4, 0.2);
				cr->rel_line_to(-0.4, 0.2);
				cr->close_path();
				if (i)
					cr->fill();
				else
					cr->stroke();
				y += 0.4;
			}
			while (speed >= 2) {
				speed -= 2;
				y += 0.2;
				cr->move_to(0, y);
				cr->rel_line_to(0.4, -0.2);
				cr->stroke();
			}
			if (speed) {
				y += 0.2;
				cr->move_to(0, y);
				cr->rel_line_to(0.2, -0.1);
				cr->stroke();
			}
		}
		cr->restore();
	}
	if (mask & 2) {
		cr->begin_new_path();
		cr->arc(0.0, 0.0, 0.1, 0.0, 2.0 * M_PI);
		cr->close_path();
		cr->set_source_rgb(0, 0, 0);
		cr->set_line_width(0.30);
		cr->fill();
	}
}

std::ostream& SkewTChart::define_latex_colors(std::ostream& os)
{
	static const char coldef[] =
		"\\definecolor{colskewtgnd}{rgb}{0.77278,0.73752,0.71270}%\n"
		"\\definecolor{colskewtgridpress}{rgb}{0,0,1}%\n"
		"\\definecolor{colskewtgridtemp}{rgb}{1,0,0}%\n"
		"\\definecolor{colskewtgriddryad}{rgb}{0,1,0}%\n"
		"\\definecolor{colskewtgridsatad}{rgb}{0,1,0}%\n"
		"\\definecolor{colskewtgridmixr}{rgb}{0,1,1}%\n"
		"\\definecolor{colskewttempicao}{rgb}{1,0,0}%\n"
		"\\definecolor{colskewttracetemp}{rgb}{0,0,0}%\n"
		"\\definecolor{colskewttracedewpt}{rgb}{0,0,0}%\n"
		"\\definecolor{colskewtcape}{rgb}{1.0,0.9,0.8}%\n"
		"\\definecolor{colskewtcin}{rgb}{0.8,0.92,0.82}%\n"
		"\\pgfdeclarelayer{skewtwindbarbs}%\n"
		"\\pgfdeclarelayer{skewtwindbarbhalo}%\n"
		"\\pgfsetlayers{main,skewtwindbarbhalo,skewtwindbarbs}%\n";
	return os << coldef;
}

std::ostream& SkewTChart::define_latex_styles(std::ostream& os)
{
	static const char stydef[] =
		"styskewtgridpress/.style={colskewtgridpress,thin},"
		"styskewtgridtemp/.style={colskewtgridtemp,thin},"
		"styskewtgridtemp0/.style={colskewtgridtemp,very thick},"
		"styskewtgriddryad/.style={colskewtgriddryad,thin},"
		"styskewtgridsatad/.style={colskewtgridsatad,thin,dashed},"
		"styskewtgridmixr/.style={colskewtgridmixr,thin},"
		"styskewttempicao/.style={colskewttempicao,thick},"
		"styskewtgnd/.style={colskewtgnd,ultra thick},"
		"styskewttracetemp/.style={colskewttracetemp,thick},"
		"styskewttracedewpt/.style={colskewttracedewpt,thick},"
		"styskewttracesectemp/.style={colskewttracetemp,thick,dashed},"
		"styskewttracesecdewpt/.style={colskewttracedewpt,thick,dashed},"
		"styskewtcape/.style={colskewtcape},"
		"styskewtcin/.style={colskewtcin}";
	return os << stydef;
}

std::ostream& SkewTChart::draw_legend(std::ostream& os)
{
	os << "\\begin{tabular}{ll}\n";
   	define_latex_styles(os << "  \\begin{tikzpicture}[") << "]\\path[clip] (0,0) rectangle (1,0.2); \\draw[styskewtgridpress] (0,0.1) -- (1,0.1);\\end{tikzpicture} & Pressure Gridlines \\\\\n";
	define_latex_styles(os << "  \\begin{tikzpicture}[") << "]\\path[clip] (0,0) rectangle (1,0.2); \\draw[styskewtgridtemp] (0,0.1) -- (1,0.1);\\end{tikzpicture} & Temperature Gridlines \\\\\n";
	define_latex_styles(os << "  \\begin{tikzpicture}[") << "]\\path[clip] (0,0) rectangle (1,0.2); \\draw[styskewtgriddryad] (0,0.1) -- (1,0.1);\\end{tikzpicture} & Dry Adiabats \\\\\n";
	define_latex_styles(os << "  \\begin{tikzpicture}[") << "]\\path[clip] (0,0) rectangle (1,0.2); \\draw[styskewtgridsatad] (0,0.1) -- (1,0.1);\\end{tikzpicture} & Saturated Adiabats \\\\\n";
	define_latex_styles(os << "  \\begin{tikzpicture}[") << "]\\path[clip] (0,0) rectangle (1,0.2); \\draw[styskewtgridmixr] (0,0.1) -- (1,0.1);\\end{tikzpicture} & Saturated Mixing Ratio \\\\\n";
	define_latex_styles(os << "  \\begin{tikzpicture}[") << "]\\path[clip] (0,0) rectangle (1,0.2); \\draw[styskewttempicao] (0,0.1) -- (1,0.1);\\end{tikzpicture} & ICAO Standard Atmosphere \\\\\n";
	define_latex_styles(os << "  \\begin{tikzpicture}[") << "]\\path[clip] (0,0) rectangle (1,0.2); \\draw[colskewtgnd,ultra thick] (0,0.1) -- (1,0.1);\\end{tikzpicture} & Terrain \\\\\n";
	define_latex_styles(os << "  \\begin{tikzpicture}[") << "]\\path[clip] (0,0) rectangle (1,0.2); \\draw[styskewttracetemp] (0,0.1) -- (1,0.1);\\end{tikzpicture} & Temperature Trace \\\\\n";
	define_latex_styles(os << "  \\begin{tikzpicture}[") << "]\\path[clip] (0,0) rectangle (1,0.2); \\draw[styskewttracedewpt] (0,0.1) -- (1,0.1);\\end{tikzpicture} & Dewpoint Trace \\\\\n";
	return os << "\\end{tabular}\n";
}

double SkewTChart::dry_adiabat(double t_kelvin, double p_mbar)
{
	return t_kelvin * pow(p_mbar * 1e-3, 0.288);
}

double SkewTChart::sat_adiabat(double t_kelvin, double p_mbar)
{
	double t(253.16);
	for (int i = 1; i <= 12; ++i) {
		double x(t_kelvin * exp(-2.6518986 * wfunc(t, p_mbar) / t) - t * pow(1e3 / p_mbar, 0.286));
		if (fabs(x) < 0.01)
			break;
		t += ldexp(120, -i) * sign(x);
	}
	return t;
}

double SkewTChart::mixing_ratio(double w, double p_mbar)
{
	static const double a = 0.0498646455;
	static const double b = 2.4082965;
	static const double c = 280.23475 - IcaoAtmosphere<double>::degc_to_kelvin;
	static const double d = 38.9114;
	static const double f = 0.0915;
	static const double g = -1.2035;

	double log10m(log10(w * p_mbar / (622 + w)));
	double x(pow(10, f * log10m) + g);
	return pow(10, a * log10m + b) - c + d * x * x;
}

double SkewTChart::sign(double x)
{
	if (std::isnan(x))
		return x;
	if (x < 0)
		return -1;
	if (x > 0)
		return 1;
	return 0;
}

double SkewTChart::esat(double t_kelvin)
{
	// http://www.met.wau.nl/metlukweb/Reading/Clausius-Clapeyron.pdf
	// Teten's Formula
	// returns water saturation pressure in mb
	double tc(t_kelvin - IcaoAtmosphere<double>::degc_to_kelvin);
	return 6.1078 * exp((17.2693882 * tc) / (tc + 237.3));
}

double SkewTChart::wfunc(double t_kelvin, double p_mbar)
{
	// return value in g water vapor / kg dry air
	if (t_kelvin > 999.)
		return 0;
	double es(esat(t_kelvin));
	return 621.97 * es / (p_mbar - es);
}

std::pair<double,double> SkewTChart::transform(double p_mbar, double t_degc)
{
	// chart range: -55 <= t_degc <= 45
	double logp(log10(p_mbar));
	double t(t_degc + 55);
	std::pair<double,double> r;
	r.first = (0.1408 * t - 10.53975 * logp + 31.61923) * (1.0 / 14.08);
	r.second = (-11.5 * logp + 34.5) * (1.0 / 11.5);
	return r;
}

std::string SkewTChart::pgfcoord(std::pair<double,double> r, double width, double height)
{
	std::ostringstream oss;
	oss << '(' << std::fixed << (r.first * width) << ',' << std::fixed << (r.second * height) << ')';
	return oss.str();
}

void SkewTChart::cairopath(const Cairo::RefPtr<Cairo::Context>& cr, bool move, std::pair<double,double> r, double width, double height)
{
	if (move)
		cr->move_to(r.first * width, height - r.second * height);
	else
		cr->line_to(r.first * width, height - r.second * height);
}

double SkewTChart::refractivity(double press_mb, double temp_kelvin, double dewpt_kelvin)
{
	static const double a = 77.6, b = 3.73e5;
	// http://www.mike-willis.com/Tutorial/PF6.htm
	double inv_temp_k(1.0 / temp_kelvin);
	return a * press_mb * inv_temp_k + b * esat(dewpt_kelvin) * inv_temp_k * inv_temp_k;
}

double SkewTChart::refractivity(const GRIB2::WeatherProfilePoint& wpp, unsigned int i)
{
	if (i >= sizeof(GRIB2::WeatherProfilePoint::isobaric_levels)/sizeof(GRIB2::WeatherProfilePoint::isobaric_levels[0]))
		return 0;
	if (GRIB2::WeatherProfilePoint::isobaric_levels[i] <= 0)
		return 0;
	const GRIB2::WeatherProfilePoint::Surface& sfc(wpp[i]);
	return refractivity(GRIB2::WeatherProfilePoint::isobaric_levels[i], sfc.get_temp(), sfc.get_dewpoint());
}

double SkewTChart::refractivity(const RadioSounding::Level& lvl)
{
	return refractivity(lvl.get_press_dpa() * 0.1,
			    lvl.get_temp_degc10() * 0.1 + IcaoAtmosphere<double>::degc_to_kelvin,
			    lvl.get_dewpt_degc10() * 0.1 + IcaoAtmosphere<double>::degc_to_kelvin);
}

SkewTChartPrognostic::SkewTChartPrognostic(const Point& pt, gint64 efftime, const std::string& name, int32_t alt, TopoDb30::elev_t elev, GRIB2 *wxdb)
	: SkewTChart(alt), m_pt(pt), m_efftime(efftime), m_name(name), m_elev(elev)
{
	if (!wxdb)
		return;
	if (pt.is_invalid())
		return;
	FPlanRoute fpl(*(FPlan *)0);
	{
		FPlanWaypoint wpt;
		wpt.set_coord(pt);
		wpt.set_time_unix(efftime);
		wpt.set_altitude(alt);
		fpl.insert_wpt(~0, wpt);
		wpt.set_flighttime(600);
		wpt.set_time_unix(efftime + wpt.get_flighttime());
		fpl.insert_wpt(~0, wpt);
	}
	GRIB2::WeatherProfile p(wxdb->get_profile(fpl));
	if (false)
		std::cerr << "SkewTChart: " << pt.get_fpl_str() << ' ' << name << " alt " << alt << " elev " << elev
			  << ' ' << Glib::TimeVal(efftime, 0).as_iso8601() << " profile " << p.size() << std::endl;
	if (p.empty())
		return;
	m_wpp = p.front();
}

void SkewTChartPrognostic::calc_stability(void)
{
	m_stability = GRIB2::WeatherProfilePoint::Stability(m_wpp);
}

std::ostream& SkewTChartPrognostic::drawtikztraces(std::ostream& os, double width, double height)
{
	// temperature curve
	static const unsigned int nrsfc(sizeof(GRIB2::WeatherProfilePoint::isobaric_levels)/
					sizeof(GRIB2::WeatherProfilePoint::isobaric_levels[0]));
	{
		bool subseq(false);
		for (unsigned int i = 1; i < nrsfc; ++i) {
			if (std::isnan(m_wpp[i].get_temp()))
				continue;
			os << (subseq ? " -- " : "    \\draw[styskewttracetemp] ")
			   << pgfcoord(GRIB2::WeatherProfilePoint::isobaric_levels[i],
				       m_wpp[i].get_temp() - IcaoAtmosphere<double>::degc_to_kelvin, width, height);
			subseq = true;
		}
		if (subseq)
			os << ';' << std::endl;
		else
			return os;
	}
	// dewpoint curve
	{
		bool subseq(false);
		for (unsigned int i = 1; i < nrsfc; ++i) {
			if (std::isnan(m_wpp[i].get_dewpoint()))
				continue;
			os << (subseq ? " -- " : "    \\draw[styskewttracedewpt] ")
			   << pgfcoord(GRIB2::WeatherProfilePoint::isobaric_levels[i],
				       m_wpp[i].get_dewpoint() - IcaoAtmosphere<double>::degc_to_kelvin, width, height);
			subseq = true;
		}
		if (subseq)
			os << ';' << std::endl;
	}
	return os;
}

std::ostream& SkewTChartPrognostic::drawtikzwindbarbs(std::ostream& os, double xorigin, double height, double xorigin2)
{
	if (std::isnan(xorigin))
		return os;
	static const unsigned int nrsfc(sizeof(GRIB2::WeatherProfilePoint::isobaric_levels)/
					sizeof(GRIB2::WeatherProfilePoint::isobaric_levels[0]));
	for (unsigned int m = 0; m < 2; ++m) {
		for (unsigned int i = 1; i < nrsfc; ++i) {
			float wu(m_wpp[i].get_uwind()), wv(m_wpp[i].get_vwind());
			if (std::isnan(wu) || std::isnan(wv))
				continue;
			double y(transform(GRIB2::WeatherProfilePoint::isobaric_levels[i], 0).second);
			if (y > 1)
				continue;
			os << "  \\begin{scope}[shift={(" << xorigin << ',' << (y * height)
			   << ")},scale=0.3]" << std::endl;
			draw_windbarbuv(os, wu, wv, 1U << m);
			os << "  \\end{scope}" << std::endl;
		}
	}
	return os;
}

std::ostream& SkewTChartPrognostic::drawtikzterrain(std::ostream& os, double width, double height)
{
	if (m_elev == TopoDb30::nodata)
		return os;
	float press(0);
	IcaoAtmosphere<float>::std_altitude_to_pressure(&press, 0, m_elev);
	std::pair<double,double> r(transform(press, 0));
	return os << "      \\fill[colskewtgnd] (0,0) rectangle (" << width << ',' << (r.second * height) << ");" << std::endl;
}

std::ostream& SkewTChartPrognostic::drawtikzlegend(std::ostream& os, double width, double height)
{
	std::ostringstream oss;
	oss << "\\contour{white}{";
	if (m_name.empty())
		oss << m_pt.get_fpl_str();
	else
		oss << METARTAFChart::latex_string_meta(m_name) << " (" << m_pt.get_fpl_str() << ')';
	{
		std::string tm(Glib::TimeVal(m_efftime, 0).as_iso8601());
		if (tm.size() > 10 && tm[10] == 'T')
			tm[10] = ' ';
		oss << "}\\\\ \\contour{white}{" << tm << '}';
	}
	for (int i = 0; i < 6; ++i) {
		std::ostringstream oss1;
		switch (i) {
		case 0:
			if (get_stability().is_lcl()) {
				float alt(0);
				IcaoAtmosphere<float>::std_pressure_to_altitude(&alt, 0, get_stability().get_lcl_press());
				oss << "LCL F" << std::setw(3) << std::setfill('0') << Point::round<int,float>(alt * (Point::m_to_ft * 0.01f))
				    << ' ' << std::fixed << std::setprecision(1)
				    << (get_stability().get_lcl_temp() - IcaoAtmosphere<double>::degc_to_kelvin)
				    << "$^\\circ$C";
			}
			break;

		case 1:
			if (get_stability().is_lfc()) {
				float alt(0);
				IcaoAtmosphere<float>::std_pressure_to_altitude(&alt, 0, get_stability().get_lfc_press());
				oss1 << "LFC F" << std::setw(3) << std::setfill('0') << Point::round<int,float>(alt * (Point::m_to_ft * 0.01f))
				    << ' ' << std::fixed << std::setprecision(1)
				    << (get_stability().get_lfc_temp() - IcaoAtmosphere<double>::degc_to_kelvin)
				    << "$^\\circ$C";
			}
			break;

		case 2:
			if (get_stability().is_el()) {
				float alt(0);
				IcaoAtmosphere<float>::std_pressure_to_altitude(&alt, 0, get_stability().get_el_press());
				oss1 << "EL F" << std::setw(3) << std::setfill('0') << Point::round<int,float>(alt * (Point::m_to_ft * 0.01f))
				    << ' ' << std::fixed << std::setprecision(1)
				    << (get_stability().get_el_temp() - IcaoAtmosphere<double>::degc_to_kelvin)
				    << "$^\\circ$C";
			}
			break;

		case 3:
			if (std::isnan(get_stability().get_liftedindex()))
				break;
			oss1 << "LI " << std::fixed << std::setprecision(1) << get_stability().get_liftedindex() << "$^\\circ$C";
			break;

		case 4:
			if (std::isnan(get_stability().get_cape()))
				break;
			oss1 << "CAPE " << std::fixed << std::setprecision(0) << get_stability().get_cape() << "J/kg";
			break;

		case 5:
			if (std::isnan(get_stability().get_cin()))
				break;
			oss1 << "CIN " << std::fixed << std::setprecision(0) << get_stability().get_cin() << "J/kg";
			break;

		default:
			break;
		}
		if (oss1.str().empty())
			continue;
		std::string txt(oss.str());
		for (Glib::ustring::size_type i(0), n(txt.size()); i < n; ++i)
			if (txt[i] == '-') {
				txt.replace(i, 1, "$-$");
				i += 2;
				n = txt.size();
			}
		oss << "\\\\ \\contour{white}{" << METARTAFChart::latex_string_meta(txt) << '}';
	}
	return os << "    \\node[anchor=north west,align=left] at (0," << height << ") {" << oss.str() << "};" << std::endl;
}

void SkewTChartPrognostic::drawtraces(const Cairo::RefPtr<Cairo::Context>& cr, double width, double height)
{
	// temperature curve
	static const unsigned int nrsfc(sizeof(GRIB2::WeatherProfilePoint::isobaric_levels)/
					sizeof(GRIB2::WeatherProfilePoint::isobaric_levels[0]));
	set_style_tracetemp(cr);
	{
		bool subseq(false);
		for (unsigned int i = 1; i < nrsfc; ++i) {
			if (std::isnan(m_wpp[i].get_temp()))
				continue;
			cairopath(cr, !subseq, GRIB2::WeatherProfilePoint::isobaric_levels[i],
				  m_wpp[i].get_temp() - IcaoAtmosphere<double>::degc_to_kelvin, width, height);
			subseq = true;
		}
		if (subseq)
			cr->stroke();
		else
			return;
	}
	// dewpoint curve
	set_style_tracedewpt(cr);
	{
		bool subseq(false);
		for (unsigned int i = 1; i < nrsfc; ++i) {
			if (std::isnan(m_wpp[i].get_dewpoint()))
				continue;
			cairopath(cr, !subseq, GRIB2::WeatherProfilePoint::isobaric_levels[i],
				  m_wpp[i].get_dewpoint() - IcaoAtmosphere<double>::degc_to_kelvin, width, height);
			subseq = true;
		}
		if (subseq)
		        cr->stroke();
	}
}

void SkewTChartPrognostic::drawterrain(const Cairo::RefPtr<Cairo::Context>& cr, double width, double height)
{
	if (m_elev == TopoDb30::nodata)
		return;
	float press(0);
	IcaoAtmosphere<float>::std_altitude_to_pressure(&press, 0, m_elev);
	std::pair<double,double> r(transform(press, 0));
	set_color_gnd(cr);
	cr->rectangle(0, height, width, -r.second * height);
	cr->fill();
}

void SkewTChartPrognostic::drawlegend(const Cairo::RefPtr<Cairo::Context>& cr, double width, double height)
{
	cr->select_font_face("sans", Cairo::FONT_SLANT_NORMAL, Cairo::FONT_WEIGHT_NORMAL);
	cr->set_font_size(8);
	Cairo::TextExtents ext;
	cr->get_text_extents("M", ext);
	double yadvance(1.4 * ext.height);
	double x(3), y(3);
	std::string txt(m_pt.get_fpl_str());
	if (!m_name.empty())
		txt = m_name + " (" + txt + ")";
	cr->get_text_extents(txt, ext);
	{
		double xc(x - ext.x_bearing), yc(y - ext.y_bearing);
		cr->set_source_rgb(1.0, 1.0, 1.0);
		cr->move_to(xc, yc + 1);
		cr->show_text(txt);
		cr->move_to(xc, yc - 1);
		cr->show_text(txt);
		cr->move_to(xc + 1, yc);
		cr->show_text(txt);
		cr->move_to(xc - 1, yc);
		cr->show_text(txt);
		cr->set_source_rgb(0.0, 0.0, 0.0);
		cr->move_to(xc, yc);
		cr->show_text(txt);
	}
	y += yadvance;
	txt = Glib::TimeVal(m_efftime, 0).as_iso8601();
	if (txt.size() > 10 && txt[10] == 'T')
		txt[10] = ' ';
	cr->get_text_extents(txt, ext);
	{
		double xc(x - ext.x_bearing), yc(y - ext.y_bearing);
		cr->set_source_rgb(1.0, 1.0, 1.0);
		cr->move_to(xc, yc + 1);
		cr->show_text(txt);
		cr->move_to(xc, yc - 1);
		cr->show_text(txt);
		cr->move_to(xc + 1, yc);
		cr->show_text(txt);
		cr->move_to(xc - 1, yc);
		cr->show_text(txt);
		cr->set_source_rgb(0.0, 0.0, 0.0);
		cr->move_to(xc, yc);
		cr->show_text(txt);
	}
	y += yadvance;
	cr->set_font_size(6);
	cr->get_text_extents("M", ext);
	yadvance = 1.4 * ext.height;
	y += yadvance;
	for (int i = 0; i < 6; ++i) {
		std::ostringstream oss;
		switch (i) {
		case 0:
			if (get_stability().is_lcl()) {
				float alt(0);
				IcaoAtmosphere<float>::std_pressure_to_altitude(&alt, 0, get_stability().get_lcl_press());
				oss << "LCL F" << std::setw(3) << std::setfill('0') << Point::round<int,float>(alt * (Point::m_to_ft * 0.01f))
				    << ' ' << std::fixed << std::setprecision(1)
				    << (get_stability().get_lcl_temp() - IcaoAtmosphere<double>::degc_to_kelvin)
				    << "\302\260C";
			}
			break;

		case 1:
			if (get_stability().is_lfc()) {
				float alt(0);
				IcaoAtmosphere<float>::std_pressure_to_altitude(&alt, 0, get_stability().get_lfc_press());
				oss << "LFC F" << std::setw(3) << std::setfill('0') << Point::round<int,float>(alt * (Point::m_to_ft * 0.01f))
				    << ' ' << std::fixed << std::setprecision(1)
				    << (get_stability().get_lfc_temp() - IcaoAtmosphere<double>::degc_to_kelvin)
				    << "\302\260C";
			}
			break;

		case 2:
			if (get_stability().is_el()) {
				float alt(0);
				IcaoAtmosphere<float>::std_pressure_to_altitude(&alt, 0, get_stability().get_el_press());
				oss << "EL F" << std::setw(3) << std::setfill('0') << Point::round<int,float>(alt * (Point::m_to_ft * 0.01f))
				    << ' ' << std::fixed << std::setprecision(1)
				    << (get_stability().get_el_temp() - IcaoAtmosphere<double>::degc_to_kelvin)
				    << "\302\260C";
			}
			break;

		case 3:
			if (std::isnan(get_stability().get_liftedindex()))
				break;
			oss << "LI " << std::fixed << std::setprecision(1) << get_stability().get_liftedindex() << "\302\260C";
			break;

		case 4:
			if (std::isnan(get_stability().get_cape()))
				break;
			oss << "CAPE " << std::fixed << std::setprecision(0) << get_stability().get_cape() << "J/kg";
			break;

		case 5:
			if (std::isnan(get_stability().get_cin()))
				break;
			oss << "CIN " << std::fixed << std::setprecision(0) << get_stability().get_cin() << "J/kg";
			break;

		default:
			break;
		}
		if (oss.str().empty())
			continue;
		Glib::ustring txt(oss.str());
		for (Glib::ustring::size_type i(0), n(txt.size()); i < n; ++i)
			if (txt[i] == '-')
				txt.replace(i, 1, 1, (gunichar)0x2212);
		cr->get_text_extents(txt, ext);
		{
			double xc(x - ext.x_bearing), yc(y - ext.y_bearing);
			cr->set_source_rgb(1.0, 1.0, 1.0);
			cr->move_to(xc, yc + 1);
			cr->show_text(txt);
			cr->move_to(xc, yc - 1);
			cr->show_text(txt);
			cr->move_to(xc + 1, yc);
			cr->show_text(txt);
			cr->move_to(xc - 1, yc);
			cr->show_text(txt);
			cr->set_source_rgb(0.0, 0.0, 0.0);
			cr->move_to(xc, yc);
			cr->show_text(txt);
		}
		y += yadvance;
	}
}

void SkewTChartPrognostic::drawwindbarbs(const Cairo::RefPtr<Cairo::Context>& cr, double xorigin, double height, double xorigin2)
{
	if (std::isnan(xorigin))
		return;
	static const unsigned int nrsfc(sizeof(GRIB2::WeatherProfilePoint::isobaric_levels)/
					sizeof(GRIB2::WeatherProfilePoint::isobaric_levels[0]));
	for (unsigned int m = 0; m < 2; ++m) {
		for (unsigned int i = 1; i < nrsfc; ++i) {
			float wu(m_wpp[i].get_uwind()), wv(m_wpp[i].get_vwind());
			if (std::isnan(wu) || std::isnan(wv))
				continue;
			double y(transform(GRIB2::WeatherProfilePoint::isobaric_levels[i], 0).second);
			if (y > 1)
				continue;
			cr->save();
			cr->translate(xorigin, height - y * height);
			cr->scale(10, 10);
			draw_windbarbuv(cr, wu, wv, 1U << m);
			cr->restore();
		}
	}
}

SkewTChartSounding::SkewTChartSounding(const RadioSounding& prim, const RadioSounding& sec, int32_t alt)
	: SkewTChart(alt)
{
	m_data[0] = prim;
	m_data[1] = sec;
}

void SkewTChartSounding::calc_stability(void)
{
	m_stability = GRIB2::WeatherProfilePoint::Stability(m_data[0]);
}

std::ostream& SkewTChartSounding::drawtikztraces(std::ostream& os, double width, double height)
{
	for (unsigned int i = 0; i < 2; ++i) {
		const RadioSounding& snd(m_data[i]);
		// temperature curve
		{
			bool subseq(false);
			for (RadioSounding::levels_t::const_iterator li(snd.get_levels().begin()), le(snd.get_levels().end()); li != le; ++li) {
				if (!li->is_temp_valid())
					continue;
				os << (subseq ? " -- " : (i ? "    \\draw[styskewttracesectemp] " : "    \\draw[styskewttracetemp] "))
				   << pgfcoord(li->get_press_dpa() * 0.1, li->get_temp_degc10() * 0.1, width, height);
				subseq = true;
			}
			if (subseq)
				os << ';' << std::endl;
			else
				continue;
		}
		// dewpoint curve
		{
			bool subseq(false);
			for (RadioSounding::levels_t::const_iterator li(snd.get_levels().begin()), le(snd.get_levels().end()); li != le; ++li) {
				if (!li->is_dewpt_valid())
					continue;
				os << (subseq ? " -- " : (i ? "    \\draw[styskewttracesecdewpt] " : "    \\draw[styskewttracedewpt] "))
				   << pgfcoord(li->get_press_dpa() * 0.1, li->get_dewpt_degc10() * 0.1, width, height);
				subseq = true;
			}
			if (subseq)
				os << ';' << std::endl;
		}
	}
	return os;
}

std::ostream& SkewTChartSounding::drawtikzwindbarbs(std::ostream& os, double xorigin, double height, double xorigin2)
{
	for (unsigned int i = 0; i < 2; ++i) {
		double xorg(i ? xorigin2 : xorigin);
		if (std::isnan(xorg))
			continue;
		const RadioSounding& snd(m_data[i]);
		for (unsigned int m = 0; m < 2; ++m) {
			for (RadioSounding::levels_t::const_iterator li(snd.get_levels().begin()), le(snd.get_levels().end()); li != le; ++li) {
				if (!li->is_wind_valid())
					continue;
				double y(transform(li->get_press_dpa() * 0.1, 0).second);
				if (y > 1)
					continue;
				os << "  \\begin{scope}[shift={(" << xorg << ',' << (y * height)
				   << ")},scale=0.3]" << std::endl;
				draw_windbarb(os, li->get_winddir_deg(), li->get_windspeed_kts16() * (1.0f / 16), 1U << m);
				os << "  \\end{scope}" << std::endl;
			}
		}
	}
	return os;
}

std::ostream& SkewTChartSounding::drawtikzterrain(std::ostream& os, double width, double height)
{
	TopoDb30::elev_t e[2] = { TopoDb30::nodata, TopoDb30::nodata };
	for (unsigned int i = 0; i < 2; ++i) {
		const RadioSounding& snd(m_data[i]);
		if (snd.get_wmonr() == ~0U)
			continue;
		WMOStation::const_iterator stn(WMOStation::find(snd.get_wmonr()));
		if (stn == WMOStation::end())
			continue;
		e[i] = stn->get_elev();
	}
	if (e[0] == TopoDb30::nodata)
		e[0] = e[1];
	if (e[0] == TopoDb30::nodata || (e[1] != TopoDb30::nodata && e[1] != e[0]))
		return os;
	float press(0);
	IcaoAtmosphere<float>::std_altitude_to_pressure(&press, 0, e[0]);
	std::pair<double,double> r(transform(press, 0));
	return os << "      \\fill[colskewtgnd] (0,0) rectangle (" << width << ',' << (r.second * height) << ");" << std::endl;
}

std::ostream& SkewTChartSounding::drawtikzlegend(std::ostream& os, double width, double height)
{
	std::ostringstream oss;
	bool subseq(false);
	unsigned int laststn(~0U);
	for (unsigned int i = 0; i < 2; ++i) {
		const RadioSounding& snd(m_data[i]);
		if (snd.get_wmonr() == ~0U)
			continue;
		if (snd.get_wmonr() != laststn) {
			WMOStation::const_iterator stn(WMOStation::find(snd.get_wmonr()));
			if (stn != WMOStation::end()) {
				laststn = snd.get_wmonr();
				if (subseq)
					oss << "\\\\ ";
				subseq = true;
				oss << "\\contour{white}{";
				if (stn->get_icao())
					oss << METARTAFChart::latex_string_meta(std::string(stn->get_icao()))
					    << " (" << std::setfill('0') << std::setw(5) << laststn << ')';
				else
					oss << std::setfill('0') << std::setw(5) << laststn;
				if (!stn->get_coord().is_invalid())
					oss << " (" << stn->get_coord().get_fpl_str() << ')';
				oss << '}';
				if (stn->get_name()) {
					if (subseq)
						oss << "\\\\ ";
					subseq = true;
					oss << "\\contour{white}{" << METARTAFChart::latex_string_meta(std::string(stn->get_name())) << '}';
				}
			}
		}
		if (subseq)
			oss << "\\\\ ";
		subseq = true;
		{
			std::string tm(Glib::TimeVal(snd.get_obstime(), 0).as_iso8601());
			if (tm.size() > 10 && tm[10] == 'T')
				tm[10] = ' ';
			if (tm.size() >= 19 && !tm.compare(13, 6, ":00:00"))
				tm.erase(13, 6);
			oss << "\\contour{white}{" << tm << "}\\begin{tikzpicture}\\path[clip] (0,0) rectangle (0.5,0.2); \\draw["
			    << (i ? "styskewttracesectemp" : "styskewttracetemp") << "] (0,0.1) -- (0.5,0.1);\\end{tikzpicture}";
		}
	}
	for (int i = 0; i < 6; ++i) {
		std::ostringstream oss1;
		switch (i) {
		case 0:
			if (get_stability().is_lcl()) {
				float alt(0);
				IcaoAtmosphere<float>::std_pressure_to_altitude(&alt, 0, get_stability().get_lcl_press());
				oss << "LCL F" << std::setw(3) << std::setfill('0') << Point::round<int,float>(alt * (Point::m_to_ft * 0.01f))
				    << ' ' << std::fixed << std::setprecision(1)
				    << (get_stability().get_lcl_temp() - IcaoAtmosphere<double>::degc_to_kelvin)
				    << "$^\\circ$C";
			}
			break;

		case 1:
			if (get_stability().is_lfc()) {
				float alt(0);
				IcaoAtmosphere<float>::std_pressure_to_altitude(&alt, 0, get_stability().get_lfc_press());
				oss1 << "LFC F" << std::setw(3) << std::setfill('0') << Point::round<int,float>(alt * (Point::m_to_ft * 0.01f))
				    << ' ' << std::fixed << std::setprecision(1)
				    << (get_stability().get_lfc_temp() - IcaoAtmosphere<double>::degc_to_kelvin)
				    << "$^\\circ$C";
			}
			break;

		case 2:
			if (get_stability().is_el()) {
				float alt(0);
				IcaoAtmosphere<float>::std_pressure_to_altitude(&alt, 0, get_stability().get_el_press());
				oss1 << "EL F" << std::setw(3) << std::setfill('0') << Point::round<int,float>(alt * (Point::m_to_ft * 0.01f))
				    << ' ' << std::fixed << std::setprecision(1)
				    << (get_stability().get_el_temp() - IcaoAtmosphere<double>::degc_to_kelvin)
				    << "$^\\circ$C";
			}
			break;

		case 3:
			if (std::isnan(get_stability().get_liftedindex()))
				break;
			oss1 << "LI " << std::fixed << std::setprecision(1) << get_stability().get_liftedindex() << "$^\\circ$C";
			break;

		case 4:
			if (std::isnan(get_stability().get_cape()))
				break;
			oss1 << "CAPE " << std::fixed << std::setprecision(0) << get_stability().get_cape() << "J/kg";
			break;

		case 5:
			if (std::isnan(get_stability().get_cin()))
				break;
			oss1 << "CIN " << std::fixed << std::setprecision(0) << get_stability().get_cin() << "J/kg";
			break;

		default:
			break;
		}
		if (oss1.str().empty())
			continue;
		std::string txt(oss.str());
		for (Glib::ustring::size_type i(0), n(txt.size()); i < n; ++i)
			if (txt[i] == '-') {
				txt.replace(i, 1, "$-$");
				i += 2;
				n = txt.size();
			}
		if (subseq)
			oss << "\\\\ ";
		subseq = true;
		oss << "\\contour{white}{" << METARTAFChart::latex_string_meta(txt) << '}';
	}
	if (subseq)
		os << "    \\node[anchor=north west,align=left] at (0," << height << ") {" << oss.str() << "};" << std::endl;
	return os;
}

void SkewTChartSounding::drawtraces(const Cairo::RefPtr<Cairo::Context>& cr, double width, double height)
{
	for (unsigned int i = 0; i < 2; ++i) {
		const RadioSounding& snd(m_data[i]);
		// temperature curve
		set_style_tracetemp(cr, !!i);
		{
			bool subseq(false);
			for (RadioSounding::levels_t::const_iterator li(snd.get_levels().begin()), le(snd.get_levels().end()); li != le; ++li) {
				if (!li->is_temp_valid())
					continue;
				cairopath(cr, !subseq, li->get_press_dpa() * 0.1, li->get_temp_degc10() * 0.1, width, height);
				subseq = true;
			}
			if (subseq)
				cr->stroke();
			else
				continue;
		}
		// dewpoint curve
		set_style_tracedewpt(cr, !!i);
		{
			bool subseq(false);
			for (RadioSounding::levels_t::const_iterator li(snd.get_levels().begin()), le(snd.get_levels().end()); li != le; ++li) {
				if (!li->is_dewpt_valid())
					continue;
				cairopath(cr, !subseq, li->get_press_dpa() * 0.1, li->get_dewpt_degc10() * 0.1, width, height);
				subseq = true;
			}
			if (subseq)
				cr->stroke();
		}
	}
}

void SkewTChartSounding::drawterrain(const Cairo::RefPtr<Cairo::Context>& cr, double width, double height)
{
	TopoDb30::elev_t e[2] = { TopoDb30::nodata, TopoDb30::nodata };
	for (unsigned int i = 0; i < 2; ++i) {
		const RadioSounding& snd(m_data[i]);
		if (snd.get_wmonr() == ~0U)
			continue;
		WMOStation::const_iterator stn(WMOStation::find(snd.get_wmonr()));
		if (stn == WMOStation::end())
			continue;
		e[i] = stn->get_elev();
	}
	if (e[0] == TopoDb30::nodata)
		e[0] = e[1];
	if (e[0] == TopoDb30::nodata || (e[1] != TopoDb30::nodata && e[1] != e[0]))
		return;
	float press(0);
	IcaoAtmosphere<float>::std_altitude_to_pressure(&press, 0, e[0]);
	std::pair<double,double> r(transform(press, 0));
	set_color_gnd(cr);
	cr->rectangle(0, height, width, -r.second * height);
	cr->fill();
}

void SkewTChartSounding::drawlegend(const Cairo::RefPtr<Cairo::Context>& cr, double width, double height)
{
	cr->select_font_face("sans", Cairo::FONT_SLANT_NORMAL, Cairo::FONT_WEIGHT_NORMAL);
	cr->set_font_size(8);
	Cairo::TextExtents ext;
	cr->get_text_extents("M", ext);
	double yadvance(1.4 * ext.height);
	double x(3), y(3);
	unsigned int laststn(~0U);
	for (unsigned int i = 0; i < 2; ++i) {
		const RadioSounding& snd(m_data[i]);
		if (snd.get_wmonr() == ~0U)
			continue;
		if (snd.get_wmonr() != laststn) {
			WMOStation::const_iterator stn(WMOStation::find(snd.get_wmonr()));
			if (stn != WMOStation::end()) {
				laststn = snd.get_wmonr();
				std::ostringstream oss;
				if (stn->get_icao())
					oss << std::string(stn->get_icao())
					    << " (" << std::setfill('0') << std::setw(5) << laststn << ')';
				else
					oss << std::setfill('0') << std::setw(5) << laststn;
				if (!stn->get_coord().is_invalid())
					oss << " (" << stn->get_coord().get_fpl_str() << ')';
				cr->get_text_extents(oss.str(), ext);
				{
					double xc(x - ext.x_bearing), yc(y - ext.y_bearing);
					cr->set_source_rgb(1.0, 1.0, 1.0);
					cr->move_to(xc, yc + 1);
					cr->show_text(oss.str());
					cr->move_to(xc, yc - 1);
					cr->show_text(oss.str());
					cr->move_to(xc + 1, yc);
					cr->show_text(oss.str());
					cr->move_to(xc - 1, yc);
					cr->show_text(oss.str());
					cr->set_source_rgb(0.0, 0.0, 0.0);
					cr->move_to(xc, yc);
					cr->show_text(oss.str());
				}
				y += yadvance;
				if (stn->get_name()) {
					std::string txt(stn->get_name());
					cr->get_text_extents(txt, ext);
					{
						double xc(x - ext.x_bearing), yc(y - ext.y_bearing);
						cr->set_source_rgb(1.0, 1.0, 1.0);
						cr->move_to(xc, yc + 1);
						cr->show_text(txt);
						cr->move_to(xc, yc - 1);
						cr->show_text(txt);
						cr->move_to(xc + 1, yc);
						cr->show_text(txt);
						cr->move_to(xc - 1, yc);
						cr->show_text(txt);
						cr->set_source_rgb(0.0, 0.0, 0.0);
						cr->move_to(xc, yc);
						cr->show_text(txt);
					}
					y += yadvance;
				}
			}
		}
		{
			std::string tm(Glib::TimeVal(snd.get_obstime(), 0).as_iso8601());
			if (tm.size() > 10 && tm[10] == 'T')
				tm[10] = ' ';
			if (tm.size() >= 19 && !tm.compare(13, 6, ":00:00"))
				tm.erase(13, 6);
			cr->get_text_extents(tm, ext);
			{
				double xc(x - ext.x_bearing), yc(y - ext.y_bearing);
				cr->set_source_rgb(1.0, 1.0, 1.0);
				cr->move_to(xc, yc + 1);
				cr->show_text(tm);
				cr->move_to(xc, yc - 1);
				cr->show_text(tm);
				cr->move_to(xc + 1, yc);
				cr->show_text(tm);
				cr->move_to(xc - 1, yc);
				cr->show_text(tm);
				cr->set_source_rgb(0.0, 0.0, 0.0);
				cr->move_to(xc, yc);
				cr->show_text(tm);
				set_style_tracetemp(cr, !!i);
				cr->move_to(xc + ext.width, yc - 0.5 * ext.height);
				cr->rel_line_to(0.05 * width, 0);
				cr->stroke();
			}
			y += yadvance;
		}
	}
	cr->set_font_size(6);
	cr->get_text_extents("M", ext);
	yadvance = 1.4 * ext.height;
	y += yadvance;
	for (int i = 0; i < 6; ++i) {
		std::ostringstream oss;
		switch (i) {
		case 0:
			if (get_stability().is_lcl()) {
				float alt(0);
				IcaoAtmosphere<float>::std_pressure_to_altitude(&alt, 0, get_stability().get_lcl_press());
				oss << "LCL F" << std::setw(3) << std::setfill('0') << Point::round<int,float>(alt * (Point::m_to_ft * 0.01f))
				    << ' ' << std::fixed << std::setprecision(1)
				    << (get_stability().get_lcl_temp() - IcaoAtmosphere<double>::degc_to_kelvin)
				    << "\302\260C";
			}
			break;

		case 1:
			if (get_stability().is_lfc()) {
				float alt(0);
				IcaoAtmosphere<float>::std_pressure_to_altitude(&alt, 0, get_stability().get_lfc_press());
				oss << "LFC F" << std::setw(3) << std::setfill('0') << Point::round<int,float>(alt * (Point::m_to_ft * 0.01f))
				    << ' ' << std::fixed << std::setprecision(1)
				    << (get_stability().get_lfc_temp() - IcaoAtmosphere<double>::degc_to_kelvin)
				    << "\302\260C";
			}
			break;

		case 2:
			if (get_stability().is_el()) {
				float alt(0);
				IcaoAtmosphere<float>::std_pressure_to_altitude(&alt, 0, get_stability().get_el_press());
				oss << "EL F" << std::setw(3) << std::setfill('0') << Point::round<int,float>(alt * (Point::m_to_ft * 0.01f))
				    << ' ' << std::fixed << std::setprecision(1)
				    << (get_stability().get_el_temp() - IcaoAtmosphere<double>::degc_to_kelvin)
				    << "\302\260C";
			}
			break;

		case 3:
			if (std::isnan(get_stability().get_liftedindex()))
				break;
			oss << "LI " << std::fixed << std::setprecision(1) << get_stability().get_liftedindex() << "\302\260C";
			break;

		case 4:
			if (std::isnan(get_stability().get_cape()))
				break;
			oss << "CAPE " << std::fixed << std::setprecision(0) << get_stability().get_cape() << "J/kg";
			break;

		case 5:
			if (std::isnan(get_stability().get_cin()))
				break;
			oss << "CIN " << std::fixed << std::setprecision(0) << get_stability().get_cin() << "J/kg";
			break;

		default:
			break;
		}
		if (oss.str().empty())
			continue;
		Glib::ustring txt(oss.str());
		for (Glib::ustring::size_type i(0), n(txt.size()); i < n; ++i)
			if (txt[i] == '-')
				txt.replace(i, 1, 1, (gunichar)0x2212);
		cr->get_text_extents(txt, ext);
		{
			double xc(x - ext.x_bearing), yc(y - ext.y_bearing);
			cr->set_source_rgb(1.0, 1.0, 1.0);
			cr->move_to(xc, yc + 1);
			cr->show_text(txt);
			cr->move_to(xc, yc - 1);
			cr->show_text(txt);
			cr->move_to(xc + 1, yc);
			cr->show_text(txt);
			cr->move_to(xc - 1, yc);
			cr->show_text(txt);
			cr->set_source_rgb(0.0, 0.0, 0.0);
			cr->move_to(xc, yc);
			cr->show_text(txt);
		}
		y += yadvance;
	}
}

void SkewTChartSounding::drawwindbarbs(const Cairo::RefPtr<Cairo::Context>& cr, double xorigin, double height, double xorigin2)
{
	for (unsigned int i = 0; i < 2; ++i) {
		double xorg(i ? xorigin2 : xorigin);
		if (std::isnan(xorg))
			continue;
		const RadioSounding& snd(m_data[i]);
		for (unsigned int m = 0; m < 2; ++m) {
			double lasty(std::numeric_limits<double>::min());
			for (RadioSounding::levels_t::const_iterator li(snd.get_levels().begin()), le(snd.get_levels().end()); li != le; ++li) {
				if (!li->is_wind_valid())
					continue;
				double y(transform(li->get_press_dpa() * 0.1, 0).second);
				if (y > 1 || fabs(y - lasty) < 0.02)
					continue;
				lasty = y;
				cr->save();
				cr->translate(xorg, height - y * height);
				cr->scale(10, 10);
				draw_windbarb(cr, li->get_winddir_deg(), li->get_windspeed_kts16() * (1.0f / 16), 1U << m);
				cr->restore();
			}
		}
	}
}

const int16_t WindsAloft::isobaric_levels[3][6] = {
	{ 850, 750, 700, 600, 500, 450 },
	{ 700, 500, 450, 400, 350, 300 },
	{ 700, 500, 300, 250, 200, 150 }
};

WindsAloft::WindsAloft(const GRIB2& wxdb)
	: m_wxdb(&wxdb),
	  m_minefftime(std::numeric_limits<gint64>::max()),
	  m_maxefftime(std::numeric_limits<gint64>::min()),
	  m_minreftime(std::numeric_limits<gint64>::max()),
	  m_maxreftime(std::numeric_limits<gint64>::min()),
	  m_lvlindex(0)
{
}

WindsAloft::WindsAloft(const Aircraft& acft, const GRIB2& wxdb)
	: m_wxdb(&wxdb),
	  m_minefftime(std::numeric_limits<gint64>::max()),
	  m_maxefftime(std::numeric_limits<gint64>::min()),
	  m_minreftime(std::numeric_limits<gint64>::max()),
	  m_maxreftime(std::numeric_limits<gint64>::min()),
	  m_lvlindex(0)
{
	{
		double ceil(acft.get_climb().get_ceiling());
		if (ceil > 30000)
			m_lvlindex = 2;
		else if (ceil > 18000)
			m_lvlindex = 1;
	}
}

std::ostream& WindsAloft::definitions(std::ostream& os)
{
	os << ("\\newlength{\\windsalofttblname}\n"
	       "\\newlength{\\windsalofttblwind}\n"
	       "\\newlength{\\windsalofttbltropo}\n"
	       "\\settowidth{\\windsalofttblwind}{888888}\n"
	       "\\settowidth{\\windsalofttbltropo}{FL888}\n"
	       "\\setlength{\\windsalofttblname}{\\linewidth}\n"
	       "\\addtolength{\\windsalofttblname}{-")
	   << (sizeof(isobaric_levels[0])/sizeof(isobaric_levels[0][0]))
	   << (".0\\windsalofttblwind}\n"
	       "\\addtolength{\\windsalofttblname}{-\\windsalofttbltropo}\n"
	       "\\addtolength{\\windsalofttblname}{-")
	   << (2*(sizeof(isobaric_levels[0])/sizeof(isobaric_levels[0][0])+2))
	   << (".0\\tabcolsep}\n"
	       "\\addtolength{\\windsalofttblname}{-")
	   << (sizeof(isobaric_levels[0])/sizeof(isobaric_levels[0][0])+3)
	   << ".0\\arrayrulewidth}\n\n";
	return os;
}

std::ostream& WindsAloft::legend(std::ostream& os)
{
	os << ("\\medskip\\noindent$\\downarrow$: headwind, $\\uparrow$: tailwind, "
	       "$\\leftarrow$: crosswind from the right, $\\rightarrow$: crosswind from the left\n\n");
	return os;
}

std::ostream& WindsAloft::hhline(std::ostream& os, bool mid)
{
	os << "  \\hhline{|" << (mid ? '-' : '~');
	for (unsigned int i(0); i < (sizeof(isobaric_levels[0])/sizeof(isobaric_levels[0][0])+1); ++i)
		os << '|' << (mid ? '~' : '-');
	return os << "|}";
}

std::ostream& WindsAloft::winds_aloft(std::ostream& os, unsigned int& lines, const FPlanRoute& route)
{
	static const unsigned int maxlines = 83;
	if (!m_wxdb)
		return os;
	if (lines > maxlines)
		lines = maxlines;
	Glib::RefPtr<GRIB2::LayerInterpolateResult> wxwindu[sizeof(isobaric_levels[0])/sizeof(isobaric_levels[0][0])];
	Glib::RefPtr<GRIB2::LayerInterpolateResult> wxwindv[sizeof(isobaric_levels[0])/sizeof(isobaric_levels[0][0])];
	Glib::RefPtr<GRIB2::LayerInterpolateResult> wxtemperature[sizeof(isobaric_levels[0])/sizeof(isobaric_levels[0][0])];
	Glib::RefPtr<GRIB2::LayerInterpolateResult> wxprmsl;
	Glib::RefPtr<GRIB2::LayerInterpolateResult> wxtropopause;
	Rect bbox(route.get_bbox());
        Rect bbox1(bbox.oversize_nmi(200));
	unsigned int nr(route.get_nrwpt());
	bool intbl(false);
	for (unsigned int icur(0), iprev(0); iprev < nr; ) {
		const FPlanWaypoint& wptp(route[iprev]);
		if (wptp.get_coord().is_invalid()) {
			++iprev;
			continue;
		}
		for (icur = iprev + 1; icur < nr; ++icur) {
			const FPlanWaypoint& wptn(route[icur]);
			if (!wptn.get_coord().is_invalid() &&
			    (wptn.get_type() < FPlanWaypoint::type_generated_start || wptn.get_type() > FPlanWaypoint::type_generated_end))
				break;
		}
		if (intbl && (icur >= nr || lines < 3)) {
			hhline(os, false) << "\n  ";
			if (wptp.is_altitude_valid())
				os << "\\truncate{\\windsalofttblname}{" << wptp.get_fpl_altstr() << "} ";
			for (unsigned int i(0); i < sizeof(isobaric_levels[0])/sizeof(isobaric_levels[0][0]); ++i)
				os << " &";
			os << " & \\\\\n  \\hline\n\\end{tabular}\n\n";
			intbl = false;
		}
		if (icur >= nr)
			break;
		const FPlanWaypoint& wptn(route[icur]);
		if (!intbl) {
			if (lines < 4) {
				os << "\\clearpage\n\n";
				lines = maxlines;
			}
			os << "\\noindent\\begin{tabular}{|p{\\windsalofttblname}|";
			for (unsigned int i(0); i < sizeof(isobaric_levels[0])/sizeof(isobaric_levels[0][0]); ++i)
				os << "p{\\windsalofttblwind}|";
			os << "p{\\windsalofttbltropo}|}\n  \\hline\n  "
			   << "\\truncate{\\windsalofttblname}{" << METARTAFChart::latex_string_meta(wptp.get_icao_name())
			   << "}";
			for (unsigned int i(0); i < sizeof(isobaric_levels[0])/sizeof(isobaric_levels[0][0]); ++i) {
				float alt;
				IcaoAtmosphere<float>::std_pressure_to_altitude(&alt, 0, isobaric_levels[m_lvlindex][i]);
				os << " & FL" << std::setw(3) << std::setfill('0')
				   << Point::round<int32_t,float>(alt * (0.01f * Point::m_to_ft));
			}
			os << "& Tropo \\\\\n";
			--lines;
			intbl = true;
		}
		std::pair<Point,double> ptc(wptp.get_coord().get_gcnav(wptn.get_coord(), 0.5));
		gint64 efftime(route[0].get_time_unix() + ((wptp.get_flighttime() + wptn.get_flighttime()) >> 1));
		os << "% Point " << ptc.first.get_lat_str2() << ' ' << ptc.first.get_lon_str2() << std::endl
		   << "% Track " << ptc.second << std::endl
		   << "% Time " << Glib::TimeVal(efftime, 0).as_iso8601() << std::endl;
		for (unsigned int i(0); i < sizeof(isobaric_levels[0])/sizeof(isobaric_levels[0][0]); ++i) {
			const int16_t isobarlvl(isobaric_levels[m_lvlindex][i]);
			const uint8_t sfc((isobarlvl < 0) ? GRIB2::surface_specific_height_gnd : GRIB2::surface_isobaric_surface);
			const double pressure((isobarlvl < 0) ? 2 : isobarlvl * 100.0);
			if (!wxwindu[i] || efftime < wxwindu[i]->get_minefftime() || efftime > wxwindu[i]->get_maxefftime() ||
			    pressure < wxwindu[i]->get_minsurface1value() || pressure > wxwindu[i]->get_maxsurface1value() ||
			    !wxwindu[i]->get_bbox().is_inside(bbox)) {
				GRIB2::layerlist_t ll(const_cast<GRIB2 *>(m_wxdb)->find_layers(GRIB2::find_parameter(GRIB2::param_meteorology_momentum_ugrd),
											       efftime, sfc, pressure));
				wxwindu[i] = GRIB2::interpolate_results(bbox1, ll, efftime, pressure);
			}
			if (!wxwindv[i] || efftime < wxwindv[i]->get_minefftime() || efftime > wxwindv[i]->get_maxefftime() ||
			    pressure < wxwindv[i]->get_minsurface1value() || pressure > wxwindv[i]->get_maxsurface1value() ||
			    !wxwindv[i]->get_bbox().is_inside(bbox)) {
				GRIB2::layerlist_t ll(const_cast<GRIB2 *>(m_wxdb)->find_layers(GRIB2::find_parameter(GRIB2::param_meteorology_momentum_vgrd),
											       efftime, sfc, pressure));
				wxwindv[i] = GRIB2::interpolate_results(bbox1, ll, efftime, pressure);
			}
			if (!wxtemperature[i] || efftime < wxtemperature[i]->get_minefftime() || efftime > wxtemperature[i]->get_maxefftime() ||
			    pressure < wxtemperature[i]->get_minsurface1value() || pressure > wxtemperature[i]->get_maxsurface1value() ||
			    !wxtemperature[i]->get_bbox().is_inside(bbox)) {
				GRIB2::layerlist_t ll(const_cast<GRIB2 *>(m_wxdb)->find_layers(GRIB2::find_parameter(GRIB2::param_meteorology_temperature_tmp),
											       efftime, sfc, pressure));
				wxtemperature[i] = GRIB2::interpolate_results(bbox1, ll, efftime, pressure);
			}
			if (wxwindu[i]) {
				m_minefftime = std::min(m_minefftime, wxwindu[i]->get_minefftime());
				m_maxefftime = std::max(m_maxefftime, wxwindu[i]->get_maxefftime());
				m_minreftime = std::min(m_minreftime, wxwindu[i]->get_minreftime());
				m_maxreftime = std::max(m_maxreftime, wxwindu[i]->get_maxreftime());
			}
			if (wxwindv[i]) {
				m_minefftime = std::min(m_minefftime, wxwindv[i]->get_minefftime());
				m_maxefftime = std::max(m_maxefftime, wxwindv[i]->get_maxefftime());
				m_minreftime = std::min(m_minreftime, wxwindv[i]->get_minreftime());
				m_maxreftime = std::max(m_maxreftime, wxwindv[i]->get_maxreftime());
			}
			if (wxtemperature[i]) {
				m_minefftime = std::min(m_minefftime, wxtemperature[i]->get_minefftime());
				m_maxefftime = std::max(m_maxefftime, wxtemperature[i]->get_maxefftime());
				m_minreftime = std::min(m_minreftime, wxtemperature[i]->get_minreftime());
				m_maxreftime = std::max(m_maxreftime, wxtemperature[i]->get_maxreftime());
			}
		}
		if (!wxprmsl || efftime < wxprmsl->get_minefftime() || efftime > wxprmsl->get_maxefftime() ||
		    !wxprmsl->get_bbox().is_inside(bbox)) {
			GRIB2::layerlist_t ll(const_cast<GRIB2 *>(m_wxdb)->find_layers(GRIB2::find_parameter(GRIB2::param_meteorology_mass_prmsl),
										       efftime));
			wxprmsl = GRIB2::interpolate_results(bbox1, ll, efftime);
		}
		if (!wxtropopause || efftime < wxtropopause->get_minefftime() || efftime > wxtropopause->get_maxefftime() ||
		    !wxtropopause->get_bbox().is_inside(bbox)) {
			GRIB2::layerlist_t ll(const_cast<GRIB2 *>(m_wxdb)->find_layers(GRIB2::find_parameter(GRIB2::param_meteorology_mass_hgt),
										       efftime, GRIB2::surface_tropopause, 0));
			wxtropopause = GRIB2::interpolate_results(bbox1, ll, efftime);
		}
		if (wxprmsl) {
			m_minefftime = std::min(m_minefftime, wxprmsl->get_minefftime());
			m_maxefftime = std::max(m_maxefftime, wxprmsl->get_maxefftime());
			m_minreftime = std::min(m_minreftime, wxprmsl->get_minreftime());
			m_maxreftime = std::max(m_maxreftime, wxprmsl->get_maxreftime());
		}
		if (wxtropopause) {
			m_minefftime = std::min(m_minefftime, wxtropopause->get_minefftime());
			m_maxefftime = std::max(m_maxefftime, wxtropopause->get_maxefftime());
			m_minreftime = std::min(m_minreftime, wxtropopause->get_minreftime());
			m_maxreftime = std::max(m_maxreftime, wxtropopause->get_maxreftime());
		}
		hhline(os, false) << "\n  ";
		if (wptp.is_altitude_valid())
			os << "\\truncate{\\windsalofttblname}{" << wptp.get_fpl_altstr() << "} ";
		float windspeed[sizeof(isobaric_levels[0])/sizeof(isobaric_levels[0][0])];
		float winddirrad[sizeof(isobaric_levels[0])/sizeof(isobaric_levels[0][0])];
		for (unsigned int i(0); i < sizeof(isobaric_levels[0])/sizeof(isobaric_levels[0][0]); ++i) {
			windspeed[i] = winddirrad[i] = std::numeric_limits<float>::quiet_NaN();
			os << "& ";
			const int16_t isobarlvl(isobaric_levels[m_lvlindex][i]);
			const double pressure((isobarlvl < 0) ? 2 : isobarlvl * 100.0);
			if (wxwindu[i] && wxwindv[i]) {
				float wu(wxwindu[i]->operator()(ptc.first, efftime, pressure));
				float wv(wxwindv[i]->operator()(ptc.first, efftime, pressure));
				if (!std::isnan(wu) && !std::isnan(wv)) {
					wu *= (-1e-3f * Point::km_to_nmi * 3600);
					wv *= (-1e-3f * Point::km_to_nmi * 3600);
					windspeed[i] = sqrtf(wu * wu + wv * wv);
					winddirrad[i] = M_PI - atan2f(wu, wv);
				}
			}
			if (std::isnan(windspeed[i]) || std::isnan(winddirrad[i]))
				continue;
			os << std::setw(3) << std::setfill('0') << Point::round<int,float>(winddirrad[i] * Wind<float>::from_rad)
			   << std::setw(2) << std::setfill('0') << Point::round<int,float>(windspeed[i]) << ' ';
		}
		os << "& ";
		if (wxtropopause) {
			float tp(wxtropopause->operator()(ptc.first, efftime, 0.0) * Point::m_to_ft);
			if (!std::isnan(tp))
				os << "FL" << std::setw(3) << std::setfill('0') << Point::round<int,float>(tp * 0.01f);
		}
		hhline(os << "\\\\\n", true) << "\n  \\truncate{\\windsalofttblname}{" << wptn.get_icao_name() << "} ";
		for (unsigned int i(0); i < sizeof(isobaric_levels[0])/sizeof(isobaric_levels[0][0]); ++i) {
			os << "& ";
			if (std::isnan(windspeed[i]) || std::isnan(winddirrad[i]))
				continue;
			float dir(winddirrad[i] - ptc.second * Wind<float>::to_rad);
			if (std::isnan(dir))
				continue;
			float hw, xw;
			sincosf(dir, &xw, &hw);
			xw *= windspeed[i];
			hw *= windspeed[i];
			if (hw > 0)
				os << "$\\downarrow$";
			else
				os << "$\\uparrow$";
			os << Point::round<int,float>(fabsf(hw));
			if (xw > 0)
				os << "$\\leftarrow$";
			else
				os << "$\\rightarrow$";
			os << Point::round<int,float>(fabsf(xw));
		}
		os << "& ";
		if (wxprmsl) {
			float hpa(wxprmsl->operator()(ptc.first, efftime, 0.0) * 0.01f);
			if (!std::isnan(hpa))
				os << 'Q' << Point::round<int,float>(hpa);
		}
		os << "\\\\\n";
		lines -= 2;
		iprev = icur;
	}
	return os;
}
