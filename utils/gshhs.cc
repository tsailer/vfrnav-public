/*
 * gshhs.cc: GSHHS water line to DB conversion utility
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation; either version 2 of the License, or (at your
 * option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General
 * Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program; if not, write to the Free Software Foundation, Inc.,
 * 59 Temple Place, Suite 330, Boston, MA 02111-1307 USA
 *
 */

#include "sysdeps.h"

#include <unistd.h>
#include <getopt.h>
#include <iostream>
#include <sstream>
#include <sqlite3x.hpp>
#include <sqlite3.h>
#include <stdlib.h>
#include <errno.h>
#include <stdexcept>
#include <stdarg.h>

#include <map>
#include <vector>
#include <limits>
#include <cctype>

#if defined(HAVE_GDAL)
#if defined(HAVE_GDAL2)
#include <gdal.h>
#include <ogr_geometry.h>
#include <ogr_feature.h>
#include <ogrsf_frmts.h>
#else
#include <gdal.h>
#include <ogrsf_frmts.h>
#endif
#endif

#include "geom.h"
#include "dbobj.h"

/* ---------------------------------------------------------------------- */

class GSHHSParser {
public:
	GSHHSParser(const Glib::ustring& fname, const Rect& rect = Rect(Point(0, 0), Point(-1, -1)));
	~GSHHSParser();
	std::ostream& info(std::ostream& os);
	void save(WaterelementsDbQueryInterface& db);

protected:
	OGRLayer& get_layer(const std::string& name);
	static PolygonSimple get_polyline(OGRGeometry *geom);
	static PolygonSimple get_polyline(OGRLineString *geom);
	static PolygonHole get_polygon(OGRGeometry *geom);
	static PolygonHole get_polygon(OGRPolygon *geom);
	static Point get_point(OGRGeometry *geom);
	static Point get_point(OGRPoint *geom);
	static std::string trim(const std::string& x);
	void save(WaterelementsDbQueryInterface& db, WaterelementsDb::Waterelement& e);
	void save_area(WaterelementsDbQueryInterface& db, OGRFeature *feature, WaterelementsDb::Waterelement::type_t typ);
	void save_line(WaterelementsDbQueryInterface& db, OGRFeature *feature, WaterelementsDb::Waterelement::type_t typ);
	void save_point(WaterelementsDbQueryInterface& db, OGRFeature *feature, WaterelementsDb::Waterelement::type_t typ);
	void save_layer(WaterelementsDbQueryInterface& db, OGRLayer& layer, WaterelementsDb::Waterelement::type_t typ);
	void save_layer(WaterelementsDbQueryInterface& db, const std::string& layername, WaterelementsDb::Waterelement::type_t typ);
	void save_layer(WaterelementsDbQueryInterface& db, OGRLayer& layer);

private:
#if defined(HAVE_GDAL2)
	GDALDataset *m_ds;
#else
	OGRDataSource *m_ds;
#endif
	Rect m_rect;
	unsigned int m_recs;
};

GSHHSParser::GSHHSParser(const Glib::ustring & fname, const Rect& rect)
	: m_ds(0), m_rect(rect), m_recs(0)
{
	OGRRegisterAll();
#if defined(HAVE_GDAL2)
	m_ds = (GDALDataset *)GDALOpenEx(fname.c_str(), GDAL_OF_READONLY | GDAL_OF_VECTOR, 0, 0, 0);
#else
	OGRSFDriver *drv(0);
	m_ds = OGRSFDriverRegistrar::Open(fname.c_str(), false, &drv);
#endif
	if (!m_ds)
		throw std::runtime_error("Cannot open GSHHS database " + fname);
}
 
GSHHSParser::~GSHHSParser()
{
#if defined(HAVE_GDAL2)
	GDALClose(m_ds);
#else
	OGRDataSource::DestroyDataSource(m_ds);
#endif
	m_ds = 0;
}

std::ostream & GSHHSParser::info(std::ostream & os)
{
#if !defined(HAVE_GDAL2)
	os << "Name: " << m_ds->GetName() << std::endl;
#endif
	for (int layernr = 0; layernr < m_ds->GetLayerCount(); ++layernr) {
		OGRLayer *layer(m_ds->GetLayer(layernr));
		OGRFeatureDefn *layerdef(layer->GetLayerDefn());
		os << "  Layer Name: \"" << layerdef->GetName() << "\"" << std::endl;
		for (int fieldnr = 0; fieldnr < layerdef->GetFieldCount(); ++fieldnr) {
			OGRFieldDefn *fielddef(layerdef->GetFieldDefn(fieldnr));
			os << "    Field Name: " << fielddef->GetNameRef()
			   << " type " << OGRFieldDefn::GetFieldTypeName(fielddef->GetType())  << std::endl;
		}
		layer->ResetReading();
		while (OGRFeature *feature = layer->GetNextFeature()) {
			os << "  Feature: " << feature->GetFID() << std::endl;
			for (int fieldnr = 0; fieldnr < layerdef->GetFieldCount(); ++fieldnr) {
				OGRFieldDefn *fielddef(layerdef->GetFieldDefn(fieldnr));
				os << "    Field Name: " << fielddef->GetNameRef() << " Value ";
				switch (fielddef->GetType()) {
				case OFTInteger:
					os << feature->GetFieldAsInteger(fieldnr);
					break;

				case OFTReal:
					os << feature->GetFieldAsDouble(fieldnr);
					break;

				case OFTString:
				default:
					os << feature->GetFieldAsString(fieldnr);
					break;
				}
				os << std::endl;
			}
			OGRGeometry *geom(feature->GetGeometryRef());
			char *wkt;
			geom->exportToWkt(&wkt);
			os << "  Geom: " << wkt << std::endl;
			delete wkt;
		}
	}
	return os;
}

