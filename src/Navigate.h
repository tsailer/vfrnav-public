//
// C++ Interface: Navigate
//
// Description: Navigation Mode Windows
//
//
// Author: Thomas Sailer <t.sailer@alumni.ethz.ch>, (C) 2007, 2009, 2012
//
// Copyright: See COPYING file that comes with this distribution
//
//

#ifndef NAVIGATE_H
#define NAVIGATE_H

#include "sysdeps.h"

#include <gtkmm.h>
#include <sigc++/sigc++.h>

#include "engine.h"
#include "fplan.h"
#include "geom.h"
#include "maps.h"
#include "prefs.h"

#ifdef HAVE_HILDONMM
#include <hildonmm.h>
#endif
#ifdef HAVE_GPS_H
#include <gps.h>
#endif

class Navigate : public sigc::trackable {
        private:

#ifdef HAVE_HILDONMM
                typedef Hildon::Window toplevel_window_t;
#else
                typedef Gtk::Window toplevel_window_t;
#endif

                class NavInfoWindow : public toplevel_window_t {
                        public:
                                explicit NavInfoWindow(BaseObjectType *castitem, const Glib::RefPtr<Builder>& refxml);
                                virtual ~NavInfoWindow();
                };

                class NavPageWindow : public toplevel_window_t {
                        public:
                                explicit NavPageWindow(BaseObjectType *castitem, const Glib::RefPtr<Builder>& refxml);
                                virtual ~NavPageWindow();

                                void toggle_fullscreen(void);

                                sigc::signal<bool>& signal_zoomin(void) { return m_zoomin; }
                                sigc::signal<bool>& signal_zoomout(void) { return m_zoomout; }

                                bool zoom_in(void) { return m_zoomin(); }
                                bool zoom_out(void) { return m_zoomout(); }

                        private:
                                bool m_fullscreen;
                                sigc::signal<bool> m_zoomin;
                                sigc::signal<bool> m_zoomout;

                                bool on_my_window_state_event(GdkEventWindowState *);
                };

                class NavPOIWindow : public NavPageWindow {
                        protected:
                                class POI {
                                        public:
                                                POI(const Glib::ustring& name = "", const Point& coord = Point()) : m_name(name), m_coord(coord), m_dist(0), m_angle(0) {}
                                                void update_dist(const Point& coord) { m_dist = m_coord.simple_distance_nmi(coord); }
                                                void update_angle(const Point& coord) { m_angle = Point::round<uint16_t,float>(m_coord.simple_true_course(coord) * ((1 << 16) / 360.0f)); }
                                                void update(const Point& coord) { update_dist(coord); update_angle(coord); }
                                                const Glib::ustring& get_name(void) const { return m_name; }
                                                const Point& get_coord(void) const { return m_coord; }
                                                float get_dist(void) const { return m_dist; }
                                                int get_angle(void) const { uint32_t a = m_angle * 360U; return a >> 16; }
                                                Glib::ustring get_angletext(void) const;
                                                bool operator<(const POI& b) const { return (m_dist < b.m_dist); }

                                        protected:
                                                Glib::ustring m_name;
                                                Point m_coord;
                                                float m_dist;
                                                uint16_t m_angle;
                                };

                        public:
                                explicit NavPOIWindow(BaseObjectType *castitem, const Glib::RefPtr<Builder>& refxml);
                                virtual ~NavPOIWindow();

                                void set_engine(Engine& eng) { m_engine = &eng; }
                                void set_coord(const Point& coord);

                        protected:
                                class POIModelColumns : public Gtk::TreeModelColumnRecord {
                                        public:
                                                POIModelColumns(void);
                                                Gtk::TreeModelColumn<Glib::ustring> m_col_name;
                                                Gtk::TreeModelColumn<Glib::ustring> m_col_anglename;
                                                Gtk::TreeModelColumn<Glib::ustring> m_col_angle;
                                                Gtk::TreeModelColumn<Glib::ustring> m_col_dist;
                                };

