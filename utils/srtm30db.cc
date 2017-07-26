/*
 * srtm30.cc: SRTM30 / GTOPO30 to DB conversion utility
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
#include <cstring>
#include <memory>
#include <iostream>
#include <zfstream.hpp>
#include <glibmm.h>
#include <giomm.h>

#include "geom.h"
#include "dbobj.h"

/* ---------------------------------------------------------------------- */

class DEMTile;
class TopoDb;

class DirScanner {
        public:
                DirScanner(void);
                ~DirScanner();
                void scan_directory(const Glib::ustring& dir);
                void save_tiles(TopoDb30& db);

                class HdrDict : protected std::map<std::string,std::string> {
                        public:
                                HdrDict(void) {}
                                bool add(const std::string& k, const std::string& v);
                                bool get(const std::string& k, std::string& v) const;
                                bool get(const std::string& k, int& v) const;
                                bool get(const std::string& k, double& v) const;

                        protected:
                                typedef std::map<std::string,std::string>::const_iterator const_iterator;
                                typedef std::map<std::string,std::string>::const_reverse_iterator const_reverse_iterator;
                                typedef std::map<std::string,std::string>::iterator iterator;
                                typedef std::map<std::string,std::string>::reverse_iterator reverse_iterator;
                                typedef std::map<std::string,std::string>::value_type value_type;
                                typedef std::map<std::string,std::string>::key_type key_type;
                                typedef std::map<std::string,std::string>::pointer pointer;
                                typedef std::map<std::string,std::string>::const_reference const_reference;
                                typedef std::map<std::string,std::string>::reference reference;
                };

        private:
                static const char *hdrextensions[];
                typedef std::vector<DEMTile *> demtiles_t;
                demtiles_t m_demtiles;

                void parse_header(const Glib::ustring& fn);
};

class DEMTile {
        public:
                static DEMTile *create(const Glib::ustring& fname, const DirScanner::HdrDict& dict);
                ~DEMTile();
                void save(TopoDb30& db);
                const Glib::ustring& get_name(void) const { return m_filename; }

                class Sorter {
                        public:
                                Sorter(void) {}
                                bool operator()(const DEMTile& t1, const DEMTile& t2) const { return t1.get_name() < t2.get_name(); }
                                bool operator()(const DEMTile *t1, const DEMTile *t2) const { return operator()(*t1, *t2); }
                                bool operator()(const std::unique_ptr<DEMTile> t1, const std::unique_ptr<DEMTile> t2) const { return operator()(t1.get(), t2.get()); }
                };

        private:
                DEMTile(const Glib::ustring& fname, double lon, double lat, double dlon, double dlat, unsigned int ncols, unsigned int nrows, int nodata);
                void load_dem(void);
                void clear_dem(void);

                static constexpr double tilesize = 1.0;

                Glib::ustring m_filename;
                double m_lon;   // west edge of the most southwest DEM pixel
                double m_lat;   // south edge of the most southwest DEM pixel
                double m_dlon;  // longitudinal length of a DEM pixel
                double m_dlat;  // latitudinal length of a DEM pixel
                unsigned int m_ncols;
                unsigned int m_nrows;
                int16_t m_nodata;
                int16_t *m_elev;
};

bool DirScanner::HdrDict::add(const std::string & k, const std::string & v)
{
        std::pair<iterator, bool> x(insert(value_type(k, v)));
        return x.second;
}

bool DirScanner::HdrDict::get(const std::string & k, std::string & v) const
{
        v.clear();
        const_iterator i(find(k));
        if (i == end())
                return false;
        v = i->second;
        return true;
}

bool DirScanner::HdrDict::get(const std::string & k, int & v) const
{
        std::string s;
        v = 0;
        if (!get(k, s))
                return false;
        char *cp;
        v = strtol(s.c_str(), &cp, 0);
        return !*cp;
}

bool DirScanner::HdrDict::get(const std::string & k, double & v) const
{
        std::string s;
        v = 0;
        if (!get(k, s))
                return false;
        char *cp;
        v = strtod(s.c_str(), &cp);
        return !*cp;
}

const char *DirScanner::hdrextensions[] = {
        ".hdr",
        ".hdr.zip",
        ".hdr.bz2",
        ".hdr.gz"
};

DirScanner::DirScanner(void )
{
        //hdrextensions.push_back(Glib::ustring(".hdr").casefold());
        //hdrextensions.push_back(Glib::ustring(".hdr.zip").casefold());
        //hdrextensions.push_back(Glib::ustring(".hdr.bz2").casefold());
        //hdrextensions.push_back(Glib::ustring(".hdr.gz").casefold());
}

DirScanner::~DirScanner()
{
        for (demtiles_t::iterator ti(m_demtiles.begin()), te(m_demtiles.end()); ti != te; ti++)
                delete *ti;
        m_demtiles.clear();
}

