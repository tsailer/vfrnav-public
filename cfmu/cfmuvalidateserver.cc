#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <iostream>
#include <iomanip>
#include <gdkmm.h>
#include <gtkmm.h>

#ifdef HAVE_PWD_H
#include <pwd.h>
#endif

#ifdef HAVE_GRP_H
#include <grp.h>
#endif

#ifdef HAVE_SYSTEMD_SD_DAEMON_H
#include <systemd/sd-daemon.h>
#endif

#include <errno.h>
#include <string.h>
#include <libxml/HTMLparser.h>

#include "cfmuvalidateserver.hh"
#include "fplan.h"
#include "icaofpl.h"

// Xlib last because it defines Status to int...

#ifdef HAVE_X11_XLIB_H
#include <X11/Xlib.h>
#undef None
#endif

#ifdef HAVE_SYSEXITS_H
#include <sysexits.h>
#else
#define EX_OK           0
#define EX_USAGE        64
#define EX_DATAERR      65
#define EX_NOINPUT      66
#define EX_UNAVAILABLE  69
#endif  

static Glib::RefPtr<Glib::MainLoop> mainloop;

static void signalterminate(int signr)
{
	if (mainloop)
		mainloop->quit();
}

void CFMUValidateServer::Results::print(const ADR::Object::ptr_t& p, ADR::timetype_t tm)
{
	ADR::FlightRestrictionTimeSlice& tsrule(p->operator()(tm).as_flightrestriction());
	if (!tsrule.is_valid()) {
		push_back("Rule: ?""?");
		return;
	}
	std::ostringstream oss;
	oss << "Rule: " << tsrule.get_ident() << ' ';
	switch (tsrule.get_type()) {
	case ADR::FlightRestrictionTimeSlice::type_mandatory:
		oss << "Mandatory";
		break;

	case ADR::FlightRestrictionTimeSlice::type_forbidden:
		oss << "Forbidden";
		break;

	case ADR::FlightRestrictionTimeSlice::type_closed:
		oss << "ClosedForCruising";
		break;

	case ADR::FlightRestrictionTimeSlice::type_allowed:
		oss << "Allowed";
		break;

	default:
		oss << '?';
		break;
	}
	if (tsrule.is_dct() || tsrule.is_unconditional() || tsrule.is_routestatic() || tsrule.is_mandatoryinbound() || !tsrule.is_enabled()) {
		oss << " (";
		if (!tsrule.is_enabled())
			oss << '-';
		if (tsrule.is_dct())
			oss << 'D';
		if (tsrule.is_unconditional())
			oss << 'U';
		if (tsrule.is_routestatic())
			oss << 'S';
		if (tsrule.is_mandatoryinbound())
			oss << 'I';
		oss << ')';
	}
	push_back(oss.str());
	push_back("Desc: " + tsrule.get_instruction());
	//add_resulttext("OprGoal: \n", m_txttagoprgoal);
	if (tsrule.get_condition())
	        push_back("Cond: " + tsrule.get_condition()->to_str(tm));
	for (unsigned int i = 0, n = tsrule.get_restrictions().size(); i < n; ++i) {
		const ADR::RestrictionSequence& rs(tsrule.get_restrictions()[i]);
		std::ostringstream oss;
		oss << "  Alternative " << i << ' ' << rs.to_str(tm);
	        push_back(oss.str());
	}
}

void CFMUValidateServer::Results::print(const rules_t& rules, const Glib::ustring& pfx)
{
	std::string r;
	bool subseq(false);
	for (rules_t::const_iterator ti(rules.begin()), te(rules.end()); ti != te; ++ti) {
		if (subseq)
			r += ",";
		subseq = true;
		r += *ti;
	}
	push_back(pfx + r);
}

void CFMUValidateServer::Results::print_tracerules(ADR::RestrictionEval& reval)
{
	std::string r;
	bool subseq(false);
	uint64_t tm(time(0));
	for (ADR::RestrictionEval::rules_t::const_iterator ri(reval.get_rules().begin()), re(reval.get_rules().end()); ri != re; ++ri) {
		const ADR::Object::ptr_t& p(*ri);
		const ADR::FlightRestrictionTimeSlice& ts(p->operator()(tm).as_flightrestriction());
		if (!ts.is_valid())
			continue;
		if (!ts.is_trace())
			continue;
		if (subseq)
			r += ",";
		subseq = true;
		r += ts.get_ident();
	}
	push_back("trace:" + r);
}

void CFMUValidateServer::Results::print_disabledrules(ADR::RestrictionEval& reval)
{
	std::string r;
	bool subseq(false);
	uint64_t tm(time(0));
	for (ADR::RestrictionEval::rules_t::const_iterator ri(reval.get_rules().begin()), re(reval.get_rules().end()); ri != re; ++ri) {
		const ADR::Object::ptr_t& p(*ri);
		const ADR::FlightRestrictionTimeSlice& ts(p->operator()(tm).as_flightrestriction());
		if (!ts.is_valid())
			continue;
		if (ts.is_enabled())
			continue;
		if (subseq)
			r += ",";
		subseq = true;
		r += ts.get_ident();
	}
	push_back("disabled:" + r);
}

void CFMUValidateServer::Results::print(const ADR::Message& m)
{
	std::ostringstream oss;
	oss << m.get_type_char() << ':';
	{
		const std::string& id(m.get_ident());
		if (!id.empty())
			oss << " R:" << id;
	}
	if (!m.get_vertexset().empty()) {
		oss << " V:";
		bool subseq(false);
		for (ADR::Message::set_t::const_iterator i(m.get_vertexset().begin()), e(m.get_vertexset().end()); i != e; ++i) {
			if (subseq)
				oss << ',';
			subseq = true;
			oss << *i;
		}
	}
	if (!m.get_edgeset().empty()) {
		oss << " E:";
		bool subseq(false);
		for (ADR::Message::set_t::const_iterator i(m.get_edgeset().begin()), e(m.get_edgeset().end()); i != e; ++i) {
			if (subseq)
				oss << ',';
			subseq = true;
			oss << *i;
		}
	}
	oss << ' ' << m.get_text();
	push_back(oss.str());
}

void CFMUValidateServer::Results::print(const ADR::RestrictionResult& res, ADR::timetype_t tm)
{
	const ADR::FlightRestrictionTimeSlice& tsrule(res.get_rule()->operator()(tm).as_flightrestriction());
	if (!tsrule.is_valid()) {
		push_back("Rule: ?""?");
		return;
	}
	std::ostringstream oss;
	oss << "Rule: " << tsrule.get_ident() << ' ';
	switch (tsrule.get_type()) {
	case ADR::FlightRestrictionTimeSlice::type_mandatory:
		oss << "Mandatory";
		break;

	case ADR::FlightRestrictionTimeSlice::type_forbidden:
		oss << "Forbidden";
		break;

	case ADR::FlightRestrictionTimeSlice::type_closed:
		oss << "ClosedForCruising";
		break;

	case ADR::FlightRestrictionTimeSlice::type_allowed:
		oss << "Allowed";
		break;

	default:
		oss << '?';
		break;
	}
	if (tsrule.is_dct() || tsrule.is_unconditional() || tsrule.is_routestatic() ||
	    tsrule.is_mandatoryinbound() || tsrule.is_mandatoryoutbound() || !tsrule.is_enabled()) {
		oss << " (";
		if (!tsrule.is_enabled())
			oss << '-';
		if (tsrule.is_dct())
			oss << 'D';
		if (tsrule.is_unconditional())
			oss << 'U';
		if (tsrule.is_routestatic())
			oss << 'S';
		if (tsrule.is_mandatoryinbound())
			oss << 'I';
		if (tsrule.is_mandatoryoutbound())
			oss << 'O';
		oss << ')';
	}
	if (!res.get_vertexset().empty()) {
		oss << " V:";
		bool subseq(false);
		for (ADR::RestrictionResult::set_t::const_iterator i(res.get_vertexset().begin()), e(res.get_vertexset().end()); i != e; ++i) {
			if (subseq)
				oss << ',';
			subseq = true;
			oss << *i;
		}
	}
	if (!res.get_edgeset().empty()) {
		oss << " E:";
		bool subseq(false);
		for (ADR::RestrictionResult::set_t::const_iterator i(res.get_edgeset().begin()), e(res.get_edgeset().end()); i != e; ++i) {
			if (subseq)
				oss << ',';
			subseq = true;
			oss << *i;
		}
	}
	if (!res.has_refloc())
		oss << " L:" << res.get_refloc();
	push_back(oss.str());
	push_back("Desc: " + tsrule.get_instruction());
	//push_back("OprGoal: ");
	if (tsrule.get_condition())
	        push_back("Cond: " + tsrule.get_condition()->to_str(tm));
	for (unsigned int i = 0, n = tsrule.get_restrictions().size(); i < n; ++i) {
		const ADR::RestrictionSequence& rs(tsrule.get_restrictions()[i]);
		const ADR::RestrictionSequenceResult& rsr(res[i]);
		{
			std::ostringstream oss;
			oss << "  Alternative " << i;
			if (!rsr.get_vertexset().empty()) {
				oss << " V:";
				bool subseq(false);
				for (ADR::RestrictionResult::set_t::const_iterator i(rsr.get_vertexset().begin()), e(rsr.get_vertexset().end()); i != e; ++i) {
					if (subseq)
						oss << ',';
					subseq = true;
					oss << *i;
				}
			}
			if (!rsr.get_edgeset().empty()) {
				oss << " E:";
				bool subseq(false);
				for (ADR::RestrictionResult::set_t::const_iterator i(rsr.get_edgeset().begin()), e(rsr.get_edgeset().end()); i != e; ++i) {
					if (subseq)
						oss << ',';
					subseq = true;
					oss << *i;
				}
			}
			push_back(oss.str());
		}
		push_back(rs.to_str(tm));
	}
}

void CFMUValidateServer::Results::annotate(ADR::RestrictionEval& reval)
{
	Results x;
	for (Results::const_iterator ri(begin()), re(end()); ri != re; ++ri) {
		x.push_back(*ri);
		Glib::ustring::size_type n(ri->rfind('['));
		if (n == Glib::ustring::npos)
			continue;
		Glib::ustring::size_type ne(ri->find(']', n + 1));
		if (ne == Glib::ustring::npos || ne - n < 2 || ne - n > 11)
			continue;
		std::string ruleid(ri->substr(n + 1, ne - n - 1));
		{
			uint64_t tm(time(0));
			ADR::RestrictionEval::rules_t::const_iterator ri(reval.get_rules().begin()), re(reval.get_rules().end());
			for (; ri != re; ++ri) {
				const ADR::Object::ptr_t& p(*ri);
				ADR::FlightRestrictionTimeSlice& ts(p->operator()(tm).as_flightrestriction());
				if (!ts.is_valid())
					continue;
				if (ts.get_ident() != ruleid)
					continue;
			        x.print(p, tm);
				break;
			}
		}
	}
	swap(x);
}

bool CFMUValidateServer::OptionalArgFilename::on_option_arg_filename(const Glib::ustring& option_name, const std::string& value, bool has_value)
{
	m_value = has_value;
	if (has_value)
		m_arg = value;
	else
		m_arg.clear();
	m_set = true;
	return true;
}
	
bool CFMUValidateServer::OptionalArgString::on_option_arg_string(const Glib::ustring& option_name, const Glib::ustring& value, bool has_value)
{
	m_value = has_value;
	if (has_value)
		m_arg = value;
	else
		m_arg.clear();
	m_set = true;
	return true;
}

CFMUValidateServer::Client::Client(CFMUValidateServer *server)
	: m_server(server), m_refcount(1),
#ifdef HAVE_CURL
	  m_curl(0), m_curl_headers(0),
#endif
	  m_validator(validator_cfmu), m_annotate(false)
{
        try {
		Glib::KeyFile kf;
                kf.load_from_file(PACKAGE_SYSCONF_DIR "/cfmu.ini", Glib::KEY_FILE_KEEP_COMMENTS | Glib::KEY_FILE_KEEP_TRANSLATIONS);
		if (kf.has_group("b2bop")) {
			if (kf.has_key("b2bop", "url"))
				m_cfmub2bopurl = kf.get_string("b2bop", "url");
			if (kf.has_key("b2bop", "url"))
				m_cfmub2bopcert = kf.get_string("b2bop", "cert");
		}
		if (kf.has_group("b2bpreop")) {
			if (kf.has_key("b2bpreop", "url"))
				m_cfmub2bpreopurl = kf.get_string("b2bpreop", "url");
			if (kf.has_key("b2bpreop", "url"))
				m_cfmub2bpreopcert = kf.get_string("b2bpreop", "cert");
		}
        } catch (const Glib::FileError& e) {
                std::cerr << "Unable to load CFMU settings from " PACKAGE_SYSCONF_DIR "/cfmu.ini: " << e.what() << std::endl;
        } catch (const Gio::Error& e) {
                std::cerr << "Unable to load CFMU settings from " PACKAGE_SYSCONF_DIR "/cfmu.ini: " << e.what() << std::endl;
        }
	if (!m_cfmub2bopcert.empty())
		if (!Glib::file_test(m_cfmub2bopcert, Glib::FILE_TEST_EXISTS) ||
		    !Glib::file_test(m_cfmub2bopcert, Glib::FILE_TEST_IS_REGULAR)) {
			std::cerr << "B2B Op Cert " << m_cfmub2bopcert << " unreadable" << std::endl;
			m_cfmub2bopcert.clear();
		}
	if (!m_cfmub2bpreopcert.empty())
		if (!Glib::file_test(m_cfmub2bpreopcert, Glib::FILE_TEST_EXISTS) ||
		    !Glib::file_test(m_cfmub2bpreopcert, Glib::FILE_TEST_IS_REGULAR)) {
			std::cerr << "B2B Preop Cert " << m_cfmub2bpreopcert << " unreadable" << std::endl;
			m_cfmub2bpreopcert.clear();
		}
	if (!m_cfmub2bopurl.empty() && !m_cfmub2bopcert.empty())
		m_validator = validator_cfmub2bop;
	else if (!m_cfmub2bpreopurl.empty() && !m_cfmub2bpreopcert.empty())
		m_validator = validator_cfmub2bpreop;
}

CFMUValidateServer::Client::~Client()
{
#ifdef HAVE_CURL
	close_curl();
#endif
}

void CFMUValidateServer::Client::reference(void) const
{
        g_atomic_int_inc(&m_refcount);
	if (false)
		std::cerr << "client " << (unsigned long long)this << " reference " << m_refcount << std::endl;
}

void CFMUValidateServer::Client::unreference(void) const
{
	bool nz(!g_atomic_int_dec_and_test(&m_refcount));
	if (false)
		std::cerr << "client " << (unsigned long long)this << " unreference " << m_refcount << std::endl;
        if (nz)
                return;
        delete this;
}

const std::string& CFMUValidateServer::Client::to_str(validator_t v)
{
	switch (v) {
	case validator_local:
	{
		static const std::string r("local");
		return r;
	}

	case validator_cfmu:
	{
		static const std::string r("cfmu");
		return r;
	}

	case validator_cfmub2bop:
	{
		static const std::string r("cfmub2bop");
		return r;
	}

	case validator_cfmub2bpreop:
	{
		static const std::string r("cfmub2bpreop");
		return r;
	}

	case validator_eurofpl:
	{
		static const std::string r("eurofpl");
		return r;
	}

	default:
	{
		static const std::string r("?");
		return r;
	}
	}
}

