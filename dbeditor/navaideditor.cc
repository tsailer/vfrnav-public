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

NavaidEditor::NavaidModelColumns::NavaidModelColumns(void)
{
	add(m_col_id);
	add(m_col_coord);
	add(m_col_elev);
	add(m_col_range);
	add(m_col_freq);
	add(m_col_sourceid);
	add(m_col_icao);
	add(m_col_name);
	add(m_col_navaid_type);
	add(m_col_label_placement);
	add(m_col_inservice);
	add(m_col_modtime);
}

NavaidEditor::NavaidEditor(const std::string& dir_main, Engine::auxdb_mode_t auxdbmode, const std::string& dir_aux)
	: m_dbchanged(false), m_engine(dir_main, auxdbmode, dir_aux, false, false), m_navaideditorwindow(0), m_navaideditortreeview(0), m_navaideditorstatusbar(0), 
	  m_navaideditorfind(0), m_navaideditormap(0), m_navaideditorundo(0), 
	  m_navaideditorredo(0), m_navaideditordeletenavaid(0), m_navaideditormapenable(0), m_navaideditorairportdiagramenable(0),
	  m_navaideditormapzoomin(0), m_navaideditormapzoomout(0), m_aboutdialog(0)
{
#ifdef HAVE_PQXX
	if (auxdbmode == Engine::auxdb_postgres)
		m_db.reset(new NavaidsPGDb(dir_main));
	else
#endif
	{
		m_db.reset(new NavaidsDb(dir_main));
		std::cerr << "Main database " << (dir_main.empty() ? std::string("(default)") : dir_main) << std::endl;
		Glib::ustring dir_aux1(m_engine.get_aux_dir(auxdbmode, dir_aux));
		NavaidsDb *db(static_cast<NavaidsDb *>(m_db.get()));
		if (!dir_aux1.empty() && db->is_dbfile_exists(dir_aux1))
			db->attach(dir_aux1);
		if (db->is_aux_attached())
			std::cerr << "Auxillary navaids database " << dir_aux1 << " attached" << std::endl;
	}
	m_refxml = load_glade_file("dbeditor" UIEXT, "navaideditorwindow");
	get_widget_derived(m_refxml, "navaideditorwindow", m_navaideditorwindow);
	get_widget(m_refxml, "navaideditortreeview", m_navaideditortreeview);
	get_widget(m_refxml, "navaideditorstatusbar", m_navaideditorstatusbar);
	get_widget(m_refxml, "navaideditorfind", m_navaideditorfind);
	get_widget_derived(m_refxml, "navaideditormap", m_navaideditormap);
	{
#ifdef HAVE_GTKMM2
		Glib::RefPtr<Builder> refxml = load_glade_file_nosearch(m_refxml->get_filename(), "aboutdialog");
#else
		//Glib::RefPtr<Builder> refxml = load_glade_file("dbeditor" UIEXT, "aboutdialog");
		Glib::RefPtr<Builder> refxml = m_refxml;
#endif
		get_widget(refxml, "aboutdialog", m_aboutdialog);
		m_aboutdialog->signal_response().connect(sigc::mem_fun(*this, &NavaidEditor::aboutdialog_response));
	}
	Gtk::Button *buttonclear(0);
	get_widget(m_refxml, "navaideditorclearbutton", buttonclear);
	buttonclear->signal_clicked().connect(sigc::mem_fun(*this, &NavaidEditor::buttonclear_clicked));
	m_navaideditorfind->signal_changed().connect(sigc::mem_fun(*this, &NavaidEditor::find_changed));
	Gtk::MenuItem *menu_quit(0), *menu_newnavaid(0), *menu_cut(0), *menu_copy(0), *menu_paste(0), *menu_delete(0), *menu_preferences(0), *menu_about(0);
	get_widget(m_refxml, "navaideditorquit", menu_quit);
	get_widget(m_refxml, "navaideditorundo", m_navaideditorundo);
	get_widget(m_refxml, "navaideditorredo", m_navaideditorredo);
	get_widget(m_refxml, "navaideditornewnavaid", menu_newnavaid);
	get_widget(m_refxml, "navaideditordeletenavaid", m_navaideditordeletenavaid);
	get_widget(m_refxml, "navaideditormapenable", m_navaideditormapenable);
	get_widget(m_refxml, "navaideditorairportdiagramenable", m_navaideditorairportdiagramenable);
	get_widget(m_refxml, "navaideditormapzoomin", m_navaideditormapzoomin);
	get_widget(m_refxml, "navaideditormapzoomout", m_navaideditormapzoomout);
	get_widget(m_refxml, "navaideditorcut", menu_cut);
	get_widget(m_refxml, "navaideditorcopy", menu_copy);
	get_widget(m_refxml, "navaideditorpaste", menu_paste);
	get_widget(m_refxml, "navaideditordelete", menu_delete);
	get_widget(m_refxml, "navaideditorpreferences", menu_preferences);
	get_widget(m_refxml, "navaideditorabout", menu_about);
	menu_quit->signal_activate().connect(sigc::mem_fun(*this, &NavaidEditor::menu_quit_activate));
	m_navaideditorundo->signal_activate().connect(sigc::mem_fun(*this, &NavaidEditor::menu_undo_activate));
	m_navaideditorredo->signal_activate().connect(sigc::mem_fun(*this, &NavaidEditor::menu_redo_activate));
	menu_newnavaid->signal_activate().connect(sigc::mem_fun(*this, &NavaidEditor::menu_newnavaid_activate));
	m_navaideditordeletenavaid->signal_activate().connect(sigc::mem_fun(*this, &NavaidEditor::menu_deletenavaid_activate));
	m_navaideditormapenable->signal_toggled().connect(sigc::mem_fun(*this, &NavaidEditor::menu_mapenable_toggled));
	m_navaideditorairportdiagramenable->signal_toggled().connect(sigc::mem_fun(*this, &NavaidEditor::menu_airportdiagramenable_toggled));
	m_navaideditormapzoomin->signal_activate().connect(sigc::mem_fun(*this, &NavaidEditor::menu_mapzoomin_activate));
	m_navaideditormapzoomout->signal_activate().connect(sigc::mem_fun(*this, &NavaidEditor::menu_mapzoomout_activate));
	if (m_navaideditormapenable->get_active()) {
		m_navaideditormapzoomin->show();
		m_navaideditormapzoomout->show();
	} else {
		m_navaideditormapzoomin->hide();
		m_navaideditormapzoomout->hide();
	}
	m_navaideditorwindow->signal_zoomin().connect(sigc::bind_return(sigc::mem_fun(*this, &NavaidEditor::menu_mapzoomin_activate), true));
	m_navaideditorwindow->signal_zoomout().connect(sigc::bind_return(sigc::mem_fun(*this, &NavaidEditor::menu_mapzoomout_activate), true));
	menu_cut->signal_activate().connect(sigc::mem_fun(*this, &NavaidEditor::menu_cut_activate));
	menu_copy->signal_activate().connect(sigc::mem_fun(*this, &NavaidEditor::menu_copy_activate));
	menu_paste->signal_activate().connect(sigc::mem_fun(*this, &NavaidEditor::menu_paste_activate));
	menu_delete->signal_activate().connect(sigc::mem_fun(*this, &NavaidEditor::menu_delete_activate));
	menu_preferences->signal_activate().connect(sigc::mem_fun(*this, &NavaidEditor::menu_prefs_activate));
	menu_about->signal_activate().connect(sigc::mem_fun(*this, &NavaidEditor::menu_about_activate));
	update_undoredo_menu();
	// Vector Map
	m_navaideditormap->set_engine(m_engine);
	m_navaideditormap->set_renderer(VectorMapArea::renderer_vmap);
	m_navaideditormap->signal_cursor().connect(sigc::mem_fun(*this, &NavaidEditor::map_cursor));
	m_navaideditormap->set_map_scale(m_engine.get_prefs().mapscale);
	NavaidEditor::map_drawflags(m_engine.get_prefs().mapflags);
	m_engine.get_prefs().mapflags.signal_change().connect(sigc::mem_fun(*this, &NavaidEditor::map_drawflags));
	// Navaid List
	m_navaidliststore = Gtk::ListStore::create(m_navaidlistcolumns);
	m_navaideditortreeview->set_model(m_navaidliststore);
	Gtk::CellRendererText *icao_renderer = Gtk::manage(new Gtk::CellRendererText());
	Gtk::TreeViewColumn *icao_column = m_navaideditortreeview->get_column(m_navaideditortreeview->append_column(_("ICAO"), *icao_renderer) - 1);
	if (icao_column) {
		icao_column->add_attribute(*icao_renderer, "text", m_navaidlistcolumns.m_col_icao);
		icao_column->set_sort_column(m_navaidlistcolumns.m_col_icao);
	}
	icao_renderer->signal_edited().connect(sigc::mem_fun(*this, &NavaidEditor::edited_icao));
	icao_renderer->set_property("editable", true);
	Gtk::CellRendererText *name_renderer = Gtk::manage(new Gtk::CellRendererText());
	Gtk::TreeViewColumn *name_column = m_navaideditortreeview->get_column(m_navaideditortreeview->append_column(_("Name"), *name_renderer) - 1);
	if (name_column) {
		name_column->add_attribute(*name_renderer, "text", m_navaidlistcolumns.m_col_name);
		name_column->set_sort_column(m_navaidlistcolumns.m_col_name);
	}
	name_renderer->signal_edited().connect(sigc::mem_fun(*this, &NavaidEditor::edited_name));
	name_renderer->set_property("editable", true);
	CoordCellRenderer *coord_renderer = Gtk::manage(new CoordCellRenderer());
	Gtk::TreeViewColumn *coord_column = m_navaideditortreeview->get_column(m_navaideditortreeview->append_column(_("Coordinate"), *coord_renderer) - 1);
	if (coord_column) {
		coord_column->add_attribute(*coord_renderer, coord_renderer->get_property_name(), m_navaidlistcolumns.m_col_coord);
		coord_column->set_sort_column(m_navaidlistcolumns.m_col_coord);
	}
	coord_renderer->set_property("editable", true);
	coord_renderer->signal_edited().connect(sigc::mem_fun(*this, &NavaidEditor::edited_coord));
  	Gtk::CellRendererText *elev_renderer = Gtk::manage(new Gtk::CellRendererText());
	Gtk::TreeViewColumn *elev_column = m_navaideditortreeview->get_column(m_navaideditortreeview->append_column(_("Elev"), *elev_renderer) - 1);
	if (elev_column) {
 		elev_column->add_attribute(*elev_renderer, "text", m_navaidlistcolumns.m_col_elev);
		elev_column->set_sort_column(m_navaidlistcolumns.m_col_elev);
	}
	elev_renderer->set_property("xalign", 1.0);
	elev_renderer->signal_edited().connect(sigc::mem_fun(*this, &NavaidEditor::edited_elev));
	elev_renderer->set_property("editable", true);
	Gtk::CellRendererText *range_renderer = Gtk::manage(new Gtk::CellRendererText());
	Gtk::TreeViewColumn *range_column = m_navaideditortreeview->get_column(m_navaideditortreeview->append_column(_("Range"), *range_renderer) - 1);
	if (range_column) {
		range_column->add_attribute(*range_renderer, "text", m_navaidlistcolumns.m_col_range);
		range_column->set_sort_column(m_navaidlistcolumns.m_col_range);
	}
	range_renderer->set_property("xalign", 1.0);
	range_renderer->signal_edited().connect(sigc::mem_fun(*this, &NavaidEditor::edited_range));
	range_renderer->set_property("editable", true);
	FrequencyCellRenderer *freq_renderer = Gtk::manage(new FrequencyCellRenderer());
	Gtk::TreeViewColumn *freq_column = m_navaideditortreeview->get_column(m_navaideditortreeview->append_column(_("Freq"), *freq_renderer) - 1);
	if (freq_column) {
		freq_column->add_attribute(*freq_renderer, freq_renderer->get_property_name(), m_navaidlistcolumns.m_col_freq);
		freq_column->set_sort_column(m_navaidlistcolumns.m_col_freq);
	}
	freq_renderer->set_property("editable", true);
	freq_renderer->signal_edited().connect(sigc::mem_fun(*this, &NavaidEditor::edited_freq));
	freq_renderer->set_property("xalign", 1.0);
	NavaidTypeRenderer *navaid_type_renderer = Gtk::manage(new NavaidTypeRenderer());
	Gtk::TreeViewColumn *navaid_type_column = m_navaideditortreeview->get_column(m_navaideditortreeview->append_column(_("Type"), *navaid_type_renderer) - 1);
	if (navaid_type_column) {
		navaid_type_column->add_attribute(*navaid_type_renderer, navaid_type_renderer->get_property_name(), m_navaidlistcolumns.m_col_navaid_type);
		navaid_type_column->set_sort_column(m_navaidlistcolumns.m_col_navaid_type);
	}
	navaid_type_renderer->set_property("editable", true);
	navaid_type_renderer->signal_edited().connect(sigc::mem_fun(*this, &NavaidEditor::edited_navaid_type));
	//m_navaideditortreeview->append_column_editable(_("Type"), m_navaidlistcolumns.m_col_navaid_type);
	LabelPlacementRenderer *label_placement_renderer = Gtk::manage(new LabelPlacementRenderer());
	Gtk::TreeViewColumn *label_placement_column = m_navaideditortreeview->get_column(m_navaideditortreeview->append_column(_("Label"), *label_placement_renderer) - 1);
	if (label_placement_column) {
		label_placement_column->add_attribute(*label_placement_renderer, label_placement_renderer->get_property_name(), m_navaidlistcolumns.m_col_label_placement);
		label_placement_column->set_sort_column(m_navaidlistcolumns.m_col_label_placement);
	}
	label_placement_renderer->set_property("editable", true);
	label_placement_renderer->signal_edited().connect(sigc::mem_fun(*this, &NavaidEditor::edited_label_placement));
	//m_navaideditortreeview->append_column_editable(_("Label"), m_navaidlistcolumns.m_col_label_placement);
	Gtk::CellRendererToggle *inserv_renderer = Gtk::manage(new Gtk::CellRendererToggle());
	Gtk::TreeViewColumn *inserv_column = m_navaideditortreeview->get_column(m_navaideditortreeview->append_column(_("InServ"), *inserv_renderer) - 1);
	if (inserv_column) {
 		inserv_column->add_attribute(*inserv_renderer, "active", m_navaidlistcolumns.m_col_inservice);
		inserv_column->set_sort_column(m_navaidlistcolumns.m_col_inservice);
	}
	inserv_renderer->signal_toggled().connect(sigc::mem_fun(*this, &NavaidEditor::edited_inservice));
	inserv_renderer->set_property("editable", true);
	Gtk::CellRendererText *srcid_renderer = Gtk::manage(new Gtk::CellRendererText());
	Gtk::TreeViewColumn *srcid_column = m_navaideditortreeview->get_column(m_navaideditortreeview->append_column(_("Source ID"), *srcid_renderer) - 1);
	if (srcid_column) {
  		srcid_column->add_attribute(*srcid_renderer, "text", m_navaidlistcolumns.m_col_sourceid);
		srcid_column->set_sort_column(m_navaidlistcolumns.m_col_sourceid);
	}
	srcid_renderer->signal_edited().connect(sigc::mem_fun(*this, &NavaidEditor::edited_sourceid));
	srcid_renderer->set_property("editable", true);
	DateTimeCellRenderer *modtime_renderer = Gtk::manage(new DateTimeCellRenderer());
	Gtk::TreeViewColumn *modtime_column = m_navaideditortreeview->get_column(m_navaideditortreeview->append_column(_("Mod Time"), *modtime_renderer) - 1);
	if (modtime_column) {
		modtime_column->add_attribute(*modtime_renderer, modtime_renderer->get_property_name(), m_navaidlistcolumns.m_col_modtime);
		modtime_column->set_sort_column(m_navaidlistcolumns.m_col_modtime);
	}
	modtime_renderer->set_property("editable", true);
	modtime_renderer->signal_edited().connect(sigc::mem_fun(*this, &NavaidEditor::edited_modtime));
	//m_navaideditortreeview->append_column_editable(_("Mod Time"), m_navaidlistcolumns.m_col_modtime);
	for (unsigned int i = 0; i < 12; ++i) {
		Gtk::TreeViewColumn *col = m_navaideditortreeview->get_column(i);
		if (!col)
			continue;
		col->set_resizable(true);
		col->set_reorderable(true);
	}
	m_navaideditortreeview->columns_autosize();
	m_navaideditortreeview->set_enable_search(true);
	m_navaideditortreeview->set_search_column(m_navaidlistcolumns.m_col_name);
	// selection
	Glib::RefPtr<Gtk::TreeSelection> navaideditor_selection = m_navaideditortreeview->get_selection();
	navaideditor_selection->set_mode(Gtk::SELECTION_SINGLE);
	navaideditor_selection->signal_changed().connect(sigc::mem_fun(*this, &NavaidEditor::navaideditor_selection_changed));
	//navaideditor_selection->set_select_function(sigc::mem_fun(*this, &NavaidEditor::navaideditor_select_function));
	navaideditor_selection_changed();

	//set_navaidlist("");
	m_dbselectdialog.signal_byname().connect(sigc::mem_fun(*this, &NavaidEditor::dbselect_byname));
	m_dbselectdialog.signal_byrectangle().connect(sigc::mem_fun(*this, &NavaidEditor::dbselect_byrect));
	m_dbselectdialog.signal_bytime().connect(sigc::mem_fun(*this, &NavaidEditor::dbselect_bytime));
	m_dbselectdialog.show();
	m_prefswindow = PrefsWindow::create(m_engine.get_prefs());
	m_navaideditorwindow->show();
}

