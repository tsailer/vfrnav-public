/*
 * airwaydump.cc:  Airway dumper
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation; either version 2 of the License, or (at your
 * option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General
 * Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program; if not, write to the Free Software Foundation, Inc.,
 * 59 Temple Place, Suite 330, Boston, MA 02111-1307 USA
 *
 */

#include "sysdeps.h"

#include <getopt.h>
#include <iomanip>
#include <iostream>
#include <sstream>
#include <limits>
#include <sqlite3x.hpp>
#include <sqlite3.h>
#include <unistd.h>
#include <stdexcept>
#include <glibmm.h>
#include <giomm.h>

#include "geom.h"
#include "dbobj.h"
#include "engine.h"


/* ---------------------------------------------------------------------- */

class AirwayDump {
public:
	AirwayDump(const Glib::ustring& dir_main = "", Engine::auxdb_mode_t auxdbmode = Engine::auxdb_prefs, const Glib::ustring& dir_aux = "",
		   unsigned int coordcomp = 1);
	~AirwayDump();
	std::ostream& print(std::ostream& os, const Point& pt);
	std::ostream& dump_airway(std::ostream& os, const Glib::ustring& name, bool rev = false, bool verify = false);
	std::ostream& dump_through(std::ostream& os, const Glib::ustring& name);

protected:
	Engine m_engine;
	unsigned int m_coordcomp;
	Glib::RefPtr<Engine::AirwayGraphResult> m_airway;
	Glib::RefPtr<Engine::NavaidResult> m_navaid;
	Glib::RefPtr<Engine::WaypointResult> m_waypoint;
        Glib::Mutex m_mutex;
        Glib::Cond m_cond;

        void notify(void);
	void wait(void);
	void cancel(void);
	void query_point(const Glib::ustring& name);
	void query_airway(const Glib::ustring& name);
	void query_airway_text(const Glib::ustring& name);
};

AirwayDump::AirwayDump(const Glib::ustring& dir_main, Engine::auxdb_mode_t auxdbmode, const Glib::ustring& dir_aux,
		   unsigned int coordcomp)
	: m_engine(dir_main, auxdbmode, dir_aux, false, false), m_coordcomp(coordcomp)
{
}

AirwayDump::~AirwayDump()
{
	cancel();
}

void AirwayDump::notify(void)
{
        m_cond.broadcast();
}

void AirwayDump::wait(void)
{
        Glib::Mutex::Lock lock(m_mutex);
        for (;;) {
                bool ok(true);
                if (m_airway && !m_airway->is_done() && !m_airway->is_error())
                        ok = false;
                if (ok && m_navaid && !m_navaid->is_done() && !m_navaid->is_error())
                        ok = false;
                if (ok && m_waypoint && !m_waypoint->is_done() && !m_waypoint->is_error())
                        ok = false;
                if (ok)
                        break;
                m_cond.wait(m_mutex);
        }
}

void AirwayDump::cancel(void)
{
        if (m_airway) {
                Glib::RefPtr<Engine::AirwayGraphResult> q(m_airway);
                m_airway.reset();
                Engine::AirwayGraphResult::cancel(q);
        }
        if (m_navaid) {
                Glib::RefPtr<Engine::NavaidResult> q(m_navaid);
                m_navaid.reset();
                Engine::NavaidResult::cancel(q);
        }
        if (m_waypoint) {
                Glib::RefPtr<Engine::WaypointResult> q(m_waypoint);
                m_waypoint.reset();
                Engine::WaypointResult::cancel(q);
        }
}

void AirwayDump::query_point(const Glib::ustring& name)
{
        cancel();
	m_navaid = m_engine.async_navaid_find_by_icao(name, ~0U, 0, NavaidsDb::comp_exact, NavaidsDb::element_t::subtables_none);
	if (m_navaid)
		m_navaid->connect(sigc::mem_fun(*this, &AirwayDump::notify));
	m_waypoint = m_engine.async_waypoint_find_by_name(name, ~0U, 0, WaypointsDb::comp_exact, WaypointsDb::element_t::subtables_none);
	if (m_waypoint)
		m_waypoint->connect(sigc::mem_fun(*this, &AirwayDump::notify));
}

