//
// C++ Implementation: Attitude Sensor: raw STM Sensor Hub driver
//
// Description: Attitude Sensor Handling
//
//
// Author: Thomas Sailer <t.sailer@alumni.ethz.ch>, (C) 2012, 2013
//
// Copyright: See COPYING file that comes with this distribution
//
//


#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#define _GNU_SOURCE 1

#include "sysdeps.h"

#include <iostream>
#include <iomanip>
#include <libudev.h>
#include <sys/poll.h>

#include "sensattstmhub.h"

const uint16_t Sensors::SensorAttitudeSTMSensorHub::stmsensorhub_vid;
const uint16_t Sensors::SensorAttitudeSTMSensorHub::stmsensorhub_pid;

Sensors::SensorAttitudeSTMSensorHub::SensorAttitudeSTMSensorHub(Sensors& sensors, const Glib::ustring& configgroup)
	: SensorAttitude(sensors, configgroup), m_udev(0), m_udevmon(0), m_thread(0), m_tstate(tstate_none),
	  m_context(0), m_devh(0), m_kerneldrv(false), m_ahrs(50, "", AHRS::init_standard), 
	  m_pitch(std::numeric_limits<double>::quiet_NaN()), m_bank(std::numeric_limits<double>::quiet_NaN()),
	  m_slip(std::numeric_limits<double>::quiet_NaN()), m_rate(std::numeric_limits<double>::quiet_NaN()),
	  m_hdg(std::numeric_limits<double>::quiet_NaN()), m_dirty(false), m_update(false), m_samplecount(0),
	  m_attitudeprio(0), m_headingprio(0)
{
        // get configuration
	const Glib::KeyFile& cf(get_sensors().get_configfile());
	if (!cf.has_key(get_configgroup(), "attitudepriority"))
		get_sensors().get_configfile().set_uint64(get_configgroup(), "attitudepriority", m_attitudeprio);
	m_attitudeprio = cf.get_uint64(get_configgroup(), "attitudepriority");
	if (!cf.has_key(get_configgroup(), "headingpriority"))
		get_sensors().get_configfile().set_uint64(get_configgroup(), "headingpriority", m_headingprio);
	m_headingprio = cf.get_uint64(get_configgroup(), "headingpriority");
	m_rawgyro[0] = m_rawgyro[1] = m_rawgyro[2] = m_rawaccel[0] = m_rawaccel[1] = m_rawaccel[2] = m_rawmag[0] = m_rawmag[1] = m_rawmag[2] = 0;
	// dispatcher
	m_dispatcher.connect(sigc::mem_fun(*this, &SensorAttitudeSTMSensorHub::usb_thread_poll));
	// initialize udev
	m_udev = udev_new();
	if (!m_udev)
		throw std::runtime_error("PSMove: unable to create udev");
	m_udevmon = udev_monitor_new_from_netlink(m_udev, "udev");
	if (!m_udevmon)
		throw std::runtime_error("PSMove: unable to create udev monitor");
	if (udev_monitor_filter_add_match_subsystem_devtype(m_udevmon, "usb", NULL))
		throw std::runtime_error("PSMove: udev monitor: error adding filter");
	if (udev_monitor_enable_receiving(m_udevmon))
		throw std::runtime_error("PSMove: udev monitor: cannot start receiving");
	m_conndevarrival = Glib::signal_io().connect(sigc::mem_fun(*this, &SensorAttitudeSTMSensorHub::handle_connect_cb),
						     udev_monitor_get_fd(m_udevmon), Glib::IO_IN | Glib::IO_HUP);
	handle_initial_connect();
	//try_connect();
}

Sensors::SensorAttitudeSTMSensorHub::~SensorAttitudeSTMSensorHub()
{
	// prevent calling the update event handler, which would call into get_params
	close_internal();
	m_conndevarrival.disconnect();
	if (m_udevmon)
		udev_monitor_unref(m_udevmon);
	m_udevmon = 0;
	if (m_udev)
		udev_unref(m_udev);
	m_udev = 0;
}


bool Sensors::SensorAttitudeSTMSensorHub::handle_connect_cb(Glib::IOCondition iocond)
{
	struct udev_device *udev = udev_monitor_receive_device(m_udevmon);
	if (!udev)
		return true;
	// check for action
	bool ok(true);
	{
		const char *cp(udev_device_get_action(udev));
		if (cp && strcmp(cp, "add"))
			ok = false;
	}
	if (!ok) {
		udev_device_unref(udev);
		return true;
	}
	handle_connect(udev);
	return true;
}

bool Sensors::SensorAttitudeSTMSensorHub::handle_initial_connect(void)
{
	if (is_open())
		return false;
	struct udev_enumerate *enumerate = udev_enumerate_new(m_udev);
	udev_enumerate_add_match_subsystem(enumerate, "usb");
	udev_enumerate_scan_devices(enumerate);
	struct udev_list_entry *devices = udev_enumerate_get_list_entry(enumerate);
	struct udev_list_entry *dev_list_entry;
	udev_list_entry_foreach(dev_list_entry, devices) {
		const char *path = udev_list_entry_get_name(dev_list_entry);
		struct udev_device *dev = udev_device_new_from_syspath(m_udev, path);
		handle_connect(dev);
	}
	/* Free the enumerator object */
	udev_enumerate_unref(enumerate);
	return false;
}

void Sensors::SensorAttitudeSTMSensorHub::handle_connect(struct udev_device *udev)
{
	if (!udev)
		return;
	{
		const char *subsys = udev_device_get_subsystem(udev);
		if (g_strcmp0(subsys, "usb") != 0) {
			udev_device_unref(udev);
			return;
		}
	}
	const char *attrvid(udev_device_get_sysattr_value(udev, "idVendor"));
	const char *attrpid(udev_device_get_sysattr_value(udev, "idProduct"));
	uint16_t vid(0), pid(0);
	if (attrvid) {
		char *cp(0);
		vid = strtoul(attrvid, &cp, 16);
		if (cp == attrvid || *cp)
			vid = 0;
	}
	if (attrpid) {
		char *cp(0);
		pid = strtoul(attrpid, &cp, 16);
		if (cp == attrpid || *cp)
			pid = 0;
	}
	if (vid != stmsensorhub_vid || pid != stmsensorhub_pid) {
		udev_device_unref(udev);
		return;
	}
	if (!m_devh)
		try_connect();
	udev_device_unref(udev);
}

bool Sensors::SensorAttitudeSTMSensorHub::is_open(void) const
{
	bool x;
	{
		Glib::Mutex::Lock lock(m_mutex);
		//statechange();
		x = m_tstate != tstate_none;
	}
	return x;
}

bool Sensors::SensorAttitudeSTMSensorHub::is_open(void)
{
	bool x;
	{
		Glib::Mutex::Lock lock(m_mutex);
		statechange();
		x = m_tstate != tstate_none;
	}
	return x;
}

bool Sensors::SensorAttitudeSTMSensorHub::close_internal(void)
{
	bool wasopen(false);
	{
		Glib::Mutex::Lock lock(m_mutex);
		switch (m_tstate) {
		case tstate_initializing:
		case tstate_initahrs:
			m_tstate = tstate_terminating_nosave;
			m_cond.broadcast();
			break;

		case tstate_running:
			m_tstate = tstate_terminating;
			m_cond.broadcast();
			wasopen = true;
			break;

		default:
			break;
		}
		for (;;) {
			statechange();
			if (m_tstate == tstate_none)
				break;
			m_cond.wait(m_mutex);
		}
	}
	m_ahrs.set_id("");
	m_attitudetime = Glib::TimeVal();
	m_attitudereporttime = Glib::TimeVal();
	m_pitch = m_bank = m_slip = m_rate = m_hdg = std::numeric_limits<double>::quiet_NaN();
	m_dirty = false;
	m_update = false;
	m_samplecount = 0;
	m_rawgyro[0] = m_rawgyro[1] = m_rawgyro[2] = m_rawaccel[0] = m_rawaccel[1] = m_rawaccel[2] = m_rawmag[0] = m_rawmag[1] = m_rawmag[2] = 0;
	for (unsigned int i = 0; i < 3; ++i) {
		m_devname[i].clear();
		m_devmfg[i].clear();
	}
	return wasopen;
}

bool Sensors::SensorAttitudeSTMSensorHub::close(void)
{
	if (close_internal()) {
		ParamChanged pc;
		pc.set_changed(parnracceldevname, parnrmagdevmfg);
		pc.set_changed(parnrmode, parnrgyro);
		update(pc);
		return true;
	}
	return false;
}

bool Sensors::SensorAttitudeSTMSensorHub::try_connect(void)
{
	if (close()) {
                ParamChanged pc;
		pc.set_changed(parnracceldevname, parnrmagdevmfg);
		pc.set_changed(parnrmode, parnrgyro);
                update(pc);
        }
	m_tstate = tstate_initializing;
        m_thread = Glib::Thread::create(sigc::mem_fun(*this, &SensorAttitudeSTMSensorHub::usb_thread), true);
	if (!m_thread) {
		get_sensors().log(Sensors::loglevel_warning, "stmsensorhub: cannot create helper thread");
		return false;
	}
	return false;
}

