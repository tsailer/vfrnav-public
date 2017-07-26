//
// C++ Implementation: palm
//
// Description: Palm Database File Reader
//
//
// Author: Thomas Sailer <t.sailer@alumni.ethz.ch>, (C) 2007, 2009, 2012, 2016
//
// Copyright: See COPYING file that comes with this distribution
//
//

#include "sysdeps.h"

#include <limits>
#include <cstring>
#include <memory>
#include <iostream>
#include <sstream>
#include <stdexcept>
#include <set>

#include <pi-file.h>
#include <zlib.h>

#include "palm.h"
#include "fplan.h"

#undef PALMICONV_NOEXCEPTIONS

class PalmIConv {
public:
	PalmIConv(void);
	~PalmIConv();
	std::string convert(const std::string& str);
#if defined(PALMICONV_NOEXCEPTIONS)
	std::string convert(const std::string& str, std::auto_ptr<Glib::Error>& ex);
#endif

private:
	Glib::IConv *m_iconv;
};

PalmIConv::PalmIConv(void)
        : m_iconv(0)
{
        m_iconv = new Glib::IConv("UTF-8", "CP1252");
        if (m_iconv->gobj() != (GIConv)-1) {
		if (false)
	                std::cerr << "PalmIConv: using CP1252" << std::endl;
                return;
        }
        delete m_iconv;
        std::cerr << "PalmIConv: CP1252 unusable trying ISO8859-1" << std::endl;
        m_iconv = new Glib::IConv("UTF-8", "ISO8859-1");
        if (m_iconv->gobj() == (GIConv)-1) {
                throw std::runtime_error("PalmIConv: no suitable charset conversion found");
        }
	if (false)
	        std::cerr << "PalmIConv: using ISO8859-1" << std::endl;
}

PalmIConv::~PalmIConv()
{
        delete m_iconv;
}

std::string PalmIConv::convert(const std::string & str)
{
#if defined(GLIBMM_EXCEPTIONS_ENABLED)
        return m_iconv->convert(str);
#else
        std::auto_ptr<Glib::Error> ex;
        std::string r(m_iconv->convert(str, ex));
        if (ex.get())
                throw *ex;
        return r;
#endif
}

#if defined(PALMICONV_NOEXCEPTIONS)

std::string PalmIConv::convert(const std::string & str, std::auto_ptr<Glib::Error> & ex)
{
#if defined(GLIBMM_EXCEPTIONS_ENABLED)
        std::string r;
        try {
                r = m_iconv->convert(str);
        } catch (const Glib::Error& e) {
                ex = std::auto_ptr<Glib::Error>(new Glib::Error(e));
        }
        return r;
#else
        return m_iconv->convert(str, ex);
#endif
}

#endif

static PalmIConv palm_iconv;

bool PalmDbBase::PalmStringCompare::operator ==( const Glib::ustring & s ) const
{
        switch (comp) {
                case comp_contains:
                        return s.casefold().find(str) != Glib::ustring::npos;

                case comp_startswith:
                        return !s.casefold().compare(0, strln, str);

                default:
                        return s.casefold() == str;
        };
}

unsigned int PalmDbBase::to_tile( const Point & pt )
{
        return (((pt.get_lon() >> 25) & 0x7f) << 6) | ((pt.get_lat() >> 25) & 0x3f);
}

unsigned int PalmDbBase::next_tile( unsigned int tile, unsigned int tmin, unsigned int tmax )
{
        unsigned int tlat = tile & 0x3f;
        unsigned int tlatmin = tmin & 0x3f;
        unsigned int tlatmax = tmax & 0x3f;
        if (tlat == tlatmax) {
                tlat = tlatmin;
                tile += (1 << 6);
        } else {
                tlat++;
        }
        return (tlat & 0x3f) | (tile & (0x7f << 6));
}

template <class PalmDb> typename PalmFindByName<PalmDb>::entryvector_t PalmFindByName<PalmDb>::find_by_name( const Glib::ustring & nm, unsigned int limit, unsigned int skip, comp_t comp )
{
        PalmDbBase::PalmStringCompare c(nm, comp);
        entryvector_t r;
        id_t nr = PalmDb::get_nrentries();
        for (id_t i = 0; i < nr; i++) {
                if (!limit)
                        break;
                entry_t n(this->operator[](i));
                if (!n.is_valid())
                        continue;
                if (!(c == n.get_name()))
                        continue;
                if (skip) {
                        skip--;
                        continue;
                }
                r.push_back(n);
                limit--;
        }
        return r;
}

template <class PalmDb> typename PalmFindByIcao<PalmDb>::entryvector_t PalmFindByIcao<PalmDb>::find_by_icao( const Glib::ustring & nm, unsigned int limit, unsigned int skip, comp_t comp )
{
        PalmDbBase::PalmStringCompare c(nm, comp);
        entryvector_t r;
        id_t nr = PalmDb::get_nrentries();
        for (id_t i = 0; i < nr; i++) {
                if (!limit)
                        break;
                entry_t n(this->operator[](i));
                if (!n.is_valid())
                        continue;
                if (!(c == n.get_icao()))
                        continue;
                if (skip) {
                        skip--;
                        continue;
                }
                r.push_back(n);
                limit--;
        }
        return r;
}

template <class PalmDb> typename PalmFindByText<PalmDb>::entryvector_t PalmFindByText<PalmDb>::find_by_text( const Glib::ustring & nm, unsigned int limit, unsigned int skip, comp_t comp )
{
        PalmDbBase::PalmStringCompare c(nm, comp);
        entryvector_t r;
        id_t nr = PalmDb::get_nrentries();
        for (id_t i = 0; i < nr; i++) {
                if (!limit)
                        break;
                entry_t n(this->operator[](i));
                if (!n.is_valid())
                        continue;
                if (!(c == n.get_name()) && !(c == n.get_icao()))
                        continue;
                if (skip) {
                        skip--;
                        continue;
                }
                r.push_back(n);
                limit--;
        }
        return r;
}

template <class PalmDb> typename PalmFindNearest<PalmDb>::entryvector_t PalmFindNearest<PalmDb>::find_nearest( const Point & pt, unsigned int limit, const Rect & rect )
{
        less_dist comp(pt);
        uint64_t lastdist = ~0ULL;
        entryvector_t r;
        id_t nr = PalmDb::get_nrentries();
        bool smalllimit(nr / 2 >= limit);
        for (id_t i = 0; i < nr; i++) {
                entry_t n(this->operator[](i));
                if (!n.is_valid())
                        continue;
                if (!rect.is_inside(n.get_coord()))
                        continue;
                if (pt.simple_distance_rel(n.get_coord()) > lastdist)
                        continue;
                r.push_back(n);
                if (!smalllimit)
                        continue;
                if (r.size() < 2*limit)
                        continue;
                std::stable_sort(r.begin(), r.end(), comp);
                r.resize(limit);
                lastdist = pt.simple_distance_rel(r.back().get_coord());
        };
        std::stable_sort(r.begin(), r.end(), comp);
        if (r.size() > limit)
                r.resize(limit);
        return r;
}

