//
// C++ Interface: RouteEditUi
//
// Description: Route Editing Windows
//
//
// Author: Thomas Sailer <t.sailer@alumni.ethz.ch>, (C) 2007, 2008, 2009, 2012, 2013
//
// Copyright: See COPYING file that comes with this distribution
//
//

#ifndef ROUTEEDITUI_H
#define ROUTEEDITUI_H

#include "sysdeps.h"

#include <gtkmm.h>
#include <sigc++/sigc++.h>

#ifdef HAVE_OSSO
#include <libosso.h>
#endif
#ifdef HAVE_HILDONMM
#include <hildonmm.h>
#endif

#include "fplan.h"
#include "engine.h"
#include "maps.h"
#include "prefs.h"
#include "nwxweather.h"

class RouteEditUi : public sigc::trackable {
        private:

#ifdef HAVE_HILDONMM
                typedef Hildon::Window toplevel_window_t;
#else
                typedef Gtk::Window toplevel_window_t;
#endif

                class FullscreenableWindow : public toplevel_window_t {
                        public:
                                explicit FullscreenableWindow(BaseObjectType *castitem, const Glib::RefPtr<Builder>& refxml);
                                virtual ~FullscreenableWindow();

                                void toggle_fullscreen(void);

                        private:
                                bool m_fullscreen;

                                bool on_my_window_state_event(GdkEventWindowState *);
                                bool on_my_key_press_event(GdkEventKey* event);
                };

                class RouteEditWindow : public FullscreenableWindow {
                        public:
                                explicit RouteEditWindow(BaseObjectType *castitem, const Glib::RefPtr<Builder>& refxml);
                                virtual ~RouteEditWindow();

                        private:
                                void menu_cut(void);
                                void menu_copy(void);
                                void menu_paste(void);
                                void menu_delete(void);
                                bool on_my_delete_event(GdkEventAny* event);
                };

                class FindWindow : public FullscreenableWindow {
                        public:
                                static const unsigned int line_limit = 100;
                                static constexpr float distance_limit = 100;

                                explicit FindWindow(BaseObjectType *castitem, const Glib::RefPtr<Builder>& refxml);
                                virtual ~FindWindow();
                                void set_engine(Engine& eng);
                                virtual void set_center(const Point& pt);
                                template <typename T> void set_objects(const T& v1, const T& v2 = T());
                                void clear_objects(void);

                                typedef sigc::signal<void, const FPlanWaypoint&> signal_setwaypoint_t;
                                signal_setwaypoint_t signal_setwaypoint(void) { return m_signal_setwaypoint; }

                                typedef sigc::signal<void> signal_insertwaypoint_t;
                                signal_insertwaypoint_t signal_insertwaypoint(void) { return m_signal_insertwaypoint; }

                        public:
                                void buttonok_clicked(void);
                                void buttoncancel_clicked(void);

                        private:
                                void on_my_hide(void);
                                bool on_my_delete_event(GdkEventAny* event);

                        protected:
                                void init_list(void);
                                int sort_dist(const Gtk::TreeModel::iterator& ia, const Gtk::TreeModel::iterator& ib);
                                void async_dispatchdone(void);
                                void async_done(void);
                                void async_cancel(void);

                        private:
                                class FindModelColumns : public Gtk::TreeModelColumnRecord {
                                        public:
						FindModelColumns(void);
						Gtk::TreeModelColumn<Glib::ustring> m_col_display;
						Gtk::TreeModelColumn<Gdk::Color> m_col_display_color;
						Gtk::TreeModelColumn<Glib::ustring> m_col_dist;
						Gtk::TreeModelColumn<Glib::ustring> m_col_icao;
						Gtk::TreeModelColumn<Glib::ustring> m_col_name;
						Gtk::TreeModelColumn<Point> m_col_coord;
						Gtk::TreeModelColumn<int> m_col_alt;
						Gtk::TreeModelColumn<int> m_col_flags;
						Gtk::TreeModelColumn<unsigned int> m_col_freq;
						Gtk::TreeModelColumn<FPlanWaypoint::pathcode_t> m_col_pathcode;
						Gtk::TreeModelColumn<Glib::ustring> m_col_pathname;
						Gtk::TreeModelColumn<Glib::ustring> m_col_comment;
						Gtk::TreeModelColumn<FPlanWaypoint::type_t> m_col_wpttype;
						Gtk::TreeModelColumn<FPlanWaypoint::navaid_type_t> m_col_wptnavaid;
						Gtk::TreeModelColumn<unsigned int> m_col_type;
                                };