void CFMUValidateServer::Client::eurocontrol_textual(xmlpp::Element *fplel)
{
	if (!fplel)
		return;
	xmlpp::Element *fpltxtel(fplel->add_child("textual"));
	fpltxtel->set_child_text(m_fplan.c_str());
}

void CFMUValidateServer::Client::eurocontrol_textual_json(xmlpp::Element *fplel)
{
	if (!fplel)
		return;
	if (!m_jsonfpl.isMember("fpl"))
		return;
	const Json::Value& fpl(m_jsonfpl["fpl"]);
        if (!fpl.isString())
		return;
	xmlpp::Element *fpltxtel(fplel->add_child("textual"));
	fpltxtel->set_child_text(fpl.asString());
}

void CFMUValidateServer::Client::eurocontrol_structural_aerodrome(xmlpp::Element *el, const Json::Value& wpt)
{
	if (!el)
		return;
	if (wpt.isMember("ident")) {
		const Json::Value& idv(wpt["ident"]);
		if (idv.isString()) {
			std::string id(idv.asString());
			if (id.size() == 4 && id != "ZZZZ") {
				xmlpp::Element *el2(el->add_child("icaoId"));
				el2->set_child_text(id);
				return;
			}
		}
	}
	if (!wpt.isMember("coordlatdeg") || !wpt.isMember("coordlondeg"))
		return;
	const Json::Value& wlat(wpt["coordlatdeg"]), wlon(wpt["coordlondeg"]);
	if (!wlat.isNumeric() || !wlon.isNumeric())
		return;
	xmlpp::Element *el2(el->add_child("otherDesignation"));
	{
		xmlpp::Element *el3(el2->add_child("aerodromeLocation-GeoPoint"));
		xmlpp::Element *el4(el3->add_child("position"));
		{
			xmlpp::Element *el5(el4->add_child("latitude"));
			double v(wlat.asDouble());
			xmlpp::Element *el6(el5->add_child("side"));
			if (v <= 0) {
				el6->set_child_text("SOUTH");
				v = -v;
			} else {
				el6->set_child_text("NORTH");
			}
			xmlpp::Element *el7(el5->add_child("angle"));
			std::ostringstream oss;
			unsigned int v1(std::floor(v));
			oss << std::fixed << std::setfill('0') << std::setw(2) << v1;
			v = (v - v1) * 60;
			v1 = std::floor(v);
			oss << std::setfill('0') << std::setw(2) << v1;
			v = (v - v1) * 60;
			v1 = std::floor(v);
			oss << std::setfill('0') << std::setw(2) << v1;
			el7->set_child_text(oss.str());
		}
		{
			xmlpp::Element *el5(el4->add_child("longitude"));
			double v(wlat.asDouble());
			xmlpp::Element *el6(el5->add_child("side"));
			if (v <= 0) {
				el6->set_child_text("WEST");
				v = -v;
			} else {
				el6->set_child_text("EAST");
			}
			xmlpp::Element *el7(el5->add_child("angle"));
			std::ostringstream oss;
			unsigned int v1(std::floor(v));
			oss << std::fixed << std::setfill('0') << std::setw(3) << v1;
			v = (v - v1) * 60;
			v1 = std::floor(v);
			oss << std::setfill('0') << std::setw(2) << v1;
			v = (v - v1) * 60;
			v1 = std::floor(v);
			oss << std::setfill('0') << std::setw(2) << v1;
			el7->set_child_text(oss.str());
		}
	}
	if (wpt.isMember("name")) {
		const Json::Value& idv(wpt["name"]);
		if (idv.isString()) {
			xmlpp::Element *el3(el2->add_child("aerodromeName"));
			el3->set_child_text(idv.asString());
		}
	}
}

void CFMUValidateServer::Client::eurocontrol_structural(xmlpp::Element *fplel)
{
	if (!fplel)
		return;
	if (!m_jsonfpl.isMember("fplan") || !m_jsonfpl.isMember("otherinfo")) {
		eurocontrol_textual_json(fplel);
		return;
	}
	const Json::Value& fplan(m_jsonfpl["fplan"]);
	const Json::Value& otherinfo(m_jsonfpl["otherinfo"]);
	if (!fplan.isArray() || fplan.size() < 2 || !otherinfo.isObject()) {
		eurocontrol_textual_json(fplel);
		return;
	}
	xmlpp::Element *fplstrel(fplel->add_child("structural"));
	// departure
	eurocontrol_structural_aerodrome(fplstrel->add_child("aerodromeOfDeparture"), fplan[0]);
	{
		xmlpp::Element *el(fplstrel->add_child("aerodromesOfDestination"));
		// destination
		eurocontrol_structural_aerodrome(el->add_child("aerodromeOfDestination"), fplan[fplan.size() - 1]);
		if (m_jsonfpl.isMember("destalt")) {
			const Json::Value& alt(m_jsonfpl["destalt"]);
			if (alt.isArray()) {
				for (unsigned int i = 0, n = alt.size(); i < n; ++i) {
					std::ostringstream oss;
					oss << "alternate" << (i + 1);
					eurocontrol_structural_aerodrome(el->add_child(oss.str()), alt[i]);
				}
			}
		}
	}
	// aircraft id
	if (m_jsonfpl.isMember("aircraftid")) {
		const Json::Value& id(m_jsonfpl["aircraftid"]);
		if (id.isString()) {
			xmlpp::Element *el(fplstrel->add_child("aircraftId"));
			xmlpp::Element *el2(el->add_child("aircraftId"));
			el2->set_child_text(id.asString());
		}
	}
	// aircraft num
	{
		xmlpp::Element *el(fplstrel->add_child("numberOfAircraft"));
		el->set_child_text("1");
	}
	// type
	if (m_jsonfpl.isMember("aircrafttype")) {
		const Json::Value& id(m_jsonfpl["aircrafttype"]);
		if (id.isString()) {
			std::string id2(id.asString());
			if (id2 == "ZZZZ") {
				if (otherinfo.isMember("TYP")) {
					const Json::Value& typ(otherinfo["TYP"]);
					if (typ.isString()) {
						xmlpp::Element *el(fplstrel->add_child("aircraftType"));
						xmlpp::Element *el2(el->add_child("otherDesignation"));
						el2->set_child_text(typ.asString());
					}
				}
			} else {
				xmlpp::Element *el(fplstrel->add_child("aircraftType"));
				xmlpp::Element *el2(el->add_child("icaoId"));
				el2->set_child_text(id2);
			}
		}
	}
	// EET
	if (m_jsonfpl.isMember("totaleet")) {
		const Json::Value& eet(m_jsonfpl["totaleet"]);
		if (eet.isIntegral()) {
			unsigned int m(eet.asLargestUInt());
			m += 30;
			m /= 60;
			unsigned int h(m / 60);
			m -= h * 60;
			std::ostringstream oss;
			oss << std::setfill('0') << std::setw(2) << h << std::setfill('0') << std::setw(2) << m;
			xmlpp::Element *el(fplstrel->add_child("totalEstimatedElapsedTime"));
			el->set_child_text(oss.str());
		}
	}
	if (otherinfo.isMember("EET")) {
		const Json::Value& eet(otherinfo["EET"]);
		if (eet.isString()) {
			std::vector<std::string> eets(ADR::FlightPlan::tokenize(eet.asString()));
			xmlpp::Element *el(0);
			for (std::vector<std::string>::const_iterator ei(eets.begin()), ee(eets.end()); ei != ee; ++ei) {
				if (ei->size() < 5 || !std::isdigit((*ei)[ei->size()-4]) || !std::isdigit((*ei)[ei->size()-3]) ||
				    !std::isdigit((*ei)[ei->size()-2]) || !std::isdigit((*ei)[ei->size()-1]))
					continue;
				if (!el)
					el = fplstrel->add_child("eetsToLocations");
				el->add_child("elapsedTime")->set_child_text(ei->substr(ei->size() - 4, 4));
				xmlpp::Element *el2(el->add_child("point"));
				el2->add_child("pointId")->set_child_text(ei->substr(0, ei->size() - 4));
			}
		}
	}
	// Wake Category
	if (m_jsonfpl.isMember("wakecategory")) {
		const Json::Value& cat1(m_jsonfpl["wakecategory"]);
		if (cat1.isString()) {
			char cat(cat1.asString().c_str()[0]);
			xmlpp::Element *el(fplstrel->add_child("wakeTurbulenceCategory"));
			switch (cat) {
			default:
			case 'L':
				el->set_child_text("LIGHT");
				break;

			case 'M':
				el->set_child_text("MEDIUM");
				break;

			case 'H':
				el->set_child_text("HEAVY");
				break;

			case 'J':
				el->set_child_text("SUPER");
				break;
			}
		}
	}
	// FLT Type
	if (m_jsonfpl.isMember("flighttype")) {
		const Json::Value& typ1(m_jsonfpl["flighttype"]);
		if (typ1.isString()) {
			char typ(typ1.asString().c_str()[0]);
			xmlpp::Element *el(fplstrel->add_child("flightType"));
			switch (typ) {
			case 'S':
				el->set_child_text("SCHEDULED");
				break;

			case 'N':
				el->set_child_text("NOT_SCHEDULED");
				break;

			case 'G':
				el->set_child_text("GENERAL");
				break;

			case 'M':
				el->set_child_text("MILITARY");
				break;

			default:
			case 'X':
				el->set_child_text("OTHER");
				break;
			}
		}
	}
	// flight rules
	if (m_jsonfpl.isMember("flightrules")) {
		const Json::Value& rul1(m_jsonfpl["flightrules"]);
		if (rul1.isString()) {
			char rul(rul1.asString().c_str()[0]);
			xmlpp::Element *el(fplstrel->add_child("flightRules"));
			switch (rul) {
			default:
			case 'V':
				el->set_child_text("VFR");
				break;

			case 'I':
				el->set_child_text("IFR");
				break;

			case 'Z':
				el->set_child_text("VFR_THEN_IFR");
				break;

			case 'Y':
				el->set_child_text("IFR_THEN_VFR");
				break;
			}
		}
	}
	// EOBT
	if (m_jsonfpl.isMember("offblock")) {
		const Json::Value& eobt(m_jsonfpl["offblock"]);
		if (eobt.isIntegral()) {
			std::string t(Glib::TimeVal(eobt.asLargestUInt(), 0).as_iso8601());
			t[10] = ' ';
			xmlpp::Element *el(fplstrel->add_child("estimatedOffBlockTime"));
			el->set_child_text(t.substr(0, 16));
		}
	}
	// Route
	if (m_jsonfpl.isMember("item15")) {
		const Json::Value& route(m_jsonfpl["item15"]);
		if (route.isString()) {
			xmlpp::Element *el(fplstrel->add_child("icaoRoute"));
			el->set_child_text(route.asString());
		}
	}
	// Stay Info
	for (unsigned int i = 1; i < 10; ++i) {
		std::ostringstream oss;
		oss << "STAYINFO" << i;
		if (!otherinfo.isMember(oss.str()))
			break;
		const Json::Value& stay(otherinfo[oss.str()]);
		if (!stay.isString())
			break;
		fplstrel->add_child("stayInformation")->set_child_text(stay.asString());
	}
	// Equipment
	static const char * const equipstr[2] = { "NOT_EQUIPPED", "EQUIPPED" };
	if (m_jsonfpl.isMember("equipment")) {
		const Json::Value& equip1(m_jsonfpl["equipment"]);
		if (equip1.isString()) {
			std::string equip(equip1.asString());
			xmlpp::Element *el(fplstrel->add_child("equipmentCapabilityAndStatus"));
			el->add_child("dme")->set_child_text(equipstr[equip.find('D') != std::string::npos]);
			el->add_child("adf")->set_child_text(equipstr[equip.find('F') != std::string::npos]);
			el->add_child("gnss")->set_child_text(equipstr[equip.find('G') != std::string::npos]);
			el->add_child("standard")->set_child_text(equipstr[equip.find('S') != std::string::npos ||
									   (equip.find('V') != std::string::npos &&
									    equip.find('O') != std::string::npos &&
									    equip.find('L') != std::string::npos)]);
		}
	}
	// Surveillance
	if (m_jsonfpl.isMember("transponder")) {
		const Json::Value& xpdr1(m_jsonfpl["transponder"]);
		if (xpdr1.isString()) {
			std::string xpdr(xpdr1.asString());
			xmlpp::Element *el(fplstrel->add_child("surveillanceEquipment"));
			bool xe(xpdr.find('E') != std::string::npos);
			bool xh(xpdr.find('H') != std::string::npos);
			bool xi(xpdr.find('I') != std::string::npos);
			bool xl(xpdr.find('L') != std::string::npos);
			bool xs(xpdr.find('S') != std::string::npos);
			bool xp(xpdr.find('P') != std::string::npos);
			bool xx(xpdr.find('X') != std::string::npos);
			bool modes(xe || xh || xi || xl || xs || xp || xx);
			el->add_child("modeS")->set_child_text(equipstr[modes]);
			if (modes) {
				xmlpp::Element *el2(el->add_child("modeSCapabilities"));
				el2->add_child("aircraftIdentification")->set_child_text(equipstr[xe || xh || xi || xl || xs]);
				el2->add_child("pressureAltitude")->set_child_text(equipstr[xe || xh || xl || xp || xs]);
				el2->add_child("extendedSquitterADSB")->set_child_text(equipstr[xe || xl]);
				el2->add_child("enhancedSurveillance")->set_child_text(equipstr[xh || xl]);
			}			
		}
	}
	// Other Info
	{
		xmlpp::Element *el(fplstrel->add_child("otherInformation"));
		el->add_child("reasonForSpecialHandling");
		if (otherinfo.isMember("RMK")) {
			const Json::Value& rmk(otherinfo["RMK"]);
			if (rmk.isString())
				el->add_child("otherRemarks")->set_child_text(rmk.asString());
		}
	}
	// Supplementary Info
	{
		xmlpp::Element *el(fplstrel->add_child("supplementaryInformation"));
		if (m_jsonfpl.isMember("endurance")) {
			const Json::Value& end(m_jsonfpl["endurance"]);
			if (end.isIntegral()) {
				unsigned int m(end.asLargestUInt());
				if (m) {
					m += 30;
					m /= 60;
					unsigned int h(m / 60);
					m -= h * 60;
					std::ostringstream oss;
					oss << std::setfill('0') << std::setw(2) << h << std::setfill('0') << std::setw(2) << m;
					xmlpp::Element *el2(el->add_child("fuelEndurance"));
					el2->set_child_text(oss.str());
				}
			}
		}
		if (m_jsonfpl.isMember("personsonboard")) {
			const Json::Value& pob(m_jsonfpl["personsonboard"]);
			if (pob.isIntegral()) {
				std::ostringstream oss;
				oss << pob.asLargestUInt();
				xmlpp::Element *el2(el->add_child("numberOfPersons"));
				el2->set_child_text(oss.str());
			}
		}
		if (m_jsonfpl.isMember("emergencyradio")) {
			const Json::Value& emerg1(m_jsonfpl["emergencyradio"]);
			if (emerg1.isString()) {
				std::string emerg(emerg1.asString());
				if (emerg.find('E') != std::string::npos)
					el->add_child("frequencyAvailability")->set_child_text("ELT");
				if (emerg.find('V') != std::string::npos)
					el->add_child("frequencyAvailability")->set_child_text("VHF");
				if (emerg.find('U') != std::string::npos)
					el->add_child("frequencyAvailability")->set_child_text("UHF");
			}
		}
		// FIXME: survival stuff
		if (m_jsonfpl.isMember("colourmarkings")) {
			const Json::Value& col(m_jsonfpl["colourmarkings"]);
			if (col.isString())
				el->add_child("aircraftColourAndMarkings")->set_child_text(col.asString());
		}
		if (m_jsonfpl.isMember("picname")) {
			const Json::Value& pil(m_jsonfpl["picname"]);
			if (pil.isString())
				el->add_child("pilotInCommand")->set_child_text(pil.asString());
		}
	}
	// trajectory info


	//<xs:element name="ifplId" type="flight:IFPLId" minOccurs="0" maxOccurs="1"/>
	//<xs:element name="airFiledData" type="flight:AirFiledData" minOccurs="0" maxOccurs="1"/>
	//<xs:element name="aerodromeOfDeparture" type="flight:Aerodrome" minOccurs="0" maxOccurs="1"/>
	//<xs:element name="aerodromesOfDestination" type="flight:AerodromesOfDestination" minOccurs="0" maxOccurs="1"/>
	//<xs:element name="enrouteAlternateAerodromes" type="flight:AlternateAerodrome_DataType" minOccurs="0" maxOccurs="1"/>
	//<xs:element name="takeOffAlternateAerodromes" type="flight:AlternateAerodrome_DataType" minOccurs="0" maxOccurs="1"/>
	//<xs:element name="aircraftId" type="flight:AircraftIdentification" minOccurs="0" maxOccurs="1"/>
	//<xs:element name="whatIfRerouteReference" minOccurs="0" maxOccurs="1"><xs:simpleType><xs:restriction base="xs:int"></xs:restriction></xs:simpleType></xs:element>
	//<xs:element name="numberOfAircraft" minOccurs="0" maxOccurs="1"><xs:simpleType><xs:restriction base="xs:int"></xs:restriction></xs:simpleType></xs:element>
	//<xs:element name="aircraftType" type="flight:AircraftType" minOccurs="0" maxOccurs="1"/>
	//<xs:element name="totalEstimatedElapsedTime" type="common:DurationHourMinute" minOccurs="0" maxOccurs="1"/>
	//<xs:element name="eetsToLocations" type="flight:EstimatedElapsedTimeAtLocation" minOccurs="0" maxOccurs="unbounded"/>
	//<xs:element name="wakeTurbulenceCategory" type="flight:WakeTurbulenceCategory" minOccurs="0" maxOccurs="1"/>
	//<xs:element name="flightType" type="flight:FlightType" minOccurs="0" maxOccurs="1"/>
	//<xs:element name="flightRules" type="flight:FlightRules" minOccurs="0" maxOccurs="1"/>
	//<xs:element name="estimatedOffBlockTime" type="common:DateTimeMinute" minOccurs="0" maxOccurs="1"/>
	//<xs:element name="icaoRoute" minOccurs="0" maxOccurs="1"><xs:simpleType><xs:restriction base="xs:string"></xs:restriction></xs:simpleType></xs:element>
	//<xs:element name="stayInformation" type="flight:StayInformation_DataType" minOccurs="0" maxOccurs="9"/>
	//<xs:element name="enrouteDelays" type="flight:EnrouteDelay" minOccurs="0" maxOccurs="unbounded"/>
	//<xs:element name="equipmentCapabilityAndStatus" type="flight:EquipmentCapabilityAndStatus" minOccurs="0" maxOccurs="1"/>
	//<xs:element name="surveillanceEquipment" type="flight:SurveillanceEquipment" minOccurs="0" maxOccurs="1"/>
	//<xs:element name="otherInformation" type="flight:OtherInformation" minOccurs="0" maxOccurs="1"/>
	//<xs:element name="supplementaryInformation" type="flight:SupplementaryInformation" minOccurs="0" maxOccurs="1"/>
}

