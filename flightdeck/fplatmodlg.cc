//
// C++ Implementation: FlightDeckWindow
//
// Description: Flight Deck Windows, Altitude Dialog
//
//
// Author: Thomas Sailer <t.sailer@alumni.ethz.ch>, (C) 2011, 2012, 2013, 2014, 2016
//
// Copyright: See COPYING file that comes with this distribution
//
//

#include "sysdeps.h"
#include "sysdepsgui.h"
#include "icaofpl.h"
#include <iomanip>

#include "flightdeckwindow.h"

void FlightDeckWindow::fplatmosphere_dialog_open(void)
{
	if (!m_fplatmodialog || !m_sensors)
		return;
	m_fplatmodialog->open(m_sensors, perfpage_getwbvalues());
}

const unsigned int FlightDeckWindow::FPLAtmosphereDialog::nrkeybuttons;
const struct FlightDeckWindow::FPLAtmosphereDialog::keybutton FlightDeckWindow::FPLAtmosphereDialog::keybuttons[nrkeybuttons] = {
	{ "fplatmobutton0", GDK_KEY_KP_0, 90, 1 },
	{ "fplatmobutton1", GDK_KEY_KP_1, 87, 1 },
	{ "fplatmobutton2", GDK_KEY_KP_2, 88, 1 },
	{ "fplatmobutton3", GDK_KEY_KP_3, 89, 1 },
	{ "fplatmobutton4", GDK_KEY_KP_4, 83, 1 },
	{ "fplatmobutton5", GDK_KEY_KP_5, 84, 1 },
	{ "fplatmobutton6", GDK_KEY_KP_6, 85, 1 },
	{ "fplatmobutton7", GDK_KEY_KP_7, 79, 1 },
	{ "fplatmobutton8", GDK_KEY_KP_8, 80, 1 },
	{ "fplatmobutton9", GDK_KEY_KP_9, 81, 1 },
	{ "fplatmobuttondot", GDK_KEY_KP_Decimal, 91, 1 }
};

