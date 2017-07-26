//
// C++ Implementation: Attitude Sensor: Linux Industrial I/O API
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

#include "sensattiio.h"

Sensors::SensorAttitudeIIO::SensorAttitudeIIO(Sensors& sensors, const Glib::ustring& configgroup)
	: SensorAttitude(sensors, configgroup), m_udev(0), m_udevmon(0), m_ahrs(50, "", AHRS::init_standard), 
	  m_pitch(std::numeric_limits<double>::quiet_NaN()), m_bank(std::numeric_limits<double>::quiet_NaN()),
	  m_slip(std::numeric_limits<double>::quiet_NaN()), m_rate(std::numeric_limits<double>::quiet_NaN()),
	  m_hdg(std::numeric_limits<double>::quiet_NaN()), m_dirty(false), m_samplecount(0),
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
	// initialize udev
	for (unsigned int i = 0; i < devtyp_invalid; ++i)
		m_devfd[i] = m_devnum[i] = -1;
	m_udev = udev_new();
	if (!m_udev)
		throw std::runtime_error("PSMove: unable to create udev");
	m_udevmon = udev_monitor_new_from_netlink(m_udev, "udev");
	if (!m_udevmon)
		throw std::runtime_error("PSMove: unable to create udev monitor");
	if (udev_monitor_filter_add_match_subsystem_devtype(m_udevmon, "iio", NULL))
		throw std::runtime_error("PSMove: udev monitor: error adding filter");
	if (udev_monitor_enable_receiving(m_udevmon))
		throw std::runtime_error("PSMove: udev monitor: cannot start receiving");
	m_conndevarrival = Glib::signal_io().connect(sigc::mem_fun(*this, &SensorAttitudeIIO::handle_connect_cb),
						     udev_monitor_get_fd(m_udevmon), Glib::IO_IN | Glib::IO_HUP);
	handle_initial_connect();
}

Sensors::SensorAttitudeIIO::~SensorAttitudeIIO()
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


bool Sensors::SensorAttitudeIIO::handle_connect_cb(Glib::IOCondition iocond)
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

