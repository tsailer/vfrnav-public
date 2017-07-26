//
// C++ Implementation: FlightDeckWindow
//
// Description: Flight Deck Windows, Text Entry Dialog
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

void FlightDeckWindow::textentry_dialog_open(const Glib::ustring& label, const Glib::ustring& text, const sigc::slot<void,bool>& done)
{
	m_textentrydialogdoneconn.disconnect();
	if (!m_textentrydialog)
		return;
	m_textentrydialog->set_label(label);
	m_textentrydialog->set_text(text);
	m_textentrydialogdoneconn = m_textentrydialog->signal_done().connect(done);
	m_textentrydialog->show();
}

const unsigned int FlightDeckWindow::TextEntryDialog::nrkeybuttons;
const struct FlightDeckWindow::TextEntryDialog::keybutton FlightDeckWindow::TextEntryDialog::keybuttons[nrkeybuttons] = {
	{ "textentrybutton0", GDK_KEY_0, GDK_KEY_parenright, 19 },
	{ "textentrybutton1", GDK_KEY_1, GDK_KEY_exclam, 10 },
	{ "textentrybutton2", GDK_KEY_2, GDK_KEY_at, 11 },
	{ "textentrybutton3", GDK_KEY_3, GDK_KEY_numbersign, 12 },
	{ "textentrybutton4", GDK_KEY_4, GDK_KEY_dollar, 13 },
	{ "textentrybutton5", GDK_KEY_5, GDK_KEY_percent, 14 },
	{ "textentrybutton6", GDK_KEY_6, GDK_KEY_asciicircum, 15 },
	{ "textentrybutton7", GDK_KEY_7, GDK_KEY_ampersand, 16 },
	{ "textentrybutton8", GDK_KEY_8, GDK_KEY_asterisk, 17 },
	{ "textentrybutton9", GDK_KEY_9, GDK_KEY_parenleft, 18 },
	{ "textentrybuttona", GDK_KEY_a, GDK_KEY_A, 38 },
	{ "textentrybuttonb", GDK_KEY_b, GDK_KEY_B, 56 },
	{ "textentrybuttonc", GDK_KEY_c, GDK_KEY_C, 54 },
	{ "textentrybuttond", GDK_KEY_d, GDK_KEY_D, 40 },
	{ "textentrybuttone", GDK_KEY_e, GDK_KEY_E, 26 },
	{ "textentrybuttonf", GDK_KEY_f, GDK_KEY_F, 41 },
	{ "textentrybuttong", GDK_KEY_g, GDK_KEY_G, 42 },
	{ "textentrybuttonh", GDK_KEY_h, GDK_KEY_H, 43 },
	{ "textentrybuttoni", GDK_KEY_i, GDK_KEY_I, 31 },
	{ "textentrybuttonj", GDK_KEY_j, GDK_KEY_J, 44 },
	{ "textentrybuttonk", GDK_KEY_k, GDK_KEY_K, 45 },
	{ "textentrybuttonl", GDK_KEY_l, GDK_KEY_L, 46 },
	{ "textentrybuttonm", GDK_KEY_m, GDK_KEY_M, 58 },
	{ "textentrybuttonn", GDK_KEY_n, GDK_KEY_N, 57 },
	{ "textentrybuttono", GDK_KEY_o, GDK_KEY_O, 32 },
	{ "textentrybuttonp", GDK_KEY_p, GDK_KEY_P, 33 },
	{ "textentrybuttonq", GDK_KEY_q, GDK_KEY_Q, 24 },
	{ "textentrybuttonr", GDK_KEY_r, GDK_KEY_R, 27 },
	{ "textentrybuttons", GDK_KEY_s, GDK_KEY_S, 39 },
	{ "textentrybuttont", GDK_KEY_t, GDK_KEY_T, 28 },
	{ "textentrybuttonu", GDK_KEY_u, GDK_KEY_U, 30 },
	{ "textentrybuttonv", GDK_KEY_v, GDK_KEY_V, 55 },
	{ "textentrybuttonw", GDK_KEY_w, GDK_KEY_W, 25 },
	{ "textentrybuttonx", GDK_KEY_x, GDK_KEY_X, 53 },
	{ "textentrybuttony", GDK_KEY_y, GDK_KEY_Y, 29 },
	{ "textentrybuttonz", GDK_KEY_z, GDK_KEY_Z, 52 },
	{ "textentrybuttonapostrophe", GDK_KEY_quoteright, GDK_KEY_quotedbl, 48 },
	{ "textentrybuttonbracketclose", GDK_KEY_bracketright, GDK_KEY_braceright, 35 },
	{ "textentrybuttonbracketopen", GDK_KEY_bracketleft, GDK_KEY_braceleft, 34 },
	{ "textentrybuttoncomma", GDK_KEY_comma, GDK_KEY_less, 59 },
	{ "textentrybuttondash", GDK_KEY_minus, GDK_KEY_underscore, 20 },
	{ "textentrybuttondot", GDK_KEY_period, GDK_KEY_greater, 60 },
	{ "textentrybuttonequal", GDK_KEY_equal, GDK_KEY_plus, 21 },
	{ "textentrybuttonquestionmark", GDK_KEY_question, GDK_KEY_slash, 61 },
	{ "textentrybuttonsemicolon", GDK_KEY_semicolon, GDK_KEY_colon, 47 },
	{ "textentrybuttonspace", GDK_KEY_space, GDK_KEY_space, 65 },
	{ "textentrybuttonbackslash", GDK_KEY_backslash, GDK_KEY_bar, 51 },
	{ "textentrybuttonbackspace", GDK_KEY_BackSpace, GDK_KEY_BackSpace, 22 }
};

