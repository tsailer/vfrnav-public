//
// C++ Implementation: Attitude Sensor: psmove using direct bluetooth sockets
//
// Description: Attitude Sensor Handling
//
//
// Author: Thomas Sailer <t.sailer@alumni.ethz.ch>, (C) 2012
//
// Copyright: See COPYING file that comes with this distribution
//
//

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "sysdeps.h"

#include <iostream>
#include <iomanip>
#include <errno.h>

#include "sensattpsmovebt.h"

#include <bluetooth/l2cap.h>

#define L2CAP_PSM_HIDP_CTRL 0x11
#define L2CAP_PSM_HIDP_INTR 0x13

#define HIDP_TRANS_GET_REPORT    0x40
#define HIDP_TRANS_SET_REPORT    0x50
#define HIDP_DATA_RTYPE_INPUT    0x01
#define HIDP_DATA_RTYPE_OUTPUT   0x02
#define HIDP_DATA_RTYPE_FEATURE  0x03

const bdaddr_t Sensors::SensorAttitudePSMoveBT::bdaddr_any = { {0, 0, 0, 0, 0, 0} };

Sensors::SensorAttitudePSMoveBT::SensorAttitudePSMoveBT(Sensors& sensors, const Glib::ustring& configgroup)
	: SensorAttitudePSMove(sensors, configgroup), m_cskl(-1), m_iskl(-1), m_csk(-1), m_isk(-1)
{
	memset(&m_ifaddr, 0, sizeof(m_ifaddr));
	memset(&m_devaddr, 0, sizeof(m_devaddr));
	const Glib::KeyFile& cf(get_sensors().get_configfile());
	if (!cf.has_key(get_configgroup(), "ifaddr"))
		get_sensors().get_configfile().set_string(get_configgroup(), "ifaddr", bdaddr_str(&bdaddr_any));
	{
		Glib::ustring a(cf.get_string(get_configgroup(), "ifaddr"));
		str2ba(a.c_str(), &m_ifaddr);
	}
	if (!cf.has_key(get_configgroup(), "devaddr"))
		get_sensors().get_configfile().set_string(get_configgroup(), "devaddr", bdaddr_str(&bdaddr_any));
	{
		Glib::ustring a(cf.get_string(get_configgroup(), "devaddr"));
		str2ba(a.c_str(), &m_devaddr);
	}
	setup_listen();
}

Sensors::SensorAttitudePSMoveBT::~SensorAttitudePSMoveBT()
{
	close_listen();
	close_dev();
}

void Sensors::SensorAttitudePSMoveBT::close_listen(void)
{
	if (m_iskl != -1)
		::close(m_iskl);
	if (m_cskl != -1)
		::close(m_cskl);
	m_iskl = m_cskl = -1;
	m_connlisten.disconnect();
}

void Sensors::SensorAttitudePSMoveBT::close_dev(void)
{
	if (m_isk != -1)
		::close(m_isk);
	if (m_csk != -1)
		::close(m_csk);
	m_isk = m_csk = -1;
}

int Sensors::SensorAttitudePSMoveBT::l2cap_listen(const bdaddr_t *bdaddr, unsigned short psm)
{
        int sk = socket(PF_BLUETOOTH, SOCK_SEQPACKET, BTPROTO_L2CAP);
        if (sk < 0) {
                std::cerr << "l2cap_listen: failed to create socket" << std::endl;
                return -1;
        }
        struct sockaddr_l2 addr;
        memset(&addr, 0, sizeof(addr));
        addr.l2_family = AF_BLUETOOTH;
        addr.l2_bdaddr = *bdaddr;
        addr.l2_psm = htobs(psm);
        if (bind(sk, (struct sockaddr *)&addr, sizeof(addr)) < 0) {
                std::cerr << "l2cap_listen: bind error address " << bdaddr_str(bdaddr) << std::endl;
                ::close(sk);
                return -1;
        }
        if (listen(sk, 5) < 0) {
                ::close(sk);
                std::cerr << "l2cap_listen: listen error address " << bdaddr_str(bdaddr) << std::endl;
                return -1;
        }
        return sk;
}

std::string Sensors::SensorAttitudePSMoveBT::bdaddr_str(const bdaddr_t *bdaddr)
{
	char str[32];
	ba2str(bdaddr, str);
	return str;
}

