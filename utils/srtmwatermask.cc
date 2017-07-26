/*
 * srtmwatermask.cc: SRTM to DB water masking utility
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

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

/* AIX requires this to be the first thing in the file.  */
#ifndef __GNUC__
# if HAVE_ALLOCA_H
#  include <alloca.h>
# else
#  ifdef _AIX
#pragma alloca
#  else
#   ifndef alloca /* predefined by HP cc +Olibcalls */
char *alloca ();
#   endif
#  endif
# endif
#endif

#include <unistd.h>
#include <getopt.h>
#include <iostream>
#include <sstream>
#include <sqlite3x.hpp>
#include <sqlite3.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <errno.h>
#include <stdexcept>
#include <stdarg.h>
#include <dirent.h>
#include <sysexits.h>
#include <map>
#include <vector>
#include <limits>
#include <memory>
#if defined(HAVE_GDAL)
#if defined(HAVE_GDAL2)
#include <gdal.h>
#include <ogr_geometry.h>
#include <ogr_feature.h>
#include <ogrsf_frmts.h>
#else
#include <ogrsf_frmts.h>
#endif
#endif

#include "geom.h"
#include "dbobj.h"

/* ---------------------------------------------------------------------- */

class SWBDTile;
class TopoDb;

class DirScanner {
        public:
                DirScanner(void);
                ~DirScanner();
                void scan_directory(const Glib::ustring& dir);
                void save_tiles(TopoDb30& db);

        private:
                static const char *hdrextensions[];
                typedef std::vector<SWBDTile *> swbdtiles_t;
                swbdtiles_t m_swbdtiles;
};

class SWBDTile {
        public:
                static SWBDTile *create(const Glib::ustring& fname);
                ~SWBDTile();
                void save(TopoDb30& db);
                std::ostream& info(std::ostream& os);
                const Glib::ustring& get_name(void) const { return m_filename; }

                class Sorter {
                        public:
                                Sorter(void) {}
                                bool operator()(const SWBDTile& t1, const SWBDTile& t2) const { return t1.get_name() < t2.get_name(); }
                                bool operator()(const SWBDTile *t1, const SWBDTile *t2) const { return operator()(*t1, *t2); }
                                bool operator()(const std::unique_ptr<SWBDTile> t1, const std::unique_ptr<SWBDTile> t2) const { return operator()(t1.get(), t2.get()); }
                };

        private:
#if defined(HAVE_GDAL2)
                SWBDTile(const Glib::ustring& fname, GDALDataset *ds);
#else
                SWBDTile(const Glib::ustring& fname, OGRDataSource *ds);
#endif
                void open(void);
                void close(void);

                Glib::ustring m_filename;
#if defined(HAVE_GDAL2)
                GDALDataset *m_ds;
#else
                OGRDataSource *m_ds;
#endif
};

const char *DirScanner::hdrextensions[] = {
        ".shp",
        ".SHP"
};

DirScanner::DirScanner(void)
{
}

DirScanner::~DirScanner()
{
        for (swbdtiles_t::iterator ti(m_swbdtiles.begin()), te(m_swbdtiles.end()); ti != te; ti++)
                delete *ti;
        m_swbdtiles.clear();
}

void DirScanner::scan_directory(const Glib::ustring & dir)
{
        DIR *d = opendir(dir.c_str());
        if (!d) {
                std::cerr << "cannot open directory " << dir << " errno " << errno << " error " << strerror(errno) << std::endl;
                return;
        }
        struct dirent *de;
        while ((de = readdir(d))) {
#ifndef _DIRENT_HAVE_D_TYPE
#error "need dirent d_type"
#endif
                Glib::ustring dname(de->d_name);
                Glib::ustring dnamec(dname.casefold());
                Glib::ustring dnamep(dir + "/" + de->d_name);
                switch (de->d_type) {
                case DT_DIR:
                        //std::cerr << "directory: " << dnamep << std::endl;
                        if (dname == ".." || dname == ".")
                                break;
                        scan_directory(dnamep);
                        break;

                case DT_REG:
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
                        //std::cerr << "trying file " << dnamep << std::endl;
                        SWBDTile *tile(SWBDTile::create(dnamep));
                        if (!tile) {
                                std::cerr << "tile creation " << dnamep << " failed" << std::endl;
                                return;
                        }
                        m_swbdtiles.push_back(tile);
                        break;
                }

                        default:
                        std::cerr << "unknown type: " << de->d_type << " file " << dname << std::endl;
                        break;
                }
        }
        closedir(d);
}

