//
// C++ Interface: King GPS Sensor
//
// Description: GPS Sensor Handling
//
//
// Author: Thomas Sailer <t.sailer@alumni.ethz.ch>, (C) 2011, 2012
//
// Copyright: See COPYING file that comes with this distribution
//
//

#ifndef SENSGPSKINGTTY_H
#define SENSGPSKINGTTY_H

#include "sysdeps.h"

#ifdef __MINGW32__
#include <windows.h>
#endif

#include "sensgpsking.h"

class Sensors::SensorKingGPSTTY : public Sensors::SensorKingGPS {
  public:
	SensorKingGPSTTY(Sensors& sensors, const Glib::ustring& configgroup);
	virtual ~SensorKingGPSTTY();

	virtual void get_param_desc(unsigned int pagenr, paramdesc_t& pd);
	using SensorKingGPS::get_param;
	virtual paramfail_t get_param(unsigned int nr, Glib::ustring& v) const;
	virtual paramfail_t get_param(unsigned int nr, double& v) const;
	virtual paramfail_t get_param(unsigned int nr, int& v) const;
	using SensorKingGPS::set_param;
	virtual void set_param(unsigned int nr, const Glib::ustring& v);
	virtual void set_param(unsigned int nr, int v);

  protected:
#ifdef __MINGW32__
	static const struct baudrates {
		unsigned int baud;
		DWORD tbaud;
	} baudrates[8];
#else
	static const struct baudrates {
		unsigned int baud;
		unsigned int tbaud;
	} baudrates[8];
#endif

        enum {
                parnrstart = SensorKingGPS::parnrend,
		parnrdevice = parnrstart,
		parnrbaudrate,
		parnrend
	};

	bool close(void);
	bool try_connect(void);
	bool gps_poll(Glib::IOCondition iocond);
	bool gps_timeout(void);

	Glib::ustring m_device;
	sigc::connection m_conn;
	sigc::connection m_conntimeout;
#ifdef __MINGW32__
	HANDLE m_fd;
#else
	int m_fd;
#endif
	unsigned int m_baudrate;
};

#endif /* SENSGPSKINGTTY_H */
