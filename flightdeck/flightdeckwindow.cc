//
// C++ Implementation: FlightDeckWindow
//
// Description: Flight Deck Windows
//
//
// Author: Thomas Sailer <t.sailer@alumni.ethz.ch>, (C) 2012, 2013, 2014, 2016
//
// Copyright: See COPYING file that comes with this distribution
//
//

#include "sysdeps.h"

#include <sstream>
#include <cstring>

#include "flightdeckwindow.h"
#include "wmm.h"
#include "SunriseSunset.h"
#include "baro.h"
#include "sysdepsgui.h"

#ifdef HAVE_X11_XLIB_H
#include <X11/Xlib.h>
#endif

#ifdef HAVE_X11_EXTENSIONS_SCRNSAVER_H
#include <X11/extensions/scrnsaver.h>
#endif

#ifdef HAVE_X11_EXTENSIONS_DPMS_H
#include <X11/extensions/dpms.h>
#endif

#ifdef HAVE_X11_RSS
#include <gdk/gdkx.h>
#endif

#ifdef HAVE_WINDOWS_H
#include <windows.h>
#endif

FullscreenableWindow::FullscreenableWindow(BaseObjectType * castitem, const Glib::RefPtr< Builder > & refxml)
        : TOPLEVELWINDOW(castitem), m_fullscreen(false)
{
#ifndef GLIBMM_DEFAULT_SIGNAL_HANDLERS_ENABLED
        signal_window_state_event().connect(sigc::mem_fun(*this, &FullscreenableWindow::on_window_state_event));
        signal_key_press_event().connect(sigc::mem_fun(*this, &FullscreenableWindow::on_key_press_event));
#endif //GLIBMM_DEFAULT_SIGNAL_HANDLERS_ENABLED
}

FullscreenableWindow::~FullscreenableWindow()
{
}

bool FullscreenableWindow::on_window_state_event(GdkEventWindowState *state)
{
        if (state)
		m_fullscreen = !!(Gdk::WINDOW_STATE_FULLSCREEN & state->new_window_state);
#ifdef GLIBMM_DEFAULT_SIGNAL_HANDLERS_ENABLED
	if (TOPLEVELWINDOW::on_window_state_event(state))
		return true;
#endif //GLIBMM_DEFAULT_SIGNAL_HANDLERS_ENABLED
	return false;
}

void FullscreenableWindow::toggle_fullscreen(void)
{
        if (m_fullscreen)
                unfullscreen();
        else
                fullscreen();
}

bool FullscreenableWindow::on_key_press_event(GdkEventKey * event)
{
        if (event) {
		//std::cerr << "key press: type " << event->type << " keyval " << event->keyval << std::endl;
		if (event->type == GDK_KEY_PRESS) {
			switch (event->keyval) {
			case GDK_KEY_F6:
				toggle_fullscreen();
				return true;
				
			default:
				break;
			}
		}
	}
#ifdef GLIBMM_DEFAULT_SIGNAL_HANDLERS_ENABLED
	if (TOPLEVELWINDOW::on_key_press_event(event))
		return true;
#endif //GLIBMM_DEFAULT_SIGNAL_HANDLERS_ENABLED
	return false;
}

FlightDeckWindow::perfunits_t FlightDeckWindow::prefvalueunits[perfval_nr] = {
	perfunits_mass,
	perfunits_pressure,
	perfunits_altitude,
	perfunits_temperature,
	perfunits_temperature,
	perfunits_temperature,
	perfunits_speed,
};

const char FlightDeckWindow::cfggroup_mainwindow[] = "mainwindow";
const char FlightDeckWindow::cfgkey_mainwindow_fullscreen[] = "fullscreen";
const char FlightDeckWindow::cfgkey_mainwindow_onscreenkeyboard[] = "onscreenkeyboard";
const char FlightDeckWindow::cfgkey_mainwindow_fplid[] = "flightplan";
const char FlightDeckWindow::cfgkey_mainwindow_docdirs[] = "docdirs";
const char FlightDeckWindow::cfgkey_mainwindow_docalltoken[] = "docalltoken";
const char FlightDeckWindow::cfgkey_mainwindow_grib2mapscale[] = "grib2mapscale";
const char FlightDeckWindow::cfgkey_mainwindow_grib2mapflags[] = "grib2mapflags";

FlightDeckWindow::FlightDeckWindow(BaseObjectType *castitem, const Glib::RefPtr<Builder>& refxml)
	: FullscreenableWindow(castitem, refxml), m_engine(0), m_sensors(0), m_screensaver(screensaver_default),
	  m_altdialog(0), m_hsidialog(0), m_mapdialog(0), m_maptiledialog(0), m_textentrydialog(0), m_keyboarddialog(0),
	  m_onscreenkeyboard(true), m_coorddialog(0), m_suspenddialog(0), m_fplatmodialog(0), m_fplverifdialog(0), m_fplautoroutedialog(0),
	  m_nwxchartdialog(0), m_mainnotebook(0), m_mainvbox(0), m_sensorcfgframe(0), m_magcalibdialog(0),
	  m_wptframelabel(0), m_wpttreeview(0), m_wpttogglebuttonnearest(0), m_wpttogglebuttonfind(0), m_wptbuttoninsert(0),
	  m_wptbuttondirectto(0), m_wpttogglebuttonbrg2(0), m_wpttogglebuttonmap(0), m_wptdrawingarea(0), m_wptnotebook(0),
	  m_wptarpticaoentry(0), m_wptarptnameentry(0), m_wptarptlatentry(0), m_wptarptlonentry(0), m_wptarpteleventry(0),
	  m_wptarpttypeentry(0), m_wptarptnotebook(0), m_wptarptremarktextview(0), m_wptarptrwytreeview(0), m_wptarpthelitreeview(0),
	  m_wptarptcommtreeview(0), m_wptarptfastreeview(0), m_wptnavaidicaoentry(0), m_wptnavaidnameentry(0), m_wptnavaidlatentry(0),
	  m_wptnavaidlonentry(0), m_wptnavaideleventry(0), m_wptnavaidtypeentry(0), m_wptnavaidfreqentry(0),
	  m_wptnavaidrangeentry(0), m_wptnavaidinservicecheckbutton(0), m_wptinticaoentry(0), m_wptintnameentry(0),
	  m_wptintlatentry(0), m_wptintlonentry(0), m_wptintusageentry(0), m_wptawynameentry(0), m_wptawybeginlonentry(0),
	  m_wptawybeginlatentry(0), m_wptawyendlonentry(0), m_wptawyendlatentry(0), m_wptawybeginnameentry(0), m_wptawyendnameentry(0),
	  m_wptawybasealtentry(0), m_wptawytopaltentry(0), m_wptawyteleventry(0), m_wptawyt5eleventry(0),
	  m_wptawytypeentry(0), m_wptaspcicaoentry(0), m_wptaspcnameentry(0),
	  m_wptaspcidententry(0), m_wptaspcclassexceptentry(0), m_wptaspcclassentry(0), m_wptaspcfromaltentry(0),
	  m_wptaspctoaltentry(0), m_wptaspccommnameentry(0), m_wptaspccomm0entry(0), m_wptaspccomm1entry(0), m_wptaspcnotebook(0),
	  m_wptaspceffectivetextview(0), m_wptaspcnotetextview(0), m_wptmapelnameentry(0), m_wptmapeltypeentry(0),
          m_wptfpldbicaoentry(0), m_wptfpldbnameentry(0), m_wptfpldblatentry(0), m_wptfpldblonentry(0), m_wptfpldbaltentry(0),
	  m_wptfpldbfreqentry(0), m_wptfpldbtypeentry(0), m_wptfpldbnotetextview(0), m_wptcoordlatentry(0), m_wptcoordlonentry(0),
	  m_wpttexttextentry(0), m_fpltreeview(0), m_fplinsertbutton(0), m_fplinserttxtbutton(0), m_fplduplicatebutton(0),
	  m_fplmoveupbutton(0), m_fplmovedownbutton(0), m_fpldeletebutton(0), m_fplstraightenbutton(0), m_fpldirecttobutton(0),
	  m_fplbrg2togglebutton(0), m_fpldrawingarea(0), m_fplnotebook(0), m_fplicaoentry(0), m_fplnameentry(0),
	  m_fplpathcodecombobox(0), m_fplpathnameentry(0), m_fplcoordentry(0),
	  m_fplcoordeditbutton(0), m_fplfreqentry(0), m_fplfrequnitlabel(0), m_fplaltentry(0), m_fplaltunitcombobox(0),
	  m_fpllevelupbutton(0), m_fplleveldownbutton(0), m_fplsemicircularbutton(0), m_fplifrcheckbutton(0), m_fpltypecombobox(0),
	  m_fplwinddirspinbutton(0), m_fplwindspeedspinbutton(0), m_fplqffspinbutton(0), m_fploatspinbutton(0), m_fplwptnotestextview(0),
	  m_fplnotestextview(0), m_fpldbtreeview(0), m_fpldbnewbutton(0), m_fpldbloadbutton(0), m_fpldbduplicatebutton(0),
	  m_fpldbdeletebutton(0), m_fpldbreversebutton(0), m_fpldbimportbutton(0), m_fpldbexportbutton(0), m_fpldbdirecttobutton(0),
	  m_fpldbcfmwindow(0), m_fpldbcfmlabel(0), m_fpldbcfmtreeview(0), m_fpldbcfmbuttoncancel(0), m_fpldbcfmbuttonok(0),
	  m_fplgpsimpdrawingarea(0), m_fplgpsimptreeview(0), m_fplgpsimplabel(0), m_vertprofiledrawingarea(0), m_wxpiclabel(0), m_wxpicimage(0),
	  m_wxpicalignment(0), m_metartaftreeview(0), m_wxchartlisttreeview(0), m_docdirframe(0), m_docdirtreeview(0),
	  m_docframelabel(0), m_docframe(0), m_docscrolledwindow(0),
	  m_perfbuttonfromwb(0), m_perfbuttonfromfpl(0), m_perfspinbuttonsoftsurface(0),
	  m_perftreeviewwb(0), m_perfdrawingareawb(0), m_srsstreeview(0), m_srsscalendar(0),
	  m_euops1comboboxapproach(0), m_euops1spinbuttontdzelev(0), m_euops1comboboxtdzelevunit(0), m_euops1comboboxtdzlist(0),
	  m_euops1comboboxocaoch(0), m_euops1spinbuttonocaoch(0), m_euops1comboboxocaochunit(0),
	  m_euops1labeldamda(0), m_euops1entrydamda(0), m_euops1buttonsetda(0), m_euops1entryrvr(0), m_euops1entrymetvis(0), 
	  m_euops1comboboxlight(0), m_euops1comboboxmetvis(0), m_euops1checkbuttonrwyedgelight(0), m_euops1checkbuttonrwycentreline(0), 
	  m_euops1checkbuttonmultiplervr(0), m_euops1entrytkoffrvr(0), m_euops1entrytkoffmetvis(0), m_logtreeview(0),
	  m_grib2drawingarea(0), m_grib2layertreeview(0), m_grib2layerabbreventry(0), m_grib2layerunitentry(0),
	  m_grib2layerparamidentry(0), m_grib2layerparamentry(0), m_grib2layerparamcatidentry(0), m_grib2layerdisciplineidentry(0),
	  m_grib2layerparamcatentry(0), m_grib2layerdisciplineentry(0), m_grib2layerefftimeentry(0), m_grib2layerreftimeentry(0),
	  m_grib2layersurface1entry(0), m_grib2layersurface2entry(0), m_grib2layersurface1valueentry(0), m_grib2layersurface2valueentry(0),
	  m_grib2layersurface1identry(0), m_grib2layersurface2identry(0), m_grib2layersurface1unitentry(0), m_grib2layersurface2unitentry(0),
	  m_grib2layercenteridentry(0), m_grib2layersubcenteridentry(0), m_grib2layerprodstatusidentry(0), m_grib2layerdatatypeidentry(0),
	  m_grib2layergenprocidentry(0), m_grib2layergenproctypidentry(0), m_grib2layercenterentry(0), m_grib2layersubcenterentry(0),
	  m_grib2layerprodstatusentry(0), m_grib2layerdatatypeentry(0), m_grib2layergenprocentry(0), m_grib2layergenproctypentry(0),
	  m_wptpagemode(wptpagemode_nearest), m_wptqueryrect(Point(0, Point::pole_lat), Point(0, Point::pole_lat)),
	  m_fplchg(fplchg_none), m_fpldbcfmmode(fpldbcfmmode_none), 
	  m_fpldbimportfilechooser(*this, "Import Flight Plan(s)...", Gtk::FILE_CHOOSER_ACTION_OPEN),
	  m_fpldbexportfilechooser(*this, "Export Flight Plan(s)...", Gtk::FILE_CHOOSER_ACTION_SAVE),
	  m_fpldbexportformattext("File Format:", 0.0, 0.5), m_fplgpsimproute(*(FPlan *)0),
	  m_wxchartexportfilechooser(*this, "Export Charts...", Gtk::FILE_CHOOSER_ACTION_SAVE),
#ifdef HAVE_EVINCE
	  m_docevdocmodel(0), m_docevdocmodel_sighandler(0), m_docevview(0),
