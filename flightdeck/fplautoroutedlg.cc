//
// C++ Implementation: FlightDeckWindow
//
// Description: Flight Deck Windows, Autoroute Dialog
//
//
// Author: Thomas Sailer <t.sailer@alumni.ethz.ch>, (C) 2011, 2012, 2013, 2014
//
// Copyright: See COPYING file that comes with this distribution
//
//

#include "sysdeps.h"
#include "sysdepsgui.h"
#include <iomanip>

#include "flightdeckwindow.h"
#include "icaofpl.h"

FlightDeckWindow::FPLAutoRouteDialog::TreeViewPopup::TreeViewPopup(BaseObjectType *castitem, const Glib::RefPtr<Builder>& refxml)
	: Gtk::TreeView(castitem), m_menuitem_new(0), m_menuitem_delete(0)
{
	m_menuitem_new = Gtk::manage(new Gtk::MenuItem("_New", true));
	m_menu.append(*m_menuitem_new);
	m_menuitem_delete = Gtk::manage(new Gtk::MenuItem("_Delete", true));
	m_menu.append(*m_menuitem_delete);
	m_menu.accelerate(*this);
	m_menu.show_all();
#ifndef GLIBMM_DEFAULT_SIGNAL_HANDLERS_ENABLED
	signal_button_press_event().connect(sigc::mem_fun(*this, &TreeViewPopup::on_button_press_event), false);
#endif
}

FlightDeckWindow::FPLAutoRouteDialog::TreeViewPopup::~TreeViewPopup()
{
}

sigc::connection FlightDeckWindow::FPLAutoRouteDialog::TreeViewPopup::connect_menunew(const sigc::slot<void>& slot)
{
	if (!m_menuitem_new)
		return sigc::connection();
	return m_menuitem_new->signal_activate().connect(slot);
}

sigc::connection FlightDeckWindow::FPLAutoRouteDialog::TreeViewPopup::connect_menudelete(const sigc::slot<void>& slot)
{
	if (!m_menuitem_delete)
		return sigc::connection();
	return m_menuitem_delete->signal_activate().connect(slot);
}

void FlightDeckWindow::FPLAutoRouteDialog::TreeViewPopup::set_sensitive_new(bool s)
{
	if (m_menuitem_new)
		m_menuitem_new->set_sensitive(s);
}

void FlightDeckWindow::FPLAutoRouteDialog::TreeViewPopup::set_sensitive_delete(bool s)
{
	if (m_menuitem_delete)
		m_menuitem_delete->set_sensitive(s);
}

bool FlightDeckWindow::FPLAutoRouteDialog::TreeViewPopup::on_button_press_event(GdkEventButton *ev)
{
	bool ret(Gtk::TreeView::on_button_press_event(ev));
	if (ev->type == GDK_BUTTON_PRESS && ev->button == 3)
		m_menu.popup(ev->button, ev->time);
	return ret;
}

FlightDeckWindow::FPLAutoRouteDialog::RulesModelColumns::RulesModelColumns(void)
{
	add(m_rule);
}

FlightDeckWindow::FPLAutoRouteDialog::CrossingModelColumns::CrossingModelColumns(void)
{
	add(m_coord);
	add(m_ident);
	add(m_name);
	add(m_radius);
	add(m_minlevel);
	add(m_maxlevel);
}

FlightDeckWindow::FPLAutoRouteDialog::AirspaceModelColumns::AirspaceModelColumns(void)
{
	add(m_ident);
	add(m_type);
	add(m_name);
	add(m_coord);
	add(m_minlevel);
	add(m_maxlevel);
	add(m_awylimit);
	add(m_dctlimit);
	add(m_dctoffset);
	add(m_dctscale);
}

FlightDeckWindow::FPLAutoRouteDialog::RoutesModelColumns::RoutesModelColumns(void)
{	
	add(m_time);
	add(m_fuel);
	add(m_routetxt);
	add(m_flags);
	add(m_route);
	add(m_routemetric);
	add(m_routefuel);
	add(m_routezerowindfuel);
	add(m_routetime);
	add(m_routezerowindtime);
	add(m_variant);
}

class FlightDeckWindow::FPLAutoRouteDialog::FindAirspace {
public:
	FindAirspace(Engine& engine);
	bool find(const std::string& icao);
	const Engine::AirspaceResult::elementvector_t& get_airspaces(void) const { return m_airspaces; }

protected:
	Engine::AirspaceResult::elementvector_t m_airspaces;
	Glib::RefPtr<Engine::AirspaceResult> m_airspacequery;
	Glib::Cond m_cond;
	Glib::Mutex m_mutex;
	Engine& m_engine;

	void async_cancel(void);
	void async_clear(void);
	void async_connectdone(void);
	void async_done(void);
	unsigned int async_waitdone(void);
};

FlightDeckWindow::FPLAutoRouteDialog::FindAirspace::FindAirspace(Engine& engine)
	: m_engine(engine)
{
}

bool FlightDeckWindow::FPLAutoRouteDialog::FindAirspace::find(const std::string& icao)
{
        async_cancel();
        async_clear();
        if (!icao.empty())
		m_airspacequery = m_engine.async_airspace_find_by_icao(icao, ~0, 0, AirportsDb::comp_exact, AirspacesDb::element_t::subtables_none);
	async_connectdone();
	async_waitdone();
	return !m_airspaces.empty();
}

void FlightDeckWindow::FPLAutoRouteDialog::FindAirspace::async_cancel(void)
{
        Engine::AirportResult::cancel(m_airspacequery);
}

void FlightDeckWindow::FPLAutoRouteDialog::FindAirspace::async_clear(void)
{
        m_airspacequery = Glib::RefPtr<Engine::AirspaceResult>();
        m_airspaces.clear();
}

void FlightDeckWindow::FPLAutoRouteDialog::FindAirspace::async_connectdone(void)
{
        if (m_airspacequery)
                m_airspacequery->connect(sigc::mem_fun(*this, &FindAirspace::async_done));
}

void FlightDeckWindow::FPLAutoRouteDialog::FindAirspace::async_done(void)
{
	Glib::Mutex::Lock lock(m_mutex);
        m_cond.broadcast();
}

unsigned int FlightDeckWindow::FPLAutoRouteDialog::FindAirspace::async_waitdone(void)
{
        unsigned int flags(0);
        Glib::Mutex::Lock lock(m_mutex);
        for (;;) {
		if (m_airspacequery && m_airspacequery->is_done()) {
                        m_airspaces.clear();
                        if (!m_airspacequery->is_error())
                                m_airspaces = m_airspacequery->get_result();
                        m_airspacequery = Glib::RefPtr<Engine::AirspaceResult>();
                }
		if (!m_airspacequery)
                        break;
                m_cond.wait(m_mutex);
        }
        return flags;
}

const unsigned int FlightDeckWindow::FPLAutoRouteDialog::nrkeybuttons;
const struct FlightDeckWindow::FPLAutoRouteDialog::keybutton FlightDeckWindow::FPLAutoRouteDialog::keybuttons[nrkeybuttons] = {
	{ "fplautoroutebutton0", GDK_KEY_0, GDK_KEY_parenright, 19 },
	{ "fplautoroutebutton1", GDK_KEY_1, GDK_KEY_exclam, 10 },
	{ "fplautoroutebutton2", GDK_KEY_2, GDK_KEY_at, 11 },
	{ "fplautoroutebutton3", GDK_KEY_3, GDK_KEY_numbersign, 12 },
	{ "fplautoroutebutton4", GDK_KEY_4, GDK_KEY_dollar, 13 },
	{ "fplautoroutebutton5", GDK_KEY_5, GDK_KEY_percent, 14 },
	{ "fplautoroutebutton6", GDK_KEY_6, GDK_KEY_asciicircum, 15 },
	{ "fplautoroutebutton7", GDK_KEY_7, GDK_KEY_ampersand, 16 },
	{ "fplautoroutebutton8", GDK_KEY_8, GDK_KEY_asterisk, 17 },
	{ "fplautoroutebutton9", GDK_KEY_9, GDK_KEY_parenleft, 18 },
	{ "fplautoroutebuttona", GDK_KEY_a, GDK_KEY_A, 38 },
	{ "fplautoroutebuttonb", GDK_KEY_b, GDK_KEY_B, 56 },
	{ "fplautoroutebuttonc", GDK_KEY_c, GDK_KEY_C, 54 },
	{ "fplautoroutebuttond", GDK_KEY_d, GDK_KEY_D, 40 },
	{ "fplautoroutebuttone", GDK_KEY_e, GDK_KEY_E, 26 },
	{ "fplautoroutebuttonf", GDK_KEY_f, GDK_KEY_F, 41 },
	{ "fplautoroutebuttong", GDK_KEY_g, GDK_KEY_G, 42 },
	{ "fplautoroutebuttonh", GDK_KEY_h, GDK_KEY_H, 43 },
	{ "fplautoroutebuttoni", GDK_KEY_i, GDK_KEY_I, 31 },
	{ "fplautoroutebuttonj", GDK_KEY_j, GDK_KEY_J, 44 },
	{ "fplautoroutebuttonk", GDK_KEY_k, GDK_KEY_K, 45 },
	{ "fplautoroutebuttonl", GDK_KEY_l, GDK_KEY_L, 46 },
	{ "fplautoroutebuttonm", GDK_KEY_m, GDK_KEY_M, 58 },
	{ "fplautoroutebuttonn", GDK_KEY_n, GDK_KEY_N, 57 },
	{ "fplautoroutebuttono", GDK_KEY_o, GDK_KEY_O, 32 },
	{ "fplautoroutebuttonp", GDK_KEY_p, GDK_KEY_P, 33 },
	{ "fplautoroutebuttonq", GDK_KEY_q, GDK_KEY_Q, 24 },
	{ "fplautoroutebuttonr", GDK_KEY_r, GDK_KEY_R, 27 },
	{ "fplautoroutebuttons", GDK_KEY_s, GDK_KEY_S, 39 },
	{ "fplautoroutebuttont", GDK_KEY_t, GDK_KEY_T, 28 },
	{ "fplautoroutebuttonu", GDK_KEY_u, GDK_KEY_U, 30 },
	{ "fplautoroutebuttonv", GDK_KEY_v, GDK_KEY_V, 55 },
	{ "fplautoroutebuttonw", GDK_KEY_w, GDK_KEY_W, 25 },
	{ "fplautoroutebuttonx", GDK_KEY_x, GDK_KEY_X, 53 },
	{ "fplautoroutebuttony", GDK_KEY_y, GDK_KEY_Y, 29 },
	{ "fplautoroutebuttonz", GDK_KEY_z, GDK_KEY_Z, 52 },
	{ "fplautoroutebuttonapostrophe", GDK_KEY_quoteright, GDK_KEY_quotedbl, 48 },
	{ "fplautoroutebuttonbracketclose", GDK_KEY_bracketright, GDK_KEY_braceright, 35 },
	{ "fplautoroutebuttonbracketopen", GDK_KEY_bracketleft, GDK_KEY_braceleft, 34 },
	{ "fplautoroutebuttoncomma", GDK_KEY_comma, GDK_KEY_less, 59 },
	{ "fplautoroutebuttondash", GDK_KEY_minus, GDK_KEY_underscore, 20 },
	{ "fplautoroutebuttondot", GDK_KEY_period, GDK_KEY_greater, 60 },
	{ "fplautoroutebuttonequal", GDK_KEY_equal, GDK_KEY_plus, 21 },
	{ "fplautoroutebuttonquestionmark", GDK_KEY_question, GDK_KEY_slash, 61 },
	{ "fplautoroutebuttonsemicolon", GDK_KEY_semicolon, GDK_KEY_colon, 47 },
	{ "fplautoroutebuttonspace", GDK_KEY_space, GDK_KEY_space, 65 },
	{ "fplautoroutebuttonbackslash", GDK_KEY_backslash, GDK_KEY_bar, 51 },
	{ "fplautoroutebuttonbackspace", GDK_KEY_BackSpace, GDK_KEY_BackSpace, 22 }
};

