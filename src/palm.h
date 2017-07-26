//
// C++ Interface: palm
//
// Description: 
//
//
// Author: Thomas Sailer <t.sailer@alumni.ethz.ch>, (C) 2007, 2012
//
// Copyright: See COPYING file that comes with this distribution
//
//

#ifndef PALM_H
#define PALM_H

#include <stdint.h>
#include <arpa/inet.h>
#include <glibmm.h>

#include "dbobj.h"

struct pi_file;

class PalmDbBase
{
        public:
                class PalmStringCompare {
                        public:
                                typedef enum {
                                        comp_startswith,
                                        comp_contains,
                                        comp_exact
                                } comp_t;

                                PalmStringCompare(const Glib::ustring& s, comp_t c = comp_contains) : str(s.casefold()), strln(str.length()), comp(c) {}
                                bool operator==(const Glib::ustring& s) const;
                        private:
                                Glib::ustring str;
                                Glib::ustring::size_type strln;
                                comp_t comp;
                };

                static unsigned int to_tile(const Point& pt);
                static unsigned int next_tile(unsigned int tile, unsigned int tmin, unsigned int tmax);

        protected:
                class PalmInt16 {
                        public:
                                PalmInt16(int16_t x = 0) : v(htons(x)) {}
                                int16_t operator=(int16_t x) { v = htons(x); return x; }
                                operator int16_t(void) const { return ntohs(v); }

                        private:
                                int16_t v __attribute__ ((packed));
                } __attribute__ ((packed));

                class PalmUInt16 {
                        public:
                                PalmUInt16(uint16_t x = 0) : v(htons(x)) {}
                                uint16_t operator=(uint16_t x) { v = htons(x); return x; }
                                operator uint16_t(void) const { return ntohs(v); }

                        private:
                                uint16_t v __attribute__ ((packed));
                } __attribute__ ((packed));

                class PalmInt32 {
                        public:
                                PalmInt32(int32_t x = 0) : v(htonl(x)) {}
                                int32_t operator=(int32_t x) { v = htonl(x); return x; }
                                operator int32_t(void) const { return ntohl(v); }

                        private:
                                int32_t v __attribute__ ((packed));
                } __attribute__ ((packed));

                class PalmUInt32 {
                        public:
                                PalmUInt32(uint32_t x = 0) : v(htonl(x)) {}
                                uint32_t operator=(uint32_t x) { v = htonl(x); return x; }
                                operator uint32_t(void) const { return ntohl(v); }

                        private:
                                uint32_t v __attribute__ ((packed));
                } __attribute__ ((packed));

                class PalmFloat32 {
                        private:
                                typedef union {
                                        float f;
                                        uint32_t u;
                                } cnv_t;

                        public:
                                PalmFloat32(float x = 0) { cnv_t c; c.f = x; v = htonl(c.u); }
                                float operator=(float x) { cnv_t c; c.f = x; v = htonl(c.u); return c.f; }
                                operator float(void) const { cnv_t c; c.u = ntohl(v); return c.f; }

                        private:
                                uint32_t v __attribute__ ((packed));
                } __attribute__ ((packed));

                template <int len> class PalmStrI {
                        public:
                                PalmStrI(const char *x = 0) { strncpy(v, x ? x : "", sizeof(v)); }
                                PalmStrI(const std::string& x) { PalmStrI(x.c_str()); }
                                const char *operator=(const char *x) { strncpy(v, x ? x : "", sizeof(v)); return x; }
                                std::string operator=(const std::string& x) { strncpy(v, x.c_str(), sizeof(v)); return x; }
                                operator std::string(void) const { return std::string(v, strnlen(v, sizeof(v))); }
                                operator Glib::ustring(void) const { return std::string(v, strnlen(v, sizeof(v))); }

                        private:
                                char v[len] __attribute__ ((packed));
                } __attribute__ ((packed));
};

class PalmImage : protected PalmDbBase {
        public:
                PalmImage(void);