OGRLayer& GSHHSParser::get_layer(const std::string& name)
{
	OGRLayer *l(m_ds->GetLayerByName(name.c_str()));
	if (!l)
		throw std::runtime_error("Cannot find layer " + name);
	return *l;
}

PolygonSimple GSHHSParser::get_polyline(OGRLineString *geom)
{
	PolygonSimple p;
	if (!geom)
		return p;
	int pt(geom->getNumPoints());
	if (pt <= 0)
		return p;
	p.reserve(pt);
	for (int i = 0; i < pt; i++) {
		OGRPoint opt;
		geom->getPoint(i, &opt);
		Point ipt;
		ipt.set_lon_deg_dbl(opt.getX());
		ipt.set_lat_deg_dbl(opt.getY());
		p.push_back(ipt);
	}
	return p;
}

PolygonSimple GSHHSParser::get_polyline(OGRGeometry *geom)
{
	if (!geom)
		return PolygonSimple();
	OGRLineString *geoml(dynamic_cast<OGRLineString *>(geom));
	if (!geoml) {
		std::cerr << "Geometry not of type LINESTRING, but " << geom->getGeometryName() << std::endl;
		return PolygonSimple();
	}
	return get_polyline(geoml);
}

PolygonHole GSHHSParser::get_polygon(OGRPolygon *geom)
{
	if (!geom)
		return PolygonHole();
	PolygonHole p(get_polyline(geom->getExteriorRing()));
	for (int i = 0; i < geom->getNumInteriorRings(); i++)
		p.add_interior(get_polyline(geom->getInteriorRing(i)));
	p.remove_redundant_polypoints();
	p.normalize();
	return p;
}

PolygonHole GSHHSParser::get_polygon(OGRGeometry *geom)
{
	if (!geom)
		return PolygonHole();
	OGRPolygon *geomp(dynamic_cast<OGRPolygon *>(geom));
	if (!geomp) {
		std::cerr << "Geometry not of type POLYGON, but " << geom->getGeometryName() << std::endl;
		return PolygonHole();
	}
	return get_polygon(geomp);
}

Point GSHHSParser::get_point(OGRPoint *geom)
{
	if (!geom)
		return Point();
	Point pt;
	pt.set_lon_deg_dbl(geom->getX());
	pt.set_lat_deg_dbl(geom->getY());
	return pt;
}

Point GSHHSParser::get_point(OGRGeometry *geom)
{
	if (!geom)
		return Point();
	OGRPoint *geomp(dynamic_cast<OGRPoint *>(geom));
	if (!geomp) {
		std::cerr << "Geometry not of type POINT, but " << geom->getGeometryName() << std::endl;
		return Point();
	}
	return get_point(geomp);
}

std::string GSHHSParser::trim(const std::string & xx)
{
	std::string x(xx);
	while (!x.empty() && std::isspace(*x.begin()))
		x.erase(x.begin());
	while (!x.empty() && std::isspace(*--x.end()))
		x.erase(--x.end());
	if (x == "UNK")
		return "";
	return x;
}

void GSHHSParser::save(WaterelementsDbQueryInterface& db, WaterelementsDb::Waterelement& e)
{
	if (!e.is_valid())
		return;
	Rect r(e.get_bbox());
	if (!m_rect.is_inside(r.get_southwest()) &&
	    !m_rect.is_inside(r.get_southeast()) &&
	    !m_rect.is_inside(r.get_northwest()) &&
	    !m_rect.is_inside(r.get_northeast()))
		return;
	db.save(e);
	++m_recs;
}

void GSHHSParser::save_area(WaterelementsDbQueryInterface& db, OGRFeature *feature, WaterelementsDb::Waterelement::type_t typ)
{
	PolygonHole p(get_polygon(feature->GetGeometryRef()));
	if (p.get_exterior().empty()) {
		std::cerr << "save_area: empty object" << std::endl;
		return;
	}
	WaterelementsDb::Waterelement e(typ, p);
	save(db, e);
}

void GSHHSParser::save_line(WaterelementsDbQueryInterface& db, OGRFeature *feature, WaterelementsDb::Waterelement::type_t typ)
{
	PolygonHole p(get_polyline(feature->GetGeometryRef()));
	if (p.get_exterior().empty()) {
		std::cerr << "save_line: empty object" << std::endl;
		return;
	}
	WaterelementsDb::Waterelement e(typ, p);
	save(db, e);
}

