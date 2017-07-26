//
// C++ Implementation: King GPS Sensor
//
// Description: GPS Sensor Handling
//
//
// Author: Thomas Sailer <t.sailer@alumni.ethz.ch>, (C) 2011, 2012, 2013
//
// Copyright: See COPYING file that comes with this distribution
//
//

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <errno.h>
#include <limits>
#include <cstring>
#include <cstdio>
#include <glibmm/datetime.h>
#include "sensgpskingftdi.h"

Sensors::SensorKingGPSFTDI::SensorKingGPSFTDI(Sensors& sensors, const Glib::ustring& configgroup)
	: SensorKingGPS(sensors, configgroup), m_thread(0), m_baudrate(38400), m_invalidated(true), m_lastinterlock(false),
	  m_ftdi_context(0), m_fstate(fstate_disconnected), m_pstate(pstate_error), m_interlock(true),
	  m_psenscal(0), m_psenspress(~0), m_psenstemp(~0), m_psenspress1(~0), m_psenstemp1(~0)
{
        const Glib::KeyFile& cf(get_sensors().get_configfile());
        if (!cf.has_key(get_configgroup(), "devicedesc"))
		get_sensors().get_configfile().set_string(get_configgroup(), "devicedesc", "");
	m_devicedesc = cf.get_string(get_configgroup(), "devicedesc");
        if (!cf.has_key(get_configgroup(), "deviceserial"))
		get_sensors().get_configfile().set_string(get_configgroup(), "deviceserial", "");
	m_deviceserial = cf.get_string(get_configgroup(), "deviceserial");
        if (!cf.has_key(get_configgroup(), "baudrate"))
		get_sensors().get_configfile().set_integer(get_configgroup(), "baudrate", m_baudrate);
	m_baudrate = cf.get_integer(get_configgroup(), "baudrate");
#if defined(HAVE_FTDI) && !defined(HAVE_FTDID2XX)
	m_ftdi_context = ftdi_new();
	if (!m_ftdi_context) {
		get_sensors().log(Sensors::loglevel_warning, "king gps ftdi: cannot allocate ftdi structure");
		return;
	}
#endif
	m_dispatcher.connect(sigc::mem_fun(*this, &SensorKingGPSFTDI::gps_poll));
	m_thread = Glib::Thread::create(sigc::mem_fun(*this, &SensorKingGPSFTDI::ftdi_thread), true);
	if (!m_thread) {
		get_sensors().log(Sensors::loglevel_warning, "king gps ftdi: cannot create helper thread");
		return;
	}
}

Sensors::SensorKingGPSFTDI::~SensorKingGPSFTDI()
{
	m_conntimeout.disconnect();
	if (m_thread) {
		m_fstate = fstate_terminate;
		m_cond.broadcast();
		m_thread->join();
	}
	m_thread = 0;
#if defined(HAVE_FTDI) && !defined(HAVE_FTDID2XX)
	if (m_ftdi_context)
		ftdi_free(m_ftdi_context);
	m_ftdi_context = 0;
#endif
}

void Sensors::SensorKingGPSFTDI::gps_poll(void)
{
	// log messages
	ParamChanged pc;
	{
		logmsgs_t msgs;
		std::string readdata;
		fstate_t fs;
		pstate_t ps;
		bool interlock;
		uint64_t psenscal = 0;
		uint16_t psenspress = ~0;
		uint16_t psenstemp = ~0;
		{
			Glib::Mutex::Lock lock(m_mutex);
			msgs.swap(m_logmsgs);
			readdata.swap(m_readdata);
			fs = m_fstate;
			ps = m_pstate;
			if (ps == pstate_result) {
				m_pstate = pstate_startconv;
				psenscal = m_psenscal;
				psenspress = m_psenspress;
				psenstemp = m_psenstemp;
			}
			interlock = m_interlock;
		}
		for (logmsgs_t::const_iterator mi(msgs.begin()), me(msgs.end()); mi != me; ++mi)
			get_sensors().log(mi->first, mi->second);
		bool update = parse(readdata);
		while (update) {
			m_invalidated = false;
			update_gps(pc);
			m_conntimeout.disconnect();
			m_conntimeout = Glib::signal_timeout().connect(sigc::mem_fun(*this, &SensorKingGPSFTDI::gps_timeout), 2500);
			update = parse();
		}
		if (fs != fstate_connected)
			m_conntimeout.disconnect();
		else if (m_conntimeout.empty() && !m_invalidated)
			m_conntimeout = Glib::signal_timeout().connect(sigc::mem_fun(*this, &SensorKingGPSFTDI::gps_timeout), 2500);
		if (ps == pstate_result)
			m_psenssig.emit(psenscal, psenspress, psenstemp);
		if (interlock != m_lastinterlock) {
			m_lastinterlock = interlock;
			pc.set_changed(parnrinterlock);
		}
	}
	if (pc)
		update(pc);
}

bool Sensors::SensorKingGPSFTDI::gps_timeout(void)
{
	m_conntimeout.disconnect();
	invalidate_update_gps();
	m_invalidated = true;
        {
                std::ostringstream oss;
                oss << "king gps: timeout from device " << m_devicedesc << ' ' << m_deviceserial;
                get_sensors().log(Sensors::loglevel_warning, oss.str());
        }
	return false;
}

