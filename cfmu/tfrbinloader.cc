#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <iomanip>
#include <fstream>

#include "tfr.hh"

class TrafficFlowRestrictions::BinLoader {
public:
	BinLoader(TrafficFlowRestrictions& tfrs, std::istream& is, AirportsDb& airportdb, NavaidsDb& navaiddb,
		  WaypointsDb& waypointdb, AirwaysDb& airwaydb, AirspacesDb& airspacedb, bool v2);
	virtual ~BinLoader();

	void error(const std::string& text) const;
	void error(const std::string& child, const std::string& text) const;
	void warning(const std::string& text) const;
	void warning(const std::string& child, const std::string& text) const;
	void info(const std::string& text) const;
	void info(const std::string& child, const std::string& text) const;
	void message(Message::type_t mt, const std::string& child, const std::string& text) const;

	uint8_t loadbinu8(void) { return TrafficFlowRestrictions::loadbinu8(get_is()); }
	uint16_t loadbinu16(void) { return TrafficFlowRestrictions::loadbinu16(get_is()); }
	uint32_t loadbinu32(void) { return TrafficFlowRestrictions::loadbinu32(get_is()); }
	uint64_t loadbinu64(void) { return TrafficFlowRestrictions::loadbinu64(get_is()); }
	double loadbindbl(void) { return TrafficFlowRestrictions::loadbindbl(get_is()); }
	std::string loadbinstring(void) { return TrafficFlowRestrictions::loadbinstring(get_is()); }
	Point loadbinpt(void) { return TrafficFlowRestrictions::loadbinpt(get_is()); }

	std::istream& get_is(void) { return m_is; }
	TrafficFlowRestrictions& get_tfrs(void) const { return m_dbldr.get_tfrs(); }
	AirportsDb& get_airportdb(void) const { return m_dbldr.get_airportdb(); }
	NavaidsDb& get_navaiddb(void) const { return m_dbldr.get_navaiddb(); }
	WaypointsDb& get_waypointdb(void) const { return m_dbldr.get_waypointdb(); }
	AirwaysDb& get_airwaydb(void) const { return m_dbldr.get_airwaydb(); }
	AirspacesDb& get_airspacedb(void) const { return m_dbldr.get_airspacedb(); }

	TFRAirspace::const_ptr_t find_airspace(const std::string& id, const std::string& type) { return m_dbldr.find_airspace(id, type); }
	TFRAirspace::const_ptr_t find_airspace(const std::string& id, char bdryclass, uint8_t typecode = AirspacesDb::Airspace::typecode_ead) { return m_dbldr.find_airspace(id, bdryclass, typecode); }
	const AirportsDb::Airport& find_airport(const std::string& icao) { return m_dbldr.find_airport(icao); }
	void fill_airport_cache(void) { m_dbldr.fill_airport_cache(); }

	bool is_v2(void) const { return m_v2; }

protected:
	DbLoader m_dbldr;
	std::istream& m_is;
	bool m_v2;
};

TrafficFlowRestrictions::BinLoader::BinLoader(TrafficFlowRestrictions& tfrs, std::istream& is, AirportsDb& airportdb, NavaidsDb& navaiddb,
					      WaypointsDb& waypointdb, AirwaysDb& airwaydb, AirspacesDb& airspacedb, bool v2)
	: m_dbldr(tfrs, sigc::mem_fun(*this, &BinLoader::message), airportdb, navaiddb, waypointdb, airwaydb, airspacedb), m_is(is), m_v2(v2)
{
}

TrafficFlowRestrictions::BinLoader::~BinLoader()
{
}

void TrafficFlowRestrictions::BinLoader::message(Message::type_t mt, const std::string& child, const std::string& text) const
{
	std::string str("TFR Loader: ");
	std::string strm(str);
	std::string ruleid;
	if (!child.empty()) {
		str += child + ": ";
		strm += child + ": ";
	}
	str += text;
	strm += text;
	get_tfrs().message(strm, mt, ruleid);
	if (true)
		std::cerr << str << std::endl;
	if (mt == Message::type_error)
		throw std::runtime_error(str);	
}

void TrafficFlowRestrictions::BinLoader::error(const std::string& text) const
{
	error("", text);
}

void TrafficFlowRestrictions::BinLoader::error(const std::string& child, const std::string& text) const
{
	message(Message::type_error, child, text);
}

void TrafficFlowRestrictions::BinLoader::warning(const std::string& text) const
{
	warning("", text);
}

void TrafficFlowRestrictions::BinLoader::warning(const std::string& child, const std::string& text) const
{
	message(Message::type_warning, child, text);
}

void TrafficFlowRestrictions::BinLoader::info(const std::string& text) const
{
	info("", text);
}

void TrafficFlowRestrictions::BinLoader::info(const std::string& child, const std::string& text) const
{
	message(Message::type_info, child, text);
}

uint8_t TrafficFlowRestrictions::loadbinu8(std::istream& is)
{
	uint8_t buf[1];
	is.read(reinterpret_cast<char *>(buf), sizeof(buf));
	if (!is)
		throw std::runtime_error("loadbinu8: short read");
	return buf[0];
}

