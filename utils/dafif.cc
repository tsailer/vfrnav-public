/*
 * dafif.cc:  DAFIF-T to DB conversion utility
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
#include <sqlite3x.hpp>
#include <sqlite3.h>
#include <unistd.h>
#include <sys/stat.h>
#include <stdexcept>
#include <libxml++/parsers/domparser.h>
#include <cstring>
#include <limits>
#include <ctime>

#include "geom.h"
#include "dbobj.h"


class DafifHeader {
        public:
                DafifHeader(void) : m_efftime(0), m_exptime(0) {}
                void parse_version(const Glib::ustring& dir);

                Rect get_bbox(void) const { return m_bbox; }
                void set_bbox(const Rect& bbox) { m_bbox = bbox; }
                time_t get_efftime(void) const { return m_efftime; }
                time_t get_exptime(void) const { return m_exptime; }
                Glib::ustring get_efftime_gmt_str(const Glib::ustring& fmt = "%F %T");
                Glib::ustring get_exptime_gmt_str(const Glib::ustring& fmt = "%F %T");

        private:
                Rect m_bbox;
                time_t m_efftime;
                time_t m_exptime;
};

void DafifHeader::parse_version( const Glib::ustring & dafif_dir )
{
        Glib::ustring buf;
        {
                Glib::RefPtr<Glib::IOChannel> chan;
#ifdef GLIBMM_EXCEPTIONS_ENABLED
                try {
                        chan = Glib::IOChannel::create_from_file(dafif_dir + "/Version", "r");
                } catch (Glib::FileError& e) {
                        if (e.code() != Glib::FileError::NO_SUCH_ENTITY)
                                throw;
                        chan = Glib::IOChannel::create_from_file(dafif_dir + "/VERSION", "r");
                }
                if (chan->read_line(buf) != Glib::IO_STATUS_NORMAL)
                        throw std::runtime_error("Cannot read Version file");
#else
                std::auto_ptr<Glib::Error> ex;
                chan = Glib::IOChannel::create_from_file(dafif_dir + "/Version", "r", ex);
                if (ex.get()) {
                        if (ex->code() != Glib::FileError::NO_SUCH_ENTITY)
                                throw *ex;
                        chan = Glib::IOChannel::create_from_file(dafif_dir + "/VERSION", "r", ex);
                        if (ex.get())
                                throw *ex;
                }
                Glib::IOStatus st = chan->read_line(buf, ex);
                if (ex.get())
                        throw *ex;
                if (st != Glib::IO_STATUS_NORMAL)
                        throw std::runtime_error("Cannot read Version file");
#endif
        }
        if (buf.size() < 20) {
                throw std::runtime_error("Cannot read Version file (line too short)");
        }
        char buf1[16];
        struct tm tm;
        memset(&tm, 0, sizeof(tm));
        time_t tm0, tm1, tm2;
        tm0 = 0;
        gmtime_r(&tm0, &tm);
        tm0 = mktime(&tm);
        memset(&tm, 0, sizeof(tm));
        memcpy(buf1, buf.c_str() + 4, 2);
        buf1[2] = 0;
        tm.tm_mday = strtoul(buf1, NULL, 10);
        memcpy(buf1, buf.c_str() + 6, 2);
        buf1[2] = 0;
        tm.tm_mon = strtoul(buf1, NULL, 10) - 1;
        memcpy(buf1, buf.c_str() + 8, 4);
        buf1[4] = 0;
        tm.tm_year = strtoul(buf1, NULL, 10) - 1900;
        tm1 = mktime(&tm);
        memcpy(buf1, buf.c_str() + 12, 2);
        buf1[2] = 0;
        tm.tm_mday = strtoul(buf1, NULL, 10);
        memcpy(buf1, buf.c_str() + 14, 2);
        buf1[2] = 0;
        tm.tm_mon = strtoul(buf1, NULL, 10) - 1;
        memcpy(buf1, buf.c_str() + 16, 4);
        buf1[4] = 0;
        tm.tm_year = strtoul(buf1, NULL, 10) - 1900;
        tm2 = mktime(&tm);
        m_efftime = tm1 - tm0;
        m_exptime = tm2 - tm0;
}

Glib::ustring DafifHeader::get_efftime_gmt_str( const Glib::ustring & fmt )
{
        struct tm tm;
        if (!gmtime_r(&m_efftime, &tm))
                return "?";
        char buf[128];
        strftime(buf, sizeof(buf), fmt.c_str(), &tm);
        return buf;
}

Glib::ustring DafifHeader::get_exptime_gmt_str( const Glib::ustring & fmt )
{
        struct tm tm;
        if (!gmtime_r(&m_exptime, &tm))
                return "?";
        char buf[128];
        strftime(buf, sizeof(buf), fmt.c_str(), &tm);
        return buf;
}

/* ---------------------------------------------------------------------- */

class DafiftParser {
        public:
                DafiftParser(void);
                void open(const Glib::ustring& dafif_dir, const Glib::ustring& fn);
                void close(void);
                operator bool(void) const;
                bool nextrec(void);
                unsigned int size(void) const { return m_fld.size(); }
                const Glib::ustring& operator[](unsigned int x) const { return m_fld[x]; }
                double get_double(unsigned int x) const;
                unsigned int get_uint(unsigned int x) const;
                int get_int(unsigned int x) const;
                unsigned int get_frequency(unsigned int x) const;
                unsigned int get_vu_frequency(unsigned int x) const;
                Point get_point(unsigned int lonidx, unsigned int latidx) const;
                time_t get_cycle(unsigned int x) const;

        private:
                Glib::RefPtr<Glib::IOChannel> m_chan;
                std::vector<Glib::ustring> m_fld;
                Glib::IOStatus m_status;
};

DafiftParser::DafiftParser(void)
        : m_status(Glib::IO_STATUS_ERROR)
{
}

void DafiftParser::open( const Glib::ustring & dafif_dir, const Glib::ustring & fn )
{
        m_fld.clear();
        m_status = Glib::IO_STATUS_ERROR;
#ifdef GLIBMM_EXCEPTIONS_ENABLED
        try {
                m_chan = Glib::IOChannel::create_from_file(dafif_dir + "/" + fn, "r");
        } catch (Glib::FileError& e) {
                if (e.code() != Glib::FileError::NO_SUCH_ENTITY)
                        throw;
                m_chan = Glib::IOChannel::create_from_file(dafif_dir + "/" + fn.uppercase(), "r");
        }
#else
        std::auto_ptr<Glib::Error> ex;
        m_chan = Glib::IOChannel::create_from_file(dafif_dir + "/" + fn, "r", ex);
        if (ex.get()) {
                if (ex->code() != Glib::FileError::NO_SUCH_ENTITY)
                        throw *ex;
                m_chan = Glib::IOChannel::create_from_file(dafif_dir + "/" + fn.uppercase(), "r", ex);
                if (ex.get())
                        throw *ex;
        }
#endif
        // read header record
        if (!nextrec())
                throw std::runtime_error("DafiftParser: cannot read header");
}

void DafiftParser::close(void)
{
        m_fld.clear();
        m_status = Glib::IO_STATUS_ERROR;
        m_chan.clear();
}

DafiftParser::operator bool(void) const
{
        return m_status == Glib::IO_STATUS_NORMAL;
}

bool DafiftParser::nextrec(void)
{
        m_fld.clear();
        if (!m_chan) {
                m_status = Glib::IO_STATUS_ERROR;
                return false;
        }
        Glib::ustring buf;
#ifdef GLIBMM_EXCEPTIONS_ENABLED
        m_status = m_chan->read_line(buf);
#else
        std::auto_ptr<Glib::Error> ex;
        m_status = m_chan->read_line(buf, ex);
        if (ex.get())
                throw *ex;
#endif
        if (m_status != Glib::IO_STATUS_NORMAL)
                return false;
        Glib::ustring::size_type n;
        while ((n = buf.find('\t')) != Glib::ustring::npos) {
                m_fld.push_back(buf.substr(0, n));
                buf.erase(0, n+1);
        }
        if ((n = buf.find('\n')) != Glib::ustring::npos)
                buf.erase(n);
        if ((n = buf.find('\r')) != Glib::ustring::npos)
                buf.erase(n);
        m_fld.push_back(buf);
        for (unsigned int i = 0; i < m_fld.size(); i++) {
                Glib::ustring& buf(m_fld[i]);
                n = buf.size();
                while (n > 0 && buf[n-1] == ' ')
                        n--;
                buf.resize(n);
        }
        return true;
}

double DafiftParser::get_double( unsigned int x ) const
{
        const char *f = operator[](x).c_str();
        char *cp;
        double r = strtod(f, &cp);
        if (*cp) {
                std::ostringstream oss;
                oss << "DafiftParser::get_double: invalid field index " << x << " \"" << operator[](x) << "\"";
                throw std::runtime_error(oss.str());
        }
        return r;
}

