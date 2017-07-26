/***************************************************************************
 *   Copyright (C) 2007, 2008, 2009, 2011, 2012, 2013, 2014, 2015, 2016    *
 *       by Thomas Sailer t.sailer@alumni.ethz.ch                          *
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

#include "flightdeckwindow.h"

FlightDeckWindow::PerfUnitsModelColumns::PerfUnitsModelColumns(void)
{
	add(m_text);
	add(m_gain);
	add(m_offset);
}

FlightDeckWindow::PerfWBModelColumns::PerfWBModelColumns(void)
{
	add(m_text);
	add(m_value);
	add(m_valueeditable);
	add(m_unit);
	add(m_model);
	add(m_modelvisible);
	add(m_mass);
	add(m_minmass);
	add(m_maxmass);
	add(m_arm);
	add(m_moment);
	add(m_style);
	add(m_nr);
}

FlightDeckWindow::PerfResultModelColumns::PerfResultModelColumns(void)
{
	add(m_text);
	add(m_value1);
	add(m_value2);
}

FlightDeckWindow::PerfWBDrawingArea::PerfWBDrawingArea(BaseObjectType *castitem, const Glib::RefPtr<Builder>& refxml)
	: Gtk::DrawingArea(castitem), m_wb(0)
{
#ifndef GLIBMM_DEFAULT_SIGNAL_HANDLERS_ENABLED
#ifdef HAVE_GTKMM2
        signal_expose_event().connect(sigc::mem_fun(*this, &PerfWBDrawingArea::on_expose_event));
#endif
#ifdef HAVE_GTKMM3
        signal_draw().connect(sigc::mem_fun(*this, &PerfWBDrawingArea::on_draw));
#endif
#endif //GLIBMM_DEFAULT_SIGNAL_HANDLERS_ENABLED
}

FlightDeckWindow::PerfWBDrawingArea::PerfWBDrawingArea(void)
	: m_wb(0)
{
#ifndef GLIBMM_DEFAULT_SIGNAL_HANDLERS_ENABLED
#ifdef HAVE_GTKMM2
        signal_expose_event().connect(sigc::mem_fun(*this, &PerfWBDrawingArea::on_expose_event));
#endif
#ifdef HAVE_GTKMM3
        signal_draw().connect(sigc::mem_fun(*this, &PerfWBDrawingArea::on_draw));
#endif
#endif //GLIBMM_DEFAULT_SIGNAL_HANDLERS_ENABLED
}

FlightDeckWindow::PerfWBDrawingArea::~PerfWBDrawingArea()
{
}

void FlightDeckWindow::PerfWBDrawingArea::set_wb(const Aircraft::WeightBalance *wb)
{
	m_wb = wb;
        if (get_is_drawable())
                queue_draw();
}

void FlightDeckWindow::PerfWBDrawingArea::clear_curves(void)
{
	bool chg(!m_curves.empty());
	m_curves.clear();
	if (chg && get_is_drawable())
                queue_draw();
}

void FlightDeckWindow::PerfWBDrawingArea::add_curve(const Glib::ustring& name, const std::vector<Aircraft::WeightBalance::Envelope::Point>& path)
{
	m_curves.push_back(curve_t(name, path));
	if (get_is_drawable())
                queue_draw();
}

void FlightDeckWindow::PerfWBDrawingArea::add_curve(const Glib::ustring& name, const Aircraft::WeightBalance::Envelope::Point& p0,
						    const Aircraft::WeightBalance::Envelope::Point& p1)
{
	std::vector<Aircraft::WeightBalance::Envelope::Point> p;
	p.push_back(p0);
	p.push_back(p1);
	add_curve(name, p);
}

void FlightDeckWindow::PerfWBDrawingArea::add_curve(const Glib::ustring& name, double arm0, double mass0, double arm1, double mass1)
{
	add_curve(name, Aircraft::WeightBalance::Envelope::Point(arm0, mass0), Aircraft::WeightBalance::Envelope::Point(arm1, mass1));
}

#ifdef HAVE_GTKMM2
bool FlightDeckWindow::PerfWBDrawingArea::on_expose_event(GdkEventExpose *event)
{
        Glib::RefPtr<Gdk::Window> wnd(get_window());
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
        return on_draw(cr);
}
#endif

bool FlightDeckWindow::PerfWBDrawingArea::on_draw(const Cairo::RefPtr<Cairo::Context>& cr)
{
	cr->save();
	// find scaling
	double minarm(std::numeric_limits<double>::max());
	double maxarm(std::numeric_limits<double>::min());
	double minmass(minarm);
	double maxmass(maxarm);
	for (curves_t::const_iterator ci(m_curves.begin()), ce(m_curves.end()); ci != ce; ++ci) {
		for (path_t::const_iterator pi(ci->second.begin()), pe(ci->second.end()); pi != pe; ++pi) {
			const Aircraft::WeightBalance::Envelope::Point& p(*pi);
			minarm = std::min(minarm, p.get_arm());
			maxarm = std::max(maxarm, p.get_arm());
			minmass = std::min(minmass, p.get_mass());
			maxmass = std::max(maxmass, p.get_mass());
		}
	}
	if (m_wb) {
		for (unsigned int i = 0; i < m_wb->get_nrenvelopes(); ++i) {
			const Aircraft::WeightBalance::Envelope& ev(m_wb->get_envelope(i));
			double mina, maxa, minm, maxm;
			ev.get_bounds(mina, maxa, minm, maxm);
			minarm = std::min(minarm, mina);
			maxarm = std::max(maxarm, maxa);
			minmass = std::min(minmass, minm);
			maxmass = std::max(maxmass, maxm);
		}
	}
	// oversize
	{
		double o(0.1 * (maxarm - minarm));
		minarm -= o;
		maxarm += o;
	}
	{
		double o(0.1 * (maxmass - minmass));
		minmass -= o;
		maxmass += o;
	}
	if (minarm > maxarm)
		minarm = maxarm = 0;
	if (minmass > maxmass)
		minmass = maxmass = 0;
	Gtk::Allocation allocation = get_allocation();
	const int width = allocation.get_width();
	const int height = allocation.get_height();
	double scalearm(width);
	double scalemass(-height);
	if (maxarm > minarm)
		scalearm /= maxarm - minarm;
	if (maxmass > minmass)
		scalemass /= maxmass - minmass;
	// background
	set_color_background(cr);
	cr->paint();
	// envelopes
	if (m_wb) {
		cr->set_line_width(1);
		for (unsigned int i = 0; i < m_wb->get_nrenvelopes(); ++i) {
			const Aircraft::WeightBalance::Envelope& ev(m_wb->get_envelope(i));
			for (unsigned int j = 0; j < ev.size(); ++j) {
				const Aircraft::WeightBalance::Envelope::Point& p(ev[j]);
				if (!j)
					cr->move_to((p.get_arm() - minarm) * scalearm, (p.get_mass() - maxmass) * scalemass);
				else
					cr->line_to((p.get_arm() - minarm) * scalearm, (p.get_mass() - maxmass) * scalemass);
			}
			cr->close_path();
			set_color_envelope(cr, i, true);
			cr->fill_preserve();
			set_color_envelope(cr, i, false);
			cr->stroke();
		}
	}
	cr->set_line_width(2);
	for (unsigned int i = 0; i < m_curves.size(); ++i) {
		set_color_curve(cr, i);
		{
			bool subseq(false);
			for (path_t::const_iterator pi(m_curves[i].second.begin()), pe(m_curves[i].second.end()); pi != pe; ++pi) {
				const Aircraft::WeightBalance::Envelope::Point& p(*pi);
				if (subseq)
					cr->line_to((p.get_arm() - minarm) * scalearm, (p.get_mass() - maxmass) * scalemass);
				else
					cr->move_to((p.get_arm() - minarm) * scalearm, (p.get_mass() - maxmass) * scalemass);
				subseq = true;
			}
		}
		cr->stroke();
		for (path_t::const_iterator pi(m_curves[i].second.begin()), pe(m_curves[i].second.end()); pi != pe; ++pi) {
			const Aircraft::WeightBalance::Envelope::Point& p(*pi);
			cr->begin_new_sub_path();
			cr->arc((p.get_arm() - minarm) * scalearm, (p.get_mass() - maxmass) * scalemass, 3.0, 0.0, 2.0 * M_PI);
			cr->close_path();
		}
		cr->fill();
	}
	// draw legend
	{
		cr->select_font_face("helvetica", Cairo::FONT_SLANT_NORMAL, Cairo::FONT_WEIGHT_NORMAL);
		cr->set_font_size(12);
		Cairo::TextExtents ext;
		cr->get_text_extents("00", ext);
		const double baselineskip(ext.height * 1.5);
		const double linex(ext.width);
		const double liney(0.5 * ext.height);
		const double textx(linex * 1.1);
		double y(baselineskip);
		cr->set_line_width(1);
		if (m_wb) {
			for (unsigned int i = 0; i < m_wb->get_nrenvelopes(); ++i) {
				const Aircraft::WeightBalance::Envelope& ev(m_wb->get_envelope(i));
				set_color_envelope(cr, i, false);
				cr->move_to(0, y - liney);
				cr->rel_line_to(linex, 0);
				cr->stroke();
				set_color_text(cr);
				cr->move_to(textx, y);
				cr->show_text(ev.get_name());
				y += baselineskip;
			}
		}
		for (unsigned int i = 0; i < m_curves.size(); ++i) {
			set_color_curve(cr, i);
			cr->move_to(0, y - liney);
			cr->rel_line_to(linex, 0);
			cr->stroke();
			set_color_text(cr);
			cr->move_to(textx, y);
			cr->show_text(m_curves[i].first);
			y += baselineskip;
		}		
	}
	cr->restore();
	return true;
}

void FlightDeckWindow::perf_cell_data_func(Gtk::CellRenderer *cell, const Gtk::TreeModel::iterator& iter, int column)
{
	Gtk::CellRendererText *cellt(dynamic_cast<Gtk::CellRendererText*>(cell));
	if (!cellt)
		return;
	Gtk::TreeModel::Row row(*iter);
	double v;
	row.get_value(column, v);
	cellt->property_text() = Glib::ustring::format(std::fixed, std::setprecision(1), v);
}

void FlightDeckWindow::perf_cellrenderer_edited(const Glib::ustring& path_string, const Glib::ustring& new_text, int column)
{
	if (!m_perfwbstore)
		return;
	Gtk::TreePath path(path_string);
	Gtk::TreeModel::iterator iter(m_perfwbstore->get_iter(path));
	if (!iter)
		return;
	Gtk::TreeRow row(*iter);
	if (new_text.empty())
		return;
	row.set_value(column, Glib::Ascii::strtod(new_text));
	if (column != m_perfwbcolumns.m_value.index())
		return;
	if (m_sensors) {
		const Aircraft::WeightBalance& wb(m_sensors->get_aircraft().get_wb());
		unsigned int idx(row[m_perfwbcolumns.m_nr]);
		if (idx < wb.get_nrelements()) {
			const Aircraft::WeightBalance::Element& el(wb.get_element(idx));
			double val(row[m_perfwbcolumns.m_value]);
			for (unsigned int j = 0; j < el.get_nrunits(); ++j) {
				const Aircraft::WeightBalance::Element::Unit& u(el.get_unit(j));
				if (u.get_name() != row[m_perfwbcolumns.m_unit])
					continue;
				double mass(val * u.get_factor() + u.get_offset());
				if (mass < el.get_massmin()) {
					mass = el.get_massmin();
					if (u.is_fixed())
						val = mass;
					else
						val = (mass - u.get_offset()) / u.get_factor();
					row[m_perfwbcolumns.m_value] = val;
				} else if (mass > el.get_massmax()) {
					mass = el.get_massmax();
					if (u.is_fixed())
						val = mass;
					else
						val = (mass - u.get_offset()) / u.get_factor();
				}
				row[m_perfwbcolumns.m_mass] = mass;
				row[m_perfwbcolumns.m_value] = val;
			}
		}
	}
	perfpage_recomputewb();
}

void FlightDeckWindow::perf_cellrendererunit_edited(const Glib::ustring& path_string, const Glib::ustring& new_text)
{
	if (!m_perfwbstore)
		return;
	Gtk::TreePath path(path_string);
	Gtk::TreeModel::iterator iter(m_perfwbstore->get_iter(path));
	if (!iter)
		return;
	Gtk::TreeRow row(*iter);
	row[m_perfwbcolumns.m_unit] = new_text;
	if (m_sensors) {
		const Aircraft::WeightBalance& wb(m_sensors->get_aircraft().get_wb());
		unsigned int idx(row[m_perfwbcolumns.m_nr]);
		if (idx < wb.get_nrelements()) {
			const Aircraft::WeightBalance::Element& el(wb.get_element(idx));
			for (unsigned int j = 0; j < el.get_nrunits(); ++j) {
				const Aircraft::WeightBalance::Element::Unit& u(el.get_unit(j));
				if (u.get_name() != new_text)
					continue;
				if ((el.get_flags() & Aircraft::WeightBalance::Element::flag_fixed) || u.is_fixed()) {
					row[m_perfwbcolumns.m_value] = u.get_offset();
					row[m_perfwbcolumns.m_mass] = u.get_offset();
				} else {
					double mass(row[m_perfwbcolumns.m_mass]);
					row[m_perfwbcolumns.m_value] = (mass - u.get_offset()) / u.get_factor();
				}
				break;
			}
		}
	}
	perfpage_recomputewb();
}

void FlightDeckWindow::perfpage_recomputewb(void)
{
	if (!m_perfwbstore || !m_sensors)
		return;
	const Aircraft::WeightBalance& wb(m_sensors->get_aircraft().get_wb());
	std::vector<Glib::ustring> cfgunit(wb.get_nrelements(), "");
	std::vector<double> cfgmass(wb.get_nrelements(), 0.0);
	double mass(0), moment(0), fuelmass(0), fuelmoment(0), gearmass(0), gearmoment(0);
	double mass1(0), moment1(0);
	std::vector<Glib::ustring> gearnames;
	Gtk::TreeIter totiter;
	for (Gtk::TreeIter iter(m_perfwbstore->children().begin()); iter; ++iter) {
		Gtk::TreeModel::Row row(*iter);
		unsigned int nr(row[m_perfwbcolumns.m_nr]);
		if (nr == ~0U) {
			totiter = iter;
			continue;
		}
		if (nr >= wb.get_nrelements())
			continue;
		cfgmass[nr] = row[m_perfwbcolumns.m_mass];
		cfgunit[nr] = row[m_perfwbcolumns.m_unit];
		const Aircraft::WeightBalance::Element& el(wb.get_element(nr));
		mass1 += cfgmass[nr];
		moment1 += el.get_moment(cfgmass[nr]);
		row[m_perfwbcolumns.m_moment] = el.get_moment(cfgmass[nr]);
		if (el.get_flags() & Aircraft::WeightBalance::Element::flag_fuel) {
			double m(el.get_massmin());
			mass += m;
			moment += el.get_moment(m);
			m = cfgmass[nr] - m;
			fuelmass += m;
			fuelmoment += el.get_moment(m);
		} else if (!((Aircraft::WeightBalance::Element::flag_gear |
			      Aircraft::WeightBalance::Element::flag_fixed) & ~el.get_flags()) &&
			   el.get_nrunits() == 2) {
			const Aircraft::WeightBalance::Element::Unit& u0(el.get_unit(0));
			const Aircraft::WeightBalance::Element::Unit& u1(el.get_unit(1));
			gearnames.clear();
			gearnames.push_back(el.get_name() + " " + u0.get_name());
			gearnames.push_back(el.get_name() + " " + u1.get_name());
			double m(u0.get_offset());
			mass += m;
			moment += el.get_moment(m);
			m = u1.get_offset() - m;
			gearmass += m;
			gearmoment += el.get_moment(m);
		} else {
			mass += cfgmass[nr];
			moment += el.get_moment(cfgmass[nr]);
		} 
	}
	m_sensors->get_configfile().set_string_list("weightbalance", "unit", cfgunit);
	m_sensors->get_configfile().set_double_list("weightbalance", "mass", cfgmass);
	m_sensors->save_config();
	if (totiter) {
		Gtk::TreeModel::Row row(*totiter);
		double a(moment1);
		if (mass1 > 0)
			a /= mass1;
		else
			a = 0;
		row[m_perfwbcolumns.m_moment] = moment1;
		row[m_perfwbcolumns.m_value] = mass1;
		row[m_perfwbcolumns.m_mass] = mass1;
		row[m_perfwbcolumns.m_arm] = a;
	}
	if (m_perfdrawingareawb) {
		m_perfdrawingareawb->clear_curves();
		if (gearnames.size() < 2) {
			double mass0(mass + gearmass);
			double arm0(moment + gearmoment);
			double mass1(mass0 + fuelmass);
			double arm1(arm0 + fuelmoment);
			if (mass0 > 0)
				arm0 /= mass0;
			else
				arm0 = 0;
			if (mass1 > 0)
				arm1 /= mass1;
			else
				arm1 = 0;
			m_perfdrawingareawb->add_curve("CG", arm0, mass0, arm1, mass1);
		} else {
			for (unsigned int i = 0; i < gearnames.size(); ++i) {
				double mass0(mass);
				double arm0(moment);
				if (i) {
					mass0 += gearmass;
					arm0 += gearmoment;
				}
				double mass1(mass0 + fuelmass);
				double arm1(arm0 + fuelmoment);
				if (mass0 > 0)
					arm0 /= mass0;
				else
					arm0 = 0;
				if (mass1 > 0)
					arm1 /= mass1;
				else
					arm1 = 0;
				m_perfdrawingareawb->add_curve(gearnames[i], arm0, mass0, arm1, mass1);
			}
		}			
	}
}

void FlightDeckWindow::perfpage_saveperf(void)
{
	if (!m_sensors)
		return;
	{
		std::vector<Glib::ustring> cfgunit(perfunits_nr);
		for (unsigned int i = 0; i < perfunits_nr; ++i) {
			if (!m_perfcombobox[i])
				continue;
			Gtk::TreeModel::const_iterator iter(m_perfcombobox[i]->get_active());
			if (!iter)
				continue;
			Gtk::TreeModel::Row row(*iter);
			cfgunit[i] = row[m_perfunitscolumns.m_text];
		}
		m_sensors->get_configfile().set_string_list("tkoffldgdistance", "unit", cfgunit);
	}
	{
		std::vector<double> cfgvalue(perfval_nr * 2);
		for (unsigned int i = 0; i < perfval_nr; ++i)
			for (unsigned int j = 0; j < 2; ++j)
				cfgvalue[2 * i + j] = m_perfvalues[j][i];
		m_sensors->get_configfile().set_double_list("tkoffldgdistance", "value", cfgvalue);
	}
	{
		std::vector<Glib::ustring> cfgaltname(2);
		for (unsigned int i = 0; i < 2; ++i)
			if (m_perfentryname[i])
				cfgaltname[i] = m_perfentryname[i]->get_text();
		m_sensors->get_configfile().set_string_list("tkoffldgdistance", "altname", cfgaltname);
	}
	m_sensors->save_config();
}

void FlightDeckWindow::perfpage_restoreperf(void)
{
	if (!m_sensors || !m_sensors->get_configfile().has_group("tkoffldgdistance")) {
		perfpage_update();
		perfpage_update_results();
		return;
	}
	{
		std::vector<Glib::ustring> cfgunit;
		if (m_sensors->get_configfile().has_key("tkoffldgdistance", "unit"))
			cfgunit = m_sensors->get_configfile().get_string_list("tkoffldgdistance", "unit");
		for (unsigned int i = 0; i < perfunits_nr; ++i) {
			if (!m_perfcombobox[i] || i >= cfgunit.size() || !m_perfunitsstore[i])
				continue;
			Gtk::TreeModel::const_iterator iter(m_perfunitsstore[i]->children().begin());
			for (; iter; ++iter) {
				Gtk::TreeModel::Row row(*iter);
				if (row[m_perfunitscolumns.m_text] != cfgunit[i])
					continue;
				m_perfcombobox[i]->set_active(iter);
				break;
			}
		}
	}
	{
		std::vector<double> cfgvalue;
		if (m_sensors->get_configfile().has_key("tkoffldgdistance", "value"))
			cfgvalue = m_sensors->get_configfile().get_double_list("tkoffldgdistance", "value");
		for (unsigned int i = 0; i < perfval_nr; ++i) {
			if (2 * i >= cfgvalue.size())
				break;
			for (unsigned int j = 0; j < 2; ++j) {
				if (2 * i + j >= cfgvalue.size())
					break;
				m_perfvalues[j][i] = cfgvalue[2 * i + j];
			}
		}
	}
	{
		std::vector<Glib::ustring> cfgaltname;
		if (m_sensors->get_configfile().has_key("tkoffldgdistance", "altname"))
			cfgaltname = m_sensors->get_configfile().get_string_list("tkoffldgdistance", "altname");
		for (unsigned int i = 0; i < 2; ++i)
			if (m_perfentryname[i])
				m_perfentryname[i]->set_text(i < cfgaltname.size() ? cfgaltname[i] : "");
	}
	perfpage_update();
	perfpage_update_results();
}

void FlightDeckWindow::perfpage_button_fromwb_clicked(void)
{
	if (!m_perfwbstore || !m_sensors || !m_perfunitsstore[perfunits_mass])
		return;
	const Aircraft::WeightBalance& wb(m_sensors->get_aircraft().get_wb());
	double unitgain(1);
	double unitoffs(0);
	{
		Gtk::TreeModel::const_iterator iter(m_perfunitsstore[perfunits_mass]->children().begin());
		for (; iter; ++iter) {
			Gtk::TreeModel::Row row(*iter);
			if (row[m_perfunitscolumns.m_text] == wb.get_massunitname()) {
				unitgain = row[m_perfunitscolumns.m_gain];
				unitoffs = row[m_perfunitscolumns.m_offset];
				break;
			}
		}
		if (!iter) {
			std::cerr << "perfpage_button_fromwb: mass unit " << wb.get_massunitname() << " unknown" << std::endl;
			return;
		}
		if (m_perfcombobox[perfunits_mass])
			m_perfcombobox[perfunits_mass]->set_active(iter);
	}
	Gtk::TreeIter totiter;
	double minfuel(0), curfuel(0);
	for (Gtk::TreeIter iter(m_perfwbstore->children().begin()); iter; ++iter) {
		Gtk::TreeModel::Row row(*iter);
		unsigned int nr(row[m_perfwbcolumns.m_nr]);
		if (nr == ~0U) {
			totiter = iter;
			continue;
		}
		if (nr >= wb.get_nrelements())
			continue;
		const Aircraft::WeightBalance::Element& el(wb.get_element(nr));
		if (!(el.get_flags() & Aircraft::WeightBalance::Element::flag_fuel))
			continue;
		curfuel += row[m_perfwbcolumns.m_mass];
		minfuel += el.get_massmin();
	}
	if (!totiter)
		return;
	Gtk::TreeModel::Row row(*totiter);
	double mass(row[m_perfwbcolumns.m_mass]);
	curfuel = std::max(curfuel, minfuel);
	double ldgmass(mass);
	if (m_sensors->get_route().get_nrwpt()) {
		const FPlanRoute& fpl(m_sensors->get_route());
		const FPlanWaypoint& wpt(fpl[fpl.get_nrwpt()-1]);
		ldgmass -= std::min(wpt.get_fuel_usg() * m_sensors->get_aircraft().get_fuelmass(), curfuel - minfuel);
		if (std::isnan(ldgmass))
			ldgmass = mass;
		if (true)
			std::cerr << "Landing Mass correction: fuel " << wpt.get_fuel_usg()
				  << " fuelmass " << m_sensors->get_aircraft().get_fuelmass()
				  << " curfuel " << curfuel << " minfuel " << minfuel << std::endl;
	}
	m_perfvalues[0][perfval_mass] = mass * unitgain + unitoffs;
	m_perfvalues[1][perfval_mass] = ldgmass * unitgain + unitoffs;
	perfpage_update();
	perfpage_update_results();
	perfpage_saveperf();
}

void FlightDeckWindow::perfpage_button_fromfpl_clicked(void)
{
	if (!m_sensors)
		return;
	const FPlanRoute& fpl(m_sensors->get_route());
	if (!fpl.get_nrwpt())
		return;
	for (unsigned int j = 0; j < 2; ++j) {
		const FPlanWaypoint& wpt(fpl[(fpl.get_nrwpt() - 1) * j]);
		if (m_perfentryname[j])
			m_perfentryname[j]->set_text(wpt.get_icao_name());
		m_perfvalues[j][perfval_altitude] = wpt.get_altitude() * FPlanWaypoint::ft_to_m;
		m_perfvalues[j][perfval_isa] = wpt.get_isaoffset_kelvin();
		if (wpt.get_qff_hpa() > 100)
			m_perfvalues[j][perfval_qnh] = wpt.get_qff_hpa();
		IcaoAtmosphere<float> icao(m_perfvalues[j][perfval_qnh], m_perfvalues[j][perfval_isa] + IcaoAtmosphere<double>::std_sealevel_temperature);
		float press, temp;
	        icao.altitude_to_pressure(&press, &temp, m_perfvalues[j][perfval_altitude]);
		m_perfvalues[j][perfval_oat] = temp;
	}
	perfpage_update();
	perfpage_update_results();
	perfpage_saveperf();
}

void FlightDeckWindow::perfpage_update(unsigned int idxmask, perfval_t pvskip)
{
	double unitgain[perfval_nr];
	double unitoffs[perfval_nr];
	for (unsigned int i = 0; i < perfunits_nr; ++i) {
		unitgain[i] = 1;
		unitoffs[i] = 0;
		if (!m_perfcombobox[i])
			continue;
                Gtk::TreeModel::const_iterator iter(m_perfcombobox[i]->get_active());
                if (!iter)
			continue;
		Gtk::TreeModel::Row row(*iter);
		unitgain[i] = 1.0 / row[m_perfunitscolumns.m_gain];
		unitoffs[i] = row[m_perfunitscolumns.m_offset];
	}
	for (unsigned int i = 0; i < perfval_nr; ++i)
		for (unsigned int j = 0; j < 2; ++j)
			m_perfspinbuttonconn[j][i].block();
	for (unsigned int j = 0; j < 2; ++j) {
		if (!(idxmask & (1 << j)))
			continue;
		for (unsigned int i = 0; i < perfval_nr; ++i) {
			if (i == pvskip)
				continue;
			perfunits_t u(prefvalueunits[i]);
			if (!m_perfspinbutton[j][i])
				continue;
			m_perfspinbutton[j][i]->set_value((m_perfvalues[j][i] - (i == perfval_isa ? 0 : unitoffs[u])) * unitgain[u]);
		}
	}
	for (unsigned int i = 0; i < perfval_nr; ++i)
		for (unsigned int j = 0; j < 2; ++j)
			m_perfspinbuttonconn[j][i].unblock();
}

void FlightDeckWindow::perfpage_unit_changed(perfunits_t pu)
{
	perfpage_update();
}

void FlightDeckWindow::perfpage_spinbutton_value_changed(unsigned int idx, perfval_t pv)
{
	if (pv >= perfval_nr || idx >= 2 || !m_perfspinbutton[idx][pv])
		return;
	double unitgain(1);
	double unitoffs(0);
	perfunits_t u(prefvalueunits[pv]);
	if (m_perfcombobox[u]) {
		Gtk::TreeModel::const_iterator iter(m_perfcombobox[u]->get_active());
                if (iter) {
			Gtk::TreeModel::Row row(*iter);
			unitgain = row[m_perfunitscolumns.m_gain];
			unitoffs = row[m_perfunitscolumns.m_offset];
		}
	}
	m_perfvalues[idx][pv] = m_perfspinbutton[idx][pv]->get_value() * unitgain + (pv == perfval_isa ? 0 : unitoffs);
	switch (pv) {
	case perfval_altitude:
		if (m_perfentryname[idx])
			m_perfentryname[idx]->set_text("");
		/* fall through */

	case perfval_isa:
	case perfval_qnh:
	{
		IcaoAtmosphere<float> icao(m_perfvalues[idx][perfval_qnh], m_perfvalues[idx][perfval_isa] + IcaoAtmosphere<double>::std_sealevel_temperature);
		float press, temp;
	        icao.altitude_to_pressure(&press, &temp, m_perfvalues[idx][perfval_altitude]);
		m_perfvalues[idx][perfval_oat] = temp;
		break;
	}

	case perfval_oat:
	{
		IcaoAtmosphere<float> icao(m_perfvalues[idx][perfval_qnh], IcaoAtmosphere<double>::std_sealevel_temperature);
		float press, temp;
	        icao.altitude_to_pressure(&press, &temp, m_perfvalues[idx][perfval_altitude]);
		m_perfvalues[idx][perfval_isa] = m_perfvalues[idx][perfval_oat] - temp;
		break;
	}

	default:
		break;
	}
	perfpage_update_results();
	perfpage_update(1 << idx, pv);
	perfpage_saveperf();
}

