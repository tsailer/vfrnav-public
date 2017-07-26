#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <fstream>
#include <sstream>
#include <iostream>
#include <iomanip>

#include "cfmuautoroute45.hh"

const char CFMUAutoroute45::GraphSerializeBase::signature[] = "vfrnav CFMU Routing Graph V2\n";

CFMUAutoroute45::GraphSerializeBase::GraphSerializeBase(void)
	: m_minlevel(std::numeric_limits<int16_t>::min()),
	  m_maxlevel(std::numeric_limits<int16_t>::min())
{
	Point pt;
	pt.set_invalid();
	m_bbox = Rect(pt, pt);
}

bool CFMUAutoroute45::GraphSerializeBase::is_valid(void) const
{
	if (m_bbox.get_southwest().is_invalid() ||
	    m_bbox.get_northeast().is_invalid() ||
	    m_minlevel > m_maxlevel || m_minlevel < 0 ||
	    m_minlevel % 10 || m_maxlevel % 10)
		return false;
	return true;
}

CFMUAutoroute45::GraphWrite::GraphWrite(std::ostream& os)
	: m_os(os)
{
}

template <> void CFMUAutoroute45::GraphWrite::io<int8_t>(const int8_t& v)
{
        m_os.write(reinterpret_cast<const char *>(&v), sizeof(v));
        if (!m_os)
                throw std::runtime_error("io<int8_t>: write error");	
}

template <> void CFMUAutoroute45::GraphWrite::io<uint8_t>(const uint8_t& v)
{
        m_os.write(reinterpret_cast<const char *>(&v), sizeof(v));
        if (!m_os)
                throw std::runtime_error("io<uint8_t>: write error");	
}

template <> void CFMUAutoroute45::GraphWrite::io<int16_t>(const int16_t& v)
{
        uint8_t buf[2];
        buf[0] = v;
        buf[1] = v >> 8;
        m_os.write(reinterpret_cast<char *>(buf), sizeof(buf));
        if (!m_os)
                throw std::runtime_error("io<int16_t>: write error");	
}

template <> void CFMUAutoroute45::GraphWrite::io<uint16_t>(const uint16_t& v)
{
        uint8_t buf[2];
        buf[0] = v;
        buf[1] = v >> 8;
        m_os.write(reinterpret_cast<char *>(buf), sizeof(buf));
        if (!m_os)
                throw std::runtime_error("io<uint16_t>: write error");	
}

template <> void CFMUAutoroute45::GraphWrite::io<int32_t>(const int32_t& v)
{
        uint8_t buf[4];
        buf[0] = v;
        buf[1] = v >> 8;
        buf[2] = v >> 16;
        buf[3] = v >> 24;
        m_os.write(reinterpret_cast<char *>(buf), sizeof(buf));
        if (!m_os)
                throw std::runtime_error("io<int32_t>: write error");	
}

template <> void CFMUAutoroute45::GraphWrite::io<uint32_t>(const uint32_t& v)
{
        uint8_t buf[4];
        buf[0] = v;
        buf[1] = v >> 8;
        buf[2] = v >> 16;
        buf[3] = v >> 24;
        m_os.write(reinterpret_cast<char *>(buf), sizeof(buf));
        if (!m_os)
                throw std::runtime_error("io<uint32_t>: write error");	
}

template <> void CFMUAutoroute45::GraphWrite::io<int64_t>(const int64_t& v)
{
        uint8_t buf[8];
        buf[0] = v;
        buf[1] = v >> 8;
        buf[2] = v >> 16;
        buf[3] = v >> 24;
        buf[4] = v >> 32;
        buf[5] = v >> 40;
        buf[6] = v >> 48;
        buf[7] = v >> 56;
	m_os.write(reinterpret_cast<char *>(buf), sizeof(buf));
        if (!m_os)
                throw std::runtime_error("io<int64_t>: write error");	
}

template <> void CFMUAutoroute45::GraphWrite::io<uint64_t>(const uint64_t& v)
{
        uint8_t buf[8];
        buf[0] = v;
        buf[1] = v >> 8;
        buf[2] = v >> 16;
        buf[3] = v >> 24;
        buf[4] = v >> 32;
        buf[5] = v >> 40;
        buf[6] = v >> 48;
        buf[7] = v >> 56;
	m_os.write(reinterpret_cast<char *>(buf), sizeof(buf));
        if (!m_os)
                throw std::runtime_error("io<uint64_t>: write error");	
}

