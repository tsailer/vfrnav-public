//
// C++ Interface: Attitude Sensor: psmove
//
// Description: Attitude Sensor Handling
//
//
// Author: Thomas Sailer <t.sailer@alumni.ethz.ch>, (C) 2011, 2012, 2013
//
// Copyright: See COPYING file that comes with this distribution
//
//

#ifndef SENSATTPSMOVE_H
#define SENSATTPSMOVE_H

#ifdef HAVE_BLUEZ
#include <bluetooth/bluetooth.h>
#else
typedef struct {
        uint8_t b[6];
} __attribute__((packed)) bdaddr_t;
#endif

#ifdef HAVE_LIBUSB1
#include <libusb.h>
#else
struct libusb_device;
#endif

#include "sensattitude.h"
#include "ahrs.h"

class Sensors::SensorAttitudePSMove : public SensorAttitude {
  public:
	SensorAttitudePSMove(Sensors& sensors, const Glib::ustring& configgroup);
	virtual ~SensorAttitudePSMove();

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
	bool open(const uint8_t *rep, unsigned int replen, const Glib::ustring& id = "");
	bool open(uint32_t vndid, uint32_t prodid, const Glib::ustring& id = "");
	void close(void);

  private:
	bool close_internal(void);

  protected:
	bool is_open(void) const { return m_devopen; }
	void handle_report(const uint8_t *report, int length);
	void set_navigation(unsigned int deltasamples, int16_t accel[3], int16_t gyro[3], int16_t mag[3]);
	void update_values(void) const;
	paramfail_t get_paramfail_open(void) const;
	paramfail_t get_paramfail(void) const;
	static void libusb_perror(int err, const std::string & txt);
	bool pair_find_btaddr(bdaddr_t& addr);
	void pair_scan(void);
	void pair_sixaxis_ds3(libusb_device *dev);
	void pair_psmove(libusb_device *dev);
	void calib_psmove(const std::string& deviceid, const uint8_t calreport[49+47+47]);

	static constexpr double samplerate = 87.8;
	static const unsigned short PSMOVE_VID = 0x054c;
	static const unsigned short PSMOVE_PID = 0x03d5;
	static const unsigned short SIXAXIS_VID = 0x054c;
	static const unsigned short SIXAXIS_PID = 0x0268;
	static const AHRS::mode_t ahrsmodemap[4];

	enum {
		parnrstart = SensorInstance::parnrend,
		parnrid = parnrstart,
		parnrserial,
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
		parnrpair,
		parnrend
	};

	AHRS m_ahrs;
	int16_t m_rawgyro[3];
	int16_t m_rawaccel[3];
	int16_t m_rawmag[3];
	Glib::ustring m_serial;
	Glib::TimeVal m_attitudetime;
	mutable Glib::TimeVal m_attitudereporttime;
	mutable double m_pitch;
	mutable double m_bank;
	mutable double m_slip;
	mutable double m_rate;
	mutable double m_hdg;
	mutable bool m_dirty;
	bool m_devopen;
	uint32_t m_samplecount;
        int m_seq;
	unsigned int m_attitudeprio;
	unsigned int m_headingprio;
};

#endif /* SENSATTPSMOVE_H */