void Sensors::SensorKingGPSFTDI::log(Sensors::loglevel_t lvl, const Glib::ustring& msg)
{
	Glib::Mutex::Lock lock(m_mutex);
	if (m_logmsgs.size() > 128)
		return;
	m_logmsgs.push_back(std::make_pair(lvl, msg));
	m_dispatcher();
}

Sensors::SensorKingGPSFTDI::fstate_t Sensors::SensorKingGPSFTDI::get_fstate(void) const
{
	Glib::Mutex::Lock lock(m_mutex);
	return m_fstate;
}

void Sensors::SensorKingGPSFTDI::set_fstate(fstate_t newstate, bool notify)
{
	Glib::Mutex::Lock lock(m_mutex);
	if (m_fstate == newstate)
		return;
	if (m_fstate == fstate_terminate)
		return;
	if (m_fstate == fstate_connected || newstate != fstate_reconnect)
		m_fstate = newstate;
	if (newstate == fstate_disconnected) {
		m_pstate = pstate_result;
		m_psenscal = 0;
		m_psenspress = ~0;
		m_psenstemp = ~0;
	}
	m_cond.broadcast();
	if (notify)
		m_dispatcher();
}

#if defined(HAVE_FTDID2XX)

namespace
{

std::string to_ftdi_str(FT_STATUS stat)
{
	switch (stat) {
        case FT_OK: return "OK";
        case FT_INVALID_HANDLE: return "INVALID_HANDLE";
        case FT_DEVICE_NOT_FOUND: return "DEVICE_NOT_FOUND";
        case FT_DEVICE_NOT_OPENED: return "DEVICE_NOT_OPENED";
        case FT_IO_ERROR: return "IO_ERROR";
        case FT_INSUFFICIENT_RESOURCES: return "INSUFFICIENT_RESOURCES";
        case FT_INVALID_PARAMETER: return "INVALID_PARAMETER";
        case FT_INVALID_BAUD_RATE: return "INVALID_BAUD_RATE";
        case FT_DEVICE_NOT_OPENED_FOR_ERASE: return "DEVICE_NOT_OPENED_FOR_ERASE";
        case FT_DEVICE_NOT_OPENED_FOR_WRITE: return "DEVICE_NOT_OPENED_FOR_WRITE";
        case FT_FAILED_TO_WRITE_DEVICE: return "FAILED_TO_WRITE_DEVICE";
        case FT_EEPROM_READ_FAILED: return "EEPROM_READ_FAILED";
        case FT_EEPROM_WRITE_FAILED: return "EEPROM_WRITE_FAILED";
        case FT_EEPROM_ERASE_FAILED: return "EEPROM_ERASE_FAILED";
        case FT_EEPROM_NOT_PRESENT: return "EEPROM_NOT_PRESENT";
        case FT_EEPROM_NOT_PROGRAMMED: return "EEPROM_NOT_PROGRAMMED";
        case FT_INVALID_ARGS: return "INVALID_ARGS";
        case FT_NOT_SUPPORTED: return "NOT_SUPPORTED";
        case FT_OTHER_ERROR: return "OTHER_ERROR";
        case FT_DEVICE_LIST_NOT_READY: return "DEVICE_LIST_NOT_READY";
	default:
		break;
	}
	{
		std::ostringstream oss;
		oss << "unknown FTDI error: " << stat;
		return oss.str();
	}
}

}

#if !defined(BITMODE_CBUS)
#define BITMODE_CBUS 0x20
#endif

void Sensors::SensorKingGPSFTDI::ftdi_thread(void)
{
	while (get_fstate() != fstate_terminate) {
		{
			std::string desc, serial;
			{
				Glib::Mutex::Lock lock(m_mutex);
				desc = m_devicedesc;
				serial = m_deviceserial;
				m_interlock = false;
			}
			if (m_ftdi_context)
				FT_Close(m_ftdi_context);
			m_ftdi_context = 0;
			DWORD numDevs;
			FT_STATUS ftStatus = FT_ListDevices(&numDevs, NULL, FT_LIST_NUMBER_ONLY);
			if (FT_SUCCESS(ftStatus)) {
				for (DWORD devIndex = 0; devIndex < numDevs; ++devIndex) {
					char ddesc[64];
					ftStatus = FT_ListDevices((PVOID)devIndex, ddesc, FT_LIST_BY_INDEX|FT_OPEN_BY_DESCRIPTION);
					if (!FT_SUCCESS(ftStatus))
						continue;
					ddesc[sizeof(ddesc)-1] = 0;
					char dserial[64];
					ftStatus = FT_ListDevices((PVOID)devIndex, dserial, FT_LIST_BY_INDEX|FT_OPEN_BY_SERIAL_NUMBER);
					if (!FT_SUCCESS(ftStatus))
						continue;
					dserial[sizeof(dserial)-1] = 0;
					if ((!desc.empty() && desc != ddesc) ||
					    (!serial.empty() && serial != dserial)) {
						ftStatus = FT_DEVICE_NOT_FOUND;
						continue;
					}
					ftStatus = FT_Open(devIndex, &m_ftdi_context);
					if (FT_SUCCESS(ftStatus))
						break;
				}
			}
			if (!FT_SUCCESS(ftStatus) || !m_ftdi_context) {
				std::ostringstream oss;
				oss << "king gps: cannot open ftdi device description " << desc << " serial " << serial
				    << ':' << to_ftdi_str(ftStatus);
				log(Sensors::loglevel_warning, oss.str());
				{
					Glib::Mutex::Lock lock(m_mutex);
					Glib::TimeVal tv;
					tv.assign_current_time();
					tv.add_seconds(15);
					m_cond.timed_wait(m_mutex, tv);
				}
				continue;
			}
		}
		set_fstate(fstate_connected, true);
		ftdi_connected();
		{
			Glib::Mutex::Lock lock(m_mutex);
			m_readdata.clear();
			m_interlock = false;
		}
		FT_Close(m_ftdi_context);
		m_ftdi_context = 0;
		set_fstate(fstate_disconnected, true);
	}
}

