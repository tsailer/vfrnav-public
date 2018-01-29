//
// C++ Implementation: FlightDeckWindow
//
// Description: Flight Deck Windows, Gyro Instrument Drawing Areas
//
//
// Author: Thomas Sailer <t.sailer@alumni.ethz.ch>, (C) 2011, 2012, 2013
//
// Copyright: See COPYING file that comes with this distribution
//
//

#include "sysdeps.h"

#include "flightdeckwindow.h"
#include "wind.h"

#include <memory>
#include <iomanip>

FlightDeckWindow::NavAIDrawingArea::NavAIDrawingArea(BaseObjectType *castitem, const Glib::RefPtr<Builder>& refxml)
	: Gtk::DrawingArea(castitem), m_bank(0), m_pitch(0), m_slip(0)
{
#ifndef GLIBMM_DEFAULT_SIGNAL_HANDLERS_ENABLED
#ifdef HAVE_GTKMM2
	signal_expose_event().connect(sigc::mem_fun(*this, &NavAIDrawingArea::on_expose_event));
#endif
#ifdef HAVE_GTKMM3
        signal_draw().connect(sigc::mem_fun(*this, &NavAIDrawingArea::on_draw));
#endif
#endif //GLIBMM_DEFAULT_SIGNAL_HANDLERS_ENABLED
	//set_size_request(128, 128);
	//add_events(Gdk::EXPOSURE_MASK | Gdk::PROPERTY_CHANGE_MASK | Gdk::POINTER_MOTION_MASK | Gdk::POINTER_MOTION_HINT_MASK);
}

FlightDeckWindow::NavAIDrawingArea::NavAIDrawingArea(void)
	: m_bank(0), m_pitch(0), m_slip(0)
{
#ifndef GLIBMM_DEFAULT_SIGNAL_HANDLERS_ENABLED
#ifdef HAVE_GTKMM2
	signal_expose_event().connect(sigc::mem_fun(*this, &NavAIDrawingArea::on_expose_event));
#endif
#ifdef HAVE_GTKMM3
        signal_draw().connect(sigc::mem_fun(*this, &NavAIDrawingArea::on_draw));
#endif
#endif //GLIBMM_DEFAULT_SIGNAL_HANDLERS_ENABLED
	//set_size_request(128, 128);
	//add_events(Gdk::EXPOSURE_MASK | Gdk::PROPERTY_CHANGE_MASK | Gdk::POINTER_MOTION_MASK | Gdk::POINTER_MOTION_HINT_MASK);
}

FlightDeckWindow::NavAIDrawingArea::~NavAIDrawingArea()
{
}

#ifdef HAVE_GTKMM2
bool FlightDeckWindow::NavAIDrawingArea::on_expose_event(GdkEventExpose *event)
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

bool FlightDeckWindow::NavAIDrawingArea::on_draw(const Cairo::RefPtr<Cairo::Context>& cr)
{
	cr->save();
	// normalize
	{
		Gtk::Allocation allocation = get_allocation();
		const int width = allocation.get_width();
		const int height = allocation.get_height();
		const double scale = 0.5 * std::min(width, height);
       		cr->translate(width * 0.5, height * 0.5);
		cr->scale(scale, scale);
	}
	// background
	cr->set_source_rgb(0.0, 0.0, 0.0);
	cr->paint();
	// draw instrument
	// clip to round disk
	cr->begin_new_path();
	cr->arc(0.0, 0.0, 1.0, 0.0, 2.0 * M_PI);
	cr->close_path();
	cr->clip();
	// rotate according to bank
	bool fail(false);
	cr->save();
	{
		double bank(get_bank());
		if (std::isnan(bank))
			fail = true;
		else
			cr->rotate(bank * (M_PI / 180.0));
	}
	// paint sky and earth
	set_color_earth(cr);
	cr->paint();
	cr->rectangle(-1.1, 0.0, 2.2, 1.1);
	cr->fill();
	set_color_sky(cr);
	cr->rectangle(-1.1, -1.1, 2.2, 1.1);
	cr->fill();
	// paint "sky pointer" scale
	// big triangle
	set_color_marking(cr);
	move_to_angle(cr, 0, 0.86);
	line_to_angle(cr, 5*M_PI/180.0, 1.0);
	line_to_angle(cr, -5*M_PI/180.0, 1.0);
	cr->close_path();
	cr->fill();
	// smaller triangle at +-45deg
	for (int i = -1; i <= 1; i += 2) {
		double a(i * (45*M_PI/180.0));
		move_to_angle(cr, a, 0.86);
		line_to_angle(cr, a+2.5*M_PI/180.0, 0.93);
		line_to_angle(cr, a-2.5*M_PI/180.0, 0.93);
		cr->close_path();
		cr->fill();
	}
	// 30deg markings
	cr->set_line_width(0.03);
	for (int i = -3; i <= 3; ++i) {
		if (!i)
			continue;
		double a(i * (30*M_PI/180.0));
		move_to_angle(cr, a, 0.86);
		line_to_angle(cr, a, 1.0);
		cr->stroke();
	}
	// smaller 10deg markings
	for (int i = -2; i <= 2; ++i) {
		if (!i)
			continue;
		double a(i * (10*M_PI/180.0));
		move_to_angle(cr, a, 0.86);
		line_to_angle(cr, a, 0.93);
		cr->stroke();
	}
	// clip to "inside" disk
	cr->reset_clip();
	cr->begin_new_path();
	cr->arc(0.0, 0.0, 0.86, 0.0, 2 * M_PI);
	cr->close_path();
	cr->clip();
	// shift inside disk according to pitch
	const double pitch_scale(-1.0 / 20.0 * 0.5);
	{
		double pitch(get_pitch());
		if (std::isnan(pitch))
			fail = true;
		else
			cr->translate(0, pitch * pitch_scale);
       	}
	// paint sky and earth
	set_color_earth(cr);
	cr->rectangle(-1.0, 0.0, 2.0, 100.0);
	cr->fill();
	set_color_sky(cr);
	cr->rectangle(-1.0, -100.0, 2.0, 100.0);
	cr->fill();
	set_color_marking(cr);
	cr->set_line_width(0.015);
	cr->move_to(-1.0, 0.0);
	cr->line_to(1.0, 0.0);
	cr->stroke();
	// small dashes
	for (int i = -3; i <= 3; i += 2) {
		double y(i * (5 * pitch_scale));
		cr->move_to(-0.08, y);
		cr->line_to(0.08, y);
		cr->stroke();
	}
	// large dashes
	cr->select_font_face("helvetica", Cairo::FONT_SLANT_NORMAL, Cairo::FONT_WEIGHT_NORMAL);
	cr->set_font_size(0.16);
	for (int i = -2; i <= 2; ++i) {
		if (!i)
			continue;
		double y(i * (10 * pitch_scale));
		double x(0.16 * abs(i));
		cr->move_to(-x, y);
		cr->line_to(x, y);
		cr->stroke();
		std::ostringstream oss;
		oss << (10 * abs(i));
		Cairo::TextExtents ext;
		cr->get_text_extents(oss.str(), ext);
		y -= 0.5 * ext.y_bearing;
		x += 0.02;
		cr->move_to(x, y);
		cr->show_text(oss.str());
		cr->move_to(-x - ext.width, y);
		cr->show_text(oss.str());		
	}
	cr->restore();
	// sky pointer
	cr->save();
	// clip to "inside" disk
	cr->reset_clip();
	cr->begin_new_path();
	cr->arc(0.0, 0.0, 0.86, 0.0, 2 * M_PI);
	cr->close_path();
	cr->clip();
	set_color_airplane(cr);
	cr->set_line_width(0.03);
	cr->translate(0, -0.86);
	cr->move_to(0, 0);
	line_to_angle(cr, 24*M_PI/180.0, -0.12);
	line_to_angle(cr, -24*M_PI/180.0, -0.12);
	cr->close_path();
	cr->fill();
	if (!std::isnan(get_slip())) {
		cr->translate(get_slip() * (0.1 * 0.06), 0);
		move_to_angle(cr, 24*M_PI/180.0, -0.14);
		line_to_angle(cr, -24*M_PI/180.0, -0.14);
		line_to_angle(cr, -24*M_PI/180.0, -0.19);
		line_to_angle(cr, 24*M_PI/180.0, -0.19);
		cr->close_path();
		cr->fill();
	}
	cr->restore();
	// aircraft symbol
	set_color_airplane(cr);
	cr->set_line_width(0.03);
	cr->move_to(-0.5, 0.0);
	cr->line_to(-0.15, 0.0);
	cr->line_to(-0.15, 0.05);
	cr->stroke();
	cr->move_to(0.5, 0.0);
	cr->line_to(0.15, 0.0);
	cr->line_to(0.15, 0.05);
	cr->stroke();
	cr->arc(0.0, 0.0, 0.03, 0.0, 2 * M_PI);
	cr->fill();
	// black border between external and internal disk
	cr->set_source_rgba(0.0, 0.0, 0.0, 0.2);
	cr->set_line_width(0.005);
	cr->begin_new_path();
	cr->arc(0.0, 0.0, 0.86, 0.0, 2.0 * M_PI);
	cr->close_path();
	cr->stroke();
	if (fail) {
		cr->reset_clip();
		cr->rectangle(-1, -1, 2, 2);
		cr->clip();
		cr->set_line_width(0.05);
		cr->set_source_rgba(1.0, 0.0, 0.0, 0.7);
		cr->begin_new_path();
		cr->move_to(-1, -1);
		cr->line_to(1, 1);
		cr->move_to(1, -1);
		cr->line_to(-1, 1);
		cr->stroke();
	}
	cr->restore();
	return true;
}

