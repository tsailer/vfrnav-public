//
// C++ Implementation: Sensor Configuration Dialog
//
// Description: Sensor Configuration Dialog
//
//
// Author: Thomas Sailer <t.sailer@alumni.ethz.ch>, (C) 2011, 2012
//
// Copyright: See COPYING file that comes with this distribution
//
//

#include "sysdeps.h"

#include <iomanip>
#include <glibmm.h>

#include "flightdeckwindow.h"
#include "modes.h"

template <typename W> class FlightDeckWindow::ConfigItem::Fail : public W {
public:
	explicit Fail(void);
	void set_paramfail(Sensors::Sensor::paramfail_t pf);

protected:
#ifdef HAVE_GTKMM2
	virtual bool on_expose_event(GdkEventExpose *event);
#endif
	virtual bool on_draw(const Cairo::RefPtr<Cairo::Context>& cr);
	Sensors::Sensor::paramfail_t m_paramfail;
};

template <typename W> FlightDeckWindow::ConfigItem::Fail<W>::Fail(void)
	: W(), m_paramfail(Sensors::Sensor::paramfail_fail)
{
#ifndef GLIBMM_DEFAULT_SIGNAL_HANDLERS_ENABLED
#ifdef HAVE_GTKMM2
        signal_expose_event().connect(sigc::mem_fun(*this, &Fail<W>::on_expose_event));
#endif
#ifdef HAVE_GTKMM3
        signal_draw().connect(sigc::mem_fun(*this, &Fail<W>::on_draw));
#endif
#endif //GLIBMM_DEFAULT_SIGNAL_HANDLERS_ENABLED
}

template <typename W> void FlightDeckWindow::ConfigItem::Fail<W>::set_paramfail(Sensors::Sensor::paramfail_t pf)
{
	m_paramfail = pf;
	if (W::get_is_drawable())
		W::queue_draw();
}

#ifdef HAVE_GTKMM2
template <typename W> bool FlightDeckWindow::ConfigItem::Fail<W>::on_expose_event(GdkEventExpose *event)
{
	bool ret(true);
#ifdef GLIBMM_DEFAULT_SIGNAL_HANDLERS_ENABLED
	ret = W::on_expose_event(event);
#endif //GLIBMM_DEFAULT_SIGNAL_HANDLERS_ENABLED
        Glib::RefPtr<Gdk::Window> wnd(this->get_window());
        if (!wnd)
                return false;
        Cairo::RefPtr<Cairo::Context> cr(wnd->create_cairo_context());
	if (event) {
		GdkRectangle *rects;
		int n_rects;
		gdk_region_get_rectangles(event->region, &rects, &n_rects);
		for (int i = 0; i < n_rects; i++)
			cr->rectangle(rects[i].x, rects[i].y, rects[i].width, rects[i].height);
		cr->clip();
		g_free(rects);
	}
	if (!this->get_has_window()) {
		Gtk::Allocation alloc(this->get_allocation());
		cr->translate(alloc.get_x(), alloc.get_y());
	}
	on_draw(cr);
        return ret;
}
#endif

template <typename W> bool FlightDeckWindow::ConfigItem::Fail<W>::on_draw(const Cairo::RefPtr<Cairo::Context>& cr)
{
	bool ret(true);
#ifdef GLIBMM_DEFAULT_SIGNAL_HANDLERS_ENABLED
#ifdef HAVE_GTKMM3
	ret = W::on_draw(cr);
#endif
#endif //GLIBMM_DEFAULT_SIGNAL_HANDLERS_ENABLED
#ifdef HAVE_GTKMM3
	if (m_paramfail != Sensors::Sensor::paramfail_ok)
		draw_failflag(cr, Gtk::Allocation(0, 0, this->get_allocated_width(), this->get_allocated_height()));
#else
	if (m_paramfail != Sensors::Sensor::paramfail_ok)
		draw_failflag(cr, Gtk::Allocation(0, 0, this->get_width(), this->get_height()));
#endif
	return ret;
}

template<> bool FlightDeckWindow::ConfigItem::Fail<Gtk::SpinButton>::on_draw(const Cairo::RefPtr<Cairo::Context>& cr)
{
	bool ret(true);
#ifdef GLIBMM_DEFAULT_SIGNAL_HANDLERS_ENABLED
#ifdef HAVE_GTKMM3
	ret = Gtk::SpinButton::on_draw(cr);
#endif
#endif //GLIBMM_DEFAULT_SIGNAL_HANDLERS_ENABLED
#ifdef HAVE_GTKMM2
	if (m_paramfail != Sensors::Sensor::paramfail_ok)
		draw_failflag(get_text_window());
#endif
#ifdef HAVE_GTKMM3
	if (m_paramfail != Sensors::Sensor::paramfail_ok)
		draw_failflag(cr, get_text_area());
#endif
	return ret;
}

template<> bool FlightDeckWindow::ConfigItem::Fail<Gtk::Entry>::on_draw(const Cairo::RefPtr<Cairo::Context>& cr)
{
	bool ret(true);
#ifdef GLIBMM_DEFAULT_SIGNAL_HANDLERS_ENABLED
#ifdef HAVE_GTKMM3
	ret = Gtk::Entry::on_draw(cr);
#endif
#endif //GLIBMM_DEFAULT_SIGNAL_HANDLERS_ENABLED
#ifdef HAVE_GTKMM2
	if (m_paramfail != Sensors::Sensor::paramfail_ok)
		draw_failflag(get_text_window());
#endif
#ifdef HAVE_GTKMM3
	if (m_paramfail != Sensors::Sensor::paramfail_ok)
		draw_failflag(cr, get_text_area());
#endif
	return ret;
}

FlightDeckWindow::ConfigItem::ConfigItem(Sensors::Sensor& sens, const Sensors::Sensor::ParamDesc& pd)
	: m_sensor(sens), m_refcount(1), m_paramnr(pd.get_parnumber()), m_editable(pd.get_editable())
{
}

FlightDeckWindow::ConfigItem::~ConfigItem()
{
}

void FlightDeckWindow::ConfigItem::reference(void) const
{
        ++m_refcount;
}

void FlightDeckWindow::ConfigItem::unreference(void) const
{
        if (--m_refcount)
                return;
        delete this;
}

void FlightDeckWindow::ConfigItem::update(const Sensors::Sensor::ParamChanged& pc)
{
}

void FlightDeckWindow::ConfigItem::set_admin(bool admin)
{
	m_admin = admin;
}

void FlightDeckWindow::ConfigItem::draw_failflag(Cairo::RefPtr<Cairo::Context> cr, const Gtk::Allocation& alloc)
{
	cr->save();
	cr->translate(alloc.get_x(), alloc.get_y());
	cr->rectangle(0.0, 0.0, alloc.get_width(), alloc.get_height());
	cr->clip();
	cr->set_line_width(5.0);
	//cr->set_source_rgba(0.0, 0.0, 0.0, 0.0);
	//cr->set_source_rgb(1.0, 0.0, 0.0);
	//cr->paint();
	cr->set_source_rgba(1.0, 0.0, 0.0, 0.7);
	cr->begin_new_path();
	cr->move_to(2, 2);
	cr->line_to(alloc.get_width() - 2, alloc.get_height() - 2);
	cr->move_to(2, alloc.get_height() - 2);
	cr->line_to(alloc.get_width() - 2, 2);
	cr->stroke();
	cr->restore();
}

void FlightDeckWindow::ConfigItem::draw_failflag(Glib::RefPtr<Gdk::Window> wnd, const Gtk::Allocation& alloc)
{
	if (!wnd)
		return;
	Cairo::RefPtr<Cairo::Context> cr(wnd->create_cairo_context());
	draw_failflag(cr, alloc);
}

void FlightDeckWindow::ConfigItem::draw_failflag(Glib::RefPtr<Gdk::Window> wnd)
{
	if (!wnd)
		return;
	draw_failflag(wnd, Gtk::Allocation(0, 0, wnd->get_width(), wnd->get_height()));
}

Sensors::Sensor::paramfail_t FlightDeckWindow::ConfigItem::get_param(Glib::ustring& v) const
{
	return get_sensor().get_param(get_paramnr(), v);
}

Sensors::Sensor::paramfail_t FlightDeckWindow::ConfigItem::get_param(int& v) const
{
	return get_sensor().get_param(get_paramnr(), v);
}

Sensors::Sensor::paramfail_t FlightDeckWindow::ConfigItem::get_param(double& v) const
{
	return get_sensor().get_param(get_paramnr(), v);
}

Sensors::Sensor::paramfail_t FlightDeckWindow::ConfigItem::get_param(double& pitch, double& bank, double& slip, double& hdg, double& rate) const
{
	return get_sensor().get_param(get_paramnr(), pitch, bank, slip, hdg, rate);
}

Sensors::Sensor::paramfail_t FlightDeckWindow::ConfigItem::get_param(Sensors::Sensor::satellites_t& sat) const
{
	return get_sensor().get_param(get_paramnr(), sat);
}

Sensors::Sensor::paramfail_t FlightDeckWindow::ConfigItem::get_param(Sensors::Sensor::adsbtargets_t& t) const
{
	return get_sensor().get_param(get_paramnr(), t);
}

