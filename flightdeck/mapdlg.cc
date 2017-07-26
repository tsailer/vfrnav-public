//
// C++ Implementation: FlightDeckWindow
//
// Description: Flight Deck Windows, Altitude Dialog
//
//
// Author: Thomas Sailer <t.sailer@alumni.ethz.ch>, (C) 2011, 2012, 2013
//
// Copyright: See COPYING file that comes with this distribution
//
//

#include "sysdeps.h"
#include "sysdepsgui.h"
#include <iomanip>

#include "flightdeckwindow.h"

void FlightDeckWindow::map_dialog_open(void)
{
	if (!m_mapdialog)
		return;
	// update Bitmap Maps database
	m_mapdialog->clear_bitmap_maps();
	if (m_engine) {
		m_engine->update_bitmapmaps(true);
		BitmapMaps& db(m_engine->get_bitmapmaps_db());
		for (unsigned int i = 0, sz = db.size(); i < sz; ++i)
			m_mapdialog->add_bitmap_map(db[i]);
	}
	m_mapdialog->set_flags(m_navarea.map_get_drawflags());
	m_mapdialog->set_mapup(m_navarea.map_get_mapup());
	m_mapdialog->set_renderers(m_navarea.map_get_renderers());
	m_mapdialog->set_bitmapmaps(m_navarea.map_get_bitmapmaps());
	m_mapdialog->show();
}

void FlightDeckWindow::map_settings_update(void)
{
	if (!m_mapdialog)
		return;
	m_navarea.map_set_drawflags(m_mapdialog->get_flags());
	m_navarea.map_set_mapup(m_mapdialog->get_mapup());
	m_navarea.map_set_renderers(m_mapdialog->get_renderers());
	m_navarea.map_set_bitmapmaps(m_mapdialog->get_bitmapmaps());
	m_navarea.map_set_declutter(NavArea::dcltr_normal);
	if (m_wptdrawingarea)
		*m_wptdrawingarea = m_navarea.map_get_declutter_drawflags();
	if (m_fpldrawingarea)
		*m_fpldrawingarea = m_navarea.map_get_declutter_drawflags();
	if (m_menuid <= 300)
		set_menu_button(5, m_navarea.map_get_declutter_name());
}

const unsigned int FlightDeckWindow::MapDialog::nrcheckbuttons;
const struct FlightDeckWindow::MapDialog::checkbutton FlightDeckWindow::MapDialog::checkbuttons[nrcheckbuttons] = {
	{ "mapcheckbuttontopo", MapRenderer::drawflags_topo },
	{ "mapcheckbuttonnorth", MapRenderer::drawflags_northpointer },
	{ "mapcheckbuttonterrain", MapRenderer::drawflags_terrain },
	{ "mapcheckbuttonterrainnames", MapRenderer::drawflags_terrain_names },
	{ "mapcheckbuttonterrainborders", MapRenderer::drawflags_terrain_borders },
	{ "mapcheckbuttonroute", MapRenderer::drawflags_route },
	{ "mapcheckbuttonroutelabels", MapRenderer::drawflags_route_labels },
	{ "mapcheckbuttontrack", MapRenderer::drawflags_track },
	{ "mapcheckbuttontrackpoints", MapRenderer::drawflags_track_points },
	{ "mapcheckbuttonnavaidvor", MapRenderer::drawflags_navaids_vor },
	{ "mapcheckbuttonnavaiddme", MapRenderer::drawflags_navaids_dme },
	{ "mapcheckbuttonnavaidndb", MapRenderer::drawflags_navaids_ndb },
	{ "mapcheckbuttonwaypointlow", MapRenderer::drawflags_waypoints_low },
	{ "mapcheckbuttonwaypointhigh", MapRenderer::drawflags_waypoints_high },
	{ "mapcheckbuttonwaypointrnav", MapRenderer::drawflags_waypoints_rnav },
	{ "mapcheckbuttonwaypointterminal", MapRenderer::drawflags_waypoints_terminal },
	{ "mapcheckbuttonwaypointvfr", MapRenderer::drawflags_waypoints_vfr },
	{ "mapcheckbuttonwaypointuser", MapRenderer::drawflags_waypoints_user },
	{ "mapcheckbuttonairportciv", MapRenderer::drawflags_airports_civ },
	{ "mapcheckbuttonairportmil", MapRenderer::drawflags_airports_mil },
	{ "mapcheckbuttonairportloccone", MapRenderer::drawflags_airports_localizercone },
	{ "mapcheckbuttonairspaceab", MapRenderer::drawflags_airspaces_ab },
	{ "mapcheckbuttonairspacecd", MapRenderer::drawflags_airspaces_cd },
	{ "mapcheckbuttonairspaceefg", MapRenderer::drawflags_airspaces_efg },
	{ "mapcheckbuttonairspacespecialuse", MapRenderer::drawflags_airspaces_specialuse },
	{ "mapcheckbuttonairspacefillground", MapRenderer::drawflags_airspaces_fill_ground },
	{ "mapcheckbuttonairwaylow", MapRenderer::drawflags_airways_low },
	{ "mapcheckbuttonairwayhigh", MapRenderer::drawflags_airways_high },
	{ "mapcheckbuttonweather", MapRenderer::drawflags_weather },
};

