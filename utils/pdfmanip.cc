/*
 * pdfmanip.cc:  PDF file manipulation utility
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
 * Example File:
 * http://www.avenza.com/sites/default/files/spatialpdf/US_County_Populations.pdf
 *
 * http://epsg-registry.org/
 * EPSG::3034
 */

#include "sysdeps.h"

#include <json/json.h>
#include <podofo/podofo.h>
#include <ogr_spatialref.h>
#include <cpl_conv.h>
#include <getopt.h>
#include <iostream>
#include <sstream>
#include <fstream>
#include <limits>
#include <sqlite3x.hpp>
#include <sqlite3.h>
#include <unistd.h>
#include <stdexcept>
#include <glibmm.h>
#include <giomm.h>

#ifdef HAVE_EIGEN3
#include <Eigen/Dense>
#endif

#include "geom.h"
#include "dbobj.h"


/* ---------------------------------------------------------------------- */

static const PoDoFo::PdfObject *GetPagesKids(const PoDoFo::PdfMemDocument& doc)
{
	const PoDoFo::PdfObject *cat(doc.GetCatalog());
	if (!cat || !cat->IsDictionary())
		throw std::runtime_error("No catalog dictionary");
	const PoDoFo::PdfObject *pagesdict(cat->GetIndirectKey("Pages"));
	if (!pagesdict || !pagesdict->IsDictionary())
		throw std::runtime_error("No pages dictionary");
	const PoDoFo::PdfObject *pageskids(pagesdict->GetIndirectKey("Kids"));
	if (!pageskids || !pageskids->IsArray())
		throw std::runtime_error("No pages array");
	return pageskids;
}

static PoDoFo::PdfObject *GetPagesKids(PoDoFo::PdfMemDocument& doc)
{
	PoDoFo::PdfObject *cat(doc.GetCatalog());
	if (!cat || !cat->IsDictionary())
		throw std::runtime_error("No catalog dictionary");
	PoDoFo::PdfObject *pagesdict(cat->GetIndirectKey("Pages"));
	if (!pagesdict || !pagesdict->IsDictionary())
		throw std::runtime_error("No pages dictionary");
	PoDoFo::PdfObject *pageskids(pagesdict->GetIndirectKey("Kids"));
	if (!pageskids || !pageskids->IsArray())
		throw std::runtime_error("No pages array");
	return pageskids;
}

static const PoDoFo::PdfObject *GetPage(const PoDoFo::PdfMemDocument& doc, unsigned int pgnr, const PoDoFo::PdfObject *pageskids = 0)
{
	if (!pageskids || !pageskids->IsArray()) {
		pageskids = GetPagesKids(doc);
		if (!pageskids)
			throw std::runtime_error("No pages array");
	}
	if (pgnr >= pageskids->GetArray().size())
		return 0;
	const PoDoFo::PdfObject& pageref(pageskids->GetArray()[pgnr]);
	if (!pageref.IsReference() && !pageref.IsDictionary())
		return 0;
	const PoDoFo::PdfObject *page(&pageref);
	if (pageref.IsReference())
		page = doc.GetObjects().GetObject(pageref.GetReference());
	if (!page || !page->IsDictionary())
		return 0;
	return page;
}

static PoDoFo::PdfObject *GetPage(PoDoFo::PdfMemDocument& doc, unsigned int pgnr, PoDoFo::PdfObject *pageskids = 0)
{
	if (!pageskids || !pageskids->IsArray()) {
		pageskids = GetPagesKids(doc);
		if (!pageskids)
			throw std::runtime_error("No pages array");
	}
	if (pgnr >= pageskids->GetArray().size())
		return 0;
	PoDoFo::PdfObject& pageref(pageskids->GetArray()[pgnr]);
	if (!pageref.IsReference() && !pageref.IsDictionary())
		return 0;
	PoDoFo::PdfObject *page(&pageref);
	if (pageref.IsReference())
		page = doc.GetObjects().GetObject(pageref.GetReference());
	if (!page || !page->IsDictionary())
		return 0;
	return page;
}

static std::vector<double> GetDoubleArray(const PoDoFo::PdfObject *arr)
{
	std::vector<double> r;
	if (!arr || !arr->IsArray())
		return r;
	for (unsigned int i = 0, n = arr->GetArray().size(); i < n; ++i) {
		r.push_back(std::numeric_limits<double>::quiet_NaN());
		const PoDoFo::PdfObject& no(arr->GetArray()[i]);
		if (!no.IsReal())
			continue;
		r.back() = no.GetReal();
	}
	return r;
}

static std::vector<double> GetDoubleArray(const PoDoFo::PdfObject *dict, const char *name)
{
	std::vector<double> r;
	if (!dict || !name)
		return r;
	return GetDoubleArray(dict->GetIndirectKey(name));
}

/* ---------------------------------------------------------------------- */

static void dump_pdf(const std::string& filename)
{
	PoDoFo::PdfMemDocument doc;
	doc.Load(filename.c_str());
 	int nrpgs(doc.GetPageCount());
	std::cout << filename << ": " << nrpgs << " pages" << std::endl;
	for (int pgnr = 0; pgnr < nrpgs; ++pgnr) {
		const PoDoFo::PdfPage *page(doc.GetPage(pgnr));
		if (!page)
			continue;
		std::cout << "Page " << pgnr << std::endl
			  << "  size " << page->GetPageSize().ToString() << std::endl
			  << "  media " << page->GetMediaBox().ToString() << std::endl
			  << "  crop " << page->GetCropBox().ToString() << std::endl
			  << "  trim " << page->GetTrimBox().ToString() << std::endl;
		const PoDoFo::PdfObject *vp(page->GetInheritedKey("VP"));
		if (!vp || !vp->IsArray())
			continue;
		for (unsigned int vpnr = 0, vpnrs = vp->GetArray().size(); vpnr < vpnrs; ++vpnr) {
			const PoDoFo::PdfObject& vpdict(vp->GetArray()[vpnr]);
			if (!vpdict.IsDictionary())
				continue;
#if 0
			const PoDoFo::PdfObject *measdict(vpdict.GetIndirectKey("Measure"));
			if (!measdict || !measdict->IsDictionary())
				continue;
#else
			const PoDoFo::PdfObject *measref(vpdict.GetDictionary().GetKey("Measure"));
			if (!measref || (!measref->IsReference() && !measref->IsDictionary()))
				continue;
			const PoDoFo::PdfObject *measdict(measref);
			if (measdict->IsReference())
				measdict = doc.GetObjects().GetObject(measref->GetReference());
#endif
			const PoDoFo::PdfObject *subtyp(measdict->GetIndirectKey("Subtype"));
			if (!subtyp || !subtyp->IsName() || subtyp->GetName() != "GEO")
				continue;
			const PoDoFo::PdfObject *bounds(measdict->GetIndirectKey("Bounds"));
			if (!bounds || !bounds->IsArray())
				continue;
			const PoDoFo::PdfObject *gpts(measdict->GetIndirectKey("GPTS"));
			if (!gpts || !gpts->IsArray())
				continue;
			const PoDoFo::PdfObject *lpts(measdict->GetIndirectKey("LPTS"));
			if (!lpts || !lpts->IsArray())
				continue;
			const PoDoFo::PdfObject *gcsdict(measdict->GetIndirectKey("GCS"));
			if (!gcsdict || !gcsdict->IsDictionary())
				continue;
			const PoDoFo::PdfObject *wkt(gcsdict->GetIndirectKey("WKT"));
			if (!wkt || !wkt->IsString())
				continue;
		        OGRSpatialReference sref;
			{
				char *cp(const_cast<char *>(wkt->GetString().GetStringUtf8().c_str()));
				if (sref.importFromWkt(&cp) != OGRERR_NONE) {
					std::cerr << "Page " << pgnr << '(' << vpnr << "): invalid coordinate system \""
						  << wkt->GetString().GetStringUtf8() << "\"" << std::endl;
				}
			}
			{
				char *cp(0);
				sref.exportToProj4(&cp);
				std::cerr << "Page " << pgnr << '(' << vpnr << "): proj " << cp << std::endl;
				CPLFree(cp);
			}
		}
	}

}

