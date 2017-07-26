//
// C++ Implementation: Attitude Sensor: psmove
//
// Description: Attitude Sensor Handling
//
//
// Author: Thomas Sailer <t.sailer@alumni.ethz.ch>, (C) 2012, 2013
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

#define _GNU_SOURCE 1

#include "sysdeps.h"

#include <iostream>
#include <iomanip>
#include <errno.h>

#include "sensattpsmove.h"

#ifdef HAVE_LINUX_USB_CH9_H
#include <linux/usb/ch9.h>
#endif

#ifdef HAVE_BLUEZ
#include <sys/ioctl.h>
#include <bluetooth/l2cap.h>
#include <bluetooth/hci.h>
#include <bluetooth/hci_lib.h>
#include <Eigen/Dense>
#include <Eigen/Geometry>
#endif

constexpr double Sensors::SensorAttitudePSMove::samplerate;
const unsigned short Sensors::SensorAttitudePSMove::PSMOVE_VID;
const unsigned short Sensors::SensorAttitudePSMove::PSMOVE_PID;
const unsigned short Sensors::SensorAttitudePSMove::SIXAXIS_VID;
const unsigned short Sensors::SensorAttitudePSMove::SIXAXIS_PID;

Sensors::SensorAttitudePSMove::SensorAttitudePSMove(Sensors& sensors, const Glib::ustring& configgroup)
	: SensorAttitude(sensors, configgroup),
	  m_ahrs(samplerate, "", AHRS::init_psmove), 
	  m_pitch(std::numeric_limits<double>::quiet_NaN()),
	  m_bank(std::numeric_limits<double>::quiet_NaN()),
	  m_slip(std::numeric_limits<double>::quiet_NaN()),
	  m_rate(std::numeric_limits<double>::quiet_NaN()),
	  m_hdg(std::numeric_limits<double>::quiet_NaN()),
	  m_dirty(false), m_devopen(false), m_samplecount(0), m_seq(-1),
	  m_attitudeprio(0), m_headingprio(0)
{
	const Glib::KeyFile& cf(get_sensors().get_configfile());
	if (!cf.has_key(get_configgroup(), "serial"))
		get_sensors().get_configfile().set_string(get_configgroup(), "serial", "");
	m_serial = cf.get_string(get_configgroup(), "serial");
	if (!cf.has_key(get_configgroup(), "attitudepriority"))
		get_sensors().get_configfile().set_uint64(get_configgroup(), "attitudepriority", m_attitudeprio);
	m_attitudeprio = cf.get_uint64(get_configgroup(), "attitudepriority");
	if (!cf.has_key(get_configgroup(), "headingpriority"))
		get_sensors().get_configfile().set_uint64(get_configgroup(), "headingpriority", m_headingprio);
	m_headingprio = cf.get_uint64(get_configgroup(), "headingpriority");
	m_rawgyro[0] = m_rawgyro[1] = m_rawgyro[2] = m_rawaccel[0] = m_rawaccel[1] = m_rawaccel[2] = m_rawmag[0] = m_rawmag[1] = m_rawmag[2] = 0;
}

Sensors::SensorAttitudePSMove::~SensorAttitudePSMove()
{
	// prevent calling the update event handler, which would call into get_params
	close_internal();
}

bool Sensors::SensorAttitudePSMove::open(const uint8_t *rep, unsigned int replen, const Glib::ustring& id)
{
	if (!rep || replen < 1)
		return false;
	const Glib::ustring full_id("bluetooth_" + id);
	if (!m_serial.empty() && full_id != m_serial)
		return false;
	{
                // Distinguish SIXAXIS / DS3 / PSMOVE.
                if (replen < 19) {
			close();
			m_devopen = true;
			m_ahrs.set_samplerate(samplerate);
			m_ahrs.set_id(full_id);
			if (!m_ahrs.init(AHRS::init_psmove))
				m_ahrs.load(get_sensors().get_configfile(), get_configgroup());
			if (id.empty())
				get_sensors().log(Sensors::loglevel_warning, "PSMove: connect");
			else
				get_sensors().log(Sensors::loglevel_warning, "PSMove: connect from " + id);
			{
				ParamChanged pc;
				pc.set_changed(parnrid);
				pc.set_changed(parnrmode, parnrpair);
				update(pc);
			}
			return true;
                }
                if (rep[13] == 0x40) {
                        // sixaxis
                } else {
                        // ds3
                }
        }
	get_sensors().log(Sensors::loglevel_warning, "PSMove: unrecognized device");
	return false;
}

bool Sensors::SensorAttitudePSMove::open(uint32_t vndid, uint32_t prodid, const Glib::ustring& id)
{
	const Glib::ustring full_id("bluetooth_" + id);
	if (!m_serial.empty() && full_id != m_serial)
		return false;
	if (vndid == SIXAXIS_VID && prodid == SIXAXIS_PID) {
		get_sensors().log(Sensors::loglevel_notice, "PSMove: sixaxis not supported yet");
		return false;
	}
	if (vndid != PSMOVE_VID || prodid != PSMOVE_PID) {
		std::ostringstream oss;
		oss << "PSMove: unrecognized device " << std::hex << std::setw(4) << std::setfill('0') << vndid
		    << ':' << std::setw(4) << std::setfill('0') << prodid;
		get_sensors().log(Sensors::loglevel_notice, oss.str());
		return 0;
	}
	close();
	m_devopen = true;
	m_ahrs.set_samplerate(samplerate);
	m_ahrs.set_id(full_id);
	if (!m_ahrs.init(AHRS::init_psmove))
		m_ahrs.load(get_sensors().get_configfile(), get_configgroup());
	if (id.empty())
		get_sensors().log(Sensors::loglevel_warning, "PSMove: connect");
	else
		get_sensors().log(Sensors::loglevel_warning, "PSMove: connect from " + id);
	{
		ParamChanged pc;
		pc.set_changed(parnrid);
		pc.set_changed(parnrmode, parnrpair);
		update(pc);
	}
	return true;
}