void FlightDeckWindow::NavAIDrawingArea::set_pitch_bank_slip(double p, double b, double s)
{
	bool chg(m_bank != b || m_pitch != p || m_slip != s);
	m_bank = b;
	m_pitch = p;
	m_slip = s;
	if (chg && get_is_drawable())
		queue_draw();
}

FlightDeckWindow::NavHSIDrawingArea::NavHSIDrawingArea(BaseObjectType *castitem, const Glib::RefPtr<Builder>& refxml)
	: Gtk::DrawingArea(castitem), m_engine(0), m_route(0), m_track(0), m_renderer(0), m_mapscale(0.1), m_altitude(0),
	  m_drawflags(MapRenderer::drawflags_route), m_centervalid(false), m_altvalid(false), m_hasterrain(false),
	  m_heading(std::numeric_limits<double>::quiet_NaN()), m_headingflags(Sensors::mapangle_manualheading),
	  m_course(std::numeric_limits<double>::quiet_NaN()), m_groundspeed(std::numeric_limits<double>::quiet_NaN()),
	  m_rate(std::numeric_limits<double>::quiet_NaN()), m_declination(std::numeric_limits<double>::quiet_NaN()),
	  m_wind_heading(std::numeric_limits<double>::quiet_NaN()), m_wind_headingflags(Sensors::mapangle_manualheading),
	  m_pointer1brg(std::numeric_limits<double>::quiet_NaN()), m_pointer1dev(std::numeric_limits<double>::quiet_NaN()),
	  m_pointer1dist(std::numeric_limits<double>::quiet_NaN()), m_pointer1maxdev(1.0), m_pointer1flag(pointer1flag_off),
	  m_pointer2brg(std::numeric_limits<double>::quiet_NaN()), m_pointer2dist(std::numeric_limits<double>::quiet_NaN()),
	  m_rat(std::numeric_limits<double>::quiet_NaN()), m_oat(std::numeric_limits<double>::quiet_NaN()),
	  m_ias(std::numeric_limits<double>::quiet_NaN()), m_tas(std::numeric_limits<double>::quiet_NaN()),
	  m_mach(std::numeric_limits<double>::quiet_NaN())
{
#ifndef GLIBMM_DEFAULT_SIGNAL_HANDLERS_ENABLED
#ifdef HAVE_GTKMM2
	signal_expose_event().connect(sigc::mem_fun(*this, &NavHSIDrawingArea::on_expose_event), false);
#endif
#ifdef HAVE_GTKMM3
        signal_draw().connect(sigc::mem_fun(*this, &NavHSIDrawingArea::on_draw));
#endif
        signal_size_allocate().connect(sigc::mem_fun(*this, &NavHSIDrawingArea::on_size_allocate));
        signal_show().connect(sigc::mem_fun(*this, &NavHSIDrawingArea::on_show));
        signal_hide().connect(sigc::mem_fun(*this, &NavHSIDrawingArea::on_hide));
#endif //GLIBMM_DEFAULT_SIGNAL_HANDLERS_ENABLED
        set_map_scale(0.1);

	// this is for testing
	if (false) {
		m_pointer1brg = 120;
		m_pointer1dev = 1.51;
		m_pointer1dist = 2.01;
		m_pointer1flag = pointer1flag_to;
		m_pointer1dest = "LSZK";

		m_pointer2brg = 220;
		m_pointer2dist = 12.25;
		m_pointer2dest = "KUDIS";

		m_course = 20.5;
		m_groundspeed = 102.3;
		m_declination = 3.54;
		m_rat = 18.3;
		m_oat = 16.2;
		m_ias = 99.999;
		m_tas = 110.2;
		m_mach = 0.212;
	}
}

FlightDeckWindow::NavHSIDrawingArea::NavHSIDrawingArea(void)
	: m_engine(0), m_route(0), m_track(0), m_renderer(0), m_mapscale(0.1), m_altitude(0),
	  m_drawflags(MapRenderer::drawflags_route), m_centervalid(false), m_altvalid(false), m_hasterrain(false),
	  m_heading(std::numeric_limits<double>::quiet_NaN()), m_headingflags(Sensors::mapangle_manualheading),
	  m_course(std::numeric_limits<double>::quiet_NaN()), m_groundspeed(std::numeric_limits<double>::quiet_NaN()),
	  m_rate(std::numeric_limits<double>::quiet_NaN()), m_declination(std::numeric_limits<double>::quiet_NaN()),
	  m_wind_heading(std::numeric_limits<double>::quiet_NaN()), m_wind_headingflags(Sensors::mapangle_manualheading),
	  m_pointer1brg(std::numeric_limits<double>::quiet_NaN()), m_pointer1dev(std::numeric_limits<double>::quiet_NaN()),
	  m_pointer1dist(std::numeric_limits<double>::quiet_NaN()), m_pointer1maxdev(1.0), m_pointer1flag(pointer1flag_off),
	  m_pointer2brg(std::numeric_limits<double>::quiet_NaN()), m_pointer2dist(std::numeric_limits<double>::quiet_NaN()),
	  m_rat(std::numeric_limits<double>::quiet_NaN()), m_oat(std::numeric_limits<double>::quiet_NaN()),
	  m_ias(std::numeric_limits<double>::quiet_NaN()), m_tas(std::numeric_limits<double>::quiet_NaN()),
	  m_mach(std::numeric_limits<double>::quiet_NaN())
{
#ifndef GLIBMM_DEFAULT_SIGNAL_HANDLERS_ENABLED
#ifdef HAVE_GTKMM2
	signal_expose_event().connect(sigc::mem_fun(*this, &NavHSIDrawingArea::on_expose_event));
#endif
#ifdef HAVE_GTKMM3
        signal_draw().connect(sigc::mem_fun(*this, &NavHSIDrawingArea::on_draw));
#endif
        signal_size_allocate().connect(sigc::mem_fun(*this, &NavHSIDrawingArea::on_size_allocate));
        signal_show().connect(sigc::mem_fun(*this, &NavHSIDrawingArea::on_show));
        signal_hide().connect(sigc::mem_fun(*this, &NavHSIDrawingArea::on_hide));
#endif //GLIBMM_DEFAULT_SIGNAL_HANDLERS_ENABLED
        set_map_scale(0.1);
}

