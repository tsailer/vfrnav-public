//
// C++ Interface: geomboost
//
// Description:
//
//
// Author: Thomas Sailer <t.sailer@alumni.ethz.ch>, (C) 2007, 2009, 2012
//
// Copyright: See COPYING file that comes with this distribution
//
//

#ifndef GEOMBOOST_H
#define GEOMBOOST_H

#include "sysdeps.h"
#include "geom.h"

#ifdef HAVE_BOOST_GEOMETRY_HPP
#include <boost/config.hpp>
#include <boost/geometry.hpp>
#include <boost/geometry/geometries/register/point.hpp>

namespace boost { namespace geometry {

struct garmin {};

namespace detail {

template<std::size_t Dimension, typename Geometry>
struct garmin_radian_converter
{
	typedef typename fp_coordinate_type<Geometry>::type coordinate_type;

	static inline coordinate_type get(Geometry const& geometry)
	{
		return boost::numeric_cast<coordinate_type>(geometry::get<Dimension>(geometry) * Point::to_rad_dbl);
	}

	static inline void set(Geometry& geometry, coordinate_type const& radians)
	{
		geometry::set<Dimension>(geometry, boost::numeric_cast<coordinate_type>(radians * Point::from_rad_dbl));
	}
};

template<typename Geometry, template<typename> class CoordinateSystem> struct radian_access<0, Geometry, CoordinateSystem<garmin> >
	: garmin_radian_converter<0, Geometry>
{};

template<typename Geometry, template<typename> class CoordinateSystem> struct radian_access<1, Geometry, CoordinateSystem<garmin> >
	: garmin_radian_converter<1, Geometry>
{};

}}}

//BOOST_GEOMETRY_REGISTER_POINT_2D_GET_SET(Point, Point::coord_t, boost::geometry::cs::geographic<boost::geometry::garmin>, get_lon, get_lat, set_lon, set_lat)
BOOST_GEOMETRY_REGISTER_POINT_2D_GET_SET(Point, Point::coord_t, boost::geometry::cs::spherical_equatorial<boost::geometry::garmin>, get_lon, get_lat, set_lon, set_lat)
//BOOST_GEOMETRY_REGISTER_POINT_2D_GET_SET(Point, double, boost::geometry::cs::geographic<radian>, get_lon_rad_dbl, get_lat_rad_dbl, set_lon_rad_dbl, set_lat_rad_dbl)
//BOOST_GEOMETRY_REGISTER_POINT_2D_GET_SET(Point, double, boost::geometry::cs::spherical_equatorial<radian>, get_lon_rad_dbl, get_lat_rad_dbl, set_lon_rad_dbl, set_lat_rad_dbl)

namespace boost { namespace geometry { namespace traits {

template<> struct tag<PolygonSimple>
{
	typedef ring_tag type;
};

template<> struct point_order<PolygonSimple>
{
	static const order_selector value = counterclockwise;
};

template<> struct closure<PolygonSimple>
{
	static const closure_selector value = open;
};

template<> struct tag<PolygonHole>
{
	typedef polygon_tag type;
};

template<> struct ring_const_type<PolygonHole>
{
	typedef PolygonSimple const& type;
};

template<> struct ring_mutable_type<PolygonHole>
{
	typedef PolygonSimple& type;
};

template<> struct interior_const_type<PolygonHole>
{
	typedef PolygonHole::interior_t const& type;
};

template<> struct interior_mutable_type<PolygonHole>
{
	typedef PolygonHole::interior_t& type;
};

template<> struct exterior_ring<PolygonHole>
{
	static inline PolygonSimple& get(PolygonHole& p)
	{
		return p.get_exterior();
	}

	static inline PolygonSimple const& get(PolygonHole const& p)
	{
		return p.get_exterior();
	}
};

template<> struct interior_rings<PolygonHole>
{
	static inline PolygonHole::interior_t& get(PolygonHole& p)
	{
		return p.get_interior();
	}

	static inline PolygonHole::interior_t const& get(PolygonHole const& p)
	{
		return p.get_interior();
	}
};

}}}

#endif /* HAVE_BOOST_GEOMETRY_HPP */

#endif /* GEOMBOOST_H */
