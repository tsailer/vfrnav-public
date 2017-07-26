#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <boost/type_traits.hpp>
#include <iomanip>
#include <sstream>

#include "adr.hh"
#include "adrhibernate.hh"

using namespace ADR;

namespace ADR {

template class DependencyGenerator<UUID::set_t>;
template class DependencyGenerator<Link::LinkSet>;

template <> void ArchiveWriteStream::io<int8_t>(const int8_t& v)
{
        m_os.write(reinterpret_cast<const char *>(&v), sizeof(v));
        if (!m_os)
                throw std::runtime_error("io<int8_t>: write error");	
}

template <> void ArchiveWriteStream::io<uint8_t>(const uint8_t& v)
{
        m_os.write(reinterpret_cast<const char *>(&v), sizeof(v));
        if (!m_os)
                throw std::runtime_error("io<uint8_t>: write error");	
}

template <> void ArchiveWriteStream::io<int16_t>(const int16_t& v)
{
        uint8_t buf[2];
        buf[0] = v;
        buf[1] = v >> 8;
        m_os.write(reinterpret_cast<char *>(buf), sizeof(buf));
        if (!m_os)
                throw std::runtime_error("io<int16_t>: write error");	
}

template <> void ArchiveWriteStream::io<uint16_t>(const uint16_t& v)
{
        uint8_t buf[2];
        buf[0] = v;
        buf[1] = v >> 8;
        m_os.write(reinterpret_cast<char *>(buf), sizeof(buf));
        if (!m_os)
                throw std::runtime_error("io<uint16_t>: write error");	
}

template <> void ArchiveWriteStream::io<int32_t>(const int32_t& v)
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

template <> void ArchiveWriteStream::io<uint32_t>(const uint32_t& v)
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

template <> void ArchiveWriteStream::io<int64_t>(const int64_t& v)
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

template <> void ArchiveWriteStream::io<uint64_t>(const uint64_t& v)
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

template <> void ArchiveWriteStream::io<float>(const float& v)
{
        union {
                float f;
                uint32_t u;
        } x;
        x.f = v;
	io(x.u);
}

template <> void ArchiveWriteStream::io<double>(const double& v)
{
	union {
                double d;
                uint64_t u;
        } x;
        x.d = v;
	io(x.u);
}

template <> void ArchiveWriteStream::io<std::string>(const std::string& v)
{
        uint32_t n(v.size());
	ioleb(n);
        m_os.write(v.c_str(), v.size());
        if (!m_os)
                throw std::runtime_error("io<std::string>: write error");
}

template <> void ArchiveWriteStream::io<Glib::ustring>(const Glib::ustring& v)
{
	uint32_t n(v.size());
	ioleb(n);
        m_os.write(v.c_str(), v.size());
        if (!m_os)
                throw std::runtime_error("io<Glib::ustring>: write error");
}

template <> void ArchiveWriteStream::io<char>(const char& v)
{
	ioint8(v);
}

template <> void ArchiveWriteStream::io<bool>(const bool& v)
{
	uint8_t vv(!!v);
	io(vv);
}

template <> void ArchiveWriteStream::io<Point>(const Point& v)
{
	Point::coord_t vv(v.get_lat());
	io(vv);
	vv = v.get_lon();
	io(vv);
}

template <> void ArchiveWriteStream::io<Rect>(const Rect& v)
{
	io(v.get_southwest());
	io(v.get_northeast());
}

template <> void ArchiveWriteStream::io<UUID>(const UUID& v)
{
	const_cast<UUID&>(v).hibernate(*this);
}

template <> void ArchiveWriteStream::io<Link>(const Link& v)
{
	const_cast<Link&>(v).hibernate(*this);
}

template <> void ArchiveWriteStream::io<std::vector<std::string> >(const std::vector<std::string>& v)
{
	uint32_t n(v.size());
	ioleb(n);
	for (std::vector<std::string>::const_iterator i(v.begin()), e(v.end()); i != e; ++i)
		io(*i);
}

template <> void ArchiveWriteStream::io<std::set<Link> >(const std::set<Link>& x)
{
	uint32_t sz(x.size());
	ioleb(sz);
	for (std::set<Link>::iterator i(x.begin()), e(x.end()); i != e && sz > 0; ++i, --sz)
		io(*i);
}

template <typename T> void ArchiveWriteStream::ioleb(T x)
{
        uint8_t buf[(sizeof(T) * 8 + 6) / 7 + 2];
	unsigned int bp(0);
	if (boost::is_signed<T>::value) {
		uint8_t b;
		do {
			b = x & 0x7F;
			x >>= 7;
			if ((x != 0 && !(b & 0x40)) || (x != -1 && (b & 0x40)))
				b |= 0x80;
			buf[bp++] = b;
		} while (b & 0x80);
	} else {
		do {
			uint8_t b(x);
			x >>= 7;
			if (x)
				b |= 0x80;
			buf[bp++] = b;
		} while (x);
	}
	m_os.write(reinterpret_cast<char *>(buf), bp);
        if (!m_os)
                throw std::runtime_error("ioleb: write error");	
}

template <> void ArchiveReadStream::io<int8_t>(int8_t& v)
{
        m_is.read(reinterpret_cast<char *>(&v), sizeof(v));
        if (!m_is)
                throw std::runtime_error("io<int8_t>: short read");
}

template <> void ArchiveReadStream::io<uint8_t>(uint8_t& v)
{
        m_is.read(reinterpret_cast<char *>(&v), sizeof(v));
        if (!m_is)
                throw std::runtime_error("io<uint8_t>: short read");
}

template <> void ArchiveReadStream::io<int16_t>(int16_t& v)
{
        uint8_t buf[2];
        m_is.read(reinterpret_cast<char *>(buf), sizeof(buf));
        if (!m_is)
                throw std::runtime_error("io<int16_t>: short read");
        v = buf[0] | (buf[1] << 8);
}

template <> void ArchiveReadStream::io<uint16_t>(uint16_t& v)
{
        uint8_t buf[2];
        m_is.read(reinterpret_cast<char *>(buf), sizeof(buf));
        if (!m_is)
                throw std::runtime_error("io<uint16_t>: short read");
        v = buf[0] | (buf[1] << 8);
}

template <> void ArchiveReadStream::io<int32_t>(int32_t& v)
{
        uint8_t buf[4];
        m_is.read(reinterpret_cast<char *>(buf), sizeof(buf));
        if (!m_is)
                throw std::runtime_error("io<int32_t>: short read");
        v = buf[0] | (buf[1] << 8) | (buf[2] << 16) | (buf[3] << 24);
}

template <> void ArchiveReadStream::io<uint32_t>(uint32_t& v)
{
        uint8_t buf[4];
        m_is.read(reinterpret_cast<char *>(buf), sizeof(buf));
        if (!m_is)
                throw std::runtime_error("io<uint32_t>: short read");
        v = buf[0] | (buf[1] << 8) | (buf[2] << 16) | (buf[3] << 24);
}

template <> void ArchiveReadStream::io<int64_t>(int64_t& v)
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

template <> void ArchiveReadStream::io<uint64_t>(uint64_t& v)
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

template <> void ArchiveReadStream::io<float>(float& v)
{
        union {
                float f;
                uint32_t u;
        } x;
	io(x.u);
        v = x.f;
}

template <> void ArchiveReadStream::io<double>(double& v)
{
	union {
                double d;
                uint64_t u;
        } x;
	io(x.u);
        v = x.d;
}

template <> void ArchiveReadStream::io<std::string>(std::string& v)
{
	v.clear();
        uint32_t len(0);
	ioleb(len);
        if (!len)
                return;
        char buf[len];
        m_is.read(buf, len);
        if (!m_is)
                throw std::runtime_error("io<std::string>: short read");
        v = std::string(buf, buf + len);
}

template <> void ArchiveReadStream::io<Glib::ustring>(Glib::ustring& v)
{
	v.clear();
        uint32_t len(0);
	ioleb(len);
        if (!len)
                return;
        char buf[len];
        m_is.read(buf, len);
        if (!m_is)
                throw std::runtime_error("io<Glib::ustring>: short read");
        v = std::string(buf, buf + len);
}

template <> void ArchiveReadStream::io<char>(char& v)
{
	ioint8(v);
}

template <> void ArchiveReadStream::io<bool>(bool& v)
{
	uint8_t vv(0);
	io(vv);
	v = !!vv;
}

template <> void ArchiveReadStream::io<Point>(Point& v)
{
	Point::coord_t vv(0);
	io(vv);
	v.set_lat(vv);
	vv = 0;
	io(vv);
	v.set_lon(vv);
}

template <> void ArchiveReadStream::io<Rect>(Rect& v)
{
	Point ptsw;
	ptsw.set_invalid();
	io(ptsw);
	Point ptne;
	ptne.set_invalid();
	io(ptne);
	v = Rect(ptsw, ptne);
}

template <> void ArchiveReadStream::io<UUID>(UUID& v)
{
	v.hibernate(*this);
}

template <> void ArchiveReadStream::io<Link>(Link& v)
{
	UUID u;
	u.hibernate(*this);
	v = Link(u);
}

template <> void ArchiveReadStream::io<std::vector<std::string> >(std::vector<std::string>& v)
{
	uint32_t n(0);
	ioleb(n);
	v.resize(n);
	for (std::vector<std::string>::iterator i(v.begin()), e(v.end()); i != e; ++i)
		io(*i);
}

template <> void ArchiveReadStream::io<std::set<Link> >(std::set<Link>& x)
{
	uint32_t sz(0);
	ioleb(sz);
	x.clear();
	for (; sz; --sz) {
		UUID uuid;
		io(uuid);
		x.insert(uuid);
	}
}

template <typename T> void ArchiveReadStream::ioleb(T& x)
{
	T y(0);
	unsigned int sh(0);
	uint8_t b;
	do {
		iouint8(b);
		y |= ((T)(b & 0x7F)) << sh;
		sh += 7;
	} while (b & 0x80);
	if (boost::is_signed<T>::value && (sh < sizeof(T) * CHAR_BIT) && (b & 0x40))
		y |= -(((T)1) << sh);
	x = y;
}

template <> void ArchiveReadBuffer::io<int8_t>(int8_t& v)
{
        if (!m_ptr || m_ptr >= m_end)
                throw std::runtime_error("io<int8_t>: short read");
	v = *m_ptr++;
}

template <> void ArchiveReadBuffer::io<uint8_t>(uint8_t& v)
{
        if (!m_ptr || m_ptr >= m_end)
                throw std::runtime_error("io<uint8_t>: short read");
	v = *m_ptr++;
}

template <> void ArchiveReadBuffer::io<int16_t>(int16_t& v)
{
	if (!m_ptr || m_ptr + 2 > m_end)
               throw std::runtime_error("io<int16_t>: short read");
        v = m_ptr[0] | (m_ptr[1] << 8);
	m_ptr += 2;
}

template <> void ArchiveReadBuffer::io<uint16_t>(uint16_t& v)
{
	if (!m_ptr || m_ptr + 2 > m_end)
               throw std::runtime_error("io<uint16_t>: short read");
        v = m_ptr[0] | (m_ptr[1] << 8);
	m_ptr += 2;
}

template <> void ArchiveReadBuffer::io<int32_t>(int32_t& v)
{
	if (!m_ptr || m_ptr + 4 > m_end)
               throw std::runtime_error("io<int32_t>: short read");
        v = m_ptr[0] | (m_ptr[1] << 8) | (m_ptr[2] << 16) | (m_ptr[3] << 24);
	m_ptr += 4;
}

template <> void ArchiveReadBuffer::io<uint32_t>(uint32_t& v)
{
	if (!m_ptr || m_ptr + 4 > m_end)
		throw std::runtime_error("io<uint32_t>: short read");
        v = m_ptr[0] | (m_ptr[1] << 8) | (m_ptr[2] << 16) | (m_ptr[3] << 24);
	m_ptr += 4;
}

template <> void ArchiveReadBuffer::io<int64_t>(int64_t& v)
{
   	if (!m_ptr || m_ptr + 8 > m_end)
                throw std::runtime_error("io<int64_t>: short read");
	uint32_t r1(m_ptr[4] | (m_ptr[5] << 8) | (m_ptr[6] << 16) | (m_ptr[7] << 24));
        uint64_t r(r1);
        r <<= 32;
	r1 = m_ptr[0] | (m_ptr[1] << 8) | (m_ptr[2] << 16) | (m_ptr[3] << 24);
        r |= r1;
        v = r;
	m_ptr += 8;
}

template <> void ArchiveReadBuffer::io<uint64_t>(uint64_t& v)
{
   	if (!m_ptr || m_ptr + 8 > m_end)
                throw std::runtime_error("io<uint64_t>: short read");
	uint32_t r1(m_ptr[4] | (m_ptr[5] << 8) | (m_ptr[6] << 16) | (m_ptr[7] << 24));
        uint64_t r(r1);
        r <<= 32;
	r1 = m_ptr[0] | (m_ptr[1] << 8) | (m_ptr[2] << 16) | (m_ptr[3] << 24);
        r |= r1;
        v = r;
	m_ptr += 8;
}

template <> void ArchiveReadBuffer::io<float>(float& v)
{
        union {
                float f;
                uint32_t u;
        } x;
	io(x.u);
        v = x.f;
}

template <> void ArchiveReadBuffer::io<double>(double& v)
{
	union {
                double d;
                uint64_t u;
        } x;
	io(x.u);
        v = x.d;
}

template <> void ArchiveReadBuffer::io<std::string>(std::string& v)
{
	v.clear();
        uint32_t len(0);
	ioleb(len);
        if (!len)
                return;
   	if (!m_ptr || m_ptr + len > m_end)
                throw std::runtime_error("io<std::string>: short read");
        v = std::string(m_ptr, m_ptr + len);
	m_ptr += len;
}

template <> void ArchiveReadBuffer::io<Glib::ustring>(Glib::ustring& v)
{
	v.clear();
        uint32_t len(0);
	ioleb(len);
        if (!len)
                return;
	if (!m_ptr || m_ptr + len > m_end)
		throw std::runtime_error("io<Glib::ustring>: short read");
        v = std::string(m_ptr, m_ptr + len);
	m_ptr += len;
}

template <> void ArchiveReadBuffer::io<char>(char& v)
{
	ioint8(v);
}

template <> void ArchiveReadBuffer::io<bool>(bool& v)
{
	uint8_t vv(0);
	io(vv);
	v = !!vv;
}

template <> void ArchiveReadBuffer::io<Point>(Point& v)
{
	Point::coord_t vv(0);
	io(vv);
	v.set_lat(vv);
	vv = 0;
	io(vv);
	v.set_lon(vv);
}

template <> void ArchiveReadBuffer::io<Rect>(Rect& v)
{
	Point ptsw;
	ptsw.set_invalid();
	io(ptsw);
	Point ptne;
	ptne.set_invalid();
	io(ptne);
	v = Rect(ptsw, ptne);
}

template <> void ArchiveReadBuffer::io<UUID>(UUID& v)
{
	v.hibernate(*this);
}

template <> void ArchiveReadBuffer::io<Link>(Link& v)
{
	UUID u;
	u.hibernate(*this);
	v = Link(u);
}

template <> void ArchiveReadBuffer::io<std::vector<std::string> >(std::vector<std::string>& v)
{
	uint32_t n(0);
	ioleb(n);
	v.resize(n);
	for (std::vector<std::string>::iterator i(v.begin()), e(v.end()); i != e; ++i)
		io(*i);
}

template <> void ArchiveReadBuffer::io<std::set<Link> >(std::set<Link>& x)
{
	uint32_t sz(0);
	ioleb(sz);
	x.clear();
	for (; sz; --sz) {
		UUID uuid;
		io(uuid);
		x.insert(uuid);
	}
}

template <typename T> void ArchiveReadBuffer::ioleb(T& x)
{
	T y(0);
	unsigned int sh(0);
	uint8_t b;
	do {
		iouint8(b);
		y |= ((T)(b & 0x7F)) << sh;
		sh += 7;
	} while (b & 0x80);
	if (boost::is_signed<T>::value && (sh < sizeof(T) * CHAR_BIT) && (b & 0x40))
		y |= -(((T)1) << sh);
	x = y;
}

template void ArchiveWriteStream::ioleb(int16_t x);
template void ArchiveWriteStream::ioleb(int32_t x);
template void ArchiveWriteStream::ioleb(int64_t x);
template void ArchiveWriteStream::ioleb(uint16_t x);
template void ArchiveWriteStream::ioleb(uint32_t x);
template void ArchiveWriteStream::ioleb(uint64_t x);
template void ArchiveReadStream::ioleb(int16_t& x);
template void ArchiveReadStream::ioleb(int32_t& x);
template void ArchiveReadStream::ioleb(int64_t& x);
template void ArchiveReadStream::ioleb(uint16_t& x);
template void ArchiveReadStream::ioleb(uint32_t& x);
template void ArchiveReadStream::ioleb(uint64_t& x);
template void ArchiveReadBuffer::ioleb(int16_t& x);
template void ArchiveReadBuffer::ioleb(int32_t& x);
template void ArchiveReadBuffer::ioleb(int64_t& x);
template void ArchiveReadBuffer::ioleb(uint16_t& x);
template void ArchiveReadBuffer::ioleb(uint32_t& x);
template void ArchiveReadBuffer::ioleb(uint64_t& x);


};
