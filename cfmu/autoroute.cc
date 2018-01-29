#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <getopt.h>
#include <iostream>
#include <iomanip>
#include <glibmm.h>
#include <giomm.h>
#include <stdexcept>
#include <errno.h>

#ifdef HAVE_PWD_H
#include <pwd.h>
#endif

#ifdef HAVE_GRP_H
#include <grp.h>
#endif

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
#include "cfmuautoroute.hh"
#include "cfmuautoroute45.hh"
#include "cfmuautoroute51.hh"
#include "cfmuautorouteproxy.hh"

#ifdef HAVE_JSONCPP
#include "arsockserver.hh"
#endif

class Command : public CFMUAutorouteProxy::Command {
public:
	Command(void) : CFMUAutorouteProxy::Command() {}
	Command(const std::string& cmd) : CFMUAutorouteProxy::Command(cmd) {}

	using CFMUAutorouteProxy::Command::set_option;

	void set_option(const std::string& name, CFMUAutoroute::opttarget_t ot, bool replace = false);
	std::pair<CFMUAutoroute::opttarget_t,bool> get_option_opttarget(const std::string& name) const;
};

static CFMUAutoroute *autoroute(0);
static Glib::RefPtr<Glib::MainLoop> mainloop;
typedef void (*cmd_t)(const Command& cmdin, Command& cmdout);
typedef std::map<std::string,cmd_t> cmdlist_t;
static cmdlist_t cmdlist;
static Glib::RefPtr<Glib::IOChannel> inputchan;
static Glib::RefPtr<Glib::IOChannel> outputchan;
typedef enum {
	timestamp_off,
	timestamp_iso8601,
	timestamp_relms
} timestamp_t;
static timestamp_t timestamp(timestamp_off);
static Glib::TimeVal starttime;

static std::string get_timestamp(void)
{
	switch (timestamp) {
	default:
		return std::string();

	case timestamp_iso8601:
	{
		std::ostringstream oss;
		Glib::TimeVal tv;
		tv.assign_current_time();
		oss << tv.as_iso8601() << ' ';
		return oss.str();
	}

	case timestamp_relms:
	{
		std::ostringstream oss;
		Glib::TimeVal tv;
		tv.assign_current_time();
		tv -= starttime;
		oss << std::fixed << std::setw(6) << std::setprecision(1) << tv.as_double() << ' ';
		return oss.str();
	}
	}
}

void Command::set_option(const std::string& name, CFMUAutoroute::opttarget_t ot, bool replace)
{
	set_option(name, ::to_str(ot), replace);
}

