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
#include <time.h>

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

class CFMUSIDSTAR {
public:
	CFMUSIDSTAR(int& argc, char**& argv);
	~CFMUSIDSTAR();
	void set_dbdirs(const std::string& dir_main = "", Engine::auxdb_mode_t auxdbmode = Engine::auxdb_prefs, const std::string& dir_aux = "");
	void init(void);
	void run(void);	
	void run(const std::string& id);
	void run(const std::string& aspcid, const std::string& aspctype);

protected:
	class RoutePoint {
	public:
		RoutePoint(const std::string& name = "", const Point& pt = Point()) : m_name(name), m_pt(pt) {}
		RoutePoint(const NavaidsDb::Navaid& el) : m_name(el.get_icao()), m_pt(el.get_coord()) {}
		RoutePoint(const WaypointsDb::Waypoint& el) : m_name(el.get_name()), m_pt(el.get_coord()) {}
		const std::string& get_name(void) const { return m_name; }
		const Point& get_pt(void) const { return m_pt; }
		bool operator<(const RoutePoint& x) const;
		bool is_same(const AirportsDb::Airport::VFRRoute::VFRRoutePoint& pt) const;

	protected:
		std::string m_name;
		Point m_pt;
	};

	std::string m_dir_main;
	std::string m_dir_aux;
        AirportsDb m_airportdb;
        NavaidsDb m_navaiddb;
        WaypointsDb m_waypointdb;
        AirspacesDb m_airspacedb;
	MultiPolygonHole m_ecac;
	Glib::RefPtr<Gio::Socket> m_childsock;
#ifdef G_OS_UNIX
	Glib::RefPtr<Gio::UnixSocketAddress> m_childsockaddr;
#else
	Glib::RefPtr<Gio::InetSocketAddress> m_childsockaddr;
#endif
	std::string m_childstdout;
	std::string m_childbinary;
	sigc::connection m_connchildtimeout;
	sigc::connection m_connchildwatch;
	sigc::connection m_connchildstdout;
	Glib::RefPtr<Glib::IOChannel> m_childchanstdin;
	Glib::RefPtr<Glib::IOChannel> m_childchanstdout;
	Glib::Pid m_childpid;
	unsigned int m_childtimeoutcount;
	int m_childxdisplay;
	bool m_childrun;
	typedef enum {
		validationstate_idle,
		validationstate_wait,
		validationstate_done,
		validationstate_error
	} validationstate_t;
	validationstate_t m_validationstate;
	typedef std::vector<std::string> validationresponse_t;
	validationresponse_t m_validationresponse;
	std::string m_validationfpl;
	double m_sidstarradius;

	Glib::ustring m_oppositearpt;
	Glib::ustring m_oppositewpt;
	Point m_oppositearptcoord;
	Point m_oppositewptcoord;

	bool m_continue;

	void child_watch(GPid pid, int child_status);
	void child_run(void);
	void child_close(void);
	bool child_stdout_handler(Glib::IOCondition iocond);
	bool child_socket_handler(Glib::IOCondition iocond);
	bool child_timeout(void);
	void child_send(const std::string& tx);
	bool child_is_running(void) const;
	void child_configure(void);
	unsigned int child_get_timeout(void) const { return m_childrun ? 120 : 60; }
	static const unsigned int child_restarts = 10;
	void opendb(void);
	void closedb(void);
	void run(AirportsDb::Airport& arpt, bool force = true);	
	validationresponse_t validate(const std::string& fpl);
	void clear_sid(AirportsDb::Airport& arpt);
	void clear_star(AirportsDb::Airport& arpt);
	void clear_sid(AirportsDb::Airport& arpt, const RoutePoint& rpt);
	void clear_star(AirportsDb::Airport& arpt, const RoutePoint& rpt);
	void add_sid(AirportsDb::Airport& arpt, const RoutePoint& rpt, int32_t minalt, int32_t maxalt);
	void add_star(AirportsDb::Airport& arpt, const RoutePoint& rpt, int32_t minalt, int32_t maxalt);
	void try_point(AirportsDb::Airport& arpt, const RoutePoint& rpt, bool dep);
};

bool CFMUSIDSTAR::RoutePoint::operator<(const RoutePoint& x) const
{
	int c(get_name().compare(x.get_name()));
	if (c < 0)
		return true;
	if (0 < c)
		return false;
	if (get_pt().get_lat() < x.get_pt().get_lat())
		return true;
	if (x.get_pt().get_lat() < get_pt().get_lat())
		return true;
	return get_pt().get_lon() < x.get_pt().get_lon();
}

bool CFMUSIDSTAR::RoutePoint::is_same(const AirportsDb::Airport::VFRRoute::VFRRoutePoint& pt) const
{
	if (get_name() != (std::string)pt.get_name())
		return false;
	return get_pt().simple_distance_nmi(pt.get_coord()) < 0.1f;
}

const unsigned int CFMUSIDSTAR::child_restarts;

CFMUSIDSTAR::CFMUSIDSTAR(int& argc, char**& argv)
	: m_childtimeoutcount(0), m_childxdisplay(-1), m_childrun(false), m_validationstate(validationstate_idle), m_sidstarradius(50),
	  m_oppositearpt("LSZF"), m_oppositewpt(""), m_continue(false)
{
#ifdef __MINGW32__
	std::string workdir;
	{
		gchar *instdir(g_win32_get_package_installation_directory_of_module(0));
		if (instdir)
			m_childbinary = Glib::build_filename(instdir, "bin", "cfmuvalidateserver.exe");
		else
			m_childbinary = Glib::build_filename(PACKAGE_PREFIX_DIR "/bin", "cfmuvalidateserver.exe");
	}
#else
	m_childbinary = Glib::build_filename(PACKAGE_LIBEXEC_DIR, "cfmuvalidateserver");
#endif
#ifdef G_OS_UNIX
	std::string sockaddr(PACKAGE_RUN_DIR "/validator/socket");
#else
	std::string sockaddr("53181");
#endif
	std::string dir_main(""), dir_aux("");
	Engine::auxdb_mode_t auxdbmode = Engine::auxdb_prefs;
	bool dis_aux(false);
	Glib::OptionGroup optgroup("cfmusidstar", "CFMU SID/STAR Utility Options", "Options controlling the CFMU SID/STAR Utility");
	Glib::OptionEntry optmaindir, optauxdir, optdisableaux, optxdisplay, optsidstarradius, optvalbinary, optvalsocket;
	Glib::OptionEntry optopparpt, optoppwpt, optcont;
	optmaindir.set_short_name('m');
	optmaindir.set_long_name("maindir");
	optmaindir.set_description("Directory containing the main database");
	optmaindir.set_arg_description("DIR");
	optauxdir.set_short_name('a');
	optauxdir.set_long_name("auxdir");
	optauxdir.set_description("Directory containing the auxilliary database");
	optauxdir.set_arg_description("DIR");
	optdisableaux.set_short_name('A');
	optdisableaux.set_long_name("disableaux");
	optdisableaux.set_description("Disable the auxilliary database");
	optdisableaux.set_arg_description("BOOL");
	optxdisplay.set_short_name('x');
	optxdisplay.set_long_name("xdisplay");
	optxdisplay.set_description("Set X Display number");
	optxdisplay.set_arg_description("INT");
	optsidstarradius.set_short_name('r');
	optsidstarradius.set_long_name("radius");
	optsidstarradius.set_description("Set SID/STAR Radius");
	optsidstarradius.set_arg_description("FLOAT");
	optvalbinary.set_short_name('B');
	optvalbinary.set_long_name("validator-binary");
	optvalbinary.set_description("Validator Binary");
	optvalbinary.set_arg_description("DIR");
	optvalsocket.set_short_name('S');
	optvalsocket.set_long_name("validator-socket");
	optvalsocket.set_description("Validator Socket");
	optvalsocket.set_arg_description("DIR");
	optopparpt.set_short_name('o');
	optopparpt.set_long_name("opposite-airport");
	optopparpt.set_description("Opposite Airport");
	optopparpt.set_arg_description("STRING");
	optoppwpt.set_short_name('O');
	optoppwpt.set_long_name("opposite-wpt");
	optoppwpt.set_description("Opposite Waypoint");
	optoppwpt.set_arg_description("STRING");
	optcont.set_short_name('c');
	optcont.set_long_name("continue");
	optcont.set_description("Continue");
	optcont.set_arg_description("BOOL");
	optgroup.add_entry_filename(optmaindir, dir_main);
	optgroup.add_entry_filename(optauxdir, dir_aux);
	optgroup.add_entry(optdisableaux, dis_aux);
	optgroup.add_entry(optxdisplay, m_childxdisplay);
	optgroup.add_entry(optsidstarradius, m_sidstarradius);
	optgroup.add_entry_filename(optvalbinary, m_childbinary);
	optgroup.add_entry_filename(optvalsocket, sockaddr);
	optgroup.add_entry(optopparpt, m_oppositearpt);
	optgroup.add_entry(optoppwpt, m_oppositewpt);
	optgroup.add_entry(optcont, m_continue);
	Glib::OptionContext optctx("[--maindir=<dir>] [--auxdir=<dir>] [--disableaux] [--xdisplay=<disp>] [--radius=<r>] "
				   "[--validator-socket=<sock>] [--validator-binary=<bin>] [--opposite-airport=<arpt>] "
				   "[--opposite-wpt=<wpt>] [--continue]");
	optctx.set_help_enabled(true);
	optctx.set_ignore_unknown_options(false);
	optctx.set_main_group(optgroup);
	if (!optctx.parse(argc, argv)) {
		std::ostringstream oss;
		oss << "Command Line Option Error: Usage: " << Glib::get_prgname() << std::endl
		    << optctx.get_help(false);
		throw std::runtime_error(oss.str());
	}
	if (dis_aux)
		auxdbmode = Engine::auxdb_none;
	else if (!dir_aux.empty())
		auxdbmode = Engine::auxdb_override;
	set_dbdirs(dir_main, auxdbmode, dir_aux);
#ifdef G_OS_UNIX
	m_childsockaddr = Gio::UnixSocketAddress::create(sockaddr, Gio::UNIX_SOCKET_ADDRESS_PATH);
#else
	m_childsockaddr = Gio::InetSocketAddress::create(Gio::InetAddress::create_loopback(Gio::SOCKET_FAMILY_IPV4), strtoul(sockaddr.c_str(), 0, 0));
#endif
}

