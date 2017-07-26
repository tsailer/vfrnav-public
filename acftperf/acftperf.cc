/***************************************************************************
 *   Copyright (C) 2007, 2008, 2009, 2012, 2013 by Thomas Sailer           *
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

#include <iostream>
#include <iomanip>
#include <sstream>
#include <limits>

#include <gtkmm/main.h>
#include <glibmm/optionentry.h>
#include <glibmm/optiongroup.h>
#include <glibmm/optioncontext.h>

#include "acftperf.h"
#include "baro.h"
#include "sysdepsgui.h"

#include "archerII.h"
#include "arrow.h"
#include "grob115b.h"
#include "t67m.h"


 /* --------------------------------------------------------------------- */

constexpr float AircraftPerformance::fuel_specweight;

const struct AircraftPerformance::units AircraftPerformance::mass_units[] = {
        { 1, 0, "kg" },
        { 0.45359237, 0, "lb" },
        { 0, 0, 0 }
};

const struct AircraftPerformance::units AircraftPerformance::length_units[] = {
        { 1, 0, "m" },
        { 0.0254, 0, "in" },
        { 0, 0, 0 }
};

const struct AircraftPerformance::units AircraftPerformance::volume_units[] = {
        { 1, 0, "l" },
        { 3.7854118, 0, "USgal" },
        { 0, 0, 0 }
};

const struct AircraftPerformance::units AircraftPerformance::altitude_units[] = {
        { 1, 0, "m" },
        { 0.3048, 0, "ft" },
        { 0, 0, 0 }
};

const struct AircraftPerformance::units AircraftPerformance::qnh_units[] = {
        { 1, 0, "hPa" },
        { 33.863886, 0, "inHg" },
        { 1.3332239, 0, "mmHg" },
        { 0, 0, 0 }
};

const struct AircraftPerformance::units AircraftPerformance::temp_units[] = {
        { 1, 273.15, "°C" },
        { 0.55556, 255.37222, "°F" },
        { 1, 0, "K" },
        { 0, 0, 0 }
};

const struct AircraftPerformance::units AircraftPerformance::windspeed_units[] = {
        { 1, 0, "m/s" },
        { 1.0/3.6, 0, "km/h" },
        { 0.51444444, 0, "kts" },
        { 0.44704, 0, "mph" },
        { 0, 0, 0 }
};

const struct AircraftPerformance::units AircraftPerformance::densityalt_units[] = {
        { 1, 0, "m" },
        { 0.3048, 0, "ft" },
        { 1, 0, "hPa" },
        { 33.863886, 0, "inHg" },
        { 1.3332239, 0, "mmHg" },
        { 1, 0, "kg/m^3" },
        { 0, 0, 0 }
};

const struct AircraftPerformance::units AircraftPerformance::dist_units[] = {
        { 1, 0, "m" },
        { 0.3048, 0, "ft" },
        { 1852, 0, "nm" },
        { 0, 0, 0 }
};

/* --------------------------------------------------------------------- */

#define CG_MASS_MOMENT(ma,mo) { (ma), ((mo)/(ma)) }
#define CG_MASS_ARM(ma,a)     { (ma), (a) }
#define KG      * 1.0
#define KG_M    * 1.0
#define KG_INCH * 0.0254
#define LB      * 0.45359237
#define LB_INCH * 0.011521246
#define M       * 1.0
#define INCH    * 0.0254

#if 0

/*
 * Bild 6.4
 */

const struct AircraftPerformance::cglimits AircraftPerformance::cg_normal_grob115[] = {
        CG_MASS_MOMENT(750 KG, 150 KG_M),
        CG_MASS_MOMENT(840 KG, 165 KG_M),
        CG_MASS_MOMENT(920 KG, 235 KG_M),
        CG_MASS_MOMENT(920 KG, 275 KG_M),
        CG_MASS_MOMENT(750 KG, 225 KG_M),
        { 0, 0 }
};

const struct AircraftPerformance::cglimits AircraftPerformance::cg_utility_grob115[] = {
        CG_MASS_MOMENT(750 KG, 150 KG_M),
        CG_MASS_MOMENT(800 KG, 158.3 KG_M),
        CG_MASS_MOMENT(850 KG, 215 KG_M),
        CG_MASS_MOMENT(850 KG, 254.5 KG_M),
        CG_MASS_MOMENT(750 KG, 225 KG_M),
        { 0, 0 }
};

#else

/*
 * Bild 6.3
 * lmue = 1.25m??
 */

const struct AircraftPerformance::cglimits AircraftPerformance::cg_normal_grob115[] = {
        CG_MASS_ARM(750 KG, 0.160 * 1.25 M),
        CG_MASS_ARM(840 KG, 0.160 * 1.25 M),
        CG_MASS_ARM(920 KG, 0.205 * 1.25 M),
        CG_MASS_ARM(920 KG, 0.240 * 1.25 M),
        CG_MASS_ARM(750 KG, 0.240 * 1.25 M),
        { 0, 0 }
};

const struct AircraftPerformance::cglimits AircraftPerformance::cg_utility_grob115[] = {
        CG_MASS_ARM(750 KG, 0.160 * 1.25 M),
        CG_MASS_ARM(800 KG, 0.160 * 1.25 M),
        CG_MASS_ARM(850 KG, 0.205 * 1.25 M),
        CG_MASS_ARM(850 KG, 0.240 * 1.25 M),
        CG_MASS_ARM(750 KG, 0.240 * 1.25 M),
        { 0, 0 }
};

#endif

const struct AircraftPerformance::cglimits AircraftPerformance::cg_normal_warrior[] = {
        CG_MASS_ARM(1200 LB, 83 INCH),
        CG_MASS_ARM(1940 LB, 83 INCH),
        CG_MASS_ARM(2325 LB, 87 INCH),
        CG_MASS_ARM(2325 LB, 93 INCH),
        CG_MASS_ARM(1200 LB, 93 INCH),
        { 0, 0 }
};

const struct AircraftPerformance::cglimits AircraftPerformance::cg_utility_warrior[] = {
        CG_MASS_ARM(1200 LB, 83 INCH),
        CG_MASS_ARM(1940 LB, 83 INCH),
        CG_MASS_ARM(2020 LB, 84 INCH),
        CG_MASS_ARM(2020 LB, 93 INCH),
        CG_MASS_ARM(1200 LB, 93 INCH),
        { 0, 0 }
};

/* also valid for Archer III */
const struct AircraftPerformance::cglimits AircraftPerformance::cg_normal_archerII[] = {
        CG_MASS_ARM(1200 LB, 82 INCH),
        CG_MASS_ARM(2050 LB, 82 INCH),
        CG_MASS_ARM(2550 LB, 88.6 INCH),
        CG_MASS_ARM(2550 LB, 93 INCH),
        CG_MASS_ARM(1200 LB, 93 INCH),
        { 0, 0 }
};

const struct AircraftPerformance::cglimits AircraftPerformance::cg_utility_archerII[] = {
        CG_MASS_ARM(1200 LB, 82 INCH),
        CG_MASS_ARM(2050 LB, 82 INCH),
        CG_MASS_ARM(2130 LB, 83 INCH),
        CG_MASS_ARM(2130 LB, 93 INCH),
        CG_MASS_ARM(1200 LB, 93 INCH),
        { 0, 0 }
};

const struct AircraftPerformance::cglimits AircraftPerformance::cg_normal_arrow[] = {
        CG_MASS_ARM(1200 LB, 81 INCH),
        CG_MASS_ARM(1925 LB, 81 INCH),
        CG_MASS_ARM(2600 LB, 90 INCH),
        CG_MASS_ARM(2600 LB, 95.9 INCH),
        CG_MASS_ARM(1200 LB, 95.9 INCH),
        { 0, 0 }
};

const struct AircraftPerformance::cglimits AircraftPerformance::cg_normal_arrow_201[] = {
        CG_MASS_ARM(1400 LB, 85.5 INCH),
        CG_MASS_ARM(2400 LB, 85.5 INCH),
        CG_MASS_ARM(2750 LB, 90 INCH),
        CG_MASS_ARM(2750 LB, 93 INCH),
        CG_MASS_ARM(1400 LB, 93 INCH),
        { 0, 0 }
};

const struct AircraftPerformance::cglimits AircraftPerformance::cg_normal_slingsby_t67m[] = {
        CG_MASS_ARM(600 KG, 0.784 M),
        CG_MASS_ARM(850 KG, 0.784 M),
        CG_MASS_ARM(907 KG, 0.810 M),
        CG_MASS_ARM(907 KG, 0.927 M),
        CG_MASS_ARM(600 KG, 0.927 M),
        { 0, 0 }
};

const struct AircraftPerformance::cglimits AircraftPerformance::cg_utility_slingsby_t67m[] = {
        CG_MASS_ARM(600 KG, 0.784 M),
        CG_MASS_ARM(850 KG, 0.784 M),
        CG_MASS_ARM(884 KG, 0.800 M),
        CG_MASS_ARM(884 KG, 0.927 M),
        CG_MASS_ARM(600 KG, 0.927 M),
        { 0, 0 }
};

const struct AircraftPerformance::cglimits AircraftPerformance::cg_normal_cessna_c152[] = {
        CG_MASS_ARM(1000 LB, 31 INCH),
        CG_MASS_ARM(1350 LB, 31 INCH),
        CG_MASS_ARM(1670 LB, 32.6 INCH),
        CG_MASS_ARM(1670 LB, 36.5 INCH),
        CG_MASS_ARM(1000 LB, 36.5 INCH),
        { 0, 0 }
};

const struct AircraftPerformance::cglimits AircraftPerformance::cg_normal_cessna_c172[] = {
        CG_MASS_ARM(600 KG, 0.895 M),
        CG_MASS_ARM(880 KG, 0.895 M),
        CG_MASS_ARM(1095 KG, 1.005 M),
        CG_MASS_ARM(1095 KG, 1.200 M),
        CG_MASS_ARM(600 KG, 1.200 M),
        { 0, 0 }
};

const struct AircraftPerformance::cglimits AircraftPerformance::cg_utility_cessna_c172[] = {
        CG_MASS_ARM(600 KG, 0.925 M),
        CG_MASS_ARM(880 KG, 0.925 M),
        CG_MASS_ARM(1005 KG, 1.010 M),
        CG_MASS_ARM(1005 KG, 1.150 M),
        CG_MASS_ARM(600 KG, 1.150 M),
        { 0, 0 }
};

const struct AircraftPerformance::cglimits AircraftPerformance::cg_normal_cessna_c172r[] = {
        CG_MASS_ARM(1600 LB, 36 INCH),
        CG_MASS_ARM(1950 LB, 36 INCH),
        CG_MASS_ARM(2650 LB, 39.5 INCH),
        CG_MASS_ARM(2650 LB, 46.5 INCH),
        CG_MASS_ARM(1600 LB, 46.5 INCH),
        { 0, 0 }
};


#undef CG_MASS_MOMENT
#undef KG
#undef KG_M
#undef KG_INCH
#undef LB
#undef LB_INCH
#undef M
#undef INCH

/* --------------------------------------------------------------------- */

const char *AircraftPerformance::ArcherII_curvelist[] = {
        "Takeoff Flaps 0 Concrete",
        "Takeoff Flaps 25 Concrete",
        "Landing Flaps 40 Concrete"
};

const char *AircraftPerformance::Arrow180_curvelist[] = {
        "Takeoff Flaps 0 Concrete",
        "Takeoff Flaps 25 Concrete",
        "Landing Flaps 40 Concrete",
        0
};

const char *AircraftPerformance::Arrow200_curvelist[] = {
        "Takeoff Flaps 0 Concrete",
        "Takeoff Flaps 25 Concrete",
        "Landing Flaps 40 Concrete",
        0
};

const char *AircraftPerformance::Grob115_curvelist[] = {
        "Takeoff Concrete",
        "Landing Concrete",
        0
};

