#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <iomanip>

#ifdef HAVE_GDAL
#if defined(HAVE_GDAL2)
#include <gdal.h>
#include <ogr_geometry.h>
#include <ogr_feature.h>
#include <ogrsf_frmts.h>
#else
#include <ogrsf_frmts.h>
#endif
#endif

#include "adrbdry.hh"

using namespace ADR;

ParseOGR::ParseOGR(Database& db, bool verbose)
	: m_db(db), m_errorcnt(0), m_warncnt(0), m_verbose(verbose)
{
	init_thematicmapping_tables();
	// try Write Ahead Logging mode
	m_db.set_wal();
}

ParseOGR::~ParseOGR()
{
}

void ParseOGR::global_init(void)
{
#ifdef HAVE_GDAL
	OGRRegisterAll();
#endif
}

void ParseOGR::parse(const std::string& file)
{
        if (file.empty())
                return;
#ifdef HAVE_GDAL
#if defined(HAVE_GDAL2)
	GDALDataset *ds((GDALDataset *)GDALOpenEx(file.c_str(), GDAL_OF_READONLY, 0, 0, 0));
#else
	OGRSFDriver *drv(0);
	OGRDataSource *ds(OGRSFDriverRegistrar::Open(file.c_str(), false, &drv));
#endif
	if (!ds) {
		std::cerr << "Cannot open borders file " << file << std::endl;
		return;
	}
	typedef std::set<std::string> countries_t;
	countries_t countries;
	for (int layernr = 0; layernr < ds->GetLayerCount(); ++layernr) {
		OGRLayer *layer(ds->GetLayer(layernr));
		OGRFeatureDefn *layerdef(layer->GetLayerDefn());
		if (m_verbose)
			std::cout << "  Layer Name: " << layerdef->GetName() << std::endl;
		if (m_verbose) {
			for (int fieldnr = 0; fieldnr < layerdef->GetFieldCount(); ++fieldnr) {
				OGRFieldDefn *fielddef(layerdef->GetFieldDefn(fieldnr));
				std::cout << "    Field Name: " << fielddef->GetNameRef()
					  << " type " << OGRFieldDefn::GetFieldTypeName(fielddef->GetType())  << std::endl;
			}
		}
		int name_index(layerdef->GetFieldIndex("NAME"));
		if (name_index == -1)
			std::cerr << "Layer Name: " << layerdef->GetName() << " has no NAME field, skipping" << std::endl;
		layer->ResetReading();
		while (OGRFeature *feature = layer->GetNextFeature()) {
			std::cout << "  Feature: " << feature->GetFID() << std::endl;
			if (m_verbose) {
				for (int fieldnr = 0; fieldnr < layerdef->GetFieldCount(); ++fieldnr) {
					OGRFieldDefn *fielddef(layerdef->GetFieldDefn(fieldnr));
					std::cout << "    Field Name: " << fielddef->GetNameRef() << " Value ";
					switch (fielddef->GetType()) {
					case OFTInteger:
						std::cout << feature->GetFieldAsInteger(fieldnr);
						break;

					case OFTReal:
						std::cout << feature->GetFieldAsDouble(fieldnr);
						break;

					case OFTString:
					default:
						std::cout << feature->GetFieldAsString(fieldnr);
						break;
					}
					std::cout << std::endl;
				}
				OGRGeometry *geom(feature->GetGeometryRef());
				char *wkt;
				geom->exportToWkt(&wkt);
				std::cout << "  Geom " << geom->getGeometryName() << ": " << wkt << std::endl;
				delete wkt;
			}
			if (name_index == -1) {
				OGRFeature::DestroyFeature(feature);
				feature = 0;
				continue;
			}
			const char *name_code(feature->GetFieldAsString(name_index));
			if (!name_code) {
				std::cerr << "Layer Name: " << layerdef->GetName() << " Feature: " << feature->GetFID()
					  << " has no NAME, skipping" << std::endl;
				OGRFeature::DestroyFeature(feature);
				feature = 0;
				continue;
			}
			//std::string nameu(Glib::ustring(name_code).uppercase());
			std::string nameu(name_code);
			// remapping
			{
				namemap_t::const_iterator i(m_namemap.find(nameu));
				if (false && i == m_namemap.end())
					i = m_namemap.find(Glib::ustring(nameu).uppercase());
				if (i != m_namemap.end())
					nameu = i->second;
			}
			UUID uuid(UUID::from_countryborder(nameu));
			Airspace::ptr_t p;
			{
				Object::const_ptr_t p1(m_db.load(uuid));
				if (p1) {
					p = Airspace::ptr_t::cast_dynamic(Object::ptr_t::cast_const(p1));
					if (!p) {
						p1->print(std::cerr << "Object ") << " not " << to_str(Airspace::get_static_type()) << ", discarding";
						++m_warncnt;
					}
				}
			}
			AirspaceTimeSlice ts;
			if (p && p->size()) {
				const AirspaceTimeSlice& ts1(p->operator[](0).as_airspace());
				if (ts1.is_valid())
					ts = ts1;
			}
			if (!p) {
				p = Airspace::ptr_t(new Airspace(uuid));
				p->set_modified();
				p->set_dirty();
			}
			ts.set_starttime(0);
			ts.set_endtime(std::numeric_limits<uint64_t>::max());
			ts.set_ident(name_code);
			ts.set_type(AirspaceTimeSlice::type_border);
			ts.set_localtype(AirspaceTimeSlice::localtype_none);
			ts.set_icao(false);
			ts.set_flexibleuse(false);
			ts.get_components().clear();
			ts.get_components().resize(1);
			AirspaceTimeSlice::Component& comp(ts.get_components()[0]);
			comp.set_operator(AirspaceTimeSlice::Component::operator_base);
			comp.set_full_geometry(false);
			OGRGeometry *geom(feature->GetGeometryRef());
			if (false && m_verbose)
				std::cout << "Layer Name: " << layerdef->GetName()
					  << " Feature: " << feature->GetFID()
					  << " Geom: " << geom->getGeometryName() << std::endl;
			loadborder(comp.get_poly(), geom, layerdef, feature);
			OGRFeature::DestroyFeature(feature);
			feature = 0;
			if (m_verbose)
				std::cout << "UUID " << p->get_uuid() << " country \"" << ts.get_ident()
					  << "\" (translated \"" << nameu << "\")" << std::endl;
			p->add_timeslice(ts);
			if (p->is_dirty()) {
			        m_db.save_temp(p);
				if (m_verbose)
					p->print(std::cout);
			}
			{
				std::pair<countries_t::iterator,bool> ins(countries.insert(ts.get_ident()));
				if (!ins.second) {
					std::cerr << "Duplicate Country: " << ts.get_ident() << std::endl;
					++m_warncnt;
				}
			}
		}
	}
#if defined(HAVE_GDAL2)
	GDALClose(ds);
#else
	OGRDataSource::DestroyDataSource(ds);
#endif
	if (true) {
		std::cerr << "Borders file " << file << " Countries: ";
		bool subseq(false);
		for (countries_t::const_iterator ci(countries.begin()), ce(countries.end()); ci != ce; ++ci) {
			if (subseq)
				std::cerr << ", ";
			subseq = true;
			std::cerr << "\"" << *ci << "\"";
		}
		std::cerr << std::endl;
	}
#else
	std::cerr << "Cannot process borders file " << file << std::endl;
	++m_errorcnt;
#endif
}

