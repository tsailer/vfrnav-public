//
// C++ Implementation: dbobjtrk
//
// Description: Database Objects: Tracks
//
//
// Author: Thomas Sailer <t.sailer@alumni.ethz.ch>, (C) 2007, 2009, 2012, 2016
//
// Copyright: See COPYING file that comes with this distribution
//
//

#include "sysdeps.h"

#include <limits>
#include <zlib.h>

#include "dbobj.h"

#ifdef HAVE_PQXX
#include <pqxx/transactor.hxx>
#include <pqxx/result.hxx>
#include <pqxx/except.hxx>
#endif

const DbBaseElements::Track::TrackPoint::alt_t DbBaseElements::Track::TrackPoint::altinvalid = std::numeric_limits<alt_t>::min();

const char *DbBaseElements::Track::db_query_string = "tracks.ID AS ID,0 AS \"TABLE\",tracks.SRCID AS SRCID,tracks.FROMICAO,tracks.FROMNAME,tracks.TOICAO,tracks.TONAME,tracks.NOTES,tracks.OFFBLOCKTIME,tracks.TAKEOFFTIME,tracks.LANDINGTIME,tracks.ONBLOCKTIME,tracks.SWLAT,tracks.SWLON,tracks.NELAT,tracks.NELON,tracks.POINTS,tracks.MODIFIED";
const char *DbBaseElements::Track::db_aux_query_string = "aux.tracks.ID AS ID,1 AS \"TABLE\",aux.tracks.SRCID AS SRCID,aux.tracks.FROMICAO,aux.tracks.FROMNAME,aux.tracks.TOICAO,aux.tracks.TONAME,aux.tracks.NOTES,aux.tracks.OFFBLOCKTIME,aux.tracks.TAKEOFFTIME,aux.tracks.LANDINGTIME,aux.tracks.ONBLOCKTIME,aux.tracks.SWLAT,aux.tracks.SWLON,aux.tracks.NELAT,aux.tracks.NELON,aux.tracks.POINTS,aux.tracks.MODIFIED";
const char *DbBaseElements::Track::db_text_fields[] = { "SRCID", "FROMICAO", "FROMNAME", "TOICAO", "TONAME", 0 };
const char *DbBaseElements::Track::db_time_fields[] = { "MODIFIED", "OFFBLOCKTIME", "TAKEOFFTIME", "LANDINGTIME", "ONBLOCKTIME", 0 };

DbBaseElements::Track::TrackPoint::TrackPoint(void)
	: m_time(0), m_alt(altinvalid)
{
	m_coord.set_invalid();
}

Glib::TimeVal DbBaseElements::Track::TrackPoint::get_timeval(void) const
{
	Glib::TimeVal tv;
	tv.tv_sec = m_time >> 8;
	tv.tv_usec = ((m_time & 0xff) * 1000000) >> 8;
	return tv;
}

void DbBaseElements::Track::TrackPoint::set_timeval(const Glib::TimeVal& tv)
{
	m_time = tv.tv_sec;
	m_time <<= 8;
	m_time += (tv.tv_usec << 8) / 1000000;
}

void DbBaseElements::Track::TrackPoint::read_blob(const uint8_t *data)
{
	if (!data) {
		m_coord.set_invalid();
		m_time = 0;
		m_alt = altinvalid;
		return;
	}
	m_time = data[0] | (data[1] << 8) | (data[2] << 16) | (data[3] << 24);
	m_time <<= 8;
	m_time |= data[14];
	m_coord = Point(data[4] | (data[5] << 8) | (data[6] << 16) | (data[7] << 24),
			data[8] | (data[9] << 8) | (data[10] << 16) | (data[11] << 24));
	m_alt = data[12] | (data[13] << 8);
}

void DbBaseElements::Track::TrackPoint::write_blob(uint8_t *data) const
{
	if (!data)
		return;
	data[14] = m_time;
	data[0] = m_time >> 8;
	data[1] = m_time >> 16;
	data[2] = m_time >> 24;
	data[3] = m_time >> 32;
	data[4] = m_coord.get_lon();
	data[5] = m_coord.get_lon() >> 8;
	data[6] = m_coord.get_lon() >> 16;
	data[7] = m_coord.get_lon() >> 24;
	data[8] = m_coord.get_lat();
	data[9] = m_coord.get_lat() >> 8;
	data[10] = m_coord.get_lat() >> 16;
	data[11] = m_coord.get_lat() >> 24;
	data[12] = m_alt;
	data[13] = m_alt >> 8;
	data[15] = 0;
}

DbBaseElements::Track::Track(void)
	: m_fromicao(), m_fromname(), m_toicao(), m_toname(), m_notes(),
	  m_offblocktime(0), m_takeofftime(0), m_landingtime(0), m_onblocktime(0), m_bbox(), m_trackpoints(), m_savedtrackpoints(0),
	  m_format(format_invalid)
{
}

