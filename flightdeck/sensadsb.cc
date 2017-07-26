//
// C++ Implementation: ADS-B Decoder
//
// Description: ADS-B Decoder
//
//
// Author: Thomas Sailer <t.sailer@alumni.ethz.ch>, (C) 2011, 2012, 2013
//
// Copyright: See COPYING file that comes with this distribution
//
//

#include <glibmm/datetime.h>
#include <iomanip>

#include "sensadsb.h"

Sensors::SensorADSB::SensorADSB(Sensors& sensors, const Glib::ustring& configgroup)
	: SensorInstance(sensors, configgroup), m_ownship(0x4b27cd), m_correctbiterrors(0),
	  m_positionpriority(1), m_baroaltitudepriority(1), m_gnssaltitudepriority(1), m_coursepriority(1), m_headingpriority(1)
{
	// get configuration
        const Glib::KeyFile& cf(get_sensors().get_configfile());
        if (!cf.has_key(get_configgroup(), "positionpriority"))
		get_sensors().get_configfile().set_integer(get_configgroup(), "positionpriority", m_positionpriority);
	m_positionpriority = cf.get_integer(get_configgroup(), "positionpriority");
	if (!cf.has_key(get_configgroup(), "baroaltitudepriority"))
		get_sensors().get_configfile().set_integer(get_configgroup(), "baroaltitudepriority", m_baroaltitudepriority);
	m_baroaltitudepriority = cf.get_integer(get_configgroup(), "baroaltitudepriority");
	if (!cf.has_key(get_configgroup(), "gnssaltitudepriority"))
		get_sensors().get_configfile().set_integer(get_configgroup(), "gnssaltitudepriority", m_gnssaltitudepriority);
	m_gnssaltitudepriority = cf.get_integer(get_configgroup(), "gnssaltitudepriority");
        if (!cf.has_key(get_configgroup(), "coursepriority"))
		get_sensors().get_configfile().set_integer(get_configgroup(), "coursepriority", m_coursepriority);
	m_coursepriority = cf.get_integer(get_configgroup(), "coursepriority");
        if (!cf.has_key(get_configgroup(), "headingpriority"))
		get_sensors().get_configfile().set_integer(get_configgroup(), "headingpriority", m_headingpriority);
	m_headingpriority = cf.get_integer(get_configgroup(), "headingpriority");
        if (!cf.has_key(get_configgroup(), "ownship"))
		get_sensors().get_configfile().set_integer(get_configgroup(), "ownship", m_ownship);
	m_ownship = cf.get_integer(get_configgroup(), "ownship") & 0xffffff;
        if (!cf.has_key(get_configgroup(), "correctbiterrors"))
		get_sensors().get_configfile().set_integer(get_configgroup(), "correctbiterrors", m_correctbiterrors);
	m_correctbiterrors = cf.get_integer(get_configgroup(), "correctbiterrors");
	{
		std::pair<adsbtargets_t::const_iterator,bool> ins(m_targets.insert(ADSBTarget(m_ownship, true)));
		ADSBTarget& t(const_cast<ADSBTarget&>(*ins.first));
		t.set_ownship(true);
		m_ownshipptr = ins.first;
	}
        if (cf.has_key(get_configgroup(), "tracefile")) {
		std::string fn(cf.get_string(get_configgroup(), "tracefile"));
		m_tracefile.open(fn.c_str(), std::ofstream::out | std::ofstream::app);
		if (!m_tracefile.is_open())
			get_sensors().log(Sensors::loglevel_warning, std::string("ADS-B: cannot open trace file ") + fn);
	}
}

Sensors::SensorADSB::~SensorADSB()
{
	m_connexpire.disconnect();
}

unsigned int Sensors::SensorADSB::get_position_priority(void) const
{
	return m_positionpriority;
}

unsigned int Sensors::SensorADSB::get_truealt_priority(void) const
{
	return m_gnssaltitudepriority;
}

unsigned int Sensors::SensorADSB::get_baroalt_priority(void) const
{
	return m_baroaltitudepriority;
}

unsigned int Sensors::SensorADSB::get_course_priority(void) const
{
	return m_coursepriority;
}

unsigned int Sensors::SensorADSB::get_heading_priority(void) const
{
	return m_headingpriority;
}

bool Sensors::SensorADSB::get_position(Point& pt) const
{
	pt.set_invalid();
	const ADSBTarget& os(*m_ownshipptr);
	if (os.empty())
		return false;
	pt = os.back().get_coord();
	return !pt.is_invalid();
}

