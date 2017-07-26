//
// C++ Implementation: FlightDeckWindow
//
// Description: Flight Deck Windows, Text Entry Dialog
//
//
// Author: Thomas Sailer <t.sailer@alumni.ethz.ch>, (C) 2011, 2012
//
// Copyright: See COPYING file that comes with this distribution
//
//

#include "sysdeps.h"
#include "sysdepsgui.h"
#include <iomanip>

#include "flightdeckwindow.h"

bool FlightDeckWindow::onscreenkbd_focus_in_event(GdkEventFocus *event, Gtk::Widget *widget, kbd_t kbd)
{
	if (!widget)
		return false;
	if (false)
		std::cerr << "onscreenkbd_focus_in_event: " << widget->get_name() << std::endl;
	Gtk::Entry *entry(dynamic_cast<Gtk::Entry *>(widget));
	if (entry) {
		entry->select_region(0, -1);
		if (!entry->get_editable())
			return false;
	} else {
		Gtk::TextView *txtview(dynamic_cast<Gtk::TextView *>(widget));
		if (txtview && !txtview->get_editable())
			return false;
	}
	if (kbd == kbd_off || !m_onscreenkeyboard || !m_keyboarddialog)
		return false;
	if (entry && kbd == kbd_auto) {
		Gtk::SpinButton *spinbtn(dynamic_cast<Gtk::SpinButton *>(entry));
		if (spinbtn && spinbtn->get_numeric())
			kbd = kbd_numeric;
	}
	int kw, kh, kx, ky, mw, mh, mx, my;
	m_keyboarddialog->open(widget, kw, kh, kbd != kbd_numeric, kbd != kbd_main);
	Gtk::Allocation ea(widget->get_allocation()); 
	get_size(mw, mh);
	get_position(mx, my);
	if (ea.get_y() > mh - ea.get_y() - ea.get_height())
		ky = my + ea.get_y() - kh - 10;
	else
		ky = my + ea.get_y() + ea.get_height() + 30;
	kx = std::max(std::min(mx + ea.get_x() + (ea.get_width() - kw) / 2, mx + mw - kw - 10), mx + 10);
	if (true)
		std::cerr << "Main Window: (" << mx << ',' << my << ") (" << mw << ',' << mh
			  << ") Entry: (" << ea.get_x() << ',' << ea.get_y() << ") (" << ea.get_width() << ',' << ea.get_height()
			  << ") Kbd: (" << kx << ',' << ky << ") (" << kw << ',' << kh << ')' << std::endl;
	m_keyboarddialog->move(kx, ky);
	m_keyboarddialog->set_skip_taskbar_hint(true);
	m_keyboarddialog->raise();
	m_keyboarddialog->set_accept_focus(true);
	m_keyboarddialog->present();
	m_keyboarddialog->get_window()->show();
	return false;
}

bool FlightDeckWindow::onscreenkbd_focus_out_event(GdkEventFocus *event)
{
	if (false)
		std::cerr << "fplpage_focus_out_event" << std::endl;
	if (m_keyboarddialog)
		m_keyboarddialog->close();
	return false;
}

