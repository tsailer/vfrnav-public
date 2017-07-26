//
// C++ Implementation: NMEA GPS Sensor
//
// Description: GPS Sensor Handling
//
//
// Author: Thomas Sailer <t.sailer@alumni.ethz.ch>, (C) 2011, 2012, 2013
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
#include <iomanip>
#include <glibmm/datetime.h>
#ifdef HAVE_TERMIOS_H
#include <termios.h>
#endif
#include "sensgpsnmea.h"

// See http://www.gpsinformation.org/dale/nmea.htm

const unsigned int Sensors::SensorNMEAGPS::timeout_reconnect;
const unsigned int Sensors::SensorNMEAGPS::timeout_timeoutreconnect;
const unsigned int Sensors::SensorNMEAGPS::timeout_data;
const char Sensors::SensorNMEAGPS::cr;
const char Sensors::SensorNMEAGPS::lf;

const struct Sensors::SensorNMEAGPS::baudrates Sensors::SensorNMEAGPS::baudrates[8] = {
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

Sensors::SensorNMEAGPS::SensorNMEAGPS(Sensors& sensors, const Glib::ustring& configgroup)
	: SensorGPS(sensors, configgroup), m_course(std::numeric_limits<double>::quiet_NaN()),
	  m_gs(std::numeric_limits<double>::quiet_NaN()), m_alt(std::numeric_limits<double>::quiet_NaN()),
	  m_altrate(std::numeric_limits<double>::quiet_NaN()), m_altfixtime(std::numeric_limits<double>::quiet_NaN()),
	  m_pdop(std::numeric_limits<double>::quiet_NaN()), m_hdop(std::numeric_limits<double>::quiet_NaN()),
	  m_vdop(std::numeric_limits<double>::quiet_NaN()),
#ifdef __MINGW32__
	  m_fd(INVALID_HANDLE_VALUE),
#else
	  m_fd(-1),
#endif
	  m_baudrate(5)
{
	m_pos.set_invalid();
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
        if (cf.has_key(get_configgroup(), "tracefile")) {
		std::string fn(cf.get_string(get_configgroup(), "tracefile"));
		m_tracefile.open(fn.c_str(), std::ofstream::out | std::ofstream::app | std::ofstream::binary);
		if (!m_tracefile.is_open())
			get_sensors().log(Sensors::loglevel_warning, std::string("nmea gps: cannot open trace file ") + fn);
	}
        try_connect();
}

Sensors::SensorNMEAGPS::~SensorNMEAGPS()
{
        close();
}


bool Sensors::SensorNMEAGPS::parse(const std::string& raw)
{
	m_raw += raw;
	if (m_tracefile.is_open())
		m_tracefile << raw;
	return parse();
}

template <typename T>
bool Sensors::SensorNMEAGPS::parse(T b, T e)
{
	m_raw.insert(m_raw.end(), b, e);
	if (m_tracefile.is_open())
		m_tracefile << std::string(b, e);
	return parse();
}

template bool Sensors::SensorNMEAGPS::parse<const char *>(const char *b, const char *e);
template bool Sensors::SensorNMEAGPS::parse<char *>(char *b, char *e);

bool Sensors::SensorNMEAGPS::parse(void)
{
	bool work(false);
	std::string::iterator i(m_raw.begin()), e(m_raw.end());
	for (;;) {
		std::string::iterator i2(i);
		while (i2 != e && *i2 != cr && *i2 != lf)
			++i2;
		if (i2 == e)
			break;
		work = parse_line(i, i2) || work;
		i = i2;
		++i;
		while (i != e && (*i == cr || *i == lf))
			++i;
	}
	m_raw.erase(m_raw.begin(), i);
	return work;
}

bool Sensors::SensorNMEAGPS::parse_line(std::string::const_iterator lb, std::string::const_iterator le)
{
	if (lb == le || *lb != '$')
		return false;
	++lb;
	bool cksumok(false);
	{
		uint8_t cksum(0);
		std::string::const_iterator i(lb);
		while (i != le && *i != '*') {
			cksum ^= *i;
			++i;
		}
		if (i != le) {
			std::string::const_iterator i2(i);
			++i2;
			if (i2 == le)
				return false;
			std::string s(i2, le);
			if (s.size() != 2)
				return false;
			char *ep;
			unsigned int cksum2(strtoul(s.c_str(), &ep, 16));
			if (*ep)
				return false;
			if (cksum != cksum2)
				return false;
			cksumok = true;
			le = i;
		}
	}
	if (false)
		std::cerr << "NMEA GPS: sentence: \"" << std::string(lb, le) << "\"" << std::endl;
	tokens_t tok;
	while (lb != le) {
		std::string::const_iterator i(lb);
		while (i != le && *i != ',')
			++i;
		tok.push_back(std::string(lb, i));
		lb = i;
		if (lb == le)
			break;
		++lb;
	}
	if (tok.empty())
		return false;
	if (tok[0] == "GPGGA")
		return parse_gpgga(tok);
	if (tok[0] == "GPRMC")
		return /* cksumok && */ parse_gprmc(tok);
	if (tok[0] == "GPGSA")
		return parse_gpgsa(tok);
	if (tok[0] == "GPGSV")
		return parse_gpgsv(tok);
	return false;
}

double Sensors::SensorNMEAGPS::parse_double(const std::string& s)
{
	if (s.empty())
		return std::numeric_limits<double>::quiet_NaN();
	char *ep;
	double v(strtod(s.c_str(), &ep));
	if (*ep)
		return std::numeric_limits<double>::quiet_NaN();
	return v;
}

bool Sensors::SensorNMEAGPS::parse_uint(unsigned int& v, const std::string& s)
{
	if (s.empty())
		return false;
	char *ep;
	v = strtoul(s.c_str(), &ep, 10);
	if (*ep)
		return false;
	return true;
}

bool Sensors::SensorNMEAGPS::parse_gpgga(const tokens_t& tok)
{
	if (tok.size() < 13)
		return false;
	ParamChanged pc;
	{
		double ftime(std::numeric_limits<double>::quiet_NaN());
		if (tok[1].size() >= 6) {
			unsigned int h, m, s;
			ftime = parse_double(tok[1].substr(4));
			if (!std::isnan(s) && parse_uint(h, tok[1].substr(0, 2)) &&
			    parse_uint(m, tok[1].substr(2, 2))) {
				ftime += 60 * (m + 60 * h);
			}
		}
		double alt(parse_double(tok[9]));
		if (tok[10] != "M")
			alt = std::numeric_limits<double>::quiet_NaN();
		else
			alt *= Point::m_to_ft;
		double altrate(std::numeric_limits<double>::quiet_NaN());
		if (!std::isnan(alt) && !std::isnan(m_alt)) {
			double tdiff(ftime - m_altfixtime);
			if (!std::isnan(tdiff)) {
				if (tdiff < 0)
					tdiff += 24 * 60 * 60;
				if (tdiff > 0 && tdiff < 10)
					altrate = (alt - m_alt) / tdiff	* (Point::m_to_ft * 60.0);
			}
		}
		m_altfixtime = ftime;
		if (m_alt != alt) {
			m_alt = alt;
			pc.set_changed(parnralt);
		}
		if (m_altrate != altrate) {
			m_altrate = altrate;
			pc.set_changed(parnraltrate);
		}
	}
	{
		unsigned int f;
		if (parse_uint(f, tok[6])) {
			switch (f) {
			case 1:
				if (m_fixtype != fixtype_2d && m_fixtype != fixtype_3d) {
					m_fixtype = fixtype_2d;
					pc.set_changed(parnrgpstime);
				}
				break;

			case 2:
				if (m_fixtype != fixtype_3d_diff) {
					m_fixtype = fixtype_3d_diff;
					pc.set_changed(parnrgpstime);
				}
				break;

			default:
				if (m_fixtype != fixtype_nofix) {
					m_fixtype = fixtype_nofix;
					pc.set_changed(parnrgpstime);
				}
				break;
			}
		}
	}
	update(pc);
	if (false)
		std::cerr << "NMEA GPS: GGA: fix " << (unsigned int)m_fixtype << std::endl;
	return true;
}

bool Sensors::SensorNMEAGPS::parse_gprmc(const tokens_t& tok)
{
	if (tok.size() < 12)
		return false;
	ParamChanged pc;
	// Time & Date
	if (tok[1].size() >= 6 && tok[9].size() == 6) {
		unsigned int hh, mm, d;
		double ss(parse_double(tok[1].substr(4)));
		if (!std::isnan(ss) && parse_uint(hh, tok[1].substr(0, 2)) && parse_uint(mm, tok[1].substr(2, 2)) &&
		    parse_uint(d, tok[9])) {
			unsigned int y(d);
			d /= 100;
			y -= d * 100;
			unsigned int m(d);
			d /= 100;
			m -= d * 100;
			y += 1900;
			if (y < 1990)
				y += 100;
			Glib::DateTime::create_utc(y, m, d, hh, mm, Point::round<unsigned int,double>(ss)).to_timeval(m_fixtime);
			pc.set_changed(parnrgpstime);
		}
	}
	Point pos;
	pos.set_invalid();
	if (tok[2] == "A") {
		double lat, lon;
		lat = parse_double(tok[3].substr(2)) * (1.0 / 60);
		lon = parse_double(tok[5].substr(3)) * (1.0 / 60);
		{
			unsigned int v;
			if (!parse_uint(v, tok[3].substr(0, 2)))
				lat = std::numeric_limits<double>::quiet_NaN();
			else
				lat += v;
			if (!parse_uint(v, tok[5].substr(0, 3)))
				lon = std::numeric_limits<double>::quiet_NaN();
			else
				lon += v;
		}
		if (tok[4] == "S")
			lat = -lat;
		else if (tok[4] != "N")
			lat = std::numeric_limits<double>::quiet_NaN();
		if (tok[6] == "W")
			lon = -lon;
		else if (tok[6] != "E")
			lon = std::numeric_limits<double>::quiet_NaN();
		if (!std::isnan(lat) && !std::isnan(lon)) {
			pos.set_lat_deg_dbl(lat);
			pos.set_lon_deg_dbl(lon);
		}
	}
	if (pos.is_invalid()) {
		if (!m_pos.is_invalid()) {

			pc.set_changed(parnrlat);
			pc.set_changed(parnrlon);
		}
	} else {
		if (m_pos.is_invalid()) {
			pc.set_changed(parnrlat);
			pc.set_changed(parnrlon);
		} else {
			if (pos.get_lat() != m_pos.get_lat())
				pc.set_changed(parnrlat);
			if (pos.get_lon() != m_pos.get_lon())
				pc.set_changed(parnrlon);
		}
	}
	m_pos = pos;
	m_gs = parse_double(tok[7]);
	pc.set_changed(parnrgroundspeed);
	m_course = parse_double(tok[8]);
	pc.set_changed(parnrcourse);
	m_time.assign_current_time();
	pc.set_changed(parnrtime);
	update(pc);
	if (false)
		std::cerr << "NMEA GPS: RMC: fix " << (unsigned int)m_fixtype << std::endl;
	return true;
}

bool Sensors::SensorNMEAGPS::parse_gpgsa(const tokens_t& tok)
{
	if (tok.size() < 3)
		return false;
	ParamChanged pc;
	{
		unsigned int f;
		if (parse_uint(f, tok[2])) {
			switch (f) {
			case 2:
				if (m_fixtype == fixtype_3d || m_fixtype == fixtype_3d_diff) {
					m_fixtype = fixtype_2d;
					pc.set_changed(parnrgpstime);
				}
				break;

			case 3:
				if (m_fixtype == fixtype_2d) {
					m_fixtype = fixtype_3d;
					pc.set_changed(parnrgpstime);
				}
				break;

			default:
				if (m_fixtype != fixtype_nofix) {
					m_fixtype = fixtype_nofix;
					pc.set_changed(parnrgpstime);
				}
				break;
			}
		}
	}
	m_satinuse.clear();
	for (unsigned int i = 3; i < 15; ++i) {
		if (i >= tok.size())
			break;
		unsigned int nr;
		if (!parse_uint(nr, tok[i]))
			continue;
		if (!nr)
			continue;
		m_satinuse.insert(nr);
	}
	for (satellites_t::iterator i(m_satellites.begin()), e(m_satellites.end()); i != e; ++i) {
		unsigned int prn(i->get_prn());
		if (!prn)
			continue;
		bool inuse(m_satinuse.find(prn) != m_satinuse.end());
		if (inuse == i->is_used())
			continue;
		(*i) = Satellite(i->get_prn(), i->get_azimuth(), i->get_elevation(), i->get_snr(), inuse);
		pc.set_changed(parnrsatellites);
	}
	if (tok.size() < 16)
		m_pdop = std::numeric_limits<double>::quiet_NaN();
	else
		m_pdop = parse_double(tok[15]);
	if (tok.size() < 17)
		m_hdop = std::numeric_limits<double>::quiet_NaN();
	else
		m_hdop = parse_double(tok[16]);
	if (tok.size() < 18)
		m_vdop = std::numeric_limits<double>::quiet_NaN();
	else
		m_vdop = parse_double(tok[17]);
	pc.set_changed(parnrpdop);
	pc.set_changed(parnrhdop);
	pc.set_changed(parnrvdop);
	update(pc);
	if (false)
		std::cerr << "NMEA GPS: GSA: fix " << (unsigned int)m_fixtype << std::endl;
	return true;
}

bool Sensors::SensorNMEAGPS::parse_gpgsv(const tokens_t& tok)
{
	if (tok.size() < 4)
		return false;
	unsigned int seqnr, seqmax;
	if (!parse_uint(seqmax, tok[1]) || !parse_uint(seqnr, tok[2]) || seqmax < 1 || seqnr < 1)
		return false;
	if (seqnr > seqmax)
		return false;
	if (seqnr == 1) {
		m_satparse.clear();
		unsigned int nrsat;
		if (!parse_uint(nrsat, tok[3]))
			return false;
		if (nrsat > 64)
			return false;
		m_satparse.resize(nrsat, Satellite(0, 0, 0, false));
	}
	if (m_satparse.empty())
		return false;
	bool last(seqnr == seqmax);
	--seqnr;
	seqnr *= 4;
	for (unsigned int i = 7; i < tok.size(); i += 4, ++seqnr) {
		if (seqnr >= m_satparse.size())
			break;
		unsigned int prn;
		bool ok(parse_uint(prn, tok[i - 3]));
		double el(parse_double(tok[i - 2]));
		double az(parse_double(tok[i - 1]));
		double snr(parse_double(tok[i]));
		if (!ok || std::isnan(el) || std::isnan(az)) {
			m_satparse[seqnr] = Satellite(0, 0, 0, false);
			continue;
		}
		bool inuse(m_satinuse.find(prn) != m_satinuse.end());
		m_satparse[seqnr] = Satellite(prn, az, el, std::isnan(snr) ? 0 : snr, !std::isnan(snr) && prn);
	}
	if (!last)
		return true;
	{
		ParamChanged pc;
		if (m_satparse.size() != m_satellites.size()) {
			pc.set_changed(parnrsatellites);
		} else {
			for (unsigned int i = 0, n = m_satparse.size(); i < n; ++i) {
				if (!m_satparse[i].compare_all(m_satellites[i]))
					continue;
				pc.set_changed(parnrsatellites);
				break;
			}
		}
		m_satellites.swap(m_satparse);
		m_satparse.clear();
		update(pc);
	}
	if (false)
		std::cerr << "NMEA GPS: GSV: nr satellites " << m_satellites.size() << std::endl;
	return true;
}

bool Sensors::SensorNMEAGPS::get_position(Point& pt) const
{
	if (m_fixtype < fixtype_2d || m_pos.is_invalid()) {
		pt = Point();
		return false;
	}
	pt = m_pos;
	return true;
}

Glib::TimeVal Sensors::SensorNMEAGPS::get_position_time(void) const
{
	return m_time;
}

void Sensors::SensorNMEAGPS::get_truealt(double& alt, double& altrate) const
{
	alt = altrate = std::numeric_limits<double>::quiet_NaN();
	if (m_fixtype < fixtype_3d)
		return;
	alt = m_alt;
	altrate = m_altrate;
}

Glib::TimeVal Sensors::SensorNMEAGPS::get_truealt_time(void) const
{
	return m_time;
}

void Sensors::SensorNMEAGPS::get_course(double& crs, double& gs) const
{
	crs = gs = std::numeric_limits<double>::quiet_NaN();
	if (m_fixtype < fixtype_2d)
		return;
	crs = m_course;
	gs = m_gs;
}

Glib::TimeVal Sensors::SensorNMEAGPS::get_course_time(void) const
{
	return m_time;
}

void Sensors::SensorNMEAGPS::invalidate_update_gps(void)
{
	m_pos.set_invalid();
	m_course = m_gs = m_alt = m_altrate = m_altfixtime = m_pdop = m_hdop = m_vdop = std::numeric_limits<double>::quiet_NaN();
	m_satellites.clear();
	m_satparse.clear();
	m_satinuse.clear();
	ParamChanged pc;
        pc.set_changed(parnrfixtype, parnrgpstime);
        update(pc);
}

bool Sensors::SensorNMEAGPS::close(void)
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

bool Sensors::SensorNMEAGPS::try_connect(void)
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
                m_conn = Glib::signal_timeout().connect_seconds(sigc::mem_fun(*this, &SensorNMEAGPS::try_connect), timeout_reconnect);
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
                m_conn = Glib::signal_timeout().connect_seconds(sigc::mem_fun(*this, &SensorNMEAGPS::try_connect), timeout_reconnect);
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
                m_conn = Glib::signal_timeout().connect_seconds(sigc::mem_fun(*this, &SensorNMEAGPS::try_connect), timeout_reconnect);
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
                m_conn = Glib::signal_timeout().connect_seconds(sigc::mem_fun(*this, &SensorNMEAGPS::try_connect), timeout_reconnect);
                return true;
	}
        m_conn = Glib::signal_io().connect(sigc::mem_fun(*this, &SensorNMEAGPS::gps_poll), (int)(long long)m_fd, Glib::IO_IN);
