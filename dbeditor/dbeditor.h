//
// C++ Interface: dbeditor
//
// Description: Database Editors Common Code
//
//
// Author: Thomas Sailer <t.sailer@alumni.ethz.ch>, (C) 2007, 2008, 2009, 2012, 2016
//
// Copyright: See COPYING file that comes with this distribution
//
//

#ifndef _DBEDITOR_H
#define _DBEDITOR_H

#include "sysdeps.h"

#include <vector>
#include <string>
#include <gtkmm.h>
#include <sigc++/sigc++.h>
#include <gtk/gtk.h>

#ifdef HAVE_OSSO
#include <libosso.h>
#endif
#ifdef HAVE_HILDONMM
#include <hildonmm.h>
#include <hildon-fmmm.h>
#endif

#include "dbobj.h"
#include "geom.h"
#include "maps.h"
#include "engine.h"
#include "prefs.h"

class ComboCellRenderer1 : public Gtk::CellRendererCombo {
public:
	virtual ~ComboCellRenderer1(void);
	const char *get_property_name(void) const { return m_property_name.c_str(); }
	int get_property_id(void) const { return m_property_id; }

protected:
	GParamSpec *m_property_paramspec;

	ComboCellRenderer1(const std::string& pname, int pid, Glib::ValueBase& propval);
	void construct(void);

private:
	std::string m_property_name;
	int m_property_id;
	Glib::ValueBase& m_property_value;

	static void custom_get_property_callback(GObject *object, unsigned int property_id,
						 GValue *value, GParamSpec *param_spec);
	static void custom_set_property_callback(GObject *object, unsigned int property_id,
						 const GValue *value, GParamSpec *param_spec);
};

template <class T>
class ComboCellRenderer : public ComboCellRenderer1 {
public:
	T get_value(void);
	void set_value(T val);
	sigc::signal< void, const Glib::ustring&, const Glib::ustring&, T > signal_edited(void) { return m_signal_edited; }

protected:
	class ModelColumns : public Gtk::TreeModelColumnRecord {
	public:
		ModelColumns(void);
		Gtk::TreeModelColumn<T> m_col_value;
		Gtk::TreeModelColumn<Glib::ustring> m_col_text;
	};

	ModelColumns m_model_columns;
	Glib::RefPtr<Gtk::TreeStore> m_model;

	ComboCellRenderer(const std::string& pname, int pid);
	void add_value(T val, const Glib::ustring& text);

private:
	sigc::signal< void, const Glib::ustring&, const Glib::ustring&, T > m_signal_edited;
	Glib::Value<T> m_property_value;

	void on_my_edited(const Glib::ustring& path, const Glib::ustring& new_text);
	void property_changed(void);
	void text_changed(void);
};

template <class T>
class NumericCellRenderer : public Gtk::CellRendererText {
public:
	virtual ~NumericCellRenderer(void);
	const char *get_property_name(void) const { return m_property_name.c_str(); }
	int get_property_id(void) const { return m_property_id; }
	T get_value(void);
	void set_value(T val = 0);
	sigc::signal< void, const Glib::ustring&, const Glib::ustring&, T > signal_edited(void) { return m_signal_edited; }

protected:
	NumericCellRenderer(const std::string& pname, int pid);
	virtual T to_numeric(const Glib::ustring& text) = 0;
	virtual Glib::ustring to_text(T val) = 0;

private:
	std::string m_property_name;
	int m_property_id;
	sigc::signal< void, const Glib::ustring&, const Glib::ustring&, T > m_signal_edited;
	GParamSpec *m_property_paramspec;
	Glib::Value<T> m_property_value;

	void on_my_edited(const Glib::ustring& path, const Glib::ustring& new_text);
	void property_changed(void);
	void text_changed(void);
	static void custom_get_property_callback(GObject *object, unsigned int property_id,
						 GValue *value, GParamSpec *param_spec);
	static void custom_set_property_callback(GObject *object, unsigned int property_id,
						 const GValue *value, GParamSpec *param_spec);
};

class LabelPlacementRenderer : public ComboCellRenderer<NavaidsDb::Navaid::label_placement_t> {
public:
	LabelPlacementRenderer(void);

private:
	void add_value(NavaidsDb::Navaid::label_placement_t val) {
		ComboCellRenderer<NavaidsDb::Navaid::label_placement_t>::add_value(val, NavaidsDb::Navaid::get_label_placement_name(val));
	}
};

class NavaidTypeRenderer : public ComboCellRenderer<NavaidsDb::Navaid::navaid_type_t> {
public:
	NavaidTypeRenderer(void);

private:
	void add_value(NavaidsDb::Navaid::navaid_type_t val) {
		ComboCellRenderer<NavaidsDb::Navaid::navaid_type_t>::add_value(val, NavaidsDb::Navaid::get_navaid_typename(val));
	}
};

class WaypointUsageRenderer : public ComboCellRenderer<char> {
public:
	WaypointUsageRenderer(void);

private:
	void add_value(char val) {
		ComboCellRenderer<char>::add_value(val, WaypointsDb::Waypoint::get_usage_name(val));
	}
};

class WaypointTypeRenderer : public ComboCellRenderer<char> {
public:
	WaypointTypeRenderer(void);

private:
	void add_value(char val) {
		ComboCellRenderer<char>::add_value(val, WaypointsDb::Waypoint::get_type_name(val));
	}
};

class AirwayTypeRenderer : public ComboCellRenderer<AirwaysDb::Airway::airway_type_t> {
public:
	AirwayTypeRenderer(void);

private:
	void add_value(AirwaysDb::Airway::airway_type_t val) {
		ComboCellRenderer<AirwaysDb::Airway::airway_type_t>::add_value(val, AirwaysDb::Airway::get_type_name(val));
	}
};

class AirspaceClassType {
public:
	AirspaceClassType(char c = 0, uint8_t t = 0): m_class(c), m_type(t) {}
	char get_class(void) const { return m_class; }
	uint8_t get_type(void) const { return m_type; }
	void set_class(char c) { m_class = c; }
	void set_type(uint8_t t) { m_type = t; }
	//bool operator==(const AirspaceClassType& x) const { return m_class == x.m_class && m_type == x.m_type; }
	//bool operator!=(const AirspaceClassType& x) const { return !operator==(x); }

private:
	char m_class;
	uint8_t m_type;
};

inline bool operator==(const AirspaceClassType& x, const AirspaceClassType& y)
{
        return x.get_class() == y.get_class() && x.get_type() == y.get_type();
}

inline bool operator!=(const AirspaceClassType& x, const AirspaceClassType& y)
{
        return !(x == y);
}

inline std::ostream& operator<<(std::ostream& os, const AirspaceClassType& x)
{
        return os << '(' << x.get_class() << ',' << (int)x.get_type() << ')';
}

class AirspaceClassRenderer : public ComboCellRenderer<AirspaceClassType> {
public:
	AirspaceClassRenderer(void);
	static Glib::ustring get_class_string(char bc, uint8_t tc);
};

class AirspaceShapeRenderer : public ComboCellRenderer<char> {
public:
	AirspaceShapeRenderer(void);
	static const Glib::ustring& get_shape_string(char shp);

private:
	void add_value(char val) {
		ComboCellRenderer<char>::add_value(val, get_shape_string(val));
	}
};

class AirspaceOperatorRenderer : public ComboCellRenderer<AirspacesDb::Airspace::Component::operator_t> {
public:
	AirspaceOperatorRenderer(void);
 
private:
	void add_value(AirspacesDb::Airspace::Component::operator_t val) {
		ComboCellRenderer<AirspacesDb::Airspace::Component::operator_t>::add_value(val, AirspacesDb::Airspace::Component::get_operator_string(val));
	}
};

class AirportTypeRenderer : public ComboCellRenderer<char> {
public:
	AirportTypeRenderer(void);

private:
	void add_value(char val) {
		ComboCellRenderer<char>::add_value(val, AirportsDb::Airport::get_type_string(val));
	}
};

class AirportVFRAltCodeRenderer : public ComboCellRenderer<AirportsDb::Airport::VFRRoute::VFRRoutePoint::altcode_t> {
public:
	AirportVFRAltCodeRenderer(void);

private:
};

class AirportVFRPathCodeRenderer : public ComboCellRenderer<AirportsDb::Airport::VFRRoute::VFRRoutePoint::pathcode_t> {
public:
	AirportVFRPathCodeRenderer(void);

private:
	void add_value(AirportsDb::Airport::VFRRoute::VFRRoutePoint::pathcode_t val) {
		ComboCellRenderer<AirportsDb::Airport::VFRRoute::VFRRoutePoint::pathcode_t>::add_value(val, AirportsDb::Airport::VFRRoute::VFRRoutePoint::get_pathcode_string(val));
	}
};

class AirportVFRPointSymbolRenderer : public ComboCellRenderer<char> {
public:
	AirportVFRPointSymbolRenderer(void);

private:
};

class AirportRunwayLightRenderer : public ComboCellRenderer<AirportsDb::Airport::Runway::light_t> {
public:
	AirportRunwayLightRenderer(void);

private:
};

class DateTimeCellRenderer : public NumericCellRenderer<time_t> {
public:
	DateTimeCellRenderer(void);

protected:
	virtual time_t to_numeric(const Glib::ustring& text);
	virtual Glib::ustring to_text(time_t val);
};

class DateTimeFrac8CellRenderer : public NumericCellRenderer<uint64_t> {
public:
	DateTimeFrac8CellRenderer(void);

protected:
	virtual uint64_t to_numeric(const Glib::ustring& text);
	virtual Glib::ustring to_text(uint64_t val);
};

class CoordCellRenderer : public NumericCellRenderer<Point> {
public:
	CoordCellRenderer(void);

protected:
	virtual Point to_numeric(const Glib::ustring& text);
	virtual Glib::ustring to_text(Point val);
};

class LatLonCellRenderer : public NumericCellRenderer<Point::coord_t> {
public:
	LatLonCellRenderer(bool latitude_mode);

protected:
	virtual Point::coord_t to_numeric(const Glib::ustring& text);
	virtual Glib::ustring to_text(Point::coord_t val);

private:
	bool m_latitude_mode;
};

class FrequencyCellRenderer : public NumericCellRenderer<uint32_t> {
public:
	FrequencyCellRenderer(void);

protected:
	virtual uint32_t to_numeric(const Glib::ustring& text);
	virtual Glib::ustring to_text(uint32_t val);
};

class AngleCellRenderer : public NumericCellRenderer<uint16_t> {
public:
	AngleCellRenderer(void);

protected:
	virtual uint16_t to_numeric(const Glib::ustring& text);
	virtual Glib::ustring to_text(uint16_t val);
};

class DbSelectDialog {
public:
	DbSelectDialog(void);
	~DbSelectDialog(void);