#ifdef HAVE_GDAL

void ParseOGR::loadborder(MultiPolygonHole& poly, OGRGeometry *geom, OGRFeatureDefn *layerdef, OGRFeature *feature)
{
	poly.clear();
	if (!geom)
		return;
	switch (geom->getGeometryType()) {
	case wkbPolygon:
	case wkbPolygon25D:
	{
		OGRPolygon *ogrpoly(dynamic_cast<OGRPolygon *>(geom));
		if (!ogrpoly) {
			if (layerdef && feature)
				std::cerr << "Layer Name: " << layerdef->GetName() << " Feature: " << feature->GetFID()
					  << " Geom: " << geom->getGeometryName()
					  << " cannot convert to polygon, skipping" << std::endl;
			break;
		}
		PolygonHole ph;
		loadborder(ph, ogrpoly, layerdef, feature);
		poly.push_back(ph);
		break;
	}

	case wkbMultiPoint:
	case wkbMultiLineString:
	case wkbMultiPolygon:
	case wkbGeometryCollection:
	case wkbMultiPoint25D:
	case wkbMultiLineString25D:
	case wkbMultiPolygon25D:
	case wkbGeometryCollection25D:
	{
		OGRGeometryCollection *coll(dynamic_cast<OGRGeometryCollection *>(geom));
		if (!coll) {
			if (layerdef && feature)
				std::cerr << "Layer Name: " << layerdef->GetName() << " Feature: " << feature->GetFID()
					  << " Geom: " << geom->getGeometryName()
					  << " cannot convert to collection, skipping" << std::endl;
			break;
		}
		for (int i = 0, j = coll->getNumGeometries(); i < j; ++i) {
			OGRGeometry *geom(coll->getGeometryRef(i));
			MultiPolygonHole mph;
			loadborder(mph, geom, layerdef, feature);
			poly.insert(poly.end(), mph.begin(), mph.end());
		}
		break;
	}

	default:
		OGRGeometry *geom1(geom->Polygonize());
		if (!geom1 || (geom1->getGeometryType() != wkbPolygon && geom1->getGeometryType() != wkbMultiPolygon)) {
			if (layerdef && feature)
				std::cerr << "Layer Name: " << layerdef->GetName() << " Feature: " << feature->GetFID()
					  << " Geom: " << geom->getGeometryName()
					  << " is not a polygon and cannot be converted to a polygon, skipping" << std::endl;
			delete geom1;
			break;
		}
		loadborder(poly, geom1, layerdef, feature);
		delete geom1;
		break;
	}
}