void CFMUValidateServer::Client::handle_input(void)
{
	if (false)
		std::cerr << "handle_input: " << m_fplan << std::endl;
	if (m_fplan.empty())
		return;
	// check for pseudo instructions
	if (!m_fplan.compare(0, 6, "trace?")) {
		Results res;
		res.print_tracerules(m_server->m_reval);
		set_result(res);
		return;
	}
	if (!m_fplan.compare(0, 6, "trace:") || !m_fplan.compare(0, 6, "trace+")) {
		Glib::ustring txt1(m_fplan);
		Glib::ustring::size_type n(txt1.find('\n'));
		if (n != Glib::ustring::npos)
			txt1.erase(n);
		n = txt1.find(' ');
		if (n != Glib::ustring::npos)
			txt1.erase(n);
		uint64_t tm(time(0));
		for (ADR::RestrictionEval::rules_t::const_iterator ri(m_server->m_reval.get_rules().begin()), re(m_server->m_reval.get_rules().end()); ri != re; ++ri) {
			const ADR::Object::ptr_t& p(*ri);
			ADR::FlightRestrictionTimeSlice& ts(p->operator()(tm).as_flightrestriction());
			if (!ts.is_valid())
				continue;
			if (ts.get_ident() != txt1)
				continue;
			ts.set_trace(true);
		}
		Results res;
		res.print_tracerules(m_server->m_reval);
		set_result(res);
		return;
	}
	if (!m_fplan.compare(0, 6, "trace-")) {
		Glib::ustring txt1(m_fplan);
		Glib::ustring::size_type n(txt1.find('\n'));
		if (n != Glib::ustring::npos)
			txt1.erase(n);
		n = txt1.find(' ');
		if (n != Glib::ustring::npos)
			txt1.erase(n);
		uint64_t tm(time(0));
		for (ADR::RestrictionEval::rules_t::const_iterator ri(m_server->m_reval.get_rules().begin()), re(m_server->m_reval.get_rules().end()); ri != re; ++ri) {
			const ADR::Object::ptr_t& p(*ri);
			ADR::FlightRestrictionTimeSlice& ts(p->operator()(tm).as_flightrestriction());
			if (!ts.is_valid())
				continue;
			if (ts.get_ident() != txt1)
				continue;
			ts.set_trace(false);
		}
		Results res;
		res.print_tracerules(m_server->m_reval);
		set_result(res);
		return;
	}
	if (!m_fplan.compare(0, 6, "trace*")) {
		uint64_t tm(time(0));
		for (ADR::RestrictionEval::rules_t::const_iterator ri(m_server->m_reval.get_rules().begin()), re(m_server->m_reval.get_rules().end()); ri != re; ++ri) {
			const ADR::Object::ptr_t& p(*ri);
			ADR::FlightRestrictionTimeSlice& ts(p->operator()(tm).as_flightrestriction());
			if (!ts.is_valid())
				continue;
			ts.set_trace(false);
		}
		Results res;
		res.print_tracerules(m_server->m_reval);
		set_result(res);
		return;
	}
	if (!m_fplan.compare(0, 9, "disabled?")) {
	        Results res;
		res.print_disabledrules(m_server->m_reval);
		set_result(res);
		return;
	}
	if (!m_fplan.compare(0, 9, "disabled:") || !m_fplan.compare(0, 9, "disabled+")) {
		Glib::ustring txt1(m_fplan);
		Glib::ustring::size_type n(txt1.find('\n'));
		if (n != Glib::ustring::npos)
			txt1.erase(n);
		n = txt1.find(' ');
		if (n != Glib::ustring::npos)
			txt1.erase(n);
		uint64_t tm(time(0));
		for (ADR::RestrictionEval::rules_t::const_iterator ri(m_server->m_reval.get_rules().begin()), re(m_server->m_reval.get_rules().end()); ri != re; ++ri) {
			const ADR::Object::ptr_t& p(*ri);
			ADR::FlightRestrictionTimeSlice& ts(p->operator()(tm).as_flightrestriction());
			if (!ts.is_valid())
				continue;
			if (ts.get_ident() != txt1)
				continue;
			ts.set_enabled(false);
		}
		Results res;
		res.print_disabledrules(m_server->m_reval);
		set_result(res);
		return;
	}
	if (!m_fplan.compare(0, 9, "disabled-")) {
		Glib::ustring txt1(m_fplan);
		Glib::ustring::size_type n(txt1.find('\n'));
		if (n != Glib::ustring::npos)
			txt1.erase(n);
		n = txt1.find(' ');
		if (n != Glib::ustring::npos)
			txt1.erase(n);
		uint64_t tm(time(0));
		for (ADR::RestrictionEval::rules_t::const_iterator ri(m_server->m_reval.get_rules().begin()), re(m_server->m_reval.get_rules().end()); ri != re; ++ri) {
			const ADR::Object::ptr_t& p(*ri);
			ADR::FlightRestrictionTimeSlice& ts(p->operator()(tm).as_flightrestriction());
			if (!ts.is_valid())
				continue;
			if (ts.get_ident() != txt1)
				continue;
			ts.set_enabled(true);
		}
		Results res;
		res.print_disabledrules(m_server->m_reval);
		set_result(res);
		return;
	}
	if (!m_fplan.compare(0, 9, "disabled*")) {
		uint64_t tm(time(0));
		for (ADR::RestrictionEval::rules_t::const_iterator ri(m_server->m_reval.get_rules().begin()), re(m_server->m_reval.get_rules().end()); ri != re; ++ri) {
			const ADR::Object::ptr_t& p(*ri);
			ADR::FlightRestrictionTimeSlice& ts(p->operator()(tm).as_flightrestriction());
			if (!ts.is_valid())
				continue;
			ts.set_enabled(true);
		}
		Results res;
		res.print_disabledrules(m_server->m_reval);
		set_result(res);
		return;
	}
	if (!m_fplan.compare(0, 9, "validate?")) {
		Results res;
		res.push_back("validate:" + to_str(m_validator));
		set_result(res);
		return;
	}
	if (!m_fplan.compare(0, 9, "validate:")) {
		Glib::ustring txt1(m_fplan.substr(9));
		Glib::ustring::size_type n(txt1.find('\n'));
		if (n != Glib::ustring::npos)
			txt1.erase(n);
		n = txt1.find(' ');
		if (n != Glib::ustring::npos)
			txt1.erase(n);
		for (validator_t v = validator_cfmu; v <= validator_local; v = (validator_t)(v + 1)) {
			if (txt1 != to_str(v))
				continue;
			m_validator = v;
			break;
		}
		Results res;
		res.push_back("validate:" + to_str(m_validator));
		set_result(res);
		return;
	}
	if (!m_fplan.compare(0, 10, "validate*:")) {
		Glib::ustring txt1(m_fplan.substr(10));
		Glib::ustring::size_type n(txt1.find('\n'));
		if (n != Glib::ustring::npos)
			txt1.erase(n);
		n = txt1.find(' ');
		if (n != Glib::ustring::npos)
			txt1.erase(n);
		for (validator_t v = validator_cfmu; v <= validator_local; v = (validator_t)(v + 1)) {
			if (txt1 != to_str(v))
				continue;
			m_validator = v;
			break;
		}
		m_fplan.clear();
		return;
	}
	if (!m_fplan.compare(0, 9, "annotate?")) {
		Results res;
		res.push_back((std::string)"annotate:" + (m_annotate ? "on" : "off"));
		set_result(res);
		return;
	}
	if (!m_fplan.compare(0, 11, "annotate:on")) {
		m_annotate = true;
		Results res;
		if (m_server)
			m_server->annotate(res);
		res.push_back("annotate:on");
		set_result(res);
		return;
	}
	if (!m_fplan.compare(0, 12, "annotate:off")) {
		m_annotate = false;
		Results res;
		res.push_back("annotate:off");
		set_result(res);
		return;
	}
	if (!m_fplan.compare(0, 8, "suggest:")) {
		m_fplan.erase(0, 8);
		m_jsonfpl = Json::Value();
		if (!m_fplan.compare(0, 1, "{")) {
			Json::Reader reader;
			if (reader.parse(m_fplan, m_jsonfpl, false) && m_jsonfpl.isObject()) {
				m_fplan.clear();
				if (m_jsonfpl.isMember("fpl")) {
					const Json::Value& fpl(m_jsonfpl["fpl"]);
					if (fpl.isString())
						m_fplan = fpl.asString();
				}
			} else {
				m_jsonfpl = Json::Value();
			}
		}
		switch (m_validator) {
#ifdef HAVE_CURL
		case validator_cfmub2bop:
		case validator_cfmub2bpreop:
		{
			const std::string *url(&m_cfmub2bopurl);
			const std::string *cert(&m_cfmub2bopcert);
			if (m_validator == validator_cfmub2bpreop) {
				url = &m_cfmub2bpreopurl;
				cert = &m_cfmub2bpreopcert;
			}
			if (url->empty() || cert->empty()) {
				Results res;
				res.push_back("FAIL:Validator not configured");
				set_result(res);
				return;
			}
			close_curl();
			m_curlrxbuffer.clear();
			m_curltxbuffer.clear();
			m_curl = curl_easy_init();
			if (!m_curl) {
				Results res;
				res.push_back("FAIL:Curl Context");
				set_result(res);
				return;
			}
			{
				std::unique_ptr<xmlpp::Document> doc(new xmlpp::Document());
				xmlpp::Element *rootel(doc->create_root_node("Envelope", "http://schemas.xmlsoap.org/soap/envelope/", "S"));
				rootel->set_namespace_declaration("eurocontrol/cfmu/b2b/FlightServices", "flight");
				//rootel->set_namespace_declaration("eurocontrol/cfmu/b2b/CommonServices", "common");
				xmlpp::Element *bodyel(rootel->add_child("Body", "S"));
				xmlpp::Element *valreqel(bodyel->add_child("RoutingAssistanceRequest", "flight"));
				xmlpp::Element *sndtel(valreqel->add_child("sendTime"));
				xmlpp::Element *fplel(valreqel->add_child("flightPlan"));
				if (m_jsonfpl.isObject()) {
					eurocontrol_structural(fplel);
					if (m_jsonfpl.isMember("adepFreezePoint")) {
						const Json::Value& fp(m_jsonfpl["adepFreezePoint"]);
						if (fp.isString()) {
							xmlpp::Element *fpel(valreqel->add_child("adepFreezePoint"));
							fpel->set_child_text(fp.asString().c_str());
						}
					}
					if (m_jsonfpl.isMember("adesFreezePoint")) {
						const Json::Value& fp(m_jsonfpl["adesFreezePoint"]);
						if (fp.isString()) {
							xmlpp::Element *fpel(valreqel->add_child("adesFreezePoint"));
							fpel->set_child_text(fp.asString().c_str());
						}
					}
					if (m_jsonfpl.isMember("nbRoutes")) {
						const Json::Value& nr(m_jsonfpl["nbRoutes"]);
						if (nr.isIntegral()) {
							xmlpp::Element *nrel(valreqel->add_child("nbRoutes"));
							std::ostringstream oss;
							oss << nr.asInt();
							nrel->set_child_text(oss.str().c_str());
						}
					}
					if (m_jsonfpl.isMember("radOff")) {
						const Json::Value& ro(m_jsonfpl["radOff"]);
						if (ro.isBool()) {
							xmlpp::Element *roel(valreqel->add_child("radOff"));
							roel->set_child_text(ro.asBool() ? "true" : "false");
						}
					}
					if (m_jsonfpl.isMember("avoidAirspaces")) {
						const Json::Value& arr(m_jsonfpl["avoidAirspaces"]);
						if (arr.isArray()) {
							for (Json::ArrayIndex i = 0; i < arr.size(); ++i) {
								const Json::Value& ae(arr[i]);
								if (!ae.isString())
									continue;
								xmlpp::Element *arrel(valreqel->add_child("avoidAirspaces"));
								arrel->set_child_text(ae.asString().c_str());
							}
						}
					}
					if (m_jsonfpl.isMember("avoidPoints")) {
						const Json::Value& arr(m_jsonfpl["avoidPoints"]);
						if (arr.isArray()) {
							for (Json::ArrayIndex i = 0; i < arr.size(); ++i) {
								const Json::Value& ae(arr[i]);
								if (!ae.isString())
									continue;
								xmlpp::Element *arrel(valreqel->add_child("avoidPoints"));
								arrel->set_child_text(ae.asString().c_str());
							}
						}
					}
					if (m_jsonfpl.isMember("viaAirspaces")) {
						const Json::Value& arr(m_jsonfpl["viaAirspaces"]);
						if (arr.isArray()) {
							for (Json::ArrayIndex i = 0; i < arr.size(); ++i) {
								const Json::Value& ae(arr[i]);
								if (!ae.isString())
									continue;
								xmlpp::Element *arrel(valreqel->add_child("viaAirspaces"));
								arrel->set_child_text(ae.asString().c_str());
							}
						}
					}
					if (m_jsonfpl.isMember("viaPoints")) {
						const Json::Value& arr(m_jsonfpl["viaPoints"]);
						if (arr.isArray()) {
							for (Json::ArrayIndex i = 0; i < arr.size(); ++i) {
								const Json::Value& ae(arr[i]);
								if (!ae.isString())
									continue;
								xmlpp::Element *arrel(valreqel->add_child("viaPoints"));
								arrel->set_child_text(ae.asString().c_str());
							}
						}
					}
				} else {
					eurocontrol_textual(fplel);
				}
				Glib::DateTime dt(Glib::DateTime::create_now_utc());
				sndtel->set_child_text(dt.format("%Y-%m-%d %H:%M:%S").c_str());
				if (!false)
					std::cerr << "transmitting: " << doc->write_to_string_formatted() << std::endl;
				Glib::ustring q(doc->write_to_string());
				//m_curltxbuffer.insert(m_curltxbuffer.end(), q.begin(), q.end());
				m_curltxbuffer.resize(q.size());
				if (m_curltxbuffer.size())
					memcpy(&m_curltxbuffer[0], q.c_str(), m_curltxbuffer.size());
			}
			m_curl_headers = curl_slist_append(m_curl_headers, "Content-Type: text/xml; charset=utf-8");
			{
				std::ostringstream oss;
				oss << "Content-Length: " << m_curltxbuffer.size();
				m_curl_headers = curl_slist_append(m_curl_headers, oss.str().c_str());
			}
			m_curl_headers = curl_slist_append(m_curl_headers, "SOAPAction: \"\"");
			curl_easy_setopt(m_curl, CURLOPT_HTTPHEADER, m_curl_headers);
			curl_easy_setopt(m_curl, CURLOPT_URL, url->c_str());
			curl_easy_setopt(m_curl, CURLOPT_WRITEDATA, this);
			curl_easy_setopt(m_curl, CURLOPT_WRITEFUNCTION, my_write_func_1);
			curl_easy_setopt(m_curl, CURLOPT_READDATA, this);
			curl_easy_setopt(m_curl, CURLOPT_READFUNCTION, my_read_func_1);
			curl_easy_setopt(m_curl, CURLOPT_NOPROGRESS, 0L);
			curl_easy_setopt(m_curl, CURLOPT_PROGRESSFUNCTION, my_progress_func_1);
			curl_easy_setopt(m_curl, CURLOPT_PROGRESSDATA, this);
			curl_easy_setopt(m_curl, CURLOPT_POST, 1L);
			if (false && (curl_easy_setopt(m_curl, CURLOPT_SSLENGINE, 0) != CURLE_OK ||
				      curl_easy_setopt(m_curl, CURLOPT_SSLENGINE_DEFAULT, 1L) != CURLE_OK)) {
				close_curl();
				Results res;
				res.push_back("FAIL:Curl SSL Setup");
				set_result(res);
				return;
			}
			curl_easy_setopt(m_curl, CURLOPT_SSLCERTTYPE, "PEM");
			curl_easy_setopt(m_curl, CURLOPT_SSLCERT, cert->c_str());
			//curl_easy_setopt(m_curl, CURLOPT_KEYPASSWD, "");
			curl_easy_setopt(m_curl, CURLOPT_SSLKEYTYPE, "PEM");
			curl_easy_setopt(m_curl, CURLOPT_SSLKEY, cert->c_str());
			curl_easy_setopt(m_curl, CURLOPT_CAINFO, "/etc/pki/tls/certs/ca-bundle.crt");
			//curl_easy_setopt(m_curl, CURLOPT_SSL_VERIFYPEER, 1L);
			curl_easy_setopt(m_curl, CURLOPT_SSL_VERIFYPEER, 1L);
			//curl_easy_setopt(m_curl, CURLOPT_VERBOSE, 1L);
			m_server->curl_add(m_curl, get_ptr());
			break;
		}
#endif

		default:
		{
			Results res;
			res.push_back("FAIL:Invalid Validator");
			set_result(res);
			return;
		}
		}


		return;
	}	
	m_jsonfpl = Json::Value();
	if (!m_fplan.compare(0, 1, "{")) {
		Json::Reader reader;
		if (reader.parse(m_fplan, m_jsonfpl, false) && m_jsonfpl.isObject()) {
			m_fplan.clear();
			if (m_jsonfpl.isMember("fpl")) {
				const Json::Value& fpl(m_jsonfpl["fpl"]);
				if (fpl.isString())
					m_fplan = fpl.asString();
			}
		} else {
			m_jsonfpl = Json::Value();
		}
	}
	if (!m_server) {
		Results res;
		res.push_back("FAIL:Internal Error");
		set_result(res);
		return;
	}
	switch (m_validator) {
	case validator_cfmu:
		m_server->validate_cfmu(get_ptr());
		break;

	case validator_local:
		m_server->validate_locally(get_ptr());
		break;

#ifdef HAVE_CURL
	case validator_eurofpl:
	{
		close_curl();
		m_curlrxbuffer.clear();
		m_curltxbuffer.clear();
		m_curl = curl_easy_init();
		if (!m_curl) {
			Results res;
			res.push_back("FAIL:Curl Context");
			set_result(res);
			return;
		}
		//m_curl_headers = curl_slist_append(m_curl_headers, "Content-Type: application/xml");
		curl_easy_setopt(m_curl, CURLOPT_HTTPHEADER, m_curl_headers);
		curl_easy_setopt(m_curl, CURLOPT_URL, "http://validation.eurofpl.eu");
		curl_easy_setopt(m_curl, CURLOPT_WRITEDATA, this);
		curl_easy_setopt(m_curl, CURLOPT_WRITEFUNCTION, my_write_func_1);
		curl_easy_setopt(m_curl, CURLOPT_READDATA, this);
		curl_easy_setopt(m_curl, CURLOPT_READFUNCTION, my_read_func_1);
		curl_easy_setopt(m_curl, CURLOPT_NOPROGRESS, 0L);
		curl_easy_setopt(m_curl, CURLOPT_PROGRESSFUNCTION, my_progress_func_1);
		curl_easy_setopt(m_curl, CURLOPT_PROGRESSDATA, this);
		curl_easy_setopt(m_curl, CURLOPT_REFERER, "http://validation.eurofpl.eu");
		curl_easy_setopt(m_curl, CURLOPT_POST, 1L);
		std::string query("freeEntry=");
		{
			unsigned int skipdash(!m_fplan.empty() && m_fplan[0] == '-');
			char *postfld(curl_easy_escape(m_curl, m_fplan.c_str() + skipdash, m_fplan.size() - skipdash));
			query += postfld;
			curl_free(postfld);
		}
		curl_easy_setopt(m_curl, CURLOPT_COPYPOSTFIELDS, query.c_str());
		m_server->curl_add(m_curl, get_ptr());
		break;
	}

	case validator_cfmub2bop:
	case validator_cfmub2bpreop:
	{
		const std::string *url(&m_cfmub2bopurl);
		const std::string *cert(&m_cfmub2bopcert);
		if (m_validator == validator_cfmub2bpreop) {
			url = &m_cfmub2bpreopurl;
			cert = &m_cfmub2bpreopcert;
		}
		if (url->empty() || cert->empty()) {
			Results res;
			res.push_back("FAIL:Validator not configured");
			set_result(res);
			return;
		}
		close_curl();
		m_curlrxbuffer.clear();
		m_curltxbuffer.clear();
		m_curl = curl_easy_init();
		if (!m_curl) {
			Results res;
			res.push_back("FAIL:Curl Context");
			set_result(res);
			return;
		}
		{
			std::unique_ptr<xmlpp::Document> doc(new xmlpp::Document());
			xmlpp::Element *rootel(doc->create_root_node("Envelope", "http://schemas.xmlsoap.org/soap/envelope/", "S"));
			rootel->set_namespace_declaration("eurocontrol/cfmu/b2b/FlightServices", "flight");
			//rootel->set_namespace_declaration("eurocontrol/cfmu/b2b/CommonServices", "common");
			xmlpp::Element *bodyel(rootel->add_child("Body", "S"));
			if (false) {
				xmlpp::Element *valreqel(bodyel->add_child("ExtendedFlightPlanValidationRequest", "flight"));
				xmlpp::Element *sndtel(valreqel->add_child("sendTime"));
				xmlpp::Element *fplel(valreqel->add_child("flightPlan"));
				if (m_jsonfpl.isObject())
					eurocontrol_structural(fplel);
				else
					eurocontrol_textual(fplel);
				Glib::DateTime dt(Glib::DateTime::create_now_utc());
				sndtel->set_child_text(dt.format("%Y-%m-%d %H:%M:%S").c_str());
#if 0
				xmlpp::Element *fplreqfld0(valreqel->add_child("requestedFields"));
				fplreqfld0->set_child_text("fourDimensionalTrajectory");
				xmlpp::Element *fplreqfld1(valreqel->add_child("requestedFields"));
				fplreqfld1->set_child_text("profileTuningRestrictions");
#endif
			} else {
				xmlpp::Element *valreqel(bodyel->add_child("FlightPlanValidationRequest", "flight"));
				xmlpp::Element *sndtel(valreqel->add_child("sendTime"));
				xmlpp::Element *fplel(valreqel->add_child("flightPlan"));
				if (m_jsonfpl.isObject())
					eurocontrol_structural(fplel);
				else
					eurocontrol_textual(fplel);
				Glib::DateTime dt(Glib::DateTime::create_now_utc());
				sndtel->set_child_text(dt.format("%Y-%m-%d %H:%M:%S").c_str());
			}
			if (!false)
				std::cerr << "transmitting: " << doc->write_to_string_formatted() << std::endl;
			Glib::ustring q(doc->write_to_string());
			//m_curltxbuffer.insert(m_curltxbuffer.end(), q.begin(), q.end());
			m_curltxbuffer.resize(q.size());
			if (m_curltxbuffer.size())
				memcpy(&m_curltxbuffer[0], q.c_str(), m_curltxbuffer.size());
		}
		m_curl_headers = curl_slist_append(m_curl_headers, "Content-Type: text/xml; charset=utf-8");
		{
			std::ostringstream oss;
			oss << "Content-Length: " << m_curltxbuffer.size();
			m_curl_headers = curl_slist_append(m_curl_headers, oss.str().c_str());
		}
		m_curl_headers = curl_slist_append(m_curl_headers, "SOAPAction: \"\"");
		curl_easy_setopt(m_curl, CURLOPT_HTTPHEADER, m_curl_headers);
		curl_easy_setopt(m_curl, CURLOPT_URL, url->c_str());
		curl_easy_setopt(m_curl, CURLOPT_WRITEDATA, this);
		curl_easy_setopt(m_curl, CURLOPT_WRITEFUNCTION, my_write_func_1);
		curl_easy_setopt(m_curl, CURLOPT_READDATA, this);
		curl_easy_setopt(m_curl, CURLOPT_READFUNCTION, my_read_func_1);
		curl_easy_setopt(m_curl, CURLOPT_NOPROGRESS, 0L);
		curl_easy_setopt(m_curl, CURLOPT_PROGRESSFUNCTION, my_progress_func_1);
		curl_easy_setopt(m_curl, CURLOPT_PROGRESSDATA, this);
		curl_easy_setopt(m_curl, CURLOPT_POST, 1L);
		if (false && (curl_easy_setopt(m_curl, CURLOPT_SSLENGINE, 0) != CURLE_OK ||
			      curl_easy_setopt(m_curl, CURLOPT_SSLENGINE_DEFAULT, 1L) != CURLE_OK)) {
			close_curl();
			Results res;
			res.push_back("FAIL:Curl SSL Setup");
			set_result(res);
			return;
		}
		curl_easy_setopt(m_curl, CURLOPT_SSLCERTTYPE, "PEM");
		curl_easy_setopt(m_curl, CURLOPT_SSLCERT, cert->c_str());
		//curl_easy_setopt(m_curl, CURLOPT_KEYPASSWD, "");
		curl_easy_setopt(m_curl, CURLOPT_SSLKEYTYPE, "PEM");
		curl_easy_setopt(m_curl, CURLOPT_SSLKEY, cert->c_str());
		curl_easy_setopt(m_curl, CURLOPT_CAINFO, "/etc/pki/tls/certs/ca-bundle.crt");
		//curl_easy_setopt(m_curl, CURLOPT_SSL_VERIFYPEER, 1L);
		curl_easy_setopt(m_curl, CURLOPT_SSL_VERIFYPEER, 1L);
		//curl_easy_setopt(m_curl, CURLOPT_VERBOSE, 1L);
		m_server->curl_add(m_curl, get_ptr());
		break;
	}
#endif

	default:
	{
		Results res;
		res.push_back("FAIL:Invalid Validator");
		set_result(res);
		return;
	}
	}
}

