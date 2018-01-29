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

AirwayEditor::AirwayModelColumns::AirwayModelColumns(void)
{
	add(m_col_id);
	add(m_col_begin_coord);
	add(m_col_end_coord);
	add(m_col_sourceid);
	add(m_col_begin_name);
	add(m_col_end_name);
	add(m_col_name);
	add(m_col_base_level);
	add(m_col_top_level);
	add(m_col_terrain_elev);
	add(m_col_corridor5_elev);
	add(m_col_type);
	add(m_col_label_placement);
	add(m_col_modtime);
}

AirwayEditor::AsyncElevUpdate::AsyncElevUpdate(Engine& eng, AirwaysDbQueryInterface& db, const AirwaysDb::Address & addr, const sigc::slot<void,AirwaysDb::Airway&>& save)
	: m_db(db), m_addr(addr), m_save(save), m_refcnt(2), m_asyncref(true)
{
	std::cerr << "AirwayEditor::AsyncElevUpdate::AsyncElevUpdate: " << (unsigned long long)this << std::endl;
	AirwaysDb::Airway e(m_db(m_addr));
	if (!e.is_valid())
		return;
	m_dispatch.connect(sigc::mem_fun(*this, &AirwayEditor::AsyncElevUpdate::callback));
	m_async = eng.async_elevation_profile(e.get_begin_coord(), e.get_end_coord(), 5);
	if (m_async) {
		m_async->connect(sigc::mem_fun(*this, &AirwayEditor::AsyncElevUpdate::callback_dispatch));
	} else {
		m_asyncref = false;
		unreference();
	}
}

AirwayEditor::AsyncElevUpdate::~AsyncElevUpdate()
{
	cancel();
	std::cerr << "AirwayEditor::AsyncElevUpdate::~AsyncElevUpdate: " << (unsigned long long)this << std::endl;
}

void AirwayEditor::AsyncElevUpdate::reference(void)
{
	++m_refcnt;
}

void AirwayEditor::AsyncElevUpdate::unreference(void)
{
	if (!(--m_refcnt))
		delete this;
}

void AirwayEditor::AsyncElevUpdate::cancel(void)
{
	Engine::ElevationMinMaxResult::cancel(m_async);
	if (m_asyncref) {
		m_asyncref = false;
		unreference();
	}
}

bool AirwayEditor::AsyncElevUpdate::is_done(void)
{
	return !m_async;
}

void AirwayEditor::AsyncElevUpdate::callback_dispatch(void)
{
	m_dispatch();
}

void AirwayEditor::AsyncElevUpdate::callback(void)
{
	if (!m_async)
		return;
	if (!m_async->is_done())
		return;
	if (!m_async->is_error()) {
		AirwaysDb::Airway e(m_db(m_addr));
		if (e.is_valid()) {
			e.set_modtime(time(0));
			TopoDb30::Profile prof(m_async->get_result());
			{
				TopoDb30::elev_t elev(std::numeric_limits<TopoDb30::elev_t>::min());
				for (TopoDb30::Profile::const_iterator i(prof.begin()), e(prof.end()); i != e; ++i) {
					TopoDb30::elev_t el(i->get_elev());
					if (el != TopoDb30::nodata)
						elev = std::max(elev, el);
				}
				if (elev == std::numeric_limits<TopoDb30::elev_t>::min())
					e.set_terrain_elev();
				else
					e.set_terrain_elev(elev * Point::m_to_ft);
			}
			{
				TopoDb30::elev_t el(prof.get_maxelev());
				if (el == TopoDb30::nodata)
					e.set_corridor5_elev();
				else
					e.set_corridor5_elev(el * Point::m_to_ft);
			}
			std::cerr << "terrain elev: " << e.get_terrain_elev() << " / " << e.get_corridor5_elev() << std::endl;
			m_save(e);
		}
	}
	cancel();
}