void GSHHSParser::save_point(WaterelementsDbQueryInterface& db, OGRFeature *feature, WaterelementsDb::Waterelement::type_t typ)
{
	Point pt(get_point(feature->GetGeometryRef()));
	PolygonSimple p;
	p.push_back(pt);
	WaterelementsDb::Waterelement e(typ, p);
	save(db, e);
}

void GSHHSParser::save_layer(WaterelementsDbQueryInterface& db, OGRLayer& layer, WaterelementsDb::Waterelement::type_t typ)
{
	db.purge(typ);
	m_recs = 0;
	OGRFeatureDefn& layerdef(*layer.GetLayerDefn());
	layer.ResetReading();
	while (OGRFeature *feature = layer.GetNextFeature())
		save_area(db, feature, typ);
	std::cerr << "Layer " << layerdef.GetName() << ": " << m_recs << " records, type "
		  << WaterelementsDb::Waterelement::get_typename(typ) << std::endl;
}

void GSHHSParser::save_layer(WaterelementsDbQueryInterface& db, const std::string& layername, WaterelementsDb::Waterelement::type_t typ)
{
	OGRLayer& layer(get_layer(layername));
	save_layer(db, layer, typ);
}

void GSHHSParser::save_layer(WaterelementsDbQueryInterface& db, OGRLayer& layer)
{
	OGRFeatureDefn& layerdef(*layer.GetLayerDefn());
	std::string name(layerdef.GetName());
	if (name.size() != 10 || name.compare(0, 6, "GSHHS_") || name.compare(7, 2, "_L") || name[9] < '1' || name[9] > '6')
		throw std::runtime_error("Unhandled layer " + name);
	save_layer(db, layer, (WaterelementsDb::Waterelement::type_t)(WaterelementsDb::Waterelement::type_landmass + (name[9] - '1')));
}

void GSHHSParser::save(WaterelementsDbQueryInterface& db)
{
	for (int layernr = 0; layernr < m_ds->GetLayerCount(); ++layernr) {
		try {
			save_layer(db, *m_ds->GetLayer(layernr));
		} catch (const std::runtime_error& e) {
			std::cerr << "Error: " << e.what() << std::endl;
		}
	}
}

/* ---------------------------------------------------------------------- */

class MergePoly {
public:
	MergePoly(void);
	MergePoly(const WaterelementsDb::Waterelement& e);
	void add(const WaterelementsDb::Waterelement& e);
	void add(const MergePoly& x);
	void process(void);
	void process(Point::coord_t lonoffs);
	bool is_inside(const Rect& x) const;
	bool is_overlap(const Rect& x) const;
	bool is_overlap(const MergePoly& x) const;
	const Rect& get_bbox(void) const { return m_bbox; }
	const MultiPolygonHole& get_poly(void) const { return m_poly[0]; }
	void clear(void);
	bool empty(void) const;
	void save(WaterelementsDbQueryInterface& db) const;

protected:
	MultiPolygonHole m_poly[4];
	Rect m_bbox;
};

MergePoly::MergePoly(void)
{
	m_bbox.set_empty();
}

MergePoly::MergePoly(const WaterelementsDb::Waterelement& e)
{
	unsigned int t(e.get_type() - WaterelementsDb::Waterelement::type_landmass);
	if (t < 4) {
		m_poly[t].push_back(e.get_polygon());
		m_bbox = m_poly[t].back().get_bbox();
	}
}

void MergePoly::add(const WaterelementsDb::Waterelement& e)
{
	unsigned int t(e.get_type() - WaterelementsDb::Waterelement::type_landmass);
	if (t >= 4)
		return;
	m_poly[t].push_back(e.get_polygon());
	m_bbox = m_bbox.add(m_poly[t].back().get_bbox());
}

void MergePoly::add(const MergePoly& x)
{
	for (unsigned int i = 0; i < 4; ++i)
		m_poly[i].insert(m_poly[i].end(), x.m_poly[i].begin(), x.m_poly[i].end());
	m_bbox = m_bbox.add(x.m_bbox);
}

void MergePoly::process(void)
{
	if (!m_poly[1].empty())
		m_poly[0].geos_subtract(m_poly[1]);
	m_poly[1].clear();
	m_poly[0].geos_union(m_poly[2]);
	m_poly[2].clear();
	if (!m_poly[3].empty())
		m_poly[0].geos_subtract(m_poly[3]);
	m_poly[3].clear();
	m_bbox = m_poly[0].get_bbox();
}

void MergePoly::process(Point::coord_t lonoffs)
{
	m_poly[0].normalize_boostgeom();
	if (!m_poly[1].empty()) {
		m_poly[1].normalize_boostgeom();
		m_poly[0].geos_subtract(m_poly[1], lonoffs);
	}
	m_poly[1].clear();
	m_poly[2].normalize_boostgeom();
	m_poly[0].geos_union(m_poly[2], lonoffs);
	m_poly[2].clear();
	if (!m_poly[3].empty()) {
		m_poly[3].normalize_boostgeom();
		m_poly[0].geos_subtract(m_poly[3], lonoffs);
	}
	m_poly[3].clear();
	m_bbox = m_poly[0].get_bbox();
}

bool MergePoly::is_inside(const Rect& x) const
{
	return x.is_inside(m_bbox);
}