void Sensors::SensorAttitudeSTMSensorHub::libusb_perror(int err, const std::string& txt)
{
	if (err == LIBUSB_SUCCESS)
		return;
	std::ostringstream oss;
	oss << "stmsensorhub: ";
	switch (err) {
	case LIBUSB_ERROR_IO:
		oss << "I/O Error";
		break;

	case LIBUSB_ERROR_INVALID_PARAM:
		oss << "Invalid Parameter Error";
		break;

	case LIBUSB_ERROR_ACCESS:
		oss << "Access Error";
		break;

	case LIBUSB_ERROR_NO_DEVICE:
		oss << "No Device Error";
		break;

	case LIBUSB_ERROR_NOT_FOUND:
		oss << "Not Found";
		break;

	case LIBUSB_ERROR_BUSY:
		oss << "Busy";
		break;

	case LIBUSB_ERROR_TIMEOUT:
		oss << "Timeout";
		break;

	case LIBUSB_ERROR_OVERFLOW:
		oss << "Overflow";
		break;

	case LIBUSB_ERROR_PIPE:
		oss << "Pipe Error";
		break;

	case LIBUSB_ERROR_INTERRUPTED:
		oss << "Interrupted";
		break;

	case LIBUSB_ERROR_NO_MEM:
		oss << "No Memory";
		break;

	case LIBUSB_ERROR_NOT_SUPPORTED:
		oss << "Unsupported";
		break;

	case LIBUSB_ERROR_OTHER:
		oss << "Other Error";
		break;

	default:
		oss << "Unknown Error " << err;
		break;
	}
	oss << ": " << txt;
	log(Sensors::loglevel_warning, oss.str());
	if (true)
		std::cerr << oss.str() << std::endl;
}

std::vector<uint8_t> Sensors::SensorAttitudeSTMSensorHub::get_feature(uint8_t id)
{
	uint8_t buf[0x38];
	int r(libusb_control_transfer(m_devh, LIBUSB_REQUEST_TYPE_CLASS | LIBUSB_RECIPIENT_INTERFACE | 0x80,
				      0x01, 0x300 | id, 0, buf, sizeof(buf), 1000));
	if (r >= 0)
		return std::vector<uint8_t>(&buf[0], &buf[r]);
	libusb_perror(r, "libusb_control_transfer");
	return std::vector<uint8_t>();
}

void Sensors::SensorAttitudeSTMSensorHub::set_feature(const std::vector<uint8_t>& buf)
{
	if (buf.empty())
		return;
	int r(libusb_control_transfer(m_devh, LIBUSB_REQUEST_TYPE_CLASS | LIBUSB_RECIPIENT_INTERFACE,
				      0x09, 0x300 | buf[0], 0, const_cast<uint8_t *>(&buf[0]), buf.size(), 1000));
	if (r < 0)
		libusb_perror(r, "libusb_control_transfer");
}

Glib::ustring Sensors::SensorAttitudeSTMSensorHub::wstr2ustr(const std::wstring& ws)
{
        size_t len = wcstombs(0, ws.c_str(), 0);
        if (len == (size_t)-1)
                return Glib::ustring();
        char buf[len + 1];
        len = wcstombs(buf, ws.c_str(), len + 1);
        return Glib::ustring(buf, len);
}

void Sensors::SensorAttitudeSTMSensorHub::usb_thread_init(void)
{
	{
		Glib::Mutex::Lock lock(m_mutex);
		m_tstate = tstate_initializing;
		m_cond.broadcast();
		for (unsigned int i = 0; i < 3; ++i) {
			m_devname[i].clear();
			m_devmfg[i].clear();
		}
		m_dirty = false;
		m_update = false;
	}
	// initialize libusb1
        libusb_perror(libusb_init(&m_context), "libusb_init");
	if (!m_context)
		goto err;
	m_devh = libusb_open_device_with_vid_pid(m_context, stmsensorhub_vid, stmsensorhub_pid);
	if (!m_devh)
		goto err;
	{
		int r(libusb_kernel_driver_active(m_devh, 0));
		m_kerneldrv = r == 1;
		if (!m_kerneldrv)
			libusb_perror(r, "libusb_kernel_driver_active");
	}
	if (m_kerneldrv)
		libusb_perror(libusb_detach_kernel_driver(m_devh, 0), "libusb_detach_kernel_driver");
	{
		int r(libusb_claim_interface(m_devh, 0));
		libusb_perror(r, "libusb_claim_interface");
		if (r) {
			if (true)
				std::cerr << "stmsensorhub: cannot claim interface" << std::endl;
			goto err;
		}
	}
	// feature reports
	{
		static const char * const sensorname[3] = { "accel", "gyro", "mag" };
		std::vector<uint8_t> ftr[3];
		for (unsigned int i = 0; i < 3; ++i) {
			ftr[i] = get_feature(0x02 + i);
			if (ftr[i].size() < 0x30 || ftr[i][0] != 0x02 + i) {
				std::ostringstream oss;
				oss << "stmsensorhub: unable to retrieve " << sensorname[i] << " feature report";
				get_sensors().log(Sensors::loglevel_warning, oss.str());
				goto err;
			}
			ftr[i][2] = 0x02;
			ftr[i][3] = 0x02;
			ftr[i][6] = 100;
			ftr[i][7] = 0;
			ftr[i][8] = 0;
			ftr[i][9] = 0;
			set_feature(ftr[i]);
			ftr[i] = get_feature(0x02 + i);
			if (ftr[i].size() < 0x30 || ftr[i][0] != 0x02 + i) {
				std::ostringstream oss;
				oss << "stmsensorhub: unable to retrieve " << sensorname[i] << " modified feature report";
				get_sensors().log(Sensors::loglevel_warning, oss.str());
				goto err;
			}
			{
				std::wstring ws;
				for (unsigned int j = 0x10; j < 0x18 && j + 1 < ftr[i].size(); j += 2) {
					wchar_t ch((ftr[i][j + 1] << 8) | ftr[i][j]);
					if (!ch)
						break;
					ws.push_back(ch);
				}
				std::string devmfg(wstr2ustr(ws));
				ws.clear();
				for (unsigned int j = 0x18; j < 0x30 && j + 1 < ftr[i].size(); j += 2) {
					wchar_t ch((ftr[i][j + 1] << 8) | ftr[i][j]);
					if (!ch)
						break;
					ws.push_back(ch);
				}
				std::string devname(wstr2ustr(ws));
				Glib::Mutex::Lock lock(m_mutex);
				m_devmfg[i] = devmfg;
				m_devname[i] = devname;
			}
			if (true) {
				std::ostringstream oss;
				oss << "stmsensorhub: " << sensorname[i]
				    << " report 0x" << std::hex << std::setfill('0') << std::setw(2) << (unsigned int)ftr[i][2]
				    << " power 0x" << std::setfill('0') << std::setw(2) << (unsigned int)ftr[i][3]
				    << " min interval " << std::dec << (unsigned int)ftr[i][5]
				    << " interval " << (ftr[i][6] | (ftr[i][7] << 8) | (ftr[i][8] << 16) | (ftr[i][9] << 24))
				    << " mfg " << m_devmfg[i] << " dev " << m_devname[i];
				get_sensors().log(Sensors::loglevel_notice, oss.str());
			}
		}
		{
			Glib::Mutex::Lock lock(m_mutex);
			m_ahrs.set_sampletime(1e-3 * (ftr[1][6] | (ftr[1][7] << 8) | (ftr[1][8] << 16) | (ftr[1][9] << 24)));
			m_ahrs.set_id(m_devname[1]);
		}
	}
	{
		Glib::Mutex::Lock lock(m_mutex);
		m_tstate = tstate_initahrs;
		m_cond.broadcast();
	}
	m_dispatcher();
	return;

  err:
	{
		Glib::Mutex::Lock lock(m_mutex);
		m_tstate = tstate_terminating_nosave;
		m_cond.broadcast();
	}
	return;
}