FlightDeckWindow::FPLAutoRouteDialog::FPLAutoRouteDialog(BaseObjectType *castitem, const Glib::RefPtr<Builder>& refxml)
	: Gtk::Window(castitem), m_sensors(0), m_fplautoroutenotebook(0),
	  m_fplautorouteentrydepicao(0), m_fplautorouteentrydepname(0), m_fplautoroutecheckbuttondepifr(0),
	  m_fplautoroutelabelsid(0), m_fplautorouteentrysidicao(0), m_fplautorouteentrysidname(0),
	  m_fplautoroutecheckbuttonsiddb(0), m_fplautoroutecheckbuttonsidonly(0),
	  m_fplautorouteentrydesticao(0), m_fplautorouteentrydestname(0), m_fplautoroutecheckbuttondestifr(0),
	  m_fplautoroutelabelstar(0), m_fplautorouteentrystaricao(0), m_fplautorouteentrystarname(0),
	  m_fplautoroutecheckbuttonstardb(0), m_fplautoroutecheckbuttonstaronly(0),
	  m_fplautoroutespinbuttondctlimit(0), m_fplautoroutespinbuttondctpenalty(0), m_fplautoroutespinbuttondctoffset(0),
	  m_fplautoroutespinbuttonstdroutepenalty(0), m_fplautoroutespinbuttonstdrouteoffset(0),
	  m_fplautoroutespinbuttonstdroutedctpenalty(0), m_fplautoroutespinbuttonstdroutedctoffset(0),
	  m_fplautoroutespinbuttonukcasdctpenalty(0), m_fplautoroutespinbuttonukcasdctoffset(0),
	  m_fplautoroutespinbuttonukocasdctpenalty(0), m_fplautoroutespinbuttonukocasdctoffset(0),
	  m_fplautoroutespinbuttonukenterdctpenalty(0), m_fplautoroutespinbuttonukenterdctoffset(0),
	  m_fplautoroutespinbuttonukleavedctpenalty(0), m_fplautoroutespinbuttonukleavedctoffset(0),
	  m_fplautoroutespinbuttonminlevel(0), m_fplautoroutespinbuttonminlevelpenalty(0),
	  m_fplautoroutespinbuttonminlevelmaxpenalty(0), m_fplautoroutespinbuttonvfraspclimit(0),
	  m_fplautoroutecheckbuttonforceenrouteifr(0), m_fplautoroutecheckbuttonwind(0),
	  m_fplautoroutecheckbuttonawylevels(0), m_fplautoroutecheckbuttonprecompgraph(0),
	  m_fplautoroutecheckbuttontfr(0), m_fplautoroutecomboboxvalidator(0),
	  m_fplautoroutespinbuttonsidlimit(0), m_fplautoroutespinbuttonsidpenalty(0), m_fplautoroutespinbuttonsidoffset(0),
	  m_fplautoroutespinbuttonsidminimum(0), m_fplautoroutespinbuttonstarlimit(0), m_fplautoroutespinbuttonstarpenalty(0),
	  m_fplautoroutespinbuttonstaroffset(0), m_fplautoroutespinbuttonstarminimum(0),
	  m_fplautoroutespinbuttonbaselevel(0), m_fplautoroutespinbuttontoplevel(0), m_fplautoroutecomboboxopttarget(0),
	  m_fplautoroutetreeviewtracerules(0), m_fplautoroutebuttontracerulesadd(0), m_fplautoroutebuttontracerulesdelete(0),
	  m_fplautoroutetreeviewdisablerules(0), m_fplautoroutebuttondisablerulesadd(0), m_fplautoroutebuttondisablerulesdelete(0),
	  m_fplautoroutetreeviewcrossing(0), m_fplautoroutebuttoncrossingadd(0), m_fplautoroutebuttoncrossingdelete(0),
	  m_fplautoroutetreeviewairspace(0), m_fplautoroutebuttonairspaceadd(0), m_fplautoroutebuttonairspacedelete(0),
	  m_fplautoroutespinbuttonpreferredminlevel(0), m_fplautoroutespinbuttonpreferredmaxlevel(0),
	  m_fplautoroutespinbuttonpreferredpenalty(0), m_fplautoroutespinbuttonpreferredclimb(0),
	  m_fplautoroutespinbuttonpreferreddescent(0),
	  m_fplautoroutecalendar(0), m_fplautoroutebuttontoday(0),
	  m_fplautoroutespinbuttonhour(0), m_fplautoroutespinbuttonmin(0),
	  m_fplautoroutebuttonpg1close(0), m_fplautoroutebuttonpg1copy(0), m_fplautoroutebuttonpg1next(0),
	  m_fplautoroutenotebookpg2(0), m_fplautoroutedrawingarea(0), m_fplautorouteentrypg2dep(0), m_fplautorouteentrypg2dest(0),
	  m_fplautorouteentrypg2gcdist(0), m_fplautorouteentrypg2routedist(0),m_fplautorouteentrypg2mintime(0),
	  m_fplautorouteentrypg2routetime(0), m_fplautorouteentrypg2minfuel(0),
	  m_fplautorouteentrypg2routefuel(0), m_fplautorouteentrypg2iteration(0), m_fplautorouteentrypg2times(0),
	  m_fplautorouteframepg2routes(0), m_fplautoroutetreeviewpg2routes(0), m_fplautoroutetextviewpg2route(0),
	  m_fplautoroutetextviewpg2results(0), m_fplautoroutetextviewpg2log(0), m_fplautoroutetextviewpg2graph(0),
	  m_fplautoroutetextviewpg2comm(0), m_fplautoroutetextviewpg2debug(0),
	  m_fplautoroutebuttonpg2zoomin(0), m_fplautoroutebuttonpg2zoomout(0), m_fplautoroutebuttonpg2back(0),
	  m_fplautoroutebuttonpg2close(0), m_fplautoroutebuttonpg2copy(0), m_fplautoroutetogglebuttonpg2run(0),
	  m_fplautoroutetogglebuttonshiftleft(0), m_fplautoroutetogglebuttonshiftright(0), m_fplautoroutetogglebuttoncapslock(0),
	  m_shiftstate(shiftstate_none), m_sidend(Point::invalid), m_starstart(Point::invalid),
	  m_depifr(true), m_destifr(true), m_firstrun(true), m_fplvalclear(true),
	  m_flags(MapRenderer::drawflags_route | MapRenderer::drawflags_route_labels), m_mapscale(0.1)
{
#ifndef GLIBMM_DEFAULT_SIGNAL_HANDLERS_ENABLED
        signal_delete_event().connect(sigc::mem_fun(*this, &FPLAutoRouteDialog::on_delete_event));
#endif //GLIBMM_DEFAULT_SIGNAL_HANDLERS_ENABLED
	get_widget(refxml, "fplautoroutenotebook", m_fplautoroutenotebook);
	get_widget(refxml, "fplautorouteentrydepicao", m_fplautorouteentrydepicao);
	get_widget(refxml, "fplautorouteentrydepname", m_fplautorouteentrydepname);
	get_widget(refxml, "fplautoroutecheckbuttondepifr", m_fplautoroutecheckbuttondepifr);
	get_widget(refxml, "fplautoroutelabelsid", m_fplautoroutelabelsid);
	get_widget(refxml, "fplautorouteentrysidicao", m_fplautorouteentrysidicao);
	get_widget(refxml, "fplautorouteentrysidname", m_fplautorouteentrysidname);
	get_widget(refxml, "fplautoroutecheckbuttonsiddb", m_fplautoroutecheckbuttonsiddb);
	get_widget(refxml, "fplautoroutecheckbuttonsidonly", m_fplautoroutecheckbuttonsidonly);
	get_widget(refxml, "fplautorouteentrydesticao", m_fplautorouteentrydesticao);
	get_widget(refxml, "fplautorouteentrydestname", m_fplautorouteentrydestname);
	get_widget(refxml, "fplautoroutecheckbuttondestifr", m_fplautoroutecheckbuttondestifr);
	get_widget(refxml, "fplautoroutelabelstar", m_fplautoroutelabelstar);
	get_widget(refxml, "fplautorouteentrystaricao", m_fplautorouteentrystaricao);
	get_widget(refxml, "fplautorouteentrystarname", m_fplautorouteentrystarname);
	get_widget(refxml, "fplautoroutecheckbuttonstardb", m_fplautoroutecheckbuttonstardb);
	get_widget(refxml, "fplautoroutecheckbuttonstaronly", m_fplautoroutecheckbuttonstaronly);
	get_widget(refxml, "fplautoroutespinbuttondctlimit", m_fplautoroutespinbuttondctlimit);
	get_widget(refxml, "fplautoroutespinbuttondctpenalty", m_fplautoroutespinbuttondctpenalty);
	get_widget(refxml, "fplautoroutespinbuttondctoffset", m_fplautoroutespinbuttondctoffset);
	get_widget(refxml, "fplautoroutespinbuttonstdroutepenalty", m_fplautoroutespinbuttonstdroutepenalty);
	get_widget(refxml, "fplautoroutespinbuttonstdrouteoffset", m_fplautoroutespinbuttonstdrouteoffset);
	get_widget(refxml, "fplautoroutespinbuttonstdroutedctpenalty", m_fplautoroutespinbuttonstdroutedctpenalty);
	get_widget(refxml, "fplautoroutespinbuttonstdroutedctoffset", m_fplautoroutespinbuttonstdroutedctoffset);
	get_widget(refxml, "fplautoroutespinbuttonukcasdctpenalty", m_fplautoroutespinbuttonukcasdctpenalty);
	get_widget(refxml, "fplautoroutespinbuttonukcasdctoffset", m_fplautoroutespinbuttonukcasdctoffset);
	get_widget(refxml, "fplautoroutespinbuttonukocasdctpenalty", m_fplautoroutespinbuttonukocasdctpenalty);
	get_widget(refxml, "fplautoroutespinbuttonukocasdctoffset", m_fplautoroutespinbuttonukocasdctoffset);
	get_widget(refxml, "fplautoroutespinbuttonukenterdctpenalty", m_fplautoroutespinbuttonukenterdctpenalty);
	get_widget(refxml, "fplautoroutespinbuttonukenterdctoffset", m_fplautoroutespinbuttonukenterdctoffset);
	get_widget(refxml, "fplautoroutespinbuttonukleavedctpenalty", m_fplautoroutespinbuttonukleavedctpenalty);
	get_widget(refxml, "fplautoroutespinbuttonukleavedctoffset", m_fplautoroutespinbuttonukleavedctoffset);
	get_widget(refxml, "fplautoroutespinbuttonminlevel", m_fplautoroutespinbuttonminlevel);
	get_widget(refxml, "fplautoroutespinbuttonminlevelpenalty", m_fplautoroutespinbuttonminlevelpenalty);
	get_widget(refxml, "fplautoroutespinbuttonminlevelmaxpenalty", m_fplautoroutespinbuttonminlevelmaxpenalty);
	get_widget(refxml, "fplautoroutespinbuttonvfraspclimit", m_fplautoroutespinbuttonvfraspclimit);
	get_widget(refxml, "fplautoroutecheckbuttonforceenrouteifr", m_fplautoroutecheckbuttonforceenrouteifr);
	get_widget(refxml, "fplautoroutecheckbuttonwind", m_fplautoroutecheckbuttonwind);
	get_widget(refxml, "fplautoroutecheckbuttonawylevels", m_fplautoroutecheckbuttonawylevels);
	get_widget(refxml, "fplautoroutecheckbuttonprecompgraph", m_fplautoroutecheckbuttonprecompgraph);
	get_widget(refxml, "fplautoroutecheckbuttontfr", m_fplautoroutecheckbuttontfr);
	get_widget(refxml, "fplautoroutecomboboxvalidator", m_fplautoroutecomboboxvalidator);
	get_widget(refxml, "fplautoroutespinbuttonsidlimit", m_fplautoroutespinbuttonsidlimit);
	get_widget(refxml, "fplautoroutespinbuttonsidpenalty", m_fplautoroutespinbuttonsidpenalty);
	get_widget(refxml, "fplautoroutespinbuttonsidoffset", m_fplautoroutespinbuttonsidoffset);
	get_widget(refxml, "fplautoroutespinbuttonsidminimum", m_fplautoroutespinbuttonsidminimum);
	get_widget(refxml, "fplautoroutespinbuttonstarlimit", m_fplautoroutespinbuttonstarlimit);
	get_widget(refxml, "fplautoroutespinbuttonstarpenalty", m_fplautoroutespinbuttonstarpenalty);
	get_widget(refxml, "fplautoroutespinbuttonstaroffset", m_fplautoroutespinbuttonstaroffset);
	get_widget(refxml, "fplautoroutespinbuttonstarminimum", m_fplautoroutespinbuttonstarminimum);
	get_widget(refxml, "fplautoroutespinbuttonbaselevel", m_fplautoroutespinbuttonbaselevel);
	get_widget(refxml, "fplautoroutespinbuttontoplevel", m_fplautoroutespinbuttontoplevel);
	get_widget(refxml, "fplautoroutecomboboxopttarget", m_fplautoroutecomboboxopttarget);
	get_widget_derived(refxml, "fplautoroutetreeviewtracerules", m_fplautoroutetreeviewtracerules);
	get_widget(refxml, "fplautoroutebuttontracerulesadd", m_fplautoroutebuttontracerulesadd);
	get_widget(refxml, "fplautoroutebuttontracerulesdelete", m_fplautoroutebuttontracerulesdelete);
	get_widget_derived(refxml, "fplautoroutetreeviewdisablerules", m_fplautoroutetreeviewdisablerules);
	get_widget(refxml, "fplautoroutebuttondisablerulesadd", m_fplautoroutebuttondisablerulesadd);
	get_widget(refxml, "fplautoroutebuttondisablerulesdelete", m_fplautoroutebuttondisablerulesdelete);
	get_widget_derived(refxml, "fplautoroutetreeviewcrossing", m_fplautoroutetreeviewcrossing);
	get_widget(refxml, "fplautoroutebuttoncrossingadd", m_fplautoroutebuttoncrossingadd);
	get_widget(refxml, "fplautoroutebuttoncrossingdelete", m_fplautoroutebuttoncrossingdelete);
	get_widget_derived(refxml, "fplautoroutetreeviewairspace", m_fplautoroutetreeviewairspace);
	get_widget(refxml, "fplautoroutebuttonairspaceadd", m_fplautoroutebuttonairspaceadd);
	get_widget(refxml, "fplautoroutebuttonairspacedelete", m_fplautoroutebuttonairspacedelete);
	get_widget(refxml, "fplautoroutespinbuttonpreferredminlevel", m_fplautoroutespinbuttonpreferredminlevel);
	get_widget(refxml, "fplautoroutespinbuttonpreferredmaxlevel", m_fplautoroutespinbuttonpreferredmaxlevel);
	get_widget(refxml, "fplautoroutespinbuttonpreferredpenalty", m_fplautoroutespinbuttonpreferredpenalty);
	get_widget(refxml, "fplautoroutespinbuttonpreferredclimb", m_fplautoroutespinbuttonpreferredclimb);
	get_widget(refxml, "fplautoroutespinbuttonpreferreddescent", m_fplautoroutespinbuttonpreferreddescent);
	get_widget(refxml, "fplautoroutecalendar", m_fplautoroutecalendar);
	get_widget(refxml, "fplautoroutebuttontoday", m_fplautoroutebuttontoday);
	get_widget(refxml, "fplautoroutespinbuttonhour", m_fplautoroutespinbuttonhour);
	get_widget(refxml, "fplautoroutespinbuttonmin", m_fplautoroutespinbuttonmin);
	get_widget(refxml, "fplautoroutebuttonpg1close", m_fplautoroutebuttonpg1close);
	get_widget(refxml, "fplautoroutebuttonpg1copy", m_fplautoroutebuttonpg1copy);
	get_widget(refxml, "fplautoroutebuttonpg1next", m_fplautoroutebuttonpg1next);
	get_widget(refxml, "fplautoroutenotebookpg2", m_fplautoroutenotebookpg2);
	get_widget_derived(refxml, "fplautoroutedrawingarea", m_fplautoroutedrawingarea);
	get_widget(refxml, "fplautorouteentrypg2dep", m_fplautorouteentrypg2dep);
	get_widget(refxml, "fplautorouteentrypg2dest", m_fplautorouteentrypg2dest);
	get_widget(refxml, "fplautorouteentrypg2gcdist", m_fplautorouteentrypg2gcdist);
	get_widget(refxml, "fplautorouteentrypg2routedist", m_fplautorouteentrypg2routedist);
	get_widget(refxml, "fplautorouteentrypg2mintime", m_fplautorouteentrypg2mintime);
	get_widget(refxml, "fplautorouteentrypg2routetime", m_fplautorouteentrypg2routetime);
	get_widget(refxml, "fplautorouteentrypg2minfuel", m_fplautorouteentrypg2minfuel);
	get_widget(refxml, "fplautorouteentrypg2routefuel", m_fplautorouteentrypg2routefuel);
	get_widget(refxml, "fplautorouteentrypg2iteration", m_fplautorouteentrypg2iteration);
	get_widget(refxml, "fplautorouteentrypg2times", m_fplautorouteentrypg2times);
	get_widget(refxml, "fplautorouteframepg2routes", m_fplautorouteframepg2routes);
	get_widget(refxml, "fplautoroutetreeviewpg2routes", m_fplautoroutetreeviewpg2routes);
	get_widget(refxml, "fplautoroutetextviewpg2route", m_fplautoroutetextviewpg2route);
	get_widget(refxml, "fplautoroutetextviewpg2results", m_fplautoroutetextviewpg2results);
	get_widget(refxml, "fplautoroutetextviewpg2log", m_fplautoroutetextviewpg2log);
	get_widget(refxml, "fplautoroutetextviewpg2graph", m_fplautoroutetextviewpg2graph);
	get_widget(refxml, "fplautoroutetextviewpg2comm", m_fplautoroutetextviewpg2comm);
	get_widget(refxml, "fplautoroutetextviewpg2debug", m_fplautoroutetextviewpg2debug);
	get_widget(refxml, "fplautoroutebuttonpg2zoomin", m_fplautoroutebuttonpg2zoomin);
	get_widget(refxml, "fplautoroutebuttonpg2zoomout", m_fplautoroutebuttonpg2zoomout);
	get_widget(refxml, "fplautoroutebuttonpg2back", m_fplautoroutebuttonpg2back);
	get_widget(refxml, "fplautoroutebuttonpg2close", m_fplautoroutebuttonpg2close);
	get_widget(refxml, "fplautoroutebuttonpg2copy", m_fplautoroutebuttonpg2copy);
	get_widget(refxml, "fplautoroutetogglebuttonpg2run", m_fplautoroutetogglebuttonpg2run);
	get_widget(refxml, "fplautoroutetogglebuttonshiftleft", m_fplautoroutetogglebuttonshiftleft);
	get_widget(refxml, "fplautoroutetogglebuttonshiftright", m_fplautoroutetogglebuttonshiftright);
	get_widget(refxml, "fplautoroutetogglebuttoncapslock", m_fplautoroutetogglebuttoncapslock);
	if (m_fplautoroutetogglebuttonshiftleft)
		m_connshiftleft = m_fplautoroutetogglebuttonshiftleft->signal_clicked().connect(sigc::mem_fun(*this, &FPLAutoRouteDialog::shift_clicked));
	if (m_fplautoroutetogglebuttonshiftright)
		m_connshiftright = m_fplautoroutetogglebuttonshiftright->signal_clicked().connect(sigc::mem_fun(*this, &FPLAutoRouteDialog::shift_clicked));
	if (m_fplautoroutetogglebuttoncapslock)
		m_conncapslock = m_fplautoroutetogglebuttoncapslock->signal_clicked().connect(sigc::mem_fun(*this, &FPLAutoRouteDialog::capslock_clicked));
	for (unsigned int i = 0; i < nrkeybuttons; ++i) {
		m_keybutton[i] = 0;
		get_widget(refxml, keybuttons[i].widget_name, m_keybutton[i]);
		if (!m_keybutton[i])
			continue;
		m_keybutton[i]->signal_clicked().connect(sigc::bind(sigc::mem_fun(*this, &FPLAutoRouteDialog::key_clicked), i));
		m_keybutton[i]->set_can_focus(false);
	}
	if (m_fplautoroutenotebook) {
		m_fplautoroutenotebook->set_current_page(0);
		m_fplautoroutenotebook->set_show_border(false);
		m_fplautoroutenotebook->set_show_tabs(false);
	}
	if (m_fplautoroutebuttontoday)
		m_fplautoroutebuttontoday->signal_clicked().connect(sigc::mem_fun(*this, &FPLAutoRouteDialog::today_clicked));
	if (m_fplautoroutecheckbuttontfr)
		m_fplautoroutecheckbuttontfr->signal_toggled().connect(sigc::mem_fun(*this, &FPLAutoRouteDialog::tfr_toggled));
	if (m_fplautoroutecheckbuttonwind)
		m_fplautoroutecheckbuttonwind->signal_toggled().connect(sigc::mem_fun(*this, &FPLAutoRouteDialog::wind_toggled));
	if (m_fplautoroutebuttonpg1close)
		m_fplautoroutebuttonpg1close->signal_clicked().connect(sigc::mem_fun(*this, &FPLAutoRouteDialog::pg1_close_clicked));
	if (m_fplautoroutebuttonpg1copy)
		m_fplautoroutebuttonpg1copy->signal_clicked().connect(sigc::mem_fun(*this, &FPLAutoRouteDialog::pg1_copy_clicked));
	if (m_fplautoroutebuttonpg1next)
		m_fplautoroutebuttonpg1next->signal_clicked().connect(sigc::mem_fun(*this, &FPLAutoRouteDialog::pg1_next_clicked));
	if (m_fplautoroutebuttonpg2zoomin)
		m_fplautoroutebuttonpg2zoomin->signal_clicked().connect(sigc::mem_fun(*this, &FPLAutoRouteDialog::pg2_zoomin_clicked));
	if (m_fplautoroutebuttonpg2zoomout)
		m_fplautoroutebuttonpg2zoomout->signal_clicked().connect(sigc::mem_fun(*this, &FPLAutoRouteDialog::pg2_zoomout_clicked));
	if (m_fplautoroutebuttonpg2back)
		m_fplautoroutebuttonpg2back->signal_clicked().connect(sigc::mem_fun(*this, &FPLAutoRouteDialog::pg2_back_clicked));
	if (m_fplautoroutebuttonpg2close)
		m_fplautoroutebuttonpg2close->signal_clicked().connect(sigc::mem_fun(*this, &FPLAutoRouteDialog::pg1_close_clicked));
	if (m_fplautoroutebuttonpg2copy)
		m_fplautoroutebuttonpg2copy->signal_clicked().connect(sigc::mem_fun(*this, &FPLAutoRouteDialog::pg1_copy_clicked));
	if (m_fplautoroutetogglebuttonpg2run)
		m_connpg2run = m_fplautoroutetogglebuttonpg2run->signal_clicked().connect(sigc::mem_fun(*this, &FPLAutoRouteDialog::pg2_run_clicked));
	if (m_fplautoroutecheckbuttondepifr)
		m_fplautoroutecheckbuttondepifr->signal_toggled().connect(sigc::mem_fun(*this, &FPLAutoRouteDialog::ifr_toggled));
	if (m_fplautoroutecheckbuttondestifr)
		m_fplautoroutecheckbuttondestifr->signal_toggled().connect(sigc::mem_fun(*this, &FPLAutoRouteDialog::ifr_toggled));
	if (m_fplautoroutecheckbuttondepifr)
		m_fplautoroutecheckbuttondepifr->set_active(true);
	if (m_fplautoroutecheckbuttondestifr)
		m_fplautoroutecheckbuttondestifr->set_active(true);
	m_sidstarlimit[0][0] = m_sidstarlimit[1][0] = 20;
	m_sidstarlimit[0][1] = m_sidstarlimit[1][1] = 40;
	if (m_fplautoroutespinbuttonsidlimit)
		m_fplautoroutespinbuttonsidlimit->set_value(m_sidstarlimit[0][m_depifr]);
	if (m_fplautoroutespinbuttonstarlimit)
		m_fplautoroutespinbuttonstarlimit->set_value(m_sidstarlimit[1][m_destifr]);
	for (unsigned int i = 0; i < sizeof(m_ptchanged)/sizeof(m_ptchanged[0]); ++i)
		m_ptchanged[i] = false;
	if (m_fplautorouteentrydepicao) {
		m_fplautorouteentrydepicao->signal_changed().connect(sigc::bind(sigc::mem_fun(*this, &FPLAutoRouteDialog::pt_changed), 0));
		m_fplautorouteentrydepicao->signal_focus_out_event().connect(sigc::mem_fun(*this, &FPLAutoRouteDialog::pt_focusout));
	}
	if (m_fplautorouteentrydesticao) {
		m_fplautorouteentrydesticao->signal_changed().connect(sigc::bind(sigc::mem_fun(*this, &FPLAutoRouteDialog::pt_changed), 1));
		m_fplautorouteentrydesticao->signal_focus_out_event().connect(sigc::mem_fun(*this, &FPLAutoRouteDialog::pt_focusout));
	}
	if (m_fplautorouteentrysidicao) {
		m_fplautorouteentrysidicao->signal_changed().connect(sigc::bind(sigc::mem_fun(*this, &FPLAutoRouteDialog::pt_changed), 2));
		m_fplautorouteentrysidicao->signal_focus_out_event().connect(sigc::mem_fun(*this, &FPLAutoRouteDialog::pt_focusout));
	}
	if (m_fplautorouteentrystaricao) {
		m_fplautorouteentrystaricao->signal_changed().connect(sigc::bind(sigc::mem_fun(*this, &FPLAutoRouteDialog::pt_changed), 3));
		m_fplautorouteentrystaricao->signal_focus_out_event().connect(sigc::mem_fun(*this, &FPLAutoRouteDialog::pt_focusout));
	}
	if (m_fplautoroutecomboboxopttarget)
		m_fplautoroutecomboboxopttarget->signal_changed().connect(sigc::mem_fun(*this, &FPLAutoRouteDialog::opttarget_changed));
	opttarget_changed();
        m_tracerulesstore = Gtk::ListStore::create(m_rulescolumns);
	if (m_fplautoroutetreeviewtracerules) {
		m_fplautoroutetreeviewtracerules->set_model(m_tracerulesstore);
		int rule_col = m_fplautoroutetreeviewtracerules->append_column_editable(_("Trace Rule"), m_rulescolumns.m_rule) - 1;
		{
			Gtk::TreeViewColumn *col = m_fplautoroutetreeviewtracerules->get_column(rule_col);
			col->set_resizable(true);
                        col->set_sort_column(m_rulescolumns.m_rule);
                        col->set_expand(true);
		}
		m_fplautoroutetreeviewtracerules->columns_autosize();
                //m_fplautoroutetreeviewtracerules->set_headers_clickable();
		Glib::RefPtr<Gtk::TreeSelection> selection = m_fplautoroutetreeviewtracerules->get_selection();
                selection->set_mode(Gtk::SELECTION_MULTIPLE);
                m_tracerulesselchangeconn = selection->signal_changed().connect(sigc::mem_fun(*this, &FPLAutoRouteDialog::tracerules_selection_changed));
		m_fplautoroutetreeviewtracerules->connect_menunew(sigc::mem_fun(*this, &FPLAutoRouteDialog::tracerules_menu_new));
		m_fplautoroutetreeviewtracerules->connect_menudelete(sigc::mem_fun(*this, &FPLAutoRouteDialog::tracerules_menu_delete));
		tracerules_selection_changed();
	}
	if (m_fplautoroutebuttontracerulesadd)
		m_fplautoroutebuttontracerulesadd->signal_clicked().connect(sigc::mem_fun(*this, &FPLAutoRouteDialog::tracerules_menu_new));
	if (m_fplautoroutebuttontracerulesdelete)
		m_fplautoroutebuttontracerulesdelete->signal_clicked().connect(sigc::mem_fun(*this, &FPLAutoRouteDialog::tracerules_menu_delete));
        m_disablerulesstore = Gtk::ListStore::create(m_rulescolumns);
	if (m_fplautoroutetreeviewdisablerules) {
		m_fplautoroutetreeviewdisablerules->set_model(m_disablerulesstore);
                int rule_col = m_fplautoroutetreeviewdisablerules->append_column_editable(_("Disable Rule"), m_rulescolumns.m_rule) - 1;
		{
			Gtk::TreeViewColumn *col = m_fplautoroutetreeviewdisablerules->get_column(rule_col);
			col->set_resizable(true);
                        col->set_sort_column(m_rulescolumns.m_rule);
                        col->set_expand(true);
		}
		m_fplautoroutetreeviewdisablerules->columns_autosize();
                //m_fplautoroutetreeviewdisablerules->set_headers_clickable();
		Glib::RefPtr<Gtk::TreeSelection> selection = m_fplautoroutetreeviewdisablerules->get_selection();
                selection->set_mode(Gtk::SELECTION_MULTIPLE);
                m_disablerulesselchangeconn = selection->signal_changed().connect(sigc::mem_fun(*this, &FPLAutoRouteDialog::disablerules_selection_changed));
		m_fplautoroutetreeviewdisablerules->connect_menunew(sigc::mem_fun(*this, &FPLAutoRouteDialog::disablerules_menu_new));
		m_fplautoroutetreeviewdisablerules->connect_menudelete(sigc::mem_fun(*this, &FPLAutoRouteDialog::disablerules_menu_delete));
		disablerules_selection_changed();
	}
	if (m_fplautoroutebuttondisablerulesadd)
		m_fplautoroutebuttondisablerulesadd->signal_clicked().connect(sigc::mem_fun(*this, &FPLAutoRouteDialog::disablerules_menu_new));
	if (m_fplautoroutebuttondisablerulesdelete)
		m_fplautoroutebuttondisablerulesdelete->signal_clicked().connect(sigc::mem_fun(*this, &FPLAutoRouteDialog::disablerules_menu_delete));
        m_crossingstore = Gtk::ListStore::create(m_crossingcolumns);
	if (m_fplautoroutetreeviewcrossing) {
		m_fplautoroutetreeviewcrossing->set_model(m_crossingstore);
                int ident_col = m_fplautoroutetreeviewcrossing->append_column_editable(_("Ident"), m_crossingcolumns.m_ident) - 1;
                int name_col = m_fplautoroutetreeviewcrossing->append_column(_("Name"), m_crossingcolumns.m_name) - 1;
                int radius_col = m_fplautoroutetreeviewcrossing->append_column_numeric_editable(_("Radius"), m_crossingcolumns.m_radius, "%g") - 1;
		int minlevel_col = m_fplautoroutetreeviewcrossing->append_column_numeric_editable(_("Min FL"), m_crossingcolumns.m_minlevel, "%u") - 1;
		int maxlevel_col = m_fplautoroutetreeviewcrossing->append_column_numeric_editable(_("Max FL"), m_crossingcolumns.m_maxlevel, "%u") - 1;
		{
			Gtk::TreeViewColumn *col = m_fplautoroutetreeviewcrossing->get_column(ident_col);
			col->set_resizable(true);
                        col->set_sort_column(m_crossingcolumns.m_ident);
                        col->set_expand(true);
		}
		{
			Gtk::TreeViewColumn *col = m_fplautoroutetreeviewcrossing->get_column(name_col);
			col->set_resizable(true);
                        col->set_sort_column(m_crossingcolumns.m_name);
                        col->set_expand(true);
		}
		{
			Gtk::TreeViewColumn *col = m_fplautoroutetreeviewcrossing->get_column(radius_col);
			col->set_resizable(true);
                        col->set_sort_column(m_crossingcolumns.m_radius);
                        col->set_expand(false);
		}
		{
			Gtk::TreeViewColumn *col = m_fplautoroutetreeviewcrossing->get_column(minlevel_col);
			col->set_resizable(true);
                        col->set_sort_column(m_crossingcolumns.m_minlevel);
                        col->set_expand(false);
		}
		{
			Gtk::TreeViewColumn *col = m_fplautoroutetreeviewcrossing->get_column(maxlevel_col);
			col->set_resizable(true);
                        col->set_sort_column(m_crossingcolumns.m_maxlevel);
                        col->set_expand(false);
		}
		m_fplautoroutetreeviewcrossing->columns_autosize();
                //m_fplautoroutetreeviewcrossing->set_headers_clickable();
		Glib::RefPtr<Gtk::TreeSelection> selection = m_fplautoroutetreeviewcrossing->get_selection();
                selection->set_mode(Gtk::SELECTION_MULTIPLE);
                m_crossingselchangeconn = selection->signal_changed().connect(sigc::mem_fun(*this, &FPLAutoRouteDialog::crossing_selection_changed));
		m_fplautoroutetreeviewcrossing->connect_menunew(sigc::mem_fun(*this, &FPLAutoRouteDialog::crossing_menu_new));
		m_fplautoroutetreeviewcrossing->connect_menudelete(sigc::mem_fun(*this, &FPLAutoRouteDialog::crossing_menu_delete));
		crossing_selection_changed();
	}
	if (m_fplautoroutebuttoncrossingadd)
		m_fplautoroutebuttoncrossingadd->signal_clicked().connect(sigc::mem_fun(*this, &FPLAutoRouteDialog::crossing_menu_new));
	if (m_fplautoroutebuttoncrossingdelete)
		m_fplautoroutebuttoncrossingdelete->signal_clicked().connect(sigc::mem_fun(*this, &FPLAutoRouteDialog::crossing_menu_delete));
	m_crossingrowchangeconn = m_crossingstore->signal_row_changed().connect(sigc::mem_fun(*this, &FPLAutoRouteDialog::crossing_row_changed));
        m_airspacestore = Gtk::ListStore::create(m_airspacecolumns);
	if (m_fplautoroutetreeviewairspace) {
		m_fplautoroutetreeviewairspace->set_model(m_airspacestore);
                int ident_col = m_fplautoroutetreeviewairspace->append_column_editable(_("Ident"), m_airspacecolumns.m_ident) - 1;
		int type_col = m_fplautoroutetreeviewairspace->append_column_editable(_("Type"), m_airspacecolumns.m_type) - 1;
		int name_col = m_fplautoroutetreeviewairspace->append_column(_("Name"), m_airspacecolumns.m_name) - 1;
		int minlevel_col = m_fplautoroutetreeviewairspace->append_column_numeric_editable(_("Min FL"), m_airspacecolumns.m_minlevel, "%u") - 1;
		int maxlevel_col = m_fplautoroutetreeviewairspace->append_column_numeric_editable(_("Max FL"), m_airspacecolumns.m_maxlevel, "%u") - 1;
		int awylimit_col = m_fplautoroutetreeviewairspace->append_column_numeric_editable(_("Airway Limit"), m_airspacecolumns.m_awylimit, "%g") - 1;
                int dctlimit_col = m_fplautoroutetreeviewairspace->append_column_numeric_editable(_("DCT Limit"), m_airspacecolumns.m_dctlimit, "%g") - 1;
                int dctoffset_col = m_fplautoroutetreeviewairspace->append_column_numeric_editable(_("DCT Offset"), m_airspacecolumns.m_dctoffset, "%g") - 1;
                int dctscale_col = m_fplautoroutetreeviewairspace->append_column_numeric_editable(_("DCT Scale"), m_airspacecolumns.m_dctscale, "%g") - 1;
		{
			Gtk::TreeViewColumn *col = m_fplautoroutetreeviewairspace->get_column(ident_col);
			col->set_resizable(true);
                        col->set_sort_column(m_airspacecolumns.m_ident);
                        col->set_expand(true);
		}
		{
			Gtk::TreeViewColumn *col = m_fplautoroutetreeviewairspace->get_column(type_col);
			col->set_resizable(true);
                        col->set_sort_column(m_airspacecolumns.m_type);
                        col->set_expand(false);
		}
		{
			Gtk::TreeViewColumn *col = m_fplautoroutetreeviewairspace->get_column(name_col);
			col->set_resizable(true);
                        col->set_sort_column(m_airspacecolumns.m_name);
                        col->set_expand(false);
		}
		{
			Gtk::TreeViewColumn *col = m_fplautoroutetreeviewairspace->get_column(minlevel_col);
			col->set_resizable(true);
                        col->set_sort_column(m_airspacecolumns.m_minlevel);
                        col->set_expand(false);
		}
		{
			Gtk::TreeViewColumn *col = m_fplautoroutetreeviewairspace->get_column(maxlevel_col);
			col->set_resizable(true);
                        col->set_sort_column(m_airspacecolumns.m_maxlevel);
                        col->set_expand(false);
		}
		{
			Gtk::TreeViewColumn *col = m_fplautoroutetreeviewairspace->get_column(awylimit_col);
			col->set_resizable(true);
                        col->set_sort_column(m_airspacecolumns.m_awylimit);
                        col->set_expand(false);
		}
		{
			Gtk::TreeViewColumn *col = m_fplautoroutetreeviewairspace->get_column(dctlimit_col);
			col->set_resizable(true);
                        col->set_sort_column(m_airspacecolumns.m_dctlimit);
                        col->set_expand(false);
		}
		{
			Gtk::TreeViewColumn *col = m_fplautoroutetreeviewairspace->get_column(dctoffset_col);
			col->set_resizable(true);
                        col->set_sort_column(m_airspacecolumns.m_dctoffset);
                        col->set_expand(false);
		}
		{
			Gtk::TreeViewColumn *col = m_fplautoroutetreeviewairspace->get_column(dctscale_col);
			col->set_resizable(true);
                        col->set_sort_column(m_airspacecolumns.m_dctscale);
                        col->set_expand(false);
		}
		m_fplautoroutetreeviewairspace->columns_autosize();
                //m_fplautoroutetreeviewairspace->set_headers_clickable();
		Glib::RefPtr<Gtk::TreeSelection> selection = m_fplautoroutetreeviewairspace->get_selection();
                selection->set_mode(Gtk::SELECTION_MULTIPLE);
                m_airspaceselchangeconn = selection->signal_changed().connect(sigc::mem_fun(*this, &FPLAutoRouteDialog::airspace_selection_changed));
		m_fplautoroutetreeviewairspace->connect_menunew(sigc::mem_fun(*this, &FPLAutoRouteDialog::airspace_menu_new));
		m_fplautoroutetreeviewairspace->connect_menudelete(sigc::mem_fun(*this, &FPLAutoRouteDialog::airspace_menu_delete));
		airspace_selection_changed();
	}
	if (m_fplautoroutebuttonairspaceadd)
		m_fplautoroutebuttonairspaceadd->signal_clicked().connect(sigc::mem_fun(*this, &FPLAutoRouteDialog::airspace_menu_new));
	if (m_fplautoroutebuttonairspacedelete)
		m_fplautoroutebuttonairspacedelete->signal_clicked().connect(sigc::mem_fun(*this, &FPLAutoRouteDialog::airspace_menu_delete));
	m_airspacerowchangeconn = m_airspacestore->signal_row_changed().connect(sigc::mem_fun(*this, &FPLAutoRouteDialog::airspace_row_changed));
	if (m_fplautoroutedrawingarea)
                m_fplautoroutedrawingarea->set_route(m_proxy.get_fplan());
	m_proxy.signal_recvcmd().connect(sigc::mem_fun(*this, &FPLAutoRouteDialog::recvcmd));
	m_sidend.set_invalid();
	m_starstart.set_invalid();
        set_flags(MapRenderer::drawflags_route | MapRenderer::drawflags_route_labels | MapRenderer::drawflags_navaids |
		  /*MapRenderer::drawflags_waypoints | */MapRenderer::drawflags_airports);
	m_routesstore = Gtk::ListStore::create(m_routescolumns);
	if (m_fplautoroutetreeviewpg2routes) {
		m_fplautoroutetreeviewpg2routes->set_model(m_routesstore);
		Gtk::CellRendererText *time_renderer = Gtk::manage(new Gtk::CellRendererText());
		int time_col = m_fplautoroutetreeviewpg2routes->append_column(_("Time"), *time_renderer) - 1;
		Gtk::TreeViewColumn *time_column = m_fplautoroutetreeviewpg2routes->get_column(time_col);
		if (time_column) {
			time_column->add_attribute(*time_renderer, "text", m_routescolumns.m_time);
			time_column->set_resizable(true);
                        time_column->set_sort_column(m_routescolumns.m_time);
                        time_column->set_expand(false);
		}
		Gtk::CellRendererText *fuel_renderer = Gtk::manage(new Gtk::CellRendererText());
		int fuel_col = m_fplautoroutetreeviewpg2routes->append_column(_("Fuel"), *fuel_renderer) - 1;
		Gtk::TreeViewColumn *fuel_column = m_fplautoroutetreeviewpg2routes->get_column(fuel_col);
		if (fuel_column) {
			fuel_column->add_attribute(*fuel_renderer, "text", m_routescolumns.m_fuel);
			fuel_column->set_resizable(true);
                        fuel_column->set_sort_column(m_routescolumns.m_fuel);
                        fuel_column->set_expand(false);
		}
		Gtk::CellRendererText *route_renderer = Gtk::manage(new Gtk::CellRendererText());
		int route_col = m_fplautoroutetreeviewpg2routes->append_column(_("Route"), *route_renderer) - 1;
		Gtk::TreeViewColumn *route_column = m_fplautoroutetreeviewpg2routes->get_column(route_col);
		if (route_column) {
			route_column->add_attribute(*route_renderer, "text", m_routescolumns.m_routetxt);
			route_column->set_resizable(true);
                        route_column->set_sort_column(m_routescolumns.m_routetxt);
                        route_column->set_expand(true);
		}
		Gtk::CellRendererText *flags_renderer = Gtk::manage(new Gtk::CellRendererText());
		int flags_col = m_fplautoroutetreeviewpg2routes->append_column(_("Flags"), *flags_renderer) - 1;
		Gtk::TreeViewColumn *flags_column = m_fplautoroutetreeviewpg2routes->get_column(flags_col);
		if (flags_column) {
			flags_column->add_attribute(*flags_renderer, "text", m_routescolumns.m_flags);
			flags_column->set_resizable(true);
                        flags_column->set_sort_column(m_routescolumns.m_flags);
                        flags_column->set_expand(false);
		}
		m_fplautoroutetreeviewpg2routes->columns_autosize();
                //m_fplautoroutetreeviewpg2routes->set_headers_clickable();
		Glib::RefPtr<Gtk::TreeSelection> selection = m_fplautoroutetreeviewpg2routes->get_selection();
                selection->set_mode(Gtk::SELECTION_SINGLE);
                m_routesselchangeconn = selection->signal_changed().connect(sigc::mem_fun(*this, &FPLAutoRouteDialog::routes_selection_changed));
		routes_selection_changed();
	}
	if (m_fplautorouteframepg2routes)
		m_fplautorouteframepg2routes->set_visible(false);
	if (m_fplautoroutetextviewpg2graph) {
		Glib::RefPtr<Gtk::TextBuffer> buf(m_fplautoroutetextviewpg2graph->get_buffer());
		if (buf) {
			m_txtgraphfpl = buf->create_tag();
			m_txtgraphrule = buf->create_tag();
			m_txtgraphruledesc = buf->create_tag();
			m_txtgraphruleoprgoal = buf->create_tag();
			m_txtgraphchange = buf->create_tag();
			m_txtgraphfpl->property_weight() = 600;
			m_txtgraphfpl->property_weight_set() = true;
			m_txtgraphfpl->property_style() = Pango::STYLE_NORMAL;
			m_txtgraphfpl->property_style_set() = true;
			m_txtgraphrule->property_weight() = 400;
			m_txtgraphrule->property_weight_set() = true;
			m_txtgraphrule->property_style() = Pango::STYLE_NORMAL;
			m_txtgraphrule->property_style_set() = true;
			m_txtgraphruledesc->property_weight() = 300;
			m_txtgraphruledesc->property_weight_set() = true;
			m_txtgraphruledesc->property_style() = Pango::STYLE_NORMAL;
			m_txtgraphruledesc->property_style_set() = true;
			m_txtgraphruleoprgoal->property_weight() = 300;
			m_txtgraphruleoprgoal->property_weight_set() = true;
			m_txtgraphruleoprgoal->property_style() = Pango::STYLE_ITALIC;
			m_txtgraphruleoprgoal->property_style_set() = true;
			m_txtgraphchange->property_weight() = 400;
			m_txtgraphchange->property_weight_set() = true;
			m_txtgraphchange->property_style() = Pango::STYLE_ITALIC;
			m_txtgraphchange->property_style_set() = true;
		}
	}
}

FlightDeckWindow::FPLAutoRouteDialog::~FPLAutoRouteDialog()
{
}

bool FlightDeckWindow::FPLAutoRouteDialog::on_delete_event(GdkEventAny* event)
{
        close();
        return true;
}

void FlightDeckWindow::FPLAutoRouteDialog::shift_clicked(void)
{
	m_shiftstate = (m_shiftstate == shiftstate_once) ? shiftstate_none : shiftstate_once;
	set_shiftstate();
}

void FlightDeckWindow::FPLAutoRouteDialog::capslock_clicked(void)
{
	m_shiftstate = (m_shiftstate == shiftstate_lock) ? shiftstate_none : shiftstate_lock;
	set_shiftstate();
}

void FlightDeckWindow::FPLAutoRouteDialog::set_shiftstate(void)
{
	m_connshiftleft.block();
	m_connshiftright.block();
	m_conncapslock.block();
	if (m_fplautoroutetogglebuttonshiftleft)
		m_fplautoroutetogglebuttonshiftleft->set_active(m_shiftstate == shiftstate_once);
	if (m_fplautoroutetogglebuttonshiftright)
		m_fplautoroutetogglebuttonshiftright->set_active(m_shiftstate == shiftstate_once);
	if (m_fplautoroutetogglebuttoncapslock)
		m_fplautoroutetogglebuttoncapslock->set_active(m_shiftstate == shiftstate_lock);
	m_connshiftleft.unblock();
	m_connshiftright.unblock();
	m_conncapslock.unblock();
}

void FlightDeckWindow::FPLAutoRouteDialog::key_clicked(unsigned int i)
{
	if (i >= nrkeybuttons)
		return;
	guint keyval((m_shiftstate != shiftstate_none) ? keybuttons[i].key_shift : keybuttons[i].key);
	if (false)
		std::cerr << "key_clicked: " << keyval << std::endl;
        Gtk::Widget *w(get_focus());
        if (!w)
                return;
	KeyboardDialog::send_key(w, ((m_shiftstate == shiftstate_once) ? GDK_SHIFT_MASK : 0) |
				 ((m_shiftstate == shiftstate_lock) ? GDK_LOCK_MASK : 0), keyval, keybuttons[i].hwkeycode);
	if (m_shiftstate == shiftstate_once) {
		m_shiftstate = shiftstate_none;
		set_shiftstate();
	}
}

void FlightDeckWindow::FPLAutoRouteDialog::set_flags(MapRenderer::DrawFlags flags)
{
        m_flags = flags;
        if (m_fplautoroutedrawingarea)
                *m_fplautoroutedrawingarea = m_flags;
}

void FlightDeckWindow::FPLAutoRouteDialog::set_map_scale(float scale)
{
        m_mapscale = scale;
        if (!m_fplautoroutedrawingarea)
                return;
	m_fplautoroutedrawingarea->set_map_scale(m_mapscale);
	m_mapscale = m_fplautoroutedrawingarea->get_map_scale();
}

