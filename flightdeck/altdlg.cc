//
// C++ Implementation: FlightDeckWindow
//
// Description: Flight Deck Windows, Altitude Dialog
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

void FlightDeckWindow::alt_dialog_open(void)
{
	if (!m_altdialog)
		return;
	if (m_sensors) {
		m_altdialog->set_std(m_sensors->is_altimeter_std());
		m_altdialog->set_qnh(m_sensors->get_altimeter_qnh());
	}
	m_altdialog->set_altbug(m_navarea.alt_get_altbug());
	m_altdialog->set_minimum(m_navarea.alt_get_minimum());
	m_altdialog->set_metric(m_navarea.alt_is_metric());
	m_altdialog->set_altlabels(m_navarea.alt_is_labels());
	m_altdialog->show();
}

bool FlightDeckWindow::alt_indicator_button_press_event(GdkEventButton *event)
{
	unsigned int button = event->button;
	if (button != 1)
		return false;
	alt_dialog_open();
	return false;
}

void FlightDeckWindow::alt_settings_update(void)
{
	if (!m_altdialog)
		return;
	if (m_sensors) {
		if (m_altdialog->is_qnh_changed())
			m_sensors->set_altimeter_qnh(m_altdialog->get_qnh());
		m_sensors->set_altimeter_std(m_altdialog->is_std());
	}
	if (m_altdialog->is_altbug_changed())
		m_navarea.alt_set_altbug(m_altdialog->get_altbug());
	if (m_altdialog->is_minimum_changed())
		m_navarea.alt_set_minimum(m_altdialog->get_minimum());
	if (m_altdialog->is_qnh_changed())
		m_navarea.alt_set_qnh(m_altdialog->get_qnh());
	m_navarea.alt_set_std(m_altdialog->is_std());
	m_navarea.alt_set_inhg(m_altdialog->is_inhg());
	m_navarea.alt_set_metric(m_altdialog->is_metric());
	m_navarea.alt_set_labels(m_altdialog->is_altlabels());
}

const unsigned int FlightDeckWindow::AltDialog::nrkeybuttons;
const struct FlightDeckWindow::AltDialog::keybutton FlightDeckWindow::AltDialog::keybuttons[nrkeybuttons] = {
	{ "altbutton0", GDK_KEY_KP_0, 90, 1 },
	{ "altbutton1", GDK_KEY_KP_1, 87, 1 },
	{ "altbutton2", GDK_KEY_KP_2, 88, 1 },
	{ "altbutton3", GDK_KEY_KP_3, 89, 1 },
	{ "altbutton4", GDK_KEY_KP_4, 83, 1 },
	{ "altbutton5", GDK_KEY_KP_5, 84, 1 },
	{ "altbutton6", GDK_KEY_KP_6, 85, 1 },
	{ "altbutton7", GDK_KEY_KP_7, 79, 1 },
	{ "altbutton8", GDK_KEY_KP_8, 80, 1 },
	{ "altbutton9", GDK_KEY_KP_9, 81, 1 },
	{ "altbuttondot", GDK_KEY_KP_Decimal, 91, 1 },
	{ "altbutton00", GDK_KEY_KP_0, 90, 2 },
	{ "altbutton000", GDK_KEY_KP_0, 90, 3 }
};

