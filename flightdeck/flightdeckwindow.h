//
// C++ Interface: FlightDeckWindow
//
// Description: Flight Deck Windows
//
//
// Author: Thomas Sailer <t.sailer@alumni.ethz.ch>, (C) 2011, 2012, 2013, 2014, 2015, 2017
//
// Copyright: See COPYING file that comes with this distribution
//
//

#ifndef FLIGHTDECKWINDOW_H
#define FLIGHTDECKWINDOW_H

#include "sysdeps.h"

#include <gtkmm.h>
#include <sigc++/sigc++.h>
#include <Eigen/Dense>
#include <gsl/gsl_vector.h>
#include <gsl/gsl_matrix.h>
#ifdef HAVE_EVINCE
#include <evince-document.h>
#include <evince-view.h>
#endif

#ifdef HAVE_OSSO
#include <libosso.h>
#endif
#ifdef HAVE_HILDONMM
#include <hildonmm.h>
#endif
#ifdef HAVE_JSONCPP
#include <json/json.h>
#endif

#include "RouteEditUi.h"
#include "fplan.h"
#include "engine.h"
#include "maps.h"
#include "sensors.h"
#include "nwxweather.h"
#include "adds.h"
#include "metgraph.h"
#include "cfmuautorouteproxy.hh"

#ifdef HAVE_HILDONMM
#define TOPLEVELWINDOW Hildon::Window
#else
#define TOPLEVELWINDOW Gtk::Window
#endif

class FullscreenableWindow : public TOPLEVELWINDOW {
public:
	explicit FullscreenableWindow(BaseObjectType *castitem, const Glib::RefPtr<Builder>& refxml);
	virtual ~FullscreenableWindow();

	void toggle_fullscreen(void);

protected:
	bool m_fullscreen;

	virtual bool on_window_state_event(GdkEventWindowState *);
	virtual bool on_key_press_event(GdkEventKey* event);
};

class FlightDeckWindow : public FullscreenableWindow {
public:
	explicit FlightDeckWindow(BaseObjectType *castitem, const Glib::RefPtr<Builder>& refxml);
	virtual ~FlightDeckWindow();
	void set_engine(Engine& eng);
	void set_sensors(Sensors& sensors);
	void directto_activated(void) { set_menu_id(0); }
	void fpldirpage_set_default_fpl(FPlanRoute::id_t id);

	class SplashWindow : public FullscreenableWindow {
	public:
		explicit SplashWindow(BaseObjectType *castitem, const Glib::RefPtr<Builder>& refxml);
		virtual ~SplashWindow();
		template<typename T> void set_configs(T b, T e);
		const std::string& get_result(void) const { return m_result; }

	protected:
		void button_clicked(bool ok);
		void button_about_clicked(void);
                void aboutdialog_response(int response);
		void selection_changed(void);

		class ConfigModelColumns : public Gtk::TreeModelColumnRecord {
		public:
			ConfigModelColumns(void);
			Gtk::TreeModelColumn<Glib::ustring> m_col_name;
			Gtk::TreeModelColumn<Glib::ustring> m_col_description;
			Gtk::TreeModelColumn<Glib::ustring> m_col_filename;
		};

		ConfigModelColumns m_cfgcolumns;
		Glib::RefPtr<Gtk::ListStore> m_cfgstore;
		Gtk::Button *m_buttonok;
		Gtk::Button *m_buttoncancel;
		Gtk::Button *m_buttonabout;
		Gtk::TreeView *m_treeviewcfg;
                Gtk::AboutDialog *m_aboutdialog;
		std::string m_result;
	};

protected:
	static const char cfggroup_mainwindow[];
	static const char cfgkey_mainwindow_fullscreen[];
	static const char cfgkey_mainwindow_onscreenkeyboard[];
	static const char cfgkey_mainwindow_fplid[];
	static const char cfgkey_mainwindow_docdirs[];
	static const char cfgkey_mainwindow_docalltoken[];
	static const char cfgkey_mainwindow_grib2mapscale[];
	static const char cfgkey_mainwindow_grib2mapflags[];

	void sensors_change(Sensors::change_t changemask);
	virtual bool on_delete_event(GdkEventAny* event);
	virtual void on_show(void);
	virtual void on_hide(void);
	bool screensaver_heartbeat(void);
	void disable_screensaver(void);
	void enable_screensaver(void);
	bool suspend_xscreensaver(bool ena);
	int suspend_dpms(bool ena);
	void menubutton_clicked(unsigned int nr);
	void set_mainnotebook_page(unsigned int pg);
	void set_menu_id(unsigned int id);
	void set_menu_button(unsigned int nr, const Glib::ustring& txt);
	virtual bool on_key_press_event(GdkEventKey *event);
	void sensorcfg_fini(void);
	void sensorcfg_init(Sensors::Sensor& sens, unsigned int pagenr);
	void sensorcfg_update(bool all = true);
	void sensorcfg_sensor_update(const Sensors::Sensor::ParamChanged& pc);
	bool sensorcfg_delayed_update(void);
	void sensorcfg_set_admin(bool admin);
	void alt_dialog_open(void);
	bool alt_indicator_button_press_event(GdkEventButton *event);
	void alt_settings_update(void);
	void hsi_dialog_open(void);
	bool hsi_indicator_button_press_event(GdkEventButton *event);
	void hsi_settings_update(void);
	void map_dialog_open(void);
	void map_settings_update(void);
	void maptile_dialog_open(void);
	void textentry_dialog_open(const Glib::ustring& label, const Glib::ustring& text, const sigc::slot<void,bool>& done);
	typedef enum {
		kbd_off,
		kbd_main,
		kbd_numeric,
		kbd_full,
		kbd_auto
	} kbd_t;
	bool onscreenkbd_focus_in_event(GdkEventFocus *event, Gtk::Widget *widget, kbd_t kbd);
	bool onscreenkbd_focus_out_event(GdkEventFocus *event);
	void fplatmosphere_dialog_open(void);
	int compare_dist(const Glib::ustring& sa, const Glib::ustring& sb);
	int compare_dist(double da, double db);
	int wptpage_sortdist(const Gtk::TreeModelColumn<Glib::ustring>& col, const Gtk::TreeModel::iterator& ia, const Gtk::TreeModel::iterator& ib);
	int wptpage_airport_sortcol(const Gtk::TreeModelColumn<Glib::ustring>& col, const Gtk::TreeModel::iterator& ia, const Gtk::TreeModel::iterator& ib);
	int wptpage_airport_sortdist(const Gtk::TreeModel::iterator& ia, const Gtk::TreeModel::iterator& ib);
	int wptpage_navaid_sortdist(const Gtk::TreeModel::iterator& ia, const Gtk::TreeModel::iterator& ib);
	int wptpage_waypoint_sortdist(const Gtk::TreeModel::iterator& ia, const Gtk::TreeModel::iterator& ib);
	int wptpage_airway_sortdist(const Gtk::TreeModel::iterator& ia, const Gtk::TreeModel::iterator& ib);
	int wptpage_airspace_sortdist(const Gtk::TreeModel::iterator& ia, const Gtk::TreeModel::iterator& ib);
	int wptpage_mapelement_sortdist(const Gtk::TreeModel::iterator& ia, const Gtk::TreeModel::iterator& ib);
	int wptpage_fplan_sortdist(const Gtk::TreeModel::iterator& ia, const Gtk::TreeModel::iterator& ib);
	void wptpage_clearall(void);
	void wptpage_cancel(void);
	void wptpage_asyncdone(void);
	void wptpage_querydone(void);
	void wptpage_setnotebook(int pg);
	void wptpage_select(void);
	void wptpage_select(const AirportsDb::Airport& e, unsigned int routeid, unsigned int routepointid);
	void wptpage_select(const NavaidsDb::Navaid& e);
	void wptpage_select(const WaypointsDb::Waypoint& e);
	void wptpage_select(const AirwaysDb::Airway& e);
	void wptpage_select(const AirspacesDb::Airspace& e);
	void wptpage_select(const MapelementsDb::Mapelement& e);
	void wptpage_select(const FPlanWaypoint& e);
	void wptpage_setresult(const Engine::AirportResult::elementvector_t& elements);
	void wptpage_setresult(const Engine::NavaidResult::elementvector_t& elements);
	void wptpage_setresult(const Engine::WaypointResult::elementvector_t& elements);
	void wptpage_setresult(const Engine::AirwayResult::elementvector_t& elements);
	void wptpage_setresult(const Engine::AirspaceResult::elementvector_t& elements);
	void wptpage_setresult(const Engine::MapelementResult::elementvector_t& elements);
	void wptpage_setresult(const std::vector<FPlanWaypoint>& elements);
	template <typename T> void wptpage_updatedist(Gtk::TreeModel::Row& row, const T& cols);
	template <typename T> void wptpage_updatedist(const T& cols, Gtk::TreeIter it, Gtk::TreeIter itend);
	template <typename T> void wptpage_updatedist(const T& cols);
	template <typename T> void wptpage_selection_changed(const T& cols);
	void wptpage_airport_selection_changed(void);
	void wptpage_navaid_selection_changed(void);
	void wptpage_waypoint_selection_changed(void);
	void wptpage_airway_selection_changed(void);
	void wptpage_airspace_selection_changed(void);
	void wptpage_mapelement_selection_changed(void);
	void wptpage_fplan_selection_changed(void);
	void wptpage_updatepos(const Point& pt);
	void wptpage_setpos(const Point& pt);
	void wptpage_updatemenu(void);
	void wptpage_find(const Glib::ustring& text);
	void wptpage_findnearest(void);
	void wptpage_findcoord(const Point& pt);
	void wptpage_find(void);
	void wptpage_textentry_done(bool ok);
	void wptpage_map_cursor_change(Point pt, VectorMapArea::cursor_validity_t valid);
	void wptpage_updateswitches(void);
	void wptpage_togglebuttonnearest(void);
	void wptpage_togglebuttonfind(void);
	void wptpage_buttoninsert(void);
	void wptpage_buttondirectto(void);
	void wptpage_togglebuttonbrg2(void);
	void wptpage_togglebuttonarptmap(void);
	static void wptpage_setwpt(FPlanWaypoint& wpt, const Cairo::RefPtr<AirportsDb::Airport> el);
	static void wptpage_setwpt(FPlanWaypoint& wpt, const AirwaysDb::Airway& el, bool end);
	bool wptpage_getcoord(Glib::ustring& icao, Glib::ustring& name, Point& coord, double& track, int& altitude);
	void fpldirpage_updatepos(void);
	void fpldirpage_updatemenu(void);
	int fpldirpage_sortfloatcol(const Gtk::TreeModelColumn<float>& col, const Gtk::TreeModel::iterator& ia, const Gtk::TreeModel::iterator& ib);
	int fpldirpage_sortstringcol(const Gtk::TreeModelColumn<Glib::ustring>& col, const Gtk::TreeModel::iterator& ia, const Gtk::TreeModel::iterator& ib);
	void fpldirpage_exportfileresponse(int response);
	void fpldirpage_importfileresponse(int response);
        void fpldirpage_importchildwatch(GPid pid, int child_status);

	static void fpldirpage_lengthcelldatafunc(const Gtk::TreeModelColumn<float>& col, Gtk::CellRenderer *render, const Gtk::TreeModel::iterator& it);
	void fpldirpage_updatefpl(const FPlanRoute& fpl, Gtk::TreeIter& it);
	Gtk::TreeIter fpldirpage_updatefpl(FPlanRoute::id_t id);
	Gtk::TreeIter fpldirpage_updatefpl(const FPlanRoute& fpl);
	void fpldirpage_deletefpl(FPlanRoute::id_t id);
	bool fpldb_select_function(const Glib::RefPtr<Gtk::TreeModel>& model, const Gtk::TreeModel::Path& path, bool currently_selected);
	void fpldb_selection_changed(void);
	void fpldirpage_buttonload(void);
	void fpldirpage_buttondirectto(void);
	void fpldirpage_buttonduplicate(void);
	void fpldirpage_buttondelete(void);
	void fpldirpage_buttonreverse(void);
	void fpldirpage_buttonnew(void);
	void fpldirpage_buttonimport(void);
	void fpldirpage_buttonexport(void);
	void fpldirpage_cfmbuttonok(void);
	void fpldirpage_cfmbuttoncancel(void);
	typedef enum {
		fpldbcfmmode_none,
		fpldbcfmmode_delete,
		fpldbcfmmode_reverse,
		fpldbcfmmode_duplicate
	} fpldbcfmmode_t;
	Gtk::TreeIter fpldirpage_cfmdialog(fpldbcfmmode_t mode);
	bool fpldirpage_cfmvisible(const Gtk::TreeModel::const_iterator& iter);
	void fplpage_updatemenu(void);
	void fplpage_updatefpl(void);
	void fpl_selection_changed(void);
	void fplpage_select(void);
	void fplpage_select(unsigned int nr);
	bool fplpage_focus_in_event(GdkEventFocus *event, Gtk::Widget *widget, kbd_t kbd);
	bool fplpage_focus_out_event(GdkEventFocus *event);
	typedef enum {
		fplchg_none,
		fplchg_icao,
		fplchg_name,
		fplchg_pathname,
		fplchg_coord,
		fplchg_freq,
		fplchg_alt,
		fplchg_winddir,
		fplchg_windspeed,
		fplchg_qff,
		fplchg_oat,
		fplchg_note,
		fplchg_fplnote
	} fplchg_t;
	void fplpage_change_event(fplchg_t chg);
	void fplpage_pathcode_changed(void);
	void fplpage_altunit_changed(void);
	void fplpage_levelup_clicked(void);
	void fplpage_leveldown_clicked(void);
	void fplpage_semicircular_clicked(void);
	void fplpage_ifr_changed(void);
	void fplpage_type_changed(void);
	int32_t fplpage_get_altwidgets(uint16_t& flags);
	void fplpage_edit_coord(void);
	void fplpage_insert_clicked(void);
	void fplpage_inserttxt_clicked(void);
	void fplpage_inserttxt(bool ok);
	void fplpage_duplicate_clicked(void);
	void fplpage_moveup_clicked(void);
	void fplpage_movedown_clicked(void);
	void fplpage_delete_clicked(void);
	void fplpage_straighten_clicked(void);
	void fplpage_directto_clicked(void);
	void fplpage_brg2_clicked(void);
	void fplpage_fetchprofilegraph_clicked(void);
	void fplpage_fetchprofilegraph_cb(const std::string& pix, const std::string& mimetype, time_t epoch, unsigned int level);
	void fplpage_softkeyenable(void);
	void fplgpsimppage_selection_changed(void);
	bool fplgpsimppage_loadfpl(void);
	bool fplgpsimppage_brg2(void);
	bool fplgpsimppage_directto(void);
	void fplgpsimppage_zoomin(void);
	void fplgpsimppage_zoomout(void);
	bool fplgpsimppage_convertfpl(const Sensors::Sensor::gpsflightplan_t& fpl, unsigned int curwpt = ~0U);
	Sensors::Sensor *fplgpsimppage_convertfpl(unsigned int devnr);
	bool fplgpsimppage_updatemenu(void);
#ifdef HAVE_EVINCE
	bool docdir_select_function(const Glib::RefPtr<Gtk::TreeModel>& model, const Gtk::TreeModel::Path& path, bool currently_selected);
	void docdir_selection_changed(void);
	void docdirpage_scandirs(void);
	void docdirpage_scandir(const Glib::ustring& dir, bool alltoken);
	bool docdirpage_verify(void);
	class Document;
	void docdirpage_addpage(Gtk::TreeIter iter, Document *doc);
	void docdirpage_updatefilelist(void);
	bool docdir_display_document(Gtk::TreeIter iter);
	bool docdir_display_document(void);
	void docdir_save_view(void);
	static void docdir_page_changed(EvDocumentModel *evdocumentmodel, gint arg1, gint arg2, gpointer userdata);
#endif
	void docdirpage_updatemenu(void);
#ifdef HAVE_EVINCE
	bool docpage_on_motion_notify_event(GdkEventMotion *event);
#endif
	typedef enum {
		perfval_mass,
		perfval_qnh,
		perfval_altitude,
		perfval_oat,
		perfval_isa,
		perfval_dewpt,
		perfval_wind,
		perfval_nr
	} perfval_t;
	typedef enum {
		perfunits_mass,
		perfunits_altitude,
		perfunits_pressure,
		perfunits_temperature,
		perfunits_speed,
		perfunits_nr
	} perfunits_t;
	void perf_cell_data_func(Gtk::CellRenderer *cell, const Gtk::TreeModel::iterator& iter, int column);
	void perf_cellrenderer_edited(const Glib::ustring& path_string, const Glib::ustring& new_text, int column);
	void perf_cellrendererunit_edited(const Glib::ustring& path_string, const Glib::ustring& new_text);
	void perfpage_recomputewb(void);
	void perfpage_saveperf(void);
	void perfpage_restoreperf(void);
	void perfpage_button_fromwb_clicked(void);
	void perfpage_button_fromfpl_clicked(void);
	void perfpage_spinbutton_value_changed(unsigned int idx, perfval_t pv);
	void perfpage_softpenalty_value_changed(void);
	void perfpage_unit_changed(perfunits_t pu);
	void perfpage_update(unsigned int idxmask = ~0U, perfval_t pvskip = perfval_nr);
	void perfpage_update_results(void);
	Aircraft::WeightBalance::elementvalues_t perfpage_getwbvalues(void);
	void perfpage_reloadwb(void);
	void perfpage_updatemenu(void);
	void srsspage_recompute(void);
	void srsspage_updatemenu(void);
	void euops1_recomputetdzlist(void);
	void euops1_recomputeldg(void);
	void euops1_recomputetkoff(void);
	void euops1_setminimum(void);
	void euops1_change_tdzlist(void);
	void euops1_change_tdzelev(void);
	void euops1_change_tdzunit(void);
	void euops1_change_ocaoch(void);
	void euops1_change_ocaochunit(void);
	void log_cell_data_func(Gtk::CellRenderer *cell, const Gtk::TreeModel::iterator& iter, int column);
	void log_add_message(Glib::TimeVal tv, Sensors::loglevel_t lvl, const Glib::ustring& msg);
	bool log_filter_func(const Gtk::TreeModel::const_iterator& iter);
	static const char *log_level_button[Sensors::loglevel_fatal+1];
	void metartaf_update(void);
	void metartaf_fetch(void);
	void metartaf_fetch_cb(const NWXWeather::metartaf_t& metartaf);
	void metartaf_adds_fetch_cb(const ADDS::metartaf_t& metartaf);
	bool wxchartlist_next(void);
	bool wxchartlist_prev(void);
	bool wxchartlist_nextepoch(void);
	bool wxchartlist_prevepoch(void);
	bool wxchartlist_nextcreatetime(void);
	bool wxchartlist_prevcreatetime(void);
	bool wxchartlist_nextlevel(void);
	bool wxchartlist_prevlevel(void);
	bool wxchartlist_displaysel(void);
	void wxchartlist_update(uint64_t selid = 0);
	void wxchartlist_addchart(const WXDatabase::Chart& ch);
	void wxchartlist_opendlg(void);
	void wxchartlist_refetch(void);
	void grib2layer_zoomin(void);
	void grib2layer_zoomout(void);
	bool grib2layer_next(void);
	bool grib2layer_prev(void);
	void grib2layer_togglemap(void);
	void grib2layer_unselect(void);
	void grib2layer_select(Glib::RefPtr<GRIB2::Layer> layer);
	bool grib2layer_displaysel(void);
	void grib2layer_update(void);
	void grib2layer_hide(void);
	void routeprofile_savefileresponse(int response);

	class MenuToggleButton : public Gtk::ToggleButton {
	public:
		explicit MenuToggleButton(BaseObjectType *castitem, const Glib::RefPtr<Builder>& refxml) : Gtk::ToggleButton(castitem) {}
		explicit MenuToggleButton(void) {}
	};

	class SVGImage : public Gtk::DrawingArea {
	public:
                explicit SVGImage(BaseObjectType *castitem, const Glib::RefPtr<Builder>& refxml);
                explicit SVGImage(void);
                virtual ~SVGImage();

		void set(const std::string& data, const std::string& mimetype = "");

	protected:
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
		virtual bool on_draw(const Cairo::RefPtr<Cairo::Context>& cr);
                virtual void on_hide(void);

		std::string m_data;
		std::string m_mimetype;
		Glib::RefPtr<Gdk::Pixbuf> m_pixbuf;
		int m_width;
		int m_height;
	};

	class MapDrawingArea : public Gtk::DrawingArea {
	public:
                explicit MapDrawingArea(BaseObjectType *castitem, const Glib::RefPtr<Builder>& refxml);
                explicit MapDrawingArea(void);
                virtual ~MapDrawingArea();

