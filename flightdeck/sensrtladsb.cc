//
// C++ Implementation: Realtek ADS-B Receiver
//
// Description: Realtek ADS-B Receiver
//
//
// Author: Thomas Sailer <t.sailer@alumni.ethz.ch>, (C) 2013
//
// Copyright: See COPYING file that comes with this distribution
//
//

#include <glibmm/datetime.h>

#include "sensrtladsb.h"

Sensors::SensorRTLADSB::SensorRTLADSB(Sensors& sensors, const Glib::ustring& configgroup)
	: SensorADSB(sensors, configgroup), m_rxbinary("/usr/bin/rtl_adsb"), m_rxargs(""), m_childrun(false)
{
	// get configuration
        const Glib::KeyFile& cf(get_sensors().get_configfile());
        if (!cf.has_key(get_configgroup(), "rxbinary"))
		get_sensors().get_configfile().set_string(get_configgroup(), "rxbinary", m_rxbinary);
        m_rxbinary = cf.get_string(get_configgroup(), "rxbinary");
	if (!cf.has_key(get_configgroup(), "rxargs"))
		get_sensors().get_configfile().set_string(get_configgroup(), "rxargs", m_rxargs);
        m_rxargs = cf.get_string(get_configgroup(), "rxargs");
	try_connect();
}

Sensors::SensorRTLADSB::~SensorRTLADSB()
{
	close();
}

bool Sensors::SensorRTLADSB::close(void)
{
	clear();
	m_connchildstdout.disconnect();
	m_connchildwatch.disconnect();
	if (m_childchanstdout) {
		m_childchanstdout->close();
		m_childchanstdout.reset();
	}
	if (!m_childrun)
		return false;
	m_childrun = false;
	Glib::signal_child_watch().connect(sigc::hide(sigc::ptr_fun(&Glib::spawn_close_pid)), m_childpid);
	return true;
}

bool Sensors::SensorRTLADSB::try_connect(void)
{
	if (m_childrun)
		return false;
	m_connchildwatch.disconnect();
	if (m_rxbinary.empty())
		return false;
	int childstdout(-1);
	try {
		std::vector<std::string> argv, env;
#ifdef __MINGW32__
		std::string workdir;
		{
			gchar *instdir(g_win32_get_package_installation_directory_of_module(0));
			if (instdir) {
				workdir = instdir;
			} else {
				workdir = PACKAGE_PREFIX_DIR;
			}
		}
#else
		std::string workdir(PACKAGE_DATA_DIR);
		env.push_back("PATH=/bin:/usr/bin");
		{
			bool found(false);
			std::string disp(Glib::getenv("DISPLAY", found));
			if (found)
				env.push_back("DISPLAY=" + disp);
		}
#endif
		argv.push_back(m_rxbinary);
		// split up args
		{
			enum {
				pm_normal,
				pm_newarg,
				pm_single,
				pm_double
			} pm(pm_newarg);
			for (std::string::const_iterator i(m_rxargs.begin()), e(m_rxargs.end()); i != e; ++i) {
				if (pm == pm_single) {
					argv.back().push_back(*i);
					if (*i == '\'')
						pm = pm_normal;
					continue;
				}
				if (pm == pm_double) {
					argv.back().push_back(*i);
					if (*i == '"')
						pm = pm_normal;
					continue;
				}
				if (std::isspace(*i)) {
					pm = pm_newarg;
					continue;
				}
				if (pm == pm_newarg)
					argv.push_back("");
				pm = pm_normal;
				argv.back().push_back(*i);
				if (*i == '\'')
					pm = pm_single;
				else if (*i == '"')
					pm = pm_double;
			}
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
					     sigc::slot<void>(), &m_childpid, 0, &childstdout, 0);
#else
		Glib::spawn_async_with_pipes(workdir, argv, env,
					     Glib::SPAWN_DO_NOT_REAP_CHILD | Glib::SPAWN_STDERR_TO_DEV_NULL,
					     sigc::slot<void>(), &m_childpid, 0, &childstdout, 0);
#endif
		m_childrun = true;
	} catch (Glib::SpawnError& e) {
		m_childrun = false;
	        get_sensors().log(Sensors::loglevel_warning, "RTL ADSB: cannot run receiver binary: " + e.what());
		m_connchildwatch = Glib::signal_timeout().connect_seconds(sigc::mem_fun(*this, &SensorRTLADSB::try_connect), 15);
		return true;
	}
	m_connchildwatch = Glib::signal_child_watch().connect(sigc::mem_fun(this, &SensorRTLADSB::child_watch), m_childpid);
	m_childchanstdout = Glib::IOChannel::create_from_fd(childstdout);
	m_childchanstdout->set_encoding();
	m_childchanstdout->set_close_on_unref(true);
	m_connchildstdout = Glib::signal_io().connect(sigc::mem_fun(this, &SensorRTLADSB::child_stdout_handler), m_childchanstdout,
						      Glib::IO_IN | Glib::IO_ERR | Glib::IO_HUP);
	return false;
}