template <> void CFMUAutoroute45::GraphWrite::io<float>(const float& v)
{
        union {
                float f;
                uint32_t u;
        } x;
        x.f = v;
	io(x.u);
}

template <> void CFMUAutoroute45::GraphWrite::io<double>(const double& v)
{
	union {
                double d;
                uint64_t u;
        } x;
        x.d = v;
	io(x.u);
}

template <> void CFMUAutoroute45::GraphWrite::io<std::string>(const std::string& v)
{
        iouint32(v.size());
        m_os.write(v.c_str(), v.size());
        if (!m_os)
                throw std::runtime_error("io<std::string>: write error");
}

template <> void CFMUAutoroute45::GraphWrite::io<Glib::ustring>(const Glib::ustring& v)
{
        iouint32(v.size());
        m_os.write(v.c_str(), v.size());
        if (!m_os)
                throw std::runtime_error("io<Glib::ustring>: write error");
}

template <> void CFMUAutoroute45::GraphWrite::io<char>(const char& v)
{
	ioint8(v);
}

template <> void CFMUAutoroute45::GraphWrite::io<bool>(const bool& v)
{
	uint8_t vv(!!v);
	io(vv);
}

void CFMUAutoroute45::GraphWrite::header(void)
{
	m_os.write(signature, sizeof(signature));
	if (!m_os)
		throw std::runtime_error("cannot write signature");
	if (!is_valid())
		throw std::runtime_error("write header: invalid parameters");
	m_bbox.hibernate_binary(*this);
	io(m_minlevel);
	io(m_maxlevel);
}

CFMUAutoroute45::GraphRead::GraphRead(std::istream& is)
	: m_is(is)
{
}

template <> void CFMUAutoroute45::GraphRead::io<int8_t>(int8_t& v)
{
        m_is.read(reinterpret_cast<char *>(&v), sizeof(v));
        if (!m_is)
                throw std::runtime_error("io<int8_t>: short read");
}

template <> void CFMUAutoroute45::GraphRead::io<uint8_t>(uint8_t& v)
{
        m_is.read(reinterpret_cast<char *>(&v), sizeof(v));
        if (!m_is)
                throw std::runtime_error("io<uint8_t>: short read");
}

template <> void CFMUAutoroute45::GraphRead::io<int16_t>(int16_t& v)
{
        uint8_t buf[2];
        m_is.read(reinterpret_cast<char *>(buf), sizeof(buf));
        if (!m_is)
                throw std::runtime_error("io<int16_t>: short read");
        v = buf[0] | (buf[1] << 8);
}

template <> void CFMUAutoroute45::GraphRead::io<uint16_t>(uint16_t& v)
{
        uint8_t buf[2];
        m_is.read(reinterpret_cast<char *>(buf), sizeof(buf));
        if (!m_is)
                throw std::runtime_error("io<uint16_t>: short read");
        v = buf[0] | (buf[1] << 8);
}

template <> void CFMUAutoroute45::GraphRead::io<int32_t>(int32_t& v)
{
        uint8_t buf[4];
        m_is.read(reinterpret_cast<char *>(buf), sizeof(buf));
        if (!m_is)
                throw std::runtime_error("io<int32_t>: short read");
        v = buf[0] | (buf[1] << 8) | (buf[2] << 16) | (buf[3] << 24);
}

template <> void CFMUAutoroute45::GraphRead::io<uint32_t>(uint32_t& v)
{
        uint8_t buf[4];
        m_is.read(reinterpret_cast<char *>(buf), sizeof(buf));
        if (!m_is)
                throw std::runtime_error("io<uint32_t>: short read");
        v = buf[0] | (buf[1] << 8) | (buf[2] << 16) | (buf[3] << 24);
}