#endif
	  m_euops1tdzelev(500), m_euops1ocaoch(500), m_euops1minimum(700),
	  m_perfsoftfieldpenalty(0.15), m_logdisplaylevel(Sensors::loglevel_notice), m_menuid(0), m_admin(false)
{
#ifndef GLIBMM_DEFAULT_SIGNAL_HANDLERS_ENABLED
        signal_delete_event().connect(sigc::mem_fun(*this, &FlightDeckWindow::on_delete_event));
        signal_key_press_event().connect(sigc::mem_fun(*this, &FlightDeckWindow::on_key_press_event));
        signal_show().connect(sigc::mem_fun(*this, &FlightDeckWindow::on_show));
        signal_hide().connect(sigc::mem_fun(*this, &FlightDeckWindow::on_hide));
#endif //GLIBMM_DEFAULT_SIGNAL_HANDLERS_ENABLED

	m_wxdb.open(FPlan::get_userdbpath(), "wx.db");
	m_softkeysconn = m_softkeys.signal_clicked().connect(sigc::mem_fun(*this, &FlightDeckWindow::menubutton_clicked));
	get_widget_derived(refxml, "altwindow", m_altdialog);
	get_widget_derived(refxml, "hsiwindow", m_hsidialog);
	get_widget_derived(refxml, "mapwindow", m_mapdialog);
	get_widget_derived(refxml, "fplmaptilewindow", m_maptiledialog);
	get_widget_derived(refxml, "textentrywindow", m_textentrydialog);
	get_widget_derived(refxml, "keyboardwindow", m_keyboarddialog);
	get_widget_derived(refxml, "coordwindow", m_coorddialog);
	get_widget_derived(refxml, "suspendwindow", m_suspenddialog);
	get_widget_derived(refxml, "fplatmowindow", m_fplatmodialog);
	get_widget_derived(refxml, "fplverifwindow", m_fplverifdialog);
	get_widget_derived(refxml, "fplautoroutewindow", m_fplautoroutedialog);
	get_widget_derived(refxml, "nwxchartwindow", m_nwxchartdialog);
	get_widget(refxml, "mainnotebook", m_mainnotebook);
	get_widget(refxml, "mainvbox", m_mainvbox);
	get_widget(refxml, "sensorcfgframe", m_sensorcfgframe);
	get_widget_derived(refxml, "magcalibwindow", m_magcalibdialog);
	if (m_altdialog)
		m_altdialog->set_transient_for(*this);
	if (m_hsidialog)
		m_hsidialog->set_transient_for(*this);
	if (m_mapdialog)
		m_mapdialog->set_transient_for(*this);
	if (m_maptiledialog)
		m_maptiledialog->set_transient_for(*this);
	if (m_textentrydialog)
		m_textentrydialog->set_transient_for(*this);
	if (m_keyboarddialog) {
		m_keyboarddialog->set_focus_on_map(false);
		m_keyboarddialog->set_accept_focus(false);
		m_keyboarddialog->set_transient_for(*this);
		m_keyboarddialog->set_keep_above(true);
	}
	if (m_coorddialog) {
		m_coorddialog->set_transient_for(*this);
		m_coorddialog->init(this);
		m_coorddialog->set_flightdeckwindow(this);
	}
	if (m_suspenddialog)
		m_suspenddialog->set_transient_for(*this);
	if (m_fplatmodialog)
		m_fplatmodialog->set_transient_for(*this);
	if (m_fplverifdialog)
		m_fplverifdialog->set_transient_for(*this);
	if (m_fplautoroutedialog)
		m_fplautoroutedialog->set_transient_for(*this);
	if (m_nwxchartdialog)
		m_nwxchartdialog->set_transient_for(*this);
	// Waypoint Page
	get_widget(refxml, "wptframelabel", m_wptframelabel);
	get_widget(refxml, "wpttreeview", m_wpttreeview);
	get_widget(refxml, "wpttogglebuttonnearest", m_wpttogglebuttonnearest);
	get_widget(refxml, "wpttogglebuttonfind", m_wpttogglebuttonfind);
	get_widget(refxml, "wptbuttoninsert", m_wptbuttoninsert);
	get_widget(refxml, "wptbuttondirectto", m_wptbuttondirectto);
	get_widget(refxml, "wpttogglebuttonbrg2", m_wpttogglebuttonbrg2);
	get_widget(refxml, "wpttogglebuttonmap", m_wpttogglebuttonmap);
	get_widget_derived(refxml, "wptdrawingarea", m_wptdrawingarea);
	get_widget(refxml, "wptnotebook", m_wptnotebook);
	get_widget(refxml, "wptarpticaoentry", m_wptarpticaoentry);
	get_widget(refxml, "wptarptnameentry", m_wptarptnameentry);
	get_widget(refxml, "wptarptlatentry", m_wptarptlatentry);
	get_widget(refxml, "wptarptlonentry", m_wptarptlonentry);
	get_widget(refxml, "wptarpteleventry", m_wptarpteleventry);
	get_widget(refxml, "wptarpttypeentry", m_wptarpttypeentry);
	get_widget(refxml, "wptarptnotebook", m_wptarptnotebook);
	get_widget(refxml, "wptarptremarktextview", m_wptarptremarktextview);
	get_widget(refxml, "wptarptrwytreeview", m_wptarptrwytreeview);
	get_widget(refxml, "wptarpthelitreeview", m_wptarpthelitreeview);
	get_widget(refxml, "wptarptcommtreeview", m_wptarptcommtreeview);
	get_widget(refxml, "wptarptfastreeview", m_wptarptfastreeview);
	get_widget(refxml, "wptnavaidicaoentry", m_wptnavaidicaoentry);
	get_widget(refxml, "wptnavaidnameentry", m_wptnavaidnameentry);
	get_widget(refxml, "wptnavaidlatentry", m_wptnavaidlatentry);
	get_widget(refxml, "wptnavaidlonentry", m_wptnavaidlonentry);
	get_widget(refxml, "wptnavaideleventry", m_wptnavaideleventry);
	get_widget(refxml, "wptnavaidtypeentry", m_wptnavaidtypeentry);
	get_widget(refxml, "wptnavaidfreqentry", m_wptnavaidfreqentry);
	get_widget(refxml, "wptnavaidrangeentry", m_wptnavaidrangeentry);
	get_widget(refxml, "wptnavaidinservicecheckbutton", m_wptnavaidinservicecheckbutton);
	get_widget(refxml, "wptinticaoentry", m_wptinticaoentry);
	get_widget(refxml, "wptintnameentry", m_wptintnameentry);
	get_widget(refxml, "wptintlatentry", m_wptintlatentry);
	get_widget(refxml, "wptintlonentry", m_wptintlonentry);
	get_widget(refxml, "wptintusageentry", m_wptintusageentry);
	get_widget(refxml, "wptawynameentry", m_wptawynameentry);
	get_widget(refxml, "wptawybeginlonentry", m_wptawybeginlonentry);
	get_widget(refxml, "wptawybeginlatentry", m_wptawybeginlatentry);
	get_widget(refxml, "wptawyendlonentry", m_wptawyendlonentry);
	get_widget(refxml, "wptawyendlatentry", m_wptawyendlatentry);
	get_widget(refxml, "wptawybeginnameentry", m_wptawybeginnameentry);
	get_widget(refxml, "wptawyendnameentry", m_wptawyendnameentry);
	get_widget(refxml, "wptawybasealtentry", m_wptawybasealtentry);
	get_widget(refxml, "wptawytopaltentry", m_wptawytopaltentry);
	get_widget(refxml, "wptawyteleventry", m_wptawyteleventry);
	get_widget(refxml, "wptawyt5eleventry", m_wptawyt5eleventry);
	get_widget(refxml, "wptawytypeentry", m_wptawytypeentry);
	get_widget(refxml, "wptaspcicaoentry", m_wptaspcicaoentry);
	get_widget(refxml, "wptaspcnameentry", m_wptaspcnameentry);
	get_widget(refxml, "wptaspcidententry", m_wptaspcidententry);
	get_widget(refxml, "wptaspcclassexceptentry", m_wptaspcclassexceptentry);
	get_widget(refxml, "wptaspcclassentry", m_wptaspcclassentry);
	get_widget(refxml, "wptaspcfromaltentry", m_wptaspcfromaltentry);
	get_widget(refxml, "wptaspctoaltentry", m_wptaspctoaltentry);
	get_widget(refxml, "wptaspccommnameentry", m_wptaspccommnameentry);
	get_widget(refxml, "wptaspccomm0entry", m_wptaspccomm0entry);
	get_widget(refxml, "wptaspccomm1entry", m_wptaspccomm1entry);
	get_widget(refxml, "wptaspcnotebook", m_wptaspcnotebook);
	get_widget(refxml, "wptaspceffectivetextview", m_wptaspceffectivetextview);
	get_widget(refxml, "wptaspcnotetextview", m_wptaspcnotetextview);
	get_widget(refxml, "wptmapelnameentry", m_wptmapelnameentry);
	get_widget(refxml, "wptmapeltypeentry", m_wptmapeltypeentry);
        get_widget(refxml, "wptfpldbicaoentry", m_wptfpldbicaoentry);
        get_widget(refxml, "wptfpldbnameentry", m_wptfpldbnameentry);
        get_widget(refxml, "wptfpldblatentry", m_wptfpldblatentry);
        get_widget(refxml, "wptfpldblonentry", m_wptfpldblonentry);
        get_widget(refxml, "wptfpldbaltentry", m_wptfpldbaltentry);
	get_widget(refxml, "wptfpldbfreqentry", m_wptfpldbfreqentry);
        get_widget(refxml, "wptfpldbtypeentry", m_wptfpldbtypeentry);
	get_widget(refxml, "wptfpldbnotetextview", m_wptfpldbnotetextview);
	get_widget(refxml, "wptcoordlatentry", m_wptcoordlatentry);
	get_widget(refxml, "wptcoordlonentry", m_wptcoordlonentry);
	get_widget(refxml, "wpttexttextentry", m_wpttexttextentry);
	// Flight Plan Page
	get_widget(refxml, "fpltreeview", m_fpltreeview);
	get_widget(refxml, "fplinsertbutton", m_fplinsertbutton);
	get_widget(refxml, "fplinserttxtbutton", m_fplinserttxtbutton);
	get_widget(refxml, "fplduplicatebutton", m_fplduplicatebutton);
	get_widget(refxml, "fplmoveupbutton", m_fplmoveupbutton);
	get_widget(refxml, "fplmovedownbutton", m_fplmovedownbutton);
	get_widget(refxml, "fpldeletebutton", m_fpldeletebutton);
	get_widget(refxml, "fplstraightenbutton", m_fplstraightenbutton);
	get_widget(refxml, "fpldirecttobutton", m_fpldirecttobutton);
	get_widget(refxml, "fplbrg2togglebutton", m_fplbrg2togglebutton);
	get_widget_derived(refxml, "fpldrawingarea", m_fpldrawingarea);
	get_widget(refxml, "fplnotebook", m_fplnotebook);
	get_widget(refxml, "fplicaoentry", m_fplicaoentry);
	get_widget(refxml, "fplnameentry", m_fplnameentry);
	get_widget(refxml, "fplpathcodecombobox", m_fplpathcodecombobox);
	get_widget(refxml, "fplpathnameentry", m_fplpathnameentry);
	get_widget(refxml, "fplcoordentry", m_fplcoordentry);
	get_widget(refxml, "fplcoordeditbutton", m_fplcoordeditbutton);
	get_widget(refxml, "fplfreqentry", m_fplfreqentry);
	get_widget(refxml, "fplfrequnitlabel", m_fplfrequnitlabel);
	get_widget(refxml, "fplaltentry", m_fplaltentry);
	get_widget(refxml, "fplaltunitcombobox", m_fplaltunitcombobox);
	get_widget(refxml, "fpllevelupbutton", m_fpllevelupbutton);
	get_widget(refxml, "fplleveldownbutton", m_fplleveldownbutton);
	get_widget(refxml, "fplsemicircularbutton", m_fplsemicircularbutton);
	get_widget(refxml, "fpltypecombobox", m_fpltypecombobox);
	get_widget(refxml, "fplifrcheckbutton", m_fplifrcheckbutton);
	get_widget(refxml, "fplwinddirspinbutton", m_fplwinddirspinbutton);
	get_widget(refxml, "fplwindspeedspinbutton", m_fplwindspeedspinbutton);
	get_widget(refxml, "fplqffspinbutton", m_fplqffspinbutton);
	get_widget(refxml, "fploatspinbutton", m_fploatspinbutton);
	get_widget(refxml, "fplwptnotestextview", m_fplwptnotestextview);
	get_widget(refxml, "fplnotestextview", m_fplnotestextview);
	// Flight Plan Database Page
	get_widget(refxml, "fpldbtreeview", m_fpldbtreeview);
	get_widget(refxml, "fpldbnewbutton", m_fpldbnewbutton);
	get_widget(refxml, "fpldbloadbutton", m_fpldbloadbutton);
	get_widget(refxml, "fpldbduplicatebutton", m_fpldbduplicatebutton);
	get_widget(refxml, "fpldbdeletebutton", m_fpldbdeletebutton);
	get_widget(refxml, "fpldbreversebutton", m_fpldbreversebutton);
	get_widget(refxml, "fpldbimportbutton", m_fpldbimportbutton);
	get_widget(refxml, "fpldbexportbutton", m_fpldbexportbutton);
	get_widget(refxml, "fpldbdirecttobutton", m_fpldbdirecttobutton);
	get_widget(refxml, "fpldbcfmwindow", m_fpldbcfmwindow);
	get_widget(refxml, "fpldbcfmlabel", m_fpldbcfmlabel);
	get_widget(refxml, "fpldbcfmtreeview", m_fpldbcfmtreeview);
	get_widget(refxml, "fpldbcfmbuttoncancel", m_fpldbcfmbuttoncancel);
	get_widget(refxml, "fpldbcfmbuttonok", m_fpldbcfmbuttonok);
	// Flight Plan GPS Import Page
	get_widget_derived(refxml, "fplgpsimpdrawingarea", m_fplgpsimpdrawingarea);
	get_widget(refxml, "fplgpsimptreeview", m_fplgpsimptreeview);
	get_widget(refxml, "fplgpsimplabel", m_fplgpsimplabel);
	// Route Vertical Profile Drawing Area
	get_widget_derived(refxml, "vertprofiledrawingarea", m_vertprofiledrawingarea);
	// WX Picture Page
	get_widget(refxml, "wxpiclabel", m_wxpiclabel);
	get_widget_derived(refxml, "wxpicimage", m_wxpicimage);
	get_widget(refxml, "wxpicalignment", m_wxpicalignment);
	// METAR/TAF Page
	get_widget(refxml, "metartaftreeview", m_metartaftreeview);
	// Wx Chart List Page
	get_widget(refxml, "wxchartlisttreeview", m_wxchartlisttreeview);
	// Documents Directory Page
	get_widget(refxml, "docdirframe", m_docdirframe);
	get_widget(refxml, "docdirtreeview", m_docdirtreeview);
	// Documents Page
	get_widget(refxml, "docframelabel", m_docframelabel);
	get_widget(refxml, "docframe", m_docframe);
	get_widget(refxml, "docscrolledwindow", m_docscrolledwindow);
	// Performace Page
	for (unsigned int i = 0; i < perfunits_nr; ++i)
		m_perfcombobox[i] = 0;
	for (unsigned int i = 0; i < perfval_nr; ++i)
		m_perfspinbutton[0][i] = m_perfspinbutton[1][i] = 0;
	m_perfentryname[0] = m_perfentryname[1] = 0;
	m_perftreeview[0] = m_perftreeview[1] = 0;
	get_widget(refxml, "perfbuttonfromwb", m_perfbuttonfromwb);
	get_widget(refxml, "perfbuttonfromfpl", m_perfbuttonfromfpl);
	get_widget(refxml, "perfcomboboxmassunit", m_perfcombobox[perfunits_mass]);
	get_widget(refxml, "perfcomboboxaltunit", m_perfcombobox[perfunits_altitude]);
	get_widget(refxml, "perfcomboboxqnhunit", m_perfcombobox[perfunits_pressure]);
	get_widget(refxml, "perfcomboboxtempunit", m_perfcombobox[perfunits_temperature]);
	get_widget(refxml, "perfcomboboxwindunit", m_perfcombobox[perfunits_speed]);
	get_widget(refxml, "perfspinbuttonsoftsurface", m_perfspinbuttonsoftsurface);
	get_widget(refxml, "perfspinbuttontkoffmass", m_perfspinbutton[0][perfval_mass]);
	get_widget(refxml, "perfspinbuttonldgmass", m_perfspinbutton[1][perfval_mass]);
	get_widget(refxml, "perfspinbuttondepqnh", m_perfspinbutton[0][perfval_qnh]);
	get_widget(refxml, "perfspinbuttondestqnh", m_perfspinbutton[1][perfval_qnh]);
	get_widget(refxml, "perfspinbuttondepalt", m_perfspinbutton[0][perfval_altitude]);
	get_widget(refxml, "perfspinbuttondestalt", m_perfspinbutton[1][perfval_altitude]);
	get_widget(refxml, "perfentrydepname", m_perfentryname[0]);
	get_widget(refxml, "perfentrydestname", m_perfentryname[1]);
	get_widget(refxml, "perfspinbuttondepoat", m_perfspinbutton[0][perfval_oat]);
	get_widget(refxml, "perfspinbuttondestoat", m_perfspinbutton[1][perfval_oat]);
	get_widget(refxml, "perfspinbuttondepisa", m_perfspinbutton[0][perfval_isa]);
	get_widget(refxml, "perfspinbuttondestisa", m_perfspinbutton[1][perfval_isa]);
	get_widget(refxml, "perfspinbuttondepdewpt", m_perfspinbutton[0][perfval_dewpt]);
	get_widget(refxml, "perfspinbuttondestdewpt", m_perfspinbutton[1][perfval_dewpt]);
	get_widget(refxml, "perfspinbuttondepwind", m_perfspinbutton[0][perfval_wind]);
	get_widget(refxml, "perfspinbuttondestwind", m_perfspinbutton[1][perfval_wind]);
	get_widget(refxml, "perftreeviewdep", m_perftreeview[0]);
	get_widget(refxml, "perftreeviewdest", m_perftreeview[1]);
	get_widget(refxml, "perftreeviewwb", m_perftreeviewwb);
	get_widget_derived(refxml, "perfdrawingareawb", m_perfdrawingareawb);
	// Sunrise Sunset Page
	get_widget(refxml, "srsstreeview", m_srsstreeview);
	get_widget(refxml, "srsscalendar", m_srsscalendar);
	// EU OPS1 Minimum Calculator (on Sunrise Sunset Page)
	get_widget(refxml, "euops1comboboxapproach", m_euops1comboboxapproach);
	get_widget(refxml, "euops1spinbuttontdzelev", m_euops1spinbuttontdzelev);
	get_widget(refxml, "euops1comboboxtdzelevunit", m_euops1comboboxtdzelevunit);
	get_widget(refxml, "euops1comboboxtdzlist", m_euops1comboboxtdzlist);
	get_widget(refxml, "euops1comboboxocaoch", m_euops1comboboxocaoch);
	get_widget(refxml, "euops1spinbuttonocaoch", m_euops1spinbuttonocaoch);
	get_widget(refxml, "euops1comboboxocaochunit", m_euops1comboboxocaochunit);
	get_widget(refxml, "euops1labeldamda", m_euops1labeldamda);
	get_widget(refxml, "euops1entrydamda", m_euops1entrydamda);
	get_widget(refxml, "euops1buttonsetda", m_euops1buttonsetda);
	get_widget(refxml, "euops1entryrvr", m_euops1entryrvr);
	get_widget(refxml, "euops1entrymetvis", m_euops1entrymetvis);
	get_widget(refxml, "euops1comboboxlight", m_euops1comboboxlight);
	get_widget(refxml, "euops1comboboxmetvis", m_euops1comboboxmetvis);
	get_widget(refxml, "euops1checkbuttonrwyedgelight", m_euops1checkbuttonrwyedgelight);
	get_widget(refxml, "euops1checkbuttonrwycentreline", m_euops1checkbuttonrwycentreline);
	get_widget(refxml, "euops1checkbuttonmultiplervr", m_euops1checkbuttonmultiplervr);
	get_widget(refxml, "euops1entrytkoffrvr", m_euops1entrytkoffrvr);
	get_widget(refxml, "euops1entrytkoffmetvis", m_euops1entrytkoffmetvis);
	// Log Page
	get_widget(refxml, "logtreeview", m_logtreeview);
	// GRIB2 Page
	get_widget_derived(refxml, "grib2drawingarea", m_grib2drawingarea);
        get_widget(refxml, "grib2layertreeview", m_grib2layertreeview);
        get_widget(refxml, "grib2layerabbreventry", m_grib2layerabbreventry);
        get_widget(refxml, "grib2layerunitentry", m_grib2layerunitentry);
        get_widget(refxml, "grib2layerparamidentry", m_grib2layerparamidentry);
        get_widget(refxml, "grib2layerparamentry", m_grib2layerparamentry);
        get_widget(refxml, "grib2layerparamcatidentry", m_grib2layerparamcatidentry);
        get_widget(refxml, "grib2layerdisciplineidentry", m_grib2layerdisciplineidentry);
        get_widget(refxml, "grib2layerparamcatentry", m_grib2layerparamcatentry);
        get_widget(refxml, "grib2layerdisciplineentry", m_grib2layerdisciplineentry);
        get_widget(refxml, "grib2layerefftimeentry", m_grib2layerefftimeentry);
        get_widget(refxml, "grib2layerreftimeentry", m_grib2layerreftimeentry);
        get_widget(refxml, "grib2layersurface1entry", m_grib2layersurface1entry);
        get_widget(refxml, "grib2layersurface2entry", m_grib2layersurface2entry);
        get_widget(refxml, "grib2layersurface1valueentry", m_grib2layersurface1valueentry);
        get_widget(refxml, "grib2layersurface2valueentry", m_grib2layersurface2valueentry);
        get_widget(refxml, "grib2layersurface1identry", m_grib2layersurface1identry);
        get_widget(refxml, "grib2layersurface2identry", m_grib2layersurface2identry);
        get_widget(refxml, "grib2layersurface1unitentry", m_grib2layersurface1unitentry);
        get_widget(refxml, "grib2layersurface2unitentry", m_grib2layersurface2unitentry);
        get_widget(refxml, "grib2layercenteridentry", m_grib2layercenteridentry);
        get_widget(refxml, "grib2layersubcenteridentry", m_grib2layersubcenteridentry);
        get_widget(refxml, "grib2layerprodstatusidentry", m_grib2layerprodstatusidentry);
        get_widget(refxml, "grib2layerdatatypeidentry", m_grib2layerdatatypeidentry);
        get_widget(refxml, "grib2layergenprocidentry", m_grib2layergenprocidentry);
        get_widget(refxml, "grib2layergenproctypidentry", m_grib2layergenproctypidentry);
        get_widget(refxml, "grib2layercenterentry", m_grib2layercenterentry);
        get_widget(refxml, "grib2layersubcenterentry", m_grib2layersubcenterentry);
        get_widget(refxml, "grib2layerprodstatusentry", m_grib2layerprodstatusentry);
        get_widget(refxml, "grib2layerdatatypeentry", m_grib2layerdatatypeentry);
        get_widget(refxml, "grib2layergenprocentry", m_grib2layergenprocentry);
        get_widget(refxml, "grib2layergenproctypentry", m_grib2layergenproctypentry);
	if (m_mainvbox)
		m_mainvbox->pack_start(m_softkeys, false, true, 0);
	m_softkeys.set_visible();
	m_navarea.set_visible();
	if (m_mainnotebook) {
		m_mainnotebook->set_show_tabs(false);
		m_mainnotebook->insert_page(m_navarea, "NAV", 0);
		m_navarea.add_events(Gdk::BUTTON_MOTION_MASK | Gdk::BUTTON_PRESS_MASK | Gdk::BUTTON_RELEASE_MASK);
	}

	if (m_altdialog)
		m_altdialog->signal_update().connect(sigc::mem_fun(*this, &FlightDeckWindow::alt_settings_update));
	if (m_hsidialog)
		m_hsidialog->signal_update().connect(sigc::mem_fun(*this, &FlightDeckWindow::hsi_settings_update));
	if (m_mapdialog)
		m_mapdialog->signal_update().connect(sigc::mem_fun(*this, &FlightDeckWindow::map_settings_update));

	{
		MapDrawingArea& map(m_navarea.get_map_drawing_area());
		NavAIDrawingArea& ai(m_navarea.get_ai_drawing_area());
		NavHSIDrawingArea& hsi(m_navarea.get_hsi_drawing_area());
		NavAltDrawingArea& alt(m_navarea.get_alt_drawing_area());
		alt.signal_button_press_event().connect(sigc::mem_fun(*this, &FlightDeckWindow::alt_indicator_button_press_event));
		alt.add_events(Gdk::BUTTON_PRESS_MASK);
		hsi.signal_button_press_event().connect(sigc::mem_fun(*this, &FlightDeckWindow::hsi_indicator_button_press_event));
		hsi.add_events(Gdk::BUTTON_PRESS_MASK);
	}

	// Setup Waypoint Page
 	if (m_wptnotebook) {
		m_wptnotebook->set_show_border(false);
		m_wptnotebook->set_show_tabs(false);
	}
	m_querydispatch.connect(sigc::mem_fun(*this, &FlightDeckWindow::wptpage_querydone));
	m_wptarptrwystore = Gtk::ListStore::create(m_wptairportrunwaycolumns);
	if (m_wptarptrwytreeview) {
		m_wptarptrwytreeview->set_model(m_wptarptrwystore);
		Gtk::CellRendererText *ident_renderer = Gtk::manage(new Gtk::CellRendererText());
		int ident_col = m_wptarptrwytreeview->append_column(_("Id"), *ident_renderer) - 1;
		Gtk::TreeViewColumn *ident_column = m_wptarptrwytreeview->get_column(ident_col);
                if (ident_column) {
                        ident_column->add_attribute(*ident_renderer, "text", m_wptairportrunwaycolumns.m_ident);
			ident_column->add_attribute(*ident_renderer, "foreground-gdk", m_wptairportrunwaycolumns.m_color);
		}
		Gtk::CellRendererText *hdg_renderer = Gtk::manage(new Gtk::CellRendererText());
		int hdg_col = m_wptarptrwytreeview->append_column(_("Hdg"), *hdg_renderer) - 1;
		Gtk::TreeViewColumn *hdg_column = m_wptarptrwytreeview->get_column(hdg_col);
		if (hdg_column) {
			hdg_column->add_attribute(*hdg_renderer, "text", m_wptairportrunwaycolumns.m_hdg);
			hdg_column->add_attribute(*hdg_renderer, "foreground-gdk", m_wptairportrunwaycolumns.m_color);
		}
		Gtk::CellRendererText *length_renderer = Gtk::manage(new Gtk::CellRendererText());
		int length_col = m_wptarptrwytreeview->append_column(_("Length"), *length_renderer) - 1;
		Gtk::TreeViewColumn *length_column = m_wptarptrwytreeview->get_column(length_col);
		if (length_column) {
			length_column->add_attribute(*length_renderer, "text", m_wptairportrunwaycolumns.m_length);
			length_column->add_attribute(*length_renderer, "foreground-gdk", m_wptairportrunwaycolumns.m_color);
		}
		Gtk::CellRendererText *width_renderer = Gtk::manage(new Gtk::CellRendererText());
		int width_col = m_wptarptrwytreeview->append_column(_("Width"), *width_renderer) - 1;
		Gtk::TreeViewColumn *width_column = m_wptarptrwytreeview->get_column(width_col);
		if (width_column) {
			width_column->add_attribute(*width_renderer, "text", m_wptairportrunwaycolumns.m_width);
			width_column->add_attribute(*width_renderer, "foreground-gdk", m_wptairportrunwaycolumns.m_color);
		}
		Gtk::CellRendererText *tda_renderer = Gtk::manage(new Gtk::CellRendererText());
		int tda_col = m_wptarptrwytreeview->append_column(_("TDA"), *tda_renderer) - 1;
		Gtk::TreeViewColumn *tda_column = m_wptarptrwytreeview->get_column(tda_col);
		if (tda_column) {
			tda_column->add_attribute(*tda_renderer, "text", m_wptairportrunwaycolumns.m_tda);
			tda_column->add_attribute(*tda_renderer, "foreground-gdk", m_wptairportrunwaycolumns.m_color);
		}
		Gtk::CellRendererText *lda_renderer = Gtk::manage(new Gtk::CellRendererText());
		int lda_col = m_wptarptrwytreeview->append_column(_("LDA"), *lda_renderer) - 1;
		Gtk::TreeViewColumn *lda_column = m_wptarptrwytreeview->get_column(lda_col);
		if (lda_column) {
			lda_column->add_attribute(*lda_renderer, "text", m_wptairportrunwaycolumns.m_lda);
			lda_column->add_attribute(*lda_renderer, "foreground-gdk", m_wptairportrunwaycolumns.m_color);
		}
		Gtk::CellRendererText *elev_renderer = Gtk::manage(new Gtk::CellRendererText());
		int elev_col = m_wptarptrwytreeview->append_column(_("Elev"), *elev_renderer) - 1;
		Gtk::TreeViewColumn *elev_column = m_wptarptrwytreeview->get_column(elev_col);
		if (elev_column) {
			elev_column->add_attribute(*elev_renderer, "text", m_wptairportrunwaycolumns.m_elev);
			elev_column->add_attribute(*elev_renderer, "foreground-gdk", m_wptairportrunwaycolumns.m_color);
		}
		Gtk::CellRendererText *surface_renderer = Gtk::manage(new Gtk::CellRendererText());
		int surface_col = m_wptarptrwytreeview->append_column(_("SFC"), *surface_renderer) - 1;
		Gtk::TreeViewColumn *surface_column = m_wptarptrwytreeview->get_column(surface_col);
		if (surface_column) {
			surface_column->add_attribute(*surface_renderer, "text", m_wptairportrunwaycolumns.m_surface);
			surface_column->add_attribute(*surface_renderer, "foreground-gdk", m_wptairportrunwaycolumns.m_color);
		}
		Gtk::CellRendererText *light_renderer = Gtk::manage(new Gtk::CellRendererText());
		int light_col = m_wptarptrwytreeview->append_column(_("Lights"), *light_renderer) - 1;
		Gtk::TreeViewColumn *light_column = m_wptarptrwytreeview->get_column(light_col);
		if (light_column) {
			light_column->add_attribute(*light_renderer, "text", m_wptairportrunwaycolumns.m_light);
 			light_column->add_attribute(*light_renderer, "foreground-gdk", m_wptairportrunwaycolumns.m_color);
		}
		hdg_renderer->set_property("xalign", 1.0);
                length_renderer->set_property("xalign", 1.0);
                width_renderer->set_property("xalign", 1.0);
                tda_renderer->set_property("xalign", 1.0);
                lda_renderer->set_property("xalign", 1.0);
                elev_renderer->set_property("xalign", 1.0);
		const Gtk::TreeModelColumnBase *cols[] = { &m_wptairportrunwaycolumns.m_ident,
							   &m_wptairportrunwaycolumns.m_hdg,
							   &m_wptairportrunwaycolumns.m_length,
							   &m_wptairportrunwaycolumns.m_width,
							   &m_wptairportrunwaycolumns.m_tda,
							   &m_wptairportrunwaycolumns.m_lda,
							   &m_wptairportrunwaycolumns.m_elev,
							   &m_wptairportrunwaycolumns.m_surface,
							   &m_wptairportrunwaycolumns.m_light };
		for (unsigned int i = 0; i < sizeof(cols)/sizeof(cols[0]); ++i) {
			Gtk::TreeViewColumn *col = m_wptarptrwytreeview->get_column(i);
			if (!col)
				continue;
			col->set_resizable(true);
			col->set_sort_column(*cols[i]);
			col->set_expand(i == 0);
		}
		m_wptarptrwytreeview->columns_autosize();
		m_wptarptrwytreeview->set_headers_clickable();
	}
	m_wptarpthelipadstore = Gtk::ListStore::create(m_wptairporthelipadcolumns);
	if (m_wptarpthelitreeview) {
		m_wptarpthelitreeview->set_model(m_wptarpthelipadstore);
		Gtk::CellRendererText *ident_renderer = Gtk::manage(new Gtk::CellRendererText());
		int ident_col = m_wptarpthelitreeview->append_column(_("Id"), *ident_renderer) - 1;
		Gtk::TreeViewColumn *ident_column = m_wptarpthelitreeview->get_column(ident_col);
                if (ident_column)
                        ident_column->add_attribute(*ident_renderer, "text", m_wptairporthelipadcolumns.m_ident);
		Gtk::CellRendererText *hdg_renderer = Gtk::manage(new Gtk::CellRendererText());
		int hdg_col = m_wptarpthelitreeview->append_column(_("Hdg"), *hdg_renderer) - 1;
		Gtk::TreeViewColumn *hdg_column = m_wptarpthelitreeview->get_column(hdg_col);
                if (hdg_column)
			hdg_column->add_attribute(*hdg_renderer, "text", m_wptairporthelipadcolumns.m_hdg);
		Gtk::CellRendererText *length_renderer = Gtk::manage(new Gtk::CellRendererText());
		int length_col = m_wptarpthelitreeview->append_column(_("Length"), *length_renderer) - 1;
		Gtk::TreeViewColumn *length_column = m_wptarpthelitreeview->get_column(length_col);
                if (length_column)
                        length_column->add_attribute(*length_renderer, "text", m_wptairporthelipadcolumns.m_length);
		Gtk::CellRendererText *width_renderer = Gtk::manage(new Gtk::CellRendererText());
		int width_col = m_wptarpthelitreeview->append_column(_("Width"), *width_renderer) - 1;
		Gtk::TreeViewColumn *width_column = m_wptarpthelitreeview->get_column(width_col);
                if (width_column)
                        width_column->add_attribute(*width_renderer, "text", m_wptairporthelipadcolumns.m_width);
		Gtk::CellRendererText *elev_renderer = Gtk::manage(new Gtk::CellRendererText());
		int elev_col = m_wptarpthelitreeview->append_column(_("Elev"), *elev_renderer) - 1;
		Gtk::TreeViewColumn *elev_column = m_wptarpthelitreeview->get_column(elev_col);
                if (elev_column)
                        elev_column->add_attribute(*elev_renderer, "text", m_wptairporthelipadcolumns.m_elev);
                hdg_renderer->set_property("xalign", 1.0);
                length_renderer->set_property("xalign", 1.0);
                width_renderer->set_property("xalign", 1.0);
                elev_renderer->set_property("xalign", 1.0);
		const Gtk::TreeModelColumnBase *cols[] = { &m_wptairporthelipadcolumns.m_ident,
							   &m_wptairporthelipadcolumns.m_hdg,
							   &m_wptairporthelipadcolumns.m_length,
							   &m_wptairporthelipadcolumns.m_width,
							   &m_wptairporthelipadcolumns.m_elev };
		for (unsigned int i = 0; i < sizeof(cols)/sizeof(cols[0]); ++i) {
			Gtk::TreeViewColumn *col = m_wptarpthelitreeview->get_column(i);
			if (!col)
				continue;
			col->set_resizable(true);
			col->set_sort_column(*cols[i]);
			col->set_expand(i == 0);
		}
		m_wptarpthelitreeview->columns_autosize();
		m_wptarpthelitreeview->set_headers_clickable();
	}
	m_wptarptcommstore = Gtk::ListStore::create(m_wptairportcommcolumns);
	if (m_wptarptcommtreeview) {
		m_wptarptcommtreeview->set_model(m_wptarptcommstore);
		Gtk::CellRendererText *name_renderer = Gtk::manage(new Gtk::CellRendererText());
		int name_col = m_wptarptcommtreeview->append_column(_("Name"), *name_renderer) - 1;
		Gtk::TreeViewColumn *name_column = m_wptarptcommtreeview->get_column(name_col);
                if (name_column)
                        name_column->add_attribute(*name_renderer, "text", m_wptairportcommcolumns.m_name);
		Gtk::CellRendererText *freq0_renderer = Gtk::manage(new Gtk::CellRendererText());
		int freq0_col = m_wptarptcommtreeview->append_column(_("Freq 0"), *freq0_renderer) - 1;
		Gtk::TreeViewColumn *freq0_column = m_wptarptcommtreeview->get_column(freq0_col);
                if (freq0_column)
                        freq0_column->add_attribute(*freq0_renderer, "text", m_wptairportcommcolumns.m_freq0);
		Gtk::CellRendererText *freq1_renderer = Gtk::manage(new Gtk::CellRendererText());
		int freq1_col = m_wptarptcommtreeview->append_column(_("Freq 1"), *freq1_renderer) - 1;
		Gtk::TreeViewColumn *freq1_column = m_wptarptcommtreeview->get_column(freq1_col);
                if (freq1_column)
                        freq1_column->add_attribute(*freq1_renderer, "text", m_wptairportcommcolumns.m_freq1);
		Gtk::CellRendererText *freq2_renderer = Gtk::manage(new Gtk::CellRendererText());
		int freq2_col = m_wptarptcommtreeview->append_column(_("Freq 2"), *freq2_renderer) - 1;
		Gtk::TreeViewColumn *freq2_column = m_wptarptcommtreeview->get_column(freq2_col);
                if (freq2_column)
                        freq2_column->add_attribute(*freq2_renderer, "text", m_wptairportcommcolumns.m_freq2);
		Gtk::CellRendererText *freq3_renderer = Gtk::manage(new Gtk::CellRendererText());
		int freq3_col = m_wptarptcommtreeview->append_column(_("Freq 3"), *freq3_renderer) - 1;
		Gtk::TreeViewColumn *freq3_column = m_wptarptcommtreeview->get_column(freq3_col);
                if (freq3_column)
                        freq3_column->add_attribute(*freq3_renderer, "text", m_wptairportcommcolumns.m_freq3);
		Gtk::CellRendererText *freq4_renderer = Gtk::manage(new Gtk::CellRendererText());
		int freq4_col = m_wptarptcommtreeview->append_column(_("Freq 4"), *freq4_renderer) - 1;
		Gtk::TreeViewColumn *freq4_column = m_wptarptcommtreeview->get_column(freq4_col);
                if (freq4_column)
                        freq4_column->add_attribute(*freq4_renderer, "text", m_wptairportcommcolumns.m_freq4);
		Gtk::CellRendererText *sector_renderer = Gtk::manage(new Gtk::CellRendererText());
		int sector_col = m_wptarptcommtreeview->append_column(_("Sector"), *sector_renderer) - 1;
		Gtk::TreeViewColumn *sector_column = m_wptarptcommtreeview->get_column(sector_col);
                if (sector_column)
                        sector_column->add_attribute(*sector_renderer, "text", m_wptairportcommcolumns.m_sector);
		Gtk::CellRendererText *oprhours_renderer = Gtk::manage(new Gtk::CellRendererText());
		int oprhours_col = m_wptarptcommtreeview->append_column(_("Oprhours"), *oprhours_renderer) - 1;
		Gtk::TreeViewColumn *oprhours_column = m_wptarptcommtreeview->get_column(oprhours_col);
                if (oprhours_column)
                        oprhours_column->add_attribute(*oprhours_renderer, "text", m_wptairportcommcolumns.m_oprhours);
                freq0_renderer->set_property("xalign", 1.0);
                freq1_renderer->set_property("xalign", 1.0);
                freq2_renderer->set_property("xalign", 1.0);
                freq3_renderer->set_property("xalign", 1.0);
                freq4_renderer->set_property("xalign", 1.0);
		const Gtk::TreeModelColumnBase *cols[] = { &m_wptairportcommcolumns.m_name,
							   &m_wptairportcommcolumns.m_freq0,
							   &m_wptairportcommcolumns.m_freq1,
							   &m_wptairportcommcolumns.m_freq2,
							   &m_wptairportcommcolumns.m_freq3,
							   &m_wptairportcommcolumns.m_freq4,
							   &m_wptairportcommcolumns.m_sector,
							   &m_wptairportcommcolumns.m_oprhours };
		for (unsigned int i = 0; i < sizeof(cols)/sizeof(cols[0]); ++i) {
			Gtk::TreeViewColumn *col = m_wptarptcommtreeview->get_column(i);
			if (!col)
				continue;
			col->set_resizable(true);
			col->set_sort_column(*cols[i]);
			col->set_expand(i == 0);
		}
		m_wptarptcommtreeview->columns_autosize();
		m_wptarptcommtreeview->set_headers_clickable();
	}
	m_wptarptfasstore = Gtk::ListStore::create(m_wptairportfascolumns);
	if (m_wptarptfastreeview) {
		m_wptarptfastreeview->set_model(m_wptarptfasstore);
		Gtk::CellRendererText *ident_renderer = Gtk::manage(new Gtk::CellRendererText());
		int ident_col = m_wptarptfastreeview->append_column(_("Ident"), *ident_renderer) - 1;
		Gtk::TreeViewColumn *ident_column = m_wptarptfastreeview->get_column(ident_col);
                if (ident_column)
                        ident_column->add_attribute(*ident_renderer, "text", m_wptairportfascolumns.m_ident);
		Gtk::CellRendererText *runway_renderer = Gtk::manage(new Gtk::CellRendererText());
		int runway_col = m_wptarptfastreeview->append_column(_("Runway"), *runway_renderer) - 1;
		Gtk::TreeViewColumn *runway_column = m_wptarptfastreeview->get_column(runway_col);
                if (runway_column)
                        runway_column->add_attribute(*runway_renderer, "text", m_wptairportfascolumns.m_runway);
		Gtk::CellRendererText *route_renderer = Gtk::manage(new Gtk::CellRendererText());
		int route_col = m_wptarptfastreeview->append_column(_("Route"), *route_renderer) - 1;
		Gtk::TreeViewColumn *route_column = m_wptarptfastreeview->get_column(route_col);
                if (route_column)
                        route_column->add_attribute(*route_renderer, "text", m_wptairportfascolumns.m_route);
		Gtk::CellRendererText *glide_renderer = Gtk::manage(new Gtk::CellRendererText());
		int glide_col = m_wptarptfastreeview->append_column(_("Glide Angle"), *glide_renderer) - 1;
		Gtk::TreeViewColumn *glide_column = m_wptarptfastreeview->get_column(glide_col);
                if (glide_column)
                        glide_column->add_attribute(*glide_renderer, "text", m_wptairportfascolumns.m_glide);
		Gtk::CellRendererText *ftpalt_renderer = Gtk::manage(new Gtk::CellRendererText());
		int ftpalt_col = m_wptarptfastreeview->append_column(_("Alt"), *ftpalt_renderer) - 1;
		Gtk::TreeViewColumn *ftpalt_column = m_wptarptfastreeview->get_column(ftpalt_col);
                if (ftpalt_column)
                        ftpalt_column->add_attribute(*ftpalt_renderer, "text", m_wptairportfascolumns.m_ftpalt);
		Gtk::CellRendererText *tch_renderer = Gtk::manage(new Gtk::CellRendererText());
		int tch_col = m_wptarptfastreeview->append_column(_("TCH"), *tch_renderer) - 1;
		Gtk::TreeViewColumn *tch_column = m_wptarptfastreeview->get_column(tch_col);
                if (tch_column)
                        tch_column->add_attribute(*tch_renderer, "text", m_wptairportfascolumns.m_tch);
                glide_renderer->set_property("xalign", 1.0);
                ftpalt_renderer->set_property("xalign", 1.0);
		tch_renderer->set_property("xalign", 1.0);
		const Gtk::TreeModelColumnBase *cols[] = { &m_wptairportfascolumns.m_ident,
							   &m_wptairportfascolumns.m_runway,
							   &m_wptairportfascolumns.m_route,
							   &m_wptairportfascolumns.m_glide,
							   &m_wptairportfascolumns.m_ftpalt,
							   &m_wptairportfascolumns.m_tch };
		for (unsigned int i = 0; i < sizeof(cols)/sizeof(cols[0]); ++i) {
			Gtk::TreeViewColumn *col = m_wptarptfastreeview->get_column(i);
			if (!col)
				continue;
			col->set_resizable(true);
			col->set_sort_column(*cols[i]);
			col->set_expand(i == 0);
		}
		m_wptarptfastreeview->columns_autosize();
		m_wptarptfastreeview->set_headers_clickable();
	}
	if (m_wptarpteleventry)
		m_wptarpteleventry->set_alignment(1.0);
	if (m_wptnavaideleventry)
		m_wptnavaideleventry->set_alignment(1.0);
	if (m_wptnavaidfreqentry)
		m_wptnavaidfreqentry->set_alignment(1.0);
	if (m_wptnavaidrangeentry)
		m_wptnavaidrangeentry->set_alignment(1.0);
	if (m_wptawybasealtentry)
		m_wptawybasealtentry->set_alignment(1.0);
	if (m_wptawytopaltentry)
		m_wptawytopaltentry->set_alignment(1.0);
	if (m_wptawyteleventry)
		m_wptawyteleventry->set_alignment(1.0);
	if (m_wptawyt5eleventry)
		m_wptawyt5eleventry->set_alignment(1.0);
	if (m_wptaspcfromaltentry)
		m_wptaspcfromaltentry->set_alignment(1.0);
	if (m_wptaspctoaltentry)
		m_wptaspctoaltentry->set_alignment(1.0);
	if (m_wptaspccomm0entry)
		m_wptaspccomm0entry->set_alignment(1.0);
	if (m_wptaspccomm1entry)
		m_wptaspccomm1entry->set_alignment(1.0);
	if (m_wpttogglebuttonnearest)
		m_wptpagebuttonnearestconn = m_wpttogglebuttonnearest->signal_clicked().connect(sigc::mem_fun(*this, &FlightDeckWindow::wptpage_togglebuttonnearest));
	if (m_wpttogglebuttonfind)
		m_wptpagetogglebuttonfindconn = m_wpttogglebuttonfind->signal_clicked().connect(sigc::mem_fun(*this, &FlightDeckWindow::wptpage_togglebuttonfind));
	if (m_wptbuttoninsert)
		m_wptbuttoninsert->signal_clicked().connect(sigc::mem_fun(*this, &FlightDeckWindow::wptpage_buttoninsert));
	if (m_wptbuttondirectto)
		m_wptbuttondirectto->signal_clicked().connect(sigc::mem_fun(*this, &FlightDeckWindow::wptpage_buttondirectto));
	if (m_wpttogglebuttonbrg2)
		m_wptbrg2conn = m_wpttogglebuttonbrg2->signal_clicked().connect(sigc::mem_fun(*this, &FlightDeckWindow::wptpage_togglebuttonbrg2));
	if (m_wpttogglebuttonmap)
		m_wpttogglebuttonarptmap = m_wpttogglebuttonmap->signal_clicked().connect(sigc::mem_fun(*this, &FlightDeckWindow::wptpage_togglebuttonarptmap));
	if (m_wptdrawingarea)
		m_wptmapcursorconn = m_wptdrawingarea->signal_cursor().connect(sigc::mem_fun(*this, &FlightDeckWindow::wptpage_map_cursor_change));
	// Setup Flight Plan Page
	m_fplstore = Gtk::ListStore::create(m_fplcolumns);
	if (m_fpltreeview) {
		m_fpltreeview->set_model(m_fplstore);
		Gtk::CellRendererText *icao_renderer = Gtk::manage(new Gtk::CellRendererText());
		int icao_col = m_fpltreeview->append_column(_("ICAO"), *icao_renderer) - 1;
		Gtk::TreeViewColumn *icao_column = m_fpltreeview->get_column(icao_col);
                if (icao_column) {
                        icao_column->add_attribute(*icao_renderer, "text", m_fplcolumns.m_icao);
                        icao_column->add_attribute(*icao_renderer, "weight", m_fplcolumns.m_weight);
                        icao_column->add_attribute(*icao_renderer, "style", m_fplcolumns.m_style);
		}
		Gtk::CellRendererText *name_renderer = Gtk::manage(new Gtk::CellRendererText());
		int name_col = m_fpltreeview->append_column(_("Name"), *name_renderer) - 1;
		Gtk::TreeViewColumn *name_column = m_fpltreeview->get_column(name_col);
                if (name_column) {
                        name_column->add_attribute(*name_renderer, "text", m_fplcolumns.m_name);
                        name_column->add_attribute(*name_renderer, "weight", m_fplcolumns.m_weight);
                        name_column->add_attribute(*name_renderer, "style", m_fplcolumns.m_style);
		}
		Gtk::CellRendererText *freq_renderer = Gtk::manage(new Gtk::CellRendererText());
		int freq_col = m_fpltreeview->append_column(_("Freq"), *freq_renderer) - 1;
		Gtk::TreeViewColumn *freq_column = m_fpltreeview->get_column(freq_col);
                if (freq_column) {
                        freq_column->add_attribute(*freq_renderer, "text", m_fplcolumns.m_freq);
                        freq_column->add_attribute(*freq_renderer, "weight", m_fplcolumns.m_weight);
                        freq_column->add_attribute(*freq_renderer, "style", m_fplcolumns.m_style);
		}
		Gtk::CellRendererText *time_renderer = Gtk::manage(new Gtk::CellRendererText());
		int time_col = m_fpltreeview->append_column(_("Time"), *time_renderer) - 1;
		Gtk::TreeViewColumn *time_column = m_fpltreeview->get_column(time_col);
                if (time_column) {
                        time_column->add_attribute(*time_renderer, "text", m_fplcolumns.m_time);
                        time_column->add_attribute(*time_renderer, "weight", m_fplcolumns.m_weight);
                        time_column->add_attribute(*time_renderer, "foreground-gdk", m_fplcolumns.m_time_color);
                        time_column->add_attribute(*time_renderer, "style", m_fplcolumns.m_time_style);
		}
		Gtk::CellRendererText *lon_renderer = Gtk::manage(new Gtk::CellRendererText());
		int lon_col = m_fpltreeview->append_column(_("Lon"), *lon_renderer) - 1;
		Gtk::TreeViewColumn *lon_column = m_fpltreeview->get_column(lon_col);
                if (lon_column) {
                        lon_column->add_attribute(*lon_renderer, "text", m_fplcolumns.m_lon);
                        lon_column->add_attribute(*lon_renderer, "weight", m_fplcolumns.m_weight);
                        lon_column->add_attribute(*lon_renderer, "style", m_fplcolumns.m_style);
		}
		Gtk::CellRendererText *lat_renderer = Gtk::manage(new Gtk::CellRendererText());
		int lat_col = m_fpltreeview->append_column(_("Lat"), *lat_renderer) - 1;
		Gtk::TreeViewColumn *lat_column = m_fpltreeview->get_column(lat_col);
                if (lat_column) {
                        lat_column->add_attribute(*lat_renderer, "text", m_fplcolumns.m_lat);
                        lat_column->add_attribute(*lat_renderer, "weight", m_fplcolumns.m_weight);
                        lat_column->add_attribute(*lat_renderer, "style", m_fplcolumns.m_style);
		}
		Gtk::CellRendererText *alt_renderer = Gtk::manage(new Gtk::CellRendererText());
		int alt_col = m_fpltreeview->append_column(_("Alt"), *alt_renderer) - 1;
		Gtk::TreeViewColumn *alt_column = m_fpltreeview->get_column(alt_col);
                if (alt_column) {
                        alt_column->add_attribute(*alt_renderer, "text", m_fplcolumns.m_alt);
                        alt_column->add_attribute(*alt_renderer, "weight", m_fplcolumns.m_weight);
                        alt_column->add_attribute(*alt_renderer, "style", m_fplcolumns.m_style);
		}
		Gtk::CellRendererText *mt_renderer = Gtk::manage(new Gtk::CellRendererText());
		int mt_col = m_fpltreeview->append_column(_("MT"), *mt_renderer) - 1;
		Gtk::TreeViewColumn *mt_column = m_fpltreeview->get_column(mt_col);
                if (mt_column) {
                        mt_column->add_attribute(*mt_renderer, "text", m_fplcolumns.m_mt);
                        mt_column->add_attribute(*mt_renderer, "weight", m_fplcolumns.m_weight);
                        mt_column->add_attribute(*mt_renderer, "style", m_fplcolumns.m_style);
		}
		Gtk::CellRendererText *tt_renderer = Gtk::manage(new Gtk::CellRendererText());
		int tt_col = m_fpltreeview->append_column(_("TT"), *tt_renderer) - 1;
		Gtk::TreeViewColumn *tt_column = m_fpltreeview->get_column(tt_col);
                if (tt_column) {
                        tt_column->add_attribute(*tt_renderer, "text", m_fplcolumns.m_tt);
                        tt_column->add_attribute(*tt_renderer, "weight", m_fplcolumns.m_weight);
                        tt_column->add_attribute(*tt_renderer, "style", m_fplcolumns.m_style);
		}
		Gtk::CellRendererText *dist_renderer = Gtk::manage(new Gtk::CellRendererText());
		int dist_col = m_fpltreeview->append_column(_("Dist"), *dist_renderer) - 1;
		Gtk::TreeViewColumn *dist_column = m_fpltreeview->get_column(dist_col);
                if (dist_column) {
                        dist_column->add_attribute(*dist_renderer, "text", m_fplcolumns.m_dist);
                        dist_column->add_attribute(*dist_renderer, "weight", m_fplcolumns.m_weight);
                        dist_column->add_attribute(*dist_renderer, "style", m_fplcolumns.m_style);
		}
		Gtk::CellRendererText *vertspeed_renderer = Gtk::manage(new Gtk::CellRendererText());
		int vertspeed_col = m_fpltreeview->append_column(_("VS"), *vertspeed_renderer) - 1;
		Gtk::TreeViewColumn *vertspeed_column = m_fpltreeview->get_column(vertspeed_col);
                if (vertspeed_column) {
                        vertspeed_column->add_attribute(*vertspeed_renderer, "text", m_fplcolumns.m_vertspeed);
                        vertspeed_column->add_attribute(*vertspeed_renderer, "weight", m_fplcolumns.m_weight);
                        vertspeed_column->add_attribute(*vertspeed_renderer, "style", m_fplcolumns.m_style);
		}
		Gtk::CellRendererText *mh_renderer = Gtk::manage(new Gtk::CellRendererText());
		int mh_col = m_fpltreeview->append_column(_("MH"), *mh_renderer) - 1;
		Gtk::TreeViewColumn *mh_column = m_fpltreeview->get_column(mh_col);
                if (mh_column) {
                        mh_column->add_attribute(*mh_renderer, "text", m_fplcolumns.m_mh);
                        mh_column->add_attribute(*mh_renderer, "weight", m_fplcolumns.m_weight);
                        mh_column->add_attribute(*mh_renderer, "style", m_fplcolumns.m_style);
		}
		Gtk::CellRendererText *tas_renderer = Gtk::manage(new Gtk::CellRendererText());
		int tas_col = m_fpltreeview->append_column(_("TAS"), *tas_renderer) - 1;
		Gtk::TreeViewColumn *tas_column = m_fpltreeview->get_column(tas_col);
                if (tas_column) {
                        tas_column->add_attribute(*tas_renderer, "text", m_fplcolumns.m_tas);
                        tas_column->add_attribute(*tas_renderer, "weight", m_fplcolumns.m_weight);
                        tas_column->add_attribute(*tas_renderer, "style", m_fplcolumns.m_style);
		}
		Gtk::CellRendererText *truealt_renderer = Gtk::manage(new Gtk::CellRendererText());
		int truealt_col = m_fpltreeview->append_column(_("TrueAlt"), *truealt_renderer) - 1;
		Gtk::TreeViewColumn *truealt_column = m_fpltreeview->get_column(truealt_col);
                if (truealt_column) {
                        truealt_column->add_attribute(*truealt_renderer, "text", m_fplcolumns.m_truealt);
                        truealt_column->add_attribute(*truealt_renderer, "weight", m_fplcolumns.m_weight);
                        truealt_column->add_attribute(*truealt_renderer, "style", m_fplcolumns.m_style);
		}
		Gtk::CellRendererText *power_renderer = Gtk::manage(new Gtk::CellRendererText());
		int power_col = m_fpltreeview->append_column(_("Power"), *power_renderer) - 1;
		Gtk::TreeViewColumn *power_column = m_fpltreeview->get_column(power_col);
                if (power_column) {
                        power_column->add_attribute(*power_renderer, "text", m_fplcolumns.m_power);
                        power_column->add_attribute(*power_renderer, "weight", m_fplcolumns.m_weight);
                        power_column->add_attribute(*power_renderer, "style", m_fplcolumns.m_style);
		}
		Gtk::CellRendererText *fuel_renderer = Gtk::manage(new Gtk::CellRendererText());
		int fuel_col = m_fpltreeview->append_column(_("Fuel"), *fuel_renderer) - 1;
		Gtk::TreeViewColumn *fuel_column = m_fpltreeview->get_column(fuel_col);
                if (fuel_column) {
                        fuel_column->add_attribute(*fuel_renderer, "text", m_fplcolumns.m_fuel);
                        fuel_column->add_attribute(*fuel_renderer, "weight", m_fplcolumns.m_weight);
                        fuel_column->add_attribute(*fuel_renderer, "style", m_fplcolumns.m_style);
		}
		Gtk::CellRendererText *pathname_renderer = Gtk::manage(new Gtk::CellRendererText());
		int pathname_col = m_fpltreeview->append_column(_("Pathname"), *pathname_renderer) - 1;
		Gtk::TreeViewColumn *pathname_column = m_fpltreeview->get_column(pathname_col);
                if (pathname_column) {
                        pathname_column->add_attribute(*pathname_renderer, "text", m_fplcolumns.m_pathname);
                        pathname_column->add_attribute(*pathname_renderer, "weight", m_fplcolumns.m_weight);
                        pathname_column->add_attribute(*pathname_renderer, "style", m_fplcolumns.m_style);
		}
		Gtk::CellRendererText *wind_renderer = Gtk::manage(new Gtk::CellRendererText());
		int wind_col = m_fpltreeview->append_column(_("Wind"), *wind_renderer) - 1;
		Gtk::TreeViewColumn *wind_column = m_fpltreeview->get_column(wind_col);
                if (wind_column) {
                        wind_column->add_attribute(*wind_renderer, "text", m_fplcolumns.m_wind);
                        wind_column->add_attribute(*wind_renderer, "weight", m_fplcolumns.m_weight);
                        wind_column->add_attribute(*wind_renderer, "style", m_fplcolumns.m_style);
		}
		Gtk::CellRendererText *qff_renderer = Gtk::manage(new Gtk::CellRendererText());
		int qff_col = m_fpltreeview->append_column(_("QFF"), *qff_renderer) - 1;
		Gtk::TreeViewColumn *qff_column = m_fpltreeview->get_column(qff_col);
                if (qff_column) {
                        qff_column->add_attribute(*qff_renderer, "text", m_fplcolumns.m_qff);
                        qff_column->add_attribute(*qff_renderer, "weight", m_fplcolumns.m_weight);
                        qff_column->add_attribute(*qff_renderer, "style", m_fplcolumns.m_style);
		}

		Gtk::CellRendererText *oat_renderer = Gtk::manage(new Gtk::CellRendererText());
		int oat_col = m_fpltreeview->append_column(_("Oat"), *oat_renderer) - 1;
		Gtk::TreeViewColumn *oat_column = m_fpltreeview->get_column(oat_col);
                if (oat_column) {
                        oat_column->add_attribute(*oat_renderer, "text", m_fplcolumns.m_oat);
                        oat_column->add_attribute(*oat_renderer, "weight", m_fplcolumns.m_weight);
                        oat_column->add_attribute(*oat_renderer, "style", m_fplcolumns.m_style);
		}
		alt_renderer->set_property("xalign", 1.0);
                mt_renderer->set_property("xalign", 1.0);
                tt_renderer->set_property("xalign", 1.0);
                dist_renderer->set_property("xalign", 1.0);
                vertspeed_renderer->set_property("xalign", 1.0);
                mh_renderer->set_property("xalign", 1.0);
                tas_renderer->set_property("xalign", 1.0);
		truealt_renderer->set_property("xalign", 1.0);
		power_renderer->set_property("xalign", 1.0);
		fuel_renderer->set_property("xalign", 1.0);
		wind_renderer->set_property("xalign", 1.0);
		qff_renderer->set_property("xalign", 1.0);
		oat_renderer->set_property("xalign", 1.0);
		const Gtk::TreeModelColumnBase *cols[] = { &m_fplcolumns.m_icao,
							   &m_fplcolumns.m_name,
							   &m_fplcolumns.m_freq,
							   &m_fplcolumns.m_time,
							   &m_fplcolumns.m_lon,
							   &m_fplcolumns.m_lat,
							   &m_fplcolumns.m_alt,
							   &m_fplcolumns.m_mt,
							   &m_fplcolumns.m_tt,
							   &m_fplcolumns.m_dist,
							   &m_fplcolumns.m_vertspeed,
							   &m_fplcolumns.m_mh,
							   &m_fplcolumns.m_tas,
							   &m_fplcolumns.m_truealt,
							   &m_fplcolumns.m_power,
							   &m_fplcolumns.m_fuel,
							   &m_fplcolumns.m_pathname,
							   &m_fplcolumns.m_wind,
							   &m_fplcolumns.m_qff,
							   &m_fplcolumns.m_oat };
		for (unsigned int i = 0; i < sizeof(cols)/sizeof(cols[0]); ++i) {
			Gtk::TreeViewColumn *col = m_fpltreeview->get_column(i);
			if (!col)
				continue;
			col->set_resizable(true);
			//col->set_sort_column(*cols[i]);
			col->set_expand(i == 1);
		}
		m_fpltreeview->columns_autosize();
		//m_fpltreeview->set_headers_clickable();
		Glib::RefPtr<Gtk::TreeSelection> selection = m_fpltreeview->get_selection();
		selection->set_mode(Gtk::SELECTION_SINGLE);
		m_fplselchangeconn = selection->signal_changed().connect(sigc::mem_fun(*this, &FlightDeckWindow::fpl_selection_changed));
		//selection->set_select_function(sigc::mem_fun(*this, &FlightDeckWindow::fpl_select_function));
	}
	m_fplpathcodestore = Gtk::ListStore::create(m_fplpathcodecolumns);
	for (FPlanWaypoint::pathcode_t pc(FPlanWaypoint::pathcode_none); pc <= FPlanWaypoint::pathcode_directto;
	     pc = static_cast<FPlanWaypoint::pathcode_t>(pc + 1)) {
		Gtk::TreeModel::Row row(*(m_fplpathcodestore->append()));
		row[m_fplpathcodecolumns.m_name] = FPlanWaypoint::get_pathcode_name(pc);
		row[m_fplpathcodecolumns.m_id] = pc;
	}
	if (m_fplpathcodecombobox) {
		m_fplpathcodecombobox->clear();
		m_fplpathcodecombobox->set_model(m_fplpathcodestore);
		m_fplpathcodecombobox->set_entry_text_column(m_fplpathcodecolumns.m_name);
		m_fplpathcodecombobox->pack_start(m_fplpathcodecolumns.m_name);
	}
	m_fplaltunitstore = Gtk::ListStore::create(m_fplaltunitcolumns);
	{
		Gtk::TreeModel::Row row(*(m_fplaltunitstore->append()));
		row[m_fplaltunitcolumns.m_name] = "ft";
		row[m_fplaltunitcolumns.m_id] = 0;
		row = *(m_fplaltunitstore->append());
		row[m_fplaltunitcolumns.m_name] = "FL";
		row[m_fplaltunitcolumns.m_id] = 1;
		row = *(m_fplaltunitstore->append());
		row[m_fplaltunitcolumns.m_name] = "m";
		row[m_fplaltunitcolumns.m_id] = 2;
	}
	if (m_fplaltunitcombobox) {
		m_fplaltunitcombobox->clear();
		m_fplaltunitcombobox->set_model(m_fplaltunitstore);
		m_fplaltunitcombobox->set_entry_text_column(m_fplaltunitcolumns.m_name);
		m_fplaltunitcombobox->pack_start(m_fplaltunitcolumns.m_name);
	}
	m_fpltypestore = Gtk::ListStore::create(m_fpltypecolumns);
	for (FPlanWaypoint::type_t typ = FPlanWaypoint::type_airport; typ <= FPlanWaypoint::type_user; typ = (FPlanWaypoint::type_t)(typ + 1)) {
		if (typ != FPlanWaypoint::type_navaid) {
			Gtk::TreeModel::Row row(*(m_fpltypestore->append()));
			row[m_fpltypecolumns.m_name] = FPlanWaypoint::get_type_string(typ);
			row[m_fpltypecolumns.m_type] = typ;
			row[m_fpltypecolumns.m_navaid_type] = FPlanWaypoint::navaid_invalid;
			continue;
		}
		for (FPlanWaypoint::navaid_type_t nav = FPlanWaypoint::navaid_vor; nav <= FPlanWaypoint::navaid_tacan; nav = (FPlanWaypoint::navaid_type_t)(nav + 1)) {
			Gtk::TreeModel::Row row(*(m_fpltypestore->append()));
			row[m_fpltypecolumns.m_name] = FPlanWaypoint::get_type_string(typ, nav);
			row[m_fpltypecolumns.m_type] = typ;
			row[m_fpltypecolumns.m_navaid_type] = nav;
		}
	}
	if (m_fpltypecombobox) {
		m_fpltypecombobox->clear();
		m_fpltypecombobox->set_model(m_fpltypestore);
		m_fpltypecombobox->set_entry_text_column(m_fpltypecolumns.m_name);
		m_fpltypecombobox->pack_start(m_fpltypecolumns.m_name);
	}
	if (m_fplicaoentry) {
		m_fplicaoentry->signal_focus_in_event().connect(sigc::bind(sigc::bind(sigc::mem_fun(*this, &FlightDeckWindow::fplpage_focus_in_event), kbd_main), m_fplicaoentry));
		m_fplicaoentry->signal_focus_out_event().connect(sigc::mem_fun(*this, &FlightDeckWindow::fplpage_focus_out_event));
		m_fplicaoentry->signal_changed().connect(sigc::bind(sigc::mem_fun(*this, &FlightDeckWindow::fplpage_change_event), fplchg_icao));
	}
	if (m_fplnameentry) {
		m_fplnameentry->signal_focus_in_event().connect(sigc::bind(sigc::bind(sigc::mem_fun(*this, &FlightDeckWindow::fplpage_focus_in_event), kbd_main), m_fplnameentry));
		m_fplnameentry->signal_focus_out_event().connect(sigc::mem_fun(*this, &FlightDeckWindow::fplpage_focus_out_event));
		m_fplnameentry->signal_changed().connect(sigc::bind(sigc::mem_fun(*this, &FlightDeckWindow::fplpage_change_event), fplchg_name));
	}
	if (m_fplpathnameentry) {
		m_fplpathnameentry->signal_focus_in_event().connect(sigc::bind(sigc::bind(sigc::mem_fun(*this, &FlightDeckWindow::fplpage_focus_in_event), kbd_main), m_fplnameentry));
		m_fplpathnameentry->signal_focus_out_event().connect(sigc::mem_fun(*this, &FlightDeckWindow::fplpage_focus_out_event));
		m_fplpathnameentry->signal_changed().connect(sigc::bind(sigc::mem_fun(*this, &FlightDeckWindow::fplpage_change_event), fplchg_pathname));
	}
	if (m_fplcoordentry) {
		m_fplcoordentry->signal_focus_in_event().connect(sigc::bind(sigc::bind(sigc::mem_fun(*this, &FlightDeckWindow::fplpage_focus_in_event), kbd_auto), m_fplcoordentry));
		m_fplcoordentry->signal_focus_out_event().connect(sigc::mem_fun(*this, &FlightDeckWindow::fplpage_focus_out_event));
		m_fplcoordentry->signal_changed().connect(sigc::bind(sigc::mem_fun(*this, &FlightDeckWindow::fplpage_change_event), fplchg_coord));
	}
	if (m_fplfreqentry) {
		m_fplfreqentry->signal_focus_in_event().connect(sigc::bind(sigc::bind(sigc::mem_fun(*this, &FlightDeckWindow::fplpage_focus_in_event), kbd_numeric), m_fplfreqentry));
		m_fplfreqentry->signal_focus_out_event().connect(sigc::mem_fun(*this, &FlightDeckWindow::fplpage_focus_out_event));
		m_fplfreqentry->signal_changed().connect(sigc::bind(sigc::mem_fun(*this, &FlightDeckWindow::fplpage_change_event), fplchg_freq));
		m_fplfreqentry->set_alignment(1.0);
	}
	if (m_fplaltentry) {
		m_fplaltentry->signal_focus_in_event().connect(sigc::bind(sigc::bind(sigc::mem_fun(*this, &FlightDeckWindow::fplpage_focus_in_event), kbd_numeric), m_fplaltentry));
		m_fplaltentry->signal_focus_out_event().connect(sigc::mem_fun(*this, &FlightDeckWindow::fplpage_focus_out_event));
		m_fplaltentry->signal_changed().connect(sigc::bind(sigc::mem_fun(*this, &FlightDeckWindow::fplpage_change_event), fplchg_alt));
		m_fplaltentry->set_alignment(1.0);
	}
	if (m_fplwptnotestextview) {
		m_fplwptnotestextview->signal_focus_in_event().connect(sigc::bind(sigc::bind(sigc::mem_fun(*this, &FlightDeckWindow::fplpage_focus_in_event), kbd_full), m_fplwptnotestextview));
		m_fplwptnotestextview->signal_focus_out_event().connect(sigc::mem_fun(*this, &FlightDeckWindow::fplpage_focus_out_event));
		Glib::RefPtr<Gtk::TextBuffer> buf(m_fplwptnotestextview->get_buffer());
		if (buf)
			buf->signal_changed().connect(sigc::bind(sigc::mem_fun(*this, &FlightDeckWindow::fplpage_change_event), fplchg_note));
	}
	if (m_fplnotestextview) {
		m_fplnotestextview->signal_focus_in_event().connect(sigc::bind(sigc::bind(sigc::mem_fun(*this, &FlightDeckWindow::fplpage_focus_in_event), kbd_full), m_fplnotestextview));
		m_fplnotestextview->signal_focus_out_event().connect(sigc::mem_fun(*this, &FlightDeckWindow::fplpage_focus_out_event));
		Glib::RefPtr<Gtk::TextBuffer> buf(m_fplnotestextview->get_buffer());
		if (buf)
			buf->signal_changed().connect(sigc::bind(sigc::mem_fun(*this, &FlightDeckWindow::fplpage_change_event), fplchg_fplnote));
	}
	if (m_fplpathcodecombobox)
		m_fplpathcodechangeconn = m_fplpathcodecombobox->signal_changed().connect(sigc::mem_fun(*this, &FlightDeckWindow::fplpage_pathcode_changed));
	if (m_fplaltunitcombobox)
		m_fplaltunitchangeconn = m_fplaltunitcombobox->signal_changed().connect(sigc::mem_fun(*this, &FlightDeckWindow::fplpage_altunit_changed));
	if (m_fpltypecombobox)
		m_fpltypechangeconn = m_fpltypecombobox->signal_changed().connect(sigc::mem_fun(*this, &FlightDeckWindow::fplpage_type_changed));
	if (m_fpllevelupbutton)
		m_fpllevelupbutton->signal_clicked().connect(sigc::mem_fun(*this, &FlightDeckWindow::fplpage_levelup_clicked));
	if (m_fplleveldownbutton)
		m_fplleveldownbutton->signal_clicked().connect(sigc::mem_fun(*this, &FlightDeckWindow::fplpage_leveldown_clicked));
	if (m_fplsemicircularbutton)
		m_fplsemicircularbutton->signal_clicked().connect(sigc::mem_fun(*this, &FlightDeckWindow::fplpage_semicircular_clicked));
	if (m_fplifrcheckbutton)
		m_fplifrchangeconn = m_fplifrcheckbutton->signal_toggled().connect(sigc::mem_fun(*this, &FlightDeckWindow::fplpage_ifr_changed));
	if (m_fplcoordeditbutton)
		m_fplcoordeditbutton->signal_clicked().connect(sigc::mem_fun(*this, &FlightDeckWindow::fplpage_edit_coord));
	if (m_fplwinddirspinbutton) {
		m_fplwinddirspinbutton->signal_focus_in_event().connect(sigc::bind(sigc::bind(sigc::mem_fun(*this, &FlightDeckWindow::fplpage_focus_in_event), kbd_numeric), m_fplwinddirspinbutton));
		m_fplwinddirspinbutton->signal_focus_out_event().connect(sigc::mem_fun(*this, &FlightDeckWindow::fplpage_focus_out_event));
		m_fplwinddirspinbutton->signal_changed().connect(sigc::bind(sigc::mem_fun(*this, &FlightDeckWindow::fplpage_change_event), fplchg_winddir));
	}
	if (m_fplwindspeedspinbutton) {
		m_fplwindspeedspinbutton->signal_focus_in_event().connect(sigc::bind(sigc::bind(sigc::mem_fun(*this, &FlightDeckWindow::fplpage_focus_in_event), kbd_numeric), m_fplwindspeedspinbutton));
		m_fplwindspeedspinbutton->signal_focus_out_event().connect(sigc::mem_fun(*this, &FlightDeckWindow::fplpage_focus_out_event));
		m_fplwindspeedspinbutton->signal_changed().connect(sigc::bind(sigc::mem_fun(*this, &FlightDeckWindow::fplpage_change_event), fplchg_windspeed));
	}
	if (m_fplqffspinbutton) {
		m_fplqffspinbutton->signal_focus_in_event().connect(sigc::bind(sigc::bind(sigc::mem_fun(*this, &FlightDeckWindow::fplpage_focus_in_event), kbd_numeric), m_fplqffspinbutton));
		m_fplqffspinbutton->signal_focus_out_event().connect(sigc::mem_fun(*this, &FlightDeckWindow::fplpage_focus_out_event));
		m_fplqffspinbutton->signal_changed().connect(sigc::bind(sigc::mem_fun(*this, &FlightDeckWindow::fplpage_change_event), fplchg_qff));
	}
	if (m_fploatspinbutton) {
		m_fploatspinbutton->signal_focus_in_event().connect(sigc::bind(sigc::bind(sigc::mem_fun(*this, &FlightDeckWindow::fplpage_focus_in_event), kbd_numeric), m_fploatspinbutton));
		m_fploatspinbutton->signal_focus_out_event().connect(sigc::mem_fun(*this, &FlightDeckWindow::fplpage_focus_out_event));
		m_fploatspinbutton->signal_changed().connect(sigc::bind(sigc::mem_fun(*this, &FlightDeckWindow::fplpage_change_event), fplchg_oat));
	}
	if (m_fplinsertbutton)
		m_fplinsertbutton->signal_clicked().connect(sigc::mem_fun(*this, &FlightDeckWindow::fplpage_insert_clicked));
	if (m_fplinserttxtbutton)
		m_fplinserttxtbutton->signal_clicked().connect(sigc::mem_fun(*this, &FlightDeckWindow::fplpage_inserttxt_clicked));
	if (m_fplduplicatebutton)
		m_fplduplicatebutton->signal_clicked().connect(sigc::mem_fun(*this, &FlightDeckWindow::fplpage_duplicate_clicked));
	if (m_fplmoveupbutton)
		m_fplmoveupbutton->signal_clicked().connect(sigc::mem_fun(*this, &FlightDeckWindow::fplpage_moveup_clicked));
	if (m_fplmovedownbutton)
		m_fplmovedownbutton->signal_clicked().connect(sigc::mem_fun(*this, &FlightDeckWindow::fplpage_movedown_clicked));
	if (m_fpldeletebutton)
		m_fpldeletebutton->signal_clicked().connect(sigc::mem_fun(*this, &FlightDeckWindow::fplpage_delete_clicked));
	if (m_fplstraightenbutton)
		m_fplstraightenbutton->signal_clicked().connect(sigc::mem_fun(*this, &FlightDeckWindow::fplpage_straighten_clicked));
	if (m_fpldirecttobutton)
		m_fpldirecttobutton->signal_clicked().connect(sigc::mem_fun(*this, &FlightDeckWindow::fplpage_directto_clicked));
	if (m_fplbrg2togglebutton)
		m_fplbrg2conn = m_fplbrg2togglebutton->signal_clicked().connect(sigc::mem_fun(*this, &FlightDeckWindow::fplpage_brg2_clicked));
	// Setup Flight Plan Database Page
	m_fpldbstore = Gtk::TreeStore::create(m_fpldbcolumns);
	if (m_fpldbtreeview) {
		m_fpldbtreeview->set_model(m_fpldbstore);
		Gtk::CellRendererText *fromicao_renderer = Gtk::manage(new Gtk::CellRendererText());
		int fromicao_col = m_fpldbtreeview->append_column(_("ICAO"), *fromicao_renderer) - 1;
		Gtk::TreeViewColumn *fromicao_column = m_fpldbtreeview->get_column(fromicao_col);
                if (fromicao_column) {
                        fromicao_column->add_attribute(*fromicao_renderer, "text", m_fpldbcolumns.m_fromicao);
                        fromicao_column->add_attribute(*fromicao_renderer, "style", m_fpldbcolumns.m_style);
		}
		Gtk::CellRendererText *fromname_renderer = Gtk::manage(new Gtk::CellRendererText());
		int fromname_col = m_fpldbtreeview->append_column(_("From"), *fromname_renderer) - 1;
		Gtk::TreeViewColumn *fromname_column = m_fpldbtreeview->get_column(fromname_col);
                if (fromname_column) {
                        fromname_column->add_attribute(*fromname_renderer, "text", m_fpldbcolumns.m_fromname);
                        fromname_column->add_attribute(*fromname_renderer, "style", m_fpldbcolumns.m_style);
		}
		Gtk::CellRendererText *toicao_renderer = Gtk::manage(new Gtk::CellRendererText());
		int toicao_col = m_fpldbtreeview->append_column(_("ICAO"), *toicao_renderer) - 1;
		Gtk::TreeViewColumn *toicao_column = m_fpldbtreeview->get_column(toicao_col);
                if (toicao_column) {
                        toicao_column->add_attribute(*toicao_renderer, "text", m_fpldbcolumns.m_toicao);
                        toicao_column->add_attribute(*toicao_renderer, "style", m_fpldbcolumns.m_style);
		}
		Gtk::CellRendererText *toname_renderer = Gtk::manage(new Gtk::CellRendererText());
		int toname_col = m_fpldbtreeview->append_column(_("To"), *toname_renderer) - 1;
		Gtk::TreeViewColumn *toname_column = m_fpldbtreeview->get_column(toname_col);
                if (toname_column) {
                        toname_column->add_attribute(*toname_renderer, "text", m_fpldbcolumns.m_toname);
			toname_column->add_attribute(*toname_renderer, "style", m_fpldbcolumns.m_style);
		}
		Gtk::CellRendererText *length_renderer = Gtk::manage(new Gtk::CellRendererText());
		int length_col = m_fpldbtreeview->append_column(_("Length"), *length_renderer) - 1;
		Gtk::TreeViewColumn *length_column = m_fpldbtreeview->get_column(length_col);
                if (length_column) {
 			length_column->set_cell_data_func(*length_renderer, sigc::bind<0>(sigc::ptr_fun(&FlightDeckWindow::fpldirpage_lengthcelldatafunc), m_fpldbcolumns.m_length));
			length_column->add_attribute(*length_renderer, "style", m_fpldbcolumns.m_style);
		}
                length_renderer->set_property("xalign", 1.0);
		Gtk::CellRendererText *dist_renderer = Gtk::manage(new Gtk::CellRendererText());
		int dist_col = m_fpldbtreeview->append_column(_("Dist"), *dist_renderer) - 1;
		Gtk::TreeViewColumn *dist_column = m_fpldbtreeview->get_column(dist_col);
                if (dist_column) {
			dist_column->set_cell_data_func(*dist_renderer, sigc::bind<0>(sigc::ptr_fun(&FlightDeckWindow::fpldirpage_lengthcelldatafunc), m_fpldbcolumns.m_dist));
                        dist_column->add_attribute(*dist_renderer, "style", m_fpldbcolumns.m_style);
		}
                dist_renderer->set_property("xalign", 1.0);
		const Gtk::TreeModelColumnBase *cols[] = { &m_fpldbcolumns.m_fromicao,
							   &m_fpldbcolumns.m_fromname,
							   &m_fpldbcolumns.m_toicao,
							   &m_fpldbcolumns.m_toname,
							   &m_fpldbcolumns.m_length,
							   &m_fpldbcolumns.m_dist };
		for (unsigned int i = 0; i < sizeof(cols)/sizeof(cols[0]); ++i) {
			Gtk::TreeViewColumn *col = m_fpldbtreeview->get_column(i);
			if (!col)
				continue;
			col->set_resizable(true);
			col->set_sort_column(*cols[i]);
			col->set_expand(i == 1 || i == 3);
		}
		m_fpldbstore->set_sort_func(m_fpldbcolumns.m_toname, sigc::bind<0>(sigc::mem_fun(*this, &FlightDeckWindow::fpldirpage_sortstringcol), m_fpldbcolumns.m_toname));
		m_fpldbstore->set_sort_func(m_fpldbcolumns.m_toicao, sigc::bind<0>(sigc::mem_fun(*this, &FlightDeckWindow::fpldirpage_sortstringcol), m_fpldbcolumns.m_toicao));
		m_fpldbstore->set_sort_func(m_fpldbcolumns.m_fromname, sigc::bind<0>(sigc::mem_fun(*this, &FlightDeckWindow::fpldirpage_sortstringcol), m_fpldbcolumns.m_fromname));
		m_fpldbstore->set_sort_func(m_fpldbcolumns.m_fromicao, sigc::bind<0>(sigc::mem_fun(*this, &FlightDeckWindow::fpldirpage_sortstringcol), m_fpldbcolumns.m_fromicao));
		m_fpldbstore->set_sort_func(m_fpldbcolumns.m_length, sigc::bind<0>(sigc::mem_fun(*this, &FlightDeckWindow::fpldirpage_sortfloatcol), m_fpldbcolumns.m_length));
		m_fpldbstore->set_sort_func(m_fpldbcolumns.m_dist, sigc::bind<0>(sigc::mem_fun(*this, &FlightDeckWindow::fpldirpage_sortfloatcol), m_fpldbcolumns.m_dist));
		m_fpldbtreeview->columns_autosize();
		m_fpldbtreeview->set_headers_clickable();
		Glib::RefPtr<Gtk::TreeSelection> selection = m_fpldbtreeview->get_selection();
		selection->set_mode(Gtk::SELECTION_MULTIPLE);
		m_fpldbselchangeconn = selection->signal_changed().connect(sigc::mem_fun(*this, &FlightDeckWindow::fpldb_selection_changed));
		selection->set_select_function(sigc::mem_fun(*this, &FlightDeckWindow::fpldb_select_function));
	}
	if (m_fpldbloadbutton)
		m_fpldbloadbutton->signal_clicked().connect(sigc::mem_fun(*this, &FlightDeckWindow::fpldirpage_buttonload));
	if (m_fpldbduplicatebutton)
		m_fpldbduplicatebutton->signal_clicked().connect(sigc::mem_fun(*this, &FlightDeckWindow::fpldirpage_buttonduplicate));
	if (m_fpldbdeletebutton)
		m_fpldbdeletebutton->signal_clicked().connect(sigc::mem_fun(*this, &FlightDeckWindow::fpldirpage_buttondelete));
	if (m_fpldbreversebutton)
		m_fpldbreversebutton->signal_clicked().connect(sigc::mem_fun(*this, &FlightDeckWindow::fpldirpage_buttonreverse));
	if (m_fpldbnewbutton)
		m_fpldbnewbutton->signal_clicked().connect(sigc::mem_fun(*this, &FlightDeckWindow::fpldirpage_buttonnew));
	if (m_fpldbimportbutton)
		m_fpldbimportbutton->signal_clicked().connect(sigc::mem_fun(*this, &FlightDeckWindow::fpldirpage_buttonimport));
	if (m_fpldbexportbutton)
		m_fpldbexportbutton->signal_clicked().connect(sigc::mem_fun(*this, &FlightDeckWindow::fpldirpage_buttonexport));
	if (m_fpldbdirecttobutton)
		m_fpldbdirecttobutton->signal_clicked().connect(sigc::mem_fun(*this, &FlightDeckWindow::fpldirpage_buttondirectto));
	// Flight Plan Database Confirm Dialog
	if (m_fpldbcfmtreeview) {
		Gtk::CellRendererText *fromname_renderer = Gtk::manage(new Gtk::CellRendererText());
		int fromname_col = m_fpldbcfmtreeview->append_column(_("From"), *fromname_renderer) - 1;
		Gtk::TreeViewColumn *fromname_column = m_fpldbcfmtreeview->get_column(fromname_col);
                if (fromname_column) {
                        fromname_column->add_attribute(*fromname_renderer, "text", m_fpldbcolumns.m_fromname);
                        fromname_column->add_attribute(*fromname_renderer, "style", m_fpldbcolumns.m_style);
		}
		Gtk::CellRendererText *fromicao_renderer = Gtk::manage(new Gtk::CellRendererText());
		int fromicao_col = m_fpldbcfmtreeview->append_column(_("ICAO"), *fromicao_renderer) - 1;
		Gtk::TreeViewColumn *fromicao_column = m_fpldbcfmtreeview->get_column(fromicao_col);
                if (fromicao_column) {
                        fromicao_column->add_attribute(*fromicao_renderer, "text", m_fpldbcolumns.m_fromicao);
                        fromicao_column->add_attribute(*fromicao_renderer, "style", m_fpldbcolumns.m_style);
		}
		Gtk::CellRendererText *toname_renderer = Gtk::manage(new Gtk::CellRendererText());
		int toname_col = m_fpldbcfmtreeview->append_column(_("To"), *toname_renderer) - 1;
		Gtk::TreeViewColumn *toname_column = m_fpldbcfmtreeview->get_column(toname_col);
                if (toname_column) {
                        toname_column->add_attribute(*toname_renderer, "text", m_fpldbcolumns.m_toname);
			toname_column->add_attribute(*toname_renderer, "style", m_fpldbcolumns.m_style);
		}
		Gtk::CellRendererText *toicao_renderer = Gtk::manage(new Gtk::CellRendererText());
		int toicao_col = m_fpldbcfmtreeview->append_column(_("ICAO"), *toicao_renderer) - 1;
		Gtk::TreeViewColumn *toicao_column = m_fpldbcfmtreeview->get_column(toicao_col);
                if (toicao_column) {
                        toicao_column->add_attribute(*toicao_renderer, "text", m_fpldbcolumns.m_toicao);
                        toicao_column->add_attribute(*toicao_renderer, "style", m_fpldbcolumns.m_style);
		}
		Gtk::CellRendererText *length_renderer = Gtk::manage(new Gtk::CellRendererText());
		int length_col = m_fpldbcfmtreeview->append_column(_("Length"), *length_renderer) - 1;
		Gtk::TreeViewColumn *length_column = m_fpldbcfmtreeview->get_column(length_col);
                if (length_column) {
 			length_column->set_cell_data_func(*length_renderer, sigc::bind<0>(sigc::ptr_fun(&FlightDeckWindow::fpldirpage_lengthcelldatafunc), m_fpldbcolumns.m_length));
			length_column->add_attribute(*length_renderer, "style", m_fpldbcolumns.m_style);
		}
                length_renderer->set_property("xalign", 1.0);
		Gtk::CellRendererText *dist_renderer = Gtk::manage(new Gtk::CellRendererText());
		int dist_col = m_fpldbcfmtreeview->append_column(_("Dist"), *dist_renderer) - 1;
		Gtk::TreeViewColumn *dist_column = m_fpldbcfmtreeview->get_column(dist_col);
                if (dist_column) {
			dist_column->set_cell_data_func(*dist_renderer, sigc::bind<0>(sigc::ptr_fun(&FlightDeckWindow::fpldirpage_lengthcelldatafunc), m_fpldbcolumns.m_dist));
                        dist_column->add_attribute(*dist_renderer, "style", m_fpldbcolumns.m_style);
		}
                dist_renderer->set_property("xalign", 1.0);
		const Gtk::TreeModelColumnBase *cols[] = { &m_fpldbcolumns.m_fromname,
							   &m_fpldbcolumns.m_fromicao,
							   &m_fpldbcolumns.m_toname,
							   &m_fpldbcolumns.m_toicao,
							   &m_fpldbcolumns.m_length,
							   &m_fpldbcolumns.m_dist };
		for (unsigned int i = 0; i < sizeof(cols)/sizeof(cols[0]); ++i) {
			Gtk::TreeViewColumn *col = m_fpldbcfmtreeview->get_column(i);
			if (!col)
				continue;
			col->set_resizable(true);
			col->set_sort_column(*cols[i]);
			col->set_expand(i == 0);
		}
		m_fpldbcfmtreeview->columns_autosize();
		m_fpldbcfmtreeview->set_headers_clickable();
		Glib::RefPtr<Gtk::TreeSelection> selection = m_fpldbcfmtreeview->get_selection();
		selection->set_mode(Gtk::SELECTION_NONE);
	}
	if (m_fpldbcfmbuttonok)
		m_fpldbcfmbuttonok->signal_clicked().connect(sigc::mem_fun(*this, &FlightDeckWindow::fpldirpage_cfmbuttonok));
	if (m_fpldbcfmbuttoncancel)
		m_fpldbcfmbuttoncancel->signal_clicked().connect(sigc::mem_fun(*this, &FlightDeckWindow::fpldirpage_cfmbuttoncancel));
	m_fpldbexportformat.remove_all();
	m_fpldbexportformat.append("Native");
	m_fpldbexportformat.append("GPX");
	m_fpldbexportformat.append("Garmin FPL");
	m_fpldbexportformat.set_active(0);
	m_fpldbexportformat.show();
	m_fpldbexportformattext.show();
	m_fpldbexportfilechooserextra.pack_start(m_fpldbexportformattext, false, true);
	m_fpldbexportfilechooserextra.pack_start(m_fpldbexportformat, false, true);
	m_fpldbexportfilechooserextra.show();
	m_fpldbexportfilechooser.set_extra_widget(m_fpldbexportfilechooserextra);
	if (true) {
		Glib::RefPtr<Gtk::FileFilter> filt(Gtk::FileFilter::create());
		filt->add_pattern("*.xml");
		filt->set_name("Native; GPX");
		m_fpldbimportfilechooser.add_filter(filt);
		m_fpldbexportfilechooser.add_filter(filt);
		filt = Gtk::FileFilter::create();
		filt->add_pattern("*.fpl");
		filt->set_name("Garmin FPL");
		m_fpldbimportfilechooser.add_filter(filt);
		m_fpldbexportfilechooser.add_filter(filt);
		filt = Gtk::FileFilter::create();
		filt->add_pattern("*");
		filt->set_name("All Files");
		m_fpldbimportfilechooser.add_filter(filt);
		m_fpldbexportfilechooser.add_filter(filt);
	}
	m_fpldbexportfilechooser.set_create_folders(true);
	m_fpldbexportfilechooser.set_do_overwrite_confirmation(true);
	m_fpldbexportfilechooser.set_local_only(true);
	m_fpldbimportfilechooser.set_local_only(true);
	m_fpldbimportfilechooser.set_select_multiple(true);
	m_fpldbexportfilechooser.set_transient_for(*this);
	m_fpldbimportfilechooser.set_transient_for(*this);
	m_fpldbexportfilechooser.signal_response().connect(sigc::mem_fun(*this, &FlightDeckWindow::fpldirpage_exportfileresponse));
	m_fpldbimportfilechooser.signal_response().connect(sigc::mem_fun(*this, &FlightDeckWindow::fpldirpage_importfileresponse));
	m_fpldbexportfilechooser.add_button(Gtk::Stock::CANCEL, Gtk::RESPONSE_CANCEL);
	m_fpldbexportfilechooser.add_button("Export", Gtk::RESPONSE_OK);
	m_fpldbimportfilechooser.add_button(Gtk::Stock::CANCEL, Gtk::RESPONSE_CANCEL);
	m_fpldbimportfilechooser.add_button("Import", Gtk::RESPONSE_OK);
	// Flight Plan GPS Import Page
	m_fplgpsimpstore = Gtk::ListStore::create(m_fplgpsimpcolumns);
	if (m_fplgpsimptreeview) {
		m_fplgpsimptreeview->set_model(m_fplgpsimpstore);
		Gtk::CellRendererText *name_renderer = Gtk::manage(new Gtk::CellRendererText());
		int name_col = m_fplgpsimptreeview->append_column(_("Name"), *name_renderer) - 1;
		Gtk::TreeViewColumn *name_column = m_fplgpsimptreeview->get_column(name_col);
                if (name_column) {
                        name_column->add_attribute(*name_renderer, "text", m_fplgpsimpcolumns.m_name);
                        name_column->add_attribute(*name_renderer, "weight", m_fplgpsimpcolumns.m_weight);
                        name_column->add_attribute(*name_renderer, "style", m_fplgpsimpcolumns.m_style);
		}
		Gtk::CellRendererText *icao_renderer = Gtk::manage(new Gtk::CellRendererText());
		int icao_col = m_fplgpsimptreeview->append_column(_("ICAO"), *icao_renderer) - 1;
		Gtk::TreeViewColumn *icao_column = m_fplgpsimptreeview->get_column(icao_col);
                if (icao_column) {
                        icao_column->add_attribute(*icao_renderer, "text", m_fplgpsimpcolumns.m_icao);
                        icao_column->add_attribute(*icao_renderer, "weight", m_fplgpsimpcolumns.m_weight);
                        icao_column->add_attribute(*icao_renderer, "style", m_fplgpsimpcolumns.m_style);
		}
		Gtk::CellRendererText *freq_renderer = Gtk::manage(new Gtk::CellRendererText());
		int freq_col = m_fplgpsimptreeview->append_column(_("Freq"), *freq_renderer) - 1;
		Gtk::TreeViewColumn *freq_column = m_fplgpsimptreeview->get_column(freq_col);
                if (freq_column) {
                        freq_column->add_attribute(*freq_renderer, "text", m_fplgpsimpcolumns.m_freq);
                        freq_column->add_attribute(*freq_renderer, "weight", m_fplgpsimpcolumns.m_weight);
                        freq_column->add_attribute(*freq_renderer, "style", m_fplgpsimpcolumns.m_style);
		}
		Gtk::CellRendererText *lon_renderer = Gtk::manage(new Gtk::CellRendererText());
		int lon_col = m_fplgpsimptreeview->append_column(_("Lon"), *lon_renderer) - 1;
		Gtk::TreeViewColumn *lon_column = m_fplgpsimptreeview->get_column(lon_col);
                if (lon_column) {
                        lon_column->add_attribute(*lon_renderer, "text", m_fplgpsimpcolumns.m_lon);
                        lon_column->add_attribute(*lon_renderer, "weight", m_fplgpsimpcolumns.m_weight);
                        lon_column->add_attribute(*lon_renderer, "style", m_fplgpsimpcolumns.m_style);
		}
		Gtk::CellRendererText *lat_renderer = Gtk::manage(new Gtk::CellRendererText());
		int lat_col = m_fplgpsimptreeview->append_column(_("Lat"), *lat_renderer) - 1;
		Gtk::TreeViewColumn *lat_column = m_fplgpsimptreeview->get_column(lat_col);
                if (lat_column) {
                        lat_column->add_attribute(*lat_renderer, "text", m_fplgpsimpcolumns.m_lat);
                        lat_column->add_attribute(*lat_renderer, "weight", m_fplgpsimpcolumns.m_weight);
                        lat_column->add_attribute(*lat_renderer, "style", m_fplgpsimpcolumns.m_style);
		}
		Gtk::CellRendererText *mt_renderer = Gtk::manage(new Gtk::CellRendererText());
		int mt_col = m_fplgpsimptreeview->append_column(_("MT"), *mt_renderer) - 1;
		Gtk::TreeViewColumn *mt_column = m_fplgpsimptreeview->get_column(mt_col);
                if (mt_column) {
                        mt_column->add_attribute(*mt_renderer, "text", m_fplgpsimpcolumns.m_mt);
                        mt_column->add_attribute(*mt_renderer, "weight", m_fplgpsimpcolumns.m_weight);
                        mt_column->add_attribute(*mt_renderer, "style", m_fplgpsimpcolumns.m_style);
		}
		Gtk::CellRendererText *tt_renderer = Gtk::manage(new Gtk::CellRendererText());
		int tt_col = m_fplgpsimptreeview->append_column(_("TT"), *tt_renderer) - 1;
		Gtk::TreeViewColumn *tt_column = m_fplgpsimptreeview->get_column(tt_col);
                if (tt_column) {
                        tt_column->add_attribute(*tt_renderer, "text", m_fplgpsimpcolumns.m_tt);
                        tt_column->add_attribute(*tt_renderer, "weight", m_fplgpsimpcolumns.m_weight);
                        tt_column->add_attribute(*tt_renderer, "style", m_fplgpsimpcolumns.m_style);
		}
		Gtk::CellRendererText *dist_renderer = Gtk::manage(new Gtk::CellRendererText());
		int dist_col = m_fplgpsimptreeview->append_column(_("Dist"), *dist_renderer) - 1;
		Gtk::TreeViewColumn *dist_column = m_fplgpsimptreeview->get_column(dist_col);
                if (dist_column) {
                        dist_column->add_attribute(*dist_renderer, "text", m_fplgpsimpcolumns.m_dist);
                        dist_column->add_attribute(*dist_renderer, "weight", m_fplgpsimpcolumns.m_weight);
                        dist_column->add_attribute(*dist_renderer, "style", m_fplgpsimpcolumns.m_style);
		}
                mt_renderer->set_property("xalign", 1.0);
                tt_renderer->set_property("xalign", 1.0);
                dist_renderer->set_property("xalign", 1.0);
		const Gtk::TreeModelColumnBase *cols[] = { &m_fplgpsimpcolumns.m_name,
							   &m_fplgpsimpcolumns.m_icao,
							   &m_fplgpsimpcolumns.m_freq,
							   &m_fplgpsimpcolumns.m_lon,
							   &m_fplgpsimpcolumns.m_lat,
							   &m_fplgpsimpcolumns.m_mt,
							   &m_fplgpsimpcolumns.m_tt,
							   &m_fplgpsimpcolumns.m_dist };
		for (unsigned int i = 0; i < sizeof(cols)/sizeof(cols[0]); ++i) {
			Gtk::TreeViewColumn *col = m_fplgpsimptreeview->get_column(i);
			if (!col)
				continue;
			col->set_resizable(true);
			//col->set_sort_column(*cols[i]);
			col->set_expand(i == 0);
		}
		m_fplgpsimptreeview->columns_autosize();
		//m_fplgpsimptreeview->set_headers_clickable();
		Glib::RefPtr<Gtk::TreeSelection> selection = m_fplgpsimptreeview->get_selection();
		selection->set_mode(Gtk::SELECTION_SINGLE);
		m_fplgpsimpselchangeconn = selection->signal_changed().connect(sigc::mem_fun(*this, &FlightDeckWindow::fplgpsimppage_selection_changed));
	}
	if (m_fplgpsimpdrawingarea)
		m_fplgpsimpdrawingarea->set_route(m_fplgpsimproute);
	// Route Vertical Profile Drawing Area
	if (true) {
		Glib::RefPtr<Gtk::FileFilter> filt(Gtk::FileFilter::create());
		filt->add_pattern("*.pdf");
		filt->set_name("PDF");
		m_wxchartexportfilechooser.add_filter(filt);
		filt = Gtk::FileFilter::create();
		filt->add_pattern("*.svg");
		filt->set_name("SVG");
		m_wxchartexportfilechooser.add_filter(filt);
		filt = Gtk::FileFilter::create();
		filt->add_pattern("*");
		filt->set_name("All Files");
		m_wxchartexportfilechooser.add_filter(filt);
	}
	m_wxchartexportfilechooser.set_create_folders(true);
	m_wxchartexportfilechooser.set_do_overwrite_confirmation(true);
	m_wxchartexportfilechooser.set_local_only(true);
	m_wxchartexportfilechooser.set_transient_for(*this);
	m_wxchartexportfilechooser.signal_response().connect(sigc::mem_fun(*this, &FlightDeckWindow::routeprofile_savefileresponse));
	m_wxchartexportfilechooser.add_button(Gtk::Stock::CANCEL, Gtk::RESPONSE_CANCEL);
	m_wxchartexportfilechooser.add_button("Save", Gtk::RESPONSE_OK);
	// METAR/TAF Page
	m_metartafstore = Gtk::ListStore::create(m_metartafcolumns);
	if (m_metartaftreeview) {
		m_metartaftreeview->set_model(m_metartafstore);
		Gtk::CellRendererText *icao_renderer = Gtk::manage(new Gtk::CellRendererText());
		int icao_col = m_metartaftreeview->append_column(_("ICAO"), *icao_renderer) - 1;
		Gtk::TreeViewColumn *icao_column = m_metartaftreeview->get_column(icao_col);
                if (icao_column) {
                        icao_column->add_attribute(*icao_renderer, "text", m_metartafcolumns.m_icao);
                        icao_column->add_attribute(*icao_renderer, "weight", m_metartafcolumns.m_weight);
                        icao_column->add_attribute(*icao_renderer, "style", m_metartafcolumns.m_style);
			icao_column->add_attribute(*icao_renderer, "foreground-gdk", m_metartafcolumns.m_color);
		}
		Gtk::CellRendererText *type_renderer = Gtk::manage(new Gtk::CellRendererText());
		int type_col = m_metartaftreeview->append_column(_("Type"), *type_renderer) - 1;
		Gtk::TreeViewColumn *type_column = m_metartaftreeview->get_column(type_col);
                if (type_column) {
                        type_column->add_attribute(*type_renderer, "text", m_metartafcolumns.m_type);
                        type_column->add_attribute(*type_renderer, "weight", m_metartafcolumns.m_weight);
                        type_column->add_attribute(*type_renderer, "style", m_metartafcolumns.m_style);
			type_column->add_attribute(*type_renderer, "foreground-gdk", m_metartafcolumns.m_color);
		}
		Gtk::CellRendererText *issue_renderer = Gtk::manage(new Gtk::CellRendererText());
		int issue_col = m_metartaftreeview->append_column(_("Issue"), *issue_renderer) - 1;
		Gtk::TreeViewColumn *issue_column = m_metartaftreeview->get_column(issue_col);
                if (issue_column) {
                        issue_column->add_attribute(*issue_renderer, "text", m_metartafcolumns.m_issue);
                        issue_column->add_attribute(*issue_renderer, "weight", m_metartafcolumns.m_weight);
                        issue_column->add_attribute(*issue_renderer, "style", m_metartafcolumns.m_style);
			issue_column->add_attribute(*issue_renderer, "foreground-gdk", m_metartafcolumns.m_color);
		}
		Gtk::CellRendererText *message_renderer = Gtk::manage(new Gtk::CellRendererText());
		int message_col = m_metartaftreeview->append_column(_("Message"), *message_renderer) - 1;
		Gtk::TreeViewColumn *message_column = m_metartaftreeview->get_column(message_col);
                if (message_column) {
                        message_column->add_attribute(*message_renderer, "text", m_metartafcolumns.m_message);
                        message_column->add_attribute(*message_renderer, "weight", m_metartafcolumns.m_weight);
                        message_column->add_attribute(*message_renderer, "style", m_metartafcolumns.m_style);
			message_column->add_attribute(*message_renderer, "foreground-gdk", m_metartafcolumns.m_color);
		}
		icao_renderer->set_alignment(0, 0);
		type_renderer->set_alignment(0, 0);
		issue_renderer->set_alignment(0, 0);
		message_renderer->set_alignment(0, 0);
		message_renderer->set_property("wrap-mode", Pango::WRAP_WORD);
		const Gtk::TreeModelColumnBase *cols[] = { &m_metartafcolumns.m_icao,
							   &m_metartafcolumns.m_type,
							   &m_metartafcolumns.m_issue,
							   &m_metartafcolumns.m_message };
		for (unsigned int i = 0; i < sizeof(cols)/sizeof(cols[0]); ++i) {
			Gtk::TreeViewColumn *col = m_metartaftreeview->get_column(i);
			if (!col)
				continue;
			col->set_resizable(true);
			//col->set_sort_column(*cols[i]);
			col->set_expand(i == 3);
		}
		m_metartaftreeview->columns_autosize();
		//m_metartaftreeview->set_headers_clickable();
		Glib::RefPtr<Gtk::TreeSelection> selection = m_metartaftreeview->get_selection();
		selection->set_mode(Gtk::SELECTION_NONE);
	}
	// Wx Chart List Page
	m_wxchartliststore = Gtk::ListStore::create(m_wxchartlistcolumns);
	if (m_wxchartlisttreeview) {
		m_wxchartlisttreeview->set_model(m_wxchartliststore);
		Gtk::CellRendererText *epoch_renderer = Gtk::manage(new Gtk::CellRendererText());
		int epoch_col = m_wxchartlisttreeview->append_column(_("Time"), *epoch_renderer) - 1;
		Gtk::TreeViewColumn *epoch_column = m_wxchartlisttreeview->get_column(epoch_col);
                if (epoch_column)
                        epoch_column->add_attribute(*epoch_renderer, "text", m_wxchartlistcolumns.m_epoch);
		Gtk::CellRendererText *level_renderer = Gtk::manage(new Gtk::CellRendererText());
		int level_col = m_wxchartlisttreeview->append_column(_("Level"), *level_renderer) - 1;
		Gtk::TreeViewColumn *level_column = m_wxchartlisttreeview->get_column(level_col);
                if (level_column)
                        level_column->add_attribute(*level_renderer, "text", m_wxchartlistcolumns.m_level);
		Gtk::CellRendererText *createtime_renderer = Gtk::manage(new Gtk::CellRendererText());
		int createtime_col = m_wxchartlisttreeview->append_column(_("Createtime"), *createtime_renderer) - 1;
		Gtk::TreeViewColumn *createtime_column = m_wxchartlisttreeview->get_column(createtime_col);
                if (createtime_column)
                        createtime_column->add_attribute(*createtime_renderer, "text", m_wxchartlistcolumns.m_createtime);
		Gtk::CellRendererText *departure_renderer = Gtk::manage(new Gtk::CellRendererText());
		int departure_col = m_wxchartlisttreeview->append_column(_("Departure"), *departure_renderer) - 1;
		Gtk::TreeViewColumn *departure_column = m_wxchartlisttreeview->get_column(departure_col);
                if (departure_column)
                        departure_column->add_attribute(*departure_renderer, "text", m_wxchartlistcolumns.m_departure);
		Gtk::CellRendererText *destination_renderer = Gtk::manage(new Gtk::CellRendererText());
		int destination_col = m_wxchartlisttreeview->append_column(_("Destination"), *destination_renderer) - 1;
		Gtk::TreeViewColumn *destination_column = m_wxchartlisttreeview->get_column(destination_col);
                if (destination_column)
                        destination_column->add_attribute(*destination_renderer, "text", m_wxchartlistcolumns.m_destination);

		Gtk::CellRendererText *description_renderer = Gtk::manage(new Gtk::CellRendererText());
		int description_col = m_wxchartlisttreeview->append_column(_("Description"), *description_renderer) - 1;
		Gtk::TreeViewColumn *description_column = m_wxchartlisttreeview->get_column(description_col);
                if (description_column)
                        description_column->add_attribute(*description_renderer, "text", m_wxchartlistcolumns.m_description);
		epoch_renderer->set_alignment(0, 0);
		level_renderer->set_alignment(0, 0);
		createtime_renderer->set_alignment(0, 0);
		departure_renderer->set_alignment(0, 0);
		destination_renderer->set_alignment(0, 0);
		description_renderer->set_alignment(0, 0);
		description_renderer->set_property("wrap-mode", Pango::WRAP_WORD);
		const Gtk::TreeModelColumnBase *cols[] = { &m_wxchartlistcolumns.m_epoch,
							   &m_wxchartlistcolumns.m_level,
							   &m_wxchartlistcolumns.m_createtime,
							   &m_wxchartlistcolumns.m_departure,
							   &m_wxchartlistcolumns.m_destination,
							   &m_wxchartlistcolumns.m_description };
		for (unsigned int i = 0; i < sizeof(cols)/sizeof(cols[0]); ++i) {
			Gtk::TreeViewColumn *col = m_wxchartlisttreeview->get_column(i);
			if (!col)
				continue;
			col->set_resizable(true);
			//col->set_sort_column(*cols[i]);
			col->set_expand(i == 5);
		}
		m_wxchartlisttreeview->columns_autosize();
		//m_wxchartlisttreeview->set_headers_clickable();
		Glib::RefPtr<Gtk::TreeSelection> selection = m_wxchartlisttreeview->get_selection();
		selection->set_mode(Gtk::SELECTION_SINGLE);
	}
#ifdef HAVE_EVINCE
	// Setup Documents Directory Page
	if (!ev_init())
		std::cerr << "Warning: Evince: no backends found" << std::endl;
	m_docdirstore = Gtk::TreeStore::create(m_docdircolumns);
	if (m_docdirtreeview) {
		m_docdirtreeview->set_model(m_docdirstore);
		Gtk::CellRendererText *text_renderer = Gtk::manage(new Gtk::CellRendererText());
		int text_col = m_docdirtreeview->append_column(_("Text"), *text_renderer) - 1;
		Gtk::TreeViewColumn *text_column = m_docdirtreeview->get_column(text_col);
                if (text_column) {
                        text_column->add_attribute(*text_renderer, "text", m_docdircolumns.m_text);
			text_column->add_attribute(*text_renderer, "weight", m_docdircolumns.m_weight);
                        text_column->add_attribute(*text_renderer, "style", m_docdircolumns.m_style);
		}
		const Gtk::TreeModelColumnBase *cols[] = { &m_docdircolumns.m_text };
		for (unsigned int i = 0; i < sizeof(cols)/sizeof(cols[0]); ++i) {
			Gtk::TreeViewColumn *col = m_docdirtreeview->get_column(i);
			if (!col)
				continue;
			col->set_resizable(true);
			col->set_sort_column(*cols[i]);
			col->set_expand(i == 0);
		}
		m_docdirtreeview->columns_autosize();
		m_docdirtreeview->set_headers_clickable();
		Glib::RefPtr<Gtk::TreeSelection> selection = m_docdirtreeview->get_selection();
		selection->set_mode(Gtk::SELECTION_SINGLE);
		selection->signal_changed().connect(sigc::mem_fun(*this, &FlightDeckWindow::docdir_selection_changed));
		selection->set_select_function(sigc::mem_fun(*this, &FlightDeckWindow::docdir_select_function));
	}
	// Setup Documents Page
	m_docevdocmodel = ev_document_model_new();
	m_docevdocmodel_sighandler = g_signal_connect(m_docevdocmodel, "page-changed", G_CALLBACK(FlightDeckWindow::docdir_page_changed), this);
	if (m_docscrolledwindow) {
		m_docevview = Gtk::manage(Glib::wrap(ev_view_new()));
		ev_view_set_model((EvView *)m_docevview->gobj(), m_docevdocmodel);
		m_docscrolledwindow->add(*m_docevview);
		m_docevview->show();
		m_docevview->signal_motion_notify_event().connect(sigc::mem_fun(*this, &FlightDeckWindow::docpage_on_motion_notify_event));
	}
#endif
	// Performace Page
	if (m_perfbuttonfromwb)
		m_perfbuttonfromwb->signal_clicked().connect(sigc::mem_fun(*this, &FlightDeckWindow::perfpage_button_fromwb_clicked));
	if (m_perfbuttonfromfpl)
		m_perfbuttonfromfpl->signal_clicked().connect(sigc::mem_fun(*this, &FlightDeckWindow::perfpage_button_fromfpl_clicked));
	if (m_perfspinbuttonsoftsurface) {
		m_perfspinbuttonsoftsurface->set_value(m_perfsoftfieldpenalty * 100);
		m_perfsoftsurfaceconn = m_perfspinbuttonsoftsurface->signal_value_changed().connect(sigc::mem_fun(*this, &FlightDeckWindow::perfpage_softpenalty_value_changed));
	}
	for (unsigned int i = 0; i < perfunits_nr; ++i) {
		m_perfunitsstore[i] = Gtk::ListStore::create(m_perfunitscolumns);
		if (!m_perfcombobox[i])
			continue;
		m_perfcombobox[i]->clear();
		m_perfcombobox[i]->set_model(m_perfunitsstore[i]);
		m_perfcombobox[i]->set_entry_text_column(m_perfunitscolumns.m_text);
		m_perfcombobox[i]->pack_start(m_perfunitscolumns.m_text);
		m_perfcombobox[i]->signal_changed().connect(sigc::bind(sigc::mem_fun(*this, &FlightDeckWindow::perfpage_unit_changed), (perfunits_t)i));
	}
	for (unsigned int i = 0; i < perfval_nr; ++i) {
		for (unsigned int j = 0; j < 2; ++j) {
			m_perfvalues[j][i] = 0;
			if (!m_perfspinbutton[j][i])
				continue;
			m_perfspinbuttonconn[j][i] = m_perfspinbutton[j][i]->signal_value_changed().connect(sigc::bind(sigc::bind(sigc::mem_fun(*this, &FlightDeckWindow::perfpage_spinbutton_value_changed), (perfval_t)i), j));
			m_perfspinbutton[j][i]->signal_focus_in_event().connect(sigc::bind(sigc::bind(sigc::mem_fun(*this, &FlightDeckWindow::onscreenkbd_focus_in_event), kbd_numeric), m_perfspinbutton[j][i]));
			m_perfspinbutton[j][i]->signal_focus_out_event().connect(sigc::mem_fun(*this, &FlightDeckWindow::onscreenkbd_focus_out_event));
			m_perfspinbutton[j][i]->set_alignment(1.0);
		}
	}
	for (unsigned int j = 0; j < 2; ++j) {
		m_perfresultstore[j] = Gtk::TreeStore::create(m_perfresultcolumns);
		if (!m_perftreeview[j])
			continue;
		m_perftreeview[j]->set_model(m_perfresultstore[j]);
		Gtk::CellRendererText *text_renderer = Gtk::manage(new Gtk::CellRendererText());
		int text_col = m_perftreeview[j]->append_column(_("Text"), *text_renderer) - 1;
		Gtk::TreeViewColumn *text_column = m_perftreeview[j]->get_column(text_col);
                if (text_column)
                        text_column->add_attribute(*text_renderer, "text", m_perfresultcolumns.m_text);
 		Gtk::CellRendererText *value1_renderer = Gtk::manage(new Gtk::CellRendererText());
		int value1_col = m_perftreeview[j]->append_column(_("Value 1"), *value1_renderer) - 1;
		Gtk::TreeViewColumn *value1_column = m_perftreeview[j]->get_column(value1_col);
                if (value1_column)
                        value1_column->add_attribute(*value1_renderer, "text", m_perfresultcolumns.m_value1);
 		Gtk::CellRendererText *value2_renderer = Gtk::manage(new Gtk::CellRendererText());
		int value2_col = m_perftreeview[j]->append_column(_("Value 2"), *value2_renderer) - 1;
		Gtk::TreeViewColumn *value2_column = m_perftreeview[j]->get_column(value2_col);
                if (value2_column)
                        value2_column->add_attribute(*value2_renderer, "text", m_perfresultcolumns.m_value2);
                value1_renderer->set_property("xalign", 1.0);
                value2_renderer->set_property("xalign", 1.0);
		const Gtk::TreeModelColumnBase *cols[] = { &m_perfresultcolumns.m_text,
							   &m_perfresultcolumns.m_value1,
							   &m_perfresultcolumns.m_value2 };
		for (unsigned int i = 0; i < sizeof(cols)/sizeof(cols[0]); ++i) {
			Gtk::TreeViewColumn *col = m_perftreeview[j]->get_column(i);
			if (!col)
				continue;
			col->set_resizable(true);
			//col->set_sort_column(*cols[i]);
			col->set_expand(i == 0);
		}
		m_perftreeview[j]->columns_autosize();
		Glib::RefPtr<Gtk::TreeSelection> selection = m_perftreeview[j]->get_selection();
		selection->set_mode(Gtk::SELECTION_NONE);
	}
	m_perfwbstore = Gtk::ListStore::create(m_perfwbcolumns);
	if (m_perftreeviewwb) {
		m_perftreeviewwb->set_model(m_perfwbstore);
		Gtk::CellRendererText *text_renderer = Gtk::manage(new Gtk::CellRendererText());
		int text_col = m_perftreeviewwb->append_column(_("Text"), *text_renderer) - 1;
		Gtk::TreeViewColumn *text_column = m_perftreeviewwb->get_column(text_col);
                if (text_column) {
                        text_column->add_attribute(*text_renderer, "text", m_perfwbcolumns.m_text);
			text_column->add_attribute(*text_renderer, "style", m_perfwbcolumns.m_style);
		}
		Gtk::CellRendererText *value_renderer = Gtk::manage(new Gtk::CellRendererText());
		int value_col = m_perftreeviewwb->append_column(_("Value"), *value_renderer) - 1;
		Gtk::TreeViewColumn *value_column = m_perftreeviewwb->get_column(value_col);
                if (value_column) {
                        value_column->add_attribute(*value_renderer, "editable", m_perfwbcolumns.m_valueeditable);
			value_column->set_cell_data_func(*value_renderer, sigc::bind(sigc::mem_fun(*this, &FlightDeckWindow::perf_cell_data_func), m_perfwbcolumns.m_value.index()));
			value_column->add_attribute(*value_renderer, "style", m_perfwbcolumns.m_style);
		}
		if (value_renderer)
			value_renderer->signal_edited().connect(sigc::bind(sigc::mem_fun(*this, &FlightDeckWindow::perf_cellrenderer_edited), m_perfwbcolumns.m_value.index()));
		Gtk::CellRendererCombo *unit_renderer = Gtk::manage(new Gtk::CellRendererCombo());
		int unit_col = m_perftreeviewwb->append_column(_("Unit"), *unit_renderer) - 1;
		Gtk::TreeViewColumn *unit_column = m_perftreeviewwb->get_column(unit_col);
                if (unit_column) {
                        unit_column->add_attribute(*unit_renderer, "text", m_perfwbcolumns.m_unit);
                        unit_column->add_attribute(*unit_renderer, "model", m_perfwbcolumns.m_model);
                        unit_column->add_attribute(*unit_renderer, "visible", m_perfwbcolumns.m_modelvisible);
			unit_column->add_attribute(*unit_renderer, "style", m_perfwbcolumns.m_style);
		}
		if (unit_renderer)
			unit_renderer->signal_edited().connect(sigc::mem_fun(*this, &FlightDeckWindow::perf_cellrendererunit_edited));
		Gtk::CellRendererText *mass_renderer = Gtk::manage(new Gtk::CellRendererText());
		int mass_col = m_perftreeviewwb->append_column(_("Mass"), *mass_renderer) - 1;
		Gtk::TreeViewColumn *mass_column = m_perftreeviewwb->get_column(mass_col);
                if (mass_column) {
			mass_column->set_cell_data_func(*mass_renderer, sigc::bind(sigc::mem_fun(*this, &FlightDeckWindow::perf_cell_data_func), m_perfwbcolumns.m_mass.index()));
			mass_column->add_attribute(*mass_renderer, "style", m_perfwbcolumns.m_style);
		}
		Gtk::CellRendererText *minmass_renderer = Gtk::manage(new Gtk::CellRendererText());
		int minmass_col = m_perftreeviewwb->append_column(_("Minimum"), *minmass_renderer) - 1;
		Gtk::TreeViewColumn *minmass_column = m_perftreeviewwb->get_column(minmass_col);
                if (minmass_column) {
			minmass_column->set_cell_data_func(*minmass_renderer, sigc::bind(sigc::mem_fun(*this, &FlightDeckWindow::perf_cell_data_func), m_perfwbcolumns.m_minmass.index()));
			minmass_column->add_attribute(*minmass_renderer, "style", m_perfwbcolumns.m_style);
		}
		Gtk::CellRendererText *maxmass_renderer = Gtk::manage(new Gtk::CellRendererText());
		int maxmass_col = m_perftreeviewwb->append_column(_("Maximum"), *maxmass_renderer) - 1;
		Gtk::TreeViewColumn *maxmass_column = m_perftreeviewwb->get_column(maxmass_col);
                if (maxmass_column) {
			maxmass_column->set_cell_data_func(*maxmass_renderer, sigc::bind(sigc::mem_fun(*this, &FlightDeckWindow::perf_cell_data_func), m_perfwbcolumns.m_maxmass.index()));
			maxmass_column->add_attribute(*maxmass_renderer, "style", m_perfwbcolumns.m_style);
		}
 		Gtk::CellRendererText *arm_renderer = Gtk::manage(new Gtk::CellRendererText());
		int arm_col = m_perftreeviewwb->append_column(_("ARM"), *arm_renderer) - 1;
		Gtk::TreeViewColumn *arm_column = m_perftreeviewwb->get_column(arm_col);
                if (arm_column) {
 			arm_column->set_cell_data_func(*arm_renderer, sigc::bind(sigc::mem_fun(*this, &FlightDeckWindow::perf_cell_data_func), m_perfwbcolumns.m_arm.index()));
			arm_column->add_attribute(*arm_renderer, "style", m_perfwbcolumns.m_style);
		}
		Gtk::CellRendererText *moment_renderer = Gtk::manage(new Gtk::CellRendererText());
		int moment_col = m_perftreeviewwb->append_column(_("Moment"), *moment_renderer) - 1;
		Gtk::TreeViewColumn *moment_column = m_perftreeviewwb->get_column(moment_col);
                if (moment_column) {
 			moment_column->set_cell_data_func(*moment_renderer, sigc::bind(sigc::mem_fun(*this, &FlightDeckWindow::perf_cell_data_func), m_perfwbcolumns.m_moment.index()));
			moment_column->add_attribute(*moment_renderer, "style", m_perfwbcolumns.m_style);
		}
 		value_renderer->set_property("xalign", 1.0);
		unit_renderer->set_property("text-column", m_perfunitscolumns.m_text.index());
		unit_renderer->set_property("editable", true);
		unit_renderer->set_property("has-entry", false);
		mass_renderer->set_property("xalign", 1.0);
		minmass_renderer->set_property("xalign", 1.0);
		maxmass_renderer->set_property("xalign", 1.0);
                arm_renderer->set_property("xalign", 1.0);
                moment_renderer->set_property("xalign", 1.0);
		const Gtk::TreeModelColumnBase *cols[] = { &m_perfwbcolumns.m_text,
							   &m_perfwbcolumns.m_value,
							   &m_perfwbcolumns.m_model,
							   &m_perfwbcolumns.m_mass,
							   &m_perfwbcolumns.m_minmass,
							   &m_perfwbcolumns.m_maxmass,
							   &m_perfwbcolumns.m_arm,
							   &m_perfwbcolumns.m_moment };
		for (unsigned int i = 0; i < sizeof(cols)/sizeof(cols[0]); ++i) {
			Gtk::TreeViewColumn *col = m_perftreeviewwb->get_column(i);
			if (!col)
				continue;
			col->set_resizable(true);
			//col->set_sort_column(*cols[i]);
			col->set_expand(i == 0);
		}
		m_perftreeviewwb->columns_autosize();
		//m_perftreeviewwb->set_headers_clickable();
		Glib::RefPtr<Gtk::TreeSelection> selection = m_perftreeviewwb->get_selection();
		selection->set_mode(Gtk::SELECTION_SINGLE);
	}
	{
		Gtk::TreeModel::Row row(*(m_perfunitsstore[perfunits_mass]->append()));
		row[m_perfunitscolumns.m_text] = "kg";
		row[m_perfunitscolumns.m_gain] = 1;
		row[m_perfunitscolumns.m_offset] = 0;
		row = *(m_perfunitsstore[perfunits_mass]->append());
		row[m_perfunitscolumns.m_text] = "lb";
		row[m_perfunitscolumns.m_gain] = 0.45359237;
		row[m_perfunitscolumns.m_offset] = 0;

		row = *(m_perfunitsstore[perfunits_altitude]->append());
		row[m_perfunitscolumns.m_text] = "m";
		row[m_perfunitscolumns.m_gain] = 1;
		row[m_perfunitscolumns.m_offset] = 0;
		row = *(m_perfunitsstore[perfunits_altitude]->append());
		row[m_perfunitscolumns.m_text] = "ft";
		row[m_perfunitscolumns.m_gain] = 0.3048;
		row[m_perfunitscolumns.m_offset] = 0;

		row = *(m_perfunitsstore[perfunits_pressure]->append());
		row[m_perfunitscolumns.m_text] = "hPa";
		row[m_perfunitscolumns.m_gain] = 1;
		row[m_perfunitscolumns.m_offset] = 0;
		row = *(m_perfunitsstore[perfunits_pressure]->append());
		row[m_perfunitscolumns.m_text] = "inHg";
		row[m_perfunitscolumns.m_gain] = 33.863886;
		row[m_perfunitscolumns.m_offset] = 0;
		row = *(m_perfunitsstore[perfunits_pressure]->append());
		row[m_perfunitscolumns.m_text] = "mmHg";
		row[m_perfunitscolumns.m_gain] = 1.3332239;
		row[m_perfunitscolumns.m_offset] = 0;

		row = *(m_perfunitsstore[perfunits_temperature]->append());
		row[m_perfunitscolumns.m_text] = Glib::ustring::format((wchar_t)0xb0, "C");
		row[m_perfunitscolumns.m_gain] = 1;
		row[m_perfunitscolumns.m_offset] = IcaoAtmosphere<double>::degc_to_kelvin;
		row = *(m_perfunitsstore[perfunits_temperature]->append());
		row[m_perfunitscolumns.m_text] = Glib::ustring::format((wchar_t)0xb0, "F");
		row[m_perfunitscolumns.m_gain] = 0.55556;
		row[m_perfunitscolumns.m_offset] = 255.37222;
		row = *(m_perfunitsstore[perfunits_temperature]->append());
		row[m_perfunitscolumns.m_text] = "K";
		row[m_perfunitscolumns.m_gain] = 1;
		row[m_perfunitscolumns.m_offset] = 0;

		row = *(m_perfunitsstore[perfunits_speed]->append());
		row[m_perfunitscolumns.m_text] = "m/s";
		row[m_perfunitscolumns.m_gain] = 1;
		row[m_perfunitscolumns.m_offset] = 0;
		row = *(m_perfunitsstore[perfunits_speed]->append());
		row[m_perfunitscolumns.m_text] = "km/h";
		row[m_perfunitscolumns.m_gain] = 1.0/3.6;
		row[m_perfunitscolumns.m_offset] = 0;
		row = *(m_perfunitsstore[perfunits_speed]->append());
		row[m_perfunitscolumns.m_text] = "kts";
		row[m_perfunitscolumns.m_gain] = 0.51444444;
		row[m_perfunitscolumns.m_offset] = 0;
		row = *(m_perfunitsstore[perfunits_speed]->append());
		row[m_perfunitscolumns.m_text] = "mph";
		row[m_perfunitscolumns.m_gain] = 0.44704;
		row[m_perfunitscolumns.m_offset] = 0;
	}
	for (unsigned int i = 0; i < perfunits_nr; ++i) {
		if (!m_perfcombobox[i])
			continue;
		m_perfcombobox[i]->set_active(0);
	}
	m_perfvalues[0][perfval_qnh] = m_perfvalues[1][perfval_qnh] = IcaoAtmosphere<double>::std_sealevel_pressure;
	m_perfvalues[0][perfval_dewpt] = m_perfvalues[1][perfval_dewpt] = IcaoAtmosphere<double>::degc_to_kelvin;
	// Sunrise Sunset Page
	m_srssstore = Gtk::ListStore::create(m_srsscolumns);
	if (m_srsstreeview) {
		m_srsstreeview->set_model(m_srssstore);
		Gtk::CellRendererText *name_renderer = Gtk::manage(new Gtk::CellRendererText());
		int name_col = m_srsstreeview->append_column(_("Name"), *name_renderer) - 1;
		Gtk::TreeViewColumn *name_column = m_srsstreeview->get_column(name_col);
                if (name_column)
                        name_column->add_attribute(*name_renderer, "text", m_srsscolumns.m_name);
		Gtk::CellRendererText *icao_renderer = Gtk::manage(new Gtk::CellRendererText());
		int icao_col = m_srsstreeview->append_column(_("ICAO"), *icao_renderer) - 1;
		Gtk::TreeViewColumn *icao_column = m_srsstreeview->get_column(icao_col);
                if (icao_column)
                        icao_column->add_attribute(*icao_renderer, "text", m_srsscolumns.m_icao);
 		Gtk::CellRendererText *sunrise_renderer = Gtk::manage(new Gtk::CellRendererText());
		int sunrise_col = m_srsstreeview->append_column(_("Sunrise"), *sunrise_renderer) - 1;
		Gtk::TreeViewColumn *sunrise_column = m_srsstreeview->get_column(sunrise_col);
                if (sunrise_column)
                        sunrise_column->add_attribute(*sunrise_renderer, "text", m_srsscolumns.m_sunrise);
		Gtk::CellRendererText *sunset_renderer = Gtk::manage(new Gtk::CellRendererText());
		int sunset_col = m_srsstreeview->append_column(_("Sunset"), *sunset_renderer) - 1;
		Gtk::TreeViewColumn *sunset_column = m_srsstreeview->get_column(sunset_col);
                if (sunset_column)
                        sunset_column->add_attribute(*sunset_renderer, "text", m_srsscolumns.m_sunset);
		Gtk::CellRendererText *lon_renderer = Gtk::manage(new Gtk::CellRendererText());
		int lon_col = m_srsstreeview->append_column(_("Lon"), *lon_renderer) - 1;
		Gtk::TreeViewColumn *lon_column = m_srsstreeview->get_column(lon_col);
                if (lon_column)
                        lon_column->add_attribute(*lon_renderer, "text", m_srsscolumns.m_lon);
		Gtk::CellRendererText *lat_renderer = Gtk::manage(new Gtk::CellRendererText());
		int lat_col = m_srsstreeview->append_column(_("Lat"), *lat_renderer) - 1;
		Gtk::TreeViewColumn *lat_column = m_srsstreeview->get_column(lat_col);
                if (lat_column)
                        lat_column->add_attribute(*lat_renderer, "text", m_srsscolumns.m_lat);
		Gtk::CellRendererText *mt_renderer = Gtk::manage(new Gtk::CellRendererText());
		int mt_col = m_srsstreeview->append_column(_("MT"), *mt_renderer) - 1;
		Gtk::TreeViewColumn *mt_column = m_srsstreeview->get_column(mt_col);
                if (mt_column)
                        mt_column->add_attribute(*mt_renderer, "text", m_srsscolumns.m_mt);
		Gtk::CellRendererText *tt_renderer = Gtk::manage(new Gtk::CellRendererText());
		int tt_col = m_srsstreeview->append_column(_("TT"), *tt_renderer) - 1;
		Gtk::TreeViewColumn *tt_column = m_srsstreeview->get_column(tt_col);
                if (tt_column)
                        tt_column->add_attribute(*tt_renderer, "text", m_srsscolumns.m_tt);
		Gtk::CellRendererText *dist_renderer = Gtk::manage(new Gtk::CellRendererText());
		int dist_col = m_srsstreeview->append_column(_("Dist"), *dist_renderer) - 1;
		Gtk::TreeViewColumn *dist_column = m_srsstreeview->get_column(dist_col);
                if (dist_column)
                        dist_column->add_attribute(*dist_renderer, "text", m_srsscolumns.m_dist);
                sunrise_renderer->set_property("xalign", 1.0);
                sunset_renderer->set_property("xalign", 1.0);
                mt_renderer->set_property("xalign", 1.0);
                tt_renderer->set_property("xalign", 1.0);
                dist_renderer->set_property("xalign", 1.0);
		const Gtk::TreeModelColumnBase *cols[] = { &m_srsscolumns.m_name,
							   &m_srsscolumns.m_icao,
							   &m_srsscolumns.m_sunrise,
							   &m_srsscolumns.m_sunset,
							   &m_srsscolumns.m_lon,
							   &m_srsscolumns.m_lat,
							   &m_srsscolumns.m_mt,
							   &m_srsscolumns.m_tt,
							   &m_srsscolumns.m_dist };
		for (unsigned int i = 0; i < sizeof(cols)/sizeof(cols[0]); ++i) {
			Gtk::TreeViewColumn *col = m_srsstreeview->get_column(i);
			if (!col)
				continue;
			col->set_resizable(true);
			//col->set_sort_column(*cols[i]);
			col->set_expand(i == 0 || true);
		}
		m_srsstreeview->columns_autosize();
		//m_srsstreeview->set_headers_clickable();
		Glib::RefPtr<Gtk::TreeSelection> selection = m_srsstreeview->get_selection();
		selection->set_mode(Gtk::SELECTION_NONE);
	}
	if (m_srsscalendar)
		m_srsscalendar->signal_day_selected().connect(sigc::mem_fun(*this, &FlightDeckWindow::srsspage_recompute));
	// EU OPS1 Minimum Calculator (on Sunrise Sunset Page)
	m_euops1tdzstore = Gtk::ListStore::create(m_euops1tdzcolumns);
	if (m_euops1comboboxtdzlist) {
		m_euops1comboboxtdzlist->set_model(m_euops1tdzstore);
		m_euops1comboboxtdzlist->set_entry_text_column(m_euops1tdzcolumns.m_text);
                m_euops1comboboxtdzlist->pack_start(m_euops1tdzcolumns.m_text);
		m_euops1tdzlistchange = m_euops1comboboxtdzlist->signal_changed().connect(sigc::mem_fun(*this, &FlightDeckWindow::euops1_change_tdzlist));
	}
	if (m_euops1comboboxapproach)
		m_euops1comboboxapproach->signal_changed().connect(sigc::mem_fun(*this, &FlightDeckWindow::euops1_recomputeldg));
	if (m_euops1spinbuttontdzelev)
		m_euops1tdzelevchange = m_euops1spinbuttontdzelev->signal_changed().connect(sigc::mem_fun(*this, &FlightDeckWindow::euops1_change_tdzelev));
	if (m_euops1comboboxtdzelevunit)
		m_euops1tdzunitchange = m_euops1comboboxtdzelevunit->signal_changed().connect(sigc::mem_fun(*this, &FlightDeckWindow::euops1_change_tdzunit));
	if (m_euops1comboboxocaoch)
		m_euops1comboboxocaoch->signal_changed().connect(sigc::mem_fun(*this, &FlightDeckWindow::euops1_recomputeldg));
	if (m_euops1spinbuttonocaoch)
		m_euops1ocaochchange = m_euops1spinbuttonocaoch->signal_changed().connect(sigc::mem_fun(*this, &FlightDeckWindow::euops1_change_ocaoch));
	if (m_euops1comboboxocaochunit)
		m_euops1ocaochunitchange = m_euops1comboboxocaochunit->signal_changed().connect(sigc::mem_fun(*this, &FlightDeckWindow::euops1_change_ocaochunit));
	if (m_euops1buttonsetda)
		m_euops1buttonsetda->signal_clicked().connect(sigc::mem_fun(*this, &FlightDeckWindow::euops1_setminimum));
	if (m_euops1comboboxlight)
		m_euops1comboboxlight->signal_changed().connect(sigc::mem_fun(*this, &FlightDeckWindow::euops1_recomputeldg));
	if (m_euops1comboboxmetvis)
		m_euops1comboboxmetvis->signal_changed().connect(sigc::mem_fun(*this, &FlightDeckWindow::euops1_recomputeldg));
	if (m_euops1checkbuttonrwyedgelight)
		m_euops1checkbuttonrwyedgelight->signal_toggled().connect(sigc::mem_fun(*this, &FlightDeckWindow::euops1_recomputetkoff));
	if (m_euops1checkbuttonrwycentreline)
		m_euops1checkbuttonrwycentreline->signal_toggled().connect(sigc::mem_fun(*this, &FlightDeckWindow::euops1_recomputetkoff));
	if (m_euops1checkbuttonmultiplervr)
		m_euops1checkbuttonmultiplervr->signal_toggled().connect(sigc::mem_fun(*this, &FlightDeckWindow::euops1_recomputetkoff));
	euops1_change_tdzelev();
	euops1_change_ocaoch();
	// Log Page
	m_logstore = Gtk::ListStore::create(m_logcolumns);
	m_logfilter = Gtk::TreeModelFilter::create(m_logstore);
	m_logfilter->set_visible_func(sigc::mem_fun(*this, &FlightDeckWindow::log_filter_func));
	if (m_logtreeview) {
		m_logtreeview->set_model(m_logfilter);
		Gtk::CellRendererText *time_renderer = Gtk::manage(new Gtk::CellRendererText());
		int time_col = m_logtreeview->append_column(_("Time"), *time_renderer) - 1;
		Gtk::TreeViewColumn *time_column = m_logtreeview->get_column(time_col);
                if (time_column) {
			time_column->set_cell_data_func(*time_renderer, sigc::bind(sigc::mem_fun(*this, &FlightDeckWindow::log_cell_data_func), m_logcolumns.m_time.index()));
                        time_column->add_attribute(*time_renderer, "foreground-gdk", m_logcolumns.m_color);
		}
		Gtk::CellRendererText *text_renderer = Gtk::manage(new Gtk::CellRendererText());
		int text_col = m_logtreeview->append_column(_("Text"), *text_renderer) - 1;
		Gtk::TreeViewColumn *text_column = m_logtreeview->get_column(text_col);
                if (text_column) {
                        text_column->add_attribute(*text_renderer, "text", m_logcolumns.m_text);
                        text_column->add_attribute(*text_renderer, "foreground-gdk", m_logcolumns.m_color);
		}
		const Gtk::TreeModelColumnBase *cols[] = { &m_logcolumns.m_time,
							   &m_logcolumns.m_text };
		for (unsigned int i = 0; i < sizeof(cols)/sizeof(cols[0]); ++i) {
			Gtk::TreeViewColumn *col = m_logtreeview->get_column(i);
			if (!col)
				continue;
			col->set_resizable(true);
			//col->set_sort_column(*cols[i]);
			col->set_expand(i == 1);
		}
		m_logtreeview->columns_autosize();
	}
	// GRIB2 Page
	m_grib2layerstore = Gtk::TreeStore::create(m_grib2layercolumns);
	if (m_grib2layertreeview) {
		m_grib2layertreeview->set_model(m_grib2layerstore);
		Gtk::CellRendererText *text_renderer = Gtk::manage(new Gtk::CellRendererText());
		int text_col = m_grib2layertreeview->append_column(_("Text"), *text_renderer) - 1;
		Gtk::TreeViewColumn *text_column = m_grib2layertreeview->get_column(text_col);
                if (text_column) {
                        text_column->add_attribute(*text_renderer, "text", m_grib2layercolumns.m_text);
 		}
		const Gtk::TreeModelColumnBase *cols[] = { &m_grib2layercolumns.m_text };
		for (unsigned int i = 0; i < sizeof(cols)/sizeof(cols[0]); ++i) {
			Gtk::TreeViewColumn *col = m_grib2layertreeview->get_column(i);
			if (!col)
				continue;
			col->set_resizable(true);
			col->set_sort_column(*cols[i]);
			col->set_expand(i == 0);
		}
		m_grib2layertreeview->columns_autosize();
		Glib::RefPtr<Gtk::TreeSelection> selection = m_grib2layertreeview->get_selection();
		selection->set_mode(Gtk::SELECTION_SINGLE);
		m_grib2layerselchangeconn = selection->signal_changed().connect(sigc::hide_return(sigc::mem_fun(*this, &FlightDeckWindow::grib2layer_displaysel)));
	}
	grib2layer_displaysel();
	grib2layer_hide();

	//set_hide_titlebar_when_maximized(true);
	set_menu_id(0);
}

