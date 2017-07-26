//
// C++ Implementation: RouteEditUi
//
// Description: The main flight plan entry GUI
//
//
// Author: Thomas Sailer <t.sailer@alumni.ethz.ch>, (C) 2007, 2008, 2009, 2012, 2013
//
// Copyright: See COPYING file that comes with this distribution
//
//

#include "sysdeps.h"

#include <sstream>
#include <iomanip>
#include <cstring>

#include "RouteEditUi.h"
#include "wmm.h"
#include "SunriseSunset.h"
#include "baro.h"
#include "icaofpl.h"
#include "sysdepsgui.h"

RouteEditUi::FullscreenableWindow::FullscreenableWindow(BaseObjectType * castitem, const Glib::RefPtr<Builder>& refxml)
        : toplevel_window_t(castitem), m_fullscreen(false)
{
        signal_window_state_event().connect(sigc::mem_fun(*this, &RouteEditUi::FullscreenableWindow::on_my_window_state_event));
        signal_key_press_event().connect(sigc::mem_fun(*this, &RouteEditUi::FullscreenableWindow::on_my_key_press_event));
}

RouteEditUi::FullscreenableWindow::~FullscreenableWindow()
{
}

bool RouteEditUi::FullscreenableWindow::on_my_window_state_event(GdkEventWindowState *state)
{
        if (!state)
                return false;
        m_fullscreen = !!(Gdk::WINDOW_STATE_FULLSCREEN & state->new_window_state);
        return false;
}

void RouteEditUi::FullscreenableWindow::toggle_fullscreen(void)
{
        if (m_fullscreen)
                unfullscreen();
        else
                fullscreen();
}

bool RouteEditUi::FullscreenableWindow::on_my_key_press_event(GdkEventKey * event)
{
        if(!event)
                return false;
        //std::cerr << "key press: type " << event->type << " keyval " << event->keyval << std::endl;
        if (event->type != GDK_KEY_PRESS)
                return false;
        switch (event->keyval) {
	case GDK_KEY_F6:
		toggle_fullscreen();
		return true;

	default:
		break;
        }
        return false;
}

RouteEditUi::RouteEditWindow::RouteEditWindow(BaseObjectType *castitem, const Glib::RefPtr<Builder>& refxml)
        : FullscreenableWindow(castitem, refxml)
{
        // Append Menu Item Callbacks
        Gtk::MenuItem *routeedit_menu_cut;
        get_widget(refxml, "routeedit_menu_cut", routeedit_menu_cut);
        routeedit_menu_cut->signal_activate().connect(sigc::mem_fun(*this, &RouteEditWindow::menu_cut));
        Gtk::MenuItem *routeedit_menu_copy;
        get_widget(refxml, "routeedit_menu_copy", routeedit_menu_copy);
        routeedit_menu_copy->signal_activate().connect(sigc::mem_fun(*this, &RouteEditWindow::menu_copy));
        Gtk::MenuItem *routeedit_menu_paste;
        get_widget(refxml, "routeedit_menu_paste", routeedit_menu_paste);
        routeedit_menu_paste->signal_activate().connect(sigc::mem_fun(*this, &RouteEditWindow::menu_paste));
        Gtk::MenuItem *routeedit_menu_delete;
        get_widget(refxml, "routeedit_menu_delete", routeedit_menu_delete);
        routeedit_menu_delete->signal_activate().connect(sigc::mem_fun(*this, &RouteEditWindow::menu_delete));
        signal_delete_event().connect(sigc::mem_fun(*this, &RouteEditUi::RouteEditWindow::on_my_delete_event));
}

RouteEditUi::RouteEditWindow::~RouteEditWindow()
{
}

void RouteEditUi::RouteEditWindow::menu_cut(void)
{
        Gtk::Widget *focus = get_focus();
        if (!focus)
                return;
        Gtk::Editable *edit = dynamic_cast<Gtk::Editable *>(focus);
        if (!edit)
                return;
        edit->cut_clipboard();
}

void RouteEditUi::RouteEditWindow::menu_copy(void)
{
        Gtk::Widget *focus = get_focus();
        if (!focus)
                return;
        Gtk::Editable *edit = dynamic_cast<Gtk::Editable *>(focus);
        if (!edit)
                return;
        edit->copy_clipboard();
}

void RouteEditUi::RouteEditWindow::menu_paste(void)
{
        Gtk::Widget *focus = get_focus();
        if (!focus)
                return;
        Gtk::Editable *edit = dynamic_cast<Gtk::Editable *>(focus);
        if (!edit)
                return;
        edit->paste_clipboard();
}

void RouteEditUi::RouteEditWindow::menu_delete(void)
{
        Gtk::Widget *focus = get_focus();
        if (!focus)
                return;
        Gtk::Editable *edit = dynamic_cast<Gtk::Editable *>(focus);
        if (!edit)
                return;
        edit->delete_selection();
}

bool RouteEditUi::RouteEditWindow::on_my_delete_event(GdkEventAny * event)
{
        hide();
        return true;
}

RouteEditUi::FindWindow::FindWindow(BaseObjectType * castitem, const Glib::RefPtr<Builder>& refxml)
        : FullscreenableWindow(castitem, refxml), m_refxml(refxml), m_engine(0), m_treeview(0), m_buttonok(0)
{
        signal_hide().connect(sigc::mem_fun(*this, &RouteEditUi::FindWindow::on_my_hide));
        signal_delete_event().connect(sigc::mem_fun(*this, &RouteEditUi::FindWindow::on_my_delete_event));
        m_querydispatch.connect(sigc::mem_fun(*this, &RouteEditUi::FindWindow::async_done));
}

RouteEditUi::FindWindow::~FindWindow()
{
        async_cancel();
}

void RouteEditUi::FindWindow::set_engine(Engine & eng)
{
        m_engine = &eng;
}

void RouteEditUi::FindWindow::set_center(const Point & pt)
{
        m_center = pt;
}

void RouteEditUi::FindWindow::init_list(void)
{
        // Create Tree Model
        m_findstore = Gtk::TreeStore::create(m_findcolumns);
        m_treeview->set_model(m_findstore);
        Gtk::CellRendererText *display_renderer = Gtk::manage(new Gtk::CellRendererText());
        int display_col = m_treeview->append_column(_("Waypoint"), *display_renderer) - 1;
        Gtk::CellRendererText *dist_renderer = Gtk::manage(new Gtk::CellRendererText());
        int dist_col = m_treeview->append_column(_("Dist"), *dist_renderer) - 1;
        {
                Gtk::TreeViewColumn *display_column = m_treeview->get_column(display_col);
                if (display_column) {
                        display_column->add_attribute(*display_renderer, "text", m_findcolumns.m_col_display);
                        display_column->add_attribute(*display_renderer, "foreground-gdk", m_findcolumns.m_col_display_color);
                }
        }
        {
                Gtk::TreeViewColumn *dist_column = m_treeview->get_column(dist_col);
                if (dist_column) {
                        dist_column->add_attribute(*dist_renderer, "text", m_findcolumns.m_col_dist);
                        //dist_column->set_cell_data_func(*dist_renderer, sigc::mem_fun(*this, &RouteEditUi::FindWindow::distance_cell_func));
                }
                dist_renderer->set_property("xalign", 1.0);
        }
        for (unsigned int i = 0; i < 2; ++i) {
                Gtk::TreeViewColumn *col = m_treeview->get_column(i);
                if (!col)
                        continue;
                col->set_resizable(true);
                const Gtk::TreeModelColumnBase *cols[2] = { &m_findcolumns.m_col_display, &m_findcolumns.m_col_dist };
                col->set_sort_column(*cols[i]);
                col->set_expand(i == 0);
        }
        m_findstore->set_sort_func(m_findcolumns.m_col_dist, sigc::mem_fun(*this, &RouteEditUi::FindWindow::sort_dist));
#if 0
        for (unsigned int i = 1; i < 2; ++i) {
                Gtk::CellRenderer *renderer = m_treeview->get_column_cell_renderer(i);
                if (!renderer)
                        continue;
                renderer->set_property("xalign", 1.0);
        }
#endif
        //m_treeview->columns_autosize();
        //m_treeview->set_headers_clickable();
        // selection
        Glib::RefPtr<Gtk::TreeSelection> find_selection = m_treeview->get_selection();
        find_selection->set_mode(Gtk::SELECTION_SINGLE);
        find_selection->signal_changed().connect(sigc::mem_fun(*this, &FindWindow::find_selection_changed));
        find_selection->set_select_function(sigc::mem_fun(*this, &FindWindow::find_select_function));
        m_buttonok->set_sensitive(false);
        // Insert Headers
        Gdk::Color black;
        black.set_grey(0);
        Gtk::TreeModel::Row row(*(m_findstore->append()));
        row[m_findcolumns.m_col_display] = "Flightplan Waypoints";
        row[m_findcolumns.m_col_display_color] = black;
        row[m_findcolumns.m_col_dist] = "";
        row = *(m_findstore->append());
        row[m_findcolumns.m_col_display] = "Airports";
        row[m_findcolumns.m_col_display_color] = black;
        row[m_findcolumns.m_col_dist] = "";
        row = *(m_findstore->append());
        row[m_findcolumns.m_col_display] = "Navaids";
        row[m_findcolumns.m_col_display_color] = black;
        row[m_findcolumns.m_col_dist] = "";
        row = *(m_findstore->append());
        row[m_findcolumns.m_col_display] = "Waypoints";
        row[m_findcolumns.m_col_display_color] = black;
        row[m_findcolumns.m_col_dist] = "";
        row = *(m_findstore->append());
        row[m_findcolumns.m_col_display] = "Map Elements";
        row[m_findcolumns.m_col_display_color] = black;
        row[m_findcolumns.m_col_dist] = "";
        row = *(m_findstore->append());
        row[m_findcolumns.m_col_display] = "Airspaces";
        row[m_findcolumns.m_col_display_color] = black;
        row[m_findcolumns.m_col_dist] = "";
        row = *(m_findstore->append());
        row[m_findcolumns.m_col_display] = "Airways";
        row[m_findcolumns.m_col_display_color] = black;
        row[m_findcolumns.m_col_dist] = "";
}

int RouteEditUi::FindWindow::sort_dist(const Gtk::TreeModel::iterator& ia, const Gtk::TreeModel::iterator& ib)
{
        //Gtk::TreeModel::Path pa(m_findstore->get_path(ia)), pb(m_findstore->get_path(ib));
        const Gtk::TreeModel::Row rowa(*ia), rowb(*ib);
        return compare_dist(rowa.get_value(m_findcolumns.m_col_dist), rowb.get_value(m_findcolumns.m_col_dist));
}

template<> const unsigned int RouteEditUi::FindWindow::FindObjectInfo<FPlanWaypoint>::parent_row = 0;
template<> const unsigned int RouteEditUi::FindWindow::FindObjectInfo<AirportsDb::Airport>::parent_row = 1;
template<> const unsigned int RouteEditUi::FindWindow::FindObjectInfo<NavaidsDb::Navaid>::parent_row = 2;
template<> const unsigned int RouteEditUi::FindWindow::FindObjectInfo<WaypointsDb::Waypoint>::parent_row = 3;
template<> const unsigned int RouteEditUi::FindWindow::FindObjectInfo<MapelementsDb::Mapelement>::parent_row = 4;
template<> const unsigned int RouteEditUi::FindWindow::FindObjectInfo<AirspacesDb::Airspace>::parent_row = 5;
template<> const unsigned int RouteEditUi::FindWindow::FindObjectInfo<AirwaysDb::Airway>::parent_row = 6;

template<typename T> void RouteEditUi::FindWindow::set_objects(const T& v1, const T& v2)
{
        Gtk::TreeModel::Row parentrow(m_findstore->children()[FindObjectInfo<typename T::value_type>::parent_row]);
        Gtk::TreeModel::Path parentpath(m_findstore->get_path(parentrow));
        bool exp(m_treeview->row_expanded(parentpath));
        {
                Gtk::TreeIter iter;
                while (iter = parentrow->children().begin())
                        m_findstore->erase(iter);
        }
        for (typename T::const_iterator i1(v1.begin()), ie(v1.end()); i1 != ie; i1++) {
                const typename T::value_type& nn(*i1);
                Gtk::TreeModel::Row row(*(m_findstore->append(parentrow.children())));
                set_row(row, nn, false);
        }
        for (typename T::const_iterator i1(v2.begin()), ie(v2.end()); i1 != ie; i1++) {
                const typename T::value_type& nn(*i1);
                Gtk::TreeModel::Row row(*(m_findstore->append(parentrow.children())));
                set_row(row, nn, true);
        }
        if (exp)
                m_treeview->expand_row(parentpath, false);
        else
                m_treeview->collapse_row(parentpath);
        Gdk::Color black;
        black.set_grey(0);
        parentrow[m_findcolumns.m_col_display_color] = black;
}

void RouteEditUi::FindWindow::clear_objects(void)
{
        Gdk::Color blue;
        blue.set_rgb(0, 0, 0xffff);
        for (Gtk::TreeIter i1(m_findstore->children().begin()); i1; i1++) {
                Gtk::TreeModel::Row parentrow(*i1);
                Gtk::TreeIter iter;
                while (iter = parentrow->children().begin())
                        m_findstore->erase(iter);
                parentrow[m_findcolumns.m_col_display_color] = blue;
        }
}

void RouteEditUi::FindWindow::find_selection_changed(void)
{
        Glib::RefPtr<Gtk::TreeSelection> find_selection = m_treeview->get_selection();
        Gtk::TreeModel::iterator iter = find_selection->get_selected();
        if (iter) {
                Gtk::TreeModel::Row row = *iter;
                //Do something with the row.
                std::cerr << "selected row: " << row[m_findcolumns.m_col_display] << std::endl;
                m_buttonok->set_sensitive(true);
                return;
        }
        std::cerr << "selection cleared" << std::endl;
        m_buttonok->set_sensitive(false);
}

bool RouteEditUi::FindWindow::find_select_function(const Glib::RefPtr< Gtk::TreeModel > & model, const Gtk::TreeModel::Path & path, bool)
{
        return (path.size() > 1);
}

void RouteEditUi::FindWindow::buttonok_clicked(void)
{
        Glib::RefPtr<Gtk::TreeSelection> find_selection = m_treeview->get_selection();
        Gtk::TreeModel::iterator iter = find_selection->get_selected();
        if (!iter)
                return;
        Gtk::TreeModel::Row row = *iter;
        switch (row[m_findcolumns.m_col_type]) {
                case 1:
                {
                        std::vector<FPlanWaypoint> wpts;
                        iter = row.children().begin();
                        while (iter) {
                                row = *iter;
                                std::cerr << "set route point: " << row[m_findcolumns.m_col_display] << std::endl;
                                iter++;
                                FPlanWaypoint wpt;
                                wpt.set_icao(row[m_findcolumns.m_col_icao]);
                                wpt.set_name(row[m_findcolumns.m_col_name]);
                                wpt.set_note(row[m_findcolumns.m_col_comment]);
                                wpt.set_frequency(row[m_findcolumns.m_col_freq]);
                                wpt.set_coord(row[m_findcolumns.m_col_coord]);
                                wpt.set_altitude(row[m_findcolumns.m_col_alt]);
                                wpt.set_flags(row[m_findcolumns.m_col_flags]);
				wpt.set_pathcode(row[m_findcolumns.m_col_pathcode]);
				wpt.set_pathname(row[m_findcolumns.m_col_pathname]);
				wpt.set_type(row[m_findcolumns.m_col_wpttype], row[m_findcolumns.m_col_wptnavaid]);
				wpts.push_back(wpt);
                        }
                        for (std::vector<FPlanWaypoint>::size_type i = 0; i < wpts.size(); i++) {
                                if (i)
                                        signal_insertwaypoint()();
                                signal_setwaypoint()(wpts[i]);
                        }
                        break;
                }

                default:
                {
                        std::cerr << "set point: " << row[m_findcolumns.m_col_display] << std::endl;
                        FPlanWaypoint wpt;
                        wpt.set_icao(row[m_findcolumns.m_col_icao]);
                        wpt.set_name(row[m_findcolumns.m_col_name]);
                        wpt.set_note(row[m_findcolumns.m_col_comment]);
                        wpt.set_frequency(row[m_findcolumns.m_col_freq]);
                        wpt.set_coord(row[m_findcolumns.m_col_coord]);
                        wpt.set_altitude(row[m_findcolumns.m_col_alt]);
                        wpt.set_flags(row[m_findcolumns.m_col_flags]);
			wpt.set_pathcode(row[m_findcolumns.m_col_pathcode]);
			wpt.set_pathname(row[m_findcolumns.m_col_pathname]);
			wpt.set_type(row[m_findcolumns.m_col_wpttype], row[m_findcolumns.m_col_wptnavaid]);
			signal_setwaypoint()(wpt);
                        break;
                }
        }
        hide();
}

void RouteEditUi::FindWindow::buttoncancel_clicked(void)
{
        hide();
        async_cancel();
}

void RouteEditUi::FindWindow::on_my_hide(void)
{
        clear_objects();
        std::cerr << "find: " << this << " hide" << std::endl;
}

bool RouteEditUi::FindWindow::on_my_delete_event(GdkEventAny * event)
{
        hide();
        return true;
}

RouteEditUi::FindWindow::FindModelColumns::FindModelColumns(void)
{
        add(m_col_display);
        add(m_col_display_color);
        add(m_col_dist);
        add(m_col_icao);
        add(m_col_name);
        add(m_col_coord);
        add(m_col_alt);
        add(m_col_flags);
        add(m_col_freq);
	add(m_col_pathcode);
        add(m_col_pathname);
        add(m_col_comment);
	add(m_col_wpttype);
	add(m_col_wptnavaid);
        add(m_col_type);
}

void RouteEditUi::FindWindow::set_row_display(Gtk::TreeModel::Row & row, const Glib::ustring & comp1, const Glib::ustring & comp2)
{
        Glib::ustring s(comp1);
        if (!comp1.empty() && !comp2.empty())
                s += ' ';
        s += comp2;
        row[m_findcolumns.m_col_display] = s;
        Gdk::Color black;
        black.set_grey(0);
        row[m_findcolumns.m_col_display_color] = black;
}

void RouteEditUi::FindWindow::set_row(Gtk::TreeModel::Row & row, const FPlanWaypoint & nn, bool dispreverse)
{
        Glib::ustring icao(nn.get_icao()), name(nn.get_name());
        Point pt(nn.get_coord());
        row[m_findcolumns.m_col_dist] = Conversions::dist_str(pt.spheric_distance_nmi(m_center));
        row[m_findcolumns.m_col_icao] = icao;
        row[m_findcolumns.m_col_name] = name;
        row[m_findcolumns.m_col_coord] = pt;
        row[m_findcolumns.m_col_alt] = nn.get_altitude();
        row[m_findcolumns.m_col_flags] = nn.get_flags();
        row[m_findcolumns.m_col_freq] = nn.get_frequency();
	row[m_findcolumns.m_col_pathcode] = FPlanWaypoint::pathcode_none;
        row[m_findcolumns.m_col_pathname] = "";
        row[m_findcolumns.m_col_comment] = nn.get_note();
	row[m_findcolumns.m_col_wpttype] = FPlanWaypoint::type_fplwaypoint;
	row[m_findcolumns.m_col_wptnavaid] = FPlanWaypoint::navaid_invalid;
        row[m_findcolumns.m_col_type] = 0;
        if (dispreverse)
                set_row_display(row, name, icao);
        else
                set_row_display(row, icao, name);
}

void RouteEditUi::FindWindow::set_row(Gtk::TreeModel::Row & row, const AirportsDb::Airport & nn, bool dispreverse)
{
        Glib::ustring icao(nn.get_icao()), name(nn.get_name()), comment;
        Point pt(nn.get_coord());
        row[m_findcolumns.m_col_dist] = Conversions::dist_str(pt.spheric_distance_nmi(m_center));
        row[m_findcolumns.m_col_icao] = icao;
        row[m_findcolumns.m_col_name] = name;
        row[m_findcolumns.m_col_coord] = pt;
        row[m_findcolumns.m_col_alt] = nn.get_elevation();
        row[m_findcolumns.m_col_flags] = 0;
        row[m_findcolumns.m_col_freq] = 0;
        if (nn.get_nrcomm())
                row[m_findcolumns.m_col_freq] = nn.get_comm(0)[0];
 	row[m_findcolumns.m_col_pathcode] = FPlanWaypoint::pathcode_none;
        row[m_findcolumns.m_col_pathname] = "";
	row[m_findcolumns.m_col_comment] = comment;
	row[m_findcolumns.m_col_wpttype] = FPlanWaypoint::type_airport;
	row[m_findcolumns.m_col_wptnavaid] = FPlanWaypoint::navaid_invalid;
        row[m_findcolumns.m_col_type] = 0;
        if (dispreverse)
                set_row_display(row, name, icao);
        else
                set_row_display(row, icao, name);
        for (unsigned int rtenr = 0; rtenr < nn.get_nrvfrrte(); rtenr++) {
                const AirportsDb::Airport::VFRRoute& rte(nn.get_vfrrte(rtenr));
                Gtk::TreeModel::Row rterow(*(m_findstore->append(row.children())));
                Point pt1(pt);
                if (rte.size() > 0) {
                        const AirportsDb::Airport::VFRRoute::VFRRoutePoint& rtep(rte[0]);
                        pt1 = rtep.get_coord();
                }
                rterow[m_findcolumns.m_col_dist] = Conversions::dist_str(pt1.spheric_distance_nmi(m_center));
                rterow[m_findcolumns.m_col_icao] = icao;
                rterow[m_findcolumns.m_col_name] = name;
                rterow[m_findcolumns.m_col_coord] = pt;
                rterow[m_findcolumns.m_col_alt] = nn.get_elevation();
                rterow[m_findcolumns.m_col_flags] = 0;
                rterow[m_findcolumns.m_col_freq] = 0;
                if (nn.get_nrcomm())
                        rterow[m_findcolumns.m_col_freq] = nn.get_comm(0)[0];
		rterow[m_findcolumns.m_col_pathcode] = FPlanWaypoint::pathcode_vfrtransition;
		rterow[m_findcolumns.m_col_pathname] = rte.get_name();
                rterow[m_findcolumns.m_col_comment] = comment;
		rterow[m_findcolumns.m_col_wpttype] = FPlanWaypoint::type_vfrreportingpt;
		rterow[m_findcolumns.m_col_wptnavaid] = FPlanWaypoint::navaid_invalid;
                rterow[m_findcolumns.m_col_type] = 1;
                rterow[m_findcolumns.m_col_display] = rte.get_name();
                for (unsigned int rtepnr = 0; rtepnr < rte.size(); rtepnr++) {
                        const AirportsDb::Airport::VFRRoute::VFRRoutePoint& rtep(rte[rtepnr]);
                        Gtk::TreeModel::Row rteprow(*(m_findstore->append(rterow.children())));
                        Point pt2(rtep.get_coord());
                        rteprow[m_findcolumns.m_col_dist] = Conversions::dist_str(pt2.spheric_distance_nmi(m_center));
                        rteprow[m_findcolumns.m_col_icao] = icao;
                        rteprow[m_findcolumns.m_col_coord] = pt2;
                        if (rtep.is_at_airport()) {
                                rteprow[m_findcolumns.m_col_name] = name;
                                rteprow[m_findcolumns.m_col_alt] = nn.get_elevation();
                        } else {
                                rteprow[m_findcolumns.m_col_name] = rtep.get_name();
                                rteprow[m_findcolumns.m_col_alt] = rtep.get_altitude();
                        }
                        rteprow[m_findcolumns.m_col_flags] = 0;
                        rteprow[m_findcolumns.m_col_freq] = 0;
                        if (nn.get_nrcomm())
                                rteprow[m_findcolumns.m_col_freq] = nn.get_comm(0)[0];
			switch (rtep.get_pathcode()) {
			case AirportsDb::Airport::VFRRoute::VFRRoutePoint::pathcode_arrival:
				rteprow[m_findcolumns.m_col_pathcode] = FPlanWaypoint::pathcode_vfrarrival;
				break;

			case AirportsDb::Airport::VFRRoute::VFRRoutePoint::pathcode_departure:
				rteprow[m_findcolumns.m_col_pathcode] = FPlanWaypoint::pathcode_vfrdeparture;
				break;

			case AirportsDb::Airport::VFRRoute::VFRRoutePoint::pathcode_vfrtransition:
				rteprow[m_findcolumns.m_col_pathcode] = FPlanWaypoint::pathcode_vfrtransition;
				break;

			case AirportsDb::Airport::VFRRoute::VFRRoutePoint::pathcode_holding:
			case AirportsDb::Airport::VFRRoute::VFRRoutePoint::pathcode_trafficpattern:
			case AirportsDb::Airport::VFRRoute::VFRRoutePoint::pathcode_other:
			case AirportsDb::Airport::VFRRoute::VFRRoutePoint::pathcode_invalid:
			default:
				rteprow[m_findcolumns.m_col_pathcode] = FPlanWaypoint::pathcode_none;
				break;
			}
			rteprow[m_findcolumns.m_col_pathname] = rte.get_name();
                        rteprow[m_findcolumns.m_col_comment] = comment;
			rteprow[m_findcolumns.m_col_wpttype] = FPlanWaypoint::type_vfrreportingpt;
			rteprow[m_findcolumns.m_col_wptnavaid] = FPlanWaypoint::navaid_invalid;
			rteprow[m_findcolumns.m_col_type] = 0;
                        rteprow[m_findcolumns.m_col_display] = rtep.get_name();
                }
        }
}