                typedef enum {
                        cursor_invalid,
                        cursor_mouse,
                        cursor_set
                } cursor_validity_t;

		/* must be in sync with VectorMapArea::renderer_t */
                typedef enum {
                        renderer_vmap,
                        renderer_terrain,
                        renderer_airportdiagram,
			renderer_bitmap,
			renderer_openstreetmap,
			renderer_opencyclemap,
			renderer_osm_public_transport,
			renderer_osmc_trails,
			renderer_maps_for_free,
			renderer_google_street,
			renderer_google_satellite,
			renderer_google_hybrid,
			renderer_virtual_earth_street,
			renderer_virtual_earth_satellite,
			renderer_virtual_earth_hybrid,
     			renderer_skyvector_worldvfr,
			renderer_skyvector_worldifrlow,
			renderer_skyvector_worldifrhigh,
			renderer_cartabossy,
			renderer_cartabossy_weekend,
			renderer_ign,
			renderer_openaip,
			renderer_icao_switzerland,
			renderer_glider_switzerland,
			renderer_obstacle_switzerland,
			renderer_icao_germany,
			renderer_vfr_germany,
			renderer_none
                } renderer_t;

                void set_engine(Engine& eng);
                void set_renderer(renderer_t render);
                renderer_t get_renderer(void) const;
		static bool is_renderer_enabled(renderer_t rnd);
		static VectorMapRenderer::renderer_t convert_renderer(renderer_t render);
		void set_route(const FPlanRoute& route);
                void set_track(const TracksDb::Track *track);
                void redraw_route(void);
                void redraw_track(void);
                virtual void set_center(const Point& pt, int alt, uint16_t upangle = 0, int64_t time = 0);
                virtual Point get_center(void) const;
                virtual int get_altitude(void) const;
		virtual uint16_t get_upangle(void) const;
		virtual int64_t get_time(void) const;
                void set_map_scale(float nmi_per_pixel);
                float get_map_scale(void) const;
		void set_glidemodel(const MapRenderer::GlideModel& gm = MapRenderer::GlideModel());
		const MapRenderer::GlideModel& get_glidemodel(void) const;
		void set_wind(float dir = 0, float speed = 0);
		float get_winddir(void) const;
		float get_windspeed(void) const;
                void zoom_out(void);
                void zoom_in(void);
                void pan_up(void);
                void pan_down(void);
                void pan_left(void);
                void pan_right(void);
                void set_dragbutton(unsigned int dragbutton);
                bool is_cursor_valid(void) const { return m_cursorvalid; }
                Point get_cursor(void) const { return m_cursorvalid ? m_cursor : Point(); }
                void set_cursor(const Point& pt);
                Rect get_cursor_rect(void) const;
                void invalidate_cursor(void);
                void set_cursor_angle(float angle);
                float get_cursor_angle(void) const { return m_cursorangle * MapRenderer::to_deg_16bit; }
                bool is_cursor_angle_valid(void) const { return m_cursoranglevalid; }
                void invalidate_cursor_angle(void);
                virtual operator MapRenderer::DrawFlags() const;
                virtual MapDrawingArea& operator=(MapRenderer::DrawFlags f);
                virtual MapDrawingArea& operator&=(MapRenderer::DrawFlags f);
                virtual MapDrawingArea& operator|=(MapRenderer::DrawFlags f);
                virtual MapDrawingArea& operator^=(MapRenderer::DrawFlags f);
		void set_grib2(const Glib::RefPtr<GRIB2::Layer>& layer = Glib::RefPtr<GRIB2::Layer>());
		void set_bitmap(const Glib::RefPtr<BitmapMaps::Map>& map = Glib::RefPtr<BitmapMaps::Map>());
		const Glib::RefPtr<BitmapMaps::Map>& get_bitmap(void) const { return m_map; }
                void airspaces_changed(void);
                void airports_changed(void);
                void navaids_changed(void);
                void waypoints_changed(void);
                void airways_changed(void);
                sigc::signal<void,Point,cursor_validity_t> signal_cursor(void) { return m_signal_cursor; }
                sigc::signal<void,Point> signal_pointer(void) { return m_signal_pointer; }
		void set_aircraft(const Sensors::MapAircraft& acft);
		void clear_aircraft(void);

	private:
#ifdef HAVE_GTKMM2
                virtual bool on_expose_event(GdkEventExpose *event);
#endif
		virtual bool on_draw(const Cairo::RefPtr<Cairo::Context>& cr);
                virtual void on_size_allocate(Gtk::Allocation& allocation);
                virtual void on_show(void);
                virtual void on_hide(void);
                static unsigned int button_translate(GdkEventButton* event);
                virtual bool on_button_press_event(GdkEventButton* event);
                virtual bool on_button_release_event(GdkEventButton* event);
                virtual bool on_motion_notify_event(GdkEventMotion* event);

	protected:
                void redraw(void) { if (get_is_drawable()) queue_draw(); }
                void renderer_update(void);
                void renderer_alloc(void);
		void draw_aircraft(const Cairo::RefPtr<Cairo::Context>& cr);
		bool expire_aircraft(void);
		void schedule_aircraft_redraw(void);

	protected:
                Engine *m_engine;
                MapRenderer *m_renderer;
                renderer_t m_rendertype;
                unsigned int m_dragbutton;
                MapRenderer::ScreenCoordFloat m_dragstart;
                MapRenderer::ScreenCoord m_dragoffset;
                bool m_draginprogress;
                bool m_cursorvalid;
                bool m_cursoranglevalid;
		bool m_pan;
                Point m_cursor;
                uint16_t m_cursorangle;
                sigc::signal<void,Point,cursor_validity_t> m_signal_cursor;
                sigc::signal<void,Point> m_signal_pointer;
		Glib::Dispatcher m_dispatch;
		sigc::connection m_connupdate;
                // Caching
		MapRenderer::GlideModel m_glidemodel;
		Point m_center;
		int64_t m_time;
                int m_altitude;
		uint16_t m_upangle;
                float m_mapscale;
		float m_tas;
		float m_winddir;
		float m_windspeed;
		Glib::RefPtr<GRIB2::Layer> m_grib2layer;
		Glib::RefPtr<BitmapMaps::Map> m_map;
		const FPlanRoute *m_route;
                const TracksDb::Track *m_track;
                MapRenderer::DrawFlags m_drawflags;
		// Aircraft
		typedef std::set<Sensors::MapAircraft> mapaircraft_t;
		mapaircraft_t m_mapaircraft;
		sigc::connection m_mapacftexpconn;
		sigc::connection m_mapacftconn;
		Glib::TimeVal m_mapacftredraw;
	};

	class NavAIDrawingArea : public Gtk::DrawingArea {
	public:
		explicit NavAIDrawingArea(BaseObjectType *castitem, const Glib::RefPtr<Builder>& refxml);
		explicit NavAIDrawingArea(void);
		virtual ~NavAIDrawingArea();

		double get_bank(void) const { return m_bank; }
		double get_pitch(void) const { return m_pitch; }
		double get_slip(void) const { return m_slip; }
		void set_pitch_bank_slip(double p, double b, double s);

	protected:
		double m_bank;
		double m_pitch;
		double m_slip;

#ifdef HAVE_GTKMM2
		virtual bool on_expose_event(GdkEventExpose *event);
#endif
		virtual bool on_draw(const Cairo::RefPtr<Cairo::Context>& cr);

		static inline void set_color_sky(const Cairo::RefPtr<Cairo::Context>& cr) {
			cr->set_source_rgb(0.59608, 0.81176, 0.93333);
		}

		static inline void set_color_earth(const Cairo::RefPtr<Cairo::Context>& cr) {
			cr->set_source_rgb(0.31765, 0.21176, 0.13725);
		}

		static inline void set_color_marking(const Cairo::RefPtr<Cairo::Context>& cr) {
			cr->set_source_rgb(1.0, 1.0, 1.0);
		}

		static inline void set_color_airplane(const Cairo::RefPtr<Cairo::Context>& cr) {
			cr->set_source_rgb(0.956863, 0.537255, 0.035294);
		}

		static inline void move_to_angle(const Cairo::RefPtr<Cairo::Context>& cr, double angle, double r) {
			cr->move_to(sin(angle) * r, -cos(angle) * r);
		}
     
		static inline void line_to_angle(const Cairo::RefPtr<Cairo::Context>& cr, double angle, double r) {
			cr->line_to(sin(angle) * r, -cos(angle) * r);
		}
	};

	class NavHSIDrawingArea : public Gtk::DrawingArea {
	public:
		explicit NavHSIDrawingArea(BaseObjectType *castitem, const Glib::RefPtr<Builder>& refxml);
		explicit NavHSIDrawingArea(void);
		virtual ~NavHSIDrawingArea();

		// Horizontal Situation
		double get_heading(void) const { return m_heading; }
		Sensors::mapangle_t get_heading_flags(void) const { return m_headingflags; }
		double get_rate(void) const { return m_rate; }
		void set_heading(double h, Sensors::mapangle_t hflags = Sensors::mapangle_heading);
		void set_rate(double r);
		void set_heading_rate(double h, double r, Sensors::mapangle_t hflags = Sensors::mapangle_heading);
		double get_course(void) const { return m_course; }
		double get_groundspeed(void) const { return m_groundspeed; }
		void set_course(double crs);
		void set_groundspeed(double gs);
		void set_course_groundspeed(double crs, double gs);
		double get_declination(void) const { return m_declination; }
		void set_declination(double declination);
		double get_wind_heading(void) const { return m_wind_heading; }
		Sensors::mapangle_t get_wind_heading_flags(void) const { return m_wind_headingflags; }
		void set_wind_heading(double h, Sensors::mapangle_t hflags = Sensors::mapangle_heading);

		// Navigation Pointers
		double get_pointer1brg(void) const { return m_pointer1brg; }
		double get_pointer1dev(void) const { return m_pointer1dev; }
		double get_pointer1dist(void) const { return m_pointer1dist; }
		void set_pointer1(double brg = std::numeric_limits<double>::quiet_NaN(),
				  double dev = std::numeric_limits<double>::quiet_NaN(),
				  double dist = std::numeric_limits<double>::quiet_NaN());
		typedef enum {
			pointer1flag_off,
			pointer1flag_from,
			pointer1flag_to
		} pointer1flag_t;
		pointer1flag_t get_pointer1flag(void) const { return m_pointer1flag; }
		void set_pointer1flag(pointer1flag_t flag);
		double get_pointer1maxdev(void) const { return m_pointer1maxdev; }
		void set_pointer1maxdev(double maxdev = 1.0);
		const Glib::ustring& get_pointer1dest(void) const { return m_pointer1dest; }
		void set_pointer1dest(const Glib::ustring& dest = "");
		double get_pointer2brg(void) const { return m_pointer2brg; }
		double get_pointer2dist(void) const { return m_pointer2dist; }
		void set_pointer2(double brg = std::numeric_limits<double>::quiet_NaN(),
				  double dist = std::numeric_limits<double>::quiet_NaN());
		const Glib::ustring& get_pointer2dest(void) const { return m_pointer2dest; }
		void set_pointer2dest(const Glib::ustring& dest = "");

		// Air Data
		double get_rat(void) const { return m_rat; }
		double get_oat(void) const { return m_oat; }
		double get_ias(void) const { return m_ias; }
		double get_tas(void) const { return m_tas; }
		double get_mach(void) const { return m_mach; }
		void set_rat(double rat);
		void set_oat(double oat);
		void set_ias(double ias);
		void set_tas(double tas);
		void set_mach(double mach);

		// Terrain Map
                void set_engine(Engine& eng);
                void set_route(const FPlanRoute& route);
                void set_track(const TracksDb::Track *track);
                void set_center(const Point& pt, int alt);
                void set_center(const Point& pt);
                void set_center(void);
                Point get_center(void) const;
                int get_altitude(void) const;
                void set_map_scale(float nmi_per_pixel);
                float get_map_scale(void) const;
                void set_glidemodel(const MapRenderer::GlideModel& gm = MapRenderer::GlideModel());
                const MapRenderer::GlideModel& get_glidemodel(void) const;
		void set_glidewind(float dir = 0, float speed = 0);
		float get_glidewinddir(void) const;
		float get_glidewindspeed(void) const;
                void zoom_out(void);
                void zoom_in(void);
                operator MapRenderer::DrawFlags() const;
                NavHSIDrawingArea& operator=(MapRenderer::DrawFlags f);
                NavHSIDrawingArea& operator&=(MapRenderer::DrawFlags f);
                NavHSIDrawingArea& operator|=(MapRenderer::DrawFlags f);
                NavHSIDrawingArea& operator^=(MapRenderer::DrawFlags f);
                void airspaces_changed(void);
                void airports_changed(void);
                void navaids_changed(void);
                void waypoints_changed(void);
                void airways_changed(void);

	protected:
                Engine *m_engine;
                const FPlanRoute *m_route;
                const TracksDb::Track *m_track;
                MapRenderer *m_renderer;
 		Glib::Dispatcher m_dispatch;
		sigc::connection m_connupdate;
                // Caching
		MapRenderer::GlideModel m_glidemodel;
                Point m_center;
                float m_mapscale;
		float m_glidewinddir;
		float m_glidewindspeed;
                int m_altitude;
                MapRenderer::DrawFlags m_drawflags;
		bool m_centervalid;
		bool m_altvalid;
		bool m_hasterrain;
		Glib::ustring m_pointer1dest;
		Glib::ustring m_pointer2dest;
		double m_heading;
		Sensors::mapangle_t m_headingflags;
		double m_course;
		double m_groundspeed;
		double m_rate;
		double m_declination;
		double m_wind_heading;
		Sensors::mapangle_t m_wind_headingflags;
		double m_pointer1brg;
		double m_pointer1dev;
		double m_pointer1dist;
		double m_pointer1maxdev;
		pointer1flag_t m_pointer1flag;
		double m_pointer2brg;
		double m_pointer2dist;
		double m_rat;
		double m_oat;
		double m_ias;
		double m_tas;
		double m_mach;

#ifdef HAVE_GTKMM2
		virtual bool on_expose_event(GdkEventExpose *event);
#endif
		virtual bool on_draw(const Cairo::RefPtr<Cairo::Context>& cr);
                virtual void on_size_allocate(Gtk::Allocation& allocation);
                virtual void on_show(void);
                virtual void on_hide(void);
		void renderer_update(void);

		static inline void set_color_marking(const Cairo::RefPtr<Cairo::Context>& cr) {
			cr->set_source_rgb(1.0, 1.0, 1.0);
		}

		static inline void set_color_hdgbug(const Cairo::RefPtr<Cairo::Context>& cr) {
			cr->set_source_rgb(0.956863, 0.537255, 0.035294);
		}

		static inline void set_color_rate(const Cairo::RefPtr<Cairo::Context>& cr) {
			cr->set_source_rgb(1.0, 0.0, 1.0);
		}

		static inline void set_color_course(const Cairo::RefPtr<Cairo::Context>& cr) {
			cr->set_source_rgb(1.0, 0.0, 1.0);
		}

		static inline void set_color_pointer1(const Cairo::RefPtr<Cairo::Context>& cr) {
			cr->set_source_rgb(1.0, 1.0, 0.0);
		}

		static inline void set_color_pointer1dots(const Cairo::RefPtr<Cairo::Context>& cr) {
			cr->set_source_rgb(0.3, 0.3, 0.3);
		}

		static inline void set_color_pointer2(const Cairo::RefPtr<Cairo::Context>& cr) {
			cr->set_source_rgb(0.0, 1.0, 1.0);
		}

		static inline void set_color_wind(const Cairo::RefPtr<Cairo::Context>& cr) {
			cr->set_source_rgb(0.0, 1.0, 1.0);
		}

		static inline void move_to_angle(const Cairo::RefPtr<Cairo::Context>& cr, double angle, double r) {
			cr->move_to(sin(angle) * r, -cos(angle) * r);
		}
     
		static inline void line_to_angle(const Cairo::RefPtr<Cairo::Context>& cr, double angle, double r) {
			cr->line_to(sin(angle) * r, -cos(angle) * r);
		}
	};

	class NavAltDrawingArea : public Gtk::DrawingArea {
	public:
		explicit NavAltDrawingArea(BaseObjectType *castitem, const Glib::RefPtr<Builder>& refxml);
		explicit NavAltDrawingArea(void);
		virtual ~NavAltDrawingArea();

		double get_alt(void) const { return m_alt; }
		double get_altrate(void) const { return m_altrate; }
		double get_altbug(void) const { return m_altbug; }
		double get_minimum(void) const { return m_minimum; }
		double get_glideslope(void) const { return m_glideslope; }
		double get_vsbug(void) const { return m_vsbug; }
		double get_qnh(void) const { return m_qnh; }
		bool is_std(void) const { return m_std; }
		bool is_inhg(void) const { return m_inhg; }
		bool is_metric(void) const { return m_metric; }
		TopoDb30::elev_t get_elevation(void) const { return m_elev; }

		void set_altitude(double alt, double altrate = std::numeric_limits<double>::quiet_NaN());
		void set_altbug(double altbug = std::numeric_limits<double>::quiet_NaN());
		void set_minimum(double minimum = std::numeric_limits<double>::quiet_NaN());
		void set_glideslope(double gs = std::numeric_limits<double>::quiet_NaN());
		void unset_glideslope(void) { set_glideslope(std::numeric_limits<double>::quiet_NaN()); }
		void set_vsbug(double vs = std::numeric_limits<double>::quiet_NaN());
		void unset_vsbug(void) { set_vsbug(std::numeric_limits<double>::quiet_NaN()); }
		void set_qnh(double qnh, bool std);
		void set_qnh(double qnh);
		void set_std(bool std);
		void set_inhg(bool inhg);
		void set_metric(bool metric);
		void set_elevation(TopoDb30::elev_t elev);

		typedef std::pair<Glib::ustring,double> altmarker_t;
		typedef std::vector<altmarker_t> altmarkers_t;
		void set_altmarkers(const altmarkers_t& m = altmarkers_t());
		void add_altmarker(const Glib::ustring& name, double alt) { add_altmarker(std::make_pair(name, alt)); }
		void add_altmarker(const altmarker_t& m);

		static constexpr double inhg_to_hpa = 33.86389;
		static constexpr double hpa_to_inhg = 1.0 / 33.86389;

	protected:
		altmarkers_t m_altmarkers;
		double m_alt;
		double m_altrate;
		double m_altbug;
		double m_minimum;
		double m_glideslope;
		double m_vsbug;
		double m_qnh;
		TopoDb30::elev_t m_elev;
		bool m_std;
		bool m_inhg;
		bool m_metric;

#ifdef HAVE_GTKMM2
		virtual bool on_expose_event(GdkEventExpose *event);
#endif
		virtual bool on_draw(const Cairo::RefPtr<Cairo::Context>& cr);

		static inline void set_color_marking(const Cairo::RefPtr<Cairo::Context>& cr) {
			cr->set_source_rgb(1.0, 1.0, 1.0);
		}

		static inline void set_color_background(const Cairo::RefPtr<Cairo::Context>& cr) {
			cr->set_source_rgb(0.0, 0.0, 0.0);
		}

		static inline void set_color_transparent_background(const Cairo::RefPtr<Cairo::Context>& cr) {
			cr->set_source_rgba(0.0, 0.0, 0.0, 0.7);
		}

		static inline void set_color_errorflag(const Cairo::RefPtr<Cairo::Context>& cr) {
			cr->set_source_rgba(1.0, 0.0, 0.0, 0.8);
		}

		static inline void set_color_rate(const Cairo::RefPtr<Cairo::Context>& cr) {
			cr->set_source_rgb(1.0, 0.0, 1.0);
		}

		static inline void set_color_altbug(const Cairo::RefPtr<Cairo::Context>& cr) {
			cr->set_source_rgb(0.0, 1.0, 1.0);
		}

		static inline void set_color_glideslope(const Cairo::RefPtr<Cairo::Context>& cr) {
			cr->set_source_rgb(1.0, 1.0, 0.0);
		}

		static inline void set_color_vsbug(const Cairo::RefPtr<Cairo::Context>& cr) {
			cr->set_source_rgb(0.5, 0.0, 0.5);
		}

		static inline void set_color_minimum(const Cairo::RefPtr<Cairo::Context>& cr) {
			cr->set_source_rgb(1.0, 0.0, 0.0);
		}

		static inline void set_color_terrain(const Cairo::RefPtr<Cairo::Context>& cr) {
			cr->set_source_rgb(0.5, 0.25, 0.0);
		}

		static inline void set_color_ocean(const Cairo::RefPtr<Cairo::Context>& cr) {
			cr->set_source_rgb(0.0, 0.5, 1.0);
		}

		static inline void set_color_transparent_terrain(const Cairo::RefPtr<Cairo::Context>& cr) {
			cr->set_source_rgba(0.5, 0.25, 0.0, 0.7);
		}

