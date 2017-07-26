#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <fstream>
#include <sstream>
#include <iostream>
#include <iomanip>

#include "cfmuautoroute45.hh"

const CFMUAutoroute45::lgraphairwayindex_t CFMUAutoroute45::airwaymatchall;
const CFMUAutoroute45::lgraphairwayindex_t CFMUAutoroute45::airwaymatchnone;
const CFMUAutoroute45::lgraphairwayindex_t CFMUAutoroute45::airwaymatchawy;
const CFMUAutoroute45::lgraphairwayindex_t CFMUAutoroute45::airwaymatchdctawy;
const CFMUAutoroute45::lgraphairwayindex_t CFMUAutoroute45::airwaymatchdctawysidstar;
const CFMUAutoroute45::lgraphairwayindex_t CFMUAutoroute45::airwaymatchsidstar;
const CFMUAutoroute45::lgraphairwayindex_t CFMUAutoroute45::airwaysid;
const CFMUAutoroute45::lgraphairwayindex_t CFMUAutoroute45::airwaystar;
const CFMUAutoroute45::lgraphairwayindex_t CFMUAutoroute45::airwaydct;

CFMUAutoroute45::CFMUAutoroute45()
	: CFMUAutoroute(), m_vertexdep(0), m_vertexdest(0)
{
	// test compile regexes
	for (const char * const *ignrx(ignoreregex); *ignrx; ++ignrx) {
		Glib::RefPtr<Glib::Regex> rx(Glib::Regex::create(*ignrx));
		if (true)
			continue;
		std::cerr << "Ignore Regex: " << rx->get_pattern() << " capcount " << rx->get_capture_count()
			  << " max lookbehind " << rx->get_max_lookbehind() << std::endl;
	}
	for (const struct parsefunctions *pf(parsefunctions); pf->regex; ++pf) {
		Glib::RefPtr<Glib::Regex> rx(Glib::Regex::create(pf->regex));
		if (true)
			continue;
		std::cerr << "Error Regex: " << rx->get_pattern() << " capcount " << rx->get_capture_count()
			  << " max lookbehind " << rx->get_max_lookbehind() << std::endl;
	}
}

bool CFMUAutoroute45::check_fplan(TrafficFlowRestrictions::Result& res, const FPlanRoute& route)
{
	if (!get_tfr_available_enabled()) {
                res = TrafficFlowRestrictions::Result();
                return false;
        }
	opendb();
	res = m_tfr.check_fplan(m_route, 'G', m_aircraft.get_icaotype(), m_aircraft.get_equipment(), m_aircraft.get_pbn(),
				m_airportdb, m_navaiddb, m_waypointdb, m_airwaydb, m_airspacedb);
	return true;
}

