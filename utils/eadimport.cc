//
// C++ Implementation: eadimport
//
// Description: Database XML representation to sqlite db conversion
//
//
// Author: Thomas Sailer <t.sailer@alumni.ethz.ch>, (C) 2007, 2009, 2012, 2013
//
// Copyright: See COPYING file that comes with this distribution
//
//

#include "sysdeps.h"

#include <libxml++/libxml++.h>
#include <sigc++/sigc++.h>
#include <unistd.h>
#include <getopt.h>
#include <iostream>
#include <sstream>
#include <limits>
#include <stdlib.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <errno.h>
#include <stdexcept>
#include <stdarg.h>
#include <cassert>
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

#include "dbobj.h"

class DbXmlImporter : public xmlpp::SaxParser {
public:
	DbXmlImporter(const Glib::ustring& output_dir, bool supersedefullawy = true, const Glib::ustring& borderfile = "");
	virtual ~DbXmlImporter();

	void set_airway_type(AirwaysDb::Airway::airway_type_t at) { m_awytype = at; }
	void set_dme(void) { m_tacan = false; }
	void set_tacan(void) { m_tacan = true; }

	void finalize_airways(void);
	void finalize_airspaces(void);
	void update_authority(void);

protected:
	virtual void on_start_document();
	virtual void on_end_document();
	virtual void on_start_element(const Glib::ustring& name,
				      const AttributeList& properties);
	virtual void on_end_element(const Glib::ustring& name);
	virtual void on_characters(const Glib::ustring& characters);
	virtual void on_comment(const Glib::ustring& text);
	virtual void on_warning(const Glib::ustring& text);
	virtual void on_error(const Glib::ustring& text);
	virtual void on_fatal_error(const Glib::ustring& text);

	void open_airports_db(void);
	void open_airspaces_db(void);
	void open_navaids_db(void);
	void open_waypoints_db(void);
	void open_airways_db(void);
	void open_topo_db(void);

	void begin_record(void);
	void end_record(void);
	void record_error(const Glib::ustring& msg);
	uint32_t get_frequency(void) const;
	uint32_t get_channel_frequency(void) const;
	static NavaidsDb::Navaid::navaid_type_t navaid_type(const Glib::ustring& ct);
	void process_intersection(void);
	void process_ndb(void);
	void process_vor(void);
	void process_dme(void);
	void process_airway(void);
	void process_airspace(void);
	void process_airspace_vertex(void);

private:
	typedef enum {
		state_document_c,
		state_sdoresponse_c,
		state_sdoreportresult_c,
		state_reportsettings_c,
		state_sdoreporteffectivedate_c,
		state_sdoreportstyle_c,
		state_timeformat_c,
		state_geoformat_c,
		state_record_c,
		state_mid_c,
		state_codetype_c,
		state_codeid_c,
		state_txtname_c,
		state_txtrmk_c,
		state_geolat_c,
		state_geolon_c,
		state_codedatum_c,
		state_dtwef_c,
		state_orgcre_c,
		state_orgcre_txtname_c,
		state_valfreq_c,
		state_uomfreq_c,
		state_codetypenorth_c,
		state_vor_c,
		state_vor_codeid_c,
		state_vor_geolat_c,
		state_vor_geolon_c,
		state_codechannel_c,
		state_codeclass_c,
		state_codeworkhr_c,
		state_rte_c,
		state_rte_txtdesig_c,
		state_rte_txtlocdesig_c,
		state_spnsta_c,
		state_spnsta_codetype_c,
		state_spnsta_codeid_c,
		state_spnend_c,
		state_spnend_codetype_c,
		state_spnend_codeid_c,
		state_org_c,
		state_org_txtname_c,
		state_valghostfreq_c,
		state_uomghostfreq_c,
		state_codelocind_c,
		state_codedistverupper_c,
		state_valdistverupper_c,
		state_uomdistverupper_c,
		state_codedistverlower_c,
		state_valdistverlower_c,
		state_uomdistverlower_c,
		state_geolatarc_c,
		state_geolonarc_c,
		state_valradiusarc_c,
		state_uomradiusarc_c,
		state_valcrc_c,
		state_valgeoaccuracy_c,
		state_uomgeoaccuracy_c,
		state_ase_c,
		state_ase_codetype_c,
		state_ase_codeid_c,
		state_gbr_c,
		state_gbr_txtname_c
	} state_t;
	state_t m_state;

	Glib::ustring m_outputdir;
	bool m_supersedefullawy;
	bool m_purgedb;
	bool m_airportsdbopen;
	AirportsDb m_airportsdb;
	bool m_airspacesdbopen;
	AirspacesDb m_airspacesdb;
	bool m_navaidsdbopen;
	NavaidsDb m_navaidsdb;
	bool m_waypointsdbopen;
	WaypointsDb m_waypointsdb;
	bool m_airwaysdbopen;
	AirwaysDb m_airwaysdb;
	bool m_topodbopen;
	TopoDb30 m_topodb;
	AirspacesDb::Airspace m_airspace;
	bool m_finalizeairspaces;
	Glib::ustring m_bordersfile;
	time_t m_starttime;
	AirwaysDb::Airway::airway_type_t m_awytype;
	bool m_tacan;
	bool m_intsdelete;

	Glib::ustring m_rec_mid;
	Glib::ustring m_rec_codetype;
	Glib::ustring m_rec_codeid;
	Glib::ustring m_rec_codetypenorth;
	Glib::ustring m_rec_codechannel;
	Glib::ustring m_rec_codeclass;
	Glib::ustring m_rec_codeworkhr;
	Glib::ustring m_rec_codedatum;
	Glib::ustring m_rec_geolat;
	Glib::ustring m_rec_geolon;
	Glib::ustring m_rec_txtname;
	Glib::ustring m_rec_txtrmk;
	Glib::ustring m_rec_valfreq;
	Glib::ustring m_rec_uomfreq;
	Glib::ustring m_rec_valghostfreq;
	Glib::ustring m_rec_uomghostfreq;
	Glib::ustring m_rec_dtwef;
	Glib::ustring m_rec_codelocind;
	Glib::ustring m_rec_codedistverupper;
	Glib::ustring m_rec_valdistverupper;
	Glib::ustring m_rec_uomdistverupper;
	Glib::ustring m_rec_codedistverlower;
	Glib::ustring m_rec_valdistverlower;
	Glib::ustring m_rec_uomdistverlower;
	Glib::ustring m_rec_geolatarc;
	Glib::ustring m_rec_geolonarc;
	Glib::ustring m_rec_valradiusarc;
	Glib::ustring m_rec_uomradiusarc;
	Glib::ustring m_rec_valcrc;
	Glib::ustring m_rec_valgeoaccuracy;
	Glib::ustring m_rec_uomgeoaccuracy;
	Glib::ustring m_rec_ase_codetype;
	Glib::ustring m_rec_ase_codeid;
	Glib::ustring m_rec_gbr_txtname;
	Glib::ustring m_rec_org_txtname;
	Glib::ustring m_rec_orgcre_txtname;
	Glib::ustring m_rec_vor_codeid;
	Glib::ustring m_rec_vor_geolat;
	Glib::ustring m_rec_vor_geolon;
	Glib::ustring m_rec_rte_txtdesig;
	Glib::ustring m_rec_rte_txtlocdesig;
	Glib::ustring m_rec_spnsta_codetype;
	Glib::ustring m_rec_spnsta_codeid;
	Glib::ustring m_rec_spnend_codetype;
	Glib::ustring m_rec_spnend_codeid;

	class AirwayCompare : public std::less<AirwaysDb::element_t> {
	public:
		bool operator()(const AirwaysDb::element_t& x, const AirwaysDb::element_t& y);
	};

	typedef std::set<AirwaysDb::element_t, AirwayCompare> airway_t;
	airway_t m_airway;

	class ForEachAirspace : public AirspacesDb::ForEach {
	public:
		ForEachAirspace(time_t starttime);
		~ForEachAirspace();
		bool operator()(const AirspacesDb::Airspace& e);
		bool operator()(const std::string& id);

		void loadborders(const Glib::ustring& bordersfile);

		void process(AirspacesDb& airspacesdb);

	protected:
		typedef std::vector<PolygonSimple> polygons_t;
		typedef std::map<std::string,polygons_t> countries_t;
		countries_t m_countries;
		typedef std::map<std::string,std::string> countrymap_t;
		countrymap_t m_countrymap;
		std::stack<AirspacesDb::Address> m_records;
		time_t m_starttime;

		typedef enum {
			result_nochange,
			result_save,
			result_erase
		} result_t;

		result_t process_one(AirspacesDb::Airspace& e);

#ifdef HAVE_GDAL
		void loadborder(const std::string& country, OGRGeometry *geom, OGRFeatureDefn *layerdef = 0, OGRFeature *feature = 0);
		void loadborder(const std::string& country, OGRPolygon *poly, OGRFeatureDefn *layerdef = 0, OGRFeature *feature = 0);
		void loadborder(const std::string& country, OGRLinearRing *ring, OGRFeatureDefn *layerdef = 0, OGRFeature *feature = 0);
#endif

		class FindNearest {
		public:
			FindNearest(const Point& p1, const Point& p2);
			void process(const polygons_t& polys);
			void process(const PolygonSimple& poly);

			const PolygonSimple& get_poly(void) const { return m_poly; }
			const Point& get_p1(void) const { return m_p1; }
			const Point& get_p2(void) const { return m_p2; }
			uint64_t get_dist(void) const { return m_dist; }
			unsigned int get_idx1(void) const { return m_idx1; }
			unsigned int get_idx2(void) const { return m_idx2; }

		protected:
			PolygonSimple m_poly;
			Point m_p1;
			Point m_p2;
			uint64_t m_dist;
			unsigned int m_idx1;
			unsigned int m_idx2;
		};
	};

	class RecomputeAuthority : public AirspacesDb::ForEach {
	public:
		RecomputeAuthority(const Glib::ustring& output_dir);
		~RecomputeAuthority();
		bool operator()(const AirspacesDb::Airspace& e);
		bool operator()(const std::string& id);

	protected:
		AirportsDb m_airportsdb;
		NavaidsDb m_navaidsdb;
		WaypointsDb m_waypointsdb;
	};

	void finalize_airway(airway_t::const_iterator awb, airway_t::const_iterator awe);
};

bool DbXmlImporter::AirwayCompare::operator()(const AirwaysDb::element_t& x, const AirwaysDb::element_t& y)
{
	int c(x.get_name().compare(y.get_name()));
	if (c < 0)
		return true;
	if (c > 0)
		return false;
	c = x.get_begin_name().compare(y.get_begin_name());
	if (c < 0)
		return true;
	if (c > 0)
		return false;
	c = x.get_end_name().compare(y.get_end_name());
	if (c < 0)
		return true;
	if (c > 0)
		return false;
	if (x.get_begin_coord().get_lat() < y.get_begin_coord().get_lat())
		return true;
	if (y.get_begin_coord().get_lat() < x.get_begin_coord().get_lat())
		return false;
	if (x.get_begin_coord().get_lon() < y.get_begin_coord().get_lon())
		return true;
	if (y.get_begin_coord().get_lon() < x.get_begin_coord().get_lon())
		return false;
	if (x.get_end_coord().get_lat() < y.get_end_coord().get_lat())
		return true;
	if (y.get_end_coord().get_lat() < x.get_end_coord().get_lat())
		return false;
	if (x.get_end_coord().get_lon() < y.get_end_coord().get_lon())
		return true;
	if (y.get_end_coord().get_lon() < x.get_end_coord().get_lon())
		return false;
	return false;
}

DbXmlImporter::DbXmlImporter(const Glib::ustring & output_dir, bool supersedefullawy, const Glib::ustring& borderfile)
        : xmlpp::SaxParser(false), m_state(state_document_c), m_outputdir(output_dir), m_supersedefullawy(supersedefullawy), m_purgedb(false),
	  m_airportsdbopen(false), m_airspacesdbopen(false), m_navaidsdbopen(false), m_waypointsdbopen(false), m_airwaysdbopen(false),
	  m_topodbopen(false), m_finalizeairspaces(false), m_bordersfile(borderfile), m_starttime(0),
	  m_awytype(AirwaysDb::Airway::airway_user), m_tacan(false), m_intsdelete(true)
{
	time(&m_starttime);
}

DbXmlImporter::~DbXmlImporter()
{
	finalize_airways();

}

void DbXmlImporter::on_start_document()
{
        m_state = state_document_c;
}

void DbXmlImporter::on_end_document()
{
	if (m_airspace.is_valid() && m_airspacesdbopen)
		m_airspacesdb.save(m_airspace);
	m_airspace = AirspacesDb::Airspace();
        if (m_airportsdbopen)
                m_airportsdb.close();
        m_airportsdbopen = false;
        if (m_airspacesdbopen)
                m_airspacesdb.close();
        m_airspacesdbopen = false;
        if (m_navaidsdbopen)
                m_navaidsdb.close();
        m_navaidsdbopen = false;
        if (m_waypointsdbopen)
                m_waypointsdb.close();
        m_waypointsdbopen = false;
        if (m_airwaysdbopen)
                m_airwaysdb.close();
        m_airwaysdbopen = false;
        if (m_state != state_document_c)
                throw std::runtime_error("XML Parser: end document and not document state");
}

void DbXmlImporter::on_start_element(const Glib::ustring& name, const AttributeList& properties)
{
	if (false)
		std::cerr << "on_start_element: " << name << " state " << m_state << std::endl;
        switch (m_state) {
	case state_document_c:
		if (name == "SdoReportResponse") {
			m_state = state_sdoresponse_c;
			return;
		}
		break;

	case state_sdoresponse_c:
		if (name == "SdoReportResult") {
			m_state = state_sdoreportresult_c;
			return;
		}
		break;

	case state_sdoreportresult_c:
		if (name == "ReportSettings") {
			m_state = state_reportsettings_c;
			return;
		}
		if (name == "Record") {
			begin_record();
			m_state = state_record_c;
			return;
		}
		break;

	case state_reportsettings_c:
		if (name == "SdoReportEffectiveDate") {
			m_state = state_sdoreporteffectivedate_c;
			return;
		}
		if (name == "SdoReportStyle") {
			m_state = state_sdoreportstyle_c;
			return;
		}
		break;

	case state_sdoreportstyle_c:
		if (name == "TimeFormat") {
			m_state = state_timeformat_c;
			return;
		}
		if (name == "GeoFormat") {
			m_state = state_geoformat_c;
			return;
		}
		break;

	case state_record_c:
		if (name == "mid") {
			m_state = state_mid_c;
			return;
		}
		if (name == "codeType") {
			m_state = state_codetype_c;
			return;
		}
		if (name == "codeId") {
			m_state = state_codeid_c;
			return;
		}
		if (name == "txtName") {
			m_state = state_txtname_c;
			return;
		}
		if (name == "txtRmk") {
			m_state = state_txtrmk_c;
			return;
		}
		if (name == "geoLat") {
			m_state = state_geolat_c;
			return;
		}
		if (name == "geoLong") {
			m_state = state_geolon_c;
			return;
		}
		if (name == "codeDatum") {
			m_state = state_codedatum_c;
			return;
		}
		if (name == "dtWef") {
			m_state = state_dtwef_c;
			return;
		}
		if (name == "OrgCre") {
			m_state = state_orgcre_c;
			return;
		}
		if (name == "valFreq") {
			m_state = state_valfreq_c;
			return;
		}
		if (name == "uomFreq") {
			m_state = state_uomfreq_c;
			return;
		}
		if (name == "codeTypeNorth") {
			m_state = state_codetypenorth_c;
			return;
		}
		if (name == "Vor") {
			m_state = state_vor_c;
			return;
		}
		if (name == "codeChannel") {
			m_state = state_codechannel_c;
			return;
		}
		if (name == "codeClass") {
			m_state = state_codeclass_c;
			return;
		}
		if (name == "codeWorkHr") {
			m_state = state_codeworkhr_c;
			return;
		}
		if (name == "Rte") {
			m_state = state_rte_c;
			return;
		}
		if (name == "SpnSta") {
			m_state = state_spnsta_c;
			return;
		}
		if (name == "SpnEnd") {
			m_state = state_spnend_c;
			return;
		}
		if (name == "Org") {
			m_state = state_org_c;
			return;
		}
		if (name == "valGhostFreq") {
			m_state = state_valghostfreq_c;
			return;
		}
		if (name == "uomGhostFreq") {
			m_state = state_uomghostfreq_c;
			return;
		}
		if (name == "codeLocInd") {
			m_state = state_codelocind_c;
			return;
		}
		if (name == "codeDistVerUpper") {
			m_state = state_codedistverupper_c;
			return;
		}
		if (name == "valDistVerUpper") {
			m_state = state_valdistverupper_c;
			return;
		}
		if (name == "uomDistVerUpper") {
			m_state = state_uomdistverupper_c;
			return;
		}
		if (name == "codeDistVerLower") {
			m_state = state_codedistverlower_c;
			return;
		}
		if (name == "valDistVerLower") {
			m_state = state_valdistverlower_c;
			return;
		}
		if (name == "uomDistVerLower") {
			m_state = state_uomdistverlower_c;
			return;
		}
		if (name == "geoLatArc") {
			m_state = state_geolatarc_c;
			return;
		}
		if (name == "geoLongArc") {
			m_state = state_geolonarc_c;
			return;
		}
		if (name == "valRadiusArc") {
			m_state = state_valradiusarc_c;
			return;
		}
		if (name == "uomRadiusArc") {
			m_state = state_uomradiusarc_c;
			return;
		}
		if (name == "valCrc") {
			m_state = state_valcrc_c;
			return;
		}
		if (name == "valGeoAccuracy") {
			m_state = state_valgeoaccuracy_c;
			return;
		}
		if (name == "uomGeoAccuracy") {
			m_state = state_uomgeoaccuracy_c;
			return;
		}
		if (name == "Ase") {
			m_state = state_ase_c;
			return;
		}
		if (name == "Gbr") {
			m_state = state_gbr_c;
			return;
		}
		break;

	case state_orgcre_c:
		if (name == "txtName") {
			m_state = state_orgcre_txtname_c;
			return;
		}
		break;

	case state_vor_c:
		if (name == "codeId") {
			m_state = state_vor_codeid_c;
			return;
		}
		if (name == "geoLat") {
			m_state = state_vor_geolat_c;
			return;
		}
		if (name == "geoLong") {
			m_state = state_vor_geolon_c;
			return;
		}
		break;

	case state_rte_c:
		if (name == "txtDesig") {
			m_state = state_rte_txtdesig_c;
			return;
		}
		if (name == "txtLocDesig") {
			m_state = state_rte_txtlocdesig_c;
			return;
		}
		break;

	case state_spnsta_c:
		if (name == "codeType") {
			m_state = state_spnsta_codetype_c;
			return;
		}
		if (name == "codeId") {
			m_state = state_spnsta_codeid_c;
			return;
		}
		break;

	case state_spnend_c:
		if (name == "codeType") {
			m_state = state_spnend_codetype_c;
			return;
		}
		if (name == "codeId") {
			m_state = state_spnend_codeid_c;
			return;
		}
		break;

	case state_org_c:
		if (name == "txtName") {
			m_state = state_org_txtname_c;
			return;
		}
		break;

	case state_ase_c:
		if (name == "codeType") {
			m_state = state_ase_codetype_c;
			return;
		}
		if (name == "codeId") {
			m_state = state_ase_codeid_c;
			return;
		}
		break;

	case state_gbr_c:
		if (name == "txtName") {
			m_state = state_gbr_txtname_c;
			return;
		}
		break;

	default:
		break;
        }
        std::ostringstream oss;
        oss << "XML Parser: Invalid element " << name << " in state " << (int)m_state;
        throw std::runtime_error(oss.str());
}

