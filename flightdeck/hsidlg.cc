//
// C++ Implementation: FlightDeckWindow
//
// Description: Flight Deck Windows, HSI Dialog
//
//
// Author: Thomas Sailer <t.sailer@alumni.ethz.ch>, (C) 2011, 2012, 2016
//
// Copyright: See COPYING file that comes with this distribution
//
//

#include "sysdeps.h"
#include "sysdepsgui.h"
#include <iomanip>

#include "flightdeckwindow.h"

void FlightDeckWindow::hsi_dialog_open(void)
{
	if (!m_hsidialog)
		return;
	if (m_sensors) {
		m_hsidialog->set_heading(m_sensors->get_manualheading());
		m_hsidialog->set_heading_true(m_sensors->is_manualheading_true());
		m_hsidialog->set_rat(m_sensors->get_airdata_rat());
		m_hsidialog->set_ias(m_sensors->get_airdata_cas());
		m_hsidialog->set_cruiserpm(m_sensors->get_acft_cruise().get_rpm());
		m_hsidialog->set_cruisemp(m_sensors->get_acft_cruise().get_mp());
		m_hsidialog->set_cruisebhp(m_sensors->get_acft_cruise().get_bhp());
		m_hsidialog->set_constantspeed(m_sensors->get_aircraft().is_constantspeed());
		m_hsidialog->set_iasmode(m_sensors->get_acft_iasmode());
		m_hsidialog->set_maxbhp(m_sensors->get_aircraft().get_maxbhp());
	}
	m_hsidialog->set_flags(m_navarea.hsi_get_drawflags());
	m_hsidialog->set_displaywind(m_navarea.hsi_get_wind());
	m_hsidialog->set_displayterrain(m_navarea.hsi_get_terrain());
	m_hsidialog->set_displayglide(m_navarea.hsi_get_glide());
	m_hsidialog->show();
}

bool FlightDeckWindow::hsi_indicator_button_press_event(GdkEventButton *event)
{
	unsigned int button = event->button;
	if (button != 1)
		return false;
	hsi_dialog_open();
        return false;
}

void FlightDeckWindow::hsi_settings_update(void)
{
	if (!m_hsidialog)
		return;
	if (m_sensors) {
		if (m_hsidialog->is_heading_changed())
			m_sensors->set_manualheading(m_hsidialog->get_heading(), m_hsidialog->is_heading_true());
		if (m_hsidialog->is_rat_changed() || std::isnan(m_sensors->get_airdata_rat()))
			m_sensors->set_airdata_rat(m_hsidialog->get_rat());
		if (m_hsidialog->is_ias_changed() || std::isnan(m_sensors->get_airdata_cas()))
			m_sensors->set_airdata_cas(m_hsidialog->get_ias());
		Aircraft::Cruise::EngineParams ep(m_sensors->get_acft_cruise());
		if (m_hsidialog->is_cruiserpm_changed() || std::isnan(ep.get_rpm()))
			ep.set_rpm(m_hsidialog->get_cruiserpm());
		if (m_hsidialog->is_cruisemp_changed() || std::isnan(ep.get_mp()))
			ep.set_mp(m_hsidialog->get_cruisemp());
		if (m_hsidialog->is_cruisebhp_changed() || std::isnan(ep.get_bhp()))
			ep.set_bhp(m_hsidialog->get_cruisebhp());
		m_sensors->set_acft_cruise(ep);
	        m_sensors->set_acft_iasmode(m_hsidialog->get_iasmode());
	}
	m_navarea.hsi_set_drawflags(m_hsidialog->get_flags());
	m_navarea.hsi_set_wind(m_hsidialog->is_displaywind());
	m_navarea.hsi_set_terrain(m_hsidialog->is_displayterrain());
	m_navarea.hsi_set_glide(m_hsidialog->is_displayglide());
	if (m_sensors)
		m_navarea.sensors_change(m_sensors, Sensors::change_heading);
}