        private:
                static const uint16_t map_proj_lambert = 0;
                static const uint16_t map_proj_linear = 1;

                class Header {
                        public:
                                PalmUInt16 version;
                                PalmUInt16 sizex;
                                PalmUInt16 sizey;
                                /* projection stuff */
                                PalmUInt16 projection;
                                PalmUInt16 xmin, xmax, ymin, ymax;
                                PalmUInt32 scale;
                                PalmInt32 latmin, latmax, lonmin, lonmax;
                                PalmInt32 reflon;
                                PalmFloat32 atfwd[6];
                                PalmFloat32 atrev[4];
                                /* lambert */
                                PalmFloat32 lambert_n;
                                PalmFloat32 lambert_f;
                                PalmFloat32 lambert_rho0;
                } __attribute__ ((packed));
};

template <class PalmDb> class PalmFindByName : public PalmDb {
        public:
                typedef typename PalmDb::id_t id_t;
                typedef typename PalmDb::entry_t entry_t;
                typedef typename std::vector<entry_t> entryvector_t;
                typedef PalmDbBase::PalmStringCompare::comp_t comp_t;
                static const comp_t comp_startswith = PalmDbBase::PalmStringCompare::comp_startswith;
                static const comp_t comp_contains = PalmDbBase::PalmStringCompare::comp_contains;
                static const comp_t comp_exact = PalmDbBase::PalmStringCompare::comp_exact;

                entryvector_t find_by_name(const Glib::ustring& nm = "", unsigned int limit = ~0, unsigned int skip = 0, comp_t comp = comp_contains);
};

template <class PalmDb> class PalmFindByIcao : public PalmDb {
        public:
                typedef typename PalmDb::id_t id_t;
                typedef typename PalmDb::entry_t entry_t;
                typedef typename std::vector<entry_t> entryvector_t;
                typedef PalmDbBase::PalmStringCompare::comp_t comp_t;
                static const comp_t comp_startswith = PalmDbBase::PalmStringCompare::comp_startswith;
                static const comp_t comp_contains = PalmDbBase::PalmStringCompare::comp_contains;
                static const comp_t comp_exact = PalmDbBase::PalmStringCompare::comp_exact;

                entryvector_t find_by_icao(const Glib::ustring& nm = "", unsigned int limit = ~0, unsigned int skip = 0, comp_t comp = comp_contains);
};

template <class PalmDb> class PalmFindByText : public PalmDb {
        public:
                typedef typename PalmDb::id_t id_t;
                typedef typename PalmDb::entry_t entry_t;
                typedef typename std::vector<entry_t> entryvector_t;
                typedef PalmDbBase::PalmStringCompare::comp_t comp_t;
                static const comp_t comp_startswith = PalmDbBase::PalmStringCompare::comp_startswith;
                static const comp_t comp_contains = PalmDbBase::PalmStringCompare::comp_contains;
                static const comp_t comp_exact = PalmDbBase::PalmStringCompare::comp_exact;

                entryvector_t find_by_text(const Glib::ustring& nm = "", unsigned int limit = ~0, unsigned int skip = 0, comp_t comp = comp_contains);
};

template <class PalmDb> class PalmFindNearest : public PalmDb {
        public:
                typedef typename PalmDb::id_t id_t;
                typedef typename PalmDb::entry_t entry_t;
                typedef typename std::vector<entry_t> entryvector_t;
                typedef PalmDbBase::PalmStringCompare::comp_t comp_t;
                static const comp_t comp_startswith = PalmDbBase::PalmStringCompare::comp_startswith;
                static const comp_t comp_contains = PalmDbBase::PalmStringCompare::comp_contains;
                static const comp_t comp_exact = PalmDbBase::PalmStringCompare::comp_exact;

                entryvector_t find_nearest(const Point& pt, unsigned int limit = ~0, const Rect& rect = Rect());

        private:
                class less_dist : public std::binary_function<const entry_t&, const entry_t&, bool> {
                        public:
                                less_dist(const Point& ptx): pt(ptx) {}
                                bool operator()(const entry_t& x, const entry_t& y) const {
                                        return pt.simple_distance_rel(x.get_coord()) < pt.simple_distance_rel(y.get_coord());
                                }
                        private:
                                Point pt;
                };
};

