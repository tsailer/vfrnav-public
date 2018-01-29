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

AirspaceEditor::AirspaceModelColumns::AirspaceModelColumns(void)
{
	add(m_col_id);
	add(m_col_labelcoord);
	add(m_col_label_placement);
	add(m_col_sourceid);
	add(m_col_icao);
	add(m_col_name);
	add(m_col_ident);
	add(m_col_commname);
	add(m_col_commfreq0);
	add(m_col_commfreq1);
	add(m_col_classtype);
	add(m_col_width);
	add(m_col_gndelevmin);
	add(m_col_gndelevmax);
	add(m_col_altlwr);
	add(m_col_altlwr_agl);
	add(m_col_altlwr_fl);
	add(m_col_altlwr_sfc);
	add(m_col_altlwr_gnd);
	add(m_col_altlwr_unlim);
	add(m_col_altlwr_notam);
	add(m_col_altlwr_unkn);
	add(m_col_altupr);
	add(m_col_altupr_agl);
	add(m_col_altupr_fl);
	add(m_col_altupr_sfc);
	add(m_col_altupr_gnd);
	add(m_col_altupr_unlim);
	add(m_col_altupr_notam);
	add(m_col_altupr_unkn);
	add(m_col_modtime);
}

AirspaceEditor::AirspaceSegmentModelColumns::AirspaceSegmentModelColumns(void)
{
	add(m_col_segment);
	add(m_col_coord1);
	add(m_col_coord2);
	add(m_col_coord0);
	add(m_col_shape);
	add(m_col_radius1);
	add(m_col_radius2);
}

AirspaceEditor::AirspaceComponentModelColumns::AirspaceComponentModelColumns(void)
{
	add(m_col_component);
	add(m_col_icao);
	add(m_col_classtype);
	add(m_col_operator);
}

AirspaceEditor::AirspacePolygonModelColumns::AirspacePolygonModelColumns(void)
{
	add(m_col_point);
	add(m_col_coord);
}

AirspaceEditor::AsyncElevUpdate::AsyncElevUpdate(Engine& eng, AirspacesDbQueryInterface& db, const AirspacesDb::Address & addr, const sigc::slot<void,AirspacesDb::Airspace&>& save)
	: m_db(db), m_addr(addr), m_save(save), m_refcnt(2), m_asyncref(true)
{
	std::cerr << "AirspaceEditor::AsyncElevUpdate::AsyncElevUpdate: " << (unsigned long long)this << std::endl;
	AirspacesDb::Airspace e(m_db(m_addr));
	if (!e.is_valid())
		return;
	m_dispatch.connect(sigc::mem_fun(*this, &AirspaceEditor::AsyncElevUpdate::callback));
	m_async = eng.async_elevation_minmax(e.get_polygon());
	if (m_async) {
		m_async->connect(sigc::mem_fun(*this, &AirspaceEditor::AsyncElevUpdate::callback_dispatch));
	} else {
		m_asyncref = false;
		unreference();
	}
}

AirspaceEditor::AsyncElevUpdate::~AsyncElevUpdate()
{
	cancel();
	std::cerr << "AirspaceEditor::AsyncElevUpdate::~AsyncElevUpdate: " << (unsigned long long)this << std::endl;
}

void AirspaceEditor::AsyncElevUpdate::reference(void)
{
	++m_refcnt;
}

void AirspaceEditor::AsyncElevUpdate::unreference(void)
{
	if (!(--m_refcnt))
		delete this;
}

void AirspaceEditor::AsyncElevUpdate::cancel(void)
{
	Engine::ElevationMinMaxResult::cancel(m_async);
	if (m_asyncref) {
		m_asyncref = false;
		unreference();
	}
}

bool AirspaceEditor::AsyncElevUpdate::is_done(void)
{
	return !m_async;
}

void AirspaceEditor::AsyncElevUpdate::callback_dispatch(void)
{
	m_dispatch();
}

void AirspaceEditor::AsyncElevUpdate::callback(void)
{
	if (!m_async)
		return;
	if (!m_async->is_done())
		return;
	if (!m_async->is_error()) {
		AirspacesDb::Airspace e(m_db(m_addr));
		if (e.is_valid()) {
			e.set_modtime(time(0));
			TopoDb30::minmax_elev_t elev(m_async->get_result());
			std::cerr << "gnd elev: " << elev.first << " / " << elev.second << std::endl;
			if (elev.first != TopoDb30::nodata)
				e.set_gndelevmin(Point::round<AirspacesDb::Airspace::gndelev_t,float>(elev.first * Point::m_to_ft));
			if (elev.second != TopoDb30::nodata)
				e.set_gndelevmax(Point::round<AirspacesDb::Airspace::gndelev_t,float>(elev.second * Point::m_to_ft));
			if (elev.first != TopoDb30::nodata || elev.second != TopoDb30::nodata)
				m_save(e);
		}
	}
	cancel();
}

AirspaceEditor::MyVectorMapArea::MyVectorMapArea(BaseObjectType *castitem, const Glib::RefPtr<Builder>& refxml)
	: VectorMapArea(castitem, refxml)
{
}

void AirspaceEditor::MyVectorMapArea::set_airspace(const AirspacesDb::Airspace& aspc)
{
	m_aspc = aspc;
	if (get_is_drawable())
		queue_draw();
}

void AirspaceEditor::MyVectorMapArea::clear_airspace(void)
{
	m_aspc = AirspacesDb::Airspace();
	if (get_is_drawable())
		queue_draw();
}

bool AirspaceEditor::MyVectorMapArea::on_draw(const Cairo::RefPtr<Cairo::Context>& cr)
{
	bool ret(VectorMapArea::on_draw(cr));
	if (!m_renderer || !cr || !m_aspc.is_valid())
		return ret;
	cr->save();
	const MultiPolygonHole& poly(m_aspc.get_polygon());
	for (MultiPolygonHole::const_iterator phi(poly.begin()), phe(poly.end()); phi != phe; ++phi) {
		const PolygonHole& ph(*phi);
		{
			const PolygonSimple& p(ph.get_exterior());		
			for (unsigned int i = 0, j = p.size(); i < j; ++i) {
				MapRenderer::ScreenCoord sc(m_renderer->coord_screen(p[i]));
				if (!i)
					cr->move_to(sc.getx(), sc.gety());
				else
					cr->line_to(sc.getx(), sc.gety());
			}
			cr->close_path();
		}
		for (unsigned int i = 0; i < ph.get_nrinterior(); ++i) {
			const PolygonSimple& p(ph[i]);		
			for (unsigned int i = 0, j = p.size(); i < j; ++i) {
				MapRenderer::ScreenCoord sc(m_renderer->coord_screen(p[i]));
				if (!i)
					cr->move_to(sc.getx(), sc.gety());
				else
					cr->line_to(sc.getx(), sc.gety());
			}
			cr->close_path();
		}
	}
	cr->set_source_rgba(0.75, 0.0, 1.0, 0.5);
	cr->fill_preserve();
	cr->set_source_rgb(0.75, 0.0, 1.0);
	cr->set_line_width(3);
	cr->stroke();
	cr->restore();
	return ret;
}