NavaidEditor::~NavaidEditor()
{
	if (m_dbchanged) {
		std::cerr << "Analyzing database..." << std::endl;
		m_db->analyze();
		m_db->vacuum();
	}
	m_db->close();
}

void NavaidEditor::dbselect_byname( const Glib::ustring & s, NavaidsDb::comp_t comp )
{
	NavaidsDb::elementvector_t els(m_db->find_by_text(s, 0, comp, 0));
	m_navaidliststore->clear();
	for (NavaidsDb::elementvector_t::iterator ei(els.begin()), ee(els.end()); ei != ee; ++ei) {
		NavaidsDb::element_t& e(*ei);
		if (!e.is_valid())
			continue;
		Gtk::TreeModel::Row row(*(m_navaidliststore->append()));
		set_row(row, e);
	}
}

void NavaidEditor::dbselect_byrect( const Rect & r )
{
	NavaidsDb::elementvector_t els(m_db->find_by_rect(r, 0));
	m_navaidliststore->clear();
	for (NavaidsDb::elementvector_t::iterator ei(els.begin()), ee(els.end()); ei != ee; ++ei) {
		NavaidsDb::element_t& e(*ei);
		if (!e.is_valid())
			continue;
		Gtk::TreeModel::Row row(*(m_navaidliststore->append()));
		set_row(row, e);
	}
}