const char *AircraftPerformance::SlingsbyT67M_curvelist[] = {
        "Takeoff Flaps 18 Concrete",
        "Takeoff Flaps 18 Dry Grass",
        "Takeoff Flaps 18 Wet Grass",
        "Landing Flaps 40 Concrete",
        "Landing Flaps 40 Dry Grass",
        "Landing Flaps 40 Wet Grass",
        "Landing Flaps 0 Concrete",
        "Landing Flaps 0 Dry Grass",
        "Landing Flaps 0 Wet Grass",
        0
};

/* --------------------------------------------------------------------- */

#define HAS_PILOT     (1 << 0)
#define HAS_FRONTPAX  (1 << 1)
#define HAS_REARPAX1  (1 << 2)
#define HAS_REARPAX2  (1 << 3)
#define HAS_BAGGAGE   (1 << 4)

#define L
#define USGAL * 3.7854118
#define KG
#define LB * 0.45359237
#define M
#define IN * 0.0254

const struct AircraftPerformance::aircraft_type AircraftPerformance::aircraft_warrior = {
        "PA28", "Piper Warrior",
        HAS_PILOT|HAS_FRONTPAX|HAS_REARPAX1|HAS_REARPAX2|HAS_BAGGAGE,
        200 LB,
        48 USGAL,
        80.5 IN, 118.1 IN, 142.8 IN, 95.0 IN,
        cg_normal_warrior, cg_utility_warrior,
        0, 0
};

const struct AircraftPerformance::aircraft_type AircraftPerformance::aircraft_archerII = {
        "P28A", "Piper Archer II",
        HAS_PILOT|HAS_FRONTPAX|HAS_REARPAX1|HAS_REARPAX2|HAS_BAGGAGE,
        200 LB,
        48 USGAL,
        80.5 IN, 118.1 IN, 142.8 IN, 95.0 IN,
        cg_normal_archerII, cg_utility_archerII,
        ArcherII_curvelist, &AircraftPerformance::dist_archerII_compute
};

const struct AircraftPerformance::aircraft_type AircraftPerformance::aircraft_arrow_180 = {
        "P28R", "Piper Arrow 180",
        HAS_PILOT|HAS_FRONTPAX|HAS_REARPAX1|HAS_REARPAX2|HAS_BAGGAGE,
        200 LB,
        50 USGAL,
        85.5 IN, 118.1 IN, 142.8 IN, 95.0 IN,
        cg_normal_arrow, cg_normal_arrow,
        Arrow180_curvelist, &AircraftPerformance::dist_arrow180_compute
};

const struct AircraftPerformance::aircraft_type AircraftPerformance::aircraft_arrow_200 = {
        "P28R", "Piper Arrow 200",
        HAS_PILOT|HAS_FRONTPAX|HAS_REARPAX1|HAS_REARPAX2|HAS_BAGGAGE,
        200 LB,
        50 USGAL,
        85.5 IN, 118.1 IN, 142.8 IN, 95.0 IN,
        cg_normal_arrow, cg_normal_arrow,
        Arrow200_curvelist, &AircraftPerformance::dist_arrow200_compute
};

const struct AircraftPerformance::aircraft_type AircraftPerformance::aircraft_arrow_201 = {
        "P28R", "Piper Arrow 201",
        HAS_PILOT|HAS_FRONTPAX|HAS_REARPAX1|HAS_REARPAX2|HAS_BAGGAGE,
        200 LB,
        72 USGAL,
        85.5 IN, 118.1 IN, 142.8 IN, 95.0 IN,
        cg_normal_arrow_201, cg_normal_arrow_201,
        0, 0
};

const struct AircraftPerformance::aircraft_type AircraftPerformance::aircraft_grob115 = {
        "G115", "Grob 115",
        HAS_PILOT|HAS_FRONTPAX|HAS_BAGGAGE,
        20 KG,
        107 L,
        0.25 M, 0 M, 0.9 M, 0.89028 M,
        cg_normal_grob115, cg_utility_grob115,
        Grob115_curvelist, &AircraftPerformance::dist_grob115_compute
};

const struct AircraftPerformance::aircraft_type AircraftPerformance::aircraft_slingsby_t67m = {
        "T67M", "Slingsby Firefly",
        HAS_PILOT|HAS_FRONTPAX|HAS_BAGGAGE,
        30 KG,
        83 L,
        1.2 M, 0 M, 1.86667 M, 0.2 M,
        cg_normal_slingsby_t67m, cg_utility_slingsby_t67m,
        SlingsbyT67M_curvelist, &AircraftPerformance::dist_slingsbyT67M_compute
};

const struct AircraftPerformance::aircraft_type AircraftPerformance::aircraft_cessna_c152 = {
        "C152", "Cessna C152",
        HAS_PILOT|HAS_FRONTPAX|HAS_BAGGAGE,
        30 KG,
        24.5 USGAL,
        39 IN, 0 IN, 63.6 IN, 39 IN,
        cg_normal_cessna_c152, cg_normal_cessna_c152,
        0, 0
};

const struct AircraftPerformance::aircraft_type AircraftPerformance::aircraft_cessna_c172 = {
        "C172", "Cessna C172",
        HAS_PILOT|HAS_FRONTPAX|HAS_REARPAX1|HAS_REARPAX2|HAS_BAGGAGE,
        55 KG,
        235 L,
        0.93 M, 1.87 M, 2.40 M, 1.20 M,
        cg_normal_cessna_c172, cg_utility_cessna_c172,
        0, 0
};

const struct AircraftPerformance::aircraft_type AircraftPerformance::aircraft_cessna_c172r = {
        "C172R", "Cessna C172R",
        HAS_PILOT|HAS_FRONTPAX|HAS_BAGGAGE,
        200 LB,
        62 USGAL,
        37 IN, 73 IN, 95 IN, 47.1 IN,
        cg_normal_cessna_c172r, cg_normal_cessna_c172r,
        0, 0
};


#undef L
#undef USGAL
#undef KG
#undef LB
#undef M
#undef IN

const struct AircraftPerformance::aircraft_type *AircraftPerformance::aircraft_types[] = {
        &aircraft_warrior,
        &aircraft_archerII,
        &aircraft_arrow_180,
        &aircraft_arrow_200,
        &aircraft_arrow_201,
        &aircraft_grob115,
        &aircraft_slingsby_t67m,
        &aircraft_cessna_c152,
        &aircraft_cessna_c172,
        &aircraft_cessna_c172r,
        0
};

/* --------------------------------------------------------------------- */

#define CG_MASS_MOMENT(ma,mo) (ma), ((mo)/(ma))
#define KG      * 1.0
#define KG_M    * 1.0
#define KG_INCH * 0.0254
#define LB      * 0.45359237
#define LB_INCH * 0.011521246

#define AIRCRAFT(name,str,typ,cg) static const char acftstr_ ## name[] = #str;
#include "aircraftdb.h"
#undef AIRCRAFT

const char *AircraftPerformance::aircraft_registration[] = {
#define AIRCRAFT(name,str,typ,cg) acftstr_ ## name,
#include "aircraftdb.h"
#undef AIRCRAFT
};

const struct AircraftPerformance::aircraft AircraftPerformance::aircraft[] = {
#define AIRCRAFT(name,str,typ,cg) { &typ, cg },
#include "aircraftdb.h"
#undef AIRCRAFT
};

#undef CG_MASS_MOMENT
#undef KG
#undef KG_M
#undef KG_INCH
#undef LB
#undef LB_INCH

/* --------------------------------------------------------------------- */

class AircraftPerformance::WeightBalanceArea : public Gtk::DrawingArea {
        public:
                explicit WeightBalanceArea(BaseObjectType *castitem, const Glib::RefPtr<Builder>& refxml);
                virtual ~WeightBalanceArea();
                void set_limits(const struct cglimits *normal, const struct cglimits *utility);
                void set_minfuel(float mass, float arm);
                void set_maxfuel(float mass, float arm);

#ifdef HAVE_GTKMM2
	inline bool get_is_drawable(void) const { return is_drawable(); }
#endif


        private:
#ifdef HAVE_GTKMM2
                virtual bool on_expose_event(GdkEventExpose *event);
#endif
		virtual bool on_draw(const Cairo::RefPtr<Cairo::Context>& cr);
                virtual void on_size_allocate(Gtk::Allocation& allocation);
                virtual void on_show(void);
                virtual void on_hide(void);

        protected:
		void redraw(void) { if (get_is_drawable()) queue_draw(); }

        protected:
                const struct cglimits *m_normal;
                const struct cglimits *m_utility;
                float m_minmass;
                float m_maxmass;
                float m_minarm;
                float m_maxarm;
};

AircraftPerformance::WeightBalanceArea::WeightBalanceArea(BaseObjectType * castitem, const Glib::RefPtr< Builder > & refxml)
        : Gtk::DrawingArea(castitem), m_normal(0), m_utility(0), 
          m_minmass(std::numeric_limits<float>::quiet_NaN()), m_maxmass(std::numeric_limits<float>::quiet_NaN()),
          m_minarm(std::numeric_limits<float>::quiet_NaN()), m_maxarm(std::numeric_limits<float>::quiet_NaN())
{
#ifndef GLIBMM_DEFAULT_SIGNAL_HANDLERS_ENABLED
#ifdef HAVE_GTKMM2
        signal_expose_event().connect(sigc::mem_fun(*this, &WeightBalanceArea::on_expose_event));
#endif
#ifdef HAVE_GTKMM3
        signal_draw().connect(sigc::mem_fun(*this, &WeightBalanceArea::on_draw));
#endif
        signal_size_allocate().connect(sigc::mem_fun(*this, &WeightBalanceArea::on_size_allocate));
        signal_show().connect(sigc::mem_fun(*this, &WeightBalanceArea::on_show));
        signal_hide().connect(sigc::mem_fun(*this, &WeightBalanceArea::on_hide));
#endif //GLIBMM_DEFAULT_SIGNAL_HANDLERS_ENABLED
}

AircraftPerformance::WeightBalanceArea::~WeightBalanceArea()
{
}

#ifdef HAVE_GTKMM2
bool AircraftPerformance::WeightBalanceArea::on_expose_event(GdkEventExpose *event)
{
        Glib::RefPtr<Gdk::Window> wnd(get_window());
        if (!wnd)
                return false;
        Cairo::RefPtr<Cairo::Context> cr(wnd->create_cairo_context());
	if (event) {
		GdkRectangle *rects;
		int n_rects;
		gdk_region_get_rectangles(event->region, &rects, &n_rects);
		for (int i = 0; i < n_rects; i++)
			cr->rectangle(rects[i].x, rects[i].y, rects[i].width, rects[i].height);
		cr->clip();
		g_free(rects);
	}
        return on_draw(cr);
}
#endif

