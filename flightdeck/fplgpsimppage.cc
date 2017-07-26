/***************************************************************************
 *   Copyright (C) 2007, 2008, 2009, 2011, 2012, 2013 by Thomas Sailer     *
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

#include <iomanip>

#include "flightdeckwindow.h"

FlightDeckWindow::GPSImportWaypointListModelColumns::GPSImportWaypointListModelColumns(void)
{
	add(m_icao);
	add(m_name);
	add(m_freq);
	add(m_lon);
	add(m_lat);
	add(m_mt);
	add(m_tt);
	add(m_dist);
	add(m_nr);
	add(m_weight);
	add(m_style);
}

class FlightDeckWindow::FPLGPSImpQuery {
public:
	FPLGPSImpQuery(Engine *eng);
	~FPLGPSImpQuery();
	void wait(void);
	void sort(const Point& pt);
	void cancel(void);
	void query(const Point& pt, bool arpt = false, bool navaid = false, bool wpt = false);
	const AirportsDb::elementvector_t& get_arpts(void) const;
	const NavaidsDb::elementvector_t& get_navaids(void) const;
	const WaypointsDb::elementvector_t& get_wpts(void) const;

protected:
	static const AirportsDb::elementvector_t nullarpt;
	static const NavaidsDb::elementvector_t nullnavaid;
	static const WaypointsDb::elementvector_t nullwpt;
	Engine *m_engine;
	Glib::RefPtr<Engine::AirportResult> m_queryarpt;
	Glib::RefPtr<Engine::NavaidResult> m_querynavaid;
	Glib::RefPtr<Engine::WaypointResult> m_querywpt;
	Glib::Mutex m_mutex;
	Glib::Cond m_cond;

	void notify(void);

	struct sorter {
		Point m_pt;
		sorter(const Point& pt) : m_pt(pt) {}
		bool operator()(const AirportsDb::element_t& a, const AirportsDb::element_t& b) const {
			return m_pt.spheric_distance_nmi(a.get_coord()) < m_pt.spheric_distance_nmi(b.get_coord());
		}
		bool operator()(const NavaidsDb::element_t& a, const NavaidsDb::element_t& b) const {
			return m_pt.spheric_distance_nmi(a.get_coord()) < m_pt.spheric_distance_nmi(b.get_coord());
		}
		bool operator()(const WaypointsDb::element_t& a, const WaypointsDb::element_t& b) const {
			return m_pt.spheric_distance_nmi(a.get_coord()) < m_pt.spheric_distance_nmi(b.get_coord());
		}
	};
};

const AirportsDb::elementvector_t FlightDeckWindow::FPLGPSImpQuery::nullarpt;
const NavaidsDb::elementvector_t FlightDeckWindow::FPLGPSImpQuery::nullnavaid;
const WaypointsDb::elementvector_t FlightDeckWindow::FPLGPSImpQuery::nullwpt;

FlightDeckWindow::FPLGPSImpQuery::FPLGPSImpQuery(Engine *eng)
	: m_engine(eng)
{
}

FlightDeckWindow::FPLGPSImpQuery::~FPLGPSImpQuery()
{
	cancel();
}

void FlightDeckWindow::FPLGPSImpQuery::notify(void)
{
	m_cond.broadcast();
}

void FlightDeckWindow::FPLGPSImpQuery::wait(void)
{
	Glib::Mutex::Lock lock(m_mutex);
	for (;;) {
		bool ok(true);
		if (m_queryarpt && !m_queryarpt->is_done() && !m_queryarpt->is_error())
			ok = false;
		if (ok && m_querynavaid && !m_querynavaid->is_done() && !m_querynavaid->is_error())
			ok = false;
		if (ok && m_querywpt && !m_querywpt->is_done() && !m_querywpt->is_error())
			ok = false;
		if (ok)
			break;
		m_cond.wait(m_mutex);
	}
}

void FlightDeckWindow::FPLGPSImpQuery::cancel(void)
{
	if (m_queryarpt) {
                Glib::RefPtr<Engine::AirportResult> q(m_queryarpt);
                m_queryarpt.reset();
		Engine::AirportResult::cancel(q);
	}
	if (m_querynavaid) {
                Glib::RefPtr<Engine::NavaidResult> q(m_querynavaid);
                m_querynavaid.reset();
		Engine::NavaidResult::cancel(q);
	}
	if (m_querywpt) {
                Glib::RefPtr<Engine::WaypointResult> q(m_querywpt);
                m_querywpt.reset();
		Engine::WaypointResult::cancel(q);
	}
}

void FlightDeckWindow::FPLGPSImpQuery::query(const Point& pt, bool arpt, bool navaid, bool wpt)
{
	cancel();
	if (!m_engine)
		return;
	Rect wnd(pt.simple_box_nmi(1));
	if (arpt) {
		m_queryarpt = m_engine->async_airport_find_nearest(pt, ~0U, wnd,
								   AirportsDb::element_t::subtables_vfrroutes |
								   AirportsDb::element_t::subtables_runways |
								   AirportsDb::element_t::subtables_helipads |
								   AirportsDb::element_t::subtables_comms);
		if (m_queryarpt)
			m_queryarpt->connect(sigc::mem_fun(*this, &FPLGPSImpQuery::notify));
	}
	if (navaid) {
		m_querynavaid = m_engine->async_navaid_find_nearest(pt, ~0U, wnd, NavaidsDb::element_t::subtables_none);
		if (m_querynavaid)
			m_querynavaid->connect(sigc::mem_fun(*this, &FPLGPSImpQuery::notify));
	}
	if (wpt) {
		m_querywpt = m_engine->async_waypoint_find_nearest(pt, ~0U, wnd, WaypointsDb::element_t::subtables_none);
		if (m_querywpt)
			m_querywpt->connect(sigc::mem_fun(*this, &FPLGPSImpQuery::notify));
	}
}

void FlightDeckWindow::FPLGPSImpQuery::sort(const Point& pt)
{
	sorter s(pt);
	if (m_queryarpt && m_queryarpt->is_done() && !m_queryarpt->is_error()) {
		AirportsDb::elementvector_t& r(m_queryarpt->get_result());
		std::sort(r.begin(), r.end(), s);
	}
	if (m_querynavaid && m_querynavaid->is_done() && !m_querynavaid->is_error()) {
		NavaidsDb::elementvector_t& r(m_querynavaid->get_result());
		std::sort(r.begin(), r.end(), s);
	}
	if (m_querywpt && m_querywpt->is_done() && !m_querywpt->is_error()) {
		WaypointsDb::elementvector_t& r(m_querywpt->get_result());
		std::sort(r.begin(), r.end(), s);
	}	
}

const AirportsDb::elementvector_t& FlightDeckWindow::FPLGPSImpQuery::get_arpts(void) const
{
	if (m_queryarpt && m_queryarpt->is_done() && !m_queryarpt->is_error())
		return m_queryarpt->get_result();
	return nullarpt;
}

const NavaidsDb::elementvector_t& FlightDeckWindow::FPLGPSImpQuery::get_navaids(void) const
{
	if (m_querynavaid && m_querynavaid->is_done() && !m_querynavaid->is_error())
		return m_querynavaid->get_result();
	return nullnavaid;
}

const WaypointsDb::elementvector_t& FlightDeckWindow::FPLGPSImpQuery::get_wpts(void) const
{
	if (m_querywpt && m_querywpt->is_done() && !m_querywpt->is_error())
		return m_querywpt->get_result();
	return nullwpt;
}

void FlightDeckWindow::fplgpsimppage_selection_changed(void)
{
        if (m_coorddialog)
                m_coorddialog->hide();
        if (!m_fplgpsimptreeview)
                return;
        Glib::RefPtr<Gtk::TreeSelection> selection(m_fplgpsimptreeview->get_selection());
        if (!selection)
                return;
        Gtk::TreeModel::iterator iter(selection->get_selected());
        if (!iter)
                return;
        Gtk::TreeModel::Row row(*iter);
        unsigned int nr(row[m_fplgpsimpcolumns.m_nr]);
	if (nr < 1 || nr > m_fplgpsimproute.get_nrwpt())
		return;
	const FPlanWaypoint& wpt(m_fplgpsimproute[nr - 1]);
	if (m_fplgpsimpdrawingarea) {
		Glib::TimeVal tv;
		tv.assign_current_time();		
 		m_fplgpsimpdrawingarea->set_center(wpt.get_coord(), wpt.get_altitude(), 0, tv.tv_sec);
		m_fplgpsimpdrawingarea->set_cursor(wpt.get_coord());
	}
}

bool FlightDeckWindow::fplgpsimppage_loadfpl(void)
{
	if (!m_sensors)
		return false;
        FPlanRoute& fpl(m_sensors->get_route());
	fpl.new_fp();
	for (unsigned int nr = 0; nr < m_fplgpsimproute.get_nrwpt(); ++nr)
		fpl.insert_wpt(~0U, m_fplgpsimproute[nr]);
	fpl.set_note(m_fplgpsimproute.get_note());
	fpl.set_curwpt(m_fplgpsimproute.get_curwpt());
	fpl.set_time_offblock(m_fplgpsimproute.get_time_offblock());
	fpl.set_time_onblock(m_fplgpsimproute.get_time_onblock());
	fpl.save_fp();
	m_sensors->nav_fplan_modified();
	return true;
}

bool FlightDeckWindow::fplgpsimppage_brg2(void)
{
        if (!m_fplgpsimptreeview || !m_sensors)
                return false;
        Glib::RefPtr<Gtk::TreeSelection> selection(m_fplgpsimptreeview->get_selection());
        if (!selection)
                return false;
        Gtk::TreeModel::iterator iter(selection->get_selected());
        if (!iter)
                return false;
        Gtk::TreeModel::Row row(*iter);
        unsigned int nr(row[m_fplgpsimpcolumns.m_nr]);
	if (nr < 1 || nr > m_fplgpsimproute.get_nrwpt())
		return false;
	const FPlanWaypoint& wpt(m_fplgpsimproute[nr - 1]);
	m_sensors->nav_set_brg2(wpt.get_icao_name());
	m_sensors->nav_set_brg2(wpt.get_coord());
	m_sensors->nav_set_brg2(true);
	return true;
}

bool FlightDeckWindow::fplgpsimppage_directto(void)
{
        if (!m_fplgpsimptreeview || !m_coorddialog)
                return false;
        Glib::RefPtr<Gtk::TreeSelection> selection(m_fplgpsimptreeview->get_selection());
        if (!selection)
                return false;
        Gtk::TreeModel::iterator iter(selection->get_selected());
        if (!iter)
                return false;
        Gtk::TreeModel::Row row(*iter);
        unsigned int nr(row[m_fplgpsimpcolumns.m_nr]);
	if (nr < 1 || nr > m_fplgpsimproute.get_nrwpt())
		return false;
	const FPlanWaypoint& wpt(m_fplgpsimproute[nr - 1]);
        m_coorddialog->set_flags(m_navarea.map_get_drawflags());
        if (m_sensors &&
            m_sensors->get_configfile().has_key(cfggroup_mainwindow, NavArea::cfgkey_mainwindow_mapscale) &&
            m_sensors->get_configfile().has_key(cfggroup_mainwindow, NavArea::cfgkey_mainwindow_arptmapscale))
                m_coorddialog->set_map_scale(m_sensors->get_configfile().get_double(cfggroup_mainwindow, NavArea::cfgkey_mainwindow_mapscale),
                                             m_sensors->get_configfile().get_double(cfggroup_mainwindow, NavArea::cfgkey_mainwindow_arptmapscale));
	double track(std::numeric_limits<double>::quiet_NaN());
	if (nr >= 2) {
		const FPlanWaypoint& wpto(m_fplgpsimproute[nr - 2]);
		track = wpto.get_coord().spheric_true_course(wpt.get_coord());
	}
        m_coorddialog->directto(wpt.get_coord(), wpt.get_icao(), wpt.get_name(), track);
	return true;
}

void FlightDeckWindow::fplgpsimppage_zoomin(void)
{
	if (m_fplgpsimpdrawingarea)
		m_fplgpsimpdrawingarea->zoom_in();
}

void FlightDeckWindow::fplgpsimppage_zoomout(void)
{
	if (m_fplgpsimpdrawingarea)
		m_fplgpsimpdrawingarea->zoom_out();
}

bool FlightDeckWindow::fplgpsimppage_convertfpl(const Sensors::Sensor::gpsflightplan_t& fpl, unsigned int curwpt)
{
	if (fpl.empty() || !m_engine)
		return false;
	m_fplgpsimproute.new_fp();
	m_fplgpsimproute.set_curwpt(0);
	m_fplgpsimproute.set_time_offblock_unix(time(0));
	m_fplgpsimproute.set_time_onblock_unix(time(0));
	for (Sensors::Sensor::gpsflightplan_t::const_iterator fpli(fpl.begin()), fple(fpl.end()); fpli != fple; ++fpli, --curwpt) {
		const Sensors::Sensor::GPSFlightPlanWaypoint& gwpt(*fpli);
		if (gwpt.get_type() == Sensors::Sensor::GPSFlightPlanWaypoint::type_invalid)
			continue;
		FPLGPSImpQuery q(m_engine);
		{
			FPlanWaypoint wpt;
			wpt.set_time_unix(time(0));
			wpt.set_coord(gwpt.get_coord());
			if (gwpt.get_type() == Sensors::Sensor::GPSFlightPlanWaypoint::type_intersection)
				wpt.set_name(gwpt.get_ident());
			else
				wpt.set_icao(gwpt.get_ident());
			wpt.set_altitude(5000);
			switch (gwpt.get_type()) {
			case Sensors::Sensor::GPSFlightPlanWaypoint::type_undefined:
				q.query(wpt.get_coord(), true, true, true);
				break;

			case Sensors::Sensor::GPSFlightPlanWaypoint::type_airport:
			case Sensors::Sensor::GPSFlightPlanWaypoint::type_military:
			case Sensors::Sensor::GPSFlightPlanWaypoint::type_heliport:
			case Sensors::Sensor::GPSFlightPlanWaypoint::type_seaplane:
				q.query(wpt.get_coord(), true, false, false);
				break;

			case Sensors::Sensor::GPSFlightPlanWaypoint::type_vor:
			case Sensors::Sensor::GPSFlightPlanWaypoint::type_ndb:
				q.query(wpt.get_coord(), false, true, false);
				break;

			case Sensors::Sensor::GPSFlightPlanWaypoint::type_intersection:
				q.query(wpt.get_coord(), false, false, true);
				break;

			default:
				break;
			}
			m_fplgpsimproute.insert_wpt(~0U, wpt);
		}
		if (!curwpt)
			m_fplgpsimproute.set_curwpt(m_fplgpsimproute.get_nrwpt());
		FPlanWaypoint& wpt(m_fplgpsimproute[m_fplgpsimproute.get_nrwpt() - 1]);
		q.wait();
		q.sort(wpt.get_coord());
		if (!q.get_arpts().empty()) {
			wpt.set(q.get_arpts()[0]);
			wpt.set_coord(gwpt.get_coord());
			wpt.set_icao(gwpt.get_ident());
			if (true)
				std::cerr << "GPS FPL: found airport " << q.get_arpts()[0].get_icao_name()
					  << ' ' << q.get_arpts()[0].get_coord().get_lat_str2()
					  << ' ' << q.get_arpts()[0].get_coord().get_lon_str2()
					  << " for " << gwpt.get_ident()
					  << ' ' << gwpt.get_coord().get_lat_str2()
					  << ' ' << gwpt.get_coord().get_lon_str2() << std::endl;
			continue;
		}
		if (!q.get_navaids().empty()) {
			wpt.set(q.get_navaids()[0]);
			wpt.set_coord(gwpt.get_coord());
			wpt.set_icao(gwpt.get_ident());
			continue;
		}
		if (!q.get_wpts().empty()) {
			wpt.set(q.get_wpts()[0]);
			wpt.set_coord(gwpt.get_coord());
			wpt.set_altitude(5000);
			wpt.set_name(gwpt.get_ident());
			continue;
		}
	}
	return true;
}

Sensors::Sensor *FlightDeckWindow::fplgpsimppage_convertfpl(unsigned int devnr)
{
	if (!m_sensors)
		return 0;
	for (unsigned int sensnr = 0; sensnr < m_sensors->size(); ++sensnr) {
		Sensors::Sensor& sens((*m_sensors)[sensnr]);
		Sensors::Sensor::gpsflightplan_t fpl;
		unsigned int curwpt(~0U);
		if (sens.get_param(0, fpl, curwpt) != Sensors::Sensor::paramfail_ok ||
		    fpl.empty())
			continue;
		if (!devnr) {
			if (!fplgpsimppage_convertfpl(fpl, curwpt))
				return 0;
			return &sens;
		}
		--devnr;
	}
	return 0;
}

bool FlightDeckWindow::fplgpsimppage_updatemenu(void)
{
	m_fplgpsimpselchangeconn.block();
	if (m_fplgpsimpstore)
	       	m_fplgpsimpstore->clear();
	m_fplgpsimpselchangeconn.unblock();
	fplgpsimppage_selection_changed();
	if (m_coorddialog)
		m_coorddialog->hide();
	if (m_menuid < 202 || m_menuid >= 300 || !m_sensors || !m_fplgpsimpstore) {
		if (m_fplgpsimpdrawingarea) {
			m_fplgpsimpdrawingarea->set_renderer(VectorMapArea::renderer_none);
			m_fplgpsimpdrawingarea->hide();
		}
		return false;
	}
	Sensors::Sensor *sens(fplgpsimppage_convertfpl(m_menuid - 202));
	if (!sens)
		return false;
	if (m_fplgpsimplabel)
		m_fplgpsimplabel->set_markup("GPS Flight Plan Import: <b>" + sens->get_name() + "</b>");
	Gtk::TreeIter seliter;
	m_fplgpsimpselchangeconn.block();
	for (unsigned int nr = 0; nr < m_fplgpsimproute.get_nrwpt(); ++nr) {
		const FPlanWaypoint& wpt(m_fplgpsimproute[nr]);
		Gtk::TreeIter iter(m_fplgpsimpstore->append());
		Gtk::TreeModel::Row row(*iter);
		row[m_fplgpsimpcolumns.m_icao] = wpt.get_icao();
		row[m_fplgpsimpcolumns.m_name] = wpt.get_name();
		row[m_fplgpsimpcolumns.m_freq] = Conversions::freq_str(wpt.get_frequency());
		row[m_fplgpsimpcolumns.m_lon] = wpt.get_coord().get_lon_str();
		row[m_fplgpsimpcolumns.m_lat] = wpt.get_coord().get_lat_str();
		if (nr + 1 < m_fplgpsimproute.get_nrwpt()) {
			const FPlanWaypoint& wpt2(m_fplgpsimproute[nr + 1]);
			Point pt1(wpt.get_coord()), pt2(wpt2.get_coord());
			float dist = pt1.spheric_distance_nmi(pt2);
			float tt = pt1.spheric_true_course(pt2);
			WMM wmm;
			wmm.compute(wpt.get_altitude() * (FPlanWaypoint::ft_to_m / 2000.0), pt1.halfway(pt2), wpt.get_time_unix());
			float mt = tt;
			if (wmm)
				mt -= wmm.get_dec();
			row[m_fplgpsimpcolumns.m_mt] = Conversions::track_str(mt);
			row[m_fplgpsimpcolumns.m_tt] = Conversions::track_str(tt);
			row[m_fplgpsimpcolumns.m_dist] = Conversions::dist_str(dist);;
		} else {
			row[m_fplgpsimpcolumns.m_mt] = "";
			row[m_fplgpsimpcolumns.m_tt] = "";
			row[m_fplgpsimpcolumns.m_dist] = "";
		}
		row[m_fplgpsimpcolumns.m_nr] = nr + 1;
		row[m_fplgpsimpcolumns.m_weight] = (m_fplgpsimproute.get_curwpt() == nr + 1) ? 600 : 400;
		row[m_fplgpsimpcolumns.m_style] = (m_fplgpsimproute.get_curwpt() == nr + 1) ? Pango::STYLE_ITALIC : Pango::STYLE_NORMAL;
		if (m_fplgpsimproute.get_curwpt() == nr + 1)
			seliter = iter;
	}
	if (seliter && m_fplgpsimptreeview) {
		Glib::RefPtr<Gtk::TreeSelection> selection = m_fplgpsimptreeview->get_selection();
		if (selection)
			selection->select(seliter);
		m_fplgpsimptreeview->scroll_to_row(Gtk::TreePath(seliter));
	}
	if (m_fplgpsimpdrawingarea) {
		m_fplgpsimpdrawingarea->set_renderer(VectorMapArea::renderer_vmap);
		*m_fplgpsimpdrawingarea = m_navarea.map_get_drawflags();
		if (m_sensors && m_sensors->get_configfile().has_group(cfggroup_mainwindow)) {
			if (false && m_sensors->get_configfile().has_key(cfggroup_mainwindow, NavArea::cfgkey_mainwindow_mapflags))
				*m_fplgpsimpdrawingarea = (MapRenderer::DrawFlags)m_sensors->get_configfile().get_integer(cfggroup_mainwindow, NavArea::cfgkey_mainwindow_mapflags);
			if (m_sensors->get_configfile().has_key(cfggroup_mainwindow, NavArea::cfgkey_mainwindow_mapscale))
				m_fplgpsimpdrawingarea->set_map_scale(m_sensors->get_configfile().get_double(cfggroup_mainwindow, NavArea::cfgkey_mainwindow_mapscale));
		}
		if (m_fplgpsimproute.get_nrwpt() > 0) {
			const FPlanWaypoint& wpt(m_fplgpsimproute[std::min(std::max(m_fplgpsimproute.get_curwpt(), (uint16_t)1U), m_fplgpsimproute.get_nrwpt()) - 1]);
			Glib::TimeVal tv;
			tv.assign_current_time();		
 			m_fplgpsimpdrawingarea->set_center(wpt.get_coord(), wpt.get_altitude(), 0, tv.tv_sec);
			m_fplgpsimpdrawingarea->set_cursor(wpt.get_coord());
		}
		m_fplgpsimpdrawingarea->show();
		m_fplgpsimpdrawingarea->redraw_route();
	}
	m_fplgpsimpselchangeconn.unblock();
	fplgpsimppage_selection_changed();
	return true;
}