void ParseOGR::loadborder(PolygonHole& ph, OGRPolygon *poly, OGRFeatureDefn *layerdef, OGRFeature *feature)
{
	ph.clear();
	if (!poly)
		return;
	{
		PolygonSimple ps;
		loadborder(ps, poly->getExteriorRing(), layerdef, feature);
		ph.set_exterior(ps);
	}
	for (int i = 0, j = poly->getNumInteriorRings(); i < j; ++i) {
		PolygonSimple ps;
		loadborder(ps, poly->getInteriorRing(i), layerdef, feature);
		ph.add_interior(ps);
	}
	ph.normalize_boostgeom();
}

void ParseOGR::loadborder(PolygonSimple& poly, OGRLinearRing *ring, OGRFeatureDefn *layerdef, OGRFeature *feature)
{
	poly.clear();
	if (!ring)
		return;
	for (int ii = 0, ie = ring->getNumPoints(); ii < ie; ++ii) {
		Point pt;
		pt.set_lon_deg_dbl(ring->getX(ii));
		pt.set_lat_deg_dbl(ring->getY(ii));
		poly.push_back(pt);
	}
	poly.remove_redundant_polypoints();
}

#else /* !HAVE_GDAL */

void ParseOGR::loadborder(MultiPolygonHole& poly, OGRGeometry *geom, OGRFeatureDefn *layerdef, OGRFeature *feature)
{
}

void ParseOGR::loadborder(PolygonHole& ph, OGRPolygon *poly, OGRFeatureDefn *layerdef, OGRFeature *feature)
{
}

void ParseOGR::loadborder(PolygonSimple& poly, OGRLinearRing *ring, OGRFeatureDefn *layerdef, OGRFeature *feature)
{
}

#endif