	void hide(void) { show(false); }
	void show(bool doit = true);
	sigc::signal< void > signal_cancel(void) { return m_signal_cancel; }
	sigc::signal< void, const Glib::ustring&, DbQueryInterfaceCommon::comp_t > signal_byname(void) { return m_signal_byname; }
	sigc::signal< void, const Rect& > signal_byrectangle(void) { return m_signal_byrectangle; }
	sigc::signal< void, time_t, time_t > signal_bytime(void) { return m_signal_bytime; }

private:
	class RegionModelColumns : public Gtk::TreeModelColumnRecord {
	public:
		RegionModelColumns(void);
		Gtk::TreeModelColumn<Glib::ustring> m_text;
		Gtk::TreeModelColumn<Rect> m_rect;
	};

	class CompareModelColumns : public Gtk::TreeModelColumnRecord {
	public:
		CompareModelColumns(void);
		Gtk::TreeModelColumn<Glib::ustring> m_text;
		Gtk::TreeModelColumn<DbQueryInterfaceCommon::comp_t> m_comp;
	};

	class WorldMap : public Gtk::DrawingArea {
	public:
		explicit WorldMap(BaseObjectType *castitem, const Glib::RefPtr<Builder>& refxml);
		virtual ~WorldMap();
		sigc::signal< void, const Rect& > signal_rect(void) { return m_signal_rect; }
		void set_rect(const Rect& r);
		void invalidate_rect(void);

	protected:
		static const char *pixmap_filename;
		virtual bool on_draw(const Cairo::RefPtr<Cairo::Context>& cr);
#ifdef HAVE_GTKMM2
		virtual bool on_expose_event(GdkEventExpose *event);
		virtual void on_size_request(Gtk::Requisition *requisition);
#endif
#ifdef HAVE_GTKMM3
		virtual Gtk::SizeRequestMode get_request_mode_vfunc() const;
		virtual void get_preferred_width_vfunc(int& minimum_width, int& natural_width) const;
		virtual void get_preferred_height_for_width_vfunc(int width, int& minimum_height, int& natural_height) const;
		virtual void get_preferred_height_vfunc(int& minimum_height, int& natural_height) const;
		virtual void get_preferred_width_for_height_vfunc(int height, int& minimum_width, int& natural_width) const;
#endif
		virtual void on_show(void);
		virtual void on_hide(void);
		virtual bool on_button_press_event(GdkEventButton* event);
		virtual bool on_button_release_event(GdkEventButton* event);
		virtual bool on_motion_notify_event(GdkEventMotion* event);
		virtual bool on_leave_notify_event(GdkEventCrossing* event);
		void draw_pixmap(GdkEventExpose *event);
		static void load_pixmap(Glib::RefPtr<Gdk::Pixbuf>& pixbuf, int width, int height = -1);
		void load_pixmap(int width, int height = -1) { load_pixmap(m_pixbuf, width, height); }

		void redraw(void) { queue_draw(); }

		Point screen_to_coord(const Gdk::Point& x) const;
		Gdk::Point coord_to_screen(const Point& x) const;

	private:
		Glib::RefPtr<Gdk::Pixbuf> m_pixbuf;
		Gdk::Point m_offset;
		Rect m_rect;
		Rect m_ptrrect;
		Point m_ptr;
		bool m_rect_valid;
		bool m_ptrrect_valid;
		bool m_ptrinside;
		sigc::signal< void, const Rect& > m_signal_rect;
	};

	sigc::signal< void > m_signal_cancel;
	sigc::signal< void, const Glib::ustring&, DbQueryInterfaceCommon::comp_t > m_signal_byname;
	sigc::signal< void, const Rect& > m_signal_byrectangle;
	sigc::signal< void, time_t, time_t > m_signal_bytime;
	Glib::RefPtr<Builder> m_refxml;
	Gtk::Dialog *m_dlg;
	Gtk::Notebook *m_dbselectnotebook;
	Gtk::Entry *m_dbselectfindstring;
	Gtk::ComboBox *m_dbselectfindcomp;
	WorldMap *m_dbselectworldmap;
	Gtk::Entry *m_westlon;
	Gtk::Entry *m_eastlon;
	Gtk::Entry *m_southlat;
	Gtk::Entry *m_northlat;
	Gtk::ComboBox *m_dbselectregion;
	Gtk::Entry *m_dbselecttimefrom;
	Gtk::Entry *m_dbselecttimeto;
	Rect m_rect;
	Glib::RefPtr<Gtk::ListStore> m_region_model;
	RegionModelColumns m_region_model_columns;
	Glib::RefPtr<Gtk::ListStore> m_comp_model;
	CompareModelColumns m_comp_model_columns;
	time_t m_timefrom;
	time_t m_timeto;

	void button_cancel_clicked(void);
	void button_ok_clicked(void);
	void westlon_changed(void);
	void eastlon_changed(void);
	void southlat_changed(void);
	void northlat_changed(void);
	bool latlon_focus_out_event(GdkEventFocus* event);
	void rect_changed(void);
	bool dialog_delete_event(GdkEventAny* event);
	void new_pointer_rect(const Rect& r);
	void region_changed(void);
	void timefrom_changed(void);
	bool timefrom_focus_out_event(GdkEventFocus* event);
	void timeto_changed(void);
	bool timeto_focus_out_event(GdkEventFocus* event);
	void timefrom_set(time_t offs);
	void timeto_set(time_t offs);
};

template <class T> class UndoRedoStack {
public:
	typedef T element_t;
	static const unsigned int max_undo = 1024;

	UndoRedoStack(void);
	void save(const element_t& e);
	void erase(const element_t& e);
	element_t undo(void);
	element_t redo(void);
	bool can_undo(void) const;
	bool can_redo(void) const;

private:
	typedef std::vector<element_t> elementvector_t;
	elementvector_t m_stack;
	unsigned int m_stackptr;

	void limit_stack(void);
};

class DbEditor : public sigc::trackable {
public:
	static Glib::ustring make_sourceid(void) { return NewWaypointWindow::make_sourceid(); }

protected:
      	class TopoEngine {
	public:
		TopoEngine(const std::string& dir_main = "", const std::string& dir_aux = "");

		TopoDb30::minmax_elev_t get_minmax_elev(const Rect& r);
		TopoDb30::minmax_elev_t get_minmax_elev(const PolygonSimple& p);

	protected:
		TopoDb30 m_topodb;
#ifdef HAVE_PILOTLINK
		PalmGtopo m_palmgtopodb;
#endif
	};

#ifdef HAVE_HILDONMM
	typedef Hildon::Window toplevel_window_t;
#else
	typedef Gtk::Window toplevel_window_t;
#endif

	class DbEditorWindow : public toplevel_window_t {
	public:
		explicit DbEditorWindow(BaseObjectType *castitem, const Glib::RefPtr<Builder>& refxml);
		virtual ~DbEditorWindow();

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
		bool on_my_delete_event(GdkEventAny* event);
		bool on_my_key_press_event(GdkEventKey* event);
	};
};

class NavaidEditor : public DbEditor {
public:
	NavaidEditor(const std::string& dir_main = "", Engine::auxdb_mode_t auxdbmode = Engine::auxdb_prefs, const std::string& dir_aux = "");
	~NavaidEditor();
	Gtk::Window *get_mainwindow(void) { return m_navaideditorwindow; }
	const Gtk::Window *get_mainwindow(void) const { return m_navaideditorwindow; }

private:
	class NavaidModelColumns : public Gtk::TreeModelColumnRecord {
	public:
		NavaidModelColumns(void);
		Gtk::TreeModelColumn<NavaidsDb::Address> m_col_id;
		Gtk::TreeModelColumn<Point> m_col_coord;
		Gtk::TreeModelColumn<int32_t> m_col_elev;
		Gtk::TreeModelColumn<int32_t> m_col_range;
		Gtk::TreeModelColumn<uint32_t> m_col_freq;
		Gtk::TreeModelColumn<Glib::ustring> m_col_sourceid;
		Gtk::TreeModelColumn<Glib::ustring> m_col_icao;
		Gtk::TreeModelColumn<Glib::ustring> m_col_name;
		Gtk::TreeModelColumn<NavaidsDb::Navaid::navaid_type_t> m_col_navaid_type;
		Gtk::TreeModelColumn<NavaidsDb::Navaid::label_placement_t> m_col_label_placement;
		Gtk::TreeModelColumn<bool> m_col_inservice;
		Gtk::TreeModelColumn<time_t> m_col_modtime;
	};

	void buttonclear_clicked(void);
	void find_changed(void);
	void menu_quit_activate(void);
	void menu_undo_activate(void);
	void menu_redo_activate(void);
	void menu_newnavaid_activate(void);
	void menu_deletenavaid_activate(void);
	void menu_cut_activate(void);
	void menu_copy_activate(void);
	void menu_paste_activate(void);
	void menu_delete_activate(void);
	void menu_mapenable_toggled(void);
	void menu_airportdiagramenable_toggled(void);
	void menu_mapzoomin_activate(void);
	void menu_mapzoomout_activate(void);
	void menu_prefs_activate(void);
	void menu_about_activate(void);
	void aboutdialog_response(int response);
	void set_row(Gtk::TreeModel::Row& row, const NavaidsDb::Navaid& e);
	void dbselect_byname(const Glib::ustring& s, NavaidsDb::comp_t comp = NavaidsDb::comp_contains);
	void dbselect_byrect(const Rect& r);
	void dbselect_bytime(time_t timefrom, time_t timeto);

	void edited_coord(const Glib::ustring& path_string, const Glib::ustring& new_text, Point new_coord);
	void edited_elev(const Glib::ustring& path_string, const Glib::ustring& new_text);
	void edited_range(const Glib::ustring& path_string, const Glib::ustring& new_text);
	void edited_freq(const Glib::ustring& path_string, const Glib::ustring& new_text, uint32_t new_freq);
	void edited_sourceid(const Glib::ustring& path_string, const Glib::ustring& new_text);
	void edited_icao(const Glib::ustring& path_string, const Glib::ustring& new_text);
	void edited_name(const Glib::ustring& path_string, const Glib::ustring& new_text);
	void edited_navaid_type(const Glib::ustring& path_string, const Glib::ustring& new_text, NavaidsDb::Navaid::navaid_type_t new_type);
	void edited_label_placement(const Glib::ustring& path_string, const Glib::ustring& new_text, NavaidsDb::Navaid::label_placement_t new_placement);
	void edited_inservice(const Glib::ustring& path_string);
	void edited_modtime(const Glib::ustring& path_string, const Glib::ustring& new_text, time_t new_time);

	void map_cursor(Point cursor, VectorMapArea::cursor_validity_t valid);
	void map_drawflags(int flags);

	void save(NavaidsDb::Navaid& e);
	void save_noerr(NavaidsDb::Navaid& e, Gtk::TreeModel::Row& row);
	void save_nostack(NavaidsDb::Navaid& e);
	void update_undoredo_menu(void);
	void navaideditor_selection_changed(void);
	void select(const NavaidsDb::Navaid& e);