#ifdef HAVE_CURL
void CFMUValidateServer::Client::close_curl(void)
{
	if (m_curl_headers)
		curl_slist_free_all(m_curl_headers);
	m_curl_headers = 0;
	if (!m_curl)
		return;
	if (m_server)
		m_server->curl_remove(m_curl);
	curl_easy_cleanup(m_curl);
	m_curl = 0;
}

size_t CFMUValidateServer::Client::my_write_func_1(const void *ptr, size_t size, size_t nmemb, Client *clnt)
{
        return clnt->my_write_func(ptr, size, nmemb);
}

size_t CFMUValidateServer::Client::my_read_func_1(void *ptr, size_t size, size_t nmemb, Client *clnt)
{
        return clnt->my_read_func(ptr, size, nmemb);
}

int CFMUValidateServer::Client::my_progress_func_1(Client *clnt, double t, double d, double ultotal, double ulnow)
{
        return clnt->my_progress_func(t, d, ultotal, ulnow);
}

size_t CFMUValidateServer::Client::my_write_func(const void *ptr, size_t size, size_t nmemb)
{
        unsigned int sz(size * nmemb);
        unsigned int p(m_curlrxbuffer.size());
        m_curlrxbuffer.resize(p + sz);
        memcpy(&m_curlrxbuffer[p], ptr, sz);
        return sz;
}

size_t CFMUValidateServer::Client::my_read_func(void *ptr, size_t size, size_t nmemb)
{
        unsigned int sz(size * nmemb);
	if (sz > m_curltxbuffer.size())
		sz = m_curltxbuffer.size();
	if (sz) {
		memcpy(ptr, &m_curltxbuffer[0], sz);
		m_curltxbuffer.erase(m_curltxbuffer.begin(), m_curltxbuffer.begin() + sz);
	}
        return sz;
}

int CFMUValidateServer::Client::my_progress_func(double t, double d, double ultotal, double ulnow)
{
        return 0;
}