bool AircraftPerformance::WeightBalanceArea::on_draw(const Cairo::RefPtr<Cairo::Context>& cr)
{
	if (!cr)
		return true;
	float minmass(std::numeric_limits<float>::max());
        float maxmass(0);
        float minarm(std::numeric_limits<float>::max());
        float maxarm(0);
        if (!std::isnan(m_minmass)) {
                minmass = std::min(minmass, m_minmass);
                maxmass = std::max(maxmass, m_minmass);
        }
        if (!std::isnan(m_maxmass)) {
                minmass = std::min(minmass, m_maxmass);
                maxmass = std::max(maxmass, m_maxmass);
        }
        if (!std::isnan(m_minarm)) {
                minarm = std::min(minarm, m_minarm);
                maxarm = std::max(maxarm, m_minarm);
        }
        if (!std::isnan(m_maxarm)) {
                minarm = std::min(minarm, m_maxarm);
                maxarm = std::max(maxarm, m_maxarm);
        }
        if (m_normal) {
                const struct AircraftPerformance::cglimits *p(m_normal);
                while (p->mass > 0) {
                        minmass = std::min(minmass, p->mass);
                        maxmass = std::max(maxmass, p->mass);
                        minarm = std::min(minarm, p->arm);
                        maxarm = std::max(maxarm, p->arm);
                        ++p;
                }
        }
        if (m_utility) {
                const struct AircraftPerformance::cglimits *p(m_utility);
                while (p->mass > 0) {
                        minmass = std::min(minmass, p->mass);
                        maxmass = std::max(maxmass, p->mass);
                        minarm = std::min(minarm, p->arm);
                        maxarm = std::max(maxarm, p->arm);
                        ++p;
                }
        }
	cr->save();
        cr->set_source_rgb(1.0, 1.0, 1.0);
        cr->paint();
        cr->set_line_width(1.0);
        float massdiff(maxmass - minmass);
        float armdiff(maxarm - minarm);
        if (false)
                std::cerr << "w " << get_width() << " h " << get_height()
                          << " arm: min " << minarm << " max " << maxarm << " mass: min " << minmass << " max " << maxmass << std::endl;
        if (massdiff <= 0 || armdiff <= 0) {
		cr->restore();
                return true;
	}
        float sx(get_width() / (1.1 * armdiff));
        float sy(-get_height() / (1.1 * massdiff));
        float ox(get_width() * 0.05 - minarm * sx);
        float oy(get_height() * 0.05 - maxmass * sy);
        if (m_utility) {
                cr->set_source_rgb(0.0, 0.0, 1.0);
                const struct AircraftPerformance::cglimits *p(m_utility);
                bool first(true);
                while (p->mass > 0) {
                        if (first)
                                cr->move_to(p->arm * sx + ox, p->mass * sy + oy);
                        else
                                cr->line_to(p->arm * sx + ox, p->mass * sy + oy);
                        first = false;
                        ++p;
                }
                cr->close_path();
                cr->stroke();
        }
        if (m_normal) {
                cr->set_source_rgb(0.0, 0.0, 0.0);
                const struct AircraftPerformance::cglimits *p(m_normal);
                bool first(true);
                while (p->mass > 0) {
                        if (first)
                                cr->move_to(p->arm * sx + ox, p->mass * sy + oy);
                        else
                                cr->line_to(p->arm * sx + ox, p->mass * sy + oy);
                        first = false;
                        ++p;
                }
                cr->close_path();
                cr->stroke();
        }
        if (!std::isnan(m_minmass) && !std::isnan(m_maxmass) && !std::isnan(m_minarm) && !std::isnan(m_maxarm)) {
                cr->set_source_rgb(1.0, 0.0, 0.0);
                cr->move_to(m_minarm * sx + ox, m_minmass * sy + oy);
                cr->rel_move_to(2, 2);
                cr->rel_line_to(-4, -4);
                cr->rel_move_to(4, 0);
                cr->rel_line_to(-4, 4);
                cr->rel_move_to(2, -2);
                cr->line_to(m_maxarm * sx + ox, m_maxmass * sy + oy);
                cr->rel_move_to(2, 2);
                cr->rel_line_to(-4, -4);
                cr->rel_move_to(4, 0);
                cr->rel_line_to(-4, 4);
                cr->rel_move_to(2, -2);
                cr->stroke();
        }
	cr->restore();
        return true;
}

void AircraftPerformance::WeightBalanceArea::on_size_allocate( Gtk::Allocation & allocation )
{
#ifdef GLIBMM_DEFAULT_SIGNAL_HANDLERS_ENABLED
	Gtk::DrawingArea::on_size_allocate(allocation);
#endif //GLIBMM_DEFAULT_SIGNAL_HANDLERS_ENABLED
        if (false)
                std::cerr << "map size allocate x " << allocation.get_x() << " y " << allocation.get_y() << std::endl;
}

void AircraftPerformance::WeightBalanceArea::on_show(void)
{
#ifdef GLIBMM_DEFAULT_SIGNAL_HANDLERS_ENABLED
	Gtk::DrawingArea::on_show();
#endif //GLIBMM_DEFAULT_SIGNAL_HANDLERS_ENABLED
        if (false)
                std::cerr << "show" << std::endl;
}

void AircraftPerformance::WeightBalanceArea::on_hide(void)
{
#ifdef GLIBMM_DEFAULT_SIGNAL_HANDLERS_ENABLED
	Gtk::DrawingArea::on_hide();
#endif //GLIBMM_DEFAULT_SIGNAL_HANDLERS_ENABLED
        if (false)
                std::cerr << "hide" << std::endl;
}

void AircraftPerformance::WeightBalanceArea::set_limits(const struct cglimits *normal, const struct cglimits *utility)
{
        m_normal = normal;
        m_utility = utility;
}

void AircraftPerformance::WeightBalanceArea::set_minfuel(float mass, float arm)
{
        m_minmass = mass;
        m_minarm = arm;
        redraw();
}

void AircraftPerformance::WeightBalanceArea::set_maxfuel(float mass, float arm)
{
        m_maxmass = mass;
        m_maxarm = arm;
        redraw();
}

AircraftPerformance::ToplevelWindow::ToplevelWindow(BaseObjectType * castitem, const Glib::RefPtr< Builder > & refxml)
        : toplevel_window_t(castitem), m_fullscreen(false)
{
        signal_window_state_event().connect(sigc::mem_fun(*this, &AircraftPerformance::ToplevelWindow::on_my_window_state_event));
        signal_delete_event().connect(sigc::mem_fun(*this, &AircraftPerformance::ToplevelWindow::on_my_delete_event));
        signal_key_press_event().connect(sigc::mem_fun(*this, &AircraftPerformance::ToplevelWindow::on_my_key_press_event));
}

AircraftPerformance::ToplevelWindow::~ToplevelWindow()
{
}

void AircraftPerformance::ToplevelWindow::toggle_fullscreen(void)
{
        if (m_fullscreen)
                unfullscreen();
        else
                fullscreen();
}

bool AircraftPerformance::ToplevelWindow::on_my_window_state_event(GdkEventWindowState *state)
{
        if (!state)
                return false;
        m_fullscreen = !!(Gdk::WINDOW_STATE_FULLSCREEN & state->new_window_state);
        return false;
}

bool AircraftPerformance::ToplevelWindow::on_my_delete_event(GdkEventAny *event)
{
        hide();
        return true;
}

bool AircraftPerformance::ToplevelWindow::on_my_key_press_event(GdkEventKey * event)
{
        if(!event)
                return false;
        std::cerr << "key press: type " << event->type << " keyval " << event->keyval << std::endl;
        if (event->type != GDK_KEY_PRESS)
                return false;
        switch (event->keyval) {
                case GDK_KEY_F6:
                        toggle_fullscreen();
                        return true;

                case GDK_KEY_F7:
                        return zoom_in();

                case GDK_KEY_F8:
                        return zoom_out();

                default:
                        break;
        }
        return false;
}

AircraftPerformance::ComboBoxText::ModelColumns::ModelColumns(void)
{
        add(m_text);
}

AircraftPerformance::ComboBoxText::ComboBoxText(BaseObjectType *cobject, const Glib::RefPtr<Builder>& xml)
        : Gtk::ComboBoxText(cobject), m_model(Gtk::ListStore::create(m_model_columns))
{
        set_model(m_model);
        Gtk::CellLayout::clear();
        {
                Gtk::CellRendererText *text_renderer = Gtk::manage(new Gtk::CellRendererText());
                text_renderer->set_property("xalign", 0.0);
                pack_start(*text_renderer, true);
                add_attribute(*text_renderer, "text", m_model_columns.m_text);
        }
}

void AircraftPerformance::ComboBoxText::clear(void)
{
        m_model->clear();
}

void AircraftPerformance::ComboBoxText::append(const Glib::ustring & str)
{
        Gtk::TreeModel::iterator iter(m_model->append());
        (*iter)[m_model_columns.m_text] = str;
}

void AircraftPerformance::ComboBoxText::set_units(const struct units *units, unsigned int dflt)
{
        if (!units)
                return;
        clear();
        unsigned int cnt(0);
        while (units->str) {
                append(units->str);
                units++;
                cnt++;
        }
        if (dflt >= cnt)
                dflt = 0;
        set_active(dflt);
}

