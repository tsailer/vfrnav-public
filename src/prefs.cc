//
// C++ Implementation: prefs
//
// Description: Preferences Database
//
//
// Author: Thomas Sailer <t.sailer@alumni.ethz.ch>, (C) 2007, 2009, 2012
//
// Copyright: See COPYING file that comes with this distribution
//
//

#include "sysdeps.h"

#include "prefs.h"
#include "wmm.h"
#include "baro.h"
#include "sysdepsgui.h"

#include <sstream>
#include <libintl.h>

#include <sys/types.h>
#include <glib.h>
#include <glibmm.h>

#ifdef HAVE_LIBLOCATION
extern "C" {
#include <location/location-gps-device.h>
#include <location/location-gpsd-control.h>
}
#endif

#ifdef HAVE_GNOMEBT
//#include <gnomebt-chooser.h>
#include <bluetooth-chooser.h>
#endif

PrefsWindow::PrefsWindow(BaseObjectType * castitem, const Glib::RefPtr< Builder > & refxml)
#ifdef HAVE_HILDONMM
        : Hildon::Window(castitem),
#else
        : Gtk::Window(castitem),
#endif
          m_prefs(0), m_prefsvcruise(0), m_prefsgps(0), m_prefsautowpt(0), m_prefsvblock(0), m_prefsmaxalt(0), m_prefsglobaldbdir(0),
          m_prefsgpshost(0), m_prefsgpsport(0),
          m_prefsvmaptopo(0), m_prefsvmapnorth(0), m_prefsvmapterrain(0), m_prefsvmapterrainnames(0), m_prefsvmapterrainborders(0),
          m_prefsvmapnavaidsall(0), m_prefsvmapnavaidsvor(0), m_prefsvmapnavaidsdme(0), m_prefsvmapnavaidsndb(0),
          m_prefsvmapwaypointsall(0), m_prefsvmapwaypointslow(0), m_prefsvmapwaypointshigh(0), m_prefsvmapwaypointsrnav(0),
          m_prefsvmapwaypointsterminal(0), m_prefsvmapwaypointsvfr(0), m_prefsvmapwaypointsuser(0),
          m_prefsvmapairportsall(0), m_prefsvmapairportsciv(0), m_prefsvmapairportsmil(0), m_prefsvmapairportsloccone(0),
          m_prefsvmapairspaceall(0), m_prefsvmapairspaceab(0), m_prefsvmapairspacecd(0), m_prefsvmapairspaceefg(0), m_prefsvmapairspacespecialuse(0),
          m_prefsvmapairspacefillgnd(0), m_prefsvmapairwaylow(0), m_prefsvmapairwayhigh(0), m_prefsvmaproute(0), m_prefsvmaproutelabels(0),
	  m_prefsvmaptrack(0)
{
        get_widget(refxml, "prefsvcruise", m_prefsvcruise);
        get_widget(refxml, "prefsgps", m_prefsgps);
        get_widget(refxml, "prefsautowpt", m_prefsautowpt);
        get_widget(refxml, "prefsvblock", m_prefsvblock);
        get_widget(refxml, "prefsmaxalt", m_prefsmaxalt);
        get_widget(refxml, "prefsglobaldbdir", m_prefsglobaldbdir);
        get_widget(refxml, "prefsgpshost", m_prefsgpshost);
        get_widget(refxml, "prefsgpsport", m_prefsgpsport);
        get_widget(refxml, "prefsvmaptopo", m_prefsvmaptopo);
        get_widget(refxml, "prefsvmapnorth", m_prefsvmapnorth);
        get_widget(refxml, "prefsvmapterrain", m_prefsvmapterrain);
        get_widget(refxml, "prefsvmapterrainnames", m_prefsvmapterrainnames);
        get_widget(refxml, "prefsvmapterrainborders", m_prefsvmapterrainborders);
        get_widget(refxml, "prefsvmapnavaidsall", m_prefsvmapnavaidsall);
        get_widget(refxml, "prefsvmapnavaidsvor", m_prefsvmapnavaidsvor);
        get_widget(refxml, "prefsvmapnavaidsdme", m_prefsvmapnavaidsdme);
        get_widget(refxml, "prefsvmapnavaidsndb", m_prefsvmapnavaidsndb);
        get_widget(refxml, "prefsvmapwaypointsall", m_prefsvmapwaypointsall);
        get_widget(refxml, "prefsvmapwaypointslow", m_prefsvmapwaypointslow);
        get_widget(refxml, "prefsvmapwaypointshigh", m_prefsvmapwaypointshigh);
        get_widget(refxml, "prefsvmapwaypointsrnav", m_prefsvmapwaypointsrnav);
        get_widget(refxml, "prefsvmapwaypointsterminal", m_prefsvmapwaypointsterminal);
        get_widget(refxml, "prefsvmapwaypointsvfr", m_prefsvmapwaypointsvfr);
        get_widget(refxml, "prefsvmapwaypointsuser", m_prefsvmapwaypointsuser);
        get_widget(refxml, "prefsvmapairportsall", m_prefsvmapairportsall);
        get_widget(refxml, "prefsvmapairportsciv", m_prefsvmapairportsciv);
        get_widget(refxml, "prefsvmapairportsmil", m_prefsvmapairportsmil);
        get_widget(refxml, "prefsvmapairportsloccone", m_prefsvmapairportsloccone);
        get_widget(refxml, "prefsvmapairspaceall", m_prefsvmapairspaceall);
        get_widget(refxml, "prefsvmapairspaceab", m_prefsvmapairspaceab);
        get_widget(refxml, "prefsvmapairspacecd", m_prefsvmapairspacecd);
        get_widget(refxml, "prefsvmapairspaceefg", m_prefsvmapairspaceefg);
        get_widget(refxml, "prefsvmapairspacespecialuse", m_prefsvmapairspacespecialuse);
        get_widget(refxml, "prefsvmapairwaylow", m_prefsvmapairwaylow);
        get_widget(refxml, "prefsvmapairwayhigh", m_prefsvmapairwayhigh);
        get_widget(refxml, "prefsvmaproute", m_prefsvmaproute);
        get_widget(refxml, "prefsvmaproutelabels", m_prefsvmaproutelabels);
        get_widget(refxml, "prefsvmaptrack", m_prefsvmaptrack);
        get_widget(refxml, "prefsvmaptrackpoints", m_prefsvmaptrackpoints);
        get_widget(refxml, "prefsvmapairspacefillgnd", m_prefsvmapairspacefillgnd);
        {
                Gtk::Label *lblgpshost(0), *lblgpsport(0);
                get_widget(refxml, "prefslabelgpshost", lblgpshost);
                get_widget(refxml, "prefslabelgpsport", lblgpsport);
#ifdef HAVE_HILDONMM
                Hildon::Program::get_instance()->add_window(*this);
                m_prefsgpshost->hide();
                m_prefsgpsport->hide();
                lblgpshost->hide();
                lblgpsport->hide();
#endif
#ifdef HAVE_GYPSY
                m_prefsgpshost->hide();
                lblgpshost->hide();
#endif
        }

        signal_delete_event().connect(sigc::mem_fun(*this, &PrefsWindow::window_delete_event));
        // connect buttons
        Gtk::Button *b = 0;
        get_widget(refxml, "prefsbuttoncancel", b);
        b->signal_clicked().connect(sigc::mem_fun(*this, &PrefsWindow::button_prefscancel));
        b = 0;
        get_widget(refxml, "prefsbuttonapply", b);
        b->signal_clicked().connect(sigc::mem_fun(*this, &PrefsWindow::button_prefsapply));
        b = 0;
        get_widget(refxml, "prefsbuttonok", b);
        b->signal_clicked().connect(sigc::mem_fun(*this, &PrefsWindow::button_prefsok));
        b = 0;
        get_widget(refxml, "prefsglobaldbdirchange", b);
        b->signal_clicked().connect(sigc::mem_fun(*this, &PrefsWindow::button_globaldbdirchange));
        b = 0;
        get_widget(refxml, "prefsgpsportbluetooth", b);
        b->signal_clicked().connect(sigc::mem_fun(*this, &PrefsWindow::button_gpsportbluetooth));
#if defined(HAVE_GNOMEBT) && defined(HAVE_GYPSY)
        b->show();
#else
        b->hide();
#endif

        m_prefsvmapnavaidsall->signal_toggled().connect(sigc::mem_fun(*this, &PrefsWindow::change_vmapnavaidsall));
        m_prefsvmapnavaidsvor->signal_toggled().connect(sigc::mem_fun(*this, &PrefsWindow::change_vmapnavaids));
        m_prefsvmapnavaidsdme->signal_toggled().connect(sigc::mem_fun(*this, &PrefsWindow::change_vmapnavaids));
        m_prefsvmapnavaidsndb->signal_toggled().connect(sigc::mem_fun(*this, &PrefsWindow::change_vmapnavaids));
        m_prefsvmapwaypointsall->signal_toggled().connect(sigc::mem_fun(*this, &PrefsWindow::change_vmapwaypointsall));
        m_prefsvmapwaypointslow->signal_toggled().connect(sigc::mem_fun(*this, &PrefsWindow::change_vmapwaypoints));
        m_prefsvmapwaypointshigh->signal_toggled().connect(sigc::mem_fun(*this, &PrefsWindow::change_vmapwaypoints));
        m_prefsvmapwaypointsrnav->signal_toggled().connect(sigc::mem_fun(*this, &PrefsWindow::change_vmapwaypoints));
        m_prefsvmapwaypointsterminal->signal_toggled().connect(sigc::mem_fun(*this, &PrefsWindow::change_vmapwaypoints));
        m_prefsvmapwaypointsvfr->signal_toggled().connect(sigc::mem_fun(*this, &PrefsWindow::change_vmapwaypoints));
        m_prefsvmapwaypointsuser->signal_toggled().connect(sigc::mem_fun(*this, &PrefsWindow::change_vmapwaypoints));
        m_prefsvmapairportsall->signal_toggled().connect(sigc::mem_fun(*this, &PrefsWindow::change_vmapairportsall));
        m_prefsvmapairportsciv->signal_toggled().connect(sigc::mem_fun(*this, &PrefsWindow::change_vmapairports));
        m_prefsvmapairportsmil->signal_toggled().connect(sigc::mem_fun(*this, &PrefsWindow::change_vmapairports));
        m_prefsvmapairspaceall->signal_toggled().connect(sigc::mem_fun(*this, &PrefsWindow::change_vmapairspacesall));
        m_prefsvmapairspaceab->signal_toggled().connect(sigc::mem_fun(*this, &PrefsWindow::change_vmapairspaces));
        m_prefsvmapairspacecd->signal_toggled().connect(sigc::mem_fun(*this, &PrefsWindow::change_vmapairspaces));
        m_prefsvmapairspaceefg->signal_toggled().connect(sigc::mem_fun(*this, &PrefsWindow::change_vmapairspaces));
        m_prefsvmapairspacespecialuse->signal_toggled().connect(sigc::mem_fun(*this, &PrefsWindow::change_vmapairspaces));
}

