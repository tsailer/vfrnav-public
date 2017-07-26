#ifndef AHRS_H
#define AHRS_H

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#define _GNU_SOURCE 1
//#define EIGEN_DONT_ALIGN 1
//#define EIGEN_INTERNAL_DEBUGGING 1

#include <stdint.h>
#include <vector>
#include <glibmm/keyfile.h>
#include <Eigen/Dense>
#include <Eigen/Geometry>

class AHRS {
public:
	typedef enum {
		mode_normal,
		mode_fast,
		mode_slow,
		mode_gyroonly
	} mode_t;

	typedef enum {
		init_standard,
		init_psmove
	} init_t;

	AHRS(double samplerate = 87.8, const std::string& id = "", init_t ini = init_standard);
	~AHRS();
	bool init(init_t ini);
	void save(Glib::KeyFile& kf, const Glib::ustring& group) const;
	bool load(const Glib::KeyFile& kf, const Glib::ustring& group);
	void save(const std::string& filename) const;
	bool load(const std::string& filename);
	void save(void) const;
	bool load(void);
	mode_t get_mode(void) const;
	void set_mode(mode_t mode);
	void new_samples(unsigned int interval, const int16_t gyro[3], const int16_t accel[3], const int16_t mag[3]);
	void new_samples(unsigned int interval, const double gyro[3], const double accel[3], const double mag[3]);
	void new_gyro_samples(unsigned int interval, const double gyro[3]);
	void new_accel_samples(const double accel[3]);
	void new_mag_samples(const double mag[3]);
	const std::string& get_id(void) const { return m_id; }
	void set_id(const std::string& id) { m_id = id; }
	double get_sampletime(void) const { return m_sampletime; }
	void set_sampletime(double t) { m_sampletime = t; }
	double get_samplerate(void) const { return 1.0 / m_sampletime; }
	void set_samplerate(double f) { m_sampletime = 1.0 / f; }

	// Raw Sensor Measurement, in Sensor coordinate frame
	const Eigen::Vector3d& get_accelerometer(void) const { return m_accel; }
	const Eigen::Vector3d& get_magnetometer(void) const { return m_mag; }
	const Eigen::Vector3d& get_gyroscope(void) const { return m_gyro; }

	// Filtered Sensor Measurements, in Body coordinate frame
	Eigen::Vector3d get_filtered_accelerometer(void) const;
	Eigen::Vector3d get_filtered_gyroscope(void) const;

	// Bias and Scaling, in Sensor coordinate frame
	const Eigen::Vector3d& get_gyrobias(void) const { return m_gyrobias; }
	const Eigen::Vector3d& get_gyroscale(void) const { return m_gyroscale; }
	const Eigen::Vector3d& get_accelbias(void) const { return m_accelbias; }
	const Eigen::Vector3d& get_accelscale(void) const { return m_accelscale; }
	const Eigen::Vector3d& get_magbias(void) const { return m_magbias; }
	const Eigen::Vector3d& get_magscale(void) const { return m_magscale; }
	double get_magdeviation(void) const { return m_magdeviation; }
	// Sensor to Body transformation
	const Eigen::Quaterniond& get_sensororientation(void) const { return m_sensororient; }

	double get_accelscale2(void) const { return m_accelscale2; }
	double get_magscale2(void) const { return m_magscale2; }

	void set_gyrobias(const Eigen::Vector3d& v) { m_gyrobias = v; }
	void set_gyroscale(const Eigen::Vector3d& v) { m_gyroscale = v; }
	void set_accelbias(const Eigen::Vector3d& v) { m_accelbias = v; }
	void set_accelscale(const Eigen::Vector3d& v) { m_accelscale = v; }
	void set_magbias(const Eigen::Vector3d& v) { m_magbias = v; }
	void set_magscale(const Eigen::Vector3d& v) { m_magscale = v; }
	void set_magdeviation(double dev = 0) { m_magdeviation = dev; }
	void set_sensororientation(const Eigen::Quaterniond& v) { m_sensororient = v; }

	// coordinate transformations
	Eigen::Vector3d body_to_inertial(const Eigen::Vector3d& v) const;
	Eigen::Vector3d inertial_to_body(const Eigen::Vector3d& v) const;
	Eigen::Vector3d body_to_sensor(const Eigen::Vector3d& v) const;
	Eigen::Vector3d sensor_to_body(const Eigen::Vector3d& v) const;
	Eigen::Vector3d e1_to_body(void) const;
	Eigen::Vector3d e3_to_body(void) const;