/* ---------------------------------------------------------------------- */

typedef std::pair<double,double> xy_t;

static xy_t GetXY(const Json::Value& desc)
{
	xy_t r;
	r.first = r.second = std::numeric_limits<double>::quiet_NaN();
	if (!desc.isArray())
		return r;
	if (desc.size() >= 1 && desc[0].isNumeric())
		r.first = desc[0].asDouble();
	if (desc.size() >= 2 && desc[1].isNumeric())
		r.second = desc[1].asDouble();
	return r;
}

static std::vector<xy_t> GetXYArray(const Json::Value& desc)
{
	std::vector<xy_t> r;
	if (!desc.isArray())
		return r;
	for (Json::ArrayIndex i = 0; i < desc.size(); ++i)
		r.push_back(GetXY(desc[i]));
	return r;
}

class GeoRefPt {
public:
	GeoRefPt(const Json::Value& desc);
	const Point& get_coord(void) const { return m_coord; }
	const xy_t& get_xy(void) const { return m_xy; }
	const xy_t& get_coordxy(void) const { return m_coordxy; }
	const std::string& get_name(void) const { return m_name; }
	void set_coord(const Point& p) { m_coord = p; }
	void set_xy(const xy_t& xy) { m_xy = xy; }
	void set_coordxy(const xy_t& xy) { m_coordxy = xy; }
	void set_name(const std::string& s) { m_name = s; }

protected:
	Point m_coord;
	xy_t m_xy;
	xy_t m_coordxy;
	std::string m_name;
};

GeoRefPt::GeoRefPt(const Json::Value& desc)
{
	m_coord.set_invalid();
	m_xy.first = m_xy.second = m_coordxy.first = m_coordxy.second = std::numeric_limits<double>::quiet_NaN();
	if (!desc.isObject())
		return;
	if (desc.isMember("xy"))
		m_xy = GetXY(desc["xy"]);
	if (desc.isMember("name") && desc["name"].isString())
		m_name = desc["name"].asString();
	if (desc.isMember("lat") && desc.isMember("lon") && desc["lat"].isNumeric() && desc["lon"].isNumeric()) {
		m_coord.set_lat_deg_dbl(desc["lat"].asDouble());
		m_coord.set_lon_deg_dbl(desc["lon"].asDouble());
	} else if (desc.isMember("coord")) {
		const Json::Value& coord(desc["coord"]);
		if (coord.isArray()) {
			if (coord.size() >= 2 && coord[0].isNumeric() && coord[1].isNumeric()) {
				m_coord.set_lat_deg_dbl(coord[0].asDouble());
				m_coord.set_lon_deg_dbl(coord[1].asDouble());
			}
		} else if (coord.isString()) {
			if ((Point::setstr_lat | Point::setstr_lon) & ~m_coord.set_str(coord.asString()))
				m_coord.set_invalid();
		}
	}
}

namespace {
	static PoDoFo::PdfVariant to_lgireal(double x)
	{
		if (true) {
			std::ostringstream oss;
			oss << std::fixed << x;
			return PoDoFo::PdfString(oss.str());
		}
		return x;
	}
};