void Sensors::SensorAttitudeSTMSensorHub::usb_thread(void)
{
	usb_thread_init();
	for (;;) {
		{
			tstate_t ts;
			{
				Glib::Mutex::Lock lock(m_mutex);
				ts = m_tstate;
				if (ts == tstate_initahrs) {
					m_cond.wait(m_mutex);
					ts = m_tstate;
				}
			}
			if (ts == tstate_initahrs)
				continue;
			if (ts != tstate_running)
				break;
		}
		uint8_t buf[64];
		int actlen(0);
		int err(libusb_interrupt_transfer(m_devh, 0x81, buf, sizeof(buf), &actlen, 1000));
		if (err == LIBUSB_ERROR_TIMEOUT)
			continue;
		if (err) {
			libusb_perror(err, "libusb_interrupt_transfer");
			{
				Glib::Mutex::Lock lock(m_mutex);
				m_tstate = tstate_terminating;
				m_cond.broadcast();
			}
			break;
		}
		if (actlen >= 0x1e) {
			switch (buf[0]) {
			case 0x02: // acceleration
			{
				static const double scale = 1e-3;
				{
					Glib::Mutex::Lock lock(m_mutex);
					m_rawaccel[1] = (buf[3] | (buf[4] << 8) | (buf[5] << 16) | (buf[6] << 24)) * scale;
					m_rawaccel[0] = (buf[7] | (buf[8] << 8) | (buf[9] << 16) | (buf[10] << 24)) * scale;
					m_rawaccel[2] = -(buf[11] | (buf[12] << 8) | (buf[13] << 16) | (buf[14] << 24)) * scale;
					m_ahrs.new_accel_samples(m_rawaccel);
				}
				break;
			}

			case 0x03: // gyro
			{
				static const double scale = 1e-3;
				{
					Glib::Mutex::Lock lock(m_mutex);
					m_rawgyro[1] = (buf[3] | (buf[4] << 8) | (buf[5] << 16) | (buf[6] << 24)) * scale;
					m_rawgyro[0] = (buf[7] | (buf[8] << 8) | (buf[9] << 16) | (buf[10] << 24)) * scale;
					m_rawgyro[2] = -(buf[11] | (buf[12] << 8) | (buf[13] << 16) | (buf[14] << 24)) * scale;
					m_ahrs.new_gyro_samples(1, m_rawgyro);
					++m_samplecount;
				}
				if (true) {
					std::cout << "Accel " << std::setw(6) << m_rawaccel[0]
						  << ' ' << std::setw(6) << m_rawaccel[1]
						  << ' ' << std::setw(6) << m_rawaccel[2]
						  << " Gyro " << std::setw(6) << m_rawgyro[0]
						  << ' ' << std::setw(6) << m_rawgyro[1]
						  << ' ' << std::setw(6) << m_rawgyro[2]
						  << " Mag " << std::setw(6) << m_rawmag[0]
						  << ' ' << std::setw(6) << m_rawmag[1]
						  << ' ' << std::setw(6) << m_rawmag[2]
						  << " scnt " << m_samplecount << std::endl;
				}
				m_attitudetime.assign_current_time();
				m_dirty = true;
				{
					Glib::TimeVal tv(m_attitudetime);
					tv.subtract(m_attitudereporttime);
					tv.subtract_milliseconds(100);
					if (!m_attitudereporttime.valid() || !tv.negative()) {
						m_update = true;
						m_attitudereporttime = m_attitudetime;
						m_dispatcher();
					}
				}
				break;
			}

			case 0x04: // mag
			{
				static const double scale = 1e-3;
				{
					Glib::Mutex::Lock lock(m_mutex);
					m_rawmag[1] = (buf[3] | (buf[4] << 8) | (buf[5] << 16) | (buf[6] << 24)) * scale;
					m_rawmag[0] = (buf[7] | (buf[8] << 8) | (buf[9] << 16) | (buf[10] << 24)) * scale;
					m_rawmag[2] = -(buf[11] | (buf[12] << 8) | (buf[13] << 16) | (buf[14] << 24)) * scale;
					m_ahrs.new_mag_samples(m_rawmag);
				}
				break;
			}

			default:
				break;
			}
		}
	}
	if (m_devh) {
		libusb_perror(libusb_release_interface(m_devh, 0), "libusb_release_interface");
		if (m_kerneldrv)
			libusb_perror(libusb_attach_kernel_driver(m_devh, 0), "libusb_attach_kernel_driver");
		libusb_close(m_devh);
	}
	m_devh = 0;
	m_kerneldrv = false;
	if (m_context)
		libusb_exit(m_context);
	m_context = 0;
	{
		Glib::Mutex::Lock lock(m_mutex);
		if (m_tstate == tstate_terminating)
			m_tstate = tstate_stopped;
		else
			m_tstate = tstate_stopped_nosave;
		m_cond.broadcast();
	}
	m_dispatcher();
}

void Sensors::SensorAttitudeSTMSensorHub::usb_thread_poll(void)
{
	logmsgs_t msgs;
	bool upd;
	{
		Glib::Mutex::Lock lock(m_mutex);
		statechange();
		msgs.swap(m_logmsgs);
		upd = m_update;
		m_update = false;
	}
	for (logmsgs_t::const_iterator mi(msgs.begin()), me(msgs.end()); mi != me; ++mi)
		get_sensors().log(mi->first, mi->second);
	if (upd) {
		ParamChanged pc;
		pc.set_changed(parnrattituderoll, parnrattitudeheading);
		pc.set_changed(parnrrawgyrox, parnrattitudequatw);
		pc.set_changed(parnrcalgyrobiasx, parnrcalgyrobiasz);
		pc.set_changed(parnrcalaccelscale2, parnrgyro);
		pc.set_changed(parnrrawsamplemagx, parnrrawsamplemagz);
		update(pc);
	}
}

void Sensors::SensorAttitudeSTMSensorHub::log(Sensors::loglevel_t lvl, const Glib::ustring& msg)
{
	Glib::Mutex::Lock lock(m_mutex);
	if (m_logmsgs.size() > 128)
		return;
	m_logmsgs.push_back(std::make_pair(lvl, msg));
	m_dispatcher();
}

void Sensors::SensorAttitudeSTMSensorHub::statechange(void)
{
	switch (m_tstate) {
	case tstate_initahrs:
		if (!m_ahrs.init(AHRS::init_standard))
			m_ahrs.load(get_sensors().get_configfile(), get_configgroup());
		m_tstate = tstate_running;
                m_cond.broadcast();
		break;

	case tstate_stopped:
		m_ahrs.save(get_sensors().get_configfile(), get_configgroup());
		m_ahrs.save();
		m_tstate = tstate_stopped_nosave;

		// fall through
	case tstate_stopped_nosave:
		m_ahrs.set_id("");
		m_attitudetime = Glib::TimeVal();
		m_attitudereporttime = Glib::TimeVal();
		m_pitch = m_bank = m_slip = m_rate = m_hdg = std::numeric_limits<double>::quiet_NaN();
		m_dirty = false;
		m_update = false;
		m_samplecount = 0;
		m_rawgyro[0] = m_rawgyro[1] = m_rawgyro[2] = m_rawaccel[0] = m_rawaccel[1] = m_rawaccel[2] = m_rawmag[0] = m_rawmag[1] = m_rawmag[2] = 0;
		for (unsigned int i = 0; i < 3; ++i) {
			m_devname[i].clear();
			m_devmfg[i].clear();
		}
		m_thread->join();
		m_thread = 0;
		m_tstate = tstate_none;
                m_cond.broadcast();
		break;

	default:
		break;
	}
}

void Sensors::SensorAttitudeSTMSensorHub::update_values(void) const
{
	Glib::Mutex::Lock lock(m_mutex);
	//statechange();
	if (!m_dirty)
		return;
	m_dirty = false;
	m_attitudereporttime = m_attitudetime;
	Eigen::Vector3d bodye1(m_ahrs.e1_to_body());
	Eigen::Vector3d bodye3(m_ahrs.e3_to_body());
	if (false)
		std::cerr << "bodye1 " << bodye1.transpose() << " bodye3 " << bodye3.transpose() << std::endl;
	m_pitch = (180.0 / M_PI) * atan2(bodye3[0], bodye3[2]);
	m_bank = (-180.0 / M_PI) * atan2(bodye3[1], bodye3[2]);
	m_hdg = (-180.0 / M_PI) * atan2(bodye1[1], bodye1[0]) + m_ahrs.get_magdeviation();
	Eigen::Vector3d filtaccel(m_ahrs.get_filtered_accelerometer());
	Eigen::Vector3d filtgyro(m_ahrs.get_filtered_gyroscope());
	m_rate = filtgyro.z() * (180.0 / M_PI / 3.0);
	m_slip = (180.0 / M_PI) * atan2(filtaccel.z(), filtaccel.y()) - 90.0;
}
	
void Sensors::SensorAttitudeSTMSensorHub::get_heading(double& hdg) const
{
	update_values();
	double h;
	{
		Glib::Mutex::Lock lock(m_mutex);
		h = m_hdg;
	}
	hdg = h;
}

unsigned int Sensors::SensorAttitudeSTMSensorHub::get_heading_priority(void) const
{
	return m_headingprio;
}

Glib::TimeVal Sensors::SensorAttitudeSTMSensorHub::get_heading_time(void) const
{
	Glib::TimeVal t;
	{
		Glib::Mutex::Lock lock(m_mutex);
		t = m_attitudetime;
	}
	return t;
}

bool Sensors::SensorAttitudeSTMSensorHub::is_heading_true(void) const
{
	return false;
}

bool Sensors::SensorAttitudeSTMSensorHub::is_heading_ok(const Glib::TimeVal& tv) const
{
	return is_open() && SensorAttitude::is_heading_ok(tv);
}

void Sensors::SensorAttitudeSTMSensorHub::get_attitude(double& pitch, double& bank, double& slip, double& rate) const
{
	update_values();
	double p, b, s, r;
	{
		Glib::Mutex::Lock lock(m_mutex);
		p = m_pitch;
		b = m_bank;
		s = m_slip;
		r = m_rate;
	}
	pitch = p;
	bank = b;
	slip = s;
	rate = r;
}

unsigned int Sensors::SensorAttitudeSTMSensorHub::get_attitude_priority(void) const
{
	return m_attitudeprio;
}

Glib::TimeVal Sensors::SensorAttitudeSTMSensorHub::get_attitude_time(void) const
{
	Glib::TimeVal t;
	{
		Glib::Mutex::Lock lock(m_mutex);
		t = m_attitudetime;
	}
	return t;
}

bool Sensors::SensorAttitudeSTMSensorHub::is_attitude_ok(const Glib::TimeVal& tv) const
{
	return is_open() && SensorAttitude::is_attitude_ok(tv);
}

const AHRS::mode_t  Sensors::SensorAttitudeSTMSensorHub::ahrsmodemap[4] = {
	AHRS::mode_slow, AHRS::mode_normal, AHRS::mode_fast, AHRS::mode_gyroonly
};