AirspaceEditor::AirspaceEditor(const std::string & dir_main, Engine::auxdb_mode_t auxdbmode, const std::string & dir_aux)
	: m_dbchanged(false), m_engine(dir_main, auxdbmode, dir_aux, false, false), m_airspaceeditorwindow(0), m_airspaceeditortreeview(0), m_airspaceeditorstatusbar(0),
	  m_airspaceeditorfind(0), m_airspaceeditornotebook(0), m_airspaceeditorclassexcept(0), m_airspaceeditorefftime(0),
	  m_airspaceeditornote(0), m_airspaceshapeeditortreeview(0), m_airspacecomponenteditortreeview(0), m_airspacepolyapproxeditortreeview(0),
	  m_airspaceeditormap(0), m_airspaceeditorundo(0), m_airspaceeditorredo(0),
	  m_airspaceeditornewairspace(0), m_airspaceeditorduplicateairspace(0), m_airspaceeditordeleteairspace(0),
 	  m_airspaceeditorplacelabel(0), m_airspaceeditorgndelev(0), m_airspaceeditorpoly(0),
	  m_airspaceeditormapenable(0),  m_airspaceeditorairportdiagramenable(0),
	  m_airspaceeditormapzoomin(0), m_airspaceeditormapzoomout(0),
	  m_airspaceeditorappendshape(0), m_airspaceeditorinsertshape(0), m_airspaceeditordeleteshape(0),
	  m_airspaceeditorappendcomponent(0), m_airspaceeditorinsertcomponent(0), m_airspaceeditordeletecomponent(0),
	  m_airspaceeditorselectall(0), m_airspaceeditorunselectall(0), m_aboutdialog(0),
	  m_curentryid(0)
{
#ifdef HAVE_PQXX
	if (auxdbmode == Engine::auxdb_postgres) {
		m_pgconn = std::unique_ptr<pqxx::connection>(new pqxx::connection(dir_main));
		if (m_pgconn->get_variable("application_name").empty())
			m_pgconn->set_variable("application_name", "airspaceeditor");
		m_db.reset(new AirspacesPGDb(*m_pgconn));
	} else
#endif
	{
		m_db.reset(new AirspacesDb(dir_main));
		std::cerr << "Main database " << (dir_main.empty() ? std::string("(default)") : dir_main) << std::endl;
		Glib::ustring dir_aux1(m_engine.get_aux_dir(auxdbmode, dir_aux));
		AirspacesDb *db(static_cast<AirspacesDb *>(m_db.get()));
		if (!dir_aux1.empty() && db->is_dbfile_exists(dir_aux1))
			db->attach(dir_aux1);
		if (db->is_aux_attached())
			std::cerr << "Auxillary airspaces database " << dir_aux1 << " attached" << std::endl;
	}
	m_refxml = load_glade_file("dbeditor" UIEXT, "airporteditorwindow");
	get_widget_derived(m_refxml, "airspaceeditorwindow", m_airspaceeditorwindow);
	get_widget(m_refxml, "airspaceeditortreeview", m_airspaceeditortreeview);
	get_widget(m_refxml, "airspaceeditorstatusbar", m_airspaceeditorstatusbar);
	get_widget(m_refxml, "airspaceeditorfind", m_airspaceeditorfind);
	get_widget(m_refxml, "airspaceeditornotebook", m_airspaceeditornotebook);
	get_widget(m_refxml, "airspaceeditorclassexcept", m_airspaceeditorclassexcept);
	get_widget(m_refxml, "airspaceeditorefftime", m_airspaceeditorefftime);
	get_widget(m_refxml, "airspaceeditornote", m_airspaceeditornote);
	get_widget(m_refxml, "airspaceshapeeditortreeview", m_airspaceshapeeditortreeview);
	get_widget(m_refxml, "airspacecomponenteditortreeview", m_airspacecomponenteditortreeview);
	get_widget(m_refxml, "airspacepolyapproxeditortreeview", m_airspacepolyapproxeditortreeview);
	get_widget_derived(m_refxml, "airspaceeditormap", m_airspaceeditormap);
	{
#ifdef HAVE_GTKMM2
		Glib::RefPtr<Builder> refxml = load_glade_file_nosearch(m_refxml->get_filename(), "aboutdialog");
#else
		//Glib::RefPtr<Builder> refxml = load_glade_file("dbeditor" UIEXT, "aboutdialog");
		Glib::RefPtr<Builder> refxml = m_refxml;
#endif
		get_widget(refxml, "aboutdialog", m_aboutdialog);
		m_aboutdialog->signal_response().connect(sigc::mem_fun(*this, &AirspaceEditor::aboutdialog_response));
	}
	Gtk::Button *buttonclear(0);
	get_widget(m_refxml, "airspaceeditorclearbutton", buttonclear);
	buttonclear->signal_clicked().connect(sigc::mem_fun(*this, &AirspaceEditor::buttonclear_clicked));
	m_airspaceeditorfind->signal_changed().connect(sigc::mem_fun(*this, &AirspaceEditor::find_changed));
	Gtk::MenuItem *menu_quit(0), *menu_cut(0), *menu_copy(0), *menu_paste(0), *menu_delete(0);
	Gtk::MenuItem *menu_preferences(0), *menu_about(0);
	get_widget(m_refxml, "airspaceeditorquit", menu_quit);
	get_widget(m_refxml, "airspaceeditorundo", m_airspaceeditorundo);
	get_widget(m_refxml, "airspaceeditorredo", m_airspaceeditorredo);
	get_widget(m_refxml, "airspaceeditornewairspace", m_airspaceeditornewairspace);
	get_widget(m_refxml, "airspaceeditorduplicateairspace", m_airspaceeditorduplicateairspace);
	get_widget(m_refxml, "airspaceeditordeleteairspace", m_airspaceeditordeleteairspace);
	get_widget(m_refxml, "airspaceeditorplacelabel", m_airspaceeditorplacelabel);
	get_widget(m_refxml, "airspaceeditorgndelev", m_airspaceeditorgndelev);
	get_widget(m_refxml, "airspaceeditorpoly", m_airspaceeditorpoly);
	get_widget(m_refxml, "airspaceeditormapenable", m_airspaceeditormapenable);
	get_widget(m_refxml, "airspaceeditorairportdiagramenable", m_airspaceeditorairportdiagramenable);
	get_widget(m_refxml, "airspaceeditormapzoomin", m_airspaceeditormapzoomin);
	get_widget(m_refxml, "airspaceeditormapzoomout", m_airspaceeditormapzoomout);
	get_widget(m_refxml, "airspaceeditorappendshape", m_airspaceeditorappendshape);
	get_widget(m_refxml, "airspaceeditorinsertshape", m_airspaceeditorinsertshape);
	get_widget(m_refxml, "airspaceeditordeleteshape", m_airspaceeditordeleteshape);
	get_widget(m_refxml, "airspaceeditorappendcomponent", m_airspaceeditorappendcomponent);
	get_widget(m_refxml, "airspaceeditorinsertcomponent", m_airspaceeditorinsertcomponent);
	get_widget(m_refxml, "airspaceeditordeletecomponent", m_airspaceeditordeletecomponent);
	get_widget(m_refxml, "airspaceeditorcut", menu_cut);
	get_widget(m_refxml, "airspaceeditorcopy", menu_copy);
	get_widget(m_refxml, "airspaceeditorpaste", menu_paste);
	get_widget(m_refxml, "airspaceeditordelete", menu_delete);
	get_widget(m_refxml, "airspaceeditorselectall", m_airspaceeditorselectall);
	get_widget(m_refxml, "airspaceeditorunselectall", m_airspaceeditorunselectall);
	get_widget(m_refxml, "airspaceeditorpreferences", menu_preferences);
	get_widget(m_refxml, "airspaceeditorabout", menu_about);
	hide_notebook_pages();
	menu_quit->signal_activate().connect(sigc::mem_fun(*this, &AirspaceEditor::menu_quit_activate));
	m_airspaceeditorundo->signal_activate().connect(sigc::mem_fun(*this, &AirspaceEditor::menu_undo_activate));
	m_airspaceeditorredo->signal_activate().connect(sigc::mem_fun(*this, &AirspaceEditor::menu_redo_activate));
	m_airspaceeditornewairspace->signal_activate().connect(sigc::mem_fun(*this, &AirspaceEditor::menu_newairspace_activate));
	m_airspaceeditorduplicateairspace->signal_activate().connect(sigc::mem_fun(*this, &AirspaceEditor::menu_duplicateairspace_activate));
	m_airspaceeditordeleteairspace->signal_activate().connect(sigc::mem_fun(*this, &AirspaceEditor::menu_deleteairspace_activate));
	m_airspaceeditorplacelabel->signal_activate().connect(sigc::mem_fun(*this, &AirspaceEditor::menu_placelabel_activate));
	m_airspaceeditorgndelev->signal_activate().connect(sigc::mem_fun(*this, &AirspaceEditor::menu_gndelev_activate));
	m_airspaceeditorpoly->signal_activate().connect(sigc::mem_fun(*this, &AirspaceEditor::menu_poly_activate));
	m_airspaceeditormapenable->signal_toggled().connect(sigc::mem_fun(*this, &AirspaceEditor::menu_mapenable_toggled));
	m_airspaceeditorairportdiagramenable->signal_toggled().connect(sigc::mem_fun(*this, &AirspaceEditor::menu_airportdiagramenable_toggled));
	m_airspaceeditormapzoomin->signal_activate().connect(sigc::mem_fun(*this, &AirspaceEditor::menu_mapzoomin_activate));
	m_airspaceeditormapzoomout->signal_activate().connect(sigc::mem_fun(*this, &AirspaceEditor::menu_mapzoomout_activate));
	if (m_airspaceeditormapenable->get_active()) {
		m_airspaceeditormapzoomin->show();
		m_airspaceeditormapzoomout->show();
	} else {
		m_airspaceeditormapzoomin->hide();
		m_airspaceeditormapzoomout->hide();
	}
	m_airspaceeditorwindow->signal_zoomin().connect(sigc::bind_return(sigc::mem_fun(*this, &AirspaceEditor::menu_mapzoomin_activate), true));
	m_airspaceeditorwindow->signal_zoomout().connect(sigc::bind_return(sigc::mem_fun(*this, &AirspaceEditor::menu_mapzoomout_activate), true));
	menu_cut->signal_activate().connect(sigc::mem_fun(*this, &AirspaceEditor::menu_cut_activate));
	menu_copy->signal_activate().connect(sigc::mem_fun(*this, &AirspaceEditor::menu_copy_activate));
	menu_paste->signal_activate().connect(sigc::mem_fun(*this, &AirspaceEditor::menu_paste_activate));
	menu_delete->signal_activate().connect(sigc::mem_fun(*this, &AirspaceEditor::menu_delete_activate));
	m_airspaceeditorselectall->signal_activate().connect(sigc::mem_fun(*this, &AirspaceEditor::menu_selectall_activate));
	m_airspaceeditorunselectall->signal_activate().connect(sigc::mem_fun(*this, &AirspaceEditor::menu_unselectall_activate));
	m_airspaceeditorappendshape->signal_activate().connect(sigc::mem_fun(*this, &AirspaceEditor::menu_appendshape_activate));
	m_airspaceeditorinsertshape->signal_activate().connect(sigc::mem_fun(*this, &AirspaceEditor::menu_insertshape_activate));
	m_airspaceeditordeleteshape->signal_activate().connect(sigc::mem_fun(*this, &AirspaceEditor::menu_deleteshape_activate));
	m_airspaceeditorappendcomponent->signal_activate().connect(sigc::mem_fun(*this, &AirspaceEditor::menu_appendcomponent_activate));
	m_airspaceeditorinsertcomponent->signal_activate().connect(sigc::mem_fun(*this, &AirspaceEditor::menu_insertcomponent_activate));
	m_airspaceeditordeletecomponent->signal_activate().connect(sigc::mem_fun(*this, &AirspaceEditor::menu_deletecomponent_activate));
	menu_preferences->signal_activate().connect(sigc::mem_fun(*this, &AirspaceEditor::menu_prefs_activate));
	menu_about->signal_activate().connect(sigc::mem_fun(*this, &AirspaceEditor::menu_about_activate));
	update_undoredo_menu();
	// Vector Map
	m_airspaceeditormap->set_engine(m_engine);
	m_airspaceeditormap->set_renderer(VectorMapArea::renderer_vmap);
	m_airspaceeditormap->signal_cursor().connect(sigc::mem_fun(*this, &AirspaceEditor::map_cursor));
	m_airspaceeditormap->set_map_scale(m_engine.get_prefs().mapscale);
	AirspaceEditor::map_drawflags(m_engine.get_prefs().mapflags);
	m_engine.get_prefs().mapflags.signal_change().connect(sigc::mem_fun(*this, &AirspaceEditor::map_drawflags));
	// Airspace List
	m_airspaceliststore = Gtk::ListStore::create(m_airspacelistcolumns);
	m_airspaceeditortreeview->set_model(m_airspaceliststore);
  	Gtk::CellRendererText *icao_renderer = Gtk::manage(new Gtk::CellRendererText());
	Gtk::TreeViewColumn *icao_column = m_airspaceeditortreeview->get_column(m_airspaceeditortreeview->append_column(_("ICAO"), *icao_renderer) - 1);
	if (icao_column) {
		icao_column->add_attribute(*icao_renderer, "text", m_airspacelistcolumns.m_col_icao);
		icao_column->set_sort_column(m_airspacelistcolumns.m_col_icao);
	}
	icao_renderer->signal_edited().connect(sigc::mem_fun(*this, &AirspaceEditor::edited_icao));
	icao_renderer->set_property("editable", true);
  	Gtk::CellRendererText *name_renderer = Gtk::manage(new Gtk::CellRendererText());
	Gtk::TreeViewColumn *name_column = m_airspaceeditortreeview->get_column(m_airspaceeditortreeview->append_column(_("Name"), *name_renderer) - 1);
	if (name_column) {
		name_column->add_attribute(*name_renderer, "text", m_airspacelistcolumns.m_col_name);
		name_column->set_sort_column(m_airspacelistcolumns.m_col_name);
	}
	name_renderer->signal_edited().connect(sigc::mem_fun(*this, &AirspaceEditor::edited_name));
	name_renderer->set_property("editable", true);
  	Gtk::CellRendererText *ident_renderer = Gtk::manage(new Gtk::CellRendererText());
	Gtk::TreeViewColumn *ident_column = m_airspaceeditortreeview->get_column(m_airspaceeditortreeview->append_column(_("Ident"), *ident_renderer) - 1);
	if (ident_column) {
		ident_column->add_attribute(*ident_renderer, "text", m_airspacelistcolumns.m_col_ident);
		ident_column->set_sort_column(m_airspacelistcolumns.m_col_ident);
	}
	ident_renderer->signal_edited().connect(sigc::mem_fun(*this, &AirspaceEditor::edited_ident));
	ident_renderer->set_property("editable", true);
	CoordCellRenderer *labelcoord_renderer = Gtk::manage(new CoordCellRenderer());
	Gtk::TreeViewColumn *labelcoord_column = m_airspaceeditortreeview->get_column(m_airspaceeditortreeview->append_column(_("Label Coordinate"), *labelcoord_renderer) - 1);
	if (labelcoord_column) {
		labelcoord_column->add_attribute(*labelcoord_renderer, labelcoord_renderer->get_property_name(), m_airspacelistcolumns.m_col_labelcoord);
		labelcoord_column->set_sort_column(m_airspacelistcolumns.m_col_labelcoord);
	}
	labelcoord_renderer->set_property("editable", true);
	labelcoord_renderer->signal_edited().connect(sigc::mem_fun(*this, &AirspaceEditor::edited_labelcoord));
	LabelPlacementRenderer *label_placement_renderer = Gtk::manage(new LabelPlacementRenderer());
	Gtk::TreeViewColumn *label_placement_column = m_airspaceeditortreeview->get_column(m_airspaceeditortreeview->append_column(_("Label"), *label_placement_renderer) - 1);
	if (label_placement_column) {
		label_placement_column->add_attribute(*label_placement_renderer, label_placement_renderer->get_property_name(), m_airspacelistcolumns.m_col_label_placement);
		label_placement_column->set_sort_column(m_airspacelistcolumns.m_col_label_placement);
	}
	label_placement_renderer->set_property("editable", true);
	label_placement_renderer->signal_edited().connect(sigc::mem_fun(*this, &AirspaceEditor::edited_label_placement));
	AirspaceClassRenderer *airspaceclass_renderer = Gtk::manage(new AirspaceClassRenderer());
	Gtk::TreeViewColumn *airspaceclass_column = m_airspaceeditortreeview->get_column(m_airspaceeditortreeview->append_column(_("Class"), *airspaceclass_renderer) - 1);
	if (airspaceclass_column) {
		airspaceclass_column->add_attribute(*airspaceclass_renderer, airspaceclass_renderer->get_property_name(), m_airspacelistcolumns.m_col_classtype);
		airspaceclass_column->set_sort_column(m_airspacelistcolumns.m_col_classtype);
	}
	airspaceclass_renderer->set_property("editable", true);
	airspaceclass_renderer->signal_edited().connect(sigc::mem_fun(*this, &AirspaceEditor::edited_airspaceclass));
  	Gtk::CellRendererText *commname_renderer = Gtk::manage(new Gtk::CellRendererText());
	Gtk::TreeViewColumn *commname_column = m_airspaceeditortreeview->get_column(m_airspaceeditortreeview->append_column(_("Comm"), *commname_renderer) - 1);
	if (commname_column) {
		commname_column->add_attribute(*commname_renderer, "text", m_airspacelistcolumns.m_col_commname);
		commname_column->set_sort_column(m_airspacelistcolumns.m_col_commname);
	}
	commname_renderer->signal_edited().connect(sigc::mem_fun(*this, &AirspaceEditor::edited_commname));
	commname_renderer->set_property("editable", true);
	FrequencyCellRenderer *commfreq0_renderer = Gtk::manage(new FrequencyCellRenderer());
	Gtk::TreeViewColumn *commfreq0_column = m_airspaceeditortreeview->get_column(m_airspaceeditortreeview->append_column(_("Freq 0"), *commfreq0_renderer) - 1);
	if (commfreq0_column) {
		commfreq0_column->add_attribute(*commfreq0_renderer, commfreq0_renderer->get_property_name(), m_airspacelistcolumns.m_col_commfreq0);
		commfreq0_column->set_sort_column(m_airspacelistcolumns.m_col_commfreq0);
	}
	commfreq0_renderer->set_property("editable", true);
	commfreq0_renderer->signal_edited().connect(sigc::mem_fun(*this, &AirspaceEditor::edited_commfreq0));
	commfreq0_renderer->set_property("xalign", 1.0);
	FrequencyCellRenderer *commfreq1_renderer = Gtk::manage(new FrequencyCellRenderer());
	Gtk::TreeViewColumn *commfreq1_column = m_airspaceeditortreeview->get_column(m_airspaceeditortreeview->append_column(_("Freq 1"), *commfreq1_renderer) - 1);
	if (commfreq1_column) {
		commfreq1_column->add_attribute(*commfreq1_renderer, commfreq1_renderer->get_property_name(), m_airspacelistcolumns.m_col_commfreq1);
		commfreq1_column->set_sort_column(m_airspacelistcolumns.m_col_commfreq1);
	}
	commfreq1_renderer->set_property("editable", true);
	commfreq1_renderer->signal_edited().connect(sigc::mem_fun(*this, &AirspaceEditor::edited_commfreq1));
	commfreq1_renderer->set_property("xalign", 1.0);
  	Gtk::CellRendererText *width_renderer = Gtk::manage(new Gtk::CellRendererText());
	Gtk::TreeViewColumn *width_column = m_airspaceeditortreeview->get_column(m_airspaceeditortreeview->append_column(_("Width"), *width_renderer) - 1);
	if (width_column) {
	      	width_column->add_attribute(*width_renderer, "text", m_airspacelistcolumns.m_col_width);
		width_column->set_sort_column(m_airspacelistcolumns.m_col_width);
	}
	width_renderer->set_property("xalign", 1.0);
	width_renderer->signal_edited().connect(sigc::mem_fun(*this, &AirspaceEditor::edited_width));
	width_renderer->set_property("editable", true);
  	Gtk::CellRendererText *altlwr_renderer = Gtk::manage(new Gtk::CellRendererText());
	Gtk::TreeViewColumn *altlwr_column = m_airspaceeditortreeview->get_column(m_airspaceeditortreeview->append_column(_("Lower Altitude"), *altlwr_renderer) - 1);
	if (altlwr_column) {
	    	altlwr_column->add_attribute(*altlwr_renderer, "text", m_airspacelistcolumns.m_col_altlwr);
		altlwr_column->set_sort_column(m_airspacelistcolumns.m_col_altlwr);
	}
	altlwr_renderer->set_property("xalign", 1.0);
	altlwr_renderer->signal_edited().connect(sigc::mem_fun(*this, &AirspaceEditor::edited_altlwr));
	altlwr_renderer->set_property("editable", true);
  	Gtk::CellRendererToggle *altlwr_agl_renderer = Gtk::manage(new Gtk::CellRendererToggle());
	Gtk::TreeViewColumn *altlwr_agl_column = m_airspaceeditortreeview->get_column(m_airspaceeditortreeview->append_column(_("AGL"), *altlwr_agl_renderer) - 1);
	if (altlwr_agl_column) {
		altlwr_agl_column->add_attribute(*altlwr_agl_renderer, "active", m_airspacelistcolumns.m_col_altlwr_agl);
		altlwr_agl_column->set_sort_column(m_airspacelistcolumns.m_col_altlwr_agl);
	}
	altlwr_agl_renderer->signal_toggled().connect(sigc::bind(sigc::mem_fun(*this, &AirspaceEditor::edited_altlwr_flag), AirspacesDb::Airspace::altflag_agl));
	altlwr_agl_renderer->set_property("editable", true);
   	Gtk::CellRendererToggle *altlwr_fl_renderer = Gtk::manage(new Gtk::CellRendererToggle());
	Gtk::TreeViewColumn *altlwr_fl_column = m_airspaceeditortreeview->get_column(m_airspaceeditortreeview->append_column(_("FL"), *altlwr_fl_renderer) - 1);
	if (altlwr_fl_column) {
		altlwr_fl_column->add_attribute(*altlwr_fl_renderer, "active", m_airspacelistcolumns.m_col_altlwr_fl);
		altlwr_fl_column->set_sort_column(m_airspacelistcolumns.m_col_altlwr_fl);
	}
	altlwr_fl_renderer->signal_toggled().connect(sigc::bind(sigc::mem_fun(*this, &AirspaceEditor::edited_altlwr_flag), AirspacesDb::Airspace::altflag_fl));
	altlwr_fl_renderer->set_property("editable", true);
  	Gtk::CellRendererToggle *altlwr_sfc_renderer = Gtk::manage(new Gtk::CellRendererToggle());
	Gtk::TreeViewColumn *altlwr_sfc_column = m_airspaceeditortreeview->get_column(m_airspaceeditortreeview->append_column(_("SFC"), *altlwr_sfc_renderer) - 1);
	if (altlwr_sfc_column) {
		altlwr_sfc_column->add_attribute(*altlwr_sfc_renderer, "active", m_airspacelistcolumns.m_col_altlwr_sfc);
		altlwr_sfc_column->set_sort_column(m_airspacelistcolumns.m_col_altlwr_sfc);
	}
	altlwr_sfc_renderer->signal_toggled().connect(sigc::bind(sigc::mem_fun(*this, &AirspaceEditor::edited_altlwr_flag), AirspacesDb::Airspace::altflag_sfc));
	altlwr_sfc_renderer->set_property("editable", true);
  	Gtk::CellRendererToggle *altlwr_gnd_renderer = Gtk::manage(new Gtk::CellRendererToggle());
	Gtk::TreeViewColumn *altlwr_gnd_column = m_airspaceeditortreeview->get_column(m_airspaceeditortreeview->append_column(_("GND"), *altlwr_gnd_renderer) - 1);
	if (altlwr_gnd_column) {
		altlwr_gnd_column->add_attribute(*altlwr_gnd_renderer, "active", m_airspacelistcolumns.m_col_altlwr_gnd);
		altlwr_gnd_column->set_sort_column(m_airspacelistcolumns.m_col_altlwr_gnd);
	}
	altlwr_gnd_renderer->signal_toggled().connect(sigc::bind(sigc::mem_fun(*this, &AirspaceEditor::edited_altlwr_flag), AirspacesDb::Airspace::altflag_gnd));
	altlwr_gnd_renderer->set_property("editable", true);
  	Gtk::CellRendererToggle *altlwr_unlim_renderer = Gtk::manage(new Gtk::CellRendererToggle());
	Gtk::TreeViewColumn *altlwr_unlim_column = m_airspaceeditortreeview->get_column(m_airspaceeditortreeview->append_column(_("Unlim"), *altlwr_unlim_renderer) - 1);
	if (altlwr_unlim_column) {
 		altlwr_unlim_column->add_attribute(*altlwr_unlim_renderer, "active", m_airspacelistcolumns.m_col_altlwr_unlim);
		altlwr_unlim_column->set_sort_column(m_airspacelistcolumns.m_col_altlwr_unlim);
	}
	altlwr_unlim_renderer->signal_toggled().connect(sigc::bind(sigc::mem_fun(*this, &AirspaceEditor::edited_altlwr_flag), AirspacesDb::Airspace::altflag_unlim));
	altlwr_unlim_renderer->set_property("editable", true);
  	Gtk::CellRendererToggle *altlwr_notam_renderer = Gtk::manage(new Gtk::CellRendererToggle());
	Gtk::TreeViewColumn *altlwr_notam_column = m_airspaceeditortreeview->get_column(m_airspaceeditortreeview->append_column(_("NOTAM"), *altlwr_notam_renderer) - 1);
	if (altlwr_notam_column) {
		altlwr_notam_column->add_attribute(*altlwr_notam_renderer, "active", m_airspacelistcolumns.m_col_altlwr_notam);
		altlwr_notam_column->set_sort_column(m_airspacelistcolumns.m_col_altlwr_notam);
	}
	altlwr_notam_renderer->signal_toggled().connect(sigc::bind(sigc::mem_fun(*this, &AirspaceEditor::edited_altlwr_flag), AirspacesDb::Airspace::altflag_notam));
	altlwr_notam_renderer->set_property("editable", true);
  	Gtk::CellRendererToggle *altlwr_unkn_renderer = Gtk::manage(new Gtk::CellRendererToggle());
	Gtk::TreeViewColumn *altlwr_unkn_column = m_airspaceeditortreeview->get_column(m_airspaceeditortreeview->append_column(_("Unknown"), *altlwr_unkn_renderer) - 1);
	if (altlwr_unkn_column) {
		altlwr_unkn_column->add_attribute(*altlwr_unkn_renderer, "active", m_airspacelistcolumns.m_col_altlwr_unkn);
		altlwr_unkn_column->set_sort_column(m_airspacelistcolumns.m_col_altlwr_unkn);
	}
	altlwr_unkn_renderer->signal_toggled().connect(sigc::bind(sigc::mem_fun(*this, &AirspaceEditor::edited_altlwr_flag), AirspacesDb::Airspace::altflag_unkn));
	altlwr_unkn_renderer->set_property("editable", true);
  	Gtk::CellRendererText *altupr_renderer = Gtk::manage(new Gtk::CellRendererText());
	Gtk::TreeViewColumn *altupr_column = m_airspaceeditortreeview->get_column(m_airspaceeditortreeview->append_column(_("Upper Altitude"), *altupr_renderer) - 1);
	if (altupr_column) {
	     	altupr_column->add_attribute(*altupr_renderer, "text", m_airspacelistcolumns.m_col_altupr);
		altupr_column->set_sort_column(m_airspacelistcolumns.m_col_altupr);
	}
	altupr_renderer->set_property("xalign", 1.0);
	altupr_renderer->signal_edited().connect(sigc::mem_fun(*this, &AirspaceEditor::edited_altupr));
	altupr_renderer->set_property("editable", true);
  	Gtk::CellRendererToggle *altupr_agl_renderer = Gtk::manage(new Gtk::CellRendererToggle());
	Gtk::TreeViewColumn *altupr_agl_column = m_airspaceeditortreeview->get_column(m_airspaceeditortreeview->append_column(_("AGL"), *altupr_agl_renderer) - 1);
	if (altupr_agl_column) {
		altupr_agl_column->add_attribute(*altupr_agl_renderer, "active", m_airspacelistcolumns.m_col_altupr_agl);
		altupr_agl_column->set_sort_column(m_airspacelistcolumns.m_col_altupr_agl);
	}
	altupr_agl_renderer->signal_toggled().connect(sigc::bind(sigc::mem_fun(*this, &AirspaceEditor::edited_altupr_flag), AirspacesDb::Airspace::altflag_agl));
	altupr_agl_renderer->set_property("editable", true);
  	Gtk::CellRendererToggle *altupr_fl_renderer = Gtk::manage(new Gtk::CellRendererToggle());
	Gtk::TreeViewColumn *altupr_fl_column = m_airspaceeditortreeview->get_column(m_airspaceeditortreeview->append_column(_("FL"), *altupr_fl_renderer) - 1);
	if (altupr_fl_column) {
 		altupr_fl_column->add_attribute(*altupr_fl_renderer, "active", m_airspacelistcolumns.m_col_altupr_fl);
		altupr_fl_column->set_sort_column(m_airspacelistcolumns.m_col_altupr_fl);
	}
	altupr_fl_renderer->signal_toggled().connect(sigc::bind(sigc::mem_fun(*this, &AirspaceEditor::edited_altupr_flag), AirspacesDb::Airspace::altflag_fl));
	altupr_fl_renderer->set_property("editable", true);
 	Gtk::CellRendererToggle *altupr_sfc_renderer = Gtk::manage(new Gtk::CellRendererToggle());
	Gtk::TreeViewColumn *altupr_sfc_column = m_airspaceeditortreeview->get_column(m_airspaceeditortreeview->append_column(_("SFC"), *altupr_sfc_renderer) - 1);
	if (altupr_sfc_column) {
		altupr_sfc_column->add_attribute(*altupr_sfc_renderer, "active", m_airspacelistcolumns.m_col_altupr_sfc);
		altupr_sfc_column->set_sort_column(m_airspacelistcolumns.m_col_altupr_sfc);
	}
	altupr_sfc_renderer->signal_toggled().connect(sigc::bind(sigc::mem_fun(*this, &AirspaceEditor::edited_altupr_flag), AirspacesDb::Airspace::altflag_sfc));
	altupr_sfc_renderer->set_property("editable", true);
  	Gtk::CellRendererToggle *altupr_gnd_renderer = Gtk::manage(new Gtk::CellRendererToggle());
	Gtk::TreeViewColumn *altupr_gnd_column = m_airspaceeditortreeview->get_column(m_airspaceeditortreeview->append_column(_("GND"), *altupr_gnd_renderer) - 1);
	if (altupr_gnd_column) {
		altupr_gnd_column->add_attribute(*altupr_gnd_renderer, "active", m_airspacelistcolumns.m_col_altupr_gnd);
		altupr_gnd_column->set_sort_column(m_airspacelistcolumns.m_col_altupr_gnd);
	}
	altupr_gnd_renderer->signal_toggled().connect(sigc::bind(sigc::mem_fun(*this, &AirspaceEditor::edited_altupr_flag), AirspacesDb::Airspace::altflag_gnd));
	altupr_gnd_renderer->set_property("editable", true);
  	Gtk::CellRendererToggle *altupr_unlim_renderer = Gtk::manage(new Gtk::CellRendererToggle());
	Gtk::TreeViewColumn *altupr_unlim_column = m_airspaceeditortreeview->get_column(m_airspaceeditortreeview->append_column(_("Unlim"), *altupr_unlim_renderer) - 1);
	if (altupr_unlim_column) {
		altupr_unlim_column->add_attribute(*altupr_unlim_renderer, "active", m_airspacelistcolumns.m_col_altupr_unlim);
		altupr_unlim_column->set_sort_column(m_airspacelistcolumns.m_col_altupr_unlim);
	}
	altupr_unlim_renderer->signal_toggled().connect(sigc::bind(sigc::mem_fun(*this, &AirspaceEditor::edited_altupr_flag), AirspacesDb::Airspace::altflag_unlim));
	altupr_unlim_renderer->set_property("editable", true);
  	Gtk::CellRendererToggle *altupr_notam_renderer = Gtk::manage(new Gtk::CellRendererToggle());
	Gtk::TreeViewColumn *altupr_notam_column = m_airspaceeditortreeview->get_column(m_airspaceeditortreeview->append_column(_("NOTAM"), *altupr_notam_renderer) - 1);
	if (altupr_notam_column) {
		altupr_notam_column->add_attribute(*altupr_notam_renderer, "active", m_airspacelistcolumns.m_col_altupr_notam);
		altupr_notam_column->set_sort_column(m_airspacelistcolumns.m_col_altupr_notam);
	}
	altupr_notam_renderer->signal_toggled().connect(sigc::bind(sigc::mem_fun(*this, &AirspaceEditor::edited_altupr_flag), AirspacesDb::Airspace::altflag_notam));
	altupr_notam_renderer->set_property("editable", true);
  	Gtk::CellRendererToggle *altupr_unkn_renderer = Gtk::manage(new Gtk::CellRendererToggle());
	Gtk::TreeViewColumn *altupr_unkn_column = m_airspaceeditortreeview->get_column(m_airspaceeditortreeview->append_column(_("Unknown"), *altupr_unkn_renderer) - 1);
	if (altupr_unkn_column) {
		altupr_unkn_column->add_attribute(*altupr_unkn_renderer, "active", m_airspacelistcolumns.m_col_altupr_unkn);
		altupr_unkn_column->set_sort_column(m_airspacelistcolumns.m_col_altupr_unkn);
	}
	altupr_unkn_renderer->signal_toggled().connect(sigc::bind(sigc::mem_fun(*this, &AirspaceEditor::edited_altupr_flag), AirspacesDb::Airspace::altflag_unkn));
	altupr_unkn_renderer->set_property("editable", true);
  	Gtk::CellRendererText *gndelevmin_renderer = Gtk::manage(new Gtk::CellRendererText());
	Gtk::TreeViewColumn *gndelevmin_column = m_airspaceeditortreeview->get_column(m_airspaceeditortreeview->append_column(_("GND Elev Min"), *gndelevmin_renderer) - 1);
	if (gndelevmin_column) {
	      	gndelevmin_column->add_attribute(*gndelevmin_renderer, "text", m_airspacelistcolumns.m_col_gndelevmin);
		gndelevmin_column->set_sort_column(m_airspacelistcolumns.m_col_gndelevmin);
	}
	gndelevmin_renderer->set_property("xalign", 1.0);
	gndelevmin_renderer->signal_edited().connect(sigc::mem_fun(*this, &AirspaceEditor::edited_gndelevmin));
	gndelevmin_renderer->set_property("editable", true);
  	Gtk::CellRendererText *gndelevmax_renderer = Gtk::manage(new Gtk::CellRendererText());
	Gtk::TreeViewColumn *gndelevmax_column = m_airspaceeditortreeview->get_column(m_airspaceeditortreeview->append_column(_("GND Elev Max"), *gndelevmax_renderer) - 1);
	if (gndelevmax_column) {
	  	gndelevmax_column->add_attribute(*gndelevmax_renderer, "text", m_airspacelistcolumns.m_col_gndelevmax);
		gndelevmax_column->set_sort_column(m_airspacelistcolumns.m_col_gndelevmax);
	}
	gndelevmax_renderer->set_property("xalign", 1.0);
	gndelevmax_renderer->signal_edited().connect(sigc::mem_fun(*this, &AirspaceEditor::edited_gndelevmax));
	gndelevmax_renderer->set_property("editable", true);
  	Gtk::CellRendererText *srcid_renderer = Gtk::manage(new Gtk::CellRendererText());
	Gtk::TreeViewColumn *srcid_column = m_airspaceeditortreeview->get_column(m_airspaceeditortreeview->append_column(_("Source ID"), *srcid_renderer) - 1);
	if (srcid_column) {
		srcid_column->add_attribute(*srcid_renderer, "text", m_airspacelistcolumns.m_col_sourceid);
		srcid_column->set_sort_column(m_airspacelistcolumns.m_col_sourceid);
	}
	srcid_renderer->signal_edited().connect(sigc::mem_fun(*this, &AirspaceEditor::edited_sourceid));
	srcid_renderer->set_property("editable", true);
	DateTimeCellRenderer *modtime_renderer = Gtk::manage(new DateTimeCellRenderer());
	Gtk::TreeViewColumn *modtime_column = m_airspaceeditortreeview->get_column(m_airspaceeditortreeview->append_column(_("Mod Time"), *modtime_renderer) - 1);
	if (modtime_column) {
		modtime_column->add_attribute(*modtime_renderer, modtime_renderer->get_property_name(), m_airspacelistcolumns.m_col_modtime);
		modtime_column->set_sort_column(m_airspacelistcolumns.m_col_modtime);
	}
	modtime_renderer->set_property("editable", true);
	modtime_renderer->signal_edited().connect(sigc::mem_fun(*this, &AirspaceEditor::edited_modtime));
	//m_airspaceeditortreeview->append_column_editable(_("Mod Time"), m_airspacelistcolumns.m_col_modtime);
	for (unsigned int i = 0; i < 30; ++i) {
		Gtk::TreeViewColumn *col = m_airspaceeditortreeview->get_column(i);
		if (!col)
			continue;
		col->set_resizable(true);
		col->set_reorderable(true);
	}
	m_airspaceeditortreeview->columns_autosize();
	m_airspaceeditortreeview->set_enable_search(true);
	m_airspaceeditortreeview->set_search_column(m_airspacelistcolumns.m_col_name);
	// selection
	Glib::RefPtr<Gtk::TreeSelection> airspaceeditor_selection = m_airspaceeditortreeview->get_selection();
	airspaceeditor_selection->set_mode(Gtk::SELECTION_MULTIPLE);
	airspaceeditor_selection->signal_changed().connect(sigc::mem_fun(*this, &AirspaceEditor::airspaceeditor_selection_changed));
	//airspaceeditor_selection->set_select_function(sigc::mem_fun(*this, &AirspaceEditor::airspaceeditor_select_function));

	// steup notebook pages
	m_airspaceeditorclassexcept->get_buffer()->signal_changed().connect(sigc::mem_fun(*this, &AirspaceEditor::edited_classexcept));
	m_airspaceeditorefftime->get_buffer()->signal_changed().connect(sigc::mem_fun(*this, &AirspaceEditor::edited_efftime));
	m_airspaceeditornote->get_buffer()->signal_changed().connect(sigc::mem_fun(*this, &AirspaceEditor::edited_note));
	m_airspacesegment_model = Gtk::ListStore::create(m_airspacesegment_model_columns);
	m_airspacecomponent_model = Gtk::ListStore::create(m_airspacecomponent_model_columns);
	m_airspacepolygon_model = Gtk::TreeStore::create(m_airspacepolygon_model_columns);
	m_airspaceshapeeditortreeview->set_model(m_airspacesegment_model);
	m_airspacecomponenteditortreeview->set_model(m_airspacecomponent_model);
	m_airspacepolyapproxeditortreeview->set_model(m_airspacepolygon_model);
	// Segment Treeview
  	Gtk::CellRendererText *segmentnr_renderer = Gtk::manage(new Gtk::CellRendererText());
	Gtk::TreeViewColumn *segmentnr_column = m_airspaceshapeeditortreeview->get_column(m_airspaceshapeeditortreeview->append_column(_("Number"), *segmentnr_renderer) - 1);
	if (segmentnr_column) {
		segmentnr_column->add_attribute(*segmentnr_renderer, "text", m_airspacesegment_model_columns.m_col_segment);
		segmentnr_column->set_sort_column(m_airspacesegment_model_columns.m_col_segment);
	}
	AirspaceShapeRenderer *segmentshape_renderer = Gtk::manage(new AirspaceShapeRenderer());
	Gtk::TreeViewColumn *segmentshape_column = m_airspaceshapeeditortreeview->get_column(m_airspaceshapeeditortreeview->append_column(_("Shape"), *segmentshape_renderer) - 1);
	if (segmentshape_column) {
		segmentshape_column->add_attribute(*segmentshape_renderer, segmentshape_renderer->get_property_name(), m_airspacesegment_model_columns.m_col_shape);
		segmentshape_column->set_sort_column(m_airspacesegment_model_columns.m_col_shape);
	}
	segmentshape_renderer->set_property("editable", true);
	segmentshape_renderer->signal_edited().connect(sigc::mem_fun(*this, &AirspaceEditor::edited_shape));
	CoordCellRenderer *coord1_renderer = Gtk::manage(new CoordCellRenderer());
	Gtk::TreeViewColumn *coord1_column = m_airspaceshapeeditortreeview->get_column(m_airspaceshapeeditortreeview->append_column(_("Coordinate 1"), *coord1_renderer) - 1);
	if (coord1_column) {
		coord1_column->add_attribute(*coord1_renderer, coord1_renderer->get_property_name(), m_airspacesegment_model_columns.m_col_coord1);
		coord1_column->set_sort_column(m_airspacesegment_model_columns.m_col_coord1);
	}
	coord1_renderer->set_property("editable", true);
	coord1_renderer->signal_edited().connect(sigc::mem_fun(*this, &AirspaceEditor::edited_coord1));
	CoordCellRenderer *coord2_renderer = Gtk::manage(new CoordCellRenderer());
	Gtk::TreeViewColumn *coord2_column = m_airspaceshapeeditortreeview->get_column(m_airspaceshapeeditortreeview->append_column(_("Coordinate 2"), *coord2_renderer) - 1);
	if (coord2_column) {
		coord2_column->add_attribute(*coord2_renderer, coord2_renderer->get_property_name(), m_airspacesegment_model_columns.m_col_coord2);
		coord2_column->set_sort_column(m_airspacesegment_model_columns.m_col_coord2);
	}
	coord2_renderer->set_property("editable", true);
	coord2_renderer->signal_edited().connect(sigc::mem_fun(*this, &AirspaceEditor::edited_coord2));
	CoordCellRenderer *coord0_renderer = Gtk::manage(new CoordCellRenderer());
	Gtk::TreeViewColumn *coord0_column = m_airspaceshapeeditortreeview->get_column(m_airspaceshapeeditortreeview->append_column(_("Coordinate 0"), *coord0_renderer) - 1);
	if (coord0_column) {
		coord0_column->add_attribute(*coord0_renderer, coord0_renderer->get_property_name(), m_airspacesegment_model_columns.m_col_coord0);
		coord0_column->set_sort_column(m_airspacesegment_model_columns.m_col_coord0);
	}
	coord0_renderer->set_property("editable", true);
	coord0_renderer->signal_edited().connect(sigc::mem_fun(*this, &AirspaceEditor::edited_coord0));
   	Gtk::CellRendererText *radius1_renderer = Gtk::manage(new Gtk::CellRendererText());
	Gtk::TreeViewColumn *radius1_column = m_airspaceshapeeditortreeview->get_column(m_airspaceshapeeditortreeview->append_column(_("Radius 1"), *radius1_renderer) - 1);
	if (radius1_column) {
		radius1_column->add_attribute(*radius1_renderer, "text", m_airspacesegment_model_columns.m_col_radius1);
		radius1_column->set_sort_column(m_airspacesegment_model_columns.m_col_radius1);
	}
	radius1_renderer->signal_edited().connect(sigc::mem_fun(*this, &AirspaceEditor::edited_radius1));
	radius1_renderer->set_property("editable", true);
  	Gtk::CellRendererText *radius2_renderer = Gtk::manage(new Gtk::CellRendererText());
	Gtk::TreeViewColumn *radius2_column = m_airspaceshapeeditortreeview->get_column(m_airspaceshapeeditortreeview->append_column(_("Radius 2"), *radius2_renderer) - 1);
	if (radius2_column) {
		radius2_column->add_attribute(*radius2_renderer, "text", m_airspacesegment_model_columns.m_col_radius2);
		radius2_column->set_sort_column(m_airspacesegment_model_columns.m_col_radius2);
	}
	radius2_renderer->signal_edited().connect(sigc::mem_fun(*this, &AirspaceEditor::edited_radius2));
	radius2_renderer->set_property("editable", true);
	// Component Treeview
  	Gtk::CellRendererText *componentnr_renderer = Gtk::manage(new Gtk::CellRendererText());
	Gtk::TreeViewColumn *componentnr_column = m_airspacecomponenteditortreeview->get_column(m_airspacecomponenteditortreeview->append_column(_("Number"), *componentnr_renderer) - 1);
	if (componentnr_column) {
		componentnr_column->add_attribute(*componentnr_renderer, "text", m_airspacecomponent_model_columns.m_col_component);
		componentnr_column->set_sort_column(m_airspacecomponent_model_columns.m_col_component);
	}
   	Gtk::CellRendererText *compicao_renderer = Gtk::manage(new Gtk::CellRendererText());
	Gtk::TreeViewColumn *compicao_column = m_airspacecomponenteditortreeview->get_column(m_airspacecomponenteditortreeview->append_column(_("ICAO"), *compicao_renderer) - 1);
	if (compicao_column) {
		compicao_column->add_attribute(*compicao_renderer, "text", m_airspacecomponent_model_columns.m_col_icao);
		compicao_column->set_sort_column(m_airspacecomponent_model_columns.m_col_icao);
	}
	compicao_renderer->signal_edited().connect(sigc::mem_fun(*this, &AirspaceEditor::edited_compicao));
	compicao_renderer->set_property("editable", true);
	AirspaceClassRenderer *compclass_renderer = Gtk::manage(new AirspaceClassRenderer());
	Gtk::TreeViewColumn *compclass_column = m_airspacecomponenteditortreeview->get_column(m_airspacecomponenteditortreeview->append_column(_("Class"), *compclass_renderer) - 1);
	if (compclass_column) {
		compclass_column->add_attribute(*compclass_renderer, compclass_renderer->get_property_name(), m_airspacecomponent_model_columns.m_col_classtype);
		compclass_column->set_sort_column(m_airspacecomponent_model_columns.m_col_classtype);
	}
	compclass_renderer->set_property("editable", true);
	compclass_renderer->signal_edited().connect(sigc::mem_fun(*this, &AirspaceEditor::edited_compclass));
	AirspaceOperatorRenderer *compoperator_renderer = Gtk::manage(new AirspaceOperatorRenderer());
	Gtk::TreeViewColumn *compoperator_column = m_airspacecomponenteditortreeview->get_column(m_airspacecomponenteditortreeview->append_column(_("Shape"), *compoperator_renderer) - 1);
	if (compoperator_column) {
		compoperator_column->add_attribute(*compoperator_renderer, compoperator_renderer->get_property_name(), m_airspacecomponent_model_columns.m_col_operator);
		compoperator_column->set_sort_column(m_airspacecomponent_model_columns.m_col_operator);
	}
	compoperator_renderer->set_property("editable", true);
	compoperator_renderer->signal_edited().connect(sigc::mem_fun(*this, &AirspaceEditor::edited_compoperator));
	// Polygon Treeview
  	Gtk::CellRendererText *polynr_renderer = Gtk::manage(new Gtk::CellRendererText());
	Gtk::TreeViewColumn *polynr_column = m_airspacepolyapproxeditortreeview->get_column(m_airspacepolyapproxeditortreeview->append_column(_("Number"), *polynr_renderer) - 1);
	if (polynr_column) {
	     	polynr_column->add_attribute(*polynr_renderer, "text", m_airspacepolygon_model_columns.m_col_point);
		polynr_column->set_sort_column(m_airspacepolygon_model_columns.m_col_point);
	}
	CoordCellRenderer *coord_renderer = Gtk::manage(new CoordCellRenderer());
	Gtk::TreeViewColumn *coord_column = m_airspacepolyapproxeditortreeview->get_column(m_airspacepolyapproxeditortreeview->append_column(_("Coordinate"), *coord_renderer) - 1);
	if (coord_column) {
		coord_column->add_attribute(*coord_renderer, coord_renderer->get_property_name(), m_airspacepolygon_model_columns.m_col_coord);
		coord_column->set_sort_column(m_airspacepolygon_model_columns.m_col_coord);
	}
	//coord_renderer->set_property("editable", false);
	//coord_renderer->signal_edited().connect(sigc::mem_fun(*this, &AirspaceEditor::edited_lon));
	// selection
	Glib::RefPtr<Gtk::TreeSelection> shape_selection = m_airspaceshapeeditortreeview->get_selection();
	shape_selection->set_mode(Gtk::SELECTION_MULTIPLE);
	shape_selection->signal_changed().connect(sigc::mem_fun(*this, &AirspaceEditor::airspaceeditor_shape_selection_changed));
	Glib::RefPtr<Gtk::TreeSelection> component_selection = m_airspacecomponenteditortreeview->get_selection();
	component_selection->set_mode(Gtk::SELECTION_MULTIPLE);
	component_selection->signal_changed().connect(sigc::mem_fun(*this, &AirspaceEditor::airspaceeditor_component_selection_changed));
	Glib::RefPtr<Gtk::TreeSelection> polygon_selection = m_airspacepolyapproxeditortreeview->get_selection();
	polygon_selection->set_mode(Gtk::SELECTION_NONE);
	polygon_selection->signal_changed().connect(sigc::mem_fun(*this, &AirspaceEditor::airspaceeditor_polygon_selection_changed));
	// notebook
	m_airspaceeditornotebook->signal_switch_page().connect(sigc::hide<0>(sigc::mem_fun(*this, &AirspaceEditor::notebook_switch_page)));
	// final setup
	airspaceeditor_selection_changed();
	//set_airspacelist("");
	m_dbselectdialog.signal_byname().connect(sigc::mem_fun(*this, &AirspaceEditor::dbselect_byname));
	m_dbselectdialog.signal_byrectangle().connect(sigc::mem_fun(*this, &AirspaceEditor::dbselect_byrect));
	m_dbselectdialog.signal_bytime().connect(sigc::mem_fun(*this, &AirspaceEditor::dbselect_bytime));
	m_dbselectdialog.show();
	m_prefswindow = PrefsWindow::create(m_engine.get_prefs());
	m_airspaceeditorwindow->show();
}