                                template <typename T> class FindObjectInfo {
                                        public:
                                                static const unsigned int parent_row;
                                };

                                void set_row_display(Gtk::TreeModel::Row& row, const Glib::ustring& comp1, const Glib::ustring& comp2 = Glib::ustring());
                                void set_row(Gtk::TreeModel::Row& row, const FPlanWaypoint& nn, bool dispreverse = false);
                                void set_row(Gtk::TreeModel::Row& row, const AirportsDb::Airport& nn, bool dispreverse = false);
                                void set_row(Gtk::TreeModel::Row& row, const NavaidsDb::Navaid& nn, bool dispreverse = false);
                                void set_row(Gtk::TreeModel::Row& row, const WaypointsDb::Waypoint& nn, bool dispreverse = false);
                                void set_row(Gtk::TreeModel::Row& row, const AirwaysDb::Airway& nn, bool dispreverse = false);
                                void set_row(Gtk::TreeModel::Row& row, const AirspacesDb::Airspace& nn, bool dispreverse = false);
                                void set_row(Gtk::TreeModel::Row& row, const MapelementsDb::Mapelement& nn, bool dispreverse = false);

                                void find_selection_changed(void);
                                bool find_select_function(const Glib::RefPtr<Gtk::TreeModel>& model, const Gtk::TreeModel::Path& path, bool);

                        protected:
                                Glib::RefPtr<Builder> m_refxml;
                                Engine *m_engine;
                                Gtk::TreeView *m_treeview;
                                Gtk::Button *m_buttonok;
                                Point m_center;
                                Glib::Dispatcher m_querydispatch;
                                Glib::RefPtr<Engine::AirportResult> m_airportquery;
                                Glib::RefPtr<Engine::AirportResult> m_airportquery1;
                                Glib::RefPtr<Engine::AirspaceResult> m_airspacequery;
                                Glib::RefPtr<Engine::AirspaceResult> m_airspacequery1;
                                Glib::RefPtr<Engine::NavaidResult> m_navaidquery;
                                Glib::RefPtr<Engine::NavaidResult> m_navaidquery1;
                                Glib::RefPtr<Engine::WaypointResult> m_waypointquery;
                                Glib::RefPtr<Engine::AirwayResult> m_airwayquery;
                                Glib::RefPtr<Engine::MapelementResult> m_mapelementquery;

                        private:
                                FindModelColumns m_findcolumns;
                                Glib::RefPtr<Gtk::TreeStore> m_findstore;
                                signal_setwaypoint_t m_signal_setwaypoint;
                                signal_insertwaypoint_t m_signal_insertwaypoint;
                };

                class FindNameWindow : public FindWindow {
                        public:
                                explicit FindNameWindow(BaseObjectType *castitem, const Glib::RefPtr<Builder>& refxml);
                                ~FindNameWindow();
                                void set_center(const Point& pt);

                        protected:
                                void buttonclear_clicked(void);
                                void buttonfind_clicked(void);
                                void searchtype_changed(void);
                                void entry_changed(void);

                        protected:
                                Gtk::Entry *m_entry;
                                Gtk::ComboBox *m_searchtype;
                };

                class FindNearestWindow : public FindWindow {
                        public:
                                explicit FindNearestWindow(BaseObjectType *castitem, const Glib::RefPtr<Builder>& refxml);
                                ~FindNearestWindow();
                                void set_center(const Point& pt);

                        protected:
                                void buttonfind_clicked(void);

                        protected:
                                Gtk::SpinButton *m_distance;
                };

                class ConfirmDeleteWindow : public FullscreenableWindow {
                        public:
                                explicit ConfirmDeleteWindow(BaseObjectType *castitem, const Glib::RefPtr<Builder>& refxml);
                                virtual ~ConfirmDeleteWindow();

                                typedef sigc::signal<void> signal_delete_t;
                                signal_delete_t signal_delete(void) { return m_signal_delete; }

                                void set_route(FPlanRoute& rt) { m_route = &rt; }

