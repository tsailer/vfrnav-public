//
// C++ Interface: acftperf.h
//
// Description: Simple Aircraft Performance Calculator
//
//
// Author: Thomas Sailer <t.sailer@alumni.ethz.ch>, (C) 2007, 2008, 2009, 2012
//
// Copyright: See COPYING file that comes with this distribution
//
//

#ifndef _ACFTPERF_H
#define _ACFTPERF_H

#include "sysdeps.h"

#include <vector>
#include <string>
#include <gtkmm.h>
#include <sigc++/sigc++.h>
#include <gtk/gtk.h>

#ifdef HAVE_GTKMM2
#include <libglademm.h>
typedef Gnome::Glade::Xml Builder;
#endif

#ifdef HAVE_GTKMM3
typedef Gtk::Builder Builder;
#endif

#ifdef HAVE_OSSO
#include <libosso.h>
#endif
#ifdef HAVE_HILDONMM
#include <hildonmm.h>
#include <hildon-fmmm.h>
#endif

class AircraftPerformance : public sigc::trackable {
        protected:

#ifdef HAVE_HILDONMM
                typedef Hildon::Window toplevel_window_t;
#else
                typedef Gtk::Window toplevel_window_t;
#endif

                class ToplevelWindow : public toplevel_window_t {
                        public:
                                explicit ToplevelWindow(BaseObjectType *castitem, const Glib::RefPtr<Builder>& refxml);
                                virtual ~ToplevelWindow();

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

                struct cglimits {
                        float mass;
                        float arm;
                };

                struct aircraft_type {
                        char icao_designator[6];
                        const char *name;
                        unsigned int flags;
                        float max_baggage;
                        float max_fuel;
                        float arm_frontpax;
                        float arm_rearpax;
                        float arm_baggage;
                        float arm_fuel;
                        const struct cglimits *cgnormal, *cgutility;
                        const char * const *dist_curves;
                        typedef void (AircraftPerformance::*compute_dist_t)(unsigned int curveidx, float density, float densityalt, float mass, float wind);
                        compute_dist_t compute_dist;
                };

                struct aircraft {
                        const struct aircraft_type *acfttype;
                        float empty_mass;
                        float empty_arm;
                };

                struct units {
                        float scale, offset;
                        const char *str;
                };

                class ComboBoxText : public Gtk::ComboBoxText {
                        public:
                                ComboBoxText(BaseObjectType *cobject, const Glib::RefPtr<Builder>& xml);
                                void clear(void);
                                void append(const Glib::ustring& str);
                                void set_units(const struct units *units, unsigned int dflt = 0);

                        protected:
                                class ModelColumns : public Gtk::TreeModelColumnRecord {
                                        public:
                                                ModelColumns(void);
                                                Gtk::TreeModelColumn<Glib::ustring> m_text;
                                };

                                ModelColumns m_model_columns;
                                Glib::RefPtr<Gtk::ListStore> m_model;
                };

        public:
                AircraftPerformance(void);
                ~AircraftPerformance();
                Gtk::Window *get_mainwindow(void) { return m_acftperfwindow; }
                const Gtk::Window *get_mainwindow(void) const { return m_acftperfwindow; }

        private:
                class WeightBalanceArea;
                static constexpr float fuel_specweight = 0.72;  // kg / l
                static const struct units mass_units[];
                static const struct units length_units[];
                static const struct units volume_units[];
                static const struct units altitude_units[];
                static const struct units qnh_units[];
                static const struct units temp_units[];
                static const struct units windspeed_units[];
                static const struct units densityalt_units[];
                static const struct units dist_units[];

                static float to_si(const struct units& unit, float value);
                static float from_si(const struct units& unit, float value);

                static float temp_to_vapor_pressure(float temp);
                static float pressure_to_density_2(float pressure, float temp, float dewpoint);
                static float density_to_pressure_2(float density, float temp, float dewpoint);
                static float relative_humidity(float temp, float dewpoint);

                static const struct cglimits cg_normal_grob115[];
                static const struct cglimits cg_utility_grob115[];
                static const struct cglimits cg_normal_warrior[];
                static const struct cglimits cg_utility_warrior[];
                static const struct cglimits cg_normal_archerII[];
                static const struct cglimits cg_utility_archerII[];
                static const struct cglimits cg_normal_arrow[];
                static const struct cglimits cg_normal_arrow_201[];
                static const struct cglimits cg_normal_slingsby_t67m[];
                static const struct cglimits cg_utility_slingsby_t67m[];
                static const struct cglimits cg_normal_cessna_c152[];
                static const struct cglimits cg_normal_cessna_c172[];
                static const struct cglimits cg_utility_cessna_c172[];
                static const struct cglimits cg_normal_cessna_c172r[];
                static const char *ArcherII_curvelist[];
                static const char *Arrow180_curvelist[];
                static const char *Arrow200_curvelist[];
                static const char *Grob115_curvelist[];
                static const char *SlingsbyT67M_curvelist[];
                static const struct aircraft_type aircraft_warrior;
                static const struct aircraft_type aircraft_archerII;
                static const struct aircraft_type aircraft_arrow_180;
                static const struct aircraft_type aircraft_arrow_200;
                static const struct aircraft_type aircraft_arrow_201;
                static const struct aircraft_type aircraft_grob115;
                static const struct aircraft_type aircraft_slingsby_t67m;
                static const struct aircraft_type aircraft_cessna_c152;
                static const struct aircraft_type aircraft_cessna_c172;
                static const struct aircraft_type aircraft_cessna_c172r;
                static const char *aircraft_registration[];
                static const struct aircraft aircraft[];
                static const struct aircraft_type *aircraft_types[];

