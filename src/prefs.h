//
// C++ Interface: prefs
//
// Description: Preferences Database
//
//
// Author: Thomas Sailer <t.sailer@alumni.ethz.ch>, (C) 2007, 2012
//
// Copyright: See COPYING file that comes with this distribution
//
//

#ifndef PREFS_H
#define PREFS_H

#include "sysdeps.h"

#include <gtkmm.h>
#include <sigc++/sigc++.h>

#include "engine.h"
#include "fplan.h"
#include "geom.h"
#include "maps.h"

#ifdef HAVE_HILDONMM
#include <hildonmm.h>
#include <hildon-fmmm.h>
#endif
#ifdef HAVE_GPS_H
#include <gps.h>
#endif


class PrefsWindow : public
#ifdef HAVE_HILDONMM
                Hildon::Window
#else
                Gtk::Window
#endif
{
        public:
                explicit PrefsWindow(BaseObjectType *castitem, const Glib::RefPtr<Builder>& refxml);
                virtual ~PrefsWindow();

                static Glib::RefPtr<PrefsWindow> create(Preferences& prefs);

        private:
                void button_prefscancel(void);
                void button_prefsapply(void);
                void button_prefsok(void);
                void button_globaldbdirchange(void);
                void button_gpsportbluetooth(void);
                void load_prefs(void);
                void save_prefs(void);
                bool window_delete_event(GdkEventAny* event);
                void connect_engine(void);
                void change_vcruise(double v);
                void change_gps(bool v);
                void change_autowpt(bool v);
                void change_vblock(double v);
                void change_maxalt(int v);
                void change_mapflags(int v);
                void change_globaldbdir(Glib::ustring v);
                void change_gpshost(Glib::ustring v);
                void change_gpsport(Glib::ustring v);

                void change_vmapnavaidsall(void);
                void change_vmapnavaids(void);
                void change_vmapwaypointsall(void);
                void change_vmapwaypoints(void);
                void change_vmapairportsall(void);
                void change_vmapairports(void);
                void change_vmapairspacesall(void);
                void change_vmapairspaces(void);

        private:
                Preferences *m_prefs;
                Gtk::Entry *m_prefsvcruise;
                Gtk::CheckButton *m_prefsgps;
                Gtk::CheckButton *m_prefsautowpt;
                Gtk::Entry *m_prefsvblock;
                Gtk::Entry *m_prefsmaxalt;
                Gtk::Entry *m_prefsglobaldbdir;
                Gtk::Entry *m_prefsgpshost;
                Gtk::Entry *m_prefsgpsport;
                Gtk::CheckButton *m_prefsvmaptopo;
                Gtk::CheckButton *m_prefsvmapnorth;
                Gtk::CheckButton *m_prefsvmapterrain;
                Gtk::CheckButton *m_prefsvmapterrainnames;
                Gtk::CheckButton *m_prefsvmapterrainborders;
                Gtk::CheckButton *m_prefsvmapnavaidsall;
                Gtk::CheckButton *m_prefsvmapnavaidsvor;
                Gtk::CheckButton *m_prefsvmapnavaidsdme;
                Gtk::CheckButton *m_prefsvmapnavaidsndb;
                Gtk::CheckButton *m_prefsvmapwaypointsall;
                Gtk::CheckButton *m_prefsvmapwaypointslow;
                Gtk::CheckButton *m_prefsvmapwaypointshigh;
                Gtk::CheckButton *m_prefsvmapwaypointsrnav;
                Gtk::CheckButton *m_prefsvmapwaypointsterminal;
                Gtk::CheckButton *m_prefsvmapwaypointsvfr;
                Gtk::CheckButton *m_prefsvmapwaypointsuser;
                Gtk::CheckButton *m_prefsvmapairportsall;
                Gtk::CheckButton *m_prefsvmapairportsciv;
                Gtk::CheckButton *m_prefsvmapairportsmil;
                Gtk::CheckButton *m_prefsvmapairportsloccone;
                Gtk::CheckButton *m_prefsvmapairspaceall;
                Gtk::CheckButton *m_prefsvmapairspaceab;
                Gtk::CheckButton *m_prefsvmapairspacecd;
                Gtk::CheckButton *m_prefsvmapairspaceefg;
                Gtk::CheckButton *m_prefsvmapairspacespecialuse;
                Gtk::CheckButton *m_prefsvmapairspacefillgnd;
                Gtk::CheckButton *m_prefsvmapairwaylow;
                Gtk::CheckButton *m_prefsvmapairwayhigh;
                Gtk::CheckButton *m_prefsvmaproute;
                Gtk::CheckButton *m_prefsvmaproutelabels;
                Gtk::CheckButton *m_prefsvmaptrack;
                Gtk::CheckButton *m_prefsvmaptrackpoints;
};