Glib::TimeVal Sensors::SensorADSB::get_position_time(void) const
{
	const ADSBTarget& os(*m_ownshipptr);
	if (os.empty())
		return Glib::TimeVal(-1, 0);
	return os.back().get_timestamp();
}

void Sensors::SensorADSB::get_baroalt(double& alt, double& altrate) const
{
	alt = altrate = std::numeric_limits<double>::quiet_NaN();
	const ADSBTarget& os(*m_ownshipptr);
	if (!os.get_baroalttimestamp().valid())
		return;
	if (os.get_baroalt() == std::numeric_limits<int32_t>::min())
		return;
	alt = os.get_baroalt();
	if (os.get_vmotion() == ADSBTarget::vmotion_baro)
		altrate = os.get_verticalspeed();
}

Glib::TimeVal Sensors::SensorADSB::get_baroalt_time(void) const
{
	const ADSBTarget& os(*m_ownshipptr);
	return os.get_baroalttimestamp();
}

void Sensors::SensorADSB::get_truealt(double& alt, double& altrate) const
{
	alt = altrate = std::numeric_limits<double>::quiet_NaN();
	const ADSBTarget& os(*m_ownshipptr);
	if (!os.get_gnssalttimestamp().valid())
		return;
	if (os.get_gnssalt() == std::numeric_limits<int32_t>::min())
		return;
	alt = os.get_gnssalt();
	if (os.get_vmotion() == ADSBTarget::vmotion_gnss)
		altrate = os.get_verticalspeed();
}

Glib::TimeVal Sensors::SensorADSB::get_truealt_time(void) const
{
	const ADSBTarget& os(*m_ownshipptr);
	return os.get_gnssalttimestamp();
}

void Sensors::SensorADSB::get_course(double& crs, double& gs) const
{
	crs = gs = std::numeric_limits<double>::quiet_NaN();
	const ADSBTarget& os(*m_ownshipptr);
	if (!os.get_motiontimestamp().valid())
		return;
	if (os.get_lmotion() != ADSBTarget::lmotion_gnd)
		return;
	gs = os.get_speed();
	crs = (360.0 / (1U << 16)) * os.get_direction();
}

Glib::TimeVal Sensors::SensorADSB::get_course_time(void) const
{
	const ADSBTarget& os(*m_ownshipptr);
	return os.get_motiontimestamp();
}

void Sensors::SensorADSB::get_heading(double& hdg) const
{
	hdg = std::numeric_limits<double>::quiet_NaN();
	const ADSBTarget& os(*m_ownshipptr);
	if (!os.get_motiontimestamp().valid())
		return;
	if (os.get_lmotion() != ADSBTarget::lmotion_ias &&
	    os.get_lmotion() != ADSBTarget::lmotion_tas)
		return;
	hdg = (360.0 / (1U << 16)) * os.get_direction();
}

Glib::TimeVal Sensors::SensorADSB::get_heading_time(void) const
{
	const ADSBTarget& os(*m_ownshipptr);
	return os.get_motiontimestamp();
}

bool Sensors::SensorADSB::is_heading_true(void) const
{
	return true;
}

