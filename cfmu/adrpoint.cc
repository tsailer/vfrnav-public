#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <iomanip>
#include <sstream>

#include "adr.hh"
#include "adrdb.hh"
#include "adrhibernate.hh"
#include "adrpoint.hh"

using namespace ADR;

Navaid::Navaid(const UUID& uuid)
	: objbase_t(uuid)
{
}

NavaidTimeSlice::NavaidTimeSlice(timetype_t starttime, timetype_t endtime)
	: ElevPointIdentTimeSlice(starttime, endtime), m_type(type_invalid)
{
}

NavaidTimeSlice& NavaidTimeSlice::as_navaid(void)
{
	if (this)
		return *this;
	return const_cast<NavaidTimeSlice&>(Navaid::invalid_timeslice);
}

const NavaidTimeSlice& NavaidTimeSlice::as_navaid(void) const
{
	if (this)
		return *this;
	return Navaid::invalid_timeslice;
}

void NavaidTimeSlice::dependencies(UUID::set_t& dep) const
{
	DependencyGenerator<UUID::set_t> gen(dep);
	(const_cast<NavaidTimeSlice *>(this))->hibernate(gen);	
}

void NavaidTimeSlice::dependencies(Link::LinkSet& dep) const
{
	DependencyGenerator<Link::LinkSet> gen(dep);
	(const_cast<NavaidTimeSlice *>(this))->hibernate(gen);
}

bool NavaidTimeSlice::is_vor(type_t t)
{
	switch (t) {
	case type_vor:
	case type_vor_dme:
	case type_vortac:
		return true;

	default:
		return false;
	};
}

bool NavaidTimeSlice::is_dme(type_t t)
{
	switch (t) {
	case type_vor_dme:
	case type_ils_dme:
	case type_loc_dme:
	case type_dme:
	case type_ndb_dme:
		return true;

	default:
		return false;
	};
}

bool NavaidTimeSlice::is_tacan(type_t t)
{
	switch (t) {
	case type_vortac:
	case type_tacan:
		return true;

	default:
		return false;
	};
}

bool NavaidTimeSlice::is_ils(type_t t)
{
	switch (t) {
	case type_ils:
	case type_ils_dme:
	case type_loc:
	case type_loc_dme:
		return true;

	default:
		return false;
	};
}

bool NavaidTimeSlice::is_loc(type_t t)
{
	switch (t) {
	case type_ils:
	case type_ils_dme:
	case type_loc:
	case type_loc_dme:
		return true;

	default:
		return false;
	};
}

bool NavaidTimeSlice::is_ndb(type_t t)
{
	switch (t) {
	case type_ndb:
	case type_ndb_dme:
	case type_ndb_mkr:
		return true;

	default:
		return false;
	};
}

bool NavaidTimeSlice::is_mkr(type_t t)
{
	switch (t) {
	case type_ndb_mkr:
	case type_mkr:
		return true;

	default:
		return false;
	};
}
const std::string& to_str(ADR::NavaidTimeSlice::type_t t)
{
	switch (t) {
	case ADR::NavaidTimeSlice::type_vor:
	{
		static const std::string r("VOR");
		return r;
	}

	case ADR::NavaidTimeSlice::type_vor_dme:
	{
		static const std::string r("VOR_DME");
		return r;
	}

	case ADR::NavaidTimeSlice::type_vortac:
	{
		static const std::string r("VORTAC");
		return r;
	}

	case ADR::NavaidTimeSlice::type_tacan:
	{
		static const std::string r("TACAN");
		return r;
	}

	case ADR::NavaidTimeSlice::type_ils:
	{
		static const std::string r("ILS");
		return r;
	}

	case ADR::NavaidTimeSlice::type_ils_dme:
	{
		static const std::string r("ILS_DME");
		return r;
	}

	case ADR::NavaidTimeSlice::type_loc:
	{
		static const std::string r("LOC");
		return r;
	}

	case ADR::NavaidTimeSlice::type_loc_dme:
	{
		static const std::string r("LOC_DME");
		return r;
	}

	case ADR::NavaidTimeSlice::type_dme:
	{
		static const std::string r("DME");
		return r;
	}

	case ADR::NavaidTimeSlice::type_ndb:
	{
		static const std::string r("NDB");
		return r;
	}

	case ADR::NavaidTimeSlice::type_ndb_dme:
	{
		static const std::string r("NDB_DME");
		return r;
	}

	case ADR::NavaidTimeSlice::type_ndb_mkr:
	{
		static const std::string r("NDB_MKR");
		return r;
	}

	case ADR::NavaidTimeSlice::type_mkr:
	{
		static const std::string r("MKR");
		return r;
	}

	default:
	{
		static const std::string r("?");
		return r;
	}
	}
}

