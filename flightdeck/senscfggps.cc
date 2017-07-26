//
// C++ Implementation: Sensor Configuration Dialog; GPS Widgets
//
// Description: Sensor Configuration Dialog; GPS Widgets
//
//
// Author: Thomas Sailer <t.sailer@alumni.ethz.ch>, (C) 2011, 2012
//
// Copyright: See COPYING file that comes with this distribution
//
//

#include "sysdeps.h"

#include <glibmm.h>

#include "flightdeckwindow.h"

FlightDeckWindow::GPSAzimuthElevation::GPSAzimuthElevation(BaseObjectType *castitem, const Glib::RefPtr<Builder>& refxml)
        : Gtk::DrawingArea(castitem), m_paramfail(Sensors::Sensor::paramfail_ok)
{
#ifndef GLIBMM_DEFAULT_SIGNAL_HANDLERS_ENABLED
#ifdef HAVE_GTKMM2
        signal_expose_event().connect(sigc::mem_fun(*this, &GPSAzimuthElevation::on_expose_event));
#endif
#ifdef HAVE_GTKMM3
        signal_draw().connect(sigc::mem_fun(*this, &GPSAzimuthElevation::on_draw));
#endif
        signal_size_request().connect(sigc::mem_fun(*this, &GPSAzimuthElevation::on_size_request));
        signal_size_allocate().connect(sigc::mem_fun(*this, &GPSAzimuthElevation::on_size_allocate));
        signal_show().connect(sigc::mem_fun(*this, &GPSAzimuthElevation::on_show));
        signal_hide().connect(sigc::mem_fun(*this, &GPSAzimuthElevation::on_hide));
#endif //GLIBMM_DEFAULT_SIGNAL_HANDLERS_ENABLED
}

FlightDeckWindow::GPSAzimuthElevation::GPSAzimuthElevation(void)
        : Gtk::DrawingArea(), m_paramfail(Sensors::Sensor::paramfail_ok)
{
#ifndef GLIBMM_DEFAULT_SIGNAL_HANDLERS_ENABLED
#ifdef HAVE_GTKMM2
        signal_expose_event().connect(sigc::mem_fun(*this, &GPSAzimuthElevation::on_expose_event));
        signal_size_request().connect(sigc::mem_fun(*this, &GPSAzimuthElevation::on_size_request));
#endif
#ifdef HAVE_GTKMM3
        signal_draw().connect(sigc::mem_fun(*this, &GPSAzimuthElevation::on_draw));
#endif
        signal_size_allocate().connect(sigc::mem_fun(*this, &GPSAzimuthElevation::on_size_allocate));
        signal_show().connect(sigc::mem_fun(*this, &GPSAzimuthElevation::on_show));
        signal_hide().connect(sigc::mem_fun(*this, &GPSAzimuthElevation::on_hide));
#endif //GLIBMM_DEFAULT_SIGNAL_HANDLERS_ENABLED
}

FlightDeckWindow::GPSAzimuthElevation::~GPSAzimuthElevation()
{
}

void FlightDeckWindow::GPSAzimuthElevation::update(const Sensors::Sensor::satellites_t& svs)
{
        m_sv = svs;
        if (get_is_drawable())
                queue_draw();
}

void FlightDeckWindow::GPSAzimuthElevation::set_paramfail(Sensors::Sensor::paramfail_t pf)
{
	m_paramfail = pf;
	if (get_is_drawable())
		queue_draw();
}

