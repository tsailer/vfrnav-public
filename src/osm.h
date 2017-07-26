//
// C++ Interface: osm
//
// Description: OpenStreetMap
//
//
// Author: Thomas Sailer <t.sailer@alumni.ethz.ch>, (C) 2015
//
// Copyright: See COPYING file that comes with this distribution
//
//


#ifndef OSM_H
#define OSM_H

#include <glibmm.h>
#include <iostream>
#include <cairomm/cairomm.h>

#include "sysdeps.h"
#include "geom.h"

class OpenStreetMap {
public:
	class Tags : public std::map<std::string,std::string> {
	public:
		void add_pg(const std::string& x);
		std::string to_str(void) const;
	};

	class OSMObject {
	public:
		OSMObject(int64_t id = 0, const std::string& access = "", 
			  const std::string& addr_housename = "", const std::string& addr_housenumber = "", 
			  const std::string& addr_interpolation = "", const std::string& admin_level = "", 
			  const std::string& aerialway = "", const std::string& aeroway = "", 
			  const std::string& amenity = "", const std::string& area = "", 
			  const std::string& barrier = "", const std::string& bicycle = "", 
			  const std::string& brand = "", const std::string& bridge = "", 
			  const std::string& boundary = "", const std::string& building = "", 
			  const std::string& construction = "", const std::string& covered = "",
			  const std::string& culvert = "", const std::string& cutting = "",
			  const std::string& denomination = "", const std::string& disused = "",
			  const std::string& embankment = "", const std::string& foot = "", 
			  const std::string& generator_source = "", const std::string& harbour = "", 
			  const std::string& highway = "", const std::string& historic = "", 
			  const std::string& horse = "", const std::string& intermittent = "", 
			  const std::string& junction = "", const std::string& landuse = "", 
			  const std::string& layer = "", const std::string& leisure = "", 
			  const std::string& lock = "", const std::string& man_made = "", 
			  const std::string& military = "", const std::string& motorcar = "", 
			  const std::string& name = "", const std::string& natural = "", 
			  const std::string& office = "", const std::string& oneway = "", 
			  const std::string& oper = "", const std::string& place = "", 
			  const std::string& population = "", const std::string& power = "",
			  const std::string& power_source = "", const std::string& public_transport = "",
			  const std::string& railway = "", const std::string& ref = "",
			  const std::string& religion = "", const std::string& route = "",
			  const std::string& service = "", const std::string& shop = "",
			  const std::string& sport = "", const std::string& surface = "",
			  const std::string& toll = "", const std::string& tourism = "",
			  const std::string& tower_type = "", const std::string& tunnel = "",
			  const std::string& water = "", const std::string& waterway = "",
			  const std::string& wetland = "", const std::string& width = "",
			  const std::string& wood = "", int32_t z_order = 0, const Tags& tags = Tags());