void ParseOGR::addcomposite(void)
{
	for (componentmap_t::const_iterator ci(m_componentmap.begin()), ce(m_componentmap.end()); ci != ce; ++ci) {
		UUID uuid(UUID::from_countryborder(ci->first));
		Airspace::ptr_t p;
		{
			Object::const_ptr_t p1(m_db.load(uuid));
			if (p1) {
				p = Airspace::ptr_t::cast_dynamic(Object::ptr_t::cast_const(p1));
				if (!p) {
					p1->print(std::cerr << "Object ") << " not " << to_str(Airspace::get_static_type()) << ", discarding";
					++m_warncnt;
				}
			}
		}
		AirspaceTimeSlice ts;
		if (p && p->size()) {
			const AirspaceTimeSlice& ts1(p->operator[](0).as_airspace());
			if (ts1.is_valid())
				ts = ts1;
		}
		if (!p) {
			p = Airspace::ptr_t(new Airspace(uuid));
			p->set_modified();
			p->set_dirty();
		}
		ts.set_starttime(0);
		ts.set_endtime(std::numeric_limits<uint64_t>::max());
		if (false) {
			ts.set_ident(ci->first);
		} else {
			std::string name;
			bool subseq(false);
			for (components_t::const_iterator coi(ci->second.begin()), coe(ci->second.end()); coi != coe; ++coi) {
				if (subseq)
					name += "; ";
				subseq = true;
				name += *coi;
			}
			ts.set_ident(name);
		}
		ts.set_type(AirspaceTimeSlice::type_border);
		ts.set_localtype(AirspaceTimeSlice::localtype_none);
		ts.set_icao(false);
		ts.set_flexibleuse(false);
		ts.get_components().clear();
		AirspaceTimeSlice::Component::operator_t op(AirspaceTimeSlice::Component::operator_base);
		for (components_t::const_iterator coi(ci->second.begin()), coe(ci->second.end()); coi != coe; ++coi) {
			ts.get_components().push_back(AirspaceTimeSlice::Component());
			AirspaceTimeSlice::Component& comp(ts.get_components().back());
			comp.set_operator(op);
			op = AirspaceTimeSlice::Component::operator_union;
			comp.set_full_geometry(false);
			comp.set_airspace(UUID::from_countryborder(*coi));
		}
		if (m_verbose) {
			std::cout << "UUID " << p->get_uuid() << " country \"" << ci->first << "\" components";
			for (AirspaceTimeSlice::components_t::const_iterator coi(ts.get_components().begin()), coe(ts.get_components().end()); coi != coe; ++coi) {
				const AirspaceTimeSlice::Component& comp(*coi);
				std::cout << ' ' << comp.get_airspace();
			}
			std::cout << std::endl;
		}
		p->add_timeslice(ts);
		if (p->is_dirty()) {
			m_db.save_temp(p);
			if (m_verbose)
				p->print(std::cout);
		}
	}
}

// initialize tables suitable for
// thematicmapping.org
// TM_WORLD_BORDERS-0.3.shp