const unsigned int FlightDeckWindow::KeyboardDialog::nrkeybuttons;
const struct FlightDeckWindow::KeyboardDialog::keybutton FlightDeckWindow::KeyboardDialog::keybuttons[nrkeybuttons] = {
	{ "keyboardbutton0", GDK_KEY_0, GDK_KEY_parenright, 19 },
	{ "keyboardbutton1", GDK_KEY_1, GDK_KEY_exclam, 10 },
	{ "keyboardbutton2", GDK_KEY_2, GDK_KEY_at, 11 },
	{ "keyboardbutton3", GDK_KEY_3, GDK_KEY_numbersign, 12 },
	{ "keyboardbutton4", GDK_KEY_4, GDK_KEY_dollar, 13 },
	{ "keyboardbutton5", GDK_KEY_5, GDK_KEY_percent, 14 },
	{ "keyboardbutton6", GDK_KEY_6, GDK_KEY_asciicircum, 15 },
	{ "keyboardbutton7", GDK_KEY_7, GDK_KEY_ampersand, 16 },
	{ "keyboardbutton8", GDK_KEY_8, GDK_KEY_asterisk, 17 },
	{ "keyboardbutton9", GDK_KEY_9, GDK_KEY_parenleft, 18 },
	{ "keyboardbuttona", GDK_KEY_a, GDK_KEY_A, 38 },
	{ "keyboardbuttonb", GDK_KEY_b, GDK_KEY_B, 56 },
	{ "keyboardbuttonc", GDK_KEY_c, GDK_KEY_C, 54 },
	{ "keyboardbuttond", GDK_KEY_d, GDK_KEY_D, 40 },
	{ "keyboardbuttone", GDK_KEY_e, GDK_KEY_E, 26 },
	{ "keyboardbuttonf", GDK_KEY_f, GDK_KEY_F, 41 },
	{ "keyboardbuttong", GDK_KEY_g, GDK_KEY_G, 42 },
	{ "keyboardbuttonh", GDK_KEY_h, GDK_KEY_H, 43 },
	{ "keyboardbuttoni", GDK_KEY_i, GDK_KEY_I, 31 },
	{ "keyboardbuttonj", GDK_KEY_j, GDK_KEY_J, 44 },
	{ "keyboardbuttonk", GDK_KEY_k, GDK_KEY_K, 45 },
	{ "keyboardbuttonl", GDK_KEY_l, GDK_KEY_L, 46 },
	{ "keyboardbuttonm", GDK_KEY_m, GDK_KEY_M, 58 },
	{ "keyboardbuttonn", GDK_KEY_n, GDK_KEY_N, 57 },
	{ "keyboardbuttono", GDK_KEY_o, GDK_KEY_O, 32 },
	{ "keyboardbuttonp", GDK_KEY_p, GDK_KEY_P, 33 },
	{ "keyboardbuttonq", GDK_KEY_q, GDK_KEY_Q, 24 },
	{ "keyboardbuttonr", GDK_KEY_r, GDK_KEY_R, 27 },
	{ "keyboardbuttons", GDK_KEY_s, GDK_KEY_S, 39 },
	{ "keyboardbuttont", GDK_KEY_t, GDK_KEY_T, 28 },
	{ "keyboardbuttonu", GDK_KEY_u, GDK_KEY_U, 30 },
	{ "keyboardbuttonv", GDK_KEY_v, GDK_KEY_V, 55 },
	{ "keyboardbuttonw", GDK_KEY_w, GDK_KEY_W, 25 },
	{ "keyboardbuttonx", GDK_KEY_x, GDK_KEY_X, 53 },
	{ "keyboardbuttony", GDK_KEY_y, GDK_KEY_Y, 29 },
	{ "keyboardbuttonz", GDK_KEY_z, GDK_KEY_Z, 52 },
	{ "keyboardbuttonapostrophe", GDK_KEY_quoteright, GDK_KEY_quotedbl, 48 },
	{ "keyboardbuttonbracketclose", GDK_KEY_bracketright, GDK_KEY_braceright, 35 },
	{ "keyboardbuttonbracketopen", GDK_KEY_bracketleft, GDK_KEY_braceleft, 34 },
	{ "keyboardbuttoncomma", GDK_KEY_comma, GDK_KEY_less, 59 },
	{ "keyboardbuttondash", GDK_KEY_minus, GDK_KEY_underscore, 20 },
	{ "keyboardbuttondot", GDK_KEY_period, GDK_KEY_greater, 60 },
	{ "keyboardbuttonequal", GDK_KEY_equal, GDK_KEY_plus, 21 },
	{ "keyboardbuttonquestionmark", GDK_KEY_question, GDK_KEY_slash, 61 },
	{ "keyboardbuttonsemicolon", GDK_KEY_semicolon, GDK_KEY_colon, 47 },
	{ "keyboardbuttonspace", GDK_KEY_space, GDK_KEY_space, 65 },
	{ "keyboardbuttonbacktick", GDK_KEY_quoteleft, GDK_KEY_asciitilde, 49 },
	{ "keyboardbuttonbackslash", GDK_KEY_backslash, GDK_KEY_bar, 51 },
	{ "keyboardbuttonbackspace", GDK_KEY_BackSpace, GDK_KEY_BackSpace, 22 },
	{ "keyboardbuttontab", GDK_KEY_Tab, GDK_KEY_Tab, 23 },
	{ "keyboardbuttonenter", GDK_KEY_Return, GDK_KEY_Return, 36 },
	{ "keyboardbuttonkeypad0", GDK_KEY_KP_0, GDK_KEY_KP_0, 90 },
	{ "keyboardbuttonkeypad1", GDK_KEY_KP_1, GDK_KEY_KP_1, 87 },
	{ "keyboardbuttonkeypad2", GDK_KEY_KP_2, GDK_KEY_KP_2, 88 },
	{ "keyboardbuttonkeypad3", GDK_KEY_KP_3, GDK_KEY_KP_3, 89 },
	{ "keyboardbuttonkeypad4", GDK_KEY_KP_4, GDK_KEY_KP_4, 83 },
	{ "keyboardbuttonkeypad5", GDK_KEY_KP_5, GDK_KEY_KP_5, 84 },
	{ "keyboardbuttonkeypad6", GDK_KEY_KP_6, GDK_KEY_KP_6, 85 },
	{ "keyboardbuttonkeypad7", GDK_KEY_KP_7, GDK_KEY_KP_7, 79 },
	{ "keyboardbuttonkeypad8", GDK_KEY_KP_8, GDK_KEY_KP_8, 80 },
	{ "keyboardbuttonkeypad9", GDK_KEY_KP_9, GDK_KEY_KP_9, 81 },
	{ "keyboardbuttonkeypaddot", GDK_KEY_KP_Decimal, GDK_KEY_KP_Decimal, 91 },
	{ "keyboardbuttonkeypadmul", GDK_KEY_KP_Multiply, GDK_KEY_KP_Multiply, 63 },
	{ "keyboardbuttonkeypaddiv", GDK_KEY_KP_Divide, GDK_KEY_KP_Divide, 106 },
	{ "keyboardbuttonkeypadplus", GDK_KEY_KP_Add, GDK_KEY_KP_Add, 86 },
	{ "keyboardbuttonkeypadminus", GDK_KEY_KP_Subtract, GDK_KEY_KP_Subtract, 82 },
	{ "keyboardbuttonkeypadenter", GDK_KEY_KP_Enter, GDK_KEY_KP_Enter, 104 },
	{ "keyboardbuttonkeypadtab", GDK_KEY_Tab, GDK_KEY_Tab, 23 }
};