static bool lgi_convert(PoDoFo::PdfDictionary& proj, const OGRSpatialReference& sref)
{
	proj.AddKey("Type", PoDoFo::PdfName("Projection"));
	if (sref.IsGeographic()) {
		proj.AddKey("ProjectionType", PoDoFo::PdfString("GEOGRAPHIC"));
	} else if (sref.IsProjected()) {
		const OGR_SRSNode *attr(sref.GetAttrNode("PROJCS|PROJECTION"));
		if (!attr) {
			std::cerr << "cannot find projection node" << std::endl;
			return false;
		}
		const char *val(attr->GetChild(0)->GetValue());
		if (!val) {
			std::cerr << "projection node has no value" << std::endl;
			return false;
		}
		if (!strcmp(val, SRS_PT_ALBERS_CONIC_EQUAL_AREA)) {
			proj.AddKey("ProjectionType", PoDoFo::PdfString("AC"));
			proj.AddKey("OriginLatitude", to_lgireal(sref.GetProjParm(SRS_PP_LATITUDE_OF_CENTER, std::numeric_limits<double>::quiet_NaN())));
			proj.AddKey("CentralMeridian", to_lgireal(sref.GetProjParm(SRS_PP_LONGITUDE_OF_CENTER, std::numeric_limits<double>::quiet_NaN())));
			proj.AddKey("StandardParallelOne", to_lgireal(sref.GetProjParm(SRS_PP_STANDARD_PARALLEL_1, std::numeric_limits<double>::quiet_NaN())));
			proj.AddKey("StandardParallelTwo", to_lgireal(sref.GetProjParm(SRS_PP_STANDARD_PARALLEL_2, std::numeric_limits<double>::quiet_NaN())));
			proj.AddKey("FalseEasting", to_lgireal(sref.GetProjParm(SRS_PP_FALSE_EASTING, std::numeric_limits<double>::quiet_NaN())));
			proj.AddKey("FalseNorthing", to_lgireal(sref.GetProjParm(SRS_PP_FALSE_NORTHING, std::numeric_limits<double>::quiet_NaN())));
		} else if (!strcmp(val, SRS_PT_AZIMUTHAL_EQUIDISTANT)) {
			proj.AddKey("ProjectionType", PoDoFo::PdfString("AL"));
			proj.AddKey("OriginLatitude", to_lgireal(sref.GetProjParm(SRS_PP_LATITUDE_OF_CENTER, std::numeric_limits<double>::quiet_NaN())));
			proj.AddKey("CentralMeridian", to_lgireal(sref.GetProjParm(SRS_PP_LONGITUDE_OF_CENTER, std::numeric_limits<double>::quiet_NaN())));
			proj.AddKey("FalseEasting", to_lgireal(sref.GetProjParm(SRS_PP_FALSE_EASTING, std::numeric_limits<double>::quiet_NaN())));
			proj.AddKey("FalseNorthing", to_lgireal(sref.GetProjParm(SRS_PP_FALSE_NORTHING, std::numeric_limits<double>::quiet_NaN())));
		} else if (!strcmp(val, SRS_PT_BONNE)) {
			proj.AddKey("ProjectionType", PoDoFo::PdfString("BF"));
			proj.AddKey("OriginLatitude", to_lgireal(sref.GetProjParm(SRS_PP_STANDARD_PARALLEL_1, std::numeric_limits<double>::quiet_NaN())));
			proj.AddKey("CentralMeridian", to_lgireal(sref.GetProjParm(SRS_PP_LONGITUDE_OF_CENTER, std::numeric_limits<double>::quiet_NaN())));
			proj.AddKey("FalseEasting", to_lgireal(sref.GetProjParm(SRS_PP_FALSE_EASTING, std::numeric_limits<double>::quiet_NaN())));
			proj.AddKey("FalseNorthing", to_lgireal(sref.GetProjParm(SRS_PP_FALSE_NORTHING, std::numeric_limits<double>::quiet_NaN())));
		} else if (!strcmp(val, SRS_PT_CASSINI_SOLDNER)) {
			proj.AddKey("ProjectionType", PoDoFo::PdfString("CS"));
			proj.AddKey("OriginLatitude", to_lgireal(sref.GetProjParm(SRS_PP_LATITUDE_OF_CENTER, std::numeric_limits<double>::quiet_NaN())));
			proj.AddKey("CentralMeridian", to_lgireal(sref.GetProjParm(SRS_PP_LONGITUDE_OF_CENTER, std::numeric_limits<double>::quiet_NaN())));
			proj.AddKey("FalseEasting", to_lgireal(sref.GetProjParm(SRS_PP_FALSE_EASTING, std::numeric_limits<double>::quiet_NaN())));
			proj.AddKey("FalseNorthing", to_lgireal(sref.GetProjParm(SRS_PP_FALSE_NORTHING, std::numeric_limits<double>::quiet_NaN())));
		} else if (!strcmp(val, SRS_PT_CYLINDRICAL_EQUAL_AREA)) {
			proj.AddKey("ProjectionType", PoDoFo::PdfString("LI"));
			proj.AddKey("OriginLatitude", to_lgireal(sref.GetProjParm(SRS_PP_STANDARD_PARALLEL_1, std::numeric_limits<double>::quiet_NaN())));
			proj.AddKey("CentralMeridian", to_lgireal(sref.GetProjParm(SRS_PP_LONGITUDE_OF_CENTER, std::numeric_limits<double>::quiet_NaN())));
			proj.AddKey("FalseEasting", to_lgireal(sref.GetProjParm(SRS_PP_FALSE_EASTING, std::numeric_limits<double>::quiet_NaN())));
			proj.AddKey("FalseNorthing", to_lgireal(sref.GetProjParm(SRS_PP_FALSE_NORTHING, std::numeric_limits<double>::quiet_NaN())));
		} else if (!strcmp(val, SRS_PT_ECKERT_IV)) {
			proj.AddKey("ProjectionType", PoDoFo::PdfString("EF"));
			proj.AddKey("CentralMeridian", to_lgireal(sref.GetProjParm(SRS_PP_CENTRAL_MERIDIAN, std::numeric_limits<double>::quiet_NaN())));
			proj.AddKey("FalseEasting", to_lgireal(sref.GetProjParm(SRS_PP_FALSE_EASTING, std::numeric_limits<double>::quiet_NaN())));
			proj.AddKey("FalseNorthing", to_lgireal(sref.GetProjParm(SRS_PP_FALSE_NORTHING, std::numeric_limits<double>::quiet_NaN())));
		} else if (!strcmp(val, SRS_PT_ECKERT_VI)) {
			proj.AddKey("ProjectionType", PoDoFo::PdfString("ED"));
			proj.AddKey("CentralMeridian", to_lgireal(sref.GetProjParm(SRS_PP_CENTRAL_MERIDIAN, std::numeric_limits<double>::quiet_NaN())));
			proj.AddKey("FalseEasting", to_lgireal(sref.GetProjParm(SRS_PP_FALSE_EASTING, std::numeric_limits<double>::quiet_NaN())));
			proj.AddKey("FalseNorthing", to_lgireal(sref.GetProjParm(SRS_PP_FALSE_NORTHING, std::numeric_limits<double>::quiet_NaN())));
		} else if (!strcmp(val, SRS_PT_GNOMONIC)) {
			proj.AddKey("ProjectionType", PoDoFo::PdfString("GN"));
			proj.AddKey("OriginLatitude", to_lgireal(sref.GetProjParm(SRS_PP_LATITUDE_OF_ORIGIN, std::numeric_limits<double>::quiet_NaN())));
			proj.AddKey("CentralMeridian", to_lgireal(sref.GetProjParm(SRS_PP_CENTRAL_MERIDIAN, std::numeric_limits<double>::quiet_NaN())));
			proj.AddKey("FalseEasting", to_lgireal(sref.GetProjParm(SRS_PP_FALSE_EASTING, std::numeric_limits<double>::quiet_NaN())));
			proj.AddKey("FalseNorthing", to_lgireal(sref.GetProjParm(SRS_PP_FALSE_NORTHING, std::numeric_limits<double>::quiet_NaN())));
		} else if (!strcmp(val, SRS_PT_LAMBERT_CONFORMAL_CONIC_2SP)) {
			proj.AddKey("ProjectionType", PoDoFo::PdfString("LE"));
			proj.AddKey("OriginLatitude", to_lgireal(sref.GetProjParm(SRS_PP_LATITUDE_OF_ORIGIN, std::numeric_limits<double>::quiet_NaN())));
			proj.AddKey("CentralMeridian", to_lgireal(sref.GetProjParm(SRS_PP_CENTRAL_MERIDIAN, std::numeric_limits<double>::quiet_NaN())));
			proj.AddKey("StandardParallelOne", to_lgireal(sref.GetProjParm(SRS_PP_STANDARD_PARALLEL_1, std::numeric_limits<double>::quiet_NaN())));
			proj.AddKey("StandardParallelTwo", to_lgireal(sref.GetProjParm(SRS_PP_STANDARD_PARALLEL_2, std::numeric_limits<double>::quiet_NaN())));
			proj.AddKey("FalseEasting", to_lgireal(sref.GetProjParm(SRS_PP_FALSE_EASTING, std::numeric_limits<double>::quiet_NaN())));
			proj.AddKey("FalseNorthing", to_lgireal(sref.GetProjParm(SRS_PP_FALSE_NORTHING, std::numeric_limits<double>::quiet_NaN())));
		} else if (!strcmp(val, SRS_PT_MERCATOR_1SP)) {
			proj.AddKey("ProjectionType", PoDoFo::PdfString("MC"));
			proj.AddKey("OriginLatitude", to_lgireal(sref.GetProjParm(SRS_PP_LATITUDE_OF_ORIGIN, std::numeric_limits<double>::quiet_NaN())));
			proj.AddKey("CentralMeridian", to_lgireal(sref.GetProjParm(SRS_PP_CENTRAL_MERIDIAN, std::numeric_limits<double>::quiet_NaN())));
			proj.AddKey("ScaleFactor", to_lgireal(sref.GetProjParm(SRS_PP_SCALE_FACTOR, std::numeric_limits<double>::quiet_NaN())));
			proj.AddKey("FalseEasting", to_lgireal(sref.GetProjParm(SRS_PP_FALSE_EASTING, std::numeric_limits<double>::quiet_NaN())));
			proj.AddKey("FalseNorthing", to_lgireal(sref.GetProjParm(SRS_PP_FALSE_NORTHING, std::numeric_limits<double>::quiet_NaN())));
		} else if (!strcmp(val, SRS_PT_MOLLWEIDE)) {
			proj.AddKey("ProjectionType", PoDoFo::PdfString("MP"));
			proj.AddKey("CentralMeridian", to_lgireal(sref.GetProjParm(SRS_PP_CENTRAL_MERIDIAN, std::numeric_limits<double>::quiet_NaN())));
			proj.AddKey("FalseEasting", to_lgireal(sref.GetProjParm(SRS_PP_FALSE_EASTING, std::numeric_limits<double>::quiet_NaN())));
			proj.AddKey("FalseNorthing", to_lgireal(sref.GetProjParm(SRS_PP_FALSE_NORTHING, std::numeric_limits<double>::quiet_NaN())));
		} else if (!strcmp(val, SRS_PT_NEW_ZEALAND_MAP_GRID)) {
			proj.AddKey("ProjectionType", PoDoFo::PdfString("NT"));
		} else if (!strcmp(val, SRS_PT_HOTINE_OBLIQUE_MERCATOR_TWO_POINT_NATURAL_ORIGIN)) {
			proj.AddKey("ProjectionType", PoDoFo::PdfString("OC"));
			proj.AddKey("OriginLatitude", to_lgireal(sref.GetProjParm(SRS_PP_LATITUDE_OF_CENTER, std::numeric_limits<double>::quiet_NaN())));
			proj.AddKey("LatitudeOne", to_lgireal(sref.GetProjParm(SRS_PP_LATITUDE_OF_POINT_1, std::numeric_limits<double>::quiet_NaN())));
			proj.AddKey("LongitudeOne", to_lgireal(sref.GetProjParm(SRS_PP_LONGITUDE_OF_POINT_1, std::numeric_limits<double>::quiet_NaN())));
			proj.AddKey("LatitudeTwo", to_lgireal(sref.GetProjParm(SRS_PP_LATITUDE_OF_POINT_2, std::numeric_limits<double>::quiet_NaN())));
			proj.AddKey("LongitudeTwo", to_lgireal(sref.GetProjParm(SRS_PP_LONGITUDE_OF_POINT_2, std::numeric_limits<double>::quiet_NaN())));
			proj.AddKey("ScaleFactor", to_lgireal(sref.GetProjParm(SRS_PP_SCALE_FACTOR, std::numeric_limits<double>::quiet_NaN())));
			proj.AddKey("FalseEasting", to_lgireal(sref.GetProjParm(SRS_PP_FALSE_EASTING, std::numeric_limits<double>::quiet_NaN())));
			proj.AddKey("FalseNorthing", to_lgireal(sref.GetProjParm(SRS_PP_FALSE_NORTHING, std::numeric_limits<double>::quiet_NaN())));
		} else if (!strcmp(val, SRS_PT_ORTHOGRAPHIC)) {
			proj.AddKey("ProjectionType", PoDoFo::PdfString("OD"));
			proj.AddKey("OriginLatitude", to_lgireal(sref.GetProjParm(SRS_PP_LATITUDE_OF_ORIGIN, std::numeric_limits<double>::quiet_NaN())));
			proj.AddKey("CentralMeridian", to_lgireal(sref.GetProjParm(SRS_PP_CENTRAL_MERIDIAN, std::numeric_limits<double>::quiet_NaN())));
			proj.AddKey("FalseEasting", to_lgireal(sref.GetProjParm(SRS_PP_FALSE_EASTING, std::numeric_limits<double>::quiet_NaN())));
			proj.AddKey("FalseNorthing", to_lgireal(sref.GetProjParm(SRS_PP_FALSE_NORTHING, std::numeric_limits<double>::quiet_NaN())));
		} else if (!strcmp(val, SRS_PT_POLAR_STEREOGRAPHIC)) {
			proj.AddKey("ProjectionType", PoDoFo::PdfString("PG"));
			proj.AddKey("LatitudeTrueScale", to_lgireal(sref.GetProjParm(SRS_PP_LATITUDE_OF_ORIGIN, std::numeric_limits<double>::quiet_NaN())));
			proj.AddKey("LongitudeDownFromPole", to_lgireal(sref.GetProjParm(SRS_PP_CENTRAL_MERIDIAN, std::numeric_limits<double>::quiet_NaN())));
			proj.AddKey("FalseEasting", to_lgireal(sref.GetProjParm(SRS_PP_FALSE_EASTING, std::numeric_limits<double>::quiet_NaN())));
			proj.AddKey("FalseNorthing", to_lgireal(sref.GetProjParm(SRS_PP_FALSE_NORTHING, std::numeric_limits<double>::quiet_NaN())));
			//SRS_PP_SCALE_FACTOR,
		} else if (!strcmp(val, SRS_PT_POLYCONIC)) {
			proj.AddKey("ProjectionType", PoDoFo::PdfString("PH"));
			proj.AddKey("OriginLatitude", to_lgireal(sref.GetProjParm(SRS_PP_LATITUDE_OF_ORIGIN, std::numeric_limits<double>::quiet_NaN())));
			proj.AddKey("CentralMeridian", to_lgireal(sref.GetProjParm(SRS_PP_CENTRAL_MERIDIAN, std::numeric_limits<double>::quiet_NaN())));
			proj.AddKey("FalseEasting", to_lgireal(sref.GetProjParm(SRS_PP_FALSE_EASTING, std::numeric_limits<double>::quiet_NaN())));
			proj.AddKey("FalseNorthing", to_lgireal(sref.GetProjParm(SRS_PP_FALSE_NORTHING, std::numeric_limits<double>::quiet_NaN())));
		} else if (!strcmp(val, SRS_PT_SINUSOIDAL)) {
			proj.AddKey("ProjectionType", PoDoFo::PdfString("SA"));
			proj.AddKey("CentralMeridian", to_lgireal(sref.GetProjParm(SRS_PP_CENTRAL_MERIDIAN, std::numeric_limits<double>::quiet_NaN())));
			proj.AddKey("FalseEasting", to_lgireal(sref.GetProjParm(SRS_PP_FALSE_EASTING, std::numeric_limits<double>::quiet_NaN())));
			proj.AddKey("FalseNorthing", to_lgireal(sref.GetProjParm(SRS_PP_FALSE_NORTHING, std::numeric_limits<double>::quiet_NaN())));
		} else if (!strcmp(val, SRS_PT_STEREOGRAPHIC)) {
			proj.AddKey("ProjectionType", PoDoFo::PdfString("SD"));
			proj.AddKey("OriginLatitude", to_lgireal(sref.GetProjParm(SRS_PP_LATITUDE_OF_ORIGIN, std::numeric_limits<double>::quiet_NaN())));
			proj.AddKey("CentralMeridian", to_lgireal(sref.GetProjParm(SRS_PP_CENTRAL_MERIDIAN, std::numeric_limits<double>::quiet_NaN())));
			proj.AddKey("FalseEasting", to_lgireal(sref.GetProjParm(SRS_PP_FALSE_EASTING, std::numeric_limits<double>::quiet_NaN())));
			proj.AddKey("FalseNorthing", to_lgireal(sref.GetProjParm(SRS_PP_FALSE_NORTHING, std::numeric_limits<double>::quiet_NaN())));
			//SRS_PP_SCALE_FACTOR ?
		} else if (!strcmp(val, SRS_PT_TRANSVERSE_MERCATOR)) {
			proj.AddKey("ProjectionType", PoDoFo::PdfString("TC"));
			proj.AddKey("OriginLatitude", to_lgireal(sref.GetProjParm(SRS_PP_LATITUDE_OF_ORIGIN, std::numeric_limits<double>::quiet_NaN())));
			proj.AddKey("CentralMeridian", to_lgireal(sref.GetProjParm(SRS_PP_CENTRAL_MERIDIAN, std::numeric_limits<double>::quiet_NaN())));
			proj.AddKey("ScaleFactor", to_lgireal(sref.GetProjParm(SRS_PP_SCALE_FACTOR, std::numeric_limits<double>::quiet_NaN())));
			proj.AddKey("FalseEasting", to_lgireal(sref.GetProjParm(SRS_PP_FALSE_EASTING, std::numeric_limits<double>::quiet_NaN())));
			proj.AddKey("FalseNorthing", to_lgireal(sref.GetProjParm(SRS_PP_FALSE_NORTHING, std::numeric_limits<double>::quiet_NaN())));
		} else if (!strcmp(val, SRS_PT_CYLINDRICAL_EQUAL_AREA)) {
			proj.AddKey("ProjectionType", PoDoFo::PdfString("TX"));
			proj.AddKey("OriginLatitude", to_lgireal(sref.GetProjParm(SRS_PP_STANDARD_PARALLEL_1, std::numeric_limits<double>::quiet_NaN())));
			proj.AddKey("CentralMeridian", to_lgireal(sref.GetProjParm(SRS_PP_CENTRAL_MERIDIAN, std::numeric_limits<double>::quiet_NaN())));
			proj.AddKey("ScaleFactor", to_lgireal(sref.GetProjParm(SRS_PP_SCALE_FACTOR, 1.0)));
			proj.AddKey("FalseEasting", to_lgireal(sref.GetProjParm(SRS_PP_FALSE_EASTING, std::numeric_limits<double>::quiet_NaN())));
			proj.AddKey("FalseNorthing", to_lgireal(sref.GetProjParm(SRS_PP_FALSE_NORTHING, std::numeric_limits<double>::quiet_NaN())));
		} else if (!strcmp(val, SRS_PT_VANDERGRINTEN)) {
			proj.AddKey("ProjectionType", PoDoFo::PdfString("VA"));
			proj.AddKey("CentralMeridian", to_lgireal(sref.GetProjParm(SRS_PP_CENTRAL_MERIDIAN, std::numeric_limits<double>::quiet_NaN())));
			proj.AddKey("FalseEasting", to_lgireal(sref.GetProjParm(SRS_PP_FALSE_EASTING, std::numeric_limits<double>::quiet_NaN())));
			proj.AddKey("FalseNorthing", to_lgireal(sref.GetProjParm(SRS_PP_FALSE_NORTHING, std::numeric_limits<double>::quiet_NaN())));
		} else {
			std::cerr << "unknown/unsupported projection" << std::endl;
			return false;
		}
	} else {
		std::cerr << "unknown projection" << std::endl;
		return false;
	}
	PoDoFo::PdfDictionary datum;
	{
		const OGR_SRSNode *attr(sref.GetAttrNode("PROJCS|GEOGCS|DATUM"));
		if (attr)
			datum.AddKey("Description", PoDoFo::PdfString(attr->GetChild(0)->GetValue()));
	}
	{
		PoDoFo::PdfDictionary towgs84;
		double coeff[7];
		if (sref.GetTOWGS84(coeff, sizeof(coeff)/sizeof(coeff[0])) != OGRERR_NONE) {
			std::cerr << "cannot get TOWGS84" << std::endl;
		} else {
			towgs84.AddKey("dx", to_lgireal(coeff[0]));
			towgs84.AddKey("dy", to_lgireal(coeff[1]));
			towgs84.AddKey("dz", to_lgireal(coeff[2]));
			towgs84.AddKey("rx", to_lgireal(coeff[3]));
			towgs84.AddKey("ry", to_lgireal(coeff[4]));
			towgs84.AddKey("rz", to_lgireal(coeff[5]));
			towgs84.AddKey("sf", to_lgireal(coeff[6]));
			datum.AddKey("ToWGS84", towgs84);
		}
	}
	{
		std::string ename;
		const OGR_SRSNode *attr(sref.GetAttrNode("PROJCS|GEOGCS|DATUM"));
		if (attr) {
			const char *val(attr->GetChild(0)->GetValue());
			if (val) {
				if (!strcmp(val, "Airy 1830"))
					ename = "AA";
				else if (!strcmp(val, "Modified Airy"))
					ename = "AM";
				else if (!strcmp(val, "Australian Natl & S. Amer. 1969"))
					ename = "AN";
				else if (!strcmp(val, "Bessel 1841 (Namibia)"))
					ename = "BN";
				else if (!strcmp(val, "Bessel 1841"))
					ename = "BR";
				else if (!strcmp(val, "Clarke 1866"))
					ename = "CC";
				else if (!strcmp(val, "Clarke 1880 mod."))
					ename = "CD";
				else if (!strcmp(val, "Everest 1830"))
					ename = "EA";
				else if (!strcmp(val, "Everest (Sabah & Sarawak)"))
					ename = "EB";
				else if (!strcmp(val, "Everest 1956"))
					ename = "EC";
				else if (!strcmp(val, "Everest 1969"))
					ename = "ED";
				else if (!strcmp(val, "Everest 1948"))
					ename = "EE";
				else if (!strcmp(val, "Modified Fischer 1960"))
					ename = "FA";
				else if (!strcmp(val, "Helmert 1906"))
					ename = "HE";
				else if (!strcmp(val, "Hough"))
					ename = "HO";
				else if (!strcmp(val, "GRS 1980"))
					ename = "RF";
				else if (!strcmp(val, "WGS 72"))
					ename = "WD";
				else if (!strcmp(val, "WGS84") || !strcmp(val, "WGS_1984"))
					ename = "WE";
			}
		}
		if (ename.empty()) {
			PoDoFo::PdfDictionary ell;
			{
				const OGR_SRSNode *attr(sref.GetAttrNode("PROJCS|GEOGCS|DATUM|SPHEROID"));
				if (attr)
					ell.AddKey("Description", PoDoFo::PdfString(attr->GetChild(0)->GetValue()));
			}
			ell.AddKey("SemiMajorAxis", to_lgireal(sref.GetSemiMajor()));
			ell.AddKey("SemiMinorAxis", to_lgireal(sref.GetSemiMinor()));
			ell.AddKey("InvFlattening", to_lgireal(sref.GetInvFlattening()));
			datum.AddKey("Ellipsoid", ell);
		} else {
			datum.AddKey("Ellipsoid", PoDoFo::PdfString(ename));
		}
	}
	proj.AddKey("Datum", datum);
	return true;
}