void Sensors::SensorADSB::receive(const ModeSMessage& msg)
{
	if (true) {
		std::cerr << "ADSB: " << msg.get_raw_string();
		if (msg.is_adsb() && !msg.crc()) {
			uint32_t addr((msg[1] << 16) | (msg[2] << 8) | msg[3]);
			std::ostringstream oss;
			oss << std::hex << std::uppercase << std::setw(6) << std::setfill('0') << addr;
			std::cerr << " ADSB " << oss.str() << " type " << (unsigned int)msg.get_adsb_type();
		}
		std::cerr << std::endl;
	}
	ModeSMessage msg1(msg);
	Glib::TimeVal tv;
	tv.assign_current_time();
	ParamChanged pc;
	bool frameok(false);
	int frameerr(0);
	switch (msg1.get_format()) {
	case 11:
	{
		if (msg1.is_long())
			break;
		if (m_correctbiterrors) {
			frameerr = msg1.correct();
			if (frameerr < 0 || frameerr > m_correctbiterrors)
				break;
			if (true && msg1.crc()) {
				std::cerr << "ADSB: error correction failed: " << msg1.get_raw_string()
					  << " (original " << msg.get_raw_string() << ')' << std::endl;
				break;
			}
		} else if (msg1.crc()) {
			break;
		}
		frameok = true;
		uint32_t addr((msg1[1] << 16) | (msg1[2] << 8) | msg1[3]);
		std::pair<adsbtargets_t::const_iterator,bool> ins(m_targets.insert(ADSBTarget(tv, addr, false)));
		ADSBTarget& t(const_cast<ADSBTarget&>(*ins.first));
		pc.set_changed(parnrtargets);
		break;
	}

	case 0:
	case 16:
	{
		if (msg1.is_long() == !(msg1.get_format() & 16))
			break;
		adsbtargets_t::const_iterator ti;
		if (m_correctbiterrors) {
			adsbtargets_t::const_iterator te(m_targets.end());
			for (ti = m_targets.begin(); ti != te; ++ti) {
				ModeSMessage msg2(msg1);
				frameerr = msg2.correct(ti->get_icaoaddr());
				if (frameerr < 0 || frameerr > m_correctbiterrors)
					continue;
				if (true && msg2.addr() != ti->get_icaoaddr()) {
					std::cerr << "ADSB: error correction failed: " << msg2.get_raw_string()
						  << " (original " << msg.get_raw_string() << ')' << std::endl;
					continue;
				}
				msg1 = msg2;
				break;
			}
			if (ti == m_targets.end())
				break;
		} else {
			ti = m_targets.find(ADSBTarget(msg1.addr()));
			if (ti == m_targets.end())
				break;
		}
		frameok = true;
		ADSBTarget& t(const_cast<ADSBTarget&>(*ti));
		uint16_t ac((msg1[2] << 8) | msg1[3]);
		ac &= 0x1fff;
		t.set_baroalt(tv, ModeSMessage::decode_alt13(ac));
		pc.set_changed(parnrtargets);
		if (t.is_ownship())
			pc.set_changed(parnrbaroalt);
		break;
	}

	case 4:
	case 20:
	{
		if (msg1.is_long() == !(msg1.get_format() & 16))
			break;
		adsbtargets_t::const_iterator ti;
		if (m_correctbiterrors) {
			adsbtargets_t::const_iterator te(m_targets.end());
			for (ti = m_targets.begin(); ti != te; ++ti) {
				ModeSMessage msg2(msg1);
				frameerr = msg2.correct(ti->get_icaoaddr());
				if (frameerr < 0 || frameerr > m_correctbiterrors)
					continue;
				if (true && msg2.addr() != ti->get_icaoaddr()) {
					std::cerr << "ADSB: error correction failed: " << msg2.get_raw_string()
						  << " (original " << msg.get_raw_string() << ')' << std::endl;
					continue;
				}
				msg1 = msg2;
				break;
			}
			if (ti == m_targets.end())
				break;
		} else {
			ti = m_targets.find(ADSBTarget(msg1.addr()));
			if (ti == m_targets.end())
				break;
		}
		frameok = true;
		ADSBTarget& t(const_cast<ADSBTarget&>(*ti));
		uint16_t ac((msg1[2] << 8) | msg1[3]);
		ac &= 0x1fff;
		// Flight Status: (msg1[0] & 7)
		t.set_baroalt(tv, ModeSMessage::decode_alt13(ac));
		pc.set_changed(parnrtargets);
		if (t.is_ownship())
			pc.set_changed(parnrbaroalt);
		break;
	}

	case 5:
	case 21:
	{
		if (msg1.is_long() == !(msg1.get_format() & 16))
			break;
		adsbtargets_t::const_iterator ti;
		if (m_correctbiterrors) {
			adsbtargets_t::const_iterator te(m_targets.end());
			for (ti = m_targets.begin(); ti != te; ++ti) {
				ModeSMessage msg2(msg1);
				frameerr = msg2.correct(ti->get_icaoaddr());
				if (frameerr < 0 || frameerr > m_correctbiterrors)
					continue;
				if (true && msg2.addr() != ti->get_icaoaddr()) {
					std::cerr << "ADSB: error correction failed: " << msg2.get_raw_string()
						  << " (original " << msg.get_raw_string() << ')' << std::endl;
					continue;
				}
				msg1 = msg2;
				break;
			}
			if (ti == m_targets.end())
				break;
		} else {
			ti = m_targets.find(ADSBTarget(msg1.addr()));
			if (ti == m_targets.end())
				break;
		}
		frameok = true;
		ADSBTarget& t(const_cast<ADSBTarget&>(*ti));
		uint16_t id((msg1[2] << 8) | msg1[3]);
		id &= 0x1fff;
		// Flight Status: (msg1[0] & 7)
		t.set_modea(tv, ModeSMessage::decode_identity(id));
		pc.set_changed(parnrtargets);
		break;
	}

	case 17:
	case 18:
	{
		if (m_correctbiterrors) {
			frameerr = msg1.correct();
			if (frameerr < 0 || frameerr > m_correctbiterrors)
				break;
			if (true && msg1.crc()) {
				std::cerr << "ADSB: error correction failed: " << msg1.get_raw_string()
					  << " (original " << msg.get_raw_string() << ')' << std::endl;
				break;
			}
		} else if (msg1.crc()) {
			break;
		}
		frameok = true;
		uint32_t addr((msg1[1] << 16) | (msg1[2] << 8) | msg1[3]);
		std::pair<adsbtargets_t::const_iterator,bool> ins(m_targets.insert(ADSBTarget(tv, addr, false)));
		ADSBTarget& t(const_cast<ADSBTarget&>(*ins.first));
		pc.set_changed(parnrtargets);
		if (msg1.is_adsb_ident_category())
			t.set_ident(tv, msg1.get_adsb_ident(), msg1.get_adsb_emittercategory());
		if (msg1.is_adsb_modea())
			t.set_modea(tv, ModeSMessage::decode_identity(msg1.get_adsb_modea()), msg1.get_adsb_emergencystate());
		bool newpos(false);
		if (msg1.is_adsb_position()) {
			Point pt;
			pt.set_invalid();
			for (unsigned int i = t.size(); i > 0; ) {
				--i;
				const ADSBTarget::Position& pos(t[i]);
				if (pos.get_cprformat() == msg1.get_adsb_cprformat())
					continue;
				Glib::TimeVal tvdiff(tv - pos.get_timestamp());
				tvdiff.subtract_seconds(10);
				if (!tvdiff.negative())
					continue;
				if (msg1.get_adsb_cprformat()) {
					std::pair<Point,Point> r(ModeSMessage::decode_global_cpr17(pos.get_cprlat(), pos.get_cprlon(),
												   msg1.get_adsb_cprlat(), msg1.get_adsb_cprlon()));
					pt = r.second;
				} else {
					std::pair<Point,Point> r(ModeSMessage::decode_global_cpr17(msg1.get_adsb_cprlat(), msg1.get_adsb_cprlon(),
												   pos.get_cprlat(), pos.get_cprlon()));
					pt = r.first;
				}
				if (!pt.is_invalid())
					break;
			}
			if (pt.is_invalid()) {
				if (!t.empty())
					pt = t.back().get_coord();
				if (pt.is_invalid()) {
					if (!get_sensors().get_position(pt))
						pt.set_invalid();
				}
				if (!pt.is_invalid())
					pt = ModeSMessage::decode_local_cpr17(msg1.get_adsb_cprlat(), msg1.get_adsb_cprlon(), msg1.get_adsb_cprformat(), pt);
			}
			ADSBTarget::Position::altmode_t altmode(ADSBTarget::Position::altmode_invalid);
			uint16_t alt(0);
			if (msg1.is_adsb_baroalt()) {
				altmode = ADSBTarget::Position::altmode_baro;
				alt = msg1.get_adsb_altitude();
			}
			if (msg1.is_adsb_gnssheight()) {
				altmode = ADSBTarget::Position::altmode_gnss;
				alt = msg1.get_adsb_altitude();
			}
			t.add_position(ADSBTarget::Position(tv, pt, msg1.get_adsb_type(), msg1.get_adsb_cprlat(), msg1.get_adsb_cprlon(),
							    msg1.get_adsb_cprformat(), msg1.get_adsb_timebit(), altmode, alt));
			if (t.is_ownship())
				pc.set_changed(parnrlat, parnrlon);
			newpos = !pt.is_invalid();
		}
		if (msg1.is_adsb_baroalt()) {
			t.set_baroalt(tv, ModeSMessage::decode_alt12(msg1.get_adsb_altitude()));
			if (t.is_ownship())
				pc.set_changed(parnrbaroalt);
		}
		if (msg1.is_adsb_gnssheight()) {
			t.set_gnssalt(tv, msg1.get_adsb_altitude());
			if (t.is_ownship())
				pc.set_changed(parnrgnssalt);
		}
		if (msg1.is_adsb_velocity_cartesian()) {
			int16_t vn1(msg1.get_adsb_velocity_north());
			int16_t ve1(msg1.get_adsb_velocity_east());
			uint16_t crs(0);
			int16_t speed(0);
			ADSBTarget::lmotion_t lmotion(ADSBTarget::lmotion_gnd);
			if (vn1 == std::numeric_limits<int16_t>::min() || ve1 == std::numeric_limits<int16_t>::min()) {
				lmotion = ADSBTarget::lmotion_invalid;
			} else {
				double vn(vn1);
				double ve(ve1);
				crs = Point::round<int,double>(atan2(ve, vn) * (32768.0 / M_PI));
				speed = Point::round<int,double>(sqrt(vn * vn + ve * ve));
			}
			int32_t vs(msg1.get_adsb_vertical_rate());
			ADSBTarget::vmotion_t vmotion(msg1.get_adsb_vertical_source_baro() ?
						      ADSBTarget::vmotion_baro : ADSBTarget::vmotion_gnss);
			if (vs == std::numeric_limits<int32_t>::min()) {
				vmotion = ADSBTarget::vmotion_invalid;
				vs = 0;
			}
			t.set_motion(tv, speed, crs, lmotion, vs, vmotion);
			if (t.is_ownship()) {
				pc.set_changed(parnrcourse, parnrgroundspeed);
				pc.set_changed(parnrbaroaltrate);
				pc.set_changed(parnrgnssaltrate);
			}
		}
		if (msg1.is_adsb_velocity_polar()) {
			int16_t as(msg1.get_adsb_airspeed());
			ADSBTarget::lmotion_t lmotion(msg1.get_adsb_airspeed_tas() ? ADSBTarget::lmotion_tas : ADSBTarget::lmotion_ias);
			if (as == std::numeric_limits<int16_t>::min()) {
				lmotion = ADSBTarget::lmotion_invalid;
				as = 0;
			}
			int32_t vs(msg1.get_adsb_vertical_rate());
			ADSBTarget::vmotion_t vmotion(msg1.get_adsb_vertical_source_baro() ?
						      ADSBTarget::vmotion_baro : ADSBTarget::vmotion_gnss);
			if (vs == std::numeric_limits<int32_t>::min()) {
				vmotion = ADSBTarget::vmotion_invalid;
				vs = 0;
			}
			t.set_motion(tv, as, msg1.get_adsb_heading(), lmotion, vs, vmotion);
			if (t.is_ownship()) {
				pc.set_changed(parnrcourse, parnrgroundspeed);
				pc.set_changed(parnrbaroaltrate);
				pc.set_changed(parnrgnssaltrate);
			}
		}
		if (newpos) {
			MapAircraft acft(t);
			get_sensors().set_mapaircraft(acft);
		}
		break;
	}

	default:
		break;
	}
	if (frameok && m_tracefile.is_open()) {
		m_tracefile << msg1.get_msg_string(true) << ' ' << tv.as_iso8601();
		if (frameerr)
			m_tracefile << " (bit errors " << frameerr << " original " << msg.get_msg_string(true) << ')';
		m_tracefile << std::endl;
	}
	expire_targets(pc);
	update(pc);
}

