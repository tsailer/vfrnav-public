//
// C++ Implementation: King GPS Sensor
//
// Description: GPS Sensor Handling
//
//
// Author: Thomas Sailer <t.sailer@alumni.ethz.ch>, (C) 2011, 2012
//
// Copyright: See COPYING file that comes with this distribution
//
//

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <errno.h>
#include <limits>
#include <cstring>
#include <cstdio>
#include <glibmm/datetime.h>
#ifdef HAVE_TERMIOS_H
#include <termios.h>
#endif
#include "sensgpskingtty.h"

const struct Sensors::SensorKingGPSTTY::baudrates Sensors::SensorKingGPSTTY::baudrates[8] = {
#if defined(__MINGW32__)
        { 1200, CBR_1200 },
        { 2400, CBR_2400 },
        { 4800, CBR_4800 },
        { 9600, CBR_9600 },
        { 19200, CBR_19200 },
        { 38400, CBR_38400 },
        { 57600, CBR_57600 },
        { 115200, CBR_115200 }
#elif defined(HAVE_TERMIOS_H)
	{ 1200, B1200 },
	{ 2400, B2400 },
	{ 4800, B4800 },
	{ 9600, B9600 },
	{ 19200, B19200 },
	{ 38400, B38400 },
	{ 57600, B57600 },
	{ 115200, B115200 }
#endif
};

Sensors::SensorKingGPSTTY::SensorKingGPSTTY(Sensors& sensors, const Glib::ustring& configgroup)
	: SensorKingGPS(sensors, configgroup),
#ifdef __MINGW32__
	  m_fd(INVALID_HANDLE_VALUE),
#else
	  m_fd(-1),
#endif
	  m_baudrate(5)
{
        const Glib::KeyFile& cf(get_sensors().get_configfile());
        if (!cf.has_key(get_configgroup(), "device"))
		get_sensors().get_configfile().set_string(get_configgroup(), "device", "/dev/ttyS0");
	m_device = cf.get_string(get_configgroup(), "device");
        if (!cf.has_key(get_configgroup(), "baudrate"))
		get_sensors().get_configfile().set_integer(get_configgroup(), "baudrate", baudrates[m_baudrate].baud);
	{
		unsigned int br = cf.get_integer(get_configgroup(), "baudrate");
		unsigned int i = 0;
		unsigned int e = sizeof(baudrates)/sizeof(baudrates[0]) - 1;
		while (e - i > 1) {
			unsigned int m = (i + e) / 2;
			if (br < baudrates[m].baud)
				e = m;
			else
				i = m;
		}
		m_baudrate = (br < (baudrates[i].baud + baudrates[e].baud) / 2) ? i : e;
		if (br != baudrates[m_baudrate].baud)
			get_sensors().get_configfile().set_integer(get_configgroup(), "baudrate", baudrates[m_baudrate].baud);
	}
	try_connect();
}

Sensors::SensorKingGPSTTY::~SensorKingGPSTTY()
{
	close();
}

bool Sensors::SensorKingGPSTTY::close(void)
{
	m_conn.disconnect();
	m_conntimeout.disconnect();
#ifdef __MINGW32__
        bool was_open(m_fd != INVALID_HANDLE_VALUE);
        if (was_open)
                CloseHandle(m_fd);
        m_fd = INVALID_HANDLE_VALUE;
#else
	bool was_open(m_fd != -1);
	if (was_open)
		::close(m_fd);
	m_fd = -1;
#endif
	return was_open;
}