void Sensors::SensorAttitudeSTMSensorHub::get_param_desc(unsigned int pagenr, paramdesc_t& pd)
{
	SensorAttitude::get_param_desc(pagenr, pd);

	switch (pagenr) {
	default:
		{
			paramdesc_t::iterator pdi(pd.begin()), pde(pd.end());
			if (pdi != pde)
				++pdi;
			for (; pdi != pde; ++pdi)
				if (pdi->get_type() == ParamDesc::type_section)
					break;
			pdi = pd.insert(pdi, ParamDesc(ParamDesc::type_string, ParamDesc::editable_readonly, parnracceldevname, "Accel Device Name", "Acceleration Sensor Device Name", ""));
			++pdi;
			pdi = pd.insert(pdi, ParamDesc(ParamDesc::type_string, ParamDesc::editable_readonly, parnracceldevmfg, "Accel Manufacturer", "Acceleration Sensor Device Manufacturer", ""));
			++pdi;
			pdi = pd.insert(pdi, ParamDesc(ParamDesc::type_string, ParamDesc::editable_readonly, parnrgyrodevname, "Gyro Device Name", "Gyro Sensor Device Name", ""));
			++pdi;
			pdi = pd.insert(pdi, ParamDesc(ParamDesc::type_string, ParamDesc::editable_readonly, parnrgyrodevmfg, "Gyro Manufacturer", "Gyro Sensor Device Manufacturer", ""));
			++pdi;
			pdi = pd.insert(pdi, ParamDesc(ParamDesc::type_string, ParamDesc::editable_readonly, parnrmagdevname, "Mag Device Name", "Magnetic Field Sensor Device Name", ""));
			++pdi;
			pdi = pd.insert(pdi, ParamDesc(ParamDesc::type_string, ParamDesc::editable_readonly, parnrmagdevmfg, "Mag Manufacturer", "Magnetic Field Sensor Device Manufacturer", ""));
		}
		pd.push_back(ParamDesc(ParamDesc::type_integer, ParamDesc::editable_admin, parnrattitudeprio, "Attitude Priority", "Attitude Priority; higher values mean this sensor is preferred to other sensors delivering attitude information", "", 0, 0, 9999, 1, 10));
		pd.push_back(ParamDesc(ParamDesc::type_integer, ParamDesc::editable_admin, parnrheadingprio, "Heading Priority", "Heading Priority; higher values mean this sensor is preferred to other sensors delivering heading information", "", 0, 0, 9999, 1, 10));
		pd.push_back(ParamDesc(ParamDesc::type_section, ParamDesc::editable_readonly, ~0U, "Parameters", "AHRS Parameters", ""));
		{
			ParamDesc::choices_t ch;
			for (int i = 0; i < (int)(sizeof(ahrsmodemap)/sizeof(ahrsmodemap[0])); ++i)
				ch.push_back(AHRS::to_str(ahrsmodemap[i]));
			pd.push_back(ParamDesc(ParamDesc::type_choice, ParamDesc::editable_user, parnrmode, "Mode", "Controls the amount of coupling of the gyro to the accelerometer and magnetometer", "", ch.begin(), ch.end()));
		}
		pd.push_back(ParamDesc(ParamDesc::type_double, ParamDesc::editable_admin, parnrcoeffla, "la", "", "", 3, 0.0, 99.999, 0.01, 0.1));
		pd.push_back(ParamDesc(ParamDesc::type_double, ParamDesc::editable_admin, parnrcoefflc, "lc", "", "", 3, 0.0, 99.999, 0.01, 0.1));
		pd.push_back(ParamDesc(ParamDesc::type_double, ParamDesc::editable_admin, parnrcoeffld, "ld", "", "", 3, 0.0, 99.999, 0.01, 0.1));
		pd.push_back(ParamDesc(ParamDesc::type_double, ParamDesc::editable_admin, parnrcoeffsigma, "sigma", "", "", 3, 0.0, 99.999, 0.01, 0.1));
		pd.push_back(ParamDesc(ParamDesc::type_double, ParamDesc::editable_admin, parnrcoeffn, "n", "", "", 3, 0.0, 99.999, 0.01, 0.1));
		pd.push_back(ParamDesc(ParamDesc::type_double, ParamDesc::editable_admin, parnrcoeffo, "o", "", "", 3, 0.0, 99.999, 0.01, 0.1));

		pd.push_back(ParamDesc(ParamDesc::type_section, ParamDesc::editable_readonly, ~0U, "Attitude", "Aircraft Attitude", ""));
		pd.push_back(ParamDesc(ParamDesc::type_double, ParamDesc::editable_readonly, parnrattituderoll, "Roll", "Roll Angle", "°", 1, -180.0, 360.0, 0.1, 1.0));
		pd.push_back(ParamDesc(ParamDesc::type_double, ParamDesc::editable_readonly, parnrattitudepitch, "Pitch", "Pitch Angle", "°", 1, -180.0, 360.0, 0.1, 1.0));
		pd.push_back(ParamDesc(ParamDesc::type_double, ParamDesc::editable_readonly, parnrattitudeheading, "Heading", "Heading Angle", "°", 1, 0.0, 360.0, 0.1, 1.0));

		pd.push_back(ParamDesc(ParamDesc::type_section, ParamDesc::editable_readonly, ~0U, "Orientation", "Sensor Orientation", ""));
		{
			ParamDesc::choices_t ch;
			ch.push_back("$gtk-go-back");
			ch.push_back("$gtk-go-forward");
			pd.push_back(ParamDesc(ParamDesc::type_double, ParamDesc::editable_readonly, parnrorientationroll, "Roll", "Roll Orientation Angle", "°", 1, -180.0, 360.0, 0.1, 1.0));
			pd.push_back(ParamDesc(ParamDesc::type_button, ParamDesc::editable_admin, parnrorientcoarseadjustroll, "Coarse Adj.", "Coarse Adjust Roll Angle", "", ch.begin(), ch.end()));
			pd.push_back(ParamDesc(ParamDesc::type_button, ParamDesc::editable_admin, parnrorientfineadjustroll, "Fine Adj.", "Fine Adjust Roll Angle", "", ch.begin(), ch.end()));
			pd.push_back(ParamDesc(ParamDesc::type_double, ParamDesc::editable_readonly, parnrorientationpitch, "Pitch", "Pitch Orientation Angle", "°", 1, -180.0, 360.0, 0.1, 1.0));
			pd.push_back(ParamDesc(ParamDesc::type_button, ParamDesc::editable_admin, parnrorientcoarseadjustpitch, "Coarse Adj.", "Coarse Adjust Pitch Angle", "", ch.begin(), ch.end()));
			pd.push_back(ParamDesc(ParamDesc::type_button, ParamDesc::editable_admin, parnrorientfineadjustpitch, "Fine Adj.", "Fine Adjust Pitch Angle", "", ch.begin(), ch.end()));
			pd.push_back(ParamDesc(ParamDesc::type_double, ParamDesc::editable_readonly, parnrorientationheading, "Heading", "Heading Orientation Angle", "°", 1, -180.0, 360.0, 0.1, 1.0));
			pd.push_back(ParamDesc(ParamDesc::type_button, ParamDesc::editable_admin, parnrorientcoarseadjustheading, "Coarse Adj.", "Coarse Adjust Heading Angle", "", ch.begin(), ch.end()));
			pd.push_back(ParamDesc(ParamDesc::type_button, ParamDesc::editable_admin, parnrorientfineadjustheading, "Fine Adj.", "Fine Adjust Heading Angle", "", ch.begin(), ch.end()));
			pd.push_back(ParamDesc(ParamDesc::type_double, ParamDesc::editable_admin, parnrorientationdeviation, "Deviation", "Deviation Angle", "°", 1, -180.0, 540.0, 0.1, 1.0));
		}
		{
			ParamDesc::choices_t ch;
			ch.push_back("Erect");
			ch.push_back("Default");
			pd.push_back(ParamDesc(ParamDesc::type_button, ParamDesc::editable_admin, parnrorienterectdefault, "Quick Adjust", "Quick Adjust of Sensor Orientation", "", ch.begin(), ch.end()));
		}

		pd.push_back(ParamDesc(ParamDesc::type_section, ParamDesc::editable_readonly, ~0U, "Attitudes", "Attitude Quaternions", ""));
		pd.push_back(ParamDesc(ParamDesc::type_double, ParamDesc::editable_readonly, parnrattitudequatx, "Attitude X", "Attitude Quaternion X Value", "", 4, -1.0, 1.0, 0.01, 0.1));
		pd.push_back(ParamDesc(ParamDesc::type_double, ParamDesc::editable_readonly, parnrattitudequaty, "Attitude Y", "Attitude Quaternion Y Value", "", 4, -1.0, 1.0, 0.01, 0.1));
		pd.push_back(ParamDesc(ParamDesc::type_double, ParamDesc::editable_readonly, parnrattitudequatz, "Attitude Z", "Attitude Quaternion Z Value", "", 4, -1.0, 1.0, 0.01, 0.1));
		pd.push_back(ParamDesc(ParamDesc::type_double, ParamDesc::editable_readonly, parnrattitudequatw, "Attitude W", "Attitude Quaternion W Value", "", 4, -1.0, 1.0, 0.01, 0.1));
		pd.push_back(ParamDesc(ParamDesc::type_double, ParamDesc::editable_admin, parnrorientationquatx, "Orientation X", "Sensor Orientation Quaternion X Value", "", 4, -1.0, 1.0, 0.01, 0.1));
		pd.push_back(ParamDesc(ParamDesc::type_double, ParamDesc::editable_admin, parnrorientationquaty, "Orientation Y", "Sensor Orientation Quaternion Y Value", "", 4, -1.0, 1.0, 0.01, 0.1));
		pd.push_back(ParamDesc(ParamDesc::type_double, ParamDesc::editable_admin, parnrorientationquatz, "Orientation Z", "Sensor Orientation Quaternion Z Value", "", 4, -1.0, 1.0, 0.01, 0.1));
		pd.push_back(ParamDesc(ParamDesc::type_double, ParamDesc::editable_admin, parnrorientationquatw, "Orientation W", "Sensor Orientation Quaternion W Value", "", 4, -1.0, 1.0, 0.01, 0.1));
		break;

	case 1:
		pd.push_back(ParamDesc(ParamDesc::type_section, ParamDesc::editable_readonly, ~0U, "Raw Values", "Raw Sensor Values", ""));
		pd.push_back(ParamDesc(ParamDesc::type_double, ParamDesc::editable_readonly, parnrrawgyrox, "Gyro X", "Gyro Sensor X Value", "rad/s", 3, -100.0, 100.0, 0.1, 1.0));
		pd.push_back(ParamDesc(ParamDesc::type_double, ParamDesc::editable_readonly, parnrrawgyroy, "Gyro Y", "Gyro Sensor Y Value", "rad/s", 3, -100.0, 100.0, 0.1, 1.0));
		pd.push_back(ParamDesc(ParamDesc::type_double, ParamDesc::editable_readonly, parnrrawgyroz, "Gyro Z", "Gyro Sensor Z Value", "rad/s", 3, -100.0, 100.0, 0.1, 1.0));
		pd.push_back(ParamDesc(ParamDesc::type_double, ParamDesc::editable_readonly, parnrrawaccelx, "Accel X", "Accelerometer Sensor X Value", "g", 3, -100.0, 100.0, 0.1, 1.0));
		pd.push_back(ParamDesc(ParamDesc::type_double, ParamDesc::editable_readonly, parnrrawaccely, "Accel Y", "Accelerometer Sensor Y Value", "g", 3, -100.0, 100.0, 0.1, 1.0));
		pd.push_back(ParamDesc(ParamDesc::type_double, ParamDesc::editable_readonly, parnrrawaccelz, "Accel Z", "Accelerometer Sensor Z Value", "g", 3, -100.0, 100.0, 0.1, 1.0));
		pd.push_back(ParamDesc(ParamDesc::type_double, ParamDesc::editable_readonly, parnrrawmagx, "Mag X", "Magnetometer Sensor X Value", "B", 3, -100.0, 100.0, 0.1, 1.0));
		pd.push_back(ParamDesc(ParamDesc::type_double, ParamDesc::editable_readonly, parnrrawmagy, "Mag Y", "Magnetometer Sensor Y Value", "B", 3, -100.0, 100.0, 0.1, 1.0));
		pd.push_back(ParamDesc(ParamDesc::type_double, ParamDesc::editable_readonly, parnrrawmagz, "Mag Z", "Magnetometer Sensor Z Value", "B", 3, -100.0, 100.0, 0.1, 1.0));

		pd.push_back(ParamDesc(ParamDesc::type_section, ParamDesc::editable_readonly, ~0U, "Gyro Calibration", "Gyro Calibration Values", ""));
		pd.push_back(ParamDesc(ParamDesc::type_double, ParamDesc::editable_readonly, parnrcalgyrobiasx, "Gyro Bias X", "Gyro Sensor Bias (Offset) X Value", "", 6, -10000.0, 10000.0, 0.1, 1.0));
		pd.push_back(ParamDesc(ParamDesc::type_double, ParamDesc::editable_readonly, parnrcalgyrobiasy, "Gyro Bias Y", "Gyro Sensor Bias (Offset) Y Value", "", 6, -10000.0, 10000.0, 0.1, 1.0));
		pd.push_back(ParamDesc(ParamDesc::type_double, ParamDesc::editable_readonly, parnrcalgyrobiasz, "Gyro Bias Z", "Gyro Sensor Bias (Offset) Z Value", "", 6, -10000.0, 10000.0, 0.1, 1.0));
		pd.push_back(ParamDesc(ParamDesc::type_double, ParamDesc::editable_admin, parnrcalgyroscalex, "Gyro Scale X", "Gyro Sensor Scaling X Value", "", 6, -10000.0, 10000.0, 0.1, 1.0));
		pd.push_back(ParamDesc(ParamDesc::type_double, ParamDesc::editable_admin, parnrcalgyroscaley, "Gyro Scale Y", "Gyro Sensor Scaling Y Value", "", 6, -10000.0, 10000.0, 0.1, 1.0));
		pd.push_back(ParamDesc(ParamDesc::type_double, ParamDesc::editable_admin, parnrcalgyroscalez, "Gyro Scale Z", "Gyro Sensor Scaling Z Value", "", 6, -10000.0, 10000.0, 0.1, 1.0));

		pd.push_back(ParamDesc(ParamDesc::type_section, ParamDesc::editable_readonly, ~0U, "Accelerometer Calibration", "Accelerometer Calibration Values", ""));
		pd.push_back(ParamDesc(ParamDesc::type_double, ParamDesc::editable_admin, parnrcalaccelbiasx, "Accel Bias X", "Accelerometer Sensor Bias (Offset) X Value", "", 6, -10000.0, 10000.0, 0.1, 1.0));
		pd.push_back(ParamDesc(ParamDesc::type_double, ParamDesc::editable_admin, parnrcalaccelbiasy, "Accel Bias Y", "Accelerometer Sensor Bias (Offset) Y Value", "", 6, -10000.0, 10000.0, 0.1, 1.0));
		pd.push_back(ParamDesc(ParamDesc::type_double, ParamDesc::editable_admin, parnrcalaccelbiasz, "Accel Bias Z", "Accelerometer Sensor Bias (Offset) Z Value", "", 6, -10000.0, 10000.0, 0.1, 1.0));
		pd.push_back(ParamDesc(ParamDesc::type_double, ParamDesc::editable_admin, parnrcalaccelscalex, "Accel Scale X", "Accelerometer Sensor Scaling X Value", "", 6, -10000.0, 10000.0, 0.1, 1.0));
		pd.push_back(ParamDesc(ParamDesc::type_double, ParamDesc::editable_admin, parnrcalaccelscaley, "Accel Scale Y", "Accelerometer Sensor Scaling Y Value", "", 6, -10000.0, 10000.0, 0.1, 1.0));
		pd.push_back(ParamDesc(ParamDesc::type_double, ParamDesc::editable_admin, parnrcalaccelscalez, "Accel Scale Z", "Accelerometer Sensor Scaling Z Value", "", 6, -10000.0, 10000.0, 0.1, 1.0));
		pd.push_back(ParamDesc(ParamDesc::type_double, ParamDesc::editable_readonly, parnrcalaccelscale2, "Accel Scale 2", "Accelerometer Sensor Scalar Scaling", "", 6, -10000.0, 10000.0, 0.1, 1.0));

		pd.push_back(ParamDesc(ParamDesc::type_section, ParamDesc::editable_readonly, ~0U, "Magnetometer Calibration", "Magnetometer Calibration Values", ""));
		pd.push_back(ParamDesc(ParamDesc::type_double, ParamDesc::editable_admin, parnrcalmagbiasx, "Mag Bias X", "Magnetometer Sensor Bias (Offset) X Value", "", 6, -10000.0, 10000.0, 0.1, 1.0));
		pd.push_back(ParamDesc(ParamDesc::type_double, ParamDesc::editable_admin, parnrcalmagbiasy, "Mag Bias Y", "Magnetometer Sensor Bias (Offset) Y Value", "", 6, -10000.0, 10000.0, 0.1, 1.0));
		pd.push_back(ParamDesc(ParamDesc::type_double, ParamDesc::editable_admin, parnrcalmagbiasz, "Mag Bias Z", "Magnetometer Sensor Bias (Offset) Z Value", "", 6, -10000.0, 10000.0, 0.1, 1.0));
		pd.push_back(ParamDesc(ParamDesc::type_double, ParamDesc::editable_admin, parnrcalmagscalex, "Mag Scale X", "Magnetometer Sensor Scaling X Value", "", 6, -10000.0, 10000.0, 0.1, 1.0));
		pd.push_back(ParamDesc(ParamDesc::type_double, ParamDesc::editable_admin, parnrcalmagscaley, "Mag Scale Y", "Magnetometer Sensor Scaling Y Value", "", 6, -10000.0, 10000.0, 0.1, 1.0));
		pd.push_back(ParamDesc(ParamDesc::type_double, ParamDesc::editable_admin, parnrcalmagscalez, "Mag Scale Z", "Magnetometer Sensor Scaling Z Value", "", 6, -10000.0, 10000.0, 0.1, 1.0));
		pd.push_back(ParamDesc(ParamDesc::type_double, ParamDesc::editable_readonly, parnrcalmagscale2, "Mag Scale 2", "Magnetometer Sensor Scalar Scaling", "", 6, -10000.0, 10000.0, 0.1, 1.0));
		{
			static const Glib::ustring bname("$gtk-preferences");
			pd.push_back(ParamDesc(ParamDesc::type_magcalib, ParamDesc::editable_admin, parnrrawsamplemag, "Magnetometer Calib", "Opens a Dialog to Calibrate the Magnetometer; requires rotating the Magnetometer around all axes", "", &bname, &bname + 1));
		}
		break;
	}
	pd.push_back(ParamDesc(ParamDesc::type_gyro, ParamDesc::editable_readonly, parnrgyro, ""));
}

