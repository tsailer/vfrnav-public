#ifndef ADRHIBERNATE_H
#define ADRHIBERNATE_H

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "sysdeps.h"
#include "adr.hh"

namespace ADR {

template <typename TS> class DependencyGenerator {
public:
	typedef TS set_t;
	DependencyGenerator(set_t& set) : m_set(set) {}
	static bool is_save(void) { return false; }
	static bool is_load(void) { return false; }
	static bool is_dbblob(void) { return false; }
	void io(const int8_t& x) {}
	void io(const int16_t& x) {}
	void io(const int32_t& x) {}
	void io(const int64_t& x) {}
	void io(const uint8_t& x) {}
	void io(const uint16_t& x) {}
	void io(const uint32_t& x) {}
	void io(const uint64_t& x) {}
	void io(const float& x) {}
	void io(const double& x) {}
	void io(const char& x) {}
	void io(const bool& x) {}
	void io(const std::string& x) {}
	void io(const Glib::ustring& x) {}
	void io(const Point& x) {}
	void io(const Rect& x) {}
	void io(const UUID& x) {}
	void io(const Link& x) { x.dependencies(m_set); }
	void io(const std::vector<std::string>& x) {}
	void io(const std::set<Link>& x) {
		for (std::set<Link>::const_iterator i(x.begin()), e(x.end()); i != e; ++i)
			i->dependencies(m_set);
	}
	template<typename T> void ioleb(const T& x) {}
	template<typename T> void ioint8(const T& x) {}
	template<typename T> void ioint16(const T& x) {}
	template<typename T> void ioint32(const T& x) {}
	template<typename T> void ioint64(const T& x) {}
	template<typename T> void iouint8(const T& x) {}
	template<typename T> void iouint16(const T& x) {}
	template<typename T> void iouint32(const T& x) {}
	template<typename T> void iouint64(const T& x) {}

protected:
	set_t& m_set;
};

class LinkLoader {
public:
	LinkLoader(Database& db, unsigned int depth) : m_db(db), m_depth(depth) {}
	static bool is_save(void) { return false; }
	static bool is_load(void) { return false; }
	static bool is_dbblob(void) { return false; }
	void io(int8_t& x) {}
	void io(int16_t& x) {}
	void io(int32_t& x) {}
	void io(int64_t& x) {}
	void io(uint8_t& x) {}
	void io(uint16_t& x) {}
	void io(uint32_t& x) {}
	void io(uint64_t& x) {}
	void io(float& x) {}
	void io(double& x) {}
	void io(char& x) {}
	void io(bool& x) {}
	void io(std::string& x) {}
	void io(Glib::ustring& x) {}
	void io(Point& x) {}
	void io(Rect& x) {}
	void io(UUID& x) {}
	void io(Link& x) {
		x.load(m_db);
		if (false)
			std::cerr << "Link Loading " << x << ": " << (x.get_obj() ? "ok" : "NOT ok")
				  << " addr 0x" << std::hex << (unsigned long long)&x << std::dec << " depth " << m_depth << std::endl;
		if (m_depth && x.get_obj()) {
			unsigned int nextdepth(m_depth);
			if (nextdepth != std::numeric_limits<unsigned int>::max())
				--nextdepth;
			x.get_obj()->link(m_db, nextdepth);
		}
	}
	void io(const std::vector<std::string>& x) {}
	void io(std::set<Link>& x) {
		for (std::set<Link>::iterator i(x.begin()), e(x.end()); i != e; ++i)
			io(const_cast<Link&>(*i));
	}
	template<typename T> void ioleb(const T& x) {}
	template<typename T> void ioint8(T& x) {}
	template<typename T> void ioint16(T& x) {}
	template<typename T> void ioint32(T& x) {}
	template<typename T> void ioint64(T& x) {}
	template<typename T> void iouint8(T& x) {}
	template<typename T> void iouint16(T& x) {}
	template<typename T> void iouint32(T& x) {}
	template<typename T> void iouint64(T& x) {}

protected:
        Database& m_db;
	unsigned int m_depth;
};

class ArchiveWriteStream {
public:
	ArchiveWriteStream(std::ostream& os) : m_os(os) {}
	static bool is_save(void) { return true; }
	static bool is_load(void) { return false; }
	static bool is_dbblob(void) { return true; }
	template <typename T> void io(const T& v);
	template <typename T> void ioleb(T x);
	template <typename T> inline void ioint8(T v) { int8_t vv(v); io(vv); }
	template <typename T> inline void ioint16(T v) { int16_t vv(v); io(vv); }
	template <typename T> inline void ioint32(T v) { int32_t vv(v); io(vv); }
	template <typename T> inline void ioint64(T v) { int64_t vv(v); io(vv); }
	template <typename T> inline void iouint8(T v) { uint8_t vv(v); io(vv); }
	template <typename T> inline void iouint16(T v) { uint16_t vv(v); io(vv); }
	template <typename T> inline void iouint32(T v) { uint32_t vv(v); io(vv); }
	template <typename T> inline void iouint64(T v) { uint64_t vv(v); io(vv); }

protected:
	std::ostream& m_os;
};

class ArchiveReadStream {
public:
	ArchiveReadStream(std::istream& is) : m_is(is) {}
	static bool is_save(void) { return false; }
	static bool is_load(void) { return true; }
	static bool is_dbblob(void) { return true; }
	template <typename T> void io(T& v);
	template <typename T> void ioleb(T& x);
	template <typename T> inline void ioint8(T& v) { int8_t vv; io(vv); v = (T)vv; }
	template <typename T> inline void ioint16(T& v) { int16_t vv; io(vv); v = (T)vv; }
	template <typename T> inline void ioint32(T& v) { int32_t vv; io(vv); v = (T)vv; }
	template <typename T> inline void ioint64(T& v) { int64_t vv; io(vv); v = (T)vv; }
	template <typename T> inline void iouint8(T& v) { uint8_t vv; io(vv); v = (T)vv; }
	template <typename T> inline void iouint16(T& v) { uint16_t vv; io(vv); v = (T)vv; }
	template <typename T> inline void iouint32(T& v) { uint32_t vv; io(vv); v = (T)vv; }
	template <typename T> inline void iouint64(T& v) { uint64_t vv; io(vv); v = (T)vv; }

protected:
	std::istream& m_is;
};

class ArchiveReadBuffer {
public:
	ArchiveReadBuffer(const uint8_t *ptr, const uint8_t *end) : m_ptr(ptr), m_end(end) {}
	static bool is_save(void) { return false; }
	static bool is_load(void) { return true; }
	static bool is_dbblob(void) { return true; }
	template <typename T> void io(T& v);
	template <typename T> void ioleb(T& x);
	template <typename T> inline void ioint8(T& v) { int8_t vv; io(vv); v = (T)vv; }
	template <typename T> inline void ioint16(T& v) { int16_t vv; io(vv); v = (T)vv; }
	template <typename T> inline void ioint32(T& v) { int32_t vv; io(vv); v = (T)vv; }
	template <typename T> inline void ioint64(T& v) { int64_t vv; io(vv); v = (T)vv; }
	template <typename T> inline void iouint8(T& v) { uint8_t vv; io(vv); v = (T)vv; }
	template <typename T> inline void iouint16(T& v) { uint16_t vv; io(vv); v = (T)vv; }
	template <typename T> inline void iouint32(T& v) { uint32_t vv; io(vv); v = (T)vv; }
	template <typename T> inline void iouint64(T& v) { uint64_t vv; io(vv); v = (T)vv; }

protected:
	const uint8_t *m_ptr;
	const uint8_t *m_end;
};


};

#endif /* ADRHIBERNATE_H */
