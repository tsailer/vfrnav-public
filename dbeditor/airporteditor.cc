/***************************************************************************
 *   Copyright (C) 2007, 2008, 2009, 2012, 2013, 2016 by Thomas Sailer     *
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

#include <gtkmm/main.h>
#include <glibmm/optionentry.h>
#include <glibmm/optiongroup.h>
#include <glibmm/optioncontext.h>
#include <set>
#include <sstream>
#include <iomanip>
#include <cstring>

#include "dbeditor.h"
#include "wmm.h"
#include "sysdepsgui.h"

AirportEditor::AirportModelColumns::AirportModelColumns(void)
{
	add(m_col_id);
	add(m_col_coord);
	add(m_col_label_placement);
	add(m_col_sourceid);
	add(m_col_icao);
	add(m_col_name);
	add(m_col_elev);
	add(m_col_typecode);
	add(m_col_frules_arr_vfr);
	add(m_col_frules_arr_ifr);
	add(m_col_frules_dep_vfr);
	add(m_col_frules_dep_ifr);
	add(m_col_modtime);
}

AirportEditor::AirportRunwayModelColumns::AirportRunwayModelColumns(void)
{
	add(m_col_index);
	add(m_col_ident_he);
	add(m_col_ident_le);
	add(m_col_length);
	add(m_col_width);
	add(m_col_surface);
	add(m_col_he_coord);
	add(m_col_le_coord);
	add(m_col_he_tda);
	add(m_col_he_lda);
	add(m_col_he_disp);
	add(m_col_he_hdg);
	add(m_col_he_elev);
	add(m_col_le_tda);
	add(m_col_le_lda);
	add(m_col_le_disp);
	add(m_col_le_hdg);
	add(m_col_le_elev);
	add(m_col_flags_he_usable);
	add(m_col_flags_le_usable);
	for (unsigned int i = 0; i < 8; ++i)
		add(m_col_he_light[i]);
	for (unsigned int i = 0; i < 8; ++i)
		add(m_col_le_light[i]);
}

AirportEditor::AirportHelipadModelColumns::AirportHelipadModelColumns(void)
{
	add(m_col_index);
	add(m_col_ident);
	add(m_col_length);
	add(m_col_width);
	add(m_col_surface);
	add(m_col_coord);
	add(m_col_hdg);
	add(m_col_elev);
	add(m_col_flags_usable);
}

AirportEditor::AirportCommsModelColumns::AirportCommsModelColumns(void)
{
	add(m_col_index);
	add(m_col_freq0);
	add(m_col_freq1);
	add(m_col_freq2);
	add(m_col_freq3);
	add(m_col_freq4);
	add(m_col_name);
	add(m_col_sector);
	add(m_col_oprhours);
}

AirportEditor::AirportVFRRoutesModelColumns::AirportVFRRoutesModelColumns(void)
{
	add(m_col_index);
	add(m_col_name);
	add(m_col_minalt);
	add(m_col_maxalt);
	add(m_col_cpcoord);
	add(m_col_cpalt);
	add(m_col_cppath);
	add(m_col_cpname);
}

AirportEditor::AirportVFRRouteModelColumns::AirportVFRRouteModelColumns(void)
{
	add(m_col_index);
	add(m_col_name);
	add(m_col_coord);
	add(m_col_label_placement);
	add(m_col_alt);
	add(m_col_altcode);
	add(m_col_pathcode);
	add(m_col_ptsym);
	add(m_col_at_airport);
}

AirportEditor::AirportVFRPointListModelColumns::AirportVFRPointListModelColumns(void)
{
	add(m_col_name);
	add(m_col_coord);
	add(m_col_label_placement);
	add(m_col_at_airport);
}

AirportEditor::PointListEntry::PointListEntry(const Glib::ustring & name, AirportsDb::Airport::label_placement_t lp, const Point & pt, bool atapt)
	: m_name(name), m_coord(pt), m_labelplacement(lp), m_at_airport(atapt)
{
}

bool AirportEditor::PointListEntry::operator <(const PointListEntry & pe) const
{
	if (m_at_airport > pe.m_at_airport)
		return true;
	if (m_at_airport < pe.m_at_airport)
		return false;
	if (m_name < pe.m_name)
		return true;
	if (m_name > pe.m_name)
		return false;
	if (m_coord.get_lon() < pe.m_coord.get_lon())
		return true;
	if (m_coord.get_lon() > pe.m_coord.get_lon())
		return false;
	if (m_coord.get_lat() < pe.m_coord.get_lat())
		return true;
	if (m_coord.get_lat() > pe.m_coord.get_lat())
		return false;
	return false;
}

class AirportEditor::RunwayDimensions : public toplevel_window_t {
	public:
		explicit RunwayDimensions(BaseObjectType *castitem, const Glib::RefPtr<Builder>& refxml);
		virtual ~RunwayDimensions();

		static Glib::RefPtr<RunwayDimensions> create(void);

		void set_runwayinfo(const AirportsDb::Airport::Runway& rwy, const Point& arp);
		sigc::signal<void,const AirportsDb::Airport::Runway&>& signal_set(void) { return m_cb; }
		const sigc::signal<void,const AirportsDb::Airport::Runway&>& signal_set(void) const { return m_cb; }

	protected:
		typedef enum {
			indep_length,
			indep_heading,
			indep_centercoord,
			indep_hecoord,
			indep_lecoord
		} indep_t;
		Glib::RefPtr<Builder> m_refxml;
		sigc::signal<void,const AirportsDb::Airport::Runway&> m_cb;
		AirportsDb::Airport::Runway m_rwy;
		Point m_arp;
		Point m_center;
		unsigned int m_radiomask;
		sigc::connection m_radioconn[5];
		Gtk::CheckButton *m_radio[5];
		Gtk::Button *m_buttonapply;
		Gtk::Button *m_buttoncancel;
		Gtk::Button *m_buttonok;
		Gtk::Entry *m_heident;
		Gtk::Entry *m_leident;
		Gtk::Entry *m_width;
		Gtk::Entry *m_length;
		Gtk::Entry *m_heading;
		Gtk::Entry *m_centerlon;
		Gtk::Entry *m_centerlat;
		Gtk::Entry *m_helon;
		Gtk::Entry *m_helat;
		Gtk::Entry *m_lelon;
		Gtk::Entry *m_lelat;
		Gtk::Entry *m_hedisp;
		Gtk::Entry *m_ledisp;
		Gtk::Entry *m_heelev;
		Gtk::Entry *m_leelev;

		template <typename T> static Glib::ustring numtotext(T x);
		static Glib::ustring angletotext(uint16_t x);
		static uint16_t texttoangle(const Glib::ustring& x);
		void buttonapply_clicked(void);
		void buttoncancel_clicked(void);
		void buttonok_clicked(void);
		void buttonradio_clicked(indep_t bnr);
		void set_independent(indep_t nr);
		void compute_dependent(void);
		void compute_ldatda(void);
		void edited_heident(void);
		void edited_leident(void);
		void edited_width(void);
		void edited_length(void);
		void edited_heading(void);
		void edited_centerlon(void);
		void edited_centerlat(void);
		void edited_helon(void);
		void edited_helat(void);
		void edited_lelon(void);
		void edited_lelat(void);
		void edited_hedisp(void);
		void edited_ledisp(void);
		void edited_heelev(void);
		void edited_leelev(void);
		void set_heading(uint16_t ang);
};

template <typename T> Glib::ustring AirportEditor::RunwayDimensions::numtotext(T x)
{
	std::ostringstream oss;
	oss << x;
	return oss.str();
}

Glib::ustring AirportEditor::RunwayDimensions::angletotext(uint16_t x)
{
	char buf[16];
	snprintf(buf, sizeof(buf), "%5.1f", x * FPlanLeg::to_deg);
	return buf;
}

uint16_t AirportEditor::RunwayDimensions::texttoangle(const Glib::ustring & x)
{
	return Point::round<int,double>(strtod(x.c_str(), 0) * FPlanLeg::from_deg);
}

AirportEditor::RunwayDimensions::RunwayDimensions(BaseObjectType * castitem, const Glib::RefPtr< Builder > & refxml)
	: toplevel_window_t(castitem), m_refxml(refxml), m_rwy("", "", 0, 0, "", Point::invalid, Point::invalid, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0),
	  m_radiomask((1 << indep_length) | (1 << indep_heading) | (1 << indep_centercoord))
{
	get_widget(m_refxml, "buttonapply", m_buttonapply);
	get_widget(m_refxml, "buttoncancel", m_buttoncancel);
	get_widget(m_refxml, "buttonok", m_buttonok);
	get_widget(m_refxml, "heident", m_heident);
	get_widget(m_refxml, "leident", m_leident);
	get_widget(m_refxml, "width", m_width);
	get_widget(m_refxml, "length", m_length);
	get_widget(m_refxml, "heading", m_heading);
	get_widget(m_refxml, "radiolength", m_radio[indep_length]);
	get_widget(m_refxml, "radioheading", m_radio[indep_heading]);
	get_widget(m_refxml, "radiocentercoord", m_radio[indep_centercoord]);
	get_widget(m_refxml, "radiohecoord", m_radio[indep_hecoord]);
	get_widget(m_refxml, "radiolecoord", m_radio[indep_lecoord]);
	get_widget(m_refxml, "centerlon", m_centerlon);
	get_widget(m_refxml, "centerlat", m_centerlat);
	get_widget(m_refxml, "helon", m_helon);
	get_widget(m_refxml, "helat", m_helat);
	get_widget(m_refxml, "lelon", m_lelon);
	get_widget(m_refxml, "lelat", m_lelat);
	get_widget(m_refxml, "hedisp", m_hedisp);
	get_widget(m_refxml, "ledisp", m_ledisp);
	get_widget(m_refxml, "heelev", m_heelev);
	get_widget(m_refxml, "leelev", m_leelev);
	m_buttonapply->signal_clicked().connect(sigc::mem_fun(*this, &AirportEditor::RunwayDimensions::buttonapply_clicked));
	m_buttoncancel->signal_clicked().connect(sigc::mem_fun(*this, &AirportEditor::RunwayDimensions::buttoncancel_clicked));
	m_buttonok->signal_clicked().connect(sigc::mem_fun(*this, &AirportEditor::RunwayDimensions::buttonok_clicked));
	buttonradio_clicked(indep_length);
	m_heident->signal_focus_out_event().connect(sigc::bind_return(sigc::hide(sigc::mem_fun(*this, &AirportEditor::RunwayDimensions::edited_heident)), false));
	m_leident->signal_focus_out_event().connect(sigc::bind_return(sigc::hide(sigc::mem_fun(*this, &AirportEditor::RunwayDimensions::edited_leident)), false));
	m_width->signal_focus_out_event().connect(sigc::bind_return(sigc::hide(sigc::mem_fun(*this, &AirportEditor::RunwayDimensions::edited_width)), false));
	m_length->signal_focus_out_event().connect(sigc::bind_return(sigc::hide(sigc::mem_fun(*this, &AirportEditor::RunwayDimensions::edited_length)), false));
	m_heading->signal_focus_out_event().connect(sigc::bind_return(sigc::hide(sigc::mem_fun(*this, &AirportEditor::RunwayDimensions::edited_heading)), false));
	m_centerlon->signal_focus_out_event().connect(sigc::bind_return(sigc::hide(sigc::mem_fun(*this, &AirportEditor::RunwayDimensions::edited_centerlon)), false));
	m_centerlat->signal_focus_out_event().connect(sigc::bind_return(sigc::hide(sigc::mem_fun(*this, &AirportEditor::RunwayDimensions::edited_centerlat)), false));
	m_helon->signal_focus_out_event().connect(sigc::bind_return(sigc::hide(sigc::mem_fun(*this, &AirportEditor::RunwayDimensions::edited_helon)), false));
	m_helat->signal_focus_out_event().connect(sigc::bind_return(sigc::hide(sigc::mem_fun(*this, &AirportEditor::RunwayDimensions::edited_helat)), false));
	m_lelon->signal_focus_out_event().connect(sigc::bind_return(sigc::hide(sigc::mem_fun(*this, &AirportEditor::RunwayDimensions::edited_lelon)), false));
	m_lelat->signal_focus_out_event().connect(sigc::bind_return(sigc::hide(sigc::mem_fun(*this, &AirportEditor::RunwayDimensions::edited_lelat)), false));
	m_hedisp->signal_focus_out_event().connect(sigc::bind_return(sigc::hide(sigc::mem_fun(*this, &AirportEditor::RunwayDimensions::edited_hedisp)), false));
	m_ledisp->signal_focus_out_event().connect(sigc::bind_return(sigc::hide(sigc::mem_fun(*this, &AirportEditor::RunwayDimensions::edited_ledisp)), false));
	m_heelev->signal_focus_out_event().connect(sigc::bind_return(sigc::hide(sigc::mem_fun(*this, &AirportEditor::RunwayDimensions::edited_heelev)), false));
	m_leelev->signal_focus_out_event().connect(sigc::bind_return(sigc::hide(sigc::mem_fun(*this, &AirportEditor::RunwayDimensions::edited_leelev)), false));
}

AirportEditor::RunwayDimensions::~RunwayDimensions()
{
}

Glib::RefPtr<AirportEditor::RunwayDimensions> AirportEditor::RunwayDimensions::create(void)
{
	Glib::RefPtr<Builder> refxml = load_glade_file("dbeditor" UIEXT, "runwaydimensiondialog");
	RunwayDimensions *runwaydim;
	get_widget_derived(refxml, "runwaydimensiondialog", runwaydim);
	runwaydim->hide();
	return Glib::RefPtr<RunwayDimensions>(runwaydim);
}

void AirportEditor::RunwayDimensions::set_runwayinfo(const AirportsDb::Airport::Runway & rwy, const Point & arp)
{
	m_rwy = rwy;
	m_center = m_arp = arp;
	m_heident->set_text(m_rwy.get_ident_he());
	m_leident->set_text(m_rwy.get_ident_le());
	m_width->set_text(numtotext(m_rwy.get_width()));
	m_length->set_text(numtotext(m_rwy.get_length()));
	m_heading->set_text(angletotext(m_rwy.get_he_hdg()));;
	m_centerlon->set_text(m_center.get_lon_str());
	m_centerlat->set_text(m_center.get_lat_str());
	m_helon->set_text(m_rwy.get_he_coord().get_lon_str());
	m_helat->set_text(m_rwy.get_he_coord().get_lat_str());
	m_lelon->set_text(m_rwy.get_le_coord().get_lon_str());
	m_lelat->set_text(m_rwy.get_le_coord().get_lat_str());
	m_hedisp->set_text(numtotext(m_rwy.get_he_disp()));
	m_ledisp->set_text(numtotext(m_rwy.get_le_disp()));
	m_heelev->set_text(numtotext(m_rwy.get_he_elev()));
	m_leelev->set_text(numtotext(m_rwy.get_le_elev()));
}

void AirportEditor::RunwayDimensions::buttonapply_clicked(void)
{
	m_cb.emit(m_rwy);
}

void AirportEditor::RunwayDimensions::buttoncancel_clicked(void)
{
	hide();
}

void AirportEditor::RunwayDimensions::buttonok_clicked(void)
{
	buttonapply_clicked();
	buttonok_clicked();
}

void AirportEditor::RunwayDimensions::buttonradio_clicked(indep_t bnr)
{
	{
		unsigned int cm = m_radiomask & ((1 << indep_centercoord) | (1 << indep_hecoord) | (1 << indep_lecoord));
		unsigned int nrc = ((cm >> indep_centercoord) & 1) + ((cm >> indep_hecoord) & 1) + ((cm >> indep_lecoord) & 1);
		std::cerr << "AirportEditor::RunwayDimensions::buttonradio_clicked: bnr " << bnr
			  << " radiomask: 0x" << std::hex << m_radiomask << " nrc " << nrc << std::endl;
	}
	switch (bnr) {
		case indep_length:
		case indep_heading:
		{
			m_radiomask |= (1 << indep_length) | (1 << indep_heading);
			unsigned int cm = m_radiomask & ((1 << indep_centercoord) | (1 << indep_hecoord) | (1 << indep_lecoord));
			unsigned int nrc = ((cm >> indep_centercoord) & 1) + ((cm >> indep_hecoord) & 1) + ((cm >> indep_lecoord) & 1);
			if (nrc == 1)
				break;
			if (!nrc) {
				m_radiomask |= (1 << indep_centercoord);
				break;
			}
			m_radiomask &= ~((1 << indep_centercoord) | (1 << indep_hecoord) | (1 << indep_lecoord));
			m_radiomask |= (1 << (ffs(cm) - 1));
			break;
		}

		case indep_centercoord:
		case indep_hecoord:
		case indep_lecoord:
		{
			unsigned int cm = m_radiomask & ((1 << indep_centercoord) | (1 << indep_hecoord) | (1 << indep_lecoord));
			unsigned int nrc = ((cm >> indep_centercoord) & 1) + ((cm >> indep_hecoord) & 1) + ((cm >> indep_lecoord) & 1);
			if (nrc == 1 && !(cm & (1 << bnr))) {
				m_radiomask &= ~((1 << indep_length) | (1 << indep_heading));
				m_radiomask |= (1 << bnr);
				break;
			}
			if (nrc == 2) {
				if (cm & (1 << bnr)) {
					m_radiomask |= ((1 << indep_length) | (1 << indep_heading));
					m_radiomask &= ~(1 << bnr);
				} else if (bnr == indep_centercoord) {
					m_radiomask = (1 << indep_hecoord) | (1 << indep_centercoord);
				} else {
					m_radiomask = (1 << indep_hecoord) | (1 << indep_lecoord);
				}
				break;
			}
			if (nrc == 3) {
				m_radiomask &= ~((1 << indep_length) | (1 << indep_heading) | (1 << bnr));
				break;
			}
			m_radiomask = (1 << indep_length) | (1 << indep_heading) | (1 << indep_centercoord);
			break;
		}
		default:
			m_radiomask = (1 << indep_length) | (1 << indep_heading) | (1 << indep_centercoord);
	}
	for (int i = indep_length; i <= indep_lecoord; ++i)
		m_radioconn[i].disconnect();
	for (int i = indep_length; i <= indep_lecoord; ++i)
		m_radio[i]->set_active(!!(m_radiomask & (1 << i)));
	for (int i = indep_length; i <= indep_lecoord; ++i)
		m_radioconn[i] = m_radio[i]->signal_clicked().connect(sigc::bind(sigc::mem_fun(*this, &AirportEditor::RunwayDimensions::buttonradio_clicked), (indep_t)i));
}

void AirportEditor::RunwayDimensions::set_independent(indep_t nr)
{
	if (m_radiomask & (1 << nr))
		return;
	buttonradio_clicked(nr);
}

void AirportEditor::RunwayDimensions::compute_ldatda(void)
{
	m_rwy.set_he_lda(m_rwy.get_length() - m_rwy.get_he_disp());
	m_rwy.set_he_tda(m_rwy.get_length() - m_rwy.get_le_disp());
	m_rwy.set_le_lda(m_rwy.get_length() - m_rwy.get_le_disp());
	m_rwy.set_le_tda(m_rwy.get_length() - m_rwy.get_he_disp());
}

void AirportEditor::RunwayDimensions::compute_dependent(void)
{
	bool sethdglength(false);
	if (m_radiomask & ((1 << indep_length) | (1 << indep_heading))) {
		if (m_radiomask & (1 << indep_centercoord)) {
			float ang(m_rwy.get_he_hdg() * FPlanLeg::to_deg);
			float dist(m_rwy.get_length() * (Point::ft_to_m / 2000.0 * Point::km_to_nmi));
			m_rwy.set_he_coord(m_center.spheric_course_distance_nmi(ang + M_PI, dist));
			m_rwy.set_le_coord(m_center.spheric_course_distance_nmi(ang, dist));
			m_helon->set_text(m_rwy.get_he_coord().get_lon_str());
			m_helat->set_text(m_rwy.get_he_coord().get_lat_str());
			m_lelon->set_text(m_rwy.get_le_coord().get_lon_str());
			m_lelat->set_text(m_rwy.get_le_coord().get_lat_str());
		} else if (m_radiomask & (1 << indep_hecoord)) {
			float ang((m_rwy.get_he_hdg() + 0x8000) * FPlanLeg::to_deg);
			float dist(m_rwy.get_length() * (Point::ft_to_m / 2000.0 * Point::km_to_nmi));
			m_center = m_rwy.get_he_coord().spheric_course_distance_nmi(ang, dist);
			m_rwy.set_le_coord(m_rwy.get_he_coord().spheric_course_distance_nmi(ang, ldexp(dist, 1)));
			m_centerlon->set_text(m_center.get_lon_str());
			m_centerlat->set_text(m_center.get_lat_str());
			m_lelon->set_text(m_rwy.get_le_coord().get_lon_str());
			m_lelat->set_text(m_rwy.get_le_coord().get_lat_str());
		} else if (m_radiomask & (1 << indep_lecoord)) {
			float ang(m_rwy.get_he_hdg() * FPlanLeg::to_deg);
			float dist(m_rwy.get_length() * (Point::ft_to_m / 2000.0 * Point::km_to_nmi));
			m_center = m_rwy.get_le_coord().spheric_course_distance_nmi(ang, dist);
			m_rwy.set_he_coord(m_rwy.get_le_coord().spheric_course_distance_nmi(ang, ldexp(dist, 1)));
			m_centerlon->set_text(m_center.get_lon_str());
			m_centerlat->set_text(m_center.get_lat_str());
			m_helon->set_text(m_rwy.get_he_coord().get_lon_str());
			m_helat->set_text(m_rwy.get_he_coord().get_lat_str());
		}
	} else if (m_radiomask == ((1 << indep_hecoord) | (1 << indep_lecoord))) {
		m_center = m_rwy.get_le_coord().halfway(m_rwy.get_he_coord());
		sethdglength = true;
		m_centerlon->set_text(m_center.get_lon_str());
		m_centerlat->set_text(m_center.get_lat_str());
	} else if (m_radiomask == ((1 << indep_centercoord) | (1 << indep_lecoord))) {
		m_rwy.set_he_coord(m_center + (m_center - m_rwy.get_le_coord()));
		sethdglength = true;
		m_helon->set_text(m_rwy.get_he_coord().get_lon_str());
		m_helat->set_text(m_rwy.get_he_coord().get_lat_str());
	} else if (m_radiomask == ((1 << indep_centercoord) | (1 << indep_hecoord))) {
		m_rwy.set_le_coord(m_center + (m_center - m_rwy.get_he_coord()));
		sethdglength = true;
		m_lelon->set_text(m_rwy.get_le_coord().get_lon_str());
		m_lelat->set_text(m_rwy.get_le_coord().get_lat_str());
	}
	if (sethdglength) {
		set_heading(Point::round<int,float>(m_rwy.get_he_coord().spheric_true_course(m_rwy.get_le_coord()) * FPlanLeg::from_deg));
		m_rwy.set_length(Point::round<int,float>(m_rwy.get_he_coord().spheric_distance_km(m_rwy.get_le_coord()) * (1000.0 * Point::m_to_ft)));
		m_length->set_text(numtotext(m_rwy.get_length()));
	}
	compute_ldatda();
}

void AirportEditor::RunwayDimensions::edited_heident(void)
{
	m_rwy.set_ident_he(m_heident->get_text());
}

void AirportEditor::RunwayDimensions::edited_leident(void)
{
	m_rwy.set_ident_le(m_leident->get_text());
}

void AirportEditor::RunwayDimensions::edited_width(void)
{
	m_rwy.set_width(Conversions::unsigned_feet_parse(m_width->get_text()));
	m_width->set_text(numtotext(m_rwy.get_width()));
}

void AirportEditor::RunwayDimensions::edited_length(void)
{
	m_rwy.set_length(Conversions::unsigned_feet_parse(m_length->get_text()));
	m_length->set_text(numtotext(m_rwy.get_length()));
	set_independent(indep_length);
	compute_dependent();
}

void AirportEditor::RunwayDimensions::set_heading(uint16_t ang)
{
	m_rwy.set_he_hdg(ang);
	m_rwy.set_le_hdg(ang + 0x8000);
	m_heading->set_text(angletotext(ang));
	{
		WMM wmm;
		if (wmm.compute(m_rwy.get_he_elev() * (Point::ft_to_m * 1e-3), m_center, time(0)))
			ang += (uint32_t)(wmm.get_dec() * FPlanLeg::from_deg);
	}
	{
		Glib::ustring s(m_rwy.get_ident_he());
		while (!s.empty() && Glib::Unicode::isdigit(s[0]))
			s.erase(s.begin());
		std::ostringstream oss;
		oss << Point::round<int,float>(ang * (FPlanLeg::to_deg * 0.1));
		s = oss.str() + s;
		m_rwy.set_ident_he(s);
		m_heident->set_text(s);
	}
	{
		Glib::ustring s(m_rwy.get_ident_he());
		while (!s.empty() && Glib::Unicode::isdigit(s[0]))
			s.erase(s.begin());
		std::ostringstream oss;
		oss << Point::round<int,float>(((ang + 0x8000) & 0xffff) * (FPlanLeg::to_deg * 0.1));
		s = oss.str() + s;
		m_rwy.set_ident_le(s);
		m_leident->set_text(s);
	}
}

void AirportEditor::RunwayDimensions::edited_heading(void)
{
	set_heading(texttoangle(m_heading->get_text()));
	set_independent(indep_heading);
	compute_dependent();
}

void AirportEditor::RunwayDimensions::edited_centerlon(void)
{
	m_center.set_lon_str(m_centerlon->get_text());
	m_centerlon->set_text(m_center.get_lon_str());
	set_independent(indep_centercoord);
	compute_dependent();
}

void AirportEditor::RunwayDimensions::edited_centerlat(void)
{
	m_center.set_lat_str(m_centerlat->get_text());
	m_centerlat->set_text(m_center.get_lat_str());
	set_independent(indep_centercoord);
	compute_dependent();
}

void AirportEditor::RunwayDimensions::edited_helon(void)
{
	Point pt(m_rwy.get_he_coord());
	pt.set_lon_str(m_helon->get_text());
	m_rwy.set_he_coord(pt);
	m_helon->set_text(m_rwy.get_he_coord().get_lon_str());
	set_independent(indep_hecoord);
	compute_dependent();
}

void AirportEditor::RunwayDimensions::edited_helat(void)
{
	Point pt(m_rwy.get_he_coord());
	pt.set_lat_str(m_helat->get_text());
	m_rwy.set_he_coord(pt);
	m_helat->set_text(m_rwy.get_he_coord().get_lat_str());
	set_independent(indep_hecoord);
	compute_dependent();
}

void AirportEditor::RunwayDimensions::edited_lelon(void)
{
	Point pt(m_rwy.get_le_coord());
	pt.set_lon_str(m_lelon->get_text());
	m_rwy.set_le_coord(pt);
	m_lelon->set_text(m_rwy.get_le_coord().get_lon_str());
	set_independent(indep_lecoord);
	compute_dependent();
}

void AirportEditor::RunwayDimensions::edited_lelat(void)
{
	Point pt(m_rwy.get_le_coord());
	pt.set_lat_str(m_lelat->get_text());
	m_rwy.set_le_coord(pt);
	m_lelat->set_text(m_rwy.get_le_coord().get_lat_str());
	set_independent(indep_lecoord);
	compute_dependent();
}

void AirportEditor::RunwayDimensions::edited_hedisp(void)
{
	m_rwy.set_he_disp(Conversions::unsigned_feet_parse(m_hedisp->get_text()));
	m_hedisp->set_text(numtotext(m_rwy.get_he_disp()));
	compute_ldatda();
}

void AirportEditor::RunwayDimensions::edited_ledisp(void)
{
	m_rwy.set_le_disp(Conversions::unsigned_feet_parse(m_ledisp->get_text()));
	m_ledisp->set_text(numtotext(m_rwy.get_le_disp()));
	compute_ldatda();
}

void AirportEditor::RunwayDimensions::edited_heelev(void)
{
	m_rwy.set_he_elev(Conversions::unsigned_feet_parse(m_heelev->get_text()));
	m_heelev->set_text(numtotext(m_rwy.get_he_elev()));
}

void AirportEditor::RunwayDimensions::edited_leelev(void)
{
	m_rwy.set_le_elev(Conversions::unsigned_feet_parse(m_leelev->get_text()));
	m_leelev->set_text(numtotext(m_rwy.get_le_elev()));
}

AirportEditor::AsyncElevUpdate::AsyncElevUpdate(Engine& eng, AirportsDbQueryInterface& db, const AirportsDb::Address& addr, const sigc::slot<void,AirportsDb::Airport&>& save)
	: m_db(db), m_addr(addr), m_save(save), m_refcnt(2), m_asyncref(true)
{
	std::cerr << "AirportEditor::AsyncElevUpdate::AsyncElevUpdate: " << (unsigned long long)this << std::endl;
	AirportsDb::Airport e(m_db(m_addr));
	if (!e.is_valid())
		return;
	if (e.get_coord().is_invalid())
		return;
	m_dispatch.connect(sigc::mem_fun(*this, &AirportEditor::AsyncElevUpdate::callback));
	m_async = eng.async_elevation_point(e.get_coord());
	if (m_async) {
		m_async->connect(sigc::mem_fun(*this, &AirportEditor::AsyncElevUpdate::callback_dispatch));
	} else {
		m_asyncref = false;
		unreference();
	}
}

AirportEditor::AsyncElevUpdate::~AsyncElevUpdate()
{
	cancel();
	std::cerr << "AirportEditor::AsyncElevUpdate::~AsyncElevUpdate: " << (unsigned long long)this << std::endl;
}

void AirportEditor::AsyncElevUpdate::reference(void)
{
	++m_refcnt;
}

void AirportEditor::AsyncElevUpdate::unreference(void)
{
	if (!(--m_refcnt))
		delete this;
}

void AirportEditor::AsyncElevUpdate::cancel(void)
{
	Engine::ElevationMinMaxResult::cancel(m_async);
	if (m_asyncref) {
		m_asyncref = false;
		unreference();
	}
}

bool AirportEditor::AsyncElevUpdate::is_done(void)
{
	return !m_async;
}

void AirportEditor::AsyncElevUpdate::callback_dispatch(void)
{
	m_dispatch();
}

void AirportEditor::AsyncElevUpdate::callback(void)
{
	if (!m_async)
		return;
	if (!m_async->is_done())
		return;
	if (!m_async->is_error()) {
		AirportsDb::Airport e(m_db(m_addr));
		if (e.is_valid()) {
			e.set_modtime(time(0));
			TopoDb30::elev_t elev(m_async->get_result());
			std::cerr << "elev: " << elev << std::endl;
			if (elev != TopoDb30::nodata) {
				e.set_elevation(Point::round<int16_t,float>(elev * Point::m_to_ft));
				m_save(e);
			}
		}
	}
	cancel();
}

AirportEditor::AirportEditor(const std::string & dir_main, Engine::auxdb_mode_t auxdbmode, const std::string & dir_aux)
	: m_dbchanged(false), m_engine(dir_main, auxdbmode, dir_aux, false, false), m_runwaydim(0), m_airporteditorwindow(0), m_airporteditortreeview(0), m_airporteditorstatusbar(0),
	  m_airporteditorfind(0), m_airporteditornotebook(0), m_airporteditorvfrremark(0),
	  m_airportrunwayeditortreeview(0), m_airporthelipadeditortreeview(0), m_airportcommeditortreeview(0),
	  m_airportvfrrouteseditortreeview(0), m_airportvfrrouteeditortreeview(0),
	  m_airportvfrrouteeditorappend(0), m_airportvfrrouteeditorinsert(0), m_airportvfrrouteeditordelete(0), m_airportvfrrouteeditorpointlist(0),
	  m_airporteditormap(0), m_airporteditorundo(0), m_airporteditorredo(0),
	  m_airporteditormapenable(0), m_airporteditorairportdiagramenable(0), m_airporteditormapzoomin(0), m_airporteditormapzoomout(0),
	  m_airporteditornewairport(0), m_airporteditordeleteairport(0), m_airporteditorairportsetelev(0),
	  m_airporteditornewrunway(0), m_airporteditordeleterunway(0), m_airporteditorrunwaydim(0),
	  m_airporteditorrunwayhetole(0), m_airporteditorrunwayletohe(0), m_airporteditorrunwaycoords(0),
	  m_airporteditornewhelipad(0), m_airporteditordeletehelipad(0),
	  m_airporteditornewcomm(0), m_airporteditordeletecomm(0),
	  m_airporteditornewvfrroute(0), m_airporteditordeletevfrroute(0),
	  m_airporteditorduplicatevfrroute(0), m_airporteditorreversevfrroute(0),
	  m_airporteditornamevfrroute(0), m_airporteditorduplicatereverseandnameallvfrroutes(0),
	  m_airporteditorappendvfrroutepoint(0), m_airporteditorinsertvfrroutepoint(0), m_airporteditordeletevfrroutepoint(0),
	  m_airporteditorselectall(0), m_airporteditorunselectall(0), m_aboutdialog(0),
	  m_curentryid(0), m_vfrroute_index(-1)
{
#ifdef HAVE_PQXX
	if (auxdbmode == Engine::auxdb_postgres)
		m_db.reset(new AirportsPGDb(dir_main));
	else
#endif
	{
		m_db.reset(new AirportsDb(dir_main));
		std::cerr << "Main database " << (dir_main.empty() ? std::string("(default)") : dir_main) << std::endl;
		Glib::ustring dir_aux1(m_engine.get_aux_dir(auxdbmode, dir_aux));
		AirportsDb *db(static_cast<AirportsDb *>(m_db.get()));
		if (!dir_aux1.empty() && db->is_dbfile_exists(dir_aux1))
			db->attach(dir_aux1);
		if (db->is_aux_attached())
			std::cerr << "Auxillary airports database " << dir_aux1 << " attached" << std::endl;
	}
	m_runwaydim = AirportEditor::RunwayDimensions::create();
	m_runwaydim->signal_set().connect(sigc::mem_fun(*this, &AirportEditor::edited_rwy_dimensions));
	m_refxml = load_glade_file("dbeditor" UIEXT, "airporteditorwindow");
	get_widget_derived(m_refxml, "airporteditorwindow", m_airporteditorwindow);
	get_widget(m_refxml, "airporteditortreeview", m_airporteditortreeview);
	get_widget(m_refxml, "airporteditorstatusbar", m_airporteditorstatusbar);
	get_widget(m_refxml, "airporteditorfind", m_airporteditorfind);
	get_widget(m_refxml, "airporteditornotebook", m_airporteditornotebook);
	get_widget(m_refxml, "airporteditorvfrremark", m_airporteditorvfrremark);
	get_widget(m_refxml, "airportrunwayeditortreeview", m_airportrunwayeditortreeview);
	get_widget(m_refxml, "airporthelipadeditortreeview", m_airporthelipadeditortreeview);
	get_widget(m_refxml, "airportcommeditortreeview", m_airportcommeditortreeview);
	get_widget(m_refxml, "airportvfrrouteseditortreeview", m_airportvfrrouteseditortreeview);
	get_widget(m_refxml, "airportvfrrouteeditortreeview", m_airportvfrrouteeditortreeview);
	get_widget(m_refxml, "airportvfrrouteeditorappend", m_airportvfrrouteeditorappend);
	get_widget(m_refxml, "airportvfrrouteeditorinsert", m_airportvfrrouteeditorinsert);
	get_widget(m_refxml, "airportvfrrouteeditordelete", m_airportvfrrouteeditordelete);
	get_widget(m_refxml, "airportvfrrouteeditorpointlist", m_airportvfrrouteeditorpointlist);
	get_widget_derived(m_refxml, "airporteditormap", m_airporteditormap);
	{
#ifdef HAVE_GTKMM2
		Glib::RefPtr<Builder> refxml = load_glade_file_nosearch(m_refxml->get_filename(), "aboutdialog");
#else
		//Glib::RefPtr<Builder> refxml = load_glade_file("dbeditor" UIEXT, "aboutdialog");
		Glib::RefPtr<Builder> refxml = m_refxml;
#endif
		get_widget(refxml, "aboutdialog", m_aboutdialog);
		m_aboutdialog->signal_response().connect(sigc::mem_fun(*this, &AirportEditor::aboutdialog_response));
	}
	Gtk::Button *buttonclear(0);
	get_widget(m_refxml, "airporteditorclearbutton", buttonclear);
	buttonclear->signal_clicked().connect(sigc::mem_fun(*this, &AirportEditor::buttonclear_clicked));
	m_airporteditorfind->signal_changed().connect(sigc::mem_fun(*this, &AirportEditor::find_changed));
	Gtk::MenuItem *menu_quit(0), *menu_cut(0), *menu_copy(0), *menu_paste(0), *menu_delete(0);
	Gtk::MenuItem *menu_preferences(0), *menu_about(0);
	get_widget(m_refxml, "airporteditorquit", menu_quit);
	get_widget(m_refxml, "airporteditorundo", m_airporteditorundo);
	get_widget(m_refxml, "airporteditorredo", m_airporteditorredo);
	get_widget(m_refxml, "airporteditornewairport", m_airporteditornewairport);
	get_widget(m_refxml, "airporteditordeleteairport", m_airporteditordeleteairport);
	get_widget(m_refxml, "airporteditorairportsetelev", m_airporteditorairportsetelev);
	get_widget(m_refxml, "airporteditormapenable", m_airporteditormapenable);
	get_widget(m_refxml, "airporteditorairportdiagramenable", m_airporteditorairportdiagramenable);
	get_widget(m_refxml, "airporteditormapzoomin", m_airporteditormapzoomin);
	get_widget(m_refxml, "airporteditormapzoomout", m_airporteditormapzoomout);
	get_widget(m_refxml, "airporteditornewrunway", m_airporteditornewrunway);
	get_widget(m_refxml, "airporteditordeleterunway", m_airporteditordeleterunway);
	get_widget(m_refxml, "airporteditorrunwaydim", m_airporteditorrunwaydim);
	get_widget(m_refxml, "airporteditorrunwayhetole", m_airporteditorrunwayhetole);
	get_widget(m_refxml, "airporteditorrunwayletohe", m_airporteditorrunwayletohe);
        get_widget(m_refxml, "airporteditorrunwaycoords", m_airporteditorrunwaycoords);
	get_widget(m_refxml, "airporteditornewhelipad", m_airporteditornewhelipad);
	get_widget(m_refxml, "airporteditordeletehelipad", m_airporteditordeletehelipad);
	get_widget(m_refxml, "airporteditornewcomm", m_airporteditornewcomm);
	get_widget(m_refxml, "airporteditordeletecomm", m_airporteditordeletecomm);
	get_widget(m_refxml, "airporteditornewvfrroute", m_airporteditornewvfrroute);
	get_widget(m_refxml, "airporteditordeletevfrroute", m_airporteditordeletevfrroute);
	get_widget(m_refxml, "airporteditorduplicatevfrroute", m_airporteditorduplicatevfrroute);
	get_widget(m_refxml, "airporteditorreversevfrroute", m_airporteditorreversevfrroute);
	get_widget(m_refxml, "airporteditornamevfrroute", m_airporteditornamevfrroute);
	get_widget(m_refxml, "airporteditorduplicatereverseandnameallvfrroutes", m_airporteditorduplicatereverseandnameallvfrroutes);
	get_widget(m_refxml, "airporteditorappendvfrroutepoint", m_airporteditorappendvfrroutepoint);
	get_widget(m_refxml, "airporteditorinsertvfrroutepoint", m_airporteditorinsertvfrroutepoint);
	get_widget(m_refxml, "airporteditordeletevfrroutepoint", m_airporteditordeletevfrroutepoint);
	get_widget(m_refxml, "airporteditorcut", menu_cut);
	get_widget(m_refxml, "airporteditorcopy", menu_copy);
	get_widget(m_refxml, "airporteditorpaste", menu_paste);
	get_widget(m_refxml, "airporteditordelete", menu_delete);
	get_widget(m_refxml, "airporteditorselectall", m_airporteditorselectall);
	get_widget(m_refxml, "airporteditorunselectall", m_airporteditorunselectall);
	get_widget(m_refxml, "airporteditorpreferences", menu_preferences);
	get_widget(m_refxml, "airporteditorabout", menu_about);
	hide_notebook_pages();
	menu_quit->signal_activate().connect(sigc::mem_fun(*this, &AirportEditor::menu_quit_activate));
	m_airporteditorundo->signal_activate().connect(sigc::mem_fun(*this, &AirportEditor::menu_undo_activate));
	m_airporteditorredo->signal_activate().connect(sigc::mem_fun(*this, &AirportEditor::menu_redo_activate));
	m_airporteditornewairport->signal_activate().connect(sigc::mem_fun(*this, &AirportEditor::menu_newairport_activate));
	m_airporteditordeleteairport->signal_activate().connect(sigc::mem_fun(*this, &AirportEditor::menu_deleteairport_activate));
	m_airporteditorairportsetelev->signal_activate().connect(sigc::mem_fun(*this, &AirportEditor::menu_airportsetelev_activate));
	m_airporteditormapenable->signal_toggled().connect(sigc::mem_fun(*this, &AirportEditor::menu_mapenable_toggled));
	m_airporteditorairportdiagramenable->signal_toggled().connect(sigc::mem_fun(*this, &AirportEditor::menu_airportdiagramenable_toggled));
	m_airporteditormapzoomin->signal_activate().connect(sigc::mem_fun(*this, &AirportEditor::menu_mapzoomin_activate));
	m_airporteditormapzoomout->signal_activate().connect(sigc::mem_fun(*this, &AirportEditor::menu_mapzoomout_activate));
	if (m_airporteditormapenable->get_active()) {
		m_airporteditormapzoomin->show();
		m_airporteditormapzoomout->show();
	} else {
		m_airporteditormapzoomin->hide();
		m_airporteditormapzoomout->hide();
	}
	m_airporteditorwindow->signal_zoomin().connect(sigc::bind_return(sigc::mem_fun(*this, &AirportEditor::menu_mapzoomin_activate), true));
	m_airporteditorwindow->signal_zoomout().connect(sigc::bind_return(sigc::mem_fun(*this, &AirportEditor::menu_mapzoomout_activate), true));
	menu_cut->signal_activate().connect(sigc::mem_fun(*this, &AirportEditor::menu_cut_activate));
	menu_copy->signal_activate().connect(sigc::mem_fun(*this, &AirportEditor::menu_copy_activate));
	menu_paste->signal_activate().connect(sigc::mem_fun(*this, &AirportEditor::menu_paste_activate));
	menu_delete->signal_activate().connect(sigc::mem_fun(*this, &AirportEditor::menu_delete_activate));
	m_airporteditorselectall->signal_activate().connect(sigc::mem_fun(*this, &AirportEditor::menu_selectall_activate));
	m_airporteditorunselectall->signal_activate().connect(sigc::mem_fun(*this, &AirportEditor::menu_unselectall_activate));
	m_airporteditornewrunway->signal_activate().connect(sigc::mem_fun(*this, &AirportEditor::menu_newrunway_activate));
	m_airporteditordeleterunway->signal_activate().connect(sigc::mem_fun(*this, &AirportEditor::menu_deleterunway_activate));
	m_airporteditorrunwaydim->signal_activate().connect(sigc::mem_fun(*this, &AirportEditor::menu_runwaydim_activate));
	m_airporteditorrunwayhetole->signal_activate().connect(sigc::mem_fun(*this, &AirportEditor::menu_runwayhetole_activate));
	m_airporteditorrunwayletohe->signal_activate().connect(sigc::mem_fun(*this, &AirportEditor::menu_runwayletohe_activate));
	m_airporteditorrunwaycoords->signal_activate().connect(sigc::mem_fun(*this, &AirportEditor::menu_runwaycoords_activate));
	m_airporteditornewhelipad->signal_activate().connect(sigc::mem_fun(*this, &AirportEditor::menu_newhelipad_activate));
	m_airporteditordeletehelipad->signal_activate().connect(sigc::mem_fun(*this, &AirportEditor::menu_deletehelipad_activate));
	m_airporteditornewcomm->signal_activate().connect(sigc::mem_fun(*this, &AirportEditor::menu_newcomm_activate));
	m_airporteditordeletecomm->signal_activate().connect(sigc::mem_fun(*this, &AirportEditor::menu_deletecomm_activate));
	m_airporteditornewvfrroute->signal_activate().connect(sigc::mem_fun(*this, &AirportEditor::menu_newvfrroute_activate));
	m_airporteditordeletevfrroute->signal_activate().connect(sigc::mem_fun(*this, &AirportEditor::menu_deletevfrroute_activate));
	m_airporteditorduplicatevfrroute->signal_activate().connect(sigc::mem_fun(*this, &AirportEditor::menu_duplicatevfrroute_activate));
	m_airporteditorreversevfrroute->signal_activate().connect(sigc::mem_fun(*this, &AirportEditor::menu_reversevfrroute_activate));
	m_airporteditornamevfrroute->signal_activate().connect(sigc::mem_fun(*this, &AirportEditor::menu_namevfrroute_activate));
	m_airporteditorduplicatereverseandnameallvfrroutes->signal_activate().connect(sigc::mem_fun(*this, &AirportEditor::menu_duplicatereverseandnameallvfrroutes_activate));
	m_airporteditorappendvfrroutepoint->signal_activate().connect(sigc::mem_fun(*this, &AirportEditor::menu_appendvfrroutepoint_activate));
	m_airporteditorinsertvfrroutepoint->signal_activate().connect(sigc::mem_fun(*this, &AirportEditor::menu_insertvfrroutepoint_activate));
	m_airporteditordeletevfrroutepoint->signal_activate().connect(sigc::mem_fun(*this, &AirportEditor::menu_deletevfrroutepoint_activate));
	m_airportvfrrouteeditorappend->signal_clicked().connect(sigc::mem_fun(*this, &AirportEditor::menu_appendvfrroutepoint_activate));
	m_airportvfrrouteeditorinsert->signal_clicked().connect(sigc::mem_fun(*this, &AirportEditor::menu_insertvfrroutepoint_activate));
	m_airportvfrrouteeditordelete->signal_clicked().connect(sigc::mem_fun(*this, &AirportEditor::menu_deletevfrroutepoint_activate));
	menu_preferences->signal_activate().connect(sigc::mem_fun(*this, &AirportEditor::menu_prefs_activate));
	menu_about->signal_activate().connect(sigc::mem_fun(*this, &AirportEditor::menu_about_activate));
	update_undoredo_menu();
	// Vector Map
	m_airporteditormap->set_engine(m_engine);
	m_airporteditormap->set_renderer(VectorMapArea::renderer_vmap);
	m_airporteditormap->signal_cursor().connect(sigc::mem_fun(*this, &AirportEditor::map_cursor));
	m_airporteditormap->set_map_scale(m_engine.get_prefs().mapscale);
	AirportEditor::map_drawflags(m_engine.get_prefs().mapflags);
	m_engine.get_prefs().mapflags.signal_change().connect(sigc::mem_fun(*this, &AirportEditor::map_drawflags));
	// Airport List
	m_airportliststore = Gtk::ListStore::create(m_airportlistcolumns);
	m_airporteditortreeview->set_model(m_airportliststore);
	Gtk::CellRendererText *icao_renderer = Gtk::manage(new Gtk::CellRendererText());
	Gtk::TreeViewColumn *icao_column = m_airporteditortreeview->get_column(m_airporteditortreeview->append_column(_("ICAO"), *icao_renderer) - 1);
	if (icao_column) {
		icao_column->add_attribute(*icao_renderer, "text", m_airportlistcolumns.m_col_icao);
		icao_column->set_sort_column(m_airportlistcolumns.m_col_icao);
	}
	icao_renderer->signal_edited().connect(sigc::mem_fun(*this, &AirportEditor::edited_icao));
	icao_renderer->set_property("editable", true);
	Gtk::CellRendererText *name_renderer = Gtk::manage(new Gtk::CellRendererText());
	Gtk::TreeViewColumn *name_column = m_airporteditortreeview->get_column(m_airporteditortreeview->append_column(_("Name"), *name_renderer) - 1);
	if (name_column) {
		name_column->add_attribute(*name_renderer, "text", m_airportlistcolumns.m_col_name);
		name_column->set_sort_column(m_airportlistcolumns.m_col_name);
	}
	name_renderer->signal_edited().connect(sigc::mem_fun(*this, &AirportEditor::edited_name));
	name_renderer->set_property("editable", true);
	CoordCellRenderer *coord_renderer = Gtk::manage(new CoordCellRenderer());
	Gtk::TreeViewColumn *coord_column = m_airporteditortreeview->get_column(m_airporteditortreeview->append_column(_("Lon"), *coord_renderer) - 1);
	if (coord_column) {
		coord_column->add_attribute(*coord_renderer, coord_renderer->get_property_name(), m_airportlistcolumns.m_col_coord);
		coord_column->set_sort_column(m_airportlistcolumns.m_col_coord);
	}
	coord_renderer->set_property("editable", true);
	coord_renderer->signal_edited().connect(sigc::mem_fun(*this, &AirportEditor::edited_coord));
	LabelPlacementRenderer *label_placement_renderer = Gtk::manage(new LabelPlacementRenderer());
	Gtk::TreeViewColumn *label_placement_column = m_airporteditortreeview->get_column(m_airporteditortreeview->append_column(_("Label"), *label_placement_renderer) - 1);
	if (label_placement_column) {
		label_placement_column->add_attribute(*label_placement_renderer, label_placement_renderer->get_property_name(), m_airportlistcolumns.m_col_label_placement);
		label_placement_column->set_sort_column(m_airportlistcolumns.m_col_label_placement);
	}
	label_placement_renderer->set_property("editable", true);
	label_placement_renderer->signal_edited().connect(sigc::mem_fun(*this, &AirportEditor::edited_label_placement));
	Gtk::CellRendererText *elev_renderer = Gtk::manage(new Gtk::CellRendererText());
	Gtk::TreeViewColumn *elev_column = m_airporteditortreeview->get_column(m_airporteditortreeview->append_column(_("Elevation"), *elev_renderer) - 1);
	if (elev_column) {
		elev_column->add_attribute(*elev_renderer, "text", m_airportlistcolumns.m_col_elev);
		elev_column->set_sort_column(m_airportlistcolumns.m_col_elev);
	}
	elev_renderer->set_property("xalign", 1.0);
	elev_renderer->signal_edited().connect(sigc::mem_fun(*this, &AirportEditor::edited_elev));
	elev_renderer->set_property("editable", true);
	AirportTypeRenderer *airporttype_renderer = Gtk::manage(new AirportTypeRenderer());
	Gtk::TreeViewColumn *airporttype_column = m_airporteditortreeview->get_column(m_airporteditortreeview->append_column(_("Type"), *airporttype_renderer) - 1);
	if (airporttype_column) {
		airporttype_column->add_attribute(*airporttype_renderer, airporttype_renderer->get_property_name(), m_airportlistcolumns.m_col_typecode);
		airporttype_column->set_sort_column(m_airportlistcolumns.m_col_typecode);
	}
	airporttype_renderer->set_property("editable", true);
	airporttype_renderer->signal_edited().connect(sigc::mem_fun(*this, &AirportEditor::edited_typecode));
	Gtk::CellRendererToggle *airportfrules_arr_vfr_renderer = Gtk::manage(new Gtk::CellRendererToggle());
	Gtk::TreeViewColumn *airportfrules_arr_vfr_column = m_airporteditortreeview->get_column(m_airporteditortreeview->append_column(_("ARR VFR"), *airportfrules_arr_vfr_renderer) - 1);
	if (airportfrules_arr_vfr_column) {
		airportfrules_arr_vfr_column->add_attribute(*airportfrules_arr_vfr_renderer, "active", m_airportlistcolumns.m_col_frules_arr_vfr);
		airportfrules_arr_vfr_column->set_sort_column(m_airportlistcolumns.m_col_frules_arr_vfr);
	}
	airportfrules_arr_vfr_renderer->signal_toggled().connect(sigc::bind(sigc::mem_fun(*this, &AirportEditor::edited_frules), AirportsDb::Airport::flightrules_arr_vfr));
	airportfrules_arr_vfr_renderer->set_property("editable", true);
	Gtk::CellRendererToggle *airportfrules_arr_ifr_renderer = Gtk::manage(new Gtk::CellRendererToggle());
	Gtk::TreeViewColumn *airportfrules_arr_ifr_column = m_airporteditortreeview->get_column(m_airporteditortreeview->append_column(_("ARR IFR"), *airportfrules_arr_ifr_renderer) - 1);
	if (airportfrules_arr_ifr_column) {
		airportfrules_arr_ifr_column->add_attribute(*airportfrules_arr_ifr_renderer, "active", m_airportlistcolumns.m_col_frules_arr_ifr);
		airportfrules_arr_ifr_column->set_sort_column(m_airportlistcolumns.m_col_frules_arr_ifr);
	}
	airportfrules_arr_ifr_renderer->signal_toggled().connect(sigc::bind(sigc::mem_fun(*this, &AirportEditor::edited_frules), AirportsDb::Airport::flightrules_arr_ifr));
	airportfrules_arr_ifr_renderer->set_property("editable", true);
	Gtk::CellRendererToggle *airportfrules_dep_vfr_renderer = Gtk::manage(new Gtk::CellRendererToggle());
	Gtk::TreeViewColumn *airportfrules_dep_vfr_column = m_airporteditortreeview->get_column(m_airporteditortreeview->append_column(_("DEP VFR"), *airportfrules_dep_vfr_renderer) - 1);
	if (airportfrules_dep_vfr_column) {
		airportfrules_dep_vfr_column->add_attribute(*airportfrules_dep_vfr_renderer, "active", m_airportlistcolumns.m_col_frules_dep_vfr);
		airportfrules_dep_vfr_column->set_sort_column(m_airportlistcolumns.m_col_frules_dep_vfr);
	}
	airportfrules_dep_vfr_renderer->signal_toggled().connect(sigc::bind(sigc::mem_fun(*this, &AirportEditor::edited_frules), AirportsDb::Airport::flightrules_dep_vfr));
	airportfrules_dep_vfr_renderer->set_property("editable", true);
	Gtk::CellRendererToggle *airportfrules_dep_ifr_renderer = Gtk::manage(new Gtk::CellRendererToggle());
	Gtk::TreeViewColumn *airportfrules_dep_ifr_column = m_airporteditortreeview->get_column(m_airporteditortreeview->append_column(_("DEP IFR"), *airportfrules_dep_ifr_renderer) - 1);
	if (airportfrules_dep_ifr_column) {
		airportfrules_dep_ifr_column->add_attribute(*airportfrules_dep_ifr_renderer, "active", m_airportlistcolumns.m_col_frules_dep_ifr);
		airportfrules_dep_ifr_column->set_sort_column(m_airportlistcolumns.m_col_frules_dep_ifr);
	}
	airportfrules_dep_ifr_renderer->signal_toggled().connect(sigc::bind(sigc::mem_fun(*this, &AirportEditor::edited_frules), AirportsDb::Airport::flightrules_dep_ifr));
	airportfrules_dep_ifr_renderer->set_property("editable", true);
  	Gtk::CellRendererText *srcid_renderer = Gtk::manage(new Gtk::CellRendererText());
	Gtk::TreeViewColumn *srcid_column = m_airporteditortreeview->get_column(m_airporteditortreeview->append_column(_("Source ID"), *srcid_renderer) - 1);
	if (srcid_column) {
		srcid_renderer->signal_edited().connect(sigc::mem_fun(*this, &AirportEditor::edited_sourceid));
		srcid_column->add_attribute(*srcid_renderer, "text", m_airportlistcolumns.m_col_sourceid);
		srcid_column->set_sort_column(m_airportlistcolumns.m_col_sourceid);
	}
	srcid_renderer->signal_edited().connect(sigc::mem_fun(*this, &AirportEditor::edited_sourceid));
	srcid_renderer->set_property("editable", true);
	DateTimeCellRenderer *modtime_renderer = Gtk::manage(new DateTimeCellRenderer());
	Gtk::TreeViewColumn *modtime_column = m_airporteditortreeview->get_column(m_airporteditortreeview->append_column(_("Mod Time"), *modtime_renderer) - 1);
	if (modtime_column) {
		modtime_column->add_attribute(*modtime_renderer, modtime_renderer->get_property_name(), m_airportlistcolumns.m_col_modtime);
		modtime_column->set_sort_column(m_airportlistcolumns.m_col_modtime);
	}
	modtime_renderer->set_property("editable", true);
	modtime_renderer->signal_edited().connect(sigc::mem_fun(*this, &AirportEditor::edited_modtime));
	//m_airporteditortreeview->append_column_editable(_("Mod Time"), m_airportlistcolumns.m_col_modtime);
	for (unsigned int i = 0; i < 13; ++i) {
		Gtk::TreeViewColumn *col = m_airporteditortreeview->get_column(i);
		if (!col)
			continue;
		col->set_resizable(true);
		col->set_reorderable(true);
	}
	m_airporteditortreeview->columns_autosize();
	m_airporteditortreeview->set_enable_search(true);
	m_airporteditortreeview->set_search_column(m_airportlistcolumns.m_col_name);
	// selection
	Glib::RefPtr<Gtk::TreeSelection> airporteditor_selection = m_airporteditortreeview->get_selection();
	airporteditor_selection->set_mode(Gtk::SELECTION_MULTIPLE);
	airporteditor_selection->signal_changed().connect(sigc::mem_fun(*this, &AirportEditor::airporteditor_selection_changed));
	//airporteditor_selection->set_select_function(sigc::mem_fun(*this, &AirportEditor::airporteditor_select_function));
	// steup notebook pages
	m_airporteditorvfrremark->signal_focus_out_event().connect(sigc::mem_fun(*this, &AirportEditor::edited_vfrremark_focus));
	m_airportrunway_model = Gtk::ListStore::create(m_airportrunway_model_columns);
	m_airporthelipad_model = Gtk::ListStore::create(m_airporthelipad_model_columns);
	m_airportcomm_model = Gtk::ListStore::create(m_airportcomm_model_columns);
	m_airportvfrroute_model = Gtk::ListStore::create(m_airportvfrroute_model_columns);
	m_airportvfrroutes_model = Gtk::ListStore::create(m_airportvfrroutes_model_columns);
	m_airportrunwayeditortreeview->set_model(m_airportrunway_model);
	m_airporthelipadeditortreeview->set_model(m_airporthelipad_model);
	m_airportcommeditortreeview->set_model(m_airportcomm_model);
	m_airportvfrrouteeditortreeview->set_model(m_airportvfrroute_model);
	m_airportvfrrouteseditortreeview->set_model(m_airportvfrroutes_model);
	// Runway Treeview
  	Gtk::CellRendererText *rwy_identhe_renderer = Gtk::manage(new Gtk::CellRendererText());
	Gtk::TreeViewColumn *rwy_identhe_column = m_airportrunwayeditortreeview->get_column(m_airportrunwayeditortreeview->append_column(_("Ident HE"), *rwy_identhe_renderer) - 1);
	if (rwy_identhe_column) {
		rwy_identhe_column->add_attribute(*rwy_identhe_renderer, "text", m_airportrunway_model_columns.m_col_ident_he);
		rwy_identhe_column->set_sort_column(m_airportrunway_model_columns.m_col_ident_he);
	}
	rwy_identhe_renderer->set_property("xalign", 0.0);
	rwy_identhe_renderer->signal_edited().connect(sigc::mem_fun(*this, &AirportEditor::edited_rwy_identhe));		
	rwy_identhe_renderer->set_property("editable", true);
  	Gtk::CellRendererText *rwy_identle_renderer = Gtk::manage(new Gtk::CellRendererText());
	Gtk::TreeViewColumn *rwy_identle_column = m_airportrunwayeditortreeview->get_column(m_airportrunwayeditortreeview->append_column(_("Ident LE"), *rwy_identle_renderer) - 1);
	if (rwy_identle_column) {
		rwy_identle_column->add_attribute(*rwy_identle_renderer, "text", m_airportrunway_model_columns.m_col_ident_le);
		rwy_identle_column->set_sort_column(m_airportrunway_model_columns.m_col_ident_le);
	}
	rwy_identle_renderer->set_property("xalign", 0.0);
	rwy_identle_renderer->signal_edited().connect(sigc::mem_fun(*this, &AirportEditor::edited_rwy_identle));
	rwy_identle_renderer->set_property("editable", true);
  	Gtk::CellRendererText *rwy_length_renderer = Gtk::manage(new Gtk::CellRendererText());
	Gtk::TreeViewColumn *rwy_length_column = m_airportrunwayeditortreeview->get_column(m_airportrunwayeditortreeview->append_column(_("Length"), *rwy_length_renderer) - 1);
	if (rwy_length_column) {
		rwy_length_column->add_attribute(*rwy_length_renderer, "text", m_airportrunway_model_columns.m_col_length);
		rwy_length_column->set_sort_column(m_airportrunway_model_columns.m_col_length);
	}
	rwy_length_renderer->set_property("xalign", 1.0);
	rwy_length_renderer->signal_edited().connect(sigc::mem_fun(*this, &AirportEditor::edited_rwy_length));
	rwy_length_renderer->set_property("editable", true);
  	Gtk::CellRendererText *rwy_width_renderer = Gtk::manage(new Gtk::CellRendererText());
	Gtk::TreeViewColumn *rwy_width_column = m_airportrunwayeditortreeview->get_column(m_airportrunwayeditortreeview->append_column(_("Width"), *rwy_width_renderer) - 1);
	if (rwy_width_column) {
	     	rwy_width_column->add_attribute(*rwy_width_renderer, "text", m_airportrunway_model_columns.m_col_width);
		rwy_width_column->set_sort_column(m_airportrunway_model_columns.m_col_width);
	}
	rwy_width_renderer->set_property("xalign", 1.0);
	rwy_width_renderer->signal_edited().connect(sigc::mem_fun(*this, &AirportEditor::edited_rwy_width));
	rwy_width_renderer->set_property("editable", true);
  	Gtk::CellRendererText *rwy_surface_renderer = Gtk::manage(new Gtk::CellRendererText());
	Gtk::TreeViewColumn *rwy_surface_column = m_airportrunwayeditortreeview->get_column(m_airportrunwayeditortreeview->append_column(_("Surface"), *rwy_surface_renderer) - 1);
	if (rwy_surface_column) {
	     	rwy_surface_column->add_attribute(*rwy_surface_renderer, "text", m_airportrunway_model_columns.m_col_surface);
		rwy_surface_column->set_sort_column(m_airportrunway_model_columns.m_col_surface);
	}
	rwy_surface_renderer->set_property("xalign", 1.0);
	rwy_surface_renderer->signal_edited().connect(sigc::mem_fun(*this, &AirportEditor::edited_rwy_surface));
	rwy_surface_renderer->set_property("editable", true);
	CoordCellRenderer *rwy_he_coord_renderer = Gtk::manage(new CoordCellRenderer());
	Gtk::TreeViewColumn *rwy_he_coord_column = m_airportrunwayeditortreeview->get_column(m_airportrunwayeditortreeview->append_column(_("HE Coordinate"), *rwy_he_coord_renderer) - 1);
	if (rwy_he_coord_column) {
		rwy_he_coord_column->add_attribute(*rwy_he_coord_renderer, rwy_he_coord_renderer->get_property_name(), m_airportrunway_model_columns.m_col_he_coord);
		rwy_he_coord_column->set_sort_column(m_airportrunway_model_columns.m_col_he_coord);
	}
	rwy_he_coord_renderer->set_property("editable", true);
	rwy_he_coord_renderer->signal_edited().connect(sigc::mem_fun(*this, &AirportEditor::edited_rwy_hecoord));
  	Gtk::CellRendererText *rwy_he_tda_renderer = Gtk::manage(new Gtk::CellRendererText());
	Gtk::TreeViewColumn *rwy_he_tda_column = m_airportrunwayeditortreeview->get_column(m_airportrunwayeditortreeview->append_column(_("HE TDA"), *rwy_he_tda_renderer) - 1);
	if (rwy_he_tda_column) {
	      	rwy_he_tda_column->add_attribute(*rwy_he_tda_renderer, "text", m_airportrunway_model_columns.m_col_he_tda);
		rwy_he_tda_column->set_sort_column(m_airportrunway_model_columns.m_col_he_tda);
	}
	rwy_he_tda_renderer->set_property("xalign", 1.0);
	rwy_he_tda_renderer->signal_edited().connect(sigc::mem_fun(*this, &AirportEditor::edited_rwy_hetda));
	rwy_he_tda_renderer->set_property("editable", true);
  	Gtk::CellRendererText *rwy_he_lda_renderer = Gtk::manage(new Gtk::CellRendererText());
	Gtk::TreeViewColumn *rwy_he_lda_column = m_airportrunwayeditortreeview->get_column(m_airportrunwayeditortreeview->append_column(_("HE LDA"), *rwy_he_lda_renderer) - 1);
	if (rwy_he_lda_column) {
	       	rwy_he_lda_column->add_attribute(*rwy_he_lda_renderer, "text", m_airportrunway_model_columns.m_col_he_lda);
		rwy_he_lda_column->set_sort_column(m_airportrunway_model_columns.m_col_he_lda);
	}
	rwy_he_lda_renderer->set_property("xalign", 1.0);
	rwy_he_lda_renderer->signal_edited().connect(sigc::mem_fun(*this, &AirportEditor::edited_rwy_helda));
	rwy_he_lda_renderer->set_property("editable", true);
  	Gtk::CellRendererText *rwy_he_disp_renderer = Gtk::manage(new Gtk::CellRendererText());
	Gtk::TreeViewColumn *rwy_he_disp_column = m_airportrunwayeditortreeview->get_column(m_airportrunwayeditortreeview->append_column(_("HE Disp"), *rwy_he_disp_renderer) - 1);
	if (rwy_he_disp_column) {
		rwy_he_disp_column->add_attribute(*rwy_he_disp_renderer, "text", m_airportrunway_model_columns.m_col_he_disp);
		rwy_he_disp_column->set_sort_column(m_airportrunway_model_columns.m_col_he_disp);
	}
	rwy_he_disp_renderer->set_property("xalign", 1.0);
	rwy_he_disp_renderer->signal_edited().connect(sigc::mem_fun(*this, &AirportEditor::edited_rwy_hedisp));
	rwy_he_disp_renderer->set_property("editable", true);
	AngleCellRenderer *rwy_he_hdg_renderer = Gtk::manage(new AngleCellRenderer());
	Gtk::TreeViewColumn *rwy_he_hdg_column = m_airportrunwayeditortreeview->get_column(m_airportrunwayeditortreeview->append_column(_("HE Hdg"), *rwy_he_hdg_renderer) - 1);
	if (rwy_he_hdg_column) {
		rwy_he_hdg_column->add_attribute(*rwy_he_hdg_renderer, rwy_he_hdg_renderer->get_property_name(), m_airportrunway_model_columns.m_col_he_hdg);
		rwy_he_hdg_column->set_sort_column(m_airportrunway_model_columns.m_col_he_hdg);
	}
	rwy_he_hdg_renderer->set_property("editable", true);
	rwy_he_hdg_renderer->signal_edited().connect(sigc::mem_fun(*this, &AirportEditor::edited_rwy_hehdg));
  	Gtk::CellRendererText *rwy_he_elev_renderer = Gtk::manage(new Gtk::CellRendererText());
	Gtk::TreeViewColumn *rwy_he_elev_column = m_airportrunwayeditortreeview->get_column(m_airportrunwayeditortreeview->append_column(_("HE Elev"), *rwy_he_elev_renderer) - 1);
	if (rwy_he_elev_column) {
	    	rwy_he_elev_column->add_attribute(*rwy_he_elev_renderer, "text", m_airportrunway_model_columns.m_col_he_elev);
		rwy_he_elev_column->set_sort_column(m_airportrunway_model_columns.m_col_he_elev);
	}
	rwy_he_elev_renderer->set_property("xalign", 1.0);
	rwy_he_elev_renderer->signal_edited().connect(sigc::mem_fun(*this, &AirportEditor::edited_rwy_heelev));
	rwy_he_elev_renderer->set_property("editable", true);
	CoordCellRenderer *rwy_le_coord_renderer = Gtk::manage(new CoordCellRenderer());
	Gtk::TreeViewColumn *rwy_le_coord_column = m_airportrunwayeditortreeview->get_column(m_airportrunwayeditortreeview->append_column(_("LE Coordinate"), *rwy_le_coord_renderer) - 1);
	if (rwy_le_coord_column) {
		rwy_le_coord_column->add_attribute(*rwy_le_coord_renderer, rwy_le_coord_renderer->get_property_name(), m_airportrunway_model_columns.m_col_le_coord);
		rwy_le_coord_column->set_sort_column(m_airportrunway_model_columns.m_col_le_coord);
	}
	rwy_le_coord_renderer->set_property("editable", true);
	rwy_le_coord_renderer->signal_edited().connect(sigc::mem_fun(*this, &AirportEditor::edited_rwy_lecoord));
  	Gtk::CellRendererText *rwy_le_tda_renderer = Gtk::manage(new Gtk::CellRendererText());
	Gtk::TreeViewColumn *rwy_le_tda_column = m_airportrunwayeditortreeview->get_column(m_airportrunwayeditortreeview->append_column(_("LE TDA"), *rwy_le_tda_renderer) - 1);
	if (rwy_le_tda_column) {
	  	rwy_le_tda_column->add_attribute(*rwy_le_tda_renderer, "text", m_airportrunway_model_columns.m_col_le_tda);
		rwy_le_tda_column->set_sort_column(m_airportrunway_model_columns.m_col_le_tda);
	}
	rwy_le_tda_renderer->set_property("xalign", 1.0);
	rwy_le_tda_renderer->signal_edited().connect(sigc::mem_fun(*this, &AirportEditor::edited_rwy_letda));
	rwy_le_tda_renderer->set_property("editable", true);
  	Gtk::CellRendererText *rwy_le_lda_renderer = Gtk::manage(new Gtk::CellRendererText());
	Gtk::TreeViewColumn *rwy_le_lda_column = m_airportrunwayeditortreeview->get_column(m_airportrunwayeditortreeview->append_column(_("LE LDA"), *rwy_le_lda_renderer) - 1);
	if (rwy_le_lda_column) {
		rwy_le_lda_column->add_attribute(*rwy_le_lda_renderer, "text", m_airportrunway_model_columns.m_col_le_lda);
		rwy_le_lda_column->set_sort_column(m_airportrunway_model_columns.m_col_le_lda);
	}
	rwy_le_lda_renderer->set_property("xalign", 1.0);
	rwy_le_lda_renderer->signal_edited().connect(sigc::mem_fun(*this, &AirportEditor::edited_rwy_lelda));
	rwy_le_lda_renderer->set_property("editable", true);
  	Gtk::CellRendererText *rwy_le_disp_renderer = Gtk::manage(new Gtk::CellRendererText());
	Gtk::TreeViewColumn *rwy_le_disp_column = m_airportrunwayeditortreeview->get_column(m_airportrunwayeditortreeview->append_column(_("LE Disp"), *rwy_le_disp_renderer) - 1);
	if (rwy_le_disp_column) {
	  	rwy_le_disp_column->add_attribute(*rwy_le_disp_renderer, "text", m_airportrunway_model_columns.m_col_le_disp);
		rwy_le_disp_column->set_sort_column(m_airportrunway_model_columns.m_col_le_disp);
	}
	rwy_le_disp_renderer->set_property("xalign", 1.0);
	rwy_le_disp_renderer->signal_edited().connect(sigc::mem_fun(*this, &AirportEditor::edited_rwy_ledisp));
	rwy_le_disp_renderer->set_property("editable", true);
	AngleCellRenderer *rwy_le_hdg_renderer = Gtk::manage(new AngleCellRenderer());
	Gtk::TreeViewColumn *rwy_le_hdg_column = m_airportrunwayeditortreeview->get_column(m_airportrunwayeditortreeview->append_column(_("LE Hdg"), *rwy_le_hdg_renderer) - 1);
	if (rwy_le_hdg_column) {
		rwy_le_hdg_column->add_attribute(*rwy_le_hdg_renderer, rwy_le_hdg_renderer->get_property_name(), m_airportrunway_model_columns.m_col_le_hdg);
		rwy_le_hdg_column->set_sort_column(m_airportrunway_model_columns.m_col_le_hdg);
	}
	rwy_le_hdg_renderer->set_property("editable", true);
	rwy_le_hdg_renderer->signal_edited().connect(sigc::mem_fun(*this, &AirportEditor::edited_rwy_lehdg));
  	Gtk::CellRendererText *rwy_le_elev_renderer = Gtk::manage(new Gtk::CellRendererText());
	Gtk::TreeViewColumn *rwy_le_elev_column = m_airportrunwayeditortreeview->get_column(m_airportrunwayeditortreeview->append_column(_("LE Elev"), *rwy_le_elev_renderer) - 1);
	if (rwy_le_elev_column) {
    		rwy_le_elev_column->add_attribute(*rwy_le_elev_renderer, "text", m_airportrunway_model_columns.m_col_le_elev);
		rwy_le_elev_column->set_sort_column(m_airportrunway_model_columns.m_col_le_elev);
	}
	rwy_le_elev_renderer->set_property("xalign", 1.0);
	rwy_le_elev_renderer->signal_edited().connect(sigc::mem_fun(*this, &AirportEditor::edited_rwy_leelev));
	rwy_le_elev_renderer->set_property("editable", true);
  	Gtk::CellRendererToggle *rwy_he_usable_renderer = Gtk::manage(new Gtk::CellRendererToggle());
	Gtk::TreeViewColumn *rwy_he_usable_column = m_airportrunwayeditortreeview->get_column(m_airportrunwayeditortreeview->append_column(_("HE"), *rwy_he_usable_renderer) - 1);
	if (rwy_he_usable_column) {
		rwy_he_usable_column->add_attribute(*rwy_he_usable_renderer, "active", m_airportrunway_model_columns.m_col_flags_he_usable);
		rwy_he_usable_column->set_sort_column(m_airportrunway_model_columns.m_col_flags_he_usable);
	}
	rwy_he_usable_renderer->set_property("xalign", 1.0);
	rwy_he_usable_renderer->signal_toggled().connect(sigc::bind(sigc::mem_fun(*this, &AirportEditor::edited_rwy_flags), AirportsDb::Airport::Runway::flag_he_usable));
	rwy_he_usable_renderer->set_property("editable", true);
  	Gtk::CellRendererToggle *rwy_le_usable_renderer = Gtk::manage(new Gtk::CellRendererToggle());
	Gtk::TreeViewColumn *rwy_le_usable_column = m_airportrunwayeditortreeview->get_column(m_airportrunwayeditortreeview->append_column(_("LE"), *rwy_le_usable_renderer) - 1);
	if (rwy_le_usable_column) {
	   	rwy_le_usable_column->add_attribute(*rwy_le_usable_renderer, "active", m_airportrunway_model_columns.m_col_flags_le_usable);
		rwy_le_usable_column->set_sort_column(m_airportrunway_model_columns.m_col_flags_le_usable);
	}
	rwy_le_usable_renderer->set_property("xalign", 1.0);
	rwy_le_usable_renderer->signal_toggled().connect(sigc::bind(sigc::mem_fun(*this, &AirportEditor::edited_rwy_flags), AirportsDb::Airport::Runway::flag_le_usable));
	rwy_le_usable_renderer->set_property("editable", true);
	for (unsigned int idx = 0; idx < 8; ++idx) {
		AirportRunwayLightRenderer *rwy_he_light_renderer = Gtk::manage(new AirportRunwayLightRenderer());
		std::ostringstream oss;
		oss << _("HE Light ") << (idx + 1);
		Gtk::TreeViewColumn *rwy_he_light_column = m_airportrunwayeditortreeview->get_column(m_airportrunwayeditortreeview->append_column(oss.str(), *rwy_he_light_renderer) - 1);
		if (rwy_he_light_column) {
			rwy_he_light_column->add_attribute(*rwy_he_light_renderer, rwy_he_light_renderer->get_property_name(), m_airportrunway_model_columns.m_col_he_light[idx]);
			rwy_he_light_column->set_sort_column(m_airportrunway_model_columns.m_col_he_light[idx]);
		}
		rwy_he_light_renderer->set_property("editable", true);
		rwy_he_light_renderer->signal_edited().connect(sigc::bind(sigc::mem_fun(*this, &AirportEditor::edited_rwy_helight), idx));
	}
	for (unsigned int idx = 0; idx < 8; ++idx) {
		AirportRunwayLightRenderer *rwy_le_light_renderer = Gtk::manage(new AirportRunwayLightRenderer());
		std::ostringstream oss;
		oss << _("LE Light ") << (idx + 1);
		Gtk::TreeViewColumn *rwy_le_light_column = m_airportrunwayeditortreeview->get_column(m_airportrunwayeditortreeview->append_column(oss.str(), *rwy_le_light_renderer) - 1);
		if (rwy_le_light_column) {
			rwy_le_light_column->add_attribute(*rwy_le_light_renderer, rwy_le_light_renderer->get_property_name(), m_airportrunway_model_columns.m_col_le_light[idx]);
			rwy_le_light_column->set_sort_column(m_airportrunway_model_columns.m_col_le_light[idx]);
		}
		rwy_le_light_renderer->set_property("editable", true);
		rwy_le_light_renderer->signal_edited().connect(sigc::bind(sigc::mem_fun(*this, &AirportEditor::edited_rwy_lelight), idx));
	}
	for (unsigned int i = 0; i < 37; ++i) {
		Gtk::TreeViewColumn *col = m_airportrunwayeditortreeview->get_column(i);
		if (!col)
			continue;
		col->set_resizable(true);
		col->set_reorderable(true);
	}
	m_airportrunwayeditortreeview->columns_autosize();
	m_airportrunwayeditortreeview->set_enable_search(true);
	m_airportrunwayeditortreeview->set_search_column(m_airportrunway_model_columns.m_col_ident_he);
	// Helipad Treeview
  	Gtk::CellRendererText *hp_ident_renderer = Gtk::manage(new Gtk::CellRendererText());
	Gtk::TreeViewColumn *hp_ident_column = m_airporthelipadeditortreeview->get_column(m_airporthelipadeditortreeview->append_column(_("Ident HE"), *hp_ident_renderer) - 1);
	if (hp_ident_column) {
	     	hp_ident_column->add_attribute(*hp_ident_renderer, "text", m_airporthelipad_model_columns.m_col_ident);
		hp_ident_column->set_sort_column(m_airporthelipad_model_columns.m_col_ident);
	}
	hp_ident_renderer->set_property("xalign", 0.0);
	hp_ident_renderer->signal_edited().connect(sigc::mem_fun(*this, &AirportEditor::edited_hp_ident));
	hp_ident_renderer->set_property("editable", true);
   	Gtk::CellRendererText *hp_length_renderer = Gtk::manage(new Gtk::CellRendererText());
	Gtk::TreeViewColumn *hp_length_column = m_airporthelipadeditortreeview->get_column(m_airporthelipadeditortreeview->append_column(_("Length"), *hp_length_renderer) - 1);
	if (hp_length_column) {
	  	hp_length_column->add_attribute(*hp_length_renderer, "text", m_airporthelipad_model_columns.m_col_length);
		hp_length_column->set_sort_column(m_airporthelipad_model_columns.m_col_length);
	}
	hp_length_renderer->set_property("xalign", 1.0);
	hp_length_renderer->signal_edited().connect(sigc::mem_fun(*this, &AirportEditor::edited_hp_length));
	hp_length_renderer->set_property("editable", true);
  	Gtk::CellRendererText *hp_width_renderer = Gtk::manage(new Gtk::CellRendererText());
	Gtk::TreeViewColumn *hp_width_column = m_airporthelipadeditortreeview->get_column(m_airporthelipadeditortreeview->append_column(_("Width"), *hp_width_renderer) - 1);
	if (hp_width_column) {
  		hp_width_column->add_attribute(*hp_width_renderer, "text", m_airporthelipad_model_columns.m_col_width);
		hp_width_column->set_sort_column(m_airporthelipad_model_columns.m_col_width);
	}
	hp_width_renderer->set_property("xalign", 1.0);
	hp_width_renderer->signal_edited().connect(sigc::mem_fun(*this, &AirportEditor::edited_hp_width));
	hp_width_renderer->set_property("editable", true);
  	Gtk::CellRendererText *hp_surface_renderer = Gtk::manage(new Gtk::CellRendererText());
	Gtk::TreeViewColumn *hp_surface_column = m_airporthelipadeditortreeview->get_column(m_airporthelipadeditortreeview->append_column(_("Surface"), *hp_surface_renderer) - 1);
	if (hp_surface_column) {
   		hp_surface_column->add_attribute(*hp_surface_renderer, "text", m_airporthelipad_model_columns.m_col_surface);
		hp_surface_column->set_sort_column(m_airporthelipad_model_columns.m_col_surface);
	}
	hp_surface_renderer->set_property("xalign", 1.0);
	hp_surface_renderer->signal_edited().connect(sigc::mem_fun(*this, &AirportEditor::edited_hp_surface));
	hp_surface_renderer->set_property("editable", true);
	CoordCellRenderer *hp_coord_renderer = Gtk::manage(new CoordCellRenderer());
	Gtk::TreeViewColumn *hp_coord_column = m_airporthelipadeditortreeview->get_column(m_airporthelipadeditortreeview->append_column(_("Coordinate"), *hp_coord_renderer) - 1);
	if (hp_coord_column) {
		hp_coord_column->add_attribute(*hp_coord_renderer, hp_coord_renderer->get_property_name(), m_airporthelipad_model_columns.m_col_coord);
		hp_coord_column->set_sort_column(m_airporthelipad_model_columns.m_col_coord);
	}
	hp_coord_renderer->set_property("editable", true);
	hp_coord_renderer->signal_edited().connect(sigc::mem_fun(*this, &AirportEditor::edited_hp_coord));
	AngleCellRenderer *hp_hdg_renderer = Gtk::manage(new AngleCellRenderer());
	Gtk::TreeViewColumn *hp_hdg_column = m_airporthelipadeditortreeview->get_column(m_airporthelipadeditortreeview->append_column(_("Hdg"), *hp_hdg_renderer) - 1);
	if (hp_hdg_column) {
		hp_hdg_column->add_attribute(*hp_hdg_renderer, hp_hdg_renderer->get_property_name(), m_airporthelipad_model_columns.m_col_hdg);
		hp_hdg_column->set_sort_column(m_airporthelipad_model_columns.m_col_hdg);
	}
	hp_hdg_renderer->set_property("editable", true);
	hp_hdg_renderer->signal_edited().connect(sigc::mem_fun(*this, &AirportEditor::edited_hp_hdg));
  	Gtk::CellRendererText *hp_elev_renderer = Gtk::manage(new Gtk::CellRendererText());
	Gtk::TreeViewColumn *hp_elev_column = m_airporthelipadeditortreeview->get_column(m_airporthelipadeditortreeview->append_column(_("Elev"), *hp_elev_renderer) - 1);
	if (hp_elev_column) {
	    	hp_elev_column->add_attribute(*hp_elev_renderer, "text", m_airporthelipad_model_columns.m_col_elev);
		hp_elev_column->set_sort_column(m_airporthelipad_model_columns.m_col_elev);
	}
	hp_elev_renderer->set_property("xalign", 1.0);
	hp_elev_renderer->signal_edited().connect(sigc::mem_fun(*this, &AirportEditor::edited_hp_elev));
	hp_elev_renderer->set_property("editable", true);
  	Gtk::CellRendererToggle *hp_usable_renderer = Gtk::manage(new Gtk::CellRendererToggle());
	Gtk::TreeViewColumn *hp_usable_column = m_airporthelipadeditortreeview->get_column(m_airporthelipadeditortreeview->append_column(_("Usable"), *hp_usable_renderer) - 1);
	if (hp_usable_column) {
	    	hp_usable_column->add_attribute(*hp_usable_renderer, "active", m_airporthelipad_model_columns.m_col_flags_usable);
		hp_usable_column->set_sort_column(m_airporthelipad_model_columns.m_col_flags_usable);
	}
	hp_usable_renderer->set_property("xalign", 1.0);
	hp_usable_renderer->signal_toggled().connect(sigc::bind(sigc::mem_fun(*this, &AirportEditor::edited_hp_flags), AirportsDb::Airport::Helipad::flag_usable));
	hp_usable_renderer->set_property("editable", true);
	for (unsigned int i = 0; i < 8; ++i) {
		Gtk::TreeViewColumn *col = m_airporthelipadeditortreeview->get_column(i);
		if (!col)
			continue;
		col->set_resizable(true);
		col->set_reorderable(true);
	}
	m_airporthelipadeditortreeview->columns_autosize();
	m_airporthelipadeditortreeview->set_enable_search(true);
	m_airporthelipadeditortreeview->set_search_column(m_airporthelipad_model_columns.m_col_ident);
	 // Comm Treeview
  	Gtk::CellRendererText *comm_name_renderer = Gtk::manage(new Gtk::CellRendererText());
	Gtk::TreeViewColumn *comm_name_column = m_airportcommeditortreeview->get_column(m_airportcommeditortreeview->append_column(_("Name"), *comm_name_renderer) - 1);
	if (comm_name_column) {
		comm_name_column->add_attribute(*comm_name_renderer, "text", m_airportcomm_model_columns.m_col_name);
		comm_name_column->set_sort_column(m_airportcomm_model_columns.m_col_name);
	}
	comm_name_renderer->signal_edited().connect(sigc::mem_fun(*this, &AirportEditor::edited_comm_name));
	comm_name_renderer->set_property("editable", true);
	FrequencyCellRenderer *comm_freq0_renderer = Gtk::manage(new FrequencyCellRenderer());
	Gtk::TreeViewColumn *comm_freq0_column = m_airportcommeditortreeview->get_column(m_airportcommeditortreeview->append_column(_("Freq 0"), *comm_freq0_renderer) - 1);
	if (comm_freq0_column) {
		comm_freq0_column->add_attribute(*comm_freq0_renderer, comm_freq0_renderer->get_property_name(), m_airportcomm_model_columns.m_col_freq0);
		comm_freq0_column->set_sort_column(m_airportcomm_model_columns.m_col_freq0);
	}
	comm_freq0_renderer->set_property("editable", true);
	comm_freq0_renderer->signal_edited().connect(sigc::mem_fun(*this, &AirportEditor::edited_comm_freq0));
	comm_freq0_renderer->set_property("xalign", 1.0);
	FrequencyCellRenderer *comm_freq1_renderer = Gtk::manage(new FrequencyCellRenderer());
	Gtk::TreeViewColumn *comm_freq1_column = m_airportcommeditortreeview->get_column(m_airportcommeditortreeview->append_column(_("Freq 1"), *comm_freq1_renderer) - 1);
	if (comm_freq1_column) {
		comm_freq1_column->add_attribute(*comm_freq1_renderer, comm_freq1_renderer->get_property_name(), m_airportcomm_model_columns.m_col_freq1);
		comm_freq1_column->set_sort_column(m_airportcomm_model_columns.m_col_freq1);
	}
	comm_freq1_renderer->set_property("editable", true);
	comm_freq1_renderer->signal_edited().connect(sigc::mem_fun(*this, &AirportEditor::edited_comm_freq1));
	comm_freq1_renderer->set_property("xalign", 1.0);
	FrequencyCellRenderer *comm_freq2_renderer = Gtk::manage(new FrequencyCellRenderer());
	Gtk::TreeViewColumn *comm_freq2_column = m_airportcommeditortreeview->get_column(m_airportcommeditortreeview->append_column(_("Freq 2"), *comm_freq2_renderer) - 1);
	if (comm_freq2_column) {
		comm_freq2_column->add_attribute(*comm_freq2_renderer, comm_freq2_renderer->get_property_name(), m_airportcomm_model_columns.m_col_freq2);
		comm_freq2_column->set_sort_column(m_airportcomm_model_columns.m_col_freq2);
	}
	comm_freq2_renderer->set_property("editable", true);
	comm_freq2_renderer->signal_edited().connect(sigc::mem_fun(*this, &AirportEditor::edited_comm_freq2));
	comm_freq2_renderer->set_property("xalign", 1.0);
	FrequencyCellRenderer *comm_freq3_renderer = Gtk::manage(new FrequencyCellRenderer());
	Gtk::TreeViewColumn *comm_freq3_column = m_airportcommeditortreeview->get_column(m_airportcommeditortreeview->append_column(_("Freq 3"), *comm_freq3_renderer) - 1);
	if (comm_freq3_column) {
		comm_freq3_column->add_attribute(*comm_freq3_renderer, comm_freq3_renderer->get_property_name(), m_airportcomm_model_columns.m_col_freq3);
		comm_freq3_column->set_sort_column(m_airportcomm_model_columns.m_col_freq3);
	}
	comm_freq3_renderer->set_property("editable", true);
	comm_freq3_renderer->signal_edited().connect(sigc::mem_fun(*this, &AirportEditor::edited_comm_freq3));
	comm_freq3_renderer->set_property("xalign", 1.0);
	FrequencyCellRenderer *comm_freq4_renderer = Gtk::manage(new FrequencyCellRenderer());
	Gtk::TreeViewColumn *comm_freq4_column = m_airportcommeditortreeview->get_column(m_airportcommeditortreeview->append_column(_("Freq 4"), *comm_freq4_renderer) - 1);
	if (comm_freq4_column) {
		comm_freq4_column->add_attribute(*comm_freq4_renderer, comm_freq4_renderer->get_property_name(), m_airportcomm_model_columns.m_col_freq4);
		comm_freq4_column->set_sort_column(m_airportcomm_model_columns.m_col_freq4);
	}
	comm_freq4_renderer->set_property("editable", true);
	comm_freq4_renderer->signal_edited().connect(sigc::mem_fun(*this, &AirportEditor::edited_comm_freq4));
	comm_freq4_renderer->set_property("xalign", 1.0);
  	Gtk::CellRendererText *comm_sector_renderer = Gtk::manage(new Gtk::CellRendererText());
	Gtk::TreeViewColumn *comm_sector_column = m_airportcommeditortreeview->get_column(m_airportcommeditortreeview->append_column(_("Sector"), *comm_sector_renderer) - 1);
	if (comm_sector_column) {
		comm_sector_column->add_attribute(*comm_sector_renderer, "text", m_airportcomm_model_columns.m_col_sector);
		comm_sector_column->set_sort_column(m_airportcomm_model_columns.m_col_sector);
	}
	comm_sector_renderer->signal_edited().connect(sigc::mem_fun(*this, &AirportEditor::edited_comm_sector));
	comm_sector_renderer->set_property("editable", true);
  	Gtk::CellRendererText *comm_oprhours_renderer = Gtk::manage(new Gtk::CellRendererText());
	Gtk::TreeViewColumn *comm_oprhours_column = m_airportcommeditortreeview->get_column(m_airportcommeditortreeview->append_column(_("OprHours"), *comm_oprhours_renderer) - 1);
	if (comm_oprhours_column) {
		comm_oprhours_column->add_attribute(*comm_oprhours_renderer, "text", m_airportcomm_model_columns.m_col_oprhours);
		comm_oprhours_column->set_sort_column(m_airportcomm_model_columns.m_col_oprhours);
	}
	comm_oprhours_renderer->signal_edited().connect(sigc::mem_fun(*this, &AirportEditor::edited_comm_oprhours));
	comm_oprhours_renderer->set_property("editable", true);
	for (unsigned int i = 0; i < 8; ++i) {
		Gtk::TreeViewColumn *col = m_airportcommeditortreeview->get_column(i);
		if (!col)
			continue;
		col->set_resizable(true);
		col->set_reorderable(true);
	}
	m_airportcommeditortreeview->columns_autosize();
	m_airportcommeditortreeview->set_enable_search(true);
	m_airportcommeditortreeview->set_search_column(m_airportvfrroutes_model_columns.m_col_name);
	// VFR Routes Treeview
  	Gtk::CellRendererText *vfrroutes_name_renderer = Gtk::manage(new Gtk::CellRendererText());
	Gtk::TreeViewColumn *vfrroutes_name_column = m_airportvfrrouteseditortreeview->get_column(m_airportvfrrouteseditortreeview->append_column(_("Name"), *vfrroutes_name_renderer) - 1);
	if (vfrroutes_name_column) {
		vfrroutes_name_column->add_attribute(*vfrroutes_name_renderer, "text", m_airportvfrroutes_model_columns.m_col_name);
		vfrroutes_name_column->set_sort_column(m_airportvfrroutes_model_columns.m_col_name);
	}
	vfrroutes_name_renderer->signal_edited().connect(sigc::mem_fun(*this, &AirportEditor::edited_vfrroutes_name));
	vfrroutes_name_renderer->set_property("editable", true);
  	Gtk::CellRendererText *vfrroutes_minalt_renderer = Gtk::manage(new Gtk::CellRendererText());
	Gtk::TreeViewColumn *vfrroutes_minalt_column = m_airportvfrrouteseditortreeview->get_column(m_airportvfrrouteseditortreeview->append_column(_("Min Alt"), *vfrroutes_minalt_renderer) - 1);
	if (vfrroutes_minalt_column) {
		vfrroutes_minalt_column->add_attribute(*vfrroutes_minalt_renderer, "text", m_airportvfrroutes_model_columns.m_col_minalt);
		vfrroutes_minalt_column->set_sort_column(m_airportvfrroutes_model_columns.m_col_minalt);
	}
	vfrroutes_minalt_renderer->set_property("xalign", 1.0);
	vfrroutes_minalt_renderer->signal_edited().connect(sigc::mem_fun(*this, &AirportEditor::edited_vfrroutes_minalt));
	vfrroutes_minalt_renderer->set_property("editable", true);

  	Gtk::CellRendererText *vfrroutes_maxalt_renderer = Gtk::manage(new Gtk::CellRendererText());
	Gtk::TreeViewColumn *vfrroutes_maxalt_column = m_airportvfrrouteseditortreeview->get_column(m_airportvfrrouteseditortreeview->append_column(_("Max Alt"), *vfrroutes_maxalt_renderer) - 1);
	if (vfrroutes_maxalt_column) {
		vfrroutes_maxalt_column->add_attribute(*vfrroutes_maxalt_renderer, "text", m_airportvfrroutes_model_columns.m_col_maxalt);
		vfrroutes_maxalt_column->set_sort_column(m_airportvfrroutes_model_columns.m_col_maxalt);
	}
	vfrroutes_maxalt_renderer->set_property("xalign", 1.0);
	vfrroutes_maxalt_renderer->signal_edited().connect(sigc::mem_fun(*this, &AirportEditor::edited_vfrroutes_maxalt));
	vfrroutes_maxalt_renderer->set_property("editable", true);

  	Gtk::CellRendererText *vfrroutes_cpcoord_renderer = Gtk::manage(new Gtk::CellRendererText());
	Gtk::TreeViewColumn *vfrroutes_cpcoord_column = m_airportvfrrouteseditortreeview->get_column(m_airportvfrrouteseditortreeview->append_column(_("Connection Point Coord"), *vfrroutes_cpcoord_renderer) - 1);
	if (vfrroutes_cpcoord_column) {
		vfrroutes_cpcoord_column->add_attribute(*vfrroutes_cpcoord_renderer, "text", m_airportvfrroutes_model_columns.m_col_cpcoord);
		vfrroutes_cpcoord_column->set_sort_column(m_airportvfrroutes_model_columns.m_col_cpcoord);
	}
	vfrroutes_cpcoord_renderer->set_property("xalign", 0.0);
	vfrroutes_cpcoord_renderer->set_property("editable", false);

  	Gtk::CellRendererText *vfrroutes_cpalt_renderer = Gtk::manage(new Gtk::CellRendererText());
	Gtk::TreeViewColumn *vfrroutes_cpalt_column = m_airportvfrrouteseditortreeview->get_column(m_airportvfrrouteseditortreeview->append_column(_("Alt"), *vfrroutes_cpalt_renderer) - 1);
	if (vfrroutes_cpalt_column) {
		vfrroutes_cpalt_column->add_attribute(*vfrroutes_cpalt_renderer, "text", m_airportvfrroutes_model_columns.m_col_cpalt);
		vfrroutes_cpalt_column->set_sort_column(m_airportvfrroutes_model_columns.m_col_cpalt);
	}
	vfrroutes_cpalt_renderer->set_property("xalign", 1.0);
	vfrroutes_cpalt_renderer->set_property("editable", false);

  	Gtk::CellRendererText *vfrroutes_cppath_renderer = Gtk::manage(new Gtk::CellRendererText());
	Gtk::TreeViewColumn *vfrroutes_cppath_column = m_airportvfrrouteseditortreeview->get_column(m_airportvfrrouteseditortreeview->append_column(_("Pathcode"), *vfrroutes_cppath_renderer) - 1);
	if (vfrroutes_cppath_column) {
		vfrroutes_cppath_column->add_attribute(*vfrroutes_cppath_renderer, "text", m_airportvfrroutes_model_columns.m_col_cppath);
		vfrroutes_cppath_column->set_sort_column(m_airportvfrroutes_model_columns.m_col_cppath);
	}
	vfrroutes_cppath_renderer->set_property("xalign", 0.0);
	vfrroutes_cppath_renderer->set_property("editable", false);

  	Gtk::CellRendererText *vfrroutes_cpname_renderer = Gtk::manage(new Gtk::CellRendererText());
	Gtk::TreeViewColumn *vfrroutes_cpname_column = m_airportvfrrouteseditortreeview->get_column(m_airportvfrrouteseditortreeview->append_column(_("Name"), *vfrroutes_cpname_renderer) - 1);
	if (vfrroutes_cpname_column) {
		vfrroutes_cpname_column->add_attribute(*vfrroutes_cpname_renderer, "text", m_airportvfrroutes_model_columns.m_col_cpname);
		vfrroutes_cpname_column->set_sort_column(m_airportvfrroutes_model_columns.m_col_cpname);
	}
	vfrroutes_cpname_renderer->set_property("xalign", 0.0);
	vfrroutes_cpname_renderer->set_property("editable", false);

	for (unsigned int i = 0; i < 7; ++i) {
		Gtk::TreeViewColumn *col = m_airportvfrrouteseditortreeview->get_column(i);
		if (!col)
			continue;
		col->set_resizable(true);
		col->set_reorderable(true);
	}
	m_airportvfrrouteseditortreeview->columns_autosize();
	m_airportvfrrouteseditortreeview->set_enable_search(true);
	m_airportvfrrouteseditortreeview->set_search_column(m_airportvfrroutes_model_columns.m_col_name);
	// VFR Route Points Treeview
  	Gtk::CellRendererText *vfrroute_index_renderer = Gtk::manage(new Gtk::CellRendererText());
	Gtk::TreeViewColumn *vfrroute_index_column = m_airportvfrrouteeditortreeview->get_column(m_airportvfrrouteeditortreeview->append_column(_("Nr"), *vfrroute_index_renderer) - 1);
	if (vfrroute_index_column) {
       		vfrroute_index_column->add_attribute(*vfrroute_index_renderer, "text", m_airportvfrroute_model_columns.m_col_index);
		vfrroute_index_column->set_sort_column(m_airportvfrroute_model_columns.m_col_index);
	}
  	Gtk::CellRendererText *vfrroute_name_renderer = Gtk::manage(new Gtk::CellRendererText());
	Gtk::TreeViewColumn *vfrroute_name_column = m_airportvfrrouteeditortreeview->get_column(m_airportvfrrouteeditortreeview->append_column(_("Name"), *vfrroute_name_renderer) - 1);
	if (vfrroute_name_column) {
       		vfrroute_name_column->add_attribute(*vfrroute_name_renderer, "text", m_airportvfrroute_model_columns.m_col_name);
		vfrroute_name_column->set_sort_column(m_airportvfrroute_model_columns.m_col_name);
	}
	vfrroute_name_renderer->signal_edited().connect(sigc::mem_fun(*this, &AirportEditor::edited_vfrroute_name));
	vfrroute_name_renderer->set_property("editable", true);
	CoordCellRenderer *vfrroute_coord_renderer = Gtk::manage(new CoordCellRenderer());
	Gtk::TreeViewColumn *vfrroute_coord_column = m_airportvfrrouteeditortreeview->get_column(m_airportvfrrouteeditortreeview->append_column(_("Coordinate"), *vfrroute_coord_renderer) - 1);
	if (vfrroute_coord_column) {
		vfrroute_coord_column->add_attribute(*vfrroute_coord_renderer, vfrroute_coord_renderer->get_property_name(), m_airportvfrroute_model_columns.m_col_coord);
		vfrroute_coord_column->set_sort_column(m_airportvfrroute_model_columns.m_col_coord);
	}
	vfrroute_coord_renderer->set_property("editable", true);
	vfrroute_coord_renderer->signal_edited().connect(sigc::mem_fun(*this, &AirportEditor::edited_vfrroute_coord));
	LabelPlacementRenderer *vfrroute_label_placement_renderer = Gtk::manage(new LabelPlacementRenderer());
	Gtk::TreeViewColumn *vfrroute_label_placement_column = m_airportvfrrouteeditortreeview->get_column(m_airportvfrrouteeditortreeview->append_column(_("Label"), *vfrroute_label_placement_renderer) - 1);
	if (vfrroute_label_placement_column) {
		vfrroute_label_placement_column->add_attribute(*vfrroute_label_placement_renderer, vfrroute_label_placement_renderer->get_property_name(), m_airportvfrroute_model_columns.m_col_label_placement);
		vfrroute_label_placement_column->set_sort_column(m_airportvfrroute_model_columns.m_col_label_placement);
	}
	vfrroute_label_placement_renderer->set_property("editable", true);
	vfrroute_label_placement_renderer->signal_edited().connect(sigc::mem_fun(*this, &AirportEditor::edited_vfrroute_label_placement));
  	Gtk::CellRendererText *vfrroute_alt_renderer = Gtk::manage(new Gtk::CellRendererText());
	Gtk::TreeViewColumn *vfrroute_alt_column = m_airportvfrrouteeditortreeview->get_column(m_airportvfrrouteeditortreeview->append_column(_("Altitude"), *vfrroute_alt_renderer) - 1);
	if (vfrroute_alt_column) {
       		vfrroute_alt_column->add_attribute(*vfrroute_alt_renderer, "text", m_airportvfrroute_model_columns.m_col_alt);
		vfrroute_alt_column->set_sort_column(m_airportvfrroute_model_columns.m_col_alt);
	}
	vfrroute_alt_renderer->signal_edited().connect(sigc::mem_fun(*this, &AirportEditor::edited_vfrroute_alt));
	vfrroute_alt_renderer->set_property("editable", true);
	AirportVFRAltCodeRenderer *vfrroute_altcode_renderer = Gtk::manage(new AirportVFRAltCodeRenderer());
	Gtk::TreeViewColumn *vfrroute_altcode_column = m_airportvfrrouteeditortreeview->get_column(m_airportvfrrouteeditortreeview->append_column(_("Altcode"), *vfrroute_altcode_renderer) - 1);
	if (vfrroute_altcode_column) {
		vfrroute_altcode_column->add_attribute(*vfrroute_altcode_renderer, vfrroute_altcode_renderer->get_property_name(), m_airportvfrroute_model_columns.m_col_altcode);
		vfrroute_altcode_column->set_sort_column(m_airportvfrroute_model_columns.m_col_altcode);
	}
	vfrroute_altcode_renderer->set_property("editable", true);
	vfrroute_altcode_renderer->signal_edited().connect(sigc::mem_fun(*this, &AirportEditor::edited_vfrroute_altcode));
	AirportVFRPathCodeRenderer *vfrroute_pathcode_renderer = Gtk::manage(new AirportVFRPathCodeRenderer());
	Gtk::TreeViewColumn *vfrroute_pathcode_column = m_airportvfrrouteeditortreeview->get_column(m_airportvfrrouteeditortreeview->append_column(_("Pathcode"), *vfrroute_pathcode_renderer) - 1);
	if (vfrroute_pathcode_column) {
		vfrroute_pathcode_column->add_attribute(*vfrroute_pathcode_renderer, vfrroute_pathcode_renderer->get_property_name(), m_airportvfrroute_model_columns.m_col_pathcode);
		vfrroute_pathcode_column->set_sort_column(m_airportvfrroute_model_columns.m_col_pathcode);
	}
	vfrroute_pathcode_renderer->set_property("editable", true);
	vfrroute_pathcode_renderer->signal_edited().connect(sigc::mem_fun(*this, &AirportEditor::edited_vfrroute_pathcode));
	AirportVFRPointSymbolRenderer *vfrroute_ptsym_renderer = Gtk::manage(new AirportVFRPointSymbolRenderer());
	Gtk::TreeViewColumn *vfrroute_ptsym_column = m_airportvfrrouteeditortreeview->get_column(m_airportvfrrouteeditortreeview->append_column(_("Symbol"), *vfrroute_ptsym_renderer) - 1);
	if (vfrroute_ptsym_column) {
		vfrroute_ptsym_column->add_attribute(*vfrroute_ptsym_renderer, vfrroute_ptsym_renderer->get_property_name(), m_airportvfrroute_model_columns.m_col_ptsym);
		vfrroute_ptsym_column->set_sort_column(m_airportvfrroute_model_columns.m_col_ptsym);
	}
	vfrroute_ptsym_renderer->set_property("editable", true);
	vfrroute_ptsym_renderer->signal_edited().connect(sigc::mem_fun(*this, &AirportEditor::edited_vfrroute_ptsym));
  	Gtk::CellRendererToggle *vfrroute_atairport_renderer = Gtk::manage(new Gtk::CellRendererToggle());
	Gtk::TreeViewColumn *vfrroute_atairport_column = m_airportvfrrouteeditortreeview->get_column(m_airportvfrrouteeditortreeview->append_column(_("At Airport"), *vfrroute_atairport_renderer) - 1);
	if (vfrroute_atairport_column) {
		vfrroute_atairport_column->add_attribute(*vfrroute_atairport_renderer, "active", m_airportvfrroute_model_columns.m_col_at_airport);
		vfrroute_atairport_column->set_sort_column(m_airportvfrroute_model_columns.m_col_at_airport);
	}
	vfrroute_atairport_renderer->signal_toggled().connect(sigc::mem_fun(*this, &AirportEditor::edited_vfrroute_atairport));
	vfrroute_atairport_renderer->set_property("editable", true);
	vfrroute_atairport_renderer->set_property("xalign", 0.0);
	for (unsigned int i = 0; i < 10; ++i) {
		Gtk::TreeViewColumn *col = m_airportvfrrouteeditortreeview->get_column(i);
		if (!col)
			continue;
		col->set_resizable(true);
		col->set_reorderable(true);
	}
	m_airportvfrrouteeditortreeview->columns_autosize();
	m_airportvfrrouteeditortreeview->set_enable_search(true);
	m_airportvfrrouteeditortreeview->set_search_column(m_airportvfrroute_model_columns.m_col_name);
	// point list
	{
		Gtk::CellRendererText *name_renderer = Gtk::manage(new Gtk::CellRendererText());
		name_renderer->set_property("xalign", 0.0);
		m_airportvfrrouteeditorpointlist->pack_start(*name_renderer, true);
		m_airportvfrrouteeditorpointlist->add_attribute(*name_renderer, "text", m_airportvfrpointlist_model_columns.m_col_name);
		CoordCellRenderer *coord_renderer = Gtk::manage(new CoordCellRenderer());
		coord_renderer->set_property("xalign", 0.0);
		m_airportvfrrouteeditorpointlist->pack_start(*coord_renderer, true);
		m_airportvfrrouteeditorpointlist->add_attribute(*coord_renderer, coord_renderer->get_property_name(), m_airportvfrpointlist_model_columns.m_col_coord);
	}
	// selection
	Glib::RefPtr<Gtk::TreeSelection> runway_selection = m_airportrunwayeditortreeview->get_selection();
	runway_selection->set_mode(Gtk::SELECTION_MULTIPLE);
	runway_selection->signal_changed().connect(sigc::mem_fun(*this, &AirportEditor::airporteditor_runway_selection_changed));
	Glib::RefPtr<Gtk::TreeSelection> helipad_selection = m_airporthelipadeditortreeview->get_selection();
	helipad_selection->set_mode(Gtk::SELECTION_MULTIPLE);
	helipad_selection->signal_changed().connect(sigc::mem_fun(*this, &AirportEditor::airporteditor_helipad_selection_changed));
	Glib::RefPtr<Gtk::TreeSelection> comm_selection = m_airportcommeditortreeview->get_selection();
	comm_selection->set_mode(Gtk::SELECTION_MULTIPLE);
	comm_selection->signal_changed().connect(sigc::mem_fun(*this, &AirportEditor::airporteditor_comm_selection_changed));
	Glib::RefPtr<Gtk::TreeSelection> vfrroutes_selection = m_airportvfrrouteseditortreeview->get_selection();
	vfrroutes_selection->set_mode(Gtk::SELECTION_MULTIPLE);
	vfrroutes_selection->signal_changed().connect(sigc::mem_fun(*this, &AirportEditor::airporteditor_vfrroutes_selection_changed));
	Glib::RefPtr<Gtk::TreeSelection> vfrroute_selection = m_airportvfrrouteeditortreeview->get_selection();
	vfrroute_selection->set_mode(Gtk::SELECTION_MULTIPLE);
	vfrroute_selection->signal_changed().connect(sigc::mem_fun(*this, &AirportEditor::airporteditor_vfrroute_selection_changed));
	// notebook
	m_airporteditornotebook->signal_switch_page().connect(sigc::hide<0>(sigc::mem_fun(*this, &AirportEditor::notebook_switch_page)));
	// final setup
	airporteditor_selection_changed();
	airporteditor_runway_selection_changed();
	airporteditor_helipad_selection_changed();
	airporteditor_comm_selection_changed();
	airporteditor_vfrroutes_selection_changed();
	airporteditor_vfrroute_selection_changed();
	//set_airportlist("");
	m_dbselectdialog.signal_byname().connect(sigc::mem_fun(*this, &AirportEditor::dbselect_byname));
	m_dbselectdialog.signal_byrectangle().connect(sigc::mem_fun(*this, &AirportEditor::dbselect_byrect));
	m_dbselectdialog.signal_bytime().connect(sigc::mem_fun(*this, &AirportEditor::dbselect_bytime));
	m_dbselectdialog.show();
	m_prefswindow = PrefsWindow::create(m_engine.get_prefs());
	m_airporteditorwindow->show();
}

AirportEditor::~AirportEditor()
{
	if (m_dbchanged) {
		std::cerr << "Analyzing database..." << std::endl;
		m_db->analyze();
		m_db->vacuum();
	}
	m_db->close();
}

void AirportEditor::dbselect_byname( const Glib::ustring & s, AirportsDb::comp_t comp )
{
	AirportsDb::elementvector_t els(m_db->find_by_text(s, 0, comp, 0));
	m_airportliststore->clear();
	for (AirportsDb::elementvector_t::iterator ei(els.begin()), ee(els.end()); ei != ee; ++ei) {
		AirportsDb::element_t& e(*ei);
		if (!e.is_valid())
			continue;
		Gtk::TreeModel::Row row(*(m_airportliststore->append()));
		set_row(row, e);
	}
}

void AirportEditor::dbselect_byrect( const Rect & r )
{
	AirportsDb::elementvector_t els(m_db->find_by_rect(r, 0));
	m_airportliststore->clear();
	for (AirportsDb::elementvector_t::iterator ei(els.begin()), ee(els.end()); ei != ee; ++ei) {
		AirportsDb::element_t& e(*ei);
		if (!e.is_valid())
			continue;
		Gtk::TreeModel::Row row(*(m_airportliststore->append()));
		set_row(row, e);
	}
}

void AirportEditor::dbselect_bytime(time_t timefrom, time_t timeto)
{
	AirportsDb::elementvector_t els(m_db->find_by_time(timefrom, timeto, 0));
	m_airportliststore->clear();
	for (AirportsDb::elementvector_t::iterator ei(els.begin()), ee(els.end()); ei != ee; ++ei) {
		AirportsDb::element_t& e(*ei);
		if (!e.is_valid())
			continue;
		Gtk::TreeModel::Row row(*(m_airportliststore->append()));
		set_row(row, e);
	}
}

void AirportEditor::buttonclear_clicked(void)
{
	m_airporteditorfind->set_text("");
}

void AirportEditor::find_changed(void)
{
	dbselect_byname(m_airporteditorfind->get_text());
}

void AirportEditor::menu_quit_activate(void)
{
	m_airporteditorwindow->hide();
}

void AirportEditor::menu_undo_activate(void)
{
	{
		Glib::RefPtr<Gtk::TreeSelection> selection = m_airporteditortreeview->get_selection();
		selection->unselect_all();
	}
	AirportsDb::Airport e(m_undoredo.undo());
	if (e.is_valid()) {
		save_nostack(e);
		select(e);
	}
	update_undoredo_menu();
}

void AirportEditor::menu_redo_activate(void)
{
	{
		Glib::RefPtr<Gtk::TreeSelection> selection = m_airporteditortreeview->get_selection();
		selection->unselect_all();
	}
	AirportsDb::Airport e(m_undoredo.redo());
	if (e.is_valid()) {
		save_nostack(e);
		select(e);
	}
	update_undoredo_menu();
}

void AirportEditor::menu_newairport_activate(void)
{
	AirportsDb::Airport e;
	e.set_typecode('A');
	e.set_sourceid(make_sourceid());
	e.set_modtime(time(0));
	save(e);
	select(e);
}

void AirportEditor::menu_deleteairport_activate(void)
{
	Glib::RefPtr<Gtk::TreeSelection> airporteditor_selection = m_airporteditortreeview->get_selection();
	std::vector<Gtk::TreeModel::Path> sel(airporteditor_selection->get_selected_rows());
	if (sel.empty())
		return;
	if (sel.size() > 1) {
		Gtk::MessageDialog *dlg = new Gtk::MessageDialog(_("Delete multiple airports?"), false, Gtk::MESSAGE_QUESTION, Gtk::BUTTONS_YES_NO, true);
		int res = dlg->run();
		delete dlg;
		if (res != Gtk::RESPONSE_YES) {
			if (res != Gtk::RESPONSE_NO)
				std::cerr << "unexpected dialog result: " << res << std::endl;
			return;
		}
	}
	for (std::vector<Gtk::TreeModel::Path>::iterator si(sel.begin()), se(sel.end()); si != se; si++) {
		Gtk::TreeModel::iterator iter(m_airportliststore->get_iter(*si));
		if (!iter) {
			std::cerr << "cannot get iterator for path " << (*si).to_string() << " ?""?" << std::endl;
			continue;
		}
		Gtk::TreeModel::Row row = *iter;
		std::cerr << "deleting row: " << row[m_airportlistcolumns.m_col_id] << std::endl;
		AirportsDb::Airport e((*m_db)(row[m_airportlistcolumns.m_col_id]));
		if (!e.is_valid())
			continue;
		m_undoredo.erase(e);
		m_db->erase(e);
		m_airportliststore->erase(iter);
	}
}

void AirportEditor::menu_airportsetelev_activate(void)
{
	Glib::RefPtr<Gtk::TreeSelection> airporteditor_selection = m_airporteditortreeview->get_selection();
	std::vector<Gtk::TreeModel::Path> sel(airporteditor_selection->get_selected_rows());
	if (sel.empty())
		return;
	if (sel.size() > 1) {
		Gtk::MessageDialog *dlg = new Gtk::MessageDialog(_("Set elevation of multiple airports?"), false, Gtk::MESSAGE_QUESTION, Gtk::BUTTONS_YES_NO, true);
		int res = dlg->run();
		delete dlg;
		if (res != Gtk::RESPONSE_YES) {
			if (res != Gtk::RESPONSE_NO)
				std::cerr << "unexpected dialog result: " << res << std::endl;
			return;
		}
	}
	for (std::vector<Gtk::TreeModel::Path>::iterator si(sel.begin()), se(sel.end()); si != se; si++) {
		Gtk::TreeModel::iterator iter(m_airportliststore->get_iter(*si));
		if (!iter) {
			std::cerr << "cannot get iterator for path " << (*si).to_string() << " ?""?" << std::endl;
			continue;
		}
		Gtk::TreeModel::Row row = *iter;
		Glib::RefPtr<AsyncElevUpdate> aeu(new AsyncElevUpdate(m_engine, *m_db.get(), row[m_airportlistcolumns.m_col_id], sigc::mem_fun(*this, &AirportEditor::save)));
	}
}

void AirportEditor::menu_selectall_activate(void)
{
	switch (m_airporteditornotebook->get_current_page()) {
		case 0:
		{
			Glib::RefPtr<Gtk::TreeSelection> selection = m_airporteditortreeview->get_selection();
			selection->select_all();
			break;
		}

		case 2:
		{
			Glib::RefPtr<Gtk::TreeSelection> selection = m_airportrunwayeditortreeview->get_selection();
			selection->select_all();
			break;
		}

		case 3:
		{
			Glib::RefPtr<Gtk::TreeSelection> selection = m_airporthelipadeditortreeview->get_selection();
			selection->select_all();
			break;
		}

		case 4:
		{
			Glib::RefPtr<Gtk::TreeSelection> selection = m_airportcommeditortreeview->get_selection();
			selection->select_all();
			break;
		}

		case 5:
		{
			Glib::RefPtr<Gtk::TreeSelection> selection = m_airportvfrrouteseditortreeview->get_selection();
			selection->select_all();
			break;
		}

		case 6:
		{
			Glib::RefPtr<Gtk::TreeSelection> selection = m_airportvfrrouteeditortreeview->get_selection();
			selection->select_all();
			break;
		}
	}
}

void AirportEditor::menu_unselectall_activate(void)
{
	switch (m_airporteditornotebook->get_current_page()) {
		case 0:
		{
			Glib::RefPtr<Gtk::TreeSelection> selection = m_airporteditortreeview->get_selection();
			selection->unselect_all();
			break;
		}

		case 2:
		{
			Glib::RefPtr<Gtk::TreeSelection> selection = m_airportrunwayeditortreeview->get_selection();
			selection->unselect_all();
			break;
		}

		case 3:
		{
			Glib::RefPtr<Gtk::TreeSelection> selection = m_airporthelipadeditortreeview->get_selection();
			selection->unselect_all();
			break;
		}

		case 4:
		{
			Glib::RefPtr<Gtk::TreeSelection> selection = m_airportcommeditortreeview->get_selection();
			selection->unselect_all();
			break;
		}

		case 5:
		{
			Glib::RefPtr<Gtk::TreeSelection> selection = m_airportvfrrouteseditortreeview->get_selection();
			selection->unselect_all();
			break;
		}

		case 6:
		{
			Glib::RefPtr<Gtk::TreeSelection> selection = m_airportvfrrouteeditortreeview->get_selection();
			selection->unselect_all();
			break;
		}
	}
}

void AirportEditor::menu_cut_activate(void)
{
	Gtk::Widget *focus = m_airporteditorwindow->get_focus();
	if (!focus)
		return;
	Gtk::Editable *edit = dynamic_cast<Gtk::Editable *>(focus);
	if (!edit)
		return;
	edit->cut_clipboard();
}

void AirportEditor::menu_copy_activate(void)
{
	Gtk::Widget *focus = m_airporteditorwindow->get_focus();
	if (!focus)
		return;
	Gtk::Editable *edit = dynamic_cast<Gtk::Editable *>(focus);
	if (!edit)
		return;
	edit->copy_clipboard();
}

void AirportEditor::menu_paste_activate(void)
{
	Gtk::Widget *focus = m_airporteditorwindow->get_focus();
	if (!focus)
		return;
	Gtk::Editable *edit = dynamic_cast<Gtk::Editable *>(focus);
	if (!edit)
		return;
	edit->paste_clipboard();
}

void AirportEditor::menu_delete_activate(void)
{
	Gtk::Widget *focus = m_airporteditorwindow->get_focus();
	if (!focus)
		return;
	Gtk::Editable *edit = dynamic_cast<Gtk::Editable *>(focus);
	if (!edit)
		return;
	edit->delete_selection();
}

void AirportEditor::menu_mapenable_toggled(void)
{
	if (m_airporteditormapenable->get_active()) {
		m_airporteditorairportdiagramenable->set_active(false);
		m_airporteditormap->set_renderer(VectorMapArea::renderer_vmap);
		m_airporteditormap->set_map_scale(m_engine.get_prefs().mapscale);
		m_airporteditormap->show();
		m_airporteditormapzoomin->show();
		m_airporteditormapzoomout->show();
	} else {
		m_airporteditormap->hide();
		m_airporteditormapzoomin->hide();
		m_airporteditormapzoomout->hide();
	}
}

void AirportEditor::menu_airportdiagramenable_toggled(void)
{
	if (m_airporteditorairportdiagramenable->get_active()) {
		m_airporteditormapenable->set_active(false);
		m_airporteditormap->set_renderer(VectorMapArea::renderer_airportdiagram);
		m_airporteditormap->set_map_scale(m_engine.get_prefs().mapscaleairportdiagram);
		m_airporteditormap->show();
		m_airporteditormapzoomin->show();
		m_airporteditormapzoomout->show();
	} else {
		m_airporteditormap->hide();
		m_airporteditormapzoomin->hide();
		m_airporteditormapzoomout->hide();
	}
}

void AirportEditor::menu_mapzoomin_activate(void)
{
	m_airporteditormap->zoom_in();
	if (m_airporteditormap->get_renderer() == VectorMapArea::renderer_airportdiagram)
		m_engine.get_prefs().mapscaleairportdiagram = m_airporteditormap->get_map_scale();
	else
		m_engine.get_prefs().mapscale = m_airporteditormap->get_map_scale();
}

void AirportEditor::menu_mapzoomout_activate(void)
{
	m_airporteditormap->zoom_out();
	if (m_airporteditormap->get_renderer() == VectorMapArea::renderer_airportdiagram)
		m_engine.get_prefs().mapscaleairportdiagram = m_airporteditormap->get_map_scale();
	else
		m_engine.get_prefs().mapscale = m_airporteditormap->get_map_scale();
}

void AirportEditor::new_runway(void)
{
	if (!m_curentryid)
		return;
	AirportsDb::Airport e((*m_db)(m_curentryid));
	if (!e.is_valid())
		return;
	e.set_modtime(time(0));
	e.add_rwy(AirportsDb::Airport::Runway("00", "18", 600, 20,
					      "ASP", e.get_coord(), e.get_coord(),
					      600, 600, 0, 0, e.get_elevation(), 600, 600, 0, 0x8000, e.get_elevation(),
					      AirportsDb::Airport::Runway::flag_le_usable | AirportsDb::Airport::Runway::flag_he_usable));
	m_db->save(e);
	m_dbchanged = true;
	set_rwy(e);
}

void AirportEditor::new_helipad(void)
{
	if (!m_curentryid)
		return;
	AirportsDb::Airport e((*m_db)(m_curentryid));
	if (!e.is_valid())
		return;
	e.set_modtime(time(0));
	e.add_helipad(AirportsDb::Airport::Helipad("H1", 30, 30, "ASP", e.get_coord(), 0, e.get_elevation(), AirportsDb::Airport::Helipad::flag_usable));
	m_db->save(e);
	m_dbchanged = true;
	set_hp(e);
}

void AirportEditor::new_comm(void)
{
	if (!m_curentryid)
		return;
	AirportsDb::Airport e((*m_db)(m_curentryid));
	if (!e.is_valid())
		return;
	e.set_modtime(time(0));
	e.add_comm(AirportsDb::Airport::Comm("", "", "", 0, 0, 0, 0, 0));
	m_db->save(e);
	m_dbchanged = true;
	set_comm(e);
}

void AirportEditor::new_vfrroutes(void)
{
	if (!m_curentryid)
		return;
	AirportsDb::Airport e((*m_db)(m_curentryid));
	if (!e.is_valid())
		return;
	e.set_modtime(time(0));
	e.add_vfrrte(AirportsDb::Airport::VFRRoute("?"));
	m_db->save(e);
	m_dbchanged = true;
	set_vfrroutes(e);
}

void AirportEditor::new_vfrroute(int index)
{
	if (!m_curentryid || m_vfrroute_index < 0)
		return;
	AirportsDb::Airport e((*m_db)(m_curentryid));
	if (!e.is_valid())
		return;
	e.set_modtime(time(0));
	AirportsDb::Airport::VFRRoute& rte(e.get_vfrrte(m_vfrroute_index));
	if (index < 0)
		index = 0;
	if (index > rte.size())
		index = rte.size();
	AirportsDb::Airport::VFRRoute::VFRRoutePoint pt(e.get_name(), e.get_coord(), 1000, AirportsDb::Airport::label_off,
							' ', AirportsDb::Airport::VFRRoute::VFRRoutePoint::pathcode_arrival,
							AirportsDb::Airport::VFRRoute::VFRRoutePoint::altcode_recommendedalt, true);
	if (rte.size()) {
		AirportsDb::Airport::VFRRoute::VFRRoutePoint pt1(rte[std::max(index - 1, 0)]);
		pt = AirportsDb::Airport::VFRRoute::VFRRoutePoint("", pt1.get_coord(), pt1.get_altitude(), pt1.get_label_placement(),
								  pt1.get_ptsym(), pt1.get_pathcode(), pt1.get_altcode(), pt1.is_at_airport());
	}
	rte.insert_point(pt, index);
	m_db->save(e);
	m_dbchanged = true;
	set_vfrroute(e);
}

void AirportEditor::menu_newrunway_activate(void)
{
	new_runway();
}

void AirportEditor::menu_deleterunway_activate(void)
{
	if (!m_curentryid)
		return;
	Glib::RefPtr<Gtk::TreeSelection> airporteditor_selection = m_airportrunwayeditortreeview->get_selection();
	std::vector<Gtk::TreeModel::Path> sel(airporteditor_selection->get_selected_rows());
	std::vector<int> ids;
	for (std::vector<Gtk::TreeModel::Path>::iterator si(sel.begin()), se(sel.end()); si != se; si++) {
		Gtk::TreeModel::Row row(*m_airportrunway_model->get_iter(*si));
		ids.push_back(row[m_airportrunway_model_columns.m_col_index]);
	}
	sort(ids.begin(), ids.end());
	AirportsDb::Airport e((*m_db)(m_curentryid));
	if (!e.is_valid())
		return;
	e.set_modtime(time(0));
	for (std::vector<int>::reverse_iterator ib(ids.rbegin()), ie(ids.rend()); ib != ie; ++ib)
		e.erase_rwy(*ib);
	m_db->save(e);
	m_dbchanged = true;
	set_rwy(e);
}

void AirportEditor::menu_runwaydim_activate(void)
{
	if (!m_curentryid)
		return;
	Glib::RefPtr<Gtk::TreeSelection> airporteditor_selection = m_airportrunwayeditortreeview->get_selection();
	std::vector<Gtk::TreeModel::Path> sel(airporteditor_selection->get_selected_rows());
	if (sel.size() != 1)
		return;
	Gtk::TreeModel::Row row(*m_airportrunway_model->get_iter(*sel.begin()));
	int id(row[m_airportrunway_model_columns.m_col_index]);
	AirportsDb::Airport e((*m_db)(m_curentryid));
	if (!e.is_valid() || id < 0 || id >= e.get_nrrwy())
		return;
	m_runwaydim->set_runwayinfo(e.get_rwy(id), e.get_coord());
	m_runwaydim->show();
}

void AirportEditor::menu_runwayhetole_activate(void)
{
	if (!m_curentryid)
		return;
	Glib::RefPtr<Gtk::TreeSelection> airporteditor_selection = m_airportrunwayeditortreeview->get_selection();
	std::vector<Gtk::TreeModel::Path> sel(airporteditor_selection->get_selected_rows());
	if (sel.size() != 1)
		return;
	Gtk::TreeModel::Row row(*m_airportrunway_model->get_iter(*sel.begin()));
	int id(row[m_airportrunway_model_columns.m_col_index]);
	AirportsDb::Airport e((*m_db)(m_curentryid));
	if (!e.is_valid() || id < 0 || id >= e.get_nrrwy())
		return;
	e.set_modtime(time(0));
	AirportsDb::Airport::Runway& rwy(e.get_rwy(id));
	{
		const char *cp(rwy.get_ident_he().c_str());
		char *cp1(0);
		unsigned int idn(strtoul(cp, &cp1, 10));
		bool ok(cp1 != cp && idn <= 36);
		char sfx(0);
		switch (std::toupper(*cp1)) {
		case 'L':
			sfx = 'R';
			++cp1;
			break;

		case 'R':
			sfx = 'L';
			++cp1;
			break;

		case 'C':
			sfx = 'C';
			++cp1;
			break;

		case 0:
			break;

		default:
			ok = false;
			break;
		}
		if (*cp1)
			ok = false;
		if (ok) {
			idn += 18;
			if (idn > 36)
				idn -= 36;
			std::ostringstream oss;
			oss << std::setfill('0') << std::setw(2) << idn;
			if (sfx)
				oss << sfx;
			rwy.set_ident_le(oss.str());
		}
	}
	rwy.set_length(std::max(rwy.get_length(), std::max((uint16_t)(rwy.get_he_lda() + rwy.get_he_disp()), rwy.get_he_tda())));
	rwy.set_le_elev(rwy.get_he_elev());
	rwy.set_le_hdg(rwy.get_he_hdg() + (1 << 15));
	rwy.set_le_disp(0);
	rwy.set_le_lda(rwy.get_he_tda());
	rwy.set_le_tda(rwy.get_he_lda());
	if (rwy.is_he_usable())
		rwy.set_flags(rwy.get_flags() | AirportsDb::Airport::Runway::flag_le_usable);
	else
		rwy.set_flags(rwy.get_flags() & ~AirportsDb::Airport::Runway::flag_le_usable);
	for (unsigned int i = 0; i < 8; ++i)
		rwy.set_le_light(i, rwy.get_he_light(i));
	rwy.normalize_he_le();
	if (!e.get_coord().is_invalid())
		rwy.compute_default_coord(e.get_coord());
	save_noerr_lists(e);
}

void AirportEditor::menu_runwayletohe_activate(void)
{
	if (!m_curentryid)
		return;
	Glib::RefPtr<Gtk::TreeSelection> airporteditor_selection = m_airportrunwayeditortreeview->get_selection();
	std::vector<Gtk::TreeModel::Path> sel(airporteditor_selection->get_selected_rows());
	if (sel.size() != 1)
		return;
	Gtk::TreeModel::Row row(*m_airportrunway_model->get_iter(*sel.begin()));
	int id(row[m_airportrunway_model_columns.m_col_index]);
	AirportsDb::Airport e((*m_db)(m_curentryid));
	if (!e.is_valid() || id < 0 || id >= e.get_nrrwy())
		return;
	e.set_modtime(time(0));
	AirportsDb::Airport::Runway& rwy(e.get_rwy(id));
	{
		const char *cp(rwy.get_ident_le().c_str());
		char *cp1(0);
		unsigned int idn(strtoul(cp, &cp1, 10));
		bool ok(cp1 != cp && idn <= 36);
		char sfx(0);
		switch (std::toupper(*cp1)) {
		case 'L':
			sfx = 'R';
			++cp1;
			break;

		case 'R':
			sfx = 'L';
			++cp1;
			break;

		case 'C':
			sfx = 'C';
			++cp1;
			break;

		case 0:
			break;

		default:
			ok = false;
			break;
		}
		if (*cp1)
			ok = false;
		if (ok) {
			idn += 18;
			if (idn > 36)
				idn -= 36;
			std::ostringstream oss;
			oss << std::setfill('0') << std::setw(2) << idn;
			if (sfx)
				oss << sfx;
			rwy.set_ident_he(oss.str());
		}
	}
	rwy.set_length(std::max(rwy.get_length(), std::max((uint16_t)(rwy.get_le_lda() + rwy.get_le_disp()), rwy.get_le_tda())));
	rwy.set_he_elev(rwy.get_le_elev());
	rwy.set_he_hdg(rwy.get_le_hdg() + (1 << 15));
	rwy.set_he_disp(0);
	rwy.set_he_lda(rwy.get_le_tda());
	rwy.set_he_tda(rwy.get_le_lda());
	if (rwy.is_le_usable())
		rwy.set_flags(rwy.get_flags() | AirportsDb::Airport::Runway::flag_he_usable);
	else
		rwy.set_flags(rwy.get_flags() & ~AirportsDb::Airport::Runway::flag_he_usable);
	for (unsigned int i = 0; i < 8; ++i)
		rwy.set_he_light(i, rwy.get_le_light(i));
	rwy.normalize_he_le();
	if (!e.get_coord().is_invalid())
		rwy.compute_default_coord(e.get_coord());
	save_noerr_lists(e);
}

void AirportEditor::menu_runwaycoords_activate(void)
{
	if (!m_curentryid)
		return;
	Glib::RefPtr<Gtk::TreeSelection> airporteditor_selection = m_airportrunwayeditortreeview->get_selection();
	std::vector<Gtk::TreeModel::Path> sel(airporteditor_selection->get_selected_rows());
	if (sel.size() != 1)
		return;
	Gtk::TreeModel::Row row(*m_airportrunway_model->get_iter(*sel.begin()));
	int id(row[m_airportrunway_model_columns.m_col_index]);
	AirportsDb::Airport e((*m_db)(m_curentryid));
	if (!e.is_valid() || id < 0 || id >= e.get_nrrwy())
		return;
	AirportsDb::Airport::Runway& rwy(e.get_rwy(id));
	if (!e.get_coord().is_invalid()) {
		e.set_modtime(time(0));
		rwy.compute_default_coord(e.get_coord());
		save_noerr_lists(e);
	}
}

void AirportEditor::menu_newhelipad_activate(void)
{
	new_helipad();
}

void AirportEditor::menu_deletehelipad_activate(void)
{
	if (!m_curentryid)
		return;
	Glib::RefPtr<Gtk::TreeSelection> airporteditor_selection = m_airporthelipadeditortreeview->get_selection();
	std::vector<Gtk::TreeModel::Path> sel(airporteditor_selection->get_selected_rows());
	std::vector<int> ids;
	for (std::vector<Gtk::TreeModel::Path>::iterator si(sel.begin()), se(sel.end()); si != se; si++) {
		Gtk::TreeModel::Row row(*m_airporthelipad_model->get_iter(*si));
		ids.push_back(row[m_airporthelipad_model_columns.m_col_index]);
	}
	sort(ids.begin(), ids.end());
	AirportsDb::Airport e((*m_db)(m_curentryid));
	if (!e.is_valid())
		return;
	e.set_modtime(time(0));
	for (std::vector<int>::reverse_iterator ib(ids.rbegin()), ie(ids.rend()); ib != ie; ++ib)
		e.erase_helipad(*ib);
	m_db->save(e);
	m_dbchanged = true;
	set_hp(e);
}

void AirportEditor::menu_newcomm_activate(void)
{
	new_comm();
}

void AirportEditor::menu_deletecomm_activate(void)
{
	if (!m_curentryid)
		return;
	Glib::RefPtr<Gtk::TreeSelection> airporteditor_selection = m_airportcommeditortreeview->get_selection();
	std::vector<Gtk::TreeModel::Path> sel(airporteditor_selection->get_selected_rows());
	std::vector<int> ids;
	for (std::vector<Gtk::TreeModel::Path>::iterator si(sel.begin()), se(sel.end()); si != se; si++) {
		Gtk::TreeModel::Row row(*m_airportcomm_model->get_iter(*si));
		ids.push_back(row[m_airportcomm_model_columns.m_col_index]);
	}
	sort(ids.begin(), ids.end());
	AirportsDb::Airport e((*m_db)(m_curentryid));
	if (!e.is_valid())
		return;
	e.set_modtime(time(0));
	for (std::vector<int>::reverse_iterator ib(ids.rbegin()), ie(ids.rend()); ib != ie; ++ib)
		e.erase_comm(*ib);
	m_db->save(e);
	m_dbchanged = true;
	set_comm(e);
}

void AirportEditor::menu_newvfrroute_activate(void)
{
	new_vfrroutes();
}

void AirportEditor::menu_deletevfrroute_activate(void)
{
	if (!m_curentryid)
		return;
	Glib::RefPtr<Gtk::TreeSelection> airporteditor_selection = m_airportvfrrouteseditortreeview->get_selection();
	std::vector<Gtk::TreeModel::Path> sel(airporteditor_selection->get_selected_rows());
	std::vector<int> ids;
	for (std::vector<Gtk::TreeModel::Path>::iterator si(sel.begin()), se(sel.end()); si != se; si++) {
		Gtk::TreeModel::Row row(*m_airportvfrroutes_model->get_iter(*si));
		ids.push_back(row[m_airportvfrroutes_model_columns.m_col_index]);
	}
	sort(ids.begin(), ids.end());
	AirportsDb::Airport e((*m_db)(m_curentryid));
	if (!e.is_valid())
		return;
	e.set_modtime(time(0));
	for (std::vector<int>::reverse_iterator ib(ids.rbegin()), ie(ids.rend()); ib != ie; ++ib)
		e.erase_vfrrte(*ib);
	m_db->save(e);
	m_dbchanged = true;
	set_vfrroutes(e);
}

void AirportEditor::menu_duplicatevfrroute_activate(void)
{
	if (!m_curentryid)
		return;
	Glib::RefPtr<Gtk::TreeSelection> airporteditor_selection = m_airportvfrrouteseditortreeview->get_selection();
	std::vector<Gtk::TreeModel::Path> sel(airporteditor_selection->get_selected_rows());
	std::vector<int> ids;
	for (std::vector<Gtk::TreeModel::Path>::iterator si(sel.begin()), se(sel.end()); si != se; si++) {
		Gtk::TreeModel::Row row(*m_airportvfrroutes_model->get_iter(*si));
		ids.push_back(row[m_airportvfrroutes_model_columns.m_col_index]);
	}
	sort(ids.begin(), ids.end());
	AirportsDb::Airport e((*m_db)(m_curentryid));
	if (!e.is_valid())
		return;
	e.set_modtime(time(0));
	for (std::vector<int>::reverse_iterator ib(ids.rbegin()), ie(ids.rend()); ib != ie; ++ib)
		e.add_vfrrte(e.get_vfrrte(*ib));
	m_db->save(e);
	m_dbchanged = true;
	set_vfrroutes(e);
}

void AirportEditor::menu_reversevfrroute_activate(void)
{
	if (!m_curentryid)
		return;
	Glib::RefPtr<Gtk::TreeSelection> airporteditor_selection = m_airportvfrrouteseditortreeview->get_selection();
	std::vector<Gtk::TreeModel::Path> sel(airporteditor_selection->get_selected_rows());
	AirportsDb::Airport e((*m_db)(m_curentryid));
	if (!e.is_valid())
		return;
	e.set_modtime(time(0));
	for (std::vector<Gtk::TreeModel::Path>::iterator si(sel.begin()), se(sel.end()); si != se; si++) {
		Gtk::TreeModel::Row row(*m_airportvfrroutes_model->get_iter(*si));
		AirportsDb::Airport::VFRRoute& vfrrte(e.get_vfrrte(row[m_airportvfrroutes_model_columns.m_col_index]));
		vfrrte.reverse();
	}
	m_db->save(e);
	m_dbchanged = true;
	set_vfrroutes(e);
}

void AirportEditor::menu_namevfrroute_activate(void)
{
	if (!m_curentryid)
		return;
	Glib::RefPtr<Gtk::TreeSelection> airporteditor_selection = m_airportvfrrouteseditortreeview->get_selection();
	std::vector<Gtk::TreeModel::Path> sel(airporteditor_selection->get_selected_rows());
	AirportsDb::Airport e((*m_db)(m_curentryid));
	if (!e.is_valid())
		return;
	e.set_modtime(time(0));
	for (std::vector<Gtk::TreeModel::Path>::iterator si(sel.begin()), se(sel.end()); si != se; si++) {
		Gtk::TreeModel::Row row(*m_airportvfrroutes_model->get_iter(*si));
		AirportsDb::Airport::VFRRoute& vfrrte(e.get_vfrrte(row[m_airportvfrroutes_model_columns.m_col_index]));
		vfrrte.name();
	}
	m_db->save(e);
	m_dbchanged = true;
	set_vfrroutes(e);
}

void AirportEditor::menu_duplicatereverseandnameallvfrroutes_activate(void)
{
	if (!m_curentryid)
		return;
	AirportsDb::Airport e((*m_db)(m_curentryid));
	if (!e.is_valid())
		return;
	e.set_modtime(time(0));
	for (unsigned int i(0), nr(e.get_nrvfrrte()); i < nr; ++i) {
		AirportsDb::Airport::VFRRoute rte(e.get_vfrrte(i));
		if (!rte.size())
			continue;
		const AirportsDb::Airport::VFRRoute::VFRRoutePoint& rtept(rte[0]);
		switch (rtept.get_pathcode()) {
		case AirportsDb::Airport::VFRRoute::VFRRoutePoint::pathcode_arrival:
		case AirportsDb::Airport::VFRRoute::VFRRoutePoint::pathcode_departure:
		case AirportsDb::Airport::VFRRoute::VFRRoutePoint::pathcode_holding:
		case AirportsDb::Airport::VFRRoute::VFRRoutePoint::pathcode_trafficpattern:
		case AirportsDb::Airport::VFRRoute::VFRRoutePoint::pathcode_vfrtransition:
		case AirportsDb::Airport::VFRRoute::VFRRoutePoint::pathcode_other:
			break;

		default:
			continue;
		}
		rte.reverse();
		rte.name();
		e.add_vfrrte(rte);
	}
	m_db->save(e);
	m_dbchanged = true;
	set_vfrroutes(e);
}

void AirportEditor::menu_appendvfrroutepoint_activate(void)
{
	new_vfrroute(std::numeric_limits<int>::max());
}

void AirportEditor::menu_insertvfrroutepoint_activate(void)
{
	Glib::RefPtr<Gtk::TreeSelection> airporteditor_selection = m_airportvfrrouteeditortreeview->get_selection();
	std::vector<Gtk::TreeModel::Path> sel(airporteditor_selection->get_selected_rows());
	std::vector<Gtk::TreeModel::Path>::size_type selcount(sel.size());
	if (selcount != 1) {
		menu_appendvfrroutepoint_activate();
		return;
	}
	Gtk::TreeModel::Row row(*m_airportvfrroute_model->get_iter(*sel.begin()));
	new_vfrroute(row[m_airportvfrroute_model_columns.m_col_index]);
}

void AirportEditor::menu_deletevfrroutepoint_activate(void)
{
	if (!m_curentryid || m_vfrroute_index < 0)
		return;
	Glib::RefPtr<Gtk::TreeSelection> airporteditor_selection = m_airportvfrrouteeditortreeview->get_selection();
	std::vector<Gtk::TreeModel::Path> sel(airporteditor_selection->get_selected_rows());
	std::vector<int> ids;
	for (std::vector<Gtk::TreeModel::Path>::iterator si(sel.begin()), se(sel.end()); si != se; si++) {
		Gtk::TreeModel::Row row(*m_airportvfrroute_model->get_iter(*si));
		ids.push_back(row[m_airportvfrroute_model_columns.m_col_index]);
	}
	sort(ids.begin(), ids.end());
	AirportsDb::Airport e((*m_db)(m_curentryid));
	if (!e.is_valid())
		return;
	e.set_modtime(time(0));
	AirportsDb::Airport::VFRRoute& rte(e.get_vfrrte(m_vfrroute_index));
	for (std::vector<int>::reverse_iterator ib(ids.rbegin()), ie(ids.rend()); ib != ie; ++ib)
		rte.erase_point(*ib);
	m_db->save(e);
	m_dbchanged = true;
	set_vfrroute(e);
}

void AirportEditor::menu_prefs_activate(void)
{
	if (m_prefswindow)
		m_prefswindow->show();
}

void AirportEditor::menu_about_activate(void)
{
	m_aboutdialog->show();
}

void AirportEditor::set_row_frules_flags(Gtk::TreeModel::Row& row, const AirportsDb::Airport& e)
{
	row[m_airportlistcolumns.m_col_frules_arr_vfr] = !!(e.get_flightrules() & AirportsDb::Airport::flightrules_arr_vfr);
	row[m_airportlistcolumns.m_col_frules_arr_ifr] = !!(e.get_flightrules() & AirportsDb::Airport::flightrules_arr_ifr);
	row[m_airportlistcolumns.m_col_frules_dep_vfr] = !!(e.get_flightrules() & AirportsDb::Airport::flightrules_dep_vfr);
	row[m_airportlistcolumns.m_col_frules_dep_ifr] = !!(e.get_flightrules() & AirportsDb::Airport::flightrules_dep_ifr);
}

void AirportEditor::set_row( Gtk::TreeModel::Row& row, const AirportsDb::Airport& e )
{
	row[m_airportlistcolumns.m_col_id] = e.get_address();
	row[m_airportlistcolumns.m_col_coord] = e.get_coord();
	row[m_airportlistcolumns.m_col_label_placement] = e.get_label_placement();
	row[m_airportlistcolumns.m_col_icao] = e.get_icao();
	row[m_airportlistcolumns.m_col_name] = e.get_name();
	row[m_airportlistcolumns.m_col_elev] = e.get_elevation();
	row[m_airportlistcolumns.m_col_typecode] = e.get_typecode();
	set_row_frules_flags(row, e);
	row[m_airportlistcolumns.m_col_sourceid] = e.get_sourceid();
	row[m_airportlistcolumns.m_col_modtime] = e.get_modtime();
	if (e.get_address() == m_curentryid)
		set_lists(e);
}

void AirportEditor::set_rwy_flags(Gtk::TreeModel::Row& row, const AirportsDb::Airport::Runway& rwy)
{
	row[m_airportrunway_model_columns.m_col_flags_he_usable] = rwy.is_he_usable();
	row[m_airportrunway_model_columns.m_col_flags_le_usable] = rwy.is_le_usable();
}

void AirportEditor::set_rwy(const AirportsDb::Airport & e)
{
	Gtk::TreeModel::iterator iter(m_airportrunway_model->children().begin());
	for (unsigned int i = 0; i < e.get_nrrwy(); i++) {
		const AirportsDb::Airport::Runway& rwy(e.get_rwy(i));
		Gtk::TreeModel::Row row;
		if (!iter)
			iter = m_airportrunway_model->append();
		row = *iter;
		iter++;
		row[m_airportrunway_model_columns.m_col_index] = i;
		row[m_airportrunway_model_columns.m_col_ident_he] = rwy.get_ident_he();
		row[m_airportrunway_model_columns.m_col_ident_le] = rwy.get_ident_le();
		row[m_airportrunway_model_columns.m_col_length] = rwy.get_length();
		row[m_airportrunway_model_columns.m_col_width] = rwy.get_width();
		row[m_airportrunway_model_columns.m_col_surface] = rwy.get_surface();
		row[m_airportrunway_model_columns.m_col_he_coord] = rwy.get_he_coord();
		row[m_airportrunway_model_columns.m_col_le_coord] = rwy.get_le_coord();
		row[m_airportrunway_model_columns.m_col_he_tda] = rwy.get_he_tda();
		row[m_airportrunway_model_columns.m_col_he_lda] = rwy.get_he_lda();
		row[m_airportrunway_model_columns.m_col_he_disp] = rwy.get_he_disp();
		row[m_airportrunway_model_columns.m_col_he_hdg] = rwy.get_he_hdg();
		row[m_airportrunway_model_columns.m_col_he_elev] = rwy.get_he_elev();
		row[m_airportrunway_model_columns.m_col_le_tda] = rwy.get_le_tda();
		row[m_airportrunway_model_columns.m_col_le_lda] = rwy.get_le_lda();
		row[m_airportrunway_model_columns.m_col_le_disp] = rwy.get_le_disp();
		row[m_airportrunway_model_columns.m_col_le_hdg] = rwy.get_le_hdg();
		row[m_airportrunway_model_columns.m_col_le_elev] = rwy.get_le_elev();
		set_rwy_flags(row, rwy);
		for (unsigned int i = 0; i < 8; ++i)
			row[m_airportrunway_model_columns.m_col_he_light[i]] = rwy.get_he_light(i);
		for (unsigned int i = 0; i < 8; ++i)
			row[m_airportrunway_model_columns.m_col_le_light[i]] = rwy.get_le_light(i);
	}
	for (; iter; ) {
		Gtk::TreeModel::iterator iter1(iter);
		iter++;
		m_airportrunway_model->erase(iter1);
	}
}

void AirportEditor::set_hp_flags(Gtk::TreeModel::Row& row, const AirportsDb::Airport::Helipad& hp)
{
	row[m_airporthelipad_model_columns.m_col_flags_usable] = hp.is_usable();
}

void AirportEditor::set_hp(const AirportsDb::Airport& e)
{
	Gtk::TreeModel::iterator iter(m_airporthelipad_model->children().begin());
	for (unsigned int i = 0; i < e.get_nrhelipads(); i++) {
		const AirportsDb::Airport::Helipad& hp(e.get_helipad(i));
		Gtk::TreeModel::Row row;
		if (!iter)
			iter = m_airporthelipad_model->append();
		row = *iter;
		iter++;
		row[m_airporthelipad_model_columns.m_col_index] = i;
		row[m_airporthelipad_model_columns.m_col_ident] = hp.get_ident();
		row[m_airporthelipad_model_columns.m_col_length] = hp.get_length();
		row[m_airporthelipad_model_columns.m_col_width] = hp.get_width();
		row[m_airporthelipad_model_columns.m_col_surface] = hp.get_surface();
		row[m_airporthelipad_model_columns.m_col_coord] = hp.get_coord();
		row[m_airporthelipad_model_columns.m_col_hdg] = hp.get_hdg();
		row[m_airporthelipad_model_columns.m_col_elev] = hp.get_elev();
		set_hp_flags(row, hp);
	}
	for (; iter; ) {
		Gtk::TreeModel::iterator iter1(iter);
		iter++;
		m_airporthelipad_model->erase(iter1);
	}
}

void AirportEditor::set_comm(const AirportsDb::Airport & e)
{
	Gtk::TreeModel::iterator iter(m_airportcomm_model->children().begin());
	for (unsigned int i = 0; i < e.get_nrcomm(); i++) {
		const AirportsDb::Airport::Comm& comm(e.get_comm(i));
		Gtk::TreeModel::Row row;
		if (!iter)
			iter = m_airportcomm_model->append();
		row = *iter;
		iter++;
		row[m_airportcomm_model_columns.m_col_index] = i;
		row[m_airportcomm_model_columns.m_col_freq0] = comm[0];
		row[m_airportcomm_model_columns.m_col_freq1] = comm[1];
		row[m_airportcomm_model_columns.m_col_freq2] = comm[2];
		row[m_airportcomm_model_columns.m_col_freq3] = comm[3];
		row[m_airportcomm_model_columns.m_col_freq4] = comm[4];
		row[m_airportcomm_model_columns.m_col_name] = comm.get_name();
		row[m_airportcomm_model_columns.m_col_sector] = comm.get_sector();
		row[m_airportcomm_model_columns.m_col_oprhours] = comm.get_oprhours();
	}
	for (; iter; ) {
		Gtk::TreeModel::iterator iter1(iter);
		iter++;
		m_airportcomm_model->erase(iter1);
	}
}

void AirportEditor::set_vfrroutes(const AirportsDb::Airport & e)
{
	Gtk::TreeModel::iterator iter(m_airportvfrroutes_model->children().begin());
	for (unsigned int i = 0; i < e.get_nrvfrrte(); i++) {
		const AirportsDb::Airport::VFRRoute& rte(e.get_vfrrte(i));
		Gtk::TreeModel::Row row;
		if (!iter)
			iter = m_airportvfrroutes_model->append();
		row = *iter;
		iter++;
		row[m_airportvfrroutes_model_columns.m_col_index] = i;
		row[m_airportvfrroutes_model_columns.m_col_name] = rte.get_name();
		row[m_airportvfrroutes_model_columns.m_col_minalt] = rte.get_minalt();
		row[m_airportvfrroutes_model_columns.m_col_maxalt] = rte.get_maxalt();
		if (rte.size() >= 2) {
			const AirportsDb::Airport::VFRRoute::VFRRoutePoint& pf(rte[0]);
			const AirportsDb::Airport::VFRRoute::VFRRoutePoint& pl(rte[rte.size()-1]);
			const AirportsDb::Airport::VFRRoute::VFRRoutePoint *pc(0);
			if (pf.is_at_airport()) {
				switch (pl.get_pathcode()) {
				case DbBaseElements::Airport::VFRRoute::VFRRoutePoint::pathcode_departure:
				case DbBaseElements::Airport::VFRRoute::VFRRoutePoint::pathcode_sid:
					pc = &pl;
					break;

				default:
					break;
				}
			} else if (pl.is_at_airport()) {
				switch (pf.get_pathcode()) {
				case DbBaseElements::Airport::VFRRoute::VFRRoutePoint::pathcode_arrival:
				case DbBaseElements::Airport::VFRRoute::VFRRoutePoint::pathcode_star:
					pc = &pf;
					break;

				default:
					break;
				}
			}
			if (pc) {
				if (!pc->get_coord().is_invalid())
					row[m_airportvfrroutes_model_columns.m_col_cpcoord] = pc->get_coord().get_lat_str() + " " + pc->get_coord().get_lon_str();
				row[m_airportvfrroutes_model_columns.m_col_cpalt] = pc->get_altitude_string();
				row[m_airportvfrroutes_model_columns.m_col_cppath] = pc->get_pathcode_string();
				row[m_airportvfrroutes_model_columns.m_col_cpname] = pc->get_name();
			}
		}
	}
	for (; iter; ) {
		Gtk::TreeModel::iterator iter1(iter);
		iter++;
		m_airportvfrroutes_model->erase(iter1);
	}
}

void AirportEditor::set_vfrroute(const AirportsDb::Airport & e)
{
	if (m_vfrroute_index < 0 || m_vfrroute_index >= e.get_nrvfrrte()) {
		m_airportvfrroute_model->clear();
		return;
	}
	const AirportsDb::Airport::VFRRoute& rte(e.get_vfrrte(m_vfrroute_index));
	Gtk::TreeModel::iterator iter(m_airportvfrroute_model->children().begin());
	for (unsigned int i = 0; i < rte.size(); i++) {
		const AirportsDb::Airport::VFRRoute::VFRRoutePoint& pt(rte[i]);
		Gtk::TreeModel::Row row;
		if (!iter)
			iter = m_airportvfrroute_model->append();
		row = *iter;
		iter++;
		row[m_airportvfrroute_model_columns.m_col_index] = i;
		row[m_airportvfrroute_model_columns.m_col_name] = pt.get_name();
		row[m_airportvfrroute_model_columns.m_col_coord] = pt.get_coord();
		row[m_airportvfrroute_model_columns.m_col_label_placement] = pt.get_label_placement();
		row[m_airportvfrroute_model_columns.m_col_alt] = pt.get_altitude();
		row[m_airportvfrroute_model_columns.m_col_altcode] = pt.get_altcode();
		row[m_airportvfrroute_model_columns.m_col_pathcode] = pt.get_pathcode();
		row[m_airportvfrroute_model_columns.m_col_ptsym] = pt.get_ptsym();
		row[m_airportvfrroute_model_columns.m_col_at_airport] = pt.is_at_airport();
	}
	for (; iter; ) {
		Gtk::TreeModel::iterator iter1(iter);
		iter++;
		m_airportvfrroute_model->erase(iter1);
	}
}

void AirportEditor::set_vfrpointlist(const AirportsDb::Airport & e)
{
	std::set<PointListEntry> pts;
	pts.insert(PointListEntry(e.get_name(), e.get_label_placement(), e.get_coord(), true));
	for (unsigned int rtenr = 0; rtenr < e.get_nrvfrrte(); rtenr++) {
		const AirportsDb::Airport::VFRRoute& rte(e.get_vfrrte(rtenr));
		for (unsigned int ptnr = 0; ptnr < rte.size(); ptnr++) {
			const AirportsDb::Airport::VFRRoute::VFRRoutePoint& pt(rte[ptnr]);
			if (pt.is_at_airport())
				continue;
			pts.insert(PointListEntry(pt.get_name(), pt.get_label_placement(), pt.get_coord(), false));
		}
	}
	m_airportvfrpointlist_model = Gtk::ListStore::create(m_airportvfrpointlist_model_columns);
	for (std::set<PointListEntry>::iterator pi(pts.begin()), pe(pts.end()); pi != pe; ++pi) {
		Gtk::TreeModel::Row row(*m_airportvfrpointlist_model->append());
		row[m_airportvfrpointlist_model_columns.m_col_name] = pi->get_name();
		row[m_airportvfrpointlist_model_columns.m_col_coord] = pi->get_coord();
		row[m_airportvfrpointlist_model_columns.m_col_label_placement] = pi->get_label_placement();
		row[m_airportvfrpointlist_model_columns.m_col_at_airport] = pi->is_at_airport();
	}
	m_airportvfrrouteeditorpointlist->set_model(m_airportvfrpointlist_model);
}

void AirportEditor::set_lists(const AirportsDb::Airport & e)
{
	Glib::RefPtr<Gtk::TextBuffer> tb(m_airporteditorvfrremark->get_buffer());
	tb->set_text(e.get_vfrrmk());
	tb->set_modified(false);
	set_rwy(e);
	set_hp(e);
	set_comm(e);
	set_vfrroutes(e);
	set_vfrroute(e);
	set_vfrpointlist(e);
}

void AirportEditor::edited_coord( const Glib::ustring & path_string, const Glib::ustring& new_text, Point new_coord )
{
	Gtk::TreePath path(path_string);
	Gtk::TreeModel::iterator iter = m_airportliststore->get_iter(path);
	if (!iter)
		return;
	Gtk::TreeRow row = *iter;
	row[m_airportlistcolumns.m_col_coord] = new_coord;
	AirportsDb::Airport e((*m_db)(row[m_airportlistcolumns.m_col_id]));
	if (!e.is_valid())
		return;
	e.set_modtime(time(0));
	e.set_coord(new_coord);
	e.recompute_bbox();
	save_noerr(e, row);
	m_airporteditormap->set_center(new_coord, e.get_elevation());
	m_airporteditormap->set_cursor(new_coord);
}

void AirportEditor::edited_label_placement( const Glib::ustring & path_string, const Glib::ustring & new_text, NavaidsDb::Navaid::label_placement_t new_placement )
{
	Gtk::TreePath path(path_string);
	Gtk::TreeModel::iterator iter = m_airportliststore->get_iter(path);
	if (!iter)
		return;
	Gtk::TreeRow row = *iter;
	//std::cerr << "edited_label_placement: txt " << new_text << " pl " << (int)new_placement << std::endl;
	row[m_airportlistcolumns.m_col_label_placement] = new_placement;
	AirportsDb::Airport e((*m_db)(row[m_airportlistcolumns.m_col_id]));
	if (!e.is_valid())
		return;
	e.set_modtime(time(0));
	e.set_label_placement(new_placement);
	save_noerr(e, row);
}

void AirportEditor::edited_sourceid( const Glib::ustring & path_string, const Glib::ustring & new_text )
{
	Gtk::TreePath path(path_string);
	Gtk::TreeModel::iterator iter = m_airportliststore->get_iter(path);
	if (!iter)
		return;
	Gtk::TreeRow row = *iter;
	row[m_airportlistcolumns.m_col_sourceid] = new_text;
	AirportsDb::Airport e((*m_db)(row[m_airportlistcolumns.m_col_id]));
	if (!e.is_valid())
		return;
	e.set_modtime(time(0));
	e.set_sourceid(new_text);
	save_noerr(e, row);
}

void AirportEditor::edited_icao( const Glib::ustring & path_string, const Glib::ustring & new_text )
{
	Gtk::TreePath path(path_string);
	Gtk::TreeModel::iterator iter = m_airportliststore->get_iter(path);
	if (!iter)
		return;
	Gtk::TreeRow row = *iter;
	row[m_airportlistcolumns.m_col_icao] = new_text;
	AirportsDb::Airport e((*m_db)(row[m_airportlistcolumns.m_col_id]));
	if (!e.is_valid())
		return;
	e.set_modtime(time(0));
	e.set_icao(new_text);
	save_noerr(e, row);
}

void AirportEditor::edited_name( const Glib::ustring & path_string, const Glib::ustring & new_text )
{
	Gtk::TreePath path(path_string);
	Gtk::TreeModel::iterator iter = m_airportliststore->get_iter(path);
	if (!iter)
		return;
	Gtk::TreeRow row = *iter;
	row[m_airportlistcolumns.m_col_name] = new_text;
	AirportsDb::Airport e((*m_db)(row[m_airportlistcolumns.m_col_id]));
	if (!e.is_valid())
		return;
	e.set_modtime(time(0));
	e.set_name(new_text);
	save_noerr(e, row);
}

void AirportEditor::edited_elev(const Glib::ustring & path_string, const Glib::ustring & new_text)
{
	Gtk::TreePath path(path_string);
	Gtk::TreeModel::iterator iter = m_airportliststore->get_iter(path);
	if (!iter)
		return;
	Gtk::TreeRow row = *iter;
	int32_t elev = Conversions::signed_feet_parse(new_text);
	row[m_airportlistcolumns.m_col_elev] = elev;
	AirportsDb::Airport e((*m_db)(row[m_airportlistcolumns.m_col_id]));
	if (!e.is_valid())
		return;
	e.set_modtime(time(0));
	e.set_elevation(elev);
	save_noerr(e, row);
}

void AirportEditor::edited_typecode(const Glib::ustring & path_string, const Glib::ustring & new_text, char new_typecode)
{
	Gtk::TreePath path(path_string);
	Gtk::TreeModel::iterator iter = m_airportliststore->get_iter(path);
	if (!iter)
		return;
	Gtk::TreeRow row = *iter;
	row[m_airportlistcolumns.m_col_typecode] = new_typecode;
	AirportsDb::Airport e((*m_db)(row[m_airportlistcolumns.m_col_id]));
	if (!e.is_valid())
		return;
	e.set_modtime(time(0));
	e.set_typecode(new_typecode);
	save_noerr(e, row);
}

void AirportEditor::edited_frules( const Glib::ustring & path_string, uint8_t f )
{
	Gtk::TreePath path(path_string);
	Gtk::TreeModel::iterator iter = m_airportliststore->get_iter(path);
	if (!iter)
		return;
	Gtk::TreeRow row = *iter;
	AirportsDb::Airport e((*m_db)(row[m_airportlistcolumns.m_col_id]));
	if (!e.is_valid())
		return;
	e.set_modtime(time(0));
	e.set_flightrules(e.get_flightrules() ^ f);
	set_row_frules_flags(row, e);
	save_noerr(e, row);
}

void AirportEditor::edited_modtime( const Glib::ustring & path_string, const Glib::ustring& new_text, time_t new_time )
{
	Gtk::TreePath path(path_string);
	Gtk::TreeModel::iterator iter = m_airportliststore->get_iter(path);
	if (!iter)
		return;
	Gtk::TreeRow row = *iter;
	row[m_airportlistcolumns.m_col_modtime] = new_time;
	AirportsDb::Airport e((*m_db)(row[m_airportlistcolumns.m_col_id]));
	if (!e.is_valid())
		return;
	e.set_modtime(new_time);
	save_noerr(e, row);
}

bool AirportEditor::edited_vfrremark_focus(GdkEventFocus * event)
{
	Glib::RefPtr<Gtk::TextBuffer> tb(m_airporteditorvfrremark->get_buffer());
	if (!tb->get_modified())
		return false;
	AirportsDb::Airport e((*m_db)(m_curentryid));
	if (!e.is_valid())
		return false;
	e.set_modtime(time(0));
	e.set_vfrrmk(tb->get_text());
	save_noerr_lists(e);
	tb->set_modified(false);
	return false;
}

void AirportEditor::edited_rwy_dimensions(const AirportsDb::Airport::Runway & rwyn)
{
	Glib::RefPtr<Gtk::TreeSelection> airporteditor_selection = m_airportrunwayeditortreeview->get_selection();
	std::vector<Gtk::TreeModel::Path> sel(airporteditor_selection->get_selected_rows());
	if (sel.size() != 1)
		return;
	Gtk::TreeModel::Row row(*m_airportrunway_model->get_iter(*sel.begin()));
	int index = row[m_airportrunway_model_columns.m_col_index];
	if (!m_curentryid || index < 0)
		return;
	AirportsDb::Airport e((*m_db)(m_curentryid));
	if (!e.is_valid())
		return;
	e.set_modtime(time(0));
	AirportsDb::Airport::Runway& rwy(e.get_rwy(index));
	rwy.set_width(rwyn.get_width());
	rwy.set_length(rwyn.get_length());
	rwy.set_ident_he(rwyn.get_ident_he());
	rwy.set_he_coord(rwyn.get_he_coord());
	rwy.set_he_elev(rwyn.get_he_elev());
	rwy.set_he_hdg(rwyn.get_he_hdg());
	rwy.set_he_disp(rwyn.get_he_disp());
	rwy.set_he_lda(rwyn.get_he_lda());
	rwy.set_he_tda(rwyn.get_he_tda());
	rwy.set_ident_le(rwyn.get_ident_le());
	rwy.set_le_coord(rwyn.get_le_coord());
	rwy.set_le_elev(rwyn.get_le_elev());
	rwy.set_le_hdg(rwyn.get_le_hdg());
	rwy.set_le_disp(rwyn.get_le_disp());
	rwy.set_le_lda(rwyn.get_le_lda());
	rwy.set_le_tda(rwyn.get_le_tda());
	save_noerr_lists(e);
}

void AirportEditor::edited_rwy_identhe(const Glib::ustring & path_string, const Glib::ustring & new_text)
{
	Gtk::TreePath path(path_string);
	Gtk::TreeModel::iterator iter = m_airportrunway_model->get_iter(path);
	if (!iter)
		return;
	Gtk::TreeRow row = *iter;
	int index = row[m_airportrunway_model_columns.m_col_index];
	if (!m_curentryid || index < 0)
		return;
	row[m_airportrunway_model_columns.m_col_ident_he] = new_text;
	AirportsDb::Airport e((*m_db)(m_curentryid));
	if (!e.is_valid())
		return;
	e.set_modtime(time(0));
	AirportsDb::Airport::Runway& rwy(e.get_rwy(index));
	rwy.set_ident_he(new_text);
	save_noerr_lists(e);
}

void AirportEditor::edited_rwy_identle(const Glib::ustring & path_string, const Glib::ustring & new_text)
{
	Gtk::TreePath path(path_string);
	Gtk::TreeModel::iterator iter = m_airportrunway_model->get_iter(path);
	if (!iter)
		return;
	Gtk::TreeRow row = *iter;
	int index = row[m_airportrunway_model_columns.m_col_index];
	if (!m_curentryid || index < 0)
		return;
	row[m_airportrunway_model_columns.m_col_ident_le] = new_text;
	AirportsDb::Airport e((*m_db)(m_curentryid));
	if (!e.is_valid())
		return;
	e.set_modtime(time(0));
	AirportsDb::Airport::Runway& rwy(e.get_rwy(index));
	rwy.set_ident_le(new_text);
	save_noerr_lists(e);
}

void AirportEditor::edited_rwy_length(const Glib::ustring & path_string, const Glib::ustring & new_text)
{
	Gtk::TreePath path(path_string);
	Gtk::TreeModel::iterator iter = m_airportrunway_model->get_iter(path);
	if (!iter)
		return;
	Gtk::TreeRow row = *iter;
	int index = row[m_airportrunway_model_columns.m_col_index];
	uint32_t length = Conversions::unsigned_feet_parse(new_text.c_str());
	if (!m_curentryid || index < 0)
		return;
	row[m_airportrunway_model_columns.m_col_length] = length;
	AirportsDb::Airport e((*m_db)(m_curentryid));
	if (!e.is_valid())
		return;
	e.set_modtime(time(0));
	AirportsDb::Airport::Runway& rwy(e.get_rwy(index));
	rwy.set_length(length);
	save_noerr_lists(e);
}

void AirportEditor::edited_rwy_width(const Glib::ustring & path_string, const Glib::ustring & new_text)
{
	Gtk::TreePath path(path_string);
	Gtk::TreeModel::iterator iter = m_airportrunway_model->get_iter(path);
	if (!iter)
		return;
	Gtk::TreeRow row = *iter;
	int index = row[m_airportrunway_model_columns.m_col_index];
	uint32_t width = Conversions::unsigned_feet_parse(new_text.c_str());
	if (!m_curentryid || index < 0)
		return;
	row[m_airportrunway_model_columns.m_col_width] = width;
	AirportsDb::Airport e((*m_db)(m_curentryid));
	if (!e.is_valid())
		return;
	e.set_modtime(time(0));
	AirportsDb::Airport::Runway& rwy(e.get_rwy(index));
	rwy.set_width(width);
	save_noerr_lists(e);
}

void AirportEditor::edited_rwy_surface(const Glib::ustring & path_string, const Glib::ustring & new_text)
{
	Gtk::TreePath path(path_string);
	Gtk::TreeModel::iterator iter = m_airportrunway_model->get_iter(path);
	if (!iter)
		return;
	Gtk::TreeRow row = *iter;
	int index = row[m_airportrunway_model_columns.m_col_index];
	if (!m_curentryid || index < 0)
		return;
	row[m_airportrunway_model_columns.m_col_surface] = new_text;
	AirportsDb::Airport e((*m_db)(m_curentryid));
	if (!e.is_valid())
		return;
	e.set_modtime(time(0));
	AirportsDb::Airport::Runway& rwy(e.get_rwy(index));
	rwy.set_surface(new_text);
	save_noerr_lists(e);
}

void AirportEditor::edited_rwy_hecoord(const Glib::ustring & path_string, const Glib::ustring & new_text, Point new_coord)
{
	Gtk::TreePath path(path_string);
	Gtk::TreeModel::iterator iter = m_airportrunway_model->get_iter(path);
	if (!iter)
		return;
	Gtk::TreeRow row = *iter;
	int index = row[m_airportrunway_model_columns.m_col_index];
	if (!m_curentryid || index < 0)
		return;
	row[m_airportrunway_model_columns.m_col_he_coord] = new_coord;
	AirportsDb::Airport e((*m_db)(m_curentryid));
	if (!e.is_valid())
		return;
	e.set_modtime(time(0));
	AirportsDb::Airport::Runway& rwy(e.get_rwy(index));
	rwy.set_he_coord(new_coord);
	save_noerr_lists(e);
	m_airporteditormap->set_center(new_coord, rwy.get_he_elev());
	m_airporteditormap->set_cursor(new_coord);
}

void AirportEditor::edited_rwy_lecoord(const Glib::ustring & path_string, const Glib::ustring & new_text, Point new_coord)
{
	Gtk::TreePath path(path_string);
	Gtk::TreeModel::iterator iter = m_airportrunway_model->get_iter(path);
	if (!iter)
		return;
	Gtk::TreeRow row = *iter;
	int index = row[m_airportrunway_model_columns.m_col_index];
	if (!m_curentryid || index < 0)
		return;
	row[m_airportrunway_model_columns.m_col_le_coord] = new_coord;
	AirportsDb::Airport e((*m_db)(m_curentryid));
	if (!e.is_valid())
		return;
	e.set_modtime(time(0));
	AirportsDb::Airport::Runway& rwy(e.get_rwy(index));
	rwy.set_le_coord(new_coord);
	save_noerr_lists(e);
	m_airporteditormap->set_center(new_coord, rwy.get_le_elev());
	m_airporteditormap->set_cursor(new_coord);
}

void AirportEditor::edited_rwy_hetda(const Glib::ustring & path_string, const Glib::ustring & new_text)
{
	Gtk::TreePath path(path_string);
	Gtk::TreeModel::iterator iter = m_airportrunway_model->get_iter(path);
	if (!iter)
		return;
	Gtk::TreeRow row = *iter;
	int index = row[m_airportrunway_model_columns.m_col_index];
	uint32_t length = Conversions::unsigned_feet_parse(new_text.c_str());
	if (!m_curentryid || index < 0)
		return;
	row[m_airportrunway_model_columns.m_col_he_tda] = length;
	AirportsDb::Airport e((*m_db)(m_curentryid));
	if (!e.is_valid())
		return;
	e.set_modtime(time(0));
	AirportsDb::Airport::Runway& rwy(e.get_rwy(index));
	rwy.set_he_tda(length);
	save_noerr_lists(e);
}

void AirportEditor::edited_rwy_helda(const Glib::ustring & path_string, const Glib::ustring & new_text)
{
	Gtk::TreePath path(path_string);
	Gtk::TreeModel::iterator iter = m_airportrunway_model->get_iter(path);
	if (!iter)
		return;
	Gtk::TreeRow row = *iter;
	int index = row[m_airportrunway_model_columns.m_col_index];
	uint32_t length = Conversions::unsigned_feet_parse(new_text.c_str());
	if (!m_curentryid || index < 0)
		return;
	row[m_airportrunway_model_columns.m_col_he_lda] = length;
	AirportsDb::Airport e((*m_db)(m_curentryid));
	if (!e.is_valid())
		return;
	e.set_modtime(time(0));
	AirportsDb::Airport::Runway& rwy(e.get_rwy(index));
	rwy.set_he_lda(length);
	save_noerr_lists(e);
}

void AirportEditor::edited_rwy_hedisp(const Glib::ustring & path_string, const Glib::ustring & new_text)
{
	Gtk::TreePath path(path_string);
	Gtk::TreeModel::iterator iter = m_airportrunway_model->get_iter(path);
	if (!iter)
		return;
	Gtk::TreeRow row = *iter;
	int index = row[m_airportrunway_model_columns.m_col_index];
	uint32_t length = Conversions::unsigned_feet_parse(new_text.c_str());
	if (!m_curentryid || index < 0)
		return;
	row[m_airportrunway_model_columns.m_col_he_disp] = length;
	AirportsDb::Airport e((*m_db)(m_curentryid));
	if (!e.is_valid())
		return;
	e.set_modtime(time(0));
	AirportsDb::Airport::Runway& rwy(e.get_rwy(index));
	rwy.set_he_disp(length);
	save_noerr_lists(e);
}

void AirportEditor::edited_rwy_hehdg(const Glib::ustring & path_string, const Glib::ustring & new_text, uint16_t new_angle)
{
	Gtk::TreePath path(path_string);
	Gtk::TreeModel::iterator iter = m_airportrunway_model->get_iter(path);
	if (!iter)
		return;
	Gtk::TreeRow row = *iter;
	int index = row[m_airportrunway_model_columns.m_col_index];
	if (!m_curentryid || index < 0)
		return;
	row[m_airportrunway_model_columns.m_col_he_hdg] = new_angle;
	AirportsDb::Airport e((*m_db)(m_curentryid));
	if (!e.is_valid())
		return;
	e.set_modtime(time(0));
	AirportsDb::Airport::Runway& rwy(e.get_rwy(index));
	rwy.set_he_hdg(new_angle);
	save_noerr_lists(e);
}

void AirportEditor::edited_rwy_heelev(const Glib::ustring & path_string, const Glib::ustring & new_text)
{
	Gtk::TreePath path(path_string);
	Gtk::TreeModel::iterator iter = m_airportrunway_model->get_iter(path);
	if (!iter)
		return;
	Gtk::TreeRow row = *iter;
	int index = row[m_airportrunway_model_columns.m_col_index];
	int32_t elev = Conversions::signed_feet_parse(new_text.c_str());
	if (!m_curentryid || index < 0)
		return;
	row[m_airportrunway_model_columns.m_col_he_elev] = elev;
	AirportsDb::Airport e((*m_db)(m_curentryid));
	if (!e.is_valid())
		return;
	e.set_modtime(time(0));
	AirportsDb::Airport::Runway& rwy(e.get_rwy(index));
	rwy.set_he_elev(elev);
	save_noerr_lists(e);
}

void AirportEditor::edited_rwy_letda(const Glib::ustring & path_string, const Glib::ustring & new_text)
{
	Gtk::TreePath path(path_string);
	Gtk::TreeModel::iterator iter = m_airportrunway_model->get_iter(path);
	if (!iter)
		return;
	Gtk::TreeRow row = *iter;
	int index = row[m_airportrunway_model_columns.m_col_index];
	uint32_t length = Conversions::unsigned_feet_parse(new_text.c_str());
	if (!m_curentryid || index < 0)
		return;
	row[m_airportrunway_model_columns.m_col_le_tda] = length;
	AirportsDb::Airport e((*m_db)(m_curentryid));
	if (!e.is_valid())
		return;
	e.set_modtime(time(0));
	AirportsDb::Airport::Runway& rwy(e.get_rwy(index));
	rwy.set_le_tda(length);
	save_noerr_lists(e);
}

void AirportEditor::edited_rwy_lelda(const Glib::ustring & path_string, const Glib::ustring & new_text)
{
	Gtk::TreePath path(path_string);
	Gtk::TreeModel::iterator iter = m_airportrunway_model->get_iter(path);
	if (!iter)
		return;
	Gtk::TreeRow row = *iter;
	int index = row[m_airportrunway_model_columns.m_col_index];
	uint32_t length = Conversions::unsigned_feet_parse(new_text.c_str());
	if (!m_curentryid || index < 0)
		return;
	row[m_airportrunway_model_columns.m_col_le_lda] = length;
	AirportsDb::Airport e((*m_db)(m_curentryid));
	if (!e.is_valid())
		return;
	e.set_modtime(time(0));
	AirportsDb::Airport::Runway& rwy(e.get_rwy(index));
	rwy.set_le_lda(length);
	save_noerr_lists(e);
}

void AirportEditor::edited_rwy_ledisp(const Glib::ustring & path_string, const Glib::ustring & new_text)
{
	Gtk::TreePath path(path_string);
	Gtk::TreeModel::iterator iter = m_airportrunway_model->get_iter(path);
	if (!iter)
		return;
	Gtk::TreeRow row = *iter;
	int index = row[m_airportrunway_model_columns.m_col_index];
	uint32_t length = Conversions::unsigned_feet_parse(new_text.c_str());
	if (!m_curentryid || index < 0)
		return;
	row[m_airportrunway_model_columns.m_col_le_disp] = length;
	AirportsDb::Airport e((*m_db)(m_curentryid));
	if (!e.is_valid())
		return;
	e.set_modtime(time(0));
	AirportsDb::Airport::Runway& rwy(e.get_rwy(index));
	rwy.set_le_disp(length);
	save_noerr_lists(e);
}

void AirportEditor::edited_rwy_lehdg(const Glib::ustring & path_string, const Glib::ustring & new_text, uint16_t new_angle)
{
	Gtk::TreePath path(path_string);
	Gtk::TreeModel::iterator iter = m_airportrunway_model->get_iter(path);
	if (!iter)
		return;
	Gtk::TreeRow row = *iter;
	int index = row[m_airportrunway_model_columns.m_col_index];
	if (!m_curentryid || index < 0)
		return;
	row[m_airportrunway_model_columns.m_col_le_hdg] = new_angle;
	AirportsDb::Airport e((*m_db)(m_curentryid));
	if (!e.is_valid())
		return;
	e.set_modtime(time(0));
	AirportsDb::Airport::Runway& rwy(e.get_rwy(index));
	rwy.set_le_hdg(new_angle);
	save_noerr_lists(e);
}

void AirportEditor::edited_rwy_leelev(const Glib::ustring & path_string, const Glib::ustring & new_text)
{
	Gtk::TreePath path(path_string);
	Gtk::TreeModel::iterator iter = m_airportrunway_model->get_iter(path);
	if (!iter)
		return;
	Gtk::TreeRow row = *iter;
	int index = row[m_airportrunway_model_columns.m_col_index];
	int32_t elev = Conversions::signed_feet_parse(new_text.c_str());
	if (!m_curentryid || index < 0)
		return;
	row[m_airportrunway_model_columns.m_col_le_elev] = elev;
	AirportsDb::Airport e((*m_db)(m_curentryid));
	if (!e.is_valid())
		return;
	e.set_modtime(time(0));
	AirportsDb::Airport::Runway& rwy(e.get_rwy(index));
	rwy.set_le_elev(elev);
	save_noerr_lists(e);
}

void AirportEditor::edited_rwy_flags(const Glib::ustring & path_string, uint16_t flag)
{
	Gtk::TreePath path(path_string);
	Gtk::TreeModel::iterator iter = m_airportrunway_model->get_iter(path);
	if (!iter)
		return;
	Gtk::TreeRow row = *iter;
	int index = row[m_airportrunway_model_columns.m_col_index];
	if (!m_curentryid || index < 0)
		return;
	AirportsDb::Airport e((*m_db)(m_curentryid));
	if (!e.is_valid())
		return;
	e.set_modtime(time(0));
	AirportsDb::Airport::Runway& rwy(e.get_rwy(index));
	rwy.set_flags(rwy.get_flags() ^ flag);
	set_rwy_flags(row, rwy);
	save_noerr_lists(e);
}

void AirportEditor::edited_rwy_helight(const Glib::ustring & path_string, const Glib::ustring & new_text, AirportsDb::Airport::Runway::light_t new_light, unsigned int idx)
{
	Gtk::TreePath path(path_string);
	Gtk::TreeModel::iterator iter = m_airportrunway_model->get_iter(path);
	if (!iter)
		return;
	Gtk::TreeRow row = *iter;
	int index = row[m_airportrunway_model_columns.m_col_index];
	if (!m_curentryid || index < 0)
		return;
	row[m_airportrunway_model_columns.m_col_he_light[idx]] = new_light;
	AirportsDb::Airport e((*m_db)(m_curentryid));
	if (!e.is_valid())
		return;
	e.set_modtime(time(0));
	AirportsDb::Airport::Runway& rwy(e.get_rwy(index));
	rwy.set_he_light(idx, new_light);
	save_noerr_lists(e);
}

void AirportEditor::edited_rwy_lelight(const Glib::ustring & path_string, const Glib::ustring & new_text, AirportsDb::Airport::Runway::light_t new_light, unsigned int idx)
{
	Gtk::TreePath path(path_string);
	Gtk::TreeModel::iterator iter = m_airportrunway_model->get_iter(path);
	if (!iter)
		return;
	Gtk::TreeRow row = *iter;
	int index = row[m_airportrunway_model_columns.m_col_index];
	if (!m_curentryid || index < 0)
		return;
	row[m_airportrunway_model_columns.m_col_le_light[idx]] = new_light;
	AirportsDb::Airport e((*m_db)(m_curentryid));
	if (!e.is_valid())
		return;
	e.set_modtime(time(0));
	AirportsDb::Airport::Runway& rwy(e.get_rwy(index));
	rwy.set_le_light(idx, new_light);
	save_noerr_lists(e);
}

void AirportEditor::edited_hp_ident(const Glib::ustring & path_string, const Glib::ustring & new_text)
{
	Gtk::TreePath path(path_string);
	Gtk::TreeModel::iterator iter = m_airporthelipad_model->get_iter(path);
	if (!iter)
		return;
	Gtk::TreeRow row = *iter;
	int index = row[m_airporthelipad_model_columns.m_col_index];
	if (!m_curentryid || index < 0)
		return;
	row[m_airporthelipad_model_columns.m_col_ident] = new_text;
	AirportsDb::Airport e((*m_db)(m_curentryid));
	if (!e.is_valid())
		return;
	e.set_modtime(time(0));
	AirportsDb::Airport::Helipad& hp(e.get_helipad(index));
	hp.set_ident(new_text);
	save_noerr_lists(e);
}

void AirportEditor::edited_hp_length(const Glib::ustring & path_string, const Glib::ustring & new_text)
{
	Gtk::TreePath path(path_string);
	Gtk::TreeModel::iterator iter = m_airporthelipad_model->get_iter(path);
	if (!iter)
		return;
	Gtk::TreeRow row = *iter;
	int index = row[m_airporthelipad_model_columns.m_col_index];
	uint32_t length = Conversions::unsigned_feet_parse(new_text.c_str());
	if (!m_curentryid || index < 0)
		return;
	row[m_airporthelipad_model_columns.m_col_length] = length;
	AirportsDb::Airport e((*m_db)(m_curentryid));
	if (!e.is_valid())
		return;
	e.set_modtime(time(0));
	AirportsDb::Airport::Helipad& hp(e.get_helipad(index));
	hp.set_length(length);
	save_noerr_lists(e);
}

void AirportEditor::edited_hp_width(const Glib::ustring & path_string, const Glib::ustring & new_text)
{
	Gtk::TreePath path(path_string);
	Gtk::TreeModel::iterator iter = m_airporthelipad_model->get_iter(path);
	if (!iter)
		return;
	Gtk::TreeRow row = *iter;
	int index = row[m_airporthelipad_model_columns.m_col_index];
	uint32_t width = Conversions::unsigned_feet_parse(new_text.c_str());
	if (!m_curentryid || index < 0)
		return;
	row[m_airporthelipad_model_columns.m_col_width] = width;
	AirportsDb::Airport e((*m_db)(m_curentryid));
	if (!e.is_valid())
		return;
	e.set_modtime(time(0));
	AirportsDb::Airport::Helipad& hp(e.get_helipad(index));
	hp.set_width(width);
	save_noerr_lists(e);
}

void AirportEditor::edited_hp_surface(const Glib::ustring & path_string, const Glib::ustring & new_text)
{
	Gtk::TreePath path(path_string);
	Gtk::TreeModel::iterator iter = m_airporthelipad_model->get_iter(path);
	if (!iter)
		return;
	Gtk::TreeRow row = *iter;
	int index = row[m_airporthelipad_model_columns.m_col_index];
	if (!m_curentryid || index < 0)
		return;
	row[m_airporthelipad_model_columns.m_col_surface] = new_text;
	AirportsDb::Airport e((*m_db)(m_curentryid));
	if (!e.is_valid())
		return;
	e.set_modtime(time(0));
	AirportsDb::Airport::Helipad& hp(e.get_helipad(index));
	hp.set_surface(new_text);
	save_noerr_lists(e);
}

void AirportEditor::edited_hp_coord(const Glib::ustring & path_string, const Glib::ustring & new_text, Point new_coord)
{
	Gtk::TreePath path(path_string);
	Gtk::TreeModel::iterator iter = m_airporthelipad_model->get_iter(path);
	if (!iter)
		return;
	Gtk::TreeRow row = *iter;
	int index = row[m_airporthelipad_model_columns.m_col_index];
	if (!m_curentryid || index < 0)
		return;
	row[m_airporthelipad_model_columns.m_col_coord] = new_coord;
	AirportsDb::Airport e((*m_db)(m_curentryid));
	if (!e.is_valid())
		return;
	e.set_modtime(time(0));
	AirportsDb::Airport::Helipad& hp(e.get_helipad(index));
	hp.set_coord(new_coord);
	save_noerr_lists(e);
	m_airporteditormap->set_center(new_coord, hp.get_elev());
	m_airporteditormap->set_cursor(new_coord);
}

void AirportEditor::edited_hp_hdg(const Glib::ustring & path_string, const Glib::ustring & new_text, uint16_t new_angle)
{
	Gtk::TreePath path(path_string);
	Gtk::TreeModel::iterator iter = m_airporthelipad_model->get_iter(path);
	if (!iter)
		return;
	Gtk::TreeRow row = *iter;
	int index = row[m_airporthelipad_model_columns.m_col_index];
	if (!m_curentryid || index < 0)
		return;
	row[m_airporthelipad_model_columns.m_col_hdg] = new_angle;
	AirportsDb::Airport e((*m_db)(m_curentryid));
	if (!e.is_valid())
		return;
	e.set_modtime(time(0));
	AirportsDb::Airport::Helipad& hp(e.get_helipad(index));
	hp.set_hdg(new_angle);
	save_noerr_lists(e);
}

void AirportEditor::edited_hp_elev(const Glib::ustring & path_string, const Glib::ustring & new_text)
{
	Gtk::TreePath path(path_string);
	Gtk::TreeModel::iterator iter = m_airporthelipad_model->get_iter(path);
	if (!iter)
		return;
	Gtk::TreeRow row = *iter;
	int index = row[m_airporthelipad_model_columns.m_col_index];
	int32_t elev = Conversions::signed_feet_parse(new_text.c_str());
	if (!m_curentryid || index < 0)
		return;
	row[m_airporthelipad_model_columns.m_col_elev] = elev;
	AirportsDb::Airport e((*m_db)(m_curentryid));
	if (!e.is_valid())
		return;
	e.set_modtime(time(0));
	AirportsDb::Airport::Helipad& hp(e.get_helipad(index));
	hp.set_elev(elev);
	save_noerr_lists(e);
}

void AirportEditor::edited_hp_flags(const Glib::ustring & path_string, uint16_t flag)
{
	Gtk::TreePath path(path_string);
	Gtk::TreeModel::iterator iter = m_airporthelipad_model->get_iter(path);
	if (!iter)
		return;
	Gtk::TreeRow row = *iter;
	int index = row[m_airporthelipad_model_columns.m_col_index];
	if (!m_curentryid || index < 0)
		return;
	AirportsDb::Airport e((*m_db)(m_curentryid));
	if (!e.is_valid())
		return;
	e.set_modtime(time(0));
	AirportsDb::Airport::Helipad& hp(e.get_helipad(index));
	hp.set_flags(hp.get_flags() ^ flag);
	set_hp_flags(row, hp);
	save_noerr_lists(e);
}

void AirportEditor::edited_comm_name(const Glib::ustring & path_string, const Glib::ustring & new_text)
{
	Gtk::TreePath path(path_string);
	Gtk::TreeModel::iterator iter = m_airportcomm_model->get_iter(path);
	if (!iter)
		return;
	Gtk::TreeRow row = *iter;
	int index = row[m_airportcomm_model_columns.m_col_index];
	if (!m_curentryid || index < 0)
		return;
	row[m_airportcomm_model_columns.m_col_name] = new_text;
	AirportsDb::Airport e((*m_db)(m_curentryid));
	if (!e.is_valid())
		return;
	e.set_modtime(time(0));
	AirportsDb::Airport::Comm& comm(e.get_comm(index));
	comm.set_name(new_text);
	save_noerr_lists(e);
}

void AirportEditor::edited_comm_freq0(const Glib::ustring & path_string, const Glib::ustring & new_text, uint32_t new_freq)
{
	Gtk::TreePath path(path_string);
	Gtk::TreeModel::iterator iter = m_airportcomm_model->get_iter(path);
	if (!iter)
		return;
	Gtk::TreeRow row = *iter;
	int index = row[m_airportcomm_model_columns.m_col_index];
	if (!m_curentryid || index < 0)
		return;
	row[m_airportcomm_model_columns.m_col_freq0] = new_freq;
	AirportsDb::Airport e((*m_db)(m_curentryid));
	if (!e.is_valid())
		return;
	e.set_modtime(time(0));
	AirportsDb::Airport::Comm& comm(e.get_comm(index));
	comm.set_freq(0, new_freq);
	save_noerr_lists(e);
}

void AirportEditor::edited_comm_freq1(const Glib::ustring & path_string, const Glib::ustring & new_text, uint32_t new_freq)
{
	Gtk::TreePath path(path_string);
	Gtk::TreeModel::iterator iter = m_airportcomm_model->get_iter(path);
	if (!iter)
		return;
	Gtk::TreeRow row = *iter;
	int index = row[m_airportcomm_model_columns.m_col_index];
	if (!m_curentryid || index < 0)
		return;
	row[m_airportcomm_model_columns.m_col_freq1] = new_freq;
	AirportsDb::Airport e((*m_db)(m_curentryid));
	if (!e.is_valid())
		return;
	e.set_modtime(time(0));
	AirportsDb::Airport::Comm& comm(e.get_comm(index));
	comm.set_freq(1, new_freq);
	save_noerr_lists(e);
}

void AirportEditor::edited_comm_freq2(const Glib::ustring & path_string, const Glib::ustring & new_text, uint32_t new_freq)
{
	Gtk::TreePath path(path_string);
	Gtk::TreeModel::iterator iter = m_airportcomm_model->get_iter(path);
	if (!iter)
		return;
	Gtk::TreeRow row = *iter;
	int index = row[m_airportcomm_model_columns.m_col_index];
	if (!m_curentryid || index < 0)
		return;
	row[m_airportcomm_model_columns.m_col_freq2] = new_freq;
	AirportsDb::Airport e((*m_db)(m_curentryid));
	if (!e.is_valid())
		return;
	e.set_modtime(time(0));
	AirportsDb::Airport::Comm& comm(e.get_comm(index));
	comm.set_freq(2, new_freq);
	save_noerr_lists(e);
}

void AirportEditor::edited_comm_freq3(const Glib::ustring & path_string, const Glib::ustring & new_text, uint32_t new_freq)
{
	Gtk::TreePath path(path_string);
	Gtk::TreeModel::iterator iter = m_airportcomm_model->get_iter(path);
	if (!iter)
		return;
	Gtk::TreeRow row = *iter;
	int index = row[m_airportcomm_model_columns.m_col_index];
	if (!m_curentryid || index < 0)
		return;
	row[m_airportcomm_model_columns.m_col_freq3] = new_freq;
	AirportsDb::Airport e((*m_db)(m_curentryid));
	if (!e.is_valid())
		return;
	e.set_modtime(time(0));
	AirportsDb::Airport::Comm& comm(e.get_comm(index));
	comm.set_freq(3, new_freq);
	save_noerr_lists(e);
}

void AirportEditor::edited_comm_freq4(const Glib::ustring & path_string, const Glib::ustring & new_text, uint32_t new_freq)
{
	Gtk::TreePath path(path_string);
	Gtk::TreeModel::iterator iter = m_airportcomm_model->get_iter(path);
	if (!iter)
		return;
	Gtk::TreeRow row = *iter;
	int index = row[m_airportcomm_model_columns.m_col_index];
	if (!m_curentryid || index < 0)
		return;
	row[m_airportcomm_model_columns.m_col_freq4] = new_freq;
	AirportsDb::Airport e((*m_db)(m_curentryid));
	if (!e.is_valid())
		return;
	e.set_modtime(time(0));
	AirportsDb::Airport::Comm& comm(e.get_comm(index));
	comm.set_freq(4, new_freq);
	save_noerr_lists(e);
}

void AirportEditor::edited_comm_sector(const Glib::ustring & path_string, const Glib::ustring & new_text)
{
	Gtk::TreePath path(path_string);
	Gtk::TreeModel::iterator iter = m_airportcomm_model->get_iter(path);
	if (!iter)
		return;
	Gtk::TreeRow row = *iter;
	int index = row[m_airportcomm_model_columns.m_col_index];
	if (!m_curentryid || index < 0)
		return;
	row[m_airportcomm_model_columns.m_col_sector] = new_text;
	AirportsDb::Airport e((*m_db)(m_curentryid));
	if (!e.is_valid())
		return;
	e.set_modtime(time(0));
	AirportsDb::Airport::Comm& comm(e.get_comm(index));
	comm.set_sector(new_text);
	save_noerr_lists(e);
}

void AirportEditor::edited_comm_oprhours(const Glib::ustring & path_string, const Glib::ustring & new_text)
{
	Gtk::TreePath path(path_string);
	Gtk::TreeModel::iterator iter = m_airportcomm_model->get_iter(path);
	if (!iter)
		return;
	Gtk::TreeRow row = *iter;
	int index = row[m_airportcomm_model_columns.m_col_index];
	if (!m_curentryid || index < 0)
		return;
	row[m_airportcomm_model_columns.m_col_oprhours] = new_text;
	AirportsDb::Airport e((*m_db)(m_curentryid));
	if (!e.is_valid())
		return;
	e.set_modtime(time(0));
	AirportsDb::Airport::Comm& comm(e.get_comm(index));
	comm.set_oprhours(new_text);
	save_noerr_lists(e);
}

void AirportEditor::edited_vfrroutes_name(const Glib::ustring & path_string, const Glib::ustring & new_text)
{
	Gtk::TreePath path(path_string);
	Gtk::TreeModel::iterator iter = m_airportvfrroutes_model->get_iter(path);
	if (!iter)
		return;
	Gtk::TreeRow row = *iter;
	int index = row[m_airportvfrroutes_model_columns.m_col_index];
	if (!m_curentryid || index < 0)
		return;
	row[m_airportvfrroutes_model_columns.m_col_name] = new_text;
	AirportsDb::Airport e((*m_db)(m_curentryid));
	if (!e.is_valid())
		return;
	e.set_modtime(time(0));
	AirportsDb::Airport::VFRRoute& rte(e.get_vfrrte(index));
	rte.set_name(new_text);
	save_noerr_lists(e);
}

void AirportEditor::edited_vfrroutes_minalt(const Glib::ustring & path_string, const Glib::ustring & new_text)
{
	Gtk::TreePath path(path_string);
	Gtk::TreeModel::iterator iter = m_airportvfrroutes_model->get_iter(path);
	if (!iter)
		return;
	Gtk::TreeRow row = *iter;
	int index = row[m_airportvfrroutes_model_columns.m_col_index];
	if (!m_curentryid || index < 0)
		return;
	int32_t alt = Conversions::signed_feet_parse(new_text);
	row[m_airportvfrroutes_model_columns.m_col_minalt] = alt;
	AirportsDb::Airport e((*m_db)(m_curentryid));
	if (!e.is_valid())
		return;
	e.set_modtime(time(0));
	AirportsDb::Airport::VFRRoute& rte(e.get_vfrrte(index));
	rte.set_minalt(alt);
	save_noerr_lists(e);
}

void AirportEditor::edited_vfrroutes_maxalt(const Glib::ustring & path_string, const Glib::ustring & new_text)
{
	Gtk::TreePath path(path_string);
	Gtk::TreeModel::iterator iter = m_airportvfrroutes_model->get_iter(path);
	if (!iter)
		return;
	Gtk::TreeRow row = *iter;
	int index = row[m_airportvfrroutes_model_columns.m_col_index];
	if (!m_curentryid || index < 0)
		return;
	int32_t alt = Conversions::signed_feet_parse(new_text);
	row[m_airportvfrroutes_model_columns.m_col_maxalt] = alt;
	AirportsDb::Airport e((*m_db)(m_curentryid));
	if (!e.is_valid())
		return;
	e.set_modtime(time(0));
	AirportsDb::Airport::VFRRoute& rte(e.get_vfrrte(index));
	rte.set_maxalt(alt);
	save_noerr_lists(e);
}

void AirportEditor::edited_vfrroute_name(const Glib::ustring & path_string, const Glib::ustring & new_text)
{
	Gtk::TreePath path(path_string);
	Gtk::TreeModel::iterator iter = m_airportvfrroute_model->get_iter(path);
	if (!iter)
		return;
	Gtk::TreeRow row = *iter;
	int index = row[m_airportvfrroute_model_columns.m_col_index];
	if (!m_curentryid || m_vfrroute_index < 0 || index < 0)
		return;
	row[m_airportvfrroute_model_columns.m_col_name] = new_text;
	AirportsDb::Airport e((*m_db)(m_curentryid));
	if (!e.is_valid())
		return;
	e.set_modtime(time(0));
	AirportsDb::Airport::VFRRoute& rte(e.get_vfrrte(m_vfrroute_index));
	AirportsDb::Airport::VFRRoute::VFRRoutePoint& pt(rte[index]);
	pt.set_name(new_text);
	save_noerr_lists(e);
}

void AirportEditor::edited_vfrroute_coord(const Glib::ustring & path_string, const Glib::ustring & new_text, Point new_coord)
{
	Gtk::TreePath path(path_string);
	Gtk::TreeModel::iterator iter = m_airportvfrroute_model->get_iter(path);
	if (!iter)
		return;
	Gtk::TreeRow row = *iter;
	int index = row[m_airportvfrroute_model_columns.m_col_index];
	if (!m_curentryid || m_vfrroute_index < 0 || index < 0)
		return;
	row[m_airportvfrroute_model_columns.m_col_coord] = new_coord;
	AirportsDb::Airport e((*m_db)(m_curentryid));
	if (!e.is_valid())
		return;
	e.set_modtime(time(0));
	AirportsDb::Airport::VFRRoute& rte(e.get_vfrrte(m_vfrroute_index));
	AirportsDb::Airport::VFRRoute::VFRRoutePoint& pt(rte[index]);
	pt.set_coord(new_coord);
	e.recompute_bbox();
	save_noerr_lists(e);
	m_airporteditormap->set_center(pt.get_coord(), pt.get_altitude());
	m_airporteditormap->set_cursor(pt.get_coord());
}

void AirportEditor::edited_vfrroute_label_placement(const Glib::ustring & path_string, const Glib::ustring & new_text, NavaidsDb::Navaid::label_placement_t new_placement)
{
	Gtk::TreePath path(path_string);
	Gtk::TreeModel::iterator iter = m_airportvfrroute_model->get_iter(path);
	if (!iter)
		return;
	Gtk::TreeRow row = *iter;
	int index = row[m_airportvfrroute_model_columns.m_col_index];
	if (!m_curentryid || m_vfrroute_index < 0 || index < 0)
		return;
	row[m_airportvfrroute_model_columns.m_col_name] = new_text;
	AirportsDb::Airport e((*m_db)(m_curentryid));
	if (!e.is_valid())
		return;
	e.set_modtime(time(0));
	AirportsDb::Airport::VFRRoute& rte(e.get_vfrrte(m_vfrroute_index));
	AirportsDb::Airport::VFRRoute::VFRRoutePoint& pt(rte[index]);
	pt.set_label_placement(new_placement);
	save_noerr_lists(e);
}

void AirportEditor::edited_vfrroute_alt(const Glib::ustring & path_string, const Glib::ustring & new_text)
{
	Gtk::TreePath path(path_string);
	Gtk::TreeModel::iterator iter = m_airportvfrroute_model->get_iter(path);
	if (!iter)
		return;
	Gtk::TreeRow row = *iter;
	int32_t alt = Conversions::signed_feet_parse(new_text);
	int index = row[m_airportvfrroute_model_columns.m_col_index];
	if (!m_curentryid || m_vfrroute_index < 0 || index < 0)
		return;
	row[m_airportvfrroute_model_columns.m_col_alt] = alt;
	AirportsDb::Airport e((*m_db)(m_curentryid));
	if (!e.is_valid())
		return;
	e.set_modtime(time(0));
	AirportsDb::Airport::VFRRoute& rte(e.get_vfrrte(m_vfrroute_index));
	AirportsDb::Airport::VFRRoute::VFRRoutePoint& pt(rte[index]);
	pt.set_altitude(alt);
	save_noerr_lists(e);
}

void AirportEditor::edited_vfrroute_altcode(const Glib::ustring & path_string, const Glib::ustring & new_text, AirportsDb::Airport::VFRRoute::VFRRoutePoint::altcode_t new_altcode)
{
	Gtk::TreePath path(path_string);
	Gtk::TreeModel::iterator iter = m_airportvfrroute_model->get_iter(path);
	if (!iter)
		return;
	Gtk::TreeRow row = *iter;
	int index = row[m_airportvfrroute_model_columns.m_col_index];
	if (!m_curentryid || m_vfrroute_index < 0 || index < 0)
		return;
	row[m_airportvfrroute_model_columns.m_col_altcode] = new_altcode;
	AirportsDb::Airport e((*m_db)(m_curentryid));
	if (!e.is_valid())
		return;
	e.set_modtime(time(0));
	AirportsDb::Airport::VFRRoute& rte(e.get_vfrrte(m_vfrroute_index));
	AirportsDb::Airport::VFRRoute::VFRRoutePoint& pt(rte[index]);
	pt.set_altcode(new_altcode);
	save_noerr_lists(e);
}

void AirportEditor::edited_vfrroute_pathcode(const Glib::ustring & path_string, const Glib::ustring & new_text, AirportsDb::Airport::VFRRoute::VFRRoutePoint::pathcode_t new_pathcode)
{
	Gtk::TreePath path(path_string);
	Gtk::TreeModel::iterator iter = m_airportvfrroute_model->get_iter(path);
	if (!iter)
		return;
	Gtk::TreeRow row = *iter;
	int index = row[m_airportvfrroute_model_columns.m_col_index];
	if (!m_curentryid || m_vfrroute_index < 0 || index < 0)
		return;
	row[m_airportvfrroute_model_columns.m_col_pathcode] = new_pathcode;
	AirportsDb::Airport e((*m_db)(m_curentryid));
	if (!e.is_valid())
		return;
	e.set_modtime(time(0));
	AirportsDb::Airport::VFRRoute& rte(e.get_vfrrte(m_vfrroute_index));
	AirportsDb::Airport::VFRRoute::VFRRoutePoint& pt(rte[index]);
	pt.set_pathcode(new_pathcode);
	save_noerr_lists(e);
}

void AirportEditor::edited_vfrroute_ptsym(const Glib::ustring & path_string, const Glib::ustring & new_text, char new_ptsym)
{
	Gtk::TreePath path(path_string);
	Gtk::TreeModel::iterator iter = m_airportvfrroute_model->get_iter(path);
	if (!iter)
		return;
	Gtk::TreeRow row = *iter;
	int index = row[m_airportvfrroute_model_columns.m_col_index];
	if (!m_curentryid || m_vfrroute_index < 0 || index < 0)
		return;
	row[m_airportvfrroute_model_columns.m_col_ptsym] = new_ptsym;
	AirportsDb::Airport e((*m_db)(m_curentryid));
	if (!e.is_valid())
		return;
	e.set_modtime(time(0));
	AirportsDb::Airport::VFRRoute& rte(e.get_vfrrte(m_vfrroute_index));
	AirportsDb::Airport::VFRRoute::VFRRoutePoint& pt(rte[index]);
	pt.set_ptsym(new_ptsym);
	save_noerr_lists(e);
}

void AirportEditor::edited_vfrroute_atairport(const Glib::ustring & path_string)
{
	Gtk::TreePath path(path_string);
	Gtk::TreeModel::iterator iter = m_airportvfrroute_model->get_iter(path);
	if (!iter)
		return;
	Gtk::TreeRow row = *iter;
	int index = row[m_airportvfrroute_model_columns.m_col_index];
	if (!m_curentryid || m_vfrroute_index < 0 || index < 0)
		return;
	row[m_airportvfrroute_model_columns.m_col_at_airport] = !row[m_airportvfrroute_model_columns.m_col_at_airport];
	AirportsDb::Airport e((*m_db)(m_curentryid));
	if (!e.is_valid())
		return;
	e.set_modtime(time(0));
	AirportsDb::Airport::VFRRoute& rte(e.get_vfrrte(m_vfrroute_index));
	AirportsDb::Airport::VFRRoute::VFRRoutePoint& pt(rte[index]);
	pt.set_at_airport(!pt.is_at_airport());
	save_noerr_lists(e);
}

void AirportEditor::edited_vfrroute_pointlist(void)
{
	Gtk::TreeModel::iterator iterpl = m_airportvfrrouteeditorpointlist->get_active();
	if (!iterpl)
		return;
	Gtk::TreeRow rowpl = *iterpl;
	Glib::RefPtr<Gtk::TreeSelection> airporteditor_selection = m_airportvfrrouteeditortreeview->get_selection();
	std::vector<Gtk::TreeModel::Path> sel(airporteditor_selection->get_selected_rows());
	std::vector<Gtk::TreeModel::Path>::size_type selcount(sel.size());
	if (selcount != 1)
		return;
	Gtk::TreeModel::iterator iter = m_airportvfrroute_model->get_iter(*sel.begin());
	if (!iter)
		return;
	Gtk::TreeRow row = *iter;
	int index = row[m_airportvfrroute_model_columns.m_col_index];
	if (!m_curentryid || m_vfrroute_index < 0 || index < 0)
		return;
	row[m_airportvfrroute_model_columns.m_col_name] = (Glib::ustring)rowpl[m_airportvfrpointlist_model_columns.m_col_name];
	row[m_airportvfrroute_model_columns.m_col_coord] = (Point)rowpl[m_airportvfrpointlist_model_columns.m_col_coord];
	row[m_airportvfrroute_model_columns.m_col_label_placement] = (AirportsDb::Airport::label_placement_t)rowpl[m_airportvfrpointlist_model_columns.m_col_label_placement];
	row[m_airportvfrroute_model_columns.m_col_at_airport] = (bool)rowpl[m_airportvfrpointlist_model_columns.m_col_at_airport];
	AirportsDb::Airport e((*m_db)(m_curentryid));
	if (!e.is_valid())
		return;
	e.set_modtime(time(0));
	AirportsDb::Airport::VFRRoute& rte(e.get_vfrrte(m_vfrroute_index));
	AirportsDb::Airport::VFRRoute::VFRRoutePoint& pt(rte[index]);
	pt.set_name(rowpl[m_airportvfrpointlist_model_columns.m_col_name]);
	pt.set_coord(Point(rowpl[m_airportvfrpointlist_model_columns.m_col_coord]));
	pt.set_label_placement(rowpl[m_airportvfrpointlist_model_columns.m_col_label_placement]);
	pt.set_at_airport(rowpl[m_airportvfrpointlist_model_columns.m_col_at_airport]);
	save_noerr_lists(e);
}

void AirportEditor::map_cursor( Point cursor, VectorMapArea::cursor_validity_t valid )
{
	if (valid != VectorMapArea::cursor_mouse)
		return;
	if (6 == m_airporteditornotebook->get_current_page()) {
		if (!m_curentryid || m_vfrroute_index < 0)
			return;
		Glib::RefPtr<Gtk::TreeSelection> airporteditor_selection = m_airportvfrrouteeditortreeview->get_selection();
		std::vector<Gtk::TreeModel::Path> sel(airporteditor_selection->get_selected_rows());
		std::vector<Gtk::TreeModel::Path>::size_type selcount(sel.size());
		if (selcount != 1)
			return;
		Gtk::TreeModel::iterator iter = m_airportvfrroute_model->get_iter(*sel.begin());
		if (!iter)
			return;
		Gtk::TreeModel::Row row = *iter;
		int index = row[m_airportvfrroute_model_columns.m_col_index];
		if (index < 0)
			return;
		row[m_airportvfrroute_model_columns.m_col_coord] = cursor;
		AirportsDb::Airport e((*m_db)(m_curentryid));
		if (!e.is_valid())
			return;
		e.set_modtime(time(0));
		AirportsDb::Airport::VFRRoute& rte(e.get_vfrrte(m_vfrroute_index));
		AirportsDb::Airport::VFRRoute::VFRRoutePoint& pt(rte[index]);
		pt.set_coord(cursor);
		e.recompute_bbox();
		save_noerr_lists(e);
		return;
	}
	Glib::RefPtr<Gtk::TreeSelection> airporteditor_selection = m_airporteditortreeview->get_selection();
	std::vector<Gtk::TreeModel::Path> sel(airporteditor_selection->get_selected_rows());
	std::vector<Gtk::TreeModel::Path>::size_type selcount(sel.size());
	Gtk::TreeModel::iterator iter;
	if (selcount > 0)
		iter = m_airportliststore->get_iter(*sel.begin());
	if (selcount != 1 || !iter)
		return;
	Gtk::TreeRow row = *iter;
	row[m_airportlistcolumns.m_col_coord] = cursor;
	AirportsDb::Airport e((*m_db)(row[m_airportlistcolumns.m_col_id]));
	if (!e.is_valid())
		return;
	e.set_modtime(time(0));
	e.set_coord(cursor);
	save_noerr(e, row);
}

void AirportEditor::map_drawflags(int flags)
{
	*m_airporteditormap = (MapRenderer::DrawFlags)flags;
}

void AirportEditor::save_nostack( AirportsDb::Airport & e )
{
	if (!e.is_valid())
		return;
	//std::cerr << "save_nostack" << std::endl;
	AirportsDb::Airport::Address addr(e.get_address());
	for (unsigned int i = 0; i < e.get_nrrwy(); ++i)
		e.get_rwy(i).normalize_he_le();
	m_db->save(e);
	m_dbchanged = true;
	if (m_curentryid == addr)
		m_curentryid = e.get_address();
	m_airporteditormap->airports_changed();
	Gtk::TreeIter iter = m_airportliststore->children().begin();
	while (iter) {
		Gtk::TreeRow row = *iter;
		if (addr == row[m_airportlistcolumns.m_col_id]) {
			set_row(row, e);
			return;
		}
		iter++;
	}
	Gtk::TreeModel::Row row(*(m_airportliststore->append()));
	set_row(row, e);
}

void AirportEditor::save( AirportsDb::Airport & e )
{
	//std::cerr << "save" << std::endl;
	save_nostack(e);
	m_undoredo.save(e);
	update_undoredo_menu();
}

void AirportEditor::save_noerr(AirportsDb::Airport & e, Gtk::TreeModel::Row & row)
{
	if (e.get_address() != row[m_airportlistcolumns.m_col_id])
		throw std::runtime_error("AirportEditor::save: ID mismatch");
	try {
		save(e);
	} catch (const sqlite3x::database_error& dberr) {
		AirportsDb::Airport eold((*m_db)(row[m_airportlistcolumns.m_col_id]));
		set_row(row, eold);
	}
}

void AirportEditor::save_noerr_lists(AirportsDb::Airport & e)
{
	try {
		save(e);
	} catch (const sqlite3x::database_error& dberr) {
		AirportsDb::Airport eold((*m_db)(e.get_address()));
		set_lists(eold);
	}
}

void AirportEditor::select( const AirportsDb::Airport & e )
{
	Glib::RefPtr<Gtk::TreeSelection> airporteditor_selection = m_airporteditortreeview->get_selection();
	Gtk::TreeIter iter = m_airportliststore->children().begin();
	while (iter) {
		Gtk::TreeRow row = *iter;
		if (e.get_address() == row[m_airportlistcolumns.m_col_id]) {
			airporteditor_selection->unselect_all();
			airporteditor_selection->select(iter);
			m_airporteditortreeview->scroll_to_row(Gtk::TreeModel::Path(iter));
			return;
		}
		iter++;
	}
}

void AirportEditor::update_undoredo_menu(void)
{
	m_airporteditorundo->set_sensitive(m_undoredo.can_undo());
	m_airporteditorredo->set_sensitive(m_undoredo.can_redo());
}

void AirportEditor::col_latlon1_clicked(void)
{
	std::cerr << "col_latlon1_clicked" << std::endl;
}

void AirportEditor::notebook_switch_page(guint page_num)
{
	std::cerr << "notebook page " << page_num << std::endl;
	m_airporteditornewairport->set_sensitive(!page_num);
	airporteditor_runway_selection_changed();
	airporteditor_helipad_selection_changed();
	airporteditor_comm_selection_changed();
	airporteditor_vfrroutes_selection_changed();
	airporteditor_vfrroute_selection_changed();
}

void AirportEditor::hide_notebook_pages(void)
{
	for (int pg = 1;; ++pg) {
		Gtk::Widget *w(m_airporteditornotebook->get_nth_page(pg));
		if (!w)
			break;
		//w->hide();
		w->set_sensitive(false);
	}
}

void AirportEditor::show_notebook_pages(void)
{
	for (int pg = 1;; ++pg) {
		Gtk::Widget *w(m_airporteditornotebook->get_nth_page(pg));
		if (!w)
			break;
		//w->show();
		w->set_sensitive(true);
	}
}

void AirportEditor::airporteditor_selection_changed(void)
{
	Glib::RefPtr<Gtk::TreeSelection> airporteditor_selection = m_airporteditortreeview->get_selection();
	std::vector<Gtk::TreeModel::Path> sel(airporteditor_selection->get_selected_rows());
	std::vector<Gtk::TreeModel::Path>::size_type selcount(sel.size());
	Gtk::TreeModel::iterator iter;
	if (selcount > 0)
		iter = m_airportliststore->get_iter(*sel.begin());
	if (selcount > 1) {
		// multiple selections
		std::cerr << "multiple selection" << std::endl;
		m_airporteditordeleteairport->set_sensitive(!m_airporteditornotebook->get_current_page());
		m_airporteditorairportsetelev->set_sensitive(!m_airporteditornotebook->get_current_page());
		m_curentryid = 0;
		m_vfrroute_index = -1;
		m_airporteditormap->invalidate_cursor();
		m_airportrunway_model->clear();
		m_airportcomm_model->clear();
		m_airportvfrroutes_model->clear();
		m_airportvfrroute_model->clear();
		hide_notebook_pages();
		return;
	}
	if (!selcount || !iter) {
		std::cerr << "selection cleared" << std::endl;
		m_airporteditordeleteairport->set_sensitive(false);
		m_airporteditorairportsetelev->set_sensitive(false);
		m_curentryid = 0;
		m_vfrroute_index = -1;
		m_airporteditormap->invalidate_cursor();
		m_airportrunway_model->clear();
		m_airportcomm_model->clear();
		m_airportvfrroutes_model->clear();
		m_airportvfrroute_model->clear();
		hide_notebook_pages();
		return;
	}
	Gtk::TreeModel::Row row = *iter;
	//Do something with the row.
	std::cerr << "selected row: " << row[m_airportlistcolumns.m_col_id] << std::endl;
	m_airporteditordeleteairport->set_sensitive(!m_airporteditornotebook->get_current_page());
	m_airporteditorairportsetelev->set_sensitive(!m_airporteditornotebook->get_current_page());
	m_curentryid = row[m_airportlistcolumns.m_col_id];
	m_vfrroute_index = -1;
	Point pt(row[m_airportlistcolumns.m_col_coord]);
	m_airporteditormap->set_center(pt, row[m_airportlistcolumns.m_col_elev]);
	m_airporteditormap->set_cursor(pt);
	set_lists((*m_db)(m_curentryid));
	show_notebook_pages();
}

void AirportEditor::airporteditor_runway_selection_changed(void)
{
	m_runwaydim->hide();
	if (!m_curentryid || 2 != m_airporteditornotebook->get_current_page()) {
		m_airporteditornewrunway->set_sensitive(false);
		m_airporteditordeleterunway->set_sensitive(false);
		m_airporteditorrunwaydim->set_sensitive(false);
		m_airporteditorrunwayhetole->set_sensitive(false);
		m_airporteditorrunwayletohe->set_sensitive(false);
		m_airporteditorrunwaycoords->set_sensitive(false);
		return;
	}
	m_airporteditornewrunway->set_sensitive(true);
	Glib::RefPtr<Gtk::TreeSelection> airporteditor_selection = m_airportrunwayeditortreeview->get_selection();
	std::vector<Gtk::TreeModel::Path> sel(airporteditor_selection->get_selected_rows());
	std::vector<Gtk::TreeModel::Path>::size_type selcount(sel.size());
	Gtk::TreeModel::iterator iter;
	if (selcount > 0)
		iter = m_airportrunway_model->get_iter(*sel.begin());
	if (selcount > 1) {
		// multiple selections
		std::cerr << "rwy: multiple selection" << std::endl;
		m_airporteditordeleterunway->set_sensitive(true);
		m_airporteditorrunwaydim->set_sensitive(false);
		m_airporteditorrunwayhetole->set_sensitive(false);
		m_airporteditorrunwayletohe->set_sensitive(false);
		m_airporteditorrunwaycoords->set_sensitive(false);
		return;
	}
	if (!selcount || !iter) {
		std::cerr << "rwy: selection cleared" << std::endl;
		m_airporteditordeleterunway->set_sensitive(false);
		m_airporteditorrunwaydim->set_sensitive(false);
		m_airporteditorrunwayhetole->set_sensitive(false);
		m_airporteditorrunwayletohe->set_sensitive(false);
		m_airporteditorrunwaycoords->set_sensitive(false);
		return;
	}
	Gtk::TreeModel::Row row = *iter;
	//Do something with the row.
	std::cerr << "rwy: selected row: " << row[m_airportrunway_model_columns.m_col_index] << std::endl;
	m_airporteditordeleterunway->set_sensitive(true);
	m_airporteditorrunwaydim->set_sensitive(true);
	m_airporteditorrunwayhetole->set_sensitive(true);
	m_airporteditorrunwayletohe->set_sensitive(true);
	m_airporteditorrunwaycoords->set_sensitive(true);
}

void AirportEditor::airporteditor_helipad_selection_changed(void)
{
	if (!m_curentryid || 3 != m_airporteditornotebook->get_current_page()) {
		m_airporteditornewhelipad->set_sensitive(false);
		m_airporteditordeletehelipad->set_sensitive(false);
		return;
	}
	m_airporteditornewhelipad->set_sensitive(true);
	Glib::RefPtr<Gtk::TreeSelection> airporteditor_selection = m_airporthelipadeditortreeview->get_selection();
	std::vector<Gtk::TreeModel::Path> sel(airporteditor_selection->get_selected_rows());
	std::vector<Gtk::TreeModel::Path>::size_type selcount(sel.size());
	Gtk::TreeModel::iterator iter;
	if (selcount > 0)
		iter = m_airporthelipad_model->get_iter(*sel.begin());
	if (selcount > 1) {
		// multiple selections
		std::cerr << "rwy: multiple selection" << std::endl;
		m_airporteditordeletehelipad->set_sensitive(true);
		return;
	}
	if (!selcount || !iter) {
		std::cerr << "rwy: selection cleared" << std::endl;
		m_airporteditordeletehelipad->set_sensitive(false);
		return;
	}
	Gtk::TreeModel::Row row = *iter;
	//Do something with the row.
	std::cerr << "rwy: selected row: " << row[m_airporthelipad_model_columns.m_col_index] << std::endl;
	m_airporteditordeletehelipad->set_sensitive(true);
}

void AirportEditor::airporteditor_comm_selection_changed(void)
{
	if (!m_curentryid || 4 != m_airporteditornotebook->get_current_page()) {
		m_airporteditornewcomm->set_sensitive(false);
		m_airporteditordeletecomm->set_sensitive(false);
		return;
	}
	m_airporteditornewcomm->set_sensitive(true);
	Glib::RefPtr<Gtk::TreeSelection> airporteditor_selection = m_airportcommeditortreeview->get_selection();
	std::vector<Gtk::TreeModel::Path> sel(airporteditor_selection->get_selected_rows());
	std::vector<Gtk::TreeModel::Path>::size_type selcount(sel.size());
	Gtk::TreeModel::iterator iter;
	if (selcount > 0)
		iter = m_airportcomm_model->get_iter(*sel.begin());
	if (selcount > 1) {
		// multiple selections
		std::cerr << "comm: multiple selection" << std::endl;
		m_airporteditordeletecomm->set_sensitive(true);
		return;
	}
	if (!selcount || !iter) {
		std::cerr << "comm: selection cleared" << std::endl;
		m_airporteditordeletecomm->set_sensitive(false);
		return;
	}
	Gtk::TreeModel::Row row = *iter;
	//Do something with the row.
	std::cerr << "comm: selected row: " << row[m_airportcomm_model_columns.m_col_index] << std::endl;
	m_airporteditordeletecomm->set_sensitive(true);
}

void AirportEditor::airporteditor_vfrroutes_selection_changed(void)
{
	if (!m_curentryid || 5 != m_airporteditornotebook->get_current_page()) {
		m_airporteditornewvfrroute->set_sensitive(false);
		m_airporteditordeletevfrroute->set_sensitive(false);
		m_airporteditorduplicatevfrroute->set_sensitive(false);
		m_airporteditorreversevfrroute->set_sensitive(false);
		m_airporteditornamevfrroute->set_sensitive(false);
		m_airporteditorduplicatereverseandnameallvfrroutes->set_sensitive(false);
		if (m_curentryid)
			return;
		m_vfrroute_index = -1;
		m_airportvfrroute_model->clear();
		m_airportvfrrouteeditortreeview->set_sensitive(false);
		return;
	}
	m_airporteditornewvfrroute->set_sensitive(true);
	m_airporteditorduplicatereverseandnameallvfrroutes->set_sensitive(!m_airportvfrroutes_model->children().empty());
	Glib::RefPtr<Gtk::TreeSelection> airporteditor_selection = m_airportvfrrouteseditortreeview->get_selection();
	std::vector<Gtk::TreeModel::Path> sel(airporteditor_selection->get_selected_rows());
	std::vector<Gtk::TreeModel::Path>::size_type selcount(sel.size());
	Gtk::TreeModel::iterator iter;
	if (selcount > 0)
		iter = m_airportvfrroutes_model->get_iter(*sel.begin());
	if (selcount > 1) {
		// multiple selections
		std::cerr << "vfrrte: multiple selection" << std::endl;
		m_airporteditordeletevfrroute->set_sensitive(true);
		m_airporteditorduplicatevfrroute->set_sensitive(false);
		m_airporteditorreversevfrroute->set_sensitive(false);
		m_airporteditornamevfrroute->set_sensitive(false);
		m_vfrroute_index = -1;
		m_airportvfrroute_model->clear();
		m_airportvfrrouteeditortreeview->set_sensitive(false);
		return;
	}
	if (!selcount || !iter) {
		std::cerr << "vfrrte: selection cleared" << std::endl;
		m_airporteditordeletevfrroute->set_sensitive(false);
		m_airporteditorduplicatevfrroute->set_sensitive(false);
		m_airporteditorreversevfrroute->set_sensitive(false);
		m_airporteditornamevfrroute->set_sensitive(false);
		m_vfrroute_index = -1;
		m_airportvfrroute_model->clear();
		m_airportvfrrouteeditortreeview->set_sensitive(false);
		return;
	}
	Gtk::TreeModel::Row row = *iter;
	//Do something with the row.
	std::cerr << "vfrrte: selected row: " << row[m_airportvfrroutes_model_columns.m_col_index] << std::endl;
	m_airporteditordeletevfrroute->set_sensitive(true);
	m_airporteditorduplicatevfrroute->set_sensitive(true);
	m_airporteditorreversevfrroute->set_sensitive(true);
	m_airporteditornamevfrroute->set_sensitive(true);
	m_vfrroute_index = -1;
	m_airportvfrrouteeditortreeview->set_sensitive(true);
	if (m_curentryid) {
		m_vfrroute_index = row[m_airportvfrroutes_model_columns.m_col_index];
		set_vfrroute((*m_db)(m_curentryid));
	}
}

void AirportEditor::airporteditor_vfrroute_selection_changed(void)
{
	m_airportvfrrouteeditorpointlist_conn.disconnect();
	if (!m_curentryid || m_vfrroute_index < 0 || 6 != m_airporteditornotebook->get_current_page()) {
		m_airporteditorappendvfrroutepoint->set_sensitive(false);
		m_airporteditorinsertvfrroutepoint->set_sensitive(false);
		m_airporteditordeletevfrroutepoint->set_sensitive(false);
		m_airportvfrrouteeditorappend->set_sensitive(false);
		m_airportvfrrouteeditorinsert->set_sensitive(false);
		m_airportvfrrouteeditordelete->set_sensitive(false);
		m_airportvfrrouteeditorpointlist->set_sensitive(false);
		return;
	}
	m_airporteditorappendvfrroutepoint->set_sensitive(true);
	m_airportvfrrouteeditorappend->set_sensitive(true);
	Glib::RefPtr<Gtk::TreeSelection> airporteditor_selection = m_airportvfrrouteeditortreeview->get_selection();
	std::vector<Gtk::TreeModel::Path> sel(airporteditor_selection->get_selected_rows());
	std::vector<Gtk::TreeModel::Path>::size_type selcount(sel.size());
	Gtk::TreeModel::iterator iter;
	if (selcount > 0)
		iter = m_airportvfrroute_model->get_iter(*sel.begin());
	if (selcount > 1) {
		// multiple selections
		std::cerr << "vfrrtept: multiple selection" << std::endl;
		m_airporteditorinsertvfrroutepoint->set_sensitive(false);
		m_airporteditordeletevfrroutepoint->set_sensitive(true);
		m_airportvfrrouteeditorinsert->set_sensitive(false);
		m_airportvfrrouteeditordelete->set_sensitive(true);
		m_airportvfrrouteeditorpointlist->set_sensitive(false);
		return;
	}
	if (!selcount || !iter) {
		std::cerr << "vfrrtept: selection cleared" << std::endl;
		m_airporteditorinsertvfrroutepoint->set_sensitive(false);
		m_airporteditordeletevfrroutepoint->set_sensitive(false);
		m_airportvfrrouteeditorinsert->set_sensitive(false);
		m_airportvfrrouteeditordelete->set_sensitive(false);
		m_airportvfrrouteeditorpointlist->set_sensitive(false);
		return;
	}
	Gtk::TreeModel::Row row = *iter;
	//Do something with the row.
	std::cerr << "vfrrtept: selected row: " << row[m_airportvfrroute_model_columns.m_col_index] << std::endl;
	m_airporteditorinsertvfrroutepoint->set_sensitive(true);
	m_airporteditordeletevfrroutepoint->set_sensitive(true);
	m_airportvfrrouteeditorinsert->set_sensitive(true);
	m_airportvfrrouteeditordelete->set_sensitive(true);
	m_airportvfrrouteeditorpointlist->set_sensitive(true);
	{
		//std::cerr << "Selected: Name " << row[m_airportvfrroute_model_columns.m_col_name]
		//	  << " Coord " << Point(row[m_airportvfrroute_model_columns.m_col_lon], row[m_airportvfrroute_model_columns.m_col_lat])
		//	  << " AtAirport " << row[m_airportvfrroute_model_columns.m_col_at_airport] << std::endl;
		Gtk::TreeModel::iterator iterpl = m_airportvfrpointlist_model->children().begin();
		for (; iterpl; ++iterpl) {
			Gtk::TreeModel::Row rowpl = *iterpl;
			//std::cerr << "Model: Name " << rowpl[m_airportvfrpointlist_model_columns.m_col_name]
			//	  << " Coord " << Point(rowpl[m_airportvfrpointlist_model_columns.m_col_lon], rowpl[m_airportvfrpointlist_model_columns.m_col_lat])
			//	  << " AtAirport " << rowpl[m_airportvfrpointlist_model_columns.m_col_at_airport] << std::endl;
			if (row[m_airportvfrroute_model_columns.m_col_at_airport] != rowpl[m_airportvfrpointlist_model_columns.m_col_at_airport])
				continue;
			if (rowpl[m_airportvfrpointlist_model_columns.m_col_at_airport])
				break;
			if (row[m_airportvfrroute_model_columns.m_col_name] == rowpl[m_airportvfrpointlist_model_columns.m_col_name] &&
			    (Point)row[m_airportvfrroute_model_columns.m_col_coord] == (Point)rowpl[m_airportvfrpointlist_model_columns.m_col_coord])
				break;
		}
		//std::cerr << "Pointlist active " << (iterpl ? "FOUND" : "not found") << std::endl;
		if (iterpl)
			m_airportvfrrouteeditorpointlist->set_active(iterpl);
		else
			m_airportvfrrouteeditorpointlist->unset_active();
	}
	m_airportvfrrouteeditorpointlist_conn = m_airportvfrrouteeditorpointlist->signal_changed().connect(sigc::mem_fun(*this, &AirportEditor::edited_vfrroute_pointlist));
}

void AirportEditor::aboutdialog_response( int response )
{
	if (response == Gtk::RESPONSE_CLOSE || response == Gtk::RESPONSE_CANCEL)
		m_aboutdialog->hide();
}

int main(int argc, char *argv[])
{
	try {
		std::string dir_main(""), dir_aux("");
		Engine::auxdb_mode_t auxdbmode = Engine::auxdb_prefs;
		bool dis_aux(false);
		Glib::init();
		Glib::OptionGroup optgroup("airporteditor", "Airport Editor Options", "Options controlling the Airport Editor");
		Glib::OptionEntry optmaindir, optauxdir, optdisableaux;
		optmaindir.set_short_name('m');
		optmaindir.set_long_name("maindir");
		optmaindir.set_description("Directory containing the main database");
		optmaindir.set_arg_description("DIR");
		optauxdir.set_short_name('a');
		optauxdir.set_long_name("auxdir");
		optauxdir.set_description("Directory containing the auxilliary database");
		optauxdir.set_arg_description("DIR");
		optdisableaux.set_short_name('A');
		optdisableaux.set_long_name("disableaux");
		optdisableaux.set_description("Disable the auxilliary database");
		optdisableaux.set_arg_description("BOOL");
		optgroup.add_entry_filename(optmaindir, dir_main);
		optgroup.add_entry_filename(optauxdir, dir_aux);
		optgroup.add_entry(optdisableaux, dis_aux);
#ifdef HAVE_PQXX
		Glib::ustring pg_conn;
		Glib::OptionEntry optpgsql;
		optpgsql.set_short_name('p');
		optpgsql.set_long_name("pgsql");
		optpgsql.set_description("PostgreSQL Connect String");
		optpgsql.set_arg_description("CONNSTR");
		optgroup.add_entry(optpgsql, pg_conn);
#endif
		Glib::OptionContext optctx("[--maindir=<dir>] [--auxdir=<dir>] [--disableaux]");
		optctx.set_help_enabled(true);
		optctx.set_ignore_unknown_options(false);
		optctx.set_main_group(optgroup);
		Gtk::Main m(argc, argv, optctx);
#ifdef HAVE_PQXX
		if (!pg_conn.empty()) {
			dir_main = pg_conn;
			dir_aux.clear();
			pg_conn.clear();
			auxdbmode = Engine::auxdb_postgres;
		} else
#endif
		if (dis_aux)
			auxdbmode = Engine::auxdb_none;
		else if (!dir_aux.empty())
			auxdbmode = Engine::auxdb_override;
#ifdef HAVE_OSSO
		osso_context_t* osso_context = osso_initialize("vfrairporteditor", VERSION, TRUE /* deprecated parameter */, 0 /* Use default Glib main loop context */);
		if (!osso_context) {
			std::cerr << "osso_initialize() failed." << std::endl;
			return OSSO_ERROR;
		}
#endif
#ifdef HAVE_HILDON
		Hildon::init();
#endif
		Glib::set_application_name("VFR Airport Editor");
		Glib::thread_init();
		AirportEditor editor(dir_main, auxdbmode, dir_aux);

		Gtk::Main::run(*editor.get_mainwindow());
#ifdef HAVE_OSSO
		osso_deinitialize(osso_context);
#endif
	} catch (const Glib::Exception& e) {
		std::cerr << "glib exception: " << e.what() << std::endl;
		return 1;
	} catch (const std::exception& e) {
		std::cerr << "exception: " << e.what() << std::endl;
		return 1;
	}
	return 0;
}
