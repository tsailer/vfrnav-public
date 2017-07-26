//
// C++ Interface: Attitude Sensor: Linux Industrial I/O API
//
// Description: Attitude Sensor Handling
//
//
// Author: Thomas Sailer <t.sailer@alumni.ethz.ch>, (C) 2011, 2012, 2013
//
// Copyright: See COPYING file that comes with this distribution
//
//

#ifndef SENSATTIIO_H
#define SENSATTIIO_H

#include "sensattitude.h"
#include "ahrs.h"

class Sensors::SensorAttitudeIIO : public SensorAttitude {
  public:
	SensorAttitudeIIO(Sensors& sensors, const Glib::ustring& configgroup);
	virtual ~SensorAttitudeIIO();

	virtual void get_heading(double& hdg) const;
	virtual unsigned int get_heading_priority(void) const;
	virtual Glib::TimeVal get_heading_time(void) const;
	virtual bool is_heading_true(void) const;
	virtual bool is_heading_ok(const Glib::TimeVal& tv) const;

	virtual void get_attitude(double& pitch, double& bank, double& slip, double& rate) const;
	virtual unsigned int get_attitude_priority(void) const;
	virtual Glib::TimeVal get_attitude_time(void) const;
	virtual bool is_attitude_ok(const Glib::TimeVal& tv) const;

	virtual void get_param_desc(unsigned int pagenr, paramdesc_t& pd);
	virtual unsigned int get_param_pages(void) const { return 2; }
	using SensorAttitude::get_param;
	virtual paramfail_t get_param(unsigned int nr, Glib::ustring& v) const;
	virtual paramfail_t get_param(unsigned int nr, int& v) const;
	virtual paramfail_t get_param(unsigned int nr, double& v) const;
	virtual paramfail_t get_param(unsigned int nr, double& pitch, double& bank, double& slip, double& hdg, double& rate) const;
	using SensorAttitude::set_param;
	virtual void set_param(unsigned int nr, const Glib::ustring& v);
	virtual void set_param(unsigned int nr, int v);
	virtual void set_param(unsigned int nr, double v);

	virtual std::string logfile_header(void) const;
	virtual std::string logfile_records(void) const;

        EIGEN_MAKE_ALIGNED_OPERATOR_NEW

  protected:

	bool try_connect(void);
	bool close(void);
	bool is_open(void) const { return false; }

        bool handle_initial_connect(void);
        bool handle_connect_cb(Glib::IOCondition iocond);
        void handle_connect(struct udev_device *udev);

  private:
	bool close_internal(void);

  protected:
	void update_values(void) const;
	paramfail_t get_paramfail_open(void) const;
	paramfail_t get_paramfail(void) const;

	static const AHRS::mode_t ahrsmodemap[4];

	enum {
		parnrstart = SensorInstance::parnrend,
		parnracceldevname = parnrstart,
		parnracceldevmfg,
		parnracceldevdesc,
		parnracceldevserial,
		parnrgyrodevname,
		parnrgyrodevmfg,
		parnrgyrodevdesc,
		parnrgyrodevserial,
		parnrmagdevname,
		parnrmagdevmfg,
		parnrmagdevdesc,
		parnrmagdevserial,
		parnrattitudeprio,
		parnrheadingprio,
		parnrmode,
		parnrcoeffla,
		parnrcoefflc,
		parnrcoeffld,
		parnrcoeffsigma,
		parnrcoeffn,
		parnrcoeffo,
		parnrattitude,
		parnrattituderoll = parnrattitude,
		parnrattitudepitch,
		parnrattitudeheading,
		parnrorientation,
		parnrorientationroll = parnrorientation,
		parnrorientationpitch,
		parnrorientationheading,
		parnrorientationdeviation,
		parnrorientadjust,
		parnrorientfineadjustroll = parnrorientadjust,
		parnrorientcoarseadjustroll,
		parnrorientfineadjustpitch,
		parnrorientcoarseadjustpitch,
		parnrorientfineadjustheading,
		parnrorientcoarseadjustheading,
		parnrorienterectdefault,
		parnrrawgyro,
		parnrrawgyrox = parnrrawgyro,
		parnrrawgyroy,
		parnrrawgyroz,
		parnrrawaccel,
		parnrrawaccelx = parnrrawaccel,
		parnrrawaccely,
		parnrrawaccelz,
		parnrrawmag,
		parnrrawmagx = parnrrawmag,
		parnrrawmagy,
		parnrrawmagz,
		parnrattitudequat,
		parnrattitudequatx = parnrattitudequat,
		parnrattitudequaty,
		parnrattitudequatz,
		parnrattitudequatw,
		parnrorientationquat,
		parnrorientationquatx = parnrorientationquat,
		parnrorientationquaty,
		parnrorientationquatz,
		parnrorientationquatw,
		parnrcalgyrobias,
		parnrcalgyrobiasx = parnrcalgyrobias,
		parnrcalgyrobiasy,
		parnrcalgyrobiasz,
		parnrcalgyroscale,
		parnrcalgyroscalex = parnrcalgyroscale,
		parnrcalgyroscaley,
		parnrcalgyroscalez,
		parnrcalaccelbias,
		parnrcalaccelbiasx = parnrcalaccelbias,
		parnrcalaccelbiasy,
		parnrcalaccelbiasz,
		parnrcalaccelscale,
		parnrcalaccelscalex = parnrcalaccelscale,
		parnrcalaccelscaley,
		parnrcalaccelscalez,
		// note: the following nine parameters must remain in this order, otherwise magcalib won't work
		parnrrawsamplemag,
		parnrrawsamplemagx = parnrrawsamplemag,
		parnrrawsamplemagy,
		parnrrawsamplemagz,
		parnrcalmagbias,
		parnrcalmagbiasx = parnrcalmagbias,
		parnrcalmagbiasy,
		parnrcalmagbiasz,
		parnrcalmagscale,
		parnrcalmagscalex = parnrcalmagscale,
		parnrcalmagscaley,
		parnrcalmagscalez,
		parnrcalaccelscale2,
		parnrcalmagscale2,
		parnrgyro,
		parnrend
	};

	typedef enum {
		devtyp_gyro = 0,
		devtyp_accel,
		devtyp_mag,
		devtyp_invalid
	} devtyp_t;

	struct udev *m_udev;
	struct udev_monitor *m_udevmon;
	sigc::connection m_conndevarrival;
	sigc::connection m_conndev[devtyp_invalid];
	int m_devfd[devtyp_invalid];
	int m_devnum[devtyp_invalid];
	AHRS m_ahrs;
	double m_rawgyro[3];
	double m_rawaccel[3];
	double m_rawmag[3];
	Glib::TimeVal m_attitudetime;
	mutable Glib::TimeVal m_attitudereporttime;
	mutable double m_pitch;
	mutable double m_bank;
	mutable double m_slip;
	mutable double m_rate;
	mutable double m_hdg;
	mutable bool m_dirty;
	uint32_t m_samplecount;
 	unsigned int m_attitudeprio;
	unsigned int m_headingprio;
};

#endif /* SENSATTIIO_H */
