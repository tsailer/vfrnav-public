//
// C++ Implementation: NavArea Container Widget
//
// Description: NavArea Container Widget
//
//
// Author: Thomas Sailer <t.sailer@alumni.ethz.ch>, (C) 2011, 2012, 2013
//
// Copyright: See COPYING file that comes with this distribution
//
//

#include "sysdeps.h"

#include <glibmm.h>

#include "flightdeckwindow.h"

const char FlightDeckWindow::NavArea::cfgkey_mainwindow_mapscale[] = "mapscale";
const char FlightDeckWindow::NavArea::cfgkey_mainwindow_arptmapscale[] = "arptscale";
const char FlightDeckWindow::NavArea::cfgkey_mainwindow_terrainmapscale[] = "terrainscale";
const char * const FlightDeckWindow::NavArea::cfgkey_mainwindow_mapscales[2] = {
	cfgkey_mainwindow_mapscale,
	cfgkey_mainwindow_arptmapscale
};
const char FlightDeckWindow::NavArea::cfgkey_mainwindow_renderers[] = "renderers";
const char FlightDeckWindow::NavArea::cfgkey_mainwindow_bitmapmaps[] = "bitmapmaps";
const char FlightDeckWindow::NavArea::cfgkey_mainwindow_mapflags[] = "mapflags";
const char FlightDeckWindow::NavArea::cfgkey_mainwindow_terrainflags[] = "terrainflags";
const char FlightDeckWindow::NavArea::cfgkey_mainwindow_mapdeclutter[] = "mapdeclutter";
const char FlightDeckWindow::NavArea::cfgkey_mainwindow_mapup[] = "mapup";
const char FlightDeckWindow::NavArea::cfgkey_mainwindow_hsiwind[] = "hsiwind";
const char FlightDeckWindow::NavArea::cfgkey_mainwindow_hsiterrain[] = "hsiterrain";
const char FlightDeckWindow::NavArea::cfgkey_mainwindow_hsiglide[] = "hsiglide";
const char FlightDeckWindow::NavArea::cfgkey_mainwindow_altinhg[] = "altinhg";
const char FlightDeckWindow::NavArea::cfgkey_mainwindow_altmetric[] = "altmetric";
const char FlightDeckWindow::NavArea::cfgkey_mainwindow_altlabels[] = "altlabels";
const char FlightDeckWindow::NavArea::cfgkey_mainwindow_altbug[] = "altbug";
const char FlightDeckWindow::NavArea::cfgkey_mainwindow_altminimum[] = "altminimum";

const MapRenderer::DrawFlags FlightDeckWindow::NavArea::declutter_mask[dcltr_number] = {
	~MapRenderer::drawflags_none,
	~(MapRenderer::drawflags_topo |
	  MapRenderer::drawflags_terrain | MapRenderer::drawflags_terrain_names | MapRenderer::drawflags_terrain_borders |
	  MapRenderer::drawflags_airways_low | MapRenderer::drawflags_airways_high),
	~(MapRenderer::drawflags_topo |
	  MapRenderer::drawflags_terrain | MapRenderer::drawflags_terrain_names | MapRenderer::drawflags_terrain_borders |
	  MapRenderer::drawflags_airspaces),
	~(MapRenderer::drawflags_airways_low | MapRenderer::drawflags_airways_high |
	  MapRenderer::drawflags_airspaces)
};

const char * const FlightDeckWindow::NavArea::declutter_names[dcltr_number + 1] = {
	"D:NORM", "D:ASPC", "D:AWY", "D:TERR", "D:?"
};

FlightDeckWindow::NavArea::NavArea(BaseObjectType *castitem, const Glib::RefPtr<Builder>& refxml)
	: Gtk::Container(castitem), m_engine(0), m_sensors(0), m_hsiwind(true), m_hsiterrain(true), m_hsiglide(true),
	  m_altlabels(true), m_mapup(mapup_north), m_mapdeclutter(dcltr_normal),
	  m_renderers((1U << VectorMapArea::renderer_vmap) | (1U << VectorMapArea::renderer_airportdiagram))
{
	set_has_window(false);
	set_redraw_on_allocate(false);
	m_navmapdrawingarea.set_parent(*this);
	m_navmapdrawingarea.set_visible(true);
	m_navaidrawingarea.set_parent(*this);
	m_navaidrawingarea.set_visible(true);
	m_navhsidrawingarea.set_parent(*this);
	m_navhsidrawingarea.set_visible(true);
	m_navaltdrawingarea.set_parent(*this);
	m_navaltdrawingarea.set_visible(true);
	m_navinfodrawingarea.set_parent(*this);
	m_navinfodrawingarea.set_visible(true);
	m_mapdrawflags = m_navmapdrawingarea;
	m_navmapdrawingarea.set_dragbutton(0);
}

FlightDeckWindow::NavArea::NavArea(void)
	: m_sensors(0), m_hsiwind(true), m_hsiterrain(true), m_hsiglide(true),
	  m_altlabels(true), m_mappan(false), m_mapup(mapup_north), m_mapdeclutter(dcltr_normal),
	  m_renderers((1U << VectorMapArea::renderer_vmap) | (1U << VectorMapArea::renderer_airportdiagram))
{
	set_has_window(false);
	set_redraw_on_allocate(false);
	m_navmapdrawingarea.set_parent(*this);
	m_navmapdrawingarea.set_visible(true);
	m_navaidrawingarea.set_parent(*this);
	m_navaidrawingarea.set_visible(true);
	m_navhsidrawingarea.set_parent(*this);
	m_navhsidrawingarea.set_visible(true);
	m_navaltdrawingarea.set_parent(*this);
	m_navaltdrawingarea.set_visible(true);
	m_navinfodrawingarea.set_parent(*this);
	m_navinfodrawingarea.set_visible(true);
	m_mapdrawflags = m_navmapdrawingarea;
	m_navmapdrawingarea.set_dragbutton(0);
}

FlightDeckWindow::NavArea::~NavArea()
{
	m_connmapaircraft.disconnect();
}