FlightDeckWindow::FPLAtmosphereDialog::FPLAtmosphereDialog(BaseObjectType *castitem, const Glib::RefPtr<Builder>& refxml)
	: Gtk::Window(castitem), m_calendaroffblock(0), m_spinbuttonoffblockh(0), m_spinbuttonoffblockm(0), m_spinbuttonoffblocks(0), 
	  m_spinbuttontaxitimem(0), m_spinbuttontaxitimes(0), m_spinbuttoncruisealt(0), m_spinbuttonwinddir(0), m_spinbuttonwindspeed(0), 
	  m_spinbuttonisaoffs(0), m_spinbuttonqff(0), m_buttontoday(0), m_buttondone(0), m_buttonsetwind(0), m_buttonsetisaoffs(0),
	  m_buttonsetqff(0), m_buttongetnwx(0), m_buttongfs(0), m_buttonskyvector(0), m_buttonsaveplog(0), m_buttonsavewx(0),
	  m_entryflighttime(0), m_entryfuel(0), m_sensors(0), m_gfsminreftime(-1), m_gfsmaxreftime(-1), m_gfsminefftime(-1), m_gfsmaxefftime(-1)
{
#ifndef GLIBMM_DEFAULT_SIGNAL_HANDLERS_ENABLED
        signal_delete_event().connect(sigc::mem_fun(*this, &FPLAtmosphereDialog::on_delete_event));
#endif //GLIBMM_DEFAULT_SIGNAL_HANDLERS_ENABLED
	get_widget(refxml, "fplatmocalendaroffblock", m_calendaroffblock);
	get_widget(refxml, "fplatmospinbuttonoffblockh", m_spinbuttonoffblockh);
	get_widget(refxml, "fplatmospinbuttonoffblockm", m_spinbuttonoffblockm);
	get_widget(refxml, "fplatmospinbuttonoffblocks", m_spinbuttonoffblocks);
	get_widget(refxml, "fplatmospinbuttontaxitimem", m_spinbuttontaxitimem);
	get_widget(refxml, "fplatmospinbuttontaxitimes", m_spinbuttontaxitimes);
	get_widget(refxml, "fplatmospinbuttoncruisealt", m_spinbuttoncruisealt);
	get_widget(refxml, "fplatmospinbuttonwinddir", m_spinbuttonwinddir);
	get_widget(refxml, "fplatmospinbuttonwindspeed", m_spinbuttonwindspeed);
	get_widget(refxml, "fplatmospinbuttonisaoffs", m_spinbuttonisaoffs);
	get_widget(refxml, "fplatmospinbuttonqff", m_spinbuttonqff);
	get_widget(refxml, "fplatmobuttontoday", m_buttontoday);
	get_widget(refxml, "fplatmobuttondone", m_buttondone);
	get_widget(refxml, "fplatmobuttonsetwind", m_buttonsetwind);
	get_widget(refxml, "fplatmobuttonsetisaoffs", m_buttonsetisaoffs);
	get_widget(refxml, "fplatmobuttonsetqff", m_buttonsetqff);
	get_widget(refxml, "fplatmobuttongetnwx", m_buttongetnwx);
	get_widget(refxml, "fplatmobuttongfs", m_buttongfs);
	get_widget(refxml, "fplatmobuttonskyvector", m_buttonskyvector);
	get_widget(refxml, "fplatmobuttonsaveplog", m_buttonsaveplog);
	get_widget(refxml, "fplatmobuttonsavewx", m_buttonsavewx);
	get_widget(refxml, "fplatmoentryflighttime", m_entryflighttime);
	get_widget(refxml, "fplatmoentryfuel", m_entryfuel);

        if (m_calendaroffblock)
                m_connoffblock = m_calendaroffblock->signal_day_selected().connect(sigc::mem_fun(*this, &FPLAtmosphereDialog::deptime_changed));
	if (m_spinbuttonoffblockh)
		m_connoffblockh = m_spinbuttonoffblockh->signal_value_changed().connect(sigc::mem_fun(*this, &FPLAtmosphereDialog::deptime_changed));
	if (m_spinbuttonoffblockm)
		m_connoffblockm = m_spinbuttonoffblockm->signal_value_changed().connect(sigc::mem_fun(*this, &FPLAtmosphereDialog::deptime_changed));
	if (m_spinbuttonoffblocks)
		m_connoffblocks = m_spinbuttonoffblocks->signal_value_changed().connect(sigc::mem_fun(*this, &FPLAtmosphereDialog::deptime_changed));
	if (m_spinbuttontaxitimem)
		m_conntaxitimem = m_spinbuttontaxitimem->signal_value_changed().connect(sigc::mem_fun(*this, &FPLAtmosphereDialog::deptime_changed));
	if (m_spinbuttontaxitimes)
		m_conntaxitimes = m_spinbuttontaxitimes->signal_value_changed().connect(sigc::mem_fun(*this, &FPLAtmosphereDialog::deptime_changed));
	if (m_buttontoday)
		m_buttontoday->signal_clicked().connect(sigc::mem_fun(*this, &FPLAtmosphereDialog::today_clicked));
	if (m_buttondone)
		m_buttondone->signal_clicked().connect(sigc::mem_fun(*this, &FPLAtmosphereDialog::done_clicked));
	if (m_buttonsetwind)
		m_buttonsetwind->signal_clicked().connect(sigc::mem_fun(*this, &FPLAtmosphereDialog::setwind_clicked));
	if (m_buttonsetisaoffs)
		m_buttonsetisaoffs->signal_clicked().connect(sigc::mem_fun(*this, &FPLAtmosphereDialog::setisaoffs_clicked));
	if (m_buttonsetqff)
		m_buttonsetqff->signal_clicked().connect(sigc::mem_fun(*this, &FPLAtmosphereDialog::setqff_clicked));
	if (m_buttongetnwx)
		m_buttongetnwx->signal_clicked().connect(sigc::mem_fun(*this, &FPLAtmosphereDialog::getnwx_clicked));
	if (m_buttongfs)
		m_buttongfs->signal_clicked().connect(sigc::mem_fun(*this, &FPLAtmosphereDialog::gfs_clicked));
	if (m_buttonskyvector)
		m_buttonskyvector->signal_clicked().connect(sigc::mem_fun(*this, &FPLAtmosphereDialog::skyvector_clicked));
	if (m_buttonsaveplog)
		m_buttonsaveplog->signal_clicked().connect(sigc::mem_fun(*this, &FPLAtmosphereDialog::saveplog_clicked));
	if (m_buttonsavewx)
		m_buttonsavewx->signal_clicked().connect(sigc::mem_fun(*this, &FPLAtmosphereDialog::savewx_clicked));
	if (m_spinbuttoncruisealt)
		m_conncruisealt = m_spinbuttoncruisealt->signal_value_changed().connect(sigc::mem_fun(*this, &FPLAtmosphereDialog::cruisealt_changed));
	for (unsigned int i = 0; i < nrkeybuttons; ++i) {
		m_keybutton[i] = 0;
		get_widget(refxml, keybuttons[i].widget_name, m_keybutton[i]);
		if (!m_keybutton[i])
			continue;
		m_connkey[i] = m_keybutton[i]->signal_clicked().connect(sigc::bind(sigc::mem_fun(*this, &FPLAtmosphereDialog::key_clicked), i));
		m_keybutton[i]->set_can_focus(false);
	}
}