void FlightDeckWindow::perfpage_softpenalty_value_changed(void)
{
	if (m_perfspinbuttonsoftsurface) {
		m_perfsoftfieldpenalty = m_perfspinbuttonsoftsurface->get_value() * 0.01;
		perfpage_update_results();
	}
}

class FlightDeckWindow::PerfAtmo {
public:
	static constexpr double const_m             = 28.9644;                     /* Molecular Weight of air */
	static constexpr double const_r             = 8.31432;                     /* gas const. for air */
	static const double const_mr;
	static const double const_rm;
	static constexpr double const_rm_watervapor = 0.461495;
	static const double const_mr_watervapor;

	static const double const_zerodegc;
	static constexpr double const_wobus_c0      = 0.99999683;
	static constexpr double const_wobus_c1      = -0.90826951e-02;
	static constexpr double const_wobus_c2      = 0.78736169e-04;
	static constexpr double const_wobus_c3      = -0.61117958e-06;
	static constexpr double const_wobus_c4      = 0.43884187e-08;
	static constexpr double const_wobus_c5      = -0.29883885e-10;
	static constexpr double const_wobus_c6      = 0.21874425e-12;
	static constexpr double const_wobus_c7      = -0.17892321e-14;
	static constexpr double const_wobus_c8      = 0.11112018e-16;
	static constexpr double const_wobus_c9      = -0.30994571e-19;
	static constexpr double const_wobus_es0     = 6.1078;