void FlightDeckWindow::NavArea::sensors_change(Sensors *sensors, Sensors::change_t changemask)
{
	if (!sensors || !changemask)
		return;
	if (changemask & (Sensors::change_position | Sensors::change_altitude | Sensors::change_course)) {
		Point pt;
		TopoDb30::elev_t elev;
		bool posok(sensors->get_position(pt, elev));
		double baroalt, truealt;
		sensors->get_altitude(baroalt, truealt);
		bool altok(!std::isnan(truealt));
		if (!altok)
			truealt = 0;
		double crsmag, crstrue, groundspeed;
		sensors->get_course(crsmag, crstrue, groundspeed);
		{
			Sensors::mapangle_t upangleflags(Sensors::mapangle_true);
			if (m_mapup >= mapup_course)
				upangleflags |= Sensors::mapangle_course;
			if (m_mapup >= mapup_heading)
				upangleflags |= Sensors::mapangle_heading;
			if (false)
				std::cerr << "NavArea::sensors_change: mapup " << (unsigned int)m_mapup
					  << " upangleflags " << (unsigned int)upangleflags << std::endl;
			double upangle;
			upangleflags = sensors->get_mapangle(upangle, upangleflags);
			if (false)
				std::cerr << "NavArea::sensors_change: upangle " << upangle
					  << " upangleflags " << (unsigned int)upangleflags << std::endl;
			if (std::isnan(upangle))
				upangle = 0;
			Glib::TimeVal tv;
			tv.assign_current_time();
			if (!m_mappan)
				m_navmapdrawingarea.set_center(pt, truealt, upangle * MapRenderer::from_deg_16bit_dbl, tv.tv_sec);
		}
		if (posok) {
			m_navmapdrawingarea.set_cursor(pt);
		} else {
			m_navmapdrawingarea.invalidate_cursor();
		}
		if (std::isnan(crstrue))
			m_navmapdrawingarea.invalidate_cursor_angle();
		else
			m_navmapdrawingarea.set_cursor_angle(crstrue);
		m_navhsidrawingarea.set_declination(sensors->is_declination_ok() ? sensors->get_declination()
						    : std::numeric_limits<double>::quiet_NaN());
		if (!posok)
			m_navhsidrawingarea.set_center();
		else if (!altok || !m_hsiterrain)
			m_navhsidrawingarea.set_center(pt);
		else
			m_navhsidrawingarea.set_center(pt, truealt);
		m_navaltdrawingarea.set_elevation(elev);
	}
	if (changemask & Sensors::change_altitude) {
		double baroalt, truealt, altrate;
		sensors->get_altitude(baroalt, truealt, altrate);
		bool std(m_navaltdrawingarea.is_std());
		m_navaltdrawingarea.set_altitude(std ? baroalt : truealt, altrate);
	}
	if (alt_is_labels()) {
		NavAltDrawingArea::altmarkers_t m;
		bool std(m_navaltdrawingarea.is_std());
		for (unsigned int i = 0; i < sensors->size(); ++i) {
			Sensors::Sensor& sens((*sensors)[i]);
			double truealt(std::numeric_limits<double>::quiet_NaN());
			if (sens.is_truealt_ok()) {
				double altrate;
				sens.get_truealt(truealt, altrate);
				if (std)
					truealt = sensors->true_to_pressure_altitude(truealt);
			}
			double baroalt(std::numeric_limits<double>::quiet_NaN());
			if (sens.is_baroalt_ok()) {
				double altrate;
				sens.get_baroalt(baroalt, altrate);
				if (!std)
					baroalt = sensors->pressure_to_true_altitude(baroalt);
			}
			const Glib::ustring& name(sens.get_name());
			if (!std::isnan(truealt) && !std::isnan(baroalt)) {
				m.push_back(std::make_pair(name + "t", truealt));
				m.push_back(std::make_pair(name + "b", baroalt));
				continue;
			}
			if (!std::isnan(truealt))
				m.push_back(std::make_pair(name, truealt));
			if (!std::isnan(baroalt))
				m.push_back(std::make_pair(name, baroalt));
		}
		m_navaltdrawingarea.set_altmarkers(m);
	}
	Sensors::mapangle_t mapangleflags(Sensors::mapangle_course | Sensors::mapangle_heading | Sensors::mapangle_manualheading);
	double mapangle;
	if (changemask & (Sensors::change_attitude | Sensors::change_course | Sensors::change_heading | Sensors::change_navigation)) {
		if (sensors->is_manualheading_true())
			mapangleflags |= Sensors::mapangle_true;
		mapangleflags = sensors->get_mapangle(mapangle, mapangleflags);
	}
	if (changemask & (Sensors::change_attitude | Sensors::change_course | Sensors::change_heading)) {
		double crsmag, crstrue, groundspeed;
		sensors->get_course(crsmag, crstrue, groundspeed);
		double hdgmag, hdgtrue;
		sensors->get_heading(hdgmag, hdgtrue);
		Sensors::mapangle_t windhdgflags((Sensors::mapangle_t)0);
		if (m_hsiwind)
			windhdgflags |= Sensors::mapangle_heading | Sensors::mapangle_manualheading;
		if (sensors->is_manualheading_true())
			windhdgflags |= Sensors::mapangle_true;
		double windhdg;
		windhdgflags = sensors->get_mapangle(windhdg, windhdgflags);
		double pitch, bank, slip, rate;
		sensors->get_attitude(pitch, bank, slip, rate);
		if (changemask & Sensors::change_attitude)
			m_navaidrawingarea.set_pitch_bank_slip(pitch, bank, slip);
		m_navhsidrawingarea.set_heading_rate(mapangle, rate, mapangleflags);
		m_navhsidrawingarea.set_wind_heading(windhdg, windhdgflags);
		m_navhsidrawingarea.set_course_groundspeed((mapangleflags & Sensors::mapangle_true) ? crstrue : crsmag, groundspeed);
	}
	if (changemask & Sensors::change_airdata) {
		m_navhsidrawingarea.set_rat(sensors->get_airdata_rat());
		m_navhsidrawingarea.set_oat(sensors->get_airdata_oat());
		m_navhsidrawingarea.set_ias(sensors->get_airdata_cas());
		m_navhsidrawingarea.set_tas(sensors->get_airdata_tas());
		m_navhsidrawingarea.set_mach(sensors->get_airdata_mach());
	}
	if (changemask & Sensors::change_navigation) {
		if (sensors->nav_is_on()) {
			double brg((mapangleflags & Sensors::mapangle_true) ? sensors->nav_get_track_true() : sensors->nav_get_track_mag());
			bool isfrom(abs(sensors->nav_get_trkerr()) >= (1 << 30));
			m_navhsidrawingarea.set_pointer1(brg, -sensors->nav_get_xtk(), sensors->nav_get_curdist());
			m_navhsidrawingarea.set_pointer1flag(isfrom ? NavHSIDrawingArea::pointer1flag_from : NavHSIDrawingArea::pointer1flag_to);
			m_navhsidrawingarea.set_pointer1maxdev(sensors->nav_get_maxxtk());
			m_navhsidrawingarea.set_pointer1dest(sensors->nav_get_curdest());
			m_navaltdrawingarea.set_vsbug(sensors->nav_get_roc());
			m_navaltdrawingarea.set_glideslope(sensors->nav_get_glideslope());
		} else {
			m_navhsidrawingarea.set_pointer1();
			m_navhsidrawingarea.set_pointer1flag(NavHSIDrawingArea::pointer1flag_off);
			m_navhsidrawingarea.set_pointer1dest();
			m_navaltdrawingarea.unset_vsbug();
			m_navaltdrawingarea.unset_glideslope();
		}
		{
			double brg((mapangleflags & Sensors::mapangle_true) ? sensors->nav_get_brg2_track_true() : sensors->nav_get_brg2_track_mag());
			double dist(sensors->nav_get_brg2_dist());
			if (std::isnan(brg))
				m_navhsidrawingarea.set_pointer2dest();
			else
				m_navhsidrawingarea.set_pointer2dest(sensors->nav_get_brg2_dest());
			m_navhsidrawingarea.set_pointer2(brg, dist);
		}
	}
	m_navinfodrawingarea.sensors_change(sensors, changemask);
	if (changemask & (Sensors::change_position | Sensors::change_fplan | Sensors::change_navigation)) {
		Sensors::fplanwind_t wind(sensors->get_fplan_wind());
		hsi_set_glidewind(wind.first, wind.second);
	}
}

