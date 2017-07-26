//
// C++ Implementation: FlightDeckWindow
//
// Description: Flight Deck Windows, Altitude Instrument Drawing Areas
//
//
// Author: Thomas Sailer <t.sailer@alumni.ethz.ch>, (C) 2011, 2012, 2016
//
// Copyright: See COPYING file that comes with this distribution
//
//

#include "sysdeps.h"
#include <iomanip>

#include "flightdeckwindow.h"

FlightDeckWindow::NavAltDrawingArea::NavAltDrawingArea(BaseObjectType *castitem, const Glib::RefPtr<Builder>& refxml)
	: Gtk::DrawingArea(castitem), m_alt(std::numeric_limits<double>::quiet_NaN()),
	  m_altrate(std::numeric_limits<double>::quiet_NaN()), m_altbug(std::numeric_limits<double>::quiet_NaN()),
	  m_minimum(std::numeric_limits<double>::quiet_NaN()), m_glideslope(std::numeric_limits<double>::quiet_NaN()),
	  m_vsbug(std::numeric_limits<double>::quiet_NaN()), m_qnh(IcaoAtmosphere<double>::std_sealevel_pressure), m_elev(TopoDb30::nodata),
	  m_std(false), m_inhg(false), m_metric(false)
{
#ifndef GLIBMM_DEFAULT_SIGNAL_HANDLERS_ENABLED
#ifdef HAVE_GTKMM2
        signal_expose_event().connect(sigc::mem_fun(*this, &NavAltDrawingArea::on_expose_event));
#endif
#ifdef HAVE_GTKMM3
        signal_draw().connect(sigc::mem_fun(*this, &NavAltDrawingArea::on_draw));
#endif
#endif //GLIBMM_DEFAULT_SIGNAL_HANDLERS_ENABLED
	//m_altbug = 200;
	//add_altmarker("TNL", -245);
	//m_glideslope = 0.5;
}

FlightDeckWindow::NavAltDrawingArea::NavAltDrawingArea(void)
	: Gtk::DrawingArea(), m_alt(std::numeric_limits<double>::quiet_NaN()),
	  m_altrate(std::numeric_limits<double>::quiet_NaN()), m_altbug(std::numeric_limits<double>::quiet_NaN()),
	  m_glideslope(std::numeric_limits<double>::quiet_NaN()), m_vsbug(std::numeric_limits<double>::quiet_NaN()),
	  m_qnh(IcaoAtmosphere<double>::std_sealevel_pressure), m_elev(TopoDb30::nodata), m_std(false), m_inhg(false), m_metric(false)
{
#ifndef GLIBMM_DEFAULT_SIGNAL_HANDLERS_ENABLED
#ifdef HAVE_GTKMM2
        signal_expose_event().connect(sigc::mem_fun(*this, &NavAltDrawingArea::on_expose_event));
#endif
#ifdef HAVE_GTKMM3
        signal_draw().connect(sigc::mem_fun(*this, &NavAltDrawingArea::on_draw));
#endif
#endif //GLIBMM_DEFAULT_SIGNAL_HANDLERS_ENABLED
}

FlightDeckWindow::NavAltDrawingArea::~NavAltDrawingArea()
{
}

constexpr double FlightDeckWindow::NavAltDrawingArea::inhg_to_hpa;
constexpr double FlightDeckWindow::NavAltDrawingArea::hpa_to_inhg;

void FlightDeckWindow::NavAltDrawingArea::set_altitude(double alt, double altrate)
{
	bool chg(m_alt != alt || m_altrate != altrate);
	m_alt = alt;
	m_altrate = altrate;
	if (chg && get_is_drawable())
		queue_draw();
}

void FlightDeckWindow::NavAltDrawingArea::set_altbug(double altbug)
{
	bool chg(m_altbug != altbug);
	m_altbug = altbug;
	if (chg && get_is_drawable())
		queue_draw();
}

void FlightDeckWindow::NavAltDrawingArea::set_minimum(double minimum)
{
	bool chg(m_minimum != minimum);
	m_minimum = minimum;
	if (chg && get_is_drawable())
		queue_draw();
}