void FlightDeckWindow::ConfigItem::set_param(const Glib::ustring& v)
{
	get_sensor().set_param(get_paramnr(), v);
}

void FlightDeckWindow::ConfigItem::set_param(const int& v)
{
	get_sensor().set_param(get_paramnr(), v);
}

void FlightDeckWindow::ConfigItem::set_param(const double& v)
{
	get_sensor().set_param(get_paramnr(), v);
}

Sensors::Sensor::paramfail_t FlightDeckWindow::ConfigItem::get_param(unsigned int idx, Glib::ustring& v) const
{
	return get_sensor().get_param(get_paramnr() + idx, v);
}

Sensors::Sensor::paramfail_t FlightDeckWindow::ConfigItem::get_param(unsigned int idx, int& v) const
{
	return get_sensor().get_param(get_paramnr() + idx, v);
}

Sensors::Sensor::paramfail_t FlightDeckWindow::ConfigItem::get_param(unsigned int idx, double& v) const
{
	return get_sensor().get_param(get_paramnr() + idx, v);
}

Sensors::Sensor::paramfail_t FlightDeckWindow::ConfigItem::get_param(unsigned int idx, Sensors::Sensor::satellites_t& sat) const
{
	return get_sensor().get_param(get_paramnr() + idx, sat);
}

Sensors::Sensor::paramfail_t FlightDeckWindow::ConfigItem::get_param(unsigned int idx, Sensors::Sensor::adsbtargets_t& t) const
{
	return get_sensor().get_param(get_paramnr() + idx, t);
}

void FlightDeckWindow::ConfigItem::set_param(unsigned int idx, const Glib::ustring& v)
{
	get_sensor().set_param(get_paramnr() + idx, v);
}

void FlightDeckWindow::ConfigItem::set_param(unsigned int idx, const int& v)
{
	get_sensor().set_param(get_paramnr() + idx, v);
}

void FlightDeckWindow::ConfigItem::set_param(unsigned int idx, const double& v)
{
	get_sensor().set_param(get_paramnr() + idx, v);
}

class FlightDeckWindow::ConfigItemString : public FlightDeckWindow::ConfigItem {
public:
	ConfigItemString(Sensors::Sensor& sens, const Sensors::Sensor::ParamDesc& pd);
	virtual void update(const Sensors::Sensor::ParamChanged& pc);
	virtual void set_admin(bool admin);
	virtual Gtk::Widget *get_widget(void) { return &m_entry; }
	virtual Gtk::Widget *get_screenkbd_widget(bool& numeric) { numeric = false; return &m_entry; }

protected:
	Fail<Gtk::Entry> m_entry;

	void changed(void);
};

FlightDeckWindow::ConfigItemString::ConfigItemString(Sensors::Sensor& sens, const Sensors::Sensor::ParamDesc& pd)
	: ConfigItem(sens, pd)
{
	m_entry.set_visible(true);
	m_entry.set_can_focus(!is_readonly());
	m_entry.set_max_length(pd.get_maxlength());
	m_entry.set_width_chars(pd.get_maxlength() ? pd.get_maxlength() : 8);
	if (!is_readonly())
		m_entry.signal_changed().connect(sigc::mem_fun(*this, &ConfigItemString::changed));
	m_entry.set_editable(is_writable());
	m_entry.set_sensitive(true);
	m_entry.set_width_chars(16);
	//m_entry.set_placeholder_text(pd.get_description());
}

void FlightDeckWindow::ConfigItemString::update(const Sensors::Sensor::ParamChanged& pc)
{
	FlightDeckWindow::ConfigItem::update(pc);
	if (!pc.is_changed(get_paramnr()))
		return;
	Glib::ustring v;
	m_entry.set_paramfail(get_param(v));
	m_entry.set_text(v);
}

void FlightDeckWindow::ConfigItemString::set_admin(bool admin)
{
	FlightDeckWindow::ConfigItem::set_admin(admin);
	m_entry.set_editable(is_writable());
}

void FlightDeckWindow::ConfigItemString::changed(void)
{
	set_param(m_entry.get_text());
}

class FlightDeckWindow::ConfigItemSwitch : public FlightDeckWindow::ConfigItem {
public:
	ConfigItemSwitch(Sensors::Sensor& sens, const Sensors::Sensor::ParamDesc& pd);
	virtual void update(const Sensors::Sensor::ParamChanged& pc);
	virtual void set_admin(bool admin);
	virtual Gtk::Widget *get_widget(void) { return m_hbox ? (Gtk::Widget *)m_hbox : (Gtk::Widget *)m_checkbuttons.front(); }

protected:
	Gtk::HBox *m_hbox;
	std::vector<Fail<Gtk::CheckButton> *> m_checkbuttons;

	void update(void);
	void changed(void);
};

FlightDeckWindow::ConfigItemSwitch::ConfigItemSwitch(Sensors::Sensor& sens, const Sensors::Sensor::ParamDesc& pd)
	: ConfigItem(sens, pd), m_hbox(0)
{
	const Sensors::Sensor::ParamDesc::choices_t& labels(pd.get_choices());
	for (unsigned int i = 0; i < labels.size() || i < 1; ++i) {
		m_checkbuttons.push_back(Gtk::manage(new Fail<Gtk::CheckButton>()));
		m_checkbuttons.back()->set_visible(true);
		if (labels.empty())
			m_checkbuttons.back()->set_label("On/Off");
		else
			m_checkbuttons.back()->set_label(labels[i]);
		if (!is_readonly())
			m_checkbuttons.back()->signal_toggled().connect(sigc::mem_fun(*this, &ConfigItemSwitch::changed), i);
		m_checkbuttons.back()->set_sensitive(is_writable());
	}
	if (m_checkbuttons.size() > 1) {
		m_hbox = Gtk::manage(new Gtk::HBox(false, 2));
		for (unsigned int i = 0; i < m_checkbuttons.size(); ++i)
			m_hbox->pack_start(*m_checkbuttons[i], true, true, 2);
	}
}

void FlightDeckWindow::ConfigItemSwitch::update(void)
{
	int v;
	Sensors::Sensor::paramfail_t pf(get_param(v));
	for (unsigned int i = 0; i < m_checkbuttons.size(); ++i) {
		m_checkbuttons[i]->set_paramfail(pf);
		m_checkbuttons[i]->set_active(!!(v & (1 << i)));
	}
}

void FlightDeckWindow::ConfigItemSwitch::update(const Sensors::Sensor::ParamChanged& pc)
{
	FlightDeckWindow::ConfigItem::update(pc);
	if (!pc.is_changed(get_paramnr()))
		return;
	update();
}

void FlightDeckWindow::ConfigItemSwitch::set_admin(bool admin)
{
	FlightDeckWindow::ConfigItem::set_admin(admin);
	for (unsigned int i = 0; i < m_checkbuttons.size(); ++i)
		m_checkbuttons[i]->set_sensitive(is_writable());
}

void FlightDeckWindow::ConfigItemSwitch::changed(void)
{
	int v(0);
	for (unsigned int i = 0; i < m_checkbuttons.size(); ++i)
		if (m_checkbuttons[i]->get_active())
			v |= 1 << i;
	set_param(v);
	update();
}

class FlightDeckWindow::ConfigItemNumeric : public FlightDeckWindow::ConfigItem {
public:
	ConfigItemNumeric(Sensors::Sensor& sens, const Sensors::Sensor::ParamDesc& pd);
	virtual void set_admin(bool admin);
	virtual Gtk::Widget *get_widget(void) { return &m_spinbutton; }
	virtual Gtk::Widget *get_screenkbd_widget(bool& numeric) { numeric = true; return &m_spinbutton; }

	class ConfigItemDouble;
	class ConfigItemInteger;
	class ConfigItemReadOnlyNumeric;
	class ConfigItemReadOnlyDouble;
	class ConfigItemReadOnlyInteger;

protected:
	Fail<Gtk::SpinButton> m_spinbutton;
};

FlightDeckWindow::ConfigItemNumeric::ConfigItemNumeric(Sensors::Sensor& sens, const Sensors::Sensor::ParamDesc& pd)
	: ConfigItem(sens, pd)
{
	unsigned int digits;
	double min, max, stepinc, pageinc;
	pd.get_range(digits, min, max, stepinc, pageinc);
	m_spinbutton.set_visible(true);
	m_spinbutton.set_can_focus(!is_readonly());
	if (is_readonly())
		stepinc = pageinc = 0;
	m_spinbutton.set_increments(stepinc, pageinc);
	m_spinbutton.set_range(min, max);
	m_spinbutton.set_alignment(1.0);
	m_spinbutton.set_editable(is_writable());
	m_spinbutton.set_width_chars(8);
	m_spinbutton.set_digits(digits);
}

void FlightDeckWindow::ConfigItemNumeric::set_admin(bool admin)
{
	FlightDeckWindow::ConfigItem::set_admin(admin);
	m_spinbutton.set_editable(is_writable());
}

