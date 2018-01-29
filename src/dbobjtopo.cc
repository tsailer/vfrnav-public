//
// C++ Implementation: dbobjtopo
//
// Description: Database Objects: Topography
//
//
// Author: Thomas Sailer <t.sailer@alumni.ethz.ch>, (C) 2007, 2009, 2012, 2013, 2014, 2017
//
// Copyright: See COPYING file that comes with this distribution
//
//

#include "sysdeps.h"

#include <limits>
#include <zlib.h>

#if defined(HAVE_WINDOWS_H)
#include <windows.h>
#else
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/mman.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#endif

#include "dbobj.h"

template<int resolution, int tilesize>
const typename TopoDbN<resolution,tilesize>::pixel_index_t TopoDbN<resolution,tilesize>::tile_size;

template<int resolution, int tilesize>
const typename TopoDbN<resolution,tilesize>::pixel_index_t TopoDbN<resolution,tilesize>::pixels_per_tile;

template<int resolution, int tilesize>
const typename TopoDbN<resolution,tilesize>::elev_t TopoDbN<resolution,tilesize>::nodata = std::numeric_limits<elev_t>::min();

template<int resolution, int tilesize>
const typename TopoDbN<resolution,tilesize>::elev_t TopoDbN<resolution,tilesize>::ocean = std::numeric_limits<elev_t>::min() + 1;

template<int resolution, int tilesize>
const typename TopoDbN<resolution,tilesize>::TopoCoordinate::coord_t TopoDbN<resolution,tilesize>::TopoCoordinate::from_point;

template<int resolution, int tilesize>
const uint64_t TopoDbN<resolution,tilesize>::TopoCoordinate::to_point;

template<int resolution, int tilesize>
const unsigned int TopoDbN<resolution,tilesize>::tile_cache_size;

template<int resolution, int tilesize>
const typename TopoDbN<resolution,tilesize>::tile_index_t TopoDbN<resolution,tilesize>::lon_tiles;

template<int resolution, int tilesize>
const typename TopoDbN<resolution,tilesize>::tile_index_t TopoDbN<resolution,tilesize>::lat_tiles;

template<int resolution, int tilesize>
const typename TopoDbN<resolution,tilesize>::tile_index_t TopoDbN<resolution,tilesize>::nr_tiles;

template<int resolution, int tilesize>
const uint16_t TopoDbN<resolution,tilesize>::TopoTileCoordinate::lon_tiles;

template<int resolution, int tilesize>
const uint16_t TopoDbN<resolution,tilesize>::TopoTileCoordinate::lat_tiles;

template<int resolution, int tilesize>
TopoDbN<resolution,tilesize>::TopoTileCoordinate::TopoTileCoordinate(const TopoCoordinate & tc)
{
        m_lon_tile = tc.get_lon() / tilesize;
        m_lat_tile = tc.get_lat() / tilesize;
        m_lon_offs = tc.get_lon() - m_lon_tile * tilesize;
        m_lat_offs = tc.get_lat() - m_lat_tile * tilesize;
}

template<int resolution, int tilesize>
void TopoDbN<resolution,tilesize>::TopoTileCoordinate::advance_tile_east(void)
{
        m_lon_tile++;
        if (m_lon_tile >= lon_tiles)
                m_lon_tile = 0;
}

template<int resolution, int tilesize>
void TopoDbN<resolution,tilesize>::TopoTileCoordinate::advance_tile_north(void)
{
        if (m_lat_tile < lat_tiles)
                m_lat_tile++;
}

template<int resolution, int tilesize>
void TopoDbN<resolution,tilesize>::TopoTileCoordinate::advance_tile_west(void)
{
        if (!m_lon_tile)
                m_lon_tile = lon_tiles;
        m_lon_tile--;
}

template<int resolution, int tilesize>
void TopoDbN<resolution,tilesize>::TopoTileCoordinate::advance_tile_south(void)
{
        if (m_lat_tile)
                m_lat_tile--;
}

template<int resolution, int tilesize>
TopoDbN<resolution,tilesize>::TopoCoordinate::TopoCoordinate(const Point & pt)
{
        int64_t lat1 = pt.get_lat();
        lat1 += 0x40000000;
        uint64_t lon = (uint32_t)pt.get_lon();
        uint64_t lat = std::min(std::max(lat1, (int64_t)0), (int64_t)0x80000000);
        lon *= from_point;
        lat *= from_point;
        m_lon = lon >> 32;
        m_lat = lat >> 32;
}

template<int resolution, int tilesize>
TopoDbN<resolution,tilesize>::TopoCoordinate::operator Point(void) const
{
        uint64_t lon(m_lon), lat(m_lat);
        lon *= to_point;
        lat *= to_point;
        lon += (((uint64_t)1) << 31);
        lat += (((uint64_t)1) << 31);
        Point pt(lon >> 32, (lat >> 32) - 0x40000000);
        return pt;
}

template<int resolution, int tilesize>
std::ostream & TopoDbN<resolution,tilesize>::TopoCoordinate::print(std::ostream & os)
{
        return os << '(' << get_lon() << ',' << get_lat() << ')';
}

template<int resolution, int tilesize>
void TopoDbN<resolution,tilesize>::TopoCoordinate::advance_east(void)
{
        m_lon++;
        if (m_lon == from_point)
                m_lon = 0;
}

template<int resolution, int tilesize>
void TopoDbN<resolution,tilesize>::TopoCoordinate::advance_west(void)
{
        if (!m_lon)
                m_lon = from_point;
        m_lon--;
}

template<int resolution, int tilesize>
void TopoDbN<resolution,tilesize>::TopoCoordinate::advance_north(void)
{
        if (m_lat < from_point/2)
                m_lat++;
}

template<int resolution, int tilesize>
void TopoDbN<resolution,tilesize>::TopoCoordinate::advance_south(void)
{
        if (m_lat)
                m_lat--;
}

template<int resolution, int tilesize>
typename TopoDbN<resolution,tilesize>::TopoCoordinate::coord_t TopoDbN<resolution,tilesize>::TopoCoordinate::lon_dist(const TopoCoordinate & tc) const
{
        coord_t r = tc.get_lon() + from_point - get_lon();
        if (r >= from_point)
                r -= from_point;
        return r;
}

template<int resolution, int tilesize>
typename TopoDbN<resolution,tilesize>::TopoCoordinate::coord_t TopoDbN<resolution,tilesize>::TopoCoordinate::lat_dist(const TopoCoordinate & tc) const
{
        return tc.get_lat() - get_lat();
}

template<int resolution, int tilesize>
typename TopoDbN<resolution,tilesize>::TopoCoordinate::coords_t TopoDbN<resolution,tilesize>::TopoCoordinate::lon_dist_signed(const TopoCoordinate& tc) const
{
	coords_t r(lon_dist(tc));
	if (r >= (from_point / 2U))
		r -= from_point;
	return r;
}

template<int resolution, int tilesize>
typename TopoDbN<resolution,tilesize>::TopoCoordinate::coords_t TopoDbN<resolution,tilesize>::TopoCoordinate::lat_dist_signed(const TopoCoordinate& tc) const
{
        return tc.get_lat() - get_lat();
}

template<int resolution, int tilesize>
TopoDbN<resolution,tilesize>::Tile::Tile(tile_index_t index)
        : m_refcount(1), m_index(index), m_lastaccess(0), m_minelev(nodata), m_maxelev(nodata),
	  m_flags(flags_dataowned)
{
	m_elevdata = new uint8_t[2 * pixels_per_tile];
	for (pixel_index_t idx = 0; idx < pixels_per_tile; ++idx) {
		m_elevdata[2 * idx] = nodata;
		m_elevdata[2 * idx + 1] = nodata >> 8;
	}
}

template<int resolution, int tilesize>
TopoDbN<resolution,tilesize>::Tile::Tile(const Tile& tile)
        : m_refcount(1), m_index(tile.m_index), m_lastaccess(0), m_minelev(tile.m_minelev), m_maxelev(tile.m_maxelev),
	  m_flags(tile.m_flags)
{
	m_elevdata = new uint8_t[2 * pixels_per_tile];
	for (pixel_index_t idx = 0; idx < 2 * pixels_per_tile; ++idx)
		m_elevdata[idx] = tile.m_elevdata[idx];
}

