//
// C++ Implementation: FlightDeckWindow
//
// Description: Flight Deck Windows, Route Profile Drawing Area
//
//
// Author: Thomas Sailer <t.sailer@alumni.ethz.ch>, (C) 2011, 2012, 2014
//
// Copyright: See COPYING file that comes with this distribution
//
//

#include "sysdeps.h"

#include "flightdeckwindow.h"

#include <memory>
#include <iomanip>

void FlightDeckWindow::routeprofile_savefileresponse(int response)
{
	m_wxchartexportfilechooser.hide();
	switch (response) {
	case Gtk::RESPONSE_CANCEL:
	default:
		return;

	case Gtk::RESPONSE_OK:
		break;
	}
	std::string path(m_wxchartexportfilechooser.get_filename());
	if (path.empty())
		return;
	if (m_vertprofiledrawingarea)
		m_vertprofiledrawingarea->save(path);
}

const unsigned int FlightDeckWindow::RouteProfileDrawingArea::nrcharts;

FlightDeckWindow::RouteProfileDrawingArea::RouteProfileDrawingArea(BaseObjectType *castitem, const Glib::RefPtr<Builder>& refxml)
	: Gtk::DrawingArea(castitem), m_engine(0), m_route(0), m_center(Point::invalid), m_scaledist(1), m_scaleelev(0.1), m_origindist(0), m_originelev(0),
	  m_scalelon(1e-5), m_scalelat(1e-5), m_dragx(-1), m_dragy(-1), m_chartindex(-1), m_autozoomprofile(true), m_autozoomchart(true),
	  m_ymode(MeteoProfile::yaxis_altitude)
{
#ifndef GLIBMM_DEFAULT_SIGNAL_HANDLERS_ENABLED
#ifdef HAVE_GTKMM2
	signal_expose_event().connect(sigc::mem_fun(*this, &RouteProfileDrawingArea::on_expose_event));
#endif
#ifdef HAVE_GTKMM3
        signal_draw().connect(sigc::mem_fun(*this, &RouteProfileDrawingArea::on_draw));
#endif
        signal_show().connect(sigc::mem_fun(*this, &RouteProfileDrawingArea::on_show));
        signal_hide().connect(sigc::mem_fun(*this, &RouteProfileDrawingArea::on_hide));
        signal_button_press_event().connect(sigc::mem_fun(*this, &RouteProfileDrawingArea::on_button_press_event));
        signal_button_release_event().connect(sigc::mem_fun(*this, &RouteProfileDrawingArea::on_button_release_event));
        signal_motion_notify_event().connect(sigc::mem_fun(*this, &RouteProfileDrawingArea::on_motion_notify_event));
#endif //GLIBMM_DEFAULT_SIGNAL_HANDLERS_ENABLED
	m_dispatch.connect(sigc::mem_fun(*this, &RouteProfileDrawingArea::async_result));
	//set_size_request(128, 128);
	//add_events(Gdk::EXPOSURE_MASK | Gdk::PROPERTY_CHANGE_MASK | Gdk::POINTER_MOTION_MASK | Gdk::POINTER_MOTION_HINT_MASK);
}

FlightDeckWindow::RouteProfileDrawingArea::RouteProfileDrawingArea(void)
	: m_engine(0), m_route(0), m_center(Point::invalid), m_scaledist(1), m_scaleelev(0.1), m_origindist(0), m_originelev(0),
	  m_scalelon(1e-5), m_scalelat(1e-5), m_dragx(-1), m_dragy(-1), m_chartindex(-1), m_autozoomprofile(true), m_autozoomchart(true),
	  m_ymode(MeteoProfile::yaxis_altitude)
{
#ifndef GLIBMM_DEFAULT_SIGNAL_HANDLERS_ENABLED
#ifdef HAVE_GTKMM2
	signal_expose_event().connect(sigc::mem_fun(*this, &RouteProfileDrawingArea::on_expose_event));
#endif
#ifdef HAVE_GTKMM3
        signal_draw().connect(sigc::mem_fun(*this, &RouteProfileDrawingArea::on_draw));
#endif
        signal_show().connect(sigc::mem_fun(*this, &RouteProfileDrawingArea::on_show));
        signal_hide().connect(sigc::mem_fun(*this, &RouteProfileDrawingArea::on_hide));
        signal_button_press_event().connect(sigc::mem_fun(*this, &RouteProfileDrawingArea::on_button_press_event));
        signal_button_release_event().connect(sigc::mem_fun(*this, &RouteProfileDrawingArea::on_button_release_event));
        signal_motion_notify_event().connect(sigc::mem_fun(*this, &RouteProfileDrawingArea::on_motion_notify_event));
#endif //GLIBMM_DEFAULT_SIGNAL_HANDLERS_ENABLED
	m_dispatch.connect(sigc::mem_fun(*this, &RouteProfileDrawingArea::async_result));
	//set_size_request(128, 128);
	//add_events(Gdk::EXPOSURE_MASK | Gdk::PROPERTY_CHANGE_MASK | Gdk::POINTER_MOTION_MASK | Gdk::POINTER_MOTION_HINT_MASK);
}