void FlightDeckWindow::FPLAutoRouteDialog::today_clicked(void)
{
	if (!m_fplautoroutecalendar)
		return;
        Glib::Date d;
        d.set_time_current();
        m_fplautoroutecalendar->select_month(d.get_month() - 1, d.get_year());
        m_fplautoroutecalendar->select_day(d.get_day());
}

void FlightDeckWindow::FPLAutoRouteDialog::tfr_toggled(void)
{
#ifdef HAVE_JSONCPP
	if (m_childsockaddr) {
		Json::Value root;
		root["session"] = m_childsession;
		Json::Value cmds;
		{
			Json::Value cmd;
			cmd["cmdname"] = "tfr";
			if (m_fplautoroutecheckbuttontfr)
				cmd["enabled"] = m_fplautoroutecheckbuttontfr->get_active();
			if (m_tracerulesstore) {
				std::string txt;
				for (Gtk::TreeIter iter(m_tracerulesstore->children().begin()); iter; ++iter) {
					Gtk::TreeModel::Row row(*iter);
					std::string txt1((Glib::ustring)row[m_rulescolumns.m_rule]);
					while (!txt1.empty() && std::isspace(txt1[txt1.size()-1]))
						txt1.erase(txt1.size() - 1, 1);
					while (!txt1.empty() && std::isspace(txt1[0]))
						txt1.erase(0, 1);
					if (txt1.empty() || txt1 == "?")
						continue;
					if (!txt.empty())
						txt += ",";
					txt += txt1;
				}
				cmd["trace"] = txt;
			}
			if (m_disablerulesstore) {
				std::string txt;
				for (Gtk::TreeIter iter(m_disablerulesstore->children().begin()); iter; ++iter) {
					Gtk::TreeModel::Row row(*iter);
					std::string txt1((Glib::ustring)row[m_rulescolumns.m_rule]);
					while (!txt1.empty() && std::isspace(txt1[txt1.size()-1]))
						txt1.erase(txt1.size() - 1, 1);
					while (!txt1.empty() && std::isspace(txt1[0]))
						txt1.erase(0, 1);
					if (txt1.empty() || txt1 == "?")
						continue;
					if (!txt.empty())
						txt += ",";
					txt += txt1;
				}
				cmd["disable"] = txt;
			}
			cmds.append(cmd);
		}
#ifndef G_OS_WIN32
		{
			Json::Value cmd;
			cmd["cmdname"] = "preload";
			cmds.append(cmd);
		}
#endif
		root["cmds"] = cmds;
		sendjson(root);
		return;
	}
#endif
	{
		CFMUAutorouteProxy::Command cmd("tfr");
		if (m_fplautoroutecheckbuttontfr)
			cmd.set_option("enabled", (int)m_fplautoroutecheckbuttontfr->get_active());
		if (m_tracerulesstore) {
			std::string txt;
			for (Gtk::TreeIter iter(m_tracerulesstore->children().begin()); iter; ++iter) {
				Gtk::TreeModel::Row row(*iter);
				std::string txt1((Glib::ustring)row[m_rulescolumns.m_rule]);
				while (!txt1.empty() && std::isspace(txt1[txt1.size()-1]))
					txt1.erase(txt1.size() - 1, 1);
				while (!txt1.empty() && std::isspace(txt1[0]))
					txt1.erase(0, 1);
				if (txt1.empty() || txt1 == "?")
					continue;
				if (!txt.empty())
					txt += ",";
				txt += txt1;
			}
			cmd.set_option("trace", txt, true);
		}
		if (m_disablerulesstore) {
			std::string txt;
			for (Gtk::TreeIter iter(m_disablerulesstore->children().begin()); iter; ++iter) {
				Gtk::TreeModel::Row row(*iter);
				std::string txt1((Glib::ustring)row[m_rulescolumns.m_rule]);
				while (!txt1.empty() && std::isspace(txt1[txt1.size()-1]))
					txt1.erase(txt1.size() - 1, 1);
				while (!txt1.empty() && std::isspace(txt1[0]))
					txt1.erase(0, 1);
				if (txt1.empty() || txt1 == "?")
					continue;
				if (!txt.empty())
					txt += ",";
				txt += txt1;
			}
			cmd.set_option("disable", txt, true);
		}
		m_proxy.sendcmd(cmd);
	}
#ifndef G_OS_WIN32
	{
		CFMUAutorouteProxy::Command cmd("preload");
		m_proxy.sendcmd(cmd);
	}
#endif
}

void FlightDeckWindow::FPLAutoRouteDialog::wind_toggled(void)
{
#ifdef HAVE_JSONCPP
	if (m_childsockaddr) {
		Json::Value root;
		root["session"] = m_childsession;
		Json::Value cmds;
		{
			Json::Value cmd;
			cmd["cmdname"] = "atmosphere";
			if (m_fplautoroutecheckbuttonwind)
				cmd["wind"] = m_fplautoroutecheckbuttonwind->get_active();
			cmds.append(cmd);
		}
#ifndef G_OS_WIN32
		{
			Json::Value cmd;
			cmd["cmdname"] = "preload";
			cmds.append(cmd);
		}
#endif
		root["cmds"] = cmds;
		sendjson(root);
		return;
	}
#endif
	{
		CFMUAutorouteProxy::Command cmd("atmosphere");
		if (m_fplautoroutecheckbuttonwind)
			cmd.set_option("wind", (int)m_fplautoroutecheckbuttonwind->get_active());
		m_proxy.sendcmd(cmd);
	}
#ifndef G_OS_WIN32
	{
		CFMUAutorouteProxy::Command cmd("preload");
		m_proxy.sendcmd(cmd);
	}
#endif
}

void FlightDeckWindow::FPLAutoRouteDialog::tracerules_selection_changed(void)
{
	Glib::RefPtr<Gtk::TreeSelection> selection(m_fplautoroutetreeviewtracerules->get_selection());
	if (!selection)
		return;
	if (m_fplautoroutetreeviewtracerules)
		m_fplautoroutetreeviewtracerules->set_sensitive_delete(!!selection->count_selected_rows());
	if (m_fplautoroutebuttontracerulesdelete)
		m_fplautoroutebuttontracerulesdelete->set_sensitive(!!selection->count_selected_rows());
}

void FlightDeckWindow::FPLAutoRouteDialog::tracerules_menu_new(void)
{
	if (!m_fplautoroutetreeviewtracerules)
		return;
	Gtk::TreeIter iter(m_tracerulesstore->append());
	Gtk::TreeModel::Row row(*iter);
	row[m_rulescolumns.m_rule] = "?";
        Glib::RefPtr<Gtk::TreeSelection> selection(m_fplautoroutetreeviewtracerules->get_selection());
	if (!selection)
		return;
	selection->unselect_all();
	selection->select(iter);
}

void FlightDeckWindow::FPLAutoRouteDialog::tracerules_menu_delete(void)
{
	if (!m_fplautoroutetreeviewtracerules)
		return;
        Glib::RefPtr<Gtk::TreeSelection> selection(m_fplautoroutetreeviewtracerules->get_selection());
	if (!selection)
		return;
	if (!m_tracerulesstore)
		return;
        std::vector<Gtk::TreePath> selrows(selection->get_selected_rows());
        for (std::vector<Gtk::TreePath>::const_iterator si(selrows.begin()), se(selrows.end()); si != se; ++si) {
                Gtk::TreeIter iter(m_tracerulesstore->get_iter(*si));
                if (!iter)
                        continue;
		m_tracerulesstore->erase(iter);
	}
}

void FlightDeckWindow::FPLAutoRouteDialog::disablerules_selection_changed(void)
{
        Glib::RefPtr<Gtk::TreeSelection> selection(m_fplautoroutetreeviewdisablerules->get_selection());
	if (!selection)
		return;
	if (m_fplautoroutetreeviewdisablerules)
		m_fplautoroutetreeviewdisablerules->set_sensitive_delete(!!selection->count_selected_rows());
	if (m_fplautoroutebuttondisablerulesdelete)
		m_fplautoroutebuttondisablerulesdelete->set_sensitive(!!selection->count_selected_rows());
}

void FlightDeckWindow::FPLAutoRouteDialog::disablerules_menu_new(void)
{
	if (!m_fplautoroutetreeviewdisablerules)
		return;
	Gtk::TreeIter iter(m_disablerulesstore->append());
	Gtk::TreeModel::Row row(*iter);
	row[m_rulescolumns.m_rule] = "?";
        Glib::RefPtr<Gtk::TreeSelection> selection(m_fplautoroutetreeviewdisablerules->get_selection());
	if (!selection)
		return;
	selection->unselect_all();
	selection->select(iter);
}

void FlightDeckWindow::FPLAutoRouteDialog::disablerules_menu_delete(void)
{
	if (!m_fplautoroutetreeviewdisablerules)
		return;
        Glib::RefPtr<Gtk::TreeSelection> selection(m_fplautoroutetreeviewdisablerules->get_selection());
	if (!selection)
		return;
	if (!m_disablerulesstore)
		return;
        std::vector<Gtk::TreePath> selrows(selection->get_selected_rows());
        for (std::vector<Gtk::TreePath>::const_iterator si(selrows.begin()), se(selrows.end()); si != se; ++si) {
                Gtk::TreeIter iter(m_disablerulesstore->get_iter(*si));
                if (!iter)
                        continue;
		m_disablerulesstore->erase(iter);
	}
}

void FlightDeckWindow::FPLAutoRouteDialog::crossing_selection_changed(void)
{
        Glib::RefPtr<Gtk::TreeSelection> selection(m_fplautoroutetreeviewcrossing->get_selection());
	if (!selection)
		return;
	if (m_fplautoroutetreeviewcrossing)
		m_fplautoroutetreeviewcrossing->set_sensitive_delete(!!selection->count_selected_rows());
	if (m_fplautoroutebuttoncrossingdelete)
		m_fplautoroutebuttoncrossingdelete->set_sensitive(!!selection->count_selected_rows());
}

void FlightDeckWindow::FPLAutoRouteDialog::crossing_menu_new(void)
{
	if (!m_fplautoroutetreeviewcrossing)
		return;
	m_crossingrowchangeconn.block();
	Gtk::TreeIter iter(m_crossingstore->append());
	Gtk::TreeModel::Row row(*iter);
	{
		Point pt;
		pt.set_invalid();
		row[m_crossingcolumns.m_coord] = pt;
	}
	row[m_crossingcolumns.m_ident] = "?";
	row[m_crossingcolumns.m_name] = "";
	row[m_crossingcolumns.m_radius] = 0;
	row[m_crossingcolumns.m_minlevel] = 0;
	row[m_crossingcolumns.m_maxlevel] = 600;
	m_crossingrowchangeconn.unblock();
	Glib::RefPtr<Gtk::TreeSelection> selection(m_fplautoroutetreeviewcrossing->get_selection());
	if (!selection)
		return;
	selection->unselect_all();
	selection->select(iter);
}

void FlightDeckWindow::FPLAutoRouteDialog::crossing_menu_delete(void)
{
	if (!m_fplautoroutetreeviewcrossing)
		return;
        Glib::RefPtr<Gtk::TreeSelection> selection(m_fplautoroutetreeviewcrossing->get_selection());
	if (!selection)
		return;
	if (!m_crossingstore)
		return;
        std::vector<Gtk::TreePath> selrows(selection->get_selected_rows());
        for (std::vector<Gtk::TreePath>::const_iterator si(selrows.begin()), se(selrows.end()); si != se; ++si) {
                Gtk::TreeIter iter(m_crossingstore->get_iter(*si));
                if (!iter)
                        continue;
		m_crossingstore->erase(iter);
	}
}

void FlightDeckWindow::FPLAutoRouteDialog::crossing_row_changed(const Gtk::TreeModel::Path& path, const Gtk::TreeModel::iterator& iter)
{
	if (!iter)
		return;
	if (false)
		std::cerr << "crossing row change: " << path.to_string() << " back " << path.back() << std::endl;
	Gtk::TreeModel::Row row(*iter);
	Glib::ustring ptname(row[m_crossingcolumns.m_ident]);
	Point pt;
	pt.set_invalid();
	if (m_departure.is_valid() && m_destination.is_valid())
		pt = m_departure.get_coord().halfway(m_destination.get_coord());
	else if (m_departure.is_valid())
		pt = m_departure.get_coord();
	else if (m_departure.is_valid())
		pt = m_departure.get_coord();
	if (ptname.empty()) {
		pt.set_invalid();
		ptname.clear();
	} else {
		Glib::ustring ptident;
		IcaoFlightPlan::FindCoord f(*m_sensors->get_engine());
		if (f.find(ptname, ptname, IcaoFlightPlan::FindCoord::flag_navaid | IcaoFlightPlan::FindCoord::flag_waypoint, pt)) {
			pt.set_invalid();
			ptname.clear();
			NavaidsDb::element_t nav;
			WaypointsDb::element_t wpt;
			if (f.get_navaid(nav)) {
				pt = nav.get_coord();
				ptname = nav.get_navaid_typename() + " " + nav.get_name();
				ptident = nav.get_icao();
			} else if (f.get_waypoint(wpt)) {
				pt = wpt.get_coord();
				ptname = "INT " + wpt.get_name();
				ptident = wpt.get_name();
			}
		} else {
			pt.set_invalid();
			ptname.clear();
		}
		m_crossingrowchangeconn.block();
		row[m_crossingcolumns.m_coord] = pt;
		row[m_crossingcolumns.m_name] = ptname;
		if (!ptident.empty())
			row[m_crossingcolumns.m_ident] = ptident;
		m_crossingrowchangeconn.unblock();
	}
}

void FlightDeckWindow::FPLAutoRouteDialog::airspace_selection_changed(void)
{
        Glib::RefPtr<Gtk::TreeSelection> selection(m_fplautoroutetreeviewairspace->get_selection());
	if (!selection)
		return;
	if (m_fplautoroutetreeviewairspace)
		m_fplautoroutetreeviewairspace->set_sensitive_delete(!!selection->count_selected_rows());
	if (m_fplautoroutebuttonairspacedelete)
		m_fplautoroutebuttonairspacedelete->set_sensitive(!!selection->count_selected_rows());
}

void FlightDeckWindow::FPLAutoRouteDialog::airspace_menu_new(void)
{
	if (!m_fplautoroutetreeviewairspace)
		return;
	m_airspacerowchangeconn.block();
	Gtk::TreeIter iter(m_airspacestore->append());
	Gtk::TreeModel::Row row(*iter);
	row[m_airspacecolumns.m_ident] = "?";
	row[m_airspacecolumns.m_type] = "";
	row[m_airspacecolumns.m_name] = "";
	{
		Point pt;
		pt.set_invalid();
		row[m_airspacecolumns.m_coord] = pt;
	}
	row[m_airspacecolumns.m_minlevel] = 0;
	row[m_airspacecolumns.m_maxlevel] = 600;
	row[m_airspacecolumns.m_awylimit] = 9999;
	row[m_airspacecolumns.m_dctlimit] = 9999;
	row[m_airspacecolumns.m_dctoffset] = 0;
	row[m_airspacecolumns.m_dctscale] = 1;
	m_airspacerowchangeconn.unblock();
	Glib::RefPtr<Gtk::TreeSelection> selection(m_fplautoroutetreeviewairspace->get_selection());
	if (!selection)
		return;
	selection->unselect_all();
	selection->select(iter);
}

void FlightDeckWindow::FPLAutoRouteDialog::airspace_menu_delete(void)
{
	if (!m_fplautoroutetreeviewairspace)
		return;
        Glib::RefPtr<Gtk::TreeSelection> selection(m_fplautoroutetreeviewairspace->get_selection());
	if (!selection)
		return;
	if (!m_airspacestore)
		return;
        std::vector<Gtk::TreePath> selrows(selection->get_selected_rows());
        for (std::vector<Gtk::TreePath>::const_iterator si(selrows.begin()), se(selrows.end()); si != se; ++si) {
                Gtk::TreeIter iter(m_airspacestore->get_iter(*si));
                if (!iter)
                        continue;
		m_airspacestore->erase(iter);
	}
}

void FlightDeckWindow::FPLAutoRouteDialog::airspace_row_changed(const Gtk::TreeModel::Path& path, const Gtk::TreeModel::iterator& iter)
{
	if (!iter)
		return;
	if (false)
		std::cerr << "airspace row change: " << path.to_string() << " back " << path.back() << std::endl;
	Gtk::TreeModel::Row row(*iter);
	Glib::ustring ptident(row[m_airspacecolumns.m_ident]);
	Glib::ustring pttype(row[m_airspacecolumns.m_type]);
	Glib::ustring ptname;
	Point pt;
	pt.set_invalid();
	if (!ptident.empty() && (pttype.empty() || !std::isdigit(pttype[0]))) {
		FindAirspace f(*m_sensors->get_engine());
		if (f.find(ptident)) {
			pttype = pttype.uppercase();
			Engine::AirspaceResult::elementvector_t ev(f.get_airspaces());
			for (Engine::AirspaceResult::elementvector_t::iterator i(ev.begin()), e(ev.end()); i != e; ) {
				if (!pttype.empty() && i->get_class_string() != pttype) {
					i = ev.erase(i);
					e = ev.end();
					continue;
				}
				++i;
			}
			if (ev.size() == 1) {
				ptident = ev[0].get_icao();
				pttype = ev[0].get_class_string();
				ptname = "Airspace " + ptident + "/" + pttype;
				m_airspacerowchangeconn.block();
				row[m_airspacecolumns.m_ident] = ptident;
				row[m_airspacecolumns.m_type] = pttype;
				row[m_airspacecolumns.m_name] = ptname;
				row[m_airspacecolumns.m_coord] = pt;
				m_airspacerowchangeconn.unblock();
				return;
			}
		}
	}
	if (!ptident.empty() && (pttype.empty() || std::isdigit(pttype[0]))) {
		if (m_departure.is_valid() && m_destination.is_valid())
			pt = m_departure.get_coord().halfway(m_destination.get_coord());
		else if (m_departure.is_valid())
			pt = m_departure.get_coord();
		else if (m_departure.is_valid())
			pt = m_departure.get_coord();
		IcaoFlightPlan::FindCoord f(*m_sensors->get_engine());
		if (f.find(ptident, ptident, IcaoFlightPlan::FindCoord::flag_navaid | IcaoFlightPlan::FindCoord::flag_waypoint, pt)) {
			pt.set_invalid();
			NavaidsDb::element_t nav;
			WaypointsDb::element_t wpt;
			if (f.get_navaid(nav)) {
				pt = nav.get_coord();
				ptname = nav.get_navaid_typename() + " " + nav.get_name();
				ptident = nav.get_icao();
			} else if (f.get_waypoint(wpt)) {
				pt = wpt.get_coord();
				ptname = "INT " + wpt.get_name();
				ptident = wpt.get_name();
			}
			m_airspacerowchangeconn.block();
			row[m_airspacecolumns.m_ident] = ptident;
			row[m_airspacecolumns.m_name] = ptname;
			row[m_airspacecolumns.m_coord] = pt;
			m_airspacerowchangeconn.unblock();
			return;
		}
		pt.set_invalid();
	}
	m_airspacerowchangeconn.block();
	row[m_airspacecolumns.m_name] = "";
	row[m_airspacecolumns.m_coord] = pt;
	m_airspacerowchangeconn.unblock();
}

void FlightDeckWindow::FPLAutoRouteDialog::routes_selection_changed(void)
{
        Glib::RefPtr<Gtk::TreeSelection> selection(m_fplautoroutetreeviewpg2routes->get_selection());
	if (!selection)
		return;
	std::vector<Gtk::TreePath> selrows(selection->get_selected_rows());
	for (std::vector<Gtk::TreePath>::const_iterator si(selrows.begin()), se(selrows.end()); si != se; ++si) {
		Gtk::TreeIter iter(m_routesstore->get_iter(*si));
		if (!iter)
			continue;
		Gtk::TreeModel::Row row(*iter);
		if (m_fplautoroutetextviewpg2route) {
			Glib::RefPtr<Gtk::TextBuffer> buf(m_fplautoroutetextviewpg2route->get_buffer());
			if (buf)
				buf->set_text(row[m_routescolumns.m_routetxt]);
		}
		m_proxy.set_fplan(row[m_routescolumns.m_route]);
		if (m_fplautorouteentrypg2routedist)
			m_fplautorouteentrypg2routedist->set_text(Conversions::dist_str(m_proxy.get_fplan().total_distance_nmi_dbl()));
		if (m_fplautorouteentrypg2routetime) {
			std::ostringstream oss;
			if (row[m_routescolumns.m_routetime])
				oss << Conversions::time_str(row[m_routescolumns.m_routetime]);
			if (row[m_routescolumns.m_routetime] && row[m_routescolumns.m_routezerowindtime])
				oss << ' ';
			if (row[m_routescolumns.m_routezerowindtime])
				oss << '[' << Conversions::time_str(row[m_routescolumns.m_routezerowindtime]) << ']';
			m_fplautorouteentrypg2routetime->set_text(oss.str());
		}
		if (m_fplautorouteentrypg2routefuel) {
			std::ostringstream oss;
			if (!std::isnan((double)row[m_routescolumns.m_routefuel]))
				oss << Conversions::dist_str(row[m_routescolumns.m_routefuel]);
			if (!std::isnan((double)row[m_routescolumns.m_routefuel]) && !std::isnan((double)row[m_routescolumns.m_routezerowindfuel]))
				oss << ' ';
			if (!std::isnan((double)row[m_routescolumns.m_routezerowindfuel]))
				oss << '[' << Conversions::dist_str(row[m_routescolumns.m_routezerowindfuel]) << ']';
			m_fplautorouteentrypg2routefuel->set_text(oss.str());
		}
	}
	if (m_fplautoroutedrawingarea)
		m_fplautoroutedrawingarea->redraw_route();
	//m_routesselchangeconn.block();
	//m_routesselchangeconn.unblock();
}

void FlightDeckWindow::FPLAutoRouteDialog::pg2_zoomin_clicked(void)
{
        if (!m_fplautoroutedrawingarea)
                return;
	m_fplautoroutedrawingarea->zoom_in();
	m_mapscale = m_fplautoroutedrawingarea->get_map_scale();
}

void FlightDeckWindow::FPLAutoRouteDialog::pg2_zoomout_clicked(void)
{
        if (!m_fplautoroutedrawingarea)
                return;
	m_fplautoroutedrawingarea->zoom_out();
	m_mapscale = m_fplautoroutedrawingarea->get_map_scale();
}