template<int resolution, int tilesize>
TopoDbN<resolution,tilesize>::Tile::Tile(sqlite3x::sqlite3_connection & db, tile_index_t index)
        : m_refcount(1), m_index(index), m_lastaccess(0), m_minelev(nodata), m_maxelev(nodata),
	  m_flags(flags_dataowned)
{
	m_elevdata = new uint8_t[2 * pixels_per_tile];
	for (pixel_index_t idx = 0; idx < pixels_per_tile; ++idx) {
		m_elevdata[2 * idx] = nodata;
		m_elevdata[2 * idx + 1] = nodata >> 8;
	}
	std::string qs("SELECT MINELEV,MAXELEV,ELEV FROM topo WHERE ID=?1;");
	if (false)
 		std::cerr << "sql: " << qs << std::endl;
	sqlite3x::sqlite3_command cmd(db, qs);
	int nr = 1;
	cmd.bind(nr++, (int)m_index);
	sqlite3x::sqlite3_cursor cursor = cmd.executecursor();
	if (!cursor.step())
		return;
	load(cursor);
}

template<int resolution, int tilesize>
TopoDbN<resolution,tilesize>::Tile::Tile(sqlite3x::sqlite3_cursor& cursor)
        : m_refcount(1), m_index(0), m_lastaccess(0), m_minelev(nodata), m_maxelev(nodata),
	  m_flags(flags_dataowned)
{
	m_elevdata = new uint8_t[2 * pixels_per_tile];
	for (pixel_index_t idx = 0; idx < pixels_per_tile; ++idx) {
		m_elevdata[2 * idx] = nodata;
		m_elevdata[2 * idx + 1] = nodata >> 8;
	}
	m_index = cursor.getint(3);
	load(cursor);
}

template<int resolution, int tilesize>
TopoDbN<resolution,tilesize>::Tile::Tile(const BinFileHeader& hdr, tile_index_t index)
        : m_refcount(1), m_index(index), m_lastaccess(0), m_minelev(nodata), m_maxelev(nodata),
	  m_flags(0)
{
	const typename BinFileHeader::Tile& t(hdr[index]);
	if (index >= nr_tiles || !t.get_offset()) {
		m_elevdata = new uint8_t[2 * pixels_per_tile];
		for (pixel_index_t idx = 0; idx < pixels_per_tile; ++idx) {
			m_elevdata[2 * idx] = nodata;
			m_elevdata[2 * idx + 1] = nodata >> 8;
		}
		set_dataowned(true);
		return;
	}
	m_elevdata = const_cast<uint8_t *>((const uint8_t *)&hdr) + t.get_offset();
	m_minelev = t.get_minelev();
	m_maxelev = t.get_maxelev();
}

template<int resolution, int tilesize>
void TopoDbN<resolution,tilesize>::Tile::load(sqlite3x::sqlite3_cursor& cursor)
{
	if (!is_dataowned()) {
		m_elevdata = new uint8_t[2 * pixels_per_tile];
		set_dataowned(true);
	}
	set_loaded(true);
	set_minmaxinvalid(false);
	elev_t minelev = m_minelev = cursor.getint(0);
	elev_t maxelev = m_maxelev = cursor.getint(1);
	if (m_minelev == ocean)
		m_minelev = 0;
	if (m_maxelev == ocean)
		m_maxelev = 0;
	int blobsize(0);
	bool ok(!cursor.isnull(2));
	if (ok) {
		void const *blob(cursor.getblob(2, blobsize));
		z_stream d_stream;
		memset(&d_stream, 0, sizeof(d_stream));
		d_stream.zalloc = (alloc_func)0;
		d_stream.zfree = (free_func)0;
		d_stream.opaque = (voidpf)0;
		d_stream.next_in  = (Bytef *)blob;
		d_stream.avail_in = blobsize;
		int err = inflateInit(&d_stream);
		if (err != Z_OK) {
			inflateEnd(&d_stream);
			std::cerr << "zlib: inflateInit error " << err << std::endl;
			return;
		}
		d_stream.next_out = m_elevdata;
		d_stream.avail_out = 2 * pixels_per_tile;
		err = inflate(&d_stream, Z_FINISH);
		inflateEnd(&d_stream);
		if (err != Z_STREAM_END) {
			std::cerr << "zlib: inflate error " << err << std::endl;
			ok = false;
		}
	}
	if (!ok) {
		elev_t e(nodata);
		if (minelev == maxelev)
			e = minelev;
		for (pixel_index_t i = 0; i < pixels_per_tile; i++) {
			m_elevdata[2 * i] = e;
			m_elevdata[2 * i + 1] = e >> 8;
		}
	}
	if (false)
		std::cerr << "TopoDb::Tile::Tile: index " << m_index << " blobsize " << blobsize << std::endl;
}

template<int resolution, int tilesize>
TopoDbN<resolution,tilesize>::Tile::~Tile(void)
{
	if (is_dataowned())
		delete[] m_elevdata;
}

template<int resolution, int tilesize>
void TopoDbN<resolution,tilesize>::Tile::reference(void) const
{
	g_atomic_int_inc(&m_refcount);
}

template<int resolution, int tilesize>
void TopoDbN<resolution,tilesize>::Tile::unreference(void) const
{
	if (!g_atomic_int_dec_and_test(&m_refcount))
		return;
	delete this;
}

template<int resolution, int tilesize>
void TopoDbN<resolution,tilesize>::Tile::save(sqlite3x::sqlite3_connection & db)
{
	if (!is_dirty())
		return;
	if (is_minmaxinvalid())
		update_minmax();
	std::cerr << "Saving tile: ID " << m_index << " min: " << m_minelev << " max: " << m_maxelev << std::endl;
	sqlite3x::sqlite3_transaction tran(db);
	{
		sqlite3x::sqlite3_command cmd(db, "INSERT OR REPLACE INTO topo (ID) VALUES(?);");
		cmd.bind(1, (int)m_index);
		cmd.executenonquery();
	}
	{
		sqlite3x::sqlite3_command cmd(db, "UPDATE topo SET MINELEV=?2,MAXELEV=?3,ELEV=?4 WHERE ID=?1;");
		cmd.bind(1, (int)m_index);
		if (m_minelev == 0 && m_maxelev == 0 &&
		    m_elevdata[0] == (uint8_t)ocean && m_elevdata[1] == (uint8_t)(ocean >> 8)) {
			cmd.bind(2, (long long int)ocean);
			cmd.bind(3, (long long int)ocean);
		} else {
			cmd.bind(2, (long long int)m_minelev);
			cmd.bind(3, (long long int)m_maxelev);
		}
		cmd.bind(4);
		if (m_minelev != m_maxelev) {
			uint8_t cdata[2*pixels_per_tile+64];
			z_stream c_stream;
			memset(&c_stream, 0, sizeof(c_stream));
			c_stream.zalloc = (alloc_func)0;
			c_stream.zfree = (free_func)0;
			c_stream.opaque = (voidpf)0;
			int err = deflateInit(&c_stream, Z_BEST_COMPRESSION);
			if (err) {
				std::cerr << "deflateInit error " << err << ", " << c_stream.msg << std::endl;
				goto outerr2;
			}
			c_stream.next_out = cdata;
			c_stream.avail_out = 2*pixels_per_tile+64;
			c_stream.next_in = (Bytef *)m_elevdata;
			c_stream.avail_in = 2*pixels_per_tile;
			err = deflate(&c_stream, Z_FINISH);
			if (err != Z_STREAM_END) {
				std::cerr << "deflate error " << err << ", " << c_stream.msg << std::endl;
				goto outerr2;
			}
			if (c_stream.avail_in != 0 || c_stream.total_in != 2*pixels_per_tile) {
				std::cerr << "deflate did not consume all input" << std::endl;
				goto outerr2;
			}
			cmd.bind(4, cdata, c_stream.total_out);
			std::cerr << "Tile compression: input: " << (2*pixels_per_tile) << " output: " << c_stream.total_out << std::endl;
			err = deflateEnd(&c_stream);
			if (err) {
				std::cerr << "deflateEnd error " << err << ", " << c_stream.msg << std::endl;
				goto outerr2;
			}
		  outerr2:;
		}
		cmd.executenonquery();
	}
	tran.commit();
	set_loaded(true);
	set_dirty(false);
}

