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

#include <sstream>
#include <iomanip>

#include <gtkmm/main.h>
#include <glibmm/optionentry.h>
#include <glibmm/optiongroup.h>
#include <glibmm/optioncontext.h>

#include <libxml/encoding.h>
#include <libxml/xmlwriter.h>

#include "dbeditor.h"
#include "sysdepsgui.h"

TrackEditor::FindWindow::NameIcaoModelColumns::NameIcaoModelColumns(void )
{
	add(m_col_selectable);
	add(m_col_icao);
	add(m_col_name);
	add(m_col_dist);
}

TrackEditor::FindWindow::FindWindow(BaseObjectType * castitem, const Glib::RefPtr< Builder > & refxml)
#ifdef HAVE_HILDONMM
	: Hildon::Window(castitem),
#else
	: Gtk::Window(castitem),
#endif
	  m_refxml(refxml), m_engine(0),
	  m_trackeditorfindsetoffblock(0), m_trackeditorfindsettakeoff(0), m_trackeditorfindsetlanding(0), m_btrackeditorfindsetonblock(0),
	  m_trackeditorfindoffblockcomp(0), m_trackeditorfindtakeoffcomp(0), m_trackeditorfindlandingcomp(0), m_trackeditorfindonblockcomp(0),
	  m_trackeditorfindonblock(0), m_trackeditorfindlanding(0), m_trackeditorfindtakeoff(0), m_trackeditorfindoffblock(0),
	  m_trackeditorfindtoname(0), m_trackeditorfindtoicao(0), m_trackeditorfindfromicao(0), m_trackeditorfindfromname(0),
	  m_trackeditorfindtobox(0), m_trackeditorfindfrombox(0), m_trackeditorfindbcancel(0), m_trackeditorfindbok(0)
{
	get_widget(refxml, "trackeditorfindsetoffblock", m_trackeditorfindsetoffblock);
	get_widget(refxml, "trackeditorfindsettakeoff", m_trackeditorfindsettakeoff);
	get_widget(refxml, "trackeditorfindsetlanding", m_trackeditorfindsetlanding);
	get_widget(refxml, "btrackeditorfindsetonblock", m_btrackeditorfindsetonblock);
	get_widget(refxml, "trackeditorfindoffblockcomp", m_trackeditorfindoffblockcomp);
	get_widget(refxml, "trackeditorfindtakeoffcomp", m_trackeditorfindtakeoffcomp);
	get_widget(refxml, "trackeditorfindlandingcomp", m_trackeditorfindlandingcomp);
	get_widget(refxml, "trackeditorfindonblockcomp", m_trackeditorfindonblockcomp);
	get_widget(refxml, "trackeditorfindonblock", m_trackeditorfindonblock);
	get_widget(refxml, "trackeditorfindlanding", m_trackeditorfindlanding);
	get_widget(refxml, "trackeditorfindtakeoff", m_trackeditorfindtakeoff);
	get_widget(refxml, "trackeditorfindoffblock", m_trackeditorfindoffblock);
	get_widget(refxml, "trackeditorfindtoname", m_trackeditorfindtoname);
	get_widget(refxml, "trackeditorfindtoicao", m_trackeditorfindtoicao);
	get_widget(refxml, "trackeditorfindfromicao", m_trackeditorfindfromicao);
	get_widget(refxml, "trackeditorfindfromname", m_trackeditorfindfromname);
	get_widget(refxml, "trackeditorfindtobox", m_trackeditorfindtobox);
	get_widget(refxml, "trackeditorfindfrombox", m_trackeditorfindfrombox);
	get_widget(refxml, "trackeditorfindbcancel", m_trackeditorfindbcancel);
	get_widget(refxml, "trackeditorfindbok", m_trackeditorfindbok);
	signal_hide().connect(sigc::mem_fun(*this, &TrackEditor::FindWindow::on_my_hide));
	signal_delete_event().connect(sigc::mem_fun(*this, &TrackEditor::FindWindow::on_my_delete_event));
#ifdef HAVE_HILDONMM
	Hildon::Program::get_instance()->add_window(*this);
#endif
	m_querydispatch.connect(sigc::mem_fun(*this, &TrackEditor::FindWindow::async_done));
	m_trackeditorfindbok->signal_clicked().connect(sigc::mem_fun(*this, &TrackEditor::FindWindow::buttonok_clicked));
	m_trackeditorfindbcancel->signal_clicked().connect(sigc::mem_fun(*this, &TrackEditor::FindWindow::buttoncancel_clicked));
	m_trackeditorfindsetoffblock->signal_clicked().connect(sigc::mem_fun(*this, &TrackEditor::FindWindow::set_offblock));
	m_trackeditorfindsettakeoff->signal_clicked().connect(sigc::mem_fun(*this, &TrackEditor::FindWindow::set_takeoff));
	m_trackeditorfindsetlanding->signal_clicked().connect(sigc::mem_fun(*this, &TrackEditor::FindWindow::set_landing));
	m_btrackeditorfindsetonblock->signal_clicked().connect(sigc::mem_fun(*this, &TrackEditor::FindWindow::set_onblock));
	m_trackeditorfindtakeoff->signal_focus_out_event().connect(sigc::hide(sigc::bind_return(sigc::bind(sigc::ptr_fun(&TrackEditor::FindWindow::time_changed), m_trackeditorfindtakeoff), true)));
	m_trackeditorfindoffblock->signal_focus_out_event().connect(sigc::hide(sigc::bind_return(sigc::bind(sigc::ptr_fun(&TrackEditor::FindWindow::time_changed), m_trackeditorfindoffblock), true)));
	m_trackeditorfindlanding->signal_focus_out_event().connect(sigc::hide(sigc::bind_return(sigc::bind(sigc::ptr_fun(&TrackEditor::FindWindow::time_changed), m_trackeditorfindlanding), true)));
	m_trackeditorfindonblock->signal_focus_out_event().connect(sigc::hide(sigc::bind_return(sigc::bind(sigc::ptr_fun(&TrackEditor::FindWindow::time_changed), m_trackeditorfindonblock), true)));
	m_nameicaomodelstorefrom = Gtk::TreeStore::create(m_nameicaomodelcolumns);
	m_nameicaomodelstoreto = Gtk::TreeStore::create(m_nameicaomodelcolumns);
	m_trackeditorfindfrombox->set_model(m_nameicaomodelstorefrom);
	m_trackeditorfindfrombox->pack_start(m_nameicaomodelcolumns.m_col_name);
	m_trackeditorfindfrombox->pack_start(m_nameicaomodelcolumns.m_col_icao);
	m_trackeditorfindfrombox->pack_start(m_nameicaomodelcolumns.m_col_dist);
	m_trackeditorfindfrombox->signal_changed().connect(sigc::mem_fun(*this, &TrackEditor::FindWindow::findfrom_changed));
	m_trackeditorfindtobox->set_model(m_nameicaomodelstoreto);
	m_trackeditorfindtobox->pack_start(m_nameicaomodelcolumns.m_col_name);
	m_trackeditorfindtobox->pack_start(m_nameicaomodelcolumns.m_col_icao);
	m_trackeditorfindtobox->pack_start(m_nameicaomodelcolumns.m_col_dist);
	m_trackeditorfindtobox->signal_changed().connect(sigc::mem_fun(*this, &TrackEditor::FindWindow::findto_changed));
	typedef void (Gtk::ComboBox::*set_active_t)(int);
	m_trackeditorfindfromname->signal_changed().connect(sigc::bind(sigc::mem_fun(*m_trackeditorfindfrombox, (set_active_t)&Gtk::ComboBox::set_active), -1));
	m_trackeditorfindfromicao->signal_changed().connect(sigc::bind(sigc::mem_fun(*m_trackeditorfindfrombox, (set_active_t)&Gtk::ComboBox::set_active), -1));
	m_trackeditorfindtoname->signal_changed().connect(sigc::bind(sigc::mem_fun(*m_trackeditorfindtobox, (set_active_t)&Gtk::ComboBox::set_active), -1));
	m_trackeditorfindtoicao->signal_changed().connect(sigc::bind(sigc::mem_fun(*m_trackeditorfindtobox, (set_active_t)&Gtk::ComboBox::set_active), -1));
}

TrackEditor::FindWindow::~FindWindow()
{
#ifdef HAVE_HILDONMM
	Hildon::Program::get_instance()->remove_window(*this);
#endif
	async_cancel();
}

void TrackEditor::FindWindow::on_my_hide(void)
{
	async_cancel();
}

bool TrackEditor::FindWindow::on_my_delete_event(GdkEventAny * event)
{
	async_cancel();
	hide();
	return true;
}

void TrackEditor::FindWindow::buttonok_clicked(void)
{
	async_cancel();
	m_track.set_fromicao(m_trackeditorfindfromicao->get_text());
	m_track.set_fromname(m_trackeditorfindfromname->get_text());
	m_track.set_toicao(m_trackeditorfindtoicao->get_text());
	m_track.set_toname(m_trackeditorfindtoname->get_text());
	m_track.set_offblocktime(Conversions::time_parse(m_trackeditorfindoffblock->get_text()));
	m_track.set_takeofftime(Conversions::time_parse(m_trackeditorfindtakeoff->get_text()));
	m_track.set_landingtime(Conversions::time_parse(m_trackeditorfindlanding->get_text()));
	m_track.set_onblocktime(Conversions::time_parse(m_trackeditorfindonblock->get_text()));
	m_signal_save(m_track);
	hide();
}

void TrackEditor::FindWindow::buttoncancel_clicked(void)
{
	async_cancel();
	hide();
}

void TrackEditor::FindWindow::findfrom_changed(void)
{
	Gtk::TreeModel::iterator iter(m_trackeditorfindfrombox->get_active());
	if (!iter)
		return;
	Gtk::TreeModel::Row row(*iter);
	if (!row[m_nameicaomodelcolumns.m_col_selectable])
		return;
	m_trackeditorfindfromname->set_text(row[m_nameicaomodelcolumns.m_col_name]);
	m_trackeditorfindfromicao->set_text(row[m_nameicaomodelcolumns.m_col_icao]);
}

void TrackEditor::FindWindow::findto_changed(void)
{
	Gtk::TreeModel::iterator iter(m_trackeditorfindtobox->get_active());
	if (!iter)
		return;
	Gtk::TreeModel::Row row(*iter);
	if (!row[m_nameicaomodelcolumns.m_col_selectable])
		return;
	m_trackeditorfindtoname->set_text(row[m_nameicaomodelcolumns.m_col_name]);
	m_trackeditorfindtoicao->set_text(row[m_nameicaomodelcolumns.m_col_icao]);
}

void TrackEditor::FindWindow::time_changed(Gtk::Entry * entry)
{
	entry->set_text(Conversions::time_str("%Y-%m-%d %H:%M:%S", Conversions::time_parse(entry->get_text())));
}

void TrackEditor::FindWindow::set_offblock(void)
{
	m_trackeditorfindoffblock->set_text(m_trackeditorfindoffblockcomp->get_text());
}

void TrackEditor::FindWindow::set_takeoff(void)
{
	m_trackeditorfindtakeoff->set_text(m_trackeditorfindtakeoffcomp->get_text());
}

void TrackEditor::FindWindow::set_landing(void)
{
	m_trackeditorfindlanding->set_text(m_trackeditorfindlandingcomp->get_text());
}

void TrackEditor::FindWindow::set_onblock(void)
{
	m_trackeditorfindonblock->set_text(m_trackeditorfindonblockcomp->get_text());
}

void TrackEditor::FindWindow::async_dispatchdone( void )
{
	m_querydispatch();
}