		int64_t get_id(void) const { return m_id; }
		const std::string& get_access(void) const { return m_access; }
		const std::string& get_addr_housename(void) const { return m_addr_housename; }
		const std::string& get_addr_housenumber(void) const { return m_addr_housenumber; }
		const std::string& get_addr_interpolation(void) const { return m_addr_interpolation; }
		const std::string& get_admin_level(void) const { return m_admin_level; }
		const std::string& get_aerialway(void) const { return m_aerialway; }
		const std::string& get_aeroway(void) const { return m_aeroway; }
		const std::string& get_amenity(void) const { return m_amenity; }
		const std::string& get_area(void) const { return m_area; }
		const std::string& get_barrier(void) const { return m_barrier; }
		const std::string& get_bicycle(void) const { return m_bicycle; }
		const std::string& get_brand(void) const { return m_brand; }
		const std::string& get_bridge(void) const { return m_bridge; }
		const std::string& get_boundary(void) const { return m_boundary; }
		const std::string& get_building(void) const { return m_building; }
		const std::string& get_construction(void) const { return m_construction; }
		const std::string& get_covered(void) const { return m_covered; }
		const std::string& get_culvert(void) const { return m_culvert; }
		const std::string& get_cutting(void) const { return m_cutting; }
		const std::string& get_denomination(void) const { return m_denomination; }
		const std::string& get_disused(void) const { return m_disused; }
		const std::string& get_embankment(void) const { return m_embankment; }
		const std::string& get_foot(void) const { return m_foot; }
		const std::string& get_generator_source(void) const { return m_generator_source; }
		const std::string& get_harbour(void) const { return m_harbour; }
		const std::string& get_highway(void) const { return m_highway; }
		const std::string& get_historic(void) const { return m_historic; }
		const std::string& get_horse(void) const { return m_horse; }
		const std::string& get_intermittent(void) const { return m_intermittent; }
		const std::string& get_junction(void) const { return m_junction; }
		const std::string& get_landuse(void) const { return m_landuse; }
		const std::string& get_layer(void) const { return m_layer; }
		const std::string& get_leisure(void) const { return m_leisure; }
		const std::string& get_lock(void) const { return m_lock; }
		const std::string& get_man_made(void) const { return m_man_made; }
		const std::string& get_military(void) const { return m_military; }
		const std::string& get_motorcar(void) const { return m_motorcar; }
		const std::string& get_name(void) const { return m_name; }
		const std::string& get_natural(void) const { return m_natural; }
		const std::string& get_office(void) const { return m_office; }
		const std::string& get_oneway(void) const { return m_oneway; }
		const std::string& get_operator(void) const { return m_operator; }
		const std::string& get_place(void) const { return m_place; }
		const std::string& get_population(void) const { return m_population; }
		const std::string& get_power(void) const { return m_power; }
		const std::string& get_power_source(void) const { return m_power_source; }
		const std::string& get_public_transport(void) const { return m_public_transport; }
		const std::string& get_railway(void) const { return m_railway; }
		const std::string& get_ref(void) const { return m_ref; }
		const std::string& get_religion(void) const { return m_religion; }
		const std::string& get_route(void) const { return m_route; }
		const std::string& get_service(void) const { return m_service; }
		const std::string& get_shop(void) const { return m_shop; }
		const std::string& get_sport(void) const { return m_sport; }
		const std::string& get_surface(void) const { return m_surface; }
		const std::string& get_toll(void) const { return m_toll; }
		const std::string& get_tourism(void) const { return m_tourism; }
		const std::string& get_tower_type(void) const { return m_tower_type; }
		const std::string& get_tunnel(void) const { return m_tunnel; }
		const std::string& get_water(void) const { return m_water; }
		const std::string& get_waterway(void) const { return m_waterway; }
		const std::string& get_wetland(void) const { return m_wetland; }
		const std::string& get_width(void) const { return m_width; }
		const std::string& get_wood(void) const { return m_wood; }
		int32_t get_z_order(void) const { return m_z_order; }
		const Tags& get_tags(void) const { return m_tags; }
		double get_width_meter(void) const;
		std::string to_str(void) const;
		bool is_attr(const std::string& attr) const;
		const std::string& get_attr(const std::string& attr) const;

