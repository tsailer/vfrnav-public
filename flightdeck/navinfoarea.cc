//
// C++ Implementation: NavInfoArea Widget
//
// Description: NavInfoArea Widget
//
//
// Author: Thomas Sailer <t.sailer@alumni.ethz.ch>, (C) 2011, 2012
//
// Copyright: See COPYING file that comes with this distribution
//
//

#include "sysdeps.h"

#include <iomanip>
#include <glibmm.h>

#include "flightdeckwindow.h"

FlightDeckWindow::NavInfoDrawingArea::NavInfoDrawingArea(BaseObjectType *castitem, const Glib::RefPtr<Builder>& refxml)
	: Gtk::DrawingArea(castitem), m_sensors(0), m_timeroffs(0), m_timersttime(0)
{
#ifndef GLIBMM_DEFAULT_SIGNAL_HANDLERS_ENABLED
#ifdef HAVE_GTKMM2
        signal_expose_event().connect(sigc::mem_fun(*this, &NavInfoDrawingArea::on_expose_event));
#endif
#ifdef HAVE_GTKMM3
        signal_draw().connect(sigc::mem_fun(*this, &NavInfoDrawingArea::on_draw));
#endif
#endif //GLIBMM_DEFAULT_SIGNAL_HANDLERS_ENABLED
}

FlightDeckWindow::NavInfoDrawingArea::NavInfoDrawingArea(void)
	: m_sensors(0), m_timeroffs(0), m_timersttime(0)
{
#ifndef GLIBMM_DEFAULT_SIGNAL_HANDLERS_ENABLED
#ifdef HAVE_GTKMM2
        signal_expose_event().connect(sigc::mem_fun(*this, &NavInfoDrawingArea::on_expose_event));
#endif
#ifdef HAVE_GTKMM3
        signal_draw().connect(sigc::mem_fun(*this, &NavInfoDrawingArea::on_draw));
#endif
#endif //GLIBMM_DEFAULT_SIGNAL_HANDLERS_ENABLED
}

FlightDeckWindow::NavInfoDrawingArea::~NavInfoDrawingArea()
{
}

void FlightDeckWindow::NavInfoDrawingArea::set_sensors(Sensors& sensors)
{
	m_sensors = &sensors;
	if (get_is_drawable())
                queue_draw();
}

void FlightDeckWindow::NavInfoDrawingArea::sensors_change(Sensors *sensors, Sensors::change_t changemask)
{
	if (!sensors || !changemask)
		return;
	if (!(changemask & (Sensors::change_position | Sensors::change_altitude | Sensors::change_course |
			    Sensors::change_navigation | Sensors::change_fplan | Sensors::change_airdata)))
		return;
	if (get_is_drawable())
                queue_draw();
}

bool FlightDeckWindow::NavInfoDrawingArea::timer_redraw(void)
{
	if (get_is_drawable())
                queue_draw();
	return false;
}

time_t FlightDeckWindow::NavInfoDrawingArea::get_timer(void) const
{
	if (!m_timersttime)
		return m_timeroffs;
	return m_timeroffs + time(0) - m_timersttime;
}

void FlightDeckWindow::NavInfoDrawingArea::start_timer(void)
{
	if (m_timersttime)
		return;
	m_timersttime = time(0);
	m_timerredraw.disconnect();
	m_timerredraw = Glib::signal_timeout().connect_seconds(sigc::mem_fun(*this, &NavInfoDrawingArea::timer_redraw), 1);
}

void FlightDeckWindow::NavInfoDrawingArea::stop_timer(void)
{
	if (!m_timersttime)
		return;
	m_timeroffs += time(0) - m_timersttime;
	m_timersttime = 0;
	m_timerredraw.disconnect();
	m_timerredraw = Glib::signal_timeout().connect_seconds(sigc::mem_fun(*this, &NavInfoDrawingArea::timer_redraw), 1);
}

void FlightDeckWindow::NavInfoDrawingArea::clear_timer(void)
{
	m_timeroffs = m_timersttime = 0;
	m_timerredraw.disconnect();
	m_timerredraw = Glib::signal_timeout().connect_seconds(sigc::mem_fun(*this, &NavInfoDrawingArea::timer_redraw), 1);
}

#ifdef HAVE_GTKMM2
bool FlightDeckWindow::NavInfoDrawingArea::on_expose_event(GdkEventExpose *event)
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