                                POIModelColumns m_poimodelcolumns;
                                Engine *m_engine;
                                Point m_coord;
                                std::vector<POI> m_poiairports;
                                std::vector<POI> m_poimapelements;
                                std::vector<POI> m_poinavaids;
                                std::vector<POI> m_poiwaypoints;
                                Glib::Dispatcher m_dispatch;
                                Rect m_queryrect;
                                Glib::RefPtr<Engine::AirportResult> m_airportquery;
                                Glib::RefPtr<Engine::NavaidResult> m_navaidquery;
                                Glib::RefPtr<Engine::WaypointResult> m_waypointquery;
                                Glib::RefPtr<Engine::MapelementResult> m_mapelementquery;
                                Gtk::TreeView *m_navpoiairports;
                                Gtk::TreeView *m_navpoinavaids;
                                Gtk::TreeView *m_navpoimapelements;
                                Gtk::TreeView *m_navpoiwaypoints;
                                Glib::RefPtr<Gtk::ListStore> m_airportsstore;
                                Glib::RefPtr<Gtk::ListStore> m_navaidsstore;
                                Glib::RefPtr<Gtk::ListStore> m_waypointsstore;
                                Glib::RefPtr<Gtk::ListStore> m_mapelementsstore;

                                void async_cancel(void);
                                void async_done_dispatch(void);
                                void async_done(void);
                                void update_display(void);
                };

                class GPSComm {
                        public:
                                typedef enum {
                                        fixtype_noconn,
                                        fixtype_nofix,
                                        fixtype_2d,
                                        fixtype_3d
                                } fixtype_t;

                                typedef enum {
                                        fixstatus_nofix,
                                        fixstatus_fix,
                                        fixstatus_dgpsfix
                                } fixstatus_t;

                                class SV {
                                        public:
                                                SV(int prn = 0, int az = 0, int elev = 0, int ss = 0, bool used = false) : m_prn(prn), m_az(az), m_elev(elev), m_ss(ss), m_used(used) {}
                                                int get_prn(void) const { return m_prn; }
                                                int get_azimuth(void) const { return m_az; }
                                                int get_elevation(void) const { return m_elev; }
                                                int get_signalstrength(void) const { return m_ss; }
                                                bool is_used(void) const { return m_used; }
                                                bool operator==(const SV& sv) const;
                                                bool operator!=(const SV& sv) const { return !operator==(sv); }

                                        private:
                                                int m_prn, m_az, m_elev, m_ss;
                                                bool m_used;
                                };

                                class SVVector : public std::vector<SV> {
                                        public:
                                                bool operator==(const SVVector& v) const;
                                                bool operator!=(const SVVector& v) const { return !operator==(v); }
                                };

                                GPSComm(void);
                                virtual ~GPSComm();
                                virtual fixtype_t get_fixtype(void) const;
                                virtual fixstatus_t get_fixstatus(void) const;
                                virtual double get_fixtime(void) const;
                                virtual Point get_coord(void) const;
                                virtual float get_coord_uncertainty(void) const;
                                virtual bool is_coord_valid(void) const;
                                virtual float get_altitude(void) const;
                                virtual float get_altitude_uncertainty(void) const;
                                virtual bool is_altitude_valid(void) const;
                                virtual float get_track(void) const;
                                virtual float get_track_uncertainty(void) const;
                                virtual float get_velocity(void) const;
                                virtual float get_velocity_uncertainty(void) const;
                                virtual float get_verticalvelocity(void) const;
                                virtual float get_verticalvelocity_uncertainty(void) const;
                                virtual float get_pdop(void) const;
                                virtual float get_hdop(void) const;
                                virtual float get_vdop(void) const;
                                virtual float get_tdop(void) const;
                                virtual float get_gdop(void) const;
                                virtual unsigned int get_nrsv(void) const;
                                virtual SV get_sv(unsigned int idx) const;
                                virtual SVVector get_svvector(void) const;
                                sigc::connection connect(const sigc::slot<void>& slot) { return m_update.connect(slot); }