FlightDeckWindow::NavHSIDrawingArea::~NavHSIDrawingArea()
{
	m_connupdate.disconnect();
        delete m_renderer;
}

#ifdef HAVE_GTKMM2
bool FlightDeckWindow::NavHSIDrawingArea::on_expose_event(GdkEventExpose *event)
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

bool FlightDeckWindow::NavHSIDrawingArea::on_draw(const Cairo::RefPtr<Cairo::Context>& cr)
{
	static const double xmin = -1.0;
	static const double xmax = 1.0;
	static const double ymin = -1.1;
	static const double ymax = 0.98;
	//static const char figdash[] = "\342\200\222";
	static const char figdash[] = "\342\200\223";
	static const char degreesign[] = "\302\260";

	Gtk::Allocation allocation = get_allocation();
	const int width = allocation.get_width();
	const int height = allocation.get_height();
	cr->save();
	// paint background (terrain if there is a renderer)
	if (m_renderer) {
		m_renderer->draw(cr, MapRenderer::ScreenCoord(width * (0.5 + xmin / (xmax - xmin)), height * (0.5 + ymin / (ymax - ymin))));
		// translucent background
		cr->set_source_rgba(0.0, 0.0, 0.0, 0.5);
		cr->paint();
	} else {
		// background
		cr->set_source_rgb(0.0, 0.0, 0.0);
		cr->paint();
	}
	// normalize
	{
		const double scale = std::min(width * (1.0 / (xmax - xmin)), height * (1.0 / (ymax - ymin)));
       		cr->translate(width * (xmin / (xmin - xmax)), height * (ymin / (ymin - ymax)));
		cr->scale(scale, scale);
	}
	// cache fonts
	cr->select_font_face("helvetica", Cairo::FONT_SLANT_NORMAL, Cairo::FONT_WEIGHT_NORMAL);
	Cairo::RefPtr<Cairo::FontFace> font_normal(cr->get_font_face());
	cr->select_font_face("helvetica", Cairo::FONT_SLANT_NORMAL, Cairo::FONT_WEIGHT_BOLD);
	Cairo::RefPtr<Cairo::FontFace> font_bold(cr->get_font_face());
	// determine whether a fail flag needs to be drawn
	bool fail(false);
	double hdg(get_heading());
	if (std::isnan(hdg)) {
		fail = true;
		hdg = 0;
	}
	double winddir(0), windspeed(0);
	bool havewind;
	{
		Wind<double> wind;
		wind.set_hdg_tas_crs_gs(get_wind_heading(), get_tas(), get_course(), get_groundspeed());
		havewind = !std::isnan(wind.get_winddir()) && !std::isnan(wind.get_windspeed());
		if (havewind) {
			winddir = wind.get_winddir();
			windspeed = wind.get_windspeed();
		}
	}
	// draw additional infos
	{
		static const double fontsizeinfo = 0.09;
		static const double fontsizelbl = 0.05;
		static const double baselineaddskip = 0.02;
		static const double xmargin = 0.01;
		static const bool drawfailedlabels = false;
		cr->set_font_face(font_normal);
		cr->set_font_size(fontsizeinfo);
		Cairo::RefPtr<Cairo::ScaledFont> font_info(cr->get_scaled_font());
		cr->set_font_size(fontsizelbl);
		Cairo::RefPtr<Cairo::ScaledFont> font_lbl(cr->get_scaled_font());
		set_color_marking(cr);
		double x(xmax - xmargin), y(ymin);
		{
			cr->set_scaled_font(font_info);
			double tas(get_tas());
			bool ok(!std::isnan(tas));
			Cairo::TextExtents ext;
			cr->get_text_extents("000", ext);
			y += baselineaddskip - ext.y_bearing;
			if (drawfailedlabels || ok) {
				std::ostringstream oss;
				if (ok)
					oss << std::setw(3) << Point::round<int,double>(tas);
				else
					oss << figdash << figdash << figdash;
				Cairo::TextExtents ext2;
				cr->get_text_extents(oss.str(), ext2);
				cr->move_to(x - ext2.width - ext2.x_bearing, y);
				cr->show_text(oss.str());
			}
			x -= ext.width;
			cr->set_scaled_font(font_lbl);
			cr->get_text_extents("TAS", ext);
			if (drawfailedlabels || ok) {
				cr->move_to(x - ext.width - ext.x_bearing, y);
				cr->show_text("TAS");
			}
			x -= ext.width;
			cr->get_text_extents("8", ext);
			x -= ext.width;
		}
		{
			cr->set_scaled_font(font_info);
			double mach(get_mach());
			bool ok(!std::isnan(mach));
			Cairo::TextExtents ext;
			cr->get_text_extents("0.00", ext);
			if (drawfailedlabels || ok) {
				std::ostringstream oss;
				if (ok)
					oss << std::setw(4) << std::fixed << std::showpoint << std::setprecision(2) << mach;
				else
					oss << figdash << '.' << figdash << figdash;
				Cairo::TextExtents ext2;
				cr->get_text_extents(oss.str(), ext2);
				cr->move_to(x - ext2.width - ext2.x_bearing, y);
				cr->show_text(oss.str());
			}
			x -= ext.width;
			cr->set_scaled_font(font_lbl);
			cr->get_text_extents("MA", ext);
			if (drawfailedlabels || ok) {
				cr->move_to(x - ext.width - ext.x_bearing, y);
				cr->show_text("MA");
			}
			x -= ext.width;
			cr->get_text_extents("8", ext);
			x -= ext.width;
		}
		x = xmax - xmargin;
		{
			cr->set_scaled_font(font_info);
			double gs(get_groundspeed());
			bool ok(!std::isnan(gs));
			Cairo::TextExtents ext;
			cr->get_text_extents("000", ext);
			y += baselineaddskip + ext.height;
			if (drawfailedlabels || ok) {
				std::ostringstream oss;
				if (ok)
					oss << std::setw(3) << Point::round<int,double>(gs);
				else
					oss << figdash << figdash << figdash;
				Cairo::TextExtents ext2;
				cr->get_text_extents(oss.str(), ext2);
				cr->move_to(x - ext2.width - ext2.x_bearing, y);
				cr->show_text(oss.str());
			}
			x -= ext.width;
			cr->set_scaled_font(font_lbl);
			cr->get_text_extents("GS", ext);
			if (drawfailedlabels || ok) {
				cr->move_to(x - ext.width - ext.x_bearing, y);
				cr->show_text("GS");
			}
			x -= ext.width;
			cr->get_text_extents("8", ext);
			x -= ext.width;
		}
		{
			cr->set_scaled_font(font_info);
			double oat(get_oat());
			bool ok(!std::isnan(oat));
			Cairo::TextExtents ext;
			cr->get_text_extents("000", ext);
			if (drawfailedlabels || ok) {
				std::ostringstream oss;
				if (ok)
					oss << std::setw(3) << Point::round<int,double>(oat);
				else
					oss << figdash << figdash << figdash;
				Cairo::TextExtents ext2;
				cr->get_text_extents(oss.str(), ext2);
				cr->move_to(x - ext2.width - ext2.x_bearing, y);
				cr->show_text(oss.str());
			}
			x -= ext.width;
			cr->set_scaled_font(font_lbl);
			cr->get_text_extents("OAT", ext);
			if (drawfailedlabels || ok) {
				cr->move_to(x - ext.width - ext.x_bearing, y);
				cr->show_text("OAT");
			}
			x -= ext.width;
			cr->get_text_extents("8", ext);
			x -= ext.width;
		}
		x = xmax - xmargin;
		{
			cr->set_scaled_font(font_info);
			bool ok(havewind && !std::isnan(winddir) && !std::isnan(windspeed));
			Cairo::TextExtents ext;
			cr->get_text_extents("000/00", ext);
			y += baselineaddskip + ext.height;
			if (drawfailedlabels || ok) {
				std::ostringstream oss;
				if (ok) {
					int dir(Point::round<int,double>(winddir));
					while (dir >= 360)
						dir -= 360;
					while (dir < 0)
						dir += 360;
					int speed(Point::round<int,double>(windspeed));
					oss << std::setw(3) << std::setfill('0') << dir << '/'
					    << std::setw(2) << std::setfill('0') << speed;
				} else {
					oss << figdash << figdash << figdash << '/' << figdash << figdash;
				}
				Cairo::TextExtents ext2;
				cr->get_text_extents(oss.str(), ext2);
				cr->move_to(x - ext2.width - ext2.x_bearing, y);
				cr->show_text(oss.str());
			}
			x -= ext.width;
			cr->set_scaled_font(font_lbl);
			cr->get_text_extents("WIND", ext);
			if (drawfailedlabels || ok) {
				cr->move_to(x - ext.width - ext.x_bearing, y);
				cr->show_text("WIND");
			}
			x -= ext.width;
			cr->get_text_extents("8", ext);
			x -= ext.width;
		}
		x = xmax - xmargin;
		{
			cr->set_scaled_font(font_info);
			double var(get_declination());
			bool ok(!std::isnan(var));
			Cairo::TextExtents ext;
			cr->get_text_extents("00W", ext);
			y += baselineaddskip + ext.height;
			if (drawfailedlabels || ok) {
				std::ostringstream oss;
				if (ok)
					oss << std::setw(2) << abs(Point::round<int,double>(var))
					    << (get_declination() < 0.0 ? 'W' : 'E');
				else
					oss << figdash << figdash << figdash;
				Cairo::TextExtents ext2;
				cr->get_text_extents(oss.str(), ext2);
				cr->move_to(x - ext2.width - ext2.x_bearing, y);
				cr->show_text(oss.str());
			}
			x -= ext.width;
			cr->set_scaled_font(font_lbl);
			cr->get_text_extents("VAR", ext);
			if (drawfailedlabels || ok) {
				cr->move_to(x - ext.width - ext.x_bearing, y);
				cr->show_text("VAR");
			}
			x -= ext.width;
			cr->get_text_extents("8", ext);
			x -= ext.width;
		}
		// Pointer Display
		y = ymin;
		x = xmin + xmargin;
		bool drawhsifullscale(!std::isnan(get_pointer1dev()));
		{
			double brg(get_pointer1brg());
			if (!std::isnan(brg)) {
				{
					cr->set_scaled_font(font_info);
					Cairo::TextExtents ext;
					cr->get_text_extents("888", ext);
					y += baselineaddskip - ext.y_bearing;
					set_color_pointer1(cr);
					cr->set_line_width(0.3 * ext.height);
					cr->move_to(x, y - 0.5 * ext.height);
					cr->rel_line_to(ext.width - 0.2 * ext.height, 0);
					cr->stroke();
					cr->move_to(x + ext.width, y - 0.5 * ext.height);
					cr->rel_line_to(-0.7 * ext.height, -0.5 * ext.height);
					cr->rel_line_to(0, ext.height);
					cr->close_path();
					cr->fill();
					set_color_marking(cr);
					x += ext.width;
				}
				const Glib::ustring& dest(get_pointer1dest());
				if (!dest.empty()) {
					cr->set_scaled_font(font_info);
					Cairo::TextExtents ext;
					cr->get_text_extents(dest, ext);
					cr->move_to(x - ext.x_bearing, y);
					cr->show_text(dest);
					x += ext.width;
				}
				x = xmin + xmargin;
				{
					cr->set_scaled_font(font_info);
					Cairo::TextExtents ext;
					cr->get_text_extents("888", ext);
					y += baselineaddskip + ext.height;
					cr->set_scaled_font(font_lbl);
					Cairo::TextExtents ext2;
					cr->get_text_extents("TO", ext2);
					cr->move_to(x - ext2.x_bearing, y);
					cr->show_text("TO");
					x += ext2.width;
					cr->set_scaled_font(font_info);
					std::ostringstream oss;
					{
						int a(Point::round<int,double>(brg));
						while (a >= 360)
							a -= 360;
						while (a < 0)
							a += 360;
						oss << std::setw(3) << std::setfill('0') << a;
					}
					cr->move_to(x - ext.x_bearing, y);
					cr->show_text(oss.str());
					x += ext.width;

				}
				double dist(get_pointer1dist());
				if (!std::isnan(dist)) {
					cr->set_scaled_font(font_lbl);
					Cairo::TextExtents ext;
					cr->get_text_extents("8D", ext);
					x += ext.width;
					cr->get_text_extents("D", ext);
					cr->move_to(x - ext.width - ext.x_bearing, y);
					cr->show_text("D");
					cr->set_scaled_font(font_info);
					std::ostringstream oss;
					dist = std::min(fabs(dist), 999.0);
					oss << std::fixed << std::showpoint << std::setw(4)
					    << std::setprecision((dist < 10) + (dist < 100)) << dist;
					cr->get_text_extents("8.88", ext);
					cr->move_to(x - ext.x_bearing, y);
					cr->show_text(oss.str());
					x += ext.width;
				}
			} else {
				cr->set_scaled_font(font_info);
				Cairo::TextExtents ext;
				cr->get_text_extents("888", ext);
				y += baselineaddskip - ext.y_bearing;
				drawhsifullscale = false;
			}
		}
		x = xmin + xmargin;
		{
			double brg(get_pointer2brg());
			if (!std::isnan(brg)) {
				{
					cr->set_scaled_font(font_info);
					Cairo::TextExtents ext;
					cr->get_text_extents("888", ext);
					y += baselineaddskip + ext.height;
					set_color_pointer2(cr);
					cr->set_line_width(0.15 * ext.height);
					cr->move_to(x, y - 0.35 * ext.height);
					cr->rel_line_to(ext.width - 0.2 * ext.height, 0);
					cr->move_to(x, y - 0.65 * ext.height);
					cr->rel_line_to(ext.width - 0.2 * ext.height, 0);
					cr->stroke();
					cr->move_to(x + ext.width, y - 0.5 * ext.height);
					cr->rel_line_to(-0.7 * ext.height, -0.5 * ext.height);
					cr->rel_line_to(0, ext.height);
					cr->close_path();
					cr->fill();
					set_color_marking(cr);
					x += ext.width;
				}
				const Glib::ustring& dest(get_pointer2dest());
				if (!dest.empty()) {
					cr->set_scaled_font(font_info);
					Cairo::TextExtents ext;
					cr->get_text_extents(dest, ext);
					cr->move_to(x - ext.x_bearing, y);
					cr->show_text(dest);
					x += ext.width;
				}
				x = xmin + xmargin;
				{
					cr->set_scaled_font(font_info);
					Cairo::TextExtents ext;
					cr->get_text_extents("888", ext);
					y += baselineaddskip + ext.height;
					cr->set_scaled_font(font_lbl);
					Cairo::TextExtents ext2;
					cr->get_text_extents("TO", ext2);
					cr->move_to(x - ext2.x_bearing, y);
					cr->show_text("TO");
					x += ext2.width;
					cr->set_scaled_font(font_info);
					std::ostringstream oss;
					{
						int a(Point::round<int,double>(brg));
						while (a >= 360)
							a -= 360;
						while (a < 0)
							a += 360;
						oss << std::setw(3) << std::setfill('0') << a;
					}
					cr->move_to(x - ext.x_bearing, y);
					cr->show_text(oss.str());
					x += ext.width;

				}
				double dist(get_pointer2dist());
				if (!std::isnan(dist)) {
					cr->set_scaled_font(font_lbl);
					Cairo::TextExtents ext;
					cr->get_text_extents("8D", ext);
					x += ext.width;
					cr->get_text_extents("D", ext);
					cr->move_to(x - ext.width - ext.x_bearing, y);
					cr->show_text("D");
					cr->set_scaled_font(font_info);
					std::ostringstream oss;
					dist = std::min(fabs(dist), 999.0);
					oss << std::fixed << std::showpoint << std::setw(4)
					    << std::setprecision((dist < 10) + (dist < 100)) << dist;
					cr->get_text_extents("8.88", ext);
					cr->move_to(x - ext.x_bearing, y);
					cr->show_text(oss.str());
					x += ext.width;
				}
			} else {
				cr->set_scaled_font(font_info);
				Cairo::TextExtents ext;
				cr->get_text_extents("888", ext);
				y += baselineaddskip - ext.y_bearing;
			}
		}
		x = xmin + xmargin;
		if (drawhsifullscale) {
			double maxdev(get_pointer1maxdev());
			if (maxdev > 0) {
				cr->set_scaled_font(font_info);
				Cairo::TextExtents ext;
				cr->get_text_extents("8.88", ext);
				y += baselineaddskip + 1.5 * ext.height;
				cr->set_scaled_font(font_lbl);
				Cairo::TextExtents ext2;
				cr->get_text_extents("FS", ext2);
				cr->move_to(x - ext2.x_bearing, y);
				cr->show_text("FS");
				x += ext2.width;
				cr->set_scaled_font(font_info);
				std::ostringstream oss;
				oss << std::fixed << std::showpoint << std::setw(4)
				    << std::setprecision((maxdev < 10) + (maxdev < 100)) << maxdev;
				cr->get_text_extents("8.88", ext);
				cr->move_to(x - ext.x_bearing, y);
				cr->show_text(oss.str());
				x += ext.width;
			}
		}
		y = ymax;
		x = xmin + xmargin;
		if (false)
			std::cerr  << "hasterrain " << (int)m_hasterrain
				   << " renderer " << (m_renderer ? "yes" : "no")
				   << " centervalid " << (int)m_centervalid
				   << " altvalid " << (int)m_altvalid << std::endl;
		if (m_hasterrain && (!m_renderer || !m_centervalid || !m_altvalid)) {
			cr->set_source_rgb(1.0, 0.0, 0.0);
			//cr->set_scaled_font(font_info);
			cr->set_font_face(font_bold);
			cr->set_font_size(fontsizeinfo);
			Cairo::TextExtents ext;
			cr->get_text_extents("NO TERRAIN", ext);
			y -= baselineaddskip + 1.5 * ext.height;
			cr->move_to(x - ext.x_bearing, y);
			cr->show_text("NO TERRAIN");
			x += ext.width;
		}
	}
	// wind circle
	if (havewind && !std::isnan(winddir) && !std::isnan(windspeed)) {
		cr->save();
		cr->translate(xmax - 0.2, ymax - 0.2);
		cr->scale(0.2, 0.2);
		// clip to round disk
		cr->begin_new_path();
		cr->arc(0.0, 0.0, 1.0, 0.0, 2.0 * M_PI);
		cr->close_path();
		cr->clip();
		set_color_wind(cr);
		cr->arc(0.0, 0.0, 0.075, 0.0, 2.0 * M_PI);
		cr->close_path();
		cr->fill();
		// speed is wind speed in 5 knots increments
		int speed(Point::round<int,double>(windspeed * 0.2));
		if (speed) {
			cr->rotate((winddir - hdg) * (M_PI / 180.0));
			cr->set_line_width(0.05);
			double y(-0.85);
			cr->move_to(0, 0);
			cr->line_to(0, y);
			cr->stroke();
			if (speed < 10)
				y -= 0.1;
			while (speed >= 10) {
				speed -= 10;
				cr->move_to(0, y);
				cr->rel_line_to(-0.2, 0.1);
				cr->rel_line_to(0.2, 0.1);
				cr->fill();
				y += 0.2;
			}
			while (speed >= 2) {
				speed -= 2;
				y += 0.1;
				cr->move_to(0, y);
				cr->rel_line_to(-0.2, -0.1);
				cr->stroke();
			}
			if (speed) {
				y += 0.1;
				cr->move_to(0, y);
				cr->rel_line_to(-0.1, -0.05);
				cr->stroke();
			}
		}
		cr->restore();
	}
	// draw instrument
	// clip to round disk
	cr->begin_new_path();
	cr->arc(0.0, 0.0, 1.0, 0.0, 2.0 * M_PI);
	cr->close_path();
	cr->begin_new_sub_path();
	cr->arc_negative(0.0, 0.0, 0.86, 2.0 * M_PI, 0.0);
	cr->close_path();
	cr->clip();
	// draw rate indicator
	if (!std::isnan(get_rate())) {
		// markings
		set_color_marking(cr);
		cr->set_line_width(0.015);
		for (int i = -2*6*3; i <= 2*6*3; i += 6*3) {
			if (!i)
				continue;
			double a(i * (M_PI / 180.0));
			move_to_angle(cr, a, 0.86);
			line_to_angle(cr, a, 0.95);
		}
		cr->stroke();
		set_color_rate(cr);
		double angle(get_rate() * (6 * 3 * M_PI / 180.0));
		static const double maxangle(40 * M_PI / 180.0);
		bool overflow(false);
		if (angle < -maxangle) {
			angle = -maxangle;
			overflow = true;
		} else if (angle > maxangle) {
			angle = maxangle;
			overflow = true;
		}
		cr->set_line_width(0.05);
		cr->begin_new_path();
		if (angle < 0) {
			cr->arc_negative(0.0, 0.0, 0.86, -0.5 * M_PI, angle - 0.5 * M_PI);
			if (overflow)
				line_to_angle(cr, -(35 * M_PI / 180.0), 0.90);
		} else {
			cr->arc(0.0, 0.0, 0.86, -0.5 * M_PI, angle - 0.5 * M_PI);
			if (overflow)
				line_to_angle(cr, (35 * M_PI / 180.0), 0.90);
		}
		cr->stroke();
	}
	// draw 45deg marker
	set_color_marking(cr);
	for (int i = 2; i < 8; i += 2) {
		move_to_angle(cr, i * (45.0 / 180.0 * M_PI), 0.86);
		line_to_angle(cr, i * (45.0 / 180.0 * M_PI) - (3.0 / 180.0 * M_PI), 0.92);
		line_to_angle(cr, i * (45.0 / 180.0 * M_PI) + (3.0 / 180.0 * M_PI), 0.92);
		cr->close_path();
		cr->fill();
	}
	for (int i = 1; i < 8; i += 2) {
		move_to_angle(cr, i * (45.0 / 180.0 * M_PI), 0.86);
		line_to_angle(cr, i * (45.0 / 180.0 * M_PI) - (2.0 / 180.0 * M_PI), 0.90);
		line_to_angle(cr, i * (45.0 / 180.0 * M_PI) + (2.0 / 180.0 * M_PI), 0.90);
		cr->close_path();
		cr->fill();
	}
	// draw course indicator
	if (!std::isnan(get_course()) && !(get_heading_flags() & Sensors::mapangle_course) && !std::isnan(get_heading())) {
		set_color_course(cr);
		double a((M_PI / 180.0) * (get_course() - get_heading()));
		move_to_angle(cr, a, 0.86);
		line_to_angle(cr, a - (2.0 * M_PI / 180.0), 0.92);
		line_to_angle(cr, a, 0.98);
		line_to_angle(cr, a + (2.0 * M_PI / 180.0), 0.92);
		cr->close_path();
		cr->fill();		
	}
	// draw heading bug and numeric heading
	cr->reset_clip();
	cr->rectangle(xmin, ymin, xmax - xmin, ymax - ymin);
	cr->clip();
	{
		if (get_heading_flags() & (Sensors::mapangle_course | Sensors::mapangle_heading | Sensors::mapangle_manualheading)) {
			cr->set_font_face(font_normal);
			cr->set_font_size(0.16);
			std::ostringstream oss;
			Cairo::TextExtents ext;
			{
				std::ostringstream oss2;
				if (get_heading_flags() & Sensors::mapangle_manualheading) {
					oss2 << 'M';
					oss << 'M';
				} else if (get_heading_flags() & Sensors::mapangle_course) {
					oss2 << 'C';
					oss << 'C';
				}
				oss2 << "000"<< degreesign;
				if (std::isnan(get_heading())) {
					oss << figdash << figdash << figdash << degreesign;
				} else {
					int a(Point::round<int,double>(get_heading()));
					while (a < 0)
						a += 360;
					while (a >= 360)
						a -= 360;
					oss << std::setw(3) << std::setfill('0') << a << degreesign;
				}
				if (get_heading_flags() & Sensors::mapangle_true) {
					oss2 << 'T';
					oss << 'T';
				}
				cr->get_text_extents(oss2.str(), ext);
			}
			double x(0.5 * ext.width + 0.04);
			cr->set_line_width(0.02);
			cr->move_to(-x, ymin);
			cr->line_to(-x, -0.93);
			cr->line_to(-0.05, -0.93);
			cr->line_to(0, -0.86);
			cr->line_to(0.05, -0.93);
			cr->line_to(x, -0.93);
			cr->line_to(x, ymin);
			{
				std::unique_ptr<Cairo::Path> path(cr->copy_path());
				cr->close_path();
				cr->set_source_rgba(0.0, 0.0, 0.0, 0.5);
				cr->fill();
				cr->begin_new_path();
				cr->append_path(*path);
			}
			set_color_hdgbug(cr);
			cr->stroke();
			cr->move_to(-0.5 * ext.width, ymin + 0.02 - ext.y_bearing);
			cr->show_text(oss.str());
		} else {
			double x(0.20);
			set_color_hdgbug(cr);
			cr->set_line_width(0.02);
			cr->move_to(-x, ymin);
			cr->line_to(-x, -0.93);
			cr->line_to(-0.05, -0.93);
			cr->line_to(0, -0.86);
			cr->line_to(0.05, -0.93);
			cr->line_to(x, -0.93);
			cr->line_to(x, ymin);
			cr->stroke();
		}
	}
	cr->save();
	// clip to "inside" disk
	cr->save();
	cr->reset_clip();
	cr->begin_new_path();
	cr->arc(0.0, 0.0, 0.86, 0.0, 2 * M_PI);
	cr->close_path();
	cr->clip();
	// rotate according to heading
	cr->rotate((-M_PI / 180.0) * hdg);
	set_color_marking(cr);
	cr->set_font_face(font_normal);
	cr->set_font_size(0.16);
	for (int i = 0; i < 72; ++i) {
		if (i & 1) {
			cr->set_line_width(0.015);
			cr->move_to(0, -0.80);
			cr->line_to(0, -0.86);
			cr->stroke();
			cr->rotate(5.0 / 180.0 * M_PI);
			continue;
		}
		cr->set_line_width(0.02);
		if (i % 6) {
			cr->move_to(0, -0.75);
			cr->line_to(0, -0.86);
			cr->stroke();
			cr->rotate(5.0 / 180.0 * M_PI);
			continue;
		}
		cr->move_to(0, -0.75);
		cr->line_to(0, -0.86);
		cr->stroke();
		std::ostringstream oss;
		switch (i) {
		case 0:
			oss << 'N';
			break;

		case 18:
			oss << 'E';
			break;

		case 36:
			oss << 'S';
			break;

		case 54:
			oss << 'W';
			break;

		default:
			oss << (i / 2);
			break;
		}
		Cairo::TextExtents ext;
		cr->get_text_extents(oss.str(), ext);
		cr->move_to(-0.5 * ext.width - ext.x_bearing, -0.74 - ext.y_bearing);
		cr->show_text(oss.str());
		cr->rotate(5.0 / 180.0 * M_PI);
	}
	// draw pointer 1
	{
		double brg(get_pointer1brg());
		double dev, devrel;
		if (!std::isnan(brg)) {
			cr->save();
			cr->rotate(brg * (M_PI / 180.0));
			dev = get_pointer1dev();
			devrel = get_pointer1maxdev();
			if (devrel <= 0)
				dev = std::numeric_limits<double>::quiet_NaN();
			else
				devrel = dev / devrel;
		}
		{
			int j(5 * (!std::isnan(brg) && !std::isnan(dev)));
			set_color_pointer1dots(cr);
			cr->set_line_width(0.010);
			for (int i = -j; i <= j; ++i) {
				cr->begin_new_path();
				cr->arc(i * (0.40 / 5), 0, 0.02, 0, 2*M_PI);
				cr->close_path();
				cr->stroke();
			}
		}
		// draw pointer 2
		{
			double brg2(get_pointer2brg());
			if (!std::isnan(brg2)) {
				cr->save();
				cr->rotate((brg2 - (std::isnan(brg) ? 0 : brg)) * (M_PI / 180.0));
				set_color_pointer2(cr);
				cr->set_line_width(0.015);
				cr->move_to(0.015, 0.70);
				cr->line_to(0.015, -0.65);
				cr->move_to(-0.015, 0.70);
				cr->line_to(-0.015, -0.65);
				cr->stroke();
				move_to_angle(cr, -5 * M_PI / 180.0, 0.60);
				cr->line_to(0.0, -0.70);
				line_to_angle(cr, 5 * M_PI / 180.0, 0.60);
				cr->close_path();
				cr->fill();
				cr->restore();
			}
		}
		if (!std::isnan(brg)) {
			set_color_pointer1(cr);
			cr->set_line_width(0.03);
			cr->move_to(0.0, 0.70);
			if (std::isnan(dev)) {
				cr->line_to(0.0, -0.65);
				cr->stroke();
			} else {
				bool xtk(fabs(devrel) > 1);
				if (xtk)
					devrel = (devrel < 0) ? -1 : 1;
				cr->line_to(0.0, 0.40);
				cr->move_to(0.40 * devrel, 0.38);
				cr->rel_line_to(0.0, -2 * 0.38);
				cr->move_to(0.0, -0.40);
				cr->line_to(0.0, -0.65);
				cr->stroke();
				if (xtk) {
					cr->save();
					set_color_marking(cr);
					cr->set_font_face(font_normal);
					cr->set_font_size(0.10);
					cr->rotate((hdg - brg) * (M_PI / 180.0));
					std::ostringstream oss;
					double deva(fabs(dev));
					oss << "XTK " << std::fixed << std::setprecision(1 + (deva < 10)) << deva;
					Cairo::TextExtents ext;
					cr->get_text_extents(oss.str(), ext);
					cr->move_to(-0.5 * ext.width - ext.x_bearing, -0.5 * ext.y_bearing);
					cr->show_text(oss.str());
					set_color_pointer1(cr);
					cr->restore();
				}
			}
			move_to_angle(cr, -5 * M_PI / 180.0, 0.60);
			cr->line_to(0.0, -0.70);
			line_to_angle(cr, 5 * M_PI / 180.0, 0.60);
			cr->close_path();
			cr->fill();
			set_color_marking(cr);
			if (m_pointer1flag == pointer1flag_to) {
				cr->move_to(0.16, -0.16);
				cr->line_to(0.32, -0.16);
				cr->line_to(0.24, -0.30);
				cr->close_path();
				cr->fill();
			} else if (m_pointer1flag == pointer1flag_from) {
				cr->move_to(0.16, 0.16);
				cr->line_to(0.32, 0.16);
				cr->line_to(0.24, 0.30);
				cr->close_path();
				cr->fill();
			}
			cr->restore();
		}
	}
	cr->restore();
	// end
	if (fail) {
		cr->reset_clip();
		cr->rectangle(-1, -1, 2, 2);
		cr->clip();
		cr->set_line_width(0.05);
		cr->set_source_rgba(1.0, 0.0, 0.0, 0.7);
		cr->begin_new_path();
		cr->move_to(-1, -1);
		cr->line_to(1, 1);
		cr->move_to(1, -1);
		cr->line_to(-1, 1);
		cr->stroke();
	}
	cr->restore();
	return true;
}

