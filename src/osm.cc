/*****************************************************************************/

/*
 *      osm.cc  --  OpenStreetMap.
 *
 *      Copyright (C) 2015  Thomas Sailer (t.sailer@alumni.ethz.ch)
 *
 *      This program is free software; you can redistribute it and/or modify
 *      it under the terms of the GNU General Public License as published by
 *      the Free Software Foundation; either version 2 of the License, or
 *      (at your option) any later version.
 *
 *      This program is distributed in the hope that it will be useful,
 *      but WITHOUT ANY WARRANTY; without even the implied warranty of
 *      MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *      GNU General Public License for more details.
 *
 *      You should have received a copy of the GNU General Public License
 *      along with this program; if not, write to the Free Software
 *      Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 *
 */

/*****************************************************************************/

#include <iomanip>
#include <fstream>

#include "osm.h"
#include "metgraph.h"

#ifdef HAVE_PQXX
#include <pqxx/connection.hxx>
#include <pqxx/transaction.hxx>
#include <pqxx/transactor.hxx>
#include <pqxx/result.hxx>
#include <pqxx/except.hxx>
#endif

// HOWTO create the database:
// su - postgres
// createuser osm
// createdb -e -O postgres planetosm
// psql -f  /usr/share/pgsql/contrib/postgis-2.1/postgis.sql -d planetosm
// psql -t -d planetosm -U postgres -a  -c  "CREATE SCHEMA markware;"
// psql -t -d planetosm -U postgres -a  -c  "ALTER DATABASE planetosm SET search_path='markware','public';"
// psql -t -d planetosm -U postgres -a  -c  "GRANT ALL PRIVILEGES ON DATABASE planetosm to osm;"
// psql -t -d planetosm -U postgres -a  -c  "CREATE EXTENSION hstore;"
// psql -f /usr/share/pgsql/contrib/postgis-2.1/spatial_ref_sys.sql planetosm
// vacuumdb  -d planetosm -z -e
// psql -t -d planetosm -U postgres -a -c "create user psqlar encrypted password 'xxxx';"
// osm2pgsql --keep-coastlines --unlogged --cache-strategy sparse -x -k -s -C 8000 -c -d planetosm -S /usr/share/osm2pgsql/default.style --latlong -G --number-processes 4 europe-latest.osm.pbf
// grant usage on schema markware to osm;

void OpenStreetMap::Tags::add_pg(const std::string& x)
{
	typedef enum {
		st_idle,
		st_qkey,
		st_qkeyq,
		st_keyend,
		st_keyend1,
		st_keyend2,
		st_value,
		st_qvalue,
		st_qvalueq,
		st_valueend
	} state_t;
	state_t st(st_idle);
	std::string key, value;
	for (std::string::const_iterator i(x.begin()), e(x.end()); ; ++i) {
		if ((st == st_keyend || st == st_valueend || st == st_value) && (i == e || *i == ',')) {
			operator[](key) = value;
			st = st_idle;
			if (i == e)
				break;
			continue;
		}
		if (i == e) {
			if (st != st_idle)
				throw std::runtime_error("pg tags syntax error: " + x);
			break;
		}
		switch (st) {
		default:
		case st_idle:
			if (*i == '"') {
				key.clear();
				value.clear();
				st = st_qkey;
				continue;
			}
			if (std::isspace(*i))
				continue;
			throw std::runtime_error("pg tags syntax error: " + x);
			continue;

		case st_qkey:
			if (*i == '"') {
				st = st_keyend;
				continue;
			}
			if (*i == '\\') {
				st = st_qkeyq;
				continue;
			}
			// fall through

		case st_qkeyq:
			key += *i;
			st = st_qkey;
			continue;

		case st_keyend:
			if (*i == '=') {
				st = st_keyend1;
				continue;
			}
			if (std::isspace(*i))
				continue;
			throw std::runtime_error("pg tags syntax error: " + x);
			continue;

		case st_keyend1:
			if (*i == '>') {
				st = st_keyend2;
				continue;
			}
			throw std::runtime_error("pg tags syntax error: " + x);
			continue;

		case st_keyend2:
			if (std::isalnum(*i)) {
				value = *i;
				st = st_value;
				continue;
			}
			if (*i == '"') {
				value.clear();
				st = st_qvalue;
				continue;
			}
			if (std::isspace(*i))
				continue;
			throw std::runtime_error("pg tags syntax error: " + x);
			continue;

		case st_value:
			if (std::isalnum(*i)) {
				value += *i;
				continue;
			}
			if (std::isspace(*i)) {
				st = st_valueend;
				continue;
			}
			throw std::runtime_error("pg tags syntax error: " + x);
			continue;

		case st_qvalue:
			if (*i == '"') {
				st = st_valueend;
				continue;
			}
			if (*i == '\\') {
				st = st_qvalueq;
				continue;
			}
			// fall through

		case st_qvalueq:
			value += *i;
			st = st_qvalue;
			continue;

		case st_valueend:
			if (std::isspace(*i))
				continue;
			throw std::runtime_error("pg tags syntax error: " + x);
			continue;
		}
	}
}

std::string OpenStreetMap::Tags::to_str(void) const
{
	bool subseq(false);
	std::ostringstream oss;
	for (const_iterator i(begin()), e(end()); i != e; ++i) {
		if (subseq)
			oss << ", ";
		subseq = true;
		oss << i->first;
		if (!i->second.empty())
			oss << ": " << i->second;
	}
	return oss.str();
}

OpenStreetMap::OSMObject::OSMObject(int64_t id, const std::string& access, 
				    const std::string& addr_housename, const std::string& addr_housenumber, 
				    const std::string& addr_interpolation, const std::string& admin_level, 
				    const std::string& aerialway, const std::string& aeroway, 
				    const std::string& amenity, const std::string& area, 
				    const std::string& barrier, const std::string& bicycle, 
				    const std::string& brand, const std::string& bridge, 
				    const std::string& boundary, const std::string& building, 
				    const std::string& construction, const std::string& covered,
				    const std::string& culvert, const std::string& cutting,
				    const std::string& denomination, const std::string& disused,
				    const std::string& embankment, const std::string& foot, 
				    const std::string& generator_source, const std::string& harbour, 
				    const std::string& highway, const std::string& historic, 
				    const std::string& horse, const std::string& intermittent, 
				    const std::string& junction, const std::string& landuse, 
				    const std::string& layer, const std::string& leisure, 
				    const std::string& lock, const std::string& man_made, 
				    const std::string& military, const std::string& motorcar, 
				    const std::string& name, const std::string& natural, 
				    const std::string& office, const std::string& oneway, 
				    const std::string& oper, const std::string& place, 
				    const std::string& population, const std::string& power,
				    const std::string& power_source, const std::string& public_transport,
				    const std::string& railway, const std::string& ref,
				    const std::string& religion, const std::string& route,
				    const std::string& service, const std::string& shop,
				    const std::string& sport, const std::string& surface,
				    const std::string& toll, const std::string& tourism,
				    const std::string& tower_type, const std::string& tunnel,
				    const std::string& water, const std::string& waterway,
				    const std::string& wetland, const std::string& width,
				    const std::string& wood, int32_t z_order, const Tags& tags)
	: m_id(id), m_access(access), m_addr_housename(addr_housename), m_addr_housenumber(addr_housenumber), m_addr_interpolation(addr_interpolation),
	  m_admin_level(admin_level), m_aerialway(aerialway), m_aeroway(aeroway), m_amenity(amenity), m_area(area),
	  m_barrier(barrier), m_bicycle(bicycle), m_brand(brand), m_bridge(bridge), m_boundary(boundary),
	  m_building(building), m_construction(construction), m_covered(covered), m_culvert(culvert), m_cutting(cutting),
	  m_denomination(denomination), m_disused(disused), m_embankment(embankment), m_foot(foot), m_generator_source(generator_source),
	  m_harbour(harbour), m_highway(highway), m_historic(historic), m_horse(horse), m_intermittent(intermittent),
	  m_junction(junction), m_landuse(landuse), m_layer(layer), m_leisure(leisure), m_lock(lock), m_man_made(man_made),
	  m_military(military), m_motorcar(motorcar), m_name(name), m_natural(natural), m_office(office),
	  m_oneway(oneway), m_operator(oper), m_place(place), m_population(population), m_power(power),
	  m_power_source(power_source), m_public_transport(public_transport), m_railway(railway), m_ref(ref), m_religion(religion),
	  m_route(route), m_service(service), m_shop(shop), m_sport(sport), m_surface(surface),
	  m_toll(toll), m_tourism(tourism), m_tower_type(tower_type), m_tunnel(tunnel), m_water(water),
	  m_waterway(waterway), m_wetland(wetland), m_width(width), m_wood(wood), m_z_order(z_order),
	  m_tags(tags)
{
}

double OpenStreetMap::OSMObject::get_width_meter(void) const
{
	std::vector<double> a;
	for (const char *cp(get_width().c_str()); *cp; ) {
		if (std::isdigit(*cp) || *cp == '+' || *cp == '-') {
			char *cp1(0);
			a.push_back(strtod(cp, &cp1));
			if (cp == cp1)
				return std::numeric_limits<double>::quiet_NaN();
			cp = cp1;
			continue;
		}
		if (cp[0] == 'm' || cp[0] == 'M') {
			++cp;
			if (a.empty())
				return std::numeric_limits<double>::quiet_NaN();
			if (cp[0] == 'm' || cp[0] == 'M') {
				a.back() *= 1e-3;
				++cp;
				continue;
			}
			continue;
		}
		if ((cp[0] == 'k' || cp[0] == 'K') && (cp[1] == 'm' || cp[1] == 'M')) {
			if (a.empty())
				return std::numeric_limits<double>::quiet_NaN();
			a.back() *= 1e3;
			cp += 2;
			continue;
		}
		if ((cp[0] == 'f' || cp[0] == 'F') && (cp[1] == 't' || cp[1] == 'T')) {
			if (a.empty())
				return std::numeric_limits<double>::quiet_NaN();
			a.back() *= Point::ft_to_m_dbl;
			cp += 2;
			continue;
		}
		if (*cp == '\'') {
			if (a.empty())
				return std::numeric_limits<double>::quiet_NaN();
			a.back() *= Point::ft_to_m_dbl;
			++cp;
			continue;
		}
		if (*cp == '"') {
			if (a.empty())
				return std::numeric_limits<double>::quiet_NaN();
			a.back() *= 0.0254;
			++cp;
			continue;
		}
		if (std::isspace(*cp)) {
			++cp;
			continue;
		}
		return std::numeric_limits<double>::quiet_NaN();
	}
	if (a.empty())
		return std::numeric_limits<double>::quiet_NaN();
	double r(0);
	for (std::vector<double>::const_iterator i(a.begin()), e(a.end()); i != e; ++i)
		r += *i;
	return r;
}

