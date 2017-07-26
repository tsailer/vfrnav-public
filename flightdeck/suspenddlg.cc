//
// C++ Implementation: FlightDeckWindow
//
// Description: Flight Deck Windows, Suspend Dialog
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

FlightDeckWindow::SuspendDialog::MapModelColumns::MapModelColumns(void)
{
	add(m_name);
	add(m_renderer);
}

const unsigned int FlightDeckWindow::SuspendDialog::nrkeybuttons;
const struct FlightDeckWindow::SuspendDialog::keybutton FlightDeckWindow::SuspendDialog::keybuttons[nrkeybuttons] = {
	{ "suspendbutton0", GDK_KEY_KP_0, 90, 1 },
	{ "suspendbutton1", GDK_KEY_KP_1, 87, 1 },
	{ "suspendbutton2", GDK_KEY_KP_2, 88, 1 },
	{ "suspendbutton3", GDK_KEY_KP_3, 89, 1 },
	{ "suspendbutton4", GDK_KEY_KP_4, 83, 1 },
	{ "suspendbutton5", GDK_KEY_KP_5, 84, 1 },
	{ "suspendbutton6", GDK_KEY_KP_6, 85, 1 },
	{ "suspendbutton7", GDK_KEY_KP_7, 79, 1 },
	{ "suspendbutton8", GDK_KEY_KP_8, 80, 1 },
	{ "suspendbutton9", GDK_KEY_KP_9, 81, 1 },
	{ "suspendbuttondot", GDK_KEY_KP_Decimal, 91, 1 }
};