                        private:
                                void on_my_hide(void);
                                void on_my_show(void);
                                bool on_my_delete_event(GdkEventAny* event);

                        protected:
                                void buttonok_clicked(void);
                                void buttoncancel_clicked(void);

                        private:
                                signal_delete_t m_signal_delete;
                                FPlanRoute *m_route;
                                Gtk::TextView *m_confirmdeletetextview;
                };

                class MapWindow : public FullscreenableWindow {
                        public:
                                explicit MapWindow(BaseObjectType *castitem, const Glib::RefPtr<Builder>& refxml);
                                virtual ~MapWindow();

                                void set_engine(Engine& eng);
                                void set_route(FPlanRoute& rt);
                                void redraw_route(void);
                                void set_coord(const Point& pt, int alt);
                                void hide(void);
                                void set_renderer(VectorMapArea::renderer_t render);

                                typedef sigc::signal<void,const Point&> signal_setcoord_t;
                                signal_setcoord_t signal_setcoord(void) { return m_signal_setcoord; }
                                typedef sigc::signal<void,const Point&,const Rect&> signal_info_t;
                                signal_info_t signal_info(void) { return m_signal_info; }

                        protected:
                                void buttonok_clicked(void);
                                void buttoninfo_clicked(void);
                                void buttoncancel_clicked(void);
                                void buttonzoomin_clicked(void);
                                void buttonzoomout_clicked(void);
                                void cursor_change(Point pt, VectorMapArea::cursor_validity_t valid);
                                void pointer_change(Point pt);
                                void menucenter_activate(void);
                                void menuclear_activate(void);
                                void set_menu_coord(const Point& pt);
                                void set_menu_coord_asyncdone(const Point& pt);
                                void set_menu_coord_asyncdispatchdone(void);
                                void set_menu_coord_asynccancel(void);
                                bool on_my_delete_event(GdkEventAny* event);
                                bool on_my_key_press_event(GdkEventKey* event);
                                void mapflags_change(int f);

                        private:
                                signal_setcoord_t m_signal_setcoord;
                                signal_info_t m_signal_info;
                                Engine *m_engine;
                                FPlanRoute *m_route;
                                VectorMapArea *m_drawingarea;
                                Point m_center;
                                Gtk::ToolButton *m_buttonok;
                                Gtk::ToolButton *m_buttoninfo;
                                Gtk::MenuItem *m_menucoord;
                                Glib::Dispatcher m_async_dispatch;
                                sigc::connection m_async_dispatch_conn;
                                Glib::RefPtr<Engine::ElevationResult> m_async_elevation;
                                sigc::connection m_prefs_mapflags;
                                VectorMapArea::renderer_t m_renderer;
                };

                class DistanceWindow : public FullscreenableWindow {
                        public:
                                explicit DistanceWindow(BaseObjectType *castitem, const Glib::RefPtr<Builder>& refxml);
                                virtual ~DistanceWindow();

                                void set_engine(Engine& eng);
                                void set_coord(const Point& pt, const Point& ptprev = Point(), bool prevena = false, const Point& ptnext = Point(), bool nextena = false);

                                typedef sigc::signal<void,const Point&,bool> signal_insertwpt_t;
                                signal_insertwpt_t signal_insertwpt(void) { return m_signal_insertwpt; }

                        protected:
                                void buttoncancel_clicked(void);
                                void buttonfrom_clicked(void);
                                void buttonto_clicked(void);
                                bool on_my_delete_event(GdkEventAny* event);

                        private:
                                signal_insertwpt_t m_signal_insertwpt;
                                Engine *m_engine;
                                Point m_point, m_prevpoint, m_nextpoint;
                                bool m_prevena, m_nextena;
                                Gtk::Button *m_buttoncancel;
                                Gtk::Button *m_buttonfrom;
                                Gtk::Button *m_buttonto;
                                Gtk::Entry *m_lon;
                                Gtk::Entry *m_lat;
                                Gtk::SpinButton *m_distancevalue;
                                Gtk::ComboBox *m_distanceunit;
                };

                class CourseDistanceWindow : public FullscreenableWindow {
                        public:
                                explicit CourseDistanceWindow(BaseObjectType *castitem, const Glib::RefPtr<Builder>& refxml);
                                virtual ~CourseDistanceWindow();