#ifdef HAVE_GTKMM2
void FlightDeckWindow::NavArea::on_size_request(Gtk::Requisition* requisition)
{
	if (!requisition)
		return;
	*requisition = m_navmapdrawingarea.size_request();
	Gtk::Requisition reqsidepane(m_navaltdrawingarea.size_request());
	{
		Gtk::Requisition req(m_navinfodrawingarea.size_request());
		reqsidepane.width += req.width;
		reqsidepane.height = std::max(reqsidepane.height, req.height);
	}
	{
		Gtk::Requisition req(m_navhsidrawingarea.size_request());
		reqsidepane.width = std::max(reqsidepane.width, req.width);
		reqsidepane.height = std::max(reqsidepane.height, req.height);
	}
	{
		Gtk::Requisition req(m_navaidrawingarea.size_request());
		reqsidepane.width = std::max(reqsidepane.width, req.width);
		reqsidepane.height = std::max(reqsidepane.height, req.height);
	}
	requisition->width += reqsidepane.width;
	requisition->height = std::max(requisition->height, 3 * reqsidepane.height);
}
#endif

#ifdef HAVE_GTKMM3
Gtk::SizeRequestMode FlightDeckWindow::NavArea::get_request_mode_vfunc() const
{
	return Gtk::Container::get_request_mode_vfunc();
}

void FlightDeckWindow::NavArea::get_preferred_width_vfunc(int& minimum_width, int& natural_width) const
{
	int mwidth(0), nwidth(0);
	m_navinfodrawingarea.get_preferred_width(mwidth, nwidth);
	{
		int mw(0), nw(0);
		m_navhsidrawingarea.get_preferred_width(mw, nw);
		mwidth = std::max(mwidth, mw);
		nwidth = std::max(nwidth, nw);
	}
	{
		int mw(0), nw(0);
		m_navaidrawingarea.get_preferred_width(mw, nw);
		mwidth = std::max(mwidth, mw);
		nwidth = std::max(nwidth, nw);
	}
	{
		int mw(0), nw(0);
		m_navmapdrawingarea.get_preferred_width(mw, nw);
		mwidth += mw;
		nwidth += nw;
	}
	minimum_width = mwidth;
	natural_width = nwidth;
	if (false)
		std::cerr << "NavArea::get_preferred_width_vfunc mwidth " << mwidth << " nwidth " << nwidth << std::endl;
}

void FlightDeckWindow::NavArea::get_preferred_height_for_width_vfunc(int width, int& minimum_height, int& natural_height) const
{
	int width3(width / 3);
	int mheight(0), nheight(0);
	m_navinfodrawingarea.get_preferred_height_for_width(width3, mheight, nheight);
	{
		int mh(0), nh(0);
		m_navhsidrawingarea.get_preferred_height_for_width(width3, mh, nh);
		mheight += mh;
		nheight += nh;
	}
	{
		int mh(0), nh(0);
		m_navaidrawingarea.get_preferred_height_for_width(width3, mh, nh);
		mheight += mh;
		nheight += nh;
	}
	{
		int mh(0), nh(0);
		m_navmapdrawingarea.get_preferred_height_for_width(width - width3, mh, nh);
		mheight = std::max(mheight, mh);
		nheight = std::max(nheight, nh);
	}
	minimum_height = mheight;
	natural_height = nheight;
	if (false)
		std::cerr << "NavArea::get_preferred_height_for_width_vfunc width " << width << " mheight " << mheight << " nheight " << nheight << std::endl;
}

void FlightDeckWindow::NavArea::get_preferred_height_vfunc(int& minimum_height, int& natural_height) const
{
	int mheight(0), nheight(0);
	m_navinfodrawingarea.get_preferred_height(mheight, nheight);
	{
		int mh(0), nh(0);
		m_navhsidrawingarea.get_preferred_height(mh, nh);
		mheight += mh;
		nheight += nh;
	}
	{
		int mh(0), nh(0);
		m_navaidrawingarea.get_preferred_height(mh, nh);
		mheight += mh;
		nheight += nh;
	}
	{
		int mh(0), nh(0);
		m_navmapdrawingarea.get_preferred_height(mh, nh);
		mheight = std::max(mheight, mh);
		nheight = std::max(nheight, nh);
	}
	minimum_height = mheight;
	natural_height = nheight;
	if (false)
		std::cerr << "NavArea::get_preferred_height_vfunc mheight " << mheight << " nheight " << nheight << std::endl;
}