FlightDeckWindow::SuspendDialog::SuspendDialog(BaseObjectType *castitem, const Glib::RefPtr<Builder>& refxml)
	: Gtk::Window(castitem), m_route(*(FPlan *)0), m_sensors(0), m_suspenddrawingarea(0), m_suspendspinbutton(0), m_suspendcheckbuttontrue(0),
	  m_suspendbutton180(0), m_suspendbuttoncancel(0), m_suspendbuttonok(0), m_suspendcomboboxmap(0), m_suspendbuttonzoomin(0), m_suspendbuttonzoomout(0), 
	  m_track(0), m_flags(MapRenderer::drawflags_route | MapRenderer::drawflags_route_labels), m_mapscale(0.1), m_arptscale(0.05)
{
#ifndef GLIBMM_DEFAULT_SIGNAL_HANDLERS_ENABLED
        signal_delete_event().connect(sigc::mem_fun(*this, &SuspendDialog::on_delete_event));
        signal_show().connect(sigc::mem_fun(*this, &SuspendDialog::on_show));
        signal_hide().connect(sigc::mem_fun(*this, &SuspendDialog::on_hide));
#endif //GLIBMM_DEFAULT_SIGNAL_HANDLERS_ENABLED
	get_widget_derived(refxml, "suspenddrawingarea", m_suspenddrawingarea);
	get_widget(refxml, "suspendspinbutton", m_suspendspinbutton);
	get_widget(refxml, "suspendcheckbuttontrue", m_suspendcheckbuttontrue);
	get_widget(refxml, "suspendbutton180", m_suspendbutton180);
	get_widget(refxml, "suspendbuttoncancel", m_suspendbuttoncancel);
	get_widget(refxml, "suspendbuttonok", m_suspendbuttonok);
	get_widget(refxml, "suspendcomboboxmap", m_suspendcomboboxmap);
	get_widget(refxml, "suspendbuttonzoomin", m_suspendbuttonzoomin);
	get_widget(refxml, "suspendbuttonzoomout", m_suspendbuttonzoomout);
	for (unsigned int i = 0; i < nrkeybuttons; ++i) {
		m_keybutton[i] = 0;
		get_widget(refxml, keybuttons[i].widget_name, m_keybutton[i]);
		if (!m_keybutton[i])
			continue;
		m_connkey[i] = m_keybutton[i]->signal_clicked().connect(sigc::bind(sigc::mem_fun(*this, &SuspendDialog::key_clicked), i));
		m_keybutton[i]->set_can_focus(false);
	}
	if (m_suspenddrawingarea)
		m_suspenddrawingarea->set_route(m_route);
	if (m_suspendspinbutton)
		m_connspinbutton = m_suspendspinbutton->signal_value_changed().connect(sigc::mem_fun(*this, &SuspendDialog::track_changed));
	if (m_suspendcheckbuttontrue)
		m_suspendcheckbuttontrue->signal_toggled().connect(sigc::mem_fun(*this, &SuspendDialog::buttontrue_toggled));
	if (m_suspendbutton180)
		m_suspendbutton180->signal_clicked().connect(sigc::mem_fun(*this, &SuspendDialog::button180_clicked));
	if (m_suspendbuttoncancel)
		m_suspendbuttoncancel->signal_clicked().connect(sigc::mem_fun(*this, &SuspendDialog::hide));
	if (m_suspendbuttonok)
		m_suspendbuttonok->signal_clicked().connect(sigc::mem_fun(*this, &SuspendDialog::buttonok_clicked));
	m_mapstore = Gtk::ListStore::create(m_mapcolumns);
	for (VectorMapArea::renderer_t r = VectorMapArea::renderer_vmap; r < VectorMapArea::renderer_none; r = (VectorMapArea::renderer_t)(r + 1)) {
		if (!VectorMapArea::is_renderer_enabled(r))
			continue;
                Gtk::TreeModel::Row row(*(m_mapstore->append()));
                row[m_mapcolumns.m_name] = to_str(r);
                row[m_mapcolumns.m_renderer] = r;
	}
	if (m_suspendcomboboxmap) {
                m_suspendcomboboxmap->clear();
                m_suspendcomboboxmap->set_model(m_mapstore);
                m_suspendcomboboxmap->set_entry_text_column(m_mapcolumns.m_name);
                m_suspendcomboboxmap->pack_start(m_mapcolumns.m_name);
		m_suspendcomboboxmap->unset_active();
		if (m_suspenddrawingarea) {
			VectorMapArea::renderer_t r(m_suspenddrawingarea->get_renderer());
			for (Gtk::TreeIter iter(m_mapstore->children().begin()); iter; ++iter) {
				Gtk::TreeModel::Row row(*iter);
				if (row[m_mapcolumns.m_renderer] != r)
					continue;
				m_suspendcomboboxmap->set_active(iter);
				break;
			}
		}
		m_suspendcomboboxmap->signal_changed().connect(sigc::mem_fun(*this, &SuspendDialog::comboboxmap_changed));
        }
	if (m_suspendbuttonzoomin)
		m_suspendbuttonzoomin->signal_clicked().connect(sigc::mem_fun(*this, &SuspendDialog::buttonzoomin_clicked));
	if (m_suspendbuttonzoomout)
		m_suspendbuttonzoomout->signal_clicked().connect(sigc::mem_fun(*this, &SuspendDialog::buttonzoomout_clicked));
	set_flags(MapRenderer::drawflags_route | MapRenderer::drawflags_route_labels);
}

FlightDeckWindow::SuspendDialog::~SuspendDialog()
{
}

void FlightDeckWindow::SuspendDialog::set_sensors(Sensors *sensors)
{
	if (!sensors)
		return;
	m_sensors = sensors;
	if (m_suspenddrawingarea)
		m_suspenddrawingarea->set_track(&m_sensors->get_track());
}

void FlightDeckWindow::SuspendDialog::set_engine(Engine& eng)
{
	if (m_suspenddrawingarea)
		m_suspenddrawingarea->set_engine(eng);
}

bool FlightDeckWindow::SuspendDialog::on_delete_event(GdkEventAny* event)
{
        hide();
        return true;
}

