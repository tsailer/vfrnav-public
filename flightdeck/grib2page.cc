/***************************************************************************
 *   Copyright (C) 2013, 2016 by Thomas Sailer                             *
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

#include <iomanip>
#include <glibmm/datetime.h>

#include "flightdeckwindow.h"

FlightDeckWindow::GRIB2LayerModelColumns::GRIB2LayerModelColumns(void)
{
	add(m_text);
	add(m_layer);
}

namespace
{
	struct layersort {
		int compare(const Glib::RefPtr<GRIB2::Layer>& a, const Glib::RefPtr<GRIB2::Layer>& b) const {
			if (!a && b)
				return -1;
			if (a && !b)
				return 1;
			if (!a && !b)
				return 0;
			{
				const GRIB2::Parameter *pa(a->get_parameter());
				const GRIB2::Parameter *pb(b->get_parameter());
				if (!a && b)
					return -2;
				if (a && !b)
					return 2;
				if (a && b) {
					int c(pa->compareid(*pb));
					if (c < 0)
						return -2;
					if (c > 0)
						return 2;
				}
			}
			{
				gint64 ta(a->get_efftime());
				gint64 tb(b->get_efftime());
				if (ta < tb)
					return -3;
				if (ta > tb)
					return 3;
			}
			{
				uint8_t ta(a->get_surface1type());
				uint8_t tb(b->get_surface1type());
				if (ta < tb)
					return -4;
				if (ta > tb)
					return 4;
			}
			{
				double ta(a->get_surface1value());
				double tb(b->get_surface1value());
				if (ta < tb)
					return -5;
				if (ta > tb)
					return 5;
			}
			{
				gint64 ta(a->get_reftime());
				gint64 tb(b->get_reftime());
				if (ta < tb)
					return -6;
				if (ta > tb)
					return 6;
			}
			{
				uint8_t ta(a->get_surface2type());
				uint8_t tb(b->get_surface2type());
				if (ta < tb)
					return -7;
				if (ta > tb)
					return 7;
			}
			{
				double ta(a->get_surface2value());
				double tb(b->get_surface2value());
				if (ta < tb)
					return -8;
				if (ta > tb)
					return 8;
			}
			return 0;
		}

		bool operator()(const Glib::RefPtr<GRIB2::Layer>& a, const Glib::RefPtr<GRIB2::Layer>& b) const {
			return compare(a, b) < 0;
		}
	};
};

void FlightDeckWindow::grib2layer_zoomin(void)
{
	if (!m_grib2drawingarea)
		return;
	m_grib2drawingarea->zoom_in();
	if (m_sensors)
		m_sensors->get_configfile().set_double(cfggroup_mainwindow, cfgkey_mainwindow_grib2mapscale,
						       m_grib2drawingarea->get_map_scale());
}

void FlightDeckWindow::grib2layer_zoomout(void)
{
	if (!m_grib2drawingarea)
		return;
	m_grib2drawingarea->zoom_out();
	if (m_sensors)
		m_sensors->get_configfile().set_double(cfggroup_mainwindow, cfgkey_mainwindow_grib2mapscale,
						       m_grib2drawingarea->get_map_scale());
}

bool FlightDeckWindow::grib2layer_next(void)
{
        if (!m_grib2layertreeview)
                return false;
        Glib::RefPtr<Gtk::TreeSelection> selection(m_grib2layertreeview->get_selection());
        if (!selection)
                return false;
        Gtk::TreeModel::iterator iter(selection->get_selected());
        if (!iter)
                return false;
        ++iter;
        if (!iter)
                return false;
        selection->select(iter);
        return true;
}

bool FlightDeckWindow::grib2layer_prev(void)
{
        if (!m_grib2layertreeview)
                return false;
        Glib::RefPtr<Gtk::TreeSelection> selection(m_grib2layertreeview->get_selection());
        if (!selection)
                return false;
        Gtk::TreeModel::iterator iter(selection->get_selected());
        if (!iter)
                return false;
        // make sure we do not iterate past the beginning
        {
                Gtk::TreeModel::Row row(*iter);
                Gtk::TreeIter iter1(row.parent());
                if (!iter1) {
                        if (!m_grib2layerstore)
                                return false;
                        iter1 = m_grib2layerstore->children().begin();
                } else {
                        row = *iter1;
                        iter1 = row.children().begin();
                }
                if (iter == iter1)
                        return false;
        }
        --iter;
        if (!iter)
                return false;
        selection->select(iter);
        return true;
}

void FlightDeckWindow::grib2layer_togglemap(void)
{
	if (!m_grib2drawingarea)
		return;
	VectorMapRenderer::DrawFlags df(*m_grib2drawingarea);
	df ^= MapRenderer::drawflags_topo;
	if (!(df & MapRenderer::drawflags_topo))
		df ^= MapRenderer::drawflags_weather;
	*m_grib2drawingarea = df;
	m_sensors->get_configfile().set_integer(cfggroup_mainwindow, cfgkey_mainwindow_grib2mapflags, (unsigned int)df);
}

void FlightDeckWindow::grib2layer_unselect(void)
{
        if (m_grib2layerabbreventry) {
		m_grib2layerabbreventry->set_text("");
		m_grib2layerabbreventry->set_sensitive(false);
	}
        if (m_grib2layerunitentry) {
		m_grib2layerunitentry->set_text("");
		m_grib2layerunitentry->set_sensitive(false);
	}
        if (m_grib2layerparamidentry) {
		m_grib2layerparamidentry->set_text("");
		m_grib2layerparamidentry->set_sensitive(false);
	}
        if (m_grib2layerparamentry) {
		m_grib2layerparamentry->set_text("");
		m_grib2layerparamentry->set_sensitive(false);
	}
        if (m_grib2layerparamcatidentry) {
		m_grib2layerparamcatidentry->set_text("");
		m_grib2layerparamcatidentry->set_sensitive(false);
	}
        if (m_grib2layerdisciplineidentry) {
		m_grib2layerdisciplineidentry->set_text("");
		m_grib2layerdisciplineidentry->set_sensitive(false);
	}
        if (m_grib2layerparamcatentry) {
		m_grib2layerparamcatentry->set_text("");
		m_grib2layerparamcatentry->set_sensitive(false);
	}
        if (m_grib2layerdisciplineentry) {
		m_grib2layerdisciplineentry->set_text("");
		m_grib2layerdisciplineentry->set_sensitive(false);
	}
        if (m_grib2layerefftimeentry) {
		m_grib2layerefftimeentry->set_text("");
		m_grib2layerefftimeentry->set_sensitive(false);
	}
        if (m_grib2layerreftimeentry) {
		m_grib2layerreftimeentry->set_text("");
		m_grib2layerreftimeentry->set_sensitive(false);
	}
        if (m_grib2layersurface1entry) {
		m_grib2layersurface1entry->set_text("");
		m_grib2layersurface1entry->set_sensitive(false);
	}
        if (m_grib2layersurface2entry) {
		m_grib2layersurface2entry->set_text("");
		m_grib2layersurface2entry->set_sensitive(false);
	}
        if (m_grib2layersurface1valueentry) {
		m_grib2layersurface1valueentry->set_text("");
		m_grib2layersurface1valueentry->set_sensitive(false);
	}
        if (m_grib2layersurface2valueentry) {
		m_grib2layersurface2valueentry->set_text("");
		m_grib2layersurface2valueentry->set_sensitive(false);
	}
        if (m_grib2layersurface1identry) {
		m_grib2layersurface1identry->set_text("");
		m_grib2layersurface1identry->set_sensitive(false);
	}
        if (m_grib2layersurface2identry) {
		m_grib2layersurface2identry->set_text("");
		m_grib2layersurface2identry->set_sensitive(false);
	}
        if (m_grib2layersurface1unitentry) {
		m_grib2layersurface1unitentry->set_text("");
		m_grib2layersurface1unitentry->set_sensitive(false);
	}
        if (m_grib2layersurface2unitentry) {
		m_grib2layersurface2unitentry->set_text("");
		m_grib2layersurface2unitentry->set_sensitive(false);
	}
        if (m_grib2layercenteridentry) {
		m_grib2layercenteridentry->set_text("");
		m_grib2layercenteridentry->set_sensitive(false);
	}
        if (m_grib2layersubcenteridentry) {
		m_grib2layersubcenteridentry->set_text("");
		m_grib2layersubcenteridentry->set_sensitive(false);
	}
        if (m_grib2layerprodstatusidentry) {
		m_grib2layerprodstatusidentry->set_text("");
		m_grib2layerprodstatusidentry->set_sensitive(false);
	}
        if (m_grib2layerdatatypeidentry) {
		m_grib2layerdatatypeidentry->set_text("");
		m_grib2layerdatatypeidentry->set_sensitive(false);
	}
        if (m_grib2layergenprocidentry) {
		m_grib2layergenprocidentry->set_text("");
		m_grib2layergenprocidentry->set_sensitive(false);
	}
        if (m_grib2layergenproctypidentry) {
		m_grib2layergenproctypidentry->set_text("");
		m_grib2layergenproctypidentry->set_sensitive(false);
	}
        if (m_grib2layercenterentry) {
		m_grib2layercenterentry->set_text("");
		m_grib2layercenterentry->set_sensitive(false);
	}
        if (m_grib2layersubcenterentry) {
		m_grib2layersubcenterentry->set_text("");
		m_grib2layersubcenterentry->set_sensitive(false);
	}
        if (m_grib2layerprodstatusentry) {
		m_grib2layerprodstatusentry->set_text("");
		m_grib2layerprodstatusentry->set_sensitive(false);
	}
        if (m_grib2layerdatatypeentry) {
		m_grib2layerdatatypeentry->set_text("");
		m_grib2layerdatatypeentry->set_sensitive(false);
	}
        if (m_grib2layergenprocentry) {
		m_grib2layergenprocentry->set_text("");
		m_grib2layergenprocentry->set_sensitive(false);
	}
        if (m_grib2layergenproctypentry) {
		m_grib2layergenproctypentry->set_text("");
		m_grib2layergenproctypentry->set_sensitive(false);
	}
	if (m_grib2drawingarea)
		m_grib2drawingarea->set_grib2();
}

void FlightDeckWindow::grib2layer_select(Glib::RefPtr<GRIB2::Layer> layer)
{
	if (!layer) {
		grib2layer_unselect();
		return;
	}
	const GRIB2::Parameter *param(layer->get_parameter());
	if (m_grib2layerabbreventry) {
		m_grib2layerabbreventry->set_text(param->get_abbrev_nonnull());
		m_grib2layerabbreventry->set_sensitive(!!param);
	}
        if (m_grib2layerunitentry) {
		m_grib2layerunitentry->set_text(param->get_unit_nonnull());
		m_grib2layerunitentry->set_sensitive(!!param);
	}
        if (m_grib2layerparamentry) {
		m_grib2layerparamentry->set_text(param->get_str_nonnull());
		m_grib2layerparamentry->set_sensitive(!!param);
	}
        if (m_grib2layerparamidentry) {
		std::ostringstream oss;
		if (param)
			oss << (unsigned int)param->get_id();
		m_grib2layerparamidentry->set_text(oss.str());
		m_grib2layerparamidentry->set_sensitive(!!param);
	}
	const GRIB2::ParamCategory *paramcat(param->get_category());
        if (m_grib2layerparamcatentry) {
		m_grib2layerparamcatentry->set_text(paramcat->get_str_nonnull());
		m_grib2layerparamcatentry->set_sensitive(!!paramcat);
	}
        if (m_grib2layerparamcatidentry) {
		std::ostringstream oss;
		if (paramcat)
			oss << (unsigned int)paramcat->get_id();
		m_grib2layerparamcatidentry->set_text(oss.str());
		m_grib2layerparamcatidentry->set_sensitive(!!paramcat);
	}
	const GRIB2::ParamDiscipline *paramdisc(paramcat->get_discipline());
        if (m_grib2layerdisciplineentry) {
		m_grib2layerdisciplineentry->set_text(paramdisc->get_str_nonnull());
		m_grib2layerdisciplineentry->set_sensitive(!!paramdisc);
	}
        if (m_grib2layerdisciplineidentry) {
		std::ostringstream oss;
		if (paramdisc)
			oss << (unsigned int)paramdisc->get_id();
		m_grib2layerdisciplineidentry->set_text(oss.str());
		m_grib2layerdisciplineidentry->set_sensitive(!!paramdisc);
	}
        if (m_grib2layerefftimeentry) {
		Glib::DateTime dt(Glib::DateTime::create_now_utc(layer->get_efftime()));
		m_grib2layerefftimeentry->set_text(dt.format("%H:%M %F"));
		m_grib2layerefftimeentry->set_sensitive(true);
	}
        if (m_grib2layerreftimeentry) {
		Glib::DateTime dt(Glib::DateTime::create_now_utc(layer->get_reftime()));
		m_grib2layerreftimeentry->set_text(dt.format("%H:%M %F"));
		m_grib2layerreftimeentry->set_sensitive(true);
	}
        if (m_grib2layersurface1entry) {
		m_grib2layersurface1entry->set_text(GRIB2::find_surfacetype_str(layer->get_surface1type(), ""));
		m_grib2layersurface1entry->set_sensitive(layer->get_surface1type() != 255);
	}
        if (m_grib2layersurface1identry) {
		std::ostringstream oss;
		oss << (unsigned int)layer->get_surface1type();
		m_grib2layersurface1identry->set_text(oss.str());
		m_grib2layersurface1identry->set_sensitive(layer->get_surface1type() != 255);
	}
        if (m_grib2layersurface1unitentry) {
		m_grib2layersurface1unitentry->set_text(GRIB2::find_surfaceunit_str(layer->get_surface1type(), ""));
		m_grib2layersurface1unitentry->set_sensitive(layer->get_surface1type() != 255);
	}
        if (m_grib2layersurface1valueentry) {
		std::ostringstream oss;
		oss << layer->get_surface1value() << GRIB2::find_surfaceunit_str(layer->get_surface1type(), "");
		if (layer->get_surface1type() == GRIB2::surface_isobaric_surface) {
			float alt(0);
			IcaoAtmosphere<float>::std_pressure_to_altitude(&alt, 0, layer->get_surface1value() * 0.01);
			oss << " (F" << std::setw(3) << std::setfill('0') << Point::round<int,float>(alt * (Point::m_to_ft * 0.01f)) << ")";
		}
		m_grib2layersurface1valueentry->set_text(oss.str());
		m_grib2layersurface1valueentry->set_sensitive(layer->get_surface1type() != 255);
	}
        if (m_grib2layersurface2entry) {
		m_grib2layersurface2entry->set_text(GRIB2::find_surfacetype_str(layer->get_surface2type(), ""));
		m_grib2layersurface2entry->set_sensitive(layer->get_surface2type() != 255);
	}
        if (m_grib2layersurface2identry) {
		std::ostringstream oss;
		oss << (unsigned int)layer->get_surface2type();
		m_grib2layersurface2identry->set_text(oss.str());
		m_grib2layersurface2identry->set_sensitive(layer->get_surface2type() != 255);
	}
        if (m_grib2layersurface2unitentry) {
		m_grib2layersurface2unitentry->set_text(GRIB2::find_surfaceunit_str(layer->get_surface2type(), ""));
		m_grib2layersurface2unitentry->set_sensitive(layer->get_surface2type() != 255);
	}
        if (m_grib2layersurface2valueentry) {
		std::ostringstream oss;
		oss << layer->get_surface2value() << GRIB2::find_surfaceunit_str(layer->get_surface2type(), "");
		if (layer->get_surface2type() == GRIB2::surface_isobaric_surface) {
			float alt(0);
			IcaoAtmosphere<float>::std_pressure_to_altitude(&alt, 0, layer->get_surface2value() * 0.01);
			oss << " (F" << std::setw(3) << std::setfill('0') << Point::round<int,float>(alt * (Point::m_to_ft * 0.01f)) << ")";
		}
		m_grib2layersurface2valueentry->set_text(oss.str());
		m_grib2layersurface2valueentry->set_sensitive(layer->get_surface2type() != 255);
	}
        if (m_grib2layercenterentry) {
		m_grib2layercenterentry->set_text(GRIB2::find_centerid_str(layer->get_centerid(), ""));
		m_grib2layercenterentry->set_sensitive(true);
	}
        if (m_grib2layercenteridentry) {
		std::ostringstream oss;
		oss << (unsigned int)layer->get_centerid();
		m_grib2layercenteridentry->set_text(oss.str());
		m_grib2layercenteridentry->set_sensitive(true);
	}
        if (m_grib2layersubcenterentry) {
		m_grib2layersubcenterentry->set_text(GRIB2::find_subcenterid_str(layer->get_centerid(), layer->get_subcenterid(), ""));
		m_grib2layersubcenterentry->set_sensitive(true);
	}
        if (m_grib2layersubcenteridentry) {
		std::ostringstream oss;
		oss << (unsigned int)layer->get_subcenterid();
		m_grib2layersubcenteridentry->set_text(oss.str());
		m_grib2layersubcenteridentry->set_sensitive(true);
	}
        if (m_grib2layergenproctypentry) {
		m_grib2layergenproctypentry->set_text(GRIB2::find_genprocesstype_str(layer->get_centerid(), layer->get_genprocesstype(), ""));
		m_grib2layergenproctypentry->set_sensitive(true);
	}
        if (m_grib2layergenproctypidentry) {
		std::ostringstream oss;
		oss << (unsigned int)layer->get_genprocesstype();
		m_grib2layergenproctypidentry->set_text(oss.str());
		m_grib2layergenproctypidentry->set_sensitive(true);
	}
        if (m_grib2layerprodstatusentry) {
		m_grib2layerprodstatusentry->set_text(GRIB2::find_productionstatus_str(layer->get_productionstatus(), ""));
		m_grib2layerprodstatusentry->set_sensitive(true);
	}
        if (m_grib2layerprodstatusidentry) {
		std::ostringstream oss;
		oss << (unsigned int)layer->get_productionstatus();
		m_grib2layerprodstatusidentry->set_text(oss.str());
		m_grib2layerprodstatusidentry->set_sensitive(true);
	}
        if (m_grib2layerdatatypeentry) {
		m_grib2layerdatatypeentry->set_text(GRIB2::find_datatype_str(layer->get_datatype(), ""));
		m_grib2layerdatatypeentry->set_sensitive(true);
	}
        if (m_grib2layerdatatypeidentry) {
		std::ostringstream oss;
		oss << (unsigned int)layer->get_datatype();
		m_grib2layerdatatypeidentry->set_text(oss.str());
		m_grib2layerdatatypeidentry->set_sensitive(true);
	}
        if (m_grib2layergenprocentry) {
		m_grib2layergenprocentry->set_text(GRIB2::find_genprocess_str(layer->get_genprocess(), ""));
		m_grib2layergenprocentry->set_sensitive(true);
	}
        if (m_grib2layergenprocidentry) {
		std::ostringstream oss;
		oss << (unsigned int)layer->get_genprocess();
		m_grib2layergenprocidentry->set_text(oss.str());
		m_grib2layergenprocidentry->set_sensitive(true);
	}
	if (m_grib2drawingarea) {
		int alt(m_grib2drawingarea->get_altitude());
		if (layer->get_surface1type() == GRIB2::surface_isobaric_surface) {
			float alt1(0);
			IcaoAtmosphere<float>::std_pressure_to_altitude(&alt1, 0, layer->get_surface1value() * 0.01);
			alt = Point::round<int,float>(alt1 * Point::m_to_ft);
		} else if (m_sensors) {
			double balt(std::numeric_limits<double>::quiet_NaN());
			double talt(std::numeric_limits<double>::quiet_NaN());
			m_sensors->get_altitude(balt, talt);
			if (std::isnan(balt))
				alt = Point::round<int,double>(balt);
			else if (std::isnan(talt))
				alt = Point::round<int,double>(talt);
		}
		m_grib2drawingarea->set_center(m_grib2drawingarea->get_center(), alt, 0, layer->get_efftime());
		m_grib2drawingarea->set_grib2(layer);
	}
}

bool FlightDeckWindow::grib2layer_displaysel(void)
{
	if (!m_grib2layertreeview) {
		grib2layer_unselect();
                return false;
	}
        Glib::RefPtr<Gtk::TreeSelection> selection(m_grib2layertreeview->get_selection());
        if (!selection) {
		grib2layer_unselect();
                return false;
	}
        Gtk::TreeModel::iterator iter(selection->get_selected());
        if (!iter) {
		grib2layer_unselect();
                return false;
	}
        Gtk::TreeModel::Row row(*iter);
        GRIB2LayerModelColumns::LayerPtr layerptr(row[m_grib2layercolumns.m_layer]);
        Glib::RefPtr<GRIB2::Layer> layer(layerptr.get());
	if (!layer) {
		grib2layer_unselect();
		return false;
	}
	grib2layer_select(layer);
	return true;
}

void FlightDeckWindow::grib2layer_hide(void)
{
	if (m_grib2drawingarea) {
		m_grib2drawingarea->hide();
		m_grib2drawingarea->set_grib2();
	}
}

void FlightDeckWindow::grib2layer_update(void)
{
        if (m_grib2drawingarea) {
                m_grib2drawingarea->set_renderer(VectorMapArea::renderer_vmap);
		*m_grib2drawingarea = MapRenderer::drawflags_topo | MapRenderer::drawflags_terrain_borders |
			MapRenderer::drawflags_route | MapRenderer::drawflags_northpointer;
                if (m_sensors && m_sensors->get_configfile().has_group(cfggroup_mainwindow)) {
                        if (m_sensors->get_configfile().has_key(cfggroup_mainwindow, cfgkey_mainwindow_grib2mapscale))
                                m_grib2drawingarea->set_map_scale(m_sensors->get_configfile().get_double(cfggroup_mainwindow,
													 cfgkey_mainwindow_grib2mapscale));
			if (m_sensors->get_configfile().has_key(cfggroup_mainwindow, cfgkey_mainwindow_grib2mapflags))
				*m_grib2drawingarea = (MapRenderer::DrawFlags)m_sensors->get_configfile().get_integer(cfggroup_mainwindow, cfgkey_mainwindow_grib2mapflags);
                }
                m_grib2drawingarea->show();
		if (m_sensors) {
			Point pt;
			{
				bool posok(m_sensors->get_position(pt));
				if (!posok)
					pt.set_invalid();
			}
			if (m_sensors->get_route().get_nrwpt() > 0)
				pt = m_sensors->get_route().get_bbox().boxcenter();
			if (pt.is_invalid())
				pt = m_grib2drawingarea->get_center();
			int alt(m_grib2drawingarea->get_altitude());
			{
				double balt(std::numeric_limits<double>::quiet_NaN());
				double talt(std::numeric_limits<double>::quiet_NaN());
				m_sensors->get_altitude(balt, talt);
				if (!std::isnan(balt)) {
					alt = Point::round<int,double>(balt);
				} else if (!std::isnan(talt)) {
					alt = Point::round<int,double>(talt);
				}
			}
			Glib::TimeVal tv;
			tv.assign_current_time();
			m_grib2drawingarea->set_center(pt, alt, 0, tv.tv_sec);
			m_grib2drawingarea->set_renderer(VectorMapArea::renderer_vmap);
		}
        }
	if (!m_grib2layerstore)
		return;
        m_grib2layerselchangeconn.block();
	m_grib2layerstore->clear();
        m_grib2layerselchangeconn.unblock();
       	if (!m_engine)
		return;
	GRIB2::layerlist_t layers(m_engine->get_grib2_db().find_layers());
	std::sort(layers.begin(), layers.end(), layersort());
	while (!layers.empty() && !*layers.begin())
		layers.erase(layers.begin());
	static const int compmin(2);
	static const int compmax(8);
	int lastcomp(compmin);
	Gtk::TreeIter iters[compmax - compmin + 1];
	for (GRIB2::layerlist_t::iterator li(layers.begin()), le(layers.end()); li != le; ++li) {
		int maxcomp(lastcomp);
		unsigned int compmask(1U << lastcomp);
		int nextcomp(lastcomp);
		{
			bool first(true);
			GRIB2::layerlist_t::iterator li2(li);
			for (;;) {
				++li2;
				if (li2 == le)
					break;
				int comp(-layersort().compare(*li, *li2));
				comp = std::max(std::min(comp, compmax), compmin);
				if (first) {
					nextcomp = comp;
					first = false;
				}
				if (comp <= lastcomp)
					break;
				maxcomp = std::max(maxcomp, comp);
				compmask |= 1U << comp;
			}
		}
		for (int comp = lastcomp; comp <= compmax; ++comp) {
			if (!(compmask & (1U << comp))) {
				if (comp > compmin)
					iters[comp - compmin] = iters[comp - compmin - 1];
				continue;
			}
			if (comp > compmin) {
				Gtk::TreeModel::Row row(*iters[comp - compmin - 1]);
				iters[comp - compmin] = m_grib2layerstore->append(row.children());
			} else {
				iters[0] = m_grib2layerstore->append();
			}
			Gtk::TreeModel::Row row(*iters[comp - compmin]);
			switch (comp) {
			case 2:
			{
				const GRIB2::Parameter *param((*li)->get_parameter());
				Glib::ustring txt(param->get_abbrev_nonnull());
				{
					const char *cp(param->get_unit());
					if (cp) {
						txt += " (";
						txt += cp;
						txt += ")";
					}
					txt += " ";
				}
				txt += param->get_str_nonnull();
				row[m_grib2layercolumns.m_text] = txt;
				break;
			}

			case 3:
			{
				Glib::DateTime dt(Glib::DateTime::create_now_utc((*li)->get_efftime()));
				row[m_grib2layercolumns.m_text] = dt.format("%H:%M %F");
				break;
			}

			case 4:
			{
				row[m_grib2layercolumns.m_text] = GRIB2::find_surfacetype_str((*li)->get_surface1type(), "");
				break;
			}

			case 5:
			{
				std::ostringstream oss;
				oss << (*li)->get_surface1value() << GRIB2::find_surfaceunit_str((*li)->get_surface1type(), "");
				if ((*li)->get_surface1type() == GRIB2::surface_isobaric_surface) {
					float alt(0);
					IcaoAtmosphere<float>::std_pressure_to_altitude(&alt, 0, (*li)->get_surface1value() * 0.01);
					oss << " (F" << std::setw(3) << std::setfill('0') << Point::round<int,float>(alt * (Point::m_to_ft * 0.01f)) << ")";
				}
				row[m_grib2layercolumns.m_text] = oss.str();
				break;
			}

			case 6:
			{
				Glib::DateTime dt(Glib::DateTime::create_now_utc((*li)->get_reftime()));
				row[m_grib2layercolumns.m_text] = dt.format("%H:%M %F");
				break;
			}

			case 7:
			{
				row[m_grib2layercolumns.m_text] = GRIB2::find_surfacetype_str((*li)->get_surface2type(), "");
				break;
			}

			case 8:
			{
				std::ostringstream oss;
				oss << (*li)->get_surface2value() << GRIB2::find_surfaceunit_str((*li)->get_surface2type(), "");
				if ((*li)->get_surface2type() == GRIB2::surface_isobaric_surface) {
					float alt(0);
					IcaoAtmosphere<float>::std_pressure_to_altitude(&alt, 0, (*li)->get_surface2value() * 0.01);
					oss << " (F" << std::setw(3) << std::setfill('0') << Point::round<int,float>(alt * (Point::m_to_ft * 0.01f)) << ")";
				}
				row[m_grib2layercolumns.m_text] = oss.str();
				break;
			}

			default:
				row[m_grib2layercolumns.m_text] = "";
				break;
			}
			row[m_grib2layercolumns.m_layer] = GRIB2LayerModelColumns::LayerPtr(*li);
		}
		lastcomp = nextcomp;
	}
}
