//
// C++ Implementation: FlightDeckWindow
//
// Description: Flight Deck Windows, Coordinate Dialog
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

FlightDeckWindow::CoordDialog::DistUnitModelColumns::DistUnitModelColumns(void)
{
	add(m_name);
	add(m_factor);
}

FlightDeckWindow::CoordDialog::AltUnitModelColumns::AltUnitModelColumns(void)
{
	add(m_name);
	add(m_id);
}

FlightDeckWindow::CoordDialog::MapModelColumns::MapModelColumns(void)
{
	add(m_name);
	add(m_renderer);
}

const unsigned int FlightDeckWindow::CoordDialog::nrkeybuttons;
const struct FlightDeckWindow::CoordDialog::keybutton FlightDeckWindow::CoordDialog::keybuttons[nrkeybuttons] = {
	{ "coordbutton0", GDK_KEY_KP_0, 90, 1 },
	{ "coordbutton1", GDK_KEY_KP_1, 87, 1 },
	{ "coordbutton2", GDK_KEY_KP_2, 88, 1 },
	{ "coordbutton3", GDK_KEY_KP_3, 89, 1 },
	{ "coordbutton4", GDK_KEY_KP_4, 83, 1 },
	{ "coordbutton5", GDK_KEY_KP_5, 84, 1 },
	{ "coordbutton6", GDK_KEY_KP_6, 85, 1 },
	{ "coordbutton7", GDK_KEY_KP_7, 79, 1 },
	{ "coordbutton8", GDK_KEY_KP_8, 80, 1 },
	{ "coordbutton9", GDK_KEY_KP_9, 81, 1 },
	{ "coordbuttondot", GDK_KEY_KP_Decimal, 91, 1 },
	{ "coordbuttonn", GDK_KEY_N, 57, 1 },
	{ "coordbuttons", GDK_KEY_S, 39, 1 },
	{ "coordbuttone", GDK_KEY_E, 26, 1 },
	{ "coordbuttonw", GDK_KEY_W, 25, 1 }
};

