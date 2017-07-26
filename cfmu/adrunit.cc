#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <iomanip>
#include <sstream>

#include "adr.hh"
#include "adrdb.hh"
#include "adrhibernate.hh"
#include "adrunit.hh"
#include "adrairspace.hh"

using namespace ADR;

OrganisationAuthority::OrganisationAuthority(const UUID& uuid)
	: objbase_t(uuid)
{
}

OrganisationAuthorityTimeSlice::OrganisationAuthorityTimeSlice(timetype_t starttime, timetype_t endtime)
	: IdentTimeSlice(starttime, endtime)
{
}

OrganisationAuthorityTimeSlice& OrganisationAuthorityTimeSlice::as_organisationauthority(void)
{
	if (this)
		return *this;
	return const_cast<OrganisationAuthorityTimeSlice&>(OrganisationAuthority::invalid_timeslice);
}

const OrganisationAuthorityTimeSlice& OrganisationAuthorityTimeSlice::as_organisationauthority(void) const
{
	if (this)
		return *this;
	return OrganisationAuthority::invalid_timeslice;
}

void OrganisationAuthorityTimeSlice::dependencies(UUID::set_t& dep) const
{
	DependencyGenerator<UUID::set_t> gen(dep);
	(const_cast<OrganisationAuthorityTimeSlice *>(this))->hibernate(gen);	
}

void OrganisationAuthorityTimeSlice::dependencies(Link::LinkSet& dep) const
{
	DependencyGenerator<Link::LinkSet> gen(dep);
	(const_cast<OrganisationAuthorityTimeSlice *>(this))->hibernate(gen);
}

std::ostream& OrganisationAuthorityTimeSlice::print(std::ostream& os) const
{
	IdentTimeSlice::print(os);
	if (!get_name().empty())
		os << " name " << get_name();
	return os;
}

AirTrafficManagementService::AirTrafficManagementService(const UUID& uuid)
	: objbase_t(uuid)
{
}

AirTrafficManagementServiceTimeSlice::AirTrafficManagementServiceTimeSlice(timetype_t starttime, timetype_t endtime)
	: TimeSlice(starttime, endtime)
{
}

AirTrafficManagementServiceTimeSlice& AirTrafficManagementServiceTimeSlice::as_airtrafficmanagementservice(void)
{
	if (this)
		return *this;
	return const_cast<AirTrafficManagementServiceTimeSlice&>(AirTrafficManagementService::invalid_timeslice);
}

const AirTrafficManagementServiceTimeSlice& AirTrafficManagementServiceTimeSlice::as_airtrafficmanagementservice(void) const
{
	if (this)
		return *this;
	return AirTrafficManagementService::invalid_timeslice;
}

void AirTrafficManagementServiceTimeSlice::dependencies(UUID::set_t& dep) const
{
	DependencyGenerator<UUID::set_t> gen(dep);
	(const_cast<AirTrafficManagementServiceTimeSlice *>(this))->hibernate(gen);	
}

void AirTrafficManagementServiceTimeSlice::dependencies(Link::LinkSet& dep) const
{
	DependencyGenerator<Link::LinkSet> gen(dep);
	(const_cast<AirTrafficManagementServiceTimeSlice *>(this))->hibernate(gen);
}

std::ostream& AirTrafficManagementServiceTimeSlice::print(std::ostream& os) const
{
	TimeSlice::print(os);
	os << std::endl << "    " << get_serviceprovider();
	{
		const IdentTimeSlice& id(get_serviceprovider().get_obj()->operator()(*this).as_ident());
		if (id.is_valid())
			os << ' ' << id.get_ident();
	}
	const clientairspaces_t& cla(get_clientairspaces());
	for (clientairspaces_t::const_iterator ci(cla.begin()), ce(cla.end()); ci != ce; ++ci) {
		const Link& l(*ci);
		os << std::endl << "    " << l;
		const IdentTimeSlice& id(l.get_obj()->operator()(*this).as_ident());
		if (id.is_valid())
			os << ' ' << id.get_ident();
	}
	return os;
}

Unit::Unit(const UUID& uuid)
	: objbase_t(uuid)
{
}

UnitTimeSlice::UnitTimeSlice(timetype_t starttime, timetype_t endtime)
	: IdentTimeSlice(starttime, endtime)
{
}

UnitTimeSlice& UnitTimeSlice::as_unit(void)
{
	if (this)
		return *this;
	return const_cast<UnitTimeSlice&>(Unit::invalid_timeslice);
}

const UnitTimeSlice& UnitTimeSlice::as_unit(void) const
{
	if (this)
		return *this;
	return Unit::invalid_timeslice;
}