	// Sensor to Inertial transformation
	const Eigen::Quaterniond& get_attitude(void) const { return m_q; }

	static const std::string& to_str(mode_t mode);

	template <typename T>
	class EulerAngles {
	public:
		EulerAngles(T r = 0, T p = 0, T y = 0) { m_angles[0] = r; m_angles[1] = p; m_angles[2] = y; }
		const T& roll(void) const { return m_angles[0]; }
		const T& pitch(void) const { return m_angles[1]; }
		const T& yaw(void) const { return m_angles[2]; }
		const T& operator[](unsigned int idx) const { return m_angles[idx]; }
		T& roll(void) { return m_angles[0]; }
		T& pitch(void) { return m_angles[1]; }
		T& yaw(void) { return m_angles[2]; }
		T& operator[](unsigned int idx) { return m_angles[idx]; }

	protected:
		T m_angles[3];
	};

	typedef EulerAngles<double> EulerAnglesd;

	static EulerAnglesd to_euler(const Eigen::Quaterniond& q);
	static Eigen::Quaterniond to_quaternion(const EulerAnglesd& ea);

	static inline Eigen::Quaterniond to_quaternion(const Eigen::Vector3d& v) {
		Eigen::Quaterniond q;
		q.w() = 0;
		q.vec() = v;
		return q;
	}

	// coefficients
	double get_coeff_la(void) const { return m_coeff_la; }
	double get_coeff_lc(void) const { return m_coeff_lc; }
	double get_coeff_ld(void) const { return m_coeff_ld; }
	double get_coeff_sigma(void) const { return m_coeff_sigma; }
	double get_coeff_n(void) const { return m_coeff_n; }
	double get_coeff_o(void) const { return m_coeff_o; }

	void set_coeff_la(double v) { m_coeff_la = v; }
	void set_coeff_lc(double v) { m_coeff_lc = v; }
	void set_coeff_ld(double v) { m_coeff_ld = v; }
	void set_coeff_sigma(double v) { m_coeff_sigma = v; }
	void set_coeff_n(double v) { m_coeff_n = v; }
	void set_coeff_o(double v) { m_coeff_o = v; }

	static void savekf(Glib::KeyFile& kf, const Glib::ustring& group, const Glib::ustring& name, double v);
	static bool loadkf(const Glib::KeyFile& kf, const Glib::ustring& group, const Glib::ustring& name, double& v);
	static void savekf(Glib::KeyFile& kf, const Glib::ustring& group, const Glib::ustring& name, const Eigen::Vector3d& v);
	static bool loadkf(const Glib::KeyFile& kf, const Glib::ustring& group, const Glib::ustring& name, Eigen::Vector3d& v);
	static void savekf(Glib::KeyFile& kf, const Glib::ustring& group, const Glib::ustring& name, const Eigen::Quaterniond& v);
	static bool loadkf(const Glib::KeyFile& kf, const Glib::ustring& group, const Glib::ustring& name, Eigen::Quaterniond& v);

	EIGEN_MAKE_ALIGNED_OPERATOR_NEW

protected:
	static double givens(double& c, double& s, double a, double b);
	void new_samples(unsigned int interval);

	std::string m_id;
	Eigen::Quaterniond m_q;
	Eigen::Quaterniond m_sensororient;
	Eigen::Vector3d m_accel;
	Eigen::Vector3d m_mag;
	Eigen::Vector3d m_gyro;
	Eigen::Vector3d m_accelbias;
	Eigen::Vector3d m_accelscale;
	Eigen::Vector3d m_magbias;
	Eigen::Vector3d m_magscale;
	Eigen::Vector3d m_gyrobias;
	Eigen::Vector3d m_gyroscale;
	Eigen::Vector3d m_accelfilt;
	Eigen::Vector3d m_gyrofilt;

	double m_sampletime;
	double m_accelscale2;
	double m_magscale2;
	double m_magdeviation;
	double m_coeff_la;
	double m_coeff_lc;
	double m_coeff_ld;
	double m_coeff_sigma;
	double m_coeff_n;
	double m_coeff_o;
	mode_t m_mode;
	bool m_init;
};

#endif /* AHRS_H */