bool FlightDeckWindow::NavInfoDrawingArea::on_draw(const Cairo::RefPtr<Cairo::Context>& cr)
{
        static const char figdash[] = "\342\200\223";
	static const double fontsizeinfo = 0.055;
	static const double fontsizelbl = 0.0375;
	static const double margin = 0.01;

	m_timerredraw.disconnect();
        {
                Gtk::Allocation allocation = get_allocation();
                const int height = allocation.get_height();
		if (height > 0) {
			double scale(height);
			cr->scale(scale, scale);
		}
	}
	// font cache
        cr->select_font_face("helvetica", Cairo::FONT_SLANT_NORMAL, Cairo::FONT_WEIGHT_NORMAL);
        Cairo::RefPtr<Cairo::FontFace> font_normal(cr->get_font_face());
        cr->select_font_face("helvetica", Cairo::FONT_SLANT_NORMAL, Cairo::FONT_WEIGHT_BOLD);
        Cairo::RefPtr<Cairo::FontFace> font_bold(cr->get_font_face());
	cr->set_font_size(fontsizeinfo);
	cr->set_font_face(font_bold);
	Cairo::RefPtr<Cairo::ScaledFont> font_infobold(cr->get_scaled_font());
	cr->set_font_face(font_normal);
	Cairo::RefPtr<Cairo::ScaledFont> font_info(cr->get_scaled_font());
	cr->set_font_size(fontsizelbl);
	Cairo::RefPtr<Cairo::ScaledFont> font_lbl(cr->get_scaled_font());
	// paint background
	set_color_background(cr);
	cr->paint();
	// add some margin
	double baselineskip;
	double zerowidth;
	double ycursor;
	{
		cr->set_scaled_font(font_info);
		Cairo::TextExtents ext;
		cr->get_text_extents("0", ext);
		ycursor = 2 * margin - ext.y_bearing;
		baselineskip = 1.5 * ext.height;
		zerowidth = ext.x_advance;
	}
	bool mag(true);
	bool posok(false);
	Point position;
	double decl(0);
	double crsmag(std::numeric_limits<double>::quiet_NaN());
	double crstrue(crsmag);
	double groundspeed(crsmag);
	double ias(crsmag);
	double tas(crsmag);
	double baroalt(crsmag);
	double truealt(crsmag);
	double altrate(crsmag);
	double tkoffldgspeed(10);
	double fuel(crsmag);
	double fuelflow(crsmag);
	double bhppercent(crsmag);
	if (m_sensors) {
		mag = !m_sensors->is_manualheading_true();
		posok = m_sensors->get_position(position);
		decl = m_sensors->get_declination();
		m_sensors->get_course(crsmag, crstrue, groundspeed);
		m_sensors->get_altitude(baroalt, truealt, altrate);
		tkoffldgspeed = m_sensors->nav_get_tkoffldgspeed();
		ias = m_sensors->get_airdata_cas();
		tas = m_sensors->get_airdata_tas();
		fuel = m_sensors->get_acft_fuel();
		fuelflow = m_sensors->get_acft_fuelflow();
		bhppercent = 100.0 * m_sensors->get_acft_bhp() / m_sensors->get_aircraft().get_maxbhp();
		if (!std::isnan(bhppercent))
			bhppercent = std::max(std::min(bhppercent, 100.0), 0.0);
	}
	if (std::isnan(decl)) {
		decl = 0;
		mag = false;
	}
	// draw waypoint info
	set_color_marking(cr);
	if (m_sensors && m_sensors->nav_is_on()) {
		cr->set_scaled_font(font_info);
		cr->move_to(margin, ycursor);
		Glib::ustring txt(m_sensors->nav_get_curdest());
		cr->show_text(txt);
		ycursor += baselineskip;
		cr->move_to(margin + 2 * zerowidth, ycursor);
		cr->set_scaled_font(font_lbl);
		cr->show_text("D");
		cr->set_scaled_font(font_info);
		{
			double d(m_sensors->nav_get_curdist());
			txt = Glib::ustring::format(std::fixed, std::setprecision(1), d);
			if (std::isnan(d)) {
				txt = figdash;
				txt += txt + "." + txt;
			}
		}
		Cairo::TextExtents ext;
		cr->get_text_extents(txt, ext);
		Cairo::TextExtents ext2;
		cr->get_text_extents("88.8", ext2);
		cr->rel_move_to(ext2.x_advance - ext.x_advance, 0);
		cr->show_text(txt);
		cr->rel_move_to(zerowidth, 0);
		cr->set_scaled_font(font_lbl);
		cr->show_text("BRG");
		cr->set_scaled_font(font_info);
		{
			double brg(position.spheric_true_course(m_sensors->nav_get_curcoord()));
			if (mag)
				brg -= decl;
			if (brg < 0)
				brg += 360;
			else if (brg >= 360)
				brg -= 360;
			txt = Glib::ustring::format(std::fixed, std::setprecision(0), brg);
			if (!posok) {
				txt = figdash;
				txt += txt + txt;
			}
		}
		cr->get_text_extents(txt, ext);
		cr->get_text_extents("888", ext2);
		cr->rel_move_to(ext2.x_advance - ext.x_advance, 0);
		cr->show_text(txt);
		cr->rel_move_to(zerowidth, 0);
		ycursor += baselineskip;
		cr->move_to(margin + 2 * zerowidth, ycursor);
		cr->set_scaled_font(font_lbl);
		cr->show_text("VS");
		cr->set_scaled_font(font_info);
		{
			double roc(m_sensors->nav_get_roc());
			txt = Glib::ustring::format(std::fixed, std::setprecision(0), roc);
			if (std::isnan(roc)) {
				txt = figdash;
				txt += txt + txt;
			}
		}
		cr->get_text_extents(txt, ext);
		cr->get_text_extents("-8888", ext2);
		cr->rel_move_to(ext2.x_advance - ext.x_advance, 0);
		cr->show_text(txt);
		cr->rel_move_to(zerowidth, 0);
		cr->set_scaled_font(font_lbl);
		cr->show_text("DTK");
		cr->set_scaled_font(font_info);
		{
			double dtk(mag ? m_sensors->nav_get_track_mag() : m_sensors->nav_get_track_true());
			txt = Glib::ustring::format(std::fixed, std::setprecision(0), dtk);
			if (std::isnan(dtk)) {
				txt = figdash;
				txt += txt + txt;
			}
		}
		cr->get_text_extents(txt, ext);
		cr->get_text_extents("888", ext2);
		cr->rel_move_to(ext2.x_advance - ext.x_advance, 0);
		cr->show_text(txt);
		cr->rel_move_to(zerowidth, 0);
		ycursor += baselineskip;
		cr->move_to(margin + 2 * zerowidth, ycursor);
		cr->set_scaled_font(font_lbl);
		cr->show_text(m_sensors->nav_is_fpl() ? "FPL" : m_sensors->nav_is_directto() ? "D" : m_sensors->nav_is_finalapproach() ? "FAS" : "");
		cr->get_text_extents("FPL", ext);
		cr->move_to(margin + 3 * zerowidth + ext.x_advance, ycursor);
		if (m_sensors->nav_is_hold()) {
			cr->set_scaled_font(font_lbl);
			cr->show_text("HLD");
			cr->set_scaled_font(font_info);
			{
				double trk(m_sensors->nav_get_hold_track());
				if (mag)
					trk -= decl;
				if (trk < 0)
					trk += 360;
				else if (trk >= 360)
					trk -= 360;
				txt = Glib::ustring::format(std::fixed, std::setprecision(0), trk);
				if (std::isnan(trk)) {
					txt = figdash;
					txt += txt + txt;
				}
			}
			cr->get_text_extents(txt, ext);
			cr->get_text_extents("888", ext2);
			cr->rel_move_to(ext2.x_advance - ext.x_advance, 0);
			cr->show_text(txt);
		}
		ycursor += baselineskip;
		cr->move_to(margin + 2 * zerowidth, ycursor);
		cr->set_scaled_font(font_lbl);
		cr->show_text("TIME");
		cr->set_scaled_font(font_info);
		{
			double t(m_sensors->nav_get_timetowpt());
			int sec(Point::round<int,double>(t));
			int min(sec / 60);
			sec -= 60 * min;
			if (sec < 0) {
				sec += 60;
				min -= 1;
			}
			int hour(min / 60);
			min -= 60 * hour;
			if (min < 0) {
				min += 60;
				hour -= 1;
			}
			txt = Glib::ustring::format(std::setw(2), std::setfill(L'0'), hour, L':') +
				Glib::ustring::format(std::setw(2), std::setfill(L'0'), min, L':') +
				Glib::ustring::format(std::setw(2), std::setfill(L'0'), sec);
			if (std::isnan(t)) {
				txt = figdash;
				txt += txt + ":" + txt + txt + ":" + txt + txt;
			}
		}
		cr->get_text_extents(txt, ext);
		cr->get_text_extents("88:88:88", ext2);
		cr->rel_move_to(ext2.x_advance - ext.x_advance, 0);
		cr->show_text(txt);
		cr->rel_move_to(zerowidth, 0);
		ycursor += baselineskip;
	}  else {
		ycursor += 5 * baselineskip;
	}
	ycursor += 0.5 * baselineskip;
	if (m_sensors && m_sensors->get_route().get_nrwpt() > 0) {
		const FPlanRoute& fpl(m_sensors->get_route());
		const FPlanWaypoint& wpt(fpl[fpl.get_nrwpt() - 1]);
		cr->set_scaled_font(font_info);
		cr->move_to(margin, ycursor);
		Glib::ustring txt(wpt.get_icao_name());
		cr->show_text(txt);
		ycursor += baselineskip;
		cr->move_to(margin + 2 * zerowidth, ycursor);
		cr->set_scaled_font(font_lbl);
		cr->show_text("D");
		cr->set_scaled_font(font_info);
		double dist(position.spheric_distance_nmi_dbl(wpt.get_coord()));
		{
			txt = Glib::ustring::format(std::fixed, std::setprecision(1), dist);
			if (!posok || std::isnan(dist)) {
				txt = figdash;
				txt += txt + "." + txt;
			}
		}
		Cairo::TextExtents ext;
		cr->get_text_extents(txt, ext);
		Cairo::TextExtents ext2;
		cr->get_text_extents("88.8", ext2);
		cr->rel_move_to(ext2.x_advance - ext.x_advance, 0);
		cr->show_text(txt);
		cr->rel_move_to(zerowidth, 0);
		cr->set_scaled_font(font_lbl);
		cr->show_text("BRG");
		cr->set_scaled_font(font_info);
		{
			double brg(position.spheric_true_course(wpt.get_coord()));
			if (mag)
				brg -= decl;
			if (brg < 0)
				brg += 360;
			else if (brg >= 360)
				brg -= 360;
			txt = Glib::ustring::format(std::fixed, std::setprecision(0), brg);
			if (!posok) {
				txt = figdash;
				txt += txt + txt;
			}
		}
		cr->get_text_extents(txt, ext);
		cr->get_text_extents("888", ext2);
		cr->rel_move_to(ext2.x_advance - ext.x_advance, 0);
		cr->show_text(txt);
		cr->rel_move_to(zerowidth, 0);
		ycursor += baselineskip;
		cr->move_to(margin + 2 * zerowidth, ycursor);
		cr->set_scaled_font(font_lbl);
		cr->show_text("VS");
		cr->set_scaled_font(font_info);
		{
			double roc(wpt.get_altitude());
			if (wpt.get_flags() & FPlanWaypoint::altflag_standard)
				roc = m_sensors->pressure_to_true_altitude(roc);
			roc -= truealt;
			if (!posok || std::isnan(dist) || std::isnan(groundspeed) || groundspeed < tkoffldgspeed)
				roc = std::numeric_limits<double>::quiet_NaN();
			else
				roc *= groundspeed / dist * (1.0 / 60);
			txt = Glib::ustring::format(std::fixed, std::setprecision(0), roc);
			if (std::isnan(roc)) {
				txt = figdash;
				txt += txt + txt;
			}
		}
		cr->get_text_extents(txt, ext);
		cr->get_text_extents("-8888", ext2);
		cr->rel_move_to(ext2.x_advance - ext.x_advance, 0);
		cr->show_text(txt);
		ycursor += baselineskip;
		cr->move_to(margin + 2 * zerowidth, ycursor);
		cr->set_scaled_font(font_lbl);
		cr->show_text("TIME");
		cr->set_scaled_font(font_info);
		{
			double t(std::numeric_limits<double>::quiet_NaN());
			if (!std::isnan(groundspeed) && groundspeed >= tkoffldgspeed)
				t = dist / groundspeed * 3600;
			int sec(Point::round<int,double>(t));
			int min(sec / 60);
			sec -= 60 * min;
			if (sec < 0) {
				sec += 60;
				min -= 1;
			}
			int hour(min / 60);
			min -= 60 * hour;
			if (min < 0) {
				min += 60;
				hour -= 1;
			}
			txt = Glib::ustring::format(std::setw(2), std::setfill(L'0'), hour, L':') +
				Glib::ustring::format(std::setw(2), std::setfill(L'0'), min, L':') +
				Glib::ustring::format(std::setw(2), std::setfill(L'0'), sec);
			if (std::isnan(t)) {
				txt = figdash;
				txt += txt + ":" + txt + txt + ":" + txt + txt;
			}
		}
		cr->get_text_extents(txt, ext);
		cr->get_text_extents("88:88:88", ext2);
		cr->rel_move_to(ext2.x_advance - ext.x_advance, 0);
		cr->show_text(txt);
		cr->rel_move_to(zerowidth, 0);
		ycursor += baselineskip;
	}  else {
		ycursor += 4 * baselineskip;
	}
	ycursor += 0.5 * baselineskip;
	if (m_sensors) {
		cr->move_to(margin, ycursor);
		cr->set_scaled_font(font_lbl);
		cr->show_text("WPT TIME");
		Cairo::TextExtents ext;
		cr->get_text_extents("FLIGHT TIME ", ext);
		cr->move_to(margin + ext.x_advance, ycursor);
		cr->set_scaled_font(font_info);
		Glib::ustring txt;
		{
			time_t t0(m_sensors->nav_get_lastwpttime());
			int sec(time(0) - t0);
			int min(sec / 60);
			sec -= 60 * min;
			if (sec < 0) {
				sec += 60;
				min -= 1;
			}
			txt = Glib::ustring::format(min, L':', std::setw(2), std::setfill(L'0'), sec);
			if (!t0) {
				txt = figdash;
				txt += txt + ":" + txt + txt;
			}
		}
		cr->get_text_extents(txt, ext);
		Cairo::TextExtents ext2;
		cr->get_text_extents("88:88:88", ext2);
		cr->rel_move_to(ext2.x_advance - ext.x_advance, 0);
		cr->show_text(txt);
		cr->rel_move_to(zerowidth, 0);
	}
	ycursor += baselineskip;
	// Flight Timer
	if (m_sensors) {
		cr->move_to(margin, ycursor);
		cr->set_scaled_font(font_lbl);
		cr->show_text("FLIGHT TIME ");
		cr->set_scaled_font(font_info);
		if (m_sensors->nav_is_flighttime_running())
			set_color_activetimer(cr);
		Glib::ustring txt;
		{
			int sec(m_sensors->nav_get_flighttime());
			int min(sec / 60);
			sec -= 60 * min;
			if (sec < 0) {
				sec += 60;
				min -= 1;
			}
			int hour(min / 60);
			min -= 60 * hour;
			if (min < 0) {
				min += 60;
				hour -= 1;
			}
			txt = Glib::ustring::format(std::setw(2), std::setfill(L'0'), hour, L':') +
				Glib::ustring::format(std::setw(2), std::setfill(L'0'), min, L':') +
				Glib::ustring::format(std::setw(2), std::setfill(L'0'), sec);
		}
		cr->show_text(txt);
	}
	ycursor += 1.5 * baselineskip;
	set_color_marking(cr);
	// Timer
	{
		cr->move_to(margin, ycursor);
		cr->set_scaled_font(font_lbl);
		cr->show_text("TIMER ");
		cr->show_text(is_timer_running() ? "UP" : "STOP");
		Cairo::TextExtents ext;
		cr->get_text_extents("FLIGHT TIME ", ext);
		cr->move_to(margin + ext.x_advance, ycursor);
		Glib::ustring txt;
		cr->set_scaled_font(font_info);
		if (is_timer_running())
			set_color_activetimer(cr);
		{
			int sec(get_timer());
			int min(sec / 60);
			sec -= 60 * min;
			if (sec < 0) {
				sec += 60;
				min -= 1;
			}
			int hour(min / 60);
			min -= 60 * hour;
			if (min < 0) {
				min += 60;
				hour -= 1;
			}
			txt = Glib::ustring::format(std::setw(2), std::setfill(L'0'), hour, L':') +
				Glib::ustring::format(std::setw(2), std::setfill(L'0'), min, L':') +
				Glib::ustring::format(std::setw(2), std::setfill(L'0'), sec);
		}
		cr->show_text(txt);
	}
	ycursor += 1.5 * baselineskip;
	set_color_marking(cr);
	// Ground Speed
	{
		cr->move_to(margin, ycursor);
		Cairo::TextExtents ext;
		Cairo::TextExtents ext2;
		cr->get_text_extents("888", ext2);
		Glib::ustring txt;
		if (false) {
			cr->set_scaled_font(font_lbl);
			cr->show_text("GS ");
			cr->set_scaled_font(font_info);
			if (std::isnan(groundspeed)) {
				txt = figdash;
				txt += txt + txt;
			} else {
				txt = Glib::ustring::format(std::fixed, std::setw(3), std::setprecision(0), groundspeed);
			}
			cr->get_text_extents(txt, ext);
			cr->rel_move_to(ext2.x_advance - ext.x_advance, 0);
			cr->show_text(txt);
			cr->rel_move_to(zerowidth, 0);
		}
		cr->set_scaled_font(font_lbl);
		cr->show_text("IAS ");
		cr->set_scaled_font(font_info);
		if (std::isnan(ias)) {
			txt = figdash;
			txt += txt + txt;
		} else {
			txt = Glib::ustring::format(std::fixed, std::setw(3), std::setprecision(0), ias);
		}
		cr->get_text_extents(txt, ext);
		cr->rel_move_to(ext2.x_advance - ext.x_advance, 0);
		cr->show_text(txt);
		cr->rel_move_to(zerowidth, 0);
		cr->set_scaled_font(font_lbl);
		cr->show_text("TAS ");
		cr->set_scaled_font(font_info);
		if (std::isnan(tas)) {
			txt = figdash;
			txt += txt + txt;
		} else {
			txt = Glib::ustring::format(std::fixed, std::setw(3), std::setprecision(0), tas);
		}
		cr->get_text_extents(txt, ext);
		cr->rel_move_to(ext2.x_advance - ext.x_advance, 0);
		cr->show_text(txt);
	}
	ycursor += baselineskip;
	// Fuel
	{
		cr->move_to(margin, ycursor);
		cr->set_scaled_font(font_lbl);
		cr->show_text("F ");
		Glib::ustring txt;
		cr->set_scaled_font(font_info);
		if (std::isnan(fuel)) {
			txt = figdash;
			txt += txt + "." + txt;
		} else if (fuel > 99.9) {
			txt = Glib::ustring::format(std::fixed, std::setw(3), std::setprecision(0), fuel);
		} else if (fuel > 9.9) {
			txt = Glib::ustring::format(std::fixed, std::setw(4), std::setprecision(1), fuel);
		} else {
			txt = Glib::ustring::format(std::fixed, std::setw(4), std::setprecision(2), fuel);
		}
		Cairo::TextExtents ext;
		cr->get_text_extents(txt, ext);
		Cairo::TextExtents ext2;
		cr->get_text_extents("888.", ext2);
		cr->rel_move_to(ext2.x_advance - ext.x_advance, 0);
		cr->show_text(txt);
		cr->rel_move_to(zerowidth, 0);
		cr->set_scaled_font(font_lbl);
		cr->show_text("FF ");
		cr->set_scaled_font(font_info);
		if (std::isnan(fuelflow)) {
			txt = figdash;
			txt += txt + "." + txt;
		} else if (fuelflow > 99.9) {
			txt = Glib::ustring::format(std::fixed, std::setw(3), std::setprecision(0), fuelflow);
		} else if (fuelflow > 9.9) {
			txt = Glib::ustring::format(std::fixed, std::setw(4), std::setprecision(1), fuelflow);
		} else {
			txt = Glib::ustring::format(std::fixed, std::setw(4), std::setprecision(2), fuelflow);
		}
		cr->get_text_extents(txt, ext);
		cr->rel_move_to(ext2.x_advance - ext.x_advance, 0);
		cr->show_text(txt);
		cr->rel_move_to(zerowidth, 0);
		cr->set_scaled_font(font_info);
		if (std::isnan(bhppercent)) {
			txt = figdash;
			txt += txt;
		} else {
			txt = Glib::ustring::format(std::fixed, std::setw(2), std::setprecision(0), bhppercent);
		}
		txt += "%";
		cr->get_text_extents(txt, ext);
		cr->get_text_extents("88%", ext2);
		cr->rel_move_to(ext2.x_advance - ext.x_advance, 0);
		cr->show_text(txt);
	}
	ycursor += baselineskip;

	if (is_timer_running() || (m_sensors && m_sensors->nav_is_flighttime_running()))
		m_timerredraw = Glib::signal_timeout().connect_seconds(sigc::mem_fun(*this, &NavInfoDrawingArea::timer_redraw), 1);

	return true;
}