uint16_t TrafficFlowRestrictions::loadbinu16(std::istream& is)
{
	uint8_t buf[2];
	is.read(reinterpret_cast<char *>(buf), sizeof(buf));
	if (!is)
		throw std::runtime_error("loadbinu16: short read");
	return buf[0] | (buf[1] << 8);
}

uint32_t TrafficFlowRestrictions::loadbinu32(std::istream& is)
{
	uint8_t buf[4];
	is.read(reinterpret_cast<char *>(buf), sizeof(buf));
	if (!is)
		throw std::runtime_error("loadbinu32: short read");
	return buf[0] | (buf[1] << 8) | (buf[2] << 16) | (buf[3] << 24);
}

uint64_t TrafficFlowRestrictions::loadbinu64(std::istream& is)
{
	uint8_t buf[8];
	is.read(reinterpret_cast<char *>(buf), sizeof(buf));
	if (!is)
		throw std::runtime_error("loadbinu64: short read");
	uint32_t r1(buf[4] | (buf[5] << 8) | (buf[6] << 16) | (buf[7] << 24));
	uint64_t r(r1);
	r <<= 32;
	r1 = buf[0] | (buf[1] << 8) | (buf[2] << 16) | (buf[3] << 24);
	r |= r1;
	return r;
}

double TrafficFlowRestrictions::loadbindbl(std::istream& is)
{
	union {
		double d;
		uint64_t u;
	} x;
	x.u = loadbinu64(is);
	return x.d;
}

std::string TrafficFlowRestrictions::loadbinstring(std::istream& is)
{
	uint32_t len(loadbinu32(is));
	if (!len)
		return std::string();
	char buf[len];
	is.read(buf, len);
	if (!is)
		throw std::runtime_error("loadbinstring: short read");
	return std::string(buf, buf + len);
}

Point TrafficFlowRestrictions::loadbinpt(std::istream& is)
{
	Point pt;
	pt.set_lat(loadbinu32(is));
	pt.set_lon(loadbinu32(is));
	return pt;
}

void TrafficFlowRestrictions::savebinu8(std::ostream& os, uint8_t v)
{
	uint8_t buf[1];
	buf[0] = v;
	os.write(reinterpret_cast<char *>(buf), sizeof(buf));
	if (!os)
		throw std::runtime_error("savebinu8: write error");
}

void TrafficFlowRestrictions::savebinu16(std::ostream& os, uint16_t v)
{
	uint8_t buf[2];
	buf[0] = v;
	buf[1] = v >> 8;
	os.write(reinterpret_cast<char *>(buf), sizeof(buf));
	if (!os)
		throw std::runtime_error("savebinu16: write error");
}

void TrafficFlowRestrictions::savebinu32(std::ostream& os, uint32_t v)
{
	uint8_t buf[4];
	buf[0] = v;
	buf[1] = v >> 8;
	buf[2] = v >> 16;
	buf[3] = v >> 24;
	os.write(reinterpret_cast<char *>(buf), sizeof(buf));
	if (!os)
		throw std::runtime_error("savebinu32: write error");
}

void TrafficFlowRestrictions::savebinu64(std::ostream& os, uint64_t v)
{
	uint8_t buf[8];
	buf[0] = v;
	buf[1] = v >> 8;
	buf[2] = v >> 16;
	buf[3] = v >> 24;
	buf[4] = v >> 32;
	buf[5] = v >> 40;
	buf[6] = v >> 48;
	buf[7] = v >> 56;
	os.write(reinterpret_cast<char *>(buf), sizeof(buf));
	if (!os)
		throw std::runtime_error("savebinu64: write error");
}

void TrafficFlowRestrictions::savebindbl(std::ostream& os, double v)
{
	union {
		double d;
		uint64_t u;
	} x;
	x.d = v;
	savebinu64(os, x.u);
}

void TrafficFlowRestrictions::savebinstring(std::ostream& os, const std::string& v)
{
	savebinu32(os, v.size());
	os.write(v.c_str(), v.size());
	if (!os)
		throw std::runtime_error("savebinstring: write error");
}

void TrafficFlowRestrictions::savebinpt(std::ostream& os, const Point& v)
{
	savebinu32(os, v.get_lat());
	savebinu32(os, v.get_lon());
}

const char TrafficFlowRestrictions::binfile_signature_v1[] = "vfrnav Traffic Flow Restrictions V1\n";
const char TrafficFlowRestrictions::binfile_signature_v2[] = "vfrnav Traffic Flow Restrictions V2\n";

bool TrafficFlowRestrictions::add_binary_rules(std::vector<Message>& msg, const std::string& fname, AirportsDb& airportdb,
					       NavaidsDb& navaiddb, WaypointsDb& waypointdb, AirwaysDb& airwaydb, AirspacesDb& airspacedb)
{
	msg.clear();
	std::ifstream is(fname.c_str(), std::ifstream::binary);
	if (!is)
		return false;
	return add_binary_rules(msg, is, airportdb, navaiddb, waypointdb, airwaydb, airspacedb);
}