PrefsWindow::~PrefsWindow()
{
#ifdef HAVE_HILDONMM
        Hildon::Program::get_instance()->remove_window(*this);
#endif
}

Glib::RefPtr<PrefsWindow> PrefsWindow::create(Preferences& prefs)
{
        Glib::RefPtr<Builder> refxml;
        // load glade file
        refxml = load_glade_file("prefs" UIEXT, "preferenceswindow");
        // construct windows
        PrefsWindow *preferenceswindow;
        get_widget_derived(refxml, "preferenceswindow", preferenceswindow);
        preferenceswindow->hide();
        preferenceswindow->m_prefs = &prefs;
        preferenceswindow->connect_engine();
        preferenceswindow->load_prefs();
        return Glib::RefPtr<PrefsWindow>(preferenceswindow);
}

void PrefsWindow::connect_engine(void)
{
        m_prefs->vcruise.signal_change().connect(sigc::mem_fun(*this, &PrefsWindow::change_vcruise));
        m_prefs->gps.signal_change().connect(sigc::mem_fun(*this, &PrefsWindow::change_gps));
        m_prefs->autowpt.signal_change().connect(sigc::mem_fun(*this, &PrefsWindow::change_autowpt));
        m_prefs->vblock.signal_change().connect(sigc::mem_fun(*this, &PrefsWindow::change_vblock));
        m_prefs->maxalt.signal_change().connect(sigc::mem_fun(*this, &PrefsWindow::change_maxalt));
        m_prefs->mapflags.signal_change().connect(sigc::mem_fun(*this, &PrefsWindow::change_mapflags));
        m_prefs->globaldbdir.signal_change().connect(sigc::mem_fun(*this, &PrefsWindow::change_globaldbdir));
        m_prefs->gpshost.signal_change().connect(sigc::mem_fun(*this, &PrefsWindow::change_gpshost));
        m_prefs->gpsport.signal_change().connect(sigc::mem_fun(*this, &PrefsWindow::change_gpsport));
}

