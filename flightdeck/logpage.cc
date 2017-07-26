/***************************************************************************
 *   Copyright (C) 2012 by Thomas Sailer                                   *
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

#include <iomanip>
#include <glibmm/datetime.h>

#include "flightdeckwindow.h"

FlightDeckWindow::LogModelColumns::LogModelColumns(void)
{

	add(m_time);
	add(m_loglevel);
	add(m_color);
	add(m_text);
}

const char *FlightDeckWindow::log_level_button[Sensors::loglevel_fatal+1] = {
	"NOTICE", "WARNING", "ERROR", "FATAL"
};

void FlightDeckWindow::log_cell_data_func(Gtk::CellRenderer *cell, const Gtk::TreeModel::iterator& iter, int column)
{
        Gtk::CellRendererText *cellt(dynamic_cast<Gtk::CellRendererText*>(cell));
        if (!cellt)
                return;
        Gtk::TreeModel::Row row(*iter);
	Glib::TimeVal tv;
        row.get_value(column, tv);
	Glib::DateTime dt(Glib::DateTime::create_now_local(tv));
	dt = dt.to_utc();
        cellt->property_text() = dt.format("%T");
}

void FlightDeckWindow::log_add_message(Glib::TimeVal tv, Sensors::loglevel_t lvl, const Glib::ustring& msg)
{
	if (!m_logstore)
		return;
	{
		Gtk::TreeIter iter(m_logstore->append());
		Gtk::TreeModel::Row row(*iter);
		row[m_logcolumns.m_time] = tv;
		row[m_logcolumns.m_loglevel] = lvl;
		row[m_logcolumns.m_text] = msg;
		Gdk::Color col;
		switch (lvl) {
		default:
		case Sensors::loglevel_notice:
			col.set_rgb(0, 0xd800, 0);
			break;

		case Sensors::loglevel_warning:
			col.set_rgb(0xffff, 0xd800, 0);
			break;

		case Sensors::loglevel_error:
			col.set_rgb(0xffff, 0, 0);
			break;

		case Sensors::loglevel_fatal:
			col.set_rgb(0, 0, 0);
			break;

		}
		row[m_logcolumns.m_color] = col;
		if (m_logtreeview)
			m_logtreeview->scroll_to_row(Gtk::TreePath(iter));
	}
	unsigned int sz(m_logstore->children().size());
	if (sz < 1000)
		return;
	sz -= 1000;
	for (; sz > 0; --sz)
		m_logstore->erase(m_logstore->children().begin());
}

bool FlightDeckWindow::log_filter_func(const Gtk::TreeModel::const_iterator& iter)
{
	if (!iter)
                return false;
        Gtk::TreeModel::Row row(*iter);
	return row[m_logcolumns.m_loglevel] >= m_logdisplaylevel;
}