CFMUSIDSTAR::~CFMUSIDSTAR()
{
        child_close();
}

bool CFMUSIDSTAR::child_is_running(void) const
{
	return m_childrun || m_childsock;
}

void CFMUSIDSTAR::child_run(void)
{
	if (child_is_running())
		return;
	m_childstdout.clear();
	std::cerr << "Connecting to validation server..." << std::endl;
	if (m_childsockaddr) {
		m_childsock = Gio::Socket::create(m_childsockaddr->get_family(), Gio::SOCKET_TYPE_STREAM, Gio::SOCKET_PROTOCOL_DEFAULT);
		try {
			m_childsock->connect(m_childsockaddr);
		} catch (...) {
			m_childsock.reset();
		}
		if (m_childsock) {
			m_connchildstdout = Glib::signal_io().connect(sigc::mem_fun(this, &CFMUSIDSTAR::child_socket_handler), m_childsock->get_fd(), Glib::IO_IN | Glib::IO_ERR | Glib::IO_HUP);
			child_configure();
			return;
		}
	}
	if (m_childbinary.empty()) {
		m_childrun = false;
		std::cerr << "Cannot run validator: no validator configured" << std::endl;
		return;
	}
	int childstdin(-1), childstdout(-1);
	try {
		std::vector<std::string> argv, env;
#ifdef __MINGW32__
		std::string workdir;
		{
			gchar *instdir(g_win32_get_package_installation_directory_of_module(0));
			if (instdir)
				workdir = instdir;
			else
				workdir = PACKAGE_PREFIX_DIR;
		}
#else
		std::string workdir(PACKAGE_DATA_DIR);
		env.push_back("PATH=/bin:/usr/bin");
		if (m_childxdisplay >= 0) {
			std::ostringstream oss;
			oss << "DISPLAY=:" << m_childxdisplay;
			env.push_back(oss.str());
		} else {
			bool found(false);
			std::string disp(Glib::getenv("DISPLAY", found));
			if (found)
				env.push_back("DISPLAY=" + disp);
		}
#endif
		argv.push_back(m_childbinary);
		if (m_childxdisplay >= 0) {
			std::ostringstream oss;
			oss << "--xdisplay=" << m_childxdisplay;
			argv.push_back(oss.str());
		}
		if (false) {
			std::cerr << "Spawn: workdir " << workdir << " argv:" << std::endl;
			for (std::vector<std::string>::const_iterator i(argv.begin()), e(argv.end()); i != e; ++i)
				std::cerr << "  " << *i << std::endl;
			std::cerr << "env:" << std::endl;
			for (std::vector<std::string>::const_iterator i(env.begin()), e(env.end()); i != e; ++i)
				std::cerr << "  " << *i << std::endl;
		}
#ifdef __MINGW32__
		// inherit environment; otherwise, this may fail on WindowsXP with STATUS_SXS_ASSEMBLY_NOT_FOUND (C0150004)
		Glib::spawn_async_with_pipes(workdir, argv,
					     Glib::SPAWN_DO_NOT_REAP_CHILD | Glib::SPAWN_STDERR_TO_DEV_NULL,
					     sigc::slot<void>(), &m_childpid, &childstdin, &childstdout, 0);
#else
		Glib::spawn_async_with_pipes(workdir, argv, env,
					     Glib::SPAWN_DO_NOT_REAP_CHILD | Glib::SPAWN_STDERR_TO_DEV_NULL,
					     sigc::slot<void>(), &m_childpid, &childstdin, &childstdout, 0);
#endif
		m_childrun = true;
	} catch (Glib::SpawnError& e) {
		m_childrun = false;
		std::cerr << "Cannot run validator: " << e.what() << std::endl;
		return;
	}
	m_connchildwatch = Glib::signal_child_watch().connect(sigc::mem_fun(this, &CFMUSIDSTAR::child_watch), m_childpid);
	m_childchanstdin = Glib::IOChannel::create_from_fd(childstdin);
	m_childchanstdout = Glib::IOChannel::create_from_fd(childstdout);
	m_childchanstdin->set_encoding();
	m_childchanstdout->set_encoding();
	m_childchanstdin->set_close_on_unref(true);
	m_childchanstdout->set_close_on_unref(true);
	m_connchildstdout = Glib::signal_io().connect(sigc::mem_fun(this, &CFMUSIDSTAR::child_stdout_handler), m_childchanstdout,
						      Glib::IO_IN | Glib::IO_ERR | Glib::IO_HUP);
	child_configure();
}

bool CFMUSIDSTAR::child_stdout_handler(Glib::IOCondition iocond)
{
        Glib::ustring line;
        Glib::IOStatus iostat(m_childchanstdout->read_line(line));
        if (iostat == Glib::IO_STATUS_ERROR ||
            iostat == Glib::IO_STATUS_EOF) {
		if (iostat == Glib::IO_STATUS_ERROR)
			std::cerr << "Child stdout error, stopping..." << std::endl;
		else
		        std::cerr << "Child stdout eof, stopping..." << std::endl;
                child_close();
		m_validationstate = validationstate_error;
		return true;
        }
	m_childtimeoutcount = 0;
	if (false)
		std::cerr << "validator rx: " << line << std::endl;
	{
		std::string::size_type n(line.size());
		while (n > 0) {
			--n;
			if (!isspace(line[n]) && line[n] != '\n' && line[n] != '\r') {
				++n;
				break;
			}
		}
		line = std::string(line, 0, n);
	}
	if (!line.empty())
		m_validationresponse.push_back(line);
	if (false)
		std::cerr << "validator: " << line << std::endl;
	if (line.empty()) {
		if (false)
			std::cerr << "validator: response finished" << std::endl;
		m_validationstate = validationstate_done;
	}
        return true;
}