void PrefsWindow::change_vcruise(double v)
{
        std::ostringstream oss;
        oss << m_prefs->vcruise;
        m_prefsvcruise->set_text(oss.str());
}

void PrefsWindow::change_gps(bool v)
{
        m_prefsgps->set_active(m_prefs->gps);
}

void PrefsWindow::change_autowpt(bool v)
{
        m_prefsautowpt->set_active(m_prefs->autowpt);
}

void PrefsWindow::change_vblock(double v)
{
        std::ostringstream oss;
        oss << m_prefs->vblock;
        m_prefsvblock->set_text(oss.str());
}

void PrefsWindow::change_maxalt(int v)
{
        std::ostringstream oss;
        oss << m_prefs->maxalt;
        m_prefsmaxalt->set_text(oss.str());
}

void PrefsWindow::change_mapflags(int v)
{
        MapRenderer::DrawFlags df((MapRenderer::DrawFlags)v);
        m_prefsvmaptopo->set_active(!!(df & MapRenderer::drawflags_topo));
        m_prefsvmapnorth->set_active(!!(df & MapRenderer::drawflags_northpointer));
        m_prefsvmapterrain->set_active(!!(df & MapRenderer::drawflags_terrain));
        m_prefsvmapterrainnames->set_active(!!(df & MapRenderer::drawflags_terrain_names));
        m_prefsvmapterrainborders->set_active(!!(df & MapRenderer::drawflags_terrain_borders));
        if ((df & MapRenderer::drawflags_navaids) == MapRenderer::drawflags_navaids) {
                m_prefsvmapnavaidsall->set_inconsistent(false);
                m_prefsvmapnavaidsall->set_active(true);
        } else if (!(df & MapRenderer::drawflags_navaids)) {
                m_prefsvmapnavaidsall->set_inconsistent(false);
                m_prefsvmapnavaidsall->set_active(false);
        } else {
                m_prefsvmapairportsall->set_inconsistent(true);
        }
        m_prefsvmapnavaidsvor->set_active(!!(df & MapRenderer::drawflags_navaids_vor));
        m_prefsvmapnavaidsdme->set_active(!!(df & MapRenderer::drawflags_navaids_dme));
        m_prefsvmapnavaidsndb->set_active(!!(df & MapRenderer::drawflags_navaids_ndb));
        if ((df & MapRenderer::drawflags_waypoints) == MapRenderer::drawflags_waypoints) {
                m_prefsvmapwaypointsall->set_inconsistent(false);
                m_prefsvmapwaypointsall->set_active(true);
        } else if (!(df & MapRenderer::drawflags_waypoints)) {
                m_prefsvmapwaypointsall->set_inconsistent(false);
                m_prefsvmapwaypointsall->set_active(false);
        } else {
                m_prefsvmapwaypointsall->set_inconsistent(true);
        }
        m_prefsvmapwaypointslow->set_active(!!(df & MapRenderer::drawflags_waypoints_low));
        m_prefsvmapwaypointshigh->set_active(!!(df & MapRenderer::drawflags_waypoints_high));
        m_prefsvmapwaypointsrnav->set_active(!!(df & MapRenderer::drawflags_waypoints_rnav));
        m_prefsvmapwaypointsterminal->set_active(!!(df & MapRenderer::drawflags_waypoints_terminal));
        m_prefsvmapwaypointsvfr->set_active(!!(df & MapRenderer::drawflags_waypoints_vfr));
        m_prefsvmapwaypointsuser->set_active(!!(df & MapRenderer::drawflags_waypoints_user));
        if ((df & MapRenderer::drawflags_airports) == MapRenderer::drawflags_airports) {
                m_prefsvmapairportsall->set_inconsistent(false);
                m_prefsvmapairportsall->set_active(true);
        } else if (!(df & MapRenderer::drawflags_airports)) {
                m_prefsvmapairportsall->set_inconsistent(false);
                m_prefsvmapairportsall->set_active(false);
        } else {
                m_prefsvmapairportsall->set_inconsistent(true);
        }
        m_prefsvmapairportsciv->set_active(!!(df & MapRenderer::drawflags_airports_civ));
        m_prefsvmapairportsmil->set_active(!!(df & MapRenderer::drawflags_airports_mil));
        m_prefsvmapairportsloccone->set_active(!!(df & MapRenderer::drawflags_airports_localizercone));
        if ((df & MapRenderer::drawflags_airspaces) == MapRenderer::drawflags_airspaces) {
                m_prefsvmapairspaceall->set_inconsistent(false);
                m_prefsvmapairspaceall->set_active(true);
        } else if (!(df & MapRenderer::drawflags_airspaces)) {
                m_prefsvmapairspaceall->set_inconsistent(false);
                m_prefsvmapairspaceall->set_active(false);
        } else {
                m_prefsvmapairspaceall->set_inconsistent(true);
        }
        m_prefsvmapairspaceab->set_active(!!(df & MapRenderer::drawflags_airspaces_ab));
        m_prefsvmapairspacecd->set_active(!!(df & MapRenderer::drawflags_airspaces_cd));
        m_prefsvmapairspaceefg->set_active(!!(df & MapRenderer::drawflags_airspaces_efg));
        m_prefsvmapairspacespecialuse->set_active(!!(df & MapRenderer::drawflags_airspaces_specialuse));
        m_prefsvmaptrack->set_active(!!(df & MapRenderer::drawflags_track));
        m_prefsvmaptrackpoints->set_active(!!(df & MapRenderer::drawflags_track_points));
        m_prefsvmaproute->set_active(!!(df & MapRenderer::drawflags_route));
        m_prefsvmaproutelabels->set_active(!!(df & MapRenderer::drawflags_route_labels));
        m_prefsvmapairspacefillgnd->set_active(!!(df & MapRenderer::drawflags_airspaces_fill_ground));
        m_prefsvmapairwaylow->set_active(!!(df & MapRenderer::drawflags_airways_low));
        m_prefsvmapairwayhigh->set_active(!!(df & MapRenderer::drawflags_airways_high));
}