std::pair<const std::string&,bool> OpenStreetMap::OSMObject::get_fixed_attr(const std::string& attr) const
{
	if (attr == "access")
		return std::pair<const std::string&,bool>(get_access(), true);
	if (attr == "addr:housename")
		return std::pair<const std::string&,bool>(get_addr_housename(), true);
	if (attr == "addr:housenumber")
		return std::pair<const std::string&,bool>(get_addr_housenumber(), true);
	if (attr == "addr:interpolation")
		return std::pair<const std::string&,bool>(get_addr_interpolation(), true);
	if (attr == "admin_level")
		return std::pair<const std::string&,bool>(get_admin_level(), true);
	if (attr == "aerialway")
		return std::pair<const std::string&,bool>(get_aerialway(), true);
	if (attr == "aeroway")
		return std::pair<const std::string&,bool>(get_aeroway(), true);
	if (attr == "amenity")
		return std::pair<const std::string&,bool>(get_amenity(), true);
	if (attr == "area")
		return std::pair<const std::string&,bool>(get_area(), true);
	if (attr == "barrier")
		return std::pair<const std::string&,bool>(get_barrier(), true);
	if (attr == "bicycle")
		return std::pair<const std::string&,bool>(get_bicycle(), true);
	if (attr == "brand")
		return std::pair<const std::string&,bool>(get_brand(), true);
	if (attr == "bridge")
		return std::pair<const std::string&,bool>(get_bridge(), true);
	if (attr == "boundary")
		return std::pair<const std::string&,bool>(get_boundary(), true);
	if (attr == "building")
		return std::pair<const std::string&,bool>(get_building(), true);
	if (attr == "construction")
		return std::pair<const std::string&,bool>(get_construction(), true);
	if (attr == "covered")
		return std::pair<const std::string&,bool>(get_covered(), true);
	if (attr == "culvert")
		return std::pair<const std::string&,bool>(get_culvert(), true);
	if (attr == "cutting")
		return std::pair<const std::string&,bool>(get_cutting(), true);
	if (attr == "denomination")
		return std::pair<const std::string&,bool>(get_denomination(), true);
	if (attr == "disused")
		return std::pair<const std::string&,bool>(get_disused(), true);
	if (attr == "embankment")
		return std::pair<const std::string&,bool>(get_embankment(), true);
	if (attr == "foot")
		return std::pair<const std::string&,bool>(get_foot(), true);
	if (attr == "generator_source")
		return std::pair<const std::string&,bool>(get_generator_source(), true);
	if (attr == "harbour")
		return std::pair<const std::string&,bool>(get_harbour(), true);
	if (attr == "highway")
		return std::pair<const std::string&,bool>(get_highway(), true);
	if (attr == "historic")
		return std::pair<const std::string&,bool>(get_historic(), true);
	if (attr == "horse")
		return std::pair<const std::string&,bool>(get_horse(), true);
	if (attr == "intermittent")
		return std::pair<const std::string&,bool>(get_intermittent(), true);
	if (attr == "junction")
		return std::pair<const std::string&,bool>(get_junction(), true);
	if (attr == "landuse")
		return std::pair<const std::string&,bool>(get_landuse(), true);
	if (attr == "layer")
		return std::pair<const std::string&,bool>(get_layer(), true);
	if (attr == "leisure")
		return std::pair<const std::string&,bool>(get_leisure(), true);
	if (attr == "lock")
		return std::pair<const std::string&,bool>(get_lock(), true);
	if (attr == "man_made")
		return std::pair<const std::string&,bool>(get_man_made(), true);
	if (attr == "military")
		return std::pair<const std::string&,bool>(get_military(), true);
	if (attr == "motorcar")
		return std::pair<const std::string&,bool>(get_motorcar(), true);
	if (attr == "name")
		return std::pair<const std::string&,bool>(get_name(), true);
	if (attr == "natural")
		return std::pair<const std::string&,bool>(get_natural(), true);
	if (attr == "office")
		return std::pair<const std::string&,bool>(get_office(), true);
	if (attr == "oneway")
		return std::pair<const std::string&,bool>(get_oneway(), true);
	if (attr == "operator")
		return std::pair<const std::string&,bool>(get_operator(), true);
	if (attr == "place")
		return std::pair<const std::string&,bool>(get_place(), true);
	if (attr == "population")
		return std::pair<const std::string&,bool>(get_population(), true);
	if (attr == "power")
		return std::pair<const std::string&,bool>(get_power(), true);
	if (attr == "power_source")
		return std::pair<const std::string&,bool>(get_power_source(), true);
	if (attr == "public_transport")
		return std::pair<const std::string&,bool>(get_public_transport(), true);
	if (attr == "railway")
		return std::pair<const std::string&,bool>(get_railway(), true);
	if (attr == "ref")
		return std::pair<const std::string&,bool>(get_ref(), true);
	if (attr == "religion")
		return std::pair<const std::string&,bool>(get_religion(), true);
	if (attr == "route")
		return std::pair<const std::string&,bool>(get_route(), true);
	if (attr == "service")
		return std::pair<const std::string&,bool>(get_service(), true);
	if (attr == "shop")
		return std::pair<const std::string&,bool>(get_shop(), true);
	if (attr == "sport")
		return std::pair<const std::string&,bool>(get_sport(), true);
	if (attr == "surface")
		return std::pair<const std::string&,bool>(get_surface(), true);
	if (attr == "toll")
		return std::pair<const std::string&,bool>(get_toll(), true);
	if (attr == "tourism")
		return std::pair<const std::string&,bool>(get_tourism(), true);
	if (attr == "tower:type")
		return std::pair<const std::string&,bool>(get_tower_type(), true);
	if (attr == "tunnel")
		return std::pair<const std::string&,bool>(get_tunnel(), true);
	if (attr == "water")
		return std::pair<const std::string&,bool>(get_water(), true);
	if (attr == "waterway")
		return std::pair<const std::string&,bool>(get_waterway(), true);
	if (attr == "wetland")
		return std::pair<const std::string&,bool>(get_wetland(), true);
	if (attr == "width")
		return std::pair<const std::string&,bool>(get_width(), true);
	if (attr == "wood")
		return std::pair<const std::string&,bool>(get_wood(), true);
	static const std::string empty;
	return std::pair<const std::string&,bool>(empty, false);
}

bool OpenStreetMap::OSMObject::is_attr(const std::string& attr) const
{
	std::pair<const std::string&,bool> a(get_fixed_attr(attr));
	if (a.second && !a.first.empty())
		return true;
	return get_tags().find(attr) != get_tags().end();
}

const std::string& OpenStreetMap::OSMObject::get_attr(const std::string& attr) const
{
	std::pair<const std::string&,bool> a(get_fixed_attr(attr));
	if (a.second && !a.first.empty())
		return a.first;
	Tags::const_iterator i(get_tags().find(attr));
	if (i != get_tags().end())
		return i->second;
	static const std::string empty;
	return empty;
}

std::string OpenStreetMap::OSMObject::to_str(void) const
{
	std::ostringstream oss;
	if (get_id() != std::numeric_limits<int64_t>::min())
		oss << "id: " << get_id() << ", ";
	if (!get_access().empty())
		oss << "access: " << get_access() << ", ";
	if (!get_addr_housename().empty())
		oss << "addr:housename: " << get_addr_housename() << ", ";
	if (!get_addr_housenumber().empty())
		oss << "addr:housenumber: " << get_addr_housenumber() << ", ";
	if (!get_addr_interpolation().empty())
		oss << "addr:interpolation: " << get_addr_interpolation() << ", ";
	if (!get_admin_level().empty())
		oss << "admin_level: " << get_admin_level() << ", ";
	if (!get_aerialway().empty())
		oss << "aerialway: " << get_aerialway() << ", ";
	if (!get_aeroway().empty())
		oss << "aeroway: " << get_aeroway() << ", ";
	if (!get_amenity().empty())
		oss << "amenity: " << get_amenity() << ", ";
	if (!get_area().empty())
		oss << "area: " << get_area() << ", ";
	if (!get_barrier().empty())
		oss << "barrier: " << get_barrier() << ", ";
	if (!get_bicycle().empty())
		oss << "bicycle: " << get_bicycle() << ", ";
	if (!get_brand().empty())
		oss << "brand: " << get_brand() << ", ";
	if (!get_bridge().empty())
		oss << "bridge: " << get_bridge() << ", ";
	if (!get_boundary().empty())
		oss << "boundary: " << get_boundary() << ", ";
	if (!get_building().empty())
		oss << "building: " << get_building() << ", ";
	if (!get_construction().empty())
		oss << "construction: " << get_construction() << ", ";
	if (!get_covered().empty())
		oss << "covered: " << get_covered() << ", ";
	if (!get_culvert().empty())
		oss << "culvert: " << get_culvert() << ", ";
	if (!get_cutting().empty())
		oss << "cutting: " << get_cutting() << ", ";
	if (!get_denomination().empty())
		oss << "denomination: " << get_denomination() << ", ";
	if (!get_disused().empty())
		oss << "disused: " << get_disused() << ", ";
	if (!get_embankment().empty())
		oss << "embankment: " << get_embankment() << ", ";
	if (!get_foot().empty())
		oss << "foot: " << get_foot() << ", ";
	if (!get_generator_source().empty())
		oss << "generator_source: " << get_generator_source() << ", ";
	if (!get_harbour().empty())
		oss << "harbour: " << get_harbour() << ", ";
	if (!get_highway().empty())
		oss << "highway: " << get_highway() << ", ";
	if (!get_historic().empty())
		oss << "historic: " << get_historic() << ", ";
	if (!get_horse().empty())
		oss << "horse: " << get_horse() << ", ";
	if (!get_intermittent().empty())
		oss << "intermittent: " << get_intermittent() << ", ";
	if (!get_junction().empty())
		oss << "junction: " << get_junction() << ", ";
	if (!get_landuse().empty())
		oss << "landuse: " << get_landuse() << ", ";
	if (!get_layer().empty())
		oss << "layer: " << get_layer() << ", ";
	if (!get_leisure().empty())
		oss << "leisure: " << get_leisure() << ", ";
	if (!get_lock().empty())
		oss << "lock: " << get_lock() << ", ";
	if (!get_man_made().empty())
		oss << "man_made: " << get_man_made() << ", ";
	if (!get_military().empty())
		oss << "military: " << get_military() << ", ";
	if (!get_motorcar().empty())
		oss << "motorcar: " << get_motorcar() << ", ";
	if (!get_name().empty())
		oss << "name: " << get_name() << ", ";
	if (!get_natural().empty())
		oss << "natural: " << get_natural() << ", ";
	if (!get_office().empty())
		oss << "office: " << get_office() << ", ";
	if (!get_oneway().empty())
		oss << "oneway: " << get_oneway() << ", ";
	if (!get_operator().empty())
		oss << "operator: " << get_operator() << ", ";
	if (!get_place().empty())
		oss << "place: " << get_place() << ", ";
	if (!get_population().empty())
		oss << "population: " << get_population() << ", ";
	if (!get_power().empty())
		oss << "power: " << get_power() << ", ";
	if (!get_power_source().empty())
		oss << "power_source: " << get_power_source() << ", ";
	if (!get_public_transport().empty())
		oss << "public_transport: " << get_public_transport() << ", ";
	if (!get_railway().empty())
		oss << "railway: " << get_railway() << ", ";
	if (!get_ref().empty())
		oss << "ref: " << get_ref() << ", ";
	if (!get_religion().empty())
		oss << "religion: " << get_religion() << ", ";
	if (!get_route().empty())
		oss << "route: " << get_route() << ", ";
	if (!get_service().empty())
		oss << "service: " << get_service() << ", ";
	if (!get_shop().empty())
		oss << "shop: " << get_shop() << ", ";
	if (!get_sport().empty())
		oss << "sport: " << get_sport() << ", ";
	if (!get_surface().empty())
		oss << "surface: " << get_surface() << ", ";
	if (!get_toll().empty())
		oss << "toll: " << get_toll() << ", ";
	if (!get_tourism().empty())
		oss << "tourism: " << get_tourism() << ", ";
	if (!get_tower_type().empty())
		oss << "tower:type: " << get_tower_type() << ", ";
	if (!get_tunnel().empty())
		oss << "tunnel: " << get_tunnel() << ", ";
	if (!get_water().empty())
		oss << "water: " << get_water() << ", ";
	if (!get_waterway().empty())
		oss << "waterway: " << get_waterway() << ", ";
	if (!get_wetland().empty())
		oss << "wetland: " << get_wetland() << ", ";
	if (!get_width().empty())
		oss << "width: " << get_width() << ", ";
	if (!get_wood().empty())
		oss << "wood: " << get_wood() << ", ";
	if (get_z_order() != std::numeric_limits<int32_t>::min())
		oss << "z_order: " << get_z_order() << ", ";
	if (!get_tags().empty())
		oss << get_tags().to_str() << ", ";
	std::string::size_type sz(oss.str().size());
	sz = std::max(sz, (std::string::size_type)2) - 2;
	return oss.str().substr(0, sz);
}