AirwayEditor::AirwayEditor(const std::string & dir_main, Engine::auxdb_mode_t auxdbmode, const std::string & dir_aux)
	: m_dbchanged(false), m_engine(dir_main, auxdbmode, dir_aux, false, false), m_airwayeditorwindow(0), m_airwayeditortreeview(0), m_airwayeditorstatusbar(0),
	  m_airwayeditorfind(0), m_airwayeditormap(0), m_airwayeditorundo(0), m_airwayeditorredo(0), m_airwayeditordeleteairway(0),
	  m_airwayeditorswapfromto(0), m_airwayeditorelev(0), m_airwayeditormapenable(0),  m_airwayeditorairportdiagramenable(0),
	  m_airwayeditormapfromcoord(0), m_airwayeditormaptocoord(0), m_airwayeditormapzoomin(0), m_airwayeditormapzoomout(0), m_aboutdialog(0)
{
#ifdef HAVE_PQXX
	if (auxdbmode == Engine::auxdb_postgres) {
		m_pgconn = std::unique_ptr<pqxx::connection>(new pqxx::connection(dir_main));
		if (m_pgconn->get_variable("application_name").empty())
			m_pgconn->set_variable("application_name", "airwayeditor");
		m_db.reset(new AirwaysPGDb(*m_pgconn));
	} else
#endif
	{
		m_db.reset(new AirwaysDb(dir_main));
		std::cerr << "Main database " << (dir_main.empty() ? std::string("(default)") : dir_main) << std::endl;
		Glib::ustring dir_aux1(m_engine.get_aux_dir(auxdbmode, dir_aux));
		AirwaysDb *db(static_cast<AirwaysDb *>(m_db.get()));
		if (!dir_aux1.empty() && db->is_dbfile_exists(dir_aux1))
			db->attach(dir_aux1);
		if (db->is_aux_attached())
			std::cerr << "Auxillary airways database " << dir_aux1 << " attached" << std::endl;
	}
	m_refxml = load_glade_file("dbeditor" UIEXT, "airwayeditorwindow");
	get_widget_derived(m_refxml, "airwayeditorwindow", m_airwayeditorwindow);
	get_widget(m_refxml, "airwayeditortreeview", m_airwayeditortreeview);
	get_widget(m_refxml, "airwayeditorstatusbar", m_airwayeditorstatusbar);
	get_widget(m_refxml, "airwayeditorfind", m_airwayeditorfind);
	get_widget_derived(m_refxml, "airwayeditormap", m_airwayeditormap);
	{
#ifdef HAVE_GTKMM2
		Glib::RefPtr<Builder> refxml = load_glade_file_nosearch(m_refxml->get_filename(), "aboutdialog");
#else
		//Glib::RefPtr<Builder> refxml = load_glade_file("dbeditor" UIEXT, "aboutdialog");
		Glib::RefPtr<Builder> refxml = m_refxml;
#endif
		get_widget(refxml, "aboutdialog", m_aboutdialog);
		m_aboutdialog->signal_response().connect(sigc::mem_fun(*this, &AirwayEditor::aboutdialog_response));
	}
	Gtk::Button *buttonclear(0);
	get_widget(m_refxml, "airwayeditorclearbutton", buttonclear);
	buttonclear->signal_clicked().connect(sigc::mem_fun(*this, &AirwayEditor::buttonclear_clicked));
	m_airwayeditorfind->signal_changed().connect(sigc::mem_fun(*this, &AirwayEditor::find_changed));
	Gtk::MenuItem *menu_quit(0), *menu_newairway(0), *menu_cut(0), *menu_copy(0), *menu_paste(0), *menu_delete(0), *menu_preferences(0), *menu_about(0);
	get_widget(m_refxml, "airwayeditorquit", menu_quit);
	get_widget(m_refxml, "airwayeditorundo", m_airwayeditorundo);
	get_widget(m_refxml, "airwayeditorredo", m_airwayeditorredo);
	get_widget(m_refxml, "airwayeditornewairway", menu_newairway);
	get_widget(m_refxml, "airwayeditordeleteairway", m_airwayeditordeleteairway);
	get_widget(m_refxml, "airwayeditorswapfromto", m_airwayeditorswapfromto);
	get_widget(m_refxml, "airwayeditorelev", m_airwayeditorelev);
	get_widget(m_refxml, "airwayeditormapenable", m_airwayeditormapenable);
	get_widget(m_refxml, "airwayeditorairportdiagramenable", m_airwayeditorairportdiagramenable);
	get_widget(m_refxml, "airwayeditormapfromcoord", m_airwayeditormapfromcoord);
	get_widget(m_refxml, "airwayeditormaptocoord", m_airwayeditormaptocoord);
	get_widget(m_refxml, "airwayeditormapzoomin", m_airwayeditormapzoomin);
	get_widget(m_refxml, "airwayeditormapzoomout", m_airwayeditormapzoomout);
	get_widget(m_refxml, "airwayeditorcut", menu_cut);
	get_widget(m_refxml, "airwayeditorcopy", menu_copy);
	get_widget(m_refxml, "airwayeditorpaste", menu_paste);
	get_widget(m_refxml, "airwayeditordelete", menu_delete);
	get_widget(m_refxml, "airwayeditorpreferences", menu_preferences);
	get_widget(m_refxml, "airwayeditorabout", menu_about);
	menu_quit->signal_activate().connect(sigc::mem_fun(*this, &AirwayEditor::menu_quit_activate));
	m_airwayeditorundo->signal_activate().connect(sigc::mem_fun(*this, &AirwayEditor::menu_undo_activate));
	m_airwayeditorredo->signal_activate().connect(sigc::mem_fun(*this, &AirwayEditor::menu_redo_activate));
	menu_newairway->signal_activate().connect(sigc::mem_fun(*this, &AirwayEditor::menu_newairway_activate));
	m_airwayeditordeleteairway->signal_activate().connect(sigc::mem_fun(*this, &AirwayEditor::menu_deleteairway_activate));
	m_airwayeditorswapfromto->signal_activate().connect(sigc::mem_fun(*this, &AirwayEditor::menu_swapfromto_activate));
	m_airwayeditorelev->signal_activate().connect(sigc::mem_fun(*this, &AirwayEditor::menu_elev_activate));
	m_airwayeditormapenable->signal_toggled().connect(sigc::mem_fun(*this, &AirwayEditor::menu_mapenable_toggled));
	m_airwayeditorairportdiagramenable->signal_toggled().connect(sigc::mem_fun(*this, &AirwayEditor::menu_airportdiagramenable_toggled));
	m_airwayeditormapfromcoord->signal_toggled().connect(sigc::mem_fun(*this, &AirwayEditor::menu_mapcoord_toggled));
	m_airwayeditormaptocoord->signal_toggled().connect(sigc::mem_fun(*this, &AirwayEditor::menu_mapcoord_toggled));
	m_airwayeditormapzoomin->signal_activate().connect(sigc::mem_fun(*this, &AirwayEditor::menu_mapzoomin_activate));
	m_airwayeditormapzoomout->signal_activate().connect(sigc::mem_fun(*this, &AirwayEditor::menu_mapzoomout_activate));
	if (m_airwayeditormapenable->get_active()) {
		m_airwayeditormapzoomin->show();
		m_airwayeditormapzoomout->show();
	} else {
		m_airwayeditormapzoomin->hide();
		m_airwayeditormapzoomout->hide();
	}
	m_airwayeditorwindow->signal_zoomin().connect(sigc::bind_return(sigc::mem_fun(*this, &AirwayEditor::menu_mapzoomin_activate), true));
	m_airwayeditorwindow->signal_zoomout().connect(sigc::bind_return(sigc::mem_fun(*this, &AirwayEditor::menu_mapzoomout_activate), true));
	menu_cut->signal_activate().connect(sigc::mem_fun(*this, &AirwayEditor::menu_cut_activate));
	menu_copy->signal_activate().connect(sigc::mem_fun(*this, &AirwayEditor::menu_copy_activate));
	menu_paste->signal_activate().connect(sigc::mem_fun(*this, &AirwayEditor::menu_paste_activate));
	menu_delete->signal_activate().connect(sigc::mem_fun(*this, &AirwayEditor::menu_delete_activate));
	menu_preferences->signal_activate().connect(sigc::mem_fun(*this, &AirwayEditor::menu_prefs_activate));
	menu_about->signal_activate().connect(sigc::mem_fun(*this, &AirwayEditor::menu_about_activate));
	update_undoredo_menu();
	// Vector Map
	m_airwayeditormap->set_engine(m_engine);
	m_airwayeditormap->set_renderer(VectorMapArea::renderer_vmap);
	m_airwayeditormap->signal_cursor().connect(sigc::mem_fun(*this, &AirwayEditor::map_cursor));
	m_airwayeditormap->set_map_scale(m_engine.get_prefs().mapscale);
	AirwayEditor::map_drawflags(m_engine.get_prefs().mapflags);
	m_engine.get_prefs().mapflags.signal_change().connect(sigc::mem_fun(*this, &AirwayEditor::map_drawflags));
	// Airway List
	m_airwayliststore = Gtk::ListStore::create(m_airwaylistcolumns);
	m_airwayeditortreeview->set_model(m_airwayliststore);
	Gtk::CellRendererText *name_renderer = Gtk::manage(new Gtk::CellRendererText());
	Gtk::TreeViewColumn *name_column = m_airwayeditortreeview->get_column(m_airwayeditortreeview->append_column(_("Name"), *name_renderer) - 1);
	if (name_column) {
		name_column->add_attribute(*name_renderer, "text", m_airwaylistcolumns.m_col_name);
		name_column->set_sort_column(m_airwaylistcolumns.m_col_name);
	}
	name_renderer->signal_edited().connect(sigc::mem_fun(*this, &AirwayEditor::edited_name));
	name_renderer->set_property("editable", true);
	Gtk::CellRendererText *name_begin_renderer = Gtk::manage(new Gtk::CellRendererText());
	Gtk::TreeViewColumn *name_begin_column = m_airwayeditortreeview->get_column(m_airwayeditortreeview->append_column(_("From"), *name_begin_renderer) - 1);
	if (name_begin_column) {
		name_begin_column->add_attribute(*name_begin_renderer, "text", m_airwaylistcolumns.m_col_begin_name);
		name_begin_column->set_sort_column(m_airwaylistcolumns.m_col_begin_name);
	}
	name_begin_renderer->signal_edited().connect(sigc::mem_fun(*this, &AirwayEditor::edited_begin_name));
	name_begin_renderer->set_property("editable", true);
	CoordCellRenderer *coord_begin_renderer = Gtk::manage(new CoordCellRenderer());
	Gtk::TreeViewColumn *coord_begin_column = m_airwayeditortreeview->get_column(m_airwayeditortreeview->append_column(_("Coordinate"), *coord_begin_renderer) - 1);
	if (coord_begin_column) {
		coord_begin_column->add_attribute(*coord_begin_renderer, coord_begin_renderer->get_property_name(), m_airwaylistcolumns.m_col_begin_coord);
		coord_begin_column->set_sort_column(m_airwaylistcolumns.m_col_begin_coord);
	}
	coord_begin_renderer->set_property("editable", true);
	coord_begin_renderer->signal_edited().connect(sigc::mem_fun(*this, &AirwayEditor::edited_begin_coord));
	Gtk::CellRendererText *name_end_renderer = Gtk::manage(new Gtk::CellRendererText());
	Gtk::TreeViewColumn *name_end_column = m_airwayeditortreeview->get_column(m_airwayeditortreeview->append_column(_("To"), *name_end_renderer) - 1);
	if (name_end_column) {
		name_end_column->add_attribute(*name_end_renderer, "text", m_airwaylistcolumns.m_col_end_name);
		name_end_column->set_sort_column(m_airwaylistcolumns.m_col_end_name);
	}
	name_end_renderer->signal_edited().connect(sigc::mem_fun(*this, &AirwayEditor::edited_end_name));
	name_end_renderer->set_property("editable", true);
	CoordCellRenderer *coord_end_renderer = Gtk::manage(new CoordCellRenderer());
	Gtk::TreeViewColumn *coord_end_column = m_airwayeditortreeview->get_column(m_airwayeditortreeview->append_column(_("Coordinate"), *coord_end_renderer) - 1);
	if (coord_end_column) {
		coord_end_column->add_attribute(*coord_end_renderer, coord_end_renderer->get_property_name(), m_airwaylistcolumns.m_col_end_coord);
		coord_end_column->set_sort_column(m_airwaylistcolumns.m_col_end_coord);
	}
	coord_end_renderer->set_property("editable", true);
	coord_end_renderer->signal_edited().connect(sigc::mem_fun(*this, &AirwayEditor::edited_end_coord));
	Gtk::CellRendererText *baselevel_renderer = Gtk::manage(new Gtk::CellRendererText());
	Gtk::TreeViewColumn *baselevel_column = m_airwayeditortreeview->get_column(m_airwayeditortreeview->append_column(_("Base FL"), *baselevel_renderer) - 1);
	if (baselevel_column) {
	    	baselevel_column->add_attribute(*baselevel_renderer, "text", m_airwaylistcolumns.m_col_base_level);
		baselevel_column->set_sort_column(m_airwaylistcolumns.m_col_base_level);
	}
	baselevel_renderer->set_property("xalign", 1.0);
	baselevel_renderer->signal_edited().connect(sigc::mem_fun(*this, &AirwayEditor::edited_base_level));
	baselevel_renderer->set_property("editable", true);
	Gtk::CellRendererText *toplevel_renderer = Gtk::manage(new Gtk::CellRendererText());
	Gtk::TreeViewColumn *toplevel_column = m_airwayeditortreeview->get_column(m_airwayeditortreeview->append_column(_("Top FL"), *toplevel_renderer) - 1);
	if (toplevel_column) {
	     	toplevel_column->add_attribute(*toplevel_renderer, "text", m_airwaylistcolumns.m_col_top_level);
		toplevel_column->set_sort_column(m_airwaylistcolumns.m_col_top_level);
	}
	toplevel_renderer->set_property("xalign", 1.0);
	toplevel_renderer->signal_edited().connect(sigc::mem_fun(*this, &AirwayEditor::edited_top_level));
	toplevel_renderer->set_property("editable", true);
	Gtk::CellRendererText *terrainelev_renderer = Gtk::manage(new Gtk::CellRendererText());
	Gtk::TreeViewColumn *terrainelev_column = m_airwayeditortreeview->get_column(m_airwayeditortreeview->append_column(_("T Elev"), *terrainelev_renderer) - 1);
	if (terrainelev_column) {
	     	terrainelev_column->add_attribute(*terrainelev_renderer, "text", m_airwaylistcolumns.m_col_terrain_elev);
		terrainelev_column->set_sort_column(m_airwaylistcolumns.m_col_terrain_elev);
	}
	terrainelev_renderer->set_property("xalign", 1.0);
	terrainelev_renderer->signal_edited().connect(sigc::mem_fun(*this, &AirwayEditor::edited_terrain_elev));
	terrainelev_renderer->set_property("editable", true);
	Gtk::CellRendererText *corridor5elev_renderer = Gtk::manage(new Gtk::CellRendererText());
	Gtk::TreeViewColumn *corridor5elev_column = m_airwayeditortreeview->get_column(m_airwayeditortreeview->append_column(_("T5 Elev"), *corridor5elev_renderer) - 1);
	if (corridor5elev_column) {
	     	corridor5elev_column->add_attribute(*corridor5elev_renderer, "text", m_airwaylistcolumns.m_col_corridor5_elev);
		corridor5elev_column->set_sort_column(m_airwaylistcolumns.m_col_corridor5_elev);
	}
	corridor5elev_renderer->set_property("xalign", 1.0);
	corridor5elev_renderer->signal_edited().connect(sigc::mem_fun(*this, &AirwayEditor::edited_corridor5_elev));
	corridor5elev_renderer->set_property("editable", true);
	AirwayTypeRenderer *type_renderer = Gtk::manage(new AirwayTypeRenderer());
	Gtk::TreeViewColumn *type_column = m_airwayeditortreeview->get_column(m_airwayeditortreeview->append_column(_("Type"), *type_renderer) - 1);
	if (type_column) {
		type_column->add_attribute(*type_renderer, type_renderer->get_property_name(), m_airwaylistcolumns.m_col_type);
		type_column->set_sort_column(m_airwaylistcolumns.m_col_type);
	}
	type_renderer->set_property("editable", true);
	type_renderer->signal_edited().connect(sigc::mem_fun(*this, &AirwayEditor::edited_type));
	LabelPlacementRenderer *label_placement_renderer = Gtk::manage(new LabelPlacementRenderer());
	Gtk::TreeViewColumn *label_placement_column = m_airwayeditortreeview->get_column(m_airwayeditortreeview->append_column(_("Label"), *label_placement_renderer) - 1);
	if (label_placement_column) {
		label_placement_column->add_attribute(*label_placement_renderer, label_placement_renderer->get_property_name(), m_airwaylistcolumns.m_col_label_placement);
		label_placement_column->set_sort_column(m_airwaylistcolumns.m_col_label_placement);
	}
	label_placement_renderer->set_property("editable", true);
	label_placement_renderer->signal_edited().connect(sigc::mem_fun(*this, &AirwayEditor::edited_label_placement));
	Gtk::CellRendererText *srcid_renderer = Gtk::manage(new Gtk::CellRendererText());
	Gtk::TreeViewColumn *srcid_column = m_airwayeditortreeview->get_column(m_airwayeditortreeview->append_column(_("Source ID"), *srcid_renderer) - 1);
	if (srcid_column) {
 		srcid_column->add_attribute(*srcid_renderer, "text", m_airwaylistcolumns.m_col_sourceid);
		srcid_column->set_sort_column(m_airwaylistcolumns.m_col_sourceid);
	}
	srcid_renderer->signal_edited().connect(sigc::mem_fun(*this, &AirwayEditor::edited_sourceid));
	srcid_renderer->set_property("editable", true);
	DateTimeCellRenderer *modtime_renderer = Gtk::manage(new DateTimeCellRenderer());
	Gtk::TreeViewColumn *modtime_column = m_airwayeditortreeview->get_column(m_airwayeditortreeview->append_column(_("Mod Time"), *modtime_renderer) - 1);
	if (modtime_column) {
		modtime_column->add_attribute(*modtime_renderer, modtime_renderer->get_property_name(), m_airwaylistcolumns.m_col_modtime);
		modtime_column->set_sort_column(m_airwaylistcolumns.m_col_modtime);
	}
	modtime_renderer->set_property("editable", true);
	modtime_renderer->signal_edited().connect(sigc::mem_fun(*this, &AirwayEditor::edited_modtime));
	//m_airwayeditortreeview->append_column_editable(_("Mod Time"), m_airwaylistcolumns.m_col_modtime);
	for (unsigned int i = 0; i < 8; ++i) {
		Gtk::TreeViewColumn *col = m_airwayeditortreeview->get_column(i);
		if (!col)
			continue;
		col->set_resizable(true);
		col->set_reorderable(true);
	}
	m_airwayeditortreeview->columns_autosize();
	m_airwayeditortreeview->set_enable_search(true);
	m_airwayeditortreeview->set_search_column(m_airwaylistcolumns.m_col_name);
	// selection
	Glib::RefPtr<Gtk::TreeSelection> airwayeditor_selection = m_airwayeditortreeview->get_selection();
	airwayeditor_selection->set_mode(Gtk::SELECTION_SINGLE);
	airwayeditor_selection->signal_changed().connect(sigc::mem_fun(*this, &AirwayEditor::airwayeditor_selection_changed));
	//airwayeditor_selection->set_select_function(sigc::mem_fun(*this, &AirwayEditor::airwayeditor_select_function));
	airwayeditor_selection_changed();

	//set_airwaylist("");
	m_dbselectdialog.signal_byname().connect(sigc::mem_fun(*this, &AirwayEditor::dbselect_byname));
	m_dbselectdialog.signal_byrectangle().connect(sigc::mem_fun(*this, &AirwayEditor::dbselect_byrect));
	m_dbselectdialog.signal_bytime().connect(sigc::mem_fun(*this, &AirwayEditor::dbselect_bytime));
	m_dbselectdialog.show();
	m_prefswindow = PrefsWindow::create(m_engine.get_prefs());
	m_airwayeditorwindow->show();
}