	std::unique_ptr<NavaidsDbQueryInterface> m_db;
#ifdef HAVE_PQXX
	std::unique_ptr<pqxx::connection> m_pgconn;
#endif
	bool m_dbchanged;
	Engine m_engine;
	UndoRedoStack<NavaidsDb::Navaid> m_undoredo;
	DbSelectDialog m_dbselectdialog;
	Glib::RefPtr<PrefsWindow> m_prefswindow;
	Glib::RefPtr<Builder> m_refxml;
	DbEditorWindow *m_navaideditorwindow;
	Gtk::TreeView *m_navaideditortreeview;
	Gtk::Statusbar *m_navaideditorstatusbar;
	Gtk::Entry *m_navaideditorfind;
	VectorMapArea *m_navaideditormap;
	Gtk::MenuItem *m_navaideditorundo;
	Gtk::MenuItem *m_navaideditorredo;
	Gtk::MenuItem *m_navaideditordeletenavaid;
	Gtk::CheckMenuItem *m_navaideditormapenable;
	Gtk::CheckMenuItem *m_navaideditorairportdiagramenable;
	Gtk::MenuItem *m_navaideditormapzoomin;
	Gtk::MenuItem *m_navaideditormapzoomout;
	Gtk::AboutDialog *m_aboutdialog;
	NavaidModelColumns m_navaidlistcolumns;
	Glib::RefPtr<Gtk::ListStore> m_navaidliststore;
};

class WaypointEditor : public DbEditor {
public:
	WaypointEditor(const std::string& dir_main = "", Engine::auxdb_mode_t auxdbmode = Engine::auxdb_prefs, const std::string& dir_aux = "");
	~WaypointEditor();
	Gtk::Window *get_mainwindow(void) { return m_waypointeditorwindow; }
	const Gtk::Window *get_mainwindow(void) const { return m_waypointeditorwindow; }

private:
	class WaypointModelColumns : public Gtk::TreeModelColumnRecord {
	public:
		WaypointModelColumns(void);
		Gtk::TreeModelColumn<WaypointsDb::Address> m_col_id;
		Gtk::TreeModelColumn<Point> m_col_coord;
		Gtk::TreeModelColumn<Glib::ustring> m_col_sourceid;
		Gtk::TreeModelColumn<Glib::ustring> m_col_icao;
		Gtk::TreeModelColumn<Glib::ustring> m_col_name;
		Gtk::TreeModelColumn<char> m_col_usage;
		Gtk::TreeModelColumn<char> m_col_type;
		Gtk::TreeModelColumn<NavaidsDb::Navaid::label_placement_t> m_col_label_placement;
		Gtk::TreeModelColumn<time_t> m_col_modtime;
	};

	void buttonclear_clicked(void);
	void find_changed(void);
	void menu_quit_activate(void);
	void menu_undo_activate(void);
	void menu_redo_activate(void);
	void menu_newwaypoint_activate(void);
	void menu_deletewaypoint_activate(void);
	void menu_cut_activate(void);
	void menu_copy_activate(void);
	void menu_paste_activate(void);
	void menu_delete_activate(void);
	void menu_mapenable_toggled(void);
	void menu_airportdiagramenable_toggled(void);
	void menu_mapzoomin_activate(void);
	void menu_mapzoomout_activate(void);
	void menu_prefs_activate(void);
	void menu_about_activate(void);
	void aboutdialog_response(int response);
	void set_row(Gtk::TreeModel::Row& row, const WaypointsDb::Waypoint& e);
	void dbselect_byname(const Glib::ustring& s, WaypointsDb::comp_t comp = WaypointsDb::comp_contains);
	void dbselect_byrect(const Rect& r);
	void dbselect_bytime(time_t timefrom, time_t timeto);

	void edited_coord(const Glib::ustring& path_string, const Glib::ustring& new_text, Point new_coord);
	void edited_sourceid(const Glib::ustring& path_string, const Glib::ustring& new_text);
	void edited_icao(const Glib::ustring& path_string, const Glib::ustring& new_text);
	void edited_name(const Glib::ustring& path_string, const Glib::ustring& new_text);
	void edited_usage(const Glib::ustring& path_string, const Glib::ustring& new_text, char new_usage);
	void edited_type(const Glib::ustring& path_string, const Glib::ustring& new_text, char new_type);
	void edited_label_placement(const Glib::ustring& path_string, const Glib::ustring& new_text, NavaidsDb::Navaid::label_placement_t new_placement);
	void edited_modtime(const Glib::ustring& path_string, const Glib::ustring& new_text, time_t new_time);

	void map_cursor(Point cursor, VectorMapArea::cursor_validity_t valid);
	void map_drawflags(int flags);

	void save(WaypointsDb::Waypoint& e);
	void save_noerr(WaypointsDb::Waypoint& e, Gtk::TreeModel::Row& row);
	void save_nostack(WaypointsDb::Waypoint& e);
	void update_undoredo_menu(void);
	void waypointeditor_selection_changed(void);
	void select(const WaypointsDb::Waypoint& e);

	std::unique_ptr<WaypointsDbQueryInterface> m_db;
#ifdef HAVE_PQXX
	std::unique_ptr<pqxx::connection> m_pgconn;
#endif
	bool m_dbchanged;
	Engine m_engine;
	UndoRedoStack<WaypointsDb::Waypoint> m_undoredo;
	DbSelectDialog m_dbselectdialog;
	Glib::RefPtr<PrefsWindow> m_prefswindow;
	Glib::RefPtr<Builder> m_refxml;
	DbEditorWindow *m_waypointeditorwindow;
	Gtk::TreeView *m_waypointeditortreeview;
	Gtk::Statusbar *m_waypointeditorstatusbar;
	Gtk::Entry *m_waypointeditorfind;
	VectorMapArea *m_waypointeditormap;
	Gtk::MenuItem *m_waypointeditorundo;
	Gtk::MenuItem *m_waypointeditorredo;
	Gtk::MenuItem *m_waypointeditordeletewaypoint;
	Gtk::CheckMenuItem *m_waypointeditormapenable;
	Gtk::CheckMenuItem *m_waypointeditorairportdiagramenable;
	Gtk::MenuItem *m_waypointeditormapzoomin;
	Gtk::MenuItem *m_waypointeditormapzoomout;
	Gtk::AboutDialog *m_aboutdialog;
	WaypointModelColumns m_waypointlistcolumns;
	Glib::RefPtr<Gtk::ListStore> m_waypointliststore;
};

class AirwayEditor : public DbEditor {
public:
	AirwayEditor(const std::string& dir_main = "", Engine::auxdb_mode_t auxdbmode = Engine::auxdb_prefs, const std::string& dir_aux = "");
	~AirwayEditor();
	Gtk::Window *get_mainwindow(void) { return m_airwayeditorwindow; }
	const Gtk::Window *get_mainwindow(void) const { return m_airwayeditorwindow; }

private:
	class AirwayModelColumns : public Gtk::TreeModelColumnRecord {
	public:
		AirwayModelColumns(void);
		Gtk::TreeModelColumn<AirwaysDb::Address> m_col_id;
		Gtk::TreeModelColumn<Point> m_col_begin_coord;
		Gtk::TreeModelColumn<Point> m_col_end_coord;
		Gtk::TreeModelColumn<Glib::ustring> m_col_sourceid;
		Gtk::TreeModelColumn<Glib::ustring> m_col_begin_name;
		Gtk::TreeModelColumn<Glib::ustring> m_col_end_name;
		Gtk::TreeModelColumn<Glib::ustring> m_col_name;
		Gtk::TreeModelColumn<int32_t> m_col_base_level;
		Gtk::TreeModelColumn<int32_t> m_col_top_level;
		Gtk::TreeModelColumn<int32_t> m_col_terrain_elev;
		Gtk::TreeModelColumn<int32_t> m_col_corridor5_elev;
		Gtk::TreeModelColumn<AirwaysDb::Airway::airway_type_t> m_col_type;
		Gtk::TreeModelColumn<NavaidsDb::Navaid::label_placement_t> m_col_label_placement;
		Gtk::TreeModelColumn<time_t> m_col_modtime;
	};

	class AsyncElevUpdate {
	public:
		AsyncElevUpdate(Engine& eng, AirwaysDbQueryInterface& db, const AirwaysDb::Address& addr, const sigc::slot<void,AirwaysDb::Airway&>& save);
		~AsyncElevUpdate();
		void cancel(void);
		bool is_done(void);
		void reference(void);
		void unreference(void);

	protected:
		AirwaysDbQueryInterface& m_db;
		AirwaysDb::Address m_addr;
		Glib::RefPtr<Engine::ElevationProfileResult> m_async;
		Glib::Dispatcher m_dispatch;
		sigc::slot<void,AirwaysDb::Airway&> m_save;
		int m_refcnt;
		bool m_asyncref;

		void callback_dispatch(void);
		void callback(void);
	};

	void buttonclear_clicked(void);
	void find_changed(void);
	void menu_quit_activate(void);
	void menu_undo_activate(void);
	void menu_redo_activate(void);
	void menu_newairway_activate(void);
	void menu_deleteairway_activate(void);
	void menu_swapfromto_activate(void);
	void menu_elev_activate(void);
	void menu_cut_activate(void);
	void menu_copy_activate(void);
	void menu_paste_activate(void);
	void menu_delete_activate(void);
	void menu_mapenable_toggled(void);
	void menu_airportdiagramenable_toggled(void);
	void menu_mapcoord_toggled(void);
	void menu_mapzoomin_activate(void);
	void menu_mapzoomout_activate(void);
	void menu_prefs_activate(void);
	void menu_about_activate(void);
	void aboutdialog_response(int response);
	void set_row(Gtk::TreeModel::Row& row, const AirwaysDb::Airway& e);
	void dbselect_byname(const Glib::ustring& s, AirwaysDb::comp_t comp = AirwaysDb::comp_contains);
	void dbselect_byrect(const Rect& r);
	void dbselect_bytime(time_t timefrom, time_t timeto);

	void edited_begin_coord(const Glib::ustring& path_string, const Glib::ustring& new_text, Point new_coord);
	void edited_end_coord(const Glib::ustring& path_string, const Glib::ustring& new_text, Point new_coord);
	void edited_sourceid(const Glib::ustring& path_string, const Glib::ustring& new_text);
	void edited_begin_name(const Glib::ustring& path_string, const Glib::ustring& new_text);
	void edited_end_name(const Glib::ustring& path_string, const Glib::ustring& new_text);
	void edited_name(const Glib::ustring& path_string, const Glib::ustring& new_text);
	void edited_base_level(const Glib::ustring& path_string, const Glib::ustring& new_text);
	void edited_top_level(const Glib::ustring& path_string, const Glib::ustring& new_text);
	void edited_terrain_elev(const Glib::ustring& path_string, const Glib::ustring& new_text);
	void edited_corridor5_elev(const Glib::ustring& path_string, const Glib::ustring& new_text);
	void edited_type(const Glib::ustring& path_string, const Glib::ustring& new_text, AirwaysDb::Airway::airway_type_t new_type);
	void edited_label_placement(const Glib::ustring& path_string, const Glib::ustring& new_text, NavaidsDb::Navaid::label_placement_t new_placement);
	void edited_modtime(const Glib::ustring& path_string, const Glib::ustring& new_text, time_t new_time);