void Sensors::SensorADSB::clear(void)
{
	m_connexpire.disconnect();
	m_targets.clear();
	{
		std::pair<adsbtargets_t::const_iterator,bool> ins(m_targets.insert(ADSBTarget(m_ownship, true)));
		ADSBTarget& t(const_cast<ADSBTarget&>(*ins.first));
		t.set_ownship(true);
		m_ownshipptr = ins.first;
	}
}

void Sensors::SensorADSB::expire_targets(ParamChanged& pc)
{
	m_connexpire.disconnect();
	Glib::TimeVal tv;
	tv.assign_current_time();
	tv.subtract_seconds(120);
	Glib::TimeVal tvold(std::numeric_limits<long>::max(), 0);
	adsbtargets_t::iterator ti(m_targets.begin()), te(m_targets.end());
	while (ti != te) {
		if (ti->is_ownship()) {
			++ti;
			continue;
		}
		if (ti->get_timestamp() < tv) {
			adsbtargets_t::iterator ti2(ti);
			++ti;
			m_targets.erase(ti2);
			pc.set_changed(parnrtargets);
			continue;
		}
		if (ti->get_timestamp() < tvold)
			tvold = ti->get_timestamp();
		++ti;
	}
	tvold -= tv;
	m_connexpire = Glib::signal_timeout().connect(sigc::mem_fun(*this, &SensorADSB::expire), tvold.tv_sec * 1000 + (tvold.tv_usec + 999) / 1000);
}