void CFMUValidateServer::Client::find_eurofpl_results(std::string& result, xmlNode *a_node, unsigned int level)
{
	for (xmlNode *cur_node = a_node; cur_node; cur_node = cur_node->next) {
		if (cur_node->type == XML_ELEMENT_NODE) {
			xmlChar *cls(xmlGetProp(cur_node, BAD_CAST "class"));
			if (cls) {
				if (!xmlStrcmp(cls, BAD_CAST "ifpuv_result")) {
					for (xmlNode *cld_node = cur_node->children; cld_node; cld_node = cld_node->next) {
						if (cld_node->type == XML_ELEMENT_NODE && !xmlStrcmp(cld_node->name, BAD_CAST "br")) {
							result += "\r\n";
							continue;
						}
						if (cld_node->type == XML_TEXT_NODE) {
							result += (const char *)cld_node->content;
						}
					}
				}
			}
			if (false) {
				std::cerr << std::string(level, ' ') << "node type: Element, name: " << cur_node->name;
				if (cls)
					std::cerr << " class: " << (char *)cls;
				std::cerr << std::endl;
			}
			if (cls)
				xmlFree(cls);
			if (!result.empty())
				break;
		}
		find_eurofpl_results(result, cur_node->children, level + 1);
		if (!result.empty())
			break;
	}
}

void CFMUValidateServer::Client::parse_eurofpl(void)
{
	xmlDocPtr doc(htmlReadMemory(const_cast<char *>(&m_curlrxbuffer[0]), m_curlrxbuffer.size(), "http://validation.eurofpl.eu", 0, 0));
	xmlCleanupParser();
	std::string resultstr;
	find_eurofpl_results(resultstr, xmlDocGetRootElement(doc));
	xmlFreeDoc(doc);
	xmlCleanupParser();
	if (resultstr.empty()) {
		Results res;
		res.push_back("FAIL:ifpuv_result not found");
		set_result(res);
		return;
	}
	Results res;
	res.push_back(resultstr);
	resultstr.clear();
	for (;;) {
		Glib::ustring::size_type n(res.back().find_first_of("\r\n"));
		if (n == Glib::ustring::npos)
			break;
		Glib::ustring::size_type n1(n);
		++n1;
		while (n1 < res.back().size() && (res.back()[n1] == '\n' || res.back()[n1] == '\r'))
			++n1;
		Glib::ustring x(res.back().substr(n1));
		res.back().erase(n);
		res.push_back(x);
	}
	for (Results::size_type i(0); i < res.size(); ) {
		if (res[i].empty()) {
			res.erase(res.begin() + i);
			continue;
		}
		++i;
	}
	set_result(res);
}

void CFMUValidateServer::Client::parse_cfmub2b(void)
{
	if (!false)
		std::cerr << "received: " << std::string(&m_curlrxbuffer[0], m_curlrxbuffer.size()) << std::endl;
	xmlpp::DomParser p;
	p.set_validate(false);
	p.set_substitute_entities(false);
	//p.set_throw_messages(false);
 	p.parse_memory_raw((const unsigned char *)&m_curlrxbuffer[0], m_curlrxbuffer.size());
        if (!p) {
		Results res;
		res.push_back("FAIL:Unparseable XML");
		set_result(res);
                return;
	}
	if (false)
		std::cerr << "received: " << p.get_document()->write_to_string_formatted() << std::endl;
	const xmlpp::Element *rootel(p.get_document()->get_root_node());
	if (rootel->get_name() != "Envelope") {
		Results res;
		res.push_back("FAIL:XML:root element not Envelope");
		set_result(res);
                return;
	}
	xmlpp::Node::NodeList nlbody(rootel->get_children("Body"));
	for (xmlpp::Node::NodeList::const_iterator ni(nlbody.begin()), ne(nlbody.end()); ni != ne; ++ni) {
                const xmlpp::Element *bodyel(static_cast<const xmlpp::Element *>(*ni));
		if (!bodyel)
			continue;
		xmlpp::Node::NodeList nlval(bodyel->get_children("FlightPlanValidationReply"));
		for (xmlpp::Node::NodeList::const_iterator ni(nlval.begin()), ne(nlval.end()); ni != ne; ++ni) {
			const xmlpp::Element *valel(static_cast<const xmlpp::Element *>(*ni));
			if (!valel)
				continue;
			{
				xmlpp::Node::NodeList nlstat(valel->get_children("status"));
				xmlpp::Node::NodeList::const_iterator ni(nlstat.begin()), ne(nlstat.end());
				for (; ni != ne; ++ni) {
					std::string t(get_text(*ni));
					if (t.empty())
						continue;
					if (t == "OK")
						break;
					Results res;
					res.push_back("FAIL:CFMU:Status:" + t);
					set_result(res);
					return;
				}
				if (ni == ne)
					continue;
			}
			Results res;
			xmlpp::Node::NodeList nldata(valel->get_children("data"));
			for (xmlpp::Node::NodeList::const_iterator ni(nldata.begin()), ne(nldata.end()); ni != ne; ++ni) {
				const xmlpp::Element *datael(static_cast<const xmlpp::Element *>(*ni));
				if (!datael)
					continue;
				xmlpp::Node::NodeList nlerr(datael->get_children("ifpsErrors"));
				for (xmlpp::Node::NodeList::const_iterator ni(nlerr.begin()), ne(nlerr.end()); ni != ne; ++ni) {
					const xmlpp::Element *errel(static_cast<const xmlpp::Element *>(*ni));
					if (!errel)
						continue;
					std::string code;
					std::string descr;
					xmlpp::Node::NodeList nl(errel->get_children("code"));
					for (xmlpp::Node::NodeList::const_iterator ni(nl.begin()), ne(nl.end()); ni != ne; ++ni)
						code += get_text(*ni);
					nl = errel->get_children("description");
					for (xmlpp::Node::NodeList::const_iterator ni(nl.begin()), ne(nl.end()); ni != ne; ++ni)
						descr += get_text(*ni);
					res.push_back(code + ": " + descr);
				}
			}
			if (res.empty())
				res.push_back("NO ERRORS");
			set_result(res);
			return;
		}
		nlval = bodyel->get_children("RoutingAssistanceReply");
		for (xmlpp::Node::NodeList::const_iterator ni(nlval.begin()), ne(nlval.end()); ni != ne; ++ni) {
			const xmlpp::Element *valel(static_cast<const xmlpp::Element *>(*ni));
			if (!valel)
				continue;
			{
				xmlpp::Node::NodeList nlstat(valel->get_children("status"));
				xmlpp::Node::NodeList::const_iterator ni(nlstat.begin()), ne(nlstat.end());
				for (; ni != ne; ++ni) {
					std::string t(get_text(*ni));
					if (t.empty())
						continue;
					if (t == "OK")
						break;
					Results res;
					res.push_back("FAIL:CFMU:Status:" + t);
					set_result(res);
					return;
				}
				if (ni == ne)
					continue;
			}
			Results res;
			xmlpp::Node::NodeList nldata(valel->get_children("data"));
			for (xmlpp::Node::NodeList::const_iterator ni(nldata.begin()), ne(nldata.end()); ni != ne; ++ni) {
				const xmlpp::Element *datael(static_cast<const xmlpp::Element *>(*ni));
				if (!datael)
					continue;
				xmlpp::Node::NodeList nlres(datael->get_children("result"));
				for (xmlpp::Node::NodeList::const_iterator ni(nlres.begin()), ne(nlres.end()); ni != ne; ++ni) {
					const xmlpp::Element *resel(static_cast<const xmlpp::Element *>(*ni));
					if (!resel)
						continue;
					xmlpp::Node::NodeList nlrte(resel->get_children("proposedRoutes"));
					for (xmlpp::Node::NodeList::const_iterator ni(nlrte.begin()), ne(nlrte.end()); ni != ne; ++ni) {
						const xmlpp::Element *rteel(static_cast<const xmlpp::Element *>(*ni));
						if (!rteel)
							continue;
						{
							xmlpp::Node::NodeList nlerr(rteel->get_children("routeErrors"));
							if (!nlerr.empty())
								continue;
						}
						std::string icaoroute;
						{
							xmlpp::Node::NodeList nlirte(rteel->get_children("icaoRoute"));
							for (xmlpp::Node::NodeList::const_iterator ni(nlirte.begin()), ne(nlirte.end()); ni != ne; ++ni) {
								const xmlpp::Element *irteel(static_cast<const xmlpp::Element *>(*ni));
								if (!irteel)
									continue;
								icaoroute = get_text(*ni);
								if (!icaoroute.empty())
									break;
							}
						}
						if (icaoroute.empty())
							continue;
						res.push_back("SUGGEST:" + icaoroute);
					}
				}
			}
			if (res.empty())
				res.push_back("FAIL:NO ROUTE FOUND");
			set_result(res);
			return;			
		}
	}
	Results res;
	res.push_back("FAIL:XML:no result element");
	set_result(res);
}

std::string CFMUValidateServer::Client::get_text(const xmlpp::Node *n)
{
        const xmlpp::Element *el(static_cast<const xmlpp::Element *>(n));
        if (!el)
                return "";
        const xmlpp::TextNode *t(el->get_child_text());
        if (!t)
                return "";
        return t->get_content();
}

void CFMUValidateServer::Client::curl_done(CURLcode result)
{
	if (result != CURLE_OK) {
		std::ostringstream oss;
		oss << "FAIL:CURL " << curl_easy_strerror(result);
		Results res;
		res.push_back(oss.str());
		set_result(res);
		m_curlrxbuffer.clear();
		return;
	}
	if (false)
		std::cerr << "received: " << std::string((char *)&m_curlrxbuffer[0], ((char *)&m_curlrxbuffer[0]) + m_curlrxbuffer.size()) << std::endl;
	switch (m_validator) {
	case validator_eurofpl:
		parse_eurofpl();
		break;

	case validator_cfmub2bop:
	case validator_cfmub2bpreop:
		parse_cfmub2b();
		break;

	default:
	{
		Results res;
		res.push_back("FAIL:Invalid Validator");
		set_result(res);
		break;
	}
	}
	m_curlrxbuffer.clear();
}

#endif

CFMUValidateServer::StdioClient::StdioClient(CFMUValidateServer *server)
	: Client(server), m_inbufptr(0)
{
#ifdef G_OS_WIN32
        m_inputchan = Glib::IOChannel::create_from_win32_fd(0);
        m_outputchan = Glib::IOChannel::create_from_win32_fd(1);	
#else
        m_inputchan = Glib::IOChannel::create_from_fd(0);
        m_outputchan = Glib::IOChannel::create_from_fd(1);
#endif
        m_inputchan->set_encoding();
        m_outputchan->set_encoding();
#ifndef G_OS_WIN32
	m_inputchan->set_flags(m_inputchan->get_flags() | Glib::IO_FLAG_NONBLOCK);
#endif
	m_inputreaderconn = Glib::signal_io().connect(sigc::mem_fun(this, &StdioClient::input_line_handler),
						      m_inputchan, Glib::IO_IN | Glib::IO_ERR | Glib::IO_HUP);
}

CFMUValidateServer::StdioClient::~StdioClient()
{
	m_inputreaderconn.disconnect();
	m_inputchan.reset();
	m_outputchan.reset();
}

void CFMUValidateServer::StdioClient::set_result(const Results& res)
{
	if (!m_outputchan) {
		m_inputreaderconn.disconnect();
		Client::ptr_t p(get_ptr());
		if (m_server)
			m_server->close(p);
		return;
	}
	Results res1(res);
	if (m_annotate && m_server && m_validator != validator_local)
		m_server->annotate(res1);
	for (Results::const_iterator i(res1.begin()), e(res1.end()); i != e; ++i)
		m_outputchan->write(*i + "\n");
	m_outputchan->write("\n");
	m_outputchan->flush();
	m_fplan.clear();
	handle_input_loop();
}

bool CFMUValidateServer::StdioClient::input_line_handler(Glib::IOCondition iocond)
{
	if (!m_inputchan) {
		handle_input_loop();
		return false;
        }
	gsize nrread;
#ifdef G_OS_WIN32
	Glib::IOStatus iostat(m_inputchan->read(m_inbuffer + m_inbufptr, 1, nrread));
#else
	Glib::IOStatus iostat(m_inputchan->read(m_inbuffer + m_inbufptr, sizeof(m_inbuffer) - m_inbufptr, nrread));
#endif
        if (iostat == Glib::IO_STATUS_ERROR ||
            iostat == Glib::IO_STATUS_EOF) {
		m_inputreaderconn.disconnect();
		m_inputchan.reset();
		m_outputchan.reset();
		handle_input_loop();
		return false;
        }
	m_inbufptr += nrread;
	handle_input_loop();
	return true;
}

void CFMUValidateServer::StdioClient::handle_input_loop(void)
{
	for (;;) {
		if (!m_inputchan || !m_outputchan) {
			m_inputreaderconn.disconnect();
			Client::ptr_t p(get_ptr());
			if (m_server)
				m_server->close(p);
			return;
		}
		if (!m_fplan.empty()) {
			m_inputreaderconn.disconnect();
			return;
		}
		if (false)
			std::cerr << "handle_input_loop: buffer count " << m_inbufptr << std::endl;
		char *cp(static_cast<char *>(memchr(m_inbuffer, '\n', m_inbufptr)));
		{
			char *cp2(static_cast<char *>(memchr(m_inbuffer, '\r', m_inbufptr)));
			if (!cp || (cp2 && cp2 < cp))
				cp = cp2;
		}
		if (!cp) {
			if (m_inbufptr >= sizeof(m_inbuffer))
				m_inbufptr = 0;
			m_inputreaderconn.disconnect();
			if (m_inputchan)
				m_inputreaderconn = Glib::signal_io().connect(sigc::mem_fun(this, &StdioClient::input_line_handler),
									      m_inputchan, Glib::IO_IN | Glib::IO_ERR | Glib::IO_HUP);
			return;
		}
		{
			std::string fpl(m_inbuffer, cp);
			m_fplan.swap(fpl);
		}
		while (cp < m_inbuffer + m_inbufptr && (*cp == '\n' || *cp == '\r'))
			++cp;
		m_inbufptr -= cp - m_inbuffer;
		if (m_inbufptr)
			memmove(m_inbuffer, cp, m_inbufptr);
		handle_input();
	}
}

CFMUValidateServer::SocketClient::SocketClient(CFMUValidateServer *server, const Glib::RefPtr<Gio::SocketConnection>& connection)
	: Client(server), m_connection(connection), m_inbufptr(0)
{
	if (m_connection)
		m_inputreaderconn = Glib::signal_io().connect(sigc::mem_fun(*this, &SocketClient::input_line_handler),
							      m_connection->get_socket()->get_fd(),
							      Glib::IO_IN | Glib::IO_ERR | Glib::IO_HUP);
}

CFMUValidateServer::SocketClient::~SocketClient()
{
	m_inputreaderconn.disconnect();
	m_connection.reset();
}

void CFMUValidateServer::SocketClient::set_result(const Results& res)
{
	if (!m_connection) {
		m_inputreaderconn.disconnect();
		Client::ptr_t p(get_ptr());
		if (m_server)
			m_server->close(p);
		return;
	}
	Results res1(res);
	if (m_annotate && m_server && m_validator != validator_local)
		m_server->annotate(res1);
	std::string tx;
	for (Results::const_iterator i(res1.begin()), e(res1.end()); i != e; ++i)
		tx += *i + "\n";
	tx += "\n";
	try {
		m_connection->get_socket()->property_blocking() = true;
		m_connection->get_socket()->send(tx.c_str(), tx.size());
	} catch (const Glib::Error& e) {
		m_inputreaderconn.disconnect();
		m_connection.reset();
		std::cerr << "error sending reply: " << e.what() << std::endl;
	}
	m_fplan.clear();
	handle_input_loop();
}