FlightDeckWindow::~FlightDeckWindow()
{
	wptpage_clearall();
	m_sensorschange.disconnect();
	sensorcfg_fini();
#ifdef HAVE_EVINCE
	if (m_docevdocmodel) {
		g_signal_handler_disconnect(m_docevdocmodel, m_docevdocmodel_sighandler);
 		g_object_unref(m_docevdocmodel);
	}
	m_docevdocmodel = 0;
	m_docevdocmodel_sighandler = 0;
	ev_shutdown();
#endif
}

void FlightDeckWindow::set_engine(Engine& eng)
{
	m_engine = &eng;
	m_navarea.set_engine(*m_engine);
	if (m_wptdrawingarea)
		m_wptdrawingarea->set_engine(*m_engine);
	if (m_fpldrawingarea)
		m_fpldrawingarea->set_engine(*m_engine);
	if (m_fplgpsimpdrawingarea)
		m_fplgpsimpdrawingarea->set_engine(*m_engine);
	if (m_grib2drawingarea)
		m_grib2drawingarea->set_engine(*m_engine);
	if (m_coorddialog)
		m_coorddialog->set_engine(*m_engine);
	if (m_suspenddialog)
		m_suspenddialog->set_engine(*m_engine);
}

void FlightDeckWindow::set_sensors(Sensors& sensors)
{
	m_sensorschange.disconnect();
	sensorcfg_fini();
	m_sensors = &sensors;
	m_navarea.set_sensors(*m_sensors);
	if (m_perfdrawingareawb)
		m_perfdrawingareawb->set_wb(&m_sensors->get_aircraft().get_wb());
	if (m_coorddialog)
		m_coorddialog->set_sensors(m_sensors);
	if (m_suspenddialog)
		m_suspenddialog->set_sensors(m_sensors);
	if (m_sensors) {
		m_sensorschange = m_sensors->connect_change(sigc::mem_fun(*this, &FlightDeckWindow::sensors_change));
		m_sensors->signal_logmessage().connect(sigc::mem_fun(*this, &FlightDeckWindow::log_add_message));
	}
	sensors_change(Sensors::change_all);
	// read some config settings
	if (m_sensors) {
		if (!m_sensors->get_configfile().has_group(cfggroup_mainwindow) ||
		    !m_sensors->get_configfile().has_key(cfggroup_mainwindow, cfgkey_mainwindow_fullscreen))
			m_sensors->get_configfile().set_integer(cfggroup_mainwindow, cfgkey_mainwindow_fullscreen, 1);
		if (m_sensors->get_configfile().get_integer(cfggroup_mainwindow, cfgkey_mainwindow_fullscreen))
			fullscreen();
		if (!m_sensors->get_configfile().has_group(cfggroup_mainwindow) ||
		    !m_sensors->get_configfile().has_key(cfggroup_mainwindow, cfgkey_mainwindow_onscreenkeyboard))
			m_sensors->get_configfile().set_integer(cfggroup_mainwindow, cfgkey_mainwindow_onscreenkeyboard, m_onscreenkeyboard);
		m_onscreenkeyboard = !!m_sensors->get_configfile().get_integer(cfggroup_mainwindow, cfgkey_mainwindow_onscreenkeyboard);
		m_sensors->save_config();
		if (m_wptdrawingarea) {
			m_wptdrawingarea->set_route(m_sensors->get_route());
			m_wptdrawingarea->set_track(&m_sensors->get_track());
		}
		if (m_fpldrawingarea) {
			m_fpldrawingarea->set_route(m_sensors->get_route());
			m_fpldrawingarea->set_track(&m_sensors->get_track());
		}
		if (m_fplgpsimpdrawingarea)
			m_fplgpsimpdrawingarea->set_track(&m_sensors->get_track());
		if (m_grib2drawingarea) {
			m_grib2drawingarea->set_route(m_sensors->get_route());
			m_grib2drawingarea->set_track(&m_sensors->get_track());
		}
		if (m_vertprofiledrawingarea) {
			m_vertprofiledrawingarea->set_engine(*m_sensors->get_engine());
			m_vertprofiledrawingarea->set_route(m_sensors->get_route());
		}
		perfpage_restoreperf();
	}
}