void PrefsWindow::change_globaldbdir(Glib::ustring v)
{
        m_prefsglobaldbdir->set_text(v);
}

void PrefsWindow::change_gpshost(Glib::ustring v)
{
        m_prefsgpshost->set_text(v);
}

void PrefsWindow::change_gpsport(Glib::ustring v)
{
        m_prefsgpsport->set_text(v);
}

void PrefsWindow::change_vmapnavaidsall(void)
{
        if (m_prefsvmapnavaidsall->get_inconsistent()) {
                m_prefsvmapnavaidsall->set_inconsistent(false);
                m_prefsvmapnavaidsall->set_active(false);
        }
        bool v(m_prefsvmapnavaidsall->get_active());
        m_prefsvmapnavaidsvor->set_active(v);
        m_prefsvmapnavaidsdme->set_active(v);
        m_prefsvmapnavaidsndb->set_active(v);
}

void PrefsWindow::change_vmapnavaids(void)
{
        unsigned int cnt(0);
        if (m_prefsvmapnavaidsvor->get_active())
                cnt++;
        if (m_prefsvmapnavaidsdme->get_active())
                cnt++;
        if (m_prefsvmapnavaidsndb->get_active())
                cnt++;
        if (cnt > 0 && cnt < 3) {
                m_prefsvmapnavaidsall->set_inconsistent(true);
                return;
        }
        m_prefsvmapnavaidsall->set_inconsistent(false);
        m_prefsvmapnavaidsall->set_active(!!cnt);
}

void PrefsWindow::change_vmapwaypointsall(void)
{
        if (m_prefsvmapwaypointsall->get_inconsistent()) {
                m_prefsvmapwaypointsall->set_inconsistent(false);
                m_prefsvmapwaypointsall->set_active(false);
        }
        bool v(m_prefsvmapwaypointsall->get_active());
        m_prefsvmapwaypointslow->set_active(v);
        m_prefsvmapwaypointshigh->set_active(v);
        m_prefsvmapwaypointsrnav->set_active(v);
        m_prefsvmapwaypointsterminal->set_active(v);
        m_prefsvmapwaypointsvfr->set_active(v);
        m_prefsvmapwaypointsuser->set_active(v);
}

void PrefsWindow::change_vmapwaypoints(void)
{
        unsigned int cnt(0);
        if (m_prefsvmapwaypointslow->get_active())
                cnt++;
        if (m_prefsvmapwaypointshigh->get_active())
                cnt++;
        if (m_prefsvmapwaypointsrnav->get_active())
                cnt++;
        if (m_prefsvmapwaypointsterminal->get_active())
                cnt++;
        if (m_prefsvmapwaypointsvfr->get_active())
                cnt++;
        if (m_prefsvmapwaypointsuser->get_active())
                cnt++;
        if (cnt > 0 && cnt < 6) {
                m_prefsvmapwaypointsall->set_inconsistent(true);
                return;
        }
        m_prefsvmapwaypointsall->set_inconsistent(false);
        m_prefsvmapwaypointsall->set_active(!!cnt);
}

void PrefsWindow::change_vmapairportsall(void)
{
        if (m_prefsvmapairportsall->get_inconsistent()) {
                m_prefsvmapairportsall->set_inconsistent(false);
                m_prefsvmapairportsall->set_active(false);
        }
        bool v(m_prefsvmapairportsall->get_active());
        m_prefsvmapairportsciv->set_active(v);
        m_prefsvmapairportsmil->set_active(v);
}

void PrefsWindow::change_vmapairports(void)
{
        unsigned int cnt(0);
        if (m_prefsvmapairportsciv->get_active())
                cnt++;
        if (m_prefsvmapairportsmil->get_active())
                cnt++;
        if (cnt > 0 && cnt < 2) {
                m_prefsvmapairportsall->set_inconsistent(true);
                return;
        }
        m_prefsvmapairportsall->set_inconsistent(false);
        m_prefsvmapairportsall->set_active(!!cnt);
}

void PrefsWindow::change_vmapairspacesall(void)
{
        if (m_prefsvmapairspaceall->get_inconsistent()) {
                m_prefsvmapairspaceall->set_inconsistent(false);
                m_prefsvmapairspaceall->set_active(false);
        }
        bool v(m_prefsvmapairspaceall->get_active());
        m_prefsvmapairspaceab->set_active(v);
        m_prefsvmapairspacecd->set_active(v);
        m_prefsvmapairspaceefg->set_active(v);
        m_prefsvmapairspacespecialuse->set_active(v);
}

void PrefsWindow::change_vmapairspaces(void)
{
        unsigned int cnt(0);
        if (m_prefsvmapairspaceab->get_active())
                cnt++;
        if (m_prefsvmapairspacecd->get_active())
                cnt++;
        if (m_prefsvmapairspaceefg->get_active())
                cnt++;
        if (m_prefsvmapairspacespecialuse->get_active())
                cnt++;
        if (cnt > 0 && cnt < 4) {
                m_prefsvmapairspaceall->set_inconsistent(true);
                return;
        }
        m_prefsvmapairspaceall->set_inconsistent(false);
        m_prefsvmapairspaceall->set_active(!!cnt);
}

bool PrefsWindow::window_delete_event(GdkEventAny * event)
{
        hide();
        return true;
}

void PrefsWindow::button_prefscancel( void )
{
        load_prefs();
        hide();
}