bool Sensors::SensorAttitudePSMove::close_internal(void)
{
	bool wasopen(is_open());
	if (wasopen) {
		m_ahrs.save(get_sensors().get_configfile(), get_configgroup());
		m_ahrs.save();
	}
	m_ahrs.set_id("");
	m_devopen = false;
	m_attitudetime = Glib::TimeVal();
	m_attitudereporttime = Glib::TimeVal();
	m_pitch = m_bank = m_slip = m_rate = m_hdg = std::numeric_limits<double>::quiet_NaN();
	m_dirty = false;
	m_samplecount = 0;
	m_seq = -1;
	m_rawgyro[0] = m_rawgyro[1] = m_rawgyro[2] = m_rawaccel[0] = m_rawaccel[1] = m_rawaccel[2] = m_rawmag[0] = m_rawmag[1] = m_rawmag[2] = 0;
	return wasopen;
}

void Sensors::SensorAttitudePSMove::close(void)
{
	if (close_internal()) {
		ParamChanged pc;
		pc.set_changed(parnrid);
		pc.set_changed(parnrmode, parnrpair);
		update(pc);
	}
}

void Sensors::SensorAttitudePSMove::handle_report(const uint8_t *report, int length)
{
	if (!is_open() || !report || length < 49 || report[0] != 0x01)
		return;
	// Report Sampling Rate is 87.8Hz
	unsigned int seq = report[4] & 15;
	{
		bool buttonsquare = !!(report[2] & 0x80);
		bool buttontriangle = !!(report[2] & 0x10);
		bool buttoncross = !!(report[2] & 0x40);
		bool buttoncircle = !!(report[2] & 0x20);
		bool buttonstart = !!(report[1] & 0x08);
		bool buttonselect = !!(report[1] & 0x01);
		bool buttonlambda = !!(report[3] & 0x08);
		bool buttonps = !!(report[3] & 0x01);
		bool buttontrigger = !!(report[3] & 0x10);
		uint8_t valtrigger = report[5];
		uint32_t btnmask(0);
		if (buttonsquare)
			btnmask |= 1 << 0;
		if (buttontriangle)
			btnmask |= 1 << 1;
		if (buttoncross)
			btnmask |= 1 << 2;
		if (buttoncircle)
			btnmask |= 1 << 3;
		if (buttonstart)
			btnmask |= 1 << 4;
		if (buttonselect)
			btnmask |= 1 << 5;
		if (buttonlambda)
			btnmask |= 1 << 6;
		if (buttonps)
			btnmask |= 1 << 7;
		if (buttontrigger)
			btnmask |= 1 << 8;
		//set_button(9, &btnmask);
		//set_analog(0, valtrigger << 7);		
	}
	int16_t accel[2][3];
	int16_t gyro[2][3];
	for (int i = 0; i < 2; ++i) {
		for (int j = 0; j < 3; ++j) {
			accel[i][j] = 32768 - ((report[14 + 6 * i + 2 * j] << 8) | report[13 + 6 * i + 2 * j]);
			gyro[i][j] = ((report[26 + 6 * i + 2 * j] << 8) | report[25 + 6 * i + 2 * j]) - 32768;
		}
	}
	int16_t mag[3];
	mag[0] = (report[38] << 12) | (report[39] << 4);
	mag[1] = (report[40] << 8)  | (report[41] & 0xf0);
	mag[2] = (report[41] << 12) | (report[42] << 4);
	mag[0] = -mag[0];
	mag[2] = -mag[2];
	set_navigation((m_seq == -1) ? 1 : ((seq - m_seq) * 2 - 1) & 31, accel[0], gyro[0], mag);
	set_navigation(1, accel[1], gyro[1], mag);
	m_seq = seq;
	m_attitudetime.assign_current_time();
	m_dirty = true;
	{
		Glib::TimeVal tv(m_attitudetime);
		tv.subtract(m_attitudereporttime);
		tv.subtract_milliseconds(50);
		if (!m_attitudereporttime.valid() || !tv.negative()) {
			m_attitudereporttime = m_attitudetime;
			ParamChanged pc;
			pc.set_changed(parnrattituderoll, parnrattitudeheading);
			pc.set_changed(parnrrawgyrox, parnrattitudequatw);
			pc.set_changed(parnrcalgyrobiasx, parnrcalgyrobiasz);
			pc.set_changed(parnrcalaccelscale2, parnrgyro);
			pc.set_changed(parnrrawsamplemagx, parnrrawsamplemagz);
			update(pc);
		}
	}
}

