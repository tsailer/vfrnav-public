#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <fstream>
#include <sstream>
#include <iostream>
#include <iomanip>

#include "cfmuautoroute51.hh"

CFMUAutoroute51::CFMUAutoroute51()
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

bool CFMUAutoroute51::check_fplan(std::vector<ADR::Message>& msgs, ADR::RestrictionResults& res, ADR::FlightPlan& route)
{
	m_eval.set_fplan(route);
	bool r(m_eval.check_fplan(get_honour_profilerules()));
	route = m_eval.get_fplan();
	msgs.swap(m_eval.get_messages());
	m_eval.clear_messages();
	res.swap(m_eval.get_results());
	m_eval.clear_results();
	return r;
}

void CFMUAutoroute51::reload(void)
{
	m_eval = ADR::RestrictionEval();
	m_db.close();
	m_db.open();
	CFMUAutoroute::reload();
}

void CFMUAutoroute51::preload(bool validator)
{
	CFMUAutoroute::preload(validator);
	m_eval.set_graph(&m_graph);
	m_eval.set_db(&m_db);
	if (!m_eval.count_rules()) {
		Glib::TimeVal tv;
		tv.assign_current_time();
		m_eval.load_rules();
		{
			Glib::TimeVal tv1;
			tv1.assign_current_time();
			tv = tv1 - tv;
		}
		std::ostringstream oss;
		oss << m_eval.count_rules() << " rules loaded, "
		    << std::fixed << std::setprecision(3) << tv.as_double() << 's';
		m_signal_log(log_normal, oss.str());
	}
}

void CFMUAutoroute51::stop(statusmask_t sm)
{
	CFMUAutoroute::stop(sm);
}

void CFMUAutoroute51::clear(void)
{
	CFMUAutoroute::clear();
	clearlgraph();
}

void CFMUAutoroute51::precompute_graph(const Rect& bbox, const std::string& fn)
{
	throw std::runtime_error("precomputed graphs not supported in AIXM5.1 mode");
}

void CFMUAutoroute51::debug_print_rules(const std::string& fname)
{
	if (true)
		return;
	std::ofstream os(Glib::build_filename(get_logdir(), fname).c_str(),
			 std::ofstream::out | std::ofstream::trunc);
	if (!os.is_open() || !os.good())
		return;
	std::set<std::string> rdis, rena;
	for (ADR::RestrictionEval::rules_t::const_iterator
		     ri(m_eval.get_srules().begin()), re(m_eval.get_srules().end()); ri != re; ++ri) {
		const ADR::FlightRestrictionTimeSlice& tsr((*ri)->operator()(get_deptime()).as_flightrestriction());
		if (!tsr.is_valid())
			continue;
		if (!tsr.is_enabled()) {
			rdis.insert(tsr.get_ident());
			continue;
		}
		rena.insert(tsr.get_ident());
		if (true || tsr.get_ident() == "EGD513A")
			tsr.print(os) << std::endl;
	}
	if (false) {
		os << "Disabled Rules:";
		bool subseq(false);
		for (std::set<std::string>::const_iterator i(rdis.begin()), e(rdis.end()); i != e; ++i, subseq = true)
			os << (subseq ? ',' : ' ') << *i;
		os << std::endl;
	}
	if (false) {
		os << "Enabled Rules:";
		bool subseq(false);
		for (std::set<std::string>::const_iterator i(rena.begin()), e(rena.end()); i != e; ++i, subseq = true)
			os << (subseq ? ',' : ' ') << *i;
		os << std::endl;
	}
}