void FlightDeckWindow::NavAltDrawingArea::set_glideslope(double gs)
{
	bool chg(m_glideslope != gs);
	m_glideslope = gs;
	if (chg && get_is_drawable())
		queue_draw();
}

void FlightDeckWindow::NavAltDrawingArea::set_vsbug(double vs)
{
	bool chg(m_vsbug != vs);
	m_vsbug = vs;
	if (chg && get_is_drawable())
		queue_draw();
}

void FlightDeckWindow::NavAltDrawingArea::set_qnh(double qnh, bool std)
{
	bool chg(m_qnh != qnh || m_std != std);
	m_qnh = qnh;
	m_std = std;
	if (chg && get_is_drawable())
		queue_draw();
}

void FlightDeckWindow::NavAltDrawingArea::set_qnh(double qnh)
{
	bool chg(m_qnh != qnh);
	m_qnh = qnh;
	if (chg && get_is_drawable())
		queue_draw();
}

void FlightDeckWindow::NavAltDrawingArea::set_std(bool std)
{
	bool chg(m_std != std);
	m_std = std;
	if (chg && get_is_drawable())
		queue_draw();
}

void FlightDeckWindow::NavAltDrawingArea::set_inhg(bool inhg)
{
	bool chg(m_inhg != inhg);
	m_inhg = inhg;
	if (chg && get_is_drawable())
		queue_draw();
}

void FlightDeckWindow::NavAltDrawingArea::set_metric(bool metric)
{
	bool chg(m_metric != metric);
	m_metric = metric;
	if (chg && get_is_drawable())
		queue_draw();
}

void FlightDeckWindow::NavAltDrawingArea::set_altmarkers(const altmarkers_t& m)
{
	if (m_altmarkers.empty() && m.empty())
		return;
	m_altmarkers = m;
	if (get_is_drawable())
		queue_draw();
}

void FlightDeckWindow::NavAltDrawingArea::add_altmarker(const altmarker_t& m)
{
	m_altmarkers.push_back(m);
	if (get_is_drawable())
		queue_draw();
}

void FlightDeckWindow::NavAltDrawingArea::set_elevation(TopoDb30::elev_t elev)
{
	bool chg(m_elev != elev);
	m_elev = elev;
	if (chg && get_is_drawable())
		queue_draw();
}

#ifdef HAVE_GTKMM2
bool FlightDeckWindow::NavAltDrawingArea::on_expose_event(GdkEventExpose *event)
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