bool TrafficFlowRestrictions::add_binary_rules(std::vector<Message>& msg, std::istream& is, AirportsDb& airportdb,
					       NavaidsDb& navaiddb, WaypointsDb& waypointdb, AirwaysDb& airwaydb, AirspacesDb& airspacedb)
{
	msg.clear();
	if (!is)
		return false;
	try {
		bool v2(false);
		{
			char buf[sizeof(binfile_signature_v2)];
			is.read(buf, sizeof(binfile_signature_v2));
			if (!is)
				throw std::runtime_error("cannot read signature");
			if (!memcmp(buf, binfile_signature_v2, sizeof(binfile_signature_v2)))
				v2 = true;
			else if (memcmp(buf, binfile_signature_v1, sizeof(binfile_signature_v1)))
				throw std::runtime_error("invalid signature");
		}
		BinLoader ldr(*this, is, airportdb, navaiddb, waypointdb, airwaydb, airspacedb, v2);
		{
			std::string s(ldr.loadbinstring());
			if (!s.empty())
				set_origin(s);
		}
		{
			std::string s(ldr.loadbinstring());
			if (!s.empty())
				set_created(s);
		}
		{
			std::string s(ldr.loadbinstring());
			if (!s.empty())
				set_effective(s);
		}
		m_loadedtfr.load_binary(ldr);
		uint8_t buf[64];
		is.read(reinterpret_cast<char *>(buf), sizeof(buf));
		int sz;
		if (is)
			sz = sizeof(buf);
		else
			sz = is.gcount();
		if (sz) {
			std::ostringstream oss;
			oss << "Ignoring excess bytes" << std::hex;
			for (int i = 0; i < sz; ++i)
				oss << ' ' << std::setfill('0') << std::setw(2) << (unsigned int)buf[i];
			ldr.warning(oss.str());
		}
	} catch (const std::exception& e) {
		if (true)
			std::cerr << "load_binary_rules: " << e.what() << std::endl;
		m_messages.push_back(Message(e.what(), Message::type_error));
		msg.swap(m_messages);
		return false;
	}
	msg.swap(m_messages);
	return true;
}

bool TrafficFlowRestrictions::save_binary_rules(const std::string& fname) const
{
	std::ofstream os(fname.c_str(), std::ofstream::binary);
	if (!os)
		return false;
	return save_binary_rules(os);
}

bool TrafficFlowRestrictions::save_binary_rules(std::ostream& os) const
{
	if (!os)
		return false;
	try {
		os.write(binfile_signature_v2, sizeof(binfile_signature_v2));
		if (!os)
			throw std::runtime_error("cannot write signature");
		savebinstring(os, get_origin());
		savebinstring(os, get_created());
		savebinstring(os, get_effective());
		m_loadedtfr.save_binary(os);
	} catch (const std::exception& e) {
		if (true)
			std::cerr << "save_binary_rules: " << e.what() << std::endl;
		return false;
	}
	return true;
}

void TrafficFlowRestrictions::Timesheet::Element::save_binary(std::ostream& os) const
{
	if (m_timeref == timeref_invalid)
		return;
	savebinu8(os, m_timeref);
	savebinu8(os, (m_sunrise ? 1 : 0) | (m_sunset ? 2 : 0));
	savebinu32(os, m_timewef);
	savebinu32(os, m_timetil);
	savebinu8(os, m_monwef);
	savebinu8(os, m_montil);
	savebinu8(os, m_daywef);
	savebinu8(os, m_daytil);
	savebinu8(os, m_day);
}

void TrafficFlowRestrictions::Timesheet::Element::load_binary(BinLoader& ldr)
{
	m_timeref = (timeref_t)ldr.loadbinu8();
	if (m_timeref >= timeref_invalid) {
		m_timeref = timeref_invalid;
		return;
	}
	{
		uint8_t flg(ldr.loadbinu8());
		m_sunrise = !!(flg & 1);
		m_sunset = !!(flg & 2);
	}
	m_timewef = ldr.loadbinu32();
	m_timetil = ldr.loadbinu32();
	m_monwef = ldr.loadbinu8();
	m_montil = ldr.loadbinu8();
	m_daywef = ldr.loadbinu8();
	m_daytil = ldr.loadbinu8();
	m_day = ldr.loadbinu8();
}

void TrafficFlowRestrictions::Timesheet::save_binary(std::ostream& os) const
{
	savebinu8(os, m_workhr);
	for (elements_t::const_iterator i(m_elements.begin()), e(m_elements.end()); i != e; ++i)
		i->save_binary(os);
	savebinu8(os, 0xff);
}

void TrafficFlowRestrictions::Timesheet::load_binary(BinLoader& ldr)
{
	m_workhr = (workhr_t)ldr.loadbinu8();
	m_elements.clear();
	for (;;) {
		m_elements.push_back(Element());
		m_elements.back().load_binary(ldr);
		if (m_elements.back().get_timeref() != Element::timeref_invalid)
			continue;
		m_elements.erase(m_elements.end() - 1);
		break;
	}
}

void TrafficFlowRestrictions::TFRAirspace::save_binary(std::ostream& os) const
{
	savebinu8(os, m_bdryclass);
	if (!m_bdryclass)
		return;
	savebinu8(os, m_typecode);
	savebinstring(os, m_id);
}

void TrafficFlowRestrictions::TFRAirspace::save_binary(std::ostream& os, const_ptr_t aspc)
{
	if (!aspc) {
		savebinu8(os, 0);
		return;
	}
	aspc->save_binary(os);
}