bool Sensors::SensorADSB::expire(void)
{
	ParamChanged pc;
	expire_targets(pc);
	if (pc)
		update(pc);
	return false;
}

void Sensors::SensorADSB::get_param_desc(unsigned int pagenr, paramdesc_t& pd)
{
	SensorInstance::get_param_desc(pagenr, pd);
	if (pagenr)
		return;

	pd.push_back(ParamDesc(ParamDesc::type_section, ParamDesc::editable_readonly, ~0U, "Own Ship", "Own Ship State", ""));
	pd.push_back(ParamDesc(ParamDesc::type_string, ParamDesc::editable_readonly, parnrtime, "Time UTC", "Last Received Time in UTC", ""));
	pd.push_back(ParamDesc(ParamDesc::type_string, ParamDesc::editable_readonly, parnrlat, "Latitude", "Latitude", ""));
	pd.push_back(ParamDesc(ParamDesc::type_string, ParamDesc::editable_readonly, parnrlon, "Longitude", "Longitude", ""));
	pd.push_back(ParamDesc(ParamDesc::type_double, ParamDesc::editable_readonly, parnrbaroalt, "Baro Alt", "Baro Altitude", "ft", 0, -99999.0, 99999.0, 1.0, 10.0));
	pd.push_back(ParamDesc(ParamDesc::type_double, ParamDesc::editable_readonly, parnrbaroaltrate, "Baro Climb", "Baro Climb Rate", "ft/min", 0, -9999.0, 9999.0, 1.0, 10.0));
	pd.push_back(ParamDesc(ParamDesc::type_double, ParamDesc::editable_readonly, parnrgnssalt, "GNSS Alt", "True Altitude", "ft", 0, -99999.0, 99999.0, 1.0, 10.0));
	pd.push_back(ParamDesc(ParamDesc::type_double, ParamDesc::editable_readonly, parnrgnssaltrate, "GNSS Climb", "True Climb Rate", "ft/min", 0, -9999.0, 9999.0, 1.0, 10.0));
	pd.push_back(ParamDesc(ParamDesc::type_double, ParamDesc::editable_readonly, parnrcourse, "Course", "Course", "°", 0, -999.0, 999.0, 1.0, 10.0));
	pd.push_back(ParamDesc(ParamDesc::type_double, ParamDesc::editable_readonly, parnrgroundspeed, "Speed", "Speed", "kt", 0, 0.0, 999.0, 1.0, 10.0));

	pd.push_back(ParamDesc(ParamDesc::type_section, ParamDesc::editable_readonly, ~0U, "Priorities", "Priority Values", ""));
	pd.push_back(ParamDesc(ParamDesc::type_string, ParamDesc::editable_admin, parnrownship, "Own Ship", "Own Ship Hex Address", ""));
	pd.push_back(ParamDesc(ParamDesc::type_string, ParamDesc::editable_readonly, parnrownshipreg, "RegNr", "Own Ship Registration Number/Marks, if known", ""));
	pd.push_back(ParamDesc(ParamDesc::type_string, ParamDesc::editable_readonly, parnrownshipctry, "Country", "Own Ship Country", ""));
	pd.push_back(ParamDesc(ParamDesc::type_integer, ParamDesc::editable_admin, parnrcorrectberr, "Bit Errors", "Maximum Number of Bit Errors corrected (0..2)", "", 0, 0, 2, 1, 1));
	pd.push_back(ParamDesc(ParamDesc::type_integer, ParamDesc::editable_admin, parnrposprio, "Pos Prio", "Position Priority; higher values mean this sensor is preferred to other sensors delivering position information", "", 0, 0, 9999, 1, 10));
	pd.push_back(ParamDesc(ParamDesc::type_integer, ParamDesc::editable_admin, parnrbaroaltprio, "Baro Alt Prio", "Baro Altitude Priority; higher values mean this sensor is preferred to other sensors delivering altitude information", "", 0, 0, 9999, 1, 10));
	pd.push_back(ParamDesc(ParamDesc::type_integer, ParamDesc::editable_admin, parnrgnssaltprio, "GNSS Alt Prio", "GNSS Altitude Priority; higher values mean this sensor is preferred to other sensors delivering altitude information", "", 0, 0, 9999, 1, 10));
	pd.push_back(ParamDesc(ParamDesc::type_integer, ParamDesc::editable_admin, parnrcrsprio, "Course Prio", "Course Priority; higher values mean this sensor is preferred to other sensors delivering course information", "", 0, 0, 9999, 1, 10));
	pd.push_back(ParamDesc(ParamDesc::type_integer, ParamDesc::editable_admin, parnrhdgprio, "Heading Prio", "Heading Priority; higher values mean this sensor is preferred to other sensors delivering course information", "", 0, 0, 9999, 1, 10));

	pd.push_back(ParamDesc(ParamDesc::type_adsbtargets, ParamDesc::editable_readonly, parnrtargets, "ADS-B", "ADS-B Targets", ""));
}