AirspaceEditor::~AirspaceEditor()
{
	if (m_dbchanged) {
		std::cerr << "Analyzing database..." << std::endl;
		m_db->analyze();
		m_db->vacuum();
	}
	m_db->close();
}

void AirspaceEditor::dbselect_byname(const Glib::ustring& s, AirspacesDb::comp_t comp)
{
	AirspacesDb::elementvector_t els(m_db->find_by_text(s, 0, comp, 0));
	m_airspaceliststore->clear();
	for (AirspacesDb::elementvector_t::iterator ei(els.begin()), ee(els.end()); ei != ee; ++ei) {
		AirspacesDb::element_t& e(*ei);
		if (!e.is_valid())
			continue;
		Gtk::TreeModel::Row row(*(m_airspaceliststore->append()));
		set_row(row, e);
	}
}

void AirspaceEditor::dbselect_byrect(const Rect& r)
{
	AirspacesDb::elementvector_t els(m_db->find_by_rect(r, 0));
	m_airspaceliststore->clear();
	for (AirspacesDb::elementvector_t::iterator ei(els.begin()), ee(els.end()); ei != ee; ++ei) {
		AirspacesDb::element_t& e(*ei);
		if (!e.is_valid())
			continue;
		Gtk::TreeModel::Row row(*(m_airspaceliststore->append()));
		set_row(row, e);
	}
}

