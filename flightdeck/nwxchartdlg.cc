//
// C++ Implementation: FlightDeckWindow
//
// Description: Flight Deck Windows, NWX Chart Selector Dialog
//
//
// Author: Thomas Sailer <t.sailer@alumni.ethz.ch>, (C) 2011, 2012, 2013, 2016
//
// Copyright: See COPYING file that comes with this distribution
//
//

#include "sysdeps.h"
#include "sysdepsgui.h"
#include <iomanip>

#include "flightdeckwindow.h"

FlightDeckWindow::MetarTafModelColumns::MetarTafModelColumns(void)
{
	add(m_icao);
	add(m_type);
	add(m_issue);
	add(m_message);
	add(m_weight);
	add(m_style);
}

FlightDeckWindow::WxChartListModelColumns::WxChartListModelColumns(void)
{
	add(m_departure);
	add(m_destination);
	add(m_description);
	add(m_createtime);
	add(m_epoch);
	add(m_level);
	add(m_id);
	add(m_nepoch);
	add(m_nlevel);
	add(m_ntype);
}

void FlightDeckWindow::metartaf_update(void)
{
	if (!m_metartafstore)
		return;
	m_metartafstore->clear();
	if (!m_sensors)
		return;
	const FPlanRoute& fpl(m_sensors->get_route());
	if (fpl.get_nrwpt() <= 0)
		return;
	time_t curtime;
	time(&curtime);
	WXDatabase::metartafresult_t mt(m_wxdb.find_metartaf(fpl.get_bbox().oversize_nmi(50), curtime - 24*60*60, curtime - 2*24*60*60, 1000));
	for (unsigned int wnr = 0; wnr < fpl.get_nrwpt(); ++wnr) {
		const FPlanWaypoint& wpt(fpl[wnr]);
		std::string nrststn;
		{
			uint64_t dist(std::numeric_limits<uint64_t>::max());
			for (WXDatabase::metartafresult_t::const_iterator mi(mt.begin()), me(mt.end()); mi != me; ++mi) {
				const WXDatabase::MetarTaf& m(*mi);
			        uint64_t d(m.get_coord().simple_distance_rel(wpt.get_coord()));
				if (d >= dist)
					continue;
				dist = d;
				nrststn = m.get_icao();
			}
		}
		if (nrststn.empty())
			continue;
		{
			unsigned int nrtaf(0), nrmetar(0);
			for (WXDatabase::metartafresult_t::const_iterator mi(mt.begin()), me(mt.end()); mi != me;) {
				const WXDatabase::MetarTaf& m(*mi);
				WXDatabase::metartafresult_t::iterator mi2(mi);
				++mi;
				if (m.get_icao() != nrststn)
					continue;
				bool add(false);
				if (m.is_metar()) {
					add = nrmetar < 3;
					if (add)
						++nrmetar;
				} else if (m.is_taf()) {
					add = nrtaf < 1;
					if (add)
						++nrtaf;
				}
				if (add) {
					Gtk::TreeModel::iterator iter(m_metartafstore->append());
					Gtk::TreeModel::Row row(*iter);
					row[m_metartafcolumns.m_icao] = m.get_icao();
					{
						char buf[64];
						struct tm utm;
						time_t e(m.get_epoch());
						strftime(buf, sizeof(buf), "%m-%d %H:%M", gmtime_r(&e, &utm));
						row[m_metartafcolumns.m_issue] = buf;
					}
					{
						std::string msg(m.get_message());
						WXDatabase::MetarTaf::remove_linefeeds(msg);
						row[m_metartafcolumns.m_message] = msg;
					}
					Gdk::Color col;
					switch (m.get_type()) {
					case NWXWeather::MetarTaf::type_metar:
						col.set_rgb(0, 0, 0);
						row[m_metartafcolumns.m_type] = "METAR";
						row[m_metartafcolumns.m_weight] = 400;
						row[m_metartafcolumns.m_style] = Pango::STYLE_ITALIC;
						break;

					case NWXWeather::MetarTaf::type_taf:
						col.set_rgb(0, 0, 0x8000);
						row[m_metartafcolumns.m_type] = "TAF";
						row[m_metartafcolumns.m_weight] = 400;
						row[m_metartafcolumns.m_style] = Pango::STYLE_NORMAL;
						break;

					default:
						col.set_rgb(0xffff, 0, 0);
						row[m_metartafcolumns.m_type] = "?";
						row[m_metartafcolumns.m_weight] = 400;
						row[m_metartafcolumns.m_style] = Pango::STYLE_NORMAL;
						break;
					}
					row[m_metartafcolumns.m_color] = col;
				}
				mt.erase(mi2);
			}
		}
	}
}

void FlightDeckWindow::metartaf_fetch(void)
{
	if (!m_sensors)
		return;
	if (false) {
		m_fplnwxweather.metar_taf(m_sensors->get_route(), sigc::mem_fun(*this, &FlightDeckWindow::metartaf_fetch_cb), 0, 3, 0, 2);
	}
	if (true) {
		Rect bbox(m_sensors->get_route().get_bbox().oversize_nmi(100.f));
		m_fpladdsweather.metar_taf(bbox, sigc::mem_fun(*this, &FlightDeckWindow::metartaf_adds_fetch_cb), 3, 12);
	}
}

void FlightDeckWindow::metartaf_fetch_cb(const NWXWeather::metartaf_t& metartaf)
{
	if (true) {
		m_wxdb.save(metartaf.begin(), metartaf.end());
	} else {
		for (NWXWeather::metartaf_t::const_iterator mi(metartaf.begin()), me(metartaf.end()); mi != me; ++mi)
			m_wxdb.save(*mi);
	}
	metartaf_update();
}

void FlightDeckWindow::metartaf_adds_fetch_cb(const ADDS::metartaf_t& metartaf)
{
	if (true)
		std::cerr << "ADDS METAR/TAF: saving " << metartaf.size() << " reports" << std::endl;
	if (true) {
		m_wxdb.save(metartaf.begin(), metartaf.end());
	} else {
		for (ADDS::metartaf_t::const_iterator mi(metartaf.begin()), me(metartaf.end()); mi != me; ++mi) 
			m_wxdb.save(*mi);
	}
	metartaf_update();
}