FlightDeckWindow::KeyboardDialog::KeyboardDialog(BaseObjectType *castitem, const Glib::RefPtr<Builder>& refxml)
	: Gtk::Window(castitem), m_keyboardtable(0), m_keyboardtablemain(0), m_keyboardtablekeypad(0), m_keyboardvseparator(0),
	  m_keyboardtogglebuttoncapslock(0), m_keyboardtogglebuttonshiftleft(0), m_keyboardtogglebuttonshiftright(0),
	  m_shiftstate(shiftstate_none)
{
#ifndef GLIBMM_DEFAULT_SIGNAL_HANDLERS_ENABLED
        signal_delete_event().connect(sigc::mem_fun(*this, &KeyboardDialog::on_delete_event));
#endif //GLIBMM_DEFAULT_SIGNAL_HANDLERS_ENABLED
	get_widget(refxml, "keyboardtable", m_keyboardtable);
	get_widget(refxml, "keyboardtablemain", m_keyboardtablemain);
	get_widget(refxml, "keyboardtablekeypad", m_keyboardtablekeypad);
	get_widget(refxml, "keyboardvseparator", m_keyboardvseparator);
	get_widget(refxml, "keyboardtogglebuttonshiftleft", m_keyboardtogglebuttonshiftleft);
	get_widget(refxml, "keyboardtogglebuttonshiftright", m_keyboardtogglebuttonshiftright);
	get_widget(refxml, "keyboardtogglebuttoncapslock", m_keyboardtogglebuttoncapslock);
	if (m_keyboardtogglebuttonshiftleft)
		m_connshiftleft = m_keyboardtogglebuttonshiftleft->signal_clicked().connect(sigc::mem_fun(*this, &KeyboardDialog::shift_clicked));
	if (m_keyboardtogglebuttonshiftright)
		m_connshiftright = m_keyboardtogglebuttonshiftright->signal_clicked().connect(sigc::mem_fun(*this, &KeyboardDialog::shift_clicked));
	if (m_keyboardtogglebuttoncapslock)
		m_conncapslock = m_keyboardtogglebuttoncapslock->signal_clicked().connect(sigc::mem_fun(*this, &KeyboardDialog::capslock_clicked));
	for (unsigned int i = 0; i < nrkeybuttons; ++i) {
		m_keybutton[i] = 0;
		get_widget(refxml, keybuttons[i].widget_name, m_keybutton[i]);
		if (!m_keybutton[i])
			continue;
		m_keybutton[i]->signal_clicked().connect(sigc::bind(sigc::mem_fun(*this, &KeyboardDialog::key_clicked), i));
		m_keybutton[i]->set_can_focus(false);
	}
}