FlightDeckWindow::MapDialog::MapupModelColumns::MapupModelColumns(void)
{
	add(m_text);
}

FlightDeckWindow::MapDialog::RendererModelColumns::RendererModelColumns(void)
{
	add(m_short);
	add(m_text);
	add(m_id);
}

FlightDeckWindow::MapDialog::BitmapMapsModelColumns::BitmapMapsModelColumns(void)
{
	add(m_name);
	add(m_scale);
	add(m_date);
	add(m_map);
}

FlightDeckWindow::MapDialog::MapDialog(BaseObjectType *castitem, const Glib::RefPtr<Builder>& refxml)
	: Gtk::Window(castitem), m_mapcomboboxup(0), m_maptreeviewrenderer(0), m_buttonrevert(0), m_buttondone(0),
	  m_curflags(MapRenderer::drawflags_all), m_flags(MapRenderer::drawflags_all),
	  m_curmapup(NavArea::mapup_north), m_mapup(NavArea::mapup_north),
	  m_currenderers(~0U), m_renderers(~0U)
{
#ifndef GLIBMM_DEFAULT_SIGNAL_HANDLERS_ENABLED
        signal_delete_event().connect(sigc::mem_fun(*this, &MapDialog::on_delete_event));
#endif //GLIBMM_DEFAULT_SIGNAL_HANDLERS_ENABLED
	get_widget(refxml, "mapcomboboxup", m_mapcomboboxup);
	get_widget(refxml, "maptreeviewrenderer", m_maptreeviewrenderer);
	get_widget(refxml, "maptreeviewbitmapmaps", m_maptreeviewbitmapmaps);
	get_widget(refxml, "mapbuttonrevert", m_buttonrevert);
	get_widget(refxml, "mapbuttondone", m_buttondone);
	m_mapupstore = Gtk::ListStore::create(m_mapupcolumns);
	m_bitmapmapsstore = Gtk::ListStore::create(m_bitmapmapscolumns);
	{
                Gtk::TreeModel::Row row(*(m_mapupstore->append()));
                row[m_mapupcolumns.m_text] = "North Up";
                row = *(m_mapupstore->append());
                row[m_mapupcolumns.m_text] = "Track Up";
                row = *(m_mapupstore->append());
                row[m_mapupcolumns.m_text] = "Heading Up";
	}
        if (m_mapcomboboxup) {
                m_mapcomboboxup->clear();
                m_mapcomboboxup->set_model(m_mapupstore);
                m_mapcomboboxup->set_entry_text_column(m_mapupcolumns.m_text);
                m_mapcomboboxup->pack_start(m_mapupcolumns.m_text);
		m_connmapup = m_mapcomboboxup->signal_changed().connect(sigc::mem_fun(*this, &MapDialog::mapup_changed));
        }
	m_rendererstore = Gtk::ListStore::create(m_renderercolumns);
	for (VectorMapArea::renderer_t r = VectorMapArea::renderer_vmap; r < VectorMapArea::renderer_none; r = (VectorMapArea::renderer_t)(r + 1)) {
		if (!VectorMapArea::is_renderer_enabled(r))
			continue;
		if (r == VectorMapArea::renderer_bitmap)
			continue;
		Gtk::TreeModel::Row row(*(m_rendererstore->append()));
	        row[m_renderercolumns.m_short] = to_short_str(r);
	        row[m_renderercolumns.m_text] = to_str(r);
		row[m_renderercolumns.m_id] = r;
	}
	if (m_maptreeviewrenderer) {
		m_maptreeviewrenderer->set_model(m_rendererstore);
                Gtk::CellRendererText *ident_renderer = Gtk::manage(new Gtk::CellRendererText());
                int ident_col = m_maptreeviewrenderer->append_column(_("Id"), *ident_renderer) - 1;
                Gtk::TreeViewColumn *ident_column = m_maptreeviewrenderer->get_column(ident_col);
                if (ident_column) {
                        ident_column->add_attribute(*ident_renderer, "text", m_renderercolumns.m_short);
                }
                Gtk::CellRendererText *text_renderer = Gtk::manage(new Gtk::CellRendererText());
                int text_col = m_maptreeviewrenderer->append_column(_("Name"), *text_renderer) - 1;
                Gtk::TreeViewColumn *text_column = m_maptreeviewrenderer->get_column(text_col);
                if (text_column) {
                        text_column->add_attribute(*text_renderer, "text", m_renderercolumns.m_text);
                }
		ident_column->set_resizable(false);
		text_column->set_resizable(true);
		ident_column->set_expand(false);
		text_column->set_expand(true);
		ident_column->set_sort_column(m_renderercolumns.m_short);
		text_column->set_sort_column(m_renderercolumns.m_text);
                m_maptreeviewrenderer->columns_autosize();
                m_maptreeviewrenderer->set_headers_clickable();
		Glib::RefPtr<Gtk::TreeSelection> selection = m_maptreeviewrenderer->get_selection();
		selection->set_mode(Gtk::SELECTION_MULTIPLE);
		m_connrenderer = selection->signal_changed().connect(sigc::mem_fun(*this, &MapDialog::renderer_changed));
		m_maptreeviewrenderer->set_rubber_banding(true);
	}
	if (m_maptreeviewbitmapmaps) {
		m_maptreeviewbitmapmaps->set_model(m_bitmapmapsstore);
                Gtk::CellRendererText *name_renderer = Gtk::manage(new Gtk::CellRendererText());
                int name_col = m_maptreeviewbitmapmaps->append_column(_("Name"), *name_renderer) - 1;
                Gtk::TreeViewColumn *name_column = m_maptreeviewbitmapmaps->get_column(name_col);
                if (name_column) {
                        name_column->add_attribute(*name_renderer, "text", m_bitmapmapscolumns.m_name);
                }
                Gtk::CellRendererText *scale_renderer = Gtk::manage(new Gtk::CellRendererText());
                int scale_col = m_maptreeviewbitmapmaps->append_column(_("Scale"), *scale_renderer) - 1;
                Gtk::TreeViewColumn *scale_column = m_maptreeviewbitmapmaps->get_column(scale_col);
                if (scale_column) {
                        scale_column->add_attribute(*scale_renderer, "text", m_bitmapmapscolumns.m_scale);
                }
                Gtk::CellRendererText *date_renderer = Gtk::manage(new Gtk::CellRendererText());
                int date_col = m_maptreeviewbitmapmaps->append_column(_("Date"), *date_renderer) - 1;
                Gtk::TreeViewColumn *date_column = m_maptreeviewbitmapmaps->get_column(date_col);
                if (date_column) {
                        date_column->add_attribute(*date_renderer, "text", m_bitmapmapscolumns.m_date);
                }
		scale_renderer->set_property("xalign", 1.0);
		date_renderer->set_property("xalign", 1.0);
		name_column->set_resizable(true);
		scale_column->set_resizable(false);
		date_column->set_resizable(false);
		name_column->set_expand(true);
		scale_column->set_expand(false);
		date_column->set_expand(false);
		name_column->set_sort_column(m_bitmapmapscolumns.m_name);
		scale_column->set_sort_column(m_bitmapmapscolumns.m_scale);
		date_column->set_sort_column(m_bitmapmapscolumns.m_date);
                m_maptreeviewbitmapmaps->columns_autosize();
                m_maptreeviewbitmapmaps->set_headers_clickable();
		Glib::RefPtr<Gtk::TreeSelection> selection = m_maptreeviewbitmapmaps->get_selection();
		selection->set_mode(Gtk::SELECTION_MULTIPLE);
	        m_connbitmapmaps = selection->signal_changed().connect(sigc::mem_fun(*this, &MapDialog::bitmapmaps_changed));
		m_maptreeviewbitmapmaps->set_rubber_banding(true);
	}
	update_renderer();
	update_bitmapmaps();
	if (m_buttonrevert)
		m_connrevert = m_buttonrevert->signal_clicked().connect(sigc::mem_fun(*this, &MapDialog::revert_clicked));
	if (m_buttondone)
		m_conndone = m_buttondone->signal_clicked().connect(sigc::mem_fun(*this, &MapDialog::done_clicked));
	for (unsigned int i = 0; i < nrcheckbuttons; ++i) {
		m_checkbutton[i] = 0;
		get_widget(refxml, checkbuttons[i].widgetname, m_checkbutton[i]);
		if (!m_checkbutton[i])
			continue;
		m_conncheckbutton[i] = m_checkbutton[i]->signal_toggled().connect(sigc::bind(sigc::mem_fun(*this, &MapDialog::flag_toggled), i));
	}
	set_flags(MapRenderer::drawflags_all);
}