Sensors::SensorADSB::paramfail_t Sensors::SensorADSB::get_param(unsigned int nr, Glib::ustring& v) const
{
	switch (nr) {
	default:
		break;

	case parnrtime:
	{
		v.clear();
		Glib::TimeVal t(m_ownshipptr->get_timestamp());
		if (!t.valid())
			return paramfail_fail;
		Glib::DateTime dt(Glib::DateTime::create_now_utc(t));
		v = dt.format("%Y-%m-%d %H:%M:%S");
		return paramfail_ok;
	}

	case parnrlat:
	{
		v.clear();
		Point pt;
		if (!is_position_ok() || !get_position(pt))
			return paramfail_fail;
		v = pt.get_lat_str();
		return paramfail_ok;
	}

	case parnrlon:
	{
		v.clear();
		Point pt;
		if (!is_position_ok() || !get_position(pt))
			return paramfail_fail;
		v = pt.get_lon_str();
		return paramfail_ok;
	}

	case parnrownship:
	{
		std::ostringstream oss;
		oss << std::hex << std::setfill('0') << std::setw(6) << std::uppercase << m_ownship;
		v = oss.str();
		return paramfail_ok;
	}

	case parnrownshipreg:
		v = ModeSMessage::decode_registration(m_ownship);
		return paramfail_ok;

	case parnrownshipctry:
		v = ModeSMessage::decode_registration_country(m_ownship);
		return paramfail_ok;
	}
	return SensorInstance::get_param(nr, v);
}

