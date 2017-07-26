#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <getopt.h>
#include <iostream>
#include <iomanip>
#include <glibmm.h>
#include <stdexcept>

#ifdef HAVE_SYSEXITS_H
#include <sysexits.h>
#else
#define EX_OK           0
#define EX_USAGE        64
#define EX_DATAERR      65
#define EX_NOINPUT      66
#define EX_UNAVAILABLE  69
#endif  

#include "engine.h"
#include "aircraft.h"
#include "icaofpl.h"
#include "tfr.hh"
#include "tfrproxy.hh"
#include "adr.hh"
#include "adrdb.hh"
#include "adrfplan.hh"
#include "adrrestriction.hh"

class Command : public FPlanCheckerProxy::Command {
public:
	Command(void) : FPlanCheckerProxy::Command() {}
	Command(const std::string& cmd) : FPlanCheckerProxy::Command(cmd) {}

	using FPlanCheckerProxy::Command::set_option;
};

static Glib::RefPtr<Glib::MainLoop> mainloop;
typedef void (*cmd_t)(const Command& cmdin, Command& cmdout);
typedef std::map<std::string,cmd_t> cmdlist_t;
static cmdlist_t cmdlist;
static Glib::RefPtr<Glib::IOChannel> inputchan;
static Glib::RefPtr<Glib::IOChannel> outputchan;
static AirportsDb airportdb;
static NavaidsDb navaiddb;
static WaypointsDb waypointdb;
static AirwaysDb airwaydb;
static AirspacesDb airspacedb;
static ADR::Database adrdb;
static Engine *eng(0);
static Glib::ustring dir_main(""), dir_aux("");
static Engine::auxdb_mode_t auxdbmode(Engine::auxdb_prefs);
static TrafficFlowRestrictions tfr;
static ADR::RestrictionEval reval;
static bool parseexpand(false);
static bool useadr(false);
static bool tfrloaded(false);
static bool adrloaded(false);
static bool fplvariants(false);
static bool honourprofilerules(true);

static void loadtfr(void)
{
	if (tfrloaded)
		return;
	std::vector<TrafficFlowRestrictions::Message> msg;
	tfr.start_add_rules();
	std::vector<std::string> tfrfnames;
	bool ok(false);
	{
		std::string fname(Glib::build_filename(dir_aux, "eRAD.bin"));
		if (Glib::file_test(fname, Glib::FILE_TEST_EXISTS) && !Glib::file_test(fname, Glib::FILE_TEST_IS_DIR)) {
			std::cerr << "Loading traffic flow restrictions " << fname << std::endl;
			std::vector<TrafficFlowRestrictions::Message> msg1;
			ok = tfr.add_binary_rules(msg1, fname, airportdb, navaiddb, waypointdb, airwaydb, airspacedb);
			msg.insert(msg.end(), msg1.begin(), msg1.end());
			if (ok)
				tfrfnames.push_back(fname);
		}
	}
	if (!ok) {
		std::string fname(Glib::build_filename(dir_aux, "eRAD.xml"));
		if (Glib::file_test(fname, Glib::FILE_TEST_EXISTS) && !Glib::file_test(fname, Glib::FILE_TEST_IS_DIR)) {
			std::cerr << "Loading traffic flow restrictions " << fname << std::endl;
			std::vector<TrafficFlowRestrictions::Message> msg1;
			ok = tfr.add_rules(msg1, fname, airportdb, navaiddb, waypointdb, airwaydb, airspacedb);
			msg.insert(msg.end(), msg1.begin(), msg1.end());
			if (ok)
				tfrfnames.push_back(fname);
		}
	}
	bool okloc(false);
	{
		std::string fname(Glib::build_filename(dir_aux, "eRAD_local.bin"));
		if (Glib::file_test(fname, Glib::FILE_TEST_EXISTS) && !Glib::file_test(fname, Glib::FILE_TEST_IS_DIR)) {
			std::cerr << "Loading traffic flow restrictions " << fname << std::endl;
			std::vector<TrafficFlowRestrictions::Message> msg1;
			okloc = tfr.add_binary_rules(msg1, fname, airportdb, navaiddb, waypointdb, airwaydb, airspacedb);
			msg.insert(msg.end(), msg1.begin(), msg1.end());
			if (okloc)
				tfrfnames.push_back(fname);
		}
	}
	if (!okloc) {
		std::string fname(Glib::build_filename(dir_aux, "eRAD_local.xml"));
		if (Glib::file_test(fname, Glib::FILE_TEST_EXISTS) && !Glib::file_test(fname, Glib::FILE_TEST_IS_DIR)) {
			std::cerr << "Loading traffic flow restrictions " << fname << std::endl;
			std::vector<TrafficFlowRestrictions::Message> msg1;
			okloc = tfr.add_rules(msg1, fname, airportdb, navaiddb, waypointdb, airwaydb, airspacedb);
			msg.insert(msg.end(), msg1.begin(), msg1.end());
			if (okloc)
				tfrfnames.push_back(fname);
		}
	}
	bool okuser(false);
	{
		std::string fname(Glib::build_filename(FPlan::get_userdbpath(), "eRAD.bin"));
		if (Glib::file_test(fname, Glib::FILE_TEST_EXISTS) && !Glib::file_test(fname, Glib::FILE_TEST_IS_DIR)) {
			std::cerr << "Loading traffic flow restrictions " << fname << std::endl;
			std::vector<TrafficFlowRestrictions::Message> msg1;
			okuser = tfr.add_binary_rules(msg1, fname, airportdb, navaiddb, waypointdb, airwaydb, airspacedb);
			msg.insert(msg.end(), msg1.begin(), msg1.end());
			if (okuser)
				tfrfnames.push_back(fname);
		}
	}
	if (!okuser) {
		std::string fname(Glib::build_filename(FPlan::get_userdbpath(), "eRAD.xml"));
		if (Glib::file_test(fname, Glib::FILE_TEST_EXISTS) && !Glib::file_test(fname, Glib::FILE_TEST_IS_DIR)) {
			std::cerr << "Loading traffic flow restrictions " << fname << std::endl;
			std::vector<TrafficFlowRestrictions::Message> msg1;
			okuser = tfr.add_rules(msg1, fname, airportdb, navaiddb, waypointdb, airwaydb, airspacedb);
			msg.insert(msg.end(), msg1.begin(), msg1.end());
			if (okuser)
				tfrfnames.push_back(fname);
		}
	}
	if (!ok) {
		std::cerr << "Cannot load Traffic Flow Restrictions file " << Glib::build_filename(dir_aux, "eRAD.xml") << std::endl;
		exit(EX_UNAVAILABLE);
	}
	tfr.end_add_rules();
	if (ok || okloc || okuser) {
		tfr.reset_rules();
		tfr.prepare_dct_rules();
		if (true) {
			std::cerr << "Origin:    " << tfr.get_origin() << std::endl
				  << "Created:   " << tfr.get_created() << std::endl
				  << "Effective: " << tfr.get_effective() << std::endl;
			std::cerr << tfr.count_rules() << " rules, " << tfr.count_dct_rules() << " DCT rules" << std::endl;
		}
	}
	{
		std::string fname(Glib::build_filename(dir_aux, "eRAD_rules.xml"));
		if (Glib::file_test(fname, Glib::FILE_TEST_EXISTS) && !Glib::file_test(fname, Glib::FILE_TEST_IS_DIR)) {
			if (tfr.load_disabled_trace_rules_file(fname))
				std::cerr << "Loaded global disabled/trace rules from " << fname << std::endl;
		}
	}
	{
		std::string fname(Glib::build_filename(FPlan::get_userdbpath(), "eRAD_rules.xml"));
		if (Glib::file_test(fname, Glib::FILE_TEST_EXISTS) && !Glib::file_test(fname, Glib::FILE_TEST_IS_DIR)) {
			if (tfr.load_disabled_trace_rules_file(fname))
				std::cerr << "Loaded user disabled/trace rules from " << fname << std::endl;
		}
	}
	tfrloaded = true;
}