bool Sensors::SensorAttitudeIIO::handle_initial_connect(void)
{
	if (is_open())
		return false;
	struct udev_enumerate *enumerate = udev_enumerate_new(m_udev);
	udev_enumerate_add_match_subsystem(enumerate, "iio");
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

void Sensors::SensorAttitudeIIO::handle_connect(struct udev_device *udev)
{
	if (!udev)
		return;
	{
		const char *subsys = udev_device_get_subsystem(udev);
		if (g_strcmp0(subsys, "iio") != 0) {
			udev_device_unref(udev);
			return;
		}
	}
	const char *sysname(udev_device_get_sysname(udev));
	const char *sysnum(udev_device_get_sysnum(udev));
	const char *attrname(udev_device_get_sysattr_value(udev, "name"));
	if (true) {
		std::ostringstream oss;
		oss << "IIOAttitude: Device Connect: " << (sysname ? sysname : "(null)")
		    << ' ' << (sysnum ? sysnum : "(null)") << " type "
		    << (attrname ? attrname : "(null)");
		get_sensors().log(Sensors::loglevel_notice, oss.str());
	}
	if (!sysname || strcmp(sysname, "iio:device")) {
		udev_device_unref(udev);
		return;
	}
	int devnum(-1);
	if (sysnum) {
		char *cp(0);
		devnum = strtoul(sysnum, &cp, 10);
		if (cp == sysnum || *cp)
			devnum = -1;
	}
	if (devnum == -1) {
		udev_device_unref(udev);
		return;
	}




	udev_device_unref(udev);
}

bool Sensors::SensorAttitudeIIO::close_internal(void)
{
	bool wasopen(is_open());



	if (wasopen) {
		m_ahrs.save(get_sensors().get_configfile(), get_configgroup());
		m_ahrs.save();
	}
	m_ahrs.set_id("");
	m_attitudetime = Glib::TimeVal();
	m_attitudereporttime = Glib::TimeVal();
	m_pitch = m_bank = m_slip = m_rate = m_hdg = std::numeric_limits<double>::quiet_NaN();
	m_dirty = false;
	m_samplecount = 0;
	m_rawgyro[0] = m_rawgyro[1] = m_rawgyro[2] = m_rawaccel[0] = m_rawaccel[1] = m_rawaccel[2] = m_rawmag[0] = m_rawmag[1] = m_rawmag[2] = 0;
	return wasopen;
}

bool Sensors::SensorAttitudeIIO::close(void)
{
	if (close_internal()) {
		ParamChanged pc;
		pc.set_changed(parnracceldevname, parnrmagdevserial);
		pc.set_changed(parnrmode, parnrgyro);
		update(pc);
		return true;
	}
	return false;
}

bool Sensors::SensorAttitudeIIO::try_connect(void)
{
	if (close()) {
                ParamChanged pc;
		pc.set_changed(parnracceldevname, parnrmagdevserial);
		pc.set_changed(parnrmode, parnrgyro);
                update(pc);
        }



	return false;
}


void Sensors::SensorAttitudeIIO::update_values(void) const
{
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
	
void Sensors::SensorAttitudeIIO::get_heading(double& hdg) const
{
	update_values();
	hdg = m_hdg;
}

unsigned int Sensors::SensorAttitudeIIO::get_heading_priority(void) const
{
	return m_headingprio;
}

Glib::TimeVal Sensors::SensorAttitudeIIO::get_heading_time(void) const
{
	return m_attitudetime;
}

bool Sensors::SensorAttitudeIIO::is_heading_true(void) const
{
	return false;
}

bool Sensors::SensorAttitudeIIO::is_heading_ok(const Glib::TimeVal& tv) const
{
	return is_open() && SensorAttitude::is_heading_ok(tv);
}

void Sensors::SensorAttitudeIIO::get_attitude(double& pitch, double& bank, double& slip, double& rate) const
{
	update_values();
	pitch = m_pitch;
	bank = m_bank;
	slip = m_slip;
	rate = m_rate;
}

unsigned int Sensors::SensorAttitudeIIO::get_attitude_priority(void) const
{
	return m_attitudeprio;
}

Glib::TimeVal Sensors::SensorAttitudeIIO::get_attitude_time(void) const
{
	return m_attitudetime;
}

bool Sensors::SensorAttitudeIIO::is_attitude_ok(const Glib::TimeVal& tv) const
{
	return is_open() && SensorAttitude::is_attitude_ok(tv);
}

const AHRS::mode_t  Sensors::SensorAttitudeIIO::ahrsmodemap[4] = {
	AHRS::mode_slow, AHRS::mode_normal, AHRS::mode_fast, AHRS::mode_gyroonly
};

void Sensors::SensorAttitudeIIO::get_param_desc(unsigned int pagenr, paramdesc_t& pd)
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
			pdi = pd.insert(pdi, ParamDesc(ParamDesc::type_string, ParamDesc::editable_readonly, parnracceldevdesc, "Accel Description", "Acceleration Sensor Device Description", ""));
			++pdi;
			pdi = pd.insert(pdi, ParamDesc(ParamDesc::type_string, ParamDesc::editable_readonly, parnracceldevserial, "Accel Serial", "Acceleration Sensor Device Serial Number", ""));
			++pdi;
			pdi = pd.insert(pdi, ParamDesc(ParamDesc::type_string, ParamDesc::editable_readonly, parnrgyrodevname, "Gyro Device Name", "Gyro Sensor Device Name", ""));
			++pdi;
			pdi = pd.insert(pdi, ParamDesc(ParamDesc::type_string, ParamDesc::editable_readonly, parnrgyrodevmfg, "Gyro Manufacturer", "Gyro Sensor Device Manufacturer", ""));
			++pdi;
			pdi = pd.insert(pdi, ParamDesc(ParamDesc::type_string, ParamDesc::editable_readonly, parnrgyrodevdesc, "Gyro Description", "Gyro Sensor Device Description", ""));
			++pdi;
			pdi = pd.insert(pdi, ParamDesc(ParamDesc::type_string, ParamDesc::editable_readonly, parnrgyrodevserial, "Gyro Serial", "Gyro Sensor Device Serial Number", ""));
			++pdi;
			pdi = pd.insert(pdi, ParamDesc(ParamDesc::type_string, ParamDesc::editable_readonly, parnrmagdevname, "Mag Device Name", "Magnetic Field Sensor Device Name", ""));
			++pdi;
			pdi = pd.insert(pdi, ParamDesc(ParamDesc::type_string, ParamDesc::editable_readonly, parnrmagdevmfg, "Mag Manufacturer", "Magnetic Field Sensor Device Manufacturer", ""));
			++pdi;
			pdi = pd.insert(pdi, ParamDesc(ParamDesc::type_string, ParamDesc::editable_readonly, parnrmagdevdesc, "Mag Description", "Magnetic Field Sensor Device Description", ""));
			++pdi;
			pdi = pd.insert(pdi, ParamDesc(ParamDesc::type_string, ParamDesc::editable_readonly, parnrmagdevserial, "Mag Serial", "Magnetic Field Sensor Device Serial Number", ""));
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

Sensors::SensorAttitudeIIO::paramfail_t Sensors::SensorAttitudeIIO::get_param(unsigned int nr, Glib::ustring& v) const
{
	switch (nr) {
	default:
		break;

	case parnracceldevname:
	{
		v.clear();
		if (m_devnum[devtyp_accel] == -1)
			return paramfail_fail;

		return paramfail_ok;
	}

	case parnracceldevmfg:
	{
		v.clear();
		if (m_devnum[devtyp_accel] == -1)
			return paramfail_fail;

		return paramfail_ok;
	}

	case parnracceldevdesc:
	{
		v.clear();
		if (m_devnum[devtyp_accel] == -1)
			return paramfail_fail;

		return paramfail_ok;
	}

	case parnracceldevserial:
	{
		v.clear();
		if (m_devnum[devtyp_accel] == -1)
			return paramfail_fail;

		return paramfail_ok;
	}

	case parnrgyrodevname:
	{
		v.clear();
		if (m_devnum[devtyp_gyro] == -1)
			return paramfail_fail;

		return paramfail_ok;
	}

	case parnrgyrodevmfg:
	{
		v.clear();
		if (m_devnum[devtyp_gyro] == -1)
			return paramfail_fail;

		return paramfail_ok;
	}

	case parnrgyrodevdesc:
	{
		v.clear();
		if (m_devnum[devtyp_gyro] == -1)
			return paramfail_fail;

		return paramfail_ok;
	}

	case parnrgyrodevserial:
	{
		v.clear();
		if (m_devnum[devtyp_gyro] == -1)
			return paramfail_fail;

		return paramfail_ok;
	}

	case parnrmagdevname:
	{
		v.clear();
		if (m_devnum[devtyp_mag] == -1)
			return paramfail_fail;

		return paramfail_ok;
	}

	case parnrmagdevmfg:
	{
		v.clear();
		if (m_devnum[devtyp_mag] == -1)
			return paramfail_fail;

		return paramfail_ok;
	}

	case parnrmagdevdesc:
	{
		v.clear();
		if (m_devnum[devtyp_mag] == -1)
			return paramfail_fail;

		return paramfail_ok;
	}

	case parnrmagdevserial:
	{
		v.clear();
		if (m_devnum[devtyp_mag] == -1)
			return paramfail_fail;

		return paramfail_ok;
	}

	}
	return SensorAttitude::get_param(nr, v);
}

Sensors::SensorAttitudeIIO::paramfail_t Sensors::SensorAttitudeIIO::get_param(unsigned int nr, int& v) const
{
	switch (nr) {
	default:
		break;

	case parnrmode:
	{
		v = 1;
		AHRS::mode_t m(m_ahrs.get_mode());
		for (int i = 0; i < (int)(sizeof(ahrsmodemap)/sizeof(ahrsmodemap[0])); ++i) {
			if (ahrsmodemap[i] != m)
				continue;
			v = i;
			break;
		}
		return get_paramfail_open();
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
		v = m_rawmag[nr - parnrrawsamplemag];
		return get_paramfail_open();

	case parnrorientcoarseadjustroll:
	case parnrorientfineadjustroll:
	case parnrorientcoarseadjustpitch:
	case parnrorientfineadjustpitch:
	case parnrorientcoarseadjustheading:
	case parnrorientfineadjustheading:
	case parnrorienterectdefault:
		v = 0;
		return get_paramfail_open();
	}
	return SensorAttitude::get_param(nr, v);
}

Sensors::SensorAttitudeIIO::paramfail_t Sensors::SensorAttitudeIIO::get_param(unsigned int nr, double& v) const
{
	switch (nr) {
	default:
		break;

	case parnrcoeffla:
		v = m_ahrs.get_coeff_la();
		return get_paramfail_open();

	case parnrcoefflc:
		v = m_ahrs.get_coeff_lc();
		return get_paramfail_open();

	case parnrcoeffld:
		v = m_ahrs.get_coeff_ld();
		return get_paramfail_open();

	case parnrcoeffsigma:
		v = m_ahrs.get_coeff_sigma();
		return get_paramfail_open();

	case parnrcoeffn:
		v = m_ahrs.get_coeff_n();
		return get_paramfail_open();

	case parnrcoeffo:
		v = m_ahrs.get_coeff_o();
		return get_paramfail_open();

	case parnrattituderoll:
	case parnrattitudepitch:
	case parnrattitudeheading:
	{
		Eigen::Quaterniond q(m_ahrs.get_sensororientation() * m_ahrs.get_attitude().conjugate());
		AHRS::EulerAnglesd ea(AHRS::to_euler(q));
		v = ea[nr - parnrattitude] * (180.0 / M_PI);
		return get_paramfail();
	}

	case parnrorientationroll:
	case parnrorientationpitch:
	case parnrorientationheading:
	{
		AHRS::EulerAnglesd ea(AHRS::to_euler(m_ahrs.get_sensororientation()));
		v = ea[nr - parnrorientation] * (180.0 / M_PI);
		return get_paramfail_open();
	}

	case parnrorientationdeviation:
		v = m_ahrs.get_magdeviation();
		return get_paramfail_open();

	case parnrrawgyrox:
	case parnrrawgyroy:
	case parnrrawgyroz:
		v = m_ahrs.get_gyroscope()(nr - parnrrawgyro);
		return get_paramfail();

	case parnrrawaccelx:
	case parnrrawaccely:
	case parnrrawaccelz:
	     	v = m_ahrs.get_accelerometer()(nr - parnrrawaccel);
	     	return get_paramfail();

	case parnrrawmagx:
	case parnrrawmagy:
	case parnrrawmagz:
		v = m_ahrs.get_magnetometer()(nr - parnrrawmag);
		return get_paramfail();

	case parnrattitudequatx:
		v = m_ahrs.get_attitude().x();
		return get_paramfail();

	case parnrattitudequaty:
		v = m_ahrs.get_attitude().y();
		return get_paramfail();

	case parnrattitudequatz:
		v = m_ahrs.get_attitude().z();
		return get_paramfail();

	case parnrattitudequatw:
		v = m_ahrs.get_attitude().w();
		return get_paramfail();

	case parnrorientationquatx:
		v = m_ahrs.get_sensororientation().x();
		return get_paramfail_open();

	case parnrorientationquaty:
		v = m_ahrs.get_sensororientation().y();
		return get_paramfail_open();

	case parnrorientationquatz:
		v = m_ahrs.get_sensororientation().z();
		return get_paramfail_open();

	case parnrorientationquatw:
		v = m_ahrs.get_sensororientation().w();
		return get_paramfail_open();

	case parnrcalgyrobiasx:
	case parnrcalgyrobiasy:
	case parnrcalgyrobiasz:
	     	v = m_ahrs.get_gyrobias()(nr - parnrcalgyrobias);
	     	return get_paramfail();

	case parnrcalgyroscalex:
	case parnrcalgyroscaley:
	case parnrcalgyroscalez:
	     	v = m_ahrs.get_gyroscale()(nr - parnrcalgyroscale);
	     	return get_paramfail_open();

	case parnrcalaccelbiasx:
	case parnrcalaccelbiasy:
	case parnrcalaccelbiasz:
	     	v = m_ahrs.get_accelbias()(nr - parnrcalaccelbias);
	     	return get_paramfail_open();

	case parnrcalaccelscalex:
	case parnrcalaccelscaley:
	case parnrcalaccelscalez:
	     	v = m_ahrs.get_accelscale()(nr - parnrcalaccelscale);
	     	return get_paramfail_open();

	case parnrcalmagbiasx:
	case parnrcalmagbiasy:
	case parnrcalmagbiasz:
	     	v = m_ahrs.get_magbias()(nr - parnrcalmagbias);
	     	return get_paramfail_open();

	case parnrcalmagscalex:
	case parnrcalmagscaley:
	case parnrcalmagscalez:
	     	v = m_ahrs.get_magscale()(nr - parnrcalmagscale);
	     	return get_paramfail_open();

	case parnrcalaccelscale2:
	     	v = m_ahrs.get_accelscale2();
	     	return get_paramfail();

	case parnrcalmagscale2:
		v = m_ahrs.get_magscale2();
		return get_paramfail();
		
	case parnrrawsamplemagx:
	case parnrrawsamplemagy:
	case parnrrawsamplemagz:
		v = m_rawmag[nr - parnrrawsamplemag];
		return get_paramfail_open();
	}
	return SensorAttitude::get_param(nr, v);
}

Sensors::SensorAttitudeIIO::paramfail_t Sensors::SensorAttitudeIIO::get_paramfail_open(void) const
{
	return is_open() ? paramfail_ok : paramfail_fail;
}

Sensors::SensorAttitudeIIO::paramfail_t Sensors::SensorAttitudeIIO::get_paramfail(void) const
{
	if (!is_open())
		return paramfail_fail;
	Glib::TimeVal tv;
        tv.assign_current_time();
	tv.subtract(m_attitudetime);
	tv.add_seconds(-1);
	return tv.negative() ? paramfail_ok : paramfail_fail;
}

Sensors::SensorAttitudeIIO::paramfail_t Sensors::SensorAttitudeIIO::get_param(unsigned int nr, double& pitch, double& bank, double& slip, double& hdg, double& rate) const
{
	update_values();
	pitch = m_pitch;
	bank = m_bank;
	slip = m_slip;
	hdg = m_hdg;
	rate = m_rate;
	if (!is_open() || std::isnan(m_pitch) || std::isnan(m_bank) || std::isnan(m_slip) || std::isnan(m_hdg) || std::isnan(m_rate))
		return paramfail_fail;
	return get_paramfail();
}

void Sensors::SensorAttitudeIIO::set_param(unsigned int nr, const Glib::ustring& v)
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

void Sensors::SensorAttitudeIIO::set_param(unsigned int nr, int v)
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
		if (v1 >= 0 && v1 < (int)(sizeof(ahrsmodemap)/sizeof(ahrsmodemap[0])))
			m_ahrs.set_mode(ahrsmodemap[v1]);
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
			Eigen::Quaterniond q1(m_ahrs.get_sensororientation());
			q1 = q * q1;
			m_ahrs.set_sensororientation(q1);
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
			Eigen::Quaterniond q(m_ahrs.get_attitude());
			q.normalize();
			m_ahrs.set_sensororientation(q);
		}
		if (v & 0x02) {
			// Default
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

void Sensors::SensorAttitudeIIO::set_param(unsigned int nr, double v)
{
	switch (nr) {
	default:
		SensorAttitude::set_param(nr, v);
		return;

	case parnrcoeffla:
		m_ahrs.set_coeff_la(v);
		break;

	case parnrcoefflc:
		m_ahrs.set_coeff_lc(v);
		break;

	case parnrcoeffld:
		m_ahrs.set_coeff_ld(v);
		break;

	case parnrcoeffsigma:
		m_ahrs.set_coeff_sigma(v);
		break;

	case parnrcoeffn:
		m_ahrs.set_coeff_n(v);
		break;

	case parnrcoeffo:
		m_ahrs.set_coeff_o(v);
		break;

	case parnrorientationdeviation:
		while (v < 0)
			v += 360;
		while (v >= 360)
			v -= 360;
		m_ahrs.set_magdeviation(v);
		break;

	case parnrorientationquatx:
	{
		Eigen::Quaterniond q(m_ahrs.get_sensororientation());
		q.x() = v;
		q.normalize();
		m_ahrs.set_sensororientation(q);
		ParamChanged pc;
		pc.set_changed(parnrorientationquatx, parnrorientationquatz);
		update(pc);
		return;
	}

	case parnrorientationquaty:
	{
		Eigen::Quaterniond q(m_ahrs.get_sensororientation());
		q.y() = v;
		q.normalize();
		m_ahrs.set_sensororientation(q);
		ParamChanged pc;
		pc.set_changed(parnrorientationquatx, parnrorientationquatz);
		update(pc);
		return;
	}

	case parnrorientationquatz:
	{
		Eigen::Quaterniond q(m_ahrs.get_sensororientation());
		q.z() = v;
		q.normalize();
		m_ahrs.set_sensororientation(q);
		ParamChanged pc;
		pc.set_changed(parnrorientationquatx, parnrorientationquatz);
		update(pc);
		return;
	}

	case parnrorientationquatw:
	{
		Eigen::Quaterniond q(m_ahrs.get_sensororientation());
		q.w() = v;
		q.normalize();
		m_ahrs.set_sensororientation(q);
		ParamChanged pc;
		pc.set_changed(parnrorientationquatx, parnrorientationquatz);
		update(pc);
		return;
	}

	case parnrcalgyroscalex:
	case parnrcalgyroscaley:
	case parnrcalgyroscalez:
	{
		Eigen::Vector3d x(m_ahrs.get_gyroscale());
	     	x(nr - parnrcalgyroscale) = v;
		m_ahrs.set_gyroscale(x);
		break;
	}

	case parnrcalaccelbiasx:
	case parnrcalaccelbiasy:
	case parnrcalaccelbiasz:
	{
		Eigen::Vector3d x(m_ahrs.get_accelbias());
	     	x(nr - parnrcalaccelbias) = v;
		m_ahrs.set_accelbias(x);
		break;
	}

	case parnrcalaccelscalex:
	case parnrcalaccelscaley:
	case parnrcalaccelscalez:
	{
		Eigen::Vector3d x(m_ahrs.get_accelscale());
	     	x(nr - parnrcalaccelscale) = v;
		m_ahrs.set_accelscale(x);
		break;
	}

	case parnrcalmagbiasx:
	case parnrcalmagbiasy:
	case parnrcalmagbiasz:
	{
		Eigen::Vector3d x(m_ahrs.get_magbias());
	     	x(nr - parnrcalmagbias) = v;
		m_ahrs.set_magbias(x);
		break;
	}

	case parnrcalmagscalex:
	case parnrcalmagscaley:
	case parnrcalmagscalez:
	{
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

std::string Sensors::SensorAttitudeIIO::logfile_header(void) const
{
	return SensorAttitude::logfile_header() + ",GyroX,GyroY,GyroZ,AccelX,AccelY,AccelZ,MagX,MagY,MagZ,AttX,AttY,AttZ,AttW,GyroBX,GyroBY,GyroBZ,Accel2,Mag2,RawMagX,RawMagY,RawMagZ,Mode";
}

std::string Sensors::SensorAttitudeIIO::logfile_records(void) const
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