bool CFMUValidateServer::SocketClient::input_line_handler(Glib::IOCondition iocond)
{
	if (!m_connection) {
		handle_input_loop();
		return false;
        }
	m_connection->get_socket()->property_blocking() = false;
	gssize nrread(m_connection->get_socket()->receive(m_inbuffer + m_inbufptr, sizeof(m_inbuffer) - m_inbufptr));
	if (nrread <= 0) {
		m_inputreaderconn.disconnect();
		m_connection.reset();
		handle_input_loop();
		return false;
        }
	m_inbufptr += nrread;
	handle_input_loop();
	return true;
}

void CFMUValidateServer::SocketClient::handle_input_loop(void)
{
	for (;;) {
		if (!m_connection) {
			m_inputreaderconn.disconnect();
			Client::ptr_t p(get_ptr());
			if (m_server)
				m_server->close(p);
			return;
		}
		if (!m_fplan.empty()) {
			m_inputreaderconn.disconnect();
			return;
		}
		if (false)
			std::cerr << "handle_input_loop: buffer count " << m_inbufptr << std::endl;
		char *cp(static_cast<char *>(memchr(m_inbuffer, '\n', m_inbufptr)));
		{
			char *cp2(static_cast<char *>(memchr(m_inbuffer, '\r', m_inbufptr)));
			if (!cp || (cp2 && cp2 < cp))
				cp = cp2;
		}
		if (!cp) {
			if (m_inbufptr >= sizeof(m_inbuffer))
				m_inbufptr = 0;
			m_inputreaderconn.disconnect();
			if (m_connection)
				m_inputreaderconn = Glib::signal_io().connect(sigc::mem_fun(*this, &SocketClient::input_line_handler),
									      m_connection->get_socket()->get_fd(),
									      Glib::IO_IN | Glib::IO_ERR | Glib::IO_HUP);
			return;
		}
		{
			std::string fpl(m_inbuffer, cp);
			m_fplan.swap(fpl);
		}
		while (cp < m_inbuffer + m_inbufptr && (*cp == '\n' || *cp == '\r'))
			++cp;
		m_inbufptr -= cp - m_inbuffer;
		if (m_inbufptr)
			memmove(m_inbuffer, cp, m_inbufptr);
		handle_input();
	}
}

CFMUValidateServer::SingleFplClient::SingleFplClient(CFMUValidateServer *server)
	: Client(server)
{
        m_outputchan = Glib::IOChannel::create_from_fd(1);
        m_outputchan->set_encoding();
}

CFMUValidateServer::SingleFplClient::~SingleFplClient()
{
	m_outputchan.reset();
}

void CFMUValidateServer::SingleFplClient::handle_input_loop(void)
{
	for (;;) {
		if (!m_fplan.empty())
			return;
		if (m_fplans.empty()) {
			if (!is_closeatend())
				return;
			m_outputchan.reset();
			Client::ptr_t p(get_ptr());
			if (m_server)
				m_server->close(p);
			return;
		}
		m_fplan = *m_fplans.begin();
		m_fplans.erase(m_fplans.begin());
		handle_input();
	}
}

void CFMUValidateServer::SingleFplClient::set_result(const Results& res)
{
	if (!m_outputchan) {
		Client::ptr_t p(get_ptr());
		if (m_server)
			m_server->close(p);
		return;
	}
	Results res1(res);
	if (m_annotate && m_server && m_validator != validator_local)
		m_server->annotate(res1);
	for (Results::const_iterator i(res1.begin()), e(res1.end()); i != e; ++i)
		m_outputchan->write(*i + "\n");
	m_outputchan->write("\n");
	m_outputchan->flush();
	m_fplan.clear();
	handle_input_loop();
}

void CFMUValidateServer::SingleFplClient::set_fplan(const std::string& fpl)
{
	m_fplans.push_back(fpl);
	handle_input_loop();
}

CFMUValidateServer::CFMUValidateServer(int& argc, char**& argv)
	: Gtk::Application(argc, argv), m_engine(0), m_statecnt(0), m_valwidget(0), m_verbose(0),
#ifdef HAVE_X11_XLIB_H
	  m_xdisplay(-1), m_x11srvrun(false),
#endif
#ifdef HAVE_CURL
	  m_curl(0),
#endif
	  m_adrloaded(false), m_quit(true)
{
#ifdef HAVE_CURL
	m_curl = curl_multi_init();
#endif
        bool version(false), local(false), eurofpl(false), b2bop(false), b2bpreop(false);
	Glib::ustring fpl;
	int verbose(0);
        OptionalArgFilename sockaddr;
#ifdef G_OS_UNIX
	uid_t sockservuid(0);
	gid_t sockservgid(0);
	uid_t socketuid(getuid());
	gid_t socketgid(getgid());
	mode_t socketmode(S_IRUSR | S_IWUSR | S_IXUSR | S_IRGRP | S_IWGRP | S_IXGRP);
#endif
	bool noterminate(false);
	// option group
	{
		bool dis_aux(false);
#ifdef G_OS_UNIX
		Glib::ustring strsockservuid;
		Glib::ustring strsockservgid;
		Glib::ustring strsocketuid;
		Glib::ustring strsocketgid;
		Glib::ustring strsocketmode;
#endif
		Glib::OptionGroup options("cfmuvalidateserver", "CFMU Flight Plan Validation Server");
		// version
		Glib::OptionEntry entry_version;
		entry_version.set_long_name(_("version"));
		entry_version.set_description(_("Show version and exit."));
		options.add_entry(entry_version, version);
		// fpl
                Glib::OptionEntry entry_fpl;
                entry_fpl.set_long_name(_("fpl"));
                entry_fpl.set_description(_("Query flight plan given as argument and return."));
                options.add_entry(entry_fpl, fpl);
 		// verbose      
		Glib::OptionEntry entry_verbose;
		entry_verbose.set_long_name(_("verbose"));
		entry_verbose.set_description(_("Set Verbosity."));
		options.add_entry(entry_verbose, verbose);
		// maindir
                Glib::OptionEntry entry_maindir;
                entry_maindir.set_short_name('m');
                entry_maindir.set_long_name("maindir");
                entry_maindir.set_description("Directory containing the main database");
                entry_maindir.set_arg_description("DIR");
                options.add_entry_filename(entry_maindir, m_dir_main);
		// auxdir
                Glib::OptionEntry entry_auxdir;
                entry_auxdir.set_short_name('a');
                entry_auxdir.set_long_name("auxdir");
                entry_auxdir.set_description("Directory containing the auxilliary database");
                entry_auxdir.set_arg_description("DIR");
                options.add_entry_filename(entry_auxdir, m_dir_aux);
		// disableaux
                Glib::OptionEntry entry_disableaux;
                entry_disableaux.set_short_name('A');
                entry_disableaux.set_long_name("disableaux");
                entry_disableaux.set_description("Disable the auxilliary database");
                entry_disableaux.set_arg_description("BOOL");
                options.add_entry(entry_disableaux, dis_aux);
		// local
                Glib::OptionEntry entry_local;
                entry_local.set_short_name('l');
                entry_local.set_long_name("local");
                entry_local.set_description("Local Checker (default CFMU Checks)");
                entry_local.set_arg_description("BOOL");
                options.add_entry(entry_local, local);
		// EuroFPL
                Glib::OptionEntry entry_eurofpl;
                entry_eurofpl.set_short_name('e');
                entry_eurofpl.set_long_name("eurofpl");
                entry_eurofpl.set_description("EuroFPL Checker (default CFMU Checks)");
                entry_eurofpl.set_arg_description("BOOL");
                options.add_entry(entry_eurofpl, eurofpl);
		// B2B Ops
                Glib::OptionEntry entry_b2bop;
		entry_b2bop.set_long_name("b2bop");
                entry_b2bop.set_description("CFMU B2B Ops Checker (default CFMU Checks)");
                entry_b2bop.set_arg_description("BOOL");
                options.add_entry(entry_b2bop, b2bop);
		// B2B Preop
                Glib::OptionEntry entry_b2bpreop;
		entry_b2bpreop.set_long_name("b2bpreop");
                entry_b2bpreop.set_description("CFMU B2B Preop Checker (default CFMU Checks)");
                entry_b2bpreop.set_arg_description("BOOL");
                options.add_entry(entry_b2bpreop, b2bpreop);
#ifdef HAVE_X11_XLIB_H
 		// xdisplay      
		Glib::OptionEntry entry_xdisplay;
		entry_xdisplay.set_long_name(_("xdisplay"));
		entry_xdisplay.set_description(_("Set X Display Server to be spawned."));
                entry_xdisplay.set_arg_description("INT");
		options.add_entry(entry_xdisplay, m_xdisplay);
#endif
		Glib::OptionEntry entry_socketserver;
		entry_socketserver.set_long_name("socketserver");
                entry_socketserver.set_description("Operate as socket server");
		entry_socketserver.set_arg_description("SOCKADDR");
		entry_socketserver.set_flags(entry_socketserver.get_flags() | Glib::OptionEntry::FLAG_OPTIONAL_ARG);
		options.add_entry(entry_socketserver, sigc::mem_fun(&sockaddr, &OptionalArgFilename::on_option_arg_filename));

#ifdef G_OS_UNIX
		Glib::OptionEntry entry_uid;
		entry_uid.set_long_name(_("uid"));
		entry_uid.set_description(_("Set Daemon User ID."));
                entry_uid.set_arg_description("UID");
		options.add_entry(entry_uid, strsockservuid);

		Glib::OptionEntry entry_gid;
		entry_gid.set_long_name(_("gid"));
		entry_gid.set_description(_("Set Daemon Group ID."));
                entry_gid.set_arg_description("GID");
		options.add_entry(entry_gid, strsockservgid);

		Glib::OptionEntry entry_sockuid;
		entry_sockuid.set_long_name(_("sockuid"));
		entry_sockuid.set_description(_("Set Daemon User ID."));
                entry_sockuid.set_arg_description("UID");
		options.add_entry(entry_sockuid, strsocketuid);

		Glib::OptionEntry entry_sockgid;
		entry_sockgid.set_long_name(_("sockgid"));
		entry_sockgid.set_description(_("Set Daemon Group ID."));
                entry_sockgid.set_arg_description("GID");
		options.add_entry(entry_sockgid, strsocketgid);

		Glib::OptionEntry entry_sockmode;
		entry_sockmode.set_long_name(_("sockmode"));
		entry_sockmode.set_description(_("Set Daemon Group ID."));
                entry_sockmode.set_arg_description("MODE");
		options.add_entry(entry_sockmode, strsocketmode);
#endif
#ifdef HAVE_PQXX
		Glib::ustring pg_conn;
		Glib::OptionEntry optpgsql;
		optpgsql.set_short_name('p');
		optpgsql.set_long_name("pgavdb");
		optpgsql.set_description("PostgreSQL Connect String");
		optpgsql.set_arg_description("CONNSTR");
		options.add_entry(optpgsql, pg_conn);
#endif

		Glib::OptionEntry entry_noterminate;
		entry_noterminate.set_long_name(_("noterminate"));
		entry_noterminate.set_description(_("Do not terminate when socket activated."));
                entry_noterminate.set_arg_description("BOOL");
		options.add_entry(entry_noterminate, noterminate);

		// option context
		Glib::OptionContext context(_("-- CFMU Flight Plan Validation Server"));
		//context.add_group(options);
		context.set_main_group(options);
		context.set_help_enabled(true);
		context.set_ignore_unknown_options(true);
		Glib::set_prgname(Glib::path_get_basename(*argv));
		if (!context.parse(argc, argv)) {
			std::cerr << "Command Line Option Error: Usage: " << Glib::get_prgname() << std::endl
				  << context.get_help(false);
			exit(1);
		}
		m_auxdbmode = Engine::auxdb_prefs;
#ifdef HAVE_PQXX
		if (!pg_conn.empty()) {
			m_dir_main = pg_conn;
			m_dir_aux.clear();
			pg_conn.clear();
			m_auxdbmode = Engine::auxdb_postgres;
		} else
#endif
		if (dis_aux)
                        m_auxdbmode = Engine::auxdb_none;
                else if (!m_dir_aux.empty())
                        m_auxdbmode = Engine::auxdb_override;
#ifdef G_OS_UNIX
		if (!strsockservuid.empty()) {
			char *cp;
			uid_t uid1(strtoul(strsockservuid.c_str(), &cp, 0));
			if (*cp) {
				struct passwd *pw = getpwnam(strsockservuid.c_str());
				if (pw) {
					sockservuid = pw->pw_uid;
					sockservgid = pw->pw_gid;
				} else {
					std::cerr << "User " << strsockservuid << " not found" << std::endl;
				}
			} else {
				sockservuid = uid1;
			}
		}
		if (!strsockservgid.empty()) {
			char *cp;
			uid_t gid1(strtoul(strsockservgid.c_str(), &cp, 0));
			if (*cp) {
				struct group *gr = getgrnam(strsockservgid.c_str());
				if (gr) {
					sockservgid = gr->gr_gid;
				} else {
					std::cerr << "Group " << strsockservgid << " not found" << std::endl;
				}
			} else {
				sockservgid = gid1;
			}
		}
		if (!strsocketuid.empty()) {
			char *cp;
			uid_t uid1(strtoul(strsocketuid.c_str(), &cp, 0));
			if (*cp) {
				struct passwd *pw = getpwnam(strsocketuid.c_str());
				if (pw) {
					socketuid = pw->pw_uid;
					socketgid = pw->pw_gid;
				} else {
					std::cerr << "User " << strsocketuid << " not found" << std::endl;
				}
			} else {
				socketuid = uid1;
			}
		}
		if (!strsocketgid.empty()) {
			char *cp;
			uid_t gid1(strtoul(strsocketgid.c_str(), &cp, 0));
			if (*cp) {
				struct group *gr = getgrnam(strsocketgid.c_str());
				if (gr) {
					socketgid = gr->gr_gid;
				} else {
					std::cerr << "Group " << strsocketgid << " not found" << std::endl;
				}
			} else {
				socketgid = gid1;
			}
		}
		if (!strsocketmode.empty()) {
			socketmode = strtoul(strsocketmode.c_str(), 0, 0);
		}
#endif
 	}
        if (version) {
                std::cout << Glib::get_prgname() << (" Version " VERSION) << std::endl;
		exit(0);
	}
#ifdef HAVE_X11_XLIB_H
	if (m_xdisplay >= 0) {
		std::ostringstream disp;
		disp << ':' << m_xdisplay;
		Glib::setenv("DISPLAY", disp.str());
	}
#endif
	m_verbose = std::max(verbose, 0);
        m_mainloop = Glib::MainLoop::create(Glib::MainContext::get_default());
	if (sockaddr.is_set()) {
		m_quit = m_quit && !noterminate;
		std::string path(sockaddr.get_arg());
#ifdef G_OS_UNIX
		if (sockservgid && setgid(sockservgid))
			std::cerr << "Cannot set gid to " << sockservgid << ": "
				  << strerror(errno) << " (" << errno << ')' << std::endl;
		if (sockservuid && setuid(sockservuid))
			std::cerr << "Cannot set uid to " << sockservuid << ": "
				  << strerror(errno) << " (" << errno << ')' << std::endl;
		if (path.empty())
			path = PACKAGE_RUN_DIR "/validator/socket";
	        listen(path, socketuid, socketgid, socketmode);
		/* Cancel certain signals */
		signal(SIGTSTP, SIG_IGN); /* Various TTY signals */
		signal(SIGTTOU, SIG_IGN);
		signal(SIGTTIN, SIG_IGN);
		signal(SIGHUP,  SIG_IGN); /* Ignore hangup signal */
		signal(SIGTERM, signalterminate); /* terminate mainloop on SIGTERM */
		/* Create a new SID for the child process */
		if (getpgrp() != getpid() && setsid() == (pid_t)-1)
			std::cerr << "Cannot create new session: "
				  << strerror(errno) << " (" << errno << ')' << std::endl;
		/* Change the file mode mask */
		umask(0);
		/* Change the current working directory.  This prevents the current
		   directory from being locked; hence not being able to remove it. */
		if (chdir("/tmp") < 0)
			std::cerr << "Cannot change directory to /tmp: "
				  << strerror(errno) << " (" << errno << ')' << std::endl;
#else
		if (path.empty())
			path = "53181";
		listen(path);
#endif
		std::cout << "Starting Validation service on " << path << std::endl;
		return;
	}
 	if (fpl.empty()) {
		StdioClient::ptr_t p(new StdioClient(this));
		if (local)
			p->set_validator(Client::validator_local);
		else if (eurofpl)
			p->set_validator(Client::validator_eurofpl);
		else if (b2bop)
			p->set_validator(Client::validator_cfmub2bop);
		else if (b2bpreop)
			p->set_validator(Client::validator_cfmub2bpreop);
		else
			p->set_validator(Client::validator_cfmu);
		m_clients.insert(p);
	} else {
		SingleFplClient::ptr_t p(new SingleFplClient(this));
		if (local)
			p->set_validator(Client::validator_local);
		else if (eurofpl)
			p->set_validator(Client::validator_eurofpl);
		else if (b2bop)
			p->set_validator(Client::validator_cfmub2bop);
		else if (b2bpreop)
			p->set_validator(Client::validator_cfmub2bpreop);
		else
			p->set_validator(Client::validator_cfmu);
		m_clients.insert(p);
		p->set_closeatend(true);
		p->set_fplan(fpl);
	}
	if (!local && !eurofpl && !b2bop && !b2bpreop)
		start_cfmu();
}