template <class PalmDb> class PalmFindBboxPoint : public PalmDb {
        public:
                typedef typename PalmDb::id_t id_t;
                typedef typename PalmDb::entry_t entry_t;
                typedef typename std::vector<entry_t> entryvector_t;
                typedef PalmDbBase::PalmStringCompare::comp_t comp_t;
                static const comp_t comp_startswith = PalmDbBase::PalmStringCompare::comp_startswith;
                static const comp_t comp_contains = PalmDbBase::PalmStringCompare::comp_contains;
                static const comp_t comp_exact = PalmDbBase::PalmStringCompare::comp_exact;

                entryvector_t find_bbox(const Rect& rect, unsigned int limit = ~0);
};

template <class PalmDb> class PalmFindBboxBox : public PalmDb {
        public:
                typedef typename PalmDb::id_t id_t;
                typedef typename PalmDb::entry_t entry_t;
                typedef typename std::vector<entry_t> entryvector_t;
                typedef PalmDbBase::PalmStringCompare::comp_t comp_t;
                static const comp_t comp_startswith = PalmDbBase::PalmStringCompare::comp_startswith;
                static const comp_t comp_contains = PalmDbBase::PalmStringCompare::comp_contains;
                static const comp_t comp_exact = PalmDbBase::PalmStringCompare::comp_exact;

                entryvector_t find_bbox(const Rect& rect, unsigned int limit = ~0);
};

class PalmAirportBase : protected PalmDbBase {
        public:
                typedef uint16_t id_t;
                typedef AirportsDb::Airport entry_t;
                typedef std::vector<entry_t> entryvector_t;

                PalmAirportBase(void);
                ~PalmAirportBase(void);
                void open(const std::string& fname);
                void close(void);
                bool is_open(void) const { return !!fdb; }
                id_t get_nrentries(void) const;
                time_t get_effdate(void) const;
                time_t get_expdate(void) const;
                entry_t operator[](id_t id);

        protected:
                typedef std::vector<id_t> idvector_t;
                idvector_t tile_first_ids(unsigned int tilenr) const;
                id_t tile_next_id(id_t id);

        private:
                static const uint32_t db_type = ('a' << 24) | ('r' << 16) | ('p' << 8) | 't';

                class Header {
                        public:
                                PalmUInt16 version;
                                PalmUInt16 number;
                                PalmUInt32 effdate;
                                PalmUInt32 expdate;
                } __attribute__ ((packed));

                class Runway {
                        public:
                                PalmStrI<4> ident_he;
                                PalmStrI<4> ident_le;
                                PalmUInt16 length;
                                PalmUInt16 width;
                                PalmStrI<4> surface;
                                PalmUInt16 he_tda;
                                PalmUInt16 he_lda;
                                PalmUInt16 le_tda;
                                PalmUInt16 le_lda;
                } __attribute__ ((packed));

                class Comm {
                        public:
                                PalmUInt32 freq[5];
                                PalmUInt16 nameptr;
                                PalmUInt16 sectorptr;
                                PalmUInt16 oprhoursptr;
                                PalmUInt16 dummy;
                } __attribute__ ((packed));

                class VFRRoutePoint {
                        public:
                                PalmInt32 lat;
                                PalmInt32 lon;
                                uint8_t label_placement;
                                char ptsym;
                                char pathcode;
                                char altcode;
                                PalmUInt16 ptnameptr;
                                PalmInt16 alt;
                } __attribute__ ((packed));

                class VFRRoute {
                        public:
                                PalmUInt16 rtenameptr;
                                PalmUInt16 rteptnr;
                                PalmUInt16 rteptptr;
                                PalmUInt16 dummy;
                } __attribute__ ((packed));