template<int resolution, int tilesize>
typename TopoDbN<resolution,tilesize>::elev_t TopoDbN<resolution,tilesize>::Tile::get_elev(pixel_index_t index) const
{
        if (index < 0 || index >= pixels_per_tile)
                return nodata;
        return m_elevdata[2 * index] | (m_elevdata[2 * index + 1] << 8);
}

template<int resolution, int tilesize>
void TopoDbN<resolution,tilesize>::Tile::set_elev(pixel_index_t index, elev_t elev)
{
        if (index < 0 || index >= pixels_per_tile)
                return;
       	uint8_t e[2] = { elev, elev >> 8 };
        if (e[0] == m_elevdata[2 * index] && e[1] == m_elevdata[2 * index + 1])
		return;
	if (!is_dataowned()) {
		uint8_t *ed(new uint8_t[2 * pixels_per_tile]);
		for (pixel_index_t idx = 0; idx < pixels_per_tile; ++idx) {
			ed[2 * idx] = m_elevdata[2 * idx];
			ed[2 * idx + 1] = m_elevdata[2 * idx + 1];
		}
		m_elevdata = ed;
		set_dataowned(true);
	}
	set_dirty(true);
	m_elevdata[2 * index] = e[0];
	m_elevdata[2 * index + 1] = e[1];
	set_minmaxinvalid(false);
}

template<int resolution, int tilesize>
void TopoDbN<resolution,tilesize>::Tile::update_minmax(void) const
{
        int16_t mi(std::numeric_limits<elev_t>::max()), ma(std::numeric_limits<elev_t>::min());
	for (unsigned int i = 0; i < pixels_per_tile; i++) {
                elev_t e(m_elevdata[2 * i] | (m_elevdata[2 * i + 1] << 8));
                if (e == nodata)
                        continue;
                if (e == ocean)
                        e = 0;
                mi = std::min(mi, e);
                ma = std::max(ma, e);
        }
        m_minelev = mi;
        m_maxelev = ma;
        set_minmaxinvalid(false);
}

template<int resolution, int tilesize>
TopoDbN<resolution,tilesize>::ProfilePoint::ProfilePoint(double dist, elev_t elev, elev_t minelev, elev_t maxelev)
	: m_dist(dist), m_elev(elev), m_minelev(minelev), m_maxelev(maxelev)
{
	if (m_elev == ocean)
		m_elev = 0;
	if (m_minelev == ocean)
		m_minelev = 0;
	if (m_maxelev == ocean)
	        m_maxelev = 0;
}

template<int resolution, int tilesize>
TopoDbN<resolution,tilesize>::ProfilePoint::ProfilePoint(double dist, elev_t elev, minmax_elev_t minmaxelev)
	: m_dist(dist), m_elev(elev), m_minelev(minmaxelev.first), m_maxelev(minmaxelev.second)
{
	if (m_elev == ocean)
		m_elev = 0;
	if (m_minelev == ocean)
		m_minelev = 0;
	if (m_maxelev == ocean)
	        m_maxelev = 0;
}

template<int resolution, int tilesize>
TopoDbN<resolution,tilesize>::ProfilePoint::ProfilePoint(double dist, elev_t elev)
	: m_dist(dist), m_elev(elev), m_minelev(elev), m_maxelev(elev)
{
	if (m_elev == ocean)
		m_elev = m_minelev = m_maxelev = 0;
}

template<int resolution, int tilesize>
TopoDbN<resolution,tilesize>::ProfilePoint::ProfilePoint(double dist)
	: m_dist(dist), m_elev(nodata), m_minelev(nodata), m_maxelev(nodata)
{
}

template<int resolution, int tilesize>
void TopoDbN<resolution,tilesize>::ProfilePoint::scaledist(double ds)
{
	m_dist *= ds;
}

template<int resolution, int tilesize>
void TopoDbN<resolution,tilesize>::ProfilePoint::adddist(double d)
{
	m_dist += d;
}

template<int resolution, int tilesize>
void TopoDbN<resolution,tilesize>::ProfilePoint::add(elev_t elev)
{
	if (elev == nodata)
		return;
	if (elev == ocean)
		elev = 0;
	if (m_elev == nodata) {
		m_elev = elev;
		m_minelev = elev;
		m_maxelev = elev;
		return;
	}
	m_minelev = std::min(m_minelev, elev);
	m_maxelev = std::max(m_maxelev, elev);
}

template<int resolution, int tilesize>
void TopoDbN<resolution,tilesize>::Profile::scaledist(double ds)
{
	for (typename base_t::iterator i(this->begin()), e(this->end()); i != e; ++i)
		i->scaledist(ds);
}

template<int resolution, int tilesize>
void TopoDbN<resolution,tilesize>::Profile::adddist(double d)
{
	for (typename base_t::iterator i(this->begin()), e(this->end()); i != e; ++i)
		i->adddist(d);
}

template<int resolution, int tilesize>
double TopoDbN<resolution,tilesize>::Profile::get_dist(void) const
{
	if (this->empty())
		return 0;
	return this->operator[](this->size() - 1).get_dist();
}

template<int resolution, int tilesize>
typename TopoDbN<resolution,tilesize>::elev_t TopoDbN<resolution,tilesize>::Profile::get_minelev(void) const
{
	elev_t el(std::numeric_limits<elev_t>::max());
	for (typename base_t::const_iterator i(this->begin()), e(this->end()); i != e; ++i) {
		elev_t ee(i->get_minelev());
		if (ee == nodata)
			continue;
		el = std::min(el, ee);
	}
	if (el == std::numeric_limits<elev_t>::max())
		return nodata;
	return el;
}

template<int resolution, int tilesize>
typename TopoDbN<resolution,tilesize>::elev_t TopoDbN<resolution,tilesize>::Profile::get_maxelev(void) const
{
	elev_t el(std::numeric_limits<elev_t>::min());
	for (typename base_t::const_iterator i(this->begin()), e(this->end()); i != e; ++i) {
		elev_t ee(i->get_maxelev());
		if (ee == nodata)
			continue;
		el = std::max(el, ee);
	}
	if (el == std::numeric_limits<elev_t>::min())
		return nodata;
	return el;
}

template<int resolution, int tilesize>
TopoDbN<resolution,tilesize>::RouteProfilePoint::RouteProfilePoint(const ProfilePoint& pp, unsigned int rteidx, double rtedist)
	: ProfilePoint(pp), m_routeindex(rteidx), m_routedist(rtedist + pp.get_dist())
{
}

template<int resolution, int tilesize>
double TopoDbN<resolution,tilesize>::RouteProfile::get_dist(void) const
{
	if (this->empty())
		return 0;
	return this->operator[](this->size() - 1).get_routedist();
}

template<int resolution, int tilesize>
typename TopoDbN<resolution,tilesize>::elev_t TopoDbN<resolution,tilesize>::RouteProfile::get_minelev(void) const
{
	elev_t el(std::numeric_limits<elev_t>::max());
	for (typename base_t::const_iterator i(this->begin()), e(this->end()); i != e; ++i) {
		elev_t ee(i->get_minelev());
		if (ee == nodata)
			continue;
		el = std::min(el, ee);
	}
	if (el == std::numeric_limits<elev_t>::max())
		return nodata;
	return el;
}

template<int resolution, int tilesize>
typename TopoDbN<resolution,tilesize>::elev_t TopoDbN<resolution,tilesize>::RouteProfile::get_maxelev(void) const
{
	elev_t el(std::numeric_limits<elev_t>::min());
	for (typename base_t::const_iterator i(this->begin()), e(this->end()); i != e; ++i) {
		elev_t ee(i->get_maxelev());
		if (ee == nodata)
			continue;
		el = std::max(el, ee);
	}
	if (el == std::numeric_limits<elev_t>::min())
		return nodata;
	return el;
}

template<int resolution, int tilesize>
class TopoDbN<resolution,tilesize>::RouteProfile::RouteDistCompare {
public:
	bool operator()(const RouteProfilePoint& a, double b) const { return a.get_routedist() < b; }
	bool operator()(double a, const RouteProfilePoint& b) const { return a < b.get_routedist(); }
	bool operator()(const RouteProfilePoint& a, const RouteProfilePoint& b) const { return a.get_routedist() < b.get_routedist(); }
};