void FlightDeckWindow::NavHSIDrawingArea::on_size_allocate(Gtk::Allocation& allocation)
{
#ifdef GLIBMM_DEFAULT_SIGNAL_HANDLERS_ENABLED
	Gtk::DrawingArea::on_size_allocate(allocation);
#endif //GLIBMM_DEFAULT_SIGNAL_HANDLERS_ENABLED
        if (m_renderer) {
		//static const double ymin = -1.1;
		//static const double ymax = 0.98;
		//* (1 - 0.5 * (ymin + ymax))
                m_renderer->set_screensize(MapRenderer::ScreenCoord(allocation.get_width(), allocation.get_height()));
	}
}

void FlightDeckWindow::NavHSIDrawingArea::on_show(void)
{
#ifdef GLIBMM_DEFAULT_SIGNAL_HANDLERS_ENABLED
	Gtk::DrawingArea::on_show();
#endif //GLIBMM_DEFAULT_SIGNAL_HANDLERS_ENABLED
        if (m_renderer)
                m_renderer->show();
}

void FlightDeckWindow::NavHSIDrawingArea::on_hide(void)
{
#ifdef GLIBMM_DEFAULT_SIGNAL_HANDLERS_ENABLED
	Gtk::DrawingArea::on_hide();
#endif //GLIBMM_DEFAULT_SIGNAL_HANDLERS_ENABLED
        if (m_renderer)
                m_renderer->hide();
}