#else
        m_fd = open(m_device.c_str(), O_RDONLY);
        if (m_fd == -1) {
                std::ostringstream oss;
                oss << "nmea gps: cannot open " << m_device << ':' << strerror(errno);
                get_sensors().log(Sensors::loglevel_warning, oss.str());
                m_conn = Glib::signal_timeout().connect_seconds(sigc::mem_fun(*this, &SensorNMEAGPS::try_connect), timeout_reconnect);
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
                oss << "nmea gps: cannot set master terminal attributes " << m_device << ':' << strerror(errno);
                get_sensors().log(Sensors::loglevel_warning, oss.str());
                close();
                m_conn = Glib::signal_timeout().connect_seconds(sigc::mem_fun(*this, &SensorNMEAGPS::try_connect), timeout_reconnect);
                return true;
        }
#endif
        m_conn = Glib::signal_io().connect(sigc::mem_fun(*this, &SensorNMEAGPS::gps_poll), m_fd, Glib::IO_IN);
#endif
        m_conntimeout = Glib::signal_timeout().connect(sigc::mem_fun(*this, &SensorNMEAGPS::gps_timeout), timeout_data);
        {
                std::ostringstream oss;
                oss << "nmea gps: starting device " << m_device;
                get_sensors().log(Sensors::loglevel_warning, oss.str());
        }
        return false;
}

