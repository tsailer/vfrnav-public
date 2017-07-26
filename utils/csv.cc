/*
 * csv.cc:  cvs (Excel) to DB conversion utility
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
#include <iostream>
#include <sstream>
#include <limits>
#include <sqlite3x.hpp>
#include <sqlite3.h>
#include <unistd.h>
#include <sys/stat.h>
#include <stdexcept>
#include <sysexits.h>

#include "geom.h"
#include "dbobj.h"


/* ---------------------------------------------------------------------- */

class CSVParser {
        public:
                CSVParser(const Glib::ustring& output_dir = ".", const Glib::ustring& srcid = "@CSV", bool purgedb = false);
                ~CSVParser();
                void parse_file(const Glib::ustring& fn);

        protected:
                Glib::RefPtr<Glib::IOChannel> m_chan;
                Glib::IOStatus m_status;
                time_t m_filetime;
                std::vector<Glib::ustring> m_fld;
                Glib::ustring m_outputdir;
                Glib::ustring m_srcid;
                bool m_purgedb;
                bool m_airportsdbopen;
                AirportsDb m_airportsdb;
                bool m_topodbopen;
                TopoDb30 m_topodb;

                bool checkheader(void);
                bool getline(Glib::ustring& ln);
                static void trim(Glib::ustring& s);
                void open(const Glib::ustring& fn);
                void close(void);
                time_t get_filetime(void) const { return m_filetime; }
                operator bool(void) const;
                bool nextrec(void);
                unsigned int size(void) const { return m_fld.size(); }
                const Glib::ustring& get_string(unsigned int x) const { return m_fld[x]; }
                double get_double(unsigned int x) const;
                unsigned int get_uint(unsigned int x) const;
                int get_int(unsigned int x) const;
                Point get_point(unsigned int lonidx, unsigned int latidx) const;
                unsigned int get_frequency(unsigned int x) const;
                void parse_airport(void);
};

CSVParser::CSVParser(const Glib::ustring& output_dir, const Glib::ustring& srcid, bool purgedb)
        : m_status(Glib::IO_STATUS_ERROR), m_filetime(0), m_outputdir(output_dir), m_srcid(srcid), m_purgedb(purgedb),
          m_airportsdbopen(false), m_topodbopen(false)
{
}

CSVParser::~CSVParser()
{
        if (m_airportsdbopen) {
                m_airportsdb.analyze();
                m_airportsdb.vacuum();
                m_airportsdb.close();
        }
        m_airportsdbopen = false;
        if (m_topodbopen)
                m_topodb.close();
        m_topodbopen = false;
}

void CSVParser::open(const Glib::ustring& fn)
{
        m_fld.clear();
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
        {
                struct stat st;
                if (stat(fn.c_str(), &st))
                        std::cerr << "cannot get modified time of file " << fn << ", using current time" << std::endl;
                else
                        m_filetime = st.st_mtime;
        }
        // read header record
        if (!checkheader())
                throw std::runtime_error("CSVParser: cannot read header");
}

void CSVParser::close(void)
{
        m_fld.clear();
        m_status = Glib::IO_STATUS_ERROR;
        m_chan.clear();
}

CSVParser::operator bool(void) const
{
        return m_status == Glib::IO_STATUS_NORMAL;
}

void CSVParser::trim(Glib::ustring & s)
{
        while (!s.empty() && Glib::Unicode::isspace(*s.begin()))
                s.erase(s.begin());
        while (!s.empty() && Glib::Unicode::isspace(*--s.end()))
                s.erase(--s.end());
}

bool CSVParser::getline(Glib::ustring& ln)
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