void FlightDeckWindow::FPLAutoRouteDialog::open(Sensors *sensors)
{
	m_sensors = sensors;
	if (!m_sensors) {
		close();
		return;
	}
#ifdef HAVE_JSONCPP
	if (!m_childsockaddr && m_sensors->get_configfile().has_key(cfggroup_mainwindow, "autorouteserver")) {
#ifdef G_OS_UNIX
		std::string addr(m_sensors->get_configfile().get_string(cfggroup_mainwindow, "autorouteserver"));
		if (!addr.empty())
			m_childsockaddr = Gio::UnixSocketAddress::create(addr, Gio::UNIX_SOCKET_ADDRESS_PATH);
#else
		int addr(m_sensors->get_configfile().get_integer(cfggroup_mainwindow, "autorouteserver"));
		if (addr)
			m_childsockaddr = Gio::InetSocketAddress::create(Gio::InetAddress::create_loopback(Gio::SOCKET_FAMILY_IPV4), addr);
#endif
	}
#endif
	if (m_fplautoroutedrawingarea)
                m_fplautoroutedrawingarea->set_engine(*m_sensors->get_engine());
	m_departure = m_destination = AirportsDb::Airport();
	m_sidend = m_starstart = Point::invalid;
	if (m_fplautoroutecheckbuttondepifr)
		m_fplautoroutecheckbuttondepifr->set_active(true);
	if (m_fplautoroutecheckbuttondestifr)
		m_fplautoroutecheckbuttondestifr->set_active(true);
	if (m_fplautorouteentrydestname)
		m_fplautorouteentrydestname->set_text("");
	if (m_fplautorouteentrydepname)
		m_fplautorouteentrydepname->set_text("");
	if (m_fplautorouteentrysidname)
		m_fplautorouteentrysidname->set_text("");
	if (m_fplautorouteentrystarname)
		m_fplautorouteentrystarname->set_text("");
	if (!m_fplautoroutenotebook) {
		close();
		return;
	}
	if (m_fplautorouteentrydepicao)
		m_fplautorouteentrydepicao->set_text("");
	if (m_fplautorouteentrydepname)
		m_fplautorouteentrydepname->set_text("");
	if (m_fplautorouteentrydesticao)
		m_fplautorouteentrydesticao->set_text("");
	if (m_fplautorouteentrydestname)
		m_fplautorouteentrydestname->set_text("");
	if (m_fplautoroutecheckbuttondepifr)
		m_fplautoroutecheckbuttondepifr->set_active(true);
	if (m_fplautoroutecheckbuttondestifr)
		m_fplautoroutecheckbuttondestifr->set_active(true);
	if (m_fplautorouteentrysidicao)
		m_fplautorouteentrysidicao->set_text("");
	if (m_fplautorouteentrysidname)
		m_fplautorouteentrysidname->set_text("");
	if (m_fplautorouteentrystaricao)
		m_fplautorouteentrystaricao->set_text("");
	if (m_fplautorouteentrystarname)
		m_fplautorouteentrystarname->set_text("");
	m_fplautoroutenotebook->set_current_page(0);
	{
		Glib::DateTime dt(Glib::DateTime::create_now_utc());
		{
			int x(60 - dt.get_minute());
			if (x < 60)
				dt = dt.add_minutes(x);
		}
		dt = dt.add_hours(4);
		if (m_fplautoroutecalendar) {
			m_fplautoroutecalendar->select_month(dt.get_month() - 1, dt.get_year());
			m_fplautoroutecalendar->select_day(dt.get_day_of_month());
		}
		if (m_fplautoroutespinbuttonhour)
			m_fplautoroutespinbuttonhour->set_value(dt.get_hour());
		if (m_fplautoroutespinbuttonmin)
			m_fplautoroutespinbuttonmin->set_value(dt.get_minute());
	}
	if (m_fplautoroutecheckbuttontfr)
		m_fplautoroutecheckbuttontfr->set_sensitive(false);
	if (m_fplautoroutespinbuttonbaselevel && m_sensors->get_configfile().has_key(cfggroup_mainwindow, "autoroutebaselevel"))
		m_fplautoroutespinbuttonbaselevel->set_value(m_sensors->get_configfile().get_integer(cfggroup_mainwindow, "autoroutebaselevel"));
	if (m_fplautoroutespinbuttontoplevel && m_sensors->get_configfile().has_key(cfggroup_mainwindow, "autoroutetoplevel"))
		m_fplautoroutespinbuttontoplevel->set_value(m_sensors->get_configfile().get_integer(cfggroup_mainwindow, "autoroutetoplevel"));
	if (m_fplautoroutecheckbuttonsiddb && m_sensors->get_configfile().has_key(cfggroup_mainwindow, "autoroutesiddb"))
		m_fplautoroutecheckbuttonsiddb->set_active(!!m_sensors->get_configfile().get_integer(cfggroup_mainwindow, "autoroutesiddb"));
	if (m_fplautoroutecheckbuttonsidonly && m_sensors->get_configfile().has_key(cfggroup_mainwindow, "autoroutesidonly"))
		m_fplautoroutecheckbuttonsidonly->set_active(!!m_sensors->get_configfile().get_integer(cfggroup_mainwindow, "autoroutesidonly"));
	if (m_fplautoroutecheckbuttonstardb && m_sensors->get_configfile().has_key(cfggroup_mainwindow, "autoroutestardb"))
		m_fplautoroutecheckbuttonstardb->set_active(!!m_sensors->get_configfile().get_integer(cfggroup_mainwindow, "autoroutestardb"));
	if (m_fplautoroutecheckbuttonstaronly && m_sensors->get_configfile().has_key(cfggroup_mainwindow, "autoroutestaronly"))
		m_fplautoroutecheckbuttonstaronly->set_active(!!m_sensors->get_configfile().get_integer(cfggroup_mainwindow, "autoroutestaronly"));
#ifdef HAVE_JSONCPP
	m_childsockets.clear();
        if (m_childsockaddr) {
		{
			Glib::TimeVal tv;
			tv.assign_current_time();
			Glib::Rand rnd(tv.tv_sec);
			rnd.set_seed(rnd.get_int() ^ tv.tv_usec);
#ifdef G_OS_UNIX
			rnd.set_seed(rnd.get_int() ^ getpid());
#endif
			std::ostringstream oss;
			oss << std::hex;
			for (unsigned int i = 0; i < 4; ++i)
				oss << std::setfill('0') << std::setw(8) << rnd.get_int();
			m_childsession = oss.str();
		}
		if (true) {
			std::cerr << "Trying autoroute server ";
			{
				Glib::RefPtr<Gio::UnixSocketAddress> sa(Glib::RefPtr<Gio::UnixSocketAddress>::cast_dynamic(m_childsockaddr));
				if (sa)
					std::cerr << "unix path " << sa->get_path();
			}
			{
				Glib::RefPtr<Gio::InetSocketAddress> sa(Glib::RefPtr<Gio::InetSocketAddress>::cast_dynamic(m_childsockaddr));
				if (sa)
					std::cerr << "inet " << sa->get_address()->to_string() << ':' << sa->get_port();
			}
			std::cerr << " session " << m_childsession << std::endl;
		}
		Json::Value root;
		root["session"] = m_childsession;
		Json::Value cmds;
		{
			Json::Value cmd;
			cmd["cmdname"] = "departure";
			cmd["cmdseq"] = "init";
			cmds.append(cmd);
		}
		{
			Json::Value cmd;
			cmd["cmdname"] = "destination";
			cmd["cmdseq"] = "init";
			cmds.append(cmd);
		}
		{
			Json::Value cmd;
			cmd["cmdname"] = "crossing";
			cmd["cmdseq"] = "init";
			cmds.append(cmd);
		}
		{
			Json::Value cmd;
			cmd["cmdname"] = "enroute";
			cmd["cmdseq"] = "init";
			cmds.append(cmd);
		}
		{
			Json::Value cmd;
			cmd["cmdname"] = "levels";
			cmd["cmdseq"] = "init";
			cmds.append(cmd);
		}
		{
			Json::Value cmd;
			cmd["cmdname"] = "preferred";
			cmd["cmdseq"] = "init";
			cmds.append(cmd);
		}
		{
			Json::Value cmd;
			cmd["cmdname"] = "tfr";
			cmd["cmdseq"] = "init";
			cmds.append(cmd);
		}
		{
			Json::Value cmd;
			cmd["cmdname"] = "atmosphere";
			cmd["cmdseq"] = "init";
			cmds.append(cmd);
		}
#ifndef G_OS_WIN32
		{
			Json::Value cmd;
			cmd["cmdname"] = "preload";
			cmd["cmdseq"] = "init";
			cmds.append(cmd);
		}
#endif
		root["cmds"] = cmds;
		sendjson(root);
		if (m_childsockets.empty())
			m_childsockaddr.reset();
		m_proxy.clear_fplan();
	}
	if (!m_childsockaddr)
#endif
	{
		m_proxy.start_autorouter();
		m_proxy.clear_fplan();
		try {
			if (m_firstrun) {
				m_firstrun = false;
				{
					CFMUAutorouteProxy::Command cmd("departure");
					cmd.set_option("cmdseq", "init");
					m_proxy.sendcmd(cmd);
				}
				{
					CFMUAutorouteProxy::Command cmd("destination");
					cmd.set_option("cmdseq", "init");
					m_proxy.sendcmd(cmd);
				}
				{
					CFMUAutorouteProxy::Command cmd("crossing");
					cmd.set_option("cmdseq", "init");
					m_proxy.sendcmd(cmd);
				}
				{
					CFMUAutorouteProxy::Command cmd("enroute");
					cmd.set_option("cmdseq", "init");
					m_proxy.sendcmd(cmd);
				}
				{
					CFMUAutorouteProxy::Command cmd("levels");
					cmd.set_option("cmdseq", "init");
					m_proxy.sendcmd(cmd);
				}
				{
					CFMUAutorouteProxy::Command cmd("preferred");
					cmd.set_option("cmdseq", "init");
					m_proxy.sendcmd(cmd);
				}
				{
					CFMUAutorouteProxy::Command cmd("tfr");
					cmd.set_option("cmdseq", "init");
					m_proxy.sendcmd(cmd);
				}
				{
					CFMUAutorouteProxy::Command cmd("atmosphere");
					cmd.set_option("cmdseq", "init");
					m_proxy.sendcmd(cmd);
				}
#ifndef G_OS_WIN32
				{
					CFMUAutorouteProxy::Command cmd("preload");
					cmd.set_option("cmdseq", "init");
					m_proxy.sendcmd(cmd);
				}
#endif
			}
		} catch (const std::exception& e) {
			append_log(std::string("Exception: ") + e.what() + "\n");
		}
	}
	{
		const FPlanRoute& route(m_sensors->get_route());
		for (unsigned int i = 0; i < 2; ++i) {
			if (route.get_nrwpt() <= i)
				break;
			IcaoFlightPlan::FindCoord f(*m_sensors->get_engine());
			const FPlanWaypoint& wpt(route[(route.get_nrwpt() - 1) & -i]);
			AirportsDb::Airport el;
			bool elok(false);
			if (f.find(wpt.get_coord(), IcaoFlightPlan::FindCoord::flag_airport, 0.1f))
				elok = f.get_airport(el);
			if (!elok || !el.is_valid())
				continue;
			if (i) {
				m_destination = el;
				if (m_destination.is_valid()) {
					if (m_fplautorouteentrydesticao)
						m_fplautorouteentrydesticao->set_text(m_destination.get_icao());
					if (m_fplautorouteentrydestname)
						m_fplautorouteentrydestname->set_text(m_destination.get_name());
				}
				if (m_fplautoroutecheckbuttondestifr)
					m_fplautoroutecheckbuttondestifr->set_active(!!(wpt.get_flags() & FPlanWaypoint::altflag_ifr));
			} else {
				m_departure = el;
				if (m_departure.is_valid()) {
					if (m_fplautorouteentrydepicao)
						m_fplautorouteentrydepicao->set_text(m_departure.get_icao());
					if (m_fplautorouteentrydepname)
						m_fplautorouteentrydepname->set_text(m_departure.get_name());
				}
				if (m_fplautoroutecheckbuttondepifr)
					m_fplautoroutecheckbuttondepifr->set_active(!!(wpt.get_flags() & FPlanWaypoint::altflag_ifr));
			}
		}
		if (route.get_nrwpt() >= 3) {
			for (unsigned int i = 0; i < 2; ++i) {
				Point pt;
				if (i) {
					unsigned int j(route.get_nrwpt() - 2);
					while (j > 1) {
						if (route[j - 1].get_pathcode() != FPlanWaypoint::pathcode_star)
							break;
						--j;
					}
					pt = route[j].get_coord();
				} else {
					unsigned int j(1);
					while (j + 2 < route.get_nrwpt()) {
						if (route[j].get_pathcode() != FPlanWaypoint::pathcode_sid)
							break;
						++j;
					}
					pt = route[j].get_coord();
				}
				if (route.get_nrwpt() == 3) {
					bool ok(pt.simple_distance_rel(route[0].get_coord()) <
						pt.simple_distance_rel(route[route.get_nrwpt() - 1].get_coord()));
					ok ^= i;
					if (!ok)
						continue;
				}
				Glib::ustring pticao, ptname;
				IcaoFlightPlan::FindCoord f(*m_sensors->get_engine());
				if (f.find(pt, IcaoFlightPlan::FindCoord::flag_navaid | IcaoFlightPlan::FindCoord::flag_waypoint, 0.1f)) {
					pt = Point::invalid;
					NavaidsDb::element_t nav;
					WaypointsDb::element_t wpt;
					if (f.get_navaid(nav)) {
						pt = nav.get_coord();
						ptname = nav.get_navaid_typename() + " " + nav.get_name();
						pticao = nav.get_icao();
					} else if (f.get_waypoint(wpt)) {
						pt = wpt.get_coord();
						ptname = "INT " + wpt.get_name();
						pticao = wpt.get_name();
					}
				} else {
					pt = Point::invalid;
				}
				if (!i) {
					m_sidend = pt;
					if (m_fplautorouteentrysidicao)
						m_fplautorouteentrysidicao->set_text(pticao);
					if (m_fplautorouteentrysidname)
						m_fplautorouteentrysidname->set_text(ptname);
				} else {
					m_starstart = pt;
					if (m_fplautorouteentrystaricao)
						m_fplautorouteentrystaricao->set_text(pticao);
					if (m_fplautorouteentrystarname)
						m_fplautorouteentrystarname->set_text(ptname);
				}
			}
		}
	}
	for (unsigned int i = 0; i < sizeof(m_ptchanged)/sizeof(m_ptchanged[0]); ++i)
		m_ptchanged[i] = false;
	pt_focusout();
	initial_fplan();
	show();
	if (m_fplautoroutedrawingarea) {
                m_fplautoroutedrawingarea->show();
		m_fplautoroutedrawingarea->set_renderer(VectorMapArea::renderer_vmap);
                *m_fplautoroutedrawingarea = m_flags;
                m_fplautoroutedrawingarea->set_map_scale(m_mapscale);
		m_fplautoroutedrawingarea->set_route(m_proxy.get_fplan());
	}
	if (m_fplautoroutetextviewpg2route) {
		Glib::RefPtr<Gtk::TextBuffer> buf(m_fplautoroutetextviewpg2route->get_buffer());
		if (buf)
			buf->set_text("");
	}
	if (m_fplautoroutetextviewpg2results) {
		Glib::RefPtr<Gtk::TextBuffer> buf(m_fplautoroutetextviewpg2results->get_buffer());
		if (buf)
			buf->set_text("");
	}
	if (m_fplautoroutetextviewpg2log) {
		Glib::RefPtr<Gtk::TextBuffer> buf(m_fplautoroutetextviewpg2log->get_buffer());
		if (buf)
			buf->set_text("");
	}
	if (m_fplautoroutetextviewpg2graph) {
		Glib::RefPtr<Gtk::TextBuffer> buf(m_fplautoroutetextviewpg2graph->get_buffer());
		if (buf)
			buf->set_text("");
	}
	if (m_fplautoroutetextviewpg2comm) {
		Glib::RefPtr<Gtk::TextBuffer> buf(m_fplautoroutetextviewpg2comm->get_buffer());
		if (buf)
			buf->set_text("");
	}
	if (m_fplautoroutetextviewpg2debug) {
		Glib::RefPtr<Gtk::TextBuffer> buf(m_fplautoroutetextviewpg2debug->get_buffer());
		if (buf)
			buf->set_text("");
	}
}

void FlightDeckWindow::FPLAutoRouteDialog::close(void)
{
	if (m_sensors) {
		if (m_fplautoroutespinbuttonbaselevel)
			m_sensors->get_configfile().set_integer(cfggroup_mainwindow, "autoroutebaselevel", m_fplautoroutespinbuttonbaselevel->get_value());
		if (m_fplautoroutespinbuttontoplevel)
			m_sensors->get_configfile().set_integer(cfggroup_mainwindow, "autoroutetoplevel", m_fplautoroutespinbuttontoplevel->get_value());
		if (m_fplautoroutecheckbuttonsiddb)
			m_sensors->get_configfile().set_integer(cfggroup_mainwindow, "autoroutesiddb", m_fplautoroutecheckbuttonsiddb->get_active());
		if (m_fplautoroutecheckbuttonsidonly)
			m_sensors->get_configfile().set_integer(cfggroup_mainwindow, "autoroutesidonly", m_fplautoroutecheckbuttonsidonly->get_active());
		if (m_fplautoroutecheckbuttonstardb)
			m_sensors->get_configfile().set_integer(cfggroup_mainwindow, "autoroutestardb", m_fplautoroutecheckbuttonstardb->get_active());
		if (m_fplautoroutecheckbuttonstaronly)
			m_sensors->get_configfile().set_integer(cfggroup_mainwindow, "autoroutestaronly", m_fplautoroutecheckbuttonstaronly->get_active());
		m_sensors->save_config();
	}
	if (m_fplautoroutedrawingarea)
                m_fplautoroutedrawingarea->hide();
	m_sensors = 0;
	hide();
	m_departure = m_destination = AirportsDb::Airport();
#ifdef HAVE_JSONCPP
	m_childsockets.clear();
	if (m_childsockaddr) {
		Json::Value root;
		root["session"] = m_childsession;
		Json::Value cmds;
		{
			Json::Value cmd;
			cmd["cmdname"] = "stop";
			cmds.append(cmd);
		}
		root["cmds"] = cmds;
		root["quit"] = true;
		sendjson(root);
	}
	m_childsockaddr.reset();
	m_childsession.clear();
#endif
	m_proxy.stop_autorouter();
	m_proxy.clear_fplan();
	if (m_fplautoroutetextviewpg2route) {
		Glib::RefPtr<Gtk::TextBuffer> buf(m_fplautoroutetextviewpg2route->get_buffer());
		if (buf)
			buf->set_text("");
	}
	if (m_fplautoroutetextviewpg2results) {
		Glib::RefPtr<Gtk::TextBuffer> buf(m_fplautoroutetextviewpg2results->get_buffer());
		if (buf)
			buf->set_text("");
	}
	if (m_fplautoroutetextviewpg2log) {
		Glib::RefPtr<Gtk::TextBuffer> buf(m_fplautoroutetextviewpg2log->get_buffer());
		if (buf)
			buf->set_text("");
	}
	if (m_fplautoroutetextviewpg2graph) {
		Glib::RefPtr<Gtk::TextBuffer> buf(m_fplautoroutetextviewpg2graph->get_buffer());
		if (buf)
			buf->set_text("");
	}
	if (m_fplautoroutetextviewpg2comm) {
		Glib::RefPtr<Gtk::TextBuffer> buf(m_fplautoroutetextviewpg2comm->get_buffer());
		if (buf)
			buf->set_text("");
	}
	if (m_fplautoroutetextviewpg2debug) {
		Glib::RefPtr<Gtk::TextBuffer> buf(m_fplautoroutetextviewpg2debug->get_buffer());
		if (buf)
			buf->set_text("");
	}
}

void FlightDeckWindow::FPLAutoRouteDialog::append_log(const Glib::ustring& text)
{
	if (!m_fplautoroutetextviewpg2log)
		return;
	Glib::RefPtr<Gtk::TextBuffer> buf(m_fplautoroutetextviewpg2log->get_buffer());
	if (!buf)
		return;
	buf->insert(buf->end(), text);
	Gtk::TextBuffer::iterator it(buf->end());
	m_fplautoroutetextviewpg2log->scroll_to(it);
}

void FlightDeckWindow::FPLAutoRouteDialog::initial_fplan(void)
{
	m_proxy.clear_fplan();
	if (m_departure.is_valid()) {
		m_proxy.get_fplan().insert(m_departure);
		if (m_depifr) {
			FPlanWaypoint& wpt(m_proxy.get_fplan()[0]);
			wpt.set_flags(wpt.get_flags() | FPlanWaypoint::altflag_ifr);
		}
	}
	int alt(100);
	if (m_fplautoroutespinbuttonbaselevel)
		alt = m_fplautoroutespinbuttonbaselevel->get_value();
	alt *= 100;
	if (!m_sidend.is_invalid()) {
		FPlanWaypoint wpt;
		wpt.set_coord(m_sidend);
		wpt.set_name("SID");
		wpt.set_flags(wpt.get_flags() | FPlanWaypoint::altflag_ifr);
		wpt.set_flags(wpt.get_flags() | FPlanWaypoint::altflag_standard);
		wpt.set_altitude(alt);
		m_proxy.get_fplan().insert_wpt(~0U, wpt);
	}
	if (!m_starstart.is_invalid()) {
		FPlanWaypoint wpt;
		wpt.set_coord(m_starstart);
		wpt.set_name("STAR");
		if (m_destifr)
			wpt.set_flags(wpt.get_flags() | FPlanWaypoint::altflag_ifr);
		wpt.set_flags(wpt.get_flags() | FPlanWaypoint::altflag_standard);
		wpt.set_altitude(alt);
		m_proxy.get_fplan().insert_wpt(~0U, wpt);
	}
	if (m_destination.is_valid()) {
		m_proxy.get_fplan().insert(m_destination);
		if (m_destifr) {
			FPlanWaypoint& wpt(m_proxy.get_fplan()[m_proxy.get_fplan().get_nrwpt() - 1]);
			wpt.set_flags(wpt.get_flags() | FPlanWaypoint::altflag_ifr);
		}
	}
	if (m_fplautoroutedrawingarea)
		m_fplautoroutedrawingarea->redraw_route();
}

Rect FlightDeckWindow::FPLAutoRouteDialog::get_bbox(void) const
{
	if (!m_departure.is_valid() && !m_destination.is_valid())
		return Rect();
	if (!m_departure.is_valid())
		return Rect(m_destination.get_coord(), m_destination.get_coord());
	if (!m_destination.is_valid())
		return Rect(m_departure.get_coord(), m_departure.get_coord());
	Point pt1(m_departure.get_coord());
	Point pt2(m_destination.get_coord() - pt1);
	return Rect(Point(std::min(pt2.get_lon(), (Point::coord_t)0), std::min(pt2.get_lat(), (Point::coord_t)0)) + pt1,
		    Point(std::max(pt2.get_lon(), (Point::coord_t)0), std::max(pt2.get_lat(), (Point::coord_t)0)) + pt1);
}