bool MergePoly::is_overlap(const Rect& x) const
{
	return m_bbox.is_intersect(x);
}

bool MergePoly::is_overlap(const MergePoly& x) const
{
	return is_overlap(x.m_bbox);
}

void MergePoly::clear(void)
{
	for (unsigned int i = 0; i < 4; ++i)
		m_poly[i].clear();
}

bool MergePoly::empty(void) const
{
	for (unsigned int i = 0; i < 4; ++i)
		if (!m_poly[i].empty())
			return false;
	return true;
}

void MergePoly::save(WaterelementsDbQueryInterface& db) const
{
	for (MultiPolygonHole::const_iterator pi(m_poly[0].begin()), pe(m_poly[0].end()); pi != pe; ++pi) {
		WaterelementsDb::Waterelement e(WaterelementsDb::Waterelement::type_land, *pi);
		db.save(e);
	}
}

class MergePolygons : public std::vector<MergePoly> {
public:
	void load(WaterelementsDbQueryInterface& db);
	void merge(void);
	void merge(const Rect& window);
	void merge(Point::coord_t window_width, Point::coord_t window_inc);
	void process(void);
	void save(WaterelementsDbQueryInterface& db);
};

void MergePolygons::load(WaterelementsDbQueryInterface& db)
{
	std::cerr << "Loading polygons..." << std::endl;
	MultiPolygonHole mph[3];
	{
		WaterelementsDb::Waterelement e;
		db.loadfirst(e);
		while (e.is_valid()) {
			unsigned int t(e.get_type() - WaterelementsDb::Waterelement::type_landmass);
			if (!t)
				push_back(MergePoly(e));
			else if (t < 4)
				mph[t - 1].push_back(e.get_polygon());
			db.loadnext(e);
		}
	}
	std::cerr << "Loaded " << size() << ',' << mph[0].size() << ',' << mph[1].size() << ',' << mph[2].size() << " polygons" << std::endl;
	for (unsigned int i = 0; i < 3; ++i) {
		for (MultiPolygonHole::const_iterator pi(mph[i].begin()), pe(mph[i].end()); pi != pe; ++pi) {
			WaterelementsDb::Waterelement e((WaterelementsDb::Waterelement::type_t)(WaterelementsDb::Waterelement::type_lake + i), *pi);
			bool work(false);
			for (iterator mi(begin()), me(end()); mi != me; ++mi) {
				if (!mi->is_overlap(e.get_bbox()))
					continue;
				mi->add(e);
				work = true;
			}
			if (work)
				continue;
			push_back(e);
		}
		mph[i].clear();
	}
	std::cerr << "Merged " << size() << " polygons" << std::endl;
}

void MergePolygons::merge(const Rect& window)
{
	size_type sz(size());
	MergePoly mp;
	{
		MergePolygons ep;
		bool work(false);
		for (const_iterator i(begin()), e(end()); i != e; ++i) {
			if (i->is_inside(window)) {
				mp.add(*i);
				work = true;
			} else {
				ep.push_back(*i);
			}
		}
		if (!work)
			return;
		swap(ep);
	}
	if (true)
		std::cerr << "Merge Window " << window.get_southwest().get_lat_str2()
			  << ' ' << window.get_southwest().get_lon_str2()
			  << ' ' << window.get_northeast().get_lat_str2()
			  << ' ' << window.get_northeast().get_lon_str2()
			  << " inside " << (sz - size()) << " outside " << size() << std::endl;
	mp.process(window.get_west());
	push_back(mp);
	if (true)
		std::cerr << "  merged " << sz << " new size " << size() << std::endl;
}

void MergePolygons::merge(Point::coord_t window_width, Point::coord_t window_inc)
{
	Rect window(Point(0, std::numeric_limits<Point::coord_t>::min()),
		    Point(0, std::numeric_limits<Point::coord_t>::max()));
	for (;;) {
		window.set_east(window.get_west() + window_width);
		merge(window);
		{
			uint32_t c(window.get_west()), cn(c + window_inc);
			window.set_west(cn);
			if (cn < c)
				break;
		}
	}
}

void MergePolygons::merge(void)
{
	merge(0x40000000, 0x10000000);
	//merge(0x80000000, 0x20000000);
	merge(0xE0000000, 0x20000000);
}

void MergePolygons::process(void)
{
	for (size_type n(size()), i(0); i < n; ++i) {
		MergePoly& mp(operator[](i));
		std::cerr << "Processing " << i << '/' << n
			  << ' ' << mp.get_bbox().get_southwest().get_lat_str2()
			  << ' ' << mp.get_bbox().get_southwest().get_lon_str2()
			  << ' ' << mp.get_bbox().get_northeast().get_lat_str2()
			  << ' ' << mp.get_bbox().get_northeast().get_lon_str2() << std::endl;
		mp.process();
	}
}

void MergePolygons::save(WaterelementsDbQueryInterface& db)
{
	db.purge(WaterelementsDb::Waterelement::type_land);
	for (const_iterator pi(begin()), pe(end()); pi != pe; ++pi)
		pi->save(db);
}