class FlightDeckWindow::ConfigItemNumeric::ConfigItemDouble : public ConfigItemNumeric {
public:
	ConfigItemDouble(Sensors::Sensor& sens, const Sensors::Sensor::ParamDesc& pd);
	virtual void update(const Sensors::Sensor::ParamChanged& pc);

protected:
	void update(void);
	void changed(void);
};

FlightDeckWindow::ConfigItemNumeric::ConfigItemDouble::ConfigItemDouble(Sensors::Sensor& sens, const Sensors::Sensor::ParamDesc& pd)
	: ConfigItemNumeric(sens, pd)
{
	unsigned int digits;
	double min, max, stepinc, pageinc;
	pd.get_range(digits, min, max, stepinc, pageinc);
	m_spinbutton.set_digits(digits);
	if (true || !is_readonly())
		m_spinbutton.signal_value_changed().connect(sigc::mem_fun(*this, &ConfigItemDouble::changed));
}

void FlightDeckWindow::ConfigItemNumeric::ConfigItemDouble::update(void)
{
	double v;
	m_spinbutton.set_paramfail(get_param(v));
	m_spinbutton.set_value(v);
}

void FlightDeckWindow::ConfigItemNumeric::ConfigItemDouble::update(const Sensors::Sensor::ParamChanged& pc)
{
	FlightDeckWindow::ConfigItem::update(pc);
	if (!pc.is_changed(get_paramnr()))
		return;
	update();
}

void FlightDeckWindow::ConfigItemNumeric::ConfigItemDouble::changed(void)
{
	// "non-editable" spin buttons are still changeable with the arrows - sigh
	if (is_writable())
		set_param(m_spinbutton.get_value());
	else
		update();
}

class FlightDeckWindow::ConfigItemNumeric::ConfigItemInteger : public ConfigItemNumeric {
public:
	ConfigItemInteger(Sensors::Sensor& sens, const Sensors::Sensor::ParamDesc& pd);
	virtual void update(const Sensors::Sensor::ParamChanged& pc);

protected:
	void update(void);
	void changed(void);
};

FlightDeckWindow::ConfigItemNumeric::ConfigItemInteger::ConfigItemInteger(Sensors::Sensor& sens, const Sensors::Sensor::ParamDesc& pd)
	: ConfigItemNumeric(sens, pd)
{
	unsigned int digits;
	double min, max, stepinc, pageinc;
	pd.get_range(digits, min, max, stepinc, pageinc);
	m_spinbutton.set_digits(digits);
	if (true || !is_readonly())
		m_spinbutton.signal_value_changed().connect(sigc::mem_fun(*this, &ConfigItemInteger::changed));
}

void FlightDeckWindow::ConfigItemNumeric::ConfigItemInteger::update(void)
{
	int v;
	m_spinbutton.set_paramfail(get_param(v));
	m_spinbutton.set_value(v);
}

void FlightDeckWindow::ConfigItemNumeric::ConfigItemInteger::update(const Sensors::Sensor::ParamChanged& pc)
{
	FlightDeckWindow::ConfigItem::update(pc);
	if (!pc.is_changed(get_paramnr()))
		return;
	update();
}

void FlightDeckWindow::ConfigItemNumeric::ConfigItemInteger::changed(void)
{
	// "non-editable" spin buttons are still changeable with the arrows - sigh
	if (is_writable())
		set_param(m_spinbutton.get_value_as_int());
	else
		update();
}

class FlightDeckWindow::ConfigItemNumeric::ConfigItemReadOnlyNumeric : public ConfigItem {
public:
	ConfigItemReadOnlyNumeric(Sensors::Sensor& sens, const Sensors::Sensor::ParamDesc& pd);
	virtual void set_admin(bool admin);
	virtual Gtk::Widget *get_widget(void) { return &m_entry; }
	virtual Gtk::Widget *get_screenkbd_widget(bool& numeric) { numeric = true; return &m_entry; }

protected:
	Fail<Gtk::Entry> m_entry;
};

FlightDeckWindow::ConfigItemNumeric::ConfigItemReadOnlyNumeric::ConfigItemReadOnlyNumeric(Sensors::Sensor& sens, const Sensors::Sensor::ParamDesc& pd)
	: ConfigItem(sens, pd)
{
	m_entry.set_visible(true);
	m_entry.set_can_focus(!is_readonly());
	m_entry.set_max_length(0);
	m_entry.set_width_chars(8);
	m_entry.set_editable(is_writable());
	m_entry.set_alignment(1.0);
	//m_entry.set_placeholder_text(pd.get_description());
	//m_entry.signal_focus_in_event().connect(sigc::bind(sigc::bind(sigc::mem_fun(*this, &FlightDeckWindow::onscreenkbd_focus_in_event), kbd_numeric), m_entry));
	//m_entry.signal_focus_out_event().connect(sigc::mem_fun(*this, &FlightDeckWindow::onscreenkbd_focus_out_event));
}

void FlightDeckWindow::ConfigItemNumeric::ConfigItemReadOnlyNumeric::set_admin(bool admin)
{
	FlightDeckWindow::ConfigItem::set_admin(admin);
	m_entry.set_editable(is_writable());
}

class FlightDeckWindow::ConfigItemNumeric::ConfigItemReadOnlyDouble : public ConfigItemReadOnlyNumeric {
public:
	ConfigItemReadOnlyDouble(Sensors::Sensor& sens, const Sensors::Sensor::ParamDesc& pd);
	virtual void update(const Sensors::Sensor::ParamChanged& pc);

protected:
	unsigned int m_digits;
	double m_min;
	double m_max;
	void update(void);
	void changed(void);
};

FlightDeckWindow::ConfigItemNumeric::ConfigItemReadOnlyDouble::ConfigItemReadOnlyDouble(Sensors::Sensor& sens, const Sensors::Sensor::ParamDesc& pd)
	: ConfigItemReadOnlyNumeric(sens, pd)
{
	double stepinc, pageinc;
	pd.get_range(m_digits, m_min, m_max, stepinc, pageinc);
	if (!is_readonly())
		m_entry.signal_changed().connect(sigc::mem_fun(*this, &ConfigItemNumeric::ConfigItemReadOnlyDouble::changed));
}

void FlightDeckWindow::ConfigItemNumeric::ConfigItemReadOnlyDouble::update(void)
{
	double v;
	m_entry.set_paramfail(get_param(v));
	{
		std::ostringstream oss;
		oss << std::setprecision(m_digits) << std::fixed << v;
		m_entry.set_text(oss.str());
	}
}

void FlightDeckWindow::ConfigItemNumeric::ConfigItemReadOnlyDouble::update(const Sensors::Sensor::ParamChanged& pc)
{
	FlightDeckWindow::ConfigItem::update(pc);
	if (!pc.is_changed(get_paramnr()))
		return;
	update();
}

void FlightDeckWindow::ConfigItemNumeric::ConfigItemReadOnlyDouble::changed(void)
{
	set_param(std::max(m_min, std::min(m_max, Glib::Ascii::strtod(m_entry.get_text()))));
	update();
}

class FlightDeckWindow::ConfigItemNumeric::ConfigItemReadOnlyInteger : public ConfigItemReadOnlyNumeric {
public:
	ConfigItemReadOnlyInteger(Sensors::Sensor& sens, const Sensors::Sensor::ParamDesc& pd);
	virtual void update(const Sensors::Sensor::ParamChanged& pc);

protected:
	unsigned int m_digits;
	int m_min;
	int m_max;
	void update(void);
	void changed(void);
};

FlightDeckWindow::ConfigItemNumeric::ConfigItemReadOnlyInteger::ConfigItemReadOnlyInteger(Sensors::Sensor& sens, const Sensors::Sensor::ParamDesc& pd)
	: ConfigItemReadOnlyNumeric(sens, pd)
{
	unsigned int digits;
	double min, max, stepinc, pageinc;
	pd.get_range(digits, min, max, stepinc, pageinc);
	m_min = std::max(min, (double)std::numeric_limits<int>::min());
	m_max = std::min(max, (double)std::numeric_limits<int>::max());
	if (!is_readonly())
		m_entry.signal_changed().connect(sigc::mem_fun(*this, &ConfigItemNumeric::ConfigItemReadOnlyInteger::changed));
}

void FlightDeckWindow::ConfigItemNumeric::ConfigItemReadOnlyInteger::update(void)
{
	int v;
	m_entry.set_paramfail(get_param(v));
	{
		std::ostringstream oss;
		oss << v;
		m_entry.set_text(oss.str());
	}
}

void FlightDeckWindow::ConfigItemNumeric::ConfigItemReadOnlyInteger::update(const Sensors::Sensor::ParamChanged& pc)
{
	FlightDeckWindow::ConfigItem::update(pc);
	if (!pc.is_changed(get_paramnr()))
		return;
	update();
}

void FlightDeckWindow::ConfigItemNumeric::ConfigItemReadOnlyInteger::changed(void)
{
	int v = std::max(m_min, std::min(m_max, (int)strtol(m_entry.get_text().c_str(), 0, 0)));
	set_param(v);
	update();
}