FlightDeckWindow::MapDialog::~MapDialog()
{
}

void FlightDeckWindow::MapDialog::set_flags(MapRenderer::DrawFlags flags)
{
	m_curflags = m_flags = flags;
	for (unsigned int i = 0; i < nrcheckbuttons; ++i)
		m_conncheckbutton[i].block();
	for (unsigned int i = 0; i < nrcheckbuttons; ++i)
		if (m_checkbutton[i])
			m_checkbutton[i]->set_active(!!(m_flags & checkbuttons[i].drawflag));
	for (unsigned int i = 0; i < nrcheckbuttons; ++i)
		m_conncheckbutton[i].unblock();
}

void FlightDeckWindow::MapDialog::set_mapup(NavArea::mapup_t up)
{
	m_curmapup = m_mapup = up;
	m_connmapup.block();
	if (m_mapcomboboxup)
		m_mapcomboboxup->set_active((int)m_mapup);
	m_connmapup.unblock();
}

void FlightDeckWindow::MapDialog::set_renderers(uint32_t renderers)
{
	m_currenderers = m_renderers = renderers;
	update_renderer();
}

void FlightDeckWindow::MapDialog::set_bitmapmaps(const NavArea::bitmapmaps_t& bmm)
{
	m_curbitmapmaps = m_bitmapmaps = bmm;
	update_bitmapmaps();
}