	void map_cursor(Point cursor, VectorMapArea::cursor_validity_t valid);
	void map_drawflags(int flags);

	void save(AirwaysDb::Airway& e);
	void save_noerr(AirwaysDb::Airway& e, Gtk::TreeModel::Row& row);
	void save_nostack(AirwaysDb::Airway& e);
	void update_undoredo_menu(void);
	void airwayeditor_selection_changed(void);
	void select(const AirwaysDb::Airway& e);

	std::unique_ptr<AirwaysDbQueryInterface> m_db;
#ifdef HAVE_PQXX
	std::unique_ptr<pqxx::connection> m_pgconn;
#endif
	bool m_dbchanged;
	Engine m_engine;
	UndoRedoStack<AirwaysDb::Airway> m_undoredo;
	DbSelectDialog m_dbselectdialog;
	Glib::RefPtr<PrefsWindow> m_prefswindow;
	Glib::RefPtr<Builder> m_refxml;
	DbEditorWindow *m_airwayeditorwindow;
	Gtk::TreeView *m_airwayeditortreeview;
	Gtk::Statusbar *m_airwayeditorstatusbar;
	Gtk::Entry *m_airwayeditorfind;
	VectorMapArea *m_airwayeditormap;
	Gtk::MenuItem *m_airwayeditorundo;
	Gtk::MenuItem *m_airwayeditorredo;
	Gtk::MenuItem *m_airwayeditordeleteairway;
	Gtk::MenuItem *m_airwayeditorswapfromto;
	Gtk::MenuItem *m_airwayeditorelev;
	Gtk::CheckMenuItem *m_airwayeditormapenable;
	Gtk::CheckMenuItem *m_airwayeditorairportdiagramenable;
	Gtk::RadioMenuItem *m_airwayeditormapfromcoord;
	Gtk::RadioMenuItem *m_airwayeditormaptocoord;
	Gtk::MenuItem *m_airwayeditormapzoomin;
	Gtk::MenuItem *m_airwayeditormapzoomout;
	Gtk::AboutDialog *m_aboutdialog;
	AirwayModelColumns m_airwaylistcolumns;
	Glib::RefPtr<Gtk::ListStore> m_airwayliststore;
};

class AirspaceEditor : public DbEditor {
public:
	AirspaceEditor(const std::string& dir_main = "", Engine::auxdb_mode_t auxdbmode = Engine::auxdb_prefs, const std::string& dir_aux = "");
	~AirspaceEditor();
	Gtk::Window *get_mainwindow(void) { return m_airspaceeditorwindow; }
	const Gtk::Window *get_mainwindow(void) const { return m_airspaceeditorwindow; }

private:
	class MyVectorMapArea : public VectorMapArea {
	public:
		explicit MyVectorMapArea(BaseObjectType *castitem, const Glib::RefPtr<Builder>& refxml);
		void set_airspace(const AirspacesDb::Airspace& aspc);
		void clear_airspace(void);

		virtual MyVectorMapArea& operator=(MapRenderer::DrawFlags f) { VectorMapArea::operator=(f); return *this; }
		virtual MyVectorMapArea& operator&=(MapRenderer::DrawFlags f) { VectorMapArea::operator&=(f); return *this; }
		virtual MyVectorMapArea& operator|=(MapRenderer::DrawFlags f) { VectorMapArea::operator|=(f); return *this; }
		virtual MyVectorMapArea& operator^=(MapRenderer::DrawFlags f) { VectorMapArea::operator^=(f); return *this; }

	protected:
		AirspacesDb::Airspace m_aspc;

		virtual bool on_draw(const Cairo::RefPtr<Cairo::Context>& cr);
	};

	class AirspaceModelColumns : public Gtk::TreeModelColumnRecord {
	public:
		AirspaceModelColumns(void);
		Gtk::TreeModelColumn<AirspacesDb::Address> m_col_id;
		Gtk::TreeModelColumn<Point> m_col_labelcoord;
		Gtk::TreeModelColumn<AirspacesDb::Airspace::label_placement_t> m_col_label_placement;
		Gtk::TreeModelColumn<Glib::ustring> m_col_sourceid;
		Gtk::TreeModelColumn<Glib::ustring> m_col_icao;
		Gtk::TreeModelColumn<Glib::ustring> m_col_name;
		Gtk::TreeModelColumn<Glib::ustring> m_col_ident;
		Gtk::TreeModelColumn<Glib::ustring> m_col_commname;
		Gtk::TreeModelColumn<uint32_t> m_col_commfreq0;
		Gtk::TreeModelColumn<uint32_t> m_col_commfreq1;
		Gtk::TreeModelColumn<AirspaceClassType> m_col_classtype;
		Gtk::TreeModelColumn<double> m_col_width;
		Gtk::TreeModelColumn<int32_t> m_col_gndelevmin;
		Gtk::TreeModelColumn<int32_t> m_col_gndelevmax;
		Gtk::TreeModelColumn<int32_t> m_col_altlwr;
		Gtk::TreeModelColumn<bool> m_col_altlwr_agl;
		Gtk::TreeModelColumn<bool> m_col_altlwr_fl;
		Gtk::TreeModelColumn<bool> m_col_altlwr_sfc;
		Gtk::TreeModelColumn<bool> m_col_altlwr_gnd;
		Gtk::TreeModelColumn<bool> m_col_altlwr_unlim;
		Gtk::TreeModelColumn<bool> m_col_altlwr_notam;
		Gtk::TreeModelColumn<bool> m_col_altlwr_unkn;
		Gtk::TreeModelColumn<int32_t> m_col_altupr;
		Gtk::TreeModelColumn<bool> m_col_altupr_agl;
		Gtk::TreeModelColumn<bool> m_col_altupr_fl;
		Gtk::TreeModelColumn<bool> m_col_altupr_sfc;
		Gtk::TreeModelColumn<bool> m_col_altupr_gnd;
		Gtk::TreeModelColumn<bool> m_col_altupr_unlim;
		Gtk::TreeModelColumn<bool> m_col_altupr_notam;
		Gtk::TreeModelColumn<bool> m_col_altupr_unkn;
		Gtk::TreeModelColumn<time_t> m_col_modtime;
	};

	class AirspaceSegmentModelColumns : public Gtk::TreeModelColumnRecord {
	public:
		AirspaceSegmentModelColumns(void);
		Gtk::TreeModelColumn<unsigned int> m_col_segment;
		Gtk::TreeModelColumn<Point> m_col_coord1;
		Gtk::TreeModelColumn<Point> m_col_coord2;
		Gtk::TreeModelColumn<Point> m_col_coord0;
		Gtk::TreeModelColumn<char> m_col_shape;
		Gtk::TreeModelColumn<double> m_col_radius1;
		Gtk::TreeModelColumn<double> m_col_radius2;
	};

	class AirspaceComponentModelColumns : public Gtk::TreeModelColumnRecord {
	public:
		AirspaceComponentModelColumns(void);
		Gtk::TreeModelColumn<unsigned int> m_col_component;
		Gtk::TreeModelColumn<Glib::ustring> m_col_icao;
		Gtk::TreeModelColumn<AirspaceClassType> m_col_classtype;
		Gtk::TreeModelColumn<AirspacesDb::Airspace::Component::operator_t> m_col_operator;
	};

	class AirspacePolygonModelColumns : public Gtk::TreeModelColumnRecord {
	public:
		AirspacePolygonModelColumns(void);
		Gtk::TreeModelColumn<unsigned int> m_col_point;
		Gtk::TreeModelColumn<Point> m_col_coord;
	};

	class AsyncElevUpdate {
	public:
		AsyncElevUpdate(Engine& eng, AirspacesDbQueryInterface& db, const AirspacesDb::Address& addr, const sigc::slot<void,AirspacesDb::Airspace&>& save);
		~AsyncElevUpdate();
		void cancel(void);
		bool is_done(void);
		void reference(void);
		void unreference(void);

	protected:
		AirspacesDbQueryInterface& m_db;
		AirspacesDb::Address m_addr;
		Glib::RefPtr<Engine::ElevationMinMaxResult> m_async;
		Glib::Dispatcher m_dispatch;
		sigc::slot<void,AirspacesDb::Airspace&> m_save;
		int m_refcnt;
		bool m_asyncref;

		void callback_dispatch(void);
		void callback(void);
	};

	static Glib::ustring get_class_string(const AirspacesDb::Airspace& e) { return AirspaceClassRenderer::get_class_string(e.get_bdryclass(), e.get_typecode()); }

	void hide_notebook_pages(void);
	void show_notebook_pages(void);
	void notebook_switch_page(guint page_num);
	void col_latlon1_clicked(void);
	void buttonclear_clicked(void);
	void find_changed(void);
	void menu_quit_activate(void);
	void menu_undo_activate(void);
	void menu_redo_activate(void);
	void menu_newairspace_activate(void);
	void menu_duplicateairspace_activate(void);
	void menu_deleteairspace_activate(void);
	void menu_placelabel_activate(void);
	void menu_gndelev_activate(void);
	void menu_poly_activate(void);
	void menu_cut_activate(void);
	void menu_copy_activate(void);
	void menu_paste_activate(void);
	void menu_delete_activate(void);
	void menu_selectall_activate(void);
	void menu_unselectall_activate(void);
	void menu_mapenable_toggled(void);
	void menu_airportdiagramenable_toggled(void);
	void menu_mapzoomin_activate(void);
	void menu_mapzoomout_activate(void);
	void menu_appendshape_activate(void);
	void menu_insertshape_activate(void);
	void menu_deleteshape_activate(void);
	void menu_appendcomponent_activate(void);
	void menu_insertcomponent_activate(void);
	void menu_deletecomponent_activate(void);
	void menu_prefs_activate(void);
	void menu_about_activate(void);
	void aboutdialog_response(int response);
	void set_row(Gtk::TreeModel::Row& row, const AirspacesDb::Airspace& e);
	void set_row_altlwr_flags(Gtk::TreeModel::Row& row, const AirspacesDb::Airspace& e);
	void set_row_altupr_flags(Gtk::TreeModel::Row& row, const AirspacesDb::Airspace& e);
	void set_segmentcomponentpolygonlist(AirspacesDb::Airspace::Address addr);
	void dbselect_byname(const Glib::ustring& s, AirspacesDb::comp_t comp = AirspacesDb::comp_contains);
	void dbselect_byrect(const Rect& r);
	void dbselect_bytime(time_t timefrom, time_t timeto);