/* ---------------------------------------------------------------------- */

class ExtPoly {
public:
	ExtPoly(const WaterelementsDb::Waterelement& e);
	ExtPoly(const PolygonHole& p);
	void add(const ExtPoly& x);
	void process(float dist, bool verbose);
	void process(void);
	bool is_inside(const Rect& x) const;
	const Rect& get_bbox(void) const { return m_bbox; }
	const MultiPolygonHole& get_poly(void) const { return m_poly; }
	void save(WaterelementsDbQueryInterface& db) const;

protected:
	MultiPolygonHole m_poly;
	Rect m_bbox;

	void process(const PolygonSimple& ps, float dist);
	void process(const PolygonHole& ps, float dist);
	void process(const MultiPolygonHole& ps, float dist);
};

ExtPoly::ExtPoly(const WaterelementsDb::Waterelement& e)
{
	m_poly.push_back(e.get_polygon());
	m_bbox = m_poly.back().get_bbox();
}

ExtPoly::ExtPoly(const PolygonHole& p)
{
	m_poly.push_back(p);
	m_bbox = m_poly.back().get_bbox();
}

void ExtPoly::add(const ExtPoly& x)
{
	m_poly.insert(m_poly.end(), x.m_poly.begin(), x.m_poly.end());
	m_bbox = m_bbox.add(x.m_bbox);
}

void ExtPoly::process(const PolygonSimple& ps, float dist)
{
	PolygonSimple::size_type n(ps.size());
	if (n < 2)
		return;
	for (PolygonSimple::size_type j(n - 1), i(0); i < n; j = i, ++i)
		m_poly.push_back(PolygonHole(ps[j].make_fat_line(ps[i], dist)));
}

void ExtPoly::process(const PolygonHole& ph, float dist)
{
	process(ph.get_exterior(), dist);
	for (unsigned int i(0), n(ph.get_nrinterior()); i < n; ++i)
		process(ph[i], dist);
}

void ExtPoly::process(const MultiPolygonHole& p, float dist)
{
	for (MultiPolygonHole::const_iterator pi(p.begin()), pe(p.end()); pi != pe; ++pi)
		process(*pi, dist);
}

void ExtPoly::process(float dist, bool verbose)
{
	MultiPolygonHole p;
	m_poly.swap(p);
	process(p, dist);
	m_poly.normalize_boostgeom();
	p.normalize_boostgeom();
	if (verbose)
		std::cerr << "  Extending " << (m_poly.size() + p.size()) << " polygons..." << std::endl;
	m_poly.geos_union(p);
	//m_poly.insert(m_poly.end(), p.begin(), p.end());
	m_bbox = m_poly.get_bbox();
}

void ExtPoly::process(void)
{
	MultiPolygonHole p;
	m_poly.normalize_boostgeom();
	m_poly.geos_union(p);
	m_bbox = m_poly.get_bbox();
}

bool ExtPoly::is_inside(const Rect& x) const
{
	return x.is_inside(m_bbox);
}

void ExtPoly::save(WaterelementsDbQueryInterface& db) const
{
	for (MultiPolygonHole::const_iterator pi(m_poly.begin()), pe(m_poly.end()); pi != pe; ++pi) {
		WaterelementsDb::Waterelement e(WaterelementsDb::Waterelement::type_landext, *pi);
		db.save(e);
		if (true)
			std::cerr << "landext: " << e.get_id()
				  << ' ' << e.get_bbox().get_southwest().get_lat_str2()
				  << ' ' << e.get_bbox().get_southwest().get_lon_str2()
				  << ' ' << e.get_bbox().get_northeast().get_lat_str2()
				  << ' ' << e.get_bbox().get_northeast().get_lon_str2() << std::endl;
	}
}

class ExtPolygons : public std::vector<ExtPoly> {
public:
	void load(WaterelementsDbQueryInterface& db);
	void merge(void);
	void merge(const Rect& window);
	void merge(Point::coord_t window_width, Point::coord_t window_inc);
	void process(float dist, bool verbose);
	void save(WaterelementsDbQueryInterface& db);
};

void ExtPolygons::load(WaterelementsDbQueryInterface& db)
{
	std::cerr << "Loading polygons..." << std::endl;
	MultiPolygonHole mph[3];
	{
		WaterelementsDb::Waterelement e;
		db.loadfirst(e);
		while (e.is_valid()) {
			if (e.get_type() == WaterelementsDb::Waterelement::type_land)
			//if (e.get_type() == WaterelementsDb::Waterelement::type_landmass)
				push_back(ExtPoly(e));
			db.loadnext(e);
		}
	}
	std::cerr << "Loaded " << size() << " polygons" << std::endl;
	if (false) {
		merge();
		std::cerr << "Merged " << size() << " polygons" << std::endl;
	}
}