bool FlightDeckWindow::MapDialog::on_delete_event(GdkEventAny* event)
{
	unset_focus();
        hide();
	m_signal_done.emit();
        return true;
}

void FlightDeckWindow::MapDialog::done_clicked(void)
{
	unset_focus();
        hide();
	m_signal_done.emit();
}

void FlightDeckWindow::MapDialog::revert_clicked(void)
{
	set_flags(m_curflags);
	set_mapup(m_curmapup);
	set_renderers(m_currenderers);
	set_bitmapmaps(m_curbitmapmaps);
}

void FlightDeckWindow::MapDialog::flag_toggled(unsigned int nr)
{
	if (nr >= nrcheckbuttons || !m_checkbutton[nr])
		return;
	if (m_checkbutton[nr]->get_active())
		m_flags |= checkbuttons[nr].drawflag;
	else
		m_flags &= ~checkbuttons[nr].drawflag;
}

void FlightDeckWindow::MapDialog::mapup_changed(void)
{
	if (!m_mapcomboboxup)
		return;
	int x(m_mapcomboboxup->get_active_row_number());
	if (x == -1)
		return;
	m_mapup = (NavArea::mapup_t)x;
}

void FlightDeckWindow::MapDialog::renderer_changed(void)
{
	if (!m_maptreeviewrenderer || !m_rendererstore)
		return;
	Glib::RefPtr<Gtk::TreeSelection> selection = m_maptreeviewrenderer->get_selection();
        for (Gtk::TreeIter iter(m_rendererstore->children().begin()); iter; ++iter) {
                Gtk::TreeModel::Row row(*iter);
		if (selection->is_selected(iter))
			m_renderers |= 1 << row[m_renderercolumns.m_id];
		else
			m_renderers &= ~(1 << row[m_renderercolumns.m_id]);
	}
}

