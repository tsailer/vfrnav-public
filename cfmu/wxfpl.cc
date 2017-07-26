#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <getopt.h>
#include <iostream>
#include <iomanip>
#include <glibmm.h>
#include <giomm.h>
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
#include "metgraph.h"
#include "adr.hh"
#include "adrdb.hh"
#include "adrfplan.hh"
#include "tfrproxy.hh"

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
static Cairo::RefPtr<Cairo::Surface> surface;
static Cairo::RefPtr<Cairo::PdfSurface> pdfsurface;
static double height(595.28), width(841.89), maxprofilepagedist(100);
static MeteoProfile::yaxis_t profileymode(MeteoProfile::yaxis_altitude);
static AirportsDb airportdb;
static NavaidsDb navaiddb;
static WaypointsDb waypointdb;
static AirwaysDb airwaydb;
static AirspacesDb airspacedb;
static ADR::Database adrdb;
static TopoDb30 topodb;
static Aircraft acft;
static double cruiserpm(std::numeric_limits<double>::quiet_NaN());
static double cruisemp(std::numeric_limits<double>::quiet_NaN());
static double cruisebhp(std::numeric_limits<double>::quiet_NaN());
static Engine *eng(0);
static Glib::ustring dir_main(""), dir_aux("");
static Engine::auxdb_mode_t auxdbmode(Engine::auxdb_prefs);
static std::string spreadsheetfile;
static bool parseexpand(false);
static bool useadr(false);
static bool recalc(false);

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

static void plotcharts(const FPlanRoute& route)
{
	if (!surface)
		return;
	if (pdfsurface)
		pdfsurface->set_size(width, height);
	Cairo::RefPtr<Cairo::Context> ctx(Cairo::Context::create(surface));
	ctx->translate(20, 20);
	{
		double pagedist(route.total_distance_nmi_dbl());
		unsigned int nrpages(1);
		if (!std::isnan(maxprofilepagedist) && maxprofilepagedist > 0 && pagedist > maxprofilepagedist) {
			nrpages = std::ceil(pagedist / maxprofilepagedist);
			pagedist /= nrpages;
		}
		MeteoProfile profile(route, topodb.get_profile(route, 5), eng->get_grib2_db().get_profile(route));
		double scaledist(profile.get_scaledist(ctx, width - 40, pagedist));
		double scaleelev, originelev;
		if (profileymode == MeteoProfile::yaxis_pressure) {
			scaleelev = profile.get_scaleelev(ctx, height - 40, 60000, profileymode);
			originelev = IcaoAtmosphere<double>::std_sealevel_pressure;
		} else {
			scaleelev = profile.get_scaleelev(ctx, height - 40, route.max_altitude() + 4000, profileymode);
			originelev = 0;
		}
		for (unsigned int pgnr(0); pgnr < nrpages; ++pgnr) {
			profile.draw(ctx, width - 40, height - 40, pgnr * pagedist, scaledist, originelev, scaleelev, profileymode);
			ctx->show_page();
			//surface->show_page();
		}
	}
	{
		MeteoChart chart(route, std::vector<FPlanAlternate>(), std::vector<FPlanRoute>(), eng->get_grib2_db());
		double width1(width), height1(height);
                Point center;
                double scalelon(1e-5), scalelat(1e-5);
		chart.get_fullscale(width1 - 40, height1 - 40, center, scalelon, scalelat);
		// check whether portrait is the better orientation for this route
		if (pdfsurface) {
			Point center1;
			double scalelon1(1e-5), scalelat1(1e-5);
			chart.get_fullscale(height - 40, width - 40, center1, scalelon1, scalelat1);
			if (fabs(scalelat1) > fabs(scalelat)) {
				center = center1;
				scalelon = scalelon1;
				scalelat = scalelat1;
				std::swap(width1, height1);
				pdfsurface->set_size(width1, height1);
				ctx = Cairo::Context::create(surface);
				ctx->translate(20, 20);
			}
		}
		static const unsigned int nrcharts = (sizeof(GRIB2::WeatherProfilePoint::isobaric_levels) /
						      sizeof(GRIB2::WeatherProfilePoint::isobaric_levels[0]));
		for (unsigned int i = 0; i < nrcharts; ++i) {
			chart.draw(ctx, width1 - 40, height1 - 40, center, scalelon, scalelat,
				   100.0 * GRIB2::WeatherProfilePoint::isobaric_levels[i]);
			ctx->show_page();
			//surface->show_page();
		}
	}
}

static void savespreadsheet(const FPlanRoute& route)
{
	if (spreadsheetfile.empty())
		return;
	acft.navfplan_gnumeric(spreadsheetfile, *eng, route, std::vector<FPlanAlternate>(),
			       Aircraft::Cruise::EngineParams(cruisebhp, cruiserpm, cruisemp),
			       std::numeric_limits<double>::quiet_NaN(), Aircraft::WeightBalance::elementvalues_t());
}