void ExtPolygons::merge(const Rect& window)
{
	MultiPolygonHole mp;
	{
		ExtPolygons ep;
		for (const_iterator i(begin()), e(end()); i != e; ++i)
			if (i->is_inside(window))
				mp.insert(mp.end(), i->get_poly().begin(), i->get_poly().end());
			else
				ep.push_back(*i);
		swap(ep);
	}
	if (true)
		std::cerr << "Merge Window " << window.get_southwest().get_lat_str2()
			  << ' ' << window.get_southwest().get_lon_str2()
			  << ' ' << window.get_northeast().get_lat_str2()
			  << ' ' << window.get_northeast().get_lon_str2()
			  << " inside " << mp.size() << " outside " << size() << std::endl;
	mp.normalize_boostgeom();
	MultiPolygonHole mpx(mp);
	Rect bbox1;
	{
		IntervalBoundingBox bb;
		bb.add(mp);
		bbox1 = bb;
	}
	mp.geos_union(MultiPolygonHole(), window.get_west());
	Rect bbox2;
	{
		IntervalBoundingBox bb;
		bb.add(mp);
		bbox2 = bb;
	}
	if (!bbox1.is_inside(bbox2)) {
		std::cerr << "ERROR: bbox1 " << bbox1 << " bbox2 " << bbox2 << std::endl;
		Rect bboxx;
		bboxx.set_empty();
		const MultiPolygonHole& mph(mp);
		for (MultiPolygonHole::const_iterator mi(mph.begin()), me(mph.end()); mi != me; ++mi) {
			Rect bboxp(mi->get_bbox());
			bboxx = bboxx.add(bboxp);
			std::cerr << "  BBOX " << bboxp << " cum " << bboxx
				  << " w " << (uint32_t)(bboxx.get_east() - bboxx.get_west()) << std::endl;
			continue;
			for (PolygonSimple::const_iterator pi(mi->get_exterior().begin()), pe(mi->get_exterior().end()); pi != pe; ++pi)
				if (!bbox1.is_inside(*pi))
					std::cerr << "  NOT inside " << *pi << std::endl;
		}
	}
	for (MultiPolygonHole::const_iterator i(mp.begin()), e(mp.end()); i != e; ++i)
		push_back(ExtPoly(*i));
	if (true)
		std::cerr << "  merged " << mp.size() << " total " << size() << std::endl;
}

void ExtPolygons::merge(Point::coord_t window_width, Point::coord_t window_inc)
{
	Rect window(Point(0, std::numeric_limits<Point::coord_t>::min()),
		    Point(0, std::numeric_limits<Point::coord_t>::max()));
	for (;;) {
		window.set_east(window.get_west() + window_width);
		merge(window);
		{
			uint32_t c(window.get_west()), cn(c + window_inc);
			window.set_west(cn);
			if (cn < c)
				break;
		}
	}
}

void ExtPolygons::merge(void)
{
	merge(0x40000000, 0x10000000);
	//merge(0x80000000, 0x20000000);
	merge(0xE0000000, 0x20000000);
}

void ExtPolygons::process(float dist, bool verbose)
{
	for (size_type n(size()), i(0); i < n; ++i) {
		ExtPoly& mp(operator[](i));
		std::cerr << "Processing " << i << '/' << n
			  << ' ' << mp.get_bbox().get_southwest().get_lat_str2()
			  << ' ' << mp.get_bbox().get_southwest().get_lon_str2()
			  << ' ' << mp.get_bbox().get_northeast().get_lat_str2()
			  << ' ' << mp.get_bbox().get_northeast().get_lon_str2() << std::endl;
		mp.process(dist, verbose);
	}
}

void ExtPolygons::save(WaterelementsDbQueryInterface& db)
{
	db.purge(WaterelementsDb::Waterelement::type_landext);
	for (const_iterator pi(begin()), pe(end()); pi != pe; ++pi)
		pi->save(db);
}

/* ---------------------------------------------------------------------- */

static void export_polygon(OGRPolygon& poly, const Rect& bbox, const PolygonSimple& ps)
{
	static const int64_t fullangle = 1ULL << 32;
	if (ps.empty())
		return;
	int64_t lonoffs(bbox.get_west());
	{
		int64_t east = lonoffs + (uint32_t)(bbox.get_east() - bbox.get_west());
		if (east > fullangle && east - fullangle > fullangle - lonoffs)
			lonoffs -= fullangle;
	}
	OGRLinearRing ring;
	for (PolygonSimple::size_type i(0), n(ps.size()); i < n; ++i) {
		Point pt(ps[i] - Point(bbox.get_west(), 0));
		ring.addPoint((lonoffs + (uint32_t)pt.get_lon()) * Point::to_deg_dbl, pt.get_lat_deg_dbl());
	}
	{
		Point pt(ps[0] - Point(bbox.get_west(), 0));
		ring.addPoint((lonoffs + (uint32_t)pt.get_lon()) * Point::to_deg_dbl, pt.get_lat_deg_dbl());
	}
	poly.addRing(&ring);
}

static void export_polygon(OGRPolygon& poly, const Rect& bbox, const PolygonHole& ph)
{
	export_polygon(poly, bbox, ph.get_exterior());
	for (unsigned int i(0), n(ph.get_nrinterior()); i < n; ++i)
		export_polygon(poly, bbox, ph[i]);
}

static void export_polygon(OGRPolygon& poly, const PolygonSimple& ps)
{
	export_polygon(poly, ps.get_bbox(), ps);
}

