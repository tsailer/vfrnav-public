//
// C++ Implementation: FlightDeckWindow
//
// Description: Flight Deck Windows
//
//
// Author: Thomas Sailer <t.sailer@alumni.ethz.ch>, (C) 2012
//
// Copyright: See COPYING file that comes with this distribution
//
//

#include "sysdeps.h"

#include <set>
#include <sstream>
#include <cstring>

#include "flightdeckwindow.h"
#include "sensors.h"
#include "wmm.h"
#include "SunriseSunset.h"
#include "baro.h"
#include "sysdepsgui.h"


FlightDeckWindow::SplashWindow::ConfigModelColumns::ConfigModelColumns(void)
{
	add(m_col_name);
	add(m_col_description);
	add(m_col_filename);
}

FlightDeckWindow::SplashWindow::SplashWindow(BaseObjectType *castitem, const Glib::RefPtr<Builder>& refxml)
	: FullscreenableWindow(castitem, refxml), m_buttonok(0), m_buttoncancel(0), m_buttonabout(0), m_treeviewcfg(0), m_aboutdialog(0)
{
	get_widget(refxml, "buttonok", m_buttonok);
	get_widget(refxml, "buttoncancel", m_buttoncancel);
	get_widget(refxml, "buttonabout", m_buttonabout);
	get_widget(refxml, "treeviewcfg", m_treeviewcfg);
	get_widget(refxml, "aboutdialog", m_aboutdialog);
	if (m_buttonok)
		m_buttonok->signal_clicked().connect(sigc::bind(sigc::mem_fun(*this, &SplashWindow::button_clicked), true));
	if (m_buttoncancel)
		m_buttoncancel->signal_clicked().connect(sigc::bind(sigc::mem_fun(*this, &SplashWindow::button_clicked), false));
	if (m_buttonabout)
		m_buttonabout->signal_clicked().connect(sigc::mem_fun(*this, &SplashWindow::button_about_clicked));
	if (m_aboutdialog) {
		m_aboutdialog->hide();
		m_aboutdialog->signal_response().connect(sigc::mem_fun(*this, &SplashWindow::aboutdialog_response));
	}
	m_cfgstore = Gtk::ListStore::create(m_cfgcolumns);
	m_cfgstore->set_sort_column(m_cfgcolumns.m_col_name, Gtk::SORT_ASCENDING);
	if (m_treeviewcfg) {
		m_treeviewcfg->set_model(m_cfgstore);
		Gtk::CellRendererText *name_renderer = Gtk::manage(new Gtk::CellRendererText());
		int name_col = m_treeviewcfg->append_column(_("Name"), *name_renderer) - 1;
		Gtk::CellRendererText *desc_renderer = Gtk::manage(new Gtk::CellRendererText());
		int desc_col = m_treeviewcfg->append_column(_("Description"), *desc_renderer) - 1;
		{
			Gtk::TreeViewColumn *name_column = m_treeviewcfg->get_column(name_col);
			if (name_column) {
				name_column->add_attribute(*name_renderer, "text", m_cfgcolumns.m_col_name);
			}
		}
		{
			Gtk::TreeViewColumn *desc_column = m_treeviewcfg->get_column(desc_col);
			if (desc_column) {
				desc_column->add_attribute(*desc_renderer, "text", m_cfgcolumns.m_col_description);
			}
		}
		{
			double sz(name_renderer->property_size_points());
			if (sz < 24.0)
				name_renderer->property_size_points() = 24;
		}
		{
			double sz(desc_renderer->property_size_points());
			if (sz < 24.0)
				desc_renderer->property_size_points() = 24;
		}
		m_treeviewcfg->set_search_column(name_col);
		Glib::RefPtr<Gtk::TreeSelection> sel(m_treeviewcfg->get_selection());
		sel->set_mode(Gtk::SELECTION_SINGLE);
		sel->unselect_all();
		sel->signal_changed().connect(sigc::mem_fun(*this, &SplashWindow::selection_changed));
		selection_changed();
	}
}

FlightDeckWindow::SplashWindow::~SplashWindow()
{
}

void FlightDeckWindow::SplashWindow::button_about_clicked(void)
{
	m_aboutdialog->present();
}

void FlightDeckWindow::SplashWindow::aboutdialog_response(int response)
{
	if (!m_aboutdialog)
		return;
        if (response == Gtk::RESPONSE_CLOSE || response == Gtk::RESPONSE_CANCEL)
                m_aboutdialog->hide();
}

void FlightDeckWindow::SplashWindow::selection_changed(void)
{
	Glib::RefPtr<Gtk::TreeSelection> sel(m_treeviewcfg->get_selection());
	m_buttonok->set_sensitive(sel && sel->count_selected_rows() == 1);
}

void FlightDeckWindow::SplashWindow::button_clicked(bool ok)
{
	Glib::RefPtr<Gtk::TreeSelection> sel(m_treeviewcfg->get_selection());
	if (ok && sel) {
		Gtk::TreeModel::iterator it(sel->get_selected());
		if (it) {
			m_result = (Glib::ustring)(*it)[m_cfgcolumns.m_col_filename];
		}
	}
	m_cfgstore->clear();	
	hide();
}

template<typename T> void FlightDeckWindow::SplashWindow::set_configs(T b, T e)
{
	m_result.clear();
	m_cfgstore->clear();
	for (; b != e; ++b) {
		Gtk::TreeModel::Row row(*(m_cfgstore->append()));
		row[m_cfgcolumns.m_col_name] = b->get_name();
		row[m_cfgcolumns.m_col_description] = b->get_description();
		row[m_cfgcolumns.m_col_filename] = b->get_filename();
	}
}

template void FlightDeckWindow::SplashWindow::set_configs(std::set<Sensors::ConfigFile>::const_iterator b, std::set<Sensors::ConfigFile>::const_iterator e);
