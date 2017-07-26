//
// C++ Implementation: Softkeys Widget
//
// Description: Softkeys Widget
//
//
// Author: Thomas Sailer <t.sailer@alumni.ethz.ch>, (C) 2011, 2012
//
// Copyright: See COPYING file that comes with this distribution
//
//

#include "sysdeps.h"

#include <glibmm.h>

#include "flightdeckwindow.h"

const unsigned int FlightDeckWindow::Softkeys::nrbuttons;
const unsigned int FlightDeckWindow::Softkeys::nrgroups;
const unsigned int FlightDeckWindow::Softkeys::groupgap;

FlightDeckWindow::Softkeys::Softkeys(BaseObjectType *castitem, const Glib::RefPtr<Builder>& refxml)
	: Gtk::Container(castitem)
{
	set_has_window(false);
	set_redraw_on_allocate(false);
	for (unsigned int i = 0; i < nrbuttons; ++i) {
		m_button[i].set_parent(*this);
		m_button[i].set_visible(true);
		m_conn[i] = m_button[i].signal_clicked().connect(sigc::bind(sigc::mem_fun(*this, &Softkeys::clicked), i));
	}
}

FlightDeckWindow::Softkeys::Softkeys()
	: Gtk::Container()
{
	set_has_window(false);
	set_redraw_on_allocate(false);
	for (unsigned int i = 0; i < nrbuttons; ++i) {
		m_button[i].set_parent(*this);
		m_button[i].set_visible(true);
		m_conn[i] = m_button[i].signal_clicked().connect(sigc::bind(sigc::mem_fun(*this, &Softkeys::clicked), i));
	}
}

FlightDeckWindow::Softkeys::~Softkeys()
{
}

#ifdef HAVE_GTKMM2
void FlightDeckWindow::Softkeys::on_size_request(Gtk::Requisition *requisition)
{
	if (!requisition || !nrbuttons)
		return;
	*requisition = m_button[0].size_request();
	for (unsigned int i = 1; i < nrbuttons; ++i) {
		Gtk::Requisition req(m_button[i].size_request());
		requisition->width = std::max(requisition->width, req.width);
		requisition->height = std::max(requisition->height, req.height);
	}
	requisition->width *= nrbuttons;
	requisition->width += (nrgroups - 1) * groupgap;
}
#endif

#ifdef HAVE_GTKMM3
Gtk::SizeRequestMode FlightDeckWindow::Softkeys::get_request_mode_vfunc() const
{
	return Gtk::Container::get_request_mode_vfunc();
}

void FlightDeckWindow::Softkeys::get_preferred_width_vfunc(int& minimum_width, int& natural_width) const
{
	int mwidth(0), nwidth(0);
	for (unsigned int i = 0; i < nrbuttons; ++i) {
		int mw(0), nw(0);
		m_button[i].get_preferred_width(mw, nw);
		mwidth = std::max(mwidth, mw);
		nwidth = std::max(nwidth, nw);
	}
	mwidth *= nrbuttons;
	nwidth *= nrbuttons;
	//mwidth = std::min(mwidth, 1024);
	minimum_width = nrbuttons;
	//nwidth = std::min(nwidth, 1024);
	natural_width = nrbuttons;
	if (false)
		std::cerr << "Softkeys::get_preferred_width_vfunc mwidth " << mwidth << " nwidth " << nwidth << std::endl;
}

void FlightDeckWindow::Softkeys::get_preferred_height_for_width_vfunc(int width, int& minimum_height, int& natural_height) const
{
	width /= nrbuttons;
	int mheight(0), nheight(0);
	for (unsigned int i = 0; i < nrbuttons; ++i) {
		int mh(0), nh(0);
		m_button[i].get_preferred_height_for_width(width, mh, nh);
		mheight = std::max(mheight, mh);
		nheight = std::max(nheight, nh);
	}
	minimum_height = mheight;
	natural_height = nheight;
	if (false)
		std::cerr << "Softkeys::get_preferred_height_for_width_vfunc width " << width << " mheight " << mheight << " nheight " << nheight << std::endl;
}

void FlightDeckWindow::Softkeys::get_preferred_height_vfunc(int& minimum_height, int& natural_height) const
{
	int mheight(0), nheight(0);
	for (unsigned int i = 0; i < nrbuttons; ++i) {
		int mh(0), nh(0);
		m_button[i].get_preferred_height(mh, nh);
		mheight = std::max(mheight, mh);
		nheight = std::max(nheight, nh);
	}
	minimum_height = mheight;
	natural_height = nheight;
	if (false)
		std::cerr << "Softkeys::get_preferred_height_vfunc mheight " << mheight << " nheight " << nheight << std::endl;
}