unsigned int DafiftParser::get_uint( unsigned int x ) const
{
        const char *f = operator[](x).c_str();
        char *cp;
        unsigned long r = strtoul(f, &cp, 10);
        if (*cp) {
                std::ostringstream oss;
                oss << "DafiftParser::get_uint: invalid field index " << x << " \"" << operator[](x) << "\"";
                throw std::runtime_error(oss.str());
        }
        return r;
}

int DafiftParser::get_int( unsigned int x ) const
{
        const char *f = operator[](x).c_str();
        char *cp;
        long r = strtol(f, &cp, 10);
        if (*cp) {
                std::ostringstream oss;
                oss << "DafiftParser::get_int: invalid field index " << x << " \"" << operator[](x) << "\"";
                throw std::runtime_error(oss.str());
        }
        return r;
}

unsigned int DafiftParser::get_frequency( unsigned int x ) const
{
        const char *f = operator[](x).c_str();
        char *cp;
        if (!*f)
                return 0;
        unsigned long r = strtoul(f, &cp, 10);
        switch (*cp) {
                case 'M':
                        r *= 1000;
                        break;

                case 'K':
                        break;

                default:
                {
                        std::ostringstream oss;
                        oss << "DafiftParser::get_frequency: invalid field index " << x << " \"" << operator[](x) << "\"";
                        throw std::runtime_error(oss.str());
                }
        }
        cp++;
        if (*cp) {
                std::ostringstream oss;
                oss << "DafiftParser::get_frequency: invalid field index " << x << " \"" << operator[](x) << "\"";
                throw std::runtime_error(oss.str());
        }
        return r;
}

Point DafiftParser::get_point( unsigned int lonidx, unsigned int latidx ) const
{
        Point pt;
        pt.set_lon_deg(get_double(lonidx));
        pt.set_lat_deg(get_double(latidx));
        return pt;
}

time_t DafiftParser::get_cycle( unsigned int x ) const
{
        struct tm tm;
        memset(&tm, 0, sizeof(tm));
        time_t tm0 = 0, tm1;
        gmtime_r(&tm0, &tm);
        tm0 = mktime(&tm);
        memset(&tm, 0, sizeof(tm));
        unsigned int t = get_uint(x);
        tm.tm_year = t / 100 - 1900;
        tm.tm_mon = 0;
        tm.tm_mday = 1;
        tm1 = mktime(&tm) + (t % 100) * (28 * 24 * 60 * 60) - tm0;
        //gmtime_r(&tm1, &tm);
        //std::cerr << "Cycle " << t << " -> " << (tm.tm_year + 1900) << '-' << (tm.tm_mon + 1) << '-' << tm.tm_mday << std::endl;
        return tm1;
}

unsigned int DafiftParser::get_vu_frequency( unsigned int x ) const
{
        const Glib::ustring& fld(operator[](x));
        if (fld.empty())
                return 0;
        if (fld.size() < 8) {
                std::ostringstream oss;
                oss << "DafiftParser::get_vu_frequency: invalid field index " << x << " \"" << operator[](x) << "\"";
                throw std::runtime_error("DafiftParser::get_vu_frequency: invalid field \"" + fld + "\"");
        }
        char *cp;
        double freq = strtod(fld.c_str(), &cp);
        unsigned int f = (unsigned int)(freq + 0.5);
        if (fld[8] == 'M') {
                f = (unsigned int)(freq * 1e3 + 0.5);
                if (fld[6] == ' ')
                        f = ((f + 12) / 25) * 25;
        } else if (fld[8] != 'K')
                f = 0;
        return f * 1000;
}

/* ---------------------------------------------------------------------- */

/*
 * reading navaids
 */

static void read_navaids(const Glib::ustring& dafif_dir, const Glib::ustring& output_dir)
{
        DafiftParser p;
        p.open(dafif_dir, "NAV/NAV.TXT");
        NavaidsDb db;
        db.open(output_dir);
        db.sync_off();
        db.purgedb();
        while (p.nextrec()) {
                if (p.size() < 32)
                        throw std::runtime_error("Navaids: too few records");
                std::cerr << "reading: " << p[0] << ' ' << p[6] << ' ' << p[5] << std::endl;
                NavaidsDb::Navaid e;
                e.set_sourceid(p[0] + "_" + p[5] + "_" + p[8] + "@DAFIF");
                e.set_coord(p.get_point(20, 18));
                int elev = 0;
                if (p[23] != "U")
                        elev = p.get_int(23);
                e.set_elev(elev);
                unsigned int range = 0;
                if (p[14] != "U")
                        range = p.get_uint(14);
                e.set_range(range);
                e.set_frequency(p.get_frequency(8));
                e.set_icao(p[0]);
                e.set_name(p[5]);
                switch (*p[1].c_str()) {
                        case '1': // VOR
                                e.set_navaid_type(NavaidsDb::Navaid::navaid_vor);
                                break;

                        case '2': // VORTAC
                                e.set_navaid_type(NavaidsDb::Navaid::navaid_vortac);
                                break;

                        case '3': // TACAN
                                e.set_navaid_type(NavaidsDb::Navaid::navaid_tacan);
                                break;

                        case '4': // VOR-DME
                                e.set_navaid_type(NavaidsDb::Navaid::navaid_vordme);
                                break;

                        case '5': // NDB
                                e.set_navaid_type(NavaidsDb::Navaid::navaid_ndb);
                                break;

                        case '7': // NDB-DME
                                e.set_navaid_type(NavaidsDb::Navaid::navaid_ndbdme);
                                break;

                        case '9': // DME
                                e.set_navaid_type(NavaidsDb::Navaid::navaid_dme);
                                break;

                        default:
                                e.set_navaid_type(NavaidsDb::Navaid::navaid_invalid);
                                std::cerr << "Invalid Navaid Type " << p[1] << std::endl;
                                break;
                }
		if (e.has_dme())
			e.set_dmecoord(e.get_coord());
                e.set_label_placement((e.get_icao().empty() && e.get_name().empty()) ? NavaidsDb::Navaid::label_off : NavaidsDb::Navaid::label_any);
                switch (*p[30].c_str()) {
                        case 'I':
                                e.set_inservice(true);
                                break;

                        case 'O':
                                e.set_inservice(false);
                                break;

                        default:
                                e.set_inservice(false);
                                std::cerr << "Invalid Navaid Status " << p[30] << std::endl;
                                break;
                }
                e.set_modtime(p.get_cycle(31));
                if (e.is_valid())
                        db.save(e);
        }
        db.analyze();
        db.vacuum();
}

/* ---------------------------------------------------------------------- */

/*
 * reading airports
 */