template <class PalmDb> typename PalmFindBboxPoint<PalmDb>::entryvector_t PalmFindBboxPoint<PalmDb>::find_bbox( const Rect & rect, unsigned int limit )
{
        entryvector_t r;
        id_t nr = PalmDb::get_nrentries();
        if (false) {
                for (id_t i = 0; i < nr; i++) {
                        if (!limit)
                                break;
                        entry_t n(this->operator[](i));
                        if (!n.is_valid())
                                continue;
                        if (!rect.is_inside(n.get_coord()))
                                continue;
                        r.push_back(n);
                        limit--;
                }
        } else {
                typedef typename PalmDb::idvector_t idvector_t;
                typedef std::set<id_t> idset_t;
                idset_t idset;
                unsigned int tilestart = PalmDb::to_tile(rect.get_southwest());
                unsigned int tileend = PalmDb::to_tile(rect.get_northeast());
                for (unsigned int tile = tilestart;; tile = PalmDb::next_tile(tile, tilestart, tileend)) {
                        idvector_t ids = PalmDb::tile_first_ids(tile);
                        for (typename idvector_t::const_iterator idi(ids.begin()), ide(ids.end()); idi != ide; idi++) {
                                id_t id(*idi);
                                for (;;) {
                                        if (!limit)
                                                break;
                                        entry_t n(this->operator[](id));
                                        if (!n.is_valid())
                                                break;
                                        id_t id1(id);
                                        id = PalmDb::tile_next_id(id);
                                        if (!rect.is_inside(n.get_coord()))
                                                continue;
                                        std::pair<typename idset_t::iterator, bool> idins(idset.insert(id1));
                                        if (!idins.second)
                                                continue;
                                        r.push_back(n);
                                        limit--;
                                }
                        }
                        if (tile == tileend)
                                break;
                }
        }
        return r;
}

template <class PalmDb> typename PalmFindBboxBox<PalmDb>::entryvector_t PalmFindBboxBox<PalmDb>::find_bbox( const Rect & rect, unsigned int limit )
{
        entryvector_t r;
        id_t nr = PalmDb::get_nrentries();
        if (false) {
                for (id_t i = 0; i < nr; i++) {
                        if (!limit)
                                break;
                        entry_t n(this->operator[](i));
                        if (!n.is_valid())
                                continue;
                        if (!rect.is_intersect(n.get_bbox()))
                                continue;
                        r.push_back(n);
                        limit--;
                }
        } else {
                typedef typename PalmDb::idvector_t idvector_t;
                typedef std::set<id_t> idset_t;
                idset_t idset;
                unsigned int tilestart = PalmDb::to_tile(rect.get_southwest());
                unsigned int tileend = PalmDb::to_tile(rect.get_northeast());
                for (unsigned int tile = tilestart;; tile = PalmDb::next_tile(tile, tilestart, tileend)) {
                        idvector_t ids = PalmDb::tile_first_ids(tile);
                        for (typename idvector_t::const_iterator idi(ids.begin()), ide(ids.end()); idi != ide; idi++) {
                                id_t id(*idi);
                                for (;;) {
                                        if (!limit)
                                                break;
                                        entry_t n(this->operator[](id));
                                        if (!n.is_valid())
                                                break;
                                        id_t id1(id);
                                        id = PalmDb::tile_next_id(id);
                                        if (!rect.is_intersect(n.get_bbox()))
                                                continue;
                                        std::pair<typename idset_t::iterator, bool> idins(idset.insert(id1));
                                        if (!idins.second)
                                                continue;
                                        r.push_back(n);
                                        limit--;
                                }
                        }
                        if (tile == tileend)
                                break;
                }
        }
        return r;
}

PalmNavaidBase::PalmNavaidBase( void )
        : fdb(0)
{
}

PalmNavaidBase::~PalmNavaidBase( void )
{
        close();
}

void PalmNavaidBase::open( const std::string & fname )
{
        close();
        fdb = pi_file_open((char *)fname.c_str());
        if (!fdb) {
                std::ostringstream oss;
                oss << "PalmNavaidBase: cannot open file " << fname;
                throw std::runtime_error(oss.str());
        }
        struct DBInfo info;
        pi_file_get_info(fdb, &info);
        if (info.type != db_type) {
                close();
                std::ostringstream oss;
                oss << "PalmNavaidBase: invalid DB type " << std::hex << info.type;
                throw std::runtime_error(oss.str());
        }
        size_t hdrsize(0), nameidxsize(0), tileidxsize(0);
        void *buf;
        if (pi_file_read_record(fdb, 0, &buf, &hdrsize, 0, 0, 0) ||
            hdrsize < sizeof(hdr)) {
                close();
                throw std::runtime_error("PalmNavaidBase: cannot read header");
        }
        memcpy(&hdr, buf, sizeof(hdr));
        if (hdr.version != 1) {
                close();
                throw std::runtime_error("PalmNavaidBase: invalid header version");
        }
        if (pi_file_read_record(fdb, 1, &buf, &nameidxsize, 0, 0, 0) ||
            nameidxsize < hdr.number * sizeof(PalmUInt16)) {
                close();
                throw std::runtime_error("PalmNavaidBase: cannot read name_index");
        }
        name_index.resize(hdr.number);
        memcpy(&name_index[0], buf, hdr.number * sizeof(PalmUInt16));
        if (pi_file_read_record(fdb, 2, &buf, &tileidxsize, 0, 0, 0) ||
            tileidxsize < sizeof(tile_index)) {
                close();
                throw std::runtime_error("PalmNavaidBase: cannot tile_index");
        }
        memcpy(&tile_index, buf, sizeof(tile_index));
}

void PalmNavaidBase::close( void )
{
        if (fdb)
                pi_file_close(fdb);
        fdb = 0;
        name_index.resize(0);
}

PalmNavaidBase::id_t PalmNavaidBase::get_nrentries( void ) const
{
        return hdr.number;
}

time_t PalmNavaidBase::get_effdate( void ) const
{
        return hdr.effdate * 86400 - FPlanWaypoint::unix_minus_palmos_epoch;
}