void Sensors::SensorAttitudePSMove::set_navigation(unsigned int deltasamples, int16_t accel[3], int16_t gyro[3], int16_t mag[3])
{
        m_samplecount += deltasamples;
	m_rawgyro[0] = gyro[0];
	m_rawgyro[1] = gyro[1];
	m_rawgyro[2] = gyro[2];
	m_rawaccel[0] = accel[0];
	m_rawaccel[1] = accel[1];
	m_rawaccel[2] = accel[2];
	m_rawmag[0] = mag[0];
	m_rawmag[1] = mag[1];
	m_rawmag[2] = mag[2];
        m_ahrs.new_samples(deltasamples, gyro, accel, mag);
        if (false) {
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
}

void Sensors::SensorAttitudePSMove::update_values(void) const
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
	
void Sensors::SensorAttitudePSMove::get_heading(double& hdg) const
{
	update_values();
	hdg = m_hdg;
}

unsigned int Sensors::SensorAttitudePSMove::get_heading_priority(void) const
{
	return m_headingprio;
}

Glib::TimeVal Sensors::SensorAttitudePSMove::get_heading_time(void) const
{
	return m_attitudetime;
}

bool Sensors::SensorAttitudePSMove::is_heading_true(void) const
{
	return false;
}

bool Sensors::SensorAttitudePSMove::is_heading_ok(const Glib::TimeVal& tv) const
{
	return is_open() && SensorAttitude::is_heading_ok(tv);
}

void Sensors::SensorAttitudePSMove::get_attitude(double& pitch, double& bank, double& slip, double& rate) const
{
	update_values();
	pitch = m_pitch;
	bank = m_bank;
	slip = m_slip;
	rate = m_rate;
}

unsigned int Sensors::SensorAttitudePSMove::get_attitude_priority(void) const
{
	return m_attitudeprio;
}

Glib::TimeVal Sensors::SensorAttitudePSMove::get_attitude_time(void) const
{
	return m_attitudetime;
}

bool Sensors::SensorAttitudePSMove::is_attitude_ok(const Glib::TimeVal& tv) const
{
	return is_open() && SensorAttitude::is_attitude_ok(tv);
}

const AHRS::mode_t  Sensors::SensorAttitudePSMove::ahrsmodemap[4] = {
	AHRS::mode_slow, AHRS::mode_normal, AHRS::mode_fast, AHRS::mode_gyroonly
};

void Sensors::SensorAttitudePSMove::get_param_desc(unsigned int pagenr, paramdesc_t& pd)
{
	SensorAttitude::get_param_desc(pagenr, pd);

	switch (pagenr) {
	default:
		pd.push_back(ParamDesc(ParamDesc::type_string, ParamDesc::editable_readonly, parnrid, "Current Serial", "Current Device Serial Number", ""));
		pd.push_back(ParamDesc(ParamDesc::type_string, ParamDesc::editable_admin, parnrserial, "Required Serial", "Constrain to Device Serial Number", ""));
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
		{
			static const Glib::ustring bname("$gtk-refresh");
			pd.push_back(ParamDesc(ParamDesc::type_button, ParamDesc::editable_admin, parnrpair, "Fetch Calibration", "Reads the calibration from the sensor non-volatile memory; requires the sensor to be connected via USB", "", &bname, &bname + 1));
		}

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

Sensors::SensorAttitudePSMove::paramfail_t Sensors::SensorAttitudePSMove::get_param(unsigned int nr, Glib::ustring& v) const
{
	switch (nr) {
	default:
		break;

	case parnrid:
		v = m_ahrs.get_id();
		return paramfail_ok;

	case parnrserial:
		v = m_serial;
		return paramfail_ok;
	}
	return SensorAttitude::get_param(nr, v);
}

Sensors::SensorAttitudePSMove::paramfail_t Sensors::SensorAttitudePSMove::get_param(unsigned int nr, int& v) const
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
	case parnrpair:
		v = 0;
		return get_paramfail_open();
	}
	return SensorAttitude::get_param(nr, v);
}

Sensors::SensorAttitudePSMove::paramfail_t Sensors::SensorAttitudePSMove::get_param(unsigned int nr, double& v) const
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

Sensors::SensorAttitudePSMove::paramfail_t Sensors::SensorAttitudePSMove::get_paramfail_open(void) const
{
	return is_open() ? paramfail_ok : paramfail_fail;
}

Sensors::SensorAttitudePSMove::paramfail_t Sensors::SensorAttitudePSMove::get_paramfail(void) const
{
	if (!is_open())
		return paramfail_fail;
	Glib::TimeVal tv;
        tv.assign_current_time();
	tv.subtract(m_attitudetime);
	tv.add_seconds(-1);
	return tv.negative() ? paramfail_ok : paramfail_fail;
}

Sensors::SensorAttitudePSMove::paramfail_t Sensors::SensorAttitudePSMove::get_param(unsigned int nr, double& pitch, double& bank, double& slip, double& hdg, double& rate) const
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

void Sensors::SensorAttitudePSMove::set_param(unsigned int nr, const Glib::ustring& v)
{
	switch (nr) {
	default:
		SensorAttitude::set_param(nr, v);
		return;

	case parnrserial:
		if (v == m_serial)
			return;
		m_serial = v;
		get_sensors().get_configfile().set_string(get_configgroup(), "serial", m_serial);
		break;
	}
	ParamChanged pc;
	pc.set_changed(nr);
	update(pc);
}

void Sensors::SensorAttitudePSMove::set_param(unsigned int nr, int v)
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

	case parnrpair:
		if (!(v & 0x01))
			break;
		pair_scan();
		break;
	}
	ParamChanged pc;
	pc.set_changed(nr);
	update(pc);
}

void Sensors::SensorAttitudePSMove::set_param(unsigned int nr, double v)
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

std::string Sensors::SensorAttitudePSMove::logfile_header(void) const
{
	return SensorAttitude::logfile_header() + ",GyroX,GyroY,GyroZ,AccelX,AccelY,AccelZ,MagX,MagY,MagZ,AttX,AttY,AttZ,AttW,GyroBX,GyroBY,GyroBZ,Accel2,Mag2,RawMagX,RawMagY,RawMagZ,Mode";
}

std::string Sensors::SensorAttitudePSMove::logfile_records(void) const
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
	oss << ',' << m_rawmag[0] << ',' << m_rawmag[1] << ',' << m_rawmag[2] << ',' << AHRS::to_str(m_ahrs.get_mode());
	return oss.str();
}

void Sensors::SensorAttitudePSMove::libusb_perror(int err, const std::string& txt)
{
#ifdef HAVE_LIBUSB1
	if (err == LIBUSB_SUCCESS)
		return;
        std::string t(txt + ":");
	switch (err) {
	case LIBUSB_ERROR_IO:
		t += "I/O Error";
		break;

	case LIBUSB_ERROR_INVALID_PARAM:
		t += "Invalid Parameter";
		break;

	case LIBUSB_ERROR_ACCESS:
		t += "Access Denied";
		break;

	case LIBUSB_ERROR_NO_DEVICE:
		t += "No Device";
		break;

	case LIBUSB_ERROR_NOT_FOUND:
		t += "Not Found";
		break;

	case LIBUSB_ERROR_BUSY:
		t += "Busy";
		break;

	case LIBUSB_ERROR_TIMEOUT:
		t += "Timeout";
		break;

	case LIBUSB_ERROR_OVERFLOW:
		t += "Overflow";
		break;

	case LIBUSB_ERROR_PIPE:
		t += "Pipe";
		break;

	case LIBUSB_ERROR_INTERRUPTED:
		t += "Interrupted";
		break;

	case LIBUSB_ERROR_NO_MEM:
		t += "No Memory";
		break;

	case LIBUSB_ERROR_NOT_SUPPORTED:
		t += "Not Supported";
		break;

	case LIBUSB_ERROR_OTHER:
		t += "Other";
		break;

	default:
	{
		std::ostringstream oss;
		oss << "Unknown Error " << err;
		t += oss.str();
		break;
	}
	}
        throw std::runtime_error(t);
#endif
}