void FlightDeckWindow::Softkeys::get_preferred_width_for_height_vfunc(int height, int& minimum_width, int& natural_width) const
{
	int mwidth(0), nwidth(0);
	for (unsigned int i = 0; i < nrbuttons; ++i) {
		int mw(0), nw(0);
		m_button[i].get_preferred_width_for_height(height, mw, nw);
		mwidth = std::max(mwidth, mw);
		nwidth = std::max(nwidth, nw);
	}
	mwidth *= nrbuttons;
	nwidth *= nrbuttons;
	//mwidth = std::min(mwidth, 1024);
	minimum_width = nrbuttons;
	//nwidth = std::min(nwidth, 1024);
	natural_width = nrbuttons;
	if (false)
		std::cerr << "Softkeys::get_preferred_width_for_height_vfunc height " << height << " mwidth " << mwidth << " nwidth " << nwidth << std::endl;
}
#endif

void FlightDeckWindow::Softkeys::on_size_allocate(Gtk::Allocation& allocation)
{
	set_allocation(allocation);
	const unsigned int groupmemb(std::max((nrbuttons + nrgroups - 1) / nrgroups, 1U));
	unsigned int width(allocation.get_width());
	if (width > (nrgroups - 1) * groupgap)
		width -= (nrgroups - 1) * groupgap;
	const unsigned int buttonwidth(width / nrbuttons);
	width = allocation.get_width() - buttonwidth * nrbuttons;
	unsigned int posadd[nrgroups];
	if (nrgroups >= 1)
		posadd[0] = 0;
	if (nrgroups >= 2) {
		posadd[1] = width / std::max(nrgroups - 1U, 1U);
		width -= posadd[1] * std::max(nrgroups - 1U, 1U);
	}
	for (unsigned int i = 2; i < nrgroups; ++i)
		posadd[i] = posadd[i - 1];
	for (unsigned int i = 1; i < nrgroups && width > 0; ++i) {
		++posadd[i];
		--width;
	}
	for (unsigned int i = 1; i < nrgroups; ++i)
		posadd[i] += posadd[i - 1];
	if (false) {
		for (unsigned int i = 0; i < nrgroups; ++i)
			std::cerr << "Group " << i << " offs " << posadd[i] << std::endl;
	}
	Gtk::Allocation a(allocation);
	a.set_width(buttonwidth);
	width = a.get_x();
	for (unsigned int i = 0; i < nrbuttons; ++i, width += buttonwidth) {
		a.set_x(width + posadd[i / groupmemb]);
		m_button[i].size_allocate(a);
		if (false) {
			std::cerr << "Softkey " << i << " X " << a.get_x() << " (" << width << ", " << (i / groupmemb) << ") "
				  << " Y " << a.get_y() << " W " << a.get_width() << " H " << a.get_height() << std::endl;
		}
	}
}

void FlightDeckWindow::Softkeys::on_add(Widget* widget)
{
	if (!widget)
		return;
	throw std::runtime_error("FlightDeckWindow::Softkeys::on_add: cannot add widgets");
}

void FlightDeckWindow::Softkeys::on_remove(Widget* widget)
{
	if (!widget)
		return;
	throw std::runtime_error("FlightDeckWindow::Softkeys::on_remove: cannot remove widgets");
}

GType FlightDeckWindow::Softkeys::child_type_vfunc() const
{
	return G_TYPE_NONE;
}

void FlightDeckWindow::Softkeys::forall_vfunc(gboolean include_internals, GtkCallback callback, gpointer callback_data)
{
	for (unsigned int i = 0; i < nrbuttons; ++i)
		callback(static_cast<Gtk::Widget&>(m_button[i]).gobj(), callback_data);
}

bool FlightDeckWindow::Softkeys::blocked(unsigned int nr) const
{
	return m_conn[std::min(nr, nrbuttons - 1)].blocked();
}

bool FlightDeckWindow::Softkeys::block(unsigned int nr, bool should_block)
{
	return m_conn[std::min(nr, nrbuttons - 1)].block(should_block);
}

bool FlightDeckWindow::Softkeys::unblock(unsigned int nr)
{
	return m_conn[std::min(nr, nrbuttons - 1)].unblock();
}