void AirwayDump::query_airway(const Glib::ustring& name)
{
        cancel();
	m_airway = m_engine.async_airway_graph_find_by_name(name, ~0U, 0, AirwaysDb::comp_contains, AirwaysDb::element_t::subtables_none);
	if (m_airway)
		m_airway->connect(sigc::mem_fun(*this, &AirwayDump::notify));
}

void AirwayDump::query_airway_text(const Glib::ustring& name)
{
        cancel();
	m_airway = m_engine.async_airway_graph_find_by_text(name, ~0U, 0, AirwaysDb::comp_contains, AirwaysDb::element_t::subtables_none);
	if (m_airway)
		m_airway->connect(sigc::mem_fun(*this, &AirwayDump::notify));
}

std::ostream& AirwayDump::print(std::ostream& os, const Point& pt)
{
	double lat(pt.get_lat_deg_dbl());
	double lon(pt.get_lon_deg_dbl());
	char lats(lat < 0 ? 'S' : 'N');
	char lons(lon < 0 ? 'W' : 'E');
	lat = fabs(lat);
	lon = fabs(lon);
	switch (m_coordcomp) {
	case 0:
	{
		std::ostringstream oss;
		oss << lats << std::fixed << std::setprecision(5) << std::setw(8) << std::setfill('0') << lat << ' '
		    << lons << std::fixed << std::setprecision(5) << std::setw(9) << std::setfill('0') << lon;
		return os << oss.str();
	}

	case 1:
	{
		int latd(floor(lat));
		lat -= latd;
		lat *= 60;
		int lond(floor(lon));
		lon -= lond;
		lon *= 60;
		std::ostringstream oss;
		oss << lats << std::setw(2) << std::setfill('0') << latd << ' '
		    << std::fixed << std::setprecision(3) << std::setw(6) << std::setfill('0') << lat << ' '
		    << lons << std::setw(3) << std::setfill('0') << lond << ' '
		    << std::fixed << std::setprecision(3) << std::setw(6) << std::setfill('0') << lon;
		return os << oss.str();
	}

	default:
	{
		int latd(floor(lat));
		lat -= latd;
		lat *= 60;
		int latm(floor(lat));
		lat -= latm;
		lat *= 60;
		int lond(floor(lon));
		lon -= lond;
		lon *= 60;
		int lonm(floor(lon));
		lon -= lonm;
		lon *= 60;
		std::ostringstream oss;
		oss << lats << std::setw(2) << std::setfill('0') << latd << ' '
		    << std::setw(2) << std::setfill('0') << latm << ' '
		    << std::fixed << std::setprecision(1) << std::setw(4) << std::setfill('0') << lat << ' '
		    << lons << std::setw(3) << std::setfill('0') << lond << ' '
		    << std::setw(2) << std::setfill('0') << lonm << ' '
		    << std::fixed << std::setprecision(1) << std::setw(4) << std::setfill('0') << lon;
		return os << oss.str();
	}
	}
}