AirwayEditor::~AirwayEditor()
{
	if (m_dbchanged) {
		std::cerr << "Analyzing database..." << std::endl;
		m_db->analyze();
		m_db->vacuum();
	}
	m_db->close();
}

void AirwayEditor::dbselect_byname( const Glib::ustring & s, AirwaysDb::comp_t comp )
{
	AirwaysDb::elementvector_t els(m_db->find_by_text(s, 0, comp, 0));
	m_airwayliststore->clear();
	for (AirwaysDb::elementvector_t::iterator ei(els.begin()), ee(els.end()); ei != ee; ++ei) {
		AirwaysDb::element_t& e(*ei);
		if (!e.is_valid())
			continue;
		Gtk::TreeModel::Row row(*(m_airwayliststore->append()));
		set_row(row, e);
	}
}

void AirwayEditor::dbselect_byrect( const Rect & r )
{
	AirwaysDb::elementvector_t els(m_db->find_by_rect(r, 0));
	m_airwayliststore->clear();
	for (AirwaysDb::elementvector_t::iterator ei(els.begin()), ee(els.end()); ei != ee; ++ei) {
		AirwaysDb::element_t& e(*ei);
		if (!e.is_valid())
			continue;
		Gtk::TreeModel::Row row(*(m_airwayliststore->append()));
		set_row(row, e);
	}
}