time_t PalmNavaidBase::get_expdate( void ) const
{
        return hdr.expdate * 86400 - FPlanWaypoint::unix_minus_palmos_epoch;
}

NavaidsDb::Navaid::label_placement_t PalmNavaidBase::decode_label_placement( uint8_t x )
{
        switch (x) {
                case 0:
                        return NavaidsDb::Navaid::label_e;

                case 1:
                        return NavaidsDb::Navaid::label_n;

                case 2:
                        return NavaidsDb::Navaid::label_w;

                case 3:
                        return NavaidsDb::Navaid::label_s;

                default:
                        return NavaidsDb::Navaid::label_off;
        }
}

PalmNavaidBase::entry_t PalmNavaidBase::operator[](id_t id)
{
        if (!fdb || id >= hdr.number)
                return entry_t();
        Rec *rec;
        size_t recsize;
        if (pi_file_read_record(fdb, id + 3, (void **)&rec, &recsize, 0, 0, 0))
                return entry_t();
        if (recsize < sizeof(Rec))
                return entry_t();
        entry_t::navaid_type_t nt(entry_t::navaid_invalid);
        entry_t::label_placement_t lp(decode_label_placement(rec->label_placement & 3));
        switch (rec->typecode) {
                case '1':
                case '2':
                        nt = entry_t::navaid_vor;
                        break;

                case '4':
                        nt = entry_t::navaid_vordme;
                        break;

                case '9':
                        nt = entry_t::navaid_dme;
                        break;

                case '5':
                case '7':
                        nt = entry_t::navaid_ndb;
                        break;
        }
        entry_t n;
        try {
                n.set_sourceid("");
                n.set_coord(Point(rec->lon, rec->lat));
                n.set_elev(rec->elev);
                n.set_range(rec->range);
                n.set_frequency(rec->freq * 1000);
                n.set_icao(palm_iconv.convert(rec->icao));
                n.set_name(palm_iconv.convert(((char *)rec) + rec->nameptr));
                n.set_navaid_type(nt);
                n.set_label_placement(lp);
                n.set_inservice(true);
                n.set_modtime(get_effdate());
        } catch(const Glib::Error& e) {
                std::cerr << "get_navaid: Glib error: " << e.what() << std::endl;
                n = entry_t();
        }
        return n;
}

PalmNavaidBase::idvector_t PalmNavaidBase::tile_first_ids( unsigned int tilenr ) const
{
        return idvector_t(1, tile_index[tilenr & 0x1fff] - 3);
}

PalmNavaidBase::id_t PalmNavaidBase::tile_next_id( id_t id )
{
        if (!fdb || id >= hdr.number)
                return ~0;
        Rec *rec;
	size_t recsize;
        if (pi_file_read_record(fdb, id + 3, (void **)&rec, &recsize, 0, 0, 0))
                return ~0;
        if (recsize < sizeof(Rec))
                return ~0;
        return rec->tileidx - 3;
}

PalmWaypointBase::PalmWaypointBase( void )
        : fdb(0)
{
}

PalmWaypointBase::~PalmWaypointBase( void )
{
        close();
}

void PalmWaypointBase::open( const std::string & fname )
{
        close();
        fdb = pi_file_open((char *)fname.c_str());
        if (!fdb) {
                std::ostringstream oss;
                oss << "PalmNavaidBase: cannot open file " << fname;
                throw std::runtime_error(oss.str());
        }
        struct DBInfo info;
        pi_file_get_info(fdb, &info);
        if (info.type != db_type) {
                close();
                std::ostringstream oss;
                oss << "PalmNavaidBase: invalid DB type " << std::hex << info.type;
                throw std::runtime_error(oss.str());
        }
        size_t hdrsize(0), nameidxsize(0), tileidxsize(0);
        void *buf;
        if (pi_file_read_record(fdb, 0, &buf, &hdrsize, 0, 0, 0) ||
            hdrsize < sizeof(hdr)) {
                close();
                throw std::runtime_error("PalmNavaidBase: cannot read header");
        }
        memcpy(&hdr, buf, sizeof(hdr));
        if (hdr.version != 1) {
                close();
                throw std::runtime_error("PalmNavaidBase: invalid header version");
        }
        if (pi_file_read_record(fdb, 1, &buf, &tileidxsize, 0, 0, 0) ||
            tileidxsize < sizeof(tile_index)) {
                close();
                throw std::runtime_error("PalmNavaidBase: cannot tile_index");
        }
        memcpy(&tile_index, buf, sizeof(tile_index));
}

void PalmWaypointBase::close( void )
{
        if (fdb)
                pi_file_close(fdb);
        fdb = 0;
}

PalmWaypointBase::id_t PalmWaypointBase::get_nrentries( void ) const
{
        return hdr.number;
}

time_t PalmWaypointBase::get_effdate( void ) const
{
        return hdr.effdate * 86400 - FPlanWaypoint::unix_minus_palmos_epoch;
}

time_t PalmWaypointBase::get_expdate( void ) const
{
        return hdr.expdate * 86400 - FPlanWaypoint::unix_minus_palmos_epoch;
}

PalmWaypointBase::entry_t PalmWaypointBase::operator[]( id_t id )
{
        if (!fdb || id >= hdr.number)
                return entry_t();
        unsigned int recnr = id & 31;
        id >>= 5;
        Rec *rec;
        size_t recsize;
        if (pi_file_read_record(fdb, id + 2, (void **)&rec, &recsize, 0, 0, 0))
                return entry_t();
        if (recsize < sizeof(Rec) * (recnr + 1))
                return entry_t();
        Rec& rec1(rec[recnr]);
        entry_t w;
        try {
                w.set_sourceid("");
                w.set_coord(Point(rec1.lon, rec1.lat));
                w.set_icao("");
                w.set_name(palm_iconv.convert(((char *)&rec1) + rec1.nameptr));
                w.set_usage(rec1.usagecode);
                w.set_label_placement(PalmNavaidBase::decode_label_placement(rec1.label_placement_backptr & 3));
                w.set_modtime(get_effdate());
        } catch(const Glib::Error& e) {
                std::cerr << "get_waypoint: Glib error: " << e.what() << std::endl;
                w = entry_t();
        }
        return w;
}

PalmWaypointBase::idvector_t PalmWaypointBase::tile_first_ids( unsigned int tilenr ) const
{
        return idvector_t(1, tile_index[tilenr & 0x1fff] - 32);
}