void FlightDeckWindow::SuspendDialog::on_show(void)
{
#ifdef GLIBMM_DEFAULT_SIGNAL_HANDLERS_ENABLED
        Gtk::Window::on_show();
#endif //GLIBMM_DEFAULT_SIGNAL_HANDLERS_ENABLED
	if (!m_sensors || !m_sensors->nav_is_on()) {
		hide();
		return;
	}
	m_track = m_sensors->nav_get_hold_track();
	m_route.new_fp();
	if (m_sensors->nav_is_fpl()) {
		const FPlanRoute& fpl(m_sensors->get_route());
		unsigned int nr(m_sensors->nav_get_wptnr());
		if (nr >= 1 && nr <= fpl.get_nrwpt()) {
			FPlanWaypoint wpt(fpl[nr - 1]);
			wpt.set_coord(m_sensors->nav_get_curcoord());
			wpt.set_altitude(m_sensors->nav_get_curalt());
			wpt.set_flags((wpt.get_flags() & ~FPlanWaypoint::altflag_standard) | (m_sensors->nav_get_curflags() & FPlanWaypoint::altflag_standard));
			m_route.insert_wpt(~0U, wpt);
			m_route.insert_wpt(~0U, wpt);
			for (; nr < fpl.get_nrwpt(); ++nr)
				m_route.insert_wpt(~0U, fpl[nr]);
		}
	}
	if (m_route.get_nrwpt() <= 0) {
		FPlanWaypoint wpt;
		wpt.set_coord(m_sensors->nav_get_curcoord());
		wpt.set_name(m_sensors->nav_get_curdest());
		wpt.set_altitude(m_sensors->nav_get_curalt());
		wpt.set_flags(m_sensors->nav_get_curflags());
		m_route.insert_wpt(~0U, wpt);
		m_route.insert_wpt(~0U, wpt);
	}
	if (m_route.get_nrwpt() > 0) {
		m_route[0].set_icao("");
		m_route[0].set_name("");
	}
	track_update();
        if (m_suspenddrawingarea) {
                m_suspenddrawingarea->show();
		m_suspenddrawingarea->set_renderer(VectorMapArea::renderer_vmap);
		*m_suspenddrawingarea = m_flags;
		m_suspenddrawingarea->set_map_scale(m_mapscale);
		if (m_sensors) {
			Glib::TimeVal tv;
			tv.assign_current_time();
			Point pt(m_sensors->nav_get_curcoord());
			m_suspenddrawingarea->set_center(pt, m_sensors->nav_get_curalt(), 0, tv.tv_sec);
			m_suspenddrawingarea->set_cursor(pt);
		}
	}
	if (m_suspendcomboboxmap && m_mapstore) {
 		m_suspendcomboboxmap->unset_active();
		for (Gtk::TreeIter iter(m_mapstore->children().begin()); iter; ++iter) {
			Gtk::TreeModel::Row row(*iter);
			if (row[m_mapcolumns.m_renderer] != VectorMapArea::renderer_vmap)
				continue;
			m_suspendcomboboxmap->set_active(iter);
			break;
		}
	}
}

void FlightDeckWindow::SuspendDialog::on_hide(void)
{
#ifdef GLIBMM_DEFAULT_SIGNAL_HANDLERS_ENABLED
        Gtk::Window::on_hide();
#endif //GLIBMM_DEFAULT_SIGNAL_HANDLERS_ENABLED
        if (m_suspenddrawingarea)
                m_suspenddrawingarea->hide();
}

void FlightDeckWindow::SuspendDialog::set_flags(MapRenderer::DrawFlags flags)
{
	m_flags = flags;
	if (m_suspenddrawingarea)
		*m_suspenddrawingarea = m_flags;
}

void FlightDeckWindow::SuspendDialog::set_map_scale(float scale, float arptscale)
{
	m_mapscale = scale;
	m_arptscale = arptscale;
	if (!m_suspenddrawingarea)
		return;
	switch (m_suspenddrawingarea->get_renderer()) {
	case VectorMapArea::renderer_airportdiagram:
		m_suspenddrawingarea->set_map_scale(m_arptscale);
		m_arptscale = m_suspenddrawingarea->get_map_scale();
		break;

	default:
		m_suspenddrawingarea->set_map_scale(m_mapscale);
		m_mapscale = m_suspenddrawingarea->get_map_scale();
		break;
	}
}

void FlightDeckWindow::SuspendDialog::comboboxmap_changed(void)
{
	if (!m_suspenddrawingarea || !m_suspendcomboboxmap)
		return;
	Gtk::TreeModel::iterator iter(m_suspendcomboboxmap->get_active());
	if (!iter)
		return;
	Gtk::TreeModel::Row row(*iter);
	VectorMapArea::renderer_t renderer(row[m_mapcolumns.m_renderer]);
	if (renderer != VectorMapArea::renderer_none) {
		m_suspenddrawingarea->set_renderer(renderer);
		switch (renderer) {
		case VectorMapArea::renderer_airportdiagram:
			m_suspenddrawingarea->set_map_scale(m_arptscale);
			m_arptscale = m_suspenddrawingarea->get_map_scale();
			break;

		default:
			m_suspenddrawingarea->set_map_scale(m_mapscale);
			m_mapscale = m_suspenddrawingarea->get_map_scale();
			break;
		}
	} else {
		m_suspendcomboboxmap->unset_active();
	}
}