                                void set_engine(Engine& eng);
                                void set_coord(const Point& pt);

                                typedef sigc::signal<void,const Point&> signal_setcoord_t;
                                signal_setcoord_t signal_setcoord(void) { return m_signal_setcoord; }

                        protected:
                                void buttoncancel_clicked(void);
                                void buttonfrom_clicked(void);
                                void buttonto_clicked(void);
                                void setcoord(uint32_t angoffset = 0);
                                void setcoord_asyncdone(const Point& pt, uint32_t ang, float dist);
                                void setcoord_asyncdispatchdone(void);
                                void setcoord_asynccancel(void);
                                bool on_my_delete_event(GdkEventAny* event);

                        private:
                                signal_setcoord_t m_signal_setcoord;
                                Engine *m_engine;
                                Point m_point;
                                Gtk::Button *m_buttoncancel;
                                Gtk::Button *m_buttonfrom;
                                Gtk::Button *m_buttonto;
                                Gtk::Entry *m_lon;
                                Gtk::Entry *m_lat;
                                Gtk::SpinButton *m_angle;
                                Gtk::ComboBox *m_headingmode;
                                Gtk::SpinButton *m_distancevalue;
                                Gtk::ComboBox *m_distanceunit;
                                Glib::Dispatcher m_async_dispatch;
                                sigc::connection m_async_dispatch_conn;
                                Glib::RefPtr<Engine::ElevationResult> m_async_elevation;
                };

                class CoordMaidenheadWindow : public FullscreenableWindow {
                        public:
                                explicit CoordMaidenheadWindow(BaseObjectType *castitem, const Glib::RefPtr<Builder>& refxml);
                                virtual ~CoordMaidenheadWindow();

                                void set_engine(Engine& eng);
                                void set_coord(const Point& pt);

                                typedef sigc::signal<void,const Point&> signal_setcoord_t;
                                signal_setcoord_t signal_setcoord(void) { return m_signal_setcoord; }

                        protected:
                                void buttonok_clicked(void);
                                void buttoncancel_clicked(void);
                                bool on_my_delete_event(GdkEventAny* event);

                        private:
                                signal_setcoord_t m_signal_setcoord;
                                Engine *m_engine;
                                Point m_point;
                                Gtk::Button *m_buttoncancel;
                                Gtk::Button *m_buttonok;
                                Gtk::Entry *m_lon;
                                Gtk::Entry *m_lat;
                                Gtk::Entry *m_locator;
                };

                class CoordCH1903Window : public FullscreenableWindow {
                        public:
                                explicit CoordCH1903Window(BaseObjectType *castitem, const Glib::RefPtr<Builder>& refxml);
                                virtual ~CoordCH1903Window();

                                void set_engine(Engine& eng);
                                void set_coord(const Point& pt);

                                typedef sigc::signal<void,const Point&> signal_setcoord_t;
                                signal_setcoord_t signal_setcoord(void) { return m_signal_setcoord; }

                        protected:
                                void buttonok_clicked(void);
                                void buttoncancel_clicked(void);
                                bool on_my_delete_event(GdkEventAny* event);

                        private:
                                signal_setcoord_t m_signal_setcoord;
                                Engine *m_engine;
                                Point m_point;
                                Gtk::Button *m_buttoncancel;
                                Gtk::Button *m_buttonok;
                                Gtk::Entry *m_lon;
                                Gtk::Entry *m_lat;
                                Gtk::Entry *m_x;
                                Gtk::Entry *m_y;
                };

                class InformationWindow : public FullscreenableWindow {
                        public:
                                explicit InformationWindow(BaseObjectType *castitem, const Glib::RefPtr<Builder>& refxml);
                                virtual ~InformationWindow();

                                void set_text(const Glib::ustring& str);
                                void clear_text(void);
                                void append_text(const Glib::ustring& str);

                        protected:
                                void buttonclose_clicked(void);

                        private:
                                void on_my_hide(void);
                                bool on_my_delete_event(GdkEventAny* event);

                        private:
                                Gtk::TextView *m_textview;
                };