bool Sensors::SensorAttitudePSMove::pair_find_btaddr(bdaddr_t& addr)
{
	bool found(false);
#ifdef HAVE_BLUEZ
	int sock = socket(AF_BLUETOOTH, SOCK_RAW, BTPROTO_HCI);
	if (sock < 0) {
		get_sensors().log(Sensors::loglevel_warning, "PSMove: pair: Cannot open bluetooth socket");
		return false;
	}
	uint8_t dlbuf[HCI_MAX_DEV * sizeof(struct hci_dev_req) + sizeof(struct hci_dev_list_req)];
	struct hci_dev_list_req *dl = (struct hci_dev_list_req *)dlbuf;
	struct hci_dev_req *dr = dl->dev_req;
	dl->dev_num = HCI_MAX_DEV;
	if (ioctl(sock, HCIGETDEVLIST, (void *)dl)) {
		::close(sock);
		get_sensors().log(Sensors::loglevel_warning, "PSMove: pair: Cannot get bluetooth device list");
		return false;
	}
	for (int i = 0; i < dl->dev_num; ++i, ++dr) {
		if (!hci_test_bit(HCI_UP, &dr->dev_opt))
			continue;
		int dd = hci_open_dev(dr->dev_id);
		if (dd >= 0) {
                       int err = hci_read_bd_addr(dd, &addr, 1000);
                        hci_close_dev(dd);
                        if (err >= 0) {
				found = true;
				break;
			}
		}
	}
	::close(sock);
#endif
	if (!found)
		get_sensors().log(Sensors::loglevel_warning, "PSMove: pair: Cannot get bluetooth address");
	return found;
}

void Sensors::SensorAttitudePSMove::pair_scan(void)
{
#ifdef HAVE_LIBUSB1
	struct libusb_context *context(0);
	libusb_perror(libusb_init(&context), "libusb_init");
	try {
		libusb_device **devlist;
		ssize_t nrdev(libusb_get_device_list(context, &devlist));
		if (nrdev < 0)
			libusb_perror(nrdev, "libusb_get_device_list");
		int dev = 0;
		for (; dev < nrdev; ++dev) {
			struct libusb_device_descriptor desc;
			if (libusb_get_device_descriptor(devlist[dev], &desc) != LIBUSB_SUCCESS)
				continue;
			try {
				if (desc.idVendor == PSMOVE_VID && desc.idProduct == PSMOVE_PID)
					pair_psmove(devlist[dev]);
				else if (desc.idVendor == SIXAXIS_VID && desc.idProduct == SIXAXIS_PID)
					pair_sixaxis_ds3(devlist[dev]);
			} catch(const std::exception& e) {
				get_sensors().log(Sensors::loglevel_warning, std::string("PSMove: pair: pairing failed: ") + e.what());
			}
		}
		libusb_free_device_list(devlist, 1);
	} catch(const std::exception& e) {
		get_sensors().log(Sensors::loglevel_warning, std::string("PSMove: pair: USB device scanning failed: ") + e.what());
	}
	if (context)
		libusb_exit(context);
#endif
}

#define USB_GET_REPORT 0x01
#define USB_SET_REPORT 0x09

#ifdef HAVE_LIBUSB1