	protected:
		int64_t m_id;
		std::string m_access;
		std::string m_addr_housename;
		std::string m_addr_housenumber;
		std::string m_addr_interpolation;
		std::string m_admin_level;
		std::string m_aerialway;
		std::string m_aeroway;
		std::string m_amenity;
		std::string m_area;
		std::string m_barrier;
		std::string m_bicycle;
		std::string m_brand;
		std::string m_bridge;
		std::string m_boundary;
		std::string m_building;
		std::string m_construction;
		std::string m_covered;
		std::string m_culvert;
		std::string m_cutting;
		std::string m_denomination;
		std::string m_disused;
		std::string m_embankment;
		std::string m_foot;
		std::string m_generator_source;
		std::string m_harbour;
		std::string m_highway;
		std::string m_historic;
		std::string m_horse;
		std::string m_intermittent;
		std::string m_junction;
		std::string m_landuse;
		std::string m_layer;
		std::string m_leisure;
		std::string m_lock;
		std::string m_man_made;
		std::string m_military;
		std::string m_motorcar;
		std::string m_name;
		std::string m_natural;
		std::string m_office;
		std::string m_oneway;
		std::string m_operator;
		std::string m_place;
		std::string m_poi;
		std::string m_population;
		std::string m_power;
		std::string m_power_source;
		std::string m_public_transport;
		std::string m_railway;
		std::string m_ref;
		std::string m_religion;
		std::string m_route;
		std::string m_service;
		std::string m_shop;
		std::string m_sport;
		std::string m_surface;
		std::string m_toll;
		std::string m_tourism;
		std::string m_tower_type;
		std::string m_tunnel;
		std::string m_water;
		std::string m_waterway;
		std::string m_wetland;
		std::string m_width;
		std::string m_wood;
		int32_t m_z_order;
		Tags m_tags;

		std::pair<const std::string&,bool> get_fixed_attr(const std::string& attr) const;
	};

	class OSMPoint : public OSMObject {
	public:
		OSMPoint(int64_t id = 0, const std::string& access = "", 
			 const std::string& addr_housename = "", const std::string& addr_housenumber = "", 
			 const std::string& addr_interpolation = "", const std::string& admin_level = "", 
			 const std::string& aerialway = "", const std::string& aeroway = "", 
			 const std::string& amenity = "", const std::string& area = "", 
			 const std::string& barrier = "", const std::string& bicycle = "", 
			 const std::string& brand = "", const std::string& bridge = "", 
			 const std::string& boundary = "", const std::string& building = "", 
			 const std::string& capital = "", const std::string& construction = "", 
			 const std::string& covered = "", const std::string& culvert = "", 
			 const std::string& cutting = "", const std::string& denomination = "", 
			 const std::string& disused = "", const std::string& ele = "", 
			 const std::string& embankment = "", const std::string& foot = "", 
			 const std::string& generator_source = "", const std::string& harbour = "", 
			 const std::string& highway = "", const std::string& historic = "", 
			 const std::string& horse = "", const std::string& intermittent = "", 
			 const std::string& junction = "", const std::string& landuse = "", 
			 const std::string& layer = "", const std::string& leisure = "", 
			 const std::string& lock = "", const std::string& man_made = "", 
			 const std::string& military = "", const std::string& motorcar = "", 
			 const std::string& name = "", const std::string& natural = "", 
			 const std::string& office = "", const std::string& oneway = "", 
			 const std::string& oper = "", const std::string& place = "", 
			 const std::string& poi = "", const std::string& population = "", 
			 const std::string& power = "", const std::string& power_source = "", 
			 const std::string& public_transport = "", const std::string& railway = "", 
			 const std::string& ref = "", const std::string& religion = "", 
			 const std::string& route = "", const std::string& service = "", 
			 const std::string& shop = "", const std::string& sport = "", 
			 const std::string& surface = "", const std::string& toll = "", 
			 const std::string& tourism = "", const std::string& tower_type = "", 
			 const std::string& tunnel = "", const std::string& water = "", 
			 const std::string& waterway = "", const std::string& wetland = "", 
			 const std::string& width = "", const std::string& wood = "", 
			 int32_t z_order = 0, const Tags& tags = Tags(), const Point& pt = Point());

		const std::string& get_capital(void) const { return m_capital; }
		const std::string& get_ele(void) const { return m_ele; }
		const std::string& get_poi(void) const { return m_poi; }
		const Point& get_pt(void) const { return m_pt; }
		bool is_attr(const std::string& attr) const;
		const std::string& get_attr(const std::string& attr) const;
		std::string to_str(void) const;

	protected:
		std::string m_capital;
		std::string m_ele;
		std::string m_poi;
		Point m_pt;

