/*
 * optimizelabelplacement.cc:  label placement optimization utility
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation; either version 2 of the License, or (at your
 * option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General
 * Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program; if not, write to the Free Software Foundation, Inc.,
 * 59 Temple Place, Suite 330, Boston, MA 02111-1307 USA
 *
 */

#include "sysdeps.h"

#include <unistd.h>
#include <getopt.h>
#include <iostream>
#include <sstream>
#include <sqlite3x.hpp>
#include <sqlite3.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <errno.h>
#include <stdexcept>
#include <stdarg.h>
#include <set>

#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <time.h>
#include <math.h>
#include <stdarg.h>
#include <dirent.h>
#include <ctype.h>
#include <limits.h>

#include "geom.h"
#include "dbobj.h"

/* ---------------------------------------------------------------------- */

class LabelPlacementOpt {
        public:
                LabelPlacementOpt(const Glib::ustring& output_dir);
                ~LabelPlacementOpt();
                void optimize(unsigned int arpt_nav_wpt_passes = 3, unsigned int bdry_passes = 3,
                              bool arptmutable = false, bool navmutable = false, bool wptmutable = false, bool bdrymutable = false);

        protected:
                AirportsDb m_airportdb;
                NavaidsDb m_navaiddb;
                WaypointsDb m_waypointdb;
                AirspacesDb m_airspacedb;
                AirwaysDb m_airwaysdb;
                LabelsDb m_labelsdb;

                static Point label_offset(const Point& p, LabelsDb::Label::label_placement_t lp = LabelsDb::Label::label_center);
                static double label_metric(const Point& p1, const Point& p2);

                LabelsDb::Label find_mutable_label(const Point& pt);
                void save_label(LabelsDb::Label& e);
                void copy_arpt_nav_wpt_to_labelsdb(bool arptmutable = false, bool navmutable = false, bool wptmutable = false);
                void update_arpt_nav_wpt(void);
                double compute_metric(const LabelsDb::Label& e, const Point& pt, LabelsDb::Label::label_placement_t lp);
                double compute_metric(const LabelsDb::Label& e) { return compute_metric(e, e.get_coord(), e.get_label_placement()); }
                void compute_metrics(void);
                void optimize_pass(void);
                void optimize_arpt_nav_wpt(unsigned int passes = 3, bool arptmutable = false, bool navmutable = false, bool wptmutable = false);
                void copy_bdry_to_labelsdb(bool bdrymutable = false);
                void update_bdry(void);
                void optimize_bdry_pass(void);
                void optimize_bdry(unsigned int passes = 3, bool bdrymutable = false);
};


/* ---------------------------------------------------------------------- */

LabelPlacementOpt::LabelPlacementOpt(const Glib::ustring& output_dir)
{
        try {
                m_airportdb.open(output_dir);
        } catch (const std::exception& e) {
                std::cerr << "Error opening airports database: " << e.what() << std::endl;
        }
        try {
                m_navaiddb.open(output_dir);
        } catch (const std::exception& e) {
                std::cerr << "Error opening navaids database: " << e.what() << std::endl;
        }
        try {
                m_waypointdb.open(output_dir);
        } catch (const std::exception& e) {
                std::cerr << "Error opening waypoints database: " << e.what() << std::endl;
        }
        try {
                m_airspacedb.open(output_dir);
        } catch (const std::exception& e) {
                std::cerr << "Error opening airspaces database: " << e.what() << std::endl;
        }
        try {
                m_airwaysdb.open(output_dir);
        } catch (const std::exception& e) {
                std::cerr << "Error opening airways database: " << e.what() << std::endl;
        }
        try {
                m_labelsdb.open(output_dir);
        } catch (const std::exception& e) {
                std::cerr << "Error opening labels database: " << e.what() << std::endl;
        }
        m_airportdb.sync_off();
        m_navaiddb.sync_off();
        m_waypointdb.sync_off();
        m_airspacedb.sync_off();
        m_airwaysdb.sync_off();
        m_labelsdb.sync_off();
}

LabelPlacementOpt::~LabelPlacementOpt()
{
        m_airportdb.close();
        m_navaiddb.close();
        m_waypointdb.close();
        m_airspacedb.close();
        m_airwaysdb.close();
        m_labelsdb.close();
}

