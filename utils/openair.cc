/*
 * openair.cc:  openair to DB conversion utility
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
 * The openair format is documented here:
 * http://www.winpilot.com/UsersGuide/UserAirspace.asp
 *
 * Example files can be obtained here:
 * http://soaringweb.org/Airspace/CH/ch_201103.txt
 * http://www.daec.de/fachbereiche/luftraum-flugbetrieb/luftraumdaten/
 */

#include "sysdeps.h"

#include <getopt.h>
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


/* ---------------------------------------------------------------------- */

class OpenAirParser {
public:
	OpenAirParser(const Glib::ustring& output_dir = ".", bool purgedb = false);
	~OpenAirParser();
	void parse_file(const Glib::ustring& fn, const Glib::ustring& srcid);

protected:
	Glib::RefPtr<Glib::IOChannel> m_chan;
	Glib::IOStatus m_status;
	time_t m_filetime;
	Glib::ustring m_outputdir;
	bool m_purgedb;
	bool m_airspacesdbopen;
	AirspacesDb m_airspacesdb;
	AirspacesDb::Airspace m_airspace;

	bool getline_raw(Glib::ustring& ln);
	static void trim(Glib::ustring& s);
	static void removectrlchars(Glib::ustring& s);
	void open(const Glib::ustring& fn);
	void close(void);
	time_t get_filetime(void) const { return m_filetime; }
	operator bool(void) const;
	bool getline(Glib::ustring& ln);
	Glib::ustring extractcmd(Glib::ustring& ln);
	bool getcoord(Point& pt, const Glib::ustring& ln);
	bool getalt(int32_t& alt, uint8_t& flags, Glib::ustring ln);
	typedef std::vector<Glib::ustring> token_t;
	token_t gettoken(const Glib::ustring& ln);
	Glib::ustring getstring(const token_t& tok);
	void save_airspace(void);
};

OpenAirParser::OpenAirParser(const Glib::ustring& output_dir, bool purgedb)
        : m_status(Glib::IO_STATUS_ERROR), m_filetime(0), m_outputdir(output_dir), m_purgedb(purgedb),
          m_airspacesdbopen(false)
{
}

OpenAirParser::~OpenAirParser()
{
	save_airspace();
        if (m_airspacesdbopen) {
                m_airspacesdb.analyze();
                m_airspacesdb.vacuum();
                m_airspacesdb.close();
        }
        m_airspacesdbopen = false;
}

void OpenAirParser::open(const Glib::ustring& fn)
{
        m_status = Glib::IO_STATUS_ERROR;
#ifdef GLIBMM_EXCEPTIONS_ENABLED
        m_chan = Glib::IOChannel::create_from_file(fn, "r");
        m_chan->set_encoding("ISO8859-1");
#else
        std::auto_ptr<Glib::Error> ex;
        m_chan = Glib::IOChannel::create_from_file(fn, "r", ex);
        if (ex.get())
                throw *ex;
        m_chan->set_encoding("ISO8859-1", ex);
        if (ex.get())
                throw *ex;
#endif
        m_filetime = time(0);
        try {
                Glib::RefPtr<Gio::File> df(Gio::File::create_for_path(fn));
		Glib::RefPtr<Gio::FileInfo> fi(df->query_info(G_FILE_ATTRIBUTE_TIME_MODIFIED, Gio::FILE_QUERY_INFO_NONE));
                m_filetime = fi->modification_time().tv_sec;
        } catch (const Gio::Error& e) {
                std::cerr << "cannot get modified time of file " << fn << "(" << e.what() << "), using current time" << std::endl;
        }
}

void OpenAirParser::close(void)
{
        m_status = Glib::IO_STATUS_ERROR;
        m_chan.clear();
}

OpenAirParser::operator bool(void) const
{
        return m_status == Glib::IO_STATUS_NORMAL;
}

void OpenAirParser::trim(Glib::ustring & s)
{
        while (!s.empty() && Glib::Unicode::isspace(*s.begin()))
                s.erase(s.begin());
        while (!s.empty() && Glib::Unicode::isspace(*--s.end()))
                s.erase(--s.end());
}

void OpenAirParser::removectrlchars(Glib::ustring& s)
{
	for (Glib::ustring::iterator i(s.begin()); i != s.end(); ) {
		if (*i >= ' ') {
			++i;
			continue;
		}
		i = s.erase(i);
	}
}