OpenStreetMap::OSMPoint::OSMPoint(int64_t id, const std::string& access, 
				  const std::string& addr_housename, const std::string& addr_housenumber, 
				  const std::string& addr_interpolation, const std::string& admin_level, 
				  const std::string& aerialway, const std::string& aeroway, 
				  const std::string& amenity, const std::string& area, 
				  const std::string& barrier, const std::string& bicycle, 
				  const std::string& brand, const std::string& bridge, 
				  const std::string& boundary, const std::string& building, 
				  const std::string& capital, const std::string& construction, 
				  const std::string& covered, const std::string& culvert, 
				  const std::string& cutting, const std::string& denomination, 
				  const std::string& disused, const std::string& ele, 
				  const std::string& embankment, const std::string& foot, 
				  const std::string& generator_source, const std::string& harbour, 
				  const std::string& highway, const std::string& historic, 
				  const std::string& horse, const std::string& intermittent, 
				  const std::string& junction, const std::string& landuse, 
				  const std::string& layer, const std::string& leisure, 
				  const std::string& lock, const std::string& man_made, 
				  const std::string& military, const std::string& motorcar, 
				  const std::string& name, const std::string& natural, 
				  const std::string& office, const std::string& oneway, 
				  const std::string& oper, const std::string& place, 
				  const std::string& poi, const std::string& population, 
				  const std::string& power, const std::string& power_source, 
				  const std::string& public_transport, const std::string& railway, 
				  const std::string& ref, const std::string& religion, 
				  const std::string& route, const std::string& service, 
				  const std::string& shop, const std::string& sport, 
				  const std::string& surface, const std::string& toll, 
				  const std::string& tourism, const std::string& tower_type, 
				  const std::string& tunnel, const std::string& water, 
				  const std::string& waterway, const std::string& wetland, 
				  const std::string& width, const std::string& wood, 
				  int32_t z_order, const Tags& tags, const Point& pt)
	: OSMObject(id, access, addr_housename, addr_housenumber, addr_interpolation, admin_level, aerialway, aeroway, amenity, area, 
		    barrier, bicycle, brand, bridge, boundary, building, construction, covered, culvert, cutting,
		    denomination, disused, embankment, foot, generator_source, harbour, highway, historic, horse, intermittent, 
		    junction, landuse, layer, leisure, lock, man_made, military, motorcar, name, natural, 
		    office, oneway, oper, place, population, power, power_source, public_transport, railway, ref,
		    religion, route, service, shop, sport, surface, toll, tourism, tower_type, tunnel,
		    water, waterway, wetland, width, wood, z_order, tags),
	  m_capital(capital), m_ele(ele), m_poi(poi), m_pt(pt)
{
}

std::pair<const std::string&,bool> OpenStreetMap::OSMPoint::get_fixed_attr(const std::string& attr) const
{
	if (attr == "capital")
		return std::pair<const std::string&,bool>(get_capital(), true);
	if (attr == "ele")
		return std::pair<const std::string&,bool>(get_ele(), true);
	if (attr == "poi")
		return std::pair<const std::string&,bool>(get_poi(), true);
	return OSMObject::get_fixed_attr(attr);
}

bool OpenStreetMap::OSMPoint::is_attr(const std::string& attr) const
{
	std::pair<const std::string&,bool> a(get_fixed_attr(attr));
	if (a.second && !a.first.empty())
		return true;
	return get_tags().find(attr) != get_tags().end();
}

const std::string& OpenStreetMap::OSMPoint::get_attr(const std::string& attr) const
{
	std::pair<const std::string&,bool> a(get_fixed_attr(attr));
	if (a.second && !a.first.empty())
		return a.first;
	Tags::const_iterator i(get_tags().find(attr));
	if (i != get_tags().end())
		return i->second;
	static const std::string empty;
	return empty;
}

std::string OpenStreetMap::OSMPoint::to_str(void) const
{
	std::ostringstream oss;
	get_pt().to_wkt(oss << OSMObject::to_str() << ", ");
	return oss.str();
}

OpenStreetMap::OSMObject2D::OSMObject2D(int64_t id, const std::string& access, 
					const std::string& addr_housename, const std::string& addr_housenumber, 
					const std::string& addr_interpolation, const std::string& admin_level, 
					const std::string& aerialway, const std::string& aeroway, 
					const std::string& amenity, const std::string& area, 
					const std::string& barrier, const std::string& bicycle, 
					const std::string& brand, const std::string& bridge, 
					const std::string& boundary, const std::string& building, 
					const std::string& construction, const std::string& covered,
					const std::string& culvert, const std::string& cutting,
					const std::string& denomination, const std::string& disused,
					const std::string& embankment, const std::string& foot,
					const std::string& generator_source, const std::string& harbour, 
					const std::string& highway, const std::string& historic, 
					const std::string& horse, const std::string& intermittent, 
					const std::string& junction, const std::string& landuse, 
					const std::string& layer, const std::string& leisure, 
					const std::string& lock, const std::string& man_made, 
					const std::string& military, const std::string& motorcar, 
					const std::string& name, const std::string& natural, 
					const std::string& office, const std::string& oneway, 
					const std::string& oper, const std::string& place, 
					const std::string& population, const std::string& power,
					const std::string& power_source, const std::string& public_transport,
					const std::string& railway, const std::string& ref,
					const std::string& religion, const std::string& route,
					const std::string& service, const std::string& shop,
					const std::string& sport, const std::string& surface,
					const std::string& toll, const std::string& tourism,
					const std::string& tower_type, const std::string& tracktype,
					const std::string& tunnel, const std::string& water, 
					const std::string& waterway, const std::string& wetland, 
					const std::string& width, const std::string& wood, 
					int32_t z_order, double way_area, const Tags& tags)
	: OSMObject(id, access, addr_housename, addr_housenumber, addr_interpolation, admin_level, aerialway, aeroway, amenity, area, 
		    barrier, bicycle, brand, bridge, boundary, building, construction, covered, culvert, cutting,
		    denomination, disused, embankment, foot, generator_source, harbour, highway, historic, horse, intermittent, 
		    junction, landuse, layer, leisure, lock, man_made, military, motorcar, name, natural, 
		    office, oneway, oper, place, population, power, power_source, public_transport, railway, ref,
		    religion, route, service, shop, sport, surface, toll, tourism, tower_type, tunnel,
		    water, waterway, wetland, width, wood, z_order, tags),
	  m_tracktype(tracktype), m_way_area(way_area)
{
}

std::pair<const std::string&,bool> OpenStreetMap::OSMObject2D::get_fixed_attr(const std::string& attr) const
{
	if (attr == "tracktype")
		return std::pair<const std::string&,bool>(get_tracktype(), true);
	return OSMObject::get_fixed_attr(attr);
}

bool OpenStreetMap::OSMObject2D::is_attr(const std::string& attr) const
{
	std::pair<const std::string&,bool> a(get_fixed_attr(attr));
	if (a.second && !a.first.empty())
		return true;
	return get_tags().find(attr) != get_tags().end();
}

const std::string& OpenStreetMap::OSMObject2D::get_attr(const std::string& attr) const
{
	std::pair<const std::string&,bool> a(get_fixed_attr(attr));
	if (a.second && !a.first.empty())
		return a.first;
	Tags::const_iterator i(get_tags().find(attr));
	if (i != get_tags().end())
		return i->second;
	static const std::string empty;
	return empty;
}

std::string OpenStreetMap::OSMObject2D::to_str(void) const
{
	std::ostringstream oss;
	oss << OSMObject::to_str() << ", ";
	if (!get_tracktype().empty())
		oss << "tracktype: " << get_tracktype() << ", ";
	if (!std::isnan(get_way_area()))
		oss << "way_area: " << get_way_area() << ", ";
	std::string::size_type sz(oss.str().size());
	sz = std::max(sz, (std::string::size_type)2) - 2;
	return oss.str().substr(0, sz);
}

OpenStreetMap::OSMLine::OSMLine(int64_t id, const std::string& access, 
				const std::string& addr_housename, const std::string& addr_housenumber, 
				const std::string& addr_interpolation, const std::string& admin_level, 
				const std::string& aerialway, const std::string& aeroway, 
				const std::string& amenity, const std::string& area, 
				const std::string& barrier, const std::string& bicycle, 
				const std::string& brand, const std::string& bridge, 
				const std::string& boundary, const std::string& building, 
				const std::string& construction, const std::string& covered,
				const std::string& culvert, const std::string& cutting,
				const std::string& denomination, const std::string& disused,
				const std::string& embankment, const std::string& foot,
				const std::string& generator_source, const std::string& harbour, 
				const std::string& highway, const std::string& historic, 
				const std::string& horse, const std::string& intermittent, 
				const std::string& junction, const std::string& landuse, 
				const std::string& layer, const std::string& leisure, 
				const std::string& lock, const std::string& man_made, 
				const std::string& military, const std::string& motorcar, 
				const std::string& name, const std::string& natural, 
				const std::string& office, const std::string& oneway, 
				const std::string& oper, const std::string& place, 
				const std::string& population, const std::string& power,
				const std::string& power_source, const std::string& public_transport,
				const std::string& railway, const std::string& ref,
				const std::string& religion, const std::string& route,
				const std::string& service, const std::string& shop,
				const std::string& sport, const std::string& surface,
				const std::string& toll, const std::string& tourism,
				const std::string& tower_type, const std::string& tracktype,
				const std::string& tunnel, const std::string& water, 
				const std::string& waterway, const std::string& wetland, 
				const std::string& width, const std::string& wood, 
				int32_t z_order, double way_area, const Tags& tags, const LineString& line)
	: OSMObject2D(id, access, addr_housename, addr_housenumber, addr_interpolation, admin_level, aerialway, aeroway, amenity, area, 
		      barrier, bicycle, brand, bridge, boundary, building, construction, covered, culvert, cutting,
		      denomination, disused, embankment, foot, generator_source, harbour, highway, historic, horse, intermittent, 
		      junction, landuse, layer, leisure, lock, man_made, military, motorcar, name, natural, 
		      office, oneway, oper, place, population, power, power_source, public_transport, railway, ref,
		      religion, route, service, shop, sport, surface, toll, tourism, tower_type, tracktype,
		      tunnel, water, waterway, wetland, width, wood, z_order, way_area, tags),
	  m_line(line)
{
}

std::string OpenStreetMap::OSMLine::to_str(void) const
{
	std::ostringstream oss;
	get_line().to_wkt(oss << OSMObject2D::to_str() << ", ");
	return oss.str();
}

OpenStreetMap::OSMPolygon::OSMPolygon(int64_t id, const std::string& access, 
				      const std::string& addr_housename, const std::string& addr_housenumber, 
				      const std::string& addr_interpolation, const std::string& admin_level, 
				      const std::string& aerialway, const std::string& aeroway, 
				      const std::string& amenity, const std::string& area, 
				      const std::string& barrier, const std::string& bicycle, 
				      const std::string& brand, const std::string& bridge, 
				      const std::string& boundary, const std::string& building, 
				      const std::string& construction, const std::string& covered,
				      const std::string& culvert, const std::string& cutting,
				      const std::string& denomination, const std::string& disused,
				      const std::string& embankment, const std::string& foot,
				      const std::string& generator_source, const std::string& harbour, 
				      const std::string& highway, const std::string& historic, 
				      const std::string& horse, const std::string& intermittent, 
				      const std::string& junction, const std::string& landuse, 
				      const std::string& layer, const std::string& leisure, 
				      const std::string& lock, const std::string& man_made, 
				      const std::string& military, const std::string& motorcar, 
				      const std::string& name, const std::string& natural, 
				      const std::string& office, const std::string& oneway, 
				      const std::string& oper, const std::string& place, 
				      const std::string& population, const std::string& power,
				      const std::string& power_source, const std::string& public_transport,
				      const std::string& railway, const std::string& ref,
				      const std::string& religion, const std::string& route,
				      const std::string& service, const std::string& shop,
				      const std::string& sport, const std::string& surface,
				      const std::string& toll, const std::string& tourism,
				      const std::string& tower_type, const std::string& tracktype,
				      const std::string& tunnel, const std::string& water, 
				      const std::string& waterway, const std::string& wetland, 
				      const std::string& width, const std::string& wood, 
				      int32_t z_order, double way_area, const Tags& tags, const MultiPolygonHole& poly)
	: OSMObject2D(id, access, addr_housename, addr_housenumber, addr_interpolation, admin_level, aerialway, aeroway, amenity, area, 
		      barrier, bicycle, brand, bridge, boundary, building, construction, covered, culvert, cutting,
		      denomination, disused, embankment, foot, generator_source, harbour, highway, historic, horse, intermittent, 
		      junction, landuse, layer, leisure, lock, man_made, military, motorcar, name, natural, 
		      office, oneway, oper, place, population, power, power_source, public_transport, railway, ref,
		      religion, route, service, shop, sport, surface, toll, tourism, tower_type, tracktype,
		      tunnel, water, waterway, wetland, width, wood, z_order, way_area, tags),
	  m_poly(poly)
{
}