void UnitTimeSlice::dependencies(UUID::set_t& dep) const
{
	DependencyGenerator<UUID::set_t> gen(dep);
	(const_cast<UnitTimeSlice *>(this))->hibernate(gen);	
}

void UnitTimeSlice::dependencies(Link::LinkSet& dep) const
{
	DependencyGenerator<Link::LinkSet> gen(dep);
	(const_cast<UnitTimeSlice *>(this))->hibernate(gen);
}

std::ostream& UnitTimeSlice::print(std::ostream& os) const
{
	IdentTimeSlice::print(os);
	if (!get_name().empty())
		os << " name " << get_name();
	return os;
}

SpecialDate::SpecialDate(const UUID& uuid)
	: objbase_t(uuid)
{
}

SpecialDateTimeSlice::SpecialDateTimeSlice(timetype_t starttime, timetype_t endtime)
	: IdentTimeSlice(starttime, endtime), m_year(0), m_month(0), m_day(0), m_type(type_invalid)
{
}

SpecialDateTimeSlice& SpecialDateTimeSlice::as_specialdate(void)
{
	if (this)
		return *this;
	return const_cast<SpecialDateTimeSlice&>(SpecialDate::invalid_timeslice);
}

const SpecialDateTimeSlice& SpecialDateTimeSlice::as_specialdate(void) const
{
	if (this)
		return *this;
	return SpecialDate::invalid_timeslice;
}

void SpecialDateTimeSlice::dependencies(UUID::set_t& dep) const
{
	DependencyGenerator<UUID::set_t> gen(dep);
	(const_cast<SpecialDateTimeSlice *>(this))->hibernate(gen);	
}

void SpecialDateTimeSlice::dependencies(Link::LinkSet& dep) const
{
	DependencyGenerator<Link::LinkSet> gen(dep);
	(const_cast<SpecialDateTimeSlice *>(this))->hibernate(gen);
}

std::ostream& SpecialDateTimeSlice::print(std::ostream& os) const
{
	IdentTimeSlice::print(os);
	switch (get_type()) {
	case type_hol:
		os << " hol ";
		break;

	case type_busyfri:
		os << " busy fri ";
		break;

	default:
		os << " invalid ";
		break;
	}
	if (get_year())
		os << std::setfill('0') << std::setw(4) << (unsigned int)get_year() << '-';
	os << std::setfill('0') << std::setw(2) << (unsigned int)get_month() << '-'
	   << std::setfill('0') << std::setw(2) << (unsigned int)get_day() << std::endl
	   << "    " << get_authority();
	{
		const IdentTimeSlice& id(get_authority().get_obj()->operator()(*this).as_ident());
		if (id.is_valid())
			os << ' ' << id.get_ident();
	}
	return os;
}

TimeTableSpecialDateEval::TimeTableSpecialDateEval(void)
{
}

void TimeTableSpecialDateEval::load(Database& db)
{
	m_specialdates.clear();
	Database::findresults_t r(db.find_all(Database::loadmode_link, 0, std::numeric_limits<uint64_t>::max(),
					      Object::type_specialdate, Object::type_specialdate, 0));
	for (Database::findresults_t::iterator i(r.begin()), e(r.end()); i != e; ++i) {
		if (!i->get_obj())
			continue;
		m_specialdates.push_back(i->get_obj());
	}
}

bool TimeTableSpecialDateEval::is_specialday(const TimeTableEval& tte, SpecialDateTimeSlice::type_t t) const
{
	if (t == SpecialDateTimeSlice::type_invalid || tte.get_point().is_invalid())
		return false;
	for (specialdates_t::const_iterator i(m_specialdates.begin()), e(m_specialdates.end()); i != e; ++i) {
		const SpecialDateTimeSlice& ts((*i)->operator()(tte.get_time()).as_specialdate());
		if (!ts.is_valid())
			continue;
		if (ts.get_day() != tte.get_mday() || ts.get_month() != tte.get_mon() || (ts.get_year() && ts.get_year() != tte.get_year()))
			continue;
		const OrganisationAuthorityTimeSlice& tsoa(ts.get_authority().get_obj()->operator()(tte.get_time()).as_organisationauthority());
		if (!tsoa.is_valid())
			continue;
		const AirspaceTimeSlice& tsa(tsoa.get_border().get_obj()->operator()(tte.get_time()).as_airspace());
		if (!tsa.is_valid())
			continue;
		if (!tsa.is_inside(tte))
			continue;
		return true;
	}
	return false;
}