void FlightDeckWindow::NavArea::get_preferred_width_for_height_vfunc(int height, int& minimum_width, int& natural_width) const
{
	int height3(height / 3);
	int mwidth(0), nwidth(0);
	m_navinfodrawingarea.get_preferred_width_for_height(height3, mwidth, nwidth);
	{
		int mw(0), nw(0);
		m_navhsidrawingarea.get_preferred_width_for_height(height3, mw, nw);
		mwidth = std::max(mwidth, mw);
		nwidth = std::max(nwidth, nw);
	}
	{
		int mw(0), nw(0);
		m_navaidrawingarea.get_preferred_width_for_height(height3, mw, nw);
		mwidth = std::max(mwidth, mw);
		nwidth = std::max(nwidth, nw);
	}
	{
		int mw(0), nw(0);
		m_navmapdrawingarea.get_preferred_width_for_height(height, mw, nw);
		mwidth += mw;
		nwidth += nw;
	}
	minimum_width = mwidth;
	natural_width = nwidth;
	if (false)
		std::cerr << "NavArea::get_preferred_width_for_height_vfunc height " << height << " mwidth " << mwidth << " nwidth " << nwidth << std::endl;
}
#endif



void FlightDeckWindow::NavArea::on_size_allocate(Gtk::Allocation& allocation)
{
	set_allocation(allocation);
	const unsigned int height(allocation.get_height() / 3);
	const unsigned int width(std::min(height, (unsigned int)allocation.get_width() / 2));
	Gtk::Allocation a(allocation);
	a.set_width(width);
	a.set_height(height);
	m_navaidrawingarea.size_allocate(a);
	a.set_y(a.get_y() + allocation.get_height() - height);
	m_navhsidrawingarea.size_allocate(a);
	a.set_height(allocation.get_height() - 2 * height);
	a.set_y(allocation.get_y() + height);
	a.set_width(width / 2);
	m_navinfodrawingarea.size_allocate(a);
	a.set_x(a.get_x() + a.get_width());
	a.set_width(width - a.get_width());
	m_navaltdrawingarea.size_allocate(a);
	a = allocation;
	a.set_width(a.get_width() - width);
	a.set_x(a.get_x() + width);
	m_navmapdrawingarea.size_allocate(a);
}

void FlightDeckWindow::NavArea::on_add(Widget* widget)
{
	if (!widget)
		return;
	throw std::runtime_error("FlightDeckWindow::NavArea::on_add: cannot add widgets");
}

void FlightDeckWindow::NavArea::on_remove(Widget* widget)
{
	if (!widget)
		return;
	throw std::runtime_error("FlightDeckWindow::NavArea::on_remove: cannot remove widgets");
}

GType FlightDeckWindow::NavArea::child_type_vfunc() const
{
	return G_TYPE_NONE;
}

void FlightDeckWindow::NavArea::forall_vfunc(gboolean include_internals, GtkCallback callback, gpointer callback_data)
{
	callback(static_cast<Gtk::Widget&>(m_navaidrawingarea).gobj(), callback_data);
	callback(static_cast<Gtk::Widget&>(m_navmapdrawingarea).gobj(), callback_data);
	callback(static_cast<Gtk::Widget&>(m_navaltdrawingarea).gobj(), callback_data);
	callback(static_cast<Gtk::Widget&>(m_navinfodrawingarea).gobj(), callback_data);
	callback(static_cast<Gtk::Widget&>(m_navhsidrawingarea).gobj(), callback_data);
}

void FlightDeckWindow::NavArea::set_engine(Engine& eng)
{
	m_engine = &eng;
	m_navmapdrawingarea.set_engine(eng);
	m_navhsidrawingarea.set_engine(eng);
}