template<int resolution, int tilesize>
typename TopoDbN<resolution,tilesize>::RouteProfilePoint TopoDbN<resolution,tilesize>::RouteProfile::interpolate(double rtedist) const
{
	if (this->empty() || std::isnan(rtedist))
		return RouteProfilePoint(ProfilePoint());
	if (rtedist <= 0)
		return this->front();
	if (rtedist >= this->back().get_routedist())
		return this->back();
	typename RouteProfile::const_iterator i2(std::upper_bound(this->begin(), this->end(), rtedist, RouteDistCompare()));
	if (i2 == this->begin())
		return *i2;
	typename RouteProfile::const_iterator i1(i2 - 1);
	if (i2 == this->end())
		return *i1;
	double t(i2->get_routedist() - i1->get_routedist());
	if (t <= 0 || std::isnan(t))
		return *i1;
	t = (rtedist - i1->get_routedist()) / t;
	elev_t elev(nodata), minelev(nodata), maxelev(nodata);
	if (i1->get_elev() != nodata && i2->get_elev() != nodata)
		elev = i1->get_elev() + (i2->get_elev() - i1->get_elev()) * t;
	if (i1->get_minelev() == nodata) {
		if (i2->get_minelev() != nodata)
			minelev = i2->get_minelev();
	} else if (i2->get_minelev() == nodata) {
		minelev = i1->get_minelev();
	} else {
		minelev = std::min(i1->get_minelev(), i2->get_minelev());
	}
	if (i1->get_maxelev() == nodata) {
		if (i2->get_maxelev() != nodata)
			maxelev = i2->get_maxelev();
	} else if (i2->get_maxelev() == nodata) {
		maxelev = i1->get_maxelev();
	} else {
		maxelev = std::max(i1->get_maxelev(), i2->get_maxelev());
	}
	return RouteProfilePoint(ProfilePoint(i1->get_dist() + (rtedist - i1->get_routedist()), elev, minelev, maxelev),
				 i1->get_routeindex(), rtedist);
}

template<int resolution, int tilesize>
TopoDbN<resolution,tilesize>::TileCache::TileCache(void)
	: m_lastaccess(0)
{
}

template<int resolution, int tilesize>
TopoDbN<resolution,tilesize>::TileCache::~TileCache()
{
	Glib::RWLock::WriterLock lock(m_lock);
	m_cache.clear();
}

template<int resolution, int tilesize>
void TopoDbN<resolution,tilesize>::TileCache::clear(void)
{
	Glib::RWLock::WriterLock lock(m_lock);
	m_cache.clear();
}

template<int resolution, int tilesize>
typename TopoDbN<resolution,tilesize>::Tile::ptr_t TopoDbN<resolution,tilesize>::TileCache::find(tile_index_t index, sqlite3x::sqlite3_connection& db)
{
	g_atomic_int_inc(&m_lastaccess);
	guint lastacc(g_atomic_int_get(&m_lastaccess));
	{
		Glib::RWLock::ReaderLock lock(m_lock);
		typename cache_t::iterator ti(m_cache.find(index));
		if (ti != m_cache.end()) {
			typename Tile::ptr_t& p(ti->second);
			if (p) {
				p->set_lastaccess(lastacc);
				return p;
			}
		}
	}
	Glib::RWLock::WriterLock lock(m_lock);
	typename cache_t::iterator te(m_cache.end()), ti(te);
	guint maxage(0);
	unsigned int count(0);
	for (typename cache_t::iterator ti2(m_cache.begin()); ti2 != te; ) {
		typename Tile::ptr_t& p(ti2->second);
		typename cache_t::iterator ti3(ti2);
		++ti2;
		if (!p) {
			p->save(db);
			m_cache.erase(ti3);
			continue;
		}
		++count;
		guint age(lastacc - p->get_lastaccess());
		if (ti == te || age > maxage)
			ti = ti3;
	}
	if (count >= tile_cache_size && ti != te)
		m_cache.erase(ti);
	std::pair<typename cache_t::iterator,bool> ins(m_cache.insert(typename cache_t::value_type(index, typename Tile::ptr_t(new Tile(db, index)))));
	typename Tile::ptr_t& p(ins.first->second);
	p->set_lastaccess(lastacc);
	return p;
}

template<int resolution, int tilesize>
void TopoDbN<resolution,tilesize>::TileCache::commit(sqlite3x::sqlite3_connection& db)
{
	Glib::RWLock::WriterLock lock(m_lock);
	for (typename cache_t::iterator ti(m_cache.begin()), te(m_cache.end()); ti != te; ++ti) {
		typename Tile::ptr_t& p(ti->second);
		if (!p)
			continue;
                p->save(db);
        }
}

template<int resolution, int tilesize>
TopoDbN<resolution,tilesize>::BinFileHeader::Tile::Tile(uint32_t offs, elev_t minelev, elev_t maxelev)
{
	m_offset[0] = offs;
	m_offset[1] = offs >> 8;
	m_offset[2] = offs >> 16;
	m_offset[3] = offs >> 24;
	m_minelev[0] = minelev;
	m_minelev[1] = minelev >> 8;
	m_maxelev[0] = maxelev;
	m_maxelev[1] = maxelev >> 8;
}

template<int resolution, int tilesize>
uint32_t TopoDbN<resolution,tilesize>::BinFileHeader::Tile::get_offset(void) const
{
	return (m_offset[3] << 24) | (m_offset[2] << 16) | (m_offset[1] << 8) | m_offset[0];
}

template<int resolution, int tilesize>
typename TopoDbN<resolution,tilesize>::elev_t TopoDbN<resolution,tilesize>::BinFileHeader::Tile::get_minelev(void) const
{
	return (m_minelev[1] << 8) | m_minelev[0];
}

template<int resolution, int tilesize>
typename TopoDbN<resolution,tilesize>::elev_t TopoDbN<resolution,tilesize>::BinFileHeader::Tile::get_maxelev(void) const
{
	return (m_maxelev[1] << 8) | m_maxelev[0];
}

template<int resolution, int tilesize>
void TopoDbN<resolution,tilesize>::BinFileHeader::Tile::set_offset(uint32_t offs)
{
	m_offset[0] = offs;
	m_offset[1] = offs >> 8;
	m_offset[2] = offs >> 16;
	m_offset[3] = offs >> 24;
}

template<int resolution, int tilesize>
void TopoDbN<resolution,tilesize>::BinFileHeader::Tile::set_minelev(elev_t elev)
{
	m_minelev[0] = elev;
	m_minelev[1] = elev >> 8;
}

template<int resolution, int tilesize>
void TopoDbN<resolution,tilesize>::BinFileHeader::Tile::set_maxelev(elev_t elev)
{
	m_maxelev[0] = elev;
	m_maxelev[1] = elev >> 8;
}

#define STRINGIFY(x) #x

template<int resolution, int tilesize>
const char TopoDbN<resolution,tilesize>::BinFileHeader::signature[] = "TopoDb" STRINGIFY(resolution) "_" STRINGIFY(tilesize) " V1";

template<int resolution, int tilesize>
const typename TopoDbN<resolution,tilesize>::BinFileHeader::Tile TopoDbN<resolution,tilesize>::BinFileHeader::nulltile(~0U);

template<int resolution, int tilesize>
typename TopoDbN<resolution,tilesize>::BinFileHeader::Tile& TopoDbN<resolution,tilesize>::BinFileHeader::operator[](unsigned int idx)
{
	if (idx < nr_tiles)
		return m_tiles[idx];
	return const_cast<typename TopoDbN<resolution,tilesize>::BinFileHeader::Tile&>(nulltile);
}

template<int resolution, int tilesize>
const typename TopoDbN<resolution,tilesize>::BinFileHeader::Tile& TopoDbN<resolution,tilesize>::BinFileHeader::operator[](unsigned int idx) const
{
	if (idx < nr_tiles)
		return m_tiles[idx];
	return nulltile;
}

template<int resolution, int tilesize>
TopoDbN<resolution,tilesize>::BinFileHeader::BinFileHeader(void)
{
	memset(m_signature, 0, sizeof(m_signature));
	memcpy(m_signature, signature, sizeof(signature));
}

template<int resolution, int tilesize>
bool TopoDbN<resolution,tilesize>::BinFileHeader::check_signature(void) const
{
	return !memcmp(m_signature, signature, sizeof(signature));
}

template<int resolution, int tilesize>
void TopoDbN<resolution,tilesize>::BinFileHeader::set_signature(void)
{
	memset(m_signature, 0, sizeof(m_signature));
	memcpy(m_signature, signature, sizeof(signature));
}