FlightDeckWindow::FPLAtmosphereDialog::~FPLAtmosphereDialog()
{
}

void FlightDeckWindow::FPLAtmosphereDialog::open(Sensors *sensors, const Aircraft::WeightBalance::elementvalues_t& wbev)
{
	m_sensors = sensors;
	m_wbev = wbev;
	m_gfsminreftime = m_gfsmaxreftime = m_gfsminefftime = m_gfsmaxefftime = -1;
	if (m_sensors) {
		sensors_change(Sensors::change_all);
		show();
	} else {
		hide();
	}
}

void FlightDeckWindow::FPLAtmosphereDialog::sensors_change(Sensors::change_t changemask)
{
	if (!m_sensors || !changemask)
		return;
	if (changemask & Sensors::change_fplan) {
		m_gfsminreftime = m_gfsmaxreftime = m_gfsminefftime = m_gfsmaxefftime = -1;
		const FPlanRoute& fpl(m_sensors->get_route());
		if (!fpl.get_nrwpt()) {
			m_sensors = 0;
			hide();
		}
		// set times
		m_connoffblock.block();
		m_connoffblockh.block();
		m_connoffblockm.block();
		m_connoffblocks.block();
		m_conntaxitimem.block();
		m_conntaxitimes.block();
		{
			time_t t(fpl.get_time_offblock_unix());
			struct tm utm;
			gmtime_r(&t, &utm);
			if (m_calendaroffblock) {
				m_calendaroffblock->select_month(utm.tm_mon, utm.tm_year + 1900);
				m_calendaroffblock->select_day(utm.tm_mday);
				m_calendaroffblock->mark_day(utm.tm_mday);
			}
			if (m_spinbuttonoffblockh)
				m_spinbuttonoffblockh->set_value(utm.tm_hour);
			if (m_spinbuttonoffblockm)
				m_spinbuttonoffblockm->set_value(utm.tm_min);
			if (m_spinbuttonoffblocks)
				m_spinbuttonoffblocks->set_value(utm.tm_sec);
			t = fpl[0].get_time_unix() - t;
			if (m_spinbuttontaxitimem)
				m_spinbuttontaxitimem->set_value(t / 60U);
			if (m_spinbuttontaxitimes)
				m_spinbuttontaxitimes->set_value(t % 60U);
		}
		m_connoffblock.unblock();
		m_connoffblockh.unblock();
		m_connoffblockm.unblock();
		m_connoffblocks.unblock();
		m_conntaxitimem.unblock();
		m_conntaxitimes.unblock();
		// find altitudes, average atmosphere
		{
			typedef std::set<int> altset_t;
			altset_t altset;
			float windx(0), windy(0), isaoffs(0), qff(0);
			double fuel(0);
			for (unsigned int i = 0; i < fpl.get_nrwpt(); ++i) {
				const FPlanWaypoint& wpt(fpl[i]);
				altset.insert(wpt.get_altitude());
				float x(0), y(0);
				sincosf(wpt.get_winddir_rad(), &y, &x);
				windx += x * wpt.get_windspeed_kts();
				windy += y * wpt.get_windspeed_kts();
				isaoffs += wpt.get_isaoffset_kelvin();
				qff += wpt.get_qff_hpa();
				fuel = wpt.get_fuel_usg();
			}
			{
				float s(1.f / fpl.get_nrwpt());
				windx *= s;
				windy *= s;
				isaoffs *= s;
				qff *= s;
			}
			float windspeed(sqrtf(windx * windx + windy * windy)), winddir(0.f);
			if (windspeed >= 0.001f)
				winddir = atan2f(windy, windx) * (float)(180.f / M_PI);
			m_conncruisealt.block();
			if (m_spinbuttoncruisealt) {
				m_spinbuttoncruisealt->set_sensitive(false);
				altset_t::const_reverse_iterator ai(altset.rbegin()), ae(altset.rend());
				if (ai != ae) {
					int crsalt(*ai++);
					m_spinbuttoncruisealt->set_value(crsalt);
					if (ai != ae) {
						double rmin, rmax;
						m_spinbuttoncruisealt->get_range(rmin, rmax);
						rmin = std::min(crsalt, *ai + 100);
						m_spinbuttoncruisealt->set_range(rmin, rmax);
						m_spinbuttoncruisealt->set_sensitive(true);
					}
				}						
			}
			m_conncruisealt.unblock();
			if (m_spinbuttonwinddir)
				m_spinbuttonwinddir->set_value(winddir);
			if (m_spinbuttonwindspeed)
				m_spinbuttonwindspeed->set_value(windspeed);
			if (m_spinbuttonisaoffs)
				m_spinbuttonisaoffs->set_value(isaoffs);
			if (m_spinbuttonqff)
				m_spinbuttonqff->set_value(qff);
			if (m_entryfuel)
				m_entryfuel->set_text(Glib::ustring::format(std::fixed, std::setprecision(1), fuel));
		}
		// compute total flight time
		if (m_entryflighttime) {
			time_t t(fpl[fpl.get_nrwpt() - 1].get_time_unix() - fpl[0].get_time_unix());
			m_entryflighttime->set_text(Conversions::time_str(t));
		}
	}
}