void FlightDeckWindow::NavHSIDrawingArea::set_heading(double h, Sensors::mapangle_t hflags)
{
	bool chg(m_headingflags != hflags || m_heading != h);
	m_headingflags = hflags;
	m_heading = h;
	if (chg) {
		double hdg(get_heading());
		if (std::isnan(hdg))
			hdg = 0;
		if (m_renderer) {
			Glib::TimeVal tv;
			tv.assign_current_time();
			m_renderer->set_center(m_center, m_altvalid ? m_altitude : 999999, hdg * MapRenderer::from_deg_16bit_dbl, tv.tv_sec);
			if (!get_visible())
				m_renderer->hide();
		}
		if (get_is_drawable())
			queue_draw();
	}
}

void FlightDeckWindow::NavHSIDrawingArea::set_rate(double r)
{
	bool chg(m_rate != r);
	m_rate = r;
	if (chg && get_is_drawable())
		queue_draw();
}

void FlightDeckWindow::NavHSIDrawingArea::set_heading_rate(double h, double r, Sensors::mapangle_t hflags)
{
	bool chg(m_headingflags != hflags || m_heading != h || m_rate != r);
	m_headingflags = hflags;
	m_heading = h;
	m_rate = r;
	if (chg) {
		double hdg(get_heading());
		if (std::isnan(hdg))
			hdg = 0;
		if (m_renderer) {
			Glib::TimeVal tv;
			tv.assign_current_time();
			m_renderer->set_center(m_center, m_altvalid ? m_altitude : 999999, hdg * MapRenderer::from_deg_16bit_dbl, tv.tv_sec);
			if (!get_visible())
				m_renderer->hide();
		}
		if (get_is_drawable())
			queue_draw();
	}
}