FlightDeckWindow::TextEntryDialog::TextEntryDialog(BaseObjectType *castitem, const Glib::RefPtr<Builder>& refxml)
	: Gtk::Window(castitem), m_textentrylabel(0), m_textentrytext(0),
	  m_textentrybuttoncancel(0), m_textentrybuttonclear(0), m_textentrybuttonok(0),
	  m_textentrytogglebuttonshiftleft(0), m_textentrytogglebuttonshiftright(0), m_textentrytogglebuttoncapslock(0),
	  m_shiftstate(shiftstate_none)
{
#ifndef GLIBMM_DEFAULT_SIGNAL_HANDLERS_ENABLED
        signal_delete_event().connect(sigc::mem_fun(*this, &TextEntryDialog::on_delete_event));
#endif //GLIBMM_DEFAULT_SIGNAL_HANDLERS_ENABLED
	get_widget(refxml, "textentrylabel", m_textentrylabel);
	get_widget(refxml, "textentrytext", m_textentrytext);
	get_widget(refxml, "textentrybuttoncancel", m_textentrybuttoncancel);
	get_widget(refxml, "textentrybuttonclear", m_textentrybuttonclear);
	get_widget(refxml, "textentrybuttonok", m_textentrybuttonok);
	get_widget(refxml, "textentrytogglebuttonshiftleft", m_textentrytogglebuttonshiftleft);
	get_widget(refxml, "textentrytogglebuttonshiftright", m_textentrytogglebuttonshiftright);
	get_widget(refxml, "textentrytogglebuttoncapslock", m_textentrytogglebuttoncapslock);
	if (m_textentrybuttoncancel)
		m_textentrybuttoncancel->signal_clicked().connect(sigc::mem_fun(*this, &TextEntryDialog::cancel_clicked));
	if (m_textentrybuttonclear)
		m_textentrybuttonclear->signal_clicked().connect(sigc::mem_fun(*this, &TextEntryDialog::clear_clicked));
	if (m_textentrybuttonok)
		m_textentrybuttonok->signal_clicked().connect(sigc::mem_fun(*this, &TextEntryDialog::ok_clicked));
	if (m_textentrytogglebuttonshiftleft)
		m_connshiftleft = m_textentrytogglebuttonshiftleft->signal_clicked().connect(sigc::mem_fun(*this, &TextEntryDialog::shift_clicked));
	if (m_textentrytogglebuttonshiftright)
		m_connshiftright = m_textentrytogglebuttonshiftright->signal_clicked().connect(sigc::mem_fun(*this, &TextEntryDialog::shift_clicked));
	if (m_textentrytogglebuttoncapslock)
		m_conncapslock = m_textentrytogglebuttoncapslock->signal_clicked().connect(sigc::mem_fun(*this, &TextEntryDialog::capslock_clicked));
	for (unsigned int i = 0; i < nrkeybuttons; ++i) {
		m_keybutton[i] = 0;
		get_widget(refxml, keybuttons[i].widget_name, m_keybutton[i]);
		if (!m_keybutton[i])
			continue;
		m_keybutton[i]->signal_clicked().connect(sigc::bind(sigc::mem_fun(*this, &TextEntryDialog::key_clicked), i));
		m_keybutton[i]->set_can_focus(false);
	}
}