Sensors::SensorAttitudeSTMSensorHub::paramfail_t Sensors::SensorAttitudeSTMSensorHub::get_param(unsigned int nr, Glib::ustring& v) const
{
	switch (nr) {
	default:
		break;

	case parnracceldevname:
	{
		Glib::ustring s;
		paramfail_t p;
		{
			Glib::Mutex::Lock lock(m_mutex);
			//statechange();
			s = m_devname[0];
			p = (m_tstate == tstate_running) ? paramfail_ok : paramfail_fail;
		}
		v = s;
		return p;
	}

	case parnracceldevmfg:
	{
		Glib::ustring s;
		paramfail_t p;
		{
			Glib::Mutex::Lock lock(m_mutex);
			//statechange();
			s = m_devmfg[0];
			p = (m_tstate == tstate_running) ? paramfail_ok : paramfail_fail;
		}
		v = s;
		return p;
	}

	case parnrgyrodevname:
	{
		Glib::ustring s;
		paramfail_t p;
		{
			Glib::Mutex::Lock lock(m_mutex);
			//statechange();
			s = m_devname[1];
			p = (m_tstate == tstate_running) ? paramfail_ok : paramfail_fail;
		}
		v = s;
		return p;
	}

	case parnrgyrodevmfg:
	{
		Glib::ustring s;
		paramfail_t p;
		{
			Glib::Mutex::Lock lock(m_mutex);
			//statechange();
			s = m_devmfg[1];
			p = (m_tstate == tstate_running) ? paramfail_ok : paramfail_fail;
		}
		v = s;
		return p;
	}

	case parnrmagdevname:
	{
		Glib::ustring s;
		paramfail_t p;
		{
			Glib::Mutex::Lock lock(m_mutex);
			//statechange();
			s = m_devname[2];
			p = (m_tstate == tstate_running) ? paramfail_ok : paramfail_fail;
		}
		v = s;
		return p;
	}

	case parnrmagdevmfg:
	{
		Glib::ustring s;
		paramfail_t p;
		{
			Glib::Mutex::Lock lock(m_mutex);
			//statechange();
			s = m_devmfg[2];
			p = (m_tstate == tstate_running) ? paramfail_ok : paramfail_fail;
		}
		v = s;
		return p;
	}
	}
	return SensorAttitude::get_param(nr, v);
}