template <> void CFMUAutoroute45::GraphRead::io<int64_t>(int64_t& v)
{
        uint8_t buf[8];
        m_is.read(reinterpret_cast<char *>(buf), sizeof(buf));
        if (!m_is)
                throw std::runtime_error("io<int64_t>: short read");
	uint32_t r1(buf[4] | (buf[5] << 8) | (buf[6] << 16) | (buf[7] << 24));
        uint64_t r(r1);
        r <<= 32;
	r1 = buf[0] | (buf[1] << 8) | (buf[2] << 16) | (buf[3] << 24);
        r |= r1;
        v = r;
}

template <> void CFMUAutoroute45::GraphRead::io<uint64_t>(uint64_t& v)
{
        uint8_t buf[8];
        m_is.read(reinterpret_cast<char *>(buf), sizeof(buf));
        if (!m_is)
                throw std::runtime_error("io<uint64_t>: short read");
	uint32_t r1(buf[4] | (buf[5] << 8) | (buf[6] << 16) | (buf[7] << 24));
        uint64_t r(r1);
        r <<= 32;
	r1 = buf[0] | (buf[1] << 8) | (buf[2] << 16) | (buf[3] << 24);
        r |= r1;
        v = r;
}

template <> void CFMUAutoroute45::GraphRead::io<float>(float& v)
{
        union {
                float f;
                uint32_t u;
        } x;
	io(x.u);
        v = x.f;
}

template <> void CFMUAutoroute45::GraphRead::io<double>(double& v)
{
	union {
                double d;
                uint64_t u;
        } x;
	io(x.u);
        v = x.d;
}

template <> void CFMUAutoroute45::GraphRead::io<std::string>(std::string& v)
{
	v.clear();
        uint32_t len(0);
	io(len);
        if (!len)
                return;
        char buf[len];
        m_is.read(buf, len);
        if (!m_is)
                throw std::runtime_error("io<std::string>: short read");
        v = std::string(buf, buf + len);
}

template <> void CFMUAutoroute45::GraphRead::io<Glib::ustring>(Glib::ustring& v)
{
	v.clear();
        uint32_t len(0);
	io(len);
        if (!len)
                return;
        char buf[len];
        m_is.read(buf, len);
        if (!m_is)
                throw std::runtime_error("io<Glib::ustring>: short read");
        v = std::string(buf, buf + len);
}

template <> void CFMUAutoroute45::GraphRead::io<char>(char& v)
{
	ioint8(v);
}

template <> void CFMUAutoroute45::GraphRead::io<bool>(bool& v)
{
	uint8_t vv(0);
	io(vv);
	v = !!vv;
}

void CFMUAutoroute45::GraphRead::header(void)
{
	{
		char buf[sizeof(signature)];
		m_is.read(buf, sizeof(signature));
		if (!m_is)
			throw std::runtime_error("cannot read signature");
		if (memcmp(buf, signature, sizeof(signature)))
			throw std::runtime_error("invalid signature");
	}
	m_bbox.hibernate_binary(*this);
	io(m_minlevel);
	io(m_maxlevel);
	if (!is_valid())
		throw std::runtime_error("read header: invalid parameters");
}