static void process_line(const Glib::ustring& line)
{
	if (!line.compare(0, 4, "adr+")) {
		useadr = true;
		outputchan->write("adr+\n");
		outputchan->flush();
		return;
	}
	if (!line.compare(0, 4, "adr-")) {
		useadr = false;
		outputchan->write("adr-\n");
		outputchan->flush();
		return;
	}
	if (!line.compare(0, 4, "adr?")) {
		outputchan->write(useadr ? "adr+\n" : "adr-\n");
		outputchan->flush();
		return;
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
		adrfpl.set_aircraft(acft, cruiserpm, cruisemp, cruisebhp);
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
		FPlanRoute route(*(FPlan *)0);
		for (ADR::FlightPlan::const_iterator ri(adrfpl.begin()), re(adrfpl.end()); ri != re; ++ri)
			route.insert_wpt(~0, *ri);
		if (route.get_nrwpt()) {
			route.set_time_offblock_unix(route[0].get_time_unix() - 5 * 60);
			route.set_time_onblock_unix(route[0].get_time_unix() + route[route.get_nrwpt() - 1].get_flighttime() + 5 * 60);
		}
		if (recalc) {
			route.gfs(eng->get_grib2_db());
			route.recompute(acft, std::numeric_limits<float>::quiet_NaN(), std::numeric_limits<float>::quiet_NaN(),
					Aircraft::Cruise::EngineParams(cruisebhp, cruiserpm, cruisemp));
		}
		// print final Flight Plan
		if (true) {
			for (unsigned int i(0), n(route.get_nrwpt()); i < n; ++i) {
				const FPlanWaypoint& wpt(route[i]);
				std::ostringstream oss;
				oss << "FFPL[" << i << "]: " << wpt.get_icao() << '/' << wpt.get_name() << ' ';
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
				    << ' ' << (wpt.is_ifr() ? 'I' : 'V') << "FR "
				    << std::setw(2) << std::setfill('0') << (wpt.get_flighttime() / 3600) << ':'
				    << std::setw(2) << std::setfill('0') << ((wpt.get_flighttime() / 60) % 60)
				    << " F" << std::fixed << std::setprecision(1) << wpt.get_fuel_usg()
				    << " TAS" << std::fixed << std::setprecision(0) << wpt.get_tas_kts();
				outputchan->write(oss.str() + "\n");
			}
		}
		plotcharts(route);
		savespreadsheet(route);
		outputchan->flush();
		return;
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
			return;
		}
		icaofpl.set_aircraft(acft, Aircraft::Cruise::EngineParams(cruisebhp, cruiserpm, cruisemp));
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
	if (true) {
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
	if (recalc) {
		fpl.gfs(eng->get_grib2_db());
		fpl.recompute(acft, std::numeric_limits<float>::quiet_NaN(), std::numeric_limits<float>::quiet_NaN(),
			      Aircraft::Cruise::EngineParams(cruisebhp, cruiserpm, cruisemp));
	}
	plotcharts(fpl);
	savespreadsheet(fpl);
        outputchan->flush();
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
	process_line(line);
	return true;
}

int main(int argc, char *argv[])
{
        try {
		Glib::init();
		Gio::init();
		Glib::set_application_name("Flight Plan Weather");
		Glib::thread_init();
		mainloop = Glib::MainLoop::create(Glib::MainContext::get_default());
		bool dis_aux(false), machine(false);
		std::string filename, fpl;
		bool svg(false);

		srand(time(0));

		{
			static struct option long_options[] = {
				{ "maindir", required_argument, 0, 'm' },
				{ "auxdir", required_argument, 0, 'a' },
				{ "disableaux", no_argument, 0, 'A' },
				{ "machineinterface", no_argument, 0, 0x100 },
				{ "expand", no_argument, 0, 0x101 },
				{ "adr", no_argument, 0, 0x102 },
				{ "aircraft", required_argument, 0, 0x103 },
				{ "rpm", required_argument, 0, 0x104 },
				{ "mp", required_argument, 0, 0x105 },
				{ "bhp", required_argument, 0, 0x106 },
				{ "recalculate", no_argument, 0, 0x107 },
				{ "fpl", required_argument, 0, 'f' },
				{ "height", required_argument, 0, 'h' },
				{ "width", required_argument, 0, 'w' },
				{ "maxdist", required_argument, 0, 0x109 },
				{ "ypressure", no_argument, 0, 0x10a },
				{ "pdf", required_argument, 0, 'p' },
				{ "svg", required_argument, 0, 's' },
				{ "spreadsheet", required_argument, 0, 0x108 },
				{ "ss", required_argument, 0, 0x108 },
				{0, 0, 0, 0}
			};
			int c, err(0);
			while ((c = getopt_long(argc, argv, "m:a:A45f:h:w:p:s:", long_options, 0)) != EOF) {
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

				case 'f':
					fpl = optarg;
					break;

				case 'h':
				        height = strtod(optarg, 0);
					break;

				case 'w':
				        width = strtod(optarg, 0);
					break;

				case 0x109:
					maxprofilepagedist = strtod(optarg, 0);
					break;

				case 0x10a:
					profileymode = MeteoProfile::yaxis_pressure;
					break;

				case 'p':
					svg = false;
					if (optarg)
						filename = optarg;
					break;

				case 's':
					svg = true;
					if (optarg)
						filename = optarg;
					break;

				case 0x103:
					if (optarg) {
						if (acft.load_file(optarg)) {
							if (acft.get_dist().recalculatepoly(false))
								std::cerr << "Aircraft " << acft.get_callsign() << " distance polynomials recalculated" << std::endl;
							if (acft.get_climb().recalculatepoly(false))
								std::cerr << "Aircraft " << acft.get_callsign() << " climb polynomials recalculated" << std::endl;
							if (acft.get_descent().recalculatepoly(false))
								std::cerr << "Aircraft " << acft.get_callsign() << " descent polynomials recalculated" << std::endl;
						} else {
							std::cerr << "Cannot load aircraft " << optarg << std::endl;
						}
					}
					break;

				case 0x104:
					if (optarg)
						cruiserpm = strtod(optarg, 0);
					break;

				case 0x105:
					if (optarg)
						cruisemp = strtod(optarg, 0);
					break;

				case 0x106:
					if (optarg)
						cruisebhp = strtod(optarg, 0);
					break;

				case 0x107:
					recalc = true;
					break;

				case 0x108:
					if (optarg)
						spreadsheetfile = optarg;
					break;

				default:
					err++;
					break;
				}
			}
			if (err) {
				std::cerr << "usage: weatherfplan [-m <maindir>] [-a <auxdir>] [-A] [-4] [-5] [-f <fpl>] [-p <pdffile>] [-s <svgfile>] [-w <width>] [-h <height>]" << std::endl;
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
		topodb.open(dir_aux.empty() ? Engine::get_default_aux_dir() : (std::string)dir_aux);
		if (std::isnan(cruiserpm) && std::isnan(cruisemp) && std::isnan(cruisebhp)) {
			cruiserpm = 2400;
			cruisebhp = acft.get_maxbhp() * 0.65;
		}
		// load wx non-threaded
		{
			std::string path1(dir_main);
			if (path1.empty())
				path1 = FPlan::get_userdbpath();
			path1 = Glib::build_filename(path1, "gfs");
			std::string path2(dir_aux);
			if (path2.empty()) {
				GRIB2::Parser p(eng->get_grib2_db());
				p.parse(path1);
				unsigned int count(eng->get_grib2_db().find_layers().size());
				if (true)
					std::cout << "Loaded " << count << " GRIB2 Layers from " << path1 << std::endl;
			} else {
				path2 = Glib::build_filename(path2, "gfs");
				GRIB2::Parser p(eng->get_grib2_db());
				p.parse(path1);
				unsigned int count1(eng->get_grib2_db().find_layers().size());
				p.parse(path2);
				unsigned int count2(eng->get_grib2_db().find_layers().size() - count1);
				if (true)
					std::cout << "Loaded " << count1 << " GRIB2 Layers from " << path1
						  << " and " << count2 << " GRIB2 Layers from " << path2 << std::endl;
			}
		}
		if (filename.empty()) {
			if (spreadsheetfile.empty()) {
				std::cerr << "No graphics or spreadsheet file given" << std::endl;
				return EX_USAGE;
			}
		} else {
			if (height < 60 || width < 60) {
				std::cerr << "Invalid height / width" << std::endl;
				return EX_USAGE;
			}
			if (svg) {
				surface = Cairo::SvgSurface::create(filename, width, height);
			} else {
				surface = pdfsurface = Cairo::PdfSurface::create(filename, width, height);
			}
		}
		std::cerr << "Using aircraft " << acft.get_callsign() << std::endl;
		inputchan = Glib::IOChannel::create_from_fd(0);
		outputchan = Glib::IOChannel::create_from_fd(1);
		inputchan->set_encoding();
		outputchan->set_encoding();
		if (fpl.empty()) {
			if (machine) {
				delete eng;
				eng = 0;
				cmdlist["nop"] = cmd_nop;
				cmdlist["quit"] = cmd_quit;
				Glib::signal_io().connect(sigc::ptr_fun(&machine_input), inputchan, Glib::IO_IN | Glib::IO_ERR | Glib::IO_HUP);
				{
					Command cmd("weatherfplan");
					cmd.set_option("version", VERSION);
					machine_writecmd(cmd);
				}
				outputchan->flush();
				mainloop->run();
				return EX_OK;
			}
			Glib::signal_io().connect(sigc::ptr_fun(&normal_input), inputchan, Glib::IO_IN | Glib::IO_ERR | Glib::IO_HUP);
			mainloop->run();
		} else {
			process_line(fpl);
		}
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
