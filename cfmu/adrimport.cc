//
// C++ Implementation: adrimport
//
// Description: AIXM Database to sqlite db conversion
//
//
// Author: Thomas Sailer <t.sailer@alumni.ethz.ch>, (C) 2007, 2009, 2012, 2013, 2014
//
// Copyright: See COPYING file that comes with this distribution
//
//

#include "sysdeps.h"

#include <boost/graph/adjacency_list.hpp>
#include <boost/graph/topological_sort.hpp>
#include <boost/uuid/string_generator.hpp>

#include "adr.hh"
#include "adrparse.hh"
#include "adrbdry.hh"
#include "adrhibernate.hh"

#include "getopt.h"

static std::pair<unsigned int,unsigned int> recompute(ADR::Database& db, TopoDb30& topodb, uint64_t tmodified, bool verbose)
{
	typedef boost::adjacency_list<boost::vecS,boost::vecS,boost::directedS,boost::property<boost::vertex_color_t,boost::default_color_type> > Graph;
        typedef boost::graph_traits<Graph>::vertex_descriptor vertex_descriptor;
        typedef boost::graph_traits<Graph>::edge_descriptor edge_descriptor;
	typedef std::vector<ADR::UUID> uuids_t;
	uuids_t uuids;
	typedef std::vector<bool> temps_t;
	temps_t temps;
	temps_t done;
	temps_t changed;
	Graph topograph;
	{
		typedef std::map<ADR::UUID,vertex_descriptor> uuidmap_t;
		uuidmap_t uuidmap;
		// create vertices
		{
			ADR::Database::findresults_t r(db.find_all_temp(ADR::Database::loadmode_uuid));
			for (ADR::Database::findresults_t::const_iterator ri(r.begin()), re(r.end()); ri != re; ++ri) {
				std::pair<uuidmap_t::iterator,bool> ins(uuidmap.insert(std::make_pair(static_cast<const ADR::UUID&>(*ri), uuids.size())));
				if (!ins.second) {
					std::ostringstream oss;
					oss << "duplicate UUID " << *ri;
					throw std::runtime_error(oss.str());
				}
				uuids.push_back(*ri);
				temps.push_back(true);
				done.push_back(false);
				changed.push_back(false);
				boost::add_vertex(topograph);
			}
			r.clear();
			r = db.find_all(ADR::Database::loadmode_uuid);
			for (ADR::Database::findresults_t::const_iterator ri(r.begin()), re(r.end()); ri != re; ++ri) {
				std::pair<uuidmap_t::iterator,bool> ins(uuidmap.insert(std::make_pair(static_cast<const ADR::UUID&>(*ri), uuids.size())));
				if (!ins.second)
					continue;
				uuids.push_back(*ri);
				temps.push_back(false);
				done.push_back(false);
				changed.push_back(false);
				boost::add_vertex(topograph);
			}
		}
		// add dependency edges
		{
			ADR::Database::deppairs_t dp(db.find_all_temp_deps());
			for (ADR::Database::deppairs_t::const_iterator dpi(dp.begin()), dpe(dp.end()); dpi != dpe; ++dpi) {
				vertex_descriptor vd0, vd1;
				{
					uuidmap_t::const_iterator i(uuidmap.find(dpi->first));
					if (i == uuidmap.end()) {
						std::ostringstream oss;
						oss << "UUID not found " << dpi->first;
						throw std::runtime_error(oss.str());
					}
					vd0 = i->second;
				}
				{
					uuidmap_t::const_iterator i(uuidmap.find(dpi->second));
					if (i == uuidmap.end()) {
						std::ostringstream oss;
						oss << "UUID(D) not found " << dpi->second << " for " << dpi->first;
						throw std::runtime_error(oss.str());
					}
					vd1 = i->second;
				}
				if (!temps[vd0]) {
					std::ostringstream oss;
					oss << "Temp dependency for non-temp UUID " << uuids[vd0] << " -> " << uuids[vd1];
					throw std::runtime_error(oss.str());
				}
				boost::add_edge(vd0, vd1, topograph);
				if (false)
					std::cerr << "Dep " << uuids[vd0] << (temps[vd0] ? " (T)" : "")
						  << " -> " << uuids[vd1] << (temps[vd1] ? " (T)" : "") << std::endl;
			}
			dp.clear();
			dp = db.find_all_deps();
			for (ADR::Database::deppairs_t::const_iterator dpi(dp.begin()), dpe(dp.end()); dpi != dpe; ++dpi) {
				vertex_descriptor vd0, vd1;
{
					uuidmap_t::const_iterator i(uuidmap.find(dpi->first));
					if (i == uuidmap.end()) {
						std::ostringstream oss;
						oss << "UUID not found " << dpi->first;
						throw std::runtime_error(oss.str());
					}
					vd0 = i->second;
				}
				if (temps[vd0])
					continue;
				{
					uuidmap_t::const_iterator i(uuidmap.find(dpi->second));
					if (i == uuidmap.end()) {
						std::ostringstream oss;
						oss << "UUID(D) not found " << dpi->second << " for " << dpi->first;
						throw std::runtime_error(oss.str());
					}
					vd1 = i->second;
				}
				boost::add_edge(vd0, vd1, topograph);
				if (false)
					std::cerr << "Dep " << uuids[vd0] << (temps[vd0] ? " (T)" : "")
						  << " -> " << uuids[vd1] << (temps[vd1] ? " (T)" : "") << std::endl;
			}
		}
		if (verbose)
			std::cout << "Dependency Graph: " << boost::num_vertices(topograph) << " (" << uuids.size() << ", "
				  << temps.size() << ", " << done.size() << ", " << changed.size() << ") vertices, "
				  << boost::num_edges(topograph) << " edges" << std::endl;
	}
	typedef std::vector<vertex_descriptor> container_t;
	container_t c;
	topological_sort(topograph, std::back_inserter(c));
	std::pair<unsigned int,unsigned int> stats(0, 0);
	unsigned int objcount(0);
	for (container_t::const_iterator ci(c.begin()), ce(c.end()); ci != ce; ++ci) {
		const ADR::UUID& uuid(uuids[*ci]);
		// check dependencies
		if (true) {
			Graph::out_edge_iterator e0, e1;
			for (boost::tie(e0, e1) = boost::out_edges(*ci, topograph); e0 != e1; ++e0) {
				vertex_descriptor vt(boost::target(*e0, topograph));
				if (false)
					std::cerr << "Dep: " << uuid << " dependency " << uuids[vt] << std::endl;
				if (done[vt])
					continue;
				std::cerr << "WARNING: " << uuid << " dependency " << uuids[vt] << " not done" << std::endl;
			}
		}
		ADR::Object::ptr_t p;
		ADR::Object::const_ptr_t pold;
		if (temps[*ci]) {
			p = ADR::Object::ptr_t::cast_const(db.load_temp(uuid));
			pold = db.load(uuid);
		} else {
			bool haschanged(false);
			Graph::out_edge_iterator e0, e1;
			for (boost::tie(e0, e1) = boost::out_edges(*ci, topograph); e0 != e1; ++e0) {
				haschanged = changed[boost::target(*e0, topograph)];
				if (haschanged)
					break;
			}
			if (haschanged || tmodified != std::numeric_limits<uint64_t>::max()) {
				p = ADR::Object::ptr_t::cast_const(db.load(uuid));
				pold = p;
				if (!p || p->get_modified() >= tmodified)
					haschanged = true;
			}
			if (!haschanged) {
				++stats.second;
				done[*ci] = true;
				continue;
			}
		}
		if (!p) {
			std::cerr << "ERROR: cannot load UUID " << uuid << std::endl;
			continue;
		}
		bool chg(!pold);
		{
			if (!chg)
				chg = pold->get_modified() >= tmodified;
			std::ostringstream blobbefore;
			if (!chg) {
				ADR::ArchiveWriteStream ar(blobbefore);
				pold->save(ar);
			}
			try {
				p->link(db);
			} catch (const std::exception& e) {
				std::ostringstream oss;
				oss << e.what() << " [linking " << p->get_uuid() << ']';
				throw std::runtime_error(oss.str());
			}
			try {
				p->recompute(topodb);
			} catch (const std::exception& e) {
				std::ostringstream oss;
				oss << e.what() << " [recomputing " << p->get_uuid() << ']';
				throw std::runtime_error(oss.str());
			}
			if (!chg) {
				std::ostringstream blobafter;
				{
					ADR::ArchiveWriteStream ar(blobafter);
					p->save(ar);
				}
				chg = blobbefore.str() != blobafter.str();
			}
		}
		changed[*ci] = chg;
		if (chg) {
			p->set_modified();
			p->set_dirty();
			db.save(p);
			++stats.first;
		} else {
			++stats.second;
		}
		done[*ci] = true;
		Glib::TimeVal tv(p->get_modified(), 0);
		std::cout << "UUID " << uuid << (temps[*ci] ? " (T)" : "") << ' ' << (chg ? "modified" : "unmodified")
			  << ' ' << tv.as_iso8601() << " [" << objcount << ']' << std::endl;
		if (verbose)
			p->print(std::cout);
		++objcount;
		if (!(objcount & ((1 << 10) - 1))) {
			Glib::TimeVal tv;
			tv.assign_current_time();
			tv.subtract_seconds(60);
			unsigned int cnt(db.flush_cache(tv));
			if (cnt || verbose)
				std::cout << "flushed " << cnt << " objects from cache" << std::endl;
		}
	}
	return stats;
}