class FlightDeckWindow::ConfigItemChoice : public FlightDeckWindow::ConfigItem {
public:
	ConfigItemChoice(Sensors::Sensor& sens, const Sensors::Sensor::ParamDesc& pd);
	virtual void update(const Sensors::Sensor::ParamChanged& pc);
	virtual void set_admin(bool admin);
	virtual Gtk::Widget *get_widget(void) { return m_combobox; }

protected:
	class ModelColumns : public Gtk::TreeModelColumnRecord {
	public:
		ModelColumns(void);
		Gtk::TreeModelColumn<Glib::ustring> m_name;
		Gtk::TreeModelColumn<int> m_value;
	};

	Fail<Gtk::ComboBox> *m_combobox;
	ModelColumns m_listcolumns;
	Glib::RefPtr<Gtk::ListStore> m_liststore;

	void changed(void);
};

FlightDeckWindow::ConfigItemChoice::ModelColumns::ModelColumns(void)
{
	add(m_name);
	add(m_value);
}

FlightDeckWindow::ConfigItemChoice::ConfigItemChoice(Sensors::Sensor& sens, const Sensors::Sensor::ParamDesc& pd)
	: ConfigItem(sens, pd), m_combobox(0)
{
        m_liststore = Gtk::ListStore::create(m_listcolumns);
	{
		const Sensors::Sensor::ParamDesc::choices_t& ch(pd.get_choices());
		unsigned int i = 0;
		for (Sensors::Sensor::ParamDesc::choices_t::const_iterator chi(ch.begin()), che(ch.end()); chi != che; ++chi, ++i) {
			Gtk::TreeModel::Row row(*(m_liststore->append()));
			row[m_listcolumns.m_name] = *chi;
			row[m_listcolumns.m_value] = i;
			if (false)
				std::cerr << "Choice: add " << *chi << std::endl;
		}
	}
	m_combobox = Gtk::manage(new Fail<Gtk::ComboBox>());
	m_combobox->set_visible(true);
	m_combobox->set_can_focus(!is_readonly());
        m_combobox->set_model(m_liststore);
	m_combobox->set_entry_text_column(m_listcolumns.m_name);
	m_combobox->pack_start(m_listcolumns.m_name);
	if (!is_readonly())
		m_combobox->signal_changed().connect(sigc::mem_fun(*this, &ConfigItemChoice::changed));
	m_combobox->set_sensitive(is_writable());
}

void FlightDeckWindow::ConfigItemChoice::update(const Sensors::Sensor::ParamChanged& pc)
{
	FlightDeckWindow::ConfigItem::update(pc);
	if (!pc.is_changed(get_paramnr()))
		return;
	int v;
	m_combobox->set_paramfail(get_param(v));
        Gtk::TreeIter iter = m_liststore->children().begin();
        while (iter) {
		Gtk::TreeModel::Row row(*iter);
		if (row[m_listcolumns.m_value] == v)
			break;
		++iter;
	}
 	m_combobox->set_active(iter);
}

void FlightDeckWindow::ConfigItemChoice::set_admin(bool admin)
{
	FlightDeckWindow::ConfigItem::set_admin(admin);
	m_combobox->set_sensitive(is_writable());
}

void FlightDeckWindow::ConfigItemChoice::changed(void)
{
	Gtk::TreeIter it(m_combobox->get_active());
	if (it) {
		Gtk::TreeModel::Row row(*it);
		set_param(row[m_listcolumns.m_value]);
	}
}

class FlightDeckWindow::ConfigItemButton : public FlightDeckWindow::ConfigItem {
public:
	ConfigItemButton(Sensors::Sensor& sens, const Sensors::Sensor::ParamDesc& pd);
	virtual void update(const Sensors::Sensor::ParamChanged& pc);
	virtual void set_admin(bool admin);
	virtual Gtk::Widget *get_widget(void) { return m_buttonbox; }

protected:
	Gtk::HButtonBox *m_buttonbox;

	void update(void);
	void changed(unsigned int bnr);
};

FlightDeckWindow::ConfigItemButton::ConfigItemButton(Sensors::Sensor& sens, const Sensors::Sensor::ParamDesc& pd)
	: ConfigItem(sens, pd), m_buttonbox(0)
{
	m_buttonbox = Gtk::manage(new Gtk::HButtonBox());
 	m_buttonbox->set_layout(Gtk::BUTTONBOX_SPREAD);
	const Sensors::Sensor::ParamDesc::choices_t& labels(pd.get_choices());
	for (unsigned int i = 0; i < labels.size() || i < 1; ++i) {
		Fail<Gtk::ToggleButton> *btn(Gtk::manage(new Fail<Gtk::ToggleButton>()));
		if (labels.empty()) {
			btn->set_label("On/Off");
		} else {
			bool stock(!labels[i].empty() && labels[i][0] == '$');
			if (stock) {
				Gtk::StockItem sitem;
				stock = Gtk::StockItem::lookup(Gtk::StockID(labels[i].substr(1)), sitem);
				if (stock) {
					Gtk::Image *img = Gtk::manage(new Gtk::Image(sitem.get_stock_id(), Gtk::IconSize(Gtk::ICON_SIZE_SMALL_TOOLBAR /* Gtk::ICON_SIZE_BUTTON */)));
					btn->set_image(*img);
				}
			}
			if (!stock)
				btn->set_label(labels[i]);
		}
		btn->set_visible(true);
		if (!is_readonly())
			btn->signal_toggled().connect(sigc::bind(sigc::mem_fun(*this, &ConfigItemButton::changed), i));
		btn->set_sensitive(is_writable());
		m_buttonbox->add(*btn);
	}
}

void FlightDeckWindow::ConfigItemButton::update(void)
{
	int v;
	Sensors::Sensor::paramfail_t pf(get_param(v));
	std::vector<Gtk::Widget *> children(m_buttonbox->get_children());
	for (int i = 0; i < (int)children.size(); ++i) {
		Fail<Gtk::ToggleButton> *btn(static_cast<Fail<Gtk::ToggleButton> *>(children[i]));
		btn->set_paramfail(pf);
		btn->set_active(!!(v & (1 << i)));
	}
}

void FlightDeckWindow::ConfigItemButton::update(const Sensors::Sensor::ParamChanged& pc)
{
	FlightDeckWindow::ConfigItem::update(pc);
	if (!pc.is_changed(get_paramnr()))
		return;
	update();
}

void FlightDeckWindow::ConfigItemButton::set_admin(bool admin)
{
	FlightDeckWindow::ConfigItem::set_admin(admin);
	std::vector<Gtk::Widget *> children(m_buttonbox->get_children());
	for (int i = 0; i < (int)children.size(); ++i) {
		Fail<Gtk::ToggleButton> *btn(static_cast<Fail<Gtk::ToggleButton> *>(children[i]));
		btn->set_sensitive(is_writable());
	}
}

void FlightDeckWindow::ConfigItemButton::changed(unsigned int bnr)
{
	set_param((1 << bnr));
	update();
}

class FlightDeckWindow::ConfigItemGyro : public FlightDeckWindow::ConfigItem {
public:
	ConfigItemGyro(Sensors::Sensor& sens, const Sensors::Sensor::ParamDesc& pd);
	virtual void update(const Sensors::Sensor::ParamChanged& pc);
	virtual Gtk::Widget *get_widget(void) { return &m_vbox; }
	virtual bool is_fastupdate(void) const { return true; }

protected:
	Gtk::VBox m_vbox;
	Gtk::AspectFrame m_afai;
	Gtk::AspectFrame m_afhsi;
	NavAIDrawingArea m_ai;
	NavHSIDrawingArea m_hsi;
};

FlightDeckWindow::ConfigItemGyro::ConfigItemGyro(Sensors::Sensor& sens, const Sensors::Sensor::ParamDesc& pd)
	: ConfigItem(sens, pd), m_vbox(true, 2), m_afai("", 0.0, 0.0, 1.0, false), m_afhsi("", 0.0, 1.0, 1.0, false), m_ai(), m_hsi()
{
	m_vbox.set_visible(true);
	m_afai.set_visible(true);
	m_afai.unset_label();
	m_vbox.pack_start(m_afai, true, true, 0);
	m_ai.set_visible(true);
	m_afai.add(m_ai);
	m_afhsi.set_visible(true);
	m_afhsi.unset_label();
	m_vbox.pack_start(m_afhsi, true, true, 0);
	m_hsi.set_visible(true);
	m_afhsi.add(m_hsi);
}

void FlightDeckWindow::ConfigItemGyro::update(const Sensors::Sensor::ParamChanged& pc)
{
	double pitch, bank, slip, hdg, rate;
	Sensors::Sensor::paramfail_t pf(get_param(pitch, bank, slip, hdg, rate));
	if (pf != Sensors::Sensor::paramfail_ok)
		pitch = bank = slip = hdg = rate = std::numeric_limits<double>::quiet_NaN();
	m_ai.set_pitch_bank_slip(pitch, bank, slip);
	m_hsi.set_heading_rate(hdg, rate);
}