bool CFMUSIDSTAR::child_socket_handler(Glib::IOCondition iocond)
{
	{
		char buf[4096];
		gssize r(m_childsock->receive_with_blocking(buf, sizeof(buf), false));
		if (r < 0) {
			std::cerr << "Child stdout error, stopping..." << std::endl;
			child_close();
			m_validationstate = validationstate_error;
			return true;
		}
		if (!r) {
			child_close();
			child_timeout();
			return true;
		}
		m_childstdout += std::string(buf, buf + r);
		if (false)
			std::cerr << "validator rx: " << std::string(buf, buf + r) << std::endl;
	}
	m_childtimeoutcount = 0;
	for (;;) {
		std::string::size_type n(m_childstdout.find_first_of("\r\n"));
		if (n == std::string::npos)
			break;
		std::string line(m_childstdout, 0, n);
		{
			uint8_t flg(0);
			while (n < m_childstdout.size()) {
				bool quit(false);
				switch (m_childstdout[n]) {
				case '\r':
					quit = quit || (flg & 1);
					flg |= 1;
					break;

				case '\n':
					quit = quit || (flg & 2);
					flg |= 2;
					break;

				default:
					quit = true;
					break;
				}
				if (quit)
					break;
				++n;
			}
		}
		m_childstdout.erase(0, n);
		if (!line.empty())
			m_validationresponse.push_back(line);
		if (false)
			std::cerr << "validator: " << line << std::endl;
		if (line.empty()) {
			if (false)
				std::cerr << "validator: response finished" << std::endl;
			m_validationstate = validationstate_done;
		}
	}
	return true;
}

void CFMUSIDSTAR::child_watch(GPid pid, int child_status)
{
	if (!m_childrun)
		return;
	if (m_childpid != pid) {
		Glib::spawn_close_pid(pid);
		return;
	}
	Glib::spawn_close_pid(pid);
	m_childrun = false;
	m_connchildwatch.disconnect();
	m_connchildstdout.disconnect();
	child_close();
	child_timeout();
}

void CFMUSIDSTAR::child_close(void)
{
	m_connchildstdout.disconnect();
	m_childstdout.clear();
	if (m_childsock) {
		m_childsock->close();
		m_childsock.reset();
	}
	if (m_childchanstdin) {
		m_childchanstdin->close();
		m_childchanstdin.reset();
	}
	if (m_childchanstdout) {
		m_childchanstdout->close();
		m_childchanstdout.reset();
	}
	if (!m_childrun)
		return;
	m_childrun = false;
	m_connchildwatch.disconnect();
	Glib::signal_child_watch().connect(sigc::hide(sigc::ptr_fun(&Glib::spawn_close_pid)), m_childpid);
}

void CFMUSIDSTAR::child_send(const std::string& tx)
{
	if (false)
		std::cerr << "validator tx: " << tx << std::endl;
	if (m_childsock) {
		gssize r(m_childsock->send_with_blocking(const_cast<char *>(tx.c_str()), tx.size(), true));
		if (r == -1) {
			child_close();
			child_timeout();
		}
	} else if (m_childchanstdin) {
		m_childchanstdin->write(tx);
		m_childchanstdin->flush();
	}
}

bool CFMUSIDSTAR::child_timeout(void)
{
        m_connchildtimeout.disconnect();
        ++m_childtimeoutcount;
        if (m_childtimeoutcount > child_restarts) {
		std::cerr << "Child timed out, stopping..." << std::endl;
		m_validationstate = validationstate_error;
                child_close();
                return false;
        }
        child_close();
        child_run();
        if (!child_is_running()) {
		std::cerr << "Cannot restart child after timeout, stopping..." << std::endl;
		m_validationstate = validationstate_error;
                return false;
        }
        child_send(m_validationfpl + "\n");
        m_connchildtimeout = Glib::signal_timeout().connect_seconds(sigc::mem_fun(*this, &CFMUSIDSTAR::child_timeout), child_get_timeout());
        return true;
}

void CFMUSIDSTAR::child_configure(void)
{
	child_send("validate*:cfmu\n");
	//child_send("validate*:eurofpl\n");
}

CFMUSIDSTAR::validationresponse_t CFMUSIDSTAR::validate(const std::string& fpl)
{
        m_connchildtimeout.disconnect();
	m_validationfpl = fpl;
	m_validationresponse.clear();
	m_validationstate = validationstate_wait;
        if (!child_is_running()) {
                child_close();
                child_run();
        }
        if (!child_is_running()) {
		std::cerr << "Cannot restart child, stopping..." << std::endl;
		m_validationstate = validationstate_error;
                throw std::runtime_error("validation error");
        }
        child_send(m_validationfpl + "\n");
        m_connchildtimeout = Glib::signal_timeout().connect_seconds(sigc::mem_fun(*this, &CFMUSIDSTAR::child_timeout), child_get_timeout());
	Glib::RefPtr<Glib::MainContext> ctx(Glib::MainContext::get_default());
	while (m_validationstate == validationstate_wait) {
		ctx->iteration(true);
	}
	if (m_validationstate == validationstate_error) {
                throw std::runtime_error("validation error");
	}
	return m_validationresponse;
}

void CFMUSIDSTAR::set_dbdirs(const std::string& dir_main, Engine::auxdb_mode_t auxdbmode, const std::string& dir_aux)
{
	m_dir_main = dir_main;
        switch (auxdbmode) {
	case Engine::auxdb_prefs:
	{
		Preferences prefs;
		m_dir_aux = (Glib::ustring)prefs.globaldbdir;
		break;
	}

	case Engine::auxdb_override:
		m_dir_aux = dir_aux;
		break;

	default:
		m_dir_aux.clear();
		break;
        }
}

void CFMUSIDSTAR::opendb(void)
{
	closedb();
	m_airportdb.open(m_dir_main);
	if (!m_dir_aux.empty() && m_airportdb.is_dbfile_exists(m_dir_aux))
		m_airportdb.attach(m_dir_aux);
	m_navaiddb.open(m_dir_main);
	if (!m_dir_aux.empty() && m_navaiddb.is_dbfile_exists(m_dir_aux))
		m_navaiddb.attach(m_dir_aux);
	m_waypointdb.open(m_dir_main);
	if (!m_dir_aux.empty() && m_waypointdb.is_dbfile_exists(m_dir_aux))
		m_waypointdb.attach(m_dir_aux);
	m_airspacedb.open(m_dir_main);
	if (!m_dir_aux.empty() && m_airspacedb.is_dbfile_exists(m_dir_aux))
		m_airspacedb.attach(m_dir_aux);
}

void CFMUSIDSTAR::closedb(void)
{
        m_airportdb.close();
        m_navaiddb.close();
	m_waypointdb.close();
	m_airspacedb.close();
}