void NavaidEditor::dbselect_bytime(time_t timefrom, time_t timeto)
{
	NavaidsDb::elementvector_t els(m_db->find_by_time(timefrom, timeto, 0));
	m_navaidliststore->clear();
	for (NavaidsDb::elementvector_t::iterator ei(els.begin()), ee(els.end()); ei != ee; ++ei) {
		NavaidsDb::element_t& e(*ei);
		if (!e.is_valid())
			continue;
		Gtk::TreeModel::Row row(*(m_navaidliststore->append()));
		set_row(row, e);
	}
}

void NavaidEditor::buttonclear_clicked(void)
{
	m_navaideditorfind->set_text("");
}

void NavaidEditor::find_changed(void)
{
	dbselect_byname(m_navaideditorfind->get_text());
}

void NavaidEditor::menu_quit_activate(void)
{
	m_navaideditorwindow->hide();
}

void NavaidEditor::menu_undo_activate(void)
{
	NavaidsDb::Navaid e(m_undoredo.undo());
	if (e.is_valid()) {
		save_nostack(e);
		select(e);
	}
	update_undoredo_menu();
}

void NavaidEditor::menu_redo_activate(void)
{
	NavaidsDb::Navaid e(m_undoredo.redo());
	if (e.is_valid()) {
		save_nostack(e);
		select(e);
	}
	update_undoredo_menu();
}