void DbBaseElements::Track::load(sqlite3x::sqlite3_cursor& cursor, sqlite3x::sqlite3_connection& db, unsigned int loadsubtables)
{
	m_id = 0;
	m_table = table_main;
	m_sourceid.clear();
	m_fromicao.clear();
	m_fromname.clear();
	m_toicao.clear();
	m_toname.clear();
	m_notes.clear();
	m_offblocktime = 0;
	m_takeofftime = 0;
	m_landingtime = 0;
	m_onblocktime = 0;
	m_bbox = Rect();
	m_trackpoints.clear();
	m_savedtrackpoints = 0;
	m_format = format_invalid;
	m_modtime = 0;
	{
		if (!cursor.step())
			return;
		m_format = format_empty;
		m_id = cursor.getint(0);
		m_table = (table_t)cursor.getint(1);
		m_sourceid = cursor.getstring(2);
		m_fromicao = cursor.getstring(3);
		m_fromname = cursor.getstring(4);
		m_toicao = cursor.getstring(5);
		m_toname = cursor.getstring(6);
		m_notes = cursor.getstring(7);
		m_offblocktime = cursor.getint(8);
		m_takeofftime = cursor.getint(9);
		m_landingtime = cursor.getint(10);
		m_onblocktime = cursor.getint(11);
		m_bbox = Rect(Point(cursor.getint(13), cursor.getint(12)), Point(cursor.getint(15), cursor.getint(14)));
		if (!cursor.isnull(16)) {
			int blobsize(0);
			void const *blob(cursor.getblob(16, blobsize));
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
			uint8_t data[256];
			d_stream.next_out = data;
			d_stream.avail_out = sizeof(data);
			for (;;) {
				err = inflate(&d_stream, Z_SYNC_FLUSH);
				if (err != Z_OK && err != Z_STREAM_END) {
					std::cerr << "zlib: inflate error " << err << std::endl;
					return;
				}
				uint8_t *data2;
				for (data2 = data; data2 + TrackPoint::blob_size_v1 <= d_stream.next_out; data2 += TrackPoint::blob_size_v1) {
					m_trackpoints.push_back(TrackPoint());
					m_trackpoints.back().read_blob(data2);
				}
				if (err == Z_STREAM_END)
					break;
				if (data2 < d_stream.next_out) {
					unsigned int len(d_stream.next_out - data2);
					memmove(data, data2, len);
					d_stream.next_out = data + len;
					d_stream.avail_out = sizeof(data) - len;
					continue;
				}
				d_stream.next_out = data;
				d_stream.avail_out = sizeof(data);
			}
			inflateEnd(&d_stream);
			m_format = format_gzip;
		}
		m_modtime = cursor.getint(17);
	}
	std::string tablepfx;
	switch (m_table) {
		case 0:
			break;
		case 1:
			tablepfx = "aux.";
			break;
		default:
			throw std::runtime_error("invalid table ID");
	}
	{
		sqlite3x::sqlite3_command cmd(db, "SELECT POINTS FROM " + tablepfx + "tracks_appendlog WHERE TRACKID=? ORDER BY ID;");
		cmd.bind(1, (long long int)m_id);
		sqlite3x::sqlite3_cursor cursor(cmd.executecursor());
		while (cursor.step()) {
			int blobsize(0);
			uint8_t const *blob((uint8_t const *)cursor.getblob(1, blobsize));
			uint8_t const *data = blob;
			for (; data + TrackPoint::blob_size_v1 <= blob + blobsize; data += TrackPoint::blob_size_v1) {
				m_trackpoints.push_back(TrackPoint());
				m_trackpoints.back().read_blob(data);
			}
		}
	}
	m_savedtrackpoints = m_trackpoints.size();
	if (!loadsubtables)
		return;
	load_subtables(db);
}