AircraftPerformance::AircraftPerformance(void)
        : m_acftperfwindow(0), m_acftperfnotebook(0), m_acftperfstatusbar(0),
          m_acftperfwbacft(0), m_acftperfwbacfttext(0),
          m_acftperfwbpilot(0), m_acftperfwbpax(0), m_acftperfwbrearpax1(0), m_acftperfwbrearpax2(0),
          m_acftperfwbbaggage(0), m_acftperfwbminfuel(0), m_acftperfwbmaxfuel(0),
          m_acftperfwbfrontunit(0), m_acftperfwbrearunit(0), m_acftperfwbbaggageunit(0), m_acftperfwbfuelunit(0),
          m_acftperfwbminfuelarm(0), m_acftperfwbmaxfuelarm(0), m_acftperfwbminfuelmass(0), m_acftperfwbmaxfuelmass(0),
          m_acftperfwbminfuelmoment(0), m_acftperfwbmaxfuelmoment(0),
          m_acftperfwbarmunit(0), m_acftperfwbmassunit(0), m_acftperfwbmomentunit(0),
          m_acftperfwbdrawingarea(0),
          m_acftperfdistacfttype(0), m_acftperfdistoperation(0),
          m_acftperfdistmass(0), m_acftperfdistaltitude(0), m_acftperfdistqnh(0),
          m_acftperfdistoat(0), m_acftperfdistdewpoint(0), m_acftperfdisthumidity(0),
          m_acftperfdistwinddir(0), m_acftperfdistwindspeed(0),
          m_acftperfdistmassunit(0), m_acftperfdistaltunit(0), m_acftperfdistqnhunit(0),
          m_acftperfdisttempunit(0), m_acftperfdistwindunit(0),
          m_acftperfdistdensaltlabel(0), m_acftperfdistdensityalt(0), m_acftperfdistgndroll(0), m_acftperfdistobstacle(0),
          m_acftperfdistdensityaltunit(0), m_acftperfdistdistunit(0), m_acftperfdistdistunit2(0),
          m_acftperfdistdebug(0),
          m_aboutdialog(0),
          m_wb_pilot(80.0), m_wb_frontpax(0), m_wb_rearpax1(0), m_wb_rearpax2(0), m_wb_baggage(0), m_wb_minfuel(0), m_wb_maxfuel(0),
          m_dist_mass(0), m_dist_altitude(1760 * 0.3048), m_dist_qnh(1013.25), m_dist_oat(20 + 273.15), m_dist_dewpoint(5 + 273.15), m_dist_wind(0)
{
        m_refxml = load_glade_file("acftperformance" UIEXT, "acftperfwindow");
        m_refxml->get_widget_derived("acftperfwindow", m_acftperfwindow);
        m_refxml->get_widget("acftperfnotebook", m_acftperfnotebook);
        m_refxml->get_widget("acftperfstatusbar", m_acftperfstatusbar);
        m_refxml->get_widget_derived("acftperfwbacft", m_acftperfwbacft);
        m_refxml->get_widget("acftperfwbacfttext", m_acftperfwbacfttext);
        m_refxml->get_widget("acftperfwbpilot", m_acftperfwbpilot);
        m_refxml->get_widget("acftperfwbpax", m_acftperfwbpax);
        m_refxml->get_widget("acftperfwbrearpax1", m_acftperfwbrearpax1);
        m_refxml->get_widget("acftperfwbrearpax2", m_acftperfwbrearpax2);
        m_refxml->get_widget("acftperfwbbaggage", m_acftperfwbbaggage);
        m_refxml->get_widget("acftperfwbminfuel", m_acftperfwbminfuel);
        m_refxml->get_widget("acftperfwbmaxfuel", m_acftperfwbmaxfuel);
        m_refxml->get_widget_derived("acftperfwbfrontunit", m_acftperfwbfrontunit);
        m_refxml->get_widget_derived("acftperfwbrearunit", m_acftperfwbrearunit);
        m_refxml->get_widget_derived("acftperfwbbaggageunit", m_acftperfwbbaggageunit);
        m_refxml->get_widget_derived("acftperfwbfuelunit", m_acftperfwbfuelunit);
        m_refxml->get_widget("acftperfwbminfuelarm", m_acftperfwbminfuelarm);
        m_refxml->get_widget("acftperfwbmaxfuelarm", m_acftperfwbmaxfuelarm);
        m_refxml->get_widget("acftperfwbminfuelmass", m_acftperfwbminfuelmass);
        m_refxml->get_widget("acftperfwbmaxfuelmass", m_acftperfwbmaxfuelmass);
        m_refxml->get_widget("acftperfwbminfuelmoment", m_acftperfwbminfuelmoment);
        m_refxml->get_widget("acftperfwbmaxfuelmoment", m_acftperfwbmaxfuelmoment);
        m_refxml->get_widget_derived("acftperfwbarmunit", m_acftperfwbarmunit);
        m_refxml->get_widget_derived("acftperfwbmassunit", m_acftperfwbmassunit);
        m_refxml->get_widget("acftperfwbmomentunit", m_acftperfwbmomentunit);
        m_refxml->get_widget_derived("acftperfwbdrawingarea", m_acftperfwbdrawingarea);
        m_refxml->get_widget_derived("acftperfdistacfttype", m_acftperfdistacfttype);
        m_refxml->get_widget_derived("acftperfdistoperation", m_acftperfdistoperation);
        m_refxml->get_widget("acftperfdistmass", m_acftperfdistmass);
        m_refxml->get_widget("acftperfdistaltitude", m_acftperfdistaltitude);
        m_refxml->get_widget("acftperfdistqnh", m_acftperfdistqnh);
        m_refxml->get_widget("acftperfdistoat", m_acftperfdistoat);
        m_refxml->get_widget("acftperfdistdewpoint", m_acftperfdistdewpoint);
        m_refxml->get_widget("acftperfdisthumidity", m_acftperfdisthumidity);
        m_refxml->get_widget_derived("acftperfdistwinddir", m_acftperfdistwinddir);
        m_refxml->get_widget("acftperfdistwindspeed", m_acftperfdistwindspeed);
        m_refxml->get_widget_derived("acftperfdistmassunit", m_acftperfdistmassunit);
        m_refxml->get_widget_derived("acftperfdistaltunit", m_acftperfdistaltunit);
        m_refxml->get_widget_derived("acftperfdistqnhunit", m_acftperfdistqnhunit);
        m_refxml->get_widget_derived("acftperfdisttempunit", m_acftperfdisttempunit);
        m_refxml->get_widget_derived("acftperfdistwindunit", m_acftperfdistwindunit);
        m_refxml->get_widget("acftperfdistdensaltlabel", m_acftperfdistdensaltlabel);
        m_refxml->get_widget("acftperfdistdensityalt", m_acftperfdistdensityalt);
        m_refxml->get_widget("acftperfdistgndroll", m_acftperfdistgndroll);
        m_refxml->get_widget("acftperfdistobstacle", m_acftperfdistobstacle);
        m_refxml->get_widget_derived("acftperfdistdensityaltunit", m_acftperfdistdensityaltunit);
        m_refxml->get_widget_derived("acftperfdistdistunit", m_acftperfdistdistunit);
        m_refxml->get_widget("acftperfdistdistunit2", m_acftperfdistdistunit2);
        m_refxml->get_widget("acftperfdistdebug", m_acftperfdistdebug);
        {
#ifdef HAVE_GTKMM2
                Glib::RefPtr<Builder> refxml = load_glade_file_nosearch(m_refxml->get_filename(), "aboutdialog");
#else
		//Glib::RefPtr<Builder> refxml = load_glade_file("acftperformance" UIEXT, "aboutdialog");
		Glib::RefPtr<Builder> refxml(m_refxml);
#endif
                refxml->get_widget("aboutdialog", m_aboutdialog);
                m_aboutdialog->signal_response().connect(sigc::mem_fun(*this, &AircraftPerformance::aboutdialog_response));
        }
        Gtk::MenuItem *menu_quit(0), *menu_cut(0), *menu_copy(0), *menu_paste(0), *menu_delete(0), *menu_about(0);
        m_refxml->get_widget("acftperfmenuquit", menu_quit);
        m_refxml->get_widget("acftperfmenucut", menu_cut);
        m_refxml->get_widget("acftperfmenucopy", menu_copy);
        m_refxml->get_widget("acftperfmenupaste", menu_paste);
        m_refxml->get_widget("acftperfmenudelete", menu_delete);
        m_refxml->get_widget("acftperfmenuabout", menu_about);
        menu_quit->signal_activate().connect(sigc::mem_fun(*this, &AircraftPerformance::menu_quit_activate));
        //m_acftperfwindow->signal_zoomin().connect(sigc::bind_return(sigc::mem_fun(*this, &AircraftPerformance::menu_mapzoomin_activate), true));
        //m_acftperfwindow->signal_zoomout().connect(sigc::bind_return(sigc::mem_fun(*this, &AircraftPerformance::menu_mapzoomout_activate), true));
        menu_cut->signal_activate().connect(sigc::mem_fun(*this, &AircraftPerformance::menu_cut_activate));
        menu_copy->signal_activate().connect(sigc::mem_fun(*this, &AircraftPerformance::menu_copy_activate));
        menu_paste->signal_activate().connect(sigc::mem_fun(*this, &AircraftPerformance::menu_paste_activate));
        menu_delete->signal_activate().connect(sigc::mem_fun(*this, &AircraftPerformance::menu_delete_activate));
        menu_about->signal_activate().connect(sigc::mem_fun(*this, &AircraftPerformance::menu_about_activate));
        // update combo boxes
        m_acftperfwbfrontunit->set_units(mass_units);
        m_acftperfwbrearunit->set_units(mass_units);
        m_acftperfwbbaggageunit->set_units(mass_units);
        m_acftperfwbfuelunit->set_units(volume_units);
        m_acftperfwbarmunit->set_units(length_units);
        m_acftperfwbmassunit->set_units(mass_units);
        m_acftperfdistwinddir->clear();
        m_acftperfdistwinddir->append(_("Headwind"));
        m_acftperfdistwinddir->append(_("Tailwind"));
        m_acftperfdistwinddir->set_active(0);
        m_acftperfdistmassunit->set_units(mass_units);
        m_acftperfdistaltunit->set_units(altitude_units, 1);
        m_acftperfdistqnhunit->set_units(qnh_units);
        m_acftperfdisttempunit->set_units(temp_units);
        m_acftperfdistwindunit->set_units(windspeed_units, 2);
        m_acftperfdistdensityaltunit->set_units(densityalt_units, 1);
        m_acftperfdistdistunit->set_units(dist_units);
        // add aircraft
        m_acftperfwbacft->clear();
        for (unsigned int i = 0; i < sizeof(aircraft_registration)/sizeof(aircraft_registration[0]); ++i) {
                m_acftperfwbacft->append(aircraft_registration[i]);
        }
        m_acftperfwbacft->set_active(0);
        // alignment
        m_acftperfwbminfuelarm->set_alignment(1.0);
        m_acftperfwbmaxfuelarm->set_alignment(1.0);
        m_acftperfwbminfuelmass->set_alignment(1.0);
        m_acftperfwbmaxfuelmass->set_alignment(1.0);
        m_acftperfwbminfuelmoment->set_alignment(1.0);
        m_acftperfwbmaxfuelmoment->set_alignment(1.0);
        // aircraft type list
        m_acftperfdistacfttype->clear();
        for (unsigned int i = 0; aircraft_types[i]; ++i) {
                std::ostringstream oss;
                oss << aircraft_types[i]->icao_designator << ' ' << aircraft_types[i]->name;
                m_acftperfdistacfttype->append(oss.str());
        }
        // alignment
        m_acftperfdistdensityalt->set_alignment(1.0);
        m_acftperfdistgndroll->set_alignment(1.0);
        m_acftperfdistobstacle->set_alignment(1.0);
        // widget signals
        m_acftperfnotebook->signal_switch_page().connect(sigc::hide<0>(sigc::mem_fun(*this, &AircraftPerformance::notebook_switch_page)));
        m_acftperfwbacft->signal_changed().connect(sigc::mem_fun(*this, &AircraftPerformance::update_weightbalance_aircraft));
        m_acftperfwbpilot->signal_value_changed().connect(sigc::mem_fun(*this, &AircraftPerformance::update_weightbalance_pilot));
        m_acftperfwbpax->signal_value_changed().connect(sigc::mem_fun(*this, &AircraftPerformance::update_weightbalance_frontpax));
        m_acftperfwbrearpax1->signal_value_changed().connect(sigc::mem_fun(*this, &AircraftPerformance::update_weightbalance_rearpax1));
        m_acftperfwbrearpax2->signal_value_changed().connect(sigc::mem_fun(*this, &AircraftPerformance::update_weightbalance_rearpax2));
        m_acftperfwbbaggage->signal_value_changed().connect(sigc::mem_fun(*this, &AircraftPerformance::update_weightbalance_baggage));
        m_acftperfwbminfuel->signal_value_changed().connect(sigc::mem_fun(*this, &AircraftPerformance::update_weightbalance_minfuel));
        m_acftperfwbmaxfuel->signal_value_changed().connect(sigc::mem_fun(*this, &AircraftPerformance::update_weightbalance_maxfuel));
        m_acftperfwbfrontunit->signal_changed().connect(sigc::mem_fun(*this, &AircraftPerformance::update_weightbalance_frontunit));
        m_acftperfwbrearunit->signal_changed().connect(sigc::mem_fun(*this, &AircraftPerformance::update_weightbalance_rearunit));
        m_acftperfwbbaggageunit->signal_changed().connect(sigc::mem_fun(*this, &AircraftPerformance::update_weightbalance_baggageunit));
        m_acftperfwbfuelunit->signal_changed().connect(sigc::mem_fun(*this, &AircraftPerformance::update_weightbalance_fuelunit));
        m_acftperfwbarmunit->signal_changed().connect(sigc::mem_fun(*this, &AircraftPerformance::update_weightbalance));
        m_acftperfwbmassunit->signal_changed().connect(sigc::mem_fun(*this, &AircraftPerformance::update_weightbalance));
        m_acftperfdistacfttype->signal_changed().connect(sigc::mem_fun(*this, &AircraftPerformance::update_distance_aircraft));
        m_acftperfdistoperation->signal_changed().connect(sigc::mem_fun(*this, &AircraftPerformance::update_distance));
        m_acftperfdistmass->signal_value_changed().connect(sigc::mem_fun(*this, &AircraftPerformance::update_distance_mass));
        m_acftperfdistaltitude->signal_value_changed().connect(sigc::mem_fun(*this, &AircraftPerformance::update_distance_altitude));
        m_acftperfdistqnh->signal_value_changed().connect(sigc::mem_fun(*this, &AircraftPerformance::update_distance_qnh));
        m_acftperfdistoat->signal_value_changed().connect(sigc::mem_fun(*this, &AircraftPerformance::update_distance_oat));
        m_acftperfdistdewpoint->signal_value_changed().connect(sigc::mem_fun(*this, &AircraftPerformance::update_distance_dewpoint));
        m_acftperfdistwinddir->signal_changed().connect(sigc::mem_fun(*this, &AircraftPerformance::update_distance_winddir));
        m_acftperfdistwindspeed->signal_value_changed().connect(sigc::mem_fun(*this, &AircraftPerformance::update_distance_windspeed));
        m_acftperfdistmassunit->signal_changed().connect(sigc::mem_fun(*this, &AircraftPerformance::update_distance_massunit));
        m_acftperfdistaltunit->signal_changed().connect(sigc::mem_fun(*this, &AircraftPerformance::update_distance_altunit));
        m_acftperfdistqnhunit->signal_changed().connect(sigc::mem_fun(*this, &AircraftPerformance::update_distance_qnhunit));
        m_acftperfdisttempunit->signal_changed().connect(sigc::mem_fun(*this, &AircraftPerformance::update_distance_tempunit));
        m_acftperfdistwindunit->signal_changed().connect(sigc::mem_fun(*this, &AircraftPerformance::update_distance_windunit));
        m_acftperfdistdensityaltunit->signal_changed().connect(sigc::mem_fun(*this, &AircraftPerformance::update_distance));
        m_acftperfdistdistunit->signal_changed().connect(sigc::mem_fun(*this, &AircraftPerformance::update_distance));
        update_weightbalance_aircraft();
}

AircraftPerformance::~AircraftPerformance()
{
}

void AircraftPerformance::menu_quit_activate(void)
{
        m_acftperfwindow->hide();
}