bool FlightDeckWindow::GPSAzimuthElevation::on_draw(const Cairo::RefPtr<Cairo::Context>& cr)
{
        static const double prn_font = 12;
        static const double windrose_font = 12;
        static const char *windrose_text[4] = { "N", "E", "S", "W" };
        int width, height;
	{
		Gtk::Allocation allocation(get_allocation());
		width = allocation.get_width();
		height = allocation.get_height();
	}
	cr->save();
	cr->set_source_rgb(1.0, 1.0, 1.0);
        cr->paint();
        // draw windrose
        cr->translate(0.5 * width, 0.5 * height);
        double radius = 0.5 * std::min(width, height);
        cr->set_font_size(windrose_font);
        Cairo::TextExtents ext;
        cr->get_text_extents("W", ext);
        radius -= 0.5 * std::max(ext.width, ext.height);
        cr->set_source_rgb(0.85, 0.85, 0.85);
        cr->set_line_width(1.0);
        for (int i = 1; i <= 3; i++) {
                cr->arc(0, 0, radius * i * (1.0 / 3.0), 0, 2.0 * M_PI);
                cr->stroke();
        }
        for (int i = 0; i < 12; i++) {
                double a = i * (M_PI/ 6.0);
                cr->move_to(0, 0);
                cr->line_to(radius * cos(a), radius * sin(a));
                cr->stroke();
        }
        cr->set_source_rgb(0.75, 0.75, 0.75);
        for (int i = 0; i < 4; i++) {
                double a = i * (M_PI/ 2.0);
                cr->save();
                cr->translate(radius * sin(a), -radius * cos(a));
                cr->save();
                cr->set_source_rgb(1.0, 1.0, 1.0);
                cr->rectangle(-0.5 * ext.width, -0.5 * ext.height, ext.width, ext.height);
                cr->fill();
                cr->restore();
                Cairo::TextExtents ext1;
                cr->get_text_extents(windrose_text[i], ext1);
                cr->move_to(-0.5 * ext1.width, 0.5 * ext1.height);
                cr->show_text(windrose_text[i]);
                cr->restore();
        }
        cr->set_source_rgb(0.0, 0.0, 0.0);
        cr->set_line_width(1.0);
        cr->set_font_size(prn_font);
        cr->get_text_extents("88", ext);
        double satradius = sqrt(ext.width * ext.width + ext.height * ext.height) * (0.5 * 1.2);
        double satsail = 1.2 * satradius;
        for (Sensors::Sensor::satellites_t::size_type i = 0; i < m_sv.size(); i++) {
                const Sensors::Sensor::Satellite& sv(m_sv[i]);
                cr->save();
                double a = sv.get_azimuth() * (M_PI / 180.0);
                double r = (90 - sv.get_elevation()) * (1.0 / 90.0) * radius;
                cr->translate(r * sin(a), -r * cos(a));
                cr->begin_new_path();
                cr->save();
                svcolor(cr, sv);
                cr->arc(0, 0, satradius, 0, 2.0 * M_PI);
                if (sv.is_used())
                        cr->fill();
                else
                        cr->stroke();
                cr->move_to(-satsail, 0);
                cr->line_to(-satradius, 0);
                cr->stroke();
                cr->move_to(satsail, 0);
                cr->line_to(satradius, 0);
                cr->stroke();
                cr->move_to(-satsail, -satradius);
                cr->line_to(-satsail, satradius);
                cr->stroke();
                cr->move_to(satsail, -satradius);
                cr->line_to(satsail, satradius);
                cr->stroke();
                cr->restore();
                char buf[8];
                snprintf(buf, sizeof(buf), "%d", sv.get_prn());
                Cairo::TextExtents ext1;
                cr->get_text_extents(buf, ext1);
                cr->move_to(-0.5 * ext1.width, 0.5 * ext1.height);
                cr->show_text(buf);
                cr->restore();
        }
	cr->restore();
	if (m_paramfail != Sensors::Sensor::paramfail_ok) {
		cr->save();
		cr->set_line_width(5.0);
		//cr->set_source_rgba(0.0, 0.0, 0.0, 0.0);
		//cr->set_source_rgb(1.0, 0.0, 0.0);
		//cr->paint();
		cr->set_source_rgba(1.0, 0.0, 0.0, 0.7);
		cr->begin_new_path();
		cr->move_to(2, 2);
		cr->line_to(width - 2, height - 2);
		cr->move_to(2, height - 2);
		cr->line_to(width - 2, 2);
		cr->stroke();
		cr->restore();
	}
        return true;
}