                        protected:
                                sigc::signal<void> m_update;
                };

                class GPSCommGPSD;

                class GPSAzimuthElevation : public Gtk::DrawingArea {
                        public:
                                explicit GPSAzimuthElevation(BaseObjectType *castitem, const Glib::RefPtr<Builder>& refxml);
                                virtual ~GPSAzimuthElevation();

                                void update(const GPSComm::SVVector& svs);
                                static void svcolor(const Cairo::RefPtr<Cairo::Context>& cr, const GPSComm::SV& sv);

#ifdef HAVE_GTKMM2
				inline bool get_is_drawable(void) const { return is_drawable(); }
#endif

                        private:
				virtual bool on_draw(const Cairo::RefPtr<Cairo::Context>& cr);
#ifdef HAVE_GTKMM2
                                virtual bool on_expose_event(GdkEventExpose *event);
                                virtual void on_size_request(Gtk::Requisition* requisition);
#endif
#ifdef HAVE_GTKMM3
				virtual Gtk::SizeRequestMode get_request_mode_vfunc() const;
				virtual void get_preferred_width_vfunc(int& minimum_width, int& natural_width) const;
				virtual void get_preferred_height_for_width_vfunc(int width, int& minimum_height, int& natural_height) const;
				virtual void get_preferred_height_vfunc(int& minimum_height, int& natural_height) const;
				virtual void get_preferred_width_for_height_vfunc(int height, int& minimum_width, int& natural_width) const;
#endif

                                GPSComm::SVVector m_sv;
                };

                class GPSSignalStrength : public Gtk::DrawingArea {
                        public:
                                explicit GPSSignalStrength(BaseObjectType *castitem, const Glib::RefPtr<Builder>& refxml);
                                virtual ~GPSSignalStrength();

                                void update(const GPSComm::SVVector& svs);

#ifdef HAVE_GTKMM2
				inline bool get_is_drawable(void) const { return is_drawable(); }
#endif

                        private:
				virtual bool on_draw(const Cairo::RefPtr<Cairo::Context>& cr);
#ifdef HAVE_GTKMM2
                                virtual bool on_expose_event(GdkEventExpose *event);
				virtual void on_size_request(Gtk::Requisition* requisition);
#endif
#ifdef HAVE_GTKMM3
				virtual Gtk::SizeRequestMode get_request_mode_vfunc() const;
				virtual void get_preferred_width_vfunc(int& minimum_width, int& natural_width) const;
				virtual void get_preferred_height_for_width_vfunc(int width, int& minimum_height, int& natural_height) const;
				virtual void get_preferred_height_vfunc(int& minimum_height, int& natural_height) const;
				virtual void get_preferred_width_for_height_vfunc(int height, int& minimum_width, int& natural_width) const;
#endif

                                GPSComm::SVVector m_sv;
                };

                class NavVectorMapArea : public VectorMapArea {
                        public:
                                explicit NavVectorMapArea(BaseObjectType *castitem, const Glib::RefPtr<Builder>& refxml);
                                virtual ~NavVectorMapArea();

                                void set_cursor(const Point& pt);
                                void set_cursor(const Point& pt, float track_deg, bool trackvalid = true);

                        private:
                                float m_cursortrack;
                                bool m_cursortrackvalid;
                };