template<int resolution, int tilesize>
TopoDbN<resolution,tilesize>::TopoDbN()
	: m_binhdr(0), m_binsz(0)
{
}

template<int resolution, int tilesize>
void TopoDbN<resolution,tilesize>::open(const Glib::ustring& path, const Glib::ustring& dbname)
{
        Glib::ustring dbname1(path);
        if (dbname1.empty())
                dbname1 = FPlan::get_userdbpath();
        dbname1 = Glib::build_filename(dbname1, dbname);
        m_db.open(dbname1);
        create_tables();
        create_indices();
}

template<int resolution, int tilesize>
void TopoDbN<resolution,tilesize>::open_readonly(const Glib::ustring& path, const Glib::ustring& dbname, const Glib::ustring& binname)
{
        Glib::ustring dbname2(path);
        if (dbname2.empty())
                dbname2 = FPlan::get_userdbpath();
        Glib::ustring dbname1(Glib::build_filename(dbname2, dbname));
	{
		sqlite3 *h(0);
		if (SQLITE_OK == sqlite3_open_v2(dbname1.c_str(), &h, SQLITE_OPEN_READONLY, 0))
			m_db.take(h);
		else
			throw sqlite3x::database_error("unable to open database");
	}
	if (!binname.empty())
		open_bin(dbname1, Glib::build_filename(dbname2, binname));
}

template<int resolution, int tilesize>
void TopoDbN<resolution,tilesize>::take(sqlite3 * dbh)
{
        m_db.take(dbh);
        if (!m_db.db())
                return;
        create_tables();
        create_indices();
	close_bin();
}

template<int resolution, int tilesize>
void TopoDbN<resolution,tilesize>::close(void)
{
        cache_commit();
        m_db.close();
	close_bin();
}

template<int resolution, int tilesize>
void TopoDbN<resolution,tilesize>::purgedb(void)
{
        m_cache.clear();
        drop_indices();
        drop_tables();
        create_tables();
        create_indices();
	close_bin();
}

template<int resolution, int tilesize>
void TopoDbN<resolution,tilesize>::sync_off(void)
{
        sqlite3x::sqlite3_command cmd(m_db, "PRAGMA synchronous = OFF;");
        cmd.executenonquery();
}

template<int resolution, int tilesize>
void TopoDbN<resolution,tilesize>::analyze(void)
{
        cache_commit();
        sqlite3x::sqlite3_command cmd(m_db, "ANALYZE;");
        cmd.executenonquery();
}

template<int resolution, int tilesize>
void TopoDbN<resolution,tilesize>::vacuum(void)
{
        cache_commit();
        sqlite3x::sqlite3_command cmd(m_db, "VACUUM;");
        cmd.executenonquery();
}

template<int resolution, int tilesize>
void TopoDbN<resolution,tilesize>::interrupt(void)
{
        sqlite3_interrupt(m_db.db());
}

template<int resolution, int tilesize>
void TopoDbN<resolution,tilesize>::drop_tables(void)
{
        { sqlite3x::sqlite3_command cmd(m_db, "DROP TABLE IF EXISTS topo;"); cmd.executenonquery(); }
}

template<int resolution, int tilesize>
void TopoDbN<resolution,tilesize>::create_tables(void)
{
        {
                sqlite3x::sqlite3_command cmd(m_db, "CREATE TABLE IF NOT EXISTS topo (ID INTEGER PRIMARY KEY, "
                                              "MINELEV INTEGER,"
                                              "MAXELEV INTEGER,"
                                              "ELEV BLOB);");
                cmd.executenonquery();
        }
}

template<int resolution, int tilesize>
void TopoDbN<resolution,tilesize>::drop_indices(void)
{
        //{ sqlite3x::sqlite3_command cmd(m_db, "DROP INDEX IF EXISTS topo_bbox;"); cmd.executenonquery(); }
}

template<int resolution, int tilesize>
void TopoDbN<resolution,tilesize>::create_indices(void)
{
        //{ sqlite3x::sqlite3_command cmd(m_db, "CREATE INDEX IF NOT EXISTS topo_bbox ON topo(SWLAT,NELAT,SWLON,NELON);"); cmd.executenonquery(); }
}

#if defined(HAVE_WINDOWS_H)

template<int resolution, int tilesize>
void TopoDbN<resolution,tilesize>::open_bin(const Glib::ustring& dbname, const Glib::ustring& binname)
{
	HANDLE hFile = CreateFile(dbname.c_str(), GENERIC_READ, FILE_SHARE_READ, 0,
				  OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, 0);
	if (hFile == INVALID_HANDLE_VALUE) {
		if (true)
			std::cerr << "Cannot open db file " << dbname << ": 0x"
				  << std::hex << GetLastError() << std::dec << std::endl;
		return;
	}
	FILETIME dbtime;
	if (!GetFileTime(hFile, 0, 0, &dbtime)) {
		CloseHandle(hFile);
		if (true)
			std::cerr << "Cannot get db file time " << dbname << ": 0x"
				  << std::hex << GetLastError() << std::dec << std::endl;
		return;
	}
	CloseHandle(hFile);
	hFile = CreateFile(binname.c_str(), GENERIC_READ, FILE_SHARE_READ, 0,
			   OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, 0);
	if (hFile == INVALID_HANDLE_VALUE) {
		if (true)
			std::cerr << "Cannot open bin file " << binname << ": 0x"
				  << std::hex << GetLastError() << std::dec << std::endl;
		return;
	}
	FILETIME bintime;
	if (!GetFileTime(hFile, 0, 0, &bintime)) {
		CloseHandle(hFile);
		if (true)
			std::cerr << "Cannot get bin file time " << binname << ": 0x"
				  << std::hex << GetLastError() << std::dec << std::endl;
		return;
	}
	if (dbtime.dwHighDateTime > bintime.dwHighDateTime ||
	    (dbtime.dwHighDateTime == bintime.dwHighDateTime && dbtime.dwLowDateTime > bintime.dwLowDateTime)) {
		CloseHandle(hFile);
		if (true)
			std::cerr << "Bin file " << binname << " is older than db file " << dbname << ": "
				  << bintime.dwHighDateTime << ',' << bintime.dwLowDateTime << " vs "
				  << dbtime.dwHighDateTime << ',' << dbtime.dwLowDateTime << std::endl;
		return;
	}
	HANDLE hMap = CreateFileMapping(hFile, 0, PAGE_READONLY, 0, 0, 0);
	if (hMap == INVALID_HANDLE_VALUE) {
		CloseHandle(hFile);
		if (true)
			std::cerr << "Cannot create bin file mapping " << binname << ": 0x"
				  << std::hex << GetLastError() << std::dec << std::endl;
		return;
	}
	m_binhdr = reinterpret_cast<BinFileHeader *>(MapViewOfFile(hMap, FILE_MAP_READ, 0, 0, 0));
	m_binsz = 0;
	CloseHandle(hFile);
	CloseHandle(hMap);
	if (!m_binhdr) {
		if (true)
			std::cerr << "Cannot map bin file " << binname << ": 0x"
				  << std::hex << GetLastError() << std::dec << std::endl;
		return;
	}
	MEMORY_BASIC_INFORMATION meminfo;
	if (VirtualQuery(m_binhdr, &meminfo, sizeof(meminfo)) != sizeof(meminfo)) {
		if (true)
			std::cerr << "Cannot get bin file " << binname << " mapping information" << std::endl;
		close_binfile();
		return;
	}
	m_binsz = meminfo.RegionSize;
	const BinFileHeader& hdr(*(const BinFileHeader *)m_binhdr);
	if (!hdr.check_signature()) {
		if (true)
			std::cerr << "Invalid signature on bin file " << binname << std::endl;
		close_bin();
		return;
	}
	if (true)
		std::cerr << "Using bin file " << binname << std::endl;
}

template<int resolution, int tilesize>
void TopoDbN<resolution,tilesize>::close_bin(void)
{
	if (!m_binhdr)
		return;
	if (!UnmapViewOfFile(m_binhdr)) {
		if (true)
			std::cerr << "Error unmapping binfile: 0x" << std::hex << GetLastError() << std::dec << std::endl;
	}
	m_binhdr = 0;
	m_binsz = 0;
}

#else