void CFMUSIDSTAR::init(void)
{
	opendb();
	{
		m_ecac.clear();
		AirspacesDb::elementvector_t ev(m_airspacedb.find_by_icao("ECAC1", 0, AirspacesDb::comp_exact, 0, AirspacesDb::Airspace::subtables_all));
		for (AirspacesDb::elementvector_t::const_iterator evi(ev.begin()), eve(ev.end()); evi != eve; ++evi) {
			if (evi->get_typecode() != AirspacesDb::Airspace::typecode_ead || evi->get_bdryclass() != AirspacesDb::Airspace::bdryclass_ead_part)
				continue;
			m_ecac = evi->get_polygon();
			break;
		}
	}
	if (m_ecac.empty())
		throw std::runtime_error("Cannot find airspace ECAC1");
	m_oppositearptcoord.set_invalid();
	m_oppositewptcoord.set_invalid();
	if (!m_oppositearpt.empty()) {
		{
			AirportsDb::elementvector_t ev(m_airportdb.find_by_icao(m_oppositearpt, 0, AirportsDb::comp_exact, 2, AirportsDb::element_t::subtables_none));
			if (ev.empty())
				throw std::runtime_error("Airport " + m_oppositearpt + " not found");
			if (ev.size() != 1)
				throw std::runtime_error("Airport " + m_oppositearpt + " not unique");
			m_oppositearpt = ev[0].get_icao();
			m_oppositearptcoord = ev[0].get_coord();
		}
		// find opposite waypoint
		{
			WaypointsDb::elementvector_t ev;
			if (m_oppositewpt.empty())
				ev = m_waypointdb.find_by_rect(m_oppositearptcoord.simple_box_nmi(50), 0, WaypointsDb::Waypoint::subtables_none);
			else
				ev = m_waypointdb.find_by_name(m_oppositewpt, 0, WaypointsDb::comp_exact, 2, WaypointsDb::element_t::subtables_none);
			double dist(std::numeric_limits<double>::max());
			for (WaypointsDb::elementvector_t::const_iterator evi(ev.begin()), eve(ev.end()); evi != eve; ++evi) {
				if (evi->get_type() != WaypointsDb::Waypoint::type_icao)
					continue;
				double d(evi->get_coord().spheric_distance_nmi(m_oppositearptcoord));
				if (d > dist)
					continue;
				dist = d;
				m_oppositewpt = evi->get_name();
				m_oppositewptcoord = evi->get_coord();
			}
			if (m_oppositewptcoord.is_invalid())
				throw std::runtime_error("Cannot find opposite waypoint");
		}
	}
	if (true) {
		Rect bbox(m_ecac.get_bbox());
		std::cerr << "ECAC1: " << bbox.get_southwest().get_lat_str2() << ' ' << bbox.get_southwest().get_lon_str2() << " - "
			  << bbox.get_northeast().get_lat_str2() << ' ' << bbox.get_northeast().get_lon_str2() << std::endl;
	}
	child_run();
}

void CFMUSIDSTAR::clear_sid(AirportsDb::Airport& arpt)
{
	for (unsigned int i = 0, n = arpt.get_nrvfrrte(); i < n;) {
		AirportsDb::Airport::VFRRoute& rte(arpt.get_vfrrte(i));
		if (!rte.size()) {
			++i;
			continue;
		}
		const AirportsDb::Airport::VFRRoute::VFRRoutePoint& pt(rte[rte.size() - 1U]);
		if (pt.get_pathcode() != AirportsDb::Airport::VFRRoute::VFRRoutePoint::pathcode_sid) {
			++i;
			continue;
		}
		arpt.erase_vfrrte(i);
		--n;
	}
	arpt.set_flightrules(arpt.get_flightrules() & ~AirportsDb::Airport::flightrules_dep_ifr);
}

void CFMUSIDSTAR::clear_star(AirportsDb::Airport& arpt)
{
	for (unsigned int i = 0, n = arpt.get_nrvfrrte(); i < n;) {
		AirportsDb::Airport::VFRRoute& rte(arpt.get_vfrrte(i));
		if (!rte.size()) {
			++i;
			continue;
		}
		const AirportsDb::Airport::VFRRoute::VFRRoutePoint& pt(rte[0U]);
		if (pt.get_pathcode() != AirportsDb::Airport::VFRRoute::VFRRoutePoint::pathcode_star) {
			++i;
			continue;
		}
		arpt.erase_vfrrte(i);
		--n;
	}
	arpt.set_flightrules(arpt.get_flightrules() & ~AirportsDb::Airport::flightrules_arr_ifr);
}

void CFMUSIDSTAR::clear_sid(AirportsDb::Airport& arpt, const RoutePoint& rpt)
{
	for (unsigned int i = 0, n = arpt.get_nrvfrrte(); i < n;) {
		AirportsDb::Airport::VFRRoute& rte(arpt.get_vfrrte(i));
		if (!rte.size()) {
			++i;
			continue;
		}
		const AirportsDb::Airport::VFRRoute::VFRRoutePoint& pt(rte[rte.size() - 1U]);
		if (pt.get_pathcode() != AirportsDb::Airport::VFRRoute::VFRRoutePoint::pathcode_sid) {
			++i;
			continue;
		}
		if (!rpt.is_same(pt)) {
			++i;
			continue;
		}
		arpt.erase_vfrrte(i);
		n = arpt.get_nrvfrrte();
	}
}

void CFMUSIDSTAR::clear_star(AirportsDb::Airport& arpt, const RoutePoint& rpt)
{
	for (unsigned int i = 0, n = arpt.get_nrvfrrte(); i < n;) {
		AirportsDb::Airport::VFRRoute& rte(arpt.get_vfrrte(i));
		if (!rte.size()) {
			++i;
			continue;
		}
		const AirportsDb::Airport::VFRRoute::VFRRoutePoint& pt(rte[0U]);
		if (pt.get_pathcode() != AirportsDb::Airport::VFRRoute::VFRRoutePoint::pathcode_star) {
			++i;
			continue;
		}
		if (!rpt.is_same(pt)) {
			++i;
			continue;
		}
		arpt.erase_vfrrte(i);
		n = arpt.get_nrvfrrte();
	}
}


void CFMUSIDSTAR::add_sid(AirportsDb::Airport& arpt, const RoutePoint& rpt, int32_t minalt, int32_t maxalt)
{
	arpt.set_flightrules(arpt.get_flightrules() | AirportsDb::Airport::flightrules_dep_ifr);
	bool added(false);
	for (unsigned int i = 0, n = arpt.get_nrvfrrte(); i < n; ++i) {
		AirportsDb::Airport::VFRRoute& rte(arpt.get_vfrrte(i));
		if (!rte.size())
			continue;
		const AirportsDb::Airport::VFRRoute::VFRRoutePoint& pt(rte[rte.size() - 1U]);
		if (pt.get_pathcode() != AirportsDb::Airport::VFRRoute::VFRRoutePoint::pathcode_sid)
			continue;
		if (!rpt.is_same(pt))
			continue;
		rte.set_minalt(minalt);
		rte.set_maxalt(maxalt);
		added = true;
	}
	if (added)
		return;
	AirportsDb::Airport::VFRRoute rte /*(rpt.get_name() + "0Z")*/;
	rte.set_minalt(minalt);
	rte.set_maxalt(maxalt);
	rte.add_point(AirportsDb::Airport::VFRRoute::VFRRoutePoint(arpt.get_icao(), arpt.get_coord(), arpt.get_elevation(), AirportsDb::Airport::label_off, ' ',
								   AirportsDb::Airport::VFRRoute::VFRRoutePoint::pathcode_sid,
								   AirportsDb::Airport::VFRRoute::VFRRoutePoint::altcode_assigned, true));
	rte.add_point(AirportsDb::Airport::VFRRoute::VFRRoutePoint(rpt.get_name(), rpt.get_pt(), minalt, AirportsDb::Airport::label_e, 'C',
								   AirportsDb::Airport::VFRRoute::VFRRoutePoint::pathcode_sid,
								   AirportsDb::Airport::VFRRoute::VFRRoutePoint::altcode_atorabove, false));
	arpt.add_vfrrte(rte);
}

void CFMUSIDSTAR::add_star(AirportsDb::Airport& arpt, const RoutePoint& rpt, int32_t minalt, int32_t maxalt)
{
	arpt.set_flightrules(arpt.get_flightrules() | AirportsDb::Airport::flightrules_arr_ifr);
	bool added(false);
	for (unsigned int i = 0, n = arpt.get_nrvfrrte(); i < n; ++i) {
		AirportsDb::Airport::VFRRoute& rte(arpt.get_vfrrte(i));
		if (!rte.size())
			continue;
		const AirportsDb::Airport::VFRRoute::VFRRoutePoint& pt(rte[0U]);
		if (pt.get_pathcode() != AirportsDb::Airport::VFRRoute::VFRRoutePoint::pathcode_star)
			continue;
		if (!rpt.is_same(pt))
			continue;
		rte.set_minalt(minalt);
		rte.set_maxalt(maxalt);
		added = true;
	}
	if (added)
		return;
	AirportsDb::Airport::VFRRoute rte /*(rpt.get_name() + "0Z")*/;
	rte.set_minalt(minalt);
	rte.set_maxalt(maxalt);
	rte.add_point(AirportsDb::Airport::VFRRoute::VFRRoutePoint(rpt.get_name(), rpt.get_pt(), minalt, AirportsDb::Airport::label_e, 'C',
								   AirportsDb::Airport::VFRRoute::VFRRoutePoint::pathcode_star,
								   AirportsDb::Airport::VFRRoute::VFRRoutePoint::altcode_atorabove, false));
	rte.add_point(AirportsDb::Airport::VFRRoute::VFRRoutePoint(arpt.get_icao(), arpt.get_coord(), arpt.get_elevation(), AirportsDb::Airport::label_off, ' ',
								   AirportsDb::Airport::VFRRoute::VFRRoutePoint::pathcode_star,
								   AirportsDb::Airport::VFRRoute::VFRRoutePoint::altcode_assigned, true));
	arpt.add_vfrrte(rte);
}