TrafficFlowRestrictions::TFRAirspace::const_ptr_t TrafficFlowRestrictions::TFRAirspace::load_binary(BinLoader& ldr)
{
	char bdryclass(ldr.loadbinu8());
	if (!bdryclass)
		return const_ptr_t();
	uint8_t typecode(ldr.loadbinu8());
	std::string id(ldr.loadbinstring());
	return ldr.find_airspace(id, bdryclass, typecode);
}

void TrafficFlowRestrictions::RuleWpt::save_binary(std::ostream& os) const
{
	savebinu8(os, (m_type & 0x3f) | (m_vor ? 0x40 : 0) | (m_ndb ? 0x80 : 0));
	savebinpt(os, m_coord);
	savebinstring(os, m_ident);
}

void TrafficFlowRestrictions::RuleWpt::load_binary(BinLoader& ldr)
{
	{
		uint8_t t(ldr.loadbinu8());
		m_type = (FplWpt::type_t)(t & 0x3f);
		m_vor = !!(t & 0x40);
		m_ndb = !!(t & 0x80);
	}
	m_coord = ldr.loadbinpt();
	m_ident = ldr.loadbinstring();
}

void TrafficFlowRestrictions::AltRange::save_binary(std::ostream& os) const
{
	savebinu8(os, m_lwrmode);
	savebinu8(os, m_uprmode);
	savebinu32(os, m_lwralt);
	savebinu32(os, m_upralt);
}

void TrafficFlowRestrictions::AltRange::load_binary(BinLoader& ldr)
{
	m_lwrmode = (alt_t)ldr.loadbinu8();
	m_uprmode = (alt_t)ldr.loadbinu8();
	m_lwralt = ldr.loadbinu32();
	m_upralt = ldr.loadbinu32();
}

void TrafficFlowRestrictions::TrafficFlowRules::save_binary(std::ostream& os) const
{
	for (const_iterator i(begin()), e(end()); i != e; ++i)
		if (*i)
			(*i)->save_binary(os);
	savebinu8(os, 0xff);
}

void TrafficFlowRestrictions::TrafficFlowRules::load_binary(BinLoader& ldr)
{
	for (;;) {
		TrafficFlowRule::ptr_t p(new TrafficFlowRule());
		p->load_binary(ldr);
		if (p->is_valid()) {
			push_back(p);
			continue;
		}
		break;
	}
}

void TrafficFlowRestrictions::TrafficFlowRule::save_binary(std::ostream& os) const
{
	if (m_codetype == codetype_invalid)
		return;
	savebinu8(os, m_codetype);
	savebinu64(os, m_mid);
	savebinstring(os, m_codeid);
	savebinstring(os, m_oprgoal);
	savebinstring(os, m_descr);
	m_timesheet.save_binary(os);
	m_restrictions.save_binary(os);
	Condition::save_binary(os, m_condition);
}

void TrafficFlowRestrictions::TrafficFlowRule::load_binary(BinLoader& ldr)
{
	m_codetype = (codetype_t)ldr.loadbinu8();
	if (m_codetype >= codetype_invalid) {
		m_codetype = codetype_invalid;
		return;
	}
	if (ldr.is_v2())
		m_mid = ldr.loadbinu64();
	else
		m_mid = ldr.loadbinu32();
	m_codeid = ldr.loadbinstring();
	m_oprgoal = ldr.loadbinstring();
	m_descr = ldr.loadbinstring();
	m_timesheet.load_binary(ldr);
	m_restrictions.load_binary(ldr);
	m_condition = Condition::load_binary(ldr);
}

void TrafficFlowRestrictions::Restrictions::save_binary(std::ostream& os) const
{
	savebinu32(os, m_elements.size());
	for (elements_t::const_iterator i(m_elements.begin()), e(m_elements.end()); i != e; ++i)
		i->save_binary(os);
}

void TrafficFlowRestrictions::Restrictions::load_binary(BinLoader& ldr)
{
	m_elements.clear();
	m_elements.resize(ldr.loadbinu32());
	for (elements_t::iterator i(m_elements.begin()), e(m_elements.end()); i != e; ++i)
		i->load_binary(ldr);	
}

void TrafficFlowRestrictions::RestrictionSequence::save_binary(std::ostream& os) const
{
	for (elements_t::const_iterator i(m_elements.begin()), e(m_elements.end()); i != e; ++i) {
		if (!*i)
			continue;
		(*i)->save_binary(os);
	}
	savebinu8(os, 0xff);
}

void TrafficFlowRestrictions::RestrictionSequence::load_binary(BinLoader& ldr)
{
	m_elements.clear();
	for (;;) {
		TrafficFlowRestrictions::RestrictionElement::ptr_t p(TrafficFlowRestrictions::RestrictionElement::load_binary(ldr));
		if (!p)
			break;
		m_elements.push_back(p);
	}
}

TrafficFlowRestrictions::RestrictionElement::ptr_t TrafficFlowRestrictions::RestrictionElement::load_binary(BinLoader& ldr)
{
	uint8_t t(ldr.loadbinu8());
	switch (t) {
	case 0:
		return TrafficFlowRestrictions::RestrictionElementRoute::load_binary(ldr);

	case 1:
		return TrafficFlowRestrictions::RestrictionElementPoint::load_binary(ldr);

	case 2:
	case 3:
		return TrafficFlowRestrictions::RestrictionElementSidStar::load_binary(ldr, t == 3);

	case 4:
		return TrafficFlowRestrictions::RestrictionElementAirspace::load_binary(ldr);

	default:
		return ptr_t();
	}
}