void RouteEditUi::FindWindow::set_row(Gtk::TreeModel::Row & row, const NavaidsDb::Navaid & nn, bool dispreverse)
{
        Glib::ustring icao(nn.get_icao()), name(nn.get_name()), comment(nn.get_navaid_typename());
        Point pt(nn.get_coord());
        char buf[16];
        snprintf(buf, sizeof(buf), "%d", nn.get_range());
        comment += " Range: ";
        comment += buf;
        comment += "\n";
        row[m_findcolumns.m_col_dist] = Conversions::dist_str(pt.spheric_distance_nmi(m_center));
        row[m_findcolumns.m_col_icao] = icao;
        row[m_findcolumns.m_col_name] = name;
        row[m_findcolumns.m_col_coord] = pt;
        row[m_findcolumns.m_col_alt] = nn.get_elev();
        row[m_findcolumns.m_col_flags] = 0;
        row[m_findcolumns.m_col_freq] = nn.get_frequency();
	row[m_findcolumns.m_col_pathcode] = FPlanWaypoint::pathcode_none;
        row[m_findcolumns.m_col_pathname] = "";
        row[m_findcolumns.m_col_comment] = comment;
	row[m_findcolumns.m_col_wpttype] = FPlanWaypoint::type_navaid;
	row[m_findcolumns.m_col_wptnavaid] = static_cast<FPlanWaypoint::navaid_type_t>(nn.get_navaid_type());
        row[m_findcolumns.m_col_type] = 0;
        if (dispreverse)
                set_row_display(row, name, icao);
        else
                set_row_display(row, icao, name);
}

void RouteEditUi::FindWindow::set_row(Gtk::TreeModel::Row & row, const WaypointsDb::Waypoint & nn, bool dispreverse)
{
        Glib::ustring name(nn.get_name());
        Point pt(nn.get_coord());
        row[m_findcolumns.m_col_dist] = Conversions::dist_str(pt.spheric_distance_nmi(m_center));
        row[m_findcolumns.m_col_icao] = "";
        row[m_findcolumns.m_col_name] = name;
        row[m_findcolumns.m_col_coord] = pt;
        row[m_findcolumns.m_col_alt] = std::numeric_limits<int>::min();
        row[m_findcolumns.m_col_flags] = 0;
        row[m_findcolumns.m_col_freq] = 0;
	row[m_findcolumns.m_col_pathcode] = FPlanWaypoint::pathcode_none;
        row[m_findcolumns.m_col_pathname] = "";
        row[m_findcolumns.m_col_comment] = "";
	row[m_findcolumns.m_col_wpttype] = FPlanWaypoint::type_intersection;
	row[m_findcolumns.m_col_wptnavaid] = FPlanWaypoint::navaid_invalid;
        row[m_findcolumns.m_col_type] = 0;
        set_row_display(row, name);
}

void RouteEditUi::FindWindow::set_row(Gtk::TreeModel::Row & row, const AirwaysDb::Airway & nn, bool dispreverse)
{
        Glib::ustring name(nn.get_name()), nameb(nn.get_begin_name()), namee(nn.get_end_name());
        Point ptb(nn.get_begin_coord()), pte(nn.get_end_coord());
        float db(ptb.spheric_distance_nmi(m_center)), de(pte.spheric_distance_nmi(m_center));
        if (db > de) {
                std::swap(db, de);
                std::swap(ptb, pte);
                std::swap(nameb, namee);
        }
        row[m_findcolumns.m_col_dist] = Conversions::dist_str(db);
        row[m_findcolumns.m_col_icao] = "";
        row[m_findcolumns.m_col_name] = name;
        row[m_findcolumns.m_col_coord] = ptb;
        row[m_findcolumns.m_col_alt] = nn.get_base_level() * 100;
        row[m_findcolumns.m_col_flags] = 0;
        row[m_findcolumns.m_col_freq] = 0;
	row[m_findcolumns.m_col_pathcode] = FPlanWaypoint::pathcode_airway;
        row[m_findcolumns.m_col_pathname] = name;
        row[m_findcolumns.m_col_comment] = "";
	row[m_findcolumns.m_col_wpttype] = FPlanWaypoint::type_intersection;
	row[m_findcolumns.m_col_wptnavaid] = FPlanWaypoint::navaid_invalid;
        row[m_findcolumns.m_col_type] = 1;
        {
                std::ostringstream oss;
                oss << '(' << nameb << '-' << namee
                    << ", FL" << nn.get_base_level() << "-FL" << nn.get_top_level()
                    << ", " << nn.get_type_name() << ')';
                set_row_display(row, name, oss.str());
        }
        std::ostringstream oss;
        oss << nn.get_type_name() << ", FL" << nn.get_base_level() << "-FL" << nn.get_top_level();
        Gtk::TreeModel::Row rowb(*(m_findstore->append(row.children())));
        Gtk::TreeModel::Row rowe(*(m_findstore->append(row.children())));
        rowb[m_findcolumns.m_col_dist] = Conversions::dist_str(db);
        rowb[m_findcolumns.m_col_icao] = "";
        rowb[m_findcolumns.m_col_name] = nameb;
        rowb[m_findcolumns.m_col_coord] = ptb;
        rowb[m_findcolumns.m_col_alt] = nn.get_base_level() * 100;
        rowb[m_findcolumns.m_col_flags] = 0;
        rowb[m_findcolumns.m_col_freq] = 0;
	rowb[m_findcolumns.m_col_pathcode] = FPlanWaypoint::pathcode_airway;
        rowb[m_findcolumns.m_col_pathname] = name;
        rowb[m_findcolumns.m_col_comment] = oss.str();
	rowb[m_findcolumns.m_col_wpttype] = FPlanWaypoint::type_intersection;
	rowb[m_findcolumns.m_col_wptnavaid] = FPlanWaypoint::navaid_invalid;
        rowb[m_findcolumns.m_col_type] = 0;
        set_row_display(rowb, nameb);
        rowe[m_findcolumns.m_col_dist] = Conversions::dist_str(de);
        rowe[m_findcolumns.m_col_icao] = "";
        rowe[m_findcolumns.m_col_name] = namee;
        rowe[m_findcolumns.m_col_coord] = pte;
        rowe[m_findcolumns.m_col_alt] = nn.get_base_level() * 100;
        rowe[m_findcolumns.m_col_flags] = 0;
        rowe[m_findcolumns.m_col_freq] = 0;
	rowe[m_findcolumns.m_col_pathcode] = FPlanWaypoint::pathcode_airway;
        rowe[m_findcolumns.m_col_pathname] = name;
        rowe[m_findcolumns.m_col_comment] = "";
	rowe[m_findcolumns.m_col_wpttype] = FPlanWaypoint::type_intersection;
	rowe[m_findcolumns.m_col_wptnavaid] = FPlanWaypoint::navaid_invalid;
        rowe[m_findcolumns.m_col_type] = 0;
        set_row_display(rowe, namee);
}

void RouteEditUi::FindWindow::set_row(Gtk::TreeModel::Row & row, const AirspacesDb::Airspace & nn, bool dispreverse)
{
        Glib::ustring icao(nn.get_icao()), name(nn.get_name());
        Point pt(nn.get_labelcoord());
        Rect r(nn.get_bbox());
        row[m_findcolumns.m_col_dist] = Conversions::dist_str(r.spheric_distance_nmi(m_center));
        row[m_findcolumns.m_col_icao] = icao;
        row[m_findcolumns.m_col_name] = name;
        row[m_findcolumns.m_col_coord] = pt;
        row[m_findcolumns.m_col_alt] = (nn.get_altlwr() + nn.get_altupr()) / 2;
        row[m_findcolumns.m_col_flags] = 0;
        row[m_findcolumns.m_col_freq] = nn.get_commfreq(0);
	row[m_findcolumns.m_col_pathcode] = FPlanWaypoint::pathcode_none;
        row[m_findcolumns.m_col_pathname] = "";
        row[m_findcolumns.m_col_comment] = nn.get_commname() + "\n" +
                nn.get_classexcept() + "\n" + nn.get_ident() + "\n" +
                nn.get_efftime() + "\n" + nn.get_note() + "\n";
	row[m_findcolumns.m_col_wpttype] = FPlanWaypoint::type_user;
	row[m_findcolumns.m_col_wptnavaid] = FPlanWaypoint::navaid_invalid;
        row[m_findcolumns.m_col_type] = 0;
        if (dispreverse)
                set_row_display(row, name, icao);
        else
                set_row_display(row, icao, name);
}

void RouteEditUi::FindWindow::set_row(Gtk::TreeModel::Row & row, const MapelementsDb::Mapelement & nn, bool dispreverse)
{
        Glib::ustring name(nn.get_name());
        Point pt(nn.get_labelcoord());
        Rect r(nn.get_bbox());
        row[m_findcolumns.m_col_dist] = Conversions::dist_str(r.spheric_distance_nmi(m_center));
        row[m_findcolumns.m_col_icao] = "";
        row[m_findcolumns.m_col_name] = name;
        row[m_findcolumns.m_col_coord] = pt;
        row[m_findcolumns.m_col_alt] = std::numeric_limits<int>::min();
        row[m_findcolumns.m_col_flags] = 0;
        row[m_findcolumns.m_col_freq] = 0;
	row[m_findcolumns.m_col_pathcode] = FPlanWaypoint::pathcode_none;
        row[m_findcolumns.m_col_pathname] = "";
        row[m_findcolumns.m_col_comment] = "";
	row[m_findcolumns.m_col_wpttype] = FPlanWaypoint::type_mapelement;
	row[m_findcolumns.m_col_wptnavaid] = FPlanWaypoint::navaid_invalid;
        row[m_findcolumns.m_col_type] = 0;
        set_row_display(row, name);
}

void RouteEditUi::FindWindow::async_dispatchdone(void)
{
        m_querydispatch();
}

void RouteEditUi::FindWindow::async_done(void)
{
        if ((m_airportquery || m_airportquery1) && (!m_airportquery || m_airportquery->is_done()) && (!m_airportquery1 || m_airportquery1->is_done())) {
                Engine::AirportResult::elementvector_t empty;
                const Engine::AirportResult::elementvector_t *v1 = &empty, *v2 = &empty;

                if (m_airportquery)
                        std::cerr << "Airportquery: error: " << m_airportquery->is_error() << std::endl;
                if (m_airportquery1)
                        std::cerr << "Airportquery1: error: " << m_airportquery1->is_error() << std::endl;

                if (m_airportquery && !m_airportquery->is_error())
                        v1 = &m_airportquery->get_result();
                if (m_airportquery1 && !m_airportquery1->is_error())
                        v2 = &m_airportquery1->get_result();
                set_objects(*v1, *v2);
                m_airportquery = Glib::RefPtr<Engine::AirportResult>();
                m_airportquery1 = Glib::RefPtr<Engine::AirportResult>();
        }
        if ((m_airspacequery || m_airspacequery1) && (!m_airspacequery || m_airspacequery->is_done()) && (!m_airspacequery1 || m_airspacequery1->is_done())) {
                Engine::AirspaceResult::elementvector_t empty;
                const Engine::AirspaceResult::elementvector_t *v1 = &empty, *v2 = &empty;
                if (m_airspacequery && !m_airspacequery->is_error())
                        v1 = &m_airspacequery->get_result();
                if (m_airspacequery1 && !m_airspacequery1->is_error())
                        v2 = &m_airspacequery1->get_result();
                set_objects(*v1, *v2);
                m_airspacequery = Glib::RefPtr<Engine::AirspaceResult>();
                m_airspacequery1 = Glib::RefPtr<Engine::AirspaceResult>();
        }
        if ((m_navaidquery || m_navaidquery1) && (!m_navaidquery || m_navaidquery->is_done()) && (!m_navaidquery1 || m_navaidquery1->is_done())) {
                Engine::NavaidResult::elementvector_t empty;
                const Engine::NavaidResult::elementvector_t *v1 = &empty, *v2 = &empty;
                if (m_navaidquery && !m_navaidquery->is_error())
                        v1 = &m_navaidquery->get_result();
                if (m_navaidquery1 && !m_navaidquery1->is_error())
                        v2 = &m_navaidquery1->get_result();
                set_objects(*v1, *v2);
                m_navaidquery = Glib::RefPtr<Engine::NavaidResult>();
                m_navaidquery1 = Glib::RefPtr<Engine::NavaidResult>();
        }
        if (m_waypointquery && m_waypointquery->is_done()) {
                if (!m_waypointquery->is_error())
                        set_objects(m_waypointquery->get_result());
                m_waypointquery = Glib::RefPtr<Engine::WaypointResult>();
        }
        if (m_airwayquery && m_airwayquery->is_done()) {
                if (!m_airwayquery->is_error())
                        set_objects(m_airwayquery->get_result());
                m_airwayquery = Glib::RefPtr<Engine::AirwayResult>();
        }
        if (m_mapelementquery && m_mapelementquery->is_done()) {
                if (!m_mapelementquery->is_error())
                        set_objects(m_mapelementquery->get_result());
                m_mapelementquery = Glib::RefPtr<Engine::MapelementResult>();
        }
}

void RouteEditUi::FindWindow::async_cancel(void)
{
        Engine::AirportResult::cancel(m_airportquery);
        Engine::AirportResult::cancel(m_airportquery1);
        Engine::AirspaceResult::cancel(m_airspacequery);
        Engine::AirspaceResult::cancel(m_airspacequery1);
        Engine::NavaidResult::cancel(m_navaidquery);
        Engine::NavaidResult::cancel(m_navaidquery1);
        Engine::WaypointResult::cancel(m_waypointquery);
        Engine::AirwayResult::cancel(m_airwayquery);
        Engine::MapelementResult::cancel(m_mapelementquery);
}

RouteEditUi::FindNameWindow::FindNameWindow(BaseObjectType * castitem, const Glib::RefPtr<Builder>& refxml)
        : FindWindow(castitem, refxml), m_entry(0), m_searchtype(0)
{
        get_widget(m_refxml, "findname_treeview", m_treeview);
        Gtk::Button *bcancel, *bclear, *bfind;
        get_widget(m_refxml, "findname_buttonok", m_buttonok);
        get_widget(m_refxml, "findname_buttoncancel", bcancel);
        get_widget(m_refxml, "findname_buttonclear", bclear);
        get_widget(m_refxml, "findname_buttonfind", bfind);
        get_widget(m_refxml, "findname_searchtype", m_searchtype);
        m_buttonok->signal_clicked().connect(sigc::mem_fun(*this, &FindWindow::buttonok_clicked));
        bcancel->signal_clicked().connect(sigc::mem_fun(*this, &FindWindow::buttoncancel_clicked));
        bclear->signal_clicked().connect(sigc::mem_fun(*this, &FindNameWindow::buttonclear_clicked));
        bfind->signal_clicked().connect(sigc::mem_fun(*this, &FindNameWindow::buttonfind_clicked));
        m_searchtype->signal_changed().connect(sigc::mem_fun(*this, &FindNameWindow::searchtype_changed));
        m_searchtype->set_active(0);
        get_widget(m_refxml, "findname_entry", m_entry);
        m_entry->signal_changed().connect(sigc::mem_fun(*this, &FindNameWindow::entry_changed));
        init_list();
}

RouteEditUi::FindNameWindow::~FindNameWindow()
{
}

void RouteEditUi::FindNameWindow::set_center(const Point & pt)
{
        FindWindow::set_center(pt);
        entry_changed();
}

void RouteEditUi::FindNameWindow::buttonclear_clicked(void)
{
        m_entry->delete_text(0, std::numeric_limits<int>::max());
}

void RouteEditUi::FindNameWindow::buttonfind_clicked(void)
{
        if (!m_engine)
                return;
        DbQueryInterfaceCommon::comp_t comp;
        FPlan::comp_t fpcomp;
        std::cerr << "Search type: " << m_searchtype->get_active_row_number() << std::endl;
        switch (m_searchtype->get_active_row_number()) {
	default:
	case 0:
		comp = DbQueryInterfaceCommon::comp_startswith;
		fpcomp = FPlan::comp_startswith;
		break;

	case 1:
		comp = DbQueryInterfaceCommon::comp_exact;
		fpcomp = FPlan::comp_exact;
		break;

	case 2:
		comp = DbQueryInterfaceCommon::comp_contains;
		fpcomp = FPlan::comp_contains;
		break;
        }
        clear_objects();
        Glib::ustring text(m_entry->get_text());
        {
                std::vector<FPlanWaypoint> wpts(m_engine->get_fplan_db().find_by_icao(text, FindWindow::line_limit / 2, 0, fpcomp));
                std::vector<FPlanWaypoint> wpts1(m_engine->get_fplan_db().find_by_name(text, FindWindow::line_limit / 2, 0, fpcomp));
                set_objects(wpts, wpts1);
        }
#if 1
        async_cancel();
        m_airportquery = m_engine->async_airport_find_by_icao(text, FindWindow::line_limit / 2, 0, comp, AirportsDb::element_t::subtables_vfrroutes | AirportsDb::element_t::subtables_comms);
        m_airportquery1 = m_engine->async_airport_find_by_name(text, FindWindow::line_limit / 2, 0, comp, AirportsDb::element_t::subtables_vfrroutes | AirportsDb::element_t::subtables_comms);
        m_navaidquery = m_engine->async_navaid_find_by_icao(text, FindWindow::line_limit / 2, 0, comp, NavaidsDb::element_t::subtables_none);
        m_navaidquery1 = m_engine->async_navaid_find_by_name(text, FindWindow::line_limit / 2, 0, comp, NavaidsDb::element_t::subtables_none);
        m_waypointquery = m_engine->async_waypoint_find_by_name(text, FindWindow::line_limit, 0, comp, WaypointsDb::element_t::subtables_none);
        m_airspacequery = m_engine->async_airspace_find_by_icao(text, FindWindow::line_limit / 2, 0, comp, AirspacesDb::element_t::subtables_none);
        m_airspacequery1 = m_engine->async_airspace_find_by_name(text, FindWindow::line_limit / 2, 0, comp, AirspacesDb::element_t::subtables_none);
        m_mapelementquery = m_engine->async_mapelement_find_by_name(text, FindWindow::line_limit, 0, comp, MapelementsDb::element_t::subtables_none);
        m_airwayquery = m_engine->async_airway_find_by_text(text, FindWindow::line_limit, 0, comp, AirwaysDb::element_t::subtables_none);
        if (m_airportquery) {
                m_airportquery->connect(sigc::mem_fun(*this, &FindNameWindow::async_dispatchdone));
        }
        if (m_airportquery1) {
                m_airportquery1->connect(sigc::mem_fun(*this, &FindNameWindow::async_dispatchdone));
        }
        if (m_airspacequery) {
                m_airspacequery->connect(sigc::mem_fun(*this, &FindNameWindow::async_dispatchdone));
        }
        if (m_airspacequery1) {
                m_airspacequery1->connect(sigc::mem_fun(*this, &FindNameWindow::async_dispatchdone));
        }
        if (m_navaidquery) {
                m_navaidquery->connect(sigc::mem_fun(*this, &FindNameWindow::async_dispatchdone));
        }
        if (m_navaidquery1) {
                m_navaidquery1->connect(sigc::mem_fun(*this, &FindNameWindow::async_dispatchdone));
        }
        if (m_waypointquery) {
                m_waypointquery->connect(sigc::mem_fun(*this, &FindNameWindow::async_dispatchdone));
        }
        if (m_mapelementquery) {
                m_mapelementquery->connect(sigc::mem_fun(*this, &FindNameWindow::async_dispatchdone));
        }
        if (m_airwayquery) {
                m_airwayquery->connect(sigc::mem_fun(*this, &FindNameWindow::async_dispatchdone));
        }
#else
        {
                AirportsDb::elementvector_t palmairports(m_engine->airport_find_by_icao(text, FindWindow::line_limit / 2, 0, comp));
                AirportsDb::elementvector_t palmairports1(m_engine->airport_find_by_name(text, FindWindow::line_limit / 2, 0, comp));
                set_objects(palmairports, palmairports1);
        }
        {
                NavaidsDb::elementvector_t palmnavaids(m_engine->navaid_find_by_icao(text, FindWindow::line_limit / 2, 0, comp));
                NavaidsDb::elementvector_t palmnavaids1(m_engine->navaid_find_by_name(text, FindWindow::line_limit / 2, 0, comp));
                set_objects(palmnavaids, palmnavaids1);
        }
        {
                WaypointsDb::elementvector_t palmwaypoints(m_engine->waypoint_find_by_name(text, FindWindow::line_limit / 2, 0, comp));
                set_objects(palmwaypoints);
        }
        {
                AirspacesDb::elementvector_t palmairspaces(m_engine->airspace_find_by_icao(text, FindWindow::line_limit / 2, 0, comp));
                AirspacesDb::elementvector_t palmairspaces1(m_engine->airspace_find_by_name(text, FindWindow::line_limit / 2, 0, comp));
                set_objects(palmairspaces, palmairspaces1);
        }
        {
                MapelementsDb::elementvector_t palmmapelements(m_engine->mapelement_find_by_name(text, FindWindow::line_limit / 2, 0, comp));
                set_objects(palmmapelements);
        }
#endif
}

void RouteEditUi::FindNameWindow::searchtype_changed(void)
{
        // do nothing
}

void RouteEditUi::FindNameWindow::entry_changed(void)
{
        //buttonfind_clicked();
}

RouteEditUi::FindNearestWindow::FindNearestWindow(BaseObjectType * castitem, const Glib::RefPtr<Builder>& refxml)
        : FindWindow(castitem, refxml), m_distance(0)
{
        get_widget(m_refxml, "findnearest_treeview", m_treeview);
        Gtk::Button *bcancel, *bfind;
        get_widget(m_refxml, "findnearest_buttonok", m_buttonok);
        get_widget(m_refxml, "findnearest_buttoncancel", bcancel);
        get_widget(m_refxml, "findnearest_buttonfind", bfind);
        get_widget(m_refxml, "findnearest_distance", m_distance);
        m_buttonok->signal_clicked().connect(sigc::mem_fun(*this, &FindWindow::buttonok_clicked));
        bcancel->signal_clicked().connect(sigc::mem_fun(*this, &FindWindow::buttoncancel_clicked));
        bfind->signal_clicked().connect(sigc::mem_fun(*this, &FindNearestWindow::buttonfind_clicked));
        m_distance->set_value(distance_limit);
        init_list();
}

RouteEditUi::FindNearestWindow::~FindNearestWindow()
{
}


void RouteEditUi::FindNearestWindow::buttonfind_clicked(void)
{
        if (!m_engine)
                return;
        std::cerr << "Distance: " << m_distance->get_value() << std::endl;
        Rect bbox(m_center.simple_box_nmi(m_distance->get_value()));
        clear_objects();
        {
                std::vector<FPlanWaypoint> wpts(m_engine->get_fplan_db().find_nearest(m_center, FindWindow::line_limit, 0, bbox));
                set_objects(wpts);
        }
#if 1
        async_cancel();
        m_airportquery = m_engine->async_airport_find_nearest(m_center, FindWindow::line_limit, bbox, AirportsDb::element_t::subtables_vfrroutes | AirportsDb::element_t::subtables_comms);
        m_airportquery1 = Glib::RefPtr<Engine::AirportResult>();
        m_navaidquery = m_engine->async_navaid_find_nearest(m_center, FindWindow::line_limit, bbox, NavaidsDb::element_t::subtables_none);
        m_navaidquery1 = Glib::RefPtr<Engine::NavaidResult>();
        m_waypointquery = m_engine->async_waypoint_find_nearest(m_center, FindWindow::line_limit, bbox, WaypointsDb::element_t::subtables_none);
        m_airspacequery = m_engine->async_airspace_find_nearest(m_center, FindWindow::line_limit, bbox, AirspacesDb::element_t::subtables_none);
        m_airspacequery1 = Glib::RefPtr<Engine::AirspaceResult>();
        m_mapelementquery = m_engine->async_mapelement_find_nearest(m_center, FindWindow::line_limit, bbox, MapelementsDb::element_t::subtables_none);
        m_airwayquery = m_engine->async_airway_find_nearest(m_center, FindWindow::line_limit, bbox, WaypointsDb::element_t::subtables_none);
        if (m_airportquery) {
                m_airportquery->connect(sigc::mem_fun(*this, &FindNearestWindow::async_dispatchdone));
        }
        if (m_airspacequery) {
                m_airspacequery->connect(sigc::mem_fun(*this, &FindNearestWindow::async_dispatchdone));
        }
        if (m_navaidquery) {
                m_navaidquery->connect(sigc::mem_fun(*this, &FindNearestWindow::async_dispatchdone));
        }
        if (m_waypointquery) {
                m_waypointquery->connect(sigc::mem_fun(*this, &FindNearestWindow::async_dispatchdone));
        }
        if (m_mapelementquery) {
                m_mapelementquery->connect(sigc::mem_fun(*this, &FindNearestWindow::async_dispatchdone));
        }
        if (m_airwayquery) {
                m_airwayquery->connect(sigc::mem_fun(*this, &FindNearestWindow::async_dispatchdone));
        }
#else
        {
                AirportsDb::elementvector_t palmairports(m_engine->airport_find_nearest(m_center, FindWindow::line_limit, bbox));
                set_objects(palmairports);
        }
        {
                NavaidsDb::elementvector_t palmnavaids(m_engine->navaid_find_nearest(m_center, FindWindow::line_limit, bbox));
                set_objects(palmnavaids);
        }
        {
                WaypointsDb::elementvector_t palmwaypoints(m_engine->waypoint_find_nearest(m_center, FindWindow::line_limit, bbox));
                set_objects(palmwaypoints);
        }
        {
                AirspacesDb::elementvector_t palmairspaces(m_engine->airspace_find_nearest(m_center, FindWindow::line_limit, bbox));
                set_objects(palmairspaces);
        }
        {
                MapelementsDb::elementvector_t palmmapelements(m_engine->mapelement_find_nearest(m_center, FindWindow::line_limit, bbox));
                set_objects(palmmapelements);
        }
#endif
}