bool Sensors::SensorKingGPSTTY::try_connect(void)
{
	if (close())
		invalidate_update_gps();
#ifdef __MINGW32__
	m_fd = CreateFile(m_device.c_str(), GENERIC_READ, 0, 0, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, 0);
	if (m_fd == INVALID_HANDLE_VALUE) {
                std::ostringstream oss;
		gchar *errmsg(g_win32_error_message(GetLastError()));
                oss << "nmea gps: cannot open " << m_device << ':' << errmsg;
		g_free(errmsg);
                get_sensors().log(Sensors::loglevel_warning, oss.str());
                m_conn = Glib::signal_timeout().connect_seconds(sigc::mem_fun(*this, &SensorKingGPSTTY::try_connect), 15);
                return true;
        }
	DCB serpar = { 0 };
	serpar.DCBlength = sizeof(serpar);
	if (!GetCommState(m_fd, &serpar)) {
                std::ostringstream oss;
		gchar *errmsg(g_win32_error_message(GetLastError()));
                oss << "nmea gps: cannot get parameters of device " << m_device << ':' << errmsg;
		g_free(errmsg);
                get_sensors().log(Sensors::loglevel_warning, oss.str());
                m_conn = Glib::signal_timeout().connect_seconds(sigc::mem_fun(*this, &SensorKingGPSTTY::try_connect), 15);
                return true;
	}
	serpar.BaudRate = baudrates[m_baudrate].tbaud;
	serpar.ByteSize = 8;
	serpar.StopBits = ONESTOPBIT;
	serpar.Parity = NOPARITY;
	if (!SetCommState(m_fd, &serpar)){
                std::ostringstream oss;
		gchar *errmsg(g_win32_error_message(GetLastError()));
                oss << "nmea gps: cannot set parameters of device " << m_device << ':' << errmsg;
		g_free(errmsg);
                get_sensors().log(Sensors::loglevel_warning, oss.str());
                m_conn = Glib::signal_timeout().connect_seconds(sigc::mem_fun(*this, &SensorKingGPSTTY::try_connect), 15);
                return true;
	}
	COMMTIMEOUTS timeouts = { 0 };
	timeouts.ReadIntervalTimeout = MAXDWORD;
	timeouts.ReadTotalTimeoutConstant = 0;
	timeouts.ReadTotalTimeoutMultiplier = 0;
	timeouts.WriteTotalTimeoutConstant = 50;
	timeouts.WriteTotalTimeoutMultiplier = 10;
	if (!SetCommTimeouts(m_fd, &timeouts)){
                std::ostringstream oss;
		gchar *errmsg(g_win32_error_message(GetLastError()));
                oss << "nmea gps: cannot set comm timeout of device " << m_device << ':' << errmsg;
		g_free(errmsg);
                get_sensors().log(Sensors::loglevel_warning, oss.str());
                m_conn = Glib::signal_timeout().connect_seconds(sigc::mem_fun(*this, &SensorKingGPSTTY::try_connect), 15);
                return true;
	}
	m_conn = Glib::signal_io().connect(sigc::mem_fun(*this, &SensorKingGPSTTY::gps_poll), (int)(long long)m_fd, Glib::IO_IN);
#else
	m_fd = open(m_device.c_str(), O_RDONLY);
	if (m_fd == -1) {
		std::ostringstream oss;
		oss << "king gps: cannot open " << m_device << ':' << strerror(errno);
		get_sensors().log(Sensors::loglevel_warning, oss.str());
		m_conn = Glib::signal_timeout().connect_seconds(sigc::mem_fun(*this, &SensorKingGPSTTY::try_connect), 15);
		return true;
	}
        // set mode to raw
#ifdef HAVE_TERMIOS_H
	fcntl(m_fd, F_SETFL, fcntl(m_fd, F_GETFL, 0) | O_NONBLOCK);
        struct termios tm;
        memset(&tm, 0, sizeof(tm));
        tm.c_cflag = CS8 | CREAD | CLOCAL;
	cfsetispeed(&tm, baudrates[m_baudrate].tbaud);
        cfsetospeed(&tm, baudrates[m_baudrate].tbaud);
        if (tcsetattr(m_fd, TCSANOW, &tm)) {
		std::ostringstream oss;
		oss << "king gps: cannot set master terminal attributes " << m_device << ':' << strerror(errno);
		get_sensors().log(Sensors::loglevel_warning, oss.str());
		close();
		m_conn = Glib::signal_timeout().connect_seconds(sigc::mem_fun(*this, &SensorKingGPSTTY::try_connect), 15);
		return true;
	}
#endif
	m_conn = Glib::signal_io().connect(sigc::mem_fun(*this, &SensorKingGPSTTY::gps_poll), m_fd, Glib::IO_IN);
#endif
	m_conntimeout = Glib::signal_timeout().connect(sigc::mem_fun(*this, &SensorKingGPSTTY::gps_timeout), 2500);
	{
		std::ostringstream oss;
		oss << "king gps: starting device " << m_device;
		get_sensors().log(Sensors::loglevel_warning, oss.str());
	}
	return false;
}

