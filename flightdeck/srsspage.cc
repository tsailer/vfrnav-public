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
#include "SunriseSunset.h"
#include "icaofpl.h"

FlightDeckWindow::SunriseSunsetWaypointListModelColumns::SunriseSunsetWaypointListModelColumns(void)
{
	add(m_icao);
	add(m_name);
	add(m_sunrise);
	add(m_sunset);
	add(m_lon);
	add(m_lat);
	add(m_mt);
	add(m_tt);
	add(m_dist);
	add(m_point);
	add(m_nr);
}

void FlightDeckWindow::srsspage_recompute(void)
{
	if (!m_srssstore)
		return;
	if (!m_srsscalendar) {
		for (Gtk::TreeIter iter = m_srssstore->children().begin(); iter; iter++) {
			Gtk::TreeModel::Row row(*iter);
			row[m_srsscolumns.m_sunrise] = "";
			row[m_srsscolumns.m_sunset] = "";
		}
		return;
	}
	Glib::Date dt;
	m_srsscalendar->get_date(dt);
        for (Gtk::TreeIter iter = m_srssstore->children().begin(); iter; iter++) {
                Gtk::TreeModel::Row row(*iter);
                double sr, ss;
                int r = SunriseSunset::civil_twilight(dt.get_year(), dt.get_month(), dt.get_day(), row[m_srsscolumns.m_point], sr, ss);
                if (r) {
                        row[m_srsscolumns.m_sunrise] = _("always");
                        row[m_srsscolumns.m_sunset] = (r < 0) ? _("night") : _("day");
                        continue;
                }
                if (sr < 0.0)
                        sr += 24.0;
                if (ss < 0.0)
                        ss += 24.0;
                int h = (int)sr;
                int m = (int)((sr - h) * 60 + 0.5);
                char buf[8];
                snprintf(buf, sizeof(buf), "%02d:%02d", h, m);
                row[m_srsscolumns.m_sunrise] = buf;
                h = (int)ss;
                m = (int)((ss - h) * 60 + 0.5);
                snprintf(buf, sizeof(buf), "%02d:%02d", h, m);
                row[m_srsscolumns.m_sunset] = buf;
        }
}

void FlightDeckWindow::srsspage_updatemenu(void)
{
	if (m_srssstore)
		m_srssstore->clear();
	if (m_menuid != 303)
		return;
	if (m_srssstore && m_sensors) {
		const FPlanRoute& fpl(m_sensors->get_route());
		for (unsigned int nr = 0; nr < fpl.get_nrwpt(); ++nr) {
			Gtk::TreeModel::Row row(*(m_srssstore->append()));
			const FPlanWaypoint& wpt(fpl[nr]);
			row[m_srsscolumns.m_icao] = wpt.get_icao();
			row[m_srsscolumns.m_name] = wpt.get_name();
			row[m_srsscolumns.m_lon] = wpt.get_coord().get_lon_str();
			row[m_srsscolumns.m_lat] = wpt.get_coord().get_lat_str();
			if (nr + 1 < fpl.get_nrwpt()) {
				const FPlanWaypoint& wpt2(fpl[nr + 1]);
				Point pt1(wpt.get_coord()), pt2(wpt2.get_coord());
				float dist = pt1.spheric_distance_nmi(pt2);
				float tt = pt1.spheric_true_course(pt2);
				WMM wmm;
				wmm.compute((wpt.get_altitude() + wpt2.get_altitude()) * (FPlanWaypoint::ft_to_m / 2000.0), pt1.halfway(pt2), time(0));
				float mt = tt;
				if (wmm)
					mt -= wmm.get_dec();
				row[m_srsscolumns.m_mt] = Conversions::track_str(mt);
				row[m_srsscolumns.m_tt] = Conversions::track_str(tt);
				row[m_srsscolumns.m_dist] = Conversions::dist_str(dist);;
			} else {
				row[m_srsscolumns.m_mt] = "";
				row[m_srsscolumns.m_tt] = "";
				row[m_srsscolumns.m_dist] = "";
			}
			row[m_srsscolumns.m_point] = wpt.get_coord();
			row[m_srsscolumns.m_nr] = nr + 1;
		}
	}
	if (m_srsscalendar) {
		m_srsscalendar->clear_marks();
		Glib::Date today;
		today.set_time_current();
		m_srsscalendar->select_month(today.get_month() - 1, today.get_year());
		m_srsscalendar->select_day(today.get_day());
		m_srsscalendar->mark_day(today.get_day());
	}
	srsspage_recompute();
	euops1_recomputetdzlist();
	euops1_recomputeldg();
	euops1_recomputetkoff();
}