void FlightDeckWindow::FPLAtmosphereDialog::today_clicked(void)
{
	if (!m_calendaroffblock)
		return;
	Glib::Date d;
	d.set_time_current();
	m_calendaroffblock->select_month(d.get_month() - 1, d.get_year());
	m_calendaroffblock->select_day(d.get_day());
	deptime_changed();
}

void FlightDeckWindow::FPLAtmosphereDialog::done_clicked(void)
{
	hide();
	m_sensors = 0;
}

void FlightDeckWindow::FPLAtmosphereDialog::setwind_clicked(void)
{
	if (!m_sensors)
		return;
	FPlanRoute& fpl(m_sensors->get_route());
	if (!fpl.get_nrwpt())
		return;
	double winddir(0), windspeed(0);
	if (m_spinbuttonwinddir)
		winddir = m_spinbuttonwinddir->get_value();
	if (m_spinbuttonwindspeed)
		windspeed = m_spinbuttonwindspeed->get_value();
	fpl.set_winddir_deg(winddir);
	fpl.set_windspeed_kts(windspeed);
	deptime_changed(); // force recompute of flight plan
}

void FlightDeckWindow::FPLAtmosphereDialog::setisaoffs_clicked(void)
{
	if (!m_sensors)
		return;
	FPlanRoute& fpl(m_sensors->get_route());
	if (!fpl.get_nrwpt())
		return;
	double isaoffs(0);
	if (m_spinbuttonisaoffs)
		isaoffs = m_spinbuttonisaoffs->get_value();
	fpl.set_isaoffset_kelvin(isaoffs);
	deptime_changed(); // force recompute of flight plan
}

void FlightDeckWindow::FPLAtmosphereDialog::setqff_clicked(void)
{
	if (!m_sensors)
		return;
	FPlanRoute& fpl(m_sensors->get_route());
	if (!fpl.get_nrwpt())
		return;
	double qff(IcaoAtmosphere<double>::std_sealevel_pressure);
	if (m_spinbuttonqff)
		qff = m_spinbuttonqff->get_value();
	fpl.set_qff_hpa(qff);
	deptime_changed(); // force recompute of flight plan
}

void FlightDeckWindow::FPLAtmosphereDialog::getnwx_clicked(void)
{
	if (!m_sensors)
		return;
	deptime_changed(); // force recompute of flight plan
	m_sensors->get_route().nwxweather(m_nwxweather, sigc::mem_fun(*this, &FPLAtmosphereDialog::fetchwind_cb));
}

void FlightDeckWindow::FPLAtmosphereDialog::fetchwind_cb(bool fplmodif)
{
	if (fplmodif)
	 	deptime_changed(); // force recompute of flight plan
}