FlightDeckWindow::CoordDialog::CoordDialog(BaseObjectType *castitem, const Glib::RefPtr<Builder>& refxml)
	: Gtk::Window(castitem), m_route(*(FPlan *)0), m_sensors(0), m_flightdeckwindow(0), m_coorddrawingarea(0), m_coordentryicao(0), m_coordentryname(0),
	  m_coordentrycoord(0), m_coordbuttonrevertcoord(0), m_coordframeradialdistance(0), m_coordspinbuttonradial(0), m_coordcheckbuttonradialtrue(0), 
	  m_coordbuttonradial180(0), m_coordbuttonradialdistclear(0), m_coordcomboboxdistunit(0), m_coordspinbuttondist(0), 
	  m_coordentryfinalwgs84(0), m_coordentryfinallocator(0), m_coordentryfinalch1903(0),
	  m_coordframejoin(0), m_coordspinbuttonjoin(0), m_coordcheckbuttonjointrue(0), m_coordbuttonjoin180(0), 
	  m_coordtogglebuttonjoinleg(0), m_coordtogglebuttonjoindirect(0), m_coordspinbuttonalt(0), m_coordcomboboxaltunit(0), 
	  m_coordcomboboxmap(0), m_coordbuttonzoomin(0), m_coordbuttonzoomout(0), m_coordbuttonrevert(0), 
	  m_coordbuttoncancel(0), m_coordbuttonok(0), m_flags(MapRenderer::drawflags_route | MapRenderer::drawflags_route_labels),
	  m_mapscale(0.1), m_arptscale(0.05), m_origalt(5000), m_alt(5000), m_origaltflags(0), m_altflags(0), m_radial(0), m_dist(0),
	  m_jointrack(0), m_legtrack(std::numeric_limits<double>::quiet_NaN()), m_wptnr(~0U), m_mode(mode_off),
	  m_joinmode(joinmode_off), m_coordchange(false)
{
#ifndef GLIBMM_DEFAULT_SIGNAL_HANDLERS_ENABLED
        signal_delete_event().connect(sigc::mem_fun(*this, &CoordDialog::on_delete_event));
        signal_show().connect(sigc::mem_fun(*this, &CoordDialog::on_show));
        signal_hide().connect(sigc::mem_fun(*this, &CoordDialog::on_hide));
#endif //GLIBMM_DEFAULT_SIGNAL_HANDLERS_ENABLED
	get_widget_derived(refxml, "coorddrawingarea", m_coorddrawingarea);
	get_widget(refxml, "coordentryicao", m_coordentryicao);
	get_widget(refxml, "coordentryname", m_coordentryname);
	get_widget(refxml, "coordentrycoord", m_coordentrycoord);
	get_widget(refxml, "coordbuttonrevertcoord", m_coordbuttonrevertcoord);
	get_widget(refxml, "coordframeradialdistance", m_coordframeradialdistance);
	get_widget(refxml, "coordspinbuttonradial", m_coordspinbuttonradial);
	get_widget(refxml, "coordcheckbuttonradialtrue", m_coordcheckbuttonradialtrue);
	get_widget(refxml, "coordbuttonradial180", m_coordbuttonradial180);
	get_widget(refxml, "coordbuttonradialdistclear", m_coordbuttonradialdistclear);
	get_widget(refxml, "coordcomboboxdistunit", m_coordcomboboxdistunit);
	get_widget(refxml, "coordspinbuttondist", m_coordspinbuttondist);
	get_widget(refxml, "coordentryfinalwgs84", m_coordentryfinalwgs84);
        get_widget(refxml, "coordentryfinallocator", m_coordentryfinallocator);
	get_widget(refxml, "coordentryfinalch1903", m_coordentryfinalch1903);
	get_widget(refxml, "coordframejoin", m_coordframejoin);
	get_widget(refxml, "coordspinbuttonjoin", m_coordspinbuttonjoin);
	get_widget(refxml, "coordcheckbuttonjointrue", m_coordcheckbuttonjointrue);
	get_widget(refxml, "coordbuttonjoin180", m_coordbuttonjoin180);
	get_widget(refxml, "coordtogglebuttonjoinleg", m_coordtogglebuttonjoinleg);
	get_widget(refxml, "coordtogglebuttonjoindirect", m_coordtogglebuttonjoindirect);
	get_widget(refxml, "coordspinbuttonalt", m_coordspinbuttonalt);
	get_widget(refxml, "coordcomboboxaltunit", m_coordcomboboxaltunit);
	get_widget(refxml, "coordcomboboxmap", m_coordcomboboxmap);
	get_widget(refxml, "coordbuttonzoomin", m_coordbuttonzoomin);
	get_widget(refxml, "coordbuttonzoomout", m_coordbuttonzoomout);
	get_widget(refxml, "coordbuttonrevert", m_coordbuttonrevert);
	get_widget(refxml, "coordbuttoncancel", m_coordbuttoncancel);
	get_widget(refxml, "coordbuttonok", m_coordbuttonok);
	for (unsigned int i = 0; i < nrkeybuttons; ++i) {
		m_keybutton[i] = 0;
		get_widget(refxml, keybuttons[i].widget_name, m_keybutton[i]);
		if (!m_keybutton[i])
			continue;
		m_connkey[i] = m_keybutton[i]->signal_clicked().connect(sigc::bind(sigc::mem_fun(*this, &CoordDialog::key_clicked), i));
		m_keybutton[i]->set_can_focus(false);
	}

        m_distunitstore = Gtk::ListStore::create(m_distunitcolumns);
        {
                Gtk::TreeModel::Row row(*(m_distunitstore->append()));
                row[m_distunitcolumns.m_name] = "nmi";
                row[m_distunitcolumns.m_factor] = 1.0;
                row = *(m_distunitstore->append());
                row[m_distunitcolumns.m_name] = "miles";
                row[m_distunitcolumns.m_factor] = 0.86897624;
                row = *(m_distunitstore->append());
                row[m_distunitcolumns.m_name] = "km";
                row[m_distunitcolumns.m_factor] = Point::km_to_nmi_dbl;
        }
        if (m_coordcomboboxdistunit) {
                m_coordcomboboxdistunit->clear();
                m_coordcomboboxdistunit->set_model(m_distunitstore);
                m_coordcomboboxdistunit->set_entry_text_column(m_distunitcolumns.m_name);
                m_coordcomboboxdistunit->pack_start(m_distunitcolumns.m_name);
        }

        m_altunitstore = Gtk::ListStore::create(m_altunitcolumns);
        {
                Gtk::TreeModel::Row row(*(m_altunitstore->append()));
                row[m_altunitcolumns.m_name] = "ft";
                row[m_altunitcolumns.m_id] = 0;
                row = *(m_altunitstore->append());
                row[m_altunitcolumns.m_name] = "FL";
                row[m_altunitcolumns.m_id] = 1;
                row = *(m_altunitstore->append());
                row[m_altunitcolumns.m_name] = "m";
                row[m_altunitcolumns.m_id] = 2;
        }
        if (m_coordcomboboxaltunit) {
                m_coordcomboboxaltunit->clear();
                m_coordcomboboxaltunit->set_model(m_altunitstore);
                m_coordcomboboxaltunit->set_entry_text_column(m_altunitcolumns.m_name);
                m_coordcomboboxaltunit->pack_start(m_altunitcolumns.m_name);
		m_altunitchangedconn = m_coordcomboboxaltunit->signal_changed().connect(sigc::mem_fun(*this, &CoordDialog::altunit_changed));
        }
	if (m_coordspinbuttonalt)
		m_altchangedconn = m_coordspinbuttonalt->signal_value_changed().connect(sigc::mem_fun(*this, &CoordDialog::alt_changed));
	if (m_coordspinbuttonradial)
		m_radialchangedconn = m_coordspinbuttonradial->signal_value_changed().connect(sigc::mem_fun(*this, &CoordDialog::radial_changed));
	if (m_coordcheckbuttonradialtrue)
		m_radialtruechangedconn = m_coordcheckbuttonradialtrue->signal_toggled().connect(sigc::mem_fun(*this, &CoordDialog::radial_changed));
	if (m_coordcomboboxdistunit) {
		m_coordcomboboxdistunit->set_active(0);
		m_distunitchangedconn = m_coordcomboboxdistunit->signal_changed().connect(sigc::mem_fun(*this, &CoordDialog::dist_changed));
	}
	if (m_coordspinbuttondist)
		m_distchangedconn = m_coordspinbuttondist->signal_value_changed().connect(sigc::mem_fun(*this, &CoordDialog::dist_changed));

	if (m_coordspinbuttonjoin)
		m_joinconn = m_coordspinbuttonjoin->signal_value_changed().connect(sigc::mem_fun(*this, &CoordDialog::join_changed));
	if (m_coordcheckbuttonjointrue)
		m_coordcheckbuttonjointrue->signal_toggled().connect(sigc::mem_fun(*this, &CoordDialog::jointrue_toggled));
	if (m_coordtogglebuttonjoinleg)
		m_joinlegconn = m_coordtogglebuttonjoinleg->signal_toggled().connect(sigc::mem_fun(*this, &CoordDialog::joinleg_toggled));
	if (m_coordtogglebuttonjoindirect)
		m_joindirectconn = m_coordtogglebuttonjoindirect->signal_toggled().connect(sigc::mem_fun(*this, &CoordDialog::joindirect_toggled));

	if (m_coorddrawingarea) {
                m_cursorchangeconn = m_coorddrawingarea->signal_cursor().connect(sigc::mem_fun(*this, &CoordDialog::cursor_change));
		m_coorddrawingarea->set_route(m_route);
	}

	if (m_coordbuttonrevertcoord)
		m_coordbuttonrevertcoord->signal_clicked().connect(sigc::mem_fun(*this, &CoordDialog::buttonrevertcoord_clicked));
	if (m_coordbuttonradial180)
		m_coordbuttonradial180->signal_clicked().connect(sigc::mem_fun(*this, &CoordDialog::buttonradial180_clicked));
	if (m_coordbuttonradialdistclear)
		m_coordbuttonradialdistclear->signal_clicked().connect(sigc::mem_fun(*this, &CoordDialog::buttonradialdistclear_clicked));
	if (m_coordbuttonjoin180)
		m_coordbuttonjoin180->signal_clicked().connect(sigc::mem_fun(*this, &CoordDialog::buttonjoin180_clicked));
	m_mapstore = Gtk::ListStore::create(m_mapcolumns);
	for (VectorMapArea::renderer_t r = VectorMapArea::renderer_vmap; r < VectorMapArea::renderer_none; r = (VectorMapArea::renderer_t)(r + 1)) {
		if (!VectorMapArea::is_renderer_enabled(r))
			continue;
		if (r == VectorMapArea::renderer_bitmap)
			continue;
                Gtk::TreeModel::Row row(*(m_mapstore->append()));
                row[m_mapcolumns.m_name] = to_str(r);
                row[m_mapcolumns.m_renderer] = r;
	}
	if (m_coordcomboboxmap) {
                m_coordcomboboxmap->clear();
                m_coordcomboboxmap->set_model(m_mapstore);
                m_coordcomboboxmap->set_entry_text_column(m_mapcolumns.m_name);
                m_coordcomboboxmap->pack_start(m_mapcolumns.m_name);
		m_coordcomboboxmap->unset_active();
		if (m_coorddrawingarea) {
			VectorMapArea::renderer_t r(m_coorddrawingarea->get_renderer());
			for (Gtk::TreeIter iter(m_mapstore->children().begin()); iter; ++iter) {
				Gtk::TreeModel::Row row(*iter);
				if (row[m_mapcolumns.m_renderer] != r)
					continue;
				m_coordcomboboxmap->set_active(iter);
				break;
			}
		}
		m_coordcomboboxmap->signal_changed().connect(sigc::mem_fun(*this, &CoordDialog::comboboxmap_changed));
        }
	if (m_coordbuttonzoomin)
		m_coordbuttonzoomin->signal_clicked().connect(sigc::mem_fun(*this, &CoordDialog::buttonzoomin_clicked));
	if (m_coordbuttonzoomout)
		m_coordbuttonzoomout->signal_clicked().connect(sigc::mem_fun(*this, &CoordDialog::buttonzoomout_clicked));
	if (m_coordbuttonrevert)
		m_coordbuttonrevert->signal_clicked().connect(sigc::mem_fun(*this, &CoordDialog::buttonrevert_clicked));
	if (m_coordbuttoncancel)
		m_coordbuttoncancel->signal_clicked().connect(sigc::mem_fun(*this, &CoordDialog::buttoncancel_clicked));
	if (m_coordbuttonok)
		m_coordbuttonok->signal_clicked().connect(sigc::mem_fun(*this, &CoordDialog::buttonok_clicked));

	if (m_coordentrycoord) {
		m_coordentryconn = m_coordentrycoord->signal_changed().connect(sigc::mem_fun(*this, &CoordDialog::coordentry_changed));
		m_coordentrycoord->signal_focus_in_event().connect(sigc::mem_fun(*this, &CoordDialog::coordentry_focus_in_event));
		m_coordentrycoord->signal_focus_out_event().connect(sigc::mem_fun(*this, &CoordDialog::coordentry_focus_out_event));
	}

	set_flags(MapRenderer::drawflags_route | MapRenderer::drawflags_route_labels);
}