void FlightDeckWindow::FPLAutoRouteDialog::recvcmd(const CFMUAutorouteProxy::Command& cmd)
{
	if (false)
		std::cerr << "Autorouter: " << cmd.get_cmdstring() << std::endl;
	if (m_fplautoroutetextviewpg2comm) {
		Glib::RefPtr<Gtk::TextBuffer> buf(m_fplautoroutetextviewpg2comm->get_buffer());
		if (buf) {
			buf->insert(buf->end(), "# " + cmd.get_cmdstring() + "\n");
			Gtk::TextBuffer::iterator it(buf->end());
			m_fplautoroutetextviewpg2comm->scroll_to(it);
		}
	}
	if (cmd.get_cmdname() == "autoroute") {
		std::pair<const std::string&,bool> status(cmd.get_option("status"));
		if (status.second) {
			if ((status.first == "stopping" || status.first == "terminate") &&
			    m_fplautoroutetogglebuttonpg2run && m_fplautoroutetogglebuttonpg2run->get_active())
				m_fplautoroutetogglebuttonpg2run->set_active(false);
		}
		return;
	}
	if (cmd.get_cmdname() == "proxy") {
		std::pair<const std::string&,bool> state(cmd.get_option("state"));
		if (state.second) {
			if ((state.first == "stopped" || state.first == "error") &&
			    m_fplautoroutetogglebuttonpg2run && m_fplautoroutetogglebuttonpg2run->get_active())
				m_fplautoroutetogglebuttonpg2run->set_active(false);
		}
		return;
	}
	if (cmd.get_cmdname() == "log") {
		std::pair<const std::string&,bool> item(cmd.get_option("item"));
		std::pair<const std::string&,bool> text(cmd.get_option("text"));
		if (!item.second || !text.second)
			return;
		if (item.first == "fpllocalvalidation" ||
		    item.first == "fplremotevalidation") {
			if (!m_fplautoroutetextviewpg2results)
				return;
			Glib::RefPtr<Gtk::TextBuffer> buf(m_fplautoroutetextviewpg2results->get_buffer());
			if (!buf)
				return;
			if (m_fplvalclear)
				buf->set_text("");
			buf->insert(buf->end(), text.first + "\n");
			Gtk::TextBuffer::iterator it(buf->end());
			m_fplautoroutetextviewpg2results->scroll_to(it);
			m_fplvalclear = text.first.empty();
			return;
		}
		if (item.first == "graphchange") {
			if (m_fplautoroutetextviewpg2graph) {
				Glib::RefPtr<Gtk::TextBuffer> buf(m_fplautoroutetextviewpg2graph->get_buffer());
				if (buf) {
					buf->insert_with_tag(buf->end(), "  " + text.first + "\n", m_txtgraphchange);
					Gtk::TextBuffer::iterator it(buf->end());
					m_fplautoroutetextviewpg2graph->scroll_to(it);
				}
			}
			return;
		}
		if (item.first == "graphrule") {
			if (m_fplautoroutetextviewpg2graph) {
				Glib::RefPtr<Gtk::TextBuffer> buf(m_fplautoroutetextviewpg2graph->get_buffer());
				if (buf) {
					buf->insert_with_tag(buf->end(), text.first + "\n", m_txtgraphrule);
					Gtk::TextBuffer::iterator it(buf->end());
					m_fplautoroutetextviewpg2graph->scroll_to(it);
				}
			}
			return;
		}
		if (item.first == "graphruledesc") {
			if (m_fplautoroutetextviewpg2graph) {
				Glib::RefPtr<Gtk::TextBuffer> buf(m_fplautoroutetextviewpg2graph->get_buffer());
				if (buf) {
					buf->insert_with_tag(buf->end(), text.first + "\n", m_txtgraphruledesc);
					Gtk::TextBuffer::iterator it(buf->end());
					m_fplautoroutetextviewpg2graph->scroll_to(it);
				}
			}
			return;
		}
		if (item.first == "graphruleoprgoal") {
			if (m_fplautoroutetextviewpg2graph) {
				Glib::RefPtr<Gtk::TextBuffer> buf(m_fplautoroutetextviewpg2graph->get_buffer());
				if (buf) {
					buf->insert_with_tag(buf->end(), text.first + "\n", m_txtgraphruleoprgoal);
					Gtk::TextBuffer::iterator it(buf->end());
					m_fplautoroutetextviewpg2graph->scroll_to(it);
				}
			}
			return;
		}
		if (item.first.substr(0, 5) == "debug") {
			if (m_fplautoroutetextviewpg2debug) {
				Glib::RefPtr<Gtk::TextBuffer> buf(m_fplautoroutetextviewpg2debug->get_buffer());
				if (buf) {
					buf->insert(buf->end(), text.first + "\n");
					Gtk::TextBuffer::iterator it(buf->end());
					m_fplautoroutetextviewpg2debug->scroll_to(it);
				}
			}
			return;
		}
		if (item.first == "precompgraph") {
			append_log("Graph: " + text.first + "\n");
			return;
		}
		if (item.first == "weatherdata") {
			append_log("Wx: " + text.first + "\n");
			return;
		}
		append_log(text.first + "\n");
		return;
	}
	if (cmd.get_cmdname() == "fplend") {
		std::pair<const std::string&,bool> fpl(cmd.get_option("fpl"));
		if (fpl.second && m_fplautoroutetextviewpg2route) {
			Glib::RefPtr<Gtk::TextBuffer> buf(m_fplautoroutetextviewpg2route->get_buffer());
			if (buf)
				buf->set_text(fpl.first);
		}
		if (m_fplautoroutetextviewpg2graph) {
			Glib::RefPtr<Gtk::TextBuffer> buf(m_fplautoroutetextviewpg2graph->get_buffer());
			if (buf) {
				buf->insert_with_tag(buf->end(), fpl.first + "\n", m_txtgraphfpl);
				Gtk::TextBuffer::iterator it(buf->end());
				m_fplautoroutetextviewpg2graph->scroll_to(it);
			}
		}
		std::pair<double,bool> gcdist(cmd.get_option_float("gcdist"));
		std::pair<double,bool> routedist(cmd.get_option_float("routedist"));
		std::pair<unsigned long,bool> mintime(cmd.get_option_uint("mintime"));
		std::pair<unsigned long,bool> routetime(cmd.get_option_uint("routetime"));
		std::pair<double,bool> minfuel(cmd.get_option_float("minfuel"));
		std::pair<double,bool> routefuel(cmd.get_option_float("routefuel"));
		std::pair<unsigned long,bool> mintimezw(cmd.get_option_uint("mintimezerowind"));
		std::pair<unsigned long,bool> routetimezw(cmd.get_option_uint("routetimezerowind"));
		std::pair<double,bool> minfuelzw(cmd.get_option_float("minfuelzerowind"));
		std::pair<double,bool> routefuelzw(cmd.get_option_float("routefuelzerowind"));
		std::pair<double,bool> metric(cmd.get_option_float("routemetric"));
		std::pair<const std::string&,bool> iter(cmd.get_option("iteration"));
		std::pair<const std::string&,bool> iterlocal(cmd.get_option("localiteration"));
		std::pair<const std::string&,bool> iterremote(cmd.get_option("remoteiteration"));
		std::pair<double,bool> wallclocktime(cmd.get_option_float("wallclocktime"));
		std::pair<double,bool> validatortime(cmd.get_option_float("validatortime"));
		std::pair<unsigned long,bool> variant(cmd.get_option_uint("variant"));
		std::pair<unsigned long,bool> vfrdep(cmd.get_option_uint("vfrdep"));
		std::pair<unsigned long,bool> vfrarr(cmd.get_option_uint("vfrarr"));
		std::pair<unsigned long,bool> nonstddep(cmd.get_option_uint("nonstddep"));
		std::pair<unsigned long,bool> nonstdarr(cmd.get_option_uint("nonstdarr"));
		std::pair<unsigned long,bool> procint(cmd.get_option_uint("procint"));
		std::pair<unsigned long,bool> fullstdrte(cmd.get_option_uint("fullstdrte"));
		std::pair<const std::string&,bool> item15atc(cmd.get_option("item15atc"));
		std::pair<const std::string&,bool> status(cmd.get_option("status"));
		if (gcdist.second && m_fplautorouteentrypg2gcdist)
			m_fplautorouteentrypg2gcdist->set_text(Conversions::dist_str(gcdist.first));
		if (routedist.second && m_fplautorouteentrypg2routedist)
			m_fplautorouteentrypg2routedist->set_text(Conversions::dist_str(routedist.first));
		if (m_fplautorouteentrypg2mintime) {
			std::ostringstream oss;
			if (mintime.second)
				oss << Conversions::time_str(mintime.first);
			if (mintime.second && mintimezw.second)
				oss << ' ';
			if (mintimezw.second)
				oss << '[' << Conversions::time_str(mintimezw.first) << ']';
			m_fplautorouteentrypg2mintime->set_text(oss.str());
		}
		if (m_fplautorouteentrypg2routetime) {
			std::ostringstream oss;
			if (routetime.second)
				oss << Conversions::time_str(routetime.first);
			if (routetime.second && routetimezw.second)
				oss << ' ';
			if (routetimezw.second)
				oss << '[' << Conversions::time_str(routetimezw.first) << ']';
			m_fplautorouteentrypg2routetime->set_text(oss.str());
		}
		if (m_fplautorouteentrypg2minfuel) {
			std::ostringstream oss;
			if (minfuel.second)
				oss << Conversions::dist_str(minfuel.first);
			if (minfuel.second && minfuelzw.second)
				oss << ' ';
			if (minfuelzw.second)
				oss << '[' << Conversions::dist_str(minfuelzw.first) << ']';
			m_fplautorouteentrypg2minfuel->set_text(oss.str());
		}
		if (routefuel.second && m_fplautorouteentrypg2routefuel) {
			std::ostringstream oss;
			if (routefuel.second)
				oss << Conversions::dist_str(routefuel.first);
			if (routefuel.second && routefuelzw.second)
				oss << ' ';
			if (routefuelzw.second)
				oss << '[' << Conversions::dist_str(routefuelzw.first) << ']';
			m_fplautorouteentrypg2routefuel->set_text(oss.str());
		}
		if (m_fplautorouteentrypg2iteration) {
			std::ostringstream oss;
			if (iter.second)
				oss << iter.first;
			if (iterlocal.second && iterremote.second) {
				if (iter.second)
					oss << ' ';
				oss << '(' << iterlocal.first << '/' << iterremote.first << ')';
			}
			m_fplautorouteentrypg2iteration->set_text(oss.str());
		}
		if (m_fplautorouteentrypg2times) {
			std::ostringstream oss;
			if (wallclocktime.second)
				oss << std::fixed << std::setprecision(1) << wallclocktime.first;
			oss << '/';
			if (validatortime.second)
				oss << std::fixed << std::setprecision(1) << validatortime.first;
			m_fplautorouteentrypg2times->set_text(oss.str());
		}
		if (m_fplautoroutedrawingarea)
			m_fplautoroutedrawingarea->redraw_route();
		if (m_routesstore && status.second && status.first == "solution") {
			Gtk::TreeIter iter(m_routesstore->append());
			Gtk::TreeModel::Row row(*iter);
			{
				std::ostringstream oss;
				if (routetime.second)
					oss << Conversions::time_str(routetime.first);
				if (routetime.second && routetimezw.second)
					oss << ' ';
				if (routetimezw.second)
					oss << '[' << Conversions::time_str(routetimezw.first) << ']';
			        row[m_routescolumns.m_time] = oss.str();
			}
			{
				std::ostringstream oss;
				if (routefuel.second)
					oss << Conversions::dist_str(routefuel.first);
				if (routefuel.second && routefuelzw.second)
					oss << ' ';
				if (routefuelzw.second)
					oss << '[' << Conversions::dist_str(routefuelzw.first) << ']';
				row[m_routescolumns.m_fuel] = oss.str();
			}
			if (item15atc.second)
				row[m_routescolumns.m_routetxt] = item15atc.first;
			else if (fpl.second)
				row[m_routescolumns.m_routetxt] = fpl.first;
			else
				row[m_routescolumns.m_routetxt] = "?";
			{
				std::ostringstream oss;
				if (vfrdep.second && vfrdep.first)
					oss << " VFRDEP";
				if (vfrarr.second && vfrarr.first)
					oss << " VFRARR";
				if (nonstddep.second && nonstddep.first)
					oss << " NONSTDDEP";
				if (nonstdarr.second && nonstdarr.first)
					oss << " NONSTDARR";
				if (procint.second && procint.first)
					oss << " PROCINT";
				if (fullstdrte.second && fullstdrte.first)
					oss << " STDRTE";
				if (oss.str().empty())
					row[m_routescolumns.m_flags] = "";
				else
					row[m_routescolumns.m_flags] = oss.str().substr(1);
			}
			row[m_routescolumns.m_route] = (RoutesModelColumns::FPLRoute)m_proxy.get_fplan();
			if (metric.second)
				row[m_routescolumns.m_routemetric] = metric.first;
			else
				row[m_routescolumns.m_routemetric] = std::numeric_limits<double>::quiet_NaN();
			if (routefuel.second)
				row[m_routescolumns.m_routefuel] = routefuel.first;
			else
				row[m_routescolumns.m_routefuel] = std::numeric_limits<double>::quiet_NaN();
			if (routefuelzw.second)
				row[m_routescolumns.m_routezerowindfuel] = routefuelzw.first;
			else
				row[m_routescolumns.m_routezerowindfuel] = std::numeric_limits<double>::quiet_NaN();
			if (routetime.second)
				row[m_routescolumns.m_routetime] = routetime.first;
			else
				row[m_routescolumns.m_routetime] = std::numeric_limits<double>::quiet_NaN();
			if (routetimezw.second)
				row[m_routescolumns.m_routezerowindtime] = routetimezw.first;
			else
				row[m_routescolumns.m_routezerowindtime] = std::numeric_limits<double>::quiet_NaN();
			if (variant.second)
				row[m_routescolumns.m_variant] = variant.first;
			else
				row[m_routescolumns.m_variant] = 0;
			if (m_fplautorouteframepg2routes)
				m_fplautorouteframepg2routes->set_visible(true);
		}
		return;
	}
	if (cmd.get_cmdname() == "departure") {
		std::pair<const std::string&,bool> cmdseq(cmd.get_option("cmdseq"));
		std::pair<const std::string&,bool> icao(cmd.get_option("icao"));
		std::pair<const std::string&,bool> name(cmd.get_option("name"));
		std::string icao_name;
		if (icao.second && !icao.first.empty())
			icao_name = icao.first;
		if (icao.second && !icao.first.empty() && name.second && !name.first.empty())
			icao_name += " ";
		if (name.second && !name.first.empty())
			icao_name += name.first;
		if (m_fplautorouteentrypg2dep)
			m_fplautorouteentrypg2dep->set_text(icao_name);
		if (!cmdseq.second || cmdseq.first != "init")
			return;
		std::pair<double,bool> sidlimit(cmd.get_option_float("sidlimit"));
		if (sidlimit.second) {
			m_sidstarlimit[0][1] = sidlimit.first;
			if (m_depifr && m_fplautoroutespinbuttonsidlimit)
				m_fplautoroutespinbuttonsidlimit->set_value(m_sidstarlimit[0][m_depifr]);
		}
		std::pair<double,bool> sidpenalty(cmd.get_option_float("sidpenalty"));
		if (sidpenalty.second)
			m_fplautoroutespinbuttonsidpenalty->set_value(sidpenalty.first);
		std::pair<double,bool> sidoffset(cmd.get_option_float("sidoffset"));
		if (sidoffset.second)
			m_fplautoroutespinbuttonsidoffset->set_value(sidoffset.first);
		std::pair<double,bool> sidminimum(cmd.get_option_float("sidminimum"));
		if (sidminimum.second)
			m_fplautoroutespinbuttonsidminimum->set_value(sidminimum.first);
		std::pair<unsigned long,bool> siddb(cmd.get_option_uint("siddb"));
		if (siddb.second) {
			if (m_fplautoroutecheckbuttonsiddb &&
			    !(m_sensors && m_sensors->get_configfile().has_key(cfggroup_mainwindow, "autoroutesiddb")))
				m_fplautoroutecheckbuttonsiddb->set_active(!!siddb.first);
		}
		std::pair<unsigned long,bool> sidonly(cmd.get_option_uint("sidonly"));
		if (sidonly.second) {
			if (m_fplautoroutecheckbuttonsidonly &&
			    !(m_sensors && m_sensors->get_configfile().has_key(cfggroup_mainwindow, "autoroutesidonly")))
				m_fplautoroutecheckbuttonsidonly->set_active(!!sidonly.first);
		}
		return;
	}
	if (cmd.get_cmdname() == "destination") {
		std::pair<const std::string&,bool> cmdseq(cmd.get_option("cmdseq"));
		std::pair<const std::string&,bool> icao(cmd.get_option("icao"));
		std::pair<const std::string&,bool> name(cmd.get_option("name"));
		std::string icao_name;
		if (icao.second && !icao.first.empty())
			icao_name = icao.first;
		if (icao.second && !icao.first.empty() && name.second && !name.first.empty())
			icao_name += " ";
		if (name.second && !name.first.empty())
			icao_name += name.first;
		if (m_fplautorouteentrypg2dest)
			m_fplautorouteentrypg2dest->set_text(icao_name);
		if (!cmdseq.second || cmdseq.first != "init")
			return;
		std::pair<double,bool> starlimit(cmd.get_option_float("starlimit"));
		if (starlimit.second) {
			m_sidstarlimit[1][1] = starlimit.first;
			if (m_destifr && m_fplautoroutespinbuttonstarlimit)
				m_fplautoroutespinbuttonstarlimit->set_value(m_sidstarlimit[1][m_destifr]);
		}
		std::pair<double,bool> starpenalty(cmd.get_option_float("starpenalty"));
		if (starpenalty.second)
			m_fplautoroutespinbuttonstarpenalty->set_value(starpenalty.first);
		std::pair<double,bool> staroffset(cmd.get_option_float("staroffset"));
		if (staroffset.second)
			m_fplautoroutespinbuttonstaroffset->set_value(staroffset.first);
		std::pair<double,bool> starminimum(cmd.get_option_float("starminimum"));
		if (starminimum.second)
			m_fplautoroutespinbuttonstarminimum->set_value(starminimum.first);
		std::pair<unsigned long,bool> stardb(cmd.get_option_uint("stardb"));
		if (stardb.second) {
			if (m_fplautoroutecheckbuttonstardb &&
			    !(m_sensors && m_sensors->get_configfile().has_key(cfggroup_mainwindow, "autoroutestardb")))
				m_fplautoroutecheckbuttonstardb->set_active(!!stardb.first);
		}
		std::pair<unsigned long,bool> staronly(cmd.get_option_uint("staronly"));
		if (staronly.second) {
			if (m_fplautoroutecheckbuttonstaronly &&
			    !(m_sensors && m_sensors->get_configfile().has_key(cfggroup_mainwindow, "autoroutestaronly")))
				m_fplautoroutecheckbuttonstaronly->set_active(!!staronly.first);
		}
		return;
	}
	if (cmd.get_cmdname() == "crossing") {
		std::pair<const std::string&,bool> cmdseq(cmd.get_option("cmdseq"));
		bool init(cmdseq.second && cmdseq.first == "init");
		std::pair<unsigned long,bool> index(cmd.get_option_uint("index"));
		std::pair<unsigned long,bool> count(cmd.get_option_uint("count"));
		if (!index.second || !count.second)
			return;
		if (!init)
			return;
		std::pair<const std::string&,bool> icao(cmd.get_option("icao"));
		std::pair<const std::string&,bool> name(cmd.get_option("name"));
		std::pair<Point,bool> coord(cmd.get_option_coord("coord"));
		std::pair<double,bool> radius(cmd.get_option_float("radius"));
		std::pair<unsigned long,bool> minlevel(cmd.get_option_uint("minlevel"));
		std::pair<unsigned long,bool> maxlevel(cmd.get_option_uint("maxlevel"));
		std::string icao_name;
		if (!radius.second)
			radius.first = 0;
		if (!minlevel.second)
			minlevel.first = 0;
		if (!maxlevel.second)
			maxlevel.first = 600;
		if (icao.second && !icao.first.empty())
			icao_name = icao.first;
		if (icao.second && !icao.first.empty() && name.second && !name.first.empty())
			icao_name += " ";
		if (name.second && !name.first.empty())
			icao_name += name.first;
		if (!coord.second)
			coord.first.set_invalid();
		m_crossingrowchangeconn.block();
		while (m_crossingstore->children().size() <= index.first) {
			Gtk::TreeIter iter(m_crossingstore->append());
			Gtk::TreeModel::Row row(*iter);
			{
				Point pt;
				pt.set_invalid();
				row[m_crossingcolumns.m_coord] = pt;
			}
			row[m_crossingcolumns.m_ident] = "?";
			row[m_crossingcolumns.m_name] = "";
			row[m_crossingcolumns.m_radius] = 0;
			row[m_crossingcolumns.m_minlevel] = 0;
			row[m_crossingcolumns.m_maxlevel] = 600;
		}
		Gtk::TreeModel::Row row(m_crossingstore->children()[index.first]);
		row[m_crossingcolumns.m_ident] = (icao.second && !icao.first.empty()) ? icao.first : std::string();
		row[m_crossingcolumns.m_name] = icao_name;
		row[m_crossingcolumns.m_radius] = radius.first;
		row[m_crossingcolumns.m_minlevel] = minlevel.first;
		row[m_crossingcolumns.m_maxlevel] = maxlevel.first;
		m_crossingrowchangeconn.unblock();
	}
	if (cmd.get_cmdname() == "enroute") {
		std::pair<const std::string&,bool> cmdseq(cmd.get_option("cmdseq"));
		if (!cmdseq.second || cmdseq.first != "init")
			return;
		std::pair<double,bool> dctlimit(cmd.get_option_float("dctlimit"));
		if (dctlimit.second && m_fplautoroutespinbuttondctlimit)
			m_fplautoroutespinbuttondctlimit->set_value(dctlimit.first);
		std::pair<double,bool> dctpenalty(cmd.get_option_float("dctpenalty"));
		if (dctpenalty.second && m_fplautoroutespinbuttondctpenalty)
			m_fplautoroutespinbuttondctpenalty->set_value(dctpenalty.first);
		std::pair<double,bool> dctoffset(cmd.get_option_float("dctoffset"));
		if (dctoffset.second && m_fplautoroutespinbuttondctoffset)
			m_fplautoroutespinbuttondctoffset->set_value(dctoffset.first);
		std::pair<double,bool> stdroutepenalty(cmd.get_option_float("stdroutepenalty"));
		if (stdroutepenalty.second && m_fplautoroutespinbuttonstdroutepenalty)
			m_fplautoroutespinbuttonstdroutepenalty->set_value(stdroutepenalty.first);
		std::pair<double,bool> stdrouteoffset(cmd.get_option_float("stdrouteoffset"));
		if (stdrouteoffset.second && m_fplautoroutespinbuttonstdrouteoffset)
			m_fplautoroutespinbuttonstdrouteoffset->set_value(stdrouteoffset.first);
		std::pair<double,bool> stdroutedctpenalty(cmd.get_option_float("stdroutedctpenalty"));
		if (stdroutedctpenalty.second && m_fplautoroutespinbuttonstdroutedctpenalty)
			m_fplautoroutespinbuttonstdroutedctpenalty->set_value(stdroutedctpenalty.first);
		std::pair<double,bool> stdroutedctoffset(cmd.get_option_float("stdroutedctoffset"));
		if (stdroutedctoffset.second && m_fplautoroutespinbuttonstdroutedctoffset)
			m_fplautoroutespinbuttonstdroutedctoffset->set_value(stdroutedctoffset.first);
		std::pair<double,bool> ukcasdctpenalty(cmd.get_option_float("ukcasdctpenalty"));
		if (ukcasdctpenalty.second && m_fplautoroutespinbuttonukcasdctpenalty)
			m_fplautoroutespinbuttonukcasdctpenalty->set_value(ukcasdctpenalty.first);
		std::pair<double,bool> ukcasdctoffset(cmd.get_option_float("ukcasdctoffset"));
		if (ukcasdctoffset.second && m_fplautoroutespinbuttonukcasdctoffset)
			m_fplautoroutespinbuttonukcasdctoffset->set_value(ukcasdctoffset.first);
		std::pair<double,bool> ukocasdctpenalty(cmd.get_option_float("ukocasdctpenalty"));
		if (ukocasdctpenalty.second && m_fplautoroutespinbuttonukocasdctpenalty)
			m_fplautoroutespinbuttonukocasdctpenalty->set_value(ukocasdctpenalty.first);
		std::pair<double,bool> ukocasdctoffset(cmd.get_option_float("ukocasdctoffset"));
		if (ukocasdctoffset.second && m_fplautoroutespinbuttonukocasdctoffset)
			m_fplautoroutespinbuttonukocasdctoffset->set_value(ukocasdctoffset.first);
		std::pair<double,bool> ukenterdctpenalty(cmd.get_option_float("ukenterdctpenalty"));
		if (ukenterdctpenalty.second && m_fplautoroutespinbuttonukenterdctpenalty)
			m_fplautoroutespinbuttonukenterdctpenalty->set_value(ukenterdctpenalty.first);
		std::pair<double,bool> ukenterdctoffset(cmd.get_option_float("ukenterdctoffset"));
		if (ukenterdctoffset.second && m_fplautoroutespinbuttonukenterdctoffset)
			m_fplautoroutespinbuttonukenterdctoffset->set_value(ukenterdctoffset.first);
		std::pair<double,bool> ukleavedctpenalty(cmd.get_option_float("ukleavedctpenalty"));
		if (ukleavedctpenalty.second && m_fplautoroutespinbuttonukleavedctpenalty)
			m_fplautoroutespinbuttonukleavedctpenalty->set_value(ukleavedctpenalty.first);
		std::pair<double,bool> ukleavedctoffset(cmd.get_option_float("ukleavedctoffset"));
		if (ukleavedctoffset.second && m_fplautoroutespinbuttonukleavedctoffset)
			m_fplautoroutespinbuttonukleavedctoffset->set_value(ukleavedctoffset.first);
		std::pair<double,bool> vfraspclimit(cmd.get_option_float("vfraspclimit"));
		if (vfraspclimit.second && m_fplautoroutespinbuttonvfraspclimit)
			m_fplautoroutespinbuttonvfraspclimit->set_value(vfraspclimit.first);
		std::pair<unsigned long,bool> forceenrouteifr(cmd.get_option_uint("forceenrouteifr"));
		if (forceenrouteifr.second && m_fplautoroutecheckbuttonforceenrouteifr)
			m_fplautoroutecheckbuttonforceenrouteifr->set_active(!!forceenrouteifr.first);
		std::pair<unsigned long,bool> honourawylevels(cmd.get_option_uint("honourawylevels"));
		if (honourawylevels.second && m_fplautoroutecheckbuttonawylevels)
			m_fplautoroutecheckbuttonawylevels->set_active(!!honourawylevels.first);
		return;
	}
	if (cmd.get_cmdname() == "preferred") {
		std::pair<const std::string&,bool> cmdseq(cmd.get_option("cmdseq"));
		if (!cmdseq.second || cmdseq.first != "init")
			return;
		std::pair<long,bool> minlevel(cmd.get_option_int("minlevel"));
		if (minlevel.second && m_fplautoroutespinbuttonpreferredminlevel)
			m_fplautoroutespinbuttonpreferredminlevel->set_value(minlevel.first);
		std::pair<long,bool> maxlevel(cmd.get_option_int("maxlevel"));
		if (maxlevel.second && m_fplautoroutespinbuttonpreferredmaxlevel)
			m_fplautoroutespinbuttonpreferredmaxlevel->set_value(maxlevel.first);
		std::pair<double,bool> penalty(cmd.get_option_float("penalty"));
		if (penalty.second && m_fplautoroutespinbuttonpreferredpenalty)
			m_fplautoroutespinbuttonpreferredpenalty->set_value(penalty.first);
		std::pair<double,bool> climb(cmd.get_option_float("climb"));
		if (climb.second && m_fplautoroutespinbuttonpreferredclimb)
			m_fplautoroutespinbuttonpreferredclimb->set_value(climb.first);
		std::pair<double,bool> descent(cmd.get_option_float("descent"));
		if (descent.second && m_fplautoroutespinbuttonpreferreddescent)
			m_fplautoroutespinbuttonpreferreddescent->set_value(descent.first);
	}
	if (cmd.get_cmdname() == "tfr") {
		std::pair<unsigned long,bool> available(cmd.get_option_uint("available"));
		if (available.second && m_fplautoroutecheckbuttontfr)
			m_fplautoroutecheckbuttontfr->set_sensitive(!!available.first);
		std::pair<const std::string&,bool> cmdseq(cmd.get_option("cmdseq"));
		if (!cmdseq.second || cmdseq.first != "init")
			return;
		std::pair<unsigned long,bool> enabled(cmd.get_option_uint("enabled"));
		std::pair<const std::string&,bool> trace(cmd.get_option("trace"));
		std::pair<const std::string&,bool> disable(cmd.get_option("disable"));
		std::pair<unsigned long,bool> precompgraph(cmd.get_option_uint("precompgraph"));
		std::pair<const std::string&,bool> validator(cmd.get_option("validator"));
		if (enabled.second && m_fplautoroutecheckbuttontfr)
			m_fplautoroutecheckbuttontfr->set_active(!!enabled.first);
		if (precompgraph.second && m_fplautoroutecheckbuttonprecompgraph)
			m_fplautoroutecheckbuttonprecompgraph->set_active(!!precompgraph.first);
		if (validator.second && m_fplautoroutecomboboxvalidator) {
			if (validator.first == "default")
				m_fplautoroutecomboboxvalidator->set_active(0);
			else if (validator.first == "cfmu")
				m_fplautoroutecomboboxvalidator->set_active(1);
			else if (validator.first == "eurofpl")
				m_fplautoroutecomboboxvalidator->set_active(2);
		}
		if (trace.second && m_tracerulesstore) {
			m_tracerulesstore->clear();
			std::string::size_type n(0);
			for (;;) {
				std::string::size_type n1(trace.first.find(',', n));
				if (n1 > n) {
					std::string txt(trace.first.substr(n, n1 - n));
					if (!txt.empty()) {
						Gtk::TreeIter iter(m_tracerulesstore->append());
						Gtk::TreeModel::Row row(*iter);
						row[m_rulescolumns.m_rule] = txt;
					}
				}
				if (n1 == std::string::npos)
					break;
				n = n1 + 1;
			}
		}
		if (disable.second && m_disablerulesstore) {
			m_disablerulesstore->clear();
			std::string::size_type n(0);
			for (;;) {
				std::string::size_type n1(disable.first.find(',', n));
				if (n1 > n) {
					std::string txt(disable.first.substr(n, n1 - n));
					if (!txt.empty()) {
						Gtk::TreeIter iter(m_disablerulesstore->append());
						Gtk::TreeModel::Row row(*iter);
						row[m_rulescolumns.m_rule] = txt;
					}
				}
				if (n1 == std::string::npos)
					break;
				n = n1 + 1;
			}
		}
		return;
	}
	if (cmd.get_cmdname() == "atmosphere") {
		std::pair<const std::string&,bool> cmdseq(cmd.get_option("cmdseq"));
		if (!cmdseq.second || cmdseq.first != "init")
			return;
		std::pair<unsigned long,bool> enabled(cmd.get_option_uint("wind"));
		if (enabled.second && m_fplautoroutecheckbuttonwind)
			m_fplautoroutecheckbuttonwind->set_active(!!enabled.first);
		return;
	}
	if (cmd.get_cmdname() == "levels") {
		std::pair<const std::string&,bool> cmdseq(cmd.get_option("cmdseq"));
		if (!cmdseq.second || cmdseq.first != "init")
			return;
		std::pair<long,bool> base(cmd.get_option_int("base"));
		if (base.second && m_fplautoroutespinbuttonbaselevel &&
		    !(m_sensors && m_sensors->get_configfile().has_key(cfggroup_mainwindow, "autoroutebaselevel")))
			m_fplautoroutespinbuttonbaselevel->set_value(base.first);
		std::pair<long,bool> top(cmd.get_option_int("top"));
		if (top.second && m_fplautoroutespinbuttontoplevel &&
		    !(m_sensors && m_sensors->get_configfile().has_key(cfggroup_mainwindow, "autoroutetoplevel")))
			m_fplautoroutespinbuttontoplevel->set_value(top.first);
		std::pair<long,bool> minlevel(cmd.get_option_int("minlevel"));
		if (minlevel.second && m_fplautoroutespinbuttonminlevel)
			m_fplautoroutespinbuttonminlevel->set_value(minlevel.first);
		std::pair<double,bool> minlevelpenalty(cmd.get_option_float("minlevelpenalty"));
		if (minlevelpenalty.second && m_fplautoroutespinbuttonminlevelpenalty)
			m_fplautoroutespinbuttonminlevelpenalty->set_value(minlevelpenalty.first);
		std::pair<double,bool> minlevelmaxpenalty(cmd.get_option_float("minlevelmaxpenalty"));
		if (minlevelmaxpenalty.second && m_fplautoroutespinbuttonminlevelmaxpenalty)
			m_fplautoroutespinbuttonminlevelmaxpenalty->set_value(minlevelmaxpenalty.first);
		return;
	}
}

void FlightDeckWindow::FPLAutoRouteDialog::pg1_close_clicked(void)
{
	close();
}

void FlightDeckWindow::FPLAutoRouteDialog::pg1_copy_clicked(void)
{
	const FPlanRoute& route2(m_proxy.get_fplan());
	if (!m_sensors || !route2.get_nrwpt()) {
		close();
		return;
	}
	// read airway graph to add intermediate non-flightplannable waypoints
	if (false) {
		Engine::AirwayGraphResult::Graph areagraph;
		IcaoFlightPlan::FindCoord f(*m_sensors->get_engine());
		f.find_area(route2.get_bbox().oversize_nmi(200.f),
			    Engine::AirwayGraphResult::Vertex::typemask_awyendpoints |
			    Engine::AirwayGraphResult::Vertex::typemask_airway);
		f.get_airwaygraph(areagraph);
		areagraph.delete_disconnected_vertices();
	}
	FPlanRoute& route(m_sensors->get_route());
	while (route.get_nrwpt() > 0)
		m_sensors->nav_delete_wpt(0);
	time_t toffs(route2[0].get_time_unix());
	{
		time_t curt(time(0));
		if (toffs >= curt) {
			toffs = 0;
		} else {
			curt /= 24 * 60 * 60;
			curt *= 24 * 60 * 60;
			curt += toffs % (24 * 60 * 60);
			toffs = curt - toffs;
			if (toffs < 0)
				toffs += 24 * 60 * 60;
			struct tm utm;
			gmtime_r(&toffs, &utm);
			utm.tm_hour = utm.tm_min = utm.tm_sec = 0;
			toffs = timegm(&utm);
		}
	}
	typedef std::map<Glib::ustring, Engine::AirwayGraphResult::Graph> awygraph_t;
	awygraph_t awygraph;
	for (unsigned int i = 0; i < route2.get_nrwpt(); ++i) {
		FPlanWaypoint wpt(route2[i]);
		wpt.set_time_unix(wpt.get_time_unix() + toffs);
		m_sensors->nav_insert_wpt(~0U, wpt);
		if (!i) {
			uint16_t isifr(wpt.get_flags() & FPlanWaypoint::altflag_ifr);
			if (!isifr && route2.get_flightrules() == 'V')
				continue;
			if (false)
				std::cerr << "SID: " << wpt.get_icao() << " dep " << m_departure.get_icao() << std::endl;
			if (wpt.get_icao() != m_departure.get_icao())
				continue;
			if (i + 1U >= route2.get_nrwpt())
				continue;
			const FPlanWaypoint& wpt1(route2[i + 1]);
			unsigned int j(~0U);
			{
				double bestdist(std::numeric_limits<double>::max());
				for (unsigned int j1 = 0, k = m_departure.get_nrvfrrte(); j1 < k; ++j1) {
					const AirportsDb::Airport::VFRRoute& rte(m_departure.get_vfrrte(j1));
					if (rte.size() < 2 || rte.get_name().empty())
						continue;
					unsigned int m(rte.size()-1);
					const AirportsDb::Airport::VFRRoute::VFRRoutePoint& rtept(rte[m]);
					if (rtept.get_pathcode() != (isifr ? AirportsDb::Airport::VFRRoute::VFRRoutePoint::pathcode_sid :
								     AirportsDb::Airport::VFRRoute::VFRRoutePoint::pathcode_departure))
						continue;
					const AirportsDb::Airport::VFRRoute::VFRRoutePoint& rtepta(rte[0]);
					if (false)
						std::cerr << "SID: " << rte.get_name()
							  << " aprt dist " << rtepta.get_coord().spheric_distance_nmi_dbl(m_departure.get_coord())
							  << " end dist " << rtept.get_coord().spheric_distance_nmi_dbl(wpt1.get_coord()) << std::endl;
					if (rtepta.get_coord().spheric_distance_nmi_dbl(m_departure.get_coord()) > 3.0)
						continue;
					double dist(rtept.get_coord().spheric_distance_nmi_dbl(wpt1.get_coord()));
					if (dist < bestdist) {
						bestdist = dist;
						j = j1;
					}
				}
				if (j >= m_departure.get_nrvfrrte() || (isifr && bestdist > 0.1))
					continue;
			}
			const AirportsDb::Airport::VFRRoute& rte(m_departure.get_vfrrte(j));
			unsigned int m(rte.size()-1);
			const AirportsDb::Airport::VFRRoute::VFRRoutePoint& rtept(rte[m]);
			const AirportsDb::Airport::VFRRoute::VFRRoutePoint& rtepta(rte[0]);
			std::string rtename(rte.get_name());
			{
				std::string::size_type n(0);
				while (n < rtename.size() && std::isalpha(rtename[n]))
					++n;
				if (n <= rtept.get_name().size() && rtename.substr(0, n) == rtept.get_name().substr(0, n))
					rtename = rtept.get_name() + rtename.substr(n);
			}
			{
				FPlanRoute& route(m_sensors->get_route());
				FPlanWaypoint& wpt(route[route.get_nrwpt() - 1]);
				wpt.set_pathname(rtename);
				wpt.set_pathcode(rtepta.get_fplan_pathcode());					
			}
			if (!isifr)
				++m;
			for (unsigned int l = 1; l < m; ++l) {
				FPlanWaypoint wpt2;
				const AirportsDb::Airport::VFRRoute::VFRRoutePoint& vfrpt(rte[l]);
				//wpt2.set_icao(m_departure.get_icao());
				wpt2.set_icao("");
				wpt2.set_name(vfrpt.get_name());
				wpt2.set_coord(vfrpt.get_coord());
				wpt2.set_flags(isifr);
				wpt2.set_altitude(vfrpt.get_altitude());
				if (vfrpt.get_altitude() >= wpt1.get_altitude()) {
					wpt2.set_flags((wpt1.get_flags() & ~FPlanWaypoint::altflag_ifr) | isifr);
					wpt2.set_altitude(wpt1.get_altitude());
				}
				wpt2.set_frequency(0);
				Glib::ustring note(vfrpt.get_altcode_string());
				if (vfrpt.is_compulsory())
					note += " Compulsory";
				note += "\n" + vfrpt.get_pathcode_string();
				wpt2.set_note(note);
				wpt2.set_pathcode(vfrpt.get_fplan_pathcode());
				wpt2.set_pathname(rtename);
				m_sensors->nav_insert_wpt(~0U, wpt2);
			}
			continue;
		}
		if (i + 2 == route2.get_nrwpt()) {
			const FPlanWaypoint& wpt1(route2[i + 1]);
			uint16_t isifr(wpt1.get_flags() & FPlanWaypoint::altflag_ifr);
			if (!isifr && route2.get_flightrules() == 'V')
				continue;
			if (false)
				std::cerr << "STAR: " << wpt1.get_icao() << " dest " << m_destination.get_icao() << std::endl;
			if (wpt1.get_icao() != m_destination.get_icao())
				continue;
			unsigned int j(~0U);
			{
				double bestdist(std::numeric_limits<double>::max());
				for (unsigned int j1 = 0, k = m_destination.get_nrvfrrte(); j1 < k; ++j1) {
					const AirportsDb::Airport::VFRRoute& rte(m_destination.get_vfrrte(j1));
					if (rte.size() < 2 || rte.get_name().empty())
						continue;
					unsigned int m(rte.size()-1);
					const AirportsDb::Airport::VFRRoute::VFRRoutePoint& rtept(rte[0]);
					if (rtept.get_pathcode() != (isifr ? AirportsDb::Airport::VFRRoute::VFRRoutePoint::pathcode_star :
								     AirportsDb::Airport::VFRRoute::VFRRoutePoint::pathcode_arrival))
						continue;
					const AirportsDb::Airport::VFRRoute::VFRRoutePoint& rtepta(rte[m]);
					if (false)
						std::cerr << "STAR: " << rte.get_name()
							  << " aprt dist " << rtepta.get_coord().spheric_distance_nmi_dbl(m_destination.get_coord())
							  << " end dist " << rtept.get_coord().spheric_distance_nmi_dbl(wpt.get_coord()) << std::endl;
					if (rtepta.get_coord().spheric_distance_nmi_dbl(m_destination.get_coord()) > 3.0)
						continue;
					double dist(rtept.get_coord().spheric_distance_nmi_dbl(wpt.get_coord()));
					if (dist < bestdist) {
						bestdist = dist;
						j = j1;
					}
				}
				if (j >= m_destination.get_nrvfrrte() || (isifr && bestdist > 0.1))
					continue;
			}
			const AirportsDb::Airport::VFRRoute& rte(m_destination.get_vfrrte(j));
			unsigned int m(rte.size()-1);
			const AirportsDb::Airport::VFRRoute::VFRRoutePoint& rtept(rte[0]);
			const AirportsDb::Airport::VFRRoute::VFRRoutePoint& rtepta(rte[m]);
			std::string rtename(rte.get_name());
			{
				std::string::size_type n(0);
				while (n < rtename.size() && std::isalpha(rtename[n]))
					++n;
				if (n <= rtept.get_name().size() && rtename.substr(0, n) == rtept.get_name().substr(0, n))
					rtename = rtept.get_name() + rtename.substr(n);
			}
			{
				FPlanRoute& route(m_sensors->get_route());
				FPlanWaypoint& wpt(route[route.get_nrwpt() - 1]);
				wpt.set_pathname(rtename);
				wpt.set_pathcode(rtepta.get_fplan_pathcode());					
			}
			for (unsigned int l = !!isifr; l < m; ++l) {
				FPlanWaypoint wpt2;
				const AirportsDb::Airport::VFRRoute::VFRRoutePoint& vfrpt(rte[l]);
				//wpt2.set_icao(m_destination.get_icao());
				wpt2.set_icao("");
				wpt2.set_name(vfrpt.get_name());
				wpt2.set_coord(vfrpt.get_coord());
				wpt2.set_flags(isifr);
				wpt2.set_altitude(vfrpt.get_altitude());
				if (vfrpt.get_altitude() >= wpt.get_altitude()) {
					wpt2.set_flags((wpt.get_flags() & ~FPlanWaypoint::altflag_ifr) | isifr);
					wpt2.set_altitude(wpt.get_altitude());
				}
				wpt2.set_frequency(0);
				Glib::ustring note(vfrpt.get_altcode_string());
				if (vfrpt.is_compulsory())
					note += " Compulsory";
				note += "\n" + vfrpt.get_pathcode_string();
				wpt2.set_note(note);
				wpt2.set_pathcode(vfrpt.get_fplan_pathcode());
				wpt2.set_pathname(rtename);
				m_sensors->nav_insert_wpt(~0U, wpt2);
			}
			continue;
		}
		// try to find an intermediate airway point
		if (wpt.get_pathcode() == FPlanWaypoint::pathcode_airway && !wpt.get_pathname().empty()) {
			if (!wpt.is_ifr())
				continue;
			awygraph_t::iterator agi(awygraph.find(wpt.get_pathname()));
			if (agi == awygraph.end()) {
				std::pair<awygraph_t::iterator,bool> ins(awygraph.insert(std::make_pair(wpt.get_pathname(),
													Engine::AirwayGraphResult::Graph())));
				agi = ins.first;
				IcaoFlightPlan::FindCoord f(*m_sensors->get_engine());
				if (false)
					std::cerr << "Loading airway graph " << wpt.get_pathname() << std::endl;
				f.find_airway(wpt.get_pathname());
				f.get_airwaygraph(ins.first->second);
			}
			Engine::AirwayGraphResult::Graph& agraph(agi->second);
			Engine::AirwayGraphResult::Graph::vertex_descriptor u0;
			if (!agraph.find_nearest(u0, wpt.get_coord(), wpt.get_pathname(),
						 Engine::AirwayGraphResult::Vertex::typemask_navaid |
						 Engine::AirwayGraphResult::Vertex::typemask_intersection |
						 Engine::AirwayGraphResult::Vertex::typemask_undefined,
						 true, false))
				continue;
			const FPlanWaypoint& wpt1(route2[i + 1]);
			const Engine::AirwayGraphResult::Vertex& v0(agraph[u0]);
			if (v0.get_ident() != wpt.get_ident() ||
			    wpt.get_coord().spheric_distance_nmi(v0.get_coord()) > 0.1)
				continue;
			Engine::AirwayGraphResult::Graph::vertex_descriptor u1;
			if (!agraph.find_nearest(u1, wpt1.get_coord(), wpt.get_pathname(),
						 Engine::AirwayGraphResult::Vertex::typemask_navaid |
						 Engine::AirwayGraphResult::Vertex::typemask_intersection |
						 Engine::AirwayGraphResult::Vertex::typemask_undefined,
						 false, true))
				continue;
			if (u0 == u1)
				continue;
			const Engine::AirwayGraphResult::Vertex& v1(agraph[u1]);
			if (v1.get_ident() != wpt1.get_ident() ||
			    wpt1.get_coord().spheric_distance_nmi(v1.get_coord()) > 0.1)
				continue;
			std::vector<Engine::AirwayGraphResult::Graph::vertex_descriptor> predecessors;
			agraph.shortest_paths(u0, predecessors);
			Engine::AirwayGraphResult::Graph::vertex_descriptor uu(predecessors[u1]);
			if (uu == u1)
				continue;
			unsigned int inspt(route.get_nrwpt());
			for (; uu != u0; uu = predecessors[uu]) {
				const Engine::AirwayGraphResult::Vertex& vv(agraph[uu]);
				FPlanWaypoint wpt2;
				if (!vv.set(wpt2)) {
					wpt2.set_name(vv.get_ident());
					wpt2.set_coord(vv.get_coord());
				}
				wpt2.set_pathcode(wpt.get_pathcode());
				wpt2.set_pathname(wpt.get_pathname());
				wpt2.set_time(wpt.get_time());
				wpt2.set_altitude(wpt.get_altitude());
				wpt2.set_flags(wpt.get_flags());
				wpt2.set_winddir(wpt.get_winddir());
				wpt2.set_windspeed(wpt.get_windspeed());
				wpt2.set_qff(wpt.get_qff());
				wpt2.set_isaoffset(wpt.get_isaoffset());
				m_sensors->nav_insert_wpt(inspt, wpt2);
				if (false)
					std::cerr << "Inserting " << wpt.get_name() << std::endl;
			}
                }
	}
	//route.set_time_offblock_unix(route2[0].get_time_unix() + toffs - 5 * 60);
	//route.set_time_onblock_unix(route2[route2.get_nrwpt() - 1].get_time_unix() + toffs + 5 * 60);
	//m_sensors->nav_fplan_modified();
	m_sensors->nav_fplan_setdeparture(route2.get_time_offblock_unix() + toffs);
	close();
}

