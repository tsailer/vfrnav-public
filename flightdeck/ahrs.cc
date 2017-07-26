#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <giomm/file.h>
#include <glibmm/keyfile.h>
#include <glibmm/fileutils.h>
#include <iostream>
#include <iomanip>
#include <fstream>
#include <unistd.h>
#include "ahrs.h"
#include "fplan.h"

AHRS::AHRS(double samplerate, const std::string& id, init_t ini)
	: m_id(id), m_q(1, 0, 0, 0), m_sensororient(1, 0, 0, 0),
	  m_accel(0, 0, 0), m_mag(0, 0, 0), m_gyro(0, 0, 0), m_accelbias(0, 0, 0), m_accelscale(5e-5, 5e-5, 5e-5),
	  m_magbias(0, 0, 0), m_magscale(7e-5, 7e-5, 7e-5), m_gyrobias(0, 0, 0), m_gyroscale(0.8e-3, 0.8e-3, 0.8e-3),
	  m_accelfilt(0, 0, 0), m_gyrofilt(0, 0, 0), m_sampletime(1.0 / samplerate), m_accelscale2(1), m_magscale2(1),
	  m_magdeviation(0), m_coeff_la(0.006), m_coeff_lc(0.010), m_coeff_ld(0.006),
	  m_coeff_sigma(0.005), m_coeff_n(0.025), m_coeff_o(0.050), m_mode(mode_normal), m_init(true)
{
	init(ini);
}

AHRS::~AHRS()
{
	save();
}

bool AHRS::init(init_t ini)
{
	switch (ini) {
	default:
		// scale accel and mag that nominal vector length is one
		m_accelscale = Eigen::Vector3d(1, 1, 1);
		m_magscale = Eigen::Vector3d(0.002, 0.002, 0.002);
		// scale gyro to rads/s
		m_gyroscale = Eigen::Vector3d(M_PI / 180.0, M_PI / 180.0, M_PI / 180.0);
		break;

	case init_psmove:
		m_accelscale = Eigen::Vector3d(5e-5, 5e-5, 5e-5);
		m_magscale = Eigen::Vector3d(7e-5, 7e-5, 7e-5);
		m_gyroscale = Eigen::Vector3d(0.8e-3, 0.8e-3, 0.8e-3);
		break;
	}
	m_q = Eigen::Quaterniond(1, 0, 0, 0);
	m_accel = Eigen::Vector3d(0, 0, 0);
	m_mag = Eigen::Vector3d(0, 0, 0);
	m_gyro = Eigen::Vector3d(0, 0, 0);
	m_accelbias = Eigen::Vector3d(0, 0, 0);
	m_magbias = Eigen::Vector3d(0, 0, 0);
	m_gyrobias = Eigen::Vector3d(0, 0, 0);
	m_sensororient = Eigen::Quaterniond(1, 0, 0, 0);
	m_accelfilt = Eigen::Vector3d(0, 0, 0);
	m_gyrofilt = Eigen::Vector3d(0, 0, 0);
	m_accelscale2 = 1;
	m_magscale2 = 1;
	m_magdeviation = 0;
	m_coeff_la = 0.06;
	m_coeff_lc = 0.10;
	m_coeff_ld = 0.06;
	m_coeff_sigma = 0.05;
	m_coeff_n = 0.25;
	m_coeff_o = 0.50;
	m_mode = mode_normal;
	if (get_id() == "bluetooth_00:06:F7:DE:7C:44") {
		m_accelbias = Eigen::Vector3d(563, 105, 624);
		m_accelscale = Eigen::Vector3d(6.4217826868738761e-05, 5.0030018010806483e-05, 5.0332192470304009e-05);
		m_magbias = Eigen::Vector3d(328, 1400, -2248);
		m_magscale = Eigen::Vector3d(7.5665859564164653e-05, 6.5240083507306883e-05, 7.1839080459770114e-05);
		m_gyrobias = Eigen::Vector3d(0.00456155, -0.00106963, 0.0036704);
		m_gyroscale = Eigen::Vector3d(0.000813063, 0.000916584, 0.000801764);
		m_sensororient = Eigen::Quaterniond(1, 0, 0, 0);
	}
	bool ld = load();
	m_sensororient.normalize();
	m_init = true;
	std::cout << "AHRS: sensor " << m_id << std::endl;
	return ld;
}