Point LabelPlacementOpt::label_offset(const Point & p, LabelsDb::Label::label_placement_t lp)
{
        switch (lp) {
                case LabelsDb::Label::label_n:
                        return p + Point(0, (Point::coord_t)(Point::from_deg_dbl / 60.0 / 60.0));

                case LabelsDb::Label::label_e:
                        return p + Point((Point::coord_t)(Point::from_deg_dbl / 60.0 / 60.0), 0);

                case LabelsDb::Label::label_s:
                        return p - Point(0, (Point::coord_t)(Point::from_deg_dbl / 60.0 / 60.0));

                case LabelsDb::Label::label_w:
                        return p - Point((Point::coord_t)(Point::from_deg_dbl / 60.0 / 60.0), 0);

                default:
                        return p;
        }
}

double LabelPlacementOpt::label_metric(const Point & p1, const Point & p2)
{
        double d = p1.simple_distance_rel(p2);
        d *= (Point::to_deg_dbl * 60.0) * (Point::to_deg_dbl * 60.0);
        if (d < 0.0001)
                return 10000.0;
        return 1.0 / d;
}

LabelsDb::Label LabelPlacementOpt::find_mutable_label(const Point & pt)
{
        LabelsDb::elementvector_t ev(m_labelsdb.find_by_rect(Rect(pt, pt)));
        for (LabelsDb::elementvector_t::iterator ei(ev.begin()), ee(ev.end()); ei != ee; ei++) {
                if (ei->get_mutable() && ei->get_coord() == pt)
                        return *ei;
        }
        return LabelsDb::Label();
}

void LabelPlacementOpt::save_label(LabelsDb::Label& e)
{
        if (!e.is_valid())
                return;
        if (!e.get_mutable()) {
                m_labelsdb.save(e);
                return;
        }
        LabelsDb::Label e2(find_mutable_label(e.get_coord()));
        if (e2.is_valid())
                return;
        if (e.get_label_placement() == LabelsDb::Label::label_any)
                e.set_label_placement(LabelsDb::Label::label_e);
        m_labelsdb.save(e);
}

void LabelPlacementOpt::copy_arpt_nav_wpt_to_labelsdb(bool arptmutable, bool navmutable, bool wptmutable)
{
        m_labelsdb.purgedb();
        std::cerr << "Analyzing Airport labels..." << std::endl;
        {
                AirportsDb::Airport e;
                m_airportdb.loadfirst(e);
                while (e.is_valid()) {
                        if (e.get_label_placement() != AirportsDb::Airport::label_off) {
                                LabelsDb::Label lbl;
                                lbl.set_label_placement(e.get_label_placement());
                                lbl.set_coord(e.get_coord());
                                lbl.set_mutable(e.get_label_placement() == AirportsDb::Airport::label_any || arptmutable);
                                lbl.set_metric(0);
                                save_label(lbl);
                        }
                        for (unsigned int irte = 0; irte < e.get_nrvfrrte(); irte++) {
                                const AirportsDb::Airport::VFRRoute& rte(e.get_vfrrte(irte));
                                for (unsigned int ipts = 0; ipts < rte.size(); ipts++) {
                                        const AirportsDb::Airport::VFRRoute::VFRRoutePoint& pt(rte[ipts]);
                                        if (pt.get_label_placement() == AirportsDb::Airport::label_off)
                                                continue;
                                        LabelsDb::Label lbl;
                                        lbl.set_label_placement(pt.get_label_placement());
                                        lbl.set_coord(pt.get_coord());
                                        lbl.set_mutable(pt.get_label_placement() == AirportsDb::Airport::label_any || arptmutable);
                                        lbl.set_metric(0);
                                        save_label(lbl);
                                }
                        }
                        m_airportdb.loadnext(e);
                }
        }
        std::cerr << "Analyzing Navaid labels..." << std::endl;
        {
                NavaidsDb::Navaid e;
                m_navaiddb.loadfirst(e);
                while (e.is_valid()) {
                        if (e.get_label_placement() != NavaidsDb::Navaid::label_off) {
                                LabelsDb::Label lbl;
                                lbl.set_label_placement(e.get_label_placement());
                                lbl.set_coord(e.get_coord());
                                lbl.set_mutable(e.get_label_placement() == NavaidsDb::Navaid::label_any || navmutable);
                                lbl.set_metric(0);
                                save_label(lbl);
                        }
                        m_navaiddb.loadnext(e);
                }
        }
        std::cerr << "Analyzing Waypoint labels..." << std::endl;
        {
                WaypointsDb::Waypoint e;
                m_waypointdb.loadfirst(e);
                while (e.is_valid()) {
                        if (e.get_label_placement() != WaypointsDb::Waypoint::label_off) {
                                LabelsDb::Label lbl;
                                lbl.set_label_placement(e.get_label_placement());
                                lbl.set_coord(e.get_coord());
                                lbl.set_mutable(e.get_label_placement() == WaypointsDb::Waypoint::label_any || wptmutable);
                                lbl.set_metric(0);
                                save_label(lbl);
                        }
                        m_waypointdb.loadnext(e);
                }
        }
        std::cerr << "Analyzing Airway labels..." << std::endl;
        {
                AirwaysDb::Airway e;
                m_airwaysdb.loadfirst(e);
                while (e.is_valid()) {
                        if (e.get_label_placement() != AirwaysDb::Airway::label_off) {
                                LabelsDb::Label lbl;
                                lbl.set_label_placement(e.get_label_placement());
                                lbl.set_coord(e.get_labelcoord());
                                lbl.set_mutable(/*e.get_label_placement() == AirwaysDb::Airway::label_any*/ false);
                                lbl.set_metric(0);
                                save_label(lbl);
                        }
                        m_airwaysdb.loadnext(e);
                }
        }
#if 0
        std::cerr << "Analyzing Airspace labels..." << std::endl;
        {
                AirspacesDb::Airspace e;
                m_airspacedb.loadfirst(e);
                while (e.is_valid()) {
                        if (e.get_label_placement() != AirspacesDb::Airspace::label_off &&
                            e.get_label_placement() != AirspacesDb::Airspace::label_any) {
                                LabelsDb::Label lbl;
                                lbl.set_label_placement(e.get_label_placement());
                                lbl.set_coord(e.get_labelcoord());
                                lbl.set_mutable(false);
                                lbl.set_metric(0);
                                lbl.set_subid(e.get_id());
                                lbl.set_subtable(e.get_table());
                                save_label(lbl);
                        }
                        m_airspacedb.loadnext(e);
                }
        }
#endif
}