std::ostream& AirwayDump::dump_airway(std::ostream& os, const Glib::ustring& name, bool rev, bool verify)
{
	query_airway(name);
	wait();
	if (!m_airway || !m_airway->is_done() || m_airway->is_error())
		return os << "Cannot load airway " << name << std::endl;
	Glib::ustring nameu(name.uppercase());
	typedef Engine::AirwayGraphResult::Graph Graph;
	Graph g(m_airway->get_result());
	cancel();
	typedef std::set<Graph::vertex_descriptor> vertices_t;
	vertices_t vertices(g.find_airway_begin(nameu));
	// find end vertices
	if (vertices.empty()) {
		return os << "Cannot find starting point of airway " << name << std::endl;
	}
	vertices_t verticesdone;
	while (!vertices.empty()) {
		Graph::vertex_descriptor v0;
		{
			std::multimap<Glib::ustring, Graph::vertex_descriptor> sorted;
			for (vertices_t::const_iterator vi(vertices.begin()), ve(vertices.end()); vi != ve; ++vi)
				sorted.insert(std::make_pair(g[*vi].get_ident(), *vi));
			if (rev)
				v0 = (--sorted.end())->second;
			else
				v0 = (sorted.begin())->second;
		}
		for (;;) {
			verticesdone.insert(v0);
			{
				vertices_t::iterator i(vertices.find(v0));
				if (i != vertices.end())
					vertices.erase(i);
			}
			Graph::edge_descriptor e;
			unsigned int ecnt(0);
			{
				Graph::out_edge_iterator e0, e1;
				boost::tie(e0, e1) = boost::out_edges(v0, g);
				for (; e0 != e1; ++e0) {
					if (g[*e0].get_ident() != nameu)
						continue;
					Graph::vertex_descriptor vx(boost::target(*e0, g));
					if (verticesdone.find(vx) != verticesdone.end())
						continue;
					++ecnt;
					e = *e0;
				}
			}
			switch (ecnt) {
			case 0:
			{
				const Engine::AirwayGraphResult::Vertex& vv(g[v0]);
				print(os << std::setw(8) << std::left << nameu << ' '
				      << std::setw(6) << vv.get_ident() << std::right << ' ', vv.get_coord())
					<< std::endl << std::endl;
				break;
			}

			default:
				os << "WARNING: fork" << std::endl;
				// fall through

			case 1:
			{
				const Engine::AirwayGraphResult::Vertex& vv(g[v0]);
				const Engine::AirwayGraphResult::Edge& ee(g[e]);
				print(os << std::setw(8) << std::left << nameu << ' '
				      << std::setw(6) << vv.get_ident() << std::right << ' ', vv.get_coord())
					<< ' ' << std::setw(3) << ee.get_base_level()
					<< ' ' << std::setw(3) << ee.get_top_level()
					<< ' ' << std::fixed << std::setprecision(1) << std::setw(5) << ee.get_dist()
					<< ' ' << std::setw(10) << std::left << DbBaseElements::Airway::get_type_name(ee.get_type())
					<< ' ' << ee.get_ident() << std::right << std::endl;
				break;
			}
			}
			if (verify) {
				query_point(g[v0].get_ident());
				wait();
				bool found(false);
				Point pt(g[v0].get_coord());
				if (m_navaid && m_navaid->is_done() && !m_navaid->is_error()) {
					for (Engine::NavaidResult::elementvector_t::const_iterator ei(m_navaid->get_result().begin()), ee(m_navaid->get_result().end()); ei != ee; ++ei) {
						const Engine::NavaidResult::element_t& el(*ei);
						if (el.get_coord().simple_distance_nmi(pt) > 0.001)
							continue;
						found = true;
						break;
					}
				}
				if (!found && m_waypoint && m_waypoint->is_done() && !m_waypoint->is_error()) {
					for (Engine::WaypointResult::elementvector_t::const_iterator ei(m_waypoint->get_result().begin()), ee(m_waypoint->get_result().end()); ei != ee; ++ei) {
						const Engine::WaypointResult::element_t& el(*ei);
						if (el.get_coord().simple_distance_nmi(pt) > 0.001)
							continue;
						found = true;
						break;
					}
				}
				if (!found)
					std::cerr << "NO SAMENAME CLOSEBY NAVAID OR INTERSECTION FOUND" << std::endl;
			}
			if (!ecnt)
				break;
			v0 = boost::target(e, g);
		}
	}
	return os;
}

std::ostream& AirwayDump::dump_through(std::ostream& os, const Glib::ustring& name)
{
	query_airway_text(name);
	wait();
	if (!m_airway || !m_airway->is_done() || m_airway->is_error())
		return os << "Cannot load airway " << name << std::endl;
	Glib::ustring nameu(name.uppercase());
	typedef Engine::AirwayGraphResult::Graph Graph;
	Graph& g(m_airway->get_result());
	Graph::vertex_iterator v0, v1;
	boost::tie(v0, v1) = boost::vertices(g);
	for (; v0 != v1; ++v0) {
		const Engine::AirwayGraphResult::Vertex& vv(g[*v0]);
		if (vv.get_ident() != nameu)
			continue;
		print(os << std::left << std::setw(6) << vv.get_ident() << std::right << ' ', vv.get_coord()) << ' ';
		Graph::out_edge_iterator e0, e1;
		boost::tie(e0, e1) = boost::out_edges(*v0, g);
		bool subseq(false);
		for (; e0 != e1; ++e0) {
			if (subseq)
				os << ',';
			subseq = true;
			os << g[*e0].get_ident();
		}
		if (!subseq)
			os << "none";
		os << std::endl;
	}
	return os;
}

