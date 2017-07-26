//
// C++ Implementation: FlightDeckWindow
//
// Description: Flight Deck Windows, Map Tile Downloader Dialog
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

void FlightDeckWindow::maptile_dialog_open(void)
{
	if (!m_maptiledialog || !m_sensors)
		return;
	const FPlanRoute& fpl(m_sensors->get_route());
	m_maptiledialog->open(fpl, m_navarea.map_get_renderers());
}

FlightDeckWindow::MapTileDownloaderDialog::RendererModelColumns::RendererModelColumns(void)
{
	add(m_short);
	add(m_text);
	add(m_totaltiles);
	add(m_outstandingtiles);
	add(m_id);
}

FlightDeckWindow::MapTileDownloaderDialog::MapTileDownloaderDialog(BaseObjectType *castitem, const Glib::RefPtr<Builder>& refxml)
	: Gtk::Window(castitem), m_spinbuttonenroutezoom(0), m_spinbuttonairportzoom(0),
	  m_entryoutstanding(0), m_togglebuttondownload(0), m_buttondone(0), m_treeview(0)
{
#ifndef GLIBMM_DEFAULT_SIGNAL_HANDLERS_ENABLED
        signal_delete_event().connect(sigc::mem_fun(*this, &MapTileDownloaderDialog::on_delete_event));
#endif //GLIBMM_DEFAULT_SIGNAL_HANDLERS_ENABLED
	get_widget(refxml, "fplmaptilespinbuttonenroutezoom", m_spinbuttonenroutezoom);
	get_widget(refxml, "fplmaptilespinbuttonairportzoom", m_spinbuttonairportzoom);
	get_widget(refxml, "fplmaptileentryoutstanding", m_entryoutstanding);
	get_widget(refxml, "fplmaptiletogglebuttondownload", m_togglebuttondownload);
	get_widget(refxml, "fplmaptilebuttondone", m_buttondone);
	get_widget(refxml, "fplmaptiletreeview", m_treeview);
	m_rendererstore = Gtk::ListStore::create(m_renderercolumns);
	for (VectorMapArea::renderer_t r = VectorMapArea::renderer_openstreetmap; r < VectorMapArea::renderer_none; r = (VectorMapArea::renderer_t)(r + 1)) {
		if (!VectorMapArea::is_renderer_enabled(r))
			continue;
		Gtk::TreeModel::Row row(*(m_rendererstore->append()));
		VectorMapRenderer::TileDownloader dl((VectorMapRenderer::renderer_t)r);
	        row[m_renderercolumns.m_short] = dl.get_shortname();
	        row[m_renderercolumns.m_text] = dl.get_friendlyname();
		row[m_renderercolumns.m_totaltiles] = 0;
		row[m_renderercolumns.m_outstandingtiles] = 0;
		row[m_renderercolumns.m_id] = r;
	}
	if (m_treeview) {
		m_treeview->set_model(m_rendererstore);
                Gtk::CellRendererText *ident_renderer = Gtk::manage(new Gtk::CellRendererText());
                int ident_col = m_treeview->append_column(_("Id"), *ident_renderer) - 1;
                Gtk::TreeViewColumn *ident_column = m_treeview->get_column(ident_col);
                if (ident_column) {
                        ident_column->add_attribute(*ident_renderer, "text", m_renderercolumns.m_short);
                }
                Gtk::CellRendererText *text_renderer = Gtk::manage(new Gtk::CellRendererText());
                int text_col = m_treeview->append_column(_("Name"), *text_renderer) - 1;
                Gtk::TreeViewColumn *text_column = m_treeview->get_column(text_col);
                if (text_column) {
                        text_column->add_attribute(*text_renderer, "text", m_renderercolumns.m_text);
                }
                Gtk::CellRendererText *totaltiles_renderer = Gtk::manage(new Gtk::CellRendererText());
                int totaltiles_col = m_treeview->append_column(_("Total"), *totaltiles_renderer) - 1;
                Gtk::TreeViewColumn *totaltiles_column = m_treeview->get_column(totaltiles_col);
                if (totaltiles_column) {
                        totaltiles_column->add_attribute(*totaltiles_renderer, "text", m_renderercolumns.m_totaltiles);
                }
                Gtk::CellRendererText *outstandingtiles_renderer = Gtk::manage(new Gtk::CellRendererText());
                int outstandingtiles_col = m_treeview->append_column(_("Outstanding"), *outstandingtiles_renderer) - 1;
                Gtk::TreeViewColumn *outstandingtiles_column = m_treeview->get_column(outstandingtiles_col);
                if (outstandingtiles_column) {
                        outstandingtiles_column->add_attribute(*outstandingtiles_renderer, "text", m_renderercolumns.m_outstandingtiles);
                }
                totaltiles_renderer->set_property("xalign", 1.0);
                outstandingtiles_renderer->set_property("xalign", 1.0);
		ident_column->set_resizable(false);
		text_column->set_resizable(true);
		totaltiles_column->set_resizable(false);
		outstandingtiles_column->set_resizable(false);
		ident_column->set_expand(false);
		text_column->set_expand(true);
		totaltiles_column->set_expand(false);
		outstandingtiles_column->set_expand(false);
		ident_column->set_sort_column(m_renderercolumns.m_short);
		text_column->set_sort_column(m_renderercolumns.m_text);
		totaltiles_column->set_sort_column(m_renderercolumns.m_totaltiles);
		outstandingtiles_column->set_sort_column(m_renderercolumns.m_outstandingtiles);
                m_treeview->columns_autosize();
                m_treeview->set_headers_clickable();
		Glib::RefPtr<Gtk::TreeSelection> selection = m_treeview->get_selection();
		selection->set_mode(Gtk::SELECTION_MULTIPLE);
		m_connrenderer = selection->signal_changed().connect(sigc::mem_fun(*this, &MapTileDownloaderDialog::renderer_changed));
	}
	if (m_buttondone)
		m_buttondone->signal_clicked().connect(sigc::mem_fun(*this, &MapTileDownloaderDialog::close_clicked));
	if (m_togglebuttondownload)
		m_togglebuttondownload->signal_clicked().connect(sigc::mem_fun(*this, &MapTileDownloaderDialog::download_clicked));
	if (m_spinbuttonenroutezoom)
		m_spinbuttonenroutezoom->signal_changed().connect(sigc::mem_fun(*this, &MapTileDownloaderDialog::zoom_changed));
	if (m_spinbuttonairportzoom)
		m_spinbuttonairportzoom->signal_changed().connect(sigc::mem_fun(*this, &MapTileDownloaderDialog::zoom_changed));
}