void FlightDeckWindow::NavArea::set_sensors(Sensors& sensors)
{
	m_connmapaircraft.disconnect();
	m_sensors = &sensors;
	m_navinfodrawingarea.set_sensors(sensors);
	if (!m_sensors)
		return;
	m_navmapdrawingarea.set_route(m_sensors->get_route());
	m_navmapdrawingarea.set_track(&m_sensors->get_track());
	m_navhsidrawingarea.set_route(m_sensors->get_route());
	m_navhsidrawingarea.set_track(&m_sensors->get_track());
	m_connmapaircraft = m_sensors->connect_mapaircraft(sigc::mem_fun(m_navmapdrawingarea, &MapDrawingArea::set_aircraft));

	if (!m_sensors->get_configfile().has_group(cfggroup_mainwindow) ||
	    !m_sensors->get_configfile().has_key(cfggroup_mainwindow, cfgkey_mainwindow_renderers))
		m_sensors->get_configfile().set_integer(cfggroup_mainwindow, cfgkey_mainwindow_renderers, (unsigned int)m_renderers);
	m_renderers = m_sensors->get_configfile().get_integer(cfggroup_mainwindow, cfgkey_mainwindow_renderers);
	if (!m_sensors->get_configfile().has_group(cfggroup_mainwindow) ||
	    !m_sensors->get_configfile().has_key(cfggroup_mainwindow, cfgkey_mainwindow_bitmapmaps))
		m_sensors->get_configfile().set_string_list(cfggroup_mainwindow, cfgkey_mainwindow_bitmapmaps, std::vector<Glib::ustring>());
	if (m_engine) {
		std::vector<Glib::ustring> bmms(m_sensors->get_configfile().get_string_list(cfggroup_mainwindow, cfgkey_mainwindow_bitmapmaps));
		std::map<std::string,unsigned int> bmms1;
		for (unsigned int i = 0, sz = bmms.size(); i < sz; ++i)
			bmms1.insert(std::map<std::string,unsigned int>::value_type(bmms[i], i));
		m_bitmapmaps.clear();
		m_bitmapmaps.resize(bmms.size());
		BitmapMaps& db(m_engine->get_bitmapmaps_db());
		for (unsigned int i = 0, sz = db.size(); i < sz; ++i) {
			BitmapMaps::Map::ptr_t p(db[i]);
			if (!p)
				continue;
			std::map<std::string,unsigned int>::iterator bi(bmms1.find(p->get_name()));
			if (bi == bmms1.end())
				continue;
			m_bitmapmaps[bi->second] = p;
		}
		for (bitmapmaps_t::iterator i(m_bitmapmaps.begin()), e(m_bitmapmaps.end()); i != e;) {
			if ((*i) && (*i)->check_file()) {
				++i;
				continue;
			}
			i = m_bitmapmaps.erase(i);
			e = m_bitmapmaps.end();
		}
	}
	if (!m_sensors->get_configfile().has_group(cfggroup_mainwindow) ||
	    !m_sensors->get_configfile().has_key(cfggroup_mainwindow, cfgkey_mainwindow_mapflags))
		m_sensors->get_configfile().set_integer(cfggroup_mainwindow, cfgkey_mainwindow_mapflags, (unsigned int)m_mapdrawflags);
	m_mapdrawflags = (MapRenderer::DrawFlags)m_sensors->get_configfile().get_integer(cfggroup_mainwindow, cfgkey_mainwindow_mapflags);
	if (!m_sensors->get_configfile().has_group(cfggroup_mainwindow) ||
	    !m_sensors->get_configfile().has_key(cfggroup_mainwindow, cfgkey_mainwindow_mapscale))
		m_sensors->get_configfile().set_double(cfggroup_mainwindow, cfgkey_mainwindow_mapscale, m_navmapdrawingarea.get_map_scale());
	m_navmapdrawingarea.set_map_scale(m_sensors->get_configfile().get_double(cfggroup_mainwindow, cfgkey_mainwindow_mapscale));
	if (!m_sensors->get_configfile().has_group(cfggroup_mainwindow) ||
	    !m_sensors->get_configfile().has_key(cfggroup_mainwindow, cfgkey_mainwindow_terrainflags))
		m_sensors->get_configfile().set_integer(cfggroup_mainwindow, cfgkey_mainwindow_terrainflags, (unsigned int)(MapRenderer::DrawFlags)m_navhsidrawingarea);
	m_navhsidrawingarea = (MapRenderer::DrawFlags)m_sensors->get_configfile().get_integer(cfggroup_mainwindow, cfgkey_mainwindow_terrainflags);
	if (!m_sensors->get_configfile().has_group(cfggroup_mainwindow) ||
	    !m_sensors->get_configfile().has_key(cfggroup_mainwindow, cfgkey_mainwindow_terrainmapscale))
		m_sensors->get_configfile().set_double(cfggroup_mainwindow, cfgkey_mainwindow_terrainmapscale, m_navhsidrawingarea.get_map_scale());
	m_navhsidrawingarea.set_map_scale(m_sensors->get_configfile().get_double(cfggroup_mainwindow, cfgkey_mainwindow_terrainmapscale));
	if (!m_sensors->get_configfile().has_group(cfggroup_mainwindow) ||
	    !m_sensors->get_configfile().has_key(cfggroup_mainwindow, cfgkey_mainwindow_mapdeclutter))
		m_sensors->get_configfile().set_integer(cfggroup_mainwindow, cfgkey_mainwindow_mapdeclutter, (unsigned int)m_mapdeclutter);
	m_mapdeclutter = (dcltr_t)m_sensors->get_configfile().get_integer(cfggroup_mainwindow, cfgkey_mainwindow_mapdeclutter);
	if (m_mapdeclutter >= dcltr_number)
		m_mapdeclutter = dcltr_normal;
	m_navmapdrawingarea = map_get_declutter_drawflags();
	if (!m_sensors->get_configfile().has_group(cfggroup_mainwindow) ||
	    !m_sensors->get_configfile().has_key(cfggroup_mainwindow, cfgkey_mainwindow_mapup))
		m_sensors->get_configfile().set_integer(cfggroup_mainwindow, cfgkey_mainwindow_mapup, (unsigned int)m_mapup);
	m_mapup = (mapup_t)m_sensors->get_configfile().get_integer(cfggroup_mainwindow, cfgkey_mainwindow_mapup);
	if (m_mapup >= mapup_number)
		m_mapup = mapup_north;
	if (!m_sensors->get_configfile().has_group(cfggroup_mainwindow) ||
	    !m_sensors->get_configfile().has_key(cfggroup_mainwindow, cfgkey_mainwindow_hsiwind))
		m_sensors->get_configfile().set_integer(cfggroup_mainwindow, cfgkey_mainwindow_hsiwind, (unsigned int)m_hsiwind);
	m_hsiwind = !!m_sensors->get_configfile().get_integer(cfggroup_mainwindow, cfgkey_mainwindow_hsiwind);
	if (!m_sensors->get_configfile().has_group(cfggroup_mainwindow) ||
	    !m_sensors->get_configfile().has_key(cfggroup_mainwindow, cfgkey_mainwindow_hsiterrain))
		m_sensors->get_configfile().set_integer(cfggroup_mainwindow, cfgkey_mainwindow_hsiterrain, (unsigned int)m_hsiterrain);
	m_hsiterrain = !!m_sensors->get_configfile().get_integer(cfggroup_mainwindow, cfgkey_mainwindow_hsiterrain);
	if (!m_sensors->get_configfile().has_group(cfggroup_mainwindow) ||
	    !m_sensors->get_configfile().has_key(cfggroup_mainwindow, cfgkey_mainwindow_hsiglide))
		m_sensors->get_configfile().set_integer(cfggroup_mainwindow, cfgkey_mainwindow_hsiglide, (unsigned int)m_hsiglide);
	m_hsiglide = !!m_sensors->get_configfile().get_integer(cfggroup_mainwindow, cfgkey_mainwindow_hsiglide);
	if (!m_sensors->get_configfile().has_group(cfggroup_mainwindow) ||
	    !m_sensors->get_configfile().has_key(cfggroup_mainwindow, cfgkey_mainwindow_altinhg))
		m_sensors->get_configfile().set_integer(cfggroup_mainwindow, cfgkey_mainwindow_altinhg, 0);
        m_navaltdrawingarea.set_inhg(!!m_sensors->get_configfile().get_integer(cfggroup_mainwindow, cfgkey_mainwindow_altinhg));
	if (!m_sensors->get_configfile().has_group(cfggroup_mainwindow) ||
	    !m_sensors->get_configfile().has_key(cfggroup_mainwindow, cfgkey_mainwindow_altmetric))
		m_sensors->get_configfile().set_integer(cfggroup_mainwindow, cfgkey_mainwindow_altmetric, 0);
	m_navaltdrawingarea.set_metric(!!m_sensors->get_configfile().get_integer(cfggroup_mainwindow, cfgkey_mainwindow_altmetric));
	if (!m_sensors->get_configfile().has_group(cfggroup_mainwindow) ||
	    !m_sensors->get_configfile().has_key(cfggroup_mainwindow, cfgkey_mainwindow_altlabels))
		m_sensors->get_configfile().set_integer(cfggroup_mainwindow, cfgkey_mainwindow_altlabels, (unsigned int)m_altlabels);
        m_altlabels = !!m_sensors->get_configfile().get_integer(cfggroup_mainwindow, cfgkey_mainwindow_altlabels);
	if (!m_sensors->get_configfile().has_group(cfggroup_mainwindow) ||
	    !m_sensors->get_configfile().has_key(cfggroup_mainwindow, cfgkey_mainwindow_altbug))
		m_sensors->get_configfile().set_double(cfggroup_mainwindow, cfgkey_mainwindow_altbug, std::numeric_limits<double>::quiet_NaN());
	m_navaltdrawingarea.set_altbug(m_sensors->get_configfile().get_double(cfggroup_mainwindow, cfgkey_mainwindow_altbug));
	if (!m_sensors->get_configfile().has_group(cfggroup_mainwindow) ||
	    !m_sensors->get_configfile().has_key(cfggroup_mainwindow, cfgkey_mainwindow_altminimum))
		m_sensors->get_configfile().set_double(cfggroup_mainwindow, cfgkey_mainwindow_altminimum, std::numeric_limits<double>::quiet_NaN());
	m_navaltdrawingarea.set_minimum(m_sensors->get_configfile().get_double(cfggroup_mainwindow, cfgkey_mainwindow_altminimum));
	sensors_change(m_sensors, Sensors::change_all);
	alt_set_qnh(m_sensors->get_altimeter_qnh());
	alt_set_std(m_sensors->is_altimeter_std());
	// FIXME: should be TAS, not IAS
	hsi_set_glidetas(m_sensors->get_aircraft().get_vbestglide());
	if (m_hsiglide)
		m_navhsidrawingarea.set_glideslope(m_sensors->get_aircraft().get_glideslope());
	else
		m_navhsidrawingarea.set_glideslope();
}