void FlightDeckWindow::FPLAutoRouteDialog::pg1_next_clicked(void)
{
	if (!m_fplautoroutenotebook) {
		close();
		return;
	}
	if (m_fplautorouteentrypg2dep)
		m_fplautorouteentrypg2dep->set_text("");
	if (m_fplautorouteentrypg2dest)
		m_fplautorouteentrypg2dest->set_text("");
	if (m_fplautorouteentrypg2gcdist)
		m_fplautorouteentrypg2gcdist->set_text("");
	if (m_fplautorouteentrypg2routedist)
		m_fplautorouteentrypg2routedist->set_text("");
	if (m_fplautorouteentrypg2mintime)
		m_fplautorouteentrypg2mintime->set_text("");
	if (m_fplautorouteentrypg2routetime)
		m_fplautorouteentrypg2routetime->set_text("");
	if (m_fplautorouteentrypg2minfuel)
		m_fplautorouteentrypg2minfuel->set_text("");
	if (m_fplautorouteentrypg2routefuel)
		m_fplautorouteentrypg2routefuel->set_text("");
	if (m_fplautorouteentrypg2iteration)
		m_fplautorouteentrypg2iteration->set_text("");
	if (m_fplautorouteentrypg2times)
		m_fplautorouteentrypg2times->set_text("");
	m_fplautoroutenotebook->set_current_page(1);
	if (m_fplautoroutenotebookpg2)
		m_fplautoroutenotebookpg2->set_current_page(0);
#ifdef HAVE_JSONCPP
	if (m_childsockaddr) {
		try {
			Json::Value root;
			root["session"] = m_childsession;
			Json::Value cmds;
			if (m_sensors) {
				Json::Value cmd;
				cmd["cmdname"] = "aircraft";
				if (false && !m_sensors->get_aircraftfile().empty())
					cmd["file"] = (std::string)m_sensors->get_aircraftfile();
				else
					cmd["data"] = (std::string)m_sensors->get_aircraft().save_string();
			        cmds.append(cmd);
			}
			{
				Json::Value cmd;
				cmd["cmdname"] = "clear";
				cmds.append(cmd);
			}
			if (m_departure.is_valid()) {
				Json::Value cmd;
				cmd["cmdname"] = "departure";
				cmd["icao"] = (std::string)m_departure.get_icao();
				cmd["name"] = (std::string)m_departure.get_name();
				cmd["coordlat"] = m_departure.get_coord().get_lat();
				cmd["coordlon"] = m_departure.get_coord().get_lon();
				cmd[m_depifr ? "ifr" : "vfr"] = true;
				if (!m_sidend.is_invalid()) {
					cmd["sidlat"] = m_sidend.get_lat();
					cmd["sidlon"] = m_sidend.get_lon();
				}
				if (m_fplautoroutespinbuttonsidlimit)
					cmd["sidlimit"] = m_fplautoroutespinbuttonsidlimit->get_value();
				if (m_fplautoroutespinbuttonsidpenalty)
					cmd["sidpenalty"] = m_fplautoroutespinbuttonsidpenalty->get_value();
				if (m_fplautoroutespinbuttonsidoffset)
					cmd["sidoffset"] = m_fplautoroutespinbuttonsidoffset->get_value();
				if (m_fplautoroutespinbuttonsidminimum)
					cmd["sidminimum"] = m_fplautoroutespinbuttonsidminimum->get_value();
				if (m_fplautoroutecheckbuttonsiddb)
					cmd["siddb"] = m_fplautoroutecheckbuttonsiddb->get_active();
				if (m_fplautoroutecheckbuttonsidonly)
					cmd["sidonly"] = m_fplautoroutecheckbuttonsidonly->get_active();
				{
					guint year(1970), month(0), day(1);
					if (m_fplautoroutecalendar) {
						m_fplautoroutecalendar->get_date(year, month, day);
						++month;
					}
					gint hour(10), min(0);
					if (m_fplautoroutespinbuttonhour)
						hour = m_fplautoroutespinbuttonhour->get_value_as_int();
					if (m_fplautoroutespinbuttonmin)
						min = m_fplautoroutespinbuttonmin->get_value_as_int();
					Glib::DateTime dt(Glib::DateTime::create_utc(year, month, day, hour, min, 0));
					Glib::TimeVal tv;
					if (dt.to_timeval(tv))
						cmd["date"] = (std::string)tv.as_iso8601();
				}
				cmds.append(cmd);
        		}
			if (m_destination.is_valid()) {
				Json::Value cmd;
				cmd["cmdname"] = "destination";
				cmd["icao"] = (std::string)m_destination.get_icao();
				cmd["name"] = (std::string)m_destination.get_name();
				cmd["coordlat"] = m_destination.get_coord().get_lat();
				cmd["coordlon"] = m_destination.get_coord().get_lon();
				cmd[m_destifr ? "ifr" : "vfr"] = true;
				if (!m_starstart.is_invalid()) {
					cmd["starlat"] = m_starstart.get_lat();
					cmd["starlon"] = m_starstart.get_lon();
				}
			        if (m_fplautoroutespinbuttonstarlimit)
					cmd["starlimit"] = m_fplautoroutespinbuttonstarlimit->get_value();
				if (m_fplautoroutespinbuttonstarpenalty)
					cmd["starpenalty"] = m_fplautoroutespinbuttonstarpenalty->get_value();
				if (m_fplautoroutespinbuttonstaroffset)
					cmd["staroffset"] = m_fplautoroutespinbuttonstaroffset->get_value();
				if (m_fplautoroutespinbuttonstarminimum)
					cmd["starminimum"] = m_fplautoroutespinbuttonstarminimum->get_value();
				if (m_fplautoroutecheckbuttonstardb)
					cmd["stardb"] = m_fplautoroutecheckbuttonstardb->get_active();
				if (m_fplautoroutecheckbuttonstaronly)
					cmd["staronly"] = m_fplautoroutecheckbuttonstaronly->get_active();
				cmds.append(cmd);
        		}
			if (m_crossingstore) {
				unsigned int xvalid(0);
				for (Gtk::TreeIter iter(m_crossingstore->children().begin()); iter; ++iter) {
					Gtk::TreeModel::Row row(*iter);
					if (!static_cast<Point>(row[m_crossingcolumns.m_coord]).is_invalid())
						++xvalid;
				}
				unsigned int index(0);
				for (Gtk::TreeIter iter(m_crossingstore->children().begin()); iter; ++iter) {
					Gtk::TreeModel::Row row(*iter);
					if (static_cast<Point>(row[m_crossingcolumns.m_coord]).is_invalid())
						continue;
					Json::Value cmd;
					cmd["cmdname"] = "crossing";
					cmd["coordlat"] = static_cast<Point>(row[m_crossingcolumns.m_coord]).get_lat();
					cmd["coordlon"] = static_cast<Point>(row[m_crossingcolumns.m_coord]).get_lon();
					if (!std::isnan(static_cast<double>(row[m_crossingcolumns.m_radius])))
						cmd["radius"] = static_cast<double>(row[m_crossingcolumns.m_radius]);
					cmd["minlevel"] = static_cast<unsigned int>(row[m_crossingcolumns.m_minlevel]);
					cmd["maxlevel"] = static_cast<unsigned int>(row[m_crossingcolumns.m_maxlevel]);
					cmd["index"] = index;
					cmd["count"] = xvalid;
					cmds.append(cmd);
					++index;
				}
			}
			{
				Json::Value cmd;
				cmd["cmdname"] = "enroute";
				if (m_fplautoroutespinbuttondctlimit)
					cmd["dctlimit"] = m_fplautoroutespinbuttondctlimit->get_value();
				if (m_fplautoroutespinbuttondctpenalty)
					cmd["dctpenalty"] = m_fplautoroutespinbuttondctpenalty->get_value();
				if (m_fplautoroutespinbuttondctoffset)
					cmd["dctoffset"] = m_fplautoroutespinbuttondctoffset->get_value();
				if (m_fplautoroutespinbuttonstdroutepenalty)
					cmd["stdroutepenalty"] = m_fplautoroutespinbuttonstdroutepenalty->get_value();
				if (m_fplautoroutespinbuttonstdrouteoffset)
					cmd["stdrouteoffset"] = m_fplautoroutespinbuttonstdrouteoffset->get_value();
				if (m_fplautoroutespinbuttonstdroutedctpenalty)
					cmd["stdroutedctpenalty"] = m_fplautoroutespinbuttonstdroutedctpenalty->get_value();
				if (m_fplautoroutespinbuttonstdroutedctoffset)
					cmd["stdroutedctoffset"] = m_fplautoroutespinbuttonstdroutedctoffset->get_value();
				if (m_fplautoroutespinbuttonukcasdctpenalty)
					cmd["ukcasdctpenalty"] = m_fplautoroutespinbuttonukcasdctpenalty->get_value();
				if (m_fplautoroutespinbuttonukcasdctoffset)
					cmd["ukcasdctoffset"] = m_fplautoroutespinbuttonukcasdctoffset->get_value();
				if (m_fplautoroutespinbuttonukocasdctpenalty)
					cmd["ukocasdctpenalty"] = m_fplautoroutespinbuttonukocasdctpenalty->get_value();
				if (m_fplautoroutespinbuttonukocasdctoffset)
					cmd["ukocasdctoffset"] = m_fplautoroutespinbuttonukocasdctoffset->get_value();
				if (m_fplautoroutespinbuttonukenterdctpenalty)
					cmd["ukenterdctpenalty"] = m_fplautoroutespinbuttonukenterdctpenalty->get_value();
				if (m_fplautoroutespinbuttonukenterdctoffset)
					cmd["ukenterdctoffset"] = m_fplautoroutespinbuttonukenterdctoffset->get_value();
				if (m_fplautoroutespinbuttonukleavedctpenalty)
					cmd["ukleavedctpenalty"] = m_fplautoroutespinbuttonukleavedctpenalty->get_value();
				if (m_fplautoroutespinbuttonukleavedctoffset)
					cmd["ukleavedctoffset"] = m_fplautoroutespinbuttonukleavedctoffset->get_value();
				if (m_fplautoroutespinbuttonvfraspclimit)
					cmd["vfraspclimit"] = m_fplautoroutespinbuttonvfraspclimit->get_value();
				if (m_fplautoroutecheckbuttonforceenrouteifr)
					cmd["forceenrouteifr"] = m_fplautoroutecheckbuttonforceenrouteifr->get_active();
				if (m_fplautoroutecheckbuttonawylevels)
					cmd["honourawylevels"] = m_fplautoroutecheckbuttonawylevels->get_active();
				cmds.append(cmd);
			}
			{
				Json::Value cmd;
				cmd["cmdname"] = "preferred";
				if (m_fplautoroutespinbuttonpreferredminlevel)
					cmd["minlevel"] = m_fplautoroutespinbuttonpreferredminlevel->get_value();
				if (m_fplautoroutespinbuttonpreferredmaxlevel)
					cmd["maxlevel"] = m_fplautoroutespinbuttonpreferredmaxlevel->get_value();
				if (m_fplautoroutespinbuttonpreferredpenalty)
					cmd["penalty"] = m_fplautoroutespinbuttonpreferredpenalty->get_value();
				if (m_fplautoroutespinbuttonpreferredclimb)
					cmd["climb"] = m_fplautoroutespinbuttonpreferredclimb->get_value();
				if (m_fplautoroutespinbuttonpreferreddescent)
					cmd["descent"] = m_fplautoroutespinbuttonpreferreddescent->get_value();
				cmds.append(cmd);
			}
			{
				Json::Value cmd;
				cmd["cmdname"] = "tfr";
				if (m_fplautoroutecheckbuttontfr)
					cmd["enabled"] = m_fplautoroutecheckbuttontfr->get_active();
				if (m_fplautoroutecheckbuttonprecompgraph)
					cmd["precompgraph"] = m_fplautoroutecheckbuttonprecompgraph->get_active();
				if (m_fplautoroutecomboboxvalidator) {
					switch (m_fplautoroutecomboboxvalidator->get_active_row_number()) {
					case 0:
						cmd["validator"] = "default";
						break;

					case 1:
						cmd["validator"] = "cfmu";
						break;

					case 2:
						cmd["validator"] = "eurofpl";
						break;

					default:
						break;
					}
				}
				if (m_tracerulesstore) {
					std::string txt;
					for (Gtk::TreeIter iter(m_tracerulesstore->children().begin()); iter; ++iter) {
						Gtk::TreeModel::Row row(*iter);
						std::string txt1((Glib::ustring)row[m_rulescolumns.m_rule]);
						while (!txt1.empty() && std::isspace(txt1[txt1.size()-1]))
							txt1.erase(txt1.size() - 1, 1);
						while (!txt1.empty() && std::isspace(txt1[0]))
							txt1.erase(0, 1);
						if (txt1.empty() || txt1 == "?")
							continue;
						if (!txt.empty())
							txt += ",";
						txt += txt1;
					}
					cmd["trace"] = txt;
				}
				if (m_disablerulesstore) {
					std::string txt;
					for (Gtk::TreeIter iter(m_disablerulesstore->children().begin()); iter; ++iter) {
						Gtk::TreeModel::Row row(*iter);
						std::string txt1((Glib::ustring)row[m_rulescolumns.m_rule]);
						while (!txt1.empty() && std::isspace(txt1[txt1.size()-1]))
							txt1.erase(txt1.size() - 1, 1);
						while (!txt1.empty() && std::isspace(txt1[0]))
							txt1.erase(0, 1);
						if (txt1.empty() || txt1 == "?")
							continue;
						if (!txt.empty())
							txt += ",";
						txt += txt1;
					}
					cmd["disable"] = txt;
				}
				cmds.append(cmd);
			}
			{
				Json::Value cmd;
				cmd["cmdname"] = "atmosphere";
				if (m_fplautoroutecheckbuttonwind)
					cmd["wind"] = m_fplautoroutecheckbuttonwind->get_active();
				cmds.append(cmd);
			}
			if (m_airspacestore) {
				bool first(true);
				for (Gtk::TreeIter iter(m_airspacestore->children().begin()); iter; ++iter) {
					Gtk::TreeModel::Row row(*iter);
					Json::Value cmd;
					cmd["cmdname"] = "exclude";
					if (first) {
						cmd["clear"] = true;
						first = false;
					}
					if (static_cast<Point>(row[m_airspacecolumns.m_coord]).is_invalid()) {
						cmd["aspcid"] = (std::string)static_cast<Glib::ustring>(row[m_airspacecolumns.m_ident]);
						cmd["aspctype"] = (std::string)static_cast<Glib::ustring>(row[m_airspacecolumns.m_type]);
					} else {
						Rect bbox(static_cast<Point>(row[m_airspacecolumns.m_coord]),
							  static_cast<Point>(row[m_airspacecolumns.m_coord]));
						double miles(Glib::Ascii::strtod(static_cast<Glib::ustring>(row[m_airspacecolumns.m_type])));
						if (std::isnan(miles) || miles < 0)
							miles = 0;
						bbox = bbox.oversize_nmi(miles);
						cmd["swlat"] = bbox.get_southwest().get_lat();
						cmd["swlon"] = bbox.get_southwest().get_lon();
						cmd["nelat"] = bbox.get_northeast().get_lat();
						cmd["nelon"] = bbox.get_northeast().get_lon();
					}
					cmd["base"] = static_cast<unsigned int>(row[m_airspacecolumns.m_minlevel]);
					cmd["top"] = static_cast<unsigned int>(row[m_airspacecolumns.m_maxlevel]);
					cmd["awylimit"] = static_cast<double>(row[m_airspacecolumns.m_awylimit]);
					cmd["dctlimit"] = static_cast<double>(row[m_airspacecolumns.m_dctlimit]);
					cmd["dctoffset"] = static_cast<double>(row[m_airspacecolumns.m_dctoffset]);
					cmd["dctscale"] = static_cast<double>(row[m_airspacecolumns.m_dctscale]);
					cmds.append(cmd);
				}
				if (first) {
					Json::Value cmd;
					cmd["cmdname"] = "exclude";
					cmd["clear"] = true;
					cmds.append(cmd);
				}
			}
			{
				Json::Value cmd;
				cmd["cmdname"] = "levels";
				if (m_fplautoroutespinbuttonbaselevel)
					cmd["base"] = m_fplautoroutespinbuttonbaselevel->get_value();
				if (m_fplautoroutespinbuttontoplevel)
					cmd["top"] = m_fplautoroutespinbuttontoplevel->get_value();
				if (m_fplautoroutespinbuttonminlevel)
					cmd["minlevel"] = m_fplautoroutespinbuttonminlevel->get_value();
				if (m_fplautoroutespinbuttonminlevelpenalty)
					cmd["minlevelpenalty"] = m_fplautoroutespinbuttonminlevelpenalty->get_value();
				if (m_fplautoroutespinbuttonminlevelmaxpenalty)
					cmd["minlevelmaxpenalty"] = m_fplautoroutespinbuttonminlevelmaxpenalty->get_value();
				cmds.append(cmd);
			}
			{
				Json::Value cmd;
				cmd["cmdname"] = "atmosphere";
				if (m_sensors) {
					cmd["qnh"] = m_sensors->get_altimeter_qnh();
					cmd["isa"] = m_sensors->get_altimeter_tempoffs();
				}
				cmds.append(cmd);
			}
			{
				Json::Value cmd;
				cmd["cmdname"] = "cruise";
			        if (m_sensors) {
					Aircraft::Cruise::EngineParams ep(m_sensors->get_acft_cruise_params());
					if (!std::isnan(ep.get_rpm()))
						cmd["rpm"] = ep.get_rpm();
					if (!std::isnan(ep.get_mp()))
						cmd["mp"] = ep.get_mp();
					if (!std::isnan(ep.get_bhp()))
						cmd["bhp"] = ep.get_bhp();
				}
				cmds.append(cmd);
			}
			{
				Json::Value cmd;
				cmd["cmdname"] = "optimization";
				if (m_fplautoroutecomboboxopttarget) {
					int x(m_fplautoroutecomboboxopttarget->get_active_row_number());
					cmd["target"] = (x >= 2) ? "preferred" : x ? "fuel" : "time";
				}
				cmds.append(cmd);
			}
			{
				Json::Value cmd;
				cmd["cmdname"] = "preload";
				cmds.append(cmd);
			}
			root["cmds"] = cmds;
			sendjson(root);
		} catch (const std::exception& e) {
			append_log(std::string("Exception: ") + e.what() + "\n");
		}
	} else
#endif
	{
		try {
			if (m_sensors) {
				CFMUAutorouteProxy::Command cmd("aircraft");
				if (false && !m_sensors->get_aircraftfile().empty())
					cmd.set_option("file", m_sensors->get_aircraftfile());
				else
					cmd.set_option("data", m_sensors->get_aircraft().save_string());
				m_proxy.sendcmd(cmd);
			}
			{
				CFMUAutorouteProxy::Command cmd("clear");
				m_proxy.sendcmd(cmd);
			}
			if (m_departure.is_valid()) {
				CFMUAutorouteProxy::Command cmd("departure");
				cmd.set_option("icao", m_departure.get_icao());
				cmd.set_option("name", m_departure.get_name());
				cmd.set_option("coord", m_departure.get_coord());
				cmd.set_option(m_depifr ? "ifr" : "vfr", "");
				cmd.set_option("sid", m_sidend);
				if (m_fplautoroutespinbuttonsidlimit)
					cmd.set_option("sidlimit", m_fplautoroutespinbuttonsidlimit->get_value());
				if (m_fplautoroutespinbuttonsidpenalty)
					cmd.set_option("sidpenalty", m_fplautoroutespinbuttonsidpenalty->get_value());
				if (m_fplautoroutespinbuttonsidoffset)
					cmd.set_option("sidoffset", m_fplautoroutespinbuttonsidoffset->get_value());
				if (m_fplautoroutespinbuttonsidminimum)
					cmd.set_option("sidminimum", m_fplautoroutespinbuttonsidminimum->get_value());
				if (m_fplautoroutecheckbuttonsiddb)
					cmd.set_option("siddb", (int)m_fplautoroutecheckbuttonsiddb->get_active());
				if (m_fplautoroutecheckbuttonsidonly)
					cmd.set_option("sidonly", (int)m_fplautoroutecheckbuttonsidonly->get_active());
				{
					guint year(1970), month(0), day(1);
					if (m_fplautoroutecalendar) {
						m_fplautoroutecalendar->get_date(year, month, day);
						++month;
					}
					gint hour(10), min(0);
					if (m_fplautoroutespinbuttonhour)
						hour = m_fplautoroutespinbuttonhour->get_value_as_int();
					if (m_fplautoroutespinbuttonmin)
						min = m_fplautoroutespinbuttonmin->get_value_as_int();
					Glib::DateTime dt(Glib::DateTime::create_utc(year, month, day, hour, min, 0));
					Glib::TimeVal tv;
					if (dt.to_timeval(tv))
						cmd.set_option("date", tv.as_iso8601());
				}
				m_proxy.sendcmd(cmd);
			}
			if (m_destination.is_valid()) {
				CFMUAutorouteProxy::Command cmd("destination");
				cmd.set_option("icao", m_destination.get_icao());
				cmd.set_option("name", m_destination.get_name());
				cmd.set_option("coord", m_destination.get_coord());
				cmd.set_option(m_destifr ? "ifr" : "vfr", "");
				cmd.set_option("star", m_starstart);
				if (m_fplautoroutespinbuttonstarlimit)
					cmd.set_option("starlimit", m_fplautoroutespinbuttonstarlimit->get_value());
				if (m_fplautoroutespinbuttonstarpenalty)
					cmd.set_option("starpenalty", m_fplautoroutespinbuttonstarpenalty->get_value());
				if (m_fplautoroutespinbuttonstaroffset)
					cmd.set_option("staroffset", m_fplautoroutespinbuttonstaroffset->get_value());
				if (m_fplautoroutespinbuttonstarminimum)
					cmd.set_option("starminimum", m_fplautoroutespinbuttonstarminimum->get_value());
				if (m_fplautoroutecheckbuttonstardb)
					cmd.set_option("stardb", (int)m_fplautoroutecheckbuttonstardb->get_active());
				if (m_fplautoroutecheckbuttonstaronly)
					cmd.set_option("staronly", (int)m_fplautoroutecheckbuttonstaronly->get_active());
				m_proxy.sendcmd(cmd);
			}
			if (m_crossingstore) {
				unsigned int xvalid(0);
				for (Gtk::TreeIter iter(m_crossingstore->children().begin()); iter; ++iter) {
					Gtk::TreeModel::Row row(*iter);
					if (!static_cast<Point>(row[m_crossingcolumns.m_coord]).is_invalid())
						++xvalid;
				}
				unsigned int index(0);
				for (Gtk::TreeIter iter(m_crossingstore->children().begin()); iter; ++iter) {
					Gtk::TreeModel::Row row(*iter);
					if (static_cast<Point>(row[m_crossingcolumns.m_coord]).is_invalid())
						continue;
					CFMUAutorouteProxy::Command cmd("crossing");
					cmd.set_option("coord", static_cast<Point>(row[m_crossingcolumns.m_coord]));
					cmd.set_option("radius", static_cast<double>(row[m_crossingcolumns.m_radius]));
					cmd.set_option("minlevel", static_cast<unsigned int>(row[m_crossingcolumns.m_minlevel]));
					cmd.set_option("maxlevel", static_cast<unsigned int>(row[m_crossingcolumns.m_maxlevel]));
					cmd.set_option("index", index);
					cmd.set_option("count", xvalid);
					m_proxy.sendcmd(cmd);
					++index;
				}
			}
			{
				CFMUAutorouteProxy::Command cmd("enroute");
				if (m_fplautoroutespinbuttondctlimit)
					cmd.set_option("dctlimit", m_fplautoroutespinbuttondctlimit->get_value());
				if (m_fplautoroutespinbuttondctpenalty)
					cmd.set_option("dctpenalty", m_fplautoroutespinbuttondctpenalty->get_value());
				if (m_fplautoroutespinbuttondctoffset)
					cmd.set_option("dctoffset", m_fplautoroutespinbuttondctoffset->get_value());
				if (m_fplautoroutespinbuttonstdroutepenalty)
					cmd.set_option("stdroutepenalty", m_fplautoroutespinbuttonstdroutepenalty->get_value());
				if (m_fplautoroutespinbuttonstdrouteoffset)
					cmd.set_option("stdrouteoffset", m_fplautoroutespinbuttonstdrouteoffset->get_value());
				if (m_fplautoroutespinbuttonstdroutedctpenalty)
					cmd.set_option("stdroutedctpenalty", m_fplautoroutespinbuttonstdroutedctpenalty->get_value());
				if (m_fplautoroutespinbuttonstdroutedctoffset)
					cmd.set_option("stdroutedctoffset", m_fplautoroutespinbuttonstdroutedctoffset->get_value());
				if (m_fplautoroutespinbuttonukcasdctpenalty)
					cmd.set_option("ukcasdctpenalty", m_fplautoroutespinbuttonukcasdctpenalty->get_value());
				if (m_fplautoroutespinbuttonukcasdctoffset)
					cmd.set_option("ukcasdctoffset", m_fplautoroutespinbuttonukcasdctoffset->get_value());
				if (m_fplautoroutespinbuttonukocasdctpenalty)
					cmd.set_option("ukocasdctpenalty", m_fplautoroutespinbuttonukocasdctpenalty->get_value());
				if (m_fplautoroutespinbuttonukocasdctoffset)
					cmd.set_option("ukocasdctoffset", m_fplautoroutespinbuttonukocasdctoffset->get_value());
				if (m_fplautoroutespinbuttonukenterdctpenalty)
					cmd.set_option("ukenterdctpenalty", m_fplautoroutespinbuttonukenterdctpenalty->get_value());
				if (m_fplautoroutespinbuttonukenterdctoffset)
					cmd.set_option("ukenterdctoffset", m_fplautoroutespinbuttonukenterdctoffset->get_value());
				if (m_fplautoroutespinbuttonukleavedctpenalty)
					cmd.set_option("ukleavedctpenalty", m_fplautoroutespinbuttonukleavedctpenalty->get_value());
				if (m_fplautoroutespinbuttonukleavedctoffset)
					cmd.set_option("ukleavedctoffset", m_fplautoroutespinbuttonukleavedctoffset->get_value());
				if (m_fplautoroutespinbuttonvfraspclimit)
					cmd.set_option("vfraspclimit", m_fplautoroutespinbuttonvfraspclimit->get_value());
				if (m_fplautoroutecheckbuttonforceenrouteifr)
					cmd.set_option("forceenrouteifr", (int)m_fplautoroutecheckbuttonforceenrouteifr->get_active());
				if (m_fplautoroutecheckbuttonawylevels)
					cmd.set_option("honourawylevels", (int)m_fplautoroutecheckbuttonawylevels->get_active());
				m_proxy.sendcmd(cmd);
			}
			{
				CFMUAutorouteProxy::Command cmd("preferred");
				if (m_fplautoroutespinbuttonpreferredminlevel)
					cmd.set_option("minlevel", m_fplautoroutespinbuttonpreferredminlevel->get_value());
				if (m_fplautoroutespinbuttonpreferredmaxlevel)
					cmd.set_option("maxlevel", m_fplautoroutespinbuttonpreferredmaxlevel->get_value());
				if (m_fplautoroutespinbuttonpreferredpenalty)
					cmd.set_option("penalty", m_fplautoroutespinbuttonpreferredpenalty->get_value());
				if (m_fplautoroutespinbuttonpreferredclimb)
					cmd.set_option("climb", m_fplautoroutespinbuttonpreferredclimb->get_value());
				if (m_fplautoroutespinbuttonpreferreddescent)
					cmd.set_option("descent", m_fplautoroutespinbuttonpreferreddescent->get_value());
				m_proxy.sendcmd(cmd);
			}
			{
				CFMUAutorouteProxy::Command cmd("tfr");
				if (m_fplautoroutecheckbuttontfr)
					cmd.set_option("enabled", (int)m_fplautoroutecheckbuttontfr->get_active());
				if (m_fplautoroutecheckbuttonprecompgraph)
					cmd.set_option("precompgraph", (int)m_fplautoroutecheckbuttonprecompgraph->get_active());
				if (m_fplautoroutecomboboxvalidator) {
					switch (m_fplautoroutecomboboxvalidator->get_active_row_number()) {
					case 0:
						cmd.set_option("validator", "default");
						break;

					case 1:
						cmd.set_option("validator", "cfmu");
						break;

					case 2:
						cmd.set_option("validator", "eurofpl");
						break;

					default:
						break;
					}
				}
				if (m_tracerulesstore) {
					std::string txt;
					for (Gtk::TreeIter iter(m_tracerulesstore->children().begin()); iter; ++iter) {
						Gtk::TreeModel::Row row(*iter);
						std::string txt1((Glib::ustring)row[m_rulescolumns.m_rule]);
						while (!txt1.empty() && std::isspace(txt1[txt1.size()-1]))
							txt1.erase(txt1.size() - 1, 1);
						while (!txt1.empty() && std::isspace(txt1[0]))
							txt1.erase(0, 1);
						if (txt1.empty() || txt1 == "?")
							continue;
						if (!txt.empty())
							txt += ",";
						txt += txt1;
					}
					cmd.set_option("trace", txt, true);
				}
				if (m_disablerulesstore) {
					std::string txt;
					for (Gtk::TreeIter iter(m_disablerulesstore->children().begin()); iter; ++iter) {
						Gtk::TreeModel::Row row(*iter);
						std::string txt1((Glib::ustring)row[m_rulescolumns.m_rule]);
						while (!txt1.empty() && std::isspace(txt1[txt1.size()-1]))
							txt1.erase(txt1.size() - 1, 1);
						while (!txt1.empty() && std::isspace(txt1[0]))
							txt1.erase(0, 1);
						if (txt1.empty() || txt1 == "?")
							continue;
						if (!txt.empty())
							txt += ",";
						txt += txt1;
					}
					cmd.set_option("disable", txt, true);
				}
				m_proxy.sendcmd(cmd);
			}
			{
				CFMUAutorouteProxy::Command cmd("atmosphere");
				if (m_fplautoroutecheckbuttonwind)
					cmd.set_option("wind", (int)m_fplautoroutecheckbuttonwind->get_active());
				m_proxy.sendcmd(cmd);
			}
			if (m_airspacestore) {
				bool first(true);
				for (Gtk::TreeIter iter(m_airspacestore->children().begin()); iter; ++iter) {
					Gtk::TreeModel::Row row(*iter);
					CFMUAutorouteProxy::Command cmd("exclude");
					if (first) {
						cmd.set_option("clear", 1);
						first = false;
					}
					if (static_cast<Point>(row[m_airspacecolumns.m_coord]).is_invalid()) {
						cmd.set_option("aspcid", static_cast<Glib::ustring>(row[m_airspacecolumns.m_ident]));
						cmd.set_option("aspctype", static_cast<Glib::ustring>(row[m_airspacecolumns.m_type]));
					} else {
						Rect bbox(static_cast<Point>(row[m_airspacecolumns.m_coord]),
							  static_cast<Point>(row[m_airspacecolumns.m_coord]));
						double miles(Glib::Ascii::strtod(static_cast<Glib::ustring>(row[m_airspacecolumns.m_type])));
						if (std::isnan(miles) || miles < 0)
							miles = 0;
						bbox = bbox.oversize_nmi(miles);
						cmd.set_option("sw", bbox.get_southwest());
						cmd.set_option("ne", bbox.get_northeast());
					}
					cmd.set_option("base", static_cast<unsigned int>(row[m_airspacecolumns.m_minlevel]));
					cmd.set_option("top", static_cast<unsigned int>(row[m_airspacecolumns.m_maxlevel]));
					cmd.set_option("awylimit", static_cast<double>(row[m_airspacecolumns.m_awylimit]));
					cmd.set_option("dctlimit", static_cast<double>(row[m_airspacecolumns.m_dctlimit]));
					cmd.set_option("dctoffset", static_cast<double>(row[m_airspacecolumns.m_dctoffset]));
					cmd.set_option("dctscale", static_cast<double>(row[m_airspacecolumns.m_dctscale]));
					m_proxy.sendcmd(cmd);
				}
				if (first) {
					CFMUAutorouteProxy::Command cmd("exclude");
					cmd.set_option("clear", 1);
					m_proxy.sendcmd(cmd);
				}
			}
			{
				CFMUAutorouteProxy::Command cmd("levels");
				if (m_fplautoroutespinbuttonbaselevel)
					cmd.set_option("base", m_fplautoroutespinbuttonbaselevel->get_value());
				if (m_fplautoroutespinbuttontoplevel)
					cmd.set_option("top", m_fplautoroutespinbuttontoplevel->get_value());
				if (m_fplautoroutespinbuttonminlevel)
					cmd.set_option("minlevel", m_fplautoroutespinbuttonminlevel->get_value());
				if (m_fplautoroutespinbuttonminlevelpenalty)
					cmd.set_option("minlevelpenalty", m_fplautoroutespinbuttonminlevelpenalty->get_value());
				if (m_fplautoroutespinbuttonminlevelmaxpenalty)
					cmd.set_option("minlevelmaxpenalty", m_fplautoroutespinbuttonminlevelmaxpenalty->get_value());
				m_proxy.sendcmd(cmd);
			}
			{
				CFMUAutorouteProxy::Command cmd("atmosphere");
				if (m_sensors)
					cmd.set_option("qnh", m_sensors->get_altimeter_qnh());
				if (m_sensors)
					cmd.set_option("isa", m_sensors->get_altimeter_tempoffs());
				m_proxy.sendcmd(cmd);
			}
			{
				CFMUAutorouteProxy::Command cmd("cruise");
				if (m_sensors) {
					Aircraft::Cruise::EngineParams ep(m_sensors->get_acft_cruise_params());
					cmd.set_option("rpm", ep.get_rpm());
					cmd.set_option("mp", ep.get_mp());
					cmd.set_option("bhp", ep.get_bhp());
				}
				m_proxy.sendcmd(cmd);
			}
			{
				CFMUAutorouteProxy::Command cmd("optimization");
				if (m_fplautoroutecomboboxopttarget) {
					int x(m_fplautoroutecomboboxopttarget->get_active_row_number());
					cmd.set_option("target", (x >= 2) ? "preferred" : x ? "fuel" : "time");
				}
				m_proxy.sendcmd(cmd);
			}
			{
				CFMUAutorouteProxy::Command cmd("preload");
				m_proxy.sendcmd(cmd);
			}
		} catch (const std::exception& e) {
			append_log(std::string("Exception: ") + e.what() + "\n");
		}
	}
	{
		Point pt(get_bbox().boxcenter());
		int alt(50);
		if (m_fplautoroutespinbuttonbaselevel)
				alt = m_fplautoroutespinbuttonbaselevel->get_value();
		alt *= 100;
		if (m_fplautoroutedrawingarea) {
			Glib::TimeVal tv;
			tv.assign_current_time();		
			m_fplautoroutedrawingarea->set_center(pt, alt, 0, tv.tv_sec);
			m_fplautoroutedrawingarea->unset_cursor();
		}
	}
}