#ifdef HAVE_GTKMM2
bool FlightDeckWindow::GPSAzimuthElevation::on_expose_event(GdkEventExpose *event)
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

void FlightDeckWindow::GPSAzimuthElevation::on_size_request(Gtk::Requisition *requisition)
{
#ifdef GLIBMM_DEFAULT_SIGNAL_HANDLERS_ENABLED
	Gtk::DrawingArea::on_size_request(requisition);
#endif //GLIBMM_DEFAULT_SIGNAL_HANDLERS_ENABLED
        if (!requisition)
                return;
        requisition->width = 256;
        requisition->height = 256;
}
#endif

#ifdef HAVE_GTKMM3
Gtk::SizeRequestMode FlightDeckWindow::GPSAzimuthElevation::get_request_mode_vfunc() const
{
	return Gtk::DrawingArea::get_request_mode_vfunc();
}

void FlightDeckWindow::GPSAzimuthElevation::get_preferred_width_vfunc(int& minimum_width, int& natural_width) const
{
	minimum_width = 256;
	natural_width = 256;
}

void FlightDeckWindow::GPSAzimuthElevation::get_preferred_height_for_width_vfunc(int width, int& minimum_height, int& natural_height) const
{
	minimum_height = natural_height = width;
}

void FlightDeckWindow::GPSAzimuthElevation::get_preferred_height_vfunc(int& minimum_height, int& natural_height) const
{
	minimum_height = 256;
	natural_height = 256;
}

void FlightDeckWindow::GPSAzimuthElevation::get_preferred_width_for_height_vfunc(int height, int& minimum_width, int& natural_width) const
{
	minimum_width = natural_width = height;
}
#endif

void FlightDeckWindow::GPSAzimuthElevation::svcolor(const Cairo::RefPtr<Cairo::Context>& cr, const Sensors::Sensor::Satellite& sv)
{
        int ss = sv.get_snr();
        if (ss < 20) {
                cr->set_source_rgb(0.7, 0.7, 0.7);
                return;
        }
        if (ss < 40) {
                cr->set_source_rgb(1.0, 0.9, 0.0);
                return;
        }
        cr->set_source_rgb(0.0, 1.0, 0.0);
}

FlightDeckWindow::GPSSignalStrength::GPSSignalStrength(BaseObjectType *castitem, const Glib::RefPtr<Builder>& refxml)
        : Gtk::DrawingArea(castitem), m_paramfail(Sensors::Sensor::paramfail_ok)
{
#ifndef GLIBMM_DEFAULT_SIGNAL_HANDLERS_ENABLED
#ifdef HAVE_GTKMM2
        signal_expose_event().connect(sigc::mem_fun(*this, &GPSSignalStrength::on_expose_event));
#endif
#ifdef HAVE_GTKMM3
        signal_draw().connect(sigc::mem_fun(*this, &GPSSignalStrength::on_draw));
#endif
        signal_size_request().connect(sigc::mem_fun(*this, &GPSSignalStrength::on_size_request));
#endif //GLIBMM_DEFAULT_SIGNAL_HANDLERS_ENABLED
}

FlightDeckWindow::GPSSignalStrength::GPSSignalStrength(void)
        : Gtk::DrawingArea(), m_paramfail(Sensors::Sensor::paramfail_ok)
{
#ifndef GLIBMM_DEFAULT_SIGNAL_HANDLERS_ENABLED
#ifdef HAVE_GTKMM2
        signal_expose_event().connect(sigc::mem_fun(*this, &GPSSignalStrength::on_expose_event));
        signal_size_request().connect(sigc::mem_fun(*this, &GPSSignalStrength::on_size_request));
#endif
#ifdef HAVE_GTKMM3
        signal_draw().connect(sigc::mem_fun(*this, &GPSSignalStrength::on_draw));
#endif
#endif //GLIBMM_DEFAULT_SIGNAL_HANDLERS_ENABLED
}

FlightDeckWindow::GPSSignalStrength::~GPSSignalStrength()
{
}