void LabelPlacementOpt::update_arpt_nav_wpt(void)
{
        std::cerr << "Saving Airport labels..." << std::endl;
        {
                AirportsDb::Airport e;
                m_airportdb.loadfirst(e);
                while (e.is_valid()) {
                        bool changed(false);
                        if (e.get_label_placement() == AirportsDb::Airport::label_any) {
                                LabelsDb::Label lbl(find_mutable_label(e.get_coord()));
                                if (lbl.is_valid()) {
                                        e.set_label_placement(lbl.get_label_placement());
                                        changed = true;
                                }
                        }
                        for (unsigned int irte = 0; irte < e.get_nrvfrrte(); irte++) {
                                AirportsDb::Airport::VFRRoute& rte(e.get_vfrrte(irte));
                                for (unsigned int ipts = 0; ipts < rte.size(); ipts++) {
                                        AirportsDb::Airport::VFRRoute::VFRRoutePoint& pt(rte[ipts]);
                                        if (pt.get_label_placement() != AirportsDb::Airport::label_any)
                                                continue;
                                        LabelsDb::Label lbl(find_mutable_label(pt.get_coord()));
                                        if (lbl.is_valid()) {
                                                pt.set_label_placement(lbl.get_label_placement());
                                                changed = true;
                                        }
                                }
                        }
                        if (changed)
                                m_airportdb.save(e);
                        m_airportdb.loadnext(e);
                }
        }
        std::cerr << "Saving Navaid labels..." << std::endl;
        {
                NavaidsDb::Navaid e;
                m_navaiddb.loadfirst(e);
                while (e.is_valid()) {
                        if (e.get_label_placement() == NavaidsDb::Navaid::label_any) {
                                LabelsDb::Label lbl(find_mutable_label(e.get_coord()));
                                if (lbl.is_valid()) {
                                        e.set_label_placement(lbl.get_label_placement());
                                        m_navaiddb.save(e);
                                }
                        }
                        m_navaiddb.loadnext(e);
                }
        }
        std::cerr << "Saving Waypoint labels..." << std::endl;
        {
                WaypointsDb::Waypoint e;
                m_waypointdb.loadfirst(e);
                while (e.is_valid()) {
                        if (e.get_label_placement() == WaypointsDb::Waypoint::label_any) {
                                LabelsDb::Label lbl(find_mutable_label(e.get_coord()));
                                if (lbl.is_valid()) {
                                        e.set_label_placement(lbl.get_label_placement());
                                        m_waypointdb.save(e);
                                }
                        }
                        m_waypointdb.loadnext(e);
                }
        }
        m_airportdb.analyze();
        m_airportdb.vacuum();
        m_navaiddb.analyze();
        m_navaiddb.vacuum();
        m_waypointdb.analyze();
        m_waypointdb.vacuum();
}