void CFMUAutoroute45::lgraphsave(std::ostream& os, const Rect& bbox)
{
	const unsigned int nrperf(m_performance.size());
	if (!nrperf)
		throw std::runtime_error("no performance table");
	if (!boost::num_vertices(m_lgraph))
		throw std::runtime_error("no vertices");
	typedef std::vector<uint32_t> vertexremap_t;
	vertexremap_t vertexremap(boost::num_vertices(m_lgraph) + 1, 0);
	for (LGraph::vertex_descriptor v(0); v < boost::num_vertices(m_lgraph); ++v) {
		if (!boost::out_degree(v, m_lgraph) && !boost::in_degree(v, m_lgraph))
			continue;
		const LVertex& vv(m_lgraph[v]);
		if (!bbox.is_inside(vv.get_coord()))
			continue;
		vertexremap[v] = 1;
	}
	{
		unsigned int idx(0);
		for (vertexremap_t::iterator i(vertexremap.begin()), e(vertexremap.end()); i != e; ++i) {
			unsigned int x(idx);
			std::swap(x, *i);
			idx += x;
		}
	}
       	GraphWrite gw(os);
	gw.set_bbox(bbox);
	gw.set_minlevel(m_performance.get_minaltitude() / 100);
	gw.set_maxlevel(m_performance.get_maxaltitude() / 100);
	if (gw.get_maxlevel() - gw.get_minlevel() != (nrperf - 1U) * 10U) {
		std::ostringstream oss;
		oss << "invalid performance table: levels " << nrperf
		    << " F" << std::setfill('0') << std::setw(3) << gw.get_minlevel()
		    << "..F" << std::setfill('0') << std::setw(3) << gw.get_maxlevel();
		throw std::runtime_error(oss.str());
	}
	if (true) {
		Rect bbox(gw.get_bbox());
		bool subseq(false);
		Rect bbox1;
		for (LGraph::vertex_descriptor v(0); v < boost::num_vertices(m_lgraph); ++v) {
			if (vertexremap[v] == vertexremap[v + 1])
				continue;
			const LVertex& vv(m_lgraph[v]);
			if (subseq) {
				bbox1 = bbox1.add(vv.get_coord());
				continue;
			}
			bbox1 = Rect(vv.get_coord(), vv.get_coord());
			subseq = true;
		}
		if (!subseq)
			throw std::runtime_error("Graph does not contain a non-skipped vertex");
		std::ostringstream oss;
		oss << "Graph bbox ["
		    << bbox.get_southwest().get_lat_str2() << ' ' << bbox.get_southwest().get_lon_str2()
		    << ' ' << bbox.get_northeast().get_lat_str2() << ' ' << bbox.get_northeast().get_lon_str2()
		    << "] actual bbox ["
		    << bbox1.get_southwest().get_lat_str2() << ' ' << bbox1.get_southwest().get_lon_str2()
		    << ' ' << bbox1.get_northeast().get_lat_str2() << ' ' << bbox1.get_northeast().get_lon_str2()
		    << "] F" << std::setfill('0') << std::setw(3) << gw.get_minlevel()
		    << "..F" << std::setfill('0') << std::setw(3) << gw.get_maxlevel();
		m_signal_log(log_debug0, oss.str());
	}
	gw.header();
	// node table
	gw.iouint32(vertexremap.back());
	for (LGraph::vertex_descriptor v(0); v < boost::num_vertices(m_lgraph); ++v) {
		if (vertexremap[v] == vertexremap[v + 1])
			continue;
		const LVertex& vv(m_lgraph[v]);
		Engine::DbObject::ptr_t p(vv.get_object());
		Engine::DbObject::hibernate_binary(p, gw);
	}
	// airway table
	gw.iouint32(m_lgraphairwaytable.size());
	for (lgraphairwaytable_t::const_iterator i(m_lgraphairwaytable.begin()), e(m_lgraphairwaytable.end()); i != e; ++i)
		gw.io(*i);
	// edge list
	{
	        LGraph::edge_iterator e0, e1;
		uint32_t nredges(0), nrdiscedges(0);
		for (boost::tie(e0, e1) = boost::edges(m_lgraph); e0 != e1; ++e0) {
			const LEdge& ee(m_lgraph[*e0]);
			if (!ee.is_match(airwaymatchdctawy))
				continue;
			uint32_t v0(boost::source(*e0, m_lgraph));
			uint32_t v1(boost::target(*e0, m_lgraph));
			if (vertexremap[v0] == vertexremap[v0 + 1] ||
			    vertexremap[v1] == vertexremap[v1 + 1] ||
			    ee.get_levels() != nrperf) {
				++nrdiscedges;
				continue;
			}
			++nredges;
		}
		if (true) {
			std::ostringstream oss;
			oss << "#edges " << boost::num_edges(m_lgraph) << " perf " << nrperf
			    << " #vertices " << boost::num_vertices(m_lgraph) << " awy/dct edges " << nredges
			    << " skipped edges " << nrdiscedges;
			m_signal_log(log_debug0, oss.str());
		}
		gw.iouint32(nredges);
		for (boost::tie(e0, e1) = boost::edges(m_lgraph); e0 != e1; ++e0) {
			const LEdge& ee(m_lgraph[*e0]);
			if (!ee.is_match(airwaymatchdctawy))
				continue;
			LGraph::vertex_descriptor v0(boost::source(*e0, m_lgraph));
			LGraph::vertex_descriptor v1(boost::target(*e0, m_lgraph));
			if (vertexremap[v0] == vertexremap[v0 + 1] ||
			    vertexremap[v1] == vertexremap[v1 + 1] ||
			    ee.get_levels() != nrperf)
				continue;
        		gw.io(vertexremap[v0]);
			gw.io(vertexremap[v1]);
			gw.iouint32(ee.get_ident());
			gw.io(ee.get_dist());
			gw.io(ee.get_truetrack());
			for (unsigned int pi = 0; pi < nrperf; ++pi)
				gw.io(ee.get_metric(pi));
		}
		if (nredges + nrdiscedges != boost::num_edges(m_lgraph)) {
			std::ostringstream oss;
			oss << "boost #edges / edge iterator inconsistency: " << nredges << " + " << nrdiscedges
			    << " != " << boost::num_edges(m_lgraph);
			throw std::runtime_error(oss.str());
		}
	}
}