CFMUValidateServer::~CFMUValidateServer()
{
	if (m_listenservice)
		m_listenservice->stop();
	m_clients.clear();
	m_cfmuclients.clear();
	destroy_webview();
	m_listenservice.reset();
	m_listensock.reset();
	remove_listensockaddr();
#ifdef HAVE_X11_XLIB_H
	stop_x11();
#endif
#ifdef HAVE_CURL
	curl_removeio();
	for (curlhandlemap_t::iterator i(m_curlhandlemap.begin()), e(m_curlhandlemap.end()); i != e; ) {
		curlhandlemap_t::iterator i2(i);
		++i;
		if (false)
			std::cerr << "curl handle " << (long long)i2->first << std::endl;
		if (i2->second)
			i2->second->curl_done(CURLE_OPERATION_TIMEDOUT);
	}
	if (m_curl) {
		while (!m_curlhandlemap.empty()) {
			CURL *h(m_curlhandlemap.begin()->first);
			m_curlhandlemap.erase(m_curlhandlemap.begin());
			curl_multi_remove_handle(m_curl, h);
		}
		curl_multi_cleanup(m_curl);
	}
#endif
}

void CFMUValidateServer::run(void)
{
        m_mainloop->run();
}

void CFMUValidateServer::prepare_webview(void)
{
	destroy_webview();
	// initialize Gtk
	{
		std::string prgname(Glib::get_prgname());
		char *argv[3] = { 0, 0, 0 };
		int argc(1);
		argv[0] = const_cast<char *>(prgname.c_str());
#ifdef HAVE_X11_XLIB_H
		std::ostringstream disp;
		disp << "--display=:" << m_xdisplay;
		if (m_xdisplay >= 0)
			argv[argc++] = const_cast<char *>(disp.str().c_str());
#endif
		char **argvp(argv);
		if (false) {
			std::cerr << "gtk args: " << argv[0];
			if (argv[1])
				std::cerr << ' ' << argv[1];
			std::cerr << std::endl;
		}
		gtk_init(&argc, &argvp);
	}
	m_valwidget = new CFMUValidateWidget();
	// add cookie jar
	m_valwidget->set_cookiefile(Glib::build_filename(FPlan::get_userdbpath(), "cookies.txt"));
	// open webview
	m_valwidget->start();
	m_valwidget->show();
}

void CFMUValidateServer::destroy_webview(void)
{
	if (!m_valwidget)
		return;
	delete m_valwidget;
	m_valwidget = 0;
}

void CFMUValidateServer::submit_webview(void)
{
	if (!m_valwidget)
		return;
	while (!m_cfmuclients.empty()) {
		const Client::ptr_t& p(*m_cfmuclients.begin());
		if (!p) {
			m_cfmuclients.erase(m_cfmuclients.begin());
			continue;
		}
		const std::string& fplan(p->get_fplan());
		if (fplan.empty()) {
			m_cfmuclients.erase(m_cfmuclients.begin());
			continue;
		}
		if (!m_valwidget) {
			Results res;
			res.push_back("FAIL: Internal Error (webview)");
			p->set_result(res);
			m_cfmuclients.erase(m_cfmuclients.begin());
			continue;
		}
		m_valwidget->validate(fplan, sigc::mem_fun(*this, &CFMUValidateServer::results_webview));
		break;
	}
}

void CFMUValidateServer::results_webview(const CFMUValidateWidget::validate_result_t& res)
{
	if (res.empty())
		return;
	if (false) {
		std::cerr << "number results: " << res.size() << std::endl;
		for (CFMUValidateWidget::validate_result_t::const_iterator i(res.begin()), e(res.end()); i != e; ++i) {
			std::cerr << "  \"" << *i << "\"";
			for (std::string::const_iterator si(i->begin()), se(i->end()); si != se; ++si)
				std::cerr << ' ' << (int)*si;
			std::cerr << std::endl;
		}
	}
	Results results;
	for (CFMUValidateWidget::validate_result_t::const_iterator i(res.begin()), e(res.end()); i != e; ++i)
		results.push_back(*i);
	if (!m_cfmuclients.empty()) {
		const Client::ptr_t& p(*m_cfmuclients.begin());
		if (p) {
			p->set_result(results);
		}
		m_cfmuclients.erase(m_cfmuclients.begin());
	}
	submit_webview();
}

void CFMUValidateServer::start_cfmu(void)
{
	if (m_valwidget)
		return;
#ifdef HAVE_X11_XLIB_H
	if (m_xdisplay >= 0) {
		m_x11srvrun = false;
		start_x11();
		if (!m_x11srvrun) {
			return;
		}
		m_statecnt = 0;
		m_stateconn.disconnect();
		m_stateconn = Glib::signal_timeout().connect_seconds(sigc::mem_fun(*this, &CFMUValidateServer::poll_xserver), 5);
		return;
	}
#endif
	prepare_webview();
}

#ifdef HAVE_X11_XLIB_H
void CFMUValidateServer::start_x11(void)
{
	if (m_x11srvrun)
		return;
	if (m_xdisplay < 0)
		return;
	try {
		std::vector<std::string> argv, env;
#ifdef __MINGW32__
		std::string workdir;
		{
			gchar *instdir(g_win32_get_package_installation_directory_of_module(0));
			if (instdir) {
				workdir = instdir;
				argv.push_back(Glib::build_filename(instdir, "bin", "Xvfb.exe"));
			} else {
				argv.push_back(Glib::build_filename(PACKAGE_PREFIX_DIR "/bin", "Xvfb.exe"));
				workdir = PACKAGE_PREFIX_DIR;
			}
		}
#else
		std::string workdir("/tmp");
		argv.push_back("/usr/bin/Xvfb");
		env.push_back("PATH=/bin:/usr/bin");
		{
			std::ostringstream disp;
			disp << ':' << m_xdisplay;
			argv.push_back(disp.str());
		}
		argv.push_back("-screen");
		argv.push_back("0");
		argv.push_back("100x100x16");
#endif
		if (false) {
			std::cerr << "Spawn: workdir " << workdir << " argv:" << std::endl;
			for (std::vector<std::string>::const_iterator i(argv.begin()), e(argv.end()); i != e; ++i)
				std::cerr << "  " << *i << std::endl;
			std::cerr << "env:" << std::endl;
			for (std::vector<std::string>::const_iterator i(env.begin()), e(env.end()); i != e; ++i)
				std::cerr << "  " << *i << std::endl;
		}
#ifdef __MINGW32__
		// inherit environment; otherwise, this may fail on WindowsXP with STATUS_SXS_ASSEMBLY_NOT_FOUND (C0150004)
		Glib::spawn_async_with_pipes(workdir, argv,
					     Glib::SPAWN_DO_NOT_REAP_CHILD | Glib::SPAWN_STDERR_TO_DEV_NULL,
					     sigc::slot<void>(), &m_x11srvpid, 0, 0, 0);
#else
		Glib::spawn_async_with_pipes(workdir, argv, env,
					     Glib::SPAWN_DO_NOT_REAP_CHILD | Glib::SPAWN_STDERR_TO_DEV_NULL,
					     sigc::slot<void>(), &m_x11srvpid, 0, 0, 0);
#endif
		m_x11srvrun = true;
	} catch (Glib::SpawnError& e) {
		m_x11srvrun = false;
		std::cerr << "Cannot run X Server: " + e.what() << std::endl;
		return;
	}
	m_x11srvconnwatch.disconnect();
	m_x11srvconnwatch = Glib::signal_child_watch().connect(sigc::mem_fun(this, &CFMUValidateServer::x11srv_child_watch), m_x11srvpid);
}

void CFMUValidateServer::x11srv_child_watch(GPid pid, int child_status)
{
	if (!m_x11srvrun)
		return;
	if (m_x11srvpid != pid) {
		Glib::spawn_close_pid(pid);
		return;
	}
	Glib::spawn_close_pid(pid);
	m_x11srvrun = false;
	m_x11srvconnwatch.disconnect();
}

void CFMUValidateServer::stop_x11(void)
{
	if (!m_x11srvrun)
		return;
	kill(m_x11srvpid, SIGHUP);
}

bool CFMUValidateServer::x11_fail;

int CFMUValidateServer::x11_error(Display *dpy, XErrorEvent *err)
{
	if (true) {
		char msg[80];
		XGetErrorText(dpy, err->error_code, msg, sizeof(msg));
		std::cerr << "X11 Error " << err->error_code << " (" <<  msg << "): request "
			  << err->request_code << '.' << err->minor_code << std::endl;
	}
	x11_fail = true;
	return 0;
}

int CFMUValidateServer::x11_ioerror(Display *dpy)
{
	if (true) {
		std::cerr << "X11 I/O Error" << std::endl;
	}
	x11_fail = true;
	return 0;
}

bool CFMUValidateServer::poll_xserver(void)
{
	{
		x11_fail = false;
		std::ostringstream dpyn;
		dpyn << ':' << m_xdisplay;
		Display *dpy(XOpenDisplay(dpyn.str().c_str()));
		if (!dpy) {
			x11_fail = true;
		} else {
			int (*old_error_handler)(Display *, XErrorEvent *);
			int (*old_ioerror_handler)(Display *);
			old_error_handler = XSetErrorHandler(&CFMUValidateServer::x11_error);
			old_ioerror_handler = XSetIOErrorHandler(&CFMUValidateServer::x11_ioerror);
			int screen(DefaultScreen(dpy));
			Window win(XCreateSimpleWindow(dpy, RootWindow(dpy, screen), 0, 0, 10, 10, 5, BlackPixel(dpy, screen), WhitePixel(dpy, screen)));
			if (!win) {
				x11_fail = true;
			} else {
				XMapWindow(dpy, win);
				XFlush(dpy);
				XDestroyWindow(dpy, win);
			}
			XSetErrorHandler(old_error_handler);
			XSetIOErrorHandler(old_ioerror_handler);
			XCloseDisplay(dpy);
		}
		if (x11_fail) {
			++m_statecnt;
			if (m_statecnt < 60)
				return true;
			return false;
		}
	}
	m_stateconn.disconnect();
	prepare_webview();
	return false;
}
#endif

#ifdef HAVE_CURL
void CFMUValidateServer::curl_add(CURL *handle, const Client::ptr_t& p)
{
	if (!handle || !m_curl)
		return;
	if (m_curlhandlemap.find(handle) != m_curlhandlemap.end())
		return;
	m_curlhandlemap[handle] = p;
	curl_multi_add_handle(m_curl, handle);
	if (false)
		std::cerr << "curl_add handle " << (long long)handle << " map size " << m_curlhandlemap.size() << std::endl;
	curl_io();
}

void CFMUValidateServer::curl_remove(CURL *handle)
{
	if (!handle || !m_curl)
		return;
	curlhandlemap_t::iterator i(m_curlhandlemap.find(handle));
	if (i == m_curlhandlemap.end())
		return;
	m_curlhandlemap.erase(i);
	curl_multi_remove_handle(m_curl, handle);
	if (false)
		std::cerr << "curl_remove handle " << (long long)handle << " map size " << m_curlhandlemap.size() << std::endl;
	curl_io();
}

void CFMUValidateServer::curl_removeio(void)
{
	while (!m_curlio.empty()) {
		m_curlio.begin()->disconnect();
		m_curlio.erase(m_curlio.begin());
	}
}

void CFMUValidateServer::curl_io(void)
{
	curl_removeio();
	if (!m_curl)
		return;
	int running_handles(0);
	while (curl_multi_perform(m_curl, &running_handles) == CURLM_CALL_MULTI_PERFORM);
	int msgs_in_queue;
	while (CURLMsg *msg = curl_multi_info_read(m_curl, &msgs_in_queue)) {
		if (msg->msg != CURLMSG_DONE)
			continue;
		curlhandlemap_t::iterator i(m_curlhandlemap.find(msg->easy_handle));
		if (i == m_curlhandlemap.end())
			continue;
		if (i->second)
			i->second->curl_done(msg->data.result);
	}
	fd_set readset, writeset, exceptset;
	int maxfd(0);
	FD_ZERO(&readset);
	FD_ZERO(&writeset);
	FD_ZERO(&exceptset);
	if (CURLM_OK == curl_multi_fdset(m_curl, &readset, &writeset, &exceptset, &maxfd)) {
		for (int fd = 0; fd <= maxfd; ++fd) {
			Glib::IOCondition cond((Glib::IOCondition)0);
			if (FD_ISSET(fd, &readset))
				cond |= Glib::IO_IN;
			if (FD_ISSET(fd, &writeset))
				cond |= Glib::IO_OUT;
			if (FD_ISSET(fd, &exceptset))
				cond |= Glib::IO_ERR;
			if (cond == (Glib::IOCondition)0)
				continue;
			m_curlio.push_back(Glib::signal_io().connect(sigc::bind_return(sigc::hide(sigc::mem_fun(*this, &CFMUValidateServer::curl_io)), false), fd, cond));
		}
	}
	long timeout;
	if (CURLM_OK == curl_multi_timeout(m_curl, &timeout)) {
		if (timeout > 0) {
			m_curlio.push_back(Glib::signal_timeout().connect(sigc::bind_return(sigc::mem_fun(*this, &CFMUValidateServer::curl_io), false), timeout));
		} else if (!timeout) {
			curl_io();
			return;
		}
	}
}
#endif