		static inline void set_color_transparent_ocean(const Cairo::RefPtr<Cairo::Context>& cr) {
			cr->set_source_rgba(0.0, 0.5, 1.0, 0.7);
		}
	};

	class NavInfoDrawingArea : public Gtk::DrawingArea {
	public:
		explicit NavInfoDrawingArea(BaseObjectType *castitem, const Glib::RefPtr<Builder>& refxml);
		explicit NavInfoDrawingArea(void);
		virtual ~NavInfoDrawingArea();

		void set_sensors(Sensors& sensors);
		void sensors_change(Sensors *sensors, Sensors::change_t changemask);

		time_t get_timer(void) const;
		bool is_timer_running(void) const { return !!m_timersttime; }
		void start_timer(void);
		void stop_timer(void);
		void clear_timer(void);

	protected:
		Sensors *m_sensors;
		time_t m_timeroffs;
		time_t m_timersttime;
		sigc::connection m_timerredraw;

#ifdef HAVE_GTKMM2
		virtual bool on_expose_event(GdkEventExpose *event);
#endif
		virtual bool on_draw(const Cairo::RefPtr<Cairo::Context>& cr);
		bool timer_redraw(void);

		static inline void set_color_marking(const Cairo::RefPtr<Cairo::Context>& cr) {
			cr->set_source_rgb(1.0, 1.0, 1.0);
		}

		static inline void set_color_activetimer(const Cairo::RefPtr<Cairo::Context>& cr) {
			cr->set_source_rgb(0.0, 1.0, 0.0);
		}

		static inline void set_color_background(const Cairo::RefPtr<Cairo::Context>& cr) {
			cr->set_source_rgb(0.0, 0.0, 0.0);
		}
	};

	class NavArea : public Gtk::Container {
	public:
		explicit NavArea(BaseObjectType *castitem, const Glib::RefPtr<Builder>& refxml);
		explicit NavArea(void);
		virtual ~NavArea();

		void set_engine(Engine& eng);
		void set_sensors(Sensors& sensors);
		void sensors_change(Sensors *sensors, Sensors::change_t changemask);

		// Altitude Tape
		double alt_get_altbug(void) const { return m_navaltdrawingarea.get_altbug(); }
		double alt_get_minimum(void) const { return m_navaltdrawingarea.get_minimum(); }
		double alt_get_qnh(void) const { return m_navaltdrawingarea.get_qnh(); }
		bool alt_is_std(void) const { return m_navaltdrawingarea.is_std(); }
		bool alt_is_inhg(void) const { return m_navaltdrawingarea.is_inhg(); }
		bool alt_is_metric(void) const { return m_navaltdrawingarea.is_metric(); }
		bool alt_is_labels(void) const { return m_altlabels; }

		void alt_set_altbug(double altbug = std::numeric_limits<double>::quiet_NaN());
		void alt_set_minimum(double minimum = std::numeric_limits<double>::quiet_NaN());
		void alt_set_qnh(double qnh);
		void alt_set_std(bool std);
		void alt_set_inhg(bool inhg);
		void alt_set_metric(bool metric);
		void alt_set_labels(bool labels);

		// HSI Window
                void hsi_zoom_out(void);
                void hsi_zoom_in(void);
                MapRenderer::DrawFlags hsi_get_drawflags(void) const { return m_navhsidrawingarea; }
                void hsi_set_drawflags(MapRenderer::DrawFlags f);
		bool hsi_get_wind(void) const { return m_hsiwind; }
		void hsi_set_wind(bool v);
		bool hsi_get_terrain(void) const { return m_hsiterrain; }
		void hsi_set_terrain(bool v);
		bool hsi_get_glide(void) const { return m_hsiglide; }
		void hsi_set_glide(bool v);
		void hsi_set_glidewind(double dir, double speed);

		// Map Window
		typedef std::pair<MapDrawingArea::renderer_t,unsigned int> maprenderer_t;
 		typedef std::pair<VectorMapArea::renderer_t,unsigned int> vmarenderer_t;
	        maprenderer_t map_get_renderer(void) const;
                void map_set_renderer(maprenderer_t renderer);
		vmarenderer_t vma_first_renderer(void);
		vmarenderer_t vma_next_renderer(vmarenderer_t rstart);
		maprenderer_t map_first_renderer(void);
	        maprenderer_t map_next_renderer(maprenderer_t rstart);
		void map_set_first_renderer(void);
		void map_set_next_renderer(void);
		uint32_t map_get_renderers(void) const { return m_renderers; }
                void map_set_renderers(uint32_t rnd);
		typedef std::vector<BitmapMaps::Map::ptr_t> bitmapmaps_t;
		const bitmapmaps_t& map_get_bitmapmaps(void) const { return m_bitmapmaps; }
		void map_set_bitmapmaps(const bitmapmaps_t& bmm);
                void map_zoom_out(void);
                void map_zoom_in(void);
                float map_get_map_scale(void) const { return m_navmapdrawingarea.get_map_scale(); }
		MapRenderer::DrawFlags map_get_drawflags(void) const { return m_mapdrawflags; }
                void map_set_drawflags(MapRenderer::DrawFlags f);
		typedef enum {
			dcltr_normal,
			dcltr_airspaces,
			dcltr_airways,
			dcltr_terrain,
			dcltr_number
		} dcltr_t;
		void map_set_declutter(dcltr_t d = dcltr_normal);
		dcltr_t map_get_declutter(void) const { return m_mapdeclutter; }
		void map_next_declutter(void);
		MapRenderer::DrawFlags map_get_declutter_drawflags(void) const;
		const char *map_get_declutter_name(dcltr_t d) const { return declutter_names[std::min(d, dcltr_number)]; }
		const char *map_get_declutter_name(void) const { return map_get_declutter_name(m_mapdeclutter); }
		typedef enum {
			mapup_north,
			mapup_course,
			mapup_heading,
			mapup_number
		} mapup_t;
		mapup_t map_get_mapup(void) const { return m_mapup; }
		void map_set_mapup(mapup_t up = mapup_north);
		bool map_get_pan(void) const { return m_mappan; }
		void map_set_pan(bool pan = false);

		MapDrawingArea& get_map_drawing_area(void) { return m_navmapdrawingarea; }
		NavAIDrawingArea& get_ai_drawing_area(void) { return m_navaidrawingarea; }
		NavHSIDrawingArea& get_hsi_drawing_area(void) { return m_navhsidrawingarea; }
		NavAltDrawingArea& get_alt_drawing_area(void) { return m_navaltdrawingarea; }
		NavInfoDrawingArea& get_info_drawing_area(void) { return m_navinfodrawingarea; }

		static const char cfgkey_mainwindow_mapscale[];
		static const char cfgkey_mainwindow_arptmapscale[];
		static const char cfgkey_mainwindow_terrainmapscale[];
		static const char * const cfgkey_mainwindow_mapscales[2];
		static const char cfgkey_mainwindow_renderers[];
		static const char cfgkey_mainwindow_bitmapmaps[];
		static const char cfgkey_mainwindow_mapflags[];
		static const char cfgkey_mainwindow_terrainflags[];
		static const char cfgkey_mainwindow_mapdeclutter[];
		static const char cfgkey_mainwindow_mapup[];
		static const char cfgkey_mainwindow_hsiwind[];
		static const char cfgkey_mainwindow_hsiterrain[];
		static const char cfgkey_mainwindow_hsiglide[];
		static const char cfgkey_mainwindow_altinhg[];
		static const char cfgkey_mainwindow_altmetric[];
		static const char cfgkey_mainwindow_altlabels[];
		static const char cfgkey_mainwindow_altbug[];
		static const char cfgkey_mainwindow_altminimum[];

	protected:
		static const MapRenderer::DrawFlags declutter_mask[dcltr_number];
		static const char * const declutter_names[dcltr_number + 1];
#ifdef HAVE_GTKMM2
		virtual void on_size_request(Gtk::Requisition* requisition);
#endif
#ifdef HAVE_GTKMM3
		virtual Gtk::SizeRequestMode get_request_mode_vfunc() const;
		virtual void get_preferred_width_vfunc(int& minimum_width, int& natural_width) const;
		virtual void get_preferred_height_for_width_vfunc(int width, int& minimum_height, int& natural_height) const;
		virtual void get_preferred_height_vfunc(int& minimum_height, int& natural_height) const;
		virtual void get_preferred_width_for_height_vfunc(int height, int& minimum_width, int& natural_width) const;
#endif
		virtual void on_size_allocate(Gtk::Allocation& allocation);
		virtual void on_add (Widget* widget);
		virtual void on_remove (Widget* widget);
		virtual GType child_type_vfunc() const;
		virtual void forall_vfunc (gboolean include_internals, GtkCallback callback, gpointer callback_data);

		MapDrawingArea m_navmapdrawingarea;
		NavAIDrawingArea m_navaidrawingarea;
		NavHSIDrawingArea m_navhsidrawingarea;
		NavAltDrawingArea m_navaltdrawingarea;
		NavInfoDrawingArea m_navinfodrawingarea;
		Engine *m_engine;
		Sensors *m_sensors;
		bool m_hsiwind;
		bool m_hsiterrain;
		bool m_hsiglide;
		bool m_altlabels;
		bool m_mappan;
		mapup_t m_mapup;
		dcltr_t m_mapdeclutter;
		MapRenderer::DrawFlags m_mapdrawflags;
		uint32_t m_renderers;
		unsigned int m_bitmapmap_index;
		bitmapmaps_t m_bitmapmaps;
		sigc::connection m_connmapaircraft;
	};

	class GPSAzimuthElevation : public Gtk::DrawingArea {
	public:
		explicit GPSAzimuthElevation(BaseObjectType *castitem, const Glib::RefPtr<Builder>& refxml);
		explicit GPSAzimuthElevation(void);
		virtual ~GPSAzimuthElevation();

		void update(const Sensors::Sensor::satellites_t& svs);
		void set_paramfail(Sensors::Sensor::paramfail_t pf);

		static void svcolor(const Cairo::RefPtr<Cairo::Context>& cr, const Sensors::Sensor::Satellite& sv);

	protected:
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

		Sensors::Sensor::satellites_t m_sv;
		Sensors::Sensor::paramfail_t m_paramfail;
	};

	class GPSSignalStrength : public Gtk::DrawingArea {
	public:
		explicit GPSSignalStrength(BaseObjectType *castitem, const Glib::RefPtr<Builder>& refxml);
		explicit GPSSignalStrength(void);
		virtual ~GPSSignalStrength();

		void update(const Sensors::Sensor::satellites_t& svs);
		void set_paramfail(Sensors::Sensor::paramfail_t pf);

	protected:
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

		Sensors::Sensor::satellites_t m_sv;
		Sensors::Sensor::paramfail_t m_paramfail;
	};

	class GPSContainer : public Gtk::Container {
	public:
		explicit GPSContainer(BaseObjectType *castitem, const Glib::RefPtr<Builder>& refxml);
		explicit GPSContainer(void);
		virtual ~GPSContainer();

	protected:
#ifdef HAVE_GTKMM2
		virtual void on_size_request(Gtk::Requisition* requisition);
#endif
#ifdef HAVE_GTKMM3
		virtual Gtk::SizeRequestMode get_request_mode_vfunc() const;
		virtual void get_preferred_width_vfunc(int& minimum_width, int& natural_width) const;
		virtual void get_preferred_height_for_width_vfunc(int width, int& minimum_height, int& natural_height) const;
		virtual void get_preferred_height_vfunc(int& minimum_height, int& natural_height) const;
		virtual void get_preferred_width_for_height_vfunc(int height, int& minimum_width, int& natural_width) const;
#endif
		virtual void on_size_allocate(Gtk::Allocation& allocation);
		virtual void on_add (Widget* widget);
		virtual void on_remove (Widget* widget);
		virtual GType child_type_vfunc() const;
		virtual void forall_vfunc (gboolean include_internals, GtkCallback callback, gpointer callback_data);

		std::vector<Gtk::Widget *> m_children;
	};

	class MagCalibDialog : public Gtk::Dialog {
	public:
		explicit MagCalibDialog(BaseObjectType* cobject, const Glib::RefPtr<Builder>& builder);
		virtual ~MagCalibDialog();
		void init(void);
		void init(const Eigen::Vector3d& oldmagbias, const Eigen::Vector3d& oldmagscale);
		void start(void);
		void newrawvalue(const Eigen::Vector3d& mag);
		sigc::signal<void,const Eigen::Vector3d&,const Eigen::Vector3d&>& signal_result(void) { return m_result; }

	protected:
		typedef Eigen::Vector3d magmeas_t;
		typedef std::vector<magmeas_t> measurements_t;

		class Graph;

		friend class Graph;

		sigc::signal<void,const Eigen::Vector3d&,const Eigen::Vector3d&> m_result;
		Gtk::Button *m_buttonapply;
		Gtk::Button *m_buttonclose;
		Graph *m_graph;
		Gtk::Entry *m_nrmeas;
		Gtk::Entry *m_mag[3];
		Gtk::Entry *m_magbias[3];
		Gtk::Entry *m_magscale[3];
		Gtk::Entry *m_oldmagbias[3];
		Gtk::Entry *m_oldmagscale[3];
		Eigen::Vector3d m_vmagbias;
		Eigen::Vector3d m_vmagscale;
		measurements_t m_measurements;
		magmeas_t m_minmeas;
		magmeas_t m_maxmeas;
		unsigned int m_div;
		bool m_calrun;

		void button_apply(void);
		void button_close(void);

		static int magsolve_f(const gsl_vector *x, void *data, gsl_vector *f);
		static int magsolve_df(const gsl_vector *x, void *data, gsl_matrix *J);
		static int magsolve_fdf(const gsl_vector *x, void *data, gsl_vector *f, gsl_matrix *J);
		int magsolve_func(const gsl_vector *x, gsl_vector *f, gsl_matrix *J) const;
	};

	class ConfigItem : public sigc::trackable {
	public:
		ConfigItem(Sensors::Sensor& sens, const Sensors::Sensor::ParamDesc& pd);
		virtual ~ConfigItem();
		void reference(void) const;
		void unreference(void) const;
		Sensors::Sensor& get_sensor(void) const { return m_sensor; }
		unsigned int get_paramnr(void) const { return m_paramnr; }
		virtual void update(const Sensors::Sensor::ParamChanged& pc);
		virtual void set_admin(bool admin);
		virtual Gtk::Widget *get_widget(void) = 0;
		virtual bool is_fastupdate(void) const { return false; }
		virtual Gtk::Widget *get_screenkbd_widget(bool& numeric) { numeric = false; return 0; }

	protected:
		Sensors::Sensor::ParamDesc::editable_t get_editable(void) const { return m_editable; }
		bool is_admin(void) const { return m_admin; }
		bool is_writable(void) const { return (get_editable() == Sensors::Sensor::ParamDesc::editable_user ||
						       (get_editable() == Sensors::Sensor::ParamDesc::editable_admin && is_admin())); }
		bool is_readonly(void) const { return get_editable() == Sensors::Sensor::ParamDesc::editable_readonly; }
		Sensors::Sensor::paramfail_t get_param(Glib::ustring& v) const;
		Sensors::Sensor::paramfail_t get_param(int& v) const;
		Sensors::Sensor::paramfail_t get_param(double& v) const;
		Sensors::Sensor::paramfail_t get_param(double& pitch, double& bank, double& slip, double& hdg, double& rate) const;
		Sensors::Sensor::paramfail_t get_param(Sensors::Sensor::satellites_t& sat) const;
		Sensors::Sensor::paramfail_t get_param(Sensors::Sensor::adsbtargets_t& t) const;
		void set_param(const Glib::ustring& v);
		void set_param(const int& v);
		void set_param(const double& v);
		Sensors::Sensor::paramfail_t get_param(unsigned int idx, Glib::ustring& v) const;
		Sensors::Sensor::paramfail_t get_param(unsigned int idx, int& v) const;
		Sensors::Sensor::paramfail_t get_param(unsigned int idx, double& v) const;
		Sensors::Sensor::paramfail_t get_param(unsigned int idx, Sensors::Sensor::satellites_t& sat) const;
		Sensors::Sensor::paramfail_t get_param(unsigned int idx, Sensors::Sensor::adsbtargets_t& t) const;
		void set_param(unsigned int idx, const Glib::ustring& v);
		void set_param(unsigned int idx, const int& v);
		void set_param(unsigned int idx, const double& v);

		template <typename W> class Fail;
		static void draw_failflag(Cairo::RefPtr<Cairo::Context> cr, const Gtk::Allocation& alloc);
		static void draw_failflag(Glib::RefPtr<Gdk::Window> wnd, const Gtk::Allocation& alloc);
		static void draw_failflag(Glib::RefPtr<Gdk::Window> wnd);

		Sensors::Sensor& m_sensor;
		mutable unsigned int m_refcount;
		unsigned int m_paramnr;
		Sensors::Sensor::ParamDesc::editable_t m_editable;
		bool m_admin;
	};

	class ConfigItemString;
	class ConfigItemSwitch;
	class ConfigItemNumeric;
	class ConfigItemChoice;
	class ConfigItemButton;
	class ConfigItemGyro;
	class ConfigItemSatellites;
	class ConfigItemMagCalib;
	class ConfigItemADSBTargets;

	class ConfigItemsGroup;
	class AllConfigItems;

	class Softkeys : public Gtk::Container {
	public:
		explicit Softkeys(BaseObjectType *castitem, const Glib::RefPtr<Builder>& refxml);
		explicit Softkeys(void);
		virtual ~Softkeys();
		unsigned int size(void) const { return nrbuttons; }
		MenuToggleButton& operator[](unsigned int i) { return m_button[std::min(i, nrbuttons - 1)]; }
		const MenuToggleButton& operator[](unsigned int i) const { return m_button[std::min(i, nrbuttons - 1)]; }
		sigc::signal<void,unsigned int>& signal_clicked(void) { return m_signal_clicked; }
		const sigc::signal<void,unsigned int>& signal_clicked(void) const { return m_signal_clicked; }
		bool blocked(unsigned int nr) const;
		bool block(unsigned int nr, bool should_block = true);
		bool unblock(unsigned int nr);

	protected:
#ifdef HAVE_GTKMM2
		virtual void on_size_request(Gtk::Requisition* requisition);
#endif
#ifdef HAVE_GTKMM3
		virtual Gtk::SizeRequestMode get_request_mode_vfunc() const;
		virtual void get_preferred_width_vfunc(int& minimum_width, int& natural_width) const;
		virtual void get_preferred_height_for_width_vfunc(int width, int& minimum_height, int& natural_height) const;
		virtual void get_preferred_height_vfunc(int& minimum_height, int& natural_height) const;
		virtual void get_preferred_width_for_height_vfunc(int height, int& minimum_width, int& natural_width) const;
#endif
		virtual void on_size_allocate(Gtk::Allocation& allocation);
		virtual void on_add (Widget* widget);
		virtual void on_remove (Widget* widget);
		virtual GType child_type_vfunc() const;
		virtual void forall_vfunc (gboolean include_internals, GtkCallback callback, gpointer callback_data);

		void clicked(unsigned int nr) { m_signal_clicked.emit(nr); }

		static const unsigned int nrbuttons = 12;
		static const unsigned int nrgroups = 3;
		static const unsigned int groupgap = 5;
		MenuToggleButton m_button[nrbuttons];
		sigc::connection m_conn[nrbuttons];
		sigc::signal<void,unsigned int> m_signal_clicked;
	};

	class RouteProfileDrawingArea : public Gtk::DrawingArea {
	public:
		explicit RouteProfileDrawingArea(BaseObjectType *castitem, const Glib::RefPtr<Builder>& refxml);
		explicit RouteProfileDrawingArea(void);
		virtual ~RouteProfileDrawingArea();

                void set_engine(Engine& eng);
                void set_route(const FPlanRoute& route);
		void route_change(void);
		void dist_zoom_in(void);
		void dist_zoom_out(void);
		double get_dist_zoom(void) const;
		void set_dist_zoom(double z = 1.0);
		void elev_zoom_in(void);
		void elev_zoom_out(void);
		double get_elev_zoom(void) const;
		void set_elev_zoom(double z = 0.1);
		double get_lon_zoom(void) const;
		void set_lon_zoom(double z);
		double get_lat_zoom(void) const;
		void set_lat_zoom(double z);
		void latlon_zoom_in(void);
		void latlon_zoom_out(void);
		bool is_chart(void) const { return m_chartindex >= 0; }
		void toggle_chart(void);
		void next_chart(void);
		void prev_chart(void);
		void toggle_ymode(void);
		void save(const std::string& fn);