static void loadadr(void)
{
	if (adrloaded)
		return;
	Glib::TimeVal tv;
	tv.assign_current_time();
	reval.set_db(&adrdb);
	reval.load_rules();
	{
		Glib::TimeVal tv1;
		tv1.assign_current_time();
		tv = tv1 - tv;
	}
	std::ostringstream oss;
	oss << std::fixed << std::setprecision(1) << tv.as_double() << 's';
	std::cerr << "Loaded " << reval.count_rules() << " Restrictions, " << oss.str() << std::endl;
	adrloaded = true;
}

static void machine_writecmd(const Command& cmd)
{
        outputchan->write(cmd.get_cmdstring() + "\n");
}

static bool machine_input(Glib::IOCondition iocond)
{
        Glib::ustring line;
        Glib::IOStatus iostat(inputchan->read_line(line));
        if (iostat == Glib::IO_STATUS_ERROR ||
            iostat == Glib::IO_STATUS_EOF) {
                mainloop->quit();
                return true;
        }
        while (!line.empty()) {
                char ch(line[line.size() - 1]);
                if (ch != '\r' && ch != '\n')
                        break;
                line.erase(line.size() - 1, 1);
        }
        Command cmdin(line);
        Command cmdout;
        {
                std::string cmdname("checkfplan");
                cmdname = cmdin.get_cmdname();
                if (cmdname.empty())
                        cmdname = "checkfplan";
                cmdout.set_cmdname(cmdname);
                {
                        std::pair<const std::string&,bool> seq(cmdin.get_option("cmdseq"));
                        if (seq.second)
                                cmdout.set_option("cmdseq", seq.first);
                }
        }
        try {
                Command cmdout1(cmdout);
                cmdlist_t::const_iterator cmdi(cmdlist.find(cmdin.get_cmdname()));
                if (cmdi == cmdlist.end()) {
                        cmdout1.set_option("error", "command not found");
                } else {
                        (cmdi->second)(cmdin, cmdout1);
                }
                machine_writecmd(cmdout1);
        } catch (const std::runtime_error& e) {
                cmdout.set_option("error", e.what());
		machine_writecmd(cmdout);
        } catch (const Glib::Exception& e) {
                cmdout.set_option("error", e.what());
                machine_writecmd(cmdout);
        }
        outputchan->flush();
        return true;
}

static void cmd_nop(const Command& cmdin, Command& cmdout)
{
}

static void cmd_quit(const Command& cmdin, Command& cmdout)
{
	mainloop->quit();
}

static void print_tracerules(void)
{
	std::string r;
	bool subseq(false);
	for (TrafficFlowRestrictions::tracerules_const_iterator_t ti(tfr.tracerules_begin()), te(tfr.tracerules_end()); ti != te; ++ti) {
		if (subseq)
			r += ",";
		subseq = true;
		r += *ti;
	}
	outputchan->write("trace:" + r + "\n");
	outputchan->flush();
}

static void print_message(const TrafficFlowRestrictions::Message& msg)
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
	outputchan->write(oss.str() + "\n");
}

static void print_adr_tracerules(void)
{
	std::string r;
	bool subseq(false);
	uint64_t tm(time(0));
	for (ADR::RestrictionEval::rules_t::const_iterator ri(reval.get_rules().begin()), re(reval.get_rules().end()); ri != re; ++ri) {
		const ADR::Object::ptr_t& p(*ri);
		const ADR::FlightRestrictionTimeSlice& ts(p->operator()(tm).as_flightrestriction());
		if (!ts.is_valid())
			continue;
		if (!ts.is_enabled())
			continue;
		if (!ts.is_trace())
			continue;
		if (subseq)
			r += ",";
		subseq = true;
		r += ts.get_ident();
	}
	outputchan->write("trace:" + r + "\n");
	outputchan->flush();
}

static void print_adr_message(const ADR::Message& msg)
{
	outputchan->write(msg.to_str() + "\n");
}

static std::vector<Glib::ustring> tokenize(const Glib::ustring& line, Glib::ustring::size_type n = 0, char delim = ',')
{
	std::vector<Glib::ustring> r;
	for (;;) {
		while (n < line.size() && std::isspace(line[n]))
			++n;
		if (n >= line.size())
			return r;
		Glib::ustring::size_type n1(line.find(delim, n + 1));
		Glib::ustring::size_type ne(n1);
		if (n1 == Glib::ustring::npos)
			ne = n1 = line.size();
		else
			++n1;
		while (ne > n && std::isspace(line[ne-1]))
			--ne;
		r.push_back(line.substr(n, ne - n));
		n = n1;
	}
}