bool NavaidTimeSlice::get(uint64_t tm, NavaidsDb::Navaid& el) const
{
	if (!is_inside(tm))
		return false;
	el.set_icao(get_ident());
	el.set_name(get_name());
	el.set_coord(get_coord());
	if (is_elev_valid())
		el.set_elev(get_elev());
	switch (get_type()) {
	case type_vor:
		el.set_navaid_type(NavaidsDb::Navaid::navaid_vor);
		return true;

	case type_vor_dme:
		el.set_navaid_type(NavaidsDb::Navaid::navaid_vordme);
		return true;

	case type_vortac:
		el.set_navaid_type(NavaidsDb::Navaid::navaid_vortac);
		return true;

	case type_tacan:
		el.set_navaid_type(NavaidsDb::Navaid::navaid_tacan);
		return true;

	case type_ils_dme:
	case type_loc_dme:
	case type_dme:
		el.set_navaid_type(NavaidsDb::Navaid::navaid_dme);
		return true;

	case type_ndb:
	case type_ndb_mkr:
		el.set_navaid_type(NavaidsDb::Navaid::navaid_ndb);
		return true;

	case type_ndb_dme:
		el.set_navaid_type(NavaidsDb::Navaid::navaid_ndbdme);
		return true;

	case type_ils:
	case type_loc:
	case type_mkr:
	case type_invalid:
	default:
		el.set_navaid_type(NavaidsDb::Navaid::navaid_invalid);
		return false;
	}
}

std::ostream& NavaidTimeSlice::print(std::ostream& os) const
{
	ElevPointIdentTimeSlice::print(os);
	os << ' ' << get_type();
	if (!get_name().empty())
		os << " name " << get_name();
	return os;
}

DesignatedPoint::DesignatedPoint(const UUID& uuid)
	: objbase_t(uuid)
{
}

DesignatedPointTimeSlice::DesignatedPointTimeSlice(timetype_t starttime, timetype_t endtime)
	: PointIdentTimeSlice(starttime, endtime)
{
}

DesignatedPointTimeSlice& DesignatedPointTimeSlice::as_designatedpoint(void)
{
	if (this)
		return *this;
	return const_cast<DesignatedPointTimeSlice&>(DesignatedPoint::invalid_timeslice);
}

const DesignatedPointTimeSlice& DesignatedPointTimeSlice::as_designatedpoint(void) const
{
	if (this)
		return *this;
	return DesignatedPoint::invalid_timeslice;
}

void DesignatedPointTimeSlice::dependencies(UUID::set_t& dep) const
{
	DependencyGenerator<UUID::set_t> gen(dep);
	(const_cast<DesignatedPointTimeSlice *>(this))->hibernate(gen);	
}

void DesignatedPointTimeSlice::dependencies(Link::LinkSet& dep) const
{
	DependencyGenerator<Link::LinkSet> gen(dep);
	(const_cast<DesignatedPointTimeSlice *>(this))->hibernate(gen);
}

const std::string& to_str(ADR::DesignatedPointTimeSlice::type_t t)
{
	switch (t) {
	case ADR::DesignatedPointTimeSlice::type_icao:
	{
		static const std::string r("ICAO");
		return r;
	}

	case ADR::DesignatedPointTimeSlice::type_terminal:
	{
		static const std::string r("TERMINAL");
		return r;
	}

	case ADR::DesignatedPointTimeSlice::type_coord:
	{
		static const std::string r("COORD");
		return r;
	}

	case ADR::DesignatedPointTimeSlice::type_adrboundary:
	{
		static const std::string r("ADR_BOUNDARY");
		return r;
	}

	case ADR::DesignatedPointTimeSlice::type_adrreference:
	{
		static const std::string r("ADR_REFERENCE");
		return r;
	}

	default:
	{
		static const std::string r("?");
		return r;
	}
	}
}

bool DesignatedPointTimeSlice::get(uint64_t tm, WaypointsDb::Waypoint& el) const
{
	if (!is_inside(tm))
		return false;
	el.set_icao(get_ident());
	el.set_name(get_name());
	el.set_coord(get_coord());
	el.set_usage(WaypointsDb::Waypoint::usage_bothlevel);
	switch (get_type()) {
	case type_icao:
		el.set_type(WaypointsDb::Waypoint::type_icao);
		break;

	case type_coord:
		el.set_type(WaypointsDb::Waypoint::type_coord);
		break;

	case type_terminal:
	case type_adrboundary:
	case type_adrreference:
		el.set_type(WaypointsDb::Waypoint::type_other);
		break;

	default:
		el.set_type(WaypointsDb::Waypoint::type_invalid);
		break;
	}
	return true;
}

std::ostream& DesignatedPointTimeSlice::print(std::ostream& os) const
{
	PointIdentTimeSlice::print(os);
	os << ' ' << get_type();
	if (!get_name().empty())
		os << " name " << get_name();
	return os;
}

IndicationTimeSlice::IndicationTimeSlice(timetype_t starttime, timetype_t endtime)
	: TimeSlice(starttime, endtime)
{
}

void IndicationTimeSlice::dependencies(UUID::set_t& dep) const
{
	DependencyGenerator<UUID::set_t> gen(dep);
	(const_cast<IndicationTimeSlice *>(this))->hibernate(gen);	
}

void IndicationTimeSlice::dependencies(Link::LinkSet& dep) const
{
	DependencyGenerator<Link::LinkSet> gen(dep);
	(const_cast<IndicationTimeSlice *>(this))->hibernate(gen);
}