	protected:
		static const unsigned int nrcharts = (sizeof(GRIB2::WeatherProfilePoint::isobaric_levels) /
						      sizeof(GRIB2::WeatherProfilePoint::isobaric_levels[0]));
                Engine *m_engine;
		const FPlanRoute *m_route;
		MeteoProfile m_profile;
		MeteoChart m_chart;
		Glib::RefPtr<Engine::ElevationRouteProfileResult> m_profilequery;
		Glib::Dispatcher m_dispatch;
		Point m_center;
		double m_scaledist;
		double m_scaleelev;
		double m_origindist;
		double m_originelev;
		double m_scalelon;
		double m_scalelat;
		int m_dragx;
		int m_dragy;
		int m_chartindex;
		bool m_autozoomprofile;
		bool m_autozoomchart;
		MeteoProfile::yaxis_t m_ymode;

#ifdef HAVE_GTKMM2
		virtual bool on_expose_event(GdkEventExpose *event);
#endif
		virtual bool on_draw(const Cairo::RefPtr<Cairo::Context>& cr);
                virtual void on_size_allocate(Gtk::Allocation& allocation);
                virtual void on_show(void);
                virtual void on_hide(void);
                virtual bool on_button_press_event(GdkEventButton* event);
                virtual bool on_button_release_event(GdkEventButton* event);
                virtual bool on_motion_notify_event(GdkEventMotion* event);

		void async_startquery(void);
                void async_cancel(void);
                void async_clear(void);
                void async_connectdone(void);
                void async_done(void);
                void async_result(void);
	};

	class AltDialog : public Gtk::Window {
	public:
		explicit AltDialog(BaseObjectType *castitem, const Glib::RefPtr<Builder>& refxml);
		virtual ~AltDialog();

		void set_altlabels(bool v);
		bool is_altlabels(void) const { return m_altlabels; }
		void set_std(bool v);
		bool is_std(void) const { return m_std; }
		void set_inhg(bool v);
		bool is_inhg(void) const { return m_inhg; }
		void set_metric(bool v);
		bool is_metric(void) const { return m_metric; }
		void set_altbug(double v);
		double get_altbug(void) const { return m_altbug; }
		bool is_altbug_changed(void) const { return m_altbug != m_curaltbug; }
		void set_minimum(double v);
		double get_minimum(void) const { return m_minimum; }
		bool is_minimum_changed(void) const { return m_minimum != m_curminimum; }
		void set_qnh(double v);
		double get_qnh(void) const { return m_qnh; }
		bool is_qnh_changed(void) const { return m_qnh != m_curqnh; }

		sigc::signal<void>& signal_update(void) { return m_signal_done; }
		const sigc::signal<void>& signal_update(void) const { return m_signal_done; }

	protected:
		static const unsigned int nrkeybuttons = 13;
		static const struct keybutton {
			const char *widget_name;
			guint key;
			guint16 hwkeycode;
			guint16 count;
		} keybuttons[nrkeybuttons];

		sigc::signal<void> m_signal_done;

		sigc::connection m_connaltbug;
		sigc::connection m_connbuttonaltbug;
		sigc::connection m_connminimum;
		sigc::connection m_connbuttonminimum;
		sigc::connection m_connqnh;
		sigc::connection m_connsetqnh;
		sigc::connection m_connsetstd;
		sigc::connection m_connsethpa;
		sigc::connection m_connsetinhg;
		sigc::connection m_connaltlabels;
		sigc::connection m_connmetric;
		sigc::connection m_connkey[nrkeybuttons];
		sigc::connection m_connunset;
		sigc::connection m_conndone;

		Gtk::ToggleButton *m_togglebuttonaltbug;
		Gtk::SpinButton *m_spinbuttonaltbug;
		Gtk::ToggleButton *m_togglebuttonminimum;
		Gtk::SpinButton *m_spinbuttonminimum;
		Gtk::SpinButton *m_spinbuttonqnh;
		Gtk::ToggleButton *m_togglebuttonqnh;
		Gtk::ToggleButton *m_togglebuttonstd;
		Gtk::ToggleButton *m_togglebuttonhpa;
		Gtk::ToggleButton *m_togglebuttoninhg;
		Gtk::ToggleButton *m_togglebuttonaltlabels;
		Gtk::ToggleButton *m_togglebuttonmetric;
		Gtk::Button *m_keybutton[nrkeybuttons];
		Gtk::Button *m_buttonunset;
		Gtk::Button *m_buttondone;
		Gtk::Label *m_altunitlabel[2];

		double m_curqnh;
		double m_qnh;
		double m_curaltbug;
		double m_altbug;
		double m_curminimum;
		double m_minimum;
		bool m_altlabels;
		bool m_std;
		bool m_inhg;
		bool m_metric;

		virtual bool on_delete_event(GdkEventAny* event);
		void key_clicked(unsigned int i);
		void unset_clicked(void);
		void done_clicked(void);
		void altlabels_clicked(void);
		void altbug_changed(void);
		void altbug_toggled(void);
		void minimum_changed(void);
		void minimum_toggled(void);
		void qnh_changed(void);
		void metric_clicked(void);
	};

	class HSIDialog : public Gtk::Window {
	public:
		typedef Sensors::iasmode_t iasmode_t;

		explicit HSIDialog(BaseObjectType *castitem, const Glib::RefPtr<Builder>& refxml);
		virtual ~HSIDialog();

		void set_flags(MapRenderer::DrawFlags flags);
		MapRenderer::DrawFlags get_flags(void) const { return m_flags; }

		void set_displaywind(bool v);
		bool is_displaywind(void) const { return m_displaywind; }
		void set_displayterrain(bool v);
		bool is_displayterrain(void) const { return m_displayterrain; }
		void set_displayglide(bool v);
		bool is_displayglide(void) const { return m_displayglide; }
		void set_heading(double v);
		double get_heading(void) const { return m_heading; }
		void set_heading_true(bool v);
		bool is_heading_true(void) const { return m_headingtrue; }
		bool is_heading_changed(void) const { return m_heading != m_curheading || m_headingtrue != m_curheadingtrue; }
		void set_rat(double v);
		double get_rat(void) const { return m_rat; }
		bool is_rat_changed(void) const { return m_rat != m_currat; }
		void set_ias(double v);
		double get_ias(void) const { return m_ias; }
		bool is_ias_changed(void) const { return m_ias != m_curias; }
		void set_cruiserpm(double v);
		double get_cruiserpm(void) const { return m_cruise.get_rpm(); }
		bool is_cruiserpm_changed(void) const { return m_cruise.get_rpm() != m_curcruise.get_rpm(); }
		void set_cruisemp(double v);
		double get_cruisemp(void) const { return m_cruise.get_mp(); }
		bool is_cruisemp_changed(void) const { return m_cruise.get_mp() != m_curcruise.get_mp(); }
		void set_cruisebhp(double v);
		double get_cruisebhp(void) const { return m_cruise.get_bhp(); }
		bool is_cruisebhp_changed(void) const { return m_cruise.get_bhp() != m_curcruise.get_bhp(); }
		void set_maxbhp(double b);
		double get_maxbhp(void) const { return m_maxbhp; }
		void set_constantspeed(bool cs);
		bool is_constantspeed(void) const { return m_constantspeed; }
		void set_iasmode(iasmode_t iasmode);
		iasmode_t get_iasmode(void) const { return m_iasmode; }

		sigc::signal<void>& signal_update(void) { return m_signal_done; }
		const sigc::signal<void>& signal_update(void) const { return m_signal_done; }

	protected:
		static const unsigned int nrkeybuttons = 13;
		static const struct keybutton {
			const char *widget_name;
			guint key;
			guint16 hwkeycode;
			guint16 count;
		} keybuttons[nrkeybuttons];
		static const unsigned int nrcheckbuttons = 5;
		static const struct checkbutton {
			const char *widgetname;
			MapRenderer::DrawFlags drawflag;
		} checkbuttons[nrcheckbuttons];


		sigc::signal<void> m_signal_done;

		sigc::connection m_connenamanualheading;
		sigc::connection m_connmanualheading;
		sigc::connection m_connheadingtrue;
		sigc::connection m_connrat;
		sigc::connection m_conntoggleias;
		sigc::connection m_connias;
		sigc::connection m_conntogglecruiserpm;
		sigc::connection m_conntogglecruisebhp;
		sigc::connection m_conncruiserpm;
		sigc::connection m_conncruisemp;
		sigc::connection m_conncruisebhp;
		sigc::connection m_conndisplaywind;
		sigc::connection m_conndisplayterrain;
		sigc::connection m_conndisplayglide;
		sigc::connection m_conncheckbutton[nrcheckbuttons];
		sigc::connection m_connkey[nrkeybuttons];
		sigc::connection m_connunset;
		sigc::connection m_conndone;

		Gtk::ToggleButton *m_togglebuttonmanualheading;
		Gtk::SpinButton *m_spinbuttonmanualheading;
		Gtk::CheckButton *m_checkbuttonheadingtrue;
		Gtk::SpinButton *m_spinbuttonrat;
		Gtk::ToggleButton *m_togglebuttonias;
		Gtk::SpinButton *m_spinbuttonias;
		Gtk::ToggleButton *m_togglebuttoncruiserpm;
		Gtk::SpinButton *m_spinbuttoncruiserpm;
		Gtk::SpinButton *m_spinbuttoncruisemp;
		Gtk::Label *m_hsilabelcruisemp;
		Gtk::ToggleButton *m_togglebuttoncruisebhp;
		Gtk::SpinButton *m_spinbuttoncruisebhp;
		Gtk::Label *m_hsilabelcruisebhp;
		Gtk::CheckButton *m_checkbuttonwind;
		Gtk::CheckButton *m_checkbuttonterrain;
		Gtk::CheckButton *m_checkbuttonglide;
		Gtk::CheckButton *m_checkbutton[nrcheckbuttons];
		Gtk::Button *m_keybutton[nrkeybuttons];
		Gtk::Button *m_buttonunset;
		Gtk::Button *m_buttondone;

		double m_curheading;
		double m_heading;
		double m_currat;
		double m_rat;
		double m_curias;
		double m_ias;
		Aircraft::Cruise::EngineParams m_curcruise;
		Aircraft::Cruise::EngineParams m_cruise;
		double m_maxbhp;
		bool m_constantspeed;
		iasmode_t m_iasmode;
		MapRenderer::DrawFlags m_curflags;
		MapRenderer::DrawFlags m_flags;
		bool m_curheadingtrue;
		bool m_headingtrue;
		bool m_displaywind;
		bool m_displayterrain;
		bool m_displayglide;

		virtual bool on_delete_event(GdkEventAny* event);
		void key_clicked(unsigned int i);
		void unset_clicked(void);
		void done_clicked(void);
		void heading_clicked(void);
		void heading_changed(void);
		void headingtrue_toggled(void);
		void rat_changed(void);
		void ias_changed(void);
		void cruiserpm_changed(void);
		void cruisemp_changed(void);
		void cruisebhp_changed(void);
		void update_bhp_percent(void);
		void update_iasmode(void);
		void iasmode_clicked(iasmode_t iasmode);
		void wind_toggled(void);
		void terrain_toggled(void);
		void glide_toggled(void);
		void flag_toggled(unsigned int nr);
	};

	class MapDialog : public Gtk::Window {
	public:
		explicit MapDialog(BaseObjectType *castitem, const Glib::RefPtr<Builder>& refxml);
		virtual ~MapDialog();

		void set_flags(MapRenderer::DrawFlags flags);
		MapRenderer::DrawFlags get_flags(void) const { return m_flags; }

		void set_mapup(NavArea::mapup_t up);
		NavArea::mapup_t get_mapup(void) const { return m_mapup; }

		void set_renderers(uint32_t renderers);
		uint32_t get_renderers(void) const { return m_renderers; }

		void set_bitmapmaps(const NavArea::bitmapmaps_t& bmm);
		const NavArea::bitmapmaps_t& get_bitmapmaps(void) const { return m_bitmapmaps; }

		void clear_bitmap_maps(void);
		void add_bitmap_map(const  BitmapMaps::Map::ptr_t& m);

		sigc::signal<void>& signal_update(void) { return m_signal_done; }
		const sigc::signal<void>& signal_update(void) const { return m_signal_done; }

	protected:
		static const unsigned int nrcheckbuttons = 29;
		static const struct checkbutton {
			const char *widgetname;
			MapRenderer::DrawFlags drawflag;
		} checkbuttons[nrcheckbuttons];

		class MapupModelColumns : public Gtk::TreeModelColumnRecord {
		public:
			MapupModelColumns(void);
			Gtk::TreeModelColumn<Glib::ustring> m_text;
		};

		class RendererModelColumns : public Gtk::TreeModelColumnRecord {
		public:
			RendererModelColumns(void);
			Gtk::TreeModelColumn<Glib::ustring> m_short;
			Gtk::TreeModelColumn<Glib::ustring> m_text;
			Gtk::TreeModelColumn<VectorMapArea::renderer_t> m_id;
		};

		class BitmapMapsModelColumns : public Gtk::TreeModelColumnRecord {
		public:
			class MapPtr {
			public:
				typedef BitmapMaps::Map::ptr_t ptr_t;

				MapPtr(const ptr_t& p = ptr_t()) : m_ptr(p) {}
				ptr_t& get(void) { return m_ptr; }
				const ptr_t& get(void) const { return m_ptr; }
			protected:
				ptr_t m_ptr;
			};

			BitmapMapsModelColumns(void);
			Gtk::TreeModelColumn<Glib::ustring> m_name;
			Gtk::TreeModelColumn<Glib::ustring> m_scale;
			Gtk::TreeModelColumn<Glib::ustring> m_date;
			Gtk::TreeModelColumn<MapPtr> m_map;
		};

		sigc::signal<void> m_signal_done;
		sigc::connection m_conncheckbutton[nrcheckbuttons];
		sigc::connection m_connmapup;
		sigc::connection m_connrevert;
		sigc::connection m_conndone;
		sigc::connection m_connrenderer;
		sigc::connection m_connbitmapmaps;
		Gtk::CheckButton *m_checkbutton[nrcheckbuttons];
		Gtk::ComboBox *m_mapcomboboxup;
		Gtk::TreeView *m_maptreeviewrenderer;
		Gtk::TreeView *m_maptreeviewbitmapmaps;
		Gtk::Button *m_buttonrevert;
		Gtk::Button *m_buttondone;
		MapupModelColumns m_mapupcolumns;
		Glib::RefPtr<Gtk::ListStore> m_mapupstore;
		RendererModelColumns m_renderercolumns;
		Glib::RefPtr<Gtk::ListStore> m_rendererstore;
		BitmapMapsModelColumns m_bitmapmapscolumns;
		Glib::RefPtr<Gtk::ListStore> m_bitmapmapsstore;
		MapRenderer::DrawFlags m_curflags;
		MapRenderer::DrawFlags m_flags;
		NavArea::mapup_t m_curmapup;
		NavArea::mapup_t m_mapup;
		uint32_t m_currenderers;
		uint32_t m_renderers;
		NavArea::bitmapmaps_t m_curbitmapmaps;
		NavArea::bitmapmaps_t m_bitmapmaps;

		virtual bool on_delete_event(GdkEventAny* event);
		void revert_clicked(void);
		void done_clicked(void);
		void flag_toggled(unsigned int nr);
		void mapup_changed(void);
		void renderer_changed(void);
		void update_renderer(void);
		void bitmapmaps_changed(void);
		void update_bitmapmaps(void);
	};

	class MapTileDownloaderDialog : public Gtk::Window {
	public:
		explicit MapTileDownloaderDialog(BaseObjectType *castitem, const Glib::RefPtr<Builder>& refxml);
		virtual ~MapTileDownloaderDialog();

		void open(const FPlanRoute& fpl, uint32_t renderers);

	protected:
		class RendererModelColumns : public Gtk::TreeModelColumnRecord {
		public:
			RendererModelColumns(void);
			Gtk::TreeModelColumn<Glib::ustring> m_short;
			Gtk::TreeModelColumn<Glib::ustring> m_text;
			Gtk::TreeModelColumn<int> m_totaltiles;
			Gtk::TreeModelColumn<int> m_outstandingtiles;
			Gtk::TreeModelColumn<VectorMapArea::renderer_t> m_id;
		};

		Gtk::SpinButton* m_spinbuttonenroutezoom;
		Gtk::SpinButton* m_spinbuttonairportzoom;
		Gtk::Entry* m_entryoutstanding;
		Gtk::ToggleButton* m_togglebuttondownload;
		Gtk::Button* m_buttondone;
		Gtk::TreeView* m_treeview;
		RendererModelColumns m_renderercolumns;
		Glib::RefPtr<Gtk::ListStore> m_rendererstore;
		typedef std::vector<VectorMapRenderer::TileDownloader> downloaders_t;
		downloaders_t m_downloaders;
		sigc::connection m_connrenderer;
		Rect m_bbox;
		Rect m_bboxdep;
		Rect m_bboxdest;

		virtual bool on_delete_event(GdkEventAny* event);
		void zoom_changed(void);
		void close_clicked(void);
		void download_clicked(void);
		void renderer_changed(void);
		void queue_changed(void);
	};

	class TextEntryDialog : public Gtk::Window {
	public:
		explicit TextEntryDialog(BaseObjectType *castitem, const Glib::RefPtr<Builder>& refxml);
		virtual ~TextEntryDialog();

		void set_label(const Glib::ustring& text);
		void set_text(const Glib::ustring& text);
		Glib::ustring get_text(void) const;

		sigc::signal<void,bool>& signal_done(void) { return m_signal_done; }
		const sigc::signal<void,bool>& signal_done(void) const { return m_signal_done; }

	protected:
		static const unsigned int nrkeybuttons = 48;
		static const struct keybutton {
			const char *widget_name;
			guint key;
			guint key_shift;
			guint16 hwkeycode;
		} keybuttons[nrkeybuttons];

		sigc::signal<void,bool> m_signal_done;
		sigc::connection m_connshiftleft;
		sigc::connection m_connshiftright;
		sigc::connection m_conncapslock;
                Gtk::Label *m_textentrylabel;
                Gtk::Entry *m_textentrytext;
                Gtk::Button *m_textentrybuttoncancel;
                Gtk::Button *m_textentrybuttonclear;
                Gtk::Button *m_textentrybuttonok;
                Gtk::ToggleButton *m_textentrytogglebuttonshiftleft;
		Gtk::ToggleButton *m_textentrytogglebuttonshiftright;
		Gtk::ToggleButton *m_textentrytogglebuttoncapslock;
		Gtk::Button *m_keybutton[nrkeybuttons];
		typedef enum {
			shiftstate_none,
			shiftstate_once,
			shiftstate_lock
		} shiftstate_t;
		shiftstate_t m_shiftstate;

		virtual bool on_delete_event(GdkEventAny* event);
		void key_clicked(unsigned int i);
		void ok_clicked(void);
		void cancel_clicked(void);
		void clear_clicked(void);
		void shift_clicked(void);
		void capslock_clicked(void);
		void set_shiftstate(void);
	};

	class KeyboardDialog : public Gtk::Window {
	public:
		explicit KeyboardDialog(BaseObjectType *castitem, const Glib::RefPtr<Builder>& refxml);
		virtual ~KeyboardDialog();
		void open(Gtk::Widget *widget, bool main = true, bool keypad = true);
		void open(Gtk::Widget *widget, int& w, int& h, bool main = true, bool keypad = true);
		void close(void);

		static void send_key(Gtk::Widget *w, guint state, guint keyval, guint16 hwkeycode);
		static void send_key(Glib::RefPtr<Gdk::Window> wnd, guint state, guint keyval, guint16 hwkeycode);

	protected:
		static const unsigned int nrkeybuttons = 68;
		static const struct keybutton {
			const char *widget_name;
			guint key;
			guint key_shift;
			guint16 hwkeycode;
		} keybuttons[nrkeybuttons];

		sigc::connection m_connshiftleft;
		sigc::connection m_connshiftright;
		sigc::connection m_conncapslock;
		Glib::RefPtr<Gdk::Window> m_window;
		Gtk::Table *m_keyboardtable;
		Gtk::Table *m_keyboardtablemain;
		Gtk::Table *m_keyboardtablekeypad;
		Gtk::VSeparator *m_keyboardvseparator;
		Gtk::ToggleButton *m_keyboardtogglebuttoncapslock;
		Gtk::ToggleButton *m_keyboardtogglebuttonshiftleft;
		Gtk::ToggleButton *m_keyboardtogglebuttonshiftright;
		Gtk::Button *m_keybutton[nrkeybuttons];
		typedef enum {
			shiftstate_none,
			shiftstate_once,
			shiftstate_lock
		} shiftstate_t;
		shiftstate_t m_shiftstate;

		virtual bool on_delete_event(GdkEventAny* event);
		void key_clicked(unsigned int i);
		void shift_clicked(void);
		void capslock_clicked(void);
		void set_shiftstate(void);
	};