static unsigned int recompute_db(ADR::Database& db, bool verbose)
{
	db.clear_cache();
	ADR::Database::findresults_t r;
	r = db.find_all(ADR::Database::loadmode_link, 0, std::numeric_limits<uint64_t>::max(),
			ADR::Object::type_flightrestriction, ADR::Object::type_flightrestriction, 0);
	db.clear_cache();
	unsigned int modif(0);
	for (ADR::Database::findresults_t::iterator i(r.begin()), e(r.end()); i != e; ++i) {
		i->load(db);
		if (!i->get_obj()) {
			std::cerr << "ERROR: cannot load UUID " << *i << std::endl;
			continue;
		}

		std::cerr << "recompute_db: " << i->get_obj()->operator[](0).as_ident().get_ident() << std::endl;

		try {
			i->get_obj()->link(db, ~0U);
		} catch (const std::exception& e) {
			std::ostringstream oss;
			oss << e.what() << " [linking " << *i << ']';
			throw std::runtime_error(oss.str());
		}
		try {
			i->get_obj()->recompute(db);
		} catch (const std::exception& e) {
			std::ostringstream oss;
			oss << e.what() << " [recomputing " << *i << ']';
			throw std::runtime_error(oss.str());
		}
		if (i->get_obj()->is_dirty()) {
			db.save(i->get_obj());
			++modif;
			if (verbose)
				std::cout << "Recompute DB: " << *i << " modified "
					  << (i->get_obj()->operator[](0).as_ident().get_ident()) << std::endl;
		}
	}
	db.clear_cache();
	return modif;
}