		std::pair<const std::string&,bool> get_fixed_attr(const std::string& attr) const;
	};

	class OSMObject2D : public OSMObject {
	public:
		OSMObject2D(int64_t id = 0, const std::string& access = "", 
			    const std::string& addr_housename = "", const std::string& addr_housenumber = "", 
			    const std::string& addr_interpolation = "", const std::string& admin_level = "", 
			    const std::string& aerialway = "", const std::string& aeroway = "", 
			    const std::string& amenity = "", const std::string& area = "", 
			    const std::string& barrier = "", const std::string& bicycle = "", 
			    const std::string& brand = "", const std::string& bridge = "", 
			    const std::string& boundary = "", const std::string& building = "", 
			    const std::string& construction = "", const std::string& covered = "",
			    const std::string& culvert = "", const std::string& cutting = "",
			    const std::string& denomination = "", const std::string& disused = "",
			    const std::string& embankment = "", const std::string& foot = "", 
			    const std::string& generator_source = "", const std::string& harbour = "", 
			    const std::string& highway = "", const std::string& historic = "", 
			    const std::string& horse = "", const std::string& intermittent = "", 
			    const std::string& junction = "", const std::string& landuse = "", 
			    const std::string& layer = "", const std::string& leisure = "", 
			    const std::string& lock = "", const std::string& man_made = "", 
			    const std::string& military = "", const std::string& motorcar = "", 
			    const std::string& name = "", const std::string& natural = "", 
			    const std::string& office = "", const std::string& oneway = "", 
			    const std::string& oper = "", const std::string& place = "", 
			    const std::string& population = "", const std::string& power = "",
			    const std::string& power_source = "", const std::string& public_transport = "",
			    const std::string& railway = "", const std::string& ref = "",
			    const std::string& religion = "", const std::string& route = "",
			    const std::string& service = "", const std::string& shop = "",
			    const std::string& sport = "", const std::string& surface = "",
			    const std::string& toll = "", const std::string& tourism = "",
			    const std::string& tower_type = "", const std::string& tracktype = "", 
			    const std::string& tunnel = "", const std::string& water = "", 
			    const std::string& waterway = "", const std::string& wetland = "", 
			    const std::string& width = "", const std::string& wood = "", 
			    int32_t z_order = 0, double way_area = std::numeric_limits<double>::quiet_NaN(),
			    const Tags& tags = Tags());

		const std::string& get_tracktype(void) const { return m_tracktype; }
		double get_way_area(void) const { return m_way_area; }
		bool is_attr(const std::string& attr) const;
		const std::string& get_attr(const std::string& attr) const;
		std::string to_str(void) const;

	protected:
		std::string m_tracktype;
		double m_way_area;

		std::pair<const std::string&,bool> get_fixed_attr(const std::string& attr) const;
	};