double LabelPlacementOpt::compute_metric(const LabelsDb::Label& e, const Point& pt, LabelsDb::Label::label_placement_t lp)
{
        double m(0);
        Point pdim((Point::coord_t)(Point::from_deg_dbl), (Point::coord_t)(Point::from_deg_dbl));
        LabelsDb::elementvector_t ev(m_labelsdb.find_by_rect(Rect(pt - pdim, pt + pdim)));
        Point plbl(label_offset(pt, lp));
        for (LabelsDb::elementvector_t::iterator ei(ev.begin()), ee(ev.end()); ei != ee; ei++) {
                if (ei->get_id() == e.get_id())
                        continue;
                m += label_metric(label_offset(ei->get_coord(), ei->get_label_placement()), plbl);
        }
        return m;
}

void LabelPlacementOpt::compute_metrics(void)
{
        unsigned int nr(0);
        LabelsDb::Label e;
        m_labelsdb.loadfirst(e);
        while (e.is_valid()) {
                if (e.get_mutable()) {
                        nr++;
                        if ((nr & 1023) == 1)
                                std::cerr << "Computing Metrics " << nr << "..." << std::endl;
                        e.set_metric(compute_metric(e));
                        m_labelsdb.save(e);
                }
                m_labelsdb.loadnext(e);
        }
}

void LabelPlacementOpt::optimize_pass(void)
{
        unsigned int nr(0);
        LabelsDb::elementvector_t ev(m_labelsdb.find_mutable_by_metric());
        for (LabelsDb::elementvector_t::iterator ei(ev.begin()), ee(ev.end()); ei != ee; ei++) {
                if (!ei->get_mutable())
                        continue;
                nr++;
                if ((nr & 1023) == 1)
                        std::cerr << "Optimizing " << nr << "..." << std::endl;
                double m(ei->get_metric());
                LabelsDb::Label::label_placement_t lp(ei->get_label_placement());
                bool chg(false);
                static const LabelsDb::Label::label_placement_t lps[] = {
                        LabelsDb::Label::label_e,
                        LabelsDb::Label::label_n,
                        LabelsDb::Label::label_w,
                        LabelsDb::Label::label_s
                };
                for (unsigned int lpi = 0; lpi < sizeof(lps)/sizeof(lps[0]); lpi++) {
                        LabelsDb::Label::label_placement_t lp1(lps[lpi]);
                        if (lp == lp1)
                                continue;
                        double m1(compute_metric(*ei, ei->get_coord(), lp1));
                        if (m1 < m) {
                                m = m1;
                                lp = lp1;
                                chg = true;
                        }
                }
                if (chg) {
                        ei->set_label_placement(lp);
                        ei->set_metric(m);
                        save_label(*ei);
                }
        }
}

void LabelPlacementOpt::optimize_arpt_nav_wpt(unsigned int passes, bool arptmutable, bool navmutable, bool wptmutable)
{
        copy_arpt_nav_wpt_to_labelsdb(arptmutable, navmutable, wptmutable);
        for (unsigned int pass = 1; pass <= passes; pass++) {
                std::cerr << "Optimize Airport/Waypoint/Navaid labels pass " << pass << "..." << std::endl;
                compute_metrics();
                optimize_pass();
        }
        compute_metrics();
        update_arpt_nav_wpt();
        // set all mutable to nonmutable
        LabelsDb::Label lbl;
        m_labelsdb.loadfirst(lbl);
        while (lbl.is_valid()) {
                if (lbl.get_mutable()) {
                        lbl.set_mutable(false);
                        m_labelsdb.save(lbl);
                }
                m_labelsdb.loadnext(lbl);
        }
}

void LabelPlacementOpt::copy_bdry_to_labelsdb(bool bdrymutable)
{
        std::cerr << "Analyzing Airspace labels..." << std::endl;
        {
                AirspacesDb::Airspace e;
                m_airspacedb.loadfirst(e);
                while (e.is_valid()) {
                        if (e.get_label_placement() == AirspacesDb::Airspace::label_any ||
                            (e.get_label_placement() != AirspacesDb::Airspace::label_off && bdrymutable)) {
                                e.compute_initial_label_placement();
                                LabelsDb::Label lbl;
                                lbl.set_label_placement(AirspacesDb::Airspace::label_center);
                                lbl.set_coord(e.get_labelcoord());
                                lbl.set_mutable(true);
                                lbl.set_metric(0);
                                lbl.set_subid(e.get_id());
                                lbl.set_subtable(e.get_table());
                                save_label(lbl);
                        }
                        m_airspacedb.loadnext(e);
                }
        }
}