void RouteEditUi::FindNearestWindow::set_center(const Point & pt)
{
        FindWindow::set_center(pt);
}

RouteEditUi::ConfirmDeleteWindow::ConfirmDeleteWindow(BaseObjectType * castitem, const Glib::RefPtr<Builder>& refxml)
        : FullscreenableWindow(castitem, refxml), m_route(0), m_confirmdeletetextview(0)
{
        get_widget(refxml, "confirmdeletetextview", m_confirmdeletetextview);
        Gtk::Button *bok, *bcancel;
        get_widget(refxml, "confirmdeletebuttonok", bok);
        get_widget(refxml, "confirmdeletebuttoncancel", bcancel);
        bok->signal_clicked().connect(sigc::mem_fun(*this, &ConfirmDeleteWindow::buttonok_clicked));
        bcancel->signal_clicked().connect(sigc::mem_fun(*this, &ConfirmDeleteWindow::buttoncancel_clicked));
        signal_hide().connect(sigc::mem_fun(*this, &RouteEditUi::ConfirmDeleteWindow::on_my_hide));
        signal_show().connect(sigc::mem_fun(*this, &RouteEditUi::ConfirmDeleteWindow::on_my_show));
        signal_delete_event().connect(sigc::mem_fun(*this, &RouteEditUi::ConfirmDeleteWindow::on_my_delete_event));
}

RouteEditUi::ConfirmDeleteWindow::~ConfirmDeleteWindow()
{
}

void RouteEditUi::ConfirmDeleteWindow::on_my_hide()
{
        Glib::RefPtr<Gtk::TextBuffer> txtbuf(m_confirmdeletetextview->get_buffer());
        txtbuf->begin_user_action();
        txtbuf->erase(txtbuf->begin(), txtbuf->end());
        txtbuf->end_user_action();
}

void RouteEditUi::ConfirmDeleteWindow::on_my_show()
{
        Glib::RefPtr<Gtk::TextBuffer> txtbuf(m_confirmdeletetextview->get_buffer());
        txtbuf->begin_user_action();
        txtbuf->erase(txtbuf->begin(), txtbuf->end());
        if (m_route) {
                for (unsigned int i = 0; i < m_route->get_nrwpt(); i++) {
                        const FPlanWaypoint& wpt(m_route->operator[](i));
                        Glib::ustring s(wpt.get_icao());
                        if (!s.empty())
                                s += ' ';
                        s += wpt.get_name();
                        s += '\n';
                        txtbuf->insert(txtbuf->end(), s);
                }
        }
        txtbuf->end_user_action();
}

bool RouteEditUi::ConfirmDeleteWindow::on_my_delete_event(GdkEventAny * event)
{
        hide();
        return true;
}

void RouteEditUi::ConfirmDeleteWindow::buttonok_clicked(void)
{
        hide();
        signal_delete()();
}

void RouteEditUi::ConfirmDeleteWindow::buttoncancel_clicked(void)
{
        hide();
}

RouteEditUi::MapWindow::MapWindow(BaseObjectType * castitem, const Glib::RefPtr<Builder>& refxml)
        : FullscreenableWindow(castitem, refxml), m_engine(0), m_route(0), m_drawingarea(0), m_buttonok(0), m_buttoninfo(0),
          m_menucoord(0), m_renderer(VectorMapArea::renderer_none)
{
        get_widget_derived(refxml, "mapdrawingarea", m_drawingarea);
        Gtk::ToolButton *bcancel, *bzoomin, *bzoomout;
        get_widget(refxml, "mapbuttonok", m_buttonok);
        get_widget(refxml, "mapbuttoninfo", m_buttoninfo);
        get_widget(refxml, "mapbuttoncancel", bcancel);
        get_widget(refxml, "mapbuttonzoomin", bzoomin);
        get_widget(refxml, "mapbuttonzoomout", bzoomout);
        get_widget(refxml, "mapmenucoord", m_menucoord);
        Gtk::MenuItem *m_menucenter;
        Gtk::MenuItem *m_menuclear;
        get_widget(refxml, "mapmenucenter", m_menucenter);
        get_widget(refxml, "mapmenuclear", m_menuclear);
        m_buttonok->signal_clicked().connect(sigc::mem_fun(*this, &MapWindow::buttonok_clicked));
        m_buttoninfo->signal_clicked().connect(sigc::mem_fun(*this, &MapWindow::buttoninfo_clicked));
        bcancel->signal_clicked().connect(sigc::mem_fun(*this, &MapWindow::buttoncancel_clicked));
        bzoomin->signal_clicked().connect(sigc::mem_fun(*this, &MapWindow::buttonzoomin_clicked));
        bzoomout->signal_clicked().connect(sigc::mem_fun(*this, &MapWindow::buttonzoomout_clicked));
        m_drawingarea->show();
        m_drawingarea->signal_cursor().connect(sigc::mem_fun(*this, &MapWindow::cursor_change));
        m_drawingarea->signal_pointer().connect(sigc::mem_fun(*this, &MapWindow::pointer_change));
        m_buttonok->set_sensitive(false);
        m_buttoninfo->set_sensitive(false);
        m_menucoord->set_sensitive(false);
        m_menucenter->signal_activate().connect(sigc::mem_fun(*this, &MapWindow::menucenter_activate));
        m_menuclear->signal_activate().connect(sigc::mem_fun(*this, &MapWindow::menuclear_activate));
        signal_delete_event().connect(sigc::mem_fun(*this, &RouteEditUi::MapWindow::on_my_delete_event));
        signal_key_press_event().connect(sigc::mem_fun(*this, &RouteEditUi::MapWindow::on_my_key_press_event));
}

RouteEditUi::MapWindow::~MapWindow()
{
        m_prefs_mapflags.disconnect();
        set_menu_coord_asynccancel();
}

void RouteEditUi::MapWindow::set_engine(Engine & eng)
{
        m_prefs_mapflags.disconnect();
        m_engine = &eng;
        m_drawingarea->set_engine(eng);
        m_drawingarea->set_map_scale(m_engine->get_prefs().mapscale);
        mapflags_change(m_engine->get_prefs().mapflags);
        m_prefs_mapflags = m_engine->get_prefs().mapflags.signal_change().connect(sigc::mem_fun(*this, &RouteEditUi::MapWindow::mapflags_change));

}

void RouteEditUi::MapWindow::mapflags_change(int f)
{
        *m_drawingarea = (MapRenderer::DrawFlags)f;
}

void RouteEditUi::MapWindow::set_route(FPlanRoute & rt)
{
        m_route = &rt;
        m_drawingarea->set_route(rt);
}

void RouteEditUi::MapWindow::redraw_route(void)
{
        m_drawingarea->redraw_route();
}

void RouteEditUi::MapWindow::set_coord(const Point & pt, int alt)
{
        m_center = pt;
        m_drawingarea->set_center(pt, alt);
        m_drawingarea->set_cursor(pt);
}

void RouteEditUi::MapWindow::hide(void)
{
        FullscreenableWindow::hide();
        m_drawingarea->set_renderer(VectorMapArea::renderer_none);
}

void RouteEditUi::MapWindow::set_renderer(VectorMapArea::renderer_t render)
{
        m_drawingarea->set_renderer(VectorMapArea::renderer_none);
        m_renderer = render;
        double ms((m_renderer == VectorMapArea::renderer_airportdiagram) ? (double)m_engine->get_prefs().mapscaleairportdiagram : (double)m_engine->get_prefs().mapscale);
        m_drawingarea->set_map_scale(ms);
        m_drawingarea->set_renderer(render);
}

bool RouteEditUi::MapWindow::on_my_key_press_event(GdkEventKey * event)
{
        if(!event)
                return false;
        std::cerr << "key press: type " << event->type << " keyval " << event->keyval << std::endl;
        if (event->type != GDK_KEY_PRESS)
                return false;
        switch (event->keyval) {
                case GDK_KEY_Up:
                        if (m_drawingarea) {
                                m_drawingarea->pan_up();
                                return true;
                        }
                        return false;

                case GDK_KEY_Down:
                        if (m_drawingarea) {
                                m_drawingarea->pan_down();
                                return true;
                        }
                        return false;

                case GDK_KEY_Left:
                        if (m_drawingarea) {
                                m_drawingarea->pan_left();
                                return true;
                        }
                        return false;

                case GDK_KEY_Right:
                        if (m_drawingarea) {
                                m_drawingarea->pan_right();
                                return true;
                        }
                        return false;

                case GDK_KEY_F7:
                        if (m_drawingarea) {
                                m_drawingarea->zoom_in();
                                m_engine->get_prefs().mapscale = m_drawingarea->get_map_scale();
                                return true;
                        }
                        return false;

                case GDK_KEY_F8:
                        if (m_drawingarea) {
                                m_drawingarea->zoom_out();
                                m_engine->get_prefs().mapscale = m_drawingarea->get_map_scale();
                                return true;
                        }
                        return false;

                default:
                        break;
        }
        return false;
}

bool RouteEditUi::MapWindow::on_my_delete_event(GdkEventAny * event)
{
        hide();
        return true;
}

void RouteEditUi::MapWindow::buttonok_clicked(void)
{
        hide();
        Point c(m_drawingarea->get_cursor());
        signal_setcoord()(c);
}

void RouteEditUi::MapWindow::buttoninfo_clicked(void)
{
        Point c(m_drawingarea->get_cursor());
        Rect r(m_drawingarea->get_cursor_rect());
        signal_info()(c, r);
}

void RouteEditUi::MapWindow::buttoncancel_clicked(void)
{
        hide();
}

void RouteEditUi::MapWindow::buttonzoomin_clicked(void)
{
        m_drawingarea->zoom_in();
        if (m_renderer == VectorMapArea::renderer_airportdiagram)
                m_engine->get_prefs().mapscaleairportdiagram = m_drawingarea->get_map_scale();
        else
                m_engine->get_prefs().mapscale = m_drawingarea->get_map_scale();
}

void RouteEditUi::MapWindow::buttonzoomout_clicked(void)
{
        m_drawingarea->zoom_out();
        if (m_renderer == VectorMapArea::renderer_airportdiagram)
                m_engine->get_prefs().mapscaleairportdiagram = m_drawingarea->get_map_scale();
        else
                m_engine->get_prefs().mapscale = m_drawingarea->get_map_scale();
}

void RouteEditUi::MapWindow::set_menu_coord(const Point & pt)
{
        Glib::ustring str = pt.get_lat_str() + ' ' + pt.get_lon_str();
        dynamic_cast<Gtk::Label *>(m_menucoord->get_child())->set_label(str);
        if (!m_engine)
                return;
        set_menu_coord_asynccancel();
        m_async_elevation = m_engine->async_elevation_point(pt);
        if (m_async_elevation) {
                m_async_dispatch_conn = m_async_dispatch.connect(sigc::bind(sigc::mem_fun(*this, &RouteEditUi::MapWindow::set_menu_coord_asyncdone), pt));
                m_async_elevation->connect(sigc::mem_fun(*this, &RouteEditUi::MapWindow::set_menu_coord_asyncdispatchdone));
        }
}

void RouteEditUi::MapWindow::set_menu_coord_asyncdone(const Point & pt)
{
        if (m_async_elevation && m_async_elevation->is_done()) {
                if (!m_async_elevation->is_error()) {
                        Glib::ustring str = pt.get_lat_str() + ' ' + pt.get_lon_str();
                        {
                                char buf[16];
                                TopoDb30::elev_t elev(m_async_elevation->get_result());
                                if (elev == TopoDb30::nodata) {
                                        str = "?""?ft";
                                } else {
                                        if (elev == TopoDb30::ocean)
                                                elev = 0;
                                        snprintf(buf, sizeof(buf), " %dft", Point::round<int,float>(elev * Point::m_to_ft));
                                        str += buf;
                                }
                        }
                        dynamic_cast<Gtk::Label *>(m_menucoord->get_child())->set_label(str);
                }
                m_async_elevation = Glib::RefPtr<Engine::ElevationResult>();
        }
}

void RouteEditUi::MapWindow::set_menu_coord_asyncdispatchdone(void)
{
        m_async_dispatch();
}

void RouteEditUi::MapWindow::set_menu_coord_asynccancel(void)
{
        Engine::ElevationResult::cancel(m_async_elevation);
        m_async_dispatch_conn.disconnect();
}

void RouteEditUi::MapWindow::cursor_change(Point pt, VectorMapArea::cursor_validity_t valid)
{
        m_buttonok->set_sensitive(valid != VectorMapArea::cursor_invalid);
        m_buttoninfo->set_sensitive(valid != VectorMapArea::cursor_invalid);
        m_menucoord->set_sensitive(valid != VectorMapArea::cursor_invalid);
        set_menu_coord(pt);
}

void RouteEditUi::MapWindow::pointer_change(Point pt)
{
        if (m_drawingarea->is_cursor_valid())
                return;
        set_menu_coord(pt);
}

void RouteEditUi::MapWindow::menucenter_activate(void)
{
        m_drawingarea->set_center(m_drawingarea->get_cursor(), m_drawingarea->get_altitude());
}

void RouteEditUi::MapWindow::menuclear_activate(void)
{
        m_drawingarea->invalidate_cursor();
}

RouteEditUi::DistanceWindow::DistanceWindow(BaseObjectType * castitem, const Glib::RefPtr<Builder>& refxml)
        : FullscreenableWindow(castitem, refxml), m_engine(0), m_prevena(false), m_nextena(false), m_buttoncancel(0),
          m_buttonfrom(0), m_buttonto(0), m_lon(0), m_lat(0), m_distancevalue(0), m_distanceunit(0)
{
        get_widget(refxml, "distancebuttoncancel", m_buttoncancel);
        get_widget(refxml, "distancebuttonfrom", m_buttonfrom);
        get_widget(refxml, "distancebuttonto", m_buttonto);
        get_widget(refxml, "distancelon", m_lon);
        get_widget(refxml, "distancelat", m_lat);
        get_widget(refxml, "distancevalue", m_distancevalue);
        get_widget(refxml, "distanceunit", m_distanceunit);
        m_buttoncancel->signal_clicked().connect(sigc::mem_fun(*this, &DistanceWindow::buttoncancel_clicked));
        m_buttonfrom->signal_clicked().connect(sigc::mem_fun(*this, &DistanceWindow::buttonfrom_clicked));
        m_buttonto->signal_clicked().connect(sigc::mem_fun(*this, &DistanceWindow::buttonto_clicked));
        m_buttonfrom->set_sensitive(m_prevena);
        m_buttonto->set_sensitive(m_nextena);
        signal_delete_event().connect(sigc::mem_fun(*this, &RouteEditUi::DistanceWindow::on_my_delete_event));
}

RouteEditUi::DistanceWindow::~DistanceWindow()
{
}

void RouteEditUi::DistanceWindow::set_engine(Engine & eng)
{
        m_engine = &eng;
}

void RouteEditUi::DistanceWindow::set_coord(const Point & pt, const Point & ptprev, bool prevena, const Point & ptnext, bool nextena)
{
        m_point = pt;
        m_prevpoint = ptprev;
        m_nextpoint = ptnext;
        m_prevena = prevena;
        m_nextena = nextena;
        m_buttonfrom->set_sensitive(m_prevena);
        m_buttonto->set_sensitive(m_nextena);
        m_lon->set_text(m_point.get_lon_str());
        m_lat->set_text(m_point.get_lat_str());
}

bool RouteEditUi::DistanceWindow::on_my_delete_event(GdkEventAny * event)
{
        hide();
        return true;
}

void RouteEditUi::DistanceWindow::buttoncancel_clicked(void)
{
        hide();
}

void RouteEditUi::DistanceWindow::buttonfrom_clicked(void)
{
        float dist = m_distancevalue->get_value();
        if (m_distanceunit->get_active_row_number() == 1)
                dist *= Point::km_to_nmi;
        float d1 = m_point.spheric_distance_nmi(m_nextpoint);
        if (d1 < 0.001)
                return;
        dist /= d1;
        Point pdiff(m_nextpoint - m_point);
        m_point += Point((int)(pdiff.get_lon() * dist), (int)(pdiff.get_lat() * dist));
        signal_insertwpt()(m_point, true);
        hide();
}

void RouteEditUi::DistanceWindow::buttonto_clicked(void)
{
        float dist = m_distancevalue->get_value();
        if (m_distanceunit->get_active_row_number() == 1)
                dist *= Point::km_to_nmi;
        float d1 = m_point.spheric_distance_nmi(m_prevpoint);
        if (d1 < 0.001)
                return;
        dist /= d1;
        Point pdiff(m_prevpoint - m_point);
        m_point += Point((int)(pdiff.get_lon() * dist), (int)(pdiff.get_lat() * dist));
        signal_insertwpt()(m_point, false);
        hide();
}

RouteEditUi::CourseDistanceWindow::CourseDistanceWindow(BaseObjectType * castitem, const Glib::RefPtr<Builder>& refxml)
        : FullscreenableWindow(castitem, refxml), m_engine(0), m_buttoncancel(0), m_buttonfrom(0), m_buttonto(0),
          m_lon(0), m_lat(0), m_angle(0), m_headingmode(0), m_distancevalue(0), m_distanceunit(0)
{
        get_widget(refxml, "crsdistbuttoncancel", m_buttoncancel);
        get_widget(refxml, "crsdistbuttonfrom", m_buttonfrom);
        get_widget(refxml, "crsdistbuttonto", m_buttonto);
        get_widget(refxml, "crsdistlon", m_lon);
        get_widget(refxml, "crsdistlat", m_lat);
        get_widget(refxml, "crsdistangle", m_angle);
        get_widget(refxml, "crsdistheadingmode", m_headingmode);
        get_widget(refxml, "crsdistvalue", m_distancevalue);
        get_widget(refxml, "crsdistunit", m_distanceunit);
        m_buttoncancel->signal_clicked().connect(sigc::mem_fun(*this, &CourseDistanceWindow::buttoncancel_clicked));
        m_buttonfrom->signal_clicked().connect(sigc::mem_fun(*this, &CourseDistanceWindow::buttonfrom_clicked));
        m_buttonto->signal_clicked().connect(sigc::mem_fun(*this, &CourseDistanceWindow::buttonto_clicked));
        signal_delete_event().connect(sigc::mem_fun(*this, &RouteEditUi::CourseDistanceWindow::on_my_delete_event));
}

RouteEditUi::CourseDistanceWindow::~CourseDistanceWindow()
{
        setcoord_asynccancel();
}

void RouteEditUi::CourseDistanceWindow::set_engine(Engine & eng)
{
        m_engine = &eng;
}

void RouteEditUi::CourseDistanceWindow::set_coord(const Point & pt)
{
        m_point = pt;
        m_lon->set_text(m_point.get_lon_str());
        m_lat->set_text(m_point.get_lat_str());
}

bool RouteEditUi::CourseDistanceWindow::on_my_delete_event(GdkEventAny * event)
{
        hide();
        return true;
}

void RouteEditUi::CourseDistanceWindow::buttoncancel_clicked(void)
{
        hide();
}

void RouteEditUi::CourseDistanceWindow::buttonfrom_clicked(void)
{
        setcoord(0);
}

void RouteEditUi::CourseDistanceWindow::buttonto_clicked(void)
{
        setcoord(1U << 31);
}

void RouteEditUi::CourseDistanceWindow::setcoord(uint32_t angoffset)
{
        uint32_t ang = angoffset + (uint32_t)(m_angle->get_value() * Point::from_deg);
        float dist = m_distancevalue->get_value();
        if (m_distanceunit->get_active_row_number() == 1)
                dist *= Point::km_to_nmi;
        if (m_headingmode->get_active_row_number() == 1 && m_engine) {
                setcoord_asynccancel();
                m_async_elevation = m_engine->async_elevation_point(m_point);
                if (m_async_elevation) {
                        m_async_dispatch_conn = m_async_dispatch.connect(sigc::bind(sigc::mem_fun(*this, &RouteEditUi::CourseDistanceWindow::setcoord_asyncdone), m_point, ang, dist));
                        m_async_elevation->connect(sigc::mem_fun(*this, &RouteEditUi::CourseDistanceWindow::setcoord_asyncdispatchdone));
                        return;
                }
        }
        m_point = m_point.spheric_course_distance_nmi(ang * Point::to_deg, dist);
        signal_setcoord()(m_point);
        hide();
}

void RouteEditUi::CourseDistanceWindow::setcoord_asyncdone(const Point& pt, uint32_t ang, float dist)
{
        if (m_async_elevation && m_async_elevation->is_done()) {
                if (!m_async_elevation->is_error()) {
                        int elev = m_async_elevation->get_result();
                        if (elev == TopoDb30::nodata || elev == TopoDb30::ocean)
                                elev = 0;
                        WMM wmm;
                        if (wmm.compute(elev * (Point::ft_to_m * 1e-3), pt, time(0)))
                                ang += (uint32_t)(wmm.get_dec() * Point::from_deg);
                        m_point = pt.spheric_course_distance_nmi(ang * Point::to_deg, dist);
                        signal_setcoord()(m_point);
                        hide();
                }
                m_async_elevation = Glib::RefPtr<Engine::ElevationResult>();
        }
}

void RouteEditUi::CourseDistanceWindow::setcoord_asyncdispatchdone(void)
{
        m_async_dispatch();
}

void RouteEditUi::CourseDistanceWindow::setcoord_asynccancel(void)
{
        Engine::ElevationResult::cancel(m_async_elevation);
        m_async_dispatch_conn.disconnect();
}

RouteEditUi::CoordMaidenheadWindow::CoordMaidenheadWindow(BaseObjectType * castitem, const Glib::RefPtr<Builder>& refxml)
        : FullscreenableWindow(castitem, refxml), m_engine(0), m_buttoncancel(0), m_buttonok(0),
          m_lon(0), m_lat(0), m_locator(0)
{
        get_widget(refxml, "coordmaidenheadbuttoncancel", m_buttoncancel);
        get_widget(refxml, "coordmaidenheadbuttonok", m_buttonok);
        get_widget(refxml, "coordmaidenheadlon", m_lon);
        get_widget(refxml, "coordmaidenheadlat", m_lat);
        get_widget(refxml, "coordmaidenheadlocator", m_locator);
        m_buttoncancel->signal_clicked().connect(sigc::mem_fun(*this, &CoordMaidenheadWindow::buttoncancel_clicked));
        m_buttonok->signal_clicked().connect(sigc::mem_fun(*this, &CoordMaidenheadWindow::buttonok_clicked));
        signal_delete_event().connect(sigc::mem_fun(*this, &RouteEditUi::CoordMaidenheadWindow::on_my_delete_event));
}

RouteEditUi::CoordMaidenheadWindow::~CoordMaidenheadWindow()
{
}

void RouteEditUi::CoordMaidenheadWindow::set_engine(Engine& eng)
{
        m_engine = &eng;
}

void RouteEditUi::CoordMaidenheadWindow::set_coord(const Point& pt)
{
        m_point = pt;
        m_lon->set_text(m_point.get_lon_str());
        m_lat->set_text(m_point.get_lat_str());
        m_locator->set_text(m_point.get_locator(10));
}

bool RouteEditUi::CoordMaidenheadWindow::on_my_delete_event(GdkEventAny *event)
{
        hide();
        return true;
}

void RouteEditUi::CoordMaidenheadWindow::buttonok_clicked(void)
{
        hide();
	Point pt;
	pt.set_str(m_locator->get_text());
        signal_setcoord()(pt);
}

void RouteEditUi::CoordMaidenheadWindow::buttoncancel_clicked(void)
{
        hide();
}

RouteEditUi::CoordCH1903Window::CoordCH1903Window(BaseObjectType * castitem, const Glib::RefPtr<Builder>& refxml)
        : FullscreenableWindow(castitem, refxml), m_engine(0), m_buttoncancel(0), m_buttonok(0),
          m_lon(0), m_lat(0), m_x(0), m_y(0)
{
        get_widget(refxml, "coordch1903buttoncancel", m_buttoncancel);
        get_widget(refxml, "coordch1903buttonok", m_buttonok);
        get_widget(refxml, "coordch1903lon", m_lon);
        get_widget(refxml, "coordch1903lat", m_lat);
        get_widget(refxml, "coordch1903x", m_x);
        get_widget(refxml, "coordch1903y", m_y);
        m_buttoncancel->signal_clicked().connect(sigc::mem_fun(*this, &CoordCH1903Window::buttoncancel_clicked));
        m_buttonok->signal_clicked().connect(sigc::mem_fun(*this, &CoordCH1903Window::buttonok_clicked));
        signal_delete_event().connect(sigc::mem_fun(*this, &RouteEditUi::CoordCH1903Window::on_my_delete_event));
}

RouteEditUi::CoordCH1903Window::~CoordCH1903Window()
{
}

void RouteEditUi::CoordCH1903Window::set_engine(Engine & eng)
{
        m_engine = &eng;
}

void RouteEditUi::CoordCH1903Window::set_coord(const Point & pt)
{
        m_point = pt;
        m_lon->set_text(m_point.get_lon_str());
        m_lat->set_text(m_point.get_lat_str());
	float x, y;
	m_point.get_ch1903(x, y);
        char buf[16];
        snprintf(buf, sizeof(buf), "%9.2f", x);
        m_x->set_text(buf);
        snprintf(buf, sizeof(buf), "%9.2f", y);
        m_y->set_text(buf);
}