void PrefsWindow::button_prefsapply( void )
{
        save_prefs();
        load_prefs();
}

void PrefsWindow::button_prefsok( void )
{
        save_prefs();
        load_prefs();
        hide();
}

void PrefsWindow::button_globaldbdirchange(void)
{
#ifdef HAVE_HILDONMM
        Hildon::FileChooserDialog dlg(Gtk::FILE_CHOOSER_ACTION_SELECT_FOLDER);
#else
        Gtk::FileChooserDialog dlg("Global Database Folder", Gtk::FILE_CHOOSER_ACTION_SELECT_FOLDER);
#endif
	{
		const Glib::ustring& dir(m_prefs->globaldbdir);
		dlg.set_filename(dir);
	}
        dlg.add_button(Gtk::Stock::OK, Gtk::RESPONSE_OK);
        dlg.add_button(Gtk::Stock::CANCEL, Gtk::RESPONSE_CANCEL);
        dlg.set_modal(true);
        int resp = dlg.run();
        dlg.hide();
        switch (resp) {
                case Gtk::RESPONSE_ACCEPT:
                case Gtk::RESPONSE_OK:
                case Gtk::RESPONSE_YES:
                case Gtk::RESPONSE_APPLY:
                        m_prefs->globaldbdir = dlg.get_filename();
                        break;
        }
}

void PrefsWindow::button_gpsportbluetooth(void)
{
#ifdef HAVE_GNOMEBT
	GtkWidget *chooser = bluetooth_chooser_new();
        gint result = gtk_dialog_run(GTK_DIALOG(chooser));
        switch (result) {
                case GTK_RESPONSE_OK:
                {
                        gchar *choice = bluetooth_chooser_get_selected_device(BLUETOOTH_CHOOSER(chooser));
                        m_prefs->gpsport = choice;
                        g_free(choice);
                        break;
                }

                default:
                        break;
        }
        gtk_widget_destroy(GTK_WIDGET(chooser));
#endif
}

void PrefsWindow::load_prefs(void)
{
        if (!m_prefs)
                return;
        change_vcruise(m_prefs->vcruise);
	change_gps(m_prefs->gps);
        change_autowpt(m_prefs->autowpt);
        change_vblock(m_prefs->vblock);
        change_maxalt(m_prefs->maxalt);
        change_mapflags(m_prefs->mapflags);
        change_globaldbdir(m_prefs->globaldbdir);
        change_gpshost(m_prefs->gpshost);
        change_gpsport(m_prefs->gpsport);
}

void PrefsWindow::save_prefs(void)
{
        if (!m_prefs)
                return;
        {
                std::istringstream iss(m_prefsvcruise->get_text());
                double v;
                iss >> v;
                m_prefs->vcruise = v;
        }
	m_prefs->gps = m_prefsgps->get_active();
        m_prefs->autowpt = m_prefsautowpt->get_active();
        {
                std::istringstream iss(m_prefsvblock->get_text());
                double v;
                iss >> v;
                m_prefs->vblock = v;
        }
        {
                std::istringstream iss(m_prefsmaxalt->get_text());
                int v;
                iss >> v;
                m_prefs->maxalt = v;
        }
        m_prefs->globaldbdir = m_prefsglobaldbdir->get_text();
        m_prefs->gpshost = m_prefsgpshost->get_text();
        m_prefs->gpsport = m_prefsgpsport->get_text();
        {
                MapRenderer::DrawFlags df(MapRenderer::drawflags_none);
                if (m_prefsvmaptopo->get_active())
                        df |= MapRenderer::drawflags_topo;
                if (m_prefsvmapnorth->get_active())
                        df |= MapRenderer::drawflags_northpointer;
                if (m_prefsvmapterrain->get_active())
                        df |= MapRenderer::drawflags_terrain;
                if (m_prefsvmapterrainnames->get_active())
                        df |= MapRenderer::drawflags_terrain_names;
                if (m_prefsvmapterrainborders->get_active())
                        df |= MapRenderer::drawflags_terrain_borders;
                if (m_prefsvmapnavaidsvor->get_active())
                        df |= MapRenderer::drawflags_navaids_vor;
                if (m_prefsvmapnavaidsdme->get_active())
                        df |= MapRenderer::drawflags_navaids_dme;
                if (m_prefsvmapnavaidsndb->get_active())
                        df |= MapRenderer::drawflags_navaids_ndb;
                if (m_prefsvmapwaypointslow->get_active())
                        df |= MapRenderer::drawflags_waypoints_low;
                if (m_prefsvmapwaypointshigh->get_active())
                        df |= MapRenderer::drawflags_waypoints_high;
                if (m_prefsvmapwaypointsrnav->get_active())
                        df |= MapRenderer::drawflags_waypoints_rnav;
                if (m_prefsvmapwaypointsterminal->get_active())
                        df |= MapRenderer::drawflags_waypoints_terminal;
                if (m_prefsvmapwaypointsvfr->get_active())
                        df |= MapRenderer::drawflags_waypoints_vfr;
                if (m_prefsvmapwaypointsuser->get_active())
                        df |= MapRenderer::drawflags_waypoints_user;
                if (m_prefsvmapairportsciv->get_active())
                        df |= MapRenderer::drawflags_airports_civ;
                if (m_prefsvmapairportsmil->get_active())
                        df |= MapRenderer::drawflags_airports_mil;
                if (m_prefsvmapairportsloccone->get_active())
                        df |= MapRenderer::drawflags_airports_localizercone;
                if (m_prefsvmapairspaceab->get_active())
                        df |= MapRenderer::drawflags_airspaces_ab;
                if (m_prefsvmapairspacecd->get_active())
                        df |= MapRenderer::drawflags_airspaces_cd;
                if (m_prefsvmapairspaceefg->get_active())
                        df |= MapRenderer::drawflags_airspaces_efg;
                if (m_prefsvmapairspacespecialuse->get_active())
                        df |= MapRenderer::drawflags_airspaces_specialuse;
                if (m_prefsvmapairspacefillgnd->get_active())
                        df |= MapRenderer::drawflags_airspaces_fill_ground;
                if (m_prefsvmaproute->get_active())
                        df |= MapRenderer::drawflags_route;
                if (m_prefsvmaproutelabels->get_active())
                        df |= MapRenderer::drawflags_route_labels;
                if (m_prefsvmaptrack->get_active())
                        df |= MapRenderer::drawflags_track;
                if (m_prefsvmaptrackpoints->get_active())
                        df |= MapRenderer::drawflags_track_points;
                if (m_prefsvmapairwaylow->get_active())
                        df |= MapRenderer::drawflags_airways_low;
                if (m_prefsvmapairwayhigh->get_active())
                        df |= MapRenderer::drawflags_airways_high;
                m_prefs->mapflags = (int)df;
        }
        m_prefs->commit();
}