static void export_polygon(OGRPolygon& poly, const PolygonHole& ph)
{
	export_polygon(poly, ph.get_bbox(), ph);
}

static void export_shapefile(WaterelementsDbQueryInterface& db, WaterelementsDb::Waterelement::type_t typ, const std::string& fn)
{
	const char *pszDriverName = "ESRI Shapefile";
	OGRRegisterAll();
#if defined(HAVE_GDAL2)
	GDALDriverManager drvmgr;
	drvmgr.AutoLoadDrivers();
	GDALDriver *poDriver = drvmgr.GetDriverByName(pszDriverName);
	if (!poDriver) {
		std::cerr << pszDriverName << " driver not found" << std::endl;
		return;
	}
	GDALDataset *poDS(poDriver->Create(fn.c_str(), 0, 0, 0, GDT_Unknown, 0));
#else
	OGRSFDriver *poDriver = OGRSFDriverRegistrar::GetRegistrar()->GetDriverByName(pszDriverName);
	if (!poDriver) {
		std::cerr << pszDriverName << " driver not found" << std::endl;
		return;
	}
	OGRDataSource *poDS = poDriver->CreateDataSource(fn.c_str(), 0);
#endif
	if (!poDS) {
		std::cerr << "cannot create output file " << fn << std::endl;
		return;
	}
	OGRLayer *poLayer = poDS->CreateLayer(WaterelementsDb::Waterelement::get_typename(typ).c_str(), 0, wkbPolygon, 0);
	if (!poLayer) {
		std::cerr << "cannot create layer" << std::endl;
		return;
	}
	OGRFieldDefn oFieldID("ID", OFTInteger);
	if (poLayer->CreateField(&oFieldID) != OGRERR_NONE) {
		std::cerr << "cannot create ID field" << std::endl;
		return;
	}
	//int idxID(poLayer->FindFieldIndex("ID", TRUE));
	{
		WaterelementsDb::Waterelement e;
		db.loadfirst(e);
		while (e.is_valid()) {
			if (e.get_type() == typ) {
				OGRFeature *poFeature = OGRFeature::CreateFeature(poLayer->GetLayerDefn());
				//if (idxID != -1)
				//	poFeature->SetField(idxID, (int)e.get_id());
				poFeature->SetField("ID", (int)e.get_id());
				OGRPolygon poly;
				export_polygon(poly, e.get_polygon());
				poFeature->SetGeometry(&poly);
				if (poLayer->CreateFeature(poFeature) != OGRERR_NONE) {
					std::cerr << "cannot create feature in shapefile" << std::endl;
					return;
				}
				OGRFeature::DestroyFeature(poFeature);
			}
			db.loadnext(e);
		}
	}
	delete poDS;
}

/* ---------------------------------------------------------------------- */