class FlightDeckWindow::ConfigItemSatellites : public FlightDeckWindow::ConfigItem {
public:
	ConfigItemSatellites(Sensors::Sensor& sens, const Sensors::Sensor::ParamDesc& pd);
	virtual void update(const Sensors::Sensor::ParamChanged& pc);
	virtual Gtk::Widget *get_widget(void) { return &m_hbox; }
	virtual bool is_fastupdate(void) const { return true; }

protected:
	GPSContainer m_hbox;
	GPSAzimuthElevation m_azel;
	GPSSignalStrength m_ss;
};

FlightDeckWindow::ConfigItemSatellites::ConfigItemSatellites(Sensors::Sensor& sens, const Sensors::Sensor::ParamDesc& pd)
	: ConfigItem(sens, pd)
{
	m_azel.set_visible(true);
	m_ss.set_visible(true);
	m_azel.set_paramfail(Sensors::Sensor::paramfail_fail);
	m_ss.set_paramfail(Sensors::Sensor::paramfail_fail);
	m_hbox.set_visible(true);
	m_hbox.add(m_azel);
	m_hbox.add(m_ss);	
}

void FlightDeckWindow::ConfigItemSatellites::update(const Sensors::Sensor::ParamChanged& pc)
{
	FlightDeckWindow::ConfigItem::update(pc);
	if (!pc.is_changed(get_paramnr()))
		return;
	Sensors::Sensor::satellites_t sat;
	Sensors::Sensor::paramfail_t pf(get_param(sat));
	m_azel.update(sat);
	m_ss.update(sat);
	m_azel.set_paramfail(pf);
	m_ss.set_paramfail(pf);
}

class FlightDeckWindow::ConfigItemMagCalib : public FlightDeckWindow::ConfigItem {
public:
	ConfigItemMagCalib(Sensors::Sensor& sens, const Sensors::Sensor::ParamDesc& pd, MagCalibDialog *magcaldlg);
	virtual void update(const Sensors::Sensor::ParamChanged& pc);
	virtual void set_admin(bool admin);
	virtual Gtk::Widget *get_widget(void) { return m_button; }
	virtual bool is_fastupdate(void) const { return true; }

protected:
	Fail<Gtk::Button> *m_button;
	MagCalibDialog *m_magcalibdialog;
	sigc::connection m_resultconn;

	void update(void);
	void changed(void);
	void result(const Eigen::Vector3d& magbias, const Eigen::Vector3d& magscale);
};

FlightDeckWindow::ConfigItemMagCalib::ConfigItemMagCalib(Sensors::Sensor& sens, const Sensors::Sensor::ParamDesc& pd, MagCalibDialog *magcaldlg)
	: ConfigItem(sens, pd), m_button(0), m_magcalibdialog(magcaldlg)
{
	m_button = Gtk::manage(new Fail<Gtk::Button>());
	if (pd.get_choices().empty() || pd.get_choices().front().empty()) {
		m_button->set_label("Calibrate Magnetometer");
	} else {
		bool stock(pd.get_choices().front()[0] == '$');
		if (stock) {
			Gtk::StockItem sitem;
			stock = Gtk::StockItem::lookup(Gtk::StockID(pd.get_choices().front().substr(1)), sitem);
			if (stock) {
				Gtk::Image *img = Gtk::manage(new Gtk::Image(sitem.get_stock_id(), Gtk::IconSize(Gtk::ICON_SIZE_SMALL_TOOLBAR /* Gtk::ICON_SIZE_BUTTON */)));
				m_button->set_image(*img);
			}
		}
		if (!stock)
			m_button->set_label(pd.get_choices().front());
	}
	m_button->set_visible(true);
	if (!is_readonly())
		m_button->signal_clicked().connect(sigc::mem_fun(*this, &ConfigItemMagCalib::changed));
	m_button->set_sensitive(m_magcalibdialog && is_writable());
	if (m_magcalibdialog)
		m_resultconn = m_magcalibdialog->signal_result().connect(sigc::mem_fun(*this, &ConfigItemMagCalib::result));
}

void FlightDeckWindow::ConfigItemMagCalib::update(void)
{
	Sensors::Sensor::paramfail_t pf(Sensors::Sensor::paramfail_ok);
	Eigen::Vector3d mag(Eigen::Vector3d::Constant(std::numeric_limits<double>::quiet_NaN()));
	for (unsigned int i = 0; i < 3; ++i) {
		double v;
		if (get_param(i, v) == Sensors::Sensor::paramfail_ok) {
			mag(i) = v;
			continue;
		}
		pf = Sensors::Sensor::paramfail_fail;
		//break;
	}
	m_button->set_paramfail(pf);
	if (true || pf == Sensors::Sensor::paramfail_ok)
		m_magcalibdialog->newrawvalue(mag);
}

void FlightDeckWindow::ConfigItemMagCalib::update(const Sensors::Sensor::ParamChanged& pc)
{
	FlightDeckWindow::ConfigItem::update(pc);
	if (!pc.is_changed(get_paramnr(), get_paramnr() + 2))
		return;
	update();
}

void FlightDeckWindow::ConfigItemMagCalib::set_admin(bool admin)
{
	FlightDeckWindow::ConfigItem::set_admin(admin);
	m_button->set_sensitive(m_magcalibdialog && is_writable());
}

void FlightDeckWindow::ConfigItemMagCalib::changed(void)
{
	m_magcalibdialog->show();
	m_magcalibdialog->set_keep_above(true);
	m_magcalibdialog->raise();
	Eigen::Vector3d magbias(Eigen::Vector3d::Constant(std::numeric_limits<double>::quiet_NaN()));
	Eigen::Vector3d magscale(Eigen::Vector3d::Constant(std::numeric_limits<double>::quiet_NaN()));
	for (unsigned int i = 0; i < 3; ++i) {
		double v(std::numeric_limits<double>::quiet_NaN());
		if (get_param(i + 3, v) == Sensors::Sensor::paramfail_ok)
			magbias(i) = v;
		v = std::numeric_limits<double>::quiet_NaN();
		if (get_param(i + 6, v) == Sensors::Sensor::paramfail_ok)
			magscale(i) = v;
	}
	m_magcalibdialog->init(magbias, magscale);
	m_magcalibdialog->start();
}

void FlightDeckWindow::ConfigItemMagCalib::result(const Eigen::Vector3d& magbias, const Eigen::Vector3d& magscale)
{
	for (unsigned int i = 0; i < 3; ++i) {
		set_param(i + 3, magbias(i));
		set_param(i + 6, magscale(i));
	}
}

class FlightDeckWindow::ConfigItemADSBTargets : public FlightDeckWindow::ConfigItem {
public:
        ConfigItemADSBTargets(Sensors::Sensor& sens, const Sensors::Sensor::ParamDesc& pd);
	virtual void update(const Sensors::Sensor::ParamChanged& pc);
	virtual Gtk::Widget *get_widget(void) { return &m_scrolledwnd; }

protected:
        class ModelColumns : public Gtk::TreeModelColumnRecord {
          public:
                ModelColumns(void);
		Gtk::TreeModelColumn<uint32_t> m_addr;
                Gtk::TreeModelColumn<Glib::ustring> m_text;
	};

	Fail<Gtk::ScrolledWindow> m_scrolledwnd;
        Gtk::TreeView m_entry;
	ModelColumns m_columns;
	Glib::RefPtr<Gtk::TreeStore> m_model;
};

FlightDeckWindow::ConfigItemADSBTargets::ModelColumns::ModelColumns(void)
{
	add(m_addr);
	add(m_text);
}

FlightDeckWindow::ConfigItemADSBTargets::ConfigItemADSBTargets(Sensors::Sensor& sens, const Sensors::Sensor::ParamDesc& pd)
	: ConfigItem(sens, pd)
{
	m_scrolledwnd.set_visible(true);
	m_scrolledwnd.set_sensitive(true);
	m_scrolledwnd.set_policy(Gtk::POLICY_AUTOMATIC, Gtk::POLICY_AUTOMATIC);
	m_scrolledwnd.add(m_entry);
	m_model = Gtk::TreeStore::create(m_columns);
	m_entry.set_visible(true);
	m_entry.set_can_focus(!is_readonly());
	m_entry.set_sensitive(true);
	m_entry.set_model(m_model);
	Gtk::CellRendererText *text_renderer = Gtk::manage(new Gtk::CellRendererText());
	int text_col = m_entry.append_column(_("ADS-B Targets"), *text_renderer) - 1;
	Gtk::TreeViewColumn *text_column = m_entry.get_column(text_col);
	if (text_column) {
		text_column->add_attribute(*text_renderer, "text", m_columns.m_text);
	}
	text_column->set_resizable(true);
	text_column->set_sort_column(m_columns.m_text);
	text_column->set_expand(true);
	m_entry.columns_autosize();
	Glib::RefPtr<Gtk::TreeSelection> selection = m_entry.get_selection();
	selection->set_mode(Gtk::SELECTION_NONE);
}