	class OSMLine : public OSMObject2D {
	public:
		OSMLine(int64_t id = 0, const std::string& access = "", 
			const std::string& addr_housename = "", const std::string& addr_housenumber = "", 
			const std::string& addr_interpolation = "", const std::string& admin_level = "", 
			const std::string& aerialway = "", const std::string& aeroway = "", 
			const std::string& amenity = "", const std::string& area = "", 
			const std::string& barrier = "", const std::string& bicycle = "", 
			const std::string& brand = "", const std::string& bridge = "", 
			const std::string& boundary = "", const std::string& building = "", 
			const std::string& construction = "", const std::string& covered = "",
			const std::string& culvert = "", const std::string& cutting = "",
			const std::string& denomination = "", const std::string& disused = "",
			const std::string& embankment = "", const std::string& foot = "", 
			const std::string& generator_source = "", const std::string& harbour = "", 
			const std::string& highway = "", const std::string& historic = "", 
			const std::string& horse = "", const std::string& intermittent = "", 
			const std::string& junction = "", const std::string& landuse = "", 
			const std::string& layer = "", const std::string& leisure = "", 
			const std::string& lock = "", const std::string& man_made = "", 
			const std::string& military = "", const std::string& motorcar = "", 
			const std::string& name = "", const std::string& natural = "", 
			const std::string& office = "", const std::string& oneway = "", 
			const std::string& oper = "", const std::string& place = "", 
			const std::string& population = "", const std::string& power = "",
			const std::string& power_source = "", const std::string& public_transport = "",
			const std::string& railway = "", const std::string& ref = "",
			const std::string& religion = "", const std::string& route = "",
			const std::string& service = "", const std::string& shop = "",
			const std::string& sport = "", const std::string& surface = "",
			const std::string& toll = "", const std::string& tourism = "",
			const std::string& tower_type = "", const std::string& tracktype = "", 
			const std::string& tunnel = "", const std::string& water = "", 
			const std::string& waterway = "", const std::string& wetland = "", 
			const std::string& width = "", const std::string& wood = "", 
			int32_t z_order = 0, double way_area = std::numeric_limits<double>::quiet_NaN(),
			const Tags& tags = Tags(), const LineString& line = LineString());

		const LineString& get_line(void) const { return m_line; }
		std::string to_str(void) const;

	protected:
		LineString m_line;
	};

	class OSMPolygon : public OSMObject2D {
	public:
		OSMPolygon(int64_t id = 0, const std::string& access = "", 
			   const std::string& addr_housename = "", const std::string& addr_housenumber = "", 
			   const std::string& addr_interpolation = "", const std::string& admin_level = "", 
			   const std::string& aerialway = "", const std::string& aeroway = "", 
			   const std::string& amenity = "", const std::string& area = "", 
			   const std::string& barrier = "", const std::string& bicycle = "", 
			   const std::string& brand = "", const std::string& bridge = "", 
			   const std::string& boundary = "", const std::string& building = "", 
			   const std::string& construction = "", const std::string& covered = "",
			   const std::string& culvert = "", const std::string& cutting = "",
			   const std::string& denomination = "", const std::string& disused = "",
			   const std::string& embankment = "", const std::string& foot = "", 
			   const std::string& generator_source = "", const std::string& harbour = "", 
			   const std::string& highway = "", const std::string& historic = "", 
			   const std::string& horse = "", const std::string& intermittent = "", 
			   const std::string& junction = "", const std::string& landuse = "", 
			   const std::string& layer = "", const std::string& leisure = "", 
			   const std::string& lock = "", const std::string& man_made = "", 
			   const std::string& military = "", const std::string& motorcar = "", 
			   const std::string& name = "", const std::string& natural = "", 
			   const std::string& office = "", const std::string& oneway = "", 
			   const std::string& oper = "", const std::string& place = "", 
			   const std::string& population = "", const std::string& power = "",
			   const std::string& power_source = "", const std::string& public_transport = "",
			   const std::string& railway = "", const std::string& ref = "",
			   const std::string& religion = "", const std::string& route = "",
			   const std::string& service = "", const std::string& shop = "",
			   const std::string& sport = "", const std::string& surface = "",
			   const std::string& toll = "", const std::string& tourism = "",
			   const std::string& tower_type = "", const std::string& tracktype = "", 
			   const std::string& tunnel = "", const std::string& water = "", 
			   const std::string& waterway = "", const std::string& wetland = "", 
			   const std::string& width = "", const std::string& wood = "", 
			   int32_t z_order = 0, double way_area = std::numeric_limits<double>::quiet_NaN(),
			   const Tags& tags = Tags(), const MultiPolygonHole& poly = MultiPolygonHole());

		const MultiPolygonHole& get_poly(void) const { return m_poly; }
		std::string to_str(void) const;

	protected:
		MultiPolygonHole m_poly;
	};