void TrafficFlowRestrictions::RestrictionElementRoute::save_binary(std::ostream& os) const
{
	savebinu8(os, 0);
	savebinu8(os, m_match);
        savebinstring(os, m_rteid);
	m_point[0].save_binary(os);
	m_point[1].save_binary(os);
	m_alt.save_binary(os);
}

TrafficFlowRestrictions::RestrictionElement::ptr_t TrafficFlowRestrictions::RestrictionElementRoute::load_binary(BinLoader& ldr)
{
	match_t m((match_t)ldr.loadbinu8());	
	std::string rteid(ldr.loadbinstring());
	RuleWpt pt0, pt1;
	pt0.load_binary(ldr);
	pt1.load_binary(ldr);
	AltRange alt;
	alt.load_binary(ldr);
	return ptr_t(new RestrictionElementRoute(alt, pt0, pt1, rteid, m));
}

void TrafficFlowRestrictions::RestrictionElementPoint::save_binary(std::ostream& os) const
{
	savebinu8(os, 1);
	m_point.save_binary(os);
	m_alt.save_binary(os);
}

TrafficFlowRestrictions::RestrictionElement::ptr_t TrafficFlowRestrictions::RestrictionElementPoint::load_binary(BinLoader& ldr)
{
	RuleWpt pt;
	pt.load_binary(ldr);
	AltRange alt;
	alt.load_binary(ldr);
	return ptr_t(new RestrictionElementPoint(alt, pt));
}

void TrafficFlowRestrictions::RestrictionElementSidStar::save_binary(std::ostream& os) const
{
	savebinu8(os, m_star ? 3 : 2);
        savebinstring(os, m_procid);
	m_arpt.save_binary(os);
	m_alt.save_binary(os);
}

TrafficFlowRestrictions::RestrictionElement::ptr_t TrafficFlowRestrictions::RestrictionElementSidStar::load_binary(BinLoader& ldr, bool star)
{
	std::string procid(ldr.loadbinstring());
	RuleWpt arpt;
	arpt.load_binary(ldr);
	AltRange alt;
	alt.load_binary(ldr);
	return ptr_t(new RestrictionElementSidStar(alt, procid, arpt, star));
}

void TrafficFlowRestrictions::RestrictionElementAirspace::save_binary(std::ostream& os) const
{
	savebinu8(os, 4);
	TFRAirspace::save_binary(os, m_airspace);
	m_alt.save_binary(os);
}

TrafficFlowRestrictions::RestrictionElement::ptr_t TrafficFlowRestrictions::RestrictionElementAirspace::load_binary(BinLoader& ldr)
{
	TFRAirspace::const_ptr_t aspc(TFRAirspace::load_binary(ldr));
	AltRange alt;
	alt.load_binary(ldr);
	return ptr_t(new RestrictionElementAirspace(alt, aspc));
}

void TrafficFlowRestrictions::Condition::save_binary(std::ostream& os, const_ptr_t aspc)
{
	if (!aspc) {
		savebinu8(os, 0xff);
		return;
	}
	aspc->save_binary(os);
}

TrafficFlowRestrictions::Condition::ptr_t TrafficFlowRestrictions::Condition::load_binary(BinLoader& ldr)
{
	uint8_t t(ldr.loadbinu8());
	switch (t) {
	case 0:
		return TrafficFlowRestrictions::ConditionAnd::load_binary(ldr);

	case 1:
		return TrafficFlowRestrictions::ConditionSequence::load_binary(ldr);

	case 2:
		return TrafficFlowRestrictions::ConditionConstant::load_binary(ldr);

	case 3:
		return TrafficFlowRestrictions::ConditionCrossingAirspace1::load_binary(ldr);

	case 4:
		return TrafficFlowRestrictions::ConditionCrossingAirspace2::load_binary(ldr);

	case 5:
		return TrafficFlowRestrictions::ConditionCrossingDct::load_binary(ldr);

	case 6:
		return TrafficFlowRestrictions::ConditionCrossingAirway::load_binary(ldr);

	case 7:
		return TrafficFlowRestrictions::ConditionCrossingPoint::load_binary(ldr);

	case 8:
		return TrafficFlowRestrictions::ConditionDepArr::load_binary(ldr);

	case 9:
		return TrafficFlowRestrictions::ConditionDepArrAirspace::load_binary(ldr);

	case 10:
		return TrafficFlowRestrictions::ConditionSidStar::load_binary(ldr);

	case 11:
		return TrafficFlowRestrictions::ConditionCrossingAirspaceActive::load_binary(ldr);

	case 12:
		return TrafficFlowRestrictions::ConditionCrossingAirwayAvailable::load_binary(ldr);

	case 13:
		return TrafficFlowRestrictions::ConditionDctLimit::load_binary(ldr);

	case 14:
		return TrafficFlowRestrictions::ConditionPropulsion::load_binary(ldr);

	case 15:
		return TrafficFlowRestrictions::ConditionAircraftType::load_binary(ldr);

	case 16:
		return TrafficFlowRestrictions::ConditionEquipment::load_binary(ldr);

	case 17:
		return TrafficFlowRestrictions::ConditionFlight::load_binary(ldr);

	case 18:
		return TrafficFlowRestrictions::ConditionCivMil::load_binary(ldr);

	case 19:
		return TrafficFlowRestrictions::ConditionGeneralAviation::load_binary(ldr);


	default:
		return ptr_t();
	}
}