void FlightDeckWindow::FPLAutoRouteDialog::pg2_back_clicked(void)
{
	if (!m_fplautoroutenotebook) {
		close();
		return;
	}
	m_fplautoroutenotebook->set_current_page(0);
	if (m_fplautoroutetogglebuttonpg2run)
		m_fplautoroutetogglebuttonpg2run->set_active(false);
	try {
#ifdef HAVE_JSONCPP
		if (m_childsockaddr) {
			Json::Value root;
			root["session"] = m_childsession;
			Json::Value cmds;
			{
				Json::Value cmd;
				cmd["cmdname"] = "clear";
				cmds.append(cmd);
			}
			root["cmds"] = cmds;
			sendjson(root);
		} else
#endif
		{
			CFMUAutorouteProxy::Command cmd("clear");
			m_proxy.sendcmd(cmd);
		}
	} catch (const std::exception& e) {
		append_log(std::string("Exception: ") + e.what() + "\n");
	}
}

void FlightDeckWindow::FPLAutoRouteDialog::pg2_run_clicked(void)
{
	if (!m_fplautoroutetogglebuttonpg2run)
		return;
	if (m_routesstore && m_fplautoroutetogglebuttonpg2run->get_active()) {
		m_routesstore->clear();
		if (m_fplautorouteframepg2routes)
			m_fplautorouteframepg2routes->set_visible(false);
	}
	try {
#ifdef HAVE_JSONCPP
		if (m_childsockaddr) {
			Json::Value root;
			root["session"] = m_childsession;
			Json::Value cmds;
			{
				Json::Value cmd;
				cmd["cmdname"] = m_fplautoroutetogglebuttonpg2run->get_active() ? "start" : "stop";
				cmds.append(cmd);
			}
			root["cmds"] = cmds;
			sendjson(root);
		} else
#endif
		{
			CFMUAutorouteProxy::Command cmd(m_fplautoroutetogglebuttonpg2run->get_active() ? "start" : "stop");
			m_proxy.sendcmd(cmd);
		}
	} catch (const std::exception& e) {
		append_log(std::string("Exception: ") + e.what() + "\n");
	}
}

void FlightDeckWindow::FPLAutoRouteDialog::ifr_toggled(void)
{

	if (m_fplautoroutespinbuttonsidlimit)
		m_sidstarlimit[0][m_depifr] = m_fplautoroutespinbuttonsidlimit->get_value();
	if (m_fplautoroutespinbuttonstarlimit)
		m_sidstarlimit[1][m_destifr] = m_fplautoroutespinbuttonstarlimit->get_value();
	if (m_fplautoroutecheckbuttondepifr)
		m_depifr = m_fplautoroutecheckbuttondepifr->get_active();
	if (m_fplautoroutecheckbuttondestifr)
		m_destifr = m_fplautoroutecheckbuttondestifr->get_active();
	if (m_fplautoroutespinbuttonsidlimit)
		m_fplautoroutespinbuttonsidlimit->set_value(m_sidstarlimit[0][m_depifr]);
	if (m_fplautoroutespinbuttonstarlimit)
		m_fplautoroutespinbuttonstarlimit->set_value(m_sidstarlimit[1][m_destifr]);
	if (m_fplautoroutelabelsid)
		m_fplautoroutelabelsid->set_text(m_depifr ? "SID End" : "Joining");
	if (m_fplautoroutelabelstar)
		m_fplautoroutelabelstar->set_text(m_destifr ? "STAR Begin" : "Leaving");
	initial_fplan();
}

void FlightDeckWindow::FPLAutoRouteDialog::opttarget_changed(void)
{
	if (!m_fplautoroutecomboboxopttarget)
		return;
	bool pref(m_fplautoroutecomboboxopttarget->get_active_row_number() >= 2);
	if (m_fplautoroutespinbuttonpreferredminlevel)
		m_fplautoroutespinbuttonpreferredminlevel->set_sensitive(pref);
	if (m_fplautoroutespinbuttonpreferredmaxlevel)
		m_fplautoroutespinbuttonpreferredmaxlevel->set_sensitive(pref);
	if (m_fplautoroutespinbuttonpreferredpenalty)
		m_fplautoroutespinbuttonpreferredpenalty->set_sensitive(pref);
	if (m_fplautoroutespinbuttonpreferredclimb)
		m_fplautoroutespinbuttonpreferredclimb->set_sensitive(pref);
	if (m_fplautoroutespinbuttonpreferreddescent)
		m_fplautoroutespinbuttonpreferreddescent->set_sensitive(pref);
}

void FlightDeckWindow::FPLAutoRouteDialog::pt_changed(unsigned int i)
{
	if (i >= sizeof(m_ptchanged)/sizeof(m_ptchanged[0]))
		return;
	m_ptchanged[i] = true;
}

bool FlightDeckWindow::FPLAutoRouteDialog::pt_focusout(GdkEventFocus *event)
{
	for (unsigned int i = 0; i < sizeof(m_ptchanged)/sizeof(m_ptchanged[0]); ++i) {
		if (!m_ptchanged[i])
			continue;
		m_ptchanged[i] = false;
		if (i < 2) {
			// find airport
			Glib::ustring arptname;
			if (i) {
				if (m_fplautorouteentrydesticao)
					arptname = m_fplautorouteentrydesticao->get_text();
			} else {
				if (m_fplautorouteentrydepicao)
					arptname = m_fplautorouteentrydepicao->get_text();
			}
			AirportsDb::Airport el;
			if (!arptname.empty()) {
				IcaoFlightPlan::FindCoord f(*m_sensors->get_engine());
				if (f.find(arptname, "", IcaoFlightPlan::FindCoord::flag_airport))
					if (!f.get_airport(el))
						el = AirportsDb::Airport();
			}
			if (i) {
				m_destination = el;
				if (m_fplautorouteentrydestname) {
					if (m_destination.is_valid())
						m_fplautorouteentrydestname->set_text(m_destination.get_name());
					else
						m_fplautorouteentrydestname->set_text("");
				}
			} else {
				m_departure = el;
				if (m_fplautorouteentrydepname) {
					if (m_departure.is_valid())
						m_fplautorouteentrydepname->set_text(m_departure.get_name());
					else
						m_fplautorouteentrydepname->set_text("");
				}
			}
			continue;
		}
		// find navaid or intersection
		Glib::ustring ptname;
		Point pt;
		if (i == 2) {
			if (m_fplautorouteentrysidicao)
				ptname = m_fplautorouteentrysidicao->get_text();
			if (m_departure.is_valid())
				pt = m_departure.get_coord();
			else if (m_destination.is_valid())
				pt = m_destination.get_coord();
		} else if (i == 3) {
			if (m_fplautorouteentrystaricao)
				ptname = m_fplautorouteentrystaricao->get_text();
			if (m_destination.is_valid())
				pt = m_destination.get_coord();
			else if (m_departure.is_valid())
				pt = m_departure.get_coord();
		}
		if (ptname.empty()) {
			pt = Point::invalid;
			ptname.clear();
		} else {
			IcaoFlightPlan::FindCoord f(*m_sensors->get_engine());
			if (f.find(ptname, ptname, IcaoFlightPlan::FindCoord::flag_navaid | IcaoFlightPlan::FindCoord::flag_waypoint, pt)) {
				pt = Point::invalid;
				ptname.clear();
				NavaidsDb::element_t nav;
				WaypointsDb::element_t wpt;
				if (f.get_navaid(nav)) {
					pt = nav.get_coord();
					ptname = nav.get_navaid_typename() + " " + nav.get_name();
				} else if (f.get_waypoint(wpt)) {
					pt = wpt.get_coord();
					ptname = "INT " + wpt.get_name();
				}
			} else {
				pt = Point::invalid;
				ptname.clear();
			}
		}
		if (i == 2) {
			m_sidend = pt;
			if (m_fplautorouteentrysidname)
				m_fplautorouteentrysidname->set_text(ptname);
		} else if (i == 3) {
			m_starstart = pt;
			if (m_fplautorouteentrystarname)
				m_fplautorouteentrystarname->set_text(ptname);
		}
	}
	{
		bool ok(m_departure.is_valid() && m_destination.is_valid());
		if (m_fplautoroutebuttonpg1copy)
			m_fplautoroutebuttonpg1copy->set_sensitive(ok);
		if (m_fplautoroutebuttonpg1next)
			m_fplautoroutebuttonpg1next->set_sensitive(ok);
	}
	initial_fplan();
	return false;
}

#ifdef HAVE_JSONCPP
bool FlightDeckWindow::FPLAutoRouteDialog::recvjson(Glib::IOCondition iocond, const childsocket_t& sock)
{
	if (!sock)
		return false;
	Json::Value root;
	try {
                char mbuf[262144];
                gssize r(sock->receive(mbuf, sizeof(mbuf)));
		m_childsockets.erase(sock);
		sock->close();
		if (r <= 0) {
			std::ostringstream oss;
			oss << "JSON read return value: " << r;
			append_log(oss.str() + "\n");
                } else if (r >= sizeof(mbuf)) {
  			std::ostringstream oss;
			oss << "JSON read return value: " << r << ": invalid message size";
			append_log(oss.str() + "\n");
		} else {
			{
				Json::Reader reader;
				if (!reader.parse(mbuf, mbuf + r, root, false)) {
					std::ostringstream oss;
					oss << "JSON read parse error: " << std::string(mbuf, mbuf + r);
					append_log(oss.str() + "\n");
				}
			}
			if (m_fplautoroutetextviewpg2comm) {
				Glib::RefPtr<Gtk::TextBuffer> buf(m_fplautoroutetextviewpg2comm->get_buffer());
				if (buf) {
					buf->insert(buf->end(), "# " + std::string(mbuf, mbuf + r));
					Gtk::TextBuffer::iterator it(buf->end());
					m_fplautoroutetextviewpg2comm->scroll_to(it);
				}
			}
			if (true)
				std::cerr << ">> " << std::string(mbuf, mbuf + r) << std::endl;
		}
        } catch (const Gio::Error& error) {
                if (error.code () == Gio::Error::WOULD_BLOCK) {
			return true;
                } else {
			append_log("JSON read error: " + error.what() + "\n");
			m_childsockets.erase(sock);
			sock->close();
                }
	}
	if (m_childsockets.empty()) {
		Json::Value root;
		root["session"] = m_childsession;
		sendjson(root);
	}
	if (root.isMember("cmds")) {
		const Json::Value& cmds(root["cmds"]);
		if (cmds.isObject()) {
                        recvjsoncmd(cmds);
                } else if (cmds.isArray()) {
                        for (Json::ArrayIndex i = 0; i < cmds.size(); ++i) {
                                const Json::Value& cmd(cmds[i]);
                                if (!cmd.isObject())
                                        continue;
				recvjsoncmd(cmd);
                        }
                }
	}
	return false;
}