void FlightDeckWindow::sensors_change(Sensors::change_t changemask)
{
	if (!m_sensors || !changemask)
		return;
	m_navarea.sensors_change(m_sensors, changemask);
	if (m_fplatmodialog)
		m_fplatmodialog->sensors_change(changemask);
	if (m_fplverifdialog)
		m_fplverifdialog->sensors_change(changemask);
	if (changemask & Sensors::change_position) {
		if (m_menuid >= 100 && m_menuid < 200) {
			Point pt;
			m_sensors->get_position(pt);
			wptpage_updatepos(pt);
		}
		if (m_menuid == 201)
			fpldirpage_updatepos();
		if (m_coorddialog)
			m_coorddialog->coord_updatepos();
	}
	if (changemask & Sensors::change_fplan) {
		if (m_menuid == 200 || m_menuid == 2000 || m_menuid == 2001)
			fplpage_updatefpl();
		if (m_vertprofiledrawingarea)
			m_vertprofiledrawingarea->route_change();
	}
	if (changemask & Sensors::change_navigation) {
		if (m_fplbrg2togglebutton) {
			m_fplbrg2conn.block();
			m_fplbrg2togglebutton->set_active(m_sensors->nav_is_brg2_enabled());
			m_fplbrg2conn.unblock();
		}
		if (m_wpttogglebuttonbrg2) {
			m_wptbrg2conn.block();
			m_wpttogglebuttonbrg2->set_active(m_sensors->nav_is_brg2_enabled());
			m_wptbrg2conn.unblock();
		}
		if (m_menuid < 100) {
			m_softkeys.block(7);
			set_menu_button(7, m_sensors->nav_is_finalapproach() ? "FASNAV" :
					m_sensors->nav_is_directto() ? "DCTNAV" : "FPLNAV");
			m_softkeys[7].set_active(m_sensors->nav_is_on());
			m_softkeys.unblock(7);
			m_softkeys.block(8);
			m_softkeys[8].set_active(m_sensors->nav_is_hold());
			m_softkeys.unblock(8);
			m_softkeys.block(9);
			m_softkeys[9].set_active(m_sensors->nav_is_brg2_enabled());
			m_softkeys.unblock(9);
		} else if (m_menuid == 2000) {
			m_softkeys.block(9);
			m_softkeys[9].set_active(m_sensors->nav_is_brg2_enabled());
			m_softkeys.unblock(9);
		}
	}
}