void Sensors::SensorKingGPSFTDI::ftdi_connected(void)
{
	{
		Glib::Mutex::Lock lock(m_mutex);
		m_pstate = pstate_error;
		m_interlock = false;
	}
	{
		FT_STATUS ftStatus = FT_SetLatencyTimer(m_ftdi_context, 20);
		if (!FT_SUCCESS(ftStatus)) {
			std::ostringstream oss;
			oss << "king gps: unable to set the latency timer: " << to_ftdi_str(ftStatus);
			log(Sensors::loglevel_warning, oss.str());
		}
	}
	{
		FT_STATUS ftStatus = FT_SetTimeouts(m_ftdi_context, 20, 1000);
		if (!FT_SUCCESS(ftStatus)) {
			std::ostringstream oss;
			oss << "king gps: unable to set timeouts: " << to_ftdi_str(ftStatus);
			log(Sensors::loglevel_warning, oss.str());
		}
	}
	{
		FT_STATUS ftStatus = FT_SetBitMode(m_ftdi_context, psens_dir, BITMODE_CBUS);
		if (!FT_SUCCESS(ftStatus)) {
			std::ostringstream oss;
			oss << "king gps: unable to set bitmode: " << to_ftdi_str(ftStatus);
			log(Sensors::loglevel_warning, oss.str());
		}
	}
	{
		unsigned int baudrate;
		{
			Glib::Mutex::Lock lock(m_mutex);
			baudrate = m_baudrate;
		}
		// HACK: there seems to be an error setting baudrates when bitbang mode is on
	        FT_STATUS ftStatus = FT_SetBaudRate(m_ftdi_context, baudrate / 4);
		if (!FT_SUCCESS(ftStatus)) {
			std::ostringstream oss;
			oss << "king gps: unable to set the baudrate: " << to_ftdi_str(ftStatus);
			log(Sensors::loglevel_warning, oss.str());
		}
	}
	{
		FT_STATUS ftStatus = FT_SetDataCharacteristics(m_ftdi_context, FT_BITS_8, FT_STOP_BITS_1, FT_PARITY_NONE);
		if (!FT_SUCCESS(ftStatus)) {
			std::ostringstream oss;
			oss << "king gps: unable to set the latency timer: " << to_ftdi_str(ftStatus);
			log(Sensors::loglevel_warning, oss.str());
		}
	}
	{
		FT_STATUS ftStatus = FT_SetFlowControl(m_ftdi_context, FT_FLOW_NONE, 0x11, 0x13);
		if (!FT_SUCCESS(ftStatus)) {
			std::ostringstream oss;
			oss << "king gps: unable to disable flow control: " << to_ftdi_str(ftStatus);
			log(Sensors::loglevel_warning, oss.str());
		}
	}
	{
		FT_STATUS ftStatus = FT_SetRts(m_ftdi_context);
		if (!FT_SUCCESS(ftStatus)) {
			std::ostringstream oss;
			oss << "king gps: unable to set RTS state: " << to_ftdi_str(ftStatus);
			log(Sensors::loglevel_warning, oss.str());
		}
	}
	{
		FT_STATUS ftStatus = FT_SetDtr(m_ftdi_context);
		if (!FT_SUCCESS(ftStatus)) {
			std::ostringstream oss;
			oss << "king gps: unable to set DTR state: " << to_ftdi_str(ftStatus);
			log(Sensors::loglevel_warning, oss.str());
		}
	}
	{
		FT_STATUS ftStatus = FT_SetChars(m_ftdi_context, Parser::etx, 1, 0, 0);
		if (!FT_SUCCESS(ftStatus)) {
			std::ostringstream oss;
			oss << "king gps: unable to set the event character: " << to_ftdi_str(ftStatus);
			log(Sensors::loglevel_warning, oss.str());
		}
	}
	{
		uint64_t cal = 0;
		if (psens_readcal(cal))	{
			Glib::Mutex::Lock lock(m_mutex);
			m_pstate = pstate_startconv;
			m_psenscal = cal;
			m_psenspress = m_psenstemp = ~0;
		}
	}
	while (get_fstate() == fstate_connected) {
		{
			char buf[1024];
			DWORD r;
			FT_STATUS ftStatus = FT_Read(m_ftdi_context, buf, sizeof(buf), &r);
			if (!FT_SUCCESS(ftStatus)) {
				std::ostringstream oss;
				oss << "king gps: ftdi read error: " << to_ftdi_str(ftStatus);
				log(Sensors::loglevel_warning, oss.str());
				break;
			}
			if (r > 0) {
				Glib::Mutex::Lock lock(m_mutex);
				m_readdata.insert(m_readdata.end(), buf, buf + r);
				m_dispatcher();
				continue;
			}
		}
		{
			UCHAR pins;
			FT_STATUS ftStatus = FT_GetBitMode(m_ftdi_context, &pins);
			if (!FT_SUCCESS(ftStatus)) {
				std::ostringstream oss;
				oss << "king gps: ftdi CBUS pin read error: " << to_ftdi_str(ftStatus);
				log(Sensors::loglevel_warning, oss.str());
				break;
			}
			bool oldinterlock(m_interlock);
			m_interlock = !!(pins & psens_interlock);
			if (m_interlock != oldinterlock)
				m_dispatcher();
		}
		{
			pstate_t ps;
			{
				Glib::Mutex::Lock lock(m_mutex);
				ps = m_pstate;
				if (ps == pstate_startconv)
					m_pstate = pstate_convpress;
			}
			if (ps == pstate_startconv) {
				if (!psens_startconv(false)) {
					{
						Glib::Mutex::Lock lock(m_mutex);
						m_pstate = pstate_result;
						m_psenspress = m_psenstemp = ~0;
					}
					m_dispatcher();
				}
				m_psenstime.assign_current_time();
				m_psenstime.add_milliseconds(50);
				continue;
			}
			if (ps == pstate_convpress || ps == pstate_convtemp) {
				Glib::TimeVal tv;
				tv.assign_current_time();
				tv -= m_psenstime;
				if (tv.negative())
					continue;
			}
			if (ps == pstate_convpress) {
				if (!psens_readconv(m_psenspress1) ||
				    !psens_startconv(true)) {
					{
						Glib::Mutex::Lock lock(m_mutex);
						m_pstate = pstate_result;
						m_psenspress = m_psenstemp = ~0;
					}
					m_dispatcher();
				} else {
					m_psenstime.assign_current_time();
					m_psenstime.add_milliseconds(50);
					Glib::Mutex::Lock lock(m_mutex);
					m_pstate = pstate_convtemp;
				}
				continue;
			}
			if (ps == pstate_convtemp) {
				if (!psens_readconv(m_psenstemp1)) {
					Glib::Mutex::Lock lock(m_mutex);
					m_pstate = pstate_result;
					m_psenspress = m_psenstemp = ~0;
				} else {
					Glib::Mutex::Lock lock(m_mutex);
					m_pstate = pstate_result;
					m_psenspress = m_psenspress1;
					m_psenstemp = m_psenstemp1;
				}
				m_dispatcher();
			}
		}
	}
	log(Sensors::loglevel_notice, "king gps: closing device");
}