FlightDeckWindow::CoordDialog::~CoordDialog()
{
}

void FlightDeckWindow::CoordDialog::init(FlightDeckWindow *fdwnd)
{
	if (!fdwnd)
		return;
	return;
	if (m_coordentryicao) {
                m_coordentryicao->signal_focus_in_event().connect(sigc::bind(sigc::bind(sigc::mem_fun(*fdwnd, &FlightDeckWindow::fplpage_focus_in_event), kbd_main), m_coordentryicao));
                m_coordentryicao->signal_focus_out_event().connect(sigc::mem_fun(*fdwnd, &FlightDeckWindow::fplpage_focus_out_event));
        }
	if (m_coordentryname) {
                m_coordentryname->signal_focus_in_event().connect(sigc::bind(sigc::bind(sigc::mem_fun(*fdwnd, &FlightDeckWindow::fplpage_focus_in_event), kbd_main), m_coordentryname));
                m_coordentryname->signal_focus_out_event().connect(sigc::mem_fun(*fdwnd, &FlightDeckWindow::fplpage_focus_out_event));
        }
	if (m_coordentrycoord) {
                m_coordentrycoord->signal_focus_in_event().connect(sigc::bind(sigc::bind(sigc::mem_fun(*fdwnd, &FlightDeckWindow::fplpage_focus_in_event), kbd_full), m_coordentrycoord));
                m_coordentrycoord->signal_focus_out_event().connect(sigc::mem_fun(*fdwnd, &FlightDeckWindow::fplpage_focus_out_event));
        }
}