                class SunriseSunsetWindow : public FullscreenableWindow {
                        private:
                                class SunriseSunsetModelColumns : public Gtk::TreeModelColumnRecord {
                                        public:
                                                SunriseSunsetModelColumns(void);
                                                Gtk::TreeModelColumn<Glib::ustring> m_col_location;
                                                Gtk::TreeModelColumn<Glib::ustring> m_col_sunrise;
                                                Gtk::TreeModelColumn<Glib::ustring> m_col_sunset;
                                                Gtk::TreeModelColumn<Point> m_col_point;
                                };

                        public:
                                explicit SunriseSunsetWindow(BaseObjectType *castitem, const Glib::RefPtr<Builder>& refxml);
                                virtual ~SunriseSunsetWindow();

                                void set_route(FPlanRoute& rt) { m_route = &rt; }
                                void update_route(void);

#ifdef HAVE_GTKMM2
				inline bool get_visible(void) const { return is_visible(); }
#endif

                        private:
                                void on_my_hide(void);
                                void on_my_show(void);
                                bool on_my_delete_event(GdkEventAny* event);

                        protected:
                                void buttonclose_clicked(void);
                                void buttontoday_clicked(void);
                                void recompute_sunrisesunset(void);
                                void clear_list(void);
                                void fill_list(void);

                        private:
                                FPlanRoute *m_route;
                                Gtk::TreeView *m_treeview;
                                Gtk::Calendar *m_calendar;
                                SunriseSunsetModelColumns m_listcolumns;
                                Glib::RefPtr<Gtk::ListStore> m_liststore;
                };

		class IcaoFplImportWindow : public FullscreenableWindow {
                        public:
                                explicit IcaoFplImportWindow(BaseObjectType *castitem, const Glib::RefPtr<Builder>& refxml);
                                virtual ~IcaoFplImportWindow();

                                typedef sigc::signal<Glib::ustring,const Glib::ustring&> signal_fplimport_t;
                                signal_fplimport_t signal_fplimport(void) { return m_signal_fplimport; }

                        private:
                                void on_my_hide(void);
                                void on_my_show(void);
                                bool on_my_delete_event(GdkEventAny* event);

                        protected:
                                void buttonok_clicked(void);
                                void buttoncancel_clicked(void);

                        private:
				signal_fplimport_t m_signal_fplimport;
                                Gtk::TextView *m_icaofplimporttextview;
                                Gtk::TextView *m_icaofplimporterrmsgtextview;
				Gtk::Expander *m_icaofplimporterrmsgexpander;
                };

        public:
                RouteEditUi(Engine& eng, Glib::RefPtr<PrefsWindow>& prefswindow);
                ~RouteEditUi();

                RouteEditWindow *get_routeeditwindow(void) { return m_routeeditwindow; }
                const RouteEditWindow *get_routeeditwindow(void) const { return m_routeeditwindow; }

                typedef sigc::signal<void,FPlanRoute::id_t> signal_startnav_t;
                signal_startnav_t signal_startnav(void) { return m_signal_startnav; }

                typedef sigc::signal<void,FPlanRoute::id_t> signal_updateroute_t;
                signal_updateroute_t signal_updateroute(void) { return m_signal_updateroute; }

                void end_navigate(void);
                void update_route(FPlanRoute::id_t id);

                static int compare_dist(const Glib::ustring& sa, const Glib::ustring& sb);
                static int compare_dist(float da, float db);

        private:
                class RouteListModelColumns : public Gtk::TreeModelColumnRecord {
                        public:
                                RouteListModelColumns(void);
                                Gtk::TreeModelColumn<Glib::ustring> m_col_from;
                                Gtk::TreeModelColumn<Glib::ustring> m_col_to;
                                Gtk::TreeModelColumn<float> m_col_dist;
                                Gtk::TreeModelColumn<FPlanRoute::id_t> m_col_id;
                };