bool Sensors::SensorKingGPSFTDI::psens_readdout(bool& dout)
{
	UCHAR pins;
	FT_STATUS ftStatus = FT_GetBitMode(m_ftdi_context, &pins);
	dout = !!(pins & psens_dout);
	if (FT_SUCCESS(ftStatus))
		return true;
	std::ostringstream oss;
	oss << "king gps: ftdi CBUS pin read error: " << to_ftdi_str(ftStatus);
	log(Sensors::loglevel_warning, oss.str());
	return false;
}

bool Sensors::SensorKingGPSFTDI::psens_tx(unsigned int nrbits, uint32_t bits)
{
	if (nrbits < 32)
		bits &= (1 << nrbits) - 1;
	uint8_t pins = psens_dir;
	if (bits & 1)
		pins |= psens_din;
	{
		FT_STATUS ftStatus = FT_SetBitMode(m_ftdi_context, pins, BITMODE_CBUS);
		if (!FT_SUCCESS(ftStatus)) {
			std::ostringstream oss;
			oss << "king gps: unable to set CBUS: " << to_ftdi_str(ftStatus);
			log(Sensors::loglevel_warning, oss.str());
			return false;
		}
	}
	for (; nrbits > 0; --nrbits) {
		bits >>= 1;
		pins |= psens_sclk;
		{
			FT_STATUS ftStatus = FT_SetBitMode(m_ftdi_context, pins, BITMODE_CBUS);
			if (!FT_SUCCESS(ftStatus)) {
				std::ostringstream oss;
				oss << "king gps: unable to set CBUS: " << to_ftdi_str(ftStatus);
				log(Sensors::loglevel_warning, oss.str());
				return false;
			}
		}
		pins &= ~(psens_sclk | psens_din);
		if (bits & 1)
			pins |= psens_din;
		{
			FT_STATUS ftStatus = FT_SetBitMode(m_ftdi_context, pins, BITMODE_CBUS);
			if (!FT_SUCCESS(ftStatus)) {
				std::ostringstream oss;
				oss << "king gps: unable to set CBUS: " << to_ftdi_str(ftStatus);
				log(Sensors::loglevel_warning, oss.str());
				return false;
			}
		}
	}
	return true;
}