void Sensors::SensorAttitudePSMove::pair_sixaxis_ds3(libusb_device *dev)
{
	struct libusb_device_handle *devh;
	libusb_perror(libusb_open(dev, &devh), "libusb_open");
	try {
		int cfg;
		libusb_perror(libusb_get_configuration(devh, &cfg), "libusb_get_configuration");
		struct libusb_config_descriptor *cfgdesc;
		libusb_perror(libusb_get_config_descriptor_by_value(dev, cfg, &cfgdesc), "libusb_get_config_descriptor_by_value");
		bool setaltsetting(false);
		unsigned int intf(~0U);
		unsigned int alts(~0U);
		unsigned int intfidx(~0U);
		for (unsigned int ifnr = 0; ifnr < cfgdesc->bNumInterfaces; ++ifnr) {
			const struct libusb_interface *intfp(cfgdesc->interface + ifnr);
			for (int altsidx = 0; altsidx < intfp->num_altsetting; ++altsidx) {
				const struct libusb_interface_descriptor *altsetting(intfp->altsetting + altsidx);
				if (altsetting->bInterfaceClass != 3)
					continue;
				intfidx = ifnr;
				intf = altsetting->bInterfaceNumber;
				alts = altsetting->bAlternateSetting;
				setaltsetting = intfp->num_altsetting > 1;
				break;
			}
			if (intf != ~0U)
				break;
		}
		libusb_free_config_descriptor(cfgdesc);
		if (intfidx == ~0U)
			throw std::runtime_error("No suitable USB interface found");
		int err(libusb_detach_kernel_driver(devh, intf));
		if (err != LIBUSB_ERROR_NOT_FOUND)
			libusb_perror(err, "libusb_detach_kernel_driver");
		libusb_perror(libusb_claim_interface(devh, intf), "libusb_claim_interface");
		if (setaltsetting)
			libusb_perror(libusb_set_interface_alt_setting(devh, intf, alts), "libusb_set_interface_alt_setting");
#ifdef HAVE_BLUEZ
		{
			uint8_t msg[8];
			err = libusb_control_transfer(devh, LIBUSB_ENDPOINT_IN | LIBUSB_REQUEST_TYPE_CLASS | LIBUSB_RECIPIENT_INTERFACE, USB_GET_REPORT, 0x03f5, intf, msg, sizeof(msg), 5000);
			if (err < 0)
				libusb_perror(err, "libusb_control_transfer (get)");
			if (err < (int)sizeof(msg))
				libusb_perror(err, "libusb_control_transfer (get): incomplete transfer");
			bdaddr_t currentba;
			for (unsigned int i = 0; i < sizeof(currentba.b)/sizeof(currentba.b[0]); ++i)
				currentba.b[i] = msg[7-i];
			bdaddr_t btaddr;
			if (!pair_find_btaddr(btaddr)) {
				char bdaddrold[24];
				ba2str(&currentba, bdaddrold);
				get_sensors().log(Sensors::loglevel_warning, "PSMove: pair: unable to find master bluetooth address, device address " + std::string(bdaddrold));
			} else if (bacmp(&currentba, &btaddr)) {
				{
					char bdaddrold[24], bdaddrnew[24];
					ba2str(&currentba, bdaddrold);
					ba2str(&btaddr, bdaddrnew);
					get_sensors().log(Sensors::loglevel_notice, "PSMove: pair: changing bluetooth address from " + std::string(bdaddrold) + " to " + std::string(bdaddrnew));
				}
				uint8_t msg[8] = { 0x01, 0x00, btaddr.b[5], btaddr.b[4], btaddr.b[3], btaddr.b[2], btaddr.b[1], btaddr.b[0] };
				err = libusb_control_transfer(devh, LIBUSB_ENDPOINT_OUT | LIBUSB_REQUEST_TYPE_CLASS | LIBUSB_RECIPIENT_INTERFACE, USB_SET_REPORT, 0x03f5, intf, msg, sizeof(msg), 5000);
				if (err < 0)
					libusb_perror(err, "libusb_control_transfer (set)");
				if (err < (int)sizeof(msg))
					libusb_perror(err, "libusb_control_transfer (set): incomplete transfer");
			} else {
				char bdaddrold[24];
				ba2str(&currentba, bdaddrold);
				get_sensors().log(Sensors::loglevel_notice, "PSMove: pair: already paired with bluetooth address " + std::string(bdaddrold));
			}
		}
		{
			uint8_t msg[17];
			err = libusb_control_transfer(devh, LIBUSB_ENDPOINT_IN | LIBUSB_REQUEST_TYPE_CLASS | LIBUSB_RECIPIENT_INTERFACE, USB_GET_REPORT, 0x03f2, intf, msg, sizeof(msg), 5000);
			if (err < 0)
				libusb_perror(err, "libusb_control_transfer (get)");
			if (err < /*sizeof(msg)*/10)
				libusb_perror(err, "libusb_control_transfer (get): incomplete transfer");
			bdaddr_t ownba;
			for (unsigned int i = 0; i < sizeof(ownba.b)/sizeof(ownba.b[0]); ++i)
				ownba.b[i] = msg[9-i];
			{
				char bdaddrown[24];
				ba2str(&ownba, bdaddrown);
				get_sensors().log(Sensors::loglevel_notice, "PSMove: pair: own bluetooth address " + std::string(bdaddrown));
			}
		}
#endif
		libusb_release_interface(devh, intf);
	} catch(const std::exception& e) {
		libusb_close(devh);
		throw;
	}
	libusb_close(devh);	
}