void FlightDeckWindow::CoordDialog::set_sensors(Sensors *sensors)
{
	if (!sensors)
		return;
	m_sensors = sensors;
	if (m_coorddrawingarea)
		m_coorddrawingarea->set_track(&m_sensors->get_track());
}

void FlightDeckWindow::CoordDialog::set_flightdeckwindow(FlightDeckWindow *flightdeckwindow)
{
	if (!flightdeckwindow)
		return;
	m_flightdeckwindow = flightdeckwindow;
}

void FlightDeckWindow::CoordDialog::set_engine(Engine& eng)
{
	if (m_coorddrawingarea)
		m_coorddrawingarea->set_engine(eng);
}

bool FlightDeckWindow::CoordDialog::on_delete_event(GdkEventAny* event)
{
	m_mode = mode_off;
        hide();
        return true;
}

void FlightDeckWindow::CoordDialog::on_show(void)
{
#ifdef GLIBMM_DEFAULT_SIGNAL_HANDLERS_ENABLED
        Gtk::Window::on_show();
#endif //GLIBMM_DEFAULT_SIGNAL_HANDLERS_ENABLED
        if (m_coorddrawingarea) {
                m_coorddrawingarea->show();
		m_coorddrawingarea->set_renderer(VectorMapArea::renderer_vmap);
		*m_coorddrawingarea = m_flags;
		m_coorddrawingarea->set_map_scale(m_mapscale);
	}
	if (m_coordcomboboxmap && m_mapstore) {
 		m_coordcomboboxmap->unset_active();
		for (Gtk::TreeIter iter(m_mapstore->children().begin()); iter; ++iter) {
			Gtk::TreeModel::Row row(*iter);
			if (row[m_mapcolumns.m_renderer] != VectorMapArea::renderer_vmap)
				continue;
			m_coordcomboboxmap->set_active(iter);
			break;
		}
	}
}

void FlightDeckWindow::CoordDialog::on_hide(void)
{
#ifdef GLIBMM_DEFAULT_SIGNAL_HANDLERS_ENABLED
        Gtk::Window::on_hide();
#endif //GLIBMM_DEFAULT_SIGNAL_HANDLERS_ENABLED
        if (m_coorddrawingarea)
                m_coorddrawingarea->hide();
	m_mode = mode_off;
}

void FlightDeckWindow::CoordDialog::set_flags(MapRenderer::DrawFlags flags)
{
	m_flags = flags;
	if (m_coorddrawingarea)
		*m_coorddrawingarea = m_flags;
}

void FlightDeckWindow::CoordDialog::set_map_scale(float scale, float arptscale)
{
	m_mapscale = scale;
	m_arptscale = arptscale;
	if (!m_coorddrawingarea)
		return;
	switch (m_coorddrawingarea->get_renderer()) {
	case VectorMapArea::renderer_airportdiagram:
		m_coorddrawingarea->set_map_scale(m_arptscale);
		m_arptscale = m_coorddrawingarea->get_map_scale();
		break;

	default:
		m_coorddrawingarea->set_map_scale(m_mapscale);
		m_mapscale = m_coorddrawingarea->get_map_scale();
		break;
	}
}

void FlightDeckWindow::CoordDialog::comboboxmap_changed(void)
{
	if (!m_coorddrawingarea || !m_coordcomboboxmap)
		return;
	Gtk::TreeModel::iterator iter(m_coordcomboboxmap->get_active());
	if (!iter)
		return;
	Gtk::TreeModel::Row row(*iter);
	VectorMapArea::renderer_t renderer(row[m_mapcolumns.m_renderer]);
	if (renderer != VectorMapArea::renderer_none) {
		m_coorddrawingarea->set_renderer(renderer);
		switch (renderer) {
		case VectorMapArea::renderer_airportdiagram:
			m_coorddrawingarea->set_map_scale(m_arptscale);
			m_arptscale = m_coorddrawingarea->get_map_scale();
			break;

		default:
			m_coorddrawingarea->set_map_scale(m_mapscale);
			m_mapscale = m_coorddrawingarea->get_map_scale();
			break;
		}
	} else {
		m_coordcomboboxmap->unset_active();
	}
}

void FlightDeckWindow::CoordDialog::buttonzoomin_clicked(void)
{
	if (!m_coorddrawingarea)
		return;
	m_coorddrawingarea->zoom_in();
	switch (m_coorddrawingarea->get_renderer()) {
	case VectorMapArea::renderer_airportdiagram:
		m_arptscale = m_coorddrawingarea->get_map_scale();
		break;

	default:
		m_mapscale = m_coorddrawingarea->get_map_scale();
		break;
	}
}