bool Sensors::SensorNMEAGPS::gps_poll(Glib::IOCondition iocond)
{
        if (!(iocond & Glib::IO_IN))
                return true;
        {
                char buf[1024];
#ifdef __MINGW32__
		DWORD r = 0;
		if (ReadFile(m_fd, buf, sizeof(buf), &r, NULL)) {
			if (r > 0) {
				bool work = parse(buf, buf + r);
				if (work) {
					m_conntimeout.disconnect();
					m_conntimeout = Glib::signal_timeout().connect(sigc::mem_fun(*this, &SensorNMEAGPS::gps_timeout), timeout_data);
				}
			}
                        return true;
		}
#else
                int r = read(m_fd, buf, sizeof(buf));
                if (r > 0) {
                        bool work = parse(buf, buf + r);
                        if (work) {
                                m_conntimeout.disconnect();
                                m_conntimeout = Glib::signal_timeout().connect(sigc::mem_fun(*this, &SensorNMEAGPS::gps_timeout), timeout_data);
                        }
                }
                if (r >= 0 || errno == EAGAIN || errno == EINTR)
                        return true;
#endif
        }
        {
                std::ostringstream oss;
                oss << "nmea gps: read error " << m_device << ':' << strerror(errno);
                get_sensors().log(Sensors::loglevel_warning, oss.str());
        }
        try_connect();
        return false;
}

