//
// C++ Interface: Attitude Sensor: raw STM Sensor Hub driver
//
// Description: Attitude Sensor Handling
//
//
// Author: Thomas Sailer <t.sailer@alumni.ethz.ch>, (C) 2011, 2012, 2013
//
// Copyright: See COPYING file that comes with this distribution
//
//

#ifndef SENSATTSTMHUB_H
#define SENSATTSTMHUB_H

#include <libudev.h>
#include <libusb.h>

#include "sensattitude.h"
#include "ahrs.h"

class Sensors::SensorAttitudeSTMSensorHub : public SensorAttitude {
  public:
	SensorAttitudeSTMSensorHub(Sensors& sensors, const Glib::ustring& configgroup);
	virtual ~SensorAttitudeSTMSensorHub();

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
	static const uint16_t stmsensorhub_vid = 0x0483;
	static const uint16_t stmsensorhub_pid = 0x91d1;

	bool try_connect(void);
	bool close(void);
	bool is_open(void) const;
	bool is_open(void);

        bool handle_initial_connect(void);
        bool handle_connect_cb(Glib::IOCondition iocond);
        void handle_connect(struct udev_device *udev);

	void libusb_perror(int err, const std::string& txt);
	std::vector<uint8_t> get_feature(uint8_t id);
	void set_feature(const std::vector<uint8_t>& buf);

	static void transfer_cb_1(struct libusb_transfer *transfer);
	void transfer_cb(struct libusb_transfer *transfer);
	static void add_pollfd_1(int fd, short events, void *user_data);
	void add_pollfd(int fd, short events);
	static void remove_pollfd_1(int fd, void *user_data);
	void remove_pollfd(int fd);
	bool libusb_poll(void);
	static Glib::ustring wstr2ustr(const std::wstring& ws);

	void usb_thread_init(void);
	void usb_thread(void);
	void usb_thread_poll(void);

	void log(Sensors::loglevel_t lvl, const Glib::ustring& msg);

  private:
	bool close_internal(void);

  protected:
	void update_values(void) const;
	void statechange(void);

	static const AHRS::mode_t ahrsmodemap[4];

	enum {
		parnrstart = SensorInstance::parnrend,
		parnracceldevname = parnrstart,
		parnracceldevmfg,
		parnrgyrodevname,
		parnrgyrodevmfg,
		parnrmagdevname,
		parnrmagdevmfg,
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

	// udev
	struct udev *m_udev;
	struct udev_monitor *m_udevmon;
	sigc::connection m_conndevarrival;
	//  thread management
        Glib::Thread *m_thread;
        mutable Glib::Mutex m_mutex;
        Glib::Cond m_cond;
        Glib::Dispatcher m_dispatcher;

	// thread state
	typedef enum {
		tstate_none,
		tstate_initializing,
		tstate_initahrs,
		tstate_running,
		tstate_terminating,
		tstate_terminating_nosave,
		tstate_stopped,
		tstate_stopped_nosave
	} tstate_t;
	tstate_t m_tstate;

	struct libusb_context *m_context;
	libusb_device_handle *m_devh;
	bool m_kerneldrv;
	Glib::ustring m_devname[3];
	Glib::ustring m_devmfg[3];
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
	bool m_update;
	uint32_t m_samplecount;
 	unsigned int m_attitudeprio;
	unsigned int m_headingprio;
	typedef std::vector<std::pair<Sensors::loglevel_t,Glib::ustring> > logmsgs_t;
	logmsgs_t m_logmsgs;
};

#endif /* SENSATTSTMHUB_H */
