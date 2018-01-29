/***************************************************************************
 *   Copyright (C) 2007, 2008, 2009, 2012, 2013, 2016 by Thomas Sailer     *
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

#include <gtkmm/main.h>
#include <glibmm/optionentry.h>
#include <glibmm/optiongroup.h>
#include <glibmm/optioncontext.h>

#include "dbeditor.h"
#include "sysdepsgui.h"

WaypointEditor::WaypointModelColumns::WaypointModelColumns(void)
{
	add(m_col_id);
	add(m_col_coord);
	add(m_col_sourceid);
	add(m_col_icao);
	add(m_col_name);
	add(m_col_usage);
	add(m_col_type);
	add(m_col_label_placement);
	add(m_col_modtime);
}

WaypointEditor::WaypointEditor(const std::string & dir_main, Engine::auxdb_mode_t auxdbmode, const std::string & dir_aux)
	: m_dbchanged(false), m_engine(dir_main, auxdbmode, dir_aux, false, false), m_waypointeditorwindow(0), m_waypointeditortreeview(0), m_waypointeditorstatusbar(0), 
	  m_waypointeditorfind(0), m_waypointeditormap(0), m_waypointeditorundo(0), 
	  m_waypointeditorredo(0), m_waypointeditordeletewaypoint(0), m_waypointeditormapenable(0),  m_waypointeditorairportdiagramenable(0),
	  m_waypointeditormapzoomin(0), m_waypointeditormapzoomout(0), m_aboutdialog(0)
{
#ifdef HAVE_PQXX
	if (auxdbmode == Engine::auxdb_postgres) {
		m_pgconn = std::unique_ptr<pqxx::connection>(new pqxx::connection(dir_main));
		if (m_pgconn->get_variable("application_name").empty())
			m_pgconn->set_variable("application_name", "waypointeditor");
		m_db.reset(new WaypointsPGDb(*m_pgconn));
	} else
#endif
	{
		m_db.reset(new WaypointsDb(dir_main));
		std::cerr << "Main database " << (dir_main.empty() ? std::string("(default)") : dir_main) << std::endl;
		Glib::ustring dir_aux1(m_engine.get_aux_dir(auxdbmode, dir_aux));
		WaypointsDb *db(static_cast<WaypointsDb *>(m_db.get()));
		if (!dir_aux1.empty() && db->is_dbfile_exists(dir_aux1))
			db->attach(dir_aux1);
		if (db->is_aux_attached())
			std::cerr << "Auxillary waypoints database " << dir_aux1 << " attached" << std::endl;
	}
	m_refxml = load_glade_file("dbeditor" UIEXT, "waypointeditorwindow");
	get_widget_derived(m_refxml, "waypointeditorwindow", m_waypointeditorwindow);
	get_widget(m_refxml, "waypointeditortreeview", m_waypointeditortreeview);
	get_widget(m_refxml, "waypointeditorstatusbar", m_waypointeditorstatusbar);
	get_widget(m_refxml, "waypointeditorfind", m_waypointeditorfind);
	get_widget_derived(m_refxml, "waypointeditormap", m_waypointeditormap);
	{
#ifdef HAVE_GTKMM2
		Glib::RefPtr<Builder> refxml = load_glade_file_nosearch(m_refxml->get_filename(), "aboutdialog");
#else
		//Glib::RefPtr<Builder> refxml = load_glade_file("dbeditor" UIEXT, "aboutdialog");
		Glib::RefPtr<Builder> refxml = m_refxml;
#endif
		get_widget(refxml, "aboutdialog", m_aboutdialog);
		m_aboutdialog->signal_response().connect(sigc::mem_fun(*this, &WaypointEditor::aboutdialog_response));
	}
	Gtk::Button *buttonclear(0);
	get_widget(m_refxml, "waypointeditorclearbutton", buttonclear);
	buttonclear->signal_clicked().connect(sigc::mem_fun(*this, &WaypointEditor::buttonclear_clicked));
	m_waypointeditorfind->signal_changed().connect(sigc::mem_fun(*this, &WaypointEditor::find_changed));
	Gtk::MenuItem *menu_quit(0), *menu_newwaypoint(0), *menu_cut(0), *menu_copy(0), *menu_paste(0), *menu_delete(0), *menu_preferences(0), *menu_about(0);
	get_widget(m_refxml, "waypointeditorquit", menu_quit);
	get_widget(m_refxml, "waypointeditorundo", m_waypointeditorundo);
	get_widget(m_refxml, "waypointeditorredo", m_waypointeditorredo);
	get_widget(m_refxml, "waypointeditornewwaypoint", menu_newwaypoint);
	get_widget(m_refxml, "waypointeditordeletewaypoint", m_waypointeditordeletewaypoint);
	get_widget(m_refxml, "waypointeditormapenable", m_waypointeditormapenable);
	get_widget(m_refxml, "waypointeditorairportdiagramenable", m_waypointeditorairportdiagramenable);
	get_widget(m_refxml, "waypointeditormapzoomin", m_waypointeditormapzoomin);
	get_widget(m_refxml, "waypointeditormapzoomout", m_waypointeditormapzoomout);
	get_widget(m_refxml, "waypointeditorcut", menu_cut);
	get_widget(m_refxml, "waypointeditorcopy", menu_copy);
	get_widget(m_refxml, "waypointeditorpaste", menu_paste);
	get_widget(m_refxml, "waypointeditordelete", menu_delete);
	get_widget(m_refxml, "waypointeditorpreferences", menu_preferences);
	get_widget(m_refxml, "waypointeditorabout", menu_about);
	menu_quit->signal_activate().connect(sigc::mem_fun(*this, &WaypointEditor::menu_quit_activate));
	m_waypointeditorundo->signal_activate().connect(sigc::mem_fun(*this, &WaypointEditor::menu_undo_activate));
	m_waypointeditorredo->signal_activate().connect(sigc::mem_fun(*this, &WaypointEditor::menu_redo_activate));
	menu_newwaypoint->signal_activate().connect(sigc::mem_fun(*this, &WaypointEditor::menu_newwaypoint_activate));
	m_waypointeditordeletewaypoint->signal_activate().connect(sigc::mem_fun(*this, &WaypointEditor::menu_deletewaypoint_activate));
	m_waypointeditormapenable->signal_toggled().connect(sigc::mem_fun(*this, &WaypointEditor::menu_mapenable_toggled));
	m_waypointeditorairportdiagramenable->signal_toggled().connect(sigc::mem_fun(*this, &WaypointEditor::menu_airportdiagramenable_toggled));
	m_waypointeditormapzoomin->signal_activate().connect(sigc::mem_fun(*this, &WaypointEditor::menu_mapzoomin_activate));
	m_waypointeditormapzoomout->signal_activate().connect(sigc::mem_fun(*this, &WaypointEditor::menu_mapzoomout_activate));
	if (m_waypointeditormapenable->get_active()) {
		m_waypointeditormapzoomin->show();
		m_waypointeditormapzoomout->show();
	} else {
		m_waypointeditormapzoomin->hide();
		m_waypointeditormapzoomout->hide();
	}
	m_waypointeditorwindow->signal_zoomin().connect(sigc::bind_return(sigc::mem_fun(*this, &WaypointEditor::menu_mapzoomin_activate), true));
	m_waypointeditorwindow->signal_zoomout().connect(sigc::bind_return(sigc::mem_fun(*this, &WaypointEditor::menu_mapzoomout_activate), true));
	menu_cut->signal_activate().connect(sigc::mem_fun(*this, &WaypointEditor::menu_cut_activate));
	menu_copy->signal_activate().connect(sigc::mem_fun(*this, &WaypointEditor::menu_copy_activate));
	menu_paste->signal_activate().connect(sigc::mem_fun(*this, &WaypointEditor::menu_paste_activate));
	menu_delete->signal_activate().connect(sigc::mem_fun(*this, &WaypointEditor::menu_delete_activate));
	menu_preferences->signal_activate().connect(sigc::mem_fun(*this, &WaypointEditor::menu_prefs_activate));
	menu_about->signal_activate().connect(sigc::mem_fun(*this, &WaypointEditor::menu_about_activate));
	update_undoredo_menu();
	// Vector Map
	m_waypointeditormap->set_engine(m_engine);
	m_waypointeditormap->set_renderer(VectorMapArea::renderer_vmap);
	m_waypointeditormap->signal_cursor().connect(sigc::mem_fun(*this, &WaypointEditor::map_cursor));
	m_waypointeditormap->set_map_scale(m_engine.get_prefs().mapscale);
	WaypointEditor::map_drawflags(m_engine.get_prefs().mapflags);
	m_engine.get_prefs().mapflags.signal_change().connect(sigc::mem_fun(*this, &WaypointEditor::map_drawflags));
	// Waypoint List
	m_waypointliststore = Gtk::ListStore::create(m_waypointlistcolumns);
	m_waypointeditortreeview->set_model(m_waypointliststore);
	Gtk::CellRendererText *icao_renderer = Gtk::manage(new Gtk::CellRendererText());
	Gtk::TreeViewColumn *icao_column = m_waypointeditortreeview->get_column(m_waypointeditortreeview->append_column(_("ICAO"), *icao_renderer) - 1);
	if (icao_column) {
		icao_column->add_attribute(*icao_renderer, "text", m_waypointlistcolumns.m_col_icao);
		icao_column->set_sort_column(m_waypointlistcolumns.m_col_icao);
	}
	icao_renderer->signal_edited().connect(sigc::mem_fun(*this, &WaypointEditor::edited_icao));
	icao_renderer->set_property("editable", true);
	Gtk::CellRendererText *name_renderer = Gtk::manage(new Gtk::CellRendererText());
	Gtk::TreeViewColumn *name_column = m_waypointeditortreeview->get_column(m_waypointeditortreeview->append_column(_("Name"), *name_renderer) - 1);
	if (name_column) {
		name_column->add_attribute(*name_renderer, "text", m_waypointlistcolumns.m_col_name);
		name_column->set_sort_column(m_waypointlistcolumns.m_col_name);
	}
	name_renderer->signal_edited().connect(sigc::mem_fun(*this, &WaypointEditor::edited_name));
	name_renderer->set_property("editable", true);
	CoordCellRenderer *coord_renderer = Gtk::manage(new CoordCellRenderer());
	Gtk::TreeViewColumn *coord_column = m_waypointeditortreeview->get_column(m_waypointeditortreeview->append_column(_("Coordinate"), *coord_renderer) - 1);
	if (coord_column) {
		coord_column->add_attribute(*coord_renderer, coord_renderer->get_property_name(), m_waypointlistcolumns.m_col_coord);
		coord_column->set_sort_column(m_waypointlistcolumns.m_col_coord);
	}
	coord_renderer->set_property("editable", true);
	coord_renderer->signal_edited().connect(sigc::mem_fun(*this, &WaypointEditor::edited_coord));
	WaypointUsageRenderer *usage_renderer = Gtk::manage(new WaypointUsageRenderer());
	Gtk::TreeViewColumn *usage_column = m_waypointeditortreeview->get_column(m_waypointeditortreeview->append_column(_("Usage"), *usage_renderer) - 1);
	if (usage_column) {
		usage_column->add_attribute(*usage_renderer, usage_renderer->get_property_name(), m_waypointlistcolumns.m_col_usage);
		usage_column->set_sort_column(m_waypointlistcolumns.m_col_usage);
	}
	usage_renderer->set_property("editable", true);
	usage_renderer->signal_edited().connect(sigc::mem_fun(*this, &WaypointEditor::edited_usage));

	WaypointTypeRenderer *type_renderer = Gtk::manage(new WaypointTypeRenderer());
	Gtk::TreeViewColumn *type_column = m_waypointeditortreeview->get_column(m_waypointeditortreeview->append_column(_("Type"), *type_renderer) - 1);
	if (type_column) {
		type_column->add_attribute(*type_renderer, type_renderer->get_property_name(), m_waypointlistcolumns.m_col_type);
		type_column->set_sort_column(m_waypointlistcolumns.m_col_type);
	}
	type_renderer->set_property("editable", true);
	type_renderer->signal_edited().connect(sigc::mem_fun(*this, &WaypointEditor::edited_type));


	LabelPlacementRenderer *label_placement_renderer = Gtk::manage(new LabelPlacementRenderer());
	Gtk::TreeViewColumn *label_placement_column = m_waypointeditortreeview->get_column(m_waypointeditortreeview->append_column(_("Label"), *label_placement_renderer) - 1);
	if (label_placement_column) {
		label_placement_column->add_attribute(*label_placement_renderer, label_placement_renderer->get_property_name(), m_waypointlistcolumns.m_col_label_placement);
		label_placement_column->set_sort_column(m_waypointlistcolumns.m_col_label_placement);
	}
	label_placement_renderer->set_property("editable", true);
	label_placement_renderer->signal_edited().connect(sigc::mem_fun(*this, &WaypointEditor::edited_label_placement));
	Gtk::CellRendererText *srcid_renderer = Gtk::manage(new Gtk::CellRendererText());
	Gtk::TreeViewColumn *srcid_column = m_waypointeditortreeview->get_column(m_waypointeditortreeview->append_column(_("Source ID"), *srcid_renderer) - 1);
	if (srcid_column) {
		srcid_column->add_attribute(*icao_renderer, "text", m_waypointlistcolumns.m_col_sourceid);
		srcid_column->set_sort_column(m_waypointlistcolumns.m_col_sourceid);
	}
	srcid_renderer->signal_edited().connect(sigc::mem_fun(*this, &WaypointEditor::edited_sourceid));
	srcid_renderer->set_property("editable", true);
	DateTimeCellRenderer *modtime_renderer = Gtk::manage(new DateTimeCellRenderer());
	Gtk::TreeViewColumn *modtime_column = m_waypointeditortreeview->get_column(m_waypointeditortreeview->append_column(_("Mod Time"), *modtime_renderer) - 1);
	if (modtime_column) {
		modtime_column->add_attribute(*modtime_renderer, modtime_renderer->get_property_name(), m_waypointlistcolumns.m_col_modtime);
		modtime_column->set_sort_column(m_waypointlistcolumns.m_col_modtime);
	}
	modtime_renderer->set_property("editable", true);
	modtime_renderer->signal_edited().connect(sigc::mem_fun(*this, &WaypointEditor::edited_modtime));
	//m_waypointeditortreeview->append_column_editable(_("Mod Time"), m_waypointlistcolumns.m_col_modtime);
	for (unsigned int i = 0; i < 8; ++i) {
		Gtk::TreeViewColumn *col = m_waypointeditortreeview->get_column(i);
		if (!col)
			continue;
		col->set_resizable(true);
		col->set_reorderable(true);
	}
	m_waypointeditortreeview->columns_autosize();
	m_waypointeditortreeview->set_enable_search(true);
	m_waypointeditortreeview->set_search_column(m_waypointlistcolumns.m_col_name);
	// selection
	Glib::RefPtr<Gtk::TreeSelection> waypointeditor_selection = m_waypointeditortreeview->get_selection();
	waypointeditor_selection->set_mode(Gtk::SELECTION_SINGLE);
	waypointeditor_selection->signal_changed().connect(sigc::mem_fun(*this, &WaypointEditor::waypointeditor_selection_changed));
	//waypointeditor_selection->set_select_function(sigc::mem_fun(*this, &WaypointEditor::waypointeditor_select_function));
	waypointeditor_selection_changed();

	//set_waypointlist("");
	m_dbselectdialog.signal_byname().connect(sigc::mem_fun(*this, &WaypointEditor::dbselect_byname));
	m_dbselectdialog.signal_byrectangle().connect(sigc::mem_fun(*this, &WaypointEditor::dbselect_byrect));
	m_dbselectdialog.signal_bytime().connect(sigc::mem_fun(*this, &WaypointEditor::dbselect_bytime));
	m_dbselectdialog.show();
	m_prefswindow = PrefsWindow::create(m_engine.get_prefs());
	m_waypointeditorwindow->show();
}

WaypointEditor::~WaypointEditor()
{
	if (m_dbchanged) {
		std::cerr << "Analyzing database..." << std::endl;
		m_db->analyze();
		m_db->vacuum();
	}
	m_db->close();
}

void WaypointEditor::dbselect_byname(const Glib::ustring& s, WaypointsDb::comp_t comp)
{
	WaypointsDb::elementvector_t els(m_db->find_by_text(s, 0, comp, 0));
	m_waypointliststore->clear();
	for (WaypointsDb::elementvector_t::iterator ei(els.begin()), ee(els.end()); ei != ee; ++ei) {
		WaypointsDb::element_t& e(*ei);
		if (!e.is_valid())
			continue;
		Gtk::TreeModel::Row row(*(m_waypointliststore->append()));
		set_row(row, e);
	}
}

void WaypointEditor::dbselect_byrect(const Rect& r)
{
	WaypointsDb::elementvector_t els(m_db->find_by_rect(r, 0));
	m_waypointliststore->clear();
	for (WaypointsDb::elementvector_t::iterator ei(els.begin()), ee(els.end()); ei != ee; ++ei) {
		WaypointsDb::element_t& e(*ei);
		if (!e.is_valid())
			continue;
		Gtk::TreeModel::Row row(*(m_waypointliststore->append()));
		set_row(row, e);
	}
}

void WaypointEditor::dbselect_bytime(time_t timefrom, time_t timeto)
{
	WaypointsDb::elementvector_t els(m_db->find_by_time(timefrom, timeto, 0));
	m_waypointliststore->clear();
	for (WaypointsDb::elementvector_t::iterator ei(els.begin()), ee(els.end()); ei != ee; ++ei) {
		WaypointsDb::element_t& e(*ei);
		if (!e.is_valid())
			continue;
		Gtk::TreeModel::Row row(*(m_waypointliststore->append()));
		set_row(row, e);
	}
}

void WaypointEditor::buttonclear_clicked(void)
{
	m_waypointeditorfind->set_text("");
}

void WaypointEditor::find_changed(void)
{
	dbselect_byname(m_waypointeditorfind->get_text());
}

void WaypointEditor::menu_quit_activate(void)
{
	m_waypointeditorwindow->hide();
}

void WaypointEditor::menu_undo_activate(void)
{
	WaypointsDb::Waypoint e(m_undoredo.undo());
	if (e.is_valid()) {
		save_nostack(e);
		select(e);
	}
	update_undoredo_menu();
}

void WaypointEditor::menu_redo_activate(void)
{
	WaypointsDb::Waypoint e(m_undoredo.redo());
	if (e.is_valid()) {
		save_nostack(e);
		select(e);
	}
	update_undoredo_menu();
}

void WaypointEditor::menu_newwaypoint_activate(void)
{
	WaypointsDb::Waypoint e;
	e.set_usage(WaypointsDb::Waypoint::usage_lowlevel);
	e.set_type(WaypointsDb::Waypoint::type_unknown);
	e.set_sourceid(make_sourceid());
	e.set_modtime(time(0));
	save(e);
	select(e);
}

void WaypointEditor::menu_deletewaypoint_activate(void)
{
	Glib::RefPtr<Gtk::TreeSelection> waypointeditor_selection = m_waypointeditortreeview->get_selection();
	Gtk::TreeModel::iterator iter = waypointeditor_selection->get_selected();
	if (!iter)
		return;
	Gtk::TreeModel::Row row = *iter;
	std::cerr << "deleting row: " << row[m_waypointlistcolumns.m_col_id] << std::endl;
	WaypointsDb::Waypoint e((*m_db)(row[m_waypointlistcolumns.m_col_id]));
	if (!e.is_valid())
		return;
	m_undoredo.erase(e);
	m_db->erase(e);
	m_waypointliststore->erase(iter);
}

void WaypointEditor::menu_cut_activate(void)
{
	Gtk::Widget *focus = m_waypointeditorwindow->get_focus();
	if (!focus)
		return;
	Gtk::Editable *edit = dynamic_cast<Gtk::Editable *>(focus);
	if (!edit)
		return;
	edit->cut_clipboard();
}

void WaypointEditor::menu_copy_activate(void)
{
	Gtk::Widget *focus = m_waypointeditorwindow->get_focus();
	if (!focus)
		return;
	Gtk::Editable *edit = dynamic_cast<Gtk::Editable *>(focus);
	if (!edit)
		return;
	edit->copy_clipboard();
}

void WaypointEditor::menu_paste_activate(void)
{
	Gtk::Widget *focus = m_waypointeditorwindow->get_focus();
	if (!focus)
		return;
	Gtk::Editable *edit = dynamic_cast<Gtk::Editable *>(focus);
	if (!edit)
		return;
	edit->paste_clipboard();
}

void WaypointEditor::menu_delete_activate(void)
{
	Gtk::Widget *focus = m_waypointeditorwindow->get_focus();
	if (!focus)
		return;
	Gtk::Editable *edit = dynamic_cast<Gtk::Editable *>(focus);
	if (!edit)
		return;
	edit->delete_selection();
}

void WaypointEditor::menu_mapenable_toggled(void)
{
	if (m_waypointeditormapenable->get_active()) {
		m_waypointeditorairportdiagramenable->set_active(false);
		m_waypointeditormap->set_renderer(VectorMapArea::renderer_vmap);
		m_waypointeditormap->set_map_scale(m_engine.get_prefs().mapscale);
		m_waypointeditormap->show();
		m_waypointeditormapzoomin->show();
		m_waypointeditormapzoomout->show();
	} else {
		m_waypointeditormap->hide();
		m_waypointeditormapzoomin->hide();
		m_waypointeditormapzoomout->hide();
	}
}

void WaypointEditor::menu_airportdiagramenable_toggled(void)
{
	if (m_waypointeditorairportdiagramenable->get_active()) {
		m_waypointeditormapenable->set_active(false);
		m_waypointeditormap->set_renderer(VectorMapArea::renderer_airportdiagram);
		m_waypointeditormap->set_map_scale(m_engine.get_prefs().mapscaleairportdiagram);
		m_waypointeditormap->show();
		m_waypointeditormapzoomin->show();
		m_waypointeditormapzoomout->show();
	} else {
		m_waypointeditormap->hide();
		m_waypointeditormapzoomin->hide();
		m_waypointeditormapzoomout->hide();
	}
}

void WaypointEditor::menu_mapzoomin_activate(void)
{
	m_waypointeditormap->zoom_in();
	if (m_waypointeditormap->get_renderer() == VectorMapArea::renderer_airportdiagram)
		m_engine.get_prefs().mapscaleairportdiagram = m_waypointeditormap->get_map_scale();
	else
		m_engine.get_prefs().mapscale = m_waypointeditormap->get_map_scale();
}

void WaypointEditor::menu_mapzoomout_activate(void)
{
	m_waypointeditormap->zoom_out();
	if (m_waypointeditormap->get_renderer() == VectorMapArea::renderer_airportdiagram)
		m_engine.get_prefs().mapscaleairportdiagram = m_waypointeditormap->get_map_scale();
	else
		m_engine.get_prefs().mapscale = m_waypointeditormap->get_map_scale();
}

void WaypointEditor::menu_prefs_activate(void )
{
	if (m_prefswindow)
		m_prefswindow->show();
}

void WaypointEditor::menu_about_activate(void)
{
	m_aboutdialog->show();
}

void WaypointEditor::set_row(Gtk::TreeModel::Row& row, const WaypointsDb::Waypoint& e)
{
	row[m_waypointlistcolumns.m_col_id] = e.get_address();
	row[m_waypointlistcolumns.m_col_coord] = e.get_coord();
	row[m_waypointlistcolumns.m_col_sourceid] = e.get_sourceid();
	row[m_waypointlistcolumns.m_col_icao] = e.get_icao();
	row[m_waypointlistcolumns.m_col_name] = e.get_name();
	row[m_waypointlistcolumns.m_col_usage] = e.get_usage();
	row[m_waypointlistcolumns.m_col_type] = e.get_type();
	row[m_waypointlistcolumns.m_col_label_placement] = e.get_label_placement();
	row[m_waypointlistcolumns.m_col_modtime] = e.get_modtime();
}

void WaypointEditor::edited_coord(const Glib::ustring& path_string, const Glib::ustring& new_text, Point new_coord)
{
	Gtk::TreePath path(path_string);
	Gtk::TreeModel::iterator iter = m_waypointliststore->get_iter(path);
	if (!iter)
		return;
	Gtk::TreeRow row = *iter;
	row[m_waypointlistcolumns.m_col_coord] = new_coord;
	WaypointsDb::Waypoint e((*m_db)(row[m_waypointlistcolumns.m_col_id]));
	if (!e.is_valid())
		return;
	e.set_modtime(time(0));
	e.set_coord(new_coord);
	save_noerr(e, row);
	m_waypointeditormap->set_center(new_coord, 0);
	m_waypointeditormap->set_cursor(new_coord);
}

void WaypointEditor::edited_sourceid(const Glib::ustring& path_string, const Glib::ustring& new_text)
{
	Gtk::TreePath path(path_string);
	Gtk::TreeModel::iterator iter = m_waypointliststore->get_iter(path);
	if (!iter)
		return;
	Gtk::TreeRow row = *iter;
	row[m_waypointlistcolumns.m_col_sourceid] = new_text;
	WaypointsDb::Waypoint e((*m_db)(row[m_waypointlistcolumns.m_col_id]));
	if (!e.is_valid())
		return;
	e.set_modtime(time(0));
	e.set_sourceid(new_text);
	save_noerr(e, row);
}

void WaypointEditor::edited_icao(const Glib::ustring& path_string, const Glib::ustring& new_text)
{
	Gtk::TreePath path(path_string);
	Gtk::TreeModel::iterator iter = m_waypointliststore->get_iter(path);
	if (!iter)
		return;
	Gtk::TreeRow row = *iter;
	row[m_waypointlistcolumns.m_col_icao] = new_text;
	WaypointsDb::Waypoint e((*m_db)(row[m_waypointlistcolumns.m_col_id]));
	if (!e.is_valid())
		return;
	e.set_modtime(time(0));
	e.set_icao(new_text);
	save_noerr(e, row);
}

void WaypointEditor::edited_name(const Glib::ustring& path_string, const Glib::ustring& new_text)
{
	Gtk::TreePath path(path_string);
	Gtk::TreeModel::iterator iter = m_waypointliststore->get_iter(path);
	if (!iter)
		return;
	Gtk::TreeRow row = *iter;
	row[m_waypointlistcolumns.m_col_name] = new_text;
	WaypointsDb::Waypoint e((*m_db)(row[m_waypointlistcolumns.m_col_id]));
	if (!e.is_valid())
		return;
	e.set_modtime(time(0));
	e.set_name(new_text);
	save_noerr(e, row);
}

void WaypointEditor::edited_usage(const Glib::ustring& path_string, const Glib::ustring& new_text, char new_usage)
{
	Gtk::TreePath path(path_string);
	Gtk::TreeModel::iterator iter = m_waypointliststore->get_iter(path);
	if (!iter)
		return;
	Gtk::TreeRow row = *iter;
	if (new_usage == WaypointsDb::Waypoint::usage_invalid)
		return;
	row[m_waypointlistcolumns.m_col_usage] = new_usage;
	WaypointsDb::Waypoint e((*m_db)(row[m_waypointlistcolumns.m_col_id]));
	if (!e.is_valid())
		return;
	e.set_modtime(time(0));
	e.set_usage(new_usage);
	save_noerr(e, row);
}

void WaypointEditor::edited_type(const Glib::ustring& path_string, const Glib::ustring& new_text, char new_type)
{
	Gtk::TreePath path(path_string);
	Gtk::TreeModel::iterator iter = m_waypointliststore->get_iter(path);
	if (!iter)
		return;
	Gtk::TreeRow row = *iter;
	if (new_type == WaypointsDb::Waypoint::type_invalid)
		return;
	row[m_waypointlistcolumns.m_col_type] = new_type;
	WaypointsDb::Waypoint e((*m_db)(row[m_waypointlistcolumns.m_col_id]));
	if (!e.is_valid())
		return;
	e.set_modtime(time(0));
	e.set_type(new_type);
	save_noerr(e, row);
}

void WaypointEditor::edited_label_placement(const Glib::ustring& path_string, const Glib::ustring& new_text, NavaidsDb::Navaid::label_placement_t new_placement)
{
	Gtk::TreePath path(path_string);
	Gtk::TreeModel::iterator iter = m_waypointliststore->get_iter(path);
	if (!iter)
		return;
	Gtk::TreeRow row = *iter;
	row[m_waypointlistcolumns.m_col_label_placement] = new_placement;
	WaypointsDb::Waypoint e((*m_db)(row[m_waypointlistcolumns.m_col_id]));
	if (!e.is_valid())
		return;
	e.set_modtime(time(0));
	e.set_label_placement(new_placement);
	save_noerr(e, row);
}

void WaypointEditor::edited_modtime(const Glib::ustring& path_string, const Glib::ustring& new_text, time_t new_time)
{
	Gtk::TreePath path(path_string);
	Gtk::TreeModel::iterator iter = m_waypointliststore->get_iter(path);
	if (!iter)
		return;
	Gtk::TreeRow row = *iter;
	row[m_waypointlistcolumns.m_col_modtime] = new_time;
	WaypointsDb::Waypoint e((*m_db)(row[m_waypointlistcolumns.m_col_id]));
	if (!e.is_valid())
		return;
	e.set_modtime(time(0));
	e.set_modtime(new_time);
	save_noerr(e, row);
}

void WaypointEditor::map_cursor(Point cursor, VectorMapArea::cursor_validity_t valid)
{
	if (valid != VectorMapArea::cursor_mouse)
		return;
	Glib::RefPtr<Gtk::TreeSelection> waypointeditor_selection = m_waypointeditortreeview->get_selection();
	Gtk::TreeModel::iterator iter = waypointeditor_selection->get_selected();
	if (!iter)
		return;
	Gtk::TreeRow row = *iter;
	row[m_waypointlistcolumns.m_col_coord] = cursor;
	WaypointsDb::Waypoint e((*m_db)(row[m_waypointlistcolumns.m_col_id]));
	if (!e.is_valid())
		return;
	e.set_modtime(time(0));
	e.set_coord(cursor);
	save_noerr(e, row);
}

void WaypointEditor::map_drawflags(int flags)
{
	*m_waypointeditormap = (MapRenderer::DrawFlags)flags;
}

void WaypointEditor::save_nostack(WaypointsDb::Waypoint& e)
{
	if (!e.is_valid())
		return;
	WaypointsDb::Waypoint::Address addr(e.get_address());
	m_db->save(e);
	m_dbchanged = true;
	m_waypointeditormap->waypoints_changed();
	Gtk::TreeIter iter = m_waypointliststore->children().begin();
	while (iter) {
		Gtk::TreeRow row = *iter;
		if (addr == row[m_waypointlistcolumns.m_col_id]) {
			set_row(row, e);
			return;
		}
		iter++;
	}
	Gtk::TreeModel::Row row(*(m_waypointliststore->append()));
	set_row(row, e);
}

void WaypointEditor::save(WaypointsDb::Waypoint& e)
{
	save_nostack(e);
	m_undoredo.save(e);
	update_undoredo_menu();
}

void WaypointEditor::save_noerr(WaypointsDb::Waypoint& e, Gtk::TreeModel::Row & row)
{
	if (e.get_address() != row[m_waypointlistcolumns.m_col_id])
		throw std::runtime_error("AirportEditor::save: ID mismatch");
	try {
		save(e);
	} catch (const sqlite3x::database_error& dberr) {
		WaypointsDb::Waypoint eold((*m_db)(row[m_waypointlistcolumns.m_col_id]));
		set_row(row, eold);
	}
}

void WaypointEditor::select(const WaypointsDb::Waypoint& e)
{
	Glib::RefPtr<Gtk::TreeSelection> waypointeditor_selection = m_waypointeditortreeview->get_selection();
	Gtk::TreeIter iter = m_waypointliststore->children().begin();
	while (iter) {
		Gtk::TreeRow row = *iter;
		if (e.get_address() == row[m_waypointlistcolumns.m_col_id]) {
			waypointeditor_selection->select(iter);
			m_waypointeditortreeview->scroll_to_row(Gtk::TreeModel::Path(iter));
			return;
		}
		iter++;
	}
}

void WaypointEditor::update_undoredo_menu(void)
{
	m_waypointeditorundo->set_sensitive(m_undoredo.can_undo());
	m_waypointeditorredo->set_sensitive(m_undoredo.can_redo());
}

void WaypointEditor::waypointeditor_selection_changed(void)
{
	Glib::RefPtr<Gtk::TreeSelection> waypointeditor_selection = m_waypointeditortreeview->get_selection();
	Gtk::TreeModel::iterator iter = waypointeditor_selection->get_selected();
	if (iter) {
		Gtk::TreeModel::Row row = *iter;
		//Do something with the row.
		std::cerr << "selected row: " << row[m_waypointlistcolumns.m_col_id] << std::endl;
		m_waypointeditordeletewaypoint->set_sensitive(true);
		Point pt(row[m_waypointlistcolumns.m_col_coord]);
		m_waypointeditormap->set_center(pt, 0);
		m_waypointeditormap->set_cursor(pt);
		return;
	}
	std::cerr << "selection cleared" << std::endl;
	m_waypointeditordeletewaypoint->set_sensitive(false);
}

void WaypointEditor::aboutdialog_response(int response)
{
	if (response == Gtk::RESPONSE_CLOSE || response == Gtk::RESPONSE_CANCEL)
		m_aboutdialog->hide();
}

int main(int argc, char *argv[])
{
	try {
		std::string dir_main(""), dir_aux("");
		Engine::auxdb_mode_t auxdbmode = Engine::auxdb_prefs;
		bool dis_aux(false);
		Glib::init();
		Glib::OptionGroup optgroup("waypointeditor", "Waypoint Editor Options", "Options controlling the Waypoint Editor");
		Glib::OptionEntry optmaindir, optauxdir, optdisableaux;
		optmaindir.set_short_name('m');
		optmaindir.set_long_name("maindir");
		optmaindir.set_description("Directory containing the main database");
		optmaindir.set_arg_description("DIR");
		optauxdir.set_short_name('a');
		optauxdir.set_long_name("auxdir");
		optauxdir.set_description("Directory containing the auxilliary database");
		optauxdir.set_arg_description("DIR");
		optdisableaux.set_short_name('A');
		optdisableaux.set_long_name("disableaux");
		optdisableaux.set_description("Disable the auxilliary database");
		optdisableaux.set_arg_description("BOOL");
		optgroup.add_entry_filename(optmaindir, dir_main);
		optgroup.add_entry_filename(optauxdir, dir_aux);
		optgroup.add_entry(optdisableaux, dis_aux);
#ifdef HAVE_PQXX
		Glib::ustring pg_conn;
		Glib::OptionEntry optpgsql;
		optpgsql.set_short_name('p');
		optpgsql.set_long_name("pgsql");
		optpgsql.set_description("PostgreSQL Connect String");
		optpgsql.set_arg_description("CONNSTR");
		optgroup.add_entry(optpgsql, pg_conn);
#endif
		Glib::OptionContext optctx("[--maindir=<dir>] [--auxdir=<dir>] [--disableaux]");
		optctx.set_help_enabled(true);
		optctx.set_ignore_unknown_options(false);
		optctx.set_main_group(optgroup);
		Gtk::Main m(argc, argv, optctx);
#ifdef HAVE_PQXX
		if (!pg_conn.empty()) {
			dir_main = pg_conn;
			dir_aux.clear();
			pg_conn.clear();
			auxdbmode = Engine::auxdb_postgres;
		} else
#endif
		if (dis_aux)
			auxdbmode = Engine::auxdb_none;
		else if (!dir_aux.empty())
			auxdbmode = Engine::auxdb_override;
#ifdef HAVE_OSSO
		osso_context_t* osso_context = osso_initialize("vfrwaypointeditor", VERSION, TRUE /* deprecated parameter */, 0 /* Use default Glib main loop context */);
		if (!osso_context) {
			std::cerr << "osso_initialize() failed." << std::endl;
			return OSSO_ERROR;
		}
#endif
#ifdef HAVE_HILDON
		Hildon::init();
#endif
		Glib::set_application_name("VFR Waypoint Editor");
		Glib::thread_init();
		WaypointEditor editor(dir_main, auxdbmode, dir_aux);

		Gtk::Main::run(*editor.get_mainwindow());
#ifdef HAVE_OSSO
		osso_deinitialize(osso_context);
#endif
	} catch (const Glib::Exception& e) {
		std::cerr << "glib exception: " << e.what() << std::endl;
		return 1;
	} catch (const std::exception& e) {
		std::cerr << "exception: " << e.what() << std::endl;
		return 1;
	}
	return 0;
}