void CFMUAutoroute45::loadtfr(void)
{
	if (m_tfrloaded)
		return;
	if (!m_tfrenable)
		return;
	opendb();
	Glib::TimeVal tv;
	tv.assign_current_time();
	m_tfr.start_add_rules();
	std::vector<std::string> tfrfnames;
	bool ok(false);
	{
		std::string fname(get_tfr_bin_filename());
		if (Glib::file_test(fname, Glib::FILE_TEST_EXISTS) && !Glib::file_test(fname, Glib::FILE_TEST_IS_DIR)) {
			m_signal_log(log_normal, "Loading traffic flow restrictions " + fname);
			std::vector<TrafficFlowRestrictions::Message> msg;
			ok = m_tfr.add_binary_rules(msg, fname, m_airportdb, m_navaiddb, m_waypointdb, m_airwaydb, m_airspacedb);
			for (unsigned int i = 0; i < msg.size(); ++i)
				logmessage(msg[i]);
			if (ok)
				tfrfnames.push_back(fname);
		}
	}
	if (!ok) {
		std::string fname(get_tfr_xml_filename());
		if (Glib::file_test(fname, Glib::FILE_TEST_EXISTS) && !Glib::file_test(fname, Glib::FILE_TEST_IS_DIR)) {
			m_signal_log(log_normal, "Loading traffic flow restrictions " + fname);
			std::vector<TrafficFlowRestrictions::Message> msg;
			ok = m_tfr.add_rules(msg, fname, m_airportdb, m_navaiddb, m_waypointdb, m_airwaydb, m_airspacedb);
			for (unsigned int i = 0; i < msg.size(); ++i)
				logmessage(msg[i]);
			if (ok)
				tfrfnames.push_back(fname);
		}
	}
	bool okloc(false);
	{
		std::string fname(get_tfr_local_bin_filename());
		if (Glib::file_test(fname, Glib::FILE_TEST_EXISTS) && !Glib::file_test(fname, Glib::FILE_TEST_IS_DIR)) {
			m_signal_log(log_normal, "Loading traffic flow restrictions " + fname);
			std::vector<TrafficFlowRestrictions::Message> msg;
			okloc = m_tfr.add_binary_rules(msg, fname, m_airportdb, m_navaiddb, m_waypointdb, m_airwaydb, m_airspacedb);
			for (unsigned int i = 0; i < msg.size(); ++i)
				logmessage(msg[i]);
			if (okloc)
				tfrfnames.push_back(fname);
		}
	}
	if (!okloc) {
		std::string fname(get_tfr_local_xml_filename());
		if (Glib::file_test(fname, Glib::FILE_TEST_EXISTS) && !Glib::file_test(fname, Glib::FILE_TEST_IS_DIR)) {
			m_signal_log(log_normal, "Loading traffic flow restrictions " + fname);
			std::vector<TrafficFlowRestrictions::Message> msg;
			okloc = m_tfr.add_rules(msg, fname, m_airportdb, m_navaiddb, m_waypointdb, m_airwaydb, m_airspacedb);
			for (unsigned int i = 0; i < msg.size(); ++i)
				logmessage(msg[i]);
			if (okloc)
				tfrfnames.push_back(fname);
		}
	}
	bool okuser(false);
	{
		std::string fname(get_tfr_user_bin_filename());
		if (Glib::file_test(fname, Glib::FILE_TEST_EXISTS) && !Glib::file_test(fname, Glib::FILE_TEST_IS_DIR)) {
			m_signal_log(log_normal, "Loading traffic flow restrictions " + fname);
			std::vector<TrafficFlowRestrictions::Message> msg;
			okuser = m_tfr.add_binary_rules(msg, fname, m_airportdb, m_navaiddb, m_waypointdb, m_airwaydb, m_airspacedb);
			for (unsigned int i = 0; i < msg.size(); ++i)
				logmessage(msg[i]);
			if (okuser)
				tfrfnames.push_back(fname);
		}
	}
	if (!okuser) {
		std::string fname(get_tfr_user_xml_filename());
		if (Glib::file_test(fname, Glib::FILE_TEST_EXISTS) && !Glib::file_test(fname, Glib::FILE_TEST_IS_DIR)) {
			m_signal_log(log_normal, "Loading traffic flow restrictions " + fname);
			std::vector<TrafficFlowRestrictions::Message> msg;
			okuser = m_tfr.add_rules(msg, fname, m_airportdb, m_navaiddb, m_waypointdb, m_airwaydb, m_airspacedb);
			for (unsigned int i = 0; i < msg.size(); ++i)
				logmessage(msg[i]);
			if (okuser)
				tfrfnames.push_back(fname);
		}
	}
	{
		Glib::TimeVal tv1;
		tv1.assign_current_time();
		tv = tv1 - tv;
	}
	if (!ok) {
		m_tfravailable = false;
		set_tfr_enabled(false);
		m_signal_log(log_normal, "Cannot load traffic flow restrictions " + get_tfr_bin_filename() + ", " + get_tfr_xml_filename());
	}
	m_tfr.end_add_rules();
	if (ok || okloc || okuser) {
		m_tfr.reset_rules();
		{
			std::ostringstream oss;
			oss << m_tfr.count_rules() << " rules loaded traffic flow restrictions ";
			for (std::vector<std::string>::const_iterator i(tfrfnames.begin()), e(tfrfnames.end()); i != e; ) {
				oss << *i;
				++i;
				if (i == e)
					break;
				oss << ", ";
			}
			oss << ", created " << m_tfr.get_created() << ", effective " << m_tfr.get_effective()
			    << ", " << std::fixed << std::setprecision(3) << tv.as_double() << 's';
			m_signal_log(log_normal, oss.str());
		}
	}
	m_tfr.disabledrules_clear();
	m_tfr.tracerules_clear();
	{
		std::string fname(get_tfr_global_rules_filename());
		if (Glib::file_test(fname, Glib::FILE_TEST_EXISTS) && !Glib::file_test(fname, Glib::FILE_TEST_IS_DIR)) {
			if (m_tfr.load_disabled_trace_rules_file(fname)) {
				std::ostringstream oss;
				oss << "Loaded " << m_tfr.disabledrules_count() << " disabled and "
				    << m_tfr.tracerules_count() << " trace rules from global " << fname;
				m_signal_log(log_normal, oss.str());
			}
		}
	}
	{
		std::string fname(get_tfr_local_rules_filename());
		if (Glib::file_test(fname, Glib::FILE_TEST_EXISTS) && !Glib::file_test(fname, Glib::FILE_TEST_IS_DIR)) {
			unsigned int dcnt(m_tfr.disabledrules_count());
			unsigned int tcnt(m_tfr.tracerules_count());
			if (m_tfr.load_disabled_trace_rules_file(fname)) {
				std::ostringstream oss;
				oss << "Loaded " << (m_tfr.disabledrules_count() - dcnt) << " disabled and "
				    << (m_tfr.tracerules_count() - tcnt) << " trace rules from user " << fname;
				m_signal_log(log_normal, oss.str());
			}
		}
	}
	// copy for later
	m_systemdisabledrules.clear();
	for (TrafficFlowRestrictions::disabledrules_const_iterator_t i(m_tfr.disabledrules_begin()), e(m_tfr.disabledrules_end()); i != e; ++i)
		m_systemdisabledrules.insert(*i);
	m_systemtracerules.clear();
	for (TrafficFlowRestrictions::tracerules_const_iterator_t i(m_tfr.tracerules_begin()), e(m_tfr.tracerules_end()); i != e; ++i)
		m_systemtracerules.insert(*i);
	m_tfrloaded = true;
}