void FlightDeckWindow::GPSSignalStrength::update(const Sensors::Sensor::satellites_t& svs)
{
        m_sv = svs;
	if (get_is_drawable())
                queue_draw();
}

void FlightDeckWindow::GPSSignalStrength::set_paramfail(Sensors::Sensor::paramfail_t pf)
{
	m_paramfail = pf;
	if (get_is_drawable())
		queue_draw();
}

bool FlightDeckWindow::GPSSignalStrength::on_draw(const Cairo::RefPtr<Cairo::Context>& cr)
{
        static const double prn_font = 12;
        static const int max_sat = 14;
        int width, height;
	{
		Gtk::Allocation allocation(get_allocation());
		width = allocation.get_width();
		height = allocation.get_height();
	}
	cr->save();
	cr->set_source_rgb(1.0, 1.0, 1.0);
        cr->paint();
        cr->set_font_size(prn_font);
        Cairo::TextExtents ext;
        cr->get_text_extents("88", ext);
        double gap = ext.width * 0.2;
        double ygap = ext.height * 0.2;
        double barwidth = (width - (max_sat + 1) * gap) * (1.0 / max_sat);
        barwidth = std::max(barwidth, ext.width);
        double bary = ext.height + 2.0 * ygap;
        double barscale = std::max(height - ygap - bary, 10.0) * (1.0 / 60.0);
        bary = height - bary;
        cr->set_source_rgb(0.85, 0.85, 0.85);
        cr->set_line_width(1.0);
        for (int i = 0; i <= 60; i += 20) {
                cr->move_to(0, bary - i * barscale);
                cr->rel_line_to(width, 0);
                cr->stroke();
        }
        for (Sensors::Sensor::satellites_t::size_type i = 0; i < m_sv.size(); i++) {
                const Sensors::Sensor::Satellite& sv(m_sv[i]);
                cr->save();
                //std::cerr << "w " << width << " h " << height << " x " << (i * barwidth + (i + 1) * gap) << " y " << (height - ext.height) << std::endl;
                cr->translate(i * barwidth + (i + 1) * gap, 0);
                cr->set_source_rgb(0.0, 0.0, 0.0);
                char buf[8];
                snprintf(buf, sizeof(buf), "%d", sv.get_prn());
                Cairo::TextExtents ext1;
                cr->get_text_extents(buf, ext1);
                cr->move_to(0.5 * (barwidth - ext1.width), height - ygap);
                cr->show_text(buf);
                GPSAzimuthElevation::svcolor(cr, sv);
                double h = sv.get_snr() * barscale;
                cr->rectangle(0, bary - h, barwidth, h);
                if (sv.is_used())
                        cr->fill();
                else
                        cr->stroke();
                cr->restore();
        }
	cr->restore();
	if (m_paramfail != Sensors::Sensor::paramfail_ok) {
		cr->save();
		cr->set_line_width(5.0);
		//cr->set_source_rgba(0.0, 0.0, 0.0, 0.0);
		//cr->set_source_rgb(1.0, 0.0, 0.0);
		//cr->paint();
		cr->set_source_rgba(1.0, 0.0, 0.0, 0.7);
		cr->begin_new_path();
		cr->move_to(2, 2);
		cr->line_to(width - 2, height - 2);
		cr->move_to(2, height - 2);
		cr->line_to(width - 2, 2);
		cr->stroke();
		cr->restore();
	}
        return true;
}

#ifdef HAVE_GTKMM2
bool FlightDeckWindow::GPSSignalStrength::on_expose_event(GdkEventExpose *event)
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

void FlightDeckWindow::GPSSignalStrength::on_size_request(Gtk::Requisition *requisition)
{
#ifdef GLIBMM_DEFAULT_SIGNAL_HANDLERS_ENABLED
	Gtk::DrawingArea::on_size_request(requisition);
#endif //GLIBMM_DEFAULT_SIGNAL_HANDLERS_ENABLED
        if (!requisition)
                return;
        requisition->width = 256;
        requisition->height = 256;
}
#endif

