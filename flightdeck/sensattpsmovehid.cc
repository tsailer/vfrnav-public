//
// C++ Implementation: Attitude Sensor: psmove using hidapi
//
// Description: Attitude Sensor Handling
//
//
// Author: Thomas Sailer <t.sailer@alumni.ethz.ch>, (C) 2012
//
// Copyright: See COPYING file that comes with this distribution
//
//


// "Real" HID interface
// http://www.signal11.us/oss/udev/
// http://www.signal11.us/oss/hidapi/
// http://thp.io/2010/psmove/
// http://code.google.com/p/moveonpc/wiki/InputReport


#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "sysdeps.h"

#include <iostream>
#include <iomanip>
#include <cstdlib>

#include "sensattpsmovehid.h"

const unsigned short Sensors::SensorAttitudePSMoveHID::PSMOVE_2_VID;
const unsigned short Sensors::SensorAttitudePSMoveHID::PSMOVE_2_PID;

Sensors::SensorAttitudePSMoveHID::SensorAttitudePSMoveHID(Sensors& sensors, const Glib::ustring& configgroup)
	: SensorAttitudePSMove(sensors, configgroup), m_udev(0), m_udevmon(0), m_dev(0)
{
	const Glib::KeyFile& cf(get_sensors().get_configfile());
	if (hid_init())
		throw std::runtime_error("PSMove: unable to initialize hidapi");
	m_udev = udev_new();
	if (!m_udev)
		throw std::runtime_error("PSMove: unable to create udev");
	m_udevmon = udev_monitor_new_from_netlink(m_udev, "udev");
	if (!m_udevmon)
		throw std::runtime_error("PSMove: unable to create udev monitor");
	if (udev_monitor_filter_add_match_subsystem_devtype(m_udevmon, "hidraw", NULL))
		throw std::runtime_error("PSMove: udev monitor: error adding filter");
	if (udev_monitor_enable_receiving(m_udevmon))
		throw std::runtime_error("PSMove: udev monitor: cannot start receiving");
	m_conndevarrival = Glib::signal_io().connect(sigc::mem_fun(*this, &SensorAttitudePSMoveHID::handle_connect_cb),
						     udev_monitor_get_fd(m_udevmon), Glib::IO_IN | Glib::IO_HUP);
	handle_initial_connect();
	//m_conndev = Glib::signal_idle().connect(sigc::mem_fun(*this, &SensorAttitudePSMoveHID::handle_initial_connect));
}

Sensors::SensorAttitudePSMoveHID::~SensorAttitudePSMoveHID()
{
	close_dev();
	m_conndevarrival.disconnect();
	if (m_udevmon)
		udev_monitor_unref(m_udevmon);
	m_udevmon = 0;
	if (m_udev)
		udev_unref(m_udev);
	m_udev = 0;
	hid_exit();
}

Glib::ustring Sensors::SensorAttitudePSMoveHID::wstr2ustr(const std::wstring& ws)
{
	size_t len = wcstombs(0, ws.c_str(), 0);
	if (len == (size_t)-1)
		return Glib::ustring();
	char buf[len + 1];
	len = wcstombs(buf, ws.c_str(), len + 1);
	return Glib::ustring(buf, len);
}

std::wstring Sensors::SensorAttitudePSMoveHID::ustr2wstr(const Glib::ustring& us)
{
	size_t len = mbstowcs(0, us.c_str(), 0);
	if (len == (size_t)-1)
		return std::wstring();
	wchar_t buf[len + 1];
	len = mbstowcs(buf, us.c_str(), len + 1);
	return std::wstring(buf, len);
}

bool Sensors::SensorAttitudePSMoveHID::handle_connect_cb(Glib::IOCondition iocond)
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