void DbBaseElements::Track::save(sqlite3x::sqlite3_connection& db, bool rtree, bool aux_rtree)
{
	recompute_bbox();
	sqlite3x::sqlite3_transaction tran(db);
	if (m_table == table_aux) {
		sqlite3x::sqlite3_command cmd(db, "INSERT OR REPLACE INTO tracks_deleted (SRCID) VALUES (?);");
		cmd.bind(1, m_sourceid);
		cmd.executenonquery();
		m_table = table_main;
		m_id = 0;
	}
	if (!m_id) {
		m_id = 1;
		{
			sqlite3x::sqlite3_command cmd(db, "SELECT MAX(ID) FROM tracks;");
			sqlite3x::sqlite3_cursor cursor = cmd.executecursor();
			if (cursor.step())
				m_id = cursor.getint(0) + 1;
		}
		{
			sqlite3x::sqlite3_command cmd(db, "INSERT INTO tracks (ID,SRCID) VALUES(?,?);");
			cmd.bind(1, (long long int)m_id);
			cmd.bind(2, m_sourceid);
			cmd.executenonquery();
		}
		if (rtree) {
			sqlite3x::sqlite3_command cmd(db, "INSERT INTO tracks_rtree (ID,SWLAT,NELAT,SWLON,NELON) VALUES(?,?,?,?,?);");
			cmd.bind(1, (long long int)m_id);
			cmd.bind(2, m_bbox.get_south());
			cmd.bind(3, m_bbox.get_north());
			cmd.bind(4, m_bbox.get_west());
			cmd.bind(5, (long long int)m_bbox.get_east_unwrapped());
			cmd.executenonquery();
		}
	}
	// Update Track Record
	{
		sqlite3x::sqlite3_command cmd(db, "UPDATE tracks SET SRCID=?,FROMICAO=?,FROMNAME=?,TOICAO=?,TONAME=?,NOTES=?,OFFBLOCKTIME=?,TAKEOFFTIME=?,LANDINGTIME=?,ONBLOCKTIME=?,SWLAT=?,SWLON=?,NELAT=?,NELON=?,POINTS=?,MODIFIED=?,TILE=? WHERE ID=?;");
		cmd.bind(1, m_sourceid);
		cmd.bind(2, m_fromicao);
		cmd.bind(3, m_fromname);
		cmd.bind(4, m_toicao);
		cmd.bind(5, m_toname);
		cmd.bind(6, m_notes);
		cmd.bind(7, (long long int)m_offblocktime);
		cmd.bind(8, (long long int)m_takeofftime);
		cmd.bind(9, (long long int)m_landingtime);
		cmd.bind(10, (long long int)m_onblocktime);
		cmd.bind(11, m_bbox.get_south());
		cmd.bind(12, m_bbox.get_west());
		cmd.bind(13, m_bbox.get_north());
		cmd.bind(14, (long long int)m_bbox.get_east_unwrapped());
		cmd.bind(15);
		if (!m_trackpoints.empty()) {
			uint8_t data2[m_trackpoints.size() * TrackPoint::blob_size_v1];
			uint8_t *data3 = data2;
			for (trackpoints_t::const_iterator ti(m_trackpoints.begin()), te(m_trackpoints.end()); ti != te; ti++, data3 += TrackPoint::blob_size_v1)
				ti->write_blob(data3);
			char data[m_trackpoints.size() * TrackPoint::blob_size_v1 + 64];
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
			c_stream.next_out = (Bytef *)data;
			c_stream.avail_out = sizeof(data);
			c_stream.next_in = (Bytef *)data2;
			c_stream.avail_in = sizeof(data2);
			err = deflate(&c_stream, Z_FINISH);
			if (err != Z_STREAM_END) {
				std::cerr << "deflate error " << err << ", " << c_stream.msg << std::endl;
				goto outerr2;
			}
			if (c_stream.avail_in != 0 || c_stream.total_in != sizeof(data2)) {
				std::cerr << "deflate did not consume all input" << std::endl;
				goto outerr2;
			}
			cmd.bind(15, data, c_stream.total_out);
			std::cerr << "Track compression: input: " << (sizeof(data2)) << " output: " << c_stream.total_out << std::endl;
			err = deflateEnd(&c_stream);
			if (err) {
				std::cerr << "deflateEnd error " << err << ", " << c_stream.msg << std::endl;
				goto outerr2;
			}
			outerr2:;

		}
		cmd.bind(16, (long long int)m_modtime);
		cmd.bind(17, (int)TileNumber::to_tilenumber(m_bbox));
		cmd.bind(18, (long long int)m_id);
		cmd.executenonquery();
	}
	{
		sqlite3x::sqlite3_command cmd(db, "DELETE FROM tracks_appendlog WHERE TRACKID=?;");
		cmd.bind(1, (long long int)m_id);
		cmd.executenonquery();
	}
	if (rtree) {
		sqlite3x::sqlite3_command cmd(db, "UPDATE tracks_rtree SET SWLAT=?,NELAT=?,SWLON=?,NELON=? WHERE ID=?;");
		cmd.bind(1, m_bbox.get_south());
		cmd.bind(2, m_bbox.get_north());
		cmd.bind(3, m_bbox.get_west());
		cmd.bind(4, (long long int)m_bbox.get_east_unwrapped());
		cmd.bind(5, (long long int)m_id);
		cmd.executenonquery();
	}
	tran.commit();
	m_savedtrackpoints = m_trackpoints.size();
}

void DbBaseElements::Track::update_index(sqlite3x::sqlite3_connection& db, bool rtree, bool aux_rtree)
{
	if (m_table != table_main || !m_id)
		return;
	if (false)
		std::cerr << "Start transaction" << std::endl;
	sqlite3x::sqlite3_transaction tran(db);
	{
		sqlite3x::sqlite3_command cmd(db, "UPDATE tracks SET TILE=?2 WHERE ID=?1;");
		cmd.bind(1, (long long int)m_id);
		cmd.bind(2, (int)TileNumber::to_tilenumber(m_bbox));
		cmd.executenonquery();
	}
	if (rtree) {
		sqlite3x::sqlite3_command cmd(db, "UPDATE tracks_rtree SET SWLAT=?,NELAT=?,SWLON=?,NELON=? WHERE ID=?;");
		cmd.bind(1, m_bbox.get_south());
		cmd.bind(2, m_bbox.get_north());
		cmd.bind(3, m_bbox.get_west());
		cmd.bind(4, (long long int)m_bbox.get_east_unwrapped());
		cmd.bind(5, (long long int)m_id);
		cmd.executenonquery();
	}
	if (false)
		std::cerr << "Commit transaction" << std::endl;
	tran.commit();
}

void DbBaseElements::Track::erase(sqlite3x::sqlite3_connection& db, bool rtree, bool aux_rtree)
{
	if (!is_valid())
		return;
	sqlite3x::sqlite3_transaction tran(db);
	if (m_table == table_aux) {
		sqlite3x::sqlite3_command cmd(db, "INSERT OR REPLACE INTO tracks_deleted (SRCID) VALUES (?);");
		cmd.bind(1, m_sourceid);
		cmd.executenonquery();
		m_table = table_main;
	} else {
		{
			sqlite3x::sqlite3_command cmd(db, "DELETE FROM tracks WHERE ID=?;");
			cmd.bind(1, (long long int)m_id);
			cmd.executenonquery();
		}
		{
			sqlite3x::sqlite3_command cmd(db, "DELETE FROM tracks_appendlog WHERE TRACKID=?;");
			cmd.bind(1, (long long int)m_id);
			cmd.executenonquery();
		}
		if (rtree) {
			sqlite3x::sqlite3_command cmd(db, "DELETE FROM tracks_rtree WHERE ID=?;");
			cmd.bind(1, (long long int)m_id);
			cmd.executenonquery();
		}
	}
	tran.commit();
	m_id = 0;
}