                class NavWaypointModelColumns : public Gtk::TreeModelColumnRecord {
                        public:
                                NavWaypointModelColumns(void);
                                Gtk::TreeModelColumn<Glib::ustring> m_col_icao;
                                Gtk::TreeModelColumn<Glib::ustring> m_col_name;
                                Gtk::TreeModelColumn<Glib::ustring> m_col_freq;
                                Gtk::TreeModelColumn<Glib::ustring> m_col_time;
                                Gtk::TreeModelColumn<int> m_col_time_weight;
                                Gtk::TreeModelColumn<Gdk::Color> m_col_time_color;
                                Gtk::TreeModelColumn<Pango::Style> m_col_time_style;
                                Gtk::TreeModelColumn<Glib::ustring> m_col_lon;
                                Gtk::TreeModelColumn<Glib::ustring> m_col_lat;
                                Gtk::TreeModelColumn<Glib::ustring> m_col_alt;
                                Gtk::TreeModelColumn<Glib::ustring> m_col_truealt;
                                Gtk::TreeModelColumn<Glib::ustring> m_col_hdg;
                                Gtk::TreeModelColumn<Glib::ustring> m_col_tt;
                                Gtk::TreeModelColumn<Glib::ustring> m_col_var;
                                Gtk::TreeModelColumn<Glib::ustring> m_col_wca;
                                Gtk::TreeModelColumn<Glib::ustring> m_col_dist;
                                Gtk::TreeModelColumn<Glib::ustring> m_col_vspeed;
                                Gtk::TreeModelColumn<unsigned int> m_col_nr;
                };

                class CDI : public Gtk::DrawingArea {
                        public:
                                explicit CDI(BaseObjectType *castitem, const Glib::RefPtr<Builder>& refxml);
                                virtual ~CDI();

                                void update(float crosstrack, int16_t altdiff);

                        private:
#ifdef HAVE_GTKMM2
                                virtual bool on_expose_event(GdkEventExpose *event);
#endif
				virtual bool on_draw(const Cairo::RefPtr<Cairo::Context>& cr);

                                float m_crosstrack;
                                int16_t m_altdiff;
                };

                class FindWindow : public toplevel_window_t {
                        public:
                                static const unsigned int line_limit = 100;
                                static constexpr float distance_limit = 100;

                                explicit FindWindow(BaseObjectType *castitem, const Glib::RefPtr<Builder>& refxml);
                                virtual ~FindWindow();
                                void set_engine(Engine& eng);
                                virtual void set_center(const Point& pt);
                                template <typename T> void set_objects(const T& v1, const T& v2 = T());
                                void clear_objects(void);

                                typedef sigc::signal<void, const std::vector<FPlanWaypoint>&> signal_setwaypoints_t;
                                signal_setwaypoints_t signal_setwaypoints(void) { return m_signal_setwaypoints; }

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
                                                Gtk::TreeModelColumn<Glib::ustring> m_col_comment;
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
                                signal_setwaypoints_t m_signal_setwaypoints;
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

                class ConfirmUpdateWindow : public toplevel_window_t {
                        public:
                                explicit ConfirmUpdateWindow(BaseObjectType *castitem, const Glib::RefPtr<Builder>& refxml);
                                virtual ~ConfirmUpdateWindow();

                                typedef sigc::signal<void,bool,uint16_t,bool,const std::vector<FPlanWaypoint>&> signal_update_t;
                                signal_update_t signal_update(void) { return m_signal_update; }

                                void set_engine(Engine& eng);
                                void set_route(FPlanRoute& rt) { m_route = &rt; }
                                void update_route(void) { on_my_show(); }

                                void set_enableduplicate(bool dupl = true);
                                bool get_enableduplicate(void) const;
                                void set_wptnr(uint16_t nr) { m_wptnr = nr; }
                                uint16_t get_wptnr(void) const { return m_wptnr; }
                                void set_wptdelete(bool del) { m_wptdelete = del; }
                                bool get_wptdelete(void) const { return m_wptdelete; }
                                void set_newwpt(const FPlanWaypoint& wpt) { m_newwpts.clear(); m_newwpts.push_back(wpt); }
                                void set_newwpts(const std::vector<FPlanWaypoint>& wpts) { m_newwpts = wpts; }
                                const std::vector<FPlanWaypoint>& get_newwpts(void) const { return m_newwpts; }