	void edited_labelcoord(const Glib::ustring& path_string, const Glib::ustring& new_text, Point new_coord);
	void edited_label_placement(const Glib::ustring& path_string, const Glib::ustring& new_text, NavaidsDb::Navaid::label_placement_t new_placement);
	void edited_sourceid(const Glib::ustring& path_string, const Glib::ustring& new_text);
	void edited_icao(const Glib::ustring& path_string, const Glib::ustring& new_text);
	void edited_name(const Glib::ustring& path_string, const Glib::ustring& new_text);
	void edited_ident(const Glib::ustring& path_string, const Glib::ustring& new_text);
	void edited_commname(const Glib::ustring& path_string, const Glib::ustring& new_text);
	void edited_commfreq0(const Glib::ustring& path_string, const Glib::ustring& new_text, uint32_t new_freq);
	void edited_commfreq1(const Glib::ustring& path_string, const Glib::ustring& new_text, uint32_t new_freq);
	void edited_airspaceclass(const Glib::ustring& path_string, const Glib::ustring& new_text, AirspaceClassType new_clstype);
	void edited_width(const Glib::ustring& path_string, const Glib::ustring& new_text);
	void edited_gndelevmin(const Glib::ustring& path_string, const Glib::ustring& new_text);
	void edited_gndelevmax(const Glib::ustring& path_string, const Glib::ustring& new_text);
	void edited_altlwr(const Glib::ustring& path_string, const Glib::ustring& new_text);
	void edited_altlwr_flag(const Glib::ustring& path_string, uint8_t f);
	void edited_altupr(const Glib::ustring& path_string, const Glib::ustring& new_text);
	void edited_altupr_flag(const Glib::ustring& path_string, uint8_t f);
	void edited_modtime(const Glib::ustring& path_string, const Glib::ustring& new_text, time_t new_time);
	void edited_classexcept(void);
	void edited_efftime(void);
	void edited_note(void);
	void update_radius12(const Gtk::TreeModel::iterator& iter);
	void edited_shape(const Glib::ustring& path_string, const Glib::ustring& new_text, char new_shape);
	void edited_coord1(const Glib::ustring& path_string, const Glib::ustring& new_text, Point new_coord);
	void edited_coord2(const Glib::ustring& path_string, const Glib::ustring& new_text, Point new_coord);
	void edited_coord0(const Glib::ustring& path_string, const Glib::ustring& new_text, Point new_coord);
	void edited_radius1(const Glib::ustring& path_string, const Glib::ustring& new_text);
	void edited_radius2(const Glib::ustring& path_string, const Glib::ustring& new_text);
	void edited_compicao(const Glib::ustring& path_string, const Glib::ustring& new_text);
	void edited_compclass(const Glib::ustring& path_string, const Glib::ustring& new_text, AirspaceClassType new_clstype);
	void edited_compoperator(const Glib::ustring& path_string, const Glib::ustring& new_text, AirspacesDb::Airspace::Component::operator_t new_operator);

	void map_cursor(Point cursor, VectorMapArea::cursor_validity_t valid);
	void map_drawflags(int flags);

	void save(AirspacesDb::Airspace& e);
	void save_noerr(AirspacesDb::Airspace& e, Gtk::TreeModel::Row& row);
	void save_noerr_lists(AirspacesDb::Airspace& e);
	void save_nostack(AirspacesDb::Airspace& e);
	void update_undoredo_menu(void);
	void airspaceeditor_selection_changed(void);
	void select(const AirspacesDb::Airspace& e);
	void airspaceeditor_shape_selection_changed(void);
	void airspaceeditor_component_selection_changed(void);
	void airspaceeditor_polygon_selection_changed(void);
	void save_shapes(void);
	void new_shape(Gtk::TreeModel::iterator iter);
	void renumber_shapes(void);
	void save_components(void);
	void new_component(Gtk::TreeModel::iterator iter);
	void renumber_components(void);

	std::unique_ptr<AirspacesDbQueryInterface> m_db;
#ifdef HAVE_PQXX
	std::unique_ptr<pqxx::connection> m_pgconn;
#endif
	bool m_dbchanged;
	Engine m_engine;
	UndoRedoStack<AirspacesDb::Airspace> m_undoredo;
	DbSelectDialog m_dbselectdialog;
	Glib::RefPtr<PrefsWindow> m_prefswindow;
	Glib::RefPtr<Builder> m_refxml;
	DbEditorWindow *m_airspaceeditorwindow;
	Gtk::TreeView *m_airspaceeditortreeview;
	Gtk::Statusbar *m_airspaceeditorstatusbar;
	Gtk::Entry *m_airspaceeditorfind;
	Gtk::Notebook *m_airspaceeditornotebook;
	Gtk::TextView *m_airspaceeditorclassexcept;
	Gtk::TextView *m_airspaceeditorefftime;
	Gtk::TextView *m_airspaceeditornote;
	Gtk::TreeView *m_airspaceshapeeditortreeview;
	Gtk::TreeView *m_airspacecomponenteditortreeview;
	Gtk::TreeView *m_airspacepolyapproxeditortreeview;
	MyVectorMapArea *m_airspaceeditormap;
	Gtk::MenuItem *m_airspaceeditorundo;
	Gtk::MenuItem *m_airspaceeditorredo;
	Gtk::MenuItem *m_airspaceeditornewairspace;
	Gtk::MenuItem *m_airspaceeditorduplicateairspace;
	Gtk::MenuItem *m_airspaceeditordeleteairspace;
	Gtk::MenuItem *m_airspaceeditorplacelabel;
	Gtk::MenuItem *m_airspaceeditorgndelev;
	Gtk::MenuItem *m_airspaceeditorpoly;
	Gtk::CheckMenuItem *m_airspaceeditormapenable;
	Gtk::CheckMenuItem *m_airspaceeditorairportdiagramenable;
	Gtk::MenuItem *m_airspaceeditormapzoomin;
	Gtk::MenuItem *m_airspaceeditormapzoomout;
	Gtk::MenuItem *m_airspaceeditorappendshape;
	Gtk::MenuItem *m_airspaceeditorinsertshape;
	Gtk::MenuItem *m_airspaceeditordeleteshape;
	Gtk::MenuItem *m_airspaceeditorappendcomponent;
	Gtk::MenuItem *m_airspaceeditorinsertcomponent;
	Gtk::MenuItem *m_airspaceeditordeletecomponent;
	Gtk::MenuItem *m_airspaceeditorselectall;
	Gtk::MenuItem *m_airspaceeditorunselectall;
	Gtk::AboutDialog *m_aboutdialog;
	AirspacesDb::Address m_curentryid;
	AirspaceModelColumns m_airspacelistcolumns;
	Glib::RefPtr<Gtk::ListStore> m_airspaceliststore;
	AirspaceSegmentModelColumns m_airspacesegment_model_columns;
	Glib::RefPtr<Gtk::ListStore> m_airspacesegment_model;
	AirspaceComponentModelColumns m_airspacecomponent_model_columns;
	Glib::RefPtr<Gtk::ListStore> m_airspacecomponent_model;
	AirspacePolygonModelColumns m_airspacepolygon_model_columns;
	Glib::RefPtr<Gtk::TreeStore> m_airspacepolygon_model;
};

class AirportEditor : public DbEditor {
public:
	AirportEditor(const std::string& dir_main = "", Engine::auxdb_mode_t auxdbmode = Engine::auxdb_prefs, const std::string& dir_aux = "");
	~AirportEditor();
	Gtk::Window *get_mainwindow(void) { return m_airporteditorwindow; }
	const Gtk::Window *get_mainwindow(void) const { return m_airporteditorwindow; }

private:
	class AirportModelColumns : public Gtk::TreeModelColumnRecord {
	public:
		AirportModelColumns(void);
		Gtk::TreeModelColumn<AirportsDb::Address> m_col_id;
		Gtk::TreeModelColumn<Point> m_col_coord;
		Gtk::TreeModelColumn<AirportsDb::Airport::label_placement_t> m_col_label_placement;
		Gtk::TreeModelColumn<Glib::ustring> m_col_sourceid;
		Gtk::TreeModelColumn<Glib::ustring> m_col_icao;
		Gtk::TreeModelColumn<Glib::ustring> m_col_name;
		Gtk::TreeModelColumn<int32_t> m_col_elev;
		Gtk::TreeModelColumn<char> m_col_typecode;
		Gtk::TreeModelColumn<bool> m_col_frules_arr_vfr;
		Gtk::TreeModelColumn<bool> m_col_frules_arr_ifr;
		Gtk::TreeModelColumn<bool> m_col_frules_dep_vfr;
		Gtk::TreeModelColumn<bool> m_col_frules_dep_ifr;
		Gtk::TreeModelColumn<time_t> m_col_modtime;
	};

	class AirportRunwayModelColumns : public Gtk::TreeModelColumnRecord {
	public:
		AirportRunwayModelColumns(void);
		Gtk::TreeModelColumn<int> m_col_index;
		Gtk::TreeModelColumn<Glib::ustring> m_col_ident_he;
		Gtk::TreeModelColumn<Glib::ustring> m_col_ident_le;
		Gtk::TreeModelColumn<uint32_t> m_col_length;
		Gtk::TreeModelColumn<uint32_t> m_col_width;
		Gtk::TreeModelColumn<Glib::ustring> m_col_surface;
		Gtk::TreeModelColumn<Point> m_col_he_coord;
		Gtk::TreeModelColumn<Point> m_col_le_coord;
		Gtk::TreeModelColumn<uint32_t> m_col_he_tda;
		Gtk::TreeModelColumn<uint32_t> m_col_he_lda;
		Gtk::TreeModelColumn<uint32_t> m_col_he_disp;
		Gtk::TreeModelColumn<uint16_t> m_col_he_hdg;
		Gtk::TreeModelColumn<int32_t> m_col_he_elev;
		Gtk::TreeModelColumn<uint32_t> m_col_le_tda;
		Gtk::TreeModelColumn<uint32_t> m_col_le_lda;
		Gtk::TreeModelColumn<uint32_t> m_col_le_disp;
		Gtk::TreeModelColumn<uint16_t> m_col_le_hdg;
		Gtk::TreeModelColumn<int32_t> m_col_le_elev;
		Gtk::TreeModelColumn<bool> m_col_flags_he_usable;
		Gtk::TreeModelColumn<bool> m_col_flags_le_usable;
		Gtk::TreeModelColumn<uint8_t> m_col_he_light[8];
		Gtk::TreeModelColumn<uint8_t> m_col_le_light[8];
	};

	class AirportHelipadModelColumns : public Gtk::TreeModelColumnRecord {
	public:
		AirportHelipadModelColumns(void);
		Gtk::TreeModelColumn<int> m_col_index;
		Gtk::TreeModelColumn<Glib::ustring> m_col_ident;
		Gtk::TreeModelColumn<uint32_t> m_col_length;
		Gtk::TreeModelColumn<uint32_t> m_col_width;
		Gtk::TreeModelColumn<Glib::ustring> m_col_surface;
		Gtk::TreeModelColumn<Point> m_col_coord;
		Gtk::TreeModelColumn<uint16_t> m_col_hdg;
		Gtk::TreeModelColumn<int32_t> m_col_elev;
		Gtk::TreeModelColumn<bool> m_col_flags_usable;
	};