std::string OpenStreetMap::OSMPolygon::to_str(void) const
{
	std::ostringstream oss;
	get_poly().to_wkt(oss << OSMObject2D::to_str() << ", ");
	return oss.str();
}

#ifdef HAVE_PQXX
class OpenStreetMap::PGTransactor : public pqxx::transactor<pqxx::read_transaction> {
public:
	typedef pqxx::read_transaction transaction_t;

	PGTransactor(OpenStreetMap& osm, const Rect& bbox, const std::string& tname = "Briefing Pack");
	void operator()(transaction_t& tran);
	void on_abort(const char msg[]) throw();
	void on_doubt(void) throw();
	void on_commit(void);
	static std::string to_str(const pqxx::result::field& fld);
	static int32_t to_int32(const pqxx::result::field& fld);
	static int64_t to_int64(const pqxx::result::field& fld);
	static double to_double(const pqxx::result::field& fld);

protected:
	typedef pqxx::transactor<transaction_t> base_t;
	OpenStreetMap& m_osm;
	Rect m_bbox;
	osmpoints_t m_osmpoints;
	osmlines_t m_osmlines;
	osmlines_t m_osmroads;
	osmpolygons_t m_osmpolygons;

	std::string get_queryrect(transaction_t& tran) const;
	void get_points(transaction_t& tran);
	void get_lines(transaction_t& tran);
	void get_roads(transaction_t& tran);
	void get_polygons(transaction_t& tran);
};

OpenStreetMap::PGTransactor::PGTransactor(OpenStreetMap& osm, const Rect& bbox, const std::string& tname)
	: base_t(tname), m_osm(osm), m_bbox(bbox)
{
}

std::string OpenStreetMap::PGTransactor::to_str(const pqxx::result::field& fld)
{
	if (fld.is_null())
		return std::string();
	return fld.as<std::string>();
}

int32_t OpenStreetMap::PGTransactor::to_int32(const pqxx::result::field& fld)
{
	if (fld.is_null())
		return std::numeric_limits<int32_t>::min();
	return fld.as<int32_t>();
}

int64_t OpenStreetMap::PGTransactor::to_int64(const pqxx::result::field& fld)
{
	if (fld.is_null())
		return std::numeric_limits<int64_t>::min();
	return fld.as<int64_t>();
}

double OpenStreetMap::PGTransactor::to_double(const pqxx::result::field& fld)
{
	if (fld.is_null())
		return std::numeric_limits<double>::quiet_NaN();
	return fld.as<double>();
}

void OpenStreetMap::PGTransactor::operator()(transaction_t& tran)
{
	get_points(tran);
	get_lines(tran);
	get_roads(tran);
	get_polygons(tran);
}

void OpenStreetMap::PGTransactor::get_points(transaction_t& tran)
{
	// Schema of planet_osm_point
	// osm_id             | bigint               | 
	// access             | text                 | 
	// addr:housename     | text                 | 
	// addr:housenumber   | text                 | 
	// addr:interpolation | text                 | 
	// admin_level        | text                 | 
	// aerialway          | text                 | 
	// aeroway            | text                 | 
	// amenity            | text                 | 
	// area               | text                 | 
	// barrier            | text                 | 
	// bicycle            | text                 | 
	// brand              | text                 | 
	// bridge             | text                 | 
	// boundary           | text                 | 
	// building           | text                 | 
	// capital            | text                 | 
	// construction       | text                 | 
	// covered            | text                 | 
	// culvert            | text                 | 
	// cutting            | text                 | 
	// denomination       | text                 | 
	// disused            | text                 | 
	// ele                | text                 | 
	// embankment         | text                 | 
	// foot               | text                 | 
	// generator:source   | text                 | 
	// harbour            | text                 | 
	// highway            | text                 | 
	// historic           | text                 | 
	// horse              | text                 | 
	// intermittent       | text                 | 
	// junction           | text                 | 
	// landuse            | text                 | 
	// layer              | text                 | 
	// leisure            | text                 | 
	// lock               | text                 | 
	// man_made           | text                 | 
	// military           | text                 | 
	// motorcar           | text                 | 
	// name               | text                 | 
	// natural            | text                 | 
	// office             | text                 | 
	// oneway             | text                 | 
	// operator           | text                 | 
	// place              | text                 | 
	// poi                | text                 | 
	// population         | text                 | 
	// power              | text                 | 
	// power_source       | text                 | 
	// public_transport   | text                 | 
	// railway            | text                 | 
	// ref                | text                 | 
	// religion           | text                 | 
	// route              | text                 | 
	// service            | text                 | 
	// shop               | text                 | 
	// sport              | text                 | 
	// surface            | text                 | 
	// toll               | text                 | 
	// tourism            | text                 | 
	// tower:type         | text                 | 
	// tunnel             | text                 | 
	// water              | text                 | 
	// waterway           | text                 | 
	// wetland            | text                 | 
	// width              | text                 | 
	// wood               | text                 | 
	// z_order            | integer              | 
	// tags               | hstore               | 
	// way                | geometry(Point,4326) | 

	m_osmpoints.clear();
	std::string qstr("select osm_id,access,\"addr:housename\",\"addr:housenumber\",\"addr:interpolation\",admin_level,aerialway,aeroway,amenity,area,"
			 "barrier,bicycle,brand,bridge,boundary,building,capital,construction,covered,culvert,"
			 "cutting,denomination,disused,ele,embankment,foot,\"generator:source\",harbour,highway,historic,"
			 "horse,intermittent,junction,landuse,layer,leisure,lock,man_made,military,motorcar,"
			 "name,\"natural\",office,oneway,operator,place,poi,population,power,power_source,"
			 "public_transport,railway,ref,religion,route,service,shop,sport,surface,toll,"
			 "tourism,\"tower:type\",tunnel,water,waterway,wetland,width,wood,z_order,tags,"
			 "ST_AsBinary(ST_Transform(way,4326)) from planet_osm_point where way && " + get_queryrect(tran));
	if (false)
		std::cerr << "OSM query: " << qstr << std::endl;
	pqxx::result r(tran.exec(qstr, "Briefing Pack OSM"));
	for (pqxx::result::const_iterator ri(r.begin()), re(r.end()); ri != re; ++ri) {
		Tags tags;
		tags.add_pg(to_str(ri[69]));
		Point pt;
		{
			pqxx::binarystring bs(ri[70]);
			pt.from_wkb(bs.data(), bs.data() + bs.size());
		}
		m_osmpoints.push_back(OSMPoint(to_int64(ri[0]), to_str(ri[1]), to_str(ri[2]), to_str(ri[3]), to_str(ri[4]),
					       to_str(ri[5]), to_str(ri[6]), to_str(ri[7]), to_str(ri[8]), to_str(ri[9]),
					       to_str(ri[10]), to_str(ri[11]), to_str(ri[12]), to_str(ri[13]), to_str(ri[14]),
					       to_str(ri[15]), to_str(ri[16]), to_str(ri[17]), to_str(ri[18]), to_str(ri[19]),
					       to_str(ri[20]), to_str(ri[21]), to_str(ri[22]), to_str(ri[23]), to_str(ri[24]),
					       to_str(ri[25]), to_str(ri[26]), to_str(ri[27]), to_str(ri[28]), to_str(ri[29]),
					       to_str(ri[30]), to_str(ri[31]), to_str(ri[32]), to_str(ri[33]), to_str(ri[34]),
					       to_str(ri[35]), to_str(ri[36]), to_str(ri[37]), to_str(ri[38]), to_str(ri[39]),
					       to_str(ri[40]), to_str(ri[41]), to_str(ri[42]), to_str(ri[43]), to_str(ri[44]),
					       to_str(ri[45]), to_str(ri[46]), to_str(ri[47]), to_str(ri[48]), to_str(ri[49]),
					       to_str(ri[50]), to_str(ri[51]), to_str(ri[52]), to_str(ri[53]), to_str(ri[54]),
					       to_str(ri[55]), to_str(ri[56]), to_str(ri[57]), to_str(ri[58]), to_str(ri[59]),
					       to_str(ri[60]), to_str(ri[61]), to_str(ri[62]), to_str(ri[63]), to_str(ri[64]),
					       to_str(ri[65]), to_str(ri[66]), to_str(ri[67]), to_int32(ri[68]), tags, pt));
	}
}

void OpenStreetMap::PGTransactor::get_lines(transaction_t& tran)
{
	// Schema of planet_osm_line
	// osm_id             | bigint                    | 
	// access             | text                      | 
	// addr:housename     | text                      | 
	// addr:housenumber   | text                      | 
	// addr:interpolation | text                      | 
	// admin_level        | text                      | 
	// aerialway          | text                      | 
	// aeroway            | text                      | 
	// amenity            | text                      | 
	// area               | text                      | 
	// barrier            | text                      | 
	// bicycle            | text                      | 
	// brand              | text                      | 
	// bridge             | text                      | 
	// boundary           | text                      | 
	// building           | text                      | 
	// construction       | text                      | 
	// covered            | text                      | 
	// culvert            | text                      | 
	// cutting            | text                      | 
	// denomination       | text                      | 
	// disused            | text                      | 
	// embankment         | text                      | 
	// foot               | text                      | 
	// generator:source   | text                      | 
	// harbour            | text                      | 
	// highway            | text                      | 
	// historic           | text                      | 
	// horse              | text                      | 
	// intermittent       | text                      | 
	// junction           | text                      | 
	// landuse            | text                      | 
	// layer              | text                      | 
	// leisure            | text                      | 
	// lock               | text                      | 
	// man_made           | text                      | 
	// military           | text                      | 
	// motorcar           | text                      | 
	// name               | text                      | 
	// natural            | text                      | 
	// office             | text                      | 
	// oneway             | text                      | 
	// operator           | text                      | 
	// place              | text                      | 
	// population         | text                      | 
	// power              | text                      | 
	// power_source       | text                      | 
	// public_transport   | text                      | 
	// railway            | text                      | 
	// ref                | text                      | 
	// religion           | text                      | 
	// route              | text                      | 
	// service            | text                      | 
	// shop               | text                      | 
	// sport              | text                      | 
	// surface            | text                      | 
	// toll               | text                      | 
	// tourism            | text                      | 
	// tower:type         | text                      | 
	// tracktype          | text                      | 
	// tunnel             | text                      | 
	// water              | text                      | 
	// waterway           | text                      | 
	// wetland            | text                      | 
	// width              | text                      | 
	// wood               | text                      | 
	// z_order            | integer                   | 
	// way_area           | real                      | 
	// tags               | hstore                    | 
	// way                | geometry(LineString,4326) | 

	m_osmlines.clear();
	std::string qstr("select osm_id,access,\"addr:housename\",\"addr:housenumber\",\"addr:interpolation\",admin_level,aerialway,aeroway,amenity,area,"
			 "barrier,bicycle,brand,bridge,boundary,building,construction,covered,culvert,cutting,"
			 "denomination,disused,embankment,foot,\"generator:source\",harbour,highway,historic,horse,intermittent,"
			 "junction,landuse,layer,leisure,lock,man_made,military,motorcar,name,\"natural\","
			 "office,oneway,operator,place,population,power,power_source,public_transport,railway,ref,"
			 "religion,route,service,shop,sport,surface,toll,tourism,\"tower:type\",tracktype,"
			 "tunnel,water,waterway,wetland,width,wood,z_order,way_area,tags,"
			 "ST_AsBinary(ST_Transform(way,4326)) from planet_osm_line where way && " + get_queryrect(tran));
	if (false)
		std::cerr << "OSM query: " << qstr << std::endl;
	pqxx::result r(tran.exec(qstr, "Briefing Pack OSM"));
	for (pqxx::result::const_iterator ri(r.begin()), re(r.end()); ri != re; ++ri) {
		Tags tags;
		tags.add_pg(to_str(ri[68]));
		LineString line;
		{
			pqxx::binarystring bs(ri[69]);
			line.from_wkb(bs.data(), bs.data() + bs.size());
		}
		m_osmlines.push_back(OSMLine(to_int64(ri[0]), to_str(ri[1]), to_str(ri[2]), to_str(ri[3]), to_str(ri[4]),
					     to_str(ri[5]), to_str(ri[6]), to_str(ri[7]), to_str(ri[8]), to_str(ri[9]),
					     to_str(ri[10]), to_str(ri[11]), to_str(ri[12]), to_str(ri[13]), to_str(ri[14]),
					     to_str(ri[15]), to_str(ri[16]), to_str(ri[17]), to_str(ri[18]), to_str(ri[19]),
					     to_str(ri[20]), to_str(ri[21]), to_str(ri[22]), to_str(ri[23]), to_str(ri[24]),
					     to_str(ri[25]), to_str(ri[26]), to_str(ri[27]), to_str(ri[28]), to_str(ri[29]),
					     to_str(ri[30]), to_str(ri[31]), to_str(ri[32]), to_str(ri[33]), to_str(ri[34]),
					     to_str(ri[35]), to_str(ri[36]), to_str(ri[37]), to_str(ri[38]), to_str(ri[39]),
					     to_str(ri[40]), to_str(ri[41]), to_str(ri[42]), to_str(ri[43]), to_str(ri[44]),
					     to_str(ri[45]), to_str(ri[46]), to_str(ri[47]), to_str(ri[48]), to_str(ri[49]),
					     to_str(ri[50]), to_str(ri[51]), to_str(ri[52]), to_str(ri[53]), to_str(ri[54]),
					     to_str(ri[55]), to_str(ri[56]), to_str(ri[57]), to_str(ri[58]), to_str(ri[59]),
					     to_str(ri[60]), to_str(ri[61]), to_str(ri[62]), to_str(ri[63]), to_str(ri[64]),
					     to_str(ri[65]), to_int32(ri[66]), to_double(ri[67]), tags, line));
	}
}