bool CSVParser::checkheader(void)
{
        if (!nextrec())
                return false;
        if (m_fld.size() < 15 ||
            m_fld[0].casefold() != Glib::ustring("name").casefold() ||
            m_fld[1].casefold() != Glib::ustring("icao").casefold() ||
            m_fld[2].casefold() != Glib::ustring("elev").casefold() ||
            Glib::ustring(m_fld[3], 0, 3).casefold() != Glib::ustring("lat").casefold() ||
            Glib::ustring(m_fld[6], 0, 3).casefold() != Glib::ustring("lon").casefold() ||
            Glib::ustring(m_fld[9], 0, 3).casefold() != Glib::ustring("rwy").casefold() ||
            Glib::ustring(m_fld[10], 0, 3).casefold() != Glib::ustring("rwy").casefold() ||
            m_fld[11].casefold() != Glib::ustring("length").casefold() ||
            m_fld[12].casefold() != Glib::ustring("width").casefold() ||
            m_fld[13].casefold() != Glib::ustring("surface").casefold() ||
            m_fld[14].casefold() != Glib::ustring("frequency").casefold())
                throw std::runtime_error("CSV airport file header not recognized");
        return true;
}

bool CSVParser::nextrec(void)
{
        m_fld.clear();
        Glib::ustring buf;
        do {
                if (!getline(buf))
                        return false;
                trim(buf);
        } while (buf.empty());
        {
                Glib::ustring buf1;
                bool enacomma(true);
                for (Glib::ustring::const_iterator si(buf.begin()), se(buf.end()); si != se; ++si) {
                        if (*si == '"') {
                                enacomma = !enacomma;
                                buf1.push_back(*si);
                                continue;
                        }
                        if (enacomma && *si == ',') {
                                trim(buf1);
                                if (!buf1.empty() && buf1[0] == '"') {
                                        if (buf1.size() < 2 || buf1[buf1.size()-1] != '"')
                                                throw std::runtime_error("CSV quoting error in line: " + buf);
                                        buf1.erase(0, 1);
                                        buf1.erase(buf1.size() - 1);
                                        for (Glib::ustring::size_type i(buf1.size()); i >= 2; --i) {
                                                if (buf1[i-1] != '"' || buf1[i-2] != '"')
                                                        continue;
                                                buf1.erase(i-1, 1);
                                                --i;
                                        }
                                }
                                m_fld.push_back(buf1);
                                buf1.clear();
                                continue;
                        }
                        buf1.push_back(*si);
                }
                trim(buf1);
                if (!buf1.empty()) {
                        if (!buf1.empty() && buf1[0] == '"') {
                                if (buf1.size() < 2 || buf1[buf1.size()-1] != '"')
                                        throw std::runtime_error("CSV quoting error in line: " + buf);
                                buf1.erase(0, 1);
                                buf1.erase(buf1.size() - 1);
                                for (Glib::ustring::size_type i(buf1.size()); i >= 2; --i) {
                                        if (buf1[i-1] != '"' || buf1[i-2] != '"')
                                                continue;
                                        buf1.erase(i-1, 1);
                                        --i;
                                }
                        }
                        m_fld.push_back(buf1);
                }
        }
        return true;
}

double CSVParser::get_double( unsigned int x ) const
{
        const char *f = get_string(x).c_str();
        char *cp;
        double r = strtod(f, &cp);
        if (*cp)
                throw std::runtime_error("CSVParser::get_double: invalid field \"" + get_string(x) + "\"");
        return r;
}

unsigned int CSVParser::get_uint( unsigned int x ) const
{
        const char *f = get_string(x).c_str();
        char *cp;
        unsigned long r = strtoul(f, &cp, 10);
        if (*cp)
                throw std::runtime_error("CSVParser::get_uint: invalid field \"" + get_string(x) + "\"");
        return r;
}

int CSVParser::get_int( unsigned int x ) const
{
        const char *f = get_string(x).c_str();
        char *cp;
        long r = strtol(f, &cp, 10);
        if (*cp)
                throw std::runtime_error("CSVParser::get_int: invalid field \"" + get_string(x) + "\"");
        return r;
}

unsigned int CSVParser::get_frequency( unsigned int x ) const
{
        return get_double(x) * 1000000;
}