void NavaidEditor::menu_newnavaid_activate(void)
{
	NavaidsDb::Navaid e;
	e.set_navaid_type(NavaidsDb::Navaid::navaid_vor);
	e.set_sourceid(make_sourceid());
	e.set_modtime(time(0));
	save(e);
	select(e);
}

void NavaidEditor::menu_deletenavaid_activate(void)
{
	Glib::RefPtr<Gtk::TreeSelection> navaideditor_selection = m_navaideditortreeview->get_selection();
	Gtk::TreeModel::iterator iter = navaideditor_selection->get_selected();
	if (!iter)
		return;
	Gtk::TreeModel::Row row = *iter;
	std::cerr << "deleting row: " << row[m_navaidlistcolumns.m_col_id] << std::endl;
	NavaidsDb::Navaid e((*m_db)(row[m_navaidlistcolumns.m_col_id]));
	if (!e.is_valid())
		return;
	m_undoredo.erase(e);
	m_db->erase(e);
	m_navaidliststore->erase(iter);
}

void NavaidEditor::menu_cut_activate(void)
{
	Gtk::Widget *focus = m_navaideditorwindow->get_focus();
	if (!focus)
		return;
	Gtk::Editable *edit = dynamic_cast<Gtk::Editable *>(focus);
	if (!edit)
		return;
	edit->cut_clipboard();
}