void TrackEditor::FindWindow::async_done( void )
{
	Point coordfrom(m_track[0].get_coord());
	Point coordto(m_track[m_track.size()-1].get_coord());
	if (m_airportqueryfrom && m_airportqueryfrom->is_done()) {
		if (!m_airportqueryfrom->is_error()) {
			Engine::AirportResult::elementvector_t& ev(m_airportqueryfrom->get_result());
			Gtk::TreeModel::Row prow(*m_nameicaomodelstorefrom->append());
			prow[m_nameicaomodelcolumns.m_col_name] = "Airports";
			prow[m_nameicaomodelcolumns.m_col_selectable] = false;
			for (Engine::AirportResult::elementvector_t::const_iterator ei(ev.begin()), ee(ev.end()); ei != ee; ei++) {
				Gtk::TreeModel::Row row(*m_nameicaomodelstorefrom->append(prow.children()));
				row[m_nameicaomodelcolumns.m_col_selectable] = true;
				row[m_nameicaomodelcolumns.m_col_name] = ei->get_name();
				row[m_nameicaomodelcolumns.m_col_icao] = ei->get_icao();
				row[m_nameicaomodelcolumns.m_col_dist] = Conversions::dist_str(coordfrom.spheric_distance_nmi(ei->get_coord()));
			}
			m_nameicaomodelstorefrom->set_sort_column(m_nameicaomodelcolumns.m_col_dist, Gtk::SORT_ASCENDING);
		}
		m_airportqueryfrom = Glib::RefPtr<Engine::AirportResult>();
	}
	if (m_airportqueryto && m_airportqueryto->is_done()) {
		if (!m_airportqueryto->is_error()) {
			Engine::AirportResult::elementvector_t& ev(m_airportqueryto->get_result());
			Gtk::TreeModel::Row prow(*m_nameicaomodelstoreto->append());
			prow[m_nameicaomodelcolumns.m_col_name] = "Airports";
			prow[m_nameicaomodelcolumns.m_col_selectable] = false;
			for (Engine::AirportResult::elementvector_t::const_iterator ei(ev.begin()), ee(ev.end()); ei != ee; ei++) {
				Gtk::TreeModel::Row row(*m_nameicaomodelstoreto->append(prow.children()));
				row[m_nameicaomodelcolumns.m_col_selectable] = true;
				row[m_nameicaomodelcolumns.m_col_name] = ei->get_name();
				row[m_nameicaomodelcolumns.m_col_icao] = ei->get_icao();
				row[m_nameicaomodelcolumns.m_col_dist] = Conversions::dist_str(coordto.spheric_distance_nmi(ei->get_coord()));
			}
			m_nameicaomodelstoreto->set_sort_column(m_nameicaomodelcolumns.m_col_dist, Gtk::SORT_ASCENDING);
		}
		m_airportqueryto = Glib::RefPtr<Engine::AirportResult>();
	}
	if (m_navaidqueryfrom && m_navaidqueryfrom->is_done()) {
		if (!m_navaidqueryfrom->is_error()) {
			Engine::NavaidResult::elementvector_t& ev(m_navaidqueryfrom->get_result());
			Gtk::TreeModel::Row prow(*m_nameicaomodelstorefrom->append());
			prow[m_nameicaomodelcolumns.m_col_name] = "Navaids";
			prow[m_nameicaomodelcolumns.m_col_selectable] = false;
			for (Engine::NavaidResult::elementvector_t::const_iterator ei(ev.begin()), ee(ev.end()); ei != ee; ei++) {
				Gtk::TreeModel::Row row(*m_nameicaomodelstorefrom->append(prow.children()));
				row[m_nameicaomodelcolumns.m_col_selectable] = true;
				row[m_nameicaomodelcolumns.m_col_name] = ei->get_name();
				row[m_nameicaomodelcolumns.m_col_icao] = ei->get_icao();
				row[m_nameicaomodelcolumns.m_col_dist] = Conversions::dist_str(coordfrom.spheric_distance_nmi(ei->get_coord()));
			}
			m_nameicaomodelstorefrom->set_sort_column(m_nameicaomodelcolumns.m_col_dist, Gtk::SORT_ASCENDING);
		}
		m_navaidqueryfrom = Glib::RefPtr<Engine::NavaidResult>();
	}
	if (m_navaidqueryto && m_navaidqueryto->is_done()) {
		if (!m_navaidqueryto->is_error()) {
			Engine::NavaidResult::elementvector_t& ev(m_navaidqueryto->get_result());
			Gtk::TreeModel::Row prow(*m_nameicaomodelstoreto->append());
			prow[m_nameicaomodelcolumns.m_col_name] = "Navaids";
			prow[m_nameicaomodelcolumns.m_col_selectable] = false;
			for (Engine::NavaidResult::elementvector_t::const_iterator ei(ev.begin()), ee(ev.end()); ei != ee; ei++) {
				Gtk::TreeModel::Row row(*m_nameicaomodelstoreto->append(prow.children()));
				row[m_nameicaomodelcolumns.m_col_selectable] = true;
				row[m_nameicaomodelcolumns.m_col_name] = ei->get_name();
				row[m_nameicaomodelcolumns.m_col_icao] = ei->get_icao();
				row[m_nameicaomodelcolumns.m_col_dist] = Conversions::dist_str(coordto.spheric_distance_nmi(ei->get_coord()));
			}
			m_nameicaomodelstoreto->set_sort_column(m_nameicaomodelcolumns.m_col_dist, Gtk::SORT_ASCENDING);
		}
		m_navaidqueryto = Glib::RefPtr<Engine::NavaidResult>();
	}
	if (m_waypointqueryfrom && m_waypointqueryfrom->is_done()) {
		if (!m_waypointqueryfrom->is_error()) {
			Engine::WaypointResult::elementvector_t& ev(m_waypointqueryfrom->get_result());
			Gtk::TreeModel::Row prow(*m_nameicaomodelstorefrom->append());
			prow[m_nameicaomodelcolumns.m_col_name] = "Waypoints";
			prow[m_nameicaomodelcolumns.m_col_selectable] = false;
			for (Engine::WaypointResult::elementvector_t::const_iterator ei(ev.begin()), ee(ev.end()); ei != ee; ei++) {
				Gtk::TreeModel::Row row(*m_nameicaomodelstorefrom->append(prow.children()));
				row[m_nameicaomodelcolumns.m_col_selectable] = true;
				row[m_nameicaomodelcolumns.m_col_name] = ei->get_name();
				row[m_nameicaomodelcolumns.m_col_icao] = ei->get_icao();
				row[m_nameicaomodelcolumns.m_col_dist] = Conversions::dist_str(coordfrom.spheric_distance_nmi(ei->get_coord()));
			}
			m_nameicaomodelstorefrom->set_sort_column(m_nameicaomodelcolumns.m_col_dist, Gtk::SORT_ASCENDING);
		}
		m_waypointqueryfrom = Glib::RefPtr<Engine::WaypointResult>();
	}
	if (m_waypointqueryto && m_waypointqueryto->is_done()) {
		if (!m_waypointqueryto->is_error()) {
			Engine::WaypointResult::elementvector_t& ev(m_waypointqueryto->get_result());
			Gtk::TreeModel::Row prow(*m_nameicaomodelstoreto->append());
			prow[m_nameicaomodelcolumns.m_col_name] = "Waypoints";
			prow[m_nameicaomodelcolumns.m_col_selectable] = false;
			for (Engine::WaypointResult::elementvector_t::const_iterator ei(ev.begin()), ee(ev.end()); ei != ee; ei++) {
				Gtk::TreeModel::Row row(*m_nameicaomodelstoreto->append(prow.children()));
				row[m_nameicaomodelcolumns.m_col_selectable] = true;
				row[m_nameicaomodelcolumns.m_col_name] = ei->get_name();
				row[m_nameicaomodelcolumns.m_col_icao] = ei->get_icao();
				row[m_nameicaomodelcolumns.m_col_dist] = Conversions::dist_str(coordto.spheric_distance_nmi(ei->get_coord()));
			}
			m_nameicaomodelstoreto->set_sort_column(m_nameicaomodelcolumns.m_col_dist, Gtk::SORT_ASCENDING);
		}
		m_waypointqueryto = Glib::RefPtr<Engine::WaypointResult>();
	}
	if (m_mapelementqueryfrom && m_mapelementqueryfrom->is_done()) {
		if (!m_mapelementqueryfrom->is_error()) {
			Engine::MapelementResult::elementvector_t& ev(m_mapelementqueryfrom->get_result());
			Gtk::TreeModel::Row prow(*m_nameicaomodelstorefrom->append());
			prow[m_nameicaomodelcolumns.m_col_name] = "Map Elements";
			prow[m_nameicaomodelcolumns.m_col_selectable] = false;
			unsigned int nr(0);
			for (Engine::MapelementResult::elementvector_t::const_iterator ei(ev.begin()), ee(ev.end()); ei != ee; ei++) {
				if (ei->get_name().empty())
					continue;
				Gtk::TreeModel::Row row(*m_nameicaomodelstorefrom->append(prow.children()));
				row[m_nameicaomodelcolumns.m_col_selectable] = true;
				row[m_nameicaomodelcolumns.m_col_name] = ei->get_name();
				row[m_nameicaomodelcolumns.m_col_icao] = "";
				row[m_nameicaomodelcolumns.m_col_dist] = Conversions::dist_str(coordfrom.spheric_distance_nmi(ei->get_coord()));
				nr++;
				if (nr >= max_number)
					break;
			}
			m_nameicaomodelstorefrom->set_sort_column(m_nameicaomodelcolumns.m_col_dist, Gtk::SORT_ASCENDING);
		}
		m_mapelementqueryfrom = Glib::RefPtr<Engine::MapelementResult>();
	}
	if (m_mapelementqueryto && m_mapelementqueryto->is_done()) {
		if (!m_mapelementqueryto->is_error()) {
			Engine::MapelementResult::elementvector_t& ev(m_mapelementqueryto->get_result());
			Gtk::TreeModel::Row prow(*m_nameicaomodelstoreto->append());
			prow[m_nameicaomodelcolumns.m_col_name] = "Map Elements";
			prow[m_nameicaomodelcolumns.m_col_selectable] = false;
			unsigned int nr(0);
			for (Engine::MapelementResult::elementvector_t::const_iterator ei(ev.begin()), ee(ev.end()); ei != ee; ei++) {
				if (ei->get_name().empty())
					continue;
				Gtk::TreeModel::Row row(*m_nameicaomodelstoreto->append(prow.children()));
				row[m_nameicaomodelcolumns.m_col_selectable] = true;
				row[m_nameicaomodelcolumns.m_col_name] = ei->get_name();
				row[m_nameicaomodelcolumns.m_col_icao] = "";
				row[m_nameicaomodelcolumns.m_col_dist] = Conversions::dist_str(coordto.spheric_distance_nmi(ei->get_coord()));
				nr++;
				if (nr >= max_number)
					break;
			}
			m_nameicaomodelstoreto->set_sort_column(m_nameicaomodelcolumns.m_col_dist, Gtk::SORT_ASCENDING);
		}
		m_mapelementqueryto = Glib::RefPtr<Engine::MapelementResult>();
	}
}

void TrackEditor::FindWindow::async_cancel(void)
{
	Engine::AirportResult::cancel(m_airportqueryfrom);
	Engine::AirportResult::cancel(m_airportqueryto);
	Engine::NavaidResult::cancel(m_navaidqueryfrom);
	Engine::NavaidResult::cancel(m_navaidqueryto);
	Engine::WaypointResult::cancel(m_waypointqueryfrom);
	Engine::WaypointResult::cancel(m_waypointqueryto);
	Engine::MapelementResult::cancel(m_mapelementqueryfrom);
	Engine::MapelementResult::cancel(m_mapelementqueryto);
	m_nameicaomodelstorefrom->clear();
	m_nameicaomodelstoreto->clear();
}

void TrackEditor::FindWindow::set_engine(Engine & eng)
{
	m_engine = &eng;
}

void TrackEditor::FindWindow::present(const TracksDb::Track & track)
{
	m_track = track;
	m_nameicaomodelstorefrom->clear();
	m_nameicaomodelstoreto->clear();
	m_trackeditorfindoffblockcomp->set_text(Conversions::time_str("%Y-%m-%d %H:%M:%S", track.get_offblocktime()));
	m_trackeditorfindoffblock->set_text(Conversions::time_str("%Y-%m-%d %H:%M:%S", track.get_offblocktime()));
	m_trackeditorfindtakeoffcomp->set_text(Conversions::time_str("%Y-%m-%d %H:%M:%S", track.get_takeofftime()));
	m_trackeditorfindtakeoff->set_text(Conversions::time_str("%Y-%m-%d %H:%M:%S", track.get_takeofftime()));
	m_trackeditorfindlandingcomp->set_text(Conversions::time_str("%Y-%m-%d %H:%M:%S", track.get_landingtime()));
	m_trackeditorfindlanding->set_text(Conversions::time_str("%Y-%m-%d %H:%M:%S", track.get_landingtime()));
	m_trackeditorfindonblockcomp->set_text(Conversions::time_str("%Y-%m-%d %H:%M:%S", track.get_onblocktime()));
	m_trackeditorfindonblock->set_text(Conversions::time_str("%Y-%m-%d %H:%M:%S", track.get_onblocktime()));
	m_trackeditorfindfromicao->set_text(track.get_fromicao());
	m_trackeditorfindfromname->set_text(track.get_fromname());
	m_trackeditorfindtoicao->set_text(track.get_toicao());
	m_trackeditorfindtoname->set_text(track.get_toname());
	if (!m_engine)
		return;
	if (m_track.size() < 1)
		return;
	{
		Point coordfrom(m_track[0].get_coord());
		Point coordto(m_track[m_track.size()-1].get_coord());
		Rect bboxfrom(coordfrom.simple_box_nmi(max_distance));
		Rect bboxto(coordto.simple_box_nmi(max_distance));
		async_cancel();
		m_airportqueryfrom = m_engine->async_airport_find_nearest(coordfrom, max_number, bboxfrom);
		m_airportqueryto = m_engine->async_airport_find_nearest(coordto, max_number, bboxto);
		m_navaidqueryfrom = m_engine->async_navaid_find_nearest(coordfrom, max_number, bboxfrom);
		m_navaidqueryto = m_engine->async_navaid_find_nearest(coordto, max_number, bboxto);
		m_waypointqueryfrom = m_engine->async_waypoint_find_nearest(coordfrom, max_number, bboxfrom);
		m_waypointqueryto = m_engine->async_waypoint_find_nearest(coordto, max_number, bboxto);
		m_mapelementqueryfrom = m_engine->async_mapelement_find_nearest(coordfrom, 8 * max_number, bboxfrom);
		m_mapelementqueryto = m_engine->async_mapelement_find_nearest(coordto, 8 * max_number, bboxto);
		if (m_airportqueryfrom)
			m_airportqueryfrom->connect(sigc::mem_fun(*this, &TrackEditor::FindWindow::async_dispatchdone));
		if (m_airportqueryto)
			m_airportqueryto->connect(sigc::mem_fun(*this, &TrackEditor::FindWindow::async_dispatchdone));
		if (m_navaidqueryfrom)
			m_navaidqueryfrom->connect(sigc::mem_fun(*this, &TrackEditor::FindWindow::async_dispatchdone));
		if (m_navaidqueryto)
			m_navaidqueryto->connect(sigc::mem_fun(*this, &TrackEditor::FindWindow::async_dispatchdone));
		if (m_waypointqueryfrom)
			m_waypointqueryfrom->connect(sigc::mem_fun(*this, &TrackEditor::FindWindow::async_dispatchdone));
		if (m_waypointqueryto)
			m_waypointqueryto->connect(sigc::mem_fun(*this, &TrackEditor::FindWindow::async_dispatchdone));
		if (m_mapelementqueryfrom)
			m_mapelementqueryfrom->connect(sigc::mem_fun(*this, &TrackEditor::FindWindow::async_dispatchdone));
		if (m_mapelementqueryto)
			m_mapelementqueryto->connect(sigc::mem_fun(*this, &TrackEditor::FindWindow::async_dispatchdone));
	}
	compute_offblock();
	compute_takeoff();
	compute_landing();
	compute_onblock();
	{
		time_t offblock(Conversions::time_parse(m_trackeditorfindoffblockcomp->get_text()));
		time_t takeoff(Conversions::time_parse(m_trackeditorfindtakeoffcomp->get_text()));
		time_t landing(Conversions::time_parse(m_trackeditorfindlandingcomp->get_text()));
		time_t onblock(Conversions::time_parse(m_trackeditorfindonblockcomp->get_text()));
		takeoff = std::max(offblock, std::min(onblock, takeoff));
		landing = std::min(onblock, std::max(offblock, landing));
		m_trackeditorfindtakeoffcomp->set_text(Conversions::time_str("%Y-%m-%d %H:%M:%S", takeoff));
		m_trackeditorfindlandingcomp->set_text(Conversions::time_str("%Y-%m-%d %H:%M:%S", landing));
	}
	toplevel_window_t::present();
}