FlightDeckWindow::RouteProfileDrawingArea::~RouteProfileDrawingArea()
{
	async_cancel();
}

void FlightDeckWindow::RouteProfileDrawingArea::set_engine(Engine& eng)
{
	async_cancel();
	m_engine = &eng;
	m_profile.set_routeprofile();
	m_profile.set_wxprofile();
	m_chart.set_wxdb(eng.get_grib2_db());
	route_change();
	
}

void FlightDeckWindow::RouteProfileDrawingArea::set_route(const FPlanRoute& route)
{
	async_cancel();
	m_route = &route;
	route_change();
}

void FlightDeckWindow::RouteProfileDrawingArea::route_change(void)
{
	async_cancel();
	if (!m_route)
		return;
	m_profile.set_route(*m_route);
	m_profile.set_routeprofile();
	m_profile.set_wxprofile();
	m_chart.set_route(*m_route);
	m_autozoomprofile = m_autozoomchart = true;
	if (!get_is_drawable())
		return;
	async_startquery();
}

void FlightDeckWindow::RouteProfileDrawingArea::async_startquery(void)
{
	if (!m_engine || m_profile.get_route().get_nrwpt() < 2)
		return;
	async_cancel();
	async_clear();
	m_profile.set_routeprofile();
	m_profile.set_wxprofile();
	if (m_chartindex >= 0) {
		if (get_is_drawable())
			queue_draw();
		return;
	}
	m_profilequery = m_engine->async_elevation_profile(m_profile.get_route(), 5.);
	async_connectdone();
	m_profile.set_wxprofile(m_engine->get_grib2_db().get_profile(m_profile.get_route()));
}

double FlightDeckWindow::RouteProfileDrawingArea::get_lon_zoom(void) const
{
	return m_scalelon;
}

void FlightDeckWindow::RouteProfileDrawingArea::set_lon_zoom(double z)
{
	if (m_scalelon == z)
		return;
	m_scalelon = z;
	m_autozoomchart = false;
	if (get_is_drawable())
		queue_draw();
}

double FlightDeckWindow::RouteProfileDrawingArea::get_lat_zoom(void) const
{
	return m_scalelat;
}

void FlightDeckWindow::RouteProfileDrawingArea::set_lat_zoom(double z)
{
	if (m_scalelat == z)
		return;
	m_scalelat = z;
	m_autozoomchart = false;
	if (get_is_drawable())
		queue_draw();
}

void FlightDeckWindow::RouteProfileDrawingArea::latlon_zoom_in(void)
{
	set_lon_zoom(get_lon_zoom() * 2.);
	set_lat_zoom(get_lat_zoom() * 2.);
}

void FlightDeckWindow::RouteProfileDrawingArea::latlon_zoom_out(void)
{
	set_lon_zoom(get_lon_zoom() * 0.5);
	set_lat_zoom(get_lat_zoom() * 0.5);
}

void FlightDeckWindow::RouteProfileDrawingArea::dist_zoom_in(void)
{
	set_dist_zoom(get_dist_zoom() * 2.);
}

void FlightDeckWindow::RouteProfileDrawingArea::dist_zoom_out(void)
{
	set_dist_zoom(get_dist_zoom() * 0.5);
}

double FlightDeckWindow::RouteProfileDrawingArea::get_dist_zoom(void) const
{
	return m_scaledist;
}