bool OpenAirParser::getline_raw(Glib::ustring& ln)
{
        ln.clear();
        if (!m_chan) {
                m_status = Glib::IO_STATUS_ERROR;
                return false;
        }
#ifdef GLIBMM_EXCEPTIONS_ENABLED
        m_status = m_chan->read_line(ln);
#else
        std::auto_ptr<Glib::Error> ex;
        m_status = m_chan->read_line(ln, ex);
        if (ex.get())
                throw *ex;
#endif
        if (m_status != Glib::IO_STATUS_NORMAL)
                return false;
        return true;
}

bool OpenAirParser::getline(Glib::ustring& ln)
{
	for (;;) {
		if (!getline_raw(ln))
			return false;
		trim(ln);
		if (ln.empty() || ln[0] == '*')
			continue;
		removectrlchars(ln);
		return true;
	}
}

Glib::ustring OpenAirParser::extractcmd(Glib::ustring& ln)
{
	Glib::ustring::iterator i(ln.begin()), b(i), e(ln.end());
	while (i != e && std::isalpha(*i))
		++i;
	Glib::ustring ret(b, i);
	ln.erase(b, i);
	return ret;
}

bool OpenAirParser::getcoord(Point& pt, const Glib::ustring& ln)
{
	pt = Point(0, 0);
	std::string x(ln);
	{
		for (std::string::iterator i(x.begin()), e(x.end()); i != e; ++i)
			if (*i == ':')
				(*i) = ' ';
	}
	return !((Point::setstr_lat | Point::setstr_lon) & ~pt.set_str(x));
}

bool OpenAirParser::getalt(int32_t& alt, uint8_t& flags, Glib::ustring ln)
{
	alt = 0;
	flags = 0;
	trim(ln);
	if (ln == "GND" || ln == "MSL") {
		flags |= DbBaseElements::Airspace::altflag_gnd;
		return true;
	}
	if (ln == "SFC") {
		flags |= DbBaseElements::Airspace::altflag_sfc;
		return true;
	}
	if (ln == "UNLIM") {
		flags |= DbBaseElements::Airspace::altflag_unlim;
		return true;
	}
	if (ln == "NOTAM") {
		flags |= DbBaseElements::Airspace::altflag_notam;
		return true;
	}
	if (ln == "UNKNOWN") {
		flags |= DbBaseElements::Airspace::altflag_unkn;
		return true;
	}
	if (ln.substr(0, 2) == "FL") {
		flags |= DbBaseElements::Airspace::altflag_fl;
		ln.erase(0, 2);
		trim(ln);
	}
	{
		Glib::ustring::iterator b(ln.begin()), e(ln.end()), i(b);
		while (i != e && std::isdigit(*i))
			++i;
		if (i == b)
			return false;
		std::istringstream iss(Glib::ustring(b, i));
		iss >> alt;
		if (flags & DbBaseElements::Airspace::altflag_fl)
			alt *= 100;
		ln.erase(b, i);
		trim(ln);
	}
	if (ln == "AGL") {
		flags |= DbBaseElements::Airspace::altflag_agl;
		return true;
	}
	if (ln == "MSL")
		return true;
	return false;
}

OpenAirParser::token_t OpenAirParser::gettoken(const Glib::ustring& ln)
{
	token_t r;
	Glib::ustring::size_type st(0);
	for (;;) {
		Glib::ustring::size_type n(ln.find(',', st));
		if (n == Glib::ustring::npos)
			break;
		r.push_back(ln.substr(st, n - st));
		st = n + 1;
	}
	r.push_back(ln.substr(st));
	for (token_t::iterator i(r.begin()), e(r.end()); i != e; ++i)
		trim(*i);
	return r;
}

Glib::ustring OpenAirParser::getstring(const token_t& tok)
{
	Glib::ustring r;
	bool subseq(false);
	for (token_t::const_iterator i(tok.begin()), e(tok.end()); i != e; ++i) {
		if (subseq)
			r += ",";
		subseq = true;
		r += *i;
	}
	return r;
}