void AHRS::savekf(Glib::KeyFile& kf, const Glib::ustring& group, const Glib::ustring& name, double v)
{
	kf.set_double(group, name, v);
}

bool AHRS::loadkf(const Glib::KeyFile& kf, const Glib::ustring& group, const Glib::ustring& name, double& v)
{
	if (!kf.has_group(group))
		return false;
	if (!kf.has_key(group, name))
		return false;
	v = kf.get_double(group, name);
	return true;
}

void AHRS::savekf(Glib::KeyFile& kf, const Glib::ustring& group, const Glib::ustring& name, const Eigen::Vector3d& v)
{
	Glib::ArrayHandle<double> a(v.data(), 3, Glib::OWNERSHIP_NONE);
	kf.set_double_list(group, name, a);
}

bool AHRS::loadkf(const Glib::KeyFile& kf, const Glib::ustring& group, const Glib::ustring& name, Eigen::Vector3d& v)
{
	if (!kf.has_group(group))
		return false;
	if (!kf.has_key(group, name))
		return false;
	std::vector<double> a(kf.get_double_list(group, name));
	int i = 0;
	for (; i < 3 && i < a.size(); ++i)
		v(i) = a[i];
	for (; i < 3; ++i)
		v(i) = 0;
	return a.size() >= 3;
}

void AHRS::savekf(Glib::KeyFile& kf, const Glib::ustring& group, const Glib::ustring& name, const Eigen::Quaterniond& v)
{
	Glib::ArrayHandle<double> a(v.coeffs().data(), 4, Glib::OWNERSHIP_NONE);
	kf.set_double_list(group, name, a);
}

bool AHRS::loadkf(const Glib::KeyFile& kf, const Glib::ustring& group, const Glib::ustring& name, Eigen::Quaterniond& v)
{
	if (!kf.has_group(group))
		return false;
	if (!kf.has_key(group, name))
		return false;
	Glib::ArrayHandle<double> a(kf.get_double_list(group, name));
	if (a.size() < 4)
		return false;
	for (int i = 0; i < 4; ++i)
		v.coeffs()(i) = a.data()[i];
	return true;
}

void AHRS::save(Glib::KeyFile& kf, const Glib::ustring& group) const
{
	savekf(kf, group, "gyrobias", m_gyrobias);
	savekf(kf, group, "gyroscale", m_gyroscale);
	savekf(kf, group, "accelbias", m_accelbias);
	savekf(kf, group, "accelscale", m_accelscale);
	savekf(kf, group, "magbias", m_magbias);
	savekf(kf, group, "magscale", m_magscale);
	savekf(kf, group, "magdeviation", m_magdeviation);
	savekf(kf, group, "sensororient", m_sensororient);
	savekf(kf, group, "coeffla", m_coeff_la);
	savekf(kf, group, "coefflc", m_coeff_lc);
	savekf(kf, group, "coeffld", m_coeff_ld);
	savekf(kf, group, "coeffsigma", m_coeff_sigma);
	savekf(kf, group, "coeffn", m_coeff_n);
	savekf(kf, group, "coeffo", m_coeff_o);
}

bool AHRS::load(const Glib::KeyFile& kf, const Glib::ustring& group)
{
	if (!kf.has_group(group))
		return false;
	loadkf(kf, group, "gyrobias", m_gyrobias);
	loadkf(kf, group, "gyroscale", m_gyroscale);
	loadkf(kf, group, "accelbias", m_accelbias);
	loadkf(kf, group, "accelscale", m_accelscale);
	loadkf(kf, group, "magbias", m_magbias);
	loadkf(kf, group, "magscale", m_magscale);
	loadkf(kf, group, "magdeviation", m_magdeviation);
	loadkf(kf, group, "sensororient", m_sensororient);
	loadkf(kf, group, "coeffla", m_coeff_la);
	loadkf(kf, group, "coefflc", m_coeff_lc);
	loadkf(kf, group, "coeffld", m_coeff_ld);
	loadkf(kf, group, "coeffsigma", m_coeff_sigma);
	loadkf(kf, group, "coeffn", m_coeff_n);
	loadkf(kf, group, "coeffo", m_coeff_o);
	m_sensororient.normalize();
	m_init = true;
	return true;
}