bool RouteEditUi::CoordCH1903Window::on_my_delete_event(GdkEventAny * event)
{
        hide();
        return true;
}

void RouteEditUi::CoordCH1903Window::buttonok_clicked(void)
{
	m_point.set_ch1903(strtod(m_x->get_text().c_str(), NULL),
			   strtod(m_y->get_text().c_str(), NULL));
        signal_setcoord()(m_point);
        hide();
}

void RouteEditUi::CoordCH1903Window::buttoncancel_clicked(void)
{
        hide();
}

RouteEditUi::InformationWindow::InformationWindow(BaseObjectType * castitem, const Glib::RefPtr<Builder>& refxml)
        : FullscreenableWindow(castitem, refxml), m_textview(0)
{
        Gtk::Button *m_buttonclose = 0;
        get_widget(refxml, "infowindowbuttonclose", m_buttonclose);
        get_widget(refxml, "infowindowtext", m_textview);
        m_buttonclose->signal_clicked().connect(sigc::mem_fun(*this, &InformationWindow::buttonclose_clicked));
        signal_hide().connect(sigc::mem_fun(*this, &RouteEditUi::InformationWindow::on_my_hide));
        signal_delete_event().connect(sigc::mem_fun(*this, &RouteEditUi::InformationWindow::on_my_delete_event));
}

RouteEditUi::InformationWindow::~InformationWindow()
{
}

void RouteEditUi::InformationWindow::set_text(const Glib::ustring & str)
{
        Glib::RefPtr<Gtk::TextBuffer> txtbuf(m_textview->get_buffer());
        txtbuf->begin_user_action();
        txtbuf->erase(txtbuf->begin(), txtbuf->end());
        txtbuf->insert(txtbuf->end(), str);
        txtbuf->end_user_action();
}

void RouteEditUi::InformationWindow::clear_text(void)
{
        Glib::RefPtr<Gtk::TextBuffer> txtbuf(m_textview->get_buffer());
        txtbuf->begin_user_action();
        txtbuf->erase(txtbuf->begin(), txtbuf->end());
        txtbuf->end_user_action();
}

void RouteEditUi::InformationWindow::append_text(const Glib::ustring & str)
{
        Glib::RefPtr<Gtk::TextBuffer> txtbuf(m_textview->get_buffer());
        txtbuf->begin_user_action();
        txtbuf->insert(txtbuf->end(), str);
        txtbuf->end_user_action();
}

void RouteEditUi::InformationWindow::buttonclose_clicked(void)
{
        hide();
}

void RouteEditUi::InformationWindow::on_my_hide(void)
{
        Glib::RefPtr<Gtk::TextBuffer> txtbuf(m_textview->get_buffer());
        txtbuf->begin_user_action();
        txtbuf->erase(txtbuf->begin(), txtbuf->end());
        txtbuf->end_user_action();
}

bool RouteEditUi::InformationWindow::on_my_delete_event(GdkEventAny * event)
{
        hide();
        return true;
}

RouteEditUi::SunriseSunsetWindow::SunriseSunsetModelColumns::SunriseSunsetModelColumns(void)
{
        add(m_col_location);
        add(m_col_sunrise);
        add(m_col_sunset);
        add(m_col_point);
}

RouteEditUi::SunriseSunsetWindow::SunriseSunsetWindow(BaseObjectType * castitem, const Glib::RefPtr<Builder>& refxml)
        : FullscreenableWindow(castitem, refxml), m_route(0), m_treeview(0), m_calendar(0)
{
        get_widget(refxml, "sunrisesunsettreeview", m_treeview);
        get_widget(refxml, "sunrisesunsetcalendar", m_calendar);
        Gtk::Button *m_buttonclose, *m_buttontoday;
        get_widget(refxml, "sunrisesunsetbuttonclose", m_buttonclose);
        get_widget(refxml, "sunrisesunsetbuttontoday", m_buttontoday);
        m_buttonclose->signal_clicked().connect(sigc::mem_fun(*this, &SunriseSunsetWindow::buttonclose_clicked));
        m_buttontoday->signal_clicked().connect(sigc::mem_fun(*this, &SunriseSunsetWindow::buttontoday_clicked));
        // Waypoint List
        m_liststore = Gtk::ListStore::create(m_listcolumns);
        m_treeview->set_model(m_liststore);
        m_treeview->append_column(_("Location"), m_listcolumns.m_col_location);
        m_treeview->append_column(_("Sunrise"), m_listcolumns.m_col_sunrise);
        m_treeview->append_column(_("Sunset"), m_listcolumns.m_col_sunset);
        for (unsigned int i = 0; i < 3; ++i) {
                Gtk::TreeViewColumn *col = m_treeview->get_column(i);
                if (!col)
                        continue;
                col->set_resizable(true);
                col->set_reorderable(true);
        }
        // update signals
        m_calendar->signal_day_selected().connect(sigc::mem_fun(*this, &SunriseSunsetWindow::recompute_sunrisesunset));
        signal_hide().connect(sigc::mem_fun(*this, &RouteEditUi::SunriseSunsetWindow::on_my_hide));
        signal_show().connect(sigc::mem_fun(*this, &RouteEditUi::SunriseSunsetWindow::on_my_show));
        signal_delete_event().connect(sigc::mem_fun(*this, &RouteEditUi::SunriseSunsetWindow::on_my_delete_event));
}

RouteEditUi::SunriseSunsetWindow::~SunriseSunsetWindow()
{
}

void RouteEditUi::SunriseSunsetWindow::clear_list(void)
{
        Gtk::TreeIter iter = m_liststore->children().begin();
        while (iter)
                iter = m_liststore->erase(iter);
}

void RouteEditUi::SunriseSunsetWindow::fill_list(void)
{
        clear_list();
        if (!m_route)
                return;
        for (unsigned int i = 0; i < m_route->get_nrwpt(); i++) {
                const FPlanWaypoint& wpt((*m_route)[i]);
                Gtk::TreeModel::Row row(*(m_liststore->append()));
                row[m_listcolumns.m_col_location] = wpt.get_icao_name();
                row[m_listcolumns.m_col_point] = wpt.get_coord();
        }
        recompute_sunrisesunset();
}

void RouteEditUi::SunriseSunsetWindow::on_my_hide(void)
{
        clear_list();
}

void RouteEditUi::SunriseSunsetWindow::on_my_show(void)
{
        fill_list();
}

bool RouteEditUi::SunriseSunsetWindow::on_my_delete_event(GdkEventAny * event)
{
        hide();
        return true;
}

void RouteEditUi::SunriseSunsetWindow::buttonclose_clicked(void)
{
        hide();
}

void RouteEditUi::SunriseSunsetWindow::buttontoday_clicked(void)
{
        time_t t = time(0);
        struct tm tm;
        if (!gmtime_r(&t, &tm))
		return;
        m_calendar->select_month(tm.tm_mon, tm.tm_year + 1900);
        m_calendar->select_day(tm.tm_mday);
}