bool CFMUAutoroute51::start_ifr(bool cont, bool optfuel)
{
	m_db.set_path(get_db_auxdir().empty() ? PACKAGE_DATA_DIR : get_db_auxdir());
	m_aupdb.set_path(get_db_auxdir().empty() ? PACKAGE_DATA_DIR : get_db_auxdir());
	m_eval.set_graph(&m_graph);
	m_eval.set_db(&m_db);
	// set departure time
	{
		ADR::FlightPlan fpl;
		fpl.set_aircraft(get_aircraft(), get_engine_rpm(), get_engine_mp(), get_engine_bhp());
		fpl.set_aircraftid(m_callsign);
		fpl.set_flighttype('G');
		fpl.set_number(1);
		fpl.set_departuretime(get_deptime());
		fpl.set_personsonboard_tbn();
		m_eval.set_fplan(fpl);
	}
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
	cont = cont && (m_vertexdep < boost::num_vertices(m_graph))
		&& (m_vertexdest < boost::num_vertices(m_graph))
		&& boost::num_vertices(m_graph);
	ADR::timetype_t tmroute(3600);
	if (!cont) {
		clear();
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
		if (get_departure().is_valid() && get_destination().is_valid()) {
			double rtetm(0);
			for (unsigned int pi(0), pis(m_performance.size()); pi < pis; ++pi) {
				const Performance::Cruise& cruise(m_performance.get_cruise(pi));
				rtetm = std::max(rtetm, cruise.get_secpernmi());
			}
			rtetm *= get_departure().get_coord().spheric_distance_nmi_dbl(get_destination().get_coord());
			rtetm *= 2;
			tmroute = Point::round<ADR::timetype_t,double>(rtetm) + 3600;
		}
		try {
			m_eval.load_aup(m_aupdb, get_deptime(), get_deptime() + tmroute);
		} catch (const std::exception& e) {
			std::ostringstream oss;
			oss << "Cannot load AUP: " << e.what();
			m_signal_log(log_normal, oss.str());
		}
		if (true || get_tfr_enabled()) {
			m_eval.reset_rules();
			for (ruleset_t::const_iterator i(m_tracerules.begin()), e(m_tracerules.end()); i != e; ++i)
				m_eval.trace_rule(*i);
			for (ruleset_t::const_iterator i(m_disabledrules.begin()), e(m_disabledrules.end()); i != e; ++i)
				m_eval.disable_rule(*i);
			if (false) {
				std::ostringstream oss;
				oss << "Rules: Disabled: " << get_tfr_disable() << " Trace: " << get_tfr_trace();
				m_signal_log(log_normal, oss.str());
			}
			if (true) {
				std::set<std::string> dr, tr;
				for (ADR::RestrictionEval::rules_t::const_iterator ri(m_eval.get_rules().begin()), re(m_eval.get_rules().end()); ri != re; ++ri) {
					const ADR::FlightRestrictionTimeSlice& ts((*ri)->operator()(get_deptime()).as_flightrestriction());
					if (!ts.is_valid() || ts.get_ident().empty())
						continue;
					if (!ts.is_enabled())
						dr.insert(ts.get_ident());
					if (ts.is_trace())
						tr.insert(ts.get_ident());
				}
				if (!dr.empty()) {
					std::ostringstream oss;
					oss << "Disabled Rules: ";
					bool subseq(false);
					for (std::set<std::string>::const_iterator i(dr.begin()), e(dr.end()); i != e; ++i) {
						if (subseq)
							oss <<',';
						subseq = true;
						oss << *i;
					}
					m_signal_log(log_normal, oss.str());
				}
				if (!tr.empty()) {
					std::ostringstream oss;
					oss << "Trace Rules: ";
					bool subseq(false);
					for (std::set<std::string>::const_iterator i(tr.begin()), e(tr.end()); i != e; ++i) {
						if (subseq)
							oss <<',';
						subseq = true;
						oss << *i;
					}
					m_signal_log(log_normal, oss.str());
				}
			}
			debug_print_rules("rules.0.initial.log");
			if (true) {
				Glib::TimeVal tv;
				tv.assign_current_time();
				m_eval.simplify_rules_typeofflight('G');
				m_eval.simplify_rules_mil(false);
				{
					Glib::TimeVal tv1;
					tv1.assign_current_time();
					tv = tv1 - tv;
				}
				std::ostringstream oss;
				oss << m_eval.count_srules() << " rules after fixing type of flight G CIV, "
				    << std::fixed << std::setprecision(3) << tv.as_double() << 's';
				m_signal_log(log_normal, oss.str());
			}
			debug_print_rules("rules.1.flttype.log");
			if (true) {
				Glib::TimeVal tv;
				tv.assign_current_time();
				m_eval.simplify_rules_aircrafttype(m_aircraft.get_icaotype());
				m_eval.simplify_rules_aircraftclass(Aircraft::get_aircrafttypeclass(m_aircraft.get_icaotype()));
				{
					Glib::TimeVal tv1;
					tv1.assign_current_time();
					tv = tv1 - tv;
				}
				std::ostringstream oss;
				oss << m_eval.count_srules() << " rules after fixing aircraft type " << m_aircraft.get_icaotype()
				    << " class " << Aircraft::get_aircrafttypeclass(m_aircraft.get_icaotype())
				    << ", " << std::fixed << std::setprecision(3) << tv.as_double() << 's';
				m_signal_log(log_normal, oss.str());
			}
			debug_print_rules("rules.2.acft.log");
			if (true) {
				Glib::TimeVal tv;
				tv.assign_current_time();
				m_eval.simplify_rules_equipment(m_aircraft.get_equipment_string(), m_aircraft.get_pbn());
				{
					Glib::TimeVal tv1;
					tv1.assign_current_time();
					tv = tv1 - tv;
				}
				std::ostringstream oss;
				oss << m_eval.count_srules() << " rules after fixing aircraft equipment " << m_aircraft.get_equipment_string()
				    << " PBN/" << m_aircraft.get_pbn_string() << ", " << std::fixed << std::setprecision(3) << tv.as_double() << 's';
				m_signal_log(log_normal, oss.str());
			}
			debug_print_rules("rules.3.equip.log");
			if (true) {
				Glib::TimeVal tv;
				tv.assign_current_time();
				m_eval.simplify_rules_altrange(m_levels[0] * 100, m_levels[1] * 100);
				{
					Glib::TimeVal tv1;
					tv1.assign_current_time();
					tv = tv1 - tv;
				}
				std::ostringstream oss;
				oss << m_eval.count_srules() << " rules after fixing cruise levels F"
				    << std::setw(3) << std::setfill('0') << m_levels[0] << "..F"
				    << std::setw(3) << std::setfill('0') << m_levels[1]
				    << ", " << std::fixed << std::setprecision(3) << tv.as_double() << 's';
				m_signal_log(log_normal, oss.str());
			}
			debug_print_rules("rules.4.alt.log");
			if (true) {
				Glib::TimeVal tv;
				tv.assign_current_time();
				Rect bbox(get_bbox().oversize_nmi(200.f));
				m_eval.simplify_rules_bbox(bbox);
				{
					Glib::TimeVal tv1;
					tv1.assign_current_time();
					tv = tv1 - tv;
				}
				std::ostringstream oss;
				oss << m_eval.count_srules() << " rules after fixing area ["
				    << bbox.get_southwest().get_lat_str2() << ' ' << bbox.get_southwest().get_lon_str2() << ' '
				    << bbox.get_northeast().get_lat_str2() << ' ' << bbox.get_northeast().get_lon_str2()
				    << "], " << std::fixed << std::setprecision(3) << tv.as_double() << 's';
				m_signal_log(log_normal, oss.str());
			}
			debug_print_rules("rules.5.area.log");
			if (true) {
				Glib::TimeVal tv;
				tv.assign_current_time();
				m_eval.simplify_rules();
				{
					Glib::TimeVal tv1;
					tv1.assign_current_time();
					tv = tv1 - tv;
				}
				std::ostringstream oss;
				oss << m_eval.count_srules() << " rules after fixing complexity, "
				    << std::fixed << std::setprecision(3) << tv.as_double() << 's';
				m_signal_log(log_normal, oss.str());
			}
			debug_print_rules("rules.6.cplx.log");
			if (true) {
				Glib::TimeVal tv;
				tv.assign_current_time();
				ADR::timetype_t tmroute(3600);
				if (get_departure().is_valid() && get_destination().is_valid()) {
					double rtetm(0);
					for (unsigned int pi(0), pis(m_performance.size()); pi < pis; ++pi) {
						const Performance::Cruise& cruise(m_performance.get_cruise(pi));
						rtetm = std::max(rtetm, cruise.get_secpernmi());
					}
					rtetm *= get_departure().get_coord().spheric_distance_nmi_dbl(get_destination().get_coord());
					rtetm *= 2;
					tmroute = Point::round<ADR::timetype_t,double>(rtetm) + 3600;
				}
				m_eval.simplify_rules_time(get_deptime(), get_deptime() + tmroute);
				{
					Glib::TimeVal tv1;
					tv1.assign_current_time();
					tv = tv1 - tv;
				}
				std::ostringstream oss;
				oss << m_eval.count_srules() << " rules after fixing time "
				    << Glib::TimeVal(get_deptime(), 0).as_iso8601() << "..."
				    << Glib::TimeVal(get_deptime() + tmroute, 0).as_iso8601() << ", "
				    << std::fixed << std::setprecision(3) << tv.as_double() << 's';
				m_signal_log(log_normal, oss.str());
			}
			debug_print_rules("rules.7.time.log");
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
		clearlgraph();
		if (!lgraphload(bbox))
			return false;
		if (true) {
			std::ostringstream oss;
			oss << "Airway graph after DCT: " << boost::num_vertices(m_graph) << " vertices, "
			    << boost::num_edges(m_graph) << " edges";
			m_signal_log(log_normal, oss.str());
		}
		if (true && !get_logdir().empty()) {
			std::ofstream os(Glib::build_filename(get_logdir(), "loadedgraph.log").c_str(),
					 std::ofstream::out | std::ofstream::trunc);
			if (os.is_open() && os.good())
				m_graph.print(os);
		}
		lgraphexcluderegions();
		//lgraphfixedgemetric();
		if (true) {
			std::ostringstream oss;
			oss << "Airway graph after manually excluded regions: " << boost::num_vertices(m_graph) << " vertices, "
			    << boost::num_edges(m_graph) << " edges";
	       		m_signal_log(log_normal, oss.str());
		}
		if (!lgraphaddsidstar()) {
			if (true)
				m_signal_log(log_normal, "Cannot add SID/STAR to Airway graph");
			return false;
		}
		lgraphedgemetric();
		if (true && !get_logdir().empty()) {
			std::ofstream os(Glib::build_filename(get_logdir(), "depdestgraph.log").c_str(),
					 std::ofstream::out | std::ofstream::trunc);
			if (os.is_open() && os.good())
				m_graph.print(os);
		}
		lgraphcompositeedges();
		if (true) {
			std::ostringstream oss;
			oss << "Airway graph after SID/STAR: " << boost::num_vertices(m_graph) << " vertices, "
			    << boost::num_edges(m_graph) << " edges";
	       		m_signal_log(log_normal, oss.str());
		}
		if (true && !get_logdir().empty()) {
			std::ofstream os(Glib::build_filename(get_logdir(), "compedgegraph.log").c_str(),
					 std::ofstream::out | std::ofstream::trunc);
			if (os.is_open() && os.good())
				m_graph.print(os);
		}
		lgraphcrossing();
		if (true || get_tfr_enabled()) {
			if (true) {
				Glib::TimeVal tv;
				tv.assign_current_time();
				const ADR::GraphVertex& sdep(m_graph[m_vertexdep]);
				const ADR::GraphVertex& sdest(m_graph[m_vertexdest]);
				m_eval.simplify_rules_dep(ADR::Link(ADR::Object::ptr_t::cast_const(sdep.get_object())));
				m_eval.simplify_rules_dest(ADR::Link(ADR::Object::ptr_t::cast_const(sdest.get_object())));
				{
					Glib::TimeVal tv1;
					tv1.assign_current_time();
					tv = tv1 - tv;
				}
				std::ostringstream oss;
				oss << m_eval.count_srules() << " rules after fixing departure " << sdep.get_ident()
				    << " destination " << sdest.get_ident()
				    << ", " << std::fixed << std::setprecision(3) << tv.as_double() << 's';
				m_signal_log(log_normal, oss.str());
			}
			debug_print_rules("rules.8.arrdep.log");
			if (true) {
				Glib::TimeVal tv;
				tv.assign_current_time();
				m_eval.simplify_conditionalavailability(m_graph, ADR::timepair_t(get_deptime(), get_deptime() + tmroute));
				{
					Glib::TimeVal tv1;
					tv1.assign_current_time();
					tv = tv1 - tv;
				}
				std::ostringstream oss;
				oss << m_eval.count_srules() << " rules after fixing conditional availability "
				    << Glib::TimeVal(get_deptime(), 0).as_iso8601() << "..."
				    << Glib::TimeVal(get_deptime() + tmroute, 0).as_iso8601() << ", "
				    << std::fixed << std::setprecision(3) << tv.as_double() << 's';
				m_signal_log(log_normal, oss.str());
			}
			debug_print_rules("rules.9.cond.log");
		}
		if (get_tfr_enabled())
			lgraphapplylocalrules();
		if (!get_logdir().empty()) {
			std::ofstream os(Glib::build_filename(get_logdir(), "initialgraph.log").c_str(),
					 std::ofstream::out | std::ofstream::trunc);
			if (os.is_open() && os.good())
				m_graph.print(os);
		}
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