static void read_airports(const Glib::ustring& dafif_dir, const Glib::ustring& output_dir)
{
        DafiftParser p;
        // Parse Airports
        p.open(dafif_dir, "ARPT/ARPT.TXT");
        AirportsDb db;
        db.open(output_dir);
        db.sync_off();
        db.purgedb();
        while (p.nextrec()) {
                if (p.size() < 23)
                        throw std::runtime_error("Airports: too few records");
                AirportsDb::Airport e;
                std::cerr << "reading: " << p[0] << ' ' << p[3] << ' ' << p[1] << std::endl;
                e.set_sourceid(p[0] + "@DAFIF");
                e.set_coord(p.get_point(10, 8));
                e.set_icao(p[3]);
                e.set_name(p[1]);
                int elev = 0;
                if (p[11] != "U")
                        elev = p.get_int(11);
                e.set_elevation(elev);
                e.set_typecode(*p[12].c_str());
                e.set_label_placement((e.get_icao().empty() && e.get_name().empty()) ? AirportsDb::Airport::label_off : AirportsDb::Airport::label_any);
                if (p[16][0] == 'Y' && p[17] == "MI" && p[21] == "CI")
                        e.set_icao(p[19]);
                e.set_modtime(p.get_cycle(22));
                if (e.is_valid())
                        db.save(e);
        }

        // Parse Airport Runway Information
        p.open(dafif_dir, "ARPT/RWY.TXT");
        while (p.nextrec()) {
                if (p.size() < 51)
                        throw std::runtime_error("Airport Runways: too few records");
                std::cerr << "reading: " << p[0] << ' ' << p[1] << ' ' << p[2] << std::endl;
                std::vector<AirportsDb::Airport> ev(db.find_by_sourceid(p[0] + "@DAFIF", 0, AirportsDb::comp_exact_casesensitive, 2));
                if (ev.size() != 1 || !ev.front().is_valid())
                        throw std::runtime_error("airportrunways: ID " + p[0] + " not found in airport database");
                AirportsDb::Airport& e(ev.front());
                TopoDb30::elev_t heelev(TopoDb30::nodata), leelev(TopoDb30::nodata);
                if (*p[13].c_str() != 'U')
                        heelev = Point::round<int,double>(p.get_double(13));
                if (*p[30].c_str() != 'U')
                        leelev = Point::round<int,double>(p.get_double(30));
                AirportsDb::Airport::Runway rwy(p[1], // IDENTHE
                                                p[2], // IDENTLE
                                                p.get_uint(5), // LENGTH
                                                p.get_uint(6), // WIDTH
                                                p[7], // SURFACE
                                                p.get_point(12, 10), // HECOORD
                                                p.get_point(29, 27), // LECOORD
                                                p.get_uint(47), // HETDA
                                                p.get_uint(46), // HELDA
                                                p.get_uint(16), // HEDISP
                                                Point::round<int,double>(p.get_double(43) * FPlanLeg::from_deg), // HEHDG
                                                heelev, // HEELEV
                                                p.get_uint(49), // LETDA
                                                p.get_uint(48), // LELDA
                                                p.get_uint(33), // LEDISP
                                                Point::round<int,double>(p.get_double(44) * FPlanLeg::from_deg), // LEHDG
                                                leelev, // LEELEV
                                                (*p[45].c_str() == 'C') ? 0 : (AirportsDb::Airport::Runway::flag_le_usable | AirportsDb::Airport::Runway::flag_he_usable));
                for (unsigned int i = 0; i < 8; ++i) {
                        rwy.set_he_light(i, p.get_uint(18 + i));
                        rwy.set_le_light(i, p.get_uint(35 + i));
                }
                rwy.normalize_he_le();
                e.add_rwy(rwy);
                db.save(e);
        }

        // Parse Airport Communication Information
        p.open(dafif_dir, "ARPT/ACOM.TXT");
        while (p.nextrec()) {
                if (p.size() < 13)
                        throw std::runtime_error("Airport Comms: too few records");
                std::cerr << "reading: " << p[0] << ' ' << p[2] << std::endl;
                std::vector<AirportsDb::Airport> ev(db.find_by_sourceid(p[0] + "@DAFIF", 0, AirportsDb::comp_exact_casesensitive, 2));
                if (ev.size() != 1 || !ev.front().is_valid())
                        throw std::runtime_error("airportcomms: ID " + p[0] + " not found in airport database");
                AirportsDb::Airport& e(ev.front());
                e.add_comm(AirportsDb::Airport::Comm(p[2], // NAME
                                                     p[9], // SECTOR
                                                     p[10], // OPRHOURS
                                                     p.get_vu_frequency(4), // FREQ0
                                                     p.get_vu_frequency(5), // FREQ1
                                                     p.get_vu_frequency(6), // FREQ2
                                                     p.get_vu_frequency(7), // FREQ3
                                                     p.get_vu_frequency(8))); // FREQ4
                db.save(e);
        }

        // Parse Airport VFR Remark
        p.open(dafif_dir, "VFR/VFR_RMK.TXT");
        while (p.nextrec()) {
                if (!p.size() || (p.size() == 1 && p[0] == ""))
                        continue;
                if (p.size() < 4)
                        throw std::runtime_error("Airport VFR Remarks: too few records");
                std::cerr << "reading: " << p[0] << ' ' << p[2] << std::endl;
                std::vector<AirportsDb::Airport> ev(db.find_by_sourceid(p[0] + "@DAFIF", 0, AirportsDb::comp_exact_casesensitive, 2));
                if (ev.size() != 1 || !ev.front().is_valid()) {
                        std::cerr << "airportvfrrmks: ID " + p[0] + " not found in airport database" << std::endl;
                        continue;
                }
                AirportsDb::Airport& e(ev.front());
                e.set_vfrrmk(e.get_vfrrmk() + p[2]);
                db.save(e);
        }

        // Parse Airport VFR Route Information
        p.open(dafif_dir, "VFR/VFR_SEG.TXT");
        while (p.nextrec()) {
                if (p.size() < 29)
                        throw std::runtime_error("Airport VFR Routes: too few records");
                std::cerr << "reading: " << p[0] << ' ' << p[3] << ' ' << p[4] << std::endl;
                std::vector<AirportsDb::Airport> ev(db.find_by_sourceid(p[0] + "@DAFIF", 0, AirportsDb::comp_exact_casesensitive, 2));
                if (ev.size() != 1 || !ev.front().is_valid()) {
                        std::cerr << "airportvfrroutes: ID " + p[0] + " not found in airport database" << std::endl;
                        continue;
                }
                AirportsDb::Airport& e(ev.front());
                unsigned int rteidx;
                for (rteidx = 0; rteidx < e.get_nrvfrrte(); rteidx++)
                        if (e.get_vfrrte(rteidx).get_name() == p[3])
                                break;
                if (rteidx >= e.get_nrvfrrte())
                        e.add_vfrrte(AirportsDb::Airport::VFRRoute(p[3])); // RTENAME
                AirportsDb::Airport::VFRRoute& rte(e.get_vfrrte(rteidx));
                AirportsDb::Airport::VFRRoute::VFRRoutePoint::altcode_t altcode = AirportsDb::Airport::VFRRoute::VFRRoutePoint::altcode_invalid;
                if (p[25] == "RA")
                        altcode = AirportsDb::Airport::VFRRoute::VFRRoutePoint::altcode_recommendedalt;
                else if (p[25] == "AA")
                        altcode = AirportsDb::Airport::VFRRoute::VFRRoutePoint::altcode_atorabove;
                else if (p[25] == "AB")
                        altcode = AirportsDb::Airport::VFRRoute::VFRRoutePoint::altcode_atorbelow;
                else if (p[25] == "AS")
                        altcode = AirportsDb::Airport::VFRRoute::VFRRoutePoint::altcode_assigned;
                else if (p[25] == "AT")
                        altcode = AirportsDb::Airport::VFRRoute::VFRRoutePoint::altcode_altspecified;
                else
                        std::cerr << "Airport Ident " << p[0] << " VFR segment invalid Altitude code " << p[25] << std::endl;
                AirportsDb::Airport::VFRRoute::VFRRoutePoint::pathcode_t pathcode = AirportsDb::Airport::VFRRoute::VFRRoutePoint::pathcode_invalid;
                switch (*p[24].c_str()) {
                        case 'A':
                                pathcode = AirportsDb::Airport::VFRRoute::VFRRoutePoint::pathcode_arrival;
                                break;

                        case 'D':
                                pathcode = AirportsDb::Airport::VFRRoute::VFRRoutePoint::pathcode_departure;
                                break;

                        case 'H':
                                pathcode = AirportsDb::Airport::VFRRoute::VFRRoutePoint::pathcode_holding;
                                break;

                        case 'T':
                                pathcode = AirportsDb::Airport::VFRRoute::VFRRoutePoint::pathcode_trafficpattern;
                                break;

                        case 'V':
                                pathcode = AirportsDb::Airport::VFRRoute::VFRRoutePoint::pathcode_vfrtransition;
                                break;

                        case 'O':
                                pathcode = AirportsDb::Airport::VFRRoute::VFRRoutePoint::pathcode_other;
                                break;

                        default:
                                std::cerr << "Airport Ident " << p[0] << " VFR segment invalid Path code " << p[24] << std::endl;
                                break;
                }
                Glib::ustring name(p[4]);
                rte.add_point(AirportsDb::Airport::VFRRoute::VFRRoutePoint(name, // PTNAME
                                                                           p.get_point(9, 7),
                                                                           p.get_int(26), // ALT
                                                                           name.empty() ? AirportsDb::Airport::label_off : AirportsDb::Airport::label_any,
                                                                           *p[20].c_str(), // PTSYM
                                                                           pathcode, // PATHCODE
                                                                           altcode,
                                                                           p[21] == "Y")); // PTATAPRT
                db.save(e);
        }

        // Recompute BoundingBox
        {
                AirportsDb::Airport e;
                db.loadfirst(e);
                while (e.is_valid()) {
                        e.recompute_bbox();
                        db.save(e);
                        db.loadnext(e);
                }
        }
        db.analyze();
        db.vacuum();
}

/* ---------------------------------------------------------------------- */

/*
 * reading airspace boundaries
 */

static std::pair<int,unsigned int> decode_bdryalt(const std::string& fld)
{
        std::pair<int,unsigned int> ret(0, 0);
        if (!fld.empty() && fld[0] == 'U') {
                ret.second |= AirspacesDb::Airspace::altflag_unkn;
                return ret;
        }
        if (!strncmp(fld.c_str(), "BY NOTAM", 8)) {
                ret.second |= AirspacesDb::Airspace::altflag_notam;
                return ret;
        }
        if (!strncmp(fld.c_str(), "UNLTD", 5)) {
                ret.second |= AirspacesDb::Airspace::altflag_unlim;
                return ret;
        }
        if (!strncmp(fld.c_str(), "SURFACE", 7)) {
                ret.second |= AirspacesDb::Airspace::altflag_sfc;
                return ret;
        }
        if (!strncmp(fld.c_str(), "GND", 3)) {
                ret.second |= AirspacesDb::Airspace::altflag_gnd;
                return ret;
        }
        if (!strncmp(fld.c_str(), "FL", 2)) {
                ret.second |= AirspacesDb::Airspace::altflag_fl;
                ret.first = strtoul(fld.c_str() + 2, NULL, 10) * 100;
                return ret;
        }
        char *cp;
        ret.first = strtoul(fld.c_str(), &cp, 10);
        if (!strncmp(cp, "AGL", 3))
                ret.second |= AirspacesDb::Airspace::altflag_agl;
        return ret;
}