void AircraftPerformance::menu_cut_activate(void)
{
        Gtk::Widget *focus = m_acftperfwindow->get_focus();
        if (!focus)
                return;
        Gtk::Editable *edit = dynamic_cast<Gtk::Editable *>(focus);
        if (!edit)
                return;
        edit->cut_clipboard();
}

void AircraftPerformance::menu_copy_activate(void)
{
        Gtk::Widget *focus = m_acftperfwindow->get_focus();
        if (!focus)
                return;
        Gtk::Editable *edit = dynamic_cast<Gtk::Editable *>(focus);
        if (!edit)
                return;
        edit->copy_clipboard();
}

void AircraftPerformance::menu_paste_activate(void)
{
        Gtk::Widget *focus = m_acftperfwindow->get_focus();
        if (!focus)
                return;
        Gtk::Editable *edit = dynamic_cast<Gtk::Editable *>(focus);
        if (!edit)
                return;
        edit->paste_clipboard();
}

void AircraftPerformance::menu_delete_activate(void)
{
        Gtk::Widget *focus = m_acftperfwindow->get_focus();
        if (!focus)
                return;
        Gtk::Editable *edit = dynamic_cast<Gtk::Editable *>(focus);
        if (!edit)
                return;
        edit->delete_selection();
}

void AircraftPerformance::menu_about_activate(void)
{
        m_aboutdialog->show();
}

void AircraftPerformance::aboutdialog_response( int response )
{
        if (response == Gtk::RESPONSE_CLOSE || response == Gtk::RESPONSE_CANCEL)
                m_aboutdialog->hide();
}

float AircraftPerformance::to_si(const struct units& unit, float value)
{
        return unit.scale * value + unit.offset;
}

float AircraftPerformance::from_si(const struct units& unit, float value)
{
        return (value - unit.offset) / unit.scale;
}

#define CONST_M    28.9644                     /* Molecular Weight of air */
#define CONST_R    8.31432                     /* gas const. for air */
#define CONST_MR   (CONST_M/CONST_R)
#define CONST_RM   (CONST_R/CONST_M)
#define CONST_RM_WATERVAPOR  (0.461495)
#define CONST_MR_WATERVAPOR  (1.0 / CONST_RM_WATERVAPOR)

#define CONST_ZERODEGC  273.15
#define CONST_WOBUS_C0  0.99999683
#define CONST_WOBUS_C1  -0.90826951E-02
#define CONST_WOBUS_C2  0.78736169E-04
#define CONST_WOBUS_C3  -0.61117958E-06
#define CONST_WOBUS_C4  0.43884187E-08
#define CONST_WOBUS_C5  -0.29883885E-10
#define CONST_WOBUS_C6  0.21874425E-12
#define CONST_WOBUS_C7  -0.17892321E-14
#define CONST_WOBUS_C8  0.11112018E-16
#define CONST_WOBUS_C9  -0.30994571E-19
#define CONST_WOBUS_ES0 6.1078

float AircraftPerformance::temp_to_vapor_pressure(float temp)
{
        temp -= CONST_ZERODEGC;
        float pol = CONST_WOBUS_C0 + temp * (CONST_WOBUS_C1 + temp *
                        (CONST_WOBUS_C2 + temp *
                        (CONST_WOBUS_C3 + temp *
                        (CONST_WOBUS_C4 + temp *
                        (CONST_WOBUS_C5 + temp *
                        (CONST_WOBUS_C6 + temp *
                        (CONST_WOBUS_C7 + temp *
                        (CONST_WOBUS_C8 + temp * (CONST_WOBUS_C9)))))))));
        return CONST_WOBUS_ES0 / pow(pol, 8);
}

float AircraftPerformance::pressure_to_density_2(float pressure, float temp, float dewpoint)
{
        if (temp < 0.01)
                temp = 0.01;
        if (dewpoint < 0.01)
                dewpoint = 0.01;
        if (dewpoint > temp)
                dewpoint = temp;
        float vaporpressure = temp_to_vapor_pressure(dewpoint);
        float airpressure = pressure - vaporpressure;
        return ((CONST_MR) * airpressure + (CONST_MR_WATERVAPOR) * vaporpressure) / temp;
}

float AircraftPerformance::density_to_pressure_2(float density, float temp, float dewpoint)
{
        if (temp < 0.01)
                temp = 0.01;
        if (dewpoint < 0.01)
                dewpoint = 0.01;
        if (dewpoint > temp)
                dewpoint = temp;
        float vaporpressure = temp_to_vapor_pressure(dewpoint);
        float airpressure = (density * temp - (CONST_MR_WATERVAPOR) * vaporpressure)  * (CONST_RM);
        return airpressure + vaporpressure;
}

float AircraftPerformance::relative_humidity(float temp, float dewpoint)
{
        if (temp < 0.01)
                temp = 0.01;
        if (dewpoint < 0.01)
                dewpoint = 0.01;
        if (dewpoint > temp)
                return 1.0;
        return temp_to_vapor_pressure(dewpoint) / temp_to_vapor_pressure(temp);
}

void AircraftPerformance::notebook_switch_page(guint page_num)
{
        unsigned int acft_index(m_acftperfwbacft->get_active_row_number());
        if (acft_index < sizeof(aircraft)/sizeof(aircraft[0])) {
                const struct aircraft& acft(aircraft[acft_index]);
                unsigned int i;
                for (i = 0; aircraft_types[i] && aircraft_types[i] != acft.acfttype; ++i);
                if (aircraft_types[i]) {
                        m_acftperfdistacfttype->set_active(i);
                        m_acftperfdistoperation->clear();
                        const char * const *p(acft.acfttype->dist_curves);
                        if (p) {
                                while (*p) {
                                        m_acftperfdistoperation->append(*p);
                                        ++p;
                                }
                                m_acftperfdistoperation->set_active(0);
                        }
                }
        }
}

void AircraftPerformance::update_weightbalance_aircraft(void)
{
        // find out what aircraft
        unsigned int acft_index(m_acftperfwbacft->get_active_row_number());
        if (acft_index >= sizeof(aircraft)/sizeof(aircraft[0])) {
                m_acftperfwbacfttext->set_text("");
                m_acftperfwbminfuelarm->set_text("");
                m_acftperfwbmaxfuelarm->set_text("");
                m_acftperfwbminfuelmass->set_text("");
                m_acftperfwbmaxfuelmass->set_text("");
                m_acftperfwbminfuelmoment->set_text("");
                m_acftperfwbmaxfuelmoment->set_text("");
                m_acftperfwbpilot->set_sensitive(false);
                m_acftperfwbpax->set_sensitive(false);
                m_acftperfwbrearpax1->set_sensitive(false);
                m_acftperfwbrearpax2->set_sensitive(false);
                m_acftperfwbbaggage->set_sensitive(false);
                m_acftperfwbminfuel->set_sensitive(false);
                m_acftperfwbmaxfuel->set_sensitive(false);
                m_acftperfwbdrawingarea->set_limits(0, 0);
                m_acftperfwbdrawingarea->set_minfuel(std::numeric_limits<float>::quiet_NaN(), std::numeric_limits<float>::quiet_NaN());
                m_acftperfwbdrawingarea->set_maxfuel(std::numeric_limits<float>::quiet_NaN(), std::numeric_limits<float>::quiet_NaN());
                return;
        }
        const struct aircraft& acft(aircraft[acft_index]);
        {
                std::ostringstream oss;
                oss << acft.acfttype->icao_designator << ' ' << acft.acfttype->name;
                m_acftperfwbacfttext->set_text(oss.str());
        }
        m_acftperfwbpilot->set_sensitive(!!(acft.acfttype->flags & HAS_PILOT));
        m_acftperfwbpax->set_sensitive(!!(acft.acfttype->flags & HAS_FRONTPAX));
        m_acftperfwbrearpax1->set_sensitive(!!(acft.acfttype->flags & HAS_REARPAX1));
        m_acftperfwbrearpax2->set_sensitive(!!(acft.acfttype->flags & HAS_REARPAX2));
        m_acftperfwbbaggage->set_sensitive(!!(acft.acfttype->flags & HAS_BAGGAGE));
        m_acftperfwbminfuel->set_sensitive(true);
        m_acftperfwbmaxfuel->set_sensitive(true);
        m_wb_maxfuel = acft.acfttype->max_fuel;
        m_wb_baggage = acft.acfttype->max_baggage;
        unsigned int frontmass_unit(m_acftperfwbfrontunit->get_active_row_number());
        m_acftperfwbpilot->set_value(from_si(mass_units[frontmass_unit], m_wb_pilot));
        m_acftperfwbpax->set_value(from_si(mass_units[frontmass_unit], m_wb_frontpax));
        unsigned int rearmass_unit(m_acftperfwbrearunit->get_active_row_number());
        m_acftperfwbrearpax1->set_value(from_si(mass_units[rearmass_unit], m_wb_rearpax1));
        m_acftperfwbrearpax2->set_value(from_si(mass_units[rearmass_unit], m_wb_rearpax2));
        unsigned int baggagemass_unit(m_acftperfwbbaggageunit->get_active_row_number());
        float maxbaggage(from_si(mass_units[baggagemass_unit], acft.acfttype->max_baggage));
        m_acftperfwbbaggage->set_range(0, maxbaggage);
        m_acftperfwbbaggage->set_value(from_si(mass_units[baggagemass_unit], m_wb_baggage));
        m_acftperfwbdrawingarea->set_limits(acft.acfttype->cgnormal, acft.acfttype->cgutility);
        unsigned int fuel_unit(m_acftperfwbfuelunit->get_active_row_number());
        float maxfuel(from_si(volume_units[fuel_unit], acft.acfttype->max_fuel));
        m_acftperfwbminfuel->set_range(0, maxfuel);
        m_acftperfwbmaxfuel->set_range(0, maxfuel);
        m_acftperfwbminfuel->set_value(from_si(volume_units[fuel_unit], m_wb_minfuel));
        m_acftperfwbmaxfuel->set_value(from_si(volume_units[fuel_unit], m_wb_maxfuel));
        update_weightbalance();
}

void AircraftPerformance::update_weightbalance_pilot(void)
{
        unsigned int frontmass_unit(m_acftperfwbfrontunit->get_active_row_number());
        m_wb_pilot = to_si(mass_units[frontmass_unit], m_acftperfwbpilot->get_value());
        update_weightbalance();
}

void AircraftPerformance::update_weightbalance_frontpax(void)
{
        unsigned int frontmass_unit(m_acftperfwbfrontunit->get_active_row_number());
        m_wb_frontpax = to_si(mass_units[frontmass_unit], m_acftperfwbpax->get_value());
        update_weightbalance();
}

void AircraftPerformance::update_weightbalance_rearpax1(void)
{
        unsigned int rearmass_unit(m_acftperfwbrearunit->get_active_row_number());
        m_wb_rearpax1 = to_si(mass_units[rearmass_unit], m_acftperfwbrearpax1->get_value());
        update_weightbalance();
}

void AircraftPerformance::update_weightbalance_rearpax2(void)
{
        unsigned int rearmass_unit(m_acftperfwbrearunit->get_active_row_number());
        m_wb_rearpax2 = to_si(mass_units[rearmass_unit], m_acftperfwbrearpax2->get_value());
        update_weightbalance();
}

void AircraftPerformance::update_weightbalance_baggage(void)
{
        unsigned int baggagemass_unit(m_acftperfwbbaggageunit->get_active_row_number());
        m_wb_baggage = to_si(mass_units[baggagemass_unit], m_acftperfwbbaggage->get_value());
        update_weightbalance();
}

void AircraftPerformance::update_weightbalance_minfuel(void)
{
        unsigned int fuel_unit(m_acftperfwbfuelunit->get_active_row_number());
        m_wb_minfuel = to_si(volume_units[fuel_unit], m_acftperfwbminfuel->get_value()) ;
        update_weightbalance();
}

void AircraftPerformance::update_weightbalance_maxfuel(void)
{
        unsigned int fuel_unit(m_acftperfwbfuelunit->get_active_row_number());
        m_wb_maxfuel = to_si(volume_units[fuel_unit], m_acftperfwbmaxfuel->get_value());
        update_weightbalance();
}

