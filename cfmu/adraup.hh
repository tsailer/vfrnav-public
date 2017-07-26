#ifndef ADRAUP_H
#define ADRAUP_H

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "adr.hh"
#include "adrairspace.hh"

namespace ADR {

class ADRAUPObjectBase {
public:
	typedef Glib::RefPtr<ADRAUPObjectBase> ptr_t;
	typedef Glib::RefPtr<const ADRAUPObjectBase> const_ptr_t;

	ADRAUPObjectBase(const Link& obj = Link(), timetype_t starttime = 0, timetype_t endtime = 0);
	virtual ~ADRAUPObjectBase();
	ptr_t clone_obj(void) const;
	virtual ptr_t clone(void) const { return clone_obj(); }

	void reference(void) const;
	void unreference(void) const;
	gint get_refcount(void) const { return m_refcount; }
	ptr_t get_ptr(void) { reference(); return ptr_t(this); }
	const_ptr_t get_ptr(void) const { reference(); return const_ptr_t(this); }

	timetype_t get_starttime(void) const { return m_time.first; }
	timetype_t get_endtime(void) const { return m_time.second; }
	const timepair_t& get_timeinterval(void) const { return m_time; }
	void set_starttime(timetype_t t) { m_time.first = t; }
	void set_endtime(timetype_t t) { m_time.second = t; }
	bool is_valid(void) const { return get_starttime() < get_endtime(); }
	bool is_inside(timetype_t tm) const { return get_starttime() <= tm && tm < get_endtime(); }
	bool is_overlap(timetype_t tm0, timetype_t tm1) const;
	bool is_overlap(const TimeSlice& ts) const { return is_overlap(ts.get_starttime(), ts.get_endtime()); }
	bool is_overlap(const timepair_t& tm) const { return is_overlap(tm.first, tm.second); }
	bool is_overlap(const const_ptr_t& p) const { return p && is_overlap(p->get_starttime(), p->get_endtime()); }
	timetype_t get_overlap(timetype_t tm0, timetype_t tm1) const;
	timetype_t get_overlap(const TimeSlice& ts) const { return get_overlap(ts.get_starttime(), ts.get_endtime()); }
	timetype_t get_overlap(const timepair_t& tm) const { return get_overlap(tm.first, tm.second); }
	timetype_t get_overlap(const const_ptr_t& p) const { return p ? get_overlap(p->get_starttime(), p->get_endtime()) : 0; }

	const Link& get_obj(void) const { return m_obj; }
	void set_obj(const Link& obj) { m_obj = obj; }

	virtual void link(Database& db, unsigned int depth = 0);
	virtual void dependencies(UUID::set_t& dep) const;
	virtual void dependencies(Link::LinkSet& dep) const;
	virtual bool is_unlinked(void) const;
	virtual timeset_t timediscontinuities(void) const;

	int compare(const ADRAUPObjectBase& x) const;
	bool operator==(const ADRAUPObjectBase& x) const { return compare(x) == 0; }
	bool operator!=(const ADRAUPObjectBase& x) const { return compare(x) != 0; }
	bool operator<(const ADRAUPObjectBase& x) const { return compare(x) < 0; }
	bool operator<=(const ADRAUPObjectBase& x) const { return compare(x) <= 0; }
	bool operator>(const ADRAUPObjectBase& x) const { return compare(x) > 0; }
	bool operator>=(const ADRAUPObjectBase& x) const { return compare(x) >= 0; }

	typedef enum {
		type_cdr,
		type_rsa,
		type_invalid
	} type_t;
	virtual type_t get_type(void) const { return type_invalid; }

	static ptr_t create(type_t t = type_invalid, const Link& obj = Link(), timetype_t starttime = 0, timetype_t endtime = 0);
	static ptr_t create(ArchiveReadStream& ar, const Link& obj = Link(), timetype_t starttime = 0, timetype_t endtime = 0);
	static ptr_t create(ArchiveReadBuffer& ar, const Link& obj = Link(), timetype_t starttime = 0, timetype_t endtime = 0);
	virtual void load(ArchiveReadStream& ar);
	virtual void load(ArchiveReadBuffer& ar);
	virtual void save(ArchiveWriteStream& ar) const;

	virtual std::ostream& print(std::ostream& os) const;

	template<class Archive> void hibernate(Archive& ar) {
		if (!ar.is_dbblob()) {
			ar.io(m_obj);
			ar.iouint64(m_time.first);
			ar.iouint64(m_time.second);
		}
	}

protected:
	Link m_obj;
	timepair_t m_time;
	mutable gint m_refcount;
};

class AUPRSA : public ADRAUPObjectBase {
public:
	class Activation {
	public:
		typedef enum {
			status_active,
			status_invalid
		} status_t;

		Activation(void) : m_status(status_invalid) {}