void CFMUSIDSTAR::run(void)
{
	std::cerr << "Processing all airports" << std::endl;
	AirportsDb::Airport arpt;
	m_airportdb.loadfirst(arpt, true, AirportsDb::Airport::subtables_all);
	while (arpt.is_valid()) {
		if (!(arpt.get_flightrules() & (AirportsDb::Airport::flightrules_arr_vfr | AirportsDb::Airport::flightrules_arr_ifr)) ||
		    !(arpt.get_flightrules() & (AirportsDb::Airport::flightrules_dep_vfr | AirportsDb::Airport::flightrules_dep_ifr)))
			run(arpt, false);
                m_airportdb.loadnext(arpt, true, AirportsDb::Airport::subtables_all);
        }
}

void CFMUSIDSTAR::run(const std::string& id)
{
	std::string id1(Glib::ustring(id).uppercase());
	std::cerr << "Processing " << id1 << std::endl;
	{
		Point pt;
		if (!((Point::setstr_lat | Point::setstr_lon) & ~pt.set_str(id1))) {
			uint64_t dmin(std::numeric_limits<uint64_t>::max());
			unsigned int imin(0);
			AirportsDb::elementvector_t ev(m_airportdb.find_by_rect(pt.simple_box_nmi(50), 0, AirportsDb::Airport::subtables_all));
			for (unsigned int i = 0; i < ev.size(); ++i) {
				uint64_t d(pt.simple_distance_rel(ev[i].get_coord()));
				if (d > dmin)
					continue;
				dmin = d;
				imin = i;
			}
			if (imin >= ev.size()) {
				std::cerr << "No Airport found in vincinity of " << pt.get_lat_str2() << ' ' << pt.get_lon_str2() << std::endl;
				return;
			}
			run(ev[imin], !m_continue);
			return;
		}
	}
	AirportsDb::elementvector_t ev(m_airportdb.find_by_icao(id1, 0, AirportsDb::comp_exact, 0, AirportsDb::Airport::subtables_all));
	if (ev.empty()) {
		std::cerr << "Airport ident " << id1 << " not found" << std::endl;
		return;
	}
	if (ev.size() != 1) {
		std::cerr << "Airport ident " << id1 << " not unique" << std::endl;
		return;
	}
	run(ev[0], !m_continue);
}

namespace {
	class AirportAddressOrder : public AirportsDb::Airport::Address {
	public:
		typedef AirportsDb::Airport::Address Address;
		typedef AirportsDb::Airport Airport;
		AirportAddressOrder(const Address& addr, unsigned int order = 0) : Address(addr), m_order(order) {}
		AirportAddressOrder(uint64_t id = 0, Airport::table_t tbl = Airport::table_main, unsigned int order = 0) : Address(id, tbl), m_order(order) {}
		const Address& get_address(void) const { return (const Address&)(*this); }
		unsigned int get_order(void) const { return m_order; }
		bool operator<(const AirportAddressOrder& x) const {
			if (get_table() < x.get_table())
				return true;
			if (x.get_table() < get_table())
				return false;
			return get_id() < x.get_id();
		}

	protected:
		unsigned int m_order;
	};

	class AirportAddressOrderSorter {
	public:
		bool operator()(const AirportAddressOrder& a, const AirportAddressOrder& b) const { return a.get_order() < b.get_order(); }
	};
};

void CFMUSIDSTAR::run(const std::string& aspcid, const std::string& aspctype)
{
	std::string aspcid1(Glib::ustring(aspcid).uppercase());
	std::string aspctype1(Glib::ustring(aspctype).uppercase());
	std::cerr << "Processing " << aspcid1 << '/' << aspctype1 << std::endl;
	typedef std::vector<AirportAddressOrder> airports_t;
	airports_t airports;
	{
		AirspacesDb::elementvector_t ev(m_airspacedb.find_by_icao(aspcid1, 0, AirspacesDb::comp_exact, 0, AirspacesDb::Airspace::subtables_all));
		if (!aspctype1.empty()) {
			for (AirspacesDb::elementvector_t::iterator evi(ev.begin()), eve(ev.end()); evi != eve;) {
				if (evi->get_class_string() == aspctype1) {
					++evi;
					continue;
				}
				evi = ev.erase(evi);
				eve = ev.end();
			}
		}
		if (ev.empty()) {
			std::cerr << "Airspace(s) " << aspcid1 << '/' << aspctype1 << " not found" << std::endl;
			return;
		}
		unsigned int order(0);
		for (AirspacesDb::elementvector_t::const_iterator evi(ev.begin()), eve(ev.end()); evi != eve; ++evi) {
			std::cerr << "Processing " << evi->get_icao() << '/' << evi->get_class_string() << " (" << evi->get_sourceid() << ')' << std::endl;
			const MultiPolygonHole& poly(evi->get_polygon());
			Rect bbox(evi->get_bbox());
			const unsigned int startorder(order);
			AirportsDb::elementvector_t arpts(m_airportdb.find_by_rect(bbox, 0, AirportsDb::Airport::subtables_none));
			for (AirportsDb::elementvector_t::const_iterator ai(arpts.begin()), ae(arpts.end()); ai != ae; ++ai) {
				const AirportsDb::Airport& arpt(*ai);
				if (!bbox.is_inside(arpt.get_coord()) || !poly.windingnumber(arpt.get_coord()))
					continue;
				if ((arpt.get_flightrules() & (AirportsDb::Airport::flightrules_arr_vfr | AirportsDb::Airport::flightrules_arr_ifr)) &&
				    (arpt.get_flightrules() & (AirportsDb::Airport::flightrules_dep_vfr | AirportsDb::Airport::flightrules_dep_ifr)))
					continue;
				airports_t::iterator i(std::lower_bound(airports.begin(), airports.end(), AirportAddressOrder(arpt.get_address(), order)));
				if (i != airports.end() && i->get_address() == arpt.get_address())
					continue;
				airports.insert(i, AirportAddressOrder(arpt.get_address(), order));
				++order;
			}
			std::cerr << "Airspace " << evi->get_icao() << '/' << evi->get_class_string() << " (" << evi->get_sourceid()
				  << ") has " << (order - startorder) << " airports to process" << std::endl;
		}
		std::sort(airports.begin(), airports.end(), AirportAddressOrderSorter());
	}
	for (airports_t::const_iterator ai(airports.begin()), ae(airports.end()); ai != ae; ++ai) {
		AirportsDb::Airport arpt(m_airportdb(ai->get_address(), AirportsDb::Airport::subtables_all));
		if (!arpt.is_valid())
			continue;
		if ((arpt.get_flightrules() & (AirportsDb::Airport::flightrules_arr_vfr | AirportsDb::Airport::flightrules_arr_ifr)) &&
		    (arpt.get_flightrules() & (AirportsDb::Airport::flightrules_dep_vfr | AirportsDb::Airport::flightrules_dep_ifr)))
			continue;
		//std::cerr << "Processing " << (std::string)arpt.get_icao_name() << std::endl;
		run(arpt, false);
	}
}