void FlightDeckWindow::RouteProfileDrawingArea::set_dist_zoom(double z)
{
	double zz(std::max(std::min(z, 100.), 0.01));
	if (zz == m_scaledist)
		return;
	m_scaledist = zz;
	m_autozoomprofile = false;
	if (get_is_drawable())
		queue_draw();
}

void FlightDeckWindow::RouteProfileDrawingArea::elev_zoom_in(void)
{
	set_elev_zoom(get_elev_zoom() * 2.);
}

void FlightDeckWindow::RouteProfileDrawingArea::elev_zoom_out(void)
{
	if (m_chartindex >= 0) {
		set_lon_zoom(get_lon_zoom() * 0.5);
		set_lat_zoom(get_lat_zoom() * 0.5);
	} else {
		set_elev_zoom(get_elev_zoom() * 0.5);
	}
}

double FlightDeckWindow::RouteProfileDrawingArea::get_elev_zoom(void) const
{
	return m_scaleelev;
}

void FlightDeckWindow::RouteProfileDrawingArea::set_elev_zoom(double z)
{
	double zz(std::max(std::min(z, 10.), 0.001));
	if (zz == m_scaleelev)
		return;
	m_scaleelev = zz;
	m_autozoomprofile = false;
	if (get_is_drawable())
		queue_draw();
}

void FlightDeckWindow::RouteProfileDrawingArea::toggle_chart(void)
{
	m_chartindex = ~m_chartindex;
	if (m_chartindex >= (int)nrcharts)
		m_chartindex = 0;
	if (get_is_drawable())
		queue_draw();
}

void FlightDeckWindow::RouteProfileDrawingArea::next_chart(void)
{
	if (m_chartindex < 0)
		return;
	++m_chartindex;
	if (m_chartindex >= (int)nrcharts)
		m_chartindex = 0;
	if (get_is_drawable())
		queue_draw();
}

void FlightDeckWindow::RouteProfileDrawingArea::prev_chart(void)
{
	if (m_chartindex < 0)
		return;
	if (!m_chartindex)
		m_chartindex = nrcharts;
	--m_chartindex;
	if (get_is_drawable())
		queue_draw();
}

void FlightDeckWindow::RouteProfileDrawingArea::toggle_ymode(void)
{
	switch (m_ymode) {
	default:
		m_ymode = MeteoProfile::yaxis_pressure;
		break;

	case MeteoProfile::yaxis_pressure:
		m_ymode = MeteoProfile::yaxis_altitude;
		break;
	}
	m_scaleelev = m_originelev = std::numeric_limits<double>::quiet_NaN();
	if (m_chartindex < 0 || get_is_drawable())
		queue_draw();
}

#ifdef HAVE_GTKMM2
bool FlightDeckWindow::RouteProfileDrawingArea::on_expose_event(GdkEventExpose *event)
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

bool FlightDeckWindow::RouteProfileDrawingArea::on_draw(const Cairo::RefPtr<Cairo::Context>& cr)
{
	Gtk::Allocation allocation = get_allocation();
	if (std::isnan(m_scaledist) || std::isnan(m_origindist) || m_autozoomprofile) {
		m_origindist = 0;
		m_scaledist = m_profile.get_scaledist(cr, allocation.get_width(), m_profile.get_route().total_distance_nmi_dbl());
	}
	if (std::isnan(m_scaleelev) || std::isnan(m_originelev) || m_autozoomprofile) {
		if (m_ymode == MeteoProfile::yaxis_pressure) {
			m_originelev = IcaoAtmosphere<double>::std_sealevel_pressure;
			m_scaleelev = m_profile.get_scaleelev(cr, allocation.get_height(), 60000, m_ymode);
		} else {
			m_originelev = 0;
			m_scaleelev = m_profile.get_scaleelev(cr, allocation.get_height(), m_profile.get_route().max_altitude() + 4000, m_ymode);
		}
		std::cerr << "originelev " << m_originelev << " scaleelev " << m_scaleelev << std::endl;
	}
	if (m_center.is_invalid() || std::isnan(m_scalelon) || std::isnan(m_scalelat) || m_autozoomchart)
		m_chart.get_fullscale(allocation.get_width(), allocation.get_height(), m_center, m_scalelon, m_scalelat);
	if (m_chartindex >= 0 && m_chartindex < nrcharts)
		m_chart.draw(cr, allocation.get_width(), allocation.get_height(), m_center, m_scalelon, m_scalelat,
			     100.0 * GRIB2::WeatherProfilePoint::isobaric_levels[m_chartindex]);
	else
		m_profile.draw(cr, allocation.get_width(), allocation.get_height(),
			       m_origindist, m_scaledist, m_originelev, m_scaleelev, m_ymode);
	return true;
}