void FlightDeckWindow::FPLAtmosphereDialog::gfs_clicked(void)
{
	if (!m_sensors)
		return;
	FPlanRoute& fpl(m_sensors->get_route());
	deptime_changed(); // force recompute of flight plan
	fpl.set_winddir(0);
	fpl.set_windspeed(0);
	FPlanRoute::GFSResult r(fpl.gfs(m_sensors->get_engine()->get_grib2_db()));
	if (true) {
		std::cerr << "GFS:";
		if (r.is_modified())
			std::cerr << " modified";
		if (r.is_partial())
			std::cerr << " partial";
		if (r.get_minreftime() <= r.get_maxreftime()) {
			std::cerr << " reftime " << Glib::TimeVal(r.get_minreftime(), 0).as_iso8601();
			if (r.get_minreftime() != r.get_maxreftime())
				std::cerr << '-' << Glib::TimeVal(r.get_maxreftime(), 0).as_iso8601();
		}
		if (r.get_minefftime() <= r.get_maxefftime()) {
			std::cerr << " efftime " << Glib::TimeVal(r.get_minefftime(), 0).as_iso8601();
			if (r.get_minefftime() != r.get_maxefftime())
				std::cerr << '-' << Glib::TimeVal(r.get_maxefftime(), 0).as_iso8601();
		}
		std::cerr << std::endl;
	}
	if (r.is_modified())
		deptime_changed(); // force recompute of flight plan
	if (r.is_modified() && !r.is_partial()) {
		m_gfsminreftime = r.get_minreftime();
		m_gfsmaxreftime = r.get_maxreftime();
		m_gfsminefftime = r.get_minefftime();
		m_gfsmaxefftime = r.get_maxefftime();
	}
}

void FlightDeckWindow::FPLAtmosphereDialog::deptime_changed(void)
{
	if (!m_sensors)
		return;
	if (false)
		std::cerr << "flightplan changed" << std::endl;
	m_gfsminreftime = m_gfsmaxreftime = m_gfsminefftime = m_gfsmaxefftime = -1;
	FPlanRoute& fpl(m_sensors->get_route());
	struct tm utm;
	{
		time_t t(fpl.get_time_offblock_unix());
		gmtime_r(&t, &utm);
	}
	if (m_calendaroffblock) {
		guint year, month, day;
		m_calendaroffblock->get_date(year, month, day);
		if (year) {
			utm.tm_year = year - 1900;
			utm.tm_mon = month;
		}
		if (day)
			utm.tm_mday = day;
	}
	if (m_spinbuttonoffblockh)
		utm.tm_hour = m_spinbuttonoffblockh->get_value();
	if (m_spinbuttonoffblockm)
		utm.tm_min = m_spinbuttonoffblockm->get_value();
	if (m_spinbuttonoffblocks)
		utm.tm_sec = m_spinbuttonoffblocks->get_value();
	unsigned int taxis(0U), taxim(5U);
	if (m_spinbuttontaxitimem)
		taxim = m_spinbuttontaxitimem->get_value();
	if (m_spinbuttontaxitimes)
		taxis = m_spinbuttontaxitimes->get_value();
	m_sensors->nav_fplan_setdeparture(timegm(&utm), taxim * 60U + taxis);
}

void FlightDeckWindow::FPLAtmosphereDialog::cruisealt_changed(void)
{
	if (!m_sensors)
		return;
	FPlanRoute& fpl(m_sensors->get_route());
	int curcrsalt(std::numeric_limits<int>::min());
	for (unsigned int i = 0; i < fpl.get_nrwpt(); ++i) {
		FPlanWaypoint& wpt(fpl[i]);
		curcrsalt = std::max(curcrsalt, static_cast<int>(wpt.get_altitude()));
	}
	int newcrsalt(curcrsalt);
	if (m_spinbuttoncruisealt)
		newcrsalt = m_spinbuttoncruisealt->get_value();
	for (unsigned int i = 0; i < fpl.get_nrwpt(); ++i) {
		FPlanWaypoint& wpt(fpl[i]);
		if (wpt.get_altitude() != curcrsalt)
			continue;
		wpt.set_altitude(newcrsalt);
	}
	deptime_changed(); // force recompute of flight plan
}

bool FlightDeckWindow::FPLAtmosphereDialog::on_delete_event(GdkEventAny* event)
{
        hide();
        return true;
}