                class WaypointListModelColumns : public Gtk::TreeModelColumnRecord {
                        public:
                                WaypointListModelColumns(void);
                                Gtk::TreeModelColumn<Glib::ustring> m_col_icao;
                                Gtk::TreeModelColumn<Glib::ustring> m_col_name;
                                Gtk::TreeModelColumn<Glib::ustring> m_col_freq;
                                Gtk::TreeModelColumn<Glib::ustring> m_col_time;
                                Gtk::TreeModelColumn<Glib::ustring> m_col_lon;
                                Gtk::TreeModelColumn<Glib::ustring> m_col_lat;
                                Gtk::TreeModelColumn<Glib::ustring> m_col_alt;
                                Gtk::TreeModelColumn<Glib::ustring> m_col_mt;
                                Gtk::TreeModelColumn<Glib::ustring> m_col_tt;
                                Gtk::TreeModelColumn<Glib::ustring> m_col_dist;
                                Gtk::TreeModelColumn<Glib::ustring> m_col_vertspeed;
                                Gtk::TreeModelColumn<Glib::ustring> m_col_truealt;
				Gtk::TreeModelColumn<Glib::ustring> m_col_pathname;
                                Gtk::TreeModelColumn<Glib::ustring> m_col_wind;
                                Gtk::TreeModelColumn<Glib::ustring> m_col_qff;
                                Gtk::TreeModelColumn<Glib::ustring> m_col_oat;
                                Gtk::TreeModelColumn<unsigned int> m_col_nr;
                };

                Gtk::TreeStore::iterator routeliststore_insert(const FPlanRoute& route, const Gtk::TreeStore::iterator& iter);
                void routeliststore_update(const FPlanRoute& route, const Gtk::TreeStore::iterator& iter);
                void route_save(void);
                void route_delete(void);
                void route_load(FPlanRoute::id_t id);
                void route_update(void);
                void route_selection(void);
                void route_checklist(void);
                void waypointlist_update(void);
                void waypointlist_disable(void);
                void waypointlist_selection_update(void);

                int routelist_sort_from(const Gtk::TreeModel::iterator& ia, const Gtk::TreeModel::iterator& ib);
                int routelist_sort_to(const Gtk::TreeModel::iterator& ia, const Gtk::TreeModel::iterator& ib);
                int routelist_sort_dist(const Gtk::TreeModel::iterator& ia, const Gtk::TreeModel::iterator& ib);
                void routelist_selection_changed(void);
                bool routelist_select_function(const Glib::RefPtr<Gtk::TreeModel>& model, const Gtk::TreeModel::Path& path, bool);
                void waypointlist_selection_changed( void );
                bool waypointlist_select_function(const Glib::RefPtr<Gtk::TreeModel>& model, const Gtk::TreeModel::Path& path, bool);

                static void set_coord_widgets(Gtk::ComboBox *sign, Gtk::Entry *deg, Gtk::Entry *min, float degf);
                static float get_coord_widgets(const Gtk::ComboBox *sign, const Gtk::Entry *deg, const Gtk::Entry *min);
                static void set_alt_widgets(Gtk::Entry *altv, Gtk::ComboBox *altu, int32_t alt, uint16_t flags, unsigned int unit = 0);
                static int32_t get_alt_widgets(Gtk::Entry *altv, Gtk::ComboBox *altu, uint16_t& flags);
                static void set_freq_widgets(Gtk::Entry *freqv, Gtk::Label *frequ, uint32_t freq);
                static uint32_t get_freq_widgets(Gtk::Entry *freqv);

                void widget_changed(void);
                bool wptentry_focus_in_event(GdkEventFocus* event, Gtk::Entry *entry);
                bool wpticao_focus_out_event(GdkEventFocus* event);
                bool wptname_focus_out_event(GdkEventFocus* event);
                bool wptpathname_focus_out_event(GdkEventFocus* event);
		void wptpathcode_changed(void);
		void wptlonsign_changed(void);
                bool wptlon_focus_out_event(GdkEventFocus* event);
                void wptlatsign_changed(void);
                bool wptlat_focus_out_event(GdkEventFocus* event);
                bool wptalt_focus_out_event(GdkEventFocus* event);
                void wptaltunit_changed(void);
                bool wptfreq_focus_out_event(GdkEventFocus* event);
		void wptifr_toggled(void);
                bool wptwinddir_focus_out_event(GdkEventFocus* event);
                bool wptwindspeed_focus_out_event(GdkEventFocus* event);
                bool wptqff_focus_out_event(GdkEventFocus* event);
                bool wptoat_focus_out_event(GdkEventFocus* event);
                bool wptnote_focus_out_event(GdkEventFocus* event);
                bool routenote_focus_out_event(GdkEventFocus* event);
                void wptprev_clicked(void);
                void wptnext_clicked(void);
                void wptfind_clicked(void);
                void wptfindnearest_clicked(void);
                void wptmaps_clicked(void);
                void wptterrain_clicked(void);
                void wptairportdiagram_clicked(void);