void AirwayEditor::dbselect_bytime(time_t timefrom, time_t timeto)
{
	AirwaysDb::elementvector_t els(m_db->find_by_time(timefrom, timeto, 0));
	m_airwayliststore->clear();
	for (AirwaysDb::elementvector_t::iterator ei(els.begin()), ee(els.end()); ei != ee; ++ei) {
		AirwaysDb::element_t& e(*ei);
		if (!e.is_valid())
			continue;
		Gtk::TreeModel::Row row(*(m_airwayliststore->append()));
		set_row(row, e);
	}
}

void AirwayEditor::buttonclear_clicked(void)
{
	m_airwayeditorfind->set_text("");
}

void AirwayEditor::find_changed(void)
{
	dbselect_byname(m_airwayeditorfind->get_text());
}

void AirwayEditor::menu_quit_activate(void)
{
	m_airwayeditorwindow->hide();
}

void AirwayEditor::menu_undo_activate(void)
{
	AirwaysDb::Airway e(m_undoredo.undo());
	if (e.is_valid()) {
		save_nostack(e);
		select(e);
	}
	update_undoredo_menu();
}

void AirwayEditor::menu_redo_activate(void)
{
	AirwaysDb::Airway e(m_undoredo.redo());
	if (e.is_valid()) {
		save_nostack(e);
		select(e);
	}
	update_undoredo_menu();
}