PalmWaypointBase::id_t PalmWaypointBase::tile_next_id( id_t id )
{
        if (!fdb || id >= hdr.number)
                return ~0;
        unsigned int recnr = id & 31;
        id >>= 5;
        Rec *rec;
        size_t recsize;
        if (pi_file_read_record(fdb, id + 2, (void **)&rec, &recsize, 0, 0, 0))
                return ~0;
        if (recsize < sizeof(Rec) * (recnr + 1))
                return ~0;
        Rec& rec1(rec[recnr]);
        return rec1.tileidx - 32;
}

PalmAirportBase::PalmAirportBase( void )
        : fdb(0)
{
}

PalmAirportBase::~PalmAirportBase( void )
{
        close();
}

void PalmAirportBase::open( const std::string & fname )
{
        close();
        fdb = pi_file_open((char *)fname.c_str());
        if (!fdb) {
                std::ostringstream oss;
                oss << "PalmAirportBase: cannot open file " << fname;
                throw std::runtime_error(oss.str());
        }
        struct DBInfo info;
        pi_file_get_info(fdb, &info);
        if (info.type != db_type) {
                close();
                std::ostringstream oss;
                oss << "PalmAirportBase: invalid DB type " << std::hex << info.type;
                throw std::runtime_error(oss.str());
        }
        size_t hdrsize(0), nameidxsize(0), tileidxsize(0);
        void *buf;
        if (pi_file_read_record(fdb, 0, &buf, &hdrsize, 0, 0, 0) ||
            hdrsize < sizeof(hdr)) {
                close();
                throw std::runtime_error("PalmAirportBase: cannot read header");
        }
        memcpy(&hdr, buf, sizeof(hdr));
        if (hdr.version != 1) {
                close();
                throw std::runtime_error("PalmAirportBase: invalid header version");
        }
        if (pi_file_read_record(fdb, 1, &buf, &nameidxsize, 0, 0, 0) ||
            nameidxsize < hdr.number * sizeof(PalmUInt16)) {
                close();
                throw std::runtime_error("PalmAirportBase: cannot read name_index");
        }
        name_index.resize(hdr.number);
        memcpy(&name_index[0], buf, hdr.number * sizeof(PalmUInt16));
        if (pi_file_read_record(fdb, 2, &buf, &tileidxsize, 0, 0, 0) ||
            tileidxsize < sizeof(tile_index)) {
                close();
                throw std::runtime_error("PalmAirportBase: cannot tile_index");
        }
        memcpy(&tile_index, buf, sizeof(tile_index));
}

void PalmAirportBase::close( void )
{
        if (fdb)
                pi_file_close(fdb);
        fdb = 0;
        name_index.resize(0);
}

PalmAirportBase::id_t PalmAirportBase::get_nrentries( void ) const
{
        return hdr.number;
}

time_t PalmAirportBase::get_effdate( void ) const
{
        return hdr.effdate * 86400 - FPlanWaypoint::unix_minus_palmos_epoch;
}

time_t PalmAirportBase::get_expdate( void ) const
{
        return hdr.expdate * 86400 - FPlanWaypoint::unix_minus_palmos_epoch;
}

PalmAirportBase::entry_t PalmAirportBase::operator[](id_t id)
{
        if (!fdb || id >= hdr.number)
                return entry_t();
        Rec *rec;
        size_t recsize;
        if (pi_file_read_record(fdb, id + 3, (void **)&rec, &recsize, 0, 0, 0))
                return entry_t();
        if (recsize < sizeof(Rec))
                return entry_t();
        entry_t a;
        try {
                a.set_sourceid("");
                a.set_coord(Point(rec->lon, rec->lat));
                a.set_icao(palm_iconv.convert(rec->icao));
                a.set_name(palm_iconv.convert(((char *)rec) + rec->nameptr));
                a.set_vfrrmk(palm_iconv.convert(((char *)rec) + rec->vfrrmkptr));
                a.set_elevation(rec->elev);
                a.set_typecode(rec->typecode);
                a.set_label_placement(PalmNavaidBase::decode_label_placement(rec->label_placement & 3));
                a.set_modtime(get_effdate());
                const Runway *rwyp((const Runway *)(((char *)rec) + rec->rwyptr));
                const Comm *commp((const Comm *)(((char *)rec) + rec->commptr));
                const VFRRoute *vfrrtep((const VFRRoute *)(((char *)rec) + rec->vfrrteptr));
                for (unsigned int i = 0; i < rec->rwynr; i++) {
                        const Runway& rwy(rwyp[i]);
                        entry_t::Runway rwy1(rwy.ident_he, rwy.ident_le, rwy.length, rwy.width,
                                             rwy.surface, Point::invalid, Point::invalid,
                                             rwy.he_tda, rwy.he_lda, 0, 0, rec->elev, rwy.le_tda, rwy.le_lda, 0, 0,
                                             rec->elev, entry_t::Runway::flag_le_usable | entry_t::Runway::flag_he_usable);
                        rwy1.compute_default_hdg();
                        rwy1.compute_default_coord(Point(rec->lon, rec->lat));
                        a.add_rwy(rwy1);
                }
                for (unsigned int i = 0; i < rec->commnr; i++) {
                        const Comm& comm(commp[i]);
                        a.add_comm(entry_t::Comm(palm_iconv.convert(((char *)rec) + comm.nameptr), palm_iconv.convert(((char *)rec) + comm.sectorptr), palm_iconv.convert(((char *)rec) + comm.oprhoursptr),
                                   comm.freq[0] * 1000, comm.freq[1] * 1000, comm.freq[2] * 1000, comm.freq[3] * 1000, comm.freq[4] * 1000));
                }
                for (unsigned int i = 0; i < rec->vfrrtenr; i++) {
                        const VFRRoute& vfrrte(vfrrtep[i]);
                        entry_t::VFRRoute r(palm_iconv.convert(((char *)rec) + vfrrte.rtenameptr));
                        const VFRRoutePoint *rteptp((const VFRRoutePoint *)(((char *)rec) + vfrrte.rteptptr));
                        for (unsigned int j = 0; j < vfrrte.rteptnr; j++) {
                                const VFRRoutePoint& rtept(rteptp[j]);
                                entry_t::VFRRoute::VFRRoutePoint::altcode_t ac(entry_t::VFRRoute::VFRRoutePoint::altcode_invalid);
                                switch (rtept.altcode) {
                                        case 'A':
                                                ac = entry_t::VFRRoute::VFRRoutePoint::altcode_atorabove;
                                                break;

                                        case 'B':
                                                ac = entry_t::VFRRoute::VFRRoutePoint::altcode_atorbelow;
                                                break;

                                        case 'T':
                                                ac = entry_t::VFRRoute::VFRRoutePoint::altcode_altspecified;
                                                break;

                                        case 'S':
                                                ac = entry_t::VFRRoute::VFRRoutePoint::altcode_assigned;
                                                break;

                                        case 'R':
                                                ac = entry_t::VFRRoute::VFRRoutePoint::altcode_recommendedalt;
                                                break;
                                }
                                entry_t::VFRRoute::VFRRoutePoint::pathcode_t pc(entry_t::VFRRoute::VFRRoutePoint::pathcode_invalid);
                                switch (rtept.pathcode) {
                                        case 'A':
                                                pc = entry_t::VFRRoute::VFRRoutePoint::pathcode_arrival;
                                                break;

                                        case 'D':
                                                pc = entry_t::VFRRoute::VFRRoutePoint::pathcode_departure;
                                                break;

                                        case 'H':
                                                pc = entry_t::VFRRoute::VFRRoutePoint::pathcode_holding;
                                                break;

                                        case 'T':
                                                pc = entry_t::VFRRoute::VFRRoutePoint::pathcode_trafficpattern;
                                                break;

                                        case 'V':
                                                pc = entry_t::VFRRoute::VFRRoutePoint::pathcode_vfrtransition;
                                                break;

                                        case 'O':
                                                pc = entry_t::VFRRoute::VFRRoutePoint::pathcode_other;
                                                break;
                                }
                                r.add_point(entry_t::VFRRoute::VFRRoutePoint(palm_iconv.convert(((char *)rec) + rtept.ptnameptr), Point(rtept.lon, rtept.lat), rtept.alt,
                                            PalmNavaidBase::decode_label_placement(rtept.label_placement & 3), rtept.ptsym & 0x7f, pc, ac, !!(rtept.ptsym & 0x80)));
                        }
                        a.add_vfrrte(r);
                }
        } catch(const Glib::Error& e) {
                std::cerr << "get_airport: Glib error: " << e.what() << std::endl;
                a = entry_t();
        }
        return a;
}