FlightDeckWindow::AltDialog::AltDialog(BaseObjectType *castitem, const Glib::RefPtr<Builder>& refxml)
	: Gtk::Window(castitem), m_togglebuttonaltbug(0), m_spinbuttonaltbug(0), m_togglebuttonminimum(0), m_spinbuttonminimum(0),
	  m_spinbuttonqnh(0), m_togglebuttonqnh(0), m_togglebuttonstd(0), m_togglebuttonhpa(0), m_togglebuttoninhg(0),
	  m_togglebuttonaltlabels(0), m_togglebuttonmetric(0), m_buttonunset(0), m_buttondone(0),
	  m_curqnh(std::numeric_limits<double>::quiet_NaN()), m_qnh(std::numeric_limits<double>::quiet_NaN()),
	  m_curaltbug(std::numeric_limits<double>::quiet_NaN()), m_altbug(std::numeric_limits<double>::quiet_NaN()),
	  m_curminimum(std::numeric_limits<double>::quiet_NaN()), m_minimum(std::numeric_limits<double>::quiet_NaN()),
	  m_altlabels(true), m_std(true), m_inhg(false), m_metric(false)
{
#ifndef GLIBMM_DEFAULT_SIGNAL_HANDLERS_ENABLED
        signal_delete_event().connect(sigc::mem_fun(*this, &AltDialog::on_delete_event));
#endif //GLIBMM_DEFAULT_SIGNAL_HANDLERS_ENABLED
	get_widget(refxml, "alttogglebuttonaltbug", m_togglebuttonaltbug);
	get_widget(refxml, "altspinbuttonaltbug", m_spinbuttonaltbug);
	get_widget(refxml, "alttogglebuttonminimum", m_togglebuttonminimum);
	get_widget(refxml, "altspinbuttonminimum", m_spinbuttonminimum);
	get_widget(refxml, "altspinbuttonqnh", m_spinbuttonqnh);
	get_widget(refxml, "alttogglebuttonqnh", m_togglebuttonqnh);
	get_widget(refxml, "alttogglebuttonstd", m_togglebuttonstd);
	get_widget(refxml, "alttogglebuttonhpa", m_togglebuttonhpa);
	get_widget(refxml, "alttogglebuttoninhg", m_togglebuttoninhg);
	get_widget(refxml, "alttogglebuttonaltlabels", m_togglebuttonaltlabels);
	get_widget(refxml, "alttogglebuttonmetric", m_togglebuttonmetric);
	get_widget(refxml, "altbuttonunset", m_buttonunset);
	get_widget(refxml, "altbuttondone", m_buttondone);
	for (unsigned int i = 0; i < 2; ++i) {
		std::ostringstream oss;
		oss << "altunitlabel" << i;
		m_altunitlabel[i] = 0;
		get_widget(refxml, oss.str(), m_altunitlabel[i]);
	}
	if (m_buttonunset)
		m_connunset = m_buttonunset->signal_clicked().connect(sigc::mem_fun(*this, &AltDialog::unset_clicked));
	if (m_buttondone)
		m_conndone = m_buttondone->signal_clicked().connect(sigc::mem_fun(*this, &AltDialog::done_clicked));
	if (m_togglebuttonqnh)
		m_connsetqnh = m_togglebuttonqnh->signal_clicked().connect(sigc::bind(sigc::mem_fun(*this, &AltDialog::set_std), false));
	if (m_togglebuttonstd)
		m_connsetstd = m_togglebuttonstd->signal_clicked().connect(sigc::bind(sigc::mem_fun(*this, &AltDialog::set_std), true));
	if (m_togglebuttonhpa)
		m_connsethpa = m_togglebuttonhpa->signal_clicked().connect(sigc::bind(sigc::mem_fun(*this, &AltDialog::set_inhg), false));
	if (m_togglebuttoninhg)
		m_connsetinhg = m_togglebuttoninhg->signal_clicked().connect(sigc::bind(sigc::mem_fun(*this, &AltDialog::set_inhg), true));
	if (m_togglebuttonmetric)
		m_connmetric = m_togglebuttonmetric->signal_clicked().connect(sigc::mem_fun(*this, &AltDialog::metric_clicked));
	if (m_togglebuttonaltlabels)
		m_connaltlabels = m_togglebuttonaltlabels->signal_clicked().connect(sigc::mem_fun(*this, &AltDialog::altlabels_clicked));
	if (m_togglebuttonaltbug)
		m_connbuttonaltbug = m_togglebuttonaltbug->signal_toggled().connect(sigc::mem_fun(*this, &AltDialog::altbug_toggled));
	if (m_spinbuttonaltbug)
		m_connaltbug = m_spinbuttonaltbug->signal_value_changed().connect(sigc::mem_fun(*this, &AltDialog::altbug_changed));
	if (m_togglebuttonminimum)
		m_connbuttonminimum = m_togglebuttonminimum->signal_toggled().connect(sigc::mem_fun(*this, &AltDialog::minimum_toggled));
	if (m_spinbuttonminimum)
		m_connminimum = m_spinbuttonminimum->signal_value_changed().connect(sigc::mem_fun(*this, &AltDialog::minimum_changed));
	if (m_spinbuttonqnh)
		m_connqnh = m_spinbuttonqnh->signal_value_changed().connect(sigc::mem_fun(*this, &AltDialog::qnh_changed));
	for (unsigned int i = 0; i < nrkeybuttons; ++i) {
		m_keybutton[i] = 0;
		get_widget(refxml, keybuttons[i].widget_name, m_keybutton[i]);
		if (!m_keybutton[i])
			continue;
		m_connkey[i] = m_keybutton[i]->signal_clicked().connect(sigc::bind(sigc::mem_fun(*this, &AltDialog::key_clicked), i));
		m_keybutton[i]->set_can_focus(false);
	}
	set_altlabels(true);
	set_std(false);
	set_inhg(false);
	set_altbug(std::numeric_limits<double>::quiet_NaN());
	set_minimum(std::numeric_limits<double>::quiet_NaN());
	set_qnh(IcaoAtmosphere<double>::std_sealevel_pressure);
#if defined(HAVE_GTKMM2)
	if (true && m_spinbuttonaltbug) {
		Glib::RefPtr<Gtk::Style> style(m_spinbuttonaltbug->get_style());
		if (style) {
			style = style->copy();
			{
				Pango::FontDescription font(style->get_font());
				font.set_size(font.get_size() * 2);
				style->set_font(font);
			}
			m_spinbuttonaltbug->set_style(style);
			if (m_togglebuttonminimum)
				m_togglebuttonminimum->set_style(style);
			if (m_spinbuttonminimum)
				m_spinbuttonminimum->set_style(style);
			if (m_spinbuttonqnh)
				m_spinbuttonqnh->set_style(style);
		}
	}
#endif
}