NewWaypointWindow::UsageModelColumns::UsageModelColumns(void )
{
        add(m_col_usage);
        add(m_col_text);
}

NewWaypointWindow::NameIcaoModelColumns::NameIcaoModelColumns(void )
{
        add(m_col_selectable);
        add(m_col_icao);
        add(m_col_name);
        add(m_col_dist);
}

NewWaypointWindow::NewWaypointWindow(BaseObjectType * castitem, const Glib::RefPtr< Builder > & refxml)
#ifdef HAVE_HILDONMM
        : Hildon::Window(castitem),
#else
        : Gtk::Window(castitem),
#endif
          m_engine(0), m_referenced(false), m_newwaypointlon(0), m_newwaypointlat(0), m_newwaypointusage(0), m_newwaypointnamebox(0),
          m_newwaypointname(0), m_newwaypointicao(0), m_newwaypointmodtime(0), 
          m_newwaypointbcancel(0), m_newwaypointbok(0)
{
        get_widget(refxml, "newwaypointlon", m_newwaypointlon);
        get_widget(refxml, "newwaypointlat", m_newwaypointlat);
        get_widget(refxml, "newwaypointusage", m_newwaypointusage);
        get_widget(refxml, "newwaypointnamebox", m_newwaypointnamebox);
        get_widget(refxml, "newwaypointname", m_newwaypointname);
        get_widget(refxml, "newwaypointicao", m_newwaypointicao);
        get_widget(refxml, "newwaypointmodtime", m_newwaypointmodtime);
        get_widget(refxml, "newwaypointsourceid", m_newwaypointsourceid);
        get_widget(refxml, "newwaypointbcancel", m_newwaypointbcancel);
        get_widget(refxml, "newwaypointbok", m_newwaypointbok);
#ifdef HAVE_HILDONMM
        Hildon::Program::get_instance()->add_window(*this);
#endif
        m_querydispatch.connect(sigc::mem_fun(*this, &NewWaypointWindow::async_done));
        signal_delete_event().connect(sigc::mem_fun(*this, &NewWaypointWindow::window_delete_event));
        m_newwaypointbok->signal_clicked().connect(sigc::mem_fun(*this, &NewWaypointWindow::button_ok));
        m_newwaypointbcancel->signal_clicked().connect(sigc::mem_fun(*this, &NewWaypointWindow::button_cancel));
        m_newwaypointmodtime->signal_focus_out_event().connect(sigc::hide(sigc::bind_return(sigc::bind(sigc::ptr_fun(&NewWaypointWindow::time_changed), m_newwaypointmodtime), true)));
        m_newwaypointlon->signal_focus_out_event().connect(sigc::hide(sigc::bind_return(sigc::bind(sigc::ptr_fun(&NewWaypointWindow::lon_changed), m_newwaypointlon), true)));
        m_newwaypointlat->signal_focus_out_event().connect(sigc::hide(sigc::bind_return(sigc::bind(sigc::ptr_fun(&NewWaypointWindow::lat_changed), m_newwaypointlat), true)));
        typedef void (Gtk::ComboBox::*set_active_t)(int);
        m_newwaypointname->signal_changed().connect(sigc::bind(sigc::mem_fun(*m_newwaypointnamebox, (set_active_t)&Gtk::ComboBox::set_active), -1));
        m_newwaypointicao->signal_changed().connect(sigc::bind(sigc::mem_fun(*m_newwaypointnamebox, (set_active_t)&Gtk::ComboBox::set_active), -1));
        m_usagemodelstore = Gtk::ListStore::create(m_usagemodelcolumns);
        m_nameicaomodelstore = Gtk::TreeStore::create(m_nameicaomodelcolumns);
        m_newwaypointusage->set_model(m_usagemodelstore);
        m_newwaypointusage->pack_start(m_usagemodelcolumns.m_col_text);
        m_newwaypointnamebox->set_model(m_nameicaomodelstore);
        {
                Gtk::CellRendererText *name_renderer = Gtk::manage(new Gtk::CellRendererText());
                name_renderer->set_property("xalign", 0.0);
                m_newwaypointnamebox->pack_start(*name_renderer, true);
                m_newwaypointnamebox->add_attribute(*name_renderer, "text", m_nameicaomodelcolumns.m_col_name);
                Gtk::CellRendererText *icao_renderer = Gtk::manage(new Gtk::CellRendererText());
                icao_renderer->set_property("xalign", 0.0);
                m_newwaypointnamebox->pack_start(*icao_renderer, true);
                m_newwaypointnamebox->add_attribute(*icao_renderer, "text", m_nameicaomodelcolumns.m_col_icao);
                Gtk::CellRendererText *dist_renderer = Gtk::manage(new Gtk::CellRendererText());
                dist_renderer->set_property("xalign", 1.0);
                m_newwaypointnamebox->pack_start(*dist_renderer, true);
                m_newwaypointnamebox->add_attribute(*dist_renderer, "text", m_nameicaomodelcolumns.m_col_dist);
                //m_newwaypointnamebox->pack_start(m_nameicaomodelcolumns.m_col_name);
                //m_newwaypointnamebox->pack_start(m_nameicaomodelcolumns.m_col_icao);
                //m_newwaypointnamebox->pack_start(m_nameicaomodelcolumns.m_col_dist);
        }
        m_newwaypointnamebox->signal_changed().connect(sigc::mem_fun(*this, &NewWaypointWindow::nameicao_changed));
        {
                static const char usages[] = {
                        WaypointsDb::Waypoint::usage_highlevel,
                        WaypointsDb::Waypoint::usage_lowlevel,
                        WaypointsDb::Waypoint::usage_bothlevel,
                        WaypointsDb::Waypoint::usage_rnav,
                        WaypointsDb::Waypoint::usage_terminal,
                        WaypointsDb::Waypoint::usage_vfr,
                        WaypointsDb::Waypoint::usage_user
                };
                for (unsigned int i = 0; i < sizeof(usages)/sizeof(usages[0]); i++) {
                        Gtk::TreeModel::iterator iter(m_usagemodelstore->append());
                        Gtk::TreeRow row(*iter);
                        row[m_usagemodelcolumns.m_col_usage] = usages[i];
                        row[m_usagemodelcolumns.m_col_text] = WaypointsDb::Waypoint::get_usage_name(usages[i]);
                        if (usages[i] == WaypointsDb::Waypoint::usage_user)
                                m_newwaypointusage->set_active(iter);
                }
        }
}