void OpenAirParser::parse_file(const Glib::ustring& fn, const Glib::ustring& srcid)
{
        close();
        open(fn);
        if (!m_airspacesdbopen) {
                m_airspacesdb.open(m_outputdir);
                m_airspacesdb.sync_off();
                if (m_purgedb)
                        m_airspacesdb.purgedb();
                m_airspacesdbopen = true;
        }
	save_airspace();
	bool skip(false);
	unsigned int reccnt(0);
	bool dirccw(false);
	Point center;
	double width(0);
	double zoom(0);
	for (;;) {
		Glib::ustring line;
		if (!getline(line))
			break;
		Glib::ustring cmd(extractcmd(line));
		if (cmd.empty()) {
			std::cerr << "Line " << line << " has no command" << std::endl;
			continue;
		}
		// skip terrain
		if (cmd == "TO" || cmd == "TC") {
			if (m_airspace.is_valid())
				++reccnt;
			save_airspace();
			skip = true;
			continue;
		}
		if (cmd == "AC") {
			if (m_airspace.is_valid())
				++reccnt;
			save_airspace();
			trim(line);
			skip = false;
			{
				std::ostringstream oss;
				oss << srcid << '_' << reccnt << "@openair";
				m_airspace.set_sourceid(oss.str());
			}
			m_airspace.set_modtime(get_filetime());
			if (line.size() == 1 && line[0] >= 'A' && line[0] <= 'G') {
				m_airspace.set_bdryclass(line[0]);
				m_airspace.set_typecode('A');
				continue;
			}
			if (line == "R" || line == "P") {
				m_airspace.set_bdryclass(line[0]);
				m_airspace.set_typecode(255);
				continue;
			}
			if (line == "Q") {
				m_airspace.set_bdryclass('D');
				m_airspace.set_typecode(255);
				continue;
			}
			if (line == "CTR") {
				// guess!
				m_airspace.set_bdryclass('D');
				m_airspace.set_typecode('A');
				continue;
			}
			if (line != "GP" && line != "W")
				std::cerr << "AC: Unknown airspace class " << line << ", skipping" << std::endl;
			skip = true;
			continue;
		}
		if (skip)
			continue;
		if (cmd == "AN") {
			// check if there is a frequency at the end
			{
				Glib::ustring::iterator b(line.begin()), e(line.end()), i(e);
				while (i != b) {
					--i;
					if (std::isdigit(*i) || *i == '.')
						continue;
					++i;
					break;
				}
				double v(Glib::Ascii::strtod(Glib::ustring(i, e)));
				if (v >= 118.0 && v < 138.0) {
					m_airspace.set_commfreq(0, Point::round<uint32_t,double>(v * 1e6));
					line.erase(i, e);
					trim(line);
					if (!line.empty() && line[line.size()-1] == ':') {
						line.erase(line.size() - 1);
						trim(line);
					}
				}
			}
			m_airspace.set_ident(line);
			continue;
		}
		if (cmd == "AH") {
			int32_t alt(0);
			uint8_t flags(0);
			if (!getalt(alt, flags, line)) {
				std::cerr << "AH: cannot parse ceiling altitude " << line << std::endl;
				continue;
			}
			m_airspace.set_altupr(alt);
			m_airspace.set_altuprflags(flags);
			continue;
		}
		if (cmd == "AL") {
			int32_t alt(0);
			uint8_t flags(0);
			if (!getalt(alt, flags, line)) {
				std::cerr << "AL: cannot parse floor altitude " << line << std::endl;
				continue;
			}
			m_airspace.set_altlwr(alt);
			m_airspace.set_altlwrflags(flags);
			continue;
		}
		if (cmd == "AT") {
			Point pt;
			if (!getcoord(pt, line)) {
				std::cerr << "AT: cannot parse label coordinate " << line << std::endl;
				continue;
			}
			m_airspace.set_labelcoord(pt);
			continue;
		}
		if (cmd == "SB" || cmd == "SP")
			continue;
		if (cmd == "V") {
			Glib::ustring var(line.substr(0, 2));
			line.erase(0, 2);
			trim(line);
			if (var == "D=") {
				if (line == "+") {
					dirccw = false;
					continue;
				}
				if (line == "-") {
					dirccw = true;
					continue;
				}
			} else if (var == "X=") {
				if (getcoord(center, line))
					continue;
			} else if (var == "W=") {
				width = Glib::Ascii::strtod(line);
				continue;
			} else if (var == "Z=") {
				zoom = Glib::Ascii::strtod(line);
				continue;
			}
			std::cerr << "V: cannot parse variable assignment " << var << line << std::endl;
			continue;
		}
		if (cmd == "DC") {
			double radius(Glib::Ascii::strtod(line));
			if (radius < 0.001) {
				std::cerr << "DC: cannot parse circle radius " << line << std::endl;
				continue;
			}
			int32_t rad(Point::round<int32_t,double>(radius * (1.0 / 60) * Point::from_rad_dbl));
			AirspacesDb::Airspace::Segment seg(Point(), Point(), center, 'C', rad, rad);
			m_airspace.add_segment(seg);
			continue;
		}
		if (cmd == "DP") {
			Point pt;
			if (!getcoord(pt, line)) {
				std::cerr << "DP: cannot parse coordinate " << line << std::endl;
				continue;
			}
			AirspacesDb::Airspace::Segment seg(Point(), pt, Point(), '-', 0, 0);
			if (m_airspace.get_nrsegments() > 0)
				seg.set_coord1(m_airspace[m_airspace.get_nrsegments()-1].get_coord2());
			m_airspace.add_segment(seg);
			continue;
		}
		if (cmd == "DA") {
			token_t tok(gettoken(line));
			if (tok.size() != 3) {
				std::cerr << "DA: invalid format " << getstring(tok) << std::endl;
				continue;
			}
			double radius(Glib::Ascii::strtod(tok[0]));
			double ang1(Glib::Ascii::strtod(tok[1]));
			double ang2(Glib::Ascii::strtod(tok[2]));
			int32_t rad(Point::round<int32_t,double>(radius * (1.0 / 60) * Point::from_rad_dbl));
			AirspacesDb::Airspace::Segment seg(center.spheric_course_distance_nmi(ang1, radius),
							   center.spheric_course_distance_nmi(ang2, radius),
							   center, dirccw ? 'L' : 'R', rad, rad);
			m_airspace.add_segment(seg);
			continue;
		}
		if (cmd == "DB") {
			token_t tok(gettoken(line));
			if (tok.size() != 2) {
				std::cerr << "DB: invalid format " << getstring(tok) << std::endl;
				continue;
			}
			Point pt1, pt2;
			if (!getcoord(pt1, tok[0]) || !getcoord(pt2, tok[1])) {
				std::cerr << "DB: cannot parse coordinates " << getstring(tok) << std::endl;
				continue;
			}
			double radius1(center.spheric_distance_nmi(pt1));
			double radius2(center.spheric_distance_nmi(pt2));
			int32_t rad1(Point::round<int32_t,double>(radius1 * (1.0 / 60) * Point::from_rad_dbl));
			int32_t rad2(Point::round<int32_t,double>(radius2 * (1.0 / 60) * Point::from_rad_dbl));
			AirspacesDb::Airspace::Segment seg(pt1, pt2, center, dirccw ? 'L' : 'R', rad1, rad2);
			m_airspace.add_segment(seg);
			continue;
		}
		if (cmd == "DY")
			continue;
		std::cerr << "Unknown command " << cmd << " line " << line << ", skipping" << std::endl;
	}
        close();
	save_airspace();
}