void FlightDeckWindow::SuspendDialog::buttonzoomin_clicked(void)
{
	if (!m_suspenddrawingarea)
		return;
	m_suspenddrawingarea->zoom_in();
	switch (m_suspenddrawingarea->get_renderer()) {
	case VectorMapArea::renderer_airportdiagram:
		m_arptscale = m_suspenddrawingarea->get_map_scale();
		break;

	default:
		m_mapscale = m_suspenddrawingarea->get_map_scale();
		break;
	}
}

void FlightDeckWindow::SuspendDialog::buttonzoomout_clicked(void)
{
	if (!m_suspenddrawingarea)
		return;
	m_suspenddrawingarea->zoom_out();
	switch (m_suspenddrawingarea->get_renderer()) {
	case VectorMapArea::renderer_airportdiagram:
		m_arptscale = m_suspenddrawingarea->get_map_scale();
		break;

	default:
		m_mapscale = m_suspenddrawingarea->get_map_scale();
		break;
	}
}

void FlightDeckWindow::SuspendDialog::key_clicked(unsigned int i)
{
 	if (i >= nrkeybuttons)
		return;
	Gtk::Widget *w(get_focus());
	if (!w)
		return;
	for (unsigned int cnt(keybuttons[i].count); cnt > 0; --cnt)
		KeyboardDialog::send_key(w, 0, keybuttons[i].key, keybuttons[i].hwkeycode);
}

void FlightDeckWindow::SuspendDialog::track_update(bool updspin)
{
	bool mag(true);
	if (m_suspendcheckbuttontrue)
		mag = !m_suspendcheckbuttontrue->get_active();
	double r(m_track);
	if (mag && m_sensors) {
		WMM wmm;
		time_t curtime = time(0);
		wmm.compute(m_sensors->nav_get_curalt() * (FPlanWaypoint::ft_to_m / 1000.0), m_sensors->nav_get_curcoord(), curtime);
		if (wmm)
			r -= wmm.get_dec();
		if (r >= 360)
			r -= 360;
		if (r < 0)
			r += 360;
	}
	if (updspin && m_suspendspinbutton) {
		m_connspinbutton.block();
		m_suspendspinbutton->set_value(r);
		m_connspinbutton.unblock();
	}
	if (m_route.get_nrwpt() >= 2) {
		m_route[0].set_coord(m_route[1].get_coord().spheric_course_distance_nmi(m_track + 180.0, 5.0));
		if (m_suspenddrawingarea)
			m_suspenddrawingarea->redraw_route();
	}
}

void FlightDeckWindow::SuspendDialog::track_changed(void)
{
	bool mag(true);
	if (m_suspendcheckbuttontrue)
		mag = !m_suspendcheckbuttontrue->get_active();
	double r(0);
	if (m_suspendspinbutton)
		r = m_suspendspinbutton->get_value();
	if (mag && m_sensors) {
		WMM wmm;
		time_t curtime = time(0);
		wmm.compute(m_sensors->nav_get_curalt() * (FPlanWaypoint::ft_to_m / 1000.0), m_sensors->nav_get_curcoord(), curtime);
		if (wmm)
			r += wmm.get_dec();
	}
	if (r >= 360)
		r -= 360;
	if (r < 0)
		r += 360;
	m_track = r;
	track_update(false);
}

void FlightDeckWindow::SuspendDialog::buttontrue_toggled(void)
{
	track_update();
}

void FlightDeckWindow::SuspendDialog::button180_clicked(void)
{
	m_track += 180;
	if (m_track >= 360)
		m_track -= 360;
	track_update();
}

void FlightDeckWindow::SuspendDialog::buttonok_clicked(void)
{
	if (m_sensors)
		m_sensors->nav_set_hold(m_track);
	hide();
}
