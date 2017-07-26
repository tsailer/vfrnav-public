//
// C++ Implementation: xmlformat
//
// Description: Simple XML Formatter
//
//
// Author: Thomas Sailer <t.sailer@alumni.ethz.ch>, (C) 2007, 2009, 2012, 2013
//
// Copyright: See COPYING file that comes with this distribution
//
//

#include "sysdeps.h"

#include <libxml++/libxml++.h>
#include <unistd.h>
#include <getopt.h>
#include <iostream>
#include <sstream>
#include <stdlib.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <errno.h>
#include <stdexcept>
#include <stdarg.h>
#include <libxml/encoding.h>
#include <libxml/xmlwriter.h>
#include <libxml/parser.h>
#include <libxml/tree.h>

#include "tfr.hh"

class DisableRule {
public:
	DisableRule(const std::string& rule = "", bool rx = false) : m_rule(rule), m_rx(rx) {}
	const std::string& get_rule(void) const { return m_rule; }
	bool is_regex(void) const { return m_rx; }

protected:
	std::string m_rule;
	bool m_rx;
};

int main(int argc, char *argv[])
{
        static struct option long_options[] = {
		{ "maindir", required_argument, 0, 'm' },
		{ "auxdir", required_argument, 0, 'a' },
		{ "disableaux", no_argument, 0, 'A' },
		{ "binaryout", no_argument, 0, 'b' },
		{ "binaryinout", no_argument, 0, 'B' },
 		{ "save-disabled", required_argument, 0, 'd' },
 		{ "disable", required_argument, 0, 'D' },
 		{ "disable-regex", required_argument, 0, 'R' },
		{0, 0, 0, 0}
        };
	typedef enum {
		mode_format,
		mode_binout,
		mode_binio
	} mode_t;
	mode_t mode(mode_format);
	Glib::ustring dir_main(""), dir_aux(""), save_disabled("");
	Engine::auxdb_mode_t auxdbmode(Engine::auxdb_prefs);
	bool dis_aux(false);
	typedef std::vector<DisableRule> disablerule_t;
	disablerule_t disablerule;
        int c, err(0);

        while ((c = getopt_long(argc, argv, "m:a:AbBd:D:R:", long_options, 0)) != EOF) {
                switch (c) {
		case 'm':
			if (optarg)
				dir_main = optarg;
			break;

		case 'a':
			if (optarg)
				dir_aux = optarg;
			break;

		case 'A':
			dis_aux = true;
			break;

		case 'b':
			mode = mode_binout;
			break;

		case 'B':
			mode = mode_binio;
			break;

		case 'd':
			if (optarg)
				save_disabled = optarg;
			break;

		case 'D':
		case 'R':
			if (optarg)
				disablerule.push_back(DisableRule(optarg, c == 'R'));
			break;

 		default:
			err++;
			break;
                }
        }
        if (err || optind + 2 > argc) {
                std::cerr << "usage: vfrradconvert [-b] [-B] <input> <output>" << std::endl;
                return EX_USAGE;
        }
	if (dis_aux)
		auxdbmode = Engine::auxdb_none;
	else if (!dir_aux.empty())
		auxdbmode = Engine::auxdb_override;
 	switch (mode) {
	case mode_format:
	{
		if (false) {
			xmlpp::DomParser p(argv[optind]);
			if (!p)
				return EX_DATAERR;
			xmlpp::Document *doc(p.get_document());
			doc->write_to_file_formatted(argv[optind+1]);
			return EX_OK;
		}
		xmlDocPtr doc(xmlReadFile(argv[optind], NULL, XML_PARSE_NOBLANKS));
		if (doc == NULL)
			return EX_DATAERR;
		xmlKeepBlanksDefault(0);
		xmlIndentTreeOutput = 1;
		err = xmlSaveFormatFile(argv[optind+1], doc, 1);
		xmlFreeDoc(doc);
		return err == -1 ? EX_DATAERR : EX_OK;
	}

	case mode_binout:
	case mode_binio:
	{
		Engine eng(dir_main, auxdbmode, dir_aux, false);
                dir_aux = eng.get_aux_dir(auxdbmode, dir_aux);
		AirportsDb airportdb;
		NavaidsDb navaiddb;
		WaypointsDb waypointdb;
		AirwaysDb airwaydb;
		AirspacesDb airspacedb;
                airportdb.open(dir_main);
                if (!dir_aux.empty() && airportdb.is_dbfile_exists(dir_aux))
                        airportdb.attach(dir_aux);
                if (airportdb.is_aux_attached())
                        std::cerr << "Auxillary airports database attached" << std::endl;
                navaiddb.open(dir_main);
                if (!dir_aux.empty() && navaiddb.is_dbfile_exists(dir_aux))
                        navaiddb.attach(dir_aux);
                if (navaiddb.is_aux_attached())
                        std::cerr << "Auxillary navaids database attached" << std::endl;
                waypointdb.open(dir_main);
                if (!dir_aux.empty() && waypointdb.is_dbfile_exists(dir_aux))
                        waypointdb.attach(dir_aux);
                if (waypointdb.is_aux_attached())
                        std::cerr << "Auxillary waypoints database attached" << std::endl;
                airwaydb.open(dir_main);
                if (!dir_aux.empty() && airwaydb.is_dbfile_exists(dir_aux))
                        airwaydb.attach(dir_aux);
                if (airwaydb.is_aux_attached())
                        std::cerr << "Auxillary airways database attached" << std::endl;
                airspacedb.open(dir_main);
                if (!dir_aux.empty() && airspacedb.is_dbfile_exists(dir_aux))
                        airspacedb.attach(dir_aux);
                if (airspacedb.is_aux_attached())
                        std::cerr << "Auxillary airspaces database attached" << std::endl;
		TrafficFlowRestrictions tfr;
		std::vector<TrafficFlowRestrictions::Message> msg;
		bool ld;
		if (mode == mode_binio)
			ld = tfr.load_binary_rules(msg, argv[optind], airportdb, navaiddb, waypointdb, airwaydb, airspacedb);
		else
			ld = tfr.load_rules(msg, argv[optind], airportdb, navaiddb, waypointdb, airwaydb, airspacedb);
		for (unsigned int i = 0; i < msg.size(); ++i) {
			const TrafficFlowRestrictions::Message& m(msg[i]);
			std::cout << m.get_type_char() << ':';
			if (!m.get_rule().empty())
				std::cout << " R:" << m.get_rule();
			std::cout << ' ' << m.get_text() << std::endl;
		}
		if (!ld) {
			std::cerr << "Cannot load Traffic Flow Restrictions file " << argv[optind] << std::endl;
			return EX_UNAVAILABLE;
		}
		if (true) {
			std::cerr << "Origin:    " << tfr.get_origin() << std::endl
				  << "Created:   " << tfr.get_created() << std::endl
				  << "Effective: " << tfr.get_effective() << std::endl;
			tfr.reset_rules();
			tfr.prepare_dct_rules();
			std::cerr << tfr.count_rules() << " rules, " << tfr.count_dct_rules() << " DCT rules" << std::endl;
		}
		{
			typedef std::vector<TrafficFlowRestrictions::RuleResult> rr_t;
			rr_t r(tfr.find_rules("", false));
			for (disablerule_t::const_iterator di(disablerule.begin()), de(disablerule.end()); di != de; ++di) {
				if (!di->is_regex()) {
					for (rr_t::const_iterator ri(r.begin()), re(r.end()); ri != re; ++ri) {
						if (ri->get_rule() != di->get_rule())
							continue;
						tfr.disabledrules_add(ri->get_rule());
						break;
					}
					continue;
				}
				Glib::RefPtr<Glib::Regex> rx(Glib::Regex::create(di->get_rule()));
				for (rr_t::const_iterator ri(r.begin()), re(r.end()); ri != re; ++ri) {
					Glib::MatchInfo minfo;
					if (!rx->match(ri->get_rule(), minfo))
						continue;
					tfr.disabledrules_add(ri->get_rule());
				}
			}
		}
		bool sv(tfr.save_binary_rules(argv[optind+1]));
		if (!sv)
			std::cerr << "Cannot save Traffic Flow Restrictions file " << argv[optind+1] << std::endl;
		if (!save_disabled.empty())
			tfr.save_disabled_trace_rules_file(save_disabled);
		return sv ? EX_OK : EX_DATAERR;
	}

	default:
		return EX_USAGE;
        }
}