                class Rec {
                        public:
                                PalmInt32 lat;
                                PalmInt32 lon;
                                PalmInt16 elev;
                                PalmUInt16 tileidx;
                                PalmStrI<4> icao;
                                PalmUInt16 nameptr;
                                char typecode;
                                uint8_t label_placement;
                                PalmUInt16 rwynr;
                                PalmUInt16 rwyptr;
                                PalmUInt16 commnr;
                                PalmUInt16 commptr;
                                PalmUInt16 vfrrtenr;
                                PalmUInt16 vfrrteptr;
                                PalmUInt16 vfrrmkptr;
                                PalmUInt16 dummy;
                                PalmInt32 latmin;
                                PalmInt32 latmax;
                                PalmInt32 lonmin;
                                PalmInt32 lonmax;
                } __attribute__ ((packed));

                struct pi_file *fdb;
                Header hdr;
                std::vector<PalmUInt16> name_index;
                PalmUInt16 tile_index[128*64];
};

class PalmNavaidBase : protected PalmDbBase {
        public:
                typedef uint16_t id_t;
                typedef NavaidsDb::Navaid entry_t;
                typedef std::vector<entry_t> entryvector_t;

                PalmNavaidBase(void);
                ~PalmNavaidBase(void);
                void open(const std::string& fname);
                void close(void);
                bool is_open(void) const { return !!fdb; }
                id_t get_nrentries(void) const;
                time_t get_effdate(void) const;
                time_t get_expdate(void) const;
                entry_t operator[](id_t id);

                static NavaidsDb::Navaid::label_placement_t decode_label_placement(uint8_t x);

        protected:
                typedef std::vector<id_t> idvector_t;
                idvector_t tile_first_ids(unsigned int tilenr) const;
                id_t tile_next_id(id_t id);

         private:
                static const uint32_t db_type = ('n' << 24) | ('a' << 16) | ('v' << 8) | 'a';

                class Header {
                        public:
                                PalmUInt16 version;
                                PalmUInt16 number;
                                PalmUInt32 effdate;
                                PalmUInt32 expdate;
                } __attribute__ ((packed));

                class Rec {
                        public:
                                PalmInt32 lat;
                                PalmInt32 lon;
                                PalmInt16 elev;
                                PalmUInt16 tileidx;
                                PalmStrI<4> icao;
                                PalmUInt16 nameptr;
                                PalmUInt16 range;
                                PalmUInt32 freq;
                                char typecode;
                                char status;
                                uint8_t label_placement;
                                char dummy;
                } __attribute__ ((packed));

                struct pi_file *fdb;
                Header hdr;
                std::vector<PalmUInt16> name_index;
                PalmUInt16 tile_index[128*64];
};

class PalmAirspaceBase : protected PalmDbBase {
        public:
                typedef uint16_t id_t;
                typedef AirspacesDb::Airspace entry_t;
                typedef std::vector<entry_t> entryvector_t;

                PalmAirspaceBase(void);
                ~PalmAirspaceBase(void);
                void open(const std::string& fname);
                void close(void);
                bool is_open(void) const { return !!fdb; }
                id_t get_nrentries(void) const;
                time_t get_effdate(void) const;
                time_t get_expdate(void) const;
                uint32_t get_altlimit(void) const;
                entry_t operator[](id_t id);

        protected:
                typedef std::vector<id_t> idvector_t;
                idvector_t tile_first_ids(unsigned int tilenr) const;
                id_t tile_next_id(id_t id);

        protected:
                unsigned int to_tile(const Point& pt) { return PalmDbBase::to_tile(pt + Point(hdr.lontileoffset, 0)); }

        private:
                static const uint32_t db_type = ('b' << 24) | ('d' << 16) | ('r' << 8) | 'y';