static int read_boundaries(const Glib::ustring& dafif_dir, const Glib::ustring& output_dir)
{
        DafiftParser p;
        // Parse Airports
        p.open(dafif_dir, "BDRY/BDRY_PAR.TXT");
        AirspacesDb db;
        db.open(output_dir);
        db.sync_off();
        db.purgedb();
        while (p.nextrec()) {
                if (p.size() < 18)
                        throw std::runtime_error("Airspaces: too few records");
                std::cerr << "reading: " << p[0] << ' ' << p[3] << ' ' << p[2] << std::endl;
                AirspacesDb::Airspace e;
                e.set_sourceid(p[0] + "@DAFIF");
                e.set_icao(p[3]);
                e.set_name(p[2]);
                e.set_ident(p[0]);
                e.set_classexcept("");
                e.set_efftime("");
                e.set_commname(p[7]);
                e.set_commfreq(0, p.get_vu_frequency(8));
                e.set_commfreq(1, p.get_vu_frequency(9));
                char bclass(*p[10].c_str());
                if (!bclass)
                        bclass = ' ';
                e.set_bdryclass(bclass);
                e.set_typecode(p.get_uint(1));
                e.set_gndelevmin(AirspacesDb::Airspace::gndelev_unknown);
                e.set_gndelevmax(AirspacesDb::Airspace::gndelev_unknown);
                std::pair<int,unsigned int> alt = decode_bdryalt(p[15]);
                e.set_altlwr(alt.first);
                e.set_altlwrflags(alt.second);
                alt = decode_bdryalt(p[14]);
                if (alt.second & (AirspacesDb::Airspace::altflag_unlim | AirspacesDb::Airspace::altflag_notam))
                        alt.first = std::numeric_limits<int32_t>::max();
                e.set_altupr(alt.first);
                e.set_altuprflags(alt.second);
                e.set_label_placement((e.get_icao().empty() && e.get_name().empty()) ? AirspacesDb::Airspace::label_off : AirspacesDb::Airspace::label_any);
                e.set_modtime(p.get_cycle(17));
                if (e.is_valid())
                        db.save(e);
                else
                        std::cerr << "warning: invalid bdry: " << p[0] << ' ' << p[3] << ' ' << p[2] << std::endl;
        }

        p.open(dafif_dir, "SUAS/SUAS_PAR.TXT");
        while (p.nextrec()) {
                if (p.size() < 18)
                        throw std::runtime_error("Airspaces: too few records");
                Glib::ustring ident(p[0]);
                if (!p[1].empty())
                        ident += "/" + p[1];
                std::cerr << "reading: " << ident << ' ' << p[4] << ' ' << p[3] << std::endl;
                AirspacesDb::Airspace e;
                e.set_sourceid(ident + "@DAFIF");
                e.set_icao(p[4]);
                e.set_name(p[3]);
                e.set_ident(ident);
                e.set_classexcept(p[15]);
                e.set_efftime(p[14]);
                e.set_commname(p[8]);
                e.set_commfreq(0, p.get_vu_frequency(9));
                e.set_commfreq(1, p.get_vu_frequency(10));
                char bclass(*p[2].c_str());
                if (!bclass)
                        bclass = ' ';
                e.set_bdryclass(bclass);
                e.set_typecode(255);
                e.set_gndelevmin(AirspacesDb::Airspace::gndelev_unknown);
                e.set_gndelevmax(AirspacesDb::Airspace::gndelev_unknown);
                std::pair<int,unsigned int> alt = decode_bdryalt(p[13]);
                e.set_altlwr(alt.first);
                e.set_altlwrflags(alt.second);
                alt = decode_bdryalt(p[12]);
                if (alt.second & (AirspacesDb::Airspace::altflag_unlim | AirspacesDb::Airspace::altflag_notam))
                        alt.first = std::numeric_limits<int32_t>::max();
                e.set_altupr(alt.first);
                e.set_altuprflags(alt.second);
                e.set_label_placement((e.get_icao().empty() && e.get_name().empty()) ? AirspacesDb::Airspace::label_off : AirspacesDb::Airspace::label_any);
                e.set_modtime(p.get_cycle(16));
                if (e.is_valid())
                        db.save(e);
        }
        for (unsigned int fldoffs = 0; fldoffs < 2; fldoffs++) {
                p.open(dafif_dir, fldoffs ? "SUAS/SUAS.TXT" : "BDRY/BDRY.TXT");
                AirspacesDb::Airspace e;
                while (p.nextrec()) {
                        if (p.size() < 28)
                                throw std::runtime_error("AirspaceSegments: too few records");
                        Glib::ustring ident(p[0]);
                        if (fldoffs && !p[1].empty())
                                ident += "/" + p[1];
                        std::cerr << "reading: " << ident << ' ' << p[1+fldoffs] << std::endl;
                        if (!e.is_valid() || e.get_sourceid() != ident + "@DAFIF") {
                                if (e.is_valid())
                                        db.save(e);
                                std::vector<AirspacesDb::Airspace> ev(db.find_by_sourceid(ident + "@DAFIF", 0, AirportsDb::comp_exact_casesensitive, 2));
                                if (ev.size() != 1 || !ev.front().is_valid())
                                        throw std::runtime_error("airspacesegments: ID " + ident + " not found in airport database");
                                e = ev.front();
                        }
                        e.add_segment(AirspacesDb::Airspace::Segment(p.get_point(10+fldoffs, 8+fldoffs),
                                                                     p.get_point(14+fldoffs, 12+fldoffs),
                                                                     p.get_point(18+fldoffs, 16+fldoffs),
                                                                     *p[5+fldoffs].c_str(), // SHAPE
                                                                     (int)(p.get_double(19+fldoffs) * (Point::from_deg / 60.0)),
                                                                     (int)(p.get_double(20+fldoffs) * (Point::from_deg / 60.0))));
                }
                if (e.is_valid())
                        db.save(e);
        }

        // Parse SUAS Note
        p.open(dafif_dir, "SUAS/SUAS_NOTE.TXT");
        while (p.nextrec()) {
                if (p.size() < 6)
                        throw std::runtime_error("Airspace special use airspace notes: too few records");
                Glib::ustring ident(p[0]);
                if (!p[1].empty())
                        ident += "/" + p[1];
                std::cerr << "reading: " << ident << ' ' << p[4] << std::endl;
                std::vector<AirspacesDb::Airspace> ev(db.find_by_sourceid(ident + "@DAFIF", 0, AirportsDb::comp_exact_casesensitive, 2));
                if (ev.size() != 1 || !ev.front().is_valid())
                        throw std::runtime_error("airportvfrrmks: ID " + ident + " not found in airport database");
                AirspacesDb::Airspace& e(ev.front());
                e.set_note(e.get_note() + p[4]);
                db.save(e);
        }

        // Recompute Approximation
        {
                AirspacesDb::Airspace e;
                db.loadfirst(e);
                while (e.is_valid()) {
                        e.recompute_lineapprox(db);
                        e.compute_initial_label_placement();
                        db.save(e);
                        db.loadnext(e);
                }
        }
        db.analyze();
        db.vacuum();
        return 0;
}

/* ---------------------------------------------------------------------- */

/*
 * Read small airports from MICHAEL-LORENZ MEIER's Welt2000 database
 */

static int parse_int(const char *x, unsigned int len)
{
        char *y = (char *)alloca(len + 1);
        char *z;
        unsigned int r;

        memcpy(y, x, len);
        y[len] = 0;
        r = strtol(y, &z, 10);
        if (z != y + len)
                return INT_MIN;
        return r;
}