Sensors::SensorADSB::paramfail_t Sensors::SensorADSB::get_param(unsigned int nr, double& v) const
{
	switch (nr) {
	default:
		break;

	case parnrbaroalt:
	{
		if (!is_baroalt_ok()) {
			v = std::numeric_limits<double>::quiet_NaN();
			return paramfail_fail;
		}
		double v1;
		get_baroalt(v, v1);
		return std::isnan(v) ? paramfail_fail : paramfail_ok;
	}

	case parnrbaroaltrate:
	{
		if (!is_baroalt_ok()) {
			v = std::numeric_limits<double>::quiet_NaN();
			return paramfail_fail;
		}
		double v1;
		get_baroalt(v1, v);
		return std::isnan(v) ? paramfail_fail : paramfail_ok;
	}

	case parnrgnssalt:
	{
		if (!is_truealt_ok()) {
			v = std::numeric_limits<double>::quiet_NaN();
			return paramfail_fail;
		}
		double v1;
		get_truealt(v, v1);
		return std::isnan(v) ? paramfail_fail : paramfail_ok;
	}

	case parnrgnssaltrate:
	{
		if (!is_truealt_ok()) {
			v = std::numeric_limits<double>::quiet_NaN();
			return paramfail_fail;
		}
		double v1;
		get_truealt(v1, v);
		return std::isnan(v) ? paramfail_fail : paramfail_ok;
	}

	case parnrcourse:
	{
		if (!is_course_ok()) {
			v = std::numeric_limits<double>::quiet_NaN();
			return paramfail_fail;
		}
		double v1;
		get_course(v, v1);
		return std::isnan(v) ? paramfail_fail : paramfail_ok;
	}

	case parnrgroundspeed:
	{
		if (!is_course_ok()) {
			v = std::numeric_limits<double>::quiet_NaN();
			return paramfail_fail;
		}
		double v1;
		get_course(v1, v);
		return std::isnan(v) ? paramfail_fail : paramfail_ok;
	}

	}
	return SensorInstance::get_param(nr, v);
}