	static double temp_to_vapor_pressure(double temp);
	static double pressure_to_density(double pressure, double temp, double dewpoint);
	static double density_to_pressure(double density, double temp, double dewpoint);
	static double relative_humidity(double temp, double dewpoint);
};

constexpr double FlightDeckWindow::PerfAtmo::const_m;
constexpr double FlightDeckWindow::PerfAtmo::const_r;
const double FlightDeckWindow::PerfAtmo::const_mr            = const_m / const_r;
const double FlightDeckWindow::PerfAtmo::const_rm            = const_r / const_m;
constexpr double FlightDeckWindow::PerfAtmo::const_rm_watervapor;
const double FlightDeckWindow::PerfAtmo::const_mr_watervapor = 1.0 / const_rm_watervapor;

const double FlightDeckWindow::PerfAtmo::const_zerodegc      = IcaoAtmosphere<double>::degc_to_kelvin;
constexpr double FlightDeckWindow::PerfAtmo::const_wobus_c0;
constexpr double FlightDeckWindow::PerfAtmo::const_wobus_c1;
constexpr double FlightDeckWindow::PerfAtmo::const_wobus_c2;
constexpr double FlightDeckWindow::PerfAtmo::const_wobus_c3;
constexpr double FlightDeckWindow::PerfAtmo::const_wobus_c4;
constexpr double FlightDeckWindow::PerfAtmo::const_wobus_c5;
constexpr double FlightDeckWindow::PerfAtmo::const_wobus_c6;
constexpr double FlightDeckWindow::PerfAtmo::const_wobus_c7;
constexpr double FlightDeckWindow::PerfAtmo::const_wobus_c8;
constexpr double FlightDeckWindow::PerfAtmo::const_wobus_c9;
constexpr double FlightDeckWindow::PerfAtmo::const_wobus_es0;