void FlightDeckWindow::NavHSIDrawingArea::set_course(double crs)
{
	bool chg(m_course != crs);
	m_course = crs;
	if (chg && get_is_drawable())
		queue_draw();
}

void FlightDeckWindow::NavHSIDrawingArea::set_groundspeed(double gs)
{
	bool chg(m_groundspeed != gs);
	m_groundspeed = gs;
	if (chg && get_is_drawable())
		queue_draw();
}

void FlightDeckWindow::NavHSIDrawingArea::set_course_groundspeed(double crs, double gs)
{
	bool chg(m_course != crs || m_groundspeed != gs);
	m_course = crs;
	m_groundspeed = gs;
	if (chg && get_is_drawable())
		queue_draw();
}

void FlightDeckWindow::NavHSIDrawingArea::set_declination(double declination)
{
	bool chg(m_declination != declination);
	m_declination = declination;
	if (chg && get_is_drawable())
		queue_draw();
}

void FlightDeckWindow::NavHSIDrawingArea::set_wind_heading(double h, Sensors::mapangle_t hflags)
{
	bool chg(m_wind_headingflags != hflags || m_wind_heading != h);
	m_wind_headingflags = hflags;
	m_wind_heading = h;
	if (chg && get_is_drawable())
		queue_draw();
}