                        private:
                                void on_my_hide(void);
                                void on_my_show(void);
                                bool on_my_delete_event(GdkEventAny* event);

                        protected:
                                class WaypointListModelColumns : public Gtk::TreeModelColumnRecord {
                                        public:
                                                WaypointListModelColumns(void);
                                                Gtk::TreeModelColumn<Glib::ustring> m_col_icao;
                                                Gtk::TreeModelColumn<Glib::ustring> m_col_name;
                                                Gtk::TreeModelColumn<Glib::ustring> m_col_freq;
                                                Gtk::TreeModelColumn<Glib::ustring> m_col_lon;
                                                Gtk::TreeModelColumn<Glib::ustring> m_col_lat;
                                                Gtk::TreeModelColumn<Glib::ustring> m_col_alt;
                                                Gtk::TreeModelColumn<Glib::ustring> m_col_mt;
                                                Gtk::TreeModelColumn<Glib::ustring> m_col_tt;
                                                Gtk::TreeModelColumn<Glib::ustring> m_col_dist;
                                                Gtk::TreeModelColumn<Glib::ustring> m_col_vertspeed;
                                                Gtk::TreeModelColumn<Glib::ustring> m_col_truealt;
                                                Gtk::TreeModelColumn<unsigned int> m_col_nr;
                                                Gtk::TreeModelColumn<int> m_col_weight;
                                                Gtk::TreeModelColumn<Gdk::Color> m_col_color;
                                                Gtk::TreeModelColumn<Pango::Style> m_col_style;
                                };

                                void buttonok_clicked(void);
                                void buttonduplicate_clicked(void);
                                void buttoncancel_clicked(void);
                                void buttonup_clicked(void);
                                void buttondown_clicked(void);

                        private:
                                Gtk::Button *m_button_duplicate;
                                Gtk::Button *m_button_up;
                                Gtk::Button *m_button_down;
                                WaypointListModelColumns m_waypointlistcolumns;
                                Glib::RefPtr<Gtk::ListStore> m_waypointliststore;
                                signal_update_t m_signal_update;
                                Engine *m_engine;
                                FPlanRoute *m_route;
                                Gtk::TreeView *m_confirmupdatewaypointlist;
                                uint16_t m_wptnr;
                                bool m_wptdelete;
                                std::vector<FPlanWaypoint> m_newwpts;
                };

        public:
                Navigate(Engine& eng, Glib::RefPtr<PrefsWindow>& prefswindow);
                ~Navigate();

                typedef sigc::signal<void> signal_endnav_t;
                signal_endnav_t signal_endnav(void) { return m_signal_endnav; }

                typedef sigc::signal<void,FPlanRoute::id_t> signal_updateroute_t;
                signal_updateroute_t signal_updateroute(void) { return m_signal_updateroute; }

                void start_nav(FPlanRoute::id_t id);
                void update_route(FPlanRoute::id_t id);

        private:
                void end_nav(void);
                void menu_cut(Gtk::Window *wnd);
                void menu_copy(Gtk::Window *wnd);
                void menu_paste(Gtk::Window *wnd);
                void menu_delete(Gtk::Window *wnd);
                void menu_nextwpt(void);
                void menu_prevwpt(void);
                void find_setwpts(const std::vector<FPlanWaypoint>& wpts);
                void menu_insertbyname(void);
                void menu_insertnearest(void);
                void menu_insertcurrentpos(void);
                void menu_deletenext(void);
                void update_save_route(bool duplicate, uint16_t wptnr, bool wptdelete, const std::vector<FPlanWaypoint>& wpts);
                void menu_restartnav(void);
                void menu_viewwptinfo(void);
                void menu_viewvmap(void);
                void menu_viewnearestpoi(void);
                void menu_viewgpsinfo(void);
                void menu_viewprefs(void);
                void menu_viewcurrentleg(void);
                void menu_viewterrain(void);
                void menu_viewairportdiagram(void);
                void menu_createwaypoint(void);
                void menu_about(void);
                void bind_menubar(const Glib::ustring& wndname, Gtk::Window *wnd);
                void open_gps(void);
                void close_gps(void);
                void update_gps(void);
                void update_current_route(void);
                void update_time_estimates(void);
                void newleg(void);
                void updateleg(void);
                void updateeditmenu(void);
                void navvmapmenuview_zoomin(void);
                void navvmapmenuview_zoomout(void);
                void navterrainmenuview_zoomin(void);
                void navterrainmenuview_zoomout(void);
                void navairportdiagrammenuview_zoomin(void);
                void navairportdiagrammenuview_zoomout(void);
                void prefschange_mapflags(int f);
                bool timer_handler(void);
                void aboutdialog_response(int response);
                static bool window_delete_event(GdkEventAny* event, Gtk::Window *window);
                bool window_key_press_event(GdkEventKey* event, NavPageWindow *window);