void FlightDeckWindow::NavArea::alt_set_altbug(double altbug)
{
	m_navaltdrawingarea.set_altbug(altbug);
	if (m_sensors) {
		m_sensors->get_configfile().set_double(cfggroup_mainwindow, cfgkey_mainwindow_altbug, altbug);
		m_sensors->save_config();
	}
}

void FlightDeckWindow::NavArea::alt_set_minimum(double minimum)
{
	m_navaltdrawingarea.set_minimum(minimum);
	if (m_sensors) {
		m_sensors->get_configfile().set_double(cfggroup_mainwindow, cfgkey_mainwindow_altminimum, minimum);
		m_sensors->save_config();
	}
}

void FlightDeckWindow::NavArea::alt_set_qnh(double qnh)
{
	m_navaltdrawingarea.set_qnh(qnh);
}

void FlightDeckWindow::NavArea::alt_set_std(bool std)
{
	m_navaltdrawingarea.set_std(std);
}

void FlightDeckWindow::NavArea::alt_set_inhg(bool inhg)
{
	m_navaltdrawingarea.set_inhg(inhg);
	if (m_sensors) {
		m_sensors->get_configfile().set_integer(cfggroup_mainwindow, cfgkey_mainwindow_altinhg, (unsigned int)inhg);
		m_sensors->save_config();
	}
}

void FlightDeckWindow::NavArea::alt_set_metric(bool metric)
{
	m_navaltdrawingarea.set_metric(metric);
	if (m_sensors) {
		m_sensors->get_configfile().set_integer(cfggroup_mainwindow, cfgkey_mainwindow_altmetric, (unsigned int)metric);
		m_sensors->save_config();
	}
}

void FlightDeckWindow::NavArea::alt_set_labels(bool labels)
{
	m_altlabels = labels;
	if (m_sensors) {
		m_sensors->get_configfile().set_integer(cfggroup_mainwindow, cfgkey_mainwindow_altlabels, (unsigned int)m_altlabels);
		m_sensors->save_config();
	}
}

FlightDeckWindow::NavArea::maprenderer_t FlightDeckWindow::NavArea::map_get_renderer(void) const
{
	maprenderer_t r(m_navmapdrawingarea.get_renderer(), ~0U);
	BitmapMaps::Map::ptr_t m(m_navmapdrawingarea.get_bitmap());
	for (unsigned int i = 0, sz = m_bitmapmaps.size(); i < sz; ++i) {
		if (m_bitmapmaps[i] != m)
			continue;
		r.second = i;
		break;
	}
	return r;
}

void FlightDeckWindow::NavArea::map_set_renderer(maprenderer_t renderer)
{
	bool samerenderer(m_navmapdrawingarea.get_renderer() == renderer.first);
	if (!samerenderer)
		m_navmapdrawingarea.set_renderer(renderer.first);
	if (m_navmapdrawingarea.get_renderer() == MapDrawingArea::renderer_bitmap && renderer.second < m_bitmapmaps.size()) {
		BitmapMaps::Map::ptr_t m(m_bitmapmaps[renderer.second]);
		bool samebitmap(m_navmapdrawingarea.get_bitmap() == m);
		m_navmapdrawingarea.set_bitmap(m);
		if (!samebitmap)
			m_navmapdrawingarea.set_map_scale(m->get_nmi_per_pixel());
		if (samerenderer)
			return;
	} else {
		m_navmapdrawingarea.set_bitmap();
		if (samerenderer)
			return;
		if (m_sensors &&
		    m_sensors->get_configfile().has_group(cfggroup_mainwindow)) {
			bool airportdiag(m_navmapdrawingarea.get_renderer() == MapDrawingArea::renderer_airportdiagram);
			if (m_sensors->get_configfile().has_key(cfggroup_mainwindow, cfgkey_mainwindow_mapscales[airportdiag]))
				m_navmapdrawingarea.set_map_scale(m_sensors->get_configfile().get_double(cfggroup_mainwindow, cfgkey_mainwindow_mapscales[airportdiag]));
		}
	}
	m_navmapdrawingarea.queue_draw();
}