PalmAirportBase::idvector_t PalmAirportBase::tile_first_ids( unsigned int tilenr ) const
{
        return idvector_t(1, tile_index[tilenr & 0x1fff] - 3);
}

PalmAirportBase::id_t PalmAirportBase::tile_next_id( id_t id )
{
        if (!fdb || id >= hdr.number)
                return ~0;
        Rec *rec;
        size_t recsize;
        if (pi_file_read_record(fdb, id + 3, (void **)&rec, &recsize, 0, 0, 0))
                return ~0;
        if (recsize < sizeof(Rec))
                return ~0;
        return rec->tileidx - 3;
}

PalmAirspaceBase::PalmAirspaceBase( void )
        : fdb(0)
{
}

PalmAirspaceBase::~PalmAirspaceBase( void )
{
        close();
}

void PalmAirspaceBase::open( const std::string & fname )
{
        close();
        fdb = pi_file_open((char *)fname.c_str());
        if (!fdb) {
                std::ostringstream oss;
                oss << "PalmAirspaceBase: cannot open file " << fname;
                throw std::runtime_error(oss.str());
        }
        struct DBInfo info;
        pi_file_get_info(fdb, &info);
        if (info.type != db_type) {
                close();
                std::ostringstream oss;
                oss << "PalmAirspaceBase: invalid DB type " << std::hex << info.type;
                throw std::runtime_error(oss.str());
        }
        size_t hdrsize(0), nameidxsize(0), tileidxsize(0);
        void *buf;
        if (pi_file_read_record(fdb, 0, &buf, &hdrsize, 0, 0, 0) ||
            hdrsize < sizeof(hdr)) {
                close();
                throw std::runtime_error("PalmAirspaceBase: cannot read header");
        }
        memcpy(&hdr, buf, sizeof(hdr));
        if (hdr.version != 1) {
                close();
                throw std::runtime_error("PalmAirspaceBase: invalid header version");
        }
        if (pi_file_read_record(fdb, 1, &buf, &tileidxsize, 0, 0, 0) ||
            tileidxsize < sizeof(tile_index)) {
                close();
                throw std::runtime_error("PalmAirspaceBase: cannot tile_index");
        }
        memcpy(&tile_index, buf, sizeof(tile_index));
}

void PalmAirspaceBase::close( void )
{
        if (fdb)
                pi_file_close(fdb);
        fdb = 0;
}

PalmAirspaceBase::id_t PalmAirspaceBase::get_nrentries( void ) const
{
        return hdr.number;
}

time_t PalmAirspaceBase::get_effdate( void ) const
{
        return hdr.effdate * 86400 - FPlanWaypoint::unix_minus_palmos_epoch;
}

time_t PalmAirspaceBase::get_expdate( void ) const
{
        return hdr.expdate * 86400 - FPlanWaypoint::unix_minus_palmos_epoch;
}

uint32_t PalmAirspaceBase::get_altlimit( void ) const
{
        return hdr.altlimit;
}

PalmAirspaceBase::entry_t PalmAirspaceBase::operator[](id_t id)
{
        if (!fdb || id >= hdr.number)
                return entry_t();
        Rec *rec;
        size_t recsize;
        if (pi_file_read_record(fdb, id + 2, (void **)&rec, &recsize, 0, 0, 0))
                return entry_t();
        if (recsize < sizeof(Rec))
                return entry_t();
        int32_t alwr = rec->altlwr & 0xffffff;
        alwr |= -(alwr & 0x800000);
        int32_t aupr = rec->altupr & 0xffffff;
        aupr |= -(aupr & 0x800000);
        PolygonSimple poly;
        for (unsigned int i = 0; i < rec->polynr; i++)
                poly.push_back(Point(rec->poly[i].lon, rec->poly[i].lat));
        entry_t a;
        try {
                a.set_sourceid("");
                a.set_labelcoord(Point(rec->labellon, rec->labellat));
                a.set_icao(palm_iconv.convert(rec->icao));
                a.set_name(palm_iconv.convert(((char *)rec) + rec->nameptr));
                a.set_ident(palm_iconv.convert(((char *)rec) + rec->identptr));
                a.set_classexcept(palm_iconv.convert(((char *)rec) + rec->classexceptptr));
                a.set_efftime(palm_iconv.convert(((char *)rec) + rec->efftimeptr));
                a.set_note(palm_iconv.convert(((char *)rec) + rec->noteptr));
                a.set_commname(palm_iconv.convert(((char *)rec) + rec->commnameptr));
                a.set_commfreq(0, rec->commfreq[0] * 1000);
                a.set_commfreq(1, rec->commfreq[1] * 1000);
                a.set_bdryclass(rec->bdryclass);
                a.set_typecode(rec->typecode);
                a.set_gndelevmin(rec->gndelevmin);
                a.set_gndelevmax(rec->gndelevmax);
                a.set_altlwr(alwr);
                a.set_altupr(aupr);
                a.set_altlwrflags(rec->altlwr >> 24);
                a.set_altuprflags(rec->altupr >> 24);
                a.set_label_placement(entry_t::label_center);
                a.set_modtime(get_effdate());
		{
			MultiPolygonHole mp;
			mp.push_back(poly);
			a.set_polygon(mp);
		}
        } catch (const Glib::Error& e) {
                std::cerr << "get_airspace: Glib error: " << e.what() << std::endl;
                a = entry_t();
        }
        return a;
}