void CFMUAutoroute45::lgraphbinload(GraphRead& gr, const Rect& bbox)
{
	// header must already have been consumed
	clearlgraph();
	unsigned int nrperf(m_performance.size());
	if (!nrperf)
		throw std::runtime_error("no performance table");
	if (!gr.is_valid())
		throw std::runtime_error("header not valid");
	if (!gr.get_bbox().is_inside(bbox))
		throw std::runtime_error("area not contained in graph");
	if (m_performance.get_minaltitude() < gr.get_minlevel() * 100U ||
	    m_performance.get_maxaltitude() > gr.get_maxlevel() * 100U)
		throw std::runtime_error("levels not contained in graph");
	unsigned int nrperffile((gr.get_maxlevel() - gr.get_minlevel()) / 10U + 1U);
	unsigned int poffs((m_performance.get_minaltitude() - gr.get_minlevel() * 100U) / 1000U);
	typedef std::vector<uint32_t> vertexremap_t;
	vertexremap_t vertexremap;
	// node table
	{
		uint32_t nrnodes(0), nodeidx(0);
		gr.iouint32(nrnodes);
		for (unsigned int i = 0; i < nrnodes; ++i) {
			Engine::DbObject::ptr_t p;
			Engine::DbObject::hibernate_binary(p, gr);
			vertexremap.push_back(nodeidx);
			if (!p || !bbox.is_inside(p->get_coord()))
				continue;
			++nodeidx;
			lgraphaddvertex(p);
		}
		vertexremap.push_back(nodeidx);
	}
	// airway table
	{
		uint32_t nr(0);
		gr.iouint32(nr);
		for (unsigned int i = 0; i < nr; ++i) {
			std::string awyu;
			gr.io(awyu);
			std::pair<lgraphairwayrevtable_t::iterator,bool> ins(m_lgraphairwayrevtable.insert(std::make_pair(awyu, m_lgraphairwaytable.size())));
			if (ins.second)
				m_lgraphairwaytable.push_back(awyu);
			else
				throw std::runtime_error("duplicate airway " + awyu);
		}
	}
	// edge list
	{
		uint32_t nr(0);
		gr.iouint32(nr);
		for (unsigned int i = 0; i < nr; ++i) {
			uint32_t v0(0), v1(0), ident(0);
			float dist(0), tt(0);
			float metric[nrperffile];
			gr.io(v0);
			gr.io(v1);
			gr.io(ident);
			gr.io(dist);
			gr.io(tt);
			for (unsigned int pi = 0; pi < nrperffile; ++pi)
				gr.io(metric[pi]);
			// compute correct graph node indices
			if (v0 + 1 >= vertexremap.size() || vertexremap[v0] == vertexremap[v0 + 1])
				continue;
			v0 = vertexremap[v0];
			if (v0 >= boost::num_vertices(m_lgraph))
				continue;
			if (v1 + 1 >= vertexremap.size() || vertexremap[v1] == vertexremap[v1 + 1])
				continue;
			v1 = vertexremap[v1];
			if (v1 >= boost::num_vertices(m_lgraph))
				continue;
			if (std::isnan(dist) || std::isnan(tt)) {
				const LVertex& vv0(m_lgraph[v0]);
				const LVertex& vv1(m_lgraph[v1]);
				dist = vv0.get_coord().spheric_distance_nmi(vv1.get_coord());
				tt = vv0.get_coord().spheric_true_course(vv1.get_coord());
				if (true) {
					std::ostringstream oss;
					oss << "graph deserialize: edge " << vv0.get_ident()
					    << ' ' << lgraphindex2airway(ident, true)
					    << ' ' << vv1.get_ident()
					    << " has invalid distance/truetrack, fixing "
					    << std::fixed << std::setprecision(1) << dist << '/' << tt;
					m_signal_log(log_normal, oss.str());
				}
			}
			if (ident == airwaydct && dist > get_dctlimit()) {
				if (false) {
					std::ostringstream oss;
					oss << "skipping DCT edge " << lgraphvertexname(v0)
					    << "->" << lgraphvertexname(v1) << std::fixed
					    << " dist " << std::setprecision(1) << dist
					    << " limit " << std::setprecision(1) << get_dctlimit();
					m_signal_log(log_normal, oss.str());
				}
				continue;
			}
			LEdge edge(nrperf, ident, dist, tt);
			for (unsigned int pi = 0; pi < nrperf; ++pi) {
				unsigned int pi1(pi + poffs);
				if (pi1 >= nrperffile)
					break;
				edge.set_metric(pi, metric[pi1]);
			}
			if (edge.is_valid())
				boost::add_edge(v0, v1, edge, m_lgraph);
		}
	}
}

