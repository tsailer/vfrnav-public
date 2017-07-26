/*
 * vmap.cc: VMap Level0 to DB conversion utility
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
#include <ogrsf_frmts.h>
#endif
#endif

#include "geom.h"
#include "dbobj.h"

/* ---------------------------------------------------------------------- */

class VMapParser {
public:
	VMapParser(const Glib::ustring& fname, const Rect& rect = Rect(Point(0, 0), Point(-1, -1)));
	~VMapParser();
	std::ostream& info(std::ostream& os);
	void save(MapelementsDbQueryInterface& db);

protected:
	OGRLayer& get_layer(const std::string& name);
	static PolygonSimple get_polyline(OGRGeometry *geom);
	static PolygonSimple get_polyline(OGRLineString *geom);
	static PolygonHole get_polygon(OGRGeometry *geom);
	static PolygonHole get_polygon(OGRPolygon *geom);
	static Point get_point(OGRGeometry *geom);
	static Point get_point(OGRPoint *geom);
	static std::string trim(const std::string& x);
	void save(MapelementsDbQueryInterface& db, MapelementsDb::Mapelement& e);
	void save_area(MapelementsDbQueryInterface& db, OGRFeature *feature, uint8_t tc, const Glib::ustring nm = "");
	void save_line(MapelementsDbQueryInterface& db, OGRFeature *feature, uint8_t tc, const Glib::ustring nm = "");
	void save_point(MapelementsDbQueryInterface& db, OGRFeature *feature, uint8_t tc, const Glib::ustring nm = "");
	void save_isohypse(MapelementsDbQueryInterface& db);
	void save_polbnda_bnd(MapelementsDbQueryInterface& db);
	void save_polbndl_bnd(MapelementsDbQueryInterface& db);
	void save_watrcrsl_hydro(MapelementsDbQueryInterface& db);
	void save_aquecanl_hydro(MapelementsDbQueryInterface& db);
	void save_inwatera_hydro(MapelementsDbQueryInterface& db);
	void save_landicea_phys(MapelementsDbQueryInterface& db);
	void save_seaicea_phys(MapelementsDbQueryInterface& db);
	void save_builtupa_pop(MapelementsDbQueryInterface& db);
	void save_builtupp_pop(MapelementsDbQueryInterface& db);
	void save_mispopp_pop(MapelementsDbQueryInterface& db);
	void save_mistranl_trans(MapelementsDbQueryInterface& db);
	void save_railrdl_trans(MapelementsDbQueryInterface& db);
	void save_roadl_trans(MapelementsDbQueryInterface& db);
	void save_traill_trans(MapelementsDbQueryInterface& db);
	void save_utill_util(MapelementsDbQueryInterface& db);
	void save_treesa_veg(MapelementsDbQueryInterface& db);


private:
#if defined(HAVE_GDAL2)
	GDALDataset *m_ds;
#else
	OGRDataSource *m_ds;
#endif
	Rect m_rect;
	unsigned int m_recs;
};

VMapParser::VMapParser(const Glib::ustring & fname, const Rect& rect)
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
		throw std::runtime_error("Cannot open VMap database " + fname);
}

VMapParser::~VMapParser()
{
#if defined(HAVE_GDAL2)
	GDALClose(m_ds);
#else
	OGRDataSource::DestroyDataSource(m_ds);
#endif
	m_ds = 0;
}

