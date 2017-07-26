#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <iomanip>
#include <sstream>

#include "adr.hh"
#include "adrdb.hh"
#include "adrhibernate.hh"
#include "adraerodrome.hh"

using namespace ADR;

Airport::Airport(const UUID& uuid)
	: objbase_t(uuid)
{
}

Object::type_t Airport::get_type(void) const
{
	uint8_t flg(0);
	for (unsigned int i(0), n(size()); i < n; ++i) {
		const AirportTimeSlice& ts(operator[](i).as_airport());
		if (!ts.is_valid())
			continue;
		flg |= ts.get_flags();
	}
	flg &= AirportTimeSlice::flag_civ | AirportTimeSlice::flag_mil |
		AirportTimeSlice::flag_depifr | AirportTimeSlice::flag_arrifr;
	return (type_t)(type_airport | flg);
}

AirportTimeSlice::AirportTimeSlice(timetype_t starttime, timetype_t endtime)
	: ElevPointIdentTimeSlice(starttime, endtime), m_flags(0)
{
}

AirportTimeSlice& AirportTimeSlice::as_airport(void)
{
	if (this)
		return *this;
	return const_cast<AirportTimeSlice&>(Airport::invalid_timeslice);
}

const AirportTimeSlice& AirportTimeSlice::as_airport(void) const
{
	if (this)
		return *this;
	return Airport::invalid_timeslice;
}

void AirportTimeSlice::dependencies(UUID::set_t& dep) const
{
	DependencyGenerator<UUID::set_t> gen(dep);
	(const_cast<AirportTimeSlice *>(this))->hibernate(gen);	
}

void AirportTimeSlice::dependencies(Link::LinkSet& dep) const
{
	DependencyGenerator<Link::LinkSet> gen(dep);
	(const_cast<AirportTimeSlice *>(this))->hibernate(gen);
}

bool AirportTimeSlice::get(uint64_t tm, AirportsDb::Airport& el) const
{
	if (!is_inside(tm))
		return false;
	el.set_icao(get_ident());
	el.set_name(get_name());
	el.set_coord(get_coord());
	if (is_elev_valid())
		el.set_elevation(get_elev());
	el.set_typecode(is_mil() ? (is_civ() ? 'B' : 'C') : (is_civ() ? 'A' : 'D'));
	el.set_flightrules(AirportsDb::Airport::flightrules_arr_vfr |
			   AirportsDb::Airport::flightrules_dep_vfr |
			   (is_arrifr() ? AirportsDb::Airport::flightrules_arr_ifr : 0) |
			   (is_depifr() ? AirportsDb::Airport::flightrules_dep_ifr : 0));
	std::string rmk;
	if (!get_iata().empty())
		rmk += " IATA " + get_iata();
	if (get_cities().size() > 1)
		rmk += " Served Cities";
	else if (!get_cities().empty())
		rmk += " Served City";
	for (servedcities_t::const_iterator ci(get_cities().begin()), ce(get_cities().end()); ci != ce; ++ci)
		rmk += " " + *ci;
	if (!rmk.empty())
		el.set_vfrrmk(rmk.substr(1));
	return true;
}

std::ostream& AirportTimeSlice::print(std::ostream& os) const
{
	ElevPointIdentTimeSlice::print(os);
	if (!get_name().empty())
		os << " name " << get_name();
	if (!get_iata().empty())
		os << " iata " << get_iata();
	if (get_cities().size()) {
		os << " city";
		const servedcities_t& c(get_cities());
		for (servedcities_t::const_iterator i(c.begin()), e(c.end()); i != e; ++i)
			os << ' ' << *i;
	}
	if (is_civ())
		os << " civ";
	if (is_mil())
		os << " mil";
	if (is_depifr())
		os << " depifr";
	if (is_arrifr())
		os << " arrifr";
	return os;
}

AirportCollocation::AirportCollocation(const UUID& uuid)
	: objbase_t(uuid)
{
}

AirportCollocationTimeSlice::AirportCollocationTimeSlice(timetype_t starttime, timetype_t endtime)
	: TimeSlice(starttime, endtime)
{
}

AirportCollocationTimeSlice& AirportCollocationTimeSlice::as_airportcollocation(void)
{
	if (this)
		return *this;
	return const_cast<AirportCollocationTimeSlice&>(AirportCollocation::invalid_timeslice);
}

const AirportCollocationTimeSlice& AirportCollocationTimeSlice::as_airportcollocation(void) const
{
	if (this)
		return *this;
	return AirportCollocation::invalid_timeslice;
}

void AirportCollocationTimeSlice::dependencies(UUID::set_t& dep) const
{
	DependencyGenerator<UUID::set_t> gen(dep);
	(const_cast<AirportCollocationTimeSlice *>(this))->hibernate(gen);	
}

void AirportCollocationTimeSlice::dependencies(Link::LinkSet& dep) const
{
	DependencyGenerator<Link::LinkSet> gen(dep);
	(const_cast<AirportCollocationTimeSlice *>(this))->hibernate(gen);
}

std::ostream& AirportCollocationTimeSlice::print(std::ostream& os) const
{
	os << std::endl << "    host " << get_host();
	{
		const IdentTimeSlice& id(get_host().get_obj()->operator()(*this).as_ident());
		if (id.is_valid())
			os << ' ' << id.get_ident();
	}
	os << std::endl << "    dep " << get_dep();
	{
		const IdentTimeSlice& id(get_dep().get_obj()->operator()(*this).as_ident());
		if (id.is_valid())
			os << ' ' << id.get_ident();
	}
	return os;
}