void FlightDeckWindow::FPLAtmosphereDialog::key_clicked(unsigned int i)
{
 	if (i >= nrkeybuttons)
		return;
	Gtk::Widget *w(get_focus());
	if (!w)
		return;
	for (unsigned int cnt(keybuttons[i].count); cnt > 0; --cnt)
		KeyboardDialog::send_key(w, 0, keybuttons[i].key, keybuttons[i].hwkeycode);
}

void FlightDeckWindow::FPLAtmosphereDialog::skyvector_clicked(void)
{
	if (!m_sensors)
		return;
	const FPlanRoute& fpl(m_sensors->get_route());
	Glib::RefPtr<Gio::AppInfo> appinfo(Gio::AppInfo::get_default_for_uri_scheme("http"));
	if (!appinfo)
		return;
	std::ostringstream uri;
	uri << "http://skyvector.com/";
	if (fpl.get_nrwpt()) {
		uri << "?ll=" << fpl[0].get_coord().get_lat_deg_dbl() << ','
		    << fpl[0].get_coord().get_lon_deg_dbl() << "&plan";
		char delim('=');
		for (unsigned int i(0), n(fpl.get_nrwpt()); i < n; ++i) {
			const FPlanWaypoint& wpt(fpl[i]);
			uri << delim;
			delim = ':';
			if (wpt.get_type() == FPlanWaypoint::type_airport && wpt.get_icao().size() == 4 &&
			    !AirportsDb::Airport::is_fpl_zzzz(wpt.get_icao())) {
				uri << "A." << wpt.get_icao().substr(0, 2) << '.' << wpt.get_icao();
				continue;
			}
			if (!Engine::AirwayGraphResult::Vertex::is_ident_numeric(wpt.get_icao().empty() ? wpt.get_name() : wpt.get_icao())) {
				IcaoFlightPlan::FindCoord fc(*m_sensors->get_engine());
				unsigned int flags(0);
				switch (wpt.get_type()) {
				case FPlanWaypoint::type_airport:
					flags = IcaoFlightPlan::FindCoord::flag_airport;
					break;

				case FPlanWaypoint::type_navaid:
					flags = IcaoFlightPlan::FindCoord::flag_navaid;
					break;

				case FPlanWaypoint::type_intersection:
					flags = IcaoFlightPlan::FindCoord::flag_waypoint;
					break;

				default:
					flags = IcaoFlightPlan::FindCoord::flag_airport |
						IcaoFlightPlan::FindCoord::flag_navaid |
						IcaoFlightPlan::FindCoord::flag_waypoint;
					break;
				}
				fc.find_by_ident(wpt.get_icao().empty() ? wpt.get_name() : wpt.get_icao(), "", flags);
				{
					const Engine::AirportResult::elementvector_t& ev(fc.get_airports());
					Engine::AirportResult::elementvector_t::const_iterator i(ev.begin()), e(ev.end());
					for (; i != e; ++i) {
						if (!i->is_valid() || i->get_coord().is_invalid() || i->get_coord().simple_distance_nmi(wpt.get_coord()) > 1.f)
							continue;
						if (i->get_authorityset().size() != 1)
							continue;
						uri << "A." << *i->get_authorityset().begin() << '.' << i->get_icao();
						break;
					}
					if (i != e)
						continue;
				}
				{
					const Engine::NavaidResult::elementvector_t& ev(fc.get_navaids());
					Engine::NavaidResult::elementvector_t::const_iterator i(ev.begin()), e(ev.end());
					for (; i != e; ++i) {
						if (!i->is_valid() || i->get_coord().is_invalid() || i->get_coord().simple_distance_nmi(wpt.get_coord()) > 1.f)
							continue;
						if (i->get_authorityset().size() != 1)
							continue;
						if (i->get_icao().size() < 2)
							continue;
						bool v(i->has_vor()), n(i->has_ndb());
						if (v + n != 1)
							continue;
						uri << (v ? 'V' : 'N') << '.' << *i->get_authorityset().begin() << '.' << i->get_icao();
						break;
					}
					if (i != e)
						continue;
				}
				{
					const Engine::WaypointResult::elementvector_t& ev(fc.get_waypoints());
					Engine::WaypointResult::elementvector_t::const_iterator i(ev.begin()), e(ev.end());
					for (; i != e; ++i) {
						if (!i->is_valid() || i->get_coord().is_invalid() || i->get_coord().simple_distance_nmi(wpt.get_coord()) > 1.f)
							continue;
						if (i->get_authorityset().size() != 1)
							continue;
						uri << "F." << *i->get_authorityset().begin() << '.' << i->get_name();
						break;
					}
					if (i != e)
						continue;
				}
			}
			uri << "G." << wpt.get_coord().get_lat_deg_dbl() << ',' << wpt.get_coord().get_lon_deg_dbl();
			continue;
		}
	}
	if (true) {
		std::vector<std::string> uris;
		uris.push_back(uri.str());
		appinfo->launch_uris(uris);
	} else {
		Glib::RefPtr<Gio::File> url(Gio::File::create_for_uri(uri.str()));
		appinfo->launch(url);
	}
}