class NewWaypointWindow : public
#ifdef HAVE_HILDONMM
                Hildon::Window
#else
                Gtk::Window
#endif
{
        public:
                explicit NewWaypointWindow(BaseObjectType *castitem, const Glib::RefPtr<Builder>& refxml);
                virtual ~NewWaypointWindow();

                static Glib::RefPtr<NewWaypointWindow> create(Engine& eng, const Point& coord = Point(), const Glib::ustring& name = "", const Glib::ustring& icao = "");
                static Glib::ustring make_sourceid(void);

        private:
                class UsageModelColumns : public Gtk::TreeModelColumnRecord {
                        public:
                                UsageModelColumns(void);
                                Gtk::TreeModelColumn<char> m_col_usage;
                                Gtk::TreeModelColumn<Glib::ustring> m_col_text;
                };

                class NameIcaoModelColumns : public Gtk::TreeModelColumnRecord {
                        public:
                                NameIcaoModelColumns(void);
                                Gtk::TreeModelColumn<bool> m_col_selectable;
                                Gtk::TreeModelColumn<Glib::ustring> m_col_icao;
                                Gtk::TreeModelColumn<Glib::ustring> m_col_name;
                                Gtk::TreeModelColumn<Glib::ustring> m_col_dist;
                };

                static constexpr float max_distance = 10.0;
                static const unsigned int max_number = 25;

                static void time_changed(Gtk::Entry *entry);
                static void lat_changed(Gtk::Entry *entry);
                static void lon_changed(Gtk::Entry *entry);

                bool window_delete_event(GdkEventAny* event);
                void button_cancel(void);
                void button_ok(void);
                void nameicao_changed(void);
                void async_dispatchdone(void);
                void async_done(void);
                void async_cancel(void);

        private:
                Engine *m_engine;
                bool m_referenced;
                Point m_coord;
                Gtk::Entry *m_newwaypointlon;
                Gtk::Entry *m_newwaypointlat;
                Gtk::ComboBox *m_newwaypointusage;
                Gtk::ComboBox *m_newwaypointnamebox;
                Gtk::Entry *m_newwaypointname;
                Gtk::Entry *m_newwaypointicao;
                Gtk::Entry *m_newwaypointmodtime;
                Gtk::Entry *m_newwaypointsourceid;
                Gtk::Button *m_newwaypointbcancel;
                Gtk::Button *m_newwaypointbok;
                UsageModelColumns m_usagemodelcolumns;
                Glib::RefPtr<Gtk::ListStore> m_usagemodelstore;
                NameIcaoModelColumns m_nameicaomodelcolumns;
                Glib::RefPtr<Gtk::TreeStore> m_nameicaomodelstore;
                Glib::Dispatcher m_querydispatch;
                Glib::RefPtr<Engine::AirportResult> m_airportquery;
                Glib::RefPtr<Engine::NavaidResult> m_navaidquery;
                Glib::RefPtr<Engine::WaypointResult> m_waypointquery;
                Glib::RefPtr<Engine::MapelementResult> m_mapelementquery;
};

#endif /* PREFS_H */