                class Header {
                        public:
                                PalmUInt16 version;
                                PalmUInt16 number;
                                PalmUInt32 effdate;
                                PalmUInt32 expdate;
                                PalmInt32 lontileoffset;
                                PalmUInt32 altlimit;
                } __attribute__ ((packed));

                class PolyPoint {
                        public:
                                PalmInt32 lat;
                                PalmInt32 lon;
                } __attribute__ ((packed));

                class Rec {
                        public:
                                PalmInt32 latmin;
                                PalmInt32 latmax;
                                PalmInt32 lonmin;
                                PalmInt32 lonmax;
                                PalmUInt16 tileidx;
                                uint8_t typecode;
                                char bdryclass;
                                PalmStrI<4> icao;
                                PalmUInt32 altlwr, altupr;
                                PalmUInt16 gndelevmin, gndelevmax;
                                PalmUInt16 nameptr;
                                PalmUInt16 commnameptr;
                                PalmUInt32 commfreq[2];
                                PalmInt32 labellat;
                                PalmInt32 labellon;
                                PalmUInt16 classexceptptr;
                                PalmUInt16 identptr;
                                PalmUInt16 efftimeptr;
                                PalmUInt16 noteptr;
                                PalmUInt16 dummy;
                                PalmUInt16 polynr;
                                PolyPoint poly[0];
                } __attribute__ ((packed));

                static const uint8_t altflag_agl   = (1 << 0);
                static const uint8_t altflag_fl    = (1 << 1);
                static const uint8_t altflag_sfc   = (1 << 2);
                static const uint8_t altflag_gnd   = (1 << 3);
                static const uint8_t altflag_unlim = (1 << 4);
                static const uint8_t altflag_notam = (1 << 5);
                static const uint8_t altflag_unkn  = (1 << 6);
                /* ptsym flag */
                static const uint8_t ptsym_atairport = (1 << 7);

                struct pi_file *fdb;
                Header hdr;
                PalmUInt16 tile_index[128*64];
};

class PalmWaypointBase : protected PalmDbBase {
        public:
                typedef uint32_t id_t;
                typedef WaypointsDb::Waypoint entry_t;
                typedef std::vector<entry_t> entryvector_t;

                PalmWaypointBase(void);
                ~PalmWaypointBase(void);
                void open(const std::string& fname);
                void close(void);
                bool is_open(void) const { return !!fdb; }
                id_t get_nrentries(void) const;
                time_t get_effdate(void) const;
                time_t get_expdate(void) const;
                entry_t operator[](id_t id);

        protected:
                typedef std::vector<id_t> idvector_t;
                idvector_t tile_first_ids(unsigned int tilenr) const;
                id_t tile_next_id(id_t id);

        private:
                static const uint32_t db_type = ('w' << 24) | ('p' << 16) | ('t' << 8) | 'x';

                class Header {
                        public:
                                PalmUInt16 version;
                                PalmUInt16 dummy;
                                PalmUInt32 number;
                                PalmUInt32 effdate;
                                PalmUInt32 expdate;
                } __attribute__ ((packed));

                class Rec {
                        public:
                                PalmInt32 lat;
                                PalmInt32 lon;
                                PalmUInt32 tileidx;
                                PalmUInt16 nameptr;
                                char usagecode;
                                uint8_t label_placement_backptr;
                } __attribute__ ((packed));

                struct pi_file *fdb;
                Header hdr;
                PalmUInt16 tile_index[128*64];
};

class PalmMapelementBase : protected PalmDbBase {
        public:
                typedef uint32_t id_t;
                typedef MapelementsDb::Mapelement entry_t;
                typedef std::vector<entry_t> entryvector_t;

                PalmMapelementBase(void);
                ~PalmMapelementBase(void);
                void open(const std::string& fname);
                void close(void);
                bool is_open(void) const { return !!fdb; }
                id_t get_nrentries(void) const;
                time_t get_mindate(void) const;
                time_t get_maxdate(void) const;
                entry_t operator[](id_t id);