static bool normal_input(Glib::IOCondition iocond)
{
        Glib::ustring line;
        Glib::IOStatus iostat(inputchan->read_line(line));
        if (iostat == Glib::IO_STATUS_ERROR ||
            iostat == Glib::IO_STATUS_EOF) {
                mainloop->quit();
                return true;
        }
        while (!line.empty()) {
                char ch(line[line.size() - 1]);
                if (ch != '\r' && ch != '\n')
                        break;
                line.erase(line.size() - 1, 1);
        }
	if (!line.compare(0, 4, "adr+")) {
		useadr = true;
		outputchan->write("adr+\n");
		outputchan->flush();
		loadadr();
		return true;
	}
	if (!line.compare(0, 4, "adr-")) {
		useadr = false;
		outputchan->write("adr-\n");
		outputchan->flush();
		loadtfr();
		return true;
	}
	if (!line.compare(0, 4, "adr?")) {
		outputchan->write(useadr ? "adr+\n" : "adr-\n");
		outputchan->flush();
		return true;
	}

	if (!line.compare(0, 4, "fpr+")) {
		honourprofilerules = true;
		outputchan->write("fpr+\n");
		outputchan->flush();
		loadadr();
		return true;
	}
	if (!line.compare(0, 4, "fpr-")) {
		honourprofilerules = false;
		outputchan->write("fpr-\n");
		outputchan->flush();
		loadtfr();
		return true;
	}
	if (!line.compare(0, 4, "fpr?")) {
		outputchan->write(honourprofilerules ? "fpr+\n" : "fpr-\n");
		outputchan->flush();
		return true;
	}
	if (!line.compare(0, 6, "trace?")) {
		if (useadr) {
			print_adr_tracerules();
			return true;
		}
		print_tracerules();
		return true;
	}
	if (!line.compare(0, 6, "trace:") || !line.compare(0, 6, "trace+")) {
		Glib::ustring txt1(line);
		Glib::ustring::size_type n(txt1.find('\n'));
		if (n != Glib::ustring::npos)
			txt1.erase(n);
		n = txt1.find(' ');
		if (n != Glib::ustring::npos)
			txt1.erase(n);
		txt1 = txt1.substr(6);
		if (useadr) {
			uint64_t tm(time(0));
			for (ADR::RestrictionEval::rules_t::const_iterator ri(reval.get_rules().begin()), re(reval.get_rules().end()); ri != re; ++ri) {
				const ADR::Object::ptr_t& p(*ri);
				ADR::FlightRestrictionTimeSlice& ts(p->operator()(tm).as_flightrestriction());
				if (!ts.is_valid())
					continue;
				if (!ts.is_enabled())
					continue;
				if (ts.get_ident() != txt1)
					continue;
				ts.set_trace(true);
			}
			print_adr_tracerules();
			return true;
		}
		tfr.tracerules_add(txt1);
		print_tracerules();
		return true;
	}
	if (!line.compare(0, 6, "trace-")) {
		Glib::ustring txt1(line);
		Glib::ustring::size_type n(txt1.find('\n'));
		if (n != Glib::ustring::npos)
			txt1.erase(n);
		n = txt1.find(' ');
		if (n != Glib::ustring::npos)
			txt1.erase(n);
		txt1 = txt1.substr(6);
		if (useadr) {
			uint64_t tm(time(0));
			for (ADR::RestrictionEval::rules_t::const_iterator ri(reval.get_rules().begin()), re(reval.get_rules().end()); ri != re; ++ri) {
				const ADR::Object::ptr_t& p(*ri);
				ADR::FlightRestrictionTimeSlice& ts(p->operator()(tm).as_flightrestriction());
				if (!ts.is_valid())
					continue;
				if (!ts.is_enabled())
					continue;
				if (ts.get_ident() != txt1)
					continue;
				ts.set_trace(false);
			}
			print_adr_tracerules();
			return true;
		}
		tfr.tracerules_erase(txt1);
		print_tracerules();
		return true;
	}
	if (!line.compare(0, 6, "trace*")) {
		if (useadr) {
			uint64_t tm(time(0));
			for (ADR::RestrictionEval::rules_t::const_iterator ri(reval.get_rules().begin()), re(reval.get_rules().end()); ri != re; ++ri) {
				const ADR::Object::ptr_t& p(*ri);
				ADR::FlightRestrictionTimeSlice& ts(p->operator()(tm).as_flightrestriction());
				if (!ts.is_valid())
					continue;
				if (!ts.is_enabled())
					continue;
				ts.set_trace(false);
			}
			print_adr_tracerules();
			return true;
		}
		tfr.tracerules_clear();
		print_tracerules();
		return true;
	}
	{
		bool trace(!line.compare(0, 5, "dctt:"));
		if (!line.compare(0, 4, "dct:") || trace) {
			std::vector<Glib::ustring> t(tokenize(line, 4 + trace, ' '));
			if (t.size() < 2 || t[0].empty() || t[1].empty()) {
				outputchan->write("dct: needs at least two parameters\n");
				outputchan->flush();
				return true;
			}
			std::vector<Point> pt[2];
			for (unsigned int i = 0; i < 2; ++i) {
				{
					WaypointsDb::elementvector_t ev(waypointdb.find_by_name(t[i], 0, WaypointsDb::comp_exact, 0, WaypointsDb::element_t::subtables_none));
					for (WaypointsDb::elementvector_t::const_iterator ei(ev.begin()), ee(ev.end()); ei != ee; ++ei) {
						if (!ei->is_valid() || ei->get_name() != t[i] || ei->get_coord().is_invalid())
							continue;
						pt[i].push_back(ei->get_coord());
					}
				}
				{
					NavaidsDb::elementvector_t ev(navaiddb.find_by_icao(t[i], 0, NavaidsDb::comp_exact, 0, NavaidsDb::element_t::subtables_none));
					for (NavaidsDb::elementvector_t::const_iterator ei(ev.begin()), ee(ev.end()); ei != ee; ++ei) {
						if (!ei->is_valid() || ei->get_icao() != t[i] || ei->get_coord().is_invalid())
							continue;
						pt[i].push_back(ei->get_coord());
					}
				}
				if (pt[i].empty()) {
					outputchan->write("dct: no coordinates found for ident " + t[i] + "\n");
					outputchan->flush();
					return true;
				}
			}
			unsigned int idx[2];
			idx[0] = idx[1] = 0;
			{
				double dist(std::numeric_limits<double>::max());
				for (unsigned int i = 0; i < pt[0].size(); ++i)
					for (unsigned int j = 0; j < pt[1].size(); ++j) {
						double d(pt[0][i].spheric_distance_nmi_dbl(pt[1][j]));
						if (d > dist)
							continue;
						idx[0] = i;
						idx[1] = j;
						dist = d;
					}
			}
			TrafficFlowRestrictions::DctParameters::AltRange::type_t altlwr(std::numeric_limits<TrafficFlowRestrictions::DctParameters::AltRange::type_t>::min());
			TrafficFlowRestrictions::DctParameters::AltRange::type_t altupr(std::numeric_limits<TrafficFlowRestrictions::DctParameters::AltRange::type_t>::max());
			if (t.size() >= 3)
				altlwr = strtol(t[2].c_str(), 0, 10);
			if (t.size() >= 4)
				altupr = strtol(t[3].c_str(), 0, 10);
			TrafficFlowRestrictions::DctParameters dct(t[0], pt[0][idx[0]], t[1], pt[1][idx[1]], altlwr, altupr);
			std::vector<TrafficFlowRestrictions::Message> msg;
			bool ok(tfr.check_dct(msg, dct, trace));
			for (std::vector<TrafficFlowRestrictions::Message>::const_iterator mi(msg.begin()), me(msg.end()); mi != me; ++mi)
				print_message(*mi);
			for (unsigned int i = 0; i < 2; ++i) {
				std::ostringstream oss;
				oss << "DCT: " << dct.get_ident(i) << "->" << dct.get_ident(!i)
				    << "  " << dct.get_coord(i).get_lat_str2() << ' ' << dct.get_coord(i).get_lon_str2()
				    << " -> " << dct.get_coord(!i).get_lat_str2() << ' ' << dct.get_coord(!i).get_lon_str2()
				    << " D" << std::fixed << std::setprecision(1) << dct.get_dist()
				    << " alt " << dct.get_alt(i).to_str();
				outputchan->write(oss.str() + "\n");
			}
			outputchan->write(ok ? "OK\n" : "FAIL\n");
			outputchan->flush();
			return true;
		}
	}
	if (useadr) {
		ADR::FlightPlan adrfpl;
		{
			ADR::FlightPlan::errors_t err(adrfpl.parse(adrdb, line, parseexpand));
			for (ADR::FlightPlan::errors_t::const_iterator i(err.begin()), e(err.end()); i != e; ++i) {
				outputchan->write("EFPL: " + *i + "\n");
				outputchan->flush();
			}
		}
		if (false) {
			ADR::TimeTableSpecialDateEval ttsde;
			ttsde.load(adrdb);
			adrfpl.add_fir_eet(adrdb, ttsde);
			std::ostringstream oss;
			oss << "FPL: " << adrfpl.get_fpl() << std::endl;
			outputchan->write(oss.str() + "\n");
			outputchan->flush();
		}
		// print Parsed Flight Plan
		if (false) {
			for (unsigned int i(0), n(adrfpl.size()); i < n; ++i) {
				const ADR::FPLWaypoint& wpt(adrfpl[i]);
				std::ostringstream oss;
				oss << "PFPL[" << i << '/' << wpt.get_wptnr() << "]: " 
				    << wpt.get_icao() << '/' << wpt.get_name() << ' ';
				switch (wpt.get_pathcode()) {
				case FPlanWaypoint::pathcode_sid:
				case FPlanWaypoint::pathcode_star:
				case FPlanWaypoint::pathcode_airway:
					oss << wpt.get_pathname();
					break;

				case FPlanWaypoint::pathcode_directto:
					oss << "DCT";
					break;

				default:
					oss << '-';
					break;
				}
				oss << ' ' << wpt.get_coord().get_lat_str2() << ' ' << wpt.get_coord().get_lon_str2()
				    << " D" << std::fixed << std::setprecision(1) << wpt.get_dist_nmi()
				    << " T" << std::fixed << std::setprecision(0) << wpt.get_truetrack_deg()
				    << ' ' << (wpt.is_standard() ? 'F' : 'A')
				    << std::setw(3) << std::setfill('0') << ((wpt.get_altitude() + 50) / 100)
				    << ' ' << (wpt.is_ifr() ? 'I' : 'V') << "FR";
				if (wpt.get_ptobj())
					oss << " ptobj " << wpt.get_ptobj()->get_uuid();
				if (wpt.get_pathobj())
					oss << " pathobj " << wpt.get_pathobj()->get_uuid();
				if (wpt.is_expanded())
					oss << " (E)";
				outputchan->write(oss.str() + "\n");
			}
		}
		if (!false) {
			std::ostringstream oss;
			oss << "FPLDIST: " << adrfpl.total_distance_nmi_dbl() << "nmi GC " << adrfpl.gc_distance_nmi_dbl() << "nmi";
			outputchan->write(oss.str() + "\n");
			outputchan->flush();
		}
		reval.set_fplan(adrfpl);
		reval.set_graph(0); // force rebuild of the graph
		bool res(reval.check_fplan(honourprofilerules));
		if (false) {
			std::ostringstream oss;
			oss << "CHK: " << (res ? "" : "NOT ") << "OK, " << reval.get_results().size()
			    << " rules " << (reval.get_results().is_ok() ? "" : "NOT ") << "OK";
			outputchan->write(oss.str() + "\n");
		}
		// print Flight Plan
		{
			ADR::FlightPlan f(reval.get_fplan());
			{
				std::ostringstream oss;
				oss << "FR: " << f.get_aircrafttype() << ' ' << Aircraft::get_aircrafttypeclass(f.get_aircrafttype()) << ' '
				    << f.get_equipment() << " PBN/" << f.get_pbn_string() << ' ' << f.get_flighttype();
				outputchan->write(oss.str() + "\n");
			}
			for (unsigned int i(0), n(f.size()); i < n; ++i) {
				const ADR::FPLWaypoint& wpt(f[i]);
				std::ostringstream oss;
				oss << "F[" << i << '/' << wpt.get_wptnr() << "]: " << wpt.get_ident() << ' ' << wpt.get_type_string() << ' ';
				switch (wpt.get_pathcode()) {
				case FPlanWaypoint::pathcode_sid:
				case FPlanWaypoint::pathcode_star:
				case FPlanWaypoint::pathcode_airway:
					oss << wpt.get_pathname();
					break;

				case FPlanWaypoint::pathcode_directto:
					oss << "DCT";
					break;

				default:
					oss << '-';
					break;
				}
				oss << ' ' << wpt.get_coord().get_lat_str2() << ' ' << wpt.get_coord().get_lon_str2()
				    << " D" << std::fixed << std::setprecision(1) << wpt.get_dist_nmi()
				    << " T" << std::fixed << std::setprecision(0) << wpt.get_truetrack_deg()
				    << ' ' << (wpt.is_standard() ? 'F' : 'A') << std::setw(3) << std::setfill('0') << ((wpt.get_altitude() + 50) / 100)
				    << ' ' << wpt.get_icao() << '/' << wpt.get_name() << ' ' << (wpt.is_ifr() ? 'I' : 'V') << "FR";
				{
					time_t t(wpt.get_time_unix());
					struct tm tm;
					if (gmtime_r(&t, &tm)) {
						oss << ' ' << std::setw(2) << std::setfill('0') << tm.tm_hour
						    << ':' << std::setw(2) << std::setfill('0') << tm.tm_min;
						if (t >= 48 * 60 * 60)
							oss << ' ' << std::setw(4) << std::setfill('0') << (tm.tm_year + 1900)
							    << '-' << std::setw(2) << std::setfill('0') << (tm.tm_mon + 1)
							    << '-' << std::setw(2) << std::setfill('0') << tm.tm_mday;
					}
				}
				if (wpt.get_ptobj())
					oss << " ptobj " << wpt.get_ptobj()->get_uuid();
				if (wpt.get_pathobj())
					oss << " pathobj " << wpt.get_pathobj()->get_uuid();
				if (wpt.is_expanded())
					oss << " (E)";
				outputchan->write(oss.str() + "\n");
			}
		}
		if (fplvariants) {
			ADR::FlightPlan route(reval.get_fplan());
			route.disable_none();
			outputchan->write("FPLALL: " + route.get_fpl() + "\n");
			route.disable_unnecessary(true, false);
			outputchan->write("FPLAWY: " + route.get_fpl() + "\n");
			route.disable_unnecessary(true, true);
			outputchan->write("FPLAWYDCT: " + route.get_fpl() + "\n");
			route.disable_unnecessary(false, true);
			outputchan->write("FPLMIN: " + route.get_fpl() + "\n");
			route.disable_unnecessary(false, false);
			outputchan->write("FPLATC: " + route.get_fpl() + "\n");
		}
		// print Messages
		for (ADR::RestrictionEval::messages_const_iterator mi(reval.messages_begin()), me(reval.messages_end()); mi != me; ++mi)
			print_adr_message(*mi);
		outputchan->write(res ? "OK\n" : "FAIL\n");
		for (ADR::RestrictionEval::results_const_iterator ri(reval.results_begin()), re(reval.results_end()); ri != re; ++ri) {
			const ADR::RestrictionResult& res(*ri);
			const ADR::FlightRestrictionTimeSlice& tsrule(res.get_rule()->operator()(reval.get_departuretime()).as_flightrestriction());
			if (!tsrule.is_valid()) {
				outputchan->write("Rule: ?""?\n");
				continue;
			}
			std::ostringstream oss;
			oss << "Rule: " << tsrule.get_ident() << ' ';
			switch (tsrule.get_type()) {
			case ADR::FlightRestrictionTimeSlice::type_mandatory:
				oss << "Mandatory";
				break;

			case ADR::FlightRestrictionTimeSlice::type_forbidden:
				oss << "Forbidden";
				break;

			case ADR::FlightRestrictionTimeSlice::type_closed:
				oss << "ClosedForCruising";
				break;

			case ADR::FlightRestrictionTimeSlice::type_allowed:
				oss << "Allowed";
				break;

			default:
				oss << '?';
				break;
			}
			if (tsrule.is_dct() || tsrule.is_unconditional() || tsrule.is_routestatic() ||
			    tsrule.is_mandatoryinbound() || tsrule.is_mandatoryoutbound() || !tsrule.is_enabled()) {
				oss << " (";
				if (!tsrule.is_enabled())
					oss << '-';
				if (tsrule.is_dct())
					oss << 'D';
				if (tsrule.is_unconditional())
					oss << 'U';
				if (tsrule.is_routestatic())
					oss << 'S';
				if (tsrule.is_mandatoryinbound())
					oss << 'I';
				if (tsrule.is_mandatoryoutbound())
					oss << 'O';
				oss << ')';
			}
			if (!res.get_vertexset().empty()) {
				oss << " V:";
				bool subseq(false);
				for (ADR::RestrictionResult::set_t::const_iterator i(res.get_vertexset().begin()), e(res.get_vertexset().end()); i != e; ++i) {
					if (subseq)
						oss << ',';
					subseq = true;
					oss << *i;
				}
			}
			if (!res.get_edgeset().empty()) {
				oss << " E:";
				bool subseq(false);
				for (ADR::RestrictionResult::set_t::const_iterator i(res.get_edgeset().begin()), e(res.get_edgeset().end()); i != e; ++i) {
					if (subseq)
						oss << ',';
					subseq = true;
					oss << *i;
				}
			}
			if (!res.has_refloc())
				oss << " L:" << res.get_refloc();
			outputchan->write(oss.str() + "\n");
			outputchan->write("Desc: " + tsrule.get_instruction() + "\n");
			if (tsrule.get_condition())
				outputchan->write("Cond: " + tsrule.get_condition()->to_str(reval.get_departuretime()) + "\n");
			for (unsigned int i = 0, n = tsrule.get_restrictions().size(); i < n; ++i) {
				const ADR::RestrictionSequence& rs(tsrule.get_restrictions()[i]);
				const ADR::RestrictionSequenceResult& rsr(res[i]);
				{
					std::ostringstream oss;
					oss << "  Alternative " << i;
					if (!rsr.get_vertexset().empty()) {
						oss << " V:";
						bool subseq(false);
						for (ADR::RestrictionResult::set_t::const_iterator i(rsr.get_vertexset().begin()), e(rsr.get_vertexset().end()); i != e; ++i) {
							if (subseq)
								oss << ',';
							subseq = true;
							oss << *i;
						}
					}
					if (!rsr.get_edgeset().empty()) {
						oss << " E:";
						bool subseq(false);
						for (ADR::RestrictionResult::set_t::const_iterator i(rsr.get_edgeset().begin()), e(rsr.get_edgeset().end()); i != e; ++i) {
							if (subseq)
								oss << ',';
							subseq = true;
							oss << *i;
						}
					}
					outputchan->write(oss.str() + "\n");
				}
				outputchan->write(rs.to_str(reval.get_departuretime()) + "\n");
			}
		}
		reval.set_graph(0);
		outputchan->flush();
		return true;
	}
	FPlanRoute fpl(*(FPlan *)0);
	char type_of_flight('G');
	std::string acft_type;
	std::string equipment;
	Aircraft::pbn_t pbn(Aircraft::pbn_none);
	{
		IcaoFlightPlan icaofpl(*eng);
	        IcaoFlightPlan::errors_t err(icaofpl.parse(line, parseexpand));
		if (!err.empty()) {
			std::string er("EFPL: ");
			bool subseq(false);
			for (IcaoFlightPlan::errors_t::const_iterator i(err.begin()), e(err.end()); i != e; ++i) {
				if (subseq)
					er += "; ";
				subseq = true;
				er += *i;
			}
			outputchan->write(er + "\n");
			outputchan->flush();
			return true;
		}
		icaofpl.set_route(fpl);
		type_of_flight = icaofpl.get_flighttype();
		acft_type = icaofpl.get_aircrafttype();
		equipment = icaofpl.get_equipment();
		pbn = icaofpl.get_pbn();
		if (false) {
			std::ostringstream oss;
			oss << "I: FPL Parsed, " << fpl.get_nrwpt() << " waypoints";
			outputchan->write(oss.str() + "\n");
		}
	}
	// print Parsed Flight Plan
	if (false) {
		for (unsigned int i = 0; i < fpl.get_nrwpt(); ++i) {
			const FPlanWaypoint& wpt(fpl[i]);
			std::ostringstream oss;
			oss << "PFPL[" << i << "]: " << wpt.get_icao() << '/' << wpt.get_name() << ' ';
			switch (wpt.get_pathcode()) {
			case FPlanWaypoint::pathcode_sid:
                        case FPlanWaypoint::pathcode_star:
			case FPlanWaypoint::pathcode_airway:
				oss << wpt.get_pathname();
				break;

                        case FPlanWaypoint::pathcode_directto:
				oss << "DCT";
				break;

			default:
				oss << '-';
				break;
			}
			oss << ' ' << wpt.get_coord().get_lat_str2() << ' ' << wpt.get_coord().get_lon_str2()
			    << " D" << std::fixed << std::setprecision(1) << wpt.get_dist_nmi()
			    << " T" << std::fixed << std::setprecision(0) << wpt.get_truetrack_deg()
			    << ' ' << (wpt.is_standard() ? 'F' : 'A') << std::setw(3) << std::setfill('0') << ((wpt.get_altitude() + 50) / 100)
			    << ' ' << (wpt.is_ifr() ? 'I' : 'V') << "FR";
			outputchan->write(oss.str() + "\n");
		}
	}
	if (false) {
		std::ostringstream oss;
		oss << "FPLDIST: " << fpl.total_distance_nmi_dbl() << "nmi GC " << fpl.gc_distance_nmi_dbl() << "nmi";
		outputchan->write(oss.str() + "\n");
		outputchan->flush();
	}
	TrafficFlowRestrictions::Result res(tfr.check_fplan(fpl, type_of_flight, acft_type, equipment, pbn,
							    airportdb, navaiddb, waypointdb, airwaydb, airspacedb));
	// print Flight Plan
	{
		{
			std::ostringstream oss;
			oss << "FR: " << res.get_aircrafttype() << ' ' << res.get_aircrafttypeclass() << ' '
			    << res.get_equipment() << " PBN/" << res.get_pbn_string() << ' ' << res.get_typeofflight();
			outputchan->write(oss.str() + "\n");
		}
		const TrafficFlowRestrictions::Fpl& fpl(res.get_fplan());
		for (unsigned int i = 0; i < fpl.size(); ++i) {
			const TrafficFlowRestrictions::FplWpt& wpt(fpl[i]);
			std::ostringstream oss;
			oss << "F[" << i << '/' << wpt.get_wptnr() << "]: " << wpt.get_ident() << ' ' << wpt.get_type_string() << ' ';
			switch (wpt.get_pathcode()) {
			case FPlanWaypoint::pathcode_sid:
                        case FPlanWaypoint::pathcode_star:
			case FPlanWaypoint::pathcode_airway:
				oss << wpt.get_pathname();
				break;

                        case FPlanWaypoint::pathcode_directto:
				oss << "DCT";
				break;

			default:
				oss << '-';
				break;
			}
			oss << ' ' << wpt.get_coord().get_lat_str2() << ' ' << wpt.get_coord().get_lon_str2()
			    << " D" << std::fixed << std::setprecision(1) << wpt.get_dist_nmi()
			    << " T" << std::fixed << std::setprecision(0) << wpt.get_truetrack_deg()
			    << ' ' << (wpt.is_standard() ? 'F' : 'A') << std::setw(3) << std::setfill('0') << ((wpt.get_altitude() + 50) / 100)
			    << ' ' << wpt.get_icao() << '/' << wpt.get_name() << ' ' << (wpt.is_ifr() ? 'I' : 'V') << "FR";
			{
				time_t t(wpt.get_time_unix());
				struct tm tm;
				if (gmtime_r(&t, &tm)) {
					oss << ' ' << std::setw(2) << std::setfill('0') << tm.tm_hour
					    << ':' << std::setw(2) << std::setfill('0') << tm.tm_min;
					if (t >= 48 * 60 * 60)
						oss << ' ' << std::setw(4) << std::setfill('0') << (tm.tm_year + 1900)
						    << '-' << std::setw(2) << std::setfill('0') << (tm.tm_mon + 1)
						    << '-' << std::setw(2) << std::setfill('0') << tm.tm_mday;
				}
			}
			outputchan->write(oss.str() + "\n");
		}
	}
	// print Messages
	for (unsigned int i = 0; i < res.messages_size(); ++i)
		print_message(res.get_message(i));
	outputchan->write(res ? "OK\n" : "FAIL\n");
	for (unsigned int i = 0; i < res.size(); ++i) {
		const TrafficFlowRestrictions::RuleResult& rule(res[i]);
		std::ostringstream oss;
		oss << "Rule: " << rule.get_rule() << ' ';
		switch (rule.get_codetype()) {
		case TrafficFlowRestrictions::RuleResult::codetype_mandatory:
			oss << "Mandatory";
			break;

		case TrafficFlowRestrictions::RuleResult::codetype_forbidden:
			oss << "Forbidden";
			break;

		case TrafficFlowRestrictions::RuleResult::codetype_closed:
			oss << "ClosedForCruising";
			break;

		default:
			oss << '?';
			break;
		}
		if (rule.is_dct() || rule.is_unconditional() || rule.is_routestatic() || rule.is_mandatoryinbound() || rule.is_disabled()) {
			oss << " (";
			if(rule.is_disabled())
				oss << '-';
			if(rule.is_dct())
				oss << 'D';
			if (rule.is_unconditional())
				oss << 'U';
			if (rule.is_routestatic())
				oss << 'S';
			if (rule.is_mandatoryinbound())
				oss << 'I';
			oss << ')';
		}
		if (!rule.get_vertexset().empty()) {
			oss << " V:";
			bool subseq(false);
			for (TrafficFlowRestrictions::RuleResult::set_t::const_iterator i(rule.get_vertexset().begin()), e(rule.get_vertexset().end()); i != e; ++i) {
				if (subseq)
					oss << ',';
				subseq = true;
				oss << *i;
			}
		}
		if (!rule.get_edgeset().empty()) {
			oss << " E:";
			bool subseq(false);
			for (TrafficFlowRestrictions::RuleResult::set_t::const_iterator i(rule.get_edgeset().begin()), e(rule.get_edgeset().end()); i != e; ++i) {
				if (subseq)
					oss << ',';
				subseq = true;
				oss << *i;
			}
		}
		if (!rule.get_reflocset().empty()) {
			oss << " L:";
			bool subseq(false);
			for (TrafficFlowRestrictions::Message::set_t::const_iterator i(rule.get_reflocset().begin()), e(rule.get_reflocset().end()); i != e; ++i) {
				if (subseq)
					oss << ',';
				subseq = true;
				oss << *i;
			}
		}
		outputchan->write(oss.str() + "\n");
		outputchan->write("Desc: " + rule.get_desc() + "\n");
		outputchan->write("OprGoal: " + rule.get_oprgoal() + "\n");
		for (unsigned int i = 0; i < rule.size(); ++i) {
			const TrafficFlowRestrictions::RuleResult::Alternative& alt(rule[i]);
			{
				std::ostringstream oss;
				oss << "  Alternative " << i;
				if (!alt.get_vertexset().empty()) {
					oss << " V:";
					bool subseq(false);
					for (TrafficFlowRestrictions::RuleResult::set_t::const_iterator i(alt.get_vertexset().begin()), e(alt.get_vertexset().end()); i != e; ++i) {
						if (subseq)
							oss << ',';
						subseq = true;
						oss << *i;
					}
				}
				if (!alt.get_edgeset().empty()) {
					oss << " E:";
					bool subseq(false);
					for (TrafficFlowRestrictions::RuleResult::set_t::const_iterator i(alt.get_edgeset().begin()), e(alt.get_edgeset().end()); i != e; ++i) {
						if (subseq)
							oss << ',';
						subseq = true;
						oss << *i;
					}
				}
				outputchan->write(oss.str() + "\n");
			}
			for (unsigned int j = 0; j < alt.size(); ++j) {
				const TrafficFlowRestrictions::RuleResult::Alternative::Sequence& el(alt[j]);
				std::ostringstream oss;
				oss << "    " << el.get_type_string();
				switch (el.get_type()) {
				case TrafficFlowRestrictions::RuleResult::Alternative::Sequence::type_airway:
					oss << ' ' << el.get_ident();
					break;

				case TrafficFlowRestrictions::RuleResult::Alternative::Sequence::type_sid:
					oss << ' ' << el.get_ident() << ' ' << el.get_airport();
					break;

				case TrafficFlowRestrictions::RuleResult::Alternative::Sequence::type_star:
					oss << ' ' << el.get_ident() << ' ' << el.get_airport();
					break;

				case TrafficFlowRestrictions::RuleResult::Alternative::Sequence::type_airspace:
					oss << ' ' << el.get_ident() << '/'
					    << AirspacesDb::Airspace::get_class_string(el.get_bdryclass(), AirspacesDb::Airspace::typecode_ead);
					break;

				case TrafficFlowRestrictions::RuleResult::Alternative::Sequence::type_dct:
				case TrafficFlowRestrictions::RuleResult::Alternative::Sequence::type_point:
				default:
					break;
				}
				oss << " F" << std::setw(3) << std::setfill('0') << ((el.get_lower_alt() + 50) / 100);
				if (el.is_lower_alt_valid())
					oss << '*';
				oss << "...F"
				    << std::setw(3) << std::setfill('0') << ((std::min((int32_t)el.get_upper_alt(), (int32_t)99900) + 50) / 100);
				if (el.is_upper_alt_valid())
					oss << '*';
				outputchan->write(oss.str() + "\n");
				if (el.get_type() == TrafficFlowRestrictions::RuleResult::Alternative::Sequence::type_airway ||
				    el.get_type() == TrafficFlowRestrictions::RuleResult::Alternative::Sequence::type_dct ||
				    el.get_type() == TrafficFlowRestrictions::RuleResult::Alternative::Sequence::type_point) {
					const TrafficFlowRestrictions::RuleResult::Alternative::Sequence::RulePoint& pt(el.get_point0());
					std::ostringstream oss;
					oss << "      " << pt.get_ident() << ' ' << pt.get_coord().get_lat_str2() << ' '
					    << pt.get_coord().get_lon_str2() << ' ' << to_str(pt.get_type());
					outputchan->write(oss.str() + "\n");
				}
				if (el.get_type() == TrafficFlowRestrictions::RuleResult::Alternative::Sequence::type_airway ||
				    el.get_type() == TrafficFlowRestrictions::RuleResult::Alternative::Sequence::type_dct) {
					const TrafficFlowRestrictions::RuleResult::Alternative::Sequence::RulePoint& pt(el.get_point1());
					std::ostringstream oss;
					oss << "      " << pt.get_ident() << ' ' << pt.get_coord().get_lat_str2() << ' '
					    << pt.get_coord().get_lon_str2() << ' ' << to_str(pt.get_type());
					outputchan->write(oss.str() + "\n");
				}
			}
		}
	}
        outputchan->flush();
        return true;
}