void AHRS::save(const std::string& filename) const
{
	if (get_id().empty())
		return;
	Glib::KeyFile kf;
	try {
		kf.load_from_file(filename, Glib::KEY_FILE_KEEP_COMMENTS | Glib::KEY_FILE_KEEP_TRANSLATIONS);
	} catch (const Glib::FileError& e) {
		std::cerr << "Unable to load AHRS settings from " << filename << ": " << e.what() << std::endl;
	} catch (const Gio::Error& e) {
		std::cerr << "Unable to load AHRS settings from " << filename << ": " << e.what() << std::endl;
	}
	save(kf, get_id());
	try {
		Glib::RefPtr<Gio::File> f(Gio::File::create_for_path(filename));
		std::string new_etag;
		f->replace_contents(kf.to_data(), "", new_etag);
	} catch (const Glib::FileError& e) {
		std::cerr << "Unable to save AHRS settings to " << filename << ": " << e.what() << std::endl;
	} catch (const Gio::Error& e) {
		std::cerr << "Unable to save AHRS settings to " << filename << ": " << e.what() << std::endl;
	}
}

bool AHRS::load(const std::string& filename)
{
	if (get_id().empty())
		return false;
	Glib::KeyFile kf;
	try {
		if (!kf.load_from_file(filename, Glib::KEY_FILE_KEEP_COMMENTS | Glib::KEY_FILE_KEEP_TRANSLATIONS))
			return false;
	} catch (const Glib::FileError& e) {
		std::cerr << "Unable to load AHRS settings from " << filename << ": " << e.what() << std::endl;
		return false;
	} catch (const Gio::Error& e) {
		std::cerr << "Unable to load AHRS settings from " << filename << ": " << e.what() << std::endl;
		return false;
	}
	return load(kf, get_id());
}

void AHRS::save(void) const
{
	save(Glib::build_filename(FPlan::get_userdbpath(), "ahrsstate.cfg"));
}

bool AHRS::load(void)
{
	return load(Glib::build_filename(FPlan::get_userdbpath(), "ahrsstate.cfg"));
}

AHRS::mode_t AHRS::get_mode(void) const
{
	return m_mode;
}

void AHRS::set_mode(mode_t mode)
{
	if (m_mode == mode)
		return;
	m_mode = mode;
	std::cout << "set mode " << to_str(mode) << std::endl;
}

const std::string& AHRS::to_str(mode_t mode)
{
	switch (mode) {
	case mode_normal:
	{
		static const std::string s("normal");
		return s;
	}

	case mode_fast:
	{
		static const std::string s("fast");
		return s;
	}

	case mode_slow:
	{
		static const std::string s("slow");
		return s;
	}

	case mode_gyroonly:
	{
		static const std::string s("gyroonly");
		return s;
	}

	default:
	{
		static const std::string s("?""?");
		return s;
	}
	}
}

double AHRS::givens(double& c, double& s, double a, double b)
{
	double absa(abs(a)), absb(abs(b));
	if (b == 0) {
		c = copysign(1, a);
		s = 0;
		return absa;
	}
	if (a == 0) {
		c = 0;
		s = copysign(1, b);
		return absb;
	}
	if (absb > absa) {
		double t = a / b;
		double u = copysign(sqrt(1 + t * t), b);
		s = 1 / u;
		c = s * t;
		return b * u;
	}
	double t = b / a;
	double u = copysign(sqrt(1 + t * t), a);
	c = 1 / u;
	s = c * t;
	return a * u;
}

AHRS::EulerAnglesd AHRS::to_euler(const Eigen::Quaterniond& q)
{
	EulerAnglesd ea;
	ea.roll() = atan2(2 * (q.w() * q.x() - q.y() * q.z()), 1 - 2 * (q.x() * q.x() + q.y() * q.y()));
	ea.pitch() = asin(2 * (q.w() * q.y() + q.x() * q.z()));
	ea.yaw() = atan2(2 * (q.w() * q.z() - q.x() * q.y()), 1 - 2 * (q.y() * q.y() + q.z() * q.z()));
	return ea;
}