bool Sensors::SensorKingGPSFTDI::psens_rx(unsigned int nrbits, uint32_t& bits)
{
	uint32_t b = 0;
	bits = 0;
	{
		UCHAR pins;
		FT_STATUS ftStatus = FT_GetBitMode(m_ftdi_context, &pins);
		if (!FT_SUCCESS(ftStatus)) {
			std::ostringstream oss;
			oss << "king gps: ftdi CBUS pin read error: " << to_ftdi_str(ftStatus);
			log(Sensors::loglevel_warning, oss.str());
			return false;
		}
		if (pins & psens_dout)
			b = 1;
	}
	for (; nrbits > 0; --nrbits) {
		{
			FT_STATUS ftStatus = FT_SetBitMode(m_ftdi_context, psens_dir | psens_sclk, BITMODE_CBUS);
			if (!FT_SUCCESS(ftStatus)) {
				std::ostringstream oss;
				oss << "king gps: unable to set CBUS: " << to_ftdi_str(ftStatus);
				log(Sensors::loglevel_warning, oss.str());
				return false;
			}
		}
		{
			FT_STATUS ftStatus = FT_SetBitMode(m_ftdi_context, psens_dir, BITMODE_CBUS);
			if (!FT_SUCCESS(ftStatus)) {
				std::ostringstream oss;
				oss << "king gps: unable to set CBUS: " << to_ftdi_str(ftStatus);
				log(Sensors::loglevel_warning, oss.str());
				return false;
			}
		}
		{
			UCHAR pins;
			FT_STATUS ftStatus = FT_GetBitMode(m_ftdi_context, &pins);
			if (!FT_SUCCESS(ftStatus)) {
				std::ostringstream oss;
				oss << "king gps: ftdi CBUS pin read error: " << to_ftdi_str(ftStatus);
				log(Sensors::loglevel_warning, oss.str());
				return false;
			}
			b <<= 1;
			if (pins & psens_dout)
				b |= 1;
		}
	}
	bits = b;
	return true;
}

#elif defined(HAVE_FTDI)

void Sensors::SensorKingGPSFTDI::ftdi_thread(void)
{
	while (get_fstate() != fstate_terminate) {
		{
			std::string desc, serial;
			{
				Glib::Mutex::Lock lock(m_mutex);
				desc = m_devicedesc;
				serial = m_deviceserial;
				m_interlock = false;
			}
			int r = ftdi_usb_open_desc(m_ftdi_context, 0x0403, 0x6001,
						   desc.empty() ? 0 : desc.c_str(),
						   serial.empty() ? 0 : serial.c_str());
			if (r) {
				std::ostringstream oss;
				oss << "king gps: cannot open ftdi device description " << desc << " serial " << serial
				    << ':' << ftdi_get_error_string(m_ftdi_context);
				log(Sensors::loglevel_warning, oss.str());
				{
					Glib::Mutex::Lock lock(m_mutex);
					Glib::TimeVal tv;
					tv.assign_current_time();
					tv.add_seconds(15);
					m_cond.timed_wait(m_mutex, tv);
				}
				continue;
			}
		}
		set_fstate(fstate_connected, true);
		ftdi_connected();
		{
			Glib::Mutex::Lock lock(m_mutex);
			m_readdata.clear();
			m_interlock = false;
		}
		ftdi_usb_close(m_ftdi_context);
		set_fstate(fstate_disconnected, true);
	}
}