void TrafficFlowRestrictions::ConditionAnd::save_binary(std::ostream& os) const
{
	savebinu8(os, 0);
	savebinu32(os, m_childnum);
	savebinu16(os, (m_cond.size() & 0x7fff) | (m_inv ? 0x8000 : 0));
	for (cond_t::const_iterator i(m_cond.begin()), e(m_cond.end()); i != e; ++i) {
		Condition::save_binary(os, i->get_ptr());
		savebinu8(os, i->is_inv() ? 1 : 0);
	}
}

TrafficFlowRestrictions::Condition::ptr_t TrafficFlowRestrictions::ConditionAnd::load_binary(BinLoader& ldr)
{
	unsigned int childnum(ldr.loadbinu32());
	uint16_t len(ldr.loadbinu16());
	Glib::RefPtr<ConditionAnd> p(new ConditionAnd(childnum, !!(len & 0x8000)));
	len &= 0x7fff;
	for (; len > 0; --len) {
		ptr_t pc(Condition::load_binary(ldr));
		uint8_t inv(ldr.loadbinu8());
		p->add(pc, !!inv);
	}
	return p;
}

void TrafficFlowRestrictions::ConditionSequence::save_binary(std::ostream& os) const
{
	savebinu8(os, 1);
	savebinu32(os, m_childnum);
	savebinu16(os, m_seq.size());
	for (seq_t::const_iterator i(m_seq.begin()), e(m_seq.end()); i != e; ++i)
		Condition::save_binary(os, *i);
}

TrafficFlowRestrictions::Condition::ptr_t TrafficFlowRestrictions::ConditionSequence::load_binary(BinLoader& ldr)
{
	unsigned int childnum(ldr.loadbinu32());
	uint16_t len(ldr.loadbinu16());
	Glib::RefPtr<ConditionSequence> p(new ConditionSequence(childnum));
	for (; len > 0; --len) {
		ptr_t pc(Condition::load_binary(ldr));
		p->add(pc);
	}
	return p;
}

void TrafficFlowRestrictions::ConditionConstant::save_binary(std::ostream& os) const
{
	savebinu8(os, 2);
	savebinu32(os, m_childnum);
	savebinu8(os, m_value ? 1 : 0);
}

TrafficFlowRestrictions::Condition::ptr_t TrafficFlowRestrictions::ConditionConstant::load_binary(BinLoader& ldr)
{
	unsigned int childnum(ldr.loadbinu32());
	uint8_t val(ldr.loadbinu8());
	return ptr_t(new ConditionConstant(childnum, !!val));
}

void TrafficFlowRestrictions::ConditionCrossingAirspace1::save_binary(std::ostream& os) const
{
	savebinu8(os, 3);
	savebinu32(os, m_childnum);
	m_alt.save_binary(os);
	TFRAirspace::save_binary(os, m_airspace);
	savebinu8(os, m_refloc ? 1 : 0);
}

TrafficFlowRestrictions::Condition::ptr_t TrafficFlowRestrictions::ConditionCrossingAirspace1::load_binary(BinLoader& ldr)
{
	unsigned int childnum(ldr.loadbinu32());
	AltRange alt;
	alt.load_binary(ldr);
	TFRAirspace::const_ptr_t aspc(TFRAirspace::load_binary(ldr));
	uint8_t refloc(ldr.loadbinu8());
	return ptr_t(new ConditionCrossingAirspace1(childnum, alt, aspc, !!refloc));
}

void TrafficFlowRestrictions::ConditionCrossingAirspace2::save_binary(std::ostream& os) const
{
	savebinu8(os, 4);
	savebinu32(os, m_childnum);
	m_alt.save_binary(os);
	TFRAirspace::save_binary(os, m_airspace0);
	TFRAirspace::save_binary(os, m_airspace1);
	savebinu8(os, m_refloc ? 1 : 0);
}

TrafficFlowRestrictions::Condition::ptr_t TrafficFlowRestrictions::ConditionCrossingAirspace2::load_binary(BinLoader& ldr)
{
	unsigned int childnum(ldr.loadbinu32());
	AltRange alt;
	alt.load_binary(ldr);
	TFRAirspace::const_ptr_t aspc0(TFRAirspace::load_binary(ldr));
	TFRAirspace::const_ptr_t aspc1(TFRAirspace::load_binary(ldr));
	uint8_t refloc(ldr.loadbinu8());
	return ptr_t(new ConditionCrossingAirspace2(childnum, alt, aspc0, aspc1, !!refloc));
}

void TrafficFlowRestrictions::ConditionCrossingDct::save_binary(std::ostream& os) const
{
	savebinu8(os, 5);
	savebinu32(os, m_childnum);
	m_alt.save_binary(os);
	m_wpt0.save_binary(os);
	m_wpt1.save_binary(os);
	savebinu8(os, m_refloc ? 1 : 0);
}