void LabelPlacementOpt::update_bdry(void)
{
        std::cerr << "Saving Airspace labels..." << std::endl;
        LabelsDb::Label lbl;
        m_labelsdb.loadfirst(lbl);
        while (lbl.is_valid()) {
                if (lbl.get_mutable()) {
                        AirspacesDb::Airspace e;
                        e = m_airspacedb(lbl.get_subid(), lbl.get_subtable());
                        if (!e.is_valid()) {
                                std::cerr << "Airspace: ID " << lbl.get_subid() << '/' << lbl.get_subtable() << " not found" << std::endl;
                        } else {
                                e.set_labelcoord(lbl.get_coord());
                                e.set_label_placement(lbl.get_label_placement());
                                m_airspacedb.save(e);
                        }
                }
                m_labelsdb.loadnext(lbl);
        }
        m_airspacedb.analyze();
        m_airspacedb.vacuum();
}

void LabelPlacementOpt::optimize_bdry_pass(void)
{
        unsigned int nr(0);
        LabelsDb::elementvector_t ev(m_labelsdb.find_mutable_by_metric());
        for (LabelsDb::elementvector_t::iterator ei(ev.begin()), ee(ev.end()); ei != ee; ei++) {
                if (!ei->get_mutable())
                        continue;
                AirspacesDb::Airspace as(m_airspacedb(ei->get_subid(), ei->get_subtable()));
                if (!as.is_valid()) {
                        std::cerr << "Airspace: ID " << ei->get_subid() << '/' << ei->get_subtable() << " not found" << std::endl;
                        continue;
                }
                nr++;
                if ((nr & 1023) == 1)
                        std::cerr << "Optimizing " << nr << "..." << std::endl;
                double m(ei->get_metric());
                Point coord(ei->get_coord());
                bool chg(false);
                static const Point offs[] = {
                        Point((Point::coord_t)(Point::from_deg_dbl / 60.0 / 2), 0),
                        Point(0, (Point::coord_t)(Point::from_deg_dbl / 60.0 / 2)),
                        Point(-(Point::coord_t)(Point::from_deg_dbl / 60.0 / 2), 0),
                        Point(0, -(Point::coord_t)(Point::from_deg_dbl / 60.0 / 2))
                };
                for (unsigned int oi = 0; oi < sizeof(offs)/sizeof(offs[0]); oi++) {
                        Point pt(ei->get_coord() + offs[oi]);
                        if (!as.get_polygon().windingnumber(pt))
                                continue;
                        double m1(compute_metric(*ei, pt, ei->get_label_placement()));
                        if (m1 < m) {
                                m = m1;
                                coord = pt;
                                chg = true;
                        }
                }
                if (chg) {
                        ei->set_coord(coord);
                        ei->set_metric(m);
                        save_label(*ei);
                }
        }
}

void LabelPlacementOpt::optimize_bdry(unsigned int passes, bool bdrymutable)
{
        copy_bdry_to_labelsdb(bdrymutable);
        for (unsigned int pass = 1; pass <= passes; pass++) {
                std::cerr << "Optimize Airspace labels pass " << pass << "..." << std::endl;
                compute_metrics();
                optimize_bdry_pass();
        }
        update_bdry();
}

void LabelPlacementOpt::optimize(unsigned int arpt_nav_wpt_passes, unsigned int bdry_passes,
                                 bool arptmutable, bool navmutable, bool wptmutable, bool bdrymutable)
{
        if (arpt_nav_wpt_passes < 1 && bdry_passes < 1)
                return;
        optimize_arpt_nav_wpt(arpt_nav_wpt_passes, arptmutable, navmutable, wptmutable);
        if (bdry_passes < 1)
                return;
        optimize_bdry(bdry_passes, bdrymutable);
}




#if 0

/* --------------------------------------------------------------------- */

/* ---------------------------------------------------------------------- */

/* ---------------------------------------------------------------------- */

/* ---------------------------------------------------------------------- */

/* ---------------------------------------------------------------------- */

/* ---------------------------------------------------------------------- */

/* ---------------------------------------------------------------------- */

/* ---------------------------------------------------------------------- */

/* ---------------------------------------------------------------------- */

/* ---------------------------------------------------------------------- */

/* ---------------------------------------------------------------------- */

/* ---------------------------------------------------------------------- */

/* ---------------------------------------------------------------------- */

struct labelmetrics {
	u_int16_t typ;
	u_int16_t index;
	float metric;
};