void FlightDeckWindow::CoordDialog::buttonzoomout_clicked(void)
{
	if (!m_coorddrawingarea)
		return;
	m_coorddrawingarea->zoom_out();
	switch (m_coorddrawingarea->get_renderer()) {
	case VectorMapArea::renderer_airportdiagram:
		m_arptscale = m_coorddrawingarea->get_map_scale();
		break;

	default:
		m_mapscale = m_coorddrawingarea->get_map_scale();
		break;
	}
}

void FlightDeckWindow::CoordDialog::key_clicked(unsigned int i)
{
 	if (i >= nrkeybuttons)
		return;
	Gtk::Widget *w(get_focus());
	if (!w)
		return;
	for (unsigned int cnt(keybuttons[i].count); cnt > 0; --cnt)
		KeyboardDialog::send_key(w, 0, keybuttons[i].key, keybuttons[i].hwkeycode);
}

void FlightDeckWindow::CoordDialog::coordentry_changed(void)
{
	m_coordchange = true;
}

bool FlightDeckWindow::CoordDialog::coordentry_focus_in_event(GdkEventFocus *event)
{
	m_coordchange = false;
        return false;
}

bool FlightDeckWindow::CoordDialog::coordentry_focus_out_event(GdkEventFocus *event)
{
	if (m_coordchange && m_coordentrycoord) {
		m_wptcoord.set_str(m_coordentrycoord->get_text());
		coord_update();
	}
	m_coordchange = false;
        return false;
}

void FlightDeckWindow::CoordDialog::cursor_change(Point pt, VectorMapArea::cursor_validity_t valid)
{
        if (valid == VectorMapArea::cursor_invalid)
                return;
	m_wptcoord = pt;
	coord_update();
}

void FlightDeckWindow::CoordDialog::coord_update(void)
{
	m_coordentryconn.block();
	if (m_coordentrycoord)
		m_coordentrycoord->set_text(m_wptcoord.get_lat_str() + " " + m_wptcoord.get_lon_str());
	m_coordchange = false;
	Point pt(get_coord());
	if (m_coordentryfinalwgs84)
		m_coordentryfinalwgs84->set_text(pt.get_lat_str() + " " + pt.get_lon_str());
	if (m_coordentryfinallocator)
		m_coordentryfinallocator->set_text(pt.get_locator());
	if (m_coordentryfinalch1903) {
		float x, y;
		if (pt.get_ch1903(x, y))
			m_coordentryfinalch1903->set_text(Glib::ustring::format(std::fixed, std::setw(8), std::setprecision(1), x) + " " +
							  Glib::ustring::format(std::fixed, std::setw(8), std::setprecision(1), y));
		else
			m_coordentryfinalch1903->set_text("");
	}
	if (m_coorddrawingarea) {
		Glib::TimeVal tv;
		tv.assign_current_time();		
 		m_cursorchangeconn.block();
                m_coorddrawingarea->set_center(pt, m_alt, 0, tv.tv_sec);
                m_coorddrawingarea->set_cursor(pt);
		m_cursorchangeconn.unblock();
        }
	m_coordentryconn.unblock();
	if ((m_mode == mode_directto || m_mode == mode_directtofpl) && m_route.get_nrwpt() >= 2)
		m_route[1].set_coord(pt);
	if (m_joinmode != joinmode_off)
		join_update();
}

void FlightDeckWindow::CoordDialog::altunit_changed(void)
{
  	unsigned int unit(0);
	if (m_coordcomboboxaltunit) {
		Gtk::TreeModel::const_iterator iter(m_coordcomboboxaltunit->get_active());
		if (iter) {
			Gtk::TreeModel::Row row(*iter);
			unit = row[m_altunitcolumns.m_id];
		}
	}
	if (unit == 1)
		m_altflags |= FPlanWaypoint::altflag_standard;
	else
		m_altflags &= ~FPlanWaypoint::altflag_standard;
	alt_update();
}

void FlightDeckWindow::CoordDialog::alt_changed(void)
{
	unsigned int unit(0);
	if (m_coordcomboboxaltunit) {
		Gtk::TreeModel::const_iterator iter(m_coordcomboboxaltunit->get_active());
		if (iter) {
			Gtk::TreeModel::Row row(*iter);
			unit = row[m_altunitcolumns.m_id];
		}
		m_coordcomboboxaltunit->set_sensitive(true);
	}
	if (unit > 2)
		unit = 0;
	if (m_coordspinbuttonalt) {
		switch (unit) {
		case 1:
			m_alt = 100.0 * m_coordspinbuttonalt->get_value();
			break;

		case 2:
			m_alt = FPlanWaypoint::m_to_ft * m_coordspinbuttonalt->get_value();
			break;

		default:
			m_alt = m_coordspinbuttonalt->get_value();
			break;
		}
	}
	alt_update();
}