void FlightDeckWindow::FPLAtmosphereDialog::saveplog_clicked(void)
{
	std::string path;
	{
		Gtk::FileChooserDialog dlg(*this, "Save Plog...", Gtk::FILE_CHOOSER_ACTION_SAVE);
		{
			Glib::RefPtr<Gtk::FileFilter> filt(Gtk::FileFilter::create());
			filt->add_pattern("*.xml");
			filt->set_name("Gnumeric Spreadsheet");
			dlg.add_filter(filt);
			filt = Gtk::FileFilter::create();
			filt->add_pattern("*");
			filt->set_name("All Files");
			dlg.add_filter(filt);
		}
		dlg.set_create_folders(true);
		dlg.set_do_overwrite_confirmation(true);
		dlg.set_local_only(true);
		dlg.set_transient_for(*this);
		dlg.add_button(Gtk::Stock::CANCEL, Gtk::RESPONSE_CANCEL);
		dlg.add_button("Save Plog", Gtk::RESPONSE_OK);
		int response(dlg.run());
		switch (response) {
		case Gtk::RESPONSE_CANCEL:
		default:
			return;

		case Gtk::RESPONSE_OK:
			break;
		}
		std::vector<Glib::RefPtr<Gio::File> > files(dlg.get_files());
		if (files.empty())
			return;
		path = files[0]->get_path();
	}
	if (path.empty())
		return;
	if (!m_sensors)
		return;
	m_sensors->get_aircraft().navfplan_gnumeric(path, *(m_sensors->get_engine()),
						    m_sensors->get_route(), std::vector<FPlanAlternate>(),
						    m_sensors->get_acft_cruise_params(),
						    std::numeric_limits<double>::quiet_NaN(),
						    m_wbev, PACKAGE_DATA_DIR "/navlog.xml",
						    m_gfsminreftime, m_gfsmaxreftime,
						    m_gfsminefftime, m_gfsmaxefftime,
						    false, Aircraft::unit_invalid, Aircraft::unit_invalid);
}