void NavaidEditor::menu_copy_activate(void)
{
	Gtk::Widget *focus = m_navaideditorwindow->get_focus();
	if (!focus)
		return;
	Gtk::Editable *edit = dynamic_cast<Gtk::Editable *>(focus);
	if (!edit)
		return;
	edit->copy_clipboard();
}

void NavaidEditor::menu_paste_activate(void)
{
	Gtk::Widget *focus = m_navaideditorwindow->get_focus();
	if (!focus)
		return;
	Gtk::Editable *edit = dynamic_cast<Gtk::Editable *>(focus);
	if (!edit)
		return;
	edit->paste_clipboard();
}

void NavaidEditor::menu_delete_activate(void)
{
	Gtk::Widget *focus = m_navaideditorwindow->get_focus();
	if (!focus)
		return;
	Gtk::Editable *edit = dynamic_cast<Gtk::Editable *>(focus);
	if (!edit)
		return;
	edit->delete_selection();
}

void NavaidEditor::menu_mapenable_toggled(void)
{
	if (m_navaideditormapenable->get_active()) {
		m_navaideditorairportdiagramenable->set_active(false);
		m_navaideditormap->set_renderer(VectorMapArea::renderer_vmap);
		m_navaideditormap->set_map_scale(m_engine.get_prefs().mapscale);
		m_navaideditormap->show();
		m_navaideditormapzoomin->show();
		m_navaideditormapzoomout->show();
	} else {
		m_navaideditormap->hide();
		m_navaideditormapzoomin->hide();
		m_navaideditormapzoomout->hide();
	}
}

void NavaidEditor::menu_airportdiagramenable_toggled(void)
{
	if (m_navaideditorairportdiagramenable->get_active()) {
		m_navaideditormapenable->set_active(false);
		m_navaideditormap->set_renderer(VectorMapArea::renderer_airportdiagram);
		m_navaideditormap->set_map_scale(m_engine.get_prefs().mapscaleairportdiagram);
		m_navaideditormap->show();
		m_navaideditormapzoomin->show();
		m_navaideditormapzoomout->show();
	} else {
		m_navaideditormap->hide();
		m_navaideditormapzoomin->hide();
		m_navaideditormapzoomout->hide();
	}
}