void CFMUSIDSTAR::run(AirportsDb::Airport& arpt, bool force)
{
	if (!arpt.is_valid())
		return;
	if (!m_ecac.windingnumber(arpt.get_coord())) {
		std::cerr << "Skipping " << (std::string)arpt.get_icao_name() << ' ' << arpt.get_coord().get_lat_str2()
			  << ' ' << arpt.get_coord().get_lon_str2() << " - not in ECAC" << std::endl;
		return;
	}
	// find opposite airport
	if (m_oppositearpt.empty()) {
		m_oppositearpt.clear();
		m_oppositewpt.clear();
		m_oppositearptcoord.set_invalid();
		m_oppositewptcoord.set_invalid();
		typedef std::multimap<double,AirportsDb::Airport::Address> arptlist_t;
		arptlist_t arptlist;
		{
			AirportsDb::elementvector_t ev(m_airportdb.find_by_rect(arpt.get_coord().simple_box_nmi(1000), 0, AirportsDb::Airport::subtables_none));
			for (AirportsDb::elementvector_t::const_iterator evi(ev.begin()), eve(ev.end()); evi != eve; ++evi) {
				if (evi->is_fpl_zzzz())
					continue;
				if (evi->get_typecode() != 'A' && evi->get_typecode() != 'B')
					continue;
				arptlist.insert(std::make_pair(fabs(arpt.get_coord().spheric_distance_nmi(evi->get_coord()) - 100), evi->get_address()));
			}
		}
		for (arptlist_t::const_iterator ali(arptlist.begin()), ale(arptlist.end()); ali != ale; ++ali) {
			AirportsDb::Airport el(m_airportdb(ali->second.get_id(), ali->second.get_table(), AirportsDb::Airport::subtables_runways));
			if (!el.is_valid()) {
				std::cerr << "Error: airport " << ali->second << " not found" << std::endl;
				continue;
			}
			if (true)
				std::cerr << "Trying airport " << (std::string)el.get_icao_name() << std::endl;
			{
				bool ok(false);
				for (unsigned int i = 0, n = el.get_nrrwy(); i < n; ++i) {
					const AirportsDb::Airport::Runway& rwy(el.get_rwy(i));
					if (!(rwy.is_le_usable() || rwy.is_he_usable()))
						continue;
					if (rwy.get_length() < (unsigned int)(Point::m_to_ft * 1000))
						continue;
					if (!rwy.is_concrete())
						continue;
					ok = true;
					break;
				}
				if (!ok)
					continue;
			}
			m_oppositearpt = el.get_icao();
			m_oppositearptcoord = el.get_coord();
			// find opposite waypoint
			WaypointsDb::elementvector_t ev(m_waypointdb.find_by_rect(el.get_coord().simple_box_nmi(50), 0, WaypointsDb::Waypoint::subtables_none));
			double dist(std::numeric_limits<double>::max());
			for (WaypointsDb::elementvector_t::const_iterator evi(ev.begin()), eve(ev.end()); evi != eve; ++evi) {
				if (evi->get_type() != WaypointsDb::Waypoint::type_icao)
					continue;
				double d(evi->get_coord().spheric_distance_nmi(el.get_coord()));
				if (d > dist)
					continue;
				dist = d;
				m_oppositewpt = evi->get_name();
				m_oppositewptcoord = evi->get_coord();
			}
			if (m_oppositewpt.empty())
				throw std::runtime_error("Cannot find opposite waypoint");
			break;
		}
	}
	if (true) {
		std::cout << "Processing airport " << (std::string)arpt.get_icao_name() << ' ' << arpt.get_coord().get_lat_str2()
			  << ' ' << arpt.get_coord().get_lon_str2() << std::endl;
		for (unsigned int i = 0, n = arpt.get_nrvfrrte(); i < n; ++i) {
			const AirportsDb::Airport::VFRRoute& rte(arpt.get_vfrrte(i));
			if (rte.size()) {
				const AirportsDb::Airport::VFRRoute::VFRRoutePoint& firstpt(rte[0]);
				const AirportsDb::Airport::VFRRoute::VFRRoutePoint& lastpt(rte[rte.size() - 1U]);
				if (firstpt.get_pathcode() == AirportsDb::Airport::VFRRoute::VFRRoutePoint::pathcode_star)
					std::cout << "STAR: ";
				if (lastpt.get_pathcode() == AirportsDb::Airport::VFRRoute::VFRRoutePoint::pathcode_sid)
					std::cout << "SID: ";
			}
			std::cout << (std::string)rte.get_name() << ' ' << rte.get_minalt() << "..." << rte.get_maxalt() << " (";
			for (unsigned int j = 0, m = rte.size(); j < m; ++j) {
				const AirportsDb::Airport::VFRRoute::VFRRoutePoint& pt(rte[j]);
				std::cout << ' ' << (std::string)pt.get_name();
			}
			std::cout << " )" << std::endl;
		}
	}
	typedef std::set<RoutePoint> connpoints_t;
	connpoints_t connpoints;
	// find radius
	double radius(m_sidstarradius);
	for (unsigned int i = 0, n = arpt.get_nrvfrrte(); i < n;) {
		const AirportsDb::Airport::VFRRoute& rte(arpt.get_vfrrte(i));
		if (!rte.size()) {
			++i;
			continue;
		}
		const AirportsDb::Airport::VFRRoute::VFRRoutePoint& firstpt(rte[0]);
		const AirportsDb::Airport::VFRRoute::VFRRoutePoint& lastpt(rte[rte.size() - 1U]);
		if (firstpt.get_pathcode() == AirportsDb::Airport::VFRRoute::VFRRoutePoint::pathcode_star) {
			double d(firstpt.get_coord().spheric_distance_nmi_dbl(arpt.get_coord()));
			if (d > 200) {
				std::cerr << "Erasing STAR " << (std::string)rte.get_name() << " distance " << Conversions::dist_str(d)
					  << " point " << (std::string)firstpt.get_name() << ' '
					  << firstpt.get_coord().get_lat_str2() << ' ' << firstpt.get_coord().get_lon_str2() << std::endl;
				arpt.erase_vfrrte(i);
				--n;
				continue;
			}
			radius = std::max(radius, 1 + d);
		}
		if (lastpt.get_pathcode() == AirportsDb::Airport::VFRRoute::VFRRoutePoint::pathcode_sid) {
			double d(lastpt.get_coord().spheric_distance_nmi_dbl(arpt.get_coord()));
			if (d > 200) {
				std::cerr << "Erasing SID " << (std::string)rte.get_name() << " distance " << Conversions::dist_str(d)
					  << " point " << (std::string)lastpt.get_name() << ' '
					  << lastpt.get_coord().get_lat_str2() << ' ' << lastpt.get_coord().get_lon_str2() << std::endl;
				arpt.erase_vfrrte(i);
				--n;
				continue;
			}
			radius = std::max(radius, 1 + d);
		}
		++i;
	}
	// find possible connection points
	{
		NavaidsDb::elementvector_t ev(m_navaiddb.find_by_rect(arpt.get_coord().simple_box_nmi(radius + 10), 0, NavaidsDb::Navaid::subtables_none));
		for (NavaidsDb::elementvector_t::const_iterator evi(ev.begin()), eve(ev.end()); evi != eve; ++evi) {
			if (!evi->is_valid() || evi->get_icao().size() < 2 || Engine::AirwayGraphResult::Vertex::is_ident_numeric(evi->get_icao()))
				continue;
			switch (evi->get_navaid_type()) {
			case NavaidsDb::Navaid::navaid_vor:
			case NavaidsDb::Navaid::navaid_vordme:
			case NavaidsDb::Navaid::navaid_ndb:
			case NavaidsDb::Navaid::navaid_ndbdme:
			case NavaidsDb::Navaid::navaid_vortac:
			case NavaidsDb::Navaid::navaid_dme:
				break;

			default:
				continue;
			}
			if (arpt.get_coord().spheric_distance_nmi(evi->get_coord()) > radius)
				continue;
			connpoints.insert(RoutePoint(*evi));
 		}
	}
	{
		WaypointsDb::elementvector_t ev(m_waypointdb.find_by_rect(arpt.get_coord().simple_box_nmi(radius + 10), 0, WaypointsDb::Waypoint::subtables_none));
		for (WaypointsDb::elementvector_t::const_iterator evi(ev.begin()), eve(ev.end()); evi != eve; ++evi) {
			if (!evi->is_valid() || evi->get_name().size() < 2 || Engine::AirwayGraphResult::Vertex::is_ident_numeric(evi->get_name()))
				continue;
			switch (evi->get_type()) {
			case WaypointsDb::Waypoint::type_icao:
				break;

			case WaypointsDb::Waypoint::type_other:
				if (evi->get_icao().size() != 5)
					continue;
				break;

			default:
				continue;
			}
			if (arpt.get_coord().spheric_distance_nmi(evi->get_coord()) > radius)
				continue;
			connpoints.insert(RoutePoint(*evi));
 		}
	}
	std::cout << "Airport " << (std::string)arpt.get_icao_name() << ' ' << arpt.get_coord().get_lat_str2()
		  << ' ' << arpt.get_coord().get_lon_str2() << ": radius " << Glib::ustring::format(std::fixed, std::setprecision(1), radius)
		  << " opposite " << m_oppositearpt << ", " << m_oppositewpt << " connpoints " << connpoints.size() << std::endl;
	if (force || !(arpt.get_flightrules() & (AirportsDb::Airport::flightrules_dep_vfr | AirportsDb::Airport::flightrules_dep_ifr))) {
		arpt.set_flightrules(arpt.get_flightrules() | (AirportsDb::Airport::flightrules_dep_vfr | AirportsDb::Airport::flightrules_dep_ifr));
		for (connpoints_t::const_iterator cpi(connpoints.begin()), cpe(connpoints.end()); cpi != cpe; ++cpi) {
			try_point(arpt, *cpi, true);
			if (!(arpt.get_flightrules() & AirportsDb::Airport::flightrules_dep_ifr))
				break;
		}
		if (true) {
			std::cout << "Saving airport " << (std::string)arpt.get_icao_name() << ' ' << arpt.get_coord().get_lat_str2()
				  << ' ' << arpt.get_coord().get_lon_str2() << std::endl;
			for (unsigned int i = 0, n = arpt.get_nrvfrrte(); i < n; ++i) {
				const AirportsDb::Airport::VFRRoute& rte(arpt.get_vfrrte(i));
				if (rte.size()) {
					const AirportsDb::Airport::VFRRoute::VFRRoutePoint& firstpt(rte[0]);
					const AirportsDb::Airport::VFRRoute::VFRRoutePoint& lastpt(rte[rte.size() - 1U]);
					if (firstpt.get_pathcode() == AirportsDb::Airport::VFRRoute::VFRRoutePoint::pathcode_star)
						std::cout << "STAR: ";
					if (lastpt.get_pathcode() == AirportsDb::Airport::VFRRoute::VFRRoutePoint::pathcode_sid)
						std::cout << "SID: ";
				}
				std::cout << (std::string)rte.get_name() << ' ' << rte.get_minalt() << "..." << rte.get_maxalt() << " (";
				for (unsigned int j = 0, m = rte.size(); j < m; ++j) {
					const AirportsDb::Airport::VFRRoute::VFRRoutePoint& pt(rte[j]);
					std::cout << ' ' << (std::string)pt.get_name();
				}
				std::cout << " )" << std::endl;
			}
		}
		arpt.set_modtime(time(0));
		m_airportdb.save(arpt);
	}
	if (force || !(arpt.get_flightrules() & (AirportsDb::Airport::flightrules_arr_vfr | AirportsDb::Airport::flightrules_arr_ifr))) {
		arpt.set_flightrules(arpt.get_flightrules() | (AirportsDb::Airport::flightrules_arr_vfr | AirportsDb::Airport::flightrules_arr_ifr));
		for (connpoints_t::const_iterator cpi(connpoints.begin()), cpe(connpoints.end()); cpi != cpe; ++cpi) {
			try_point(arpt, *cpi, false);
			if (!(arpt.get_flightrules() & AirportsDb::Airport::flightrules_arr_ifr))
				break;
		}
		if (true) {
			std::cout << "Saving airport " << (std::string)arpt.get_icao_name() << ' ' << arpt.get_coord().get_lat_str2()
				  << ' ' << arpt.get_coord().get_lon_str2() << std::endl;
			for (unsigned int i = 0, n = arpt.get_nrvfrrte(); i < n; ++i) {
				const AirportsDb::Airport::VFRRoute& rte(arpt.get_vfrrte(i));
				if (rte.size()) {
					const AirportsDb::Airport::VFRRoute::VFRRoutePoint& firstpt(rte[0]);
					const AirportsDb::Airport::VFRRoute::VFRRoutePoint& lastpt(rte[rte.size() - 1U]);
					if (firstpt.get_pathcode() == AirportsDb::Airport::VFRRoute::VFRRoutePoint::pathcode_star)
						std::cout << "STAR: ";
					if (lastpt.get_pathcode() == AirportsDb::Airport::VFRRoute::VFRRoutePoint::pathcode_sid)
						std::cout << "SID: ";
				}
				std::cout << (std::string)rte.get_name() << ' ' << rte.get_minalt() << "..." << rte.get_maxalt() << " (";
				for (unsigned int j = 0, m = rte.size(); j < m; ++j) {
					const AirportsDb::Airport::VFRRoute::VFRRoutePoint& pt(rte[j]);
					std::cout << ' ' << (std::string)pt.get_name();
				}
				std::cout << " )" << std::endl;
			}
		}
		arpt.set_modtime(time(0));
		m_airportdb.save(arpt);
	}
}