Sensors::SensorAttitudeSTMSensorHub::paramfail_t Sensors::SensorAttitudeSTMSensorHub::get_param(unsigned int nr, int& v) const
{
	switch (nr) {
	default:
		break;

	case parnrmode:
	{
		AHRS::mode_t m;
		paramfail_t p;
		{
			Glib::Mutex::Lock lock(m_mutex);
			//statechange();
			m = m_ahrs.get_mode();
			p = (m_tstate == tstate_running) ? paramfail_ok : paramfail_fail;
		}
		v = 1;
		for (int i = 0; i < (int)(sizeof(ahrsmodemap)/sizeof(ahrsmodemap[0])); ++i) {
			if (ahrsmodemap[i] != m)
				continue;
			v = i;
			break;
		}
		return p;
	}

	case parnrattitudeprio:
		v = m_attitudeprio;
		return paramfail_ok;

	case parnrheadingprio:
		v = m_headingprio;
		return paramfail_ok;
		
	case parnrrawsamplemagx:
	case parnrrawsamplemagy:
	case parnrrawsamplemagz:
	{
		int x;
		paramfail_t p;
		{
			Glib::Mutex::Lock lock(m_mutex);
			//statechange();
			x = m_rawmag[nr - parnrrawsamplemag];
			p = (m_tstate == tstate_running) ? paramfail_ok : paramfail_fail;
		}
		v = x;
		return p;
	}

	case parnrorientcoarseadjustroll:
	case parnrorientfineadjustroll:
	case parnrorientcoarseadjustpitch:
	case parnrorientfineadjustpitch:
	case parnrorientcoarseadjustheading:
	case parnrorientfineadjustheading:
	case parnrorienterectdefault:
	{
		paramfail_t p;
		{
			Glib::Mutex::Lock lock(m_mutex);
			//statechange();
			p = (m_tstate == tstate_running) ? paramfail_ok : paramfail_fail;
		}
		v = 0;
		return p;
	}

	}
	return SensorAttitude::get_param(nr, v);
}

