/***************************************************************************
 *   Copyright (C) 2007, 2008, 2009, 2011, 2012 by Thomas Sailer           *
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

#include "flightdeckwindow.h"

const unsigned int FlightDeckWindow::RouteListModelColumns::nowptnr;

FlightDeckWindow::RouteListModelColumns::RouteListModelColumns(void)
{
	add(m_fromicao);
	add(m_fromname);
	add(m_toicao);
	add(m_toname);
	add(m_length);
	add(m_dist);
	add(m_style);
	add(m_id);
	add(m_wptnr);
	add(m_coord);
}

int FlightDeckWindow::fpldirpage_sortfloatcol(const Gtk::TreeModelColumn<float>& col, const Gtk::TreeModel::iterator& ia, const Gtk::TreeModel::iterator& ib)
{
	const Gtk::TreeModel::Row rowa(*ia), rowb(*ib);
	unsigned int wptnra(rowa[m_fpldbcolumns.m_wptnr]);
	unsigned int wptnrb(rowb[m_fpldbcolumns.m_wptnr]);
	if (wptnra != FlightDeckWindow::RouteListModelColumns::nowptnr &&
	    wptnrb != FlightDeckWindow::RouteListModelColumns::nowptnr) {
		if (wptnra < wptnrb)
			return -1;
		if (wptnra > wptnrb)
			return 1;
		return 0;
	}
        return compare_dist(rowa[col], rowb[col]);
}

int FlightDeckWindow::fpldirpage_sortstringcol(const Gtk::TreeModelColumn<Glib::ustring>& col, const Gtk::TreeModel::iterator& ia, const Gtk::TreeModel::iterator& ib)
{
	const Gtk::TreeModel::Row rowa(*ia), rowb(*ib);
	unsigned int wptnra(rowa[m_fpldbcolumns.m_wptnr]);
	unsigned int wptnrb(rowb[m_fpldbcolumns.m_wptnr]);
	if (wptnra != FlightDeckWindow::RouteListModelColumns::nowptnr &&
	    wptnrb != FlightDeckWindow::RouteListModelColumns::nowptnr) {
		if (wptnra < wptnrb)
			return -1;
		if (wptnra > wptnrb)
			return 1;
		return 0;
	}
	const Glib::ustring& flda(rowa[col]);
	const Glib::ustring& fldb(rowb[col]);
        return flda.compare(fldb);
}

void FlightDeckWindow::fpldirpage_lengthcelldatafunc(const Gtk::TreeModelColumn<float>& col, Gtk::CellRenderer *render, const Gtk::TreeModel::iterator& it)
{
	Gtk::CellRendererText *renderer(dynamic_cast<Gtk::CellRendererText *>(render));
	if (!renderer)
		return;
	if (!it) {
		renderer->property_text() = "";
		return;
	}
	const Gtk::TreeModel::Row row(*it);
	float d(row[col]);
	if (std::isnan(d)) {
		renderer->property_text() = "";
		return;
	}
	renderer->property_text() = Conversions::dist_str(d);
}

bool FlightDeckWindow::fpldb_select_function(const Glib::RefPtr<Gtk::TreeModel>& model, const Gtk::TreeModel::Path& path, bool currently_selected)
{
	if (currently_selected)
		return true;
        Gtk::TreeModel::iterator iter(model->get_iter(path));
        if (!iter)
                return false;
        Gtk::TreeModel::Row row(*iter);
	if (row[m_fpldbcolumns.m_wptnr] == RouteListModelColumns::nowptnr)
		return true;
        if (!m_fpldbtreeview)
		return false;
	Glib::RefPtr<Gtk::TreeSelection> selection(m_fpldbtreeview->get_selection());
	if (!selection)
		return false;
	return selection->count_selected_rows() == 0;
}

void FlightDeckWindow::fpldb_selection_changed(void)
{
	bool ena(false), enam(false);
        if (m_fpldbtreeview) {
		Glib::RefPtr<Gtk::TreeSelection> selection(m_fpldbtreeview->get_selection());
		if (selection) {
			std::vector<Gtk::TreePath> selrows(selection->get_selected_rows());
			ena = selrows.size() == 1;
			enam = selrows.size() > 1;
			if (ena && m_fpldbstore) {
				Gtk::TreeModel::iterator iter(m_fpldbstore->get_iter(selrows[0]));
				if (iter) {
					Gtk::TreeModel::Row row(*iter);
					enam = row[m_fpldbcolumns.m_wptnr] == RouteListModelColumns::nowptnr;
					if (m_sensors)
						m_sensors->get_configfile().set_integer(cfggroup_mainwindow, cfgkey_mainwindow_fplid, row[m_fpldbcolumns.m_id]);
				}
			}
		}
	}
	if (m_fpldbloadbutton)
		m_fpldbloadbutton->set_sensitive(ena);
	if (m_fpldbduplicatebutton)
		m_fpldbduplicatebutton->set_sensitive(enam);
	if (m_fpldbdeletebutton)
		m_fpldbdeletebutton->set_sensitive(enam);
	if (m_fpldbreversebutton)
		m_fpldbreversebutton->set_sensitive(enam);
	if (m_fpldbexportbutton)
		m_fpldbexportbutton->set_sensitive(enam);
}

void FlightDeckWindow::fpldirpage_cfmbuttonok(void)
{
	if (m_fpldbcfmwindow)
		m_fpldbcfmwindow->hide();
	if (m_fpldbcfmtreeview)
		m_fpldbcfmtreeview->unset_model();
	if (m_engine) {
		FPlanRoute fpl(m_engine->get_fplan_db());
		Glib::RefPtr<Gtk::TreeSelection> selection;
		if (m_fpldbtreeview) {
			selection = m_fpldbtreeview->get_selection();
			if (selection)
				selection->unselect_all();
		}
		Gtk::TreeIter iter;
		switch (m_fpldbcfmmode) {
		case fpldbcfmmode_delete:
			for (fpldbcfmids_t::const_iterator ii(m_fpldbcfmids.begin()), ie(m_fpldbcfmids.end()); ii != ie; ++ii) {
				fpl.load_fp(*ii);
				fpl.delete_fp();
				fpldirpage_deletefpl(*ii);
			}
			break;

		case fpldbcfmmode_reverse:
			for (fpldbcfmids_t::const_iterator ii(m_fpldbcfmids.begin()), ie(m_fpldbcfmids.end()); ii != ie; ++ii) {
				fpl.load_fp(*ii);
				fpl.reverse_fp();
				fpl.save_fp();
				Gtk::TreeIter it(fpldirpage_updatefpl(fpl));
				if (selection)
					selection->select(it);
				if (!iter)
					iter = it;
			}
			break;

		case fpldbcfmmode_duplicate:
			for (fpldbcfmids_t::const_iterator ii(m_fpldbcfmids.begin()), ie(m_fpldbcfmids.end()); ii != ie; ++ii) {
				fpl.load_fp(*ii);
				fpl.duplicate_fp();
				fpl.save_fp();
				Gtk::TreeIter it(fpldirpage_updatefpl(fpl));
				if (selection)
					selection->select(it);
				if (!iter)
					iter = it;
			}
			break;

		default:
			break;
		}
		if (iter && m_fpldbtreeview)
			m_fpldbtreeview->scroll_to_row(Gtk::TreePath(iter));
	}
	m_fpldbcfmmode = fpldbcfmmode_none;
	m_fpldbcfmids.clear();
}

void FlightDeckWindow::fpldirpage_cfmbuttoncancel(void)
{
	if (m_fpldbcfmwindow)
		m_fpldbcfmwindow->hide();
	if (m_fpldbcfmtreeview)
		m_fpldbcfmtreeview->unset_model();
	m_fpldbcfmmode = fpldbcfmmode_none;
	m_fpldbcfmids.clear();
}

bool FlightDeckWindow::fpldirpage_cfmvisible(const Gtk::TreeModel::const_iterator& iter)
{
	if (!iter)
		return false;
	Gtk::TreeModel::Row row(*iter);
	return (m_fpldbcfmids.find(row[m_fpldbcolumns.m_id]) != m_fpldbcfmids.end());
}

Gtk::TreeIter FlightDeckWindow::fpldirpage_cfmdialog(fpldbcfmmode_t mode)
{
	if (mode == fpldbcfmmode_none || !m_fpldbstore || !m_fpldbtreeview)
		return Gtk::TreeIter();
	Glib::RefPtr<Gtk::TreeSelection> selection(m_fpldbtreeview->get_selection());
	if (!selection)
		return Gtk::TreeIter();
	std::vector<Gtk::TreePath> selrows(selection->get_selected_rows());
	if (selrows.empty())
		return Gtk::TreeIter();
	if (selrows.size() == 1 && mode != fpldbcfmmode_delete)
		return m_fpldbstore->get_iter(selrows.front());
	m_fpldbcfmmode = mode;
	m_fpldbcfmids.clear();
	for (std::vector<Gtk::TreePath>::const_iterator si(selrows.begin()), se(selrows.end()); si != se; ++si) {
		Gtk::TreeIter iter(m_fpldbstore->get_iter(*si));
		if (!iter)
			continue;
		Gtk::TreeModel::Row row(*iter);
		m_fpldbcfmids.insert(row[m_fpldbcolumns.m_id]);
	}
	Glib::RefPtr<Gtk::TreeModelFilter> filter(Gtk::TreeModelFilter::create(m_fpldbstore));
	filter->set_visible_func(sigc::mem_fun(*this, &FlightDeckWindow::fpldirpage_cfmvisible));
	m_fpldbcfmtreeview->set_model(filter);
	m_fpldbcfmtreeview->expand_all();
	switch (mode) {
	case fpldbcfmmode_delete:
		if (m_fpldbcfmwindow)
			m_fpldbcfmwindow->set_title("Confirm Delete Flight Plans");
		if (m_fpldbcfmlabel)
			m_fpldbcfmlabel->set_markup("Confirm you want to <b>delete</b> the following flight plans:");
		break;

	case fpldbcfmmode_reverse:
		if (m_fpldbcfmwindow)
			m_fpldbcfmwindow->set_title("Confirm Reverse Flight Plans");
		if (m_fpldbcfmlabel)
			m_fpldbcfmlabel->set_markup("Confirm you want to <b>reverse</b> the following flight plans:");
		break;

	case fpldbcfmmode_duplicate:
		if (m_fpldbcfmwindow)
			m_fpldbcfmwindow->set_title("Confirm Duplicate Flight Plans");
		if (m_fpldbcfmlabel)
			m_fpldbcfmlabel->set_markup("Confirm you want to <b>duplicate</b> the following flight plans:");
		break;

	default:
		break;
	}
	if (m_fpldbcfmwindow)
		m_fpldbcfmwindow->show();
	return Gtk::TreeIter();
}

void FlightDeckWindow::fpldirpage_buttonload(void)
{
	if (!m_sensors || !m_fpldbtreeview || !m_fpldbstore)
		return;
	Glib::RefPtr<Gtk::TreeSelection> selection(m_fpldbtreeview->get_selection());
	if (!selection)
		return;
	std::vector<Gtk::TreePath> selrows(selection->get_selected_rows());
	if (selrows.size() != 1)
		return;
       	Gtk::TreeIter iter(m_fpldbstore->get_iter(selrows.front()));
	if (!iter)
		return;
	Gtk::TreeModel::Row row(*iter);
	m_sensors->nav_load_fplan(row[m_fpldbcolumns.m_id]);
	set_menu_id(200);
}

void FlightDeckWindow::fpldirpage_buttondirectto(void)
{
	if (!m_sensors || !m_fpldbtreeview || !m_fpldbstore)
		return;
	Glib::RefPtr<Gtk::TreeSelection> selection(m_fpldbtreeview->get_selection());
	if (!selection)
		return;
	std::vector<Gtk::TreePath> selrows(selection->get_selected_rows());
	if (selrows.size() != 1)
		return;
       	Gtk::TreeIter iter(m_fpldbstore->get_iter(selrows.front()));
	if (!iter)
		return;
	Gtk::TreeModel::Row row(*iter);
	m_sensors->nav_load_fplan(row[m_fpldbcolumns.m_id]);
	// start navigation
	FPlanRoute& fpl(m_sensors->get_route());
	unsigned int selwpt(row[m_fpldbcolumns.m_wptnr]);
	if (selwpt == RouteListModelColumns::nowptnr) {
 		unsigned int cur(fpl.get_curwpt());
		unsigned int nr(fpl.get_nrwpt());
		if (!nr) {
			set_menu_id(200);
			return;
		}
		double track(0);
		if (nr >= 2)
			track = fpl[0].get_coord().spheric_true_course(fpl[1].get_coord());
		m_sensors->nav_directto(fpl[0].get_icao_name(), fpl[0].get_coord(), track, fpl[0].get_altitude(), fpl[0].get_flags(), cur);
		set_menu_id(0);
		return;
	}
	if (!m_coorddialog) {
		set_menu_id(200);
		return;
	}
	set_menu_id(0);
	m_coorddialog->set_flags(m_navarea.map_get_drawflags());
	if (m_sensors &&
	    m_sensors->get_configfile().has_key(cfggroup_mainwindow, NavArea::cfgkey_mainwindow_mapscale) &&
	    m_sensors->get_configfile().has_key(cfggroup_mainwindow, NavArea::cfgkey_mainwindow_arptmapscale))
		m_coorddialog->set_map_scale(m_sensors->get_configfile().get_double(cfggroup_mainwindow, NavArea::cfgkey_mainwindow_mapscale),
					     m_sensors->get_configfile().get_double(cfggroup_mainwindow, NavArea::cfgkey_mainwindow_arptmapscale));
	m_coorddialog->directto_waypoint(selwpt);
}

void FlightDeckWindow::fpldirpage_buttonduplicate(void)
{
	Gtk::TreeModel::iterator iter(fpldirpage_cfmdialog(fpldbcfmmode_duplicate));
	if (!iter || !m_engine)
		return;
	Gtk::TreeModel::Row row(*iter);
	FPlanRoute fpl(m_engine->get_fplan_db());
	fpl.load_fp(row[m_fpldbcolumns.m_id]);
	fpl.duplicate_fp();
	fpl.save_fp();
	Gtk::TreeIter it(fpldirpage_updatefpl(fpl));
	if (m_fpldbtreeview) {
		Glib::RefPtr<Gtk::TreeSelection> selection(m_fpldbtreeview->get_selection());
		if (selection) {
			selection->unselect_all();
			selection->select(it);
		}
		m_fpldbtreeview->scroll_to_row(Gtk::TreePath(it));
	}
}

void FlightDeckWindow::fpldirpage_buttondelete(void)
{
	fpldirpage_cfmdialog(fpldbcfmmode_delete);
}

void FlightDeckWindow::fpldirpage_buttonreverse(void)
{
	Gtk::TreeModel::iterator iter(fpldirpage_cfmdialog(fpldbcfmmode_reverse));
	if (!iter || !m_engine)
		return;
	Gtk::TreeModel::Row row(*iter);
	FPlanRoute fpl(m_engine->get_fplan_db());
	fpl.load_fp(row[m_fpldbcolumns.m_id]);
	fpl.reverse_fp();
	fpl.save_fp();
	Gtk::TreeIter it(fpldirpage_updatefpl(fpl));
	if (m_fpldbtreeview) {
		Glib::RefPtr<Gtk::TreeSelection> selection(m_fpldbtreeview->get_selection());
		if (selection) {
			selection->unselect_all();
			selection->select(it);
		}
		m_fpldbtreeview->scroll_to_row(Gtk::TreePath(it));
	}
}

void FlightDeckWindow::fpldirpage_buttonnew(void)
{
	FPlanRoute fpl(m_engine->get_fplan_db());
        fpl.new_fp();
        fpl.set_note(Glib::ustring()); // make dirty
        fpl.save_fp();
	Gtk::TreeIter it(fpldirpage_updatefpl(fpl));
	if (m_fpldbtreeview) {
		Glib::RefPtr<Gtk::TreeSelection> selection(m_fpldbtreeview->get_selection());
		if (selection) {
			selection->unselect_all();
			selection->select(it);
		}
		m_fpldbtreeview->scroll_to_row(Gtk::TreePath(it));
	}
}

void FlightDeckWindow::fpldirpage_buttonimport(void)
{
	m_fpldbimportfilechooser.show();
}

void FlightDeckWindow::fpldirpage_buttonexport(void)
{
	m_fpldbexportfilechooser.show();
}

void FlightDeckWindow::fpldirpage_exportfileresponse(int response)
{
	m_fpldbexportfilechooser.hide();
	switch (response) {
	case Gtk::RESPONSE_CANCEL:
	default:
		return;

	case Gtk::RESPONSE_OK:
		break;
	}
	std::vector<Glib::RefPtr<Gio::File> > files(m_fpldbexportfilechooser.get_files());
	if (files.empty())
		return;
	std::string path(files[0]->get_path());
	if (path.empty())
		return;
	std::ostringstream ids;
	{
		Glib::RefPtr<Gtk::TreeSelection> selection(m_fpldbtreeview->get_selection());
		if (!selection)
			return;
		std::vector<Gtk::TreePath> selrows(selection->get_selected_rows());
		if (selrows.empty())
			return;
		bool subseq(false);
		for (std::vector<Gtk::TreePath>::const_iterator si(selrows.begin()), se(selrows.end()); si != se; ++si) {
			Gtk::TreeIter iter(m_fpldbstore->get_iter(*si));
			if (!iter)
				continue;
			Gtk::TreeModel::Row row(*iter);
			if (subseq)
				ids << ',';
			subseq = true;
			ids << (FPlanRoute::id_t)row[m_fpldbcolumns.m_id];
		}
	}
	if (ids.str().empty())
		return;
	try {
		std::vector<std::string> argv, env;
#ifdef __MINGW32__
		std::string workdir;
		{
			gchar *instdir(g_win32_get_package_installation_directory_of_module(0));
			if (instdir) {
				workdir = instdir;
				argv.push_back(Glib::build_filename(instdir, "bin", "vfrnavfplan.exe"));
			} else {
				argv.push_back(Glib::build_filename(PACKAGE_PREFIX_DIR "/bin", "vfrnavfplan.exe"));
				workdir = PACKAGE_PREFIX_DIR;
			}
		}
#else
		std::string workdir(PACKAGE_DATA_DIR);
		argv.push_back(Glib::build_filename(PACKAGE_PREFIX_DIR "/bin", "vfrnavfplan"));
#endif
		argv.push_back("-r");
		argv.push_back(ids.str());
		switch (m_fpldbexportformat.get_active_row_number()) {
		default:
		case 0:
			argv.push_back("--export");
			break;

		case 1:
			argv.push_back("--gpx");
			break;

		case 2:
			argv.push_back("--garmin-fpl");
			break;
		}
		argv.push_back(path);
		env.push_back("PATH=/bin:/usr/bin");
		if (true) {
			std::cerr << "export: command:";
			for (std::vector<std::string>::const_iterator ai(argv.begin()), ae(argv.end()); ai != ae; ++ai)
				std::cerr << ' ' << *ai;
			std::cerr << std::endl;
		}
#ifdef __MINGW32__
                // inherit environment; otherwise, this may fail on WindowsXP with STATUS_SXS_ASSEMBLY_NOT_FOUND (C0150004)
		Glib::spawn_async_with_pipes(workdir, argv, (Glib::SpawnFlags)0, sigc::slot<void>(), 0, 0, 0, 0);
#else
		Glib::spawn_async_with_pipes(workdir, argv, env, (Glib::SpawnFlags)0, sigc::slot<void>(), 0, 0, 0, 0);
#endif
	} catch (Glib::SpawnError& e) {
		std::cerr << "export: command failed: " << e.what() << std::endl;
	}
}

void FlightDeckWindow::fpldirpage_importfileresponse(int response)
{
	m_fpldbimportfilechooser.hide();
	switch (response) {
	case Gtk::RESPONSE_CANCEL:
	default:
		return;

	case Gtk::RESPONSE_OK:
		break;
	}
	std::vector<Glib::RefPtr<Gio::File> > files(m_fpldbimportfilechooser.get_files());
	if (files.empty())
		return;
	try {
		std::vector<std::string> argv, env;
#ifdef __MINGW32__
		std::string workdir;
		{
			gchar *instdir(g_win32_get_package_installation_directory_of_module(0));
			if (instdir) {
				workdir = instdir;
				argv.push_back(Glib::build_filename(instdir, "bin", "vfrnavfplan.exe"));
			} else {
				argv.push_back(Glib::build_filename(PACKAGE_PREFIX_DIR "/bin", "vfrnavfplan.exe"));
				workdir = PACKAGE_PREFIX_DIR;
			}
		}
#else
		std::string workdir(PACKAGE_DATA_DIR);
		argv.push_back(Glib::build_filename(PACKAGE_PREFIX_DIR "/bin", "vfrnavfplan"));
#endif
		argv.push_back("--import");
		for (std::vector<Glib::RefPtr<Gio::File> >::const_iterator fi(files.begin()), fe(files.end()); fi != fe; ++fi) {
			std::string path((*fi)->get_path());
			if (path.empty())
				continue;
			argv.push_back(path);
		}
		env.push_back("PATH=/bin:/usr/bin");
		if (true) {
			std::cerr << "import: command:";
			for (std::vector<std::string>::const_iterator ai(argv.begin()), ae(argv.end()); ai != ae; ++ai)
				std::cerr << ' ' << *ai;
			std::cerr << std::endl;
		}
		Glib::Pid childpid;
#ifdef __MINGW32__
                // inherit environment; otherwise, this may fail on WindowsXP with STATUS_SXS_ASSEMBLY_NOT_FOUND (C0150004)
		Glib::spawn_async_with_pipes(workdir, argv, Glib::SPAWN_DO_NOT_REAP_CHILD, sigc::slot<void>(), &childpid, 0, 0, 0);
#else
		Glib::spawn_async_with_pipes(workdir, argv, env, Glib::SPAWN_DO_NOT_REAP_CHILD, sigc::slot<void>(), &childpid, 0, 0, 0);
#endif
		Glib::signal_child_watch().connect(sigc::mem_fun(this, &FlightDeckWindow::fpldirpage_importchildwatch), childpid);
	} catch (Glib::SpawnError& e) {
		std::cerr << "import: command failed: " << e.what() << std::endl;
	}
}

void FlightDeckWindow::fpldirpage_importchildwatch(GPid pid, int child_status)
{
        Glib::spawn_close_pid(pid);
	fpldirpage_updatemenu();
}

void FlightDeckWindow::fpldirpage_updatepos(void)
{
	if (!m_sensors)
		return;
	Point pt;
	m_sensors->get_position(pt);
	for (Gtk::TreeIter it1(m_fpldbstore->children().begin()), it1e(m_fpldbstore->children().end()); it1 != it1e; ++it1) {
		float mindist(std::numeric_limits<float>::max());
		Gtk::TreeModel::Row row(*it1);
		for (Gtk::TreeIter it2(row.children().begin()), it2e(row.children().end()); it2 != it2e; ++it2) {
			Gtk::TreeModel::Row wptrow(*it2);
			float dist(pt.spheric_distance_nmi(wptrow[m_fpldbcolumns.m_coord]));
			wptrow[m_fpldbcolumns.m_dist] = dist;
			mindist = std::min(mindist, dist);
		}
		if (mindist == std::numeric_limits<float>::max())
			mindist = std::numeric_limits<float>::quiet_NaN();
		row[m_fpldbcolumns.m_dist] = mindist;
	}
}

void FlightDeckWindow::fpldirpage_updatefpl(const FPlanRoute& fpl, Gtk::TreeIter& it)
{
	if (!it || !m_fpldbstore)
		return;
	Gtk::TreeModel::Row row(*it);
	if (fpl.get_nrwpt() > 0) {
		const FPlanWaypoint& wpt0(fpl[0]);
		const FPlanWaypoint& wpt1(fpl[fpl.get_nrwpt() - 1]);
		row[m_fpldbcolumns.m_fromicao] = wpt0.get_icao();
		row[m_fpldbcolumns.m_fromname] = wpt0.get_name();
		row[m_fpldbcolumns.m_toicao] = wpt1.get_icao();
		row[m_fpldbcolumns.m_toname] = wpt1.get_name();
	}
	row[m_fpldbcolumns.m_dist] = 0;
	row[m_fpldbcolumns.m_style] = Pango::STYLE_NORMAL;
	row[m_fpldbcolumns.m_id] = fpl.get_id();
	row[m_fpldbcolumns.m_wptnr] = RouteListModelColumns::nowptnr;
	row[m_fpldbcolumns.m_coord] = Point();
	float totlen(0);
	Gtk::TreeIter iter(row.children().begin());
	for (unsigned int wnr = 0; wnr < fpl.get_nrwpt(); ++wnr) {
		const FPlanWaypoint& wpt(fpl[wnr]);
		float len(std::numeric_limits<float>::quiet_NaN());
		if (wnr + 1 < fpl.get_nrwpt()) {
			const FPlanWaypoint& wpt1(fpl[wnr + 1]);
			len = wpt.get_coord().spheric_distance_nmi(wpt1.get_coord());
			totlen += len;
		}
		if (!iter)
			iter = m_fpldbstore->append(row.children());
		Gtk::TreeModel::Row wptrow(*iter);
		wptrow[m_fpldbcolumns.m_fromicao] = wpt.get_icao();
		wptrow[m_fpldbcolumns.m_fromname] = wpt.get_name();
		wptrow[m_fpldbcolumns.m_toicao] = "";
		wptrow[m_fpldbcolumns.m_toname] = "";
		wptrow[m_fpldbcolumns.m_dist] = 0;
		wptrow[m_fpldbcolumns.m_length] = len;
		wptrow[m_fpldbcolumns.m_style] = (fpl.get_curwpt() == wnr) ? Pango::STYLE_ITALIC : Pango::STYLE_NORMAL;
		wptrow[m_fpldbcolumns.m_id] = fpl.get_id();
		wptrow[m_fpldbcolumns.m_wptnr] = wnr;
		wptrow[m_fpldbcolumns.m_coord] = wpt.get_coord();
		++iter;
	}
	while (iter)
		iter = m_fpldbstore->erase(iter);
	row[m_fpldbcolumns.m_length] = totlen;
}

Gtk::TreeIter FlightDeckWindow::fpldirpage_updatefpl(FPlanRoute::id_t id)
{
	if (!m_fpldbstore || !m_engine)
		return Gtk::TreeIter();
	FPlanRoute fpl(m_engine->get_fplan_db());
	fpl.load_fp(id);
	return fpldirpage_updatefpl(fpl);
}

Gtk::TreeIter FlightDeckWindow::fpldirpage_updatefpl(const FPlanRoute& fpl)
{
	for (Gtk::TreeIter it(m_fpldbstore->children().begin()), ite(m_fpldbstore->children().end()); it != ite; ++it) {
		Gtk::TreeModel::Row row(*it);
		FPlanRoute::id_t rid(row[m_fpldbcolumns.m_id]);
		if (rid == fpl.get_id()) {
			fpldirpage_updatefpl(fpl, it);
			return it;
		}
	}
	for (Gtk::TreeIter it(m_fpldbstore->children().begin()), ite(m_fpldbstore->children().end()); it != ite; ++it) {
		Gtk::TreeModel::Row row(*it);
		FPlanRoute::id_t rid(row[m_fpldbcolumns.m_id]);
		if (rid > fpl.get_id()) {
			it = m_fpldbstore->insert(it);
			fpldirpage_updatefpl(fpl, it);
			return it;
		}
	}
	{
		Gtk::TreeIter it(m_fpldbstore->append());	
		fpldirpage_updatefpl(fpl, it);
		return it;
	}
}

void FlightDeckWindow::fpldirpage_deletefpl(FPlanRoute::id_t id)
{
	if (!m_fpldbstore)
		return;
	for (Gtk::TreeIter it(m_fpldbstore->children().begin()); it;) {
		Gtk::TreeModel::Row row(*it);
		FPlanRoute::id_t rid(row[m_fpldbcolumns.m_id]);
		if (rid == id) {
			it = m_fpldbstore->erase(it);
			continue;
		}
		++it;
	}
}

void FlightDeckWindow::fpldirpage_updatemenu(void)
{
	if (!m_fpldbstore)
		return;
	if (m_menuid != 201) {
		m_fpldbselchangeconn.block();
		m_fpldbstore->clear();
		m_fpldbselchangeconn.unblock();
		fpldb_selection_changed();
		return;
	}
	if (m_engine) {
		//if (m_fpldbtreeview)
		//	m_fpldbtreeview->unset_model();
		m_fpldbselchangeconn.block();
		FPlanRoute::id_t id(-1);
		if (m_sensors &&
		    m_sensors->get_configfile().has_group(cfggroup_mainwindow) &&
		    m_sensors->get_configfile().has_key(cfggroup_mainwindow, cfgkey_mainwindow_fplid))
			id = m_sensors->get_configfile().get_integer(cfggroup_mainwindow, cfgkey_mainwindow_fplid);
		Glib::RefPtr<Gtk::TreeSelection> selection;
		if (m_fpldbtreeview)
			selection = m_fpldbtreeview->get_selection();
		Gtk::TreeIter seliter;
		FPlanRoute fpl(m_engine->get_fplan_db());
		Gtk::TreeIter it(m_fpldbstore->children().begin());
		bool ok(fpl.load_first_fp());
		while (ok) {
			if (!it)
			        it = m_fpldbstore->append();
			fpldirpage_updatefpl(fpl, it);
			if (fpl.get_id() == id)
				seliter = it;
			++it;
			ok = fpl.load_next_fp();
		}
		while (it)
			it = m_fpldbstore->erase(it);
		if (m_fpldbtreeview)
			m_fpldbtreeview->columns_autosize();
		//if (m_fpldbtreeview)
		//	m_fpldbtreeview->set_model(m_fpldbstore);
		if (seliter) {
			if (selection)
				selection->select(seliter);
			if (m_fpldbtreeview) {
				Gtk::TreePath tpath(seliter);
				m_fpldbtreeview->scroll_to_row(tpath);
				if (true)
					std::cerr << "flightplan directory: scroll to " << tpath.to_string() << std::endl;
			}
		}
		if (true)
			std::cerr << "flightplan directory: fpl id " << id << (seliter ? " found" : " not found") << std::endl;
		m_fpldbselchangeconn.unblock();
		fpldb_selection_changed();
	}
	fpldirpage_updatepos();
}