void FlightDeckWindow::CoordDialog::alt_update(void)
{
	m_altunitchangedconn.block();
	m_altchangedconn.block();
	unsigned int unit(0);
	if (m_coordcomboboxaltunit) {
		Gtk::TreeModel::const_iterator iter(m_coordcomboboxaltunit->get_active());
		if (iter) {
			Gtk::TreeModel::Row row(*iter);
			unit = row[m_altunitcolumns.m_id];
		}
		m_coordcomboboxaltunit->set_sensitive(true);
	}
	if (unit > 2)
		unit = 0;
	if (m_altflags & FPlanWaypoint::altflag_standard) {
		unit = 1;
	} else {
		if (unit == 1)
			unit = 0;
	}
	if (m_coordspinbuttonalt) {
		switch (unit) {
		case 1:
			m_coordspinbuttonalt->set_value(m_alt * 0.01);
			m_coordspinbuttonalt->set_digits(2);
			break;

		case 2:
			m_coordspinbuttonalt->set_value(m_alt * FPlanWaypoint::ft_to_m);
			m_coordspinbuttonalt->set_digits(0);
			break;

		default:
			m_coordspinbuttonalt->set_value(m_alt);
			m_coordspinbuttonalt->set_digits(0);
			break;
		}
		m_coordspinbuttonalt->set_sensitive(true);
	}
	if (m_coordcomboboxaltunit) {
		m_coordcomboboxaltunit->set_active(unit);
		m_coordcomboboxaltunit->set_sensitive(true);
	}
	Point pt(get_coord());
	if (m_coorddrawingarea) {
		Glib::TimeVal tv;
		tv.assign_current_time();		
 		m_cursorchangeconn.block();
                m_coorddrawingarea->set_center(pt, m_alt, 0, tv.tv_sec);
                m_coorddrawingarea->set_cursor(pt);
		m_cursorchangeconn.unblock();
        }
	m_altunitchangedconn.unblock();
	m_altchangedconn.unblock();
}

void FlightDeckWindow::CoordDialog::radial_changed(void)
{
	bool mag(true);
	if (m_coordcheckbuttonradialtrue)
		mag = !m_coordcheckbuttonradialtrue->get_active();
	double r(0);
	if (m_coordspinbuttonradial)
		r = m_coordspinbuttonradial->get_value();
	if (mag) {
		WMM wmm;
		time_t curtime = time(0);
		wmm.compute(m_alt * (FPlanWaypoint::ft_to_m / 1000.0), m_wptcoord, curtime);
		if (wmm)
			r += wmm.get_dec();
	}
	if (r >= 360)
		r -= 360;
	if (r < 0)
		r += 360;
	m_radial = r;
	coord_update();
}

void FlightDeckWindow::CoordDialog::dist_changed(void)
{
	double factor(1), dist(0);
	if (m_coordcomboboxdistunit) {
		Gtk::TreeModel::const_iterator iter(m_coordcomboboxdistunit->get_active());
		if (iter) {
			Gtk::TreeModel::Row row(*iter);
			factor = row[m_distunitcolumns.m_factor];
		}
	}
	if (m_coordspinbuttondist)
		dist = m_coordspinbuttondist->get_value();
	m_dist = dist * factor;
	coord_update();
}

void FlightDeckWindow::CoordDialog::buttonrevertcoord_clicked(void)
{
	if (m_coordentryicao)
		m_coordentryicao->set_text(m_wptorigicao);
	if (m_coordentryname)
		m_coordentryname->set_text(m_wptorigname);
        m_wptcoord = m_wptorigcoord;
	coord_update();
}

void FlightDeckWindow::CoordDialog::buttonradial180_clicked(void)
{
	m_radial += 180;
	if (m_radial >= 360)
		m_radial -= 360;
	bool mag(true);
	if (m_coordcheckbuttonradialtrue)
		mag = !m_coordcheckbuttonradialtrue->get_active();
	double r(m_radial);
	if (mag) {
		WMM wmm;
		time_t curtime = time(0);
		wmm.compute(m_alt * (FPlanWaypoint::ft_to_m / 1000.0), m_wptcoord, curtime);
		if (wmm)
			r -= wmm.get_dec();
		if (r >= 360)
			r -= 360;
		if (r < 0)
			r += 360;
	}
	if (m_coordspinbuttonradial) {
		m_radialchangedconn.block();
		m_coordspinbuttonradial->set_value(r);
		m_radialchangedconn.unblock();
	}
	coord_update();
}

void FlightDeckWindow::CoordDialog::buttonradialdistclear_clicked(void)
{
	m_distchangedconn.block();
	if (m_coordspinbuttondist)
		m_coordspinbuttondist->set_value(0);
	m_dist = 0;
	m_distchangedconn.unblock();
	m_radial = 0;
	bool mag(true);
	if (m_coordcheckbuttonradialtrue)
		mag = !m_coordcheckbuttonradialtrue->get_active();
	if (mag) {
		WMM wmm;
		time_t curtime = time(0);
		wmm.compute(m_alt * (FPlanWaypoint::ft_to_m / 1000.0), m_wptcoord, curtime);
		if (wmm)
			m_radial = wmm.get_dec();
		if (m_radial >= 360)
			m_radial -= 360;
		if (m_radial < 0)
			m_radial += 360;
	}
	m_radialchangedconn.block();
	m_coordspinbuttonradial->set_value(0);
	m_radialchangedconn.unblock();
	coord_update();
}