void FlightDeckWindow::ConfigItemADSBTargets::update(const Sensors::Sensor::ParamChanged& pc)
{
	FlightDeckWindow::ConfigItem::update(pc);
	if (!pc.is_changed(get_paramnr()))
		return;
	Sensors::Sensor::adsbtargets_t targets;
	m_scrolledwnd.set_paramfail(get_param(targets));
	//m_entry.set_text(v);
	Glib::RefPtr<Gtk::TreeStore> model(Gtk::TreeStore::create(m_columns));
	for (Sensors::Sensor::adsbtargets_t::const_iterator ti(targets.begin()), te(targets.end()); ti != te; ++ti) {
		Gtk::TreeIter it(model->append());
		Gtk::TreeModel::Row row(*it);
		row[m_columns.m_addr] = ti->get_icaoaddr();
		{
			std::ostringstream oss;
			oss << std::hex << std::uppercase << std::setfill('0') << std::setw(6) << ti->get_icaoaddr();
			if (ti->is_ownship())
				oss << '*';
			{
				std::string r(ModeSMessage::decode_registration(ti->get_icaoaddr()));
				if (!r.empty())
					oss << ' ' << r;
			}
			if (!ti->get_ident().empty())
				oss << ' ' << ti->get_ident();
			if (ti->is_modea_valid())
				oss << " A" << (char)('0' + ((ti->get_modea() >> 9) & 7)) << (char)('0' + ((ti->get_modea() >> 6) & 7))
				    << (char)('0' + ((ti->get_modea() >> 3) & 7)) << (char)('0' + (ti->get_modea() & 7));
			row[m_columns.m_text] = oss.str();
		}
		if (ti->get_timestamp().valid()) {
			Gtk::TreeIter itc(model->append(row.children()));
			Gtk::TreeModel::Row rowc(*itc);
			rowc[m_columns.m_addr] = ti->get_icaoaddr();
			Glib::DateTime dt(Glib::DateTime::create_now_utc(ti->get_timestamp()));
			rowc[m_columns.m_text] = dt.format("%Y-%m-%d %H:%M:%S");
		}
		if (false && !ti->empty()) {
			Gtk::TreeIter itc(model->append(row.children()));
			Gtk::TreeModel::Row rowc(*itc);
			rowc[m_columns.m_addr] = ti->get_icaoaddr();
			std::ostringstream oss;
			if (ti->back().get_coord().is_invalid()) {
				oss << "CPR lat " << ti->back().get_cprlat() << " lon " << ti->back().get_cprlon()
				    << " fmt " << (char)('0' + ti->back().get_cprformat())
				    << " T " << (char)('0' + ti->back().get_timebit());
			} else {
				oss << ti->back().get_coord().get_lat_str() + " " + ti->back().get_coord().get_lon_str();
			}
			rowc[m_columns.m_text] = oss.str();
		}
		if (true) {
			unsigned int sz(ti->size());
			for (unsigned int i = ti->size(), nr = 4; i > 0 && nr > 0; --nr) {
				--i;
				const Sensors::Sensor::ADSBTarget::Position& pos(ti->operator[](i));
				Gtk::TreeIter itc(model->append(row.children()));
				Gtk::TreeModel::Row rowc(*itc);
				rowc[m_columns.m_addr] = ti->get_icaoaddr();
				std::ostringstream oss;
				bool pvalid(!pos.get_coord().is_invalid());
				if (pvalid)
					oss << pos.get_coord().get_lat_str() + " " + pos.get_coord().get_lon_str();
				if (true || !pvalid) {
					if (pvalid)
						oss << ' ';
					Glib::DateTime dt(Glib::DateTime::create_now_utc(pos.get_timestamp()));
					oss << "CPR lat " << pos.get_cprlat() << " lon " << pos.get_cprlon()
					    << " fmt " << (char)('0' + pos.get_cprformat())
					    << " T " << (char)('0' + pos.get_timebit())
					    << ' ' << dt.format("%Y-%m-%d %H:%M:%S");
				}
				rowc[m_columns.m_text] = oss.str();
			}
		}
		if (ti->get_baroalttimestamp().valid() || ti->get_vmotion() == Sensors::Sensor::ADSBTarget::vmotion_baro) {
			Gtk::TreeIter itc(model->append(row.children()));
			Gtk::TreeModel::Row rowc(*itc);
			rowc[m_columns.m_addr] = ti->get_icaoaddr();
			std::ostringstream oss;
			oss << "Baro";
			if (ti->get_baroalttimestamp().valid() && ti->get_baroalt() != std::numeric_limits<int32_t>::min())
				oss << ' ' << ti->get_baroalt() << '\'';
			if (ti->get_vmotion() == Sensors::Sensor::ADSBTarget::vmotion_baro)
				oss << ' ' << ti->get_verticalspeed() << "ft/min";
			rowc[m_columns.m_text] = oss.str();
		}
	        if (ti->get_gnssalttimestamp().valid() || ti->get_vmotion() == Sensors::Sensor::ADSBTarget::vmotion_gnss) {
			Gtk::TreeIter itc(model->append(row.children()));
			Gtk::TreeModel::Row rowc(*itc);
			rowc[m_columns.m_addr] = ti->get_icaoaddr();
			std::ostringstream oss;
			oss << "GNSS";
			if (ti->get_gnssalttimestamp().valid() && ti->get_gnssalt() != std::numeric_limits<int32_t>::min())
				oss << ' ' << ti->get_gnssalt() << '\'';
			if (ti->get_vmotion() == Sensors::Sensor::ADSBTarget::vmotion_gnss)
				oss << ' ' << ti->get_verticalspeed() << "ft/min";
			rowc[m_columns.m_text] = oss.str();
		}
		if (ti->get_lmotion() != Sensors::Sensor::ADSBTarget::lmotion_invalid) {
			Gtk::TreeIter itc(model->append(row.children()));
			Gtk::TreeModel::Row rowc(*itc);
			rowc[m_columns.m_addr] = ti->get_icaoaddr();
			std::ostringstream oss;
			switch (ti->get_lmotion()) {
			case Sensors::Sensor::ADSBTarget::lmotion_gnd:
				oss << "GND";
				break;

			case Sensors::Sensor::ADSBTarget::lmotion_tas:
				oss << "TAS";
				break;

			case Sensors::Sensor::ADSBTarget::lmotion_ias:
				oss << "IAS";
				break;

			default:
				break;
			}
			oss << std::setw(3) << std::setfill('0') << Point::round<int,double>(ti->get_direction() * (360.0 / 65536)) << ' ' << ti->get_speed() << "kts";
			rowc[m_columns.m_text] = oss.str();
		}
	}
	m_model = model;
	m_entry.set_model(m_model);
	m_entry.expand_all();
}

class FlightDeckWindow::ConfigItemsGroup {
public:
	ConfigItemsGroup(void);
	ConfigItemsGroup(const Sensors::Sensor::ParamDesc& pd);
	ConfigItemsGroup(const ConfigItemsGroup& cig);
	~ConfigItemsGroup();
	ConfigItemsGroup& operator=(const ConfigItemsGroup& cig);
	Gtk::Widget *get_widget(void);
	unsigned int get_rows(void) const { return m_rows; }
	FlightDeckWindow::sensorcfgitem_t add(Sensors::Sensor& sens, const Sensors::Sensor::ParamDesc& pd, MagCalibDialog *magcal);
	void take_ownership(void) { m_ownership = false; }

protected:
	Gtk::Frame *m_frame;
	Gtk::Table *m_table;
	unsigned int m_rows;
	mutable bool m_ownership;
};

FlightDeckWindow::ConfigItemsGroup::ConfigItemsGroup(void)
	: m_rows(0), m_ownership(true)
{
	m_table = Gtk::manage(new Gtk::Table(1, 3, false));
	m_table->set_visible(true);
}

FlightDeckWindow::ConfigItemsGroup::ConfigItemsGroup(const Sensors::Sensor::ParamDesc& pd)
	: m_rows(0)
{
	m_table = Gtk::manage(new Gtk::Table(1, 3, false));
	m_table->set_visible(true);
	m_frame = Gtk::manage(new Gtk::Frame(pd.get_label()));
	m_frame->add(*m_table);
	m_frame->set_shadow_type(Gtk::SHADOW_OUT);
	m_frame->set_visible(true);
}

FlightDeckWindow::ConfigItemsGroup::~ConfigItemsGroup()
{
	if (m_ownership)
		delete get_widget();
}

FlightDeckWindow::ConfigItemsGroup::ConfigItemsGroup(const ConfigItemsGroup& cig)
{
	*this = cig;
}

FlightDeckWindow::ConfigItemsGroup& FlightDeckWindow::ConfigItemsGroup::operator=(const ConfigItemsGroup& cig)
{
	m_frame = cig.m_frame;
	m_table = cig.m_table;
	m_rows = cig.m_rows;
	m_ownership = m_ownership;
	cig.m_ownership = false;
	return *this;
}

Gtk::Widget *FlightDeckWindow::ConfigItemsGroup::get_widget(void)
{
	if (m_frame)
		return m_frame;
	return m_table;
}