        protected:
                typedef std::vector<id_t> idvector_t;
                idvector_t tile_first_ids(unsigned int tilenr) const;
                id_t tile_next_id(id_t id);

        private:
                static const uint32_t db_type = ('m' << 24) | ('a' << 16) | ('p' << 8) | 'e';
                static const id_t index_offset = 16;

                static const uint8_t maptyp_railway         = 47;
                static const uint8_t maptyp_railway_d       = 57;
                static const uint8_t maptyp_aerial_cable    = 48;

                static const uint8_t maptyp_highway         = 45;
                static const uint8_t maptyp_road            = 46;
                static const uint8_t maptyp_trail           = 58;

                static const uint8_t maptyp_river           = 50;
                static const uint8_t maptyp_lake            = 49;
                static const uint8_t maptyp_river_t         = 54;
                static const uint8_t maptyp_lake_t          = 55;

                static const uint8_t maptyp_canal           = 51;

                static const uint8_t maptyp_pack_ice        = 56;
                static const uint8_t maptyp_glacier         = 40;
                static const uint8_t maptyp_forest          = 60;

                static const uint8_t maptyp_city            = 42;
                static const uint8_t maptyp_village         = 43;
                static const uint8_t maptyp_spot            = 38;
                static const uint8_t maptyp_landmark        = 44;

                static const uint8_t maptyp_powerline       = 130;
                static const uint8_t maptyp_border          = 134;

                class Header {
                        public:
                                PalmUInt16 version;
                                PalmUInt16 dummy;
                                PalmUInt32 number;
                                PalmUInt32 mindate;
                                PalmUInt32 maxdate;
                } __attribute__ ((packed));

                class PolyPoint {
                        public:
                                PalmInt32 lat;
                                PalmInt32 lon;
                } __attribute__ ((packed));

                class Rec {
                        public:
                                PalmInt32 latmin;
                                PalmInt32 latmax;
                                PalmInt32 lonmin;
                                PalmInt32 lonmax;
                                PalmUInt32 tileidx;
                                uint8_t typecode;
                                uint8_t nrinrsc;
                                PalmUInt16 nameptr;
                                PalmUInt16 polynr;
                                PalmUInt16 polyptr;
                } __attribute__ ((packed));

                struct pi_file *fdb;
                Header hdr;
                PalmUInt32 tile_index[128*64];
                PalmUInt32 name_tile_index[128*64];
};

class PalmGtopo : protected PalmDbBase {
        public:
                class GtopoCoord {
                        public:
                                static constexpr float garmin_to_gtopo30 = (90.0 * 60.0 * 2.0 / (1UL << 30));
                                static constexpr float gtopo30_to_garmin = ((1UL << 30) / 90.0 / 60.0 / 2.0);
                                static const unsigned int lon_max = 360*120;
                                static const unsigned int lat_max = 180*120;

                                GtopoCoord(uint16_t lo = 0, uint16_t la = 0) : lon(lo % lon_max), lat(la % lat_max) {}
                                GtopoCoord(const Point& pt);
                                uint16_t get_lon(void) const { return lon; }
                                uint16_t get_lat(void) const { return lat; }
                                void set_lon(uint16_t lo) { lon = lo; }
                                void set_lat(uint16_t la) { lat = la; }
				uint16_t lon_dist(const GtopoCoord &p) const;
 				uint16_t lat_dist(const GtopoCoord &p) const;
				int16_t lon_dist_signed(const GtopoCoord &p) const;
 				int16_t lat_dist_signed(const GtopoCoord &p) const;
				bool operator==(const GtopoCoord &p) const;
                                bool operator!=(const GtopoCoord &p) const { return !operator==(p); }
                                GtopoCoord operator+(const GtopoCoord &p) const { return GtopoCoord((lon + p.lon) % lon_max, (lat + p.lat) % lat_max); }
                                GtopoCoord operator-(const GtopoCoord &p) const { return GtopoCoord((lon_max + lon - p.lon) % lon_max, (lat_max + lat - p.lat) % lat_max); }
                                GtopoCoord operator-() const { return GtopoCoord() - GtopoCoord(lon, lat); }
                                GtopoCoord& operator+=(const GtopoCoord &p) { return *this = *this + p; }
                                GtopoCoord& operator-=(const GtopoCoord &p) { return *this = *this - p; }
                                unsigned int tilenr(void) const;
                                operator Point(void) const;
                                std::ostream& print(std::ostream& os) const;
				static Point get_pointsize(void) { return (Point)GtopoCoord(1, 1) - (Point)GtopoCoord(0, 0); }
 