void FlightDeckWindow::CoordDialog::join_update(bool forceupdjoin)
{
	if (m_joinmode == joinmode_off) {
		if (m_coordframejoin)
			m_coordframejoin->set_visible(false);
		return;
	}
	if (m_coordtogglebuttonjoinleg) {
		m_joinlegconn.block();
		m_coordtogglebuttonjoinleg->set_active(m_joinmode == joinmode_leg);
		m_coordtogglebuttonjoinleg->set_sensitive(!std::isnan(m_legtrack));
		m_joinlegconn.unblock();
	}
	if (m_coordtogglebuttonjoindirect) {
		m_joindirectconn.block();
		m_coordtogglebuttonjoindirect->set_active(m_joinmode == joinmode_direct);
		m_joindirectconn.unblock();
	}
	if (m_joinmode == joinmode_leg) {
		m_jointrack = m_legtrack;
	} else if (m_joinmode == joinmode_direct && m_sensors) {
		Point pt;
		m_sensors->get_position(pt);
		m_jointrack = pt.spheric_true_course(m_wptcoord);
	}
	if (forceupdjoin || m_joinmode != joinmode_track) {
		bool mag(true);
		if (m_coordcheckbuttonjointrue)
			mag = !m_coordcheckbuttonjointrue->get_active();
		double r(m_jointrack);
		if (mag) {
			WMM wmm;
			time_t curtime = time(0);
			wmm.compute(m_alt * (FPlanWaypoint::ft_to_m / 1000.0), m_wptcoord, curtime);
			if (wmm)
				r -= wmm.get_dec();
			if (r >= 360)
				r -= 360;
			if (r < 0)
				r += 360;
		}
		if (m_coordspinbuttonjoin) {
			m_joinconn.block();
			m_coordspinbuttonjoin->set_value(r);
			m_joinconn.unblock();
		}
	}
	if ((m_mode == mode_directto || m_mode == mode_directtofpl) && m_route.get_nrwpt() >= 2) {
		m_route[0].set_coord(m_route[1].get_coord().spheric_course_distance_nmi(m_jointrack + 180.0, 100.0));
		if (m_coorddrawingarea)
			m_coorddrawingarea->redraw_route();
	}
}

void FlightDeckWindow::CoordDialog::join_changed(void)
{
	m_joinmode = joinmode_track;
	bool mag(true);
	if (m_coordcheckbuttonjointrue)
		mag = !m_coordcheckbuttonjointrue->get_active();
	double r(0);
	if (m_coordspinbuttonjoin)
		r = m_coordspinbuttonjoin->get_value();
	if (mag) {
		WMM wmm;
		time_t curtime = time(0);
		wmm.compute(m_alt * (FPlanWaypoint::ft_to_m / 1000.0), m_wptcoord, curtime);
		if (wmm)
			r += wmm.get_dec();
	}
	if (r >= 360)
		r -= 360;
	if (r < 0)
		r += 360;
	m_jointrack = r;
	join_update();
}

void FlightDeckWindow::CoordDialog::jointrue_toggled(void)
{
	if (m_joinmode == joinmode_track) {
		join_changed();
		return;
	}
	join_update();
}

void FlightDeckWindow::CoordDialog::joinleg_toggled(void)
{
	if (m_coordtogglebuttonjoinleg && m_coordtogglebuttonjoinleg->get_active() && !std::isnan(m_legtrack))
		m_joinmode = joinmode_leg;
	join_update();
}

void FlightDeckWindow::CoordDialog::joindirect_toggled(void)
{
	if (m_coordtogglebuttonjoindirect && m_coordtogglebuttonjoindirect->get_active())
		m_joinmode = joinmode_direct;
	join_update();
}

void FlightDeckWindow::CoordDialog::buttonjoin180_clicked(void)
{
	m_joinmode = joinmode_track;
	m_jointrack += 180;
	if (m_jointrack >= 360)
		m_jointrack -= 360;
	join_update(true);
}

void FlightDeckWindow::CoordDialog::coord_updatepos(void)
{
	if (m_joinmode == joinmode_direct)
		join_update();
}

void FlightDeckWindow::CoordDialog::buttonrevert_clicked(void)
{
	buttonrevertcoord_clicked();
	buttonradialdistclear_clicked();
}

void FlightDeckWindow::CoordDialog::buttoncancel_clicked(void)
{
	m_mode = mode_off;
        hide();
}

void FlightDeckWindow::CoordDialog::buttonok_clicked(void)
{
	switch (m_mode) {
	case mode_wpt:
	{
		if (!m_sensors)
			break;
		FPlanRoute& fpl(m_sensors->get_route());
		if (m_wptnr >= fpl.get_nrwpt())
			break;
		FPlanWaypoint& wpt(fpl[m_wptnr]);
		if (m_coordentryicao)
			wpt.set_icao(m_coordentryicao->get_text());
		if (m_coordentryname)
			wpt.set_name(m_coordentryname->get_text());
		wpt.set_coord(get_coord());
		wpt.set_altitude(m_alt);
		wpt.set_flags((wpt.get_flags() & ~FPlanWaypoint::altflag_standard) |
			      (m_altflags & FPlanWaypoint::altflag_standard));
		wpt.set_type(FPlanWaypoint::type_user);
		m_sensors->nav_fplan_modified();
		break;
	}

	case mode_directto:
	case mode_directtofpl:
	{
		if (!m_sensors)
			break;
		Glib::ustring name;
		if (m_coordentryicao)
			name = m_coordentryicao->get_text();
		if (m_coordentryname) {
			Glib::ustring s(m_coordentryname->get_text());
			if (!name.empty() && !s.empty())
				name += " ";
			name += s;
		}
		double track(std::numeric_limits<double>::quiet_NaN());
		if (m_joinmode != joinmode_direct)
			track = m_jointrack;
		uint16_t wptnr(~0U);
		if (m_mode == mode_directtofpl)
			wptnr = m_wptnr + 1;
		m_sensors->nav_directto(name, get_coord(), track, m_alt, m_altflags & FPlanWaypoint::altflag_standard, wptnr);
		if (m_flightdeckwindow)
			m_flightdeckwindow->directto_activated();
		break;
	}

	default:
		break;
	};
	m_mode = mode_off;
        hide();
}