FlightDeckWindow::EUOPS1TDZListModelColumns::EUOPS1TDZListModelColumns(void)
{
	add(m_text);
	add(m_elev);
}

const int FlightDeckWindow::m_euops1systemminima[] = {
	200, // ILS Cat I
	250, // LLZ
	300, // VOR
	250, // VOR/DME
	300, // NDB
	300, // VDF (QDM and QGH)
	250, // SRA (terminating 1/2NM)
	300, // SRA (terminating 1NM)
	350, // SRA (terminating 2NM)
	400 // circling (Cat A)
};

const unsigned int FlightDeckWindow::m_euops1apchrvr[][5] = {
	210, 550, 750, 1000, 1200,
	220, 550, 800, 1000, 1200,
	230, 550, 800, 1000, 1200,
	240, 550, 800, 1000, 1200,
	250, 550, 800, 1000, 1300,
	260, 600, 800, 1100, 1300,
	280, 600, 900, 1100, 1300,
	300, 650, 900, 1200, 1400,
	320, 700, 1000, 1200, 1400,
	340, 800, 1100, 1300, 1500,
	360, 900, 1200, 1400, 1600,
	380, 1000, 1300, 1500, 1700,
	400, 1100, 1400, 1600, 1800,
	420, 1200, 1500, 1700, 1900,
	440, 1300, 1600, 1800, 2000,
	460, 1400, 1700, 1900, 2100,
	480, 1500, 1800, 2000, 2200,
	500, 1500, 1800, 2100, 2300,
	520, 1600, 1900, 2100, 2400,
	540, 1700, 2000, 2200, 2400,
	560, 1800, 2100, 2300, 2500,
	580, 1900, 2200, 2400, 2600,
	600, 2000, 2300, 2500, 2700,
	620, 2100, 2400, 2600, 2800,
	640, 2200, 2500, 2700, 2900,
	660, 2300, 2600, 2800, 3000,
	680, 2400, 2700, 2900, 3100,
	700, 2500, 2800, 3000, 3200,
	720, 2600, 2900, 3100, 3300,
	740, 2700, 3000, 3200, 3400,
	760, 2700, 3000, 3300, 3500,
	800, 2900, 3200, 3400, 3600,
	850, 3100, 3400, 3600, 3800,
	900, 3300, 3600, 3800, 4000,
	950, 3600, 3900, 4100, 4300,
	1000, 3800, 4100, 4300, 4500,
	1100, 4100, 4400, 4600, 4900,
	1200, 4600, 4900, 5000, 5000,
	~0U, 5000, 5000, 5000, 5000
};

void FlightDeckWindow::euops1_recomputetdzlist(void)
{
	if (!m_euops1tdzstore)
		return;
	m_euops1tdzstore->clear();
	if (m_sensors) {
		const FPlanRoute& fpl(m_sensors->get_route());
		for (unsigned int nr = fpl.get_nrwpt(); nr; ) {
			--nr;
			const FPlanWaypoint& wpt(fpl[nr]);
			if (nr > 1)
				nr = 1;
			IcaoFlightPlan::FindCoord f(*m_sensors->get_engine());
			AirportsDb::Airport arpt;
			if ((!f.find(wpt.get_icao(), wpt.get_name(), IcaoFlightPlan::FindCoord::flag_airport, wpt.get_coord()) &&
			     !f.find(wpt.get_coord(), IcaoFlightPlan::FindCoord::flag_airport, 0.1)) ||
			    !f.get_airport(arpt) || !arpt.is_valid()) {
				Gtk::TreeModel::Row row(*(m_euops1tdzstore->append()));
				row[m_euops1tdzcolumns.m_text] = wpt.get_icao_name();
				row[m_euops1tdzcolumns.m_elev] = wpt.get_altitude();
				continue;
			}
			for (unsigned int ri = 0; ri < arpt.get_nrrwy(); ++ri) {
                                const AirportsDb::Airport::Runway& rwy(arpt.get_rwy(ri));
				if (rwy.is_le_usable()) {
					Gtk::TreeModel::Row row(*(m_euops1tdzstore->append()));
					row[m_euops1tdzcolumns.m_text] = arpt.get_icao_name() + " RWY" + rwy.get_ident_le();
					row[m_euops1tdzcolumns.m_elev] = rwy.get_le_elev();
				}
				if (rwy.is_he_usable()) {
					Gtk::TreeModel::Row row(*(m_euops1tdzstore->append()));
					row[m_euops1tdzcolumns.m_text] = arpt.get_icao_name() + " RWY" + rwy.get_ident_he();
					row[m_euops1tdzcolumns.m_elev] = rwy.get_he_elev();
				}
			}
			Gtk::TreeModel::Row row(*(m_euops1tdzstore->append()));
			row[m_euops1tdzcolumns.m_text] = arpt.get_icao_name();
			row[m_euops1tdzcolumns.m_elev] = arpt.get_elevation();
		}
	}
}