void DbXmlImporter::on_end_element(const Glib::ustring& name)
{
	if (false)
		std::cerr << "on_end_element: " << name << " state " << m_state << std::endl;
        switch (m_state) {
	case state_sdoresponse_c:
		m_state = state_document_c;
		return;

	case state_sdoreportresult_c:
		m_state = state_sdoresponse_c;
		return;

	case state_reportsettings_c:
		m_state = state_sdoreportresult_c;
		return;

	case state_sdoreporteffectivedate_c:
		m_state = state_reportsettings_c;
		return;

	case state_sdoreportstyle_c:
		m_state = state_reportsettings_c;
		return;

	case state_timeformat_c:
		m_state = state_sdoreportstyle_c;
		return;

	case state_geoformat_c:
		m_state = state_sdoreportstyle_c;
		return;

	case state_record_c:
		end_record();
		m_state = state_sdoreportresult_c;
		return;

	case state_mid_c:
		m_state = state_record_c;
		return;

	case state_codetype_c:
		m_state = state_record_c;
		return;

	case state_codeid_c:
		m_state = state_record_c;
		return;

	case state_txtname_c:
		m_state = state_record_c;
		return;

	case state_txtrmk_c:
		m_state = state_record_c;
		return;

	case state_geolat_c:
		m_state = state_record_c;
		return;

	case state_geolon_c:
		m_state = state_record_c;
		return;

	case state_codedatum_c:
		m_state = state_record_c;
		return;

	case state_dtwef_c:
		m_state = state_record_c;
		return;

	case state_orgcre_c:
		m_state = state_record_c;
		return;

	case state_valfreq_c:
		m_state = state_record_c;
		return;

	case state_uomfreq_c:
		m_state = state_record_c;
		return;

	case state_codetypenorth_c:
		m_state = state_record_c;
		return;

	case state_vor_c:
		m_state = state_record_c;
		return;

	case state_codechannel_c:
		m_state = state_record_c;
		return;

	case state_codeclass_c:
		m_state = state_record_c;
		return;

	case state_codeworkhr_c:
		m_state = state_record_c;
		return;

	case state_rte_c:
		m_state = state_record_c;
		return;

	case state_spnsta_c:
		m_state = state_record_c;
		return;

	case state_spnend_c:
		m_state = state_record_c;
		return;

	case state_orgcre_txtname_c:
		m_state = state_orgcre_c;
		return;

	case state_vor_codeid_c:
		m_state = state_vor_c;
		return;

	case state_vor_geolat_c:
		m_state = state_vor_c;
		return;

	case state_vor_geolon_c:
		m_state = state_vor_c;
		return;

	case state_rte_txtdesig_c:
		m_state = state_rte_c;
		return;

	case state_rte_txtlocdesig_c:
		m_state = state_rte_c;
		return;

	case state_spnsta_codetype_c:
		m_state = state_spnsta_c;
		return;

	case state_spnsta_codeid_c:
		m_state = state_spnsta_c;
		return;

	case state_spnend_codetype_c:
		m_state = state_spnend_c;
		return;

	case state_spnend_codeid_c:
		m_state = state_spnend_c;
		return;

	case state_org_c:
		m_state = state_record_c;
		return;

	case state_org_txtname_c:
		m_state = state_org_c;
		return;

	case state_valghostfreq_c:
		m_state = state_record_c;
		return;

	case state_uomghostfreq_c:
		m_state = state_record_c;
		return;

	case state_codelocind_c:
		m_state = state_record_c;
		return;

	case state_codedistverupper_c:
		m_state = state_record_c;
		return;

	case state_valdistverupper_c:
		m_state = state_record_c;
		return;

	case state_uomdistverupper_c:
		m_state = state_record_c;
		return;

	case state_codedistverlower_c:
		m_state = state_record_c;
		return;

	case state_valdistverlower_c:
		m_state = state_record_c;
		return;

	case state_uomdistverlower_c:
		m_state = state_record_c;
		return;

	case state_geolatarc_c:
		m_state = state_record_c;
		return;

	case state_geolonarc_c:
		m_state = state_record_c;
		return;

	case state_valradiusarc_c:
		m_state = state_record_c;
		return;

	case state_uomradiusarc_c:
		m_state = state_record_c;
		return;

	case state_valcrc_c:
		m_state = state_record_c;
		return;

	case state_valgeoaccuracy_c:
		m_state = state_record_c;
		return;

	case state_uomgeoaccuracy_c:
		m_state = state_record_c;
		return;

	case state_ase_c:
		m_state = state_record_c;
		return;

	case state_ase_codetype_c:
		m_state = state_ase_c;
		return;

	case state_ase_codeid_c:
		m_state = state_ase_c;
		return;

	case state_gbr_c:
		m_state = state_record_c;
		return;

	case state_gbr_txtname_c:
		m_state = state_gbr_c;
		return;

	default:
		break;
        }
        std::ostringstream oss;
        oss << "XML Parser: Invalid end element in state " << (int)m_state;
        throw std::runtime_error(oss.str());
}

void DbXmlImporter::on_characters(const Glib::ustring& characters)
{
       switch (m_state) {
       case state_mid_c:
	       m_rec_mid += characters;
	       break;

       case state_codetype_c:
	       m_rec_codetype += characters;
	       break;

       case state_codeid_c:
	       m_rec_codeid += characters;
	       break;

       case state_txtname_c:
	       m_rec_txtname += characters;
	       break;

       case state_txtrmk_c:
	       m_rec_txtrmk += characters;
	       break;

       case state_geolat_c:
	       m_rec_geolat += characters;
	       break;

       case state_geolon_c:
	       m_rec_geolon += characters;
	       break;

       case state_codedatum_c:
	       m_rec_codedatum += characters;
	       break;

       case state_dtwef_c:
	       m_rec_dtwef += characters;
	       break;

       case state_org_txtname_c:
	       m_rec_org_txtname += characters;
	       break;

       case state_orgcre_txtname_c:
	       m_rec_orgcre_txtname += characters;
	       break;

       case state_valfreq_c:
	       m_rec_valfreq += characters;
	       break;

       case state_uomfreq_c:
	       m_rec_uomfreq += characters;
	       break;

       case state_valghostfreq_c:
	       m_rec_valghostfreq += characters;
	       break;

       case state_uomghostfreq_c:
	       m_rec_uomghostfreq += characters;
	       break;

       case state_codetypenorth_c:
	       m_rec_codetypenorth += characters;
	       break;

       case state_vor_codeid_c:
	       m_rec_vor_codeid += characters;
	       break;

       case state_vor_geolat_c:
	       m_rec_vor_geolat += characters;
	       break;

       case state_vor_geolon_c:
	       m_rec_vor_geolon += characters;
	       break;

       case state_codechannel_c:
	       m_rec_codechannel += characters;
	       break;

       case state_codeclass_c:
	       m_rec_codeclass += characters;
	       break;

       case state_codeworkhr_c:
	       m_rec_codeworkhr += characters;
	       break;

       case state_rte_txtdesig_c:
	       m_rec_rte_txtdesig += characters;
	       break;

       case state_rte_txtlocdesig_c:
	       m_rec_rte_txtlocdesig += characters;
	       break;

       case state_spnsta_codetype_c:
	       m_rec_spnsta_codetype += characters;
	       break;

       case state_spnsta_codeid_c:
	       m_rec_spnsta_codeid += characters;
	       break;

       case state_spnend_codetype_c:
	       m_rec_spnend_codetype += characters;
	       break;

       case state_spnend_codeid_c:
	       m_rec_spnend_codeid += characters;
	       break;

       case state_codelocind_c:
	       m_rec_codelocind += characters;
	       break;

       case state_codedistverupper_c:
	       m_rec_codedistverupper += characters;
	       break;

       case state_valdistverupper_c:
	       m_rec_valdistverupper += characters;
	       break;

       case state_uomdistverupper_c:
	       m_rec_uomdistverupper += characters;
	       break;

       case state_codedistverlower_c:
	       m_rec_codedistverlower += characters;
	       break;

       case state_valdistverlower_c:
	       m_rec_valdistverlower += characters;
	       break;

       case state_uomdistverlower_c:
	       m_rec_uomdistverlower += characters;
	       break;

       case state_geolatarc_c:
	       m_rec_geolatarc += characters;
	       break;

       case state_geolonarc_c:
	       m_rec_geolonarc += characters;
	       break;

       case state_valradiusarc_c:
	       m_rec_valradiusarc += characters;
	       break;

       case state_uomradiusarc_c:
	       m_rec_uomradiusarc += characters;
	       break;

       case state_valcrc_c:
	       m_rec_valcrc += characters;
	       break;

       case state_valgeoaccuracy_c:
	       m_rec_valcrc += m_rec_valgeoaccuracy;
	       break;

       case state_uomgeoaccuracy_c:
	       m_rec_valcrc += m_rec_uomgeoaccuracy;
	       break;

       case state_ase_codetype_c:
	       m_rec_ase_codetype += characters;
	       break;

       case state_ase_codeid_c:
	       m_rec_ase_codeid += characters;
	       break;

       case state_gbr_txtname_c:
	       m_rec_gbr_txtname += characters;
	       break;

       default:
	       std::cerr << "XML Parser: unknown text \"" << characters << "\" in state " << m_state << std::endl;
       }
}

void DbXmlImporter::on_comment(const Glib::ustring& text)
{
}

void DbXmlImporter::on_warning(const Glib::ustring& text)
{
        std::cerr << "XML Parser: warning: " << text << std::endl;
        //throw std::runtime_error("XML Parser: warning: " + text);
}

void DbXmlImporter::on_error(const Glib::ustring& text)
{
        std::cerr << "XML Parser: error: " << text << std::endl;
        //throw std::runtime_error("XML Parser: error: " + text);
}

void DbXmlImporter::on_fatal_error(const Glib::ustring& text)
{
        throw std::runtime_error("XML Parser: fatal error: " + text);
}


void DbXmlImporter::begin_record(void)
{
	m_rec_mid.clear();
	m_rec_codetype.clear();
	m_rec_codeid.clear();
	m_rec_codetypenorth.clear();
	m_rec_codechannel.clear();
	m_rec_codeclass.clear();
	m_rec_codeworkhr.clear();
	m_rec_codedatum.clear();
	m_rec_geolat.clear();
	m_rec_geolon.clear();
	m_rec_txtname.clear();
	m_rec_txtrmk.clear();
	m_rec_valfreq.clear();
	m_rec_uomfreq.clear();
	m_rec_valghostfreq.clear();
	m_rec_uomghostfreq.clear();
	m_rec_dtwef.clear();
	m_rec_codelocind.clear();
	m_rec_codedistverupper.clear();
	m_rec_valdistverupper.clear();
	m_rec_uomdistverupper.clear();
	m_rec_codedistverlower.clear();
	m_rec_valdistverlower.clear();
	m_rec_uomdistverlower.clear();
	m_rec_geolatarc.clear();
	m_rec_geolonarc.clear();
	m_rec_valradiusarc.clear();
	m_rec_uomradiusarc.clear();
	m_rec_valcrc.clear();
	m_rec_valgeoaccuracy.clear();
	m_rec_uomgeoaccuracy.clear();
	m_rec_ase_codetype.clear();
	m_rec_ase_codeid.clear();
	m_rec_gbr_txtname.clear();
	m_rec_org_txtname.clear();
	m_rec_orgcre_txtname.clear();
	m_rec_vor_codeid.clear();
	m_rec_vor_geolat.clear();
	m_rec_vor_geolon.clear();
	m_rec_rte_txtdesig.clear();
	m_rec_rte_txtlocdesig.clear();
	m_rec_spnsta_codetype.clear();
	m_rec_spnsta_codeid.clear();
	m_rec_spnend_codetype.clear();
	m_rec_spnend_codeid.clear();
}

void DbXmlImporter::end_record(void)
{
	if (!m_rec_rte_txtdesig.empty()) {
		process_airway();
		return;
	}
	if (!m_rec_codeclass.empty() || m_rec_uomfreq == "KHZ") {
		process_ndb();
		return;
	}
	if (m_rec_codetype == "VOR" || m_rec_codetype == "DVOR") {
		process_vor();
		return;
	}
	if (!m_rec_vor_codeid.empty() || !m_rec_codechannel.empty()) {
		process_dme();
		return;
	}
	if (!m_rec_ase_codetype.empty() || !m_rec_ase_codeid.empty()) {
		process_airspace_vertex();
		return;
	}
	if (m_rec_codetype == "ADHP" || m_rec_codetype == "COORD" || m_rec_codetype == "ICAO" || m_rec_codetype == "OTHER") {
		process_intersection();
		return;
	}
	if (!m_rec_codetype.empty()) {
		process_airspace();
		return;
	}
	record_error("Cannot Determine Record Type");
}

void DbXmlImporter::record_error(const Glib::ustring& msg)
{
	std::cerr << msg << ": mid " << m_rec_mid
		  << ", codetype " << m_rec_codetype
		  << ", codeid " << m_rec_codeid
		  << ", codetypenorth " << m_rec_codetypenorth
		  << ", codechannel " << m_rec_codechannel
		  << ", codeclass " << m_rec_codeclass
		  << ", codeworkhr " << m_rec_codeworkhr
		  << ", codedatum " << m_rec_codedatum
		  << ", geolat " << m_rec_geolat
		  << ", geolon " << m_rec_geolon
		  << ", txtname " << m_rec_txtname
		  << ", txtrmk " << m_rec_txtrmk
		  << ", valfreq " << m_rec_valfreq
		  << ", uomfreq " << m_rec_uomfreq
		  << ", valghostfreq " << m_rec_valghostfreq
		  << ", uomghostfreq " << m_rec_uomghostfreq
		  << ", dtwef " << m_rec_dtwef
		  << ", codelocind " << m_rec_codelocind
		  << ", codedistverupper " << m_rec_codedistverupper
		  << ", valdistverupper " << m_rec_valdistverupper
		  << ", uomdistverupper " << m_rec_uomdistverupper
		  << ", codedistverlower " << m_rec_codedistverlower
		  << ", valdistverlower " << m_rec_valdistverlower
		  << ", uomdistverlower " << m_rec_uomdistverlower
		  << ", geolatarc " << m_rec_geolatarc
		  << ", geolonarc " << m_rec_geolonarc
		  << ", valradiusarc " << m_rec_valradiusarc
		  << ", uomradiusarc " << m_rec_uomradiusarc
		  << ", valcrc " << m_rec_valcrc
		  << ", valgeoaccuracy " << m_rec_valgeoaccuracy
		  << ", uomgeoaccuracy " << m_rec_uomgeoaccuracy
		  << ", ase_codetype " << m_rec_ase_codetype
		  << ", ase_codeid " << m_rec_ase_codeid
		  << ", gbr_txtname " << m_rec_gbr_txtname
		  << ", org_txtname " << m_rec_org_txtname
		  << ", orgcre_txtname " << m_rec_orgcre_txtname
		  << ", vor_codeid " << m_rec_vor_codeid
		  << ", vor_geolat " << m_rec_vor_geolat
		  << ", vor_geolon " << m_rec_vor_geolon
		  << ", rte_txtdesig " << m_rec_rte_txtdesig
		  << ", rte_txtlocdesig " << m_rec_rte_txtlocdesig
		  << ", spnsta_codetype " << m_rec_spnsta_codetype
		  << ", spnsta_codeid " << m_rec_spnsta_codeid
		  << ", spnend_codetype " << m_rec_spnend_codetype
		  << ", spnend_codeid " << m_rec_spnend_codeid
		  << std::endl;
}

uint32_t DbXmlImporter::get_frequency(void) const
{
	if (m_rec_valfreq.empty())
		return 0;
	double freq(Glib::Ascii::strtod(m_rec_valfreq));
	if (freq < 0)
		return 0;
	Glib::ustring unit(m_rec_uomfreq.uppercase());
	if (unit == "KHZ")
		freq *= 1000;
	else if (unit == "MHZ")
		freq *= 1000000;
	else if (unit != "HZ")
		return 0;
	return Point::round<uint32_t,double>(freq);
}

uint32_t DbXmlImporter::get_channel_frequency(void) const
{
	if (m_rec_codechannel.empty())
		return 0;
	char *cp(0);
	uint32_t ch(strtoul(m_rec_codechannel.c_str(), &cp, 10));
	if (cp == m_rec_codechannel.c_str())
		return 0;
	bool chy(*cp == 'Y' || *cp == 'y');
	if (!chy && !(*cp == 'X' || *cp == 'x'))
		return 0;
	++cp;
	if (*cp)
		return 0;
	// unpaired channels
	if (ch < 1 || ch > 126)
		return 0;
	if (ch < 17) {
		if (chy)
			return 0;
		return 134300000 + ch * 100000;
	}
	if (ch >= 60 && ch < 70) {
		if (chy)
			return 0;
		return 127300000 + ch * 100000;
	}
	// paired channels
	uint32_t freq(100000000);
	if (ch < 60)
		freq += (ch + 63) * 100000;
	else
		freq += (ch + 53) * 100000;
	if (chy)
		freq += 50000;
	return freq;
}