void Sensors::SensorRTLADSB::child_watch(GPid pid, int child_status)
{
	if (!m_childrun)
		return;
	if (m_childpid != pid) {
		Glib::spawn_close_pid(pid);
		return;
	}
	Glib::spawn_close_pid(pid);
	m_childrun = false;
	close();
	m_connchildwatch = Glib::signal_timeout().connect_seconds(sigc::mem_fun(*this, &SensorRTLADSB::try_connect), 15);	
}

bool Sensors::SensorRTLADSB::child_stdout_handler(Glib::IOCondition iocond)
{
        Glib::ustring line;
        Glib::IOStatus iostat(m_childchanstdout->read_line(line));
        if (iostat == Glib::IO_STATUS_ERROR ||
            iostat == Glib::IO_STATUS_EOF) {
		if (iostat == Glib::IO_STATUS_ERROR)
			get_sensors().log(Sensors::loglevel_warning, "RTL ADSB: Child stdout error, stopping...");
		else
		        get_sensors().log(Sensors::loglevel_warning, "RTL ADSB: Child stdout eof, stopping...");
                close();
		m_connchildwatch = Glib::signal_timeout().connect_seconds(sigc::mem_fun(*this, &SensorRTLADSB::try_connect), 15);	
		return false;
        }
	if (false)
		std::cerr << "RTLADSB: " << line << std::endl;
	ModeSMessage msg;
	{
		std::string line1(line);
		if (msg.parse_line(line1.begin(), line1.end()) != line1.begin())
			receive(msg);
	}
        return true;
}

void Sensors::SensorRTLADSB::get_param_desc(unsigned int pagenr, paramdesc_t& pd)
{
	SensorADSB::get_param_desc(pagenr, pd);
        {
                paramdesc_t::iterator pdi(pd.begin()), pde(pd.end());
                if (pdi != pde)
                        ++pdi;
                for (; pdi != pde; ++pdi)
                        if (pdi->get_type() == ParamDesc::type_section)
                                break;
		pdi = pd.insert(pdi, ParamDesc(ParamDesc::type_string, ParamDesc::editable_admin, parnrrxbinary, "Binary", "Receiver Binary", ""));
		pdi = pd.insert(pdi, ParamDesc(ParamDesc::type_string, ParamDesc::editable_admin, parnrrxargs, "Args", "Receiver Binary Arguments", ""));
        }
}

Sensors::SensorRTLADSB::paramfail_t Sensors::SensorRTLADSB::get_param(unsigned int nr, Glib::ustring& v) const
{
	switch (nr) {
	default:
		break;

	case parnrrxbinary:
		v = m_rxbinary;
		return paramfail_ok;

	case parnrrxargs:
		v = m_rxargs;
		return paramfail_ok;
	}
	return SensorADSB::get_param(nr, v);
}

void Sensors::SensorRTLADSB::set_param(unsigned int nr, const Glib::ustring& v)
{
	switch (nr) {
	default:
		SensorADSB::set_param(nr, v);
		return;

	case parnrrxbinary:
		if (m_rxbinary == v)
			return;
		m_rxbinary = v;
		get_sensors().get_configfile().set_string(get_configgroup(), "rxbinary", m_rxbinary);
		close();
		try_connect();
		break;

	case parnrrxargs:
		if (m_rxargs == v)
			return;
		m_rxargs = v;
		get_sensors().get_configfile().set_string(get_configgroup(), "rxargs", m_rxargs);
		close();
		try_connect();
		break;
	}
	ParamChanged pc;
	pc.set_changed(nr);
	update(pc);
}