bool FlightDeckWindow::on_delete_event(GdkEventAny* event)
{
	m_sensorschange.disconnect();
	sensorcfg_fini();
        hide();
        return true;
}

void FlightDeckWindow::on_show(void)
{
#ifdef GLIBMM_DEFAULT_SIGNAL_HANDLERS_ENABLED
	FullscreenableWindow::on_show();
#endif //GLIBMM_DEFAULT_SIGNAL_HANDLERS_ENABLED
	disable_screensaver();
	m_screensaverheartbeat.disconnect();
#ifdef HAVE_X11_RSS
	m_screensaverheartbeat = Glib::signal_timeout().connect_seconds(sigc::mem_fun(*this, &FlightDeckWindow::screensaver_heartbeat), 30);
#endif
	if (true) {
		Gdk::Geometry geom;
		geom.min_width = 1024;
		geom.min_height = 700;
		set_geometry_hints(*this, geom, Gdk::HINT_MIN_SIZE);
	}
}

void FlightDeckWindow::on_hide(void)
{
#ifdef GLIBMM_DEFAULT_SIGNAL_HANDLERS_ENABLED
	FullscreenableWindow::on_hide();
#endif //GLIBMM_DEFAULT_SIGNAL_HANDLERS_ENABLED
	m_screensaverheartbeat.disconnect();
	enable_screensaver();
}