#ifdef HAVE_PQXX

void DbBaseElements::Track::load(const pqxx::tuple& cursor, pqxx::basic_transaction& w, unsigned int loadsubtables)
{
	m_id = 0;
	m_table = table_main;
	m_sourceid.clear();
	m_fromicao.clear();
	m_fromname.clear();
	m_toicao.clear();
	m_toname.clear();
	m_notes.clear();
	m_offblocktime = 0;
	m_takeofftime = 0;
	m_landingtime = 0;
	m_onblocktime = 0;
	m_bbox = Rect();
	m_trackpoints.clear();
	m_savedtrackpoints = 0;
	m_format = format_invalid;
	m_modtime = 0;
	if (cursor.empty())
		return;
	m_format = format_empty;
	m_id = cursor[0].as<int64_t>(0);
	m_table = (table_t)cursor[1].as<int>(0);
	m_sourceid = cursor[2].as<std::string>("");
	m_fromicao = cursor[3].as<std::string>("");
	m_fromname = cursor[4].as<std::string>("");
	m_toicao = cursor[5].as<std::string>("");
	m_toname = cursor[6].as<std::string>("");
	m_notes = cursor[7].as<std::string>("");
	m_offblocktime = cursor[8].as<time_t>(0);
	m_takeofftime = cursor[9].as<time_t>(0);
	m_landingtime = cursor[10].as<time_t>(0);
	m_onblocktime = cursor[11].as<time_t>(0);
	m_bbox = Rect(Point(cursor[13].as<Point::coord_t>(0), cursor[12].as<Point::coord_t>(0x80000000)),
		      Point(cursor[15].as<Point::coord_t>(0), cursor[14].as<Point::coord_t>(0x80000000)));
	{
		pqxx::binarystring blob(cursor[16]);
		if (blob.size()) {
			z_stream d_stream;
			memset(&d_stream, 0, sizeof(d_stream));
			d_stream.zalloc = (alloc_func)0;
			d_stream.zfree = (free_func)0;
			d_stream.opaque = (voidpf)0;
			d_stream.next_in  = (Bytef *)blob.data();
			d_stream.avail_in = blob.size();
			int err = inflateInit(&d_stream);
			if (err != Z_OK) {
				inflateEnd(&d_stream);
				std::cerr << "zlib: inflateInit error " << err << std::endl;
				return;
			}
			uint8_t data[256];
			d_stream.next_out = data;
			d_stream.avail_out = sizeof(data);
			for (;;) {
				err = inflate(&d_stream, Z_SYNC_FLUSH);
				if (err != Z_OK && err != Z_STREAM_END) {
					std::cerr << "zlib: inflate error " << err << std::endl;
					return;
				}
				uint8_t *data2;
				for (data2 = data; data2 + TrackPoint::blob_size_v1 <= d_stream.next_out; data2 += TrackPoint::blob_size_v1) {
					m_trackpoints.push_back(TrackPoint());
					m_trackpoints.back().read_blob(data2);
				}
				if (err == Z_STREAM_END)
					break;
				if (data2 < d_stream.next_out) {
					unsigned int len(d_stream.next_out - data2);
					memmove(data, data2, len);
					d_stream.next_out = data + len;
					d_stream.avail_out = sizeof(data) - len;
					continue;
				}
				d_stream.next_out = data;
				d_stream.avail_out = sizeof(data);
			}
			inflateEnd(&d_stream);
			m_format = format_gzip;
		}
	}
        m_modtime = cursor[17].as<time_t>(0);
	{
		pqxx::result r(w.exec("SELECT POINTS FROM aviationdb.tracks_appendlog WHERE TRACKID=" + w.quote(m_id) + " ORDER BY ID;"));
		for (pqxx::result::const_iterator ri(r.begin()), re(r.end()); ri != re; ++ri) {
			pqxx::binarystring blob((*ri)[0]);
			uint8_t const *data(blob.data());
			uint8_t const *end(data + blob.size());
			for (; data + TrackPoint::blob_size_v1 <= end; data += TrackPoint::blob_size_v1) {
				m_trackpoints.push_back(TrackPoint());
				m_trackpoints.back().read_blob(data);
			}
		}
	}
	m_savedtrackpoints = m_trackpoints.size();
	if (!loadsubtables)
		return;
	load_subtables(w, loadsubtables);
}