void DbXmlImporter::process_intersection(void)
{
	static const double separate_distance = 50;
	static const double separate_distance_other = 50;
	double sepdist(separate_distance);
	if (m_rec_codetype == "OTHER")
		sepdist = separate_distance_other;
	char codetype(WaypointsDb::Waypoint::type_invalid);
	if (m_rec_codetype == "ADHP")
		codetype = WaypointsDb::Waypoint::type_adhp;
	else if (m_rec_codetype == "ICAO")
		codetype = WaypointsDb::Waypoint::type_icao;
	else if (m_rec_codetype == "OTHER")
		codetype = WaypointsDb::Waypoint::type_other;
	else if (m_rec_codetype == "COORD")
		codetype = WaypointsDb::Waypoint::type_coord;
	if (codetype == WaypointsDb::Waypoint::type_invalid) {
		record_error("Skipping Unknow Intersection Type " + m_rec_codetype);
		return;
	}
	if (m_rec_codeid.empty()) {
		record_error("Empty codeid");
		return;
	}
	Point pt;
	if ((Point::setstr_lat | Point::setstr_lon) != pt.set_str(m_rec_geolat + " " + m_rec_geolon)) {
		record_error("Cannot parse coordinate");
		return;
	}
	if (false)
		std::cerr << "Intersection: " << m_rec_codeid << ' ' << pt.get_lat_str2() << ' ' << pt.get_lon_str2()
			  << " (orig: " << m_rec_geolat << ' ' << m_rec_geolon << ")" << std::endl;
	open_waypoints_db();
	if (m_intsdelete) {
		long cntbefore(-1), cntafter(-1);
		{
                        sqlite3x::sqlite3_command cmd(m_waypointsdb.get_db(), "SELECT COUNT(*) FROM waypoints;");
                        sqlite3x::sqlite3_cursor cursor = cmd.executecursor();
                        if (cursor.step())
                                cntbefore = cursor.getint(0);
		}
		try {
			sqlite3x::sqlite3_command cmd(m_waypointsdb.get_db(), "DELETE FROM waypoints_rtree WHERE ID IN (SELECT ID FROM waypoints WHERE SRCID LIKE \"%@EAD\");");
			cmd.executenonquery();
		} catch (const sqlite3x::database_error& ex) {
			std::cerr << "Intersection: cannot modify rtree: " << ex.what() << std::endl;
		}
		{
			sqlite3x::sqlite3_command cmd(m_waypointsdb.get_db(), "DELETE FROM waypoints WHERE SRCID LIKE \"%@EAD\";");
			cmd.executenonquery();
		}
		{
                        sqlite3x::sqlite3_command cmd(m_waypointsdb.get_db(), "SELECT COUNT(*) FROM waypoints;");
                        sqlite3x::sqlite3_cursor cursor = cmd.executecursor();
                        if (cursor.step())
                                cntafter = cursor.getint(0);
		}
		m_intsdelete = false;
		std::cerr << "Intersections: " << cntbefore << " before, " << cntafter
			  << " after deletion of @EAD intersections" << std::endl;
	}
	std::string srcid(m_rec_codeid.uppercase() + "_" + m_rec_codetype + "_" + m_rec_mid + "@EAD");
	// check for existing waypoint
	{
		WaypointsDb::element_t *e(0);
		WaypointsDb::elementvector_t ev(m_waypointsdb.find_by_sourceid(srcid));
		if (!ev.empty()) {
			e = &ev[0];
		} else {
			ev = m_waypointsdb.find_by_name(m_rec_codeid);
			uint64_t d(std::numeric_limits<uint64_t>::max());
			for (WaypointsDb::elementvector_t::iterator evi(ev.begin()), eve(ev.end()); evi != eve; ++evi) {
				WaypointsDb::element_t& el(*evi);
				if (el.get_name().uppercase() != m_rec_codeid.uppercase())
					continue;
				if (srcid == el.get_sourceid()) {
					e = &el;
					break;
				}
				uint64_t d1(el.get_coord().simple_distance_rel(pt));
				if (d1 >= d)
					continue;
				d = d1;
				e = &el;
			}
		}
		double dist(std::numeric_limits<double>::quiet_NaN());
		if (e && e->get_sourceid() != srcid) {
			const Glib::ustring& esrcid(e->get_sourceid());
			if (esrcid.size() >= 4 && !esrcid.compare(esrcid.size() - 4, 4, "@EAD")) {
				e = 0;
			} else {
				dist = e->get_coord().spheric_distance_nmi(pt);
				if (dist > sepdist)
					e = 0;
			}
		}
		if (e) {
			if (std::isnan(dist))
				dist = e->get_coord().spheric_distance_nmi(pt);
			if (dist > 0.1)
				std::cerr << "Intersection " << e->get_name() << " has moved from "
					  << e->get_coord().get_lat_str2() << ' ' << e->get_coord().get_lon_str2() << " to "
					  << pt.get_lat_str2() << ' ' << pt.get_lon_str2() << " dist " << dist << std::endl;
			e->set_coord(pt);
			e->set_modtime(time(0));
			e->set_sourceid(srcid);
			m_waypointsdb.save(*e);
			for (WaypointsDb::elementvector_t::iterator evi(ev.begin()), eve(ev.end()); evi != eve; ++evi) {
				WaypointsDb::element_t& el(*evi);
				if (el.get_id() == e->get_id())
					continue;
				if (el.get_name().uppercase() != m_rec_codeid.uppercase())
					continue;
				double dist(el.get_coord().spheric_distance_nmi(pt));
				if (dist > sepdist)
					continue;
				{
					const Glib::ustring& srcid(el.get_sourceid());
					if (srcid.size() >= 4 && !srcid.compare(srcid.size() - 4, 4, "@EAD"))
						continue;
				}
				std::cerr << "Deleting Intersection " << el.get_name() << ' '
					  << el.get_coord().get_lat_str2() << ' ' << el.get_coord().get_lon_str2()
					  << " dist " << dist << std::endl;
				m_waypointsdb.erase(el);
			}
			return;
		}
	}
	WaypointsDb::element_t el;
	el.set_sourceid(srcid);
	el.set_usage(WaypointsDb::element_t::usage_bothlevel);
	el.set_type(codetype);
	el.set_label_placement(WaypointsDb::element_t::label_e);
	el.set_coord(pt);
	el.set_name(m_rec_codeid.uppercase());
	el.set_modtime(time(0));
	if (true)
		std::cerr << "New Intersection " << el.get_name() << ' '
			  << el.get_coord().get_lat_str2() << ' ' << el.get_coord().get_lon_str2() << ' ' << el.get_sourceid() << std::endl;
	try {
		m_waypointsdb.save(el);
	} catch (const std::exception& e) {
		std::cerr << "I/O Error: " << e.what() << std::endl;
		throw;
	}
}

void DbXmlImporter::process_ndb(void)
{
	static const double separate_distance = 50;
	// we cannot skip locator beacons; L615 uses locator beacon ORI in airway definition
	if ((false && m_rec_codeclass.empty()) || (false && m_rec_codeclass == "L")) {
		record_error("NDB rec_codeclass empty");
		return;
	}
	if (m_rec_codeid.empty()) {
		record_error("Empty codeid");
		return;
	}
	Point pt;
	if ((Point::setstr_lat | Point::setstr_lon) != pt.set_str(m_rec_geolat + " " + m_rec_geolon)) {
		record_error("Cannot parse coordinate");
		return;
	}
	uint32_t freq(get_frequency());
	if (freq < 100000 || freq > 2000000) {
		std::ostringstream oss;
		oss << freq;
		record_error("Cannot parse frequency (" + oss.str() + ")");
		return;
	}
	if (false)
		std::cerr << "NDB: " << m_rec_codeid << ' ' << pt.get_lat_str2() << ' ' << pt.get_lon_str2()
			  << " freq " << Conversions::freq_str(freq) << " class " << m_rec_codeclass
			  << " (orig: " << m_rec_geolat << ' ' << m_rec_geolon << ")" << std::endl;
	open_navaids_db();
	// check for existing navaid
	{
		NavaidsDb::element_t *e(0);
		NavaidsDb::elementvector_t ev(m_navaidsdb.find_by_sourceid(m_rec_mid + "@EAD"));
		if (!ev.empty()) {
			e = &ev[0];
		} else {
			ev = m_navaidsdb.find_by_icao(m_rec_codeid);
			uint64_t d(std::numeric_limits<uint64_t>::max());
			for (NavaidsDb::elementvector_t::iterator evi(ev.begin()), eve(ev.end()); evi != eve; ++evi) {
				NavaidsDb::element_t& el(*evi);
				if (el.get_navaid_type() != NavaidsDb::element_t::navaid_ndb &&
				    el.get_navaid_type() != NavaidsDb::element_t::navaid_ndbdme)
					continue;
				if (el.get_icao().uppercase() != m_rec_codeid.uppercase())
					continue;
				if (m_rec_mid + "@EAD" == el.get_sourceid()) {
					e = &el;
					break;
				}
				if (el.get_sourceid().size() > 4 && el.get_sourceid().substr(el.get_sourceid().size() - 4) == "@EAD")
					continue;
				uint64_t d1(el.get_coord().simple_distance_rel(pt));
				if (d1 >= d)
					continue;
				d = d1;
				e = &el;
			}
		}
		double dist(std::numeric_limits<double>::quiet_NaN());
		if (e) {
			dist = e->get_coord().spheric_distance_nmi(pt);
			if (e->get_sourceid() != m_rec_mid + "@EAD" && dist > separate_distance)
				e = 0;
		}
		if (e) {
			if (dist > 0.1)
				std::cerr << "NDB " << e->get_icao() << ' ' << e->get_name() << " has moved from "
					  << e->get_coord().get_lat_str2() << ' ' << e->get_coord().get_lon_str2() << " to "
					  << pt.get_lat_str2() << ' ' << pt.get_lon_str2() << " dist " << dist << std::endl;
			if (e->get_frequency() != freq)
				std::cerr << "NDB " << e->get_icao() << ' ' << e->get_name() << " has changed frequency from "
					  << Conversions::freq_str(e->get_frequency()) << " to " << Conversions::freq_str(freq) << std::endl;
			e->set_coord(pt);
			e->set_frequency(freq);
			e->set_sourceid(m_rec_mid + "@EAD");
			e->set_modtime(time(0));
			m_navaidsdb.save(*e);
			for (NavaidsDb::elementvector_t::iterator evi(ev.begin()), eve(ev.end()); evi != eve; ++evi) {
				NavaidsDb::element_t& el(*evi);
				if (el.get_navaid_type() != NavaidsDb::element_t::navaid_ndb)
					continue;
				if (el.get_id() == e->get_id())
					continue;
				if (el.get_icao().uppercase() != m_rec_codeid.uppercase())
					continue;
				if (el.get_sourceid().size() > 4 && el.get_sourceid().substr(el.get_sourceid().size() - 4) == "@EAD")
					continue;
				double dist(el.get_coord().spheric_distance_nmi(pt));
				if (dist > separate_distance)
					continue;
				std::cerr << "Deleting NDB " << el.get_icao() << ' ' << el.get_name() << ' '
					  << el.get_coord().get_lat_str2() << ' ' << el.get_coord().get_lon_str2()
					  << " freq " << Conversions::freq_str(el.get_frequency())
					  << " dist " << dist << std::endl;
				m_navaidsdb.erase(el);
			}
			return;
		}
	}
	NavaidsDb::element_t el;
	el.set_sourceid(m_rec_mid + "@EAD");
	el.set_label_placement(NavaidsDb::element_t::label_e);
	el.set_navaid_type(NavaidsDb::element_t::navaid_ndb);
	el.set_coord(pt);
	el.set_frequency(freq);
	el.set_icao(m_rec_codeid.uppercase());
	el.set_name(m_rec_txtname);
	el.set_modtime(time(0));
	if (true)
		std::cerr << "New NDB " << el.get_icao() << ' ' << el.get_name() << ' '
			  << el.get_coord().get_lat_str2() << ' ' << el.get_coord().get_lon_str2()
			  << " freq " << Conversions::freq_str(el.get_frequency()) << ' ' << el.get_sourceid() << std::endl;
	try {
		m_navaidsdb.save(el);
	} catch (const std::exception& e) {
		std::cerr << "I/O Error: " << e.what() << std::endl;
		throw;
	}
}

void DbXmlImporter::process_vor(void)
{
	static const double separate_distance = 50;
	if (m_rec_codeid.empty()) {
		record_error("Empty codeid");
		return;
	}
	Point pt;
	if ((Point::setstr_lat | Point::setstr_lon) != pt.set_str(m_rec_geolat + " " + m_rec_geolon)) {
		record_error("Cannot parse coordinate");
		return;
	}
	uint32_t freq(get_frequency());
	if (freq < 108000000 || freq > 118000000) {
		std::ostringstream oss;
		oss << freq;
		record_error("Cannot parse frequency (" + oss.str() + ")");
		return;
	}
	if (false)
		std::cerr << "VOR: " << m_rec_codeid << ' ' << pt.get_lat_str2() << ' ' << pt.get_lon_str2()
			  << " freq " << Conversions::freq_str(freq) << " class " << m_rec_codeclass
			  << " (orig: " << m_rec_geolat << ' ' << m_rec_geolon << ")" << std::endl;
	open_navaids_db();
	// check for existing navaid
	{
		NavaidsDb::element_t *e(0);
		NavaidsDb::elementvector_t ev(m_navaidsdb.find_by_sourceid(m_rec_mid + "@EAD"));
		if (!ev.empty()) {
			e = &ev[0];
		} else {
			ev = m_navaidsdb.find_by_icao(m_rec_codeid);
			uint64_t d(std::numeric_limits<uint64_t>::max());
			for (NavaidsDb::elementvector_t::iterator evi(ev.begin()), eve(ev.end()); evi != eve; ++evi) {
				NavaidsDb::element_t& el(*evi);
				if (el.get_navaid_type() != NavaidsDb::element_t::navaid_vor &&
				    el.get_navaid_type() != NavaidsDb::element_t::navaid_vordme &&
				    el.get_navaid_type() != NavaidsDb::element_t::navaid_dme &&
				    el.get_navaid_type() != NavaidsDb::element_t::navaid_vortac &&
				    el.get_navaid_type() != NavaidsDb::element_t::navaid_tacan)
					continue;
				if (el.get_icao().uppercase() != m_rec_codeid.uppercase())
					continue;
				if (m_rec_mid + "@EAD" == el.get_sourceid()) {
					e = &el;
					break;
				}
				uint64_t d1(el.get_coord().simple_distance_rel(pt));
				if (d1 >= d)
					continue;
				if (el.get_navaid_type() == NavaidsDb::element_t::navaid_dme &&
				    el.get_frequency() != freq)
					continue;
				d = d1;
				e = &el;
			}
		}
		double dist(std::numeric_limits<double>::quiet_NaN());
		if (e) {
			dist = e->get_coord().spheric_distance_nmi(pt);
			if (e->get_sourceid() != m_rec_mid + "@EAD" && dist > separate_distance)
				e = 0;
		}
		if (e) {
			if (dist > 0.1)
				std::cerr << e->get_navaid_typename() << ' ' << e->get_icao() << ' ' << e->get_name() << " has moved from "
					  << e->get_coord().get_lat_str2() << ' ' << e->get_coord().get_lon_str2() << " to "
					  << pt.get_lat_str2() << ' ' << pt.get_lon_str2() << " dist " << dist << std::endl;
			if (e->get_frequency() != freq)
				std::cerr << e->get_navaid_typename() << ' ' << e->get_icao() << ' ' << e->get_name() << " has changed frequency from "
					  << Conversions::freq_str(e->get_frequency()) << " to " << Conversions::freq_str(freq) << std::endl;
			if (e->get_navaid_type() == NavaidsDb::element_t::navaid_dme) {
				std::cerr << e->get_navaid_typename() << ' ' << e->get_icao() << ' ' << e->get_name() << " is now colocated with VOR"
					  << std::endl;
				e->set_navaid_type(NavaidsDb::element_t::navaid_vordme);
			} else if (e->get_navaid_type() == NavaidsDb::element_t::navaid_tacan) {
				std::cerr << e->get_navaid_typename() << ' ' << e->get_icao() << ' ' << e->get_name() << " is now colocated with VOR"
					  << std::endl;
				e->set_navaid_type(NavaidsDb::element_t::navaid_vortac);
			}
			e->set_coord(pt);
			e->set_frequency(freq);
			e->set_modtime(time(0));
			m_navaidsdb.save(*e);
			for (NavaidsDb::elementvector_t::iterator evi(ev.begin()), eve(ev.end()); evi != eve; ++evi) {
				NavaidsDb::element_t& el(*evi);
				if (el.get_navaid_type() != NavaidsDb::element_t::navaid_vor &&
				    el.get_navaid_type() != NavaidsDb::element_t::navaid_vordme &&
				    el.get_navaid_type() != NavaidsDb::element_t::navaid_dme &&
				    el.get_navaid_type() != NavaidsDb::element_t::navaid_vortac &&
				    el.get_navaid_type() != NavaidsDb::element_t::navaid_tacan)
					continue;
				if (el.get_id() == e->get_id())
					continue;
				if (el.get_icao().uppercase() != m_rec_codeid.uppercase())
					continue;
				double dist(el.get_coord().spheric_distance_nmi(pt));
				if (dist > separate_distance)
					continue;
				if ((el.get_navaid_type() == NavaidsDb::element_t::navaid_dme ||
				     el.get_navaid_type() == NavaidsDb::element_t::navaid_tacan) &&
				    e->get_navaid_type() == NavaidsDb::element_t::navaid_vor) {
					if (el.get_frequency() != e->get_frequency())
						continue;
					std::cerr << e->get_navaid_typename() << ' ' << e->get_icao() << ' ' << e->get_name() << " is now colocated with DME"
						  << std::endl;
					e->set_navaid_type((el.get_navaid_type() == NavaidsDb::element_t::navaid_tacan) ? 
							   NavaidsDb::element_t::navaid_vortac : NavaidsDb::element_t::navaid_vordme);
					e->set_dmecoord(el.get_dmecoord());
					m_navaidsdb.save(*e);
				}
				std::cerr << "Deleting " << el.get_navaid_typename() << ' ' << el.get_icao() << ' ' << el.get_name() << ' '
					  << el.get_coord().get_lat_str2() << ' ' << el.get_coord().get_lon_str2()
					  << " freq " << Conversions::freq_str(el.get_frequency())
					  << " dist " << dist << std::endl;
				m_navaidsdb.erase(el);
			}
			return;
		}
	}
	NavaidsDb::element_t el;
	el.set_sourceid(m_rec_mid + "@EAD");
	el.set_label_placement(NavaidsDb::element_t::label_e);
	el.set_navaid_type(NavaidsDb::element_t::navaid_vor);
	el.set_coord(pt);
	el.set_frequency(freq);
	el.set_icao(m_rec_codeid.uppercase());
	el.set_name(m_rec_txtname);
	el.set_modtime(time(0));
	if (true)
		std::cerr << "New " << el.get_navaid_typename() << ' ' << el.get_icao() << ' ' << el.get_name() << ' '
			  << el.get_coord().get_lat_str2() << ' ' << el.get_coord().get_lon_str2()
			  << " freq " << Conversions::freq_str(el.get_frequency()) << ' ' << el.get_sourceid() << std::endl;
	try {
		m_navaidsdb.save(el);
	} catch (const std::exception& e) {
		std::cerr << "I/O Error: " << e.what() << std::endl;
		throw;
	}
}