FlightDeckWindow::NavArea::vmarenderer_t FlightDeckWindow::NavArea::vma_first_renderer(void)
{
	maprenderer_t mr(map_first_renderer());
	return vmarenderer_t((VectorMapArea::renderer_t)mr.first, mr.second);
}

FlightDeckWindow::NavArea::vmarenderer_t FlightDeckWindow::NavArea::vma_next_renderer(vmarenderer_t rstart)
{
	maprenderer_t mr((MapDrawingArea::renderer_t)rstart.first, rstart.second);
	mr = map_next_renderer(mr);
	return vmarenderer_t((VectorMapArea::renderer_t)mr.first, mr.second);
}

FlightDeckWindow::NavArea::maprenderer_t FlightDeckWindow::NavArea::map_first_renderer(void)
{
	for (MapDrawingArea::renderer_t r = MapDrawingArea::renderer_vmap; r < MapDrawingArea::renderer_none; r = (MapDrawingArea::renderer_t)(r + 1)) {
		if (r == MapDrawingArea::renderer_bitmap)
			continue;
		if (!(m_renderers & (1 << r)))
			continue;
		if (!MapDrawingArea::is_renderer_enabled(r))
			continue;
		return maprenderer_t(r, 0);
	}
	if (m_bitmapmaps.empty())
		return maprenderer_t(MapDrawingArea::renderer_vmap, 0);
	return maprenderer_t(MapDrawingArea::renderer_bitmap, 0);
}

FlightDeckWindow::NavArea::maprenderer_t FlightDeckWindow::NavArea::map_next_renderer(maprenderer_t rstart)
{
	if (rstart.first >= MapDrawingArea::renderer_none || rstart.first < MapDrawingArea::renderer_vmap)
		rstart = maprenderer_t((MapDrawingArea::renderer_t)(MapDrawingArea::renderer_none - 1), 0);
	maprenderer_t r(rstart);
	if (rstart.first == MapDrawingArea::renderer_bitmap)
		rstart.first = (MapDrawingArea::renderer_t)(r.first - 1);
	do {
		if (r.first == MapDrawingArea::renderer_bitmap) {
			++r.second;
			if (r.second < m_bitmapmaps.size())
				return r;
			r.first = MapDrawingArea::renderer_vmap;
			r.second = 0;
		} else {
			r.first = (MapDrawingArea::renderer_t)(r.first + 1);
			if (r.first == MapDrawingArea::renderer_bitmap)
				r.first = (MapDrawingArea::renderer_t)(r.first + 1);
			r.second = 0;
			if (r.first >= MapDrawingArea::renderer_none) {
				if (!m_bitmapmaps.empty())				
					return maprenderer_t(MapDrawingArea::renderer_bitmap, 0);
				r.first = MapDrawingArea::renderer_vmap;
			}
		}
		if (!(m_renderers & (1U << r.first)))
			continue;
		if (!MapDrawingArea::is_renderer_enabled(r.first))
			continue;
		return r;
	} while (r.first != rstart.first);
	return maprenderer_t(MapDrawingArea::renderer_vmap, 0);
}

void FlightDeckWindow::NavArea::map_set_first_renderer(void)
{
	map_set_renderer(map_first_renderer());
}

void FlightDeckWindow::NavArea::map_set_next_renderer(void)
{
	map_set_renderer(map_next_renderer(map_get_renderer()));
}

void FlightDeckWindow::NavArea::map_set_renderers(uint32_t rnd)
{
	if (m_renderers == rnd)
		return;
	m_renderers = rnd;
	if (m_sensors) {
		m_sensors->get_configfile().set_integer(cfggroup_mainwindow, cfgkey_mainwindow_renderers, (unsigned int)m_renderers);
		m_sensors->save_config();
	}
}

void FlightDeckWindow::NavArea::map_set_bitmapmaps(const bitmapmaps_t& bmm)
{
	bool same(bmm.size() == m_bitmapmaps.size());
	if (same) {
		for (unsigned int i = 0, sz = m_bitmapmaps.size(); i < sz; ++i) {
			if (bmm[i] == m_bitmapmaps[i])
				continue;
			same = false;
			break;
		}
		if (same)
			return;
	}
	m_bitmapmaps = bmm;
	for (bitmapmaps_t::iterator i(m_bitmapmaps.begin()), e(m_bitmapmaps.end()); i != e;) {
		if ((*i) && (*i)->check_file()) {
			++i;
			continue;
		}
		i = m_bitmapmaps.erase(i);
		e = m_bitmapmaps.end();
	}
	if (!m_sensors)
		return;
	std::vector<Glib::ustring> bmms;
	for (bitmapmaps_t::const_iterator i(m_bitmapmaps.begin()), e(m_bitmapmaps.end()); i != e; ++i)
		if (*i)
			bmms.push_back((*i)->get_name());
	m_sensors->get_configfile().set_string_list(cfggroup_mainwindow, cfgkey_mainwindow_bitmapmaps, bmms);
}
 
void FlightDeckWindow::NavArea::map_zoom_out(void)
{
	m_navmapdrawingarea.zoom_out();
	if (m_sensors) {
		if (m_navmapdrawingarea.get_renderer() != MapDrawingArea::renderer_bitmap) {
			bool airportdiag(m_navmapdrawingarea.get_renderer() == MapDrawingArea::renderer_airportdiagram);
			m_sensors->get_configfile().set_double(cfggroup_mainwindow, cfgkey_mainwindow_mapscales[airportdiag], m_navmapdrawingarea.get_map_scale());
			m_sensors->save_config();
		}
	}
}