void OpenStreetMap::PGTransactor::get_roads(transaction_t& tran)
{
	// Schema of planet_osm_roads
	// osm_id             | bigint                    | 
	// access             | text                      | 
	// addr:housename     | text                      | 
	// addr:housenumber   | text                      | 
	// addr:interpolation | text                      | 
	// admin_level        | text                      | 
	// aerialway          | text                      | 
	// aeroway            | text                      | 
	// amenity            | text                      | 
	// area               | text                      | 
	// barrier            | text                      | 
	// bicycle            | text                      | 
	// brand              | text                      | 
	// bridge             | text                      | 
	// boundary           | text                      | 
	// building           | text                      | 
	// construction       | text                      | 
	// covered            | text                      | 
	// culvert            | text                      | 
	// cutting            | text                      | 
	// denomination       | text                      | 
	// disused            | text                      | 
	// embankment         | text                      | 
	// foot               | text                      | 
	// generator:source   | text                      | 
	// harbour            | text                      | 
	// highway            | text                      | 
	// historic           | text                      | 
	// horse              | text                      | 
	// intermittent       | text                      | 
	// junction           | text                      | 
	// landuse            | text                      | 
	// layer              | text                      | 
	// leisure            | text                      | 
	// lock               | text                      | 
	// man_made           | text                      | 
	// military           | text                      | 
	// motorcar           | text                      | 
	// name               | text                      | 
	// natural            | text                      | 
	// office             | text                      | 
	// oneway             | text                      | 
	// operator           | text                      | 
	// place              | text                      | 
	// population         | text                      | 
	// power              | text                      | 
	// power_source       | text                      | 
	// public_transport   | text                      | 
	// railway            | text                      | 
	// ref                | text                      | 
	// religion           | text                      | 
	// route              | text                      | 
	// service            | text                      | 
	// shop               | text                      | 
	// sport              | text                      | 
	// surface            | text                      | 
	// toll               | text                      | 
	// tourism            | text                      | 
	// tower:type         | text                      | 
	// tracktype          | text                      | 
	// tunnel             | text                      | 
	// water              | text                      | 
	// waterway           | text                      | 
	// wetland            | text                      | 
	// width              | text                      | 
	// wood               | text                      | 
	// z_order            | integer                   | 
	// way_area           | real                      | 
	// tags               | hstore                    | 
	// way                | geometry(LineString,4326) | 

	m_osmroads.clear();
	std::string qstr("select osm_id,access,\"addr:housename\",\"addr:housenumber\",\"addr:interpolation\",admin_level,aerialway,aeroway,amenity,area,"
			 "barrier,bicycle,brand,bridge,boundary,building,construction,covered,culvert,cutting,"
			 "denomination,disused,embankment,foot,\"generator:source\",harbour,highway,historic,horse,intermittent,"
			 "junction,landuse,layer,leisure,lock,man_made,military,motorcar,name,\"natural\","
			 "office,oneway,operator,place,population,power,power_source,public_transport,railway,ref,"
			 "religion,route,service,shop,sport,surface,toll,tourism,\"tower:type\",tracktype,"
			 "tunnel,water,waterway,wetland,width,wood,z_order,way_area,tags,"
			 "ST_AsBinary(ST_Transform(way,4326)) from planet_osm_roads where way && " + get_queryrect(tran));
	if (false)
		std::cerr << "OSM query: " << qstr << std::endl;
	pqxx::result r(tran.exec(qstr, "Briefing Pack OSM"));
	for (pqxx::result::const_iterator ri(r.begin()), re(r.end()); ri != re; ++ri) {
		Tags tags;
		tags.add_pg(to_str(ri[68]));
		LineString line;
		{
			pqxx::binarystring bs(ri[69]);
			line.from_wkb(bs.data(), bs.data() + bs.size());
		}
		m_osmroads.push_back(OSMLine(to_int64(ri[0]), to_str(ri[1]), to_str(ri[2]), to_str(ri[3]), to_str(ri[4]),
					     to_str(ri[5]), to_str(ri[6]), to_str(ri[7]), to_str(ri[8]), to_str(ri[9]),
					     to_str(ri[10]), to_str(ri[11]), to_str(ri[12]), to_str(ri[13]), to_str(ri[14]),
					     to_str(ri[15]), to_str(ri[16]), to_str(ri[17]), to_str(ri[18]), to_str(ri[19]),
					     to_str(ri[20]), to_str(ri[21]), to_str(ri[22]), to_str(ri[23]), to_str(ri[24]),
					     to_str(ri[25]), to_str(ri[26]), to_str(ri[27]), to_str(ri[28]), to_str(ri[29]),
					     to_str(ri[30]), to_str(ri[31]), to_str(ri[32]), to_str(ri[33]), to_str(ri[34]),
					     to_str(ri[35]), to_str(ri[36]), to_str(ri[37]), to_str(ri[38]), to_str(ri[39]),
					     to_str(ri[40]), to_str(ri[41]), to_str(ri[42]), to_str(ri[43]), to_str(ri[44]),
					     to_str(ri[45]), to_str(ri[46]), to_str(ri[47]), to_str(ri[48]), to_str(ri[49]),
					     to_str(ri[50]), to_str(ri[51]), to_str(ri[52]), to_str(ri[53]), to_str(ri[54]),
					     to_str(ri[55]), to_str(ri[56]), to_str(ri[57]), to_str(ri[58]), to_str(ri[59]),
					     to_str(ri[60]), to_str(ri[61]), to_str(ri[62]), to_str(ri[63]), to_str(ri[64]),
					     to_str(ri[65]), to_int32(ri[66]), to_double(ri[67]), tags, line));
	}
}

void OpenStreetMap::PGTransactor::get_polygons(transaction_t& tran)
{
	// Schema of planet_osm_polygon
	// osm_id             | bigint                  | 
	// access             | text                    | 
	// addr:housename     | text                    | 
	// addr:housenumber   | text                    | 
	// addr:interpolation | text                    | 
	// admin_level        | text                    | 
	// aerialway          | text                    | 
	// aeroway            | text                    | 
	// amenity            | text                    | 
	// area               | text                    | 
	// barrier            | text                    | 
	// bicycle            | text                    | 
	// brand              | text                    | 
	// bridge             | text                    | 
	// boundary           | text                    | 
	// building           | text                    | 
	// construction       | text                    | 
	// covered            | text                    | 
	// culvert            | text                    | 
	// cutting            | text                    | 
	// denomination       | text                    | 
	// disused            | text                    | 
	// embankment         | text                    | 
	// foot               | text                    | 
	// generator:source   | text                    | 
	// harbour            | text                    | 
	// highway            | text                    | 
	// historic           | text                    | 
	// horse              | text                    | 
	// intermittent       | text                    | 
	// junction           | text                    | 
	// landuse            | text                    | 
	// layer              | text                    | 
	// leisure            | text                    | 
	// lock               | text                    | 
	// man_made           | text                    | 
	// military           | text                    | 
	// motorcar           | text                    | 
	// name               | text                    | 
	// natural            | text                    | 
	// office             | text                    | 
	// oneway             | text                    | 
	// operator           | text                    | 
	// place              | text                    | 
	// population         | text                    | 
	// power              | text                    | 
	// power_source       | text                    | 
	// public_transport   | text                    | 
	// railway            | text                    | 
	// ref                | text                    | 
	// religion           | text                    | 
	// route              | text                    | 
	// service            | text                    | 
	// shop               | text                    | 
	// sport              | text                    | 
	// surface            | text                    | 
	// toll               | text                    | 
	// tourism            | text                    | 
	// tower:type         | text                    | 
	// tracktype          | text                    | 
	// tunnel             | text                    | 
	// water              | text                    | 
	// waterway           | text                    | 
	// wetland            | text                    | 
	// width              | text                    | 
	// wood               | text                    | 
	// z_order            | integer                 | 
	// way_area           | real                    | 
	// tags               | hstore                  | 
	// way                | geometry(Geometry,4326) | 

	m_osmpolygons.clear();
	std::string qstr("select osm_id,access,\"addr:housename\",\"addr:housenumber\",\"addr:interpolation\",admin_level,aerialway,aeroway,amenity,area,"
			 "barrier,bicycle,brand,bridge,boundary,building,construction,covered,culvert,cutting,"
			 "denomination,disused,embankment,foot,\"generator:source\",harbour,highway,historic,horse,intermittent,"
			 "junction,landuse,layer,leisure,lock,man_made,military,motorcar,name,\"natural\","
			 "office,oneway,operator,place,population,power,power_source,public_transport,railway,ref,"
			 "religion,route,service,shop,sport,surface,toll,tourism,\"tower:type\",tracktype,"
			 "tunnel,water,waterway,wetland,width,wood,z_order,way_area,tags,"
			 "ST_AsBinary(ST_Transform(way,4326)) from planet_osm_polygon where way && " + get_queryrect(tran));
	if (false)
		std::cerr << "OSM query: " << qstr << std::endl;
	pqxx::result r(tran.exec(qstr, "Briefing Pack OSM"));
	for (pqxx::result::const_iterator ri(r.begin()), re(r.end()); ri != re; ++ri) {
		Tags tags;
		tags.add_pg(to_str(ri[68]));
		MultiPolygonHole poly;
		{
			pqxx::binarystring bs(ri[69]);
			poly.from_wkb(bs.data(), bs.data() + bs.size());
		}
		m_osmpolygons.push_back(OSMPolygon(to_int64(ri[0]), to_str(ri[1]), to_str(ri[2]), to_str(ri[3]), to_str(ri[4]),
						   to_str(ri[5]), to_str(ri[6]), to_str(ri[7]), to_str(ri[8]), to_str(ri[9]),
						   to_str(ri[10]), to_str(ri[11]), to_str(ri[12]), to_str(ri[13]), to_str(ri[14]),
						   to_str(ri[15]), to_str(ri[16]), to_str(ri[17]), to_str(ri[18]), to_str(ri[19]),
						   to_str(ri[20]), to_str(ri[21]), to_str(ri[22]), to_str(ri[23]), to_str(ri[24]),
						   to_str(ri[25]), to_str(ri[26]), to_str(ri[27]), to_str(ri[28]), to_str(ri[29]),
						   to_str(ri[30]), to_str(ri[31]), to_str(ri[32]), to_str(ri[33]), to_str(ri[34]),
						   to_str(ri[35]), to_str(ri[36]), to_str(ri[37]), to_str(ri[38]), to_str(ri[39]),
						   to_str(ri[40]), to_str(ri[41]), to_str(ri[42]), to_str(ri[43]), to_str(ri[44]),
						   to_str(ri[45]), to_str(ri[46]), to_str(ri[47]), to_str(ri[48]), to_str(ri[49]),
						   to_str(ri[50]), to_str(ri[51]), to_str(ri[52]), to_str(ri[53]), to_str(ri[54]),
						   to_str(ri[55]), to_str(ri[56]), to_str(ri[57]), to_str(ri[58]), to_str(ri[59]),
						   to_str(ri[60]), to_str(ri[61]), to_str(ri[62]), to_str(ri[63]), to_str(ri[64]),
						   to_str(ri[65]), to_int32(ri[66]), to_double(ri[67]), tags, poly));
	}
}