static int read_welt2000_airports(const Glib::ustring& welt2000_file, const Glib::ustring& output_dir)
{
        Glib::RefPtr<Glib::IOChannel> chan;
#if defined(GLIBMM_EXCEPTIONS_ENABLED)
        chan = Glib::IOChannel::create_from_file(welt2000_file, "r");
#else
        std::auto_ptr<Glib::Error> ex;
        chan = Glib::IOChannel::create_from_file(welt2000_file, "r", ex);
        if (ex.get())
                throw *ex;
#endif
        AirportsDb db;
        db.open(output_dir);
        db.sync_off();
        time_t filetime = time(0);
        {
                struct stat st;
                if (stat(welt2000_file.c_str(), &st))
                        std::cerr << "cannot get modified time of file " << welt2000_file << ", using current time" << std::endl;
                else
                        filetime = st.st_mtime;
        }
        Glib::ustring buf;
        int err1(0);
        for (;;) {
#if defined(GLIBMM_EXCEPTIONS_ENABLED)
                Glib::IOStatus iostat = chan->read_line(buf);
#else
                std::auto_ptr<Glib::Error> ex;
                Glib::IOStatus iostat = chan->read_line(buf, ex);
                if (ex.get())
                        throw *ex;
#endif
                if (iostat != Glib::IO_STATUS_NORMAL)
                        break;
                int err(0);
                if (buf.size() < 60)
                        continue;
                char buf1[256];
                strncpy(buf1, buf.c_str(), sizeof(buf1));
                buf1[sizeof(buf1)-1] = 0;
                if (buf1[0] == '$')
                        continue;
                // check if airfield
                if (buf1[23] != '#')
                        continue;
                // check if ICAO identifier
                if (buf1[24] < 'A' || buf1[24] > 'Z')
                        continue;
                buf1[23] = 0;
                char *cp = &buf1[23];
                *cp-- = 0;
                if (!strcmp(&buf1[20], "GLD")) {
                        cp = &buf1[20];
                        *cp-- = 0;
                }
                while (cp >= &buf1[7] && *cp == ' ')
                        *cp-- = 0;
                if (!buf1[7])
                        continue;
                char name[20], icao[8], ident[12];
                strncpy(name, buf1+7, sizeof(name));
                strncpy(icao, buf1+24, 4);
                icao[4] = 0;
                ident[0] = '$';
                strncpy(ident+1, buf1, sizeof(ident)-1);
                ident[8] = 0;
                AirportsDb::Airport arpt;
                arpt.set_sourceid(ident + Glib::ustring("@WELT2000"));
                arpt.set_icao(icao);
                arpt.set_name(name);
                arpt.set_typecode('A');
                arpt.set_label_placement(AirportsDb::Airport::label_off);
                {
                        int r = parse_int(buf1+41, 4);
                        err |= (r == INT_MIN);
                        arpt.set_elevation((int)(r * (1.0 / 0.3048)));
                }
                {
                        Glib::ustring surface("U");
                        switch (buf1[28]) {
                                case 'A':
                                        surface = "ASP";
                                        break;

                                case 'C':
                                        surface = "CON";
                                        break;

                                case 'L':
                                        surface = "CLA";
                                        break;

                                case 'S':
                                        surface = "SAN";
                                        break;

                                case 'Y':
                                        surface = "CLA";
                                        break;

                                case 'G':
                                        surface = "GRS";
                                        break;

                                case 'V':
                                        surface = "GVL";
                                        break;

                                default:
                                        surface = "U";
                        }
                        uint16_t width((uint16_t)(20 * (1.0 / 0.3048)));
                        int r = parse_int(buf1+29, 3);
                        err = (r <= 0);
                        if (err) {
                                std::cerr << "WARNING: Welt2000 airport " << buf1 << " has no runway, ignoring" << std::endl;
                                continue;
                        }
                        uint16_t length((uint16_t)(r * (10.0 / 0.3048)));
                        char ident_he[4], ident_le[4];
                        memcpy(ident_he, buf1+32, 2);
                        ident_he[2] = 0;
                        memcpy(ident_le, buf1+34, 2);
                        ident_le[2] = 0;
                        AirportsDb::Airport::Runway rwy(ident_he, ident_le, length, width, surface, Point::invalid, Point::invalid, length, length, 0, 0, arpt.get_elevation(), length, length, 0, 0, arpt.get_elevation(),
                                                        AirportsDb::Airport::Runway::flag_le_usable | AirportsDb::Airport::Runway::flag_he_usable);
                        rwy.compute_default_hdg();
                        rwy.compute_default_coord(arpt.get_coord());
                        rwy.normalize_he_le();
                        arpt.add_rwy(rwy);
                }
                int r = parse_int(buf1+36, 5);
                if (r >= 0 && r <= 999999) {
                        r *= 10;
                        r = ((r + 12) / 25) * 25;
                        arpt.add_comm(AirportsDb::Airport::Comm(name + Glib::ustring(" AD"), "", "", r * 1000));
                }
                r = parse_int(buf1+46, 2);
                int r1 = parse_int(buf1+48, 2);
                int r2 = parse_int(buf1+50, 2);
                err |= (r < 0) | (r > 90) | (r1 < 0) | (r1 >= 60) | (r2 < 0) | (r2 >= 60);
                Point pt;
                pt.set_lat_deg(r + r1 * (1.0 / 60.0) + r2 * (1.0 / 60.0 / 60.0));
                if (buf1[45] == 'S')
                        pt.set_lat(-pt.get_lat());
                else if (buf1[45] != 'N')
                        err |= 1;
                r = parse_int(buf1+53, 3);
                r1 = parse_int(buf1+56, 2);
                r2 = parse_int(buf1+58, 2);
                err |= (r < 0) | (r > 180) | (r1 < 0) | (r1 >= 60) | (r2 < 0) | (r2 >= 60);
                pt.set_lon_deg(r + r1 * (1.0 / 60.0) + r2 * (1.0 / 60.0 / 60.0));
                if (buf1[52] == 'W')
                        pt.set_lon(-pt.get_lon());
                else if (buf1[52] != 'E')
                        err |= 1;
                arpt.set_coord(pt);
                arpt.set_label_placement((arpt.get_icao().empty() && arpt.get_name().empty()) ? AirspacesDb::Airspace::label_off : AirportsDb::Airport::label_any);
                if (err) {
                        err1 |= err;
                        std::cerr << "WARNING: Welt2000 airport with errors: " << buf1 << std::endl;
                        continue;
                }
                arpt.set_modtime(filetime);
                // check for colocated DAFIF airport
                {
                        Point diffpt((Point::coord_t)(Point::from_deg / 60.0), (Point::coord_t)(Point::from_deg / 60.0));
                        Rect rect(arpt.get_coord() - diffpt, arpt.get_coord() + diffpt);
                        AirportsDb::elementvector_t ev(db.find_by_rect(rect, 0, false));
                        bool store(true);
                        for (AirportsDb::elementvector_t::iterator ei(ev.begin()), ee(ev.end()); ei != ee; ei++) {
                                if (arpt.get_coord().simple_distance_nmi((*ei).get_coord()) >= 1.0)
                                        continue;
                                store = false;
                                std::cerr << "INFO: not storing airport " << arpt.get_name()
                                          << " because it is colocated with DAFIF airport " << (*ei).get_name() << std::endl;
                                break;
                        }
                        if (store)
                                db.save(arpt);
                }
        }
        db.analyze();
        db.vacuum();
        if (err1)
                fprintf(stderr, "WARNING: Welt2000 airports with errors\n");
        return err1 ? -1 : 0;
}

/* ---------------------------------------------------------------------- */

/*
 * reading waypoints
 */

static int read_waypoints(const Glib::ustring& dafif_dir, const Glib::ustring& output_dir)
{
        DafiftParser p;
        p.open(dafif_dir, "WPT/WPT.TXT");
        WaypointsDb db;
        db.open(output_dir);
        db.sync_off();
        db.purgedb();
        while (p.nextrec()) {
                if (p.size() < 23)
                        throw std::runtime_error("Waypoints: too few records");
                WaypointsDb::Waypoint e;
                std::cerr << "reading: " << p[0] << ' ' << p[6] << ' ' << p[5] << std::endl;
                e.set_sourceid(p[0] + "_" + p[6] + "@DAFIF");
                e.set_coord(p.get_point(16, 14));
                e.set_icao(p[6]);
                e.set_name(p[5]);
                e.set_usage(*p[7].c_str());
                e.set_label_placement((e.get_icao().empty() && e.get_name().empty()) ? NavaidsDb::Navaid::label_off : NavaidsDb::Navaid::label_any);
                e.set_modtime(p.get_cycle(22));
                if (e.is_valid())
                        db.save(e);
        }
        db.analyze();
        db.vacuum();
        return 0;
}

/* ---------------------------------------------------------------------- */

static int parse_coord2(double *val, const char *cp)
{
        double ret;
        char *cp2;
        int sign = 1;

        if (val)
                *val = 0;
        if (!cp)
                return -1;
        if (isdigit(*cp) || *cp == '+' || *cp == '-') {
                ret = strtod(cp, NULL);
                goto out;
        }
        switch (*cp) {
                case 'N':
                case 'E':
                case 'n':
                case 'e':
                        sign = 1;
                        break;

                case 'S':
                case 'W':
                case 's':
                case 'w':
                        sign = -1;
                        break;

                default:
                        return -1;
        }
        ret = strtod(cp+1, &cp2);
        if (cp2 == cp)
                return -1;
        cp = cp2;
        while (*cp == ((char)0xb0) || isspace(*cp))
                cp++;
        if (!*cp)
                goto out;
        ret += strtod(cp, &cp2) * (1.0 / 60.0);
        if (cp2 == cp)
                return -1;
        cp = cp2;
        while (*cp == '\'' || isspace(*cp))
                cp++;
        if (!*cp)
                goto out;
        ret += strtod(cp, &cp2) * (1.0 / 60.0 / 60.0);
        if (cp2 == cp)
                return -1;
        cp = cp2;
        while (*cp == '\'' || isspace(*cp))
                cp++;
        if (*cp)
                return -1;
  out:
        if (val)
                *val = ret * sign;
        return 0;
}