void DirScanner::scan_directory(const Glib::ustring& dir)
{
        Glib::Dir d(dir);
        for (Glib::Dir::const_iterator di(d.begin()), de(d.end()); di != de; ++di) {
                Glib::ustring dname(*di);
                Glib::ustring dnamec(dname.casefold());
                Glib::ustring dnamep(Glib::build_filename(dir, dname));
                Glib::RefPtr<Gio::File> df(Gio::File::create_for_path(dnamep));
                Glib::RefPtr<Gio::FileInfo> fi(df->query_info(G_FILE_ATTRIBUTE_STANDARD_TYPE, Gio::FILE_QUERY_INFO_NONE));
                switch (fi->get_file_type()) {
                case Gio::FILE_TYPE_DIRECTORY:
                        //std::cerr << "directory: " << dnamep << std::endl;
                        //if (dname == ".." || dname == ".")
                        //        break;
                        scan_directory(dnamep);
                        break;

                case Gio::FILE_TYPE_REGULAR:
                {
                        bool ok(false);
                        for (const char **cp = hdrextensions; *cp && !ok; cp++) {
                                Glib::ustring he(*cp);
                                he = he.casefold();
                                if (dnamec.size() <= he.size())
                                        continue;
                                ok |= dnamec.substr(dnamec.size() - he.size()) == he;
                        }
                        if (!ok) {
                                //std::cerr << "ignoring file " << dnamep << std::endl;
                                break;
                        }
                        parse_header(dnamep);
                        break;
                }

                default:
                        std::cerr << "unknown type: " << fi->get_file_type() << " file " << dnamep << std::endl;
                        break;
                }
        }
}

void DirScanner::parse_header(const Glib::ustring & fn)
{
        std::cerr << "parsing header: " << fn << std::endl;
        std::unique_ptr<std::istream> is(zfstream::get_istream(fn, true));
        if (!is.get()) {
                std::cerr << "cannot open file " << fn << std::endl;
                return;
        }
        HdrDict dict;
#if 1
        for (;;) {
                std::string line;
                std::istream::int_type ch;
                for (;;) {
                        ch = is->get();
                        if (ch == std::istream::traits_type::eof())
                                break;
                        if (ch == '\r' || ch == '\n')
                                break;
                        line.push_back((char)ch);
                }
                if (ch == std::istream::traits_type::eof())
                        break;
                if (line.empty())
                        continue;
                //std::cerr << "line: " << line << std::endl;
                std::istringstream iss(line);
                Glib::ustring skey, sval;
                iss >> skey;
                iss >> sval;
                //std::cerr << "key: " << skey << " val: \"" << sval << "\"" << std::endl;
                dict.add(skey, sval);
        }
#else
        Glib::RefPtr<Glib::StreamIOChannel> chan = Glib::StreamIOChannel::create(*is);
        chan->set_line_term("\r\n");
        chan->set_close_on_unref(true);
        for (;;) {
                Glib::ustring line;
                Glib::IOStatus st(chan->read_line(line));
                if (st == Glib::IO_STATUS_AGAIN)
                        continue;
                if (st != Glib::IO_STATUS_NORMAL)
                        break;
                if (line.empty())
                        continue;
                //std::cerr << "line: " << line << std::endl;
                std::istringstream iss(line);
                Glib::ustring skey, sval;
                iss >> skey;
                iss >> sval;
                //std::cerr << "key: " << skey << " val: \"" << sval << "\"" << std::endl;
                dict.add(skey, sval);
        }
        chan.clear();
#endif
        Glib::ustring demfile(fn);
        {
                Glib::ustring::size_type ext(demfile.lowercase().rfind(".hdr"));
                if (ext == Glib::ustring::npos) {
                        std::cerr << "cannot find .hdr extension in " << demfile << std::endl;
                        return;
                }
                char rep[4];
                strcpy(rep, "dem");
                for (int i = 0; rep[i]; i++)
                        if (Glib::Unicode::isupper(demfile[ext+1+i]))
                                rep[i] = Glib::Ascii::toupper(rep[i]);
                demfile.replace(ext+1, 3, rep);
        }
        DEMTile *tile(DEMTile::create(demfile, dict));
        if (!tile) {
                std::cerr << "tile creation " << demfile << " failed" << std::endl;
                return;
        }
        m_demtiles.push_back(tile);
}

void DirScanner::save_tiles(TopoDb30 & db)
{
        for (demtiles_t::iterator ti(m_demtiles.begin()), te(m_demtiles.end()); ti != te; ti++)
                (*ti)->save(db);
}

/* ---------------------------------------------------------------------- */

constexpr double DEMTile::tilesize;