FlightDeckWindow::sensorcfgitem_t FlightDeckWindow::ConfigItemsGroup::add(Sensors::Sensor& sens, const Sensors::Sensor::ParamDesc& pd, MagCalibDialog *magcal)
{
	sensorcfgitem_t r;
	switch (pd.get_type()) {
	case Sensors::Sensor::ParamDesc::type_string:
	{
		r = sensorcfgitem_t(new ConfigItemString(sens, pd));
		break;
	}

	case Sensors::Sensor::ParamDesc::type_switch:
	{
		r = sensorcfgitem_t(new ConfigItemSwitch(sens, pd));
		break;
	}

	case Sensors::Sensor::ParamDesc::type_integer:
	{
		if (true && pd.get_editable() == Sensors::Sensor::ParamDesc::editable_readonly)
			r = sensorcfgitem_t(new ConfigItemNumeric::ConfigItemReadOnlyInteger(sens, pd));
		else
			r = sensorcfgitem_t(new ConfigItemNumeric::ConfigItemInteger(sens, pd));
		break;
	}

	case Sensors::Sensor::ParamDesc::type_double:
	{
		if (true && pd.get_editable() == Sensors::Sensor::ParamDesc::editable_readonly)
			r = sensorcfgitem_t(new ConfigItemNumeric::ConfigItemReadOnlyDouble(sens, pd));
		else
			r = sensorcfgitem_t(new ConfigItemNumeric::ConfigItemDouble(sens, pd));
		break;
	}

	case Sensors::Sensor::ParamDesc::type_choice:
	{
		r = sensorcfgitem_t(new ConfigItemChoice(sens, pd));
		break;
	}

	case Sensors::Sensor::ParamDesc::type_button:
	{
		r = sensorcfgitem_t(new ConfigItemButton(sens, pd));
		break;
	}

	case Sensors::Sensor::ParamDesc::type_magcalib:
	{
		r = sensorcfgitem_t(new ConfigItemMagCalib(sens, pd, magcal));
		break;
	}

	default:
		throw std::runtime_error("FlightDeckWindow::ConfigItemsGroup::add: invalid type");
	}
	if (!r)
		return r;
	Gtk::Widget *widget(r->get_widget());
	if (!widget)
		return r;
	widget->set_visible(true);
	++m_rows;
	m_table->resize(m_rows, 3);
	Gtk::Label *lbl = Gtk::manage(new Gtk::Label(pd.get_label(), 0.0, 0.5));
	lbl->set_visible(true);
	lbl->set_use_markup(true);
	lbl->set_markup(pd.get_label());
	Gtk::Label *unit(0);
	if (!pd.get_unit().empty()) {
		unit = Gtk::manage(new Gtk::Label(pd.get_unit(), 0.0, 0.5));
		unit->set_visible(true);
		unit->set_use_markup(true);
		unit->set_markup(pd.get_unit());
	}
	if (!pd.get_description().empty())
		widget->set_tooltip_markup(pd.get_description());
	m_table->attach(*lbl, 0, 1, m_rows - 1, m_rows, Gtk::FILL, Gtk::FILL, 0, 0);
	if (unit) {
		m_table->attach(*widget, 1, 2, m_rows - 1, m_rows, Gtk::FILL | Gtk::EXPAND, Gtk::FILL, 0, 0);
		m_table->attach(*unit, 2, 3, m_rows - 1, m_rows, Gtk::FILL, Gtk::FILL, 0, 0);
	} else {
		m_table->attach(*widget, 1, 3, m_rows - 1, m_rows, Gtk::FILL | Gtk::EXPAND, Gtk::FILL, 0, 0);
	}
	return r;
}

class FlightDeckWindow::AllConfigItems {
public:
	AllConfigItems(void);
	AllConfigItems(Sensors::Sensor& sens, unsigned int pagenr, MagCalibDialog *magcal);
	void init(Sensors::Sensor& sens, unsigned int pagenr, MagCalibDialog *magcal);
	void clear_empty(void);
	Gtk::Widget *build_widget(sensorcfgitems_t& cfgitems);

protected:
	typedef std::vector<ConfigItemsGroup> cfggroups_t;
	cfggroups_t m_cfggroups;
	sensorcfgitems_t m_cfgitems;
	sensorcfgitem_t m_cfggyro;
	sensorcfgitem_t m_cfgsat;
	sensorcfgitem_t m_cfgadsb;

	class ColumnCounter {
	public:
		ColumnCounter(unsigned int nrcol, unsigned int nritems);
		void reset(void);
		bool inc(void);
		unsigned int get_cols(void) const { return m_nrcol; }
		unsigned int get_items(void) const { return m_nritems; }
		unsigned int operator[](unsigned int col) const;

	protected:
		static const unsigned int maxcols = 4;
		unsigned int m_nrcol;
		unsigned int m_nritems;
		std::vector<unsigned int> m_colbreak;
	};

	unsigned int metric(const ColumnCounter& cnt) const;
};

FlightDeckWindow::AllConfigItems::ColumnCounter::ColumnCounter(unsigned int nrcol, unsigned int nritems)
	: m_nrcol(std::min(nrcol, nritems)), m_nritems(nritems)
{
	reset();
}

void FlightDeckWindow::AllConfigItems::ColumnCounter::reset(void)
{
	m_colbreak.resize(std::max(m_nrcol, 1U) - 1U);
	for (unsigned int i = 0; i < m_colbreak.size(); ++i)
		m_colbreak[i] = i;
}

bool FlightDeckWindow::AllConfigItems::ColumnCounter::inc(void)
{
	int col = m_nrcol - 1;
	for (;;) {
		--col;
		if (col < 0)
			break;
		if (m_colbreak[col] < m_nritems - m_nrcol + col + 1)
			break;
	}
	if (col < 0)
		return false;
	++m_colbreak[col];
	for (++col; (unsigned int)col < m_nrcol - 1; ++col)
		m_colbreak[col] = m_colbreak[col - 1] + 1;
	if (true && m_colbreak.back() >= m_nritems) {
		std::cerr << "FlightDeckWindow::AllConfigItems::ColumnCounter::inc: failure" << std::endl;
		return false;
	}
	return true;
}

unsigned int FlightDeckWindow::AllConfigItems::ColumnCounter::operator[](unsigned int col) const
{
	if (col >= m_colbreak.size())
		return std::numeric_limits<unsigned int>::max();
	return m_colbreak[col];
}

FlightDeckWindow::AllConfigItems::AllConfigItems(void)
{
}

FlightDeckWindow::AllConfigItems::AllConfigItems(Sensors::Sensor& sens, unsigned int pagenr, MagCalibDialog *magcal)
{
	init(sens, pagenr, magcal);
}

void FlightDeckWindow::AllConfigItems::init(Sensors::Sensor& sens, unsigned int pagenr, MagCalibDialog *magcal)
{
	m_cfggroups.clear();
	m_cfgitems.clear();
	m_cfggyro.reset();
	m_cfgsat.reset();
	m_cfgadsb.reset();
	Sensors::Sensor::paramdesc_t pd;
	sens.get_param_desc(pagenr, pd);
	for (Sensors::Sensor::paramdesc_t::const_iterator pdi(pd.begin()), pde(pd.end()); pdi != pde; ++pdi) {
		switch (pdi->get_type()) {
		case Sensors::Sensor::ParamDesc::type_section:
			m_cfggroups.push_back(ConfigItemsGroup(*pdi));
			break;

		case Sensors::Sensor::ParamDesc::type_gyro:
			if (m_cfggyro)
				throw std::runtime_error("FlightDeckWindow::AllConfigItems: gyro can only be used once");
			m_cfggyro = sensorcfgitem_t(new ConfigItemGyro(sens, *pdi));
			break;

		case Sensors::Sensor::ParamDesc::type_satellites:
			if (m_cfgsat)
				throw std::runtime_error("FlightDeckWindow::AllConfigItems: satellites can only be used once");
			m_cfgsat = sensorcfgitem_t(new ConfigItemSatellites(sens, *pdi));
			break;

		case Sensors::Sensor::ParamDesc::type_adsbtargets:
			if (m_cfgadsb)
				throw std::runtime_error("FlightDeckWindow::AllConfigItems: ADS-B targets can only be used once");
			m_cfgadsb = sensorcfgitem_t(new ConfigItemADSBTargets(sens, *pdi));
			break;

		default:
			if (m_cfggroups.empty())
				m_cfggroups.push_back(ConfigItemsGroup());
			sensorcfgitem_t cfgi(m_cfggroups.back().add(sens, *pdi, magcal));
			if (cfgi)
				m_cfgitems.push_back(cfgi);
			break;
		}
	}
	clear_empty();
	if (false) {
		std::cerr << "# config groups: " << m_cfggroups.size() << std::endl
			  << "# config items: " << m_cfgitems.size();
		if (m_cfggyro)
			std::cerr << " + gyro";
		if (m_cfgsat)
			std::cerr << " + satellites";
		if (m_cfgadsb)
			std::cerr << " + ADS-B targets";
		std::cerr << std::endl;
	}
}

void FlightDeckWindow::AllConfigItems::clear_empty(void)
{
	for (unsigned int i = 0; i < m_cfggroups.size(); ) {
		if (m_cfggroups[i].get_rows()) {
			++i;
			continue;
		}
		m_cfggroups.erase(m_cfggroups.begin() + i);
	}
}