static inline double parse_coord(const char *cp, double dflt)
{
        double v;

        if (parse_coord2(&v, cp))
                return dflt;
        return v;
}

/* ---------------------------------------------------------------------- */

static int process_airport_comm(xmlpp::Element *el, AirportsDb::Airport& arpt)
{
        xmlpp::Attribute *attr = el->get_attribute("find");
        unsigned int commindex(arpt.get_nrcomm());
        if (attr) {
                std::string s(attr->get_value());
                for (commindex = 0; commindex < arpt.get_nrcomm(); commindex++) {
                        AirportsDb::Airport::Comm& comm(arpt.get_comm(commindex));
                        if (comm.get_name() == s)
                                break;
                }
        }
        if (commindex >= arpt.get_nrcomm())
                arpt.add_comm(AirportsDb::Airport::Comm("", "", ""));
        AirportsDb::Airport::Comm& comm(arpt.get_comm(commindex));
        if ((attr = el->get_attribute("name")))
                comm.set_name(attr->get_value());
        if ((attr = el->get_attribute("sector")))
                comm.set_sector(attr->get_value());
        if ((attr = el->get_attribute("oprhours")))
                comm.set_oprhours(attr->get_value());
        if ((attr = el->get_attribute("freq0")) || (attr = el->get_attribute("freq")))
                comm.set_freq(0, strtoul(attr->get_value().c_str(), 0, 0) * 1000);
        if ((attr = el->get_attribute("freq1")))
                comm.set_freq(1, strtoul(attr->get_value().c_str(), 0, 0) * 1000);
        if ((attr = el->get_attribute("freq2")))
                comm.set_freq(2, strtoul(attr->get_value().c_str(), 0, 0) * 1000);
        if ((attr = el->get_attribute("freq3")))
                comm.set_freq(3, strtoul(attr->get_value().c_str(), 0, 0) * 1000);
        if ((attr = el->get_attribute("freq4")))
                comm.set_freq(4, strtoul(attr->get_value().c_str(), 0, 0) * 1000);
        return 0;
}

static int process_airport_runway(xmlpp::Element *el, AirportsDb::Airport& arpt)
{
        xmlpp::Attribute *attr = el->get_attribute("find");
        unsigned int rwyindex(arpt.get_nrrwy());
        if (attr) {
                std::string s(attr->get_value());
                for (rwyindex = 0; rwyindex < arpt.get_nrrwy(); rwyindex++) {
                        AirportsDb::Airport::Runway& rwy(arpt.get_rwy(rwyindex));
                        if (rwy.get_ident_he() == s || rwy.get_ident_le() == s)
                                break;
                }
        }
        if (rwyindex >= arpt.get_nrrwy())
                arpt.add_rwy(AirportsDb::Airport::Runway("", "", 0, 0, "ASP", Point::invalid, Point::invalid, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0));
        AirportsDb::Airport::Runway& rwy(arpt.get_rwy(rwyindex));
        Glib::ustring ident_he;
        Glib::ustring ident_le;
        uint16_t length(0);
        uint16_t width(0);
        Glib::ustring surface("ASP");
        Point he_coord(Point::invalid), le_coord(Point::invalid);
        uint16_t he_tda(0);
        uint16_t he_lda(0);
        uint16_t he_disp(0);
        uint16_t he_hdg(0);
        int16_t he_elev(arpt.get_elevation());
        uint16_t le_tda(0);
        uint16_t le_lda(0);
        uint16_t le_disp(0);
        uint16_t le_hdg(0);
        int16_t le_elev(arpt.get_elevation());
        uint16_t flags(AirportsDb::Airport::Runway::flag_le_usable | AirportsDb::Airport::Runway::flag_he_usable);
        if ((attr = el->get_attribute("surface")))
                surface = attr->get_value();
        if ((attr = el->get_attribute("length")))
                length = (unsigned int)(strtoul(attr->get_value().c_str(), 0, 0) * Point::m_to_ft);
        if ((attr = el->get_attribute("flags")))
                flags = strtoul(attr->get_value().c_str(), 0, 0);
        xmlpp::Element::NodeList nl(el->get_children());
        for (xmlpp::Element::NodeList::iterator ni(nl.begin()), ne(nl.end()); ni != ne; ni++) {
                xmlpp::Element *el2 = dynamic_cast<xmlpp::Element *>(*ni);
                if (!el2)
                        continue;
                if (el2->get_name() == "he") {
                        if ((attr = el2->get_attribute("ident")))
                                ident_he = attr->get_value();
                        if ((attr = el2->get_attribute("tda")))
                                he_tda = Point::round<unsigned int,double>(strtod(attr->get_value().c_str(), 0) * Point::m_to_ft);
                        if ((attr = el2->get_attribute("lda")))
                                he_lda = Point::round<unsigned int,double>(strtod(attr->get_value().c_str(), 0) * Point::m_to_ft);
                        if ((attr = el2->get_attribute("disp")))
                                he_disp = Point::round<unsigned int,double>(strtod(attr->get_value().c_str(), 0) * Point::m_to_ft);
                        if ((attr = el2->get_attribute("hdg")))
                                he_hdg = Point::round<unsigned int,double>(strtod(attr->get_value().c_str(), 0) * FPlanLeg::from_deg);
                        if ((attr = el2->get_attribute("elev")))
                                he_elev = strtol(attr->get_value().c_str(), 0, 0);
                        double t;
                        if ((attr = el2->get_attribute("lat")) && !parse_coord2(&t, attr->get_value().c_str()))
                                he_coord.set_lat(Point::round<Point::coord_t,double>(t * Point::from_deg));
                        if ((attr = el2->get_attribute("lon")) && !parse_coord2(&t, attr->get_value().c_str()))
                                he_coord.set_lon(Point::round<Point::coord_t,double>(t * Point::from_deg));
                        continue;
                }
                if (el2->get_name() == "le") {
                        if ((attr = el2->get_attribute("ident")))
                                ident_le = attr->get_value();
                        if ((attr = el2->get_attribute("tda")))
                                le_tda = Point::round<unsigned int,double>(strtoul(attr->get_value().c_str(), 0, 0) * Point::m_to_ft);
                        if ((attr = el2->get_attribute("lda")))
                                le_lda = Point::round<unsigned int,double>(strtoul(attr->get_value().c_str(), 0, 0) * Point::m_to_ft);
                        if ((attr = el2->get_attribute("disp")))
                                le_disp = Point::round<unsigned int,double>(strtod(attr->get_value().c_str(), 0) * Point::m_to_ft);
                        if ((attr = el2->get_attribute("hdg")))
                                le_hdg = Point::round<unsigned int,double>(strtod(attr->get_value().c_str(), 0) * FPlanLeg::from_deg);
                        if ((attr = el2->get_attribute("elev")))
                                le_elev = strtol(attr->get_value().c_str(), 0, 0);
                        double t;
                        if ((attr = el2->get_attribute("lat")) && !parse_coord2(&t, attr->get_value().c_str()))
                                he_coord.set_lat(Point::round<Point::coord_t,double>(t * Point::from_deg));
                        if ((attr = el2->get_attribute("lon")) && !parse_coord2(&t, attr->get_value().c_str()))
                                he_coord.set_lon(Point::round<Point::coord_t,double>(t * Point::from_deg));
                        continue;
                }
        }
        rwy = AirportsDb::Airport::Runway(ident_he, ident_le, length, width, surface, he_coord, le_coord, he_tda, he_lda, he_disp, he_hdg, he_elev, le_tda, le_lda, le_disp, le_hdg, le_elev, flags);
        rwy.compute_default_hdg();
        rwy.compute_default_coord(arpt.get_coord());
        return 0;
}