void TrackEditor::FindWindow::compute_offblock(void)
{
	if (m_track.size() < 1)
		return;
	Rect bbox(m_track[0].get_coord().simple_box_nmi(block_distance));
	for (unsigned int i = 1; i < m_track.size(); i++) {
		if (bbox.is_inside(m_track[i].get_coord()))
		    continue;
		m_trackeditorfindoffblockcomp->set_text(Conversions::time_str("%Y-%m-%d %H:%M:%S", m_track[i].get_time()));
		return;
	}
}

void TrackEditor::FindWindow::compute_takeoff(void)
{
	if (!m_engine)
		return;
	float vblock(m_engine->get_prefs().vblock);
	vblock *= (1.0 / (256.0 * 60.0 * 60.0));
	for (unsigned int i = 1; i < m_track.size(); i++) {
		int64_t tm(m_track[i].get_timefrac8() - m_track[i-1].get_timefrac8());
		if (tm <= 0)
			continue;
		float d(m_track[i-1].get_coord().spheric_distance_nmi(m_track[i].get_coord()));
		d /= tm;
		if (d <= vblock)
		    continue;
		m_trackeditorfindtakeoffcomp->set_text(Conversions::time_str("%Y-%m-%d %H:%M:%S", m_track[i].get_time()));
		return;
	}
}

void TrackEditor::FindWindow::compute_landing(void)
{
	if (!m_engine)
		return;
	float vblock(m_engine->get_prefs().vblock);
	vblock *= (1.0 / (256.0 * 60.0 * 60.0));
	for (unsigned int i = m_track.size(); i > 1; i--) {
		int64_t tm(m_track[i-1].get_timefrac8() - m_track[i-2].get_timefrac8());
		if (tm <= 0)
			continue;
		float d(m_track[i-2].get_coord().spheric_distance_nmi(m_track[i-1].get_coord()));
		d /= tm;
		if (d <= vblock)
		    continue;
		m_trackeditorfindlandingcomp->set_text(Conversions::time_str("%Y-%m-%d %H:%M:%S", m_track[i-2].get_time()));
		return;
	}
}

void TrackEditor::FindWindow::compute_onblock(void)
{
	if (m_track.size() < 1)
		return;
	Rect bbox(m_track[m_track.size()-1].get_coord().simple_box_nmi(block_distance));
	for (unsigned int i = m_track.size()-1; i > 0; i--) {
		if (bbox.is_inside(m_track[i-1].get_coord()))
		    continue;
		m_trackeditorfindonblockcomp->set_text(Conversions::time_str("%Y-%m-%d %H:%M:%S", m_track[i-1].get_time()));
		return;
	}
}

class TrackEditor::KMLExporter {
	public:
		KMLExporter(void);
		KMLExporter(const Glib::ustring& filename, bool compress = true);
		~KMLExporter();
		void open(const Glib::ustring& filename, bool compress = true);
		void close(void);
		void write(const TracksDb::Track& e);

	protected:
		xmlTextWriterPtr m_writer;

		void startelement(const Glib::ustring& name);
		void endelement(void);
		void attr(const Glib::ustring& name, const Glib::ustring& val);
		void data(const Glib::ustring& s);
};

TrackEditor::KMLExporter::KMLExporter(void )
	: m_writer(0)
{
}

TrackEditor::KMLExporter::KMLExporter(const Glib::ustring & filename, bool compress)
	: m_writer(0)
{
	open(filename, compress);
}

TrackEditor::KMLExporter::~KMLExporter()
{
	close();
}

void TrackEditor::KMLExporter::open(const Glib::ustring & filename, bool compress)
{
	close();
	m_writer = xmlNewTextWriterFilename(filename.c_str(), compress);
	int rc = xmlTextWriterStartDocument(m_writer, "1.0", "UTF-8", NULL);
	if (rc < 0)
		throw std::runtime_error("XmlWriter: Error at xmlTextWriterStartDocument");
	rc = xmlTextWriterStartElementNS(m_writer, NULL, (const xmlChar *)"kml", (const xmlChar *)"http://earth.google.com/kml/2.2");
	if (rc < 0)
		throw std::runtime_error("XmlWriter: Error at xmlTextWriterStartElementNS");
	rc = xmlTextWriterSetIndent(m_writer, 1);
	if (rc < 0)
		throw std::runtime_error("XmlWriter: Error at xmlTextWriterSetIndent");
	startelement("Document");
	startelement("Style");
	attr("id", "vfrnavstyle");
	startelement("LineStyle");
	startelement("color");
	data("ff0000ff");
	endelement();
	startelement("width");
	data("15");
	endelement();
	endelement();
	startelement("PolyStyle");
	startelement("color");
	data("ff0000ff");
	endelement();
	endelement();
	endelement();
}

void TrackEditor::KMLExporter::close(void)
{
	if (!m_writer)
		return;
	endelement();
	int rc = xmlTextWriterEndElement(m_writer);
	if (rc < 0)
		throw std::runtime_error("XmlWriter: Error at xmlTextWriterEndElement");
	rc = xmlTextWriterEndDocument(m_writer);
	if (rc < 0)
		throw std::runtime_error("XmlWriter: Error at xmlTextWriterEndDocument");
	xmlFreeTextWriter(m_writer);
	m_writer = 0;
}

void TrackEditor::KMLExporter::startelement(const Glib::ustring & name)
{
	int rc = xmlTextWriterStartElement(m_writer, (const xmlChar *)name.c_str());
	if (rc < 0)
		throw std::runtime_error("XmlWriter: Error at xmlTextWriterStartElement");
}

void TrackEditor::KMLExporter::endelement(void)
{
	int rc = xmlTextWriterEndElement(m_writer);
	if (rc < 0)
		throw std::runtime_error("XmlWriter: Error at xmlTextWriterEndElement");
}

void TrackEditor::KMLExporter::attr(const Glib::ustring & name, const Glib::ustring & val)
{
	int rc = xmlTextWriterWriteAttribute(m_writer, (const xmlChar *)name.c_str(), (const xmlChar *)val.c_str());
	if (rc < 0)
		throw std::runtime_error("XmlWriter: Error at xmlTextWriterWriteAttribute");
}

void TrackEditor::KMLExporter::data(const Glib::ustring & s)
{
	int rc = xmlTextWriterWriteString(m_writer, (const xmlChar *)s.c_str());
	if (rc < 0)
		throw std::runtime_error("XmlWriter: Error at xmlTextWriterWriteString");
}

void TrackEditor::KMLExporter::write(const TracksDb::Track & e)
{
	if (!e.is_valid())
		return;
	startelement("Placemark");
	startelement("name");
	data(e.get_fromicaoname() + " - " + e.get_toicaoname());
	endelement();
	startelement("styleUrl");
	data("#vfrnavstyle");
	endelement();
	if (!e.get_notes().empty()) {
		startelement("description");
		data(e.get_notes());
		endelement();
	}
	if (e.size() > 0) {
		std::ostringstream cs;
		cs << std::setprecision(10);
		for (unsigned int i = 0; i < e.size(); i++) {
			const TracksDb::Track::TrackPoint& tp(e[i]);
			cs << tp.get_coord().get_lon_deg_dbl() << ','
			   << tp.get_coord().get_lat_deg_dbl() << ',';
			if (tp.is_alt_valid())
				cs << (tp.get_alt() * Point::ft_to_m);
			else
				cs << 0;
			cs << std::endl;
		}
		startelement("LineString");
		startelement("extrude");
		data("1");
		endelement();
		startelement("tessellate");
		data("1");
		endelement();
		startelement("altitudeMode");
		data("absolute");
		endelement();
		startelement("coordinates");
		data(cs.str());
		endelement();
		endelement();
	}
	endelement();
}

TrackEditor::TrackModelColumns::TrackModelColumns(void)
{
	add(m_col_id);
	add(m_col_fromicao);
	add(m_col_fromname);
	add(m_col_toicao);
	add(m_col_toname);
	add(m_col_offblocktime);
	add(m_col_takeofftime);
	add(m_col_landingtime);
	add(m_col_onblocktime);
	add(m_col_sourceid);
	add(m_col_modtime);
	add(m_col_nrpoints);
	add(m_col_searchname);
}

TrackEditor::TrackPointModelColumns::TrackPointModelColumns(void)
{
	add(m_col_index);
	add(m_col_coord);
	add(m_col_alt);
	add(m_col_time);
}