	class AirportCommsModelColumns : public Gtk::TreeModelColumnRecord {
	public:
		AirportCommsModelColumns(void);
		Gtk::TreeModelColumn<int> m_col_index;
		Gtk::TreeModelColumn<uint32_t> m_col_freq0;
		Gtk::TreeModelColumn<uint32_t> m_col_freq1;
		Gtk::TreeModelColumn<uint32_t> m_col_freq2;
		Gtk::TreeModelColumn<uint32_t> m_col_freq3;
		Gtk::TreeModelColumn<uint32_t> m_col_freq4;
		Gtk::TreeModelColumn<Glib::ustring> m_col_name;
		Gtk::TreeModelColumn<Glib::ustring> m_col_sector;
		Gtk::TreeModelColumn<Glib::ustring> m_col_oprhours;
	};

	class AirportVFRRoutesModelColumns : public Gtk::TreeModelColumnRecord {
	public:
		AirportVFRRoutesModelColumns(void);
		Gtk::TreeModelColumn<int> m_col_index;
		Gtk::TreeModelColumn<Glib::ustring> m_col_name;
		Gtk::TreeModelColumn<int32_t> m_col_minalt;
		Gtk::TreeModelColumn<int32_t> m_col_maxalt;
		Gtk::TreeModelColumn<Glib::ustring> m_col_cpcoord;
		Gtk::TreeModelColumn<Glib::ustring> m_col_cpalt;
		Gtk::TreeModelColumn<Glib::ustring> m_col_cppath;
		Gtk::TreeModelColumn<Glib::ustring> m_col_cpname;
	};

	class AirportVFRRouteModelColumns : public Gtk::TreeModelColumnRecord {
	public:
		AirportVFRRouteModelColumns(void);
		Gtk::TreeModelColumn<int> m_col_index;
		Gtk::TreeModelColumn<Glib::ustring> m_col_name;
		Gtk::TreeModelColumn<Point> m_col_coord;
		Gtk::TreeModelColumn<AirportsDb::Airport::label_placement_t> m_col_label_placement;
		Gtk::TreeModelColumn<int32_t> m_col_alt;
		Gtk::TreeModelColumn<AirportsDb::Airport::VFRRoute::VFRRoutePoint::altcode_t> m_col_altcode;
		Gtk::TreeModelColumn<AirportsDb::Airport::VFRRoute::VFRRoutePoint::pathcode_t> m_col_pathcode;
		Gtk::TreeModelColumn<char> m_col_ptsym;
		Gtk::TreeModelColumn<bool> m_col_at_airport;
	};

	class AirportVFRPointListModelColumns : public Gtk::TreeModelColumnRecord {
	public:
		AirportVFRPointListModelColumns(void);
		Gtk::TreeModelColumn<Glib::ustring> m_col_name;
		Gtk::TreeModelColumn<Point> m_col_coord;
		Gtk::TreeModelColumn<AirportsDb::Airport::label_placement_t> m_col_label_placement;
		Gtk::TreeModelColumn<bool> m_col_at_airport;
	};

	class PointListEntry {
	public:
		PointListEntry(const Glib::ustring& name = "", AirportsDb::Airport::label_placement_t lp = AirportsDb::Airport::label_off, const Point& pt = Point(), bool atapt = false);
		const Glib::ustring& get_name(void) const { return m_name; }
		Point get_coord(void) const { return m_coord; }
		AirportsDb::Airport::label_placement_t get_label_placement(void) const { return m_labelplacement; }
		bool is_at_airport(void) const { return m_at_airport; }
		bool operator<(const PointListEntry& pe) const;

	private:
		Glib::ustring m_name;
		Point m_coord;
		AirportsDb::Airport::label_placement_t m_labelplacement;
		bool m_at_airport;
	};

	class AsyncElevUpdate {
	public:
		AsyncElevUpdate(Engine& eng, AirportsDbQueryInterface& db, const AirportsDb::Address& addr, const sigc::slot<void,AirportsDb::Airport&>& save);
		~AsyncElevUpdate();
		void cancel(void);
		bool is_done(void);
		void reference(void);
		void unreference(void);

	protected:
		AirportsDbQueryInterface& m_db;
		AirportsDb::Address m_addr;
		Glib::RefPtr<Engine::ElevationResult> m_async;
		Glib::Dispatcher m_dispatch;
		sigc::slot<void,AirportsDb::Airport&> m_save;
		int m_refcnt;
		bool m_asyncref;

		void callback_dispatch(void);
		void callback(void);
	};

	class RunwayDimensions;

	static Glib::ustring get_class_string(const AirspacesDb::Airspace& e) { return AirspaceClassRenderer::get_class_string(e.get_bdryclass(), e.get_typecode()); }

	void hide_notebook_pages(void);
	void show_notebook_pages(void);
	void notebook_switch_page(guint page_num);
	void col_latlon1_clicked(void);
	void buttonclear_clicked(void);
	void find_changed(void);
	void menu_quit_activate(void);
	void menu_undo_activate(void);
	void menu_redo_activate(void);
	void menu_newairport_activate(void);
	void menu_deleteairport_activate(void);
	void menu_airportsetelev_activate(void);
	void menu_cut_activate(void);
	void menu_copy_activate(void);
	void menu_paste_activate(void);
	void menu_delete_activate(void);
	void menu_selectall_activate(void);
	void menu_unselectall_activate(void);
	void menu_mapenable_toggled(void);
	void menu_airportdiagramenable_toggled(void);
	void menu_mapzoomin_activate(void);
	void menu_mapzoomout_activate(void);
	void menu_newrunway_activate(void);
	void menu_deleterunway_activate(void);
	void menu_runwaydim_activate(void);
	void menu_runwayhetole_activate(void);
	void menu_runwayletohe_activate(void);
	void menu_runwaycoords_activate(void);
	void menu_newhelipad_activate(void);
	void menu_deletehelipad_activate(void);
	void menu_newcomm_activate(void);
	void menu_deletecomm_activate(void);
	void menu_newvfrroute_activate(void);
	void menu_deletevfrroute_activate(void);
	void menu_duplicatevfrroute_activate(void);
	void menu_reversevfrroute_activate(void);
	void menu_namevfrroute_activate(void);
	void menu_duplicatereverseandnameallvfrroutes_activate(void);
	void menu_appendvfrroutepoint_activate(void);
	void menu_insertvfrroutepoint_activate(void);
	void menu_deletevfrroutepoint_activate(void);
	void menu_prefs_activate(void);
	void menu_about_activate(void);
	void aboutdialog_response(int response);
	void set_row(Gtk::TreeModel::Row& row, const AirportsDb::Airport& e);
	void set_row_frules_flags(Gtk::TreeModel::Row& row, const AirportsDb::Airport& e);
	void set_rwy(const AirportsDb::Airport& e);
	void set_rwy_flags(Gtk::TreeModel::Row& row, const AirportsDb::Airport::Runway& rwy);
	void set_hp(const AirportsDb::Airport& e);
	void set_hp_flags(Gtk::TreeModel::Row& row, const AirportsDb::Airport::Helipad& hp);
	void set_comm(const AirportsDb::Airport& e);
	void set_vfrroutes(const AirportsDb::Airport& e);
	void set_vfrroute(const AirportsDb::Airport& e);
	void set_vfrpointlist(const AirportsDb::Airport& e);
	void set_lists(const AirportsDb::Airport& e);
	void dbselect_byname(const Glib::ustring& s, AirportsDb::comp_t comp = AirportsDb::comp_contains);
	void dbselect_byrect(const Rect& r);
	void dbselect_bytime(time_t timefrom, time_t timeto);

	void edited_coord(const Glib::ustring& path_string, const Glib::ustring& new_text, Point new_coord);
	void edited_label_placement(const Glib::ustring& path_string, const Glib::ustring& new_text, NavaidsDb::Navaid::label_placement_t new_placement);
	void edited_sourceid(const Glib::ustring& path_string, const Glib::ustring& new_text);
	void edited_icao(const Glib::ustring& path_string, const Glib::ustring& new_text);
	void edited_name(const Glib::ustring& path_string, const Glib::ustring& new_text);
	void edited_elev(const Glib::ustring& path_string, const Glib::ustring& new_text);
	void edited_typecode(const Glib::ustring& path_string, const Glib::ustring& new_text, char new_typecode);
	void edited_frules(const Glib::ustring& path_string, uint8_t f);
	void edited_modtime(const Glib::ustring& path_string, const Glib::ustring& new_text, time_t new_time);

	bool edited_vfrremark_focus(GdkEventFocus* event);

	void edited_rwy_dimensions(const AirportsDb::Airport::Runway& rwyn);
	void edited_rwy_identhe(const Glib::ustring& path_string, const Glib::ustring& new_text);
	void edited_rwy_identle(const Glib::ustring& path_string, const Glib::ustring& new_text);
	void edited_rwy_length(const Glib::ustring& path_string, const Glib::ustring& new_text);
	void edited_rwy_width(const Glib::ustring& path_string, const Glib::ustring& new_text);
	void edited_rwy_surface(const Glib::ustring& path_string, const Glib::ustring& new_text);
	void edited_rwy_hecoord(const Glib::ustring& path_string, const Glib::ustring& new_text, Point new_coord);
	void edited_rwy_lecoord(const Glib::ustring& path_string, const Glib::ustring& new_text, Point new_coord);
	void edited_rwy_hetda(const Glib::ustring& path_string, const Glib::ustring& new_text);
	void edited_rwy_helda(const Glib::ustring& path_string, const Glib::ustring& new_text);
	void edited_rwy_hedisp(const Glib::ustring& path_string, const Glib::ustring& new_text);
	void edited_rwy_hehdg(const Glib::ustring& path_string, const Glib::ustring& new_text, uint16_t new_angle);
	void edited_rwy_heelev(const Glib::ustring& path_string, const Glib::ustring& new_text);
	void edited_rwy_letda(const Glib::ustring& path_string, const Glib::ustring& new_text);
	void edited_rwy_lelda(const Glib::ustring& path_string, const Glib::ustring& new_text);
	void edited_rwy_ledisp(const Glib::ustring& path_string, const Glib::ustring& new_text);
	void edited_rwy_lehdg(const Glib::ustring& path_string, const Glib::ustring& new_text, uint16_t new_angle);
	void edited_rwy_leelev(const Glib::ustring& path_string, const Glib::ustring& new_text);
	void edited_rwy_flags(const Glib::ustring& path_string, uint16_t flag);
	void edited_rwy_helight(const Glib::ustring& path_string, const Glib::ustring& new_text, AirportsDb::Airport::Runway::light_t new_light, unsigned int index);
	void edited_rwy_lelight(const Glib::ustring& path_string, const Glib::ustring& new_text, AirportsDb::Airport::Runway::light_t new_light, unsigned int index);