void RouteEditUi::SunriseSunsetWindow::recompute_sunrisesunset(void)
{
        guint year, month, day;
        m_calendar->get_date(year, month, day);
        //std::cerr << "day " << day << " month " << month << " year " << year << std::endl;
        month++;
        for (Gtk::TreeIter iter = m_liststore->children().begin(); iter; iter++) {
                Gtk::TreeModel::Row row(*iter);
                double sr, ss;
                int r = SunriseSunset::civil_twilight(year, month, day, row[m_listcolumns.m_col_point], sr, ss);
                if (r) {
                        row[m_listcolumns.m_col_sunrise] = _("always");
                        row[m_listcolumns.m_col_sunset] = (r < 0) ? _("night") : _("day");
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
                row[m_listcolumns.m_col_sunrise] = buf;
                h = (int)ss;
                m = (int)((ss - h) * 60 + 0.5);
                snprintf(buf, sizeof(buf), "%02d:%02d", h, m);
                row[m_listcolumns.m_col_sunset] = buf;
        }
}

void RouteEditUi::SunriseSunsetWindow::update_route(void)
{
        if (!get_visible())
                return;
        fill_list();
}


RouteEditUi::IcaoFplImportWindow::IcaoFplImportWindow(BaseObjectType *castitem, const Glib::RefPtr<Builder>& refxml)
        : FullscreenableWindow(castitem, refxml)
{
        get_widget(refxml, "icaofplimporttextview", m_icaofplimporttextview);
        get_widget(refxml, "icaofplimporterrmsgtextview", m_icaofplimporterrmsgtextview);
        get_widget(refxml, "icaofplimporterrmsgexpander", m_icaofplimporterrmsgexpander);
        Gtk::Button *m_buttoncancel, *m_buttonok;
        get_widget(refxml, "icaofplimportbuttoncancel", m_buttoncancel);
        get_widget(refxml, "icaofplimportbuttonok", m_buttonok);
        m_buttoncancel->signal_clicked().connect(sigc::mem_fun(*this, &IcaoFplImportWindow::buttoncancel_clicked));
        m_buttonok->signal_clicked().connect(sigc::mem_fun(*this, &IcaoFplImportWindow::buttonok_clicked));
        // update signals
        signal_hide().connect(sigc::mem_fun(*this, &RouteEditUi::IcaoFplImportWindow::on_my_hide));
        signal_show().connect(sigc::mem_fun(*this, &RouteEditUi::IcaoFplImportWindow::on_my_show));
        signal_delete_event().connect(sigc::mem_fun(*this, &RouteEditUi::IcaoFplImportWindow::on_my_delete_event));
}

RouteEditUi::IcaoFplImportWindow::~IcaoFplImportWindow()
{
}

void RouteEditUi::IcaoFplImportWindow::on_my_hide(void)
{
        Glib::RefPtr<Gtk::TextBuffer> txtbuf(m_icaofplimporterrmsgtextview->get_buffer());
        txtbuf->begin_user_action();
        txtbuf->erase(txtbuf->begin(), txtbuf->end());
        txtbuf->end_user_action();
        m_icaofplimporterrmsgexpander->set_expanded(false);
}

void RouteEditUi::IcaoFplImportWindow::on_my_show(void)
{
}

bool RouteEditUi::IcaoFplImportWindow::on_my_delete_event(GdkEventAny* event)
{
        buttoncancel_clicked();
        return true;
}

void RouteEditUi::IcaoFplImportWindow::buttonok_clicked(void)
{
	Glib::ustring errmsg;
	{
		Glib::RefPtr<Gtk::TextBuffer> txtbuf(m_icaofplimporttextview->get_buffer());
		errmsg = txtbuf->get_text();
		errmsg = m_signal_fplimport(errmsg);
	}
	if (errmsg.empty()) {
		buttoncancel_clicked();
		return;
	}
	{
		Glib::RefPtr<Gtk::TextBuffer> txtbuf(m_icaofplimporterrmsgtextview->get_buffer());
		txtbuf->begin_user_action();
		txtbuf->set_text(errmsg);
		txtbuf->end_user_action();
		m_icaofplimporterrmsgexpander->set_expanded(true);
	}
}

void RouteEditUi::IcaoFplImportWindow::buttoncancel_clicked(void)
{
        hide();
}

RouteEditUi::RouteEditUi(Engine& eng, Glib::RefPtr<PrefsWindow>& prefswindow)
        : m_engine(eng), m_route(m_engine.get_fplan_db()), m_prefswindow(prefswindow), m_routeeditwindow(0), m_routeeditnotebook(0), m_routemenu(0),
          m_newroute(0), m_duplicateroute(0), m_deleteroute(0), m_reverseroute(0), m_importicaofplroute(0), m_exporticaofplroute(0),
	  m_sunrisesunsetroute(0), m_startnavigate(0), m_waypointmenu(0), m_insertwaypoint(0),
          m_swapwaypoint(0), m_deletewaypoint(0), m_waypointdistance(0), m_waypointcoursedistance(0),
          m_waypointstraighten(0), m_waypointcoordch1903(0), m_waypointcoordmaidenhead(0), m_preferencesitem(0), m_aboutitem(0),
	  m_routelist(0), m_waypointlist(0), m_wpticao(0), m_wptname(0), m_wptpathcode(0), m_wptpathname(0),
	  m_wptlonsign(0), m_wptlondeg(0), m_wptlonmin(0), m_wptlatsign(0), m_wptlatdeg(0), m_wptlatmin(0),
	  m_wptalt(0), m_wptaltunit(0), m_wptfreq(0), m_wptfrequnit(0), m_wptifr(0), m_wptwinddir(0), m_wptwindspeed(0),
	  m_wptqff(0), m_wptoat(0), m_wptnote(0), m_wptprev(0), m_wptnext(0), m_wptfind(0), m_wptfindnearest(0),
	  m_wptmaps(0), m_wptterrain(0), m_wptairportdiagram(0), m_routenote(0), m_widgetchange(false), m_editactive(false),
	  m_wxoffblock(0), m_wxoffblockh(0), m_wxoffblockm(0), m_wxoffblocks(0), m_wxtaxitimes(0),
	  m_wxtaxitimem(0), m_wxcruisealt(0), m_wxwinddir(0), m_wxwindspeed(0), m_wxisaoffs(0), m_wxqff(0),
	  m_wxsetwind(0), m_wxsetisaoffs(0), m_wxsetqff(0), m_wxgetweather(0), m_wxflighttime(0),
          m_findnamewindow(0), m_findnearestwindow(0), m_confirmdeletewindow(0), m_mapwindow(0), m_aboutdialog(0),
          m_distancewindow(0), m_coursedistwindow(0), m_maidenheadwindow(0), m_ch1903window(0), m_infowindow(0),
          m_sunrisesunsetwindow(0), m_icaofplimportwindow(0)
{
        m_refxml = load_glade_file("routeedit" UIEXT);
        get_widget_derived(m_refxml, "routeeditwindow", m_routeeditwindow);
        get_widget(m_refxml, "routemenu", m_routemenu);
        get_widget(m_refxml, "newroute", m_newroute);
        get_widget(m_refxml, "duplicateroute", m_duplicateroute);
        get_widget(m_refxml, "deleteroute", m_deleteroute);
        get_widget(m_refxml, "reverseroute", m_reverseroute);
        get_widget(m_refxml, "importicaofplroute", m_importicaofplroute);
        get_widget(m_refxml, "exporticaofplroute", m_exporticaofplroute);
        get_widget(m_refxml, "sunrisesunsetroute", m_sunrisesunsetroute);
        get_widget(m_refxml, "startnavigate", m_startnavigate);
        get_widget(m_refxml, "waypointmenu", m_waypointmenu);
        get_widget(m_refxml, "insertwaypoint", m_insertwaypoint);
        get_widget(m_refxml, "swapwaypoint", m_swapwaypoint);
        get_widget(m_refxml, "deletewaypoint", m_deletewaypoint);
        get_widget(m_refxml, "waypointdistance", m_waypointdistance);
        get_widget(m_refxml, "waypointcoursedistance", m_waypointcoursedistance);
        get_widget(m_refxml, "waypointstraighten", m_waypointstraighten);
        get_widget(m_refxml, "waypointcoordch1903", m_waypointcoordch1903);
        get_widget(m_refxml, "waypointcoordmaidenhead", m_waypointcoordmaidenhead);
        get_widget(m_refxml, "preferencesitem", m_preferencesitem);
        get_widget(m_refxml, "aboutitem", m_aboutitem);
        get_widget(m_refxml, "routelist", m_routelist);
        get_widget(m_refxml, "waypointlist", m_waypointlist);
        get_widget(m_refxml, "wpticao", m_wpticao);
        get_widget(m_refxml, "wptname", m_wptname);
        get_widget(m_refxml, "wptpathcode", m_wptpathcode);
        get_widget(m_refxml, "wptpathname", m_wptpathname);
        get_widget(m_refxml, "wptlonsign", m_wptlonsign);
        get_widget(m_refxml, "wptlondeg", m_wptlondeg);
        get_widget(m_refxml, "wptlonmin", m_wptlonmin);
        get_widget(m_refxml, "wptlatsign", m_wptlatsign);
        get_widget(m_refxml, "wptlatdeg", m_wptlatdeg);
        get_widget(m_refxml, "wptlatmin", m_wptlatmin);
        get_widget(m_refxml, "wptalt", m_wptalt);
        get_widget(m_refxml, "wptaltunit", m_wptaltunit);
        get_widget(m_refxml, "wptfreq", m_wptfreq);
        get_widget(m_refxml, "wptfrequnit", m_wptfrequnit);
        get_widget(m_refxml, "wptifr", m_wptifr);
	get_widget(m_refxml, "wptwinddir", m_wptwinddir);
	get_widget(m_refxml, "wptwindspeed", m_wptwindspeed);
 	get_widget(m_refxml, "wptqff", m_wptqff);
 	get_widget(m_refxml, "wptoat", m_wptoat);
        get_widget(m_refxml, "wptnote", m_wptnote);
        get_widget(m_refxml, "wptprev", m_wptprev);
        get_widget(m_refxml, "wptnext", m_wptnext);
        get_widget(m_refxml, "wptfind", m_wptfind);
        get_widget(m_refxml, "wptfindnearest", m_wptfindnearest);
        get_widget(m_refxml, "wptmaps", m_wptmaps);
        get_widget(m_refxml, "wptterrain", m_wptterrain);
        get_widget(m_refxml, "wptairportdiagram", m_wptairportdiagram);
        get_widget(m_refxml, "routenote", m_routenote);
	get_widget(m_refxml, "wxoffblock", m_wxoffblock);
	get_widget(m_refxml, "wxoffblockh", m_wxoffblockh);
	get_widget(m_refxml, "wxoffblockm", m_wxoffblockm);
	get_widget(m_refxml, "wxoffblocks", m_wxoffblocks);
	get_widget(m_refxml, "wxtaxitimes", m_wxtaxitimes);
	get_widget(m_refxml, "wxtaxitimem", m_wxtaxitimem);
	get_widget(m_refxml, "wxcruisealt", m_wxcruisealt);
	get_widget(m_refxml, "wxwinddir", m_wxwinddir);
	get_widget(m_refxml, "wxwindspeed", m_wxwindspeed);
	get_widget(m_refxml, "wxisaoffs", m_wxisaoffs);
	get_widget(m_refxml, "wxqff", m_wxqff);
        get_widget(m_refxml, "wxsetwind", m_wxsetwind);
        get_widget(m_refxml, "wxsetisaoffs", m_wxsetisaoffs);
        get_widget(m_refxml, "wxsetqff", m_wxsetqff);
	get_widget(m_refxml, "wxgetweather", m_wxgetweather);
	get_widget(m_refxml, "wxflighttime", m_wxflighttime);
        // Create Tree Model
        m_routeliststore = Gtk::TreeStore::create(m_routelistcolumns);
        m_routelist->set_model(m_routeliststore);
        m_routelist->append_column(_("From"), m_routelistcolumns.m_col_from);
        m_routelist->append_column(_("To"), m_routelistcolumns.m_col_to);
        //m_routelist->append_column_numeric(_("Dist"), m_routelistcolumns.m_col_dist, "%6.1f");
	Gtk::CellRendererText *dist_renderer = Gtk::manage(new Gtk::CellRendererText());
        Gtk::TreeViewColumn *dist_column = m_routelist->get_column(m_routelist->append_column(_("Dist"), *dist_renderer) - 1);
        dist_column->set_cell_data_func(*dist_renderer, sigc::mem_fun(*this, &RouteEditUi::distance_cell_func));
        for (unsigned int i = 0; i < 3; ++i) {
                Gtk::TreeViewColumn *col = m_routelist->get_column(i);
                if (!col)
                        continue;
                col->set_resizable(true);
                const Gtk::TreeModelColumnBase *cols[3] = { &m_routelistcolumns.m_col_from, &m_routelistcolumns.m_col_to, &m_routelistcolumns.m_col_dist };
                col->set_sort_column(*cols[i]);
                Gtk::TreeSortable::SlotCompare sort_from(sigc::mem_fun(*this, &RouteEditUi::routelist_sort_from));
                Gtk::TreeSortable::SlotCompare sort_to(sigc::mem_fun(*this, &RouteEditUi::routelist_sort_to));
                Gtk::TreeSortable::SlotCompare sort_dist(sigc::mem_fun(*this, &RouteEditUi::routelist_sort_dist));
                const Gtk::TreeSortable::SlotCompare *colsort[3] = { &sort_from, &sort_to, &sort_dist };
                if (colsort[i])
                        m_routeliststore->set_sort_func(*cols[i], *colsort[i]);
                col->set_expand(i <= 1);
        }
        //m_routelist->columns_autosize();
        m_routelist->set_headers_clickable();
        // selection
        Glib::RefPtr<Gtk::TreeSelection> routelist_selection = m_routelist->get_selection();
        routelist_selection->set_mode(Gtk::SELECTION_SINGLE);
        routelist_selection->signal_changed().connect(sigc::mem_fun(*this, &RouteEditUi::routelist_selection_changed));
        routelist_selection->set_select_function(sigc::mem_fun(*this, &RouteEditUi::routelist_select_function));
        // Waypoint List
        m_waypointliststore = Gtk::ListStore::create(m_waypointlistcolumns);
        m_waypointlist->set_model(m_waypointliststore);
        m_waypointlist->append_column(_("ICAO"), m_waypointlistcolumns.m_col_icao);
        m_waypointlist->append_column(_("Name"), m_waypointlistcolumns.m_col_name);
        m_waypointlist->append_column(_("Freq"), m_waypointlistcolumns.m_col_freq);
        m_waypointlist->append_column(_("Time"), m_waypointlistcolumns.m_col_time);
        m_waypointlist->append_column(_("Lon"), m_waypointlistcolumns.m_col_lon);
        m_waypointlist->append_column(_("Lat"), m_waypointlistcolumns.m_col_lat);
        m_waypointlist->append_column(_("Alt"), m_waypointlistcolumns.m_col_alt);
        m_waypointlist->append_column(_("MT"), m_waypointlistcolumns.m_col_mt);
        m_waypointlist->append_column(_("TT"), m_waypointlistcolumns.m_col_tt);
        m_waypointlist->append_column(_("Dist"), m_waypointlistcolumns.m_col_dist);
        m_waypointlist->append_column(_("VertS"), m_waypointlistcolumns.m_col_vertspeed);
        m_waypointlist->append_column(_("TrueAlt"), m_waypointlistcolumns.m_col_truealt);
        m_waypointlist->append_column(_("Path"), m_waypointlistcolumns.m_col_pathname);
        m_waypointlist->append_column(_("Wind"), m_waypointlistcolumns.m_col_wind);
        m_waypointlist->append_column(_("QFF"), m_waypointlistcolumns.m_col_qff);
        m_waypointlist->append_column(_("OAT"), m_waypointlistcolumns.m_col_oat);
        for (unsigned int i = 0; i < 16; ++i) {
                Gtk::TreeViewColumn *col = m_waypointlist->get_column(i);
                if (!col)
                        continue;
                col->set_resizable(true);
                col->set_reorderable(true);
                col->set_expand(i == 1);
        }
        for (unsigned int i = 6; i < 16; ++i) {
		if (i == 12)
			continue;
                Gtk::CellRenderer *renderer = m_waypointlist->get_column_cell_renderer(i);
                if (!renderer)
                        continue;
                renderer->set_property("xalign", 1.0);
        }
        // selection
        Glib::RefPtr<Gtk::TreeSelection> waypointlist_selection = m_waypointlist->get_selection();
        waypointlist_selection->set_mode(Gtk::SELECTION_SINGLE);
        waypointlist_selection->signal_changed().connect(sigc::mem_fun(*this, &RouteEditUi::waypointlist_selection_changed));
        waypointlist_selection->set_select_function(sigc::mem_fun(*this, &RouteEditUi::waypointlist_select_function));
        waypointlist_disable();
        // textviews
        m_wptnotebuffer = Gtk::TextBuffer::create();
        m_wptnote->set_buffer(m_wptnotebuffer);
        m_routenotebuffer = Gtk::TextBuffer::create();
        m_routenote->set_buffer(m_routenotebuffer);
        // wpt signals
        m_wpticao->signal_changed().connect(sigc::mem_fun(*this, &RouteEditUi::widget_changed));
        m_wpticao->signal_focus_out_event().connect(sigc::mem_fun(*this, &RouteEditUi::wpticao_focus_out_event));
        m_wpticao->signal_focus_in_event().connect(sigc::bind(sigc::mem_fun(*this, &RouteEditUi::wptentry_focus_in_event), m_wpticao));
        m_wptname->signal_changed().connect(sigc::mem_fun(*this, &RouteEditUi::widget_changed));
        m_wptname->signal_focus_out_event().connect(sigc::mem_fun(*this, &RouteEditUi::wptname_focus_out_event));
        m_wptname->signal_focus_in_event().connect(sigc::bind(sigc::mem_fun(*this, &RouteEditUi::wptentry_focus_in_event), m_wptname));
        m_wptpathname->signal_changed().connect(sigc::mem_fun(*this, &RouteEditUi::widget_changed));
        m_wptpathname->signal_focus_out_event().connect(sigc::mem_fun(*this, &RouteEditUi::wptpathname_focus_out_event));
        m_wptpathname->signal_focus_in_event().connect(sigc::bind(sigc::mem_fun(*this, &RouteEditUi::wptentry_focus_in_event), m_wptpathname));
        m_wptpathcode->signal_changed().connect(sigc::mem_fun(*this, &RouteEditUi::wptpathcode_changed));
        m_wptlonsign->signal_changed().connect(sigc::mem_fun(*this, &RouteEditUi::wptlonsign_changed));
        m_wptlondeg->signal_changed().connect(sigc::mem_fun(*this, &RouteEditUi::widget_changed));
        m_wptlondeg->signal_focus_out_event().connect(sigc::mem_fun(*this, &RouteEditUi::wptlon_focus_out_event));
        m_wptlondeg->signal_focus_in_event().connect(sigc::bind(sigc::mem_fun(*this, &RouteEditUi::wptentry_focus_in_event), m_wptlondeg));
        m_wptlonmin->signal_changed().connect(sigc::mem_fun(*this, &RouteEditUi::widget_changed));
        m_wptlonmin->signal_focus_out_event().connect(sigc::mem_fun(*this, &RouteEditUi::wptlon_focus_out_event));
        m_wptlonmin->signal_focus_in_event().connect(sigc::bind(sigc::mem_fun(*this, &RouteEditUi::wptentry_focus_in_event), m_wptlonmin));
        m_wptlatsign->signal_changed().connect(sigc::mem_fun(*this, &RouteEditUi::wptlatsign_changed));
        m_wptlatdeg->signal_changed().connect(sigc::mem_fun(*this, &RouteEditUi::widget_changed));
        m_wptlatdeg->signal_focus_out_event().connect(sigc::mem_fun(*this, &RouteEditUi::wptlat_focus_out_event));
        m_wptlatdeg->signal_focus_in_event().connect(sigc::bind(sigc::mem_fun(*this, &RouteEditUi::wptentry_focus_in_event), m_wptlatdeg));
        m_wptlatmin->signal_changed().connect(sigc::mem_fun(*this, &RouteEditUi::widget_changed));
        m_wptlatmin->signal_focus_out_event().connect(sigc::mem_fun(*this, &RouteEditUi::wptlat_focus_out_event));
        m_wptlatmin->signal_focus_in_event().connect(sigc::bind(sigc::mem_fun(*this, &RouteEditUi::wptentry_focus_in_event), m_wptlatmin));
        m_wptalt->signal_changed().connect(sigc::mem_fun(*this, &RouteEditUi::widget_changed));
        m_wptalt->signal_focus_out_event().connect(sigc::mem_fun(*this, &RouteEditUi::wptalt_focus_out_event));
        m_wptalt->signal_focus_in_event().connect(sigc::bind(sigc::mem_fun(*this, &RouteEditUi::wptentry_focus_in_event), m_wptalt));
	m_wptalt->set_alignment(1.0);
        m_wptaltunit->signal_changed().connect(sigc::mem_fun(*this, &RouteEditUi::wptaltunit_changed));
        m_wptfreq->signal_changed().connect(sigc::mem_fun(*this, &RouteEditUi::widget_changed));
        m_wptfreq->signal_focus_out_event().connect(sigc::mem_fun(*this, &RouteEditUi::wptfreq_focus_out_event));
        m_wptfreq->signal_focus_in_event().connect(sigc::bind(sigc::mem_fun(*this, &RouteEditUi::wptentry_focus_in_event), m_wptfreq));
	m_wptfreq->set_alignment(1.0);
	m_wptifrconn = m_wptifr->signal_toggled().connect(sigc::mem_fun(*this, &RouteEditUi::wptifr_toggled));
	m_wptwinddir->signal_changed().connect(sigc::mem_fun(*this, &RouteEditUi::widget_changed));
	m_wptwinddir->signal_focus_out_event().connect(sigc::mem_fun(*this, &RouteEditUi::wptwinddir_focus_out_event));
	m_wptwinddir->signal_focus_in_event().connect(sigc::bind(sigc::mem_fun(*this, &RouteEditUi::wptentry_focus_in_event), m_wptwinddir));
	m_wptwindspeed->signal_changed().connect(sigc::mem_fun(*this, &RouteEditUi::widget_changed));
	m_wptwindspeed->signal_focus_out_event().connect(sigc::mem_fun(*this, &RouteEditUi::wptwindspeed_focus_out_event));
	m_wptwindspeed->signal_focus_in_event().connect(sigc::bind(sigc::mem_fun(*this, &RouteEditUi::wptentry_focus_in_event), m_wptwindspeed));
	m_wptqff->signal_changed().connect(sigc::mem_fun(*this, &RouteEditUi::widget_changed));
	m_wptqff->signal_focus_out_event().connect(sigc::mem_fun(*this, &RouteEditUi::wptqff_focus_out_event));
	m_wptqff->signal_focus_in_event().connect(sigc::bind(sigc::mem_fun(*this, &RouteEditUi::wptentry_focus_in_event), m_wptqff));
	m_wptoat->signal_changed().connect(sigc::mem_fun(*this, &RouteEditUi::widget_changed));
	m_wptoat->signal_focus_out_event().connect(sigc::mem_fun(*this, &RouteEditUi::wptoat_focus_out_event));
	m_wptoat->signal_focus_in_event().connect(sigc::bind(sigc::mem_fun(*this, &RouteEditUi::wptentry_focus_in_event), m_wptoat));
        m_wptnotebuffer->signal_changed().connect(sigc::mem_fun(*this, &RouteEditUi::widget_changed));
        m_wptnote->signal_focus_out_event().connect(sigc::mem_fun(*this, &RouteEditUi::wptnote_focus_out_event));
        m_wptprev->signal_clicked().connect(sigc::mem_fun(*this, &RouteEditUi::wptprev_clicked));
        m_wptnext->signal_clicked().connect(sigc::mem_fun(*this, &RouteEditUi::wptnext_clicked));
        m_wptfind->signal_clicked().connect(sigc::mem_fun(*this, &RouteEditUi::wptfind_clicked));
        m_wptfindnearest->signal_clicked().connect(sigc::mem_fun(*this, &RouteEditUi::wptfindnearest_clicked));
        m_wptmaps->signal_clicked().connect(sigc::mem_fun(*this, &RouteEditUi::wptmaps_clicked));
        m_wptterrain->signal_clicked().connect(sigc::mem_fun(*this, &RouteEditUi::wptterrain_clicked));
        m_wptairportdiagram->signal_clicked().connect(sigc::mem_fun(*this, &RouteEditUi::wptairportdiagram_clicked));
        m_routenotebuffer->signal_changed().connect(sigc::mem_fun(*this, &RouteEditUi::widget_changed));
        m_routenote->signal_focus_out_event().connect(sigc::mem_fun(*this, &RouteEditUi::routenote_focus_out_event));
        get_widget(m_refxml, "routeeditnotebook", m_routeeditnotebook);
        m_routeeditnotebook->signal_switch_page().connect(sigc::hide<0>(sigc::mem_fun(*this, &RouteEditUi::routeeditnotebook_switch_page)));
        m_newroute->signal_activate().connect(sigc::mem_fun(*this, &RouteEditUi::menu_newroute));
        m_duplicateroute->signal_activate().connect(sigc::mem_fun(*this, &RouteEditUi::menu_duplicateroute));
        m_deleteroute->signal_activate().connect(sigc::mem_fun(*this, &RouteEditUi::menu_deleteroute));
        m_reverseroute->signal_activate().connect(sigc::mem_fun(*this, &RouteEditUi::menu_reverseroute));
	m_importicaofplroute->signal_activate().connect(sigc::mem_fun(*this, &RouteEditUi::menu_importicaofplroute));
	m_exporticaofplroute->signal_activate().connect(sigc::mem_fun(*this, &RouteEditUi::menu_exporticaofplroute));
        m_sunrisesunsetroute->signal_activate().connect(sigc::mem_fun(*this, &RouteEditUi::menu_sunrisesunset));
        m_startnavigate->signal_activate().connect(sigc::mem_fun(*this, &RouteEditUi::menu_startnavigate));
        m_waypointmenu->set_sensitive(false);
        m_insertwaypoint->signal_activate().connect(sigc::mem_fun(*this, &RouteEditUi::menu_insertwaypoint));
        m_swapwaypoint->signal_activate().connect(sigc::mem_fun(*this, &RouteEditUi::menu_swapwaypoint));
        m_deletewaypoint->signal_activate().connect(sigc::mem_fun(*this, &RouteEditUi::menu_deletewaypoint));
        m_waypointdistance->signal_activate().connect(sigc::mem_fun(*this, &RouteEditUi::menu_waypointdistance));
        m_waypointcoursedistance->signal_activate().connect(sigc::mem_fun(*this, &RouteEditUi::menu_waypointcoursedistance));
        m_waypointstraighten->signal_activate().connect(sigc::mem_fun(*this, &RouteEditUi::menu_waypointstraighten));
        m_waypointcoordch1903->signal_activate().connect(sigc::mem_fun(*this, &RouteEditUi::menu_waypointcoordch1903));
        m_waypointcoordmaidenhead->signal_activate().connect(sigc::mem_fun(*this, &RouteEditUi::menu_waypointcoordmaidenhead));
        m_preferencesitem->signal_activate().connect(sigc::mem_fun<void,PrefsWindow>(m_prefswindow.operator->(), &PrefsWindow::present));
	// WX Signals
	if (m_wxoffblock)
                m_connwxoffblock = m_wxoffblock->signal_day_selected().connect(sigc::mem_fun(*this, &RouteEditUi::wx_deptime_changed));
        if (m_wxoffblockh)
                m_connwxoffblockh = m_wxoffblockh->signal_value_changed().connect(sigc::mem_fun(*this, &RouteEditUi::wx_deptime_changed));
        if (m_wxoffblockm)
                m_connwxoffblockm = m_wxoffblockm->signal_value_changed().connect(sigc::mem_fun(*this, &RouteEditUi::wx_deptime_changed));
        if (m_wxoffblocks)
                m_connwxoffblocks = m_wxoffblocks->signal_value_changed().connect(sigc::mem_fun(*this, &RouteEditUi::wx_deptime_changed));
        if (m_wxtaxitimem)
                m_connwxtaxitimem = m_wxtaxitimem->signal_value_changed().connect(sigc::mem_fun(*this, &RouteEditUi::wx_deptime_changed));
        if (m_wxtaxitimes)
                m_connwxtaxitimes = m_wxtaxitimes->signal_value_changed().connect(sigc::mem_fun(*this, &RouteEditUi::wx_deptime_changed));
        if (m_wxsetwind)
                m_wxsetwind->signal_clicked().connect(sigc::mem_fun(*this, &RouteEditUi::wx_setwind_clicked));
        if (m_wxsetisaoffs)
                m_wxsetisaoffs->signal_clicked().connect(sigc::mem_fun(*this, &RouteEditUi::wx_setisaoffs_clicked));
        if (m_wxsetqff)
                m_wxsetqff->signal_clicked().connect(sigc::mem_fun(*this, &RouteEditUi::wx_setqff_clicked));
        if (m_wxgetweather)
                m_wxgetweather->signal_clicked().connect(sigc::mem_fun(*this, &RouteEditUi::wx_getnwx_clicked));
        if (m_wxcruisealt)
                m_connwxcruisealt = m_wxcruisealt->signal_value_changed().connect(sigc::mem_fun(*this, &RouteEditUi::wx_cruisealt_changed));
        // Find Windows
        get_widget_derived(m_refxml, "findnamewindow", m_findnamewindow);
        m_findnamewindow->set_engine(m_engine);
        m_findnamewindow->signal_setwaypoint().connect(sigc::mem_fun(*this, &RouteEditUi::set_current_waypoint));
        m_findnamewindow->signal_insertwaypoint().connect(sigc::mem_fun(*this, &RouteEditUi::menu_insertwaypoint));
        m_findnamewindow->hide();
        get_widget_derived(m_refxml, "findnearestwindow", m_findnearestwindow);
        m_findnearestwindow->set_engine(m_engine);
        m_findnearestwindow->signal_setwaypoint().connect(sigc::mem_fun(*this, &RouteEditUi::set_current_waypoint));
        m_findnearestwindow->signal_insertwaypoint().connect(sigc::mem_fun(*this, &RouteEditUi::menu_insertwaypoint));
        m_findnearestwindow->hide();
        // Confirm Delete Window
        get_widget_derived(m_refxml, "confirmdeletewindow", m_confirmdeletewindow);
        m_confirmdeletewindow->hide();
        m_confirmdeletewindow->set_route(m_route);
        m_confirmdeletewindow->signal_delete().connect(sigc::mem_fun(*this, &RouteEditUi::confirm_delete_route));
        // Map Window
        get_widget_derived(m_refxml, "mapwindow", m_mapwindow);
        m_mapwindow->hide();
        m_mapwindow->set_engine(m_engine);
        m_mapwindow->set_route(m_route);
        m_mapwindow->signal_setcoord().connect(sigc::mem_fun(*this, &RouteEditUi::map_set_coord));
        m_mapwindow->signal_info().connect(sigc::mem_fun(*this, &RouteEditUi::map_info_coord));
        // About Window
        get_widget(m_refxml, "aboutdialog", m_aboutdialog);
        m_aboutdialog->hide();
        m_aboutdialog->signal_response().connect(sigc::mem_fun(*this, &RouteEditUi::aboutdialog_response));
        m_aboutitem->signal_activate().connect(sigc::mem_fun<void,Gtk::AboutDialog>(*m_aboutdialog, &Gtk::AboutDialog::present));
        // Distance Window
        get_widget_derived(m_refxml, "distancewindow", m_distancewindow);
        m_distancewindow->hide();
        m_distancewindow->set_engine(m_engine);
        m_distancewindow->signal_insertwpt().connect(sigc::mem_fun(*this, &RouteEditUi::distance_insert_coord));
        // Course/Distance Window
        get_widget_derived(m_refxml, "crsdistwindow", m_coursedistwindow);
        m_coursedistwindow->hide();
        m_coursedistwindow->set_engine(m_engine);
        m_coursedistwindow->signal_setcoord().connect(sigc::mem_fun(*this, &RouteEditUi::map_set_coord));
        // Maidenhead Locator Window
        get_widget_derived(m_refxml, "coordmaidenheadwindow", m_maidenheadwindow);
        m_maidenheadwindow->hide();
        m_maidenheadwindow->set_engine(m_engine);
        m_maidenheadwindow->signal_setcoord().connect(sigc::mem_fun(*this, &RouteEditUi::map_set_coord));
        // CH-1903 Window
        get_widget_derived(m_refxml, "coordch1903window", m_ch1903window);
        m_ch1903window->hide();
        m_ch1903window->set_engine(m_engine);
        m_ch1903window->signal_setcoord().connect(sigc::mem_fun(*this, &RouteEditUi::map_set_coord));
        // Info Window
        get_widget_derived(m_refxml, "infowindow", m_infowindow);
        m_infowindow->hide();
        // Sunrise/Sunset Window
        get_widget_derived(m_refxml, "sunrisesunsetwindow", m_sunrisesunsetwindow);
        m_sunrisesunsetwindow->hide();
        m_sunrisesunsetwindow->set_route(m_route);
	// ICAO FPL Import Window
	get_widget_derived(m_refxml, "icaofplimportwindow", m_icaofplimportwindow);
	m_icaofplimportwindow->hide();
	m_icaofplimportwindow->signal_fplimport().connect(sigc::mem_fun(*this, &RouteEditUi::icaofpl_import));
        // insert routelist data
        {
                FPlanRoute route(m_engine.get_fplan_db());
                Gtk::TreeStore::iterator sel;
                FPlanRoute::id_t curplan = m_engine.get_prefs().curplan;
                for (bool work = route.load_first_fp(); work; work = route.load_next_fp()) {
                        Gtk::TreeStore::iterator iter(routeliststore_insert(route, m_routeliststore->children().end()));
                        if (route.get_id() == curplan)
                                sel = iter;
                }
                if (sel) {
                        routelist_selection->select(sel);
                        m_routelist->scroll_to_row(Gtk::TreeModel::Path(sel));
                }
        }
#ifdef HAVE_HILDONMM
        Hildon::Program::get_instance()->add_window(*m_routeeditwindow);
        Hildon::Program::get_instance()->add_window(*m_findnamewindow);
        Hildon::Program::get_instance()->add_window(*m_findnearestwindow);
        Hildon::Program::get_instance()->add_window(*m_confirmdeletewindow);
        Hildon::Program::get_instance()->add_window(*m_distancewindow);
        Hildon::Program::get_instance()->add_window(*m_coursedistwindow);
        Hildon::Program::get_instance()->add_window(*m_maidenheadwindow);
        Hildon::Program::get_instance()->add_window(*m_ch1903window);
        Hildon::Program::get_instance()->add_window(*m_infowindow);
        Hildon::Program::get_instance()->add_window(*m_sunrisesunsetwindow);
        Hildon::Program::get_instance()->add_window(*m_icaofplimportwindow);
#endif
        // finally show
        m_routeeditwindow->present();
}

RouteEditUi::~RouteEditUi()
{
        map_info_async_cancel();
        map_set_coord_async_cancel();
#ifdef HAVE_HILDONMM
        Hildon::Program::get_instance()->remove_window(*m_routeeditwindow);
        Hildon::Program::get_instance()->remove_window(*m_findnamewindow);
        Hildon::Program::get_instance()->remove_window(*m_findnearestwindow);
        Hildon::Program::get_instance()->remove_window(*m_confirmdeletewindow);
        Hildon::Program::get_instance()->remove_window(*m_distancewindow);
        Hildon::Program::get_instance()->remove_window(*m_coursedistwindow);
        Hildon::Program::get_instance()->remove_window(*m_maidenheadwindow);
        Hildon::Program::get_instance()->remove_window(*m_ch1903window);
        Hildon::Program::get_instance()->remove_window(*m_infowindow);
        Hildon::Program::get_instance()->remove_window(*m_sunrisesunsetwindow);
        Hildon::Program::get_instance()->remove_window(*m_icaofplimportwindow);
#endif
}

RouteEditUi::RouteListModelColumns::RouteListModelColumns(void)
{
        add(m_col_from);
        add(m_col_to);
        add(m_col_dist);
        add(m_col_id);
}

RouteEditUi::WaypointListModelColumns::WaypointListModelColumns(void)
{
        add(m_col_icao);
        add(m_col_name);
        add(m_col_freq);
        add(m_col_time);
        add(m_col_lon);
        add(m_col_lat);
        add(m_col_alt);
        add(m_col_mt);
        add(m_col_tt);
        add(m_col_dist);
        add(m_col_vertspeed);
        add(m_col_truealt);
	add(m_col_pathname);
        add(m_col_wind);
        add(m_col_qff);
        add(m_col_oat);
        add(m_col_nr);
}

void RouteEditUi::distance_cell_func(Gtk::CellRenderer *cell, const Gtk::TreeModel::iterator& iter)
{
        Gtk::CellRendererText *cellt(dynamic_cast<Gtk::CellRendererText *>(cell));
        if (!cellt)
                return;
        Gtk::TreeModel::Row row = *iter;
        float val(row[m_routelistcolumns.m_col_dist]);
        char buf[16];
        buf[0] = 0;
        if (!std::isnan(val))
                snprintf(buf, sizeof(buf), "%6.1f", (double)val);
#ifdef GLIBMM_PROPERTIES_ENABLED
        cellt->property_text() = buf;
        cellt->property_xalign() = 1.0;
#else
        Glib::ustring buf1(buf);
        cellt->set_property("text", buf1);
        cellt->set_property("xalign", 1.0);
#endif
}

Gtk::TreeStore::iterator RouteEditUi::routeliststore_insert(const FPlanRoute& route, const Gtk::TreeStore::iterator& iter)
{
        Gtk::TreeStore::iterator iter1;
        if (iter)
                iter1 = m_routeliststore->insert(iter);
        else
                iter1 = m_routeliststore->append();
        routeliststore_update(route, iter1);
        return iter1;
}

void RouteEditUi::routeliststore_update(const FPlanRoute& route, const Gtk::TreeStore::iterator& iter)
{
        Gtk::TreeModel::Row row(*iter);
        while (row->children().begin())
                m_routeliststore->erase(row->children().begin());
        row[m_routelistcolumns.m_col_id] = route.get_id();
        if (route.get_nrwpt() < 1) {
                row[m_routelistcolumns.m_col_from] = "-";
                row[m_routelistcolumns.m_col_to] = "-";
                row[m_routelistcolumns.m_col_dist] = std::numeric_limits<float>::quiet_NaN();
                return;
        }
        const FPlanWaypoint& wptfrom(route[0]);
        const FPlanWaypoint& wptto(route[route.get_nrwpt()-1]);
        Glib::ustring s = wptfrom.get_icao();
        if (s.size() > 0)
                s += ' ';
        s += wptfrom.get_name();
        row[m_routelistcolumns.m_col_from] = s;
        s = wptto.get_icao();
        if (s.size() > 0)
                s += ' ';
        s += wptto.get_name();
        row[m_routelistcolumns.m_col_to] = s;
        row[m_routelistcolumns.m_col_dist] = route.total_distance_nmi();
        for (unsigned int nr = 0; nr < route.get_nrwpt(); nr++) {
                const FPlanWaypoint& wpt(route[nr]);
                Gtk::TreeModel::Row childrow = *(m_routeliststore->append(row.children()));
                childrow[m_routelistcolumns.m_col_id] = nr;
                s = wpt.get_icao();
                if (s.size() > 0)
                        s += ' ';
                s += wpt.get_name();
                childrow[m_routelistcolumns.m_col_from] = s;
                float dist = std::numeric_limits<float>::quiet_NaN();
                if (nr + 1 < route.get_nrwpt())
                        dist = wpt.get_coord().spheric_distance_nmi(route[nr+1].get_coord());
                childrow[m_routelistcolumns.m_col_dist] = dist;
        }
}

void RouteEditUi::route_save(void)
{
        if (m_route.get_id()) {
                m_route.save_fp();
                signal_updateroute()(m_route.get_id());
        }
        Gtk::TreeStore::iterator i(m_routeliststore->children().begin()), ie(m_routeliststore->children().end());
        FPlanRoute::id_t id(m_route.get_id());
        for (; i != ie; i++) {
                 if ((*i)[m_routelistcolumns.m_col_id] != id)
                         continue;
                 routeliststore_update(m_route, i);
                 break;
        }
        if (i == ie)
                routeliststore_insert(m_route, i);
        route_update();
}

void RouteEditUi::route_selection(void)
{
        Gtk::TreeStore::iterator i(m_routeliststore->children().begin()), ie(m_routeliststore->children().end());
        FPlanRoute::id_t id(m_route.get_id());
        for (; i != ie; i++) {
                 if ((*i)[m_routelistcolumns.m_col_id] != id)
                         continue;
                 Glib::RefPtr<Gtk::TreeSelection> routelist_selection = m_routelist->get_selection();
                 routelist_selection->select(i);
                 m_routelist->scroll_to_row(Gtk::TreePath(i));
                 break;
        }
}

void RouteEditUi::route_delete(void)
{
        FPlanRoute::id_t id(m_route.get_id());
        m_route.delete_fp();
        Gtk::TreeStore::iterator i(m_routeliststore->children().begin()), ie(m_routeliststore->children().end());
        for (; i != ie;) {
                 if ((*i)[m_routelistcolumns.m_col_id] != id) { 
                         i++;
                         continue;
                 }
                 i = m_routeliststore->erase(i);
        }
        route_load(0);
}

void RouteEditUi::route_update(void)
{
        m_findnamewindow->hide();
        m_findnearestwindow->hide();
        m_confirmdeletewindow->hide();
        m_distancewindow->hide();
        m_coursedistwindow->hide();
        m_maidenheadwindow->hide();
        m_ch1903window->hide();
        m_sunrisesunsetwindow->update_route();
        if (!m_route.get_id()) {
                m_routenotebuffer->set_text("");
                m_routenote->set_sensitive(false);
                m_waypointlist->set_sensitive(false);
                m_duplicateroute->set_sensitive(false);
                m_deleteroute->set_sensitive(false);
                m_reverseroute->set_sensitive(false);
                m_startnavigate->set_sensitive(false);
		m_waypointliststore->clear();
                return;
        }
        m_routenote->set_sensitive(true);
        m_waypointlist->set_sensitive(true);
        m_duplicateroute->set_sensitive(true);
        m_deleteroute->set_sensitive(true);
        m_reverseroute->set_sensitive(true);
        m_startnavigate->set_sensitive(true);
        Gtk::TreeIter iter(m_waypointliststore->children().begin());
	if (!iter)
		iter = m_waypointliststore->append();
	Gtk::TreeModel::Row row(*iter);
	++iter;
        row[m_waypointlistcolumns.m_col_nr] = 0;
        row[m_waypointlistcolumns.m_col_icao] = "";
        row[m_waypointlistcolumns.m_col_name] = "Off Block";
        row[m_waypointlistcolumns.m_col_freq] = "";
        row[m_waypointlistcolumns.m_col_time] = Conversions::time_str(m_route.get_time_offblock_unix());
        row[m_waypointlistcolumns.m_col_lon] = "";
        row[m_waypointlistcolumns.m_col_lat] = "";
        row[m_waypointlistcolumns.m_col_alt] = "";
        row[m_waypointlistcolumns.m_col_mt] = "";
        row[m_waypointlistcolumns.m_col_tt] = "";
        row[m_waypointlistcolumns.m_col_dist] = "";
        row[m_waypointlistcolumns.m_col_vertspeed] = "";
        row[m_waypointlistcolumns.m_col_truealt] = "";
        row[m_waypointlistcolumns.m_col_pathname] = "";
        row[m_waypointlistcolumns.m_col_wind] = "";
        row[m_waypointlistcolumns.m_col_qff] = "";
        row[m_waypointlistcolumns.m_col_oat] = "";
        WMM wmm;
        time_t curtime = time(0);
        Preferences& prefs(m_engine.get_prefs());
        for (unsigned int nr = 0; nr < m_route.get_nrwpt(); nr++) {
                const FPlanWaypoint& wpt(m_route[nr]);
		if (!iter)
			iter = m_waypointliststore->append();
		row = *iter;
		++iter;
		IcaoAtmosphere<float> qnh(wpt.get_qff_hpa(), wpt.get_isaoffset_kelvin() + IcaoAtmosphere<float>::std_sealevel_temperature);
                row[m_waypointlistcolumns.m_col_nr] = nr + 1;
                row[m_waypointlistcolumns.m_col_icao] = wpt.get_icao();
                row[m_waypointlistcolumns.m_col_name] = wpt.get_name();
                row[m_waypointlistcolumns.m_col_freq] = Conversions::freq_str(wpt.get_frequency());
                row[m_waypointlistcolumns.m_col_time] = Conversions::time_str(wpt.get_time_unix());
                row[m_waypointlistcolumns.m_col_lon] = wpt.get_coord().get_lon_str();
                row[m_waypointlistcolumns.m_col_lat] = wpt.get_coord().get_lat_str();
                row[m_waypointlistcolumns.m_col_alt] = Conversions::alt_str(wpt.get_altitude(), wpt.get_flags());
                int16_t wpttruealt = FPlanLeg::baro_correction(wpt.get_qff_hpa(), wpt.get_isaoffset_kelvin(), wpt.get_altitude(), wpt.get_flags());
                row[m_waypointlistcolumns.m_col_truealt] = Conversions::alt_str(wpttruealt, 0);
		row[m_waypointlistcolumns.m_col_pathname] = "";
		if (wpt.get_pathcode() == FPlanWaypoint::pathcode_directto)
			row[m_waypointlistcolumns.m_col_pathname] = "DCT";
		else if (wpt.get_pathcode() != FPlanWaypoint::pathcode_none)
			row[m_waypointlistcolumns.m_col_pathname] = wpt.get_pathname();
		row[m_waypointlistcolumns.m_col_wind] = Conversions::track_str(wpt.get_winddir_deg()) + "/" + Conversions::dist_str(wpt.get_windspeed_kts());
		{
			std::ostringstream oss;
			oss << std::fixed << std::setprecision(0) << wpt.get_qff_hpa();
			row[m_waypointlistcolumns.m_col_qff] = oss.str();
		}
		{
			std::ostringstream oss;
			oss << std::fixed << std::setprecision(1) << (wpt.get_oat_kelvin() - IcaoAtmosphere<float>::degc_to_kelvin);
			row[m_waypointlistcolumns.m_col_oat] = oss.str();
		}
                if (nr + 1 < m_route.get_nrwpt()) {
                        const FPlanWaypoint& wpt2(m_route[nr+1]);
                        Point pt1(wpt.get_coord()), pt2(wpt2.get_coord());
                        float dist = pt1.spheric_distance_nmi(pt2);
                        float tt = pt1.spheric_true_course(pt2);
                        wmm.compute((wpt.get_altitude() + wpt2.get_altitude()) * (FPlanWaypoint::ft_to_m / 2000.0), pt1.halfway(pt2), curtime);
                        float mt = tt;
                        if (wmm)
                                mt -= wmm.get_dec();
                        int16_t wpt2truealt = FPlanLeg::baro_correction(wpt2.get_qff_hpa(), wpt2.get_isaoffset_kelvin(), wpt2.get_altitude(), wpt2.get_flags());
                        float legtime = 60.0f * dist / std::max(1.0f, (float)prefs.vcruise);
                        float vspeed = (wpt2truealt - wpttruealt) / std::max(1.0f / 60.0f, legtime);
                        row[m_waypointlistcolumns.m_col_mt] = Conversions::track_str(mt);
                        row[m_waypointlistcolumns.m_col_tt] = Conversions::track_str(tt);
                        row[m_waypointlistcolumns.m_col_dist] = Conversions::dist_str(dist);
                        row[m_waypointlistcolumns.m_col_vertspeed] = Conversions::verticalspeed_str(vspeed);
                } else {
                        row[m_waypointlistcolumns.m_col_mt] = "";
                        row[m_waypointlistcolumns.m_col_tt] = "";
                        row[m_waypointlistcolumns.m_col_dist] = "";
                        row[m_waypointlistcolumns.m_col_vertspeed] = "";
                }
        }
	if (!iter)
		iter = m_waypointliststore->append();
	row = *iter;
	++iter;
        row[m_waypointlistcolumns.m_col_nr] = m_route.get_nrwpt() + 1;
        row[m_waypointlistcolumns.m_col_icao] = "";
        row[m_waypointlistcolumns.m_col_name] = "On Block";
        row[m_waypointlistcolumns.m_col_freq] = "";
        row[m_waypointlistcolumns.m_col_time] = Conversions::time_str(m_route.get_time_onblock_unix());
        row[m_waypointlistcolumns.m_col_lon] = "";
        row[m_waypointlistcolumns.m_col_lat] = "";
        row[m_waypointlistcolumns.m_col_alt] = "";
        row[m_waypointlistcolumns.m_col_mt] = "";
        row[m_waypointlistcolumns.m_col_tt] = "";
        row[m_waypointlistcolumns.m_col_dist] = "";
        row[m_waypointlistcolumns.m_col_vertspeed] = "";
        row[m_waypointlistcolumns.m_col_truealt] = "";
	row[m_waypointlistcolumns.m_col_pathname] = "";
        row[m_waypointlistcolumns.m_col_wind] = "";
        row[m_waypointlistcolumns.m_col_qff] = "";
        row[m_waypointlistcolumns.m_col_oat] = "";
        while (iter)
                iter = m_waypointliststore->erase(iter);
        waypointlist_selection_update();
        m_routenotebuffer->set_text(m_route.get_note());
        m_mapwindow->redraw_route();
	// update weather page
	if (m_routeeditnotebook) {
		Gtk::Widget *pg(m_routeeditnotebook->get_nth_page(4));
		if (pg)
			pg->set_visible(!!m_route.get_nrwpt());
	}
	if (m_route.get_nrwpt()) {
		m_connwxoffblock.block();
                m_connwxoffblockh.block();
                m_connwxoffblockm.block();
                m_connwxoffblocks.block();
                m_connwxtaxitimem.block();
                m_connwxtaxitimes.block();
                {
                        time_t t(m_route.get_time_offblock_unix());
                        struct tm utm;
                        gmtime_r(&t, &utm);
                        if (m_wxoffblock) {
                                m_wxoffblock->select_month(utm.tm_mon, utm.tm_year + 1900);
                                m_wxoffblock->select_day(utm.tm_mday);
                                m_wxoffblock->mark_day(utm.tm_mday);
                        }
                        if (m_wxoffblockh)
                                m_wxoffblockh->set_value(utm.tm_hour);
                        if (m_wxoffblockm)
                                m_wxoffblockm->set_value(utm.tm_min);
                        if (m_wxoffblocks)
                                m_wxoffblocks->set_value(utm.tm_sec);
                        t = m_route[0].get_time_unix() - t;
                        if (m_wxtaxitimem)
                                m_wxtaxitimem->set_value(t / 60U);
                        if (m_wxtaxitimes)
                                m_wxtaxitimes->set_value(t % 60U);
                }
                m_connwxoffblock.unblock();
                m_connwxoffblockh.unblock();
                m_connwxoffblockm.unblock();
                m_connwxoffblocks.unblock();
                m_connwxtaxitimem.unblock();
                m_connwxtaxitimes.unblock();
                // find altitudes, average atmosphere
                {
                        typedef std::set<int> altset_t;
                        altset_t altset;
                        float windx(0), windy(0), isaoffs(0), qff(0);
			Preferences& p(m_engine.get_prefs());
			time_t routetime(0);
                        for (unsigned int i = 0; i < m_route.get_nrwpt(); ++i) {
                                const FPlanWaypoint& wpt(m_route[i]);
                                altset.insert(wpt.get_altitude());
                                float x(0), y(0);
                                sincosf(wpt.get_winddir_rad(), &y, &x);
                                windx += x * wpt.get_windspeed_kts();
                                windy += y * wpt.get_windspeed_kts();
                                isaoffs += wpt.get_isaoffset_kelvin();
                                qff += wpt.get_qff_hpa();
				if (!i)
					continue;
				FPlanLeg leg = m_route.get_leg(i);
				leg.wind_correction(p.vcruise);
				routetime += leg.get_legtime();
			}
			{
                                float s(1.f / m_route.get_nrwpt());
                                windx *= s;
                                windy *= s;
                                isaoffs *= s;
                                qff *= s;
                        }
                        float windspeed(sqrtf(windx * windx + windy * windy)), winddir(0.f);
                        if (windspeed >= 0.001f)
                                winddir = atan2f(windy, windx) * (float)(180.f / M_PI);
                        m_connwxcruisealt.block();
                        if (m_wxcruisealt) {
                                m_wxcruisealt->set_sensitive(false);
                                altset_t::const_reverse_iterator ai(altset.rbegin()), ae(altset.rend());
                                if (ai != ae) {
                                        int crsalt(*ai++);
                                        m_wxcruisealt->set_value(crsalt);
                                        if (ai != ae) {
                                                double rmin, rmax;
                                                m_wxcruisealt->get_range(rmin, rmax);
                                                rmin = std::min(crsalt, *ai + 100);
                                                m_wxcruisealt->set_range(rmin, rmax);
                                                m_wxcruisealt->set_sensitive(true);
                                        }
                                }                                               
                        }
                        m_connwxcruisealt.unblock();
			if (m_wxwinddir)
                                m_wxwinddir->set_value(winddir);
                        if (m_wxwindspeed)
                                m_wxwindspeed->set_value(windspeed);
                        if (m_wxisaoffs)
                                m_wxisaoffs->set_value(isaoffs);
                        if (m_wxqff)
                                m_wxqff->set_value(qff);
			if (m_wxflighttime)
				m_wxflighttime->set_text(Conversions::time_str(routetime));
                }
	}

}

void RouteEditUi::route_load(FPlanRoute::id_t id)
{
        if (id)
                m_route.load_fp(id);
        else
                m_route.new_fp();
        route_update();
}

void RouteEditUi::update_route(FPlanRoute::id_t id)
{
        if (!id)
                return;
        {
                FPlanRoute rte(m_engine.get_fplan_db());
                rte.load_fp(id);
                Gtk::TreeStore::iterator i(m_routeliststore->children().begin()), ie(m_routeliststore->children().end());
                for (; i != ie; i++) {
                        if ((*i)[m_routelistcolumns.m_col_id] != id)
                                continue;
                        routeliststore_update(rte, i);
                        break;
                }
                if (i == ie)
                        routeliststore_insert(rte, i);
        }
        if (id != m_route.get_id())
                return;
        route_load(id);
}

int RouteEditUi::compare_dist(const Glib::ustring & sa, const Glib::ustring & sb)
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

int RouteEditUi::compare_dist(float da, float db)
{
        if (da < db)
                return -1;
        if (da > db)
                return 1;
        return 0;
}

int RouteEditUi::routelist_sort_from(const Gtk::TreeModel::iterator& ia, const Gtk::TreeModel::iterator& ib)
{
        Gtk::TreeModel::Path pa(m_routeliststore->get_path(ia)), pb(m_routeliststore->get_path(ib));
        const Gtk::TreeModel::Row rowa(*ia), rowb(*ib);
        Gtk::TreeModel::Path::size_type pas(pa.size()), pbs(pb.size());
        if (pas > 1 && pbs > 1) {
                FPlanRoute::id_t ida(rowa.get_value(m_routelistcolumns.m_col_id)), idb(rowb.get_value(m_routelistcolumns.m_col_id));
                if (ida < idb)
                        return -1;
                if (ida > idb)
                        return 1;
                return 0;
        }
        if (pas > 1)
                return 1;
        if (pbs > 1)
                return -1;
        int cfrom(rowa.get_value(m_routelistcolumns.m_col_from).compare(rowb.get_value(m_routelistcolumns.m_col_from)));
        if (cfrom)
                return cfrom;
        int cto(rowa.get_value(m_routelistcolumns.m_col_to).compare(rowb.get_value(m_routelistcolumns.m_col_to)));
        if (cto)
                return cto;
        int cdist(compare_dist(rowa.get_value(m_routelistcolumns.m_col_dist), rowb.get_value(m_routelistcolumns.m_col_dist)));
        return cdist;
}

int RouteEditUi::routelist_sort_to(const Gtk::TreeModel::iterator& ia, const Gtk::TreeModel::iterator& ib)
{
        Gtk::TreeModel::Path pa(m_routeliststore->get_path(ia)), pb(m_routeliststore->get_path(ib));
        const Gtk::TreeModel::Row rowa(*ia), rowb(*ib);
        Gtk::TreeModel::Path::size_type pas(pa.size()), pbs(pb.size());
        if (pas > 1 && pbs > 1) {
                FPlanRoute::id_t ida(rowa.get_value(m_routelistcolumns.m_col_id)), idb(rowb.get_value(m_routelistcolumns.m_col_id));
                if (ida < idb)
                        return -1;
                if (ida > idb)
                        return 1;
                return 0;
        }
        if (pas > 1)
                return 1;
        if (pbs > 1)
                return -1;
        int cto(rowa.get_value(m_routelistcolumns.m_col_to).compare(rowb.get_value(m_routelistcolumns.m_col_to)));
        if (cto)
                return cto;
        int cfrom(rowa.get_value(m_routelistcolumns.m_col_from).compare(rowb.get_value(m_routelistcolumns.m_col_from)));
        if (cfrom)
                return cfrom;
        int cdist(compare_dist(rowa.get_value(m_routelistcolumns.m_col_dist), rowb.get_value(m_routelistcolumns.m_col_dist)));
        return cdist;
}

int RouteEditUi::routelist_sort_dist(const Gtk::TreeModel::iterator& ia, const Gtk::TreeModel::iterator& ib)
{
        Gtk::TreeModel::Path pa(m_routeliststore->get_path(ia)), pb(m_routeliststore->get_path(ib));
        const Gtk::TreeModel::Row rowa(*ia), rowb(*ib);
        Gtk::TreeModel::Path::size_type pas(pa.size()), pbs(pb.size());
        if (pas > 1 && pbs > 1) {
                FPlanRoute::id_t ida(rowa.get_value(m_routelistcolumns.m_col_id)), idb(rowb.get_value(m_routelistcolumns.m_col_id));
                if (ida < idb)
                        return -1;
                if (ida > idb)
                        return 1;
                return 0;
        }
        if (pas > 1)
                return 1;
        if (pbs > 1)
                return -1;
        int cdist(compare_dist(rowa.get_value(m_routelistcolumns.m_col_dist), rowb.get_value(m_routelistcolumns.m_col_dist)));
        if (cdist)
                return cdist;
        int cfrom(rowa.get_value(m_routelistcolumns.m_col_from).compare(rowb.get_value(m_routelistcolumns.m_col_from)));
        if (cfrom)
                return cfrom;
        int cto(rowa.get_value(m_routelistcolumns.m_col_to).compare(rowb.get_value(m_routelistcolumns.m_col_to)));
        return cto;
}

bool RouteEditUi::routelist_select_function(const Glib::RefPtr<Gtk::TreeModel>& model, const Gtk::TreeModel::Path& path, bool)
{
        return (path.size() <= 1);
        //const Gtk::TreeModel::iterator iter = model->get_iter(path);
        //return iter->children().empty(); // only allow leaf nodes to be selected
}

void RouteEditUi::routelist_selection_changed(void)
{
        Glib::RefPtr<Gtk::TreeSelection> routelist_selection = m_routelist->get_selection();
        Gtk::TreeModel::iterator iter = routelist_selection->get_selected();
        if (iter) {
                Gtk::TreeModel::Row row = *iter;
                FPlanRoute::id_t id = row[m_routelistcolumns.m_col_id];
                route_load(id);
                m_engine.get_prefs().curplan = id;
                //Do something with the row.
                std::cerr << "selected row id: " << id << std::endl;
                return;
        }
        route_load(0);
        std::cerr << "selection cleared" << std::endl;
}

bool RouteEditUi::waypointlist_select_function(const Glib::RefPtr<Gtk::TreeModel>& model, const Gtk::TreeModel::Path& path, bool)
{
        Gtk::TreeModel::iterator iter = model->get_iter(path);
        if (!iter)
                return false;
        Gtk::TreeModel::Row row = *iter;
        return row[m_waypointlistcolumns.m_col_nr] < m_route.get_nrwpt() + 2;
}

void RouteEditUi::waypointlist_selection_changed(void)
{
        map_set_coord_async_cancel();
        Glib::RefPtr<Gtk::TreeSelection> waypointlist_selection = m_waypointlist->get_selection();
        Gtk::TreeModel::iterator iter = waypointlist_selection->get_selected();
        if (iter) {
                Gtk::TreeModel::Row row = *iter;
                if (row[m_waypointlistcolumns.m_col_nr] == m_route.get_curwpt()) {
                        waypointlist_update();
                        return;
                }
                m_route.set_curwpt(row[m_waypointlistcolumns.m_col_nr]);
                m_route.save_fp();
                signal_updateroute()(m_route.get_id());
                waypointlist_update();
                std::cerr << "wpt selected row id: " << row[m_waypointlistcolumns.m_col_nr] << std::endl;
                return;
        }
        waypointlist_disable();
        std::cerr << "wpt selection cleared" << std::endl;
}

void RouteEditUi::waypointlist_update(void)
{
        m_findnamewindow->hide();
        m_findnearestwindow->hide();
        m_mapwindow->hide();
        m_editactive = false;
        uint16_t nr = m_route.get_curwpt();
        uint16_t nrwpt = m_route.get_nrwpt();
        if (nr < 1 || nr > nrwpt) {
                waypointlist_disable();
                return;
        }
        m_insertwaypoint->set_sensitive(nr <= nrwpt);
        m_swapwaypoint->set_sensitive(nr >= 1 && nr < nrwpt);
        m_deletewaypoint->set_sensitive(nr >= 1 && nr <= nrwpt);
        m_waypointdistance->set_sensitive(nr >= 1 && nr <= nrwpt && nrwpt > 1);
        m_waypointcoursedistance->set_sensitive(nr >= 1 && nr <= nrwpt);
        m_waypointstraighten->set_sensitive(nr > 1 && nr < nrwpt);
        m_waypointcoordch1903->set_sensitive(nr >= 1 && nr <= nrwpt);
        m_waypointcoordmaidenhead->set_sensitive(nr >= 1 && nr <= nrwpt);
        const FPlanWaypoint& wpt(m_route[nr - 1]);
        m_wpticao->set_text(wpt.get_icao());
        m_wptname->set_text(wpt.get_name());
	m_wptpathcode->set_active(wpt.get_pathcode());
	m_wptpathname->set_text(wpt.get_pathname());
        set_coord_widgets(m_wptlonsign, m_wptlondeg, m_wptlonmin, wpt.get_coord().get_lon_deg());
        set_coord_widgets(m_wptlatsign, m_wptlatdeg, m_wptlatmin, wpt.get_coord().get_lat_deg());
        set_alt_widgets(m_wptalt, m_wptaltunit, wpt.get_altitude(), wpt.get_flags());
        set_freq_widgets(m_wptfreq, m_wptfrequnit, wpt.get_frequency());
        m_wptwinddir->set_value(wpt.get_winddir_deg());
        m_wptwindspeed->set_value(wpt.get_windspeed_kts());
        m_wptqff->set_value(wpt.get_qff_hpa());
        m_wptoat->set_value(wpt.get_oat_kelvin() - IcaoAtmosphere<float>::degc_to_kelvin);
        m_wptnotebuffer->set_text(wpt.get_note());
        m_wptprev->set_sensitive(nr > 1);
        m_wptnext->set_sensitive(nr < m_route.get_nrwpt());
        m_wptfind->set_sensitive(true);
        m_wptfindnearest->set_sensitive(true);
        m_wpticao->set_sensitive(true);
        m_wptname->set_sensitive(true);
        m_wptpathcode->set_sensitive(true);
        m_wptpathname->set_sensitive(wpt.get_pathcode() != FPlanWaypoint::pathcode_none &&
				     wpt.get_pathcode() != FPlanWaypoint::pathcode_directto);
        m_wptlonsign->set_sensitive(true);
        m_wptlondeg->set_sensitive(true);
        m_wptlonmin->set_sensitive(true);
        m_wptlatsign->set_sensitive(true);
        m_wptlatdeg->set_sensitive(true);
        m_wptlatmin->set_sensitive(true);
        m_wptalt->set_sensitive(true);
        m_wptaltunit->set_sensitive(true);
        m_wptfreq->set_sensitive(true);
	m_wptifrconn.block();
	m_wptifr->set_inconsistent(false);
	m_wptifr->set_active(!!(wpt.get_flags() & FPlanWaypoint::altflag_ifr));
	m_wptifr->set_sensitive(true);
	m_wptifrconn.unblock();
	m_wptwinddir->set_sensitive(true);
	m_wptwindspeed->set_sensitive(true);
 	m_wptqff->set_sensitive(true);
 	m_wptoat->set_sensitive(true);
        m_wptnote->set_sensitive(true);
        m_widgetchange = false;
        m_editactive = true;
}

void RouteEditUi::waypointlist_disable(void)
{
        uint16_t nr = m_route.get_curwpt();
        uint16_t nrwpt = m_route.get_nrwpt();
        m_editactive = false;
        m_wpticao->set_sensitive(false);
        m_wptname->set_sensitive(false);
        m_wptpathcode->set_sensitive(false);
        m_wptpathname->set_sensitive(false);
        m_wptlonsign->set_sensitive(false);
        m_wptlondeg->set_sensitive(false);
        m_wptlonmin->set_sensitive(false);
        m_wptlatsign->set_sensitive(false);
        m_wptlatdeg->set_sensitive(false);
        m_wptlatmin->set_sensitive(false);
        m_wptalt->set_sensitive(false);
        m_wptaltunit->set_sensitive(false);
        m_wptfreq->set_sensitive(false);
	m_wptifrconn.block();
	m_wptifr->set_inconsistent(true);
	m_wptifr->set_sensitive(false);
	m_wptifrconn.unblock();
	m_wptwinddir->set_sensitive(false);
	m_wptwindspeed->set_sensitive(false);
 	m_wptqff->set_sensitive(false);
 	m_wptoat->set_sensitive(false);
        m_wptnote->set_sensitive(false);
        m_wptprev->set_sensitive(nr > 0);
        m_wptnext->set_sensitive(nr <= nrwpt);
        m_wptfind->set_sensitive(false);
        m_wptfindnearest->set_sensitive(false);
        m_insertwaypoint->set_sensitive(nr <= nrwpt);
        m_swapwaypoint->set_sensitive(nr >= 1 && nr < nrwpt);
        m_deletewaypoint->set_sensitive(nr >= 1 && nr <= nrwpt);
        m_waypointdistance->set_sensitive(nr >= 1 && nr <= nrwpt && nrwpt > 1);
        m_waypointcoursedistance->set_sensitive(nr >= 1 && nr <= nrwpt);
        m_waypointstraighten->set_sensitive(nr > 1 && nr < nrwpt);
        m_waypointcoordch1903->set_sensitive(nr >= 1 && nr <= nrwpt);
        m_waypointcoordmaidenhead->set_sensitive(nr >= 1 && nr <= nrwpt);
}

void RouteEditUi::waypointlist_selection_update(void)
{
        Glib::RefPtr<Gtk::TreeSelection> waypointlist_selection = m_waypointlist->get_selection();
        if (m_route.get_curwpt() < m_route.get_nrwpt() + 2) {
                Gtk::TreeModel::Row row = m_waypointliststore->children()[m_route.get_curwpt()];
                if (row) {
                        waypointlist_selection->select(row);
                        return;
                }
        }
        waypointlist_selection->unselect_all();
}

void RouteEditUi::set_coord_widgets(Gtk::ComboBox * sign, Gtk::Entry * deg, Gtk::Entry * min, float degf)
{
        sign->set_active(degf < 0);
        float t, t1;
        t = modff(fabsf(degf), &t1);
        t *= 60;
        char buf[16];
        snprintf(buf, sizeof(buf), "%03.0f", t1);
        deg->set_text(buf);
        snprintf(buf, sizeof(buf), "%06.3f", t);
        min->set_text(buf);
}

float RouteEditUi::get_coord_widgets(const Gtk::ComboBox * sign, const Gtk::Entry * deg, const Gtk::Entry * min)
{
        char buf[16];
        strncpy(buf, deg->get_text().c_str(), sizeof(buf));
        float d = strtod(buf, NULL);
        strncpy(buf, min->get_text().c_str(), sizeof(buf));
        char *cp = strchr(buf, '\'');
        if (cp) {
                *cp++ = 0;
                d += strtod(cp, NULL) * (1.0 / 60.0 / 60.0);
        }
        d += strtod(buf, NULL) * (1.0 / 60.0);
        if (sign->get_active_row_number())
                d = -d;
        return d;
}

void RouteEditUi::set_alt_widgets(Gtk::Entry * altv, Gtk::ComboBox * altu, int32_t alt, uint16_t flags, unsigned int unit)
{
        if (unit > 2)
                unit = 0;
        if (flags & FPlanWaypoint::altflag_standard) {
                unit = 1;
        } else {
                if (unit == 1)
                        unit = 0;
        }
        altu->set_active(unit);
        char buf[16];
        switch (unit) {
                case 1:
                        snprintf(buf, sizeof(buf), "%f", alt * 0.01);
                        break;

                case 2:
                        snprintf(buf, sizeof(buf), "%f", alt * FPlanWaypoint::ft_to_m);
                        break;

                default:
                        snprintf(buf, sizeof(buf), "%d", alt);
                        break;
        }
        altv->set_text(buf);
}

int32_t RouteEditUi::get_alt_widgets(Gtk::Entry * altv, Gtk::ComboBox * altu, uint16_t& flags)
{
        int unit = altu->get_active_row_number();
        double v = strtod(altv->get_text().c_str(), NULL);
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
        return (int32_t)(v + 0.5);
}

void RouteEditUi::set_freq_widgets(Gtk::Entry * freqv, Gtk::Label *frequ, uint32_t freq)
{
        char buf[16];
        const char *unitstr = "";

        strncpy(buf, "0", sizeof(buf));
        if (freq >= 1000 && freq <= 999999) {
                snprintf(buf, sizeof(buf), "%d", (freq + 500) / 1000);
                unitstr = "kHz";
        } else if (freq >= 1000000 && freq <= 999999999) {
                snprintf(buf, sizeof(buf), "%.3f", freq * 1e-6);
                unitstr = "MHz";
        }
        freqv->set_text(buf);
        frequ->set_text(unitstr);
}

uint32_t RouteEditUi::get_freq_widgets(Gtk::Entry * freqv)
{
        std::locale loc("");
        const std::numpunct<char>& np = std::use_facet<std::numpunct<char> >(loc);
        const char *cp = freqv->get_text().c_str();
        //std::cerr << "converting frequency: \"" << cp << "\" decimal point char '" << np.decimal_point() << "'" << std::endl;
        if (strchr(cp, np.decimal_point())) {
                //std::cerr << "converting MHz frequency: \"" << cp << "\"" << std::endl;
                std::istringstream is(cp);
                is.imbue(loc); // use the user's locale for this stream
                double f;
                is >> f;
                return (int32_t)(f * 1e6 + 0.5);
        }
        return strtoul(cp, NULL, 10) * 1000;
}

void RouteEditUi::widget_changed(void)
{
        if (!m_editactive)
                return;
        m_widgetchange = true;
}

bool RouteEditUi::wptentry_focus_in_event(GdkEventFocus * event, Gtk::Entry * entry)
{
        if (!entry)
                return false;
        entry->select_region(0, -1);
        return false;
}

bool RouteEditUi::wpticao_focus_out_event(GdkEventFocus * event)
{
        if (!m_editactive)
                return false;
        if (m_widgetchange) {
                uint32_t nr = m_route.get_curwpt();
                if (nr >= 1 && nr <= m_route.get_nrwpt()) {
                        FPlanWaypoint& wpt(m_route[nr-1]);
			if (m_wpticao->get_text() != wpt.get_icao()) {
				wpt.set_icao(m_wpticao->get_text());
				wpt.set_type(FPlanWaypoint::type_user, FPlanWaypoint::navaid_invalid);
				route_save();
				std::cerr << "save icao: " << wpt.get_icao() << std::endl;
			}
                }
        }
        m_widgetchange = false;
        return false;
}

bool RouteEditUi::wptname_focus_out_event(GdkEventFocus * event)
{
        if (!m_editactive)
                return false;
        if (m_widgetchange) {
                uint32_t nr = m_route.get_curwpt();
                if (nr >= 1 && nr <= m_route.get_nrwpt()) {
                        FPlanWaypoint& wpt(m_route[nr-1]);
			if (m_wptname->get_text() != wpt.get_name()) {
				wpt.set_name(m_wptname->get_text());
				wpt.set_type(FPlanWaypoint::type_user, FPlanWaypoint::navaid_invalid);
				route_save();
				std::cerr << "save name: " << wpt.get_name() << std::endl;
			}
                }
        }
        m_widgetchange = false;
        return false;
}

bool RouteEditUi::wptpathname_focus_out_event(GdkEventFocus * event)
{
        if (!m_editactive)
                return false;
        if (m_widgetchange) {
                uint32_t nr = m_route.get_curwpt();
                if (nr >= 1 && nr <= m_route.get_nrwpt()) {
                        FPlanWaypoint& wpt(m_route[nr-1]);
                        wpt.set_pathname(m_wptpathname->get_text());
			if (wpt.get_pathcode() == FPlanWaypoint::pathcode_airway &&
			    wpt.get_pathname().uppercase() == "DCT") {
				wpt.set_pathcode(FPlanWaypoint::pathcode_directto);
				wpt.set_pathname("");
			}
			if (wpt.get_pathcode() == FPlanWaypoint::pathcode_directto &&
			    !wpt.get_pathname().empty())
				wpt.set_pathcode(FPlanWaypoint::pathcode_airway);
			m_wptpathname->set_sensitive(wpt.get_pathcode() != FPlanWaypoint::pathcode_none &&
						     wpt.get_pathcode() != FPlanWaypoint::pathcode_directto);
                        route_save();
                        std::cerr << "save pathname: " << wpt.get_pathname() << std::endl;
                }
        }
        m_widgetchange = false;
        return false;
}

void RouteEditUi::wptpathcode_changed(void)
{
        if (!m_editactive)
                return;
        uint32_t nr = m_route.get_curwpt();
        if (nr >= 1 && nr <= m_route.get_nrwpt()) {
                FPlanWaypoint& wpt(m_route[nr-1]);
                std::cerr << "pathcode: " << m_wptpathcode->get_active_row_number() << std::endl;
		wpt.set_pathcode(static_cast<FPlanWaypoint::pathcode_t>(m_wptpathcode->get_active_row_number()));
		if (wpt.get_pathcode() == FPlanWaypoint::pathcode_airway &&
		    wpt.get_pathname().uppercase() == "DCT") {
			wpt.set_pathcode(FPlanWaypoint::pathcode_directto);
			wpt.set_pathname("");
		}
		if (wpt.get_pathcode() == FPlanWaypoint::pathcode_directto)
			wpt.set_pathname("");
		m_wptpathname->set_sensitive(wpt.get_pathcode() != FPlanWaypoint::pathcode_none &&
					     wpt.get_pathcode() != FPlanWaypoint::pathcode_directto);
                route_save();
                std::cerr << "save pathcode: " << wpt.get_pathcode_name() << std::endl;
        }
}

void RouteEditUi::wptlonsign_changed(void)
{
        if (!m_editactive)
                return;
        uint32_t nr = m_route.get_curwpt();
        if (nr >= 1 && nr <= m_route.get_nrwpt()) {
                FPlanWaypoint& wpt(m_route[nr-1]);
                std::cerr << "lonsign: " << m_wptlonsign->get_active_row_number() << std::endl;
		Point pt(wpt.get_coord());
		pt.set_lon_deg(get_coord_widgets(m_wptlonsign, m_wptlondeg, m_wptlonmin));
                if (wpt.get_coord() != pt) {
			wpt.set_coord(pt);
			wpt.set_type(FPlanWaypoint::type_user, FPlanWaypoint::navaid_invalid);
			route_save();
			std::cerr << "save lon: " << wpt.get_coord().get_lon_str2() << std::endl;
		}
                set_coord_widgets(m_wptlonsign, m_wptlondeg, m_wptlonmin, wpt.get_coord().get_lon_deg());
        }
}

bool RouteEditUi::wptlon_focus_out_event(GdkEventFocus * event)
{
        if (!m_editactive)
                return false;
        if (m_widgetchange) {
                uint32_t nr = m_route.get_curwpt();
                if (nr >= 1 && nr <= m_route.get_nrwpt()) {
                        FPlanWaypoint& wpt(m_route[nr-1]);
			Point pt(wpt.get_coord());
			pt.set_lon_deg(get_coord_widgets(m_wptlonsign, m_wptlondeg, m_wptlonmin));
			if (wpt.get_coord() != pt) {
				wpt.set_coord(pt);
				wpt.set_type(FPlanWaypoint::type_user, FPlanWaypoint::navaid_invalid);
				route_save();
				std::cerr << "save lon: " << wpt.get_coord().get_lon_str2() << std::endl;
			}
			set_coord_widgets(m_wptlonsign, m_wptlondeg, m_wptlonmin, wpt.get_coord().get_lon_deg());
                }
        }
        m_widgetchange = false;
        return false;
}

void RouteEditUi::wptlatsign_changed(void)
{
        if (!m_editactive)
                return;
        uint32_t nr = m_route.get_curwpt();
        if (nr >= 1 && nr <= m_route.get_nrwpt()) {
                FPlanWaypoint& wpt(m_route[nr-1]);
		Point pt(wpt.get_coord());
		pt.set_lat_deg(get_coord_widgets(m_wptlatsign, m_wptlatdeg, m_wptlatmin));
                if (wpt.get_coord() != pt) {
			wpt.set_coord(pt);
			wpt.set_type(FPlanWaypoint::type_user, FPlanWaypoint::navaid_invalid);
			route_save();
			std::cerr << "save lat: " << wpt.get_coord().get_lat_str2() << std::endl;
		}
                set_coord_widgets(m_wptlatsign, m_wptlatdeg, m_wptlatmin, wpt.get_coord().get_lat_deg());
	}
}

bool RouteEditUi::wptlat_focus_out_event(GdkEventFocus * event)
{
        if (!m_editactive)
                return false;
        if (m_widgetchange) {
                uint32_t nr = m_route.get_curwpt();
                if (nr >= 1 && nr <= m_route.get_nrwpt()) {
                        FPlanWaypoint& wpt(m_route[nr-1]);
			Point pt(wpt.get_coord());
			pt.set_lat_deg(get_coord_widgets(m_wptlatsign, m_wptlatdeg, m_wptlatmin));
			if (wpt.get_coord() != pt) {
				wpt.set_coord(pt);
				wpt.set_type(FPlanWaypoint::type_user, FPlanWaypoint::navaid_invalid);
				route_save();
				std::cerr << "save lat: " << wpt.get_coord().get_lat_str2() << std::endl;
			}
			set_coord_widgets(m_wptlatsign, m_wptlatdeg, m_wptlatmin, wpt.get_coord().get_lat_deg());
                }
        }
        m_widgetchange = false;
        return false;
}

bool RouteEditUi::wptalt_focus_out_event(GdkEventFocus * event)
{
        if (!m_editactive)
                return false;
        if (m_widgetchange) {
                uint32_t nr = m_route.get_curwpt();
                if (nr >= 1 && nr <= m_route.get_nrwpt()) {
                        FPlanWaypoint& wpt(m_route[nr-1]);
                        uint16_t flags = wpt.get_flags();
                        wpt.set_altitude(get_alt_widgets(m_wptalt, m_wptaltunit, flags));
                        wpt.set_flags(flags);
                        set_alt_widgets(m_wptalt, m_wptaltunit, wpt.get_altitude(), wpt.get_flags(), m_wptaltunit->get_active_row_number());
                        route_save();
			m_wptoat->set_value(wpt.get_oat_kelvin() - IcaoAtmosphere<float>::degc_to_kelvin);
                        std::cerr << "save alt: " << Conversions::alt_str(wpt.get_altitude(), wpt.get_flags()) << std::endl;
                }
        }
        m_widgetchange = false;
        return false;
}

void RouteEditUi::wptaltunit_changed(void)
{
        if (!m_editactive)
                return;
        uint32_t nr = m_route.get_curwpt();
        if (nr >= 1 && nr <= m_route.get_nrwpt()) {
                FPlanWaypoint& wpt(m_route[nr-1]);
                uint16_t flags = wpt.get_flags();
                get_alt_widgets(m_wptalt, m_wptaltunit, flags);
                wpt.set_flags(flags);
                set_alt_widgets(m_wptalt, m_wptaltunit, wpt.get_altitude(), wpt.get_flags(), m_wptaltunit->get_active_row_number());
                route_save();
		m_wptoat->set_value(wpt.get_oat_kelvin() - IcaoAtmosphere<float>::degc_to_kelvin);
                std::cerr << "save alt: " << Conversions::alt_str(wpt.get_altitude(), wpt.get_flags()) << std::endl;
        }
}

bool RouteEditUi::wptfreq_focus_out_event(GdkEventFocus * event)
{
        if (!m_editactive)
                return false;
        if (m_widgetchange) {
                uint32_t nr = m_route.get_curwpt();
                if (nr >= 1 && nr <= m_route.get_nrwpt()) {
                        FPlanWaypoint& wpt(m_route[nr-1]);
                        wpt.set_frequency(get_freq_widgets(m_wptfreq));
                        set_freq_widgets(m_wptfreq, m_wptfrequnit, wpt.get_frequency());
                        route_save();
                        std::cerr << "save freq: " << wpt.get_frequency() << std::endl;
                }
        }
        m_widgetchange = false;
        return false;
}

void RouteEditUi::wptifr_toggled(void)
{
        if (!m_editactive)
                return;
        uint32_t nr = m_route.get_curwpt();
        if (nr >= 1 && nr <= m_route.get_nrwpt()) {
                FPlanWaypoint& wpt(m_route[nr-1]);
                uint16_t flags = wpt.get_flags();
		if (m_wptifr->get_active())
			flags |= FPlanWaypoint::altflag_ifr;
		else
			flags &= ~FPlanWaypoint::altflag_ifr;
                wpt.set_flags(flags);
                route_save();
        }
}

bool RouteEditUi::wptwinddir_focus_out_event(GdkEventFocus* event)
{
        if (!m_editactive)
                return false;
        if (m_widgetchange) {
                uint32_t nr = m_route.get_curwpt();
                if (nr >= 1 && nr <= m_route.get_nrwpt()) {
                        FPlanWaypoint& wpt(m_route[nr-1]);
			wpt.set_winddir_deg(m_wptwinddir->get_value());
                        route_save();
                        std::cerr << "save winddir: " << wpt.get_winddir_deg() << std::endl;
                }
        }
        m_widgetchange = false;
        return false;
}

bool RouteEditUi::wptwindspeed_focus_out_event(GdkEventFocus* event)
{
        if (!m_editactive)
                return false;
        if (m_widgetchange) {
                uint32_t nr = m_route.get_curwpt();
                if (nr >= 1 && nr <= m_route.get_nrwpt()) {
                        FPlanWaypoint& wpt(m_route[nr-1]);
			wpt.set_windspeed_kts(m_wptwindspeed->get_value());
                        route_save();
                        std::cerr << "save windspeed: " << wpt.get_windspeed_kts() << std::endl;
                }
        }
        m_widgetchange = false;
        return false;
}

bool RouteEditUi::wptqff_focus_out_event(GdkEventFocus* event)
{
        if (!m_editactive)
                return false;
        if (m_widgetchange) {
                uint32_t nr = m_route.get_curwpt();
                if (nr >= 1 && nr <= m_route.get_nrwpt()) {
                        FPlanWaypoint& wpt(m_route[nr-1]);
			wpt.set_qff_hpa(m_wptqff->get_value());
                        route_save();
                        std::cerr << "save qff: " << wpt.get_qff_hpa() << std::endl;
                }
        }
        m_widgetchange = false;
        return false;
}

bool RouteEditUi::wptoat_focus_out_event(GdkEventFocus* event)
{
        if (!m_editactive)
                return false;
        if (m_widgetchange) {
                uint32_t nr = m_route.get_curwpt();
                if (nr >= 1 && nr <= m_route.get_nrwpt()) {
                        FPlanWaypoint& wpt(m_route[nr-1]);
			wpt.set_oat_kelvin(m_wptoat->get_value() + IcaoAtmosphere<float>::degc_to_kelvin);
                        route_save();
                        std::cerr << "save oat: " << (wpt.get_oat_kelvin() - IcaoAtmosphere<float>::degc_to_kelvin) << std::endl;
                }
        }
        m_widgetchange = false;
        return false;
}

bool RouteEditUi::wptnote_focus_out_event(GdkEventFocus * event)
{
        if (!m_editactive)
                return false;
        if (m_widgetchange) {
                uint32_t nr = m_route.get_curwpt();
                if (nr >= 1 && nr <= m_route.get_nrwpt()) {
                        FPlanWaypoint& wpt(m_route[nr-1]);
                        wpt.set_note(m_wptnotebuffer->get_text());
                        route_save();
                        std::cerr << "save note: " << wpt.get_note() << std::endl;
                }
        }
        m_widgetchange = false;
        return false;
}

bool RouteEditUi::routenote_focus_out_event(GdkEventFocus * event)
{
        if (!m_editactive)
                return false;
        if (m_widgetchange) {
                m_route.set_note(m_routenotebuffer->get_text());
                route_save();
                std::cerr << "save note: " << m_route.get_note() << std::endl;
        }
        m_widgetchange = false;
        return false;
}

void RouteEditUi::wptprev_clicked(void)
{
        uint16_t nr = m_route.get_curwpt();
        if (nr > 1) {
                m_routeeditwindow->unset_focus();
                nr--;
                m_route.set_curwpt(nr);
                waypointlist_update();
                waypointlist_selection_update();
        }
}

void RouteEditUi::wptnext_clicked(void)
{
        uint16_t nr = m_route.get_curwpt();
        if (nr <= m_route.get_nrwpt()) {
                m_routeeditwindow->unset_focus();
                nr++;
                m_route.set_curwpt(nr);
                waypointlist_update();
                waypointlist_selection_update();
        }
}

void RouteEditUi::wptfind_clicked(void)
{
        m_findnearestwindow->hide();
        m_mapwindow->hide();
        uint16_t nr = m_route.get_curwpt();
        if (nr > 0 && nr < m_route.get_nrwpt() + 1) {
                m_findnamewindow->set_center(m_route[nr-1].get_coord());
        }
        m_findnamewindow->present();
}

void RouteEditUi::wptfindnearest_clicked(void)
{
        m_findnamewindow->hide();
        m_mapwindow->hide();
        uint16_t nr = m_route.get_curwpt();
        if (nr > 0 && nr < m_route.get_nrwpt() + 1) {
                m_findnearestwindow->set_center(m_route[nr-1].get_coord());
        }
        m_findnearestwindow->present();
}

void RouteEditUi::wptmaps_clicked(void)
{
        m_findnearestwindow->hide();
        m_findnamewindow->hide();
        uint16_t nr = m_route.get_curwpt();
        if (nr > 0 && nr < m_route.get_nrwpt() + 1) {
                m_mapwindow->set_renderer(VectorMapArea::renderer_vmap);
                m_mapwindow->set_coord(m_route[nr-1].get_coord(), m_route[nr-1].get_altitude());
                m_mapwindow->present();
        }
}

void RouteEditUi::wptterrain_clicked(void)
{
        m_findnearestwindow->hide();
        m_findnamewindow->hide();
        uint16_t nr = m_route.get_curwpt();
        if (nr > 0 && nr < m_route.get_nrwpt() + 1) {
                m_mapwindow->set_renderer(VectorMapArea::renderer_terrain);
                m_mapwindow->set_coord(m_route[nr-1].get_coord(), m_route[nr-1].get_altitude());
                m_mapwindow->present();
        }
}

void RouteEditUi::wptairportdiagram_clicked(void)
{
        m_findnearestwindow->hide();
        m_findnamewindow->hide();
        uint16_t nr = m_route.get_curwpt();
        if (nr > 0 && nr < m_route.get_nrwpt() + 1) {
                m_mapwindow->set_renderer(VectorMapArea::renderer_airportdiagram);
                m_mapwindow->set_coord(m_route[nr-1].get_coord(), m_route[nr-1].get_altitude());
                m_mapwindow->present();
        }
}

void RouteEditUi::set_current_waypoint(const FPlanWaypoint & wpt)
{
        uint16_t nr = m_route.get_curwpt();
        if ((nr < 1) || (nr > m_route.get_nrwpt()))
                return;
        FPlanWaypoint& wpt1(m_route[nr-1]);
        std::cerr << "set_current_waypoint called" << std::endl;
        wpt1.set_icao(wpt.get_icao());
        wpt1.set_name(wpt.get_name());
        wpt1.set_pathcode(wpt.get_pathcode());
        wpt1.set_pathname(wpt.get_pathname());
        wpt1.set_coord(wpt.get_coord());
        wpt1.set_altitude(wpt.get_altitude());
        wpt1.set_flags(wpt.get_flags());
        wpt1.set_frequency(wpt.get_frequency());
	wpt1.set_winddir(wpt.get_winddir());
	wpt1.set_windspeed(wpt.get_windspeed());
	wpt1.set_qff(wpt.get_qff());
	wpt1.set_isaoffset(wpt.get_isaoffset());
        wpt1.set_note(wpt.get_note());
        route_save();
}

void RouteEditUi::routeeditnotebook_switch_page(guint page_num)
{
        m_routemenu->set_sensitive(page_num == 0);
        m_waypointmenu->set_sensitive(page_num == 1 || page_num == 2);
}

void RouteEditUi::menu_newroute(void)
{
        m_route.new_fp();
        m_route.set_note(Glib::ustring()); // make dirty
        m_route.save_fp();
        signal_updateroute()(m_route.get_id());
        route_save();
        route_selection();
}

void RouteEditUi::menu_duplicateroute(void)
{
        m_route.duplicate_fp();
        m_route.save_fp();
        signal_updateroute()(m_route.get_id());
        route_save();
        route_selection();
}

void RouteEditUi::menu_deleteroute(void)
{
        m_confirmdeletewindow->present();
}

void RouteEditUi::menu_reverseroute(void)
{
        m_route.reverse_fp();
        m_route.save_fp();
        signal_updateroute()(m_route.get_id());
        route_save();
        route_selection();
}

void RouteEditUi::menu_importicaofplroute(void)
{
	m_icaofplimportwindow->show();
}

void RouteEditUi::menu_exporticaofplroute(void)
{
	IcaoFlightPlan fpl(m_engine);
	fpl.populate(m_route);
        m_infowindow->clear_text();
	m_infowindow->append_text(fpl.get_fpl());
        m_infowindow->present();
}

void RouteEditUi::menu_sunrisesunset(void)
{
        m_sunrisesunsetwindow->present();
}

void RouteEditUi::menu_startnavigate(void)
{
        signal_startnav()(m_route.get_id());
        //m_routeeditwindow->hide();
}

void RouteEditUi::end_navigate(void)
{
        m_routeeditwindow->present();
}

void RouteEditUi::menu_insertwaypoint(void)
{
        uint16_t nr = m_route.get_curwpt();
        uint16_t nrwpt = m_route.get_nrwpt();
        if (nr > nrwpt)
                return;
        m_routeeditwindow->unset_focus();
        FPlanWaypoint wpt;
        if (nr > 0 && nr <= nrwpt)
                wpt = m_route[nr-1];
        m_route.insert_wpt(nr, wpt);
        m_route.set_curwpt(nr + 1);
        route_save();
}

void RouteEditUi::menu_swapwaypoint(void)
{
        uint16_t nr = m_route.get_curwpt();
        uint16_t nrwpt = m_route.get_nrwpt();
        if (nr < 1 || nr >= nrwpt)
                return;
        m_routeeditwindow->unset_focus();
        FPlanWaypoint wpt = m_route[nr-1];
        m_route[nr-1] = m_route[nr];
        m_route[nr] = wpt;
        route_save();
}

void RouteEditUi::menu_deletewaypoint(void)
{
        uint16_t nr = m_route.get_curwpt();
        uint16_t nrwpt = m_route.get_nrwpt();
        if (nr < 1 || nr > nrwpt)
                return;
        m_routeeditwindow->unset_focus();
        m_route.delete_wpt(nr - 1);
        route_save();
}

void RouteEditUi::confirm_delete_route(void)
{
        route_delete();
}

void RouteEditUi::map_set_coord(const Point & pt)
{
        uint16_t nr = m_route.get_curwpt();
        uint16_t nrwpt = m_route.get_nrwpt();
        if (nr < 1 || nr > nrwpt)
                return;
        m_routeeditwindow->unset_focus();
        FPlanWaypoint& wpt(m_route[nr-1]);
        wpt.set_coord(pt);
        route_save();
        map_set_coord_async_cancel();
        m_mapcoord_elevation = m_engine.async_elevation_point(pt);
        if (m_mapcoord_elevation) {
                m_mapcoord_dispatch_conn = m_mapcoord_dispatch.connect(sigc::mem_fun(*this, &RouteEditUi::map_set_coord_async_done));
                m_mapcoord_elevation->connect(sigc::mem_fun(*this, &RouteEditUi::map_set_coord_async_dispatch_done));
        }
}

void RouteEditUi::map_set_coord_async_dispatch_done(void)
{
        m_mapcoord_dispatch();
}

void RouteEditUi::map_set_coord_async_done(void)
{
        if (m_mapcoord_elevation && m_mapcoord_elevation->is_done()) {
                if (!m_mapcoord_elevation->is_error()) {
                        uint16_t nr = m_route.get_curwpt();
                        uint16_t nrwpt = m_route.get_nrwpt();
                        if (nr < 1 || nr > nrwpt)
                                return;
                        FPlanWaypoint& wpt(m_route[nr-1]);
                        int elev = m_mapcoord_elevation->get_result();
                        if (elev != TopoDb30::nodata) {
                                if (elev == TopoDb30::ocean)
                                        elev = 0;
                                elev = (int)(elev * Point::ft_to_m);
                                if (wpt.get_altitude() < elev)
                                        wpt.set_altitude(elev);
                        }
                        route_save();
                }
                m_mapcoord_elevation = Glib::RefPtr<Engine::ElevationResult>();
        }
}

void RouteEditUi::map_set_coord_async_cancel(void)
{
        Engine::ElevationResult::cancel(m_mapcoord_elevation);
        m_mapcoord_dispatch_conn.disconnect();
}

void RouteEditUi::distance_insert_coord(const Point & pt, bool before)
{
        uint16_t nr = m_route.get_curwpt();
        uint16_t nrwpt = m_route.get_nrwpt();
        if (nr < 1 || nr > nrwpt)
                return;
        if (before) {
                m_route.insert_wpt(nr - 1);
                m_route[nr-1] = m_route[nr];
                m_route[nr-1].set_coord(pt);
        } else {
                m_route.insert_wpt(nr);
                m_route[nr] = m_route[nr-1];
                m_route[nr].set_coord(pt);
        }
        route_save();
}

void RouteEditUi::menu_waypointdistance(void)
{
        uint16_t nr = m_route.get_curwpt();
        uint16_t nrwpt = m_route.get_nrwpt();
        if ((nr < 1) || (nr > nrwpt) || (nrwpt <= 1))
                return;
        Point ptprev, ptnext;
        bool prevena(nr > 1), nextena(nr < nrwpt);
        if (prevena)
                ptprev = m_route[nr-2].get_coord();
        if (nextena)
                ptnext = m_route[nr].get_coord();
        m_distancewindow->set_coord(m_route[nr-1].get_coord(), ptprev, prevena, ptnext, nextena);
        m_distancewindow->present();
}

void RouteEditUi::menu_waypointcoursedistance(void)
{
        uint16_t nr = m_route.get_curwpt();
        uint16_t nrwpt = m_route.get_nrwpt();
        if (nr < 1 || nr > nrwpt)
                return;
        m_coursedistwindow->set_coord(m_route[nr-1].get_coord());
        m_coursedistwindow->present();
}

void RouteEditUi::menu_waypointstraighten(void)
{
        uint16_t nr = m_route.get_curwpt();
        uint16_t nrwpt = m_route.get_nrwpt();
        if (nr <= 1 || nr >= nrwpt)
                return;
        Point pt1(m_route[nr-2].get_coord()), pt2(m_route[nr-1].get_coord()), pt3(m_route[nr].get_coord());
        pt3 -= pt1;
        pt2 -= pt1;
        double t = pt3.get_lon() * (double)pt3.get_lon() + pt3.get_lat() * (double)pt3.get_lat();
        if (t < 1)
                return;
        t = (pt2.get_lon() * (double)pt3.get_lon() + pt2.get_lat() * (double)pt3.get_lat()) / t;
        pt3 = Point((int)(pt3.get_lon() * t), (int)(pt3.get_lat() * t));
        pt3 += pt1;
        m_route[nr-1].set_coord(pt3);
        route_save();
}

void RouteEditUi::menu_waypointcoordch1903(void)
{
        uint16_t nr = m_route.get_curwpt();
        uint16_t nrwpt = m_route.get_nrwpt();
        if (nr < 1 || nr > nrwpt)
                return;
        m_ch1903window->set_coord(m_route[nr-1].get_coord());
        m_ch1903window->present();
}

void RouteEditUi::menu_waypointcoordmaidenhead(void)
{
        uint16_t nr = m_route.get_curwpt();
        uint16_t nrwpt = m_route.get_nrwpt();
        if (nr < 1 || nr > nrwpt)
                return;
        m_maidenheadwindow->set_coord(m_route[nr-1].get_coord());
        m_maidenheadwindow->present();
}

void RouteEditUi::map_info_coord(const Point& pt, const Rect& bbox)
{
        map_info_async_cancel();
        m_mapinfo_elevation = m_engine.async_elevation_point(pt);
        m_mapinfo_mapelementquery = m_engine.async_mapelement_find_bbox(bbox, ~0, MapelementsDb::element_t::subtables_none);
        m_mapinfo_navaidquery = m_engine.async_navaid_find_bbox(bbox, ~0, NavaidsDb::element_t::subtables_none);
        m_mapinfo_airportquery = m_engine.async_airport_find_bbox(bbox,  ~0, AirportsDb::element_t::subtables_vfrroutes | AirportsDb::element_t::subtables_runways | AirportsDb::element_t::subtables_helipads | AirportsDb::element_t::subtables_comms);
        m_mapinfo_airspacequery = m_engine.async_airspace_find_bbox(bbox, ~0, AirspacesDb::element_t::subtables_none);
        m_mapinfo_waypointquery = m_engine.async_waypoint_find_bbox(bbox, ~0, WaypointsDb::element_t::subtables_none);
        m_mapinfo_airwayquery = m_engine.async_airway_find_bbox(pt.simple_box_nmi(0.5 * airway_width), ~0, AirwaysDb::element_t::subtables_none);
        m_infowindow->clear_text();
        m_mapinfo_dispatch_conn = m_mapinfo_dispatch.connect(sigc::bind(sigc::mem_fun(*this, &RouteEditUi::map_info_async_done), pt, bbox));
        sigc::slot<void> asyncdone(sigc::mem_fun(*this, &RouteEditUi::map_info_async_dispatch_done));
        if (m_mapinfo_elevation) {
                m_mapinfo_elevation->connect(asyncdone);
        }
        if (m_mapinfo_mapelementquery) {
                m_mapinfo_mapelementquery->connect(asyncdone);
        }
        if (m_mapinfo_navaidquery) {
                m_mapinfo_navaidquery->connect(asyncdone);
        }
        if (m_mapinfo_airportquery) {
                m_mapinfo_airportquery->connect(asyncdone);
        }
        if (m_mapinfo_airspacequery) {
                m_mapinfo_airspacequery->connect(asyncdone);
        }
        if (m_mapinfo_waypointquery) {
                m_mapinfo_waypointquery->connect(asyncdone);
        }
        if (m_mapinfo_airwayquery) {
                m_mapinfo_airwayquery->connect(asyncdone);
        }
        m_infowindow->present();
}

void RouteEditUi::map_info_async_dispatch_done(void)
{
        m_mapinfo_dispatch();
}

void RouteEditUi::map_info_async_done(const Point& pt, const Rect& bbox)
{
        if (m_mapinfo_elevation && m_mapinfo_elevation->is_done()) {
                m_infowindow->append_text("Info for Point " + pt.get_lon_str() + " " + pt.get_lat_str());
                if (!m_mapinfo_elevation->is_error()) {
                        int elev = m_mapinfo_elevation->get_result();
                        if (elev != TopoDb30::nodata) {
                                if (elev == TopoDb30::ocean)
                                        elev = 0;
                                std::ostringstream oss;
                                oss << " Elev " << elev << "m / " << Point::round<int,float>(elev * Point::m_to_ft) << "ft";
                                m_infowindow->append_text(oss.str());
                        }
                }
                m_infowindow->append_text("\n\n");
                //m_infowindow->append_text("Info for BBox " + bbox.get_southwest().get_lon_str() + " " + bbox.get_southwest().get_lat_str() +
                //                 " -> " + bbox.get_northeast().get_lon_str() + " " + bbox.get_northeast().get_lat_str() + "\n\n");
                m_mapinfo_elevation = Glib::RefPtr<Engine::ElevationResult>();
        }
        if (m_mapinfo_mapelementquery && m_mapinfo_mapelementquery->is_done()) {
                if (!m_mapinfo_mapelementquery->is_error()) {
                        const MapelementsDb::elementvector_t& mapel(m_mapinfo_mapelementquery->get_result());
                        for (MapelementsDb::elementvector_t::const_iterator i1(mapel.begin()), ie(mapel.end()); i1 != ie; i1++) {
                                const MapelementsDb::Mapelement& e(*i1);
                                if (!e.is_valid() || e.get_name().empty())
                                        continue;
                                if (!e.is_area())
                                        continue;
                                if (!e.get_polygon().windingnumber(pt))
                                        continue;
                                m_infowindow->append_text("Map Element: " + e.get_name() + " type: " + e.get_typename() + "\n\n");
                        //std::ostringstream oss;
                        //oss << "  winding " << e.get_poly().windingnumber(pt) << std::endl;
                        //m_infowindow->append_text(oss.str());
                        }
                }
                m_mapinfo_mapelementquery = Glib::RefPtr<Engine::MapelementResult>();
        }
        if (m_mapinfo_navaidquery && m_mapinfo_navaidquery->is_done()) {
                if (!m_mapinfo_navaidquery->is_error()) {
                        const NavaidsDb::elementvector_t& navaids(m_mapinfo_navaidquery->get_result());
                        for (NavaidsDb::elementvector_t::const_iterator i1(navaids.begin()), ie(navaids.end()); i1 != ie; i1++) {
                                const NavaidsDb::Navaid& e(*i1);
                                if (!e.is_valid())
                                        continue;
                                char buf[16];
                                snprintf(buf, sizeof(buf), "%d", e.get_range());
                                m_infowindow->append_text("Navaid: " + e.get_navaid_typename() + " " + e.get_icao() + " " + e.get_name() +
                                                          " Freq " + Conversions::freq_str(e.get_frequency()) + " Elev " + Conversions::alt_str(e.get_elev(), 0) +
                                                          "' range " + buf + "\n\n");
                        }
                }
                m_mapinfo_navaidquery = Glib::RefPtr<Engine::NavaidResult>();
        }
        if (m_mapinfo_airportquery && m_mapinfo_airportquery->is_done()) {
                if (!m_mapinfo_airportquery->is_error()) {
                        const AirportsDb::elementvector_t& airports(m_mapinfo_airportquery->get_result());
                        for (AirportsDb::elementvector_t::const_iterator i1(airports.begin()), ie(airports.end()); i1 != ie; i1++) {
                                const AirportsDb::Airport& e(*i1);
                                if (!e.is_valid())
                                        continue;
                                bool arpt = false;
                                for (unsigned int rte = 0; rte < e.get_nrvfrrte(); rte++) {
                                        const AirportsDb::Airport::VFRRoute& vfrrte(e.get_vfrrte(rte));
                                        bool rthdr = false;
                                        for (unsigned int wpt = 0; wpt < vfrrte.size(); wpt++) {
                                                const AirportsDb::Airport::VFRRoute::VFRRoutePoint& pt(vfrrte[wpt]);
                                                if (pt.is_at_airport())
                                                        continue;
                                                if (!bbox.is_inside(pt.get_coord()))
                                                        continue;
                                                if (!arpt) {
                                                        m_infowindow->append_text("Airport: " + e.get_icao() + " " + e.get_name() + " " +
                                                                                  e.get_type_string() + " " + Conversions::alt_str(e.get_elevation(), 0) + "'\n");
                                                        arpt = true;
                                                }
                                                if (!rthdr) {
                                                        m_infowindow->append_text("  VFR Route " + vfrrte.get_name() + "\n");
                                                        rthdr = true;
                                                }
                                                m_infowindow->append_text(Glib::ustring("    ") + (pt.is_compulsory() ? "Compulsory " : "") + "Waypoint " +
                                                                          pt.get_name() + " " + pt.get_altitude_string() + " " + pt.get_pathcode_string() +
                                                                          " " + pt.get_coord().get_lon_str() + " " + pt.get_coord().get_lat_str() + "\n");
                                        }
                                }
                                if (!bbox.is_inside(e.get_coord())) {
                                        m_infowindow->append_text("\n");
                                        continue;
                                }
                                if (!arpt)
                                        m_infowindow->append_text("Airport: " + e.get_icao() + " " + e.get_name() + " " +
                                                                  e.get_type_string() + " " + Conversions::alt_str(e.get_elevation(), 0) + "'\n");
                                m_infowindow->append_text("  BBox: " + e.get_bbox().get_southwest().get_lon_str() + ' ' + e.get_bbox().get_southwest().get_lat_str() +
                                                          " -> "  + e.get_bbox().get_northeast().get_lon_str() + ' ' + e.get_bbox().get_northeast().get_lat_str() + "\n");
                                for (unsigned int i = 0; i < e.get_nrrwy(); i++) {
                                        const AirportsDb::Airport::Runway& rwy(e.get_rwy(i));
                                        std::ostringstream oss;
                                        oss << rwy.get_ident_he();
                                        if (!rwy.is_he_usable())
                                                oss << "(CLSD)";
                                        oss << '/' << rwy.get_ident_le();
                                        if (!rwy.is_le_usable())
                                                oss << "(CLSD)";
                                        oss << ": "
                                            << (int)(rwy.get_length() * 0.3048) << ' ' << (int)(rwy.get_width() * 0.3048) << ' ' << rwy.get_surface()
                                            << ' ' << "LDA,TDA " << (int)(rwy.get_he_lda() * 0.3048) << ',' << (int)(rwy.get_he_tda() * 0.3048) << '/'
                                            << (int)(rwy.get_le_lda() * 0.3048) << ',' << (int)(rwy.get_le_tda() * 0.3048) << '\n';
                                        m_infowindow->append_text("Runway: " + oss.str());
                                }
                                for (unsigned int i = 0; i < e.get_nrhelipads(); i++) {
                                        const AirportsDb::Airport::Helipad& hp(e.get_helipad(i));
                                        std::ostringstream oss;
                                        oss << hp.get_ident();
                                        if (!hp.is_usable())
                                                oss << "(CLSD)";
                                        oss << ": "
                                            << (int)(hp.get_length() * 0.3048) << 'x' << (int)(hp.get_width() * 0.3048) << ' ' << hp.get_surface()
                                            << '\n';
                                        m_infowindow->append_text("Helipad: " + oss.str());
                                }
                                for (unsigned int i = 0; i < e.get_nrcomm(); i++) {
                                        const AirportsDb::Airport::Comm& comm(e.get_comm(i));
                                        m_infowindow->append_text("Comm: " + comm.get_name());
                                        for (unsigned int j = 0; j < 5; j++) {
                                                uint32_t f(comm[j]);
                                                if (!f)
                                                        continue;
                                                m_infowindow->append_text(" " + Conversions::freq_str(f));
                                        }
                                        m_infowindow->append_text("\n");
                                        if (!comm.get_sector().empty())
                                                m_infowindow->append_text("\t" + comm.get_sector() + "\n");
                                        if (!comm.get_oprhours().empty())
                                                m_infowindow->append_text("\t" + comm.get_oprhours() + "\n");
                                }
                                for (unsigned int rte = 0; rte < e.get_nrvfrrte(); rte++) {
                                        const AirportsDb::Airport::VFRRoute& vfrrte(e.get_vfrrte(rte));
                                        m_infowindow->append_text("  VFR Route " + vfrrte.get_name() + "\n");
                                        for (unsigned int wpt = 0; wpt < vfrrte.size(); wpt++) {
                                                const AirportsDb::Airport::VFRRoute::VFRRoutePoint& pt(vfrrte[wpt]);
                                                m_infowindow->append_text(Glib::ustring("    ") + (pt.is_compulsory() ? "Compulsory " : "") + "Waypoint " +
                                                                pt.get_name() + " " + pt.get_altitude_string() + " " + pt.get_pathcode_string() + "\n");
                                        }
                                }
                                if (!e.get_vfrrmk().empty())
                                        m_infowindow->append_text(e.get_vfrrmk() + "\n");
                                m_infowindow->append_text("\n");
                        }
                }
                m_mapinfo_airportquery = Glib::RefPtr<Engine::AirportResult>();
        }
        if (m_mapinfo_airspacequery && m_mapinfo_airspacequery->is_done()) {
                if (!m_mapinfo_airspacequery->is_error()) {
                        const AirspacesDb::elementvector_t& airspaces(m_mapinfo_airspacequery->get_result());
                        for (AirspacesDb::elementvector_t::const_iterator i1(airspaces.begin()), ie(airspaces.end()); i1 != ie; i1++) {
                                const AirspacesDb::Airspace& e(*i1);
                                if (!e.is_valid())
                                        continue;
                                if (!e.get_polygon().windingnumber(pt))
                                        continue;
                                m_infowindow->append_text("Airspace: " + e.get_icao() + " " + e.get_name() + " Class " + e.get_class_string() +
                                                          + " " + e.get_altlwr_string() + " " + e.get_altupr_string() + "\n");
                                m_infowindow->append_text("  Comm: " + e.get_commname());
                                for (unsigned int i = 0; i < 2; i++) {
                                        uint32_t f(e.get_commfreq(i));
                                        if (!f)
                                                continue;
                                        m_infowindow->append_text(" " + Conversions::freq_str(f));
                                }
                                m_infowindow->append_text("\n");
                                if (!e.get_classexcept().empty())
                                        m_infowindow->append_text(e.get_classexcept() + "\n");
                                if (!e.get_ident().empty())
                                        m_infowindow->append_text(e.get_ident() + "\n");
                                if (!e.get_efftime().empty())
                                        m_infowindow->append_text(e.get_efftime() + "\n");
                                if (!e.get_note().empty())
                                        m_infowindow->append_text(e.get_note() + "\n");
                                m_infowindow->append_text("\n");
                        }
                }
                m_mapinfo_airspacequery = Glib::RefPtr<Engine::AirspaceResult>();
        }
        if (m_mapinfo_waypointquery && m_mapinfo_waypointquery->is_done()) {
                if (!m_mapinfo_waypointquery->is_error()) {
                        const WaypointsDb::elementvector_t& waypoints(m_mapinfo_waypointquery->get_result());
                        for (WaypointsDb::elementvector_t::const_iterator i1(waypoints.begin()), ie(waypoints.end()); i1 != ie; i1++) {
                                const WaypointsDb::Waypoint& e(*i1);
                                if (!e.is_valid())
                                        continue;
                                m_infowindow->append_text("Waypoint: " + e.get_name() + " " + e.get_usage_name() + "\n\n");
                        }
                }
                m_mapinfo_waypointquery = Glib::RefPtr<Engine::WaypointResult>();
        }
        if (m_mapinfo_airwayquery && m_mapinfo_airwayquery->is_done()) {
                if (!m_mapinfo_airwayquery->is_error()) {
                        const AirwaysDb::elementvector_t& airways(m_mapinfo_airwayquery->get_result());
                        for (AirwaysDb::elementvector_t::const_iterator i1(airways.begin()), ie(airways.end()); i1 != ie; i1++) {
                                const AirwaysDb::Airway& e(*i1);
                                if (!e.is_valid())
                                        continue;
                                double dist;
                                {
                                        CartesianVector3Dd cfrom = PolarVector3Dd(e.get_begin_coord(), 0);
                                        CartesianVector3Dd cto = PolarVector3Dd(e.get_end_coord(), 0);
                                        CartesianVector3Dd cm = PolarVector3Dd(pt, 0);
                                        double t = cm.nearest_fraction(cfrom, cto);
                                        CartesianVector3Dd cproj = cfrom + (cto - cfrom) * t;
                                        dist = (cm - cproj).length() * (Point::km_to_nmi_dbl * 1e-3);
                                }
                                if (dist > 0.5 * airway_width)
                                        continue;
                                {
                                        std::ostringstream oss;
                                        oss << "Airway: " << e.get_name() << ' '
                                            << e.get_begin_name() << '-' << e.get_end_name() << ", FL"
                                            << e.get_base_level() << "-FL" << e.get_top_level() << ", "
                                            << e.get_type_name() << ", d=" << Conversions::dist_str(dist) << std::endl << std::endl;
                                        m_infowindow->append_text(oss.str());
                                }
                        }
                }
                m_mapinfo_airwayquery = Glib::RefPtr<Engine::AirwayResult>();
        }
}

void RouteEditUi::map_info_async_cancel(void)
{
        Engine::ElevationResult::cancel(m_mapinfo_elevation);
        Engine::MapelementResult::cancel(m_mapinfo_mapelementquery);
        Engine::NavaidResult::cancel(m_mapinfo_navaidquery);
        Engine::AirportResult::cancel(m_mapinfo_airportquery);
        Engine::AirspaceResult::cancel(m_mapinfo_airspacequery);
        Engine::WaypointResult::cancel(m_mapinfo_waypointquery);
        Engine::AirwayResult::cancel(m_mapinfo_airwayquery);
        m_mapinfo_dispatch_conn.disconnect();
}

Glib::ustring RouteEditUi::icaofpl_import(const Glib::ustring& txt)
{
	Glib::ustring::const_iterator txti(txt.begin()), txte(txt.end());
	for (; txti != txte && isspace(*txti); ++txti);
	while (txti != txte) {
		Glib::ustring::const_iterator txte2(txte);
		--txte2;
		if (!isspace(*txte2))
			break;
		txte = txte2;
	}
	if (txti == txte)
		return "";
	IcaoFlightPlan fpl(m_engine);
	{
		std::string txtt(txti, txte);
		IcaoFlightPlan::parseresult_t pr(fpl.parse(txtt.begin(), txtt.end()));
		if (!pr.second.empty()) {
			bool subseq(false);
			std::string err;
			for (IcaoFlightPlan::errors_t::const_iterator i(pr.second.begin()), e(pr.second.end()); i != e; ++i) {
				if (subseq)
					err += "; ";
				subseq = true;
				err += *i;
			}
			return err;
		}
	}
        m_route.new_fp();
        m_route.set_note(Glib::ustring()); // make dirty
	fpl.set_route(m_route);
        m_route.save_fp();
        signal_updateroute()(m_route.get_id());
        route_save();
        route_selection();
	return "";
}

void RouteEditUi::aboutdialog_response(int response)
{
        if (response == Gtk::RESPONSE_CLOSE || response == Gtk::RESPONSE_CANCEL)
                m_aboutdialog->hide();
}

void RouteEditUi::wx_setwind_clicked(void)
{
	if (!m_route.get_nrwpt())
		return;
	double winddir(0), windspeed(0);
	if (m_wxwinddir)
		winddir = m_wxwinddir->get_value();
	if (m_wxwindspeed)
		windspeed = m_wxwindspeed->get_value();
	for (unsigned int i = 0; i < m_route.get_nrwpt(); ++i) {
		FPlanWaypoint& wpt(m_route[i]);
		wpt.set_winddir_deg(winddir);
		wpt.set_windspeed_kts(windspeed);
	}
	wx_deptime_changed(); // force recompute of flight plan
}

void RouteEditUi::wx_setisaoffs_clicked(void)
{
	if (!m_route.get_nrwpt())
		return;
	double isaoffs(0);
	if (m_wxisaoffs)
		isaoffs = m_wxisaoffs->get_value();
	for (unsigned int i = 0; i < m_route.get_nrwpt(); ++i) {
		FPlanWaypoint& wpt(m_route[i]);
		wpt.set_isaoffset_kelvin(isaoffs);
	}
	wx_deptime_changed(); // force recompute of flight plan
}

void RouteEditUi::wx_setqff_clicked(void)
{
	if (!m_route.get_nrwpt())
		return;
	double qff(IcaoAtmosphere<double>::std_sealevel_pressure);
	if (m_wxqff)
		qff = m_wxqff->get_value();
	for (unsigned int i = 0; i < m_route.get_nrwpt(); ++i) {
		FPlanWaypoint& wpt(m_route[i]);
		wpt.set_qff_hpa(qff);
	}
	wx_deptime_changed(); // force recompute of flight plan
}

void RouteEditUi::wx_getnwx_clicked(void)
{
	if (!m_route.get_nrwpt())
		return;
 	{
		FPlanRoute route(m_route);
		Preferences& p(m_engine.get_prefs());
		time_t routetime(route[0].get_time_unix());
		for (unsigned int i = 1; i < route.get_nrwpt(); ++i) {
			route[i].set_windspeed(0);
			FPlanLeg leg = route.get_leg(i);
			leg.wind_correction(p.vcruise);
			routetime += leg.get_legtime();
			route[i].set_time_unix(routetime);
		}
		route.nwxweather(m_nwxweather, sigc::mem_fun(*this, &RouteEditUi::wx_fetchwind_cb));
	}
}

void RouteEditUi::wx_fetchwind_cb(bool fplmodif)
{
	if (fplmodif)
	 	wx_deptime_changed(); // force recompute of flight plan
}

void RouteEditUi::wx_deptime_changed(void)
{
	struct tm utm;
	{
		time_t t(m_route.get_time_offblock_unix());
		gmtime_r(&t, &utm);
	}
	if (m_wxoffblock) {
		guint year, month, day;
		m_wxoffblock->get_date(year, month, day);
		if (year) {
			utm.tm_year = year - 1900;
			utm.tm_mon = month;
		}
		if (day)
			utm.tm_mday = day;
	}
	if (m_wxoffblockh)
		utm.tm_hour = m_wxoffblockh->get_value();
	if (m_wxoffblockm)
		utm.tm_min = m_wxoffblockm->get_value();
	if (m_wxoffblocks)
		utm.tm_sec = m_wxoffblocks->get_value();
	unsigned int taxis(0U), taxim(5U);
	if (m_wxtaxitimem)
		taxim = m_wxtaxitimem->get_value();
	if (m_wxtaxitimes)
		taxis = m_wxtaxitimes->get_value();
	m_route.set_time_offblock_unix(timegm(&utm));
	if (m_route.get_nrwpt())
		m_route[0].set_time_unix(m_route.get_time_offblock_unix() + taxim * 60U + taxis);
	route_update();
}

void RouteEditUi::wx_cruisealt_changed(void)
{
	int curcrsalt(std::numeric_limits<int>::min());
	for (unsigned int i = 0; i < m_route.get_nrwpt(); ++i) {
		FPlanWaypoint& wpt(m_route[i]);
		curcrsalt = std::max(curcrsalt, static_cast<int>(wpt.get_altitude()));
	}
	int newcrsalt(curcrsalt);
	if (m_wxcruisealt)
		newcrsalt = m_wxcruisealt->get_value();
	for (unsigned int i = 0; i < m_route.get_nrwpt(); ++i) {
		FPlanWaypoint& wpt(m_route[i]);
		if (wpt.get_altitude() != curcrsalt)
			continue;
		wpt.set_altitude(newcrsalt);
	}
	wx_deptime_changed(); // force recompute of flight plan
}