DEMTile::DEMTile(const Glib::ustring & fname, double lon, double lat, double dlon, double dlat, unsigned int ncols, unsigned int nrows, int nodata)
        : m_filename(fname), m_lon(lon), m_lat(lat), m_dlon(dlon), m_dlat(dlat), m_ncols(ncols), m_nrows(nrows), m_nodata(nodata), m_elev(0)
{
}

DEMTile::~DEMTile()
{
        clear_dem();
}

DEMTile * DEMTile::create(const Glib::ustring & fname, const DirScanner::HdrDict & dict)
{
        if (fname.empty())
                return 0;
        try {
                Glib::RefPtr<Gio::File> df(Gio::File::create_for_path(fname));
                Glib::RefPtr<Gio::FileInfo> fi(df->query_info(G_FILE_ATTRIBUTE_STANDARD_TYPE, Gio::FILE_QUERY_INFO_NONE));
                if (fi->get_file_type() != Gio::FILE_TYPE_REGULAR)
                        return 0;
        } catch (const Gio::Error& e) {
                std::cerr << "DEMTile::create: file " << fname << " error " << e.what() << std::endl;
        }
        std::string byteorder, layout;
        int nrows, ncols, nbands, nbits, bandrowbytes, totalrowbytes, bandgapbytes, nodata;
        double ulxmap, ulymap, xdim, ydim;
        if (!(dict.get("BYTEORDER", byteorder) &&
              dict.get("LAYOUT", layout) &&
              dict.get("NROWS", nrows) &&
              dict.get("NCOLS", ncols) &&
              dict.get("NBANDS", nbands) &&
              dict.get("NBITS", nbits) &&
              dict.get("BANDROWBYTES", bandrowbytes) &&
              dict.get("TOTALROWBYTES", totalrowbytes) &&
              dict.get("BANDGAPBYTES", bandgapbytes) &&
              dict.get("NODATA", nodata) &&
              dict.get("ULXMAP", ulxmap) &&
              dict.get("ULYMAP", ulymap) &&
              dict.get("XDIM", xdim) &&
              dict.get("YDIM", ydim)))
                return 0;
        if (byteorder != "M" ||
            layout != "BIL" ||
            nbands != 1 ||
            nbits != 16 ||
            bandrowbytes != 2 * ncols ||
            totalrowbytes != 2 * ncols ||
            bandgapbytes ||
            nrows <= 0 ||
            ncols <= 0 ||
            xdim <= 0.0 ||
            ydim <= 0.0)
                return 0;
        DEMTile *t = new DEMTile(fname, ulxmap - 0.5 * xdim, ulymap + (0.5 - nrows) * ydim, xdim, ydim, ncols, nrows, nodata);
        return t;
}


void DEMTile::load_dem(void)
{
        if (m_elev)
                return;
        std::cerr << "parsing DEM: " << m_filename << std::endl;
        std::unique_ptr<std::istream> is(zfstream::get_istream(m_filename, true));
        if (!is.get()) {
                std::cerr << "cannot open file " << m_filename << std::endl;
                return;
        }
        unsigned int els(m_nrows * m_ncols);
        std::vector<uint8_t> idata;
        idata.resize(2*els+1, 0);
        is->read((char *)&idata.front(), 2*els+1);
        if (is->gcount() != 2*els || !is->eof()) {
                std::cerr << "file size error: " << m_filename << " expected " << (2*els) << " got " << is->gcount() << std::endl;
                return;
        }
        m_elev = new int16_t[els];
        for (unsigned int i = 0; i < els; i++) {
                int16_t v((idata[2*i] << 8) | (idata[2*i+1] & 0xff));
                if (v == m_nodata)
                        v = TopoDb30::nodata;
                m_elev[i] = v;
        }
}

void DEMTile::clear_dem(void)
{
        delete[] m_elev;
        m_elev = 0;
}

void DEMTile::save(TopoDb30 & db)
{
        load_dem();
        {
                Point psw, pne;
                psw.set_lon_deg_dbl(m_lon);
                psw.set_lat_deg_dbl(m_lat);
                pne.set_lon_deg_dbl(m_dlon * m_ncols);
                pne.set_lat_deg_dbl(m_dlat * m_nrows);
                pne += psw;
                std::cerr << "DEM: saving " << Rect(psw, pne) << std::endl;
        }
        // note: data is in row major mode; first row 1, then row 2. rows encode latitude!
        for (unsigned int y = 0; y < m_nrows; y++) {
                double lat = m_lat + m_dlat * y + 0.5 * m_dlat;
                Point pt;
                pt.set_lat_deg_dbl(lat);
                for (unsigned int x = 0; x < m_ncols; x++) {
                        double lon = m_lon + m_dlon * x + 0.5 * m_dlon;
                        pt.set_lon_deg_dbl(lon);
                        TopoDb30::TopoCoordinate tc(pt);
                        TopoDb30::elev_t elev(m_elev[(m_nrows - 1 - y) * m_ncols + x]);
                        //tc.print(std::cerr << "Point " << pt << " TopoCoord ") << ' ' << ((Point)tc) << " elev " << elev << std::endl;
                        if (elev != TopoDb30::nodata)
                                db.set_elev(tc, elev);
                }
        }
        clear_dem();
        db.cache_commit();
}