Sensors::SensorAttitudeSTMSensorHub::paramfail_t Sensors::SensorAttitudeSTMSensorHub::get_param(unsigned int nr, double& v) const
{
	switch (nr) {
	default:
		break;

	case parnrcoeffla:
	{
		double x;
		paramfail_t p;
		{
			Glib::Mutex::Lock lock(m_mutex);
			//statechange();
			x = m_ahrs.get_coeff_la();
			p = (m_tstate == tstate_running) ? paramfail_ok : paramfail_fail;
		}
		v = x;
		return p;
	}

	case parnrcoefflc:
	{
		double x;
		paramfail_t p;
		{
			Glib::Mutex::Lock lock(m_mutex);
			//statechange();
			x = m_ahrs.get_coeff_lc();
			p = (m_tstate == tstate_running) ? paramfail_ok : paramfail_fail;
		}
		v = x;
		return p;
	}

	case parnrcoeffld:
	{
		double x;
		paramfail_t p;
		{
			Glib::Mutex::Lock lock(m_mutex);
			//statechange();
			x = m_ahrs.get_coeff_ld();
			p = (m_tstate == tstate_running) ? paramfail_ok : paramfail_fail;
		}
		v = x;
		return p;
	}

	case parnrcoeffsigma:
	{
		double x;
		paramfail_t p;
		{
			Glib::Mutex::Lock lock(m_mutex);
			//statechange();
			x = m_ahrs.get_coeff_sigma();
			p = (m_tstate == tstate_running) ? paramfail_ok : paramfail_fail;
		}
		v = x;
		return p;
	}

	case parnrcoeffn:
	{
		double x;
		paramfail_t p;
		{
			Glib::Mutex::Lock lock(m_mutex);
			//statechange();
			x = m_ahrs.get_coeff_n();
			p = (m_tstate == tstate_running) ? paramfail_ok : paramfail_fail;
		}
		v = x;
		return p;
	}

	case parnrcoeffo:
	{
		double x;
		paramfail_t p;
		{
			Glib::Mutex::Lock lock(m_mutex);
			//statechange();
			x = m_ahrs.get_coeff_o();
			p = (m_tstate == tstate_running) ? paramfail_ok : paramfail_fail;
		}
		v = x;
		return p;
	}

	case parnrattituderoll:
	case parnrattitudepitch:
	case parnrattitudeheading:
	{
		double x;
		paramfail_t p;
		{
			Glib::Mutex::Lock lock(m_mutex);
			//statechange();
			Eigen::Quaterniond q(m_ahrs.get_sensororientation() * m_ahrs.get_attitude().conjugate());
			AHRS::EulerAnglesd ea(AHRS::to_euler(q));
			x = ea[nr - parnrattitude] * (180.0 / M_PI);
			p = (m_tstate == tstate_running) ? paramfail_ok : paramfail_fail;
		}
		v = x;
		return p;
	}

	case parnrorientationroll:
	case parnrorientationpitch:
	case parnrorientationheading:
	{
		double x;
		paramfail_t p;
		{
			Glib::Mutex::Lock lock(m_mutex);
			//statechange();
			AHRS::EulerAnglesd ea(AHRS::to_euler(m_ahrs.get_sensororientation()));
			x = ea[nr - parnrorientation] * (180.0 / M_PI);
			p = (m_tstate == tstate_running) ? paramfail_ok : paramfail_fail;
		}
		v = x;
		return p;
	}

	case parnrorientationdeviation:
	{
		double x;
		paramfail_t p;
		{
			Glib::Mutex::Lock lock(m_mutex);
			//statechange();
			x = m_ahrs.get_magdeviation();
			p = (m_tstate == tstate_running) ? paramfail_ok : paramfail_fail;
		}
		v = x;
		return p;
	}

	case parnrrawgyrox:
	case parnrrawgyroy:
	case parnrrawgyroz:
	{
		double x;
		paramfail_t p;
		{
			Glib::Mutex::Lock lock(m_mutex);
			//statechange();
			x = m_ahrs.get_gyroscope()(nr - parnrrawgyro);
			p = (m_tstate == tstate_running) ? paramfail_ok : paramfail_fail;
		}
		v = x;
		return p;
	}

	case parnrrawaccelx:
	case parnrrawaccely:
	case parnrrawaccelz:
	{
		double x;
		paramfail_t p;
		{
			Glib::Mutex::Lock lock(m_mutex);
			//statechange();
			x = m_ahrs.get_accelerometer()(nr - parnrrawaccel);
			p = (m_tstate == tstate_running) ? paramfail_ok : paramfail_fail;
		}
		v = x;
		return p;
	}

	case parnrrawmagx:
	case parnrrawmagy:
	case parnrrawmagz:
	{
		double x;
		paramfail_t p;
		{
			Glib::Mutex::Lock lock(m_mutex);
			//statechange();
			x = m_ahrs.get_magnetometer()(nr - parnrrawmag);
			p = (m_tstate == tstate_running) ? paramfail_ok : paramfail_fail;
		}
		v = x;
		return p;
	}

	case parnrattitudequatx:
	{
		double x;
		paramfail_t p;
		{
			Glib::Mutex::Lock lock(m_mutex);
			//statechange();
			x = m_ahrs.get_attitude().x();
			p = (m_tstate == tstate_running) ? paramfail_ok : paramfail_fail;
		}
		v = x;
		return p;
	}

	case parnrattitudequaty:
	{
		double x;
		paramfail_t p;
		{
			Glib::Mutex::Lock lock(m_mutex);
			//statechange();
			x = m_ahrs.get_attitude().y();
			p = (m_tstate == tstate_running) ? paramfail_ok : paramfail_fail;
		}
		v = x;
		return p;
	}

	case parnrattitudequatz:
	{
		double x;
		paramfail_t p;
		{
			Glib::Mutex::Lock lock(m_mutex);
			//statechange();
			x = m_ahrs.get_attitude().z();
			p = (m_tstate == tstate_running) ? paramfail_ok : paramfail_fail;
		}
		v = x;
		return p;
	}

	case parnrattitudequatw:
	{
		double x;
		paramfail_t p;
		{
			Glib::Mutex::Lock lock(m_mutex);
			//statechange();
			x = m_ahrs.get_attitude().w();
			p = (m_tstate == tstate_running) ? paramfail_ok : paramfail_fail;
		}
		v = x;
		return p;
	}

	case parnrorientationquatx:
	{
		double x;
		paramfail_t p;
		{
			Glib::Mutex::Lock lock(m_mutex);
			//statechange();
			x = m_ahrs.get_sensororientation().x();
			p = (m_tstate == tstate_running) ? paramfail_ok : paramfail_fail;
		}
		v = x;
		return p;
	}

	case parnrorientationquaty:
	{
		double x;
		paramfail_t p;
		{
			Glib::Mutex::Lock lock(m_mutex);
			//statechange();
			x = m_ahrs.get_sensororientation().y();
			p = (m_tstate == tstate_running) ? paramfail_ok : paramfail_fail;
		}
		v = x;
		return p;
	}

	case parnrorientationquatz:
	{
		double x;
		paramfail_t p;
		{
			Glib::Mutex::Lock lock(m_mutex);
			//statechange();
			x = m_ahrs.get_sensororientation().z();
			p = (m_tstate == tstate_running) ? paramfail_ok : paramfail_fail;
		}
		v = x;
		return p;
	}

	case parnrorientationquatw:
	{
		double x;
		paramfail_t p;
		{
			Glib::Mutex::Lock lock(m_mutex);
			//statechange();
			x = m_ahrs.get_sensororientation().w();
			p = (m_tstate == tstate_running) ? paramfail_ok : paramfail_fail;
		}
		v = x;
		return p;
	}

	case parnrcalgyrobiasx:
	case parnrcalgyrobiasy:
	case parnrcalgyrobiasz:
	{
		double x;
		paramfail_t p;
		{
			Glib::Mutex::Lock lock(m_mutex);
			//statechange();
			x = m_ahrs.get_gyrobias()(nr - parnrcalgyrobias);
			p = (m_tstate == tstate_running) ? paramfail_ok : paramfail_fail;
		}
		v = x;
		return p;
	}

	case parnrcalgyroscalex:
	case parnrcalgyroscaley:
	case parnrcalgyroscalez:
	{
		double x;
		paramfail_t p;
		{
			Glib::Mutex::Lock lock(m_mutex);
			//statechange();
			x = m_ahrs.get_gyroscale()(nr - parnrcalgyroscale);
			p = (m_tstate == tstate_running) ? paramfail_ok : paramfail_fail;
		}
		v = x;
		return p;
	}

	case parnrcalaccelbiasx:
	case parnrcalaccelbiasy:
	case parnrcalaccelbiasz:
	{
		double x;
		paramfail_t p;
		{
			Glib::Mutex::Lock lock(m_mutex);
			//statechange();
			x = m_ahrs.get_accelbias()(nr - parnrcalaccelbias);
			p = (m_tstate == tstate_running) ? paramfail_ok : paramfail_fail;
		}
		v = x;
		return p;
	}

	case parnrcalaccelscalex:
	case parnrcalaccelscaley:
	case parnrcalaccelscalez:
	{
		double x;
		paramfail_t p;
		{
			Glib::Mutex::Lock lock(m_mutex);
			//statechange();
			x = m_ahrs.get_accelscale()(nr - parnrcalaccelscale);
			p = (m_tstate == tstate_running) ? paramfail_ok : paramfail_fail;
		}
		v = x;
		return p;
	}

	case parnrcalmagbiasx:
	case parnrcalmagbiasy:
	case parnrcalmagbiasz:
	{
		double x;
		paramfail_t p;
		{
			Glib::Mutex::Lock lock(m_mutex);
			//statechange();
			x = m_ahrs.get_magbias()(nr - parnrcalmagbias);
			p = (m_tstate == tstate_running) ? paramfail_ok : paramfail_fail;
		}
		v = x;
		return p;
	}

	case parnrcalmagscalex:
	case parnrcalmagscaley:
	case parnrcalmagscalez:
	{
		double x;
		paramfail_t p;
		{
			Glib::Mutex::Lock lock(m_mutex);
			//statechange();
			x = m_ahrs.get_magscale()(nr - parnrcalmagscale);
			p = (m_tstate == tstate_running) ? paramfail_ok : paramfail_fail;
		}
		v = x;
		return p;
	}

	case parnrcalaccelscale2:
	{
		double x;
		paramfail_t p;
		{
			Glib::Mutex::Lock lock(m_mutex);
			//statechange();
			x = m_ahrs.get_accelscale2();
			p = (m_tstate == tstate_running) ? paramfail_ok : paramfail_fail;
		}
		v = x;
		return p;
	}

	case parnrcalmagscale2:
	{
		double x;
		paramfail_t p;
		{
			Glib::Mutex::Lock lock(m_mutex);
			//statechange();
			x = m_ahrs.get_magscale2();
			p = (m_tstate == tstate_running) ? paramfail_ok : paramfail_fail;
		}
		v = x;
		return p;
	}

	case parnrrawsamplemagx:
	case parnrrawsamplemagy:
	case parnrrawsamplemagz:
	{
		double x;
		paramfail_t p;
		{
			Glib::Mutex::Lock lock(m_mutex);
			//statechange();
			x = m_rawmag[nr - parnrrawsamplemag];
			p = (m_tstate == tstate_running) ? paramfail_ok : paramfail_fail;
		}
		v = x;
		return p;
	}
	}
	return SensorAttitude::get_param(nr, v);
}

Sensors::SensorAttitudeSTMSensorHub::paramfail_t Sensors::SensorAttitudeSTMSensorHub::get_param(unsigned int nr, double& pitch, double& bank, double& slip, double& hdg, double& rate) const
{
	update_values();
	double p, b, s, h, r;
	paramfail_t pf;
	{
		Glib::Mutex::Lock lock(m_mutex);
		//statechange();
		p = m_pitch;
		b = m_bank;
		s = m_slip;
		h = m_hdg;
		r = m_rate;
		pf = (m_tstate == tstate_running) ? paramfail_ok : paramfail_fail;
		if (pf == paramfail_ok) {
			Glib::TimeVal tv;
			tv.assign_current_time();
			tv.subtract(m_attitudetime);
			tv.add_seconds(-1);
			if (!tv.negative())
				pf = paramfail_fail;
		}
	}
	pitch = p;
	bank = b;
	slip = s;
	hdg = h;
	rate = r;
	if (std::isnan(p) || std::isnan(b) || std::isnan(s) || std::isnan(h) || std::isnan(r))
		return paramfail_fail;
	return pf;
}

void Sensors::SensorAttitudeSTMSensorHub::set_param(unsigned int nr, const Glib::ustring& v)
{
	switch (nr) {
	default:
		SensorAttitude::set_param(nr, v);
		return;

	}
	ParamChanged pc;
	pc.set_changed(nr);
	update(pc);
}

void Sensors::SensorAttitudeSTMSensorHub::set_param(unsigned int nr, int v)
{
	switch (nr) {
	default:
		SensorAttitude::set_param(nr, v);
		return;

	case parnrattitudeprio:
		m_attitudeprio = v;
		get_sensors().get_configfile().set_uint64(get_configgroup(), "attitudepriority", m_attitudeprio);
		return;

	case parnrheadingprio:
		m_headingprio = v;
		get_sensors().get_configfile().set_uint64(get_configgroup(), "headingpriority", m_headingprio);
		return;

	case parnrmode:
	{
		int v1(v);
		if (v1 >= 0 && v1 < (int)(sizeof(ahrsmodemap)/sizeof(ahrsmodemap[0]))) {
			Glib::Mutex::Lock lock(m_mutex);
			statechange();
			m_ahrs.set_mode(ahrsmodemap[v1]);
		}
		break;
	}

	case parnrorientcoarseadjustroll:
	case parnrorientfineadjustroll:
	case parnrorientcoarseadjustpitch:
	case parnrorientfineadjustpitch:
	case parnrorientcoarseadjustheading:
	case parnrorientfineadjustheading:
	{
		if (!(v & 0x0F))
			break;
		int i = nr - parnrorientadjust;
		double angle((i & 1) ? (2.0*M_PI/180.0*0.5) : (0.1*M_PI/180.0*0.5));
		i >>= 1;
		for (int j = 0; j < 2; ++j) {
			if (!(v & (1 << j)))
				continue;
			double a((2 * j - 1) * angle);
			Eigen::Quaterniond q(cos(a), 0, 0, 0);
			q.coeffs()[i] = sin(a);
			{
				Glib::Mutex::Lock lock(m_mutex);
				statechange();
				Eigen::Quaterniond q1(m_ahrs.get_sensororientation());
				q1 = q * q1;
				m_ahrs.set_sensororientation(q1);
			}
		}
		ParamChanged pc;
		pc.set_changed(nr);
		pc.set_changed(parnrattituderoll, parnrattitudeheading);
		pc.set_changed(parnrorientationroll, parnrorientationheading);
		pc.set_changed(parnrorientationquatx, parnrorientationquatw);
		pc.set_changed(parnrgyro);
		update(pc);
		return;
	}

	case parnrorienterectdefault:
	{
		if (!(v & 0x03))
			break;
		if (v & 0x01) {
			// Erect
			Glib::Mutex::Lock lock(m_mutex);
			statechange();
			Eigen::Quaterniond q(m_ahrs.get_attitude());
			q.normalize();
			m_ahrs.set_sensororientation(q);
		}
		if (v & 0x02) {
			// Default
			Glib::Mutex::Lock lock(m_mutex);
			statechange();
			m_ahrs.set_sensororientation(Eigen::Quaterniond(1, 0, 0, 0));
		}
		ParamChanged pc;
		pc.set_changed(nr);
		pc.set_changed(parnrattituderoll, parnrattitudeheading);
		pc.set_changed(parnrorientationroll, parnrorientationheading);
		pc.set_changed(parnrorientationquatx, parnrorientationquatw);
		pc.set_changed(parnrgyro);
		update(pc);
		return;
	}

	}
	ParamChanged pc;
	pc.set_changed(nr);
	update(pc);
}