void CFMUAutoroute45::tfrpreparedct(void)
{
	Glib::TimeVal tv;
	tv.assign_current_time();
	m_tfr.prepare_dct_rules();
	{
		Glib::TimeVal tv1;
		tv1.assign_current_time();
		tv = tv1 - tv;
	}
	std::ostringstream oss;
	oss << m_tfr.count_dct_rules() << " DCT rules, "
	    << std::fixed << std::setprecision(3) << tv.as_double() << 's';
	m_signal_log(log_normal, oss.str());
}

void CFMUAutoroute45::reload(void)
{
	m_tfrloaded = false;
	CFMUAutoroute::reload();
}

void CFMUAutoroute45::preload(bool validator)
{
	CFMUAutoroute::preload(validator);
	loadtfr();
}

void CFMUAutoroute45::stop(statusmask_t sm)
{
	CFMUAutoroute::stop(sm);
	if (!m_cfmuintel.is_empty()) {
		m_cfmuintel.open(get_db_maindir(), get_db_auxdir());
		m_cfmuintel.flush();
		m_cfmuintel.close();
	}
}

void CFMUAutoroute45::clear(void)
{
	CFMUAutoroute::clear();
	clearlgraph();
	m_cfmuintel.clear();
}

void CFMUAutoroute45::precompute_graph(const Rect& bbox, const std::string& fn)
{
	m_performance.clear();
	{
		int level(m_levels[0]);
		level = ((level + 9) / 10) * 10;
		for (; level <= m_levels[1]; level += 10)
			m_performance.add(Performance::Cruise(level * 100, level * 100, 1, 1, 1));
	}
	m_performance.set(0, 0, Performance::LevelChange());
	loadtfr();
	tfrpreparedct();
	//set_dctlimit(50);
	set_opttarget(opttarget_time);
	clearlgraph();
	if (!lgraphload(bbox))
		throw std::runtime_error("cannot load graph");
	lgraphkillinvalidsuper();
	lgraphadddct();
	if (true) {
		std::ostringstream oss;
		oss << "Airway graph after DCT: " << boost::num_vertices(m_lgraph) << " vertices, "
		    << boost::num_edges(m_lgraph) << " edges";
		m_signal_log(log_debug0, oss.str());
	}
	lgraphapplycfmuintel(bbox);
	if (true) {
		std::ostringstream oss;
		oss << "Airway graph after stored restrictions: " << boost::num_vertices(m_lgraph) << " vertices, "
		    << boost::num_edges(m_lgraph) << " edges";
		m_signal_log(log_debug0, oss.str());
	}
	std::ofstream os(fn.c_str(), std::ofstream::binary);
	if (!os)
		throw std::runtime_error("cannot create file " + fn);
	lgraphsave(os, bbox);
}