FlightDeckWindow::AltDialog::~AltDialog()
{
}

void FlightDeckWindow::AltDialog::set_altlabels(bool v)
{
	m_altlabels = v;
	m_connaltlabels.block();
	if (m_togglebuttonaltlabels)
		m_togglebuttonaltlabels->set_active(m_altlabels);
	m_connaltlabels.unblock();
}

void FlightDeckWindow::AltDialog::set_std(bool v)
{
	m_std = v;
	m_connsetqnh.block();
	m_connsetstd.block();
	if (m_togglebuttonqnh)
		m_togglebuttonqnh->set_active(!m_std);
	if (m_togglebuttonstd)
		m_togglebuttonstd->set_active(m_std);
	m_connsetstd.unblock();
	m_connsetqnh.unblock();
}

void FlightDeckWindow::AltDialog::set_inhg(bool v)
{
	m_inhg = v;
	m_connsethpa.block();
	m_connsetinhg.block();
	m_connqnh.block();
	if (m_togglebuttonhpa)
		m_togglebuttonhpa->set_active(!m_inhg);
	if (m_togglebuttoninhg)
		m_togglebuttoninhg->set_active(m_inhg);
	if (m_spinbuttonqnh) {
		if (m_inhg) {
			m_spinbuttonqnh->set_digits(2);
			m_spinbuttonqnh->set_increments(0.01, 0.1);
			m_spinbuttonqnh->set_range(21, 36);
		} else {
			m_spinbuttonqnh->set_digits(0);
			m_spinbuttonqnh->set_increments(1, 10);
			m_spinbuttonqnh->set_range(700, 1200);
		}
		m_spinbuttonqnh->set_value(m_qnh * (m_inhg ? NavAltDrawingArea::hpa_to_inhg : 1));
	}
	m_connqnh.unblock();
	m_connsetinhg.unblock();
	m_connsethpa.unblock();
}

void FlightDeckWindow::AltDialog::set_metric(bool v)
{
	m_metric = v;
	m_connmetric.block();
	if (m_togglebuttonmetric)
		m_togglebuttonmetric->set_active(m_metric);
	for (unsigned int i = 0; i < sizeof(m_altunitlabel)/sizeof(m_altunitlabel[0]); ++i) {
		if (!m_altunitlabel[i])
			continue;
		m_altunitlabel[i]->set_text(m_metric ? "m" : "ft");
	}
	m_connmetric.unblock();
	double alt_factor(1);
	if (m_metric)
		alt_factor = Point::ft_to_m_dbl;
	if (m_togglebuttonaltbug && m_spinbuttonaltbug && m_togglebuttonaltbug->get_active()) {
		m_connaltbug.block();	
		m_spinbuttonaltbug->set_value(m_altbug * alt_factor);
		m_connaltbug.unblock();
	}
	if (m_togglebuttonminimum && m_spinbuttonminimum && m_togglebuttonminimum->get_active()) {
		m_connminimum.block();
		m_spinbuttonminimum->set_value(m_minimum * alt_factor);
		m_connminimum.unblock();
	}
}

void FlightDeckWindow::AltDialog::set_altbug(double v)
{
	m_curaltbug = m_altbug = v;
	m_connaltbug.block();
	m_connbuttonaltbug.block();
	bool set(!std::isnan(m_altbug));
	if (m_togglebuttonaltbug)
		m_togglebuttonaltbug->set_active(set);
	if (m_spinbuttonaltbug) {
		m_spinbuttonaltbug->set_sensitive(set);
		if (set) {
			double alt_factor(1);
			if (is_metric())
				alt_factor = Point::ft_to_m_dbl;
			m_spinbuttonaltbug->set_value(m_altbug * alt_factor);
		}
	}
	m_connaltbug.unblock();
	m_connbuttonaltbug.unblock();
}