#ifdef HAVE_GTKMM3
Gtk::SizeRequestMode FlightDeckWindow::GPSSignalStrength::get_request_mode_vfunc() const
{
	return Gtk::DrawingArea::get_request_mode_vfunc();
}

void FlightDeckWindow::GPSSignalStrength::get_preferred_width_vfunc(int& minimum_width, int& natural_width) const
{
	minimum_width = 256;
	natural_width = 256;
}

void FlightDeckWindow::GPSSignalStrength::get_preferred_height_for_width_vfunc(int width, int& minimum_height, int& natural_height) const
{
	minimum_height = natural_height = std::min(width / 2, 256);
}

void FlightDeckWindow::GPSSignalStrength::get_preferred_height_vfunc(int& minimum_height, int& natural_height) const
{
	minimum_height = 256;
	natural_height = 256;
}

void FlightDeckWindow::GPSSignalStrength::get_preferred_width_for_height_vfunc(int height, int& minimum_width, int& natural_width) const
{
	minimum_width = height + height / 2;
	natural_width = height * 2;
}
#endif

FlightDeckWindow::GPSContainer::GPSContainer(BaseObjectType *castitem, const Glib::RefPtr<Builder>& refxml)
	: Gtk::Container(castitem)
{
	set_has_window(false);
	set_redraw_on_allocate(false);
}

FlightDeckWindow::GPSContainer::GPSContainer()
	: Gtk::Container()
{
	set_has_window(false);
	set_redraw_on_allocate(false);
}

FlightDeckWindow::GPSContainer::~GPSContainer()
{
}

#ifdef HAVE_GTKMM2
void FlightDeckWindow::GPSContainer::on_size_request(Gtk::Requisition* requisition)
{
	if (!requisition)
		return;
	requisition->width = requisition->height = 0;
	if (m_children.empty())
		return;
	for (int i = 1; i < (int)m_children.size(); ++i) {
		Gtk::Requisition req(m_children[i]->size_request());
		requisition->width += req.width;
		requisition->height = std::max(requisition->height, req.height);
	}
	Gtk::Requisition req(m_children[0]->size_request());
	requisition->height = std::max(requisition->height, req.height);
	requisition->width += requisition->height;
}
#endif

#ifdef HAVE_GTKMM3
Gtk::SizeRequestMode FlightDeckWindow::GPSContainer::get_request_mode_vfunc() const
{
	return Gtk::SIZE_REQUEST_WIDTH_FOR_HEIGHT;
}

void FlightDeckWindow::GPSContainer::get_preferred_width_vfunc(int& minimum_width, int& natural_width) const
{
	minimum_width = 0;
	natural_width = 0;
	if (m_children.empty())
		return;
	int mwidth(0), nwidth(0);
	for (int i = 0; i < (int)m_children.size(); ++i) {
		int mw(0), nw(0);
		m_children[i]->get_preferred_width(mw, nw);
		mwidth += mw;
		nwidth += nw;
	}
	minimum_width = mwidth;
	natural_width = nwidth;
}

void FlightDeckWindow::GPSContainer::get_preferred_height_for_width_vfunc(int width, int& minimum_height, int& natural_height) const
{
	minimum_height = 0;
	natural_height = 0;
	if (m_children.empty())
		return;
	int width1(width / (int)m_children.size());
	int mheight(0), nheight(0);
	for (int i = 0; i < (int)m_children.size(); ++i) {
		int mh(0), nh(0);
		m_children[i]->get_preferred_height_for_width(width1, mh, nh);
		mheight = std::max(mheight, mh);
		nheight = std::max(nheight, nh);
	}
	minimum_height = mheight;
	natural_height = nheight;
}

void FlightDeckWindow::GPSContainer::get_preferred_height_vfunc(int& minimum_height, int& natural_height) const
{
	minimum_height = 0;
	natural_height = 0;
	if (m_children.empty())
		return;
	int mheight(0), nheight(0);
	for (int i = 0; i < (int)m_children.size(); ++i) {
		int mh(0), nh(0);
		m_children[i]->get_preferred_height(mh, nh);
		mheight = std::max(mheight, mh);
		nheight = std::max(nheight, nh);
	}
	minimum_height = mheight;
	natural_height = nheight;
}