void FlightDeckWindow::euops1_recomputeldg(void)
{
	if (m_euops1comboboxtdzlist && m_euops1tdzstore) {
		Gtk::TreeModel::const_iterator iter(m_euops1tdzstore->children().begin());
		Gtk::TreeModel::const_iterator bestiter;
		double bestdist(10.001);
		while (iter) {
			Gtk::TreeModel::Row row(*iter);
			double d(row[m_euops1tdzcolumns.m_elev]);
			d -= m_euops1tdzelev;
			d = fabs(d);
			if (d < bestdist) {
				bestiter = iter;
				bestdist = d;
			}
			++iter;
		}
		m_euops1tdzlistchange.block();
		m_euops1comboboxtdzlist->set_active(bestiter);
		m_euops1tdzlistchange.unblock();
	}
	double dh(m_euops1ocaoch);
	if (!m_euops1comboboxocaoch || !m_euops1comboboxocaoch->get_active_row_number())
		dh -= m_euops1tdzelev;
	bool precapch(true);
	bool circling(false);
	if (m_euops1comboboxapproach) {
		int apchnr(m_euops1comboboxapproach->get_active_row_number());
		precapch = !apchnr;
		circling = apchnr == 9U;
		if (apchnr >= 0 && apchnr < sizeof(m_euops1systemminima)/sizeof(m_euops1systemminima[0]))
			dh = std::max(dh, (double)m_euops1systemminima[apchnr]);
	}
	m_euops1minimum = m_euops1tdzelev + dh;
	if (m_euops1labeldamda)
		m_euops1labeldamda->set_text(precapch ? "DA" : "MDA");
	if (m_euops1entrydamda) {
		std::ostringstream oss;
		oss << Point::round<int,double>(m_euops1minimum) << "ft";
		m_euops1entrydamda->set_text(oss.str());
	}
	unsigned int apchrvridx(0);
	for (; apchrvridx < sizeof(m_euops1apchrvr)/sizeof(m_euops1apchrvr[0])-1U; ++apchrvridx)
		if (dh <= m_euops1apchrvr[apchrvridx][0])
			break;
	unsigned int lightidx(0);
	if (m_euops1comboboxlight) {
		lightidx = m_euops1comboboxlight->get_active_row_number();
		if (lightidx >= 4U)
			lightidx = 3U;
	}
	unsigned int rvr(m_euops1apchrvr[apchrvridx][1U + lightidx]);
	if (circling)
		rvr = std::max(rvr, 1500U); // Cat A
	unsigned int metvis(rvr);
	if (m_euops1comboboxmetvis && m_euops1comboboxmetvis->get_active_row_number()) {
		switch (lightidx) {
		case 0:
			metvis >>= 1;
			break;

		case 1:
		case 2:
			metvis *= (1.0 / 1.5);
			break;

		default:
			metvis = 0;
			break;
		}
	} else {
		if (!lightidx)
			metvis *= (1.0 / 1.5);
	}
	if (rvr < 800)
		metvis = 0;
	if (m_euops1entryrvr) {
		std::ostringstream oss;
		oss << rvr;
		m_euops1entryrvr->set_text(oss.str());
	}
	if (m_euops1entrymetvis) {
		std::ostringstream oss;
		if (metvis)
			oss << metvis;
		else
			oss << "---";
		m_euops1entrymetvis->set_text(oss.str());
	}
}