void Sensors::SensorKingGPSFTDI::ftdi_connected(void)
{
	{
		Glib::Mutex::Lock lock(m_mutex);
		m_pstate = pstate_error;
		m_interlock = false;
	}
	{
		int r = ftdi_set_latency_timer(m_ftdi_context, 20);
		if (r < 0) {
			std::ostringstream oss;
			oss << "king gps: unable to set the latency timer: " << ftdi_get_error_string(m_ftdi_context);
			log(Sensors::loglevel_warning, oss.str());
		}
	}
	{
		int r = ftdi_set_bitmode(m_ftdi_context, psens_dir, BITMODE_CBUS);
		if (r < 0) {
			std::ostringstream oss;
			oss << "king gps: unable to set bitmode: " << ftdi_get_error_string(m_ftdi_context);
			log(Sensors::loglevel_warning, oss.str());
		}
	}
	{
		unsigned int baudrate;
		{
			Glib::Mutex::Lock lock(m_mutex);
			baudrate = m_baudrate;
		}
		// HACK: there seems to be an error setting baudrates when bitbang mode is on
		int r = ftdi_set_baudrate(m_ftdi_context, baudrate / 4);
		if (r < 0) {
			std::ostringstream oss;
			oss << "king gps: unable to set the baudrate: " << ftdi_get_error_string(m_ftdi_context);
			log(Sensors::loglevel_warning, oss.str());
		}
	}
	{
		int r = ftdi_set_line_property2(m_ftdi_context, BITS_8, STOP_BIT_1, NONE, BREAK_OFF);
		if (r < 0) {
			std::ostringstream oss;
			oss << "king gps: unable to set the latency timer: " << ftdi_get_error_string(m_ftdi_context);
			log(Sensors::loglevel_warning, oss.str());
		}
	}
	{
		int r = ftdi_setflowctrl(m_ftdi_context, SIO_DISABLE_FLOW_CTRL);
		if (r < 0) {
			std::ostringstream oss;
			oss << "king gps: unable to disable flow control: " << ftdi_get_error_string(m_ftdi_context);
			log(Sensors::loglevel_warning, oss.str());
		}
	}
	{
		int r = ftdi_setdtr_rts(m_ftdi_context, 1, 1);
		if (r < 0) {
			std::ostringstream oss;
			oss << "king gps: unable to set RTS/DTS state: " << ftdi_get_error_string(m_ftdi_context);
			log(Sensors::loglevel_warning, oss.str());
		}
	}
	{
		int r = ftdi_set_event_char(m_ftdi_context, Parser::etx, 1);
		if (r < 0) {
			std::ostringstream oss;
			oss << "king gps: unable to set the event character: " << ftdi_get_error_string(m_ftdi_context);
			log(Sensors::loglevel_warning, oss.str());
		}
	}
	{
		uint64_t cal = 0;
		if (psens_readcal(cal))	{
			Glib::Mutex::Lock lock(m_mutex);
			m_pstate = pstate_startconv;
			m_psenscal = cal;
			m_psenspress = m_psenstemp = ~0;
		}
	}
	while (get_fstate() == fstate_connected) {
		{
			char buf[1024];
			int r = ftdi_read_data(m_ftdi_context, (unsigned char *)buf, sizeof(buf));
			if (r < 0) {
				std::ostringstream oss;
				oss << "king gps: ftdi read error: " << ftdi_get_error_string(m_ftdi_context);
				log(Sensors::loglevel_warning, oss.str());
				break;
			}
			if (r > 0) {
				Glib::Mutex::Lock lock(m_mutex);
				m_readdata.insert(m_readdata.end(), buf, buf + r);
				m_dispatcher();
				continue;
			}
		}
		{
			unsigned char pins;
			int r = ftdi_read_pins(m_ftdi_context, &pins);
			if (r < 0) {
				std::ostringstream oss;
				oss << "king gps: ftdi CBUS pin read error: " << ftdi_get_error_string(m_ftdi_context);
				log(Sensors::loglevel_warning, oss.str());
				break;
			}
			bool oldinterlock(m_interlock);
			m_interlock = !!(pins & psens_interlock);
			if (m_interlock != oldinterlock)
				m_dispatcher();
		}
		{
			pstate_t ps;
			{
				Glib::Mutex::Lock lock(m_mutex);
				ps = m_pstate;
				if (ps == pstate_startconv)
					m_pstate = pstate_convpress;
			}
			if (ps == pstate_startconv) {
				if (!psens_startconv(false)) {
					{
						Glib::Mutex::Lock lock(m_mutex);
						m_pstate = pstate_result;
						m_psenspress = m_psenstemp = ~0;
					}
					m_dispatcher();
				}
				m_psenstime.assign_current_time();
				m_psenstime.add_milliseconds(50);
				continue;
			}
			if (ps == pstate_convpress || ps == pstate_convtemp) {
				Glib::TimeVal tv;
				tv.assign_current_time();
				tv -= m_psenstime;
				if (tv.negative())
					continue;
			}
			if (ps == pstate_convpress) {
				if (!psens_readconv(m_psenspress1) ||
				    !psens_startconv(true)) {
					{
						Glib::Mutex::Lock lock(m_mutex);
						m_pstate = pstate_result;
						m_psenspress = m_psenstemp = ~0;
					}
					m_dispatcher();
				} else {
					m_psenstime.assign_current_time();
					m_psenstime.add_milliseconds(50);
					Glib::Mutex::Lock lock(m_mutex);
					m_pstate = pstate_convtemp;
				}
				continue;
			}
			if (ps == pstate_convtemp) {
				if (!psens_readconv(m_psenstemp1)) {
					Glib::Mutex::Lock lock(m_mutex);
					m_pstate = pstate_result;
					m_psenspress = m_psenstemp = ~0;
				} else {
					Glib::Mutex::Lock lock(m_mutex);
					m_pstate = pstate_result;
					m_psenspress = m_psenspress1;
					m_psenstemp = m_psenstemp1;
				}
				m_dispatcher();
			}
		}
	}
	log(Sensors::loglevel_notice, "king gps: closing device");
}

bool Sensors::SensorKingGPSFTDI::psens_readdout(bool& dout)
{
	unsigned char pins;
	int r = ftdi_read_pins(m_ftdi_context, &pins);
	dout = !!(pins & psens_dout);
	if (!r)
		return true;
	std::ostringstream oss;
	oss << "king gps: ftdi CBUS pin read error: " << ftdi_get_error_string(m_ftdi_context);
	log(Sensors::loglevel_warning, oss.str());
	return false;
}