bool FlightDeckWindow::screensaver_heartbeat(void)
{
#if defined(HAVE_X11_RSS)
	Glib::RefPtr<Gdk::Window> wnd(get_window());
	if (!wnd)
		return true;
	Glib::RefPtr<Gdk::Display> dpy(wnd->get_display());
	if (!dpy)
		return true;
	Display *xdpy(gdk_x11_display_get_xdisplay(dpy->gobj()));
	if (!xdpy)
		return true;
	XResetScreenSaver(xdpy);
	return true;
#elif defined(HAVE_WINDOWS_H)
	SetThreadExecutionState(ES_CONTINUOUS | ES_DISPLAY_REQUIRED | ES_SYSTEM_REQUIRED);
	return true;
#else
	return false;
#endif
}

void FlightDeckWindow::disable_screensaver(void)
{
	if (m_screensaver != screensaver_default)
		return;
	m_screensaver = screensaver_unknown;
	if (suspend_xscreensaver(true)) {
		m_screensaver = screensaver_xss;
		if (m_sensors)
			m_sensors->log(Sensors::loglevel_notice, "screensaver: disable using XSS");
		return;
	}
	{
		int r(suspend_dpms(true));
		if (r) {
			m_screensaver = (r >= 2) ? screensaver_dpms : screensaver_dpmsoff;
			if (m_sensors)
				m_sensors->log(Sensors::loglevel_notice, (r >= 2) ? "screensaver: disable using DPMS" : "screensaver: DPMS reports screensaver off");
			return;
		}
	}
	if (m_sensors)
		m_sensors->log(Sensors::loglevel_notice, "screensaver: unable to disable screensaver");
}