void NavaidEditor::menu_mapzoomin_activate(void)
{
	m_navaideditormap->zoom_in();
	if (m_navaideditormap->get_renderer() == VectorMapArea::renderer_airportdiagram)
		m_engine.get_prefs().mapscaleairportdiagram = m_navaideditormap->get_map_scale();
	else
		m_engine.get_prefs().mapscale = m_navaideditormap->get_map_scale();
}

void NavaidEditor::menu_mapzoomout_activate(void)
{
	m_navaideditormap->zoom_out();
	if (m_navaideditormap->get_renderer() == VectorMapArea::renderer_airportdiagram)
		m_engine.get_prefs().mapscaleairportdiagram = m_navaideditormap->get_map_scale();
	else
		m_engine.get_prefs().mapscale = m_navaideditormap->get_map_scale();
}

void NavaidEditor::menu_prefs_activate(void)
{
	if (m_prefswindow)
		m_prefswindow->show();
}

void NavaidEditor::menu_about_activate(void)
{
	m_aboutdialog->show();
}

void NavaidEditor::set_row( Gtk::TreeModel::Row & row, const NavaidsDb::Navaid & e )
{
	row[m_navaidlistcolumns.m_col_id] = e.get_address();
	row[m_navaidlistcolumns.m_col_coord] = e.get_coord();
	row[m_navaidlistcolumns.m_col_elev] = e.get_elev();
	row[m_navaidlistcolumns.m_col_range] = e.get_range();
	row[m_navaidlistcolumns.m_col_freq] = e.get_frequency();
	row[m_navaidlistcolumns.m_col_sourceid] = e.get_sourceid();
	row[m_navaidlistcolumns.m_col_icao] = e.get_icao();
	row[m_navaidlistcolumns.m_col_name] = e.get_name();
	row[m_navaidlistcolumns.m_col_navaid_type] = e.get_navaid_type();
	row[m_navaidlistcolumns.m_col_label_placement] = e.get_label_placement();
	row[m_navaidlistcolumns.m_col_inservice] = e.is_inservice();
	row[m_navaidlistcolumns.m_col_modtime] = e.get_modtime(); // RouteEditUi::time_str("%Y-%m-%d %H:%M:%S", e.get_modtime());
}

void NavaidEditor::edited_coord( const Glib::ustring & path_string, const Glib::ustring& new_text, Point new_coord )
{
	Gtk::TreePath path(path_string);
	Gtk::TreeModel::iterator iter = m_navaidliststore->get_iter(path);
	if (!iter)
		return;
	Gtk::TreeRow row = *iter;
	row[m_navaidlistcolumns.m_col_coord] = new_coord;
	NavaidsDb::Navaid e((*m_db)(row[m_navaidlistcolumns.m_col_id]));
	if (!e.is_valid())
		return;
	e.set_modtime(time(0));
	e.set_coord(new_coord);
	save_noerr(e, row);
	m_navaideditormap->set_center(new_coord, e.get_elev());
	m_navaideditormap->set_cursor(new_coord);
}

void NavaidEditor::edited_elev( const Glib::ustring & path_string, const Glib::ustring & new_text )
{
	Gtk::TreePath path(path_string);
	Gtk::TreeModel::iterator iter = m_navaidliststore->get_iter(path);
	if (!iter)
		return;
	Gtk::TreeRow row = *iter;
	int32_t elev = Conversions::signed_feet_parse(new_text);
	row[m_navaidlistcolumns.m_col_elev] = elev;
	NavaidsDb::Navaid e((*m_db)(row[m_navaidlistcolumns.m_col_id]));
	if (!e.is_valid())
		return;
	e.set_modtime(time(0));
	e.set_elev(elev);
	save_noerr(e, row);
}

void NavaidEditor::edited_range( const Glib::ustring & path_string, const Glib::ustring & new_text )
{
	Gtk::TreePath path(path_string);
	Gtk::TreeModel::iterator iter = m_navaidliststore->get_iter(path);
	if (!iter)
		return;
	Gtk::TreeRow row = *iter;
	int32_t range = strtol(new_text.c_str(), 0, 0);
	row[m_navaidlistcolumns.m_col_range] = range;
	NavaidsDb::Navaid e((*m_db)(row[m_navaidlistcolumns.m_col_id]));
	if (!e.is_valid())
		return;
	e.set_modtime(time(0));
	e.set_range(range);
	save_noerr(e, row);
}

void NavaidEditor::edited_freq( const Glib::ustring & path_string, const Glib::ustring& new_text, uint32_t new_freq )
{
	Gtk::TreePath path(path_string);
	Gtk::TreeModel::iterator iter = m_navaidliststore->get_iter(path);
	if (!iter)
		return;
	Gtk::TreeRow row = *iter;
	row[m_navaidlistcolumns.m_col_freq] = new_freq;
	NavaidsDb::Navaid e((*m_db)(row[m_navaidlistcolumns.m_col_id]));
	if (!e.is_valid())
		return;
	e.set_modtime(time(0));
	e.set_frequency(new_freq);
	save_noerr(e, row);
}