bool CFMUAutoroute45::start_ifr(bool cont, bool optfuel)
{
	if (start_ifr_specialcase()) {
		m_tvelapsed.assign_current_time();
		m_tvvalidator = m_tvvalidatestart = Glib::TimeVal(0, 0);
		m_running = true;
		m_signal_statuschange(statusmask_starting);
		m_signal_log(log_normal, "Starting special case IFR router");
		m_done = true;
		m_iterationcount[0] = 1;
		m_signal_log(log_fplproposal, get_simplefpl());
		return false;
	}
	cont = cont && (m_vertexdep < boost::num_vertices(m_lgraph))
		&& (m_vertexdest < boost::num_vertices(m_lgraph))
		&& boost::num_vertices(m_lgraph);
	if (!cont) {
		clear();
		m_cfmuintel.clear();
		m_callsign.clear();
		for (unsigned int i = 0; i < 5; ++i) {
			char ch('A' + (rand() % ('Z' - 'A' + 1)));
			m_callsign.push_back(ch);
		}
		m_tvelapsed.assign_current_time();
		m_tvvalidator = m_tvvalidatestart = Glib::TimeVal(0, 0);
		m_running = true;
		m_signal_statuschange(statusmask_starting);
		preload();
		m_tfr.tracerules_clear();
		for (ruleset_t::const_iterator i(m_tracerules.begin()), e(m_tracerules.end()); i != e; ++i)
			m_tfr.tracerules_add(*i);
		for (ruleset_t::const_iterator i(m_systemtracerules.begin()), e(m_systemtracerules.end()); i != e; ++i)
			m_tfr.tracerules_add(*i);
		m_tfr.disabledrules_clear();
		for (ruleset_t::const_iterator i(m_disabledrules.begin()), e(m_disabledrules.end()); i != e; ++i)
			m_tfr.disabledrules_add(*i);
		for (ruleset_t::const_iterator i(m_systemdisabledrules.begin()), e(m_systemdisabledrules.end()); i != e; ++i)
			m_tfr.disabledrules_add(*i);
		if (get_tfr_available_enabled()) {
			m_tfr.reset_rules();
			{
				std::ostringstream oss;
				oss << m_tfr.count_rules() << " rules, created " << m_tfr.get_created() << ", effective " << m_tfr.get_effective();
				m_signal_log(log_normal, oss.str());
			}
			if (false) {
				std::ostringstream oss;
				oss << "Rules: Disabled: " << get_tfr_disable() << " Trace: " << get_tfr_trace();
				m_signal_log(log_normal, oss.str());
			}
			if (true) {
				{
					std::ostringstream oss;
					oss << "Disabled Rules:";
					if (!m_disabledrules.empty()) {
						oss << " user: ";
					        bool subseq(false);
						for (ruleset_t::const_iterator i(m_disabledrules.begin()),
							     e(m_disabledrules.end()); i != e; ++i) {
							if (subseq)
								oss <<',';
							subseq = true;
							oss << *i;
						}
					}
					if (!m_systemdisabledrules.empty()) {
						oss << " system: ";
					        bool subseq(false);
						for (ruleset_t::const_iterator i(m_systemdisabledrules.begin()),
							     e(m_systemdisabledrules.end()); i != e; ++i) {
							if (subseq)
								oss <<',';
							subseq = true;
							oss << *i;
						}
					}
					if (m_disabledrules.empty() && m_systemdisabledrules.empty())
						oss << " none";
					m_signal_log(log_normal, oss.str());
				}
				{
					std::ostringstream oss;
					oss << "Trace Rules:";
					if (!m_tracerules.empty()) {
						oss << " user: ";
					        bool subseq(false);
						for (ruleset_t::const_iterator i(m_tracerules.begin()),
							     e(m_tracerules.end()); i != e; ++i) {
							if (subseq)
								oss <<',';
							subseq = true;
							oss << *i;
						}
					}
					if (!m_systemtracerules.empty()) {
						oss << " system: ";
					        bool subseq(false);
						for (ruleset_t::const_iterator i(m_systemtracerules.begin()),
							     e(m_systemtracerules.end()); i != e; ++i) {
							if (subseq)
								oss <<',';
							subseq = true;
							oss << *i;
						}
					}
					if (m_tracerules.empty() && m_systemtracerules.empty())
						oss << " none";
					m_signal_log(log_normal, oss.str());
				}
			}
			if (true) {
				Glib::TimeVal tv;
				tv.assign_current_time();
				m_tfr.simplify_rules_typeofflight('G');
				m_tfr.simplify_rules_mil(false);
				m_tfr.simplify_rules_gat(true);
				{
					Glib::TimeVal tv1;
					tv1.assign_current_time();
					tv = tv1 - tv;
				}
				std::ostringstream oss;
				oss << m_tfr.count_rules() << " rules after fixing type of flight G CIV GAT, "
				    << std::fixed << std::setprecision(3) << tv.as_double() << 's';
				m_signal_log(log_normal, oss.str());
			}
			if (true) {
				Glib::TimeVal tv;
				tv.assign_current_time();
				if (m_airport[0].is_valid() && m_airport[0].get_icao().size() == 4 && m_airport[0].get_icao() != "ZZZZ")
					m_tfr.simplify_rules_dep(m_airport[0].get_icao(), m_airport[0].get_coord());
				if (m_airport[1].is_valid() && m_airport[1].get_icao().size() == 4 && m_airport[1].get_icao() != "ZZZZ")
					m_tfr.simplify_rules_dest(m_airport[1].get_icao(), m_airport[1].get_coord());
				{
					Glib::TimeVal tv1;
					tv1.assign_current_time();
					tv = tv1 - tv;
				}
				std::ostringstream oss;
				oss << m_tfr.count_rules() << " rules after fixing departure " << m_airport[0].get_icao()
				    << " destination " << m_airport[1].get_icao()
				    << ", " << std::fixed << std::setprecision(3) << tv.as_double() << 's';
				m_signal_log(log_normal, oss.str());
			}
			if (true) {
				Glib::TimeVal tv;
				tv.assign_current_time();
				m_tfr.simplify_rules_aircrafttype(m_aircraft.get_icaotype());
				{
					Glib::TimeVal tv1;
					tv1.assign_current_time();
					tv = tv1 - tv;
				}
				std::ostringstream oss;
				oss << m_tfr.count_rules() << " rules after fixing aircraft type " << m_aircraft.get_icaotype()
				    << ", " << std::fixed << std::setprecision(3) << tv.as_double() << 's';
				m_signal_log(log_normal, oss.str());
			}
			if (true) {
				Glib::TimeVal tv;
				tv.assign_current_time();
				m_tfr.simplify_rules_equipment(m_aircraft.get_equipment(), m_aircraft.get_pbn());
				{
					Glib::TimeVal tv1;
					tv1.assign_current_time();
					tv = tv1 - tv;
				}
				std::ostringstream oss;
				oss << m_tfr.count_rules() << " rules after fixing aircraft equipment " << m_aircraft.get_equipment()
				    << " PBN/" << m_aircraft.get_pbn_string() << ", " << std::fixed << std::setprecision(3) << tv.as_double() << 's';
				m_signal_log(log_normal, oss.str());
			}
			if (true) {
				Glib::TimeVal tv;
				tv.assign_current_time();
				m_tfr.simplify_rules_altrange(m_levels[0] * 100, m_levels[1] * 100);
				{
					Glib::TimeVal tv1;
					tv1.assign_current_time();
					tv = tv1 - tv;
				}
				std::ostringstream oss;
				oss << m_tfr.count_rules() << " rules after fixing cruise levels F"
				    << std::setw(3) << std::setfill('0') << m_levels[0] << "..F"
				    << std::setw(3) << std::setfill('0') << m_levels[1]
				    << ", " << std::fixed << std::setprecision(3) << tv.as_double() << 's';
				m_signal_log(log_normal, oss.str());
			}
			if (true) {
				Glib::TimeVal tv;
				tv.assign_current_time();
				Rect bbox(get_bbox().oversize_nmi(200.f));
				m_tfr.simplify_rules_bbox(bbox);
				{
					Glib::TimeVal tv1;
					tv1.assign_current_time();
					tv = tv1 - tv;
				}
				std::ostringstream oss;
				oss << m_tfr.count_rules() << " rules after fixing area ["
				    << bbox.get_southwest().get_lat_str2() << ' ' << bbox.get_southwest().get_lon_str2() << ' '
				    << bbox.get_northeast().get_lat_str2() << ' ' << bbox.get_northeast().get_lon_str2()
				    << "], " << std::fixed << std::setprecision(3) << tv.as_double() << 's';
				m_signal_log(log_normal, oss.str());
			}
			if (true) {
				Glib::TimeVal tv;
				tv.assign_current_time();
				m_tfr.simplify_rules();
				m_tfr.simplify_rules_complexity();
				{
					Glib::TimeVal tv1;
					tv1.assign_current_time();
					tv = tv1 - tv;
				}
				std::ostringstream oss;
				oss << m_tfr.count_rules() << " rules after fixing complexity, "
				    << std::fixed << std::setprecision(3) << tv.as_double() << 's';
				m_signal_log(log_normal, oss.str());
			}
			tfrpreparedct();
			if (!get_tfr_savefile().empty())
				m_tfr.save_binary_rules(get_tfr_savefile());
		} else {
			m_signal_log(log_normal, "Traffic flow restrictions disabled");
		}
	}
	m_signal_log(log_normal, cont ? "Restarting IFR router" : "Starting IFR router");
	if (!cont) {
		m_iterationcount[0] = m_iterationcount[1] = 0;
		clearlgraph();
		Rect bbox(get_bbox());
		bbox = bbox.oversize_nmi(100.f);
		if (get_precompgraph_enabled() && lgraphbinload(bbox)) {
			//lgraphfixedgemetric();
			if (true) {
				std::ostringstream oss;
				oss << "Airway graph after deserializing: " << boost::num_vertices(m_lgraph) << " vertices, "
				    << boost::num_edges(m_lgraph) << " edges";
				m_signal_log(log_normal, oss.str());
			}
			lgraphkilllongdct();
		} else {
			clearlgraph();
			if (!lgraphload(bbox))
				return false;
			lgraphkillinvalidsuper();
			lgraphadddct();
		}
		//lgraphfixedgemetric();
		if (true) {
			std::ostringstream oss;
			oss << "Airway graph after DCT: " << boost::num_vertices(m_lgraph) << " vertices, "
			    << boost::num_edges(m_lgraph) << " edges";
			m_signal_log(log_normal, oss.str());
		}
		lgraphexcluderegions();
		//lgraphfixedgemetric();
		if (true) {
			std::ostringstream oss;
			oss << "Airway graph after manually excluded regions: " << boost::num_vertices(m_lgraph) << " vertices, "
			    << boost::num_edges(m_lgraph) << " edges";
	       		m_signal_log(log_normal, oss.str());
		}
		if (!lgraphaddsidstar()) {
			if (true)
				m_signal_log(log_normal, "Cannot add SID/STAR to Airway graph");
			return false;
		}
		lgraphedgemetric();
		if (true) {
			std::ostringstream oss;
			oss << "Airway graph after SID/STAR: " << boost::num_vertices(m_lgraph) << " vertices, "
			    << boost::num_edges(m_lgraph) << " edges";
	       		m_signal_log(log_normal, oss.str());
		}
		lgraphapplycfmuintel(bbox);
		if (true) {
			std::ostringstream oss;
			oss << "Airway graph after stored restrictions: " << boost::num_vertices(m_lgraph) << " vertices, "
			    << boost::num_edges(m_lgraph) << " edges";
			m_signal_log(log_debug0, oss.str());
		}
		lgraphcrossing();
	} else {
		m_tvelapsed.assign_current_time();
		m_tvvalidator = m_tvvalidatestart = Glib::TimeVal(0, 0);
		m_running = true;
		m_signal_statuschange(statusmask_starting);
	}
	lgraphmodified();
	lgraphroute();
	return true;
}