void DbBaseElements::Track::save(pqxx::connection& conn, bool rtree)
{
	recompute_bbox();
	pqxx::work w(conn);
	if (!m_id) {
		pqxx::result r(w.exec("INSERT INTO aviationdb.tracks (ID) SELECT COALESCE(MAX(ID),0)+1 FROM aviationdb.tracks RETURNING ID;"));
		m_id = r.front()[0].as<int64_t>();
		if (rtree)
			w.exec("INSERT INTO aviationdb.tracks_rtree (ID,SWLAT,NELAT,SWLON,NELON) VALUES(" + w.quote(m_id) + "," + w.quote(m_bbox.get_south()) +
			       "," + w.quote(m_bbox.get_north()) + "," + w.quote(m_bbox.get_west()) + "," + w.quote(m_bbox.get_east_unwrapped()) + ");");
	}
	// Update Track Record
	{
		std::string trkpt("NULL");
		if (!m_trackpoints.empty()) {
			uint8_t data2[m_trackpoints.size() * TrackPoint::blob_size_v1];
			uint8_t *data3 = data2;
			for (trackpoints_t::const_iterator ti(m_trackpoints.begin()), te(m_trackpoints.end()); ti != te; ++ti, data3 += TrackPoint::blob_size_v1)
				ti->write_blob(data3);
			char data[m_trackpoints.size() * TrackPoint::blob_size_v1 + 64];
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
			c_stream.next_out = (Bytef *)data;
			c_stream.avail_out = sizeof(data);
			c_stream.next_in = (Bytef *)data2;
			c_stream.avail_in = sizeof(data2);
			err = deflate(&c_stream, Z_FINISH);
			if (err != Z_STREAM_END) {
				std::cerr << "deflate error " << err << ", " << c_stream.msg << std::endl;
				goto outerr2;
			}
			if (c_stream.avail_in != 0 || c_stream.total_in != sizeof(data2)) {
				std::cerr << "deflate did not consume all input" << std::endl;
				goto outerr2;
			}
			{
				pqxx::binarystring blob(data, c_stream.total_out);
				trkpt = w.quote(blob);
			}
			std::cerr << "Track compression: input: " << (sizeof(data2)) << " output: " << c_stream.total_out << std::endl;
			err = deflateEnd(&c_stream);
			if (err) {
				std::cerr << "deflateEnd error " << err << ", " << c_stream.msg << std::endl;
				goto outerr2;
			}
			outerr2:;
		}
		LineString ls;
		for (trackpoints_t::const_iterator ti(m_trackpoints.begin()), te(m_trackpoints.end()); ti != te; ++ti)
			ls.push_back(ti->get_coord());
       		w.exec("UPDATE aviationdb.tracks SET SRCID=" + w.quote((std::string)m_sourceid) +
		       ",FROMICAO=" + w.quote((std::string)m_fromicao) + ",FROMNAME=" + w.quote((std::string)m_fromname) +
		       ",TOICAO=" + w.quote((std::string)m_toicao) + ",TONAME=" + w.quote((std::string)m_toname) +
		       ",NOTES=" + w.quote((std::string)m_notes) +
		       ",OFFBLOCKTIME=" + w.quote(m_offblocktime) + ",TAKEOFFTIME=" + w.quote(m_takeofftime) +
		       ",LANDINGTIME=" + w.quote(m_landingtime) + ",ONBLOCKTIME=" + w.quote(m_onblocktime) +
		       ",SWLAT=" + w.quote(m_bbox.get_south()) + ",SWLON=" + w.quote(m_bbox.get_west()) +
		       ",NELAT=" + w.quote(m_bbox.get_north()) + ",NELON=" + w.quote((long long int)m_bbox.get_east_unwrapped()) +
		       ",POINTS=" + trkpt + ",GPOLY=" + PGDbBaseCommon::get_str(ls) + ",MODIFIED=" + w.quote(m_modtime) +
		       ",TILE=" + w.quote(TileNumber::to_tilenumber(m_bbox)) + " WHERE ID=" + w.quote(m_id) + ";");
	}
	w.exec("DELETE FROM aviationdb.tracks_appendlog WHERE TRACKID=" + w.quote(m_id) + ";");
	if (rtree)
		w.exec("UPDATE aviationdb.tracks_rtree SET SWLAT=" + w.quote(m_bbox.get_south()) + ",NELAT=" + w.quote(m_bbox.get_north()) +
		       ",SWLON=" + w.quote(m_bbox.get_west()) + ",NELON=" + w.quote((long long int)m_bbox.get_east_unwrapped()) + " WHERE ID=" + w.quote(m_id) + ";");
	w.commit();
	m_savedtrackpoints = m_trackpoints.size();
}

void DbBaseElements::Track::erase(pqxx::connection& conn, bool rtree)
{
	if (!is_valid())
		return;
	pqxx::work w(conn);
        w.exec("DELETE FROM aviationdb.tracks WHERE ID=" + w.quote(m_id) + ";");
	w.exec("DELETE FROM aviationdb.tracks_appendlog WHERE TRACKID=" + w.quote(m_id) + ";");
	if (rtree)
		w.exec("DELETE FROM aviationdb.tracks_rtree WHERE ID=" + w.quote(m_id) + ";");
	w.commit();
	m_id = 0;
}

void DbBaseElements::Track::update_index(pqxx::connection& conn, bool rtree)
{
	if (!m_id)
		return;
	pqxx::work w(conn);
	w.exec("UPDATE aviationdb.tracks SET TILE=" + w.quote(TileNumber::to_tilenumber(m_bbox)) + " WHERE ID=" + w.quote(m_id) + ";");
	if (rtree)
		w.exec("UPDATE aviationdb.tracks_rtree SET SWLAT=" + w.quote(m_bbox.get_south()) + ",NELAT=" + w.quote(m_bbox.get_north()) +
		       ",SWLON=" + w.quote(m_bbox.get_west()) + ",NELON=" + w.quote((long long int)m_bbox.get_east_unwrapped()) + " WHERE ID=" + w.quote(m_id) + ";");
	w.commit();
}