		const AltRange& get_altrange(void) const { return m_altrange; }
		AltRange& get_altrange(void) { return m_altrange; }
		void set_altrange(const AltRange& ar) { m_altrange = ar; }

		typedef std::vector<Link> hostairspaces_t;
		const hostairspaces_t& get_hostairspaces(void) const { return m_hostairspaces; }
		hostairspaces_t& get_hostairspaces(void) { return m_hostairspaces; }
		void set_hostairspaces(const hostairspaces_t& ar) { m_hostairspaces = ar; }

		status_t get_status(void) const { return m_status; }
		void set_status(status_t s) { m_status = s; }

		std::ostream& print(std::ostream& os, const timepair_t& tp) const;

		template<class Archive> void hibernate(Archive& ar) {
			m_altrange.hibernate(ar);
			uint32_t sz(m_hostairspaces.size());
			ar.ioleb(sz);
			if (ar.is_load())
				m_hostairspaces.resize(sz);
			for (typename hostairspaces_t::iterator i(m_hostairspaces.begin()), e(m_hostairspaces.end()); i != e; ++i)
				i->hibernate(ar);
			ar.iouint8(m_status);
		}

	protected:
		AltRange m_altrange;
		hostairspaces_t m_hostairspaces;
		status_t m_status;
	};

	typedef Glib::RefPtr<AUPRSA> ptr_t;
	typedef Glib::RefPtr<const AUPRSA> const_ptr_t;
	typedef AirspaceTimeSlice::type_t airspacetype_t;

	AUPRSA(const Link& obj = Link(), timetype_t starttime = 0, timetype_t endtime = 0);
	ptr_t clone_obj(void) const;
	virtual ADRAUPObjectBase::ptr_t clone(void) const { return clone_obj(); }

	virtual type_t get_type(void) const { return type_rsa; }

	virtual void load(ArchiveReadStream& ar);
	virtual void load(ArchiveReadBuffer& ar);
	virtual void save(ArchiveWriteStream& ar) const;

	const Activation& get_activation(void) const { return m_activation; }
	Activation& get_activation(void) { return m_activation; }
	void set_activation(const Activation& ar) { m_activation = ar; }

	airspacetype_t get_airspacetype(void) const { return m_airspacetype; }
	void set_airspacetype(airspacetype_t t) { m_airspacetype = t; }

	bool is_icao(void) const { return !!(m_flags & 0x04); }
	void set_icao(bool icao = true) { if (icao) m_flags |= 0x04; else m_flags &= ~0x04; }

	bool is_level(unsigned int l) const { return !!(m_flags & 0x70 & (0x08 << l)); }
	void set_level(unsigned int l, bool x = true) { m_flags ^= (m_flags ^ -!!x) & 0x70 & (0x08 << l); }

	virtual std::ostream& print(std::ostream& os) const;

	template<class Archive> void hibernate(Archive& ar) {
		ADRAUPObjectBase::hibernate(ar);
	        m_activation.hibernate(ar);
		ar.iouint8(m_airspacetype);
		ar.iouint8(m_flags);
	}

protected:
	Activation m_activation;
	airspacetype_t m_airspacetype;
	uint8_t m_flags;
};

class AUPCDR : public ADRAUPObjectBase {
public:
	class Availability {
	public:
		Availability(void) : m_flags(0) {}

		const AltRange& get_altrange(void) const { return m_altrange; }
		AltRange& get_altrange(void) { return m_altrange; }
		void set_altrange(const AltRange& ar) { m_altrange = ar; }

		typedef std::vector<Link> hostairspaces_t;
		const hostairspaces_t& get_hostairspaces(void) const { return m_hostairspaces; }
		hostairspaces_t& get_hostairspaces(void) { return m_hostairspaces; }
		void set_hostairspaces(const hostairspaces_t& ar) { m_hostairspaces = ar; }

		unsigned int get_cdr(void) const { return ((m_flags & 0x0C) >> 2); }
		void set_cdr(unsigned int c) { m_flags ^= (m_flags ^ (c << 2)) & 0x0C; }

		bool is_forward(void) const { return !(m_flags & 0x10); }
		void set_forward(bool fwd) { if (fwd) m_flags &= ~0x10; else m_flags |= 0x10; }
		bool is_backward(void) const { return !!(m_flags & 0x10); }
		void set_backward(bool bwd) { set_forward(!bwd); }

		std::ostream& print(std::ostream& os, const timepair_t& tp) const;

		template<class Archive> void hibernate(Archive& ar) {
			m_altrange.hibernate(ar);
			uint32_t sz(m_hostairspaces.size());
			ar.ioleb(sz);
			if (ar.is_load())
				m_hostairspaces.resize(sz);
			for (typename hostairspaces_t::iterator i(m_hostairspaces.begin()), e(m_hostairspaces.end()); i != e; ++i)
				i->hibernate(ar);
			ar.iouint8(m_flags);
		}