template<int resolution, int tilesize>
void TopoDbN<resolution,tilesize>::open_bin(const Glib::ustring& dbname, const Glib::ustring& binname)
{
	struct stat dbstat;
	if (stat(dbname.c_str(), &dbstat)) {
		if (true)
			std::cerr << "Cannot stat db file " << dbname << ": "
				  << strerror(errno) << " (" << errno << ')' << std::endl;
		return;
	}
	int fd(::open(binname.c_str(), O_RDONLY /* | O_NOATIME*/, 0));
	if (fd == -1) {
		if (true)
			std::cerr << "Cannot open bin file " << binname << ": "
				  << strerror(errno) << " (" << errno << ')' << std::endl;
		return;
	}
	struct stat binstat;
	if (fstat(fd, &binstat)) {
		if (true)
			std::cerr << "Cannot stat bin file " << binname << ": "
				  << strerror(errno) << " (" << errno << ')' << std::endl;
		return;
	}
	if (dbstat.st_mtime > binstat.st_mtime) {
		::close(fd);
		if (true)
			std::cerr << "Bin file " << binname << " is older than db file " << dbname << ": "
				  << Glib::TimeVal(binstat.st_mtime, 0).as_iso8601() << " vs "
				  << Glib::TimeVal(dbstat.st_mtime, 0).as_iso8601() << std::endl;
		return;
	}
	size_t pgsz(sysconf(_SC_PAGE_SIZE));
	m_binsz = ((binstat.st_size + pgsz - 1) / pgsz) * pgsz;
	m_binhdr = reinterpret_cast<uint8_t *>(mmap(0, m_binsz, PROT_READ, MAP_PRIVATE, fd, 0));
	::close(fd);
	if (m_binhdr == (const uint8_t *)-1) {
		m_binhdr = 0;
		m_binsz = 0;
		if (true)
			std::cerr << "Cannot mmap bin file " << binname << ": "
				  << strerror(errno) << " (" << errno << ')' << std::endl;
		return;
	}
	const BinFileHeader& hdr(*(const BinFileHeader *)m_binhdr);
	if (!hdr.check_signature()) {
		if (true)
			std::cerr << "Invalid signature on bin file " << binname << std::endl;
		close_bin();
		return;
	}
	if (true)
		std::cerr << "Using bin file " << binname << std::endl;
}

template<int resolution, int tilesize>
void TopoDbN<resolution,tilesize>::close_bin(void)
{
	if (!m_binhdr)
		return;
	munmap(const_cast<uint8_t *>(m_binhdr), m_binsz);
	m_binhdr = 0;
	m_binsz = 0;
}

#endif

template<int resolution, int tilesize>
typename TopoDbN<resolution,tilesize>::Tile::ptr_t TopoDbN<resolution,tilesize>::find(tile_index_t index)
{
        if (index >= nr_tiles)
                return typename Tile::ptr_t();
	return m_cache.find(index, m_db);
}

template<int resolution, int tilesize>
void TopoDbN<resolution,tilesize>::cache_commit(void)
{
	m_cache.commit(m_db);
}

template<int resolution, int tilesize>
typename TopoDbN<resolution,tilesize>::elev_t TopoDbN<resolution,tilesize>::get_elev(const TopoTileCoordinate & tc)
{
	if (m_binhdr && tc.get_tile_index() < nr_tiles) {
		const BinFileHeader& hdr(*(const BinFileHeader *)m_binhdr);
		const typename BinFileHeader::Tile& t(hdr[tc.get_tile_index()]);
		const uint8_t *d((const uint8_t *)&hdr + t.get_offset() + 2 * tc.get_pixel_index());
		return d[0] | (d[1] << 8);
	}
	typename Tile::ptr_t t(find(tc.get_tile_index()));
        if (!t) {
                std::cerr << "TopoDbN: cannot find tile " << tc.get_tile_index() << std::endl;
                return nodata;
        }
        return t->get_elev(tc.get_pixel_index());
}

template<int resolution, int tilesize>
void TopoDbN<resolution,tilesize>::set_elev(const TopoTileCoordinate & tc, elev_t elev)
{
	close_bin();
        typename Tile::ptr_t t(find(tc.get_tile_index()));
        if (!t) {
                std::cerr << "TopoDbN: cannot find tile " << tc.get_tile_index() << std::endl;
                return;
        }
        t->set_elev(tc.get_pixel_index(), elev);
}

template<int resolution, int tilesize>
typename TopoDbN<resolution,tilesize>::minmax_elev_t TopoDbN<resolution,tilesize>::get_tile_minmax_elev(tile_index_t index)
{
	minmax_elev_t ret(std::numeric_limits<elev_t>::max(), std::numeric_limits<elev_t>::min());
	if (m_binhdr && index < nr_tiles) {
		const BinFileHeader& hdr(*(const BinFileHeader *)m_binhdr);
		const typename BinFileHeader::Tile& t(hdr[index]);
	        ret.first = t.get_minelev();
		ret.second = t.get_maxelev();
		return ret;
	}
	typename Tile::ptr_t t(find(index));
        if (!t) {
                std::cerr << "TopoDbN: cannot find tile " << index << std::endl;
                return ret;
        }
        ret.first = t->get_minelev();
        ret.second = t->get_maxelev();
        return ret;
}