/* ---------------------------------------------------------------------- */

class TextParser {
public:
	TextParser(void);
	void parse(std::istream& is);
	std::ostream& print(std::ostream& os);

protected:
	typedef std::vector<AirwaysDb::Airway> awy_t;
	awy_t m_awy;

	struct begincomp {
		bool operator()(const AirwaysDb::Airway& a, const AirwaysDb::Airway& b) { return a.get_begin_name() < b.get_begin_name(); }
	};
};

TextParser::TextParser(void)
{
}

void TextParser::parse(std::istream& is)
{
	typedef enum {
		ps_waitid,
		ps_id,
		ps_waitcoord,
		ps_coord1,
		ps_coord1sgn,
		ps_coord2,
		ps_coord2sgn,
		ps_endline
	} ps_t;
	while (is.good()) {
		std::string line;
		getline(is, line);
		ps_t ps(ps_waitid);
		std::string ident, coord1, coord2;
		for (std::string::const_iterator si(line.begin()), se(line.end()); si != se; ++si) {
			switch (ps) {
			case ps_waitid:
				if (std::isalnum(*si)) {
					ps = ps_id;
					ident += *si;
					break;
				}
				break;

			case ps_id:
				if (std::isspace(*si)) {
					ps = ps_waitcoord;
					break;
				}
				ident += *si;
				break;

			case ps_waitcoord:
				if (std::isdigit(*si) || *si == '.') {
					ps = ps_coord1;
					coord1 += *si;
					break;
				}
				if (*si == 'n' || *si == 's' || *si == 'e' || *si == 'w' ||
				    *si == 'N' || *si == 'S' || *si == 'E' || *si == 'W') {
					ps = ps_coord1sgn;
					coord1 += *si;
					break;
				}
				break;

			case ps_coord1:
				if (*si == 'n' || *si == 's' || *si == 'e' || *si == 'w' ||
				    *si == 'N' || *si == 'S' || *si == 'E' || *si == 'W') {
					coord1 = *si + coord1;
					ps = ps_coord2;
					break;
				}
				coord1 += *si;
				break;

			case ps_coord2:
				if (*si == 'n' || *si == 's' || *si == 'e' || *si == 'w' ||
				    *si == 'N' || *si == 'S' || *si == 'E' || *si == 'W') {
					coord2 = *si + coord2;
					ps = ps_endline;
					break;
				}
				coord2 += *si;
				break;

			case ps_coord1sgn:
				if (*si == 'n' || *si == 's' || *si == 'e' || *si == 'w' ||
				    *si == 'N' || *si == 'S' || *si == 'E' || *si == 'W') {
					coord2 += *si;
					ps = ps_coord2sgn;
					break;
				}
				coord1 += *si;
				break;

			case ps_coord2sgn:
				if (std::isspace(*si) || std::isdigit(*si) || *si == '.') {
					coord2 += *si;
					break;
				}
				ps = ps_endline;

				/* fall through */
			case ps_endline:
			default:
				if (std::isspace(*si))
					continue;
				std::cerr << "Warning: skipping characters at line end: " << std::string(si, se);
				si = se;
				break;
			}
			if (si == se)
				break;
		}
		if (ps == ps_waitid)
			continue;
		if (ps != ps_endline && ps != ps_coord2sgn) {
			std::cerr << "Cannot parse line: " << line << std::endl;
			continue;
		}
		AirwaysDb::Airway awy;
		awy.set_begin_name(ident);
		Point pt;
		unsigned int r(pt.set_str(coord1 + " " + coord2));
		if ((Point::setstr_lat | Point::setstr_lon) & ~r) {
			std::cerr << "Cannot parse coordinates: " << coord1 << ' ' << coord2 << std::endl;
			continue;
		}
		awy.set_begin_coord(pt);
		m_awy.push_back(awy);
	}
}