void Sensors::SensorAttitudePSMove::pair_psmove(libusb_device *dev)
{
	struct libusb_device_handle *devh;
	libusb_perror(libusb_open(dev, &devh), "libusb_open");
	try {
		int cfg;
		libusb_perror(libusb_get_configuration(devh, &cfg), "libusb_get_configuration");
		struct libusb_config_descriptor *cfgdesc;
		libusb_perror(libusb_get_config_descriptor_by_value(dev, cfg, &cfgdesc), "libusb_get_config_descriptor_by_value");
		bool setaltsetting(false);
		unsigned int intf(~0U);
		unsigned int alts(~0U);
		unsigned int intfidx(~0U);
		for (unsigned int ifnr = 0; ifnr < cfgdesc->bNumInterfaces; ++ifnr) {
			const struct libusb_interface *intfp(cfgdesc->interface + ifnr);
			for (int altsidx = 0; altsidx < intfp->num_altsetting; ++altsidx) {
				const struct libusb_interface_descriptor *altsetting(intfp->altsetting + altsidx);
				if (altsetting->bInterfaceClass != 3)
					continue;
				intfidx = ifnr;
				intf = altsetting->bInterfaceNumber;
				alts = altsetting->bAlternateSetting;
				setaltsetting = intfp->num_altsetting > 1;
				break;
			}
			if (intf != ~0U)
				break;
		}
		libusb_free_config_descriptor(cfgdesc);
		if (intfidx == ~0U)
			throw std::runtime_error("No suitable USB interface found");
		int err(libusb_detach_kernel_driver(devh, intf));
		if (err != LIBUSB_ERROR_NOT_FOUND)
			libusb_perror(err, "libusb_detach_kernel_driver");
		libusb_perror(libusb_claim_interface(devh, intf), "libusb_claim_interface");
		if (setaltsetting)
			libusb_perror(libusb_set_interface_alt_setting(devh, intf, alts), "libusb_set_interface_alt_setting");
#ifdef HAVE_BLUEZ
		char bdaddrown[24];
		bdaddrown[0] = 0;
		{
			uint8_t msg[16];
			err = libusb_control_transfer(devh, LIBUSB_ENDPOINT_IN | LIBUSB_REQUEST_TYPE_CLASS | LIBUSB_RECIPIENT_INTERFACE, USB_GET_REPORT, 0x0304, intfidx, msg, sizeof(msg), 5000);
			if (err < 0)
				libusb_perror(err, "libusb_control_transfer (get)");
			if (err < (int)sizeof(msg))
				libusb_perror(err, "libusb_control_transfer (get): incomplete transfer");
			bdaddr_t currentba;
			for (unsigned int i = 0; i < sizeof(currentba.b)/sizeof(currentba.b[0]); ++i)
				currentba.b[i] = msg[10+i];
			bdaddr_t ownba;
			for (unsigned int i = 0; i < sizeof(ownba.b)/sizeof(ownba.b[0]); ++i)
				ownba.b[i] = msg[1+i];
			ba2str(&ownba, bdaddrown);
			bdaddr_t btaddr;
			if (!pair_find_btaddr(btaddr)) {
				char bdaddrold[24];
				ba2str(&currentba, bdaddrold);
				get_sensors().log(Sensors::loglevel_warning, "PSMove: pair: unable to find master bluetooth address, device address " + std::string(bdaddrold));
			} else if (bacmp(&currentba, &btaddr)) {
				{
					char bdaddrold[24], bdaddrnew[24];
					ba2str(&currentba, bdaddrold);
					ba2str(&btaddr, bdaddrnew);
					get_sensors().log(Sensors::loglevel_notice, "PSMove: pair: changing bluetooth address from " + std::string(bdaddrold) + " to " + std::string(bdaddrnew));
				}
				uint8_t msg[11] = { 0x05, btaddr.b[0], btaddr.b[1], btaddr.b[2], btaddr.b[3],
						    btaddr.b[4], btaddr.b[5], 0x10, 0x01, 0x02, 0x12 };
				err = libusb_control_transfer(devh, LIBUSB_ENDPOINT_OUT | LIBUSB_REQUEST_TYPE_CLASS | LIBUSB_RECIPIENT_INTERFACE, USB_SET_REPORT, 0x0305, intfidx, msg, sizeof(msg), 5000);
				if (err < 0)
					libusb_perror(err, "libusb_control_transfer (set)");
				if (err < (int)sizeof(msg))
					libusb_perror(err, "libusb_control_transfer (set): incomplete transfer");
			} else {
				char bdaddrold[24];
				ba2str(&currentba, bdaddrold);
				get_sensors().log(Sensors::loglevel_notice, "PSMove: pair: already paired with bluetooth address " + std::string(bdaddrold));
			}
			get_sensors().log(Sensors::loglevel_notice, "PSMove: pair: own bluetooth address " + std::string(bdaddrown));
		}
		{
			uint8_t calreport[49+47+47];
			unsigned int parts(7);
			while (parts) {
				uint8_t msg[49];
				int err = libusb_control_transfer(devh, LIBUSB_ENDPOINT_IN | LIBUSB_REQUEST_TYPE_CLASS | LIBUSB_RECIPIENT_INTERFACE, USB_GET_REPORT, 0x310, intfidx, msg, sizeof(msg), 5000);
				if (err < (int)sizeof(msg)) {
					std::ostringstream oss;
					oss << "PSMove: pair: cannot read calib data, err = " << err;
					get_sensors().log(Sensors::loglevel_notice, oss.str());
					break;
				}
				uint8_t seq(msg[1] & 0x0f);
				if (!seq) {
					memcpy(calreport, msg, 49);
					parts &= ~1;
				} else if (seq <= 2) {
					memcpy(&calreport[2 + seq * 47], &msg[2], 47);
					parts &= ~(1 << seq);
				}
				if (false) {
					std::ostringstream oss;
					oss << "PSMove: Calib Report:" << std::hex;
					for (unsigned int i = 0; i < (unsigned int)err; ++i) {
						if (!(i & 15))
							oss << std::endl << std::setw(4) << std::setfill('0') << i << ':';
						oss << ' ' << std::setw(2) << std::setfill('0') << (unsigned int)msg[i];
					}
					std::cerr << oss.str() << std::endl;
				}
			}
			if (!parts) {
				if (false) {
					std::ostringstream oss;
					oss << "PSMove: Calib Report:" << std::hex;
					for (unsigned int i = 0; i < sizeof(calreport); ++i) {
						if (!(i & 15))
							oss << std::endl << std::setw(4) << std::setfill('0') << i << ':';
						oss << ' ' << std::setw(2) << std::setfill('0') << (unsigned int)calreport[i];
					}
					std::cerr << oss.str() << std::endl;
				}
				calib_psmove(bdaddrown, calreport);
			}
		}
#endif
		if (false) {
			for (uint16_t rnr = 0x300; rnr < 0x3ff; ++rnr) {
				uint8_t msg[64];
				err = libusb_control_transfer(devh, LIBUSB_ENDPOINT_IN | LIBUSB_REQUEST_TYPE_CLASS | LIBUSB_RECIPIENT_INTERFACE, USB_GET_REPORT, rnr, intfidx, msg, sizeof(msg), 5000);
				if (err <= 0)
					continue;
				std::ostringstream oss;
				oss << std::hex << "Report 0x" << std::setfill('0') << std::setw(4) << rnr;
				for (int i = 0; i < err; ++i) {
					if (!(i & 15))
						oss << std::endl << std::setfill('0') << std::setw(4) << i << ':';
					oss << ' ' << std::setfill('0') << std::setw(2) << (unsigned int)msg[i];
				}
				std::cout << oss.str() << std::endl;
			}
		}
		libusb_release_interface(devh, intf);
	} catch(const std::exception& e) {
		libusb_close(devh);
		throw;
	}
	libusb_close(devh);	
}

#endif