std::string Sensors::SensorAttitudePSMoveBT::ifaddr_str(void) const
{
	return bdaddr_str(&m_ifaddr);
}

std::string Sensors::SensorAttitudePSMoveBT::devaddr_str(void) const
{
	return bdaddr_str(&m_devaddr);
}

bool Sensors::SensorAttitudePSMoveBT::setup_listen(void)
{
	if (is_listen())
		return false;
	close_listen();
	m_cskl = l2cap_listen(&m_ifaddr, L2CAP_PSM_HIDP_CTRL);
        m_iskl = l2cap_listen(&m_ifaddr, L2CAP_PSM_HIDP_INTR);
        if (m_cskl >= 0 && m_iskl >= 0) {
                m_connlisten = Glib::signal_io().connect(sigc::mem_fun(*this, &SensorAttitudePSMoveBT::handle_listen_read),
							 m_cskl, Glib::IO_IN | Glib::IO_HUP);
		get_sensors().log(Sensors::loglevel_notice, "PSMove: Listen for " + devaddr_str() + " on interface " + ifaddr_str());
		return false;
	}
	m_connlisten = Glib::signal_timeout().connect_seconds(sigc::mem_fun(*this, &SensorAttitudePSMoveBT::setup_listen),
							      30, Glib::PRIORITY_DEFAULT);
	get_sensors().log(Sensors::loglevel_notice, "PSMove: Cannot listen, retry in 30 seconds");
	return true;
}

bool Sensors::SensorAttitudePSMoveBT::handle_listen_read(Glib::IOCondition iocond)
{
        struct sockaddr_l2 addr1;
	socklen_t addr1len(sizeof(addr1));
        int csk = accept(m_cskl, (sockaddr *)&addr1, &addr1len);
        if (csk < 0) {
		get_sensors().log(Sensors::loglevel_notice, "PSMove: CTRL accept failed");
		if (errno != EAGAIN && errno != EINTR) {
			close_listen();
			setup_listen();
			return false;
		}
                return true;
        }
        struct sockaddr_l2 addr2;
	socklen_t addr2len(sizeof(addr2));
        int isk = accept(m_iskl, (sockaddr *)&addr2, &addr2len);
        if (isk < 0) {
                ::close(csk);
		get_sensors().log(Sensors::loglevel_notice, "PSMove: INTR accept failed");
		if (errno != EAGAIN && errno != EINTR) {
			close_listen();
			setup_listen();
			return false;
		}
                return true;
        }
	if (addr1.l2_family != AF_BLUETOOTH || addr2.l2_family != AF_BLUETOOTH || 
	    (bacmp(&m_devaddr, &bdaddr_any) && (bacmp(&m_devaddr, &addr1.l2_bdaddr) || bacmp(&m_devaddr, &addr2.l2_bdaddr)))) {
		get_sensors().log(Sensors::loglevel_notice, "PSMove: rejecting device address " + bdaddr_str(&addr1.l2_bdaddr)
				  + "/" + bdaddr_str(&addr2.l2_bdaddr) + " expecting " + devaddr_str());
		::close(csk);
		::close(isk);
		return true;
	}
	if (is_open()) {
		get_sensors().log(Sensors::loglevel_notice, "PSMove: rejecting reconnect?");
		::close(csk);
		::close(isk);
		return true;
	}
	setup_device(csk, isk, &addr1.l2_bdaddr);
        return true;
}