Sensors::SensorADSB::paramfail_t Sensors::SensorADSB::get_param(unsigned int nr, int& v) const
{
	switch (nr) {
	default:
		break;

	case parnrcorrectberr:
		v = m_correctbiterrors;
		return paramfail_ok;

	case parnrposprio:
		v = m_positionpriority;
		return paramfail_ok;

	case parnrbaroaltprio:
		v = m_baroaltitudepriority;
		return paramfail_ok;

	case parnrgnssaltprio:
		v = m_gnssaltitudepriority;
		return paramfail_ok;

	case parnrcrsprio:
		v = m_coursepriority;
		return paramfail_ok;

	case parnrhdgprio:
		v = m_headingpriority;
		return paramfail_ok;
	}
	return SensorInstance::get_param(nr, v);
}

Sensors::SensorADSB::paramfail_t Sensors::SensorADSB::get_param(unsigned int nr, adsbtargets_t& targets) const
{
	switch (nr) {
	default:
		break;

	case parnrtargets:
		targets = m_targets;
		return paramfail_ok;
	}
	return SensorInstance::get_param(nr, targets);
}

void Sensors::SensorADSB::set_param(unsigned int nr, int v)
{
	switch (nr) {
	default:
		SensorInstance::set_param(nr, v);
		return;

	case parnrcorrectberr:
		v = std::max(0, std::min(2, v));
		if (m_correctbiterrors == (unsigned int)v)
			return;
		m_correctbiterrors = v;
		get_sensors().get_configfile().set_integer(get_configgroup(), "correctbiterrors", m_correctbiterrors);
		break;

	case parnrposprio:
		if (m_positionpriority == (unsigned int)v)
			return;
		m_positionpriority = v;
		get_sensors().get_configfile().set_integer(get_configgroup(), "positionpriority", m_positionpriority);
		break;

	case parnrbaroaltprio:
		if (m_baroaltitudepriority == (unsigned int)v)
			return;
		m_baroaltitudepriority = v;
		get_sensors().get_configfile().set_integer(get_configgroup(), "baroaltitudepriority", m_baroaltitudepriority);
		break;

	case parnrgnssaltprio:
		if (m_gnssaltitudepriority == (unsigned int)v)
			return;
		m_gnssaltitudepriority = v;
		get_sensors().get_configfile().set_integer(get_configgroup(), "gnssaltitudepriority", m_gnssaltitudepriority);
		break;

	case parnrcrsprio:
		if (m_coursepriority == (unsigned int)v)
			return;
		m_coursepriority = v;
		get_sensors().get_configfile().set_integer(get_configgroup(), "coursepriority", m_coursepriority);
		break;

	case parnrhdgprio:
		if (m_headingpriority == (unsigned int)v)
			return;
		m_headingpriority = v;
		get_sensors().get_configfile().set_integer(get_configgroup(), "headingpriority", m_headingpriority);
		break;
	}
	ParamChanged pc;
	pc.set_changed(nr);
	update(pc);
}

void Sensors::SensorADSB::set_param(unsigned int nr, const Glib::ustring& v)
{
	switch (nr) {
	default:
		SensorInstance::set_param(nr, v);
		return;

	case parnrownship:
	{
		std::string vv(v);
		uint32_t os(ModeSMessage::decode_registration(vv.begin(), vv.end()));
		if (os == std::numeric_limits<uint32_t>::max() || m_ownship == os)
			return;
		{
			ADSBTarget& t(const_cast<ADSBTarget&>(*m_ownshipptr));
			t.set_ownship(false);
		}
		m_ownship = os;
		{
			std::pair<adsbtargets_t::const_iterator,bool> ins(m_targets.insert(ADSBTarget(m_ownship, true)));
			ADSBTarget& t(const_cast<ADSBTarget&>(*ins.first));
			t.set_ownship(true);
			m_ownshipptr = ins.first;
		}
		get_sensors().get_configfile().set_integer(get_configgroup(), "ownship", m_ownship);
		break;
	}

	}
	ParamChanged pc;
	pc.set_changed(nr);
	if (nr == parnrownship) {
		pc.set_changed(parnrownshipreg, parnrownshipctry);
		pc.set_changed(parnrtargets);
	}
	update(pc);
}
