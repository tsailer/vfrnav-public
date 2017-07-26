#ifndef ADRUNIT_H
#define ADRUNIT_H

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "adr.hh"

namespace ADR {

class OrganisationAuthorityTimeSlice : public IdentTimeSlice {
public:
	OrganisationAuthorityTimeSlice(timetype_t starttime = 0, timetype_t endtime = 0);

	virtual OrganisationAuthorityTimeSlice& as_organisationauthority(void);
	virtual const OrganisationAuthorityTimeSlice& as_organisationauthority(void) const;

	virtual void dependencies(UUID::set_t& dep) const;
	virtual void dependencies(Link::LinkSet& dep) const;

	const Link& get_border(void) const { return m_border; }
	void set_border(const Link& l) { m_border = l; }

        const std::string& get_name(void) const { return m_name; }
	void set_name(const std::string& name) { m_name = name; }

	virtual std::ostream& print(std::ostream& os) const;

	template<class Archive> void hibernate(Archive& ar) {
		IdentTimeSlice::hibernate(ar);
		ar.io(m_border);
		ar.io(m_name);
	}

protected:
	Link m_border;
        std::string m_name;
};

class OrganisationAuthority : public ObjectImpl<OrganisationAuthorityTimeSlice,Object::type_organisationauthority> {
public:
	typedef Glib::RefPtr<OrganisationAuthority> ptr_t;
	typedef Glib::RefPtr<const OrganisationAuthority> const_ptr_t;
	typedef ObjectImpl<OrganisationAuthorityTimeSlice,Object::type_organisationauthority> objbase_t;

	OrganisationAuthority(const UUID& uuid = UUID());

	ptr_t clone_obj(void) const { ptr_t p(new OrganisationAuthority(get_uuid())); clone_impl(p); return p; }
	virtual Object::ptr_t clone(void) const { return clone_obj(); }
};

class AirTrafficManagementServiceTimeSlice : public TimeSlice {
public:
	AirTrafficManagementServiceTimeSlice(timetype_t starttime = 0, timetype_t endtime = 0);

	virtual AirTrafficManagementServiceTimeSlice& as_airtrafficmanagementservice(void);
	virtual const AirTrafficManagementServiceTimeSlice& as_airtrafficmanagementservice(void) const;

	virtual void dependencies(UUID::set_t& dep) const;
	virtual void dependencies(Link::LinkSet& dep) const;

	const Link& get_serviceprovider(void) const { return m_serviceprovider; }
	void set_serviceprovider(const Link& sp) { m_serviceprovider = sp; }

	typedef std::set<Link> clientairspaces_t;
	const clientairspaces_t& get_clientairspaces(void) const { return m_clientairspaces; }
	clientairspaces_t& get_clientairspaces(void) { return m_clientairspaces; }
	void set_clientairspaces(const clientairspaces_t& cla) { m_clientairspaces = cla; }

	virtual std::ostream& print(std::ostream& os) const;

	template<class Archive> void hibernate(Archive& ar) {
		TimeSlice::hibernate(ar);
		ar.io(m_serviceprovider);
		ar.io(m_clientairspaces);
	}

protected:
	Link m_serviceprovider;
	clientairspaces_t m_clientairspaces;
};

class AirTrafficManagementService : public ObjectImpl<AirTrafficManagementServiceTimeSlice,Object::type_airtrafficmanagementservice> {
public:
	typedef Glib::RefPtr<AirTrafficManagementService> ptr_t;
	typedef Glib::RefPtr<const AirTrafficManagementService> const_ptr_t;
	typedef ObjectImpl<AirTrafficManagementServiceTimeSlice,Object::type_airtrafficmanagementservice> objbase_t;

	AirTrafficManagementService(const UUID& uuid = UUID());

	ptr_t clone_obj(void) const { ptr_t p(new AirTrafficManagementService(get_uuid())); clone_impl(p); return p; }
	virtual Object::ptr_t clone(void) const { return clone_obj(); }
};

class UnitTimeSlice : public IdentTimeSlice {
public:
	UnitTimeSlice(timetype_t starttime = 0, timetype_t endtime = 0);