void CFMUValidateServer::annotate(Results& res)
{
	if (m_adrloaded) {
		res.annotate(m_reval);
		return;
	}
	Results res1;
	res.swap(res1);
	// first open the databases
	delete m_engine;
	m_engine = new Engine(m_dir_main, m_auxdbmode, m_dir_aux, false, false);
	std::string dir_aux1 = m_engine->get_aux_dir(m_auxdbmode, m_dir_aux);
#ifdef HAVE_PQXX
	if (m_auxdbmode == Engine::auxdb_postgres) {
		m_pgconn.reset(new pqxx::lazyconnection(m_dir_main));
		if (m_pgconn->get_variable("application_name").empty())
			m_pgconn->set_variable("application_name", "cfmuvalidateserver");
		m_airportdb.reset(new AirportsPGDb(*m_pgconn, AirportsPGDb::read_only));
		m_navaiddb.reset(new NavaidsPGDb(*m_pgconn, NavaidsPGDb::read_only));
		m_waypointdb.reset(new WaypointsPGDb(*m_pgconn, WaypointsPGDb::read_only));
		m_airwaydb.reset(new AirwaysPGDb(*m_pgconn, AirwaysPGDb::read_only));
		m_airspacedb.reset(new AirspacesPGDb(*m_pgconn, AirspacesPGDb::read_only));
	} else
#endif
	{
		{
			m_airportdb.reset(new AirportsDb());
			AirportsDb *db(static_cast<AirportsDb *>(m_airportdb.get()));
			db->open_readonly(m_dir_main);
			if (!dir_aux1.empty() && db->is_dbfile_exists(dir_aux1))
				db->attach_readonly(dir_aux1);
			if (false && db->is_aux_attached())
				std::cerr << "Auxillary airports database attached" << std::endl;
		}
		{
			m_navaiddb.reset(new NavaidsDb());
			NavaidsDb *db(static_cast<NavaidsDb *>(m_navaiddb.get()));
			db->open_readonly(m_dir_main);
			if (!dir_aux1.empty() && db->is_dbfile_exists(dir_aux1))
				db->attach_readonly(dir_aux1);
			if (false && db->is_aux_attached())
				std::cerr << "Auxillary navaids database attached" << std::endl;
		}
		{
			m_waypointdb.reset(new WaypointsDb());
			WaypointsDb *db(static_cast<WaypointsDb *>(m_waypointdb.get()));
			db->open_readonly(m_dir_main);
			if (!dir_aux1.empty() && db->is_dbfile_exists(dir_aux1))
				db->attach_readonly(dir_aux1);
			if (false && db->is_aux_attached())
				std::cerr << "Auxillary waypoints database attached" << std::endl;
		}
		{
			m_airwaydb.reset(new AirwaysDb());
			AirwaysDb *db(static_cast<AirwaysDb *>(m_airwaydb.get()));
			db->open_readonly(m_dir_main);
			if (!dir_aux1.empty() && db->is_dbfile_exists(dir_aux1))
				db->attach_readonly(dir_aux1);
			if (false && db->is_aux_attached())
				std::cerr << "Auxillary airways database attached" << std::endl;
		}
		{
			m_airspacedb.reset(new AirspacesDb());
			AirspacesDb *db(static_cast<AirspacesDb *>(m_airspacedb.get()));
			db->open_readonly(m_dir_main);
			if (!dir_aux1.empty() && db->is_dbfile_exists(dir_aux1))
				db->attach_readonly(dir_aux1);
			if (false && db->is_aux_attached())
				std::cerr << "Auxillary airspaces database attached" << std::endl;
		}
	}
	// first open the databases
	m_adrdb.set_path(m_dir_aux.empty() ? Engine::get_default_aux_dir() : m_dir_aux);
	if (!m_topodb.is_open())
		m_topodb.open(m_dir_aux.empty() ? Engine::get_default_aux_dir() : m_dir_aux);
	m_reval.set_db(&m_adrdb);
	Glib::TimeVal tv;
	tv.assign_current_time();
	m_reval.load_rules();
	{
		Glib::TimeVal tv1;
		tv1.assign_current_time();
		tv = tv1 - tv;
	}
	std::ostringstream oss;
	oss << "Loaded " << m_reval.count_rules() << " Restrictions, "
	    << std::fixed << std::setprecision(3) << tv.as_double() << 's';
	res.push_back(oss.str());
    	m_adrloaded = true;
	res1.annotate(m_reval);
	res.insert(res.end(), res1.begin(), res1.end());
}

void CFMUValidateServer::validate_locally(const Client::ptr_t& p)
{
	if (!p)
		return;
	Results res1;
	annotate(res1);
	// parse flight plan
	ADR::FlightPlan adrfpl;
	ADR::FlightPlan::errors_t err(adrfpl.parse(m_adrdb, p->get_fplan(), true));
	if (!err.empty()) {
		std::string er("EFPL: ");
		bool subseq(false);
		for (ADR::FlightPlan::errors_t::const_iterator i(err.begin()), e(err.end()); i != e; ++i) {
			if (subseq)
				er += "; ";
			subseq = true;
			er += *i;
		}
		res1.push_back(er);
		p->set_result(res1);
		return;
	}
	adrfpl.fix_invalid_altitudes(m_topodb, m_adrdb);
	m_reval.set_fplan(adrfpl);
	if (true) {
		std::ostringstream oss;
		oss << "I: FPL Parsed, " << adrfpl.size() << " waypoints";
		res1.push_back(oss.str());
	}
	// print Parsed Flight Plan
	if (false) {
		for (unsigned int i(0), n(adrfpl.size()); i < n; ++i) {
			const ADR::FPLWaypoint& wpt(adrfpl[i]);
			std::ostringstream oss;
			oss << "PFPL[" << i << "]: " << wpt.get_icao() << '/' << wpt.get_name() << ' ';
			switch (wpt.get_pathcode()) {
			case FPlanWaypoint::pathcode_sid:
			case FPlanWaypoint::pathcode_star:
			case FPlanWaypoint::pathcode_airway:
				oss << wpt.get_pathname();
				break;

			case FPlanWaypoint::pathcode_directto:
				oss << "DCT";
				break;

			default:
				oss << '-';
				break;
			}
			oss << ' ' << wpt.get_coord().get_lat_str2() << ' ' << wpt.get_coord().get_lon_str2()
			    << " D" << std::fixed << std::setprecision(1) << wpt.get_dist_nmi()
			    << " T" << std::fixed << std::setprecision(0) << wpt.get_truetrack_deg()
			    << ' ' << wpt.get_fpl_altstr() << ' ' << (wpt.is_ifr() ? 'I' : 'V') << "FR";
			if (wpt.get_ptobj())
				oss << " ptobj " << wpt.get_ptobj()->get_uuid();
			if (wpt.get_pathobj())
				oss << " pathobj " << wpt.get_pathobj()->get_uuid();
			if (wpt.is_expanded())
				oss << " (E)";
			res1.push_back(oss.str());
		}
	}
	bool res(m_reval.check_fplan(false /*m_honourprofilerules*/));
	// print Flight Plan
	{
		ADR::FlightPlan f(m_reval.get_fplan());
		{
			std::ostringstream oss;
			oss << "FR: " << f.get_aircrafttype() << ' ' << Aircraft::get_aircrafttypeclass(f.get_aircrafttype()) << ' '
			    << f.get_equipment() << " PBN/" << f.get_pbn_string() << ' ' << f.get_flighttype();
  			res1.push_back(oss.str());
		}
		for (unsigned int i(0), n(f.size()); i < n; ++i) {
			const ADR::FPLWaypoint& wpt(f[i]);
			std::ostringstream oss;
			oss << "F[" << i << '/' << wpt.get_wptnr() << "]: " << wpt.get_ident() << ' ' << wpt.get_type_string() << ' ';
			switch (wpt.get_pathcode()) {
			case FPlanWaypoint::pathcode_sid:
			case FPlanWaypoint::pathcode_star:
			case FPlanWaypoint::pathcode_airway:
				oss << wpt.get_pathname();
				break;

			case FPlanWaypoint::pathcode_directto:
				oss << "DCT";
				break;

			default:
				oss << '-';
				break;
			}
			oss << ' ' << wpt.get_coord().get_lat_str2() << ' ' << wpt.get_coord().get_lon_str2()
			    << " D" << std::fixed << std::setprecision(1) << wpt.get_dist_nmi()
			    << " T" << std::fixed << std::setprecision(0) << wpt.get_truetrack_deg()
			    << ' ' << wpt.get_fpl_altstr()
			    << ' ' << wpt.get_icao() << '/' << wpt.get_name() << ' ' << (wpt.is_ifr() ? 'I' : 'V') << "FR";
			{
				time_t t(wpt.get_time_unix());
				struct tm tm;
				if (gmtime_r(&t, &tm)) {
					oss << ' ' << std::setw(2) << std::setfill('0') << tm.tm_hour
					    << ':' << std::setw(2) << std::setfill('0') << tm.tm_min;
					if (t >= 48 * 60 * 60)
						oss << ' ' << std::setw(4) << std::setfill('0') << (tm.tm_year + 1900)
						    << '-' << std::setw(2) << std::setfill('0') << (tm.tm_mon + 1)
						    << '-' << std::setw(2) << std::setfill('0') << tm.tm_mday;
				}
			}
			if (wpt.get_ptobj())
				oss << " ptobj " << wpt.get_ptobj()->get_uuid();
			if (wpt.get_pathobj())
				oss << " pathobj " << wpt.get_pathobj()->get_uuid();
			if (wpt.is_expanded())
				oss << " (E)";
  			res1.push_back(oss.str());
		}
	}
	for (ADR::RestrictionEval::messages_const_iterator mi(m_reval.messages_begin()), me(m_reval.messages_end()); mi != me; ++mi)
		res1.print(*mi);
        res1.push_back(res ? "OK" : "FAIL");
	for (ADR::RestrictionEval::results_const_iterator ri(m_reval.results_begin()), re(m_reval.results_end()); ri != re; ++ri)
		res1.print(*ri, m_reval.get_departuretime());
	p->set_result(res1);
}

void CFMUValidateServer::validate_cfmu(const Client::ptr_t& p)
{
	if (!p)
		return;
	if (p->get_fplan().empty())
		return;
	if (m_verbose >= 1U)
		std::cerr << "Queue fpl " << p->get_fplan() << std::endl;
	m_cfmuclients.push_back(p);
	start_cfmu();
	submit_webview();
}

void CFMUValidateServer::close(const Client::ptr_t& p)
{
	m_clients.erase(p);
	if (!m_listenservice && m_clients.empty())
		m_mainloop->quit();
}

void CFMUValidateServer::logmessage(unsigned int loglevel, const Glib::ustring& msg)
{
	if (m_verbose < loglevel)
		return;
	std::cerr << msg << std::endl;
}

void CFMUValidateServer::remove_listensockaddr(void)
{
#ifdef G_OS_UNIX
	if (!m_listensockaddr)
		return;
	if (m_listensockaddr->property_address_type() == Gio::UNIX_SOCKET_ADDRESS_PATH) {
		Glib::RefPtr<Gio::File> f(Gio::File::create_for_path(m_listensockaddr->property_path()));
		try {
			f->remove();
		} catch (...) {
		}
	}
	m_listensockaddr.reset();
#endif
}

#ifdef G_OS_UNIX
void CFMUValidateServer::listen(const std::string& path, uid_t socketuid, gid_t socketgid, mode_t socketmode)
#else
void CFMUValidateServer::listen(const std::string& path)
#endif
{
	m_listenservice.reset();
	m_listensock.reset();
	remove_listensockaddr();
#ifdef HAVE_SYSTEMDDAEMON
	{
		int n = sd_listen_fds(0);
		if (n > 1)
			throw std::runtime_error("SystemD: too many file descriptors received");
		if (n == 1) {
			int fd = SD_LISTEN_FDS_START + 0;
			m_listensock = Gio::Socket::create_from_fd(fd);
		}
	}
#endif
#ifdef G_OS_UNIX
	if (!m_listensock) {
		m_listensock = Gio::Socket::create(Gio::SOCKET_FAMILY_UNIX, Gio::SOCKET_TYPE_STREAM, Gio::SOCKET_PROTOCOL_DEFAULT);
	        m_listensockaddr = Gio::UnixSocketAddress::create(path, Gio::UNIX_SOCKET_ADDRESS_PATH);
		if (true) {
			Glib::RefPtr<Gio::File> f(Gio::File::create_for_path(path));
			try {
				f->remove();
			} catch (...) {
			}
		}
		m_listensock->bind(m_listensockaddr, true);
		m_listensock->listen();
		//if (fchown(m_listensock->get_fd(), socketuid, socketgid))
		if (chown(path.c_str(), socketuid, socketgid))
			std::cerr << "Cannot set socket " << path << " uid/gid to " << socketuid << '/' << socketgid
				  << ": " << strerror(errno) << " (" << errno << ')' << std::endl;
		//if (fchmod(m_listensock->get_fd(), socketmode))
		if (chmod(path.c_str(), socketmode))
			std::cerr << "Cannot set socket " << path << " mode to " << std::oct << '0'
				  << (unsigned int)socketmode << std::dec << ": " << strerror(errno) << " (" << errno << ')' << std::endl;
		m_quit = false;
	}
#else
	if (!m_listensock) {
		Glib::RefPtr<Gio::InetAddress> addr(Gio::InetAddress::create_loopback(Gio::SOCKET_FAMILY_IPV4));
		Glib::RefPtr<Gio::InetSocketAddress> sockaddr(Gio::InetSocketAddress::create(addr, strtoul(path.c_str(), 0, 0)));
		m_listensock = Gio::Socket::create(Gio::SOCKET_FAMILY_IPV4, Gio::SOCKET_TYPE_STREAM, Gio::SOCKET_PROTOCOL_DEFAULT);
		m_listensock->bind(sockaddr, true);
		m_listensock->listen();
		m_quit = false;
	}
#endif
	m_listenservice = Gio::SocketService::create();
	m_listenservice->signal_incoming().connect(sigc::mem_fun(*this, &CFMUValidateServer::on_connect));
	if (!m_listenservice->add_socket(m_listensock))
		throw std::runtime_error("Cannot listen to socket");
	m_listenservice->start();
}

bool CFMUValidateServer::on_connect(const Glib::RefPtr<Gio::SocketConnection>& connection, const Glib::RefPtr<Glib::Object>& source_object)
{
	Client::ptr_t p(new SocketClient(this, connection));
	m_clients.insert(p);
	return true;
}

int main(int argc, char *argv[])
{
	try {
#if !GLIB_CHECK_VERSION(2,35,0)
		g_type_init();
#endif
		Glib::init();
		Gio::init();
		Glib::set_application_name("CFMU Validate Server");
		Glib::thread_init();
		CFMUValidateServer app(argc, argv);
		mainloop = app.get_mainloop();
		app.run();
		return EX_OK;
	} catch (const Glib::FileError& ex) {
		std::cerr << "FileError: " << ex.what() << std::endl;
		return EX_DATAERR;
	} catch (const Gtk::BuilderError& ex) {
		std::cerr << "BuilderError: " << ex.what() << std::endl;
		return EX_DATAERR;
	} catch (const std::exception& ex) {
		std::cerr << "Error: " << ex.what() << std::endl;
		return EX_DATAERR;
	} catch (const Glib::Exception& ex) {
		std::cerr << "Glib Error: " << ex.what() << std::endl;
		return EX_DATAERR;
	}
	return EX_DATAERR;
}