void FlightDeckWindow::euops1_recomputetkoff(void)
{
	bool edgelight(m_euops1checkbuttonrwyedgelight && m_euops1checkbuttonrwyedgelight->get_active());
	bool centreline(m_euops1checkbuttonrwycentreline && m_euops1checkbuttonrwycentreline->get_active());
	bool multirvr(m_euops1checkbuttonmultiplervr && m_euops1checkbuttonmultiplervr->get_active());
	int rvr(500), metvis(500);
	if (edgelight && centreline) {
		rvr = 200;
		metvis = 250;
		if (multirvr) {
			rvr = 150;
			metvis = 200;
		}
	} else if (edgelight || centreline) {
		rvr = 250;
		metvis = 300;
	}
	if (m_euops1entrytkoffrvr) {
		std::ostringstream oss;
		oss << rvr;
		m_euops1entrytkoffrvr->set_text(oss.str());
	}
	if (m_euops1entrytkoffmetvis) {
		std::ostringstream oss;
		oss << metvis;
		m_euops1entrytkoffmetvis->set_text(oss.str());
	}
}

void FlightDeckWindow::euops1_setminimum(void)
{
	m_navarea.alt_set_minimum(Point::round<int,double>(m_euops1minimum));
}

void FlightDeckWindow::euops1_change_tdzlist(void)
{
	if (!m_euops1comboboxtdzlist)
		return;
	Gtk::TreeModel::const_iterator iter(m_euops1comboboxtdzlist->get_active());
	if (!iter)
		return;
	Gtk::TreeModel::Row row(*iter);
	double elev(row[m_euops1tdzcolumns.m_elev]);
	if (m_euops1tdzelev == elev)
		return;
	m_euops1tdzelev = elev;
	euops1_change_tdzunit();
	euops1_recomputeldg();
}

void FlightDeckWindow::euops1_change_tdzelev(void)
{
	if (!m_euops1spinbuttontdzelev)
		return;
	double elev(m_euops1spinbuttontdzelev->get_value());
	if (m_euops1comboboxtdzelevunit && m_euops1comboboxtdzelevunit->get_active_row_number())
		elev *= Point::m_to_ft_dbl;
	if (m_euops1tdzelev == elev)
		return;
	m_euops1tdzelev = elev;
	euops1_recomputeldg();
}

void FlightDeckWindow::euops1_change_tdzunit(void)
{
	if (m_euops1spinbuttontdzelev) {
		double elev(m_euops1tdzelev);
		if (m_euops1comboboxtdzelevunit && m_euops1comboboxtdzelevunit->get_active_row_number())
			elev *= 1.0 / Point::m_to_ft_dbl;
		m_euops1tdzelevchange.block();
		m_euops1spinbuttontdzelev->set_value(elev);
		m_euops1tdzelevchange.unblock();
	}
}

void FlightDeckWindow::euops1_change_ocaoch(void)
{
	if (!m_euops1spinbuttonocaoch)
		return;
	double ocaoch(m_euops1spinbuttonocaoch->get_value());
	if (m_euops1comboboxocaochunit && m_euops1comboboxocaochunit->get_active_row_number())
		ocaoch *= Point::m_to_ft_dbl;
	if (m_euops1ocaoch == ocaoch)
		return;
	m_euops1ocaoch = ocaoch;
	euops1_recomputeldg();
}

void FlightDeckWindow::euops1_change_ocaochunit(void)
{
	if (m_euops1spinbuttonocaoch) {
		double ocaoch(m_euops1ocaoch);
		if (m_euops1comboboxocaochunit && m_euops1comboboxocaochunit->get_active_row_number())
			ocaoch *= 1.0 / Point::m_to_ft_dbl;
		m_euops1ocaochchange.block();
		m_euops1spinbuttonocaoch->set_value(ocaoch);
		m_euops1ocaochchange.unblock();
	}
}