TrafficFlowRestrictions::Condition::ptr_t TrafficFlowRestrictions::ConditionCrossingDct::load_binary(BinLoader& ldr)
{
	unsigned int childnum(ldr.loadbinu32());
	AltRange alt;
	alt.load_binary(ldr);
	RuleWpt wpt0, wpt1;
	wpt0.load_binary(ldr);
	wpt1.load_binary(ldr);
	uint8_t refloc(ldr.loadbinu8());
	return ptr_t(new ConditionCrossingDct(childnum, alt, wpt0, wpt1, !!refloc));
}

void TrafficFlowRestrictions::ConditionCrossingAirway::save_binary(std::ostream& os) const
{
	savebinu8(os, 6);
	savebinu32(os, m_childnum);
	m_alt.save_binary(os);
	m_wpt0.save_binary(os);
	m_wpt1.save_binary(os);
	savebinstring(os, m_awyname);
	savebinu8(os, m_refloc ? 1 : 0);
}

TrafficFlowRestrictions::Condition::ptr_t TrafficFlowRestrictions::ConditionCrossingAirway::load_binary(BinLoader& ldr)
{
	unsigned int childnum(ldr.loadbinu32());
	AltRange alt;
	alt.load_binary(ldr);
	RuleWpt wpt0, wpt1;
	wpt0.load_binary(ldr);
	wpt1.load_binary(ldr);
	std::string awyname(ldr.loadbinstring());
	uint8_t refloc(ldr.loadbinu8());
	return ptr_t(new ConditionCrossingAirway(childnum, alt, wpt0, wpt1, awyname, !!refloc));
}

void TrafficFlowRestrictions::ConditionCrossingPoint::save_binary(std::ostream& os) const
{
	savebinu8(os, 7);
	savebinu32(os, m_childnum);
	m_alt.save_binary(os);
	m_wpt.save_binary(os);
	savebinu8(os, m_refloc ? 1 : 0);
}

TrafficFlowRestrictions::Condition::ptr_t TrafficFlowRestrictions::ConditionCrossingPoint::load_binary(BinLoader& ldr)
{
	unsigned int childnum(ldr.loadbinu32());
	AltRange alt;
	alt.load_binary(ldr);
	RuleWpt wpt;
	wpt.load_binary(ldr);
	uint8_t refloc(ldr.loadbinu8());
	return ptr_t(new ConditionCrossingPoint(childnum, alt, wpt, !!refloc));

}

void TrafficFlowRestrictions::ConditionDepArr::save_binary(std::ostream& os) const
{
	savebinu8(os, 8);
	savebinu32(os, m_childnum);
	m_airport.save_binary(os);
	savebinu8(os, (m_arr ? 0x02 : 0) | (m_refloc ? 0x01 : 0));
}

TrafficFlowRestrictions::Condition::ptr_t TrafficFlowRestrictions::ConditionDepArr::load_binary(BinLoader& ldr)
{
	unsigned int childnum(ldr.loadbinu32());
	RuleWpt airport;
	airport.load_binary(ldr);
	uint8_t flags(ldr.loadbinu8());
	return ptr_t(new ConditionDepArr(childnum, airport, !!(flags & 0x02), !!(flags & 0x01)));
}

void TrafficFlowRestrictions::ConditionDepArrAirspace::save_binary(std::ostream& os) const
{
	savebinu8(os, 9);
	savebinu32(os, m_childnum);
	TFRAirspace::save_binary(os, m_airspace);
	savebinu8(os, (m_arr ? 0x02 : 0) | (m_refloc ? 0x01 : 0));
}

TrafficFlowRestrictions::Condition::ptr_t TrafficFlowRestrictions::ConditionDepArrAirspace::load_binary(BinLoader& ldr)
{
	unsigned int childnum(ldr.loadbinu32());
	TFRAirspace::const_ptr_t aspc(TFRAirspace::load_binary(ldr));
	uint8_t flags(ldr.loadbinu8());
	return ptr_t(new ConditionDepArrAirspace(childnum, aspc, !!(flags & 0x02), !!(flags & 0x01)));
}

void TrafficFlowRestrictions::ConditionSidStar::save_binary(std::ostream& os) const
{
	savebinu8(os, 10);
	savebinu32(os, m_childnum);
	m_airport.save_binary(os);
	savebinstring(os, m_procid);
	savebinu8(os, (m_star ? 0x02 : 0) | (m_refloc ? 0x01 : 0));
}

TrafficFlowRestrictions::Condition::ptr_t TrafficFlowRestrictions::ConditionSidStar::load_binary(BinLoader& ldr)
{
	unsigned int childnum(ldr.loadbinu32());
	RuleWpt airport;
	airport.load_binary(ldr);
	std::string procid(ldr.loadbinstring());
	uint8_t flags(ldr.loadbinu8());
	return ptr_t(new ConditionSidStar(childnum, airport, procid, !!(flags & 0x02), !!(flags & 0x01)));
}

void TrafficFlowRestrictions::ConditionCrossingAirspaceActive::save_binary(std::ostream& os) const
{
	savebinu8(os, 11);
	savebinu32(os, m_childnum);
	TFRAirspace::save_binary(os, m_airspace);
}