FlightDeckWindow::MapTileDownloaderDialog::~MapTileDownloaderDialog()
{
}

void FlightDeckWindow::MapTileDownloaderDialog::open(const FPlanRoute& fpl, uint32_t renderers)
{
	if (!m_treeview || !m_rendererstore || !fpl.get_nrwpt()) {
		hide();
		return;
	}
	{
		Glib::RefPtr<Gtk::TreeSelection> selection = m_treeview->get_selection();
		m_connrenderer.block();
		for (Gtk::TreeIter iter(m_rendererstore->children().begin()); iter; ++iter) {
			Gtk::TreeModel::Row row(*iter);
			if (renderers & (1 << row[m_renderercolumns.m_id]))
				selection->select(iter);
			else
				selection->unselect(iter);
		}
		m_connrenderer.unblock();
	}
	m_bbox = fpl.get_bbox().oversize_nmi(100.f);
	m_bboxdep = Rect(fpl[0].get_coord(), fpl[0].get_coord()).oversize_nmi(10.f);
	m_bboxdest = Rect(fpl[fpl.get_nrwpt()-1U].get_coord(), fpl[fpl.get_nrwpt()-1U].get_coord()).oversize_nmi(10.f);
	zoom_changed();
	renderer_changed();
	show();
}

void FlightDeckWindow::MapTileDownloaderDialog::zoom_changed(void)
{
	if (!m_rendererstore)
		return;
	unsigned int enrzoom(10), arptzoom(17);
	if (m_spinbuttonenroutezoom)
		enrzoom = m_spinbuttonenroutezoom->get_value_as_int();
	if (m_spinbuttonairportzoom)
		arptzoom = m_spinbuttonairportzoom->get_value_as_int();
	for (Gtk::TreeIter iter(m_rendererstore->children().begin()); iter; ++iter) {
		Gtk::TreeModel::Row row(*iter);
		VectorMapRenderer::TileDownloader dl(VectorMapArea::convert_renderer(row[m_renderercolumns.m_id]));
		unsigned int tot(0), outs(0);
		for (unsigned int zoom = dl.get_minzoom(), maxz = dl.get_maxzoom(); zoom <= maxz; ++zoom) {
			if (zoom <= enrzoom) {
				tot += dl.rect_tiles(zoom, m_bbox, false);
				outs += dl.rect_tiles(zoom, m_bbox, true);
			}
			if (zoom <= arptzoom) {
				tot += dl.rect_tiles(zoom, m_bboxdep, false);
				outs += dl.rect_tiles(zoom, m_bboxdep, true);
				tot += dl.rect_tiles(zoom, m_bboxdest, false);
				outs += dl.rect_tiles(zoom, m_bboxdest, true);
			}
			if (zoom > enrzoom && zoom > arptzoom)
				break;
		}
		row[m_renderercolumns.m_totaltiles] = tot;
		row[m_renderercolumns.m_outstandingtiles] = outs;
	}
}