static void georef_page(PoDoFo::PdfDocument& doc, PoDoFo::PdfPage *page, const Json::Value& desc)
{
	if (!page)
		return;
	bool inverty(desc.isMember("inverty") && desc["inverty"].isBool() && desc["inverty"].asBool());
	OGRSpatialReference sref;
	int epsg(-1);
	if (desc.isMember("coordsys")) {
		const Json::Value& csys(desc["coordsys"]);
		if (csys.isIntegral()) {
			epsg = csys.asInt();
			if (sref.importFromEPSG(epsg) != OGRERR_NONE) {
				std::ostringstream oss;
				oss << "Cannot set projection EPSG::" << csys.asInt();
				throw std::runtime_error(oss.str());
			}
		} else if (csys.isString()) {
			std::string c(csys.asString());
			char *cp(const_cast<char *>(c.c_str()));
			if (sref.importFromWkt(&cp) != OGRERR_NONE) {
				std::ostringstream oss;
				oss << "Cannot set projection " << c;
				throw std::runtime_error(oss.str());
			}
		}
	}
	std::vector<xy_t> bound;
	if (desc.isMember("bound"))
		bound = GetXYArray(desc["bound"]);
	std::vector<GeoRefPt> georef;
	if (desc.isMember("georef") && desc["georef"].isArray()) {
		const Json::Value& gref(desc["georef"]);
		for (Json::ArrayIndex i = 0; i < gref.size(); ++i)
			georef.push_back(GeoRefPt(gref[i]));
	}
	const PoDoFo::PdfRect trimbox(page->GetTrimBox());
	{
		if (false) {
			OGRSpatialReference sref1, sref2;
			sref1.importFromEPSG(4326);
			sref2.importFromEPSG(21781/*2056*/);
			OGRCoordinateTransformation *trans(OGRCreateCoordinateTransformation(&sref1, &sref2));
			double y(46+57/60.0+08.66/3600.0), x(7+26/60.0+22.50/3600.0);
			std::cerr << "Bern fwd transform: " << x << ' ' << y;
			trans->Transform(1, &x, &y);
			std::cerr << " -> " << x << ' ' << y << std::endl;
			OGRFree(trans);
		}
		OGRSpatialReference *latlon(sref.CloneGeogCS());
		if (false) {
			OGRCoordinateTransformation *trans(OGRCreateCoordinateTransformation(&sref, latlon));
			double x(600000), y(200000);
			std::cerr << "Bern rev transform: " << x << ' ' << y;
			trans->Transform(1, &x, &y);
			std::cerr << " -> " << x << ' ' << y << std::endl;
			OGRFree(trans);
		}
		OGRCoordinateTransformation *trans(OGRCreateCoordinateTransformation(latlon, &sref));
		if (false) {
			double x(46+57/60.0+08.66/3600.0), y(7+26/60.0+22.50/3600.0);
			std::cerr << "Bern fwd transform: " << x << ' ' << y;
			trans->Transform(1, &x, &y);
			std::cerr << " -> " << x << ' ' << y << std::endl;
		}
		{
			char *cp(0);
			sref.exportToPrettyWkt(&cp);
			std::cerr << "WKT: " << cp << std::endl;
			OGRFree(cp);
		}
		for (std::vector<GeoRefPt>::iterator i(georef.begin()), e(georef.end()); i != e;) {
			if (i->get_coord().is_invalid() || std::isnan(i->get_xy().first) || std::isnan(i->get_xy().second)) {
				i = georef.erase(i);
				e = georef.end();
				continue;
			}
			xy_t xy(i->get_xy());
			if (inverty)
				xy.second = trimbox.GetHeight() - xy.second;
			xy.first += trimbox.GetLeft();
			xy.second += trimbox.GetBottom();
			i->set_xy(xy);
			{
				xy_t xy(i->get_coord().get_lon_deg_dbl(), i->get_coord().get_lat_deg_dbl());
				if (!trans->Transform(1, &xy.first, &xy.second) || std::isnan(xy.first) || std::isnan(xy.second)) {
					if (true)
						std::cerr << "unable to transform " << i->get_coord().get_lat_str2()
							  << ' ' << i->get_coord().get_lon_str2() << std::endl;
					i = georef.erase(i);
					e = georef.end();
					continue;
				}
				i->set_coordxy(xy);
			}
			++i;
		}
		OGRFree(trans);
		OGRFree(latlon);
	}
	if (georef.size() < 3)
		throw std::runtime_error("not enough georeferencing points");
	double bminx(std::numeric_limits<double>::max());
	double bmaxx(std::numeric_limits<double>::min());
	double bminy(std::numeric_limits<double>::max());
	double bmaxy(std::numeric_limits<double>::min());
	for (std::vector<xy_t>::iterator i(bound.begin()), e(bound.end()); i != e; ++i) {
		if (std::isnan(i->first) || std::isnan(i->second))
			throw std::runtime_error("invalid bound");
		if (inverty)
			i->second = trimbox.GetHeight() - i->second;
		i->first += trimbox.GetLeft();
		i->second += trimbox.GetBottom();
		bminx = std::min(bminx, i->first);
		bmaxx = std::max(bmaxx, i->first);
		bminy = std::min(bminy, i->second);
		bmaxy = std::max(bmaxy, i->second);
	}
	if (bmaxx <= bminx || bmaxy <= bminy)
		throw std::runtime_error("bound empty");
	if (true) {
		for (std::vector<GeoRefPt>::const_iterator i(georef.begin()), e(georef.end()); i != e; ++i) {
			std::cout << "Point " << i->get_coord().get_lat_str2() << ' ' << i->get_coord().get_lon_str2()
				  << " T " << i->get_coordxy().first << ' ' << i->get_coordxy().second
				  << " P " << i->get_xy().first << ' ' << i->get_xy().second
				  << ' ' << i->get_name() << std::endl;
		}
	}
	PoDoFo::PdfObject *vp(page->GetObject()->GetIndirectKey("VP"));
	if (!vp) {
		page->GetObject()->GetDictionary().AddKey("VP", PoDoFo::PdfObject(PoDoFo::PdfArray()));
		vp = page->GetObject()->GetIndirectKey("VP");
		if (!vp)
			throw std::runtime_error("cannot create viewport directory");
	}
	if (!vp->IsArray())
		throw std::runtime_error("VP (viewport) is not an array");
	vp->GetArray().push_back(PoDoFo::PdfObject());
	PoDoFo::PdfObject& vpel(vp->GetArray().back());
	vpel.GetDictionary().AddKey("Type", PoDoFo::PdfName("Viewport"));
	double bwidth(bmaxx - bminx), bheight(bmaxy - bminy);
	{
		PoDoFo::PdfRect bbox(bminx, bminy, bwidth, bheight);
		PoDoFo::PdfVariant bboxvar;
		bbox.ToVariant(bboxvar);
		vpel.GetDictionary().AddKey("BBox", bboxvar);
	}
	PoDoFo::PdfObject *measure(0);
	if (false) {
		vpel.GetDictionary().AddKey("Measure", PoDoFo::PdfObject());
		measure = vpel.GetDictionary().GetKey("Measure");
	} else {
		measure = doc.GetObjects()->CreateObject(PoDoFo::PdfVariant(PoDoFo::PdfDictionary()));
		vpel.GetDictionary().AddKey("Measure", measure->Reference());
	}
	if (!measure || !measure->IsDictionary())
		throw std::runtime_error("cannot add Measure");
	measure->GetDictionary().AddKey("Type", PoDoFo::PdfName("Measure"));
	measure->GetDictionary().AddKey("Subtype", PoDoFo::PdfName("GEO"));
	{
		PoDoFo::PdfArray bbox;
		bbox.push_back(PoDoFo::PdfVariant(0.0));
		bbox.push_back(PoDoFo::PdfVariant(0.0));
		bbox.push_back(PoDoFo::PdfVariant(0.0));
		bbox.push_back(PoDoFo::PdfVariant(1.0));
		bbox.push_back(PoDoFo::PdfVariant(1.0));
		bbox.push_back(PoDoFo::PdfVariant(1.0));
		bbox.push_back(PoDoFo::PdfVariant(1.0));
		bbox.push_back(PoDoFo::PdfVariant(0.0));
		measure->GetDictionary().AddKey("Bounds", PoDoFo::PdfVariant(bbox));
	}
	{
		PoDoFo::PdfObject *gcs(0);
		if (false) {
			measure->GetDictionary().AddKey("GCS", PoDoFo::PdfObject());
			gcs = measure->GetDictionary().GetKey("GCS");
		} else {
			gcs = doc.GetObjects()->CreateObject(PoDoFo::PdfVariant(PoDoFo::PdfDictionary()));
			measure->GetDictionary().AddKey("GCS", gcs->Reference());
		}
		if (!gcs || !gcs->IsDictionary())
			throw std::runtime_error("cannot add GCS");
		gcs->GetDictionary().AddKey("Type", PoDoFo::PdfName("PROJCS"));
		{
			char *cp(0);
			sref.exportToWkt(&cp);
			PoDoFo::PdfString wkt(cp);
			OGRFree(cp);
			gcs->GetDictionary().AddKey("WKT", wkt);
		}
		if (epsg != -1)
			gcs->GetDictionary().AddKey("EPSG", PoDoFo::PdfVariant((PoDoFo::pdf_int64)epsg));
	}
	{
		PoDoFo::PdfArray pdu;
		pdu.push_back(PoDoFo::PdfVariant(PoDoFo::PdfName("NM")));
		pdu.push_back(PoDoFo::PdfVariant(PoDoFo::PdfName("SQM")));
		pdu.push_back(PoDoFo::PdfVariant(PoDoFo::PdfName("DEG")));
		measure->GetDictionary().AddKey("PDU", PoDoFo::PdfVariant(pdu));
	}
	{
		PoDoFo::PdfArray gpts, lpts;
		for (std::vector<GeoRefPt>::const_iterator i(georef.begin()), e(georef.end()); i != e; ++i) {
			gpts.push_back(PoDoFo::PdfVariant(i->get_coord().get_lat_deg_dbl()));
			gpts.push_back(PoDoFo::PdfVariant(i->get_coord().get_lon_deg_dbl()));
			lpts.push_back(PoDoFo::PdfVariant((i->get_xy().first - bminx) / bwidth));
			lpts.push_back(PoDoFo::PdfVariant((i->get_xy().second - bminy) / bheight));
		}
		measure->GetDictionary().AddKey("GPTS", PoDoFo::PdfVariant(gpts));
		measure->GetDictionary().AddKey("LPTS", PoDoFo::PdfVariant(lpts));
	}
	// OGC Best Practice Structures
	PoDoFo::PdfObject *lgidict(0);
	{
		PoDoFo::PdfDictionary proj;
		if (lgi_convert(proj, sref)) {
			PoDoFo::PdfObject *lgi(page->GetObject()->GetIndirectKey("LGIDict"));
			if (!lgi) {
				page->GetObject()->GetDictionary().AddKey("LGIDict", PoDoFo::PdfObject(PoDoFo::PdfArray()));
				lgi = page->GetObject()->GetIndirectKey("LGIDict");
				if (!lgi)
					throw std::runtime_error("cannot create LGI directory array");
			}
			if (!lgi->IsArray())
				throw std::runtime_error("LGI is not an array");
			lgidict = doc.GetObjects()->CreateObject(PoDoFo::PdfVariant(PoDoFo::PdfDictionary()));
			lgi->GetArray().push_back(lgidict->Reference());
			lgidict->GetDictionary().AddKey("Projection", proj);
		}
	}
	if (lgidict) {
		lgidict->GetDictionary().AddKey("Type", PoDoFo::PdfName("LGIDict"));
		lgidict->GetDictionary().AddKey("Version", PoDoFo::PdfString("2.1"));
		{
			PoDoFo::PdfDictionary disp;
			disp.AddKey("Type", PoDoFo::PdfName("Projection"));
			disp.AddKey("ProjectionType", PoDoFo::PdfString("GEODETIC"));
			disp.AddKey("Datum", PoDoFo::PdfString("WE")); // WGS 84
			lgidict->GetDictionary().AddKey("Display", disp);
		}
		{
			PoDoFo::PdfArray nl;
			double area(0);
			for (std::vector<xy_t>::size_type n(bound.size()), i0(n - 1), i1(0); i1 != n; i0 = i1, ++i1)
				area += bound[i0].first * bound[i1].second - bound[i1].first * bound[i0].second;
			if (area >= 0) {
				for (std::vector<xy_t>::const_iterator i(bound.begin()), e(bound.end()); i != e; ++i) {
					nl.push_back(to_lgireal(i->first));
					nl.push_back(to_lgireal(i->second));
				}
			} else {
				for (std::vector<xy_t>::const_reverse_iterator i(bound.rbegin()), e(bound.rend()); i != e; ++i) {
					nl.push_back(to_lgireal(i->first));
					nl.push_back(to_lgireal(i->second));
				}
			}
			lgidict->GetDictionary().AddKey("Neatline", PoDoFo::PdfVariant(nl));
		}
		{
			PoDoFo::PdfArray reg;
			for (std::vector<GeoRefPt>::const_iterator i(georef.begin()), e(georef.end()); i != e; ++i) {
				PoDoFo::PdfArray rp;
				rp.push_back(to_lgireal(i->get_xy().first));
				rp.push_back(to_lgireal(i->get_xy().second));
				rp.push_back(to_lgireal(i->get_coordxy().first));
				rp.push_back(to_lgireal(i->get_coordxy().second));
				reg.push_back(rp);
			}
			lgidict->GetDictionary().AddKey("Registration", PoDoFo::PdfVariant(reg));
		}
	}
	// compute possible transformation matrix
#ifdef HAVE_EIGEN3
	if (lgidict) {
		Eigen::Matrix<double, 2, 3> tm;
		{
			Eigen::MatrixXd A(georef.size(), 3);
			Eigen::VectorXd bx(georef.size());
			Eigen::VectorXd by(georef.size());
			for (std::vector<GeoRefPt>::size_type i(0); i < georef.size(); ++i) {
				A(i, 0) = georef[i].get_xy().first;
				A(i, 1) = georef[i].get_xy().second;
				A(i, 2) = 1;
				bx(i) = georef[i].get_coordxy().first;
				by(i) = georef[i].get_coordxy().second;
			}
			Eigen::MatrixXd ATAinvAT((A.transpose() * A).inverse() * A.transpose());
			tm.row(0) = ATAinvAT * bx;
			tm.row(1) = ATAinvAT * by;
		}
		PoDoFo::PdfArray ctm;
		for (unsigned int i = 0; i < 3; ++i)
			for (unsigned int j = 0; j < 2; ++j)
				ctm.push_back(to_lgireal(tm(j, i)));
		lgidict->GetDictionary().AddKey("CTM", PoDoFo::PdfVariant(ctm));
	}
	if (true) {
		Eigen::Matrix<double, 2, 3> tm;
		{
			Eigen::MatrixXd A(georef.size(), 3);
			Eigen::VectorXd bx(georef.size());
			Eigen::VectorXd by(georef.size());
			for (std::vector<GeoRefPt>::size_type i(0); i < georef.size(); ++i) {
				A(i, 0) = georef[i].get_coordxy().first;
				A(i, 1) = georef[i].get_coordxy().second;
				A(i, 2) = 1;
				bx(i) = georef[i].get_xy().first;
				by(i) = georef[i].get_xy().second;
			}
			Eigen::MatrixXd ATAinvAT((A.transpose() * A).inverse() * A.transpose());
			tm.row(0) = ATAinvAT * bx;
			tm.row(1) = ATAinvAT * by;
		}
		std::cout << "Transformation Matrix: " << tm << std::endl;
		for (std::vector<GeoRefPt>::const_iterator i(georef.begin()), e(georef.end()); i != e; ++i) {
			Eigen::Vector2d xyest;
			{
				Eigen::Vector3d xy;
				xy(0) = i->get_coordxy().first;
				xy(1) = i->get_coordxy().second;
				xy(2) = 1;
				xyest = tm * xy;
			}
			std::cout << "Point " << i->get_coordxy().first << ' ' << i->get_coordxy().second
				  << " P " << i->get_xy().first << ' ' << i->get_xy().second
				  << " E " << xyest(0) << ' ' << xyest(1)
				  << ' ' << i->get_name() << std::endl;
		}
	}
#endif
}