bool Sensors::SensorNMEAGPS::gps_timeout(void)
{
        m_conntimeout.disconnect();
        invalidate_update_gps();
        m_conntimeout = Glib::signal_timeout().connect_seconds(sigc::mem_fun(*this, &SensorNMEAGPS::try_connect), timeout_timeoutreconnect);
        {
                std::ostringstream oss;
                oss << "nmea gps: timeout from device " << m_device;
                get_sensors().log(Sensors::loglevel_warning, oss.str());
        }
        return false;
}

void Sensors::SensorNMEAGPS::get_param_desc(unsigned int pagenr, paramdesc_t& pd)
{
	SensorGPS::get_param_desc(pagenr, pd);
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

	pd.push_back(ParamDesc(ParamDesc::type_satellites, ParamDesc::editable_readonly, parnrsatellites, "Satellites", "Satellite Azimuth/Elevation and Signal Strengths", ""));

        pd.push_back(ParamDesc(ParamDesc::type_section, ParamDesc::editable_readonly, ~0U, "Uncertainties", "Uncertainties", ""));
        pd.push_back(ParamDesc(ParamDesc::type_double, ParamDesc::editable_readonly, parnrpdop, "PDOP", "Position Dilution of Precision", "", 3, -999999, 999999, 0.0, 0.0));
        pd.push_back(ParamDesc(ParamDesc::type_double, ParamDesc::editable_readonly, parnrhdop, "HDOP", "Position Dilution of Precision", "", 3, -999999, 999999, 0.0, 0.0));
        pd.push_back(ParamDesc(ParamDesc::type_double, ParamDesc::editable_readonly, parnrvdop, "VDOP", "Position Dilution of Precision", "", 3, -999999, 999999, 0.0, 0.0));

	pd.push_back(ParamDesc(ParamDesc::type_section, ParamDesc::editable_readonly, ~0U, "Status", "GPS Device Status", ""));
	pd.push_back(ParamDesc(ParamDesc::type_string, ParamDesc::editable_readonly, parnrgpstime, "GPS Time", "GPS Receiver Time", ""));
}