bool Sensors::SensorAttitudePSMoveHID::handle_initial_connect(void)
{
	if (is_open())
		return false;
	struct udev_enumerate *enumerate = udev_enumerate_new(m_udev);
	udev_enumerate_add_match_subsystem(enumerate, "hidraw");
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

void Sensors::SensorAttitudePSMoveHID::handle_connect(struct udev_device *udev)
{
	if (!udev)
		return;
	{
		const char *subsys = udev_device_get_subsystem(udev);
		if (g_strcmp0(subsys, "hidraw") != 0) {
			udev_device_unref(udev);
			return;
		}
	}
	struct udev_device *pdev = udev_device_get_parent(udev);
	if (!pdev) {
		udev_device_unref(udev);
		return;
	}
	{
		const char *subsys = udev_device_get_subsystem(pdev);
		if (g_strcmp0(subsys, "hid") != 0) {
			udev_device_unref(udev);
			return;
		}
	}
	Glib::ustring serial;
	Glib::ustring ctrlname;
	unsigned int busnum(0), vendorid(0), prodid(0);
	{
		const char *id(udev_device_get_property_value(pdev, "HID_ID")); // 0005:0000054C:000003D5
		const char *name(udev_device_get_property_value(pdev, "HID_NAME")); // Motion Controller
		const char *phys(udev_device_get_property_value(pdev, "HID_PHYS")); // Host Bluetooth Address
		const char *uniq(udev_device_get_property_value(pdev, "HID_UNIQ")); // Device Bluetooth Address
		if (!id || !name || !phys || !uniq) {
			udev_device_unref(udev);
			return;
		}
		serial = uniq;
		ctrlname = *name;
		if (sscanf(id, "%x:%x:%x", &busnum, &vendorid, &prodid) != 3) {
			udev_device_unref(udev);
			return;
		}
	}
	// check for node
	std::string path;
	{
		const char *cp(udev_device_get_devnode(udev));
		if (!cp) {
			udev_device_unref(udev);
			return;
		}
		path = cp;
	}
	udev_device_unref(udev);
	if (true) {
		std::ostringstream oss;
		oss << "PSMove: Device Connect: " << path << std::hex
		    << " bus " << std::setfill('0') << std::setw(4) << busnum
		    << " vendorid " << std::setfill('0') << std::setw(4) << vendorid
		    << " productid " << std::setfill('0') << std::setw(4) << prodid
		    << " serial " << serial;
		get_sensors().log(Sensors::loglevel_notice, oss.str());
	}
	// Hack!
	if (vendorid == PSMOVE_2_VID && prodid == PSMOVE_2_PID) {
		vendorid = PSMOVE_VID;
		prodid = PSMOVE_PID;
	}
	handle_connect(hid_open_path(path.c_str()), busnum, vendorid, prodid, serial);
}

void Sensors::SensorAttitudePSMoveHID::handle_connect(hid_device *dev, uint16_t busnum,
						      uint32_t vndid, uint32_t prodid, const Glib::ustring& serial)
{
	if (!dev)
		return;
	if (busnum != 5) {
		std::ostringstream oss;
		oss << "PSMove: invalid bus " << busnum << " (need bluetooth (5))";
		get_sensors().log(Sensors::loglevel_notice, oss.str());
		hid_close(dev);
		return;
	}
	if (open(vndid, prodid, serial)) {
		close_dev();
		m_dev = dev;
		hid_set_nonblocking(m_dev, 1);
		m_conndev = Glib::signal_io().connect(sigc::mem_fun(*this, &SensorAttitudePSMoveHID::handle_dev_read),
						      *(int *)m_dev, Glib::IO_IN | Glib::IO_HUP);
		return;
	}
	hid_close(dev);
}

void Sensors::SensorAttitudePSMoveHID::close_dev(void)
{
	m_conndev.disconnect();
	if (m_dev)
		hid_close(m_dev);
	m_dev = 0;
}

bool Sensors::SensorAttitudePSMoveHID::handle_dev_read(Glib::IOCondition iocond)
{
        uint8_t report[256];
        int nr = hid_read(m_dev, report, sizeof(report));
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
                std::cerr << "SensorAttitudePSMoveHID: report " << oss.str() << std::endl;
        }
	handle_report(report, nr);
        return true;
}