void FlightDeckWindow::MapDialog::update_renderer(void)
{
	if (!m_maptreeviewrenderer || !m_rendererstore)
		return;
	Glib::RefPtr<Gtk::TreeSelection> selection = m_maptreeviewrenderer->get_selection();
	m_connrenderer.block();
        for (Gtk::TreeIter iter(m_rendererstore->children().begin()); iter; ++iter) {
                Gtk::TreeModel::Row row(*iter);
		if (m_renderers & (1 << row[m_renderercolumns.m_id]))
			selection->select(iter);
		else
			selection->unselect(iter);
	}
	m_connrenderer.unblock();
}

void FlightDeckWindow::MapDialog::bitmapmaps_changed(void)
{
	if (!m_maptreeviewbitmapmaps || !m_bitmapmapsstore)
		return;
	m_bitmapmaps.clear();
	Glib::RefPtr<Gtk::TreeSelection> selection = m_maptreeviewbitmapmaps->get_selection();
        for (Gtk::TreeIter iter(m_bitmapmapsstore->children().begin()); iter; ++iter) {
		if (!selection->is_selected(iter))
			continue;
                Gtk::TreeModel::Row row(*iter);
		BitmapMapsModelColumns::MapPtr mptr(row[m_bitmapmapscolumns.m_map]);
		m_bitmapmaps.push_back(mptr.get());
	}
}

void FlightDeckWindow::MapDialog::update_bitmapmaps(void)
{
	if (!m_maptreeviewbitmapmaps || !m_bitmapmapsstore)
		return;
	typedef std::set<BitmapMaps::Map::ptr_t> set_t;
	set_t s;
	for (NavArea::bitmapmaps_t::const_iterator i(m_bitmapmaps.begin()), e(m_bitmapmaps.end()); i != e; ++i)
		s.insert(*i);
	Glib::RefPtr<Gtk::TreeSelection> selection = m_maptreeviewbitmapmaps->get_selection();
        for (Gtk::TreeIter iter(m_bitmapmapsstore->children().begin()); iter; ++iter) {
                Gtk::TreeModel::Row row(*iter);
		BitmapMapsModelColumns::MapPtr mptr(row[m_bitmapmapscolumns.m_map]);
		if (s.find(mptr.get()) != s.end())
			selection->select(iter);
		else
			selection->unselect(iter);
	}
}

void FlightDeckWindow::MapDialog::clear_bitmap_maps(void)
{
	m_bitmapmapsstore->clear();
}

void FlightDeckWindow::MapDialog::add_bitmap_map(const  BitmapMaps::Map::ptr_t& m)
{
	if (!m)
		return;
	Gtk::TreeModel::Row row(*(m_bitmapmapsstore->append()));
	row[m_bitmapmapscolumns.m_name] = m->get_name();
	row[m_bitmapmapscolumns.m_scale] = Glib::ustring::format(m->get_scale());
	{
		Glib::DateTime dt(Glib::DateTime::create_now_utc(m->get_time()));
		row[m_bitmapmapscolumns.m_date] = dt.format("%Y-%m-%d %H:%M:%S");
	}
 	row[m_bitmapmapscolumns.m_map] = BitmapMapsModelColumns::MapPtr(m);
}