int main(int argc, char *argv[])
{
	static struct option long_options[] = {
		{ "help",                 no_argument,       NULL, 'h' },
		{ "version",              no_argument,       NULL, 'v' },
		{ "quiet",                no_argument,       NULL, 'q' },
		{ "output-dir",           required_argument, NULL, 'o' },
		{ "gshhs-dir",            required_argument, NULL, 'm' },
		{ "purge",                no_argument,       NULL, 'P' },
		{ "info",                 no_argument,       NULL, 'i' },
		{ "merge",                no_argument,       NULL, 'M' },
		{ "extend",               no_argument,       NULL, 'E' },
		{ "min-lat",              required_argument, NULL, 0x400 },
		{ "max-lat",              required_argument, NULL, 0x401 },
		{ "min-lon",              required_argument, NULL, 0x402 },
		{ "max-lon",              required_argument, NULL, 0x403 },
		{ "export-landmass",      required_argument, NULL, 0x500 + WaterelementsDb::Waterelement::type_landmass },
		{ "export-lake",          required_argument, NULL, 0x500 + WaterelementsDb::Waterelement::type_lake },
		{ "export-island",        required_argument, NULL, 0x500 + WaterelementsDb::Waterelement::type_island },
		{ "export-pond",          required_argument, NULL, 0x500 + WaterelementsDb::Waterelement::type_pond },
		{ "export-ice",           required_argument, NULL, 0x500 + WaterelementsDb::Waterelement::type_ice },
		{ "export-groundingline", required_argument, NULL, 0x500 + WaterelementsDb::Waterelement::type_groundingline },
		{ "export-land",          required_argument, NULL, 0x500 + WaterelementsDb::Waterelement::type_land },
		{ "export-landext",       required_argument, NULL, 0x500 + WaterelementsDb::Waterelement::type_landext },
#ifdef HAVE_PQXX
		{ "pgsql",                required_argument, NULL, 'p' },
#endif
		{ NULL,                   0,                 NULL, 0 }
	};
	int c, err(0);
	bool quiet(false), purge(false), info(false), domerge(false), doextend(false), pgsql(false);
	Glib::ustring output_dir("."), export_file("");
	std::list<Glib::ustring> gshhs_dir;
	// check gshhs database with: ogrinfo GSHHS_shp/h/GSHHS_h_L1.shp GSHHS_h_L1
	Rect bbox(Point(Point::lon_min, Point::lat_min), Point(Point::lon_max, Point::lat_max));
	WaterelementsDb::Waterelement::type_t texp(WaterelementsDb::Waterelement::type_invalid);

	while (((c = getopt_long(argc, argv, "hvqo:k:m:p:PiME", long_options, NULL)) != -1)) {
		switch (c) {
		case 'v':
			std::cout << argv[0] << ": (C) 2007, 2015 Thomas Sailer" << std::endl;
			return 0;

		case 'o':
			if (optarg) {
				output_dir = optarg;
				pgsql = false;
			}
			break;

		case 'p':
			if (optarg) {
				output_dir = optarg;
				pgsql = true;
			}
			break;

		case 'm':
			gshhs_dir.push_back(optarg);
			break;

		case 'q':
			quiet = true;
			break;

		case 'P':
			purge = true;
			break;

		case 'i':
			info = true;
			break;

		case 'M':
			domerge = true;
			break;

		case 'E':
			doextend = true;
			break;

		case 0x400:
		{
			Point sw(bbox.get_southwest());
			Point ne(bbox.get_northeast());
			sw.set_lat_deg(strtod(optarg, NULL));
			bbox = Rect(sw, ne);
			break;
		}

		case 0x401:
		{
			Point sw(bbox.get_southwest());
			Point ne(bbox.get_northeast());
			ne.set_lat_deg(strtod(optarg, NULL));
			bbox = Rect(sw, ne);
			break;
		}

		case 0x402:
		{
			Point sw(bbox.get_southwest());
			Point ne(bbox.get_northeast());
			sw.set_lon_deg(strtod(optarg, NULL));
			bbox = Rect(sw, ne);
			break;
		}

		case 0x403:
		{
			Point sw(bbox.get_southwest());
			Point ne(bbox.get_northeast());
			ne.set_lon_deg(strtod(optarg, NULL));
			bbox = Rect(sw, ne);
			break;
		}

		case 0x500 + WaterelementsDb::Waterelement::type_landmass:
		case 0x500 + WaterelementsDb::Waterelement::type_lake:
		case 0x500 + WaterelementsDb::Waterelement::type_island:
		case 0x500 + WaterelementsDb::Waterelement::type_pond:
		case 0x500 + WaterelementsDb::Waterelement::type_ice:
		case 0x500 + WaterelementsDb::Waterelement::type_groundingline:
		case 0x500 + WaterelementsDb::Waterelement::type_land:
		case 0x500 + WaterelementsDb::Waterelement::type_landext:
			texp = (WaterelementsDb::Waterelement::type_t)(c - 0x500);
			export_file = optarg;
			break;

		case 'h':
		default:
			++err;
			break;
		}
	}
	if (err) {
		std::cerr << "usage: vfrgshhsimport [-q] [-p port] [<file>]" << std::endl
			  << "     -q		quiet" << std::endl
			  << "     -h, --help	Display this information" << std::endl
			  << "     -v, --version     Display version information" << std::endl
			  << "     -m, --gshhs-dir   GSHHS Directory (example: GSHHS_shp/h/GSHHS_h_L1.shp)" << std::endl
			  << "     -o, --output-dir  Database Output Directory" << std::endl
#ifdef HAVE_PQXX
			  << "     -p, --pgsql       Database Connection String" << std::endl
#endif
			  << "     -P, --purge       Purge Database first" << std::endl << std::endl;
		return EX_USAGE;
	}
	std::cout << "DB Boundary: " << (std::string)bbox.get_southwest().get_lat_str()
		  << ' ' << (std::string)bbox.get_southwest().get_lon_str()
		  << " -> " << (std::string)bbox.get_northeast().get_lat_str()
		  << ' ' << (std::string)bbox.get_northeast().get_lon_str() << std::endl;
	try {
		std::unique_ptr<WaterelementsDbQueryInterface> db;
#ifdef HAVE_PQXX
		if (pgsql)
			db.reset(new WaterelementsPGDb(output_dir));
		else
#endif
			db.reset(new WaterelementsDb(output_dir));
		if (purge)
			db->purgedb();
		db->sync_off();
		for (std::list<Glib::ustring>::iterator vmi(gshhs_dir.begin()), vme(gshhs_dir.end()); vmi != vme; vmi++) {
			GSHHSParser parser(*vmi, bbox);
			if (info)
				parser.info(std::cerr);
			parser.save(*db);
		}
		if (domerge) {
			MergePolygons mp;
			mp.load(*db);
			mp.process();
			mp.merge();
			mp.save(*db);
		}
		if (doextend) {
			ExtPolygons ep;
			ep.load(*db);
			ep.process(5, true);
			ep.merge();
			ep.save(*db);
		}
		db->analyze();
		db->vacuum();
		if (texp != WaterelementsDb::Waterelement::type_invalid)
			export_shapefile(*db, texp, export_file);
		db->close();
	} catch (const std::exception& e) {
		std::cerr << "Error: " << e.what() << std::endl;
		return EX_DATAERR;
	} catch (const Glib::Exception& e) {
		std::cerr << "Glib Error: " << e.what() << std::endl;
		return EX_DATAERR;
	}
	return 0;
}