static double compute_arpt_nav_wpt_label_metric(int32_t lat, int32_t lon, void *ptr)
{
	u_int16_t tile, tilestart, tileend, nr;
	struct dafif_airport *a;
	struct dafif_navaid *n;
	struct dafif_waypoint *w;
	int32_t lat1, lon1;
	double m = 0;

	tilestart = VMapCoordToTileIndex(lat - 0x20000000, lon - 0x20000000);
	tileend = VMapCoordToTileIndex(lat + 0x20000000, lon + 0x20000000);
	for (tile = tilestart; ; tile = VMapNextTile(tile, tilestart, tileend)) {
		nr = arpt_tileindex[tile];
		while (nr >= 3) {
			a = &arpt[nr - 3];
			if (a != ptr) {
				lat1 = a->lat;
				lon1 = a->lon;
				label_offset(&lat1, &lon1, a->label_placement);
				m += label_metric(lat, lon, lat1, lon1);
			}
			nr = a->tile_index;
		}
		nr = nav_tileindex[tile];
		while (nr >= 3) {
			n = &nav[nr - 3];
			if (n != ptr) {
				lat1 = n->lat;
				lon1 = n->lon;
				label_offset(&lat1, &lon1, n->label_placement);
				m += label_metric(lat, lon, lat1, lon1);
			}
			nr = n->tile_index;
		}
		nr = wpt_tileindex[tile];
		while (nr >= 32) {
			w = &wpt[nr - 32];
			if (w != ptr) {
				lat1 = w->lat;
				lon1 = w->lon;
				label_offset(&lat1, &lon1, w->label_placement);
				m += label_metric(lat, lon, lat1, lon1);
			}
			nr = w->tile_index;
		}
		if (tile == tileend)
			break;
	}
	return m;
}

static int sort_metric(const void *p1, const void *p2)
{
	const struct labelmetrics *m1 = p1, *m2 = p2;

	if (m1->metric > m2->metric)
		return -1;
	if (m1->metric < m2->metric)
		return 1;
	return 0;
}

static void place_arpt_nav_wpt_labels(void)
{
	struct labelmetrics *m;
	struct dafif_airport *a;
	struct dafif_navaid *n;
	struct dafif_waypoint *w;
	unsigned int i, j, k;
	float m1, m2;
	int32_t lat, lon;

	if (!nrarpt && !nrnav && !nrwpt)
		return;
	if (!(m = malloc((nrarpt + nrnav + nrwpt) * sizeof(struct labelmetrics))))
		return;
	for (j = 0; j < 3; j++) {
		for (i = 0; i < nrarpt; i++) {
			a = &arpt[i];
			m[i].typ = 0;
			m[i].index = i;
			lat = a->lat;
			lon = a->lon;
			label_offset(&lat, &lon, a->label_placement);
			m[i].metric = compute_arpt_nav_wpt_label_metric(lat, lon, a);
		}
		for (i = 0; i < nrnav; i++) {
			n = &nav[i];
			m[nrarpt + i].typ = 1;
			m[nrarpt + i].index = i;
			lat = n->lat;
			lon = n->lon;
			label_offset(&lat, &lon, n->label_placement);
			m[nrarpt + i].metric = compute_arpt_nav_wpt_label_metric(lat, lon, n);
		}
		for (i = 0; i < nrwpt; i++) {
			w = &wpt[i];
			m[nrarpt + nrnav + i].typ = 2;
			m[nrarpt + nrnav + i].index = i;
			lat = w->lat;
			lon = w->lon;
			label_offset(&lat, &lon, w->label_placement);
			m[nrarpt + nrnav + i].metric = compute_arpt_nav_wpt_label_metric(lat, lon, w);
		}
		qsort(m, nrarpt + nrnav + nrwpt, sizeof(struct labelmetrics), sort_metric);
		printf("Optimization pass %u, largest metric %f...\n", j + 1, m[0].metric);
		for (i = 0; i < nrarpt + nrnav + nrwpt; i++) {
			if (!(i & 1023))
				printf("Optimization pass %u/%u...\n", j + 1, i + 1);
			m1 = m[i].metric;
			switch (m[i].typ) {
			case 0:
				a = &arpt[m[i].index];
				for (k = 1; k < 4; k++) {
					lat = a->lat;
					lon = a->lon;
					label_offset(&lat, &lon, (a->label_placement + k) & 3);
					m2 = compute_arpt_nav_wpt_label_metric(a->lat, a->lon, a);
					if (m2 < m1) {
						m1 = m2;
						a->label_placement = (a->label_placement + k) & 3;
					}
				}
				break;

			case 1:
				n = &nav[m[i].index];
				for (k = 1; k < 4; k++) {
					lat = n->lat;
					lon = n->lon;
					label_offset(&lat, &lon, (n->label_placement + k) & 3);
					m2 = compute_arpt_nav_wpt_label_metric(n->lat, n->lon, n);
					if (m2 < m1) {
						m1 = m2;
						n->label_placement = (n->label_placement + k) & 3;
					}
				}
				break;

			case 2:
				w = &wpt[m[i].index];
				for (k = 1; k < 4; k++) {
					lat = w->lat;
					lon = w->lon;
					label_offset(&lat, &lon, (w->label_placement + k) & 3);
					m2 = compute_arpt_nav_wpt_label_metric(w->lat, w->lon, w);
					if (m2 < m1) {
						m1 = m2;
						w->label_placement = (w->label_placement + k) & 3;
					}
				}
				break;

			default:
				abort();
			}
		}
	}
	free(m);
}

