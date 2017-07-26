//
// C++ Interface: Realtek ADS-B Receiver
//
// Description: Realtek ADS-B Receiver
//
//
// Author: Thomas Sailer <t.sailer@alumni.ethz.ch>, (C) 2013
//
// Copyright: See COPYING file that comes with this distribution
//
//

#ifndef SENSRTLADSB_H
#define SENSRTLADSB_H

#include "sensadsb.h"

class Sensors::SensorRTLADSB : public Sensors::SensorADSB {
  public:
	SensorRTLADSB(Sensors& sensors, const Glib::ustring& configgroup);
	virtual ~SensorRTLADSB();

	virtual void get_param_desc(unsigned int pagenr, paramdesc_t& pd);
	using SensorADSB::get_param;
	virtual paramfail_t get_param(unsigned int nr, Glib::ustring& v) const;
	using SensorADSB::set_param;
	virtual void set_param(unsigned int nr, const Glib::ustring& v);

  protected:
        enum {
                parnrstart = SensorADSB::parnrend,
		parnrrxbinary = parnrstart,
		parnrrxargs,
		parnrend
	};

        bool close(void);
        bool try_connect(void);
        void child_watch(GPid pid, int child_status);
        bool child_stdout_handler(Glib::IOCondition iocond);

	std::string m_rxbinary;
	std::string m_rxargs;
        sigc::connection m_connchildtimeout;
        sigc::connection m_connchildwatch;
        sigc::connection m_connchildstdout;
        Glib::RefPtr<Glib::IOChannel> m_childchanstdout;
        Glib::Pid m_childpid;
        bool m_childrun;
};

#endif /* SENSRTLADSB_H */