void FlightDeckWindow::CoordDialog::edit_waypoint(void)
{
	std::cerr << "edit_waypoint" << std::endl;
	hide();
	if (!m_sensors)
		return;
	const FPlanRoute& fpl(m_sensors->get_route());
	unsigned int nr(fpl.get_curwpt());
	if (nr < 1)
		return;
	edit_waypoint(nr - 1);
}

void FlightDeckWindow::CoordDialog::edit_waypoint(unsigned int nr)
{
	hide();
	if (!m_sensors)
		return;
	const FPlanRoute& fpl(m_sensors->get_route());
	if (nr >= fpl.get_nrwpt())
		return;
	m_mode = mode_wpt;
	const FPlanWaypoint& wpt(fpl[nr]);
	m_wptnr = nr;
	m_wptorigname = wpt.get_name();
	m_wptorigicao = wpt.get_icao();
        m_wptorigcoord = wpt.get_coord();
	m_origalt = wpt.get_altitude();
	m_origaltflags = wpt.get_flags();
	m_legtrack = std::numeric_limits<double>::quiet_NaN();
	m_route.new_fp();
	for (unsigned int i = 0; i < fpl.get_nrwpt(); ++i)
		m_route.insert_wpt(~0U, fpl[i]);
	showdialog();
}

void FlightDeckWindow::CoordDialog::directto_waypoint(void)
{
	hide();
	if (!m_sensors)
		return;
	const FPlanRoute& fpl(m_sensors->get_route());
	unsigned int nr(fpl.get_curwpt());
	if (nr < 1)
		return;
	directto_waypoint(nr - 1);
}

void FlightDeckWindow::CoordDialog::directto_waypoint(unsigned int nr)
{
	hide();
	if (!m_sensors)
		return;
	const FPlanRoute& fpl(m_sensors->get_route());
	if (nr >= fpl.get_nrwpt())
		return;
	m_mode = mode_directtofpl;
	const FPlanWaypoint& wpt(fpl[nr]);
	m_wptnr = nr;
	m_wptorigname = wpt.get_name();
	m_wptorigicao = wpt.get_icao();
        m_wptorigcoord = wpt.get_coord();
	m_origalt = wpt.get_altitude();
	m_origaltflags = wpt.get_flags();
	m_legtrack = std::numeric_limits<double>::quiet_NaN();
	if (nr > 0) {
		const FPlanWaypoint& wpt1(fpl[nr - 1]);
		m_legtrack = wpt1.get_coord().spheric_true_course(wpt.get_coord());
	}
	m_route.new_fp();
	m_route.insert_wpt(~0U, wpt);
	for (unsigned int i = nr; i < fpl.get_nrwpt(); ++i)
		m_route.insert_wpt(~0U, fpl[i]);
	showdialog();
}

void FlightDeckWindow::CoordDialog::directto(const Point& pt, const Glib::ustring& name, const Glib::ustring& icao, double legtrack)
{
	hide();
	if (!m_sensors)
		return;
	m_mode = mode_directto;
	m_wptnr = ~0U;
	m_wptorigname = name;
	m_wptorigicao = icao;
        m_wptorigcoord = pt;
	//m_origalt = 5000;
	//m_origaltflags = 0;
	m_legtrack = legtrack;
	m_route.new_fp();
	{
		FPlanWaypoint wpt;
		wpt.set_icao(icao);
		wpt.set_name(name);
		wpt.set_coord(pt);
		m_route.insert_wpt(~0U, wpt);
		m_route.insert_wpt(~0U, wpt);
		m_route[0].set_name("");
		m_route[0].set_icao("");
	}
	showdialog();
}

void FlightDeckWindow::CoordDialog::showdialog(void)
{
	if (m_coordentryicao)
		m_coordentryicao->set_text(m_wptorigicao);
	if (m_coordentryname)
		m_coordentryname->set_text(m_wptorigname);
	m_wptcoord = m_wptorigcoord;
	m_alt = m_origalt;
	m_altflags = m_origaltflags;

	m_distchangedconn.block();
	if (m_coordspinbuttondist)
		m_coordspinbuttondist->set_value(0);
	m_dist = 0;
	m_distchangedconn.unblock();

	if (m_coordframeradialdistance)
		m_coordframeradialdistance->set_visible(m_mode == mode_directto || m_mode == mode_wpt);

	bool enablejoin(m_mode == mode_directto || m_mode == mode_directtofpl);
	if (m_coordframejoin)
		m_coordframejoin->set_visible(enablejoin);
	m_joinmode = joinmode_off;
	if (enablejoin) {
		m_joinmode = joinmode_direct;
		m_jointrack = 0;
		if (!std::isnan(m_legtrack)) {
			m_jointrack = m_legtrack;
			m_joinmode = joinmode_leg;
		}
		join_update();
	}

	buttonradialdistclear_clicked();
	alt_update();
	show();
}

Point FlightDeckWindow::CoordDialog::get_coord(void) const
{
	return m_wptcoord.spheric_course_distance_nmi(m_radial, m_dist);
}

void FlightDeckWindow::CoordDialog::set_altitude(int32_t alt, uint16_t altflags)
{
	m_origalt = m_alt = alt;
	m_origaltflags = m_altflags = altflags;
	alt_update();
}