        private:
                Engine& m_engine;
                FPlanRoute m_route;
                bool m_route_unmodified;
                Glib::RefPtr<PrefsWindow> m_prefswindow;
                FPlanLeg m_currentleg;
                FPlanLeg m_nextleg;
                FPlanLeg m_directdestleg;
                TracksDb::Track m_track;
                signal_endnav_t m_signal_endnav;
                signal_updateroute_t m_signal_updateroute;
                uint64_t m_lastgpstime;
                GPSComm *m_gps;
                Glib::RefPtr<Builder> m_refxml;
                union {
                        struct {
                                NavPageWindow *m_navwaypointwindow;
                                NavPageWindow *m_navlegwindow;
                                NavPageWindow *m_navvmapwindow;
                                NavPageWindow *m_navterrainwindow;
                                NavPageWindow *m_navairportdiagramwindow;
                                NavPOIWindow *m_navpoiwindow;
                                NavPageWindow *m_gpsinfowindow;
                        };
                        NavPageWindow *m_pagewindows[7];
                };
                NavInfoWindow *m_navinfowindow;
                FindNameWindow *m_findnamewindow;
                FindNearestWindow *m_findnearestwindow;
                ConfirmUpdateWindow *m_confirmupdatewindow;
                typedef std::vector<Gtk::MenuItem *> menuitemvector_t;
                menuitemvector_t m_menu_prevwpt;
                menuitemvector_t m_menu_nextwpt;
                menuitemvector_t m_menu_insertbyname;
                menuitemvector_t m_menu_insertnearest;
                menuitemvector_t m_menu_insertcurrentpos;
                menuitemvector_t m_menu_deletenextwpt;
                Gtk::TreeView *m_navwaypointtreeview;
                Gtk::HPaned *m_gpsinfopane;
                GPSAzimuthElevation *m_gpsinfoazelev;
                GPSSignalStrength *m_gpsinfosigstrength;
                Gtk::Entry *m_gpsinfoalt;
                Gtk::Entry *m_gpsinfolon;
                Gtk::Entry *m_gpsinfolat;
                Gtk::Entry *m_gpsinfovgnd;
                Gtk::Entry *m_gpsinfohdop;
                Gtk::Entry *m_gpsinfovdop;
                Gtk::Entry *m_gpsinfodgps;
                Gtk::Entry *m_gpsinfogps;
                NavVectorMapArea *m_navvmaparea;
                Gtk::MenuItem *m_navvmapmenuzoomin;
                Gtk::MenuItem *m_navvmapmenuzoomout;
                Gtk::Entry *m_navvmapinfohdg;
                Gtk::Entry *m_navvmapinfodist;
                Gtk::Entry *m_navvmapinfovs;
                Gtk::Entry *m_navvmapinfoname;
                Gtk::Entry *m_navvmapinfotarghdg;
                Gtk::Entry *m_navvmapinfotargdist;
                Gtk::Entry *m_navvmapinfotargvs;
                Gtk::Entry *m_navvmapinfogps;
                Gtk::Entry *m_navvmapinfovgnd;
                Gtk::Entry *m_navlegfromname;
                Gtk::Entry *m_navlegfromalt;
                Gtk::Entry *m_navlegtoname;
                Gtk::Entry *m_navlegtoalt;
                Gtk::Entry *m_navlegcurrenttt;
                Gtk::Entry *m_navlegcurrentmt;
                Gtk::Entry *m_navlegcurrenthdg;
                Gtk::Entry *m_navlegcurrentdist;
                Gtk::Entry *m_navlegnexttt;
                Gtk::Entry *m_navlegnextmt;
                Gtk::Entry *m_navlegnexthdg;
                Gtk::Entry *m_navlegnextdist;
                Gtk::Entry *m_navlegmaplon;
                Gtk::Entry *m_navlegmaplat;
                Gtk::Entry *m_navlegalt;
                Gtk::Entry *m_navleggpshdg;
                Gtk::Entry *m_navlegvgnd;
                Gtk::Entry *m_navlegcrosstrack;
                Gtk::Entry *m_navlegcoursetodest;
                Gtk::Entry *m_navlegreferencealt;
                Gtk::Entry *m_navlegaltdiff;
                Gtk::Entry *m_navleghdgtodest;
                Gtk::Entry *m_navlegdisttodest;
                Gtk::ProgressBar *m_navlegprogress;
                CDI *m_navlegcdi;
                Gtk::Label *m_navlegmaplonlabel;
                Gtk::Label *m_navlegmaplatlabel;
                Gtk::Label *m_navlegaltlabel;
                NavVectorMapArea *m_navterrainarea;
                Gtk::MenuItem *m_navterrainmenuzoomin;
                Gtk::MenuItem *m_navterrainmenuzoomout;
                Gtk::Entry *m_navterraininfohdg;
                Gtk::Entry *m_navterraininfodist;
                Gtk::Entry *m_navterraininfovs;
                Gtk::Entry *m_navterraininfoname;
                Gtk::Entry *m_navterraininfotarghdg;
                Gtk::Entry *m_navterraininfotargdist;
                Gtk::Entry *m_navterraininfotargvs;
                Gtk::Entry *m_navterraininfogps;
                Gtk::Entry *m_navterraininfovgnd;
                NavVectorMapArea *m_navairportdiagramarea;
                Gtk::MenuItem *m_navairportdiagrammenuzoomin;
                Gtk::MenuItem *m_navairportdiagrammenuzoomout;
                Gtk::Entry *m_navairportdiagraminfohdg;
                Gtk::Entry *m_navairportdiagraminfodist;
                Gtk::Entry *m_navairportdiagraminfovs;
                Gtk::Entry *m_navairportdiagraminfoname;
                Gtk::Entry *m_navairportdiagraminfotarghdg;
                Gtk::Entry *m_navairportdiagraminfotargdist;
                Gtk::Entry *m_navairportdiagraminfotargvs;
                Gtk::Entry *m_navairportdiagraminfogps;
                Gtk::Entry *m_navairportdiagraminfovgnd;
                Gtk::Entry *m_navpoiinfohdg;
                Gtk::Entry *m_navpoiinfodist;
                Gtk::Entry *m_navpoiinfovs;
                Gtk::Entry *m_navpoiinfoname;
                Gtk::Entry *m_navpoiinfotarghdg;
                Gtk::Entry *m_navpoiinfotargdist;
                Gtk::Entry *m_navpoiinfotargvs;
                Gtk::Entry *m_navpoiinfogps;
                Gtk::Entry *m_navpoiinfovgnd;
                Gtk::AboutDialog *m_aboutdialog;
                NavWaypointModelColumns m_navwaypointlistcolumns;
                Glib::RefPtr<Gtk::ListStore> m_navwaypointliststore;
                sigc::connection m_updatetimer;
};

#endif /* NAVIGATE_H */