void FlightDeckWindow::enable_screensaver(void)
{
	switch (m_screensaver) {
	case screensaver_xss:
		suspend_xscreensaver(false);
		if (m_sensors)
			m_sensors->log(Sensors::loglevel_notice, "screensaver: reenable using XSS");
		break;

	case screensaver_dpms:
		suspend_dpms(false);
		if (m_sensors)
			m_sensors->log(Sensors::loglevel_notice, "screensaver: reenable using DPMS");
		break;

	default:
		break;
	}
	m_screensaver = screensaver_default;
}

bool FlightDeckWindow::suspend_xscreensaver(bool ena)
{
#ifdef HAVE_X11_XSS
	Glib::RefPtr<Gdk::Window> wnd(get_window());
	if (!wnd)
		return false;
	Glib::RefPtr<Gdk::Display> dpy(wnd->get_display());
	if (!dpy)
		return false;
	Display *xdpy(gdk_x11_display_get_xdisplay(dpy->gobj()));
	if (!xdpy)
		return false;
	int event, error, major, minor;
	if (!XScreenSaverQueryExtension(xdpy, &event, &error) ||
	    !XScreenSaverQueryVersion(xdpy, &major, &minor))
		return false;
	if (major < 1 || (major == 1 && minor < 1))
		return false;
	XScreenSaverSuspend(xdpy, ena);
	return true;
#else
	return false;
#endif
}

int FlightDeckWindow::suspend_dpms(bool ena)
{
#if defined(CONFIG_X11_DPMS)
	Glib::RefPtr<Gdk::Window> wnd(get_window());
	if (!wnd)
		return 0;
	Glib::RefPtr<Gdk::Display> dpy(wnd->get_display());
	if (!dpy)
		return 0;
	Display *xdpy(gdk_x11_display_get_xdisplay(dpy->gobj()));
	if (!xdpy)
		return 0;
	{
		int nothing;
		if (!DPMSQueryExtension(xdpy, &nothing, &nothing))
			return 0;
	}
	if (ena) {
		BOOL onoff;
		CARD16 state;
		DPMSInfo(xdpy, &state, &onoff);
		if (!onoff)
			return 1;
		Status stat;
		stat = DPMSDisable(xdpy);
		if (m_sensors) {
			std::ostringstream oss;
			oss << "screensaver: DPMSDisable status " << stat;
			m_sensors->log(Sensors::loglevel_notice, oss.str());
		}
		return 2;
	}
	if (!DPMSEnable(xdpy)) {
		if (m_sensors)
			m_sensors->log(Sensors::loglevel_notice, "screensaver: failure to reenable DPMS");
		return 0;
	}
	{
                BOOL onoff;
                CARD16 state;

                DPMSForceLevel(xdpy, DPMSModeOn);
                DPMSInfo(xdpy, &state, &onoff);
                if (!onoff) {
			if (m_sensors)
				m_sensors->log(Sensors::loglevel_notice, "screensaver: failure to reenable DPMS");
			return 1;
		}
	}
	return 2;
#elif defined(HAVE_WINDOWS_H)
	if (ena) {
		/* disable screensaver by temporarily changing system settings */
		BOOL scrsaveactive;
		SystemParametersInfo(SPI_GETSCREENSAVEACTIVE, 0, &scrsaveactive, 0);
		if (LOWORD(GetVersion()) == 0x0005) {
			/* If this is NT 5.0 (i.e., Win2K), we need to hack around
			 * KB318781 (see http://support.microsoft.com/kb/318781) */
			HKEY hKeyCP = NULL;
			if (ERROR_SUCCESS == RegOpenKeyEx(HKEY_CURRENT_USER, TEXT("Control Panel\\Desktop"), 0, KEY_QUERY_VALUE, &hKeyCP) &&
			    ERROR_SUCCESS != RegQueryValueEx(hKeyCP, TEXT("SCRNSAVE.EXE"), NULL, NULL, NULL, NULL))
				scrsaveactive = FALSE;
		}
		if (!scrsaveactive)
			return 1;
		SystemParametersInfo(SPI_SETSCREENSAVEACTIVE, 0, NULL, 0);
		return 2;
        }
	/* restore screensaver system settings */
        SystemParametersInfo(SPI_SETSCREENSAVEACTIVE, TRUE, NULL, 0);
   	return 2;	
#else
	return 0;
#endif
}

void FlightDeckWindow::menubutton_clicked(unsigned int nr)
{
	std::cout << "Button " << nr << " clicked" << std::endl;

	if (m_menuid >= 1000 && m_menuid < 1100) {
		for (unsigned int i = 0; i < 12; ++i) {
			m_softkeys.block(i);
			m_softkeys[i].set_active(false);
			m_softkeys.unblock(i);
		}
		switch (nr) {
		case 0:
			m_navarea.map_zoom_in();
			return;

		case 1:
			m_navarea.map_zoom_out();
			return;

		case 2:
			m_navarea.hsi_zoom_in();
			return;

		case 3:
			m_navarea.hsi_zoom_out();
			return;

		case 4:
			map_dialog_open();
			return;

		case 5:
			alt_dialog_open();
			return;

		case 6:
			hsi_dialog_open();
			return;

		case 7:
			m_navarea.map_set_pan(!m_navarea.map_get_pan());
			m_softkeys.block(7);
			m_softkeys[7].set_active(m_navarea.map_get_pan());
			m_softkeys.unblock(7);
			return;

		case 8:
		{
			MapDrawingArea& map(m_navarea.get_map_drawing_area());
			NavAIDrawingArea& ai(m_navarea.get_ai_drawing_area());
			NavHSIDrawingArea& hsi(m_navarea.get_hsi_drawing_area());
			NavAltDrawingArea& alt(m_navarea.get_alt_drawing_area());
			return;
		}

		case 9:
			m_softkeys.block(9);
			if (m_sensors) {
				int fs(!m_sensors->get_configfile().get_integer(cfggroup_mainwindow, cfgkey_mainwindow_fullscreen));
				m_sensors->get_configfile().set_integer(cfggroup_mainwindow, cfgkey_mainwindow_fullscreen, fs);
				m_sensors->save_config();
				if (fs)
					fullscreen();
				else
					unfullscreen();
				set_menu_button(9, fs ? "FULLSCRN" : "WINDOW");
			}
			m_softkeys[9].set_active(false);
			m_softkeys.unblock(9);
			return;

		case 10:
			hide();
			return;

		case 11:
			set_menu_id(m_menuid - 1000);
			return;

		default:
			return;
		}
	} else if (m_menuid == 2000) {
		for (unsigned int i = 0; i < 12; ++i) {
			m_softkeys.block(i);
			m_softkeys[i].set_active(false);
			m_softkeys.unblock(i);
		}
		switch (nr) {
		case 0: // +WPT
			fplpage_insert_clicked();
			return;

		case 1: // +TXT
			fplpage_inserttxt_clicked();
			return;

		case 2: // DUP
			fplpage_duplicate_clicked();
			return;

		case 3: // MV:UP
			fplpage_moveup_clicked();
			return;

		case 4: // MV:DWN
			fplpage_movedown_clicked();
			return;

		case 5: // -WPT
			fplpage_delete_clicked();
			return;

		case 6: // STRAIGHT
			fplpage_straighten_clicked();
			return;

		case 8: // DCT TO
			fplpage_directto_clicked();
			return;

		case 9: // BRG 2
			fplpage_brg2_clicked();
			return;

		case 10:
			set_menu_id(2001);
			return;

		case 11:
			set_menu_id(200);
			return;

		default:
			return;
		}
	} else if (m_menuid == 2001) {
		for (unsigned int i = 0; i < 12; ++i) {
			m_softkeys.block(i);
			m_softkeys[i].set_active(false);
			m_softkeys.unblock(i);
		}
		switch (nr) {
		case 0:
			fplatmosphere_dialog_open();
			return;

		case 1:
			set_menu_id(2005);
			return;

		case 2:
			fplpage_fetchprofilegraph_clicked();
			return;

		case 3:
			set_menu_id(2004);
			return;

		case 4:
			grib2layer_update();
			set_menu_id(2006);
			return;

		case 5:
			maptile_dialog_open();
			return;

		case 6:
		{
			std::string templatefile(Glib::build_filename(FPlan::get_userdbpath(), "navlogtemplates", "navlog.ods"));
			bool tfok(Glib::file_test(templatefile, Glib::FILE_TEST_EXISTS) &&
				  !Glib::file_test(templatefile, Glib::FILE_TEST_IS_DIR));
			if (!tfok) {
				templatefile = Glib::build_filename(Engine::get_default_aux_dir(), "navlogtemplates", "navlog.ods");
				tfok = Glib::file_test(templatefile, Glib::FILE_TEST_EXISTS) &&
					!Glib::file_test(templatefile, Glib::FILE_TEST_IS_DIR);
			}
			if (tfok && m_sensors) {
				const Aircraft& acft(m_sensors->get_aircraft());
				acft.navfplan(templatefile, *m_sensors->get_engine(), m_sensors->get_route(),
					      m_sensors->get_acft_cruise_params(), perfpage_getwbvalues());
			}
			return;
 		}

		case 7:
			set_menu_id(2003);
			if (m_vertprofiledrawingarea)
				m_vertprofiledrawingarea->route_change();
			return;

		case 8:
			if (m_fplverifdialog)
				m_fplverifdialog->open(m_sensors);
			return;

		case 9:
			if (m_fplautoroutedialog)
			        m_fplautoroutedialog->open(m_sensors);
			return;

		case 10:
			set_menu_id(200);
			return;

		case 11:
			set_menu_id(2000);
			return;

		default:
			return;
		}
	} else if (m_menuid == 2002) {
		for (unsigned int i = 0; i < 12; ++i) {
			m_softkeys.block(i);
			m_softkeys[i].set_active(false);
			m_softkeys.unblock(i);
		}
		switch (nr) {
		case 0:
			if (wxchartlist_next())
				wxchartlist_displaysel();
			return;

		case 1:
			if (wxchartlist_prev())
				wxchartlist_displaysel();
			return;

		case 2:
			if (wxchartlist_nextepoch())
				wxchartlist_displaysel();
			return;

		case 3:
			if (wxchartlist_prevepoch())
				wxchartlist_displaysel();
			return;

		case 4:
			if (wxchartlist_nextcreatetime())
				wxchartlist_displaysel();
			return;

		case 5:
			if (wxchartlist_prevcreatetime())
				wxchartlist_displaysel();
			return;

		case 6:
			if (wxchartlist_nextlevel())
				wxchartlist_displaysel();
			return;

		case 7:
			if (wxchartlist_prevlevel())
				wxchartlist_displaysel();
			return;

		case 10:
			set_menu_id(200);
			return;

		case 11:
			set_menu_id(2005);
			return;

		default:
			return;
		}
	} else if (m_menuid == 2003) {
		for (unsigned int i = 0; i < 12; ++i) {
			m_softkeys.block(i);
			m_softkeys[i].set_active(false);
			m_softkeys.unblock(i);
		}
		switch (nr) {
		case 0:
			if (m_vertprofiledrawingarea) {
				if (m_vertprofiledrawingarea->is_chart())
					m_vertprofiledrawingarea->latlon_zoom_in();
				else
					m_vertprofiledrawingarea->dist_zoom_in();
			}
			return;

		case 1:
			if (m_vertprofiledrawingarea) {
				if (m_vertprofiledrawingarea->is_chart())
					m_vertprofiledrawingarea->latlon_zoom_out();
				else
					m_vertprofiledrawingarea->dist_zoom_out();
			}
			return;

		case 2:
			if (m_vertprofiledrawingarea) {
				if (m_vertprofiledrawingarea->is_chart())
					m_vertprofiledrawingarea->next_chart();
				else
					m_vertprofiledrawingarea->elev_zoom_in();
			}
			return;

		case 3:
			if (m_vertprofiledrawingarea) {
				if (m_vertprofiledrawingarea->is_chart())
					m_vertprofiledrawingarea->prev_chart();
				else
					m_vertprofiledrawingarea->elev_zoom_out();
			}
			return;

		case 4:
			if (m_vertprofiledrawingarea)
				m_vertprofiledrawingarea->toggle_chart();
			set_menu_id(2003);
			return;

		case 5:
			if (m_vertprofiledrawingarea->is_chart())
				return;
			if (m_vertprofiledrawingarea)
				m_vertprofiledrawingarea->toggle_ymode();
			set_menu_id(2003);
			return;

		case 9:
			m_wxchartexportfilechooser.show();
			return;

		case 10:
			set_menu_id(200);
			return;

		case 11:
			set_menu_id(2001);
			return;

		default:
			return;
		}
	} else if (m_menuid == 2004) {
		for (unsigned int i = 0; i < 12; ++i) {
			m_softkeys.block(i);
			m_softkeys[i].set_active(false);
			m_softkeys.unblock(i);
		}
		switch (nr) {
		case 0:
			metartaf_fetch();
			return;

		case 10:
			set_menu_id(200);
			return;

		case 11:
			set_menu_id(2001);
			return;

		default:
			return;
		}
	} else if (m_menuid == 2005) {
		for (unsigned int i = 0; i < 12; ++i) {
			m_softkeys.block(i);
			m_softkeys[i].set_active(false);
			m_softkeys.unblock(i);
		}
		switch (nr) {
		case 0:
			wxchartlist_opendlg();
			return;

		case 1:
			wxchartlist_displaysel();
			return;

		case 2:
			wxchartlist_refetch();
			return;

		case 10:
			set_menu_id(200);
			return;

		case 11:
			set_menu_id(2001);
			return;

		default:
			return;
		}
	} else if (m_menuid == 2006) {
		for (unsigned int i = 0; i < 12; ++i) {
			m_softkeys.block(i);
			m_softkeys[i].set_active(false);
			m_softkeys.unblock(i);
		}
		switch (nr) {
		case 0:
			grib2layer_zoomin();
			return;

		case 1:
			grib2layer_zoomout();
			return;

		case 2:
			if (grib2layer_next())
				grib2layer_displaysel();
			return;

		case 3:
			if (grib2layer_prev())
				grib2layer_displaysel();
			return;

		case 4:
			grib2layer_togglemap();
			return;

		case 10:
			grib2layer_hide();
			set_menu_id(200);
			return;

		case 11:
			grib2layer_hide();
			set_menu_id(2001);
			return;

		default:
			return;
		}
	}
	if (nr < 5) {
		if (nr == 0) {
			if (m_menuid >= 100)
				m_navarea.map_set_first_renderer();
			else
				m_navarea.map_set_next_renderer();
			set_menu_id(m_navarea.map_get_renderer().first);
		} else if (m_menuid >= 100 * nr && m_menuid < 100 * (nr + 1))
			set_menu_id(m_menuid + 1);
		else if (nr == 2 && m_sensors && !m_sensors->get_route().get_nrwpt())
			set_menu_id(201);
#ifdef HAVE_EVINCE
		else if (nr == 3 && docdir_display_document())
		        set_menu_id(301);
#else
		else if (nr == 3)
			set_menu_id(302);
#endif
		else
			set_menu_id(nr * 100);
	}

	if (m_menuid < 100) {
		switch (nr) {
		case 6:
		{
			NavInfoDrawingArea& info(m_navarea.get_info_drawing_area());
			if (info.is_timer_running()) {
				info.stop_timer();
			} else {
				info.clear_timer();
				info.start_timer();
			}
			m_softkeys.block(6);
			m_softkeys[6].set_active(info.is_timer_running());
			m_softkeys.unblock(6);
			break;
		}

		case 7:
			m_softkeys.block(7);
			m_softkeys[7].set_active(m_sensors && m_sensors->nav_is_on());
			m_softkeys.unblock(7);
			if (!m_sensors)
				break;
			if (m_sensors->nav_is_on())
				m_sensors->nav_off();
			else
				m_sensors->nav_nearestleg();
			break;

		case 8:
			m_softkeys.block(8);
			m_softkeys[8].set_active(m_sensors && m_sensors->nav_is_hold());
			m_softkeys.unblock(8);
			if (!m_sensors)
				break;
			if (m_sensors->nav_is_hold()) {
				m_sensors->nav_set_hold(false);
			} else if (m_sensors->nav_is_on() && m_suspenddialog) {
				m_suspenddialog->set_flags(m_navarea.map_get_drawflags());
				if (m_sensors->get_configfile().has_key(cfggroup_mainwindow, NavArea::cfgkey_mainwindow_mapscale) &&
				    m_sensors->get_configfile().has_key(cfggroup_mainwindow, NavArea::cfgkey_mainwindow_arptmapscale))
					m_suspenddialog->set_map_scale(m_sensors->get_configfile().get_double(cfggroup_mainwindow, NavArea::cfgkey_mainwindow_mapscale),
								       m_sensors->get_configfile().get_double(cfggroup_mainwindow, NavArea::cfgkey_mainwindow_arptmapscale));
				m_suspenddialog->show();
			}
			break;

		case 9:
			m_softkeys.block(9);
			m_softkeys[9].set_active(m_sensors && m_sensors->nav_is_brg2_enabled());
			m_softkeys.unblock(9);
			if (!m_sensors)
				break;
			m_sensors->nav_set_brg2(!m_sensors->nav_is_brg2_enabled());
			break;

		case 11:
			set_menu_id(1000 + m_menuid);
			break;

		default:
			break;
		}
	} else if (m_menuid < 200) {
		switch (nr) {
		case 8:
			if (m_wptdrawingarea)
				m_wptdrawingarea->zoom_in();
			break;

		case 9:
			if (m_wptdrawingarea)
				m_wptdrawingarea->zoom_out();
			break;

		default:
			break;
		}
		for (unsigned int i = 5; i < 12; ++i) {
			m_softkeys.block(i);
			m_softkeys[i].set_active(false);
			m_softkeys.unblock(i);
		}
	} else if (m_menuid == 200) {
		switch (nr) {
		case 7:
			m_softkeys.block(7);
			m_softkeys[7].set_active(false);
			m_softkeys.unblock(7);
			if (m_fpldrawingarea)
				m_fpldrawingarea->zoom_in();
			break;

		case 8:
			m_softkeys.block(8);
			m_softkeys[8].set_active(false);
			m_softkeys.unblock(8);
		        if (m_fpldrawingarea)
				m_fpldrawingarea->zoom_out();
			break;

		case 11:
			m_softkeys.block(11);
			m_softkeys[11].set_active(false);
			m_softkeys.unblock(11);
			set_menu_id(2000);
			break;

		default:
			break;
		}
	} else if (m_menuid >= 202 && m_menuid < 300) {
		switch (nr) {
		case 6:
			if (m_sensors) {
				if (m_sensors->nav_is_brg2_enabled())
					m_sensors->nav_set_brg2(false);
				else
					fplgpsimppage_brg2();
			}
			m_softkeys.block(6);
			m_softkeys[6].set_active(m_sensors && m_sensors->nav_is_brg2_enabled());
			m_softkeys.unblock(6);
			break;

		case 7:
			if (m_sensors) {
				if (m_sensors->nav_is_on())
					m_sensors->nav_off();
				else
					fplgpsimppage_directto();
			}
			m_softkeys.block(7);
			m_softkeys[7].set_active(m_sensors && m_sensors->nav_is_on());
			m_softkeys.unblock(7);
			break;

		case 8:
			m_softkeys.block(8);
			m_softkeys[8].set_active(false);
			m_softkeys.unblock(8);
			fplgpsimppage_zoomin();
			break;

		case 9:
			m_softkeys.block(9);
			m_softkeys[9].set_active(false);
			m_softkeys.unblock(9);
			fplgpsimppage_zoomout();
			break;

		case 10:
			m_softkeys.block(10);
			m_softkeys[10].set_active(false);
			m_softkeys.unblock(10);
			if (fplgpsimppage_loadfpl())
				set_menu_id(200);
			break;

		default:
			break;
		}
#ifdef HAVE_EVINCE
	} else if (m_menuid == 301) {
		switch (nr) {
		case 5:
			m_softkeys.block(5);
			m_softkeys[5].set_active(false);
			m_softkeys.unblock(5);
			if (!m_docevdocmodel)
				break;
			ev_document_model_set_sizing_mode(m_docevdocmodel,
							  ev_document_model_get_sizing_mode(m_docevdocmodel) == EV_SIZING_FIT_WIDTH
							  ? EV_SIZING_BEST_FIT : EV_SIZING_FIT_WIDTH);
			docdir_save_view();
			break;

		case 6:
		{
			m_softkeys.block(6);
			m_softkeys[6].set_active(false);
			m_softkeys.unblock(6);
			if (!m_docevdocmodel || !m_docevview)
				break;
			ev_document_model_set_sizing_mode(m_docevdocmodel, EV_SIZING_FREE);
			EvView *view((EvView *)m_docevview->gobj());
			if (!view)
				break;
			ev_view_zoom_in(view);
			docdir_save_view();
			break;
		}

		case 7:
		{
			m_softkeys.block(7);
			m_softkeys[7].set_active(false);
			m_softkeys.unblock(7);
			if (!m_docevdocmodel || !m_docevview)
				break;
			ev_document_model_set_sizing_mode(m_docevdocmodel, EV_SIZING_FREE);
			EvView *view((EvView *)m_docevview->gobj());
			if (!view)
				break;
			ev_view_zoom_out(view);
			docdir_save_view();
			break;
		}

		case 8:
		{
			m_softkeys.block(8);
			m_softkeys[8].set_active(false);
			m_softkeys.unblock(8);
			if (!m_docevdocmodel)
				break;
			gint rot(ev_document_model_get_rotation(m_docevdocmodel));
			rot += 90;
			if (rot >= 360)
				rot = 0;
			ev_document_model_set_rotation(m_docevdocmodel, rot);
			docdir_save_view();
			break;
		}

		case 9:
		{
			m_softkeys.block(9);
			m_softkeys[9].set_active(false);
			m_softkeys.unblock(9);
			docdir_save_view();
			if (!m_docdirtreeview)
				break;
			Glib::RefPtr<Gtk::TreeSelection> selection(m_docdirtreeview->get_selection());
			if (!selection)
				break;
			Gtk::TreeModel::iterator iter(selection->get_selected());
			if (!iter)
				break;
			for (;;) {
				Gtk::TreeModel::Row row(*iter);
				Gtk::TreeIter iter1(row.parent());
				if (!iter1)
					break;
				row = *iter1;
				if (!row[m_docdircolumns.m_document])
					break;
				iter = iter1;
			}
			++iter;
			if (docdir_display_document(iter))
				selection->select(iter);
			break;
		}

		case 10:
		{
			m_softkeys.block(10);
			m_softkeys[10].set_active(false);
			m_softkeys.unblock(10);
			docdir_save_view();
			if (!m_docdirtreeview)
				break;
			Glib::RefPtr<Gtk::TreeSelection> selection(m_docdirtreeview->get_selection());
			if (!selection)
				break;
			Gtk::TreeModel::iterator iter(selection->get_selected());
			if (!iter)
				break;
			for (;;) {
				Gtk::TreeModel::Row row(*iter);
				Gtk::TreeIter iter1(row.parent());
				if (!iter1)
					break;
				row = *iter1;
				if (!row[m_docdircolumns.m_document])
					break;
				iter = iter1;
			}
			// make sure we do not iterate past the beginning
			{
				Gtk::TreeModel::Row row(*iter);
				Gtk::TreeIter iter1(row.parent());
				if (!iter1) {
					if (!m_docdirstore)
						break;
					iter1 = m_docdirstore->children().begin();
				} else {
					row = *iter1;
					iter1 = row.children().begin();
				}
				if (iter == iter1)
					break;
			}
			--iter;
			if (docdir_display_document(iter))
				selection->select(iter);
			break;
		}

		case 11:
			docdir_save_view();
			set_menu_id(300);
			break;

		default:
			break;
		}
#endif
	} else if (m_menuid == 400) {
		switch (nr) {
		case 7:
			if (!m_sensors)
				break;
			if (m_sensors->log_is_open())
				m_sensors->log_close();
			else
				m_sensors->log_open();
			m_softkeys.block(7);
			m_softkeys[7].set_active(m_sensors->log_is_open());
			m_softkeys.unblock(7);
			break;

		case 9:
			m_logdisplaylevel = (Sensors::loglevel_t)(m_logdisplaylevel + 1);
			if (m_logdisplaylevel > Sensors::loglevel_fatal)
				m_logdisplaylevel = Sensors::loglevel_notice;
			if (m_logfilter)
				m_logfilter->refilter();
			m_softkeys.block(9);
			set_menu_button(9, log_level_button[m_logdisplaylevel]);
			m_softkeys[9].set_active(false);
			m_softkeys.unblock(9);
			break;

		default:
			break;
		}
	} else if (m_menuid > 400 && m_menuid < 500) {
		if (nr == 10) {
			m_admin = !m_admin;
			m_softkeys.block(10);
			set_menu_button(10, m_admin ? "ADMIN" : "USER");
			m_softkeys[10].set_active(m_admin);
			m_softkeys.unblock(10);
			sensorcfg_set_admin(m_admin);
		}
	}
	if ((m_menuid > 400 && m_menuid < 500) || m_menuid == 200 || m_menuid == 302) {
		if (nr == 9) {
			m_onscreenkeyboard = !m_onscreenkeyboard;
			m_sensors->get_configfile().set_integer(cfggroup_mainwindow, cfgkey_mainwindow_onscreenkeyboard, m_onscreenkeyboard);
			m_sensors->save_config();
			m_softkeys.block(9);
			m_softkeys[9].set_active(m_onscreenkeyboard);
			m_softkeys.unblock(9);
		}
	}
	if (m_menuid == 200 && nr == 10 && m_fpldrawingarea) {
		NavArea::vmarenderer_t renderer(m_fpldrawingarea->get_renderer(), 0);
		if (m_engine)
			renderer.second = m_engine->get_bitmapmaps_db().find_index(m_fpldrawingarea->get_bitmap());
		renderer = m_navarea.vma_next_renderer(renderer);
		m_softkeys.block(10);
		set_menu_button(10, "MAP:" + to_short_str(renderer.first));
		m_softkeys.unblock(10);
		if (renderer.first != VectorMapArea::renderer_none) {
			m_fpldrawingarea->set_renderer(renderer.first);
			*m_fpldrawingarea = m_navarea.map_get_declutter_drawflags();
			if (renderer.first == VectorMapArea::renderer_bitmap && renderer.second < m_navarea.map_get_bitmapmaps().size()) {
				BitmapMaps::Map::ptr_t m(m_navarea.map_get_bitmapmaps()[renderer.second]);
				m_fpldrawingarea->set_bitmap(m);
				m_fpldrawingarea->set_map_scale(m->get_nmi_per_pixel());
			} else {
				m_fpldrawingarea->set_bitmap();
				if (m_sensors && m_sensors->get_configfile().has_group(cfggroup_mainwindow)) {
					const char *key(NavArea::cfgkey_mainwindow_mapscales[renderer.first == VectorMapArea::renderer_airportdiagram]);
					if (m_sensors->get_configfile().has_key(cfggroup_mainwindow, key))
						m_fpldrawingarea->set_map_scale(m_sensors->get_configfile().get_double(cfggroup_mainwindow, key));
				}
			}
		}
	}
	if (m_menuid >= 100 && m_menuid < 200  && nr == 10 && m_wptdrawingarea) {
		NavArea::vmarenderer_t renderer(m_wptdrawingarea->get_renderer(), 0);
		if (m_engine)
			renderer.second = m_engine->get_bitmapmaps_db().find_index(m_wptdrawingarea->get_bitmap());
		renderer = m_navarea.vma_next_renderer(renderer);
		m_softkeys.block(10);
		set_menu_button(10, "MAP:" + to_short_str(renderer.first));
		m_softkeys.unblock(10);
		if (renderer.first != VectorMapArea::renderer_none) {
			m_wptdrawingarea->set_renderer(renderer.first);
			*m_wptdrawingarea = m_navarea.map_get_declutter_drawflags();
			if (renderer.first == VectorMapArea::renderer_bitmap && renderer.second < m_navarea.map_get_bitmapmaps().size()) {
				BitmapMaps::Map::ptr_t m(m_navarea.map_get_bitmapmaps()[renderer.second]);
				m_wptdrawingarea->set_bitmap(m);
				m_wptdrawingarea->set_map_scale(m->get_nmi_per_pixel());
			} else {
				m_wptdrawingarea->set_bitmap();
				if (m_sensors && m_sensors->get_configfile().has_group(cfggroup_mainwindow)) {
					const char *key(NavArea::cfgkey_mainwindow_mapscales[renderer.first == VectorMapArea::renderer_airportdiagram]);
					if (m_sensors->get_configfile().has_key(cfggroup_mainwindow, key))
						m_wptdrawingarea->set_map_scale(m_sensors->get_configfile().get_double(cfggroup_mainwindow, key));
				}
			}
		}
	}
	if (m_menuid <= 300 && nr == 5) {
		m_navarea.map_next_declutter();
		m_softkeys.block(5);
		set_menu_button(5, m_navarea.map_get_declutter_name());
		m_softkeys[5].set_active(false);
		m_softkeys.unblock(5);
		if (m_wptdrawingarea)
			*m_wptdrawingarea = m_navarea.map_get_declutter_drawflags();
		if (m_fpldrawingarea)
			*m_fpldrawingarea = m_navarea.map_get_declutter_drawflags();
	}
}