std::ostream & VMapParser::info(std::ostream & os)
{
#if !defined(HAVE_GDAL2)
	os << "Name: " << m_ds->GetName() << std::endl;
#endif
	for (int layernr = 0; layernr < m_ds->GetLayerCount(); layernr++) {
		OGRLayer *layer(m_ds->GetLayer(layernr));
		OGRFeatureDefn *layerdef(layer->GetLayerDefn());
		os << "  Layer Name: " << layerdef->GetName() << std::endl;
		for (int fieldnr = 0; fieldnr < layerdef->GetFieldCount(); fieldnr++) {
			OGRFieldDefn *fielddef(layerdef->GetFieldDefn(fieldnr));
			os << "    Field Name: " << fielddef->GetNameRef()
			   << " type " << OGRFieldDefn::GetFieldTypeName(fielddef->GetType())  << std::endl;
		}
		layer->ResetReading();
		while (OGRFeature *feature = layer->GetNextFeature()) {
			os << "  Feature: " << feature->GetFID() << std::endl;
			for (int fieldnr = 0; fieldnr < layerdef->GetFieldCount(); fieldnr++) {
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

OGRLayer& VMapParser::get_layer(const std::string & name)
{
	OGRLayer *l(m_ds->GetLayerByName(name.c_str()));
	if (!l)
		throw std::runtime_error("Cannot find layer " + name);
	return *l;
}

PolygonSimple VMapParser::get_polyline(OGRLineString * geom)
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

PolygonSimple VMapParser::get_polyline(OGRGeometry * geom)
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

PolygonHole VMapParser::get_polygon(OGRPolygon * geom)
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

PolygonHole VMapParser::get_polygon(OGRGeometry * geom)
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

Point VMapParser::get_point(OGRPoint * geom)
{
	if (!geom)
		return Point();
	Point pt;
	pt.set_lon_deg_dbl(geom->getX());
	pt.set_lat_deg_dbl(geom->getY());
	return pt;
}

Point VMapParser::get_point(OGRGeometry * geom)
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

void VMapParser::save_isohypse(MapelementsDbQueryInterface& db)
{
	static const uint16_t isohypse_alt[] = {
		0, 10, 25, 50, 75, 100, 150, 200, 250, 300,
		350, 400, 450, 500, 600, 700, 800, 900, 1000, 1250,
		1500, 1750, 2000, 2250, 2500, 2750, 3000, 3250, 3500, 3750,
		4000, 4250, 4500, 4750, 5000, 5250, 5500, 5750, 6000, 6250,
		6500, 6750, 7000, 7250, 7500, 7750, 8000, 8250, 8500, 8750
	};

	OGRLayer& layer(get_layer("contourl@elev(*)_line"));
	OGRFeatureDefn& layerdef(*layer.GetLayerDefn());
	int hqc_index(layerdef.GetFieldIndex("hqc"));
	if (hqc_index < 0)
		throw std::runtime_error("isohypse: hqc attribute not found");
	int zv2_index(layerdef.GetFieldIndex("zv2"));
	if (zv2_index < 0)
		throw std::runtime_error("isohypse: zv2 attribute not found");
	layer.ResetReading();
	while (OGRFeature *feature = layer.GetNextFeature()) {
		OGRGeometry *geom(feature->GetGeometryRef());
		PolygonSimple p(get_polyline(geom));
		int zv2(feature->GetFieldAsInteger(zv2_index));
		int hqc(feature->GetFieldAsInteger(hqc_index));
		unsigned int j;
		for (j = 0; (j < sizeof(isohypse_alt)/sizeof(isohypse_alt[0])) && (isohypse_alt[j] < zv2); j++);
		if (hqc == 6 || hqc == 18 || hqc == 22 || hqc == 23)
			j |= 0x100;
		// altcode = j;
		// polyline = p;
	}
}

std::string VMapParser::trim(const std::string & xx)
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

void VMapParser::save(MapelementsDbQueryInterface& db, MapelementsDb::Mapelement& e)
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
	m_recs++;
}

void VMapParser::save_area(MapelementsDbQueryInterface& db, OGRFeature * feature, uint8_t tc, const Glib::ustring nm)
{
	PolygonHole p(get_polygon(feature->GetGeometryRef()));
	if (p.get_exterior().empty()) {
		std::cerr << "save_area: empty object name " << nm << std::endl;
		return;
	}
	MapelementsDb::Mapelement e(tc, p, nm);
	save(db, e);
}

void VMapParser::save_line(MapelementsDbQueryInterface& db, OGRFeature * feature, uint8_t tc, const Glib::ustring nm)
{
	PolygonHole p(get_polyline(feature->GetGeometryRef()));
	if (p.get_exterior().empty()) {
		std::cerr << "save_line: empty object name " << nm << std::endl;
		return;
	}
	MapelementsDb::Mapelement e(tc, p, nm);
	save(db, e);
}

void VMapParser::save_point(MapelementsDbQueryInterface& db, OGRFeature * feature, uint8_t tc, const Glib::ustring nm)
{
	Point pt(get_point(feature->GetGeometryRef()));
	PolygonSimple p;
	p.push_back(pt);
	MapelementsDb::Mapelement e(tc, p, pt, nm);
	save(db, e);
}

void VMapParser::save_polbnda_bnd(MapelementsDbQueryInterface& db)
{
	static const char layername[] = "polbnda@bnd(*)_area";
	m_recs = 0;
	OGRLayer& layer(get_layer(layername));
	OGRFeatureDefn& layerdef(*layer.GetLayerDefn());
	int nam_index(layerdef.GetFieldIndex("nam"));
	if (nam_index < 0)
		throw std::runtime_error("polbnda_bnd: nam attribute not found");
	int na2_index(layerdef.GetFieldIndex("na2"));
	if (na2_index < 0)
		throw std::runtime_error("polbnda_bnd: na2 attribute not found");
	int na3_index(layerdef.GetFieldIndex("na3"));
	if (na3_index < 0)
		throw std::runtime_error("polbnda_bnd: na3 attribute not found");
	layer.ResetReading();
	while (OGRFeature *feature = layer.GetNextFeature()) {
		std::string nam(feature->GetFieldAsString(nam_index));
		std::string na2(feature->GetFieldAsString(na2_index));
		std::string na3(feature->GetFieldAsString(na3_index));
		save_area(db, feature, MapelementsDb::Mapelement::maptyp_border, trim(nam) + "/" + trim(na2) + "/" + trim(na3));
	}
	std::cerr << "Layer " << layername << ": " << m_recs << " records" << std::endl;
}

void VMapParser::save_polbndl_bnd(MapelementsDbQueryInterface& db)
{
	static const char layername[] = "polbndl@bnd(*)_line";
	m_recs = 0;
	OGRLayer& layer(get_layer(layername));
	OGRFeatureDefn& layerdef(*layer.GetLayerDefn());
	layer.ResetReading();
	while (OGRFeature *feature = layer.GetNextFeature()) {
		 save_line(db, feature, MapelementsDb::Mapelement::maptyp_border);
	}
	std::cerr << "Layer " << layername << ": " << m_recs << " records" << std::endl;
}

void VMapParser::save_watrcrsl_hydro(MapelementsDbQueryInterface& db)
{
	static const char layername[] = "watrcrsl@hydro(*)_line";
	m_recs = 0;
	OGRLayer& layer(get_layer(layername));
	OGRFeatureDefn& layerdef(*layer.GetLayerDefn());
	int nam_index(layerdef.GetFieldIndex("nam"));
	if (nam_index < 0)
		throw std::runtime_error("watrcrsl_hydro: nam attribute not found");
	int hyc_index(layerdef.GetFieldIndex("hyc"));
	if (hyc_index < 0)
		throw std::runtime_error("watrcrsl_hydro: hyc attribute not found");
	layer.ResetReading();
	while (OGRFeature *feature = layer.GetNextFeature()) {
		std::string nam(feature->GetFieldAsString(nam_index));
		int hyc(feature->GetFieldAsInteger(hyc_index));
		save_line(db, feature, (hyc == 6) ? MapelementsDb::Mapelement::maptyp_river_t : MapelementsDb::Mapelement::maptyp_river, trim(nam));
	}
	std::cerr << "Layer " << layername << ": " << m_recs << " records" << std::endl;
}

void VMapParser::save_aquecanl_hydro(MapelementsDbQueryInterface& db)
{
	static const char layername[] = "aquecanl@hydro(*)_line";
	m_recs = 0;
	OGRLayer& layer(get_layer(layername));
	OGRFeatureDefn& layerdef(*layer.GetLayerDefn());
	int nam_index(layerdef.GetFieldIndex("nam"));
	if (nam_index < 0)
		throw std::runtime_error("aquecanl_hydro: nam attribute not found");
	layer.ResetReading();
	while (OGRFeature *feature = layer.GetNextFeature()) {
		std::string nam(feature->GetFieldAsString(nam_index));
		save_line(db, feature, MapelementsDb::Mapelement::maptyp_canal, trim(nam));
	}
	std::cerr << "Layer " << layername << ": " << m_recs << " records" << std::endl;
}

void VMapParser::save_inwatera_hydro(MapelementsDbQueryInterface& db)
{
	static const char layername[] = "inwatera@hydro(*)_area";
	m_recs = 0;
	OGRLayer& layer(get_layer(layername));
	OGRFeatureDefn& layerdef(*layer.GetLayerDefn());
	int nam_index(layerdef.GetFieldIndex("nam"));
	if (nam_index < 0)
		throw std::runtime_error("inwatera_hydro: nam attribute not found");
	int hyc_index(layerdef.GetFieldIndex("hyc"));
	if (hyc_index < 0)
		throw std::runtime_error("inwatera_hydro: hyc attribute not found");
	layer.ResetReading();
	while (OGRFeature *feature = layer.GetNextFeature()) {
		std::string nam(feature->GetFieldAsString(nam_index));
		int hyc(feature->GetFieldAsInteger(hyc_index));
		save_area(db, feature, (hyc == 6) ? MapelementsDb::Mapelement::maptyp_lake_t : MapelementsDb::Mapelement::maptyp_lake, trim(nam));
	}
	std::cerr << "Layer " << layername << ": " << m_recs << " records" << std::endl;
}

void VMapParser::save_landicea_phys(MapelementsDbQueryInterface& db)
{
	static const char layername[] = "landicea@phys(*)_area";
	m_recs = 0;
	OGRLayer& layer(get_layer(layername));
	OGRFeatureDefn& layerdef(*layer.GetLayerDefn());
	layer.ResetReading();
	while (OGRFeature *feature = layer.GetNextFeature()) {
		save_area(db, feature, MapelementsDb::Mapelement::maptyp_glacier);
	}
	std::cerr << "Layer " << layername << ": " << m_recs << " records" << std::endl;
}

void VMapParser::save_seaicea_phys(MapelementsDbQueryInterface& db)
{
	static const char layername[] = "seaicea@phys(*)_area";
	m_recs = 0;
	OGRLayer& layer(get_layer(layername));
	OGRFeatureDefn& layerdef(*layer.GetLayerDefn());
	int nam_index(layerdef.GetFieldIndex("nam"));
	if (nam_index < 0)
		throw std::runtime_error("seaicea_phys: nam attribute not found");
	layer.ResetReading();
	while (OGRFeature *feature = layer.GetNextFeature()) {
		std::string nam(feature->GetFieldAsString(nam_index));
		save_area(db, feature, MapelementsDb::Mapelement::maptyp_pack_ice, trim(nam));
	}
	std::cerr << "Layer " << layername << ": " << m_recs << " records" << std::endl;
}

void VMapParser::save_builtupa_pop(MapelementsDbQueryInterface& db)
{
	static const char layername[] = "builtupa@pop(*)_area";
	m_recs = 0;
	OGRLayer& layer(get_layer(layername));
	OGRFeatureDefn& layerdef(*layer.GetLayerDefn());
	int nam_index(layerdef.GetFieldIndex("nam"));
	if (nam_index < 0)
		throw std::runtime_error("builtupa_pop: nam attribute not found");
	layer.ResetReading();
	while (OGRFeature *feature = layer.GetNextFeature()) {
		std::string nam(feature->GetFieldAsString(nam_index));
		save_area(db, feature, MapelementsDb::Mapelement::maptyp_city, trim(nam));
	}
	std::cerr << "Layer " << layername << ": " << m_recs << " records" << std::endl;
}

void VMapParser::save_builtupp_pop(MapelementsDbQueryInterface& db)
{
	static const char layername[] = "builtupp@pop(*)_point";
	m_recs = 0;
	OGRLayer& layer(get_layer(layername));
	OGRFeatureDefn& layerdef(*layer.GetLayerDefn());
	int nam_index(layerdef.GetFieldIndex("nam"));
	if (nam_index < 0)
		throw std::runtime_error("builtupp_pop: nam attribute not found");
	layer.ResetReading();
	while (OGRFeature *feature = layer.GetNextFeature()) {
		std::string nam(feature->GetFieldAsString(nam_index));
		save_point(db, feature, MapelementsDb::Mapelement::maptyp_village, trim(nam));
	}
	std::cerr << "Layer " << layername << ": " << m_recs << " records" << std::endl;
}

void VMapParser::save_mispopp_pop(MapelementsDbQueryInterface& db)
{
	static const char layername[] = "mispopp@pop(*)_point";
	m_recs = 0;
	OGRLayer& layer(get_layer(layername));
	OGRFeatureDefn& layerdef(*layer.GetLayerDefn());
	int nam_index(layerdef.GetFieldIndex("nam"));
	if (nam_index < 0)
		throw std::runtime_error("mispopp_pop: nam attribute not found");
	int f_code_index(layerdef.GetFieldIndex("f_code"));
	if (f_code_index < 0)
		throw std::runtime_error("mispopp_pop: f_code attribute not found");
	layer.ResetReading();
	while (OGRFeature *feature = layer.GetNextFeature()) {
		std::string nam(feature->GetFieldAsString(nam_index));
		std::string f_code(feature->GetFieldAsString(f_code_index));
		if (*f_code.c_str() != 'A')
			continue;
		save_point(db, feature, MapelementsDb::Mapelement::maptyp_village, trim(nam));
	}
	std::cerr << "Layer " << layername << ": " << m_recs << " records" << std::endl;
}

void VMapParser::save_mistranl_trans(MapelementsDbQueryInterface& db)
{
	static const char layername[] = "mistranl@trans(*)_line";
	m_recs = 0;
	OGRLayer& layer(get_layer(layername));
	OGRFeatureDefn& layerdef(*layer.GetLayerDefn());
	int f_code_index(layerdef.GetFieldIndex("f_code"));
	if (f_code_index < 0)
		throw std::runtime_error("mistranl_trans: f_code attribute not found");
	layer.ResetReading();
	while (OGRFeature *feature = layer.GetNextFeature()) {
		std::string f_code(feature->GetFieldAsString(f_code_index));
		if (f_code != "AQ010")
			continue;
		save_line(db, feature, MapelementsDb::Mapelement::maptyp_aerial_cable);
	}
	std::cerr << "Layer " << layername << ": " << m_recs << " records" << std::endl;
}

void VMapParser::save_railrdl_trans(MapelementsDbQueryInterface& db)
{
	static const char layername[] = "railrdl@trans(*)_line";
	m_recs = 0;
	OGRLayer& layer(get_layer(layername));
	OGRFeatureDefn& layerdef(*layer.GetLayerDefn());
	int fco_index(layerdef.GetFieldIndex("fco"));
	if (fco_index < 0)
		throw std::runtime_error("railrdl_trans: fco attribute not found");
	layer.ResetReading();
	while (OGRFeature *feature = layer.GetNextFeature()) {
		int fco(feature->GetFieldAsInteger(fco_index));
		save_line(db, feature, (fco == 2) ? MapelementsDb::Mapelement::maptyp_railway_d : MapelementsDb::Mapelement::maptyp_railway);
	}
	std::cerr << "Layer " << layername << ": " << m_recs << " records" << std::endl;
}

void VMapParser::save_roadl_trans(MapelementsDbQueryInterface& db)
{
	static const char layername[] = "roadl@trans(*)_line";
	m_recs = 0;
	OGRLayer& layer(get_layer(layername));
	OGRFeatureDefn& layerdef(*layer.GetLayerDefn());
	int med_index(layerdef.GetFieldIndex("med"));
	if (med_index < 0)
		throw std::runtime_error("roadl_trans: med attribute not found");
	layer.ResetReading();
	while (OGRFeature *feature = layer.GetNextFeature()) {
		int med(feature->GetFieldAsInteger(med_index));
		save_line(db, feature, (med == 1) ? MapelementsDb::Mapelement::maptyp_highway : MapelementsDb::Mapelement::maptyp_road);
	}
	std::cerr << "Layer " << layername << ": " << m_recs << " records" << std::endl;
}

void VMapParser::save_traill_trans(MapelementsDbQueryInterface& db)
{
	static const char layername[] = "traill@trans(*)_line";
	m_recs = 0;
	OGRLayer& layer(get_layer(layername));
	OGRFeatureDefn& layerdef(*layer.GetLayerDefn());
	layer.ResetReading();
	while (OGRFeature *feature = layer.GetNextFeature()) {
		save_line(db, feature, MapelementsDb::Mapelement::maptyp_trail);
	}
	std::cerr << "Layer " << layername << ": " << m_recs << " records" << std::endl;
}

void VMapParser::save_utill_util(MapelementsDbQueryInterface& db)
{
	static const char layername[] = "utill@util(*)_line";
	m_recs = 0;
	OGRLayer& layer(get_layer(layername));
	OGRFeatureDefn& layerdef(*layer.GetLayerDefn());
	int f_code_index(layerdef.GetFieldIndex("f_code"));
	if (f_code_index < 0)
		throw std::runtime_error("utill_util: f_code attribute not found");
	layer.ResetReading();
	while (OGRFeature *feature = layer.GetNextFeature()) {
		std::string f_code(feature->GetFieldAsString(f_code_index));
		if (f_code != "AT030")
			continue;
		save_line(db, feature, MapelementsDb::Mapelement::maptyp_powerline);
	}
	std::cerr << "Layer " << layername << ": " << m_recs << " records" << std::endl;
}

void VMapParser::save_treesa_veg(MapelementsDbQueryInterface& db)
{
	static const char layername[] = "treesa@veg(*)_area";
	m_recs = 0;
	OGRLayer& layer(get_layer(layername));
	OGRFeatureDefn& layerdef(*layer.GetLayerDefn());
	int nam_index(layerdef.GetFieldIndex("nam"));
	if (nam_index < 0)
		throw std::runtime_error("treesa_veg: nam attribute not found");
	layer.ResetReading();
	while (OGRFeature *feature = layer.GetNextFeature()) {
		std::string nam(feature->GetFieldAsString(nam_index));
		save_area(db, feature, MapelementsDb::Mapelement::maptyp_forest, trim(nam));
	}
	std::cerr << "Layer " << layername << ": " << m_recs << " records" << std::endl;
}

/* cropa@veg, grassa@veg, swampa@veg, tundraa@veg */

void VMapParser::save(MapelementsDbQueryInterface& db)
{
	try {
		save_isohypse(db);
	} catch (const std::runtime_error& e) {
		std::cerr << "Error: " << e.what() << std::endl;
	}
	try {
		save_polbnda_bnd(db);
	} catch (const std::runtime_error& e) {
		std::cerr << "Error: " << e.what() << std::endl;
	}
	try {
		save_polbndl_bnd(db);
	} catch (const std::runtime_error& e) {
		std::cerr << "Error: " << e.what() << std::endl;
	}
	try {
		save_watrcrsl_hydro(db);
	} catch (const std::runtime_error& e) {
		std::cerr << "Error: " << e.what() << std::endl;
	}
	try {
		save_aquecanl_hydro(db);
	} catch (const std::runtime_error& e) {
		std::cerr << "Error: " << e.what() << std::endl;
	}
	try {
		save_inwatera_hydro(db);
	} catch (const std::runtime_error& e) {
		std::cerr << "Error: " << e.what() << std::endl;
	}
	try {
		save_landicea_phys(db);
	} catch (const std::runtime_error& e) {
		std::cerr << "Error: " << e.what() << std::endl;
	}
	try {
		save_seaicea_phys(db);
	} catch (const std::runtime_error& e) {
		std::cerr << "Error: " << e.what() << std::endl;
	}
	try {
		save_builtupa_pop(db);
	} catch (const std::runtime_error& e) {
		std::cerr << "Error: " << e.what() << std::endl;
	}
	try {
		save_builtupp_pop(db);
	} catch (const std::runtime_error& e) {
		std::cerr << "Error: " << e.what() << std::endl;
	}
	try {
		save_mispopp_pop(db);
	} catch (const std::runtime_error& e) {
		std::cerr << "Error: " << e.what() << std::endl;
	}
	try {
		save_mistranl_trans(db);
	} catch (const std::runtime_error& e) {
		std::cerr << "Error: " << e.what() << std::endl;
	}
	try {
		save_railrdl_trans(db);
	} catch (const std::runtime_error& e) {
		std::cerr << "Error: " << e.what() << std::endl;
	}
	try {
		save_roadl_trans(db);
	} catch (const std::runtime_error& e) {
		std::cerr << "Error: " << e.what() << std::endl;
	}
	try {
		save_traill_trans(db);
	} catch (const std::runtime_error& e) {
		std::cerr << "Error: " << e.what() << std::endl;
	}
	try {
		save_utill_util(db);
	} catch (const std::runtime_error& e) {
		std::cerr << "Error: " << e.what() << std::endl;
	}
	try {
		save_treesa_veg(db);
	} catch (const std::runtime_error& e) {
		std::cerr << "Error: " << e.what() << std::endl;
	}
}

/* ---------------------------------------------------------------------- */

int main(int argc, char *argv[])
{
	static struct option long_options[] = {
		{ "help",        no_argument,       NULL, 'h' },
		{ "version",     no_argument,       NULL, 'v' },
		{ "quiet",       no_argument,       NULL, 'q' },
		{ "output-dir",	 required_argument, NULL, 'o' },
		{ "vmap-dir",    required_argument, NULL, 'm' },
		{ "purge",       no_argument,       NULL, 'P' },
		{ "info",        no_argument,       NULL, 'i' },
		{ "min-lat",     required_argument, NULL, 0x400 },
		{ "max-lat",     required_argument, NULL, 0x401 },
		{ "min-lon",     required_argument, NULL, 0x402 },
		{ "max-lon",     required_argument, NULL, 0x403 },
#ifdef HAVE_PQXX
		{ "pgsql",       required_argument, NULL, 'p' },
#endif
		{ NULL,	         0,                 NULL, 0 }
	};
	int c, err(0);
	bool quiet(false), purge(false), info(false), pgsql(false);
	Glib::ustring output_dir(".");
	std::list<Glib::ustring> vmap_dir;
	// check vmap database with: ogrinfo gltp:/vrf/tmp/v0eur/vmaplv0/eurnasia 'tundraa@veg(*)_area'
	Rect bbox(Point(Point::lon_min, Point::lat_min), Point(Point::lon_max, Point::lat_max));

	while (((c = getopt_long(argc, argv, "hvqo:k:m:p:Pi", long_options, NULL)) != -1)) {
		switch (c) {
		case 'v':
			std::cout << argv[0] << ": (C) 2007 Thomas Sailer" << std::endl;
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
			vmap_dir.push_back(optarg);
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

		case 'h':
		default:
			err++;
			break;
		}
	}
	if (err) {
		std::cerr << "usage: mapelementsdb [-q] [-p port] [<file>]" << std::endl
			  << "     -q                quiet" << std::endl
			  << "     -h, --help        Display this information" << std::endl
			  << "     -v, --version     Display version information" << std::endl
			  << "     -m, --vmap-dir    VMap Level0 Directory (example: gltp:/vrf/tmp/v0eur/vmaplv0/eurnasia)" << std::endl
			  << "     -o, --output-dir  Database Output Directory" << std::endl
#ifdef HAVE_PQXX
			  << "     -p, --pgsql       Database Connection String" << std::endl
#endif
			  << "     -P, --purge       Purge Database first" << std::endl << std::endl;
		return EX_USAGE;
	}
	if (vmap_dir.empty())
		vmap_dir.push_back("gltp:/vrf/tmp/vn/vmap/v0eur/vmaplv0/eurnasia");
	std::cout << "DB Boundary: " << (std::string)bbox.get_southwest().get_lat_str()
		  << ' ' << (std::string)bbox.get_southwest().get_lon_str()
		  << " -> " << (std::string)bbox.get_northeast().get_lat_str()
		  << ' ' << (std::string)bbox.get_northeast().get_lon_str() << std::endl;
	try {
		std::unique_ptr<MapelementsDbQueryInterface> db;
#ifdef HAVE_PQXX
		if (pgsql)
			db.reset(new MapelementsPGDb(output_dir));
		else
#endif
			db.reset(new MapelementsDb(output_dir));
		if (purge)
			db->purgedb();
		db->sync_off();
		for (std::list<Glib::ustring>::iterator vmi(vmap_dir.begin()), vme(vmap_dir.end()); vmi != vme; vmi++) {
			VMapParser vmap(*vmi, bbox);
			if (info)
				vmap.info(std::cerr);
			vmap.save(*db);
		}
		db->analyze();
		db->vacuum();
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