void AircraftPerformance::update_weightbalance_frontunit(void)
{
        unsigned int frontmass_unit(m_acftperfwbfrontunit->get_active_row_number());
        m_acftperfwbpilot->set_value(from_si(mass_units[frontmass_unit], m_wb_pilot));
        m_acftperfwbpax->set_value(from_si(mass_units[frontmass_unit], m_wb_frontpax));
        update_weightbalance();
}

void AircraftPerformance::update_weightbalance_rearunit(void)
{
        unsigned int rearmass_unit(m_acftperfwbrearunit->get_active_row_number());
        m_acftperfwbrearpax1->set_value(from_si(mass_units[rearmass_unit], m_wb_rearpax1));
        m_acftperfwbrearpax2->set_value(from_si(mass_units[rearmass_unit], m_wb_rearpax2));
        update_weightbalance();
}

void AircraftPerformance::update_weightbalance_fuelunit(void)
{
        unsigned int acft_index(m_acftperfwbacft->get_active_row_number());
        if (acft_index >= sizeof(aircraft)/sizeof(aircraft[0]))
                return;
        const struct aircraft& acft(aircraft[acft_index]);
        unsigned int fuel_unit(m_acftperfwbfuelunit->get_active_row_number());
        float maxfuel(from_si(volume_units[fuel_unit], acft.acfttype->max_fuel));
        m_acftperfwbminfuel->set_range(0, maxfuel);
        m_acftperfwbmaxfuel->set_range(0, maxfuel);
        m_acftperfwbminfuel->set_value(from_si(volume_units[fuel_unit], m_wb_minfuel));
        m_acftperfwbmaxfuel->set_value(from_si(volume_units[fuel_unit], m_wb_maxfuel));
        update_weightbalance();
}

void AircraftPerformance::update_weightbalance_baggageunit(void)
{
        unsigned int acft_index(m_acftperfwbacft->get_active_row_number());
        if (acft_index >= sizeof(aircraft)/sizeof(aircraft[0]))
                return;
        const struct aircraft& acft(aircraft[acft_index]);
        unsigned int baggagemass_unit(m_acftperfwbbaggageunit->get_active_row_number());
        float maxbaggage(from_si(mass_units[baggagemass_unit], acft.acfttype->max_baggage));
        m_acftperfwbbaggage->set_range(0, maxbaggage);
        m_acftperfwbbaggage->set_value(from_si(mass_units[baggagemass_unit], m_wb_baggage));
        update_weightbalance();
}

void AircraftPerformance::update_weightbalance(void)
{
        // set moment unit
        unsigned int armunit_index(m_acftperfwbarmunit->get_active_row_number());
        unsigned int massunit_index(m_acftperfwbmassunit->get_active_row_number());
        m_acftperfwbmomentunit->set_text(Glib::ustring(length_units[armunit_index].str) + " " + mass_units[massunit_index].str);
        // find out what aircraft
        unsigned int acft_index(m_acftperfwbacft->get_active_row_number());
        if (acft_index >= sizeof(aircraft)/sizeof(aircraft[0]))
                return;
        const struct aircraft& acft(aircraft[acft_index]);
        float frontmass(0), rearmass(0), baggagemass(0);
        if (acft.acfttype->flags & HAS_PILOT)
                frontmass += m_wb_pilot;
        if (acft.acfttype->flags & HAS_FRONTPAX)
                frontmass += m_wb_frontpax;
        if (acft.acfttype->flags & HAS_REARPAX1)
                rearmass += m_wb_rearpax1;
        if (acft.acfttype->flags & HAS_REARPAX2)
                rearmass += m_wb_rearpax2;
        if (acft.acfttype->flags & HAS_BAGGAGE)
                baggagemass += m_wb_baggage;
        float minfuelmass(m_wb_minfuel * fuel_specweight);
        float maxfuelmass(m_wb_maxfuel * fuel_specweight);
        float minmass(frontmass + rearmass + baggagemass + acft.empty_mass);
        float maxmass(minmass);
        float minmoment(frontmass * acft.acfttype->arm_frontpax +
                        rearmass * acft.acfttype->arm_rearpax +
                        baggagemass * acft.acfttype->arm_baggage +
                        acft.empty_mass * acft.empty_arm);
        float maxmoment(minmoment);
        minmass += minfuelmass;
        maxmass += maxfuelmass;
        minmoment += minfuelmass * acft.acfttype->arm_fuel;
        maxmoment += maxfuelmass * acft.acfttype->arm_fuel;
        float minarm(minmoment / minmass);
        float maxarm(maxmoment / maxmass);
        unsigned int length_unit(m_acftperfwbarmunit->get_active_row_number());
        unsigned int mass_unit(m_acftperfwbmassunit->get_active_row_number());
        float minarmu(from_si(length_units[length_unit], minarm));
        float maxarmu(from_si(length_units[length_unit], maxarm));
        float minmassu(from_si(mass_units[mass_unit], minmass));
        float maxmassu(from_si(mass_units[mass_unit], maxmass));
        {
                std::ostringstream oss;
                oss << std::setprecision(2) << std::fixed << minarmu;
                m_acftperfwbminfuelarm->set_text(oss.str());
        }
        {
                std::ostringstream oss;
                oss << std::setprecision(2) << std::fixed << maxarmu;
                m_acftperfwbmaxfuelarm->set_text(oss.str());
        }
        {
                std::ostringstream oss;
                oss << std::setprecision(2) << std::fixed << minmassu;
                m_acftperfwbminfuelmass->set_text(oss.str());
        }
        {
                std::ostringstream oss;
                oss << std::setprecision(2) << std::fixed << maxmassu;
                m_acftperfwbmaxfuelmass->set_text(oss.str());
        }
        {
                std::ostringstream oss;
                oss << std::setprecision(2) << std::fixed << minarmu * minmassu;
                m_acftperfwbminfuelmoment->set_text(oss.str());
        }
        {
                std::ostringstream oss;
                oss << std::setprecision(2) << std::fixed << maxarmu * maxmassu;
                m_acftperfwbmaxfuelmoment->set_text(oss.str());
        }
        m_acftperfwbdrawingarea->set_minfuel(minmass, minarm);
        m_acftperfwbdrawingarea->set_maxfuel(maxmass, maxarm);
	{
		m_dist_mass = maxmass;
		unsigned int mass_unit(m_acftperfdistmassunit->get_active_row_number());
		m_acftperfdistmass->set_value(from_si(mass_units[mass_unit], m_dist_mass));
	}
}

void AircraftPerformance::distance_error(void)
{
        m_acftperfdistgndroll->set_text("!");
        m_acftperfdistobstacle->set_text("!");
        m_acftperfdistdebug->set_text("!");
}

void AircraftPerformance::update_distance_aircraft(void)
{
        unsigned int acftindex(m_acftperfdistacfttype->get_active_row_number());
        if (acftindex < sizeof(aircraft_types)/sizeof(aircraft_types[0]) && aircraft_types[acftindex]) {
                m_acftperfdistoperation->clear();
                const char * const *p(aircraft_types[acftindex]->dist_curves);
                if (p) {
                        while (*p) {
                                m_acftperfdistoperation->append(*p);
                                ++p;
                        }
                        m_acftperfdistoperation->set_active(0);
                }
        }
        unsigned int mass_unit(m_acftperfdistmassunit->get_active_row_number());
        m_acftperfdistmass->set_value(from_si(mass_units[mass_unit], m_dist_mass));
        unsigned int alt_unit(m_acftperfdistaltunit->get_active_row_number());
        m_acftperfdistaltitude->set_value(from_si(altitude_units[alt_unit], m_dist_altitude));
        unsigned int qnh_unit(m_acftperfdistqnhunit->get_active_row_number());
        m_acftperfdistqnh->set_value(from_si(qnh_units[qnh_unit], m_dist_qnh));
        unsigned int temp_unit(m_acftperfdisttempunit->get_active_row_number());
        m_acftperfdistoat->set_value(from_si(temp_units[temp_unit], m_dist_oat));
        m_acftperfdistdewpoint->set_value(from_si(temp_units[temp_unit], m_dist_dewpoint));
        unsigned int windspeed_unit(m_acftperfdistwindunit->get_active_row_number());
        m_acftperfdistwindspeed->set_value(from_si(windspeed_units[windspeed_unit], fabsf(m_dist_wind)));
        m_acftperfdistwinddir->set_active(m_dist_wind < 0);
        update_distance();
}

void AircraftPerformance::update_distance_mass(void)
{
        unsigned int mass_unit(m_acftperfdistmassunit->get_active_row_number());
        m_dist_mass = to_si(mass_units[mass_unit], m_acftperfdistmass->get_value());
        update_distance();
}

void AircraftPerformance::update_distance_altitude(void)
{
        unsigned int alt_unit(m_acftperfdistaltunit->get_active_row_number());
        m_dist_altitude = to_si(altitude_units[alt_unit], m_acftperfdistaltitude->get_value());
        update_distance();
}

void AircraftPerformance::update_distance_qnh(void)
{
        unsigned int qnh_unit(m_acftperfdistqnhunit->get_active_row_number());
        m_dist_qnh = to_si(qnh_units[qnh_unit], m_acftperfdistqnh->get_value());
        update_distance();
}

void AircraftPerformance::update_distance_oat(void)
{
        unsigned int temp_unit(m_acftperfdisttempunit->get_active_row_number());
        m_dist_oat = to_si(temp_units[temp_unit], m_acftperfdistoat->get_value());
        update_distance();
}

void AircraftPerformance::update_distance_dewpoint(void)
{
        unsigned int temp_unit(m_acftperfdisttempunit->get_active_row_number());
        m_dist_dewpoint = to_si(temp_units[temp_unit], m_acftperfdistdewpoint->get_value());
        update_distance();
}

void AircraftPerformance::update_distance_winddir(void)
{
        m_dist_wind = fabsf(m_dist_wind);
        if (m_acftperfdistwinddir->get_active_row_number())
                m_dist_wind = -m_dist_wind;
        update_distance();
}

void AircraftPerformance::update_distance_windspeed(void)
{
        unsigned int windspeed_unit(m_acftperfdistwindunit->get_active_row_number());
        m_dist_wind = to_si(windspeed_units[windspeed_unit], m_acftperfdistwindspeed->get_value());
        if (m_acftperfdistwinddir->get_active_row_number())
                m_dist_wind = -m_dist_wind;
        update_distance();
}

void AircraftPerformance::update_distance_massunit(void)
{
        unsigned int mass_unit(m_acftperfdistmassunit->get_active_row_number());
        m_acftperfdistmass->set_value(from_si(mass_units[mass_unit], m_dist_mass));
        update_distance();
}

void AircraftPerformance::update_distance_altunit(void)
{
        unsigned int alt_unit(m_acftperfdistaltunit->get_active_row_number());
        m_acftperfdistaltitude->set_value(from_si(altitude_units[alt_unit], m_dist_altitude));
        update_distance();
}

void AircraftPerformance::update_distance_qnhunit(void)
{
        unsigned int qnh_unit(m_acftperfdistqnhunit->get_active_row_number());
        m_acftperfdistqnh->set_value(from_si(qnh_units[qnh_unit], m_dist_qnh));
        update_distance();
}

void AircraftPerformance::update_distance_tempunit(void)
{
        unsigned int temp_unit(m_acftperfdisttempunit->get_active_row_number());
        m_acftperfdistoat->set_value(from_si(temp_units[temp_unit], m_dist_oat));
        m_acftperfdistdewpoint->set_value(from_si(temp_units[temp_unit], m_dist_dewpoint));
        update_distance();
}

void AircraftPerformance::update_distance_windunit(void)
{
        unsigned int windspeed_unit(m_acftperfdistwindunit->get_active_row_number());
        m_acftperfdistwindspeed->set_value(from_si(windspeed_units[windspeed_unit], fabsf(m_dist_wind)));
        m_acftperfdistwinddir->set_active(m_dist_wind < 0);
        update_distance();
}