TrackEditor::TrackEditor(const std::string & dir_main, Engine::auxdb_mode_t auxdbmode, const std::string & dir_aux)
	: m_dbchanged(false), m_engine(dir_main, auxdbmode, dir_aux, false, false), m_findwindow(0),
	  m_trackeditorwindow(0), m_trackeditortreeview(0), m_trackeditorstatusbar(0),
	  m_trackeditorfind(0), m_trackeditornotebook(0), m_trackeditornotes(0),
	  m_trackpointeditortreeview(0),
	  m_trackeditormap(0), m_trackeditorexportkml(0), m_trackeditorundo(0), m_trackeditorredo(0),
	  m_trackeditormapenable(0), m_trackeditorairportdiagramenable(0), m_trackeditormapzoomin(0), m_trackeditormapzoomout(0),
	  m_trackeditornewtrack(0), m_trackeditordeletetrack(0), m_trackeditorduplicatetrack(0), m_trackeditormergetracks(0),
	  m_trackeditorappendtrackpoint(0), m_trackeditorinserttrackpoint(0), m_trackeditordeletetrackpoint(0),
	  m_trackeditorselectall(0), m_trackeditorunselectall(0), m_aboutdialog(0),
	  m_curentryid(0)
{
#ifdef HAVE_PQXX
	if (auxdbmode == Engine::auxdb_postgres)
		m_db.reset(new TracksPGDb(dir_main));
	else
#endif
	{
		m_db.reset(new TracksDb(dir_main));
		std::cerr << "Main database " << (dir_main.empty() ? std::string("(default)") : dir_main) << std::endl;
		Glib::ustring dir_aux1(m_engine.get_aux_dir(auxdbmode, dir_aux));
		TracksDb *db(static_cast<TracksDb *>(m_db.get()));
		if (!dir_aux1.empty() && db->is_dbfile_exists(dir_aux1))
			db->attach(dir_aux1);
		if (db->is_aux_attached())
			std::cerr << "Auxillary tracks database " << dir_aux1 << " attached" << std::endl;
	}
	// Find Window
	m_refxml = load_glade_file("dbeditor" UIEXT, "trackeditorwindow");
	{
#ifdef HAVE_GTKMM2
		Glib::RefPtr<Builder> refxml = load_glade_file_nosearch(m_refxml->get_filename(), "trackeditorfindwindow");
#else
		//Glib::RefPtr<Builder> refxml = load_glade_file("dbeditor" UIEXT, "aboutdialog");
		Glib::RefPtr<Builder> refxml = m_refxml;
#endif
		get_widget_derived(refxml, "trackeditorfindwindow", m_findwindow);
		m_findwindow->set_engine(m_engine);
	}
	// Main Window
	get_widget_derived(m_refxml, "trackeditorwindow", m_trackeditorwindow);
	get_widget(m_refxml, "trackeditortreeview", m_trackeditortreeview);
	get_widget(m_refxml, "trackeditorstatusbar", m_trackeditorstatusbar);
	get_widget(m_refxml, "trackeditorfind", m_trackeditorfind);
	get_widget(m_refxml, "trackeditornotebook", m_trackeditornotebook);
	get_widget(m_refxml, "trackeditornotes", m_trackeditornotes);
	get_widget(m_refxml, "trackpointeditortreeview", m_trackpointeditortreeview);
	get_widget_derived(m_refxml, "trackeditormap", m_trackeditormap);
	get_widget(m_refxml, "trackpointeditorstatdist", m_trackpointeditorstatdist);
	get_widget(m_refxml, "trackpointeditorstattime", m_trackpointeditorstattime);
	get_widget(m_refxml, "trackpointeditorstatspeedavg", m_trackpointeditorstatspeedavg);
	get_widget(m_refxml, "trackpointeditorstatspeedmin", m_trackpointeditorstatspeedmin);
	get_widget(m_refxml, "trackpointeditorstatspeedmax", m_trackpointeditorstatspeedmax);
	get_widget(m_refxml, "trackpointeditorstataltmin", m_trackpointeditorstataltmin);
	get_widget(m_refxml, "trackpointeditorstataltmax", m_trackpointeditorstataltmax);
	{
#ifdef HAVE_GTKMM2
		Glib::RefPtr<Builder> refxml = load_glade_file_nosearch(m_refxml->get_filename(), "aboutdialog");
#else
		//Glib::RefPtr<Builder> refxml = load_glade_file("dbeditor" UIEXT, "aboutdialog");
		Glib::RefPtr<Builder> refxml = m_refxml;
#endif
		get_widget(refxml, "aboutdialog", m_aboutdialog);
		m_aboutdialog->signal_response().connect(sigc::mem_fun(*this, &TrackEditor::aboutdialog_response));
	}
	Gtk::Button *buttonclear(0);
	get_widget(m_refxml, "trackeditorclearbutton", buttonclear);
	buttonclear->signal_clicked().connect(sigc::mem_fun(*this, &TrackEditor::buttonclear_clicked));
	m_trackeditorfind->signal_changed().connect(sigc::mem_fun(*this, &TrackEditor::find_changed));
	Gtk::MenuItem *menu_quit(0), *menu_cut(0), *menu_copy(0), *menu_paste(0), *menu_delete(0);
	Gtk::MenuItem *menu_preferences(0), *menu_about(0);
	get_widget(m_refxml, "trackeditorexportkml", m_trackeditorexportkml);
	get_widget(m_refxml, "trackeditorquit", menu_quit);
	get_widget(m_refxml, "trackeditorundo", m_trackeditorundo);
	get_widget(m_refxml, "trackeditorredo", m_trackeditorredo);
	get_widget(m_refxml, "trackeditornewtrack", m_trackeditornewtrack);
	get_widget(m_refxml, "trackeditordeletetrack", m_trackeditordeletetrack);
	get_widget(m_refxml, "trackeditorduplicatetrack", m_trackeditorduplicatetrack);
	get_widget(m_refxml, "trackeditormergetracks", m_trackeditormergetracks);
	get_widget(m_refxml, "trackeditorcompute", m_trackeditorcompute);
	get_widget(m_refxml, "trackeditormapenable", m_trackeditormapenable);
	get_widget(m_refxml, "trackeditorairportdiagramenable", m_trackeditorairportdiagramenable);
	get_widget(m_refxml, "trackeditormapzoomin", m_trackeditormapzoomin);
	get_widget(m_refxml, "trackeditormapzoomout", m_trackeditormapzoomout);
	get_widget(m_refxml, "trackeditorappendtrackpoint", m_trackeditorappendtrackpoint);
	get_widget(m_refxml, "trackeditorinserttrackpoint", m_trackeditorinserttrackpoint);
	get_widget(m_refxml, "trackeditordeletetrackpoint", m_trackeditordeletetrackpoint);
	get_widget(m_refxml, "trackeditorcreatewaypoint", m_trackeditorcreatewaypoint);
	get_widget(m_refxml, "trackeditorcut", menu_cut);
	get_widget(m_refxml, "trackeditorcopy", menu_copy);
	get_widget(m_refxml, "trackeditorpaste", menu_paste);
	get_widget(m_refxml, "trackeditordelete", menu_delete);
	get_widget(m_refxml, "trackeditorselectall", m_trackeditorselectall);
	get_widget(m_refxml, "trackeditorunselectall", m_trackeditorunselectall);
	get_widget(m_refxml, "trackeditorpreferences", menu_preferences);
	get_widget(m_refxml, "trackeditorabout", menu_about);
	hide_notebook_pages();
	m_trackeditorexportkml->signal_activate().connect(sigc::mem_fun(*this, &TrackEditor::menu_exportkml_activate));
	menu_quit->signal_activate().connect(sigc::mem_fun(*this, &TrackEditor::menu_quit_activate));
	m_trackeditorundo->signal_activate().connect(sigc::mem_fun(*this, &TrackEditor::menu_undo_activate));
	m_trackeditorredo->signal_activate().connect(sigc::mem_fun(*this, &TrackEditor::menu_redo_activate));
	m_trackeditornewtrack->signal_activate().connect(sigc::mem_fun(*this, &TrackEditor::menu_newtrack_activate));
	m_trackeditordeletetrack->signal_activate().connect(sigc::mem_fun(*this, &TrackEditor::menu_deletetrack_activate));
	m_trackeditorduplicatetrack->signal_activate().connect(sigc::mem_fun(*this, &TrackEditor::menu_duplicatetrack_activate));
	m_trackeditormergetracks->signal_activate().connect(sigc::mem_fun(*this, &TrackEditor::menu_mergetracks_activate));
	m_trackeditormapenable->signal_toggled().connect(sigc::mem_fun(*this, &TrackEditor::menu_mapenable_toggled));
	m_trackeditorairportdiagramenable->signal_toggled().connect(sigc::mem_fun(*this, &TrackEditor::menu_airportdiagramenable_toggled));
	m_trackeditormapzoomin->signal_activate().connect(sigc::mem_fun(*this, &TrackEditor::menu_mapzoomin_activate));
	m_trackeditormapzoomout->signal_activate().connect(sigc::mem_fun(*this, &TrackEditor::menu_mapzoomout_activate));
	if (m_trackeditormapenable->get_active()) {
		m_trackeditormapzoomin->show();
		m_trackeditormapzoomout->show();
	} else {
		m_trackeditormapzoomin->hide();
		m_trackeditormapzoomout->hide();
	}
	m_trackeditorwindow->signal_zoomin().connect(sigc::bind_return(sigc::mem_fun(*this, &TrackEditor::menu_mapzoomin_activate), true));
	m_trackeditorwindow->signal_zoomout().connect(sigc::bind_return(sigc::mem_fun(*this, &TrackEditor::menu_mapzoomout_activate), true));
	menu_cut->signal_activate().connect(sigc::mem_fun(*this, &TrackEditor::menu_cut_activate));
	menu_copy->signal_activate().connect(sigc::mem_fun(*this, &TrackEditor::menu_copy_activate));
	menu_paste->signal_activate().connect(sigc::mem_fun(*this, &TrackEditor::menu_paste_activate));
	menu_delete->signal_activate().connect(sigc::mem_fun(*this, &TrackEditor::menu_delete_activate));
	m_trackeditorselectall->signal_activate().connect(sigc::mem_fun(*this, &TrackEditor::menu_selectall_activate));
	m_trackeditorunselectall->signal_activate().connect(sigc::mem_fun(*this, &TrackEditor::menu_unselectall_activate));
	m_trackeditorappendtrackpoint->signal_activate().connect(sigc::mem_fun(*this, &TrackEditor::menu_appendtrackpoint_activate));
	m_trackeditorinserttrackpoint->signal_activate().connect(sigc::mem_fun(*this, &TrackEditor::menu_inserttrackpoint_activate));
	m_trackeditordeletetrackpoint->signal_activate().connect(sigc::mem_fun(*this, &TrackEditor::menu_deletetrackpoint_activate));
	menu_preferences->signal_activate().connect(sigc::mem_fun(*this, &TrackEditor::menu_prefs_activate));
	menu_about->signal_activate().connect(sigc::mem_fun(*this, &TrackEditor::menu_about_activate));
	m_trackeditorcompute->signal_activate().connect(sigc::mem_fun(*this, &TrackEditor::TrackEditor::menu_compute_activate));
	m_trackeditorcreatewaypoint->signal_activate().connect(sigc::mem_fun(*this, &TrackEditor::TrackEditor::menu_createwaypoint_activate));
	m_findwindow->signal_save().connect(sigc::mem_fun(*this, &TrackEditor::compute_save));
	update_undoredo_menu();
	// Vector Map
	m_trackeditormap->set_engine(m_engine);
	m_trackeditormap->set_renderer(VectorMapArea::renderer_vmap);
	m_trackeditormap->signal_cursor().connect(sigc::mem_fun(*this, &TrackEditor::map_cursor));
	m_trackeditormap->set_map_scale(m_engine.get_prefs().mapscale);
	TrackEditor::map_drawflags(m_engine.get_prefs().mapflags);
	m_engine.get_prefs().mapflags.signal_change().connect(sigc::mem_fun(*this, &TrackEditor::map_drawflags));
	m_trackeditormap->set_track(&m_track);
	// Track List
	m_trackliststore = Gtk::ListStore::create(m_tracklistcolumns);
	m_trackeditortreeview->set_model(m_trackliststore);
	Gtk::CellRendererText *fromicao_renderer = Gtk::manage(new Gtk::CellRendererText());
	Gtk::TreeViewColumn *fromicao_column = m_trackeditortreeview->get_column(m_trackeditortreeview->append_column(_("ICAO"), *fromicao_renderer) - 1);
	if (fromicao_column) {
		fromicao_column->add_attribute(*fromicao_renderer, "text", m_tracklistcolumns.m_col_fromicao);
		fromicao_column->set_sort_column(m_tracklistcolumns.m_col_fromicao);
	}
	fromicao_renderer->signal_edited().connect(sigc::mem_fun(*this, &TrackEditor::edited_fromicao));
	fromicao_renderer->set_property("editable", true);
	Gtk::CellRendererText *fromname_renderer = Gtk::manage(new Gtk::CellRendererText());
	Gtk::TreeViewColumn *fromname_column = m_trackeditortreeview->get_column(m_trackeditortreeview->append_column(_("From Name"), *fromname_renderer) - 1);
	if (fromname_column) {
		fromname_column->add_attribute(*fromname_renderer, "text", m_tracklistcolumns.m_col_fromname);
		fromname_column->set_sort_column(m_tracklistcolumns.m_col_fromname);
	}
	fromname_renderer->signal_edited().connect(sigc::mem_fun(*this, &TrackEditor::edited_fromname));
	fromname_renderer->set_property("editable", true);
	Gtk::CellRendererText *toicao_renderer = Gtk::manage(new Gtk::CellRendererText());
	Gtk::TreeViewColumn *toicao_column = m_trackeditortreeview->get_column(m_trackeditortreeview->append_column(_("ICAO"), *toicao_renderer) - 1);
	if (toicao_column) {
		toicao_column->add_attribute(*toicao_renderer, "text", m_tracklistcolumns.m_col_toicao);
		toicao_column->set_sort_column(m_tracklistcolumns.m_col_toicao);
	}
	toicao_renderer->signal_edited().connect(sigc::mem_fun(*this, &TrackEditor::edited_toicao));
	toicao_renderer->set_property("editable", true);
	Gtk::CellRendererText *toname_renderer = Gtk::manage(new Gtk::CellRendererText());
	Gtk::TreeViewColumn *toname_column = m_trackeditortreeview->get_column(m_trackeditortreeview->append_column(_("To Name"), *toname_renderer) - 1);
	if (toname_column) {
		toname_column->add_attribute(*toname_renderer, "text", m_tracklistcolumns.m_col_toname);
		toname_column->set_sort_column(m_tracklistcolumns.m_col_toname);
	}
	toname_renderer->signal_edited().connect(sigc::mem_fun(*this, &TrackEditor::edited_toname));
	toname_renderer->set_property("editable", true);
	DateTimeCellRenderer *offblocktime_renderer = Gtk::manage(new DateTimeCellRenderer());
	Gtk::TreeViewColumn *offblocktime_column = m_trackeditortreeview->get_column(m_trackeditortreeview->append_column(_("OffBlock Time"), *offblocktime_renderer) - 1);
	if (offblocktime_column) {
		offblocktime_column->add_attribute(*offblocktime_renderer, offblocktime_renderer->get_property_name(), m_tracklistcolumns.m_col_offblocktime);
		offblocktime_column->set_sort_column(m_tracklistcolumns.m_col_offblocktime);
	}
	offblocktime_renderer->set_property("editable", true);
	offblocktime_renderer->signal_edited().connect(sigc::mem_fun(*this, &TrackEditor::edited_offblocktime));
	//m_trackeditortreeview->append_column_editable(_("OffBlock Time"), m_tracklistcolumns.m_col_offblocktime);
	DateTimeCellRenderer *takeofftime_renderer = Gtk::manage(new DateTimeCellRenderer());
	Gtk::TreeViewColumn *takeofftime_column = m_trackeditortreeview->get_column(m_trackeditortreeview->append_column(_("Takeoff Time"), *takeofftime_renderer) - 1);
	if (takeofftime_column) {
		takeofftime_column->add_attribute(*takeofftime_renderer, takeofftime_renderer->get_property_name(), m_tracklistcolumns.m_col_takeofftime);
		takeofftime_column->set_sort_column(m_tracklistcolumns.m_col_takeofftime);
	}
	takeofftime_renderer->set_property("editable", true);
	takeofftime_renderer->signal_edited().connect(sigc::mem_fun(*this, &TrackEditor::edited_takeofftime));
	//m_trackeditortreeview->append_column_editable(_("Mod Time"), m_tracklistcolumns.m_col_takeofftime);
	DateTimeCellRenderer *landingtime_renderer = Gtk::manage(new DateTimeCellRenderer());
	Gtk::TreeViewColumn *landingtime_column = m_trackeditortreeview->get_column(m_trackeditortreeview->append_column(_("Landing Time"), *landingtime_renderer) - 1);
	if (landingtime_column) {
		landingtime_column->add_attribute(*landingtime_renderer, landingtime_renderer->get_property_name(), m_tracklistcolumns.m_col_landingtime);
		landingtime_column->set_sort_column(m_tracklistcolumns.m_col_landingtime);
	}
	landingtime_renderer->set_property("editable", true);
	landingtime_renderer->signal_edited().connect(sigc::mem_fun(*this, &TrackEditor::edited_landingtime));
	//m_trackeditortreeview->append_column_editable(_("Landing Time"), m_tracklistcolumns.m_col_landingtime);
	DateTimeCellRenderer *onblocktime_renderer = Gtk::manage(new DateTimeCellRenderer());
	Gtk::TreeViewColumn *onblocktime_column = m_trackeditortreeview->get_column(m_trackeditortreeview->append_column(_("OnBlock Time"), *onblocktime_renderer) - 1);
	if (onblocktime_column) {
		onblocktime_column->add_attribute(*onblocktime_renderer, onblocktime_renderer->get_property_name(), m_tracklistcolumns.m_col_onblocktime);
		onblocktime_column->set_sort_column(m_tracklistcolumns.m_col_onblocktime);
	}
	onblocktime_renderer->set_property("editable", true);
	onblocktime_renderer->signal_edited().connect(sigc::mem_fun(*this, &TrackEditor::edited_onblocktime));
	//m_trackeditortreeview->append_column_editable(_("OnBlock Time"), m_tracklistcolumns.m_col_onblocktime);
	Gtk::CellRendererText *srcid_renderer = Gtk::manage(new Gtk::CellRendererText());
	Gtk::TreeViewColumn *srcid_column = m_trackeditortreeview->get_column(m_trackeditortreeview->append_column(_("Source ID"), *srcid_renderer) - 1);
	if (srcid_column) {
		srcid_renderer->signal_edited().connect(sigc::mem_fun(*this, &TrackEditor::edited_sourceid));
		srcid_column->set_sort_column(m_tracklistcolumns.m_col_sourceid);
	}
	DateTimeCellRenderer *modtime_renderer = Gtk::manage(new DateTimeCellRenderer());
	Gtk::TreeViewColumn *modtime_column = m_trackeditortreeview->get_column(m_trackeditortreeview->append_column(_("Mod Time"), *modtime_renderer) - 1);
	if (modtime_column) {
		modtime_column->add_attribute(*modtime_renderer, modtime_renderer->get_property_name(), m_tracklistcolumns.m_col_modtime);
		modtime_column->set_sort_column(m_tracklistcolumns.m_col_modtime);
	}
	modtime_renderer->set_property("editable", true);
	modtime_renderer->signal_edited().connect(sigc::mem_fun(*this, &TrackEditor::edited_modtime));
	//m_airporteditortreeview->append_column_editable(_("Mod Time"), m_airportlistcolumns.m_col_modtime);
	Gtk::TreeViewColumn *nrpoints_column = m_trackeditortreeview->get_column(m_trackeditortreeview->append_column(_("# Points"), m_tracklistcolumns.m_col_nrpoints) - 1);
	if (nrpoints_column) {
		nrpoints_column->set_sort_column(m_tracklistcolumns.m_col_nrpoints);
	}
	for (unsigned int i = 0; i < 11; ++i) {
		Gtk::TreeViewColumn *col = m_trackeditortreeview->get_column(i);
		if (!col)
			continue;
		col->set_resizable(true);
		col->set_reorderable(true);
	}
	m_trackeditortreeview->columns_autosize();
	m_trackeditortreeview->set_enable_search(true);
	m_trackeditortreeview->set_search_column(m_tracklistcolumns.m_col_searchname);
	// selection
	Glib::RefPtr<Gtk::TreeSelection> trackeditor_selection = m_trackeditortreeview->get_selection();
	trackeditor_selection->set_mode(Gtk::SELECTION_MULTIPLE);
	trackeditor_selection->signal_changed().connect(sigc::mem_fun(*this, &TrackEditor::trackeditor_selection_changed));
	//trackeditor_selection->set_select_function(sigc::mem_fun(*this, &TrackEditor::trackeditor_select_function));
	// steup notebook pages
	m_trackeditornotes->signal_focus_out_event().connect(sigc::mem_fun(*this, &TrackEditor::edited_notes_focus));
	m_trackpoint_model = Gtk::ListStore::create(m_trackpoint_model_columns);
	m_trackpointeditortreeview->set_model(m_trackpoint_model);
	// TrackPoint Treeview
	DateTimeFrac8CellRenderer *trackpointtime_renderer = Gtk::manage(new DateTimeFrac8CellRenderer());
	Gtk::TreeViewColumn *trackpointtime_column = m_trackpointeditortreeview->get_column(m_trackpointeditortreeview->append_column(_("Time"), *trackpointtime_renderer) - 1);
	if (trackpointtime_column) {
		trackpointtime_column->add_attribute(*trackpointtime_renderer, trackpointtime_renderer->get_property_name(), m_trackpoint_model_columns.m_col_time);
		trackpointtime_column->set_sort_column(m_trackpoint_model_columns.m_col_time);
	}
	trackpointtime_renderer->set_property("editable", true);
	trackpointtime_renderer->signal_edited().connect(sigc::mem_fun(*this, &TrackEditor::edited_trackpoint_time));
	//m_trackpointeditortreeview->append_column_editable(_("Time"), m_trackpoint_model_columns.m_col_trackpointtime);
	CoordCellRenderer *trackpointcoord_renderer = Gtk::manage(new CoordCellRenderer());
	Gtk::TreeViewColumn *trackpointcoord_column = m_trackpointeditortreeview->get_column(m_trackpointeditortreeview->append_column(_("Lon"), *trackpointcoord_renderer) - 1);
	if (trackpointcoord_column) {
		trackpointcoord_column->add_attribute(*trackpointcoord_renderer, trackpointcoord_renderer->get_property_name(), m_trackpoint_model_columns.m_col_coord);
		trackpointcoord_column->set_sort_column(m_trackpoint_model_columns.m_col_coord);
	}
	trackpointcoord_renderer->set_property("editable", true);
	trackpointcoord_renderer->signal_edited().connect(sigc::mem_fun(*this, &TrackEditor::edited_trackpoint_coord));
	Gtk::CellRendererText *trackpointalt_renderer = Gtk::manage(new Gtk::CellRendererText());
	Gtk::TreeViewColumn *trackpointalt_column = m_trackpointeditortreeview->get_column(m_trackpointeditortreeview->append_column(_("Elevation"), *trackpointalt_renderer) - 1);
	if (trackpointalt_column) {
 		trackpointalt_column->add_attribute(*trackpointalt_renderer, "text", m_trackpoint_model_columns.m_col_alt);
		trackpointalt_column->set_sort_column(m_trackpoint_model_columns.m_col_alt);
	}
	trackpointalt_renderer->set_property("xalign", 1.0);
	trackpointalt_renderer->signal_edited().connect(sigc::mem_fun(*this, &TrackEditor::edited_trackpoint_alt));
	trackpointalt_renderer->set_property("editable", true);
	for (unsigned int i = 0; i < 4; ++i) {
		Gtk::TreeViewColumn *col = m_trackpointeditortreeview->get_column(i);
		if (!col)
			continue;
		col->set_resizable(true);
		col->set_reorderable(true);
	}
	m_trackpointeditortreeview->set_enable_search(false);
#if 0
	{
		static const int widths[4] = { 200, 100, 100, 50 };
		for (unsigned int i = 0; i < 4; ++i) {
			Gtk::TreeViewColumn *col = m_trackpointeditortreeview->get_column(i);
			if (!col)
				continue;
			col->set_sizing(Gtk::TREE_VIEW_COLUMN_FIXED);
			col->set_fixed_width(widths[i]);
		}
	}
#else
	m_trackpointeditortreeview->columns_autosize();
#endif
	//m_trackpointeditortreeview->set_search_column(m_trackpoint_model_columns.m_col_ident_he);
	// selection
	Glib::RefPtr<Gtk::TreeSelection> trackpoint_selection = m_trackpointeditortreeview->get_selection();
	trackpoint_selection->set_mode(Gtk::SELECTION_MULTIPLE);
	trackpoint_selection->signal_changed().connect(sigc::mem_fun(*this, &TrackEditor::trackeditor_trackpoint_selection_changed));
	// notebook
	m_trackeditornotebook->signal_switch_page().connect(sigc::hide<0>(sigc::mem_fun(*this, &TrackEditor::notebook_switch_page)));
	// final setup
	trackeditor_selection_changed();
	trackeditor_trackpoint_selection_changed();
	//set_tracklist("");
	m_dbselectdialog.signal_byname().connect(sigc::mem_fun(*this, &TrackEditor::dbselect_byname));
	m_dbselectdialog.signal_byrectangle().connect(sigc::mem_fun(*this, &TrackEditor::dbselect_byrect));
	m_dbselectdialog.signal_bytime().connect(sigc::mem_fun(*this, &TrackEditor::dbselect_bytime));
	m_dbselectdialog.show();
	m_prefswindow = PrefsWindow::create(m_engine.get_prefs());
	m_trackeditorwindow->show();
}