void AirwayEditor::menu_newairway_activate(void)
{
	AirwaysDb::Airway e;
	e.set_type(AirwaysDb::Airway::airway_both);
	e.set_sourceid(make_sourceid());
	e.set_modtime(time(0));
	save(e);
	select(e);
}

void AirwayEditor::menu_deleteairway_activate(void)
{
	Glib::RefPtr<Gtk::TreeSelection> airwayeditor_selection = m_airwayeditortreeview->get_selection();
	Gtk::TreeModel::iterator iter = airwayeditor_selection->get_selected();
	if (!iter)
		return;
	Gtk::TreeModel::Row row = *iter;
	std::cerr << "deleting row: " << row[m_airwaylistcolumns.m_col_id] << std::endl;
	AirwaysDb::Airway e((*m_db)(row[m_airwaylistcolumns.m_col_id]));
	if (!e.is_valid())
		return;
	m_undoredo.erase(e);
	m_db->erase(e);
	m_airwayliststore->erase(iter);
}

void AirwayEditor::menu_swapfromto_activate(void)
{
	Glib::RefPtr<Gtk::TreeSelection> airwayeditor_selection = m_airwayeditortreeview->get_selection();
	Gtk::TreeModel::iterator iter = airwayeditor_selection->get_selected();
	if (!iter)
		return;
	Gtk::TreeModel::Row row = *iter;
	std::cerr << "swapping from/to row: " << row[m_airwaylistcolumns.m_col_id] << std::endl;
	AirwaysDb::Airway e((*m_db)(row[m_airwaylistcolumns.m_col_id]));
	if (!e.is_valid())
		return;
	Point ptb(e.get_end_coord()), pte(e.get_begin_coord());
	Glib::ustring nb(e.get_end_name()), ne(e.get_begin_name());
	e.set_begin_coord(ptb);
	e.set_end_coord(pte);
	e.set_begin_name(nb);
	e.set_end_name(ne);
	save_noerr(e, row);
}

void AirwayEditor::menu_elev_activate(void)
{
	Glib::RefPtr<Gtk::TreeSelection> airwayeditor_selection = m_airwayeditortreeview->get_selection();
	std::vector<Gtk::TreeModel::Path> sel(airwayeditor_selection->get_selected_rows());
	if (sel.empty())
		return;
	if (sel.size() > 1) {
		Gtk::MessageDialog *dlg = new Gtk::MessageDialog(_("Recompute ground elevation for multiple airways?"), false, Gtk::MESSAGE_QUESTION, Gtk::BUTTONS_YES_NO, true);
		int res = dlg->run();
		delete dlg;
		if (res != Gtk::RESPONSE_YES) {
			if (res != Gtk::RESPONSE_NO)
				std::cerr << "unexpected dialog result: " << res << std::endl;
			return;
		}
	}
	for (std::vector<Gtk::TreeModel::Path>::iterator si(sel.begin()), se(sel.end()); si != se; si++) {
		Gtk::TreeModel::iterator iter(m_airwayliststore->get_iter(*si));
		if (!iter) {
			std::cerr << "cannot get iterator for path " << (*si).to_string() << " ?""?" << std::endl;
			continue;
		}
		Gtk::TreeModel::Row row = *iter;
		std::cerr << "recomputing ground elevation for row: " << row[m_airwaylistcolumns.m_col_id] << std::endl;
		Glib::RefPtr<AsyncElevUpdate> aeu(new AsyncElevUpdate(m_engine, *m_db.get(), row[m_airwaylistcolumns.m_col_id], sigc::mem_fun(*this, &AirwayEditor::save)));
	}
}

void AirwayEditor::menu_cut_activate(void)
{
	Gtk::Widget *focus = m_airwayeditorwindow->get_focus();
	if (!focus)
		return;
	Gtk::Editable *edit = dynamic_cast<Gtk::Editable *>(focus);
	if (!edit)
		return;
	edit->cut_clipboard();
}

void AirwayEditor::menu_copy_activate(void)
{
	Gtk::Widget *focus = m_airwayeditorwindow->get_focus();
	if (!focus)
		return;
	Gtk::Editable *edit = dynamic_cast<Gtk::Editable *>(focus);
	if (!edit)
		return;
	edit->copy_clipboard();
}

void AirwayEditor::menu_paste_activate(void)
{
	Gtk::Widget *focus = m_airwayeditorwindow->get_focus();
	if (!focus)
		return;
	Gtk::Editable *edit = dynamic_cast<Gtk::Editable *>(focus);
	if (!edit)
		return;
	edit->paste_clipboard();
}

void AirwayEditor::menu_delete_activate(void)
{
	Gtk::Widget *focus = m_airwayeditorwindow->get_focus();
	if (!focus)
		return;
	Gtk::Editable *edit = dynamic_cast<Gtk::Editable *>(focus);
	if (!edit)
		return;
	edit->delete_selection();
}

void AirwayEditor::menu_mapenable_toggled(void)
{
	if (m_airwayeditormapenable->get_active()) {
		m_airwayeditorairportdiagramenable->set_active(false);
		m_airwayeditormap->set_renderer(VectorMapArea::renderer_vmap);
		m_airwayeditormap->set_map_scale(m_engine.get_prefs().mapscale);
		m_airwayeditormap->show();
		m_airwayeditormapzoomin->show();
		m_airwayeditormapzoomout->show();
	} else {
		m_airwayeditormap->hide();
		m_airwayeditormapzoomin->hide();
		m_airwayeditormapzoomout->hide();
	}
}