static void georef_file(const Json::Value& desc)
{
	if (!desc.isObject())
		return;
	if (!desc.isMember("infile") || !desc["infile"].isString())
		return;
	PoDoFo::PdfMemDocument doc;
	doc.Load(desc["infile"].asString().c_str());
	PoDoFo::PdfObject *pageskids(GetPagesKids(doc));
	std::cerr << desc["infile"].asString() << ": " << pageskids->GetArray().size() << " pages" << std::endl;
	if (desc.isMember("pages") && desc["pages"].isArray()) {
		const Json::Value& pdesc(desc["pages"]);
		for (Json::ArrayIndex i = 0; i < pdesc.size(); ++i) {
			if ((int)i >= doc.GetPageCount())
				break;
			PoDoFo::PdfPage *page(doc.GetPage(i));
			if (!page)
				continue;
			if (pdesc[i].isArray()) {
				for (Json::ArrayIndex j = 0; j < pdesc[i].size(); ++j)
					georef_page(doc, page, pdesc[i][j]);
			} else {
				georef_page(doc, page, pdesc[i]);
			}
		}
	}
	if (!desc.isMember("outfile") || !desc["outfile"].isString())
		return;
	doc.Write(desc["outfile"].asString().c_str());
}

/* ---------------------------------------------------------------------- */