const unsigned int FlightDeckWindow::HSIDialog::nrkeybuttons;
const struct FlightDeckWindow::HSIDialog::keybutton FlightDeckWindow::HSIDialog::keybuttons[nrkeybuttons] = {
	{ "hsibutton0", GDK_KEY_KP_0, 90, 1 },
	{ "hsibutton1", GDK_KEY_KP_1, 87, 1 },
	{ "hsibutton2", GDK_KEY_KP_2, 88, 1 },
	{ "hsibutton3", GDK_KEY_KP_3, 89, 1 },
	{ "hsibutton4", GDK_KEY_KP_4, 83, 1 },
	{ "hsibutton5", GDK_KEY_KP_5, 84, 1 },
	{ "hsibutton6", GDK_KEY_KP_6, 85, 1 },
	{ "hsibutton7", GDK_KEY_KP_7, 79, 1 },
	{ "hsibutton8", GDK_KEY_KP_8, 80, 1 },
	{ "hsibutton9", GDK_KEY_KP_9, 81, 1 },
	{ "hsibuttondot", GDK_KEY_KP_Decimal, 91, 1 },
	{ "hsibutton00", GDK_KEY_KP_0, 90, 2 },
	{ "hsibutton000", GDK_KEY_KP_0, 90, 3 }
};

const unsigned int FlightDeckWindow::HSIDialog::nrcheckbuttons;
const struct FlightDeckWindow::HSIDialog::checkbutton FlightDeckWindow::HSIDialog::checkbuttons[nrcheckbuttons] = {
	{ "hsicheckbuttonfplan", MapRenderer::drawflags_route },
	{ "hsicheckbuttonfplanlabels", MapRenderer::drawflags_route_labels },
	{ "hsicheckbuttonairports", MapRenderer::drawflags_airports },
	{ "hsicheckbuttonnavaids", MapRenderer::drawflags_navaids },
	{ "hsicheckbuttonwaypoints", MapRenderer::drawflags_waypoints },
};