void FlightDeckWindow::GPSContainer::get_preferred_width_for_height_vfunc(int height, int& minimum_width, int& natural_width) const
{
	minimum_width = 0;
	natural_width = 0;
	if (m_children.empty())
		return;
	int mwidth(height), nwidth(height);
	for (int i = 1; i < (int)m_children.size(); ++i) {
		int mw(0), nw(0);
		m_children[i]->get_preferred_width(mw, nw);
		mwidth += mw;
		nwidth += nw;
	}
	minimum_width = mwidth;
	natural_width = nwidth;
}
#endif

void FlightDeckWindow::GPSContainer::on_size_allocate(Gtk::Allocation& allocation)
{
	set_allocation(allocation);
	if (m_children.empty())
		return;
	if (m_children.size() == 1) {
		int w(std::min(allocation.get_height(), allocation.get_width()));
		Gtk::Allocation a(allocation);
		a.set_x(a.get_x() + ((a.get_width() - w) >> 1));
		a.set_y(a.get_y() + ((a.get_height() - w) >> 1));
		a.set_width(w);
		a.set_height(w);
		m_children[0]->size_allocate(a);
		if (false)
			std::cerr << "Child 0: X " << a.get_x() << " Y " << a.get_y() << " W " << a.get_width() << " H " << a.get_height() << std::endl;
		return;
	}
	int w(std::min(allocation.get_height(), allocation.get_width() >> 1));
	{
		Gtk::Allocation a(allocation);
		a.set_y(a.get_y() + ((a.get_height() - w) >> 1));
		a.set_width(w);
		a.set_height(w);
		m_children[0]->size_allocate(a);
		if (false)
			std::cerr << "Child 0: X " << a.get_x() << " Y " << a.get_y() << " W " << a.get_width() << " H " << a.get_height() << std::endl;
	}
	int w2(allocation.get_width() - w);
	w2 /= m_children.size() - 1;
	for (int i = 1; i < (int)m_children.size() - 1; ++i) {
		Gtk::Allocation a(allocation);
		a.set_x(a.get_x() + w);
		a.set_width(w2);
		m_children[i]->size_allocate(a);
		w += w2;
		if (false)
			std::cerr << "Child " << i << ": X " << a.get_x() << " Y " << a.get_y() << " W " << a.get_width() << " H " << a.get_height() << std::endl;
	}
	Gtk::Allocation a(allocation);
	a.set_x(a.get_x() + w);
	a.set_width(allocation.get_width() - w);
	m_children.back()->size_allocate(a);
	if (false)
		std::cerr << "Child " << (m_children.size() - 1) << ": X " << a.get_x() << " Y " << a.get_y() << " W " << a.get_width() << " H " << a.get_height() << std::endl;
}

void FlightDeckWindow::GPSContainer::on_add(Widget* widget)
{
	if (!widget)
		return;
	m_children.push_back(widget);
	m_children.back()->set_parent(*this);
}

void FlightDeckWindow::GPSContainer::on_remove(Widget* widget)
{
	if (!widget)
		return;
	for (int i = 0; i < (int)m_children.size(); ++i) {
		if (m_children[i] != widget)
			continue;
		const bool visible = widget->get_visible();
		m_children.erase(m_children.begin() + i);
		widget->unparent();
		if (visible)
			queue_resize();
		return;
	}
}

GType FlightDeckWindow::GPSContainer::child_type_vfunc() const
{
	if (false && m_children.size() > 2)
		return G_TYPE_NONE;
	return Gtk::Widget::get_type();
}

void FlightDeckWindow::GPSContainer::forall_vfunc(gboolean include_internals, GtkCallback callback, gpointer callback_data)
{
	for (int i = 0; i < (int)m_children.size(); ++i)
		callback(m_children[i]->gobj(), callback_data);
}