	virtual UnitTimeSlice& as_unit(void);
	virtual const UnitTimeSlice& as_unit(void) const;

	virtual void dependencies(UUID::set_t& dep) const;
	virtual void dependencies(Link::LinkSet& dep) const;

        const std::string& get_name(void) const { return m_name; }
	void set_name(const std::string& name) { m_name = name; }

	virtual std::ostream& print(std::ostream& os) const;

	template<class Archive> void hibernate(Archive& ar) {
		IdentTimeSlice::hibernate(ar);
		ar.io(m_name);
	}

protected:
        std::string m_name;
};

class Unit : public ObjectImpl<UnitTimeSlice,Object::type_unit> {
public:
	typedef Glib::RefPtr<Unit> ptr_t;
	typedef Glib::RefPtr<const Unit> const_ptr_t;
	typedef ObjectImpl<UnitTimeSlice,Object::type_unit> objbase_t;

	Unit(const UUID& uuid = UUID());

	ptr_t clone_obj(void) const { ptr_t p(new Unit(get_uuid())); clone_impl(p); return p; }
	virtual Object::ptr_t clone(void) const { return clone_obj(); }
};

class SpecialDateTimeSlice : public IdentTimeSlice {
public:
	SpecialDateTimeSlice(timetype_t starttime = 0, timetype_t endtime = 0);

	virtual SpecialDateTimeSlice& as_specialdate(void);
	virtual const SpecialDateTimeSlice& as_specialdate(void) const;

	virtual void dependencies(UUID::set_t& dep) const;
	virtual void dependencies(Link::LinkSet& dep) const;

	const Link& get_authority(void) const { return m_authority; }
	void set_authority(const Link& a) { m_authority = a; }

	typedef enum {
		type_hol,
		type_busyfri,
		type_invalid
	} type_t;
	type_t get_type(void) const { return m_type; }
	void set_type(type_t t) { m_type = t; }
	uint8_t get_day(void) const { return m_day; }
	void set_day(uint8_t d) { m_day = d; }
	uint8_t get_month(void) const { return m_month; }
	void set_month(uint8_t m) { m_month = m; }
	uint16_t get_year(void) const { return m_year; }
	void set_year(uint16_t y) { m_year = y; }

	virtual std::ostream& print(std::ostream& os) const;

	template<class Archive> void hibernate(Archive& ar) {
		IdentTimeSlice::hibernate(ar);
		ar.io(m_authority);
		ar.io(m_year);
		ar.io(m_month);
		ar.io(m_day);
		ar.iouint8(m_type);
	}

protected:
        std::string m_name;
	Link m_authority;
	uint16_t m_year;
	uint8_t m_month;
	uint8_t m_day;
	type_t m_type;
};

class SpecialDate : public ObjectImpl<SpecialDateTimeSlice,Object::type_specialdate> {
public:
	typedef Glib::RefPtr<SpecialDate> ptr_t;
	typedef Glib::RefPtr<const SpecialDate> const_ptr_t;
	typedef ObjectImpl<SpecialDateTimeSlice,Object::type_specialdate> objbase_t;

	SpecialDate(const UUID& uuid = UUID());

	ptr_t clone_obj(void) const { ptr_t p(new SpecialDate(get_uuid())); clone_impl(p); return p; }
	virtual Object::ptr_t clone(void) const { return clone_obj(); }
};

class TimeTableSpecialDateEval {
public:
	TimeTableSpecialDateEval(void);
	void load(Database& db);
	bool is_specialday(const TimeTableEval& tte, SpecialDateTimeSlice::type_t t = SpecialDateTimeSlice::type_invalid) const;

protected:
	typedef std::vector<Object::const_ptr_t> specialdates_t;
	specialdates_t m_specialdates;
};


};

#endif /* ADRUNIT_H */