FlightDeckWindow::HSIDialog::HSIDialog(BaseObjectType *castitem, const Glib::RefPtr<Builder>& refxml)
	: Gtk::Window(castitem), m_togglebuttonmanualheading(0), m_spinbuttonmanualheading(0), m_checkbuttonheadingtrue(0),
	  m_spinbuttonrat(0), m_togglebuttonias(0), m_spinbuttonias(0), m_togglebuttoncruiserpm(0), m_spinbuttoncruiserpm(0),
	  m_spinbuttoncruisemp(0), m_hsilabelcruisemp(0), m_togglebuttoncruisebhp(0), m_spinbuttoncruisebhp(0), m_hsilabelcruisebhp(0),
	  m_checkbuttonwind(0), m_checkbuttonterrain(0), m_checkbuttonglide(0),
	  m_buttonunset(0), m_buttondone(0), m_curheading(0), m_heading(0), m_currat(15), m_rat(15), m_curias(100), m_ias(100),
	  m_curcruise(std::numeric_limits<double>::quiet_NaN(), 2200, 25), m_cruise(std::numeric_limits<double>::quiet_NaN(), 2200, 25),
	  m_constantspeed(true), m_iasmode(Sensors::iasmode_manual),
	  m_curflags(MapRenderer::drawflags_route | MapRenderer::drawflags_route_labels), m_flags(MapRenderer::drawflags_route | MapRenderer::drawflags_route_labels),
	  m_curheadingtrue(false), m_headingtrue(false), m_displayterrain(true), m_displayglide(false)
{
#ifndef GLIBMM_DEFAULT_SIGNAL_HANDLERS_ENABLED
        signal_delete_event().connect(sigc::mem_fun(*this, &HSIDialog::on_delete_event));
#endif //GLIBMM_DEFAULT_SIGNAL_HANDLERS_ENABLED
	get_widget(refxml, "hsitogglebuttonmanualheading", m_togglebuttonmanualheading);
	get_widget(refxml, "hsispinbuttonmanualheading", m_spinbuttonmanualheading);
	get_widget(refxml, "hsicheckbuttonhdgtrue", m_checkbuttonheadingtrue);
	get_widget(refxml, "hsispinbuttonrat", m_spinbuttonrat);
	get_widget(refxml, "hsitogglebuttonias", m_togglebuttonias);
	get_widget(refxml, "hsispinbuttonias", m_spinbuttonias);
	get_widget(refxml, "hsitogglebuttoncruiserpm", m_togglebuttoncruiserpm);
	get_widget(refxml, "hsispinbuttoncruiserpm", m_spinbuttoncruiserpm);
	get_widget(refxml, "hsispinbuttoncruisemp", m_spinbuttoncruisemp);
	get_widget(refxml, "hsilabelcruisemp", m_hsilabelcruisemp);
	get_widget(refxml, "hsitogglebuttoncruisebhp", m_togglebuttoncruisebhp);
	get_widget(refxml, "hsispinbuttoncruisebhp", m_spinbuttoncruisebhp);
	get_widget(refxml, "hsilabelcruisebhp", m_hsilabelcruisebhp);
	get_widget(refxml, "hsicheckbuttonwind", m_checkbuttonwind);
	get_widget(refxml, "hsicheckbuttonterrain", m_checkbuttonterrain);
	get_widget(refxml, "hsicheckbuttonglide", m_checkbuttonglide);
	get_widget(refxml, "hsibuttonunset", m_buttonunset);
	get_widget(refxml, "hsibuttondone", m_buttondone);
	if (m_buttonunset)
		m_connunset = m_buttonunset->signal_clicked().connect(sigc::mem_fun(*this, &HSIDialog::unset_clicked));
	if (m_buttondone)
		m_conndone = m_buttondone->signal_clicked().connect(sigc::mem_fun(*this, &HSIDialog::done_clicked));
	if (m_togglebuttonmanualheading)
		m_connenamanualheading = m_togglebuttonmanualheading->signal_clicked().connect(sigc::mem_fun(*this, &HSIDialog::heading_clicked));
	if (m_spinbuttonmanualheading)
		m_connmanualheading = m_spinbuttonmanualheading->signal_value_changed().connect(sigc::mem_fun(*this, &HSIDialog::heading_changed));
	if (m_checkbuttonheadingtrue)
		m_connheadingtrue = m_checkbuttonheadingtrue->signal_toggled().connect(sigc::mem_fun(*this, &HSIDialog::headingtrue_toggled));
	if (m_spinbuttonrat)
		m_connrat = m_spinbuttonrat->signal_value_changed().connect(sigc::mem_fun(*this, &HSIDialog::rat_changed));
	if (m_togglebuttonias)
		m_conntoggleias = m_togglebuttonias->signal_toggled().connect(sigc::bind(sigc::mem_fun(*this, &HSIDialog::iasmode_clicked), Sensors::iasmode_manual));
	if (m_spinbuttonias)
		m_connias = m_spinbuttonias->signal_value_changed().connect(sigc::mem_fun(*this, &HSIDialog::ias_changed));
	if (m_togglebuttoncruiserpm)
		m_conntogglecruiserpm = m_togglebuttoncruiserpm->signal_toggled().connect(sigc::bind(sigc::mem_fun(*this, &HSIDialog::iasmode_clicked), Sensors::iasmode_rpm_mp));
	if (m_spinbuttoncruiserpm)
		 m_conncruiserpm = m_spinbuttoncruiserpm->signal_value_changed().connect(sigc::mem_fun(*this, &HSIDialog::cruiserpm_changed));
	if (m_spinbuttoncruisemp)
		 m_conncruisemp = m_spinbuttoncruisemp->signal_value_changed().connect(sigc::mem_fun(*this, &HSIDialog::cruisemp_changed));
	if (m_togglebuttoncruisebhp)
		m_conntogglecruisebhp = m_togglebuttoncruisebhp->signal_toggled().connect(sigc::bind(sigc::mem_fun(*this, &HSIDialog::iasmode_clicked), Sensors::iasmode_bhp_rpm));
	if (m_spinbuttoncruisebhp)
		 m_conncruisebhp = m_spinbuttoncruisebhp->signal_value_changed().connect(sigc::mem_fun(*this, &HSIDialog::cruisebhp_changed));
	if (m_checkbuttonwind)
		m_conndisplaywind = m_checkbuttonwind->signal_toggled().connect(sigc::mem_fun(*this, &HSIDialog::wind_toggled));
	if (m_checkbuttonterrain)
		m_conndisplayterrain = m_checkbuttonterrain->signal_toggled().connect(sigc::mem_fun(*this, &HSIDialog::terrain_toggled));
	if (m_checkbuttonglide)
		m_conndisplayglide = m_checkbuttonglide->signal_toggled().connect(sigc::mem_fun(*this, &HSIDialog::glide_toggled));
	for (unsigned int i = 0; i < nrkeybuttons; ++i) {
		m_keybutton[i] = 0;
		get_widget(refxml, keybuttons[i].widget_name, m_keybutton[i]);
		if (!m_keybutton[i])
			continue;
		m_connkey[i] = m_keybutton[i]->signal_clicked().connect(sigc::bind(sigc::mem_fun(*this, &HSIDialog::key_clicked), i));
		m_keybutton[i]->set_can_focus(false);
	}
	for (unsigned int i = 0; i < nrcheckbuttons; ++i) {
		m_checkbutton[i] = 0;
		get_widget(refxml, checkbuttons[i].widgetname, m_checkbutton[i]);
		if (!m_checkbutton[i])
			continue;
		m_conncheckbutton[i] = m_checkbutton[i]->signal_toggled().connect(sigc::bind(sigc::mem_fun(*this, &HSIDialog::flag_toggled), i));
	}
	set_flags(MapRenderer::drawflags_route | MapRenderer::drawflags_route_labels);
	set_displaywind(true);
	set_displayterrain(true);
	set_displayglide(false);
	set_heading(0);
	set_rat(15);
	set_ias(100);
	set_cruiserpm(2400);
	set_cruisemp(20);
	set_cruisebhp(130);
	set_constantspeed(false);
	set_iasmode(Sensors::iasmode_manual);
	set_maxbhp(200);
#if defined(HAVE_GTKMM2)
	if (true && m_spinbuttonmanualheading) {
		Glib::RefPtr<Gtk::Style> style(m_spinbuttonmanualheading->get_style());
		if (style) {
			style = style->copy();
			{
				Pango::FontDescription font(style->get_font());
				font.set_size(font.get_size() * 2);
				style->set_font(font);
			}
			m_spinbuttonmanualheading->set_style(style);
			if (m_spinbuttonrat)
				m_spinbuttonrat->set_style(style);
			if (m_spinbuttonias)
				m_spinbuttonias->set_style(style);
			if (m_spinbuttoncruiserpm)
				m_spinbuttoncruiserpm->set_style(style);
			if (m_spinbuttoncruisemp)
				m_spinbuttoncruisemp->set_style(style);
			if (m_spinbuttoncruisebhp)
				m_spinbuttoncruisebhp->set_style(style);
		}
	}
#endif
}

