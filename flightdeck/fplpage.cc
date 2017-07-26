/***************************************************************************
 *   Copyright (C) 2007, 2008, 2009, 2011, 2012, 2013, 2016                *
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
#include "icaofpl.h"

#include <iomanip>

#include "flightdeckwindow.h"

FlightDeckWindow::WaypointListModelColumns::WaypointListModelColumns(void)
{
	add(m_icao);
	add(m_name);
	add(m_freq);
	add(m_time);
	add(m_lon);
	add(m_lat);
	add(m_alt);
	add(m_mt);
	add(m_tt);
	add(m_dist);
	add(m_vertspeed);
	add(m_mh);
	add(m_tas);
	add(m_truealt);
	add(m_power);
	add(m_fuel);
	add(m_pathname);
	add(m_wind);
	add(m_qff);
	add(m_oat);
	add(m_point);
	add(m_nr);
	add(m_weight);
	add(m_style);
	add(m_time_style);
	add(m_time_color);
}

FlightDeckWindow::WaypointListPathCodeModelColumns::WaypointListPathCodeModelColumns(void)
{
	add(m_name);
	add(m_id);
}

FlightDeckWindow::WaypointListAltUnitModelColumns::WaypointListAltUnitModelColumns(void)
{
	add(m_name);
	add(m_id);
}

FlightDeckWindow::WaypointListTypeodelColumns::WaypointListTypeodelColumns(void)
{
	add(m_name);
	add(m_type);
	add(m_navaid_type);
}

class FlightDeckWindow::FPLQuery {
public:
	FPLQuery(Engine *eng);
	~FPLQuery();
	void wait(void);
	void cancel(void);
	void query(const Glib::ustring& name);
	void query_awy(const Glib::ustring& name);
	void query_airspace(const Glib::ustring& name);
	void query_elevation(const Point& p0, const Point& p1, double corridor_width = 5);
	typedef enum {
		result_none,
		result_airport,
		result_navaid,
		result_intersection
	} result_t;
	result_t setresult(FPlanWaypoint& wpt, const Point& nearest);
	bool setgraph(Engine::AirwayGraphResult::Graph& g);
	const AirspacesDb::elementvector_t& getairspaces(void) const;
	TopoDb30::Profile getelevation(void) const;

protected:
	Engine *m_engine;
	Glib::RefPtr<Engine::AirportResult> m_queryarpt;
	Glib::RefPtr<Engine::NavaidResult> m_querynavaid;
	Glib::RefPtr<Engine::WaypointResult> m_querywpt;
	Glib::RefPtr<Engine::AirwayGraphResult> m_queryawy;
	Glib::RefPtr<Engine::AirspaceResult> m_queryaspc;
	Glib::RefPtr<Engine::ElevationProfileResult> m_queryelev;
	Glib::Mutex m_mutex;
	Glib::Cond m_cond;

	void notify(void);
};

FlightDeckWindow::FPLQuery::FPLQuery(Engine *eng)
	: m_engine(eng)
{
}

FlightDeckWindow::FPLQuery::~FPLQuery()
{
	cancel();
}

void FlightDeckWindow::FPLQuery::notify(void)
{
	m_cond.broadcast();
}

void FlightDeckWindow::FPLQuery::wait(void)
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
		if (ok && m_queryawy && !m_queryawy->is_done() && !m_queryawy->is_error())
			ok = false;
		if (ok && m_queryaspc && !m_queryaspc->is_done() && !m_queryaspc->is_error())
			ok = false;
		if (ok && m_queryelev && !m_queryelev->is_done() && !m_queryelev->is_error())
			ok = false;
		if (ok)
			break;
		m_cond.wait(m_mutex);
	}
}

void FlightDeckWindow::FPLQuery::cancel(void)
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
	if (m_queryawy) {
		Glib::RefPtr<Engine::AirwayGraphResult> q(m_queryawy);
		m_queryawy.reset();
		Engine::AirwayGraphResult::cancel(q);
	}
	if (m_queryaspc) {
		Glib::RefPtr<Engine::AirspaceResult> q(m_queryaspc);
		m_queryaspc.reset();
		Engine::AirspaceResult::cancel(q);
	}
	if (m_queryelev) {
		Glib::RefPtr<Engine::ElevationProfileResult> q(m_queryelev);
		m_queryelev.reset();
		Engine::ElevationProfileResult::cancel(q);
	}
}

void FlightDeckWindow::FPLQuery::query(const Glib::ustring& name)
{
	cancel();
	if (!m_engine)
		return;
	m_queryarpt = m_engine->async_airport_find_by_text(name, 64, 0, AirportsDb::comp_exact,
							   AirportsDb::element_t::subtables_vfrroutes |
							   AirportsDb::element_t::subtables_runways |
							   AirportsDb::element_t::subtables_helipads |
							   AirportsDb::element_t::subtables_comms);
	if (m_queryarpt)
		m_queryarpt->connect(sigc::mem_fun(*this, &FPLQuery::notify));
	m_querynavaid = m_engine->async_navaid_find_by_text(name, 64, 0, NavaidsDb::comp_exact, NavaidsDb::element_t::subtables_none);
	if (m_querynavaid)
		m_querynavaid->connect(sigc::mem_fun(*this, &FPLQuery::notify));
	m_querywpt = m_engine->async_waypoint_find_by_name(name, 64, 0, WaypointsDb::comp_exact, WaypointsDb::element_t::subtables_none);
	if (m_querywpt)
		m_querywpt->connect(sigc::mem_fun(*this, &FPLQuery::notify));
}

void FlightDeckWindow::FPLQuery::query_awy(const Glib::ustring& name)
{
	cancel();
	if (!m_engine)
		return;
	m_queryawy = m_engine->async_airway_graph_find_by_name(name, ~0U, 0, AirwaysDb::comp_contains, AirwaysDb::element_t::subtables_none);
	if (m_queryawy)
		m_queryawy->connect(sigc::mem_fun(*this, &FPLQuery::notify));
}

void FlightDeckWindow::FPLQuery::query_airspace(const Glib::ustring& name)
{
	cancel();
	if (!m_engine)
		return;
	m_queryaspc = m_engine->async_airspace_find_by_icao(name, ~0U, 0, AirwaysDb::comp_exact, AirwaysDb::element_t::subtables_none);
	if (m_queryaspc)
		m_queryaspc->connect(sigc::mem_fun(*this, &FPLQuery::notify));
}

void FlightDeckWindow::FPLQuery::query_elevation(const Point& p0, const Point& p1, double corridor_width)
{
	cancel();
	if (!m_engine)
		return;
	m_queryelev = m_engine->async_elevation_profile(p0, p1, corridor_width);
	if (m_queryelev)
		m_queryelev->connect(sigc::mem_fun(*this, &FPLQuery::notify));
}

FlightDeckWindow::FPLQuery::result_t FlightDeckWindow::FPLQuery::setresult(FPlanWaypoint& wpt, const Point& nearest)
{
	const double nodist(std::numeric_limits<double>::max());
	double dist(nodist);
	result_t res(result_none);
	if (m_queryarpt && m_queryarpt->is_done() && !m_queryarpt->is_error()) {
		const AirportsDb::elementvector_t& ev(m_queryarpt->get_result());
		for (AirportsDb::elementvector_t::const_iterator ei(ev.begin()), ee(ev.end()); ei != ee; ++ei) {
			double d(nearest.spheric_distance_nmi_dbl(ei->get_coord()));
			if (d >= dist)
				continue;
			wpt = FPlanWaypoint();
			wpt.set(*ei);
			dist = d;
			res = result_airport;
		}
	}
	if (m_querynavaid && m_querynavaid->is_done() && !m_querynavaid->is_error()) {
		const NavaidsDb::elementvector_t& ev(m_querynavaid->get_result());
		for (NavaidsDb::elementvector_t::const_iterator ei(ev.begin()), ee(ev.end()); ei != ee; ++ei) {
			double d(nearest.spheric_distance_nmi_dbl(ei->get_coord()));
			if (d >= dist)
				continue;
			wpt = FPlanWaypoint();
			wpt.set(*ei);
			dist = d;
			res = result_navaid;
		}
	}
	if (m_querywpt && m_querywpt->is_done() && !m_querywpt->is_error()) {
		const WaypointsDb::elementvector_t& ev(m_querywpt->get_result());
		for (WaypointsDb::elementvector_t::const_iterator ei(ev.begin()), ee(ev.end()); ei != ee; ++ei) {
			double d(nearest.spheric_distance_nmi_dbl(ei->get_coord()));
			if (d >= dist)
				continue;
			wpt = FPlanWaypoint();
			wpt.set(*ei);
			dist = d;
			res = result_intersection;
		}
	}
	return res;
}

bool FlightDeckWindow::FPLQuery::setgraph(Engine::AirwayGraphResult::Graph& g)
{
	if (!m_queryawy || !m_queryawy->is_done() || m_queryawy->is_error())
		return false;
	g = m_queryawy->get_result();
	return true;
}

const AirspacesDb::elementvector_t& FlightDeckWindow::FPLQuery::getairspaces(void) const
{
	static const AirspacesDb::elementvector_t null;
	if (m_queryaspc && m_queryaspc->is_done() && !m_queryaspc->is_error())
		return m_queryaspc->get_result();
	return null;
}

TopoDb30::Profile FlightDeckWindow::FPLQuery::getelevation(void) const
{
	if (m_queryelev && m_queryelev->is_done() && !m_queryelev->is_error())
		return m_queryelev->get_result();
	return TopoDb30::Profile();	
}

int32_t FlightDeckWindow::fplpage_get_altwidgets(uint16_t& flags)
{
	unsigned int unit(0);
	if (m_fplaltunitcombobox) {
		Gtk::TreeModel::const_iterator iter(m_fplaltunitcombobox->get_active());
		if (iter) {
			Gtk::TreeModel::Row row(*iter);
			unit = row[m_fplaltunitcolumns.m_id];
		}
	}
	double v(0);
	if (m_fplaltentry)
		v = strtod(m_fplaltentry->get_text().c_str(), NULL);
	switch (unit) {
                case 2:
                        v *= FPlanWaypoint::m_to_ft;
                        break;

                case 1:
                        v *= 100;
                        break;
        }
        if (unit == 1)
                flags |= FPlanWaypoint::altflag_standard;
        else
                flags &= ~FPlanWaypoint::altflag_standard;
        return Point::round<int32_t,double>(v);
}

void FlightDeckWindow::fpl_selection_changed(void)
{
	if (m_coorddialog)
		m_coorddialog->hide();
	fplpage_softkeyenable();
	if (!m_fpltreeview) {
		fplpage_select();
		return;
	}
	Glib::RefPtr<Gtk::TreeSelection> selection(m_fpltreeview->get_selection());
	if (!selection) {
		fplpage_select();
		return;
	}
        Gtk::TreeModel::iterator iter(selection->get_selected());
        if (!iter) {
		fplpage_select();
		return;
	}
	Gtk::TreeModel::Row row(*iter);
	unsigned int nr(row[m_fplcolumns.m_nr]);
	fplpage_select(nr);
	if (!m_sensors)
		return;
	FPlanRoute& fpl(m_sensors->get_route());
	std::cerr << "fpl_selection_changed: curwpt " << fpl.get_curwpt() << " nr " << nr << " nrwpt " << fpl.get_nrwpt() << std::endl;
	if (fpl.get_curwpt() == nr)
		return;
	fpl.set_curwpt(nr);
	m_sensors->nav_fplan_modified();
}

void FlightDeckWindow::fplpage_select(void)
{
	if (m_fplicaoentry) {
		m_fplicaoentry->set_text("");
		m_fplicaoentry->set_sensitive(false);
	}
	if (m_fplnameentry) {
		m_fplnameentry->set_text("");
		m_fplnameentry->set_sensitive(false);
	}
	m_fplpathcodechangeconn.block();
	if (m_fplpathcodecombobox) {
		m_fplpathcodecombobox->unset_active();
		m_fplpathcodecombobox->set_sensitive(false);
	}
	m_fplpathcodechangeconn.unblock();
	if (m_fplpathnameentry) {
		m_fplpathnameentry->set_text("");
		m_fplpathnameentry->set_sensitive(false);
	}
	if (m_fplcoordentry) {
		m_fplcoordentry->set_text("");
		m_fplcoordentry->set_sensitive(false);
	}
	if (m_fplfreqentry) {
		m_fplfreqentry->set_text("");
		m_fplfreqentry->set_sensitive(false);
	}
	if (m_fplfrequnitlabel)
		m_fplfrequnitlabel->set_text("");
	if (m_fplaltentry) {
		m_fplaltentry->set_text("");
		m_fplaltentry->set_sensitive(false);
	}
	m_fplaltunitchangeconn.block();
	if (m_fplaltunitcombobox) {
		m_fplaltunitcombobox->unset_active();
		m_fplaltunitcombobox->set_sensitive(false);
	}
	m_fplaltunitchangeconn.unblock();
	m_fplifrchangeconn.block();
	if (m_fplifrcheckbutton) {
		m_fplifrcheckbutton->set_inconsistent(true);
		m_fplifrcheckbutton->set_sensitive(false);
	}
	m_fplifrchangeconn.unblock();
	m_fpltypechangeconn.block();
	if (m_fpltypecombobox) {
		m_fpltypecombobox->unset_active();
		m_fpltypecombobox->set_sensitive(false);
	}
	m_fpltypechangeconn.unblock();
	if (m_fplcoordeditbutton)
		m_fplcoordeditbutton->set_sensitive(false);
	if (m_fpllevelupbutton)
		m_fpllevelupbutton->set_sensitive(false);
	if (m_fplleveldownbutton)
		m_fplleveldownbutton->set_sensitive(false);
	if (m_fplsemicircularbutton)
		m_fplsemicircularbutton->set_sensitive(false);
	if (m_fplwinddirspinbutton) {
		m_fplwinddirspinbutton->set_value(0);
		m_fplwinddirspinbutton->set_sensitive(false);
	}
	if (m_fplwindspeedspinbutton) {
		m_fplwindspeedspinbutton->set_value(0);
		m_fplwindspeedspinbutton->set_sensitive(false);
	}
	if (m_fplqffspinbutton) {
		m_fplqffspinbutton->set_value(IcaoAtmosphere<double>::std_sealevel_pressure);
		m_fplqffspinbutton->set_sensitive(false);
	}
	if (m_fploatspinbutton) {
		m_fploatspinbutton->set_value(15);
		m_fploatspinbutton->set_sensitive(false);
	}
	if (m_fplwptnotestextview) {
                Glib::RefPtr<Gtk::TextBuffer> buf(m_fplwptnotestextview->get_buffer());
                if (buf)
                        buf->set_text("");
		m_fplwptnotestextview->set_sensitive(false);
        }
	if (m_fplnotebook)
		m_fplnotebook->set_current_page(1);
	if (m_fplduplicatebutton)
		m_fplduplicatebutton->set_sensitive(false);
	if (m_fplmoveupbutton)
		m_fplmoveupbutton->set_sensitive(false);
	if (m_fplmovedownbutton)
		m_fplmovedownbutton->set_sensitive(false);
	if (m_fpldeletebutton)
		m_fpldeletebutton->set_sensitive(false);
	if (m_fplstraightenbutton)
		m_fplstraightenbutton->set_sensitive(false);
	if (m_fpldirecttobutton)
		m_fpldirecttobutton->set_sensitive(false);
}

void FlightDeckWindow::fplpage_select(unsigned int nr)
{
	if (!m_sensors) {
		fplpage_select();
		if (m_fpldirecttobutton)
			m_fpldirecttobutton->set_sensitive(true);
		return;
	}
	const FPlanRoute& fpl(m_sensors->get_route());
	if (!fpl.get_nrwpt()) {
		fplpage_select();
		if (m_fpldirecttobutton)
			m_fpldirecttobutton->set_sensitive(true);
		return;
	}
	if (nr < 1) {
		const FPlanWaypoint& wpt(fpl[0]);
		if (m_fpldrawingarea) {
			Glib::TimeVal tv;
			tv.assign_current_time();		
 			m_fpldrawingarea->set_center(wpt.get_coord(), wpt.get_altitude(), 0, tv.tv_sec);
			m_fpldrawingarea->set_cursor(wpt.get_coord());
		}
		fplpage_select();
		if (m_fpldirecttobutton)
			m_fpldirecttobutton->set_sensitive(true);
		return;
	}
	if (nr > fpl.get_nrwpt()) {
		const FPlanWaypoint& wpt(fpl[fpl.get_nrwpt() - 1]);
		if (m_fpldrawingarea) {
			Glib::TimeVal tv;
			tv.assign_current_time();		
 			m_fpldrawingarea->set_center(wpt.get_coord(), wpt.get_altitude(), 0, tv.tv_sec);
			m_fpldrawingarea->set_cursor(wpt.get_coord());
		}
		fplpage_select();
		if (m_fpldirecttobutton)
			m_fpldirecttobutton->set_sensitive(true);
		return;
	}
	const FPlanWaypoint& wpt(fpl[nr - 1]);
	if (m_fplicaoentry) {
		m_fplicaoentry->set_text(wpt.get_icao());
		m_fplicaoentry->set_sensitive(true);
	}
	if (m_fplnameentry) {
		m_fplnameentry->set_text(wpt.get_name());
		m_fplnameentry->set_sensitive(true);
	}
	m_fplpathcodechangeconn.block();
	if (m_fplpathcodecombobox) {
		Glib::RefPtr<const Gtk::TreeModel> model(m_fplpathcodecombobox->get_model());
		if (model) {
			for (Gtk::TreeIter iter(model->children().begin()); iter; ++iter) {
				Gtk::TreeModel::Row row(*iter);
				if (row[m_fplpathcodecolumns.m_id] != wpt.get_pathcode())
					continue;
				m_fplpathcodecombobox->set_active(iter);
				break;
			}
		}
		if (false)
			m_fplpathcodecombobox->set_active(wpt.get_pathcode());
		m_fplpathcodecombobox->set_sensitive(true);
	}
	m_fplpathcodechangeconn.unblock();
	if (m_fplpathnameentry) {
		m_fplpathnameentry->set_text(wpt.get_pathname());
		m_fplpathnameentry->set_sensitive(wpt.get_pathcode() != FPlanWaypoint::pathcode_none &&
						  wpt.get_pathcode() != FPlanWaypoint::pathcode_directto);
	}
	if (m_fplcoordentry) {
		m_fplcoordentry->set_text(wpt.get_coord().get_lat_str() + " " + wpt.get_coord().get_lon_str());
		m_fplcoordentry->set_sensitive(true);
	}
	{
		uint32_t freq(wpt.get_frequency());
		if (freq >= 1000 && freq <= 999999) {
			if (m_fplfreqentry)
				m_fplfreqentry->set_text(Glib::ustring::format((freq + 500) / 1000));
			if (m_fplfrequnitlabel)
				m_fplfrequnitlabel->set_text("kHz");
		} else if (freq >= 1000000 && freq <= 999999999) {
			if (m_fplfreqentry)
				m_fplfreqentry->set_text(Glib::ustring::format(std::fixed, std::setprecision(3), freq * 1e-6));
			if (m_fplfrequnitlabel)
				m_fplfrequnitlabel->set_text("MHz");
		} else {
			if (m_fplfreqentry)
				m_fplfreqentry->set_text(Glib::ustring::format(freq));
			if (m_fplfrequnitlabel)
				m_fplfrequnitlabel->set_text("Hz");
                }
		if (m_fplfreqentry)
			m_fplfreqentry->set_sensitive(true);
        }
	m_fplaltunitchangeconn.block();
	{
		unsigned int unit(0);
		if (m_fplaltunitcombobox) {
			Gtk::TreeModel::const_iterator iter(m_fplaltunitcombobox->get_active());
			if (iter) {
				Gtk::TreeModel::Row row(*iter);
				unit = row[m_fplaltunitcolumns.m_id];
			}
		}
		if (unit > 2)
			unit = 0;
		if (wpt.get_flags() & FPlanWaypoint::altflag_standard) {
			unit = 1;
		} else {
			if (unit == 1)
				unit = 0;
		}
		if (m_fplaltentry) {
			switch (unit) {
			case 1:
				m_fplaltentry->set_text(Glib::ustring::format(std::fixed, std::setprecision(2), wpt.get_altitude() * 0.01));
				break;

			case 2:
				m_fplaltentry->set_text(Glib::ustring::format(std::fixed, std::setprecision(2), wpt.get_altitude() * FPlanWaypoint::ft_to_m));
				break;

			default:
				m_fplaltentry->set_text(Glib::ustring::format(wpt.get_altitude()));
				break;
			}
			m_fplaltentry->set_sensitive(true);
		}
		if (m_fplaltunitcombobox) {
			m_fplaltunitcombobox->set_active(unit);
			m_fplaltunitcombobox->set_sensitive(true);
		}
	}
	m_fplaltunitchangeconn.unblock();
	m_fplifrchangeconn.block();
	if (m_fplifrcheckbutton) {
		m_fplifrcheckbutton->set_inconsistent(false);
		m_fplifrcheckbutton->set_active(!!(wpt.get_flags() & FPlanWaypoint::altflag_ifr));
		m_fplifrcheckbutton->set_sensitive(true);
	}
	m_fplifrchangeconn.unblock();
	m_fpltypechangeconn.block();
	if (m_fpltypecombobox) {
		Glib::RefPtr<const Gtk::TreeModel> model(m_fpltypecombobox->get_model());
		if (model) {
			for (Gtk::TreeIter iter(model->children().begin()); iter; ++iter) {
				Gtk::TreeModel::Row row(*iter);
				if (row[m_fpltypecolumns.m_type] != wpt.get_type())
					continue;
				if (row[m_fpltypecolumns.m_navaid_type] != wpt.get_navaid() && wpt.get_type() == FPlanWaypoint::type_navaid)
					continue;
				m_fpltypecombobox->set_active(iter);
				break;
			}
		}
		m_fpltypecombobox->set_sensitive(true);
	}
	m_fpltypechangeconn.unblock();
	if (m_fplcoordeditbutton)
		m_fplcoordeditbutton->set_sensitive(true);
	if (m_fpllevelupbutton)
		m_fpllevelupbutton->set_sensitive(true);
	if (m_fplleveldownbutton)
		m_fplleveldownbutton->set_sensitive(true);
	if (m_fplsemicircularbutton)
		m_fplsemicircularbutton->set_sensitive(true);
	if (m_fplwinddirspinbutton) {
		m_fplwinddirspinbutton->set_value(wpt.get_winddir_deg());
		m_fplwinddirspinbutton->set_sensitive(true);
	}
	if (m_fplwindspeedspinbutton) {
		m_fplwindspeedspinbutton->set_value(wpt.get_windspeed_kts());
		m_fplwindspeedspinbutton->set_sensitive(true);
	}
	if (m_fplqffspinbutton) {
		m_fplqffspinbutton->set_value(wpt.get_qff_hpa());
		m_fplqffspinbutton->set_sensitive(true);
	}
	if (m_fploatspinbutton) {
		m_fploatspinbutton->set_value(wpt.get_oat_kelvin() - IcaoAtmosphere<double>::degc_to_kelvin);
		m_fploatspinbutton->set_sensitive(true);
	}
	if (m_fplwptnotestextview) {
                Glib::RefPtr<Gtk::TextBuffer> buf(m_fplwptnotestextview->get_buffer());
                if (buf)
                        buf->set_text(wpt.get_note());
		m_fplwptnotestextview->set_sensitive(true);
        }
	if (m_fpldrawingarea) {
		Glib::TimeVal tv;
		tv.assign_current_time();		
 		m_fpldrawingarea->set_center(wpt.get_coord(), wpt.get_altitude(), 0, tv.tv_sec);
                m_fpldrawingarea->set_cursor(wpt.get_coord());
	}
	if (m_fplnotebook)
		m_fplnotebook->set_current_page(0);
	if (m_fplduplicatebutton)
		m_fplduplicatebutton->set_sensitive(true);
	if (m_fplmoveupbutton)
		m_fplmoveupbutton->set_sensitive(nr > 1);
	if (m_fplmovedownbutton)
		m_fplmovedownbutton->set_sensitive(nr < fpl.get_nrwpt());
	if (m_fpldeletebutton)
		m_fpldeletebutton->set_sensitive(true);
	if (m_fplstraightenbutton)
		m_fplstraightenbutton->set_sensitive(nr > 1 && nr < fpl.get_nrwpt());
	if (m_fpldirecttobutton)
		m_fpldirecttobutton->set_sensitive(true);
}

bool FlightDeckWindow::fplpage_focus_in_event(GdkEventFocus *event, Gtk::Widget *widget, kbd_t kbd)
{
	m_fplchg = fplchg_none;
	return onscreenkbd_focus_in_event(event, widget, kbd);
}

bool FlightDeckWindow::fplpage_focus_out_event(GdkEventFocus *event)
{
	unsigned int nr(~0U);
	if (m_fplchg != fplchg_none && m_fpltreeview) {
		Glib::RefPtr<Gtk::TreeSelection> selection(m_fpltreeview->get_selection());
		if (selection) {
			Gtk::TreeModel::iterator iter(selection->get_selected());
			if (iter) {
				Gtk::TreeModel::Row row(*iter);
				nr = row[m_fplcolumns.m_nr];
			}
		}
	}
	if (true)
		std::cerr << "fplpage_focus_out_event: " << m_fplchg << " nr: " << nr << std::endl;
	FPlanRoute& fpl(m_sensors->get_route());
	switch (m_fplchg) {
	case fplchg_icao:
		if (!m_fplicaoentry || nr < 1 || nr > fpl.get_nrwpt())
			break;
		fpl[nr - 1].set_icao(m_fplicaoentry->get_text());
		fpl[nr - 1].set_type(FPlanWaypoint::type_user);
		m_sensors->nav_fplan_modified();
		break;

	case fplchg_name:
		if (!m_fplnameentry || nr < 1 || nr > fpl.get_nrwpt())
			break;
		fpl[nr - 1].set_name(m_fplnameentry->get_text());
		fpl[nr - 1].set_type(FPlanWaypoint::type_user);
		m_sensors->nav_fplan_modified();
		break;

	case fplchg_pathname:
	{
		if (!m_fplpathnameentry || nr < 1 || nr > fpl.get_nrwpt())
			break;
		FPlanWaypoint& wpt(fpl[nr - 1]);
		wpt.set_pathname(m_fplpathnameentry->get_text());
		if (wpt.get_pathcode() == FPlanWaypoint::pathcode_airway &&
		    wpt.get_pathname().uppercase() == "DCT") {
			wpt.set_pathcode(FPlanWaypoint::pathcode_directto);
			wpt.set_pathname("");
		}
		if (wpt.get_pathcode() == FPlanWaypoint::pathcode_directto &&
		    !wpt.get_pathname().empty())
			wpt.set_pathcode(FPlanWaypoint::pathcode_airway);
		if (m_fplpathnameentry)
			m_fplpathnameentry->set_sensitive(wpt.get_pathcode() != FPlanWaypoint::pathcode_none &&
							  wpt.get_pathcode() != FPlanWaypoint::pathcode_directto);
		m_sensors->nav_fplan_modified();
		break;
	}

	case fplchg_coord:
		if (!m_fplcoordentry || nr < 1 || nr > fpl.get_nrwpt())
			break;
		{
			FPlanWaypoint& wpt(fpl[nr - 1]);
			Point pt(wpt.get_coord());
			unsigned int r(pt.set_str(m_fplcoordentry->get_text()));
			std::cerr << "coord: set_str: " << r
				  << " coord: " << pt.get_lat_str() << ' ' << pt.get_lon_str() << std::endl;
			wpt.set_coord(pt);
			wpt.set_type(FPlanWaypoint::type_user);
			if (m_fpldrawingarea) {
				Glib::TimeVal tv;
				tv.assign_current_time();		
				m_fpldrawingarea->set_center(wpt.get_coord(), wpt.get_altitude(), 0, tv.tv_sec);
				m_fpldrawingarea->set_cursor(wpt.get_coord());
			}
		}
		m_sensors->nav_fplan_modified();
		break;

	case fplchg_freq:
		if (!m_fplfreqentry || nr < 1 || nr > fpl.get_nrwpt())
			break;
		{
			std::locale loc("");
			const std::numpunct<char>& np = std::use_facet<std::numpunct<char> >(loc);
			const char *cp(m_fplfreqentry->get_text().c_str());
			if (false)
				std::cerr << "converting frequency: \"" << cp << "\" decimal point char '" << np.decimal_point() << "'" << std::endl;
			uint32_t freq(0);
			if (strchr(cp, np.decimal_point())) {
				if (false)
					std::cerr << "converting MHz frequency: \"" << cp << "\"" << std::endl;
				std::istringstream is(cp);
				is.imbue(loc); // use the user's locale for this stream
				double f;
				is >> f;
				freq = Point::round<int32_t,double>(f * 1e6);
			} else {
				freq = strtoul(cp, NULL, 10) * 1000;
			}
			fpl[nr - 1].set_frequency(freq);
		}
		m_sensors->nav_fplan_modified();
		break;

	case fplchg_alt:
		if (!m_fplaltentry || nr < 1 || nr > fpl.get_nrwpt())
			break;
		{
			FPlanWaypoint& wpt(fpl[nr - 1]);
			uint16_t flags(wpt.get_flags());
			wpt.set_altitude(fplpage_get_altwidgets(flags));
			wpt.set_flags(flags);
			if (m_fpldrawingarea) {
				Glib::TimeVal tv;
				tv.assign_current_time();		
				m_fpldrawingarea->set_center(wpt.get_coord(), wpt.get_altitude(), 0, tv.tv_sec);
				m_fpldrawingarea->set_cursor(wpt.get_coord());
			}
		}
		m_sensors->nav_fplan_modified();
		break;

	case fplchg_winddir:
		if (!m_fplwinddirspinbutton)
			break;
		{
			FPlanWaypoint& wpt(fpl[nr - 1]);
			wpt.set_winddir_deg(m_fplwinddirspinbutton->get_value());
		}
		m_sensors->nav_fplan_modified();
		break;

	case fplchg_windspeed:
		if (!m_fplwindspeedspinbutton)
			break;
		{
			FPlanWaypoint& wpt(fpl[nr - 1]);
			wpt.set_windspeed_kts(m_fplwindspeedspinbutton->get_value());
		}
		m_sensors->nav_fplan_modified();
		break;

	case fplchg_qff:
		if (!m_fplqffspinbutton)
			break;
		{
			FPlanWaypoint& wpt(fpl[nr - 1]);
			wpt.set_qff_hpa(m_fplqffspinbutton->get_value());
		}
		m_sensors->nav_fplan_modified();
		break;

	case fplchg_oat:
		if (!m_fploatspinbutton)
			break;
		{
			FPlanWaypoint& wpt(fpl[nr - 1]);
			wpt.set_oat_kelvin(m_fploatspinbutton->get_value() + IcaoAtmosphere<double>::degc_to_kelvin);
		}
		m_sensors->nav_fplan_modified();
		break;

	case fplchg_note:
		if (!m_fplwptnotestextview || nr < 1 || nr > fpl.get_nrwpt())
			break;
		{
			Glib::RefPtr<Gtk::TextBuffer> buf(m_fplwptnotestextview->get_buffer());
			if (!buf)
				break;
			fpl[nr - 1].set_note(buf->get_text());
		}
		m_sensors->nav_fplan_modified();
		break;

	case fplchg_fplnote:
		if (!m_fplnotestextview)
			break;
		{
			Glib::RefPtr<Gtk::TextBuffer> buf(m_fplnotestextview->get_buffer());
			if (!buf)
				break;
			fpl.set_note(buf->get_text());
		}
		m_sensors->nav_fplan_modified();
		break;

	case fplchg_none:
	default:
		break;
	}
	m_fplchg = fplchg_none;
	return onscreenkbd_focus_out_event(event);
}

void FlightDeckWindow::fplpage_change_event(fplchg_t chg)
{
	m_fplchg = chg;
	if (false)
		std::cerr << "fplpage_change_event: " << chg << std::endl;
}

void FlightDeckWindow::fplpage_pathcode_changed(void)
{
	unsigned int nr(~0U);
	if (m_fpltreeview) {
		Glib::RefPtr<Gtk::TreeSelection> selection(m_fpltreeview->get_selection());
		if (selection) {
			Gtk::TreeModel::iterator iter(selection->get_selected());
			if (iter) {
				Gtk::TreeModel::Row row(*iter);
				nr = row[m_fplcolumns.m_nr];
			}
		}
	}
	if (true)
		std::cerr << "fplpage_pathcode_changed: nr: " << nr << std::endl;
	FPlanRoute& fpl(m_sensors->get_route());
	if (!m_fplpathcodecombobox || nr < 1 || nr > fpl.get_nrwpt())
		return;
	FPlanWaypoint::pathcode_t pc(FPlanWaypoint::pathcode_none);
	Gtk::TreeModel::const_iterator iter(m_fplpathcodecombobox->get_active());
	if (iter) {
		Gtk::TreeModel::Row row(*iter);
		FPlanWaypoint& wpt(fpl[nr - 1]);
		wpt.set_pathcode(row[m_fplpathcodecolumns.m_id]);
		if (wpt.get_pathcode() == FPlanWaypoint::pathcode_airway &&
		    wpt.get_pathname().uppercase() == "DCT") {
			wpt.set_pathcode(FPlanWaypoint::pathcode_directto);
			wpt.set_pathname("");
		}
		if (wpt.get_pathcode() == FPlanWaypoint::pathcode_directto)
			wpt.set_pathname("");
		m_sensors->nav_fplan_modified();
		fplpage_select(fpl.get_curwpt());
	}
}

void FlightDeckWindow::fplpage_altunit_changed(void)
{
	unsigned int nr(~0U);
	if (m_fpltreeview) {
		Glib::RefPtr<Gtk::TreeSelection> selection(m_fpltreeview->get_selection());
		if (selection) {
			Gtk::TreeModel::iterator iter(selection->get_selected());
			if (iter) {
				Gtk::TreeModel::Row row(*iter);
				nr = row[m_fplcolumns.m_nr];
			}
		}
	}
	if (true)
		std::cerr << "fplpage_altunit_changed: nr: " << nr << std::endl;
	FPlanRoute& fpl(m_sensors->get_route());
	if (!m_fplaltunitcombobox || nr < 1 || nr > fpl.get_nrwpt())
		return;
	Gtk::TreeModel::const_iterator iter(m_fplaltunitcombobox->get_active());
	if (iter) {
		Gtk::TreeModel::Row row(*iter);
		FPlanWaypoint& wpt(fpl[nr - 1]);
		uint16_t flags(wpt.get_flags());
		if (row[m_fplaltunitcolumns.m_id] == 1)
			flags |= FPlanWaypoint::altflag_standard;
		else
			flags &= ~FPlanWaypoint::altflag_standard;
		wpt.set_flags(flags);
		m_sensors->nav_fplan_modified();
		fplpage_select(fpl.get_curwpt());
	}
}

void FlightDeckWindow::fplpage_levelup_clicked(void)
{
	unsigned int nr(~0U);
	if (m_fpltreeview) {
		Glib::RefPtr<Gtk::TreeSelection> selection(m_fpltreeview->get_selection());
		if (selection) {
			Gtk::TreeModel::iterator iter(selection->get_selected());
			if (iter) {
				Gtk::TreeModel::Row row(*iter);
				nr = row[m_fplcolumns.m_nr];
			}
		}
	}
	if (true)
		std::cerr << "fplpage_levelup_clicked: nr: " << nr << std::endl;
	FPlanRoute& fpl(m_sensors->get_route());
	if (nr < 1 || nr > fpl.get_nrwpt())
		return;
	FPlanWaypoint& wpt(fpl[nr - 1]);
	if (wpt.get_altitude() > 31000)
		return;
	wpt.set_altitude(wpt.get_altitude() + 1000);
	m_sensors->nav_fplan_modified();
	fplpage_select(fpl.get_curwpt());
}

void FlightDeckWindow::fplpage_leveldown_clicked(void)
{
	unsigned int nr(~0U);
	if (m_fpltreeview) {
		Glib::RefPtr<Gtk::TreeSelection> selection(m_fpltreeview->get_selection());
		if (selection) {
			Gtk::TreeModel::iterator iter(selection->get_selected());
			if (iter) {
				Gtk::TreeModel::Row row(*iter);
				nr = row[m_fplcolumns.m_nr];
			}
		}
	}
	if (true)
		std::cerr << "fplpage_leveldown_clicked: nr: " << nr << std::endl;
	FPlanRoute& fpl(m_sensors->get_route());
	if (nr < 1 || nr > fpl.get_nrwpt())
		return;
	FPlanWaypoint& wpt(fpl[nr - 1]);
	if (wpt.get_altitude() < 1000)
		return;
	wpt.set_altitude(wpt.get_altitude() - 1000);
	m_sensors->nav_fplan_modified();
	fplpage_select(fpl.get_curwpt());
}

void FlightDeckWindow::fplpage_semicircular_clicked(void)
{
	unsigned int nr(~0U);
	if (m_fpltreeview) {
		Glib::RefPtr<Gtk::TreeSelection> selection(m_fpltreeview->get_selection());
		if (selection) {
			Gtk::TreeModel::iterator iter(selection->get_selected());
			if (iter) {
				Gtk::TreeModel::Row row(*iter);
				nr = row[m_fplcolumns.m_nr];
			}
		}
	}
	if (true)
		std::cerr << "fplpage_semicircular_clicked: nr: " << nr << std::endl;
	FPlanRoute& fpl(m_sensors->get_route());
	if (nr < 1 || nr > fpl.get_nrwpt())
		return;
	FPlanWaypoint& wpt0(fpl[nr - 1]);
	int32_t alt(wpt0.get_altitude());
	int32_t base(0);
	if (!(wpt0.get_flags() & FPlanWaypoint::altflag_ifr))
		base = 500;
	if (nr == fpl.get_nrwpt()) {
		alt = ((alt + 500 - base) / 1000) * 1000 + base;
		alt = std::max(std::min(alt, (int32_t)32000), (int32_t)0);
		wpt0.set_altitude(alt);
		m_sensors->nav_fplan_modified();
		fplpage_select(fpl.get_curwpt());
		return;
	}
	const FPlanWaypoint& wpt1(fpl[nr]);
	double magcrs(wpt0.get_coord().spheric_true_course_dbl(wpt1.get_coord()));
	// convert true to magnetic
	WMM wmm;
	wmm.compute(alt * (FPlanWaypoint::ft_to_m / 1000.0), wpt0.get_coord(), time(0));
	if (wmm)
		magcrs -= wmm.get_dec();
	if (magcrs < 0)
		magcrs += 360;
	else if (magcrs >= 360)
		magcrs -= 360;
	bool italy_portugal(false);
	bool newzealand(false);
	bool england(false);
	int32_t terrain(0);
	{
		FPLQuery q(m_engine);
		{
			q.query_airspace("LI");
			q.wait();
			const AirspacesDb::elementvector_t& ev(q.getairspaces());
			for (AirspacesDb::elementvector_t::const_iterator evi(ev.begin()), eve(ev.end()); evi != eve; ++evi) {
				if (evi->get_icao() != "LI" || evi->get_typecode() != AirspacesDb::Airspace::typecode_ead ||
				    evi->get_bdryclass() != AirspacesDb::Airspace::bdryclass_ead_nas)
					continue;
				if (!evi->get_bbox().is_inside(wpt0.get_coord()))
					continue;
				if (!evi->get_polygon().windingnumber(wpt0.get_coord()))
					continue;
				italy_portugal = true;
				break;
			}
		}
		if (!italy_portugal) {
			q.query_airspace("LPPC");
			q.wait();
			const AirspacesDb::elementvector_t& ev(q.getairspaces());
			for (AirspacesDb::elementvector_t::const_iterator evi(ev.begin()), eve(ev.end()); evi != eve; ++evi) {
				if (evi->get_icao() != "LPPC" || evi->get_typecode() != AirspacesDb::Airspace::typecode_ead ||
				    evi->get_bdryclass() != AirspacesDb::Airspace::bdryclass_ead_fir)
					continue;
				if (!evi->get_bbox().is_inside(wpt0.get_coord()))
					continue;
				if (!evi->get_polygon().windingnumber(wpt0.get_coord()))
					continue;
				italy_portugal = true;
				break;
			}
		}
		{
			q.query_airspace("NZ");
			q.wait();
			const AirspacesDb::elementvector_t& ev(q.getairspaces());
			for (AirspacesDb::elementvector_t::const_iterator evi(ev.begin()), eve(ev.end()); evi != eve; ++evi) {
				if (evi->get_icao() != "NZ" || evi->get_typecode() != AirspacesDb::Airspace::typecode_ead ||
				    evi->get_bdryclass() != AirspacesDb::Airspace::bdryclass_ead_nas)
					continue;
				if (!evi->get_bbox().is_inside(wpt0.get_coord()))
					continue;
				if (!evi->get_polygon().windingnumber(wpt0.get_coord()))
					continue;
			        newzealand = true;
				break;
			}
		}
		{
			// London FIR
			q.query_airspace("EGTT");
			q.wait();
			const AirspacesDb::elementvector_t& ev(q.getairspaces());
			for (AirspacesDb::elementvector_t::const_iterator evi(ev.begin()), eve(ev.end()); evi != eve; ++evi) {
				if (evi->get_icao() != "EGTT" || evi->get_typecode() != AirspacesDb::Airspace::typecode_ead ||
				    evi->get_bdryclass() != AirspacesDb::Airspace::bdryclass_ead_fir)
					continue;
				if (!evi->get_bbox().is_inside(wpt0.get_coord()))
					continue;
				if (!evi->get_polygon().windingnumber(wpt0.get_coord()))
					continue;
			        england = true;
				break;
			}
		}
		if (!england) {
			// Scottish FIR
			q.query_airspace("EGPX");
			q.wait();
			const AirspacesDb::elementvector_t& ev(q.getairspaces());
			for (AirspacesDb::elementvector_t::const_iterator evi(ev.begin()), eve(ev.end()); evi != eve; ++evi) {
				if (evi->get_icao() != "EGPX" || evi->get_typecode() != AirspacesDb::Airspace::typecode_ead ||
				    evi->get_bdryclass() != AirspacesDb::Airspace::bdryclass_ead_fir)
					continue;
				if (!evi->get_bbox().is_inside(wpt0.get_coord()))
					continue;
				if (!evi->get_polygon().windingnumber(wpt0.get_coord()))
					continue;
			        england = true;
				break;
			}
		}
		{
			q.query_elevation(wpt0.get_coord(), wpt1.get_coord());
			q.wait();
			TopoDb30::Profile profile(q.getelevation());
			TopoDb30::elev_t maxelev(profile.get_maxelev());
			if (maxelev != TopoDb30::nodata)
				terrain = Point::round<int32_t,double>(Point::m_to_ft_dbl * maxelev);
		}
	}
	if (true)
		std::cerr << "fplpage_semicircular_clicked: LI/LP " << (italy_portugal ? 'Y' : 'N') << " NZ " << (newzealand ? 'Y' : 'N')
			  << " EG " << (england ? 'Y' : 'N') << " course " << Point::round<int,double>(magcrs) << "M"
			  << " GND " << terrain << "ft" << std::endl;
	if (england) {
		// quadrantal rules - not binding for VFR, but apply nevertheless
		base = 0;
		if (magcrs < 180) {
			base += 1000;
			if (magcrs >= 90)
				base += 500;
		} else {
			if (magcrs >= 270)
				base += 500;
		}
	} else {
		if (italy_portugal) {
			if (magcrs >= 90 && magcrs < 270)
				base += 1000;
		} else if (newzealand) {
			if (magcrs < 90 || magcrs >= 270)
				base += 1000;
		} else {
			if (magcrs < 180)
				base += 1000;
		}
	}
	alt = ((alt + 1000 - base) / 2000) * 2000 + base;
	alt = std::max(std::min(alt, (int32_t)32000), (int32_t)0);
	{
		int32_t minalt(terrain);
		if (minalt >= 5000)
			minalt += 1000;
		minalt += 1000;
		if (alt < minalt)
			alt += ((minalt - alt + 1999) / 2000) * 2000;
	}
	wpt0.set_altitude(alt);
	m_sensors->nav_fplan_modified();
	fplpage_select(fpl.get_curwpt());
}

void FlightDeckWindow::fplpage_ifr_changed(void)
{
	unsigned int nr(~0U);
	if (m_fpltreeview) {
		Glib::RefPtr<Gtk::TreeSelection> selection(m_fpltreeview->get_selection());
		if (selection) {
			Gtk::TreeModel::iterator iter(selection->get_selected());
			if (iter) {
				Gtk::TreeModel::Row row(*iter);
				nr = row[m_fplcolumns.m_nr];
			}
		}
	}
	if (true)
		std::cerr << "fplpage_altunit_changed: nr: " << nr << std::endl;
	FPlanRoute& fpl(m_sensors->get_route());
	if (!m_fplifrcheckbutton || nr < 1 || nr > fpl.get_nrwpt())
		return;
	FPlanWaypoint& wpt(fpl[nr - 1]);
	uint16_t flags(wpt.get_flags());
	if (m_fplifrcheckbutton->get_active())
		flags |= FPlanWaypoint::altflag_ifr;
	else
		flags &= ~FPlanWaypoint::altflag_ifr;
	wpt.set_flags(flags);
	m_sensors->nav_fplan_modified();
	fplpage_select(fpl.get_curwpt());
}

void FlightDeckWindow::fplpage_type_changed(void)
{
	unsigned int nr(~0U);
	if (m_fpltreeview) {
		Glib::RefPtr<Gtk::TreeSelection> selection(m_fpltreeview->get_selection());
		if (selection) {
			Gtk::TreeModel::iterator iter(selection->get_selected());
			if (iter) {
				Gtk::TreeModel::Row row(*iter);
				nr = row[m_fplcolumns.m_nr];
			}
		}
	}
	if (true)
		std::cerr << "fplpage_type_changed: nr: " << nr << std::endl;
	FPlanRoute& fpl(m_sensors->get_route());
	if (!m_fpltypecombobox || nr < 1 || nr > fpl.get_nrwpt())
		return;
	Gtk::TreeModel::const_iterator iter(m_fpltypecombobox->get_active());
	if (iter) {
		Gtk::TreeModel::Row row(*iter);
		FPlanWaypoint& wpt(fpl[nr - 1]);
		wpt.set_type(row[m_fpltypecolumns.m_type], row[m_fpltypecolumns.m_navaid_type]);
		m_sensors->nav_fplan_modified();
		fplpage_select(fpl.get_curwpt());
	}
}

void FlightDeckWindow::fplpage_edit_coord(void)
{
	if (!m_coorddialog)
		return;
	m_coorddialog->set_flags(m_navarea.map_get_drawflags());
	if (m_sensors &&
	    m_sensors->get_configfile().has_key(cfggroup_mainwindow, NavArea::cfgkey_mainwindow_mapscale) &&
	    m_sensors->get_configfile().has_key(cfggroup_mainwindow, NavArea::cfgkey_mainwindow_arptmapscale))
		m_coorddialog->set_map_scale(m_sensors->get_configfile().get_double(cfggroup_mainwindow, NavArea::cfgkey_mainwindow_mapscale),
					     m_sensors->get_configfile().get_double(cfggroup_mainwindow, NavArea::cfgkey_mainwindow_arptmapscale));
	m_coorddialog->edit_waypoint();
}

#if 0
void FlightDeckWindow::fplpage_winddir_changed(void)
{
	if (m_fplwinddirspinbutton && m_sensors) {
		m_sensors->set_fplan_winddir(m_fplwinddirspinbutton->get_value());
		m_navarea.hsi_set_glidewind(m_sensors->get_fplan_winddir(), m_sensors->get_fplan_windspeed());
	}
}

void FlightDeckWindow::fplpage_windspeed_changed(void)
{
	if (m_fplwindspeedspinbutton && m_sensors) {
		m_sensors->set_fplan_windspeed(m_fplwindspeedspinbutton->get_value());
		m_navarea.hsi_set_glidewind(m_sensors->get_fplan_winddir(), m_sensors->get_fplan_windspeed());
	}
}
#endif

void FlightDeckWindow::fplpage_insert_clicked(void)
{
	if (!m_sensors)
		return;
	FPlanRoute& fpl(m_sensors->get_route());
        unsigned int nrwpt(fpl.get_nrwpt());
	Point pt;
	if (nrwpt) {
		unsigned int cur(fpl.get_curwpt());
		pt = fpl[std::min(nrwpt - 1U, std::max(cur, 1U) - 1U)].get_coord();
	} else {
		m_sensors->get_position(pt);
	}
	if (false)
		std::cerr << "fplpage_insert: " << pt.get_lat_str2() << ' ' << pt.get_lon_str2() << std::endl;
	wptpage_findcoord(pt);
	set_menu_id(100);
}

void FlightDeckWindow::fplpage_inserttxt_clicked(void)
{
	textentry_dialog_open("Enter Waypoints to Insert", "", sigc::mem_fun(*this, &FlightDeckWindow::fplpage_inserttxt));
}

void FlightDeckWindow::fplpage_inserttxt(bool ok)
{
	if (!ok || !m_textentrydialog || !m_sensors)
                return;
	FPlanRoute& fpl(m_sensors->get_route());
	unsigned int nrwpt(fpl.get_nrwpt());
	unsigned int cur(fpl.get_curwpt());
	cur = std::min(cur, nrwpt);
	std::vector<FPlanWaypoint> wpts;
	bool ref(cur || cur < nrwpt);
	if (ref)
		wpts.push_back(fpl[std::max(cur, 1U) - 1U]);
	IcaoFlightPlan::errors_t err;
	{
		Glib::ustring text(m_textentrydialog->get_text());
		while (!text.empty() && std::isspace(text[0]))
			text.erase(0, 1);
		if (text.empty())
			return;
		if (!text.compare(0, 22, "http://skyvector.com/?")) {
			std::string url(text.substr(22));
			if (true)
				std::cerr << "Skyvector URL parameters: " << url << std::endl;
			while (!url.empty()) {
				std::string::size_type n(url.find('&'));
				std::string comp;
				if (n == std::string::npos) {
					comp.swap(url);
				} else {
					comp = url.substr(0, n);
					url.erase(0, n+1);
				}
				if (!comp.compare(0, 5, "plan=")) {
					url.swap(comp);
					url.erase(0, 5);
					break;
				}
			}
			if (true)
				std::cerr << "Skyvector plan: " << url << std::endl;
			while (!url.empty()) {
				std::string::size_type n(url.find(':'));
				std::string wptn;
				if (n == std::string::npos) {
					wptn.swap(url);
				} else {
					wptn = url.substr(0, n);
					url.erase(0, n+1);
				}
				if (wptn.size() < 2 || wptn[1] != '.')
					continue;
				switch (wptn[0]) {
				case 'G':
				{
					char *cp1(0);
					const char *cp(wptn.c_str() + 2);
					double lat(strtod(cp, &cp1));
					if (cp1 == cp || *cp1 != ',')
						continue;
					cp = cp1;
					double lon(strtod(cp, &cp1));
					if (cp1 == cp || *cp1)
						continue;
					FPlanWaypoint wpt;
					if (!wpts.empty()) {
						wpt.set_altitude(wpts.back().get_altitude());
						wpt.set_flags(wpts.back().get_flags());
					}
					Point pt;
					pt.set_lat_deg_dbl(lat);
					pt.set_lon_deg_dbl(lon);
					wpt.set_coord(pt);
					wpts.push_back(wpt);
					break;
				}

				case 'A':
				case 'V':
				case 'N':
				case 'F':
				{
					std::string::size_type n(wptn.rfind('.'));
					if (n == std::string::npos)
						break;
					std::string ident(wptn, n+1);
					std::string authority;
					if (n >= 2)
						authority = ident.substr(2, n - 2);
					IcaoFlightPlan::FindCoord fc(*m_sensors->get_engine());
					unsigned int flags(0);
					switch (wptn[0]) {
					case 'A':
						flags |= IcaoFlightPlan::FindCoord::flag_airport;
						break;

					case 'V':
					case 'N':
						flags |= IcaoFlightPlan::FindCoord::flag_navaid;
						break;

					case 'F':
						flags |= IcaoFlightPlan::FindCoord::flag_waypoint;
						break;

					default:
						break;
					}
					fc.find_by_ident(ident, "", flags);
					{
						const Engine::AirportResult::elementvector_t& ev(fc.get_airports());
						Engine::AirportResult::elementvector_t::const_iterator i0(ev.end()), i1(ev.end());
						double d0best(std::numeric_limits<double>::max());
						double d1best(std::numeric_limits<double>::max());
						for (Engine::AirportResult::elementvector_t::const_iterator i(ev.begin()), e(ev.end()); i != e; ++i) {
							if (!i->is_valid())
								continue;
							double d(0);
							if (!wpts.empty())
								d = wpts.back().get_coord().spheric_distance_nmi_dbl(i->get_coord());
							if (authority.empty() || !i->is_authority(authority)) {
								if (d0best < d)
									continue;
								d0best = d;
								i0 = i;
								continue;
							}
							if (d1best < d)
								continue;
							d1best = d;
							i1 = i;
						}
						if (i1 == ev.end())
							i1 = i0;
						if (i1 != ev.end()) {
							FPlanWaypoint wpt;
							wpt.set(*i1);
							wpts.push_back(wpt);
							break;
						}
					}
					{
						const Engine::NavaidResult::elementvector_t& ev(fc.get_navaids());
						Engine::NavaidResult::elementvector_t::const_iterator i0(ev.end()), i1(ev.end());
						double d0best(std::numeric_limits<double>::max());
						double d1best(std::numeric_limits<double>::max());
						for (Engine::NavaidResult::elementvector_t::const_iterator i(ev.begin()), e(ev.end()); i != e; ++i) {
							if (!i->is_valid())
								continue;
							double d(0);
							if (!wpts.empty())
								d = wpts.back().get_coord().spheric_distance_nmi_dbl(i->get_coord());
							if (authority.empty() || !i->is_authority(authority)) {
								if (d0best < d)
									continue;
								d0best = d;
								i0 = i;
								continue;
							}
							if (d1best < d)
								continue;
							d1best = d;
							i1 = i;
						}
						if (i1 == ev.end())
							i1 = i0;
						if (i1 != ev.end()) {
							FPlanWaypoint wpt;
							wpt.set(*i1);
							if (!wpts.empty()) {
								wpt.set_altitude(wpts.back().get_altitude());
								wpt.set_flags(wpts.back().get_flags());
							}
							wpts.push_back(wpt);
							break;
						}
					}
					{
						const Engine::WaypointResult::elementvector_t& ev(fc.get_waypoints());
						Engine::WaypointResult::elementvector_t::const_iterator i0(ev.end()), i1(ev.end());
						double d0best(std::numeric_limits<double>::max());
						double d1best(std::numeric_limits<double>::max());
						for (Engine::WaypointResult::elementvector_t::const_iterator i(ev.begin()), e(ev.end()); i != e; ++i) {
							if (!i->is_valid())
								continue;
							double d(0);
							if (!wpts.empty())
								d = wpts.back().get_coord().spheric_distance_nmi_dbl(i->get_coord());
							if (authority.empty() || !i->is_authority(authority)) {
								if (d0best < d)
									continue;
								d0best = d;
								i0 = i;
								continue;
							}
							if (d1best < d)
								continue;
							d1best = d;
							i1 = i;
						}
						if (i1 == ev.end())
							i1 = i0;
						if (i1 != ev.end()) {
							FPlanWaypoint wpt;
							wpt.set(*i1);
							if (!wpts.empty()) {
								wpt.set_altitude(wpts.back().get_altitude());
								wpt.set_flags(wpts.back().get_flags());
							}
							wpts.push_back(wpt);
							break;
						}
					}
					break;
				}

				default:
					break;
				}
			}
		} else if (text.size() >= 5 && text.substr(text[0] == '-', 4) == "(FPL") {
			IcaoFlightPlan f(*m_sensors->get_engine());
			err = f.parse(text, true);
			FPlanRoute fpl(*(FPlan *)0);
			f.set_route(fpl);
			for (unsigned int i(0), n(fpl.get_nrwpt()); i < n; ++i)
				wpts.push_back(fpl[i]);
		} else {
			IcaoFlightPlan f(*m_sensors->get_engine());
			err = f.parse_route(wpts, text, true);
		}
	}
	std::cerr << "After parse: " << wpts.size() << " waypoints" << std::endl;
	for (unsigned int i(ref), n(wpts.size()); i < n; ++i) {
		std::cerr << "Inserting " << cur << '/' << nrwpt << ' ' << wpts[i].get_icao_name() << std::endl;
		m_sensors->nav_insert_wpt(cur, wpts[i]);
		++nrwpt;
		cur = std::min(cur + 1U, nrwpt);
	}
	fpl.set_curwpt(cur);
        m_sensors->nav_fplan_modified();
	if (!err.empty()) {
		for (IcaoFlightPlan::errors_t::const_iterator i(err.begin()), e(err.end()); i != e; ++i)
			std::cerr << "Error: " << *i << std::endl;
		Glib::RefPtr<Gtk::MessageDialog> dlg(new Gtk::MessageDialog(*this, "Flight Deck: Flight Plan Parse Errors",
									    false, Gtk::MESSAGE_ERROR, Gtk::BUTTONS_OK, true));
		dlg->signal_response().connect(sigc::hide(sigc::mem_fun(*dlg.operator->(), &Gtk::MessageDialog::hide)));
		Glib::ustring text;
		for (IcaoFlightPlan::errors_t::const_iterator i(err.begin()), e(err.end()); i != e; ++i)
			text += *i + "\n";
		dlg->set_secondary_text(text, false);
		Gtk::Main::run(*dlg.operator->());
	}
}

void FlightDeckWindow::fplpage_duplicate_clicked(void)
{
	if (!m_sensors)
		return;
	FPlanRoute& fpl(m_sensors->get_route());
        unsigned int cur(fpl.get_curwpt());
        unsigned int nrwpt(fpl.get_nrwpt());
	if (fpl.get_nrwpt() > 0) {
		m_sensors->nav_insert_wpt(cur, fpl[std::min(std::max(cur, 1U), nrwpt) - 1U]);
		++nrwpt;
		fpl.set_curwpt(std::min(cur + 1, nrwpt));
	} else {
		FPlanWaypoint wpt;
		Point pt;
		m_sensors->get_position(pt);
		wpt.set_coord(pt);
		m_sensors->nav_insert_wpt(0, wpt);
		fpl.set_curwpt(1);
	}
	m_sensors->nav_fplan_modified();
}

void FlightDeckWindow::fplpage_moveup_clicked(void)
{
	if (!m_sensors)
		return;
	FPlanRoute& fpl(m_sensors->get_route());
	unsigned int cur(fpl.get_curwpt());
	if (cur < 2 || cur > fpl.get_nrwpt())
		return;
	std::swap(fpl[cur - 2], fpl[cur - 1]);
	fpl.set_curwpt(cur - 1);
	m_sensors->nav_fplan_modified();
}

void FlightDeckWindow::fplpage_movedown_clicked(void)
{
	if (!m_sensors)
		return;
	FPlanRoute& fpl(m_sensors->get_route());
	unsigned int cur(fpl.get_curwpt());
	if (cur < 1 || cur >= fpl.get_nrwpt())
		return;
	std::swap(fpl[cur - 1], fpl[cur]);
	fpl.set_curwpt(cur + 1);
	m_sensors->nav_fplan_modified();
}

void FlightDeckWindow::fplpage_delete_clicked(void)
{
	if (!m_sensors)
		return;
	FPlanRoute& fpl(m_sensors->get_route());
	unsigned int cur(fpl.get_curwpt());
	if (cur < 1 || cur > fpl.get_nrwpt())
		return;
	m_sensors->nav_delete_wpt(cur - 1);
	m_sensors->nav_fplan_modified();
}

void FlightDeckWindow::fplpage_straighten_clicked(void)
{
	if (!m_sensors)
		return;
	FPlanRoute& fpl(m_sensors->get_route());
        unsigned int nr(fpl.get_curwpt());
        unsigned int nrwpt(fpl.get_nrwpt());
        if (nr <= 1 || nr >= nrwpt)
                return;
        Point pt1(fpl[nr-2].get_coord()), pt2(fpl[nr-1].get_coord()), pt3(fpl[nr].get_coord());
        pt3 -= pt1;
        pt2 -= pt1;
        double t = pt3.get_lon() * (double)pt3.get_lon() + pt3.get_lat() * (double)pt3.get_lat();
        if (t < 1)
                return;
        t = (pt2.get_lon() * (double)pt3.get_lon() + pt2.get_lat() * (double)pt3.get_lat()) / t;
        pt3 = Point((int)(pt3.get_lon() * t), (int)(pt3.get_lat() * t));
        pt3 += pt1;
        fpl[nr-1].set_coord(pt3);
	m_sensors->nav_fplan_modified();
}

void FlightDeckWindow::fplpage_directto_clicked(void)
{
	if (m_sensors) {
		const FPlanRoute& fpl(m_sensors->get_route());
		unsigned int cur(fpl.get_curwpt());
		unsigned int nr(fpl.get_nrwpt());
		if (!nr)
			return;
		if (cur < 1) {
			double track(0);
			if (nr >= 2)
				track = fpl[0].get_coord().spheric_true_course(fpl[1].get_coord());
			m_sensors->nav_directto(fpl[0].get_icao_name(), fpl[0].get_coord(), track, fpl[0].get_altitude(), fpl[0].get_flags(), cur);
			return;
		}
		if (cur > nr) {
			double track(0);
			if (nr >= 2)
				track = fpl[nr - 2].get_coord().spheric_true_course(fpl[nr - 1].get_coord());
			m_sensors->nav_directto(fpl[nr - 1].get_icao_name(), fpl[nr - 1].get_coord(), track, fpl[nr - 1].get_altitude(), fpl[nr - 1].get_flags(), cur);
			return;
		}
	}
	if (!m_coorddialog)
		return;
	m_coorddialog->set_flags(m_navarea.map_get_drawflags());
	if (m_sensors &&
	    m_sensors->get_configfile().has_key(cfggroup_mainwindow, NavArea::cfgkey_mainwindow_mapscale) &&
	    m_sensors->get_configfile().has_key(cfggroup_mainwindow, NavArea::cfgkey_mainwindow_arptmapscale))
		m_coorddialog->set_map_scale(m_sensors->get_configfile().get_double(cfggroup_mainwindow, NavArea::cfgkey_mainwindow_mapscale),
					     m_sensors->get_configfile().get_double(cfggroup_mainwindow, NavArea::cfgkey_mainwindow_arptmapscale));
	m_coorddialog->directto_waypoint();
}

void FlightDeckWindow::fplpage_brg2_clicked(void)
{
	if (!m_fplbrg2togglebutton || !m_sensors)
		return;
	bool ena(m_fplbrg2togglebutton->get_active());
	if (ena) {
		FPlanRoute& fpl(m_sensors->get_route());
		unsigned int cur(fpl.get_curwpt());
		if (cur >= 1 || cur <= fpl.get_nrwpt()) {
			FPlanWaypoint& wpt(fpl[cur - 1]);
			m_sensors->nav_set_brg2(wpt.get_icao_name());
			m_sensors->nav_set_brg2(wpt.get_coord());
		}
	}
	m_sensors->nav_set_brg2(ena);
}

void FlightDeckWindow::fplpage_fetchprofilegraph_clicked(void)
{
	if (!m_sensors || !m_wxpicimage)
		return;
	m_fplnwxweather.abort();
	Gtk::Allocation alloc;
	if (!m_mainnotebook || !m_wxpicalignment) {
		alloc.set_width(1024);
		alloc.set_height(768);
	} else {
		int pg(m_mainnotebook->get_current_page());
		set_mainnotebook_page(11);
		alloc = m_wxpicalignment->get_allocation();
		set_mainnotebook_page(pg);
		if (alloc.get_width() < 64)
			alloc.set_width(960);
		if (alloc.get_height() < 48)
			alloc.set_height(720);
	}
	m_fplnwxweather.profile_graph(m_sensors->get_route(), true, sigc::bind(sigc::mem_fun(*this, &FlightDeckWindow::fplpage_fetchprofilegraph_cb), ~0U),
				      alloc.get_width(), alloc.get_height());
}

void FlightDeckWindow::fplpage_fetchprofilegraph_cb(const std::string& pix, const std::string& mimetype, time_t epoch, unsigned int level)
{
	if (!m_wxpicimage || pix.empty())
		return;
	m_wxpicimage->set(pix, mimetype);
	set_menu_id(2002);
	if (m_wxpiclabel) {
		std::ostringstream oss;
		oss << "<b>Profile Graph";
		if (epoch) {
			char buf[64];
			struct tm utm;
			strftime(buf, sizeof(buf), "%Y-%m-%d %H:%M:%Sz", gmtime_r(&epoch, &utm));
			oss << ": " << buf;
		}
		if (level != ~0U)
			oss << ' ' << level << "hPa";
		oss << "</b>";
		m_wxpiclabel->set_label(oss.str());
	}
}

void FlightDeckWindow::fplpage_updatefpl(void)
{
	if (m_coorddialog)
		m_coorddialog->hide();
	if (!m_fplstore) {
		fplpage_select();
		return;
	}
	m_fplselchangeconn.block();
	if (!m_sensors) {
	       	m_fplstore->clear();
		m_fplselchangeconn.unblock();
		fplpage_select();
		return;
	}
	const FPlanRoute& fpl(m_sensors->get_route());
	double tas(m_sensors->get_airdata_tas());
	if (std::isnan(tas) || tas < m_sensors->nav_get_tkoffldgspeed()) {
		if (m_engine)
			tas = m_engine->get_prefs().vcruise;
		else
			tas = 100;
	}
	uint16_t wptnr(m_sensors->nav_get_wptnr());
	const uint16_t nowptnr(~0U);
        WMM wmm;
        time_t curtime = time(0);
        Gdk::Color black, red, green;
        black.set_grey(0);
        red.set_rgb(0xffff, 0, 0);
	Gtk::TreeIter seliter;
	Gtk::TreeIter iter(m_fplstore->children().begin());
	{
		if (!iter)
			iter = m_fplstore->append();
		Gtk::TreeModel::Row row(*iter);
		if (!fpl.get_curwpt())
			seliter = iter;			
		++iter;
		row[m_fplcolumns.m_icao] = "";
		row[m_fplcolumns.m_name] = "Off Block";
		row[m_fplcolumns.m_freq] = "";
		row[m_fplcolumns.m_time] = Conversions::time_str(fpl.get_time_offblock_unix());
		row[m_fplcolumns.m_lon] = "";
		row[m_fplcolumns.m_lat] = "";
		row[m_fplcolumns.m_alt] = "";
		row[m_fplcolumns.m_mt] = "";
		row[m_fplcolumns.m_tt] = "";
		row[m_fplcolumns.m_dist] = "";
		row[m_fplcolumns.m_vertspeed] = "";
		row[m_fplcolumns.m_mh] = "";
		row[m_fplcolumns.m_tas] = "";
		row[m_fplcolumns.m_truealt] = "";
		row[m_fplcolumns.m_power] = "";
		row[m_fplcolumns.m_fuel] = "";
		row[m_fplcolumns.m_pathname] = "";
		row[m_fplcolumns.m_wind] = "";
		row[m_fplcolumns.m_qff] = "";
		row[m_fplcolumns.m_oat] = "";
		row[m_fplcolumns.m_point] = Point();
		if (fpl.get_nrwpt())
			row[m_fplcolumns.m_point] = fpl[0].get_coord();
		row[m_fplcolumns.m_nr] = 0;
		row[m_fplcolumns.m_weight] = (wptnr == 0) ? 600 : 400;
		row[m_fplcolumns.m_style] = (wptnr == 0) ? Pango::STYLE_ITALIC : Pango::STYLE_NORMAL;
		row[m_fplcolumns.m_time_style] = (wptnr <= 0 && wptnr != nowptnr) ? Pango::STYLE_ITALIC : Pango::STYLE_NORMAL;
		row[m_fplcolumns.m_time_color] = (wptnr <= 0 && wptnr != nowptnr) ? red : black;
	}
	for (unsigned int nr = 1; nr <= fpl.get_nrwpt(); ++nr) {
		const FPlanWaypoint& wpt(fpl[nr - 1]);
		if (!iter)
			iter = m_fplstore->append();
		Gtk::TreeModel::Row row(*iter);
		if (fpl.get_curwpt() == nr)
			seliter = iter;			
		++iter;
		row[m_fplcolumns.m_icao] = wpt.get_icao();
		row[m_fplcolumns.m_name] = wpt.get_name();
		row[m_fplcolumns.m_freq] = Conversions::freq_str(wpt.get_frequency());
		row[m_fplcolumns.m_time] = Conversions::time_str(wpt.get_time_unix());
		row[m_fplcolumns.m_lon] = wpt.get_coord().get_lon_str();
		row[m_fplcolumns.m_lat] = wpt.get_coord().get_lat_str();
		row[m_fplcolumns.m_alt] = Conversions::alt_str(wpt.get_altitude(), wpt.get_flags());
		row[m_fplcolumns.m_truealt] = Conversions::alt_str(wpt.get_truealt(), 0);
		{
			Glib::ustring stas;
			double tas(wpt.get_tas_kts());
			if (!std::isnan(tas) && tas > 0) 
				stas = Conversions::velocity_str(tas);
			row[m_fplcolumns.m_tas] = stas;
		}
		{
			Glib::ustring spwr;
			double rpm(wpt.get_rpm()), mp(wpt.get_mp_inhg());
			if (!std::isnan(rpm) && rpm > 0)
				spwr += Glib::ustring::format(std::fixed, std::setprecision(0), rpm);
			if (!std::isnan(mp) && mp > 0)
				spwr += "/" + Glib::ustring::format(std::fixed, std::setprecision(1), mp);
			row[m_fplcolumns.m_power] = spwr;
		}
		{
			Glib::ustring sfuel;
			double fuel(wpt.get_fuel_usg());
			if (!std::isnan(fuel) && fuel >= 0)
				sfuel = Glib::ustring::format(std::fixed, std::setprecision(1), fuel);
			row[m_fplcolumns.m_fuel] = sfuel;
		}
		row[m_fplcolumns.m_pathname] = "";
		if (wpt.get_pathcode() == FPlanWaypoint::pathcode_directto)
			row[m_fplcolumns.m_pathname] = "DCT";
		else if (wpt.get_pathcode() != FPlanWaypoint::pathcode_none)
			row[m_fplcolumns.m_pathname] = wpt.get_pathname();
		row[m_fplcolumns.m_wind] = Conversions::track_str(wpt.get_winddir_deg()) + "/" + Conversions::velocity_str(wpt.get_windspeed_kts());
		row[m_fplcolumns.m_qff] = Glib::ustring::format(std::fixed, std::setprecision(0), wpt.get_qff_hpa());
		row[m_fplcolumns.m_oat] = Glib::ustring::format(std::fixed, std::setprecision(1), wpt.get_oat_kelvin() - IcaoAtmosphere<double>::degc_to_kelvin);
		if (nr < fpl.get_nrwpt()) {
			const FPlanWaypoint& wpt2(fpl[nr]);
			float legtime(wpt2.get_flighttime() - wpt.get_flighttime());
			float vspeed = (wpt2.get_truealt() - wpt.get_truealt()) * 60.0f / std::max(1.0f, legtime);
			row[m_fplcolumns.m_mt] = Conversions::track_str(wpt.get_magnetictrack_deg());
			row[m_fplcolumns.m_tt] = Conversions::track_str(wpt.get_truetrack_deg());
			row[m_fplcolumns.m_dist] = Conversions::dist_str(wpt.get_dist_nmi());
			row[m_fplcolumns.m_vertspeed] = Conversions::verticalspeed_str(vspeed);
			row[m_fplcolumns.m_mh] = Conversions::track_str(wpt.get_magneticheading_deg());
		} else {
			row[m_fplcolumns.m_mt] = "";
			row[m_fplcolumns.m_tt] = "";
			row[m_fplcolumns.m_dist] = "";
			row[m_fplcolumns.m_vertspeed] = "";
			row[m_fplcolumns.m_mh] = "";
		}
		row[m_fplcolumns.m_point] = wpt.get_coord();
		row[m_fplcolumns.m_nr] = nr;
		row[m_fplcolumns.m_weight] = (wptnr == nr) ? 600 : 400;
		row[m_fplcolumns.m_style] = (wptnr == nr) ? Pango::STYLE_ITALIC : Pango::STYLE_NORMAL;
		row[m_fplcolumns.m_time_style] = (wptnr <= nr && wptnr != nowptnr) ? Pango::STYLE_ITALIC : Pango::STYLE_NORMAL;
		row[m_fplcolumns.m_time_color] = (wptnr <= nr && wptnr != nowptnr) ? red : black;
	}
	{
		if (!iter)
			iter = m_fplstore->append();
		Gtk::TreeModel::Row row(*iter);
		if (fpl.get_curwpt() == fpl.get_nrwpt() + 1)
			seliter = iter;
		++iter;
		row[m_fplcolumns.m_icao] = "";
		row[m_fplcolumns.m_name] = "On Block";
		row[m_fplcolumns.m_freq] = "";
		row[m_fplcolumns.m_time] = Conversions::time_str(fpl.get_time_onblock_unix());
		row[m_fplcolumns.m_lon] = "";
		row[m_fplcolumns.m_lat] = "";
		row[m_fplcolumns.m_alt] = "";
		row[m_fplcolumns.m_mt] = "";
		row[m_fplcolumns.m_tt] = "";
		row[m_fplcolumns.m_dist] = "";
		row[m_fplcolumns.m_vertspeed] = "";
		row[m_fplcolumns.m_mh] = "";
		row[m_fplcolumns.m_tas] = "";
		row[m_fplcolumns.m_truealt] = "";
		row[m_fplcolumns.m_power] = "";
		row[m_fplcolumns.m_fuel] = "";
		row[m_fplcolumns.m_pathname] = "";
		row[m_fplcolumns.m_wind] = "";
		row[m_fplcolumns.m_qff] = "";
		row[m_fplcolumns.m_oat] = "";
		unsigned int nr(fpl.get_nrwpt() + 1);
		row[m_fplcolumns.m_point] = Point();
		if (fpl.get_nrwpt())
			row[m_fplcolumns.m_point] = fpl[fpl.get_nrwpt()-1].get_coord();
		row[m_fplcolumns.m_nr] = nr;
		row[m_fplcolumns.m_weight] = (wptnr == nr) ? 600 : 400;
		row[m_fplcolumns.m_style] = (wptnr == nr) ? Pango::STYLE_ITALIC : Pango::STYLE_NORMAL;
		row[m_fplcolumns.m_time_style] = (wptnr <= nr && wptnr != nowptnr) ? Pango::STYLE_ITALIC : Pango::STYLE_NORMAL;
		row[m_fplcolumns.m_time_color] = (wptnr <= nr && wptnr != nowptnr) ? red : black;
	}
	while (iter) {
		iter = m_fplstore->erase(iter);
	}
	if (m_fplnotestextview) {
                Glib::RefPtr<Gtk::TextBuffer> buf(m_fplnotestextview->get_buffer());
                if (buf)
                        buf->set_text(fpl.get_note());
        }
	if (m_fpltreeview) {
		Glib::RefPtr<Gtk::TreeSelection> selection(m_fpltreeview->get_selection());
		if (selection) {
			if (seliter)
				selection->select(seliter);
			else
				selection->unselect_all();
		}
		if (seliter) {
			Gtk::TreeModel::Row row(*seliter);
			unsigned int nr(row[m_fplcolumns.m_nr]);
			fplpage_select(nr);
		} else {
			fplpage_select();
		}
	} else {
		fplpage_select();
	}
	m_fplselchangeconn.unblock();
}

void FlightDeckWindow::fplpage_softkeyenable(void)
{
	unsigned int enamask(0xFFF);
	switch (m_menuid) {
	default:
		return;

	case 2000:
	{
		if (m_sensors) {
			m_softkeys.block(9);
			m_softkeys[9].set_active(m_sensors->nav_is_brg2_enabled());
			m_softkeys.unblock(9);
		}
		enamask = 0xE83;
		if (!m_fpltreeview)
			break;
		Glib::RefPtr<Gtk::TreeSelection> selection(m_fpltreeview->get_selection());
		if (!selection)
			break;
		Gtk::TreeModel::iterator iter(selection->get_selected());
		if (!iter)
			break;
		Gtk::TreeModel::Row row(*iter);
		unsigned int nr(row[m_fplcolumns.m_nr]);
		if (!m_sensors || nr < 1) {
			enamask = 0xF83;
			break;
		}
		const FPlanRoute& fpl(m_sensors->get_route());
		if (nr > fpl.get_nrwpt()) {
			enamask = 0xF83;
			break;
		}
		const FPlanWaypoint& wpt(fpl[nr - 1]);
		enamask = 0xFA7;
		if (nr > 1)
			enamask |= 0x008;
		if (nr < fpl.get_nrwpt())
			enamask |= 0x010;
		if (nr > 1 && nr < fpl.get_nrwpt())
			enamask |= 0x040;
		break;
	}

	case 2001:
		break;
	}
	for (unsigned int i = 0; i < 12; ++i) {
		m_softkeys.block(i);
		m_softkeys[i].set_sensitive(!!(enamask & (1U << i)));
		m_softkeys.unblock(i);
	}
}

void FlightDeckWindow::fplpage_updatemenu(void)
{
	m_fplselchangeconn.block();
	if (m_fplstore)
	       	m_fplstore->clear();
	m_fplselchangeconn.unblock();
	if (m_coorddialog)
		m_coorddialog->hide();
	fplpage_select();
	if ((m_menuid != 200 && m_menuid != 2000 && m_menuid != 2001) ||
	    !m_sensors || !m_fplstore) {
		if (m_fpldrawingarea) {
			m_fpldrawingarea->set_renderer(VectorMapArea::renderer_none);
			m_fpldrawingarea->hide();
		}
		return;
	}
	fplpage_updatefpl();
	if (m_fpldrawingarea) {
		m_fpldrawingarea->set_renderer(VectorMapArea::renderer_vmap);
		*m_fpldrawingarea = m_navarea.map_get_drawflags();
		if (m_sensors && m_sensors->get_configfile().has_group(cfggroup_mainwindow)) {
			if (false && m_sensors->get_configfile().has_key(cfggroup_mainwindow, NavArea::cfgkey_mainwindow_mapflags))
				*m_fpldrawingarea = (MapRenderer::DrawFlags)m_sensors->get_configfile().get_integer(cfggroup_mainwindow, NavArea::cfgkey_mainwindow_mapflags);
			if (m_sensors->get_configfile().has_key(cfggroup_mainwindow, NavArea::cfgkey_mainwindow_mapscale))
				m_fpldrawingarea->set_map_scale(m_sensors->get_configfile().get_double(cfggroup_mainwindow, NavArea::cfgkey_mainwindow_mapscale));
		}
		m_fpldrawingarea->show();
	}
}