Sensors::SensorNMEAGPS::paramfail_t Sensors::SensorNMEAGPS::get_param(unsigned int nr, Glib::ustring& v) const
{
	switch (nr) {
	default:
		break;

	case parnrgpstime:
	{
                v.clear();
                if (m_fixtype < fixtype_2d)
                        return paramfail_fail;
		if (!m_fixtime.valid())
                        return paramfail_fail;
                Glib::DateTime dt(Glib::DateTime::create_now_utc(m_fixtime));
                v = dt.format("%Y-%m-%d %H:%M:%S");
                return paramfail_ok;
        }

        case parnrdevice:
                v = m_device;
                return paramfail_ok;
	}
	return SensorGPS::get_param(nr, v);
}

Sensors::SensorNMEAGPS::paramfail_t Sensors::SensorNMEAGPS::get_param(unsigned int nr, double& v) const
{
	switch (nr) {
	default:
		break;

	case parnrpdop:
		v = m_pdop;
		return std::isnan(v) ? paramfail_fail : paramfail_ok;

	case parnrhdop:
		v = m_hdop;
		return std::isnan(v) ? paramfail_fail : paramfail_ok;

	case parnrvdop:
		v = m_vdop;
		return std::isnan(v) ? paramfail_fail : paramfail_ok;
	}
	return SensorGPS::get_param(nr, v);
}