void DbXmlImporter::process_dme(void)
{
	static const double separate_distance = 50;
	if (m_rec_codeid.empty()) {
		record_error("Empty codeid");
		return;
	}
	Point pt;
	if ((Point::setstr_lat | Point::setstr_lon) != pt.set_str(m_rec_geolat + " " + m_rec_geolon)) {
		record_error("Cannot parse coordinate");
		return;
	}
	Point ptvor;
	if ((Point::setstr_lat | Point::setstr_lon) != ptvor.set_str(m_rec_vor_geolat + " " + m_rec_vor_geolon))
		ptvor = pt;
	uint32_t freq(get_channel_frequency());
	if (freq < 108000000 || freq > 136000000) {
		std::ostringstream oss;
		oss << freq;
		record_error("Cannot parse frequency (" + oss.str() + ")");
		return;
	}
	if (false)
		std::cerr << "DME: " << m_rec_codeid << ' ' << pt.get_lat_str2() << ' ' << pt.get_lon_str2()
			  << " freq " << Conversions::freq_str(freq) << " class " << m_rec_codeclass
			  << " ghostfreq " << m_rec_valghostfreq << m_rec_uomghostfreq
			  << " (orig: " << m_rec_geolat << ' ' << m_rec_geolon << ")" << std::endl;
	open_navaids_db();
	// check for existing navaid
	{
		NavaidsDb::element_t *e(0);
		NavaidsDb::elementvector_t ev(m_navaidsdb.find_by_sourceid(m_rec_mid + "@EAD"));
		if (!ev.empty()) {
			e = &ev[0];
		} else {
			ev = m_navaidsdb.find_by_icao(m_rec_codeid);
			uint64_t d(std::numeric_limits<uint64_t>::max());
			for (NavaidsDb::elementvector_t::iterator evi(ev.begin()), eve(ev.end()); evi != eve; ++evi) {
				NavaidsDb::element_t& el(*evi);
				if (el.get_navaid_type() != NavaidsDb::element_t::navaid_vor &&
				    el.get_navaid_type() != NavaidsDb::element_t::navaid_vordme &&
				    el.get_navaid_type() != NavaidsDb::element_t::navaid_dme &&
				    el.get_navaid_type() != NavaidsDb::element_t::navaid_vortac &&
				    el.get_navaid_type() != NavaidsDb::element_t::navaid_tacan)
					continue;
				if (el.get_icao().uppercase() != m_rec_codeid.uppercase())
					continue;
				if (m_rec_mid + "@EAD" == el.get_sourceid()) {
					e = &el;
					break;
				}
				uint64_t d1;
				if (el.has_vor())
					d1 = el.get_coord().simple_distance_rel(ptvor);
				else
					d1 = el.get_dmecoord().simple_distance_rel(pt);
				if (d1 >= d)
					continue;
				if (el.get_navaid_type() == NavaidsDb::element_t::navaid_vor &&
				    el.get_frequency() != freq)
					continue;
				if ((el.get_navaid_type() == NavaidsDb::element_t::navaid_vor ||
				     el.get_navaid_type() == NavaidsDb::element_t::navaid_vordme ||
				     el.get_navaid_type() == NavaidsDb::element_t::navaid_vortac) &&
				    el.get_icao().uppercase() != m_rec_vor_codeid.uppercase())
					continue;
				d = d1;
				e = &el;
			}
		}
		double dist(std::numeric_limits<double>::quiet_NaN());
		if (e && e->get_sourceid() != m_rec_mid + "@EAD") {
			dist = e->get_coord().spheric_distance_nmi(pt);
			if (dist > separate_distance)
				e = 0;
		}
		if (e) {
			if (dist > 0.1)
				std::cerr << e->get_navaid_typename() << ' ' << e->get_icao() << ' ' << e->get_name() << " has moved from "
					  << e->get_coord().get_lat_str2() << ' ' << e->get_coord().get_lon_str2() << " to "
					  << pt.get_lat_str2() << ' ' << pt.get_lon_str2() << " dist " << dist << std::endl;
			if (e->get_frequency() != freq)
				std::cerr << e->get_navaid_typename() << ' ' << e->get_icao() << ' ' << e->get_name() << " has changed frequency from "
					  << Conversions::freq_str(e->get_frequency()) << " to " << Conversions::freq_str(freq) << std::endl;
			if (e->get_navaid_type() == NavaidsDb::element_t::navaid_vor) {
				std::cerr << e->get_navaid_typename() << ' ' << e->get_icao() << ' ' << e->get_name() << " is now colocated with "
					  << (m_tacan ? "TACAN" : "DME") << std::endl;
				e->set_navaid_type(m_tacan ? NavaidsDb::element_t::navaid_vortac : NavaidsDb::element_t::navaid_vordme);
			} else {
				e->set_coord(ptvor);
			}
			e->set_dmecoord(pt);
			e->set_frequency(freq);
			e->set_modtime(time(0));
			m_navaidsdb.save(*e);
			for (NavaidsDb::elementvector_t::iterator evi(ev.begin()), eve(ev.end()); evi != eve; ++evi) {
				NavaidsDb::element_t& el(*evi);
				if (el.get_navaid_type() != NavaidsDb::element_t::navaid_vor &&
				    el.get_navaid_type() != NavaidsDb::element_t::navaid_vordme &&
				    el.get_navaid_type() != NavaidsDb::element_t::navaid_dme &&
				    el.get_navaid_type() != NavaidsDb::element_t::navaid_vortac &&
				    el.get_navaid_type() != NavaidsDb::element_t::navaid_tacan)
					continue;
				if (el.get_id() == e->get_id())
					continue;
				if (el.get_icao().uppercase() != m_rec_codeid.uppercase())
					continue;
				double dist;
				if (el.has_vor())
					dist = el.get_coord().spheric_distance_nmi(ptvor);
				else
					dist = el.get_dmecoord().spheric_distance_nmi(pt);
				if (dist > separate_distance)
					continue;
				if ((el.get_navaid_type() == NavaidsDb::element_t::navaid_vor ||
				     el.get_navaid_type() == NavaidsDb::element_t::navaid_vordme ||
				     el.get_navaid_type() == NavaidsDb::element_t::navaid_vortac) &&
				    el.get_icao().uppercase() != m_rec_vor_codeid.uppercase())
					continue;
				if (el.get_navaid_type() == NavaidsDb::element_t::navaid_vor &&
				    (e->get_navaid_type() == NavaidsDb::element_t::navaid_dme ||
				     e->get_navaid_type() == NavaidsDb::element_t::navaid_tacan)) {
					if (el.get_frequency() != e->get_frequency())
						continue;
					std::cerr << e->get_navaid_typename() << ' ' << e->get_icao() << ' ' << e->get_name() << " is now colocated with VOR"
						  << std::endl;
					e->set_navaid_type((e->get_navaid_type() == NavaidsDb::element_t::navaid_tacan) ? NavaidsDb::element_t::navaid_vortac : NavaidsDb::element_t::navaid_vordme);
					e->set_coord(el.get_coord());
					m_navaidsdb.save(*e);
				}
				std::cerr << "Deleting " << el.get_navaid_typename() << ' ' << el.get_icao() << ' ' << el.get_name() << ' '
					  << el.get_coord().get_lat_str2() << ' ' << el.get_coord().get_lon_str2()
					  << " freq " << Conversions::freq_str(el.get_frequency())
					  << " dist " << dist << std::endl;
				m_navaidsdb.erase(el);
			}
			return;
		}
	}
	NavaidsDb::element_t el;
	el.set_sourceid(m_rec_mid + "@EAD");
	el.set_label_placement(NavaidsDb::element_t::label_e);
	el.set_navaid_type(m_tacan ? NavaidsDb::element_t::navaid_tacan : NavaidsDb::element_t::navaid_dme);
	el.set_coord(ptvor);
	el.set_dmecoord(pt);
	el.set_frequency(freq);
	el.set_icao(m_rec_codeid.uppercase());
	el.set_name(m_rec_txtname);
	el.set_modtime(time(0));
	if (true)
		std::cerr << "New " << el.get_navaid_typename() << ' ' << el.get_icao() << ' ' << el.get_name() << ' '
			  << el.get_coord().get_lat_str2() << ' ' << el.get_coord().get_lon_str2()
			  << " freq " << Conversions::freq_str(el.get_frequency()) << ' ' << el.get_sourceid() << std::endl;
	try {
		m_navaidsdb.save(el);
	} catch (const std::exception& e) {
		std::cerr << "I/O Error: " << e.what() << std::endl;
		throw;
	}
}

NavaidsDb::Navaid::navaid_type_t DbXmlImporter::navaid_type(const Glib::ustring& ct)
{
	Glib::ustring ctu(ct.uppercase());
	if (ctu == "VOR")
		return NavaidsDb::Navaid::navaid_vor;
	if (ctu == "VOR/DME" || ctu == "DME/VOR")
		return NavaidsDb::Navaid::navaid_vordme;
	if (ctu == "DME")
		return NavaidsDb::Navaid::navaid_dme;
	if (ctu == "VORTAC" || ctu == "TACVOR")
		return NavaidsDb::Navaid::navaid_vortac;
	if (ctu == "TACAN")
		return NavaidsDb::Navaid::navaid_tacan;
	if (ctu == "NDB")
		return NavaidsDb::Navaid::navaid_ndb;
	if (ctu == "NDB/DME" || ctu == "DME/NDB")
		return NavaidsDb::Navaid::navaid_ndbdme;
	return NavaidsDb::Navaid::navaid_invalid;
}

void DbXmlImporter::process_airway(void)
{
	if (m_rec_rte_txtdesig.empty() || m_rec_spnsta_codetype.empty() || m_rec_spnsta_codeid.empty() ||
	    m_rec_spnend_codetype.empty() || m_rec_spnend_codeid.empty()) {
		record_error("Required Fields Empty");
		return;
	}
	open_navaids_db();
	open_waypoints_db();
	open_airways_db();
	AirwaysDb::element_t el;
	el.set_sourceid(m_rec_mid + "_" + m_rec_rte_txtdesig + "_" +
			m_rec_spnsta_codeid + "_" + m_rec_spnsta_codetype + "_" +
			m_rec_spnend_codeid + "_" + m_rec_spnend_codetype + "@EAD");
	el.set_label_placement(AirwaysDb::element_t::label_center);
	el.set_type(m_awytype);
	{
		AirwaysDb::elementvector_t ev(m_airwaysdb.find_by_sourceid(el.get_sourceid()));
		if (!ev.empty()) {
			if (ev.size() > 1)
				std::cerr << "Airway: " << ev.size() << " elements with source id " << el.get_sourceid() << " found" << std::endl;
			if (false)
				std::cerr << "Load: " << ev.front().get_sourceid() << " for " << el.get_sourceid() << std::endl;
			el = ev.front();
		}
	}
	el.set_name(m_rec_rte_txtdesig.uppercase());
	el.set_begin_name(m_rec_spnsta_codeid.uppercase());
	el.set_end_name(m_rec_spnend_codeid.uppercase());
	el.set_modtime(time(0));
	std::vector<Point> pt0, pt1;
	if (m_rec_spnsta_codetype.uppercase() == "WPT") {
		WaypointsDb::elementvector_t ev(m_waypointsdb.find_by_name(el.get_begin_name()));
		for (WaypointsDb::elementvector_t::iterator evi(ev.begin()), eve(ev.end()); evi != eve; ++evi) {
			WaypointsDb::element_t& e(*evi);
			if (e.get_name().uppercase() != el.get_begin_name())
				continue;
			pt0.push_back(e.get_coord());
		}
	} else {
		NavaidsDb::Navaid::navaid_type_t navtype(navaid_type(m_rec_spnsta_codetype));
		std::vector<Point> ptx1, ptx2;
		NavaidsDb::elementvector_t ev(m_navaidsdb.find_by_icao(el.get_begin_name()));
		for (NavaidsDb::elementvector_t::iterator evi(ev.begin()), eve(ev.end()); evi != eve; ++evi) {
			NavaidsDb::element_t& e(*evi);
			if (e.get_icao().uppercase() != el.get_begin_name())
				continue;
			unsigned int match(e.has_ndb() && NavaidsDb::Navaid::has_ndb(navtype));
			match += e.has_vor() && NavaidsDb::Navaid::has_vor(navtype);
			match += e.has_dme() && NavaidsDb::Navaid::has_dme(navtype);
			if (!match)
				continue;
			if (match >= 3)
				pt0.push_back(e.get_coord());
			else if (match >= 2)
				ptx1.push_back(e.get_coord());
			else if (match >= 1)
				ptx2.push_back(e.get_coord());
		}
		if (pt0.empty()) {
			pt0.swap(ptx1);
			if (pt0.empty())
				pt0.swap(ptx2);
		}
	}
 	if (m_rec_spnend_codetype.uppercase() == "WPT") {
		WaypointsDb::elementvector_t ev(m_waypointsdb.find_by_name(el.get_end_name()));
		for (WaypointsDb::elementvector_t::iterator evi(ev.begin()), eve(ev.end()); evi != eve; ++evi) {
			WaypointsDb::element_t& e(*evi);
			if (e.get_name().uppercase() != el.get_end_name())
				continue;
			pt1.push_back(e.get_coord());
		}
	} else {
		NavaidsDb::Navaid::navaid_type_t navtype(navaid_type(m_rec_spnend_codetype));
		std::vector<Point> ptx1, ptx2;
		NavaidsDb::elementvector_t ev(m_navaidsdb.find_by_icao(el.get_end_name()));
		for (NavaidsDb::elementvector_t::iterator evi(ev.begin()), eve(ev.end()); evi != eve; ++evi) {
			NavaidsDb::element_t& e(*evi);
			if (e.get_icao().uppercase() != el.get_end_name())
				continue;
			unsigned int match(e.has_ndb() && NavaidsDb::Navaid::has_ndb(navtype));
			match += e.has_vor() && NavaidsDb::Navaid::has_vor(navtype);
			match += e.has_dme() && NavaidsDb::Navaid::has_dme(navtype);
			if (!match)
				continue;
			if (match >= 3)
				pt1.push_back(e.get_coord());
			else if (match >= 2)
				ptx1.push_back(e.get_coord());
			else if (match >= 1)
				ptx2.push_back(e.get_coord());
		}
		if (pt1.empty()) {
			pt1.swap(ptx1);
			if (pt1.empty())
				pt1.swap(ptx2);
		}
	}
	if (pt0.empty()) {
		std::cerr << "Cannot find coordinate for " << m_rec_spnsta_codeid << " (" << m_rec_spnsta_codetype << ")" << std::endl;
		return;
	}
	if (pt1.empty()) {
		std::cerr << "Cannot find coordinate for " << m_rec_spnend_codeid << " (" << m_rec_spnend_codetype << ")" << std::endl;
		return;
	}
	if (el.get_begin_name() > el.get_end_name()) {
		el.swap_begin_end();
		pt0.swap(pt1);
	}
	{
		el.set_begin_coord(pt0[0]);
		el.set_end_coord(pt1[0]);
		AirwaysDb::element_t el2(el);
		uint64_t dmin(std::numeric_limits<uint64_t>::max());
		for (std::vector<Point>::const_iterator p0i(pt0.begin()), p0e(pt0.end()); p0i != p0e; ++p0i) {
			for (std::vector<Point>::const_iterator p1i(pt1.begin()), p1e(pt1.end()); p1i != p1e; ++p1i) {
				uint64_t d(p0i->simple_distance_rel(*p1i));
				if (d > dmin)
					continue;
				el2.set_begin_coord(*p0i);
				el2.set_end_coord(*p1i);
				if (m_airway.find(el2) != m_airway.end())
					continue;
				dmin = d;
				el.set_begin_coord(*p0i);
				el.set_end_coord(*p1i);
			}
		}
	}
	double dist(el.get_begin_coord().spheric_distance_nmi_dbl(el.get_end_coord()));
	el.set_base_level(std::numeric_limits<int16_t>::min());
	el.set_top_level(std::numeric_limits<int16_t>::max());
	{
		Glib::ustring alterr;
		bool ok(true);
		int32_t altoffset(0);
		{
			Glib::ustring c(m_rec_codedistverupper.uppercase());
			uint8_t fl(0);
			if (c == "STD") {
				fl |= AirspacesDb::Airspace::altflag_fl;
			} else if (c == "ALT" || c == "QNH") {
			} else if (c == "HEI" || c == "QFE") {
				fl |= AirspacesDb::Airspace::altflag_agl;
			} else {
				ok = false;
				alterr += ", Unknown codeDistVerUpper";
			}
			if (fl & AirspacesDb::Airspace::altflag_agl) {
				open_topo_db();
				TopoDb30::Profile prof(m_topodb.get_profile(el.get_begin_coord(), el.get_end_coord(), 5));
				TopoDb30::elev_t elev(prof.get_maxelev());
				if (prof.empty() || elev == TopoDb30::nodata) {
					std::ostringstream oss;
					oss << "WARNING: unable to compute ground level "
					    << el.get_begin_coord().get_lat_str2() << ' ' << el.get_begin_coord().get_lon_str2()
					    << " -> " << el.get_end_coord().get_lat_str2() << ' ' << el.get_end_coord().get_lon_str2()
					    << " dist " << dist << " profile size " << prof.size();
					record_error(oss.str());
				} else {
					altoffset = elev * Point::m_to_ft;
					if (true)
						std::cerr << "Awy " << el.get_name() << ' '
							  << el.get_begin_coord().get_lat_str2() << ' ' << el.get_begin_coord().get_lon_str2()
							  << " -> " << el.get_end_coord().get_lat_str2() << ' ' << el.get_end_coord().get_lon_str2()
							  << " dist " << dist << " max gnd elev " << altoffset << "ft" << std::endl;
				}
			}
		}
		{
			const char *cp(m_rec_valdistverupper.c_str());
			char *ep(const_cast<char *>(cp));
			int32_t alt(strtol(cp, &ep, 10));
			if (ep == cp || *ep) {
				ok = false;
				alterr += ", Unknown valDistVerUpper";
			}
			Glib::ustring u(m_rec_uomdistverupper.uppercase());
			if (u == "FT") {
			} else if (u == "FL") {
				alt *= 100;
			} else if (u == "M") {
				alt *= Point::m_to_ft;
			} else {
				ok = false;
				alterr += ", Unknown uomDistVerUpper";
			}
			if (ok)
				el.set_top_level((alt + altoffset) / 100);
		}
		bool errprt(!ok && !(m_rec_codedistverupper.empty() && m_rec_valdistverupper.empty() && m_rec_uomdistverupper.empty()));
		ok = true;
		altoffset = 0;
		{
			Glib::ustring c(m_rec_codedistverlower.uppercase());
			uint8_t fl(0);
			if (c == "STD") {
				fl |= AirspacesDb::Airspace::altflag_fl;
			} else if (c == "ALT" || c == "QNH") {
			} else if (c == "HEI" || c == "QFE") {
				fl |= AirspacesDb::Airspace::altflag_agl;
			} else {
				ok = false;
				alterr += ", Unknown codeDistVerLower";
			}
			if (fl & AirspacesDb::Airspace::altflag_agl) {
				open_topo_db();
				TopoDb30::Profile prof(m_topodb.get_profile(el.get_begin_coord(), el.get_end_coord(), 5));
				TopoDb30::elev_t elev(prof.get_maxelev());
				if (prof.empty() || elev == TopoDb30::nodata) {
					std::ostringstream oss;
					oss << "WARNING: unable to compute ground level "
					    << el.get_begin_coord().get_lat_str2() << ' ' << el.get_begin_coord().get_lon_str2()
					    << " -> " << el.get_end_coord().get_lat_str2() << ' ' << el.get_end_coord().get_lon_str2()
					    << " dist " << dist << " profile size " << prof.size();
					record_error(oss.str());
				} else {
					altoffset = elev * Point::m_to_ft;
					if (true)
						std::cerr << "Awy " << el.get_name() << ' '
							  << el.get_begin_coord().get_lat_str2() << ' ' << el.get_begin_coord().get_lon_str2()
							  << " -> " << el.get_end_coord().get_lat_str2() << ' ' << el.get_end_coord().get_lon_str2()
							  << " dist " << dist << " max gnd elev " << altoffset << "ft" << std::endl;
				}
			}
		}
		{
			const char *cp(m_rec_valdistverlower.c_str());
			char *ep(const_cast<char *>(cp));
			int32_t alt(strtol(cp, &ep, 10));
			if (ep == cp || *ep) {
				ok = false;
				alterr += ", Unknown valDistVerLower";
			}
			Glib::ustring u(m_rec_uomdistverlower.uppercase());
			if (u == "FT") {
			} else if (u == "FL") {
				alt *= 100;
			} else if (u == "M") {
				alt *= Point::m_to_ft;
			} else {
				ok = false;
				alterr += ", Unknown uomDistVerLower";
			}
			if (ok)
				el.set_base_level((alt + altoffset + 99) / 100);
		}
		errprt = errprt || (!ok && !(m_rec_codedistverlower.empty() && m_rec_valdistverlower.empty() && m_rec_uomdistverlower.empty()));
		if (errprt && !alterr.empty())
			record_error("WARNING: " + alterr.substr(2));
	}
	if (dist > 200)
		std::cerr << "Airway " << el.get_name() << " Segment " << el.get_begin_name() << '-' << el.get_end_name()
			  << ": Warning: Long Segment " << dist << std::endl;
	std::pair<airway_t::iterator,bool> ins(m_airway.insert(el));
	if (!ins.second)
		std::cerr << "Duplicate Airway " << el.get_name() << " Segment " << el.get_begin_name() << '-' << el.get_end_name() << std::endl;
}