void FlightDeckWindow::NavHSIDrawingArea::set_pointer1(double brg, double dev, double dist)
{
	bool chg(m_pointer1brg != brg || m_pointer1dev != dev || m_pointer1dist != dist);
	m_pointer1brg = brg;
	m_pointer1dev = dev;
	m_pointer1dist = dist;
	if (chg && get_is_drawable())
		queue_draw();
}

void FlightDeckWindow::NavHSIDrawingArea::set_pointer1flag(pointer1flag_t flag)
{
	bool chg(m_pointer1flag != flag);
	m_pointer1flag = flag;
	if (chg && get_is_drawable())
		queue_draw();
}

void FlightDeckWindow::NavHSIDrawingArea::set_pointer1dest(const Glib::ustring& dest)
{
	bool chg(m_pointer1dest != dest);
	m_pointer1dest = dest;
	if (chg && get_is_drawable())
		queue_draw();
}

void FlightDeckWindow::NavHSIDrawingArea::set_pointer1maxdev(double maxdev)
{
	if (std::isnan(maxdev))
		maxdev = 0;
	maxdev = std::max(maxdev, 0.0);
	bool chg(m_pointer1maxdev != maxdev);
	m_pointer1maxdev = maxdev;
	if (chg && get_is_drawable())
		queue_draw();	
}

void FlightDeckWindow::NavHSIDrawingArea::set_pointer2(double brg, double dist)
{
	bool chg(m_pointer2brg != brg || m_pointer2dist != dist);
	m_pointer2brg = brg;
	m_pointer2dist = dist;
	if (chg && get_is_drawable())
		queue_draw();
}

void FlightDeckWindow::NavHSIDrawingArea::set_pointer2dest(const Glib::ustring& dest)
{
	bool chg(m_pointer2dest != dest);
	m_pointer2dest = dest;
	if (chg && get_is_drawable())
		queue_draw();
}

void FlightDeckWindow::NavHSIDrawingArea::set_rat(double rat)
{
	bool chg(m_rat != rat);
	m_rat = rat;
	if (chg && get_is_drawable())
		queue_draw();
}

void FlightDeckWindow::NavHSIDrawingArea::set_oat(double oat)
{
	bool chg(m_oat != oat);
	m_oat = oat;
	if (chg && get_is_drawable())
		queue_draw();
}

void FlightDeckWindow::NavHSIDrawingArea::set_ias(double ias)
{
	bool chg(m_ias != ias);
	m_ias = ias;
	if (chg && get_is_drawable())
		queue_draw();
}

void FlightDeckWindow::NavHSIDrawingArea::set_tas(double tas)
{
	bool chg(m_tas != tas);
	m_tas = tas;
	if (chg && get_is_drawable())
		queue_draw();
}

void FlightDeckWindow::NavHSIDrawingArea::set_mach(double mach)
{
	bool chg(m_mach != mach);
	m_mach = mach;
	if (chg && get_is_drawable())
		queue_draw();
}

void FlightDeckWindow::NavHSIDrawingArea::set_engine(Engine& eng)
{
        m_engine = &eng;
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
	m_renderer = new VectorMapRenderer(*m_engine, m_dispatch, depth, VectorMapRenderer::ScreenCoord(0, 0), Point(), VectorMapRenderer::renderer_terrain);
        if (m_renderer) {
		Glib::TimeVal tv;
		tv.assign_current_time();
                m_connupdate = m_dispatch.connect(sigc::mem_fun(*this, &FlightDeckWindow::NavHSIDrawingArea::renderer_update));
                m_renderer->set_maxalt(m_engine->get_prefs().maxalt * 100);
                m_renderer->set_route(m_route);
                m_renderer->set_track(m_track);
                m_renderer->set_center(m_center, m_altitude, 0, tv.tv_sec);
                m_renderer->set_map_scale(m_mapscale);
		m_renderer->set_glidemodel(m_glidemodel);
		m_renderer->set_wind(m_glidewinddir, m_glidewindspeed);
                *m_renderer = m_drawflags;
                m_renderer->set_screensize(MapRenderer::ScreenCoord(get_width(), get_height()));
		m_hasterrain = true;
        }
}