Sensors::SensorNMEAGPS::paramfail_t Sensors::SensorNMEAGPS::get_param(unsigned int nr, int& v) const
{
	switch (nr) {
	default:
		break;

        case parnrbaudrate:
                v = m_baudrate;
                return paramfail_ok;
	}
	return SensorGPS::get_param(nr, v);
}

Sensors::SensorNMEAGPS::paramfail_t Sensors::SensorNMEAGPS::get_param(unsigned int nr, satellites_t& sat) const
{
        sat = m_satellites;
#ifdef __MINGW32__
        return (m_fd != INVALID_HANDLE_VALUE) ? paramfail_ok : paramfail_fail;
#else
        return (m_fd != -1) ? paramfail_ok : paramfail_fail;
#endif
}

void Sensors::SensorNMEAGPS::set_param(unsigned int nr, const Glib::ustring& v)
{
        switch (nr) {
        default:
                SensorGPS::set_param(nr, v);
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

void Sensors::SensorNMEAGPS::set_param(unsigned int nr, int v)
{
	switch (nr) {
	default:
		SensorGPS::set_param(nr, v);
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

std::string Sensors::SensorNMEAGPS::logfile_header(void) const
{
	return SensorGPS::logfile_header() + ",FixTime,Lat,Lon,Alt,VS,CRS,GS,PDOP,HDOP,VDOP,NrSat";
}

std::string Sensors::SensorNMEAGPS::logfile_records(void) const
{
	std::ostringstream oss;
	oss << SensorGPS::logfile_records() << std::fixed << std::setprecision(6) << ',';
	if (m_fixtime.valid() && m_fixtype >= fixtype_2d) {
                Glib::DateTime dt(Glib::DateTime::create_now_utc(m_fixtime));
                oss << dt.format("%Y-%m-%d %H:%M:%S");
	}
	oss << ',';
	if (m_fixtype < fixtype_2d || m_pos.is_invalid())
		oss << ',';
	else
		oss << m_pos.get_lat_deg_dbl() << ',' << m_pos.get_lon_deg_dbl();
	oss << ',';
	if (!std::isnan(m_alt))
		oss << m_alt;
	oss << ',';
	if (!std::isnan(m_altrate))
		oss << m_altrate;
	oss << ',';
	if (!std::isnan(m_course))
		oss << m_course;
	oss << ',';
	if (!std::isnan(m_gs))
		oss << m_gs;
	oss << ',';
	if (!std::isnan(m_pdop))
		oss << m_pdop;
	oss << ',';
	if (!std::isnan(m_hdop))
		oss << m_hdop;
	oss << ',';
	if (!std::isnan(m_vdop))
		oss << m_vdop;
	oss << ',' << m_satellites.size();
	return oss.str();
}