bool Sensors::SensorKingGPSFTDI::psens_tx(unsigned int nrbits, uint32_t bits)
{
	if (nrbits < 32)
		bits &= (1 << nrbits) - 1;
	uint8_t pins = psens_dir;
	if (bits & 1)
		pins |= psens_din;
	{
		int r = ftdi_set_bitmode(m_ftdi_context, pins, BITMODE_CBUS);
		if (r < 0) {
			std::ostringstream oss;
			oss << "king gps: unable to set CBUS: " << ftdi_get_error_string(m_ftdi_context);
			log(Sensors::loglevel_warning, oss.str());
			return false;
		}
	}
	for (; nrbits > 0; --nrbits) {
		bits >>= 1;
		pins |= psens_sclk;
		{
			int r = ftdi_set_bitmode(m_ftdi_context, pins, BITMODE_CBUS);
			if (r < 0) {
				std::ostringstream oss;
				oss << "king gps: unable to set CBUS: " << ftdi_get_error_string(m_ftdi_context);
				log(Sensors::loglevel_warning, oss.str());
				return false;
			}
		}
		pins &= ~(psens_sclk | psens_din);
		if (bits & 1)
			pins |= psens_din;
		{
			int r = ftdi_set_bitmode(m_ftdi_context, pins, BITMODE_CBUS);
			if (r < 0) {
				std::ostringstream oss;
				oss << "king gps: unable to set CBUS: " << ftdi_get_error_string(m_ftdi_context);
				log(Sensors::loglevel_warning, oss.str());
				return false;
			}
		}
	}
	return true;
}

bool Sensors::SensorKingGPSFTDI::psens_rx(unsigned int nrbits, uint32_t& bits)
{
	uint32_t b = 0;
	bits = 0;
	{
		uint8_t pins;
		int r = ftdi_read_pins(m_ftdi_context, &pins);
		if (r < 0) {
			std::ostringstream oss;
			oss << "king gps: ftdi CBUS pin read error: " << ftdi_get_error_string(m_ftdi_context);
			log(Sensors::loglevel_warning, oss.str());
			return false;
		}
		if (pins & psens_dout)
			b = 1;
	}
	for (; nrbits > 0; --nrbits) {
		{
			int r = ftdi_set_bitmode(m_ftdi_context, psens_dir | psens_sclk, BITMODE_CBUS);
			if (r < 0) {
				std::ostringstream oss;
				oss << "king gps: unable to set CBUS: " << ftdi_get_error_string(m_ftdi_context);
				log(Sensors::loglevel_warning, oss.str());
				return false;
			}
		}
		{
			int r = ftdi_set_bitmode(m_ftdi_context, psens_dir, BITMODE_CBUS);
			if (r < 0) {
				std::ostringstream oss;
				oss << "king gps: unable to set CBUS: " << ftdi_get_error_string(m_ftdi_context);
				log(Sensors::loglevel_warning, oss.str());
				return false;
			}
		}
		{
			uint8_t pins;
			int r = ftdi_read_pins(m_ftdi_context, &pins);
			if (r < 0) {
				std::ostringstream oss;
				oss << "king gps: ftdi CBUS pin read error: " << ftdi_get_error_string(m_ftdi_context);
				log(Sensors::loglevel_warning, oss.str());
				return false;
			}
			b <<= 1;
			if (pins & psens_dout)
				b |= 1;
		}
	}
	bits = b;
	return true;
}

#endif

bool Sensors::SensorKingGPSFTDI::psens_reset(void)
{
	return psens_tx(21, 0x5555);
}

bool Sensors::SensorKingGPSFTDI::psens_readcal(uint64_t& cal)
{
	static const uint16_t cfgreadcmds[4] = {
		0x157, // Word 1
		0x0D7, // Word 2
		0x137, // Word 3
		0x0B7  // Word 4
	};
	cal = 0;
	uint64_t calword = 0;
	for (unsigned int i = 0; i < 4; ++i) {
		if (!psens_reset())
			return false;
		if (!psens_tx(12, cfgreadcmds[i]))
			return false;
		uint32_t rx(0);
		if (!psens_rx(18, rx))
			return false;
		if ((rx & 0x60001) != (0x20001 | ((i & 1) << 18))) {
			std::ostringstream oss;
			oss << "king gps: MS5534 calibration framing error: read 0x" << std::hex << rx << " expected 0x" << (0x20001 | ((i & 1) << 18));
			log(Sensors::loglevel_warning, oss.str());
			return false;
		}
		calword <<= 16;
		calword |= (rx >> 1) & 0xffff;
	}
	cal = calword;
	return true;
}

bool Sensors::SensorKingGPSFTDI::psens_startconv(bool temp)
{
	if (!psens_reset())
		return false;
	if (!psens_tx(10, temp ? 0x4F : 0x2F))
		return false;
	uint32_t rx = 0;
	if (!psens_rx(2, rx))
		return false;
	if (rx == (temp ? 0x3 : 0x7))
		return true;
	std::ostringstream oss;
	oss << "king gps: MS5534 start conversion framing error: read 0x" << std::hex << rx << " expected 0x" << (temp ? 0x3 : 0x7);
	log(Sensors::loglevel_warning, oss.str());
	return false;
}