void OpenStreetMap::PGTransactor::on_abort(const char msg[]) throw()
{
	std::cerr << "pg: " << Name() << ": transaction aborted: " << msg << std::endl;
}

void OpenStreetMap::PGTransactor::on_doubt(void) throw()
{
	std::cerr << "pg: " << Name() << ": transaction in doubt" << std::endl;
}

void OpenStreetMap::PGTransactor::on_commit(void)
{
	m_osm.m_osmpoints = m_osmpoints;
	m_osm.m_osmlines = m_osmlines;   
	m_osm.m_osmroads = m_osmroads;   
	m_osm.m_osmpolygons = m_osmpolygons;
}

std::string OpenStreetMap::PGTransactor::get_queryrect(transaction_t& tran) const
{
	// 4326 is WGS84
	std::ostringstream oss;
	oss << "ST_SetSRID(ST_MakeBox2D(ST_Point(" << std::setprecision(9) << m_bbox.get_southwest().get_lon_deg_dbl()
	    << "," << std::setprecision(9) << m_bbox.get_southwest().get_lat_deg_dbl()
	    << "),ST_Point(" << std::setprecision(9) << m_bbox.get_northeast().get_lon_deg_dbl()
	    << "," << std::setprecision(9) << m_bbox.get_northeast().get_lat_deg_dbl()
	    << ")),4326)";
	return oss.str();
}
#endif

OpenStreetMap::OpenStreetMap(const Rect& bbox)
	: m_bbox(bbox)
{
}

void OpenStreetMap::load(const std::string& db)
{
#ifdef HAVE_PQXX
	try {
		pqxx::connection conn(db);
		{
			PGTransactor osm(*this, m_bbox);
			conn.perform(osm);
		}
	} catch (const pqxx::pqxx_exception& e) {
		std::cerr << "pqxx exception: " << e.base().what() << std::endl;
	}
#endif
}

bool OpenStreetMap::empty(void) const
{
	return get_points().empty() &&
		get_lines().empty() &&
		get_roads().empty() &&
		get_polygons().empty();
}

OpenStreetMap::zset_t OpenStreetMap::get_zset(void) const
{
	zset_t z;
	for (osmpoints_t::const_iterator i(m_osmpoints.begin()), e(m_osmpoints.end()); i != e; ++i)
		z.insert(i->get_z_order());
	for (osmlines_t::const_iterator i(m_osmlines.begin()), e(m_osmlines.end()); i != e; ++i)
		z.insert(i->get_z_order());
	for (osmlines_t::const_iterator i(m_osmroads.begin()), e(m_osmroads.end()); i != e; ++i)
		z.insert(i->get_z_order());
	for (osmpolygons_t::const_iterator i(m_osmpolygons.begin()), e(m_osmpolygons.end()); i != e; ++i)
		z.insert(i->get_z_order());
	return z;
}

OpenStreetMapDraw::Render::Object::AttrMatch::AttrMatch(const std::string& attr, const std::string& value, match_t match)
	: m_attr(attr), m_value(value), m_match(match)
{
}

bool OpenStreetMapDraw::Render::Object::AttrMatch::is_match(const OSMObject& x) const
{
	if (!x.is_attr(get_attr()))
		return false;
	const std::string& val(x.get_attr(get_attr()));
	switch (get_match()) {
	default:
	case match_exact:
		return val == get_value();
		
	case match_startswith:
		if (val.size() < get_value().size())
			return false;
		return !val.compare(0, get_value().size(), get_value());
	}
}

bool OpenStreetMapDraw::Render::Object::AttrMatch::is_match(const OSMPoint& x) const
{
	if (!x.is_attr(get_attr()))
		return false;
	const std::string& val(x.get_attr(get_attr()));
	switch (get_match()) {
	default:
	case match_exact:
		return val == get_value();
		
	case match_startswith:
		if (val.size() < get_value().size())
			return false;
		return !val.compare(0, get_value().size(), get_value());
	}
}

bool OpenStreetMapDraw::Render::Object::AttrMatch::is_match(const OSMObject2D& x) const
{
	if (!x.is_attr(get_attr()))
		return false;
	const std::string& val(x.get_attr(get_attr()));
	switch (get_match()) {
	default:
	case match_exact:
		return val == get_value();
		
	case match_startswith:
		if (val.size() < get_value().size())
			return false;
		return !val.compare(0, get_value().size(), get_value());
	}
}

constexpr double OpenStreetMapDraw::Render::Object::colscale;

OpenStreetMapDraw::Render::Object::Object(const std::string& ident, uint8_t colr, uint8_t colg, uint8_t colb, uint8_t cola)
	: m_ident(ident), m_colr(colr), m_colg(colg), m_colb(colb), m_cola(cola)
{
}

OpenStreetMapDraw::Render::Object::Object(const std::string& ident, uint8_t colr, uint8_t colg, uint8_t colb, uint8_t cola,
					  const std::string& attr1, const std::string& value1, AttrMatch::match_t match1)
	: m_ident(ident), m_colr(colr), m_colg(colg), m_colb(colb), m_cola(cola)
{
	m_matches.push_back(AttrMatch(attr1, value1, match1));
}

OpenStreetMapDraw::Render::Object::Object(const std::string& ident, uint8_t colr, uint8_t colg, uint8_t colb, uint8_t cola,
					  const std::string& attr1, const std::string& value1, AttrMatch::match_t match1,
					  const std::string& attr2, const std::string& value2, AttrMatch::match_t match2)
	: m_ident(ident), m_colr(colr), m_colg(colg), m_colb(colb), m_cola(cola)
{
	m_matches.push_back(AttrMatch(attr1, value1, match1));
	m_matches.push_back(AttrMatch(attr2, value2, match2));
}

OpenStreetMapDraw::Render::Object::Object(const std::string& ident, uint8_t colr, uint8_t colg, uint8_t colb, uint8_t cola,
					  const std::string& attr1, const std::string& value1, AttrMatch::match_t match1,
					  const std::string& attr2, const std::string& value2, AttrMatch::match_t match2,
					  const std::string& attr3, const std::string& value3, AttrMatch::match_t match3)
	: m_ident(ident), m_colr(colr), m_colg(colg), m_colb(colb), m_cola(cola)
{
	m_matches.push_back(AttrMatch(attr1, value1, match1));
	m_matches.push_back(AttrMatch(attr2, value2, match2));
	m_matches.push_back(AttrMatch(attr3, value3, match3));
}

bool OpenStreetMapDraw::Render::Object::is_match(const OSMObject& x) const
{
	for (matches_t::const_iterator i(m_matches.begin()), e(m_matches.end()); i != e; ++i)
		if (!i->is_match(x))
			return false;
	return true;
}

bool OpenStreetMapDraw::Render::Object::is_match(const OSMPoint& x) const
{
	for (matches_t::const_iterator i(m_matches.begin()), e(m_matches.end()); i != e; ++i)
		if (!i->is_match(x))
			return false;
	return true;
}

bool OpenStreetMapDraw::Render::Object::is_match(const OSMObject2D& x) const
{
	for (matches_t::const_iterator i(m_matches.begin()), e(m_matches.end()); i != e; ++i)
		if (!i->is_match(x))
			return false;
	return true;
}

void OpenStreetMapDraw::Render::Object::set_color(const Cairo::RefPtr<Cairo::Context>& cr) const
{
	if (get_cola() == 255) {
		cr->set_source_rgb(get_colr() * colscale, get_colg() * colscale, get_colb() * colscale);
		return;
	}
	cr->set_source_rgba(get_colr() * colscale, get_colg() * colscale, get_colb() * colscale, get_cola() * colscale);
}

std::ostream& OpenStreetMapDraw::Render::Object::define_latex_color(std::ostream& os) const
{
	std::ostringstream oss;
	oss << std::fixed << "\\definecolor{colosm" << get_ident() << "}{rgb";
	if (get_cola() == 255)
		oss << "}{" << (get_colr() * colscale) << ',' << (get_colg() * colscale)
		    << ',' << (get_colb() * colscale);
	else
		oss << "a}{" << (get_colr() * colscale) << ',' << (get_colg() * colscale)
		    << ',' << (get_colb() * colscale) << ',' << (get_cola() * colscale);
	oss << "}%";
	return os << oss.str() << std::endl;
}

const OpenStreetMapDraw::Render::Object OpenStreetMapDraw::Render::dflt("background", 241, 238, 232, 255);

OpenStreetMapDraw::Render::Render(void)
{
}
		
void OpenStreetMapDraw::Render::clear(void)
{
	m_objects.clear();
}

void OpenStreetMapDraw::Render::add(const Object& obj)
{
	m_objects.push_back(obj);
}	

const OpenStreetMapDraw::Render::Object& OpenStreetMapDraw::Render::get(const OSMObject& x) const
{
	for (objects_t::const_iterator i(m_objects.begin()), e(m_objects.end()); i != e; ++i)
		if (i->is_match(x))
			return *i;
	return dflt;
}

const OpenStreetMapDraw::Render::Object& OpenStreetMapDraw::Render::get(const OSMPoint& x) const
{
	for (objects_t::const_iterator i(m_objects.begin()), e(m_objects.end()); i != e; ++i)
		if (i->is_match(x))
			return *i;
	return dflt;
}

const OpenStreetMapDraw::Render::Object& OpenStreetMapDraw::Render::get(const OSMObject2D& x) const
{
	for (objects_t::const_iterator i(m_objects.begin()), e(m_objects.end()); i != e; ++i)
		if (i->is_match(x))
			return *i;
	return dflt;
}

const OpenStreetMapDraw::Render::Object& OpenStreetMapDraw::Render::get_background(void) const
{
	for (objects_t::const_iterator i(m_objects.begin()), e(m_objects.end()); i != e; ++i)
		if (i->empty())
			return *i;
	return dflt;
}

std::ostream& OpenStreetMapDraw::Render::define_latex_colors(std::ostream& os) const
{
	bool bkgnd(true);
	for (objects_t::const_iterator i(m_objects.begin()), e(m_objects.end()); i != e; ++i) {
		i->define_latex_color(os);
		bkgnd = bkgnd && (i->get_ident() != dflt.get_ident());
	}
	if (bkgnd)
		dflt.define_latex_color(os);
	return os;
}

OpenStreetMapDraw::Obstacle::Obstacle(const Point& pt, double agl, double amsl, bool marked, bool lighted,
				      const std::string& hyperlink)
	: m_pt(pt), m_agl(agl), m_amsl(amsl), m_hyperlink(hyperlink), m_marked(marked), m_lighted(lighted)
{
}

OpenStreetMapDraw::CoordObstacle::CoordObstacle(const Obstacle& ob, double x, double y)
	: Obstacle(ob), m_x(x), m_y(y)
{
}

const double OpenStreetMapDraw::scalemeter = 1e-3 * Point::km_to_nmi_dbl / 60 * Point::from_deg_dbl;
constexpr double OpenStreetMapDraw::obstacle_cairo_radius;
constexpr double OpenStreetMapDraw::obstacle_cairo_fontsize;
const bool OpenStreetMapDraw::obstacle_preferamsl;
constexpr double OpenStreetMapDraw::arp_cairo_radius;