FlightDeckWindow::HSIDialog::~HSIDialog()
{
}

void FlightDeckWindow::HSIDialog::set_flags(MapRenderer::DrawFlags flags)
{
	m_curflags = m_flags = flags;
	for (unsigned int i = 0; i < nrcheckbuttons; ++i)
		m_conncheckbutton[i].block();
	for (unsigned int i = 0; i < nrcheckbuttons; ++i) {
		if (!m_checkbutton[i])
			continue;
		if (!(checkbuttons[i].drawflag & flags))
			m_checkbutton[i]->set_active(false);
		else if (!(checkbuttons[i].drawflag & ~flags))
			m_checkbutton[i]->set_active(true);
		else
			m_checkbutton[i]->set_inconsistent(true);
	}
	for (unsigned int i = 0; i < nrcheckbuttons; ++i)
		m_conncheckbutton[i].unblock();
}

void FlightDeckWindow::HSIDialog::set_displaywind(bool v)
{
	m_displaywind = v;
	m_conndisplaywind.block();
	if (m_checkbuttonwind)
		m_checkbuttonwind->set_active(m_displaywind);
	m_conndisplaywind.unblock();
}

void FlightDeckWindow::HSIDialog::set_displayterrain(bool v)
{
	m_displayterrain = v;
	m_conndisplayterrain.block();
	if (m_checkbuttonterrain)
		m_checkbuttonterrain->set_active(m_displayterrain);
	m_conndisplayterrain.unblock();
}

void FlightDeckWindow::HSIDialog::set_displayglide(bool v)
{
	m_displayglide = v;
	m_conndisplayglide.block();
	if (m_checkbuttonglide)
		m_checkbuttonglide->set_active(m_displayglide);
	m_conndisplayglide.unblock();
}

void FlightDeckWindow::HSIDialog::set_heading(double v)
{
	m_curheading = m_heading = v;
	m_connmanualheading.block();
	m_connenamanualheading.block();
	if (m_togglebuttonmanualheading)
		m_togglebuttonmanualheading->set_active(!std::isnan(m_heading));
	if (m_spinbuttonmanualheading) {
		m_spinbuttonmanualheading->set_sensitive(!std::isnan(m_heading));
		m_spinbuttonmanualheading->set_value(std::isnan(m_heading) ? 0 : m_heading);
	}
	m_connmanualheading.unblock();
	m_connenamanualheading.unblock();
}

void FlightDeckWindow::HSIDialog::set_heading_true(bool v)
{
	m_curheadingtrue = m_headingtrue = v;
	m_connheadingtrue.block();
	if (m_checkbuttonheadingtrue)
		m_checkbuttonheadingtrue->set_active(m_headingtrue);
	m_connheadingtrue.unblock();
}