void DbXmlImporter::finalize_airways(void)
{
	if (m_airway.empty())
		return;
	std::cerr << "Finalizing airways, " << m_airway.size() << " count" << std::endl;
	while (!m_airway.empty()) {
		airway_t::const_iterator ai(m_airway.begin());
		unsigned int count(0);
		do {
			if (ai->get_name() != m_airway.begin()->get_name())
				break;
			++ai;
			++count;
		} while (ai != m_airway.end());
		std::cerr << "Processing airway " << m_airway.begin()->get_name() << ", " << count << " segments" << std::endl;
		finalize_airway(m_airway.begin(), ai);
		m_airway.erase(m_airway.begin(), ai);
	}
        if (m_airwaysdbopen)
                m_airwaysdb.close();
        m_airwaysdbopen = false;
}

void DbXmlImporter::finalize_airway(airway_t::const_iterator awb, airway_t::const_iterator awe)
{
	static const double separate_distance = 500;
	if (awb == awe)
		return;
	open_airways_db();
	Rect bbox;
	{
		PolygonSimple p;
		for (airway_t::const_iterator ai(awb), ae(awe); ai != ae; ++ai) {
			p.push_back(ai->get_begin_coord());
			p.push_back(ai->get_end_coord());
		}
		bbox = p.get_bbox();
	}
	bbox.oversize_nmi(separate_distance);
	// find same-name airways
	AirwaysDb::elementvector_t ev(m_airwaysdb.find_by_name(awb->get_name(), 0, AirwaysDb::comp_contains));
	// zap airways that do not contain the exact airway (due to search with comp_contains, which is needed due to multiple airways being collapsed)
	for (AirwaysDb::elementvector_t::iterator evi(ev.begin()), eve(ev.end()); evi != eve; ) {
		AirwaysDb::element_t& el(*evi);
		if (el.remove_name(awb->get_name())) {
			++evi;
			continue;
		}
		evi = ev.erase(evi);
		eve = ev.end();
	}
	// copy airway attributes
	for (airway_t::const_iterator ai(awb), ae(awe); ai != ae; ++ai) {
		AirwaysDb::element_t& a(const_cast<AirwaysDb::element_t&>(*ai));
		for (AirwaysDb::elementvector_t::iterator evi(ev.begin()), eve(ev.end()); evi != eve;) {
			const AirwaysDb::element_t& el(*evi);
			if ((a.get_begin_name() == el.get_begin_name() && a.get_end_name() == el.get_end_name() &&
			     a.get_begin_coord().simple_distance_nmi(el.get_begin_coord()) < 0.1 &&
			     a.get_end_coord().simple_distance_nmi(el.get_end_coord()) < 0.1) ||
			    (a.get_begin_name() == el.get_end_name() && a.get_end_name() == el.get_begin_name() &&
			     a.get_begin_coord().simple_distance_nmi(el.get_end_coord()) < 0.1 &&
			     a.get_end_coord().simple_distance_nmi(el.get_begin_coord()) < 0.1)) {
				// take over the segment if this is the only airway belonging to that segment
				if (el.get_name_set().empty()) {
					AirwaysDb::element_t ax(a);
					a = el;
					a.set_name(ax.get_name());
					a.set_begin_name(ax.get_begin_name());
					a.set_end_name(ax.get_end_name());
					a.set_begin_coord(ax.get_begin_coord());
					a.set_end_coord(ax.get_end_coord());
					a.set_sourceid(ax.get_sourceid());
					if (ax.get_type() != AirwaysDb::Airway::airway_user)
						a.set_type(ax.get_type());
					if (ax.get_base_level() != std::numeric_limits<int16_t>::min())
						a.set_base_level(ax.get_base_level());
					if (ax.get_top_level() != std::numeric_limits<int16_t>::min())
						a.set_top_level(ax.get_top_level());
					evi = ev.erase(evi);
					eve = ev.end();
					continue;
				}
				a.set_label_placement(el.get_label_placement());
				if (a.get_type() == AirwaysDb::Airway::airway_user)
					a.set_type(el.get_type());
				if (a.get_base_level() == std::numeric_limits<int16_t>::min())
					a.set_base_level(el.get_base_level());
				if (a.get_top_level() == std::numeric_limits<int16_t>::min())
					a.set_top_level(el.get_top_level());
			}
			++evi;
		}
	}
	// try one-sided match
	for (airway_t::const_iterator ai(awb), ae(awe); ai != ae; ++ai) {
		AirwaysDb::element_t& a(const_cast<AirwaysDb::element_t&>(*ai));
		if (a.get_base_level() != std::numeric_limits<int16_t>::min() && a.get_top_level() != std::numeric_limits<int16_t>::min() &&
		    a.get_type() != AirwaysDb::Airway::airway_user)
			continue;
		for (AirwaysDb::elementvector_t::const_iterator evi(ev.begin()), eve(ev.end()); evi != eve; ++evi) {
			const AirwaysDb::element_t& el(*evi);
			if (a.get_begin_name() == el.get_begin_name() || a.get_end_name() == el.get_end_name() ||
			    a.get_begin_name() == el.get_end_name() || a.get_end_name() == el.get_begin_name()) {
				a.set_label_placement(el.get_label_placement());
				if (a.get_type() == AirwaysDb::Airway::airway_user)
					a.set_type(el.get_type());
				if (a.get_base_level() == std::numeric_limits<int16_t>::min())
					a.set_base_level(el.get_base_level());
				if (a.get_top_level() == std::numeric_limits<int16_t>::min())
					a.set_top_level(el.get_top_level());
			}
		}
	}
	// try internal match
	for (;;) {
		bool finished(true);
		unsigned int unmatched(0);
		for (airway_t::const_iterator ai(awb), ae(awe); ai != ae; ++ai) {
			AirwaysDb::element_t& a(const_cast<AirwaysDb::element_t&>(*ai));
			if (a.get_base_level() != std::numeric_limits<int16_t>::min() && a.get_top_level() != std::numeric_limits<int16_t>::min() &&
			    a.get_type() != AirwaysDb::Airway::airway_user)
				continue;
			++unmatched;
			for (airway_t::const_iterator axi(awb), axe(awe); axi != axe; ++axi) {
				const AirwaysDb::element_t& ax(*axi);
				if (ai == axi)
					continue;
				if (a.get_begin_name() == ax.get_begin_name() || a.get_end_name() == ax.get_end_name() ||
				    a.get_begin_name() == ax.get_end_name() || a.get_end_name() == ax.get_begin_name()) {
					a.set_label_placement(ax.get_label_placement());
					if (a.get_type() == AirwaysDb::Airway::airway_user && ax.get_type() != AirwaysDb::Airway::airway_user) {
						a.set_type(ax.get_type());
						finished = false;
					}
					if (a.get_base_level() == std::numeric_limits<int16_t>::min() && ax.get_base_level() != std::numeric_limits<int16_t>::min()) {
						a.set_base_level(ax.get_base_level());
						finished = false;
					}
					if (a.get_top_level() == std::numeric_limits<int16_t>::min() && ax.get_top_level() != std::numeric_limits<int16_t>::min()) {
						a.set_top_level(ax.get_top_level());
						finished = false;
					}
				}
			}
		}
		if (!finished)
			continue;
		if (!unmatched)
			break;
		std::cerr << "Airway " << awb->get_name() << " " << unmatched
			  << " segments without attributes, guessing" << std::endl;
		int16_t baselvl(std::numeric_limits<int16_t>::max());
		int16_t toplvl(std::numeric_limits<int16_t>::min());
		AirwaysDb::element_t::airway_type_t typ(AirwaysDb::element_t::airway_user);
		for (airway_t::const_iterator axi(awb), axe(awe); axi != axe; ++axi) {
			const AirwaysDb::element_t& ax(*axi);
			if (ax.get_type() != AirwaysDb::Airway::airway_user) {
				if ((typ == AirwaysDb::Airway::airway_low || typ == AirwaysDb::Airway::airway_both) &&
				    ax.get_type() == AirwaysDb::Airway::airway_high)
					typ = AirwaysDb::Airway::airway_both;
				else if ((typ == AirwaysDb::Airway::airway_high || typ == AirwaysDb::Airway::airway_both) &&
				    ax.get_type() == AirwaysDb::Airway::airway_low)
					typ = AirwaysDb::Airway::airway_both;
				else
					typ = ax.get_type();
			}
			if (ax.get_base_level() != std::numeric_limits<int16_t>::min())
				baselvl = std::min(baselvl, ax.get_base_level());
			if (ax.get_top_level() != std::numeric_limits<int16_t>::min())
				toplvl = std::max(toplvl, ax.get_top_level());
		}
		if (typ == AirwaysDb::element_t::airway_user)
			typ = AirwaysDb::element_t::airway_both;
		if (baselvl == std::numeric_limits<int16_t>::max()) {
			baselvl = 50;
			if (typ == AirwaysDb::Airway::airway_high)
				baselvl = 235;
		}
		if (toplvl == std::numeric_limits<int16_t>::min()) {
			toplvl = 660;
			if (typ == AirwaysDb::Airway::airway_low)
				toplvl = 235;
		}
		for (airway_t::const_iterator ai(awb), ae(awe); ai != ae; ++ai) {
			AirwaysDb::element_t& a(const_cast<AirwaysDb::element_t&>(*ai));
			if (a.get_type() == AirwaysDb::Airway::airway_user)
				a.set_type(typ);
			if (a.get_base_level() == std::numeric_limits<int16_t>::min())
				a.set_base_level(baselvl);
			if (a.get_top_level() == std::numeric_limits<int16_t>::min())
				a.set_top_level(toplvl);
		}
		break;
	}
	// I/O
	unsigned int delsegs(0), delnames(0), newsegs(0);
	try {
		// Save
		if (true)
			std::cerr << "Airway " << awb->get_name() << " start saving " << m_airway.size() << " elements" << std::endl;
		for (airway_t::const_iterator ai(awb), ae(awe); ai != ae; ++ai) {
			AirwaysDb::element_t& a(const_cast<AirwaysDb::element_t&>(*ai));
			if (true)
				std::cerr << "Saving airway " << a.get_name() << " segment " << a.get_begin_name() << '-' << a.get_end_name()
					  << " F" << a.get_base_level() << "...F" << a.get_top_level() << ' ' << a.get_sourceid() << std::endl;
			a.set_modtime(time(0));
			m_airwaysdb.save(a);
			++newsegs;
		}
		// Delete old segments
		if (true)
			std::cerr << "Airway " << awb->get_name() << " start examining to delete " << ev.size() << " elements" << std::endl;
		for (AirwaysDb::elementvector_t::iterator evi(ev.begin()), eve(ev.end()); evi != eve; ++evi) {
			AirwaysDb::element_t& el(*evi);
			if (!m_supersedefullawy && (!bbox.is_inside(el.get_begin_coord()) || !bbox.is_inside(el.get_end_coord())))
				continue;
			if (el.get_name_set().empty()) {
				m_airwaysdb.erase(el);
				++delsegs;
				continue;
			}
			el.set_modtime(time(0));
			m_airwaysdb.save(el);
			++delnames;
		}
	} catch (const std::exception& e) {
		std::cerr << "I/O Error: " << e.what() << std::endl;
		throw;
	}
	std::cerr << "Airway " << awb->get_name() << " new segments " << newsegs
		  << " deleted segments " << delsegs << " names " << delnames << std::endl;
}

void DbXmlImporter::process_airspace(void)
{
	char codetype(0);
	{
		uint8_t tc(0);
		char bc(0);
		Glib::ustring tt(m_rec_codetype);
		if (tt == "D")
			tt = "D-EAD";
		if (!AirspacesDb::Airspace::set_class_string(tc, bc, tt) || tc != AirspacesDb::Airspace::typecode_ead) {
			record_error("ERROR: Unknown codeType " + m_rec_codetype);
			return;
		}
		codetype = bc;
	}
	open_airspaces_db();
	AirspacesDb::element_t el;
	el.set_sourceid(m_rec_codetype + "_" + m_rec_codeid + "@EAD");
	el.set_label_placement(AirspacesDb::element_t::label_e);
	// check for existing airspace
	{
		AirspacesDb::elementvector_t ev(m_airspacesdb.find_by_sourceid(el.get_sourceid()));
		for (AirspacesDb::elementvector_t::const_iterator evi(ev.begin()), eve(ev.end()); evi != eve; ++evi) {
			if (evi->get_sourceid() != el.get_sourceid())
				continue;
			el = *evi;
			break;
		}
	}
	el.set_icao(m_rec_codeid);
	el.set_name(m_rec_txtname);
	el.set_typecode(AirspacesDb::Airspace::typecode_ead);
	el.set_bdryclass(codetype);
	{
		std::string alterr;
		{
			Glib::ustring c(m_rec_codedistverupper.uppercase());
			uint8_t fl(0);
			if (c == "STD") {
				fl |= AirspacesDb::Airspace::altflag_fl;
			} else if (c == "ALT" || c == "QNH") {
			} else if (c == "HEI" || c == "QFE") {
				fl |= AirspacesDb::Airspace::altflag_agl;
			} else {
				fl = AirspacesDb::Airspace::altflag_fl;
				alterr += ", Unknown codeDistVerUpper";
			}
			el.set_altuprflags(fl);
		}
		{
			int32_t alt(strtol(m_rec_valdistverupper.c_str(), 0, 10));
			Glib::ustring u(m_rec_uomdistverupper.uppercase());
			if (u == "FT") {
			} else if (u == "FL") {
				alt *= 100;
			} else if (u == "M") {
				alt *= Point::m_to_ft;
			} else {
				alt = 600;
				alterr += ", Unknown uomDistVerUpper";
			}
			el.set_altupr(alt);
		}
		{
			Glib::ustring c(m_rec_codedistverlower.uppercase());
			uint8_t fl(0);
			if (c == "STD") {
				fl |= AirspacesDb::Airspace::altflag_fl;
			} else if (c == "ALT" || c == "QNH") {
			} else if (c == "HEI" || c == "QFE") {
				fl |= AirspacesDb::Airspace::altflag_agl;
			} else {
				fl = AirspacesDb::Airspace::altflag_fl;
				alterr += ", Unknown codeDistVerLower";
			}
			el.set_altlwrflags(fl);
		}
		{
			int32_t alt(strtol(m_rec_valdistverlower.c_str(), 0, 10));
			Glib::ustring u(m_rec_uomdistverlower.uppercase());
			if (u == "FT") {
			} else if (u == "FL") {
				alt *= 100;
			} else if (u == "M") {
				alt *= Point::m_to_ft;
			} else {
				alt = 0;
				alterr += ", Unknown uomDistVerLower";
			}
			el.set_altlwr(alt);
		}
		if (!alterr.empty())
			record_error("WARNING: " + alterr.substr(2));
	}
	el.set_ident(m_rec_codelocind);
	el.set_note(m_rec_orgcre_txtname);
	el.set_classexcept("");
	el.clear_segments();
	el.set_modtime(time(0));
	if (true)
		std::cerr << "New/modified Airspace " << el.get_type_string() << '/' << el.get_class_string() << ' '
			  << el.get_icao() << ' ' << el.get_name() << ' ' << el.get_sourceid() << std::endl;
	try {
		m_airspacesdb.save(el);
		m_finalizeairspaces = true;
	} catch (const std::exception& e) {
		std::cerr << "I/O Error: " << e.what() << std::endl;
		throw;
	}
}