void FlightDeckWindow::NavHSIDrawingArea::set_route(const FPlanRoute& route)
{
        m_route = &route;
        if (!m_renderer)
                return;
        m_renderer->set_route(m_route);
}

void FlightDeckWindow::NavHSIDrawingArea::set_track(const TracksDb::Track *track)
{
        m_track = track;
        if (!m_renderer)
                return;
        m_renderer->set_track(m_track);
}

void FlightDeckWindow::NavHSIDrawingArea::renderer_update(void)
{
	if (get_is_drawable())
		queue_draw();
}

void FlightDeckWindow::NavHSIDrawingArea::set_map_scale(float nmi_per_pixel)
{
        m_mapscale = nmi_per_pixel;
        if (!m_renderer)
                return;
        m_renderer->set_map_scale(m_mapscale);
        m_mapscale = m_renderer->get_map_scale();
 	if (get_is_drawable())
		queue_draw();
}

float FlightDeckWindow::NavHSIDrawingArea::get_map_scale(void) const
{
        if (!m_renderer)
                return m_mapscale;
        return m_renderer->get_map_scale();
}

void FlightDeckWindow::NavHSIDrawingArea::set_glidemodel(const MapRenderer::GlideModel& gm)
{
	m_glidemodel = gm;
	if (!m_renderer)
                return;
        m_renderer->set_glidemodel(m_glidemodel);
        m_glidemodel = m_renderer->get_glidemodel();
 	if (get_is_drawable())
		queue_draw();
}

const MapRenderer::GlideModel& FlightDeckWindow::NavHSIDrawingArea::get_glidemodel(void) const
{
        if (!m_renderer)
                return m_glidemodel;
        return m_renderer->get_glidemodel();	
}

void FlightDeckWindow::NavHSIDrawingArea::set_glidewind(float dir, float speed)
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
	m_glidewinddir = dir;
	m_glidewindspeed = speed;
	if (!m_renderer)
		return;
	m_renderer->set_wind(m_glidewinddir, m_glidewindspeed);
	m_glidewinddir = m_renderer->get_winddir();
	m_glidewindspeed = m_renderer->get_windspeed();
 	if (get_is_drawable())
		queue_draw();
}

float FlightDeckWindow::NavHSIDrawingArea::get_glidewinddir(void) const
{
	if (!m_renderer)
		return m_glidewinddir;
	return m_renderer->get_winddir();
}

float FlightDeckWindow::NavHSIDrawingArea::get_glidewindspeed(void) const
{
	if (!m_renderer)
		return m_glidewindspeed;
	return m_renderer->get_windspeed();
}

void FlightDeckWindow::NavHSIDrawingArea::set_center(const Point& pt, int alt)
{
        m_center = pt;
        m_altitude = alt;
	m_centervalid = true;
	m_altvalid = true;
        if (!m_renderer)
                return;
        if (m_engine)
                m_renderer->set_maxalt(std::max(m_engine->get_prefs().maxalt * 100, m_altitude));
	double hdg(get_heading());
	if (std::isnan(hdg))
		hdg = 0;
	Glib::TimeVal tv;
	tv.assign_current_time();
        m_renderer->set_center(m_center, m_altitude, hdg * MapRenderer::from_deg_16bit_dbl, tv.tv_sec);
	if (get_is_drawable())
		queue_draw();
        if (!get_visible())
                m_renderer->hide();
}

void FlightDeckWindow::NavHSIDrawingArea::set_center(const Point& pt)
{
        m_center = pt;
	m_centervalid = true;
	m_altvalid = false;
        if (!m_renderer)
                return;
        if (m_engine)
                m_renderer->set_maxalt(std::max(m_engine->get_prefs().maxalt * 100, m_altitude));
	double hdg(get_heading());
	if (std::isnan(hdg))
		hdg = 0;
	Glib::TimeVal tv;
	tv.assign_current_time();
        m_renderer->set_center(m_center, 999999, hdg * MapRenderer::from_deg_16bit_dbl, tv.tv_sec);
	if (get_is_drawable())
		queue_draw();
        if (!get_visible())
                m_renderer->hide();
}

void FlightDeckWindow::NavHSIDrawingArea::set_center(void)
{
	m_centervalid = false;
	m_altvalid = false;
        if (!m_renderer)
                return;
	double hdg(get_heading());
	if (std::isnan(hdg))
		hdg = 0;
	Glib::TimeVal tv;
	tv.assign_current_time();
        m_renderer->set_center(m_center, 999999, hdg * MapRenderer::from_deg_16bit_dbl, tv.tv_sec);
	if (get_is_drawable())
		queue_draw();
        if (!get_visible())
                m_renderer->hide();
}

Point FlightDeckWindow::NavHSIDrawingArea::get_center(void) const
{
        if (!m_renderer)
                return m_center;
        return m_renderer->get_center();
}

int FlightDeckWindow::NavHSIDrawingArea::get_altitude(void) const
{
        if (!m_renderer)
                return m_altitude;
        return m_renderer->get_altitude();
}

FlightDeckWindow::NavHSIDrawingArea::operator MapRenderer::DrawFlags() const
{
        if (!m_renderer)
                return m_drawflags;
        return *m_renderer;
}

FlightDeckWindow::NavHSIDrawingArea& FlightDeckWindow::NavHSIDrawingArea::operator=(MapRenderer::DrawFlags f)
{
        m_drawflags = f;
        if (m_renderer)
                *m_renderer = f;
        return *this;
}

FlightDeckWindow::NavHSIDrawingArea& FlightDeckWindow::NavHSIDrawingArea::operator&=(MapRenderer::DrawFlags f)
{
        m_drawflags &= f;
        if (m_renderer)
                *m_renderer &= f;
        return *this;
}

FlightDeckWindow::NavHSIDrawingArea& FlightDeckWindow::NavHSIDrawingArea::operator|=(MapRenderer::DrawFlags f)
{
        m_drawflags |= f;
        if (m_renderer)
                *m_renderer |= f;
        return *this;
}

FlightDeckWindow::NavHSIDrawingArea& FlightDeckWindow::NavHSIDrawingArea::operator^=(MapRenderer::DrawFlags f)
{
        m_drawflags ^= f;
        if (m_renderer)
                *m_renderer ^= f;
        return *this;
}

void FlightDeckWindow::NavHSIDrawingArea::airspaces_changed(void)
{
        if (m_renderer)
                m_renderer->airspaces_changed();
}

void FlightDeckWindow::NavHSIDrawingArea::airports_changed(void)
{
        if (m_renderer)
                m_renderer->airports_changed();
}

void FlightDeckWindow::NavHSIDrawingArea::navaids_changed(void)
{
        if (m_renderer)
                m_renderer->navaids_changed();
}

void FlightDeckWindow::NavHSIDrawingArea::waypoints_changed(void)
{
        if (m_renderer)
                m_renderer->waypoints_changed();
}

void FlightDeckWindow::NavHSIDrawingArea::airways_changed(void)
{
        if (m_renderer)
                m_renderer->airways_changed();
}

void FlightDeckWindow::NavHSIDrawingArea::zoom_out(void)
{
        if (m_renderer) {
                m_renderer->zoom_out();
                m_mapscale = m_renderer->get_map_scale();
        }
	if (get_is_drawable())
		queue_draw();
}

void FlightDeckWindow::NavHSIDrawingArea::zoom_in(void)
{
        if (m_renderer) {
                m_renderer->zoom_in();
                m_mapscale = m_renderer->get_map_scale();
        }
	if (get_is_drawable())
		queue_draw();
}
