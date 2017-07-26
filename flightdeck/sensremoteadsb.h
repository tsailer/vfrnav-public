//
// C++ Interface: Remote ADS-B Receiver
//
// Description: Remote ADS-B Receiver
//
//
// Author: Thomas Sailer <t.sailer@alumni.ethz.ch>, (C) 2013
//
// Copyright: See COPYING file that comes with this distribution
//
//

#ifndef SENSREMOTEADSB_H
#define SENSREMOTEADSB_H

#include <giomm.h>

#include "sensadsb.h"

class Sensors::SensorRemoteADSB : public Sensors::SensorADSB {
  public:
	SensorRemoteADSB(Sensors& sensors, const Glib::ustring& configgroup);
	virtual ~SensorRemoteADSB();

	virtual void get_param_desc(unsigned int pagenr, paramdesc_t& pd);
	using SensorADSB::get_param;
	virtual paramfail_t get_param(unsigned int nr, Glib::ustring& v) const;
	using SensorADSB::set_param;
	virtual void set_param(unsigned int nr, const Glib::ustring& v);

  protected:
        enum {
                parnrstart = SensorADSB::parnrend,
		parnrremoteaddr = parnrstart,
		parnrend
	};

        bool close(void);
        bool try_connect(void);
	void connect_finished(Glib::RefPtr<Gio::AsyncResult>& ar);
	void read_finished(Glib::RefPtr<Gio::AsyncResult>& ar);

	std::string m_remoteaddr;
	Glib::RefPtr<Gio::SocketClient> m_sockclient;
	Glib::RefPtr<Gio::SocketConnection> m_sockconn;
	Glib::RefPtr<Gio::Cancellable> m_cancel;
        sigc::connection m_conntimeout;
	std::string m_buffer;
	uint8_t m_readbuffer[256];
};

#endif /* SENSREMOTEADSB_H */