void FlightDeckWindow::wxchartlist_update(uint64_t selid)
{
	if (!m_wxchartliststore)
		return;
	if (!selid && m_wxchartlisttreeview) {
		Glib::RefPtr<Gtk::TreeSelection> selection(m_wxchartlisttreeview->get_selection());
		if (selection) {
			Gtk::TreeModel::iterator iter(selection->get_selected());
			if (iter) {
				Gtk::TreeModel::Row row(*iter);
				selid = row[m_wxchartlistcolumns.m_id];
			}
		}
	}
	Gtk::TreeModel::iterator seliter;
	m_wxchartliststore->clear();
	time_t curtime;
	time(&curtime);
	WXDatabase::chartresult_t charts(m_wxdb.find_chart(curtime - 90*24*60*60, 500));
	while (!charts.empty()) {
		const WXDatabase::Chart& ch(*charts.begin());
		Gtk::TreeModel::iterator iter(m_wxchartliststore->append());
		Gtk::TreeModel::Row row(*iter);
	        row[m_wxchartlistcolumns.m_departure] = ch.get_departure();
		row[m_wxchartlistcolumns.m_destination] = ch.get_destination();
		row[m_wxchartlistcolumns.m_description] = ch.get_description();
		{
			time_t t(ch.get_createtime());
			char buf[64];
			struct tm utm;
			strftime(buf, sizeof(buf), "%Y-%m-%d %H:%M:%Sz", gmtime_r(&t, &utm));
			row[m_wxchartlistcolumns.m_createtime] = buf;
		}
		{
			time_t t(ch.get_epoch());
			char buf[64];
			struct tm utm;
			strftime(buf, sizeof(buf), "%Y-%m-%d %H:%M:%Sz", gmtime_r(&t, &utm));
			row[m_wxchartlistcolumns.m_epoch] = buf;
		}
		{
			std::ostringstream oss;
			unsigned int level(ch.get_level());
			if (level != ~0U) {
				oss << level << "hPA";
				if (level) {
					float alt;
					IcaoAtmosphere<float>::std_pressure_to_altitude(&alt, 0, level);
					oss << "; FL" << Point::round<int,float>(alt * (Point::m_to_ft * 0.01f));
				}
			}
			row[m_wxchartlistcolumns.m_level] = oss.str();
		}
		row[m_wxchartlistcolumns.m_id] = ch.get_id();
		row[m_wxchartlistcolumns.m_nepoch] = ch.get_epoch();
		row[m_wxchartlistcolumns.m_nlevel] = ch.get_level();
		row[m_wxchartlistcolumns.m_ntype] = ch.get_type();
		if (ch.get_id() == selid)
			seliter = iter;
		charts.erase(charts.begin());
	}
	if (seliter && m_wxchartlisttreeview) {
		Glib::RefPtr<Gtk::TreeSelection> selection(m_wxchartlisttreeview->get_selection());
		if (selection)
			selection->select(seliter);
	}
}