void DbBaseElements::Track::save_appendlog(pqxx::connection& conn, bool rtree)
{
	if (!is_valid())
		return;
	if (m_savedtrackpoints >= m_trackpoints.size())
		return;
	if (m_table != table_main || !m_id) {
		save(conn, rtree);
		return;
	}
	{
		size_type nrel = m_trackpoints.size() - m_savedtrackpoints;
		uint8_t data[nrel * TrackPoint::blob_size_v1];
		for (size_type i = 0; i < nrel; i++)
			m_trackpoints[i + m_savedtrackpoints].write_blob(data + i * TrackPoint::blob_size_v1);
		pqxx::binarystring blob(data, nrel * TrackPoint::blob_size_v1);
		pqxx::work w(conn);
		w.exec("INSERT INTO aviationdb.tracks_appendlog (TRACKID,ID,POINTS) SELECT " +
		       w.quote(m_id) + ",COALESCE(MAX(ID),0)+1," + w.quote(blob) + " FROM aviationdb.tracks_appendlog WHERE TRACKID=" +
		       w.quote(m_id) + ";");
		w.commit();
	}
	m_savedtrackpoints = m_trackpoints.size();
}

#endif

Glib::ustring DbBaseElements::Track::get_fromicaoname(void) const
{
	Glib::ustring r(get_fromicao());
	if (!r.empty() && !get_fromname().empty())
		r += " ";
	r += get_fromname();
	return r;
}

Glib::ustring DbBaseElements::Track::get_toicaoname(void) const
{
	Glib::ustring r(get_toicao());
	if (!r.empty() && !get_toname().empty())
		r += " ";
	r += get_toname();
	return r;
}

void DbBaseElements::Track::save_appendlog(sqlite3x::sqlite3_connection& db, bool rtree, bool aux_rtree)
{
	if (!is_valid())
		return;
	if (m_savedtrackpoints >= m_trackpoints.size())
		return;
	if (m_table != table_main || !m_id) {
		save(db, rtree, aux_rtree);
		return;
	}
	{
		int64_t appid = 1;
		sqlite3x::sqlite3_transaction tran(db);
		{
			sqlite3x::sqlite3_command cmd(db, "SELECT MAX(ID) FROM tracks_appendlog WHERE TRACKID=?;");
			cmd.bind(1, (long long int)m_id);
			sqlite3x::sqlite3_cursor cursor = cmd.executecursor();
			if (cursor.step())
				appid = cursor.getint(0) + 1;
		}
		size_type nrel = m_trackpoints.size() - m_savedtrackpoints;
		uint8_t data[nrel * TrackPoint::blob_size_v1];
		for (size_type i = 0; i < nrel; i++)
			m_trackpoints[i + m_savedtrackpoints].write_blob(data + i * TrackPoint::blob_size_v1);
		{
			sqlite3x::sqlite3_command cmd(db, "INSERT INTO tracks_appendlog (TRACKID,ID,POINTS) VALUES (?,?,?);");
			cmd.bind(1, (long long int)m_id);
			cmd.bind(2, (long long int)appid);
			cmd.bind(3, data, nrel * TrackPoint::blob_size_v1);
			cmd.executenonquery();
		}
	}
	m_savedtrackpoints = m_trackpoints.size();
}

void DbBaseElements::Track::recompute_bbox(void)
{
	if (!size()) {
		m_bbox = Rect();
		return;
	}
	Point pt(operator[](0).get_coord());
	Point sw(0, 0), ne(0, 0);
	for (unsigned int i = 1; i < size(); i++) {
		Point p(operator[](i).get_coord() - pt);
		sw = Point(std::min(sw.get_lon(), p.get_lon()), std::min(sw.get_lat(), p.get_lat()));
		ne = Point(std::max(ne.get_lon(), p.get_lon()), std::max(ne.get_lat(), p.get_lat()));
	}
	m_bbox = Rect(sw + pt, ne + pt);
}

DbBaseElements::Track::TrackPoint& DbBaseElements::Track::append(const TrackPoint & tp)
{
	m_trackpoints.push_back(tp);
	return m_trackpoints.back();
}

DbBaseElements::Track::TrackPoint& DbBaseElements::Track::insert(const TrackPoint & tp, int index)
{
	m_trackpoints.insert(m_trackpoints.begin() + index, tp);
	return m_trackpoints[index];
}

void DbBaseElements::Track::erase(int index)
{
	m_trackpoints.erase(m_trackpoints.begin() + index);
}

void DbBaseElements::Track::make_valid(void)
{
	if (is_valid())
		return;
	m_format = format_gzip;
}

template<> void DbBase<DbBaseElements::Track>::drop_tables(void)
{
	{ sqlite3x::sqlite3_command cmd(m_db, "DROP TABLE IF EXISTS tracks;"); cmd.executenonquery(); }
	{ sqlite3x::sqlite3_command cmd(m_db, "DROP TABLE IF EXISTS tracks_appendlog;"); cmd.executenonquery(); }
	{ sqlite3x::sqlite3_command cmd(m_db, "DROP TABLE IF EXISTS tracks_deleted;"); cmd.executenonquery(); }
}