                        private:
                                uint16_t lon;
                                uint16_t lat;
                };

                typedef int16_t elevation_t;
                typedef std::pair<elevation_t,elevation_t> minmax_elevation_t;
                static const elevation_t nodata;

                PalmGtopo(void);
                void open(const std::string& fname);
                void close(void);
                bool is_open(void) const { return !!fdb; }
                Rect get_rect(void) const { return Rect(Point(hdr.minlon, hdr.minlat), Point(hdr.maxlon, hdr.maxlat)); }
                elevation_t operator[](const GtopoCoord& pt);
                elevation_t operator[](const Point& pt) { return operator[](GtopoCoord(pt)); }
                minmax_elevation_t get_minmax_elev(const Rect& r);
                minmax_elevation_t get_minmax_elev(const PolygonSimple& p);
                minmax_elevation_t get_minmax_elev(const PolygonHole& p);
                minmax_elevation_t get_minmax_elev(const MultiPolygonHole& p);

        private:
                static const uint32_t db_type = ('g' << 24) | ('t' << 16) | ('o' << 8) | 'p';

                class Header {
                        public:
                                PalmUInt16 version;
                                PalmUInt16 dummy;
                                PalmInt32 minlat, maxlat;
                                PalmInt32 minlon, maxlon;
                } __attribute__ ((packed));

                class TileCache {
                        public:
                                TileCache(uint16_t tilenr = ~0, uint32_t gen = 0, void *data = 0, unsigned int dlen = 0);
                                bool is_valid(void) const { return m_tilenr != (uint16_t)~0; }
                                uint32_t get_gen(void) const { return m_gen; }
                                uint16_t get_tilenr(void) const { return m_tilenr; }
                                elevation_t operator[](unsigned int idx) const;
                                void set_gen(uint32_t gen) { m_gen = gen; }

                        private:
                                std::vector<PalmInt16> m_data;
                                uint32_t m_gen;
                                uint16_t m_tilenr;
                };

                static const unsigned int tile_cache_size = 4;
                TileCache tile_cache[tile_cache_size];

                struct pi_file *fdb;
                Header hdr;
                uint32_t m_gen;

                static unsigned int to_tilenr(const GtopoCoord& pt);
                const TileCache& find_tile(unsigned int tilenr);
};

inline std::ostream& operator<<(std::ostream& os, const PalmGtopo::GtopoCoord& c)
{
        return c.print(os);
}

//class PalmAirports : public PalmFindByIcao<PalmFindByName<PalmAirportBase> > {};
typedef PalmFindBboxBox<PalmFindNearest<PalmFindByText<PalmFindByIcao<PalmFindByName<PalmAirportBase> > > > > PalmAirports;
typedef PalmFindBboxBox<PalmFindNearest<PalmFindByText<PalmFindByIcao<PalmFindByName<PalmAirspaceBase> > > > > PalmAirspaces;
typedef PalmFindBboxPoint<PalmFindNearest<PalmFindByText<PalmFindByIcao<PalmFindByName<PalmNavaidBase> > > > > PalmNavaids;
typedef PalmFindBboxPoint<PalmFindNearest<PalmFindByName<PalmWaypointBase> > > PalmWaypoints;
typedef PalmFindBboxBox<PalmFindNearest<PalmFindByName<PalmMapelementBase> > > PalmMapelements;

#endif /* PALM_H */