void AirspaceEditor::dbselect_bytime(time_t timefrom, time_t timeto)
{
	AirspacesDb::elementvector_t els(m_db->find_by_time(timefrom, timeto, 0));
	m_airspaceliststore->clear();
	for (AirspacesDb::elementvector_t::iterator ei(els.begin()), ee(els.end()); ei != ee; ++ei) {
		AirspacesDb::element_t& e(*ei);
		if (!e.is_valid())
			continue;
		Gtk::TreeModel::Row row(*(m_airspaceliststore->append()));
		set_row(row, e);
	}
}

void AirspaceEditor::buttonclear_clicked(void)
{
	m_airspaceeditorfind->set_text("");
}

void AirspaceEditor::find_changed(void)
{
	dbselect_byname(m_airspaceeditorfind->get_text());
}

void AirspaceEditor::menu_quit_activate(void)
{
	m_airspaceeditorwindow->hide();
}

void AirspaceEditor::menu_undo_activate(void)
{
	{
		Glib::RefPtr<Gtk::TreeSelection> selection = m_airspaceshapeeditortreeview->get_selection();
		selection->unselect_all();
	}
	{
		Glib::RefPtr<Gtk::TreeSelection> selection = m_airspacecomponenteditortreeview->get_selection();
		selection->unselect_all();
	}
	AirspacesDb::Airspace e(m_undoredo.undo());
	if (e.is_valid()) {
		std::cerr << "undo: " << e.get_nrsegments() << std::endl;
		save_nostack(e);
		select(e);
	}
	update_undoredo_menu();
}

void AirspaceEditor::menu_redo_activate(void)
{
	{
		Glib::RefPtr<Gtk::TreeSelection> selection = m_airspaceshapeeditortreeview->get_selection();
		selection->unselect_all();
	}
	{
		Glib::RefPtr<Gtk::TreeSelection> selection = m_airspacecomponenteditortreeview->get_selection();
		selection->unselect_all();
	}
	AirspacesDb::Airspace e(m_undoredo.redo());
	if (e.is_valid()) {
		save_nostack(e);
		select(e);
	}
	update_undoredo_menu();
}

void AirspaceEditor::menu_newairspace_activate(void)
{
	AirspacesDb::Airspace e;
	e.set_bdryclass('A');
	e.set_typecode('A');
	e.set_sourceid(make_sourceid());
	e.set_modtime(time(0));
	save(e);
	select(e);
}