template<> void DbBase<DbBaseElements::Track>::create_tables(void)
{
	{
		sqlite3x::sqlite3_command cmd(m_db, "CREATE TABLE IF NOT EXISTS tracks (ID INTEGER PRIMARY KEY NOT NULL, "
					      "SRCID TEXT UNIQUE NOT NULL,"
					      "FROMICAO TEXT,"
					      "FROMNAME TEXT,"
					      "TOICAO TEXT,"
					      "TONAME TEXT,"
					      "NOTES TEXT,"
					      "OFFBLOCKTIME INTEGER,"
					      "TAKEOFFTIME INTEGER,"
					      "LANDINGTIME INTEGER,"
					      "ONBLOCKTIME INTEGER,"
					      "SWLAT INTEGER,"
					      "SWLON INTEGER,"
					      "NELAT INTEGER,"
					      "NELON INTEGER,"
					      "POINTS BLOB,"
					      "MODIFIED INTEGER,"
					      "TILE INTEGER);");
		cmd.executenonquery();
	}
	{
		sqlite3x::sqlite3_command cmd(m_db, "CREATE TABLE IF NOT EXISTS tracks_appendlog (ID INTEGER NOT NULL, "
					      "TRACKID INTEGER NOT NULL,"
					      "POINTS BLOB);");
		cmd.executenonquery();
	}
	{
		sqlite3x::sqlite3_command cmd(m_db, "CREATE TABLE IF NOT EXISTS tracks_deleted(SRCID TEXT PRIMARY KEY NOT NULL);");
		cmd.executenonquery();
	}
}

template<> void DbBase<DbBaseElements::Track>::drop_indices(void)
{
	{ sqlite3x::sqlite3_command cmd(m_db, "DROP INDEX IF EXISTS tracks_srcid;"); cmd.executenonquery(); }
	{ sqlite3x::sqlite3_command cmd(m_db, "DROP INDEX IF EXISTS tracks_fromname;"); cmd.executenonquery(); }
	{ sqlite3x::sqlite3_command cmd(m_db, "DROP INDEX IF EXISTS tracks_fromicao;"); cmd.executenonquery(); }
	{ sqlite3x::sqlite3_command cmd(m_db, "DROP INDEX IF EXISTS tracks_toname;"); cmd.executenonquery(); }
	{ sqlite3x::sqlite3_command cmd(m_db, "DROP INDEX IF EXISTS tracks_toicao;"); cmd.executenonquery(); }
	{ sqlite3x::sqlite3_command cmd(m_db, "DROP INDEX IF EXISTS tracks_bbox;"); cmd.executenonquery(); }
	{ sqlite3x::sqlite3_command cmd(m_db, "DROP INDEX IF EXISTS tracks_modified;"); cmd.executenonquery(); }
	{ sqlite3x::sqlite3_command cmd(m_db, "DROP INDEX IF EXISTS tracks_tile;"); cmd.executenonquery(); }
	{ sqlite3x::sqlite3_command cmd(m_db, "DROP INDEX IF EXISTS tracks_appendlog_id;"); cmd.executenonquery(); }
}

template<> void DbBase<DbBaseElements::Track>::create_indices(void)
{
	{ sqlite3x::sqlite3_command cmd(m_db, "CREATE INDEX IF NOT EXISTS tracks_srcid ON tracks(SRCID);"); cmd.executenonquery(); }
	{ sqlite3x::sqlite3_command cmd(m_db, "CREATE INDEX IF NOT EXISTS tracks_fromname ON tracks(FROMNAME COLLATE NOCASE);"); cmd.executenonquery(); }
	{ sqlite3x::sqlite3_command cmd(m_db, "CREATE INDEX IF NOT EXISTS tracks_fromicao ON tracks(FROMICAO COLLATE NOCASE);"); cmd.executenonquery(); }
	{ sqlite3x::sqlite3_command cmd(m_db, "CREATE INDEX IF NOT EXISTS tracks_toname ON tracks(TONAME COLLATE NOCASE);"); cmd.executenonquery(); }
	{ sqlite3x::sqlite3_command cmd(m_db, "CREATE INDEX IF NOT EXISTS tracks_toicao ON tracks(TOICAO COLLATE NOCASE);"); cmd.executenonquery(); }
	{ sqlite3x::sqlite3_command cmd(m_db, "CREATE INDEX IF NOT EXISTS tracks_bbox ON tracks(SWLAT,NELAT,SWLON,NELON);"); cmd.executenonquery(); }
	{ sqlite3x::sqlite3_command cmd(m_db, "CREATE INDEX IF NOT EXISTS tracks_modified ON tracks(MODIFIED);"); cmd.executenonquery(); }
	{ sqlite3x::sqlite3_command cmd(m_db, "CREATE INDEX IF NOT EXISTS tracks_tile ON tracks(TILE);"); cmd.executenonquery(); }
	{ sqlite3x::sqlite3_command cmd(m_db, "CREATE INDEX IF NOT EXISTS tracks_appendlog_id ON tracks_appendlog(TRACKID,ID);"); cmd.executenonquery(); }
}

template<> const char *DbBase<DbBaseElements::Track>::main_table_name = "tracks";
template<> const char *DbBase<DbBaseElements::Track>::database_file_name = "tracks.db";
template<> const char *DbBase<DbBaseElements::Track>::order_field = "SRCID";
template<> const char *DbBase<DbBaseElements::Track>::delete_field = "SRCID";
template<> const bool DbBase<DbBaseElements::Track>::area_data = true;

void TracksDb::open(const Glib::ustring& path)
{
	DbBase<DbBaseElements::Track>::open(path);
	if (m_open != openstate_mainopen)
		return;
	clear_appendlogs();
}

void TracksDb::clear_appendlogs(void)
{
	if (m_open != openstate_mainopen)
		return;
	std::list<Address> addrs;
	{
		sqlite3x::sqlite3_command cmd(m_db, "SELECT DISTINCT TRACKID FROM tracks_appendlog;");
		sqlite3x::sqlite3_cursor cursor = cmd.executecursor();
		if (cursor.step())
			addrs.push_back(Address(cursor.getint(0), element_t::table_main));
	}
	while (!addrs.empty()) {
		element_t e(operator()(addrs.front()));
		save(e);
		addrs.pop_front();
	}
}