Point CSVParser::get_point( unsigned int lonidx, unsigned int latidx ) const
{
        Point pt;
	double londeg(get_double(lonidx));
	double lonmin(get_double(lonidx + 1));
	double lonsec(get_double(lonidx + 2));
	double latdeg(get_double(latidx));
	double latmin(get_double(latidx + 1));
	double latsec(get_double(latidx + 2));
#if 0
	double lon(londeg + lonmin * (1.0 / 60.0) + lonsec * (1.0 / 60.0 / 60.0));
	double lat(latdeg + latmin * (1.0 / 60.0) + latsec * (1.0 / 60.0 / 60.0));
#else
	double lon(fabs(londeg) + fabs(lonmin) * (1.0 / 60.0) + fabs(lonsec) * (1.0 / 60.0 / 60.0));
	double lat(fabs(latdeg) + fabs(latmin) * (1.0 / 60.0) + fabs(latsec) * (1.0 / 60.0 / 60.0));
	if (londeg < 0.0)
		lon = -lon;
	if (latdeg < 0.0)
		lat = -lat;
#endif
        pt.set_lon_deg(lon);
        pt.set_lat_deg(lat);
        return pt;
}

void CSVParser::parse_file(const Glib::ustring & fn)
{
        close();
        open(fn);
        parse_airport();
        close();
}

void CSVParser::parse_airport(void)
{
        if (!m_airportsdbopen) {
                m_airportsdb.open(m_outputdir);
                m_airportsdb.sync_off();
                if (m_purgedb)
                        m_airportsdb.purgedb();
                m_airportsdbopen = true;
        }
        while (nextrec()) {
                if (size() < 15) {
                        std::cerr << "invalid airport record" << std::endl;
                        continue;
                }
                if (get_string(0).empty())
                        continue;
                Point pt(get_point(6, 3));
                AirportsDb::element_t e;
                {
                        AirportsDb::elementvector_t ev(m_airportsdb.find_by_sourceid(get_string(1) + m_srcid));
                        if (ev.size() == 1) {
                                e = ev.front();
                                if (e.get_modtime() >= get_filetime())
                                        continue;
                        }
                }
                if (!e.is_valid()) {
                        Rect bbox(pt.simple_box_nmi(1.0));
                        AirportsDb::elementvector_t ev(m_airportsdb.find_nearest(pt, bbox));
                        if (!ev.empty()) {
                                std::cerr << "Skipping " << get_string(0) << " (" << get_string(1) << ") due to duplicate airport "
                                          << ev.front().get_name() << " (" << ev.front().get_icao() << ")" << std::endl;
                                continue;
                        }
                }
                if (!e.is_valid()) {
                        e.set_label_placement(AirportsDb::Airport::label_e);
                        e.set_typecode('A');
                        e.set_sourceid(get_string(1) + m_srcid);
                }
                if (get_string(2).empty()) {
                        if (!m_topodbopen)
                                m_topodb.open(m_outputdir);
                        m_topodbopen = true;
                        TopoDb30::elev_t elev(m_topodb.get_elev(pt));
                        if (elev == TopoDb30::nodata || elev == TopoDb30::ocean)
                                elev = 0;
                        e.set_elevation(elev * Point::m_to_ft);
                } else {
                        e.set_elevation(get_int(2));
                }
                e.set_name(get_string(0));
                e.set_icao(get_string(1));
                e.set_modtime(get_filetime());
                e.set_coord(pt);
                if (!get_string(9).empty() && !get_string(10).empty()) {
                        AirportsDb::Airport::Runway *rwy(0);
                        for (unsigned int i = 0; i < e.get_nrrwy(); ++i) {
                                AirportsDb::Airport::Runway& rwy1(e.get_rwy(i));
                                if (rwy1.get_ident_he() == get_string(9) || rwy1.get_ident_le() == get_string(10)) {
                                        rwy = &rwy1;
                                        break;
                                }
                                if (rwy1.get_ident_le() == get_string(9) || rwy1.get_ident_he() == get_string(10)) {
                                        rwy1.swap_he_le();
                                        rwy = &rwy1;
                                        break;
                                }
                        }
                        if (!rwy) {
                                e.add_rwy(AirportsDb::Airport::Runway("", "", 0, 0, "", e.get_coord(), e.get_coord(), 0, 0, 0, 0, e.get_elevation(), 0, 0, 0, 0, e.get_elevation(), 0));
                                rwy = &e.get_rwy(e.get_nrrwy()-1);
                        }
                        rwy->set_width(Point::round<unsigned int,double>(get_double(12) * Point::m_to_ft));
                        rwy->set_surface(get_string(13));
                        rwy->set_ident_he(get_string(9));
                        rwy->set_ident_le(get_string(10));
                        rwy->set_he_hdg(Point::round<int,double>(get_double(9) * (10.0 * FPlanLeg::from_deg)));
                        rwy->set_le_hdg(rwy->get_he_hdg() + 0x8000);
                        rwy->set_he_coord(pt.spheric_course_distance_nmi((rwy->get_he_hdg() - 0x8000) * FPlanLeg::to_deg, get_double(11) * (0.5 * Point::km_to_nmi * 0.001)));
                        rwy->set_le_coord(pt.spheric_course_distance_nmi((rwy->get_le_hdg() - 0x8000) * FPlanLeg::to_deg, get_double(11) * (0.5 * Point::km_to_nmi * 0.001)));
                        rwy->set_length(Point::round<int,double>(get_double(11) * Point::m_to_ft));
                        rwy->set_he_lda(rwy->get_length() - rwy->get_he_disp());
                        rwy->set_he_tda(rwy->get_length() - rwy->get_le_disp());
                        rwy->set_le_lda(rwy->get_length() - rwy->get_le_disp());
                        rwy->set_le_tda(rwy->get_length() - rwy->get_he_disp());
                        rwy->set_flags(AirportsDb::Airport::Runway::flag_le_usable | AirportsDb::Airport::Runway::flag_he_usable);
                        rwy->normalize_he_le();
                }
                if (!get_string(14).empty()) {
                        uint32_t freq(get_frequency(14));
                        if (freq) {
                                int idxf(-1);
                                for (unsigned int i = 0; i < e.get_nrcomm(); ++i) {
                                        const AirportsDb::Airport::Comm& comm(e.get_comm(i));
                                        for (unsigned int j = 0; j < 5; ++j)
                                                if (comm[j] == freq)
                                                        idxf = i;
                                }
                                if (idxf == -1) {
                                        e.add_comm(AirportsDb::Airport::Comm("RDO", "", "", freq));
                                } else {
                                        AirportsDb::Airport::Comm& comm(e.get_comm(idxf));
                                        unsigned int i;
                                        for (i = 0; i < 5; ++i)
                                                if (comm[i] == freq)
                                                        break;
                                        if (i == 5)
                                                comm.set_freq(0, freq);
                                        if (comm.get_name().empty())
                                                comm.set_name("RDO");
                                }
                        }
                }
                std::cerr << "Saving airport " << get_string(0) << " (" << get_string(1) << ")" << std::endl;
                m_airportsdb.save(e);
        }
}