void DirScanner::save_tiles(TopoDb30 & db)
{
        std::sort(m_swbdtiles.begin(), m_swbdtiles.end(), SWBDTile::Sorter());
        for (swbdtiles_t::iterator ti(m_swbdtiles.begin()), te(m_swbdtiles.end()); ti != te; ti++)
                (*ti)->save(db);
}

/* ---------------------------------------------------------------------- */

#if defined(HAVE_GDAL2)
SWBDTile::SWBDTile(const Glib::ustring& fname, GDALDataset *ds)
#else
SWBDTile::SWBDTile(const Glib::ustring& fname, OGRDataSource *ds)
#endif
        : m_filename(fname), m_ds(ds)
{
        // save some memory for now
        close();
}

SWBDTile::~SWBDTile()
{
        close();
}

void SWBDTile::open(void)
{
        if (m_ds)
                return;
#if defined(HAVE_GDAL2)
	m_ds = (GDALDataset *)GDALOpenEx(m_filename.c_str(), GDAL_OF_READONLY, 0, 0, 0);
#else
        OGRSFDriver *drv(0);
        m_ds = OGRSFDriverRegistrar::Open(m_filename.c_str(), false, &drv);
#endif
        if (m_ds == NULL)
                throw std::runtime_error("Cannot open " + m_filename);
}

void SWBDTile::close(void)
{
#if defined(HAVE_GDAL2)
	GDALClose(m_ds);
#else
	OGRDataSource::DestroyDataSource(m_ds);
#endif
        m_ds = 0;
}

SWBDTile * SWBDTile::create(const Glib::ustring & fname)
{
        if (fname.empty())
                return 0;
#if defined(HAVE_GDAL2)
	GDALDataset *ds((GDALDataset *)GDALOpenEx(fname.c_str(), GDAL_OF_READONLY, 0, 0, 0));
#else
        OGRSFDriver *drv(0);
        OGRDataSource *ds = OGRSFDriverRegistrar::Open(fname.c_str(), false, &drv);
#endif
        if (ds == NULL)
                return 0;
        SWBDTile *t = new SWBDTile(fname, ds);
        return t;
}

std::ostream & SWBDTile::info(std::ostream & os)
{
        open();
#if !defined(HAVE_GDAL2)
        os << "Tile Name: " << m_ds->GetName() << std::endl;
#endif
        for (int layernr = 0; layernr < m_ds->GetLayerCount(); layernr++) {
                OGRLayer *layer(m_ds->GetLayer(layernr));
                OGRFeatureDefn *layerdef(layer->GetLayerDefn());
                os << "  Layer Name: " << layerdef->GetName() << std::endl;
                for (int fieldnr = 0; fieldnr < layerdef->GetFieldCount(); fieldnr++) {
                        OGRFieldDefn *fielddef(layerdef->GetFieldDefn(fieldnr));
                        os << "    Field Name: " << fielddef->GetNameRef()
                           << " type " << OGRFieldDefn::GetFieldTypeName(fielddef->GetType())  << std::endl;
                }
                layer->ResetReading();
                while (OGRFeature *feature = layer->GetNextFeature()) {
                        os << "  Feature: " << feature->GetFID() << std::endl;
                        for (int fieldnr = 0; fieldnr < layerdef->GetFieldCount(); fieldnr++) {
                                OGRFieldDefn *fielddef(layerdef->GetFieldDefn(fieldnr));
                                os << "    Field Name: " << fielddef->GetNameRef() << " Value ";
                                switch (fielddef->GetType()) {
                                        case OFTInteger:
                                                os << feature->GetFieldAsInteger(fieldnr);
                                                break;

                                        case OFTReal:
                                                os << feature->GetFieldAsDouble(fieldnr);
                                                break;

                                        case OFTString:
                                        default:
                                                os << feature->GetFieldAsString(fieldnr);
                                                break;
                                }
                                os << std::endl;
                        }
                        OGRGeometry *geom(feature->GetGeometryRef());
                        char *wkt;
                        geom->exportToWkt(&wkt);
                        os << "  Geom: " << wkt << std::endl;
                        delete wkt;
                }
        }
        close();
        return os;
}