double FlightDeckWindow::PerfAtmo::temp_to_vapor_pressure(double temp)
{
        temp -= const_zerodegc;
        double pol = const_wobus_c0 + temp * (const_wobus_c1 + temp *
					      (const_wobus_c2 + temp *
					       (const_wobus_c3 + temp *
						(const_wobus_c4 + temp *
						 (const_wobus_c5 + temp *
						  (const_wobus_c6 + temp *
						   (const_wobus_c7 + temp *
						    (const_wobus_c8 + temp * const_wobus_c9))))))));
        return const_wobus_es0 / pow(pol, 8);
}


double FlightDeckWindow::PerfAtmo::pressure_to_density(double pressure, double temp, double dewpoint)
{
        if (temp < 0.01)
                temp = 0.01;
        if (dewpoint < 0.01)
                dewpoint = 0.01;
        if (dewpoint > temp)
                dewpoint = temp;
        double vaporpressure(temp_to_vapor_pressure(dewpoint));
        double airpressure(pressure - vaporpressure);
        return (const_mr * airpressure + const_mr_watervapor * vaporpressure) / temp;
}

double FlightDeckWindow::PerfAtmo::density_to_pressure(double density, double temp, double dewpoint)
{
        if (temp < 0.01)
                temp = 0.01;
        if (dewpoint < 0.01)
                dewpoint = 0.01;
        if (dewpoint > temp)
                dewpoint = temp;
        double vaporpressure(temp_to_vapor_pressure(dewpoint));
        double airpressure((density * temp - const_mr_watervapor * vaporpressure)  * const_rm);
        return airpressure + vaporpressure;
}