void DbXmlImporter::process_airspace_vertex(void)
{
	open_airspaces_db();
	// check for existing airspace
	{
		std::string srcid(m_rec_ase_codetype + "_" + m_rec_ase_codeid + "@EAD");
		if (m_airspace.is_valid() && m_airspace.get_sourceid() != srcid) {
			try {
				m_airspace.recompute_lineapprox(m_airspacesdb);
				m_airspace.recompute_bbox();
				m_airspace.compute_initial_label_placement();
				m_airspacesdb.save(m_airspace);
			} catch (const std::exception& e) {
				std::cerr << "I/O Error: " << e.what() << std::endl;
				throw;
			}
			m_airspace = AirspacesDb::Airspace();
		}
		if (!m_airspace.is_valid()) {
			AirspacesDb::elementvector_t ev(m_airspacesdb.find_by_sourceid(srcid));
			for (AirspacesDb::elementvector_t::const_iterator evi(ev.begin()), eve(ev.end()); evi != eve; ++evi) {
				if (evi->get_sourceid() != srcid)
					continue;
				if (evi->get_typecode() != AirspacesDb::Airspace::typecode_ead)
					continue;
				m_airspace = *evi;
				break;
			}
		}
 	}
	if (!m_airspace.is_valid() || m_airspace.get_typecode() != AirspacesDb::Airspace::typecode_ead) {
		record_error("ERROR: Airspace record not found for Vertex record");
		return;
	}
	Point pt;
	if ((Point::setstr_lat | Point::setstr_lon) != pt.set_str(m_rec_geolat + " " + m_rec_geolon))
		pt.set_invalid();
	Point ptarc;
	if ((Point::setstr_lat | Point::setstr_lon) != ptarc.set_str(m_rec_geolatarc + " " + m_rec_geolonarc))
		ptarc.set_invalid();
	double radius(std::numeric_limits<double>::quiet_NaN());
	if (!m_rec_valradiusarc.empty())
		radius = Glib::Ascii::strtod(m_rec_valradiusarc);
	{
		Glib::ustring x(m_rec_uomradiusarc.uppercase());
		if (x == "FT")
			radius *= Point::ft_to_m_dbl * 1e-3 * Point::km_to_nmi_dbl * (Point::from_deg_dbl / 60.0);
		else if (x == "M")
			radius *= 1e-3 * Point::km_to_nmi_dbl * (Point::from_deg_dbl / 60.0);
		else if (x == "KM")
			radius *= Point::km_to_nmi_dbl * (Point::from_deg_dbl / 60.0);
		else if (x == "NM")
			radius *= (Point::from_deg_dbl / 60.0);
		else
			radius = std::numeric_limits<double>::quiet_NaN();
	}
	{
		Point ptnext;
		if (m_airspace.get_nrsegments())
			ptnext = m_airspace[0].get_coord1();
		Glib::ustring ct(m_rec_codetype.uppercase());
		if (ct == "CIR") {
			if (std::isnan(radius) || ptarc.is_invalid()) {
				record_error("ERROR: Radius / Arc Center error");
				return;
			}
			AirspacesDb::Airspace::Segment seg(Point(), Point(), ptarc, 'C', radius, radius);
                        m_airspace.add_segment(seg);
		} else if (ct == "CCA" || ct == "CWA") {
			if (std::isnan(radius) || ptarc.is_invalid()) {
				record_error("ERROR: Radius / Arc Center error");
				return;
			}
			if (pt.is_invalid()) {
				record_error("ERROR: Coordinate error");
				return;
			}
			if (m_airspace.get_nrsegments())
				m_airspace[m_airspace.get_nrsegments() - 1U].set_coord2(pt);
			AirspacesDb::Airspace::Segment seg(pt, ptnext, ptarc, (ct == "CCA") ? 'L' : 'R', radius, radius);
                        m_airspace.add_segment(seg);
		} else if (ct == "GRC" || ct == "RHL") {
			if (pt.is_invalid()) {
				record_error("ERROR: Coordinate error");
				return;
			}
			if (m_airspace.get_nrsegments())
				m_airspace[m_airspace.get_nrsegments() - 1U].set_coord2(pt);
			AirspacesDb::Airspace::Segment seg(pt, ptnext, Point(), (ct == "RHL") ? 'H' : 'B', 0, 0);
                        m_airspace.add_segment(seg);
		} else if (ct == "END") {
			if (pt.is_invalid()) {
				record_error("ERROR: Coordinate error");
				return;
			}
			if (m_airspace.get_nrsegments())
				m_airspace[m_airspace.get_nrsegments() - 1U].set_coord2(pt);
			AirspacesDb::Airspace::Segment seg(pt, ptnext, Point(), 'E', 0, 0);
                        m_airspace.add_segment(seg);
		} else if (ct == "FNT") {
			if (pt.is_invalid()) {
				record_error("ERROR: Coordinate error");
				return;
			}
			if (m_rec_gbr_txtname.empty()) {
				record_error("ERROR: FNT without country ident");
				return;
			}
			if (m_airspace.get_nrsegments())
				m_airspace[m_airspace.get_nrsegments() - 1U].set_coord2(pt);
			AirspacesDb::Airspace::Segment seg(pt, ptnext, Point(), 'F', 0, 0);
                        m_airspace.add_segment(seg);
			m_airspace.set_classexcept(m_airspace.get_classexcept() + m_rec_gbr_txtname + ";");
		} else {
			record_error("ERROR: Unknown codeType");
			return;
		}
	}
	m_airspace.set_modtime(time(0));
	m_finalizeairspaces = true;
	if (false)
		std::cerr << " Airspace Vertex Added " << m_airspace.get_type_string() << '/' << m_airspace.get_class_string() << ' '
			  << m_airspace.get_icao() << ' ' << m_airspace.get_name() << ' ' << m_airspace.get_sourceid() << std::endl;
}

DbXmlImporter::ForEachAirspace::ForEachAirspace(time_t starttime)
	: m_starttime(starttime)
{
	//m_countrymap["THE FORMER YUGOSLAV REPUBLIC OF MACEDONIA"] = "FYROM";
	//m_countrymap["SPAIN_EAST"] = "SPAIN";
	//m_countrymap["SPAIN_WEST"] = "SPAIN";
	//m_countrymap["UNITEDSTATESOFAMERICA"] = "UNITED STATES";
	//m_countrymap["UNITED STATES MINOR OUTLYING ISLANDS"] = "UNITED STATES";
	//m_countrymap["UNITED STATES VIRGIN ISLANDS"] = "UNITED STATES";
	//m_countrymap["CZECHREPUBLIC"] = "CZECH REPUBLIC";
	//m_countrymap[""] = "";
	//m_countrymap[""] = "";
	//m_countrymap[""] = "";
	m_countrymap["ANTIGUAANDBARBUDA"] = "Antigua and Barbuda";
	m_countrymap["ALGERIA"] = "Algeria";
	m_countrymap["AZERBAIJAN"] = "Azerbaijan";
	m_countrymap["AZERBAIJAN_EAST"] = "Azerbaijan";
	m_countrymap["AZERBAIJAN_WEST"] = "Azerbaijan";
	m_countrymap["ALBANIA"] = "Albania";
	m_countrymap["ARMENIA"] = "Armenia";
	m_countrymap["ANGOLA"] = "Angola";
	m_countrymap["AMERICANSAMOA"] = "American Samoa";
	m_countrymap["ARGENTINA"] = "Argentina";
	m_countrymap["AUSTRALIA"] = "Australia";
	m_countrymap["BAHRAIN"] = "Bahrain";
	m_countrymap["BARBADOS"] = "Barbados";
	m_countrymap["BERMUDA"] = "Bermuda";
	m_countrymap["BAHAMAS"] = "Bahamas";
	m_countrymap["BANGLADESH"] = "Bangladesh";
	m_countrymap["BELIZE"] = "Belize";
	m_countrymap["BOSNIAANDHERZEGOVINA"] = "Bosnia and Herzegovina";
	m_countrymap["BOLIVIA"] = "Bolivia";
	m_countrymap["BURMA"] = "Burma";
	m_countrymap["BENIN"] = "Benin";
	m_countrymap["SOLOMONISLANDS"] = "Solomon Islands";
	m_countrymap["BRAZIL"] = "Brazil";
	m_countrymap["BULGARIA"] = "Bulgaria";
	m_countrymap["BRUNEIDARUSSALAM"] = "Brunei Darussalam";
	m_countrymap["CANADA"] = "Canada";
	m_countrymap["ALBERTA/BRITISH COLUMBIA"] = "Canada";
	m_countrymap["CAMBODIA"] = "Cambodia";
	m_countrymap["SRILANKA"] = "Sri Lanka";
	m_countrymap["CONGO"] = "Congo";
	m_countrymap["DEMOCRATICREPUBLICOFTHECONGO"] = "Democratic Republic of the Congo";
	m_countrymap["BURUNDI"] = "Burundi";
	m_countrymap["CHINA"] = "China";
	m_countrymap["AFGHANISTAN"] = "Afghanistan";
	m_countrymap["BHUTAN"] = "Bhutan";
	m_countrymap["CHILE"] = "Chile";
	m_countrymap["CAYMANISLANDS"] = "Cayman Islands";
	m_countrymap["CAMEROON"] = "Cameroon";
	m_countrymap["CHAD"] = "Chad";
	m_countrymap["COMOROS"] = "Comoros";
	m_countrymap["COLOMBIA"] = "Colombia";
	m_countrymap["COSTARICA"] = "Costa Rica";
	m_countrymap["CENTRALAFRICANREPUBLIC"] = "Central African Republic";
	m_countrymap["CUBA"] = "Cuba";
	m_countrymap["CAPEVERDE"] = "Cape Verde";
	m_countrymap["COOKISLANDS"] = "Cook Islands";
	m_countrymap["CYPRUS"] = "Cyprus";
	m_countrymap["DENMARK"] = "Denmark";
	m_countrymap["DJIBOUTI"] = "Djibouti";
	m_countrymap["DOMINICA"] = "Dominica";
	m_countrymap["DOMINICANREPUBLIC"] = "Dominican Republic";
	m_countrymap["ECUADOR"] = "Ecuador";
	m_countrymap["EGYPT"] = "Egypt";
	m_countrymap["IRELAND"] = "Ireland";
	m_countrymap["EQUATORIALGUINEA"] = "Equatorial Guinea";
	m_countrymap["ESTONIA"] = "Estonia";
	m_countrymap["ERITREA"] = "Eritrea";
	m_countrymap["ELSALVADOR"] = "El Salvador";
	m_countrymap["ETHIOPIA"] = "Ethiopia";
	m_countrymap["AUSTRIA"] = "Austria";
	m_countrymap["CZECHREPUBLIC"] = "Czech Republic";
	m_countrymap["FRENCH GUYANA"] = "French Guiana";
	m_countrymap["FINLAND"] = "Finland";
	m_countrymap["FIJI"] = "Fiji";
	m_countrymap["FALKLANDSLANDS"] = "Falkland Islands (Malvinas)";
	m_countrymap["MICRONESIA"] = "Micronesia, Federated States of";
	m_countrymap["FRENCHPOLYNESIA"] = "French Polynesia";
	m_countrymap["FRANCE"] = "France";
	m_countrymap["GAMBIA"] = "Gambia";
	m_countrymap["GABON"] = "Gabon";
	m_countrymap["GEORGIA"] = "Georgia";
	m_countrymap["GHANA"] = "Ghana";
	m_countrymap["GRENADA"] = "Grenada";
	m_countrymap["GREENLAND"] = "Greenland";
	m_countrymap["GERMANY"] = "Germany";
	m_countrymap["GUAM"] = "Guam";
	m_countrymap["GREECE"] = "Greece";
	m_countrymap["GUATEMALA"] = "Guatemala";
	m_countrymap["GUINEA"] = "Guinea";
	m_countrymap["GUYANA"] = "Guyana";
	m_countrymap["HAITI"] = "Haiti";
	m_countrymap["HONDURAS"] = "Honduras";
	m_countrymap["CROATIA"] = "Croatia";
	m_countrymap["HUNGARY"] = "Hungary";
	m_countrymap["ICELAND"] = "Iceland";
	m_countrymap["INDIA"] = "India";
	m_countrymap["IRAN"] = "Iran (Islamic Republic of)";
	m_countrymap["IRAN_WEST"] = "Iran (Islamic Republic of)";
	m_countrymap["ISRAEL"] = "Israel";
	m_countrymap["ITALY"] = "Italy";
	m_countrymap["COTEDIVOIRE"] = "Cote d'Ivoire";
	m_countrymap["IRAQ"] = "Iraq";
	m_countrymap["JAPAN"] = "Japan";
	m_countrymap["JAMAICA"] = "Jamaica";
	m_countrymap["JORDAN"] = "Jordan";
	m_countrymap["KENYA"] = "Kenya";
	m_countrymap["KYRGYZSTAN"] = "Kyrgyzstan";
	m_countrymap["DEMOCRATICKOREA"] = "Korea, Democratic People's Republic of";
	m_countrymap["KIRIBATI"] = "Kiribati";
	m_countrymap["REPUBLICOFKOREA"] = "Korea, Republic of";
	m_countrymap["KUWAIT"] = "Kuwait";
	m_countrymap["KAZAKHSTAN"] = "Kazakhstan";
	m_countrymap["LAO"] = "Lao People's Democratic Republic";
	m_countrymap["LEBANON"] = "Lebanon";
	m_countrymap["LATVIA"] = "Latvia";
	m_countrymap["BELARUS"] = "Belarus";
	m_countrymap["LITHUANIA"] = "Lithuania";
	m_countrymap["LIBERIA"] = "Liberia";
	m_countrymap["SLOVAKREPUBLIC"] = "Slovakia";
	m_countrymap["LIECHTENSTEIN"] = "Liechtenstein";
	m_countrymap["LIBYANARABJAMAHIRIYA"] = "Libyan Arab Jamahiriya";
	m_countrymap["MADAGASCAR"] = "Madagascar";
	m_countrymap["MARTINIQUE"] = "Martinique";
	m_countrymap["MONGOLIA"] = "Mongolia";
	m_countrymap["MONTSERRAT"] = "Montserrat";
	m_countrymap["MACEDONIA"] = "The former Yugoslav Republic of Macedonia";
	m_countrymap["FYROM"] = "The former Yugoslav Republic of Macedonia";
	m_countrymap["MALI"] = "Mali";
	m_countrymap["MOROCCO"] = "Morocco";
	m_countrymap["MAURITIUS"] = "Mauritius";
	m_countrymap["MAURITANIA"] = "Mauritania";
	m_countrymap["MALTA"] = "Malta";
	m_countrymap["OMAN"] = "Oman";
	m_countrymap["MALDIVES"] = "Maldives";
	m_countrymap["MEXICO"] = "Mexico";
	m_countrymap["MALAYSIA"] = "Malaysia";
	m_countrymap["MOZAMBIQUE"] = "Mozambique";
	m_countrymap["MALAWI"] = "Malawi";
	m_countrymap["NEWCALEDONIA"] = "New Caledonia";
	m_countrymap["NIUE"] = "Niue";
	m_countrymap["NIGER"] = "Niger";
	m_countrymap["ARUBA"] = "Aruba";
	m_countrymap["ANGUILLA"] = "Anguilla";
	m_countrymap["BELGIUM"] = "Belgium";
	m_countrymap["BELGIAN COASTLINE"] = "Belgium";
	m_countrymap["HONGKONG"] = "Hong Kong";
	m_countrymap["NORTHERNMARIANAISLANDS"] = "Northern Mariana Islands";
	m_countrymap["FAROEISLANDS"] = "Faroe Islands";
	m_countrymap["ANDORRA"] = "Andorra";
	m_countrymap["GIBRALTAR"] = "Gibraltar";
	m_countrymap["ISLEOFMAN"] = "Isle of Man";
	m_countrymap["LUXEMBOURG"] = "Luxembourg";
	m_countrymap["MACAU"] = "Macau";
	m_countrymap["MONACO"] = "Monaco";
	m_countrymap["PALESTINE"] = "Palestine";
	m_countrymap["MONTENEGRO"] = "Montenegro"; // In AIXM is together with Serbia
	m_countrymap["MAYOTTE"] = "Mayotte";
	//m_countrymap["LLANDISLANDS"] = "Lland Islands";
	m_countrymap["NORFOLKISLAND"] = "Norfolk Island";
	m_countrymap["COCOSISLANDS"] = "Cocos (Keeling) Islands";
	m_countrymap["ANTARCTICA"] = "Antarctica";
	m_countrymap["BOUVETISLAND"] = "Bouvet Island";
	m_countrymap["FRENCHSOUTHERNANDANTARCTICLANDS"] = "French Southern and Antarctic Lands";
	m_countrymap["HEARDISLANDANDMCDONALDISLANDS"] = "Heard Island and McDonald Islands";
	m_countrymap["BRITISHINDIANOCEANTERRITORY"] = "British Indian Ocean Territory";
	m_countrymap["CHRISTMASISLAND"] = "Christmas Island";
	m_countrymap["UNITEDSTATESMINOROUTLYINGISLANDS"] = "United States Minor Outlying Islands";
	m_countrymap["VANUATU"] = "Vanuatu";
	m_countrymap["NIGERIA"] = "Nigeria";
	m_countrymap["NETHERLANDS"] = "Netherlands";
	m_countrymap["NORWAY"] = "Norway";
	m_countrymap["NEPAL"] = "Nepal";
	m_countrymap["NAURU"] = "Nauru";
	m_countrymap["SURINAME"] = "Suriname";
	m_countrymap["NICARAGUA"] = "Nicaragua";
	m_countrymap["NEWZEALAND"] = "New Zealand";
	m_countrymap["PARAGUAY"] = "Paraguay";
	m_countrymap["PERU"] = "Peru";
	m_countrymap["PAKISTAN"] = "Pakistan";
	m_countrymap["POLAND"] = "Poland";
	m_countrymap["PANAMA"] = "Panama";
	m_countrymap["PORTUGAL"] = "Portugal";
	m_countrymap["PAPUANEWGUINEA"] = "Papua New Guinea";
	m_countrymap["GUINEABISSAU"] = "Guinea-Bissau";
	m_countrymap["QATAR"] = "Qatar";
	m_countrymap["REUNION"] = "Reunion";
	m_countrymap["ROMANIA"] = "Romania";
	m_countrymap["MOLDOVA"] = "Republic of Moldova";
	m_countrymap["PHILIPPINES"] = "Philippines";
	m_countrymap["PUERTORICO"] = "Puerto Rico";
	m_countrymap["RUSSIA"] = "Russia";
	m_countrymap["RWANDA"] = "Rwanda";
	m_countrymap["SAUDIARABIA"] = "Saudi Arabia";
	m_countrymap["SAINTKITTSANDNEVIS"] = "Saint Kitts and Nevis";
	m_countrymap["SEYCHELLES"] = "Seychelles";
	m_countrymap["SOUTHAFRICA"] = "South Africa";
	m_countrymap["LESOTHO"] = "Lesotho";
	m_countrymap["BOTSWANA"] = "Botswana";
	m_countrymap["SENEGAL"] = "Senegal";
	m_countrymap["SLOVENIA"] = "Slovenia";
	m_countrymap["SIERRALEONE"] = "Sierra Leone";
	m_countrymap["SINGAPORE"] = "Singapore";
	m_countrymap["SOMALIA"] = "Somalia";
	m_countrymap["SPAIN"] = "Spain";
	m_countrymap["SPAIN_WEST"] = "Spain";
	m_countrymap["SPAIN_EAST"] = "Spain";
	m_countrymap["SAINTLUCIA"] = "Saint Lucia";
	m_countrymap["SUDAN"] = "Sudan";
	m_countrymap["SWEDEN"] = "Sweden";
	m_countrymap["SYRIA"] = "Syrian Arab Republic";
	m_countrymap["SWITZERLAND"] = "Switzerland";
	m_countrymap["TRINIDADANDTOBAGO"] = "Trinidad and Tobago";
	m_countrymap["THAILAND"] = "Thailand";
	m_countrymap["TAJIKISTAN"] = "Tajikistan";
	m_countrymap["TOKELAU"] = "Tokelau";
	m_countrymap["TONGA"] = "Tonga";
	m_countrymap["TOGO"] = "Togo";
	m_countrymap["SAOTOMEANDPRINCIPE"] = "Sao Tome and Principe";
	m_countrymap["TUNISIA"] = "Tunisia";
	m_countrymap["TURKEY"] = "Turkey";
	m_countrymap["TURKEY_SOUTH"] = "Turkey";
	m_countrymap["TURKEY_NORTH"] = "Turkey";
	m_countrymap["TUVALU"] = "Tuvalu";
	m_countrymap["TURKMENISTAN"] = "Turkmenistan";
	m_countrymap["UNITEDREPUBLICOFTANZANIA"] = "United Republic of Tanzania";
	m_countrymap["UGANDA"] = "Uganda";
	m_countrymap["UNITEDKINGDOM"] = "United Kingdom";
	m_countrymap["UKRAINE"] = "Ukraine";
	m_countrymap["UKRAINE_NORTH"] = "Ukraine";
	m_countrymap["UKRAINE_EAST"] = "Ukraine";
	m_countrymap["UNITEDSTATESOFAMERICA"] = "United States";
	m_countrymap["BURKINAFASO"] = "Burkina Faso";
	m_countrymap["URUGUAY"] = "Uruguay";
	m_countrymap["UZBEKISTAN"] = "Uzbekistan";
	m_countrymap["SAINTVINCENTANDTHEGRENADINES"] = "Saint Vincent and the Grenadines";
	m_countrymap["VENEZUELA"] = "Venezuela";
	m_countrymap["BRITISHVIRGINISLANDS"] = "British Virgin Islands";
	m_countrymap["VIETNAM"] = "Viet Nam";
	m_countrymap["UNITEDSTATESVIRGINISLANDS"] = "United States Virgin Islands";
	m_countrymap["NAMIBIA"] = "Namibia";
	m_countrymap["WALLISANDFUTUNAISLANDS"] = "Wallis and Futuna Islands";
	m_countrymap["SAMOA"] = "Samoa";
	m_countrymap["SWAZILAND"] = "Swaziland";
	m_countrymap["YEMEN"] = "Yemen";
	m_countrymap["ZAMBIA"] = "Zambia";
	m_countrymap["ZIMBABWE"] = "Zimbabwe";
	m_countrymap["INDONESIA"] = "Indonesia";
	m_countrymap["GUADELOUPE"] = "Guadeloupe";
	m_countrymap["NETHERLANDSANTILLES"] = "Netherlands Antilles";
	m_countrymap["UNITEDARABEMIRATES"] = "United Arab Emirates";
	m_countrymap["TIMORLESTE"] = "Timor-Leste";
	m_countrymap["PITCAIRNISLANDS"] = "Pitcairn Islands";
	m_countrymap["PALAU"] = "Palau";
	m_countrymap["MARSHALLISLANDS"] = "Marshall Islands";
	m_countrymap["SAINTPIERREANDMIQUELON"] = "Saint Pierre and Miquelon";
	m_countrymap["SAINTHELENA"] = "Saint Helena";
	m_countrymap["SANMARINO"] = "San Marino";
	m_countrymap["TURKSANDCAICOSISLANDS"] = "Turks and Caicos Islands";
	m_countrymap["WESTERNSAHARA"] = "Western Sahara";
	m_countrymap["SERBIAANDMONTENEGRO"] = "Serbia"; // In AIXM is together with Montenegro
	m_countrymap["SERBIAANDMONTENEGRO_NORTH"] = "Serbia";
	m_countrymap["SERBIAANDMONTENEGRO_SOUTH"] = "Serbia";
	m_countrymap["HOLYSEE"] = "Holy See (Vatican City)";
	m_countrymap["SVALBARD"] = "Svalbard";
	m_countrymap["SAINTMARTIN"] = "Saint Martin";
	m_countrymap["SAINTBARTHELEMY"] = "Saint Barthelemy";
	m_countrymap["GUERNSEY"] = "Guernsey";
	m_countrymap["JERSEY"] = "Jersey";
	m_countrymap["TAIWAN"] = "Taiwan";
	m_countrymap["SOUTHGEORGIASOUTHSANDWICHISLANDS"] = "South Georgia South Sandwich Islands";
}