void FlightDeckWindow::AltDialog::set_minimum(double v)
{
	m_curminimum = m_minimum = v;
	m_connminimum.block();
	m_connbuttonminimum.block();
	bool set(!std::isnan(m_minimum));
	if (m_togglebuttonminimum)
		m_togglebuttonminimum->set_active(set);
	if (m_spinbuttonminimum) {
		m_spinbuttonminimum->set_sensitive(set);
		if (set) {
			double alt_factor(1);
			if (is_metric())
				alt_factor = Point::ft_to_m_dbl;
			m_spinbuttonminimum->set_value(m_minimum * alt_factor);
		}
	}
	m_connminimum.unblock();
	m_connbuttonminimum.unblock();
}

void FlightDeckWindow::AltDialog::set_qnh(double v)
{
	m_curqnh = m_qnh = v;
	m_connqnh.block();
	if (m_spinbuttonqnh)
		m_spinbuttonqnh->set_value(m_qnh * (m_inhg ? NavAltDrawingArea::hpa_to_inhg : 1));
	m_connqnh.unblock();
}

bool FlightDeckWindow::AltDialog::on_delete_event(GdkEventAny* event)
{
	unset_focus();
        hide();
	m_signal_done.emit();
        return true;
}

void FlightDeckWindow::AltDialog::done_clicked(void)
{
	unset_focus();
        hide();
	m_signal_done.emit();
}

void FlightDeckWindow::AltDialog::key_clicked(unsigned int i)
{
 	if (i >= nrkeybuttons)
		return;
	Gtk::Widget *w(get_focus());
	if (!w)
		return;
	for (unsigned int cnt(keybuttons[i].count); cnt > 0; --cnt)
		KeyboardDialog::send_key(w, 0, keybuttons[i].key, keybuttons[i].hwkeycode);
}

void FlightDeckWindow::AltDialog::unset_clicked(void)
{
	Gtk::Widget *w(get_focus());
	if (w == m_spinbuttonqnh) {
		std::cerr << "unset: qnh" << std::endl;
		set_qnh(m_curqnh);
	} else if (w == m_spinbuttonaltbug || w == m_togglebuttonaltbug) {
		std::cerr << "unset: altbug" << std::endl;
		set_altbug(m_curaltbug);
	} else if (w == m_spinbuttonminimum || w == m_togglebuttonminimum) {
		std::cerr << "unset: minimum" << std::endl;
		set_minimum(m_curminimum);
	} else if (w) {
		std::cerr << "unset: widget: " << w->get_name() << std::endl;
	}
	set_qnh(m_curqnh);
	set_altbug(m_curaltbug);
	set_minimum(m_curminimum);
}

void FlightDeckWindow::AltDialog::metric_clicked(void)
{
	if (m_togglebuttonmetric)
		set_metric(m_togglebuttonmetric->get_active());
}

void FlightDeckWindow::AltDialog::altlabels_clicked(void)
{
	if (m_togglebuttonaltlabels)
		m_altlabels = m_togglebuttonaltlabels->get_active();
}

void FlightDeckWindow::AltDialog::altbug_changed(void)
{
	if (m_togglebuttonaltbug && m_spinbuttonaltbug && m_togglebuttonaltbug->get_active()) {
		m_altbug = m_spinbuttonaltbug->get_value();
		if (is_metric())
			m_altbug *= Point::m_to_ft_dbl;
	} else {
		m_altbug = std::numeric_limits<double>::quiet_NaN();
	}
}

void FlightDeckWindow::AltDialog::altbug_toggled(void)
{
	if (m_spinbuttonaltbug)
		m_spinbuttonaltbug->set_sensitive(m_togglebuttonaltbug->get_active());
	altbug_changed();
}

void FlightDeckWindow::AltDialog::minimum_changed(void)
{
	if (m_togglebuttonminimum && m_spinbuttonminimum && m_togglebuttonminimum->get_active()) {
		m_minimum = m_spinbuttonminimum->get_value();
		if (is_metric())
			m_minimum *= Point::m_to_ft_dbl;
	} else {
		m_minimum = std::numeric_limits<double>::quiet_NaN();
	}
}

void FlightDeckWindow::AltDialog::minimum_toggled(void)
{
	if (m_spinbuttonminimum)
		m_spinbuttonminimum->set_sensitive(m_togglebuttonminimum->get_active());
	minimum_changed();
}

void FlightDeckWindow::AltDialog::qnh_changed(void)
{
	if (m_spinbuttonqnh) {
		m_qnh = m_spinbuttonqnh->get_value();
		if (m_inhg)
			m_qnh *= FlightDeckWindow::NavAltDrawingArea::inhg_to_hpa;
	}
}