	void edited_hp_ident(const Glib::ustring& path_string, const Glib::ustring& new_text);
	void edited_hp_length(const Glib::ustring& path_string, const Glib::ustring& new_text);
	void edited_hp_width(const Glib::ustring& path_string, const Glib::ustring& new_text);
	void edited_hp_surface(const Glib::ustring& path_string, const Glib::ustring& new_text);
	void edited_hp_coord(const Glib::ustring& path_string, const Glib::ustring& new_text, Point new_coord);
	void edited_hp_hdg(const Glib::ustring& path_string, const Glib::ustring& new_text, uint16_t new_angle);
	void edited_hp_elev(const Glib::ustring& path_string, const Glib::ustring& new_text);
	void edited_hp_flags(const Glib::ustring& path_string, uint16_t flag);

	void edited_comm_name(const Glib::ustring& path_string, const Glib::ustring& new_text);
	void edited_comm_freq0(const Glib::ustring& path_string, const Glib::ustring& new_text, uint32_t new_freq);
	void edited_comm_freq1(const Glib::ustring& path_string, const Glib::ustring& new_text, uint32_t new_freq);
	void edited_comm_freq2(const Glib::ustring& path_string, const Glib::ustring& new_text, uint32_t new_freq);
	void edited_comm_freq3(const Glib::ustring& path_string, const Glib::ustring& new_text, uint32_t new_freq);
	void edited_comm_freq4(const Glib::ustring& path_string, const Glib::ustring& new_text, uint32_t new_freq);
	void edited_comm_sector(const Glib::ustring& path_string, const Glib::ustring& new_text);
	void edited_comm_oprhours(const Glib::ustring& path_string, const Glib::ustring& new_text);

	void edited_vfrroutes_name(const Glib::ustring& path_string, const Glib::ustring& new_text);
	void edited_vfrroutes_minalt(const Glib::ustring& path_string, const Glib::ustring& new_text);
	void edited_vfrroutes_maxalt(const Glib::ustring& path_string, const Glib::ustring& new_text);

	void edited_vfrroute_name(const Glib::ustring& path_string, const Glib::ustring& new_text);
	void edited_vfrroute_coord(const Glib::ustring& path_string, const Glib::ustring& new_text, Point new_coord);
	void edited_vfrroute_label_placement(const Glib::ustring& path_string, const Glib::ustring& new_text, NavaidsDb::Navaid::label_placement_t new_placement);
	void edited_vfrroute_alt(const Glib::ustring& path_string, const Glib::ustring& new_text);
	void edited_vfrroute_altcode(const Glib::ustring& path_string, const Glib::ustring& new_text, AirportsDb::Airport::VFRRoute::VFRRoutePoint::altcode_t new_altcode);
	void edited_vfrroute_pathcode(const Glib::ustring& path_string, const Glib::ustring& new_text, AirportsDb::Airport::VFRRoute::VFRRoutePoint::pathcode_t new_pathcode);
	void edited_vfrroute_ptsym(const Glib::ustring& path_string, const Glib::ustring& new_text, char new_ptsym);
	void edited_vfrroute_atairport(const Glib::ustring& path_string);
	void edited_vfrroute_pointlist(void);

	void map_cursor(Point cursor, VectorMapArea::cursor_validity_t valid);
	void map_drawflags(int flags);

	void save(AirportsDb::Airport& e);
	void save_nostack(AirportsDb::Airport& e);
	void save_noerr(AirportsDb::Airport& e, Gtk::TreeModel::Row& row);
	void save_noerr_lists(AirportsDb::Airport& e);
	void update_undoredo_menu(void);
	void airporteditor_selection_changed(void);
	void select(const AirportsDb::Airport& e);
	void airporteditor_runway_selection_changed(void);
	void airporteditor_helipad_selection_changed(void);
	void airporteditor_comm_selection_changed(void);
	void airporteditor_vfrroutes_selection_changed(void);
	void airporteditor_vfrroute_selection_changed(void);
	void new_runway(void);
	void new_helipad(void);
	void new_comm(void);
	void new_vfrroutes(void);
	void new_vfrroute(int index);

	std::unique_ptr<AirportsDbQueryInterface> m_db;
#ifdef HAVE_PQXX
	std::unique_ptr<pqxx::connection> m_pgconn;
#endif
	bool m_dbchanged;
	Engine m_engine;
	UndoRedoStack<AirportsDb::Airport> m_undoredo;
	DbSelectDialog m_dbselectdialog;
	Glib::RefPtr<PrefsWindow> m_prefswindow;
	Glib::RefPtr<RunwayDimensions> m_runwaydim;
	Glib::RefPtr<Builder> m_refxml;
	DbEditorWindow *m_airporteditorwindow;
	Gtk::TreeView *m_airporteditortreeview;
	Gtk::Statusbar *m_airporteditorstatusbar;
	Gtk::Entry *m_airporteditorfind;
	Gtk::Notebook *m_airporteditornotebook;
	Gtk::TextView *m_airporteditorvfrremark;
	Gtk::TreeView *m_airportrunwayeditortreeview;
	Gtk::TreeView *m_airporthelipadeditortreeview;
	Gtk::TreeView *m_airportcommeditortreeview;
	Gtk::TreeView *m_airportvfrrouteseditortreeview;
	Gtk::TreeView *m_airportvfrrouteeditortreeview;
	Gtk::Button *m_airportvfrrouteeditorappend;
	Gtk::Button *m_airportvfrrouteeditorinsert;
	Gtk::Button *m_airportvfrrouteeditordelete;
	Gtk::ComboBox *m_airportvfrrouteeditorpointlist;
	sigc::connection m_airportvfrrouteeditorpointlist_conn;
	VectorMapArea *m_airporteditormap;
	Gtk::MenuItem *m_airporteditorundo;
	Gtk::MenuItem *m_airporteditorredo;
	Gtk::CheckMenuItem *m_airporteditormapenable;
	Gtk::CheckMenuItem *m_airporteditorairportdiagramenable;
	Gtk::MenuItem *m_airporteditormapzoomin;
	Gtk::MenuItem *m_airporteditormapzoomout;
	Gtk::MenuItem *m_airporteditornewairport;
	Gtk::MenuItem *m_airporteditordeleteairport;
	Gtk::MenuItem *m_airporteditorairportsetelev;
	Gtk::MenuItem *m_airporteditornewrunway;
	Gtk::MenuItem *m_airporteditordeleterunway;
	Gtk::MenuItem *m_airporteditorrunwaydim;
	Gtk::MenuItem *m_airporteditorrunwayhetole;
	Gtk::MenuItem *m_airporteditorrunwayletohe;
	Gtk::MenuItem *m_airporteditorrunwaycoords;
	Gtk::MenuItem *m_airporteditornewhelipad;
	Gtk::MenuItem *m_airporteditordeletehelipad;
	Gtk::MenuItem *m_airporteditornewcomm;
	Gtk::MenuItem *m_airporteditordeletecomm;
	Gtk::MenuItem *m_airporteditornewvfrroute;
	Gtk::MenuItem *m_airporteditordeletevfrroute;
	Gtk::MenuItem *m_airporteditorduplicatevfrroute;
	Gtk::MenuItem *m_airporteditorreversevfrroute;
	Gtk::MenuItem *m_airporteditornamevfrroute;
	Gtk::MenuItem *m_airporteditorduplicatereverseandnameallvfrroutes;
	Gtk::MenuItem *m_airporteditorappendvfrroutepoint;
	Gtk::MenuItem *m_airporteditorinsertvfrroutepoint;
	Gtk::MenuItem *m_airporteditordeletevfrroutepoint;
	Gtk::MenuItem *m_airporteditorselectall;
	Gtk::MenuItem *m_airporteditorunselectall;
	Gtk::AboutDialog *m_aboutdialog;
	AirportsDb::Address m_curentryid;
	int m_vfrroute_index;
	AirportModelColumns m_airportlistcolumns;
	Glib::RefPtr<Gtk::ListStore> m_airportliststore;
	AirportRunwayModelColumns m_airportrunway_model_columns;
	Glib::RefPtr<Gtk::ListStore> m_airportrunway_model;
	AirportHelipadModelColumns m_airporthelipad_model_columns;
	Glib::RefPtr<Gtk::ListStore> m_airporthelipad_model;
	AirportCommsModelColumns m_airportcomm_model_columns;
	Glib::RefPtr<Gtk::ListStore> m_airportcomm_model;
	AirportVFRRoutesModelColumns m_airportvfrroutes_model_columns;
	Glib::RefPtr<Gtk::ListStore> m_airportvfrroutes_model;
	AirportVFRRouteModelColumns m_airportvfrroute_model_columns;
	Glib::RefPtr<Gtk::ListStore> m_airportvfrroute_model;
	AirportVFRPointListModelColumns m_airportvfrpointlist_model_columns;
	Glib::RefPtr<Gtk::ListStore> m_airportvfrpointlist_model;
};

class TrackEditor : public DbEditor {
public:
	TrackEditor(const std::string& dir_main = "", Engine::auxdb_mode_t auxdbmode = Engine::auxdb_prefs, const std::string& dir_aux = "");
	~TrackEditor();
	Gtk::Window *get_mainwindow(void) { return m_trackeditorwindow; }
	const Gtk::Window *get_mainwindow(void) const { return m_trackeditorwindow; }

private:
#ifdef HAVE_HILDONMM
	typedef Hildon::Window toplevel_window_t;
#else
	typedef Gtk::Window toplevel_window_t;
#endif

	class FindWindow : public toplevel_window_t {
	public:
		static constexpr float block_distance = 0.01;
		static constexpr float max_distance = 10.0;
		static const unsigned int max_number = 25;

		explicit FindWindow(BaseObjectType *castitem, const Glib::RefPtr<Builder>& refxml);
		~FindWindow();
		void set_engine(Engine& eng);
		void present(const TracksDb::Track& track);
		sigc::signal<void,TracksDb::Track&>& signal_save(void) { return m_signal_save; }

	protected:
		static void time_changed(Gtk::Entry *entry);
		void buttonok_clicked(void);
		void buttoncancel_clicked(void);
		void findfrom_changed(void);
		void findto_changed(void);
		void set_offblock(void);
		void set_takeoff(void);
		void set_landing(void);
		void set_onblock(void);
		void async_dispatchdone(void);
		void async_done(void);
		void async_cancel(void);
		void compute_offblock(void);
		void compute_takeoff(void);
		void compute_landing(void);
		void compute_onblock(void);

		void on_my_hide(void);
		bool on_my_delete_event(GdkEventAny* event);

	private:
		class NameIcaoModelColumns : public Gtk::TreeModelColumnRecord {
		public:
			NameIcaoModelColumns(void);
			Gtk::TreeModelColumn<bool> m_col_selectable;
			Gtk::TreeModelColumn<Glib::ustring> m_col_icao;
			Gtk::TreeModelColumn<Glib::ustring> m_col_name;
			Gtk::TreeModelColumn<Glib::ustring> m_col_dist;
		};

