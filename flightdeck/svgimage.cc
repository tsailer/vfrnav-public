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

FlightDeckWindow::SVGImage::SVGImage(BaseObjectType *castitem, const Glib::RefPtr<Builder>& refxml)
        : Gtk::DrawingArea(castitem), m_width(1), m_height(1)

{
#ifndef GLIBMM_DEFAULT_SIGNAL_HANDLERS_ENABLED
#ifdef HAVE_GTKMM2
        signal_expose_event().connect(sigc::mem_fun(*this, &SVGImage::on_expose_event));
        signal_size_request().connect(sigc::mem_fun(*this, &SVGImage::on_size_request));
#endif
#ifdef HAVE_GTKMM3
        signal_draw().connect(sigc::mem_fun(*this, &SVGImage::on_draw));
#endif
        signal_hide().connect(sigc::mem_fun(*this, &SVGImage::on_hide));
#endif //GLIBMM_DEFAULT_SIGNAL_HANDLERS_ENABLED
}

FlightDeckWindow::SVGImage::SVGImage(void)
        : Gtk::DrawingArea()
{
#ifndef GLIBMM_DEFAULT_SIGNAL_HANDLERS_ENABLED
#ifdef HAVE_GTKMM2
        signal_expose_event().connect(sigc::mem_fun(*this, &SVGImage::on_expose_event));
        signal_size_request().connect(sigc::mem_fun(*this, &SVGImage::on_size_request));
#endif
#ifdef HAVE_GTKMM3
        signal_draw().connect(sigc::mem_fun(*this, &SVGImage::on_draw));
#endif
        signal_hide().connect(sigc::mem_fun(*this, &SVGImage::on_hide));
#endif //GLIBMM_DEFAULT_SIGNAL_HANDLERS_ENABLED
}

FlightDeckWindow::SVGImage::~SVGImage()
{
}

void FlightDeckWindow::SVGImage::set(const std::string& data, const std::string& mimetype)
{
	m_data = data;
	m_mimetype = mimetype;
	m_pixbuf = Glib::RefPtr<Gdk::Pixbuf>();
	m_width = 1;
	m_height = 1;
	if (!m_data.empty()) {
		Glib::RefPtr<Gdk::Pixbuf> p;
		try {
			Glib::RefPtr<Gdk::PixbufLoader> pldr;
			if (m_mimetype.empty())
				pldr = Gdk::PixbufLoader::create();
			else
				pldr = Gdk::PixbufLoader::create(m_mimetype, true);
			pldr->write(reinterpret_cast<const guint8 *>(&m_data[0]), m_data.size());
			pldr->close();
			p = pldr->get_pixbuf();
		} catch (const Glib::Error& e) {
			std::cerr << "SVGImage::set: error: " << e.what() << std::endl;
			m_data.clear();
		}
		if (p) {
			m_width = std::max(p->get_width(), 1);
			m_height = std::max(p->get_height(), 1);
		}
	}
	if (get_is_drawable())
                queue_draw();
}

bool FlightDeckWindow::SVGImage::on_draw(const Cairo::RefPtr<Cairo::Context>& cr)
{
        int width, height;
	{
		Gtk::Allocation allocation(get_allocation());
		width = allocation.get_width();
		height = allocation.get_height();
	}
	if (!m_pixbuf || m_pixbuf->get_width() != width || m_pixbuf->get_height() != height) {
		m_pixbuf = Glib::RefPtr<Gdk::Pixbuf>();
		if (!m_data.empty()) {
			try {
				Glib::RefPtr<Gdk::PixbufLoader> pldr;
				if (m_mimetype.empty())
					pldr = Gdk::PixbufLoader::create();
				else
					pldr = Gdk::PixbufLoader::create(m_mimetype, true);
				{
					int w(height * m_width / m_height);
					int h(width * m_height / m_width);
					pldr->set_size(std::min(w, width), std::min(h, height));
				}
				pldr->write(reinterpret_cast<const guint8 *>(&m_data[0]), m_data.size());
				pldr->close();
				m_pixbuf = pldr->get_pixbuf();
			} catch (const Glib::Error& e) {
				std::cerr << "SVGImage::set: error: " << e.what() << std::endl;
				m_data.clear();
			}
		}
	}
	cr->save();
	cr->set_source_rgb(1.0, 1.0, 1.0);
        cr->paint();
	cr->restore();
	if (!m_pixbuf)
                return true;
        cr->save();
        {
                Cairo::RefPtr<Cairo::Context> cr1(Cairo::RefPtr<Cairo::Context>::cast_const(cr));
		Gdk::Point offset((get_width() - m_pixbuf->get_width()) / 2, (get_height() - m_pixbuf->get_height()) / 2);
                Gdk::Cairo::set_source_pixbuf(cr1, m_pixbuf, offset.get_x(), offset.get_y());
        }
        cr->paint();
        cr->restore();
        return true;
}

#ifdef HAVE_GTKMM2
bool FlightDeckWindow::SVGImage::on_expose_event(GdkEventExpose *event)
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

void FlightDeckWindow::SVGImage::on_size_request(Gtk::Requisition *requisition)
{
#ifdef GLIBMM_DEFAULT_SIGNAL_HANDLERS_ENABLED
	Gtk::DrawingArea::on_size_request(requisition);
#endif //GLIBMM_DEFAULT_SIGNAL_HANDLERS_ENABLED
        if (!requisition)
                return;
        requisition->width = m_width;
        requisition->height = m_height;
}
#endif

#ifdef HAVE_GTKMM3
Gtk::SizeRequestMode FlightDeckWindow::SVGImage::get_request_mode_vfunc() const
{
	return Gtk::DrawingArea::get_request_mode_vfunc();
}

void FlightDeckWindow::SVGImage::get_preferred_width_vfunc(int& minimum_width, int& natural_width) const
{
	minimum_width = m_width / 4;
	natural_width = m_width;
}

void FlightDeckWindow::SVGImage::get_preferred_height_for_width_vfunc(int width, int& minimum_height, int& natural_height) const
{
	if (m_width < 1)
		natural_height = 1;
	else
		natural_height = width * m_height / m_width;
	minimum_height = natural_height;
}

void FlightDeckWindow::SVGImage::get_preferred_height_vfunc(int& minimum_height, int& natural_height) const
{
	minimum_height = m_height / 4;
	natural_height = m_height;
}

void FlightDeckWindow::SVGImage::get_preferred_width_for_height_vfunc(int height, int& minimum_width, int& natural_width) const
{
	if (m_height < 1)
		natural_width = 1;
	else
		natural_width = height * m_width / m_height;
	minimum_width = natural_width;
}
#endif

void FlightDeckWindow::SVGImage::on_hide(void)
{
#ifdef GLIBMM_DEFAULT_SIGNAL_HANDLERS_ENABLED
        Gtk::DrawingArea::on_hide();
#endif //GLIBMM_DEFAULT_SIGNAL_HANDLERS_ENABLED
        m_pixbuf = Glib::RefPtr<Gdk::Pixbuf>();
}