int main(int argc, char *argv[])
{
	static struct option long_options[] = {
		{ "georef", no_argument, 0, 'g' },
		{0, 0, 0, 0}
	};
	Glib::init();
	Gio::init();
	typedef enum {
		mode_dump,
		mode_georef
	} mode_t;
	mode_t mode(mode_dump);
	int c, err(0);

	while ((c = getopt_long(argc, argv, "g", long_options, 0)) != EOF) {
		switch (c) {
		case 'g':
			mode = mode_georef;
			break;

		default:
			err++;
			break;
		}
	}
	if (err) {
		std::cerr << "usage: vfrpdfmanip" << std::endl;
		return EX_USAGE;
	}
	try {
        	for (; optind < argc; optind++) {
			switch (mode) {
			default:
				std::cerr << "Parsing file " << argv[optind] << std::endl;
				dump_pdf(argv[optind]);
				break;

			case mode_georef:
			{
				std::cerr << "Parsing JSON description file " << argv[optind] << std::endl;
				Json::Value fdesc;
				{
					std::ifstream is(argv[optind]);
					if (!is.is_open()) {
						std::cerr << "Cannot open file " << argv[optind] << std::endl;
						break;
					}
					Json::Reader reader;
					if (!reader.parse(is, fdesc, true)) {
						std::cerr << "Cannot parse file " << argv[optind]
							  << ": " << reader.getFormattedErrorMessages()  << std::endl;
						break;
					}
				}
				if (fdesc.isArray()) {
					for (Json::ArrayIndex i = 0; i < fdesc.size(); ++i)
						georef_file(fdesc[i]);
				} else {
					georef_file(fdesc);
				}
 				break;
			}
			}
		}
	} catch (const Glib::Exception& ex) {
		std::cerr << "Glib exception: " << ex.what() << std::endl;
		return EX_DATAERR;
	} catch (const std::exception& ex) {
		std::cerr << "exception: " << ex.what() << std::endl;
		return EX_DATAERR;
	}
	return EX_OK;
}
