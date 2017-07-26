//
// C++ Implementation: Remote ADS-B Receiver
//
// Description: Remote ADS-B Receiver
//
//
// Author: Thomas Sailer <t.sailer@alumni.ethz.ch>, (C) 2013
//
// Copyright: See COPYING file that comes with this distribution
//
//

#include <glibmm/datetime.h>

#include "sensremoteadsb.h"

Sensors::SensorRemoteADSB::SensorRemoteADSB(Sensors& sensors, const Glib::ustring& configgroup)
	: SensorADSB(sensors, configgroup), m_remoteaddr("localhost:30002")
{
	m_cancel = Gio::Cancellable::create();
	// get configuration
        const Glib::KeyFile& cf(get_sensors().get_configfile());
        if (!cf.has_key(get_configgroup(), "remoteaddr"))
		get_sensors().get_configfile().set_string(get_configgroup(), "remoteaddr", m_remoteaddr);
        m_remoteaddr = cf.get_string(get_configgroup(), "remoteaddr");
	try_connect();
}

Sensors::SensorRemoteADSB::~SensorRemoteADSB()
{
	close();
}

bool Sensors::SensorRemoteADSB::close(void)
{
	clear();
        m_conntimeout.disconnect();
	m_cancel->cancel();
	m_sockclient = Glib::RefPtr<Gio::SocketClient>();
	m_sockconn = Glib::RefPtr<Gio::SocketConnection>();
	m_buffer.clear();
	return true;
}

bool Sensors::SensorRemoteADSB::try_connect(void)
{
	close();
	m_cancel->reset();
        m_sockclient = Gio::SocketClient::create();
	m_sockclient->connect_to_host_async(m_remoteaddr, 30002, m_cancel, sigc::mem_fun(*this, &SensorRemoteADSB::connect_finished));
	m_buffer.clear();
	m_conntimeout = Glib::signal_timeout().connect_seconds(sigc::mem_fun(*this, &SensorRemoteADSB::try_connect), 15);
	return false;
}

void Sensors::SensorRemoteADSB::connect_finished(Glib::RefPtr<Gio::AsyncResult>& ar)
{
        m_conntimeout.disconnect();
	m_sockconn = m_sockclient->connect_finish(ar);
	if (!m_sockconn) {
	        get_sensors().log(Sensors::loglevel_warning, "Remote ADSB: cannot connect to " + m_remoteaddr);
		m_conntimeout = Glib::signal_timeout().connect_seconds(sigc::mem_fun(*this, &SensorRemoteADSB::try_connect), 15);
		return;
	}
	get_sensors().log(Sensors::loglevel_notice, "Remote ADSB: connected to " + m_remoteaddr);
 	m_sockconn->get_input_stream()->read_async(m_readbuffer, sizeof(m_readbuffer),
						   sigc::mem_fun(*this, &SensorRemoteADSB::read_finished), m_cancel);
}

void Sensors::SensorRemoteADSB::read_finished(Glib::RefPtr<Gio::AsyncResult>& ar)
{
	if (!m_sockconn)
		return;
	gssize ret(m_sockconn->get_input_stream()->read_finish(ar));
	if (ret == -1) {
		get_sensors().log(Sensors::loglevel_warning, "Remote ADSB: read error " + m_remoteaddr);
		close();
		m_conntimeout = Glib::signal_timeout().connect_seconds(sigc::mem_fun(*this, &SensorRemoteADSB::try_connect), 5);
		return;
	}
	m_buffer += std::string(m_readbuffer, m_readbuffer + ret);
	std::string::iterator si(m_buffer.begin()), se(m_buffer.end());
	for (;;) {
		if (si == se)
			break;
		if (*si == '\r' || *si == '\n') {
			ModeSMessage msg;
			if (msg.parse_line(m_buffer.begin(), si) != m_buffer.begin())
				receive(msg);
			do {
				++si;
			} while (si != se && (*si == '\r' || *si == '\n'));
			m_buffer.erase(m_buffer.begin(), si);
			si = m_buffer.begin();
			se = m_buffer.end();
			continue;
		}
		++si;
	}
 	m_sockconn->get_input_stream()->read_async(m_readbuffer, sizeof(m_readbuffer),
						   sigc::mem_fun(*this, &SensorRemoteADSB::read_finished), m_cancel);
}

void Sensors::SensorRemoteADSB::get_param_desc(unsigned int pagenr, paramdesc_t& pd)
{
	SensorADSB::get_param_desc(pagenr, pd);
        {
                paramdesc_t::iterator pdi(pd.begin()), pde(pd.end());
                if (pdi != pde)
                        ++pdi;
                for (; pdi != pde; ++pdi)
                        if (pdi->get_type() == ParamDesc::type_section)
                                break;
		pdi = pd.insert(pdi, ParamDesc(ParamDesc::type_string, ParamDesc::editable_admin, parnrremoteaddr, "Addr", "Remote Receiver Address and Port", ""));
        }
}

Sensors::SensorRemoteADSB::paramfail_t Sensors::SensorRemoteADSB::get_param(unsigned int nr, Glib::ustring& v) const
{
	switch (nr) {
	default:
		break;

	case parnrremoteaddr:
		v = m_remoteaddr;
		return paramfail_ok;
	}
	return SensorADSB::get_param(nr, v);
}

void Sensors::SensorRemoteADSB::set_param(unsigned int nr, const Glib::ustring& v)
{
	switch (nr) {
	default:
		SensorADSB::set_param(nr, v);
		return;

	case parnrremoteaddr:
		if (m_remoteaddr == v)
			return;
		m_remoteaddr = v;
		get_sensors().get_configfile().set_string(get_configgroup(), "remoteaddr", m_remoteaddr);
		close();
		try_connect();
		break;
	}
	ParamChanged pc;
	pc.set_changed(nr);
	update(pc);
}