std::ostream& TextParser::print(std::ostream& os)
{
	if (m_awy.empty())
		return os;
	if (m_awy.size() == 1) {
		os << std::fixed << std::setw(10) << std::setprecision(6) << m_awy.front().get_begin_coord().get_lat_deg_dbl()
		   << ' ' << std::fixed << std::setw(11) << std::setprecision(6) << m_awy.front().get_begin_coord().get_lon_deg_dbl()
		   << ' ' << m_awy.front().get_begin_name() << std::endl;
		m_awy.clear();
		return os;
	}
	for (awy_t::iterator ai(m_awy.begin()), ae(m_awy.end()); ai != ae; ) {
		awy_t::iterator ai1(ai);
		++ai;
		if (ai == ae) {
			m_awy.erase(ai1);
			break;
		}
		ai1->set_end_name(ai->get_begin_name());
		ai1->set_end_coord(ai->get_begin_coord());
		if (ai1->get_begin_name() > ai1->get_end_name())
			ai1->swap_begin_end();
	}
	std::sort(m_awy.begin(), m_awy.end(), begincomp());
	for (awy_t::const_iterator ai(m_awy.begin()), ae(m_awy.end()); ai != ae; ++ai) {
		os << std::left << std::setw(6) << ai->get_begin_name() << std::right
		   << std::fixed << std::setw(10) << std::setprecision(6) << ai->get_begin_coord().get_lat_deg_dbl()
		   << ' ' << std::fixed << std::setw(11) << std::setprecision(6) << ai->get_begin_coord().get_lon_deg_dbl()
		   << ' ' << std::left << std::setw(6) << ai->get_end_name() << std::right
		   << std::fixed << std::setw(10) << std::setprecision(6) << ai->get_end_coord().get_lat_deg_dbl()
		   << ' ' << std::fixed << std::setw(11) << std::setprecision(6) << ai->get_end_coord().get_lon_deg_dbl()
		   << " 1 XXX XXX Z" << std::endl;
	}
	m_awy.clear();
	return os;
}

/* ---------------------------------------------------------------------- */

int main(int argc, char *argv[])
{
        static struct option long_options[] = {
                { "maindir", required_argument, 0, 'm' },
                { "auxdir", required_argument, 0, 'a' },
                { "disableaux", no_argument, 0, 'A' },
                { "reverse", no_argument, 0, 'r' },
                { "verify", no_argument, 0, 'v' },
                { "through", no_argument, 0, 't' },
                { "parsetext", no_argument, 0, 'p' },
                {0, 0, 0, 0}
        };
	Glib::init();
	Gio::init();
        Glib::ustring dir_main(""), dir_aux("");
	Engine::auxdb_mode_t auxdbmode(Engine::auxdb_prefs);
	bool dis_aux(false), rev(false), verify(false), through(false), parsetxt(false);
	unsigned int coordmode(1);
        int c, err(0);

        while ((c = getopt_long(argc, argv, "m:a:Axsrvtp", long_options, 0)) != EOF) {
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

		case 'x':
			coordmode = 0;
			break;

		case 's':
			coordmode = 2;
			break;

		case 'r':
			rev = true;
			break;

		case 'v':
			verify = true;
			break;

		case 't':
			through = true;
			break;

		case 'p':
			parsetxt = true;
			break;

		default:
			err++;
			break;
                }
        }
        if (err) {
                std::cerr << "usage: vfrdbairwaydump [-m <maindir>] [-a <auxdir>] [-A] [-x] [-s] [-r] [-v] [-t]" << std::endl;
                return EX_USAGE;
        }
	if (parsetxt) {
		TextParser p;
		p.parse(std::cin);
		p.print(std::cout);
		return EX_OK;
	}
	if (dis_aux)
		auxdbmode = Engine::auxdb_none;
	else if (!dir_aux.empty())
		auxdbmode = Engine::auxdb_override;
        try {
		AirwayDump dump(dir_main, auxdbmode, dir_aux, coordmode);
		for (int i = optind; i < argc; ++i) {
			if (through)
				dump.dump_through(std::cout, argv[i]);
			else
				dump.dump_airway(std::cout, argv[i], rev, verify);
		}
        } catch (const Glib::Exception& ex) {
                std::cerr << "Glib exception: " << ex.what() << std::endl;
                return EX_DATAERR;
        } catch (const std::exception& ex) {
                std::cerr << "exception: " << ex.what() << std::endl;
                return EX_DATAERR;
        }
        return EX_OK;
}