double FlightDeckWindow::PerfAtmo::relative_humidity(double temp, double dewpoint)
{
        if (temp < 0.01)
                temp = 0.01;
        if (dewpoint < 0.01)
                dewpoint = 0.01;
        if (dewpoint > temp)
                return 1.0;
        return temp_to_vapor_pressure(dewpoint) / temp_to_vapor_pressure(temp);
}

void FlightDeckWindow::perfpage_update_results(void)
{
	for (unsigned int j = 0; j < 2; ++j) {
		m_perfvalues[j][perfval_oat] = std::max(m_perfvalues[j][perfval_oat], 0.0);
		m_perfvalues[j][perfval_dewpt] = std::max(m_perfvalues[j][perfval_dewpt], 0.0);
		if (!m_perfresultstore[j])
			continue;
		m_perfresultstore[j]->clear();
		AirData<float> ad;
		ad.set_qnh_tempoffs(m_perfvalues[j][perfval_qnh], m_perfvalues[j][perfval_isa]);
		float press, densityalt;
		double density;
		{
			float temp1;
			ad.altitude_to_pressure(&press, &temp1, m_perfvalues[j][perfval_altitude]);
			density = FlightDeckWindow::PerfAtmo::pressure_to_density(press, m_perfvalues[j][perfval_oat], m_perfvalues[j][perfval_dewpt]);
			for (unsigned int i = 0; i < 4; ++i) {
				float press1(IcaoAtmosphere<float>::density_to_pressure(density, temp1));
				IcaoAtmosphere<float>::std_pressure_to_altitude(&densityalt, &temp1, press1);
			}
		}
		double rh(FlightDeckWindow::PerfAtmo::relative_humidity(m_perfvalues[j][perfval_oat], m_perfvalues[j][perfval_dewpt]));
		if (m_sensors) {
			const Aircraft::Distances& dist(m_sensors->get_aircraft().get_dist());
			double mass(m_perfvalues[j][perfval_mass]);
			double headwind(m_perfvalues[j][perfval_wind] * 1.9438445); // convert from m/s to knots
			unsigned int tkoffidx(~0U);
			double shortesttkoffgnd(std::numeric_limits<double>::max());
			for (unsigned int i(0); i < dist.get_nrtakeoffdist(); ++i) {
				const Aircraft::Distances::Distance& d(dist.get_takeoffdist(i));
				double gnddist, obstdist;
				d.calculate(gnddist, obstdist, densityalt * Point::m_to_ft, headwind, mass);
				if (std::isnan(gnddist) && std::isnan(obstdist))
					continue;
				gnddist = Aircraft::convert(dist.get_distunit(), Aircraft::unit_m, gnddist);
				obstdist = Aircraft::convert(dist.get_distunit(), Aircraft::unit_m, obstdist);
				if (gnddist < shortesttkoffgnd) {
					shortesttkoffgnd = gnddist;
					tkoffidx = i;
				}
				Gtk::TreeModel::Row row(*(m_perfresultstore[j]->append()));
				row[m_perfresultcolumns.m_text] = "Takeoff Distance " + d.get_name();
				if (!std::isnan(gnddist)) {
					Gtk::TreeModel::Row rowc(*(m_perfresultstore[j]->append(row.children())));
					rowc[m_perfresultcolumns.m_text] = "Ground Roll";
					rowc[m_perfresultcolumns.m_value1] = Glib::ustring::format(std::fixed, std::setprecision(0), gnddist) + "m";
					rowc[m_perfresultcolumns.m_value2] = Glib::ustring::format(std::fixed, std::setprecision(0), gnddist * Point::m_to_ft) + "ft";
				}
				if (!std::isnan(obstdist)) {
					Gtk::TreeModel::Row rowc(*(m_perfresultstore[j]->append(row.children())));
					rowc[m_perfresultcolumns.m_text] = "50ft Obstacle";
					rowc[m_perfresultcolumns.m_value1] = Glib::ustring::format(std::fixed, std::setprecision(0), obstdist) + "m";
					rowc[m_perfresultcolumns.m_value2] = Glib::ustring::format(std::fixed, std::setprecision(0), obstdist * Point::m_to_ft) + "ft";
				}
				if (m_perftreeview[j])
					m_perftreeview[j]->expand_row(m_perfresultstore[j]->get_path(row), false);
			}
			for (unsigned int i(0); i < dist.get_nrldgdist(); ++i) {
				const Aircraft::Distances::Distance& d(dist.get_ldgdist(i));
				double gnddist, obstdist;
				d.calculate(gnddist, obstdist, densityalt * Point::m_to_ft, headwind, mass);
				if (std::isnan(gnddist) && std::isnan(obstdist))
					continue;
				gnddist = Aircraft::convert(dist.get_distunit(), Aircraft::unit_m, gnddist);
				obstdist = Aircraft::convert(dist.get_distunit(), Aircraft::unit_m, obstdist);
				Gtk::TreeModel::Row row(*(m_perfresultstore[j]->append()));
				row[m_perfresultcolumns.m_text] = "Landing Distance " + d.get_name();
				if (!std::isnan(gnddist)) {
					Gtk::TreeModel::Row rowc(*(m_perfresultstore[j]->append(row.children())));
					rowc[m_perfresultcolumns.m_text] = "Ground Roll";
					rowc[m_perfresultcolumns.m_value1] = Glib::ustring::format(std::fixed, std::setprecision(0), gnddist) + "m";
					rowc[m_perfresultcolumns.m_value2] = Glib::ustring::format(std::fixed, std::setprecision(0), gnddist * Point::m_to_ft) + "ft";
				}
				if (!std::isnan(obstdist)) {
					Gtk::TreeModel::Row rowc(*(m_perfresultstore[j]->append(row.children())));
					rowc[m_perfresultcolumns.m_text] = "50ft Obstacle";
					rowc[m_perfresultcolumns.m_value1] = Glib::ustring::format(std::fixed, std::setprecision(0), obstdist) + "m";
					rowc[m_perfresultcolumns.m_value2] = Glib::ustring::format(std::fixed, std::setprecision(0), obstdist * Point::m_to_ft) + "ft";
				}
				if (m_perftreeview[j])
					m_perftreeview[j]->expand_row(m_perfresultstore[j]->get_path(row), false);
			}
			if (tkoffidx < dist.get_nrtakeoffdist()) {
				const Aircraft::Distances::Distance& d(dist.get_takeoffdist(tkoffidx));
				double gnddist, obstdist;
				d.calculate(gnddist, obstdist, densityalt * Point::m_to_ft, headwind, mass);
				if (std::isnan(gnddist) && std::isnan(obstdist))
					continue;
				gnddist = Aircraft::convert(dist.get_distunit(), Aircraft::unit_m, gnddist);
				obstdist = Aircraft::convert(dist.get_distunit(), Aircraft::unit_m, obstdist);
				{
					double penalty(m_perfsoftfieldpenalty * gnddist);
					obstdist += penalty;
					gnddist += penalty;
				}
				Gtk::TreeModel::Row row(*(m_perfresultstore[j]->append()));
				row[m_perfresultcolumns.m_text] = "Soft Field Takeoff Distance " + d.get_name();
				if (!std::isnan(gnddist)) {
					Gtk::TreeModel::Row rowc(*(m_perfresultstore[j]->append(row.children())));
					rowc[m_perfresultcolumns.m_text] = "Ground Roll";
					rowc[m_perfresultcolumns.m_value1] = Glib::ustring::format(std::fixed, std::setprecision(0), gnddist) + "m";
					rowc[m_perfresultcolumns.m_value2] = Glib::ustring::format(std::fixed, std::setprecision(0), gnddist * Point::m_to_ft) + "ft";
				}
				if (!std::isnan(obstdist)) {
					Gtk::TreeModel::Row rowc(*(m_perfresultstore[j]->append(row.children())));
					rowc[m_perfresultcolumns.m_text] = "50ft Obstacle";
					rowc[m_perfresultcolumns.m_value1] = Glib::ustring::format(std::fixed, std::setprecision(0), obstdist) + "m";
					rowc[m_perfresultcolumns.m_value2] = Glib::ustring::format(std::fixed, std::setprecision(0), obstdist * Point::m_to_ft) + "ft";
				}
				if (m_perftreeview[j])
					m_perftreeview[j]->expand_row(m_perfresultstore[j]->get_path(row), false);
			}
		}
		{
			Gtk::TreeModel::Row row(*(m_perfresultstore[j]->append()));
			row[m_perfresultcolumns.m_text] = "Density Altitude";
			row[m_perfresultcolumns.m_value1] = Glib::ustring::format(std::fixed, std::setprecision(0), densityalt) + "m";
			row[m_perfresultcolumns.m_value2] = Glib::ustring::format(std::fixed, std::setprecision(0), densityalt * Point::m_to_ft) + "ft";
			row = *(m_perfresultstore[j]->append());
			row[m_perfresultcolumns.m_text] = "Pressure";
			row[m_perfresultcolumns.m_value1] = Glib::ustring::format(std::fixed, std::setprecision(0), press) + "hPa";
			row[m_perfresultcolumns.m_value2] = Glib::ustring::format(std::fixed, std::setprecision(2), press * (1.0 / 33.863886)) + "inHg";
			row = *(m_perfresultstore[j]->append());
			row[m_perfresultcolumns.m_text] = "Density";
			row[m_perfresultcolumns.m_value1] = Glib::ustring::format(std::fixed, std::setprecision(3), density * 0.1) + "kg/m^3";
			row = *(m_perfresultstore[j]->append());
			row[m_perfresultcolumns.m_text] = "Relative Humidity";
			row[m_perfresultcolumns.m_value1] = Glib::ustring::format(std::fixed, std::setprecision(0), rh * 100.0) + "%";
		}
		if (m_sensors) {
			Aircraft::ClimbDescent climb(m_sensors->get_aircraft().calculate_climb("", m_perfvalues[j][perfval_mass]));
			double atakeoff(m_perfvalues[j][perfval_altitude]);
			double ttakeoff(climb.altitude_to_time(densityalt));
			double dtakeoff(climb.time_to_distance(ttakeoff));
			Gtk::TreeModel::Row row(*(m_perfresultstore[j]->append()));
			row[m_perfresultcolumns.m_text] = "Climb Performance";
			for (double alt = 1e3 * ceil(atakeoff * 1e-3); alt <= 200000; alt += 1000) {
				ad.set_true_altitude(alt);
				if (ad.get_density_altitude() <= densityalt * Point::m_to_ft)
					continue;
				if (ad.get_density_altitude() > climb.get_ceiling())
					break;
				double t(climb.altitude_to_time(ad.get_density_altitude()));
				double d(climb.time_to_distance(t));
				t -= ttakeoff;
				d -= dtakeoff;
				if (t <= 0 || d < 0.1)
					continue;
				double cgrad((100 * Point::ft_to_m_dbl * 1e-3 * Point::km_to_nmi_dbl) * (alt - atakeoff) / d);
				Gtk::TreeModel::Row rowc(*(m_perfresultstore[j]->append(row.children())));
				rowc[m_perfresultcolumns.m_text] = Glib::ustring::format(std::fixed, std::setprecision(0), std::setw(5), alt) +
					"ft (FL" + Glib::ustring::format(std::fixed, std::setprecision(0), std::setw(3), std::setfill(L'0'), ad.get_pressure_altitude() * 1e-2) +
					"; DA " + Glib::ustring::format(std::fixed, std::setprecision(0), ad.get_density_altitude()) + "ft; " +
					Conversions::time_str(t) + ")";
				rowc[m_perfresultcolumns.m_value1] = Glib::ustring::format(std::fixed, std::setprecision(2), cgrad) + "%";
				rowc[m_perfresultcolumns.m_value2] = Glib::ustring::format(std::fixed, std::setprecision(2), d) + "nmi";
			}
		}
	}
}