void OpenAirParser::save_airspace(void)
{
	if (!m_airspace.is_valid())
		return;
	// close the airspace
	if (m_airspace.get_nrsegments() > 0)
		m_airspace[0].set_coord1(m_airspace[m_airspace.get_nrsegments()-1].get_coord2());
	m_airspace.recompute_lineapprox(m_airspacesdb);
	m_airspace.recompute_bbox();
        m_airspacesdb.save(m_airspace);
	m_airspace = AirspacesDb::Airspace();
}

/* ---------------------------------------------------------------------- */


/* ---------------------------------------------------------------------- */

int main(int argc, char *argv[])
{
        static struct option long_options[] = {
                { "dir", required_argument, 0, 'd' },
                { "purgedb", no_argument, 0, 'p' },
                { "srcid", required_argument, 0, 's' },
                { "sourceid", required_argument, 0, 's' },
                {0, 0, 0, 0}
        };
	Glib::init();
	Gio::init();
        Glib::ustring db_dir(".");
        Glib::ustring srcid("oa");
        bool purgedb(false);
        int c, err(0);

        while ((c = getopt_long(argc, argv, "d:ps:", long_options, 0)) != EOF) {
                switch (c) {
                        case 'd':
                                if (optarg)
                                        db_dir = optarg;
                                break;

                        case 'p':
                                purgedb = true;
                                break;

                        case 's':
                                if (optarg)
                                        srcid = optarg;
                                break;

                        default:
                                err++;
                                break;
                }
        }
        if (err) {
                std::cerr << "usage: vfrdbopenair [-d <dir>]" << std::endl;
                return EX_USAGE;
        }
        try {
                OpenAirParser parser(db_dir, purgedb);
                for (; optind < argc; optind++) {
                        std::cerr << "Parsing file " << argv[optind] << std::endl;
                        parser.parse_file(argv[optind], srcid);
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