TrafficFlowRestrictions::Condition::ptr_t TrafficFlowRestrictions::ConditionCrossingAirspaceActive::load_binary(BinLoader& ldr)
{
	unsigned int childnum(ldr.loadbinu32());
	TFRAirspace::const_ptr_t aspc(TFRAirspace::load_binary(ldr));
	return ptr_t(new ConditionCrossingAirspaceActive(childnum, aspc));
}

void TrafficFlowRestrictions::ConditionCrossingAirwayAvailable::save_binary(std::ostream& os) const
{
	savebinu8(os, 12);
	savebinu32(os, m_childnum);
	m_alt.save_binary(os);
	m_wpt0.save_binary(os);
	m_wpt1.save_binary(os);
	savebinstring(os, m_awyname);
}

TrafficFlowRestrictions::Condition::ptr_t TrafficFlowRestrictions::ConditionCrossingAirwayAvailable::load_binary(BinLoader& ldr)
{
	unsigned int childnum(ldr.loadbinu32());
	AltRange alt;
	alt.load_binary(ldr);
	RuleWpt wpt0, wpt1;
	wpt0.load_binary(ldr);
	wpt1.load_binary(ldr);
	std::string awyname(ldr.loadbinstring());
	return ptr_t(new ConditionCrossingAirwayAvailable(childnum, alt, wpt0, wpt1, awyname));
}

void TrafficFlowRestrictions::ConditionDctLimit::save_binary(std::ostream& os) const
{
	savebinu8(os, 13);
	savebinu32(os, m_childnum);
	savebindbl(os, m_dctlimit);
}

TrafficFlowRestrictions::Condition::ptr_t TrafficFlowRestrictions::ConditionDctLimit::load_binary(BinLoader& ldr)
{
	unsigned int childnum(ldr.loadbinu32());
	double limit(ldr.loadbindbl());
	return ptr_t(new ConditionDctLimit(childnum, limit));
}

void TrafficFlowRestrictions::ConditionPropulsion::save_binary(std::ostream& os) const
{
	savebinu8(os, 14);
	savebinu32(os, m_childnum);
	savebinu8(os, m_kind);
	savebinu8(os, m_propulsion);
	savebinu8(os, m_nrengines);
}

TrafficFlowRestrictions::Condition::ptr_t TrafficFlowRestrictions::ConditionPropulsion::load_binary(BinLoader& ldr)
{
	unsigned int childnum(ldr.loadbinu32());
	kind_t kind((kind_t)ldr.loadbinu8());
	propulsion_t prop((propulsion_t)ldr.loadbinu8());
	unsigned int nrengines(ldr.loadbinu8());
	return ptr_t(new ConditionPropulsion(childnum, kind, prop, nrengines));
}

void TrafficFlowRestrictions::ConditionAircraftType::save_binary(std::ostream& os) const
{
	savebinu8(os, 15);
	savebinu32(os, m_childnum);
	savebinstring(os, m_type);
}

TrafficFlowRestrictions::Condition::ptr_t TrafficFlowRestrictions::ConditionAircraftType::load_binary(BinLoader& ldr)
{
	unsigned int childnum(ldr.loadbinu32());
	std::string type(ldr.loadbinstring());
	return ptr_t(new ConditionAircraftType(childnum, type));
}

void TrafficFlowRestrictions::ConditionEquipment::save_binary(std::ostream& os) const
{
	savebinu8(os, 16);
	savebinu32(os, m_childnum);
	savebinu8(os, m_equipment);
}

TrafficFlowRestrictions::Condition::ptr_t TrafficFlowRestrictions::ConditionEquipment::load_binary(BinLoader& ldr)
{
	unsigned int childnum(ldr.loadbinu32());
	equipment_t equipment((equipment_t)ldr.loadbinu8());
	return ptr_t(new ConditionEquipment(childnum, equipment));
}

void TrafficFlowRestrictions::ConditionFlight::save_binary(std::ostream& os) const
{
	savebinu8(os, 17);
	savebinu32(os, m_childnum);
	savebinu8(os, m_flight);
}

TrafficFlowRestrictions::Condition::ptr_t TrafficFlowRestrictions::ConditionFlight::load_binary(BinLoader& ldr)
{
	unsigned int childnum(ldr.loadbinu32());
	flight_t flight((flight_t)ldr.loadbinu8());
	return ptr_t(new ConditionFlight(childnum, flight));
}

void TrafficFlowRestrictions::ConditionCivMil::save_binary(std::ostream& os) const
{
	savebinu8(os, 18);
	savebinu32(os, m_childnum);
	savebinu8(os, m_civmil);
}

TrafficFlowRestrictions::Condition::ptr_t TrafficFlowRestrictions::ConditionCivMil::load_binary(BinLoader& ldr)
{
	unsigned int childnum(ldr.loadbinu32());
	civmil_t civmil((civmil_t)ldr.loadbinu8());
	return ptr_t(new ConditionCivMil(childnum, civmil));
}

void TrafficFlowRestrictions::ConditionGeneralAviation::save_binary(std::ostream& os) const
{
	savebinu8(os, 19);
	savebinu32(os, m_childnum);
}

TrafficFlowRestrictions::Condition::ptr_t TrafficFlowRestrictions::ConditionGeneralAviation::load_binary(BinLoader& ldr)
{
	unsigned int childnum(ldr.loadbinu32());
	return ptr_t(new ConditionGeneralAviation(childnum));
}