void FlightDeckWindow::RouteProfileDrawingArea::on_size_allocate(Gtk::Allocation& allocation)
{
#ifdef GLIBMM_DEFAULT_SIGNAL_HANDLERS_ENABLED
        Gtk::DrawingArea::on_size_allocate(allocation);
#endif //GLIBMM_DEFAULT_SIGNAL_HANDLERS_ENABLED
}

void FlightDeckWindow::RouteProfileDrawingArea::on_show(void)
{
#ifdef GLIBMM_DEFAULT_SIGNAL_HANDLERS_ENABLED
        Gtk::DrawingArea::on_show();
#endif //GLIBMM_DEFAULT_SIGNAL_HANDLERS_ENABLED
	if (m_profile.get_routeprofile().empty())
	        async_startquery();
}

void FlightDeckWindow::RouteProfileDrawingArea::on_hide(void)
{
#ifdef GLIBMM_DEFAULT_SIGNAL_HANDLERS_ENABLED
        Gtk::DrawingArea::on_hide();
#endif //GLIBMM_DEFAULT_SIGNAL_HANDLERS_ENABLED
	m_dragx = m_dragy = -1;
}

bool FlightDeckWindow::RouteProfileDrawingArea::on_button_press_event(GdkEventButton *event)
{
	if (event->button == 1) {
		m_dragx = event->x;
		m_dragy = event->y;
	}
        if (true)
                std::cerr << "routeprofile button_press " << event->x << ' ' << event->y << ' ' << event->button << std::endl;
#ifdef GLIBMM_DEFAULT_SIGNAL_HANDLERS_ENABLED
        if (Gtk::DrawingArea::on_button_press_event(event))
                return true;
#endif //GLIBMM_DEFAULT_SIGNAL_HANDLERS_ENABLED
        return false;
}

bool FlightDeckWindow::RouteProfileDrawingArea::on_button_release_event(GdkEventButton *event)
{
	m_dragx = m_dragy = -1;
        if (true)
                std::cerr << "routeprofile button_release " << event->x << ' ' << event->y << ' ' << event->button << std::endl;
#ifdef GLIBMM_DEFAULT_SIGNAL_HANDLERS_ENABLED
        if (Gtk::DrawingArea::on_button_release_event(event))
                return true;
#endif //GLIBMM_DEFAULT_SIGNAL_HANDLERS_ENABLED
        return false;
}

bool FlightDeckWindow::RouteProfileDrawingArea::on_motion_notify_event(GdkEventMotion *event)
{
	if (m_dragx >= 0 && m_dragy >= 0) {
		int dx(event->x - m_dragx), dy(event->y - m_dragy);
		m_dragx = event->x;
		m_dragy = event->y;
		if (m_chartindex >= 0) {
			m_center -= Point(dx / m_scalelon, dy / m_scalelat);
			m_autozoomchart = false;
		} else {
			m_origindist -= dx / m_scaledist;
			m_originelev += dy / m_scaleelev;
			if (!m_profile.get_routeprofile().empty()) {
				double dmax(m_profile.get_routeprofile().get_dist());
				Gtk::Allocation allocation = get_allocation();
				dmax -= (allocation.get_width() - 50) / m_scaledist;
				if (m_origindist > dmax)
					m_origindist = dmax;
			}
			if (m_origindist < 0)
				m_origindist = 0;
			if (m_originelev > 60000)
				m_originelev = 60000;
			if (m_originelev < 0)
				m_originelev = 0;
			m_autozoomprofile = false;
		}
		queue_draw();
	}
        if (false)
                std::cerr << "routeprofile motion_notify " << event->x << ' ' << event->y << std::endl;
#ifdef GLIBMM_DEFAULT_SIGNAL_HANDLERS_ENABLED
        if (Gtk::DrawingArea::on_motion_notify_event(event))
                return true;
#endif //GLIBMM_DEFAULT_SIGNAL_HANDLERS_ENABLED
        return false;
}