	OpenStreetMap(const Rect& bbox = Rect());
	void load(const std::string& db);
	bool empty(void) const;
	const Rect& get_bbox(void) const { return m_bbox; }
	typedef std::vector<OSMPoint> osmpoints_t;
	typedef std::vector<OSMLine> osmlines_t;
	typedef std::vector<OSMPolygon> osmpolygons_t;
	const osmpoints_t& get_points(void) const { return m_osmpoints; }
	const osmlines_t& get_lines(void) const { return m_osmlines; }
	const osmlines_t& get_roads(void) const { return m_osmroads; }
	const osmpolygons_t& get_polygons(void) const { return m_osmpolygons; }
	typedef std::set<int32_t> zset_t;
	zset_t get_zset(void) const;

protected:
	class PGTransactor;
	Rect m_bbox;
	osmpoints_t m_osmpoints;
	osmlines_t m_osmlines;
	osmlines_t m_osmroads;
	osmpolygons_t m_osmpolygons;
};

class OpenStreetMapDraw : public OpenStreetMap {
public:
	class Render {
	public:
		class Object {
		public:
			class AttrMatch {
			public:
				typedef enum {
					match_exact,
					match_startswith
				} match_t;

				AttrMatch(const std::string& attr = "", const std::string& value = "", match_t match = match_exact);
				const std::string& get_attr(void) const { return m_attr; }
				const std::string& get_value(void) const { return m_value; }
				match_t get_match(void) const { return m_match; }
				bool is_match(const OSMObject& x) const;
				bool is_match(const OSMPoint& x) const;
				bool is_match(const OSMObject2D& x) const;

			protected:
				std::string m_attr;
				std::string m_value;
				match_t m_match;
			};

		protected:
			typedef std::vector<AttrMatch> matches_t;

		public:
			Object(const std::string& ident = "", uint8_t colr = 0, uint8_t colg = 0, uint8_t colb = 0, uint8_t cola = 255);
			Object(const std::string& ident, uint8_t colr, uint8_t colg, uint8_t colb, uint8_t cola,
			       const std::string& attr1, const std::string& value1, AttrMatch::match_t match1 = AttrMatch::match_exact);
			Object(const std::string& ident, uint8_t colr, uint8_t colg, uint8_t colb, uint8_t cola,
			       const std::string& attr1, const std::string& value1, AttrMatch::match_t match1,
			       const std::string& attr2, const std::string& value2, AttrMatch::match_t match2 = AttrMatch::match_exact);
			Object(const std::string& ident, uint8_t colr, uint8_t colg, uint8_t colb, uint8_t cola,
			       const std::string& attr1, const std::string& value1, AttrMatch::match_t match1,
			       const std::string& attr2, const std::string& value2, AttrMatch::match_t match2,
			       const std::string& attr3, const std::string& value3, AttrMatch::match_t match3 = AttrMatch::match_exact);
			const std::string& get_ident(void) const { return m_ident; }
			uint8_t get_colr(void) const { return m_colr; }
			uint8_t get_colg(void) const { return m_colg; }
			uint8_t get_colb(void) const { return m_colb; }
			uint8_t get_cola(void) const { return m_cola; }

			bool is_match(const OSMObject& x) const;
			bool is_match(const OSMPoint& x) const;
			bool is_match(const OSMObject2D& x) const;
			void set_color(const Cairo::RefPtr<Cairo::Context>& cr) const;
			std::ostream& define_latex_color(std::ostream& os) const;

			typedef matches_t::size_type size_type;
			typedef matches_t::const_reference const_reference;
			bool empty(void) const { return m_matches.empty(); }
			size_type size(void) const { return m_matches.size(); }
			const_reference operator[](size_type i) const { return m_matches[i]; }

		protected:
			static constexpr double colscale = 1.0 / 255.0;
			matches_t m_matches;
			std::string m_ident;
			uint8_t m_colr;
			uint8_t m_colg;
			uint8_t m_colb;
			uint8_t m_cola;
		};

		Render(void);
		