void CFMUAutoroute45::logmessage(const TrafficFlowRestrictions::Message& msg)
{
	std::ostringstream oss;
	oss << msg.get_type_char() << ':';
	if (!msg.get_rule().empty())
		oss << " R:" << msg.get_rule();
	if (!msg.get_vertexset().empty()) {
		oss << " V:";
		bool subseq(false);
		for (TrafficFlowRestrictions::Message::set_t::const_iterator i(msg.get_vertexset().begin()), e(msg.get_vertexset().end()); i != e; ++i) {
			if (subseq)
				oss << ',';
			subseq = true;
			oss << *i;
		}
	}
	if (!msg.get_edgeset().empty()) {
		oss << " E:";
		bool subseq(false);
		for (TrafficFlowRestrictions::Message::set_t::const_iterator i(msg.get_edgeset().begin()), e(msg.get_edgeset().end()); i != e; ++i) {
			if (subseq)
				oss << ',';
			subseq = true;
			oss << *i;
		}
	}
	oss << ' ' << msg.get_text();
	log_t lt(log_debug1);
	switch (msg.get_type()) {
	case TrafficFlowRestrictions::Message::type_error:
		lt = log_normal;
		break;

	case TrafficFlowRestrictions::Message::type_warning:
		lt = log_debug0;
		break;

	default:
		lt = log_debug1;
		break;
	}
	m_signal_log(lt, oss.str());
}