void FlightDeckWindow::HSIDialog::set_rat(double v)
{
	m_currat = m_rat = v;
	if (std::isnan(m_rat))
		m_rat = IcaoAtmosphere<double>::std_sealevel_temperature - IcaoAtmosphere<double>::degc_to_kelvin;
	m_connrat.block();
	if (m_spinbuttonrat)
		m_spinbuttonrat->set_value(m_rat);
	m_connrat.unblock();
}

void FlightDeckWindow::HSIDialog::set_ias(double v)
{
	m_curias = m_ias = v;
	if (std::isnan(m_ias))
		m_ias = 100;
	m_connias.block();
	if (m_spinbuttonias)
		m_spinbuttonias->set_value(m_ias);
	m_connias.unblock();
}

void FlightDeckWindow::HSIDialog::set_cruiserpm(double v)
{
	m_curcruise.set_rpm(v);
	m_cruise.set_rpm(v);
	if (std::isnan(m_cruise.get_rpm()))
		m_cruise.set_rpm(2000);
	m_conncruiserpm.block();
	if (m_spinbuttoncruiserpm)
		m_spinbuttoncruiserpm->set_value(m_cruise.get_rpm());
	m_conncruiserpm.unblock();
}

void FlightDeckWindow::HSIDialog::set_cruisemp(double v)
{
	m_curcruise.set_mp(v);
	m_cruise.set_mp(v);
	if (std::isnan(m_cruise.get_mp()))
		m_cruise.set_mp(20);
	m_conncruisemp.block();
	if (m_spinbuttoncruisemp)
		m_spinbuttoncruisemp->set_value(m_cruise.get_mp());
	m_conncruisemp.unblock();
}

void FlightDeckWindow::HSIDialog::set_cruisebhp(double v)
{
	m_curcruise.set_bhp(v);
	m_cruise.set_bhp(v);
	if (std::isnan(m_cruise.get_bhp()))
		m_cruise.set_bhp(130);
	m_conncruisebhp.block();
	if (m_spinbuttoncruisebhp)
		m_spinbuttoncruisebhp->set_value(m_cruise.get_bhp());
	m_conncruisebhp.unblock();
	update_bhp_percent();
}

void FlightDeckWindow::HSIDialog::set_maxbhp(double b)
{
	if (std::isnan(b))
		b = 200;
	b = std::max(b, 0.0);
	m_maxbhp = b;
	if (m_spinbuttoncruisebhp)
		m_spinbuttoncruisebhp->set_range(0, m_maxbhp);
	update_bhp_percent();
}

void FlightDeckWindow::HSIDialog::update_bhp_percent(void)
{
	if (!m_hsilabelcruisebhp)
		return;
	std::ostringstream oss;
	if (!std::isnan(get_maxbhp()) && !std::isnan(m_cruise.get_bhp()) && get_maxbhp() > 0)
		oss << std::fixed << std::setprecision(1) << (100.0 * m_cruise.get_bhp() / get_maxbhp()) << '%';
	m_hsilabelcruisebhp->set_text(oss.str());
}

void FlightDeckWindow::HSIDialog::set_constantspeed(bool cs)
{
	m_constantspeed = cs;
	if (m_spinbuttoncruisemp)
		m_spinbuttoncruisemp->set_visible(m_constantspeed);
	if (m_hsilabelcruisemp)
		m_hsilabelcruisemp->set_visible(m_constantspeed);
	update_iasmode();
}

void FlightDeckWindow::HSIDialog::set_iasmode(iasmode_t iasmode)
{
	m_iasmode = iasmode;
	update_iasmode();
}

void FlightDeckWindow::HSIDialog::update_iasmode(void)
{
	m_conntoggleias.block();
	m_conntogglecruiserpm.block();
	m_conntogglecruisebhp.block();
	if (m_togglebuttonias)
		m_togglebuttonias->set_active(m_iasmode == Sensors::iasmode_manual);
	if (m_togglebuttoncruiserpm)
		m_togglebuttoncruiserpm->set_active(m_iasmode == Sensors::iasmode_rpm_mp);
	if (m_togglebuttoncruisebhp)
		m_togglebuttoncruisebhp->set_active(m_iasmode == Sensors::iasmode_bhp_rpm);
	m_conntoggleias.unblock();
	m_conntogglecruiserpm.unblock();
	m_conntogglecruisebhp.unblock();
	if (m_spinbuttonias)
		m_spinbuttonias->set_sensitive(m_iasmode == Sensors::iasmode_manual);
	if (m_spinbuttoncruiserpm)
		m_spinbuttoncruiserpm->set_sensitive((m_iasmode == Sensors::iasmode_rpm_mp) ||
						     (m_iasmode == Sensors::iasmode_bhp_rpm && m_constantspeed));
	if (m_spinbuttoncruisemp)
		m_spinbuttoncruisemp->set_sensitive(m_iasmode == Sensors::iasmode_rpm_mp && m_constantspeed);
	if (m_spinbuttoncruisebhp)
		m_spinbuttoncruisebhp->set_sensitive(m_iasmode == Sensors::iasmode_bhp_rpm);
}