unsigned int FlightDeckWindow::AllConfigItems::metric(const ColumnCounter& cnt) const
{
	unsigned int maxcol(std::numeric_limits<unsigned int>::min());
	unsigned int mincol(std::numeric_limits<unsigned int>::max());
	unsigned int colsize(0);
	unsigned int curcol(0);
	static const bool print(false);
	if (print) {
		std::cerr << "AllConfigItems::metric: items " << cnt.get_items() << " cols " << cnt.get_cols() << " breaks";
		for (int i = 0; i < (int)cnt.get_cols() - 1; ++i)
			std::cerr << ' ' << cnt[i];
	}
	for (unsigned int i = 0; i < m_cfggroups.size(); ++i) {
		colsize += m_cfggroups[i].get_rows() + 1; // for frame, etc.
		if (cnt[curcol] != i)
			continue;
		if (print)
			std::cerr << " col " << curcol << " size " << colsize;
		maxcol = std::max(maxcol, colsize);
		mincol = std::min(mincol, colsize);
		colsize = 0;
		++curcol;
	}
	maxcol = std::max(maxcol, colsize);
	mincol = std::min(mincol, colsize);
	if (print)
		std::cerr << " col " << curcol << " size " << colsize << " min " << mincol << " max " << maxcol << std::endl;
	return maxcol - mincol;
}

Gtk::Widget *FlightDeckWindow::AllConfigItems::build_widget(sensorcfgitems_t& cfgitems)
{
	cfgitems.swap(m_cfgitems);
	if (m_cfggyro)
		cfgitems.push_back(m_cfggyro);
	if (m_cfgsat)
		cfgitems.push_back(m_cfgsat);
	if (m_cfgadsb)
		cfgitems.push_back(m_cfgadsb);
	m_cfgitems.clear();
	if (m_cfggroups.empty()) {
		cfgitems.clear();
		m_cfggyro.reset();
		m_cfgsat.reset();
		m_cfgadsb.reset();
		return 0;
	}
	// generate main hbox
	Gtk::HBox *hbox = Gtk::manage(new Gtk::HBox(true, 2));
	hbox->set_visible(true);
	unsigned int hboxcols(0);
	if (m_cfggyro) {
		hbox->pack_start(*m_cfggyro->get_widget(), true, true, 2);
		++hboxcols;
	}
	// Find best way to break columns
	ColumnCounter colcnt(4 - hboxcols, m_cfggroups.size());
	{
		unsigned int m(metric(colcnt));
		ColumnCounter cc1(colcnt);
		while (cc1.inc()) {
			unsigned int m1(metric(cc1));
			if (m1 >= m)
				continue;
			m = m1;
			colcnt = cc1;
		}
	}
	// generate widgets
	{
		Gtk::VBox *vbox(0);
		unsigned int curcol(0);
		for (unsigned int i = 0; i < m_cfggroups.size(); ++i) {
			if (!vbox) {
				if (i == colcnt[curcol] || i + 1 == m_cfggroups.size()) {
					hbox->pack_start(*m_cfggroups[i].get_widget(), false, true, 2);
					++hboxcols;
					m_cfggroups[i].take_ownership();
					++curcol;
					continue;
				}
				vbox = Gtk::manage(new Gtk::VBox(false, 2));
				vbox->set_visible(true);
				hbox->pack_start(*vbox, true, true, 2);
				++hboxcols;
			}
			vbox->pack_start(*m_cfggroups[i].get_widget(), false, true, 2);
			m_cfggroups[i].take_ownership();
			if (colcnt[curcol] != i)
				continue;
			++curcol;
			vbox = 0;
		}
		for (; hboxcols < 4; ++hboxcols) {
			Gtk::VBox *vbox = Gtk::manage(new Gtk::VBox(false, 2));
			vbox->set_visible(true);
			hbox->pack_start(*vbox, true, true, 2);
		}
	}
	if (!m_cfgsat && !m_cfgadsb)
		return hbox;
	Gtk::VBox *vbox(Gtk::manage(new Gtk::VBox(false, 1 + !!m_cfgsat + !!m_cfgadsb)));
	vbox->set_visible(true);
	vbox->pack_start(*hbox, true, true, 2);
	if (m_cfgsat)
		vbox->pack_start(*m_cfgsat->get_widget(), true, true, 2);
	if (m_cfgadsb)
		vbox->pack_start(*m_cfgadsb->get_widget(), true, true, 2);
	return vbox;
}

void FlightDeckWindow::sensorcfg_fini(void)
{
	m_sensorupd.disconnect();
	m_sensordelayedupd.disconnect();
	if (m_sensorcfgframe)
		m_sensorcfgframe->remove();
	m_sensorcfgitems.clear();
	m_sensorlastupd = Glib::TimeVal();
	Sensors::Sensor::ParamChanged m_sensorupdmask;

}

void FlightDeckWindow::sensorcfg_init(Sensors::Sensor& sens, unsigned int pagenr)
{
	sensorcfg_fini();
	if (!m_sensorcfgframe || !m_sensors)
		return;
     	{
		//Gtk::Label *lbl(dynamic_cast<Gtk::Label *>(m_sensorcfgframe->get_label_widget()));
		Gtk::Label *lbl(Gtk::manage(new Gtk::Label()));
		if (lbl) {
			Glib::ustring str;
			if (!sens.get_name().empty())
				str = "<b>" + Glib::Markup::escape_text(sens.get_name()) + "</b>";
			if (!sens.get_description().empty()) {
				if (!str.empty())
					str += ": ";
				str += Glib::Markup::escape_text(sens.get_description());
			}
			lbl->set_use_markup(true);
			lbl->set_label(str);
			lbl->set_width_chars(100);
			lbl->set_max_width_chars(-1);
			lbl->set_single_line_mode(true);
			lbl->set_visible(true);
			m_sensorcfgframe->set_label_widget(*lbl);
		}
		m_sensorcfgframe->set_label_align(0.5, 0.5);
	}
	{
		AllConfigItems cfg(sens, pagenr, m_magcalibdialog);
		m_sensorcfgframe->remove();
		Gtk::Widget *w(cfg.build_widget(m_sensorcfgitems));
		if (w)
			m_sensorcfgframe->add(*w);
	}
	for (sensorcfgitems_t::iterator si(m_sensorcfgitems.begin()), se(m_sensorcfgitems.end()); si != se; ++si) {
		Glib::RefPtr<FlightDeckWindow::ConfigItem> cfgi(*si);
		bool numeric(false);
		Gtk::Widget *widget(cfgi->get_screenkbd_widget(numeric));
		if (!widget)
			continue;
		widget->signal_focus_in_event().connect(sigc::bind(sigc::bind(sigc::mem_fun(*this, &FlightDeckWindow::onscreenkbd_focus_in_event), numeric ? kbd_numeric : kbd_full), widget));
		widget->signal_focus_out_event().connect(sigc::mem_fun(*this, &FlightDeckWindow::onscreenkbd_focus_out_event));
	}
	if (false)
		std::cerr << "# config items: " << m_sensorcfgitems.size() << std::endl;
	m_sensorupd = sens.connect_update_params(sigc::mem_fun(*this, &FlightDeckWindow::sensorcfg_sensor_update));
	m_sensorupdmask.set_all_changed();
	sensorcfg_update(true);
	m_sensorupdmask.clear_all_changed();
	m_sensorlastupd.assign_current_time();
}

void FlightDeckWindow::sensorcfg_update(bool all)
{
	for (sensorcfgitems_t::iterator si(m_sensorcfgitems.begin()), se(m_sensorcfgitems.end()); si != se; ++si) {
		if (!(all || (*si)->is_fastupdate()))
			continue;
		(*si)->update(m_sensorupdmask);
	}
}

void FlightDeckWindow::sensorcfg_sensor_update(const Sensors::Sensor::ParamChanged& pc)
{
	m_sensorupdmask |= pc;
	m_sensordelayedupd.disconnect();
	if (!m_sensorupdmask)
		return;
	bool all(!m_sensorlastupd.valid());
	if (!all) {
		Glib::TimeVal curtime;
		curtime.assign_current_time();
		Glib::TimeVal tv(m_sensorlastupd);
		tv.add_milliseconds(250);
		tv -= curtime;
		all = tv.negative();
		if (all) {
			m_sensorlastupd = curtime;
		} else {
			tv.add_milliseconds(50);
			m_sensordelayedupd = Glib::signal_timeout().connect(sigc::mem_fun(*this, &FlightDeckWindow::sensorcfg_delayed_update), tv.tv_sec * 1000 + tv.tv_usec / 1000);
		}
	}
	sensorcfg_update(all);
	if (all)
		m_sensorupdmask.clear_all_changed();
}

bool FlightDeckWindow::sensorcfg_delayed_update(void)
{
	sensorcfg_update(true);
	m_sensorlastupd.assign_current_time();
	m_sensorupdmask.clear_all_changed();
	return false;
}

void FlightDeckWindow::sensorcfg_set_admin(bool admin)
{
	for (sensorcfgitems_t::iterator si(m_sensorcfgitems.begin()), se(m_sensorcfgitems.end()); si != se; ++si)
		(*si)->set_admin(admin);
}