TrackEditor::~TrackEditor()
{
	if (m_dbchanged) {
		std::cerr << "Analyzing database..." << std::endl;
		m_db->analyze();
		m_db->vacuum();
	}
	m_db->close();
}

void TrackEditor::dbselect_byname( const Glib::ustring & s, TracksDb::comp_t comp )
{
	TracksDb::elementvector_t els(m_db->find_by_text(s, 0, comp, 0));
	m_trackliststore->clear();
	for (TracksDb::elementvector_t::iterator ei(els.begin()), ee(els.end()); ei != ee; ++ei) {
		TracksDb::element_t& e(*ei);
		if (!e.is_valid())
			continue;
		if (e.get_sourceid().empty())
			e.set_sourceid(make_sourceid());
		Gtk::TreeModel::Row row(*(m_trackliststore->append()));
		set_row(row, e);
	}
}

void TrackEditor::dbselect_byrect( const Rect & r )
{
	TracksDb::elementvector_t els(m_db->find_by_rect(r, 0));
	m_trackliststore->clear();
	for (TracksDb::elementvector_t::iterator ei(els.begin()), ee(els.end()); ei != ee; ++ei) {
		TracksDb::element_t& e(*ei);
		if (!e.is_valid())
			continue;
		if (e.get_sourceid().empty())
			e.set_sourceid(make_sourceid());
		Gtk::TreeModel::Row row(*(m_trackliststore->append()));
		set_row(row, e);
	}
}

void TrackEditor::dbselect_bytime(time_t timefrom, time_t timeto)
{
	TracksDb::elementvector_t els(m_db->find_by_time(timefrom, timeto, 0));
	m_trackliststore->clear();
	for (TracksDb::elementvector_t::iterator ei(els.begin()), ee(els.end()); ei != ee; ++ei) {
		TracksDb::element_t& e(*ei);
		if (!e.is_valid())
			continue;
		if (e.get_sourceid().empty())
			e.set_sourceid(make_sourceid());
		Gtk::TreeModel::Row row(*(m_trackliststore->append()));
		set_row(row, e);
	}
}

void TrackEditor::buttonclear_clicked( void )
{
	m_trackeditorfind->set_text("");
}

void TrackEditor::find_changed( void )
{
	dbselect_byname(m_trackeditorfind->get_text());
}

void TrackEditor::menu_exportkml_activate(void )
{
	Glib::RefPtr<Gtk::TreeSelection> trackeditor_selection = m_trackeditortreeview->get_selection();
	std::vector<Gtk::TreeModel::Path> sel(trackeditor_selection->get_selected_rows());
	if (sel.empty())
		return;
#ifdef HAVE_HILDONMM
	Hildon::FileChooserDialog dlg(*m_trackeditorwindow, Gtk::FILE_CHOOSER_ACTION_SAVE);
	dlg.set_extension("kml");
#else
	Gtk::FileChooserDialog dlg(*m_trackeditorwindow, "Export to KML File", Gtk::FILE_CHOOSER_ACTION_SAVE);
	dlg.add_button(Gtk::Stock::OK, Gtk::RESPONSE_OK);
	dlg.add_button(Gtk::Stock::CANCEL, Gtk::RESPONSE_CANCEL);
#endif
	{
		Gtk::TreeModel::iterator iter(m_trackliststore->get_iter(*sel.begin()));
		if (iter) {
			Gtk::TreeModel::Row row = *iter;
			TracksDb::Track e((*m_db)(row[m_tracklistcolumns.m_col_id]));
			if (e.is_valid()) {
				Glib::ustring fname(e.get_fromicaoname());
				Glib::ustring fname2(e.get_toicaoname());
				if (!fname.empty() && !fname2.empty())
					fname += " ";
				fname += fname2;
				dlg.set_filename(fname + ".kml");
			}
		}
	}
	dlg.set_modal(true);
	int resp = dlg.run();
	dlg.hide();
	switch (resp) {
		case Gtk::RESPONSE_ACCEPT:
		case Gtk::RESPONSE_OK:
		case Gtk::RESPONSE_YES:
		case Gtk::RESPONSE_APPLY:
			break;

		default:
			return;
	}
	try {
		KMLExporter ex(dlg.get_filename(), false); // googleearth seems to have troubles reading compressed KML files
		for (std::vector<Gtk::TreeModel::Path>::iterator si(sel.begin()), se(sel.end()); si != se; si++) {
			Gtk::TreeModel::iterator iter(m_trackliststore->get_iter(*si));
			if (!iter) {
				std::cerr << "cannot get iterator for path " << (*si).to_string() << " ?""?" << std::endl;
				continue;
			}
			Gtk::TreeModel::Row row = *iter;
			TracksDb::Track e((*m_db)(row[m_tracklistcolumns.m_col_id]));
			if (!e.is_valid())
				continue;
			ex.write(e);
		}
	} catch (const std::exception& e) {
		Gtk::MessageDialog dlg(*m_trackeditorwindow, Glib::ustring("Export to KML failed: ") + e.what(), false, Gtk::MESSAGE_ERROR, Gtk::BUTTONS_CLOSE, false);
		dlg.show();
	}
}

void TrackEditor::menu_quit_activate( void )
{
	m_trackeditorwindow->hide();
}

void TrackEditor::menu_undo_activate( void )
{
	{
		Glib::RefPtr<Gtk::TreeSelection> selection = m_trackeditortreeview->get_selection();
		selection->unselect_all();
	}
	TracksDb::Track e(m_undoredo.undo());
	if (e.is_valid()) {
		save_nostack(e);
		select(e);
	}
	update_undoredo_menu();
}

void TrackEditor::menu_redo_activate( void )
{
	{
		Glib::RefPtr<Gtk::TreeSelection> selection = m_trackeditortreeview->get_selection();
		selection->unselect_all();
	}
	TracksDb::Track e(m_undoredo.redo());
	if (e.is_valid()) {
		save_nostack(e);
		select(e);
	}
	update_undoredo_menu();
}