                void wx_setwind_clicked(void);
                void wx_setisaoffs_clicked(void);
                void wx_setqff_clicked(void);
                void wx_getnwx_clicked(void);
                void wx_deptime_changed(void);
                void wx_cruisealt_changed(void);
                void wx_fetchwind_cb(bool fplmodif);

                void set_current_waypoint(const FPlanWaypoint& wpt);

                void distance_cell_func(Gtk::CellRenderer *cell, const Gtk::TreeModel::iterator& iter);

                void routeeditnotebook_switch_page(guint page_num);
                void menu_newroute(void);
                void menu_duplicateroute(void);
                void menu_deleteroute(void);
                void menu_reverseroute(void);
                void menu_insertwaypoint(void);
                void menu_swapwaypoint(void);
                void menu_deletewaypoint(void);
		void menu_importicaofplroute(void);
		void menu_exporticaofplroute(void);
                void menu_sunrisesunset(void);
                void menu_startnavigate(void);
                void menu_waypointdistance(void);
                void menu_waypointcoursedistance(void);
                void menu_waypointstraighten(void);
                void menu_waypointcoordch1903(void);
                void menu_waypointcoordmaidenhead(void);
                void menu_about(void);
                void confirm_delete_route(void);
                void map_set_coord(const Point& pt);
                void map_set_coord_async_dispatch_done(void);
                void map_set_coord_async_done(void);
                void map_set_coord_async_cancel(void);
                void distance_insert_coord(const Point& pt, bool before);
                void map_info_coord(const Point& pt, const Rect& bbox);
                void map_info_async_dispatch_done(void);
                void map_info_async_done(const Point& pt, const Rect& bbox);
                void map_info_async_cancel(void);
		Glib::ustring icaofpl_import(const Glib::ustring& txt);
                void aboutdialog_response(int response);

        private:
                static constexpr float airway_width = 10.0; // nautical miles