	class CoordDialog : public Gtk::Window {
	public:
		explicit CoordDialog(BaseObjectType *castitem, const Glib::RefPtr<Builder>& refxml);
		virtual ~CoordDialog();
		void init(FlightDeckWindow *fdwnd);
		void set_flightdeckwindow(FlightDeckWindow *flightdeckwindow);
		void set_sensors(Sensors *sensors);
		void set_engine(Engine& eng);
		void coord_updatepos(void);

		void set_flags(MapRenderer::DrawFlags flags);
		MapRenderer::DrawFlags get_flags(void) const { return m_flags; }
		void set_map_scale(float scale, float arptscale);

		void edit_waypoint(void);
		void edit_waypoint(unsigned int nr);
		void directto_waypoint(void);
		void directto_waypoint(unsigned int nr);
		void directto(const Point& pt, const Glib::ustring& name = "", const Glib::ustring& icao = "", double legtrack = std::numeric_limits<double>::quiet_NaN());
		Point get_coord(void) const;
		void set_altitude(int32_t alt, uint16_t altflags);
		int32_t get_altitude(uint16_t& altflags) const { altflags = m_altflags; return m_alt; }

	protected:
		static const unsigned int nrkeybuttons = 15;
		static const struct keybutton {
			const char *widget_name;
			guint key;
			guint16 hwkeycode;
			guint16 count;
		} keybuttons[nrkeybuttons];

		class DistUnitModelColumns : public Gtk::TreeModelColumnRecord {
		public:
			DistUnitModelColumns(void);
			Gtk::TreeModelColumn<Glib::ustring> m_name;
			Gtk::TreeModelColumn<double> m_factor;
		};

		class AltUnitModelColumns : public Gtk::TreeModelColumnRecord {
		public:
			AltUnitModelColumns(void);
			Gtk::TreeModelColumn<Glib::ustring> m_name;
			Gtk::TreeModelColumn<unsigned int> m_id;
		};

		class MapModelColumns : public Gtk::TreeModelColumnRecord {
		public:
			MapModelColumns(void);
			Gtk::TreeModelColumn<Glib::ustring> m_name;
			Gtk::TreeModelColumn<VectorMapArea::renderer_t> m_renderer;
		};

		sigc::connection m_connkey[nrkeybuttons];

		FPlanRoute m_route;
		Sensors *m_sensors;
		FlightDeckWindow *m_flightdeckwindow;
		VectorMapArea *m_coorddrawingarea;
		Gtk::Entry *m_coordentryicao;
		Gtk::Entry *m_coordentryname;
		Gtk::Entry *m_coordentrycoord;
		Gtk::Button *m_coordbuttonrevertcoord;
                Gtk::Frame *m_coordframeradialdistance;
		Gtk::SpinButton *m_coordspinbuttonradial;
		Gtk::CheckButton *m_coordcheckbuttonradialtrue;
		Gtk::Button *m_coordbuttonradial180;
		Gtk::Button *m_coordbuttonradialdistclear;
		Gtk::ComboBox *m_coordcomboboxdistunit;
		Gtk::SpinButton *m_coordspinbuttondist;
		Gtk::Entry *m_coordentryfinalwgs84;
                Gtk::Entry *m_coordentryfinallocator;
		Gtk::Entry *m_coordentryfinalch1903;
                Gtk::Frame *m_coordframejoin;
		Gtk::SpinButton *m_coordspinbuttonjoin;
		Gtk::CheckButton *m_coordcheckbuttonjointrue;
		Gtk::Button *m_coordbuttonjoin180;
		Gtk::ToggleButton *m_coordtogglebuttonjoinleg;
		Gtk::ToggleButton *m_coordtogglebuttonjoindirect;
		Gtk::SpinButton *m_coordspinbuttonalt;
		Gtk::ComboBox *m_coordcomboboxaltunit;
		Gtk::ComboBox *m_coordcomboboxmap;
		Gtk::Button *m_coordbuttonzoomin;
		Gtk::Button *m_coordbuttonzoomout;
		Gtk::Button *m_coordbuttonrevert;
		Gtk::Button *m_coordbuttoncancel;
		Gtk::Button *m_coordbuttonok;
		Gtk::Button *m_keybutton[nrkeybuttons];

		DistUnitModelColumns m_distunitcolumns;
		Glib::RefPtr<Gtk::ListStore> m_distunitstore;
		AltUnitModelColumns m_altunitcolumns;
		Glib::RefPtr<Gtk::ListStore> m_altunitstore;
		MapModelColumns m_mapcolumns;
		Glib::RefPtr<Gtk::ListStore> m_mapstore;
		MapRenderer::DrawFlags m_flags;
		float m_mapscale;
		float m_arptscale;
		Point m_wptorigcoord;
		Point m_wptcoord;
		Glib::ustring m_wptorigname;
		Glib::ustring m_wptorigicao;
		int32_t m_origalt;
		int32_t m_alt;
		uint16_t m_origaltflags;
		uint16_t m_altflags;
		double m_radial;
		double m_dist;
		double m_jointrack;
		double m_legtrack;
		unsigned int m_wptnr;
		typedef enum {
			mode_off,
			mode_wpt,
			mode_directto,
			mode_directtofpl,
		} mode_t;
		mode_t m_mode;
		typedef enum {
			joinmode_off,
			joinmode_track,
			joinmode_leg,
			joinmode_direct
		} joinmode_t;
		joinmode_t m_joinmode;
		bool m_coordchange;

		sigc::connection m_coordentryconn;
		sigc::connection m_altunitchangedconn;
		sigc::connection m_altchangedconn;
		sigc::connection m_radialchangedconn;
		sigc::connection m_radialtruechangedconn;
		sigc::connection m_distchangedconn;
		sigc::connection m_distunitchangedconn;
		sigc::connection m_cursorchangeconn;
		sigc::connection m_joinconn;
		sigc::connection m_joinlegconn;
		sigc::connection m_joindirectconn;

		virtual bool on_delete_event(GdkEventAny* event);
		virtual void on_show(void);
		virtual void on_hide(void);
		void key_clicked(unsigned int i);
		void coordentry_changed(void);
		bool coordentry_focus_in_event(GdkEventFocus *event);
		bool coordentry_focus_out_event(GdkEventFocus *event);
		void cursor_change(Point pt, VectorMapArea::cursor_validity_t valid);
		void coord_update(void);
		void altunit_changed(void);
		void alt_changed(void);
		void alt_update(void);
		void radial_changed(void);
		void dist_changed(void);
		void join_update(bool forceupdjoin = false);
		void join_changed(void);
		void jointrue_toggled(void);
		void joinleg_toggled(void);
		void joindirect_toggled(void);
		void buttonrevertcoord_clicked(void);
 		void buttonradial180_clicked(void);
		void buttonradialdistclear_clicked(void);
		void buttonjoin180_clicked(void);
		void comboboxmap_changed(void);
		void buttonzoomin_clicked(void);
		void buttonzoomout_clicked(void);
		void buttonrevert_clicked(void);
		void buttoncancel_clicked(void);
		void buttonok_clicked(void);
		void showdialog(void);
	};

	class SuspendDialog : public Gtk::Window {
	public:
		explicit SuspendDialog(BaseObjectType *castitem, const Glib::RefPtr<Builder>& refxml);
		virtual ~SuspendDialog();
		void set_sensors(Sensors *sensors);
		void set_engine(Engine& eng);

		void set_flags(MapRenderer::DrawFlags flags);
		MapRenderer::DrawFlags get_flags(void) const { return m_flags; }
		void set_map_scale(float scale, float arptscale);

	protected:
		static const unsigned int nrkeybuttons = 11;
		static const struct keybutton {
			const char *widget_name;
			guint key;
			guint16 hwkeycode;
			guint16 count;
		} keybuttons[nrkeybuttons];

		class MapModelColumns : public Gtk::TreeModelColumnRecord {
		public:
			MapModelColumns(void);
			Gtk::TreeModelColumn<Glib::ustring> m_name;
			Gtk::TreeModelColumn<VectorMapArea::renderer_t> m_renderer;
		};

		sigc::connection m_connkey[nrkeybuttons];
		sigc::connection m_connspinbutton;

		FPlanRoute m_route;
		Sensors *m_sensors;
		VectorMapArea *m_suspenddrawingarea;
		Gtk::SpinButton *m_suspendspinbutton;
		Gtk::CheckButton *m_suspendcheckbuttontrue;
		Gtk::Button *m_suspendbutton180;
		Gtk::Button *m_suspendbuttoncancel;
		Gtk::Button *m_suspendbuttonok;
		Gtk::ComboBox *m_suspendcomboboxmap;
		Gtk::Button *m_suspendbuttonzoomin;
		Gtk::Button *m_suspendbuttonzoomout;
		Gtk::Button *m_keybutton[nrkeybuttons];
		double m_track;
		MapModelColumns m_mapcolumns;
		Glib::RefPtr<Gtk::ListStore> m_mapstore;
		MapRenderer::DrawFlags m_flags;
		float m_mapscale;
		float m_arptscale;

		virtual bool on_delete_event(GdkEventAny* event);
		virtual void on_show(void);
		virtual void on_hide(void);
		void key_clicked(unsigned int i);
		void track_update(bool updspin = true);
		void track_changed(void);
		void buttontrue_toggled(void);
		void button180_clicked(void);
		void buttonok_clicked(void);
		void comboboxmap_changed(void);
		void buttonzoomin_clicked(void);
		void buttonzoomout_clicked(void);
	};


	class FPLAtmosphereDialog : public Gtk::Window {
	public:
		explicit FPLAtmosphereDialog(BaseObjectType *castitem, const Glib::RefPtr<Builder>& refxml);
		virtual ~FPLAtmosphereDialog();

		void open(Sensors *sensors, const Aircraft::WeightBalance::elementvalues_t& wbev = Aircraft::WeightBalance::elementvalues_t());
		void sensors_change(Sensors::change_t changemask);

	protected:
		static const unsigned int nrkeybuttons = 11;
		static const struct keybutton {
			const char *widget_name;
			guint key;
			guint16 hwkeycode;
			guint16 count;
		} keybuttons[nrkeybuttons];

		sigc::signal<void> m_signal_done;

		sigc::connection m_connoffblock;
		sigc::connection m_connoffblockh;
		sigc::connection m_connoffblockm;
		sigc::connection m_connoffblocks;
		sigc::connection m_conntaxitimem;
		sigc::connection m_conntaxitimes;
		sigc::connection m_conncruisealt;
		sigc::connection m_connkey[nrkeybuttons];

                Gtk::Calendar *m_calendaroffblock;
                Gtk::SpinButton *m_spinbuttonoffblockh;
                Gtk::SpinButton *m_spinbuttonoffblockm;
                Gtk::SpinButton *m_spinbuttonoffblocks;
                Gtk::SpinButton *m_spinbuttontaxitimem;
                Gtk::SpinButton *m_spinbuttontaxitimes;
                Gtk::SpinButton *m_spinbuttoncruisealt;
                Gtk::SpinButton *m_spinbuttonwinddir;
                Gtk::SpinButton *m_spinbuttonwindspeed;
                Gtk::SpinButton *m_spinbuttonisaoffs;
                Gtk::SpinButton *m_spinbuttonqff;
                Gtk::Button *m_buttontoday;
                Gtk::Button *m_buttondone;
                Gtk::Button *m_buttonsetwind;
                Gtk::Button *m_buttonsetisaoffs;
                Gtk::Button *m_buttonsetqff;
                Gtk::Button *m_buttongetnwx;
                Gtk::Button *m_buttongfs;
		Gtk::Button *m_buttonskyvector;
		Gtk::Button *m_buttonsaveplog;
                Gtk::Button *m_buttonsavewx;
		Gtk::Entry *m_entryflighttime;
                Gtk::Entry *m_entryfuel;

		Gtk::Button *m_keybutton[nrkeybuttons];

		Sensors *m_sensors;
		NWXWeather m_nwxweather;
		Aircraft::WeightBalance::elementvalues_t m_wbev;
		gint64 m_gfsminreftime;
		gint64 m_gfsmaxreftime;
		gint64 m_gfsminefftime;
		gint64 m_gfsmaxefftime;

		virtual bool on_delete_event(GdkEventAny* event);
		void key_clicked(unsigned int i);
		void today_clicked(void);
		void done_clicked(void);
		void setwind_clicked(void);
		void setisaoffs_clicked(void);
		void setqff_clicked(void);
		void getnwx_clicked(void);
		void gfs_clicked(void);
		void skyvector_clicked(void);
		void saveplog_clicked(void);
		void savewx_clicked(void);
		void deptime_changed(void);
		void cruisealt_changed(void);
		void fetchwind_cb(bool fplmodif);
	};

	class FPLVerificationDialog : public Gtk::Window {
	public:
		explicit FPLVerificationDialog(BaseObjectType *castitem, const Glib::RefPtr<Builder>& refxml);
		virtual ~FPLVerificationDialog();

		void open(Sensors *sensors);
		void sensors_change(Sensors::change_t changemask);

	protected:
		Gtk::Button *m_buttondone;
		Gtk::Button *m_buttonvalidate;
		Gtk::Button *m_buttonvalidateeurofpl;
		Gtk::Button *m_buttonvalidatelocal;
		Gtk::Button *m_buttonfixup;
		Gtk::ComboBox *m_comboboxawymode;
		Gtk::TextView *m_textviewfpl;
		Gtk::TextView *m_textviewresult;
		Sensors *m_sensors;
		Glib::RefPtr<Gio::Socket> m_childsock;
		sigc::connection m_connchildwatch;
		sigc::connection m_connchildstdout;
		Glib::RefPtr<Glib::IOChannel> m_childchanstdin;
		Glib::RefPtr<Glib::IOChannel> m_childchanstdout;
		Glib::Pid m_childpid;
		bool m_childrun;
		bool m_clearresulttext;

                virtual void on_show(void);
                virtual void on_hide(void);
		virtual bool on_delete_event(GdkEventAny* event);
		void done_clicked(void);
		void validate_clicked(void);
		void validateeurofpl_clicked(void);
		void validatelocal_clicked(void);
		void fixup_clicked(void);
		void fpl_changed(void);
		void awycollapsed_changed(void);
		void child_watch(GPid pid, int child_status);
		void child_run(void);
		void child_close(void);
		bool child_stdout_handler(Glib::IOCondition iocond);
		bool child_socket_handler(Glib::IOCondition iocond);
		void child_send(const std::string& tx);
		bool child_is_running(void) const;
	};

	class FPLAutoRouteDialog : public Gtk::Window {
	public:
		explicit FPLAutoRouteDialog(BaseObjectType *castitem, const Glib::RefPtr<Builder>& refxml);
		virtual ~FPLAutoRouteDialog();

		void open(Sensors *sensors);

		void set_flags(MapRenderer::DrawFlags flags);
		MapRenderer::DrawFlags get_flags(void) const { return m_flags; }
		void set_map_scale(float scale);

	protected:
		class TreeViewPopup : public Gtk::TreeView {
		public:
			explicit TreeViewPopup(BaseObjectType *castitem, const Glib::RefPtr<Builder>& refxml);
			virtual ~TreeViewPopup();
			sigc::connection connect_menunew(const sigc::slot<void>& slot);
			sigc::connection connect_menudelete(const sigc::slot<void>& slot);
			void set_sensitive_new(bool s);
			void set_sensitive_delete(bool s);

		protected:
			Gtk::MenuItem *m_menuitem_new;
			Gtk::MenuItem *m_menuitem_delete;
			Gtk::Menu m_menu;

			virtual bool on_button_press_event(GdkEventButton *ev);
		};

                class RulesModelColumns : public Gtk::TreeModelColumnRecord {
		public:
                        RulesModelColumns(void);
                        Gtk::TreeModelColumn<Glib::ustring> m_rule;
		};

                class CrossingModelColumns : public Gtk::TreeModelColumnRecord {
		public:
                        CrossingModelColumns(void);
			Gtk::TreeModelColumn<Point> m_coord;
                        Gtk::TreeModelColumn<Glib::ustring> m_ident;
                        Gtk::TreeModelColumn<Glib::ustring> m_name;
                        Gtk::TreeModelColumn<double> m_radius;
                        Gtk::TreeModelColumn<guint> m_minlevel;
                        Gtk::TreeModelColumn<guint> m_maxlevel;
		};

                class AirspaceModelColumns : public Gtk::TreeModelColumnRecord {
		public:
                        AirspaceModelColumns(void);
                        Gtk::TreeModelColumn<Glib::ustring> m_ident;
			Gtk::TreeModelColumn<Glib::ustring> m_type;
			Gtk::TreeModelColumn<Glib::ustring> m_name;
 			Gtk::TreeModelColumn<Point> m_coord;
                        Gtk::TreeModelColumn<guint> m_minlevel;
                        Gtk::TreeModelColumn<guint> m_maxlevel;
			Gtk::TreeModelColumn<double> m_awylimit;
			Gtk::TreeModelColumn<double> m_dctlimit;
			Gtk::TreeModelColumn<double> m_dctoffset;
			Gtk::TreeModelColumn<double> m_dctscale;
		};

                class RoutesModelColumns : public Gtk::TreeModelColumnRecord {
		public:
			class FPLRoute : public FPlanRoute {
			public:
				FPLRoute(void) : FPlanRoute(*(FPlan *)0) {}
				FPLRoute(const FPlanRoute& x) : FPlanRoute(x) {}
			};

                        RoutesModelColumns(void);
                        Gtk::TreeModelColumn<Glib::ustring> m_time;
			Gtk::TreeModelColumn<Glib::ustring> m_fuel;
			Gtk::TreeModelColumn<Glib::ustring> m_routetxt;
			Gtk::TreeModelColumn<Glib::ustring> m_flags;
			Gtk::TreeModelColumn<FPLRoute> m_route;
			Gtk::TreeModelColumn<double> m_routemetric;
			Gtk::TreeModelColumn<double> m_routefuel;
			Gtk::TreeModelColumn<double> m_routezerowindfuel;
			Gtk::TreeModelColumn<guint> m_routetime;
			Gtk::TreeModelColumn<guint> m_routezerowindtime;
			Gtk::TreeModelColumn<guint> m_variant;
		};

		class FindAirspace;

		static const unsigned int nrkeybuttons = 48;
		static const struct keybutton {
			const char *widget_name;
			guint key;
			guint key_shift;
			guint16 hwkeycode;
		} keybuttons[nrkeybuttons];

		typedef bool (FPLAutoRouteDialog::*parseresponse_t)(Glib::MatchInfo&);
		static const struct parsefunctions {
			const char *regex;
			parseresponse_t func;
		} parsefunctions[];