                void menu_quit_activate(void);
                void menu_cut_activate(void);
                void menu_copy_activate(void);
                void menu_paste_activate(void);
                void menu_delete_activate(void);
                void menu_about_activate(void);
                void aboutdialog_response(int response);
                void notebook_switch_page(guint page_num);
                void update_weightbalance_aircraft(void);
                void update_weightbalance_pilot(void);
                void update_weightbalance_frontpax(void);
                void update_weightbalance_rearpax1(void);
                void update_weightbalance_rearpax2(void);
                void update_weightbalance_baggage(void);
                void update_weightbalance_minfuel(void);
                void update_weightbalance_maxfuel(void);
                void update_weightbalance_frontunit(void);
                void update_weightbalance_rearunit(void);
                void update_weightbalance_fuelunit(void);
                void update_weightbalance_baggageunit(void);
                void update_weightbalance(void);
                void update_distance_aircraft(void);
                void update_distance_mass(void);
                void update_distance_altitude(void);
                void update_distance_qnh(void);
                void update_distance_oat(void);
                void update_distance_dewpoint(void);
                void update_distance_winddir(void);
                void update_distance_windspeed(void);
                void update_distance_massunit(void);
                void update_distance_altunit(void);
                void update_distance_qnhunit(void);
                void update_distance_tempunit(void);
                void update_distance_windunit(void);
                void update_distance(void);
                void distance_error(void);
                void set_distances(float gndroll, float obstacle);
                void set_distance_debug(const Glib::ustring& str);
                void dist_archerII_compute(unsigned int curveidx, float density, float densityalt, float mass, float wind);
                void dist_arrow180_compute(unsigned int curveidx, float density, float densityalt, float mass, float wind);
                void dist_arrow200_compute(unsigned int curveidx, float density, float densityalt, float mass, float wind);
                void dist_grob115_compute(unsigned int curveidx, float density, float densityalt, float mass, float wind);
                void dist_slingsbyT67M_compute(unsigned int curveidx, float density, float densityalt, float mass, float wind);

                Glib::RefPtr<Builder> m_refxml;
                ToplevelWindow *m_acftperfwindow;
                Gtk::Notebook *m_acftperfnotebook;
                Gtk::Statusbar *m_acftperfstatusbar;
                ComboBoxText *m_acftperfwbacft;
                Gtk::Entry *m_acftperfwbacfttext;
                Gtk::SpinButton *m_acftperfwbpilot;
                Gtk::SpinButton *m_acftperfwbpax;
                Gtk::SpinButton *m_acftperfwbrearpax1;
                Gtk::SpinButton *m_acftperfwbrearpax2;
                Gtk::SpinButton *m_acftperfwbbaggage;
                Gtk::SpinButton *m_acftperfwbminfuel;
                Gtk::SpinButton *m_acftperfwbmaxfuel;
                ComboBoxText *m_acftperfwbfrontunit;
                ComboBoxText *m_acftperfwbrearunit;
                ComboBoxText *m_acftperfwbbaggageunit;
                ComboBoxText *m_acftperfwbfuelunit;
                Gtk::Entry *m_acftperfwbminfuelarm;
                Gtk::Entry *m_acftperfwbmaxfuelarm;
                Gtk::Entry *m_acftperfwbminfuelmass;
                Gtk::Entry *m_acftperfwbmaxfuelmass;
                Gtk::Entry *m_acftperfwbminfuelmoment;
                Gtk::Entry *m_acftperfwbmaxfuelmoment;
                ComboBoxText *m_acftperfwbarmunit;
                ComboBoxText *m_acftperfwbmassunit;
                Gtk::Entry *m_acftperfwbmomentunit;
                WeightBalanceArea *m_acftperfwbdrawingarea;
                ComboBoxText *m_acftperfdistacfttype;
                ComboBoxText *m_acftperfdistoperation;
                Gtk::SpinButton *m_acftperfdistmass;
                Gtk::SpinButton *m_acftperfdistaltitude;
                Gtk::SpinButton *m_acftperfdistqnh;
                Gtk::SpinButton *m_acftperfdistoat;
                Gtk::SpinButton *m_acftperfdistdewpoint;
                Gtk::Entry *m_acftperfdisthumidity;
                ComboBoxText *m_acftperfdistwinddir;
                Gtk::SpinButton *m_acftperfdistwindspeed;
                ComboBoxText *m_acftperfdistmassunit;
                ComboBoxText *m_acftperfdistaltunit;
                ComboBoxText *m_acftperfdistqnhunit;
                ComboBoxText *m_acftperfdisttempunit;
                ComboBoxText *m_acftperfdistwindunit;
                Gtk::Label *m_acftperfdistdensaltlabel;
                Gtk::Entry *m_acftperfdistdensityalt;
                Gtk::Entry *m_acftperfdistgndroll;
                Gtk::Entry *m_acftperfdistobstacle;
                ComboBoxText *m_acftperfdistdensityaltunit;
                ComboBoxText *m_acftperfdistdistunit;
                Gtk::Entry *m_acftperfdistdistunit2;
                Gtk::Entry *m_acftperfdistdebug;
                Gtk::AboutDialog *m_aboutdialog;

                // application state
                float m_wb_pilot;
                float m_wb_frontpax;
                float m_wb_rearpax1;
                float m_wb_rearpax2;
                float m_wb_baggage;
                float m_wb_minfuel;
                float m_wb_maxfuel;
                float m_dist_mass;
                float m_dist_altitude;
                float m_dist_qnh;
                float m_dist_oat;
                float m_dist_dewpoint;
                float m_dist_wind; // negative wind: tailwind
};

#endif /* _ACFTPERF_H */