void ParseOGR::init_thematicmapping_tables(void)
{
	m_namemap.clear();
	m_componentmap.clear();
	m_namemap["Afghanistan"] = "AFGHANISTAN";
	m_namemap["Albania"] = "ALBANIA";
	m_namemap["Algeria"] = "ALGERIA";
	m_namemap["Andorra"] = "ANDORRA";
	//m_namemap["Angola"] = ;
	//m_namemap["Antarctica"] = ;
	//m_namemap["Argentina"] = ;
	m_namemap["Armenia"] = "ARMENIA";
	m_namemap["Austria"] = "AUSTRIA";
	m_namemap["Azerbaijan"] = "AZERBAIJAN";
	m_namemap["Bahrain"] = "BAHRAIN";
	//m_namemap["Bangladesh"] = ;
	m_namemap["Belarus"] = "BELARUS";
	m_namemap["Belgium"] = "BELGIUM";
	//m_namemap["Belize"] = ;
	m_namemap["Benin"] = "BENIN";
	//m_namemap["Bhutan"] = ;
	//m_namemap["Bolivia"] = ;
	m_namemap["Bosnia and Herzegovina"] = "BOSNIA-HERZ";
	//m_namemap["Botswana"] = ;
	m_namemap["Brazil"] = "BRAZIL";
	//m_namemap["Brunei Darussalam"] = ;
	m_namemap["Bulgaria"] = "BULGARIA";
	m_namemap["Burkina Faso"] = "BURKINA FASO";
	//m_namemap["Burma"] = ;
	m_namemap["Burundi"] = "BURUNDI";
	//m_namemap["Cambodia"] = ;
	//m_namemap["Cameroon"] = ;
	m_namemap["Canada"] = "CANADA";
	m_namemap["Cape Verde"] = "CAP VERDE";
	//m_namemap["Central African Republic"] = ;
	//m_namemap["Chad"] = ;
	//m_namemap["Chile"] = ;
	//m_namemap["Colombia"] = ;
	//m_namemap["Comoros"] = ;
	//m_namemap["Congo"] = ;
	//m_namemap["Costa Rica"] = ;
	m_namemap["Cote d'Ivoire"] = "IVORY COAST";
	m_namemap["Croatia"] = "CROATIA";
	//m_namemap["Cuba"] = ;
	m_namemap["Cyprus"] = "CYPRUS";
	m_namemap["Czech Republic"] = "CZECHIA";
	//m_namemap["Democratic Republic of the Congo"] = ;
	m_namemap["Djibouti"] = "DJIBOUTI";
	//m_namemap["Dominica"] = ;
	//m_namemap["Dominican Republic"] = ;
	//m_namemap["Ecuador"] = ;
	m_namemap["Egypt"] = "EGYPT";
	//m_namemap["El Salvador"] = ;
	//m_namemap["Equatorial Guinea"] = ;
	m_namemap["Eritrea"] = "ERITREA";
	m_namemap["Estonia"] = "ESTONIA";
	m_namemap["Ethiopia"] = "ETHIOPIA";
	//m_namemap["Fiji"] = ;
	//m_namemap["Gabon"] = ;
	m_namemap["Gambia"] = "GAMBIA";
	m_namemap["Georgia"] = "GEORGIA";
	m_namemap["Germany"] = "GERMANY";
	m_namemap["Ghana"] = "GHANA";
	m_namemap["Gibraltar"] = "GIBRALTAR";
	m_namemap["Greece"] = "GREECE";
	m_namemap["Greenland"] = "GREENLAND";
	//m_namemap["Grenada"] = ;
	//m_namemap["Guadeloupe"] = ;
	//m_namemap["Guatemala"] = ;
	m_namemap["Guinea"] = "GUINEA";
	m_namemap["Guinea-Bissau"] = "GUINEA BISSA";
	//m_namemap["Guyana"] = ;
	//m_namemap["Haiti"] = ;
	//m_namemap["Honduras"] = ;
	m_namemap["Hungary"] = "HUNGARY";
	m_namemap["Iceland"] = "ICELAND";
	m_namemap["India"] = "INDIA";
	//m_namemap["Indonesia"] = ;
	m_namemap["Iran (Islamic Republic of)"] = "IRAN";
	m_namemap["Iraq"] = "IRAQ";
	m_namemap["Ireland"] = "IRELAND";
	m_namemap["Israel"] = "ISRAEL";
	m_namemap["Japan"] = "JAPAN";
	m_namemap["Jordan"] = "JORDAN";
	m_namemap["Kazakhstan"] = "KAZAKHSTAN";
	m_namemap["Kenya"] = "KENYA";
	m_namemap["Kiribati"] = "KIRIBATI";
	m_namemap["Korea, Democratic People's Republic of"] = "NORTH-KOREA";
	m_namemap["Korea, Republic of"] = "SOUTH-KOREA";
	m_namemap["Kuwait"] = "KUWAIT";
	m_namemap["Kyrgyzstan"] = "KYRGYZTAN";
	//m_namemap["Lao People's Democratic Republic"] = ;
	m_namemap["Latvia"] = "LATVIA";
	m_namemap["Lebanon"] = "LEBANON";
	m_namemap["Liberia"] = "LIBERIA";
	m_namemap["Libyan Arab Jamahiriya"] = "LIBYA";
	m_namemap["Lithuania"] = "LITHUANIA";
	m_namemap["Luxembourg"] = "LUXEMBOURG";
	//m_namemap["Madagascar"] = ;
	//m_namemap["Malawi"] = ;
	//m_namemap["Malaysia"] = ;
	//m_namemap["Maldives"] = ;
	m_namemap["Mali"] = "MALI";
	m_namemap["Malta"] = "MALTA";
	m_namemap["Marshall Islands"] = "MARSHALL IS";
	m_namemap["Mauritania"] = "MAURITANIA";
	//m_namemap["Mauritius"] = ;
	m_namemap["Mexico"] = "MEXICO";
	m_namemap["Micronesia, Federated States of"] = "MICRONESIA";
	m_namemap["Monaco"] = "MONACO";
	m_namemap["Mongolia"] = "MONGOLIA";
	m_namemap["Montenegro"] = "MONTENEGRO";
	m_namemap["Morocco"] = "MAROCCO";
	//m_namemap["Mozambique"] = ;
	m_namemap["Namibia"] = "NAMIBIA";
	//m_namemap["Nauru"] = ;
	//m_namemap["Nepal"] = ;
	//m_namemap["Nicaragua"] = ;
	m_namemap["Niger"] = "NIGER";
	m_namemap["Nigeria"] = "NIGERIA";
	m_namemap["Oman"] = "OMAN";
	m_namemap["Pakistan"] = "PAKISTAN";
	//m_namemap["Palau"] = ;
	m_namemap["Palestine"] = "PALESTINA";
	//m_namemap["Panama"] = ;
	//m_namemap["Papua New Guinea"] = ;
	//m_namemap["Paraguay"] = ;
	//m_namemap["Peru"] = ;
	m_namemap["Philippines"] = "PHILIPPINES";
	m_namemap["Poland"] = "POLAND";
	m_namemap["Portugal"] = "PORTUGAL";
	m_namemap["Qatar"] = "QATAR";
	m_namemap["Republic of Moldova"] = "MOLDOVA";
	m_namemap["Romania"] = "ROMANIA";
	m_namemap["Russia"] = "RUSSIA";
	m_namemap["Rwanda"] = "RWANDA";
	//m_namemap["Samoa"] = ;
	//m_namemap["Sao Tome and Principe"] = ;
	m_namemap["Saudi Arabia"] = "SAUDI ARABIA";
	m_namemap["Senegal"] = "SENEGAL";
	m_namemap["Serbia"] = "SERBIA";
	//m_namemap["Seychelles"] = ;
	m_namemap["Sierra Leone"] = "SIERRA LEONE";
	//m_namemap["Singapore"] = ;
	m_namemap["Slovakia"] = "SLOVAKIA";
	m_namemap["Slovenia"] = "SLOVENIA";
	m_namemap["Somalia"] = "SOMALIA";
	m_namemap["Spain"] = "SPAIN";
	//m_namemap["Sri Lanka"] = ;
	m_namemap["Sudan"] = "SUDAN";
	//m_namemap["Suriname"] = ;
	m_namemap["Sweden"] = "SWEDEN";
	m_namemap["Syrian Arab Republic"] = "SYRIA";
	//m_namemap["Taiwan"] = ;
	m_namemap["Tajikistan"] = "TAJIKISTAN";
	//m_namemap["Thailand"] = ;
	m_namemap["The former Yugoslav Republic of Macedonia"] = "FYROM";
	//m_namemap["Timor-Leste"] = ;
	m_namemap["Togo"] = "TOGO";
	//m_namemap["Tonga"] = ;
	m_namemap["Trinidad and Tobago"] = "TRINIDAD";
	m_namemap["Tunisia"] = "TUNISIA";
	m_namemap["Turkey"] = "TURKEY";
	m_namemap["Turkmenistan"] = "TURKMENISTAN";
	m_namemap["Uganda"] = "UGANDA";
	m_namemap["Ukraine"] = "UKRAINE";
	m_namemap["United Arab Emirates"] = "UA EMIRATES";
	m_namemap["United Republic of Tanzania"] = "TANZANIA";
	//m_namemap["Uruguay"] = ;
	m_namemap["Uzbekistan"] = "UZBEKISTAN";
	//m_namemap["Vanuatu"] = ;
	//m_namemap["Venezuela"] = ;
	m_namemap["Viet Nam"] = "VIETNAM";
	m_namemap["Western Sahara"] = "WEST SAHARA";
	m_namemap["Yemen"] = "YEMEN";
	//m_namemap["Zambia"] = ;
	//m_namemap["Zimbabwe"] = ;

	m_componentmap["USA"].insert("United States");
	m_componentmap["USA"].insert("United States Minor Outlying Islands");
	m_componentmap["USA"].insert("United States Virgin Islands");
	m_componentmap["USA"].insert("American Samoa");
	m_componentmap["USA"].insert("Guam");
	m_componentmap["USA"].insert("Northern Mariana Islands");
	m_componentmap["USA"].insert("Puerto Rico");

	m_componentmap["UK"].insert("United Kingdom");
	// Crown Dependencies
	m_componentmap["UK"].insert("Guernsey");
	m_componentmap["UK"].insert("Isle of Man");
	m_componentmap["UK"].insert("Jersey");
	// Overseas Territories
	m_componentmap["UK"].insert("Anguilla");
	m_componentmap["UK"].insert("Cayman Islands");
	m_componentmap["UK"].insert("Montserrat");
	m_componentmap["UK"].insert("Pitcairn Islands");
	m_componentmap["UK"].insert("Turks and Caicos Islands");
	m_componentmap["UK"].insert("Saint Helena");
	m_componentmap["UK"].insert("South Georgia South Sandwich Islands");
	m_componentmap["UK"].insert("Bermuda");
	m_componentmap["UK"].insert("British Indian Ocean Territory");
	m_componentmap["UK"].insert("British Virgin Islands");
	// slightly independent
	m_componentmap["UK"].insert("Antigua and Barbuda");
	m_componentmap["UK"].insert("Bahamas");
	m_componentmap["UK"].insert("Barbados");
	m_componentmap["UK"].insert("Falkland Islands (Malvinas)");
	m_componentmap["UK"].insert("Jamaica");
	m_componentmap["UK"].insert("Saint Kitts and Nevis");
	m_componentmap["UK"].insert("Saint Lucia");
	m_componentmap["UK"].insert("Saint Vincent and the Grenadines");
	m_componentmap["UK"].insert("Solomon Islands");
	m_componentmap["UK"].insert("Tuvalu");

	m_componentmap["SOUTH AFRICA"].insert("Lesotho");
	m_componentmap["SOUTH AFRICA"].insert("South Africa");
	m_componentmap["SOUTH AFRICA"].insert("Swaziland");

	// Hack
	m_componentmap["SPANISH TER"].insert("SPAIN");

	m_componentmap["FINLAND"].insert("Finland");
	m_componentmap["FINLAND"].insert("\xc3\x85land Islands");

	m_componentmap["FRANCE"].insert("France");
	m_componentmap["FRANCE"].insert("French Guiana");
	m_componentmap["FRANCE"].insert("French Polynesia");
	m_componentmap["FRANCE"].insert("French Southern and Antarctic Lands");
	m_componentmap["FRANCE"].insert("Martinique");
	m_componentmap["FRANCE"].insert("Mayotte");
	m_componentmap["FRANCE"].insert("New Caledonia");
	m_componentmap["FRANCE"].insert("Saint Barthelemy");
	m_componentmap["FRANCE"].insert("Saint Martin");
	m_componentmap["FRANCE"].insert("Saint Pierre and Miquelon");
	m_componentmap["FRANCE"].insert("Wallis and Futuna Islands");
	m_componentmap["FRANCE"].insert("Reunion");

	m_componentmap["SWITZERLAND"].insert("Liechtenstein");
	m_componentmap["SWITZERLAND"].insert("Switzerland");

	m_componentmap["NEW ZEALAND"].insert("New Zealand");
	m_componentmap["NEW ZEALAND"].insert("Cook Islands");
	m_componentmap["NEW ZEALAND"].insert("Niue");
	m_componentmap["NEW ZEALAND"].insert("Tokelau");

	m_componentmap["NORWAY"].insert("Norway");
	m_componentmap["NORWAY"].insert("Bouvet Island");
	m_componentmap["NORWAY"].insert("Svalbard");

	m_componentmap["AUSTRALIA"].insert("Australia");
	m_componentmap["AUSTRALIA"].insert("Christmas Island");
	m_componentmap["AUSTRALIA"].insert("Cocos (Keeling) Islands");
	m_componentmap["AUSTRALIA"].insert("Norfolk Island");
	m_componentmap["AUSTRALIA"].insert("Heard Island and McDonald Islands");

	m_componentmap["CHINA"].insert("China");
	m_componentmap["CHINA"].insert("Hong Kong");
	m_componentmap["CHINA"].insert("Macau");

	m_componentmap["DENMARK"].insert("Denmark");
	m_componentmap["DENMARK"].insert("Faroe Islands");

	m_componentmap["NETHERLANDS"].insert("Netherlands");
	m_componentmap["NETHERLANDS"].insert("Netherlands Antilles");
	m_componentmap["NETHERLANDS"].insert("Aruba");

	m_componentmap["ITALY"].insert("Italy");
	m_componentmap["ITALY"].insert("Holy See (Vatican City)");
	m_componentmap["ITALY"].insert("San Marino");

	m_componentmap["EU"].insert("AUSTRIA");
	m_componentmap["EU"].insert("BELGIUM");
	m_componentmap["EU"].insert("BULGARIA");
	m_componentmap["EU"].insert("CROATIA");
	m_componentmap["EU"].insert("CYPRUS");
	m_componentmap["EU"].insert("CZECHIA");
	m_componentmap["EU"].insert("Denmark");
	m_componentmap["EU"].insert("Faroe Islands");
	m_componentmap["EU"].insert("ESTONIA");
	m_componentmap["EU"].insert("Finland");
	m_componentmap["EU"].insert("\xc3\x85land Islands");
	m_componentmap["EU"].insert("France");
	m_componentmap["EU"].insert("French Guiana");
	m_componentmap["EU"].insert("French Polynesia");
	m_componentmap["EU"].insert("French Southern and Antarctic Lands");
	m_componentmap["EU"].insert("Martinique");
	m_componentmap["EU"].insert("Mayotte");
	m_componentmap["EU"].insert("New Caledonia");
	m_componentmap["EU"].insert("Saint Barthelemy");
	m_componentmap["EU"].insert("Saint Martin");
	m_componentmap["EU"].insert("Saint Pierre and Miquelon");
	m_componentmap["EU"].insert("Wallis and Futuna Islands");
	m_componentmap["EU"].insert("Reunion");
	m_componentmap["EU"].insert("GERMANY");
	m_componentmap["EU"].insert("GREECE");
	m_componentmap["EU"].insert("HUNGARY");
	m_componentmap["EU"].insert("IRELAND");
	m_componentmap["EU"].insert("Italy");
	m_componentmap["EU"].insert("Holy See (Vatican City)");
	m_componentmap["EU"].insert("San Marino");
	m_componentmap["EU"].insert("LATVIA");
	m_componentmap["EU"].insert("LITHUANIA");
	m_componentmap["EU"].insert("LUXEMBOURG");
	m_componentmap["EU"].insert("MALTA");
	m_componentmap["EU"].insert("Netherlands");
	m_componentmap["EU"].insert("Netherlands Antilles");
	m_componentmap["EU"].insert("Aruba");
	m_componentmap["EU"].insert("POLAND");
	m_componentmap["EU"].insert("PORTUGAL");
	m_componentmap["EU"].insert("ROMANIA");
	m_componentmap["EU"].insert("SLOVAKIA");
	m_componentmap["EU"].insert("SLOVENIA");
	m_componentmap["EU"].insert("SPAIN");
	m_componentmap["EU"].insert("SWEDEN");
	m_componentmap["EU"].insert("United Kingdom");
	m_componentmap["EU"].insert("Guernsey");
	m_componentmap["EU"].insert("Isle of Man");
	m_componentmap["EU"].insert("Jersey");

	m_componentmap["F REGION"] = components_t();
        m_componentmap["G REGION"] = components_t();
        m_componentmap["M REGION"] = components_t();
        m_componentmap["N REGION"] = components_t();
        m_componentmap["O REGION"] = components_t();
        m_componentmap["S REGION"] = components_t();
        m_componentmap["T REGION"] = components_t();
        m_componentmap["UT REGION"] = components_t();
        m_componentmap["V REGION"] = components_t();
        m_componentmap["XX"] = components_t();
        m_componentmap["Z+R REGION"] = components_t();
}