		void clear(void);
		void add(const Object& obj);
		const Object& get(const OSMObject& x) const;
		const Object& get(const OSMPoint& x) const;
		const Object& get(const OSMObject2D& x) const;
		const Object& get_background(void) const;
		std::ostream& define_latex_colors(std::ostream& os) const;

	protected:
		static const Object dflt;
		typedef std::vector<Object> objects_t;
		objects_t m_objects;
	};

	class Obstacle {
	public:
		Obstacle(const Point& pt = Point::invalid,
			 double agl = std::numeric_limits<double>::quiet_NaN(),
			 double amsl = std::numeric_limits<double>::quiet_NaN(),
			 bool marked = false, bool lighted = false,
			 const std::string& hyperlink = "");
		const Point& get_pt(void) const { return m_pt; }
		double get_agl(void) const { return m_agl; }
		double get_amsl(void) const { return m_amsl; }
		bool is_marked(void) const { return m_marked; }
		bool is_lighted(void) const { return m_lighted; }
		const std::string& get_hyperlink(void) const { return m_hyperlink; }

	protected:
		Point m_pt;
		double m_agl;
		double m_amsl;
		std::string m_hyperlink;
		bool m_marked;
		bool m_lighted;
	};

	typedef std::vector<Obstacle> obstacles_t;

	class CoordObstacle : public Obstacle {
	public:
		CoordObstacle(const Obstacle& ob = Obstacle(),
			      double x = std::numeric_limits<double>::quiet_NaN(),
			      double y = std::numeric_limits<double>::quiet_NaN());
		double get_x(void) const { return m_x; }
		double get_y(void) const { return m_y; }

	protected:
		double m_x;
		double m_y;
	};

	typedef std::vector<CoordObstacle> coordobstacles_t;

	OpenStreetMapDraw(const Rect& bbox = Rect(), const Point& arp = Point::invalid);
	const Render& get_render(void) const { return m_render; }
	void set_render(const Render& r) { m_render = r; }
	void set_render_default(void);
	void set_render_minimal(void);
	void set_render_aeroonly(void);
	const obstacles_t& get_obstacles(void) const { return m_obstacles; }
	obstacles_t& get_obstacles(void) { return m_obstacles; }
	void set_obstacles(const obstacles_t& x) { m_obstacles = x; }
	const Point& get_arp(void) const { return m_arp; }
	void set_arp(const Point& arp) { m_arp = arp; }
	coordobstacles_t draw(const Cairo::RefPtr<Cairo::Context>& cr, double& width, double& height,
			      const Cairo::RefPtr<Cairo::PdfSurface>& surface = Cairo::RefPtr<Cairo::PdfSurface>()) const;
	coordobstacles_t draw(std::ostream& os, double& width, double& height) const;
	std::ostream& define_latex_colors(std::ostream& os) const;

protected:
	static const double scalemeter;
	static constexpr double obstacle_cairo_radius = 3;
	static constexpr double obstacle_cairo_fontsize = 8;
	static const bool obstacle_preferamsl = true;
	static constexpr double arp_cairo_radius = 3;
	Render m_render;
	obstacles_t m_obstacles;
	Point m_arp;

	static inline void set_color_obstacle(const Cairo::RefPtr<Cairo::Context>& cr) {
                cr->set_source_rgb(1, 0, 0);
        }

	static inline void set_color_obstaclebgnd(const Cairo::RefPtr<Cairo::Context>& cr) {
                cr->set_source_rgb(1, 1, 1);
        }

	static inline void set_color_arp(const Cairo::RefPtr<Cairo::Context>& cr) {
                cr->set_source_rgb(0, 0, 0);
        }

	static inline void set_color_arpbgnd(const Cairo::RefPtr<Cairo::Context>& cr) {
                cr->set_source_rgb(1, 1, 1);
        }
};

#endif /* OSM_H */

/* Local Variables: */
/* mode: c++ */
/* End: */