		Sensors *m_sensors;
		Gtk::Notebook *m_fplautoroutenotebook;
		Gtk::Entry *m_fplautorouteentrydepicao;
		Gtk::Entry *m_fplautorouteentrydepname;
		Gtk::CheckButton *m_fplautoroutecheckbuttondepifr;
		Gtk::Label *m_fplautoroutelabelsid;
		Gtk::Entry *m_fplautorouteentrysidicao;
		Gtk::Entry *m_fplautorouteentrysidname;
		Gtk::CheckButton *m_fplautoroutecheckbuttonsiddb;
		Gtk::CheckButton *m_fplautoroutecheckbuttonsidonly;
		Gtk::Entry *m_fplautorouteentrydesticao;
		Gtk::Entry *m_fplautorouteentrydestname;
		Gtk::CheckButton *m_fplautoroutecheckbuttondestifr;
		Gtk::Label *m_fplautoroutelabelstar;
		Gtk::Entry *m_fplautorouteentrystaricao;
		Gtk::Entry *m_fplautorouteentrystarname;
		Gtk::CheckButton *m_fplautoroutecheckbuttonstardb;
		Gtk::CheckButton *m_fplautoroutecheckbuttonstaronly;
		Gtk::SpinButton *m_fplautoroutespinbuttondctlimit;
		Gtk::SpinButton *m_fplautoroutespinbuttondctpenalty;
		Gtk::SpinButton *m_fplautoroutespinbuttondctoffset;
		Gtk::SpinButton *m_fplautoroutespinbuttonstdroutepenalty;
		Gtk::SpinButton *m_fplautoroutespinbuttonstdrouteoffset;
		Gtk::SpinButton *m_fplautoroutespinbuttonstdroutedctpenalty;
		Gtk::SpinButton *m_fplautoroutespinbuttonstdroutedctoffset;
		Gtk::SpinButton *m_fplautoroutespinbuttonukcasdctpenalty;
		Gtk::SpinButton *m_fplautoroutespinbuttonukcasdctoffset;
		Gtk::SpinButton *m_fplautoroutespinbuttonukocasdctpenalty;
		Gtk::SpinButton *m_fplautoroutespinbuttonukocasdctoffset;
		Gtk::SpinButton *m_fplautoroutespinbuttonukenterdctpenalty;
		Gtk::SpinButton *m_fplautoroutespinbuttonukenterdctoffset;
		Gtk::SpinButton *m_fplautoroutespinbuttonukleavedctpenalty;
		Gtk::SpinButton *m_fplautoroutespinbuttonukleavedctoffset;
		Gtk::SpinButton *m_fplautoroutespinbuttonminlevel;
		Gtk::SpinButton *m_fplautoroutespinbuttonminlevelpenalty;
		Gtk::SpinButton *m_fplautoroutespinbuttonminlevelmaxpenalty;
		Gtk::SpinButton *m_fplautoroutespinbuttonvfraspclimit;
		Gtk::CheckButton *m_fplautoroutecheckbuttonforceenrouteifr;
		Gtk::CheckButton *m_fplautoroutecheckbuttonwind;
		Gtk::CheckButton *m_fplautoroutecheckbuttonawylevels;
		Gtk::CheckButton *m_fplautoroutecheckbuttonprecompgraph;
		Gtk::CheckButton *m_fplautoroutecheckbuttontfr;
		Gtk::ComboBox *m_fplautoroutecomboboxvalidator;
		Gtk::SpinButton *m_fplautoroutespinbuttonsidlimit;
		Gtk::SpinButton *m_fplautoroutespinbuttonsidpenalty;
		Gtk::SpinButton *m_fplautoroutespinbuttonsidoffset;
		Gtk::SpinButton *m_fplautoroutespinbuttonsidminimum;
		Gtk::SpinButton *m_fplautoroutespinbuttonstarlimit;
		Gtk::SpinButton *m_fplautoroutespinbuttonstarpenalty;
		Gtk::SpinButton *m_fplautoroutespinbuttonstaroffset;
		Gtk::SpinButton *m_fplautoroutespinbuttonstarminimum;
		Gtk::SpinButton *m_fplautoroutespinbuttonbaselevel;
		Gtk::SpinButton *m_fplautoroutespinbuttontoplevel;
		Gtk::ComboBox *m_fplautoroutecomboboxopttarget;
		TreeViewPopup *m_fplautoroutetreeviewtracerules;
		Gtk::Button *m_fplautoroutebuttontracerulesadd;
		Gtk::Button *m_fplautoroutebuttontracerulesdelete;
		TreeViewPopup *m_fplautoroutetreeviewdisablerules;
		Gtk::Button *m_fplautoroutebuttondisablerulesadd;
		Gtk::Button *m_fplautoroutebuttondisablerulesdelete;
		TreeViewPopup *m_fplautoroutetreeviewcrossing;
		Gtk::Button *m_fplautoroutebuttoncrossingadd;
		Gtk::Button *m_fplautoroutebuttoncrossingdelete;
		TreeViewPopup *m_fplautoroutetreeviewairspace;
		Gtk::Button *m_fplautoroutebuttonairspaceadd;
		Gtk::Button *m_fplautoroutebuttonairspacedelete;
		Gtk::SpinButton *m_fplautoroutespinbuttonpreferredminlevel;
		Gtk::SpinButton *m_fplautoroutespinbuttonpreferredmaxlevel;
		Gtk::SpinButton *m_fplautoroutespinbuttonpreferredpenalty;
		Gtk::SpinButton *m_fplautoroutespinbuttonpreferredclimb;
		Gtk::SpinButton *m_fplautoroutespinbuttonpreferreddescent;
		Gtk::Calendar *m_fplautoroutecalendar;
		Gtk::Button *m_fplautoroutebuttontoday;
		Gtk::SpinButton *m_fplautoroutespinbuttonhour;
		Gtk::SpinButton *m_fplautoroutespinbuttonmin;
		Gtk::Button *m_fplautoroutebuttonpg1close;
		Gtk::Button *m_fplautoroutebuttonpg1copy;
		Gtk::Button *m_fplautoroutebuttonpg1next;
		Gtk::Notebook *m_fplautoroutenotebookpg2;
	        VectorMapArea *m_fplautoroutedrawingarea;
		Gtk::Entry *m_fplautorouteentrypg2dep;
		Gtk::Entry *m_fplautorouteentrypg2dest;
		Gtk::Entry *m_fplautorouteentrypg2gcdist;
		Gtk::Entry *m_fplautorouteentrypg2routedist;
		Gtk::Entry *m_fplautorouteentrypg2mintime;
		Gtk::Entry *m_fplautorouteentrypg2routetime;
		Gtk::Entry *m_fplautorouteentrypg2minfuel;
		Gtk::Entry *m_fplautorouteentrypg2routefuel;
		Gtk::Entry *m_fplautorouteentrypg2iteration;
		Gtk::Entry *m_fplautorouteentrypg2times;
		Gtk::Frame *m_fplautorouteframepg2routes;
		Gtk::TreeView *m_fplautoroutetreeviewpg2routes;
		Gtk::TextView *m_fplautoroutetextviewpg2route;
		Gtk::TextView *m_fplautoroutetextviewpg2results;
		Gtk::TextView *m_fplautoroutetextviewpg2log;
		Gtk::TextView *m_fplautoroutetextviewpg2graph;
		Gtk::TextView *m_fplautoroutetextviewpg2comm;
		Gtk::TextView *m_fplautoroutetextviewpg2debug;
		Gtk::Button *m_fplautoroutebuttonpg2zoomin;
		Gtk::Button *m_fplautoroutebuttonpg2zoomout;
		Gtk::Button *m_fplautoroutebuttonpg2back;
		Gtk::Button *m_fplautoroutebuttonpg2close;
		Gtk::Button *m_fplautoroutebuttonpg2copy;
		Gtk::ToggleButton *m_fplautoroutetogglebuttonpg2run;
		sigc::connection m_connshiftleft;
		sigc::connection m_connshiftright;
		sigc::connection m_conncapslock;
		Gtk::ToggleButton *m_fplautoroutetogglebuttonshiftleft;
		Gtk::ToggleButton *m_fplautoroutetogglebuttonshiftright;
		Gtk::ToggleButton *m_fplautoroutetogglebuttoncapslock;
		Gtk::Button *m_keybutton[nrkeybuttons];
		typedef enum {
			shiftstate_none,
			shiftstate_once,
			shiftstate_lock
		} shiftstate_t;
		shiftstate_t m_shiftstate;
		sigc::connection m_connpg2run;
		CFMUAutorouteProxy m_proxy;
#ifdef HAVE_JSONCPP
		typedef Glib::RefPtr<Gio::Socket> childsocket_t;
		typedef std::set<childsocket_t> childsockets_t;
		childsockets_t m_childsockets;
#ifdef G_OS_UNIX
		Glib::RefPtr<Gio::UnixSocketAddress> m_childsockaddr;
#else
		Glib::RefPtr<Gio::InetSocketAddress> m_childsockaddr;
#endif
		std::string m_childsession;
#endif
		AirportsDb::Airport m_departure;
		AirportsDb::Airport m_destination;
		double m_sidstarlimit[2][2];
		Point m_sidend;
		Point m_starstart;
		bool m_depifr;
		bool m_destifr;
		bool m_firstrun;
		bool m_fplvalclear;
		bool m_ptchanged[6];
		MapRenderer::DrawFlags m_flags;
		float m_mapscale;
		Glib::RefPtr<Gtk::TextTag> m_txtgraphfpl;
		Glib::RefPtr<Gtk::TextTag> m_txtgraphrule;
		Glib::RefPtr<Gtk::TextTag> m_txtgraphruledesc;
		Glib::RefPtr<Gtk::TextTag> m_txtgraphruleoprgoal;
		Glib::RefPtr<Gtk::TextTag> m_txtgraphchange;
		RulesModelColumns m_rulescolumns;
                Glib::RefPtr<Gtk::ListStore> m_tracerulesstore;
                Glib::RefPtr<Gtk::ListStore> m_disablerulesstore;
		sigc::connection m_tracerulesselchangeconn;
		sigc::connection m_disablerulesselchangeconn;
		CrossingModelColumns m_crossingcolumns;
		Glib::RefPtr<Gtk::ListStore> m_crossingstore;
		sigc::connection m_crossingselchangeconn;
		sigc::connection m_crossingrowchangeconn;
		AirspaceModelColumns m_airspacecolumns;
		Glib::RefPtr<Gtk::ListStore> m_airspacestore;
		sigc::connection m_airspaceselchangeconn;
		sigc::connection m_airspacerowchangeconn;
		RoutesModelColumns m_routescolumns;
		Glib::RefPtr<Gtk::ListStore> m_routesstore;
		sigc::connection m_routesselchangeconn;

		virtual bool on_delete_event(GdkEventAny* event);
		void key_clicked(unsigned int i);
		void shift_clicked(void);
		void capslock_clicked(void);
		void set_shiftstate(void);
		void close(void);
		void today_clicked(void);
		void tfr_toggled(void);
		void wind_toggled(void);
		void tracerules_selection_changed(void);
		void tracerules_menu_new(void);
		void tracerules_menu_delete(void);
		void disablerules_selection_changed(void);
		void disablerules_menu_new(void);
		void disablerules_menu_delete(void);
		void crossing_selection_changed(void);
		void crossing_menu_new(void);
		void crossing_menu_delete(void);
		void crossing_row_changed(const Gtk::TreeModel::Path& path, const Gtk::TreeModel::iterator& iter);
		void airspace_selection_changed(void);
		void airspace_menu_new(void);
		void airspace_menu_delete(void);
		void airspace_row_changed(const Gtk::TreeModel::Path& path, const Gtk::TreeModel::iterator& iter);
		void routes_selection_changed(void);
		void pg1_close_clicked(void);
		void pg1_copy_clicked(void);
		void pg1_next_clicked(void);
		void pg2_zoomin_clicked(void);
		void pg2_zoomout_clicked(void);
		void pg2_back_clicked(void);
		void pg2_run_clicked(void);
		void ifr_toggled(void);
		void opttarget_changed(void);
		void pt_changed(unsigned int i);
		bool pt_focusout(GdkEventFocus *event = 0);
		void append_log(const Glib::ustring& text);
		Rect get_bbox(void) const;
		void initial_fplan(void);
		void recvcmd(const CFMUAutorouteProxy::Command& cmd);
#ifdef HAVE_JSONCPP
		bool recvjson(Glib::IOCondition iocond, const childsocket_t& sock);
		void recvjsoncmd(const Json::Value& cmd);
		void sendjson(const Json::Value& v);
#endif
	};

	class NWXChartDialog : public Gtk::Window {
	public:
		static const time_t unselepoch = 0;
		static const unsigned int unsellevel = ~1U;

		explicit NWXChartDialog(BaseObjectType *castitem, const Glib::RefPtr<Builder>& refxml);
		virtual ~NWXChartDialog();

		void open(Sensors *sensors, const sigc::slot<void,const WXDatabase::Chart&>& cb,
			  unsigned int width = 1024, unsigned int height = 768,
			  time_t selepoch = unselepoch, unsigned int sellevel = unsellevel, unsigned int charttype = 0);
		void fetch(const WXDatabase::Chart& ch);

		static std::string epoch_to_str(time_t epoch);
		static std::string level_to_str(unsigned int level);

	protected:
		typedef enum {
			charttype_expert,
			charttype_profilegraph,
			charttype_windtempqff,
			charttype_relativehumidity,
			charttype_clouddensity,
			charttype_freezingclouddensity,
			charttype_freezinglevel,
			charttype_geopotentialheight,
			charttype_surfacetemperature,
			charttype_preciptype,
			charttype_accumulatedprecip,
			charttype_totalcloudcoverlow,
			charttype_totalcloudcovermid,
			charttype_totalcloudcoverhigh,
			charttype_cloudtopaltlow,
			charttype_cloudtopaltmid,
			charttype_cloudtopalthigh,
			charttype_cape,
			charttype_dvsi
		} charttype_t;

		class EpochModelColumns : public Gtk::TreeModelColumnRecord {
		public:
			EpochModelColumns(void);
			Gtk::TreeModelColumn<Glib::ustring> m_name;
			Gtk::TreeModelColumn<time_t> m_epoch;
		};

		class LevelModelColumns : public Gtk::TreeModelColumnRecord {
		public:
			LevelModelColumns(void);
			Gtk::TreeModelColumn<Glib::ustring> m_name;
			Gtk::TreeModelColumn<unsigned int> m_level;
			Gtk::TreeModelColumn<charttype_t> m_charttype;
		};

		class LevelChartType {
		public:
			LevelChartType(unsigned int level = unsellevel, charttype_t ct = charttype_expert);
			bool operator<(const LevelChartType& lc) const;
			unsigned int get_level(void) const { return m_level; }
			charttype_t get_charttype(void) const { return m_charttype; }

		protected:
			unsigned int m_level;
			charttype_t m_charttype;
		};

		typedef std::set<time_t> epochselection_t;
		typedef std::set<LevelChartType> levelselection_t;

		typedef struct {
			const char *chart_name;
			charttype_t chart_type;
			unsigned int chart_level;
		} chartsnolevel_t;

		static const chartsnolevel_t charttypes_nolevel[];

		typedef struct {
			const char *chart_name;
			charttype_t chart_type;
		} charts_t;

		static const charts_t charttypes_level[];

		Gtk::Button *m_nwxchartbuttonclose;
		Gtk::Button *m_nwxchartbuttonfetch;
		Gtk::TreeView *m_nwxcharttreeviewepochs;
		Gtk::TreeView *m_nwxcharttreeviewlevels;
		Gtk::Frame *m_nwxchartframechartsettings;
		Gtk::Frame *m_nwxchartframepgsettings;
		Gtk::ComboBox *m_nwxchartcomboboxpreciptype;
		Gtk::CheckButton *m_nwxchartcheckbuttonlatlongrid;
		Gtk::CheckButton *m_nwxchartcheckbuttoncoastlines;
		Gtk::CheckButton *m_nwxchartcheckbuttonwindbarb;
		Gtk::CheckButton *m_nwxchartcheckbuttonfrzlevel;
		Gtk::CheckButton *m_nwxchartcheckbuttonqff;
		Gtk::ComboBox *m_nwxchartcomboboxgeopotential;
		Gtk::ComboBox *m_nwxchartcomboboxtemp;
		Gtk::ComboBox *m_nwxchartcomboboxrelhumidity;
		Gtk::CheckButton *m_nwxchartcheckbuttoncape;
		Gtk::CheckButton *m_nwxchartcheckbuttonaccumprecip;
		Gtk::ComboBox *m_nwxchartcomboboxtcc;
		Gtk::CheckButton *m_nwxchartcheckbuttondvsi;
		Gtk::ComboBox *m_nwxchartcomboboxcloudtop;
		Gtk::ComboBox *m_nwxchartcomboboxclouddensity;
		Gtk::CheckButton *m_nwxchartcheckbuttonpgterrain;
		Gtk::CheckButton *m_nwxchartcheckbuttonpgflightpath;
		Gtk::CheckButton *m_nwxchartcheckbuttonpgweather;
		Gtk::CheckButton *m_nwxchartcheckbuttonpgsvg;

		NWXWeather m_nwxweather;
		EpochModelColumns m_epochcolumns;
		Glib::RefPtr<Gtk::ListStore> m_epochstore;
		LevelModelColumns m_levelcolumns;
		Glib::RefPtr<Gtk::TreeStore> m_levelstore;
		Sensors *m_sensors;
		typedef std::list<WXDatabase::Chart> fetchqueue_t;
		fetchqueue_t m_fetchqueue;
		sigc::slot<void,const WXDatabase::Chart&> m_cb;
		unsigned int m_width;
		unsigned int m_height;
		epochselection_t m_selepoch;
		levelselection_t m_sellevel;
		epochselection_t m_lastselepoch;
		levelselection_t m_lastsellevel;

		void close_clicked(void);
		void fetch_clicked(void);
		void fetch_queuehead(void);
                void selection_changed(void);
		bool level_select_function(const Glib::RefPtr<Gtk::TreeModel>& model, const Gtk::TreeModel::Path& path, bool currently_selected);
		void epochs_and_levels_cb(const NWXWeather::EpochsLevels& el);
		void chart_cb(const std::string& pix, const std::string& mimetype, time_t epoch, unsigned int level);
	};

	class WaypointAirportModelColumns : public Gtk::TreeModelColumnRecord {
	public:
		typedef AirportsDb::Airport element_t;
		typedef Cairo::RefPtr<element_t> element_ptr_t;
		static const unsigned int noid = ~0U;
		WaypointAirportModelColumns(void);
		Gtk::TreeModelColumn<Glib::ustring> m_icao;
		Gtk::TreeModelColumn<Glib::ustring> m_name;
		Gtk::TreeModelColumn<Glib::ustring> m_type;
		Gtk::TreeModelColumn<Glib::ustring> m_elev;
		Gtk::TreeModelColumn<Glib::ustring> m_rwylength;
		Gtk::TreeModelColumn<Glib::ustring> m_rwywidth;
		Gtk::TreeModelColumn<Glib::ustring> m_rwyident;
		Gtk::TreeModelColumn<Glib::ustring> m_rwysfc;
		Gtk::TreeModelColumn<Glib::ustring> m_comm;
		Gtk::TreeModelColumn<Glib::ustring> m_dist;
		Gtk::TreeModelColumn<unsigned int> m_routeid;
		Gtk::TreeModelColumn<unsigned int> m_routepointid;
		Gtk::TreeModelColumn<Pango::Style> m_style;
		Gtk::TreeModelColumn<Gdk::Color> m_color;
		Gtk::TreeModelColumn<element_ptr_t> m_element;
	};

	class WaypointAirportRunwayModelColumns : public Gtk::TreeModelColumnRecord {
	public:
		WaypointAirportRunwayModelColumns(void);
		Gtk::TreeModelColumn<Glib::ustring> m_ident;
		Gtk::TreeModelColumn<Glib::ustring> m_hdg;
		Gtk::TreeModelColumn<Glib::ustring> m_length;
		Gtk::TreeModelColumn<Glib::ustring> m_width;
		Gtk::TreeModelColumn<Glib::ustring> m_tda;
		Gtk::TreeModelColumn<Glib::ustring> m_lda;
		Gtk::TreeModelColumn<Glib::ustring> m_elev;
		Gtk::TreeModelColumn<Glib::ustring> m_surface;
		Gtk::TreeModelColumn<Glib::ustring> m_light;
		Gtk::TreeModelColumn<Gdk::Color> m_color;
	};

	class WaypointAirportHeliportModelColumns : public Gtk::TreeModelColumnRecord {
	public:
		WaypointAirportHeliportModelColumns(void);
		Gtk::TreeModelColumn<Glib::ustring> m_ident;
		Gtk::TreeModelColumn<Glib::ustring> m_hdg;
		Gtk::TreeModelColumn<Glib::ustring> m_length;
		Gtk::TreeModelColumn<Glib::ustring> m_width;
		Gtk::TreeModelColumn<Glib::ustring> m_elev;
	};

	class WaypointAirportCommModelColumns : public Gtk::TreeModelColumnRecord {
	public:
		WaypointAirportCommModelColumns(void);
		Gtk::TreeModelColumn<Glib::ustring> m_name;
		Gtk::TreeModelColumn<Glib::ustring> m_freq0;
		Gtk::TreeModelColumn<Glib::ustring> m_freq1;
		Gtk::TreeModelColumn<Glib::ustring> m_freq2;
		Gtk::TreeModelColumn<Glib::ustring> m_freq3;
		Gtk::TreeModelColumn<Glib::ustring> m_freq4;
		Gtk::TreeModelColumn<Glib::ustring> m_sector;
		Gtk::TreeModelColumn<Glib::ustring> m_oprhours;
	};

	class WaypointAirportFASModelColumns : public Gtk::TreeModelColumnRecord {
	public:
		WaypointAirportFASModelColumns(void);
		Gtk::TreeModelColumn<Glib::ustring> m_ident;
		Gtk::TreeModelColumn<Glib::ustring> m_runway;
		Gtk::TreeModelColumn<Glib::ustring> m_route;
		Gtk::TreeModelColumn<Glib::ustring> m_glide;
		Gtk::TreeModelColumn<Glib::ustring> m_ftpalt;
		Gtk::TreeModelColumn<Glib::ustring> m_tch;
	};

	class WaypointNavaidModelColumns : public Gtk::TreeModelColumnRecord {
	public:
		typedef NavaidsDb::Navaid element_t;
		WaypointNavaidModelColumns(void);
		Gtk::TreeModelColumn<Glib::ustring> m_icao;
		Gtk::TreeModelColumn<Glib::ustring> m_name;
		Gtk::TreeModelColumn<Glib::ustring> m_type;
		Gtk::TreeModelColumn<Glib::ustring> m_elev;
		Gtk::TreeModelColumn<Glib::ustring> m_freq;
		Gtk::TreeModelColumn<Glib::ustring> m_range;
		Gtk::TreeModelColumn<Glib::ustring> m_dist;
		Gtk::TreeModelColumn<element_t> m_element;
	};