PalmAirspaceBase::idvector_t PalmAirspaceBase::tile_first_ids( unsigned int tilenr ) const
{
        return idvector_t(1, tile_index[tilenr & 0x1fff] - 2);
}

PalmAirspaceBase::id_t PalmAirspaceBase::tile_next_id( id_t id )
{
        if (!fdb || id >= hdr.number)
                return ~0;
        Rec *rec;
        size_t recsize;
        if (pi_file_read_record(fdb, id + 2, (void **)&rec, &recsize, 0, 0, 0))
                return ~0;
        if (recsize < sizeof(Rec))
                return ~0;
        return rec->tileidx - 2;
}

PalmMapelementBase::PalmMapelementBase( void )
        : fdb(0)
{
}

PalmMapelementBase::~PalmMapelementBase( void )
{
        close();
}

void PalmMapelementBase::open( const std::string & fname )
{
        close();
        fdb = pi_file_open((char *)fname.c_str());
        if (!fdb) {
                std::ostringstream oss;
                oss << "PalmMapelementBase: cannot open file " << fname;
                throw std::runtime_error(oss.str());
        }
        struct DBInfo info;
        pi_file_get_info(fdb, &info);
        if (info.type != db_type) {
                close();
                std::ostringstream oss;
                oss << "PalmMapelementBase: invalid DB type " << std::hex << info.type;
                throw std::runtime_error(oss.str());
        }
        size_t hdrsize(0), nameidxsize(0), tileidxsize(0);
        void *buf;
        if (pi_file_read_record(fdb, 0, &buf, &hdrsize, 0, 0, 0) ||
            hdrsize < sizeof(hdr)) {
                close();
                std::ostringstream oss;
                oss << " filename " << fname << " hdrsize " << hdrsize << " sizeof hdr " << sizeof(hdr);
                throw std::runtime_error("PalmMapelementBase: cannot read header" + oss.str());
        }
        memcpy(&hdr, buf, sizeof(hdr));
        if (hdr.version != 1) {
                close();
                throw std::runtime_error("PalmMapelementBase: invalid header version");
        }
        if (pi_file_read_record(fdb, 1, &buf, &tileidxsize, 0, 0, 0) ||
            tileidxsize < sizeof(tile_index)) {
                close();
                throw std::runtime_error("PalmMapelementBase: cannot tile_index");
        }
        memcpy(&tile_index, buf, sizeof(name_tile_index));
        if (pi_file_read_record(fdb, 2, &buf, &tileidxsize, 0, 0, 0) ||
            tileidxsize < sizeof(name_tile_index)) {
                close();
                throw std::runtime_error("PalmMapelementBase: cannot name_tile_index");
        }
        memcpy(&name_tile_index, buf, sizeof(name_tile_index));
}

void PalmMapelementBase::close( void )
{
        if (fdb)
                pi_file_close(fdb);
        fdb = 0;
}

PalmMapelementBase::id_t PalmMapelementBase::get_nrentries( void ) const
{
        return hdr.number;
}

time_t PalmMapelementBase::get_mindate( void ) const
{
        return hdr.mindate * 86400 - FPlanWaypoint::unix_minus_palmos_epoch;
}

time_t PalmMapelementBase::get_maxdate( void ) const
{
        return hdr.maxdate * 86400 - FPlanWaypoint::unix_minus_palmos_epoch;
}

PalmMapelementBase::entry_t PalmMapelementBase::operator[](id_t id)
{
        if (!fdb || id >= hdr.number)
                return entry_t();
        unsigned int ridx(id & 7);
        id >>= 3;
        Rec *rec;
        size_t recsize;
        if (pi_file_read_record(fdb, id + 3, (void **)&rec, &recsize, 0, 0, 0))
                return entry_t();
        if (recsize < sizeof(Rec) * (ridx + 1))
                return entry_t();
        const Rec& r(rec[ridx]);
        uint8_t typecode(r.typecode);
        Glib::ustring name;
        try {
                name = palm_iconv.convert(((char *)&r) + r.nameptr);
        } catch (const Glib::Error& e) {
                std::cerr << "get_mapelement: Glib error: " << e.what() << "(name: " << (((char *)&r) + r.nameptr) << ")" << std::endl;
                name = Glib::ustring();
        }
        const PolyPoint *pp((const PolyPoint *)(((char *)&r) + r.polyptr));
        unsigned int nrpt(r.polynr);
        if (nrpt >= 4096) {
                if (pi_file_read_record(fdb, r.polyptr, (void **)&pp, &recsize, 0, 0, 0))
                        return entry_t();
                if (recsize < sizeof(PolyPoint) * (nrpt + 1))
                        return entry_t();
                pp++;
        }
        PolygonSimple poly;
        Point labelpt(Point(pp[-1].lon, pp[-1].lat));
        for (unsigned int i = 0; i < nrpt; i++)
                poly.push_back(Point(pp[i].lon, pp[i].lat));
        return entry_t(typecode, poly, labelpt, name);
}

PalmMapelementBase::idvector_t PalmMapelementBase::tile_first_ids( unsigned int tilenr ) const
{
        tilenr &= 0x1fff;
        idvector_t r;
        r.push_back(name_tile_index[tilenr] - index_offset);
        r.push_back(tile_index[tilenr] - index_offset);
        return r;
}

PalmMapelementBase::id_t PalmMapelementBase::tile_next_id( id_t id )
{
        if (!fdb || id >= hdr.number)
                return ~0;
        unsigned int ridx(id & 7);
        id >>= 3;
        Rec *rec;
        size_t recsize;
        if (pi_file_read_record(fdb, id + 3, (void **)&rec, &recsize, 0, 0, 0))
                return ~0;
        if (recsize < sizeof(Rec) * (ridx + 1))
                return ~0;
        const Rec& r(rec[ridx]);
        return r.tileidx - index_offset;
}

PalmGtopo::PalmGtopo( void )
        : fdb(0), m_gen(0)
{
        for (int i = 0; i < tile_cache_size; i++)
                tile_cache[i] = TileCache();
}