void Sensors::SensorAttitudePSMove::calib_psmove(const std::string& deviceid, const uint8_t calreport[49+47+47])
{
	const std::string full_devid("bluetooth_" + deviceid);
	if (m_ahrs.get_id() != full_devid && m_serial != full_devid && !(m_serial.empty() && m_ahrs.get_id().empty()))
		return;
	ParamChanged pc;
	pc.set_changed(parnrcalgyrobiasx, parnrcalgyrobiasz);
	pc.set_changed(parnrcalgyroscalex, parnrcalgyroscalez);
	pc.set_changed(parnrcalaccelbiasx, parnrcalaccelbiasz);
	pc.set_changed(parnrcalaccelscalex, parnrcalaccelscalez);
	if (m_serial.empty()) {
		m_serial = full_devid;
		get_sensors().get_configfile().set_string(get_configgroup(), "serial", m_serial);
		pc.set_changed(parnrserial);
	}

	// Parse the calibration data
	// I don't know what these headers are. Maybe temperatures?
	uint16_t AccHeader = calreport[2] | (calreport[3] << 8);
	int16_t AccVectors[6][3];
	{
		// convert to order minx,maxx,miny,maxy,minz,maxz
		static const uint8_t reorder[6] = { 1, 3, 2, 0, 5, 4 }; 
		// accelerometer readings for 6 different orientations
		for (int v = 0; v < 6; ++v) {
			int i = 4 + 6 * reorder[v];
			AccVectors[v][0] = (calreport[i+0] | (calreport[i+1] << 8)) - 32768;
			AccVectors[v][1] = (calreport[i+4] | (calreport[i+5] << 8)) - 32768;
			AccVectors[v][2] = (calreport[i+2] | (calreport[i+3] << 8)) - 32768;
		}
	}
	//this values have to be subtracted from the gyro values
	uint16_t GyroBiasHeaders[2];
	int16_t GyroBiasVectors[2][3];
	for (int i=0; i < 2; ++i) {
		GyroBiasHeaders[i] = calreport[40+i*8] | (calreport[41+i*8] << 8);
		GyroBiasVectors[i][0] = (calreport[42+i*8+0] | (calreport[42+i*8+1] << 8)) - 32768;
		GyroBiasVectors[i][1] = (calreport[42+i*8+4] | (calreport[42+i*8+5] << 8)) - 32768;
		GyroBiasVectors[i][2] = (calreport[42+i*8+2] | (calreport[42+i*8+3] << 8)) - 32768;
	}
	// from 56 to 64 are all zeros, 65 is 0x01, I don't know what (if anything) it means

	// this part is actually a matrix, or 3 vectors, in little-endian, with each one preceded by 0x01E0 (480)
	// the matrix contains the gyro readings at 8 rpm
	// to remove the bias, one of the bias vectors must be substracted
	uint16_t GyroHeader = calreport[66] | (calreport[67] << 8); // another strange header
	int16_t GyroVectors[3][3];
	{
		static const uint8_t reorder[3] = { 0, 2, 1 }; // convert to order x,y,z
		for (int v = 0; v < 3; ++v) {
			int i = 70 + 8 * reorder[v];
			GyroVectors[v][0] = (calreport[i+0] | (calreport[i+1] << 8)) - 32768;
			GyroVectors[v][1] = (calreport[i+4] | (calreport[i+5] << 8)) - 32768;
			GyroVectors[v][2] = (calreport[i+2] | (calreport[i+3] << 8)) - 32768;
		}
	}

	uint16_t UnknownHeader = calreport[92] | (calreport[93] << 8); // another strange header
	// from 92 to 126 there are 8 float values, 2 vectors and than 2 independent values
	float UnknownVectors[2][3];
	//the remaining 2 float values
	float UnknownValues[2];
	for (int i = 0; i < 2; ++i) {
		for (int j = 0; j < 3; ++j) {
			uint32_t v = calreport[94+i*12+j*4+0] | (calreport[94+i*12+j*4+1] << 8) | (calreport[94+i*12+j*4+2] << 16) | (calreport[94+i*12+j*4+3] << 24);
			UnknownVectors[i][j] = *reinterpret_cast<float *>(&v);
		}
		uint32_t v = calreport[118+i*4+0] | (calreport[118+i*4+1] << 8) | (calreport[118+i*4+2] << 16) | (calreport[118+i*4+3] << 24);
		UnknownValues[i] = *reinterpret_cast<float *>(&v);
	}
	if (true) {
		std::ostringstream oss;
		oss << "PSMove: Device " << deviceid << " Calib Data:" << std::endl
		    << "  Accelerometer: 0x" << std::hex << std::setw(4) << std::setfill('0') << AccHeader << std::dec << std::endl;
		for (int i = 0; i < 3; ++i)
			oss << "    " << (char)('X' + i) << " [ " << AccVectors[2*i][0] << ", " << AccVectors[2*i][1] << ", "  << AccVectors[2*i][2]
			    << " ], [ "  << AccVectors[2*i+1][0] << ", "  << AccVectors[2*i+1][1] << ", " << AccVectors[2*i+1][2] << " ]" << std::endl;
		oss << "  Gyro: 0x" << std::hex << std::setw(4) << std::setfill('0') << GyroHeader << std::dec << std::endl;
		for (int i = 0; i < 2; ++i)
			oss << "    Bias 0x" << std::hex << std::setw(4) << std::setfill('0') << GyroBiasHeaders[i] << std::dec
			    << " [ " << GyroBiasVectors[i][0] << ", " << GyroBiasVectors[i][1] << ", "  << GyroBiasVectors[i][2] << " ]" << std::endl;
		for (int i = 0; i < 3; ++i)
			oss << "    " << (char)('X' + i) << " [ " << GyroVectors[i][0] << ", " << GyroVectors[i][1] << ", "  << GyroVectors[i][2] << " ]" << std::endl;
		oss << "  Unknown: 0x" << std::hex << std::setw(4) << std::setfill('0') << UnknownHeader << std::dec << std::endl;
		for (int i = 0; i < 2; ++i)
			oss << "    ? " << UnknownValues[i]
			    << " [ " << UnknownVectors[i][0] << ", " << UnknownVectors[i][1] << ", "  << UnknownVectors[i][2] << " ]" << std::endl;
		std::cerr << oss.str();
	}
	// Gyro Least Squares Fit
	Eigen::Vector3d gyrobias(Eigen::Vector3d(GyroBiasVectors[0][0], GyroBiasVectors[0][1], GyroBiasVectors[0][2]) +
				 Eigen::Vector3d(GyroBiasVectors[1][0], GyroBiasVectors[1][1], GyroBiasVectors[1][2]));
	gyrobias = 0.5 * gyrobias.array() * Eigen::Vector3d(UnknownVectors[1][0], UnknownVectors[1][1], UnknownVectors[1][2]).array();
	if (false) {
		Eigen::Matrix<double, 9, 9> A(Eigen::Matrix<double, 9, 9>::Zero());
		Eigen::Matrix<double, 9, 1> b(Eigen::Matrix<double, 9, 1>::Zero());
		for (int i = 0; i < 3; ++i) {
			Eigen::Vector3d x(GyroVectors[i][0], GyroVectors[i][1], GyroVectors[i][2]);
			x -= gyrobias;
			for (int j = 0; j < 3; ++j)
				A.block<1,3>(i * 3 + j, j * 3) = x.transpose();
			// convert 8 RPM to 1 rotation/s (??)
			b(i * 3 + i) = 2 * M_PI / (2 * 0.75);
		}
		if (true)
			std::cerr << "A = [ " << A << " ]" << std::endl << "b = [ " << b << " ]" << std::endl;
		Eigen::Matrix<double, 9, 1> x((A.transpose() * A).inverse() * A.transpose() * b);
		Eigen::Matrix3d scale;
		for (int i = 0; i < 3; ++i)
			scale.block<1,3>(i, 0) = x.block<3,1>(i * 3, 0).transpose();
		if (true) {
			std::cerr << "Gyro Scale: [ " << scale << " ]" << std::endl
				  << "  Bias [ " << gyrobias.transpose() << " ]" << std::endl;
			for (int i = 0; i < 3; ++i) {
				Eigen::Vector3d x(GyroVectors[i][0], GyroVectors[i][1], GyroVectors[i][2]);
				std::cerr << "  [ " << x.transpose() << " ] -> [ " << (scale * (x - gyrobias)).transpose() << " ]" << std::endl;
			}
		}
		m_ahrs.set_gyrobias(scale.diagonal().array() * gyrobias.array());
		m_ahrs.set_gyroscale(scale.diagonal());
	} else {
		Eigen::Matrix<double, 3, 3> A(Eigen::Matrix<double, 3, 3>::Zero());
		Eigen::Matrix<double, 3, 1> b(Eigen::Matrix<double, 3, 1>::Zero());
		for (int i = 0; i < 3; ++i) {
			Eigen::Vector3d x(GyroVectors[i][0], GyroVectors[i][1], GyroVectors[i][2]);
			x -= gyrobias;
			A.block<1,3>(i, 0) = x.transpose();
			// convert 8 RPM to 1 rotation/s (??)
			b(i) = 2 * M_PI / (2 * 0.75);
		}
		if (true)
			std::cerr << "A = [ " << A << " ]" << std::endl << "b = [ " << b << " ]" << std::endl;
		Eigen::Vector3d scale((A.transpose() * A).inverse() * A.transpose() * b);
		if (true) {
			std::cerr << "Gyro Scale: [ " << scale.transpose() << " ]" << std::endl
				  << "  Bias [ " << gyrobias.transpose() << " ]" << std::endl;
			for (int i = 0; i < 3; ++i) {
				Eigen::Vector3d x(GyroVectors[i][0], GyroVectors[i][1], GyroVectors[i][2]);
				std::cerr << "  [ " << x.transpose() << " ] -> [ " << (scale.array() * (x - gyrobias).array()).transpose() << " ]" << std::endl;
			}
		}
		m_ahrs.set_gyrobias(scale.array() * gyrobias.array());
		m_ahrs.set_gyroscale(scale);
	}
	// Accelerometer Least Squares Fit
	if (false) {
		Eigen::Matrix<double, 18, 12> A(Eigen::Matrix<double, 18, 12>::Zero());
		Eigen::Matrix<double, 18, 1> b(Eigen::Matrix<double, 18, 1>::Zero());
		for (int i = 0; i < 6; ++i) {
			for (int j = 0; j < 3; ++j) {
				A.block<1,3>(i * 3 + j, j * 3) = Eigen::Vector3d(AccVectors[i][0], AccVectors[i][1], AccVectors[i][2]).transpose();
				A(i * 3 + j, 9 + j) = -1;
			}
			// we normalize the A vector to 1, instead of 9.81
			int sgn = 1 | -(i & 1);
			b(i * 3 + (i >> 1)) = -sgn;
		}
		if (true)
			std::cerr << "A = [ " << A << " ]" << std::endl << "b = [ " << b << " ]" << std::endl;
		Eigen::Matrix<double, 12, 1> x((A.transpose() * A).inverse() * A.transpose() * b);
		Eigen::Matrix3d scale;
		for (int i = 0; i < 3; ++i)
			scale.block<1,3>(i, 0) = x.block<3,1>(i * 3, 0).transpose();
		Eigen::Vector3d bias(x.block<3,1>(9, 0));
		if (true) {
			std::cerr << "Accelerometer Scale: [ " << scale << " ]" << std::endl
				  << "  Bias [ " << bias.transpose() << " ]" << std::endl;
			for (int i = 0; i < 6; ++i) {
				Eigen::Vector3d x(AccVectors[i][0], AccVectors[i][1], AccVectors[i][2]);
				std::cerr << "  [ " << x.transpose() << " ] -> [ " << (scale * (x - bias)).transpose() << " ]" << std::endl;
			}
		}
		m_ahrs.set_accelbias(bias.array() / scale.diagonal().array());
		m_ahrs.set_accelscale(scale.diagonal());
	} else {
		Eigen::Matrix<double, 18, 6> A(Eigen::Matrix<double, 18, 6>::Zero());
		Eigen::Matrix<double, 18, 1> b(Eigen::Matrix<double, 18, 1>::Zero());
		for (int i = 0; i < 6; ++i) {
			for (int j = 0; j < 3; ++j) {
				A(i * 3 + j, j) = AccVectors[i][j];
				A(i * 3 + j, j + 3) = -1;
			}
			// we normalize the A vector to 1, instead of 9.81
			int sgn = 1 | -(i & 1);
			b(i * 3 + (i >> 1)) = -sgn;
		}
		if (true)
			std::cerr << "A = [ " << A << " ]" << std::endl << "b = [ " << b << " ]" << std::endl;
		Eigen::Matrix<double, 6, 1> x((A.transpose() * A).inverse() * A.transpose() * b);
		Eigen::Vector3d scale(x.block<3,1>(0, 0));
		Eigen::Vector3d bias(x.block<3,1>(3, 0));
		if (true) {
			std::cerr << "Accelerometer Scale: [ " << scale.transpose() << " ]" << std::endl
				  << "  Bias [ " << bias.transpose() << " ]" << std::endl;
			for (int i = 0; i < 6; ++i) {
				Eigen::Vector3d x(AccVectors[i][0], AccVectors[i][1], AccVectors[i][2]);
				std::cerr << "  [ " << x.transpose() << " ] -> [ " << (scale.array() * (x - bias).array()).transpose() << " ]" << std::endl;
			}
		}
		m_ahrs.set_accelbias(bias.array() / scale.array());
		m_ahrs.set_accelscale(scale);
	}
	update(pc);
}