void FlightDeckWindow::HSIDialog::heading_clicked(void)
{
	if (!m_togglebuttonmanualheading)
		return;
	bool ena(m_togglebuttonmanualheading->get_active());
	m_connmanualheading.block();
	if (m_spinbuttonmanualheading)
		m_spinbuttonmanualheading->set_sensitive(ena);
	m_connmanualheading.unblock();
	if (ena) {
		m_heading = 0;
		if (m_spinbuttonmanualheading)
			m_heading = m_spinbuttonmanualheading->get_value();
	} else {
		m_heading = std::numeric_limits<double>::quiet_NaN();
	}
}

void FlightDeckWindow::HSIDialog::heading_changed(void)
{
	if (m_spinbuttonmanualheading)
		m_heading = m_spinbuttonmanualheading->get_value();
}

void FlightDeckWindow::HSIDialog::headingtrue_toggled(void)
{
	if (m_checkbuttonheadingtrue)
		m_headingtrue = m_checkbuttonheadingtrue->get_active();
}

void FlightDeckWindow::HSIDialog::rat_changed(void)
{
	if (m_spinbuttonrat)
		m_rat = m_spinbuttonrat->get_value();
}

void FlightDeckWindow::HSIDialog::ias_changed(void)
{
	if (m_spinbuttonias)
		m_ias = m_spinbuttonias->get_value();
}

void FlightDeckWindow::HSIDialog::cruiserpm_changed(void)
{
	if (m_spinbuttoncruiserpm)
		m_cruise.set_rpm(m_spinbuttoncruiserpm->get_value());
}

void FlightDeckWindow::HSIDialog::cruisemp_changed(void)
{
	if (m_spinbuttoncruisemp)
		m_cruise.set_mp(m_spinbuttoncruisemp->get_value());
}

void FlightDeckWindow::HSIDialog::cruisebhp_changed(void)
{
	if (m_spinbuttoncruisebhp)
		m_cruise.set_bhp(m_spinbuttoncruisebhp->get_value());
	update_bhp_percent();
}

void FlightDeckWindow::HSIDialog::iasmode_clicked(iasmode_t iasmode)
{
	m_iasmode = iasmode;
	update_iasmode();
}

void FlightDeckWindow::HSIDialog::wind_toggled(void)
{
	if (m_checkbuttonwind)
		m_displaywind = m_checkbuttonwind->get_active();
}

void FlightDeckWindow::HSIDialog::flag_toggled(unsigned int nr)
{
	if (nr >= nrcheckbuttons || !m_checkbutton[nr])
		return;
	if (m_checkbutton[nr]->get_active())
		m_flags |= checkbuttons[nr].drawflag;
	else
		m_flags &= ~checkbuttons[nr].drawflag;
}

void FlightDeckWindow::HSIDialog::terrain_toggled(void)
{
	if (m_checkbuttonterrain)
		m_displayterrain = m_checkbuttonterrain->get_active();
}

void FlightDeckWindow::HSIDialog::glide_toggled(void)
{
	if (m_checkbuttonglide)
		m_displayglide = m_checkbuttonglide->get_active();
}

bool FlightDeckWindow::HSIDialog::on_delete_event(GdkEventAny* event)
{
	unset_focus();
        hide();
	m_signal_done.emit();
        return true;
}

void FlightDeckWindow::HSIDialog::done_clicked(void)
{
	unset_focus();
        hide();
	m_signal_done.emit();
}

void FlightDeckWindow::HSIDialog::key_clicked(unsigned int i)
{
 	if (i >= nrkeybuttons)
		return;
	Gtk::Widget *w(get_focus());
	if (!w)
		return;
	for (unsigned int cnt(keybuttons[i].count); cnt > 0; --cnt)
		KeyboardDialog::send_key(w, 0, keybuttons[i].key, keybuttons[i].hwkeycode);
}

void FlightDeckWindow::HSIDialog::unset_clicked(void)
{
	set_heading(m_curheading);
	set_rat(m_currat);
	set_ias(m_curias);
}