Aircraft::WeightBalance::elementvalues_t FlightDeckWindow::perfpage_getwbvalues(void)
{
	perfpage_reloadwb();
	Aircraft::WeightBalance::elementvalues_t wbev;
	if (!m_perfwbstore || !m_sensors)
		return wbev;
	const Aircraft::WeightBalance& wb(m_sensors->get_aircraft().get_wb());
	for (Gtk::TreeModel::iterator iter(m_perfwbstore->children().begin()); iter; ++iter) {
		Gtk::TreeModel::Row row(*iter);
		unsigned int i(row[m_perfwbcolumns.m_nr]);
		if (i >= wb.get_nrelements())
			continue;
		const Aircraft::WeightBalance::Element& el(wb.get_element(i));
		Aircraft::WeightBalance::ElementValue ev(row[m_perfwbcolumns.m_value]);
		const Glib::ustring& unitname(row[m_perfwbcolumns.m_unit]);
		for (unsigned int j = 0; j < el.get_nrunits(); ++j) {
			const Aircraft::WeightBalance::Element::Unit& u(el.get_unit(j));
			if (unitname != u.get_name())
				continue;
			ev.set_unit(j);
			break;
		}
		if (wbev.size() <= i)
			wbev.resize(i + 1, Aircraft::WeightBalance::ElementValue());
		if (false)
			std::cerr << "perfpage_getwbvalues: " << i << ' ' << ev.get_value() << ' ' << ev.get_unit() << std::endl;
		wbev[i] = ev;
	}
	return wbev;
}