void TrackEditor::menu_newtrack_activate( void )
{
	TracksDb::Track e;
	e.make_valid();
	time_t t(time(0));
	e.set_offblocktime(t);
	e.set_takeofftime(t);
	e.set_landingtime(t);
	e.set_onblocktime(t);
	e.set_sourceid(make_sourceid());
	e.set_modtime(time(0));
	save(e);
	select(e);
}

void TrackEditor::menu_deletetrack_activate( void )
{
	Glib::RefPtr<Gtk::TreeSelection> trackeditor_selection = m_trackeditortreeview->get_selection();
	std::vector<Gtk::TreeModel::Path> sel(trackeditor_selection->get_selected_rows());
	if (sel.empty())
		return;
	if (sel.size() > 1) {
		Gtk::MessageDialog *dlg = new Gtk::MessageDialog(_("Delete multiple tracks?"), false, Gtk::MESSAGE_QUESTION, Gtk::BUTTONS_YES_NO, true);
		int res = dlg->run();
		delete dlg;
		if (res != Gtk::RESPONSE_YES) {
			if (res != Gtk::RESPONSE_NO)
				std::cerr << "unexpected dialog result: " << res << std::endl;
			return;
		}
	}
	for (std::vector<Gtk::TreeModel::Path>::iterator si(sel.begin()), se(sel.end()); si != se; si++) {
		Gtk::TreeModel::iterator iter(m_trackliststore->get_iter(*si));
		if (!iter) {
			std::cerr << "cannot get iterator for path " << (*si).to_string() << " ?""?" << std::endl;
			continue;
		}
		Gtk::TreeModel::Row row = *iter;
		std::cerr << "deleting row: " << row[m_tracklistcolumns.m_col_id] << std::endl;
		TracksDb::Track e((*m_db)(row[m_tracklistcolumns.m_col_id]));
		if (!e.is_valid())
			continue;
		m_undoredo.erase(e);
		m_db->erase(e);
		m_trackliststore->erase(iter);
	}
}

void TrackEditor::menu_duplicatetrack_activate(void)
{
	Glib::RefPtr<Gtk::TreeSelection> trackeditor_selection = m_trackeditortreeview->get_selection();
	std::vector<Gtk::TreeModel::Path> sel(trackeditor_selection->get_selected_rows());
	if (sel.empty())
		return;
	if (sel.size() > 1)
		return;
	Gtk::TreeModel::iterator iter(m_trackliststore->get_iter(*sel.begin()));
	if (!iter)
		return;
	Gtk::TreeModel::Row row = *iter;
	TracksDb::Track e((*m_db)(row[m_tracklistcolumns.m_col_id]));
	if (!e.is_valid())
		return;
	TracksDb::Track e2;
	e2.make_valid();
	e2.set_offblocktime(e.get_offblocktime());
	e2.set_takeofftime(e.get_takeofftime());
	e2.set_landingtime(e.get_landingtime());
	e2.set_onblocktime(e.get_onblocktime());
	e2.set_fromicao(e.get_fromicao());
	e2.set_fromname(e.get_fromname());
	e2.set_toicao(e.get_toicao());
	e2.set_toname(e.get_toname());
	e2.set_notes(e.get_notes());
	for (unsigned int i = 0; i < e.size(); i++)
		e2.append(e[i]);
	e2.sort();
	e2.set_modtime(time(0));
	e2.set_sourceid(make_sourceid());
	save(e2);
	select(e2);
}

void TrackEditor::menu_mergetracks_activate(void)
{
	Glib::RefPtr<Gtk::TreeSelection> trackeditor_selection = m_trackeditortreeview->get_selection();
	std::vector<Gtk::TreeModel::Path> sel(trackeditor_selection->get_selected_rows());
	if (sel.empty() || sel.size() <= 1)
		return;
	bool first(true);
	TracksDb::Track e2;
	for (std::vector<Gtk::TreeModel::Path>::iterator si(sel.begin()), se(sel.end()); si != se; si++) {
		Gtk::TreeModel::iterator iter(m_trackliststore->get_iter(*si));
		if (!iter) {
			std::cerr << "cannot get iterator for path " << (*si).to_string() << " ?""?" << std::endl;
			continue;
		}
		Gtk::TreeModel::Row row = *iter;
		TracksDb::Track e((*m_db)(row[m_tracklistcolumns.m_col_id]));
		if (!e.is_valid())
			continue;
		e2.make_valid();
		if (first || e.get_offblocktime() < e2.get_offblocktime()) {
			e2.set_fromicao(e.get_fromicao());
			e2.set_fromname(e.get_fromname());
			e2.set_offblocktime(e.get_offblocktime());
			e2.set_takeofftime(e.get_takeofftime());
		}
		if (first || e.get_onblocktime() > e2.get_onblocktime()) {
			e2.set_toicao(e.get_toicao());
			e2.set_toname(e.get_toname());
			e2.set_landingtime(e.get_landingtime());
			e2.set_onblocktime(e.get_onblocktime());
		}
		if (first)
			e2.set_notes(e.get_notes());
		for (unsigned int i = 0; i < e.size(); i++)
			e2.append(e[i]);
	}
	if (!e2.is_valid())
		return;
	e2.sort();
	e2.set_modtime(time(0));
	save(e2);
	select(e2);
}

void TrackEditor::menu_selectall_activate(void)
{
	switch (m_trackeditornotebook->get_current_page()) {
		case 0:
		{
			Glib::RefPtr<Gtk::TreeSelection> selection = m_trackeditortreeview->get_selection();
			selection->select_all();
			break;
		}

		case 2:
		{
			Glib::RefPtr<Gtk::TreeSelection> selection = m_trackpointeditortreeview->get_selection();
			selection->select_all();
			break;
		}
	}
}

void TrackEditor::menu_unselectall_activate(void)
{
	switch (m_trackeditornotebook->get_current_page()) {
		case 0:
		{
			Glib::RefPtr<Gtk::TreeSelection> selection = m_trackeditortreeview->get_selection();
			selection->unselect_all();
			break;
		}

		case 2:
		{
			Glib::RefPtr<Gtk::TreeSelection> selection = m_trackpointeditortreeview->get_selection();
			selection->unselect_all();
			break;
		}
	}
}

void TrackEditor::menu_cut_activate( void )
{
	Gtk::Widget *focus = m_trackeditorwindow->get_focus();
	if (!focus)
		return;
	Gtk::Editable *edit = dynamic_cast<Gtk::Editable *>(focus);
	if (!edit)
		return;
	edit->cut_clipboard();
}

void TrackEditor::menu_copy_activate( void )
{
	Gtk::Widget *focus = m_trackeditorwindow->get_focus();
	if (!focus)
		return;
	Gtk::Editable *edit = dynamic_cast<Gtk::Editable *>(focus);
	if (!edit)
		return;
	edit->copy_clipboard();
}

void TrackEditor::menu_paste_activate( void )
{
	Gtk::Widget *focus = m_trackeditorwindow->get_focus();
	if (!focus)
		return;
	Gtk::Editable *edit = dynamic_cast<Gtk::Editable *>(focus);
	if (!edit)
		return;
	edit->paste_clipboard();
}

void TrackEditor::menu_delete_activate( void )
{
	Gtk::Widget *focus = m_trackeditorwindow->get_focus();
	if (!focus)
		return;
	Gtk::Editable *edit = dynamic_cast<Gtk::Editable *>(focus);
	if (!edit)
		return;
	edit->delete_selection();
}

void TrackEditor::menu_mapenable_toggled( void )
{
	if (m_trackeditormapenable->get_active()) {
		m_trackeditorairportdiagramenable->set_active(false);
		m_trackeditormap->set_renderer(VectorMapArea::renderer_vmap);
		m_trackeditormap->set_map_scale(m_engine.get_prefs().mapscale);
		m_trackeditormap->show();
		m_trackeditormapzoomin->show();
		m_trackeditormapzoomout->show();
	} else {
		m_trackeditormap->hide();
		m_trackeditormapzoomin->hide();
		m_trackeditormapzoomout->hide();
	}
}

void TrackEditor::menu_airportdiagramenable_toggled( void )
{
	if (m_trackeditorairportdiagramenable->get_active()) {
		m_trackeditormapenable->set_active(false);
		m_trackeditormap->set_renderer(VectorMapArea::renderer_airportdiagram);
		m_trackeditormap->set_map_scale(m_engine.get_prefs().mapscaleairportdiagram);
		m_trackeditormap->show();
		m_trackeditormapzoomin->show();
		m_trackeditormapzoomout->show();
	} else {
		m_trackeditormap->hide();
		m_trackeditormapzoomin->hide();
		m_trackeditormapzoomout->hide();
	}
}

void TrackEditor::menu_mapzoomin_activate( void )
{
	m_trackeditormap->zoom_in();
	if (m_trackeditormap->get_renderer() == VectorMapArea::renderer_airportdiagram)
		m_engine.get_prefs().mapscaleairportdiagram = m_trackeditormap->get_map_scale();
	else
		m_engine.get_prefs().mapscale = m_trackeditormap->get_map_scale();
}

void TrackEditor::menu_mapzoomout_activate( void )
{
	m_trackeditormap->zoom_out();
	if (m_trackeditormap->get_renderer() == VectorMapArea::renderer_airportdiagram)
		m_engine.get_prefs().mapscaleairportdiagram = m_trackeditormap->get_map_scale();
	else
		m_engine.get_prefs().mapscale = m_trackeditormap->get_map_scale();
}

void TrackEditor::new_trackpoint(int index)
{
	if (!m_curentryid)
		return;
	TracksDb::Track e((*m_db)(m_curentryid));
	if (!e.is_valid())
		return;
	if (index < 0)
		index = 0;
	if (index >= e.size())
		index = e.size();
	TracksDb::Track::TrackPoint& tp(e.insert(TracksDb::Track::TrackPoint(), index));
	if (e.size() <= 1) {
		tp.set_time(time(0));
		tp.set_alt_invalid();
	} else {
		int index1 = index - 1;
		if (index1 < 0)
			index1 = 1;
		const TracksDb::Track::TrackPoint& tp1(e[index1]);
		tp.set_time(tp1.get_time());
		tp.set_coord(tp1.get_coord());
		if (tp.is_alt_valid())
			tp.set_alt(tp1.get_alt());
		else
			tp.set_alt_invalid();
	}
	e.set_modtime(time(0));
	m_db->save(e);
	m_dbchanged = true;
	set_trackpoints(e);
}

void TrackEditor::menu_appendtrackpoint_activate(void )
{
	new_trackpoint(std::numeric_limits<int>::max());
}

void TrackEditor::menu_inserttrackpoint_activate(void )
{
	Glib::RefPtr<Gtk::TreeSelection> trackpointeditor_selection = m_trackpointeditortreeview->get_selection();
	std::vector<Gtk::TreeModel::Path> sel(trackpointeditor_selection->get_selected_rows());
	std::vector<Gtk::TreeModel::Path>::size_type selcount(sel.size());
	if (selcount != 1) {
		menu_appendtrackpoint_activate();
		return;
	}
	Gtk::TreeModel::Row row(*m_trackpoint_model->get_iter(*sel.begin()));
	new_trackpoint(row[m_trackpoint_model_columns.m_col_index]);
}

void TrackEditor::menu_deletetrackpoint_activate(void )
{
	if (!m_curentryid)
		return;
	Glib::RefPtr<Gtk::TreeSelection> trackpointeditor_selection = m_trackpointeditortreeview->get_selection();
	std::vector<Gtk::TreeModel::Path> sel(trackpointeditor_selection->get_selected_rows());
	std::vector<int> ids;
	for (std::vector<Gtk::TreeModel::Path>::iterator si(sel.begin()), se(sel.end()); si != se; si++) {
		Gtk::TreeModel::Row row(*m_trackpoint_model->get_iter(*si));
		ids.push_back(row[m_trackpoint_model_columns.m_col_index]);
	}
	sort(ids.rbegin(), ids.rend());
	TracksDb::Track e((*m_db)(m_curentryid));
	if (!e.is_valid())
		return;
	for (std::vector<int>::reverse_iterator ib(ids.rbegin()), ie(ids.rend()); ib != ie; ib++)
		e.erase(*ib);
	e.set_modtime(time(0));
	m_db->save(e);
	m_dbchanged = true;
	set_trackpoints(e);
}

void TrackEditor::menu_prefs_activate(void )
{
	if (m_prefswindow)
		m_prefswindow->show();
}

void TrackEditor::menu_about_activate( void )
{
	m_aboutdialog->show();
}

void TrackEditor::menu_compute_activate(void )
{
	if (!m_curentryid)
		return;
	TracksDb::Track e((*m_db)(m_curentryid));
	if (!e.is_valid())
		return;
	m_findwindow->present(e);
}

void TrackEditor::menu_createwaypoint_activate(void)
{
	if (!m_curentryid)
		return;
	Glib::RefPtr<Gtk::TreeSelection> trackpointeditor_selection = m_trackpointeditortreeview->get_selection();
	std::vector<Gtk::TreeModel::Path> sel(trackpointeditor_selection->get_selected_rows());
	std::vector<Gtk::TreeModel::Path>::size_type selcount(sel.size());
	if (selcount != 1)
		return;
	Gtk::TreeModel::iterator iter(m_trackpoint_model->get_iter(*sel.begin()));
	Gtk::TreeModel::Row row = *iter;
	int index(row[m_trackpoint_model_columns.m_col_index]);
	TracksDb::Track e((*m_db)(m_curentryid));
	if (!e.is_valid() || index < 0 || index >= e.size())
		return;
	TracksDb::Track::TrackPoint& tp(e[index]);
	Glib::ustring name(e.get_fromname());
	Glib::ustring icao(e.get_fromicao());
	if (!e.get_fromname().empty() && !e.get_toname().empty())
		name += " - ";
	if (!e.get_fromicao().empty() && !e.get_toicao().empty())
		icao += " - ";
	name += e.get_toname();
	icao += e.get_toicao();
	Glib::RefPtr<NewWaypointWindow> wnd(NewWaypointWindow::create(m_engine, tp.get_coord(), name, icao));
	wnd->present();
}