template<int resolution, int tilesize>
typename TopoDbN<resolution,tilesize>::minmax_elev_t TopoDbN<resolution,tilesize>::get_minmax_elev(const Rect& r)
{
        Point pdim(TopoCoordinate::get_pointsize());
        TopoTileCoordinate tmin(r.get_southwest()), tmax(r.get_northeast());
        minmax_elev_t ret(std::numeric_limits<elev_t>::max(), std::numeric_limits<elev_t>::min());
        for (TopoTileCoordinate tc(tmin);;) {
                pixel_index_t ymin((tc.get_lat_tile() == tmin.get_lat_tile()) ? tmin.get_lat_offs() : 0);
                pixel_index_t ymax((tc.get_lat_tile() == tmax.get_lat_tile()) ? tmax.get_lat_offs() : tile_size - 1);
                pixel_index_t xmin((tc.get_lon_tile() == tmin.get_lon_tile()) ? tmin.get_lon_offs() : 0);
                pixel_index_t xmax((tc.get_lon_tile() == tmax.get_lon_tile()) ? tmax.get_lon_offs() : tile_size - 1);
                Rect rtile((Point)(TopoCoordinate)TopoTileCoordinate(tc.get_lon_tile(), tc.get_lat_tile(), xmin, ymin),
                           (Point)(TopoCoordinate)TopoTileCoordinate(tc.get_lon_tile(), tc.get_lat_tile(), xmax, ymax) + pdim);
                if (r.is_inside(rtile)) {
                        minmax_elev_t mm(get_tile_minmax_elev(tc));
                        ret.first = std::min(ret.first, mm.first);
                        ret.second = std::max(ret.second, mm.second);
                } else {
                        for (pixel_index_t y(ymin); y <= ymax; y++) {
                                tc.set_lat_offs(y);
                                for (pixel_index_t x(xmin); x <= xmax; x++) {
                                        tc.set_lon_offs(x);
                                        elev_t e(get_elev(tc));
                                        if (e == ocean)
                                                e = 0;
                                        if (e != nodata) {
                                                ret.first = std::min(ret.first, e);
                                                ret.second = std::max(ret.second, e);
                                        }
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
        if (ret.first == std::numeric_limits<elev_t>::max())
                ret.first = nodata;
        if (ret.second == std::numeric_limits<elev_t>::min())
                ret.second = nodata;
        return ret;
}

template<int resolution, int tilesize>
typename TopoDbN<resolution,tilesize>::minmax_elev_t TopoDbN<resolution,tilesize>::get_minmax_elev(const PolygonSimple& p)
{
        if (p.empty())
                return minmax_elev_t(nodata, nodata);
        Point pdim(TopoCoordinate::get_pointsize()), pdim2(pdim.get_lon()/2, pdim.get_lat()/2);
        Rect r(p.get_bbox());
        TopoTileCoordinate tmin(r.get_southwest()), tmax(r.get_northeast());
        minmax_elev_t ret(std::numeric_limits<elev_t>::max(), std::numeric_limits<elev_t>::min());
        for (TopoTileCoordinate tc(tmin);;) {
                pixel_index_t ymin((tc.get_lat_tile() == tmin.get_lat_tile()) ? tmin.get_lat_offs() : 0);
                pixel_index_t ymax((tc.get_lat_tile() == tmax.get_lat_tile()) ? tmax.get_lat_offs() : tile_size - 1);
                pixel_index_t xmin((tc.get_lon_tile() == tmin.get_lon_tile()) ? tmin.get_lon_offs() : 0);
                pixel_index_t xmax((tc.get_lon_tile() == tmax.get_lon_tile()) ? tmax.get_lon_offs() : tile_size - 1);
                Rect rtile((Point)(TopoCoordinate)TopoTileCoordinate(tc.get_lon_tile(), tc.get_lat_tile(), xmin, ymin),
                           (Point)(TopoCoordinate)TopoTileCoordinate(tc.get_lon_tile(), tc.get_lat_tile(), xmax, ymax) + pdim);
                bool intersect(p.is_intersection(rtile));
                bool poly_inside(!intersect && rtile.is_inside(p[0]));
                bool tile_inside(!intersect && !poly_inside && p.windingnumber(rtile.get_southwest()));
                if (false) {
                        minmax_elev_t mm(get_tile_minmax_elev(tc));
                        std::cerr << "get_minmax_elev: bbox " << r << " tile " << rtile << " intersect " << (intersect ? "yes" : "no")
                                  << " poly_inside " << (poly_inside ? "yes" : "no") << " tile_inside " << (tile_inside ? "yes" : "no")
                                  << " tile elev " << mm.first << " / " << mm.second << std::endl;
                }
                if (tile_inside) {
                        minmax_elev_t mm(get_tile_minmax_elev(tc));
                        ret.first = std::min(ret.first, mm.first);
                        ret.second = std::max(ret.second, mm.second);
                } else if (poly_inside || intersect) {
                        for (pixel_index_t y(ymin); y <= ymax; y++) {
                                tc.set_lat_offs(y);
#if 1
				Point pt2(pdim2 + (Point)(TopoCoordinate)tc);
				PolygonSimple::ScanLine sl(p.scanline(pt2.get_lat()));
				PolygonSimple::ScanLine::const_iterator slb(sl.begin()), sle(sl.end());
				if (slb == sle)
					continue;
				if (false) {
					std::cerr << "get_minmax_elev: scanline " << pt2.get_lat_str2() << ':';
					for (PolygonSimple::ScanLine::const_iterator sli(slb); sli != sle; ++sli)
						std::cerr << ' ' << Point(sli->first, pt2.get_lat()).get_lon_str2() << '@' << sli->second;
					std::cerr << std::endl;
				}
				PolygonSimple::ScanLine::const_iterator sli(slb);
                                for (pixel_index_t x(xmin); x <= xmax; x++) {
                                        tc.set_lon_offs(x);
                                        Point pt2(pdim2 + (Point)(TopoCoordinate)tc);
					if (!r.is_inside(pt2))
						continue;
					while (sli != slb && sli->first > pt2.get_lon())
						--sli;
					if (sli->first > pt2.get_lon())
						continue;
					PolygonSimple::ScanLine::const_iterator sli1(sli);
					++sli1;
					while (sli1 != sle && sli1->first <= pt2.get_lon()) {
						++sli;
						++sli1;
					}
					if (!sli->second)
						continue;
					elev_t e(get_elev(tc));
					if (false)
						std::cerr << "get_minmax_elev: " << e << " @ " << pt2 << std::endl;
					if (e == ocean)
						e = 0;
					if (e != nodata) {
						ret.first = std::min(ret.first, e);
						ret.second = std::max(ret.second, e);
					}
                                }
#else
                                for (pixel_index_t x(xmin); x <= xmax; x++) {
                                        tc.set_lon_offs(x);
                                        Point pt2(pdim2 + (Point)(TopoCoordinate)tc);
                                        if (!r.is_inside(pt2) || !p.windingnumber(pt2))
						continue;
					elev_t e(get_elev(tc));
					if (false)
						std::cerr << "get_minmax_elev: " << e << " @ " << pt2 << std::endl;
					if (e == ocean)
						e = 0;
					if (e != nodata) {
						ret.first = std::min(ret.first, e);
						ret.second = std::max(ret.second, e);
					}
				}
#endif
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
        if (ret.first == std::numeric_limits<elev_t>::max())
                ret.first = nodata;
        if (ret.second == std::numeric_limits<elev_t>::min())
                ret.second = nodata;
        return ret;
}

template<int resolution, int tilesize>
typename TopoDbN<resolution,tilesize>::minmax_elev_t TopoDbN<resolution,tilesize>::get_minmax_elev(const PolygonHole& p)
{
        if (p.get_exterior().empty())
                return minmax_elev_t(nodata, nodata);
        Point pdim(TopoCoordinate::get_pointsize()), pdim2(pdim.get_lon()/2, pdim.get_lat()/2);
        Rect r(p.get_bbox());
        TopoTileCoordinate tmin(r.get_southwest()), tmax(r.get_northeast());
        minmax_elev_t ret(std::numeric_limits<elev_t>::max(), std::numeric_limits<elev_t>::min());
        for (TopoTileCoordinate tc(tmin);;) {
                pixel_index_t ymin((tc.get_lat_tile() == tmin.get_lat_tile()) ? tmin.get_lat_offs() : 0);
                pixel_index_t ymax((tc.get_lat_tile() == tmax.get_lat_tile()) ? tmax.get_lat_offs() : tile_size - 1);
                pixel_index_t xmin((tc.get_lon_tile() == tmin.get_lon_tile()) ? tmin.get_lon_offs() : 0);
                pixel_index_t xmax((tc.get_lon_tile() == tmax.get_lon_tile()) ? tmax.get_lon_offs() : tile_size - 1);
                Rect rtile((Point)(TopoCoordinate)TopoTileCoordinate(tc.get_lon_tile(), tc.get_lat_tile(), xmin, ymin),
                           (Point)(TopoCoordinate)TopoTileCoordinate(tc.get_lon_tile(), tc.get_lat_tile(), xmax, ymax) + pdim);
                bool intersect(p.is_intersection(rtile));
                bool poly_inside(!intersect && rtile.is_inside(p.get_exterior()[0]));
                bool tile_inside(!intersect && !poly_inside && p.windingnumber(rtile.get_southwest()));
                if (false) {
                        minmax_elev_t mm(get_tile_minmax_elev(tc));
                        std::cerr << "get_minmax_elev: bbox " << r << " tile " << rtile << " intersect " << (intersect ? "yes" : "no")
                                  << " poly_inside " << (poly_inside ? "yes" : "no") << " tile_inside " << (tile_inside ? "yes" : "no")
                                  << " tile elev " << mm.first << " / " << mm.second << std::endl;
                }
                if (tile_inside) {
                        minmax_elev_t mm(get_tile_minmax_elev(tc));
                        ret.first = std::min(ret.first, mm.first);
                        ret.second = std::max(ret.second, mm.second);
                } else if (poly_inside || intersect) {
                        for (pixel_index_t y(ymin); y <= ymax; y++) {
                                tc.set_lat_offs(y);
#if 1
				Point pt2(pdim2 + (Point)(TopoCoordinate)tc);
				PolygonSimple::ScanLine sl(p.scanline(pt2.get_lat()));
				PolygonSimple::ScanLine::const_iterator sli(sl.begin()), sle(sl.end());
				int wn(0);
                                for (pixel_index_t x(xmin); x <= xmax; x++) {
                                        tc.set_lon_offs(x);
                                        Point pt2 = pdim2 + (Point)(TopoCoordinate)tc;
                                        if (!r.is_inside(pt2))
						continue;
					while (sli != sle && sli->first <= pt2.get_lat()) {
						wn = sli->second;
						++sli;
					}
					if (!wn)
						continue;
					elev_t e(get_elev(tc));
					if (false)
						std::cerr << "get_minmax_elev: " << e << " @ " << pt2 << std::endl;
					if (e == ocean)
						e = 0;
					if (e != nodata) {
						ret.first = std::min(ret.first, e);
						ret.second = std::max(ret.second, e);
					}
                                }
#else
                                for (pixel_index_t x(xmin); x <= xmax; x++) {
                                        tc.set_lon_offs(x);
                                        Point pt2(pdim2 + (Point)(TopoCoordinate)tc);
                                        if (!r.is_inside(pt2) || !p.windingnumber(pt2))
						continue;
					elev_t e(get_elev(tc));
					if (false)
						std::cerr << "get_minmax_elev: " << e << " @ " << pt2 << std::endl;
					if (e == ocean)
						e = 0;
					if (e != nodata) {
						ret.first = std::min(ret.first, e);
						ret.second = std::max(ret.second, e);
					}
                                }
#endif
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
        if (ret.first == std::numeric_limits<elev_t>::max())
                ret.first = nodata;
        if (ret.second == std::numeric_limits<elev_t>::min())
                ret.second = nodata;
        return ret;
}

template<int resolution, int tilesize>
typename TopoDbN<resolution,tilesize>::minmax_elev_t TopoDbN<resolution,tilesize>::get_minmax_elev(const MultiPolygonHole& p)
{
	minmax_elev_t r(nodata, nodata);
	for (MultiPolygonHole::const_iterator pi(p.begin()), pe(p.end()); pi != pe; ++pi) {
		minmax_elev_t m(get_minmax_elev(*pi));
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

template<int resolution, int tilesize>
typename TopoDbN<resolution,tilesize>::ProfilePoint TopoDbN<resolution,tilesize>::get_profile(const Point& pt, double corridor_nmi)
{
	if (pt.is_invalid() || corridor_nmi > 10)
		return ProfilePoint();
	return ProfilePoint(0, get_elev(pt), get_minmax_elev(pt.simple_box_nmi(corridor_nmi)));
}

template<int resolution, int tilesize>
typename TopoDbN<resolution,tilesize>::Profile TopoDbN<resolution,tilesize>::get_profile(const Point& p0, const Point& p1, double corridor_nmi)
{
	if (p0.is_invalid() || p1.is_invalid() || corridor_nmi < 0 || corridor_nmi > 10)
		return Profile();
	double gcdist(p0.spheric_distance_nmi_dbl(p1));
	if (gcdist > 2000)
		return Profile();
	Profile result;
	TopoCoordinate l0(p0), l1(p1), c0, c1;
	// corridor line coordinates
	{
		double crs(p0.spheric_true_course_dbl(p1));
		Point px0(p0.spheric_course_distance_nmi(crs + 90, corridor_nmi));
		Point px1(p0 + (p0 - px0));
		if (px0.is_invalid() || px1.is_invalid())
			return Profile();
		c0 = TopoCoordinate(px0);
		c1 = TopoCoordinate(px1);
	}
	// line step distance map
	double distmap[4];
	distmap[0] = 0;
	{
		Point ptsz(TopoCoordinate::get_pointsize());
		//Point p(p0.halfway(p1));
		Point p(p0.get_gcnav(p1, 0.5).first);
		for (unsigned int i = 1; i < 4; ++i) {
			Point p1((i & 1) ? ptsz.get_lon() : 0, (i & 2) ? ptsz.get_lat() : 0);
			p1 += p;
			distmap[i] = p.spheric_distance_nmi_dbl(p1);
		}
	}
	typename TopoCoordinate::coords_t ldx(l0.lon_dist_signed(l1)), ldy(l0.lat_dist_signed(l1));
	bool lsx(ldx < 0), lsy(ldy < 0);
	ldx = abs(ldx);
	ldy = abs(ldy);
	typename TopoCoordinate::coords_t cdx(c0.lon_dist_signed(c1)), cdy(c0.lat_dist_signed(c1));
	bool csx(cdx < 0), csy(cdy < 0);
	cdx = abs(cdx);
	cdy = abs(cdy);
	double dist(0);
	typename TopoCoordinate::coords_t lerr(ldx - ldy);
	TopoCoordinate ll(l0);
	for (;;) {
		result.push_back(ProfilePoint(dist, get_elev(ll)));
		typename TopoCoordinate::coords_t cerr(cdx - cdy);
		TopoCoordinate cc(c0);
		for (;;) {
			result.back().add(get_elev(cc));
			if (cc == c1)
				break;
			typename TopoCoordinate::coords_t cerr2(2 * cerr);
			if (cerr2 > -cdy) {
				cerr -= cdy;
				if (csx)
					cc.advance_west();
				else
					cc.advance_east();
			}
			if (cerr2 < cdx) {
				cerr += cdx;
				if (csy)
					cc.advance_south();
				else
					cc.advance_north();
			}
		}
		if (ll == l1)
			break;
		typename TopoCoordinate::coords_t lerr2(2 * lerr);
		unsigned int dmi(0);
		if (lerr2 > -ldy) {
			lerr -= ldy;
			if (lsx) {
				ll.advance_west();
				c0.advance_west();
				c1.advance_west();
			} else {
				ll.advance_east();
				c0.advance_east();
				c1.advance_east();
			}
			dmi |= 1U;
		}
		if (lerr2 < ldx) {
			lerr += ldx;
			if (lsy) {
				ll.advance_south();
				c0.advance_south();
				c1.advance_south();
			} else {
				ll.advance_north();
				c0.advance_north();
				c1.advance_north();
			}
			dmi |= 2U;
		}
		dist += distmap[dmi];
	}
	if (dist > 0)
		result.scaledist(gcdist / dist);
	return result;
}

template<int resolution, int tilesize>
typename TopoDbN<resolution,tilesize>::RouteProfile TopoDbN<resolution,tilesize>::get_profile(const FPlanRoute& fpl, double corridor_nmi)
{
	const unsigned int nrwpt(fpl.get_nrwpt());
	if (nrwpt < 2)
		return RouteProfile();
	RouteProfile rp;
	double cumdist(0);
	for (unsigned int i(0); i < nrwpt; ) {
		if (fpl[i].get_coord().is_invalid()) {
			++i;
			continue;
		}
	        unsigned int j(i);
		while (i + 1U < nrwpt) {
			++i;
			if (!fpl[i].get_coord().is_invalid())
				break;
		}
		if (i == j || fpl[i].get_coord().is_invalid())
			break;
		const FPlanWaypoint& wpt0(fpl[j]), wpt1(fpl[i]);
		if (wpt0.get_coord().is_invalid() || wpt1.get_coord().is_invalid())
			return RouteProfile();
		Profile p(get_profile(wpt0.get_coord(), wpt1.get_coord(), corridor_nmi));
		typename Profile::const_iterator pi(p.begin()), pe(p.end());
		double legdist(wpt0.get_coord().spheric_distance_nmi_dbl(wpt1.get_coord()));
		for (; pi != pe && pi->get_dist() < legdist; ++pi)
			rp.push_back(RouteProfilePoint(*pi, j, cumdist));
		cumdist += legdist;
		if (i + 1U < nrwpt)
			continue;
		for (; pi != pe && pi->get_dist() >= legdist; ++pi)
			rp.push_back(RouteProfilePoint(ProfilePoint(pi->get_dist() - legdist, pi->get_elev(), pi->get_minelev(), pi->get_maxelev()), i, cumdist));
		if (!p.empty() && !rp.empty())
			rp.back().set_routeindex(i);
		break;
	}
	return rp;
}

template<int resolution, int tilesize>
std::ostream & operator <<(std::ostream & os, const typename TopoDbN< resolution, tilesize >::TopoCoordinate & tc)
{
        return tc.print(os);
}

void TopoDb30::open(const Glib::ustring & path)
{
        TopoDbNN::open(path, "topo30.db");
}

void TopoDb30::open_readonly(const Glib::ustring & path, bool enablebinfile)
{
        TopoDbNN::open_readonly(path, "topo30.db", enablebinfile ? "topo30.bin" : "");
}

template<int resolution, int tilesize>
TopoDbN<resolution, tilesize>::RGBColor::operator Gdk::Color(void) const
{
        Gdk::Color col;
        col.set_rgb((m_col >> 8) & 0xff00, m_col & 0xff00, (m_col << 8) & 0xff00);
        return col;
}

template<int resolution, int tilesize>
typename TopoDbN<resolution, tilesize>::RGBColor TopoDbN<resolution, tilesize>::color(elev_t x)
{
#include "gtopocolor.h"
        return RGBColor(0, 0, 255);
}

template<int resolution, int tilesize>
int TopoDbN<resolution, tilesize>::m_to_ft(elev_t elev)
{
        return (elev << 2) - elev + (elev >> 2) + (elev >> 5) - (elev >> 11);
}

template class TopoDbN<30,480>;
//const TopoDb30::elev_t TopoDb30::nodata = TopoDbNN::nodata;