OpenStreetMapDraw::OpenStreetMapDraw(const Rect& bbox, const Point& arp)
	: OpenStreetMap(bbox), m_arp(arp)
{
	set_render_default();
}

void OpenStreetMapDraw::set_render_default(void)
{
	m_render.clear();
	m_render.add(Render::Object("aerorunwayconcrete", 187, 187, 204, 255,
				    "aeroway", "runway", Render::Object::AttrMatch::match_exact,
				    "surface", "concrete", Render::Object::AttrMatch::match_exact));
	m_render.add(Render::Object("aerorunwaygrass", 27, 184, 6, 255,
				    "aeroway", "runway", Render::Object::AttrMatch::match_exact,
				    "surface", "grass", Render::Object::AttrMatch::match_exact));
	m_render.add(Render::Object("aerorunway", 187, 187, 204, 255,
				    "aeroway", "runway", Render::Object::AttrMatch::match_exact));
	m_render.add(Render::Object("aerotaxiway", 187, 187, 204, 255,
				    "aeroway", "taxiway", Render::Object::AttrMatch::match_exact));
	m_render.add(Render::Object("aeroapron", 232, 209, 254, 255,
				    "aeroway", "apron", Render::Object::AttrMatch::match_exact));
	m_render.add(Render::Object("aeroterminal", 204, 153, 254, 255,
				    "aeroway", "terminal", Render::Object::AttrMatch::match_exact));
	m_render.add(Render::Object("aerohelipad", 187, 187, 204, 255,
				    "aeroway", "helipad", Render::Object::AttrMatch::match_exact));
	m_render.add(Render::Object("landusegrass", 205, 234, 176, 255,
				    "landuse", "grass", Render::Object::AttrMatch::match_exact));
	m_render.add(Render::Object("landusemeadow", 205, 234, 176, 255,
				    "landuse", "meadow", Render::Object::AttrMatch::match_exact));
	m_render.add(Render::Object("landuseforest", 173, 208, 158, 255,
				    "landuse", "forest", Render::Object::AttrMatch::match_exact));
	m_render.add(Render::Object("landuseresidential", 224, 222, 222, 255,
				    "landuse", "residential", Render::Object::AttrMatch::match_exact));
	m_render.add(Render::Object("landusefarmland", 237, 221, 201, 255,
				    "landuse", "farmland", Render::Object::AttrMatch::match_exact));
	m_render.add(Render::Object("landusefarmyard", 238, 213, 180, 255,
				    "landuse", "farmyard", Render::Object::AttrMatch::match_exact));
	m_render.add(Render::Object("landuseindustrial", 234, 218, 232, 255,
				    "landuse", "industrial", Render::Object::AttrMatch::match_exact));
	m_render.add(Render::Object("highwaymotorway", 137, 164, 203, 255,
				    "highway", "motorway", Render::Object::AttrMatch::match_startswith));
	m_render.add(Render::Object("highwayprimary", 220, 158, 158, 255,
				    "highway", "primary", Render::Object::AttrMatch::match_exact));
	m_render.add(Render::Object("highwaysecondary", 248, 213, 169, 255,
				    "highway", "secondary", Render::Object::AttrMatch::match_exact));
	m_render.add(Render::Object("highwaytertiary", 247, 247, 185, 255,
				    "highway", "tertiary", Render::Object::AttrMatch::match_exact));
	m_render.add(Render::Object("highwaytrunk", 148, 211, 148, 255,
				    "highway", "trunk", Render::Object::AttrMatch::match_exact));
	m_render.add(Render::Object("highwayresidential", 255, 255, 255, 255,
				    "highway", "residential", Render::Object::AttrMatch::match_exact));
	m_render.add(Render::Object("highwayunclassified", 255, 255, 255, 255,
				    "highway", "unclassified", Render::Object::AttrMatch::match_exact));
	m_render.add(Render::Object("buildingchurch", 174, 156, 140, 255,
				    "building", "church", Render::Object::AttrMatch::match_exact));
	m_render.add(Render::Object("building", 209, 198, 189, 255,
				    "building", "", Render::Object::AttrMatch::match_startswith));
	m_render.add(Render::Object("amenityplaceofworship", 174, 156, 140, 255,
				    "amenity", "place_of_worship", Render::Object::AttrMatch::match_exact));
	m_render.add(Render::Object("amenityparking", 246, 238, 183, 255,
				    "amenity", "parking", Render::Object::AttrMatch::match_exact));
	m_render.add(Render::Object("waterway", 181, 208, 208, 255,
				    "waterway", "", Render::Object::AttrMatch::match_startswith));
	m_render.add(Render::Object("water", 181, 208, 208, 255,
				    "water", "", Render::Object::AttrMatch::match_startswith));
	m_render.add(Render::Object("leisuresportscentre", 51, 204, 152, 255,
				    "leisure", "sports_centre", Render::Object::AttrMatch::match_exact));
	m_render.add(Render::Object("leisurepitch", 137, 210, 174, 255,
				    "leisure", "pitch", Render::Object::AttrMatch::match_exact));
	m_render.add(Render::Object("railwayrail", 153, 153, 153, 255,
				    "railway", "rail", Render::Object::AttrMatch::match_exact));
	m_render.add(Render::Object("railwayany", 153, 153, 153, 255,
				    "railway", "", Render::Object::AttrMatch::match_startswith));
	m_render.add(Render::Object("powerline", 118, 118, 118, 255,
				    "power", "line", Render::Object::AttrMatch::match_exact));
	m_render.add(Render::Object("background", 241, 238, 232, 255));
}

void OpenStreetMapDraw::set_render_minimal(void)
{
	m_render.clear();
	m_render.add(Render::Object("aerorunwayconcrete", 187, 187, 204, 255,
				    "aeroway", "runway", Render::Object::AttrMatch::match_exact,
				    "surface", "concrete", Render::Object::AttrMatch::match_exact));
	m_render.add(Render::Object("aerorunwaygrass", 27, 184, 6, 255,
				    "aeroway", "runway", Render::Object::AttrMatch::match_exact,
				    "surface", "grass", Render::Object::AttrMatch::match_exact));
	m_render.add(Render::Object("aerorunway", 187, 187, 204, 255,
				    "aeroway", "runway", Render::Object::AttrMatch::match_exact));
	m_render.add(Render::Object("aerotaxiway", 187, 187, 204, 255,
				    "aeroway", "taxiway", Render::Object::AttrMatch::match_exact));
	m_render.add(Render::Object("aeroapron", 232, 209, 254, 255,
				    "aeroway", "apron", Render::Object::AttrMatch::match_exact));
	m_render.add(Render::Object("aeroterminal", 204, 153, 254, 255,
				    "aeroway", "terminal", Render::Object::AttrMatch::match_exact));
	if (false) {
		m_render.add(Render::Object("landusegrass", 205, 234, 176, 255,
					    "landuse", "grass", Render::Object::AttrMatch::match_exact));
		m_render.add(Render::Object("landuseforest", 173, 208, 158, 255,
					    "landuse", "forest", Render::Object::AttrMatch::match_exact));
		m_render.add(Render::Object("landuseresidential", 224, 222, 222, 255,
					    "landuse", "residential", Render::Object::AttrMatch::match_exact));
		m_render.add(Render::Object("landusefarmland", 237, 221, 201, 255,
					    "landuse", "farmland", Render::Object::AttrMatch::match_exact));
		m_render.add(Render::Object("landusefarmyard", 238, 213, 180, 255,
					    "landuse", "farmyard", Render::Object::AttrMatch::match_exact));
	}
	m_render.add(Render::Object("highwaymotorway", 137, 164, 203, 255,
				    "highway", "motorway", Render::Object::AttrMatch::match_startswith));
	m_render.add(Render::Object("highwayprimary", 220, 158, 158, 255,
				    "highway", "primary", Render::Object::AttrMatch::match_exact));
	m_render.add(Render::Object("highwaysecondary", 248, 213, 169, 255,
				    "highway", "secondary", Render::Object::AttrMatch::match_exact));
	m_render.add(Render::Object("highwaytertiary", 247, 247, 185, 255,
				    "highway", "tertiary", Render::Object::AttrMatch::match_exact));
	m_render.add(Render::Object("highwaytrunk", 148, 211, 148, 255,
				    "highway", "trunk", Render::Object::AttrMatch::match_exact));
	m_render.add(Render::Object("waterway", 181, 208, 208, 255,
				    "waterway", "", Render::Object::AttrMatch::match_startswith));
	m_render.add(Render::Object("water", 181, 208, 208, 255,
				    "water", "", Render::Object::AttrMatch::match_startswith));
	m_render.add(Render::Object("background", 241, 238, 232, 255));
}

void OpenStreetMapDraw::set_render_aeroonly(void)
{
	m_render.clear();
	m_render.add(Render::Object("aerorunwayconcrete", 187, 187, 204, 255,
				    "aeroway", "runway", Render::Object::AttrMatch::match_exact,
				    "surface", "concrete", Render::Object::AttrMatch::match_exact));
	m_render.add(Render::Object("aerorunwaygrass", 27, 184, 6, 255,
				    "aeroway", "runway", Render::Object::AttrMatch::match_exact,
				    "surface", "grass", Render::Object::AttrMatch::match_exact));
	m_render.add(Render::Object("aerorunway", 187, 187, 204, 255,
				    "aeroway", "runway", Render::Object::AttrMatch::match_exact));
	m_render.add(Render::Object("aerotaxiway", 187, 187, 204, 255,
				    "aeroway", "taxiway", Render::Object::AttrMatch::match_exact));
	m_render.add(Render::Object("aeroapron", 232, 209, 254, 255,
				    "aeroway", "apron", Render::Object::AttrMatch::match_exact));
	m_render.add(Render::Object("aeroterminal", 204, 153, 254, 255,
				    "aeroway", "terminal", Render::Object::AttrMatch::match_exact));
	m_render.add(Render::Object("background", 241, 238, 232, 255));
}