void FlightDeckWindow::RouteProfileDrawingArea::async_cancel(void)
{
        Engine::ElevationProfileResult::cancel(m_profilequery);
}

void FlightDeckWindow::RouteProfileDrawingArea::async_clear(void)
{
	m_profilequery = Glib::RefPtr<Engine::ElevationRouteProfileResult>();
}

void FlightDeckWindow::RouteProfileDrawingArea::async_connectdone(void)
{
        if (m_profilequery)
                m_profilequery->connect(sigc::mem_fun(*this, &RouteProfileDrawingArea::async_done));
}

void FlightDeckWindow::RouteProfileDrawingArea::async_done(void)
{
	m_dispatch();
}

void FlightDeckWindow::RouteProfileDrawingArea::async_result(void)
{
	if (!m_profilequery || !m_profilequery->is_done())
		return;
	if (m_profilequery->is_error()) {
		m_profilequery = Glib::RefPtr<Engine::ElevationRouteProfileResult>();
		return;
	}
	m_profile.set_routeprofile(m_profilequery->get_result());
	m_profilequery = Glib::RefPtr<Engine::ElevationRouteProfileResult>();
	if (false)
		std::cerr << "RouteProfileDrawingArea: async_result: route " << m_profile.get_route().get_nrwpt()
			  << " terrain " << m_profile.get_routeprofile().size()
			  << " wx " << m_profile.get_wxprofile().size() << std::endl;
	if (get_is_drawable())
		queue_draw();
}

void FlightDeckWindow::RouteProfileDrawingArea::save(const std::string& fn)
{
	if (fn.empty() || m_profile.get_route().get_nrwpt() < 2)
		return;
	double height(595.28), width(841.89);
	Cairo::RefPtr<Cairo::Surface> surface;
	Cairo::RefPtr<Cairo::PdfSurface> pdfsurface;
	if (fn.size() > 4 && !fn.compare(fn.size() - 4, 4, ".svg")) {
		surface = Cairo::SvgSurface::create(fn, width, height);
	} else {
		surface = pdfsurface = Cairo::PdfSurface::create(fn, width, height);
	}
	Cairo::RefPtr<Cairo::Context> ctx(Cairo::Context::create(surface));
	ctx->translate(20, 20);
	{
		static const double maxprofilepagedist(200);
		double pagedist(m_profile.get_route().total_distance_nmi_dbl());
		unsigned int nrpages(1);
		if (!std::isnan(maxprofilepagedist) && maxprofilepagedist > 0 && pagedist > maxprofilepagedist) {
			nrpages = std::ceil(pagedist / maxprofilepagedist);
			pagedist /= nrpages;
		}
		double scaledist(m_profile.get_scaledist(ctx, width - 40, pagedist));
		double scaleelev(m_profile.get_scaleelev(ctx, height - 40, m_profile.get_route().max_altitude() + 4000));
		for (unsigned int pgnr(0); pgnr < nrpages; ++pgnr) {
			m_profile.draw(ctx, width - 40, height - 40, pgnr * pagedist, scaledist, 0, scaleelev);
			ctx->show_page();
			//surface->show_page();
		}
	}
	{
		Point center;
		double scalelon(1e-5), scalelat(1e-5);
		m_chart.get_fullscale(width - 40, height - 40, center, scalelon, scalelat);
		// check whether portrait is the better orientation for this route
		if (pdfsurface) {
			Point center1;
			double scalelon1(1e-5), scalelat1(1e-5);
			m_chart.get_fullscale(height - 40, width - 40, center1, scalelon1, scalelat1);
			if (false)
				std::cerr << "check aspect ratio: scalelat: landscape " << scalelat << " portrait " << scalelat1 << std::endl;
			if (fabs(scalelat1) > fabs(scalelat)) {
				center = center1;
				scalelon = scalelon1;
				scalelat = scalelat1;
				std::swap(width, height);
				pdfsurface->set_size(width, height);
				ctx = Cairo::Context::create(surface);
				ctx->translate(20, 20);
			}
		}
		for (unsigned int i = 0; i < nrcharts; ++i) {
			m_chart.draw(ctx, width - 40, height - 40, center, scalelon, scalelat,
				     100.0 * GRIB2::WeatherProfilePoint::isobaric_levels[i]);
			ctx->show_page();
			//surface->show_page();
		}
	}
	surface->finish();
}