void Sensors::SensorAttitudeSTMSensorHub::set_param(unsigned int nr, double v)
{
	switch (nr) {
	default:
		SensorAttitude::set_param(nr, v);
		return;

	case parnrcoeffla:
	{
		Glib::Mutex::Lock lock(m_mutex);
		statechange();
		m_ahrs.set_coeff_la(v);
		break;
	}

	case parnrcoefflc:
	{
		Glib::Mutex::Lock lock(m_mutex);
		statechange();
		m_ahrs.set_coeff_lc(v);
		break;
	}

	case parnrcoeffld:
	{
		Glib::Mutex::Lock lock(m_mutex);
		statechange();
		m_ahrs.set_coeff_ld(v);
		break;
	}

	case parnrcoeffsigma:
	{
		Glib::Mutex::Lock lock(m_mutex);
		statechange();
		m_ahrs.set_coeff_sigma(v);
		break;
	}

	case parnrcoeffn:
	{
		Glib::Mutex::Lock lock(m_mutex);
		statechange();
		m_ahrs.set_coeff_n(v);
		break;
	}

	case parnrcoeffo:
	{
		Glib::Mutex::Lock lock(m_mutex);
		statechange();
		m_ahrs.set_coeff_o(v);
		break;
	}

	case parnrorientationdeviation:
	{
		while (v < 0)
			v += 360;
		while (v >= 360)
			v -= 360;
		Glib::Mutex::Lock lock(m_mutex);
		statechange();
		m_ahrs.set_magdeviation(v);
		break;
	}

	case parnrorientationquatx:
	{
		{
			Glib::Mutex::Lock lock(m_mutex);
			statechange();
			Eigen::Quaterniond q(m_ahrs.get_sensororientation());
			q.x() = v;
			q.normalize();
			m_ahrs.set_sensororientation(q);
		}
		ParamChanged pc;
		pc.set_changed(parnrorientationquatx, parnrorientationquatz);
		update(pc);
		return;
	}

	case parnrorientationquaty:
	{
		{
			Glib::Mutex::Lock lock(m_mutex);
			statechange();
			Eigen::Quaterniond q(m_ahrs.get_sensororientation());
			q.y() = v;
			q.normalize();
			m_ahrs.set_sensororientation(q);
		}
		ParamChanged pc;
		pc.set_changed(parnrorientationquatx, parnrorientationquatz);
		update(pc);
		return;
	}

	case parnrorientationquatz:
	{
		{
			Glib::Mutex::Lock lock(m_mutex);
			Eigen::Quaterniond q(m_ahrs.get_sensororientation());
			q.z() = v;
			q.normalize();
			m_ahrs.set_sensororientation(q);
		}
		ParamChanged pc;
		pc.set_changed(parnrorientationquatx, parnrorientationquatz);
		update(pc);
		return;
	}

	case parnrorientationquatw:
	{
		{
			Glib::Mutex::Lock lock(m_mutex);
			statechange();
			Eigen::Quaterniond q(m_ahrs.get_sensororientation());
			q.w() = v;
			q.normalize();
			m_ahrs.set_sensororientation(q);
		}
		ParamChanged pc;
		pc.set_changed(parnrorientationquatx, parnrorientationquatz);
		update(pc);
		return;
	}

	case parnrcalgyroscalex:
	case parnrcalgyroscaley:
	case parnrcalgyroscalez:
	{
		Glib::Mutex::Lock lock(m_mutex);
		statechange();
		Eigen::Vector3d x(m_ahrs.get_gyroscale());
	     	x(nr - parnrcalgyroscale) = v;
		m_ahrs.set_gyroscale(x);
		break;
	}

	case parnrcalaccelbiasx:
	case parnrcalaccelbiasy:
	case parnrcalaccelbiasz:
	{
		Glib::Mutex::Lock lock(m_mutex);
		statechange();
		Eigen::Vector3d x(m_ahrs.get_accelbias());
	     	x(nr - parnrcalaccelbias) = v;
		m_ahrs.set_accelbias(x);
		break;
	}

	case parnrcalaccelscalex:
	case parnrcalaccelscaley:
	case parnrcalaccelscalez:
	{
		Glib::Mutex::Lock lock(m_mutex);
		statechange();
		Eigen::Vector3d x(m_ahrs.get_accelscale());
	     	x(nr - parnrcalaccelscale) = v;
		m_ahrs.set_accelscale(x);
		break;
	}

	case parnrcalmagbiasx:
	case parnrcalmagbiasy:
	case parnrcalmagbiasz:
	{
		Glib::Mutex::Lock lock(m_mutex);
		statechange();
		Eigen::Vector3d x(m_ahrs.get_magbias());
		x(nr - parnrcalmagbias) = v;
		m_ahrs.set_magbias(x);
		break;
	}

	case parnrcalmagscalex:
	case parnrcalmagscaley:
	case parnrcalmagscalez:
	{
		Glib::Mutex::Lock lock(m_mutex);
		statechange();
		Eigen::Vector3d x(m_ahrs.get_magscale());
		x(nr - parnrcalmagscale) = v;
		m_ahrs.set_magscale(x);
		break;
	}
	}
	ParamChanged pc;
	pc.set_changed(nr);
	update(pc);
}

std::string Sensors::SensorAttitudeSTMSensorHub::logfile_header(void) const
{
	return SensorAttitude::logfile_header() + ",GyroX,GyroY,GyroZ,AccelX,AccelY,AccelZ,MagX,MagY,MagZ,AttX,AttY,AttZ,AttW,GyroBX,GyroBY,GyroBZ,Accel2,Mag2,RawMagX,RawMagY,RawMagZ,Mode";
}

std::string Sensors::SensorAttitudeSTMSensorHub::logfile_records(void) const
{
	std::ostringstream oss;
	oss << SensorAttitude::logfile_records() << std::fixed << std::setprecision(6) << ',';
	{
		Eigen::Vector3d v(m_ahrs.get_gyroscope());
		if (!std::isnan(v(0)))
			oss << v(0);
		oss << ',';
		if (!std::isnan(v(1)))
			oss << v(1);
		oss << ',';
		if (!std::isnan(v(2)))
			oss << v(2);
	}
	{
		Eigen::Vector3d v(m_ahrs.get_accelerometer());
		oss << ',';
		if (!std::isnan(v(0)))
			oss << v(0);
		oss << ',';
		if (!std::isnan(v(1)))
			oss << v(1);
		oss << ',';
		if (!std::isnan(v(2)))
			oss << v(2);
	}
	{
		Eigen::Vector3d v(m_ahrs.get_magnetometer());
		oss << ',';
		if (!std::isnan(v(0)))
			oss << v(0);
		oss << ',';
		if (!std::isnan(v(1)))
			oss << v(1);
		oss << ',';
		if (!std::isnan(v(2)))
			oss << v(2);
	}
	{
		Eigen::Quaterniond v(m_ahrs.get_attitude());
		oss << ',';
		if (!std::isnan(v.x()))
			oss << v.x();
		oss << ',';
		if (!std::isnan(v.y()))
			oss << v.y();
		oss << ',';
		if (!std::isnan(v.z()))
			oss << v.z();
		oss << ',';
		if (!std::isnan(v.w()))
			oss << v.w();
	}
	{
		Eigen::Vector3d v(m_ahrs.get_gyrobias());
		oss << ',';
		if (!std::isnan(v(0)))
			oss << v(0);
		oss << ',';
		if (!std::isnan(v(1)))
			oss << v(1);
		oss << ',';
		if (!std::isnan(v(2)))
			oss << v(2);
	}
	{
		double v(m_ahrs.get_accelscale2());
		oss << ',';
		if (!std::isnan(v))
			oss << v;
	}
	{
		double v(m_ahrs.get_magscale2());
		oss << ',';
		if (!std::isnan(v))
			oss << v;
	}
	oss << ',';
	if (!std::isnan(m_rawmag[0]))
		oss << m_rawmag[0];
	oss << ',';
	if (!std::isnan(m_rawmag[1]))
		oss << m_rawmag[1];
	oss << ',';
	if (!std::isnan(m_rawmag[2]))
		oss << m_rawmag[2];
	oss << ',' << AHRS::to_str(m_ahrs.get_mode());
	return oss.str();
}