NewWaypointWindow::~NewWaypointWindow()
{
        async_cancel();
#ifdef HAVE_HILDONMM
        Hildon::Program::get_instance()->remove_window(*this);
#endif
}

Glib::RefPtr< NewWaypointWindow > NewWaypointWindow::create(Engine & eng, const Point & coord, const Glib::ustring & name, const Glib::ustring & icao)
{
        Glib::RefPtr<Builder> refxml;
        // load glade file
        refxml = load_glade_file("prefs" UIEXT, "newwaypointwindow");
        // construct windows
        NewWaypointWindow *window;
        get_widget_derived(refxml, "newwaypointwindow", window);
        window->hide();
        window->m_engine = &eng;
        window->reference();
        window->m_referenced = true;
        window->m_coord = coord;
        window->m_newwaypointlon->set_text(coord.get_lon_str());
        window->m_newwaypointlat->set_text(coord.get_lat_str());
        window->m_newwaypointname->set_text(name);
        window->m_newwaypointicao->set_text(icao);
        window->m_newwaypointmodtime->set_text(Conversions::time_str("%Y-%m-%d %H:%M:%S", time(0)));
        window->m_newwaypointsourceid->set_text(make_sourceid());
        Rect bbox(coord.simple_box_nmi(max_distance));
        window->async_cancel();
        window->m_airportquery = eng.async_airport_find_nearest(coord, max_number, bbox, AirportsDb::element_t::subtables_none);
        window->m_navaidquery = eng.async_navaid_find_nearest(coord, max_number, bbox, NavaidsDb::element_t::subtables_none);
        window->m_waypointquery = eng.async_waypoint_find_nearest(coord, max_number, bbox, WaypointsDb::element_t::subtables_none);
        window->m_mapelementquery = eng.async_mapelement_find_nearest(coord, 8 * max_number, bbox, MapelementsDb::element_t::subtables_none);
        if (window->m_airportquery)
                window->m_airportquery->connect(sigc::mem_fun(*window, &NewWaypointWindow::async_dispatchdone));
        if (window->m_navaidquery)
                window->m_navaidquery->connect(sigc::mem_fun(*window, &NewWaypointWindow::async_dispatchdone));
        if (window->m_waypointquery)
                window->m_waypointquery->connect(sigc::mem_fun(*window, &NewWaypointWindow::async_dispatchdone));
        if (window->m_mapelementquery)
                window->m_mapelementquery->connect(sigc::mem_fun(*window, &NewWaypointWindow::async_dispatchdone));
        return Glib::RefPtr<NewWaypointWindow>(window);
}

Glib::ustring NewWaypointWindow::make_sourceid( void )
{
        Glib::ustring machname("unknown");
        {
                const gchar *n(g_get_host_name());
                if (n)
                        machname = n;
        }
        Glib::ustring username(Glib::get_user_name());
        Glib::TimeVal tv;
        tv.assign_current_time();
        char buf[32];
        snprintf(buf, sizeof(buf), "%.6f", tv.as_double());
        return Glib::ustring(buf) + "." + username + "@" + machname;
}

void NewWaypointWindow::button_cancel(void )
{
        hide();
        async_cancel();
        if (!m_referenced)
                return;
        m_referenced = false;
        unreference();
}

void NewWaypointWindow::button_ok(void )
{
        WaypointsDb::Waypoint e;
        Point coord;
        coord.set_lon_str(m_newwaypointlon->get_text());
        coord.set_lat_str(m_newwaypointlat->get_text());
        e.set_sourceid(m_newwaypointsourceid->get_text());
        e.set_coord(coord);
        e.set_icao(m_newwaypointicao->get_text());
        e.set_name(m_newwaypointname->get_text());
        e.set_label_placement(WaypointsDb::Waypoint::label_n);
        e.set_modtime(Conversions::time_parse(m_newwaypointmodtime->get_text()));
        Gtk::TreeModel::iterator iter(m_newwaypointusage->get_active());
        if (!iter)
                return;
        Gtk::TreeModel::Row row(*iter);
        e.set_usage(row[m_usagemodelcolumns.m_col_usage]);
        m_engine->async_waypoint_save(e);
        button_cancel();
}

bool NewWaypointWindow::window_delete_event(GdkEventAny * event)
{
        button_cancel();
        return true;
}

void NewWaypointWindow::async_dispatchdone( void )
{
        m_querydispatch();
}