void AirwayEditor::menu_airportdiagramenable_toggled(void)
{
	if (m_airwayeditorairportdiagramenable->get_active()) {
		m_airwayeditormapenable->set_active(false);
		m_airwayeditormap->set_renderer(VectorMapArea::renderer_airportdiagram);
		m_airwayeditormap->set_map_scale(m_engine.get_prefs().mapscaleairportdiagram);
		m_airwayeditormap->show();
		m_airwayeditormapzoomin->show();
		m_airwayeditormapzoomout->show();
	} else {
		m_airwayeditormap->hide();
		m_airwayeditormapzoomin->hide();
		m_airwayeditormapzoomout->hide();
	}
}

void AirwayEditor::menu_mapcoord_toggled(void)
{
	airwayeditor_selection_changed();
}

void AirwayEditor::menu_mapzoomin_activate(void)
{
	m_airwayeditormap->zoom_in();
	if (m_airwayeditormap->get_renderer() == VectorMapArea::renderer_airportdiagram)
		m_engine.get_prefs().mapscaleairportdiagram = m_airwayeditormap->get_map_scale();
	else
		m_engine.get_prefs().mapscale = m_airwayeditormap->get_map_scale();
}

void AirwayEditor::menu_mapzoomout_activate(void)
{
	m_airwayeditormap->zoom_out();
	if (m_airwayeditormap->get_renderer() == VectorMapArea::renderer_airportdiagram)
		m_engine.get_prefs().mapscaleairportdiagram = m_airwayeditormap->get_map_scale();
	else
		m_engine.get_prefs().mapscale = m_airwayeditormap->get_map_scale();
}

void AirwayEditor::menu_prefs_activate(void)
{
	if (m_prefswindow)
		m_prefswindow->show();
}

void AirwayEditor::menu_about_activate(void)
{
	m_aboutdialog->show();
}

void AirwayEditor::set_row( Gtk::TreeModel::Row & row, const AirwaysDb::Airway & e )
{
	row[m_airwaylistcolumns.m_col_id] = e.get_address();
	row[m_airwaylistcolumns.m_col_begin_coord] = e.get_begin_coord();
	row[m_airwaylistcolumns.m_col_end_coord] = e.get_end_coord();
	row[m_airwaylistcolumns.m_col_sourceid] = e.get_sourceid();
	row[m_airwaylistcolumns.m_col_begin_name] = e.get_begin_name();
	row[m_airwaylistcolumns.m_col_end_name] = e.get_end_name();
	row[m_airwaylistcolumns.m_col_name] = e.get_name();
	row[m_airwaylistcolumns.m_col_base_level] = e.get_base_level();
	row[m_airwaylistcolumns.m_col_top_level] = e.get_top_level();
	row[m_airwaylistcolumns.m_col_terrain_elev] = e.get_terrain_elev();
	row[m_airwaylistcolumns.m_col_corridor5_elev] = e.get_corridor5_elev();
	row[m_airwaylistcolumns.m_col_type] = e.get_type();
	row[m_airwaylistcolumns.m_col_label_placement] = e.get_label_placement();
	row[m_airwaylistcolumns.m_col_modtime] = e.get_modtime();
}

void AirwayEditor::edited_begin_coord( const Glib::ustring & path_string, const Glib::ustring& new_text, Point new_coord )
{
	Gtk::TreePath path(path_string);
	Gtk::TreeModel::iterator iter = m_airwayliststore->get_iter(path);
	if (!iter)
		return;
	Gtk::TreeRow row = *iter;
	row[m_airwaylistcolumns.m_col_begin_coord] = new_coord;
	AirwaysDb::Airway e((*m_db)(row[m_airwaylistcolumns.m_col_id]));
	if (!e.is_valid())
		return;
	e.set_modtime(time(0));
	e.set_begin_coord(new_coord);
	save_noerr(e, row);
	m_airwayeditormap->set_center(new_coord, 0);
	m_airwayeditormap->set_cursor(new_coord);
}

void AirwayEditor::edited_end_coord( const Glib::ustring & path_string, const Glib::ustring& new_text, Point new_coord )
{
	Gtk::TreePath path(path_string);
	Gtk::TreeModel::iterator iter = m_airwayliststore->get_iter(path);
	if (!iter)
		return;
	Gtk::TreeRow row = *iter;
	row[m_airwaylistcolumns.m_col_end_coord] = new_coord;
	AirwaysDb::Airway e((*m_db)(row[m_airwaylistcolumns.m_col_id]));
	if (!e.is_valid())
		return;
	e.set_modtime(time(0));
	e.set_end_coord(new_coord);
	save_noerr(e, row);
	m_airwayeditormap->set_center(new_coord, 0);
	m_airwayeditormap->set_cursor(new_coord);
}

void AirwayEditor::edited_sourceid( const Glib::ustring & path_string, const Glib::ustring & new_text )
{
	Gtk::TreePath path(path_string);
	Gtk::TreeModel::iterator iter = m_airwayliststore->get_iter(path);
	if (!iter)
		return;
	Gtk::TreeRow row = *iter;
	row[m_airwaylistcolumns.m_col_sourceid] = new_text;
	AirwaysDb::Airway e((*m_db)(row[m_airwaylistcolumns.m_col_id]));
	if (!e.is_valid())
		return;
	e.set_modtime(time(0));
	e.set_sourceid(new_text);
	save_noerr(e, row);
}

void AirwayEditor::edited_begin_name( const Glib::ustring & path_string, const Glib::ustring & new_text )
{
	Gtk::TreePath path(path_string);
	Gtk::TreeModel::iterator iter = m_airwayliststore->get_iter(path);
	if (!iter)
		return;
	Gtk::TreeRow row = *iter;
	row[m_airwaylistcolumns.m_col_begin_name] = new_text;
	AirwaysDb::Airway e((*m_db)(row[m_airwaylistcolumns.m_col_id]));
	if (!e.is_valid())
		return;
	e.set_modtime(time(0));
	e.set_begin_name(new_text);
	save_noerr(e, row);
}

