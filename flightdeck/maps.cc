/***************************************************************************
 *   Copyright (C) 2007, 2008, 2009, 2011, 2012, 2013, 2015                *
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

#include <sys/time.h>
#include <sstream>
#include <iomanip>
#include <gdk/gdkkeysyms.h>
#include <set>

#include "flightdeckwindow.h"

FlightDeckWindow::MapDrawingArea::MapDrawingArea(BaseObjectType * castitem, const Glib::RefPtr< Builder > & refxml)
        : Gtk::DrawingArea(castitem), m_engine(0), m_renderer(0), m_rendertype(renderer_none),
          m_dragbutton(2), m_dragstart(0, 0), m_dragoffset(0, 0), m_draginprogress(false),
          m_cursorvalid(false), m_cursoranglevalid(false), m_cursorangle(0),
          m_time(0), m_altitude(0), m_mapscale(0.1), m_glideslope(std::numeric_limits<float>::quiet_NaN()),
	  m_tas(std::numeric_limits<float>::quiet_NaN()), m_winddir(0), m_windspeed(0),
	  m_route(0), m_track(0), m_drawflags(MapRenderer::drawflags_none)
{
        set_map_scale(0.1);
#ifndef GLIBMM_DEFAULT_SIGNAL_HANDLERS_ENABLED
#ifdef HAVE_GTKMM2
        signal_expose_event().connect(sigc::mem_fun(*this, &MapDrawingArea::on_expose_event));
#endif
#ifdef HAVE_GTKMM3
        signal_draw().connect(sigc::mem_fun(*this, &MapDrawingArea::on_draw));
#endif
        signal_size_allocate().connect(sigc::mem_fun(*this, &MapDrawingArea::on_size_allocate));
        signal_show().connect(sigc::mem_fun(*this, &MapDrawingArea::on_show));
        signal_hide().connect(sigc::mem_fun(*this, &MapDrawingArea::on_hide));
        signal_button_press_event().connect(sigc::mem_fun(*this, &MapDrawingArea::on_button_press_event));
        signal_button_release_event().connect(sigc::mem_fun(*this, &MapDrawingArea::on_button_release_event));
        signal_motion_notify_event().connect(sigc::mem_fun(*this, &MapDrawingArea::on_motion_notify_event));
#endif //GLIBMM_DEFAULT_SIGNAL_HANDLERS_ENABLED
}

FlightDeckWindow::MapDrawingArea::MapDrawingArea(void)
        : m_engine(0), m_renderer(0), m_rendertype(renderer_none),
          m_dragbutton(2), m_dragstart(0, 0), m_dragoffset(0, 0), m_draginprogress(false),
          m_cursorvalid(false), m_cursoranglevalid(false), m_cursorangle(0),
          m_time(0), m_altitude(0), m_mapscale(0.1), m_glideslope(std::numeric_limits<float>::quiet_NaN()),
	  m_tas(std::numeric_limits<float>::quiet_NaN()), m_winddir(0), m_windspeed(0),
	  m_route(0), m_track(0), m_drawflags(MapRenderer::drawflags_none)
{
        set_map_scale(0.1);
#ifndef GLIBMM_DEFAULT_SIGNAL_HANDLERS_ENABLED
#ifdef HAVE_GTKMM2
        signal_expose_event().connect(sigc::mem_fun(*this, &MapDrawingArea::on_expose_event));
#endif
#ifdef HAVE_GTKMM3
        signal_draw().connect(sigc::mem_fun(*this, &MapDrawingArea::on_draw));
#endif
        signal_size_allocate().connect(sigc::mem_fun(*this, &MapDrawingArea::on_size_allocate));
        signal_show().connect(sigc::mem_fun(*this, &MapDrawingArea::on_show));
        signal_hide().connect(sigc::mem_fun(*this, &MapDrawingArea::on_hide));
        signal_button_press_event().connect(sigc::mem_fun(*this, &MapDrawingArea::on_button_press_event));
        signal_button_release_event().connect(sigc::mem_fun(*this, &MapDrawingArea::on_button_release_event));
        signal_motion_notify_event().connect(sigc::mem_fun(*this, &MapDrawingArea::on_motion_notify_event));
#endif //GLIBMM_DEFAULT_SIGNAL_HANDLERS_ENABLED
}

FlightDeckWindow::MapDrawingArea::~MapDrawingArea()
{
	m_mapaircraft.clear();
	m_mapacftconn.disconnect();
	m_mapacftexpconn.disconnect();
	m_connupdate.disconnect();
        delete m_renderer;
}

void FlightDeckWindow::MapDrawingArea::set_engine(Engine& eng)
{
        m_engine = &eng;
        renderer_alloc();
}

void FlightDeckWindow::MapDrawingArea::set_renderer(renderer_t render)
{
        m_rendertype = render;
        renderer_alloc();
}

FlightDeckWindow::MapDrawingArea::renderer_t FlightDeckWindow::MapDrawingArea::get_renderer(void) const
{
        return m_rendertype;
}

bool FlightDeckWindow::MapDrawingArea::is_renderer_enabled(renderer_t rnd)
{
	return VectorMapArea::is_renderer_enabled((VectorMapArea::renderer_t)rnd);
}

VectorMapRenderer::renderer_t FlightDeckWindow::MapDrawingArea::convert_renderer(renderer_t render)
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
		{ renderer_google_street, VectorMapRenderer::renderer_google_street },
		{ renderer_google_satellite, VectorMapRenderer::renderer_google_satellite },
		{ renderer_google_hybrid, VectorMapRenderer::renderer_google_hybrid },
		{ renderer_virtual_earth_street, VectorMapRenderer::renderer_virtual_earth_street },
		{ renderer_virtual_earth_satellite, VectorMapRenderer::renderer_virtual_earth_satellite },
		{ renderer_virtual_earth_hybrid, VectorMapRenderer::renderer_virtual_earth_hybrid },
		{ renderer_osmc_trails, VectorMapRenderer::renderer_osmc_trails },
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

void FlightDeckWindow::MapDrawingArea::renderer_alloc(void)
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
		Glib::TimeVal tv;
		tv.assign_current_time();		
                m_connupdate = m_dispatch.connect(sigc::mem_fun(*this, &FlightDeckWindow::MapDrawingArea::renderer_update));
                m_renderer->set_maxalt(m_engine->get_prefs().maxalt * 100);
                m_renderer->set_route(m_route);
                m_renderer->set_track(m_track);
                m_renderer->set_center(m_center, m_altitude, 0, tv.tv_sec);
                m_renderer->set_map_scale(m_mapscale);
		m_renderer->set_glideslope((m_rendertype == renderer_terrain) ? m_glideslope : std::numeric_limits<float>::quiet_NaN());
		m_renderer->set_tas(m_tas);
		m_renderer->set_wind(m_winddir, m_windspeed);
                *m_renderer = m_drawflags;
                m_renderer->set_screensize(MapRenderer::ScreenCoord(get_width(), get_height()));
		m_renderer->set_grib2(m_grib2layer);
                m_renderer->set_bitmap(m_map);
        }
}

void FlightDeckWindow::MapDrawingArea::set_route(const FPlanRoute& route)
{
        m_route = &route;
        if (!m_renderer)
                return;
        m_renderer->set_route(m_route);
}

void FlightDeckWindow::MapDrawingArea::set_track(const TracksDb::Track *track)
{
        m_track = track;
        if (!m_renderer)
                return;
        m_renderer->set_track(m_track);
}

void FlightDeckWindow::MapDrawingArea::renderer_update(void)
{
	if (get_is_drawable())
		queue_draw();
}

void FlightDeckWindow::MapDrawingArea::redraw_route(void)
{
        if (!m_renderer)
                return;
        m_renderer->redraw_route();
	if (get_is_drawable())
		queue_draw();
}

void FlightDeckWindow::MapDrawingArea::redraw_track(void)
{
        if (!m_renderer)
                return;
        m_renderer->redraw_track();
	if (get_is_drawable())
		queue_draw();
}

#ifdef HAVE_GTKMM2
bool FlightDeckWindow::MapDrawingArea::on_expose_event(GdkEventExpose *event)
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

bool FlightDeckWindow::MapDrawingArea::on_draw(const Cairo::RefPtr<Cairo::Context>& cr)
{
        if (!m_renderer || !cr)
                return false;
        m_renderer->draw(cr, m_dragoffset);
	draw_aircraft(cr);
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

void FlightDeckWindow::MapDrawingArea::on_size_allocate(Gtk::Allocation & allocation)
{
#ifdef GLIBMM_DEFAULT_SIGNAL_HANDLERS_ENABLED
	Gtk::DrawingArea::on_size_allocate(allocation);
#endif //GLIBMM_DEFAULT_SIGNAL_HANDLERS_ENABLED
        if (m_renderer)
                m_renderer->set_screensize(MapRenderer::ScreenCoord(allocation.get_width(), allocation.get_height()));
        //std::cerr << "map size allocate" << std::endl;
}

void FlightDeckWindow::MapDrawingArea::on_show(void)
{
#ifdef GLIBMM_DEFAULT_SIGNAL_HANDLERS_ENABLED
	Gtk::DrawingArea::on_show();
#endif //GLIBMM_DEFAULT_SIGNAL_HANDLERS_ENABLED
        std::cerr << "vmaparea: show" << std::endl;
        if (m_renderer)
                m_renderer->show();
}

void FlightDeckWindow::MapDrawingArea::on_hide(void)
{
#ifdef GLIBMM_DEFAULT_SIGNAL_HANDLERS_ENABLED
	Gtk::DrawingArea::on_hide();
#endif //GLIBMM_DEFAULT_SIGNAL_HANDLERS_ENABLED
        std::cerr << "vmaparea: hide" << std::endl;
        if (m_renderer)
                m_renderer->hide();
}

unsigned int FlightDeckWindow::MapDrawingArea::button_translate(GdkEventButton * event)
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

bool FlightDeckWindow::MapDrawingArea::on_button_press_event(GdkEventButton * event)
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

bool FlightDeckWindow::MapDrawingArea::on_button_release_event(GdkEventButton * event)
{
        MapRenderer::ScreenCoordFloat sc(event->x, event->y);
        unsigned int button = button_translate(event);
        if (m_draginprogress && (button == m_dragbutton)) {
                if (m_renderer) {
                        Point pt = m_renderer->screen_coord(m_dragstart) - m_renderer->screen_coord(sc);
			Glib::TimeVal tv;
			tv.assign_current_time();		
			m_renderer->set_center(m_renderer->get_center() + pt, m_renderer->get_altitude(), 0, tv.tv_sec);
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

bool FlightDeckWindow::MapDrawingArea::on_motion_notify_event(GdkEventMotion * event)
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

void FlightDeckWindow::MapDrawingArea::pan_up(void)
{
        if (m_renderer) {
                m_renderer->pan_up();
                m_center = m_renderer->get_center();
        }
        redraw();
}

void FlightDeckWindow::MapDrawingArea::pan_down(void)
{
        if (m_renderer) {
                m_renderer->pan_down();
                m_center = m_renderer->get_center();
        }
        redraw();
}

void FlightDeckWindow::MapDrawingArea::pan_left(void)
{
        if (m_renderer) {
                m_renderer->pan_left();
                m_center = m_renderer->get_center();
        }
        redraw();
}

void FlightDeckWindow::MapDrawingArea::pan_right(void)
{
        if (m_renderer) {
                m_renderer->pan_right();
                m_center = m_renderer->get_center();
        }
        redraw();
}

void FlightDeckWindow::MapDrawingArea::set_map_scale(float nmi_per_pixel)
{
        m_mapscale = nmi_per_pixel;
        if (!m_renderer)
                return;
        m_renderer->set_map_scale(m_mapscale);
        m_mapscale = m_renderer->get_map_scale();
        redraw();
}

float FlightDeckWindow::MapDrawingArea::get_map_scale(void) const
{
        if (!m_renderer)
                return m_mapscale;
        return m_renderer->get_map_scale();
}

void FlightDeckWindow::MapDrawingArea::set_glideslope(float gs)
{
	if (gs <= 0)
		gs = std::numeric_limits<float>::quiet_NaN();
	m_glideslope = gs;
	if (m_renderer && m_rendertype == renderer_terrain)
		m_renderer->set_glideslope(m_glideslope);
        redraw();
}

float FlightDeckWindow::MapDrawingArea::get_glideslope(void) const
{
	if (m_renderer && m_rendertype == renderer_terrain)
		return m_renderer->get_glideslope();
	return m_glideslope;
}

void FlightDeckWindow::MapDrawingArea::set_tas(float tas)
{
	if (tas <= 0)
		tas = std::numeric_limits<float>::quiet_NaN();
	m_tas = tas;
	if (m_renderer)
		m_renderer->set_tas(m_tas);
        redraw();
}

float FlightDeckWindow::MapDrawingArea::get_tas(void) const
{
	if (m_renderer)
		return m_renderer->get_tas();
	return m_tas;
}

void FlightDeckWindow::MapDrawingArea::set_wind(float dir, float speed)
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

float FlightDeckWindow::MapDrawingArea::get_winddir(void) const
{
	if (m_renderer)
		return m_renderer->get_winddir();
	return m_winddir;
}

float FlightDeckWindow::MapDrawingArea::get_windspeed(void) const
{
	if (m_renderer)
		return m_renderer->get_windspeed();
	return m_windspeed;
}

void FlightDeckWindow::MapDrawingArea::set_center(const Point & pt, int alt, uint16_t upangle, int64_t time)
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

Point FlightDeckWindow::MapDrawingArea::get_center(void) const
{
        if (!m_renderer)
                return m_center;
        return m_renderer->get_center();
}

int FlightDeckWindow::MapDrawingArea::get_altitude(void) const
{
        if (!m_renderer)
                return m_altitude;
        return m_renderer->get_altitude();
}

uint16_t FlightDeckWindow::MapDrawingArea::get_upangle(void) const
{
        if (!m_renderer)
                return m_upangle;
        return m_renderer->get_upangle();
}

int64_t FlightDeckWindow::MapDrawingArea::get_time(void) const
{
        if (!m_renderer)
                return m_time;
        return m_renderer->get_time();
}

void FlightDeckWindow::MapDrawingArea::set_dragbutton(unsigned int dragbutton)
{
        m_dragbutton = dragbutton;
        m_dragoffset = MapRenderer::ScreenCoord(0, 0);
        m_draginprogress = false;
}

FlightDeckWindow::MapDrawingArea::operator MapRenderer::DrawFlags() const
{
        if (!m_renderer)
                return m_drawflags;
        return *m_renderer;
}

FlightDeckWindow::MapDrawingArea& FlightDeckWindow::MapDrawingArea::operator =(MapRenderer::DrawFlags f)
{
        m_drawflags = f;
        if (m_renderer)
                *m_renderer = f;
        return *this;
}

FlightDeckWindow::MapDrawingArea& FlightDeckWindow::MapDrawingArea::operator &=(MapRenderer::DrawFlags f)
{
        m_drawflags &= f;
        if (m_renderer)
                *m_renderer &= f;
        return *this;
}

FlightDeckWindow::MapDrawingArea& FlightDeckWindow::MapDrawingArea::operator |=(MapRenderer::DrawFlags f)
{
        m_drawflags |= f;
        if (m_renderer)
                *m_renderer |= f;
        return *this;
}

FlightDeckWindow::MapDrawingArea& FlightDeckWindow::MapDrawingArea::operator ^=(MapRenderer::DrawFlags f)
{
        m_drawflags ^= f;
        if (m_renderer)
                *m_renderer ^= f;
        return *this;
}

void FlightDeckWindow::MapDrawingArea::set_grib2(const Glib::RefPtr<GRIB2::Layer>& layer)
{
	m_grib2layer = layer;
	if (m_renderer)
		m_renderer->set_grib2(m_grib2layer);
}

void FlightDeckWindow::MapDrawingArea::set_bitmap(const Glib::RefPtr<BitmapMaps::Map>& map)
{
	m_map = map;
	if (m_renderer)
                m_renderer->set_bitmap(m_map);
}

void FlightDeckWindow::MapDrawingArea::airspaces_changed(void)
{
        if (m_renderer)
                m_renderer->airspaces_changed();
}

void FlightDeckWindow::MapDrawingArea::airports_changed(void)
{
        if (m_renderer)
                m_renderer->airports_changed();
}

void FlightDeckWindow::MapDrawingArea::navaids_changed(void)
{
        if (m_renderer)
                m_renderer->navaids_changed();
}

void FlightDeckWindow::MapDrawingArea::waypoints_changed(void)
{
        if (m_renderer)
                m_renderer->waypoints_changed();
}

void FlightDeckWindow::MapDrawingArea::airways_changed(void)
{
        if (m_renderer)
                m_renderer->airways_changed();
}

void FlightDeckWindow::MapDrawingArea::invalidate_cursor(void)
{
        if (m_cursorvalid) {
                m_cursorvalid = false;
                redraw();
                signal_cursor()(m_cursor, cursor_invalid);
        }
}

void FlightDeckWindow::MapDrawingArea::set_cursor(const Point & pt)
{
         m_cursor = pt;
         m_cursorvalid = true;
         redraw();
         signal_cursor()(m_cursor, cursor_set);
}

Rect FlightDeckWindow::MapDrawingArea::get_cursor_rect(void) const
{
        if (!m_cursorvalid || !m_renderer)
                return Rect();
        MapRenderer::ScreenCoord sc(m_renderer->coord_screen(m_cursor));
        return Rect(m_renderer->screen_coord(sc + MapRenderer::ScreenCoord(-5, 5)),
                    m_renderer->screen_coord(sc + MapRenderer::ScreenCoord(5, -5)));
}

void FlightDeckWindow::MapDrawingArea::set_cursor_angle(float angle)
{
        m_cursorangle = Point::round<uint16_t,float>(angle * MapRenderer::from_deg_16bit);
        m_cursoranglevalid = true;
        redraw();
}

void FlightDeckWindow::MapDrawingArea::invalidate_cursor_angle(void)
{
        m_cursorangle = 0;
        if (m_cursoranglevalid) {
                m_cursoranglevalid = false;
                redraw();
        }
}

void FlightDeckWindow::MapDrawingArea::zoom_out(void)
{
        if (m_renderer) {
                m_renderer->zoom_out();
                m_mapscale = m_renderer->get_map_scale();
        }
        redraw();
}

void FlightDeckWindow::MapDrawingArea::zoom_in(void)
{
        if (m_renderer) {
                m_renderer->zoom_in();
                m_mapscale = m_renderer->get_map_scale();
        }
        redraw();
}

void FlightDeckWindow::MapDrawingArea::set_aircraft(const Sensors::MapAircraft& acft)
{
	bool chg(false);
	if (acft.is_coord_valid() && acft.is_timestamp_valid()) {
		std::pair<mapaircraft_t::iterator,bool> ins(m_mapaircraft.insert(acft));
		bool achg(ins.second);
		if (!achg && ins.first->is_changed(acft)) {
			const_cast<Sensors::MapAircraft&>(*ins.first) = acft;
			achg = true;
		}
		if (false) {
			std::ostringstream oss;
			oss << std::hex << std::setfill('0') << std::setw(6) << acft.get_icaoaddr() << std::dec;
			if (acft.is_registration_valid())
				oss << ' ' << acft.get_registration();
			if (acft.is_ident_valid())
				oss << ' ' << acft.get_ident();
			if (acft.is_timestamp_valid())
				oss << ' ' << acft.get_timestamp().as_iso8601();
			if (acft.is_coord_valid())
				oss << ' ' << acft.get_coord().get_lat_str2() << ' ' << acft.get_coord().get_lon_str2();
			if (acft.is_baroalt_valid())
				oss << ' ' << acft.get_baroalt() << '\'';
			if (acft.is_truealt_valid())
				oss << ' ' << acft.get_truealt() << '\'';
			if (acft.is_vs_valid())
				oss << ' ' << acft.get_vs() << "ft/min";
			if (acft.is_modea_valid())
				oss << " A" << (char)('0'+((acft.get_modea() >> 9) & 7)) << (char)('0'+((acft.get_modea() >> 6) & 7))
				    << (char)('0'+((acft.get_modea() >> 3) & 7)) << (char)('0'+(acft.get_modea() & 7));
			if (acft.is_crs_speed_valid())
				oss << ' ' << Point::round<int,double>(acft.get_crs() * MapRenderer::to_deg_16bit_dbl)
				    << ' ' << acft.get_speed() << "kts";
			if (ins.second)
				oss << " (new)";
			else if (achg)
				oss << " (update)";
			std::cerr << "MapDrawingArea::set_aircraft: " << oss.str() << std::endl;
		}
		chg = chg || achg;
	}
	chg = expire_aircraft() || chg;
	if (chg)
		schedule_aircraft_redraw();
}

void FlightDeckWindow::MapDrawingArea::clear_aircraft(void)
{
	bool e(m_mapaircraft.empty());
	m_mapaircraft.clear();
	m_mapacftconn.disconnect();
	m_mapacftexpconn.disconnect();
	if (!e)
		schedule_aircraft_redraw();
}

void FlightDeckWindow::MapDrawingArea::draw_aircraft(const Cairo::RefPtr<Cairo::Context>& cr)
{
	m_mapacftredraw.assign_current_time();
	expire_aircraft();
	if (false)
		std::cerr << "MapDrawingArea::draw_aircraft: " << m_mapacftredraw.as_iso8601()
			  << " nr acft: " << m_mapaircraft.size() << std::endl;
	cr->save();
	cr->select_font_face("helvetica", Cairo::FONT_SLANT_NORMAL, Cairo::FONT_WEIGHT_NORMAL);
        cr->set_font_size(14);
	Cairo::TextExtents ext;
	cr->get_text_extents("0", ext);
	for (mapaircraft_t::const_iterator i(m_mapaircraft.begin()), e(m_mapaircraft.end()); i != e; ++i) {
		const Sensors::MapAircraft& acft(*i);
                MapRenderer::ScreenCoord sc(m_renderer->coord_screen(acft.get_coord()));
		cr->save();
		cr->translate(sc.getx(), sc.gety());
		if (false) {
			std::ostringstream oss;
			oss << std::hex << std::setfill('0') << std::setw(6) << acft.get_icaoaddr() << std::dec;
			if (acft.is_registration_valid())
				oss << ' ' << acft.get_registration();
			if (acft.is_ident_valid())
				oss << ' ' << acft.get_ident();
			if (acft.is_timestamp_valid())
				oss << ' ' << acft.get_timestamp().as_iso8601();
			if (acft.is_coord_valid())
				oss << ' ' << acft.get_coord().get_lat_str2() << ' ' << acft.get_coord().get_lon_str2();
			if (acft.is_baroalt_valid())
				oss << ' ' << acft.get_baroalt() << '\'';
			if (acft.is_truealt_valid())
				oss << ' ' << acft.get_truealt() << '\'';
			if (acft.is_vs_valid())
				oss << ' ' << acft.get_vs() << "ft/min";
			if (acft.is_modea_valid())
				oss << " A" << (char)('0'+((acft.get_modea() >> 9) & 7)) << (char)('0'+((acft.get_modea() >> 6) & 7))
				    << (char)('0'+((acft.get_modea() >> 3) & 7)) << (char)('0'+(acft.get_modea() & 7));
			if (acft.is_crs_speed_valid())
				oss << ' ' << Point::round<int,double>(acft.get_crs() * MapRenderer::to_deg_16bit_dbl)
				    << ' ' << acft.get_speed() << "kts";
			oss << " X " << sc.getx() << " Y " << sc.gety();
			std::cerr << "MapDrawingArea::draw_aircraft: " << oss.str() << std::endl;
		}
		if (acft.is_crs_speed_valid()) {
			cr->save();
			cr->rotate((acft.get_crs() - m_upangle) * MapRenderer::to_rad_16bit_dbl);
			cr->set_line_width(2);
			cr->begin_new_sub_path();
			cr->move_to(0.0, -15.0);
			cr->line_to(7.5, 12.990);
			cr->line_to(0.0, 7.5);
			cr->line_to(-7.5, 12.990);
			cr->close_path();
			// fill targets red when altitude is within 2500'
			{
				bool crisk(acft.is_truealt_valid());
				if (crisk) {
					int alt(acft.get_truealt());
					alt -= get_altitude();
					if (abs(alt) > 2500)
						crisk = false;
				}
				cr->set_source_rgba(1.0, crisk ? 0.0 : 1.0, 0.0, 0.5);
			}
			cr->fill_preserve();
			cr->set_source_rgb(0.0, 0.0, 0.0);
			cr->stroke();
			cr->restore();
		} else {
			cr->arc(0.0, 0.0, 15.0, 0.0, 2.0 * M_PI);
			cr->set_source_rgba(1.0, 1.0, 0.0, 0.5);
			cr->fill_preserve();
			cr->set_source_rgb(0.0, 0.0, 0.0);
			cr->stroke();
		}
		double baselineskip(ext.height * 1.3), x(18 - ext.x_bearing), y(-baselineskip - ext.y_bearing);
		std::string t(acft.get_line1());
		if (!t.empty()) {
			cr->set_source_rgb(1.0, 1.0, 1.0);
			cr->move_to(x, y - 2);
			cr->show_text(t);
			cr->move_to(x, y + 2);
			cr->show_text(t);
			cr->move_to(x - 2, y);
			cr->show_text(t);
			cr->move_to(x + 2, y);
			cr->show_text(t);
			cr->set_source_rgb(0.0, 0.0, 0.0);
			cr->move_to(x, y);
			cr->show_text(t);
		}
		y += baselineskip;
		t = acft.get_line2();
		if (!t.empty()) {
			cr->set_source_rgb(1.0, 1.0, 1.0);
			cr->move_to(x, y - 2);
			cr->show_text(t);
			cr->move_to(x, y + 2);
			cr->show_text(t);
			cr->move_to(x - 2, y);
			cr->show_text(t);
			cr->move_to(x + 2, y);
			cr->show_text(t);
			cr->set_source_rgb(0.0, 0.0, 0.0);
			cr->move_to(x, y);
			cr->show_text(t);
		}
		cr->restore();
	}
	cr->restore();
}

bool FlightDeckWindow::MapDrawingArea::expire_aircraft(void)
{
	m_mapacftexpconn.disconnect();
	bool chg(false);
	Glib::TimeVal tv;
	tv.assign_current_time();
	tv.subtract_seconds(15);
	Glib::TimeVal tvexp(std::numeric_limits<glong>::max(), 0);
	for (mapaircraft_t::iterator i(m_mapaircraft.begin()), e(m_mapaircraft.end()); i != e;) {
		mapaircraft_t::iterator i2(i);
		++i;
		Glib::TimeVal tvdiff(i2->get_timestamp() - tv);
		if (tvdiff <= Glib::TimeVal(0, 0)) {
			m_mapaircraft.erase(i2);
			chg = true;
			continue;
		}
		if (tvdiff < tvexp)
			tvexp = tvdiff;
	}
	if (!m_mapaircraft.empty())
		m_mapacftexpconn = Glib::signal_timeout().connect(sigc::bind_return(sigc::hide_return(sigc::mem_fun(*this, &MapDrawingArea::expire_aircraft)), false),
								  tvexp.tv_sec + (tvexp.tv_usec + 999) / 1000);
	if (false && chg)
		std::cerr << "MapDrawingArea::expire_aircraft: acft expired, remaining " << m_mapaircraft.size() << std::endl;
	return chg;
}

void FlightDeckWindow::MapDrawingArea::schedule_aircraft_redraw(void)
{
	m_mapacftconn.disconnect();
	if (!get_is_drawable()) {
		if (false)
			std::cerr << "MapDrawingArea::schedule_aircraft_redraw: not drawable" << std::endl;
		m_mapacftredraw.assign_current_time();
		return;
	}
	Glib::TimeVal tv;
	tv.assign_current_time();
	tv = m_mapacftredraw - tv;
	tv.add_seconds(1);
	if (tv <= Glib::TimeVal(0, 0)) {
		if (false)
			std::cerr << "MapDrawingArea::schedule_aircraft_redraw: queue draw" << std::endl;
		queue_draw();
		return;
	}
	m_mapacftconn = Glib::signal_timeout().connect(sigc::bind_return(sigc::mem_fun(*this, &MapDrawingArea::queue_draw), false), tv.tv_sec + (tv.tv_usec + 999) / 1000);
}
