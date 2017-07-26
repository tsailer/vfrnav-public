//
// C++ Interface: Attitude Sensor: psmove using hidapi
//
// Description: Attitude Sensor Handling
//
//
// Author: Thomas Sailer <t.sailer@alumni.ethz.ch>, (C) 2011, 2012
//
// Copyright: See COPYING file that comes with this distribution
//
//

#ifndef SENSATTPSMOVEHID_H
#define SENSATTPSMOVEHID_H

#include <libudev.h>
#include <hidapi.h>
#include "sensattpsmove.h"

class Sensors::SensorAttitudePSMoveHID : public SensorAttitudePSMove {
  public:
	SensorAttitudePSMoveHID(Sensors& sensors, const Glib::ustring& configgroup);
	virtual ~SensorAttitudePSMoveHID();

  protected:
	static const unsigned short PSMOVE_2_VID = 0x0000;
	static const unsigned short PSMOVE_2_PID = 0x0000;

	static Glib::ustring wstr2ustr(const std::wstring& ws);
	static std::wstring ustr2wstr(const Glib::ustring& us);
        bool handle_initial_connect(void);
        bool handle_connect_cb(Glib::IOCondition iocond);
        void handle_connect(struct udev_device *udev);
        void handle_connect(hid_device *dev, uint16_t busnum,
			    uint32_t vndid, uint32_t prodid, const Glib::ustring& serial);
	void close_dev(void);
        bool handle_dev_read(Glib::IOCondition iocond);

	enum {
		parnrstart = SensorAttitudePSMove::parnrend,
		parnrserial = parnrstart,
		parnrend
	};

	struct udev *m_udev;
	struct udev_monitor *m_udevmon;
	hid_device *m_dev;
	sigc::connection m_conndevarrival;
	sigc::connection m_conndev;
};

#endif /* SENSATTPSMOVEHID_H */