FlightDeckWindow::TextEntryDialog::~TextEntryDialog()
{
}

void FlightDeckWindow::TextEntryDialog::set_label(const Glib::ustring& text)
{
	if (m_textentrylabel)
		m_textentrylabel->set_markup(text);
}

void FlightDeckWindow::TextEntryDialog::set_text(const Glib::ustring& text)
{
	if (m_textentrytext) {
		m_textentrytext->set_text(text);
		m_textentrytext->select_region(0, -1);
		m_textentrytext->grab_focus();
	}
}

Glib::ustring FlightDeckWindow::TextEntryDialog::get_text(void) const
{
	if (!m_textentrytext)
		return "";
	return m_textentrytext->get_text();
}

bool FlightDeckWindow::TextEntryDialog::on_delete_event(GdkEventAny* event)
{
        hide();
	m_signal_done.emit(false);
        return true;
}

void FlightDeckWindow::TextEntryDialog::ok_clicked(void)
{
        hide();
	m_signal_done.emit(true);
}

void FlightDeckWindow::TextEntryDialog::cancel_clicked(void)
{
        hide();
	m_signal_done.emit(false);
}

void FlightDeckWindow::TextEntryDialog::clear_clicked(void)
{
	if (m_textentrytext)
		m_textentrytext->set_text("");
}

void FlightDeckWindow::TextEntryDialog::shift_clicked(void)
{
	m_shiftstate = (m_shiftstate == shiftstate_once) ? shiftstate_none : shiftstate_once;
	set_shiftstate();
}

void FlightDeckWindow::TextEntryDialog::capslock_clicked(void)
{
	m_shiftstate = (m_shiftstate == shiftstate_lock) ? shiftstate_none : shiftstate_lock;
	set_shiftstate();
}

void FlightDeckWindow::TextEntryDialog::set_shiftstate(void)
{
	m_connshiftleft.block();
	m_connshiftright.block();
	m_conncapslock.block();
	if (m_textentrytogglebuttonshiftleft)
		m_textentrytogglebuttonshiftleft->set_active(m_shiftstate == shiftstate_once);
	if (m_textentrytogglebuttonshiftright)
		m_textentrytogglebuttonshiftright->set_active(m_shiftstate == shiftstate_once);
	if (m_textentrytogglebuttoncapslock)
		m_textentrytogglebuttoncapslock->set_active(m_shiftstate == shiftstate_lock);
	m_connshiftleft.unblock();
	m_connshiftright.unblock();
	m_conncapslock.unblock();
}

void FlightDeckWindow::TextEntryDialog::key_clicked(unsigned int i)
{
 	if (!m_textentrytext || i >= nrkeybuttons)
		return;
	guint keyval((m_shiftstate != shiftstate_none) ? keybuttons[i].key_shift : keybuttons[i].key);
	if (false)
		std::cerr << "key_clicked: " << keyval << std::endl;
	KeyboardDialog::send_key(m_textentrytext, ((m_shiftstate == shiftstate_once) ? GDK_SHIFT_MASK : 0) |
				 ((m_shiftstate == shiftstate_lock) ? GDK_LOCK_MASK : 0), keyval, keybuttons[i].hwkeycode);
	if (m_shiftstate == shiftstate_once) {
		m_shiftstate = shiftstate_none;
		set_shiftstate();
	}
}