bool Sensors::SensorKingGPSTTY::gps_poll(Glib::IOCondition iocond)
{
        if (!(iocond & Glib::IO_IN))
                return true;
	{
		char buf[1024];
#ifdef __MINGW32__
		DWORD r = 0;
		if (ReadFile(m_fd, buf, sizeof(buf), &r, NULL)) {
			if (r > 0) {
				bool update = parse(buf, buf + r);
				while (update) {
					update_gps();
					m_conntimeout.disconnect();
					m_conntimeout = Glib::signal_timeout().connect(sigc::mem_fun(*this, &SensorKingGPSTTY::gps_timeout), 2500);
					update = parse();
				}
			}
                        return true;
		}
#else
		int r = read(m_fd, buf, sizeof(buf));
		if (r > 0) {
			bool update = parse(buf, buf + r);
			while (update) {
				update_gps();
				m_conntimeout.disconnect();
				m_conntimeout = Glib::signal_timeout().connect(sigc::mem_fun(*this, &SensorKingGPSTTY::gps_timeout), 2500);
				update = parse();
			}
		}
		if (r >= 0 || errno == EAGAIN || errno == EINTR)
			return true;
#endif
	}
	{
		std::ostringstream oss;
		oss << "king gps: read error " << m_device << ':' << strerror(errno);
		get_sensors().log(Sensors::loglevel_warning, oss.str());
	}
	try_connect();
	return false;
}

bool Sensors::SensorKingGPSTTY::gps_timeout(void)
{
	m_conntimeout.disconnect();
	invalidate_update_gps();
	m_conntimeout = Glib::signal_timeout().connect_seconds(sigc::mem_fun(*this, &SensorKingGPSTTY::try_connect), 5);
	{
		std::ostringstream oss;
		oss << "king gps: timeout from device " << m_device;
		get_sensors().log(Sensors::loglevel_warning, oss.str());
	}
	return false;
}

void Sensors::SensorKingGPSTTY::get_param_desc(unsigned int pagenr, paramdesc_t& pd)
{
	SensorKingGPS::get_param_desc(pagenr, pd);
	{
		paramdesc_t::iterator pdi(pd.begin()), pde(pd.end());
		if (pdi != pde)
			++pdi;
		for (; pdi != pde; ++pdi)
			if (pdi->get_type() == ParamDesc::type_section)
				break;
		pdi = pd.insert(pdi, ParamDesc(ParamDesc::type_string, ParamDesc::editable_admin, parnrdevice, "Device", "Device Path", ""));
		++pdi;
		{
			ParamDesc::choices_t ch;
                        for (int i = 0; i < (int)(sizeof(baudrates)/sizeof(baudrates[0])); ++i) {
				std::ostringstream oss;
				oss << baudrates[i].baud;
                                ch.push_back(oss.str());
			}
			pdi = pd.insert(pdi, ParamDesc(ParamDesc::type_choice, ParamDesc::editable_admin, parnrbaudrate, "Baud Rate", "Baud Rate", "", ch.begin(), ch.end()));
		}
	}
}

Sensors::SensorKingGPSTTY::paramfail_t Sensors::SensorKingGPSTTY::get_param(unsigned int nr, Glib::ustring& v) const
{
	switch (nr) {
	default:
		break;

	case parnrdevice:
		v = m_device;
		return paramfail_ok;
	}
	return SensorKingGPS::get_param(nr, v);
}

Sensors::SensorKingGPSTTY::paramfail_t Sensors::SensorKingGPSTTY::get_param(unsigned int nr, double& v) const
{
	switch (nr) {
	default:
		break;
	}
	return SensorKingGPS::get_param(nr, v);
}

Sensors::SensorKingGPSTTY::paramfail_t Sensors::SensorKingGPSTTY::get_param(unsigned int nr, int& v) const
{
	switch (nr) {
	default:
		break;

	case parnrbaudrate:
		v = m_baudrate;
		return paramfail_ok;
	}
	return SensorKingGPS::get_param(nr, v);
}

void Sensors::SensorKingGPSTTY::set_param(unsigned int nr, const Glib::ustring& v)
{
	switch (nr) {
	default:
		SensorKingGPS::set_param(nr, v);
		return;

	case parnrdevice:
		if (v == m_device)
			return;
		m_device = v;
		get_sensors().get_configfile().set_string(get_configgroup(), "device", m_device);
		try_connect();
		break;
	}
	ParamChanged pc;
	pc.set_changed(nr);
	update(pc);
}

void Sensors::SensorKingGPSTTY::set_param(unsigned int nr, int v)
{
	switch (nr) {
	default:
		SensorKingGPS::set_param(nr, v);
		return;

	case parnrbaudrate:
		m_baudrate = std::min(v, (int)(sizeof(baudrates)/sizeof(baudrates[0]))-1);
		get_sensors().get_configfile().set_integer(get_configgroup(), "baudrate", baudrates[m_baudrate].baud);
		try_connect();
		break;
	}
	ParamChanged pc;
	pc.set_changed(nr);
	update(pc);
}