void SWBDTile::save(TopoDb30 & db)
{
        open();
#if defined(HAVE_GDAL2)
        std::cerr << "Processing tile file " << m_filename << std::endl;
#else
        std::cerr << "Processing tile " << m_ds->GetName() << " (file " << m_filename << ')' << std::endl;
#endif
        for (int layernr = 0; layernr < m_ds->GetLayerCount(); layernr++) {
                OGRLayer *layer(m_ds->GetLayer(layernr));
                OGRFeatureDefn *layerdef(layer->GetLayerDefn());
                int facc_index(layerdef->GetFieldIndex("FACC_CODE"));
                if (facc_index == -1) {
                        std::cerr << "Layer Name: " << layerdef->GetName() << " has no FACC_CODE field, skipping" << std::endl;
                        continue;
                }
                layer->ResetReading();
                while (OGRFeature *feature = layer->GetNextFeature()) {
                        const char *facc_code(feature->GetFieldAsString(facc_index));
                        if (!facc_code || strcmp(facc_code, "BA040")) {
                                //std::cerr << "Layer Name: " << layerdef->GetName() << " Feature: " << feature->GetFID() << " has FACC_CODE " << facc_code << " skipping" << std::endl;
                                continue;
                        }
                        OGRGeometry *geom(feature->GetGeometryRef());
                        std::cerr << "Layer Name: " << layerdef->GetName() << " Feature: " << feature->GetFID() << " Geom: " << geom->getGeometryName() << std::endl;
                        Point pdim(TopoDb30::TopoCoordinate::get_pointsize()), pdim2(pdim.get_lon()/2, pdim.get_lat()/2);
                        TopoDb30::TopoTileCoordinate tmin, tmax;
                        {
                                OGREnvelope env;
                                geom->getEnvelope(&env);
                                Point psw, pne;
                                psw.set_lon_deg_dbl(env.MinX);
                                psw.set_lat_deg_dbl(env.MinY);
                                pne.set_lon_deg_dbl(env.MaxX);
                                pne.set_lat_deg_dbl(env.MaxY);
                                tmin = TopoDb30::TopoTileCoordinate(psw);
                                tmax = TopoDb30::TopoTileCoordinate(pne);
                        }
                        for (TopoDb30::TopoTileCoordinate tc(tmin);;) {
                                TopoDb30::pixel_index_t ymin((tc.get_lat_tile() == tmin.get_lat_tile()) ? tmin.get_lat_offs() : 0);
                                TopoDb30::pixel_index_t ymax((tc.get_lat_tile() == tmax.get_lat_tile()) ? tmax.get_lat_offs() : TopoDb30::tile_size - 1);
                                TopoDb30::pixel_index_t xmin((tc.get_lon_tile() == tmin.get_lon_tile()) ? tmin.get_lon_offs() : 0);
                                TopoDb30::pixel_index_t xmax((tc.get_lon_tile() == tmax.get_lon_tile()) ? tmax.get_lon_offs() : TopoDb30::tile_size - 1);
                                Rect rtile((Point)(TopoDb30::TopoCoordinate)TopoDb30::TopoTileCoordinate(tc.get_lon_tile(), tc.get_lat_tile(), xmin, ymin),
                                           (Point)(TopoDb30::TopoCoordinate)TopoDb30::TopoTileCoordinate(tc.get_lon_tile(), tc.get_lat_tile(), xmax, ymax) + pdim);
                                OGRPolygon rtilepoly;
                                {
                                        OGRLinearRing rring;
                                        rring.addPoint(rtile.get_southwest().get_lon_deg_dbl(), rtile.get_southwest().get_lat_deg_dbl(), 0);
                                        rring.addPoint(rtile.get_northwest().get_lon_deg_dbl(), rtile.get_northwest().get_lat_deg_dbl(), 0);
                                        rring.addPoint(rtile.get_northeast().get_lon_deg_dbl(), rtile.get_northeast().get_lat_deg_dbl(), 0);
                                        rring.addPoint(rtile.get_southeast().get_lon_deg_dbl(), rtile.get_southeast().get_lat_deg_dbl(), 0);
                                        rring.closeRings();
                                        rtilepoly.addRing(&rring);
                                }
                                if (geom->Contains(&rtilepoly)) {
                                        for (TopoDb30::pixel_index_t y(ymin); y <= ymax; y++) {
                                                tc.set_lat_offs(y);
                                                for (TopoDb30::pixel_index_t x(xmin); x <= xmax; x++) {
                                                        tc.set_lon_offs(x);
                                                        db.set_elev(tc, TopoDb30::ocean);
                                                }
                                        }
                                } else if (geom->Intersects(&rtilepoly)) {
                                        for (TopoDb30::pixel_index_t y(ymin); y <= ymax; y++) {
                                                tc.set_lat_offs(y);
                                                for (TopoDb30::pixel_index_t x(xmin); x <= xmax; x++) {
                                                        tc.set_lon_offs(x);
                                                        OGRPoint opt;
                                                        {
                                                                Point pt((TopoDb30::TopoCoordinate)tc);
                                                                opt.setX(pt.get_lon_deg_dbl());
                                                                opt.setY(pt.get_lat_deg_dbl());
                                                                opt.setZ(0);
                                                        }
                                                        if (!geom->Contains(&opt))
                                                                continue;
                                                        db.set_elev(tc, TopoDb30::ocean);
                                                }
                                        }
                                }
                                if (tc.get_lon_tile() != tmax.get_lon_tile()) {
                                        tc.advance_tile_east();
                                        continue;
                                }
                                if (tc.get_lat_tile() == tmax.get_lat_tile())
                                        break;
                                tc.set_lon_tile(tmin.get_lon_tile());
                                tc.advance_tile_north();
                        }

                }
        }
        //info(std::cerr);
        close();
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
                { "swbd-dir",           required_argument, NULL, 'g' },
                { "purge",              no_argument,       NULL, 'p' },
                { "min-lat",            required_argument, NULL, 0x400 },
                { "max-lat",            required_argument, NULL, 0x401 },
                { "min-lon",            required_argument, NULL, 0x402 },
                { "max-lon",            required_argument, NULL, 0x403 },
                { NULL,                 0,                 NULL, 0 }
        };
        int c, err(0);
        bool quiet(false), purge(false);
        Glib::ustring output_dir(".");
        Glib::ustring swbd_dir("/usr/local/download/srtm/version2/SRTM30");
        Rect bbox;

        while (((c = getopt_long(argc, argv, "hvqo:k:s:p", long_options, NULL)) != -1)) {
                switch (c) {
                        case 'v':
                                std::cout << argv[0] << ": (C) 2007 Thomas Sailer" << std::endl;
                                return 0;

                        case 'o':
                                output_dir = optarg;
                                break;

                        case 's':
                                swbd_dir = optarg;
                                break;

                        case 'q':
                                quiet = true;
                                break;

                        case 'p':
                                purge = true;
                                break;

                        case 0x400:
                        {
                                Point sw(bbox.get_southwest());
                                Point ne(bbox.get_northeast());
                                sw.set_lat_deg(strtod(optarg, NULL));
                                bbox = Rect(sw, ne);
                                break;
                        }

                        case 0x401:
                        {
                                Point sw(bbox.get_southwest());
                                Point ne(bbox.get_northeast());
                                ne.set_lat_deg(strtod(optarg, NULL));
                                bbox = Rect(sw, ne);
                                break;
                        }

                        case 0x402:
                        {
                                Point sw(bbox.get_southwest());
                                Point ne(bbox.get_northeast());
                                sw.set_lon_deg(strtod(optarg, NULL));
                                bbox = Rect(sw, ne);
                                break;
                        }

                        case 0x403:
                        {
                                Point sw(bbox.get_southwest());
                                Point ne(bbox.get_northeast());
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
                std::cerr << "usage: srtmwatermask [-q] [-p port] [<file>]" << std::endl
                          << "     -q                quiet" << std::endl
                          << "     -h, --help        Display this information" << std::endl
                          << "     -v, --version     Display version information" << std::endl
                          << "     -s, --swbd-dir    SWBD Directory" << std::endl
                          << "     -o, --output-dir  Database Output Directory" << std::endl << std::endl;
                return EX_USAGE;
        }
        std::cout << "DB Boundary: " << (std::string)bbox.get_southwest().get_lat_str()
                  << ' ' << (std::string)bbox.get_southwest().get_lon_str()
                  << " -> " << (std::string)bbox.get_northeast().get_lat_str()
                  << ' ' << (std::string)bbox.get_northeast().get_lon_str() << std::endl;
        try {
                OGRRegisterAll();
                DirScanner ds;
                ds.scan_directory(swbd_dir);
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