bool FlightDeckWindow::wxchartlist_next(void)
{
	if (!m_wxchartlisttreeview)
		return false;
	Glib::RefPtr<Gtk::TreeSelection> selection(m_wxchartlisttreeview->get_selection());
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

bool FlightDeckWindow::wxchartlist_prev(void)
{
	if (!m_wxchartlisttreeview)
		return false;
	Glib::RefPtr<Gtk::TreeSelection> selection(m_wxchartlisttreeview->get_selection());
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
			if (!m_wxchartliststore)
				return false;
			iter1 = m_wxchartliststore->children().begin();
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

bool FlightDeckWindow::wxchartlist_nextepoch(void)
{
	if (!m_wxchartlisttreeview)
		return false;
	Glib::RefPtr<Gtk::TreeSelection> selection(m_wxchartlisttreeview->get_selection());
	if (!selection)
		return false;
	Gtk::TreeModel::iterator iter(selection->get_selected());
	if (!iter)
		return false;
	Gtk::TreeModel::Row row(*iter);
	Gtk::TreeModel::iterator iter2(iter);
	for (;;) {
		++iter2;
		if (!iter2)
			return false;
		Gtk::TreeModel::Row row2(*iter2);
		if (row[m_wxchartlistcolumns.m_departure] != row2[m_wxchartlistcolumns.m_departure] ||
		    row[m_wxchartlistcolumns.m_destination] != row2[m_wxchartlistcolumns.m_destination] ||
		    row[m_wxchartlistcolumns.m_nepoch] == row2[m_wxchartlistcolumns.m_nepoch] ||
		    row[m_wxchartlistcolumns.m_nlevel] != row2[m_wxchartlistcolumns.m_nlevel] ||
		    row[m_wxchartlistcolumns.m_ntype] != row2[m_wxchartlistcolumns.m_ntype])
			continue;
		break;
	}
	selection->select(iter2);
	return true;
}

bool FlightDeckWindow::wxchartlist_prevepoch(void)
{
	if (!m_wxchartlisttreeview)
		return false;
	Glib::RefPtr<Gtk::TreeSelection> selection(m_wxchartlisttreeview->get_selection());
	if (!selection)
		return false;
	Gtk::TreeModel::iterator iter(selection->get_selected());
	if (!iter)
		return false;
	Gtk::TreeModel::Row row(*iter);
	// make sure we do not iterate past the beginning
	Gtk::TreeIter iter1(row.parent());
	if (!iter1) {
		if (!m_wxchartliststore)
			return false;
		iter1 = m_wxchartliststore->children().begin();
	} else {
		row = *iter1;
		iter1 = row.children().begin();
	}
	Gtk::TreeModel::iterator iter2(iter);
	for (;;) {
		if (iter2 == iter1)
			return false;
		--iter2;
		if (!iter2)
			return false;
		Gtk::TreeModel::Row row2(*iter2);
		if (row[m_wxchartlistcolumns.m_departure] != row2[m_wxchartlistcolumns.m_departure] ||
		    row[m_wxchartlistcolumns.m_destination] != row2[m_wxchartlistcolumns.m_destination] ||
		    row[m_wxchartlistcolumns.m_nepoch] == row2[m_wxchartlistcolumns.m_nepoch] ||
		    row[m_wxchartlistcolumns.m_nlevel] != row2[m_wxchartlistcolumns.m_nlevel] ||
		    row[m_wxchartlistcolumns.m_ntype] != row2[m_wxchartlistcolumns.m_ntype])
			continue;
		break;
	}
	selection->select(iter2);
	return true;
}

bool FlightDeckWindow::wxchartlist_nextcreatetime(void)
{
	if (!m_wxchartlisttreeview)
		return false;
	Glib::RefPtr<Gtk::TreeSelection> selection(m_wxchartlisttreeview->get_selection());
	if (!selection)
		return false;
	Gtk::TreeModel::iterator iter(selection->get_selected());
	if (!iter)
		return false;
	Gtk::TreeModel::Row row(*iter);
	Gtk::TreeModel::iterator iter2(iter);
	for (;;) {
		++iter2;
		if (!iter2)
			return false;
		Gtk::TreeModel::Row row2(*iter2);
		if (row[m_wxchartlistcolumns.m_departure] != row2[m_wxchartlistcolumns.m_departure] ||
		    row[m_wxchartlistcolumns.m_destination] != row2[m_wxchartlistcolumns.m_destination] ||
		    row[m_wxchartlistcolumns.m_nepoch] != row2[m_wxchartlistcolumns.m_nepoch] ||
		    row[m_wxchartlistcolumns.m_nlevel] != row2[m_wxchartlistcolumns.m_nlevel] ||
		    row[m_wxchartlistcolumns.m_ntype] != row2[m_wxchartlistcolumns.m_ntype])
			continue;
		break;
	}
	selection->select(iter2);
	return true;
}

bool FlightDeckWindow::wxchartlist_prevcreatetime(void)
{
	if (!m_wxchartlisttreeview)
		return false;
	Glib::RefPtr<Gtk::TreeSelection> selection(m_wxchartlisttreeview->get_selection());
	if (!selection)
		return false;
	Gtk::TreeModel::iterator iter(selection->get_selected());
	if (!iter)
		return false;
	Gtk::TreeModel::Row row(*iter);
	// make sure we do not iterate past the beginning
	Gtk::TreeIter iter1(row.parent());
	if (!iter1) {
		if (!m_wxchartliststore)
			return false;
		iter1 = m_wxchartliststore->children().begin();
	} else {
		row = *iter1;
		iter1 = row.children().begin();
	}
	Gtk::TreeModel::iterator iter2(iter);
	for (;;) {
		if (iter2 == iter1)
			return false;
		--iter2;
		if (!iter2)
			return false;
		Gtk::TreeModel::Row row2(*iter2);
		if (row[m_wxchartlistcolumns.m_departure] != row2[m_wxchartlistcolumns.m_departure] ||
		    row[m_wxchartlistcolumns.m_destination] != row2[m_wxchartlistcolumns.m_destination] ||
		    row[m_wxchartlistcolumns.m_nepoch] != row2[m_wxchartlistcolumns.m_nepoch] ||
		    row[m_wxchartlistcolumns.m_nlevel] != row2[m_wxchartlistcolumns.m_nlevel] ||
		    row[m_wxchartlistcolumns.m_ntype] != row2[m_wxchartlistcolumns.m_ntype])
			continue;
		break;
	}
	selection->select(iter2);
	return true;
}

bool FlightDeckWindow::wxchartlist_nextlevel(void)
{
	if (!m_wxchartlisttreeview)
		return false;
	Glib::RefPtr<Gtk::TreeSelection> selection(m_wxchartlisttreeview->get_selection());
	if (!selection)
		return false;
	Gtk::TreeModel::iterator iter(selection->get_selected());
	if (!iter)
		return false;
	Gtk::TreeModel::Row row(*iter);
	Gtk::TreeModel::iterator iter2(iter);
	for (;;) {
		++iter2;
		if (!iter2)
			return false;
		Gtk::TreeModel::Row row2(*iter2);
		if (row[m_wxchartlistcolumns.m_departure] != row2[m_wxchartlistcolumns.m_departure] ||
		    row[m_wxchartlistcolumns.m_destination] != row2[m_wxchartlistcolumns.m_destination] ||
		    row[m_wxchartlistcolumns.m_nepoch] != row2[m_wxchartlistcolumns.m_nepoch] ||
		    row[m_wxchartlistcolumns.m_nlevel] == row2[m_wxchartlistcolumns.m_nlevel] ||
		    row[m_wxchartlistcolumns.m_ntype] != row2[m_wxchartlistcolumns.m_ntype])
			continue;
		break;
	}
	selection->select(iter2);
	return true;
}

bool FlightDeckWindow::wxchartlist_prevlevel(void)
{
	if (!m_wxchartlisttreeview)
		return false;
	Glib::RefPtr<Gtk::TreeSelection> selection(m_wxchartlisttreeview->get_selection());
	if (!selection)
		return false;
	Gtk::TreeModel::iterator iter(selection->get_selected());
	if (!iter)
		return false;
	Gtk::TreeModel::Row row(*iter);
	// make sure we do not iterate past the beginning
	Gtk::TreeIter iter1(row.parent());
	if (!iter1) {
		if (!m_wxchartliststore)
			return false;
		iter1 = m_wxchartliststore->children().begin();
	} else {
		row = *iter1;
		iter1 = row.children().begin();
	}
	Gtk::TreeModel::iterator iter2(iter);
	for (;;) {
		if (iter2 == iter1)
			return false;
		--iter2;
		if (!iter2)
			return false;
		Gtk::TreeModel::Row row2(*iter2);
		if (row[m_wxchartlistcolumns.m_departure] != row2[m_wxchartlistcolumns.m_departure] ||
		    row[m_wxchartlistcolumns.m_destination] != row2[m_wxchartlistcolumns.m_destination] ||
		    row[m_wxchartlistcolumns.m_nepoch] != row2[m_wxchartlistcolumns.m_nepoch] ||
		    row[m_wxchartlistcolumns.m_nlevel] == row2[m_wxchartlistcolumns.m_nlevel] ||
		    row[m_wxchartlistcolumns.m_ntype] != row2[m_wxchartlistcolumns.m_ntype])
			continue;
		break;
	}
	selection->select(iter2);
	return true;
}

bool FlightDeckWindow::wxchartlist_displaysel(void)
{
	if (!m_wxchartlisttreeview || !m_wxpicimage)
		return false;
	Glib::RefPtr<Gtk::TreeSelection> selection(m_wxchartlisttreeview->get_selection());
	if (!selection)
		return false;
	Gtk::TreeModel::iterator iter(selection->get_selected());
	if (!iter)
		return false;
	Gtk::TreeModel::Row row(*iter);
	WXDatabase::chartvector_t chv(m_wxdb.loadid_chart(row[m_wxchartlistcolumns.m_id], "ID", 1));
	if (chv.empty())
		return false;
	const WXDatabase::Chart& ch(*chv.begin());
      	if (ch.get_data().empty())
		return false;
	m_wxpicimage->set(ch.get_data(), ch.get_mimetype());
	set_menu_id(2002);
	if (m_wxpiclabel) {
		std::ostringstream oss;
		oss << "<b>Chart ";
		time_t epoch(ch.get_epoch());
		if (epoch) {
			char buf[64];
			struct tm utm;
			strftime(buf, sizeof(buf), "%Y-%m-%d %H:%M:%Sz", gmtime_r(&epoch, &utm));
			oss << ": " << buf;
		}
		unsigned int level(ch.get_level());
		if (level != ~0U)
			oss << ' ' << level << "hPa";
		oss << "</b>";
		m_wxpiclabel->set_label(oss.str());
	}
	return true;
}

void FlightDeckWindow::wxchartlist_addchart(const WXDatabase::Chart& ch)
{
	uint64_t selid(0);
	{
		WXDatabase::Chart ch1(ch);
		m_wxdb.save(ch1);
		selid = ch1.get_id();
	}
	wxchartlist_update(selid);
}

void FlightDeckWindow::wxchartlist_opendlg(void)
{
	if (!m_nwxchartdialog)
		return;
	time_t selepoch(NWXChartDialog::unselepoch);
	unsigned int sellevel(NWXChartDialog::unsellevel);
	unsigned int selcharttype(0);
	if (m_wxchartlisttreeview) {
		Glib::RefPtr<Gtk::TreeSelection> selection(m_wxchartlisttreeview->get_selection());
		if (selection) {
			Gtk::TreeModel::iterator iter(selection->get_selected());
			if (iter) {
				Gtk::TreeModel::Row row(*iter);
				selepoch = row[m_wxchartlistcolumns.m_nepoch];
				sellevel = row[m_wxchartlistcolumns.m_nlevel];
				selcharttype = row[m_wxchartlistcolumns.m_ntype];
			}
		}
	}
	Gtk::Allocation alloc;
	if (!m_wxpicalignment) {
		alloc.set_width(960);
		alloc.set_height(720);
	} else {
		int pg(m_mainnotebook->get_current_page());
		set_mainnotebook_page(11);
		alloc = m_wxpicalignment->get_allocation();
		set_mainnotebook_page(pg);
		if (alloc.get_width() < 64)
			alloc.set_width(960);
		if (alloc.get_height() < 48)
			alloc.set_height(720);
	}
	m_nwxchartdialog->open(m_sensors, sigc::mem_fun(*this, &FlightDeckWindow::wxchartlist_addchart),
			       alloc.get_width(), alloc.get_height(), selepoch, sellevel, selcharttype);
}

void FlightDeckWindow::wxchartlist_refetch(void)
{
	if (!m_wxchartlisttreeview || !m_nwxchartdialog)
		return;
	Glib::RefPtr<Gtk::TreeSelection> selection(m_wxchartlisttreeview->get_selection());
	if (!selection)
		return;
	Gtk::TreeModel::iterator iter(selection->get_selected());
	if (!iter)
		return;
	Gtk::TreeModel::Row row(*iter);
	WXDatabase::Chart ch((std::string)(Glib::ustring)row[m_wxchartlistcolumns.m_departure],
			     (std::string)(Glib::ustring)row[m_wxchartlistcolumns.m_destination],
			     (std::string)(Glib::ustring)row[m_wxchartlistcolumns.m_description],
			     0, row[m_wxchartlistcolumns.m_nepoch], row[m_wxchartlistcolumns.m_nlevel], row[m_wxchartlistcolumns.m_ntype]);
	m_nwxchartdialog->fetch(ch);
}

FlightDeckWindow::NWXChartDialog::EpochModelColumns::EpochModelColumns(void)
{
	add(m_name);
	add(m_epoch);
}

FlightDeckWindow::NWXChartDialog::LevelModelColumns::LevelModelColumns(void)
{
	add(m_name);
	add(m_level);
	add(m_charttype);
}

FlightDeckWindow::NWXChartDialog::LevelChartType::LevelChartType(unsigned int level, charttype_t ct)
	: m_level(level), m_charttype(ct)
{
}

bool FlightDeckWindow::NWXChartDialog::LevelChartType::operator<(const LevelChartType& lc) const
{
	if (get_level() < lc.get_level())
		return true;
	if (get_level() > lc.get_level())
		return false;
	return get_charttype() < lc.get_charttype();
}

const time_t FlightDeckWindow::NWXChartDialog::unselepoch;
const unsigned int FlightDeckWindow::NWXChartDialog::unsellevel;

FlightDeckWindow::NWXChartDialog::NWXChartDialog(BaseObjectType *castitem, const Glib::RefPtr<Builder>& refxml)
	: Gtk::Window(castitem), m_nwxchartbuttonclose(0), m_nwxchartbuttonfetch(0), m_nwxcharttreeviewepochs(0), m_nwxcharttreeviewlevels(0),
	  m_nwxchartframechartsettings(0), m_nwxchartframepgsettings(0), m_nwxchartcomboboxpreciptype(0), m_nwxchartcheckbuttonlatlongrid(0),
	  m_nwxchartcheckbuttoncoastlines(0), m_nwxchartcheckbuttonwindbarb(0), m_nwxchartcheckbuttonfrzlevel(0),
	  m_nwxchartcheckbuttonqff(0), m_nwxchartcomboboxgeopotential(0), m_nwxchartcomboboxtemp(0),
	  m_nwxchartcomboboxrelhumidity(0), m_nwxchartcheckbuttoncape(0), m_nwxchartcheckbuttonaccumprecip(0),
	  m_nwxchartcomboboxtcc(0), m_nwxchartcheckbuttondvsi(0), m_nwxchartcomboboxcloudtop(0),
	  m_nwxchartcomboboxclouddensity(0), m_nwxchartcheckbuttonpgterrain(0), m_nwxchartcheckbuttonpgflightpath(0),
	  m_nwxchartcheckbuttonpgweather(0), m_nwxchartcheckbuttonpgsvg(0), m_sensors(0), m_width(1024), m_height(768)
{
	get_widget(refxml, "nwxchartbuttonclose", m_nwxchartbuttonclose);
	get_widget(refxml, "nwxchartbuttonfetch", m_nwxchartbuttonfetch);
	get_widget(refxml, "nwxcharttreeviewepochs", m_nwxcharttreeviewepochs);
	get_widget(refxml, "nwxcharttreeviewlevels", m_nwxcharttreeviewlevels);
	get_widget(refxml, "nwxchartframechartsettings", m_nwxchartframechartsettings);
	get_widget(refxml, "nwxchartframepgsettings", m_nwxchartframepgsettings);
	get_widget(refxml, "nwxchartcomboboxpreciptype", m_nwxchartcomboboxpreciptype);
	get_widget(refxml, "nwxchartcheckbuttonlatlongrid", m_nwxchartcheckbuttonlatlongrid);
	get_widget(refxml, "nwxchartcheckbuttoncoastlines", m_nwxchartcheckbuttoncoastlines);
	get_widget(refxml, "nwxchartcheckbuttonwindbarb", m_nwxchartcheckbuttonwindbarb);
	get_widget(refxml, "nwxchartcheckbuttonfrzlevel", m_nwxchartcheckbuttonfrzlevel);
	get_widget(refxml, "nwxchartcheckbuttonqff", m_nwxchartcheckbuttonqff);
	get_widget(refxml, "nwxchartcomboboxgeopotential", m_nwxchartcomboboxgeopotential);
	get_widget(refxml, "nwxchartcomboboxtemp", m_nwxchartcomboboxtemp);
	get_widget(refxml, "nwxchartcomboboxrelhumidity", m_nwxchartcomboboxrelhumidity);
	get_widget(refxml, "nwxchartcheckbuttoncape", m_nwxchartcheckbuttoncape);
	get_widget(refxml, "nwxchartcheckbuttonaccumprecip", m_nwxchartcheckbuttonaccumprecip);
	get_widget(refxml, "nwxchartcomboboxtcc", m_nwxchartcomboboxtcc);
	get_widget(refxml, "nwxchartcheckbuttondvsi", m_nwxchartcheckbuttondvsi);
	get_widget(refxml, "nwxchartcomboboxcloudtop", m_nwxchartcomboboxcloudtop);
	get_widget(refxml, "nwxchartcomboboxclouddensity", m_nwxchartcomboboxclouddensity);
	get_widget(refxml, "nwxchartcheckbuttonpgterrain", m_nwxchartcheckbuttonpgterrain);
	get_widget(refxml, "nwxchartcheckbuttonpgflightpath", m_nwxchartcheckbuttonpgflightpath);
	get_widget(refxml, "nwxchartcheckbuttonpgweather", m_nwxchartcheckbuttonpgweather);
	get_widget(refxml, "nwxchartcheckbuttonpgsvg", m_nwxchartcheckbuttonpgsvg);
        if (m_nwxchartbuttonclose)
                m_nwxchartbuttonclose->signal_clicked().connect(sigc::mem_fun(*this, &NWXChartDialog::close_clicked));
        if (m_nwxchartbuttonfetch)
                m_nwxchartbuttonfetch->signal_clicked().connect(sigc::mem_fun(*this, &NWXChartDialog::fetch_clicked));
        m_epochstore = Gtk::ListStore::create(m_epochcolumns);
	if (m_nwxcharttreeviewepochs) {
		m_nwxcharttreeviewepochs->set_model(m_epochstore);
		Gtk::CellRendererText *name_renderer = Gtk::manage(new Gtk::CellRendererText());
                int name_col = m_nwxcharttreeviewepochs->append_column(_("Name"), *name_renderer) - 1;
                Gtk::TreeViewColumn *name_column = m_nwxcharttreeviewepochs->get_column(name_col);
                if (name_column)
                        name_column->add_attribute(*name_renderer, "text", m_epochcolumns.m_name);
                const Gtk::TreeModelColumnBase *cols[] = { &m_epochcolumns.m_name };
                for (unsigned int i = 0; i < sizeof(cols)/sizeof(cols[0]); ++i) {
                        Gtk::TreeViewColumn *col = m_nwxcharttreeviewepochs->get_column(i);
                        if (!col)
                                continue;
                        col->set_resizable(true);
                        //col->set_sort_column(*cols[i]);
                        col->set_expand(i == 1);
                }
                m_nwxcharttreeviewepochs->columns_autosize();
                //m_nwxcharttreeviewepochs->set_headers_clickable();
                Glib::RefPtr<Gtk::TreeSelection> selection = m_nwxcharttreeviewepochs->get_selection();
                selection->set_mode(Gtk::SELECTION_MULTIPLE);
                selection->signal_changed().connect(sigc::mem_fun(*this, &NWXChartDialog::selection_changed));
	}
        m_levelstore = Gtk::TreeStore::create(m_levelcolumns);
	if (m_nwxcharttreeviewlevels) {
		m_nwxcharttreeviewlevels->set_model(m_levelstore);
		Gtk::CellRendererText *name_renderer = Gtk::manage(new Gtk::CellRendererText());
                int name_col = m_nwxcharttreeviewlevels->append_column(_("Name"), *name_renderer) - 1;
                Gtk::TreeViewColumn *name_column = m_nwxcharttreeviewlevels->get_column(name_col);
                if (name_column)
                        name_column->add_attribute(*name_renderer, "text", m_levelcolumns.m_name);
                const Gtk::TreeModelColumnBase *cols[] = { &m_levelcolumns.m_name };
                for (unsigned int i = 0; i < sizeof(cols)/sizeof(cols[0]); ++i) {
                        Gtk::TreeViewColumn *col = m_nwxcharttreeviewlevels->get_column(i);
                        if (!col)
                                continue;
                        col->set_resizable(true);
                        //col->set_sort_column(*cols[i]);
                        col->set_expand(i == 1);
                }
                m_nwxcharttreeviewlevels->columns_autosize();
                //m_nwxcharttreeviewlevels->set_headers_clickable();
                Glib::RefPtr<Gtk::TreeSelection> selection = m_nwxcharttreeviewlevels->get_selection();
                selection->set_mode(Gtk::SELECTION_MULTIPLE);
                selection->signal_changed().connect(sigc::mem_fun(*this, &NWXChartDialog::selection_changed));
                selection->set_select_function(sigc::mem_fun(*this, &NWXChartDialog::level_select_function));
	}
}

FlightDeckWindow::NWXChartDialog::~NWXChartDialog()
{
}

void FlightDeckWindow::NWXChartDialog::open(Sensors *sensors, const sigc::slot<void,const WXDatabase::Chart&>& cb,
					    unsigned int width, unsigned int height,
					    time_t selepoch, unsigned int sellevel, unsigned int charttype)
{
	m_sensors = sensors;
	m_cb = cb;
	m_width = width;
	m_height = height;
	if (selepoch != unselepoch) {
		m_lastselepoch.clear();
		m_lastselepoch.insert(selepoch);
	}
	if (sellevel != unsellevel) {
		m_lastsellevel.clear();
		m_lastsellevel.insert(LevelChartType(sellevel, (charttype_t)charttype));
	}
	if (!m_sensors) {
		hide();
		return;
	}
	m_nwxweather.epochs_and_levels(sigc::mem_fun(*this, &NWXChartDialog::epochs_and_levels_cb));
}

void FlightDeckWindow::NWXChartDialog::fetch(const WXDatabase::Chart& ch)
{
	bool start(m_fetchqueue.empty());
	m_fetchqueue.push_back(ch);
	if (!start)
		return;
	fetch_queuehead();
}

void FlightDeckWindow::NWXChartDialog::close_clicked(void)
{
	hide();
	m_lastselepoch = m_selepoch;
	m_lastsellevel = m_sellevel;
}

void FlightDeckWindow::NWXChartDialog::fetch_clicked(void)
{
	close_clicked();
	if (!m_sensors)
		return;
	const FPlanRoute& fpl(m_sensors->get_route());
	std::string dep, dest;
	if (fpl.get_nrwpt() > 0) {
		dep = fpl[0].get_icao();
		dest = fpl[fpl.get_nrwpt() - 1].get_icao();
	}
	for (epochselection_t::const_iterator ei(m_selepoch.begin()), ee(m_selepoch.end()); ei != ee; ++ei) {
		time_t epoch(*ei);
		for (levelselection_t::const_iterator li(m_sellevel.begin()), le(m_sellevel.end()); li != le; ++li) {
			const LevelChartType& lct(*li);
			std::string desc;
			bool nolevel(false);
			for (const chartsnolevel_t *ct(charttypes_nolevel); ct && ct->chart_name; ++ct) {
				if (ct->chart_type != lct.get_charttype())
					continue;
				desc = ct->chart_name;
				nolevel = true;
				break;
			}
			if (!nolevel && lct.get_level() != ~0U) {
				for (const charts_t *ct(charttypes_level); ct && ct->chart_name; ++ct) {
					if (ct->chart_type != lct.get_charttype())
						continue;
					desc = ct->chart_name;
					break;
				}
			}
			WXDatabase::Chart ch(dep, dest, desc, 0, epoch, nolevel ? ~0U : lct.get_level(), lct.get_charttype(), "", "");
			fetch(ch);
		}
	}
}

std::string FlightDeckWindow::NWXChartDialog::epoch_to_str(time_t epoch)
{
	char buf[64];
	struct tm utm;
	strftime(buf, sizeof(buf), "%Y-%m-%d %H:%M:%Sz", gmtime_r(&epoch, &utm));
	return buf;
}

std::string FlightDeckWindow::NWXChartDialog::level_to_str(unsigned int level)
{
	std::ostringstream oss;
	oss << level << "hPa";
	if (level) {
		float alt;
		IcaoAtmosphere<float>::std_pressure_to_altitude(&alt, 0, level);
		oss << " (FL" << std::setw(3) << std::setfill('0') << Point::round<int,float>(alt * (Point::m_to_ft * 0.01f)) << ')';
	}
	return oss.str();
}

void FlightDeckWindow::NWXChartDialog::fetch_queuehead(void)
{
	if (m_fetchqueue.empty() || !m_sensors)
		return;
	const FPlanRoute& fpl(m_sensors->get_route());
	const WXDatabase::Chart& ch(m_fetchqueue.front());
	NWXWeather::themes_t themes;
	switch ((charttype_t)ch.get_type()) {
	case charttype_profilegraph:
		themes.push_back("terrain");
		themes.push_back("flightpath");
		themes.push_back("clouds");
		themes.push_back("frzlvl");
		m_nwxweather.profile_graph(fpl, ch.get_epoch(), themes, false, sigc::bind(sigc::mem_fun(*this, &NWXChartDialog::chart_cb), ~0U), m_width, m_height);
		break;

	case charttype_windtempqff:
	case charttype_relativehumidity:
	case charttype_clouddensity:
	case charttype_freezingclouddensity:
	case charttype_freezinglevel:
	case charttype_geopotentialheight:
	case charttype_surfacetemperature:
	case charttype_preciptype:
	case charttype_accumulatedprecip:
	case charttype_totalcloudcoverlow:
	case charttype_totalcloudcovermid:
	case charttype_totalcloudcoverhigh:
	case charttype_cloudtopaltlow:
	case charttype_cloudtopaltmid:
	case charttype_cloudtopalthigh:
	case charttype_cape:
	case charttype_dvsi:
	{
		static const char * const ctthemes[] = {
			"wind|prmslmsl|tmpprs_g", // charttype_windtempqff
			"rhprs",		  // charttype_relativehumidity
			"incloud l",		  // charttype_clouddensity
			"incloud l 1",		  // charttype_freezingclouddensity
			"hgt0c",		  // charttype_freezinglevel
			"hgtprs",		  // charttype_geopotentialheight
			"tmpsfc",		  // charttype_surfacetemperature
			"preciptype",		  // charttype_preciptype
			"apcpsfc",		  // charttype_accumulatedprecip
			"tcc l",		  // charttype_totalcloudcoverlow
			"tcc m",		  // charttype_totalcloudcovermid
			"tcc h",		  // charttype_totalcloudcoverhigh
			"presclt l",		  // charttype_cloudtopaltlow
			"presclt m",		  // charttype_cloudtopaltmid
			"presclt h",		  // charttype_cloudtopalthigh
			"capesfc",		  // charttype_cape
			"dvsi"			  // charttype_dvsi
		};
		{
			std::string title("title " + ch.get_description() + " " + epoch_to_str(ch.get_epoch()));
			if (ch.get_level() != ~0U)
				title += " " + level_to_str(ch.get_level());
			themes.push_back(title);
		}
		themes.push_back("setmpdraw on");
		themes.push_back("setgrid on");
		if (ch.get_type() >= charttype_windtempqff) {
			unsigned int idx(ch.get_type() - charttype_windtempqff);
			if (idx < sizeof(ctthemes)/sizeof(ctthemes[0])) {
				const char *ctt(ctthemes[idx]);
				while (ctt && *ctt) {
					const char *ep(strchr(ctt, '|'));
					if (!ep) {
						themes.push_back(std::string(ctt));
						break;
					}
					themes.push_back(std::string(ctt, ep - ctt));
					ctt = ep + 1;
				}
			}
		}
		m_nwxweather.chart(fpl, fpl.get_bbox().oversize_nmi(50), ch.get_epoch(), ch.get_level(), themes, false, sigc::mem_fun(*this, &NWXChartDialog::chart_cb), m_width, m_height);
		break;
	}

	default:
		if (ch.get_level() == ~0U) {
			if (m_nwxchartcheckbuttonpgterrain && m_nwxchartcheckbuttonpgterrain->get_active())
				themes.push_back("terrain");
			if (m_nwxchartcheckbuttonpgflightpath && m_nwxchartcheckbuttonpgflightpath->get_active())
				themes.push_back("flightpath");
			if (m_nwxchartcheckbuttonpgweather && m_nwxchartcheckbuttonpgweather->get_active()) {
				themes.push_back("clouds");
				themes.push_back("frzlvl");
			}
			
			bool svg(m_nwxchartcheckbuttonpgsvg && m_nwxchartcheckbuttonpgsvg->get_active());
			m_nwxweather.profile_graph(fpl, ch.get_epoch(), themes, svg, sigc::bind(sigc::mem_fun(*this, &NWXChartDialog::chart_cb), ~0U), m_width, m_height);
			break;
		}
		themes.push_back("title " + ch.get_description() + " " + epoch_to_str(ch.get_epoch()) + " " + level_to_str(ch.get_level()));
		if (m_nwxchartcheckbuttoncoastlines && m_nwxchartcheckbuttoncoastlines->get_active())
			themes.push_back("setmpdraw on");
		if (m_nwxchartcheckbuttonlatlongrid && m_nwxchartcheckbuttonlatlongrid->get_active())
			themes.push_back("setgrid on");
		if (m_nwxchartcomboboxpreciptype) {
			switch (m_nwxchartcomboboxpreciptype->get_active_row_number()) {
			default:
				break;

			case 1:
				themes.push_back("preciptype rain");
				break;

			case 2:
				themes.push_back("preciptype freezing rain");
				break;

			case 3:
				themes.push_back("preciptype hail");
				break;

			case 4:
				themes.push_back("preciptype snow");
				break;
			}
		}
		if (m_nwxchartcheckbuttonwindbarb && m_nwxchartcheckbuttonwindbarb->get_active())
			themes.push_back("wind");
		if (m_nwxchartcheckbuttonfrzlevel && m_nwxchartcheckbuttonfrzlevel->get_active())
			themes.push_back("hgt0c");
		if (m_nwxchartcheckbuttonqff && m_nwxchartcheckbuttonqff->get_active())
			themes.push_back("prmslmsl");
		if (m_nwxchartcomboboxgeopotential) {
			switch (m_nwxchartcomboboxgeopotential->get_active_row_number()) {
			default:
				break;

			case 1:
				themes.push_back("hgtprs");
				break;

			case 2:
				themes.push_back("hgtprs rainbow");
				break;
			}
		}
		if (m_nwxchartcomboboxtemp) {
			switch (m_nwxchartcomboboxtemp->get_active_row_number()) {
			default:
				break;

			case 1:
				themes.push_back("tmpsfc");
				break;

			case 2:
				themes.push_back("tmpsfc c 5");
				break;

			case 3:
				themes.push_back("tmpsfc_g");
				break;

			case 4:
				themes.push_back("tmpprs_g");
				break;
			}
		}
		if (m_nwxchartcomboboxrelhumidity) {
			switch (m_nwxchartcomboboxrelhumidity->get_active_row_number()) {
			default:
				break;

			case 1:
				themes.push_back("rhprs");
				break;

			case 2:
				themes.push_back("rhprs c");
				break;
			}
		}
		if (m_nwxchartcheckbuttoncape && m_nwxchartcheckbuttoncape->get_active())
			themes.push_back("capesfc");
		if (m_nwxchartcheckbuttonaccumprecip && m_nwxchartcheckbuttonaccumprecip->get_active())
			themes.push_back("apcpsfc");
		if (m_nwxchartcomboboxtcc) {
			switch (m_nwxchartcomboboxtcc->get_active_row_number()) {
			default:
				break;

			case 1:
				themes.push_back("tcc l");
				break;

			case 2:
				themes.push_back("tcc m");
				break;

			case 3:
				themes.push_back("tcc h");
				break;

			case 4:
				themes.push_back("tccc l");
				break;

			case 5:
				themes.push_back("tccc m");
				break;

			case 6:
				themes.push_back("tccc h");
				break;
			}
		}
		if (m_nwxchartcheckbuttondvsi && m_nwxchartcheckbuttondvsi->get_active())
			themes.push_back("dvsi");
		if (m_nwxchartcomboboxcloudtop) {
			switch (m_nwxchartcomboboxcloudtop->get_active_row_number()) {
			default:
				break;

			case 1:
				themes.push_back("presclt l");
				break;

			case 2:
				themes.push_back("presclt m");
				break;

			case 3:
				themes.push_back("presclt h");
				break;
			}
		}
		if (m_nwxchartcomboboxclouddensity) {
			switch (m_nwxchartcomboboxclouddensity->get_active_row_number()) {
			default:
				break;

			case 1:
				{
					std::ostringstream oss;
					oss << "incloud " << ch.get_level();
					themes.push_back(oss.str());
					break;
				}

			case 2:
				{
					std::ostringstream oss;
					oss << "incloud " << ch.get_level() << " 1";
					themes.push_back(oss.str());
					break;
				}

			case 3:
				{
					std::ostringstream oss;
					float press;
					IcaoAtmosphere<float>::std_altitude_to_pressure(&press, 0, m_sensors->get_route().max_altitude() * Point::ft_to_m);
					oss << "incloud " << Point::round<int,float>(press);
					themes.push_back(oss.str());
					break;
				}

			case 4:
				{
					std::ostringstream oss;
					float press;
					IcaoAtmosphere<float>::std_altitude_to_pressure(&press, 0, m_sensors->get_route().max_altitude() * Point::ft_to_m);
					oss << "incloud " << Point::round<int,float>(press) << " 1";
					themes.push_back(oss.str());
					break;
				}
			}
		}
		m_nwxweather.chart(fpl, fpl.get_bbox().oversize_nmi(50), ch.get_epoch(), ch.get_level(), themes, false, sigc::mem_fun(*this, &NWXChartDialog::chart_cb), m_width, m_height);
		break;
	}
}

void FlightDeckWindow::NWXChartDialog::selection_changed(void)
{
	m_selepoch.clear();
	m_sellevel.clear();
	if (m_nwxcharttreeviewepochs && m_epochstore) {
		Glib::RefPtr<Gtk::TreeSelection> selection(m_nwxcharttreeviewepochs->get_selection());
		if (selection) {
			std::vector<Gtk::TreeModel::Path> rows(selection->get_selected_rows());
			for (std::vector<Gtk::TreeModel::Path>::const_iterator ri(rows.begin()), re(rows.end()); ri != re; ++ri) {
				Gtk::TreeModel::iterator iter(m_epochstore->get_iter(*ri));
				if (!iter)
					continue;
				Gtk::TreeModel::Row row(*iter);
				m_selepoch.insert(row[m_epochcolumns.m_epoch]);
			}
		}
	}
	bool haveexpchart(false), haveexppg(false);
	if (m_nwxcharttreeviewlevels && m_levelstore) {
		Glib::RefPtr<Gtk::TreeSelection> selection(m_nwxcharttreeviewlevels->get_selection());
		if (selection) {
			std::vector<Gtk::TreeModel::Path> rows(selection->get_selected_rows());
			for (std::vector<Gtk::TreeModel::Path>::const_iterator ri(rows.begin()), re(rows.end()); ri != re; ++ri) {
				Gtk::TreeModel::iterator iter(m_levelstore->get_iter(*ri));
				if (!iter)
					continue;
				Gtk::TreeModel::Row row(*iter);
				m_sellevel.insert(LevelChartType(row[m_levelcolumns.m_level], row[m_levelcolumns.m_charttype]));
				if (row[m_levelcolumns.m_charttype] == charttype_expert) {
					if (row[m_levelcolumns.m_level] == ~0U)
						haveexppg = true;
					else
						haveexpchart = true;
				}
			}
		}
	}
	if (m_nwxchartbuttonfetch)
                m_nwxchartbuttonfetch->set_sensitive(!m_selepoch.empty() && !m_sellevel.empty());
	if (m_nwxchartframechartsettings) {
		m_nwxchartframechartsettings->set_visible(haveexpchart || haveexppg);
		m_nwxchartframechartsettings->set_sensitive(haveexpchart);
	}
	if (m_nwxchartframepgsettings) {
		m_nwxchartframepgsettings->set_visible(haveexpchart || haveexppg);
		m_nwxchartframepgsettings->set_sensitive(haveexppg);
	}
}

bool FlightDeckWindow::NWXChartDialog::level_select_function(const Glib::RefPtr<Gtk::TreeModel>& model, const Gtk::TreeModel::Path& path, bool currently_selected)
{
        if (currently_selected)
                return true;
        Gtk::TreeModel::iterator iter(model->get_iter(path));
        if (!iter)
                return false;
        Gtk::TreeModel::Row row(*iter);
	return row.children().begin() == row.children().end();
}

const FlightDeckWindow::NWXChartDialog::chartsnolevel_t FlightDeckWindow::NWXChartDialog::charttypes_nolevel[] = {
	{ "Freezing Level", charttype_freezinglevel, 1000U },
	{ "Geopotential Height", charttype_geopotentialheight, 1000U },
	{ "Surface Temperature", charttype_surfacetemperature, 1000U },
	{ "Precipitation Type", charttype_preciptype, 1000U },
	{ "Accumulated Precipitation", charttype_accumulatedprecip, 1000U },
	{ "Total Cloud Cover Low Level Clouds", charttype_totalcloudcoverlow, 1000U },
	{ "Total Cloud Cover Mid Level Clouds", charttype_totalcloudcovermid, 1000U },
	{ "Total Cloud Cover High Level Clouds", charttype_totalcloudcoverhigh, 1000U },
	{ "Cloud Top Altitude Low Level Clouds", charttype_cloudtopaltlow, 1000U },
	{ "Cloud Top Altitude Mid Level Clouds", charttype_cloudtopaltmid, 1000U },
	{ "Cloud Top Altitude High Level Clouds", charttype_cloudtopalthigh, 1000U },
	{ "Convectionally Available Potential Energy (CAPE)", charttype_cape, 1000U },
	{ "Profile Graph", charttype_profilegraph, ~0U },
	{ "Expert Profile Graph", charttype_expert, ~0U },
	{ 0, charttype_expert, ~0U }
};

const FlightDeckWindow::NWXChartDialog::charts_t FlightDeckWindow::NWXChartDialog::charttypes_level[] = {
	{ "Wind, Temperature, QFF", charttype_windtempqff },
	{ "Relative Humidity", charttype_relativehumidity },
	{ "Cloud Density", charttype_clouddensity },
	{ "Freezing Cloud Density", charttype_freezingclouddensity },
	{ "Deformation Vertical Shear Index (DVSI)", charttype_dvsi },
	{ "Expert Chart", charttype_expert },
	{ 0, charttype_expert }
};

void FlightDeckWindow::NWXChartDialog::epochs_and_levels_cb(const NWXWeather::EpochsLevels& el)
{
	std::vector<Gtk::TreeModel::iterator> selepoch, sellevel;
	m_epochstore->clear();
	for (NWXWeather::EpochsLevels::epoch_const_iterator i(el.epoch_begin()), e(el.epoch_end()); i != e; ++i) {
		if (!*i)
			continue;
		Gtk::TreeModel::iterator iter(m_epochstore->append());
		Gtk::TreeModel::Row row(*iter);
                row[m_epochcolumns.m_epoch] = *i;
                row[m_epochcolumns.m_name] = epoch_to_str(*i);
		if (m_lastselepoch.find(*i) != m_lastselepoch.end())
			selepoch.push_back(iter);
	}
	m_levelstore->clear();
	for (NWXWeather::EpochsLevels::level_const_iterator i(el.level_begin()), e(el.level_end()); i != e; ++i) {
		Gtk::TreeModel::iterator iter(m_levelstore->append());
		Gtk::TreeModel::Row row(*iter);
                row[m_levelcolumns.m_level] = *i;
		row[m_levelcolumns.m_name] = level_to_str(*i);
		row[m_levelcolumns.m_charttype] = charttype_expert;
		const charts_t *ct(charttypes_level);
		if (!*i)
			ct = &charttypes_level[sizeof(charttypes_level)/sizeof(charttypes_level[0])-2];
		for (; ct && ct->chart_name; ++ct) {
			Gtk::TreeModel::iterator ctiter(m_levelstore->append(row.children()));
			Gtk::TreeModel::Row ctrow(*ctiter);
			ctrow[m_levelcolumns.m_level] = *i;
			ctrow[m_levelcolumns.m_name] = ct->chart_name;
			ctrow[m_levelcolumns.m_charttype] = ct->chart_type;
			if (m_lastsellevel.find(LevelChartType(*i, ct->chart_type)) != m_lastsellevel.end())
				sellevel.push_back(ctiter);
		}
	}
	for (const chartsnolevel_t *ct(charttypes_nolevel); ct && ct->chart_name; ++ct) {
		Gtk::TreeModel::iterator iter(m_levelstore->append());
		Gtk::TreeModel::Row row(*iter);
		row[m_levelcolumns.m_level] = ct->chart_level;
		row[m_levelcolumns.m_name] = ct->chart_name;
		row[m_levelcolumns.m_charttype] = ct->chart_type;
		if (m_lastsellevel.find(LevelChartType(ct->chart_level, ct->chart_type)) != m_lastsellevel.end())
			sellevel.push_back(iter);
	}
	if (m_nwxcharttreeviewepochs) {
		Glib::RefPtr<Gtk::TreeSelection> selection(m_nwxcharttreeviewepochs->get_selection());
		if (selection) {
			selection->freeze_notify();
			selection->unselect_all();
			for (std::vector<Gtk::TreeModel::iterator>::const_iterator ii(selepoch.begin()), ie(selepoch.end()); ii != ie; ++ii)
				selection->select(*ii);
			selection->thaw_notify();
		}
	}
	if (m_nwxcharttreeviewlevels) {
		Glib::RefPtr<Gtk::TreeSelection> selection(m_nwxcharttreeviewlevels->get_selection());
		if (selection) {
			selection->freeze_notify();
			selection->unselect_all();
			for (std::vector<Gtk::TreeModel::iterator>::const_iterator ii(sellevel.begin()), ie(sellevel.end()); ii != ie; ++ii)
				selection->select(*ii);
			selection->thaw_notify();
		}
	}
	selection_changed();
	show();
}

void FlightDeckWindow::NWXChartDialog::chart_cb(const std::string& pix, const std::string& mimetype, time_t epoch, unsigned int level)
{
	if (false)
		std::cerr << "nwx: charts: mime " << mimetype << ": \"" << pix << "\"" << std::endl;
	if (false)
		std::cerr << "nwx: charts: mime " << mimetype << std::endl;
	if (m_fetchqueue.empty())
		return;
	if (!pix.empty()) {
		const WXDatabase::Chart& chq(m_fetchqueue.front());
		WXDatabase::Chart ch(chq.get_departure(), chq.get_destination(), chq.get_description(), time(0), chq.get_epoch(), chq.get_level(), chq.get_type(), mimetype, pix);
		m_cb(ch);
	}
	m_fetchqueue.pop_front();
	fetch_queuehead();
}