FlightDeckWindow::KeyboardDialog::~KeyboardDialog()
{
}

void FlightDeckWindow::KeyboardDialog::open(Widget *widget, bool main, bool keypad)
{
	int w, h;
	open(widget, w, h, main, keypad);
}

void FlightDeckWindow::KeyboardDialog::open(Gtk::Widget *widget, int& w, int& h, bool main, bool keypad)
{
	m_window.reset();
	if (m_keyboardtablemain)
		m_keyboardtablemain->set_visible(main);
	if (m_keyboardtablekeypad)
		m_keyboardtablekeypad->set_visible(keypad);
	if (m_keyboardvseparator)
		m_keyboardvseparator->set_visible(main && keypad);
	if (widget) {
		m_window = widget->get_window();
		check_resize();
		resize_children();
		show();
		Gtk::ResizeMode mode(get_resize_mode());
		set_resize_mode(Gtk::RESIZE_IMMEDIATE);
		resize(1, 1);
		set_resize_mode(mode);
	}
	{
		int hh(0), ww(0);
		if (main && m_keyboardtablemain) {
			Gtk::Allocation alloc(m_keyboardtablemain->get_allocation());
			ww += alloc.get_width();
			hh = std::max(hh, alloc.get_height());
		}
		if (main && keypad && m_keyboardvseparator) {
			Gtk::Allocation alloc(m_keyboardvseparator->get_allocation());
			ww += alloc.get_width();
			hh = std::max(hh, alloc.get_height());
		}
		if (keypad && m_keyboardtablekeypad) {
			Gtk::Allocation alloc(m_keyboardtablekeypad->get_allocation());
			ww += alloc.get_width();
			hh = std::max(hh, alloc.get_height());
		}
		w = ww;
		h = hh;
	}
}

void FlightDeckWindow::KeyboardDialog::close(void)
{
	hide();
	m_window.reset();
}

bool FlightDeckWindow::KeyboardDialog::on_delete_event(GdkEventAny* event)
{
        hide();
        return true;
}

void FlightDeckWindow::KeyboardDialog::shift_clicked(void)
{
	m_shiftstate = (m_shiftstate == shiftstate_once) ? shiftstate_none : shiftstate_once;
	set_shiftstate();
}