class CFMUAutoroute45::GraphFile {
public:
	typedef Glib::RefPtr<GraphFile> ptr_t;

	GraphFile(const std::string& fn);
	void reference(void) const;
	void unreference(void) const;

	GraphRead& get_gr(void) { return m_gr; }
	const GraphRead& get_gr(void) const { return m_gr; }
	std::ifstream& get_is(void) { return m_is; }
	const std::ifstream& get_is(void) const { return m_is; }
	const std::string& get_filename(void) const { return m_filename; }

	struct sort_ptr_area {
	public:
		sort_ptr_area(void) {}
		bool operator()(const ptr_t& p0, const ptr_t& p1) const;
	};

protected:
	std::string m_filename;
	std::ifstream m_is;
	GraphRead m_gr;
	mutable gint m_refcount;
};

CFMUAutoroute45::GraphFile::GraphFile(const std::string& fn)
	: m_filename(fn), m_is(m_filename.c_str(), std::ifstream::binary), m_gr(m_is), m_refcount(1)
{
}

void CFMUAutoroute45::GraphFile::reference(void) const
{
        g_atomic_int_inc(&m_refcount);
}

void CFMUAutoroute45::GraphFile::unreference(void) const
{
        if (!g_atomic_int_dec_and_test(&m_refcount))
                return;
        delete this;
}

bool CFMUAutoroute45::GraphFile::sort_ptr_area::operator()(const ptr_t& p0, const ptr_t& p1) const
{
	if (!p1)
		return !!p0;
	if (!p0)
		return false;
	if (!p1->get_gr().is_valid())
		return p0->get_gr().is_valid();
	if (!p0->get_gr().is_valid())
		return false;
	return p0->get_gr().get_bbox().get_simple_area_km2_dbl() < p1->get_gr().get_bbox().get_simple_area_km2_dbl();
}