void AirspaceEditor::menu_duplicateairspace_activate(void)
{
	Glib::RefPtr<Gtk::TreeSelection> airspaceeditor_selection = m_airspaceeditortreeview->get_selection();
	std::vector<Gtk::TreeModel::Path> sel(airspaceeditor_selection->get_selected_rows());
	if (sel.size() != 1)
		return;
	Gtk::TreeModel::iterator iter(m_airspaceliststore->get_iter(*sel.begin()));
	if (!iter) {
		std::cerr << "cannot get iterator for path " << (*sel.begin()).to_string() << " ?""?" << std::endl;
		return;
	}
	Gtk::TreeModel::Row row = *iter;
	std::cerr << "duplicating row: " << row[m_airspacelistcolumns.m_col_id] << std::endl;
	AirspacesDb::Airspace e((*m_db)(row[m_airspacelistcolumns.m_col_id]));
	if (!e.is_valid())
		return;
	AirspacesDb::Airspace e2;
	e2.set_labelcoord(e.get_labelcoord());
	e2.set_icao(e.get_icao());
	e2.set_name(e.get_name());
	e2.set_ident(e.get_ident());
	e2.set_classexcept(e.get_classexcept());
	e2.set_efftime(e.get_efftime());
	e2.set_note(e.get_note());
	e2.set_commname(e.get_commname());
	e2.set_commfreq(0, e.get_commfreq(0));
	e2.set_commfreq(1, e.get_commfreq(1));
	e2.set_bdryclass(e.get_bdryclass());
	e2.set_typecode(e.get_typecode());
	e2.set_gndelevmin(e.get_gndelevmin());
	e2.set_gndelevmax(e.get_gndelevmax());
	e2.set_altlwr(e.get_altlwr());
	e2.set_altupr(e.get_altupr());
	e2.set_altlwrflags(e.get_altlwrflags());
	e2.set_altuprflags(e.get_altuprflags());
	e2.set_label_placement(e.get_label_placement());
	for (unsigned int i = 0; i < e.get_nrsegments(); i++)
		e2.add_segment(e[i]);
	for (unsigned int i = 0; i < e.get_nrcomponents(); i++)
		e2.add_component(e.get_component(i));
	e2.recompute_lineapprox(*m_db);
	e2.get_polygon() = e.get_polygon();
	e2.recompute_bbox();
	e2.set_sourceid(make_sourceid());
	e2.set_modtime(time(0));
	save(e2);
	select(e2);
}

void AirspaceEditor::menu_deleteairspace_activate(void)
{
	Glib::RefPtr<Gtk::TreeSelection> airspaceeditor_selection = m_airspaceeditortreeview->get_selection();
	std::vector<Gtk::TreeModel::Path> sel(airspaceeditor_selection->get_selected_rows());
	if (sel.empty())
		return;
	if (sel.size() > 1) {
		Gtk::MessageDialog *dlg = new Gtk::MessageDialog(_("Delete multiple airspaces?"), false, Gtk::MESSAGE_QUESTION, Gtk::BUTTONS_YES_NO, true);
		int res = dlg->run();
		delete dlg;
		if (res != Gtk::RESPONSE_YES) {
			if (res != Gtk::RESPONSE_NO)
				std::cerr << "unexpected dialog result: " << res << std::endl;
			return;
		}
	}
	for (std::vector<Gtk::TreeModel::Path>::iterator si(sel.begin()), se(sel.end()); si != se; si++) {
		Gtk::TreeModel::iterator iter(m_airspaceliststore->get_iter(*si));
		if (!iter) {
			std::cerr << "cannot get iterator for path " << (*si).to_string() << " ?""?" << std::endl;
			continue;
		}
		Gtk::TreeModel::Row row = *iter;
		std::cerr << "deleting row: " << row[m_airspacelistcolumns.m_col_id] << std::endl;
		AirspacesDb::Airspace e((*m_db)(row[m_airspacelistcolumns.m_col_id]));
		m_undoredo.erase(e);
		m_db->erase(e);
		m_airspaceliststore->erase(iter);
	}
}

void AirspaceEditor::menu_placelabel_activate(void)
{
	Glib::RefPtr<Gtk::TreeSelection> airspaceeditor_selection = m_airspaceeditortreeview->get_selection();
	std::vector<Gtk::TreeModel::Path> sel(airspaceeditor_selection->get_selected_rows());
	if (sel.empty())
		return;
	if (sel.size() > 1) {
		Gtk::MessageDialog *dlg = new Gtk::MessageDialog(_("Place labels for multiple airspaces?"), false, Gtk::MESSAGE_QUESTION, Gtk::BUTTONS_YES_NO, true);
		int res = dlg->run();
		delete dlg;
		if (res != Gtk::RESPONSE_YES) {
			if (res != Gtk::RESPONSE_NO)
				std::cerr << "unexpected dialog result: " << res << std::endl;
			return;
		}
	}
	for (std::vector<Gtk::TreeModel::Path>::iterator si(sel.begin()), se(sel.end()); si != se; si++) {
		Gtk::TreeModel::iterator iter(m_airspaceliststore->get_iter(*si));
		if (!iter) {
			std::cerr << "cannot get iterator for path " << (*si).to_string() << " ?""?" << std::endl;
			continue;
		}
		Gtk::TreeModel::Row row = *iter;
		std::cerr << "placing label in row: " << row[m_airspacelistcolumns.m_col_id] << std::endl;
		AirspacesDb::Airspace e((*m_db)(row[m_airspacelistcolumns.m_col_id]));
		if (!e.is_valid())
			continue;
		e.set_modtime(time(0));
		e.compute_initial_label_placement();
		save(e);
	}
}

void AirspaceEditor::menu_gndelev_activate(void)
{
	Glib::RefPtr<Gtk::TreeSelection> airspaceeditor_selection = m_airspaceeditortreeview->get_selection();
	std::vector<Gtk::TreeModel::Path> sel(airspaceeditor_selection->get_selected_rows());
	if (sel.empty())
		return;
	if (sel.size() > 1) {
		Gtk::MessageDialog *dlg = new Gtk::MessageDialog(_("Recompute ground elevation for multiple airspaces?"), false, Gtk::MESSAGE_QUESTION, Gtk::BUTTONS_YES_NO, true);
		int res = dlg->run();
		delete dlg;
		if (res != Gtk::RESPONSE_YES) {
			if (res != Gtk::RESPONSE_NO)
				std::cerr << "unexpected dialog result: " << res << std::endl;
			return;
		}
	}
	for (std::vector<Gtk::TreeModel::Path>::iterator si(sel.begin()), se(sel.end()); si != se; si++) {
		Gtk::TreeModel::iterator iter(m_airspaceliststore->get_iter(*si));
		if (!iter) {
			std::cerr << "cannot get iterator for path " << (*si).to_string() << " ?""?" << std::endl;
			continue;
		}
		Gtk::TreeModel::Row row = *iter;
		std::cerr << "recomputing ground elevation for row: " << row[m_airspacelistcolumns.m_col_id] << std::endl;
		Glib::RefPtr<AsyncElevUpdate> aeu(new AsyncElevUpdate(m_engine, *m_db.get(), row[m_airspacelistcolumns.m_col_id], sigc::mem_fun(*this, &AirspaceEditor::save)));
	}
}

void AirspaceEditor::menu_poly_activate(void)
{
	Glib::RefPtr<Gtk::TreeSelection> airspaceeditor_selection = m_airspaceeditortreeview->get_selection();
	std::vector<Gtk::TreeModel::Path> sel(airspaceeditor_selection->get_selected_rows());
	if (sel.empty())
		return;
	if (sel.size() > 1) {
		Gtk::MessageDialog *dlg = new Gtk::MessageDialog(_("Recompute polygons for multiple airspaces?"), false, Gtk::MESSAGE_QUESTION, Gtk::BUTTONS_YES_NO, true);
		int res = dlg->run();
		delete dlg;
		if (res != Gtk::RESPONSE_YES) {
			if (res != Gtk::RESPONSE_NO)
				std::cerr << "unexpected dialog result: " << res << std::endl;
			return;
		}
	}
	for (std::vector<Gtk::TreeModel::Path>::iterator si(sel.begin()), se(sel.end()); si != se; si++) {
		Gtk::TreeModel::iterator iter(m_airspaceliststore->get_iter(*si));
		if (!iter) {
			std::cerr << "cannot get iterator for path " << (*si).to_string() << " ?""?" << std::endl;
			continue;
		}
		Gtk::TreeModel::Row row = *iter;
		std::cerr << "recomputing polygon for row: " << row[m_airspacelistcolumns.m_col_id] << std::endl;
		AirspacesDb::Airspace e((*m_db)(row[m_airspacelistcolumns.m_col_id]));
		if (!e.is_valid())
			continue;
		e.set_modtime(time(0));
		e.recompute_lineapprox(*m_db);
		save(e);
	}
}
 
void AirspaceEditor::menu_selectall_activate(void)
{
	switch (m_airspaceeditornotebook->get_current_page()) {
		case 0:
		{
			Glib::RefPtr<Gtk::TreeSelection> selection = m_airspaceeditortreeview->get_selection();
			selection->select_all();
			break;
		}

		case 2:
		{
			Glib::RefPtr<Gtk::TreeSelection> selection = m_airspaceshapeeditortreeview->get_selection();
			selection->select_all();
			break;
		}

		case 3:
		{
			Glib::RefPtr<Gtk::TreeSelection> selection = m_airspacecomponenteditortreeview->get_selection();
			selection->select_all();
			break;
		}
	}
}

void AirspaceEditor::menu_unselectall_activate(void)
{
	switch (m_airspaceeditornotebook->get_current_page()) {
		case 0:
		{
			Glib::RefPtr<Gtk::TreeSelection> selection = m_airspaceeditortreeview->get_selection();
			selection->unselect_all();
			break;
		}

		case 2:
		{
			Glib::RefPtr<Gtk::TreeSelection> selection = m_airspaceshapeeditortreeview->get_selection();
			selection->unselect_all();
			break;
		}

		case 3:
		{
			Glib::RefPtr<Gtk::TreeSelection> selection = m_airspacecomponenteditortreeview->get_selection();
			selection->unselect_all();
			break;
		}
	}
}

void AirspaceEditor::menu_cut_activate(void)
{
	Gtk::Widget *focus = m_airspaceeditorwindow->get_focus();
	if (!focus)
		return;
	Gtk::Editable *edit = dynamic_cast<Gtk::Editable *>(focus);
	if (!edit)
		return;
	edit->cut_clipboard();
}

void AirspaceEditor::menu_copy_activate(void)
{
	Gtk::Widget *focus = m_airspaceeditorwindow->get_focus();
	if (!focus)
		return;
	Gtk::Editable *edit = dynamic_cast<Gtk::Editable *>(focus);
	if (!edit)
		return;
	edit->copy_clipboard();
}

void AirspaceEditor::menu_paste_activate(void)
{
	Gtk::Widget *focus = m_airspaceeditorwindow->get_focus();
	if (!focus)
		return;
	Gtk::Editable *edit = dynamic_cast<Gtk::Editable *>(focus);
	if (!edit)
		return;
	edit->paste_clipboard();
}

void AirspaceEditor::menu_delete_activate(void)
{
	Gtk::Widget *focus = m_airspaceeditorwindow->get_focus();
	if (!focus)
		return;
	Gtk::Editable *edit = dynamic_cast<Gtk::Editable *>(focus);
	if (!edit)
		return;
	edit->delete_selection();
}

void AirspaceEditor::menu_mapenable_toggled(void)
{
	if (m_airspaceeditormapenable->get_active()) {
		m_airspaceeditorairportdiagramenable->set_active(false);
		m_airspaceeditormap->set_renderer(VectorMapArea::renderer_vmap);
		m_airspaceeditormap->set_map_scale(m_engine.get_prefs().mapscale);
		m_airspaceeditormap->show();
		m_airspaceeditormapzoomin->show();
		m_airspaceeditormapzoomout->show();
	} else {
		m_airspaceeditormap->hide();
		m_airspaceeditormapzoomin->hide();
		m_airspaceeditormapzoomout->hide();
	}
}

void AirspaceEditor::menu_airportdiagramenable_toggled(void)
{
	if (m_airspaceeditorairportdiagramenable->get_active()) {
		m_airspaceeditormapenable->set_active(false);
		m_airspaceeditormap->set_renderer(VectorMapArea::renderer_airportdiagram);
		m_airspaceeditormap->set_map_scale(m_engine.get_prefs().mapscaleairportdiagram);
		m_airspaceeditormap->show();
		m_airspaceeditormapzoomin->show();
		m_airspaceeditormapzoomout->show();
	} else {
		m_airspaceeditormap->hide();
		m_airspaceeditormapzoomin->hide();
		m_airspaceeditormapzoomout->hide();
	}
}

void AirspaceEditor::menu_mapzoomin_activate(void)
{
	m_airspaceeditormap->zoom_in();
	if (m_airspaceeditormap->get_renderer() == VectorMapArea::renderer_airportdiagram)
		m_engine.get_prefs().mapscaleairportdiagram = m_airspaceeditormap->get_map_scale();
	else
		m_engine.get_prefs().mapscale = m_airspaceeditormap->get_map_scale();
}

void AirspaceEditor::menu_mapzoomout_activate(void)
{
	m_airspaceeditormap->zoom_out();
	if (m_airspaceeditormap->get_renderer() == VectorMapArea::renderer_airportdiagram)
		m_engine.get_prefs().mapscaleairportdiagram = m_airspaceeditormap->get_map_scale();
	else
		m_engine.get_prefs().mapscale = m_airspaceeditormap->get_map_scale();
}

void AirspaceEditor::renumber_shapes(void)
{
	unsigned int seg = 0;
	for (Gtk::TreeModel::iterator si(m_airspacesegment_model->children().begin()); si; ++si, ++seg) {
		Gtk::TreeRow row(*si);
		row[m_airspacesegment_model_columns.m_col_segment] = seg;
	}
}

void AirspaceEditor::renumber_components(void)
{
	unsigned int comp = 0;
	for (Gtk::TreeModel::iterator si(m_airspacecomponent_model->children().begin()); si; ++si, ++comp) {
		Gtk::TreeRow row(*si);
		row[m_airspacecomponent_model_columns.m_col_component] = comp;
	}
}

void AirspaceEditor::new_shape(Gtk::TreeModel::iterator iter)
{
	Gtk::TreeModel::iterator iterp(iter), itern(iter);
	itern++;
	if (itern == m_airspacesegment_model->children().end())
		itern = m_airspacesegment_model->children().begin();
	if (iterp == m_airspacesegment_model->children().begin())
		iterp = m_airspacesegment_model->children().end();
	iterp--;
	Gtk::TreeRow row(*iter), rowp(*iterp), rown(*itern);
	row[m_airspacesegment_model_columns.m_col_shape] = '-';
	Point pt1(rowp[m_airspacesegment_model_columns.m_col_coord2]);
	Point pt2(rown[m_airspacesegment_model_columns.m_col_coord1]);
	row[m_airspacesegment_model_columns.m_col_coord1] = pt1;
	row[m_airspacesegment_model_columns.m_col_coord2] = pt2;
	row[m_airspacesegment_model_columns.m_col_coord0] = Point();
	row[m_airspacesegment_model_columns.m_col_radius1] = 0;
	row[m_airspacesegment_model_columns.m_col_radius2] = 0;
	renumber_shapes();
}

void AirspaceEditor::new_component(Gtk::TreeModel::iterator iter)
{
	Gtk::TreeRow row(*iter);
	row[m_airspacecomponent_model_columns.m_col_classtype] = AirspaceClassType(AirspacesDb::Airspace::bdryclass_ead_tma, AirspacesDb::Airspace::typecode_ead);
	row[m_airspacecomponent_model_columns.m_col_operator] = AirspacesDb::Airspace::Component::operator_set;
	renumber_components();
}

void AirspaceEditor::menu_appendshape_activate(void)
{
	new_shape(m_airspacesegment_model->append());
	save_shapes();
}

void AirspaceEditor::menu_insertshape_activate(void)
{
	Glib::RefPtr<Gtk::TreeSelection> airspaceeditor_selection = m_airspaceshapeeditortreeview->get_selection();
	std::vector<Gtk::TreeModel::Path> sel(airspaceeditor_selection->get_selected_rows());
	std::vector<Gtk::TreeModel::Path>::size_type selcount(sel.size());
	if (selcount != 1) {
		menu_appendshape_activate();
		return;
	}
	new_shape(m_airspacesegment_model->insert(m_airspacesegment_model->get_iter(*sel.begin())));
	save_shapes();
}