void AirwayEditor::edited_end_name( const Glib::ustring & path_string, const Glib::ustring & new_text )
{
	Gtk::TreePath path(path_string);
	Gtk::TreeModel::iterator iter = m_airwayliststore->get_iter(path);
	if (!iter)
		return;
	Gtk::TreeRow row = *iter;
	row[m_airwaylistcolumns.m_col_end_name] = new_text;
	AirwaysDb::Airway e((*m_db)(row[m_airwaylistcolumns.m_col_id]));
	if (!e.is_valid())
		return;
	e.set_modtime(time(0));
	e.set_end_name(new_text);
	save_noerr(e, row);
}

void AirwayEditor::edited_name( const Glib::ustring & path_string, const Glib::ustring & new_text )
{
	Gtk::TreePath path(path_string);
	Gtk::TreeModel::iterator iter = m_airwayliststore->get_iter(path);
	if (!iter)
		return;
	Gtk::TreeRow row = *iter;
	row[m_airwaylistcolumns.m_col_name] = new_text;
	AirwaysDb::Airway e((*m_db)(row[m_airwaylistcolumns.m_col_id]));
	if (!e.is_valid())
		return;
	e.set_modtime(time(0));
	e.set_name(new_text);
	save_noerr(e, row);
}

void AirwayEditor::edited_base_level( const Glib::ustring & path_string, const Glib::ustring & new_text )
{
	Gtk::TreePath path(path_string);
	Gtk::TreeModel::iterator iter = m_airwayliststore->get_iter(path);
	if (!iter)
		return;
	Gtk::TreeRow row = *iter;
	char *cp;
	int32_t lvl = strtol(new_text.c_str(), &cp, 0);
	row[m_airwaylistcolumns.m_col_base_level] = lvl;
	AirwaysDb::Airway e((*m_db)(row[m_airwaylistcolumns.m_col_id]));
	if (!e.is_valid())
		return;
	e.set_modtime(time(0));
	e.set_base_level(lvl);
	if (e.get_base_level() > e.get_top_level()) {
		int32_t tl(e.get_base_level()), bl(e.get_top_level());
		e.set_base_level(bl);
		e.set_top_level(tl);
		row[m_airwaylistcolumns.m_col_base_level] = bl;
		row[m_airwaylistcolumns.m_col_top_level] = tl;
	}
	save_noerr(e, row);
}

void AirwayEditor::edited_top_level( const Glib::ustring & path_string, const Glib::ustring & new_text )
{
	Gtk::TreePath path(path_string);
	Gtk::TreeModel::iterator iter = m_airwayliststore->get_iter(path);
	if (!iter)
		return;
	Gtk::TreeRow row = *iter;
	char *cp;
	int32_t lvl = strtol(new_text.c_str(), &cp, 0);
	row[m_airwaylistcolumns.m_col_top_level] = lvl;
	AirwaysDb::Airway e((*m_db)(row[m_airwaylistcolumns.m_col_id]));
	if (!e.is_valid())
		return;
	e.set_modtime(time(0));
	e.set_top_level(lvl);
	if (e.get_base_level() > e.get_top_level()) {
		int32_t tl(e.get_base_level()), bl(e.get_top_level());
		e.set_base_level(bl);
		e.set_top_level(tl);
		row[m_airwaylistcolumns.m_col_base_level] = bl;
		row[m_airwaylistcolumns.m_col_top_level] = tl;
	}
	save_noerr(e, row);
}

void AirwayEditor::edited_terrain_elev( const Glib::ustring & path_string, const Glib::ustring & new_text )
{
	Gtk::TreePath path(path_string);
	Gtk::TreeModel::iterator iter = m_airwayliststore->get_iter(path);
	if (!iter)
		return;
	Gtk::TreeRow row = *iter;
	char *cp;
	int32_t lvl = strtol(new_text.c_str(), &cp, 0);
	row[m_airwaylistcolumns.m_col_terrain_elev] = lvl;
	AirwaysDb::Airway e((*m_db)(row[m_airwaylistcolumns.m_col_id]));
	if (!e.is_valid())
		return;
	e.set_modtime(time(0));
	e.set_terrain_elev(lvl);
	if (e.get_terrain_elev() > e.get_corridor5_elev()) {
		e.set_corridor5_elev(e.get_terrain_elev());
		row[m_airwaylistcolumns.m_col_terrain_elev] = e.get_terrain_elev();
		row[m_airwaylistcolumns.m_col_corridor5_elev] = e.get_corridor5_elev();
	}
	save_noerr(e, row);
}

void AirwayEditor::edited_corridor5_elev( const Glib::ustring & path_string, const Glib::ustring & new_text )
{
	Gtk::TreePath path(path_string);
	Gtk::TreeModel::iterator iter = m_airwayliststore->get_iter(path);
	if (!iter)
		return;
	Gtk::TreeRow row = *iter;
	char *cp;
	int32_t lvl = strtol(new_text.c_str(), &cp, 0);
	row[m_airwaylistcolumns.m_col_corridor5_elev] = lvl;
	AirwaysDb::Airway e((*m_db)(row[m_airwaylistcolumns.m_col_id]));
	if (!e.is_valid())
		return;
	e.set_modtime(time(0));
	e.set_corridor5_elev(lvl);
	if (e.get_terrain_elev() > e.get_corridor5_elev()) {
		e.set_corridor5_elev(e.get_terrain_elev());
		row[m_airwaylistcolumns.m_col_terrain_elev] = e.get_terrain_elev();
		row[m_airwaylistcolumns.m_col_corridor5_elev] = e.get_corridor5_elev();
	}
	save_noerr(e, row);
}

void AirwayEditor::edited_type( const Glib::ustring & path_string, const Glib::ustring & new_text, AirwaysDb::Airway::airway_type_t new_type )
{
	Gtk::TreePath path(path_string);
	Gtk::TreeModel::iterator iter = m_airwayliststore->get_iter(path);
	if (!iter)
		return;
	Gtk::TreeRow row = *iter;
	if (new_type == AirwaysDb::Airway::airway_invalid)
		return;
	row[m_airwaylistcolumns.m_col_type] = new_type;
	AirwaysDb::Airway e((*m_db)(row[m_airwaylistcolumns.m_col_id]));
	if (!e.is_valid())
		return;
	e.set_modtime(time(0));
	e.set_type(new_type);
	save_noerr(e, row);
}