/* ---------------------------------------------------------------------- */

static double compute_bdry_label_metric(struct dafif_boundary *bd, int32_t lat, int32_t lon)
{
	u_int16_t tile, tilestart, tileend, nr;
	struct dafif_airport *a;
	struct dafif_navaid *n;
	struct dafif_boundary *b;
	int32_t lat1, lon1;
	double m = 0;

	tilestart = VMapCoordToTileIndex(bd->latmin - 0x20000000, bd->lonmin - 0x20000000);
	tileend = VMapCoordToTileIndex(bd->latmax + 0x20000000, bd->lonmax + 0x20000000);
	for (tile = tilestart; ; tile = VMapNextTile(tile, tilestart, tileend)) {
		nr = arpt_tileindex[tile];
		while (nr >= 3) {
			a = &arpt[nr - 3];
			lat1 = a->lat;
			lon1 = a->lon;
			label_offset(&lat1, &lon1, a->label_placement);
			m += label_metric(lat, lon, lat1, lon1);
			nr = a->tile_index;
		}
		nr = nav_tileindex[tile];
		while (nr >= 3) {
			n = &nav[nr - 3];
			lat1 = n->lat;
			lon1 = n->lon;
			label_offset(&lat1, &lon1, n->label_placement);
			m += label_metric(lat, lon, lat1, lon1);
			nr = n->tile_index;
		}
		if (tile == tileend)
			break;
	}
	tilestart = VMapCoordToAirspaceTileIndex(bd->latmin - 0x20000000, bd->lonmin - 0x20000000);
	tileend = VMapCoordToAirspaceTileIndex(bd->latmax + 0x20000000, bd->lonmax + 0x20000000);
	for (tile = tilestart; ; tile = VMapNextTile(tile, tilestart, tileend)) {
		nr = bdry_tileindex[tile];
		while (nr >= 2) {
			b = &bdry[nr - 2];
			if (b != bdry)
				m += label_metric(lat, lon, b->labellat, b->labellon);
			nr = b->tile_index;
		}
		if (tile == tileend)
			break;
	}
	return m;
}

static void place_bdry_labels(void)
{
	struct dafif_boundary *b;
	struct labelmetrics *m;
	float m0, m1;
	int32_t lat, lon, dlat, dlon;
	unsigned int i, j;

	if (!nrbdry)
		return;
	if (!(m = malloc(nrbdry * sizeof(struct labelmetrics))))
		return;
	for (j = 0; j < 3; j++) {
		for (i = 0; i < nrbdry; i++) {
			b = &bdry[i];
			m[i].typ = 0;
			m[i].index = i;
			m[i].metric = compute_bdry_label_metric(b, b->labellat, b->labellon);
		}
		qsort(m, nrbdry, sizeof(struct labelmetrics), sort_metric);
		printf("Airspace Optimization pass %u, largest metric %f...\n", j + 1, m[0].metric);
		for (i = 0; i < nrbdry; i++) {
			if (!(i & 1023))
				printf("Airspace Optimization pass %u/%u...\n", j + 1, i + 1);
			b = &bdry[m[i].index];
			m0 = m[i].metric;
			dlat = dlon = 0;
			lat = b->labellat;
			lon = b->labellon + ((int32_t)MIN_TO_GARMIN);
			if (VMapGeomWindingNumber(lat, lon, b->poly, b->nrpoly)) {
				m1 = compute_bdry_label_metric(b, lat, lon);
				if (m1 < m0) {
					m0 = m1;
					dlat = 0;
					dlon = ((int32_t)MIN_TO_GARMIN);
				}
			}
			lat = b->labellat;
			lon = b->labellon - ((int32_t)MIN_TO_GARMIN);
			if (VMapGeomWindingNumber(lat, lon, b->poly, b->nrpoly)) {
				m1 = compute_bdry_label_metric(b, lat, lon);
				if (m1 < m0) {
					m0 = m1;
					dlat = 0;
					dlon = -((int32_t)MIN_TO_GARMIN);
				}
			}
			lat = b->labellat + ((int32_t)MIN_TO_GARMIN);
			lon = b->labellon;
			if (VMapGeomWindingNumber(lat, lon, b->poly, b->nrpoly)) {
				m1 = compute_bdry_label_metric(b, lat, lon);
				if (m1 < m0) {
					m0 = m1;
					dlat = ((int32_t)MIN_TO_GARMIN);
					dlon = 0;
				}
			}
			lat = b->labellat - ((int32_t)MIN_TO_GARMIN);
			lon = b->labellon;
			if (VMapGeomWindingNumber(lat, lon, b->poly, b->nrpoly)) {
				m1 = compute_bdry_label_metric(b, lat, lon);
				if (m1 < m0) {
					m0 = m1;
					dlat = -((int32_t)MIN_TO_GARMIN);
					dlon = 0;
				}
			}
			if (!dlat && !dlon)
				continue;
			b->labellat += dlat;
			b->labellon += dlon;
			for (;;) {
				dlon <<= 1;
				dlat <<= 1;
				if (!dlon && !dlat)
					break;
				lat = b->labellat + dlat;
				lon = b->labellon + dlon;
				if (!VMapGeomWindingNumber(lat, lon, b->poly, b->nrpoly))
					break;
				m1 = compute_bdry_label_metric(b, lat, lon);
				if (m1 > m0)
					break;
				b->labellat = lat;
				b->labellon = lon;
				m0 = m1;
			}
		}
	}
	free(m);
}