void AirspaceEditor::menu_deleteshape_activate(void)
{
	Glib::RefPtr<Gtk::TreeSelection> airspaceeditor_selection = m_airspaceshapeeditortreeview->get_selection();
	std::vector<Gtk::TreeModel::Path> sel(airspaceeditor_selection->get_selected_rows());
	for (std::vector<Gtk::TreeModel::Path>::iterator si(sel.begin()), se(sel.end()); si != se; si++)
		m_airspacesegment_model->erase(m_airspacesegment_model->get_iter(*si));
	renumber_shapes();
	save_shapes();
}

void AirspaceEditor::menu_appendcomponent_activate(void)
{
	new_component(m_airspacecomponent_model->append());
	save_components();
}

void AirspaceEditor::menu_insertcomponent_activate(void)
{
	Glib::RefPtr<Gtk::TreeSelection> airspaceeditor_selection = m_airspacecomponenteditortreeview->get_selection();
	std::vector<Gtk::TreeModel::Path> sel(airspaceeditor_selection->get_selected_rows());
	std::vector<Gtk::TreeModel::Path>::size_type selcount(sel.size());
	if (selcount != 1) {
		menu_appendcomponent_activate();
		return;
	}
	new_component(m_airspacecomponent_model->insert(m_airspacecomponent_model->get_iter(*sel.begin())));
	save_components();
}

void AirspaceEditor::menu_deletecomponent_activate(void)
{
	Glib::RefPtr<Gtk::TreeSelection> airspaceeditor_selection = m_airspacecomponenteditortreeview->get_selection();
	std::vector<Gtk::TreeModel::Path> sel(airspaceeditor_selection->get_selected_rows());
	for (std::vector<Gtk::TreeModel::Path>::iterator si(sel.begin()), se(sel.end()); si != se; si++)
		m_airspacecomponent_model->erase(m_airspacecomponent_model->get_iter(*si));
	renumber_components();
	save_components();
}

void AirspaceEditor::menu_prefs_activate(void)
{
	if (m_prefswindow)
		m_prefswindow->show();
}

void AirspaceEditor::menu_about_activate(void)
{
	m_aboutdialog->show();
}

void AirspaceEditor::set_row_altlwr_flags( Gtk::TreeModel::Row & row, const AirspacesDb::Airspace & e )
{
	row[m_airspacelistcolumns.m_col_altlwr_agl] = !!(e.get_altlwrflags() & AirspacesDb::Airspace::altflag_agl);
	row[m_airspacelistcolumns.m_col_altlwr_fl] = !!(e.get_altlwrflags() & AirspacesDb::Airspace::altflag_fl);
	row[m_airspacelistcolumns.m_col_altlwr_sfc] = !!(e.get_altlwrflags() & AirspacesDb::Airspace::altflag_sfc);
	row[m_airspacelistcolumns.m_col_altlwr_gnd] = !!(e.get_altlwrflags() & AirspacesDb::Airspace::altflag_gnd);
	row[m_airspacelistcolumns.m_col_altlwr_unlim] = !!(e.get_altlwrflags() & AirspacesDb::Airspace::altflag_unlim);
	row[m_airspacelistcolumns.m_col_altlwr_notam] = !!(e.get_altlwrflags() & AirspacesDb::Airspace::altflag_notam);
	row[m_airspacelistcolumns.m_col_altlwr_unkn] = !!(e.get_altlwrflags() & AirspacesDb::Airspace::altflag_unkn);
}

void AirspaceEditor::set_row_altupr_flags( Gtk::TreeModel::Row & row, const AirspacesDb::Airspace & e )
{
	row[m_airspacelistcolumns.m_col_altupr_agl] = !!(e.get_altuprflags() & AirspacesDb::Airspace::altflag_agl);
	row[m_airspacelistcolumns.m_col_altupr_fl] = !!(e.get_altuprflags() & AirspacesDb::Airspace::altflag_fl);
	row[m_airspacelistcolumns.m_col_altupr_sfc] = !!(e.get_altuprflags() & AirspacesDb::Airspace::altflag_sfc);
	row[m_airspacelistcolumns.m_col_altupr_gnd] = !!(e.get_altuprflags() & AirspacesDb::Airspace::altflag_gnd);
	row[m_airspacelistcolumns.m_col_altupr_unlim] = !!(e.get_altuprflags() & AirspacesDb::Airspace::altflag_unlim);
	row[m_airspacelistcolumns.m_col_altupr_notam] = !!(e.get_altuprflags() & AirspacesDb::Airspace::altflag_notam);
	row[m_airspacelistcolumns.m_col_altupr_unkn] = !!(e.get_altuprflags() & AirspacesDb::Airspace::altflag_unkn);
}

void AirspaceEditor::set_row( Gtk::TreeModel::Row& row, const AirspacesDb::Airspace& e )
{
	row[m_airspacelistcolumns.m_col_id] = e.get_address();
	row[m_airspacelistcolumns.m_col_labelcoord] = e.get_labelcoord();
	row[m_airspacelistcolumns.m_col_label_placement] = e.get_label_placement();
	row[m_airspacelistcolumns.m_col_icao] = e.get_icao();
	row[m_airspacelistcolumns.m_col_name] = e.get_name();
	row[m_airspacelistcolumns.m_col_ident] = e.get_ident();
	row[m_airspacelistcolumns.m_col_commname] = e.get_commname();
	row[m_airspacelistcolumns.m_col_commfreq0] = e.get_commfreq(0);
	row[m_airspacelistcolumns.m_col_commfreq1] = e.get_commfreq(1);
	row[m_airspacelistcolumns.m_col_classtype] = AirspaceClassType(e.get_bdryclass(), e.get_typecode());
	row[m_airspacelistcolumns.m_col_width] = e.get_width_nmi();
	row[m_airspacelistcolumns.m_col_gndelevmin] = e.get_gndelevmin();
	row[m_airspacelistcolumns.m_col_gndelevmax] = e.get_gndelevmax();
	row[m_airspacelistcolumns.m_col_altlwr] = e.get_altlwr();
	set_row_altlwr_flags(row, e);
	row[m_airspacelistcolumns.m_col_altupr] = e.get_altupr();
	set_row_altupr_flags(row, e);
	row[m_airspacelistcolumns.m_col_sourceid] = e.get_sourceid();
	row[m_airspacelistcolumns.m_col_modtime] = e.get_modtime();
}

void AirspaceEditor::edited_labelcoord( const Glib::ustring & path_string, const Glib::ustring& new_text, Point new_coord )
{
	Gtk::TreePath path(path_string);
	Gtk::TreeModel::iterator iter = m_airspaceliststore->get_iter(path);
	if (!iter)
		return;
	Gtk::TreeRow row = *iter;
	row[m_airspacelistcolumns.m_col_labelcoord] = new_coord;
	AirspacesDb::Airspace e((*m_db)(row[m_airspacelistcolumns.m_col_id]));
	if (!e.is_valid())
		return;
	e.set_modtime(time(0));
	e.set_labelcoord(new_coord);
	save_noerr(e, row);
	m_airspaceeditormap->set_center(new_coord, e.get_altlwr_corr());
	m_airspaceeditormap->set_cursor(new_coord);
}

void AirspaceEditor::edited_width(const Glib::ustring& path_string, const Glib::ustring& new_text)
{
	Gtk::TreePath path(path_string);
	Gtk::TreeModel::iterator iter = m_airspaceliststore->get_iter(path);
	if (!iter)
		return;
	Gtk::TreeRow row = *iter;
	double w(Glib::Ascii::strtod(new_text));
	row[m_airspacelistcolumns.m_col_width] = w;
	AirspacesDb::Airspace e((*m_db)(row[m_airspacelistcolumns.m_col_id]));
	if (!e.is_valid())
		return;
	e.set_modtime(time(0));
	e.set_width_nmi(w);
	row[m_airspacelistcolumns.m_col_width] = e.get_width_nmi();
	save_noerr(e, row);
}

void AirspaceEditor::edited_gndelevmin( const Glib::ustring & path_string, const Glib::ustring & new_text )
{
	Gtk::TreePath path(path_string);
	Gtk::TreeModel::iterator iter = m_airspaceliststore->get_iter(path);
	if (!iter)
		return;
	Gtk::TreeRow row = *iter;
	int32_t elev = Conversions::signed_feet_parse(new_text);
	row[m_airspacelistcolumns.m_col_gndelevmin] = elev;
	AirspacesDb::Airspace e((*m_db)(row[m_airspacelistcolumns.m_col_id]));
	if (!e.is_valid())
		return;
	e.set_modtime(time(0));
	e.set_gndelevmin(elev);
	save_noerr(e, row);
}

void AirspaceEditor::edited_gndelevmax( const Glib::ustring & path_string, const Glib::ustring & new_text )
{
	Gtk::TreePath path(path_string);
	Gtk::TreeModel::iterator iter = m_airspaceliststore->get_iter(path);
	if (!iter)
		return;
	Gtk::TreeRow row = *iter;
	int32_t elev = Conversions::signed_feet_parse(new_text);
	row[m_airspacelistcolumns.m_col_gndelevmax] = elev;
	AirspacesDb::Airspace e((*m_db)(row[m_airspacelistcolumns.m_col_id]));
	if (!e.is_valid())
		return;
	e.set_modtime(time(0));
	e.set_gndelevmax(elev);
	save_noerr(e, row);
}

void AirspaceEditor::edited_altlwr( const Glib::ustring & path_string, const Glib::ustring & new_text )
{
	Gtk::TreePath path(path_string);
	Gtk::TreeModel::iterator iter = m_airspaceliststore->get_iter(path);
	if (!iter)
		return;
	Gtk::TreeRow row = *iter;
	int32_t elev = Conversions::signed_feet_parse(new_text);
	row[m_airspacelistcolumns.m_col_altlwr] = elev;
	AirspacesDb::Airspace e((*m_db)(row[m_airspacelistcolumns.m_col_id]));
	if (!e.is_valid())
		return;
	e.set_modtime(time(0));
	e.set_altlwr(elev);
	save_noerr(e, row);
}

void AirspaceEditor::edited_altupr( const Glib::ustring & path_string, const Glib::ustring & new_text )
{
	Gtk::TreePath path(path_string);
	Gtk::TreeModel::iterator iter = m_airspaceliststore->get_iter(path);
	if (!iter)
		return;
	Gtk::TreeRow row = *iter;
	int32_t elev = Conversions::signed_feet_parse(new_text);
	row[m_airspacelistcolumns.m_col_altupr] = elev;
	AirspacesDb::Airspace e((*m_db)(row[m_airspacelistcolumns.m_col_id]));
	if (!e.is_valid())
		return;
	e.set_modtime(time(0));
	e.set_altupr(elev);
	save_noerr(e, row);
}

void AirspaceEditor::edited_commname( const Glib::ustring & path_string, const Glib::ustring & new_text )
{
	Gtk::TreePath path(path_string);
	Gtk::TreeModel::iterator iter = m_airspaceliststore->get_iter(path);
	if (!iter)
		return;
	Gtk::TreeRow row = *iter;
	row[m_airspacelistcolumns.m_col_commname] = new_text;
	AirspacesDb::Airspace e((*m_db)(row[m_airspacelistcolumns.m_col_id]));
	if (!e.is_valid())
		return;
	e.set_modtime(time(0));
	e.set_commname(new_text);
	save_noerr(e, row);
}

void AirspaceEditor::edited_commfreq0( const Glib::ustring & path_string, const Glib::ustring & new_text, uint32_t new_freq )
{
	Gtk::TreePath path(path_string);
	Gtk::TreeModel::iterator iter = m_airspaceliststore->get_iter(path);
	if (!iter)
		return;
	Gtk::TreeRow row = *iter;
	row[m_airspacelistcolumns.m_col_commfreq0] = new_freq;
	AirspacesDb::Airspace e((*m_db)(row[m_airspacelistcolumns.m_col_id]));
	if (!e.is_valid())
		return;
	e.set_modtime(time(0));
	e.set_commfreq(0, new_freq);
	save_noerr(e, row);
}

void AirspaceEditor::edited_commfreq1( const Glib::ustring & path_string, const Glib::ustring & new_text, uint32_t new_freq )
{
	Gtk::TreePath path(path_string);
	Gtk::TreeModel::iterator iter = m_airspaceliststore->get_iter(path);
	if (!iter)
		return;
	Gtk::TreeRow row = *iter;
	row[m_airspacelistcolumns.m_col_commfreq0] = new_freq;
	AirspacesDb::Airspace e((*m_db)(row[m_airspacelistcolumns.m_col_id]));
	if (!e.is_valid())
		return;
	e.set_modtime(time(0));
	e.set_commfreq(1, new_freq);
	save_noerr(e, row);
}

void AirspaceEditor::edited_sourceid( const Glib::ustring & path_string, const Glib::ustring & new_text )
{
	Gtk::TreePath path(path_string);
	Gtk::TreeModel::iterator iter = m_airspaceliststore->get_iter(path);
	if (!iter)
		return;
	Gtk::TreeRow row = *iter;
	row[m_airspacelistcolumns.m_col_sourceid] = new_text;
	AirspacesDb::Airspace e((*m_db)(row[m_airspacelistcolumns.m_col_id]));
	if (!e.is_valid())
		return;
	e.set_modtime(time(0));
	e.set_sourceid(new_text);
	save_noerr(e, row);
}

void AirspaceEditor::edited_icao( const Glib::ustring & path_string, const Glib::ustring & new_text )
{
	Gtk::TreePath path(path_string);
	Gtk::TreeModel::iterator iter = m_airspaceliststore->get_iter(path);
	if (!iter)
		return;
	Gtk::TreeRow row = *iter;
	row[m_airspacelistcolumns.m_col_icao] = new_text;
	AirspacesDb::Airspace e((*m_db)(row[m_airspacelistcolumns.m_col_id]));
	if (!e.is_valid())
		return;
	e.set_modtime(time(0));
	e.set_icao(new_text);
	save_noerr(e, row);
}

void AirspaceEditor::edited_name( const Glib::ustring & path_string, const Glib::ustring & new_text )
{
	Gtk::TreePath path(path_string);
	Gtk::TreeModel::iterator iter = m_airspaceliststore->get_iter(path);
	if (!iter)
		return;
	Gtk::TreeRow row = *iter;
	row[m_airspacelistcolumns.m_col_name] = new_text;
	AirspacesDb::Airspace e((*m_db)(row[m_airspacelistcolumns.m_col_id]));
	if (!e.is_valid())
		return;
	e.set_modtime(time(0));
	e.set_name(new_text);
	save_noerr(e, row);
}

void AirspaceEditor::edited_ident( const Glib::ustring & path_string, const Glib::ustring & new_text )
{
	Gtk::TreePath path(path_string);
	Gtk::TreeModel::iterator iter = m_airspaceliststore->get_iter(path);
	if (!iter)
		return;
	Gtk::TreeRow row = *iter;
	row[m_airspacelistcolumns.m_col_ident] = new_text;
	AirspacesDb::Airspace e((*m_db)(row[m_airspacelistcolumns.m_col_id]));
	if (!e.is_valid())
		return;
	e.set_modtime(time(0));
	e.set_ident(new_text);
	save_noerr(e, row);
}