bool CFMUAutoroute45::lgraphbinload(const Rect& bbox)
{
	typedef std::vector<GraphFile::ptr_t> files_t;
	files_t files;
	{
		Glib::Dir dir(get_db_auxdir());
		for (;;) {
			std::string name(dir.read_name());
			if (name.empty())
				break;
			if (name.size() < 13)
				continue;
			if (name.substr(name.size()-4) != ".bin" ||
			    name.substr(0, 9) != "cfmugraph")
				continue;
			std::string fname(Glib::build_filename(get_db_auxdir(), name));
			if (!Glib::file_test(fname, Glib::FILE_TEST_EXISTS) || Glib::file_test(fname, Glib::FILE_TEST_IS_DIR))
				continue;
			GraphFile::ptr_t fp(new GraphFile(fname));
			if (!fp->get_is()) {
				std::ostringstream oss;
				oss << "cannot open file " << fp->get_filename();
				m_signal_log(log_normal, oss.str());
				continue;
			}
			try {
				fp->get_gr().header();
			} catch (const std::exception& e) {
				std::ostringstream oss;
				oss << "cannot read file header from " << fp->get_filename()
				    << ": " << e.what();
				m_signal_log(log_normal, oss.str());
				continue;
			}
			if (!fp->get_gr().is_valid()) {
				std::ostringstream oss;
				oss << "invalid file header from " << fp->get_filename();
				m_signal_log(log_normal, oss.str());
				continue;
			}
			if (!fp->get_gr().get_bbox().is_inside(bbox)) {
				std::ostringstream oss;
				const Rect& bbox1(fp->get_gr().get_bbox());
				oss << "file " << fp->get_filename() << " bbox ["
				    << bbox1.get_southwest().get_lat_str2() << ' ' << bbox1.get_southwest().get_lon_str2()
				    << ' ' << bbox1.get_northeast().get_lat_str2() << ' ' << bbox1.get_northeast().get_lon_str2()
				    << "] does not cover [" << bbox.get_southwest().get_lat_str2() << ' ' << bbox.get_southwest().get_lon_str2()
				    << ' ' << bbox.get_northeast().get_lat_str2() << ' ' << bbox.get_northeast().get_lon_str2()
				    << ']';
				m_signal_log(log_normal, oss.str());
				continue;
			}
			if (m_performance.get_minaltitude() < fp->get_gr().get_minlevel() * 100U ||
			    m_performance.get_maxaltitude() > fp->get_gr().get_maxlevel() * 100U) {
				std::ostringstream oss;
				oss << "file " << fp->get_filename()
				    << " F" << std::setfill('0') << std::setw(3) << fp->get_gr().get_minlevel()
				    << "..F" << std::setfill('0') << std::setw(3) << fp->get_gr().get_maxlevel()
				    << " does not cover " << m_performance.get_lowest_cruise().get_fplalt()
				    << ".." << m_performance.get_highest_cruise().get_fplalt();
				m_signal_log(log_normal, oss.str());
				continue;
			}
			files.push_back(fp);
		}
	}
	std::sort(files.begin(), files.end(), GraphFile::sort_ptr_area());
	for (files_t::const_iterator i(files.begin()), e(files.end()); i != e; ++i) {
		if (!*i)
			continue;
		std::ostringstream oss;
		const Rect& bbox1((*i)->get_gr().get_bbox());
		oss << "CFMU graph file " << (*i)->get_filename() << " bbox ["
		    << bbox1.get_southwest().get_lat_str2() << ' ' << bbox1.get_southwest().get_lon_str2()
		    << ' ' << bbox1.get_northeast().get_lat_str2() << ' ' << bbox1.get_northeast().get_lon_str2()
		    << "] F" << std::setfill('0') << std::setw(3) << (*i)->get_gr().get_minlevel()
		    << "..F" << std::setfill('0') << std::setw(3) << (*i)->get_gr().get_maxlevel()
		    << " area " << (*i)->get_gr().get_bbox().get_simple_area_km2_dbl()
		    << " area2 " << ((PolygonSimple)((*i)->get_gr().get_bbox())).get_simple_area_km2_dbl();
		if (!(*i)->get_gr().is_valid())
			oss << " (INVALID)";
		m_signal_log(log_normal, oss.str());
	}
	for (files_t::iterator i(files.begin()), e(files.end()); i != e; ++i) {
		if (!*i || !(*i)->get_gr().is_valid())
			break;
		try {
			lgraphbinload((*i)->get_gr(), bbox);
		} catch (const std::exception& e) {
			std::ostringstream oss;
			oss << "cannot load file " << (*i)->get_filename() << ": " << e.what();
			m_signal_log(log_normal, oss.str());
			continue;
		}
		m_signal_log(log_precompgraph, (*i)->get_filename());
		std::ostringstream oss;
		oss << "file " << (*i)->get_filename() << " successfully loaded ";
		m_signal_log(log_normal, oss.str());
		return true;
	}
	return false;
}