Eigen::Quaterniond AHRS::to_quaternion(const EulerAnglesd& ea)
{
	Eigen::Quaterniond q(1, 0, 0, 0);
	// Yaw
	{
		double a(ea.yaw() * 0.5);
		Eigen::Quaterniond q1(cos(a), 0, 0, sin(a));
		q = q1;
	}
	// Pitch
	{
		double a(ea.pitch() * 0.5);
		Eigen::Quaterniond q1(cos(a), 0, sin(a), 0);
		q = q1 * q;
	}
	// Roll
	{
		double a(ea.roll() * 0.5);
		Eigen::Quaterniond q1(cos(a), sin(a), 0, 0);
		q = q1 * q;
	}
	return q;
}

Eigen::Vector3d AHRS::get_filtered_accelerometer(void) const
{
	return (m_sensororient * to_quaternion(m_accelfilt) * m_sensororient.conjugate()).vec();
}

Eigen::Vector3d AHRS::get_filtered_gyroscope(void) const
{
	return (m_sensororient * to_quaternion(m_gyrofilt) * m_sensororient.conjugate()).vec();
}

Eigen::Vector3d AHRS::body_to_inertial(const Eigen::Vector3d& v) const
{
	Eigen::Quaterniond q(m_sensororient.conjugate() * m_q);
	return (q * to_quaternion(v) * q.conjugate()).vec();
}

Eigen::Vector3d AHRS::inertial_to_body(const Eigen::Vector3d& v) const
{
	Eigen::Quaterniond q(m_sensororient * m_q.conjugate());
	return (q * to_quaternion(v) * q.conjugate()).vec();
}

Eigen::Vector3d AHRS::body_to_sensor(const Eigen::Vector3d& v) const
{
	return (m_sensororient.conjugate() * to_quaternion(v) * m_sensororient).vec();
}

Eigen::Vector3d AHRS::sensor_to_body(const Eigen::Vector3d& v) const
{
	return (m_sensororient * to_quaternion(v) * m_sensororient.conjugate()).vec();
}

Eigen::Vector3d AHRS::e1_to_body(void) const
{
	Eigen::Quaterniond q(m_sensororient * m_q.conjugate());
	return (q * Eigen::Quaterniond(0, 1, 0, 0) * q.conjugate()).vec();
}

Eigen::Vector3d AHRS::e3_to_body(void) const
{
	Eigen::Quaterniond q(m_sensororient * m_q.conjugate());
	return (q * Eigen::Quaterniond(0, 0, 0, 1) * q.conjugate()).vec();
}

inline Eigen::Quaterniond operator*(double x, const Eigen::Quaterniond& q)
{
	return Eigen::Quaterniond(x * q.w(), x * q.x(), x * q.y(), x * q.z());
}

inline Eigen::Quaterniond operator*(const Eigen::Quaterniond& q, double x)
{
	return Eigen::Quaterniond(x * q.w(), x * q.x(), x * q.y(), x * q.z());
}

inline Eigen::Quaterniond operator+(const Eigen::Quaterniond& q1, const Eigen::Quaterniond& q2)
{
	return Eigen::Quaterniond(q1.w() + q2.w(), q1.x() + q2.x(), q1.y() + q2.y(), q1.z() + q2.z());
}

inline Eigen::Quaterniond& operator+=(Eigen::Quaterniond& q1, const Eigen::Quaterniond& q2)
{
	return q1 = q1 + q2;
}

void AHRS::new_samples(unsigned int interval, const int16_t gyro[3], const int16_t accel[3], const int16_t mag[3])
{
	m_gyro = Eigen::Array3d(gyro[0], gyro[1], gyro[2]) * m_gyroscale.array() - m_gyrobias.array();
	m_accel = (Eigen::Vector3d(accel[0], accel[1], accel[2]) - m_accelbias).array() * m_accelscale.array();
	m_mag = (Eigen::Vector3d(mag[0], mag[1], mag[2]) - m_magbias).array() * m_magscale.array();
	new_samples(interval);
}

void AHRS::new_samples(unsigned int interval, const double gyro[3], const double accel[3], const double mag[3])
{
	m_gyro = Eigen::Array3d(gyro[0], gyro[1], gyro[2]) * m_gyroscale.array() - m_gyrobias.array();
	m_accel = (Eigen::Vector3d(accel[0], accel[1], accel[2]) - m_accelbias).array() * m_accelscale.array();
	m_mag = (Eigen::Vector3d(mag[0], mag[1], mag[2]) - m_magbias).array() * m_magscale.array();
	new_samples(interval);
}