static int process_airport_vfrroute(xmlpp::Element *el, AirportsDb::Airport& arpt)
{
        double t;

        xmlpp::Element::NodeList nl(el->get_children());
        for (xmlpp::Element::NodeList::iterator ni(nl.begin()), ne(nl.end()); ni != ne; ni++) {
                xmlpp::Element *el2 = dynamic_cast<xmlpp::Element *>(*ni);
                if (!el2)
                        continue;
                //std::cerr << "vfrroute: element: " << el2->get_name() << std::endl;
                if (el2->get_name() != "point")
                        continue;
                xmlpp::Attribute *attr;
                if (!(attr = el2->get_attribute("routename")))
                        continue;
                std::string routename(attr->get_value());
                std::cerr << "vfrroute: routename: " << routename << std::endl;
                unsigned int rteidx;
                for (rteidx = 0; rteidx < arpt.get_nrvfrrte(); rteidx++)
                        if (arpt.get_vfrrte(rteidx).get_name() == routename)
                                break;
                if (rteidx >= arpt.get_nrvfrrte())
                        arpt.add_vfrrte(AirportsDb::Airport::VFRRoute(routename)); // RTENAME
                AirportsDb::Airport::VFRRoute& rte(arpt.get_vfrrte(rteidx));
                // Route
                AirportsDb::Airport::VFRRoute::VFRRoutePoint::altcode_t altcode = AirportsDb::Airport::VFRRoute::VFRRoutePoint::altcode_invalid;
                AirportsDb::Airport::VFRRoute::VFRRoutePoint::pathcode_t pathcode = AirportsDb::Airport::VFRRoute::VFRRoutePoint::pathcode_invalid;
                std::string ptname;
                Point pt;
                int16_t alt = 0;
                AirportsDb::Airport::label_placement_t lp = AirportsDb::Airport::label_any;
                char ptsym = 'C';
                bool atapt = false;
                if ((attr = el2->get_attribute("pointname")))
                        ptname = attr->get_value();
                if ((attr = el2->get_attribute("sym")))
                        ptsym = *attr->get_value().c_str();
                if ((attr = el2->get_attribute("atairport")))
                        atapt = attr->get_value() == "Y" || attr->get_value() == "y";
                if ((attr = el2->get_attribute("pathcode"))) {
                        std::string s(attr->get_value());
                        switch (*s.c_str()) {
                                case 'A':
                                        pathcode = AirportsDb::Airport::VFRRoute::VFRRoutePoint::pathcode_arrival;
                                        break;

                                case 'D':
                                        pathcode = AirportsDb::Airport::VFRRoute::VFRRoutePoint::pathcode_departure;
                                        break;

                                case 'H':
                                        pathcode = AirportsDb::Airport::VFRRoute::VFRRoutePoint::pathcode_holding;
                                        break;

                                case 'T':
                                        pathcode = AirportsDb::Airport::VFRRoute::VFRRoutePoint::pathcode_trafficpattern;
                                        break;

                                case 'V':
                                        pathcode = AirportsDb::Airport::VFRRoute::VFRRoutePoint::pathcode_vfrtransition;
                                        break;

                                case 'O':
                                        pathcode = AirportsDb::Airport::VFRRoute::VFRRoutePoint::pathcode_other;
                                        break;

                                default:
                                        std::cerr << "Airport Name " << arpt.get_name() << " VFR segment invalid Path code " << s << std::endl;
                                        break;
                        }
                }
                if ((attr = el2->get_attribute("altcode"))) {
                        std::string s(attr->get_value());
                        if (s == "RA" || s == "R")
                                altcode = AirportsDb::Airport::VFRRoute::VFRRoutePoint::altcode_recommendedalt;
                        else if (s == "AA" || s == "A")
                                altcode = AirportsDb::Airport::VFRRoute::VFRRoutePoint::altcode_atorabove;
                        else if (s == "AB" || s == "B")
                                altcode = AirportsDb::Airport::VFRRoute::VFRRoutePoint::altcode_atorbelow;
                        else if (s == "AS" || s == "S")
                                altcode = AirportsDb::Airport::VFRRoute::VFRRoutePoint::altcode_assigned;
                        else if (s == "AT" || s == "T")
                                altcode = AirportsDb::Airport::VFRRoute::VFRRoutePoint::altcode_altspecified;
                        else
                                std::cerr << "Airport Name " << arpt.get_name() << " VFR segment invalid Altitude code " << s << std::endl;
                }
                if ((attr = el2->get_attribute("lat")) && !parse_coord2(&t, attr->get_value().c_str()))
                        pt.set_lat((Point::coord_t)(t * Point::from_deg));
                if ((attr = el2->get_attribute("lon")) && !parse_coord2(&t, attr->get_value().c_str()))
                        pt.set_lon((Point::coord_t)(t * Point::from_deg));
                if ((attr = el2->get_attribute("alt")))
                        alt = strtol(attr->get_value().c_str(), NULL, 0);
                if (ptname.empty())
                        lp = AirportsDb::Airport::label_off;
                rte.add_point(AirportsDb::Airport::VFRRoute::VFRRoutePoint(ptname, pt, alt, lp, ptsym, pathcode, altcode, atapt));
        }
        return 0;
}

static void print_airport(const AirportsDb::Airport& arpt)
{
        std::cout << "Airport " << arpt.get_icao() << ' ' << arpt.get_name()
                  << ' ' << arpt.get_coord().get_lon_str2() << ' ' << arpt.get_coord().get_lat_str2()
                  << ' ' << arpt.get_elevation() << ' ' << arpt.get_typecode() << std::endl
                  << "  Remark: " << arpt.get_vfrrmk() << std::endl;
        for (unsigned int i = 0; i < arpt.get_nrrwy(); i++) {
                const AirportsDb::Airport::Runway& rwy(arpt.get_rwy(i));
                std::cout << "  RWY: " << rwy.get_ident_he() << '/' << rwy.get_ident_le() << ' '
                          << rwy.get_length() << ' ' << rwy.get_width() << ' ' << rwy.get_surface()
                          << ' ' << rwy.get_he_tda() << ' ' << rwy.get_he_lda()
                          << ' ' << rwy.get_le_tda() << ' ' << rwy.get_le_lda() << std::endl;
        }
        for (unsigned int i = 0; i < arpt.get_nrcomm(); i++) {
                const AirportsDb::Airport::Comm& comm(arpt.get_comm(i));
                std::cout << "  COMM: " << comm.get_name() << ' ' << comm.get_sector() << ' ' << comm.get_oprhours()
                          << ' ' << Conversions::freq_str(comm[0])
                          << ' ' << Conversions::freq_str(comm[1])
                          << ' ' << Conversions::freq_str(comm[2])
                          << ' ' << Conversions::freq_str(comm[3])
                          << ' ' << Conversions::freq_str(comm[4]) << std::endl;
        }
        for (unsigned int i = 0; i < arpt.get_nrvfrrte(); i++) {
                const AirportsDb::Airport::VFRRoute& rte(arpt.get_vfrrte(i));
                std::cout << "  RTE: " << rte.get_name() << std::endl;
                for (unsigned int j = 0; j < rte.size(); j++) {
                        const AirportsDb::Airport::VFRRoute::VFRRoutePoint& pt(rte[j]);
                        std::cout << "    PT: " << pt.get_name() << ' ' << pt.get_coord().get_lon_str2()
                                  << ' ' << pt.get_coord().get_lat_str2() << ' ' << pt.get_altitude_string()
                                  << ' ' << NavaidsDb::Navaid::get_label_placement_name(pt.get_label_placement())
                                  << ' ' << pt.get_ptsym() << ' ' << pt.get_pathcode_string()
                                  << ' ' << (pt.is_at_airport() ? 'Y' : 'N') << std::endl;
                }
        }
}