/* ---------------------------------------------------------------------- */

int main(int argc, char *argv[])
{
        static struct option long_options[] = {
                { "help",               no_argument,       NULL, 'h' },
                { "version",            no_argument,       NULL, 'v' },
                { "quiet",              no_argument,       NULL, 'q' },
                { "output-dir",         required_argument, NULL, 'o' },
                { "gtopo-dir",          required_argument, NULL, 'g' },
                { "nopurge",            no_argument,       NULL, 'n' },
                { "min-lat",            required_argument, NULL, 0x400 },
                { "max-lat",            required_argument, NULL, 0x401 },
                { "min-lon",            required_argument, NULL, 0x402 },
                { "max-lon",            required_argument, NULL, 0x403 },
                { NULL,                 0,                 NULL, 0 }
        };
        int c, err(0);
        bool quiet(false), purge(true);
        Glib::ustring output_dir(".");
        Glib::ustring gtopo_dir("/usr/local/download/srtm/version2/SRTM30");
        Rect bbox(Point(Point::lon_min, Point::lat_min), Point(Point::lon_max, Point::lat_max));

        while (((c = getopt_long(argc, argv, "hvqwd:o:k:g:a:W:n", long_options, NULL)) != -1)) {
                switch (c) {
                        case 'v':
                                std::cout << argv[0] << ": (C) 2007 Thomas Sailer" << std::endl;
                                return 0;

                        case 'o':
                                output_dir = optarg;
                                break;

                        case 'g':
                                gtopo_dir = optarg;
                                break;

                        case 'q':
                                quiet = true;
                                break;

                        case 'n':
                                purge = false;
                                break;

                        case 0x400:
                        {
                                Point sw(bbox.get_southwest());
                                Point ne(bbox.get_northeast());
                                if (!sw.set_lat_str(optarg))
                                        sw.set_lat_deg(strtod(optarg, NULL));
                                bbox = Rect(sw, ne);
                                break;
                        }

                        case 0x401:
                        {
                                Point sw(bbox.get_southwest());
                                Point ne(bbox.get_northeast());
                                if (!ne.set_lat_str(optarg))
                                        ne.set_lat_deg(strtod(optarg, NULL));
                                bbox = Rect(sw, ne);
                                break;
                        }

                        case 0x402:
                        {
                                Point sw(bbox.get_southwest());
                                Point ne(bbox.get_northeast());
                                if (!sw.set_lon_str(optarg))
                                        sw.set_lon_deg(strtod(optarg, NULL));
                                bbox = Rect(sw, ne);
                                break;
                        }

                        case 0x403:
                        {
                                Point sw(bbox.get_southwest());
                                Point ne(bbox.get_northeast());
                                if (!ne.set_lon_str(optarg))
                                        ne.set_lon_deg(strtod(optarg, NULL));
                                bbox = Rect(sw, ne);
                                break;
                        }

                        case 'h':
                        default:
                                err++;
                                break;
                }
        }
        if (err) {
                std::cerr << "usage: srtm30 [-q] [-p port] [<file>]" << std::endl
                          << "     -q                quiet" << std::endl
                          << "     -h, --help        Display this information" << std::endl
                          << "     -v, --version     Display version information" << std::endl
                          << "     -g, --gtopo-dir   GTopo (or SRTM30) Directory" << std::endl
                          << "     -o, --output-dir  Database Output Directory" << std::endl << std::endl;
                return EX_USAGE;
        }
        std::cout << "DB Boundary: " << (std::string)bbox.get_southwest().get_lat_str()
                  << ' ' << (std::string)bbox.get_southwest().get_lon_str()
                  << " -> " << (std::string)bbox.get_northeast().get_lat_str()
                  << ' ' << (std::string)bbox.get_northeast().get_lon_str() << std::endl;
        try {
		Glib::init();
		Gio::init();
                DirScanner ds;
                ds.scan_directory(gtopo_dir);
                TopoDb30 db;
                db.open(output_dir);
                if (purge)
                        db.purgedb();
                db.sync_off();
                ds.save_tiles(db);


                db.analyze();
                db.vacuum();
                db.close();
        } catch (const std::exception& e) {
                std::cerr << "Error: " << e.what() << std::endl;
                return EX_DATAERR;
        } catch (const Glib::Exception& e) {
                std::cerr << "Glib Error: " << e.what() << std::endl;
                return EX_DATAERR;
        }
        return 0;
}