	protected:
		Glib::RefPtr<Builder> m_refxml;
		Engine *m_engine;
		TracksDb::Track m_track;
		Gtk::Button *m_trackeditorfindsetoffblock;
		Gtk::Button *m_trackeditorfindsettakeoff;
		Gtk::Button *m_trackeditorfindsetlanding;
		Gtk::Button *m_btrackeditorfindsetonblock;
		Gtk::Entry *m_trackeditorfindoffblockcomp;
		Gtk::Entry *m_trackeditorfindtakeoffcomp;
		Gtk::Entry *m_trackeditorfindlandingcomp;
		Gtk::Entry *m_trackeditorfindonblockcomp;
		Gtk::Entry *m_trackeditorfindonblock;
		Gtk::Entry *m_trackeditorfindlanding;
		Gtk::Entry *m_trackeditorfindtakeoff;
		Gtk::Entry *m_trackeditorfindoffblock;
		Gtk::Entry *m_trackeditorfindtoname;
		Gtk::Entry *m_trackeditorfindtoicao;
		Gtk::Entry *m_trackeditorfindfromicao;
		Gtk::Entry *m_trackeditorfindfromname;
		Gtk::ComboBox *m_trackeditorfindtobox;
		Gtk::ComboBox *m_trackeditorfindfrombox;
		Gtk::Button *m_trackeditorfindbcancel;
		Gtk::Button *m_trackeditorfindbok;
		sigc::signal<void,TracksDb::Track&> m_signal_save;
		NameIcaoModelColumns m_nameicaomodelcolumns;
		Glib::RefPtr<Gtk::TreeStore> m_nameicaomodelstorefrom;
		Glib::RefPtr<Gtk::TreeStore> m_nameicaomodelstoreto;
		Glib::Dispatcher m_querydispatch;
		Glib::RefPtr<Engine::AirportResult> m_airportqueryfrom;
		Glib::RefPtr<Engine::AirportResult> m_airportqueryto;
		Glib::RefPtr<Engine::NavaidResult> m_navaidqueryfrom;
		Glib::RefPtr<Engine::NavaidResult> m_navaidqueryto;
		Glib::RefPtr<Engine::WaypointResult> m_waypointqueryfrom;
		Glib::RefPtr<Engine::WaypointResult> m_waypointqueryto;
		Glib::RefPtr<Engine::MapelementResult> m_mapelementqueryfrom;
		Glib::RefPtr<Engine::MapelementResult> m_mapelementqueryto;
	};

	class KMLExporter;

	class TrackModelColumns : public Gtk::TreeModelColumnRecord {
	public:
		TrackModelColumns(void);
		Gtk::TreeModelColumn<TracksDb::Address> m_col_id;
		Gtk::TreeModelColumn<Glib::ustring> m_col_fromicao;
		Gtk::TreeModelColumn<Glib::ustring> m_col_fromname;
		Gtk::TreeModelColumn<Glib::ustring> m_col_toicao;
		Gtk::TreeModelColumn<Glib::ustring> m_col_toname;
		Gtk::TreeModelColumn<time_t> m_col_offblocktime;
		Gtk::TreeModelColumn<time_t> m_col_takeofftime;
		Gtk::TreeModelColumn<time_t> m_col_landingtime;
		Gtk::TreeModelColumn<time_t> m_col_onblocktime;
		Gtk::TreeModelColumn<Glib::ustring> m_col_sourceid;
		Gtk::TreeModelColumn<time_t> m_col_modtime;
		Gtk::TreeModelColumn<int> m_col_nrpoints;
		Gtk::TreeModelColumn<Glib::ustring> m_col_searchname;
	};

	class TrackPointModelColumns : public Gtk::TreeModelColumnRecord {
	public:
		TrackPointModelColumns(void);
		Gtk::TreeModelColumn<int> m_col_index;
		Gtk::TreeModelColumn<Point> m_col_coord;
		Gtk::TreeModelColumn<Glib::ustring> m_col_alt;
		Gtk::TreeModelColumn<uint64_t> m_col_time;
	};

	void hide_notebook_pages(void);
	void show_notebook_pages(void);
	void notebook_switch_page(guint page_num);
	void update_statistics(void);
	void col_latlon1_clicked(void);
	void buttonclear_clicked(void);
	void find_changed(void);
	void menu_exportkml_activate(void);
	void menu_quit_activate(void);
	void menu_undo_activate(void);
	void menu_redo_activate(void);
	void menu_newtrack_activate(void);
	void menu_deletetrack_activate(void);
	void menu_duplicatetrack_activate(void);
	void menu_mergetracks_activate(void);
	void menu_cut_activate(void);
	void menu_copy_activate(void);
	void menu_paste_activate(void);
	void menu_delete_activate(void);
	void menu_selectall_activate(void);
	void menu_unselectall_activate(void);
	void menu_mapenable_toggled(void);
	void menu_airportdiagramenable_toggled(void);
	void menu_mapzoomin_activate(void);
	void menu_mapzoomout_activate(void);
	void menu_appendtrackpoint_activate(void);
	void menu_inserttrackpoint_activate(void);
	void menu_deletetrackpoint_activate(void);
	void menu_prefs_activate(void);
	void menu_about_activate(void);
	void menu_compute_activate(void);
	void menu_createwaypoint_activate(void);
	void aboutdialog_response(int response);
	void set_row(Gtk::TreeModel::Row& row, const TracksDb::Track& e);
	void set_trackpoints(const TracksDb::Track& e);
	void set_lists(const TracksDb::Track& e);
	void dbselect_byname(const Glib::ustring& s, TracksDb::comp_t comp = TracksDb::comp_contains);
	void dbselect_byrect(const Rect& r);
	void dbselect_bytime(time_t timefrom, time_t timeto);

	void edited_fromicao(const Glib::ustring& path_string, const Glib::ustring& new_text);
	void edited_fromname(const Glib::ustring& path_string, const Glib::ustring& new_text);
	void edited_toicao(const Glib::ustring& path_string, const Glib::ustring& new_text);
	void edited_toname(const Glib::ustring& path_string, const Glib::ustring& new_text);
	void edited_offblocktime(const Glib::ustring& path_string, const Glib::ustring& new_text, time_t new_time);
	void edited_takeofftime(const Glib::ustring& path_string, const Glib::ustring& new_text, time_t new_time);
	void edited_landingtime(const Glib::ustring& path_string, const Glib::ustring& new_text, time_t new_time);
	void edited_onblocktime(const Glib::ustring& path_string, const Glib::ustring& new_text, time_t new_time);
	void edited_sourceid(const Glib::ustring& path_string, const Glib::ustring& new_text);
	void edited_modtime(const Glib::ustring& path_string, const Glib::ustring& new_text, time_t new_time);

	void edited_trackpoint_coord(const Glib::ustring& path_string, const Glib::ustring& new_text, Point new_coord);
	void edited_trackpoint_alt(const Glib::ustring& path_string, const Glib::ustring& new_text);
	void edited_trackpoint_time(const Glib::ustring& path_string, const Glib::ustring& new_text, uint64_t new_time);

	bool edited_notes_focus(GdkEventFocus* event);

	void map_cursor(Point cursor, VectorMapArea::cursor_validity_t valid);
	void map_drawflags(int flags);

	void save(TracksDb::Track& e);
	void save_nostack(TracksDb::Track& e);
	void save_noerr(TracksDb::Track& e, Gtk::TreeModel::Row& row);
	void save_noerr_lists(TracksDb::Track& e);
	void update_undoredo_menu(void);
	void trackeditor_selection_changed(void);
	void select(const TracksDb::Track& e);
	void trackeditor_trackpoint_selection_changed(void);
	void new_trackpoint(int index);
	void compute_save(TracksDb::Track& e);

	std::unique_ptr<TracksDbQueryInterface> m_db;
#ifdef HAVE_PQXX
	std::unique_ptr<pqxx::connection> m_pgconn;
#endif
	TracksDb::Track m_track;
	bool m_dbchanged;
	Engine m_engine;
	UndoRedoStack<TracksDb::Track> m_undoredo;
	DbSelectDialog m_dbselectdialog;
	Glib::RefPtr<PrefsWindow> m_prefswindow;
	FindWindow *m_findwindow;
	Glib::RefPtr<Builder> m_refxml;
	DbEditorWindow *m_trackeditorwindow;
	Gtk::TreeView *m_trackeditortreeview;
	Gtk::Statusbar *m_trackeditorstatusbar;
	Gtk::Entry *m_trackeditorfind;
	Gtk::Notebook *m_trackeditornotebook;
	Gtk::TextView *m_trackeditornotes;
	Gtk::TreeView *m_trackpointeditortreeview;
	VectorMapArea *m_trackeditormap;
	Gtk::Entry *m_trackpointeditorstatdist;
	Gtk::Entry *m_trackpointeditorstattime;
	Gtk::Entry *m_trackpointeditorstatspeedavg;
	Gtk::Entry *m_trackpointeditorstatspeedmin;
	Gtk::Entry *m_trackpointeditorstatspeedmax;
	Gtk::Entry *m_trackpointeditorstataltmin;
	Gtk::Entry *m_trackpointeditorstataltmax;
	Gtk::MenuItem *m_trackeditorexportkml;
	Gtk::MenuItem *m_trackeditorundo;
	Gtk::MenuItem *m_trackeditorredo;
	Gtk::CheckMenuItem *m_trackeditormapenable;
	Gtk::CheckMenuItem *m_trackeditorairportdiagramenable;
	Gtk::MenuItem *m_trackeditormapzoomin;
	Gtk::MenuItem *m_trackeditormapzoomout;
	Gtk::MenuItem *m_trackeditornewtrack;
	Gtk::MenuItem *m_trackeditordeletetrack;
	Gtk::MenuItem *m_trackeditorduplicatetrack;
	Gtk::MenuItem *m_trackeditormergetracks;
	Gtk::MenuItem *m_trackeditorcompute;
	Gtk::MenuItem *m_trackeditorappendtrackpoint;
	Gtk::MenuItem *m_trackeditorinserttrackpoint;
	Gtk::MenuItem *m_trackeditordeletetrackpoint;
	Gtk::MenuItem *m_trackeditorcreatewaypoint;
	Gtk::MenuItem *m_trackeditorselectall;
	Gtk::MenuItem *m_trackeditorunselectall;
	Gtk::AboutDialog *m_aboutdialog;
	TracksDb::Address m_curentryid;
	TrackModelColumns m_tracklistcolumns;
	Glib::RefPtr<Gtk::ListStore> m_trackliststore;
	TrackPointModelColumns m_trackpoint_model_columns;
	Glib::RefPtr<Gtk::ListStore> m_trackpoint_model;
};

#endif /* _DBEDITOR_H */