void AircraftPerformance::update_distance(void)
{
        unsigned int dist_unit(m_acftperfdistdistunit->get_active_row_number());
        m_acftperfdistdistunit2->set_text(dist_units[dist_unit].str);
        // negative wind: tailwind
        {
                std::ostringstream oss;
                oss << std::setprecision(1) << std::fixed << 100.0 * relative_humidity(m_dist_oat, m_dist_dewpoint) << " %";
                m_acftperfdisthumidity->set_text(oss.str());
        }
        try {
                IcaoAtmosphere<float> atmo(m_dist_qnh, m_dist_oat);
                float press, temp1;
                atmo.altitude_to_pressure(&press, &temp1, m_dist_altitude);
                float density = pressure_to_density_2(press, m_dist_oat, m_dist_dewpoint);
                atmo.set();
                float densityalt;
                for (unsigned int i = 0; i < 4; i++) {
                        float press1 = atmo.density_to_pressure(density, temp1);
                        atmo.pressure_to_altitude(&densityalt, &temp1, press1);
                }
                //pressalt = (1 - pow(press * (1.0/1013.25), 0.190284)) * 44307.69396;
                unsigned int densalt_unit(m_acftperfdistdensityaltunit->get_active_row_number());
                if (densalt_unit < 2) {
                        m_acftperfdistdensaltlabel->set_text(_("Density Alt:"));
                        std::ostringstream oss;
                        oss << std::setprecision(2) << std::fixed << from_si(densityalt_units[densalt_unit], densityalt);
                        m_acftperfdistdensityalt->set_text(oss.str());
                } else if (densalt_unit < 5) {
                        m_acftperfdistdensaltlabel->set_text(_("Pressure:"));
                        std::ostringstream oss;
                        oss << std::setprecision(2) << std::fixed << from_si(densityalt_units[densalt_unit], press);
                        m_acftperfdistdensityalt->set_text(oss.str());
                } else {
                        m_acftperfdistdensaltlabel->set_text(_("Density:"));
                        std::ostringstream oss;
                        oss << std::setprecision(2) << std::fixed << from_si(densityalt_units[densalt_unit], density * 0.1);
                        m_acftperfdistdensityalt->set_text(oss.str());
                }
                unsigned int acftindex(m_acftperfdistacfttype->get_active_row_number());
                if (acftindex >= sizeof(aircraft_types)/sizeof(aircraft_types[0]) ||
                    !aircraft_types[acftindex] || !aircraft_types[acftindex]->compute_dist) {
                        distance_error();
                        return;
                }
                aircraft_type::compute_dist_t func(aircraft_types[acftindex]->compute_dist);
                try {
                        (this->*func)(m_acftperfdistoperation->get_active_row_number(), density, densityalt, m_dist_mass, m_dist_wind);
                } catch (...) {
                        distance_error();
                        return;
                }
        } catch (...) {
                m_acftperfdistdensityalt->set_text("!");
                distance_error();
                return;
        }
}

void AircraftPerformance::set_distances(float gndroll, float obstacle)
{
        unsigned int dist_unit(m_acftperfdistdistunit->get_active_row_number());
        {
                std::ostringstream oss;
                oss << std::setprecision(2) << std::fixed << from_si(dist_units[dist_unit], gndroll);
                m_acftperfdistgndroll->set_text(oss.str());
        }
        {
                std::ostringstream oss;
                oss << std::setprecision(2) << std::fixed << from_si(dist_units[dist_unit], obstacle);
                m_acftperfdistobstacle->set_text(oss.str());
        }
}

void AircraftPerformance::set_distance_debug(const Glib::ustring & str)
{
        m_acftperfdistdebug->set_text(str);
}

void AircraftPerformance::dist_archerII_compute(unsigned int curveidx, float density, float densityalt, float mass, float wind)
{
        std::ostringstream msg;
        float gndroll_dist1, gndroll_mass_factor, gndroll_dist2, gndroll_wind_factor, gndroll_dist;
        float obst_dist1, obst_mass_factor, obst_dist2, obst_wind_factor, obst_dist;

        if (mass < 929.86436) {
                mass = 929.86436;
                msg << "Mass < 2050lb ";
        }
        if (mass > 1156.6605) {
                mass = 1156.6605;
                msg << "Mass > 2550lb ";
        }
        if (wind < -2.5722222) {
                wind = -2.5722222;
                msg << "Tailwind > 5kts ";
        }
        if (wind > 7.7166667) {
                wind = 7.7166667;
                msg << "Headwind > 15kts ";
        }
        switch (curveidx) {
                case 0:
                        gndroll_dist1 = ARCHERII_TAKEOFF0_GNDROLL_DENSITY_TO_DIST(density);
                        gndroll_mass_factor = ARCHERII_TAKEOFF0_GNDROLL_MASS_FACTOR(gndroll_dist1,mass);
                        gndroll_dist2 = gndroll_dist1 * gndroll_mass_factor;
                        if (wind < 0)
                                gndroll_wind_factor = ARCHERII_TAKEOFF0_GNDROLL_TAILWIND_FACTOR(gndroll_dist2,-wind);
                        else
                                gndroll_wind_factor = ARCHERII_TAKEOFF0_GNDROLL_HEADWIND_FACTOR(gndroll_dist2,wind);
                        gndroll_dist = gndroll_dist2 * gndroll_wind_factor;
                        obst_dist1 = ARCHERII_TAKEOFF0_OBST_DENSITY_TO_DIST(density);
                        obst_mass_factor = ARCHERII_TAKEOFF0_OBST_MASS_FACTOR(obst_dist1,mass);
                        obst_dist2 = obst_dist1 * obst_mass_factor;
                        if (wind < 0)
                                obst_wind_factor = ARCHERII_TAKEOFF0_OBST_TAILWIND_FACTOR(obst_dist2,-wind);
                        else
                                obst_wind_factor = ARCHERII_TAKEOFF0_OBST_HEADWIND_FACTOR(obst_dist2,wind);
                        obst_dist = obst_dist2 * obst_wind_factor;
                        break;

                case 1:
                        gndroll_dist1 = ARCHERII_TAKEOFF25_GNDROLL_DENSITY_TO_DIST(density);
                        gndroll_mass_factor = ARCHERII_TAKEOFF25_GNDROLL_MASS_FACTOR(gndroll_dist1,mass);
                        gndroll_dist2 = gndroll_dist1 * gndroll_mass_factor;
                        if (wind < 0)
                                gndroll_wind_factor = ARCHERII_TAKEOFF25_GNDROLL_TAILWIND_FACTOR(gndroll_dist2,-wind);
                        else
                                gndroll_wind_factor = ARCHERII_TAKEOFF25_GNDROLL_HEADWIND_FACTOR(gndroll_dist2,wind);
                        gndroll_dist = gndroll_dist2 * gndroll_wind_factor;
                        obst_dist1 = ARCHERII_TAKEOFF25_OBST_DENSITY_TO_DIST(density);
                        obst_mass_factor = ARCHERII_TAKEOFF25_OBST_MASS_FACTOR(obst_dist1,mass);
                        obst_dist2 = obst_dist1 * obst_mass_factor;
                        if (wind < 0)
                                obst_wind_factor = ARCHERII_TAKEOFF25_OBST_TAILWIND_FACTOR(obst_dist2,-wind);
                        else
                                obst_wind_factor = ARCHERII_TAKEOFF25_OBST_HEADWIND_FACTOR(obst_dist2,wind);
                        obst_dist = obst_dist2 * obst_wind_factor;
                        break;

                case 2:
                        gndroll_dist1 = ARCHERII_LANDING40_GNDROLL_DENSITY_TO_DIST(density);
                        gndroll_mass_factor = ARCHERII_LANDING40_GNDROLL_MASS_FACTOR(gndroll_dist1,mass);
                        gndroll_dist2 = gndroll_dist1 * gndroll_mass_factor;
                        if (wind < 0)
                                gndroll_wind_factor = ARCHERII_LANDING40_GNDROLL_TAILWIND_FACTOR(gndroll_dist2,-wind);
                        else
                                gndroll_wind_factor = ARCHERII_LANDING40_GNDROLL_HEADWIND_FACTOR(gndroll_dist2,wind);
                        gndroll_dist = gndroll_dist2 * gndroll_wind_factor;
                        obst_dist1 = ARCHERII_LANDING40_OBST_DENSITY_TO_DIST(density);
                        obst_mass_factor = ARCHERII_LANDING40_OBST_MASS_FACTOR(obst_dist1,mass);
                        obst_dist2 = obst_dist1 * obst_mass_factor;
                        if (wind < 0)
                                obst_wind_factor = ARCHERII_LANDING40_OBST_TAILWIND_FACTOR(obst_dist2,-wind);
                        else
                                obst_wind_factor = ARCHERII_LANDING40_OBST_HEADWIND_FACTOR(obst_dist2,wind);
                        obst_dist = obst_dist2 * obst_wind_factor;
                        break;

                default:
                        throw std::runtime_error("invalid curve");
        }
        msg << std::setprecision(1) << std::fixed
            << "G: " << gndroll_dist1 << "/" << gndroll_mass_factor << "/" << gndroll_wind_factor
            << " O: " << obst_dist1 << "/" << obst_mass_factor << "/" << obst_wind_factor;
        set_distances(gndroll_dist, obst_dist);
        set_distance_debug(msg.str());
}

void AircraftPerformance::dist_arrow180_compute(unsigned int curveidx, float density, float densityalt, float mass, float wind)
{
        std::ostringstream msg;
        float gndroll_dist1, gndroll_mass_factor, gndroll_dist2, gndroll_wind_factor, gndroll_dist;
        float obst_dist1, obst_mass_factor, obst_dist2, obst_wind_factor, obst_dist;

        if (mass < 544.31084) {
                mass = 544.31084;
                msg << "Mass < 1200lb ";
        }
        if (mass > 1179.3403) {
                mass = 1179.3403;
                msg << "Mass > 2600lb ";
        }
        if (curveidx > 1) {
                if (wind < -0.51444444) {
                        wind = -0.51444444;
                        msg << "Tailwind > 1kts ";
                }
                if (wind > 0.51444444) {
                        wind = 0.51444444;
                        msg << "Headwind > 1kts ";
                }
        } else {
                if (wind < -2.5722222) {
                        wind = -2.5722222;
                        msg << "Tailwind > 5kts ";
                }
                if (wind > 7.7166667) {
                        wind = 7.7166667;
                        msg << "Headwind > 15kts ";
                }
        }
        switch (curveidx) {
                case 0:
                        gndroll_dist1 = ARROW180_TAKEOFF0_GND(densityalt);
                        gndroll_mass_factor = mass * mass * (1.0 / 1179.3403 / 1179.3403);
                        gndroll_dist2 = gndroll_dist1 * gndroll_mass_factor;
                        gndroll_wind_factor = (29.0576 - wind) * (1.0 / 29.0576);
                        gndroll_wind_factor *= gndroll_wind_factor;
                        gndroll_dist = gndroll_dist2 * gndroll_wind_factor;
                        obst_dist1 = ARROW180_TAKEOFF0_OBST(densityalt);
                        obst_mass_factor = gndroll_mass_factor;
                        obst_dist2 = obst_dist1 * obst_mass_factor;
                        obst_wind_factor = gndroll_wind_factor;
                        obst_dist = obst_dist2 * obst_wind_factor;
                        break;

                case 1:
                        gndroll_dist1 = ARROW180_TAKEOFF25_GND(densityalt);
                        gndroll_mass_factor = mass * mass * (1.0 / 1179.3403 / 1179.3403);
                        gndroll_dist2 = gndroll_dist1 * gndroll_mass_factor;
                        gndroll_wind_factor = (29.0576 - wind) * (1.0 / 29.0576);
                        gndroll_wind_factor *= gndroll_wind_factor;
                        gndroll_dist = gndroll_dist2 * gndroll_wind_factor;
                        obst_dist1 = ARROW180_TAKEOFF25_OBST(densityalt);
                        obst_mass_factor = gndroll_mass_factor;
                        obst_dist2 = obst_dist1 * obst_mass_factor;
                        obst_wind_factor = gndroll_wind_factor;
                        obst_dist = obst_dist2 * obst_wind_factor;
                        break;

                case 2:
                        gndroll_dist1 = ARROW180_LDG40_GND(densityalt);
                        gndroll_mass_factor = mass * mass * (1.0 / 1179.3403 / 1179.3403);
                        gndroll_dist2 = gndroll_dist1 * gndroll_mass_factor;
                        if (wind < 0)
                                gndroll_wind_factor = 1;
                        else
                                gndroll_wind_factor = 1;
                        gndroll_dist = gndroll_dist2 * gndroll_wind_factor;
                        obst_dist1 = ARROW180_LDG40_OBST(densityalt);
                        obst_mass_factor = mass * mass * (1.0 / 1179.3403 / 1179.3403);
                        obst_dist2 = obst_dist1 * obst_mass_factor;
                        if (wind < 0)
                                obst_wind_factor = 1;
                        else
                                obst_wind_factor = 1;
                        obst_dist = obst_dist2 * obst_wind_factor;
                        break;

                default:
                        throw std::runtime_error("invalid curve");
        }
        msg << std::setprecision(1) << std::fixed
            << "G: " << gndroll_dist1 << "/" << gndroll_mass_factor << "/" << gndroll_wind_factor
            << " O: " << obst_dist1 << "/" << obst_mass_factor << "/" << obst_wind_factor;
        set_distances(gndroll_dist, obst_dist);
        set_distance_debug(msg.str());
}