/* ---------------------------------------------------------------------- */


/* ---------------------------------------------------------------------- */

int main(int argc, char *argv[])
{
        static struct option long_options[] = {
                { "dir", required_argument, 0, 'd' },
                { "srcid", required_argument, 0, 's' },
                { "sourceid", required_argument, 0, 's' },
                { "purgedb", no_argument, 0, 'p' },
                {0, 0, 0, 0}
        };
        Glib::ustring db_dir(".");
        Glib::ustring srcid("@CSV");
        bool purgedb(false);
        int c, err(0);

        while ((c = getopt_long(argc, argv, "d:s:p", long_options, 0)) != EOF) {
                switch (c) {
                        case 'd':
                                if (optarg)
                                        db_dir = optarg;
                                break;

                        case 's':
                                if (optarg)
                                        srcid = optarg;
                                break;

                        case 'p':
                                purgedb = true;
                                break;

                        default:
                                err++;
                                break;
                }
        }
        if (err) {
                std::cerr << "usage: vfrdbcsv [-d <dir>] [-s <srcid>] [-p]" << std::endl;
                return EX_USAGE;
        }
        try {
                CSVParser parser(db_dir, srcid, purgedb);
                for (; optind < argc; optind++) {
                        std::cerr << "Parsing file " << argv[optind] << std::endl;
                        parser.parse_file(argv[optind]);
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