bool FlightDeckWindow::NavAltDrawingArea::on_draw(const Cairo::RefPtr<Cairo::Context>& cr)
{
	static const double altscaling = -1.0 / 250.0;
	static const double altratescaling = -0.9 / 2500.0;

	double alt_factor(1);
	if (is_metric())
		alt_factor = Point::ft_to_m_dbl;
	double alt(get_alt());
	if (std::isnan(alt))
		alt = 0;
	alt *= alt_factor;
	cr->save();
	// background
	//cr->set_source_rgba(0.0, 0.0, 0.0, 0.0);
	cr->set_source_rgb(0.0, 0.0, 0.0);
	cr->paint();
	// normalize
	{
		Gtk::Allocation allocation = get_allocation();
		const int width = allocation.get_width();
		const int height = allocation.get_height();
		const double scale = height * (1.0 / 2.4);
		cr->translate(width - 0.925 * scale, height * 0.5);
 		cr->scale(scale, scale);
	}
	// altimeter box
	cr->select_font_face("helvetica", Cairo::FONT_SLANT_NORMAL, Cairo::FONT_WEIGHT_NORMAL);
	cr->set_font_size(0.12);
	if (get_elevation() != TopoDb30::nodata) {
		double y(0);
		if (get_elevation() != TopoDb30::ocean) {
			if (is_metric())
				y = get_elevation();
			else
				y = TopoDb30::m_to_ft(get_elevation());
		}
		y -= alt;
		y *= altscaling;
		if (y >= 1.0) {
			set_color_transparent_background(cr);
			cr->rectangle(0.0, -1.0, 0.5, 2.0);
			cr->fill();
		} else {
			if (y < -1.0)
				y = -1.0;
			if (get_elevation() != TopoDb30::ocean)
				set_color_transparent_terrain(cr);
			else
				set_color_transparent_ocean(cr);
			cr->rectangle(0.0, y, 0.5, 1.0 - y);
			cr->fill();
			if (y > -1.0) {
				set_color_transparent_background(cr);
				cr->rectangle(0.0, -1.0, 0.5, y + 1.0);
				cr->fill();
			}
		}
	}
	cr->rectangle(0.0, -1.0, 0.5, 2.0);
	set_color_marking(cr);
	cr->set_line_width(0.01);
	cr->stroke_preserve();
	// scale
	cr->save();
	cr->clip();
	{
		int alt1 = alt;
		alt1 /= 100;
		alt1 *= 100;
		for (int alt2 = -300; alt2 <= 300; alt2 += 20) {
			int alt3(alt1 + alt2);
			double y(alt3 - alt);
			y *= altscaling;
			cr->move_to(0.0, y);
			if (alt3 % 100) {
				cr->rel_line_to(0.05, 0.0);
				cr->stroke();
				continue;
			}
			cr->rel_line_to(0.10, 0.0);
			cr->stroke();
			std::ostringstream oss;
			if (alt3)
				oss << alt3;
			else
				oss << "000";
			Cairo::TextExtents ext;
			cr->get_text_extents(oss.str(), ext);
			cr->move_to(0.45 - ext.width, y - 0.5 * ext.y_bearing);
			cr->show_text(oss.str());
		}
		// draw trend indicator
		if (!std::isnan(get_altrate())) {
			set_color_rate(cr);
			double y(get_altrate() * alt_factor * (6.0 / 60.0 * altscaling));
			cr->move_to(0.0, 0.0);
			cr->rel_line_to(0.0, y);
			cr->set_line_width(0.02);
			cr->stroke();
		}
		// draw altitude bug
		if (!std::isnan(get_altbug())) {
			set_color_altbug(cr);
			double y(std::max(-1.0, std::min(1.0, (get_altbug() - alt) * altscaling)));
			cr->move_to(0.0, y - 0.050);
			cr->rel_line_to( 0.050, 0.000);
			cr->rel_line_to( 0.000, 0.025);
			cr->rel_line_to(-0.025, 0.025);
			cr->rel_line_to( 0.025, 0.025);
			cr->rel_line_to( 0.000, 0.025);
			cr->rel_line_to(-0.050, 0.000);
			cr->close_path();
			cr->fill();
		}
		// main altitude window
		cr->save();
		cr->select_font_face("helvetica", Cairo::FONT_SLANT_NORMAL, Cairo::FONT_WEIGHT_BOLD);
		cr->set_font_size(0.12);
		cr->move_to(0.00,  0.00);
		cr->line_to(0.10,  0.03);
		cr->line_to(0.10,  0.08);
		cr->line_to(0.32,  0.08);
		cr->line_to(0.32,  0.20);
		cr->line_to(0.48,  0.20);
		cr->line_to(0.48, -0.20);
		cr->line_to(0.32, -0.20);
		cr->line_to(0.32, -0.08);
		cr->line_to(0.10, -0.08);
		cr->line_to(0.10, -0.03);
		cr->close_path();
		set_color_background(cr);
		cr->fill_preserve();
		set_color_marking(cr);
		cr->set_line_width(0.01);
		cr->stroke_preserve();
		cr->clip();
		Cairo::TextExtents ext;
		cr->get_text_extents("88", ext);
		{
			int alt1 = alt;
			alt1 /= 20;
			alt1 *= 20;
			for (int alt2 = -40; alt2 <= 40; alt2 += 20) {
				int alt3(alt1 + alt2);
				double y(alt3 - alt);
				y *= -0.075;
				y *= ext.height;
				char str[3];
				str[0] = '0' + (((alt3 / 10) + 10) % 10);
				str[1] = '0';
				str[2] = 0;
				Cairo::TextExtents ext2;
				cr->get_text_extents(str, ext2);
				cr->move_to(0.45 - ext2.width, y - 0.5 * ext2.y_bearing);
				cr->show_text(str);
			}
		}
		{
			Cairo::TextExtents ext2;
			std::ostringstream oss;
			int alt2(((alt + 10000) / 100) - 100);
			int alt3(alt - alt2 * 100);
			oss << alt2;
			cr->get_text_extents(oss.str(), ext2);
			if (alt3 < 80) {
				cr->move_to(0.45 - ext.width - ext2.width, - 0.5 * ext2.y_bearing);
				cr->show_text(oss.str());
			} else {
				double y(80 - alt3);
				y *= -0.075;
				y *= ext.height;
				cr->move_to(0.45 - ext.width - ext2.width, y - 0.5 * ext2.y_bearing);
				cr->show_text(oss.str());
				std::ostringstream oss2;
				oss2 << (alt2 + 1);
				cr->get_text_extents(oss2.str(), ext2);
				y += (-0.075 * 20) * ext.height;
				cr->move_to(0.45 - ext.width - ext2.width, y - 0.5 * ext2.y_bearing);
				cr->show_text(oss2.str());
			}
		}
		cr->restore();
	}
	if (std::isnan(get_alt())) {
		set_color_errorflag(cr);
		cr->set_line_width(0.03);
		cr->move_to(0.0, -1.0);
		cr->line_to(0.5,  1.0);
		cr->move_to(0.0,  1.0);
		cr->line_to(0.5, -1.0);
		cr->stroke();
	}
	cr->restore();
	// alt bug box
	cr->select_font_face("helvetica", Cairo::FONT_SLANT_NORMAL, Cairo::FONT_WEIGHT_NORMAL);
	cr->set_font_size(0.12);
	{
		cr->save();
		set_color_background(cr);
		cr->rectangle(0.0, -1.12, 0.5, 0.12);
		cr->fill_preserve();
		set_color_marking(cr);
		cr->set_line_width(0.01);
		cr->stroke_preserve();
		cr->clip();
		set_color_altbug(cr);
		cr->move_to(0.02, -1.06 - 0.050);
		cr->rel_line_to( 0.050, 0.000);
		cr->rel_line_to( 0.000, 0.025);
		cr->rel_line_to(-0.025, 0.025);
		cr->rel_line_to( 0.025, 0.025);
		cr->rel_line_to( 0.000, 0.025);
		cr->rel_line_to(-0.050, 0.000);
		cr->close_path();
		cr->fill();
		set_color_marking(cr);
		if (!std::isnan(get_altbug())) {
			std::ostringstream oss;
			oss << (int)(get_altbug() * alt_factor + 0.5);
			Cairo::TextExtents ext;
			cr->get_text_extents(oss.str(), ext);
			cr->move_to(0.45 - ext.width, -1.06 - 0.5 * ext.y_bearing);
			cr->show_text(oss.str());
		}
		cr->restore();
	}
	// QNH box
	{
		cr->save();
		set_color_background(cr);
		cr->rectangle(0.0, 1.0, 0.5, 0.12);
		cr->fill_preserve();
		set_color_marking(cr);
		cr->set_line_width(0.01);
		cr->stroke_preserve();
		cr->clip();
		if (is_std()) {
			std::ostringstream oss;
			oss << "STD";
			Cairo::TextExtents ext;
			cr->get_text_extents(oss.str(), ext);
			cr->move_to(0.45 - ext.width, 1.06 - 0.5 * ext.y_bearing);
			cr->show_text(oss.str());
		} else {
			std::ostringstream oss;
			std::string unit("hPa");
			if (is_inhg()) {
				oss << std::setprecision(2) << std::fixed << (get_qnh() * hpa_to_inhg);
				unit = "IN";
			} else {
				oss << (int)(get_qnh() + 0.5);
			}
			cr->set_font_size(0.12);
			Cairo::TextExtents ext;
			cr->get_text_extents(oss.str(), ext);
			cr->set_font_size(0.08);
			Cairo::TextExtents extu;
			cr->get_text_extents(unit, extu);
			cr->move_to(0.45 - extu.width, 1.06 - 0.5 * ext.y_bearing);
			cr->show_text(unit);
			cr->set_font_size(0.12);
			cr->move_to(0.42 - extu.width - ext.width, 1.06 - 0.5 * ext.y_bearing);
			cr->show_text(oss.str());
		}
		cr->restore();
	}
	// Variometer Scale
	{
		cr->select_font_face("helvetica", Cairo::FONT_SLANT_NORMAL, Cairo::FONT_WEIGHT_NORMAL);
		cr->set_font_size(0.12);
		cr->save();
		set_color_transparent_background(cr);
		cr->move_to(0.5, -0.9);
		cr->line_to(0.7, -0.9);
		cr->line_to(0.7, -0.1);
		cr->line_to(0.5,  0.0);
		cr->line_to(0.7,  0.1);
		cr->line_to(0.7,  0.9);
		cr->line_to(0.5,  0.9);
		cr->close_path();
		cr->fill_preserve();
		set_color_marking(cr);
		cr->set_line_width(0.01);
		cr->stroke_preserve();
		cr->clip();
		int altlim(2600);
		int altinc(200);
		int altmark(1000);
		double scaling(altratescaling);
		if (is_metric()) {
			altlim = 13;
			altinc = 1;
			altmark = 10;
			scaling *= 60 * Point::m_to_ft_dbl;
		}
		for (int alt2 = -altlim; alt2 <= altlim; alt2 += altinc) {
			double y(alt2 * scaling);
			cr->move_to(0.5, y);
			if (alt2 % altmark) {
				cr->rel_line_to(0.05, 0.0);
				cr->stroke();
				continue;
			}
			cr->rel_line_to(0.10, 0.0);
			cr->stroke();
			std::ostringstream oss;
			oss << (abs(alt2) / altmark);
			Cairo::TextExtents ext;
			cr->get_text_extents(oss.str(), ext);
			cr->move_to(0.61, y - 0.5 * ext.y_bearing);
			cr->show_text(oss.str());
		}
		cr->restore();
	}
	if (!std::isnan(get_vsbug())) {
		double y(std::max(-0.9, std::min(0.9, get_vsbug() * altratescaling)));
		const double ydelta(0.05);
		cr->save();
		set_color_vsbug(cr);
		cr->move_to(0.6 - ydelta, y);
		cr->rel_line_to(ydelta, -ydelta);
		cr->rel_line_to(ydelta, ydelta);
		cr->rel_line_to(-ydelta, ydelta);
		cr->close_path();
		cr->fill();
		cr->restore();
	}
	// Variometer "Bug"
	if (!std::isnan(get_altrate())) {
		double y(std::max(-0.9, std::min(0.9, get_altrate() * altratescaling)));
		cr->save();
		set_color_background(cr);
		cr->move_to(0.5, y);
		cr->rel_line_to( 0.12,  0.06);
		cr->rel_line_to( 0.28,  0.00);
		cr->rel_line_to( 0.00, -0.12);
		cr->rel_line_to(-0.28,  0.00);
		cr->close_path();
		cr->fill_preserve();
		set_color_marking(cr);
		cr->set_line_width(0.01);
		cr->stroke_preserve();
		cr->clip();
		if (abs(get_altrate()) >= 95) {
			std::ostringstream oss;
			if (is_metric())
				oss << std::fixed << std::setprecision(2) << get_altrate() * (Point::ft_to_m_dbl / 60.0);
			else
				oss << (int)(get_altrate() + 0.5);
			Cairo::TextExtents ext;
			cr->get_text_extents(oss.str(), ext);
			cr->move_to(0.89 - ext.x_bearing - ext.width, y - 0.5 * ext.y_bearing);
			cr->show_text(oss.str());
		}
		cr->restore();
	}
	// Altitude Marker
	if (!m_altmarkers.empty()) {
		cr->select_font_face("helvetica", Cairo::FONT_SLANT_NORMAL, Cairo::FONT_WEIGHT_NORMAL);
		cr->set_font_size(0.08);
		for (altmarkers_t::const_iterator ami(m_altmarkers.begin()), ame(m_altmarkers.end()); ami != ame; ++ami) {
			const altmarker_t& am(*ami);
			cr->save();
			Cairo::TextExtents ext;
			cr->get_text_extents(am.first, ext);
			double y(std::max(-1.06, std::min(1.06, (am.second * alt_factor - alt) * altscaling)));
			set_color_background(cr);
			cr->move_to(0.0, y);
			cr->rel_line_to(-0.05, 0.05);
			cr->rel_line_to(-ext.width-0.01, 0.00);
			cr->rel_line_to(0.00, -0.10);
			cr->rel_line_to(ext.width+0.01, 0.00);
			cr->close_path();
			cr->fill_preserve();
			set_color_marking(cr);
			cr->set_line_width(0.01);
			cr->stroke_preserve();
			cr->clip();
			cr->move_to(-ext.width-0.05, y - 0.5 * ext.y_bearing);
			cr->show_text(am.first);
			cr->restore();
		}
	}
	// Glideslope Indicator
	if (!std::isnan(get_glideslope())) {
		set_color_glideslope(cr);
		double y(std::max(-1.06, std::min(1.06, -get_glideslope())));
		cr->set_line_width(0.03);
		cr->move_to(-0.1, y-0.05);
		cr->rel_line_to(0.1, 0.05);
		cr->rel_line_to(-0.1, 0.05);
		cr->stroke();
	}
	// Minimum Indicator
	if (!std::isnan(get_minimum())) {
		double y(get_minimum() * alt_factor - alt);
		y *= altscaling;
		if (y >= -1.05 && y <= 1.05) {
			set_color_minimum(cr);
			cr->set_line_width(0.03);
			cr->move_to(-0.1, y-0.05);
			cr->rel_line_to(0.1, 0.05);
			cr->rel_line_to(-0.1, 0.05);
			cr->stroke();
		}
	}
	// Terrain Elevation
	static const char metrictext_ft[] = "ft";
	static const char metrictext_m[] = "m";
	static const char * const metrictext[2] = { metrictext_ft, metrictext_m };
	{
		cr->select_font_face("helvetica", Cairo::FONT_SLANT_NORMAL, Cairo::FONT_WEIGHT_NORMAL);
		cr->set_font_size(0.10);
		Cairo::TextExtents ext;
		cr->get_text_extents("88888ft", ext);
		double x(ext.width + 0.04);
		double y(ext.height + 0.04);
		cr->rectangle(0.9 - x, 1.12 - y, x, y);
		if (get_elevation() == TopoDb30::ocean)
			set_color_ocean(cr);
		else
			set_color_terrain(cr);
		cr->fill();
		set_color_marking(cr);
		static const char figdash[] = "\342\200\223";
		std::ostringstream oss;
		if (get_elevation() == TopoDb30::nodata)
			oss << figdash << figdash << figdash << figdash;
		else if (get_elevation() == TopoDb30::ocean)
			oss << 0;
		else if (is_metric())
			oss << get_elevation();
		else
			oss << TopoDb30::m_to_ft(get_elevation());
		oss << metrictext[is_metric()];
		cr->get_text_extents(oss.str(), ext);
		cr->move_to(0.88 - ext.width - ext.x_bearing, 1.10 - ext.height - ext.y_bearing);
		cr->show_text(oss.str());
		if (get_elevation() == TopoDb30::nodata) {
			set_color_errorflag(cr);
			cr->set_line_width(0.02);
			cr->move_to(0.9 - x, 1.12 - y);
			cr->line_to(0.9, 1.12);
			cr->move_to(0.9, 1.12 - y);
			cr->line_to(0.9 - x, 1.12);
			cr->stroke();
		}
	}
	// Metric Indicator
	{
		cr->select_font_face("helvetica", Cairo::FONT_SLANT_NORMAL, Cairo::FONT_WEIGHT_NORMAL);
		cr->set_font_size(0.14);
		Cairo::TextExtents ext;
		cr->get_text_extents(metrictext[is_metric()], ext);
		cr->move_to(0.55 - ext.x_bearing, -1.12 - ext.y_bearing);
		set_color_marking(cr);
		cr->show_text(metrictext[is_metric()]);
	}
	cr->restore();
	return true;
}