void NavaidEditor::edited_sourceid( const Glib::ustring & path_string, const Glib::ustring & new_text )
{
	Gtk::TreePath path(path_string);
	Gtk::TreeModel::iterator iter = m_navaidliststore->get_iter(path);
	if (!iter)
		return;
	Gtk::TreeRow row = *iter;
	row[m_navaidlistcolumns.m_col_sourceid] = new_text;
	NavaidsDb::Navaid e((*m_db)(row[m_navaidlistcolumns.m_col_id]));
	if (!e.is_valid())
		return;
	e.set_modtime(time(0));
	e.set_sourceid(new_text);
	save_noerr(e, row);
}

void NavaidEditor::edited_icao( const Glib::ustring & path_string, const Glib::ustring & new_text )
{
	Gtk::TreePath path(path_string);
	Gtk::TreeModel::iterator iter = m_navaidliststore->get_iter(path);
	if (!iter)
		return;
	Gtk::TreeRow row = *iter;
	row[m_navaidlistcolumns.m_col_icao] = new_text;
	NavaidsDb::Navaid e((*m_db)(row[m_navaidlistcolumns.m_col_id]));
	if (!e.is_valid())
		return;
	e.set_modtime(time(0));
	e.set_icao(new_text);
	save_noerr(e, row);
}

void NavaidEditor::edited_name( const Glib::ustring & path_string, const Glib::ustring & new_text )
{
	Gtk::TreePath path(path_string);
	Gtk::TreeModel::iterator iter = m_navaidliststore->get_iter(path);
	if (!iter)
		return;
	Gtk::TreeRow row = *iter;
	row[m_navaidlistcolumns.m_col_name] = new_text;
	NavaidsDb::Navaid e((*m_db)(row[m_navaidlistcolumns.m_col_id]));
	if (!e.is_valid())
		return;
	e.set_modtime(time(0));
	e.set_name(new_text);
	save_noerr(e, row);
}

void NavaidEditor::edited_navaid_type( const Glib::ustring & path_string, const Glib::ustring & new_text, NavaidsDb::Navaid::navaid_type_t new_type )
{
	Gtk::TreePath path(path_string);
	Gtk::TreeModel::iterator iter = m_navaidliststore->get_iter(path);
	if (!iter)
		return;
	Gtk::TreeRow row = *iter;
	if (new_type == NavaidsDb::Navaid::navaid_invalid)
		return;
	row[m_navaidlistcolumns.m_col_navaid_type] = new_type;
	NavaidsDb::Navaid e((*m_db)(row[m_navaidlistcolumns.m_col_id]));
	if (!e.is_valid())
		return;
	e.set_modtime(time(0));
	e.set_navaid_type(new_type);
	save_noerr(e, row);
}

void NavaidEditor::edited_label_placement( const Glib::ustring & path_string, const Glib::ustring & new_text, NavaidsDb::Navaid::label_placement_t new_placement )
{
	Gtk::TreePath path(path_string);
	Gtk::TreeModel::iterator iter = m_navaidliststore->get_iter(path);
	if (!iter)
		return;
	Gtk::TreeRow row = *iter;
	row[m_navaidlistcolumns.m_col_label_placement] = new_placement;
	NavaidsDb::Navaid e((*m_db)(row[m_navaidlistcolumns.m_col_id]));
	if (!e.is_valid())
		return;
	e.set_modtime(time(0));
	e.set_label_placement(new_placement);
	save_noerr(e, row);
}

void NavaidEditor::edited_inservice( const Glib::ustring & path_string )
{
	Gtk::TreePath path(path_string);
	Gtk::TreeModel::iterator iter = m_navaidliststore->get_iter(path);
	if (!iter)
		return;
	Gtk::TreeRow row = *iter;
	NavaidsDb::Navaid e((*m_db)(row[m_navaidlistcolumns.m_col_id]));
	if (!e.is_valid())
		return;
	e.set_modtime(time(0));
	e.set_inservice(!e.is_inservice());
	row[m_navaidlistcolumns.m_col_inservice] = e.is_inservice();
	save_noerr(e, row);
}

void NavaidEditor::edited_modtime( const Glib::ustring & path_string, const Glib::ustring& new_text, time_t new_time )
{
	Gtk::TreePath path(path_string);
	Gtk::TreeModel::iterator iter = m_navaidliststore->get_iter(path);
	if (!iter)
		return;
	Gtk::TreeRow row = *iter;
	row[m_navaidlistcolumns.m_col_modtime] = new_time;
	NavaidsDb::Navaid e((*m_db)(row[m_navaidlistcolumns.m_col_id]));
	if (!e.is_valid())
		return;
	e.set_modtime(new_time);
	save_noerr(e, row);
}