void CFMUSIDSTAR::try_point(AirportsDb::Airport& arpt, const RoutePoint& rpt, bool dep)
{
	typedef enum {
		mode_sample,
		mode_findlow,
		mode_findhigh,
	} mode_t;
	mode_t mode(mode_sample);
	int32_t alt(5000), altl0(0), altl1(0), alth0(0), alth1(25000), minalt(0), maxalt(0);
	std::string callsign;
	for (unsigned int i = 0; i < 5; ++i) {
		char ch('A' + (rand() % ('Z' - 'A' + 1)));
		callsign.push_back(ch);
	}
	std::string deptime;
	{
		time_t t(time(0));
		struct tm *tm(gmtime(&t));
		deptime = Glib::ustring::format(std::setfill(L'0'), std::setw(2), (tm->tm_hour + 2U) % 24U) +
			Glib::ustring::format(std::setfill(L'0'), std::setw(2), tm->tm_min);
	}
	for (;;) {
		std::ostringstream fplan;
		{
			double dist(arpt.get_coord().spheric_distance_nmi_dbl(rpt.get_pt()) + 
				    rpt.get_pt().spheric_distance_nmi_dbl(m_oppositewptcoord) + 
				    m_oppositewptcoord.spheric_distance_nmi_dbl(m_oppositearptcoord));
			unsigned int rtetime(Point::round<unsigned int,double>(dist * (60.0 / 135)));
			std::string opparpt1(m_oppositearpt);
			std::string opparpt2;
			if (true || AirportsDb::Airport::is_fpl_zzzz(m_oppositearpt)) {
				opparpt1 = "ZZZZ";
				if (dep)
					opparpt2 = " DEST";
				else
					opparpt2 = " DEP";
				opparpt2 += "/" + m_oppositearptcoord.get_fpl_str();
			}
			std::string arpt1(arpt.get_icao());
			std::string arpt2;
			if (arpt.is_fpl_zzzz()) {
				arpt1 = "ZZZZ";
				if (dep)
					arpt2 = " DEP";
				else
					arpt2 = " DEST";
				arpt2 += "/" + arpt.get_coord().get_fpl_str();
			}
			if (dep)
				fplan << "-(FPL-" << callsign << "-YG -1M20T/L -SDFGLORY/S -" << arpt1 << deptime
				      << " -N0135F" << Glib::ustring::format(std::setfill(L'0'), std::setw(3), (alt + 50) / 100) << ' ' << rpt.get_name()
				      << " VFR " << m_oppositewpt << " -" << opparpt1
				      << Glib::ustring::format(std::setfill(L'0'), std::setw(4), 100U * (rtetime / 60U) + (rtetime % 60U))
				      << " -PBN/B2D2" << arpt2 << opparpt2;
			else
				fplan << "-(FPL-" << callsign << "-ZG -1M20T/L -SDFGLORY/S -" << opparpt1 << deptime
				      << " -N0135VFR " << m_oppositewpt << ' ' << rpt.get_name()
				      << "/N0135F" << Glib::ustring::format(std::setfill(L'0'), std::setw(3), (alt + 50) / 100)
				      << " IFR -" << arpt1
				      << Glib::ustring::format(std::setfill(L'0'), std::setw(4), 100U * (rtetime / 60U) + (rtetime % 60U))
				      << " -PBN/B2D2" << arpt2 << opparpt2;
		}
		fplan << ')';
		std::cerr << ">> " << fplan.str() << std::endl;
		validationresponse_t resp(validate(fplan.str()));
		typedef enum {
			res_nok,
			res_ok,
			res_undecided
		} res_t;
		res_t res(res_undecided);
		for (validationresponse_t::const_iterator ri(resp.begin()), re(resp.end()); ri != re; ++ri) {
			std::cerr << "<< " << *ri << std::endl;
			{
				Glib::RefPtr<Glib::Regex> rx(Glib::Regex::create("^FAIL: CFMU Webpage"));
				Glib::MatchInfo minfo;
				if (rx->match(*ri, minfo)) {
					throw std::runtime_error("Validation Error");
				}
			}
			{
				Glib::RefPtr<Glib::Regex> rx1(Glib::Regex::create("^PROF193: IFR OPERATIONS AT AERODROME ([[:alnum:]]+) ARE NOT PERMITTED"));
				Glib::RefPtr<Glib::Regex> rx2(Glib::Regex::create("^EFPM228: INVALID VALUE \\(ADE(S|P)\\)"));
				Glib::MatchInfo minfo1, minfo2;
				bool clsd(rx2->match(*ri, minfo2));
				if (clsd && arpt.get_name().compare(0, 3, "[X]"))
					arpt.set_name("[X] " + arpt.get_name());
				if (clsd || rx1->match(*ri, minfo1)) {
					if (dep)
						clear_sid(arpt);
					else
						clear_star(arpt);
					std::cout << "No " << (dep ? "departure" : "arrival") << " IFR operations" << std::endl;
					return;
				}
			}
			{
				Glib::RefPtr<Glib::Regex> rx1(Glib::Regex::create("^ROUTE135: THE SID LIMIT IS EXCEEDED FOR AERODROME .*? CONNECTING TO ([[:alnum:]]+)."));
				Glib::RefPtr<Glib::Regex> rx2(Glib::Regex::create("^ROUTE134: THE STAR LIMIT IS EXCEEDED FOR AERODROME .*? CONNECTING TO ([[:alnum:]]+)."));
				Glib::RefPtr<Glib::Regex> rx3(Glib::Regex::create("^ROUTE168: INVALID DCT"));
				Glib::RefPtr<Glib::Regex> rx4(Glib::Regex::create("^PROF191: TTL EET DIFFERENCE"));
				Glib::RefPtr<Glib::Regex> rx5(Glib::Regex::create("^PROF204: RS: TRAFFIC VIA ([[:alnum:]]+):.*? IS ON FORBIDDEN ROUTE"));
				Glib::MatchInfo minfo1, minfo2, minfo3, minfo4, minfo5;
				if ((rx1->match(*ri, minfo1) && (dep || minfo1.fetch(1).uppercase() == arpt.get_icao() || arpt.is_fpl_zzzz())) ||
				    (rx2->match(*ri, minfo2) && (!dep || minfo2.fetch(1).uppercase() == arpt.get_icao() || arpt.is_fpl_zzzz())) ||
				    rx3->match(*ri, minfo3) || rx4->match(*ri, minfo4) ||
				    (rx5->match(*ri, minfo5) && minfo5.fetch(1).uppercase() == rpt.get_name())) {
					res = res_nok;
					continue;
				}
			}
			{
				Glib::RefPtr<Glib::Regex> rx(Glib::Regex::create("^ROUTE130: UNKNOWN DESIGNATOR ([[:alnum:]]+)"));
				Glib::MatchInfo minfo;
				if (rx->match(*ri, minfo)) {
					if (dep)
						clear_sid(arpt, rpt);
					else
						clear_star(arpt, rpt);
					std::cout << "No " << (dep ? "SID to" : "STAR from") << ' ' << rpt.get_name() << " (unknown designator)" << std::endl;
					return;
				}
			}
			{
				Glib::RefPtr<Glib::Regex> rx(Glib::Regex::create("^NO ERRORS"));
				Glib::MatchInfo minfo;
				if (rx->match(*ri, minfo)) {
					res = res_ok;
					continue;
				}
			}
			std::cerr << "Message not understood: \"" << *ri << "\"" << std::endl;
		}
		std::cerr << "Result: " << (res == res_ok ? "OK" : (res == res_nok ? "FAIL" : "UNKNOWN")) << std::endl;
		if (res == res_undecided) {
			std::cerr << "Warning: unknown result" << std::endl;
			res = res_ok;
		}
		switch (mode) {
		case mode_sample:
			if (res == res_ok) {
				altl1 = alth0 = minalt = maxalt = alt;
				alt = (altl0 + altl1) / 2;
				alt = ((alt + 500) / 1000) * 1000;
				mode = mode_findlow;
				break;
			} else {
				altl0 = alt;
			}
			alt += 5000;
			if (alt > alth1) {
				if (dep)
					clear_sid(arpt, rpt);
				else
					clear_star(arpt, rpt);
				std::cout << "No " << (dep ? "SID to" : "STAR from") << ' ' << rpt.get_name() << std::endl;
				return;
			}
			break;

		case mode_findlow:
			if (res == res_ok)
				altl1 = minalt = alt;
			else
				altl0 = alt;
			if (altl1 - altl0 < 2000) {
				alt = (alth0 + alth1) / 2;
				alt = ((alt + 500) / 1000) * 1000;
				mode = mode_findhigh;
				break;
			}
			alt = (altl0 + altl1) / 2;
			alt = ((alt + 500) / 1000) * 1000;
			break;

		case mode_findhigh:
			if (res == res_ok)
				alth0 = maxalt = alt;
			else
				alth1 = alt;
			if (alth1 - alth0 < 2000) {
				if (dep)
					add_sid(arpt, rpt, minalt, maxalt);
				else
					add_star(arpt, rpt, minalt, maxalt);
				std::cout << (dep ? "SID to" : "STAR from") << ' ' << rpt.get_name() << " F"
					  << Glib::ustring::format(std::setfill(L'0'), std::setw(3), (minalt + 50) / 100) << "...F"
					  << Glib::ustring::format(std::setfill(L'0'), std::setw(3), (maxalt + 50) / 100) << std::endl;
				return;
			}
			alt = (alth0 + alth1) / 2;
			alt = ((alt + 500) / 1000) * 1000;
			break;
		}
	}
}

int main(int argc, char *argv[])
{
        try {
                Glib::init();
                Glib::set_application_name("CFMU SID/STAR");
                Glib::thread_init();
		Gio::init();
		srand(time(0));
		CFMUSIDSTAR app(argc, argv);
		app.init();
		if (argc < 2) {
			app.run();
		} else {
			for (int i = 1; i < argc; ++i) {
				std::string p(argv[i]);
				std::string::size_type n(p.find('/'));
				if (n == std::string::npos) {
					app.run(p);
					continue;
				}
				std::string p1(p, n + 1);
				p.erase(n);
				app.run(p, p1);
			}
		}
                return EX_OK;
        } catch(const std::exception& ex) {
                std::cerr << "Error: " << ex.what() << std::endl;
                return EX_DATAERR;
        } catch(const Glib::FileError& ex) {
                std::cerr << "FileError: " << ex.what() << std::endl;
                return EX_DATAERR;
        }
        return EX_DATAERR;
}