void AHRS::new_gyro_samples(unsigned int interval, const double gyro[3])
{
	m_gyro = Eigen::Array3d(gyro[0], gyro[1], gyro[2]) * m_gyroscale.array() - m_gyrobias.array();
	new_samples(interval);
}

void AHRS::new_accel_samples(const double accel[3])
{
	m_accel = (Eigen::Vector3d(accel[0], accel[1], accel[2]) - m_accelbias).array() * m_accelscale.array();
}

void AHRS::new_mag_samples(const double mag[3])
{
	m_mag = (Eigen::Vector3d(mag[0], mag[1], mag[2]) - m_magbias).array() * m_magscale.array();
}

void AHRS::new_samples(unsigned int interval)
{
	if (false)
		std::cerr << "AHRS: in: gyro: " << Eigen::RowVector3d(m_gyro)
			  << " scale " << Eigen::RowVector3d(m_gyroscale)
			  << " bias " << Eigen::RowVector3d(m_gyrobias) << std::endl
			  << "  accel: " << Eigen::RowVector3d(m_accel)
			  << " scale " << Eigen::RowVector3d(m_accelscale) << ' ' << m_accelscale2
			  << " bias " << Eigen::RowVector3d(m_accelbias) << std::endl
			  << "  mag: " << Eigen::RowVector3d(m_mag)
			  << " scale " << Eigen::RowVector3d(m_magscale)  << ' ' << m_magscale2
			  << " bias " << Eigen::RowVector3d(m_magbias) << std::endl
			  << "  q: " << Eigen::RowVector4d(m_q.coeffs()) << std::endl;
	if (m_init) {
		if (std::isnan(m_accel[0]) || std::isnan(m_accel[1]) || std::isnan(m_accel[2]) ||
		    std::isnan(m_mag[0]) || std::isnan(m_mag[1]) || std::isnan(m_mag[2]))
			return;
		Eigen::Vector3d accelxmag(m_accel.cross(m_mag));
		if (Eigen::Vector3d(m_accel).squaredNorm() < 1e-30 || accelxmag.squaredNorm() < 1e-30)
			return;
		m_init = false;
		// initialize acceleration filter to current acceleration vector
		m_accelfilt = m_accel;
		m_gyrofilt = m_gyro;
		// compute quaternion that rotates yA to A
		{
			Eigen::Vector3d u(m_accel);
			Eigen::Vector3d v(0, 0, 1);
			u.normalize();
			m_q.w() = u.dot(v) + 1;
			m_q.vec() = u.cross(v);
			m_q.normalize();
		}
		// compute quaternion that rotates q * (yA x yB) * q^-1 to C
		{
			Eigen::Vector3d u((m_q * to_quaternion(accelxmag) * m_q.conjugate()).vec());
			Eigen::Vector3d v(0, 1, 0);
			u.normalize();
			Eigen::Quaterniond q;
			q.w() = u.dot(v) + 1;
			q.vec() = u.cross(v);
			q.normalize();
			m_q = q * m_q;
		}
		if (false) {
			std::cerr << "Init: Accel " << m_accel << " -> "
				  << (m_q * to_quaternion(m_accel) * m_q.conjugate()).vec() << std::endl
				  << "Init: Accel x Mag " << accelxmag << " -> "
				  << (m_q * to_quaternion(accelxmag) * m_q.conjugate()).vec() << std::endl;
		}
	}
	for (unsigned int i = 0; i < interval; ++i) {
		m_accelfilt += (m_accel - m_accelfilt) * 0.03;
		m_gyrofilt += (m_gyro - m_gyrofilt) * 0.01;
	}
	switch (m_mode) {
	case mode_normal:
	case mode_fast:
	case mode_slow:
	case mode_gyroonly:
	{
		Eigen::Vector3d EA(Eigen::Vector3d(0, 0, 1) - (1.0 / m_accelscale2) * (m_q * to_quaternion(m_accel) * m_q.conjugate()).vec());
		Eigen::Vector3d yc(m_accel.cross(m_mag));
		Eigen::Vector3d EC(Eigen::Vector3d(0, 1, 0) - (1.0 / m_magscale2) * (m_q * to_quaternion(yc) * m_q.conjugate()).vec());
		Eigen::Vector3d yd(yc.cross(m_accel));
		Eigen::Vector3d ED(Eigen::Vector3d(1, 0, 0) - (1.0 / (m_accelscale2 * m_magscale2)) * (m_q * to_quaternion(yd) * m_q.conjugate()).vec());
		double coeff_la(m_coeff_la);
		double coeff_lc(m_coeff_lc);
		double coeff_ld(m_coeff_ld);
		double coeff_sigma(m_coeff_sigma);
		double coeff_n(m_coeff_n);
		double coeff_o(m_coeff_o);
		if (m_mode == mode_fast) {
			coeff_la *= 5;
			coeff_lc *= 5;
			coeff_ld *= 5;
			coeff_sigma *= 5;
			coeff_n *= 5;
			coeff_o *= 5;
		} else if (m_mode == mode_slow) {
			coeff_la *= 0.2;
			coeff_lc *= 0.2;
			coeff_ld *= 0.2;
			coeff_sigma *= 0.2;
			coeff_n *= 0.2;
			coeff_o *= 0.2;
		}
		Eigen::Vector3d LE(coeff_la * Eigen::Vector3d(0, 0, 1).cross(EA) +
				   coeff_lc * Eigen::Vector3d(0, 1, 0).cross(EC) +
				   coeff_ld * Eigen::Vector3d(1, 0, 0).cross(ED));
		Eigen::Vector3d ME(-coeff_sigma * LE);
		double NE((coeff_la * EA.dot(EA - Eigen::Vector3d(0, 0, 1)) +
			   coeff_ld * ED.dot(ED - Eigen::Vector3d(1, 0, 0))) *
			  coeff_n / (coeff_la + coeff_ld));
		double OE((coeff_lc * EA.dot(EC - Eigen::Vector3d(0, 1, 0)) +
			   coeff_ld * ED.dot(ED - Eigen::Vector3d(1, 0, 0))) *
			  coeff_o / (coeff_lc + coeff_ld));
		Eigen::Quaterniond qd(0.5 * (m_q * to_quaternion(m_gyro)));
		Eigen::Quaterniond omegabd(0, 0, 0, 0);
		double asd(0);
		double csd(0);
		if (m_mode != mode_gyroonly) {
			qd += to_quaternion(LE) * m_q;
			omegabd = m_q.conjugate() * to_quaternion(ME) * m_q;
			asd = m_accelscale2 * NE;
			csd = m_magscale2 * OE;
		}
		{
			double t(interval * m_sampletime);
			m_q += t * qd;
			m_gyrobias += t * omegabd.vec();
			m_accelscale2 += t * asd;
			m_magscale2 += t * csd;
			m_q.normalize();
		}
		if (false) {
			std::cout << "AHRS: q " << Eigen::RowVector4d(m_q.coeffs())
				  << " omega " << Eigen::RowVector3d(m_gyrobias)
				  << " a " << m_accelscale2
				  << " c " << m_magscale2 << std::endl;
		}
		break;
	}

	default:
		if (true)
			std::cout << "AHRS: unknown mode: " << m_mode << std::endl;
		break;
	}
	if (false)
		std::cerr << "AHRS: out: gyro: " << Eigen::RowVector3d(m_gyro)
			  << " scale " << Eigen::RowVector3d(m_gyroscale)
			  << " bias " << Eigen::RowVector3d(m_gyrobias) << std::endl
			  << "  accel: " << Eigen::RowVector3d(m_accel)
			  << " scale " << Eigen::RowVector3d(m_accelscale) << ' ' << m_accelscale2
			  << " bias " << Eigen::RowVector3d(m_accelbias) << std::endl
			  << "  mag: " << Eigen::RowVector3d(m_mag)
			  << " scale " << Eigen::RowVector3d(m_magscale)  << ' ' << m_magscale2
			  << " bias " << Eigen::RowVector3d(m_magbias) << std::endl
			  << "  q: " << Eigen::RowVector4d(m_q.coeffs()) << std::endl;
}