	protected:
		AltRange m_altrange;
		hostairspaces_t m_hostairspaces;
		uint8_t m_flags;
	};

	typedef Glib::RefPtr<AUPCDR> ptr_t;
	typedef Glib::RefPtr<const AUPCDR> const_ptr_t;

	AUPCDR(const Link& obj = Link(), timetype_t starttime = 0, timetype_t endtime = 0);
	ptr_t clone_obj(void) const;
	virtual ADRAUPObjectBase::ptr_t clone(void) const { return clone_obj(); }

	virtual type_t get_type(void) const { return type_cdr; }

	virtual void load(ArchiveReadStream& ar);
	virtual void load(ArchiveReadBuffer& ar);
	virtual void save(ArchiveWriteStream& ar) const;

	typedef std::vector<Availability> availability_t;
	const availability_t& get_availability(void) const { return m_availability; }
	availability_t& get_availability(void) { return m_availability; }
	void set_availability(const availability_t& a) { m_availability = a; }

	void add_availability(const Availability& a);
	void add_availability(const availability_t& a);

	virtual std::ostream& print(std::ostream& os) const;

	template<class Archive> void hibernate(Archive& ar) {
		ADRAUPObjectBase::hibernate(ar);
		uint32_t sz(m_availability.size());
		ar.ioleb(sz);
		if (ar.is_load())
			m_availability.resize(sz);
		for (typename availability_t::iterator i(m_availability.begin()), e(m_availability.end()); i != e; ++i)
			i->hibernate(ar);
	}

protected:
	availability_t m_availability;
};

class AUPDatabase {
public:
	AUPDatabase(const std::string& path = "");
	~AUPDatabase(void);
	void set_path(const std::string& path = "");
	ADRAUPObjectBase::const_ptr_t load(const UUID& uuid);
	void save(const ADRAUPObjectBase::const_ptr_t& p);
	void erase(const ADRAUPObjectBase::const_ptr_t& p);
	void erase(const ADRAUPObjectBase::ptr_t& p) { erase(ADRAUPObjectBase::const_ptr_t(p)); }
	void erase(timetype_t tmin = std::numeric_limits<timetype_t>::min(),
		   timetype_t tmax = std::numeric_limits<timetype_t>::max());
	void sync_off(void);
	void analyze(void);
	void vacuum(void);
	void set_wal(bool wal);
	void set_exclusive(bool excl);

	typedef std::vector<ADRAUPObjectBase::const_ptr_t> findresults_t;

	findresults_t find(const UUID& uuid,
			   timetype_t tmin = std::numeric_limits<timetype_t>::min(),
			   timetype_t tmax = std::numeric_limits<timetype_t>::max());

	findresults_t find(timetype_t tmin = std::numeric_limits<timetype_t>::min(),
			   timetype_t tmax = std::numeric_limits<timetype_t>::max());

protected:
	std::string m_path;
	sqlite3x::sqlite3_connection m_db;

	void open(void);
	void close(void);

	void save_helper(const ADRAUPObjectBase::const_ptr_t& p);
	findresults_t find_tail(sqlite3x::sqlite3_command& cmd);
};

class ConditionalAvailability {
public:
	void clear(void);
	void load(Database& db, AUPDatabase& aupdb, timetype_t starttime, timetype_t endtime);
	const ADRAUPObjectBase::const_ptr_t& find(const UUID& uuid, timetype_t tm) const;
	typedef std::vector<ADRAUPObjectBase::const_ptr_t> results_t;
	results_t find(const UUID& uuid, timetype_t tstart, timetype_t tend) const;

protected:
	class AUPObjectSorter {
	public:
		bool operator()(const ADRAUPObjectBase& a, const ADRAUPObjectBase& b) const { return a.compare(b) < 0; }
		bool operator()(const ADRAUPObjectBase::const_ptr_t& a, const ADRAUPObjectBase::const_ptr_t& b) const {
			if (!b)
				return false;
			if (!a)
				return true;
			return a->compare(*b.operator->()) < 0;
		}
	};
	typedef std::set<ADRAUPObjectBase::const_ptr_t, AUPObjectSorter> objects_t;
	objects_t m_objects;
};

};

const std::string& to_str(ADR::ADRAUPObjectBase::type_t t);
inline std::ostream& operator<<(std::ostream& os, ADR::ADRAUPObjectBase::type_t t) { return os << to_str(t); }
const std::string& to_str(ADR::AUPRSA::Activation::status_t s);
inline std::ostream& operator<<(std::ostream& os, ADR::AUPRSA::Activation::status_t s) { return os << to_str(s); }

#endif /* ADRAUP_H */
