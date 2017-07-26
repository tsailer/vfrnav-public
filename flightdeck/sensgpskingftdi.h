//
// C++ Interface: King GPS Sensor
//
// Description: GPS Sensor Handling
//
//
// Author: Thomas Sailer <t.sailer@alumni.ethz.ch>, (C) 2011, 2012, 2013
//
// Copyright: See COPYING file that comes with this distribution
//
//

#ifndef SENSGPSKINGFTDI_H
#define SENSGPSKINGFTDI_H

#include "sysdeps.h"

#include "sensgpsking.h"

#if defined(HAVE_FTDID2XX)
#include <windows.h>
#include <ftdi/ftd2xx.h>
#elif defined(HAVE_FTDI)
#include <ftdi.h>
#endif

class Sensors::SensorKingGPSFTDI : public Sensors::SensorKingGPS {
  public:
	SensorKingGPSFTDI(Sensors& sensors, const Glib::ustring& configgroup);
	virtual ~SensorKingGPSFTDI();

	virtual void get_param_desc(unsigned int pagenr, paramdesc_t& pd);
	using SensorKingGPS::get_param;
	virtual paramfail_t get_param(unsigned int nr, Glib::ustring& v) const;
	virtual paramfail_t get_param(unsigned int nr, double& v) const;
	virtual paramfail_t get_param(unsigned int nr, int& v) const;
	using SensorKingGPS::set_param;
	virtual void set_param(unsigned int nr, const Glib::ustring& v);
	virtual void set_param(unsigned int nr, int v);

	sigc::signal<void,uint64_t,uint16_t,uint16_t>& signal_ms5534(void) { return m_psenssig; }

	Glib::ustring get_devicedesc(void) const { return m_devicedesc; }
	Glib::ustring get_deviceserial(void) const { return m_deviceserial; }

  protected:
        enum {
                parnrstart = SensorKingGPS::parnrend,
		parnrdevicedesc = parnrstart,
		parnrdeviceserial,
		parnrbaudrate,
		parnrinterlock,
		parnrend
	};

	typedef enum {
		fstate_disconnected,
		fstate_connected,
		fstate_reconnect,
		fstate_terminate
	} fstate_t;

	typedef enum {
		pstate_error,
		pstate_startconv,
		pstate_convpress,
		pstate_convtemp,
		pstate_result
	} pstate_t;

	enum {
		psens_dout      = 1 << 0,
		psens_din       = 1 << 1,
		psens_sclk      = 1 << 2,
		psens_interlock = 1 << 3,
		psens_dir  = (psens_din | psens_sclk) << 4
	};

	void gps_poll(void);
	bool gps_timeout(void);

	// thread routines
	void ftdi_thread(void);
	void ftdi_connected(void);
	void log(Sensors::loglevel_t lvl, const Glib::ustring& msg);
	bool psens_readdout(bool& dout);
	bool psens_tx(unsigned int nrbits, uint32_t bits);
	bool psens_rx(unsigned int nrbits, uint32_t& bits);
	bool psens_reset(void);
	bool psens_readcal(uint64_t& cal);
	bool psens_startconv(bool temp);
	bool psens_readconv(uint16_t& val);


	fstate_t get_fstate(void) const;
	void set_fstate(fstate_t newstate, bool notify = false);

	Glib::Thread *m_thread;
	mutable Glib::Mutex m_mutex;
	Glib::Cond m_cond;
	Glib::Dispatcher m_dispatcher;
	Glib::ustring m_devicedesc;
	Glib::ustring m_deviceserial;
	sigc::connection m_conntimeout;
	unsigned int m_baudrate;
	bool m_invalidated;
	bool m_lastinterlock;
	// thread state
#if defined(HAVE_FTDID2XX)
	FT_HANDLE m_ftdi_context;
#elif defined(HAVE_FTDI)
	struct ftdi_context *m_ftdi_context;
#endif
	fstate_t m_fstate;
	pstate_t m_pstate;
	bool m_interlock;
	typedef std::vector<std::pair<Sensors::loglevel_t,Glib::ustring> > logmsgs_t;
	logmsgs_t m_logmsgs;
	std::string m_readdata;
	Glib::TimeVal m_psenstime;
	sigc::signal<void,uint64_t,uint16_t,uint16_t> m_psenssig;
	uint64_t m_psenscal;
	uint16_t m_psenspress;
	uint16_t m_psenstemp;
	uint16_t m_psenspress1;
	uint16_t m_psenstemp1;
};

#endif /* SENSGPSKINGFTDI_H */