void Sensors::SensorAttitudePSMoveBT::setup_device(int csk, int isk, const bdaddr_t *bdaddr)
{
	if (csk == -1 || isk == -1) {
		if (csk != -1)
			::close(csk);
		if (isk != -1)
			::close(isk);
		return;
	}
        {
                // Distinguish SIXAXIS / DS3 / PSMOVE.
                uint8_t resp[64];
                uint8_t get03f2[] = { HIDP_TRANS_GET_REPORT | HIDP_DATA_RTYPE_FEATURE | 8,
                                      0xf2, sizeof(resp), sizeof(resp)>>8 };
                if (send(m_csk, get03f2, sizeof(get03f2), 0) < 0) {
			::close(csk);
			::close(isk);
			get_sensors().log(Sensors::loglevel_warning, "PSMove: report send failure");
                        return;
                }
                int nr = recv(m_csk, resp, sizeof(resp), 0);
                if (nr < 0) {
 			::close(csk);
			::close(isk);
			get_sensors().log(Sensors::loglevel_warning, "PSMove: report receive failure");
			return;
                }
		if (open(resp, nr, bdaddr ? bdaddr_str(bdaddr) : "")) {
			close_dev();
			m_csk = csk;
			m_isk = isk;
			m_conndev = Glib::signal_io().connect(sigc::mem_fun(*this, &SensorAttitudePSMoveBT::handle_dev_read),
							      m_isk, Glib::IO_IN | Glib::IO_HUP);
			return;
                }
                if (resp[13] == 0x40) {
                        // sixaxis
                } else {
                        // ds3
                }
        }
	::close(csk);
	::close(isk);
	get_sensors().log(Sensors::loglevel_warning, "PSMove: unrecognized device");
	return;
}

bool Sensors::SensorAttitudePSMoveBT::handle_dev_read(Glib::IOCondition iocond)
{
        uint8_t report[256];
        int nr = recv(m_isk, report, sizeof(report), 0);
        if (nr <= 0) {
		get_sensors().log(Sensors::loglevel_warning, "PSMove: device disconnect");
		close_dev();
		close();
                return false;
        }
        if (false) {
                std::ostringstream oss;
                oss << std::hex;
                for (int i = 0;;) {
                        oss << std::setw(2) << std::setfill('0') << (unsigned int)report[i];
                        ++i;
                        if (i >= nr)
                                break;
                        oss << ' ';
                }
                std::cerr << "BluetoothDevice: report " << oss.str() << std::endl;
        }
        if (nr <= 2 || report[0] != 0xa1)
                return true;
        handle_report(report + 1, nr - 1);
        return true;
}

void Sensors::SensorAttitudePSMoveBT::get_param_desc(unsigned int pagenr, paramdesc_t& pd)
{
	Sensors::SensorAttitudePSMove::get_param_desc(pagenr, pd);
	if (pagenr)
		return;
	paramdesc_t::iterator pdi(pd.begin());
	{
		paramdesc_t::iterator pde(pd.end());
		if (pdi != pde)
			++pdi;
		for (; pdi != pde; ++pdi)
			if (pdi->get_type() == ParamDesc::type_section)
				break;
	}
	pdi = pd.insert(pdi, ParamDesc(ParamDesc::type_string, ParamDesc::editable_admin, parnrdevaddr, "Device Addr", "Constrain to Device Bluetooth Address", ""));
	pdi = pd.insert(pdi, ParamDesc(ParamDesc::type_string, ParamDesc::editable_admin, parnrifaddr, "Interface Addr", "Constrain to Interface Bluetooth Address", ""));
}

Sensors::SensorAttitudePSMoveBT::paramfail_t Sensors::SensorAttitudePSMoveBT::get_param(unsigned int nr, Glib::ustring& v) const
{
	switch (nr) {
	default:
		break;

	case parnrdevaddr:
		v = bdaddr_str(&m_devaddr);
		return paramfail_ok;

	case parnrifaddr:
		v = bdaddr_str(&m_ifaddr);
		return paramfail_ok;
	}
	return SensorAttitudePSMove::get_param(nr, v);


}

void Sensors::SensorAttitudePSMoveBT::set_param(unsigned int nr, const Glib::ustring& v)
{
	switch (nr) {
	default:
		SensorAttitudePSMove::set_param(nr, v);
		return;

	case parnrdevaddr:
	case parnrifaddr:
	{
		bdaddr_t *paddr((nr == parnrifaddr) ? &m_ifaddr : &m_devaddr);
		bdaddr_t addr;
		str2ba(v.c_str(), &addr);
		if (!bacmp(&addr, paddr))
			return;
		bacpy(paddr, &addr);
		get_sensors().get_configfile().set_string(get_configgroup(), (nr == parnrifaddr) ? "ifaddr" : "devaddr", bdaddr_str(paddr));
		break;
	}
	}
	ParamChanged pc;
	pc.set_changed(nr);
	update(pc);
}