void AirspaceEditor::edited_airspaceclass( const Glib::ustring & path_string, const Glib::ustring & new_text, AirspaceClassType new_clstype )
{
	Gtk::TreePath path(path_string);
	Gtk::TreeModel::iterator iter = m_airspaceliststore->get_iter(path);
	if (!iter)
		return;
	Gtk::TreeRow row = *iter;
	if (!new_clstype.get_type())
		return;
	row[m_airspacelistcolumns.m_col_classtype] = new_clstype;
	AirspacesDb::Airspace e((*m_db)(row[m_airspacelistcolumns.m_col_id]));
	if (!e.is_valid())
		return;
	e.set_modtime(time(0));
	e.set_bdryclass(new_clstype.get_class());
	e.set_typecode(new_clstype.get_type());
	save_noerr(e, row);
}

void AirspaceEditor::edited_label_placement( const Glib::ustring & path_string, const Glib::ustring & new_text, NavaidsDb::Navaid::label_placement_t new_placement )
{
	Gtk::TreePath path(path_string);
	Gtk::TreeModel::iterator iter = m_airspaceliststore->get_iter(path);
	if (!iter)
		return;
	Gtk::TreeRow row = *iter;
	//std::cerr << "edited_label_placement: txt " << new_text << " pl " << (int)new_placement << std::endl;
	row[m_airspacelistcolumns.m_col_label_placement] = new_placement;
	AirspacesDb::Airspace e((*m_db)(row[m_airspacelistcolumns.m_col_id]));
	if (!e.is_valid())
		return;
	e.set_modtime(time(0));
	e.set_label_placement(new_placement);
	save_noerr(e, row);
}

void AirspaceEditor::edited_altlwr_flag( const Glib::ustring & path_string, uint8_t f)
{
	Gtk::TreePath path(path_string);
	Gtk::TreeModel::iterator iter = m_airspaceliststore->get_iter(path);
	if (!iter)
		return;
	Gtk::TreeRow row = *iter;
	AirspacesDb::Airspace e((*m_db)(row[m_airspacelistcolumns.m_col_id]));
	if (!e.is_valid())
		return;
	e.set_modtime(time(0));
	e.set_altlwrflags(e.get_altlwrflags() ^ f);
	set_row_altlwr_flags(row, e);
	save_noerr(e, row);
}

void AirspaceEditor::edited_altupr_flag( const Glib::ustring & path_string, uint8_t f)
{
	Gtk::TreePath path(path_string);
	Gtk::TreeModel::iterator iter = m_airspaceliststore->get_iter(path);
	if (!iter)
		return;
	Gtk::TreeRow row = *iter;
	AirspacesDb::Airspace e((*m_db)(row[m_airspacelistcolumns.m_col_id]));
	if (!e.is_valid())
		return;
	e.set_modtime(time(0));
	e.set_altuprflags(e.get_altuprflags() ^ f);
	set_row_altupr_flags(row, e);
	save_noerr(e, row);
}

void AirspaceEditor::edited_modtime( const Glib::ustring & path_string, const Glib::ustring& new_text, time_t new_time )
{
	Gtk::TreePath path(path_string);
	Gtk::TreeModel::iterator iter = m_airspaceliststore->get_iter(path);
	if (!iter)
		return;
	Gtk::TreeRow row = *iter;
	row[m_airspacelistcolumns.m_col_modtime] = new_time;
	AirspacesDb::Airspace e((*m_db)(row[m_airspacelistcolumns.m_col_id]));
	if (!e.is_valid())
		return;
	e.set_modtime(new_time);
	save_noerr(e, row);
}

void AirspaceEditor::edited_classexcept(void)
{
	if (!m_curentryid)
		return;
	Glib::ustring new_text(m_airspaceeditorclassexcept->get_buffer()->get_text());
	AirspacesDb::Airspace e((*m_db)(m_curentryid));
	if (!e.is_valid())
		return;
	if (new_text == e.get_classexcept())
		return;
	e.set_modtime(time(0));
	e.set_classexcept(new_text);
	save_noerr_lists(e);
}

void AirspaceEditor::edited_efftime(void)
{
	if (!m_curentryid)
		return;
	Glib::ustring new_text(m_airspaceeditorefftime->get_buffer()->get_text());
	AirspacesDb::Airspace e((*m_db)(m_curentryid));
	if (!e.is_valid())
		return;
	if (new_text == e.get_efftime())
		return;
	e.set_modtime(time(0));
	e.set_efftime(new_text);
	save_noerr_lists(e);
}

void AirspaceEditor::edited_note(void)
{
	if (!m_curentryid)
		return;
	Glib::ustring new_text(m_airspaceeditornote->get_buffer()->get_text());
	AirspacesDb::Airspace e((*m_db)(m_curentryid));
	if (!e.is_valid())
		return;
	if (new_text == e.get_note())
		return;
	e.set_modtime(time(0));
	e.set_note(new_text);
	save_noerr_lists(e);
}

void AirspaceEditor::update_radius12(const Gtk::TreeModel::iterator & iter)
{
	Gtk::TreeRow row = *iter;
	char shape(row[m_airspacesegment_model_columns.m_col_shape]);
	switch (shape) {
		case 'C':
		case 'A':
			break;

		default:
			row[m_airspacesegment_model_columns.m_col_radius1] = 0;
			row[m_airspacesegment_model_columns.m_col_radius2] = 0;
			break;

		case 'L':
		case 'R':
			Point pt1(row[m_airspacesegment_model_columns.m_col_coord1]);
			Point pt2(row[m_airspacesegment_model_columns.m_col_coord2]);
			Point pt0(row[m_airspacesegment_model_columns.m_col_coord0]);
			row[m_airspacesegment_model_columns.m_col_radius1] = pt0.spheric_distance_nmi(pt1) * (Point::from_deg / 60.0);
			row[m_airspacesegment_model_columns.m_col_radius2] = pt0.spheric_distance_nmi(pt2) * (Point::from_deg / 60.0);
			break;
	}
}

void AirspaceEditor::edited_shape(const Glib::ustring& path_string, const Glib::ustring& new_text, char new_shape)
{
	Gtk::TreePath path(path_string);
	Gtk::TreeModel::iterator iter = m_airspacesegment_model->get_iter(path);
	if (!iter)
		return;
	Gtk::TreeRow row = *iter;
	row[m_airspacesegment_model_columns.m_col_shape] = new_shape;
	update_radius12(iter);
	save_shapes();
}

void AirspaceEditor::edited_coord1(const Glib::ustring& path_string, const Glib::ustring& new_text, Point new_coord)
{
	Gtk::TreePath path(path_string);
	Gtk::TreeModel::iterator iter = m_airspacesegment_model->get_iter(path);
	if (!iter)
		return;
	Gtk::TreeRow row = *iter;
	row[m_airspacesegment_model_columns.m_col_coord1] = new_coord;
	m_airspaceeditormap->set_center(new_coord, 0);
	m_airspaceeditormap->set_cursor(new_coord);
	Gtk::TreeModel::iterator iterp(iter);
	if (iterp == m_airspacesegment_model->children().begin())
		iterp = m_airspacesegment_model->children().end();
	iterp--;
	Gtk::TreeRow rowp = *iterp;
	rowp[m_airspacesegment_model_columns.m_col_coord2] = new_coord;
	update_radius12(iter);
	save_shapes();
}

void AirspaceEditor::edited_coord2(const Glib::ustring& path_string, const Glib::ustring& new_text, Point new_coord)
{
	Gtk::TreePath path(path_string);
	Gtk::TreeModel::iterator iter = m_airspacesegment_model->get_iter(path);
	if (!iter)
		return;
	Gtk::TreeRow row = *iter;
	row[m_airspacesegment_model_columns.m_col_coord2] = new_coord;
	m_airspaceeditormap->set_center(new_coord, 0);
	m_airspaceeditormap->set_cursor(new_coord);
	Gtk::TreeModel::iterator itern(iter);
	itern++;
	if (itern == m_airspacesegment_model->children().end())
		itern = m_airspacesegment_model->children().begin();
	Gtk::TreeRow rown = *itern;
	rown[m_airspacesegment_model_columns.m_col_coord1] = new_coord;
	update_radius12(iter);
	save_shapes();
}

void AirspaceEditor::edited_coord0(const Glib::ustring& path_string, const Glib::ustring& new_text, Point new_coord)
{
	Gtk::TreePath path(path_string);
	Gtk::TreeModel::iterator iter = m_airspacesegment_model->get_iter(path);
	if (!iter)
		return;
	Gtk::TreeRow row = *iter;
	row[m_airspacesegment_model_columns.m_col_coord0] = new_coord;
	m_airspaceeditormap->set_center(new_coord, 0);
	m_airspaceeditormap->set_cursor(new_coord);
	update_radius12(iter);
	save_shapes();
}

void AirspaceEditor::edited_radius1(const Glib::ustring& path_string, const Glib::ustring& new_text)
{
	Gtk::TreePath path(path_string);
	Gtk::TreeModel::iterator iter = m_airspacesegment_model->get_iter(path);
	if (!iter)
		return;
	Gtk::TreeRow row = *iter;
	row[m_airspacesegment_model_columns.m_col_radius1] = strtod(new_text.c_str(), 0);
	save_shapes();
}

void AirspaceEditor::edited_radius2(const Glib::ustring& path_string, const Glib::ustring& new_text)
{
	Gtk::TreePath path(path_string);
	Gtk::TreeModel::iterator iter = m_airspacesegment_model->get_iter(path);
	if (!iter)
		return;
	Gtk::TreeRow row = *iter;
	row[m_airspacesegment_model_columns.m_col_radius2] = strtod(new_text.c_str(), 0);
	save_shapes();
}

void AirspaceEditor::edited_compicao(const Glib::ustring& path_string, const Glib::ustring& new_text)
{
	Gtk::TreePath path(path_string);
	Gtk::TreeModel::iterator iter = m_airspacecomponent_model->get_iter(path);
	if (!iter)
		return;
	Gtk::TreeRow row = *iter;
	row[m_airspacecomponent_model_columns.m_col_icao] = new_text;
	save_components();
}

void AirspaceEditor::edited_compclass(const Glib::ustring& path_string, const Glib::ustring& new_text, AirspaceClassType new_clstype)
{
	Gtk::TreePath path(path_string);
	Gtk::TreeModel::iterator iter = m_airspacecomponent_model->get_iter(path);
	if (!iter)
		return;
	Gtk::TreeRow row = *iter;
	row[m_airspacecomponent_model_columns.m_col_classtype] = new_clstype;
	save_components();
}

void AirspaceEditor::edited_compoperator(const Glib::ustring& path_string, const Glib::ustring& new_text, AirspacesDb::Airspace::Component::operator_t new_operator)
{
	Gtk::TreePath path(path_string);
	Gtk::TreeModel::iterator iter = m_airspacecomponent_model->get_iter(path);
	if (!iter)
		return;
	Gtk::TreeRow row = *iter;
	row[m_airspacecomponent_model_columns.m_col_operator] = new_operator;
	save_components();
}

void AirspaceEditor::map_cursor( Point cursor, VectorMapArea::cursor_validity_t valid )
{
	if (valid != VectorMapArea::cursor_mouse)
		return;
	Glib::RefPtr<Gtk::TreeSelection> airspaceeditor_selection = m_airspaceeditortreeview->get_selection();
	std::vector<Gtk::TreeModel::Path> sel(airspaceeditor_selection->get_selected_rows());
	std::vector<Gtk::TreeModel::Path>::size_type selcount(sel.size());
	Gtk::TreeModel::iterator iter;
	if (selcount > 0)
		iter = m_airspaceliststore->get_iter(*sel.begin());
	if (selcount != 1 || !iter)
		return;
	Gtk::TreeRow row = *iter;
	row[m_airspacelistcolumns.m_col_labelcoord] = cursor;
	AirspacesDb::Airspace e((*m_db)(row[m_airspacelistcolumns.m_col_id]));
	if (!e.is_valid())
		return;
	e.set_modtime(time(0));
	e.set_labelcoord(cursor);
	save_noerr(e, row);
}

void AirspaceEditor::map_drawflags(int flags)
{
	*m_airspaceeditormap = (MapRenderer::DrawFlags)flags;
}

void AirspaceEditor::save_nostack( AirspacesDb::Airspace & e )
{
	std::cerr << "save_nostack: " << e.get_nrsegments() << std::endl;
	if (!e.is_valid())
		return;
	AirspacesDb::Airspace::Address addr(e.get_address());
	m_db->save(e);
	m_dbchanged = true;
	if (m_curentryid == addr)
		m_curentryid = e.get_address();
	m_airspaceeditormap->airspaces_changed();
	Gtk::TreeIter iter = m_airspaceliststore->children().begin();
	while (iter) {
		Gtk::TreeRow row = *iter;
		if (addr == row[m_airspacelistcolumns.m_col_id]) {
			set_row(row, e);
			return;
		}
		iter++;
	}
	Gtk::TreeModel::Row row(*(m_airspaceliststore->append()));
	set_row(row, e);
}

void AirspaceEditor::save( AirspacesDb::Airspace & e )
{
	std::cerr << "save" << std::endl;
	save_nostack(e);
	m_undoredo.save(e);
	update_undoredo_menu();
}

void AirspaceEditor::save_noerr(AirspacesDb::Airspace & e, Gtk::TreeModel::Row & row)
{
	if (e.get_address() != row[m_airspacelistcolumns.m_col_id])
		throw std::runtime_error("AirportEditor::save: ID mismatch");
	try {
		save(e);
	} catch (const sqlite3x::database_error& dberr) {
		AirspacesDb::Airspace eold((*m_db)(row[m_airspacelistcolumns.m_col_id]));
		set_row(row, eold);
	}
}

void AirspaceEditor::select( const AirspacesDb::Airspace & e )
{
	Glib::RefPtr<Gtk::TreeSelection> airspaceeditor_selection = m_airspaceeditortreeview->get_selection();
	Gtk::TreeIter iter = m_airspaceliststore->children().begin();
	while (iter) {
		Gtk::TreeRow row = *iter;
		if (e.get_address() == row[m_airspacelistcolumns.m_col_id]) {
			airspaceeditor_selection->unselect_all();
			airspaceeditor_selection->select(iter);
			m_airspaceeditortreeview->scroll_to_row(Gtk::TreeModel::Path(iter));
			return;
		}
		iter++;
	}
}

void AirspaceEditor::update_undoredo_menu(void)
{
	m_airspaceeditorundo->set_sensitive(m_undoredo.can_undo());
	m_airspaceeditorredo->set_sensitive(m_undoredo.can_redo());
}

void AirspaceEditor::col_latlon1_clicked(void)
{
	std::cerr << "col_latlon1_clicked" << std::endl;
}

void AirspaceEditor::notebook_switch_page(guint page_num)
{
	std::cerr << "notebook page " << page_num << std::endl;
	m_airspaceeditornewairspace->set_sensitive(!page_num);
	airspaceeditor_selection_changed();
	airspaceeditor_shape_selection_changed();
	airspaceeditor_component_selection_changed();
}

void AirspaceEditor::hide_notebook_pages(void)
{
	for (int pg = 1;; ++pg) {
		Gtk::Widget *w(m_airspaceeditornotebook->get_nth_page(pg));
		if (!w)
			break;
		//w->hide();
		w->set_sensitive(false);
	}
}

void AirspaceEditor::show_notebook_pages(void)
{
	for (int pg = 1;; ++pg) {
		Gtk::Widget *w(m_airspaceeditornotebook->get_nth_page(pg));
		if (!w)
			break;
		//w->show();
		w->set_sensitive(true);
	}
}