bool Sensors::SensorKingGPSFTDI::psens_readconv(uint16_t& val)
{
	uint32_t rx = 0;
	if (!psens_rx(17, rx))
		return false;
	if ((rx & 0x20001) != 0x00001) {
		std::ostringstream oss;
		oss << "king gps: MS5534 conversion result framing error: read 0x" << std::hex << rx << " expected 0x1";
		log(Sensors::loglevel_warning, oss.str());
		return false;
	}
	val = rx >> 1;
	return true;
}

void Sensors::SensorKingGPSFTDI::get_param_desc(unsigned int pagenr, paramdesc_t& pd)
{
	SensorKingGPS::get_param_desc(pagenr, pd);
	{
		paramdesc_t::iterator pdi(pd.begin()), pde(pd.end());
		if (pdi != pde)
			++pdi;
		for (; pdi != pde; ++pdi)
			if (pdi->get_type() == ParamDesc::type_section)
				break;
		pdi = pd.insert(pdi, ParamDesc(ParamDesc::type_string, ParamDesc::editable_admin, parnrdevicedesc, "Description", "Device Description to match (or empty to match all)", ""));
		++pdi;
		pdi = pd.insert(pdi, ParamDesc(ParamDesc::type_string, ParamDesc::editable_admin, parnrdeviceserial, "Serial", "Device Serial Number to match (or empty to match all)", ""));
		++pdi;
		pdi = pd.insert(pdi, ParamDesc(ParamDesc::type_integer, ParamDesc::editable_admin, parnrbaudrate, "Baud Rate", "Baud Rate", "", 0, 1200, 230400, 1, 10));
	}

	pd.push_back(ParamDesc(ParamDesc::type_section, ParamDesc::editable_readonly, ~0U, "Transmitter", "Transmitter", ""));
	if (false)
		pd.push_back(ParamDesc(ParamDesc::type_switch, ParamDesc::editable_readonly, parnrinterlock, "Transmit", "Any Radio Transmits (Interlock line)", ""));
	else
		pd.push_back(ParamDesc(ParamDesc::type_integer, ParamDesc::editable_readonly, parnrinterlock, "Transmit", "Any Radio Transmits (Interlock line)", "", 0, 0, 1, 1, 1));
}

Sensors::SensorKingGPSFTDI::paramfail_t Sensors::SensorKingGPSFTDI::get_param(unsigned int nr, Glib::ustring& v) const
{
	switch (nr) {
	default:
		break;

	case parnrdevicedesc:
		v = m_devicedesc;
		return paramfail_ok;

	case parnrdeviceserial:
		v = m_deviceserial;
		return paramfail_ok;
	}
	return SensorKingGPS::get_param(nr, v);
}

Sensors::SensorKingGPSFTDI::paramfail_t Sensors::SensorKingGPSFTDI::get_param(unsigned int nr, double& v) const
{
	switch (nr) {
	default:
		break;
	}
	return SensorKingGPS::get_param(nr, v);
}

Sensors::SensorKingGPSFTDI::paramfail_t Sensors::SensorKingGPSFTDI::get_param(unsigned int nr, int& v) const
{
	switch (nr) {
	default:
		break;

	case parnrbaudrate:
		v = m_baudrate;
		return paramfail_ok;

	case parnrinterlock:
	{
		fstate_t fs;
		{
			Glib::Mutex::Lock lock(m_mutex);
		        fs = m_fstate;
			v = m_interlock;
		}
		return (fs == fstate_connected) ? paramfail_ok : paramfail_fail;
	}
	}
	return SensorKingGPS::get_param(nr, v);
}

void Sensors::SensorKingGPSFTDI::set_param(unsigned int nr, const Glib::ustring& v)
{
	switch (nr) {
	default:
		SensorKingGPS::set_param(nr, v);
		return;

	case parnrdevicedesc:
	{
		{
			{
				Glib::Mutex::Lock lock(m_mutex);
				if (v == m_devicedesc)
					return;
				m_devicedesc = v;
			}
			set_fstate(fstate_reconnect);
		}
		get_sensors().get_configfile().set_string(get_configgroup(), "devicedesc", v);
		break;
	}

	case parnrdeviceserial:
	{
		{
			{
				Glib::Mutex::Lock lock(m_mutex);
				if (v == m_deviceserial)
					return;
				m_deviceserial = v;
			}
			set_fstate(fstate_reconnect);
		}
		get_sensors().get_configfile().set_string(get_configgroup(), "deviceserial", v);
		break;
	}
	}
	ParamChanged pc;
	pc.set_changed(nr);
	update(pc);
}

void Sensors::SensorKingGPSFTDI::set_param(unsigned int nr, int v)
{
	switch (nr) {
	default:
		SensorKingGPS::set_param(nr, v);
		return;

	case parnrbaudrate:
		{
			{
				Glib::Mutex::Lock lock(m_mutex);
				if ((unsigned int)v == m_baudrate)
					return;
				m_baudrate = v;
			}
			set_fstate(fstate_reconnect);
		}
		get_sensors().get_configfile().set_integer(get_configgroup(), "baudrate", v);
		break;
	}
	ParamChanged pc;
	pc.set_changed(nr);
	update(pc);
}