bool FlightDeckWindow::MapTileDownloaderDialog::on_delete_event(GdkEventAny* event)
{
        close_clicked();
        return true;
}

void FlightDeckWindow::MapTileDownloaderDialog::close_clicked(void)
{
        hide();
	if (m_togglebuttondownload)
		m_togglebuttondownload->set_active(false);
	m_downloaders.clear();
}

void FlightDeckWindow::MapTileDownloaderDialog::download_clicked(void)
{
	if (!m_togglebuttondownload)
		return;
	bool act(m_togglebuttondownload->get_active());
	if (m_spinbuttonenroutezoom)
		m_spinbuttonenroutezoom->set_sensitive(!act);
	if (m_spinbuttonairportzoom)
		m_spinbuttonairportzoom->set_sensitive(!act);
	if (m_treeview)
		m_treeview->set_sensitive(!act);
	if (!act) {
		zoom_changed();
		renderer_changed();
		return;
	}
	m_downloaders.clear();
	if (!m_treeview || !m_rendererstore)
		return;
	{
		Glib::RefPtr<Gtk::TreeSelection> selection = m_treeview->get_selection();
		for (Gtk::TreeIter iter(m_rendererstore->children().begin()); iter; ++iter) {
			Gtk::TreeModel::Row row(*iter);
			if (!selection->is_selected(iter))
				continue;
			m_downloaders.push_back(VectorMapRenderer::TileDownloader(VectorMapArea::convert_renderer(row[m_renderercolumns.m_id])));
		}
	}
	{
		unsigned int enrzoom(10), arptzoom(17);
		if (m_spinbuttonenroutezoom)
			enrzoom = m_spinbuttonenroutezoom->get_value_as_int();
		if (m_spinbuttonairportzoom)
			arptzoom = m_spinbuttonairportzoom->get_value_as_int();
		for (downloaders_t::iterator i(m_downloaders.begin()), e(m_downloaders.end()); i != e; ++i) {
			for (unsigned int zoom = i->get_minzoom(), maxz = i->get_maxzoom(); zoom <= maxz; ++zoom) {
				if (zoom <= enrzoom)
					i->queue_rect(zoom, m_bbox);
				if (zoom <= arptzoom) {
					i->queue_rect(zoom, m_bboxdep);
					i->queue_rect(zoom, m_bboxdest);
				}
				if (zoom > enrzoom && zoom > arptzoom)
					break;
			}
			i->connect_queuechange(sigc::mem_fun(*this, &MapTileDownloaderDialog::queue_changed));
		}
	}
}

void FlightDeckWindow::MapTileDownloaderDialog::renderer_changed(void)
{
	if (!m_treeview || !m_rendererstore)
		return;
	unsigned int outs(0);
	Glib::RefPtr<Gtk::TreeSelection> selection = m_treeview->get_selection();
        for (Gtk::TreeIter iter(m_rendererstore->children().begin()); iter; ++iter) {
                Gtk::TreeModel::Row row(*iter);
		if (!selection->is_selected(iter))
			continue;
		outs += row[m_renderercolumns.m_outstandingtiles];
	}
	if (m_entryoutstanding) {
		std::ostringstream oss;
		oss << outs;
		m_entryoutstanding->set_text(oss.str());
	}
}

void FlightDeckWindow::MapTileDownloaderDialog::queue_changed(void)
{
	unsigned int qs(0);
	for (downloaders_t::const_iterator i(m_downloaders.begin()), e(m_downloaders.end()); i != e; ++i)
		qs += i->queue_size();
	if (m_entryoutstanding) {
		std::ostringstream oss;
		oss << qs;
		m_entryoutstanding->set_text(oss.str());
	}
	if (!qs && m_togglebuttondownload)
		m_togglebuttondownload->set_active(false);
}