DbXmlImporter::ForEachAirspace::~ForEachAirspace()
{
}

void DbXmlImporter::ForEachAirspace::loadborders(const Glib::ustring& bordersfile)
{
#if defined(HAVE_GDAL)
#if defined(HAVE_GDAL2)
	GDALDataset *ds((GDALDataset*)GDALOpenEx(bordersfile.c_str(), GDAL_OF_READONLY, 0, 0, 0));
#else
	OGRSFDriver *drv(0);
	OGRDataSource *ds(OGRSFDriverRegistrar::Open(bordersfile.c_str(), false, &drv));
#endif
	if (!ds) {
		std::cerr << "Cannot open borders file " << bordersfile << std::endl;
		return;
	}
	for (int layernr = 0; layernr < ds->GetLayerCount(); ++layernr) {
		OGRLayer *layer(ds->GetLayer(layernr));
		OGRFeatureDefn *layerdef(layer->GetLayerDefn());
		std::cerr << "  Layer Name: " << layerdef->GetName() << std::endl;
		if (false) {
			for (int fieldnr = 0; fieldnr < layerdef->GetFieldCount(); ++fieldnr) {
				OGRFieldDefn *fielddef(layerdef->GetFieldDefn(fieldnr));
				std::cerr << "    Field Name: " << fielddef->GetNameRef()
					  << " type " << OGRFieldDefn::GetFieldTypeName(fielddef->GetType())  << std::endl;
			}
		}
		int name_index(layerdef->GetFieldIndex("NAME"));
		if (name_index == -1)
			std::cerr << "Layer Name: " << layerdef->GetName() << " has no NAME field, skipping" << std::endl;
		layer->ResetReading();
		while (OGRFeature *feature = layer->GetNextFeature()) {
			std::cerr << "  Feature: " << feature->GetFID() << std::endl;
			if (false) {
				for (int fieldnr = 0; fieldnr < layerdef->GetFieldCount(); ++fieldnr) {
					OGRFieldDefn *fielddef(layerdef->GetFieldDefn(fieldnr));
					std::cerr << "    Field Name: " << fielddef->GetNameRef() << " Value ";
					switch (fielddef->GetType()) {
					case OFTInteger:
						std::cerr << feature->GetFieldAsInteger(fieldnr);
						break;

					case OFTReal:
						std::cerr << feature->GetFieldAsDouble(fieldnr);
						break;

					case OFTString:
					default:
						std::cerr << feature->GetFieldAsString(fieldnr);
						break;
					}
					std::cerr << std::endl;
				}
				OGRGeometry *geom(feature->GetGeometryRef());
				char *wkt;
				geom->exportToWkt(&wkt);
				std::cerr << "  Geom " << geom->getGeometryName() << ": " << wkt << std::endl;
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
			if (false) {
				countrymap_t::const_iterator i(m_countrymap.find(nameu));
				if (i != m_countrymap.end()) {
					std::cerr << "Remapping \"" << nameu << "\" to \"" << i->second << "\"" << std::endl;
					nameu = i->second;
				}
			}
			OGRGeometry *geom(feature->GetGeometryRef());
			if (false)
				std::cerr << "Layer Name: " << layerdef->GetName()
					  << " Feature: " << feature->GetFID()
					  << " Geom: " << geom->getGeometryName() << std::endl;
			loadborder(nameu, geom, layerdef, feature);
			OGRFeature::DestroyFeature(feature);
			feature = 0;
		}
	}
#if defined(HAVE_GDAL2)
	GDALClose(ds);
#else
	OGRDataSource::DestroyDataSource(ds);
#endif
	if (true) {
		std::cerr << "Borders file " << bordersfile << " Countries: ";
		bool subseq(false);
		for (countries_t::const_iterator ci(m_countries.begin()), ce(m_countries.end()); ci != ce; ++ci) {
			if (subseq)
				std::cerr << ", ";
			subseq = true;
			std::cerr << "\"" << ci->first << "\"";
		}
		std::cerr << std::endl;
	}
#else
	std::cerr << "Cannot process borders file " << bordersfile << std::endl;
#endif
}

#ifdef HAVE_GDAL
void DbXmlImporter::ForEachAirspace::loadborder(const std::string& country, OGRGeometry *geom, OGRFeatureDefn *layerdef, OGRFeature *feature)
{
	if (!geom)
		return;
	switch (geom->getGeometryType()) {
	case wkbPolygon:
	case wkbPolygon25D:
	{
		OGRPolygon *poly(dynamic_cast<OGRPolygon *>(geom));
		if (!poly) {
			if (layerdef && feature)
				std::cerr << "Layer Name: " << layerdef->GetName() << " Feature: " << feature->GetFID()
					  << " Geom: " << geom->getGeometryName()
					  << " cannot convert to polygon, skipping" << std::endl;
			break;
		}
		loadborder(country, poly, layerdef, feature);
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
			loadborder(country, geom, layerdef, feature);
			break;
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
		loadborder(country, geom1, layerdef, feature);
		delete geom1;
		break;
	}
}

void DbXmlImporter::ForEachAirspace::loadborder(const std::string& country, OGRPolygon *poly, OGRFeatureDefn *layerdef, OGRFeature *feature)
{
	if (!poly)
		return;
       	loadborder(country, poly->getExteriorRing(), layerdef, feature);
	for (int i = 0, j = poly->getNumInteriorRings(); i < j; ++i)
		loadborder(country, poly->getInteriorRing(i), layerdef, feature);
}

void DbXmlImporter::ForEachAirspace::loadborder(const std::string& country, OGRLinearRing *ring, OGRFeatureDefn *layerdef, OGRFeature *feature)
{
	if (!ring)
		return;
	PolygonSimple poly;
	for (int ii = 0, ie = ring->getNumPoints(); ii < ie; ++ii) {
		Point pt;
		pt.set_lon_deg_dbl(ring->getX(ii));
		pt.set_lat_deg_dbl(ring->getY(ii));
		poly.push_back(pt);
	}
	poly.remove_redundant_polypoints();
	std::pair<countries_t::iterator,bool> ins(m_countries.insert(std::make_pair(country, polygons_t())));
	ins.first->second.push_back(PolygonSimple());
	ins.first->second.back().swap(poly);
}
#endif

bool DbXmlImporter::ForEachAirspace::operator()(const AirspacesDb::Airspace& e)
{
	if (e.get_typecode() != AirspacesDb::Airspace::typecode_ead)
		return true;
	if (false && e.get_modtime() < m_starttime)
		return true;
	m_records.push(e.get_address());
	return true;
}

void DbXmlImporter::ForEachAirspace::process(AirspacesDb& airspacesdb)
{
	while (!m_records.empty()) {
		AirspacesDb::Airspace e(airspacesdb(m_records.top()));
		m_records.pop();
		if (!e.is_valid())
			continue;
		result_t r(process_one(e));
		switch (r) {
		case result_save:
			e.recompute_lineapprox(airspacesdb);
			e.recompute_bbox();
			e.compute_initial_label_placement();
			airspacesdb.save(e);
			break;

		case result_erase:
			airspacesdb.erase(e);
			break;

		case result_nochange:
			break;

		default:
			break;
		} 
	}
}

DbXmlImporter::ForEachAirspace::FindNearest::FindNearest(const Point& p1, const Point& p2)
	: m_p1(p1), m_p2(p2), m_dist(std::numeric_limits<uint64_t>::max()), m_idx1(~0U), m_idx2(~0U)
{
}

void DbXmlImporter::ForEachAirspace::FindNearest::process(const polygons_t& polys)
{
	for (polygons_t::const_iterator pi(polys.begin()), pe(polys.end()); pi != pe; ++pi)
		process(*pi);
}

void DbXmlImporter::ForEachAirspace::FindNearest::process(const PolygonSimple& poly)
{
	uint64_t dist1(std::numeric_limits<uint64_t>::max());
	unsigned int idx1(~0U);
	PolygonSimple poly1;
	{
		Point pt;
		pt.set_invalid();
		for (unsigned int ii = 0, k = poly.size(), jj = k - 1; ii < k; jj = ii, ++ii) {
			Point ppt1(poly[jj]), ppt2(poly[ii]);
			float angle(ppt1.simple_true_course(ppt2) + 90.0);
			{
				bool ok(true);
				Point ptx;
				if (false) {
					ptx = m_p1.simple_course_distance_nmi(angle, 1.f);
					ok = ptx.intersection(ppt1, ppt2, m_p1, ptx);
				} else {
					ptx = m_p1.spheric_nearest_on_line(ppt1, ppt2);
				}
				ok = ok && !ptx.is_invalid() && ptx.in_box(ppt1, ppt2);
				if (ok) {
					uint64_t d(m_p1.simple_distance_rel(ptx));
					if (d < dist1) {
						pt = ptx;
						dist1 = d;
						idx1 = ii;
					}
				}
				ptx = m_p1.nearest_on_line(ppt1, ppt2);
				ok = !ptx.is_invalid() && ptx.in_box(ppt1, ppt2);
				if (ok) {
					uint64_t d(m_p1.simple_distance_rel(ptx));
					if (d < dist1) {
						pt = ptx;
						dist1 = d;
						idx1 = ii;
					}
				}
				uint64_t d(m_p1.simple_distance_rel(ppt2));
				if (d < dist1) {
					pt = ppt2;
					dist1 = d;
					idx1 = ii;
				}
			}
		}
		if (dist1 > m_dist || pt.is_invalid() || idx1 >= poly.size())
			return;
		poly1 = poly;
		if (poly[idx1] != pt)
			poly1.insert(poly1.begin() + idx1, pt);
	}
	uint64_t dist2(std::numeric_limits<uint64_t>::max());
	unsigned int idx2(~0U);
	{
		Point pt;
		pt.set_invalid();
		for (unsigned int ii = 0, k = poly1.size(), jj = k - 1; ii < k; jj = ii, ++ii) {
			Point ppt1(poly1[jj]), ppt2(poly1[ii]);
			float angle(ppt1.simple_true_course(ppt2) + 90.0);
			{
				bool ok(true);
				Point ptx;
				if (false) {
					ptx = m_p2.simple_course_distance_nmi(angle, 1.f);
					ok = ptx.intersection(ppt1, ppt2, m_p2, ptx);
				} else {
					ptx = m_p2.spheric_nearest_on_line(ppt1, ppt2);
				}
				ok = ok && !ptx.is_invalid() && ptx.in_box(ppt1, ppt2);
				if (ok) {
					uint64_t d(m_p2.simple_distance_rel(ptx));
					if (d < dist2) {
						pt = ptx;
						dist2 = d;
						idx2 = ii;
					}
				}
				ptx = m_p2.nearest_on_line(ppt1, ppt2);
				ok = !ptx.is_invalid() && ptx.in_box(ppt1, ppt2);
				if (ok) {
					uint64_t d(m_p2.simple_distance_rel(ptx));
					if (d < dist2) {
						pt = ptx;
						dist2 = d;
						idx2 = ii;
					}
				}
				uint64_t d(m_p2.simple_distance_rel(ppt2));
				if (d < dist2) {
					pt = ppt2;
					dist2 = d;
					idx2 = ii;
				}
			}
		}
		if (dist2 > m_dist || pt.is_invalid() || idx2 >= poly1.size())
			return;
		if (poly1[idx2] != pt)
			poly1.insert(poly1.begin() + idx2, pt);
	}
	// use the shorter of the two polygon orientations
	{
		uint64_t ctot(poly1.simple_circumference_rel());
		uint64_t c1(0);
		for (unsigned int i = idx1; i != idx2; ) {
			unsigned int j(poly1.next(i));
			c1 += poly1[i].simple_distance_rel(poly1[j]);
			i = j;
		}
		if (c1 * 2 > ctot) {
			poly1.reverse();
			idx1 = poly1.size() - 1 - idx1;
			idx2 = poly1.size() - 1 - idx2;
		}
	}
	m_dist = std::max(dist1, dist2);
	m_idx1 = idx1;
	m_idx2 = idx2;
	m_poly.swap(poly1);
}

DbXmlImporter::ForEachAirspace::result_t DbXmlImporter::ForEachAirspace::process_one(AirspacesDb::Airspace& e)
{
       	bool hasfnt(false);
	{
		unsigned int nrseg(e.get_nrsegments());
		for (unsigned int i = 0; i < nrseg;) {
			AirspacesDb::Airspace::Segment& seg(e[i]);
			if ((seg.get_shape() == 'B' || seg.get_shape() == 'H' || seg.get_shape() == 'G' || seg.get_shape() == '-') &&
			    seg.get_coord1() == seg.get_coord2()) {
				e.erase_segment(i);
				--nrseg;
				continue;
			}
			hasfnt = hasfnt || seg.get_shape() == 'F';
			++i;
		}
		if (!nrseg && !e.get_nrcomponents()) {
			std::cerr << "Airspace " << e.get_icao_name() << " has no segments, deleting" << std::endl;
			return result_erase;
		}
		if (!nrseg && e.get_polygon().empty()) {
			std::cerr << "Airspace " << e.get_icao_name() << " has empty polygon, deleting" << std::endl;
			return result_erase;
		}
	}
	if (!hasfnt)
		return result_nochange;
	hasfnt = false;
	std::string countrynames(e.get_classexcept().uppercase());
	std::string newcountrynames;
	for (unsigned int i = 0; i < e.get_nrsegments(); ++i) {
		AirspacesDb::Airspace::Segment& seg(e[i]);
		if (seg.get_shape() != 'F')
			continue;
		hasfnt = true;
		if (m_countries.empty())
			break;
		FindNearest f(seg.get_coord1(), seg.get_coord2());
		std::string curcountrynames;
		{
			std::string::size_type n(countrynames.find(';'));
			if (n == std::string::npos) {
				std::cerr << "WARNING: Airspace " << e.get_icao_name() << " has no country names \""
					  << countrynames << "\"" << std::endl;
				continue;
			}
			curcountrynames = countrynames.substr(0, n);
			countrynames.erase(0, n + 1);
		}
		std::string country0(curcountrynames), country1;
		{
		        std::string::size_type n(country0.find_first_of("-_:"));
			if (n != std::string::npos) {
				country1 = country0.substr(n + 1);
				country0.erase(n);
			}
			if (false)
				std::cerr << "Countries: \"" << country0 << "\", \"" << country1
					  << "\" old \"" << curcountrynames << ';' << countrynames << "\" new \""
					  << countrynames << "\"" << std::endl;
		}
		{
			countrymap_t::const_iterator mi(m_countrymap.find(country0));
			countries_t::const_iterator i(m_countries.find((mi == m_countrymap.end()) ? country0 : mi->second));
			if (i == m_countries.end())
				std::cerr << "WARNING: Airspace " << e.get_icao_name() << " Country "
					  << country0 << " not found in borders file" << std::endl;
			else
				f.process(i->second);
			mi = m_countrymap.find(country1);
			i = m_countries.find((mi == m_countrymap.end()) ? country1 : mi->second);
			if (i == m_countries.end())
				std::cerr << "WARNING: Airspace " << e.get_icao_name() << " Country "
					  << country1 << " not found in borders file" << std::endl;
			else
				f.process(i->second);
		}
		if (f.get_idx1() >= f.get_poly().size() || f.get_idx2() >= f.get_poly().size()) {
			std::cerr << "WARNING: Airspace " << e.get_icao_name() << " Countries " << country0
				  << '/' << country1 << " Border not found" << std::endl;
			newcountrynames += curcountrynames + ";";
			continue;
		}
		{
			double dist1(seg.get_coord1().spheric_distance_nmi_dbl(f.get_poly()[f.get_idx1()]));
			double dist2(seg.get_coord2().spheric_distance_nmi_dbl(f.get_poly()[f.get_idx2()]));
			std::cerr << "Border: " << country0 << '/' << country1 << " dist " << f.get_dist() << " points "
				  << seg.get_coord1() << ' ' << dist1 << "nm " << f.get_poly()[f.get_idx1()] << " - "
				  << f.get_poly()[f.get_idx2()] << ' ' << dist2 << "nm " << seg.get_coord2() << std::endl;
			if (dist1 > 10 || dist2 > 10) {
				std::cerr << "WARNING: Airspace " << e.get_icao_name()
					  << " Distance to border too big, skipping" << std::endl;
				newcountrynames += curcountrynames + ";";
				continue;
			}
		}
		e.insert_segment(AirspacesDb::Airspace::Segment(f.get_poly()[f.get_idx2()], seg.get_coord2(), Point(), '-', 0, 0), i + 1);
		// look up again after insert, may be remapped
		{
			AirspacesDb::Airspace::Segment& seg(e[i]);
			seg.set_coord2(f.get_poly()[f.get_idx1()]);
			seg.set_coord0(Point());
			seg.set_shape('-');
			seg.set_radius1(0);
			seg.set_radius2(0);
		}
		for (unsigned int j = f.get_idx1(); j != f.get_idx2();) {
			Point p1(f.get_poly()[j]);
			j = f.get_poly().next(j);
			Point p2(f.get_poly()[j]);
			if (p1 != p2) {
				++i;
				e.insert_segment(AirspacesDb::Airspace::Segment(p1, p2, Point(), '-', 0, 0), i);
			}
		}
		hasfnt = false;
	}
	e.set_classexcept(newcountrynames);
	if (hasfnt)
		std::cerr << "WARNING: Airspace " << e.get_icao_name() << " has unresolvable boundary segment" << std::endl;
	return result_save;
}

bool DbXmlImporter::ForEachAirspace::operator()(const std::string& id)
{
	return true;
}

void DbXmlImporter::finalize_airspaces(void)
{
	if (false && !m_finalizeairspaces)
		return;
	m_finalizeairspaces = false;
	open_airspaces_db();
	ForEachAirspace foreach(m_starttime);
	foreach.loadborders(m_bordersfile);
	m_airspacesdb.for_each(foreach, false, AirspacesDb::Airspace::subtables_all);
	foreach.process(m_airspacesdb);
        if (m_airspacesdbopen)
                m_airspacesdb.close();
        m_airspacesdbopen = false;	
}


DbXmlImporter::RecomputeAuthority::RecomputeAuthority(const Glib::ustring& output_dir)
{
        m_airportsdb.open(output_dir);
        m_airportsdb.sync_off();
        m_navaidsdb.open(output_dir);
        m_navaidsdb.sync_off();
        m_waypointsdb.open(output_dir);
        m_waypointsdb.sync_off();
	{
		sqlite3x::sqlite3_command cmd(m_airportsdb.get_db(), "UPDATE airports SET AUTHORITY=\"\";");
	}
	{
		sqlite3x::sqlite3_command cmd(m_navaidsdb.get_db(), "UPDATE navaids SET AUTHORITY=\"\";");
	}
	{
		sqlite3x::sqlite3_command cmd(m_waypointsdb.get_db(), "UPDATE waypoints SET AUTHORITY=\"\";");
	}
}

DbXmlImporter::RecomputeAuthority::~RecomputeAuthority()
{
	m_airportsdb.close();
	m_navaidsdb.close();
	m_waypointsdb.close();
}

bool DbXmlImporter::RecomputeAuthority::operator()(const AirspacesDb::Airspace& e)
{
	if (!e.is_valid() || e.get_typecode() != AirspacesDb::Airspace::typecode_ead ||
	    (e.get_bdryclass() != AirspacesDb::Airspace::bdryclass_ead_ead &&
	     e.get_bdryclass() != AirspacesDb::Airspace::bdryclass_ead_nas) ||
	    e.get_icao().empty())
		return true;
	Rect bbox(e.get_bbox());
	const MultiPolygonHole& poly(e.get_polygon());
	unsigned int nrairport(0), nrnavaid(0), nrwaypoint(0);
	{
		AirportsDb::elementvector_t elv(m_airportsdb.find_by_rect(bbox, ~0, AirportsDb::Airport::subtables_all));
		for (AirportsDb::elementvector_t::iterator ei(elv.begin()), ee(elv.end()); ei != ee; ++ei) {
			AirportsDb::element_t& el(*ei);
			if (!bbox.is_inside(el.get_coord()) || !poly.windingnumber(el.get_coord()))
				continue;
			el.add_authority(e.get_icao());
			//el.set_modtime(time(0));
			m_airportsdb.save(el);
			++nrairport;
			if (false)
				std::cerr << "Saving airport " << el.get_icao_name() << " authority " << el.get_authority() << std::endl;
		}
	}
	{
		NavaidsDb::elementvector_t elv(m_navaidsdb.find_by_rect(bbox, ~0, NavaidsDb::Navaid::subtables_all));
		for (NavaidsDb::elementvector_t::iterator ei(elv.begin()), ee(elv.end()); ei != ee; ++ei) {
			NavaidsDb::element_t& el(*ei);
			if (!bbox.is_inside(el.get_coord()) || !poly.windingnumber(el.get_coord()))
				continue;
			el.add_authority(e.get_icao());
			//el.set_modtime(time(0));
			m_navaidsdb.save(el);
			++nrnavaid;
			if (false)
				std::cerr << "Saving navaid " << el.get_icao_name() << " authority " << el.get_authority() << std::endl;
		}
	}
	{
		WaypointsDb::elementvector_t elv(m_waypointsdb.find_by_rect(bbox, ~0, WaypointsDb::Waypoint::subtables_all));
		for (WaypointsDb::elementvector_t::iterator ei(elv.begin()), ee(elv.end()); ei != ee; ++ei) {
			WaypointsDb::element_t& el(*ei);
			if (!bbox.is_inside(el.get_coord()) || !poly.windingnumber(el.get_coord()))
				continue;
			el.add_authority(e.get_icao());
			//el.set_modtime(time(0));
			m_waypointsdb.save(el);
			++nrwaypoint;
			if (false)
				std::cerr << "Saving intersection " << el.get_name() << " authority " << el.get_authority() << std::endl;
		}
	}
	std::cerr << "Airspace " << e.get_icao_name() << " has " << nrairport << " airports, " << nrnavaid << " navaids, "
		  << nrwaypoint << " intersections" << std::endl;
	return true;
}

bool DbXmlImporter::RecomputeAuthority::operator()(const std::string& id)
{
	return true;
}

void DbXmlImporter::update_authority(void)
{
        if (m_airportsdbopen)
                m_airportsdb.close();
        m_airportsdbopen = false;	
        if (m_navaidsdbopen)
                m_navaidsdb.close();
        m_navaidsdbopen = false;	
        if (m_waypointsdbopen)
                m_waypointsdb.close();
        m_waypointsdbopen = false;
	open_airspaces_db();
	{
		RecomputeAuthority authupdate(m_outputdir);
		m_airspacesdb.for_each(authupdate, false, AirspacesDb::Airspace::subtables_all);
	}
        if (m_airspacesdbopen)
                m_airspacesdb.close();
        m_airspacesdbopen = false;	
}

void DbXmlImporter::open_airports_db(void)
{
        if (m_airportsdbopen)
                return;
        m_airportsdb.open(m_outputdir);
        m_airportsdb.sync_off();
        if (m_purgedb)
                m_airportsdb.purgedb();
        m_airportsdbopen = true;
}

void DbXmlImporter::open_airspaces_db(void)
{
        if (m_airspacesdbopen)
                return;
        m_airspacesdb.open(m_outputdir);
        m_airspacesdb.sync_off();
        if (m_purgedb)
                m_airspacesdb.purgedb();
        m_airspacesdbopen = true;
}

void DbXmlImporter::open_navaids_db(void)
{
        if (m_navaidsdbopen)
                return;
        m_navaidsdb.open(m_outputdir);
        m_navaidsdb.sync_off();
        if (m_purgedb)
                m_navaidsdb.purgedb();
        m_navaidsdbopen = true;
}

void DbXmlImporter::open_waypoints_db(void)
{
        if (m_waypointsdbopen)
                return;
        m_waypointsdb.open(m_outputdir);
        m_waypointsdb.sync_off();
        if (m_purgedb)
                m_waypointsdb.purgedb();
        m_waypointsdbopen = true;
}

void DbXmlImporter::open_airways_db(void)
{
        if (m_airwaysdbopen)
                return;
        m_airwaysdb.open(m_outputdir);
        m_airwaysdb.sync_off();
        if (m_purgedb)
                m_airwaysdb.purgedb();
        m_airwaysdbopen = true;
}

void DbXmlImporter::open_topo_db(void)
{
        if (m_topodbopen)
                return;
        m_topodb.open(m_outputdir);
        m_topodbopen = true;
}

int main(int argc, char *argv[])
{
        static struct option long_options[] = {
                { "dir", required_argument, 0, 'd' },
                { "supersedefullawy", no_argument, 0, 'A' },
                { "authority", no_argument, 0, 'a' },
                { "lower", required_argument, 0, 'L' },
                { "upper", required_argument, 0, 'U' },
                { "dme", required_argument, 0, 'D' },
                { "tacan", required_argument, 0, 'T' },
                {0, 0, 0, 0}
        };
        Glib::ustring db_dir(".");
	Glib::ustring borderfile("");
	std::vector<std::string> lowerawy, upperawy;
	std::vector<std::string> dme, tacan;
	bool supersedefullawy(false), updateauth(false);
        int c, err(0);

        while ((c = getopt_long(argc, argv, "d:Ab:aL:U:D:T:", long_options, 0)) != EOF) {
                switch (c) {
		case 'd':
			if (optarg)
				db_dir = optarg;
			break;

		case 'A':
			supersedefullawy = true;
			break;

		case 'b':
			if (optarg)
				borderfile = optarg;
			break;

		case 'a':
			updateauth = true;
			break;

		case 'L':
			lowerawy.push_back(optarg);
			break;

		case 'U':
			upperawy.push_back(optarg);
			break;

		case 'D':
			dme.push_back(optarg);
			break;

		case 'T':
			tacan.push_back(optarg);
			break;

		default:
			err++;
			break;
                }
        }
        if (err) {
                std::cerr << "usage: vfrdbeadimport [-d <dir>] [-A] [-b <borderfile>]" << std::endl;
                return EX_USAGE;
        }
	if (false) {
		// Test Intersection
		Point pt;
		if (!pt.intersection(Point(100, 0), Point(100, 1000), Point(0, 200), Point(1000, 200))) {
			std::cerr << "Intersection failed" << std::endl;
			return EX_DATAERR;
		}
		std::cerr << "Intersection1: " << pt.get_lon() << ' ' << pt.get_lat() << std::endl;	
		if (!pt.intersection(Point(0, 0), Point(1000, 1000), Point(0, 1000), Point(1000, 0))) {
			std::cerr << "Intersection failed" << std::endl;
			return EX_DATAERR;
		}
		std::cerr << "Intersection2: " << pt.get_lon() << ' ' << pt.get_lat() << std::endl;	
		if (!pt.intersection(Point(0, 0), Point(100, 100), Point(0, 100), Point(100, 150))) {
			std::cerr << "Intersection failed" << std::endl;
			return EX_DATAERR;
		}
		std::cerr << "Intersection3: " << pt.get_lon() << ' ' << pt.get_lat() << std::endl;
		pt = Point(0, 0).nearest_on_line(Point(0, 1000), Point(1000, 0));
		std::cerr << "Nearest1: " << pt.get_lon() << ' ' << pt.get_lat() << std::endl;
		Point org(90 * Point::from_deg_dbl, 60 * Point::from_deg_dbl);
		pt = org.nearest_on_line(org + Point(0, 1000), org + Point(1000, 0)) - org;
		std::cerr << "Nearest2: " << pt.get_lon() << ' ' << pt.get_lat() << std::endl;
		{
			double crs((org + Point(0, 1000)).spheric_true_course_dbl(org + Point(1000, 0)));
			Point pt1(org.spheric_course_distance_nmi(crs + 90, 1));
			if (!pt.intersection(org, pt1, org + Point(0, 1000), org + Point(1000, 0)))
				pt.set_invalid();
			else
				pt -= org;
		}
		std::cerr << "Nearest3: " << pt.get_lon() << ' ' << pt.get_lat() << std::endl;
		return EX_OK;
	}
	if (false) {
		// Test Border stuff
		// Warning: Airspace LST52/TSA/643743548: border ITALY_SWITZERLAND first point distance 0.492048nmi seg (E010°13.39500',N46°37.54334') nearest pt (E010°12.96667',N46°37.15000')
		// Warning: Airspace LST52/TSA/643743548: border ITALY_SWITZERLAND second point distance 0.152614nmi seg (E010°02.80884',N46°30.25517') nearest pt (E010°02.71667',N46°30.11667')
		// 463732.60N 0101323.70E -> 463015.31N 0100248.53E
		// first point: likely segment 463807N 0101401E -> 463709N 0101258E
		Point seg0, seg1, seg, pt;
		seg0.set_str("463807N 0101401E");
		seg1.set_str("463709N 0101258E");
		seg.set_str("463732.60N 0101323.70E");
		pt = seg.nearest_on_line(seg0, seg1);
		std::cerr << "Segment: " << seg0 << " -> " << seg1 << " seg " << seg << " nearest pt " << pt
			  << " dist " << seg.spheric_distance_nmi_dbl(pt) << "nmi; in box " << (pt.in_box(seg0, seg1) ? "yes" : "no") << std::endl;
       		return EX_OK;
	}
	if (false) {
		// Test Border stuff
		// Warning: Airspace EER1/R/83742756: border ESTONIA_RUSSIA second point distance 0.181247nmi seg (E026°43.28333',N59°45.31667') nearest pt (E026°43.17865',N59°45.14360')
		// 573103N 0272105E -> 594519N 0264317E
		// Relevant Segment: 593642N 0273812E -> 595200N 0255830E
		Point seg0, seg1, seg, pt;
		seg0.set_str("593642N 0273812E");
		seg1.set_str("595200N 0255830E");
		seg.set_str("594519N 0264317E");
		pt = seg.nearest_on_line(seg0, seg1);
		std::cerr << "Segment: " << seg0 << " -> " << seg1 << " seg " << seg << " nearest pt " << pt
			  << " dist " << seg.spheric_distance_nmi_dbl(pt) << "nmi; in box " << (pt.in_box(seg0, seg1) ? "yes" : "no") << std::endl;
		pt = seg.spheric_nearest_on_line(seg0, seg1);
		std::cerr << "Segment: " << seg0 << " -> " << seg1 << " seg " << seg << " nearest pt " << pt
			  << " dist " << seg.spheric_distance_nmi_dbl(pt) << "nmi; in box " << (pt.in_box(seg0, seg1) ? "yes" : "no") << std::endl;
		{
			double crs(seg0.spheric_true_course_dbl(seg1));
			Point pt1(seg.spheric_course_distance_nmi(crs + 90, 10));
			if (!pt.intersection(seg, pt1, seg0, seg1))
				pt.set_invalid();
			std::cerr << "pt1 " << pt1 << std::endl;
		}
		std::cerr << "Segment: " << seg0 << " -> " << seg1 << " seg " << seg << " nearest pt " << pt
			  << " dist " << seg.spheric_distance_nmi_dbl(pt) << "nmi; in box " << (pt.in_box(seg0, seg1) ? "yes" : "no") << std::endl;
       		return EX_OK;
	}
        try {
#ifdef HAVE_GDAL
                OGRRegisterAll();
#endif
                DbXmlImporter parser(db_dir, supersedefullawy, borderfile);
                parser.set_validate(false); // Do not validate, we do not have a DTD
                parser.set_substitute_entities(true);
                for (; optind < argc; optind++) {
                        std::cerr << "Parsing file " << argv[optind] << std::endl;
                        parser.parse_file(argv[optind]);
                }
		parser.set_dme();
		for (std::vector<std::string>::const_iterator ai(dme.begin()), ae(dme.end()); ai != ae; ++ai) {
                        std::cerr << "Parsing DME file " << *ai << std::endl;
                        parser.parse_file(*ai);
                }
		parser.set_tacan();
		for (std::vector<std::string>::const_iterator ai(tacan.begin()), ae(tacan.end()); ai != ae; ++ai) {
                        std::cerr << "Parsing TACAN file " << *ai << std::endl;
                        parser.parse_file(*ai);
                }
		parser.set_airway_type(AirwaysDb::Airway::airway_low);
		for (std::vector<std::string>::const_iterator ai(lowerawy.begin()), ae(lowerawy.end()); ai != ae; ++ai) {
                        std::cerr << "Parsing lower airway file " << *ai << std::endl;
                        parser.parse_file(*ai);
                }
		parser.set_airway_type(AirwaysDb::Airway::airway_high);
		for (std::vector<std::string>::const_iterator ai(upperawy.begin()), ae(upperawy.end()); ai != ae; ++ai) {
                        std::cerr << "Parsing upper airway file " << *ai << std::endl;
                        parser.parse_file(*ai);
                }
		parser.finalize_airways();
		parser.finalize_airspaces();
		if (updateauth)
			parser.update_authority();
        } catch (const xmlpp::exception& ex) {
                std::cerr << "libxml++ exception: " << ex.what() << std::endl;
                return EX_DATAERR;
        } catch (const std::exception& ex) {
                std::cerr << "exception: " << ex.what() << std::endl;
                return EX_DATAERR;
        }
        return EX_OK;
}
// select MAX(MODIFIED) from airways;
// 1339410815
// select MAX(MODIFIED) from airways;
// 1343746264
// select count(*) from airways;
// 76998
// select count(*) from airways where MODIFIED<=1339410815;
// 45638
// delete from airways where MODIFIED<=1339410815;
// select count(*) from airways;
// 58792

// http://thematicmapping.org/downloads/world_borders.php/