void FlightDeckWindow::perfpage_reloadwb(void)
{
	for (unsigned int j = 0; j < 2; ++j) {
		IcaoAtmosphere<float> icao(m_perfvalues[j][perfval_qnh], m_perfvalues[j][perfval_isa] + IcaoAtmosphere<double>::std_sealevel_temperature);
		float press;
		float temp;
	        icao.altitude_to_pressure(&press, &temp, m_perfvalues[j][perfval_altitude]);
		m_perfvalues[j][perfval_oat] = temp;
	}
	perfpage_restoreperf();
	if (m_perfwbstore) {
		m_perfwbstore->clear();
		if (m_sensors) {
			std::vector<Glib::ustring> cfgunit;
			std::vector<double> cfgmass;
			if (m_sensors->get_configfile().has_group("weightbalance")) {
				if (m_sensors->get_configfile().has_key("weightbalance", "unit"))
					cfgunit = m_sensors->get_configfile().get_string_list("weightbalance", "unit");
				if (m_sensors->get_configfile().has_key("weightbalance", "mass"))
					cfgmass = m_sensors->get_configfile().get_double_list("weightbalance", "mass");
			}
			const Aircraft::WeightBalance& wb(m_sensors->get_aircraft().get_wb());
			for (unsigned int i = 0; i < wb.get_nrelements(); ++i) {
				const Aircraft::WeightBalance::Element& el(wb.get_element(i));
				double mass((el.get_flags() & (Aircraft::WeightBalance::Element::flag_oil |
							       Aircraft::WeightBalance::Element::flag_fuel))
					    ? el.get_massmax() : el.get_massmin());
				if (cfgmass.size() > i)
					mass = std::max(std::min(cfgmass[i], el.get_massmax()), el.get_massmin());
				Gtk::TreeModel::Row row(*(m_perfwbstore->append()));
				row[m_perfwbcolumns.m_valueeditable] = !(el.get_flags() & Aircraft::WeightBalance::Element::flag_fixed);
				row[m_perfwbcolumns.m_value] = mass;
				if (el.get_nrunits() > 0) {
					Glib::RefPtr<Gtk::ListStore> t(Gtk::ListStore::create(m_perfunitscolumns));
					for (unsigned int j = 0; j < el.get_nrunits(); ++j) {
						const Aircraft::WeightBalance::Element::Unit& u(el.get_unit(j));
						Gtk::TreeModel::Row rowu(*(t->append()));
						rowu[m_perfunitscolumns.m_text] = u.get_name();
						if (!j || (cfgunit.size() > i && u.get_name() == cfgunit[i])) {
							row[m_perfwbcolumns.m_unit] = u.get_name();
							if ((el.get_flags() & Aircraft::WeightBalance::Element::flag_fixed) || u.is_fixed()) {
								row[m_perfwbcolumns.m_value] = u.get_offset();
								row[m_perfwbcolumns.m_value] = u.get_offset();
							} else {
								row[m_perfwbcolumns.m_value] = (mass - u.get_offset()) / u.get_factor();
							}
						}
						rowu[m_perfunitscolumns.m_gain] = u.get_factor();
						rowu[m_perfunitscolumns.m_offset] = u.get_offset();
						if (u.is_fixed())
							row[m_perfwbcolumns.m_valueeditable] = false;
					}
					row[m_perfwbcolumns.m_model] = t;
				}
				row[m_perfwbcolumns.m_modelvisible] = el.get_nrunits() > 1;
				row[m_perfwbcolumns.m_text] = el.get_name();
				row[m_perfwbcolumns.m_mass] = mass;
				row[m_perfwbcolumns.m_minmass] = el.get_massmin();
				row[m_perfwbcolumns.m_maxmass] = el.get_massmax();
				row[m_perfwbcolumns.m_arm] = el.get_arm(mass);
				row[m_perfwbcolumns.m_moment] = el.get_moment(mass);
				row[m_perfwbcolumns.m_style] = Pango::STYLE_NORMAL;
				row[m_perfwbcolumns.m_nr] = i;
			}
			{
				Gtk::TreeModel::Row row(*(m_perfwbstore->append()));
				row[m_perfwbcolumns.m_valueeditable] = false;
				row[m_perfwbcolumns.m_value] = 0;
				row[m_perfwbcolumns.m_unit] = "";
				row[m_perfwbcolumns.m_modelvisible] = false;
				row[m_perfwbcolumns.m_text] = "Total";
				row[m_perfwbcolumns.m_mass] = 0;
				row[m_perfwbcolumns.m_arm] = 0;
				row[m_perfwbcolumns.m_moment] = 0;
				row[m_perfwbcolumns.m_style] = Pango::STYLE_ITALIC;
				row[m_perfwbcolumns.m_nr] = ~0U;
				double minmass(std::numeric_limits<double>::max());
				double maxmass(0);
				for (unsigned int i = 0; i < wb.get_nrenvelopes(); ++i) {
					const Aircraft::WeightBalance::Envelope& ev(wb.get_envelope(i));
					double mina, maxa, minm, maxm;
					ev.get_bounds(mina, maxa, minm, maxm);
					minmass = std::min(minmass, minm);
					maxmass = std::max(maxmass, maxm);
				}
				minmass = std::min(minmass, maxmass);
				row[m_perfwbcolumns.m_minmass] = minmass;
				row[m_perfwbcolumns.m_maxmass] = maxmass;
			}
		}
		perfpage_recomputewb();
	}
}

void FlightDeckWindow::perfpage_updatemenu(void)
{
	if (m_menuid != 302)
		return;
	perfpage_reloadwb();
}