/* ---------------------------------------------------------------------- */


#endif

/* ---------------------------------------------------------------------- */


/* ---------------------------------------------------------------------- */

int main(int argc, char *argv[])
{
        static struct option long_options[] = {
                { "help",                no_argument,       NULL, 'h' },
                { "version",             no_argument,       NULL, 'v' },
                { "output-dir",          required_argument, NULL, 'o' },
                { "arpt-nav-wpt-passes", required_argument, NULL, 0x400 },
                { "bdry-passes",         required_argument, NULL, 0x401 },
                { "arpt-mutable",        no_argument,       NULL, 0x500 },
                { "nav-mutable",         no_argument,       NULL, 0x501 },
                { "wpt-mutable",         no_argument,       NULL, 0x502 },
                { "bdry-mutable",        no_argument,       NULL, 0x503 },
                { NULL,                  0,                 NULL, 0 }
        };
        int c, err(0);
        Glib::ustring output_dir(".");
        unsigned int arpt_nav_wpt_passes = 3, bdry_passes = 3;
        bool arptmutable = false, navmutable = false, wptmutable = false, bdrymutable = false;

        while ((c = getopt_long(argc, argv, "hvo:", long_options, NULL)) != -1) {
                switch (c) {
                        case 'v':
                                std::cout << argv[0] << ": (C) 2008, 2009 Thomas Sailer" << std::endl;
                                return 0;

                        case 'o':
                                output_dir = optarg;
                                break;

                        case 0x400:
                                arpt_nav_wpt_passes = strtoul(optarg, 0, 0);
                                break;

                        case 0x401:
                                bdry_passes = strtoul(optarg, 0, 0);
                                break;

                        case 0x500:
                                arptmutable = true;
                                break;

                        case 0x501:
                                navmutable = true;
                                break;

                        case 0x502:
                                wptmutable = true;
                                break;

                        case 0x503:
                                bdrymutable = true;
                                break;

                        case 'h':
                        default:
                                err++;
                                break;
                }
        }
        if (err) {
                std::cerr << "usage: optimizelabelplacement " << std::endl
                          << "     -h, --help             Display this information" << std::endl
                          << "     -v, --version          Display version information" << std::endl
                          << "     -o, --output-dir       Output (Database) Directory" << std::endl
                          << "     --arpt-nav-wpt-passes  Number of Airport/Navaid/Waypoint passes" << std::endl
                          << "     --bdry-passes          Number of Airspace passes" << std::endl
                          << "     --arpt-mutable         Optimize all non-off airport labels (instead of only \"any\" labels)" << std::endl
                          << "     --nav-mutable          Optimize all non-off navaid labels (instead of only \"any\" labels)" << std::endl
                          << "     --wpt-mutable          Optimize all non-off waypoint labels (instead of only \"any\" labels)" << std::endl
                          << "     --bdry-mutable         Optimize all non-off airspace labels (instead of only \"any\" labels)" << std::endl << std::endl;
                return EX_USAGE;
        }
        try {
                LabelPlacementOpt opt(output_dir);
                opt.optimize(arpt_nav_wpt_passes, bdry_passes, arptmutable, navmutable, wptmutable, bdrymutable);

        } catch (const std::exception& e) {
                std::cerr << "Error: " << e.what() << std::endl;
                return EX_DATAERR;
        } catch (const Glib::Exception& e) {
                std::cerr << "Glib Error: " << e.what() << std::endl;
                return EX_DATAERR;
        }
        return 0;
}