	class WaypointIntersectionModelColumns : public Gtk::TreeModelColumnRecord {
	public:
		typedef WaypointsDb::Waypoint element_t;
		WaypointIntersectionModelColumns(void);
		Gtk::TreeModelColumn<Glib::ustring> m_name;
		Gtk::TreeModelColumn<Glib::ustring> m_usage;
		Gtk::TreeModelColumn<Glib::ustring> m_dist;
		Gtk::TreeModelColumn<element_t> m_element;
	};

	class WaypointAirwayModelColumns : public Gtk::TreeModelColumnRecord {
	public:
		typedef AirwaysDb::Airway element_t;
		WaypointAirwayModelColumns(void);
		Gtk::TreeModelColumn<Glib::ustring> m_name;
		Gtk::TreeModelColumn<Glib::ustring> m_fromname;
		Gtk::TreeModelColumn<Glib::ustring> m_toname;
		Gtk::TreeModelColumn<Glib::ustring> m_basealt;
		Gtk::TreeModelColumn<Glib::ustring> m_topalt;
		Gtk::TreeModelColumn<Glib::ustring> m_terrainelev;
		Gtk::TreeModelColumn<Glib::ustring> m_corridor5elev;
		Gtk::TreeModelColumn<Glib::ustring> m_type;
		Gtk::TreeModelColumn<Glib::ustring> m_dist;
		Gtk::TreeModelColumn<element_t> m_element;
	};

	class WaypointAirspaceModelColumns : public Gtk::TreeModelColumnRecord {
	public:
		typedef AirspacesDb::Airspace element_t;
		WaypointAirspaceModelColumns(void);
		Gtk::TreeModelColumn<Glib::ustring> m_icao;
		Gtk::TreeModelColumn<Glib::ustring> m_name;
		Gtk::TreeModelColumn<Glib::ustring> m_class;
		Gtk::TreeModelColumn<Glib::ustring> m_lwralt;
		Gtk::TreeModelColumn<Glib::ustring> m_upralt;
		Gtk::TreeModelColumn<Glib::ustring> m_comm;
		Gtk::TreeModelColumn<Glib::ustring> m_comm0;
		Gtk::TreeModelColumn<Glib::ustring> m_comm1;
		Gtk::TreeModelColumn<Glib::ustring> m_dist;
		Gtk::TreeModelColumn<element_t> m_element;
	};

	class WaypointMapelementModelColumns : public Gtk::TreeModelColumnRecord {
	public:
		typedef MapelementsDb::Mapelement element_t;
		WaypointMapelementModelColumns(void);
		Gtk::TreeModelColumn<Glib::ustring> m_name;
		Gtk::TreeModelColumn<Glib::ustring> m_type;
		Gtk::TreeModelColumn<Glib::ustring> m_dist;
		Gtk::TreeModelColumn<element_t> m_element;
	};

	class WaypointFPlanModelColumns : public Gtk::TreeModelColumnRecord {
	public:
		typedef FPlanWaypoint element_t;
		WaypointFPlanModelColumns(void);
		Gtk::TreeModelColumn<Glib::ustring> m_icao;
		Gtk::TreeModelColumn<Glib::ustring> m_name;
		Gtk::TreeModelColumn<Glib::ustring> m_altitude;
		Gtk::TreeModelColumn<Glib::ustring> m_freq;
		Gtk::TreeModelColumn<Glib::ustring> m_type;
		Gtk::TreeModelColumn<Glib::ustring> m_dist;
		Gtk::TreeModelColumn<element_t> m_element;
	};

	class RouteListModelColumns : public Gtk::TreeModelColumnRecord {
	public:
		static const unsigned int nowptnr = ~0U;
		RouteListModelColumns(void);
		Gtk::TreeModelColumn<Glib::ustring> m_fromicao;
		Gtk::TreeModelColumn<Glib::ustring> m_fromname;
		Gtk::TreeModelColumn<Glib::ustring> m_toicao;
		Gtk::TreeModelColumn<Glib::ustring> m_toname;
		Gtk::TreeModelColumn<float> m_length;
		Gtk::TreeModelColumn<float> m_dist;
		Gtk::TreeModelColumn<Pango::Style> m_style;
		Gtk::TreeModelColumn<FPlanRoute::id_t> m_id;
		Gtk::TreeModelColumn<unsigned int> m_wptnr;
		Gtk::TreeModelColumn<Point> m_coord;
	};

	class WaypointListModelColumns : public Gtk::TreeModelColumnRecord {
	public:
		WaypointListModelColumns(void);
		Gtk::TreeModelColumn<Glib::ustring> m_icao;
		Gtk::TreeModelColumn<Glib::ustring> m_name;
		Gtk::TreeModelColumn<Glib::ustring> m_freq;
		Gtk::TreeModelColumn<Glib::ustring> m_time;
		Gtk::TreeModelColumn<Glib::ustring> m_lon;
		Gtk::TreeModelColumn<Glib::ustring> m_lat;
		Gtk::TreeModelColumn<Glib::ustring> m_alt;
		Gtk::TreeModelColumn<Glib::ustring> m_mt;
		Gtk::TreeModelColumn<Glib::ustring> m_tt;
		Gtk::TreeModelColumn<Glib::ustring> m_dist;
		Gtk::TreeModelColumn<Glib::ustring> m_vertspeed;
		Gtk::TreeModelColumn<Glib::ustring> m_mh;
		Gtk::TreeModelColumn<Glib::ustring> m_tas;
		Gtk::TreeModelColumn<Glib::ustring> m_truealt;
		Gtk::TreeModelColumn<Glib::ustring> m_power;
		Gtk::TreeModelColumn<Glib::ustring> m_fuel;
		Gtk::TreeModelColumn<Glib::ustring> m_pathname;
		Gtk::TreeModelColumn<Glib::ustring> m_wind;
		Gtk::TreeModelColumn<Glib::ustring> m_qff;
		Gtk::TreeModelColumn<Glib::ustring> m_oat;
		Gtk::TreeModelColumn<Point> m_point;
		Gtk::TreeModelColumn<unsigned int> m_nr;
		// render style
		Gtk::TreeModelColumn<int> m_weight;
		Gtk::TreeModelColumn<Pango::Style> m_style;
		Gtk::TreeModelColumn<Pango::Style> m_time_style;
		Gtk::TreeModelColumn<Gdk::Color> m_time_color;
	};

	class WaypointListPathCodeModelColumns : public Gtk::TreeModelColumnRecord {
	public:
		WaypointListPathCodeModelColumns(void);
		Gtk::TreeModelColumn<Glib::ustring> m_name;
		Gtk::TreeModelColumn<FPlanWaypoint::pathcode_t> m_id;
	};

	class WaypointListAltUnitModelColumns : public Gtk::TreeModelColumnRecord {
	public:
		WaypointListAltUnitModelColumns(void);
		Gtk::TreeModelColumn<Glib::ustring> m_name;
		Gtk::TreeModelColumn<unsigned int> m_id;
	};

	class WaypointListTypeodelColumns : public Gtk::TreeModelColumnRecord {
	public:
		WaypointListTypeodelColumns(void);
		Gtk::TreeModelColumn<Glib::ustring> m_name;
		Gtk::TreeModelColumn<FPlanWaypoint::type_t> m_type;
		Gtk::TreeModelColumn<FPlanWaypoint::navaid_type_t> m_navaid_type;
	};

	class GPSImportWaypointListModelColumns : public Gtk::TreeModelColumnRecord {
	public:
		GPSImportWaypointListModelColumns(void);
		Gtk::TreeModelColumn<Glib::ustring> m_icao;
		Gtk::TreeModelColumn<Glib::ustring> m_name;
		Gtk::TreeModelColumn<Glib::ustring> m_freq;
		Gtk::TreeModelColumn<Glib::ustring> m_lon;
		Gtk::TreeModelColumn<Glib::ustring> m_lat;
		Gtk::TreeModelColumn<Glib::ustring> m_mt;
		Gtk::TreeModelColumn<Glib::ustring> m_tt;
		Gtk::TreeModelColumn<Glib::ustring> m_dist;
		Gtk::TreeModelColumn<unsigned int> m_nr;
		// render style
		Gtk::TreeModelColumn<int> m_weight;
		Gtk::TreeModelColumn<Pango::Style> m_style;
	};

	class MetarTafModelColumns : public Gtk::TreeModelColumnRecord {
	public:
		MetarTafModelColumns(void);
		Gtk::TreeModelColumn<Glib::ustring> m_icao;
		Gtk::TreeModelColumn<Glib::ustring> m_type;
		Gtk::TreeModelColumn<Glib::ustring> m_issue;
		Gtk::TreeModelColumn<Glib::ustring> m_message;
		// render style
		Gtk::TreeModelColumn<int> m_weight;
		Gtk::TreeModelColumn<Pango::Style> m_style;
		Gtk::TreeModelColumn<Gdk::Color> m_color;
	};

	class WxChartListModelColumns : public Gtk::TreeModelColumnRecord {
	public:
		WxChartListModelColumns(void);
		Gtk::TreeModelColumn<Glib::ustring> m_departure;
		Gtk::TreeModelColumn<Glib::ustring> m_destination;
		Gtk::TreeModelColumn<Glib::ustring> m_description;
		Gtk::TreeModelColumn<Glib::ustring> m_createtime;
		Gtk::TreeModelColumn<Glib::ustring> m_epoch;
		Gtk::TreeModelColumn<Glib::ustring> m_level;
		Gtk::TreeModelColumn<uint64_t> m_id;
		Gtk::TreeModelColumn<time_t> m_nepoch;
		Gtk::TreeModelColumn<unsigned int> m_nlevel;
		Gtk::TreeModelColumn<unsigned int> m_ntype;
	};

#ifdef HAVE_EVINCE
	class Document {
	public:
		typedef std::set<Glib::ustring> token_t;
		typedef std::vector<Glib::ustring> tokenvec_t;

		Document(const Glib::ustring& filename = "");
		~Document();
		void set_info(EvDocument *doc);
		bool verify(void);
		bool is_deleted(void) const { return m_deleted; }
		void mark_deleted(void);
		void undelete(void);

		const Glib::ustring& get_pathname(void) const { return m_filename; }
		Glib::ustring get_filename(void) const;
		Glib::ustring get_uri(void) const;
		const token_t& get_token(void) const { return m_token; }
		int compare(const Document& x) const;
		bool operator<(const Document& x) const { return compare(x) < 0; }
		bool operator>(const Document& x) const { return compare(x) > 0; }
		bool operator==(const Document& x) const { return compare(x) == 0; }
		bool operator!=(const Document& x) const { return compare(x) != 0; }
		template<typename T> void add_token(T b, T e);
		bool has_token(const Glib::ustring& x) const;
		template<typename T> bool has_any_token(T b, T e) const;

		static tokenvec_t split_filename(const Glib::ustring& fn);
		static tokenvec_t split_token(const Glib::ustring& fn);
		static void insert_token(token_t& tok, const Glib::ustring& fn);
		static bool has_globchars(const Glib::ustring& fn);
		static void filter_token(token_t& tok, unsigned int minlength);
		void filter_token(unsigned int minlength) { filter_token(m_token, minlength); }
		static void filter_token_generic(token_t& tok, const char * const *wordlist = 0);
		void filter_token_generic(const char * const *wordlist = 0) { filter_token_generic(m_token, wordlist); }

		const Glib::TimeVal& get_modification_time(void) const { return m_modtime; }
		void set_modification_time(const Glib::TimeVal& tv) { m_modtime = tv; }
		const Glib::ustring& get_title(void) const { return m_title; }
		const Glib::ustring& get_format(void) const { return m_format; }
		const Glib::ustring& get_author(void) const { return m_author; }
		const Glib::ustring& get_subject(void) const { return m_subject; }
		const Glib::ustring& get_keywords(void) const { return m_keywords; }
		const Glib::ustring& get_creator(void) const { return m_creator; }
		const Glib::ustring& get_producer(void) const { return m_producer; }
		const Glib::ustring& get_linearized(void) const { return m_linearized; }
		const Glib::ustring& get_security(void) const { return m_security; }
		GTime get_creation_date(void) const { return m_creation_date; }
		GTime get_modified_date(void) const { return m_modified_date; }
		int get_n_pages(void) const { return m_n_pages; }

		void save_view(EvDocumentModel *model);
		void restore_view(EvDocumentModel *model) const;

	protected:
		Glib::ustring m_filename;
		token_t m_token;
		Glib::ustring m_title;
		Glib::ustring m_format;
		Glib::ustring m_author;
		Glib::ustring m_subject;
		Glib::ustring m_keywords;
		Glib::ustring m_creator;
		Glib::ustring m_producer;
		Glib::ustring m_linearized;
		Glib::ustring m_security;
		Glib::TimeVal m_modtime;
		GTime m_creation_date;
		GTime m_modified_date;
		int m_n_pages;
		gint m_page;
		gint m_rotation;
		gdouble m_scale;
		EvSizingMode m_sizingmode;
		bool m_deleted;
	};

	class DocumentsDirectoryModelColumns : public Gtk::TreeModelColumnRecord {
	public:
		DocumentsDirectoryModelColumns(void);
		Gtk::TreeModelColumn<Glib::ustring> m_text;
		Gtk::TreeModelColumn<int> m_weight;
		Gtk::TreeModelColumn<Pango::Style> m_style;
		Gtk::TreeModelColumn<Document *> m_document;
	};
#endif

	class PerfUnitsModelColumns : public Gtk::TreeModelColumnRecord {
	public:
		PerfUnitsModelColumns(void);
		Gtk::TreeModelColumn<Glib::ustring> m_text;
		Gtk::TreeModelColumn<double> m_gain;
		Gtk::TreeModelColumn<double> m_offset;
	};

	class PerfWBModelColumns : public Gtk::TreeModelColumnRecord {
	public:
		PerfWBModelColumns(void);
		Gtk::TreeModelColumn<Glib::ustring> m_text;
		Gtk::TreeModelColumn<double> m_value;
		Gtk::TreeModelColumn<bool> m_valueeditable;
		Gtk::TreeModelColumn<Glib::ustring> m_unit;
		Gtk::TreeModelColumn<Glib::RefPtr<Gtk::TreeModel> > m_model;
		Gtk::TreeModelColumn<bool> m_modelvisible;
		Gtk::TreeModelColumn<double> m_mass;
		Gtk::TreeModelColumn<double> m_minmass;
		Gtk::TreeModelColumn<double> m_maxmass;
		Gtk::TreeModelColumn<double> m_arm;
		Gtk::TreeModelColumn<double> m_moment;
		Gtk::TreeModelColumn<Pango::Style> m_style;
		Gtk::TreeModelColumn<unsigned int> m_nr;
	};

	class PerfResultModelColumns : public Gtk::TreeModelColumnRecord {
	public:
		PerfResultModelColumns(void);
		Gtk::TreeModelColumn<Glib::ustring> m_text;
		Gtk::TreeModelColumn<Glib::ustring> m_value1;
		Gtk::TreeModelColumn<Glib::ustring> m_value2;
	};

	class PerfWBDrawingArea : public Gtk::DrawingArea {
	public:
		explicit PerfWBDrawingArea(BaseObjectType *castitem, const Glib::RefPtr<Builder>& refxml);
		explicit PerfWBDrawingArea(void);
		virtual ~PerfWBDrawingArea();

		void set_wb(const Aircraft::WeightBalance *wb);

		void clear_curves(void);
		void add_curve(const Glib::ustring& name, const std::vector<Aircraft::WeightBalance::Envelope::Point>& path);
		void add_curve(const Glib::ustring& name, const Aircraft::WeightBalance::Envelope::Point& p0,
			       const Aircraft::WeightBalance::Envelope::Point& p1);
		void add_curve(const Glib::ustring& name, double arm0, double mass0, double arm1, double mass1);
		

	protected:
		const Aircraft::WeightBalance *m_wb;
		typedef std::vector<Aircraft::WeightBalance::Envelope::Point> path_t;
		typedef std::pair<Glib::ustring,path_t> curve_t;
		typedef std::vector<curve_t> curves_t;
		curves_t m_curves;

#ifdef HAVE_GTKMM2
		virtual bool on_expose_event(GdkEventExpose *event);
#endif
		virtual bool on_draw(const Cairo::RefPtr<Cairo::Context>& cr);

		static inline void set_color_envelope(const Cairo::RefPtr<Cairo::Context>& cr, unsigned int nr, bool fill) {
			cr->set_source_rgba(0.0, std::min(nr, 2U) * 0.5, 1.0 - 0.5 * (nr >= 3), fill ? 0.25 : 1.0);
		}

		static inline void set_color_curve(const Cairo::RefPtr<Cairo::Context>& cr, unsigned int nr) {
			cr->set_source_rgb(1.0, std::min(nr, 2U) * 0.5, 0.0);
		}

		static inline void set_color_background(const Cairo::RefPtr<Cairo::Context>& cr) {
			cr->set_source_rgb(1.0, 1.0, 1.0);
		}

		static inline void set_color_text(const Cairo::RefPtr<Cairo::Context>& cr) {
			cr->set_source_rgb(0.0, 0.0, 0.0);
		}
	};

	class SunriseSunsetWaypointListModelColumns : public Gtk::TreeModelColumnRecord {
	public:
		SunriseSunsetWaypointListModelColumns(void);
		Gtk::TreeModelColumn<Glib::ustring> m_icao;
		Gtk::TreeModelColumn<Glib::ustring> m_name;
		Gtk::TreeModelColumn<Glib::ustring> m_sunrise;
		Gtk::TreeModelColumn<Glib::ustring> m_sunset;
		Gtk::TreeModelColumn<Glib::ustring> m_lon;
		Gtk::TreeModelColumn<Glib::ustring> m_lat;
		Gtk::TreeModelColumn<Glib::ustring> m_mt;
		Gtk::TreeModelColumn<Glib::ustring> m_tt;
		Gtk::TreeModelColumn<Glib::ustring> m_dist;
		Gtk::TreeModelColumn<Point> m_point;
		Gtk::TreeModelColumn<unsigned int> m_nr;
	};

	class EUOPS1TDZListModelColumns : public Gtk::TreeModelColumnRecord {
	public:
		EUOPS1TDZListModelColumns(void);
		Gtk::TreeModelColumn<Glib::ustring> m_text;
		Gtk::TreeModelColumn<int> m_elev;
	};

	class LogModelColumns : public Gtk::TreeModelColumnRecord {
	public:
		LogModelColumns(void);
		Gtk::TreeModelColumn<Glib::TimeVal> m_time;
		Gtk::TreeModelColumn<Sensors::loglevel_t> m_loglevel;
		Gtk::TreeModelColumn<Gdk::Color> m_color;
		Gtk::TreeModelColumn<Glib::ustring> m_text;
	};

	class GRIB2LayerModelColumns : public Gtk::TreeModelColumnRecord {
	public:
		class LayerPtr {
		public:
			typedef Glib::RefPtr<GRIB2::Layer> ptr_t;
		        LayerPtr(const ptr_t& p = ptr_t()) : m_ptr(p) {}
			ptr_t& get(void) { return m_ptr; }
			const ptr_t& get(void) const { return m_ptr; }
		protected:
			ptr_t m_ptr;
		};