int main(int argc, char *argv[])
{
        try {
                Glib::init();
                Glib::set_application_name("Flight Plan Checker");
                Glib::thread_init();
		mainloop = Glib::MainLoop::create(Glib::MainContext::get_default());
		bool dis_aux(false), machine(false);

		srand(time(0));

		{
			static struct option long_options[] = {
				{ "maindir", required_argument, 0, 'm' },
				{ "auxdir", required_argument, 0, 'a' },
				{ "disableaux", no_argument, 0, 'A' },
				{ "machineinterface", no_argument, 0, 0x100 },
				{ "expand", no_argument, 0, 0x101 },
				{ "adr", no_argument, 0, 0x102 },
				{ "trace", required_argument, 0, 't' },
				{ "fplvariants", no_argument, 0, 0x103 },
				{0, 0, 0, 0}
			};
			int c, err(0);
			while ((c = getopt_long(argc, argv, "m:a:At:45", long_options, 0)) != EOF) {
				switch (c) {
				case 'm':
					if (optarg)
						dir_main = optarg;
					break;

				case 'a':
					if (optarg)
						dir_aux = optarg;
					break;

				case 'A':
					dis_aux = true;
					break;

				case 0x100:
					machine = true;
					break;

				case 0x101:
					parseexpand = true;
					break;

				case 0x102:
				case '5':
					useadr = true;
					break;

				case '4':
					useadr = false;
					break;

				case 't':
					for (const char *cp = optarg; cp; ) {
						char *cp1(const_cast<char *>(strchr(cp, ',')));
						if (cp1)
							*cp1++ = 0;
						tfr.tracerules_add(Glib::ustring(cp).uppercase());
						cp = cp1;
					}
					break;

				case 0x103:
					fplvariants = true;
					break;

				default:
					err++;
					break;
				}
			}
			if (err) {
				std::cerr << "usage: checkfplan [-m <maindir>] [-a <auxdir>] [-A] [-t <trace>] [-4] [-5] [--fplvariants]" << std::endl;
				return EX_USAGE;
			}
		}
		if (dis_aux)
			auxdbmode = Engine::auxdb_none;
		else if (!dir_aux.empty())
			auxdbmode = Engine::auxdb_override;
		eng = new Engine(dir_main, auxdbmode, dir_aux, false, false);
		dir_aux = eng->get_aux_dir(auxdbmode, dir_aux);
		airportdb.open(dir_main);
		if (!dir_aux.empty() && airportdb.is_dbfile_exists(dir_aux))
			airportdb.attach(dir_aux);
		if (airportdb.is_aux_attached())
			std::cerr << "Auxillary airports database attached" << std::endl;
		navaiddb.open(dir_main);
		if (!dir_aux.empty() && navaiddb.is_dbfile_exists(dir_aux))
			navaiddb.attach(dir_aux);
		if (navaiddb.is_aux_attached())
			std::cerr << "Auxillary navaids database attached" << std::endl;
		waypointdb.open(dir_main);
		if (!dir_aux.empty() && waypointdb.is_dbfile_exists(dir_aux))
			waypointdb.attach(dir_aux);
		if (waypointdb.is_aux_attached())
			std::cerr << "Auxillary waypoints database attached" << std::endl;
		airwaydb.open(dir_main);
		if (!dir_aux.empty() && airwaydb.is_dbfile_exists(dir_aux))
			airwaydb.attach(dir_aux);
		if (airwaydb.is_aux_attached())
			std::cerr << "Auxillary airways database attached" << std::endl;
		airspacedb.open(dir_main);
		if (!dir_aux.empty() && airspacedb.is_dbfile_exists(dir_aux))
			airspacedb.attach(dir_aux);
		if (airspacedb.is_aux_attached())
			std::cerr << "Auxillary airspaces database attached" << std::endl;
		adrdb.set_path(dir_aux);
		if (useadr)
			loadadr();
		else
			loadtfr();
		inputchan = Glib::IOChannel::create_from_fd(0);
		outputchan = Glib::IOChannel::create_from_fd(1);
		inputchan->set_encoding();
		outputchan->set_encoding();
		if (machine) {
			delete eng;
			eng = 0;
			cmdlist["nop"] = cmd_nop;
			cmdlist["quit"] = cmd_quit;
			Glib::signal_io().connect(sigc::ptr_fun(&machine_input), inputchan, Glib::IO_IN | Glib::IO_ERR | Glib::IO_HUP);
			{
				Command cmd("checkfplan");
				cmd.set_option("version", VERSION);
				machine_writecmd(cmd);
			}
			outputchan->flush();
			mainloop->run();
			return EX_OK;
		}
		if (!tfr.tracerules_empty()) {
			std::ostringstream oss;
			for (TrafficFlowRestrictions::tracerules_const_iterator_t i(tfr.tracerules_begin()), e(tfr.tracerules_end()); i != e; ++i)
				oss << ',' << *i;
			outputchan->write("I: Trace Rules: " + oss.str().substr(1) + "\n");
			outputchan->flush();
		}
		Glib::signal_io().connect(sigc::ptr_fun(&normal_input), inputchan, Glib::IO_IN | Glib::IO_ERR | Glib::IO_HUP);
		mainloop->run();
	} catch (const Glib::FileError& ex) {
                std::cerr << "FileError: " << ex.what() << std::endl;
                return EX_DATAERR;
        } catch (const Glib::Exception& ex) {
                std::cerr << "Glib exception: " << ex.what() << std::endl;
                return EX_DATAERR;
        } catch (const std::exception& ex) {
                std::cerr << "exception: " << ex.what() << std::endl;
                return EX_DATAERR;
        }
        return EX_OK;
}