void FlightDeckWindow::NavArea::map_zoom_in(void)
{
	m_navmapdrawingarea.zoom_in();
	if (m_sensors) {
		if (m_navmapdrawingarea.get_renderer() != MapDrawingArea::renderer_bitmap) {
			bool airportdiag(m_navmapdrawingarea.get_renderer() == MapDrawingArea::renderer_airportdiagram);
			m_sensors->get_configfile().set_double(cfggroup_mainwindow, cfgkey_mainwindow_mapscales[airportdiag], m_navmapdrawingarea.get_map_scale());
			m_sensors->save_config();
		}
	}
}

void FlightDeckWindow::NavArea::map_set_drawflags(MapRenderer::DrawFlags f)
{
	m_mapdrawflags = f;
	m_navmapdrawingarea = map_get_declutter_drawflags();
	if (m_sensors) {
		m_sensors->get_configfile().set_integer(cfggroup_mainwindow, cfgkey_mainwindow_mapflags, (unsigned int)m_mapdrawflags);
		m_sensors->save_config();
	}
}

void FlightDeckWindow::NavArea::map_set_declutter(dcltr_t d)
{
	if (d >= dcltr_number)
		m_mapdeclutter = dcltr_normal;
	else
		m_mapdeclutter = d;
	m_navmapdrawingarea = map_get_declutter_drawflags();
	if (m_sensors) {
		m_sensors->get_configfile().set_integer(cfggroup_mainwindow, cfgkey_mainwindow_mapdeclutter, (unsigned int)m_mapdeclutter);
		m_sensors->save_config();
	}
}

void FlightDeckWindow::NavArea::map_next_declutter(void)
{
        map_set_declutter((dcltr_t)(m_mapdeclutter + 1));
}

MapRenderer::DrawFlags FlightDeckWindow::NavArea::map_get_declutter_drawflags(void) const
{
	return m_mapdrawflags & declutter_mask[m_mapdeclutter];
}

void FlightDeckWindow::NavArea::map_set_mapup(mapup_t up)
{
	if (up >= mapup_number)
		m_mapup = mapup_north;
	else
		m_mapup = up;
	if (m_sensors) {
		m_sensors->get_configfile().set_integer(cfggroup_mainwindow, cfgkey_mainwindow_mapup, (unsigned int)m_mapup);
		m_sensors->save_config();
		sensors_change(m_sensors, Sensors::change_all);
	}
}

void FlightDeckWindow::NavArea::map_set_pan(bool pan)
{
	m_mappan = pan;
	m_navmapdrawingarea.set_dragbutton(m_mappan ? 1 : 0);
	if (m_mappan)
		m_navmapdrawingarea.add_events(Gdk::BUTTON_MOTION_MASK | Gdk::BUTTON_PRESS_MASK | Gdk::BUTTON_RELEASE_MASK);
	else
		m_navmapdrawingarea.set_events(m_navmapdrawingarea.get_events() & ~(Gdk::BUTTON_MOTION_MASK | Gdk::BUTTON_PRESS_MASK | Gdk::BUTTON_RELEASE_MASK));
}

void FlightDeckWindow::NavArea::hsi_zoom_out(void)
{
	m_navhsidrawingarea.zoom_out();
	if (m_sensors) {
		m_sensors->get_configfile().set_double(cfggroup_mainwindow, cfgkey_mainwindow_terrainmapscale, m_navhsidrawingarea.get_map_scale());
		m_sensors->save_config();
	}
}

void FlightDeckWindow::NavArea::hsi_zoom_in(void)
{
	m_navhsidrawingarea.zoom_in();
	if (m_sensors) {
		m_sensors->get_configfile().set_double(cfggroup_mainwindow, cfgkey_mainwindow_terrainmapscale, m_navhsidrawingarea.get_map_scale());
		m_sensors->save_config();
	}
}

void FlightDeckWindow::NavArea::hsi_set_drawflags(MapRenderer::DrawFlags f)
{
	m_navhsidrawingarea = f;
	if (m_sensors) {
		m_sensors->get_configfile().set_integer(cfggroup_mainwindow, cfgkey_mainwindow_terrainflags, (unsigned int)(MapRenderer::DrawFlags)m_navhsidrawingarea);
		m_sensors->save_config();
	}
}

void FlightDeckWindow::NavArea::hsi_set_wind(bool v)
{
	if (m_hsiwind == v)
		return;
	m_hsiwind = v;
	if (m_sensors) {
		m_sensors->get_configfile().set_integer(cfggroup_mainwindow, cfgkey_mainwindow_hsiwind, (unsigned int)m_hsiwind);
		m_sensors->save_config();
	}
	if (!m_hsiwind)
		m_navhsidrawingarea.set_wind_heading(std::numeric_limits<double>::quiet_NaN(), (Sensors::mapangle_t)0);
}

void FlightDeckWindow::NavArea::hsi_set_terrain(bool v)
{
	if (m_hsiterrain == v)
		return;
	m_hsiterrain = v;
	if (m_sensors) {
		m_sensors->get_configfile().set_integer(cfggroup_mainwindow, cfgkey_mainwindow_hsiterrain, (unsigned int)m_hsiterrain);
		m_sensors->save_config();
	}
	if (!m_hsiterrain)
		m_navhsidrawingarea.set_center(m_navhsidrawingarea.get_center());
}

void FlightDeckWindow::NavArea::hsi_set_glide(bool v)
{
	if (m_hsiglide == v)
		return;
	m_hsiglide = v;
	if (m_sensors) {
		m_sensors->get_configfile().set_integer(cfggroup_mainwindow, cfgkey_mainwindow_hsiglide, (unsigned int)m_hsiglide);
		m_sensors->save_config();
		if (m_hsiglide)
			m_navhsidrawingarea.set_glideslope(m_sensors->get_aircraft().get_glideslope());
	}
	if (!m_hsiglide)
		m_navhsidrawingarea.set_glideslope();
}

void FlightDeckWindow::NavArea::hsi_set_glidewind(double dir, double speed)
{
	m_navhsidrawingarea.set_glidewind(dir, speed);
}

void FlightDeckWindow::NavArea::hsi_set_glidetas(double tas)
{
	m_navhsidrawingarea.set_glidetas(tas);
}