bool IndicationTimeSlice::recompute(void)
{
	bool work(TimeSlice::recompute());
        if (!get_fix().get_obj()) {
                std::ostringstream oss;
                oss << "IndicationTimeSlice::recompute: unlinked object " << get_fix();
                throw std::runtime_error(oss.str());
        }
        if (!get_navaid().get_obj()) {
                std::ostringstream oss;
                oss << "IndicationTimeSlice::recompute: unlinked object " << get_navaid();
                throw std::runtime_error(oss.str());
        }
        const PointIdentTimeSlice& ptf(get_fix().get_obj()->operator()(*this).as_point());
        if (!ptf.is_valid()) {
                std::ostringstream oss;
                oss << "IndicationTimeSlice::recompute: object " << get_fix() << " not a point";
                throw std::runtime_error(oss.str());
        }
        const PointIdentTimeSlice& ptn(get_navaid().get_obj()->operator()(*this).as_point());
        if (!ptn.is_valid()) {
                std::ostringstream oss;
                oss << "IndicationTimeSlice::recompute: object " << get_navaid() << " not a point";
                throw std::runtime_error(oss.str());
        }
        Rect bbox(ptf.get_coord(), ptf.get_coord());
        bbox = bbox.add(ptn.get_coord());
        work = work || bbox != m_bbox;
        m_bbox = bbox;
        return work;
}

std::ostream& IndicationTimeSlice::print(std::ostream& os) const
{
	TimeSlice::print(os);
	os << std::endl << "    fix " << get_fix();
	{
		const IdentTimeSlice& id(get_fix().get_obj()->operator()(*this).as_ident());
		if (id.is_valid())
			os << ' ' << id.get_ident();
	}
	os << std::endl << "    navaid " << get_navaid();
	{
		const IdentTimeSlice& id(get_navaid().get_obj()->operator()(*this).as_ident());
		if (id.is_valid())
			os << ' ' << id.get_ident();
	}
	return os;
}

AngleIndication::AngleIndication(const UUID& uuid)
	: objbase_t(uuid)
{
}

AngleIndicationTimeSlice::AngleIndicationTimeSlice(timetype_t starttime, timetype_t endtime)
	: IndicationTimeSlice(starttime, endtime)
{
}

AngleIndicationTimeSlice& AngleIndicationTimeSlice::as_angleindication(void)
{
	if (this)
		return *this;
	return const_cast<AngleIndicationTimeSlice&>(AngleIndication::invalid_timeslice);
}

const AngleIndicationTimeSlice& AngleIndicationTimeSlice::as_angleindication(void) const
{
	if (this)
		return *this;
	return AngleIndication::invalid_timeslice;
}

void AngleIndicationTimeSlice::dependencies(UUID::set_t& dep) const
{
	DependencyGenerator<UUID::set_t> gen(dep);
	(const_cast<AngleIndicationTimeSlice *>(this))->hibernate(gen);	
}

void AngleIndicationTimeSlice::dependencies(Link::LinkSet& dep) const
{
	DependencyGenerator<Link::LinkSet> gen(dep);
	(const_cast<AngleIndicationTimeSlice *>(this))->hibernate(gen);
}

std::ostream& AngleIndicationTimeSlice::print(std::ostream& os) const
{
	IndicationTimeSlice::print(os);
	{
		std::ostringstream oss;
		oss << std::fixed << std::setprecision(2) << get_angle_deg();
		os << " angle " << oss.str();
	}
	return os;
}

DistanceIndication::DistanceIndication(const UUID& uuid)
	: objbase_t(uuid)
{
}

DistanceIndicationTimeSlice::DistanceIndicationTimeSlice(timetype_t starttime, timetype_t endtime)
	: IndicationTimeSlice(starttime, endtime)
{
}

DistanceIndicationTimeSlice& DistanceIndicationTimeSlice::as_distanceindication(void)
{
	if (this)
		return *this;
	return const_cast<DistanceIndicationTimeSlice&>(DistanceIndication::invalid_timeslice);
}

const DistanceIndicationTimeSlice& DistanceIndicationTimeSlice::as_distanceindication(void) const
{
	if (this)
		return *this;
	return DistanceIndication::invalid_timeslice;
}

void DistanceIndicationTimeSlice::dependencies(UUID::set_t& dep) const
{
	DependencyGenerator<UUID::set_t> gen(dep);
	(const_cast<DistanceIndicationTimeSlice *>(this))->hibernate(gen);	
}

void DistanceIndicationTimeSlice::dependencies(Link::LinkSet& dep) const
{
	DependencyGenerator<Link::LinkSet> gen(dep);
	(const_cast<DistanceIndicationTimeSlice *>(this))->hibernate(gen);
}

std::ostream& DistanceIndicationTimeSlice::print(std::ostream& os) const
{
	IndicationTimeSlice::print(os);
	{
		std::ostringstream oss;
		oss << std::fixed << std::setprecision(3) << get_dist_nmi();
		os << " dist " << oss.str();
	}
	return os;
}
