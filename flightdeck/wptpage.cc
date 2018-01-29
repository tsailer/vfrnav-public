/***************************************************************************
 *   Copyright (C) 2007, 2008, 2009, 2011, 2012, 2013, 2015, 2016          *
 *       by Thomas Sailer t.sailer@alumni.ethz.ch                          *
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
#include "wind.h"

const unsigned int FlightDeckWindow::WaypointAirportModelColumns::noid;
FlightDeckWindow::WaypointAirportModelColumns::WaypointAirportModelColumns(void)
{
	add(m_icao);
	add(m_name);
	add(m_type);
	add(m_elev);
	add(m_rwylength);
	add(m_rwywidth);
	add(m_rwyident);
	add(m_rwysfc);
	add(m_comm);
	add(m_dist);
	add(m_routeid);
	add(m_routepointid);
	add(m_style);
	add(m_color);
	add(m_element);
}

FlightDeckWindow::WaypointAirportRunwayModelColumns::WaypointAirportRunwayModelColumns(void)
{
	add(m_ident);
	add(m_hdg);
	add(m_length);
	add(m_width);
	add(m_tda);
	add(m_lda);
	add(m_elev);
	add(m_surface);
	add(m_light);
	add(m_color);
}

FlightDeckWindow::WaypointAirportHeliportModelColumns::WaypointAirportHeliportModelColumns(void)
{
	add(m_ident);
	add(m_hdg);
	add(m_length);
	add(m_width);
	add(m_elev);
}

FlightDeckWindow::WaypointAirportCommModelColumns::WaypointAirportCommModelColumns(void)
{
	add(m_name);
	add(m_freq0);
	add(m_freq1);
	add(m_freq2);
	add(m_freq3);
	add(m_freq4);
	add(m_sector);
	add(m_oprhours);
}

FlightDeckWindow::WaypointAirportFASModelColumns::WaypointAirportFASModelColumns(void)
{
	add(m_ident);
	add(m_runway);
	add(m_route);
	add(m_glide);
	add(m_ftpalt);
	add(m_tch);
}

FlightDeckWindow::WaypointNavaidModelColumns::WaypointNavaidModelColumns(void)
{
	add(m_icao);
	add(m_name);
	add(m_type);
	add(m_elev);
	add(m_freq);
	add(m_range);
	add(m_dist);
	add(m_element);
}

FlightDeckWindow::WaypointIntersectionModelColumns::WaypointIntersectionModelColumns(void)
{
	add(m_name);
	add(m_usage);
	add(m_dist);
	add(m_element);
}

FlightDeckWindow::WaypointAirwayModelColumns::WaypointAirwayModelColumns(void)
{
	add(m_name);
	add(m_fromname);
	add(m_toname);
	add(m_basealt);
	add(m_topalt);
	add(m_terrainelev);
	add(m_corridor5elev);
	add(m_type);
	add(m_dist);
	add(m_element);
}

FlightDeckWindow::WaypointAirspaceModelColumns::WaypointAirspaceModelColumns(void)
{
	add(m_icao);
	add(m_name);
	add(m_class);
	add(m_lwralt);
	add(m_upralt);
	add(m_comm);
	add(m_comm0);
	add(m_comm1);
	add(m_dist);
	add(m_element);
}

FlightDeckWindow::WaypointMapelementModelColumns::WaypointMapelementModelColumns(void)
{
	add(m_name);
	add(m_type);
	add(m_dist);
	add(m_element);
}

FlightDeckWindow::WaypointFPlanModelColumns::WaypointFPlanModelColumns(void)
{
	add(m_icao);
	add(m_name);
	add(m_altitude);
	add(m_freq);
	add(m_type);
	add(m_dist);
	add(m_element);
}

class FlightDeckWindow::WptFPlanCompare : public std::less<FPlanWaypoint> {
public:
	bool operator()(const FPlanWaypoint& x, const FPlanWaypoint& y);
};

bool FlightDeckWindow::WptFPlanCompare::operator()(const FPlanWaypoint& x, const FPlanWaypoint& y)
{
	int c(x.get_icao().compare(y.get_icao()));
	if (c < 0)
		return true;
	if (c > 0)
		return false;
	c = x.get_name().compare(y.get_name());
	if (c < 0)
		return true;
	if (c > 0)
		return false;
	if (x.get_coord().get_lat() < y.get_coord().get_lat())
		return true;
	if (x.get_coord().get_lat() > y.get_coord().get_lat())
		return false;
	if (x.get_coord().get_lon() < y.get_coord().get_lon())
		return true;
	if (x.get_coord().get_lon() > y.get_coord().get_lon())
		return false;
	if (x.get_altitude() < y.get_altitude())
		return true;
	if (x.get_altitude() > y.get_altitude())
		return false;
	if (x.get_flags() < y.get_flags())
		return true;
	if (x.get_flags() > y.get_flags())
		return false;
	c = x.get_note().compare(y.get_note());
	return c < 0;
}

int FlightDeckWindow::compare_dist(const Glib::ustring& sa, const Glib::ustring& sb)
{
        if (sa.empty() && !sb.empty())
                return -1;
        if (!sa.empty() && sb.empty())
                return 1;
        if (sa.empty() && sb.empty())
                return 0;
        double va(strtod(sa.c_str(), 0)), vb(strtod(sb.c_str(), 0));
        return compare_dist(va, vb);
}

int FlightDeckWindow::compare_dist(double da, double db)
{
        if (da < db)
                return -1;
        if (da > db)
                return 1;
        return 0;
}

int FlightDeckWindow::wptpage_sortdist(const Gtk::TreeModelColumn<Glib::ustring>& col, const Gtk::TreeModel::iterator& ia, const Gtk::TreeModel::iterator& ib)
{
	const Gtk::TreeModel::Row rowa(*ia), rowb(*ib);
        return compare_dist(rowa[col], rowb[col]);
}

int FlightDeckWindow::wptpage_airport_sortcol(const Gtk::TreeModelColumn<Glib::ustring>& col, const Gtk::TreeModel::iterator& ia, const Gtk::TreeModel::iterator& ib)
{
	const Gtk::TreeModel::Row rowa(*ia), rowb(*ib);
	WaypointAirportModelColumns::element_ptr_t ela(rowa[m_wptairportcolumns.m_element]);
	WaypointAirportModelColumns::element_ptr_t elb(rowb[m_wptairportcolumns.m_element]);
	unsigned int routeida(rowa[m_wptairportcolumns.m_routeid]);
	unsigned int routeidb(rowb[m_wptairportcolumns.m_routeid]);
	unsigned int routepointida(rowa[m_wptairportcolumns.m_routepointid]);
	unsigned int routepointidb(rowb[m_wptairportcolumns.m_routepointid]);

	if (ela == elb && routeida != WaypointAirportModelColumns::noid && routeida == routeidb &&
	    routepointida != WaypointAirportModelColumns::noid && routepointidb != WaypointAirportModelColumns::noid) {
		if (routepointida < routepointidb)
			return -1;
		if (routepointida > routepointidb)
			return 1;
		return 0;
	}
	const Glib::ustring& flda(rowa[col]);
	const Glib::ustring& fldb(rowb[col]);
        return flda.compare(fldb);
}

int FlightDeckWindow::wptpage_airport_sortdist(const Gtk::TreeModel::iterator& ia, const Gtk::TreeModel::iterator& ib)
{
	const Gtk::TreeModel::Row rowa(*ia), rowb(*ib);
	WaypointAirportModelColumns::element_ptr_t ela(rowa[m_wptairportcolumns.m_element]);
	WaypointAirportModelColumns::element_ptr_t elb(rowb[m_wptairportcolumns.m_element]);
	unsigned int routeida(rowa[m_wptairportcolumns.m_routeid]);
	unsigned int routeidb(rowb[m_wptairportcolumns.m_routeid]);
	unsigned int routepointida(rowa[m_wptairportcolumns.m_routepointid]);
	unsigned int routepointidb(rowb[m_wptairportcolumns.m_routepointid]);

	if (ela == elb && routeida != WaypointAirportModelColumns::noid && routeida == routeidb &&
	    routepointida != WaypointAirportModelColumns::noid && routepointidb != WaypointAirportModelColumns::noid) {
		if (routepointida < routepointidb)
			return -1;
		if (routepointida > routepointidb)
			return 1;
		return 0;
	}
        return compare_dist(rowa[m_wptairportcolumns.m_dist], rowb[m_wptairportcolumns.m_dist]);
}

int FlightDeckWindow::wptpage_navaid_sortdist(const Gtk::TreeModel::iterator& ia, const Gtk::TreeModel::iterator& ib)
{
	return wptpage_sortdist(m_wptnavaidcolumns.m_dist, ia, ib);
}

int FlightDeckWindow::wptpage_waypoint_sortdist(const Gtk::TreeModel::iterator& ia, const Gtk::TreeModel::iterator& ib)
{
	return wptpage_sortdist(m_wptintersectioncolumns.m_dist, ia, ib);
}

int FlightDeckWindow::wptpage_airway_sortdist(const Gtk::TreeModel::iterator& ia, const Gtk::TreeModel::iterator& ib)
{
	return wptpage_sortdist(m_wptairwaycolumns.m_dist, ia, ib);
}

int FlightDeckWindow::wptpage_airspace_sortdist(const Gtk::TreeModel::iterator& ia, const Gtk::TreeModel::iterator& ib)
{
	return wptpage_sortdist(m_wptairspacecolumns.m_dist, ia, ib);
}

int FlightDeckWindow::wptpage_mapelement_sortdist(const Gtk::TreeModel::iterator& ia, const Gtk::TreeModel::iterator& ib)
{
	return wptpage_sortdist(m_wptmapelementcolumns.m_dist, ia, ib);
}

int FlightDeckWindow::wptpage_fplan_sortdist(const Gtk::TreeModel::iterator& ia, const Gtk::TreeModel::iterator& ib)
{
	return wptpage_sortdist(m_wptfplancolumns.m_dist, ia, ib);
}

template <typename T> void FlightDeckWindow::wptpage_updatedist(Gtk::TreeModel::Row& row, const T& cols)
{
	const typename T::element_t& el(row[cols.m_element]);
	row[cols.m_dist] = Conversions::dist_str(m_wptquerypoint.spheric_distance_nmi(el.get_coord()));
}

template <> void FlightDeckWindow::wptpage_updatedist(Gtk::TreeModel::Row& row, const WaypointAirportModelColumns& cols)
{
	const WaypointAirportModelColumns::element_ptr_t& el(row[cols.m_element]);
	unsigned int routeid(row[cols.m_routeid]);
	if (routeid == WaypointAirportModelColumns::noid || routeid >= el->get_nrvfrrte()) {
		double dist(m_wptquerypoint.spheric_distance_nmi(el->get_coord()));
		row[cols.m_dist] = Conversions::dist_str(dist);
		Pango::Style style(Pango::STYLE_NORMAL);
		if (m_sensors) {
			double baroalt, truealt;
			m_sensors->get_altitude(baroalt, truealt);
			const Aircraft& acft(m_sensors->get_aircraft());
			double gd((truealt - el->get_elevation()) * acft.get_glide().get_glideslope() *
				  (Point::ft_to_m_dbl * 0.001 * Point::km_to_nmi_dbl));
			if (!std::isnan(gd)) {
				Sensors::fplanwind_t fpwind(m_sensors->get_fplan_wind(el->get_coord()));
				if (fpwind.second > 0) {
					Wind<double> wind;
					wind.set_wind(fpwind.first, fpwind.second);
					wind.set_crs_tas(m_wptquerypoint.spheric_true_course(el->get_coord()), acft.get_glide().get_vbestglide());
					if (std::isnan(wind.get_gs()) || wind.get_gs() <= 0)
						gd = 0;
					else
						gd *= wind.get_gs() / wind.get_tas();
				}
				if (gd > dist)
					style = Pango::STYLE_ITALIC;
			}
		}
		row[m_wptairportcolumns.m_style] = style;
		return;
	}
	const AirportsDb::Airport::VFRRoute& vfrrte(el->get_vfrrte(routeid));
	unsigned int routepointid(row[cols.m_routepointid]);
	if (routepointid == WaypointAirportModelColumns::noid || routepointid >= vfrrte.size()) {
		Point pt(el->get_coord());
		if (vfrrte.size() > 0) {
			const AirportsDb::Airport::VFRRoute::VFRRoutePoint& pt1(vfrrte[0]);
			if (!pt1.is_at_airport() && pt1.get_coord() != pt)
				pt = pt1.get_coord();
			if (vfrrte.size() > 1) {
				const AirportsDb::Airport::VFRRoute::VFRRoutePoint& pt1(vfrrte[vfrrte.size()-1]);
				if (!pt1.is_at_airport() && pt1.get_coord() != pt)
					pt = pt1.get_coord();
				
			}
		}
		row[cols.m_dist] = Conversions::dist_str(m_wptquerypoint.spheric_distance_nmi(pt));
		return;
	}
	const AirportsDb::Airport::VFRRoute::VFRRoutePoint& pt1(vfrrte[routepointid]);
	row[cols.m_dist] = Conversions::dist_str(m_wptquerypoint.spheric_distance_nmi(pt1.get_coord()));
}

template <> void FlightDeckWindow::wptpage_updatedist(Gtk::TreeModel::Row& row, const WaypointAirwayModelColumns& cols)
{
	const WaypointAirwayModelColumns::element_t& el(row[cols.m_element]);
	float d(std::min(m_wptquerypoint.spheric_distance_nmi(el.get_begin_coord()),
			 m_wptquerypoint.spheric_distance_nmi(el.get_end_coord())));
	row[cols.m_dist] = Conversions::dist_str(d);
}

template <typename T> void FlightDeckWindow::wptpage_updatedist(const T& cols, Gtk::TreeIter it, Gtk::TreeIter itend)
{
	for (; it != itend; ++it) {
		Gtk::TreeModel::Row row(*it);
		wptpage_updatedist(row, cols);
		wptpage_updatedist(cols, row.children().begin(), row.children().end());
	}
}

template <typename T> void FlightDeckWindow::wptpage_updatedist(const T& cols)
{
	if (!m_wptstore)
		return;
	wptpage_updatedist(cols, m_wptstore->children().begin(), m_wptstore->children().end());
}

void FlightDeckWindow::wptpage_clearall(void)
{
	m_wptqueryrect = Rect(Point(0, Point::pole_lat), Point(0, Point::pole_lat));
	wptpage_cancel();
	wptpage_select();
}

void FlightDeckWindow::wptpage_cancel(void)
{
	if (m_airportquery) {
		Glib::RefPtr<Engine::AirportResult> q(m_airportquery);
		m_airportquery.reset();
		Engine::AirportResult::cancel(q);
	}
	if (m_airspacequery) {
		Glib::RefPtr<Engine::AirspaceResult> q(m_airspacequery);
		m_airspacequery.reset();
		Engine::AirspaceResult::cancel(q);
	}
	if (m_navaidquery) {
		Glib::RefPtr<Engine::NavaidResult> q(m_navaidquery);
		m_navaidquery.reset();
		Engine::NavaidResult::cancel(q);
	}
	if (m_waypointquery) {
		Glib::RefPtr<Engine::WaypointResult> q(m_waypointquery);
		m_waypointquery.reset();
		Engine::WaypointResult::cancel(q);
	}
	if (m_airwayquery) {
		Glib::RefPtr<Engine::AirwayResult> q(m_airwayquery);
		m_airwayquery.reset();
		Engine::AirwayResult::cancel(q);
	}
	if (m_mapelementquery) {
		Glib::RefPtr<Engine::MapelementResult> q(m_mapelementquery);
		m_mapelementquery.reset();
		Engine::MapelementResult::cancel(q);
	}
}

void FlightDeckWindow::wptpage_asyncdone(void)
{
	m_querydispatch();
}

void FlightDeckWindow::wptpage_querydone(void)
{
	if (m_airportquery && m_airportquery->is_done()) {
		if (m_airportquery->is_error()) {
			m_wptqueryrect = Rect(Point(0, Point::pole_lat), Point(0, Point::pole_lat));
		} else {
			wptpage_setresult(m_airportquery->get_result());
		}
		m_airportquery.reset();
	}
	if (m_navaidquery && m_navaidquery->is_done()) {
		if (m_navaidquery->is_error()) {
			m_wptqueryrect = Rect(Point(0, Point::pole_lat), Point(0, Point::pole_lat));
		} else {
			wptpage_setresult(m_navaidquery->get_result());
		}
		m_navaidquery.reset();
	}
	if (m_waypointquery && m_waypointquery->is_done()) {
		if (m_waypointquery->is_error()) {
			m_wptqueryrect = Rect(Point(0, Point::pole_lat), Point(0, Point::pole_lat));
		} else {
			wptpage_setresult(m_waypointquery->get_result());
		}
		m_waypointquery.reset();
	}
	if (m_airwayquery && m_airwayquery->is_done()) {
		if (m_airwayquery->is_error()) {
			m_wptqueryrect = Rect(Point(0, Point::pole_lat), Point(0, Point::pole_lat));
		} else {
			wptpage_setresult(m_airwayquery->get_result());
		}
		m_airwayquery.reset();
	}
	if (m_airspacequery && m_airspacequery->is_done()) {
		if (m_airspacequery->is_error()) {
			m_wptqueryrect = Rect(Point(0, Point::pole_lat), Point(0, Point::pole_lat));
		} else {
			wptpage_setresult(m_airspacequery->get_result());
		}
		m_airspacequery.reset();
	}
	if (m_mapelementquery && m_mapelementquery->is_done()) {
		if (m_mapelementquery->is_error()) {
			m_wptqueryrect = Rect(Point(0, Point::pole_lat), Point(0, Point::pole_lat));
		} else {
			wptpage_setresult(m_mapelementquery->get_result());
		}
		m_mapelementquery.reset();
	}
}

void FlightDeckWindow::wptpage_setnotebook(int pg)
{
	if (!m_wptnotebook)
		return;
        for (int n = 0; n < m_wptnotebook->get_n_pages(); ++n) {
                Gtk::Widget *w(m_wptnotebook->get_nth_page(n));
                w->set_visible(n == pg);
        }
	m_wptnotebook->set_current_page(pg);
}

void FlightDeckWindow::wptpage_select(void)
{
	wptpage_setnotebook(m_wptpagemode == wptpagemode_text ? 8 : 7);
	if (m_wptcoordlatentry)
		m_wptcoordlatentry->set_text(m_wptquerypoint.get_lat_str());
	if (m_wptcoordlonentry)
		m_wptcoordlonentry->set_text(m_wptquerypoint.get_lon_str());
	if (m_wpttexttextentry)
		m_wpttexttextentry->set_text(m_wptquerytext);
	if (m_wptarptnotebook)
		m_wptarptnotebook->set_current_page(0);
	if (m_wptarpticaoentry)
		m_wptarpticaoentry->set_text("");
	if (m_wptarptnameentry)
		m_wptarptnameentry->set_text("");
	if (m_wptarptlatentry)
		m_wptarptlatentry->set_text("");
	if (m_wptarptlonentry)
		m_wptarptlonentry->set_text("");
	if (m_wptarpteleventry)
		m_wptarpteleventry->set_text("");
	if (m_wptarpttypeentry)
		m_wptarpttypeentry->set_text("");
	if (m_wptarptremarktextview) {
		Glib::RefPtr<Gtk::TextBuffer> buf(m_wptarptremarktextview->get_buffer());
		if (buf)
			buf->set_text("");
	}
	if (m_wptnavaidicaoentry)
		m_wptnavaidicaoentry->set_text("");
	if (m_wptnavaidnameentry)
		m_wptnavaidnameentry->set_text("");
	if (m_wptnavaidlatentry)
		m_wptnavaidlatentry->set_text("");
	if (m_wptnavaidlonentry)
		m_wptnavaidlonentry->set_text("");
	if (m_wptnavaideleventry)
		m_wptnavaideleventry->set_text("");
	if (m_wptnavaidtypeentry)
		m_wptnavaidtypeentry->set_text("");
	if (m_wptnavaidfreqentry)
		m_wptnavaidfreqentry->set_text("");
	if (m_wptnavaidrangeentry)
		m_wptnavaidrangeentry->set_text("");
	if (m_wptnavaidinservicecheckbutton)
		m_wptnavaidinservicecheckbutton->set_inconsistent(true);
	if (m_wptinticaoentry)
		m_wptinticaoentry->set_text("");
	if (m_wptintnameentry)
		m_wptintnameentry->set_text("");
	if (m_wptintlatentry)
		m_wptintlatentry->set_text("");
	if (m_wptintlonentry)
		m_wptintlonentry->set_text("");
	if (m_wptintusageentry)
		m_wptintusageentry->set_text("");
	if (m_wptawynameentry)
		m_wptaspcicaoentry->set_text("");
	if (m_wptawybeginlonentry)
		m_wptawybeginlonentry->set_text("");
	if (m_wptawybeginlatentry)
		m_wptawybeginlatentry->set_text("");
	if (m_wptawyendlonentry)
		m_wptawyendlonentry->set_text("");
	if (m_wptawyendlatentry)
		m_wptawyendlatentry->set_text("");
	if (m_wptawybeginnameentry)
		m_wptawybeginnameentry->set_text("");
	if (m_wptawyendnameentry)
		m_wptawyendnameentry->set_text("");
	if (m_wptawybasealtentry)
		m_wptawybasealtentry->set_text("");
	if (m_wptawytopaltentry)
		m_wptawytopaltentry->set_text("");
	if (m_wptawyteleventry)
		m_wptawyteleventry->set_text("");
	if (m_wptawyt5eleventry)
		m_wptawyt5eleventry->set_text("");
	if (m_wptawytypeentry)
		m_wptawytypeentry->set_text("");
	if (m_wptaspcicaoentry)
		m_wptaspcicaoentry->set_text("");
	if (m_wptaspcnameentry)
		m_wptaspcnameentry->set_text("");
	if (m_wptaspcidententry)
		m_wptaspcidententry->set_text("");
	if (m_wptaspcclassexceptentry)
		m_wptaspcclassexceptentry->set_text("");
	if (m_wptaspcclassentry)
		m_wptaspcclassentry->set_text("");
	if (m_wptaspcfromaltentry)
		m_wptaspcfromaltentry->set_text("");
	if (m_wptaspctoaltentry)
		m_wptaspctoaltentry->set_text("");
	if (m_wptaspccommnameentry)
		m_wptaspccommnameentry->set_text("");
	if (m_wptaspccomm0entry)
		m_wptaspccomm0entry->set_text("");
	if (m_wptaspccomm1entry)
		m_wptaspccomm1entry->set_text("");
	if (m_wptaspceffectivetextview) {
		Glib::RefPtr<Gtk::TextBuffer> buf(m_wptaspceffectivetextview->get_buffer());
		if (buf)
			buf->set_text("");
	}
	if (m_wptaspcnotetextview) {
		Glib::RefPtr<Gtk::TextBuffer> buf(m_wptaspcnotetextview->get_buffer());
		if (buf)
			buf->set_text("");
	}
	if (m_wptmapelnameentry)
		m_wptmapelnameentry->set_text("");
	if (m_wptmapeltypeentry)
		m_wptmapeltypeentry->set_text("");
	if (m_wptfpldbicaoentry)
		m_wptfpldbicaoentry->set_text("");
        if (m_wptfpldbnameentry)
		m_wptfpldbnameentry->set_text("");
        if (m_wptfpldblatentry)
		m_wptfpldblatentry->set_text("");
        if (m_wptfpldblonentry)
		m_wptfpldblonentry->set_text("");
        if (m_wptfpldbaltentry)
		m_wptfpldbaltentry->set_text("");
        if (m_wptfpldbfreqentry)
		m_wptfpldbfreqentry->set_text("");
        if (m_wptfpldbtypeentry)
		m_wptfpldbtypeentry->set_text("");
	if (m_wptfpldbnotetextview) {
		Glib::RefPtr<Gtk::TextBuffer> buf(m_wptfpldbnotetextview->get_buffer());
		if (buf)
			buf->set_text("");
	}
	if (m_wptarptrwystore)
		m_wptarptrwystore->clear();
	if (m_wptarpthelipadstore)
		m_wptarpthelipadstore->clear();
	if (m_wptarptcommstore)
		m_wptarptcommstore->clear();
        if (m_wptdrawingarea && m_wptpagemode != wptpagemode_text) {
		Glib::TimeVal tv;
		tv.assign_current_time();
		m_wptmapcursorconn.block();
		m_wptdrawingarea->set_center(m_wptquerypoint, 0, 0, tv.tv_sec);
		m_wptdrawingarea->set_cursor(m_wptquerypoint);
		m_wptmapcursorconn.unblock();
	}
	if (m_wptbuttoninsert)
		m_wptbuttoninsert->set_sensitive(false);
	if (m_wptbuttondirectto)
		m_wptbuttondirectto->set_sensitive(false);
	if (m_wpttogglebuttonbrg2)
		m_wpttogglebuttonbrg2->set_sensitive(false);
}

void FlightDeckWindow::wptpage_select(const AirportsDb::Airport& e, unsigned int routeid, unsigned int routepointid)
{
        wptpage_setnotebook(0);
	if (m_wptarptnotebook)
		m_wptarptnotebook->set_current_page(1);
	if (m_wptarpticaoentry)
		m_wptarpticaoentry->set_text(e.get_icao());
	if (m_wptarptnameentry)
		m_wptarptnameentry->set_text(e.get_name());
	if (m_wptarptlatentry)
		m_wptarptlatentry->set_text(e.get_coord().get_lat_str());
	if (m_wptarptlonentry)
		m_wptarptlonentry->set_text(e.get_coord().get_lon_str());
	if (m_wptarpteleventry)
		m_wptarpteleventry->set_text(Conversions::alt_str(e.get_elevation(), 0));
	if (m_wptarpttypeentry)
		m_wptarpttypeentry->set_text(e.get_type_string());
	if (m_wptarptremarktextview) {
		Glib::RefPtr<Gtk::TextBuffer> buf(m_wptarptremarktextview->get_buffer());
		if (buf)
			buf->set_text(e.get_vfrrmk());
	}
	Point pt(e.get_coord());
	int elev(e.get_elevation());
	if (m_wptarptrwystore) {
		m_wptarptrwystore->clear();
		Sensors::fplanwind_t wind(0, 0);		
		double tkoffgnddist(std::numeric_limits<double>::max()), ldggnddist(tkoffgnddist), tkoffvrot(0), ldgvrot(0);
		if (m_sensors) {
			wind = m_sensors->get_fplan_wind(e.get_coord());
			const Aircraft::Distances& dist(m_sensors->get_aircraft().get_dist());
			double mass(m_perfvalues[0][perfval_mass]), headwind(0);
			AirData<double> ad;
			ad.set_qnh_tempoffs(m_sensors->get_altimeter_qnh(), m_sensors->get_altimeter_tempoffs());
			ad.set_true_altitude(elev);
			double densityalt(ad.get_density_altitude());
			if (false)
				std::cerr << "Airdata: Density Altitude: " << densityalt << std::endl;
			for (unsigned int i(0); i < dist.get_nrtakeoffdist(); ++i) {
				const Aircraft::Distances::Distance& d(dist.get_takeoffdist(i));
				double gnddist, obstdist;
				d.calculate(gnddist, obstdist, densityalt, headwind, mass);
				if (std::isnan(gnddist))
					continue;
				gnddist = Aircraft::convert(dist.get_distunit(), Aircraft::unit_m, gnddist);
				if (gnddist >= tkoffgnddist)
					continue;
				tkoffgnddist = gnddist;
				tkoffvrot = d.get_vrotate();
			}
			for (unsigned int i(0); i < dist.get_nrldgdist(); ++i) {
				const Aircraft::Distances::Distance& d(dist.get_ldgdist(i));
				double gnddist, obstdist;
				d.calculate(gnddist, obstdist, densityalt, headwind, mass);
				if (std::isnan(gnddist))
					continue;
				gnddist = Aircraft::convert(dist.get_distunit(), Aircraft::unit_m, gnddist);
				if (gnddist >= ldggnddist)
					continue;
				ldggnddist = gnddist;
				ldgvrot = d.get_vrotate();
			}
		}
		if (tkoffgnddist == std::numeric_limits<double>::max())
			tkoffgnddist = 0;
		if (ldggnddist == std::numeric_limits<double>::max())
			ldggnddist = 0;
		double softtkoffgnddist(tkoffgnddist * (1 + m_perfsoftfieldpenalty));
 		for (unsigned int nr(0); nr < e.get_nrrwy(); ++nr) {
			const AirportsDb::Airport::Runway& rwy(e.get_rwy(nr));
			if (rwy.get_flags() & AirportsDb::Airport::Runway::flag_le_usable) {
				Gtk::TreeModel::Row row(*(m_wptarptrwystore->append()));
				row[m_wptairportrunwaycolumns.m_ident] = rwy.get_ident_le();
				row[m_wptairportrunwaycolumns.m_hdg] = Conversions::track_str(rwy.get_le_hdg() * FPlanLeg::to_deg);
				row[m_wptairportrunwaycolumns.m_length] = Glib::ustring::format(std::fixed, std::setprecision(0), rwy.get_length() * Point::ft_to_m_dbl);
				row[m_wptairportrunwaycolumns.m_width] = Glib::ustring::format(std::fixed, std::setprecision(0), rwy.get_width() * Point::ft_to_m_dbl);
				row[m_wptairportrunwaycolumns.m_tda] = Glib::ustring::format(std::fixed, std::setprecision(0), rwy.get_le_tda() * Point::ft_to_m_dbl);
				row[m_wptairportrunwaycolumns.m_lda] = Glib::ustring::format(std::fixed, std::setprecision(0), rwy.get_le_lda() * Point::ft_to_m_dbl);
				row[m_wptairportrunwaycolumns.m_elev] = Conversions::alt_str(rwy.get_le_elev(), 0);
				row[m_wptairportrunwaycolumns.m_surface] = rwy.get_surface();
				Glib::ustring lights;
				for (unsigned int i = 0; i < AirportsDb::Airport::Runway::nr_lights; ++i) {
					AirportsDb::Airport::Runway::light_t l(rwy.get_le_light(i));
					if (l == AirportsDb::Airport::Runway::light_off)
						continue;
					if (!lights.empty())
						lights += "; ";
					lights += AirportsDb::Airport::Runway::get_light_string(l);
				}
				row[m_wptairportrunwaycolumns.m_light] = lights;
				double tgnddist(rwy.is_concrete() ? tkoffgnddist : softtkoffgnddist), lgnddist(ldggnddist);
				if (m_sensors && (tkoffvrot > 0 || ldgvrot > 0)) {
					double headwind(cos((wind.first - rwy.get_le_hdg() * FPlanLeg::to_deg) * (M_PI / 180.0)));
					headwind *= wind.second;
					if (tkoffvrot > 0) {
						double mul(std::max(0.5, (tkoffvrot - headwind) / tkoffvrot));
						tgnddist *= mul * mul;
					}
					if (ldgvrot > 0) {
						double mul(std::max(0.5, (ldgvrot - headwind) / ldgvrot));
						lgnddist *= mul * mul;
					}
				}
				bool cantkoff(rwy.get_he_tda() >= tgnddist);
				bool canland(rwy.get_he_lda() >= lgnddist);
				Gdk::Color col;
				if (cantkoff && canland)
					col.set_rgb(0, 0, 0);
				else if (cantkoff || canland)
				        col.set_rgb(0xffff, 0x8000, 0);
				else
					col.set_rgb(0xffff, 0, 0);
				row[m_wptairportrunwaycolumns.m_color] = col;
			}
			if (rwy.get_flags() & AirportsDb::Airport::Runway::flag_he_usable) {
				Gtk::TreeModel::Row row(*(m_wptarptrwystore->append()));
				row[m_wptairportrunwaycolumns.m_ident] = rwy.get_ident_he();
				row[m_wptairportrunwaycolumns.m_hdg] = Conversions::track_str(rwy.get_he_hdg() * FPlanLeg::to_deg);
				row[m_wptairportrunwaycolumns.m_length] = Glib::ustring::format(std::fixed, std::setprecision(0), rwy.get_length() * Point::ft_to_m_dbl);
				row[m_wptairportrunwaycolumns.m_width] = Glib::ustring::format(std::fixed, std::setprecision(0), rwy.get_width() * Point::ft_to_m_dbl);
				row[m_wptairportrunwaycolumns.m_tda] = Glib::ustring::format(std::fixed, std::setprecision(0), rwy.get_he_tda() * Point::ft_to_m_dbl);
				row[m_wptairportrunwaycolumns.m_lda] = Glib::ustring::format(std::fixed, std::setprecision(0), rwy.get_he_lda() * Point::ft_to_m_dbl);
				row[m_wptairportrunwaycolumns.m_elev] = Conversions::alt_str(rwy.get_he_elev(), 0);
				row[m_wptairportrunwaycolumns.m_surface] = rwy.get_surface();
				Glib::ustring lights;
				for (unsigned int i = 0; i < AirportsDb::Airport::Runway::nr_lights; ++i) {
					AirportsDb::Airport::Runway::light_t l(rwy.get_he_light(i));
					if (l == AirportsDb::Airport::Runway::light_off)
						continue;
					if (!lights.empty())
						lights += "; ";
					lights += AirportsDb::Airport::Runway::get_light_string(l);
				}
				row[m_wptairportrunwaycolumns.m_light] = lights;
				double tgnddist(rwy.is_concrete() ? tkoffgnddist : softtkoffgnddist), lgnddist(ldggnddist);
				if (m_sensors && (tkoffvrot > 0 || ldgvrot > 0)) {
					double headwind(cos((wind.first - rwy.get_he_hdg() * FPlanLeg::to_deg) * (M_PI / 180.0)));
					headwind *= wind.second;
					if (tkoffvrot > 0) {
						double mul(std::max(0.5, (tkoffvrot - headwind) / tkoffvrot));
						tgnddist *= mul * mul;
					}
					if (ldgvrot > 0) {
						double mul(std::max(0.5, (ldgvrot - headwind) / ldgvrot));
						lgnddist *= mul * mul;
					}
				}
				bool cantkoff(rwy.get_le_tda() >= tgnddist);
				bool canland(rwy.get_le_lda() >= lgnddist);
				Gdk::Color col;
				if (cantkoff && canland)
					col.set_rgb(0, 0, 0);
				else if (cantkoff || canland)
					col.set_rgb(0xffff, 0x8000, 0);
				else
					col.set_rgb(0xffff, 0, 0);
				row[m_wptairportrunwaycolumns.m_color] = col;
			}
		}
	}
	if (m_wptarpthelipadstore) {
		m_wptarpthelipadstore->clear();
		for (unsigned int nr(0); nr < e.get_nrhelipads(); ++nr) {
			const AirportsDb::Airport::Helipad& helipad(e.get_helipad(nr));
			if (!(helipad.get_flags() & AirportsDb::Airport::Helipad::flag_usable))
				continue;
			Gtk::TreeModel::Row row(*(m_wptarpthelipadstore->append()));
			row[m_wptairporthelipadcolumns.m_ident] = helipad.get_ident();
			row[m_wptairporthelipadcolumns.m_hdg] = Conversions::track_str(helipad.get_hdg() * FPlanLeg::to_deg);
			row[m_wptairporthelipadcolumns.m_length] = Glib::ustring::format(std::fixed, std::setprecision(0), helipad.get_length() * Point::ft_to_m_dbl);
			row[m_wptairporthelipadcolumns.m_width] = Glib::ustring::format(std::fixed, std::setprecision(0), helipad.get_width() * Point::ft_to_m_dbl);
			row[m_wptairporthelipadcolumns.m_elev] = Conversions::alt_str(helipad.get_elev(), 0);
		}
	}
	if (m_wptarptcommstore) {
		m_wptarptcommstore->clear();
		for (unsigned int nr(0); nr < e.get_nrcomm(); ++nr) {
			const AirportsDb::Airport::Comm& comm(e.get_comm(nr));
			Gtk::TreeModel::Row row(*(m_wptarptcommstore->append()));
			row[m_wptairportcommcolumns.m_name] = comm.get_name();
			row[m_wptairportcommcolumns.m_freq0] = Conversions::freq_str(comm[0]);
			row[m_wptairportcommcolumns.m_freq1] = Conversions::freq_str(comm[1]);
			row[m_wptairportcommcolumns.m_freq2] = Conversions::freq_str(comm[2]);
			row[m_wptairportcommcolumns.m_freq3] = Conversions::freq_str(comm[3]);
			row[m_wptairportcommcolumns.m_freq4] = Conversions::freq_str(comm[4]);
			row[m_wptairportcommcolumns.m_sector] = comm.get_sector();
			row[m_wptairportcommcolumns.m_oprhours] = comm.get_oprhours();
		}
	}
	if (m_wptarptfasstore) {
		m_wptarptfasstore->clear();
		for (unsigned int nr(0); nr < e.get_nrfinalapproachsegments(); ++nr) {
			const AirportsDb::Airport::MinimalFAS& fas(e.get_finalapproachsegment(nr));
			Gtk::TreeModel::Row row(*(m_wptarptfasstore->append()));
			row[m_wptairportfascolumns.m_ident] = fas.get_referencepathident();
			{
				std::ostringstream oss;
				uint8_t rwyn(fas.get_runwaynumber());
				if (!rwyn) {
					oss << "Helipad";
				} else {
					oss << std::setw(2) << std::setfill('0') << (unsigned int)rwyn;
					char l(fas.get_runwayletter_char());
					if (l != ' ')
						oss << l;
				}
				row[m_wptairportfascolumns.m_runway] = oss.str();
			}
			{
				std::ostringstream oss;
				oss << fas.get_routeindicator_char();
				row[m_wptairportfascolumns.m_route] = oss.str();
			}
			row[m_wptairportfascolumns.m_glide] = Glib::ustring::format(std::fixed, std::setprecision(2), fas.get_glidepathangle_deg());
			row[m_wptairportfascolumns.m_ftpalt] = Conversions::alt_str(fas.get_ftp_height_meter() * Point::m_to_ft_dbl, 0);
			row[m_wptairportfascolumns.m_tch] = Conversions::alt_str(fas.get_approach_tch_ft(), 0);
		}
	}
	bool singlept(true);
	if (routeid != WaypointAirportModelColumns::noid && routeid < e.get_nrvfrrte()) {
		const AirportsDb::Airport::VFRRoute& vfrrte(e.get_vfrrte(routeid));
		if (routepointid == WaypointAirportModelColumns::noid || routepointid >= vfrrte.size()) {
			singlept = false;
			if (vfrrte.size() > 0) {
				const AirportsDb::Airport::VFRRoute::VFRRoutePoint& pt1(vfrrte[0]);
				if (!pt1.is_at_airport() && pt1.get_coord() != pt) {
					pt = pt1.get_coord();
					elev = pt1.get_altitude();
				}
				if (vfrrte.size() > 1) {
					const AirportsDb::Airport::VFRRoute::VFRRoutePoint& pt1(vfrrte[vfrrte.size()-1]);
					if (!pt1.is_at_airport() && pt1.get_coord() != pt) {
						pt = pt1.get_coord();
						elev = pt1.get_altitude();
					}
				}
			}
		} else {
			const AirportsDb::Airport::VFRRoute::VFRRoutePoint& pt1(vfrrte[routepointid]);
			pt = pt1.get_coord();
			elev = pt1.get_altitude();
		}
	}
        if (m_wptdrawingarea) {
		Glib::TimeVal tv;
		tv.assign_current_time();
		m_wptmapcursorconn.block();
		m_wptdrawingarea->set_center(pt, elev, 0, tv.tv_sec);
		m_wptdrawingarea->set_cursor(pt);
		m_wptmapcursorconn.unblock();
	}
	if (m_wptbuttoninsert)
		m_wptbuttoninsert->set_sensitive(true);
	if (m_wptbuttondirectto)
		m_wptbuttondirectto->set_sensitive(singlept);
	if (m_wpttogglebuttonbrg2)
		m_wpttogglebuttonbrg2->set_sensitive(singlept);
}

void FlightDeckWindow::wptpage_select(const NavaidsDb::Navaid& e)
{
        wptpage_setnotebook(1);
	if (m_wptnavaidicaoentry)
		m_wptnavaidicaoentry->set_text(e.get_icao());
	if (m_wptnavaidnameentry)
		m_wptnavaidnameentry->set_text(e.get_name());
	if (m_wptnavaidlatentry)
		m_wptnavaidlatentry->set_text(e.get_coord().get_lat_str());
	if (m_wptnavaidlonentry)
		m_wptnavaidlonentry->set_text(e.get_coord().get_lon_str());
	if (m_wptnavaideleventry)
		m_wptnavaideleventry->set_text(Conversions::alt_str(e.get_elev(), 0));
	if (m_wptnavaidtypeentry)
		m_wptnavaidtypeentry->set_text(e.get_navaid_typename());
	if (m_wptnavaidfreqentry)
		m_wptnavaidfreqentry->set_text(Conversions::freq_str(e.get_frequency()));
	if (m_wptnavaidrangeentry)
		m_wptnavaidrangeentry->set_text(Glib::ustring::format(e.get_range()));
	if (m_wptnavaidinservicecheckbutton) {
		m_wptnavaidinservicecheckbutton->set_inconsistent(false);
		m_wptnavaidinservicecheckbutton->set_active(e.is_inservice());
	}
        if (m_wptdrawingarea) {
		Glib::TimeVal tv;
		tv.assign_current_time();
		m_wptmapcursorconn.block();
		m_wptdrawingarea->set_center(e.get_coord(), e.get_elev(), 0, tv.tv_sec);
		m_wptdrawingarea->set_cursor(e.get_coord());
		m_wptmapcursorconn.unblock();
	}
	if (m_wptbuttoninsert)
		m_wptbuttoninsert->set_sensitive(true);
	if (m_wptbuttondirectto)
		m_wptbuttondirectto->set_sensitive(true);
	if (m_wpttogglebuttonbrg2)
		m_wpttogglebuttonbrg2->set_sensitive(true);
}

void FlightDeckWindow::wptpage_select(const WaypointsDb::Waypoint& e)
{
        wptpage_setnotebook(2);
	if (m_wptinticaoentry)
		m_wptinticaoentry->set_text(e.get_icao());
	if (m_wptintnameentry)
		m_wptintnameentry->set_text(e.get_name());
	if (m_wptintlatentry)
		m_wptintlatentry->set_text(e.get_coord().get_lat_str());
	if (m_wptintlonentry)
		m_wptintlonentry->set_text(e.get_coord().get_lon_str());
	if (m_wptintusageentry)
		m_wptintusageentry->set_text(e.get_usage_name());
        if (m_wptdrawingarea) {
		Glib::TimeVal tv;
		tv.assign_current_time();
		m_wptmapcursorconn.block();
		m_wptdrawingarea->set_center(e.get_coord(), 0, 0, tv.tv_sec);
		m_wptdrawingarea->set_cursor(e.get_coord());
		m_wptmapcursorconn.unblock();
	}
	if (m_wptbuttoninsert)
		m_wptbuttoninsert->set_sensitive(true);
	if (m_wptbuttondirectto)
		m_wptbuttondirectto->set_sensitive(true);
	if (m_wpttogglebuttonbrg2)
		m_wpttogglebuttonbrg2->set_sensitive(true);
}

void FlightDeckWindow::wptpage_select(const AirwaysDb::Airway& e)
{
        wptpage_setnotebook(3);
	if (m_wptawynameentry)
		m_wptawynameentry->set_text(e.get_name());
	if (m_wptawybeginlonentry)
		m_wptawybeginlonentry->set_text(e.get_begin_coord().get_lon_str());
	if (m_wptawybeginlatentry)
		m_wptawybeginlatentry->set_text(e.get_begin_coord().get_lat_str());
	if (m_wptawyendlonentry)
		m_wptawyendlonentry->set_text(e.get_end_coord().get_lon_str());
	if (m_wptawyendlatentry)
		m_wptawyendlatentry->set_text(e.get_end_coord().get_lat_str());
	if (m_wptawybeginnameentry)
		m_wptawybeginnameentry->set_text(e.get_begin_name());
	if (m_wptawyendnameentry)
		m_wptawyendnameentry->set_text(e.get_end_name());
	if (m_wptawybasealtentry)
		m_wptawybasealtentry->set_text(Conversions::alt_str(e.get_base_level(), 0));
	if (m_wptawytopaltentry)
		m_wptawytopaltentry->set_text(Conversions::alt_str(e.get_top_level(), 0));
	if (m_wptawyteleventry)
		m_wptawyteleventry->set_text(e.is_terrain_elev_valid() ? Conversions::alt_str(e.get_terrain_elev(), 0) : "-");
	if (m_wptawyt5eleventry)
		m_wptawyt5eleventry->set_text(e.is_corridor5_elev_valid() ? Conversions::alt_str(e.get_corridor5_elev(), 0) : "-");
	if (m_wptawytypeentry)
		m_wptawytypeentry->set_text(e.get_type_name());
        if (m_wptdrawingarea) {
		Glib::TimeVal tv;
		tv.assign_current_time();
		Point pt(e.get_begin_coord().halfway(e.get_end_coord()));
		m_wptmapcursorconn.block();
		m_wptdrawingarea->set_center(pt, 0, 0, tv.tv_sec);
		m_wptdrawingarea->set_cursor(pt);
		m_wptmapcursorconn.unblock();
	}
	if (m_wptbuttoninsert)
		m_wptbuttoninsert->set_sensitive(true);
	if (m_wptbuttondirectto)
		m_wptbuttondirectto->set_sensitive(true);
	if (m_wpttogglebuttonbrg2)
		m_wpttogglebuttonbrg2->set_sensitive(true);
}

void FlightDeckWindow::wptpage_select(const AirspacesDb::Airspace& e)
{
        wptpage_setnotebook(4);
	if (m_wptaspcnotebook)
		m_wptaspcnotebook->set_current_page(0);
	if (m_wptaspcicaoentry)
		m_wptaspcicaoentry->set_text(e.get_icao());
	if (m_wptaspcnameentry)
		m_wptaspcnameentry->set_text(e.get_name());
	if (m_wptaspcidententry)
		m_wptaspcidententry->set_text(e.get_ident());
	if (m_wptaspcclassexceptentry)
		m_wptaspcclassexceptentry->set_text(e.get_classexcept());
	if (m_wptaspcclassentry)
		m_wptaspcclassentry->set_text(e.get_class_string());
	if (m_wptaspcfromaltentry)
		m_wptaspcfromaltentry->set_text(e.get_altlwr_string());
	if (m_wptaspctoaltentry)
		m_wptaspctoaltentry->set_text(e.get_altupr_string());
	if (m_wptaspccommnameentry)
		m_wptaspccommnameentry->set_text(e.get_commname());
	if (m_wptaspccomm0entry)
		m_wptaspccomm0entry->set_text(Conversions::freq_str(e.get_commfreq(0)));
	if (m_wptaspccomm1entry)
		m_wptaspccomm1entry->set_text(Conversions::freq_str(e.get_commfreq(1)));
	if (m_wptaspceffectivetextview) {
		Glib::RefPtr<Gtk::TextBuffer> buf(m_wptaspceffectivetextview->get_buffer());
		if (buf)
			buf->set_text(e.get_efftime());
	}
	if (m_wptaspcnotetextview) {
		Glib::RefPtr<Gtk::TextBuffer> buf(m_wptaspcnotetextview->get_buffer());
		if (buf)
			buf->set_text(e.get_note());
	}
        if (m_wptdrawingarea) {
		Glib::TimeVal tv;
		tv.assign_current_time();
		m_wptmapcursorconn.block();
		m_wptdrawingarea->set_center(e.get_labelcoord(), 0, 0, tv.tv_sec);
		m_wptdrawingarea->set_cursor(e.get_labelcoord());
		m_wptmapcursorconn.unblock();
	}
	if (m_wptbuttoninsert)
		m_wptbuttoninsert->set_sensitive(false);
	if (m_wptbuttondirectto)
		m_wptbuttondirectto->set_sensitive(false);
	if (m_wpttogglebuttonbrg2)
		m_wpttogglebuttonbrg2->set_sensitive(false);
}

void FlightDeckWindow::wptpage_select(const MapelementsDb::Mapelement& e)
{
        wptpage_setnotebook(5);
	if (m_wptmapelnameentry)
		m_wptmapelnameentry->set_text(e.get_name());
	if (m_wptmapeltypeentry)
		m_wptmapeltypeentry->set_text(e.get_typename());
        if (m_wptdrawingarea) {
		Glib::TimeVal tv;
		tv.assign_current_time();
		m_wptmapcursorconn.block();
		m_wptdrawingarea->set_center(e.get_labelcoord(), 0, 0, tv.tv_sec);
		m_wptdrawingarea->set_cursor(e.get_labelcoord());
		m_wptmapcursorconn.unblock();
	}
	if (m_wptbuttoninsert)
		m_wptbuttoninsert->set_sensitive(true);
	if (m_wptbuttondirectto)
		m_wptbuttondirectto->set_sensitive(true);
	if (m_wpttogglebuttonbrg2)
		m_wpttogglebuttonbrg2->set_sensitive(true);
}

void FlightDeckWindow::wptpage_select(const FPlanWaypoint& e)
{
	wptpage_setnotebook(6);
	if (m_wptfpldbicaoentry)
		m_wptfpldbicaoentry->set_text(e.get_icao());
        if (m_wptfpldbnameentry)
		m_wptfpldbnameentry->set_text(e.get_name());
        if (m_wptfpldblatentry)
		m_wptfpldblatentry->set_text(e.get_coord().get_lat_str());
        if (m_wptfpldblonentry)
		m_wptfpldblonentry->set_text(e.get_coord().get_lon_str());
        if (m_wptfpldbaltentry)
		m_wptfpldbaltentry->set_text(Conversions::alt_str(e.get_altitude(), e.get_flags()));
        if (m_wptfpldbfreqentry)
		m_wptfpldbfreqentry->set_text(Conversions::freq_str(e.get_frequency()));
        if (m_wptfpldbtypeentry)
		m_wptfpldbtypeentry->set_text((e.get_flags() & FPlanWaypoint::altflag_ifr) ? "IFR" : "VFR");
	if (m_wptfpldbnotetextview) {
		Glib::RefPtr<Gtk::TextBuffer> buf(m_wptfpldbnotetextview->get_buffer());
		if (buf)
			buf->set_text(e.get_note());
	}
        if (m_wptdrawingarea) {
		Glib::TimeVal tv;
		tv.assign_current_time();
		m_wptmapcursorconn.block();
		m_wptdrawingarea->set_center(e.get_coord(), e.get_altitude(), 0, tv.tv_sec);
		m_wptdrawingarea->set_cursor(e.get_coord());
		m_wptmapcursorconn.unblock();
	}
	if (m_wptbuttoninsert)
		m_wptbuttoninsert->set_sensitive(true);
	if (m_wptbuttondirectto)
		m_wptbuttondirectto->set_sensitive(true);
	if (m_wpttogglebuttonbrg2)
		m_wpttogglebuttonbrg2->set_sensitive(true);
}

void FlightDeckWindow::wptpage_setresult(const Engine::AirportResult::elementvector_t& elements)
{
	m_wpttreeviewselconn.disconnect();
	m_wptstore = Gtk::TreeStore::create(m_wptairportcolumns);
	if (m_wpttreeview) {
		m_wpttreeview->unset_model();
		m_wpttreeview->remove_all_columns();
		Gtk::CellRendererText *name_renderer = Gtk::manage(new Gtk::CellRendererText());
		int name_col = m_wpttreeview->append_column(_("Name"), *name_renderer) - 1;
		Gtk::TreeViewColumn *name_column = m_wpttreeview->get_column(name_col);
                if (name_column) {
                        name_column->add_attribute(*name_renderer, "text", m_wptairportcolumns.m_name);
			name_column->add_attribute(*name_renderer, "style", m_wptairportcolumns.m_style);
			name_column->add_attribute(*name_renderer, "foreground-gdk", m_wptairportcolumns.m_color);
		}
		Gtk::CellRendererText *icao_renderer = Gtk::manage(new Gtk::CellRendererText());
		int icao_col = m_wpttreeview->append_column(_("ICAO"), *icao_renderer) - 1;
		Gtk::TreeViewColumn *icao_column = m_wpttreeview->get_column(icao_col);
                if (icao_column) {
                        icao_column->add_attribute(*icao_renderer, "text", m_wptairportcolumns.m_icao);
			icao_column->add_attribute(*icao_renderer, "style", m_wptairportcolumns.m_style);
			icao_column->add_attribute(*icao_renderer, "foreground-gdk", m_wptairportcolumns.m_color);
		}
		Gtk::CellRendererText *type_renderer = Gtk::manage(new Gtk::CellRendererText());
		int type_col = m_wpttreeview->append_column(_("Type"), *type_renderer) - 1;
		Gtk::TreeViewColumn *type_column = m_wpttreeview->get_column(type_col);
                if (type_column) {
                        type_column->add_attribute(*type_renderer, "text", m_wptairportcolumns.m_type);
			type_column->add_attribute(*type_renderer, "style", m_wptairportcolumns.m_style);
			type_column->add_attribute(*type_renderer, "foreground-gdk", m_wptairportcolumns.m_color);
		}
		Gtk::CellRendererText *elev_renderer = Gtk::manage(new Gtk::CellRendererText());
		int elev_col = m_wpttreeview->append_column(_("Elev"), *elev_renderer) - 1;
		Gtk::TreeViewColumn *elev_column = m_wpttreeview->get_column(elev_col);
                if (elev_column) {
                        elev_column->add_attribute(*elev_renderer, "text", m_wptairportcolumns.m_elev);
			elev_column->add_attribute(*elev_renderer, "style", m_wptairportcolumns.m_style);
			elev_column->add_attribute(*elev_renderer, "foreground-gdk", m_wptairportcolumns.m_color);
		}
		Gtk::CellRendererText *rwylength_renderer = Gtk::manage(new Gtk::CellRendererText());
		int rwylength_col = m_wpttreeview->append_column(_("RWY L"), *rwylength_renderer) - 1;
		Gtk::TreeViewColumn *rwylength_column = m_wpttreeview->get_column(rwylength_col);
                if (rwylength_column) {
                        rwylength_column->add_attribute(*rwylength_renderer, "text", m_wptairportcolumns.m_rwylength);
			rwylength_column->add_attribute(*rwylength_renderer, "style", m_wptairportcolumns.m_style);
			rwylength_column->add_attribute(*rwylength_renderer, "foreground-gdk", m_wptairportcolumns.m_color);
		}
		Gtk::CellRendererText *rwywidth_renderer = Gtk::manage(new Gtk::CellRendererText());
		int rwywidth_col = m_wpttreeview->append_column(_("RWY W"), *rwywidth_renderer) - 1;
		Gtk::TreeViewColumn *rwywidth_column = m_wpttreeview->get_column(rwywidth_col);
                if (rwywidth_column) {
                        rwywidth_column->add_attribute(*rwywidth_renderer, "text", m_wptairportcolumns.m_rwywidth);
			rwywidth_column->add_attribute(*rwywidth_renderer, "style", m_wptairportcolumns.m_style);
			rwywidth_column->add_attribute(*rwywidth_renderer, "foreground-gdk", m_wptairportcolumns.m_color);
		}
		Gtk::CellRendererText *rwyident_renderer = Gtk::manage(new Gtk::CellRendererText());
		int rwyident_col = m_wpttreeview->append_column(_("RWY ID"), *rwyident_renderer) - 1;
		Gtk::TreeViewColumn *rwyident_column = m_wpttreeview->get_column(rwyident_col);
                if (rwyident_column) {
                        rwyident_column->add_attribute(*rwyident_renderer, "text", m_wptairportcolumns.m_rwyident);
			rwyident_column->add_attribute(*rwyident_renderer, "style", m_wptairportcolumns.m_style);
			rwyident_column->add_attribute(*rwyident_renderer, "foreground-gdk", m_wptairportcolumns.m_color);
		}
		Gtk::CellRendererText *rwysfc_renderer = Gtk::manage(new Gtk::CellRendererText());
		int rwysfc_col = m_wpttreeview->append_column(_("RWY SFC"), *rwysfc_renderer) - 1;
		Gtk::TreeViewColumn *rwysfc_column = m_wpttreeview->get_column(rwysfc_col);
                if (rwysfc_column) {
                        rwysfc_column->add_attribute(*rwysfc_renderer, "text", m_wptairportcolumns.m_rwysfc);
			rwysfc_column->add_attribute(*rwysfc_renderer, "style", m_wptairportcolumns.m_style);
			rwysfc_column->add_attribute(*rwysfc_renderer, "foreground-gdk", m_wptairportcolumns.m_color);
		}
		Gtk::CellRendererText *comm_renderer = Gtk::manage(new Gtk::CellRendererText());
		int comm_col = m_wpttreeview->append_column(_("Comm"), *comm_renderer) - 1;
		Gtk::TreeViewColumn *comm_column = m_wpttreeview->get_column(comm_col);
                if (comm_column) {
                        comm_column->add_attribute(*comm_renderer, "text", m_wptairportcolumns.m_comm);
			comm_column->add_attribute(*comm_renderer, "style", m_wptairportcolumns.m_style);
			comm_column->add_attribute(*comm_renderer, "foreground-gdk", m_wptairportcolumns.m_color);
		}
		Gtk::CellRendererText *dist_renderer = Gtk::manage(new Gtk::CellRendererText());
		int dist_col = m_wpttreeview->append_column(_("Dist"), *dist_renderer) - 1;
		Gtk::TreeViewColumn *dist_column = m_wpttreeview->get_column(dist_col);
                if (dist_column) {
                        dist_column->add_attribute(*dist_renderer, "text", m_wptairportcolumns.m_dist);
			dist_column->add_attribute(*dist_renderer, "style", m_wptairportcolumns.m_style);
			dist_column->add_attribute(*dist_renderer, "foreground-gdk", m_wptairportcolumns.m_color);
		}
                elev_renderer->set_property("xalign", 1.0);
                rwylength_renderer->set_property("xalign", 1.0);
                rwywidth_renderer->set_property("xalign", 1.0);
                comm_renderer->set_property("xalign", 1.0);
		dist_renderer->set_property("xalign", 1.0);
		const Gtk::TreeModelColumnBase *cols[] = { &m_wptairportcolumns.m_name,
							   &m_wptairportcolumns.m_icao,
							   &m_wptairportcolumns.m_type,
							   &m_wptairportcolumns.m_elev,
							   &m_wptairportcolumns.m_rwylength,
							   &m_wptairportcolumns.m_rwywidth,
							   &m_wptairportcolumns.m_rwyident,
							   &m_wptairportcolumns.m_rwysfc,
							   &m_wptairportcolumns.m_comm,
							   &m_wptairportcolumns.m_dist };
		for (unsigned int i = 0; i < sizeof(cols)/sizeof(cols[0]); ++i) {
			Gtk::TreeViewColumn *col = m_wpttreeview->get_column(i);
			if (!col)
				continue;
			col->set_resizable(true);
			col->set_sort_column(*cols[i]);
			col->set_expand(i == 0);
		}
		{
			const Gtk::TreeModelColumn<Glib::ustring> *sortcols[] = { &m_wptairportcolumns.m_name,
										  &m_wptairportcolumns.m_icao,
										  &m_wptairportcolumns.m_type,
										  &m_wptairportcolumns.m_elev,
										  &m_wptairportcolumns.m_rwylength,
										  &m_wptairportcolumns.m_rwywidth,
										  &m_wptairportcolumns.m_rwyident,
										  &m_wptairportcolumns.m_rwysfc,
										  &m_wptairportcolumns.m_comm };
			for (unsigned int i = 0; i < sizeof(sortcols)/sizeof(sortcols[0]); ++i)
				m_wptstore->set_sort_func(*sortcols[i], sigc::bind<0>(sigc::mem_fun(*this, &FlightDeckWindow::wptpage_airport_sortcol), *sortcols[i]));
		}
		m_wptstore->set_sort_func(m_wptairportcolumns.m_dist, sigc::mem_fun(*this, &FlightDeckWindow::wptpage_airport_sortdist));
		m_wpttreeview->set_search_column(m_wptairportcolumns.m_name);
		m_wpttreeview->columns_autosize();
		m_wpttreeview->set_headers_clickable();
		Glib::RefPtr<Gtk::TreeSelection> selection(m_wpttreeview->get_selection());
		selection->set_mode(Gtk::SELECTION_SINGLE);
		m_wpttreeviewselconn = selection->signal_changed().connect(sigc::mem_fun(*this, &FlightDeckWindow::wptpage_airport_selection_changed));
		//selection->set_select_function(sigc::mem_fun(*this, &FlightDeckWindow::wpt_select_function));
	}
	if (!m_wptstore)
		return;
	m_wptstore->clear();
	for (Engine::AirportResult::elementvector_t::const_iterator i(elements.begin()), e(elements.end()); i != e; ++i) {
		const Engine::AirportResult::element_t& el(*i);
		WaypointAirportModelColumns::element_ptr_t elp(new Engine::AirportResult::element_t(el));
                Gtk::TreeModel::Row row(*(m_wptstore->append()));
		row[m_wptairportcolumns.m_element] = elp;
		row[m_wptairportcolumns.m_icao] = el.get_icao();
		row[m_wptairportcolumns.m_name] = el.get_name();
		row[m_wptairportcolumns.m_type] = el.get_type_string();
		row[m_wptairportcolumns.m_elev] = Conversions::alt_str(el.get_elevation(), 0);
		row[m_wptairportcolumns.m_style] = Pango::STYLE_NORMAL;
		double tkoffgnddist(std::numeric_limits<double>::max()), ldggnddist(tkoffgnddist);
		if (m_sensors) {
			const Aircraft::Distances& dist(m_sensors->get_aircraft().get_dist());
			double mass(m_perfvalues[0][perfval_mass]), headwind(0);
			AirData<double> ad;
			ad.set_qnh_tempoffs(m_sensors->get_altimeter_qnh(), m_sensors->get_altimeter_tempoffs());
			for (unsigned int i(0); i < dist.get_nrtakeoffdist(); ++i) {
				const Aircraft::Distances::Distance& d(dist.get_takeoffdist(i));
				double gnddist, obstdist;
				d.calculate(gnddist, obstdist, ad.get_density_altitude(), headwind, mass);
				if (std::isnan(gnddist))
					continue;
				gnddist = Aircraft::convert(dist.get_distunit(), Aircraft::unit_m, gnddist);
				tkoffgnddist = std::min(tkoffgnddist, gnddist);
			}
			for (unsigned int i(0); i < dist.get_nrldgdist(); ++i) {
				const Aircraft::Distances::Distance& d(dist.get_ldgdist(i));
				double gnddist, obstdist;
				d.calculate(gnddist, obstdist, ad.get_density_altitude(), headwind, mass);
				if (std::isnan(gnddist))
					continue;
				gnddist = Aircraft::convert(dist.get_distunit(), Aircraft::unit_m, gnddist);
				ldggnddist = std::min(ldggnddist, gnddist);
			}
		}
		if (tkoffgnddist == std::numeric_limits<double>::max())
			tkoffgnddist = 0;
		if (ldggnddist == std::numeric_limits<double>::max())
			ldggnddist = 0;
		double softtkoffgnddist(tkoffgnddist * (1 + m_perfsoftfieldpenalty));
		{
			unsigned int rwyl(0), rwyw(0);
			Glib::ustring rwyi;
			Glib::ustring rwysfc;
			bool cantkoff(false), canland(false);
			for (unsigned int i(0); i < el.get_nrrwy(); ++i) {
				const AirportsDb::Airport::Runway& rwy(el.get_rwy(i));
				if (rwy.is_he_usable()) {
					cantkoff = cantkoff || (rwy.get_he_tda() >= (rwy.is_concrete() ? tkoffgnddist : softtkoffgnddist));
					canland = canland || (rwy.get_he_lda() >= ldggnddist);
				}
				if (rwy.is_le_usable()) {
					cantkoff = cantkoff || (rwy.get_le_tda() >= (rwy.is_concrete() ? tkoffgnddist : softtkoffgnddist));
					canland = canland || (rwy.get_le_lda() >= ldggnddist);
				}
				if (rwy.get_length() < rwyl)
					continue;
				if (!rwy.is_he_usable() && !rwy.is_le_usable())
					continue;
				if (rwy.get_length() > rwyl) {
					rwyl = rwy.get_length();
					rwyw = rwy.get_width();
					rwyi.clear();
					rwysfc = rwy.get_surface();
				} else {
					rwyw = std::min(rwyw, (unsigned int)rwy.get_width());
				}
				if (rwy.is_he_usable()) {
					if (!rwyi.empty())
						rwyi += "/";
					rwyi += rwy.get_ident_he();
				}
				if (rwy.is_le_usable()) {
					if (!rwyi.empty())
						rwyi += "/";
					rwyi += rwy.get_ident_le();
				}
			}
			if (rwyl) {
				row[m_wptairportcolumns.m_rwylength] = Glib::ustring::format(std::fixed, std::setprecision(0), rwyl * Point::ft_to_m_dbl);
				row[m_wptairportcolumns.m_rwywidth] = Glib::ustring::format(std::fixed, std::setprecision(0), rwyw * Point::ft_to_m_dbl);
				row[m_wptairportcolumns.m_rwyident] = rwyi;
				row[m_wptairportcolumns.m_rwysfc] = rwysfc;
			}
			Gdk::Color col;
			if (cantkoff && canland)
				col.set_rgb(0, 0, 0);
			else if (cantkoff || canland)
				col.set_rgb(0xffff, 0x8000, 0);
			else
				col.set_rgb(0xffff, 0, 0);
			row[m_wptairportcolumns.m_color] = col;
		}
		{
			unsigned int prio(0);
			Glib::ustring twr("twr");
			twr = twr.casefold();
			for (unsigned int i(0); i < el.get_nrcomm(); ++i) {
				const AirportsDb::Airport::Comm& comm(el.get_comm(i));
				if (!comm[0])
					continue;
				unsigned int p(1);
				Glib::ustring name(comm.get_name().casefold());
				if (name == twr)
					p = 3;
				else if (name.find(twr) != Glib::ustring::npos)
					p = 2;
				if (p <= prio)
					continue;
				row[m_wptairportcolumns.m_comm] = Conversions::freq_str(comm[0]);
			}
		}
		row[m_wptairportcolumns.m_routeid] = WaypointAirportModelColumns::noid;
		row[m_wptairportcolumns.m_routepointid] = WaypointAirportModelColumns::noid;
		for (unsigned int routeid(0); routeid < elp->get_nrvfrrte(); ++routeid) {
			const AirportsDb::Airport::VFRRoute& vfrrte(elp->get_vfrrte(routeid));
			Gtk::TreeModel::Row rterow(*(m_wptstore->append(row.children())));
			rterow[m_wptairportcolumns.m_element] = elp;
			rterow[m_wptairportcolumns.m_icao] = el.get_icao();
			rterow[m_wptairportcolumns.m_name] = vfrrte.get_name();
			rterow[m_wptairportcolumns.m_routeid] = routeid;
			rterow[m_wptairportcolumns.m_routepointid] = WaypointAirportModelColumns::noid;
			if (vfrrte.size() > 0) {
				const AirportsDb::Airport::VFRRoute::VFRRoutePoint& pt(vfrrte[0]);
				if (!pt.is_at_airport() && pt.get_coord() != el.get_coord()) {
					rterow[m_wptairportcolumns.m_type] = pt.get_pathcode_string();
					rterow[m_wptairportcolumns.m_elev] = pt.get_altcode_string() + Conversions::alt_str(pt.get_altitude(), 0);
				}
			}
			if (vfrrte.size() > 1) {
				const AirportsDb::Airport::VFRRoute::VFRRoutePoint& pt(vfrrte[vfrrte.size()-1]);
				if (!pt.is_at_airport() && pt.get_coord() != el.get_coord()) {
					rterow[m_wptairportcolumns.m_type] = pt.get_pathcode_string();
					rterow[m_wptairportcolumns.m_elev] = pt.get_altcode_string() + Conversions::alt_str(pt.get_altitude(), 0);
				}
			}
			for (unsigned int routepointid(0); routepointid < vfrrte.size(); ++routepointid) {
				const AirportsDb::Airport::VFRRoute::VFRRoutePoint& pt(vfrrte[routepointid]);
				Gtk::TreeModel::Row ptrow(*(m_wptstore->append(rterow.children())));
				ptrow[m_wptairportcolumns.m_element] = elp;
				ptrow[m_wptairportcolumns.m_icao] = el.get_icao();
				ptrow[m_wptairportcolumns.m_name] = pt.get_name();
				ptrow[m_wptairportcolumns.m_type] = pt.get_pathcode_string();
				ptrow[m_wptairportcolumns.m_elev] = pt.get_altcode_string() + Conversions::alt_str(pt.get_altitude(), 0);
				ptrow[m_wptairportcolumns.m_routeid] = routeid;
				ptrow[m_wptairportcolumns.m_routepointid] = routepointid;
			}
		}
        }
	wptpage_updatedist(m_wptairportcolumns);
	if (m_wptstore) {
		if (m_wptpagemode == wptpagemode_text)
			m_wptstore->set_sort_column(m_wptairportcolumns.m_name, Gtk::SORT_ASCENDING);
		else
			m_wptstore->set_sort_column(m_wptairportcolumns.m_dist, Gtk::SORT_ASCENDING);
	}
	if (m_wpttreeview)
		m_wpttreeview->set_model(m_wptstore);
}

void FlightDeckWindow::wptpage_setresult(const Engine::NavaidResult::elementvector_t& elements)
{
	m_wpttreeviewselconn.disconnect();
	m_wptstore = Gtk::TreeStore::create(m_wptnavaidcolumns);
	if (m_wpttreeview) {
		m_wpttreeview->unset_model();
		m_wpttreeview->remove_all_columns();
		Gtk::CellRendererText *name_renderer = Gtk::manage(new Gtk::CellRendererText());
		int name_col = m_wpttreeview->append_column(_("Name"), *name_renderer) - 1;
		Gtk::TreeViewColumn *name_column = m_wpttreeview->get_column(name_col);
                if (name_column)
                        name_column->add_attribute(*name_renderer, "text", m_wptnavaidcolumns.m_name);
		Gtk::CellRendererText *icao_renderer = Gtk::manage(new Gtk::CellRendererText());
		int icao_col = m_wpttreeview->append_column(_("ICAO"), *icao_renderer) - 1;
		Gtk::TreeViewColumn *icao_column = m_wpttreeview->get_column(icao_col);
                if (icao_column)
                        icao_column->add_attribute(*icao_renderer, "text", m_wptnavaidcolumns.m_icao);
		Gtk::CellRendererText *type_renderer = Gtk::manage(new Gtk::CellRendererText());
		int type_col = m_wpttreeview->append_column(_("Type"), *type_renderer) - 1;
		Gtk::TreeViewColumn *type_column = m_wpttreeview->get_column(type_col);
                if (type_column)
                        type_column->add_attribute(*type_renderer, "text", m_wptnavaidcolumns.m_type);
		Gtk::CellRendererText *elev_renderer = Gtk::manage(new Gtk::CellRendererText());
		int elev_col = m_wpttreeview->append_column(_("Elev"), *elev_renderer) - 1;
		Gtk::TreeViewColumn *elev_column = m_wpttreeview->get_column(elev_col);
                if (elev_column)
                        elev_column->add_attribute(*elev_renderer, "text", m_wptnavaidcolumns.m_elev);
		Gtk::CellRendererText *freq_renderer = Gtk::manage(new Gtk::CellRendererText());
		int freq_col = m_wpttreeview->append_column(_("Freq"), *freq_renderer) - 1;
		Gtk::TreeViewColumn *freq_column = m_wpttreeview->get_column(freq_col);
                if (freq_column)
                        freq_column->add_attribute(*freq_renderer, "text", m_wptnavaidcolumns.m_freq);
		Gtk::CellRendererText *range_renderer = Gtk::manage(new Gtk::CellRendererText());
		int range_col = m_wpttreeview->append_column(_("Range"), *range_renderer) - 1;
		Gtk::TreeViewColumn *range_column = m_wpttreeview->get_column(range_col);
                if (range_column)
                        range_column->add_attribute(*range_renderer, "text", m_wptnavaidcolumns.m_range);
		Gtk::CellRendererText *dist_renderer = Gtk::manage(new Gtk::CellRendererText());
		int dist_col = m_wpttreeview->append_column(_("Dist"), *dist_renderer) - 1;
		Gtk::TreeViewColumn *dist_column = m_wpttreeview->get_column(dist_col);
                if (dist_column)
                        dist_column->add_attribute(*dist_renderer, "text", m_wptnavaidcolumns.m_dist);
                elev_renderer->set_property("xalign", 1.0);
                freq_renderer->set_property("xalign", 1.0);
                range_renderer->set_property("xalign", 1.0);
                dist_renderer->set_property("xalign", 1.0);
		const Gtk::TreeModelColumnBase *cols[] = { &m_wptnavaidcolumns.m_name,
							   &m_wptnavaidcolumns.m_icao,
							   &m_wptnavaidcolumns.m_type,
							   &m_wptnavaidcolumns.m_elev,
							   &m_wptnavaidcolumns.m_freq,
							   &m_wptnavaidcolumns.m_range,
							   &m_wptnavaidcolumns.m_dist };
		for (unsigned int i = 0; i < sizeof(cols)/sizeof(cols[0]); ++i) {
			Gtk::TreeViewColumn *col = m_wpttreeview->get_column(i);
			if (!col)
				continue;
			col->set_resizable(true);
			col->set_sort_column(*cols[i]);
			col->set_expand(i == 0);
		}
		m_wptstore->set_sort_func(m_wptnavaidcolumns.m_dist, sigc::mem_fun(*this, &FlightDeckWindow::wptpage_navaid_sortdist));
		m_wpttreeview->columns_autosize();
		m_wpttreeview->set_headers_clickable();
		Glib::RefPtr<Gtk::TreeSelection> selection(m_wpttreeview->get_selection());
		selection->set_mode(Gtk::SELECTION_SINGLE);
		m_wpttreeviewselconn = selection->signal_changed().connect(sigc::mem_fun(*this, &FlightDeckWindow::wptpage_navaid_selection_changed));
		//selection->set_select_function(sigc::mem_fun(*this, &FlightDeckWindow::wpt_select_function));
	}
	if (!m_wptstore)
		return;
	m_wptstore->clear();
	for (Engine::NavaidResult::elementvector_t::const_iterator i(elements.begin()), e(elements.end()); i != e; ++i) {
		const Engine::NavaidResult::element_t& el(*i);
                Gtk::TreeModel::Row row(*(m_wptstore->append()));
		row[m_wptnavaidcolumns.m_element] = el;
		row[m_wptnavaidcolumns.m_icao] = el.get_icao();
		row[m_wptnavaidcolumns.m_name] = el.get_name();
		row[m_wptnavaidcolumns.m_type] = el.get_navaid_typename();
		row[m_wptnavaidcolumns.m_elev] = Conversions::alt_str(el.get_elev(), 0);
		row[m_wptnavaidcolumns.m_freq] = Conversions::freq_str(el.get_frequency());
		row[m_wptnavaidcolumns.m_range] = Glib::ustring::format(el.get_range());
        }
	wptpage_updatedist(m_wptnavaidcolumns);
	if (m_wptstore) {
		if (m_wptpagemode == wptpagemode_text)
			m_wptstore->set_sort_column(m_wptnavaidcolumns.m_name, Gtk::SORT_ASCENDING);
		else
			m_wptstore->set_sort_column(m_wptnavaidcolumns.m_dist, Gtk::SORT_ASCENDING);
	}
	if (m_wpttreeview)
		m_wpttreeview->set_model(m_wptstore);
}

void FlightDeckWindow::wptpage_setresult(const Engine::WaypointResult::elementvector_t& elements)
{
	m_wpttreeviewselconn.disconnect();
	m_wptstore = Gtk::TreeStore::create(m_wptintersectioncolumns);
	if (m_wpttreeview) {
		m_wpttreeview->unset_model();
		m_wpttreeview->remove_all_columns();
		Gtk::CellRendererText *name_renderer = Gtk::manage(new Gtk::CellRendererText());
		int name_col = m_wpttreeview->append_column(_("Name"), *name_renderer) - 1;
		Gtk::TreeViewColumn *name_column = m_wpttreeview->get_column(name_col);
                if (name_column)
                        name_column->add_attribute(*name_renderer, "text", m_wptintersectioncolumns.m_name);
		Gtk::CellRendererText *usage_renderer = Gtk::manage(new Gtk::CellRendererText());
		int usage_col = m_wpttreeview->append_column(_("Usage"), *usage_renderer) - 1;
		Gtk::TreeViewColumn *usage_column = m_wpttreeview->get_column(usage_col);
                if (usage_column)
                        usage_column->add_attribute(*usage_renderer, "text", m_wptintersectioncolumns.m_usage);
		Gtk::CellRendererText *dist_renderer = Gtk::manage(new Gtk::CellRendererText());
		int dist_col = m_wpttreeview->append_column(_("Dist"), *dist_renderer) - 1;
		Gtk::TreeViewColumn *dist_column = m_wpttreeview->get_column(dist_col);
                if (dist_column)
                        dist_column->add_attribute(*dist_renderer, "text", m_wptintersectioncolumns.m_dist);
                dist_renderer->set_property("xalign", 1.0);
		const Gtk::TreeModelColumnBase *cols[] = { &m_wptintersectioncolumns.m_name,
							   &m_wptintersectioncolumns.m_usage,
							   &m_wptintersectioncolumns.m_dist };
		for (unsigned int i = 0; i < sizeof(cols)/sizeof(cols[0]); ++i) {
			Gtk::TreeViewColumn *col = m_wpttreeview->get_column(i);
			if (!col)
				continue;
			col->set_resizable(true);
			col->set_sort_column(*cols[i]);
			col->set_expand(i == 0);
		}
		m_wptstore->set_sort_func(m_wptintersectioncolumns.m_dist, sigc::mem_fun(*this, &FlightDeckWindow::wptpage_waypoint_sortdist));
		m_wpttreeview->columns_autosize();
		m_wpttreeview->set_headers_clickable();
		Glib::RefPtr<Gtk::TreeSelection> selection(m_wpttreeview->get_selection());
		selection->set_mode(Gtk::SELECTION_SINGLE);
		m_wpttreeviewselconn = selection->signal_changed().connect(sigc::mem_fun(*this, &FlightDeckWindow::wptpage_waypoint_selection_changed));
		//selection->set_select_function(sigc::mem_fun(*this, &FlightDeckWindow::wpt_select_function));
	}
	if (!m_wptstore)
		return;
	m_wptstore->clear();
	for (Engine::WaypointResult::elementvector_t::const_iterator i(elements.begin()), e(elements.end()); i != e; ++i) {
		const Engine::WaypointResult::element_t& el(*i);
                Gtk::TreeModel::Row row(*(m_wptstore->append()));
		row[m_wptintersectioncolumns.m_element] = el;
		row[m_wptintersectioncolumns.m_name] = el.get_name();
		row[m_wptintersectioncolumns.m_usage] = el.get_usage_name();
        }
	wptpage_updatedist(m_wptintersectioncolumns);
	if (m_wptstore) {
		if (m_wptpagemode == wptpagemode_text)
			m_wptstore->set_sort_column(m_wptintersectioncolumns.m_name, Gtk::SORT_ASCENDING);
		else
			m_wptstore->set_sort_column(m_wptintersectioncolumns.m_dist, Gtk::SORT_ASCENDING);
	}
	if (m_wpttreeview)
		m_wpttreeview->set_model(m_wptstore);
}

void FlightDeckWindow::wptpage_setresult(const Engine::AirwayResult::elementvector_t& elements)
{
	m_wpttreeviewselconn.disconnect();
	m_wptstore = Gtk::TreeStore::create(m_wptairwaycolumns);
	if (m_wpttreeview) {
		m_wpttreeview->unset_model();
		m_wpttreeview->remove_all_columns();
		Gtk::CellRendererText *name_renderer = Gtk::manage(new Gtk::CellRendererText());
		int name_col = m_wpttreeview->append_column(_("Name"), *name_renderer) - 1;
		Gtk::TreeViewColumn *name_column = m_wpttreeview->get_column(name_col);
                if (name_column)
                        name_column->add_attribute(*name_renderer, "text", m_wptairwaycolumns.m_name);
		Gtk::CellRendererText *fromname_renderer = Gtk::manage(new Gtk::CellRendererText());
		int fromname_col = m_wpttreeview->append_column(_("From"), *fromname_renderer) - 1;
		Gtk::TreeViewColumn *fromname_column = m_wpttreeview->get_column(fromname_col);
                if (fromname_column)
                        fromname_column->add_attribute(*fromname_renderer, "text", m_wptairwaycolumns.m_fromname);
		Gtk::CellRendererText *toname_renderer = Gtk::manage(new Gtk::CellRendererText());
		int toname_col = m_wpttreeview->append_column(_("To"), *toname_renderer) - 1;
		Gtk::TreeViewColumn *toname_column = m_wpttreeview->get_column(toname_col);
                if (toname_column)
                        toname_column->add_attribute(*toname_renderer, "text", m_wptairwaycolumns.m_toname);
		Gtk::CellRendererText *basealt_renderer = Gtk::manage(new Gtk::CellRendererText());
		int basealt_col = m_wpttreeview->append_column(_("Base"), *basealt_renderer) - 1;
		Gtk::TreeViewColumn *basealt_column = m_wpttreeview->get_column(basealt_col);
                if (basealt_column)
                        basealt_column->add_attribute(*basealt_renderer, "text", m_wptairwaycolumns.m_basealt);
		Gtk::CellRendererText *topalt_renderer = Gtk::manage(new Gtk::CellRendererText());
		int topalt_col = m_wpttreeview->append_column(_("Top"), *topalt_renderer) - 1;
		Gtk::TreeViewColumn *topalt_column = m_wpttreeview->get_column(topalt_col);
                if (topalt_column)
                        topalt_column->add_attribute(*topalt_renderer, "text", m_wptairwaycolumns.m_topalt);
		Gtk::CellRendererText *terrainelev_renderer = Gtk::manage(new Gtk::CellRendererText());
		int terrainelev_col = m_wpttreeview->append_column(_("Terrain"), *terrainelev_renderer) - 1;
		Gtk::TreeViewColumn *terrainelev_column = m_wpttreeview->get_column(terrainelev_col);
                if (terrainelev_column)
                        terrainelev_column->add_attribute(*terrainelev_renderer, "text", m_wptairwaycolumns.m_terrainelev);
		Gtk::CellRendererText *corridor5elev_renderer = Gtk::manage(new Gtk::CellRendererText());
		int corridor5elev_col = m_wpttreeview->append_column(_("Corridor"), *corridor5elev_renderer) - 1;
		Gtk::TreeViewColumn *corridor5elev_column = m_wpttreeview->get_column(corridor5elev_col);
                if (corridor5elev_column)
                        corridor5elev_column->add_attribute(*corridor5elev_renderer, "text", m_wptairwaycolumns.m_corridor5elev);
		Gtk::CellRendererText *type_renderer = Gtk::manage(new Gtk::CellRendererText());
		int type_col = m_wpttreeview->append_column(_("Type"), *type_renderer) - 1;
		Gtk::TreeViewColumn *type_column = m_wpttreeview->get_column(type_col);
                if (type_column)
                        type_column->add_attribute(*type_renderer, "text", m_wptairwaycolumns.m_type);
		Gtk::CellRendererText *dist_renderer = Gtk::manage(new Gtk::CellRendererText());
		int dist_col = m_wpttreeview->append_column(_("Dist"), *dist_renderer) - 1;
		Gtk::TreeViewColumn *dist_column = m_wpttreeview->get_column(dist_col);
                if (dist_column)
                        dist_column->add_attribute(*dist_renderer, "text", m_wptairwaycolumns.m_dist);
                basealt_renderer->set_property("xalign", 1.0);
                topalt_renderer->set_property("xalign", 1.0);
                terrainelev_renderer->set_property("xalign", 1.0);
                corridor5elev_renderer->set_property("xalign", 1.0);
                dist_renderer->set_property("xalign", 1.0);
		const Gtk::TreeModelColumnBase *cols[] = { &m_wptairwaycolumns.m_name,
							   &m_wptairwaycolumns.m_fromname,
							   &m_wptairwaycolumns.m_toname,
							   &m_wptairwaycolumns.m_basealt,
							   &m_wptairwaycolumns.m_topalt,
							   &m_wptairwaycolumns.m_terrainelev,
							   &m_wptairwaycolumns.m_corridor5elev,
							   &m_wptairwaycolumns.m_type,
							   &m_wptairwaycolumns.m_dist };
		for (unsigned int i = 0; i < sizeof(cols)/sizeof(cols[0]); ++i) {
			Gtk::TreeViewColumn *col = m_wpttreeview->get_column(i);
			if (!col)
				continue;
			col->set_resizable(true);
			col->set_sort_column(*cols[i]);
			col->set_expand(i == 0);
		}
		m_wptstore->set_sort_func(m_wptairwaycolumns.m_dist, sigc::mem_fun(*this, &FlightDeckWindow::wptpage_airway_sortdist));
		m_wpttreeview->columns_autosize();
		m_wpttreeview->set_headers_clickable();
		Glib::RefPtr<Gtk::TreeSelection> selection(m_wpttreeview->get_selection());
		selection->set_mode(Gtk::SELECTION_SINGLE);
		m_wpttreeviewselconn = selection->signal_changed().connect(sigc::mem_fun(*this, &FlightDeckWindow::wptpage_airway_selection_changed));
		//selection->set_select_function(sigc::mem_fun(*this, &FlightDeckWindow::wpt_select_function));
	}
	if (!m_wptstore)
		return;
	m_wptstore->clear();
	for (Engine::AirwayResult::elementvector_t::const_iterator i(elements.begin()), e(elements.end()); i != e; ++i) {
		const Engine::AirwayResult::element_t& el(*i);
                Gtk::TreeModel::Row row(*(m_wptstore->append()));
		row[m_wptairwaycolumns.m_element] = el;
		row[m_wptairwaycolumns.m_name] = el.get_name();
		row[m_wptairwaycolumns.m_fromname] = el.get_begin_name();
		row[m_wptairwaycolumns.m_toname] = el.get_end_name();
		row[m_wptairwaycolumns.m_basealt] = Conversions::alt_str(el.get_base_level(), 0);
		row[m_wptairwaycolumns.m_topalt] = Conversions::alt_str(el.get_top_level(), 0);
		row[m_wptairwaycolumns.m_terrainelev] = Conversions::alt_str(el.get_terrain_elev(), 0);
		row[m_wptairwaycolumns.m_corridor5elev] = Conversions::alt_str(el.get_corridor5_elev(), 0);
		row[m_wptairwaycolumns.m_type] = el.get_type_name();
        }
	wptpage_updatedist(m_wptairwaycolumns);
	if (m_wptstore) {
		if (m_wptpagemode == wptpagemode_text)
			m_wptstore->set_sort_column(m_wptairwaycolumns.m_name, Gtk::SORT_ASCENDING);
		else
			m_wptstore->set_sort_column(m_wptairwaycolumns.m_dist, Gtk::SORT_ASCENDING);
	}
	if (m_wpttreeview)
		m_wpttreeview->set_model(m_wptstore);
}

void FlightDeckWindow::wptpage_setresult(const Engine::AirspaceResult::elementvector_t& elements)
{
	m_wpttreeviewselconn.disconnect();
	m_wptstore = Gtk::TreeStore::create(m_wptairspacecolumns);
	if (m_wpttreeview) {
		m_wpttreeview->unset_model();
		m_wpttreeview->remove_all_columns();
		Gtk::CellRendererText *name_renderer = Gtk::manage(new Gtk::CellRendererText());
		int name_col = m_wpttreeview->append_column(_("Name"), *name_renderer) - 1;
		Gtk::TreeViewColumn *name_column = m_wpttreeview->get_column(name_col);
                if (name_column)
                        name_column->add_attribute(*name_renderer, "text", m_wptairspacecolumns.m_name);
		Gtk::CellRendererText *icao_renderer = Gtk::manage(new Gtk::CellRendererText());
		int icao_col = m_wpttreeview->append_column(_("ICAO"), *icao_renderer) - 1;
		Gtk::TreeViewColumn *icao_column = m_wpttreeview->get_column(icao_col);
                if (icao_column)
                        icao_column->add_attribute(*icao_renderer, "text", m_wptairspacecolumns.m_icao);
		Gtk::CellRendererText *class_renderer = Gtk::manage(new Gtk::CellRendererText());
		int class_col = m_wpttreeview->append_column(_("Class"), *class_renderer) - 1;
		Gtk::TreeViewColumn *class_column = m_wpttreeview->get_column(class_col);
                if (class_column)
                        class_column->add_attribute(*class_renderer, "text", m_wptairspacecolumns.m_class);
		Gtk::CellRendererText *lwralt_renderer = Gtk::manage(new Gtk::CellRendererText());
		int lwralt_col = m_wpttreeview->append_column(_("Lower"), *lwralt_renderer) - 1;
		Gtk::TreeViewColumn *lwralt_column = m_wpttreeview->get_column(lwralt_col);
                if (lwralt_column)
                        lwralt_column->add_attribute(*lwralt_renderer, "text", m_wptairspacecolumns.m_lwralt);
		Gtk::CellRendererText *upralt_renderer = Gtk::manage(new Gtk::CellRendererText());
		int upralt_col = m_wpttreeview->append_column(_("Upper"), *upralt_renderer) - 1;
		Gtk::TreeViewColumn *upralt_column = m_wpttreeview->get_column(upralt_col);
                if (upralt_column)
                        upralt_column->add_attribute(*upralt_renderer, "text", m_wptairspacecolumns.m_upralt);
		Gtk::CellRendererText *comm_renderer = Gtk::manage(new Gtk::CellRendererText());
		int comm_col = m_wpttreeview->append_column(_("Comm"), *comm_renderer) - 1;
		Gtk::TreeViewColumn *comm_column = m_wpttreeview->get_column(comm_col);
                if (comm_column)
                        comm_column->add_attribute(*comm_renderer, "text", m_wptairspacecolumns.m_comm);
		Gtk::CellRendererText *comm0_renderer = Gtk::manage(new Gtk::CellRendererText());
		int comm0_col = m_wpttreeview->append_column(_("Comm 0"), *comm0_renderer) - 1;
		Gtk::TreeViewColumn *comm0_column = m_wpttreeview->get_column(comm0_col);
                if (comm0_column)
                        comm0_column->add_attribute(*comm0_renderer, "text", m_wptairspacecolumns.m_comm0);
		Gtk::CellRendererText *comm1_renderer = Gtk::manage(new Gtk::CellRendererText());
		int comm1_col = m_wpttreeview->append_column(_("Comm 1"), *comm1_renderer) - 1;
		Gtk::TreeViewColumn *comm1_column = m_wpttreeview->get_column(comm1_col);
                if (comm1_column)
                        comm1_column->add_attribute(*comm1_renderer, "text", m_wptairspacecolumns.m_comm1);
		Gtk::CellRendererText *dist_renderer = Gtk::manage(new Gtk::CellRendererText());
		int dist_col = m_wpttreeview->append_column(_("Dist"), *dist_renderer) - 1;
		Gtk::TreeViewColumn *dist_column = m_wpttreeview->get_column(dist_col);
                if (dist_column)
                        dist_column->add_attribute(*dist_renderer, "text", m_wptairspacecolumns.m_dist);
                lwralt_renderer->set_property("xalign", 1.0);
                upralt_renderer->set_property("xalign", 1.0);
                comm0_renderer->set_property("xalign", 1.0);
                comm1_renderer->set_property("xalign", 1.0);
                dist_renderer->set_property("xalign", 1.0);
		const Gtk::TreeModelColumnBase *cols[] = { &m_wptairspacecolumns.m_name,
							   &m_wptairspacecolumns.m_icao,
							   &m_wptairspacecolumns.m_class,
							   &m_wptairspacecolumns.m_lwralt,
							   &m_wptairspacecolumns.m_upralt,
							   &m_wptairspacecolumns.m_comm,
							   &m_wptairspacecolumns.m_comm0,
							   &m_wptairspacecolumns.m_comm1,
							   &m_wptairspacecolumns.m_dist };
		for (unsigned int i = 0; i < sizeof(cols)/sizeof(cols[0]); ++i) {
			Gtk::TreeViewColumn *col = m_wpttreeview->get_column(i);
			if (!col)
				continue;
			col->set_resizable(true);
			col->set_sort_column(*cols[i]);
			col->set_expand(i == 0);
		}
		m_wptstore->set_sort_func(m_wptairspacecolumns.m_dist, sigc::mem_fun(*this, &FlightDeckWindow::wptpage_airspace_sortdist));
		m_wpttreeview->columns_autosize();
		m_wpttreeview->set_headers_clickable();
		Glib::RefPtr<Gtk::TreeSelection> selection(m_wpttreeview->get_selection());
		selection->set_mode(Gtk::SELECTION_SINGLE);
		m_wpttreeviewselconn = selection->signal_changed().connect(sigc::mem_fun(*this, &FlightDeckWindow::wptpage_airspace_selection_changed));
		//selection->set_select_function(sigc::mem_fun(*this, &FlightDeckWindow::wpt_select_function));
	}
	if (!m_wptstore)
		return;
	m_wptstore->clear();
	for (Engine::AirspaceResult::elementvector_t::const_iterator i(elements.begin()), e(elements.end()); i != e; ++i) {
		const Engine::AirspaceResult::element_t& el(*i);
                Gtk::TreeModel::Row row(*(m_wptstore->append()));
		row[m_wptairspacecolumns.m_element] = el;
		row[m_wptairspacecolumns.m_icao] = el.get_icao();
		row[m_wptairspacecolumns.m_name] = el.get_name();
		row[m_wptairspacecolumns.m_class] = el.get_class_string();
		row[m_wptairspacecolumns.m_lwralt] = el.get_altlwr_string();
		row[m_wptairspacecolumns.m_upralt] = el.get_altupr_string();
		row[m_wptairspacecolumns.m_comm] = el.get_commname();
		row[m_wptairspacecolumns.m_comm0] = Conversions::freq_str(el.get_commfreq(0));
		row[m_wptairspacecolumns.m_comm1] = Conversions::freq_str(el.get_commfreq(1));
        }
	wptpage_updatedist(m_wptairspacecolumns);
	if (m_wptstore) {
		if (m_wptpagemode == wptpagemode_text)
			m_wptstore->set_sort_column(m_wptairspacecolumns.m_name, Gtk::SORT_ASCENDING);
		else
			m_wptstore->set_sort_column(m_wptairspacecolumns.m_dist, Gtk::SORT_ASCENDING);
	}
	if (m_wpttreeview)
		m_wpttreeview->set_model(m_wptstore);
}

void FlightDeckWindow::wptpage_setresult(const Engine::MapelementResult::elementvector_t& elements)
{
	m_wpttreeviewselconn.disconnect();
	m_wptstore = Gtk::TreeStore::create(m_wptmapelementcolumns);
	if (m_wpttreeview) {
		m_wpttreeview->unset_model();
		m_wpttreeview->remove_all_columns();
		Gtk::CellRendererText *name_renderer = Gtk::manage(new Gtk::CellRendererText());
		int name_col = m_wpttreeview->append_column(_("Name"), *name_renderer) - 1;
		Gtk::TreeViewColumn *name_column = m_wpttreeview->get_column(name_col);
                if (name_column)
                        name_column->add_attribute(*name_renderer, "text", m_wptmapelementcolumns.m_name);
		Gtk::CellRendererText *type_renderer = Gtk::manage(new Gtk::CellRendererText());
		int type_col = m_wpttreeview->append_column(_("Type"), *type_renderer) - 1;
		Gtk::TreeViewColumn *type_column = m_wpttreeview->get_column(type_col);
                if (type_column)
                        type_column->add_attribute(*type_renderer, "text", m_wptmapelementcolumns.m_type);
		Gtk::CellRendererText *dist_renderer = Gtk::manage(new Gtk::CellRendererText());
		int dist_col = m_wpttreeview->append_column(_("Dist"), *dist_renderer) - 1;
		Gtk::TreeViewColumn *dist_column = m_wpttreeview->get_column(dist_col);
                if (dist_column)
                        dist_column->add_attribute(*dist_renderer, "text", m_wptmapelementcolumns.m_dist);
                dist_renderer->set_property("xalign", 1.0);
		const Gtk::TreeModelColumnBase *cols[] = { &m_wptmapelementcolumns.m_name,
							   &m_wptmapelementcolumns.m_type,
							   &m_wptmapelementcolumns.m_dist };
		for (unsigned int i = 0; i < sizeof(cols)/sizeof(cols[0]); ++i) {
			Gtk::TreeViewColumn *col = m_wpttreeview->get_column(i);
			if (!col)
				continue;
			col->set_resizable(true);
			col->set_sort_column(*cols[i]);
			col->set_expand(i == 0);
		}
		m_wptstore->set_sort_func(m_wptmapelementcolumns.m_dist, sigc::mem_fun(*this, &FlightDeckWindow::wptpage_mapelement_sortdist));
		m_wpttreeview->columns_autosize();
		m_wpttreeview->set_headers_clickable();
		Glib::RefPtr<Gtk::TreeSelection> selection(m_wpttreeview->get_selection());
		selection->set_mode(Gtk::SELECTION_SINGLE);
		m_wpttreeviewselconn = selection->signal_changed().connect(sigc::mem_fun(*this, &FlightDeckWindow::wptpage_mapelement_selection_changed));
		//selection->set_select_function(sigc::mem_fun(*this, &FlightDeckWindow::wpt_select_function));
	}
	if (!m_wptstore)
		return;
	m_wptstore->clear();
	for (Engine::MapelementResult::elementvector_t::const_iterator i(elements.begin()), e(elements.end()); i != e; ++i) {
		const Engine::MapelementResult::element_t& el(*i);
                Gtk::TreeModel::Row row(*(m_wptstore->append()));
		row[m_wptmapelementcolumns.m_element] = el;
		row[m_wptmapelementcolumns.m_name] = el.get_name();
		row[m_wptmapelementcolumns.m_type] = el.get_typename();
        }
	wptpage_updatedist(m_wptmapelementcolumns);
	if (m_wptstore) {
		if (m_wptpagemode == wptpagemode_text)
			m_wptstore->set_sort_column(m_wptmapelementcolumns.m_name, Gtk::SORT_ASCENDING);
		else
			m_wptstore->set_sort_column(m_wptmapelementcolumns.m_dist, Gtk::SORT_ASCENDING);
	}
	if (m_wpttreeview)
		m_wpttreeview->set_model(m_wptstore);
}

void FlightDeckWindow::wptpage_setresult(const std::vector<FPlanWaypoint>& elements)
{
	m_wpttreeviewselconn.disconnect();
	m_wptstore = Gtk::TreeStore::create(m_wptfplancolumns);
	if (m_wpttreeview) {
		m_wpttreeview->unset_model();
		m_wpttreeview->remove_all_columns();
		Gtk::CellRendererText *name_renderer = Gtk::manage(new Gtk::CellRendererText());
		int name_col = m_wpttreeview->append_column(_("Name"), *name_renderer) - 1;
		Gtk::TreeViewColumn *name_column = m_wpttreeview->get_column(name_col);
                if (name_column)
                        name_column->add_attribute(*name_renderer, "text", m_wptfplancolumns.m_name);
		Gtk::CellRendererText *icao_renderer = Gtk::manage(new Gtk::CellRendererText());
		int icao_col = m_wpttreeview->append_column(_("ICAO"), *icao_renderer) - 1;
		Gtk::TreeViewColumn *icao_column = m_wpttreeview->get_column(icao_col);
                if (icao_column)
                        icao_column->add_attribute(*icao_renderer, "text", m_wptfplancolumns.m_icao);
		Gtk::CellRendererText *alt_renderer = Gtk::manage(new Gtk::CellRendererText());
		int alt_col = m_wpttreeview->append_column(_("Altitude"), *alt_renderer) - 1;
		Gtk::TreeViewColumn *alt_column = m_wpttreeview->get_column(alt_col);
                if (alt_column)
                        alt_column->add_attribute(*alt_renderer, "text", m_wptfplancolumns.m_altitude);
		Gtk::CellRendererText *freq_renderer = Gtk::manage(new Gtk::CellRendererText());
		int freq_col = m_wpttreeview->append_column(_("Freq"), *freq_renderer) - 1;
		Gtk::TreeViewColumn *freq_column = m_wpttreeview->get_column(freq_col);
                if (freq_column)
                        freq_column->add_attribute(*freq_renderer, "text", m_wptfplancolumns.m_freq);
		Gtk::CellRendererText *type_renderer = Gtk::manage(new Gtk::CellRendererText());
		int type_col = m_wpttreeview->append_column(_("Type"), *type_renderer) - 1;
		Gtk::TreeViewColumn *type_column = m_wpttreeview->get_column(type_col);
                if (type_column)
                        type_column->add_attribute(*type_renderer, "text", m_wptfplancolumns.m_type);
		Gtk::CellRendererText *dist_renderer = Gtk::manage(new Gtk::CellRendererText());
		int dist_col = m_wpttreeview->append_column(_("Dist"), *dist_renderer) - 1;
		Gtk::TreeViewColumn *dist_column = m_wpttreeview->get_column(dist_col);
                if (dist_column)
                        dist_column->add_attribute(*dist_renderer, "text", m_wptfplancolumns.m_dist);
                alt_renderer->set_property("xalign", 1.0);
                freq_renderer->set_property("xalign", 1.0);
                dist_renderer->set_property("xalign", 1.0);
		const Gtk::TreeModelColumnBase *cols[] = { &m_wptfplancolumns.m_name,
							   &m_wptfplancolumns.m_icao,
							   &m_wptfplancolumns.m_altitude,
							   &m_wptfplancolumns.m_freq,
							   &m_wptfplancolumns.m_type,
							   &m_wptfplancolumns.m_dist };
		for (unsigned int i = 0; i < sizeof(cols)/sizeof(cols[0]); ++i) {
			Gtk::TreeViewColumn *col = m_wpttreeview->get_column(i);
			if (!col)
				continue;
			col->set_resizable(true);
			col->set_sort_column(*cols[i]);
			col->set_expand(i == 0);
		}
		m_wptstore->set_sort_func(m_wptfplancolumns.m_dist, sigc::mem_fun(*this, &FlightDeckWindow::wptpage_fplan_sortdist));
		m_wpttreeview->columns_autosize();
		m_wpttreeview->set_headers_clickable();
		Glib::RefPtr<Gtk::TreeSelection> selection(m_wpttreeview->get_selection());
		selection->set_mode(Gtk::SELECTION_SINGLE);
		m_wpttreeviewselconn = selection->signal_changed().connect(sigc::mem_fun(*this, &FlightDeckWindow::wptpage_fplan_selection_changed));
		//selection->set_select_function(sigc::mem_fun(*this, &FlightDeckWindow::wpt_select_function));
	}
	if (!m_wptstore)
		return;
	m_wptstore->clear();
	for (std::vector<FPlanWaypoint>::const_iterator i(elements.begin()), e(elements.end()); i != e; ++i) {
		const FPlanWaypoint& el(*i);
                Gtk::TreeModel::Row row(*(m_wptstore->append()));
		row[m_wptfplancolumns.m_element] = el;
		row[m_wptfplancolumns.m_icao] = el.get_icao();
		row[m_wptfplancolumns.m_name] = el.get_name();
		row[m_wptfplancolumns.m_type] = (el.get_flags() & FPlanWaypoint::altflag_ifr) ? "IFR" : "VFR";
		row[m_wptfplancolumns.m_altitude] = Conversions::alt_str(el.get_altitude(), el.get_flags());
		row[m_wptfplancolumns.m_freq] = Conversions::freq_str(el.get_frequency());
       }
	wptpage_updatedist(m_wptfplancolumns);
	if (m_wptstore) {
		if (m_wptpagemode == wptpagemode_text)
			m_wptstore->set_sort_column(m_wptfplancolumns.m_name, Gtk::SORT_ASCENDING);
		else
			m_wptstore->set_sort_column(m_wptfplancolumns.m_dist, Gtk::SORT_ASCENDING);
	}
	if (m_wpttreeview)
		m_wpttreeview->set_model(m_wptstore);
}

template <typename T> void FlightDeckWindow::wptpage_selection_changed(const T& cols)
{
	if (!m_wpttreeview) {
		wptpage_select();
		return;
	}
	Glib::RefPtr<Gtk::TreeSelection> selection(m_wpttreeview->get_selection());
	if (!selection) {
		wptpage_select();
		return;
	}
        Gtk::TreeModel::iterator iter(selection->get_selected());
        if (!iter) {
		wptpage_select();
		return;
	}
	Gtk::TreeModel::Row row(*iter);
	wptpage_select(row[cols.m_element]);
}

void FlightDeckWindow::wptpage_airport_selection_changed(void)
{
	if (!m_wpttreeview) {
		wptpage_select();
		return;
	}
	Glib::RefPtr<Gtk::TreeSelection> selection(m_wpttreeview->get_selection());
	if (!selection) {
		wptpage_select();
		return;
	}
        Gtk::TreeModel::iterator iter(selection->get_selected());
        if (!iter) {
		wptpage_select();
		return;
	}
	Gtk::TreeModel::Row row(*iter);
	WaypointAirportModelColumns::element_ptr_t elp(row[m_wptairportcolumns.m_element]);
	wptpage_select(*elp.operator->(), row[m_wptairportcolumns.m_routeid], row[m_wptairportcolumns.m_routepointid]);
}

void FlightDeckWindow::wptpage_navaid_selection_changed(void)
{
	wptpage_selection_changed(m_wptnavaidcolumns);
}

void FlightDeckWindow::wptpage_waypoint_selection_changed(void)
{
	wptpage_selection_changed(m_wptintersectioncolumns);
}

void FlightDeckWindow::wptpage_airway_selection_changed(void)
{
	wptpage_selection_changed(m_wptairwaycolumns);
}

void FlightDeckWindow::wptpage_airspace_selection_changed(void)
{
	wptpage_selection_changed(m_wptairspacecolumns);
}

void FlightDeckWindow::wptpage_mapelement_selection_changed(void)
{
	wptpage_selection_changed(m_wptmapelementcolumns);
}

void FlightDeckWindow::wptpage_fplan_selection_changed(void)
{
	wptpage_selection_changed(m_wptfplancolumns);
}

void FlightDeckWindow::wptpage_updatepos(const Point& pt)
{
	if (m_wptpagemode != wptpagemode_nearest)
		return;
	wptpage_setpos(pt);
}

void FlightDeckWindow::wptpage_setpos(const Point& pt)
{
	m_wptquerypoint = pt;
	switch (m_menuid) {
	case 100:
		wptpage_updatedist(m_wptairportcolumns);
		break;

	case 101:
		wptpage_updatedist(m_wptnavaidcolumns);
		break;

	case 102:
		wptpage_updatedist(m_wptintersectioncolumns);
		break;

	case 103:
		wptpage_updatedist(m_wptairwaycolumns);
		break;

	case 104:
		wptpage_updatedist(m_wptairspacecolumns);
		break;

	case 105:
		wptpage_updatedist(m_wptmapelementcolumns);
		break;

	case 106:
		wptpage_updatedist(m_wptfplancolumns);
		break;

	default:
		return;
	}
	if (m_wptqueryrect.is_inside(m_wptquerypoint))
		return;
	wptpage_find();
}

void FlightDeckWindow::wptpage_find(const Glib::ustring& text)
{
	if (m_wptpagemode == wptpagemode_text && m_wptquerytext == text)
		return;
	m_wptquerytext = text;
	if (!m_wptquerytext.empty()) {
		m_wptpagemode = wptpagemode_text;
		wptpage_find();
	} else {
		wptpage_select();
	}
}

void FlightDeckWindow::wptpage_findnearest(void)
{
	if (m_wptpagemode == wptpagemode_nearest)
		return;
	m_wptpagemode = wptpagemode_nearest;
	wptpage_find();
}

void FlightDeckWindow::wptpage_findcoord(const Point& pt)
{
	if (m_wptpagemode == wptpagemode_coord && m_wptquerypoint == pt)
		return;
	m_wptpagemode = wptpagemode_coord;
	m_wptquerypoint = pt;
	wptpage_find();
}

void FlightDeckWindow::wptpage_textentry_done(bool ok)
{
	if (!ok || !m_textentrydialog)
		return;
	wptpage_find(m_textentrydialog->get_text());
}

void FlightDeckWindow::wptpage_map_cursor_change(Point pt, VectorMapArea::cursor_validity_t valid)
{
	if (valid == VectorMapArea::cursor_invalid)
		return;
	wptpage_findcoord(pt);
}

void FlightDeckWindow::wptpage_updateswitches(void)
{
	m_wptpagebuttonnearestconn.block();
	m_wptpagetogglebuttonfindconn.block();
	if (m_wpttogglebuttonnearest)
		m_wpttogglebuttonnearest->set_active(m_wptpagemode == wptpagemode_nearest);
	if (m_wpttogglebuttonfind)
		m_wpttogglebuttonfind->set_active(m_wptpagemode == wptpagemode_text);
	m_wptpagebuttonnearestconn.unblock();
	m_wptpagetogglebuttonfindconn.unblock();
}

void FlightDeckWindow::wptpage_find(void)
{
	static const double search_box = 100;
	static const double valid_box = search_box * 0.1;
	static const unsigned int max_lines = 1000;

	wptpage_cancel();
	if (m_wptpagemode == wptpagemode_text && m_wptquerytext.empty())
		m_wptpagemode = wptpagemode_nearest;
	wptpage_updateswitches();
	if (m_wptpagemode == wptpagemode_nearest && m_sensors)
		m_sensors->get_position(m_wptquerypoint);
	wptpage_select();
	if (m_menuid < 100 || m_menuid > 106 || !m_engine) {
		m_wpttreeviewselconn.disconnect();
		m_wptstore.reset();
		if (m_wpttreeview) {
			m_wpttreeview->remove_all_columns();
			m_wpttreeview->unset_model();
		}
		return;
	}
	if (m_wptpagemode == wptpagemode_text) {
		m_wptqueryrect = Rect(Point(0, Point::pole_lat), Point(0, Point::pole_lat));
		switch (m_menuid) {
		case 100:
			m_airportquery = m_engine->async_airport_find_by_text(m_wptquerytext, max_lines, 0, AirportsDb::comp_contains,
									      AirportsDb::element_t::subtables_vfrroutes | AirportsDb::element_t::subtables_runways |
									      AirportsDb::element_t::subtables_helipads | AirportsDb::element_t::subtables_comms |
									      AirportsDb::element_t::subtables_fas);
			if (m_airportquery)
				m_airportquery->connect(sigc::mem_fun(*this, &FlightDeckWindow::wptpage_asyncdone));
			break;

		case 101:
			m_navaidquery = m_engine->async_navaid_find_by_text(m_wptquerytext, max_lines, 0, NavaidsDb::comp_contains, NavaidsDb::element_t::subtables_none);
			if (m_navaidquery)
				m_navaidquery->connect(sigc::mem_fun(*this, &FlightDeckWindow::wptpage_asyncdone));
			break;

		case 102:
			m_waypointquery = m_engine->async_waypoint_find_by_name(m_wptquerytext, max_lines, 0, WaypointsDb::comp_contains, WaypointsDb::element_t::subtables_none);
			if (m_waypointquery)
				m_waypointquery->connect(sigc::mem_fun(*this, &FlightDeckWindow::wptpage_asyncdone));
			break;

		case 103:
			m_airwayquery = m_engine->async_airway_find_by_text(m_wptquerytext, max_lines, 0, AirwaysDb::comp_contains, AirwaysDb::element_t::subtables_none);
			if (m_airwayquery)
				m_airwayquery->connect(sigc::mem_fun(*this, &FlightDeckWindow::wptpage_asyncdone));
			break;

		case 104:
			m_airspacequery = m_engine->async_airspace_find_by_name(m_wptquerytext, max_lines, 0, AirspacesDb::comp_contains, AirspacesDb::element_t::subtables_none);
			if (m_airspacequery)
				m_airspacequery->connect(sigc::mem_fun(*this, &FlightDeckWindow::wptpage_asyncdone));
			break;

		case 105:
			m_mapelementquery = m_engine->async_mapelement_find_by_name(m_wptquerytext, max_lines, 0, MapelementsDb::comp_contains, MapelementsDb::element_t::subtables_none);
			if (m_mapelementquery)
				m_mapelementquery->connect(sigc::mem_fun(*this, &FlightDeckWindow::wptpage_asyncdone));
			break;

		case 106:
		{
			std::vector<FPlanWaypoint> wpts1;
			{
				std::set<FPlanWaypoint, WptFPlanCompare> wpts;
				{
					std::vector<FPlanWaypoint> wpts1(m_engine->get_fplan_db().find_by_name(m_wptquerytext, ~0U, 0, FPlan::comp_contains));
					for (std::vector<FPlanWaypoint>::const_iterator wi(wpts1.begin()), we(wpts1.end()); wi != we; ++wi)
						wpts.insert(*wi);
				}
				{
					std::vector<FPlanWaypoint> wpts1(m_engine->get_fplan_db().find_by_icao(m_wptquerytext, ~0U, 0, FPlan::comp_contains));
					for (std::vector<FPlanWaypoint>::const_iterator wi(wpts1.begin()), we(wpts1.end()); wi != we; ++wi)
						wpts.insert(*wi);
				}
				std::set<FPlanWaypoint, WptFPlanCompare>::const_iterator wi(wpts.begin()), we(wpts.end());
				for (unsigned int count = 0; count < max_lines && wi != we; ++count, ++wi)
					wpts1.push_back(*wi);
			}
		        wptpage_setresult(wpts1);
			break;
		}

		default:
			break;
		}
		return;
	}
	switch (m_menuid) {
	case 100:
		m_wptqueryrect = m_wptquerypoint.simple_box_nmi(valid_box);
		m_airportquery = m_engine->async_airport_find_nearest(m_wptquerypoint, max_lines, m_wptquerypoint.simple_box_nmi(search_box),
								      AirportsDb::element_t::subtables_vfrroutes | AirportsDb::element_t::subtables_runways |
								      AirportsDb::element_t::subtables_helipads | AirportsDb::element_t::subtables_comms |
								      AirportsDb::element_t::subtables_fas);
		if (m_airportquery)
			m_airportquery->connect(sigc::mem_fun(*this, &FlightDeckWindow::wptpage_asyncdone));
		break;

	case 101:
		m_wptqueryrect = m_wptquerypoint.simple_box_nmi(valid_box);
		m_navaidquery = m_engine->async_navaid_find_nearest(m_wptquerypoint, max_lines, m_wptquerypoint.simple_box_nmi(search_box),
								    NavaidsDb::element_t::subtables_none);
		if (m_navaidquery)
			m_navaidquery->connect(sigc::mem_fun(*this, &FlightDeckWindow::wptpage_asyncdone));
		break;

	case 102:
		m_wptqueryrect = m_wptquerypoint.simple_box_nmi(valid_box);
		m_waypointquery = m_engine->async_waypoint_find_nearest(m_wptquerypoint, max_lines, m_wptquerypoint.simple_box_nmi(search_box),
									WaypointsDb::element_t::subtables_none);
		if (m_waypointquery)
			m_waypointquery->connect(sigc::mem_fun(*this, &FlightDeckWindow::wptpage_asyncdone));
		break;

	case 103:
		m_wptqueryrect = m_wptquerypoint.simple_box_nmi(valid_box);
		m_airwayquery = m_engine->async_airway_find_nearest(m_wptquerypoint, max_lines, m_wptquerypoint.simple_box_nmi(search_box),
								    AirwaysDb::element_t::subtables_none);
		if (m_airwayquery)
			m_airwayquery->connect(sigc::mem_fun(*this, &FlightDeckWindow::wptpage_asyncdone));
		break;

	case 104:
		m_wptqueryrect = m_wptquerypoint.simple_box_nmi(valid_box);
		m_airspacequery = m_engine->async_airspace_find_nearest(m_wptquerypoint, max_lines, m_wptquerypoint.simple_box_nmi(search_box),
									AirspacesDb::element_t::subtables_none);
		if (m_airspacequery)
			m_airspacequery->connect(sigc::mem_fun(*this, &FlightDeckWindow::wptpage_asyncdone));
		break;

	case 105:
		m_wptqueryrect = m_wptquerypoint.simple_box_nmi(valid_box);
		m_mapelementquery = m_engine->async_mapelement_find_nearest(m_wptquerypoint, max_lines, m_wptquerypoint.simple_box_nmi(search_box),
									    MapelementsDb::element_t::subtables_none);
		if (m_mapelementquery)
			m_mapelementquery->connect(sigc::mem_fun(*this, &FlightDeckWindow::wptpage_asyncdone));
		break;

	case 106:
	{
		std::vector<FPlanWaypoint> wpts1;
		{
			std::map<uint64_t, FPlanWaypoint> wpts;
			{
				std::set<FPlanWaypoint, WptFPlanCompare> wpts2;
				{
					std::vector<FPlanWaypoint> wpts1(m_engine->get_fplan_db().find_nearest(m_wptquerypoint, ~0U, 0, m_wptquerypoint.simple_box_nmi(search_box)));
					for (std::vector<FPlanWaypoint>::const_iterator wi(wpts1.begin()), we(wpts1.end()); wi != we; ++wi)
						wpts2.insert(*wi);
				}
				for (std::set<FPlanWaypoint, WptFPlanCompare>::const_iterator wi(wpts2.begin()), we(wpts2.end()); wi != we; ++wi)
					wpts.insert(std::make_pair(m_wptquerypoint.simple_distance_rel(wi->get_coord()), *wi));
			}
			std::map<uint64_t, FPlanWaypoint>::const_iterator wi(wpts.begin()), we(wpts.end());
			for (unsigned int count = 0; count < max_lines && wi != we; ++count, ++wi)
				wpts1.push_back(wi->second);
		}
		wptpage_setresult(wpts1);
		break;
	}

	default:
		break;
	}
	return;
}

void FlightDeckWindow::wptpage_updatemenu(void)
{
	static const char * const wptframelabels[] = {
		"<b>Waypoint: Airport</b>",
		"<b>Waypoint: Navigation Aid</b>",
		"<b>Waypoint: Intersection</b>",
		"<b>Waypoint: Airway</b>",
		"<b>Waypoint: Airspace</b>",
		"<b>Waypoint: Map</b>",
		"<b>Waypoint: Flight Plan Waypoints</b>"
	};
	wptpage_select();
	if (m_menuid < 100 || m_menuid > 106) {
		wptpage_clearall();
		m_wpttreeviewselconn.disconnect();
		m_wptstore.reset();
		if (m_wpttreeview) {
			m_wpttreeview->remove_all_columns();
			m_wpttreeview->unset_model();
		}
		if (m_wptdrawingarea) {
			m_wptdrawingarea->set_renderer(VectorMapArea::renderer_none);
			m_wptdrawingarea->hide();
		}
		return;
	}
	if (m_wptframelabel)
		m_wptframelabel->set_markup(wptframelabels[m_menuid - 100]);
	if (m_wptdrawingarea) {
		m_wptdrawingarea->set_renderer(VectorMapArea::renderer_vmap);
		*m_wptdrawingarea = m_navarea.map_get_drawflags();
		if (m_sensors && m_sensors->get_configfile().has_group(cfggroup_mainwindow)) {
			if (false && m_sensors->get_configfile().has_key(cfggroup_mainwindow, NavArea::cfgkey_mainwindow_mapflags))
				*m_wptdrawingarea = (MapRenderer::DrawFlags)m_sensors->get_configfile().get_integer(cfggroup_mainwindow, NavArea::cfgkey_mainwindow_mapflags);
			if (m_sensors->get_configfile().has_key(cfggroup_mainwindow, NavArea::cfgkey_mainwindow_mapscale))
				m_wptdrawingarea->set_map_scale(m_sensors->get_configfile().get_double(cfggroup_mainwindow, NavArea::cfgkey_mainwindow_mapscale));
		}
		m_wptdrawingarea->show();
	}
	m_wpttogglebuttonarptmap.block();
	if (m_wpttogglebuttonmap)
		m_wpttogglebuttonmap->set_active(false);
	m_wpttogglebuttonarptmap.unblock();
	wptpage_find();
}

void FlightDeckWindow::wptpage_togglebuttonnearest(void)
{
	if (m_wptpagemode == wptpagemode_nearest) {
		m_wptpagemode = wptpagemode_coord;
		wptpage_updateswitches();
	} else {
		m_wptpagemode = wptpagemode_nearest;
		wptpage_find();
	}
}

void FlightDeckWindow::wptpage_togglebuttonfind(void)
{
	if (m_wptpagemode == wptpagemode_text)
		m_wptpagemode = wptpagemode_coord;
	else
		textentry_dialog_open("Enter Waypoint Search String:", m_wptquerytext, sigc::mem_fun(*this, &FlightDeckWindow::wptpage_textentry_done));
	wptpage_updateswitches();
}

bool FlightDeckWindow::wptpage_getcoord(Glib::ustring& icao, Glib::ustring& name, Point& coord, double& track, int& altitude)
{
	icao.clear();
	name.clear();
	coord = Point();
	track = std::numeric_limits<double>::quiet_NaN();
	altitude = std::numeric_limits<int>::min();
	if (!m_wpttreeview)
		return false;
	Glib::RefPtr<Gtk::TreeSelection> selection(m_wpttreeview->get_selection());
	if (!selection)
		return false;
        Gtk::TreeModel::iterator iter(selection->get_selected());
        if (!iter)
		return false;
	Gtk::TreeModel::Row row(*iter);
	switch (m_menuid) {
	case 100:
	{
		WaypointAirportModelColumns::element_ptr_t el(row[m_wptairportcolumns.m_element]);
		unsigned int routenr(row[m_wptairportcolumns.m_routeid]);
		unsigned int routepointnr(row[m_wptairportcolumns.m_routepointid]);
		if (routenr >= el->get_nrvfrrte()) {
			icao = el->get_icao();
			name = el->get_name();
			coord = el->get_coord();
			return true;
		}
		const AirportsDb::Airport::VFRRoute& vfrrte(el->get_vfrrte(routenr));
		if (routepointnr >= vfrrte.size())
			return false;
		const AirportsDb::Airport::VFRRoute::VFRRoutePoint& vfrpt(vfrrte[routepointnr]);
		icao = el->get_icao();
		name = vfrpt.get_name();
		coord = vfrpt.get_coord();
		altitude = vfrpt.get_altitude();
		if (!routepointnr)
			return true;
		const AirportsDb::Airport::VFRRoute::VFRRoutePoint& vfrptp(vfrrte[routepointnr - 1]);
		track = vfrpt.get_coord().spheric_true_course(vfrptp.get_coord());
		return true;
	}

	case 101:
	{
		const NavaidsDb::Navaid& el(row[m_wptnavaidcolumns.m_element]);
		icao = el.get_icao();
		name = el.get_name();
		coord = el.get_coord();
		return true;
	}

	case 102:
	{
		const WaypointsDb::Waypoint& el(row[m_wptintersectioncolumns.m_element]);
		icao = el.get_icao();
		name = el.get_name();
		coord = el.get_coord();
		return true;
	}

	case 103:
	{
	        const AirwaysDb::Airway& el(row[m_wptairwaycolumns.m_element]);
		icao = el.get_name();
		name = el.get_begin_name();
		coord = el.get_begin_coord();
		return true;
	}

	case 105:
	{
	        const MapelementsDb::Mapelement& el(row[m_wptmapelementcolumns.m_element]);
		name = el.get_name();
		coord = el.get_coord();
		return true;
	}

	case 106:
	{
	        const FPlanWaypoint& el(row[m_wptfplancolumns.m_element]);
		icao = el.get_icao();
		name = el.get_name();
		coord = el.get_coord();
		altitude = el.get_altitude();
		return true;
	}

	default:
		return false;
	}
	return false;
}

void FlightDeckWindow::wptpage_setwpt(FPlanWaypoint& wpt, const Cairo::RefPtr<AirportsDb::Airport> el)
{
	wpt.set(*el.operator->());
}

void FlightDeckWindow::wptpage_setwpt(FPlanWaypoint& wpt, const AirwaysDb::Airway& el, bool end)
{
	wpt.set_pathcode(FPlanWaypoint::pathcode_airway);
	wpt.set_pathname(el.get_name());
	if (end) {
		wpt.set_name(el.get_end_name());
		wpt.set_coord(el.get_end_coord());
	} else {
		wpt.set_name(el.get_begin_name());
		wpt.set_coord(el.get_begin_coord());
	}
	Glib::ustring note(el.get_type_name() + " Base FL" + Glib::ustring::format(el.get_base_level()) + " Top FL" + Glib::ustring::format(el.get_top_level()));
	wpt.set_note(note);
}

void FlightDeckWindow::wptpage_buttoninsert(void)
{
	if (!m_sensors)
		return;
	FPlanRoute& fpl(m_sensors->get_route());
        unsigned int cur(fpl.get_curwpt());
        unsigned int nrwpt(fpl.get_nrwpt());
	if (!nrwpt)
		cur = 0;
	FPlanWaypoint wpt;
	{
		time_t t(time(0));
		wpt.set_time_unix(t);
                wpt.set_save_time_unix(t);
	}
	wpt.set_flags(0);
	wpt.set_altitude(0);
	if (cur >= 1 && cur <= nrwpt) {
		wpt.set_flags(fpl[cur - 1].get_flags() & (FPlanWaypoint::altflag_ifr | FPlanWaypoint::altflag_standard));
		wpt.set_altitude(fpl[cur - 1].get_altitude());
	}
	if (!m_wpttreeview)
		return;
	Glib::RefPtr<Gtk::TreeSelection> selection(m_wpttreeview->get_selection());
	if (!selection)
		return;
        Gtk::TreeModel::iterator iter(selection->get_selected());
        if (!iter)
		return;
	Gtk::TreeModel::Row row(*iter);
	switch (m_menuid) {
	case 100:
	{
		WaypointAirportModelColumns::element_ptr_t el(row[m_wptairportcolumns.m_element]);
		unsigned int routenr(row[m_wptairportcolumns.m_routeid]);
		unsigned int routepointnr(row[m_wptairportcolumns.m_routepointid]);
		if (routenr >= el->get_nrvfrrte()) {
		        wptpage_setwpt(wpt, el);
		} else {
			const AirportsDb::Airport::VFRRoute& vfrrte(el->get_vfrrte(routenr));
			if (routepointnr >= vfrrte.size()) {
				if (!vfrrte.size()) {
					wptpage_setwpt(wpt, el);
					wpt.set_pathcode(FPlanWaypoint::pathcode_vfrtransition);
					wpt.set_pathname(vfrrte.get_name());
					break;
				}
				for (unsigned int nr(0);;) {
					const AirportsDb::Airport::VFRRoute::VFRRoutePoint& vfrpt(vfrrte[nr]);
					if (vfrpt.is_at_airport()) {
						wptpage_setwpt(wpt, el);
					} else {
						//wpt.set_icao(el->get_icao());
						wpt.set_icao("");
						wpt.set_name(vfrpt.get_name());
						wpt.set_coord(vfrpt.get_coord());
						wpt.set_altitude(vfrpt.get_altitude());
						wpt.set_frequency(0);
						Glib::ustring note(vfrpt.get_altcode_string());
						if (vfrpt.is_compulsory())
							note += " Compulsory";
						note += "\n" + vfrpt.get_pathcode_string();
						wpt.set_note(note);
					}
					wpt.set_pathcode(vfrpt.get_fplan_pathcode());
					wpt.set_pathname(vfrrte.get_name());
					++nr;
					if (nr >= vfrrte.size())
						break;
					m_sensors->nav_insert_wpt(cur, wpt);
					++nrwpt;
					cur = std::min(cur + 1U, nrwpt);
				}
			} else {
				const AirportsDb::Airport::VFRRoute::VFRRoutePoint& vfrpt(vfrrte[routepointnr]);
				if (vfrpt.is_at_airport()) {
					wptpage_setwpt(wpt, el);
				} else {
					//wpt.set_icao(el->get_icao());
					wpt.set_icao("");
					wpt.set_name(vfrpt.get_name());
					wpt.set_pathcode(FPlanWaypoint::pathcode_vfrtransition);
					wpt.set_pathname(vfrrte.get_name());
					wpt.set_coord(vfrpt.get_coord());
					wpt.set_altitude(vfrpt.get_altitude());
					wpt.set_frequency(0);
					Glib::ustring note(vfrpt.get_altcode_string());
					if (vfrpt.is_compulsory())
						note += " Compulsory";
					note += "\n" + vfrpt.get_pathcode_string();
					wpt.set_note(note);
				}
			}
		}
		break;
	}

	case 101:
		wpt.set(row[m_wptnavaidcolumns.m_element]);
		break;

	case 102:
		wpt.set(row[m_wptintersectioncolumns.m_element]);
		break;

	case 103:
		wptpage_setwpt(wpt, row[m_wptairwaycolumns.m_element], false);
		m_sensors->nav_insert_wpt(cur, wpt);
		++nrwpt;
		cur = std::min(cur + 1U, nrwpt);
		wptpage_setwpt(wpt, row[m_wptairwaycolumns.m_element], true);
		break;

	case 105:
		wpt.set(row[m_wptmapelementcolumns.m_element]);
		break;

	case 106:
		wpt.set(row[m_wptfplancolumns.m_element]);
		break;

	default:
		return;
	}
	m_sensors->nav_insert_wpt(cur, wpt);
	++nrwpt;
	cur = std::min(cur + 1U, nrwpt);
	m_sensors->nav_fplan_modified();
	set_menu_id(200);
}

void FlightDeckWindow::wptpage_buttondirectto(void)
{
	if (m_wptnotebook && m_wptarptnotebook && m_wptnotebook->get_current_page() == 0) {
		WaypointAirportModelColumns::element_ptr_t arpt;
		if (m_wpttreeview) {
			Glib::RefPtr<Gtk::TreeSelection> selection(m_wpttreeview->get_selection());
			if (selection) {
				Gtk::TreeModel::iterator iter(selection->get_selected());
				if (iter) {
					Gtk::TreeModel::Row row(*iter);
					arpt = row[m_wptairportcolumns.m_element];
				}
			}
		}
		if (arpt && m_wptarptnotebook->get_current_page() == 1 && m_wptarptrwytreeview) {
			Glib::RefPtr<Gtk::TreeSelection> selection(m_wptarptrwytreeview->get_selection());
			if (selection) {
				Gtk::TreeModel::iterator iter(selection->get_selected());
				if (iter) {
					Gtk::TreeModel::Row row(*iter);
					for (unsigned int i = 0; i < arpt->get_nrrwy(); ++i) {
						const AirportsDb::Airport::Runway& rwy(arpt->get_rwy(i));
						if (rwy.get_ident_he() == row[m_wptairportrunwaycolumns.m_ident]) {
							AirportsDb::Airport::FASNavigate fas(rwy.get_he_fake_fas());
							if (fas.is_valid()) {
								fas.set_airport(arpt->get_icao());
								if (m_sensors) {
									m_sensors->nav_finalapproach(fas);
									set_menu_id(0);
									return;
								}
							}
						}
						if (rwy.get_ident_le() == row[m_wptairportrunwaycolumns.m_ident]) {
							AirportsDb::Airport::FASNavigate fas(rwy.get_le_fake_fas());
							if (fas.is_valid()) {
								fas.set_airport(arpt->get_icao());
								if (m_sensors) {
									m_sensors->nav_finalapproach(fas);
									set_menu_id(0);
									return;
								}
							}
						}
					}
				}
			}
		}
		if (arpt && m_wptarptnotebook->get_current_page() == 4 && m_wptarptfastreeview) {
			Glib::RefPtr<Gtk::TreeSelection> selection(m_wptarptfastreeview->get_selection());
			if (selection) {
				Gtk::TreeModel::iterator iter(selection->get_selected());
				if (iter) {
					Gtk::TreeModel::Row row(*iter);
					for (unsigned int i = 0; i < arpt->get_nrfinalapproachsegments(); ++i) {
						const AirportsDb::Airport::MinimalFAS& mfas(arpt->get_finalapproachsegment(i));
						if (!mfas.is_valid() || mfas.get_referencepathident() != row[m_wptairportfascolumns.m_ident])
							continue;
						AirportsDb::Airport::FASNavigate fas(mfas.navigate());
						if (!fas.is_valid())
							continue;
						fas.set_airport(arpt->get_icao());
						if (m_sensors) {
							m_sensors->nav_finalapproach(fas);
							set_menu_id(0);
							return;
						}
					}
				}
			}
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
	Gtk::TreeModel::iterator iter;
	if (m_wpttreeview) {
		Glib::RefPtr<Gtk::TreeSelection> selection(m_wpttreeview->get_selection());
		if (selection)
			iter = selection->get_selected();
	}
	if (iter) {
		Gtk::TreeModel::Row row(*iter);
		switch (m_menuid) {
		case 100:
		{
			WaypointAirportModelColumns::element_ptr_t el(row[m_wptairportcolumns.m_element]);
			unsigned int routenr(row[m_wptairportcolumns.m_routeid]);
			unsigned int routepointnr(row[m_wptairportcolumns.m_routepointid]);
			if (routenr >= el->get_nrvfrrte()) {
				m_coorddialog->directto(el->get_coord(), el->get_name(), el->get_icao());
				break;
			}
			const AirportsDb::Airport::VFRRoute& vfrrte(el->get_vfrrte(routenr));
			if (routepointnr >= vfrrte.size())
				break;
			const AirportsDb::Airport::VFRRoute::VFRRoutePoint& vfrpt(vfrrte[routepointnr]);
			if (vfrpt.is_at_airport()) {
				m_coorddialog->directto(el->get_coord(), el->get_name(), el->get_icao());
				break;
			}
			m_coorddialog->directto(vfrpt.get_coord(), vfrpt.get_name());
			break;
		}

		case 101:
		{
			const NavaidsDb::Navaid& el(row[m_wptnavaidcolumns.m_element]);
			m_coorddialog->directto(el.get_coord(), el.get_name(), el.get_icao());
			break;
		}

		case 102:
		{
			const WaypointsDb::Waypoint& el(row[m_wptintersectioncolumns.m_element]);
			m_coorddialog->directto(el.get_coord(), el.get_name(), el.get_icao());
			break;
		}

		case 103:
		{
			const AirwaysDb::Airway& el(row[m_wptairwaycolumns.m_element]);
			Point pt;
			m_sensors->get_position(pt);
			if (pt.simple_distance_rel(el.get_begin_coord()) <
			    pt.simple_distance_rel(el.get_end_coord())) {
				m_coorddialog->directto(el.get_begin_coord(), el.get_begin_name());
			} else {
				m_coorddialog->directto(el.get_end_coord(), el.get_end_name());
			}
			break;
		}

		case 105:
		{
			const MapelementsDb::Mapelement& el(row[m_wptmapelementcolumns.m_element]);
			m_coorddialog->directto(el.get_coord(), el.get_name());
			break;
		}

		case 106:
		{
			const FPlanWaypoint& el(row[m_wptfplancolumns.m_element]);
			m_coorddialog->directto(el.get_coord(), el.get_name(), el.get_icao());
			break;
		}

		default:
			break;
		}
	}
}

void FlightDeckWindow::wptpage_togglebuttonbrg2(void)
{
	if (!m_wpttogglebuttonbrg2 || !m_sensors)
		return;
	bool ena(m_wpttogglebuttonbrg2->get_active());
	if (ena) {
		Gtk::TreeModel::iterator iter;
		if (m_wpttreeview) {
			Glib::RefPtr<Gtk::TreeSelection> selection(m_wpttreeview->get_selection());
			if (selection)
				iter = selection->get_selected();
		}
		if (iter) {
			Gtk::TreeModel::Row row(*iter);
			switch (m_menuid) {
			case 100:
			{
				WaypointAirportModelColumns::element_ptr_t el(row[m_wptairportcolumns.m_element]);
				unsigned int routenr(row[m_wptairportcolumns.m_routeid]);
				unsigned int routepointnr(row[m_wptairportcolumns.m_routepointid]);
				if (routenr >= el->get_nrvfrrte()) {
					m_sensors->nav_set_brg2(el->get_icao_name());
					m_sensors->nav_set_brg2(el->get_coord());
					break;
				}
				const AirportsDb::Airport::VFRRoute& vfrrte(el->get_vfrrte(routenr));
				if (routepointnr >= vfrrte.size())
					break;
				const AirportsDb::Airport::VFRRoute::VFRRoutePoint& vfrpt(vfrrte[routepointnr]);
				if (vfrpt.is_at_airport()) {
					m_sensors->nav_set_brg2(el->get_icao_name());
					m_sensors->nav_set_brg2(el->get_coord());
					break;
				}
				m_sensors->nav_set_brg2(vfrpt.get_name());
				m_sensors->nav_set_brg2(vfrpt.get_coord());
				break;
			}

			case 101:
			{
				const NavaidsDb::Navaid& el(row[m_wptnavaidcolumns.m_element]);
				m_sensors->nav_set_brg2(el.get_icao_name());
				m_sensors->nav_set_brg2(el.get_coord());
				break;
			}

			case 102:
			{
				const WaypointsDb::Waypoint& el(row[m_wptintersectioncolumns.m_element]);
				m_sensors->nav_set_brg2(el.get_icao_name());
				m_sensors->nav_set_brg2(el.get_coord());
				break;
			}

			case 103:
			{
				const AirwaysDb::Airway& el(row[m_wptairwaycolumns.m_element]);
				Point pt;
				m_sensors->get_position(pt);
				if (pt.simple_distance_rel(el.get_begin_coord()) <
				    pt.simple_distance_rel(el.get_end_coord())) {
					m_sensors->nav_set_brg2(el.get_begin_name());
					m_sensors->nav_set_brg2(el.get_begin_coord());
				} else {
					m_sensors->nav_set_brg2(el.get_end_name());
					m_sensors->nav_set_brg2(el.get_end_coord());
				}
				break;
			}

			case 105:
			{
				const MapelementsDb::Mapelement& el(row[m_wptmapelementcolumns.m_element]);
				m_sensors->nav_set_brg2(el.get_name());
				m_sensors->nav_set_brg2(el.get_coord());
				break;
			}

			case 106:
			{
				const FPlanWaypoint& el(row[m_wptfplancolumns.m_element]);
				m_sensors->nav_set_brg2(el.get_icao_name());
				m_sensors->nav_set_brg2(el.get_coord());
				break;
			}

			default:
				break;
			}
		}
	}
	m_sensors->nav_set_brg2(ena);
}

void FlightDeckWindow::wptpage_togglebuttonarptmap(void)
{
	if (!m_wptdrawingarea)
		return;
	switch (m_wptdrawingarea->get_renderer()) {
	case VectorMapArea::renderer_none:
		m_wpttogglebuttonarptmap.block();
		if (m_wpttogglebuttonmap)
			m_wpttogglebuttonmap->set_inconsistent(true);
		m_wpttogglebuttonarptmap.unblock();
		break;

	case VectorMapArea::renderer_airportdiagram:
		m_wptdrawingarea->set_renderer(VectorMapArea::renderer_vmap);
		if (m_sensors && m_sensors->get_configfile().has_group(cfggroup_mainwindow)) {
			if (m_sensors->get_configfile().has_key(cfggroup_mainwindow, NavArea::cfgkey_mainwindow_mapscale))
				m_wptdrawingarea->set_map_scale(m_sensors->get_configfile().get_double(cfggroup_mainwindow, NavArea::cfgkey_mainwindow_mapscale));
		}
		m_wpttogglebuttonarptmap.block();
		if (m_wpttogglebuttonmap)
			m_wpttogglebuttonmap->set_active(false);
		m_wpttogglebuttonarptmap.unblock();
		break;

	default:
		m_wptdrawingarea->set_renderer(VectorMapArea::renderer_airportdiagram);
		if (m_sensors && m_sensors->get_configfile().has_group(cfggroup_mainwindow)) {
			if (m_sensors->get_configfile().has_key(cfggroup_mainwindow, NavArea::cfgkey_mainwindow_arptmapscale))
				m_wptdrawingarea->set_map_scale(m_sensors->get_configfile().get_double(cfggroup_mainwindow, NavArea::cfgkey_mainwindow_arptmapscale));
		}
		m_wpttogglebuttonarptmap.block();
		if (m_wpttogglebuttonmap)
			m_wpttogglebuttonmap->set_active(true);
		m_wpttogglebuttonarptmap.unblock();
		break;
	}
}
