#ifndef ADRAERODROME_H
#define ADRAERODROME_H

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "adr.hh"

namespace ADR {

class AirportTimeSlice : public ElevPointIdentTimeSlice {
public:
	AirportTimeSlice(timetype_t starttime = 0, timetype_t endtime = 0);

	virtual AirportTimeSlice& as_airport(void);
	virtual const AirportTimeSlice& as_airport(void) const;

	virtual void dependencies(UUID::set_t& dep) const;
	virtual void dependencies(Link::LinkSet& dep) const;

        const std::string& get_name(void) const { return m_name; }
	void set_name(const std::string& name) { m_name = name; }

        const std::string& get_iata(void) const { return m_iata; }
	void set_iata(const std::string& iata) { m_iata = iata; }
 
	typedef std::vector<std::string> servedcities_t;
	const servedcities_t& get_cities(void) const { return m_cities; }
	servedcities_t& get_cities(void) { return m_cities; }
	void set_cities(const servedcities_t& c) { m_cities = c; }

	static const uint8_t flag_civ    = 1 << 0;
	static const uint8_t flag_mil    = 1 << 1;
	static const uint8_t flag_depifr = 1 << 2;
	static const uint8_t flag_arrifr = 1 << 3;

	uint8_t get_flags(void) const { return m_flags; }
	void set_flags(uint8_t flags) { m_flags = flags; }
	void frob_flags(uint8_t v, uint8_t m) { m_flags = (m_flags & ~m) ^ v; }

	bool is_civ(void) const { return get_flags() & flag_civ; }
	void set_civ(bool v = true) { frob_flags(v ? flag_civ : 0, flag_civ); }
	bool is_mil(void) const { return get_flags() & flag_mil; }
	void set_mil(bool v = true) { frob_flags(v ? flag_mil : 0, flag_mil); }
	bool is_depifr(void) const { return get_flags() & flag_depifr; }
	void set_depifr(bool v = true) { frob_flags(v ? flag_depifr : 0, flag_depifr); }
	bool is_arrifr(void) const { return get_flags() & flag_arrifr; }
	void set_arrifr(bool v = true) { frob_flags(v ? flag_arrifr : 0, flag_arrifr); }

	virtual bool is_airport(void) const { return true; }
	static bool is_airport_static(void) { return true; }
	virtual bool get(uint64_t tm, AirportsDb::Airport& el) const;

	virtual std::ostream& print(std::ostream& os) const;

	template<class Archive> void hibernate(Archive& ar) {
		ElevPointIdentTimeSlice::hibernate(ar);
		ar.io(m_name);
		ar.io(m_iata);
		ar.io(m_cities);
		ar.io(m_flags);
	}

protected:
        std::string m_name;
        std::string m_iata;
        servedcities_t m_cities;
        uint8_t m_flags;
};

class Airport : public ObjectImpl<AirportTimeSlice,Object::type_airport> {
public:
	typedef Glib::RefPtr<Airport> ptr_t;
	typedef Glib::RefPtr<const Airport> const_ptr_t;
	typedef ObjectImpl<AirportTimeSlice,Object::type_airport> objbase_t;

	Airport(const UUID& uuid = UUID());

	ptr_t clone_obj(void) const { ptr_t p(new Airport(get_uuid())); clone_impl(p); return p; }
	virtual Object::ptr_t clone(void) const { return clone_obj(); }

	virtual type_t get_type(void) const;
};

class AirportCollocationTimeSlice : public TimeSlice {
public:
	AirportCollocationTimeSlice(timetype_t starttime = 0, timetype_t endtime = 0);

	virtual AirportCollocationTimeSlice& as_airportcollocation(void);
	virtual const AirportCollocationTimeSlice& as_airportcollocation(void) const;

	virtual void dependencies(UUID::set_t& dep) const;
	virtual void dependencies(Link::LinkSet& dep) const;

	const Link& get_host(void) const { return m_host; }
	void set_host(const Link& l) { m_host = l; }
	const Link& get_dep(void) const { return m_dep; }
	void set_dep(const Link& l) { m_dep = l; }

	virtual std::ostream& print(std::ostream& os) const;

	template<class Archive> void hibernate(Archive& ar) {
	        TimeSlice::hibernate(ar);
		ar.io(m_host);
		ar.io(m_dep);
	}

protected:
	Link m_host;
	Link m_dep;
};

class AirportCollocation : public ObjectImpl<AirportCollocationTimeSlice,Object::type_airportcollocation> {
public:
	typedef Glib::RefPtr<AirportCollocation> ptr_t;
	typedef Glib::RefPtr<const AirportCollocation> const_ptr_t;
	typedef ObjectImpl<AirportCollocationTimeSlice,Object::type_airportcollocation> objbase_t;

	AirportCollocation(const UUID& uuid = UUID());

	ptr_t clone_obj(void) const { ptr_t p(new AirportCollocation(get_uuid())); clone_impl(p); return p; }
	virtual Object::ptr_t clone(void) const { return clone_obj(); }
};

};

#endif /* ADRAERODROME_H */