void FlightDeckWindow::FPLAutoRouteDialog::recvjsoncmd(const Json::Value& cmd)
{
	if (!cmd.isMember("cmdname") || !cmd["cmdname"].isString())
		return;
	std::string cmdname(cmd["cmdname"].asString());
       	if (cmdname == "autoroute") {
		if (cmd.isMember("status") && cmd["status"].isString()) {
			std::string status(cmd["status"].asString());
			if ((status == "stopping" || status == "terminate") &&
			    m_fplautoroutetogglebuttonpg2run && m_fplautoroutetogglebuttonpg2run->get_active())
				m_fplautoroutetogglebuttonpg2run->set_active(false);
		}
		return;
	}
	if (cmdname == "log") {
		if (!cmd.isMember("item") || !cmd["item"].isString() ||
		    !cmd.isMember("text") || !cmd["text"].isString())
			return;
		std::string item(cmd["item"].asString());
		std::string text(cmd["text"].asString());
		if (item == "fpllocalvalidation" ||
		    item == "fplremotevalidation") {
			if (!m_fplautoroutetextviewpg2results)
				return;
			Glib::RefPtr<Gtk::TextBuffer> buf(m_fplautoroutetextviewpg2results->get_buffer());
			if (!buf)
				return;
			if (m_fplvalclear)
				buf->set_text("");
			buf->insert(buf->end(), text + "\n");
			Gtk::TextBuffer::iterator it(buf->end());
			m_fplautoroutetextviewpg2results->scroll_to(it);
			m_fplvalclear = text.empty();
			return;
		}
		if (item == "graphchange") {
			if (m_fplautoroutetextviewpg2graph) {
				Glib::RefPtr<Gtk::TextBuffer> buf(m_fplautoroutetextviewpg2graph->get_buffer());
				if (buf) {
					buf->insert_with_tag(buf->end(), "  " + text + "\n", m_txtgraphchange);
					Gtk::TextBuffer::iterator it(buf->end());
					m_fplautoroutetextviewpg2graph->scroll_to(it);
				}
			}
			return;
		}
		if (item == "graphrule") {
			if (m_fplautoroutetextviewpg2graph) {
				Glib::RefPtr<Gtk::TextBuffer> buf(m_fplautoroutetextviewpg2graph->get_buffer());
				if (buf) {
					buf->insert_with_tag(buf->end(), text + "\n", m_txtgraphrule);
					Gtk::TextBuffer::iterator it(buf->end());
					m_fplautoroutetextviewpg2graph->scroll_to(it);
				}
			}
			return;
		}
		if (item == "graphruledesc") {
			if (m_fplautoroutetextviewpg2graph) {
				Glib::RefPtr<Gtk::TextBuffer> buf(m_fplautoroutetextviewpg2graph->get_buffer());
				if (buf) {
					buf->insert_with_tag(buf->end(), text + "\n", m_txtgraphruledesc);
					Gtk::TextBuffer::iterator it(buf->end());
					m_fplautoroutetextviewpg2graph->scroll_to(it);
				}
			}
			return;
		}
		if (item == "graphruleoprgoal") {
			if (m_fplautoroutetextviewpg2graph) {
				Glib::RefPtr<Gtk::TextBuffer> buf(m_fplautoroutetextviewpg2graph->get_buffer());
				if (buf) {
					buf->insert_with_tag(buf->end(), text + "\n", m_txtgraphruleoprgoal);
					Gtk::TextBuffer::iterator it(buf->end());
					m_fplautoroutetextviewpg2graph->scroll_to(it);
				}
			}
			return;
		}
		if (item.substr(0, 5) == "debug") {
			if (m_fplautoroutetextviewpg2debug) {
				Glib::RefPtr<Gtk::TextBuffer> buf(m_fplautoroutetextviewpg2debug->get_buffer());
				if (buf) {
					buf->insert(buf->end(), text + "\n");
					Gtk::TextBuffer::iterator it(buf->end());
					m_fplautoroutetextviewpg2debug->scroll_to(it);
				}
			}
			return;
		}
		append_log(text + "\n");
		return;
	}
	if (cmdname == "fpl") {
		std::string fpl;
		if (cmd.isMember("fpl") && cmd["fpl"].isString()) {
			fpl = cmd["fpl"].asString();
			if (m_fplautoroutetextviewpg2route) {
				Glib::RefPtr<Gtk::TextBuffer> buf(m_fplautoroutetextviewpg2route->get_buffer());
				if (buf)
					buf->set_text(fpl);
			}
		}
		if (m_fplautoroutetextviewpg2graph) {
			Glib::RefPtr<Gtk::TextBuffer> buf(m_fplautoroutetextviewpg2graph->get_buffer());
			if (buf) {
				buf->insert_with_tag(buf->end(), fpl + "\n", m_txtgraphfpl);
				Gtk::TextBuffer::iterator it(buf->end());
				m_fplautoroutetextviewpg2graph->scroll_to(it);
			}
		}
		if (cmd.isMember("fplan") && cmd["fplan"].isArray()) {
			const Json::Value& fpl(cmd["fplan"]);
			FPlanRoute route(*(FPlan *)0);
			if (cmd.isMember("note") && cmd["note"].isString())
				route.set_note(cmd["note"].asString());
			if (cmd.isMember("curwpt") && cmd["curwpt"].isNumeric())
				route.set_curwpt(cmd["curwpt"].asUInt());
			if (cmd.isMember("offblock") && cmd["offblock"].isNumeric())
				route.set_time_offblock_unix(cmd["offblock"].asInt());
			if (cmd.isMember("save_offblock") && cmd["save_offblock"].isNumeric())
				route.set_save_time_offblock_unix(cmd["save_offblock"].asInt());
			if (cmd.isMember("onblock") && cmd["onblock"].isNumeric())
				route.set_time_onblock_unix(cmd["onblock"].asInt());
			if (cmd.isMember("save_onblock") && cmd["save_onblock"].isNumeric())
				route.set_save_time_onblock_unix(cmd["save_onblock"].asInt());
			for (Json::ArrayIndex i = 0; i < fpl.size(); ++i) {
                                const Json::Value& wpt(fpl[i]);
                                if (!wpt.isObject())
                                        continue;
				FPlanWaypoint wptr;
				if (wpt.isMember("icao") && wpt["icao"].isString())
					wptr.set_icao(wpt["icao"].asString());
				if (wpt.isMember("name") && wpt["name"].isString())
					wptr.set_name(wpt["name"].asString());
				if (wpt.isMember("pathname") && wpt["pathname"].isString())
					wptr.set_pathname(wpt["pathname"].asString());
				if (wpt.isMember("pathcode") && wpt["pathcode"].isString()) {
					std::string pcs(wpt["pathcode"].asString());
					for (FPlanWaypoint::pathcode_t pc(FPlanWaypoint::pathcode_none); pc <= FPlanWaypoint::pathcode_directto;
					     pc = static_cast<FPlanWaypoint::pathcode_t>(pc + 1)) {
						if (FPlanWaypoint::get_pathcode_name(pc) != pcs)
							continue;
						wptr.set_pathcode(pc);
						break;
					}
				}
				if (wpt.isMember("note") && wpt["note"].isString())
					wptr.set_note(wpt["note"].asString());
				if (wpt.isMember("time") && wpt["time"].isNumeric())
					wptr.set_time_unix(wpt["time"].asUInt());
				if (wpt.isMember("save_time") && wpt["save_time"].isNumeric())
					wptr.set_save_time_unix(wpt["save_time"].asInt());
				if (wpt.isMember("frequency") && wpt["frequency"].isNumeric())
					wptr.set_frequency(wpt["frequency"].asInt());
				if (wpt.isMember("coordlat") && wpt["coordlat"].isNumeric())
					wptr.set_lat(wpt["coordlat"].asInt());
				if (wpt.isMember("coordlon") && wpt["coordlon"].isNumeric())
					wptr.set_lon(wpt["coordlon"].asInt());
				if (wpt.isMember("altitude") && wpt["altitude"].isNumeric())
					wptr.set_altitude(wpt["altitude"].asInt());
				if (wpt.isMember("flags") && wpt["flags"].isNumeric())
					wptr.set_flags(wpt["flags"].asUInt());
				if (wpt.isMember("isaoffset") && wpt["isaoffset"].isNumeric())
					wptr.set_isaoffset_kelvin(wpt["isaoffset"].asDouble());
				if (wpt.isMember("qff") && wpt["qff"].isNumeric())
					wptr.set_qff_hpa(wpt["qff"].asDouble());
				if (wpt.isMember("winddir") && wpt["winddir"].isNumeric())
					wptr.set_winddir_deg(wpt["winddir"].asDouble());
				if (wpt.isMember("windspeed") && wpt["windspeed"].isNumeric())
					wptr.set_windspeed_kts(wpt["windspeed"].asDouble());
				route.insert_wpt(~0U, wptr);
                        }
			m_proxy.set_fplan(route);
		}
		if (m_fplautorouteentrypg2gcdist && cmd.isMember("gcdist") && cmd["gcdist"].isNumeric())
			m_fplautorouteentrypg2gcdist->set_text(Conversions::dist_str(cmd["gcdist"].asDouble()));
		if (m_fplautorouteentrypg2routedist && cmd.isMember("routedist") && cmd["routedist"].isNumeric())
			m_fplautorouteentrypg2routedist->set_text(Conversions::dist_str(cmd["routedist"].asDouble()));
		if (m_fplautorouteentrypg2mintime) {
			std::ostringstream oss;
			bool v(cmd.isMember("mintime") && cmd["mintime"].isNumeric());
			bool vzw(cmd.isMember("mintimezerowind") && cmd["mintimezerowind"].isNumeric());
			if (v)
				oss << Conversions::time_str(cmd["mintime"].asInt());
			if (v && vzw)
				oss << ' ';
			if (vzw)
				oss << '[' << Conversions::time_str(cmd["mintimezerowind"].asInt()) << ']';
			m_fplautorouteentrypg2mintime->set_text(oss.str());
		}
		if (m_fplautorouteentrypg2routetime) {
			std::ostringstream oss;
			bool v(cmd.isMember("routetime") && cmd["routetime"].isNumeric());
			bool vzw(cmd.isMember("routetimezerowind") && cmd["routetimezerowind"].isNumeric());
			if (v)
				oss << Conversions::time_str(cmd["routetime"].asInt());
			if (v && vzw)
				oss << ' ';
			if (vzw)
				oss << '[' << Conversions::time_str(cmd["routetimezerowind"].asInt()) << ']';
			m_fplautorouteentrypg2routetime->set_text(oss.str());
		}
		if (m_fplautorouteentrypg2minfuel) {
			std::ostringstream oss;
			bool v(cmd.isMember("minfuel") && cmd["minfuel"].isNumeric());
			bool vzw(cmd.isMember("minfuelzerowind") && cmd["minfuelzerowind"].isNumeric());
			if (v)
				oss << Conversions::time_str(cmd["minfuel"].asDouble());
			if (v && vzw)
				oss << ' ';
			if (vzw)
				oss << '[' << Conversions::time_str(cmd["minfuelzerowind"].asDouble()) << ']';
			m_fplautorouteentrypg2minfuel->set_text(oss.str());
		}
		if (m_fplautorouteentrypg2routefuel) {
			std::ostringstream oss;
			bool v(cmd.isMember("routefuel") && cmd["routefuel"].isNumeric());
			bool vzw(cmd.isMember("routefuelzerowind") && cmd["routefuelzerowind"].isNumeric());
			if (v)
				oss << Conversions::time_str(cmd["routefuel"].asDouble());
			if (v && vzw)
				oss << ' ';
			if (vzw)
				oss << '[' << Conversions::time_str(cmd["routefuelzerowind"].asDouble()) << ']';
			m_fplautorouteentrypg2routefuel->set_text(oss.str());
		}
		if (m_fplautorouteentrypg2iteration) {
			std::ostringstream oss;
			if (cmd.isMember("iteration") && cmd["iteration"].isNumeric())
				oss << cmd["iteration"].asInt();
			if (cmd.isMember("localiteration") && cmd["localiteration"].isNumeric() &&
			    cmd.isMember("remoteiteration") && cmd["remoteiteration"].isNumeric()) {
				if (!oss.str().empty())
					oss << ' ';
				oss << '(' << cmd["localiteration"].asInt() << '/' << cmd["remoteiteration"].asInt() << ')';
			}
			m_fplautorouteentrypg2iteration->set_text(oss.str());
		}
		if (m_fplautorouteentrypg2times) {
			std::ostringstream oss;
			if (cmd.isMember("wallclocktime") && cmd["wallclocktime"].isNumeric())
				oss << std::fixed << std::setprecision(1) << cmd["wallclocktime"].asDouble();
			oss << '/';
			if (cmd.isMember("validatortime") && cmd["validatortime"].isNumeric())
				oss << std::fixed << std::setprecision(1) << cmd["validatortime"].asDouble();
			m_fplautorouteentrypg2times->set_text(oss.str());
		}
		if (m_fplautoroutedrawingarea)
			m_fplautoroutedrawingarea->redraw_route();
		if (m_routesstore && cmd.isMember("status") && cmd["status"].isString() && cmd["status"].asString() == "solution") {
			Gtk::TreeIter iter(m_routesstore->append());
			Gtk::TreeModel::Row row(*iter);
			{
				std::ostringstream oss;
				bool v(cmd.isMember("routetime") && cmd["routetime"].isNumeric());
				bool vzw(cmd.isMember("routetimezerowind") && cmd["routetimezerowind"].isNumeric());
				if (v)
					oss << Conversions::time_str(cmd["routetime"].asInt());
				if (v && vzw)
					oss << ' ';
				if (vzw)
					oss << '[' << Conversions::time_str(cmd["routetimezerowind"].asInt()) << ']';
			        row[m_routescolumns.m_time] = oss.str();
			}
			{
				std::ostringstream oss;
				bool v(cmd.isMember("routefuel") && cmd["routefuel"].isNumeric());
				bool vzw(cmd.isMember("routefuelzerowind") && cmd["routefuelzerowind"].isNumeric());
				if (v)
					oss << Conversions::time_str(cmd["routefuel"].asDouble());
				if (v && vzw)
					oss << ' ';
				if (vzw)
					oss << '[' << Conversions::time_str(cmd["routefuelzerowind"].asDouble()) << ']';
				row[m_routescolumns.m_fuel] = oss.str();
			}
			if (cmd.isMember("item15atc") && cmd["item15atc"].isString())
				row[m_routescolumns.m_routetxt] = cmd["item15atc"].asString();
			else if (cmd.isMember("fpl") && cmd["fpl"].isString())
				row[m_routescolumns.m_routetxt] = cmd["fpl"].asString();
			{
				std::ostringstream oss;
				if (cmd.isMember("vfrdep") && cmd["vfrdep"].isBool() && cmd["vfrdep"].asBool())
					oss << " VFRDEP";
				if (cmd.isMember("vfrarr") && cmd["vfrarr"].isBool() && cmd["vfrarr"].asBool())
					oss << " VFRARR";
				if (cmd.isMember("nonstddep") && cmd["nonstddep"].isBool() && cmd["nonstddep"].asBool())
					oss << " NONSTDDEP";
				if (cmd.isMember("nonstdarr") && cmd["nonstdarr"].isBool() && cmd["nonstdarr"].asBool())
					oss << " NONSTDARR";
				if (cmd.isMember("procint") && cmd["procint"].isBool() && cmd["procint"].asBool())
					oss << " PROCINT";
				if (cmd.isMember("fullstdrte") && cmd["fullstdrte"].isBool() && cmd["fullstdrte"].asBool())
					oss << " STDRTE";
				if (oss.str().empty())
					row[m_routescolumns.m_flags] = "";
				else
					row[m_routescolumns.m_flags] = oss.str().substr(1);
			}
			row[m_routescolumns.m_route] = (RoutesModelColumns::FPLRoute)m_proxy.get_fplan();
			if (cmd.isMember("routemetric") && cmd["routemetric"].isNumeric())
				row[m_routescolumns.m_routemetric] = cmd["routemetric"].asDouble();
			else
				row[m_routescolumns.m_routemetric] = std::numeric_limits<double>::quiet_NaN();
			if (cmd.isMember("routefuel") && cmd["routefuel"].isNumeric())
				row[m_routescolumns.m_routefuel] = cmd["routefuel"].asDouble();
			else
				row[m_routescolumns.m_routefuel] = std::numeric_limits<double>::quiet_NaN();
			if (cmd.isMember("routefuelzerowind") && cmd["routefuelzerowind"].isNumeric())
				row[m_routescolumns.m_routezerowindfuel] = cmd["routefuelzerowind"].asDouble();
			else
				row[m_routescolumns.m_routezerowindfuel] = std::numeric_limits<double>::quiet_NaN();
			if (cmd.isMember("routetime") && cmd["routetime"].isNumeric())
				row[m_routescolumns.m_routetime] = cmd["routetime"].asDouble();
			else
				row[m_routescolumns.m_routetime] = std::numeric_limits<double>::quiet_NaN();
			if (cmd.isMember("routetimezerowind") && cmd["routetimezerowind"].isNumeric())
				row[m_routescolumns.m_routezerowindtime] = cmd["routetimezerowind"].asDouble();
			else
				row[m_routescolumns.m_routezerowindtime] = std::numeric_limits<double>::quiet_NaN();
			if (cmd.isMember("variant") && cmd["variant"].isNumeric())
				row[m_routescolumns.m_variant] = cmd["variant"].asInt();
			else
				row[m_routescolumns.m_variant] = 0;
			if (m_fplautorouteframepg2routes)
				m_fplautorouteframepg2routes->set_visible(true);
		}
		return;
	}
	if (cmdname == "departure") {
		bool have_icao(cmd.isMember("icao") && cmd["icao"].isString());
		bool have_name(cmd.isMember("name") && cmd["name"].isString());
       		std::string icao(cmd["icao"].asString());
		std::string name(cmd["name"].asString());
		std::string icao_name;
		if (have_icao && !icao.empty())
			icao_name = icao;
		if (have_icao && !icao.empty() && have_name && !name.empty())
			icao_name += " ";
		if (have_name && !name.empty())
			icao_name += name;
		if (m_fplautorouteentrypg2dep)
			m_fplautorouteentrypg2dep->set_text(icao_name);
		if (!cmd.isMember("cmdseq") || !cmd["cmdseq"].isString() || cmd["cmdseq"].asString() != "init")
			return;
		if (cmd.isMember("sidlimit") && cmd["sidlimit"].isNumeric()) {
			m_sidstarlimit[0][1] = cmd["sidlimit"].asDouble();
			if (m_depifr && m_fplautoroutespinbuttonsidlimit)
				m_fplautoroutespinbuttonsidlimit->set_value(m_sidstarlimit[0][m_depifr]);
		}
		if (cmd.isMember("sidpenalty") && cmd["sidpenalty"].isNumeric() && m_fplautoroutespinbuttonsidpenalty)
			m_fplautoroutespinbuttonsidpenalty->set_value(cmd["sidpenalty"].asDouble());
		if (cmd.isMember("sidoffset") && cmd["sidoffset"].isNumeric() && m_fplautoroutespinbuttonsidoffset)
			m_fplautoroutespinbuttonsidoffset->set_value(cmd["sidoffset"].asDouble());
		if (cmd.isMember("sidminimum") && cmd["sidminimum"].isNumeric() && m_fplautoroutespinbuttonsidminimum)
			m_fplautoroutespinbuttonsidminimum->set_value(cmd["sidminimum"].asDouble());
		if (cmd.isMember("siddb") && cmd["siddb"].isBool()) {
			if (m_fplautoroutecheckbuttonsiddb &&
			    !(m_sensors && m_sensors->get_configfile().has_key(cfggroup_mainwindow, "autoroutesiddb")))
				m_fplautoroutecheckbuttonsiddb->set_active(cmd["siddb"].asBool());
		}
		if (cmd.isMember("sidonly") && cmd["sidonly"].isBool()) {
			if (m_fplautoroutecheckbuttonsidonly &&
			    !(m_sensors && m_sensors->get_configfile().has_key(cfggroup_mainwindow, "autoroutesidonly")))
				m_fplautoroutecheckbuttonsidonly->set_active(cmd["sidonly"].asBool());
		}
		return;
	}
	if (cmdname == "destination") {
		bool have_icao(cmd.isMember("icao") && cmd["icao"].isString());
		bool have_name(cmd.isMember("name") && cmd["name"].isString());
       		std::string icao(cmd["icao"].asString());
		std::string name(cmd["name"].asString());
		std::string icao_name;
		if (have_icao && !icao.empty())
			icao_name = icao;
		if (have_icao && !icao.empty() && have_name && !name.empty())
			icao_name += " ";
		if (have_name && !name.empty())
			icao_name += name;
		if (m_fplautorouteentrypg2dest)
			m_fplautorouteentrypg2dest->set_text(icao_name);
		if (!cmd.isMember("cmdseq") || !cmd["cmdseq"].isString() || cmd["cmdseq"].asString() != "init")
			return;
		if (cmd.isMember("starlimit") && cmd["starlimit"].isNumeric()) {
			m_sidstarlimit[1][1] = cmd["starlimit"].asDouble();
			if (m_destifr && m_fplautoroutespinbuttonstarlimit)
				m_fplautoroutespinbuttonstarlimit->set_value(m_sidstarlimit[1][m_destifr]);
		}
		if (cmd.isMember("starpenalty") && cmd["starpenalty"].isNumeric() && m_fplautoroutespinbuttonstarpenalty)
			m_fplautoroutespinbuttonstarpenalty->set_value(cmd["starpenalty"].asDouble());
		if (cmd.isMember("staroffset") && cmd["staroffset"].isNumeric() && m_fplautoroutespinbuttonstaroffset)
			m_fplautoroutespinbuttonstaroffset->set_value(cmd["staroffset"].asDouble());
		if (cmd.isMember("starminimum") && cmd["starminimum"].isNumeric() && m_fplautoroutespinbuttonstarminimum)
			m_fplautoroutespinbuttonstarminimum->set_value(cmd["starminimum"].asDouble());
		if (cmd.isMember("stardb") && cmd["stardb"].isBool()) {
			if (m_fplautoroutecheckbuttonstardb &&
			    !(m_sensors && m_sensors->get_configfile().has_key(cfggroup_mainwindow, "autoroutestardb")))
				m_fplautoroutecheckbuttonstardb->set_active(cmd["stardb"].asBool());
		}
		if (cmd.isMember("staronly") && cmd["staronly"].isBool()) {
			if (m_fplautoroutecheckbuttonstaronly &&
			    !(m_sensors && m_sensors->get_configfile().has_key(cfggroup_mainwindow, "autoroutestaronly")))
				m_fplautoroutecheckbuttonstaronly->set_active(cmd["staronly"].asBool());
		}
		return;
	}
	if (cmdname == "crossing") {
		bool init(cmd.isMember("cmdseq") && cmd["cmdseq"].isString() && cmd["cmdseq"].asString() == "init");
		if (!cmd.isMember("index") || !cmd["index"].isNumeric() ||
		    !cmd.isMember("count") || !cmd["count"].isNumeric())
			return;
		unsigned int index(cmd["index"].asUInt());
		unsigned int count(cmd["count"].asUInt());
		if (!init)
			return;
		bool have_icao(cmd.isMember("icao") && cmd["icao"].isString());
		bool have_name(cmd.isMember("name") && cmd["name"].isString());
       		std::string icao(cmd["icao"].asString());
		std::string name(cmd["name"].asString());
		std::string icao_name;
		if (have_icao && !icao.empty())
			icao_name = icao;
		if (have_icao && !icao.empty() && have_name && !name.empty())
			icao_name += " ";
		if (have_name && !name.empty())
			icao_name += name;
		Point coord;
		coord.set_invalid();
		if (cmd.isMember("coordlon") && cmd["coordlon"].isNumeric() &&
		    cmd.isMember("coordlat") && cmd["coordlat"].isNumeric()) {
			coord.set_lon(cmd["coordlon"].asInt());
			coord.set_lat(cmd["coordlat"].asInt());
		}
		double radius(0);
		if (cmd.isMember("radius") && cmd["radius"].isNumeric())
			radius = cmd["radius"].asDouble();
		unsigned int minlevel(0), maxlevel(600);
		if (cmd.isMember("minlevel") && cmd["minlevel"].isNumeric())
			minlevel = cmd["minlevel"].asUInt();
		if (cmd.isMember("maxlevel") && cmd["maxlevel"].isNumeric())
			maxlevel = cmd["maxlevel"].asUInt();
		m_crossingrowchangeconn.block();
		while (m_crossingstore->children().size() <= index) {
			Gtk::TreeIter iter(m_crossingstore->append());
			Gtk::TreeModel::Row row(*iter);
			{
				Point pt;
				pt.set_invalid();
				row[m_crossingcolumns.m_coord] = pt;
			}
			row[m_crossingcolumns.m_ident] = "?";
			row[m_crossingcolumns.m_name] = "";
			row[m_crossingcolumns.m_radius] = 0;
			row[m_crossingcolumns.m_minlevel] = 0;
			row[m_crossingcolumns.m_maxlevel] = 600;
		}
		Gtk::TreeModel::Row row(m_crossingstore->children()[index]);
		row[m_crossingcolumns.m_ident] = (have_icao && !icao.empty()) ? icao : std::string();
		row[m_crossingcolumns.m_name] = icao_name;
		row[m_crossingcolumns.m_radius] = radius;
		row[m_crossingcolumns.m_minlevel] = minlevel;
		row[m_crossingcolumns.m_maxlevel] = maxlevel;
		m_crossingrowchangeconn.unblock();
	}
	if (cmdname == "enroute") {
		if (!cmd.isMember("cmdseq") || !cmd["cmdseq"].isString() || cmd["cmdseq"].asString() != "init")
			return;
		if (m_fplautoroutespinbuttondctlimit && cmd.isMember("dctlimit") && cmd["dctlimit"].isNumeric())
			m_fplautoroutespinbuttondctlimit->set_value(cmd["dctlimit"].asDouble());
		if (m_fplautoroutespinbuttondctpenalty && cmd.isMember("dctpenalty") && cmd["dctpenalty"].isNumeric())
			m_fplautoroutespinbuttondctpenalty->set_value(cmd["dctpenalty"].asDouble());
		if (m_fplautoroutespinbuttondctoffset && cmd.isMember("dctoffset") && cmd["dctoffset"].isNumeric())
			m_fplautoroutespinbuttondctoffset->set_value(cmd["dctoffset"].asDouble());
		if (m_fplautoroutespinbuttonstdroutepenalty && cmd.isMember("stdroutepenalty") && cmd["stdroutepenalty"].isNumeric())
			m_fplautoroutespinbuttonstdroutepenalty->set_value(cmd["stdroutepenalty"].asDouble());
		if (m_fplautoroutespinbuttonstdrouteoffset && cmd.isMember("stdrouteoffset") && cmd["stdrouteoffset"].isNumeric())
			m_fplautoroutespinbuttonstdrouteoffset->set_value(cmd["stdrouteoffset"].asDouble());
		if (m_fplautoroutespinbuttonstdroutedctpenalty && cmd.isMember("stdroutedctpenalty") && cmd["stdroutedctpenalty"].isNumeric())
			m_fplautoroutespinbuttonstdroutedctpenalty->set_value(cmd["stdroutedctpenalty"].asDouble());
		if (m_fplautoroutespinbuttonstdroutedctoffset && cmd.isMember("stdroutedctoffset") && cmd["stdroutedctoffset"].isNumeric())
			m_fplautoroutespinbuttonstdroutedctoffset->set_value(cmd["stdroutedctoffset"].asDouble());
		if (m_fplautoroutespinbuttonukcasdctpenalty && cmd.isMember("ukcasdctpenalty") && cmd["ukcasdctpenalty"].isNumeric())
			m_fplautoroutespinbuttonukcasdctpenalty->set_value(cmd["ukcasdctpenalty"].asDouble());
		if (m_fplautoroutespinbuttonukcasdctoffset && cmd.isMember("ukcasdctoffset") && cmd["ukcasdctoffset"].isNumeric())
			m_fplautoroutespinbuttonukcasdctoffset->set_value(cmd["ukcasdctoffset"].asDouble());
		if (m_fplautoroutespinbuttonukocasdctpenalty && cmd.isMember("ukocasdctpenalty") && cmd["ukocasdctpenalty"].isNumeric())
			m_fplautoroutespinbuttonukocasdctpenalty->set_value(cmd["ukocasdctpenalty"].asDouble());
		if (m_fplautoroutespinbuttonukocasdctoffset && cmd.isMember("ukocasdctoffset") && cmd["ukocasdctoffset"].isNumeric())
			m_fplautoroutespinbuttonukocasdctoffset->set_value(cmd["ukocasdctoffset"].asDouble());
		if (m_fplautoroutespinbuttonukenterdctpenalty && cmd.isMember("ukenterdctpenalty") && cmd["ukenterdctpenalty"].isNumeric())
			m_fplautoroutespinbuttonukenterdctpenalty->set_value(cmd["ukenterdctpenalty"].asDouble());
		if (m_fplautoroutespinbuttonukenterdctoffset && cmd.isMember("ukenterdctoffset") && cmd["ukenterdctoffset"].isNumeric())
			m_fplautoroutespinbuttonukenterdctoffset->set_value(cmd["ukenterdctoffset"].asDouble());
		if (m_fplautoroutespinbuttonukleavedctpenalty && cmd.isMember("ukleavedctpenalty") && cmd["ukleavedctpenalty"].isNumeric())
			m_fplautoroutespinbuttonukleavedctpenalty->set_value(cmd["ukleavedctpenalty"].asDouble());
		if (m_fplautoroutespinbuttonukleavedctoffset && cmd.isMember("ukleavedctoffset") && cmd["ukleavedctoffset"].isNumeric())
			m_fplautoroutespinbuttonukleavedctoffset->set_value(cmd["ukleavedctoffset"].asDouble());
		if (m_fplautoroutespinbuttonvfraspclimit && cmd.isMember("vfraspclimit") && cmd["vfraspclimit"].isNumeric())
			m_fplautoroutespinbuttonvfraspclimit->set_value(cmd["vfraspclimit"].asDouble());
		if (m_fplautoroutecheckbuttonforceenrouteifr && cmd.isMember("forceenrouteifr") && cmd["forceenrouteifr"].isBool())
			m_fplautoroutecheckbuttonforceenrouteifr->set_active(cmd["forceenrouteifr"].asBool());
		if (m_fplautoroutecheckbuttonawylevels && cmd.isMember("honourawylevels") && cmd["honourawylevels"].isBool())
			m_fplautoroutecheckbuttonawylevels->set_active(cmd["honourawylevels"].asBool());
		return;
	}
	if (cmdname == "preferred") {
		if (!cmd.isMember("cmdseq") || !cmd["cmdseq"].isString() || cmd["cmdseq"].asString() != "init")
			return;
		if (m_fplautoroutespinbuttonpreferredminlevel && cmd.isMember("minlevel") && cmd["minlevel"].isNumeric())
			m_fplautoroutespinbuttonpreferredminlevel->set_value(cmd["minlevel"].asInt());
		if (m_fplautoroutespinbuttonpreferredmaxlevel && cmd.isMember("maxlevel") && cmd["maxlevel"].isNumeric())
			m_fplautoroutespinbuttonpreferredmaxlevel->set_value(cmd["maxlevel"].asInt());
		if (m_fplautoroutespinbuttonpreferredpenalty && cmd.isMember("penalty") && cmd["penalty"].isNumeric())
			m_fplautoroutespinbuttonpreferredpenalty->set_value(cmd["penalty"].asDouble());
		if (m_fplautoroutespinbuttonpreferredclimb && cmd.isMember("climb") && cmd["climb"].isNumeric())
			m_fplautoroutespinbuttonpreferredclimb->set_value(cmd["climb"].asDouble());
		if (m_fplautoroutespinbuttonpreferreddescent && cmd.isMember("descent") && cmd["descent"].isNumeric())
			m_fplautoroutespinbuttonpreferreddescent->set_value(cmd["descent"].asDouble());
	}
	if (cmdname == "tfr") {
		if (m_fplautoroutecheckbuttontfr && cmd.isMember("available") && cmd["available"].isBool())
			m_fplautoroutecheckbuttontfr->set_sensitive(cmd["available"].asBool());
		if (!cmd.isMember("cmdseq") || !cmd["cmdseq"].isString() || cmd["cmdseq"].asString() != "init")
			return;
		if (m_fplautoroutecheckbuttontfr && cmd.isMember("enabled") && cmd["enabled"].isBool())
			m_fplautoroutecheckbuttontfr->set_active(cmd["enabled"].asBool());
		if (m_fplautoroutecheckbuttonprecompgraph && cmd.isMember("precompgraph") && cmd["precompgraph"].isBool())
			m_fplautoroutecheckbuttonprecompgraph->set_active(cmd["precompgraph"].asBool());
		if (m_fplautoroutecomboboxvalidator && cmd.isMember("validator") && cmd["validator"].isString()) {
			std::string validator(cmd["validator"].asString());
			if (validator == "default")
				m_fplautoroutecomboboxvalidator->set_active(0);
			else if (validator == "cfmu")
				m_fplautoroutecomboboxvalidator->set_active(1);
			else if (validator == "eurofpl")
				m_fplautoroutecomboboxvalidator->set_active(2);
		}
		if (m_tracerulesstore && cmd.isMember("trace") && cmd["trace"].isString()) {
			std::string trace(cmd["trace"].asString());
			m_tracerulesstore->clear();
			std::string::size_type n(0);
			for (;;) {
				std::string::size_type n1(trace.find(',', n));
				if (n1 > n) {
					std::string txt(trace.substr(n, n1 - n));
					if (!txt.empty()) {
						Gtk::TreeIter iter(m_tracerulesstore->append());
						Gtk::TreeModel::Row row(*iter);
						row[m_rulescolumns.m_rule] = txt;
					}
				}
				if (n1 == std::string::npos)
					break;
				n = n1 + 1;
			}
		}
		if (m_disablerulesstore && cmd.isMember("disable") && cmd["disable"].isString()) {
			std::string disable(cmd["disable"].asString());
			m_disablerulesstore->clear();
			std::string::size_type n(0);
			for (;;) {
				std::string::size_type n1(disable.find(',', n));
				if (n1 > n) {
					std::string txt(disable.substr(n, n1 - n));
					if (!txt.empty()) {
						Gtk::TreeIter iter(m_disablerulesstore->append());
						Gtk::TreeModel::Row row(*iter);
						row[m_rulescolumns.m_rule] = txt;
					}
				}
				if (n1 == std::string::npos)
					break;
				n = n1 + 1;
			}
		}
		return;
	}
	if (cmdname == "atmosphere") {
		if (!cmd.isMember("cmdseq") || !cmd["cmdseq"].isString() || cmd["cmdseq"].asString() != "init")
			return;
		if (m_fplautoroutecheckbuttonwind && cmd.isMember("wind") && cmd["wind"].isBool())
			m_fplautoroutecheckbuttonwind->set_active(cmd["wind"].asBool());
		return;
	}
	if (cmdname == "levels") {
		if (!cmd.isMember("cmdseq") || !cmd["cmdseq"].isString() || cmd["cmdseq"].asString() != "init")
			return;
		if (m_fplautoroutespinbuttonbaselevel && cmd.isMember("base") && cmd["base"].isNumeric() &&
		    !(m_sensors && m_sensors->get_configfile().has_key(cfggroup_mainwindow, "autoroutebaselevel")))
			m_fplautoroutespinbuttonbaselevel->set_value(cmd["base"].asInt());
		if (m_fplautoroutespinbuttontoplevel && cmd.isMember("top") && cmd["top"].isNumeric() &&
		    !(m_sensors && m_sensors->get_configfile().has_key(cfggroup_mainwindow, "autoroutetoplevel")))
			m_fplautoroutespinbuttontoplevel->set_value(cmd["top"].asInt());
		if (m_fplautoroutespinbuttonminlevel && cmd.isMember("minlevel") && cmd["minlevel"].isNumeric())
			m_fplautoroutespinbuttonminlevel->set_value(cmd["minlevel"].asInt());
		if (m_fplautoroutespinbuttonminlevelpenalty && cmd.isMember("minlevelpenalty") && cmd["minlevelpenalty"].isNumeric())
			m_fplautoroutespinbuttonminlevelpenalty->set_value(cmd["minlevelpenalty"].asDouble());
		if (m_fplautoroutespinbuttonminlevelmaxpenalty && cmd.isMember("minlevelmaxpenalty") && cmd["minlevelmaxpenalty"].isNumeric())
			m_fplautoroutespinbuttonminlevelmaxpenalty->set_value(cmd["minlevelmaxpenalty"].asDouble());
		return;
	}
}

void FlightDeckWindow::FPLAutoRouteDialog::sendjson(const Json::Value& v)
{
	if (!m_childsockaddr)
		return;
        Json::FastWriter writer;
        std::string msg(writer.write(v));
        try {
		childsocket_t sock(Gio::Socket::create(m_childsockaddr->get_family(), Gio::SOCKET_TYPE_SEQPACKET, Gio::SOCKET_PROTOCOL_DEFAULT));
                if (!sock) {
			append_log("JSON write: cannot create socket\n");
			if (true)
				std::cerr << "JSON write: cannot create socket" << std::endl;
                        return;
                }
		sock->connect(m_childsockaddr);
		Glib::signal_io().connect(sigc::bind(sigc::mem_fun(this, &FPLAutoRouteDialog::recvjson), sock),
					  sock->get_fd(), Glib::IO_IN | Glib::IO_ERR | Glib::IO_HUP);
		gssize r(sock->send(msg.c_str(), msg.size()));
                if (r == -1) {
			std::ostringstream oss;
			oss << "JSON write return value: " << r;
			append_log(oss.str() + "\n");
  			if (true)
				std::cerr << "JSON write return value: " << r << std::endl;
			return;
                }
		m_childsockets.insert(sock);
		if (true && m_fplautoroutetextviewpg2comm) {
			Glib::RefPtr<Gtk::TextBuffer> buf(m_fplautoroutetextviewpg2comm->get_buffer());
			if (buf) {
				buf->insert(buf->end(), "> " + msg);
				Gtk::TextBuffer::iterator it(buf->end());
				m_fplautoroutetextviewpg2comm->scroll_to(it);
			}
		}
		if (true)
			std::cerr << "<< " << msg << std::endl;
	} catch (const Gio::Error& error) {
		append_log("JSON write error: " + error.what() + "\n");
		if (true)
			std::cerr << "JSON write error: " << error.what() << std::endl;
                return;
        }
}
#endif