static unsigned int recompute_airportflags(ADR::Database& db, ADR::timetype_t cutofftime, ADR::timetype_t endtime)
{
	typedef std::map<ADR::UUID,ADR::timeinterval_t> vfrtime_t;
	vfrtime_t vfrdep, vfrarr;
	{
		db.clear_cache();
		ADR::Database::findresults_t r;
		r = db.find_all(ADR::Database::loadmode_link, 0, std::numeric_limits<uint64_t>::max(),
				ADR::Object::type_flightrestriction, ADR::Object::type_flightrestriction, 0);
		db.clear_cache();
		for (ADR::Database::findresults_t::iterator i(r.begin()), e(r.end()); i != e; ++i) {
			ADR::FlightRestriction::ptr_t p(ADR::FlightRestriction::ptr_t::cast_dynamic(i->get_obj()));
			if (!p)
				continue;
			for (unsigned int i(0), n(p->size()); i < n; ++i) {
				const ADR::FlightRestrictionTimeSlice& tsr(p->operator[](i).as_flightrestriction());
				if (!tsr.is_valid())
					continue;
				std::set<ADR::Link> dep, arr;
				if (!tsr.is_deparr(dep, arr))
					continue;
				{
					const ADR::TimeTable& tt(tsr.get_timetable());
					if (tt.is_never())
						continue;
					ADR::TimeTableWeekdayPattern wd;
					if (tt.convert(wd) && !wd.is_h24())
						continue;
				}
				ADR::timeinterval_t::Intvl intv(tsr.get_starttime(),
								std::min(tsr.get_endtime(), std::numeric_limits<ADR::timetype_t>::max() - 1) + 1);
				for (std::set<ADR::Link>::const_iterator i(dep.begin()), e(dep.end()); i != e; ++i) {
					std::pair<vfrtime_t::iterator,bool> ins(vfrdep.insert(vfrtime_t::value_type(*i, ADR::timeinterval_t())));
					ins.first->second |= intv;
				}
				for (std::set<ADR::Link>::const_iterator i(arr.begin()), e(arr.end()); i != e; ++i) {
					std::pair<vfrtime_t::iterator,bool> ins(vfrarr.insert(vfrtime_t::value_type(*i, ADR::timeinterval_t())));
					ins.first->second |= intv;
				}
			}
		}
	}
	if (false) {
		std::cerr << "VFR Departure:" << std::endl;
		for (vfrtime_t::const_iterator vi(vfrdep.begin()), ve(vfrdep.end()); vi != ve; ++vi) {
			ADR::Link l(vi->first);
			l.load(db);
			std::cerr << "  " << l.get_obj()->operator()(cutofftime).as_ident().get_ident() << ' '
				  << l << ' ' << vi->second.to_str() << std::endl;
		}
		std::cerr << "VFR Arrival:" << std::endl;
		for (vfrtime_t::const_iterator vi(vfrarr.begin()), ve(vfrarr.end()); vi != ve; ++vi) {
			ADR::Link l(vi->first);
			l.load(db);
			std::cerr << "  " << l.get_obj()->operator()(cutofftime).as_ident().get_ident() << ' '
				  << l << ' ' << vi->second.to_str() << std::endl;
		}
	}
	unsigned int ret(0);
	{
		ADR::timeinterval_t::Intvl flgintv(cutofftime,
						   std::min(endtime, std::numeric_limits<ADR::timetype_t>::max() - 1) + 1);
		ADR::Database::findresults_t r;
		r = db.find_all(ADR::Database::loadmode_link, 0, std::numeric_limits<uint64_t>::max(),
				ADR::Object::type_airport, ADR::Object::type_airportend, 0);
		db.clear_cache();
		for (ADR::Database::findresults_t::iterator ri(r.begin()), re(r.end()); ri != re; ++ri) {
			for (unsigned int i(0), n(ri->get_obj()->size()); i < n; ++i) {
				ADR::AirportTimeSlice& ts(ri->get_obj()->operator[](i).as_airport());
				if (!ts.is_valid())
					continue;
				uint8_t flags(ts.get_flags());
				flags |= ADR::AirportTimeSlice::flag_depifr | ADR::AirportTimeSlice::flag_arrifr;
				ADR::timeinterval_t intv(flgintv);
				intv &= ADR::timeinterval_t::Intvl(ts.get_starttime(),
								   std::min(ts.get_endtime(), std::numeric_limits<ADR::timetype_t>::max() - 1) + 1);
				{				
					vfrtime_t::const_iterator i(vfrdep.find(*ri));
					if (i != vfrdep.end()) {
						ADR::timeinterval_t x(~i->second);
						x &= intv;
						if (x.is_empty())
							flags &= ~ADR::AirportTimeSlice::flag_depifr;
					}
				}
				{				
					vfrtime_t::const_iterator i(vfrarr.find(*ri));
					if (i != vfrarr.end()) {
						ADR::timeinterval_t x(~i->second);
						x &= intv;
						if (x.is_empty())
							flags &= ~ADR::AirportTimeSlice::flag_arrifr;
					}
				}
				if (flags == ts.get_flags())
					continue;
				ts.set_flags(flags);
				ri->get_obj()->set_dirty(true);
			}
			if (ri->get_obj()->is_dirty()) {
				db.save(ri->get_obj());
				++ret;
			}
		}
	}
	return ret;
}