std::pair<CFMUAutoroute::opttarget_t,bool> Command::get_option_opttarget(const std::string& name) const
{
        std::pair<const std::string&,bool> o(get_option(name));
        std::pair<CFMUAutoroute::opttarget_t,bool> ret(CFMUAutoroute::opttarget_time, o.second);
        if (!ret.second)
                return ret;
        static const CFMUAutoroute::opttarget_t ot[] = {
		CFMUAutoroute::opttarget_time,
		CFMUAutoroute::opttarget_fuel,
		CFMUAutoroute::opttarget_preferred
        };
        unsigned int i;
        for (i = 0; i < sizeof(ot)/sizeof(ot[0]); ++i)
                if (::to_str(ot[i]) == o.first)
                        break;
        if (i >= sizeof(ot)/sizeof(ot[0])) {
                error("invalid " + name + "=" + o.first);
                ret.second = false;
        } else {
                ret.first = ot[i];
        }
        return ret;
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
                std::string cmdname("autoroute");
                cmdname = cmdin.get_cmdname();
                if (cmdname.empty())
                        cmdname = "autoroute";
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

static void machine_statuschange(CFMUAutoroute::statusmask_t status)
{
	if (status & (CFMUAutoroute::statusmask_starting |
		      CFMUAutoroute::statusmask_stoppingdone |
		      CFMUAutoroute::statusmask_stoppingerror)) {
		Command cmd("autoroute");
		if (status & CFMUAutoroute::statusmask_starting) {
			cmd.set_option("status", "starting");
		} else if (status & (CFMUAutoroute::statusmask_stoppingdone |
				     CFMUAutoroute::statusmask_stoppingerror)) {
			cmd.set_option("status", "stopping");
			cmd.set_option("routesuccess", (unsigned int)!!(status & CFMUAutoroute::statusmask_stoppingdone));
			cmd.set_option("wallclocktime", autoroute->get_wallclocktime().as_double());
			cmd.set_option("validatortime", autoroute->get_validatortime().as_double());
			cmd.set_option("siderror", (unsigned int)!!(status & CFMUAutoroute::statusmask_stoppingerrorsid));
			cmd.set_option("starerror", (unsigned int)!!(status & CFMUAutoroute::statusmask_stoppingerrorstar));
			cmd.set_option("enrouteerror", (unsigned int)!!(status & CFMUAutoroute::statusmask_stoppingerrorenroute));
			cmd.set_option("validatorerror", (unsigned int)!!(status & CFMUAutoroute::statusmask_stoppingerrorvalidatortimeout));
			cmd.set_option("internalerror", (unsigned int)!!(status & CFMUAutoroute::statusmask_stoppingerrorinternalerror));
			cmd.set_option("iterationerror", (unsigned int)!!(status & CFMUAutoroute::statusmask_stoppingerroriteration));
			cmd.set_option("userstop", (unsigned int)!!(status & CFMUAutoroute::statusmask_stoppingerroruser));
		}
		machine_writecmd(cmd);
		outputchan->flush();
	}
}

static void machine_autoroutelog(CFMUAutoroute::log_t item, const std::string& line)
{
	if (item != CFMUAutoroute::log_fplproposal) {
		Command cmd("log");
		cmd.set_option("item", to_str(item));
		cmd.set_option("text", line);
		machine_writecmd(cmd);
		outputchan->flush();
		return;
	}
	// new flight plan
	const FPlanRoute& route(autoroute->get_route());
	{
		Command cmd("fplbegin");
		cmd.set_option("note", route.get_note());
                cmd.set_option("curwpt", route.get_curwpt());
                cmd.set_option("offblock", route.get_time_offblock_unix());
                cmd.set_option("save_offblock", route.get_save_time_offblock_unix());
                cmd.set_option("onblock", route.get_time_onblock_unix());
                cmd.set_option("save_onblock", route.get_save_time_onblock_unix());
		machine_writecmd(cmd);
	}
	for (unsigned int i = 0; i < route.get_nrwpt(); ++i) {
		const FPlanWaypoint& wpt(route[i]);
		Command cmd("fplwpt");
                cmd.set_option("icao", wpt.get_icao());
                cmd.set_option("name", wpt.get_name());
                cmd.set_option("pathname", wpt.get_pathname());
		cmd.set_option("pathcode", wpt.get_pathcode_name());
                cmd.set_option("note", wpt.get_note());
                cmd.set_option("time", wpt.get_time_unix());
                cmd.set_option("save_time", wpt.get_save_time_unix());
                cmd.set_option("flighttime", wpt.get_flighttime());
                cmd.set_option("frequency", wpt.get_frequency());
                cmd.set_option("coord", wpt.get_coord());
                cmd.set_option("lat", wpt.get_coord().get_lat_deg_dbl());
                cmd.set_option("lon", wpt.get_coord().get_lon_deg_dbl());
                cmd.set_option("altitude", wpt.get_altitude());
                cmd.set_option("flags", wpt.get_flags());
		cmd.set_option("winddir", wpt.get_winddir_deg());
		cmd.set_option("windspeed", wpt.get_windspeed_kts());
		cmd.set_option("qff", wpt.get_qff_hpa());
		cmd.set_option("isaoffset", wpt.get_isaoffset_kelvin());
		cmd.set_option("truealt", wpt.get_truealt());
   		cmd.set_option("dist", wpt.get_dist_nmi());
   		cmd.set_option("fuel", wpt.get_fuel_usg());
   		cmd.set_option("tt", wpt.get_truetrack_deg());
   		cmd.set_option("th", wpt.get_trueheading_deg());
   		cmd.set_option("decl", wpt.get_declination_deg());
   		cmd.set_option("tas", wpt.get_tas_kts());
   		cmd.set_option("rpm", wpt.get_rpm());
   		cmd.set_option("mp", wpt.get_mp_inhg());
   		cmd.set_option("type", wpt.get_type_string());
		machine_writecmd(cmd);
	}
	{
		Command cmd("fplend");
		cmd.set_option("gcdist", autoroute->get_gcdistance());
		cmd.set_option("routedist", autoroute->get_routedistance());
		cmd.set_option("mintime", autoroute->get_mintime(true));
		cmd.set_option("routetime", autoroute->get_routetime(true));
		cmd.set_option("minfuel", autoroute->get_minfuel(true));
		cmd.set_option("routefuel", autoroute->get_routefuel(true));
		cmd.set_option("mintimezerowind", autoroute->get_mintime(false));
		cmd.set_option("routetimezerowind", autoroute->get_routetime(false));
		cmd.set_option("minfuelzerowind", autoroute->get_minfuel(false));
		cmd.set_option("routefuelzerowind", autoroute->get_routefuel(false));
		cmd.set_option("fpl", autoroute->get_simplefpl());
		cmd.set_option("iteration", autoroute->get_iterationcount());
		cmd.set_option("localiteration", autoroute->get_local_iterationcount());
		cmd.set_option("remoteiteration", autoroute->get_remote_iterationcount());
		cmd.set_option("wallclocktime", autoroute->get_wallclocktime().as_double());
		cmd.set_option("validatortime", autoroute->get_validatortime().as_double());
		machine_writecmd(cmd);
	}
	outputchan->flush();
}

static void cmd_nop(const Command& cmdin, Command& cmdout)
{
}

static void cmd_quit(const Command& cmdin, Command& cmdout)
{
	mainloop->quit();
}

static void cmd_departure(const Command& cmdin, Command& cmdout)
{
	{
		std::pair<const std::string&,bool> icao(cmdin.get_option("icao"));
		std::pair<const std::string&,bool> name(cmdin.get_option("name"));
		if (icao.second || name.second)	{
			if (!autoroute->set_departure(icao.first, name.first))
				throw std::runtime_error("departure aerodrome " + icao.first + " " + name.first + " not found");
		}
	}
	if (cmdin.is_option("ifr"))
		autoroute->set_departure_ifr(true);
	else if (cmdin.is_option("vfr"))
		autoroute->set_departure_ifr(false);
	{
		std::pair<Point,bool> pt(cmdin.get_option_coord("sid"));
		if (pt.second) {
			autoroute->set_sid(pt.first);
		} else {
			std::pair<const std::string&,bool> ident(cmdin.get_option("sidident"));
			if (ident.second)
				autoroute->set_sid(ident.first);
		}
	}
	{
		std::pair<double,bool> lim(cmdin.get_option_float("sidlimit"));
		if (lim.second)
			autoroute->set_sidlimit(lim.first);
	}
	{
		std::pair<double,bool> p(cmdin.get_option_float("sidpenalty"));
		if (p.second)
			autoroute->set_sidpenalty(p.first);
	}
	{
		std::pair<double,bool> o(cmdin.get_option_float("sidoffset"));
		if (o.second)
			autoroute->set_sidoffset(o.first);
	}
	{
		std::pair<double,bool> m(cmdin.get_option_float("sidminimum"));
		if (m.second)
			autoroute->set_sidminimum(m.first);
	}
	{
		std::pair<unsigned long,bool> db(cmdin.get_option_uint("siddb"));
		if (db.second)
			autoroute->set_siddb(!!db.first);
	}
	{
		std::pair<unsigned long,bool> o(cmdin.get_option_uint("sidonly"));
		if (o.second)
			autoroute->set_sidonly(!!o.first);
	}
	{
		std::pair<Command::stringarray_t,bool> f(cmdin.get_option_stringarray("sidfilter"));
		if (f.second) {
			CFMUAutoroute::sidstarfilter_t fi;
			fi.insert(f.first.begin(), f.first.end());
			autoroute->set_sidfilter(fi);
		}
	}
	{
		std::pair<unsigned long,bool> dtime(cmdin.get_option_uint("time"));
		if (dtime.second)
			autoroute->set_deptime(dtime.first);
	}
	{
		std::pair<std::string,bool> dt(cmdin.get_option("date"));
		if (dt.second) {
			Glib::TimeVal tv;
			if (tv.assign_from_iso8601(dt.first))
				autoroute->set_deptime(tv.tv_sec);
		}
	}
	if (autoroute->get_departure().is_valid()) {
		cmdout.set_option("icao", autoroute->get_departure().get_icao());
		cmdout.set_option("name", autoroute->get_departure().get_name());
		cmdout.set_option("coord", autoroute->get_departure().get_coord());
	}
	if (autoroute->get_sid().is_invalid()) {
		cmdout.set_option("sid", "");
	} else {
		cmdout.set_option("sid", autoroute->get_sid());
		cmdout.set_option("sidtype", to_str(autoroute->get_sidtype()));
		cmdout.set_option("sidident", autoroute->get_sidident());
	}
	cmdout.set_option("sidlimit", autoroute->get_sidlimit());
	cmdout.set_option("sidpenalty", autoroute->get_sidpenalty());
	cmdout.set_option("sidoffset", autoroute->get_sidoffset());
	cmdout.set_option("sidminimum", autoroute->get_sidminimum());
	cmdout.set_option("siddb", (unsigned int)!!autoroute->get_siddb());
	cmdout.set_option("sidonly", (unsigned int)!!autoroute->get_sidonly());
	{
		const CFMUAutoroute::sidstarfilter_t& fi(autoroute->get_sidfilter());
		cmdout.set_option("sidfilter", fi.begin(), fi.end());
	}
	cmdout.set_option(autoroute->get_departure_ifr() ? "ifr" : "vfr", "");
	cmdout.set_option("time", autoroute->get_deptime());
	{
		Glib::TimeVal tv(autoroute->get_deptime(), 0);
		cmdout.set_option("date", tv.as_iso8601());
	}
}

static void cmd_destination(const Command& cmdin, Command& cmdout)
{
	{
		std::pair<const std::string&,bool> icao(cmdin.get_option("icao"));
		std::pair<const std::string&,bool> name(cmdin.get_option("name"));
		if (icao.second || name.second)	{
			if (!autoroute->set_destination(icao.first, name.first))
				throw std::runtime_error("destination aerodrome " + icao.first + " " + name.first + " not found");
		}
	}
	if (cmdin.is_option("ifr"))
		autoroute->set_destination_ifr(true);
	else if (cmdin.is_option("vfr"))
		autoroute->set_destination_ifr(false);
	{
		std::pair<Point,bool> pt(cmdin.get_option_coord("star"));
		if (pt.second) {
			autoroute->set_star(pt.first);
		} else {
			std::pair<const std::string&,bool> ident(cmdin.get_option("starident"));
			if (ident.second)
				autoroute->set_star(ident.first);
		}
	}
	{
		std::pair<double,bool> lim(cmdin.get_option_float("starlimit"));
		if (lim.second)
			autoroute->set_starlimit(lim.first);
	}
	{
		std::pair<double,bool> p(cmdin.get_option_float("starpenalty"));
		if (p.second)
			autoroute->set_starpenalty(p.first);
	}
	{
		std::pair<double,bool> o(cmdin.get_option_float("staroffset"));
		if (o.second)
			autoroute->set_staroffset(o.first);
	}
	{
		std::pair<double,bool> m(cmdin.get_option_float("starminimum"));
		if (m.second)
			autoroute->set_starminimum(m.first);
	}
	{
		std::pair<unsigned long,bool> db(cmdin.get_option_uint("stardb"));
		if (db.second)
			autoroute->set_stardb(!!db.first);
	}
	{
		std::pair<unsigned long,bool> o(cmdin.get_option_uint("staronly"));
		if (o.second)
			autoroute->set_staronly(!!o.first);
	}
	{
		std::pair<Command::stringarray_t,bool> f(cmdin.get_option_stringarray("starfilter"));
		if (f.second) {
			CFMUAutoroute::sidstarfilter_t fi;
			fi.insert(f.first.begin(), f.first.end());
			autoroute->set_starfilter(fi);
		}
	}
	{
		std::pair<const std::string&,bool> alt(cmdin.get_option("alternate1"));
		if (alt.second)
			autoroute->set_alternate(0, alt.first);
	}
	{
		std::pair<const std::string&,bool> alt(cmdin.get_option("alternate2"));
		if (alt.second)
			autoroute->set_alternate(1, alt.first);
	}
	if (autoroute->get_destination().is_valid()) {
		cmdout.set_option("icao", autoroute->get_destination().get_icao());
		cmdout.set_option("name", autoroute->get_destination().get_name());
		cmdout.set_option("coord", autoroute->get_destination().get_coord());
	}
	if (autoroute->get_star().is_invalid()) {
		cmdout.set_option("star", "");
	} else {
		cmdout.set_option("star", autoroute->get_star());
		cmdout.set_option("startype", to_str(autoroute->get_startype()));
		cmdout.set_option("starident", autoroute->get_starident());
	}
	cmdout.set_option("starlimit", autoroute->get_starlimit());
	cmdout.set_option("starpenalty", autoroute->get_starpenalty());
	cmdout.set_option("staroffset", autoroute->get_staroffset());
	cmdout.set_option("starminimum", autoroute->get_starminimum());
	cmdout.set_option("stardb", (unsigned int)!!autoroute->get_stardb());
	cmdout.set_option("staronly", (unsigned int)!!autoroute->get_staronly());
	{
		const CFMUAutoroute::sidstarfilter_t& fi(autoroute->get_starfilter());
		cmdout.set_option("starfilter", fi.begin(), fi.end());
	}
	cmdout.set_option(autoroute->get_destination_ifr() ? "ifr" : "vfr", "");
	if (!autoroute->get_alternate(0).empty())
		cmdout.set_option("alternate1", autoroute->get_alternate(0));
	if (!autoroute->get_alternate(1).empty())
		cmdout.set_option("alternate2", autoroute->get_alternate(1));
}

static void cmd_crossing(const Command& cmdin, Command& cmdout)
{
	{
		std::pair<unsigned long,bool> o(cmdin.get_option_uint("count"));
		if (o.second)
			autoroute->set_crossing_size(o.first);
	}
	std::pair<unsigned long,bool> index(cmdin.get_option_uint("index"));
	if (index.second) {
		std::pair<Point,bool> pt(cmdin.get_option_coord("coord"));
		if (pt.second) {
			autoroute->set_crossing(index.first, pt.first);
		} else {
			std::pair<const std::string&,bool> ident(cmdin.get_option("ident"));
			if (ident.second)
				autoroute->set_crossing(index.first, ident.first);
		}
		std::pair<double,bool> radius(cmdin.get_option_float("radius"));
		if (radius.second)
			autoroute->set_crossing_radius(index.first, radius.first);
		std::pair<long,bool> minlevel(cmdin.get_option_int("minlevel"));
		if (!minlevel.second)
			minlevel.first = autoroute->get_crossing_minlevel(index.first);
		std::pair<long,bool> maxlevel(cmdin.get_option_int("maxlevel"));
		if (!maxlevel.second)
			maxlevel.first = autoroute->get_crossing_maxlevel(index.first);
		if (minlevel.second || maxlevel.second)
			autoroute->set_crossing_level(index.first, minlevel.first, maxlevel.first);
	} else {
		index.first = 0;
	}
	cmdout.set_option("count", autoroute->get_crossing_size());
	if (index.first < autoroute->get_crossing_size()) {
		cmdout.set_option("coord", autoroute->get_crossing(index.first));
		cmdout.set_option("type", to_str(autoroute->get_crossing_type(index.first)));
		cmdout.set_option("ident", autoroute->get_crossing_ident(index.first));
		cmdout.set_option("radius", autoroute->get_crossing_radius(index.first));
		cmdout.set_option("minlevel", autoroute->get_crossing_minlevel(index.first));
		cmdout.set_option("maxlevel", autoroute->get_crossing_maxlevel(index.first));
	}
}

static void cmd_enroute(const Command& cmdin, Command& cmdout)
{
	{
		std::pair<double,bool> lim(cmdin.get_option_float("dctlimit"));
		if (lim.second)
			autoroute->set_dctlimit(lim.first);
	}
	{
		std::pair<double,bool> p(cmdin.get_option_float("dctpenalty"));
		if (p.second)
			autoroute->set_dctpenalty(p.first);
	}
	{
		std::pair<double,bool> o(cmdin.get_option_float("dctoffset"));
		if (o.second)
			autoroute->set_dctoffset(o.first);
	}
	{
		std::pair<double,bool> lim(cmdin.get_option_float("vfraspclimit"));
		if (lim.second)
			autoroute->set_vfraspclimit(lim.first);
	}
	{
		std::pair<unsigned long,bool> o(cmdin.get_option_uint("forceenrouteifr"));
		if (o.second)
			autoroute->set_force_enroute_ifr(!!o.first);
	}
	{
		std::pair<unsigned long,bool> o(cmdin.get_option_uint("honourawylevels"));
		if (o.second)
			autoroute->set_honour_awy_levels(!!o.first);
	}
	{
		std::pair<unsigned long,bool> o(cmdin.get_option_uint("honourprofilerules"));
		if (o.second)
			autoroute->set_honour_profilerules(!!o.first);
	}
	cmdout.set_option("dctlimit", autoroute->get_dctlimit());
	cmdout.set_option("dctpenalty", autoroute->get_dctpenalty());
	cmdout.set_option("dctoffset", autoroute->get_dctoffset());
	cmdout.set_option("vfraspclimit", autoroute->get_vfraspclimit());
	cmdout.set_option("forceenrouteifr", (unsigned int)!!autoroute->get_force_enroute_ifr());
	cmdout.set_option("honourawylevels", (unsigned int)!!autoroute->get_honour_awy_levels());
	cmdout.set_option("honourprofilerules", (unsigned int)!!autoroute->get_honour_profilerules());
}

static void cmd_levels(const Command& cmdin, Command& cmdout)
{
	{
		std::pair<long,bool> ob(cmdin.get_option_int("base"));
		std::pair<long,bool> ot(cmdin.get_option_int("top"));
		autoroute->set_levels(ob.second ? ob.first : autoroute->get_baselevel(),
				     ot.second ? ot.first : autoroute->get_toplevel());
	}
	{
		std::pair<double,bool> md(cmdin.get_option_float("maxdescent"));
		if (md.second)
			autoroute->set_maxdescent(md.first);
	}
	{
		std::pair<unsigned long,bool> o(cmdin.get_option_uint("honourlevelchangetrackmiles"));
		if (o.second)
			autoroute->set_honour_levelchangetrackmiles(!!o.first);
	}
	{
		std::pair<unsigned long,bool> o(cmdin.get_option_uint("honouropsperftrackmiles"));
		if (o.second)
			autoroute->set_honour_opsperftrackmiles(!!o.first);
	}
	cmdout.set_option("base", autoroute->get_baselevel());
	cmdout.set_option("top", autoroute->get_toplevel());
	cmdout.set_option("maxdescent", autoroute->get_maxdescent());
	cmdout.set_option("honourlevelchangetrackmiles", (unsigned int)!!autoroute->get_honour_levelchangetrackmiles());
	cmdout.set_option("honouropsperftrackmiles", (unsigned int)!!autoroute->get_honour_opsperftrackmiles());
}

static void cmd_exclude(const Command& cmdin, Command& cmdout)
{
	{
		std::pair<unsigned long,bool> o(cmdin.get_option_uint("clear"));
		if (o.second && o.first)
			autoroute->clear_excluderegions();
	}
	std::pair<unsigned long,bool> index(cmdin.get_option_uint("index"));
	{
		std::pair<long,bool> ob(cmdin.get_option_int("base"));
		std::pair<long,bool> ot(cmdin.get_option_int("top"));
		std::pair<double,bool> awylimit(cmdin.get_option_float("awylimit"));
		std::pair<double,bool> dctlimit(cmdin.get_option_float("dctlimit"));
		std::pair<double,bool> dctoffset(cmdin.get_option_float("dctoffset"));
		std::pair<double,bool> dctscale(cmdin.get_option_float("dctscale"));
		if (!ob.second)
			ob.first = 0;
		if (!ot.second)
			ot.first = 999;
		if (!awylimit.second)
			awylimit.first = 0;
		if (!dctlimit.second)
			dctlimit.first = 0;
		if (!dctoffset.second)
			dctoffset.first = 0;
		if (!dctscale.second)
			dctscale.first = 1;
		std::pair<const std::string&,bool> aspcid(cmdin.get_option("aspcid"));
		std::pair<const std::string&,bool> aspctype(cmdin.get_option("aspctype"));
		std::pair<Point,bool> ptsw(cmdin.get_option_coord("sw"));
		std::pair<Point,bool> ptne(cmdin.get_option_coord("ne"));
		if (aspcid.second) {
			autoroute->add_excluderegion(CFMUAutoroute::ExcludeRegion(aspcid.first, aspctype.second ? aspctype.first : "",
										  ob.first, ot.first, awylimit.first, dctlimit.first,
										  dctoffset.first, dctscale.first));
			if (!index.second) {
				index.first = autoroute->get_excluderegions().size() - 1;
				index.second = true;
			}
		}
		if (ptsw.second && ptne.second) {
			autoroute->add_excluderegion(CFMUAutoroute::ExcludeRegion(Rect(ptsw.first, ptne.first),
										  ob.first, ot.first, awylimit.first, dctlimit.first,
										  dctoffset.first, dctscale.first));
			if (!index.second) {
				index.first = autoroute->get_excluderegions().size() - 1;
				index.second = true;
			}
		}
	}
	cmdout.set_option("count", autoroute->get_excluderegions().size());
	if (index.second && index.first < autoroute->get_excluderegions().size()) {
		const CFMUAutoroute::ExcludeRegion& er(autoroute->get_excluderegions()[index.first]);
		cmdout.set_option("base", er.get_minlevel());
		cmdout.set_option("top", er.get_maxlevel());
		cmdout.set_option("awylimit", er.get_awylimit());
		cmdout.set_option("dctlimit", er.get_dctlimit());
		cmdout.set_option("dctoffset", er.get_dctoffset());
		cmdout.set_option("dctscale", er.get_dctscale());
		if (er.is_airspace()) {
			cmdout.set_option("aspcid", er.get_airspace_id());
			cmdout.set_option("aspctype", er.get_airspace_type());
		} else {
			cmdout.set_option("sw", er.get_bbox().get_southwest());
			cmdout.set_option("ne", er.get_bbox().get_northeast());
		}
	}

}

static void cmd_tfr(const Command& cmdin, Command& cmdout)
{
	{
		std::pair<unsigned long,bool> o(cmdin.get_option_uint("enabled"));
		if (o.second)
			autoroute->set_tfr_enabled(!!o.first);
	}
	{
		std::pair<const std::string&,bool> o(cmdin.get_option("trace"));
		if (o.second)
			autoroute->set_tfr_trace(o.first);
	}
	{
		std::pair<const std::string&,bool> o(cmdin.get_option("disable"));
		if (o.second)
			autoroute->set_tfr_disable(o.first);
	}
	{
		std::pair<unsigned long,bool> o(cmdin.get_option_uint("precompgraph"));
		if (o.second)
			autoroute->set_precompgraph_enabled(!!o.first);
	}
	{
		std::pair<const std::string&,bool> o(cmdin.get_option("validator"));
		if (o.second) {
			if (o.first == "default")
				autoroute->set_validator(CFMUAutoroute::validator_default);
			else if (o.first == "cfmu")
				autoroute->set_validator(CFMUAutoroute::validator_cfmu);
			else if (o.first == "eurofpl")
				autoroute->set_validator(CFMUAutoroute::validator_eurofpl);
		}
	}
	{
		std::pair<unsigned long,bool> it(cmdin.get_option_uint("maxlocaliterations"));
		if (it.second)
			autoroute->set_maxlocaliteration(it.first);
	}
	{
		std::pair<unsigned long,bool> it(cmdin.get_option_uint("maxremoteiterations"));
		if (it.second)
			autoroute->set_maxremoteiteration(it.first);
	}
	cmdout.set_option("enabled", (unsigned int)!!autoroute->get_tfr_enabled());
	cmdout.set_option("available", (unsigned int)!!autoroute->get_tfr_available());
	cmdout.set_option("trace", autoroute->get_tfr_trace());
	cmdout.set_option("disable", autoroute->get_tfr_disable());
	cmdout.set_option("precompgraph", (unsigned int)!!autoroute->get_precompgraph_enabled());
	switch (autoroute->get_validator()) {
	case CFMUAutoroute::validator_default:
		cmdout.set_option("validator", "default");
		break;

	case CFMUAutoroute::validator_cfmu:
		cmdout.set_option("validator", "cfmu");
		break;

	case CFMUAutoroute::validator_eurofpl:
		cmdout.set_option("validator", "eurofpl");
		break;

	default:
		break;
	}
	cmdout.set_option("maxlocaliterations", autoroute->get_maxlocaliteration());
	cmdout.set_option("maxremoteiterations", autoroute->get_maxremoteiteration());
}

static void cmd_atmosphere(const Command& cmdin, Command& cmdout)
{
	{
		std::pair<double,bool> o(cmdin.get_option_float("qnh"));
		if (o.second)
			autoroute->set_qnh(o.first);
	}
	{
		std::pair<double,bool> o(cmdin.get_option_float("isa"));
		if (o.second)
			autoroute->set_isaoffs(o.first);
	}
	{
		std::pair<unsigned long,bool> o(cmdin.get_option_uint("wind"));
		if (o.second)
			autoroute->set_wind_enabled(!!o.first);
	}
	cmdout.set_option("qnh", autoroute->get_qnh());
	cmdout.set_option("isa", autoroute->get_isaoffs());
	cmdout.set_option("wind", (unsigned int)!!autoroute->get_wind_enabled());
}

static void cmd_cruise(const Command& cmdin, Command& cmdout)
{
	{
		std::pair<double,bool> o(cmdin.get_option_float("rpm"));
		if (o.second)
			autoroute->set_engine_rpm(o.first);
	}
	{
		std::pair<double,bool> o(cmdin.get_option_float("mp"));
		if (o.second)
			autoroute->set_engine_mp(o.first);
	}
	{
		std::pair<double,bool> o(cmdin.get_option_float("bhp"));
		if (o.second)
			autoroute->set_engine_bhp(o.first);
	}
	cmdout.set_option("rpm", autoroute->get_engine_rpm());
	cmdout.set_option("mp", autoroute->get_engine_mp());
	cmdout.set_option("bhp", autoroute->get_engine_bhp());
}

static void cmd_optimization(const Command& cmdin, Command& cmdout)
{
	{
		std::pair<CFMUAutoroute::opttarget_t,bool> ot(cmdin.get_option_opttarget("target"));
		if (ot.second)
			autoroute->set_opttarget(ot.first);
	}
	cmdout.set_option("target", to_str(autoroute->get_opttarget()));
}

static void cmd_preferred(const Command& cmdin, Command& cmdout)
{
	{
		std::pair<long,bool> o(cmdin.get_option_int("level"));
		if (o.second)
			autoroute->set_preferred_level(o.first);
	}
	{
		std::pair<double,bool> o(cmdin.get_option_float("penalty"));
		if (o.second)
			autoroute->set_preferred_penalty(o.first);
	}
	{
		std::pair<double,bool> o(cmdin.get_option_float("climb"));
		if (o.second)
			autoroute->set_preferred_climb(o.first);
	}
	{
		std::pair<double,bool> o(cmdin.get_option_float("descent"));
		if (o.second)
			autoroute->set_preferred_descent(o.first);
	}
	cmdout.set_option("level", autoroute->get_preferred_level());
	cmdout.set_option("penalty", autoroute->get_preferred_penalty());
	cmdout.set_option("climb", autoroute->get_preferred_climb());
	cmdout.set_option("descent", autoroute->get_preferred_descent());
}

static void cmd_aircraft(const Command& cmdin, Command& cmdout)
{
	bool dirty(false);
	std::pair<const std::string&,bool> file(cmdin.get_option("file"));
	if (file.second) {
		bool status(!!autoroute->get_aircraft().load_file(file.first));
		cmdout.set_option("status", (int)status);
		dirty = status && (autoroute->get_aircraft().get_dist().recalculatepoly(false) ||
				   autoroute->get_aircraft().get_climb().recalculatepoly(false) ||
				   autoroute->get_aircraft().get_descent().recalculatepoly(false));
	}
	std::pair<const std::string&,bool> data(cmdin.get_option("data"));
	if (data.second) {
		bool status(!!autoroute->get_aircraft().load_string(data.first));
		cmdout.set_option("status", (int)status);
		dirty = status && (autoroute->get_aircraft().get_dist().recalculatepoly(false) ||
				   autoroute->get_aircraft().get_climb().recalculatepoly(false) ||
				   autoroute->get_aircraft().get_descent().recalculatepoly(false));
	}
	std::pair<const std::string&,bool> registration(cmdin.get_option("registration"));
	if (registration.second)
		autoroute->get_aircraft().set_callsign(registration.first);
	std::pair<const std::string&,bool> type(cmdin.get_option("type"));
	if (type.second)
		autoroute->get_aircraft().set_icaotype(type.first);
	std::pair<const std::string&,bool> equipment(cmdin.get_option("equipment"));
	if (equipment.second)
		autoroute->get_aircraft().set_equipment(equipment.first);
	std::pair<const std::string&,bool> transponder(cmdin.get_option("transponder"));
	if (transponder.second)
		autoroute->get_aircraft().set_transponder(transponder.first);
	std::pair<const std::string&,bool> pbn(cmdin.get_option("pbn"));
	if (pbn.second)
		autoroute->get_aircraft().set_pbn(pbn.first);
	if (equipment.second || pbn.second)
		autoroute->get_aircraft().pbn_fix_equipment();
	cmdout.set_option("registration", autoroute->get_aircraft().get_callsign());
	cmdout.set_option("manufacturer", autoroute->get_aircraft().get_manufacturer());
	cmdout.set_option("model", autoroute->get_aircraft().get_model());
	cmdout.set_option("year", autoroute->get_aircraft().get_year());
	cmdout.set_option("description", autoroute->get_aircraft().get_description());
	cmdout.set_option("type", autoroute->get_aircraft().get_icaotype());
	cmdout.set_option("equipment", autoroute->get_aircraft().get_equipment_string());
	cmdout.set_option("transponder", autoroute->get_aircraft().get_transponder_string());
	cmdout.set_option("pbn", autoroute->get_aircraft().get_pbn_string());
	cmdout.set_option("data", autoroute->get_aircraft().save_string());
	cmdout.set_option("dirty", (int)dirty);
}

static void cmd_start(const Command& cmdin, Command& cmdout)
{
	autoroute->start();
}

static void cmd_stop(const Command& cmdin, Command& cmdout)
{
	autoroute->stop(CFMUAutoroute::statusmask_stoppingerroruser);
}

static void cmd_continue(const Command& cmdin, Command& cmdout)
{
	autoroute->cont();
}

static void cmd_preload(const Command& cmdin, Command& cmdout)
{
	autoroute->preload();
}

static void cmd_clear(const Command& cmdin, Command& cmdout)
{
	autoroute->clear();
}

static void statuschange(CFMUAutoroute::statusmask_t status)
{
	if (false && status & CFMUAutoroute::statusmask_newfpl) {
		std::cout << get_timestamp() << ">> " << autoroute->get_simplefpl() << std::endl;
	}
	if (false && status & CFMUAutoroute::statusmask_newvalidateresponse) {
		const CFMUAutoroute::validationresponse_t& vr(autoroute->get_validationresponse());
		for (CFMUAutoroute::validationresponse_t::const_iterator i(vr.begin()), e(vr.end()); i != e; ++i) {
			std::cout << get_timestamp() << "<< " << *i << std::endl;
		}
	}
	if (status & CFMUAutoroute::statusmask_starting) {
		std::cerr << get_timestamp() << "Starting..." << std::endl;
	}
	if (status & CFMUAutoroute::statusmask_stoppingdone) {
		std::cout << get_timestamp()
			  << "DONE: " << autoroute->get_simplefpl()
			  << " GC " << Conversions::dist_str(autoroute->get_gcdistance())
			  << " route " << Conversions::dist_str(autoroute->get_routedistance())
			  << " fuel " << std::fixed << std::setprecision(1) << autoroute->get_routefuel()
			  << std::endl;
		autoroute->clear();
		mainloop->quit();
		exit(EX_OK);
	}
	if (status & CFMUAutoroute::statusmask_stoppingerror) {
		std::cout << get_timestamp() << "ERROR" << std::endl;
		autoroute->clear();
		mainloop->quit();
		exit(EX_UNAVAILABLE);
	}
}

static void autoroutelog(CFMUAutoroute::log_t item, const std::string& line)
{
	switch (item) {
	case CFMUAutoroute::log_fplproposal:
	{
		std::ostringstream oss;
		unsigned int mintime(autoroute->get_mintime());
		unsigned int rtetime(autoroute->get_routetime());
		oss << "# GC Distance " << std::fixed << std::setprecision(1) << autoroute->get_gcdistance()
		    << " Fuel " << std::fixed << std::setprecision(1) << autoroute->get_minfuel()
		    << " Time " << (mintime / 3600U) << ':' << std::setw(2) << std::setfill('0') << ((mintime / 60U) % 60U)
		    << ':' << std::setw(2) << std::setfill('0') << (mintime % 60U)
		    << " Route Distance " << std::fixed << std::setprecision(1) << autoroute->get_routedistance()
		    << " Fuel " << std::fixed << std::setprecision(1) << autoroute->get_routefuel()
		    << " Time " << (rtetime / 3600U) << ':' << std::setw(2) << std::setfill('0') << ((rtetime / 60U) % 60U)
		    << ':' << std::setw(2) << std::setfill('0') << (rtetime % 60U)
		    << " Iteration " << autoroute->get_local_iterationcount()
		    << '/' << autoroute->get_remote_iterationcount() << std::endl;
		std::cout << get_timestamp() << ">> " << line << std::endl << oss.str();
		break;
	}

	case CFMUAutoroute::log_fpllocalvalidation:
	case CFMUAutoroute::log_fplremotevalidation:
		if (line.empty())
			break;
		std::cout << get_timestamp() << "<< " << line << std::endl;
		break;

	case CFMUAutoroute::log_graphrule:
		std::cout << get_timestamp() << "* " << line << std::endl;
		break;

	case CFMUAutoroute::log_graphruledesc:
	case CFMUAutoroute::log_graphruleoprgoal:
		std::cout << get_timestamp() << "*  " << line << std::endl;
		break;

	case CFMUAutoroute::log_graphchange:
		std::cout << get_timestamp() << "*   " << line << std::endl;
		break;

	case CFMUAutoroute::log_precompgraph:
		std::cout << get_timestamp() << "Precomputed Graph File: " << line << std::endl;
		break;

	case CFMUAutoroute::log_weatherdata:
		std::cout << get_timestamp() << "Wx: " << line << std::endl;
		break;

	case CFMUAutoroute::log_normal:
	case CFMUAutoroute::log_debug0:
	case CFMUAutoroute::log_debug1:
	default:
		std::cerr << get_timestamp() << line << std::endl;
		break;
	};
}

static void signalterminate(int signr)
{
	if (mainloop)
		mainloop->quit();
}

namespace {
	class Crossing {
	public:
		Crossing(const std::string& name = "", double radius = 0, int minlevel = 0, int maxlevel = 600)
			: m_name(name), m_radius(radius), m_minlevel(minlevel), m_maxlevel(maxlevel) {}
		const std::string& get_name(void) const { return m_name; }
		double get_radius(void) const { return m_radius; }
		int get_minlevel(void) const { return m_minlevel; }
		int get_maxlevel(void) const { return m_maxlevel; }

	protected:
		std::string m_name;
		double m_radius;
		int m_minlevel;
		int m_maxlevel;
	};
};

int main(int argc, char *argv[])
{
        try {
                Glib::init();
		Gio::init();
                Glib::set_application_name("CFMU Autoroute");
                Glib::thread_init();
		mainloop = Glib::MainLoop::create(Glib::MainContext::get_default());
		if (Glib::file_test(PACKAGE_DATA_DIR "/adr.db", Glib::FILE_TEST_EXISTS) &&
		    !Glib::file_test(PACKAGE_DATA_DIR "/adr.db", Glib::FILE_TEST_IS_DIR))
			autoroute = new CFMUAutoroute51();
		else
			autoroute = new CFMUAutoroute45();
		std::string dir_main(""), dir_aux("");
		std::string sidname, starname;
		Engine::auxdb_mode_t auxdbmode(Engine::auxdb_prefs);
		bool dis_aux(false), tfrenable(true), precompute(false), setengparams(false);
		typedef enum {
			mode_commandline,
			mode_machineinterface,
			mode_socketserver
		} mode_t;
		mode_t mode(mode_commandline);
		double crossingradius(0);
		int crossingminlevel(0);
		int crossingmaxlevel(600);
		std::vector<Crossing> crossings;
		int excludeminlevel(0);
		int excludemaxlevel(600);
		double excludeawylimit(0);
		double excludedctlimit(0);
		double excludedctoffset(0);
		double excludedctscale(1);
#ifdef HAVE_JSONCPP
		unsigned int routerlimit(~0U);
		unsigned int routeractivelimit(~0U);
		unsigned int routertimeout(~0U);
		unsigned int routermaxruntime(~0U);
		std::string sockserverpath(PACKAGE_RUN_DIR "/autoroute/socket");
		uid_t sockservuid(0);
		gid_t sockservgid(0);
		uid_t socketuid(getuid());
		gid_t socketgid(getgid());
		::mode_t socketmode(S_IRUSR | S_IWUSR | S_IXUSR | S_IRGRP | S_IWGRP | S_IXGRP);
		bool logclient(false), logrouter(false), logsockcli(false), socketterminate(true), setproctitle(false);
#endif
		{
			time_t deptime(time(0));
			srand(deptime);
			{
				deptime += 3600;
				struct tm tm;
				gmtime_r(&deptime, &tm);
				tm.tm_sec = tm.tm_min = 0;
				tm.tm_hour = 10;
				time_t t(timegm(&tm));
				if (t < deptime) {
					t += 24 * 60 * 60;
					gmtime_r(&t, &tm);
					tm.tm_sec = tm.tm_min = 0;
					tm.tm_hour = 10;
					t = timegm(&tm);
				}
				deptime = t;
			}
			autoroute->set_deptime(deptime);
		}
		{
			static struct option long_options[] = {
				{ "maindir", required_argument, 0, 'm' },
				{ "auxdir", required_argument, 0, 'a' },
				{ "disableaux", no_argument, 0, 'A' },
				{ "aixm45", no_argument, 0, '4' },
				{ "aixm51", no_argument, 0, '5' },
				{ "logprefix", required_argument, 0, 'l' },
				{ "aircraft", required_argument, 0, 0x100 },
				{ "dctlimit", required_argument, 0, 0x101 },
				{ "depvfr", no_argument, 0, 0x102 },
				{ "depifr", no_argument, 0, 0x103 },
				{ "destvfr", no_argument, 0, 0x104 },
				{ "destifr", no_argument, 0, 0x105 },
				{ "sidlimit", required_argument, 0, 0x106 },
				{ "starlimit", required_argument, 0, 0x107 },
				{ "sidpenalty", required_argument, 0, 0x149 },
				{ "starpenalty", required_argument, 0, 0x150 },
				{ "sidoffset", required_argument, 0, 0x151 },
				{ "staroffset", required_argument, 0, 0x152 },
				{ "sidminimum", required_argument, 0, 0x153 },
				{ "starminimum", required_argument, 0, 0x154 },
				{ "sid", required_argument, 0, 0x108 },
				{ "star", required_argument, 0, 0x109 },
				{ "alternate1", required_argument, 0, 0x16a },
				{ "alternate2", required_argument, 0, 0x16b },
				{ "airspacelimit", required_argument, 0, 0x10a },
				{ "enrifr", no_argument, 0, 0x10b },
				{ "enrouteifr", no_argument, 0, 0x10b },
				{ "forceenrouteifr", no_argument, 0, 0x10b },
				{ "dctpenalty", no_argument, 0, 0x10c },
				{ "dctoffset", no_argument, 0, 0x155 },
				{ "machineinterface", no_argument, 0, 0x10d },
				{ "time", no_argument, 0, 0x10e },
				{ "fuel", no_argument, 0, 0x10f },
				{ "preferred", no_argument, 0, 0x118 },
				{ "disable-levelchangetrackmiles", no_argument, 0, 0x144 },
				{ "enable-levelchangetrackmiles", no_argument, 0, 0x145 },
				{ "disable-opsperftrackmiles", no_argument, 0, 0x160 },
				{ "enable-opsperftrackmiles", no_argument, 0, 0x161 },
				{ "maxdescent", required_argument, 0, 0x146 },
				{ "disable-airway-levels", no_argument, 0, 0x110 },
				{ "enable-airway-levels", no_argument, 0, 0x111 },
				{ "disable-profile-rules", no_argument, 0, 0x16c },
				{ "enable-profile-rules", no_argument, 0, 0x16d },
				{ "disable-tfr", no_argument, 0, 0x112 },
				{ "enable-tfr", no_argument, 0, 0x113 },
				{ "disable-tfr-rules", required_argument, 0, 0x142 },
				{ "trace-tfr-rules", required_argument, 0, 0x143 },
				{ "preferredlevel", required_argument, 0, 0x114 },
				{ "preferredpenalty", required_argument, 0, 0x115 },
				{ "preferredclimb", required_argument, 0, 0x116 },
				{ "preferreddescent", required_argument, 0, 0x117 },
				{ "precompute", no_argument, 0, 0x119 },
				{ "enable-precompgraph", no_argument, 0, 0x120 },
				{ "disable-precompgraph", no_argument, 0, 0x121 },
				{ "enable-siddb", no_argument, 0, 0x122 },
				{ "disable-siddb", no_argument, 0, 0x123 },
				{ "enable-stardb", no_argument, 0, 0x124 },
				{ "disable-stardb", no_argument, 0, 0x125 },
				{ "enable-sidonly", no_argument, 0, 0x162 },
				{ "disable-sidonly", no_argument, 0, 0x163 },
				{ "enable-staronly", no_argument, 0, 0x164 },
				{ "disable-staronly", no_argument, 0, 0x165 },
				{ "sidfilter", required_argument, 0, 0x166 },
				{ "starfilter", required_argument, 0, 0x167 },
#ifdef HAVE_JSONCPP
				{ "socketserver", optional_argument, 0, 0x126 },
				{ "routerlimit", required_argument, 0, 0x127 },
				{ "routeractivelimit", required_argument, 0, 0x148 },
				{ "routertimeout", required_argument, 0, 0x128 },
				{ "routermaxruntime", required_argument, 0, 0x129 },
				{ "noterminate", no_argument, 0, 0x12a },
				{ "uid", required_argument, 0, 0x12b },
				{ "gid", required_argument, 0, 0x12c },
				{ "sockuid", required_argument, 0, 0x12d },
				{ "sockgid", required_argument, 0, 0x12e },
				{ "sockmode", required_argument, 0, 0x12f },
				{ "log-client", no_argument, 0, 0x130 },
				{ "log-router", no_argument, 0, 0x131 },
				{ "log-sockcli", no_argument, 0, 0x141 },
				{ "log-socketclient", no_argument, 0, 0x141 },
				{ "setproctitle", no_argument, 0, 0x147 },
#endif
				{ "crossing", required_argument, 0, 'X' },
				{ "crossing-radius", required_argument, 0, 0x132 },
				{ "crossing-minlevel", required_argument, 0, 0x133 },
				{ "crossing-maxlevel", required_argument, 0, 0x134 },
				{ "validator-binary", optional_argument, 0, 0x135 },
				{ "validator-socket", optional_argument, 0, 0x136 },
				{ "validator-default", no_argument, 0, 0x137 },
				{ "validator-cfmu", no_argument, 0, 0x138 },
				{ "validator-eurofpl", no_argument, 0, 0x139 },
				{ "xdisplay", required_argument, 0, 0x13a },
				{ "exclude", required_argument, 0, 'E' },
				{ "exclude-minlevel", required_argument, 0, 0x13b },
				{ "exclude-maxlevel", required_argument, 0, 0x13c },
				{ "exclude-awylimit", required_argument, 0, 0x13d },
				{ "exclude-dctlimit", required_argument, 0, 0x13e },
				{ "exclude-dctoffset", required_argument, 0, 0x13f },
				{ "exclude-dctscale", required_argument, 0, 0x140 },
				{ "timestamp", no_argument, 0, 0x156 },
				{ "timestamp-relative", no_argument, 0, 0x157 },
				{ "deptime", required_argument, 0, 0x158 },
				{ "maxlocaliterations", required_argument, 0, 0x168 },
				{ "maxremoteiterations", required_argument, 0, 0x169 },
				{ "wind", no_argument, 0, 'w' },
				{ "rpm", required_argument, 0, 'R' },
				{ "mp", required_argument, 0, 'M' },
				{ "bhp", required_argument, 0, 'B' },
				{ "qnh", required_argument, 0, 'q' },
				{ "isaoffs", required_argument, 0, 't' },
				{0, 0, 0, 0}
			};
			int c, err(0);
			while ((c = getopt_long(argc, argv, "m:a:AR:M:B:q:t:wX:E:45l:", long_options, 0)) != EOF) {
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

				case 'l':
					if (optarg)
						autoroute->set_logprefix(optarg);
					break;

				case '4':
				case '5':
				{
					CFMUAutoroute *a(0);
					if (c == '5')
						a = new CFMUAutoroute51();
					else
						a = new CFMUAutoroute45();
					//*a = *autoroute;
					a->set_db_directories(autoroute->get_db_maindir(), Engine::auxdb_override, autoroute->get_db_auxdir());
					a->get_aircraft() = autoroute->get_aircraft();
					a->set_opttarget(autoroute->get_opttarget());
					a->set_departure(autoroute->get_departure());
					a->set_departure_ifr(autoroute->get_departure_ifr());
					a->set_sid(autoroute->get_sid());
					a->set_sidlimit(autoroute->get_sidlimit());
					a->set_sidpenalty(autoroute->get_sidpenalty());
					a->set_sidoffset(autoroute->get_sidoffset());
					a->set_sidminimum(autoroute->get_sidminimum());
					a->set_siddb(autoroute->get_siddb());
					a->set_sidonly(autoroute->get_sidonly());
					a->set_sidfilter(autoroute->get_sidfilter());
					a->set_destination(autoroute->get_destination());
					a->set_destination_ifr(autoroute->get_destination_ifr());
					a->set_star(autoroute->get_star());
					a->set_starlimit(autoroute->get_starlimit());
					a->set_starpenalty(autoroute->get_starpenalty());
					a->set_staroffset(autoroute->get_staroffset());
					a->set_starminimum(autoroute->get_starminimum());
					a->set_stardb(autoroute->get_stardb());
					a->set_staronly(autoroute->get_staronly());
					a->set_starfilter(autoroute->get_starfilter());
					for (unsigned int i = 0; i < 2; ++i)
						a->set_alternate(i, autoroute->get_alternate(i));
					a->set_crossing_size(autoroute->get_crossing_size());
					for (unsigned int i(0), n(autoroute->get_crossing_size()); i < n; ++i) {
						a->set_crossing(i, autoroute->get_crossing(i));
						//a->set_crossing(i, autoroute->get_crossing_ident(i));
						a->set_crossing_radius(i, autoroute->get_crossing_radius(i));
						a->set_crossing_level(i, autoroute->get_crossing_minlevel(i),
								      autoroute->get_crossing_maxlevel(i));
					}
					a->set_dctlimit(autoroute->get_dctlimit());
					a->set_dctpenalty(autoroute->get_dctpenalty());
					a->set_dctoffset(autoroute->get_dctoffset());
					a->set_vfraspclimit(autoroute->get_vfraspclimit());
					a->set_levels(autoroute->get_baselevel(), autoroute->get_toplevel());
					a->set_maxdescent(autoroute->get_maxdescent());
					a->set_honour_levelchangetrackmiles(autoroute->get_honour_levelchangetrackmiles());
					a->set_honour_opsperftrackmiles(autoroute->get_honour_opsperftrackmiles());
					a->set_honour_awy_levels(autoroute->get_honour_awy_levels());
					a->set_honour_profilerules(autoroute->get_honour_profilerules());
					a->set_preferred_level(autoroute->get_preferred_level());
					a->set_preferred_penalty(autoroute->get_preferred_penalty());
					a->set_preferred_climb(autoroute->get_preferred_climb());
					a->set_preferred_descent(autoroute->get_preferred_descent());
					a->set_excluderegions(autoroute->get_excluderegions());
					a->set_force_enroute_ifr(autoroute->get_force_enroute_ifr());
					a->set_tfr_enabled(autoroute->get_tfr_enabled());
					a->set_tfr_trace(autoroute->get_tfr_trace());
					a->set_tfr_disable(autoroute->get_tfr_disable());
					a->set_tfr_savefile(autoroute->get_tfr_savefile());
					a->set_precompgraph_enabled(autoroute->get_precompgraph_enabled());
					a->set_wind_enabled(autoroute->get_wind_enabled());
					a->set_qnh(autoroute->get_qnh());
					a->set_isaoffs(autoroute->get_isaoffs());
					a->set_engine_rpm(autoroute->get_engine_rpm());
					a->set_engine_mp(autoroute->get_engine_mp());
					a->set_engine_bhp(autoroute->get_engine_bhp());
					a->set_deptime(autoroute->get_deptime());
					a->set_maxlocaliteration(autoroute->get_maxlocaliteration());
					a->set_maxremoteiteration(autoroute->get_maxremoteiteration());
					a->set_validator(autoroute->get_validator());
					a->set_validator_socket(autoroute->get_validator_socket());
					a->set_validator_binary(autoroute->get_validator_binary());
					a->set_validator_xdisplay(autoroute->get_validator_xdisplay());
					a->set_logprefix(autoroute->get_logprefix());
					std::swap(a, autoroute);
					delete a;
					break;
				}

				case 0x100:
					if (optarg) {
						if (autoroute->get_aircraft().load_file(optarg)) {
							autoroute->get_aircraft().get_dist().recalculatepoly(false);
							autoroute->get_aircraft().get_climb().recalculatepoly(false);
							autoroute->get_aircraft().get_descent().recalculatepoly(false);
							break;
						}
						std::cerr << "Unable to load aircraft file \"" << optarg << "\"" << std::endl;
						return EX_NOINPUT;
					}
					break;

				case 0x101:
					if (optarg)
						autoroute->set_dctlimit(strtod(optarg, 0));
					break;

				case 0x102:
				case 0x103:
					autoroute->set_departure_ifr(c & 1);
					break;

				case 0x104:
				case 0x105:
					autoroute->set_destination_ifr(c & 1);
					break;

				case 0x106:
					if (optarg)
						autoroute->set_sidlimit(strtod(optarg, 0));
					break;

				case 0x107:
					if (optarg)
						autoroute->set_starlimit(strtod(optarg, 0));
					break;

				case 0x149:
					if (optarg)
						autoroute->set_sidpenalty(strtod(optarg, 0));
					break;

				case 0x150:
					if (optarg)
						autoroute->set_starpenalty(strtod(optarg, 0));
					break;

				case 0x151:
					if (optarg)
						autoroute->set_sidoffset(strtod(optarg, 0));
					break;

				case 0x152:
					if (optarg)
						autoroute->set_staroffset(strtod(optarg, 0));
					break;

				case 0x153:
					if (optarg)
						autoroute->set_sidminimum(strtod(optarg, 0));
					break;

				case 0x154:
					if (optarg)
						autoroute->set_starminimum(strtod(optarg, 0));
					break;

				case 0x108:
					if (optarg)
						sidname = optarg;
					break;

				case 0x109:
					if (optarg)
						starname = optarg;
					break;

				case 0x16a:
				case 0x16b:
					if (optarg)
						autoroute->set_alternate(c - 0x16a, optarg);
					break;

				case 0x10a:
					if (optarg)
						autoroute->set_vfraspclimit(strtod(optarg, 0));
					break;

				case 0x10b:
					autoroute->set_force_enroute_ifr(true);
					break;

				case 0x10c:
					if (optarg)
						autoroute->set_dctpenalty(strtod(optarg, 0));
					break;

				case 0x155:
					if (optarg)
						autoroute->set_dctoffset(strtod(optarg, 0));
					break;

				case 0x10d:
					mode = mode_machineinterface;
					break;

				case 0x10e:
					autoroute->set_opttarget(CFMUAutoroute::opttarget_time);
					break;

				case 0x10f:
					autoroute->set_opttarget(CFMUAutoroute::opttarget_fuel);
					break;

				case 0x118:
					autoroute->set_opttarget(CFMUAutoroute::opttarget_preferred);
					break;

				case 0x110:
				case 0x111:
					autoroute->set_honour_awy_levels(c & 1);
					break;

				case 0x16c:
				case 0x16d:
					autoroute->set_honour_profilerules(c & 1);
					break;

				case 0x144:
				case 0x145:
					autoroute->set_honour_levelchangetrackmiles(c & 1);
					break;

				case 0x160:
				case 0x161:
					autoroute->set_honour_opsperftrackmiles(c & 1);
					break;

				case 0x146:
					if (optarg)
						autoroute->set_maxdescent(strtod(optarg, 0));
					break;

				case 0x112:
				case 0x113:
					tfrenable = c & 1;
					break;

				case 0x142:
					if (optarg)
						autoroute->set_tfr_disable(optarg);
					break;

				case 0x143:
					if (optarg)
						autoroute->set_tfr_trace(optarg);
					break;

				case 'R':
					if (!setengparams) {
						autoroute->set_engine_rpm(std::numeric_limits<double>::quiet_NaN());
						autoroute->set_engine_mp(std::numeric_limits<double>::quiet_NaN());
						autoroute->set_engine_bhp(std::numeric_limits<double>::quiet_NaN());
						setengparams = true;
					}
					if (optarg)
						autoroute->set_engine_rpm(strtod(optarg, 0));
					break;

				case 'M':
					if (!setengparams) {
						autoroute->set_engine_rpm(std::numeric_limits<double>::quiet_NaN());
						autoroute->set_engine_mp(std::numeric_limits<double>::quiet_NaN());
						autoroute->set_engine_bhp(std::numeric_limits<double>::quiet_NaN());
						setengparams = true;
					}
					if (optarg)
						autoroute->set_engine_mp(strtod(optarg, 0));
					break;

				case 'B':
					if (!setengparams) {
						autoroute->set_engine_rpm(std::numeric_limits<double>::quiet_NaN());
						autoroute->set_engine_mp(std::numeric_limits<double>::quiet_NaN());
						autoroute->set_engine_bhp(std::numeric_limits<double>::quiet_NaN());
						setengparams = true;
					}
					if (optarg)
						autoroute->set_engine_bhp(strtod(optarg, 0));
					break;

				case 'q':
					if (optarg)
						autoroute->set_qnh(strtod(optarg, 0));
					break;

				case 't':
					if (optarg)
						autoroute->set_isaoffs(strtod(optarg, 0));
					break;

				case 0x114:
					if (optarg)
						autoroute->set_preferred_level(strtol(optarg, 0, 0));
					break;

				case 0x115:
					if (optarg)
						autoroute->set_preferred_penalty(strtod(optarg, 0));
					break;

				case 0x116:
					if (optarg)
						autoroute->set_preferred_climb(strtod(optarg, 0));
					break;

				case 0x117:
					if (optarg)
						autoroute->set_preferred_descent(strtod(optarg, 0));
					break;

				case 'w':
					autoroute->set_wind_enabled(true);
					break;

				case 0x119:
					precompute = true;
					break;

				case 0x120:
					autoroute->set_precompgraph_enabled(true);
					break;

				case 0x121:
					autoroute->set_precompgraph_enabled(false);
					break;

				case 0x122:
					autoroute->set_siddb(true);
					break;

				case 0x123:
					autoroute->set_siddb(false);
					break;

				case 0x124:
					autoroute->set_stardb(true);
					break;

				case 0x125:
					autoroute->set_stardb(false);
					break;

				case 0x162:
					autoroute->set_sidonly(true);
					break;

				case 0x163:
					autoroute->set_sidonly(false);
					break;

				case 0x164:
					autoroute->set_staronly(true);
					break;

				case 0x165:
					autoroute->set_staronly(false);
					break;

				case 0x166:
					if (optarg && *optarg)
						autoroute->add_sidfilter(optarg);
					break;

				case 0x167:
					if (optarg && *optarg)
						autoroute->add_starfilter(optarg);
					break;

#ifdef HAVE_JSONCPP
				case 0x126:
					mode = mode_socketserver;
					if (optarg)
						sockserverpath = optarg;
					break;

				case 0x127:
					if (optarg)
						routerlimit = strtoul(optarg, 0, 0);
					break;

				case 0x148:
					if (optarg)
						routeractivelimit = strtoul(optarg, 0, 0);
					break;

				case 0x128:
					if (optarg)
						routertimeout = strtoul(optarg, 0, 0);
					break;

				case 0x129:
					if (optarg)
						routermaxruntime = strtoul(optarg, 0, 0);
					break;

				case 0x12a:
					socketterminate = false;
					break;

				case 0x12b:
					if (optarg) {
						char *cp;
						uid_t uid1(strtoul(optarg, &cp, 0));
						if (*cp) {
							struct passwd *pw = getpwnam(optarg);
							if (pw) {
								sockservuid = pw->pw_uid;
								sockservgid = pw->pw_gid;
							} else {
								std::cerr << "User " << optarg << " not found" << std::endl;
							}
						} else {
							sockservuid = uid1;
						}
					}
					break;

				case 0x12c:
					if (optarg) {
						char *cp;
						uid_t gid1(strtoul(optarg, &cp, 0));
						if (*cp) {
							struct group *gr = getgrnam(optarg);
							if (gr) {
								sockservgid = gr->gr_gid;
							} else {
								std::cerr << "Group " << optarg << " not found" << std::endl;
							}
						} else {
							sockservgid = gid1;
						}
					}
					break;

				case 0x12d:
					if (optarg) {
						char *cp;
						uid_t uid1(strtoul(optarg, &cp, 0));
						if (*cp) {
							struct passwd *pw = getpwnam(optarg);
							if (pw) {
								socketuid = pw->pw_uid;
								socketgid = pw->pw_gid;
							} else {
								std::cerr << "User " << optarg << " not found" << std::endl;
							}
						} else {
							socketuid = uid1;
						}
					}
					break;

				case 0x12e:
					if (optarg) {
						char *cp;
						uid_t gid1(strtoul(optarg, &cp, 0));
						if (*cp) {
							struct group *gr = getgrnam(optarg);
							if (gr) {
								socketgid = gr->gr_gid;
							} else {
								std::cerr << "Group " << optarg << " not found" << std::endl;
							}
						} else {
							socketgid = gid1;
						}
					}
					break;

				case 0x12f:
					if (optarg)
						socketmode = strtoul(optarg, 0, 0);
					break;

				case 0x130:
					logclient = true;
					break;

				case 0x131:
					logrouter = true;
					break;

				case 0x141:
					logsockcli = true;
					break;

				case 0x147:
					setproctitle = true;
					break;
#endif

				case 'X':
					if (optarg)
						crossings.push_back(Crossing(optarg, crossingradius, crossingminlevel, crossingmaxlevel));
					break;

				case 0x132:
					if (optarg)
						crossingradius = strtod(optarg, 0);
					break;

				case 0x133:
					if (optarg)
						crossingminlevel = strtol(optarg, 0, 0);
					break;

				case 0x134:
					if (optarg)
						crossingmaxlevel = strtol(optarg, 0, 0);
					break;

				case 0x135:
					if (optarg)
						autoroute->set_validator_binary(optarg);
					else
						autoroute->set_validator_binary();
					break;

				case 0x136:
					if (optarg)
						autoroute->set_validator_socket(optarg);
					else
						autoroute->set_validator_socket();
					break;

				case 0x137:
					autoroute->set_validator(CFMUAutoroute::validator_default);
					break;

				case 0x138:
					autoroute->set_validator(CFMUAutoroute::validator_cfmu);
					break;

				case 0x139:
					autoroute->set_validator(CFMUAutoroute::validator_eurofpl);
					break;

				case 0x13a:
					if (optarg)
						autoroute->set_validator_xdisplay(strtoul(optarg, 0, 0));
					break;

				case 'E':
					if (optarg) {
						char *cp(strchr(optarg, '/'));
						if (cp && *cp)
							*cp++ = 0;
						autoroute->add_excluderegion(CFMUAutoroute::ExcludeRegion(optarg, cp ? cp : "",
													  excludeminlevel, excludemaxlevel,
													  excludeawylimit, excludedctlimit,
													  excludedctoffset, excludedctscale));
					}
					break;

				case 0x13b:
					if (optarg)
						excludeminlevel = strtol(optarg, 0, 0);
					break;

				case 0x13c:
					if (optarg)
						excludemaxlevel = strtol(optarg, 0, 0);
					break;

				case 0x13d:
					if (optarg)
						excludeawylimit = strtod(optarg, 0);
					break;

				case 0x13e:
					if (optarg)
						excludedctlimit = strtod(optarg, 0);
					break;

				case 0x13f:
					if (optarg)
						excludedctoffset = strtod(optarg, 0);
					break;

				case 0x140:
					if (optarg)
						excludedctscale = strtod(optarg, 0);
					break;     

				case 0x156:
					timestamp = timestamp_iso8601;
					break;

				case 0x157:
					timestamp = timestamp_relms;
					break;

				case 0x158:
					if (optarg) {
						Glib::TimeVal tv;
						if (!tv.assign_from_iso8601(optarg))
							std::cerr << "Cannot parse date " << optarg << std::endl;
						else
							autoroute->set_deptime(tv.tv_sec);
					}
					break;

				case 0x168:
					if (optarg)
						autoroute->set_maxlocaliteration(strtoul(optarg, 0, 0));
					break;

				case 0x169:
					if (optarg)
						autoroute->set_maxremoteiteration(strtoul(optarg, 0, 0));
					break;

				default:
					err++;
					break;
				}
			}
			if (err || (optind + 2 >= argc && mode == mode_commandline)) {
				std::cerr << "usage: cfmuautoroute [-m <maindir>] [-a <auxdir>] [-A] [--aircraft=file.xml]"
					" [--depvfr] [--depifr] [--destvfr] [--destifr] [--sid=wpt] [--star=wpt] [--airspacelimit=lim]"
					" [--dctlimit=lim] [--dctpenalty=p] [--dctoffset=o] [--sidlimit=lim] [--sidpenalty=p] [--sidoffset=o]"
					" [--starlimit=lim] [--starpenalty=p] [--staroffset=o]"
					" [--forceenrouteifr] [--qnh=qnh] [--isaoffs=temp] [--time] [--fuel] [--wind] [--preferred]"
					" [--preferredlevel=l] [--preferredpenalty=p] [--preferredclimb=c] [--preferreddescent=d]"
					" [--crossing=x] [--crossing-radius=x] [--crossing-minlevel=x] [--crossing-maxlevel=x]"
					" [--validator-binary=x] [--validator-socket=x] [--validator-cfmu] [--validator-eurofpl]"
					" [--timestamp] [--deptime=<YYYY-MM-DDTHH:MM:SS>]"
					" <dep> <dest> <baselvl> <toplvl>" << std::endl;
				return EX_USAGE;
			}
		}
		if (dis_aux)
			auxdbmode = Engine::auxdb_none;
		else if (!dir_aux.empty())
			auxdbmode = Engine::auxdb_override;
		autoroute->set_db_directories(dir_main, auxdbmode, dir_aux);
		autoroute->set_tfr_enabled(tfrenable);
		if (precompute) {
			if (optind + 6 >= argc) {
				std::cerr << "Not enough arguments (need <west> <south> <east> <north> <base> <top> <file>)" << std::endl;
				return EX_DATAERR;
			}
			Rect bbox;
			{
				Point sw, ne;
				unsigned int x(sw.set_str(argv[optind + 0]) | sw.set_str(argv[optind + 1]));
				if ((Point::setstr_lat | Point::setstr_lon) & ~x) {
					std::cerr << "Invalid <south> / <west>" << std::endl;
					return EX_DATAERR;
				}
				if (x & Point::setstr_excess)
					std::cerr << "Warning: excess <south> / <west> characters" << std::endl;
				x = ne.set_str(argv[optind + 2]) | ne.set_str(argv[optind + 3]);
				if ((Point::setstr_lat | Point::setstr_lon) & ~x) {
					std::cerr << "Invalid <north> / <east>" << std::endl;
					return EX_DATAERR;
				}
				if (x & Point::setstr_excess)
					std::cerr << "Warning: excess <north> / <east> characters" << std::endl;
				bbox = Rect(sw, ne);
			}
			try {
				autoroute->set_levels(strtol(argv[optind + 4], 0, 0), strtol(argv[optind + 5], 0, 0));
				autoroute->signal_log().connect(sigc::ptr_fun(&autoroutelog));
				autoroute->precompute_graph(bbox, argv[optind + 6]);
			} catch (const std::exception& ex) {
				std::cerr << "Error precomputing Graph: " << ex.what() << std::endl;
				return EX_DATAERR;
			}
			return EX_OK;
		}
		if (optind + 0 < argc) {
			if (!autoroute->set_departure(argv[optind], argv[optind])) {
				std::cerr << "Departure Airport " << argv[optind] << " not found" << std::endl;
				return EX_DATAERR;
			}
		}
		if (optind + 1 < argc) {
			if (!autoroute->set_destination(argv[optind + 1], argv[optind + 1])) {
				std::cerr << "Destination Airport " << argv[optind + 1] << " not found" << std::endl;
				return EX_DATAERR;
			}
		}
		if (!sidname.empty()) {
			if (!autoroute->set_sid(sidname)) {
				std::cerr << "SID " << sidname << " not found" << std::endl;
				return EX_DATAERR;
			}
		}
		if (!starname.empty()) {
			if (!autoroute->set_star(starname)) {
				std::cerr << "STAR " << starname << " not found" << std::endl;
				return EX_DATAERR;
			}
		}
		if (optind + 2 < argc)
			autoroute->set_levels(strtol(argv[optind + 2], 0, 0), autoroute->get_toplevel());
		if (optind + 3 < argc)
			autoroute->set_levels(autoroute->get_baselevel(), strtol(argv[optind + 3], 0, 0));
		for (unsigned int index = 0; index < crossings.size(); ++index) {
			const Crossing& c(crossings[index]);
			autoroute->set_crossing(index, c.get_name());
			autoroute->set_crossing_radius(index, c.get_radius());
			autoroute->set_crossing_level(index, c.get_minlevel(), c.get_maxlevel());
		}
		starttime.assign_current_time();
		switch (mode) {
		case mode_machineinterface:
		{
			cmdlist["nop"] = cmd_nop;
			cmdlist["quit"] = cmd_quit;
			cmdlist["departure"] = cmd_departure;
			cmdlist["destination"] = cmd_destination;
			cmdlist["crossing"] = cmd_crossing;
			cmdlist["enroute"] = cmd_enroute;
			cmdlist["levels"] = cmd_levels;
			cmdlist["exclude"] = cmd_exclude;
			cmdlist["tfr"] = cmd_tfr;
			cmdlist["atmosphere"] = cmd_atmosphere;
			cmdlist["cruise"] = cmd_cruise;
			cmdlist["optimization"] = cmd_optimization;
			cmdlist["preferred"] = cmd_preferred;
			cmdlist["aircraft"] = cmd_aircraft;
			cmdlist["start"] = cmd_start;
			cmdlist["stop"] = cmd_stop;
			cmdlist["continue"] = cmd_continue;
			cmdlist["preload"] = cmd_preload;
			cmdlist["clear"] = cmd_clear;
			inputchan = Glib::IOChannel::create_from_fd(0);
			outputchan = Glib::IOChannel::create_from_fd(1);
			inputchan->set_encoding();
			outputchan->set_encoding();
			outputchan->set_buffered(true);
			outputchan->set_buffer_size(32768);
			Glib::signal_io().connect(sigc::ptr_fun(&machine_input), inputchan, Glib::IO_IN | Glib::IO_ERR | Glib::IO_HUP);
			autoroute->signal_statuschange().connect(sigc::ptr_fun(&machine_statuschange));
			autoroute->signal_log().connect(sigc::ptr_fun(&machine_autoroutelog));
			{
				Command cmd("autoroute");
				cmd.set_option("version", VERSION);
				cmd.set_option("provider", "cfmu");
				machine_writecmd(cmd);
			}
			outputchan->flush();
			mainloop->run();
			return EX_OK;
		}

#ifdef HAVE_JSONCPP
		case mode_socketserver:
		{
			int xdisplay(autoroute->get_validator_xdisplay());
			autoroute->set_validator_xdisplay(-1);
			autoroute->signal_statuschange().connect(sigc::ptr_fun(&statuschange));
			autoroute->signal_log().connect(sigc::ptr_fun(&autoroutelog));
			{
				bool wind(autoroute->get_wind_enabled());
				autoroute->set_wind_enabled(true);
				autoroute->preload(false);
				autoroute->set_wind_enabled(wind);
			}
			SocketServer sserv(autoroute, mainloop, routerlimit, routeractivelimit, routertimeout, routermaxruntime,
					   xdisplay, logclient, logrouter, logsockcli, setproctitle);
			sserv.listen(sockserverpath, socketuid, socketgid, socketmode, socketterminate);
			if (sockservgid && setgid(sockservgid))
				std::cerr << "Cannot set gid to " << sockservgid << ": "
					  << strerror(errno) << " (" << errno << ')' << std::endl;
			if (sockservuid && setuid(sockservuid))
				std::cerr << "Cannot set uid to " << sockservuid << ": "
					  << strerror(errno) << " (" << errno << ')' << std::endl;
			/* Cancel certain signals */
			signal(SIGTSTP, SIG_IGN); /* Various TTY signals */
			signal(SIGTTOU, SIG_IGN);
			signal(SIGTTIN, SIG_IGN);
			signal(SIGHUP,  SIG_IGN); /* Ignore hangup signal */
			signal(SIGTERM, signalterminate); /* terminate mainloop on SIGTERM */
			/* Create a new SID for the child process */
			if (getpgrp() != getpid() && setsid() == (pid_t)-1)
				std::cerr << "Cannot create new session: "
					  << strerror(errno) << " (" << errno << ')' << std::endl;
			/* Change the file mode mask */
			umask(0);
			/* Change the current working directory.  This prevents the current
			   directory from being locked; hence not being able to remove it. */
			if (chdir("/tmp") < 0)
				std::cerr << "Cannot change directory to /tmp: "
					  << strerror(errno) << " (" << errno << ')' << std::endl;
			mainloop->run();
			return EX_OK;
		}
#endif

		default:
			break;
		}
		autoroute->signal_statuschange().connect(sigc::ptr_fun(&statuschange));
		autoroute->signal_log().connect(sigc::ptr_fun(&autoroutelog));
		std::cout << "Departure: " << autoroute->get_departure().get_icao_name() << " ("
			  << (autoroute->get_departure_ifr() ? 'I' : 'V') << "FR) "
			  << Glib::TimeVal(autoroute->get_deptime(), 0).as_iso8601() << std::endl;
		std::cout << "Destination: " << autoroute->get_destination().get_icao_name() << " ("
			  << (autoroute->get_destination_ifr() ? 'I' : 'V') << "FR)" << std::endl;
		if (autoroute->get_sid().is_invalid())
			std::cout << "SID Limit (nmi): " << Conversions::dist_str(autoroute->get_sidlimit())
				  << " Penalty: " << autoroute->get_sidpenalty()
				  << " Offset: " << Conversions::dist_str(autoroute->get_sidoffset())
				  << " Minimum: " << Conversions::dist_str(autoroute->get_sidminimum()) << std::endl;
		else
			std::cout << "SID: " << autoroute->get_sid().get_lat_str2() << ' ' << autoroute->get_sid().get_lon_str2() << std::endl;
		if (autoroute->get_star().is_invalid())
			std::cout << "STAR Limit (nmi): " << Conversions::dist_str(autoroute->get_starlimit())
				  << " Penalty: " << autoroute->get_starpenalty()
				  << " Offset: " << Conversions::dist_str(autoroute->get_staroffset())
				  << " Minimum: " << Conversions::dist_str(autoroute->get_starminimum()) << std::endl;
		else
			std::cout << "STAR: " << autoroute->get_star().get_lat_str2() << ' ' << autoroute->get_star().get_lon_str2() << std::endl;
		if (autoroute->get_crossing_size()) {
			std::cout << "Crossing:";
			for (unsigned int i = 0; i < autoroute->get_crossing_size(); ++i) {
				std::cout << ' ' << autoroute->get_crossing(i).get_lat_str2() << ' ' << autoroute->get_crossing(i).get_lon_str2();
				if (!autoroute->get_crossing_ident(i).empty())
					std::cout << ' ' << autoroute->get_crossing_ident(i);
				std::cout << " F" << std::setw(3) << std::setfill('0') << autoroute->get_crossing_minlevel(i)
					  << "...F" << std::setw(3) << std::setfill('0') << autoroute->get_crossing_maxlevel(i)
					  << " R" << autoroute->get_crossing_radius(i);
			}
			std::cout << std::endl;
		}
		{
			const CFMUAutoroute::excluderegions_t er(autoroute->get_excluderegions());
			if (!er.empty()) {
				std::cout << "Exclude:";
				for (unsigned int i = 0; i < er.size(); ++i) {
					const CFMUAutoroute::ExcludeRegion& e(er[i]);
					std::cout << ' ';
					if (e.is_airspace())
						std::cout << e.get_airspace_id() << '/' << e.get_airspace_type();
					else
						std::cout << e.get_bbox().get_southwest().get_lat_str2() << ' '
							  << e.get_bbox().get_southwest().get_lon_str2() << " - "
							  << e.get_bbox().get_northeast().get_lat_str2() << ' '
							  << e.get_bbox().get_northeast().get_lon_str2();
					std::cout << " F" << std::setw(3) << std::setfill('0') << e.get_minlevel()
						  << "...F" << std::setw(3) << std::setfill('0') << e.get_maxlevel()
						  << " A" << e.get_awylimit()
						  << " D" << e.get_dctlimit()
						  << ' ' << e.get_dctoffset()
						  << ' ' << e.get_dctscale();
				}
				std::cout << std::endl;
			}
		}
		std::cout << "DCT Limit (nmi): " << Conversions::dist_str(autoroute->get_dctlimit())
			  << " Penalty: " << autoroute->get_dctpenalty()
			  << " Offset: " << Conversions::dist_str(autoroute->get_dctoffset()) << std::endl;
		if (autoroute->get_honour_levelchangetrackmiles())
			std::cout << "Max Descent: " << autoroute->get_maxdescent() << "ft/min" << std::endl;
		if (autoroute->get_departure_ifr() || autoroute->get_destination_ifr() || autoroute->get_force_enroute_ifr()) {
			std::cout << "Optimization target: " << ((autoroute->get_opttarget() == CFMUAutoroute::opttarget_preferred) ? "Preferred" : 
								 (autoroute->get_opttarget() == CFMUAutoroute::opttarget_fuel) ? "Fuel" : "Time") << std::endl;
			if (autoroute->get_opttarget() == CFMUAutoroute::opttarget_preferred) {
				std::cout << "Preferred Level: " << autoroute->get_preferred_level()
					  << " Penalty: " << autoroute->get_preferred_penalty()
					  << " Climb: " << autoroute->get_preferred_climb()
					  << " Descent: " << autoroute->get_preferred_descent() << std::endl;
			}
		} else {
			std::cout << "VFR Airspace Area Limit (nmi^2): " << Conversions::dist_str(autoroute->get_vfraspclimit()) << std::endl;
		}
		std::cout << "Levels: F" << std::setw(3) << std::setfill('0') << autoroute->get_baselevel()
			  << "...F" << std::setw(3) << std::setfill('0') << autoroute->get_toplevel() << std::endl
			  << "QNH: " << autoroute->get_qnh() << " ISA" << std::showpos << autoroute->get_isaoffs() << std::noshowpos
			  << " Engine: " << autoroute->get_engine_rpm() << '/' << autoroute->get_engine_mp() << '/' << autoroute->get_engine_bhp() << std::endl;
		autoroute->start();
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
        return autoroute->is_errorfree() ? EX_OK : EX_UNAVAILABLE;
}
