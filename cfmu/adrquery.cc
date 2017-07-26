//
// C++ Implementation: adrquery
//
// Description: AIXM Database query utility
//
//
// Author: Thomas Sailer <t.sailer@alumni.ethz.ch>, (C) 2007, 2009, 2012, 2013, 2014
//
// Copyright: See COPYING file that comes with this distribution
//
//

#include "sysdeps.h"

#include <sstream>
#include <iostream>
#include <iomanip>

#include "adr.hh"
#include "adrparse.hh"
#include "adrhibernate.hh"

#include "getopt.h"

static void print_aup(ADR::Database& db, const ADR::AUPDatabase::findresults_t& r, bool blob)
{
	for (ADR::AUPDatabase::findresults_t::const_iterator ri(r.begin()), re(r.end()); ri != re; ++ri) {
		if (!*ri)
			continue;
		ADR::ADRAUPObjectBase::ptr_t::cast_const(*ri)->link(db);
		(*ri)->print(std::cout);
		if (!blob)
			continue;
		std::ostringstream blob;
		{
			ADR::ArchiveWriteStream ar(blob);
			(*ri)->save(ar);
		}
		std::ostringstream oss;
		oss << std::hex;
		for (const uint8_t *p0(reinterpret_cast<const uint8_t *>(blob.str().c_str())),
			     *p1(reinterpret_cast<const uint8_t *>(blob.str().c_str()) + blob.str().size()); p0 != p1; ++p0)
			oss << ' ' << std::setfill('0') << std::setw(2) << (unsigned int)*p0;
		std::cout << "  blob: " << oss.str() << std::endl;
	}
}