void NavaidEditor::map_cursor( Point cursor, VectorMapArea::cursor_validity_t valid )
{
	if (valid != VectorMapArea::cursor_mouse)
		return;
	Glib::RefPtr<Gtk::TreeSelection> navaideditor_selection = m_navaideditortreeview->get_selection();
	Gtk::TreeModel::iterator iter = navaideditor_selection->get_selected();
	if (!iter)
		return;
	Gtk::TreeRow row = *iter;
	row[m_navaidlistcolumns.m_col_coord] = cursor;
	NavaidsDb::Navaid e((*m_db)(row[m_navaidlistcolumns.m_col_id]));
	if (!e.is_valid())
		return;
	e.set_modtime(time(0));
	e.set_coord(cursor);
	save_noerr(e, row);
}

void NavaidEditor::map_drawflags(int flags)
{
	*m_navaideditormap = (MapRenderer::DrawFlags)flags;
}

void NavaidEditor::save_nostack( NavaidsDb::Navaid & e )
{
	if (!e.is_valid())
		return;
	NavaidsDb::Navaid::Address addr(e.get_address());
	m_db->save(e);
	m_dbchanged = true;
	m_navaideditormap->navaids_changed();
	Gtk::TreeIter iter = m_navaidliststore->children().begin();
	while (iter) {
		Gtk::TreeRow row = *iter;
		if (addr == row[m_navaidlistcolumns.m_col_id]) {
			set_row(row, e);
			return;
		}
		iter++;
	}
	Gtk::TreeModel::Row row(*(m_navaidliststore->append()));
	set_row(row, e);
}

void NavaidEditor::save( NavaidsDb::Navaid & e )
{
	save_nostack(e);
	m_undoredo.save(e);
	update_undoredo_menu();
}

void NavaidEditor::save_noerr(NavaidsDb::Navaid & e, Gtk::TreeModel::Row & row)
{
	if (e.get_address() != row[m_navaidlistcolumns.m_col_id])
		throw std::runtime_error("NavaidEditor::save: ID mismatch");
	try {
		save(e);
	} catch (const sqlite3x::database_error& dberr) {
		NavaidsDb::Navaid eold((*m_db)(row[m_navaidlistcolumns.m_col_id]));
		set_row(row, eold);
	}
}

void NavaidEditor::select( const NavaidsDb::Navaid & e )
{
	Glib::RefPtr<Gtk::TreeSelection> navaideditor_selection = m_navaideditortreeview->get_selection();
	Gtk::TreeIter iter = m_navaidliststore->children().begin();
	while (iter) {
		Gtk::TreeRow row = *iter;
		if (e.get_address() == row[m_navaidlistcolumns.m_col_id]) {
			navaideditor_selection->select(iter);
			m_navaideditortreeview->scroll_to_row(Gtk::TreeModel::Path(iter));
			return;
		}
		iter++;
	}
}

void NavaidEditor::update_undoredo_menu(void)
{
	m_navaideditorundo->set_sensitive(m_undoredo.can_undo());
	m_navaideditorredo->set_sensitive(m_undoredo.can_redo());
}

void NavaidEditor::navaideditor_selection_changed(void)
{
	Glib::RefPtr<Gtk::TreeSelection> navaideditor_selection = m_navaideditortreeview->get_selection();
	Gtk::TreeModel::iterator iter = navaideditor_selection->get_selected();
	if (iter) {
		Gtk::TreeModel::Row row = *iter;
		//Do something with the row.
		std::cerr << "selected row: " << row[m_navaidlistcolumns.m_col_id] << std::endl;
		m_navaideditordeletenavaid->set_sensitive(true);
		Point pt(row[m_navaidlistcolumns.m_col_coord]);
		m_navaideditormap->set_center(pt, row[m_navaidlistcolumns.m_col_elev]);
		m_navaideditormap->set_cursor(pt);
		return;
	}
	std::cerr << "selection cleared" << std::endl;
	m_navaideditordeletenavaid->set_sensitive(false);
}

void NavaidEditor::aboutdialog_response( int response )
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
		Glib::OptionGroup optgroup("navaideditor", "Navaid Editor Options", "Options controlling the Navaid Editor");
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
		osso_context_t* osso_context = osso_initialize("vfrnavaideditor", VERSION, TRUE /* deprecated parameter */, 0 /* Use default Glib main loop context */);
		if (!osso_context) {
			std::cerr << "osso_initialize() failed." << std::endl;
			return OSSO_ERROR;
		}
#endif
#ifdef HAVE_HILDON
		Hildon::init();
#endif
		Glib::set_application_name("VFR Navaid Editor");
		Glib::thread_init();
		NavaidEditor editor(dir_main, auxdbmode, dir_aux);

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