#ifdef HAVE_PQXX

template<> void PGDbBase<DbBaseElements::Track>::drop_tables(void)
{
	pqxx::work w(m_conn);
	w.exec("DROP TABLE IF EXISTS aviationdb.tracks;");
	w.exec("DROP TABLE IF EXISTS aviationdb.tracks_appendlog;");
	w.commit();
}

template<> void PGDbBase<DbBaseElements::Track>::create_tables(void)
{
	create_common_tables();
	pqxx::work w(m_conn);
	w.exec("CREATE TABLE IF NOT EXISTS aviationdb.tracks ("
	       "FROMICAO TEXT,"
	       "FROMNAME TEXT,"
	       "TOICAO TEXT,"
	       "TONAME TEXT,"
	       "NOTES TEXT,"
	       "OFFBLOCKTIME INTEGER,"
	       "TAKEOFFTIME INTEGER,"
	       "LANDINGTIME INTEGER,"
	       "ONBLOCKTIME INTEGER,"
	       "SWLAT INTEGER,"
	       "SWLON INTEGER,"
	       "NELAT INTEGER,"
	       "NELON BIGINT,"
	       "POINTS BYTEA,"
	       "GPOINTS ext.GEOGRAPHY(LINESTRING,4326),"
	       "TILE INTEGER,"
	       "PRIMARY KEY (ID)"
	       ") INHERITS (aviationdb.labelsourcebase);");
	w.exec("CREATE TABLE IF NOT EXISTS aviationdb.tracks_appendlog ("
	       "ID INTEGER NOT NULL,"
	       "TRACKID BIGINT NOT NULL REFERENCES aviationdb.tracks(ID),"
	       "POINTS BYTEA,"
	       "PRIMARY KEY (TRACKID,ID));");
	w.commit();
}

template<> void PGDbBase<DbBaseElements::Track>::drop_indices(void)
{
	pqxx::work w(m_conn);
	w.exec("DROP INDEX IF EXISTS aviationdb.tracks_srcid;");
	w.exec("DROP INDEX IF EXISTS aviationdb.tracks_fromname;");
	w.exec("DROP INDEX IF EXISTS aviationdb.tracks_fromicao;");
	w.exec("DROP INDEX IF EXISTS aviationdb.tracks_toname;");
	w.exec("DROP INDEX IF EXISTS aviationdb.tracks_toicao;");
	w.exec("DROP INDEX IF EXISTS aviationdb.tracks_bbox;");
	w.exec("DROP INDEX IF EXISTS aviationdb.tracks_modified;");
	w.exec("DROP INDEX IF EXISTS aviationdb.tracks_tile;");
	w.exec("DROP INDEX IF EXISTS aviationdb.tracks_appendlog_id;");
	w.commit();
}

template<> void PGDbBase<DbBaseElements::Track>::create_indices(void)
{
	pqxx::work w(m_conn);
	w.exec("CREATE INDEX IF NOT EXISTS tracks_srcid ON aviationdb.tracks(SRCID);");
	w.exec("CREATE INDEX IF NOT EXISTS tracks_fromname ON aviationdb.tracks((FROMNAME::ext.CITEXT) COLLATE \"C\");");
	w.exec("CREATE INDEX IF NOT EXISTS tracks_fromicao ON aviationdb.tracks((FROMICAO::ext.CITEXT) COLLATE \"C\");");
	w.exec("CREATE INDEX IF NOT EXISTS tracks_toname ON aviationdb.tracks((TONAME::ext.CITEXT) COLLATE \"C\");");
	w.exec("CREATE INDEX IF NOT EXISTS tracks_toicao ON aviationdb.tracks((TOICAO::ext.CITEXT) COLLATE \"C\");");
	w.exec("CREATE INDEX IF NOT EXISTS tracks_bbox ON aviationdb.tracks(SWLAT,NELAT,SWLON,NELON);");
	w.exec("CREATE INDEX IF NOT EXISTS tracks_modified ON aviationdb.tracks(MODIFIED);");
	w.exec("CREATE INDEX IF NOT EXISTS tracks_tile ON aviationdb.tracks(TILE);");
	w.exec("CREATE INDEX IF NOT EXISTS tracks_appendlog_id ON aviationdb.tracks_appendlog(TRACKID,ID);");
	w.commit();
}

template<> const char *PGDbBase<DbBaseElements::Track>::main_table_name = "aviationdb.tracks";
template<> const char *PGDbBase<DbBaseElements::Track>::order_field = "SRCID";
template<> const char *PGDbBase<DbBaseElements::Track>::delete_field = "SRCID";
template<> const bool PGDbBase<DbBaseElements::Track>::area_data = true;

void TracksPGDb::clear_appendlogs(void)
{
	std::list<Address> addrs;
	{
		pqxx::read_transaction w(m_conn);
		pqxx::result r(w.exec("SELECT DISTINCT TRACKID FROM aviationdb.tracks_appendlog;"));
		for (pqxx::result::const_iterator ri(r.begin()), re(r.end()); ri != re; ++ri)
			addrs.push_back(Address((*ri)[0].as<int64_t>(0), element_t::table_main));
	}
	while (!addrs.empty()) {
		element_t e(operator()(addrs.front()));
		save(e);
		addrs.pop_front();
	}
}

#endif