void PalmGtopo::open( const std::string & fname )
{
        close();
        fdb = pi_file_open((char *)fname.c_str());
        if (!fdb) {
                std::ostringstream oss;
                oss << "PalmGtopo: cannot open file " << fname;
                throw std::runtime_error(oss.str());
        }
        struct DBInfo info;
        pi_file_get_info(fdb, &info);
        if (info.type != db_type) {
                close();
                std::ostringstream oss;
                oss << "PalmGtopo: invalid DB type " << std::hex << info.type;
                throw std::runtime_error(oss.str());
        }
	size_t hdrsize(0);
        void *buf;
        if (pi_file_read_record(fdb, 0, &buf, &hdrsize, 0, 0, 0) ||
            hdrsize < sizeof(hdr)) {
                close();
                std::ostringstream oss;
                oss << " filename " << fname << " hdrsize " << hdrsize << " sizeof hdr " << sizeof(hdr);
                throw std::runtime_error("PalmGtopo: cannot read header" + oss.str());
        }
        memcpy(&hdr, buf, sizeof(hdr));
        if (hdr.version != 1) {
                close();
                throw std::runtime_error("PalmGtopo: invalid header version");
        }
}

void PalmGtopo::close( void )
{
        if (fdb)
                pi_file_close(fdb);
        fdb = 0;
        for (int i = 0; i < tile_cache_size; i++)
                tile_cache[i] = TileCache();
}

PalmGtopo::TileCache::TileCache( uint16_t tilenr, uint32_t gen, void *data, unsigned int dlen )
        : m_gen(gen), m_tilenr(~0)
{
        if (!data || dlen <= 0)
                return;
        z_stream d_stream;
        memset(&d_stream, 0, sizeof(d_stream));
        d_stream.zalloc = (alloc_func)0;
        d_stream.zfree = (free_func)0;
        d_stream.opaque = (voidpf)0;
        d_stream.next_in  = (Bytef *)data;
        d_stream.avail_in = dlen;
        int err = inflateInit(&d_stream);
        if (err != Z_OK) {
                inflateEnd(&d_stream);
                std::cerr << "zlib: inflateInit error " << err << std::endl;
                return;
        }
        m_data.resize(128 * 128);
        d_stream.next_out = (Bytef *)&m_data[0];
        d_stream.avail_out = m_data.size() * sizeof(PalmInt16);
        err = inflate(&d_stream, Z_FINISH);
        inflateEnd(&d_stream);
        if (err != Z_STREAM_END) {
                m_data.resize(0);
                std::cerr << "zlib: inflate error " << err << std::endl;
                return;
        }
        m_tilenr = tilenr;
}

int16_t PalmGtopo::TileCache::operator [ ]( unsigned int idx ) const
{
        if (idx >= m_data.size())
                return 0;
        return m_data[idx];
}

PalmGtopo::GtopoCoord::GtopoCoord( const Point & pt )
        : lon(((uint16_t)(pt.get_lon() * garmin_to_gtopo30 + 180*120)) % lon_max), lat(((uint16_t)(pt.get_lat() * garmin_to_gtopo30 + 90*120)) % lat_max)
{
}

uint16_t PalmGtopo::GtopoCoord::lon_dist(const GtopoCoord &p) const
{
	return (p.get_lon() + lon_max - get_lon()) % lon_max;
}

uint16_t PalmGtopo::GtopoCoord::lat_dist(const GtopoCoord &p) const
{
	return (p.get_lat() + lat_max - get_lat()) % lat_max;
}

int16_t PalmGtopo::GtopoCoord::lon_dist_signed(const GtopoCoord &p) const
{
	int16_t r(lon_dist(p));
	if (r >= lon_max/2)
		r -= lon_max;
	return r;
}

int16_t PalmGtopo::GtopoCoord::lat_dist_signed(const GtopoCoord &p) const
{
	int16_t r(lat_dist(p));
	if (r >= lat_max/2)
		r -= lat_max;
	return r;
}

bool PalmGtopo::GtopoCoord::operator==(const GtopoCoord & p) const
{
        return get_lon() == p.get_lon() && get_lat() == p.get_lat();
}

PalmGtopo::GtopoCoord::operator Point(void) const
{
        return Point((int32_t)(((int)get_lon() - 180*120) * gtopo30_to_garmin), (int32_t)(((int)get_lat() - 90*120) * gtopo30_to_garmin));
}

unsigned int PalmGtopo::GtopoCoord::tilenr( void ) const
{
        uint16_t lat1 = get_lat() >> 7;
        uint16_t lon1 = get_lon() >> 7;
        return lat1 * 338 + lon1 + 1;
}

std::ostream & PalmGtopo::GtopoCoord::print( std::ostream & os ) const
{
        os << '(' << get_lon() << ',' << get_lat() << ')';
	return os;
}

const PalmGtopo::TileCache & PalmGtopo::find_tile( unsigned int tilenr )
{
        m_gen++;
        int inv_idx(-1), old_idx(0);
        uint32_t age(0);
        for (int i = 0; i < tile_cache_size; i++) {
                if (!tile_cache[i].is_valid()) {
                        inv_idx = i;
                        continue;
                }
                if (tile_cache[i].get_tilenr() == tilenr) {
                        tile_cache[i].set_gen(m_gen);
                        return tile_cache[i];
                }
                uint32_t a(m_gen - tile_cache[i].get_gen());
                if (a < age)
                        continue;
                age = a;
                old_idx = i;
        }
        int idx = (inv_idx == -1) ? old_idx : inv_idx;
        void *data = 0;
	size_t dlen = 0;
        if (!fdb || !tilenr || pi_file_read_record(fdb, tilenr, &data, &dlen, 0, 0, 0)) {
                tile_cache[idx] = TileCache();
                return tile_cache[idx];
        };
        tile_cache[idx] = TileCache(tilenr, m_gen, data, dlen);
        return tile_cache[idx];
}

PalmGtopo::elevation_t PalmGtopo::operator [ ]( const GtopoCoord & pt )
{
        const TileCache& tc(find_tile(pt.tilenr()));
        if (!tc.is_valid())
                return std::numeric_limits<elevation_t>::min();
        return tc[((pt.get_lat() & 0x7f) << 7) | (pt.get_lon() & 0x7f)];
}

const PalmGtopo::elevation_t PalmGtopo::nodata = std::numeric_limits<elevation_t>::min();;