OpenStreetMapDraw::coordobstacles_t OpenStreetMapDraw::draw(const Cairo::RefPtr<Cairo::Context>& cr, double& width, double& height,
							    const Cairo::RefPtr<Cairo::PdfSurface>& surface) const
{
	Point center(m_bbox.boxcenter());
	double scalelon, scalelat;
	{
		Point p(m_bbox.get_northeast() - m_bbox.get_southwest());
		double lonscale(cos(center.get_lat_rad_dbl()));
		scalelat = std::max(p.get_lon() * lonscale / width, (double)p.get_lat() / height);
		scalelat = 1.0 / scalelat;
		scalelon = scalelat * lonscale;
		height = p.get_lat() * scalelat;
		width = p.get_lon() * scalelon;
		scalelat = -scalelat;
	}
	if (surface)
		surface->set_size(width, height);
	cr->save();
	// background
	{
		const Render::Object& bgnd(get_render().get_background());
		bgnd.set_color(cr);
		cr->paint();
	}
	// clip
	cr->rectangle(0, 0, width, height);
	cr->clip();
	cr->translate(width * 0.5, height * 0.5);
	// scaling
	cr->select_font_face("sans", Cairo::FONT_SLANT_NORMAL, Cairo::FONT_WEIGHT_NORMAL);
	cr->set_font_size(12);
	zset_t zset(get_zset());
	for (zset_t::const_iterator zi(zset.begin()), ze(zset.end()); zi != ze; ++zi) {
		for (osmpolygons_t::const_iterator i(m_osmpolygons.begin()), e(m_osmpolygons.end()); i != e; ++i) {
			if (i->get_z_order() != *zi)
				continue;
			const Render::Object& obj(get_render().get(*i));
			if (obj.empty())
				continue;
			obj.set_color(cr);
			METARTAFChart::polypath(cr, i->get_poly(), m_bbox, center, scalelon, scalelat);
			cr->fill();
		}
		for (osmlines_t::const_iterator i(m_osmlines.begin()), e(m_osmlines.end()); i != e; ++i) {
			if (i->get_z_order() != *zi)
				continue;
			const Render::Object& obj(get_render().get(*i));
			if (obj.empty())
				continue;
			obj.set_color(cr);
			{
				double w(i->get_width_meter());
				if (std::isnan(w))
					w = 10;
				cr->set_line_width(-w * scalelat * scalemeter);
			}
			METARTAFChart::polypath(cr, i->get_line(), m_bbox, center, scalelon, scalelat);
			cr->stroke();
		}
		for (osmlines_t::const_iterator i(m_osmroads.begin()), e(m_osmroads.end()); i != e; ++i) {
			if (i->get_z_order() != *zi)
				continue;
			const Render::Object& obj(get_render().get(*i));
			if (obj.empty())
				continue;
			obj.set_color(cr);
			{
				double w(i->get_width_meter());
				if (std::isnan(w))
					w = 10;
				cr->set_line_width(-w * scalelat * scalemeter);
			}
			METARTAFChart::polypath(cr, i->get_line(), m_bbox, center, scalelon, scalelat);
			cr->stroke();
		}
		continue;
		// No points yet drawn (could use for trees)
		for (osmpoints_t::const_iterator i(m_osmpoints.begin()), e(m_osmpoints.end()); i != e; ++i) {
			if (i->get_z_order() != *zi)
				continue;
			const Render::Object& obj(get_render().get(*i));
			if (obj.empty())
				continue;
		}
	}
	if (!m_arp.is_invalid() && m_bbox.is_inside(m_arp)) {
		double xc, yc;
		{
			Point pdiff(m_arp - center);
			xc = pdiff.get_lon() * scalelon;
			yc = pdiff.get_lat() * scalelat;
		}
		cr->save();
		cr->translate(xc, yc);
		set_color_arpbgnd(cr);
		cr->set_line_width(0.5 * arp_cairo_radius);
		cr->arc(0.0, 0.0, arp_cairo_radius, 0.0, 2.0 * M_PI);
		cr->close_path();
		cr->move_to(-arp_cairo_radius, 0);
		cr->line_to(arp_cairo_radius, 0);
		cr->move_to(0, -arp_cairo_radius);
		cr->line_to(0, arp_cairo_radius);
		cr->stroke();
		set_color_arp(cr);
		cr->set_line_width(0.2 * arp_cairo_radius);
		cr->arc(0.0, 0.0, arp_cairo_radius, 0.0, 2.0 * M_PI);
		cr->close_path();
		cr->move_to(-arp_cairo_radius, 0);
		cr->line_to(arp_cairo_radius, 0);
		cr->move_to(0, -arp_cairo_radius);
		cr->line_to(0, arp_cairo_radius);
		cr->stroke();
		cr->restore();
	}
	coordobstacles_t ob;
	for (obstacles_t::const_iterator oi(m_obstacles.begin()), oe(m_obstacles.end()); oi != oe; ++oi) {
		static const double obstacle_cairo_radius_1(1.2 * obstacle_cairo_radius);
		static const double obstacle_cairo_radius_x1(1.2 * 0.50000 * obstacle_cairo_radius);
		static const double obstacle_cairo_radius_y1(1.2 * 0.86603 * obstacle_cairo_radius);
		static const double obstacle_cairo_radius_2(1.8 * obstacle_cairo_radius);
		static const double obstacle_cairo_radius_x2(1.8 * 0.50000 * obstacle_cairo_radius);
		static const double obstacle_cairo_radius_y2(1.8 * 0.86603 * obstacle_cairo_radius);
		if (oi->get_pt().is_invalid() || !m_bbox.is_inside(oi->get_pt()))
			continue;
		double xc, yc;
		{
			Point pdiff(oi->get_pt() - center);
			xc = pdiff.get_lon() * scalelon;
			yc = pdiff.get_lat() * scalelat;
		}
		ob.push_back(CoordObstacle(*oi, xc + width * 0.5, yc + height * 0.5));
		cr->save();
		cr->translate(xc, yc);
		set_color_obstaclebgnd(cr);
		cr->set_line_width(0.6 * obstacle_cairo_radius);
		cr->arc(0.0, 0.0, obstacle_cairo_radius, 0.0, 2.0 * M_PI);
		cr->close_path();
		cr->move_to(-obstacle_cairo_radius, 0);
		cr->line_to(obstacle_cairo_radius, 0);
		cr->move_to(0, -obstacle_cairo_radius);
		cr->line_to(0, obstacle_cairo_radius);
		if (oi->is_lighted()) {
			cr->move_to(0, -obstacle_cairo_radius_1);
			cr->line_to(0, -obstacle_cairo_radius_2);
			cr->move_to(obstacle_cairo_radius_x1, -obstacle_cairo_radius_y1);
			cr->line_to(obstacle_cairo_radius_x2, -obstacle_cairo_radius_y2);
			cr->move_to(-obstacle_cairo_radius_x1, -obstacle_cairo_radius_y1);
			cr->line_to(-obstacle_cairo_radius_x2, -obstacle_cairo_radius_y2);
		}
		cr->stroke();
		set_color_obstacle(cr);
		cr->set_line_width(0.2 * obstacle_cairo_radius);
		cr->arc(0.0, 0.0, obstacle_cairo_radius, 0.0, 2.0 * M_PI);
		cr->stroke();
		cr->move_to(-obstacle_cairo_radius, 0);
		cr->line_to(obstacle_cairo_radius, 0);
		cr->move_to(0, -obstacle_cairo_radius);
		cr->line_to(0, obstacle_cairo_radius);
		if (oi->is_lighted()) {
			cr->move_to(0, -obstacle_cairo_radius_1);
			cr->line_to(0, -obstacle_cairo_radius_2);
			cr->move_to(obstacle_cairo_radius_x1, -obstacle_cairo_radius_y1);
			cr->line_to(obstacle_cairo_radius_x2, -obstacle_cairo_radius_y2);
			cr->move_to(-obstacle_cairo_radius_x1, -obstacle_cairo_radius_y1);
			cr->line_to(-obstacle_cairo_radius_x2, -obstacle_cairo_radius_y2);
		}
		cr->stroke();
		{
			double val(std::numeric_limits<double>::quiet_NaN());
			if (!std::isnan(oi->get_agl()) && (!obstacle_preferamsl || std::isnan(oi->get_amsl()))) {
				val = oi->get_agl();
				cr->select_font_face("sans", Cairo::FONT_SLANT_NORMAL, Cairo::FONT_WEIGHT_NORMAL);
				cr->set_font_size(obstacle_cairo_fontsize);
			} else if (!std::isnan(oi->get_amsl())) {
				val = oi->get_amsl();
				cr->select_font_face("sans", Cairo::FONT_SLANT_ITALIC, Cairo::FONT_WEIGHT_NORMAL);
				cr->set_font_size(obstacle_cairo_fontsize);
			}
			if (!std::isnan(val)) {
				val *= Point::m_to_ft_dbl;
				std::ostringstream oss;
				oss << std::fixed << std::setprecision(0) << val;
				Cairo::TextExtents ext;
				cr->get_text_extents(oss.str(), ext);
				cr->move_to(1.2 * obstacle_cairo_radius - ext.x_bearing, -0.5 * ext.height - ext.y_bearing);
				set_color_obstaclebgnd(cr);
				cr->text_path(oss.str());
				cr->set_line_width(0.5 * obstacle_cairo_radius);
				cr->stroke_preserve();
				set_color_obstacle(cr);
				cr->fill();
			}
		}
		cr->restore();
	}
	// end
	cr->restore();
	return ob;
}

OpenStreetMapDraw::coordobstacles_t OpenStreetMapDraw::draw(std::ostream& os, double& width, double& height) const
{
	Point center(m_bbox.boxcenter());
	double scalelon, scalelat;
	{
		Point p(m_bbox.get_northeast() - m_bbox.get_southwest());
		double lonscale(cos(center.get_lat_rad_dbl()));
		scalelat = std::max(p.get_lon() * lonscale / width, (double)p.get_lat() / height);
		scalelat = 1.0 / scalelat;
		scalelon = scalelat * lonscale;
		height = p.get_lat() * scalelat;
		width = p.get_lon() * scalelon;
	}
	os << "  \\begin{center}\\begin{tikzpicture}" << std::endl
	   << "    \\path[clip] (0,0) rectangle " << METARTAFChart::pgfcoord(width, height) << ';' << std::endl;
	{
		const Render::Object& bgnd(get_render().get_background());
		os << "    \\fill[colosm" << bgnd.get_ident() << "] (0,0) rectangle " << METARTAFChart::pgfcoord(width, height) << ';' << std::endl;
	}
	if (true)
		os << "    \\georefstart{4326}" << std::endl
		   << "    \\georefbound{0}{0}" << std::endl
		   << "    \\georefbound" << METARTAFChart::georefcoord(width, 0) << std::endl
		   << "    \\georefbound" << METARTAFChart::georefcoord(width, height) << std::endl
		   << "    \\georefbound" << METARTAFChart::georefcoord(0, height) << std::endl
		   << "    \\georefmap" << METARTAFChart::georefcoord(m_bbox.get_southwest(), center, width, height, scalelon, scalelat) << std::endl
		   << "    \\georefmap" << METARTAFChart::georefcoord(m_bbox.get_southeast(), center, width, height, scalelon, scalelat) << std::endl
		   << "    \\georefmap" << METARTAFChart::georefcoord(m_bbox.get_northeast(), center, width, height, scalelon, scalelat) << std::endl
		   << "    \\georefmap" << METARTAFChart::georefcoord(m_bbox.get_northwest(), center, width, height, scalelon, scalelat) << std::endl
		   << "    \\georefend" << std::endl;
	zset_t zset(get_zset());
	for (zset_t::const_iterator zi(zset.begin()), ze(zset.end()); zi != ze; ++zi) {
		os << "    % Z Order " << *zi << std::endl;
		for (osmpolygons_t::const_iterator i(m_osmpolygons.begin()), e(m_osmpolygons.end()); i != e; ++i) {
			if (i->get_z_order() != *zi)
				continue;
			const Render::Object& obj(get_render().get(*i));
			if (obj.empty())
				continue;
			METARTAFChart::polypath_fill(os, "fill=colosm" + obj.get_ident(), i->get_poly(),
						     center, width, height, scalelon, scalelat);
		}
		for (osmlines_t::const_iterator i(m_osmlines.begin()), e(m_osmlines.end()); i != e; ++i) {
			if (i->get_z_order() != *zi)
				continue;
			const Render::Object& obj(get_render().get(*i));
			if (obj.empty())
				continue;
			std::ostringstream attr;
			attr << "draw=colosm" << obj.get_ident();
			{
				double w(i->get_width_meter());
				if (!std::isnan(w)) {
					os << "    % line width meter " << w << " scaled " << (w * scalelat * scalemeter) << std::endl;
					attr << ",line width=" << std::fixed << (w * scalelat * scalemeter);
				}
			}
			METARTAFChart::polypath(os, attr.str(), i->get_line(),
						m_bbox, center, width, height, scalelon, scalelat);
		}
		for (osmlines_t::const_iterator i(m_osmroads.begin()), e(m_osmroads.end()); i != e; ++i) {
			if (i->get_z_order() != *zi)
				continue;
			const Render::Object& obj(get_render().get(*i));
			if (obj.empty())
				continue;
			std::ostringstream attr;
			attr << "draw=colosm" << obj.get_ident();
			{
				double w(i->get_width_meter());
				if (!std::isnan(w)) {
					os << "    % line width meter " << w << " scaled " << (w * scalelat * scalemeter) << std::endl;
					attr << ",line width=" << std::fixed << (w * scalelat * scalemeter);
				}
			}
			METARTAFChart::polypath(os, attr.str(), i->get_line(),
						m_bbox, center, width, height, scalelon, scalelat);
		}
		continue;
		// No points yet drawn (could use for trees)
		for (osmpoints_t::const_iterator i(m_osmpoints.begin()), e(m_osmpoints.end()); i != e; ++i) {
			if (i->get_z_order() != *zi)
				continue;
			const Render::Object& obj(get_render().get(*i));
			if (obj.empty())
				continue;
		}
	}
	// FIXME: obstacle
	// end
	os << "  \\end{tikzpicture}\\end{center}" << std::endl << std::endl;
	return coordobstacles_t();
}

std::ostream& OpenStreetMapDraw::define_latex_colors(std::ostream& os) const
{
	return get_render().define_latex_colors(os);
}