static int process_airport(xmlpp::Element *el, const Glib::ustring& output_dir)
{
        AirportsDb db;
        db.open(output_dir);
        db.sync_off();
        AirportsDb::Airport arpt;
        try {
                xmlpp::Attribute *attrfindname = el->get_attribute("findname");
                xmlpp::Attribute *attrfindicao = el->get_attribute("findicao");
                AirportsDb::elementvector_t elvicao, elvname;
                if (attrfindicao)
                        elvicao = db.find_by_icao(attrfindicao->get_value());
                if (attrfindname)
                        elvname = db.find_by_name(attrfindname->get_value());
                if (elvicao.size() > 1)
                        throw std::runtime_error("multiple airports match ICAO " + attrfindicao->get_value());
                if (elvname.size() > 1)
                        throw std::runtime_error("multiple airports match name " + attrfindname->get_value());
                if (!elvicao.empty() && !elvname.empty() && (*elvicao.begin()).get_id() != (*elvname.begin()).get_id())
                        throw std::runtime_error("ICAO " + attrfindicao->get_value() + " and name " + attrfindname->get_value() + " match different airports");
                elvname.insert(elvname.end(), elvicao.begin(), elvicao.end());
                if (elvname.empty()) {
                        if (attrfindname || attrfindicao) {
                                std::ostringstream oss;
                                if (attrfindicao)
                                        oss << "ICAO " + attrfindicao->get_value();
                                if (attrfindicao && attrfindname)
                                        oss << " / ";
                                if (attrfindname)
                                        oss << "name " + attrfindname->get_value();
                                oss << " does not match any airport";
                                throw std::runtime_error(oss.str());
                        }
                } else
                        arpt = *elvname.begin();
        } catch (const std::exception& err) {
                std::cerr << "WARNING: " << err.what() << std::endl;
        }
        if (!arpt.is_valid()) {
                arpt.set_typecode('A');
                arpt.set_label_placement(AirportsDb::Airport::label_any);
        }
        xmlpp::Attribute *attr;
        if ((attr = el->get_attribute("icao")))
                arpt.set_icao(attr->get_value());
        if ((attr = el->get_attribute("name")))
                arpt.set_name(attr->get_value());
        if (arpt.get_icao().empty() && arpt.get_name().empty())
                arpt.set_label_placement(AirportsDb::Airport::label_off);
        double t;
        if ((attr = el->get_attribute("lat")) && !parse_coord2(&t, attr->get_value().c_str()))
                arpt.set_coord(Point(arpt.get_coord().get_lon(), (Point::coord_t)(t * Point::from_deg)));
        if ((attr = el->get_attribute("lon")) && !parse_coord2(&t, attr->get_value().c_str()))
                arpt.set_coord(Point((Point::coord_t)(t * Point::from_deg), arpt.get_coord().get_lat()));
        if ((attr = el->get_attribute("elev")))
                arpt.set_elevation(strtol(attr->get_value().c_str(), NULL, 0));
        if ((attr = el->get_attribute("type")))
                arpt.set_typecode(*attr->get_value().c_str());
        if ((attr = el->get_attribute("vfrrmk")))
                arpt.set_vfrrmk(attr->get_value());
        xmlpp::Element::NodeList nl(el->get_children());
        for (xmlpp::Element::NodeList::iterator ni(nl.begin()), ne(nl.end()); ni != ne; ni++) {
                xmlpp::Element *el2 = dynamic_cast<xmlpp::Element *>(*ni);
                if (!el2)
                        continue;
                if (el2->get_name() == "comm") {
                        process_airport_comm(el2, arpt);
                        continue;
                }
                if (el2->get_name() == "runway") {
                        process_airport_runway(el2, arpt);
                        continue;
                }
                if (el2->get_name() == "vfrroute") {
                        process_airport_vfrroute(el2, arpt);
                        continue;
                }
        }
        if (!arpt.get_modtime())
                arpt.set_modtime(time(0));
        if (arpt.get_sourceid().empty())
                arpt.set_sourceid(arpt.get_icao() + "@dbadditions");
        try {
                db.save(arpt);
        } catch (const sqlite3x::database_error &dberr) {
                Glib::TimeVal tv;
                tv.assign_current_time();
                char buf[32];
                snprintf(buf, sizeof(buf), "%.6f", tv.as_double());
                arpt.set_sourceid(Glib::ustring(buf) + "@dbadditions");
                db.save(arpt);
        }
        print_airport(arpt);
        return 0;
}

static void process_xml(const Glib::ustring& xmlname, const Glib::ustring& output_dir)
{
        if (xmlname.empty())
                return;
        xmlpp::DomParser xml;
        xml.set_validate(false); // Do not validate, we do not have a DTD
        xml.set_substitute_entities(); // We just want the text to be resolved/unescaped automatically.
        xml.parse_file(xmlname);
        if (!xml)
                throw std::runtime_error("cannot parse input file '" + xmlname + "'");
        xmlpp::Document *doc = xml.get_document();
        xmlpp::Element *root = doc->get_root_node();
        if (root->get_name() != "Additions")
                throw std::runtime_error("invalid input file '" + xmlname + "'");
        std::cout << "Processing Additions file " << xmlname << std::endl;
        xmlpp::Element::NodeList nl(root->get_children());
        for (xmlpp::Element::NodeList::iterator ni(nl.begin()), ne(nl.end()); ni != ne; ni++) {
                xmlpp::Element *el = dynamic_cast<xmlpp::Element *>(*ni);
                if (!el)
                        continue;
                if (el->get_name() == "Airport") {
                        process_airport(el, output_dir);
                        continue;
                }
                std::cerr << "unknown node " << el->get_name() << ", skipping" << std::endl;
        }
        {
                AirportsDb db;
                db.open(output_dir);
                db.sync_off();
                db.analyze();
                db.vacuum();
        }
}

/* ---------------------------------------------------------------------- */

int main(int argc, char *argv[])
{
        static struct option long_options[] = {
                { "help",               no_argument,       NULL, 'h' },
                { "version",            no_argument,       NULL, 'v' },
                { "quiet",              no_argument,       NULL, 'q' },
                { "write",              required_argument, NULL, 'w' },
                { "dafif-dir",          required_argument, NULL, 'd' },
                { "output-dir",         required_argument, NULL, 'o' },
                { "kflog-dir",          required_argument, NULL, 'k' },
                { "additions-file",     required_argument, NULL, 'a' },
                { "min-lat",            required_argument, NULL, 0x400 },
                { "max-lat",            required_argument, NULL, 0x401 },
                { "min-lon",            required_argument, NULL, 0x402 },
                { "max-lon",            required_argument, NULL, 0x403 },
                { "welt-2000",          optional_argument, NULL, 'W' },
                { NULL,                 0,                 NULL, 0 }
        };
        int c, err(0);
        bool quiet(false);
        Glib::ustring output_dir(".");
        Glib::ustring dafif_dir;
        Glib::ustring kflog_dir;
        Glib::ustring additions_file;
        Glib::ustring welt2000_file;
        DafifHeader hdr;

        while ((c = getopt_long(argc, argv, "hvqd:o:k:a:W:", long_options, NULL)) != -1) {
                switch (c) {
                        case 'v':
                                std::cout << argv[0] << ": (C) 2007 Thomas Sailer" << std::endl;
                                return 0;

                        case 'd':
                                dafif_dir = optarg;
                                break;

                        case 'o':
                                output_dir = optarg;
                                break;

                        case 'k':
                                kflog_dir = optarg;
                                break;

                        case 'a':
                                additions_file = optarg;
                                break;

                        case 'q':
                                quiet = true;
                                break;

                        case 'W':
                                welt2000_file = optarg ? optarg : "/home/sailer/src/pilot_gps/conduit/WELT2000.TXT";
                                break;

                        case 0x400:
                        {
                                Point sw(hdr.get_bbox().get_southwest());
                                Point ne(hdr.get_bbox().get_northeast());
                                if (!sw.set_lat_str(optarg))
                                        sw.set_lat_deg(strtod(optarg, NULL));
                                hdr.set_bbox(Rect(sw, ne));
                                break;
                        }

                        case 0x401:
                        {
                                Point sw(hdr.get_bbox().get_southwest());
                                Point ne(hdr.get_bbox().get_northeast());
                                if (!ne.set_lat_str(optarg))
                                        ne.set_lat_deg(strtod(optarg, NULL));
                                hdr.set_bbox(Rect(sw, ne));
                                break;
                        }

                        case 0x402:
                        {
                                Point sw(hdr.get_bbox().get_southwest());
                                Point ne(hdr.get_bbox().get_northeast());
                                if (!sw.set_lon_str(optarg))
                                        sw.set_lon_deg(strtod(optarg, NULL));
                                hdr.set_bbox(Rect(sw, ne));
                                break;
                        }

                        case 0x403:
                        {
                                Point sw(hdr.get_bbox().get_southwest());
                                Point ne(hdr.get_bbox().get_northeast());
                                if (!ne.set_lon_str(optarg))
                                        ne.set_lon_deg(strtod(optarg, NULL));
                                hdr.set_bbox(Rect(sw, ne));
                                break;
                        }

                        case 'h':
                        default:
                                err++;
                                break;
                }
        }
        if (err) {
                std::cerr << "usage: dafif [-q]" << std::endl
                          << "     -q                quiet" << std::endl
                          << "     -h, --help        Display this information" << std::endl
                          << "     -v, --version     Display version information" << std::endl << std::endl;
                return EX_USAGE;
        }
        std::cout << "DB Boundary: " << (std::string)hdr.get_bbox().get_southwest().get_lat_str()
                  << ' ' << (std::string)hdr.get_bbox().get_southwest().get_lon_str()
                  << " -> " << (std::string)hdr.get_bbox().get_northeast().get_lat_str()
                  << ' ' << (std::string)hdr.get_bbox().get_northeast().get_lon_str() << std::endl;
        try {
                if (!dafif_dir.empty()) {
                        hdr.parse_version(dafif_dir);
                        std::cout << "Dates: eff " << hdr.get_efftime_gmt_str() << " exp " << hdr.get_exptime_gmt_str() << std::endl;
                        read_navaids(dafif_dir, output_dir);
                        read_airports(dafif_dir, output_dir);
                        read_boundaries(dafif_dir, output_dir);
                        read_waypoints(dafif_dir, output_dir);
                }
                if (!welt2000_file.empty() && read_welt2000_airports(welt2000_file, output_dir)) {
                        std::cerr << "WARNING: Error reading welt2000 airports from file " << welt2000_file << std::endl;
                }
                if (!additions_file.empty())
                        process_xml(additions_file, output_dir);

        } catch (const std::exception& e) {
                std::cerr << "Error: " << e.what() << std::endl;
                return EX_DATAERR;
        } catch (const Glib::Exception& e) {
                std::cerr << "Glib Error: " << e.what() << std::endl;
                return EX_DATAERR;
        }
        return EX_OK;
}