		GRIB2LayerModelColumns(void);
		Gtk::TreeModelColumn<Glib::ustring> m_text;
		Gtk::TreeModelColumn<LayerPtr> m_layer;
	};

protected:
	Engine *m_engine;
	Sensors *m_sensors;
	WXDatabase m_wxdb;
	// Screen Saver Disable
	sigc::connection m_screensaverheartbeat;
	typedef enum {
		screensaver_default,
		screensaver_unknown,
		screensaver_xss,
		screensaver_dpmsoff,
		screensaver_dpms
	} screensaver_t;
	screensaver_t m_screensaver;
	// Other Dialog Windows
	AltDialog *m_altdialog;
	HSIDialog *m_hsidialog;
	MapDialog *m_mapdialog;
	MapTileDownloaderDialog *m_maptiledialog;
	TextEntryDialog *m_textentrydialog;
	sigc::connection m_textentrydialogdoneconn;
	KeyboardDialog *m_keyboarddialog;
	bool m_onscreenkeyboard;
	CoordDialog *m_coorddialog;
	SuspendDialog *m_suspenddialog;
	FPLAtmosphereDialog *m_fplatmodialog;
	FPLVerificationDialog *m_fplverifdialog;
	FPLAutoRouteDialog *m_fplautoroutedialog;
	NWXChartDialog *m_nwxchartdialog;
	// GUI elements
	sigc::connection m_sensorschange;
	Gtk::Notebook *m_mainnotebook;
	Gtk::VBox *m_mainvbox;
	Softkeys m_softkeys;
	sigc::connection m_softkeysconn;
	NavArea m_navarea;
	Gtk::Frame *m_sensorcfgframe;
	MagCalibDialog *m_magcalibdialog;
	// Waypoint Page
	Gtk::Label *m_wptframelabel;
	Gtk::TreeView *m_wpttreeview;
	Gtk::ToggleButton *m_wpttogglebuttonnearest;
	Gtk::ToggleButton *m_wpttogglebuttonfind;
	Gtk::Button *m_wptbuttoninsert;
	Gtk::Button *m_wptbuttondirectto;
	Gtk::ToggleButton *m_wpttogglebuttonbrg2;
	Gtk::ToggleButton *m_wpttogglebuttonmap;
	VectorMapArea *m_wptdrawingarea;
	Gtk::Notebook *m_wptnotebook;
	Gtk::Entry *m_wptarpticaoentry;
	Gtk::Entry *m_wptarptnameentry;
	Gtk::Entry *m_wptarptlatentry;
	Gtk::Entry *m_wptarptlonentry;
	Gtk::Entry *m_wptarpteleventry;
	Gtk::Entry *m_wptarpttypeentry;
	Gtk::Notebook *m_wptarptnotebook;
	Gtk::TextView *m_wptarptremarktextview;
	Gtk::TreeView *m_wptarptrwytreeview;
	Gtk::TreeView *m_wptarpthelitreeview;
	Gtk::TreeView *m_wptarptcommtreeview;
	Gtk::TreeView *m_wptarptfastreeview;
	Gtk::Entry *m_wptnavaidicaoentry;
	Gtk::Entry *m_wptnavaidnameentry;
	Gtk::Entry *m_wptnavaidlatentry;
	Gtk::Entry *m_wptnavaidlonentry;
	Gtk::Entry *m_wptnavaideleventry;
	Gtk::Entry *m_wptnavaidtypeentry;
	Gtk::Entry *m_wptnavaidfreqentry;
	Gtk::Entry *m_wptnavaidrangeentry;
	Gtk::CheckButton *m_wptnavaidinservicecheckbutton;
	Gtk::Entry *m_wptinticaoentry;
	Gtk::Entry *m_wptintnameentry;
	Gtk::Entry *m_wptintlatentry;
	Gtk::Entry *m_wptintlonentry;
	Gtk::Entry *m_wptintusageentry;
	Gtk::Entry *m_wptawynameentry;
	Gtk::Entry *m_wptawybeginlonentry;
	Gtk::Entry *m_wptawybeginlatentry;
	Gtk::Entry *m_wptawyendlonentry;
	Gtk::Entry *m_wptawyendlatentry;
	Gtk::Entry *m_wptawybeginnameentry;
	Gtk::Entry *m_wptawyendnameentry;
	Gtk::Entry *m_wptawybasealtentry;
	Gtk::Entry *m_wptawytopaltentry;
	Gtk::Entry *m_wptawyteleventry;
	Gtk::Entry *m_wptawyt5eleventry;
	Gtk::Entry *m_wptawytypeentry;
	Gtk::Entry *m_wptaspcicaoentry;
	Gtk::Entry *m_wptaspcnameentry;
	Gtk::Entry *m_wptaspcidententry;
	Gtk::Entry *m_wptaspcclassexceptentry;
	Gtk::Entry *m_wptaspcclassentry;
	Gtk::Entry *m_wptaspcfromaltentry;
	Gtk::Entry *m_wptaspctoaltentry;
	Gtk::Entry *m_wptaspccommnameentry;
	Gtk::Entry *m_wptaspccomm0entry;
	Gtk::Entry *m_wptaspccomm1entry;
	Gtk::Notebook *m_wptaspcnotebook;
	Gtk::TextView *m_wptaspceffectivetextview;
	Gtk::TextView *m_wptaspcnotetextview;
	Gtk::Entry *m_wptmapelnameentry;
	Gtk::Entry *m_wptmapeltypeentry;
        Gtk::Entry *m_wptfpldbicaoentry;
        Gtk::Entry *m_wptfpldbnameentry;
        Gtk::Entry *m_wptfpldblatentry;
        Gtk::Entry *m_wptfpldblonentry;
        Gtk::Entry *m_wptfpldbaltentry;
        Gtk::Entry *m_wptfpldbfreqentry;
        Gtk::Entry *m_wptfpldbtypeentry;
	Gtk::TextView *m_wptfpldbnotetextview;
	Gtk::Entry *m_wptcoordlatentry;
	Gtk::Entry *m_wptcoordlonentry;
	Gtk::Entry *m_wpttexttextentry;
	// Flight Plan Page
	Gtk::TreeView *m_fpltreeview;
	Gtk::Button *m_fplinsertbutton;
	Gtk::Button *m_fplinserttxtbutton;
	Gtk::Button *m_fplduplicatebutton;
	Gtk::Button *m_fplmoveupbutton;
	Gtk::Button *m_fplmovedownbutton;
	Gtk::Button *m_fpldeletebutton;
	Gtk::Button *m_fplstraightenbutton;
	Gtk::Button *m_fpldirecttobutton;
	Gtk::ToggleButton *m_fplbrg2togglebutton;
	VectorMapArea *m_fpldrawingarea;
	Gtk::Notebook *m_fplnotebook;
	Gtk::Entry *m_fplicaoentry;
	Gtk::Entry *m_fplnameentry;
	Gtk::ComboBox *m_fplpathcodecombobox;
	Gtk::Entry *m_fplpathnameentry;
	Gtk::Entry *m_fplcoordentry;
	Gtk::Button *m_fplcoordeditbutton;
	Gtk::Entry *m_fplfreqentry;
	Gtk::Label *m_fplfrequnitlabel;
	Gtk::Entry *m_fplaltentry;
	Gtk::ComboBox *m_fplaltunitcombobox;
	Gtk::Button *m_fpllevelupbutton;
	Gtk::Button *m_fplleveldownbutton;
	Gtk::Button *m_fplsemicircularbutton;
	Gtk::CheckButton *m_fplifrcheckbutton;
	Gtk::ComboBox *m_fpltypecombobox;
	Gtk::TextView *m_fplwptnotestextview;
	Gtk::TextView *m_fplnotestextview;
	Gtk::SpinButton *m_fplwinddirspinbutton;
	Gtk::SpinButton *m_fplwindspeedspinbutton;
	Gtk::SpinButton *m_fplqffspinbutton;
	Gtk::SpinButton *m_fploatspinbutton;
	// Flight Plan Database Page
	Gtk::TreeView *m_fpldbtreeview;
	Gtk::Button *m_fpldbnewbutton;
	Gtk::Button *m_fpldbloadbutton;
	Gtk::Button *m_fpldbduplicatebutton;
	Gtk::Button *m_fpldbdeletebutton;
	Gtk::Button *m_fpldbreversebutton;
	Gtk::Button *m_fpldbimportbutton;
	Gtk::Button *m_fpldbexportbutton;
	Gtk::Button *m_fpldbdirecttobutton;
	Gtk::Window *m_fpldbcfmwindow;
	Gtk::Label *m_fpldbcfmlabel;
	Gtk::TreeView *m_fpldbcfmtreeview;
	Gtk::Button *m_fpldbcfmbuttoncancel;
	Gtk::Button *m_fpldbcfmbuttonok;
	// Flight Plan GPS Import Page
        VectorMapArea *m_fplgpsimpdrawingarea;
	Gtk::TreeView *m_fplgpsimptreeview;
	Gtk::Label *m_fplgpsimplabel;
	// Route Vertical Profile Drawing Area
	RouteProfileDrawingArea *m_vertprofiledrawingarea;
	// WX Picture Page
	Gtk::Label *m_wxpiclabel;
	SVGImage *m_wxpicimage;
	Gtk::Alignment *m_wxpicalignment;
	// METAR/TAF Page
	Gtk::TreeView *m_metartaftreeview;
	// Wx Chart List Page
	Gtk::TreeView *m_wxchartlisttreeview;
	// Documents Directory Page
	Gtk::Frame *m_docdirframe;
	Gtk::TreeView *m_docdirtreeview;
	// Documents Page
	Gtk::Label *m_docframelabel;
	Gtk::Frame *m_docframe;
	Gtk::ScrolledWindow *m_docscrolledwindow;
	// Performace Page
	Gtk::Button *m_perfbuttonfromwb;
	Gtk::Button *m_perfbuttonfromfpl;
	Gtk::SpinButton *m_perfspinbuttonsoftsurface;
	Gtk::ComboBox *m_perfcombobox[perfunits_nr];
	Gtk::SpinButton *m_perfspinbutton[2][perfval_nr];
	Gtk::Entry *m_perfentryname[2];
	Gtk::TreeView *m_perftreeview[2];
	Gtk::TreeView *m_perftreeviewwb;
        PerfWBDrawingArea *m_perfdrawingareawb;
	// Sunrise Sunset Page
	Gtk::TreeView *m_srsstreeview;
	Gtk::Calendar *m_srsscalendar;
	// EU OPS1 Minimum Calculator (on Sunrise Sunset Page)
	Gtk::ComboBox *m_euops1comboboxapproach;
	Gtk::SpinButton *m_euops1spinbuttontdzelev;
	Gtk::ComboBox *m_euops1comboboxtdzelevunit;
	Gtk::ComboBox *m_euops1comboboxtdzlist;
	Gtk::ComboBox *m_euops1comboboxocaoch;
	Gtk::SpinButton *m_euops1spinbuttonocaoch;
	Gtk::ComboBox *m_euops1comboboxocaochunit;
	Gtk::Label *m_euops1labeldamda;
	Gtk::Entry *m_euops1entrydamda;
	Gtk::Button *m_euops1buttonsetda;
	Gtk::Entry *m_euops1entryrvr;
	Gtk::Entry *m_euops1entrymetvis;
	Gtk::ComboBox *m_euops1comboboxlight;
	Gtk::ComboBox *m_euops1comboboxmetvis;
	Gtk::CheckButton *m_euops1checkbuttonrwyedgelight;
	Gtk::CheckButton *m_euops1checkbuttonrwycentreline;
	Gtk::CheckButton *m_euops1checkbuttonmultiplervr;
	Gtk::Entry *m_euops1entrytkoffrvr;
	Gtk::Entry *m_euops1entrytkoffmetvis;
	// Log Page
	Gtk::TreeView *m_logtreeview;
	// GRIB2 Page
	VectorMapArea *m_grib2drawingarea;
        Gtk::TreeView *m_grib2layertreeview;
        Gtk::Entry *m_grib2layerabbreventry;
        Gtk::Entry *m_grib2layerunitentry;
        Gtk::Entry *m_grib2layerparamidentry;
        Gtk::Entry *m_grib2layerparamentry;
        Gtk::Entry *m_grib2layerparamcatidentry;
        Gtk::Entry *m_grib2layerdisciplineidentry;
        Gtk::Entry *m_grib2layerparamcatentry;
        Gtk::Entry *m_grib2layerdisciplineentry;
        Gtk::Entry *m_grib2layerefftimeentry;
        Gtk::Entry *m_grib2layerreftimeentry;
        Gtk::Entry *m_grib2layersurface1entry;
        Gtk::Entry *m_grib2layersurface2entry;
        Gtk::Entry *m_grib2layersurface1valueentry;
        Gtk::Entry *m_grib2layersurface2valueentry;
        Gtk::Entry *m_grib2layersurface1identry;
        Gtk::Entry *m_grib2layersurface2identry;
        Gtk::Entry *m_grib2layersurface1unitentry;
        Gtk::Entry *m_grib2layersurface2unitentry;
        Gtk::Entry *m_grib2layercenteridentry;
        Gtk::Entry *m_grib2layersubcenteridentry;
        Gtk::Entry *m_grib2layerprodstatusidentry;
        Gtk::Entry *m_grib2layerdatatypeidentry;
        Gtk::Entry *m_grib2layergenprocidentry;
        Gtk::Entry *m_grib2layergenproctypidentry;
        Gtk::Entry *m_grib2layercenterentry;
        Gtk::Entry *m_grib2layersubcenterentry;
        Gtk::Entry *m_grib2layerprodstatusentry;
        Gtk::Entry *m_grib2layerdatatypeentry;
        Gtk::Entry *m_grib2layergenprocentry;
        Gtk::Entry *m_grib2layergenproctypentry;
	// Waypoint Page Variables
	Glib::Dispatcher m_querydispatch;
	Glib::RefPtr<Engine::AirportResult> m_airportquery;
	Glib::RefPtr<Engine::AirspaceResult> m_airspacequery;
	Glib::RefPtr<Engine::NavaidResult> m_navaidquery;
	Glib::RefPtr<Engine::WaypointResult> m_waypointquery;
	Glib::RefPtr<Engine::AirwayResult> m_airwayquery;
	Glib::RefPtr<Engine::MapelementResult> m_mapelementquery;
	typedef enum {
		wptpagemode_text,
		wptpagemode_nearest,
		wptpagemode_coord
	} wptpagemode_t;
	wptpagemode_t m_wptpagemode;
	Point m_wptquerypoint;
	Rect m_wptqueryrect;
	Glib::ustring m_wptquerytext;
	sigc::connection m_wptpagebuttonnearestconn;
	sigc::connection m_wptpagetogglebuttonfindconn;
	sigc::connection m_wpttreeviewselconn;
	sigc::connection m_wptmapcursorconn;
	sigc::connection m_wpttogglebuttonarptmap;
	sigc::connection m_wptbrg2conn;
	WaypointAirportModelColumns m_wptairportcolumns;
	Glib::RefPtr<Gtk::TreeStore> m_wptstore;
	WaypointAirportRunwayModelColumns m_wptairportrunwaycolumns;
	Glib::RefPtr<Gtk::ListStore> m_wptarptrwystore;
	WaypointAirportHeliportModelColumns m_wptairporthelipadcolumns;
	Glib::RefPtr<Gtk::ListStore> m_wptarpthelipadstore;
	WaypointAirportCommModelColumns m_wptairportcommcolumns;
	Glib::RefPtr<Gtk::ListStore> m_wptarptcommstore;
	WaypointAirportFASModelColumns m_wptairportfascolumns;
	Glib::RefPtr<Gtk::ListStore> m_wptarptfasstore;
	WaypointNavaidModelColumns m_wptnavaidcolumns;
	WaypointIntersectionModelColumns m_wptintersectioncolumns;
	WaypointAirwayModelColumns m_wptairwaycolumns;
	WaypointAirspaceModelColumns m_wptairspacecolumns;
	WaypointMapelementModelColumns m_wptmapelementcolumns;
	WaypointFPlanModelColumns m_wptfplancolumns;
	class WptFPlanCompare;
	// Flight Plan Page Variables
	WaypointListModelColumns m_fplcolumns;
	Glib::RefPtr<Gtk::ListStore> m_fplstore;
	WaypointListPathCodeModelColumns m_fplpathcodecolumns;
	Glib::RefPtr<Gtk::ListStore> m_fplpathcodestore;
	WaypointListAltUnitModelColumns m_fplaltunitcolumns;
	Glib::RefPtr<Gtk::ListStore> m_fplaltunitstore;
	WaypointListTypeodelColumns m_fpltypecolumns;
	Glib::RefPtr<Gtk::ListStore> m_fpltypestore;
	sigc::connection m_fplpathcodechangeconn;
	sigc::connection m_fplaltunitchangeconn;
	sigc::connection m_fpltypechangeconn;
	sigc::connection m_fplifrchangeconn;
	sigc::connection m_fplbrg2conn;
	sigc::connection m_fplselchangeconn;
	fplchg_t m_fplchg;
	class FPLQuery;
	NWXWeather m_fplnwxweather;
	ADDS m_fpladdsweather;
	// Flight Plan Database Page Variables
	RouteListModelColumns m_fpldbcolumns;
	Glib::RefPtr<Gtk::TreeStore> m_fpldbstore;
	fpldbcfmmode_t m_fpldbcfmmode;
	typedef std::set<FPlanRoute::id_t> fpldbcfmids_t;
	fpldbcfmids_t m_fpldbcfmids;
	sigc::connection m_fpldbselchangeconn;
	Gtk::FileChooserDialog m_fpldbimportfilechooser;
	Gtk::FileChooserDialog m_fpldbexportfilechooser;
	Gtk::VBox m_fpldbexportfilechooserextra;
	Gtk::Label m_fpldbexportformattext;
	Gtk::ComboBoxText m_fpldbexportformat;
	// Flight Plan GPS Import Page Variables
	GPSImportWaypointListModelColumns m_fplgpsimpcolumns;
	Glib::RefPtr<Gtk::ListStore> m_fplgpsimpstore;
	FPlanRoute m_fplgpsimproute;
	class FPLGPSImpQuery;
	sigc::connection m_fplgpsimpselchangeconn;
	// Route Vertical Profile Drawing Area
	Gtk::FileChooserDialog m_wxchartexportfilechooser;
	// METAR/TAF Page Variables
	MetarTafModelColumns m_metartafcolumns;
	Glib::RefPtr<Gtk::ListStore> m_metartafstore;
	// Wx Chart List Page Variables
	WxChartListModelColumns m_wxchartlistcolumns;
	Glib::RefPtr<Gtk::ListStore> m_wxchartliststore;
#ifdef HAVE_EVINCE
	// Documents Directory Page Variables
	DocumentsDirectoryModelColumns m_docdircolumns;
	Glib::RefPtr<Gtk::TreeStore> m_docdirstore;
	typedef std::set<Document> documents_t;
	documents_t m_documents;
	documents_t m_documentsunverified;
	sigc::connection m_docdirverifyconn;
	Glib::TimeVal m_docdirupdatetime;
	typedef std::multimap<Glib::ustring,Document *> tokendocumentmap_t;
	tokendocumentmap_t m_tokendocumentmap;
	// Document Page Variables
	EvDocumentModel *m_docevdocmodel;
	gulong m_docevdocmodel_sighandler;
	Gtk::Widget *m_docevview;
#endif
	// Performace Page Variables
	static perfunits_t prefvalueunits[perfval_nr];
	class PerfAtmo;
	PerfUnitsModelColumns m_perfunitscolumns;
	PerfWBModelColumns m_perfwbcolumns;
	PerfResultModelColumns m_perfresultcolumns;
	Glib::RefPtr<Gtk::ListStore> m_perfunitsstore[perfunits_nr];
	sigc::connection m_perfspinbuttonconn[2][perfval_nr];
	Glib::RefPtr<Gtk::TreeStore> m_perfresultstore[2];
	Glib::RefPtr<Gtk::ListStore> m_perfwbstore;
	double m_perfvalues[2][perfval_nr];
	double m_perfsoftfieldpenalty;
	sigc::connection m_perfsoftsurfaceconn;
	// Sunrise Sunset Page Variables
	SunriseSunsetWaypointListModelColumns m_srsscolumns;
	Glib::RefPtr<Gtk::ListStore> m_srssstore;
	// EU OPS1 Minimum Calculator
	EUOPS1TDZListModelColumns m_euops1tdzcolumns;
	Glib::RefPtr<Gtk::ListStore> m_euops1tdzstore;
	sigc::connection m_euops1tdzlistchange;
	sigc::connection m_euops1tdzelevchange;
	sigc::connection m_euops1tdzunitchange;
	sigc::connection m_euops1ocaochchange;
	sigc::connection m_euops1ocaochunitchange;
	double m_euops1tdzelev;
	double m_euops1ocaoch;
	double m_euops1minimum;
	static const int m_euops1systemminima[];
	static const unsigned int m_euops1apchrvr[][5];
	// Log Page Variables
	LogModelColumns m_logcolumns;
	Glib::RefPtr<Gtk::ListStore> m_logstore;
	Glib::RefPtr<Gtk::TreeModelFilter> m_logfilter;
	Sensors::loglevel_t m_logdisplaylevel;
	// GRIB2 Page Variables
	GRIB2LayerModelColumns m_grib2layercolumns;
	Glib::RefPtr<Gtk::TreeStore> m_grib2layerstore;
	sigc::connection m_grib2layerselchangeconn;

	// Menu
	unsigned int m_menuid;
	// Sensor Configuration
	typedef Glib::RefPtr<ConfigItem> sensorcfgitem_t;
	typedef std::vector<sensorcfgitem_t> sensorcfgitems_t;
	sensorcfgitems_t m_sensorcfgitems;
	sigc::connection m_sensorupd;
	sigc::connection m_sensordelayedupd;
	Glib::TimeVal m_sensorlastupd;
	Sensors::Sensor::ParamChanged m_sensorupdmask;
	bool m_admin;
};

#endif /* FLIGHTDECKWINDOW_H */