void NewWaypointWindow::async_done( void )
{
        if (m_airportquery && m_airportquery->is_done()) {
                if (!m_airportquery->is_error()) {
                        Engine::AirportResult::elementvector_t& ev(m_airportquery->get_result());
                        Gtk::TreeModel::Row prow(*m_nameicaomodelstore->append());
                        prow[m_nameicaomodelcolumns.m_col_name] = "Airports";
                        prow[m_nameicaomodelcolumns.m_col_selectable] = false;
                        for (Engine::AirportResult::elementvector_t::const_iterator ei(ev.begin()), ee(ev.end()); ei != ee; ei++) {
                                Gtk::TreeModel::Row row(*m_nameicaomodelstore->append(prow.children()));
                                row[m_nameicaomodelcolumns.m_col_selectable] = true;
                                row[m_nameicaomodelcolumns.m_col_name] = ei->get_name();
                                row[m_nameicaomodelcolumns.m_col_icao] = ei->get_icao();
                                row[m_nameicaomodelcolumns.m_col_dist] = Conversions::dist_str(m_coord.spheric_distance_nmi(ei->get_coord()));
                        }
                        m_nameicaomodelstore->set_sort_column(m_nameicaomodelcolumns.m_col_dist, Gtk::SORT_ASCENDING);
                }
                m_airportquery = Glib::RefPtr<Engine::AirportResult>();
        }
        if (m_navaidquery && m_navaidquery->is_done()) {
                if (!m_navaidquery->is_error()) {
                        Engine::NavaidResult::elementvector_t& ev(m_navaidquery->get_result());
                        Gtk::TreeModel::Row prow(*m_nameicaomodelstore->append());
                        prow[m_nameicaomodelcolumns.m_col_name] = "Navaids";
                        prow[m_nameicaomodelcolumns.m_col_selectable] = false;
                        for (Engine::NavaidResult::elementvector_t::const_iterator ei(ev.begin()), ee(ev.end()); ei != ee; ei++) {
                                Gtk::TreeModel::Row row(*m_nameicaomodelstore->append(prow.children()));
                                row[m_nameicaomodelcolumns.m_col_selectable] = true;
                                row[m_nameicaomodelcolumns.m_col_name] = ei->get_name();
                                row[m_nameicaomodelcolumns.m_col_icao] = ei->get_icao();
                                row[m_nameicaomodelcolumns.m_col_dist] = Conversions::dist_str(m_coord.spheric_distance_nmi(ei->get_coord()));
                        }
                        m_nameicaomodelstore->set_sort_column(m_nameicaomodelcolumns.m_col_dist, Gtk::SORT_ASCENDING);
                }
                m_navaidquery = Glib::RefPtr<Engine::NavaidResult>();
        }
        if (m_waypointquery && m_waypointquery->is_done()) {
                if (!m_waypointquery->is_error()) {
                        Engine::WaypointResult::elementvector_t& ev(m_waypointquery->get_result());
                        Gtk::TreeModel::Row prow(*m_nameicaomodelstore->append());
                        prow[m_nameicaomodelcolumns.m_col_name] = "Waypoints";
                        prow[m_nameicaomodelcolumns.m_col_selectable] = false;
                        for (Engine::WaypointResult::elementvector_t::const_iterator ei(ev.begin()), ee(ev.end()); ei != ee; ei++) {
                                Gtk::TreeModel::Row row(*m_nameicaomodelstore->append(prow.children()));
                                row[m_nameicaomodelcolumns.m_col_selectable] = true;
                                row[m_nameicaomodelcolumns.m_col_name] = ei->get_name();
                                row[m_nameicaomodelcolumns.m_col_icao] = ei->get_icao();
                                row[m_nameicaomodelcolumns.m_col_dist] = Conversions::dist_str(m_coord.spheric_distance_nmi(ei->get_coord()));
                        }
                        m_nameicaomodelstore->set_sort_column(m_nameicaomodelcolumns.m_col_dist, Gtk::SORT_ASCENDING);
                }
                m_waypointquery = Glib::RefPtr<Engine::WaypointResult>();
        }
        if (m_mapelementquery && m_mapelementquery->is_done()) {
                if (!m_mapelementquery->is_error()) {
                        Engine::MapelementResult::elementvector_t& ev(m_mapelementquery->get_result());
                        Gtk::TreeModel::Row prow(*m_nameicaomodelstore->append());
                        prow[m_nameicaomodelcolumns.m_col_name] = "Map Elements";
                        prow[m_nameicaomodelcolumns.m_col_selectable] = false;
                        unsigned int nr(0);
                        for (Engine::MapelementResult::elementvector_t::const_iterator ei(ev.begin()), ee(ev.end()); ei != ee; ei++) {
                                if (ei->get_name().empty())
                                        continue;
                                Gtk::TreeModel::Row row(*m_nameicaomodelstore->append(prow.children()));
                                row[m_nameicaomodelcolumns.m_col_selectable] = true;
                                row[m_nameicaomodelcolumns.m_col_name] = ei->get_name();
                                row[m_nameicaomodelcolumns.m_col_icao] = "";
                                row[m_nameicaomodelcolumns.m_col_dist] = Conversions::dist_str(m_coord.spheric_distance_nmi(ei->get_coord()));
                                nr++;
                                if (nr >= max_number)
                                        break;
                        }
                        m_nameicaomodelstore->set_sort_column(m_nameicaomodelcolumns.m_col_dist, Gtk::SORT_ASCENDING);
                }
                m_mapelementquery = Glib::RefPtr<Engine::MapelementResult>();
        }
}

void NewWaypointWindow::async_cancel(void)
{
        Engine::AirportResult::cancel(m_airportquery);
        Engine::NavaidResult::cancel(m_navaidquery);
        Engine::WaypointResult::cancel(m_waypointquery);
        Engine::MapelementResult::cancel(m_mapelementquery);
}

void NewWaypointWindow::nameicao_changed(void)
{
        Gtk::TreeModel::iterator iter(m_newwaypointnamebox->get_active());
        if (!iter)
                return;
        Gtk::TreeModel::Row row(*iter);
        if (!row[m_nameicaomodelcolumns.m_col_selectable])
                return;
        m_newwaypointname->set_text(row[m_nameicaomodelcolumns.m_col_name]);
        m_newwaypointicao->set_text(row[m_nameicaomodelcolumns.m_col_icao]);
}

void NewWaypointWindow::time_changed(Gtk::Entry * entry)
{
        entry->set_text(Conversions::time_str("%Y-%m-%d %H:%M:%S", Conversions::time_parse(entry->get_text())));
}

void NewWaypointWindow::lat_changed(Gtk::Entry * entry)
{
        Point coord;
        coord.set_lat_str(entry->get_text());
        entry->set_text(coord.get_lat_str());
}

void NewWaypointWindow::lon_changed(Gtk::Entry * entry)
{
        Point coord;
        coord.set_lon_str(entry->get_text());
        entry->set_text(coord.get_lon_str());
}