void FlightDeckWindow::set_mainnotebook_page(unsigned int pg)
{
	if (!m_mainnotebook)
		return;
	for (int n = 0; n < m_mainnotebook->get_n_pages(); ++n) {
		Gtk::Widget *w(m_mainnotebook->get_nth_page(n));
		w->set_visible(n == pg);
	}
	m_mainnotebook->set_current_page(pg);
}

void FlightDeckWindow::set_menu_id(unsigned int id)
{
	// id:
	//   0- 99: NAV
	// 100-199: WPT
	// 200-299: FPL
	// 300-399: AUX
	// 400-499: SENS
	if (true)
		std::cerr << "set_menu_id: " << id << std::endl;
	sensorcfg_fini();
	for (int i = 0; i < 12; ++i)
		m_softkeys.block(i);
	bool menuid_changed(m_menuid != id);
	m_menuid = id;

	set_menu_button(5, "");
	set_menu_button(6, "");
	set_menu_button(7, "");
	set_menu_button(8, "");
	set_menu_button(9, "");
	set_menu_button(10, "");
	set_menu_button(11, "");
	for (unsigned int i = 5; i < 12; ++i)
		m_softkeys[i].set_active(false);

	if (m_menuid < 100) {
		if (m_menuid >= MapDrawingArea::renderer_none) {
			m_navarea.map_set_first_renderer();
			m_menuid = m_navarea.map_get_renderer().first;
			menuid_changed = true;
		}
		set_menu_button(0, "NAV:" + to_short_str((VectorMapArea::renderer_t)m_menuid));
		set_menu_button(1, "WPT");
		set_menu_button(2, "FPL");
		set_menu_button(3, "AUX");
		set_menu_button(4, "SENS");
	        set_mainnotebook_page(0);
		m_navarea.map_set_renderer(NavArea::maprenderer_t((MapDrawingArea::renderer_t)m_menuid, m_navarea.map_get_renderer().second));
		set_menu_button(6, "TIMER");
		m_softkeys[6].set_active(m_navarea.get_info_drawing_area().is_timer_running());
		set_menu_button(7, (m_sensors && m_sensors->nav_is_finalapproach()) ? "FASNAV" :
				(m_sensors && m_sensors->nav_is_directto()) ? "DCTNAV" : "FPLNAV");
		m_softkeys[7].set_active(m_sensors && m_sensors->nav_is_on());
		set_menu_button(8, "HOLD");
		m_softkeys[8].set_active(m_sensors && m_sensors->nav_is_hold());
		set_menu_button(9, "BRG2");
		m_softkeys[9].set_active(m_sensors && m_sensors->nav_is_brg2_enabled());
		set_menu_button(11, "SETUP");
	} else if (m_menuid < 200) {
		if (m_menuid >= 107) {
			m_menuid = 100;
			menuid_changed = true;
		}
		set_menu_button(0, "NAV");
		static const char * const wptnames[] = { "WPT:ARPT", "WPT:NAV", "WPT:INT", "WPT:AWY", "WPT:ASPC", "WPT:MAP", "WPT:FPL" };
		set_menu_button(1, wptnames[m_menuid - 100]);
		set_menu_button(2, "FPL");
		set_menu_button(3, "AUX");
		set_menu_button(4, "SENS");
		set_menu_button(8, "ZOOM+");
		set_menu_button(9, "ZOOM-");
		set_menu_button(10, "MAP:VM");
		if (0 && m_wptdrawingarea)
			set_menu_button(10, "MAP:" + to_short_str(m_wptdrawingarea->get_renderer()));
	        set_mainnotebook_page(2);
	} else if (m_menuid < 300) {
		if (m_menuid >= 202 && !fplgpsimppage_updatemenu()) {
			m_menuid = 200;
			menuid_changed = true;
		}
		set_menu_button(0, "NAV");
		set_menu_button(1, "WPT");
		set_menu_button(2, m_menuid == 201 ? "FPLS" : "FPL");
		set_menu_button(3, "AUX");
		set_menu_button(4, "SENS");
		switch (m_menuid) {
		case 200:
			set_mainnotebook_page(3);
			break;

		case 201:
			set_mainnotebook_page(4);
			break;

		default:
			set_mainnotebook_page(5);
			break;
		}
		if (m_menuid == 200) {
			set_menu_button(9, "SCRKBD");
			m_softkeys[9].set_active(m_onscreenkeyboard);
			set_menu_button(10, "MAP:VM");
			if (0 && m_fpldrawingarea)
				set_menu_button(10, "MAP:" + to_short_str(m_fpldrawingarea->get_renderer()));
			set_menu_button(7, "ZOOM+");
			set_menu_button(8, "ZOOM-");
			set_menu_button(11, "EDIT");
		}
		if (m_menuid >= 202) {
			set_menu_button(6, "BRG2");
			m_softkeys[6].set_active(m_sensors && m_sensors->nav_is_brg2_enabled());
			set_menu_button(7, "DIRECT");
			m_softkeys[7].set_active(m_sensors && m_sensors->nav_is_on());
			set_menu_button(8, "ZOOM+");
			set_menu_button(9, "ZOOM-");
			set_menu_button(10, "LOADFPL");
		}
	} else if (m_menuid < 400) {
		if (m_menuid >= 304) {
#ifdef HAVE_EVINCE
			m_menuid = 300;
#else
			m_menuid = 302;
#endif
			menuid_changed = true;
		}
#ifdef HAVE_EVINCE
		if (m_menuid == 301 && !docdir_display_document()) {
			m_menuid = 302;
			menuid_changed = true;
		}
#endif
		set_menu_button(0, "NAV");
		set_menu_button(1, "WPT");
		set_menu_button(2, "FPL");
		set_menu_button(3, "AUX");
		set_menu_button(4, "SENS");
		switch (m_menuid) {
#ifdef HAVE_EVINCE
		case 300:
		        set_mainnotebook_page(6);
			break;

		case 301:
		        set_mainnotebook_page(7);
			set_menu_button(5, "FIT");
			set_menu_button(6, "ZOOM+");
			set_menu_button(7, "ZOOM-");
			set_menu_button(8, "ROTATE");
			set_menu_button(9, "DOC+");
			set_menu_button(10, "DOC-");
			set_menu_button(11, "BACK");
			break;
#endif

		case 302:
			set_mainnotebook_page(8);
			set_menu_button(9, "SCRKBD");
			m_softkeys[9].set_active(m_onscreenkeyboard);
			break;

		case 303:
			set_mainnotebook_page(9);
			break;

		default:
		        set_mainnotebook_page(0);
			break;
		}
	} else if (m_menuid < 500) {
		Sensors::Sensor *sens(0);
		unsigned int senspgnr(0);
		if (!m_sensors || m_menuid == 400) {
			menuid_changed = menuid_changed || m_menuid != 400;
			m_menuid = 400;
		} else {
			unsigned int nrpages(0);
			Sensors::Sensor *sens1(0);
			unsigned int sens1nr(0);
			unsigned int dpgnr(m_menuid - 401);
			for (unsigned int i = 0; i < m_sensors->size(); ++i) {
				int pgnr(dpgnr - nrpages);
				unsigned int parpg((*m_sensors)[i].get_param_pages());
				if (pgnr >= 0 && pgnr < parpg) {
					sens = &(*m_sensors)[i];
					senspgnr = pgnr;
					if (false)
						std::cerr << "Menu ID: " << m_menuid << " Sensor " << i << " Page " << senspgnr << std::endl;
				}
				nrpages += parpg;
				if (!sens1 && parpg) {
					sens1 = &(*m_sensors)[i];
					sens1nr = i;
				}
			}
			if (!sens) {
				menuid_changed = menuid_changed || m_menuid != 400;
				m_menuid = 400;
				sens = sens1;
				senspgnr = 0;
			}
			if (false)
				std::cerr << "Menu ID: " << m_menuid << " Sensor " << sens1nr << " Page " << senspgnr << std::endl;
		}
		set_menu_button(0, "NAV");
		set_menu_button(1, "WPT");
		set_menu_button(2, "FPL");
		set_menu_button(3, "AUX");
		if (m_menuid == 400) {
			set_menu_button(4, "SENS: LOG");
		        set_mainnotebook_page(10);
			set_menu_button(9, log_level_button[m_logdisplaylevel]);
			set_menu_button(7, "FLTLOG");
			m_softkeys[7].set_active(m_sensors ? m_sensors->log_is_open() : false);
		} else {
			set_menu_button(4, "SENS: " + sens->get_name());
			sensorcfg_init(*sens, senspgnr);
			sensorcfg_set_admin(m_admin);
		        set_mainnotebook_page(1);
			set_menu_button(10, m_admin ? "ADMIN" : "USER");
			m_softkeys[10].set_active(m_admin);
			set_menu_button(9, "SCRKBD");
			m_softkeys[9].set_active(m_onscreenkeyboard);
		}
	} else if (m_menuid >= 1000 && m_menuid < 1100) {
		if (m_menuid == 1002) {
			set_menu_button(0, "ARPT+");
			set_menu_button(1, "ARPT-");
		} else {
			set_menu_button(0, "MAP+");
			set_menu_button(1, "MAP-");
		}
		set_menu_button(2, "TERRAIN+");
		set_menu_button(3, "TERRAIN-");
		set_menu_button(4, "MAPCFG");
		set_menu_button(5, "ALTCFG");
		set_menu_button(6, "HSICFG");
		set_menu_button(7, "MAPPAN");
		set_menu_button(9, m_sensors && m_sensors->get_configfile().get_integer(cfggroup_mainwindow, cfgkey_mainwindow_fullscreen) ? "FULLSCRN" : "WINDOW");
		set_menu_button(10, "QUIT");
		set_menu_button(11, "BACK");
		for (unsigned int i = 0; i < 12; ++i)
			m_softkeys[i].set_active(false);
		m_softkeys[7].set_active(m_navarea.map_get_pan());
	} else if (m_menuid == 2000) {
		set_mainnotebook_page(3);
		set_menu_button(0, "+WPT");
		set_menu_button(1, "+TXT");
		set_menu_button(2, "DUP");
		set_menu_button(3, "MV:UP");
		set_menu_button(4, "MV:DWN");
		set_menu_button(5, "-WPT");
		set_menu_button(6, "STRAIGHT");
		set_menu_button(8, "DCT TO");
		set_menu_button(9, "BRG 2");
		set_menu_button(10, "PREFLIGHT");
		set_menu_button(11, "BACK");
		for (unsigned int i = 0; i < 12; ++i)
			m_softkeys[i].set_active(false);
	} else if (m_menuid == 2001) {
		set_mainnotebook_page(3);
		set_menu_button(0, "WXFCST");
		set_menu_button(1, "CHARTS");
		set_menu_button(2, "PROFGRAPH");
		set_menu_button(3, "METAR/TAF");
		set_menu_button(4, "WXCHARTS");
		set_menu_button(5, "MAPTILES");
		set_menu_button(6, "NAVLOG");
		set_menu_button(7, "PROFILE");
		set_menu_button(8, "VERIF");
		set_menu_button(9, "AUTORTE");
		set_menu_button(10, "MAIN");
		set_menu_button(11, "BACK");
		for (unsigned int i = 0; i < 12; ++i)
			m_softkeys[i].set_active(false);
	} else if (m_menuid == 2002) {
		set_mainnotebook_page(11);
		set_menu_button(0, "PREV");
		set_menu_button(1, "NEXT");
		set_menu_button(2, "EPOCH-");
		set_menu_button(3, "EPOCH+");
		set_menu_button(4, "CTIME-");
		set_menu_button(5, "CTIME+");
		set_menu_button(6, "LEVEL-");
		set_menu_button(7, "LEVEL+");
		set_menu_button(10, "MAIN");
		set_menu_button(11, "BACK");
		for (unsigned int i = 0; i < 12; ++i)
			m_softkeys[i].set_active(false);
	} else if (m_menuid == 2003) {
		set_mainnotebook_page(12);
		if (m_vertprofiledrawingarea && m_vertprofiledrawingarea->is_chart()) {
			set_menu_button(0, "ZOOM+");
			set_menu_button(1, "ZOOM-");
			set_menu_button(2, "NEXT");
			set_menu_button(3, "PREV");
			set_menu_button(4, "PROFILE");
		} else {
			set_menu_button(0, "DZOOM+");
			set_menu_button(1, "DZOOM-");
			set_menu_button(2, "AZOOM+");
			set_menu_button(3, "AZOOM-");
			set_menu_button(4, "CHART");
			set_menu_button(5, "YAXIS");
		}
		set_menu_button(9, "SAVE");
		set_menu_button(10, "MAIN");
		set_menu_button(11, "BACK");
		for (unsigned int i = 0; i < 12; ++i)
			m_softkeys[i].set_active(false);
	} else if (m_menuid == 2004) {
		metartaf_update();
		set_mainnotebook_page(13);
		set_menu_button(0, "UPDATE");
		set_menu_button(1, "");
		set_menu_button(2, "");
		set_menu_button(3, "");
		set_menu_button(4, "");
		set_menu_button(10, "MAIN");
		set_menu_button(11, "BACK");
		for (unsigned int i = 0; i < 12; ++i)
			m_softkeys[i].set_active(false);
	} else if (m_menuid == 2005) {
		wxchartlist_update();
		set_mainnotebook_page(14);
		set_menu_button(0, "FETCH");
		set_menu_button(1, "VIEW");
		set_menu_button(2, "UPDATE");
		set_menu_button(3, "");
		set_menu_button(4, "");
		set_menu_button(10, "MAIN");
		set_menu_button(11, "BACK");
		for (unsigned int i = 0; i < 12; ++i)
			m_softkeys[i].set_active(false);
	} else if (m_menuid == 2006) {
		grib2layer_update();
		set_mainnotebook_page(15);
		set_menu_button(0, "ZOOM+");
		set_menu_button(1, "ZOOM-");
		set_menu_button(2, "NEXT");
		set_menu_button(3, "PREV");
		set_menu_button(4, "MAP");
		set_menu_button(10, "MAIN");
		set_menu_button(11, "BACK");
		for (unsigned int i = 0; i < 12; ++i)
			m_softkeys[i].set_active(false);
	} else {
		set_menu_button(0, "NAV");
		set_menu_button(1, "WPT");
		set_menu_button(2, "FPL");
		set_menu_button(3, "AUX");
		set_menu_button(4, "SENS");
	        set_mainnotebook_page(0);
	}
	if (m_menuid <= 300) {
		set_menu_button(5, m_navarea.map_get_declutter_name());
	}
	if (menuid_changed) {
		wptpage_updatemenu();
		fplpage_updatemenu();
		fpldirpage_updatemenu();
		docdirpage_updatemenu();
		perfpage_updatemenu();
		srsspage_updatemenu();
	}

	if (m_menuid < 1000)
		for (unsigned int i = 0; i < 5; ++i)
			m_softkeys[i].set_active(m_menuid >= 100 * i && m_menuid < 100 * (i + 1));
	for (int i = 0; i < 12; ++i) {
		if (m_softkeys[i].get_label().empty()) {
			m_softkeys[i].set_sensitive(false);
			m_softkeys[i].set_active(false);
		} else {
			m_softkeys[i].set_sensitive(true);
		}
		m_softkeys.unblock(i);
	}
	fplpage_softkeyenable();
	if (true)
		std::cerr << "New Menu ID: " << m_menuid << " (" << id << ")" << std::endl;
	if (false)
		std::cerr << "# sensors: " << (!m_sensors ? -1 : m_sensors->size()) << std::endl;
}

void FlightDeckWindow::set_menu_button(unsigned int nr, const Glib::ustring& txt)
{
	m_softkeys[nr].set_label(txt);
}

bool FlightDeckWindow::on_key_press_event(GdkEventKey *event)
{
        if (event && event->type == GDK_KEY_PRESS) {
		if (false)
			std::cerr << "on_key_keypress_event: keyval " << event->keyval << " state " << event->state
				  << " str " << event->string << " length " << event->length
				  << " hwcode " << event->hardware_keycode << " group " << (unsigned int)event->group << std::endl;
		switch (event->keyval) {
                case GDK_KEY_F1:
                        menubutton_clicked(0);
                        return true;

                case GDK_KEY_F2:
                        menubutton_clicked(1);
                        return true;

                case GDK_KEY_F3:
                        menubutton_clicked(2);
                        return true;

                case GDK_KEY_F4:
                        menubutton_clicked(3);
                        return true;

                case GDK_KEY_F5:
                        menubutton_clicked(4);
                        return true;

                case GDK_KEY_F6:
                        menubutton_clicked(5);
                        return true;

                case GDK_KEY_F7:
                        menubutton_clicked(6);
                        return true;

                case GDK_KEY_F8:
                        menubutton_clicked(7);
                        return true;

                case GDK_KEY_F9:
                        menubutton_clicked(8);
                        return true;

                case GDK_KEY_F10:
                        menubutton_clicked(9);
                        return true;

                case GDK_KEY_F11:
                        menubutton_clicked(10);
                        return true;

                case GDK_KEY_F12:
                        menubutton_clicked(11);
                        return true;

                default:
                        break;
		}
	}
#ifdef GLIBMM_DEFAULT_SIGNAL_HANDLERS_ENABLED
	if (FullscreenableWindow::on_key_press_event(event))
		return true;
#endif //GLIBMM_DEFAULT_SIGNAL_HANDLERS_ENABLED
	return false;
}