void AircraftPerformance::dist_arrow200_compute(unsigned int curveidx, float density, float densityalt, float mass, float wind)
{
        std::ostringstream msg;
        float gndroll_dist1, gndroll_mass_factor, gndroll_dist2, gndroll_wind_factor, gndroll_dist;
        float obst_dist1, obst_mass_factor, obst_dist2, obst_wind_factor, obst_dist;

        if (mass < 544.31084) {
                mass = 544.31084;
                msg << "Mass < 1200lb ";
        }
        if (mass > 1179.3403) {
                mass = 1179.3403;
                msg << "Mass > 2600lb ";
        }
        if (curveidx > 1) {
                if (wind < -0.51444444) {
                        wind = -0.51444444;
                        msg << "Tailwind > 1kts ";
                }
                if (wind > 0.51444444) {
                        wind = 0.51444444;
                        msg << "Headwind > 1kts ";
                }
        } else {
                if (wind < -2.5722222) {
                        wind = -2.5722222;
                        msg << "Tailwind > 5kts ";
                }
                if (wind > 7.7166667) {
                        wind = 7.7166667;
                        msg << "Headwind > 15kts ";
                }
        }
        switch (curveidx) {
                case 0:
                        gndroll_dist1 = ARROW200_TAKEOFF0_GND(densityalt);
                        gndroll_mass_factor = mass * mass * (1.0 / 1179.3403 / 1179.3403);
                        gndroll_dist2 = gndroll_dist1 * gndroll_mass_factor;
                        gndroll_wind_factor = (29.0576 - wind) * (1.0 / 29.0576);
                        gndroll_wind_factor *= gndroll_wind_factor;
                        gndroll_dist = gndroll_dist2 * gndroll_wind_factor;
                        obst_dist1 = ARROW200_TAKEOFF0_OBST(densityalt);
                        obst_mass_factor = gndroll_mass_factor;
                        obst_dist2 = obst_dist1 * obst_mass_factor;
                        obst_wind_factor = gndroll_wind_factor;
                        obst_dist = obst_dist2 * obst_wind_factor;
                        break;

                case 1:
                        gndroll_dist1 = ARROW200_TAKEOFF25_GND(densityalt);
                        gndroll_mass_factor = mass * mass * (1.0 / 1179.3403 / 1179.3403);
                        gndroll_dist2 = gndroll_dist1 * gndroll_mass_factor;
                        gndroll_wind_factor = (29.0576 - wind) * (1.0 / 29.0576);
                        gndroll_wind_factor *= gndroll_wind_factor;
                        gndroll_dist = gndroll_dist2 * gndroll_wind_factor;
                        obst_dist1 = ARROW200_TAKEOFF25_OBST(densityalt);
                        obst_mass_factor = gndroll_mass_factor;
                        obst_dist2 = obst_dist1 * obst_mass_factor;
                        obst_wind_factor = gndroll_wind_factor;
                        obst_dist = obst_dist2 * obst_wind_factor;
                        break;

                case 2:
                        gndroll_dist1 = ARROW200_LDG40_GND(densityalt);
                        gndroll_mass_factor = mass * mass * (1.0 / 1179.3403 / 1179.3403);
                        gndroll_dist2 = gndroll_dist1 * gndroll_mass_factor;
                        if (wind < 0)
                                gndroll_wind_factor = 1;
                        else
                                gndroll_wind_factor = 1;
                        gndroll_dist = gndroll_dist2 * gndroll_wind_factor;
                        obst_dist1 = ARROW200_LDG40_OBST(densityalt);
                        obst_mass_factor = mass * mass * (1.0 / 1179.3403 / 1179.3403);
                        obst_dist2 = obst_dist1 * obst_mass_factor;
                        if (wind < 0)
                                obst_wind_factor = 1;
                        else
                                obst_wind_factor = 1;
                        obst_dist = obst_dist2 * obst_wind_factor;
                        break;

                default:
                        throw std::runtime_error("invalid curve");
        }
        msg << std::setprecision(1) << std::fixed
            << "G: " << gndroll_dist1 << "/" << gndroll_mass_factor << "/" << gndroll_wind_factor
            << " O: " << obst_dist1 << "/" << obst_mass_factor << "/" << obst_wind_factor;
        set_distances(gndroll_dist, obst_dist);
        set_distance_debug(msg.str());
}

void AircraftPerformance::dist_grob115_compute(unsigned int curveidx, float density, float densityalt, float mass, float wind)
{
        std::ostringstream msg;
        float dist1, mass_factor, dist2, wind_factor, dist, obst_factor, obst_dist;

        if (mass < 750) {
                mass = 750;
                msg << "Mass < 750kg ";
        }
        if (mass > 920) {
                mass = 920;
                msg << "Mass > 920kg ";
        }
        if (wind < -5.5555556) {
                wind = -5.5555556;
                msg << "Tailwind > 20km/h ";
        }
        if (wind > 11.111111) {
                wind = 11.111111;
                msg << "Headwind > 40km/h ";
        }
        switch (curveidx) {
                case 0:
                        dist1 = GROB115B_TAKEOFF_DENSITY_TO_DIST(density);
                        mass_factor = GROB115B_TAKEOFF_MASS_FACTOR(dist1,mass);
                        dist2 = dist1 * mass_factor;
                        if (wind < 0)
                                wind_factor = GROB115B_TAKEOFF_TAILWIND_FACTOR(dist2,-wind);
                        else
                                wind_factor = GROB115B_TAKEOFF_HEADWIND_FACTOR(dist2,wind);
                        dist = dist2 * wind_factor;
                        obst_factor = GROB115B_TAKEOFF_OBSTACLE_FACTOR(dist,15);
                        obst_dist = dist * obst_factor;
                        break;

                case 1:
                        dist1 = GROB115B_LANDING_DENSITY_TO_DIST(density);
                        mass_factor = GROB115B_LANDING_MASS_FACTOR(dist1,mass);
                        dist2 = dist1 * mass_factor;
                        if (wind < 0)
                                wind_factor = GROB115B_LANDING_TAILWIND_FACTOR(dist2,-wind);
                        else
                                wind_factor = GROB115B_LANDING_HEADWIND_FACTOR(dist2,wind);
                        dist = dist2 * wind_factor;
                        obst_factor = GROB115B_LANDING_OBSTACLE_FACTOR(dist,15);
                        obst_dist = dist * obst_factor;
                        break;

                default:
                        throw std::runtime_error("invalid curve");
        }
        msg << std::setprecision(1) << std::fixed
            << "G: " << dist1 << "/" << mass_factor << "/" << wind_factor << "/" << obst_factor;
        set_distances(dist, obst_dist);
        set_distance_debug(msg.str());
}

void AircraftPerformance::dist_slingsbyT67M_compute(unsigned int curveidx, float density, float densityalt, float mass, float wind)
{
        std::ostringstream msg;
        unsigned int curveidx1(curveidx / 3);
        float gndroll_dist1, gndroll_wind_factor, gndroll_dist;
        float obst_dist1, obst_wind_factor, obst_dist;

        curveidx -= curveidx1 * 3;
        if (mass < 600) {
                mass = 600;
                msg << "Mass < 600kg ";
        }
        if (mass > 907) {
                mass = 907;
                msg << "Mass > 907kg ";
        }
        if (wind < -2.5722222) {
                wind = -2.5722222;
                msg << "Tailwind > 5kts ";
        }
        if (wind > 7.7166667) {
                wind = 7.7166667;
                msg << "Headwind > 15kts ";
        }
        if (wind < 0)
                obst_wind_factor = gndroll_wind_factor = pow(1.1, wind * -(1.0 / (1.854/3.6*2)));
        else
                obst_wind_factor = gndroll_wind_factor = pow(1.0/1.1, wind * (1.0 / (1.854/3.6*10)));
        switch (curveidx1) {
                case 0:
                        gndroll_dist1 = gndroll_dist = SLINGSBY_T67M_TKOFF_GND(densityalt);
                        obst_dist1 = obst_dist = SLINGSBY_T67M_TKOFF_OBST(densityalt);
                        switch (curveidx) {
                                case 1:
                                        gndroll_dist *= 1.10;
                                        obst_dist *= 1.10;
                                        break;

                                case 2:
                                        gndroll_dist *= 1.15;
                                        obst_dist *= 1.15;
                                        break;
                        }
                        break;

                case 1:
                case 2:
                        gndroll_dist1 = gndroll_dist = SLINGSBY_T67M_LDG_GND(densityalt);
                        obst_dist1 = obst_dist = SLINGSBY_T67M_LDG_OBST(densityalt);
                        switch (curveidx) {
                                case 1:
                                        gndroll_dist *= 1.10;
                                        obst_dist *= 1.10;
                                        break;

                                case 2:
                                        gndroll_dist *= 1.30;
                                        obst_dist *= 1.30;
                                        break;
                        }
                        if (curveidx1 == 2) {
                                gndroll_dist *= 1.55;
                                obst_dist *= 1.30;
                        }
                        break;

                default:
                        throw std::runtime_error("invalid curve");
        }
        msg << std::setprecision(1) << std::fixed
            << "G: " << gndroll_dist1 << "/" << gndroll_wind_factor
            << " O: " << obst_dist1 << "/" << obst_wind_factor;
        set_distances(gndroll_dist, obst_dist);
        set_distance_debug(msg.str());
}

int main(int argc, char *argv[])
{
        try {
		Glib::init();
                Glib::OptionGroup optgroup("aircraftperformance", "Aircraft Performance Options", "Options controlling the Aircraft Performance Calculator");
                //Glib::OptionEntry optmaindir;
                //optmaindir.set_short_name('m');
                //optmaindir.set_long_name("maindir");
                //optmaindir.set_description("Directory containing the main database");
                //optmaindir.set_arg_description("DIR");
                //optgroup.add_entry_filename(optmaindir, dir_main);
                Glib::OptionContext optctx("[--maindir=<dir>]");
                optctx.set_help_enabled(true);
                optctx.set_ignore_unknown_options(false);
                optctx.set_main_group(optgroup);
                Gtk::Main m(argc, argv, optctx);
#ifdef HAVE_OSSO
                osso_context_t* osso_context = osso_initialize("aircraftperformance", VERSION, TRUE /* deprecated parameter */, 0 /* Use default Glib main loop context */);
                if (!osso_context) {
                        std::cerr << "osso_initialize() failed." << std::endl;
                        return OSSO_ERROR;
                }
#endif
#ifdef HAVE_HILDON
                Hildon::init();
#endif
                Glib::set_application_name("Aircraft Performance Calculator");
                Glib::thread_init();
                AircraftPerformance acftperf;

                Gtk::Main::run(*acftperf.get_mainwindow());
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