                Engine& m_engine;
                FPlanRoute m_route;
                Glib::RefPtr<PrefsWindow> m_prefswindow;
                Glib::RefPtr<Builder> m_refxml;
                signal_startnav_t m_signal_startnav;
                signal_updateroute_t m_signal_updateroute;
                RouteEditWindow *m_routeeditwindow;
		Gtk::Notebook *m_routeeditnotebook;
                Gtk::MenuItem *m_routemenu;
                Gtk::MenuItem *m_newroute;
                Gtk::MenuItem *m_duplicateroute;
                Gtk::MenuItem *m_deleteroute;
                Gtk::MenuItem *m_reverseroute;
                Gtk::MenuItem *m_importicaofplroute;
                Gtk::MenuItem *m_exporticaofplroute;
                Gtk::MenuItem *m_sunrisesunsetroute;
                Gtk::MenuItem *m_startnavigate;
                Gtk::MenuItem *m_waypointmenu;
                Gtk::MenuItem *m_insertwaypoint;
                Gtk::MenuItem *m_swapwaypoint;
                Gtk::MenuItem *m_deletewaypoint;
                Gtk::MenuItem *m_waypointdistance;
                Gtk::MenuItem *m_waypointcoursedistance;
                Gtk::MenuItem *m_waypointstraighten;
                Gtk::MenuItem *m_waypointcoordch1903;
                Gtk::MenuItem *m_waypointcoordmaidenhead;
                Gtk::MenuItem *m_preferencesitem;
                Gtk::MenuItem *m_aboutitem;
                Gtk::TreeView *m_routelist;
                Gtk::TreeView *m_waypointlist;
                Gtk::Entry *m_wpticao;
                Gtk::Entry *m_wptname;
                Gtk::ComboBox *m_wptpathcode;
                Gtk::Entry *m_wptpathname;
                Gtk::ComboBox *m_wptlonsign;
                Gtk::Entry *m_wptlondeg;
                Gtk::Entry *m_wptlonmin;
                Gtk::ComboBox *m_wptlatsign;
                Gtk::Entry *m_wptlatdeg;
                Gtk::Entry *m_wptlatmin;
                Gtk::Entry *m_wptalt;
                Gtk::ComboBox *m_wptaltunit;
                Gtk::Entry *m_wptfreq;
                Gtk::Label *m_wptfrequnit;
		Gtk::CheckButton *m_wptifr;
		Gtk::SpinButton *m_wptwinddir;
		Gtk::SpinButton *m_wptwindspeed;
 		Gtk::SpinButton *m_wptqff;
 		Gtk::SpinButton *m_wptoat;
                Gtk::TextView *m_wptnote;
                Gtk::ToolButton *m_wptprev;
                Gtk::ToolButton *m_wptnext;
                Gtk::ToolButton *m_wptfind;
                Gtk::ToolButton *m_wptfindnearest;
                Gtk::ToolButton *m_wptmaps;
                Gtk::ToolButton *m_wptterrain;
                Gtk::ToolButton *m_wptairportdiagram;
                Gtk::TextView *m_routenote;
		sigc::connection m_wptifrconn;
                bool m_widgetchange;
                bool m_editactive;
                Gtk::Calendar *m_wxoffblock;
                Gtk::SpinButton *m_wxoffblockh;
                Gtk::SpinButton *m_wxoffblockm;
                Gtk::SpinButton *m_wxoffblocks;
                Gtk::SpinButton *m_wxtaxitimes;
                Gtk::SpinButton *m_wxtaxitimem;
                Gtk::SpinButton *m_wxcruisealt;
                Gtk::SpinButton *m_wxwinddir;
                Gtk::SpinButton *m_wxwindspeed;
                Gtk::SpinButton *m_wxisaoffs;
                Gtk::SpinButton *m_wxqff;
                Gtk::Button *m_wxsetwind;
		Gtk::Button *m_wxsetisaoffs;
		Gtk::Button *m_wxsetqff;
                Gtk::Button *m_wxgetweather;
                Gtk::Entry *m_wxflighttime;
                FindNameWindow *m_findnamewindow;
                FindNearestWindow *m_findnearestwindow;
                ConfirmDeleteWindow *m_confirmdeletewindow;
                MapWindow *m_mapwindow;
                Gtk::AboutDialog *m_aboutdialog;
                DistanceWindow *m_distancewindow;
                CourseDistanceWindow *m_coursedistwindow;
                CoordMaidenheadWindow *m_maidenheadwindow;
                CoordCH1903Window *m_ch1903window;
                InformationWindow *m_infowindow;
                SunriseSunsetWindow *m_sunrisesunsetwindow;
		IcaoFplImportWindow *m_icaofplimportwindow;

                RouteListModelColumns m_routelistcolumns;
                Glib::RefPtr<Gtk::TreeStore> m_routeliststore;
                WaypointListModelColumns m_waypointlistcolumns;
                Glib::RefPtr<Gtk::ListStore> m_waypointliststore;
                Glib::RefPtr<Gtk::TextBuffer> m_wptnotebuffer;
                Glib::RefPtr<Gtk::TextBuffer> m_routenotebuffer;

                Glib::Dispatcher m_mapcoord_dispatch;
                sigc::connection m_mapcoord_dispatch_conn;
                Glib::RefPtr<Engine::ElevationResult> m_mapcoord_elevation;
                Glib::Dispatcher m_mapinfo_dispatch;
                sigc::connection m_mapinfo_dispatch_conn;
                Glib::RefPtr<Engine::ElevationResult> m_mapinfo_elevation;
                Glib::RefPtr<Engine::AirportResult> m_mapinfo_airportquery;
                Glib::RefPtr<Engine::AirspaceResult> m_mapinfo_airspacequery;
                Glib::RefPtr<Engine::NavaidResult> m_mapinfo_navaidquery;
                Glib::RefPtr<Engine::WaypointResult> m_mapinfo_waypointquery;
                Glib::RefPtr<Engine::MapelementResult> m_mapinfo_mapelementquery;
                Glib::RefPtr<Engine::AirwayResult> m_mapinfo_airwayquery;

		sigc::connection m_connwxoffblock;
                sigc::connection m_connwxoffblockh;
                sigc::connection m_connwxoffblockm;
                sigc::connection m_connwxoffblocks;
                sigc::connection m_connwxtaxitimem;
                sigc::connection m_connwxtaxitimes;
		sigc::connection m_connwxcruisealt;

		NWXWeather m_nwxweather;
};

#endif /* ROUTEEDITUI_H */