int main(int argc, char *argv[])
{
	static struct option long_options[] = {
		{ "dir", required_argument, 0, 'd' },
		{ "modified", required_argument, 0, 'm' },
		{ "border", required_argument, 0, 'b' },
		{ "verbose", no_argument, 0, 'v' },
		{ "binfile", required_argument, 0, 'B' },
		{ "dct", no_argument, 0, 'D' },
		{ "dct-point", required_argument, 0, 'P' },
		{ "dct-modtime", required_argument, 0, 0x400 },
		{ "dct-cutofftime", required_argument, 0, 0x401 },
		{ "dct-futurecutofftime", required_argument, 0, 0x402 },
		{ "dct-futurecutoffdays", required_argument, 0, 0x403 },
		{ "dct-all", no_argument, 0, 0x404 },
		{ "dct-verbose", no_argument, 0, 0x405 },
		{ "airport-flags", no_argument, 0, 0x406 },
		{ "dct-limit", required_argument, 0, 0x407 },
		{ "dct-worker", required_argument, 0, 0x408 },
		{ "airport-flags-cutofftime", no_argument, 0, 0x409 },
		{ "airport-flags-endtime", no_argument, 0, 0x40a },
		{0, 0, 0, 0}
	};
        Glib::ustring db_dir(".");
	std::string borderfile;
	std::string binfile;
	ADR::timetype_t modtime(std::numeric_limits<uint64_t>::max());
	ADR::timetype_t dctmodtime;
	ADR::timetype_t dctcutofftime;
	ADR::timetype_t dctfuturecutofftime;
	ADR::timetype_t arptflagscutofftime;
	ADR::timetype_t arptflagsendtime;
	ADR::Database::findresults_t dctpt;
	double dctlimit(100);
	unsigned int dctworker(0);
	bool verbose(false), dct(false), dctall(false), dctverbose(false), dctfuturecutoffrel(false), arptflags(0);
	int c, err(0);
	{
		Glib::TimeVal tv;
		tv.assign_current_time();
		arptflagscutofftime = dctcutofftime = dctmodtime = tv.tv_sec;
		arptflagsendtime = arptflagscutofftime + 28*86400; // roughly 1 AIRAC cycle
	}
	{
		struct tm tm;
		memset(&tm, 0, sizeof(tm));
		tm.tm_year = 2999 - 1900;
		tm.tm_mon = 12 - 1;
		tm.tm_mday = 31;
		dctfuturecutofftime = timegm(&tm);
	}
	Glib::init();
	Glib::set_application_name("adrimport");
	Glib::thread_init();
	ADR::ParseOGR::global_init();
	while ((c = getopt_long(argc, argv, "d:m:b:B:DP:v", long_options, 0)) != EOF) {
		switch (c) {
		case 'd':
			if (optarg)
				db_dir = optarg;
			break;

		case 'm':
			if (optarg) {
				Glib::TimeVal tv;
				if (!tv.assign_from_iso8601(optarg)) {
					std::cerr << "Cannot parse date " << optarg << std::endl;
					break;
				}
				modtime = tv.tv_sec;
			}
			break;

		case 'b':
			if (optarg)
				borderfile = optarg;
			break;

		case 'B':
			if (optarg)
				binfile = optarg;
			break;

		case 'v':
			verbose = true;
			break;

		case 'D':
			dct = true;
			break;

		case 'P':
			if (optarg) {
				try {
					boost::uuids::string_generator gen;
					ADR::UUID u(gen(optarg));
					if (u.is_nil())
						std::cerr << "UUID " << optarg << " is nil" << std::endl;
					else
						dctpt.push_back(ADR::Link(u));
				} catch (const std::runtime_error& e) {
					std::cerr << "Cannot parse UUID " << optarg << ": " << e.what() << std::endl;
				}
			}
			break;

		case 0x400:
			if (optarg) {
				Glib::TimeVal tv;
				if (!tv.assign_from_iso8601(optarg)) {
					std::cerr << "Cannot parse date " << optarg << std::endl;
					break;
				}
				dctmodtime = tv.tv_sec;
			}
			break;

		case 0x401:
			if (optarg) {
				Glib::TimeVal tv;
				if (!tv.assign_from_iso8601(optarg)) {
					std::cerr << "Cannot parse date " << optarg << std::endl;
					break;
				}
				dctcutofftime = tv.tv_sec;
			}
			break;

		case 0x402:
			if (optarg) {
				Glib::TimeVal tv;
				if (!tv.assign_from_iso8601(optarg)) {
					std::cerr << "Cannot parse date " << optarg << std::endl;
					break;
				}
				dctfuturecutofftime = tv.tv_sec;
				dctfuturecutoffrel = false;
			}
			break;

		case 0x403:
			if (optarg) {
				dctfuturecutofftime = strtoul(optarg, 0, 0) * (24 * 60 * 60);
				dctfuturecutoffrel = true;
			}
			break;

		case 0x404:
			dctall = !dctall;
			break;

		case 0x405:
			dctverbose = !dctverbose;
			break;

		case 0x406:
			arptflags = !arptflags;
			break;

		case 0x407:
			dctlimit = strtod(optarg, 0);
			break;

		case 0x408:
			dctworker = strtoul(optarg, 0, 0);
			break;

		case 0x409:
			if (optarg) {
				Glib::TimeVal tv;
				if (!tv.assign_from_iso8601(optarg)) {
					std::cerr << "Cannot parse date " << optarg << std::endl;
					break;
				}
				arptflagscutofftime = tv.tv_sec;
			}
			break;

		case 0x40a:
			if (optarg) {
				Glib::TimeVal tv;
				if (!tv.assign_from_iso8601(optarg)) {
					std::cerr << "Cannot parse date " << optarg << std::endl;
					break;
				}
				arptflagsendtime = tv.tv_sec;
			}
			break;

		default:
			err++;
			break;
		}
	}
	if (err) {
		std::cerr << "usage: adrimport [-d <dir>] [-b <borderfile>] [-m <modtime>] [-v] [--dct]" << std::endl;
		return EX_USAGE;
	}
	try {
		ADR::Database db(db_dir, false);
		if (!borderfile.empty() || optind < argc || arptflags || dct) {
			// try Write Ahead Logging mode
			db.set_wal(true);
			db.delete_simple_transitive_closure();
			unsigned int err(0), warn(0);
			if (!borderfile.empty()) {
				ADR::ParseOGR parser(db, verbose);
				std::cerr << "Parsing border file " << borderfile << std::endl;
				parser.parse(borderfile);
				parser.addcomposite();
				err += parser.get_errorcnt();
				warn += parser.get_warncnt();
				std::cerr << "Border File Import: " << err << " errors, " << warn << " warnings" << std::endl;
			}
			{
				ADR::ParseXML parser(db, verbose);
				parser.set_validate(false); // Do not validate, we do not have a DTD
				parser.set_substitute_entities(true);
				for (; optind < argc; optind++) {
					std::cerr << "Parsing file " << argv[optind] << std::endl;
					parser.parse_file(argv[optind]);
				}
				err += parser.get_errorcnt();
				warn += parser.get_warncnt();
			}
			std::cerr << "Import: " << err << " errors, " << warn << " warnings" << std::endl;
			std::string topodbpath(db_dir);
			if (topodbpath.empty())
				topodbpath = PACKAGE_DATA_DIR;
			TopoDb30 topodb;
			topodb.open(topodbpath);
			std::pair<unsigned int,unsigned int> stats(recompute(db, topodb, modtime, verbose));
			// very big and no win
			if (false)
				db.create_simple_transitive_closure();
			std::cerr << "Recompute: " << err << " errors, " << warn << " warnings, "
				  << stats.first << " modified, " << stats.second << " unmodified" << std::endl;
			{
				unsigned int mc(recompute_db(db, verbose));
				std::cerr << "Recompute Restrictions Airway Segments: " << mc << " restrictions modified" << std::endl;
			}
			if (arptflags) {
				unsigned int mc(recompute_airportflags(db, arptflagscutofftime, arptflagsendtime));
				std::cerr << "Recompute Airport Flags: " << mc << " airports modified" << std::endl;
			}
			if (dct) {
				if (dctfuturecutoffrel)
					dctfuturecutofftime += dctcutofftime;
				ADR::DctParameters dctp(&db, &topodb, topodbpath, dctmodtime, dctcutofftime, dctfuturecutofftime, dctlimit, dctworker, dctall, dctverbose);
				dctp.load_rules();
				if (dctpt.empty())
					dctp.load_points();
				else
					dctp.load_points(dctpt);
				dctp.run();
				err += dctp.get_errorcnt();
				warn += dctp.get_warncnt();
				std::cerr << "DCT: " << err << " errors, " << warn << " warnings" << std::endl;
			}
			// disable WAL mode - read only databases are not possible with WAL
			db.set_wal(false);
		}
		if (!binfile.empty())
			db.write_binfile(binfile);
	} catch (const xmlpp::exception& ex) {
		std::cerr << "libxml++ exception: " << ex.what() << std::endl;
		return EX_DATAERR;
	} catch (const std::exception& ex) {
		std::cerr << "exception: " << ex.what() << std::endl;
		return EX_DATAERR;
	}
	return EX_OK;
}