void AirwayEditor::edited_label_placement( const Glib::ustring & path_string, const Glib::ustring & new_text, NavaidsDb::Navaid::label_placement_t new_placement )
{
	Gtk::TreePath path(path_string);
	Gtk::TreeModel::iterator iter = m_airwayliststore->get_iter(path);
	if (!iter)
		return;
	Gtk::TreeRow row = *iter;
	row[m_airwaylistcolumns.m_col_label_placement] = new_placement;
	AirwaysDb::Airway e((*m_db)(row[m_airwaylistcolumns.m_col_id]));
	if (!e.is_valid())
		return;
	e.set_modtime(time(0));
	e.set_label_placement(new_placement);
	save_noerr(e, row);
}

void AirwayEditor::edited_modtime( const Glib::ustring & path_string, const Glib::ustring& new_text, time_t new_time )
{
	Gtk::TreePath path(path_string);
	Gtk::TreeModel::iterator iter = m_airwayliststore->get_iter(path);
	if (!iter)
		return;
	Gtk::TreeRow row = *iter;
	row[m_airwaylistcolumns.m_col_modtime] = new_time;
	AirwaysDb::Airway e((*m_db)(row[m_airwaylistcolumns.m_col_id]));
	if (!e.is_valid())
		return;
	e.set_modtime(time(0));
	e.set_modtime(new_time);
	save_noerr(e, row);
}

void AirwayEditor::map_cursor( Point cursor, VectorMapArea::cursor_validity_t valid )
{
	if (valid != VectorMapArea::cursor_mouse)
		return;
	Glib::RefPtr<Gtk::TreeSelection> airwayeditor_selection = m_airwayeditortreeview->get_selection();
	Gtk::TreeModel::iterator iter = airwayeditor_selection->get_selected();
	if (!iter)
		return;
	Gtk::TreeRow row = *iter;
	bool tocoord(m_airwayeditormaptocoord->get_active());
	if (tocoord) {
		row[m_airwaylistcolumns.m_col_end_coord] = cursor;
	} else {
		row[m_airwaylistcolumns.m_col_begin_coord] = cursor;
	}
	AirwaysDb::Airway e((*m_db)(row[m_airwaylistcolumns.m_col_id]));
	if (!e.is_valid())
		return;
	e.set_modtime(time(0));
	if (tocoord)
		e.set_end_coord(cursor);
	else
		e.set_begin_coord(cursor);
	save_noerr(e, row);
}

void AirwayEditor::map_drawflags(int flags)
{
	*m_airwayeditormap = (MapRenderer::DrawFlags)flags;
}

void AirwayEditor::save_nostack( AirwaysDb::Airway & e )
{
	if (!e.is_valid())
		return;
	AirwaysDb::Airway::Address addr(e.get_address());
	m_db->save(e);
	m_dbchanged = true;
	m_airwayeditormap->airways_changed();
	Gtk::TreeIter iter = m_airwayliststore->children().begin();
	while (iter) {
		Gtk::TreeRow row = *iter;
		if (addr == row[m_airwaylistcolumns.m_col_id]) {
			set_row(row, e);
			return;
		}
		iter++;
	}
	Gtk::TreeModel::Row row(*(m_airwayliststore->append()));
	set_row(row, e);
}

void AirwayEditor::save( AirwaysDb::Airway & e )
{
	save_nostack(e);
	m_undoredo.save(e);
	update_undoredo_menu();
}

void AirwayEditor::save_noerr(AirwaysDb::Airway & e, Gtk::TreeModel::Row & row)
{
	if (e.get_address() != row[m_airwaylistcolumns.m_col_id])
		throw std::runtime_error("AirportEditor::save: ID mismatch");
	try {
		save(e);
	} catch (const sqlite3x::database_error& dberr) {
		AirwaysDb::Airway eold((*m_db)(row[m_airwaylistcolumns.m_col_id]));
		set_row(row, eold);
	}
}

void AirwayEditor::select( const AirwaysDb::Airway & e )
{
	Glib::RefPtr<Gtk::TreeSelection> airwayeditor_selection = m_airwayeditortreeview->get_selection();
	Gtk::TreeIter iter = m_airwayliststore->children().begin();
	while (iter) {
		Gtk::TreeRow row = *iter;
		if (e.get_address() == row[m_airwaylistcolumns.m_col_id]) {
			airwayeditor_selection->select(iter);
			m_airwayeditortreeview->scroll_to_row(Gtk::TreeModel::Path(iter));
			return;
		}
		iter++;
	}
}

void AirwayEditor::update_undoredo_menu(void)
{
	m_airwayeditorundo->set_sensitive(m_undoredo.can_undo());
	m_airwayeditorredo->set_sensitive(m_undoredo.can_redo());
}

void AirwayEditor::airwayeditor_selection_changed(void)
{
	Glib::RefPtr<Gtk::TreeSelection> airwayeditor_selection = m_airwayeditortreeview->get_selection();
	Gtk::TreeModel::iterator iter = airwayeditor_selection->get_selected();
	{
		std::vector<Gtk::TreeModel::Path> sel(airwayeditor_selection->get_selected_rows());
		std::vector<Gtk::TreeModel::Path>::size_type selcount(sel.size());
		m_airwayeditorelev->set_sensitive(selcount > 0);
	}
	if (iter) {
		Gtk::TreeModel::Row row = *iter;
		//Do something with the row.
		std::cerr << "selected row: " << row[m_airwaylistcolumns.m_col_id] << std::endl;
		m_airwayeditordeleteairway->set_sensitive(true);
		m_airwayeditorswapfromto->set_sensitive(true);
		Point pt(row[m_airwaylistcolumns.m_col_begin_coord]);
		if (m_airwayeditormaptocoord->get_active())
			pt = Point(row[m_airwaylistcolumns.m_col_end_coord]);
		m_airwayeditormap->set_center(pt, 0);
		m_airwayeditormap->set_cursor(pt);
		return;
	}
	std::cerr << "selection cleared" << std::endl;
	m_airwayeditordeleteairway->set_sensitive(false);
	m_airwayeditorswapfromto->set_sensitive(false);
}

void AirwayEditor::aboutdialog_response( int response )
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
		Glib::OptionGroup optgroup("airwayeditor", "Airway Editor Options", "Options controlling the Airway Editor");
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
		osso_context_t* osso_context = osso_initialize("vfrairwayeditor", VERSION, TRUE /* deprecated parameter */, 0 /* Use default Glib main loop context */);
		if (!osso_context) {
			std::cerr << "osso_initialize() failed." << std::endl;
			return OSSO_ERROR;
		}
#endif
#ifdef HAVE_HILDON
		Hildon::init();
#endif
		Glib::set_application_name("VFR Airway Editor");
		Glib::thread_init();
		AirwayEditor editor(dir_main, auxdbmode, dir_aux);

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