void FlightDeckWindow::KeyboardDialog::capslock_clicked(void)
{
	m_shiftstate = (m_shiftstate == shiftstate_lock) ? shiftstate_none : shiftstate_lock;
	set_shiftstate();
}

void FlightDeckWindow::KeyboardDialog::set_shiftstate(void)
{
	m_connshiftleft.block();
	m_connshiftright.block();
	m_conncapslock.block();
	if (m_keyboardtogglebuttonshiftleft)
		m_keyboardtogglebuttonshiftleft->set_active(m_shiftstate == shiftstate_once);
	if (m_keyboardtogglebuttonshiftright)
		m_keyboardtogglebuttonshiftright->set_active(m_shiftstate == shiftstate_once);
	if (m_keyboardtogglebuttoncapslock)
		m_keyboardtogglebuttoncapslock->set_active(m_shiftstate == shiftstate_lock);
	m_connshiftleft.unblock();
	m_connshiftright.unblock();
	m_conncapslock.unblock();
}

void FlightDeckWindow::KeyboardDialog::send_key(Gtk::Widget *w, guint state, guint keyval, guint16 hwkeycode)
{
	if (!w)
		return;
	send_key(w->get_window(), state, keyval, hwkeycode);
}

void FlightDeckWindow::KeyboardDialog::send_key(Glib::RefPtr<Gdk::Window> wnd, guint state, guint keyval, guint16 hwkeycode)
{
	GdkEvent evt;
	evt.key.type = GDK_KEY_PRESS;
	evt.key.window = wnd->gobj();
	evt.key.send_event = false;
	evt.key.time = GDK_CURRENT_TIME;
	evt.key.state = state; // GdkModifierType
	evt.key.keyval = keyval;
	gchar str[2];
	str[0] = 0;
	str[1] = 0;
	evt.key.length = 0;
	evt.key.string = str;
	switch (keyval) {
	case GDK_KEY_KP_Decimal:
		str[0] = '.';
		evt.key.length = 1;
		break;

	case GDK_KEY_KP_Multiply:
		str[0] = '*';
		evt.key.length = 1;
		break;

	case GDK_KEY_KP_Divide:
		str[0] = '/';
		evt.key.length = 1;
		break;

	case GDK_KEY_KP_Add:
		str[0] = '+';
		evt.key.length = 1;
		break;

	case GDK_KEY_KP_Subtract:
		str[0] = '-';
		evt.key.length = 1;
		break;

	default:
		if (keyval > 0 && keyval < 127) {
			str[0] = keyval;
			evt.key.length = 1;
		} else if (keyval >= GDK_KEY_KP_0 && keyval <= GDK_KEY_KP_9) {
			str[0] = keyval + ('0' - GDK_KEY_KP_0);
			evt.key.length = 1;
		}
	}
	evt.key.hardware_keycode = hwkeycode;
	evt.key.group = 0;
	evt.key.is_modifier = 0;
	gtk_main_do_event(&evt);
	evt.key.type = GDK_KEY_RELEASE;
	gtk_main_do_event(&evt);
}

void FlightDeckWindow::KeyboardDialog::key_clicked(unsigned int i)
{
 	if (i >= nrkeybuttons)
		return;
	guint keyval((m_shiftstate != shiftstate_none) ? keybuttons[i].key_shift : keybuttons[i].key);
	if (false)
		std::cerr << "key_clicked: " << keyval << std::endl;
	send_key(m_window, ((m_shiftstate == shiftstate_once) ? GDK_SHIFT_MASK : 0) |
		 ((m_shiftstate == shiftstate_lock) ? GDK_LOCK_MASK : 0), keyval, keybuttons[i].hwkeycode);
	if (m_shiftstate == shiftstate_once) {
		m_shiftstate = shiftstate_none;
		set_shiftstate();
	}
}