void TrackEditor::set_row( Gtk::TreeModel::Row& row, const TracksDb::Track& e )
{
	row[m_tracklistcolumns.m_col_id] = e.get_address();
	row[m_tracklistcolumns.m_col_fromicao] = e.get_fromicao();
	row[m_tracklistcolumns.m_col_fromname] = e.get_fromname();
	row[m_tracklistcolumns.m_col_toicao] = e.get_toicao();
	row[m_tracklistcolumns.m_col_toname] = e.get_toname();
	row[m_tracklistcolumns.m_col_offblocktime] = e.get_offblocktime();
	row[m_tracklistcolumns.m_col_takeofftime] = e.get_takeofftime();
	row[m_tracklistcolumns.m_col_landingtime] = e.get_landingtime();
	row[m_tracklistcolumns.m_col_onblocktime] = e.get_onblocktime();
	row[m_tracklistcolumns.m_col_sourceid] = e.get_sourceid();
	row[m_tracklistcolumns.m_col_modtime] = e.get_modtime();
	row[m_tracklistcolumns.m_col_nrpoints] = e.size();
	row[m_tracklistcolumns.m_col_searchname] = e.get_fromicao() + " " + e.get_fromname() + " " + e.get_toicao() + " " + e.get_toname();
	if (e.get_address() == m_curentryid)
		set_lists(e);
}

#if 0

void TrackEditor::set_trackpoints(const TracksDb::Track & e)
{
	Gtk::TreeModel::iterator iter(m_trackpoint_model->children().begin());
	for (unsigned int i = 0; i < e.size(); i++) {
		const TracksDb::Track::TrackPoint& tp(e[i]);
		Gtk::TreeModel::Row row;
		if (!iter)
			iter = m_trackpoint_model->append();
		row = *iter;
		iter++;
		row[m_trackpoint_model_columns.m_col_index] = i;
		row[m_trackpoint_model_columns.m_col_lon] = tp.get_coord().get_lon();
		row[m_trackpoint_model_columns.m_col_lat] = tp.get_coord().get_lat();
		if (tp.is_alt_valid()) {
			std::ostringstream oss;
			oss << tp.get_alt();
			row[m_trackpoint_model_columns.m_col_alt] = oss.str();
		} else {
			row[m_trackpoint_model_columns.m_col_alt] = "";
		}
		row[m_trackpoint_model_columns.m_col_time] = tp.get_timefrac8();
	}
	for (; iter; ) {
		Gtk::TreeModel::iterator iter1(iter);
		iter++;
		m_trackpoint_model->erase(iter1);
	}
}

#else

void TrackEditor::set_trackpoints(const TracksDb::Track & e)
{
	Glib::RefPtr<Gtk::ListStore> model = Gtk::ListStore::create(m_trackpoint_model_columns);
	for (unsigned int i = 0; i < e.size(); i++) {
		const TracksDb::Track::TrackPoint& tp(e[i]);
		Gtk::TreeModel::iterator iter(model->append());
		Gtk::TreeModel::Row row(*iter);
		row[m_trackpoint_model_columns.m_col_index] = i;
		row[m_trackpoint_model_columns.m_col_coord] = tp.get_coord();
		if (tp.is_alt_valid()) {
			std::ostringstream oss;
			oss << tp.get_alt();
			row[m_trackpoint_model_columns.m_col_alt] = oss.str();
		} else {
			row[m_trackpoint_model_columns.m_col_alt] = "";
		}
		row[m_trackpoint_model_columns.m_col_time] = tp.get_timefrac8();
	}
	m_trackpoint_model = model;
	m_trackpointeditortreeview->set_model(m_trackpoint_model);
}

#endif

void TrackEditor::set_lists(const TracksDb::Track & e)
{
	Glib::RefPtr<Gtk::TextBuffer> tb(m_trackeditornotes->get_buffer());
	tb->set_text(e.get_notes());
	tb->set_modified(false);
	set_trackpoints(e);
}

void TrackEditor::edited_fromicao( const Glib::ustring & path_string, const Glib::ustring & new_text )
{
	Gtk::TreePath path(path_string);
	Gtk::TreeModel::iterator iter = m_trackliststore->get_iter(path);
	if (!iter)
		return;
	Gtk::TreeRow row = *iter;
	row[m_tracklistcolumns.m_col_fromicao] = new_text;
	TracksDb::Track e((*m_db)(row[m_tracklistcolumns.m_col_id]));
	if (!e.is_valid())
		return;
	e.set_fromicao(new_text);
	e.set_modtime(time(0));
	save_noerr(e, row);
}

void TrackEditor::edited_fromname( const Glib::ustring & path_string, const Glib::ustring & new_text )
{
	Gtk::TreePath path(path_string);
	Gtk::TreeModel::iterator iter = m_trackliststore->get_iter(path);
	if (!iter)
		return;
	Gtk::TreeRow row = *iter;
	row[m_tracklistcolumns.m_col_fromname] = new_text;
	TracksDb::Track e((*m_db)(row[m_tracklistcolumns.m_col_id]));
	if (!e.is_valid())
		return;
	e.set_fromname(new_text);
	e.set_modtime(time(0));
	save_noerr(e, row);
}

void TrackEditor::edited_toicao( const Glib::ustring & path_string, const Glib::ustring & new_text )
{
	Gtk::TreePath path(path_string);
	Gtk::TreeModel::iterator iter = m_trackliststore->get_iter(path);
	if (!iter)
		return;
	Gtk::TreeRow row = *iter;
	row[m_tracklistcolumns.m_col_toicao] = new_text;
	TracksDb::Track e((*m_db)(row[m_tracklistcolumns.m_col_id]));
	if (!e.is_valid())
		return;
	e.set_toicao(new_text);
	e.set_modtime(time(0));
	save_noerr(e, row);
}

void TrackEditor::edited_toname( const Glib::ustring & path_string, const Glib::ustring & new_text )
{
	Gtk::TreePath path(path_string);
	Gtk::TreeModel::iterator iter = m_trackliststore->get_iter(path);
	if (!iter)
		return;
	Gtk::TreeRow row = *iter;
	row[m_tracklistcolumns.m_col_toname] = new_text;
	TracksDb::Track e((*m_db)(row[m_tracklistcolumns.m_col_id]));
	if (!e.is_valid())
		return;
	e.set_toname(new_text);
	e.set_modtime(time(0));
	save_noerr(e, row);
}

void TrackEditor::edited_offblocktime( const Glib::ustring & path_string, const Glib::ustring& new_text, time_t new_time )
{
	Gtk::TreePath path(path_string);
	Gtk::TreeModel::iterator iter = m_trackliststore->get_iter(path);
	if (!iter)
		return;
	Gtk::TreeRow row = *iter;
	row[m_tracklistcolumns.m_col_offblocktime] = new_time;
	TracksDb::Track e((*m_db)(row[m_tracklistcolumns.m_col_id]));
	if (!e.is_valid())
		return;
	e.set_offblocktime(new_time);
	e.set_modtime(time(0));
	save_noerr(e, row);
}

void TrackEditor::edited_takeofftime( const Glib::ustring & path_string, const Glib::ustring& new_text, time_t new_time )
{
	Gtk::TreePath path(path_string);
	Gtk::TreeModel::iterator iter = m_trackliststore->get_iter(path);
	if (!iter)
		return;
	Gtk::TreeRow row = *iter;
	row[m_tracklistcolumns.m_col_takeofftime] = new_time;
	TracksDb::Track e((*m_db)(row[m_tracklistcolumns.m_col_id]));
	if (!e.is_valid())
		return;
	e.set_takeofftime(new_time);
	e.set_modtime(time(0));
	save_noerr(e, row);
}

void TrackEditor::edited_landingtime( const Glib::ustring & path_string, const Glib::ustring& new_text, time_t new_time )
{
	Gtk::TreePath path(path_string);
	Gtk::TreeModel::iterator iter = m_trackliststore->get_iter(path);
	if (!iter)
		return;
	Gtk::TreeRow row = *iter;
	row[m_tracklistcolumns.m_col_landingtime] = new_time;
	TracksDb::Track e((*m_db)(row[m_tracklistcolumns.m_col_id]));
	if (!e.is_valid())
		return;
	e.set_landingtime(new_time);
	e.set_modtime(time(0));
	save_noerr(e, row);
}

void TrackEditor::edited_onblocktime( const Glib::ustring & path_string, const Glib::ustring& new_text, time_t new_time )
{
	Gtk::TreePath path(path_string);
	Gtk::TreeModel::iterator iter = m_trackliststore->get_iter(path);
	if (!iter)
		return;
	Gtk::TreeRow row = *iter;
	row[m_tracklistcolumns.m_col_onblocktime] = new_time;
	TracksDb::Track e((*m_db)(row[m_tracklistcolumns.m_col_id]));
	if (!e.is_valid())
		return;
	e.set_onblocktime(new_time);
	e.set_modtime(time(0));
	save_noerr(e, row);
}

void TrackEditor::edited_sourceid( const Glib::ustring & path_string, const Glib::ustring & new_text )
{
	Gtk::TreePath path(path_string);
	Gtk::TreeModel::iterator iter = m_trackliststore->get_iter(path);
	if (!iter)
		return;
	Gtk::TreeRow row = *iter;
	row[m_tracklistcolumns.m_col_sourceid] = new_text;
	TracksDb::Track e((*m_db)(row[m_tracklistcolumns.m_col_id]));
	if (!e.is_valid())
		return;
	e.set_sourceid(new_text);
	e.set_modtime(time(0));
	save_noerr(e, row);
}

void TrackEditor::edited_modtime( const Glib::ustring & path_string, const Glib::ustring& new_text, time_t new_time )
{
	Gtk::TreePath path(path_string);
	Gtk::TreeModel::iterator iter = m_trackliststore->get_iter(path);
	if (!iter)
		return;
	Gtk::TreeRow row = *iter;
	row[m_tracklistcolumns.m_col_modtime] = new_time;
	TracksDb::Track e((*m_db)(row[m_tracklistcolumns.m_col_id]));
	if (!e.is_valid())
		return;
	e.set_modtime(new_time);
	save_noerr(e, row);
}

void TrackEditor::edited_trackpoint_coord( const Glib::ustring & path_string, const Glib::ustring& new_text, Point new_coord )
{
	Gtk::TreePath path(path_string);
	Gtk::TreeModel::iterator iter = m_trackpoint_model->get_iter(path);
	if (!iter)
		return;
	Gtk::TreeRow row = *iter;
	int index = row[m_trackpoint_model_columns.m_col_index];
	if (!m_curentryid || index < 0)
		return;
	row[m_trackpoint_model_columns.m_col_coord] = new_coord;
	TracksDb::Track e((*m_db)(m_curentryid));
	if (!e.is_valid() || index >= e.size())
		return;
	TracksDb::Track::TrackPoint& tp(e[index]);
	tp.set_coord(new_coord);
	e.recompute_bbox();
	e.set_modtime(time(0));
	save_noerr_lists(e);
	m_trackeditormap->set_center(new_coord, tp.is_alt_valid() ? tp.get_alt() : 0);
	m_trackeditormap->set_cursor(new_coord);
}

void TrackEditor::edited_trackpoint_alt(const Glib::ustring & path_string, const Glib::ustring & new_text)
{
	Gtk::TreePath path(path_string);
	Gtk::TreeModel::iterator iter = m_trackpoint_model->get_iter(path);
	if (!iter)
		return;
	Gtk::TreeRow row = *iter;
	int index = row[m_trackpoint_model_columns.m_col_index];
	if (!m_curentryid || index < 0)
		return;
	bool altvalid = true;
	int32_t alt = Conversions::signed_feet_parse(new_text, &altvalid);
	if (altvalid) {
		std::ostringstream oss;
		oss << alt;
		row[m_trackpoint_model_columns.m_col_alt] = oss.str();
	} else {
		row[m_trackpoint_model_columns.m_col_alt] = "";
	}
	TracksDb::Track e((*m_db)(m_curentryid));
	if (!e.is_valid() || index >= e.size())
		return;
	TracksDb::Track::TrackPoint& tp(e[index]);
	if (altvalid)
		tp.set_alt(alt);
	else
		tp.set_alt_invalid();
	e.set_modtime(time(0));
	save_noerr_lists(e);
}

void TrackEditor::edited_trackpoint_time( const Glib::ustring & path_string, const Glib::ustring& new_text, uint64_t new_time )
{
	Gtk::TreePath path(path_string);
	Gtk::TreeModel::iterator iter = m_trackpoint_model->get_iter(path);
	if (!iter)
		return;
	Gtk::TreeRow row = *iter;
	int index = row[m_trackpoint_model_columns.m_col_index];
	if (!m_curentryid || index < 0)
		return;
	row[m_trackpoint_model_columns.m_col_time] = new_time;
	TracksDb::Track e((*m_db)(m_curentryid));
	if (!e.is_valid() || index >= e.size())
		return;
	TracksDb::Track::TrackPoint& tp(e[index]);
	tp.set_timefrac8(new_time);
	e.set_modtime(time(0));
	save_noerr_lists(e);
}

bool TrackEditor::edited_notes_focus(GdkEventFocus * event)
{
	Glib::RefPtr<Gtk::TextBuffer> tb(m_trackeditornotes->get_buffer());
	if (!tb->get_modified())
		return false;
	TracksDb::Track e((*m_db)(m_curentryid));
	if (!e.is_valid())
		return false;
	e.set_notes(tb->get_text());
	e.set_modtime(time(0));
	save_noerr_lists(e);
	tb->set_modified(false);
	return false;
}

