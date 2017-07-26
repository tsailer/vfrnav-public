//
// C++ Interface: Attitude Sensor: psmove using direct bluetooth sockets
//
// Description: Attitude Sensor Handling
//
//
// Author: Thomas Sailer <t.sailer@alumni.ethz.ch>, (C) 2011, 2012
//
// Copyright: See COPYING file that comes with this distribution
//
//

#ifndef SENSATTPSMOVEBT_H
#define SENSATTPSMOVEBT_H

#include <bluetooth/bluetooth.h>
#include "sensattpsmove.h"

class Sensors::SensorAttitudePSMoveBT : public SensorAttitudePSMove {
  public:
	SensorAttitudePSMoveBT(Sensors& sensors, const Glib::ustring& configgroup);
	virtual ~SensorAttitudePSMoveBT();

	virtual void get_param_desc(unsigned int pagenr, paramdesc_t& pd);
	using SensorAttitude::get_param;
	virtual paramfail_t get_param(unsigned int nr, Glib::ustring& v) const;
	using SensorAttitude::set_param;
	virtual void set_param(unsigned int nr, const Glib::ustring& v);

  protected:
	void close_listen(void);
	void close_dev(void);
	bool is_listen(void) const { return m_cskl != -1 && m_iskl != -1; }
	static std::string bdaddr_str(const bdaddr_t *bdaddr);
	std::string ifaddr_str(void) const;
	std::string devaddr_str(void) const;
        static int l2cap_listen(const bdaddr_t *bdaddr, unsigned short psm);
	bool setup_listen(void);
	void setup_device(int csk, int isk, const bdaddr_t *bdaddr = 0);
        bool handle_listen_read(Glib::IOCondition iocond);
        bool handle_dev_read(Glib::IOCondition iocond);

	static const bdaddr_t bdaddr_any;

	enum {
		parnrstart = SensorAttitudePSMove::parnrend,
		parnrdevaddr = parnrstart,
		parnrifaddr,
		parnrend
	};

       	bdaddr_t m_ifaddr;
	bdaddr_t m_devaddr;
	sigc::connection m_connlisten;
	sigc::connection m_conndev;
	int m_cskl;
	int m_iskl;
	int m_csk;
	int m_isk;
};

#endif /* SENSATTPSMOVEBT_H */