void AirspaceEditor::set_segmentcomponentpolygonlist(AirspacesDb::Airspace::Address addr)
{
	m_curentryid = 0;
	m_airspacesegment_model->clear();
	m_airspacecomponent_model->clear();
	m_airspacepolygon_model->clear();
	if (!addr) {
	  unselected_out:
		m_airspaceeditorclassexcept->get_buffer()->set_text("");
		m_airspaceeditorefftime->get_buffer()->set_text("");
		m_airspaceeditornote->get_buffer()->set_text("");
		airspaceeditor_shape_selection_changed();
		if (m_airspaceeditormap)
			m_airspaceeditormap->clear_airspace();
		return;
	}
	AirspacesDb::Airspace e = (*m_db)(addr);
	if (!e.is_valid()) {
		goto unselected_out;
	}
	std::cerr << "Name: " << e.get_name() << " BBox: "
		  << e.get_bbox().get_southwest().get_lon_str2() << "," << e.get_bbox().get_southwest().get_lat_str2() << " -> "
		  << e.get_bbox().get_northeast().get_lon_str2() << "," << e.get_bbox().get_northeast().get_lat_str2() << std::endl;
	if (m_airspaceeditormap)
		m_airspaceeditormap->set_airspace(e);
	Gtk::TreeModel::iterator iter(m_airspacesegment_model->children().begin());
	for (unsigned int s = 0; s < e.get_nrsegments(); ++s) {
		const AirspacesDb::Airspace::Segment& seg(e[s]);
		Gtk::TreeModel::Row row;
		if (!iter)
			iter = m_airspacesegment_model->append();
		row = *iter;
		++iter;
		row[m_airspacesegment_model_columns.m_col_segment] = s;
		row[m_airspacesegment_model_columns.m_col_shape] = seg.get_shape();
		row[m_airspacesegment_model_columns.m_col_coord1] = seg.get_coord1();
		row[m_airspacesegment_model_columns.m_col_coord2] = seg.get_coord2();
		row[m_airspacesegment_model_columns.m_col_coord0] = seg.get_coord0();
		row[m_airspacesegment_model_columns.m_col_radius1] = seg.get_radius1() * (Point::to_deg * 60.0);
		row[m_airspacesegment_model_columns.m_col_radius2] = seg.get_radius2() * (Point::to_deg * 60.0);
	}
	for (; iter; ) {
		Gtk::TreeModel::iterator iter1(iter);
		++iter;
		m_airspacesegment_model->erase(iter1);
	}
	iter = m_airspacecomponent_model->children().begin();
	for (unsigned int c = 0; c < e.get_nrcomponents(); ++c) {
		const AirspacesDb::Airspace::Component& comp(e.get_component(c));
		Gtk::TreeModel::Row row;
		if (!iter)
			iter = m_airspacecomponent_model->append();
		row = *iter;
		++iter;
		row[m_airspacecomponent_model_columns.m_col_component] = c;
		row[m_airspacecomponent_model_columns.m_col_icao] = comp.get_icao();
		row[m_airspacecomponent_model_columns.m_col_classtype] = AirspaceClassType(comp.get_bdryclass(), comp.get_typecode());
		row[m_airspacecomponent_model_columns.m_col_operator] = comp.get_operator();
	}
	for (; iter; ) {
		Gtk::TreeModel::iterator iter1(iter);
		++iter;
		m_airspacecomponent_model->erase(iter1);
	}
	{
		const MultiPolygonHole& poly(e.get_polygon());
		unsigned int polynr(0);
		for (MultiPolygonHole::const_iterator phi(poly.begin()), phe(poly.end()); phi != phe; ++phi, ++polynr) {
			const PolygonHole& ph(*phi);
			Gtk::TreeModel::Row phrow(*(m_airspacepolygon_model->append()));
			phrow[m_airspacepolygon_model_columns.m_col_point] = polynr;
			{
				Point pt;
				pt.set_invalid();
				phrow[m_airspacepolygon_model_columns.m_col_coord] = pt;
			}
			{
				const PolygonSimple& ps(ph.get_exterior());
				Gtk::TreeModel::Row psrow(*(m_airspacepolygon_model->append(phrow.children())));
				psrow[m_airspacepolygon_model_columns.m_col_point] = 0;
				{
					Point pt;
					pt.set_invalid();
					psrow[m_airspacepolygon_model_columns.m_col_coord] = pt;
				}
				for (unsigned int p = 0; p < ps.size(); ++p) {
					Gtk::TreeModel::Row row(*(m_airspacepolygon_model->append(psrow.children())));
					row[m_airspacepolygon_model_columns.m_col_point] = p;
					row[m_airspacepolygon_model_columns.m_col_coord] = ps[p];
				}
			}
			for (unsigned int i = 0; i < ph.get_nrinterior(); ++i) {
				const PolygonSimple& ps(ph[i]);
				Gtk::TreeModel::Row psrow(*(m_airspacepolygon_model->append(phrow.children())));
				psrow[m_airspacepolygon_model_columns.m_col_point] = i + 1;
				{
					Point pt;
					pt.set_invalid();
					psrow[m_airspacepolygon_model_columns.m_col_coord] = pt;
				}
				for (unsigned int p = 0; p < ps.size(); ++p) {
					Gtk::TreeModel::Row row(*(m_airspacepolygon_model->append(psrow.children())));
					row[m_airspacepolygon_model_columns.m_col_point] = p;
					row[m_airspacepolygon_model_columns.m_col_coord] = ps[p];
				}
			}
		}
	}
	m_airspacepolyapproxeditortreeview->expand_all();
	m_airspaceeditorclassexcept->get_buffer()->set_text(e.get_classexcept());
	m_airspaceeditorefftime->get_buffer()->set_text(e.get_efftime());
	m_airspaceeditornote->get_buffer()->set_text(e.get_note());
	m_curentryid = addr;
}

void AirspaceEditor::airspaceeditor_selection_changed(void)
{
	Glib::RefPtr<Gtk::TreeSelection> airspaceeditor_selection = m_airspaceeditortreeview->get_selection();
	std::vector<Gtk::TreeModel::Path> sel(airspaceeditor_selection->get_selected_rows());
	std::vector<Gtk::TreeModel::Path>::size_type selcount(sel.size());
	Gtk::TreeModel::iterator iter;
	if (selcount > 0)
		iter = m_airspaceliststore->get_iter(*sel.begin());
	if (selcount > 1) {
		// multiple selections
		std::cerr << "multiple selection" << std::endl;
		bool firstpage(!m_airspaceeditornotebook->get_current_page());
		m_airspaceeditorduplicateairspace->set_sensitive(false);
		m_airspaceeditordeleteairspace->set_sensitive(firstpage);
		m_airspaceeditorplacelabel->set_sensitive(firstpage);
		m_airspaceeditorgndelev->set_sensitive(firstpage);
		m_airspaceeditorpoly->set_sensitive(firstpage);
		m_airspaceeditormap->invalidate_cursor();
		set_segmentcomponentpolygonlist(0);
		hide_notebook_pages();
		if (m_airspaceeditormap)
			m_airspaceeditormap->clear_airspace();
		return;
	}
	if (!selcount || !iter) {
		std::cerr << "selection cleared" << std::endl;
		m_airspaceeditorduplicateairspace->set_sensitive(false);
		m_airspaceeditordeleteairspace->set_sensitive(false);
		m_airspaceeditorplacelabel->set_sensitive(false);
		m_airspaceeditorgndelev->set_sensitive(false);
		m_airspaceeditorpoly->set_sensitive(false);
		m_airspaceeditormap->invalidate_cursor();
		set_segmentcomponentpolygonlist(0);
		hide_notebook_pages();
		if (m_airspaceeditormap)
			m_airspaceeditormap->clear_airspace();
		return;
	}
	Gtk::TreeModel::Row row = *iter;
	//Do something with the row.
	std::cerr << "selected row: " << row[m_airspacelistcolumns.m_col_id] << std::endl;
	bool firstpage(!m_airspaceeditornotebook->get_current_page());
	m_airspaceeditorduplicateairspace->set_sensitive(firstpage);
	m_airspaceeditordeleteairspace->set_sensitive(firstpage);
	m_airspaceeditorplacelabel->set_sensitive(firstpage);
	m_airspaceeditorgndelev->set_sensitive(firstpage);
	m_airspaceeditorpoly->set_sensitive(firstpage);
	Point pt(row[m_airspacelistcolumns.m_col_labelcoord]);
	m_airspaceeditormap->set_center(pt, 0);
	m_airspaceeditormap->set_cursor(pt);
	set_segmentcomponentpolygonlist(row[m_airspacelistcolumns.m_col_id]);
	show_notebook_pages();
}

void AirspaceEditor::save_shapes(void)
{
	if (!m_curentryid)
		return;
	AirspacesDb::Airspace e((*m_db)(m_curentryid));
	if (!e.is_valid())
		return;
	e.set_modtime(time(0));
	e.clear_segments();
	for (Gtk::TreeModel::iterator si(m_airspacesegment_model->children().begin()); si; ++si) {
		Gtk::TreeRow row(*si);
		AirspacesDb::Airspace::Segment seg(Point(row[m_airspacesegment_model_columns.m_col_coord1]),
						   Point(row[m_airspacesegment_model_columns.m_col_coord2]),
						   Point(row[m_airspacesegment_model_columns.m_col_coord0]),
						   row[m_airspacesegment_model_columns.m_col_shape],
						   Point::round<int32_t,double>(row[m_airspacesegment_model_columns.m_col_radius1] * (Point::from_deg / 60.0)),
						   Point::round<int32_t,double>(row[m_airspacesegment_model_columns.m_col_radius2] * (Point::from_deg / 60.0)));
		e.add_segment(seg);
	}
	e.recompute_lineapprox(*m_db);
	save_noerr_lists(e);
	if (m_airspaceeditormap)
		m_airspaceeditormap->set_airspace(e);
}

void AirspaceEditor::save_components(void)
{
	if (!m_curentryid)
		return;
	AirspacesDb::Airspace e((*m_db)(m_curentryid));
	if (!e.is_valid())
		return;
	e.set_modtime(time(0));
	e.clear_components();
	for (Gtk::TreeModel::iterator ci(m_airspacecomponent_model->children().begin()); ci; ++ci) {
		Gtk::TreeRow row(*ci);
		AirspacesDb::Airspace::Component comp(row[m_airspacecomponent_model_columns.m_col_icao],
						      ((AirspaceClassType const &)row[m_airspacecomponent_model_columns.m_col_classtype]).get_type(),
						      ((AirspaceClassType const &)row[m_airspacecomponent_model_columns.m_col_classtype]).get_class(),
						      row[m_airspacecomponent_model_columns.m_col_operator]);
		e.add_component(comp);
	}
	e.recompute_lineapprox(*m_db);
	save_noerr_lists(e);
	if (m_airspaceeditormap)
		m_airspaceeditormap->set_airspace(e);
}

void AirspaceEditor::save_noerr_lists(AirspacesDb::Airspace & e)
{
	if (!m_curentryid)
		return;
	try {
		save(e);
	} catch (const sqlite3x::database_error& dberr) {
		e = (*m_db)(m_curentryid);
	}
	set_segmentcomponentpolygonlist(e.get_address());
}

void AirspaceEditor::airspaceeditor_shape_selection_changed(void)
{
	if (!m_curentryid || 2 != m_airspaceeditornotebook->get_current_page()) {
		m_airspaceeditorappendshape->set_sensitive(false);
		m_airspaceeditorinsertshape->set_sensitive(false);
		m_airspaceeditordeleteshape->set_sensitive(false);
		return;
	}
	m_airspaceeditorappendshape->set_sensitive(true);
	Glib::RefPtr<Gtk::TreeSelection> airspaceeditor_selection = m_airspaceshapeeditortreeview->get_selection();
	std::vector<Gtk::TreeModel::Path> sel(airspaceeditor_selection->get_selected_rows());
	std::vector<Gtk::TreeModel::Path>::size_type selcount(sel.size());
	Gtk::TreeModel::iterator iter;
	if (selcount > 0)
		iter = m_airspacesegment_model->get_iter(*sel.begin());
	if (selcount > 1) {
		// multiple selections
		std::cerr << "multiple selection" << std::endl;
		m_airspaceeditorinsertshape->set_sensitive(false);
		m_airspaceeditordeleteshape->set_sensitive(true);
		return;
	}
	if (!selcount || !iter) {
		std::cerr << "selection cleared" << std::endl;
		m_airspaceeditorinsertshape->set_sensitive(false);
		m_airspaceeditordeleteshape->set_sensitive(false);
		return;
	}
	Gtk::TreeModel::Row row = *iter;
	//Do something with the row.
	std::cerr << "selected row: " << row[m_airspacesegment_model_columns.m_col_segment] << std::endl;
	m_airspaceeditorinsertshape->set_sensitive(true);
	m_airspaceeditordeleteshape->set_sensitive(true);
}

void AirspaceEditor::airspaceeditor_component_selection_changed(void)
{
	if (!m_curentryid || 3 != m_airspaceeditornotebook->get_current_page()) {
		m_airspaceeditorappendcomponent->set_sensitive(false);
		m_airspaceeditorinsertcomponent->set_sensitive(false);
		m_airspaceeditordeletecomponent->set_sensitive(false);
		return;
	}
	m_airspaceeditorappendcomponent->set_sensitive(true);
	Glib::RefPtr<Gtk::TreeSelection> airspaceeditor_selection = m_airspacecomponenteditortreeview->get_selection();
	std::vector<Gtk::TreeModel::Path> sel(airspaceeditor_selection->get_selected_rows());
	std::vector<Gtk::TreeModel::Path>::size_type selcount(sel.size());
	Gtk::TreeModel::iterator iter;
	if (selcount > 0)
		iter = m_airspacecomponent_model->get_iter(*sel.begin());
	if (selcount > 1) {
		// multiple selections
		std::cerr << "multiple selection" << std::endl;
		m_airspaceeditorinsertcomponent->set_sensitive(false);
		m_airspaceeditordeletecomponent->set_sensitive(true);
		return;
	}
	if (!selcount || !iter) {
		std::cerr << "selection cleared" << std::endl;
		m_airspaceeditorinsertcomponent->set_sensitive(false);
		m_airspaceeditordeletecomponent->set_sensitive(false);
		return;
	}
	Gtk::TreeModel::Row row = *iter;
	//Do something with the row.
	std::cerr << "selected row: " << row[m_airspacecomponent_model_columns.m_col_component] << std::endl;
	m_airspaceeditorinsertcomponent->set_sensitive(true);
	m_airspaceeditordeletecomponent->set_sensitive(true);
}

void AirspaceEditor::airspaceeditor_polygon_selection_changed(void)
{
	Glib::RefPtr<Gtk::TreeSelection> airspaceeditor_selection = m_airspacepolyapproxeditortreeview->get_selection();
	Gtk::TreeModel::iterator iter = airspaceeditor_selection->get_selected();
	if (iter) {
		Gtk::TreeModel::Row row = *iter;
		//Do something with the row.
		std::cerr << "selected row: " << row[m_airspacepolygon_model_columns.m_col_point] << std::endl;
		return;
	}
	std::cerr << "selection cleared" << std::endl;
}

void AirspaceEditor::aboutdialog_response( int response )
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
		Glib::OptionGroup optgroup("airspaceeditor", "Airspace Editor Options", "Options controlling the Airspace Editor");
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
		osso_context_t* osso_context = osso_initialize("vfrairspaceeditor", VERSION, TRUE /* deprecated parameter */, 0 /* Use default Glib main loop context */);
		if (!osso_context) {
			std::cerr << "osso_initialize() failed." << std::endl;
			return OSSO_ERROR;
		}
#endif
#ifdef HAVE_HILDON
		Hildon::init();
#endif
		Glib::set_application_name("VFR Airspace Editor");
		Glib::thread_init();
		AirspaceEditor editor(dir_main, auxdbmode, dir_aux);

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