void TrackEditor::map_cursor( Point cursor, VectorMapArea::cursor_validity_t valid )
{
	if (valid != VectorMapArea::cursor_mouse)
		return;
	Glib::RefPtr<Gtk::TreeSelection> trackpointeditor_selection = m_trackpointeditortreeview->get_selection();
	std::vector<Gtk::TreeModel::Path> sel(trackpointeditor_selection->get_selected_rows());
	std::vector<Gtk::TreeModel::Path>::size_type selcount(sel.size());
	Gtk::TreeModel::iterator iter;
	if (selcount > 0)
		iter = m_trackpoint_model->get_iter(*sel.begin());
	if (selcount != 1 || !iter)
		return;
	Gtk::TreeRow row = *iter;
	int index = row[m_trackpoint_model_columns.m_col_index];
	if (!m_curentryid || index < 0)
		return;
	row[m_trackpoint_model_columns.m_col_coord] = cursor;
	TracksDb::Track e((*m_db)(m_curentryid));
	if (!e.is_valid() || index >= e.size())
		return;
	TracksDb::Track::TrackPoint& tp(e[index]);
	tp.set_coord(cursor);
	e.recompute_bbox();
	e.set_modtime(time(0));
	save_noerr_lists(e);
}

void TrackEditor::map_drawflags(int flags)
{
	*m_trackeditormap = (MapRenderer::DrawFlags)flags;
}

void TrackEditor::save_nostack( TracksDb::Track & e )
{
	if (!e.is_valid())
		return;
	//std::cerr << "save_nostack" << std::endl;
	TracksDb::Track::Address addr(e.get_address());
	m_db->save(e);
	m_dbchanged = true;
	if (m_curentryid == addr) {
		m_curentryid = e.get_address();
		if (e.is_subtables_loaded()) {
			m_track = e;
			m_trackeditormap->redraw_track();
		}
	}
	Gtk::TreeIter iter = m_trackliststore->children().begin();
	while (iter) {
		Gtk::TreeRow row = *iter;
		if (addr == row[m_tracklistcolumns.m_col_id]) {
			set_row(row, e);
			return;
		}
		iter++;
	}
	Gtk::TreeModel::Row row(*(m_trackliststore->append()));
	set_row(row, e);
}

void TrackEditor::save( TracksDb::Track & e )
{
	//std::cerr << "save" << std::endl;
	save_nostack(e);
	m_undoredo.save(e);
	update_undoredo_menu();
}

void TrackEditor::save_noerr(TracksDb::Track & e, Gtk::TreeModel::Row & row)
{
	if (e.get_address() != row[m_tracklistcolumns.m_col_id])
		throw std::runtime_error("TrackEditor::save: ID mismatch");
	try {
		save(e);
	} catch (const sqlite3x::database_error& dberr) {
		TracksDb::Track eold((*m_db)(row[m_tracklistcolumns.m_col_id]));
		set_row(row, eold);
	}
}

void TrackEditor::save_noerr_lists(TracksDb::Track & e)
{
	try {
		save(e);
	} catch (const sqlite3x::database_error& dberr) {
		TracksDb::Track eold((*m_db)(e.get_address()));
		set_lists(eold);
	}
}

void TrackEditor::select( const TracksDb::Track & e )
{
	Glib::RefPtr<Gtk::TreeSelection> trackeditor_selection = m_trackeditortreeview->get_selection();
	Gtk::TreeIter iter = m_trackliststore->children().begin();
	while (iter) {
		Gtk::TreeRow row = *iter;
		if (e.get_address() == row[m_tracklistcolumns.m_col_id]) {
			trackeditor_selection->unselect_all();
			trackeditor_selection->select(iter);
			m_trackeditortreeview->scroll_to_row(Gtk::TreeModel::Path(iter));
			return;
		}
		iter++;
	}
}

void TrackEditor::compute_save(TracksDb::Track & e)
{
	e.set_modtime(time(0));
	save(e);
	select(e);
}

void TrackEditor::update_undoredo_menu( void )
{
	m_trackeditorundo->set_sensitive(m_undoredo.can_undo());
	m_trackeditorredo->set_sensitive(m_undoredo.can_redo());
}

void TrackEditor::col_latlon1_clicked(void)
{
	std::cerr << "col_latlon1_clicked" << std::endl;
}

void TrackEditor::notebook_switch_page(guint page_num)
{
	std::cerr << "notebook page " << page_num << std::endl;
	m_trackeditornewtrack->set_sensitive(!page_num);
	trackeditor_trackpoint_selection_changed();
	if (page_num == 3)
		update_statistics();
}

void TrackEditor::hide_notebook_pages(void)
{
	for (int pg = 1;; ++pg) {
		Gtk::Widget *w(m_trackeditornotebook->get_nth_page(pg));
		if (!w)
			break;
		//w->hide();
		w->set_sensitive(false);
	}
}

void TrackEditor::show_notebook_pages(void)
{
	for (int pg = 1;; ++pg) {
		Gtk::Widget *w(m_trackeditornotebook->get_nth_page(pg));
		if (!w)
			break;
		//w->show();
		w->set_sensitive(true);
	}
}

void TrackEditor::update_statistics(void)
{
	m_trackpointeditorstatdist->set_text("");
	m_trackpointeditorstattime->set_text("");
	m_trackpointeditorstatspeedavg->set_text("");
	m_trackpointeditorstatspeedmin->set_text("");
	m_trackpointeditorstatspeedmax->set_text("");
	m_trackpointeditorstataltmin->set_text("");
	m_trackpointeditorstataltmax->set_text("");
	if (!m_track.is_valid() || m_track.size() < 1)
		return;
	m_track.sort();
	int altmin(std::numeric_limits<int>::max());
	int altmax(std::numeric_limits<int>::min());
	int64_t tm(m_track[m_track.size()-1].get_timefrac8() - m_track[0].get_timefrac8());
	for (unsigned int i = 0; i < m_track.size(); i++) {
		const TracksDb::Track::TrackPoint& tp(m_track[i]);
		if (!tp.is_alt_valid())
			continue;
		int alt(tp.get_alt());
		altmin = std::min(altmin, alt);
		altmax = std::max(altmax, alt);
	}
	double vmin(std::numeric_limits<float>::max()), vmax(0), vavg(0), dist(0);
	for (unsigned int i = 1; i < m_track.size(); i++) {
		const TracksDb::Track::TrackPoint& tp0(m_track[i-1]);
		const TracksDb::Track::TrackPoint& tp1(m_track[i]);
		double d(tp0.get_coord().spheric_distance_nmi_dbl(tp1.get_coord()));
		dist += d;
		int64_t tm(tp1.get_timefrac8() - tp0.get_timefrac8());
		//std::cerr << "Pt " << i << " dist " << d << " tm " << tm << " P0 " << tp0.get_coord() << " P1 " << tp1.get_coord() << std::endl;
		if (tm <= 0)
			continue;
		double v(d / tm);
		vmin = std::min(vmin, v);
		vmax = std::max(vmax, v);
	}
	vmin *= (256.0 * 60.0 * 60.0);
	vmax *= (256.0 * 60.0 * 60.0);
	if (tm > 0)
		vavg = dist / tm * (256.0 * 60.0 * 60.0);
	std::cerr << "stat: tm " << tm << std::endl;
	m_trackpointeditorstatdist->set_text(Conversions::dist_str(dist));
	m_trackpointeditorstattime->set_text(Conversions::timefrac8_str("%H:%M:%S.%f", tm));
	if (tm > 0) {
		m_trackpointeditorstatspeedavg->set_text(Conversions::velocity_str(vavg));
		m_trackpointeditorstatspeedmin->set_text(Conversions::velocity_str(vmin));
		m_trackpointeditorstatspeedmax->set_text(Conversions::velocity_str(vmax));
	}
	if (altmin != std::numeric_limits<int>::max())
		m_trackpointeditorstataltmin->set_text(Conversions::alt_str(altmin, 0));
	if (altmax != std::numeric_limits<int>::min())
		m_trackpointeditorstataltmax->set_text(Conversions::alt_str(altmax, 0));
}

void TrackEditor::trackeditor_selection_changed( void )
{
	m_findwindow->hide();
	Glib::RefPtr<Gtk::TreeSelection> trackeditor_selection = m_trackeditortreeview->get_selection();
	std::vector<Gtk::TreeModel::Path> sel(trackeditor_selection->get_selected_rows());
	std::vector<Gtk::TreeModel::Path>::size_type selcount(sel.size());
	Gtk::TreeModel::iterator iter;
	if (selcount > 0)
		iter = m_trackliststore->get_iter(*sel.begin());
	if (selcount > 1) {
		// multiple selections
		std::cerr << "multiple selection" << std::endl;
		m_trackeditordeletetrack->set_sensitive(!m_trackeditornotebook->get_current_page());
		m_trackeditorduplicatetrack->set_sensitive(false);
		m_trackeditormergetracks->set_sensitive(true);
		m_trackeditorexportkml->set_sensitive(true);
		m_trackeditorcompute->set_sensitive(false);
		m_curentryid = 0;
		m_track = TracksDb::Track();
		m_trackeditormap->invalidate_cursor();
		m_trackpoint_model->clear();
		hide_notebook_pages();
		m_trackeditormap->redraw_track();
		return;
	}
	if (!selcount || !iter) {
		std::cerr << "selection cleared" << std::endl;
		m_trackeditordeletetrack->set_sensitive(false);
		m_trackeditorduplicatetrack->set_sensitive(false);
		m_trackeditormergetracks->set_sensitive(false);
		m_trackeditorexportkml->set_sensitive(false);
		m_trackeditorcompute->set_sensitive(false);
		m_curentryid = 0;
		m_track = TracksDb::Track();
		m_trackeditormap->invalidate_cursor();
		m_trackpoint_model->clear();
		hide_notebook_pages();
		m_trackeditormap->redraw_track();
		return;
	}
	Gtk::TreeModel::Row row = *iter;
	//Do something with the row.
	std::cerr << "selected row: " << row[m_tracklistcolumns.m_col_id] << std::endl;
	m_trackeditordeletetrack->set_sensitive(!m_trackeditornotebook->get_current_page());
	m_trackeditorduplicatetrack->set_sensitive(true);
	m_trackeditormergetracks->set_sensitive(false);
	m_trackeditorexportkml->set_sensitive(true);
	m_trackeditorcompute->set_sensitive(!m_trackeditornotebook->get_current_page());
	m_curentryid = row[m_tracklistcolumns.m_col_id];
	m_track = (*m_db)(m_curentryid, true);
	set_lists((*m_db)(m_curentryid));
	show_notebook_pages();
	m_trackeditormap->redraw_track();
}

void TrackEditor::trackeditor_trackpoint_selection_changed(void)
{
	if (!m_curentryid || 2 != m_trackeditornotebook->get_current_page()) {
		m_trackeditorappendtrackpoint->set_sensitive(false);
		m_trackeditorinserttrackpoint->set_sensitive(false);
		m_trackeditordeletetrackpoint->set_sensitive(false);
		m_trackeditorcreatewaypoint->set_sensitive(false);
		return;
	}
	m_trackeditorappendtrackpoint->set_sensitive(true);
	Glib::RefPtr<Gtk::TreeSelection> trackpointeditor_selection = m_trackpointeditortreeview->get_selection();
	std::vector<Gtk::TreeModel::Path> sel(trackpointeditor_selection->get_selected_rows());
	std::vector<Gtk::TreeModel::Path>::size_type selcount(sel.size());
	Gtk::TreeModel::iterator iter;
	if (selcount > 0)
		iter = m_trackpoint_model->get_iter(*sel.begin());
	if (selcount > 1) {
		// multiple selections
		std::cerr << "trackpt: multiple selection" << std::endl;
		m_trackeditorinserttrackpoint->set_sensitive(false);
		m_trackeditordeletetrackpoint->set_sensitive(true);
		m_trackeditorcreatewaypoint->set_sensitive(false);
		return;
	}
	if (!selcount || !iter) {
		std::cerr << "trackpt: selection cleared" << std::endl;
		m_trackeditorinserttrackpoint->set_sensitive(false);
		m_trackeditordeletetrackpoint->set_sensitive(false);
		m_trackeditorcreatewaypoint->set_sensitive(false);
		return;
	}
	Gtk::TreeModel::Row row = *iter;
	//Do something with the row.
	int index(row[m_trackpoint_model_columns.m_col_index]);
	std::cerr << "trackpt: selected row: " << index << std::endl;
	m_trackeditorinserttrackpoint->set_sensitive(true);
	m_trackeditordeletetrackpoint->set_sensitive(true);
	m_trackeditorcreatewaypoint->set_sensitive(true);
	TracksDb::Track e((*m_db)(m_curentryid));
	if (!e.is_valid() || index < 0 || index >= e.size())
		return;
	TracksDb::Track::TrackPoint& tp(e[index]);
	m_trackeditormap->set_center(tp.get_coord(), tp.is_alt_valid() ? tp.get_alt() : 0);
	m_trackeditormap->set_cursor(tp.get_coord());
}

void TrackEditor::aboutdialog_response( int response )
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
		Glib::OptionGroup optgroup("trackeditor", "Track Editor Options", "Options controlling the Track Editor");
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
		osso_context_t* osso_context = osso_initialize("vfrtrackeditor", VERSION, TRUE /* deprecated parameter */, 0 /* Use default Glib main loop context */);
		if (!osso_context) {
			std::cerr << "osso_initialize() failed." << std::endl;
			return OSSO_ERROR;
		}
#endif
#ifdef HAVE_HILDON
		Hildon::init();
#endif
		Glib::set_application_name("VFR Track Editor");
		Glib::thread_init();
		TrackEditor editor(dir_main, auxdbmode, dir_aux);

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