int main(int argc, char *argv[])
{
        static struct option long_options[] = {
		{ "dir", required_argument, 0, 'd' },
		{ "dct", no_argument, 0, 'D' },
		{ "aup", optional_argument, 0, 'A' },
		{ "uuid", no_argument, 0, 'u' },
		{ "ident", no_argument, 0, 'i' },
		{ "area", no_argument, 0, 'a' },
		{ "point", no_argument, 0, 'p' },
		{ "dependencies", no_argument, 0, 0x410 },
		{ "dependson", no_argument, 0, 0x411 },
		{ "blob", no_argument, 0, 'b' },
		{ "disable-binfile", no_argument, 0, 'B' },
		{ "time-min", required_argument, 0, 0x400 },
		{ "time-max", required_argument, 0, 0x401 },
		{ "type-min", required_argument, 0, 0x402 },
		{ "type-max", required_argument, 0, 0x403 },
		{ "time", required_argument, 0, 0x404 },
		{ "type", required_argument, 0, 0x405 },
		{ "limit", required_argument, 0, 'l' },
		{ "startswith", no_argument, 0, 0x500 + ADR::Database::comp_startswith },
		{ "contains", no_argument, 0, 0x500 + ADR::Database::comp_contains },
		{ "exact", no_argument, 0, 0x500 + ADR::Database::comp_exact },
		{ "exactcasesensitive", no_argument, 0, 0x500 + ADR::Database::comp_exact_casesensitive },
		{ "like", no_argument, 0, 0x500 + ADR::Database::comp_like },
		{ "airportcollocation", no_argument, 0, 0x600 + ADR::Object::type_airportcollocation },
		{ "organisationauthority", no_argument, 0, 0x600 + ADR::Object::type_organisationauthority },
		{ "airtrafficmanagementservice", no_argument, 0, 0x600 + ADR::Object::type_airtrafficmanagementservice },
		{ "unit", no_argument, 0, 0x600 + ADR::Object::type_unit },
		{ "specialdate", no_argument, 0, 0x600 + ADR::Object::type_specialdate },
		{ "angleindication", no_argument, 0, 0x600 + ADR::Object::type_angleindication },
		{ "distanceindication", no_argument, 0, 0x600 + ADR::Object::type_distanceindication },
		{ "sid", no_argument, 0, 0x600 + ADR::Object::type_sid },
		{ "star", no_argument, 0, 0x600 + ADR::Object::type_star },
		{ "route", no_argument, 0, 0x600 + ADR::Object::type_route },
		{ "standardlevelcolumn", no_argument, 0, 0x600 + ADR::Object::type_standardlevelcolumn },
		{ "standardleveltable", no_argument, 0, 0x600 + ADR::Object::type_standardleveltable },
		{ "flightrestriction", no_argument, 0, 0x600 + ADR::Object::type_flightrestriction },
		{ "airport", no_argument, 0, 0x600 + ADR::Object::type_airport },
		{ "navaid", no_argument, 0, 0x600 + ADR::Object::type_navaid },
		{ "designatedpoint", no_argument, 0, 0x600 + ADR::Object::type_designatedpoint },
		{ "departureleg", no_argument, 0, 0x600 + ADR::Object::type_departureleg },
		{ "arrivalleg", no_argument, 0, 0x600 + ADR::Object::type_arrivalleg },
		{ "routesegment", no_argument, 0, 0x600 + ADR::Object::type_routesegment },
		{ "airspace", no_argument, 0, 0x600 + ADR::Object::type_airspace },
		{ "simplify", no_argument, 0, 0x700 },
		{ "simplify-bbox", required_argument, 0, 0x701 },
		{ "simplify-altrange", required_argument, 0, 0x702 },
		{ "simplify-aircraft", required_argument, 0, 0x703 },
		{ "simplify-equipment", required_argument, 0, 0x704 },
		{ "simplify-ga", no_argument, 0, 0x705 },
		{ "simplify-civ", no_argument, 0, 0x706 },
		{ "simplify-mil", no_argument, 0, 0x707 },
		{ "simplify-dep", required_argument, 0, 0x708 },
		{ "simplify-dest", required_argument, 0, 0x709 },
		{ "simplify-complexity", no_argument, 0, 0x70a },
		{ "simplify-time", required_argument, 0, 0x70b },
		{ "trace-airspace", required_argument, 0, 0x800 },
		{ "trace-airspace-alt", required_argument, 0, 0x801 },
		{ "trace-airspace-time", required_argument, 0, 0x801 },
		{0, 0, 0, 0}
        };
        Glib::ustring db_dir(""), aupdb_dir("");
	typedef enum {
		mode_uuid,
		mode_ident,
		mode_area,
		mode_pointdist,
		mode_dependencies,
		mode_dependson
	} mode_t;
	mode_t mode(mode_uuid);
	ADR::Database::comp_t comp(ADR::Database::comp_contains);
	unsigned int limit(0);
	uint64_t timemin(0);
	uint64_t timemax(std::numeric_limits<uint64_t>::max());
	ADR::Object::type_t typemin(ADR::Object::type_first);
	ADR::Object::type_t typemax(ADR::Object::type_last);
	bool blob(false), binfile(true), dct(false), aup(false);
	bool simpl(false), simplcplx(false), simplcivmil(false), simplmil(false);
	Rect simplbbox(Point::invalid, Point::invalid);
	std::pair<int32_t,int32_t> simplalt(-1, -1);
	std::string simplacft;
	std::string simplequipment;
	Aircraft::pbn_t simplpbn(Aircraft::pbn_none);
	char simplflttype(0);
	ADR::Link simpldep(ADR::UUID::niluuid);
	ADR::Link simpldest(ADR::UUID::niluuid);
	ADR::timepair_t simpltime(0, 0);
	ADR::Link traceaspcpt0(ADR::UUID::niluuid);
	ADR::Link traceaspcpt1(ADR::UUID::niluuid);
	int32_t traceaspcalt(ADR::AltRange::altignore);
	ADR::timetype_t traceaspctime;
	{
		Glib::TimeVal tv;
		tv.assign_current_time();
		traceaspctime = tv.tv_sec;
	}
	int c, err(0);

        while ((c = getopt_long(argc, argv, "d:DA::uiapl:bB", long_options, 0)) != EOF) {
                switch (c) {
		case 'd':
			if (optarg)
				db_dir = optarg;
			break;

		case 'D':
			dct = true;
			break;

		case 'A':
			aup = true;
			if (optarg)
				aupdb_dir = optarg;
			break;

		case 'u':
			mode = mode_uuid;
			break;

		case 'i':
			mode = mode_ident;
			break;

		case 'a':
			mode = mode_area;
			break;

		case 'p':
			mode = mode_pointdist;
			break;

		case 0x410:
			mode = mode_dependencies;
			break;

		case 0x411:
			mode = mode_dependson;
			break;

		case 'b':
			blob = true;
			break;

		case 'B':
			binfile = false;
			break;

		case 'l':
			if (optarg)
				limit = strtoul(optarg, 0, 0);
			break;

		case 0x400:
			if (optarg) {
				Glib::TimeVal tv;
				if (tv.assign_from_iso8601(optarg))
					timemin = tv.tv_sec;
				else
					std::cerr << "Cannot parse time " << optarg << std::endl;
			}
			break;

		case 0x401:
			if (optarg) {
				Glib::TimeVal tv;
				if (tv.assign_from_iso8601(optarg))
					timemax = tv.tv_sec;
				else
					std::cerr << "Cannot parse time " << optarg << std::endl;
			}
			break;

		case 0x404:
			if (optarg) {
				Glib::TimeVal tv;
				if (tv.assign_from_iso8601(optarg))
					timemin = timemax = tv.tv_sec;
				else
					std::cerr << "Cannot parse time " << optarg << std::endl;
			}
			break;

		case 0x402:
			if (optarg) {
				if (std::isdigit(optarg[0])) {
					typemin = (ADR::Object::type_t)strtoul(optarg, 0, 0);
					break;
				}
				ADR::Object::type_t t = ADR::Object::type_first;
				for (; t < ADR::Object::type_last; t = (ADR::Object::type_t)(t + 1))
					if (to_str(t) == optarg)
						break;
				if (t >= ADR::Object::type_last)
					std::cerr << "Cannot parse type " << optarg << std::endl;
				else
					typemin = t;
			}
			break;

		case 0x403:
			if (optarg) {
				if (std::isdigit(optarg[0])) {
					typemax = (ADR::Object::type_t)strtoul(optarg, 0, 0);
					break;
				}
				ADR::Object::type_t t = ADR::Object::type_first;
				for (; t < ADR::Object::type_last; t = (ADR::Object::type_t)(t + 1))
					if (to_str(t) == optarg)
						break;
				if (t >= ADR::Object::type_last)
					std::cerr << "Cannot parse type " << optarg << std::endl;
				else
					typemax = t;
			}
			break;

		case 0x405:
			if (optarg) {
				if (std::isdigit(optarg[0])) {
					typemin = typemax = (ADR::Object::type_t)strtoul(optarg, 0, 0);
					if (typemin == ADR::Object::type_airport)
						typemax = ADR::Object::type_airportend;
					break;
				}
				ADR::Object::type_t t = ADR::Object::type_first;
				for (; t < ADR::Object::type_last; t = (ADR::Object::type_t)(t + 1))
					if (to_str(t) == optarg)
						break;
				if (t >= ADR::Object::type_last) {
					std::cerr << "Cannot parse type " << optarg << std::endl;
				} else {
					typemin = typemax = t;
					if (typemin == ADR::Object::type_airport)
						typemax = ADR::Object::type_airportend;
				}
			}
			break;

		case 0x500 + ADR::Database::comp_startswith:
		case 0x500 + ADR::Database::comp_contains:
		case 0x500 + ADR::Database::comp_exact:
		case 0x500 + ADR::Database::comp_exact_casesensitive:
		case 0x500 + ADR::Database::comp_like:
			comp = (ADR::Database::comp_t)(c - 0x500);
			break;

		case 0x600 + ADR::Object::type_airportcollocation:
		case 0x600 + ADR::Object::type_organisationauthority:
		case 0x600 + ADR::Object::type_airtrafficmanagementservice:
		case 0x600 + ADR::Object::type_unit:
		case 0x600 + ADR::Object::type_specialdate:
		case 0x600 + ADR::Object::type_angleindication:
		case 0x600 + ADR::Object::type_distanceindication:
		case 0x600 + ADR::Object::type_sid:
		case 0x600 + ADR::Object::type_star:
		case 0x600 + ADR::Object::type_route:
		case 0x600 + ADR::Object::type_standardlevelcolumn:
		case 0x600 + ADR::Object::type_standardleveltable:
		case 0x600 + ADR::Object::type_flightrestriction:
		case 0x600 + ADR::Object::type_airport:
		case 0x600 + ADR::Object::type_navaid:
		case 0x600 + ADR::Object::type_designatedpoint:
		case 0x600 + ADR::Object::type_departureleg:
		case 0x600 + ADR::Object::type_arrivalleg:
		case 0x600 + ADR::Object::type_routesegment:
		case 0x600 + ADR::Object::type_airspace:
			typemin = typemax = (ADR::Object::type_t)(c - 0x600);
			if (typemin == ADR::Object::type_airport)
				typemax = ADR::Object::type_airportend;
			break;

		case 0x700:
			simpl = true;
			break;

		case 0x701:
			if (optarg) {
				Rect bbox(Point::invalid, Point::invalid);
				char *cp(strchr(optarg, ','));
				if (!cp)
					cp = strchr(optarg, ':');
				if (!cp)
					cp = strchr(optarg, '-');
				if (cp) {
					*cp++ = 0;
					Point pt0;
					if (!((Point::setstr_lat | Point::setstr_lon) & ~pt0.set_str(optarg))) {
						bbox.set_south(pt0.get_lat());
						bbox.set_west(pt0.get_lon());
					}
					Point pt1;
					if (!((Point::setstr_lat | Point::setstr_lon) & ~pt1.set_str(cp))) {
						bbox.set_north(pt1.get_lat());
						bbox.set_east(pt1.get_lon());
					}
				}
				if (bbox.get_northeast().is_invalid() || bbox.get_northeast().is_invalid())
					std::cerr << "cannot parse bbox: " << optarg << std::endl;
				else
					simplbbox = bbox;
			}
			break;

		case 0x702:
			if (optarg) {
				const char *cp(optarg);
				std::pair<int32_t,int32_t> a(-1, -1);
				while (std::isspace(*cp))
					++cp;
				char *cp1(0);
				a.first = strtoul(cp, &cp1, 0);
				if (cp == cp1)
					a.first = -1;
				cp = cp1;
				while (std::isspace(*cp))
					++cp;
				if (*cp == ':' || *cp == ',' || *cp == '-') {
					++cp;
					while (std::isspace(*cp))
						++cp;
				}
				a.second = strtoul(cp, &cp1, 0);
				if (cp == cp1)
					a.second = -1;
				if (a.first >= 0 && a.second >= 0)
					simplalt = a;
				else
					std::cerr << "cannot parse altitudes: " << optarg << std::endl;
			}
			break;

		case 0x703:
			if (optarg)
				simplacft = optarg;
			break;

		case 0x704:
			if (optarg) {
				simplequipment = optarg;
				simplpbn = Aircraft::pbn_none;
				std::string::size_type n(simplequipment.find(':'));
				if (n != std::string::npos) {
					simplpbn = Aircraft::parse_pbn(simplequipment.substr(n + 1));
					simplequipment.erase(n);
				}
			}
			break;

		case 0x705:
			simplflttype = 'G';
			break;

		case 0x706:
			simplcivmil = true;
			simplmil = false;
			break;

	        case 0x707:
			simplcivmil = true;
			simplmil = true;
			break;

		case 0x708:
			if (optarg)
				simpldep = ADR::Link(ADR::UUID::from_str(optarg));
			break;

		case 0x709:
			if (optarg)
				simpldest = ADR::Link(ADR::UUID::from_str(optarg));
			break;

		case 0x70a:
			simplcplx = true;
			break;

		case 0x70b:
			if (optarg) {
				char *cp(strchr(optarg, ','));
				if (!cp)
					cp = strchr(optarg, '/');
				bool ok1(false);
				if (cp) {
					*cp++ = 0;
					Glib::TimeVal tv;
					ok1 = tv.assign_from_iso8601(cp);
					simpltime.second = tv.tv_sec;
				}
				Glib::TimeVal tv;
				bool ok(tv.assign_from_iso8601(optarg));
				if (ok) {
					simpltime.first = tv.tv_sec;
					if (!ok1)
						simpltime.second = simpltime.first + 1;
				} else {
					std::cerr << "cannot parse time: " << optarg << std::endl;
				}
			}
			break;

		case 0x800:
			if (optarg) {
				char *cp(strchr(optarg, ','));
				if (!cp)
					cp = strchr(optarg, '/');
				if (cp) {
					*cp++ = 0;
					traceaspcpt1 = ADR::Link(ADR::UUID::from_str(cp));
				}
				if (!traceaspcpt0.is_nil() && traceaspcpt1.is_nil())
					traceaspcpt1 = ADR::Link(ADR::UUID::from_str(optarg));
				else
					traceaspcpt0 = ADR::Link(ADR::UUID::from_str(optarg));
			}
			break;

		case 0x801:
			if (optarg) {
				char *cp1(0);
			        int32_t a = strtol(optarg, &cp1, 0);
				if (cp1 == optarg)
					std::cerr << "Cannot parse altitude " << optarg << std::endl;
				else
					traceaspcalt = a;
			}
			break;

		case 0x802:
			if (optarg) {
				Glib::TimeVal tv;
				bool ok(tv.assign_from_iso8601(optarg));
				if (ok)
				        traceaspctime = tv.tv_sec;
				else
					std::cerr << "cannot parse time: " << optarg << std::endl;
			}
			break;

		default:
			err++;
			break;
                }
        }
        if (err) {
                std::cerr << "usage: adrquery [-d <dir>] [-D] [-A <dir>] [-u] [-i] [-a] [-p]" << std::endl;
                return EX_USAGE;
        }
	try {
		ADR::Database db(db_dir, binfile);
		if (aup) {
			ADR::AUPDatabase aupdb(aupdb_dir.empty() ? db_dir : aupdb_dir);
			if (optind >= argc) {
				ADR::AUPDatabase::findresults_t r(aupdb.find(timemin, timemax));
				std::cout << "Find all AUP: " << r.size() << " results" << std::endl;
				print_aup(db, r, blob);
			}
			for (; optind < argc; ++optind) {
				ADR::UUID uuid(ADR::UUID::from_str(argv[optind]));
				if (uuid.is_nil()) {
					std::cerr << "Cannot parse UUID " << argv[optind] << std::endl;
					break;
				}
				ADR::AUPDatabase::findresults_t r(aupdb.find(uuid, timemin, timemax));
				std::cout << "Find AUP by UUID: " << uuid << ": " << r.size() << " results" << std::endl;
				print_aup(db, r, blob);
			}
			return EX_OK;
		}
		if (!simpldep.is_nil())
			simpldep.load(db);
		if (!simpldest.is_nil())
			simpldest.load(db);
		if (simpltime.second >= simpltime.first && simpltime.first > 0)
			std::cout << "simplify time " << Glib::TimeVal(simpltime.first, 0).as_iso8601()
				  << '-' << Glib::TimeVal(simpltime.second, 0).as_iso8601() << std::endl;
		if (simpl)
			std::cout << "simplify" << std::endl;
		if (simplcplx)
			std::cout << "simplify complexity" << std::endl;
		if (!simplbbox.get_northeast().is_invalid() && !simplbbox.get_southwest().is_invalid())
			std::cout << "simplify bbox " << simplbbox.get_southwest().get_lat_str2() << ' ' << simplbbox.get_southwest().get_lon_str2()
				  << " - " << simplbbox.get_northeast().get_lat_str2() << ' '<< simplbbox.get_northeast().get_lon_str2() << std::endl;
		if (simplalt.second >= simplalt.first && simplalt.first >= 0)
			std::cout << "simplify alt " << simplalt.first << "..." << simplalt.second << std::endl;
		if (!simplacft.empty())
			std::cout << "simplify acft " << simplacft << ' ' << Aircraft::get_aircrafttypeclass(simplacft) << std::endl;
		if (!simplequipment.empty())
			std::cout << "simplify equipment " << simplequipment << ' ' << Aircraft::get_pbn_string(simplpbn) << std::endl;
		if (simplflttype)
			std::cout << "simplify type of flight " << simplflttype << std::endl;
		if (simplcivmil)
			std::cout << "simplify " << (simplmil ? "mil" : "civ") << std::endl;
		if (!simpldep.is_nil()) {
			std::cout << "simplify dep " << simpldep;
			for (unsigned int i(0), n(simpldep.get_obj()->size()); i < n; ++i) {
				const ADR::AirportTimeSlice& arpt(simpldep.get_obj()->operator[](i).as_airport());
				if (arpt.is_valid() && !arpt.get_ident().empty()) {
					std::cout << ' ' << arpt.get_ident();
					break;
				}
			}
			std::cout << std::endl;
		}
		if (!simpldest.is_nil()) {
			std::cout << "simplify dest " << simpldest;
			for (unsigned int i(0), n(simpldest.get_obj()->size()); i < n; ++i) {
				const ADR::AirportTimeSlice& arpt(simpldest.get_obj()->operator[](i).as_airport());
				if (arpt.is_valid() && !arpt.get_ident().empty()) {
					std::cout << ' ' << arpt.get_ident();
					break;
				}
			}
			std::cout << std::endl;
		}
		if (!traceaspcpt0.is_nil())
			traceaspcpt0.load(db);
		if (!traceaspcpt1.is_nil())
			traceaspcpt1.load(db);
		ADR::TimeTableSpecialDateEval ttsde;
		if (traceaspcpt0.get_obj())
			ttsde.load(db);
		for (; optind < argc; ++optind) {
			if (dct) {
				ADR::Database::dctresults_t r;
        			switch (mode) {
				default:
				{
					ADR::UUID uuid(ADR::UUID::from_str(argv[optind]));
					if (uuid.is_nil()) {
						std::cerr << "Cannot parse UUID " << argv[optind] << std::endl;
						break;
					}
					r = db.find_dct_by_uuid(uuid, ADR::Database::loadmode_obj);
					std::cout << "Find by UUID: " << uuid << ": " << r.size() << " results" << std::endl;
					break;
				}

				case mode_area:
				{
					Rect bbox;
					{
						Point pt;
						unsigned int r(pt.set_str(argv[optind]));
						if ((Point::setstr_lat | Point::setstr_lon) & ~r) {
							std::cerr << "Cannot parse coordinate " << argv[optind] << std::endl;
							break;
						}
						bbox = Rect(pt, pt);
						if (r & Point::setstr_excess)
							std::cerr << "Warning: Excess characters in coordinate " << argv[optind] << std::endl;
					}
					++optind;
					if (optind < argc) {
						Point pt;
						unsigned int r(pt.set_str(argv[optind]));
						if ((Point::setstr_lat | Point::setstr_lon) & ~r) {
							std::cerr << "Cannot parse coordinate " << argv[optind] << std::endl;
							break;
						}
						bbox = bbox.add(pt);
						if (r & Point::setstr_excess)
							std::cerr << "Warning: Excess characters in coordinate " << argv[optind] << std::endl;
					} else {
						bbox = bbox.get_northeast().simple_box_nmi(10);
					}
					r = db.find_dct_by_bbox(bbox, ADR::Database::loadmode_obj);
					std::cout << "Find by bbox: " << bbox.get_southwest().get_lat_str2() << ' '
						  << bbox.get_southwest().get_lon_str2() << ' ' << bbox.get_northeast().get_lat_str2() << ' ' 
						  << bbox.get_northeast().get_lon_str2() << ": " << r.size() << " results" << std::endl;
					break;
				}
				};
				time_t curtime(time(0));
				for (ADR::Database::dctresults_t::iterator ri(r.begin()), re(r.end()); ri != re; ++ri) {
					ADR::DctLeg& dl(*ri);
					dl.link(db);
					dl.print(std::cout, 0, curtime) << std::endl;
					if (!blob)
						continue;
					std::ostringstream blob;
					{
						ADR::ArchiveWriteStream ar(blob);
						dl.save(ar);
					}
					std::ostringstream oss;
					oss << std::hex;
					for (const uint8_t *p0(reinterpret_cast<const uint8_t *>(blob.str().c_str())),
						     *p1(reinterpret_cast<const uint8_t *>(blob.str().c_str()) + blob.str().size()); p0 != p1; ++p0)
						oss << ' ' << std::setfill('0') << std::setw(2) << (unsigned int)*p0;
					std::cout << "  blob: " << oss.str() << std::endl;
				}
				continue;
			}
			ADR::Database::findresults_t r;
			switch (mode) {
			default:
			{
				ADR::UUID uuid(ADR::UUID::from_str(argv[optind]));
				if (uuid.is_nil()) {
					std::cerr << "Cannot parse UUID " << argv[optind] << std::endl;
					break;
				}
				r.push_back(ADR::Link(uuid));
				r.back().load(db);
				if (r.back().get_obj())
					r.back().get_obj()->link(db, ~0U);
				else
					r.clear();
				std::cout << "Find by UUID: " << uuid << ": " << r.size() << " results" << std::endl;
				break;
			}

			case mode_ident:
			{
				r = db.find_by_ident(argv[optind], comp, 0, ADR::Database::loadmode_link, timemin, timemax, typemin, typemax, limit);
				std::cout << "Find by ident: " << argv[optind] << ": " << r.size() << " results" << std::endl;
				break;
			}

			case mode_area:
			{
				Rect bbox;
				{
					Point pt;
					unsigned int r(pt.set_str(argv[optind]));
					if ((Point::setstr_lat | Point::setstr_lon) & ~r) {
						std::cerr << "Cannot parse coordinate " << argv[optind] << std::endl;
						break;
					}
					bbox = Rect(pt, pt);
					if (r & Point::setstr_excess)
						std::cerr << "Warning: Excess characters in coordinate " << argv[optind] << std::endl;
				}
				++optind;
				if (optind < argc) {
					Point pt;
					unsigned int r(pt.set_str(argv[optind]));
					if ((Point::setstr_lat | Point::setstr_lon) & ~r) {
						std::cerr << "Cannot parse coordinate " << argv[optind] << std::endl;
						break;
					}
					bbox = bbox.add(pt);
					if (r & Point::setstr_excess)
						std::cerr << "Warning: Excess characters in coordinate " << argv[optind] << std::endl;
				} else {
					bbox = bbox.get_northeast().simple_box_nmi(10);
				}
				r = db.find_by_bbox(bbox, ADR::Database::loadmode_link, timemin, timemax, typemin, typemax, limit);
				std::cout << "Find by bbox: " << bbox.get_southwest().get_lat_str2() << ' '
					  << bbox.get_southwest().get_lon_str2() << ' ' << bbox.get_northeast().get_lat_str2() << ' ' 
					  << bbox.get_northeast().get_lon_str2() << ": " << r.size() << " results" << std::endl;
				break;
			}

			case mode_pointdist:
			{
				Rect bbox;
				{
					Point pt;
					unsigned int r(pt.set_str(argv[optind]));
					if ((Point::setstr_lat | Point::setstr_lon) & ~r) {
						std::cerr << "Cannot parse coordinate " << argv[optind] << std::endl;
						break;
					}
					bbox = Rect(pt, pt);
					if (r & Point::setstr_excess)
						std::cerr << "Warning: Excess characters in coordinate " << argv[optind] << std::endl;
				}
				++optind;
				{
					double radius(10);
					if (optind < argc)
						radius = strtod(argv[optind], 0);
					bbox = bbox.get_northeast().simple_box_nmi(radius);
				}
				r = db.find_by_bbox(bbox, ADR::Database::loadmode_link, timemin, timemax, typemin, typemax, limit);
				std::cout << "Find by bbox: " << bbox.get_southwest().get_lat_str2() << ' '
					  << bbox.get_southwest().get_lon_str2() << ' ' << bbox.get_northeast().get_lat_str2() << ' ' 
					  << bbox.get_northeast().get_lon_str2() << ": " << r.size() << " results" << std::endl;
				break;
			}

			case mode_dependencies:
			{
				ADR::UUID uuid(ADR::UUID::from_str(argv[optind]));
				if (uuid.is_nil()) {
					std::cerr << "Cannot parse UUID " << argv[optind] << std::endl;
					break;
				}
				r = db.find_dependencies(uuid, ADR::Database::loadmode_link, timemin, timemax, typemin, typemax, limit);
				std::cout << "Find Dependencies: " << uuid << ": " << r.size() << " results" << std::endl;
				break;
			}

			case mode_dependson:
			{
				ADR::UUID uuid(ADR::UUID::from_str(argv[optind]));
				if (uuid.is_nil()) {
					std::cerr << "Cannot parse UUID " << argv[optind] << std::endl;
					break;
				}
				r = db.find_dependson(uuid, ADR::Database::loadmode_link, timemin, timemax, typemin, typemax, limit);
				std::cout << "Find Depends On: " << uuid << ": " << r.size() << " results" << std::endl;
				break;
			}
			};
			for (ADR::Database::findresults_t::const_iterator ri(r.begin()), re(r.end()); ri != re; ++ri) {
				const ADR::Link& l(*ri);
				ADR::Object::ptr_t p(l.get_obj());
				if (!p) {
					std::cout << l << " object not found" << std::endl;
					break;
				}
				if (simpltime.second >= simpltime.first && simpltime.first > 0) {
					ADR::Object::ptr_t p1(p->simplify_time(simpltime));
					if (p1)
						p.swap(p1);
				}
				{
					ADR::FlightRestriction::ptr_t pf(ADR::FlightRestriction::ptr_t::cast_dynamic(p));
					if (pf) {
						if (simpl) {
							ADR::FlightRestriction::ptr_t p1(pf->simplify());
							if (p1)
								pf.swap(p1);
						}
						if (simplcplx) {
							ADR::FlightRestriction::ptr_t p1(pf->simplify_complexity());
							if (p1)
								pf.swap(p1);
						}
						if (!simplbbox.get_northeast().is_invalid() && !simplbbox.get_southwest().is_invalid()) {
							ADR::FlightRestriction::ptr_t p1(pf->simplify_bbox(simplbbox));
							if (p1)
								pf.swap(p1);
						}
						if (simplalt.second >= simplalt.first && simplalt.first >= 0) {
							ADR::FlightRestriction::ptr_t p1(pf->simplify_altrange(simplalt.first, simplalt.second));
							if (p1)
								pf.swap(p1);
						}
						if (!simplacft.empty()) {
							ADR::FlightRestriction::ptr_t p1(pf->simplify_aircrafttype(simplacft));
							if (p1)
								pf.swap(p1);
							p1 = pf->simplify_aircraftclass(Aircraft::get_aircrafttypeclass(simplacft));
							if (p1)
								pf.swap(p1);
						}
						if (!simplequipment.empty()) {
							ADR::FlightRestriction::ptr_t p1(pf->simplify_equipment(simplequipment, simplpbn));
							if (p1)
								pf.swap(p1);
						}
						if (simplflttype) {
							ADR::FlightRestriction::ptr_t p1(pf->simplify_typeofflight(simplflttype));
							if (p1)
								pf.swap(p1);
						}
						if (simplcivmil) {
							ADR::FlightRestriction::ptr_t p1(pf->simplify_mil(simplmil));
							if (p1)
								pf.swap(p1);
						}
						if (!simpldep.is_nil()) {
							ADR::FlightRestriction::ptr_t p1(pf->simplify_dep(simpldep));
							if (p1)
								pf.swap(p1);
						}
						if (!simpldest.is_nil()) {
							ADR::FlightRestriction::ptr_t p1(pf->simplify_dest(simpldest));
							if (p1)
								pf.swap(p1);
						}
						p = pf;
					}
				}
			        p->print(std::cout);
				if (traceaspcpt0.get_obj()) {
					ADR::Airspace::ptr_t pa(ADR::Airspace::ptr_t::cast_dynamic(p));
					ADR::PointIdentTimeSlice& tspt0(traceaspcpt0.get_obj()->operator()(traceaspctime).as_point());
					ADR::PointIdentTimeSlice& tspt1(traceaspcpt1.get_obj()->operator()(traceaspctime).as_point());
					if (pa && tspt0.is_valid() && !tspt0.get_coord().is_invalid()) {
						std::vector<ADR::AirspaceTimeSlice::Trace> t;
						ADR::TimeTableEval tte(traceaspctime, tspt0.get_coord(), ttsde);
						if (tspt1.is_valid() && !tspt1.get_coord().is_invalid()) {
							t = pa->trace_intersect(tte, tspt1.get_coord(), traceaspcalt, ADR::AltRange());
							std::cout << "  Intersect time " << Glib::TimeVal(traceaspctime, 0).as_iso8601() << " alt ";
							if (traceaspcalt == ADR::AltRange::altignore)
								std::cout << "ignored";
							else
								std::cout << traceaspcalt << "ft";
							std::cout << std::endl << "    " << traceaspcpt0 << ' ' << tspt0.get_ident()
								  << ' ' << tspt0.get_coord().get_lat_str2() << ' ' << tspt0.get_coord().get_lon_str2()
								  << std::endl << "    " << traceaspcpt1 << ' ' << tspt1.get_ident()
								  << ' ' << tspt1.get_coord().get_lat_str2() << ' ' << tspt1.get_coord().get_lon_str2()
								  << std::endl;
						} else {
							t = pa->trace_inside(tte, traceaspcalt, ADR::AltRange(), traceaspcpt0);
							std::cout << "  Inside time " << Glib::TimeVal(traceaspctime, 0).as_iso8601() << " alt ";
							if (traceaspcalt == ADR::AltRange::altignore)
								std::cout << "ignored";
							else
								std::cout << traceaspcalt << "ft";
							std::cout << std::endl << "    " << traceaspcpt0 << ' ' << tspt0.get_ident()
								  << ' ' << tspt0.get_coord().get_lat_str2() << ' ' << tspt0.get_coord().get_lon_str2()
								  << std::endl;
						}
						for (std::vector<ADR::AirspaceTimeSlice::Trace>::const_iterator ti(t.begin()), te(t.end()); ti != te; ++ti) {
							const ADR::Link& la(ti->get_airspace());
							ADR::AirspaceTimeSlice& tsa(la.get_obj()->operator()(traceaspctime).as_airspace());
							std::cout << "      " << la;
							if (tsa.is_valid())
								std::cout << ' ' << tsa.get_ident();
							if (ti->get_component() != ADR::AirspaceTimeSlice::Trace::nocomponent)
								std::cout << " component " << ti->get_component();
							std::cout << ' ' << ti->get_reason() << ' ' << (*ti ? "" : "not ") << "ok" << std::endl;
						}
					}
				}
				if (!blob)
					continue;
				std::ostringstream blob;
				{
					ADR::ArchiveWriteStream ar(blob);
					p->save(ar);
				}
				std::ostringstream oss;
				oss << std::hex;
				for (const uint8_t *p0(reinterpret_cast<const uint8_t *>(blob.str().c_str())),
					     *p1(reinterpret_cast<const uint8_t *>(blob.str().c_str()) + blob.str().size()); p0 != p1; ++p0)
					oss << ' ' << std::setfill('0') << std::setw(2) << (unsigned int)*p0;
				std::cout << "  blob (" << blob.str().size() << "): " << oss.str() << std::endl;
			}
		}
	} catch (const xmlpp::exception& ex) {
                std::cerr << "libxml++ exception: " << ex.what() << std::endl;
                return EX_DATAERR;
        } catch (const std::exception& ex) {
                std::cerr << "exception: " << ex.what() << std::endl;
                return EX_DATAERR;
        }
        return EX_OK;
}