PalmGtopo::minmax_elevation_t PalmGtopo::get_minmax_elev(const Rect & r)
{
        PalmGtopo::GtopoCoord tmin(r.get_southwest()), tmax(r.get_northeast());
        Point pdim(((Point)(tmin + PalmGtopo::GtopoCoord(1, 1))) - ((Point)(tmin)));
        minmax_elevation_t ret(std::numeric_limits<elevation_t>::max(), std::numeric_limits<elevation_t>::min());
        for (PalmGtopo::GtopoCoord tc(tmin);;) {
                elevation_t elev(this->operator[](tc));
                if (elev < ret.first)
                        ret.first = elev;
                if (elev > ret.second)
                        ret.second = elev;
                if (tc.get_lon() != tmax.get_lon()) {
                        tc.set_lon(tc.get_lon() + 1);
                        continue;
                }
                if (tc.get_lat() == tmax.get_lat())
                        break;
                tc.set_lon(tmin.get_lon());
                tc.set_lat(tc.get_lat() + 1);
        }
        if (ret.first == std::numeric_limits<elevation_t>::max())
                ret.first = nodata;
        if (ret.second == std::numeric_limits<elevation_t>::min())
                ret.second = nodata;
        return ret;
}

PalmGtopo::minmax_elevation_t PalmGtopo::get_minmax_elev(const PolygonSimple& p)
{
        Rect r(p.get_bbox());
        PalmGtopo::GtopoCoord tmin(r.get_southwest()), tmax(r.get_northeast());
        Point pdim(((Point)(tmin + PalmGtopo::GtopoCoord(1, 1))) - ((Point)(tmin)));
        minmax_elevation_t ret(std::numeric_limits<elevation_t>::max(), std::numeric_limits<elevation_t>::min());
        for (PalmGtopo::GtopoCoord tc(tmin);;) {
                Point ptile(tc);
                Rect rtile(ptile, ptile + pdim);
                if (!p.is_intersection(rtile) && !p.windingnumber(ptile))
                        continue;
                elevation_t elev(this->operator[](tc));
                if (elev < ret.first)
                        ret.first = elev;
                if (elev > ret.second)
                        ret.second = elev;
                if (tc.get_lon() != tmax.get_lon()) {
                        tc.set_lon(tc.get_lon() + 1);
                        continue;
                }
                if (tc.get_lat() == tmax.get_lat())
                        break;
                tc.set_lon(tmin.get_lon());
                tc.set_lat(tc.get_lat() + 1);
        }
        if (ret.first == std::numeric_limits<elevation_t>::max())
                ret.first = nodata;
        if (ret.second == std::numeric_limits<elevation_t>::min())
                ret.second = nodata;
        return ret;
}

PalmGtopo::minmax_elevation_t PalmGtopo::get_minmax_elev(const PolygonHole& p)
{
        Rect r(p.get_bbox());
        PalmGtopo::GtopoCoord tmin(r.get_southwest()), tmax(r.get_northeast());
        Point pdim(((Point)(tmin + PalmGtopo::GtopoCoord(1, 1))) - ((Point)(tmin)));
        minmax_elevation_t ret(std::numeric_limits<elevation_t>::max(), std::numeric_limits<elevation_t>::min());
        for (PalmGtopo::GtopoCoord tc(tmin);;) {
                Point ptile(tc);
                Rect rtile(ptile, ptile + pdim);
                if (!p.is_intersection(rtile) && !p.windingnumber(ptile))
                        continue;
                elevation_t elev(this->operator[](tc));
                if (elev < ret.first)
                        ret.first = elev;
                if (elev > ret.second)
                        ret.second = elev;
                if (tc.get_lon() != tmax.get_lon()) {
                        tc.set_lon(tc.get_lon() + 1);
                        continue;
                }
                if (tc.get_lat() == tmax.get_lat())
                        break;
                tc.set_lon(tmin.get_lon());
                tc.set_lat(tc.get_lat() + 1);
        }
        if (ret.first == std::numeric_limits<elevation_t>::max())
                ret.first = nodata;
        if (ret.second == std::numeric_limits<elevation_t>::min())
                ret.second = nodata;
        return ret;
}

PalmGtopo::minmax_elevation_t PalmGtopo::get_minmax_elev(const MultiPolygonHole& p)
{
	minmax_elevation_t r(nodata, nodata);
	for (MultiPolygonHole::const_iterator pi(p.begin()), pe(p.end()); pi != pe; ++pi) {
		minmax_elevation_t m(get_minmax_elev(*pi));
		if (r.first == nodata)
			r.first = m.first;
		else if (m.first != nodata)
			r.first = std::min(r.first, m.first);
		if (r.second == nodata)
			r.second = m.second;
		else if (m.second != nodata)
			r.second = std::max(r.second, m.second);
	}
	return r;
}

template class PalmFindByName<PalmAirportBase>;
template class PalmFindByIcao<PalmFindByName<PalmAirportBase> >;
template class PalmFindByText<PalmFindByIcao<PalmFindByName<PalmAirportBase> > >;
template class PalmFindNearest<PalmFindByText<PalmFindByIcao<PalmFindByName<PalmAirportBase> > > >;
template class PalmFindBboxBox<PalmFindNearest<PalmFindByText<PalmFindByIcao<PalmFindByName<PalmAirportBase> > > > >;

template class PalmFindByName<PalmAirspaceBase>;
template class PalmFindByIcao<PalmFindByName<PalmAirspaceBase> >;
template class PalmFindByText<PalmFindByIcao<PalmFindByName<PalmAirspaceBase> > >;
template class PalmFindNearest<PalmFindByText<PalmFindByIcao<PalmFindByName<PalmAirspaceBase> > > >;
template class PalmFindBboxBox<PalmFindNearest<PalmFindByText<PalmFindByIcao<PalmFindByName<PalmAirspaceBase> > > > >;

template class PalmFindByName<PalmNavaidBase>;
template class PalmFindByIcao<PalmFindByName<PalmNavaidBase> >;
template class PalmFindByText<PalmFindByIcao<PalmFindByName<PalmNavaidBase> > >;
template class PalmFindNearest<PalmFindByText<PalmFindByIcao<PalmFindByName<PalmNavaidBase> > > >;
template class PalmFindBboxPoint<PalmFindNearest<PalmFindByText<PalmFindByIcao<PalmFindByName<PalmNavaidBase> > > > >;

template class PalmFindByName<PalmWaypointBase>;
template class PalmFindNearest<PalmFindByName<PalmWaypointBase> >;
template class PalmFindBboxPoint<PalmFindNearest<PalmFindByName<PalmWaypointBase> > >;

template class PalmFindByName<PalmMapelementBase>;
template class PalmFindNearest<PalmFindByName<PalmMapelementBase> >;
template class PalmFindBboxBox<PalmFindNearest<PalmFindByName<PalmMapelementBase> > >;