void FlightDeckWindow::FPLAtmosphereDialog::savewx_clicked(void)
{
	MeteoProfile::yaxis_t profileymode(MeteoProfile::yaxis_altitude);
	std::string path;
	{
		Gtk::FileChooserDialog dlg(*this, "Save Weather Charts...", Gtk::FILE_CHOOSER_ACTION_SAVE);
		{
			Glib::RefPtr<Gtk::FileFilter> filt(Gtk::FileFilter::create());
			filt->add_pattern("*.pdf");
			filt->set_name("Portable Document Format (PDF)");
			dlg.add_filter(filt);
			filt = Gtk::FileFilter::create();
			filt->add_pattern("*.svg");
			filt->set_name("Scalable Vector Graphics (SVG)");
			dlg.add_filter(filt);
			filt = Gtk::FileFilter::create();
			filt->add_pattern("*");
			filt->set_name("All Files");
			dlg.add_filter(filt);
		}
		dlg.set_create_folders(true);
		dlg.set_do_overwrite_confirmation(true);
		dlg.set_local_only(true);
		dlg.set_transient_for(*this);
		dlg.add_button(Gtk::Stock::CANCEL, Gtk::RESPONSE_CANCEL);
		dlg.add_button("Save WX Charts", Gtk::RESPONSE_OK);
		Gtk::CheckButton chkgrametyaxis("Gramet Pressure on Y Axis", false);
		chkgrametyaxis.show();
		Gtk::VBox vbox;
		vbox.pack_start(chkgrametyaxis, false, true);
		vbox.show();
		dlg.set_extra_widget(vbox);
		int response(dlg.run());
		switch (response) {
		case Gtk::RESPONSE_CANCEL:
		default:
			return;

		case Gtk::RESPONSE_OK:
			break;
		}
		std::vector<Glib::RefPtr<Gio::File> > files(dlg.get_files());
		if (files.empty())
			return;
		path = files[0]->get_path();
		if (chkgrametyaxis.get_active())
			profileymode = MeteoProfile::yaxis_pressure;
	}
	if (path.empty())
		return;
	if (!m_sensors)
		return;
	MeteoProfile profile(m_sensors->get_route());
	MeteoChart chart(m_sensors->get_route(), MeteoChart::alternates_t(), MeteoChart::altroutes_t(), m_sensors->get_engine()->get_grib2_db());
	if (profile.get_route().get_nrwpt() < 2)
		return;
	profile.set_wxprofile(m_sensors->get_engine()->get_grib2_db().get_profile(profile.get_route()));
	{
		TopoDb30 topodb;
		std::string dir_aux(m_sensors->get_engine()->get_dir_aux());
		if (dir_aux.empty())
			dir_aux = m_sensors->get_engine()->get_default_aux_dir();
		topodb.open(dir_aux);
		profile.set_routeprofile(topodb.get_profile(profile.get_route(), 5.));
	}
	double height(595.28), width(841.89);
	Cairo::RefPtr<Cairo::Surface> surface;
	Cairo::RefPtr<Cairo::PdfSurface> pdfsurface;
	if (path.size() > 4 && !path.compare(path.size() - 4, 4, ".svg")) {
		surface = Cairo::SvgSurface::create(path, width, height);
	} else {
		surface = pdfsurface = Cairo::PdfSurface::create(path, width, height);
	}
	Cairo::RefPtr<Cairo::Context> ctx(Cairo::Context::create(surface));
	ctx->translate(20, 20);
	{
		static const double maxprofilepagedist(200);
		double pagedist(profile.get_route().total_distance_nmi_dbl());
		unsigned int nrpages(1);
		if (!std::isnan(maxprofilepagedist) && maxprofilepagedist > 0 && pagedist > maxprofilepagedist) {
			nrpages = std::ceil(pagedist / maxprofilepagedist);
			pagedist /= nrpages;
		}
		double scaledist(profile.get_scaledist(ctx, width - 40, pagedist));
		double scaleelev, originelev;
		if (profileymode == MeteoProfile::yaxis_pressure) {
			scaleelev = profile.get_scaleelev(ctx, height - 40, 60000, profileymode);
			originelev = IcaoAtmosphere<double>::std_sealevel_pressure;
		} else {
			scaleelev = profile.get_scaleelev(ctx, height - 40, profile.get_route().max_altitude() + 4000, profileymode);
			originelev = 0;
		}
		for (unsigned int pgnr(0); pgnr < nrpages; ++pgnr) {
			profile.draw(ctx, width - 40, height - 40, pgnr * pagedist, scaledist, originelev, scaleelev, profileymode);
			ctx->show_page();
			//surface->show_page();
		}
	}
	static const unsigned int nrcharts = (sizeof(GRIB2::WeatherProfilePoint::isobaric_levels) /
					      sizeof(GRIB2::WeatherProfilePoint::isobaric_levels[0]));
	{
		Point center;
		double scalelon(1e-5), scalelat(1e-5);
		chart.get_fullscale(width - 40, height - 40, center, scalelon, scalelat);
		// check whether portrait is the better orientation for this route
		if (pdfsurface) {
			Point center1;
			double scalelon1(1e-5), scalelat1(1e-5);
			chart.get_fullscale(height - 40, width - 40, center1, scalelon1, scalelat1);
			if (false)
				std::cerr << "check aspect ratio: scalelat: landscape " << scalelat << " portrait " << scalelat1 << std::endl;
			if (fabs(scalelat1) > fabs(scalelat)) {
				center = center1;
				scalelon = scalelon1;
				scalelat = scalelat1;
				std::swap(width, height);
				pdfsurface->set_size(width, height);
				ctx = Cairo::Context::create(surface);
				ctx->translate(20, 20);
			}
		}
		for (unsigned int i = 0; i < nrcharts; ++i) {
			chart.draw(ctx, width - 40, height - 40, center, scalelon, scalelat,
				   100.0 * GRIB2::WeatherProfilePoint::isobaric_levels[i]);
			ctx->show_page();
			//surface->show_page();
		}
	}
	surface->finish();
}
