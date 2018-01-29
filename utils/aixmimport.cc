//
// C++ Implementation: aixmimport
//
// Description: AIXM Database to sqlite db conversion
//
//
// Author: Thomas Sailer <t.sailer@alumni.ethz.ch>, (C) 2007, 2009, 2012, 2013, 2015
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
#include <fstream>
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
#include <boost/graph/adjacency_list.hpp>
#include <boost/graph/topological_sort.hpp>

#include "dbobj.h"
#include "geomboost.h"

class DbXmlImporter : public xmlpp::SaxParser {
public:
	DbXmlImporter(const Glib::ustring& output_dir, const Glib::ustring& borderfile = "");
	virtual ~DbXmlImporter();

	void update_authority(void);
	void purge_ead_airspaces(void);

	void parse_auag(const Glib::ustring& auagfile);

	typedef enum {
		recalc_empty,
		recalc_composite,
		recalc_all
	} recalc_t;
	void recalc(recalc_t mode);

	class AirspaceId {
	public:
		AirspaceId(const std::string& icao, char bdryclass, uint8_t tc);
		bool operator<(const AirspaceId& x) const;
		const std::string& get_icao(void) const { return m_icao; }
		char get_bdryclass(void) const { return m_bdryclass; }
		uint8_t get_typecode(void) const { return m_typecode; }
		std::string to_str(void) const;
		bool parse(const std::string& x);

	protected:
		std::string m_icao;
		char m_bdryclass;
		uint8_t m_typecode;
	};
	void recalc(const AirspaceId& aspcid);
	void recalc(const AirspacesDb::Address& addr);

	static void test(void);

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

	void process_airspace(void);
	void process_airspace_vertices(bool expand);
	void process_composite_airspaces(void);

private:
	typedef enum {
		state_document_c,
		state_aixmsnapshot_c,
		state_gbr_c,
		state_gbr_uid_c,
		state_gbr_uid_txtname_c,
		state_gbr_codetype_c,
		state_gbr_txtrmk_c,
		state_gbr_gbv_c,
		state_gbr_gbv_codetype_c,
		state_gbr_gbv_geolat_c,
		state_gbr_gbv_geolon_c,
		state_gbr_gbv_codedatum_c,
		state_gbr_gbv_valcrc_c,
		state_gbr_gbv_txtrmk_c,
		state_ase_c,
		state_ase_aseuid_c,
		state_ase_aseuid_codetype_c,
		state_ase_aseuid_codeid_c,
		state_ase_uasuid_c,
		state_ase_uasuid_uniuid_c,
		state_ase_uasuid_uniuid_txtname_c,
		state_ase_uasuid_codetype_c,
		state_ase_uasuid_noseq_c,
		state_ase_rsguid_c,
		state_ase_rsguid_rteuid_c,
		state_ase_rsguid_rteuid_txtdesig_c,
		state_ase_rsguid_rteuid_txtlocdesig_c,
		state_ase_rsguid_dpnuidsta_c,
		state_ase_rsguid_dpnuidsta_codeid_c,
		state_ase_rsguid_dpnuidsta_geolat_c,
		state_ase_rsguid_dpnuidsta_geolon_c,
		state_ase_rsguid_dpnuidend_c,
		state_ase_rsguid_dpnuidend_codeid_c,
		state_ase_rsguid_dpnuidend_geolat_c,
		state_ase_rsguid_dpnuidend_geolon_c,
		state_ase_txtlocaltype_c,
		state_ase_txtname_c,
		state_ase_txtrmk_c,
		state_ase_codeactivity_c,
		state_ase_codeclass_c,
		state_ase_codelocind_c,
		state_ase_codemil_c,
		state_ase_codedistverupper_c,
		state_ase_valdistverupper_c,
		state_ase_uomdistverupper_c,
		state_ase_codedistverlower_c,
		state_ase_valdistverlower_c,
		state_ase_uomdistverlower_c,
		state_ase_codedistvermnm_c,
		state_ase_valdistvermnm_c,
		state_ase_uomdistvermnm_c,
		state_ase_codedistvermax_c,
		state_ase_valdistvermax_c,
		state_ase_uomdistvermax_c,
		state_ase_att_c,
		state_ase_att_codeworkhr_c,
		state_ase_att_txtrmkworkhr_c,
		state_ase_att_timsh_c,
		state_ase_att_timsh_codetimeref_c,
		state_ase_att_timsh_datevalidwef_c,
		state_ase_att_timsh_datevalidtil_c,
		state_ase_att_timsh_codeday_c,
		state_ase_att_timsh_codedaytil_c,
		state_ase_att_timsh_timewef_c,
		state_ase_att_timsh_timetil_c,
		state_ase_att_timsh_codeeventwef_c,
		state_ase_att_timsh_codeeventtil_c,
		state_ase_att_timsh_timereleventwef_c,
		state_ase_att_timsh_timereleventtil_c,
		state_ase_att_timsh_codecombwef_c,
		state_ase_att_timsh_codecombtil_c,
		state_adg_c,
		state_adg_adguid_c,
		state_adg_adguid_aseuid_c,
		state_adg_adguid_aseuid_codetype_c,
		state_adg_adguid_aseuid_codeid_c,
		state_adg_aseuidbase_c,
		state_adg_aseuidbase_codetype_c,
		state_adg_aseuidbase_codeid_c,
		state_adg_aseuidcomponent_c,
		state_adg_aseuidcomponent_codetype_c,
		state_adg_aseuidcomponent_codeid_c,
		state_adg_codeopr_c,
		state_adg_aseuidsameextent_c,
		state_adg_aseuidsameextent_codetype_c,
		state_adg_aseuidsameextent_codeid_c,
		state_adg_txtrmk_c,
		state_abd_c,
		state_abd_abduid_c,
		state_abd_abduid_aseuid_c,
		state_abd_abduid_aseuid_codetype_c,
		state_abd_abduid_aseuid_codeid_c,
		state_abd_avx_c,
		state_abd_avx_gbruid_c,
		state_abd_avx_gbruid_txtname_c,
		state_abd_avx_dpnuidspn_c,
		state_abd_avx_dpnuidspn_codeid_c,
		state_abd_avx_dpnuidspn_geolat_c,
		state_abd_avx_dpnuidspn_geolon_c,
		state_abd_avx_dmeuidspn_c,
		state_abd_avx_dmeuidspn_codeid_c,
		state_abd_avx_dmeuidspn_geolat_c,
		state_abd_avx_dmeuidspn_geolon_c,
		state_abd_avx_tcnuidspn_c,
		state_abd_avx_tcnuidspn_codeid_c,
		state_abd_avx_tcnuidspn_geolat_c,
		state_abd_avx_tcnuidspn_geolon_c,
		state_abd_avx_ndbuidspn_c,
		state_abd_avx_ndbuidspn_codeid_c,
		state_abd_avx_ndbuidspn_geolat_c,
		state_abd_avx_ndbuidspn_geolon_c,
		state_abd_avx_voruidspn_c,
		state_abd_avx_voruidspn_codeid_c,
		state_abd_avx_voruidspn_geolat_c,
		state_abd_avx_voruidspn_geolon_c,
		state_abd_avx_dmeuidcen_c,
		state_abd_avx_dmeuidcen_codeid_c,
		state_abd_avx_dmeuidcen_geolat_c,
		state_abd_avx_dmeuidcen_geolon_c,
		state_abd_avx_tcnuidcen_c,
		state_abd_avx_tcnuidcen_codeid_c,
		state_abd_avx_tcnuidcen_geolat_c,
		state_abd_avx_tcnuidcen_geolon_c,
		state_abd_avx_ndbuidcen_c,
		state_abd_avx_ndbuidcen_codeid_c,
		state_abd_avx_ndbuidcen_geolat_c,
		state_abd_avx_ndbuidcen_geolon_c,
		state_abd_avx_voruidcen_c,
		state_abd_avx_voruidcen_codeid_c,
		state_abd_avx_voruidcen_geolat_c,
		state_abd_avx_voruidcen_geolon_c,
		state_abd_avx_codetype_c,
		state_abd_avx_geolat_c,
		state_abd_avx_geolon_c,
		state_abd_avx_codedatum_c,
		state_abd_avx_geolatarc_c,
		state_abd_avx_geolonarc_c,
		state_abd_avx_valradiusarc_c,
		state_abd_avx_uomradiusarc_c,
		state_abd_avx_valgeoaccuracy_c,
		state_abd_avx_uomgeoaccuracy_c,
		state_abd_avx_valcrc_c,
		state_abd_avx_txtrmk_c,
		state_abd_circle_c,
		state_abd_circle_dmeuidcen_c,
		state_abd_circle_dmeuidcen_codeid_c,
		state_abd_circle_dmeuidcen_geolat_c,
		state_abd_circle_dmeuidcen_geolon_c,
		state_abd_circle_tcnuidcen_c,
		state_abd_circle_tcnuidcen_codeid_c,
		state_abd_circle_tcnuidcen_geolat_c,
		state_abd_circle_tcnuidcen_geolon_c,
		state_abd_circle_ndbuidcen_c,
		state_abd_circle_ndbuidcen_codeid_c,
		state_abd_circle_ndbuidcen_geolat_c,
		state_abd_circle_ndbuidcen_geolon_c,
		state_abd_circle_voruidcen_c,
		state_abd_circle_voruidcen_codeid_c,
		state_abd_circle_voruidcen_geolat_c,
		state_abd_circle_voruidcen_geolon_c,
		state_abd_circle_geolatcen_c,
		state_abd_circle_geoloncen_c,
		state_abd_circle_codedatum_c,
		state_abd_circle_valradius_c,
		state_abd_circle_uomradius_c,
		state_abd_circle_valgeoaccuracy_c,
		state_abd_circle_uomgeoaccuracy_c,
		state_abd_circle_valcrc_c,
		state_abd_circle_txtrmk_c,
		state_abd_txtrmk_c,
		state_acr_c,
		state_acr_acruid_c,
		state_acr_acruid_aseuid_c,
		state_acr_acruid_aseuid_codetype_c,
		state_acr_acruid_aseuid_codeid_c,
		state_acr_valwidth_c,
		state_acr_uomwidth_c,
		state_acr_txtrmk_c,
		state_acr_avx_c,
		state_acr_avx_gbruid_c,
		state_acr_avx_gbruid_txtname_c,
		state_acr_avx_dpnuidspn_c,
		state_acr_avx_dpnuidspn_codeid_c,
		state_acr_avx_dpnuidspn_geolat_c,
		state_acr_avx_dpnuidspn_geolon_c,
		state_acr_avx_dmeuidspn_c,
		state_acr_avx_dmeuidspn_codeid_c,
		state_acr_avx_dmeuidspn_geolat_c,
		state_acr_avx_dmeuidspn_geolon_c,
		state_acr_avx_tcnuidspn_c,
		state_acr_avx_tcnuidspn_codeid_c,
		state_acr_avx_tcnuidspn_geolat_c,
		state_acr_avx_tcnuidspn_geolon_c,
		state_acr_avx_ndbuidspn_c,
		state_acr_avx_ndbuidspn_codeid_c,
		state_acr_avx_ndbuidspn_geolat_c,
		state_acr_avx_ndbuidspn_geolon_c,
		state_acr_avx_voruidspn_c,
		state_acr_avx_voruidspn_codeid_c,
		state_acr_avx_voruidspn_geolat_c,
		state_acr_avx_voruidspn_geolon_c,
		state_acr_avx_codetype_c,
		state_acr_avx_geolat_c,
		state_acr_avx_geolon_c,
		state_acr_avx_codedatum_c,
		state_acr_avx_geolatarc_c,
		state_acr_avx_geolonarc_c,
		state_acr_avx_valradiusarc_c,
		state_acr_avx_uomradiusarc_c,
		state_acr_avx_valgeoaccuracy_c,
		state_acr_avx_uomgeoaccuracy_c,
		state_acr_avx_valcrc_c,
		state_acr_avx_txtrmk_c,
		state_aas_c,
		state_aas_aasuid_c,
		state_aas_aasuid_aseuid1_c,
		state_aas_aasuid_aseuid1_codetype_c,
		state_aas_aasuid_aseuid1_codeid_c,
		state_aas_aasuid_aseuid2_c,
		state_aas_aasuid_aseuid2_codetype_c,
		state_aas_aasuid_aseuid2_codeid_c,
		state_aas_aasuid_codetype_c,
		state_aas_txtrmk_c,
	} state_t;
	state_t m_state;

	Glib::ustring m_outputdir;
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
	time_t m_starttime;

	Glib::ustring m_characters;

	class RecalcComposites {
	public:
		RecalcComposites(AirspacesDb& aspcdb);

		void recalc(recalc_t mode);
		void recalc(const AirspaceId& aspcid);
		void recalc(const AirspacesDb::Address& addr);

	protected:
		class AirspaceStatus : public AirspaceId {
		public:
			AirspaceStatus(const std::string& icao, char bdryclass, uint8_t tc, const AirspacesDb::Address& addr, time_t modtime = 0, bool dirty = false);
			const AirspacesDb::Address& get_address(void) const { return m_addr; }
			void set_address(const AirspacesDb::Address& addr) { m_addr = addr; }
			time_t get_modtime(void) const { return m_modtime; }
			void set_modtime(time_t t) { m_modtime = t; }
			bool is_dirty(void) const { return m_dirty; }
			void set_dirty(bool d = true) { m_dirty = d; }
			static std::string addrstring(const AirspacesDb::Address& addr);
			std::string addrstring(void) const { return addrstring(m_addr); }

		protected:
			AirspacesDb::Address m_addr;
			bool m_dirty;
			time_t m_modtime;
		};

		AirspacesDb::elementvector_t find_airspaces(const AirspaceId& id, unsigned int limit = 0, unsigned int loadsubtables = AirspacesDb::Airspace::subtables_all);
		AirspacesDb::element_t find_airspace(const AirspaceId& id, unsigned int loadsubtables = AirspacesDb::Airspace::subtables_all);
		void recalc(AirspacesDb::Airspace& el);

		AirspacesDb& m_airspacesdb;
		typedef std::vector<AirspaceStatus> airspaces_t;
		airspaces_t m_airspaces;
		typedef std::map<AirspaceId,unsigned int> airspaceindex_t;
		airspaceindex_t m_airspaceindex;
	};

	class Borders {
	public:
		typedef std::vector<PolygonSimple> polygons_t;

		class Border {
		public:
			class Vertex {
			public:
				Vertex(const Point& pt, char typ = ' ') : m_coord(pt), m_type(typ) {}
				Vertex(void) : m_type(' ') { m_coord.set_invalid(); }
				bool is_valid(void) const { return get_type() != ' ' && !get_coord().is_invalid(); }

				const Point& get_coord(void) const { return m_coord; }
				void set_coord(const Point& pt) { m_coord = pt; }
				void set_coord(void) { m_coord.set_invalid(); }
				Point::coord_t get_lon(void) const { return m_coord.get_lon(); }
				void set_lon(Point::coord_t c) { m_coord.set_lon(c); }
				Point::coord_t get_lat(void) const { return m_coord.get_lat(); }
				void set_lat(Point::coord_t c) { m_coord.set_lat(c); }
				void set_lon(const Glib::ustring& t);
				void set_lat(const Glib::ustring& t);
				char get_type(void) const { return m_type; }
				void set_type(char typ = ' ') { m_type = typ; }
				void set_type(const Glib::ustring& typ);

			protected:
				Point m_coord;
				char m_type;
			};

			Border(void) : m_mid(~0UL) {}

			bool is_valid(void) const;
			bool operator<(const Border& b) const;
			void clear(void);
			unsigned int get_nrvertex(void) const { return m_vertex.size(); }
			const Vertex& operator[](unsigned int i) const { return m_vertex[i]; }
			Vertex& operator[](unsigned int i) { return m_vertex[i]; }
			const Vertex& back(void) const { return m_vertex.back(); }
			Vertex& back(void) { return m_vertex.back(); }
			void add_vertex(const Vertex& v) { m_vertex.push_back(v); }

			const Glib::ustring& get_name(void) const { return m_name; }
			void set_name(const Glib::ustring& t) { m_name = t; }
			const Glib::ustring& get_remark(void) const { return m_remark; }
			void set_remark(const Glib::ustring& t) { m_remark = t; }
			const Glib::ustring& get_codetype(void) const { return m_codetype; }
			void set_codetype(const Glib::ustring& t) { m_codetype = t; }
			unsigned long get_mid(void) const { return m_mid; }
			void set_mid(unsigned long mid = ~0UL) { m_mid = mid; }

		protected:
			typedef std::vector<Vertex> vertex_t;
			vertex_t m_vertex;
			Glib::ustring m_name;
			Glib::ustring m_remark;
			Glib::ustring m_codetype;
			unsigned long m_mid;
		};

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

		Borders(const Glib::ustring& bordersfile);
		void add(const Border& brd);
		const Border *find(unsigned long mid, const Glib::ustring& name) const;
		const polygons_t& find(const Glib::ustring& name) const;

	protected:
		typedef std::set<Border> borders_t;
		borders_t m_borders;
		Glib::ustring m_bordersfile;
		typedef std::map<std::string,polygons_t> countries_t;
		countries_t m_countries;
		typedef std::map<std::string,std::string> countrymap_t;
		countrymap_t m_countrymap;

		void loadborders(void);
#ifdef HAVE_GDAL
		void loadborder(const std::string& country, OGRGeometry *geom, OGRFeatureDefn *layerdef = 0, OGRFeature *feature = 0);
		void loadborder(const std::string& country, OGRPolygon *poly, OGRFeatureDefn *layerdef = 0, OGRFeature *feature = 0);
		void loadborder(const std::string& country, OGRLinearRing *ring, OGRFeatureDefn *layerdef = 0, OGRFeature *feature = 0);
#endif
	};

	class ParseAirspace {
	public:
		ParseAirspace(void);

		bool is_valid(void) const;
		bool is_name_valid(void) const { return !get_name().empty(); }
		bool is_efftime_valid(void) const { return !get_efftime().empty(); }
		bool is_note_valid(void) const { return !get_note().empty(); }
		bool is_ident_valid(void) const { return !get_ident().empty(); }
		bool is_lwr_valid(void) const { return !std::isnan(get_lwrscale()) && get_lwralt() != std::numeric_limits<int32_t>::max(); }
		bool is_upr_valid(void) const { return !std::isnan(get_uprscale()) && get_upralt() != std::numeric_limits<int32_t>::min(); }
		void clear(void);

		const Glib::ustring& get_icao(void) const { return m_icao; }
		void set_icao(const Glib::ustring& t) { m_icao = t; }
		const Glib::ustring& get_name(void) const { return m_name; }
		void set_name(const Glib::ustring& t) { m_name = t; }
		const Glib::ustring& get_efftime(void) const { return m_efftime; }
		void set_efftime(const Glib::ustring& t) { m_efftime = t; }
		const Glib::ustring& get_note(void) const { return m_note; }
		void set_note(const Glib::ustring& t) { m_note = t; }
		const Glib::ustring& get_ident(void) const { return m_ident; }
		void set_ident(const Glib::ustring& t) { m_ident = t; }
		unsigned long get_mid(void) const { return m_mid; }
		void set_mid(unsigned long mid = ~0UL) { m_mid = mid; }
		char get_bdryclass(void) const { return m_bdryclass; }
		void set_bdryclass(char c = 0) { m_bdryclass = c; }
		void set_bdryclass(const Glib::ustring& t);
		Glib::ustring get_codetype(void) const;
		double get_lwrscale(void) const { return m_lwrscale; }
		void set_lwrscale(double s = std::numeric_limits<double>::quiet_NaN()) { m_lwrscale = s; }
		void set_lwrscale(const Glib::ustring& t);
		double get_uprscale(void) const { return m_uprscale; }
		void set_uprscale(double s = std::numeric_limits<double>::quiet_NaN()) { m_uprscale = s; }
		void set_uprscale(const Glib::ustring& t);
		int32_t get_lwralt(void) const { return m_lwralt; }
		int32_t get_scaled_lwralt(void) const { return Point::round<int32_t,double>(get_lwralt() * get_lwrscale()); }
		void set_lwralt(int32_t a = std::numeric_limits<int32_t>::max()) { m_lwralt = a; }
		void set_lwralt(const Glib::ustring& t);
		int32_t get_upralt(void) const { return m_upralt; }
		int32_t get_scaled_upralt(void) const { return Point::round<int32_t,double>(get_upralt() * get_uprscale()); }
		void set_upralt(int32_t a = std::numeric_limits<int32_t>::min()) { m_upralt = a; }
		void set_upralt(const Glib::ustring& t);
		uint8_t get_lwrflags(void) const { return m_lwrflags; }
		void set_lwrflags(uint8_t f = 0) { m_lwrflags = f; }
		void set_lwrflags(const Glib::ustring& t);
		uint8_t get_uprflags(void) const { return m_uprflags; }
		void set_uprflags(uint8_t f = 0) { m_uprflags = f; }
		void set_uprflags(const Glib::ustring& t);

	protected:
		Glib::ustring m_icao;
		Glib::ustring m_name;
		Glib::ustring m_efftime;
		Glib::ustring m_note;
		Glib::ustring m_ident;
		double m_lwrscale;
		double m_uprscale;
		unsigned long m_mid;
		int32_t m_lwralt;
		int32_t m_upralt;
		uint8_t m_lwrflags;
		uint8_t m_uprflags;
		char m_bdryclass;
	};

	class ParseTimesheet {
	public:
		ParseTimesheet(void) {}
		bool is_valid(void) const;
		void clear(void);
		Glib::ustring to_string(void) const;

		const Glib::ustring& get_timeref(void) const { return m_timeref; }
		void set_timeref(const Glib::ustring& t) { m_timeref = t; }
		const Glib::ustring& get_dateeff(void) const { return m_dateeff; }
		void set_dateeff(const Glib::ustring& t) { m_dateeff = t; }
		const Glib::ustring& get_dateend(void) const { return m_dateend; }
		void set_dateend(const Glib::ustring& t) { m_dateend = t; }
		const Glib::ustring& get_day(void) const { return m_day; }
		void set_day(const Glib::ustring& t) { m_day = t; }
		const Glib::ustring& get_dayend(void) const { return m_dayend; }
		void set_dayend(const Glib::ustring& t) { m_dayend = t; }
		const Glib::ustring& get_timeeff(void) const { return m_timeeff; }
		void set_timeeff(const Glib::ustring& t) { m_timeeff = t; }
		const Glib::ustring& get_timeend(void) const { return m_timeend; }
		void set_timeend(const Glib::ustring& t) { m_timeend = t; }
		const Glib::ustring& get_eventtimeeff(void) const { return m_eventtimeeff; }
		void set_eventtimeeff(const Glib::ustring& t) { m_eventtimeeff = t; }
		const Glib::ustring& get_eventtimeend(void) const { return m_eventtimeend; }
		void set_eventtimeend(const Glib::ustring& t) { m_eventtimeend = t; }
		const Glib::ustring& get_eventcodeeff(void) const { return m_eventcodeeff; }
		void set_eventcodeeff(const Glib::ustring& t) { m_eventcodeeff = t; }
		const Glib::ustring& get_eventcodeend(void) const { return m_eventcodeend; }
		void set_eventcodeend(const Glib::ustring& t) { m_eventcodeend = t; }
		const Glib::ustring& get_eventopeff(void) const { return m_eventopeff; }
		void set_eventopeff(const Glib::ustring& t) { m_eventopeff = t; }
		const Glib::ustring& get_eventopend(void) const { return m_eventopend; }
		void set_eventopend(const Glib::ustring& t) { m_eventopend = t; }

	protected:
		Glib::ustring m_timeref;
		Glib::ustring m_dateeff;
		Glib::ustring m_dateend;
		Glib::ustring m_day;
		Glib::ustring m_dayend;
		Glib::ustring m_timeeff;
		Glib::ustring m_timeend;
		Glib::ustring m_eventtimeeff;
		Glib::ustring m_eventtimeend;
		Glib::ustring m_eventcodeeff;
		Glib::ustring m_eventcodeend;
		Glib::ustring m_eventopeff;
		Glib::ustring m_eventopend;
	};

	class ParseVertices {
	public:
		class Segment : public AirspacesDb::Airspace::Segment {
		public:
			Segment(void);

			using AirspacesDb::Airspace::Segment::set_shape;
			void set_shape(const Glib::ustring& shp);

			const Glib::ustring& get_bordername(void) const { return m_bordername; }
			void set_bordername(const Glib::ustring& bn) { m_bordername = bn; }
			unsigned long get_bordermid(void) const { return m_bordermid; }
			void set_bordermid(unsigned long mid = ~0UL) { m_bordermid = mid; }
			double get_radius(void) const { return m_radius; }
			void set_radius(double v = std::numeric_limits<double>::quiet_NaN());
			void set_radius(const Glib::ustring& t);
			double get_radiusscale(void) const { return m_radiusscale; }
			void set_radiusscale(double v = std::numeric_limits<double>::quiet_NaN());
			void set_radiusscale(const Glib::ustring& t);

			void set_lon(const Glib::ustring& t);
			void set_lat(const Glib::ustring& t);
			void set_centerlon(const Glib::ustring& t);
			void set_centerlat(const Glib::ustring& t);

		protected:
			Glib::ustring m_bordername;
			unsigned long m_bordermid;
			double m_radius;
			double m_radiusscale;

			void update_radius(void);
		};

		ParseVertices(void) : m_mid(~0UL), m_asemid(~0UL), m_width(std::numeric_limits<double>::quiet_NaN()),
				      m_widthscale(std::numeric_limits<double>::quiet_NaN()), m_bdryclass(0) {}

		bool is_valid(void) const;
		bool is_width_valid(void) const { return !std::isnan(get_width()) && !std::isnan(get_widthscale()); }
		void clear(void);

		unsigned long get_mid(void) const { return m_mid; }
		void set_mid(unsigned long mid = ~0UL) { m_mid = mid; }
		unsigned long get_asemid(void) const { return m_asemid; }
		void set_asemid(unsigned long mid = ~0UL) { m_asemid = mid; }
		const Glib::ustring& get_icao(void) const { return m_icao; }
		void set_icao(const Glib::ustring& t) { m_icao = t; }
		char get_bdryclass(void) const { return m_bdryclass; }
		void set_bdryclass(char c = 0) { m_bdryclass = c; }
		void set_bdryclass(const Glib::ustring& t);
		Glib::ustring get_codetype(void) const;
		double get_width(void) const { return m_width; }
		void set_width(double v = std::numeric_limits<double>::quiet_NaN()) { m_width = v; }
		void set_width(const Glib::ustring& t);
		double get_widthscale(void) const { return m_widthscale; }
		void set_widthscale(double v = std::numeric_limits<double>::quiet_NaN()) { m_widthscale = v; }
		void set_widthscale(const Glib::ustring& t);
		double get_scaled_width(void) const { return get_width() * get_widthscale(); }

		unsigned int size(void) const { return m_segments.size(); }
		Segment& operator[](unsigned int i) { return m_segments[i]; }
		const Segment& operator[](unsigned int i) const { return m_segments[i]; }
		Segment& back(void) { return m_segments.back(); }
		const Segment& back(void) const { return m_segments.back(); }
		void append_segment(const Segment& seg = Segment()) { m_segments.push_back(seg); }

		void recompute_coord2(void);
		void resolve_borders(const Borders& borders);

	protected:
		typedef std::vector<Segment> segments_t;
		segments_t m_segments;
		Glib::ustring m_icao;
		unsigned long m_mid;
		unsigned long m_asemid;
		double m_width;
		double m_widthscale;
		char m_bdryclass;
	};

	class ParseComposite {
	public:
		class Component : public AirspacesDb::Airspace::Component {
		public:
			Component(void) : m_mid(~0UL) {}

			unsigned long get_mid(void) const { return m_mid; }
			void set_mid(unsigned long mid = ~0UL) { m_mid = mid; }
			Glib::ustring get_codetype(void) const;
			using AirspacesDb::Airspace::Component::set_bdryclass;
			void set_bdryclass(const Glib::ustring& t);

		protected:
			unsigned long m_mid;
		};

		ParseComposite(void) : m_mid(~0UL), m_asemid(~0UL), m_bdryclass(0), m_operator(AirspacesDb::Airspace::Component::operator_set) {}

		bool is_valid(void) const;

		unsigned long get_mid(void) const { return m_mid; }
		void set_mid(unsigned long mid = ~0UL) { m_mid = mid; }
		unsigned long get_asemid(void) const { return m_asemid; }
		void set_asemid(unsigned long mid = ~0UL) { m_asemid = mid; }
		const Glib::ustring& get_icao(void) const { return m_icao; }
		void set_icao(const Glib::ustring& t) { m_icao = t; }
		char get_bdryclass(void) const { return m_bdryclass; }
		void set_bdryclass(char c = 0) { m_bdryclass = c; }
		void set_bdryclass(const Glib::ustring& t);
		Glib::ustring get_codetype(void) const;
		AirspacesDb::Airspace::Component::operator_t get_operator(void) const { return m_operator; }
		void set_operator(AirspacesDb::Airspace::Component::operator_t opr = AirspacesDb::Airspace::Component::operator_set) { m_operator = opr; }
		void set_operator_aixm(const Glib::ustring& opr);

		unsigned int size(void) const { return m_components.size(); }
		Component& operator[](unsigned int i) { return m_components[i]; }
		const Component& operator[](unsigned int i) const { return m_components[i]; }
		Component& back(void) { return m_components.back(); }
		const Component& back(void) const { return m_components.back(); }
		void append_component(const Component& comp = Component()) { m_components.push_back(comp); }

	protected:
		typedef std::vector<Component> components_t;
		components_t m_components;
		Glib::ustring m_icao;
		unsigned long m_mid;
		unsigned long m_asemid;
		char m_bdryclass;
		AirspacesDb::Airspace::Component::operator_t m_operator;
	};

	Borders m_borders;
	Borders::Border m_curborder;
	ParseAirspace m_curairspace;
	ParseTimesheet m_curtimesheet;
	ParseVertices m_curvertices;
	typedef std::vector<ParseComposite> composites_t;
	composites_t m_composites;

	static const std::string& to_string(DbXmlImporter::ParseComposite::Component::operator_t opr);
#ifdef HAVE_GDAL
	bool set_segments(AirspacesDb::Airspace& el, OGRGeometry *geom, const Point& offs);
#endif
	AirspacesDb::Airspace create_composite_airspace(const ParseComposite& comp);
	void recalc_composite_airspace(AirspacesDb::Airspace& el);

	class AirspaceMapping {
	public:
		AirspaceMapping(const char *name, char bc, const char *name0 = 0, char bc0 = 0,
				const char *name1 = 0, char bc1 = 0, const char *name2 = 0, char bc2 = 0);
		unsigned int size(void) const;
		bool operator<(const AirspaceMapping& x) const;
		std::string get_name(unsigned int i) const;
		char get_bdryclass(unsigned int i) const;

	protected:
		const char *m_names[4];
		char m_bdryclass[4];
	};
	static const AirspaceMapping airspacemapping[];

	const AirspaceMapping *find_airspace_mapping(const std::string& name, char bc) const;
	void check_airspace_mapping(void);

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
};

DbXmlImporter::AirspaceId::AirspaceId(const std::string& icao, char bdryclass, uint8_t tc)
	: m_icao(icao), m_bdryclass(bdryclass), m_typecode(tc)
{
}

bool DbXmlImporter::AirspaceId::operator<(const AirspaceId& x) const
{
	int c(m_icao.compare(x.m_icao));
	if (c)
		return c < 0;
	if (m_bdryclass < x.m_bdryclass)
		return true;
	if (m_bdryclass > x.m_bdryclass)
		return false;
	return m_typecode < x.m_typecode;
}

std::string DbXmlImporter::AirspaceId::to_str(void) const
{
	return m_icao + "/" +
		AirspacesDb::Airspace::get_class_string(m_bdryclass, m_typecode) + "/" +
		AirspacesDb::Airspace::get_type_string(m_typecode);
}

bool DbXmlImporter::AirspaceId::parse(const std::string& x)
{
	std::string::size_type n(x.find('/'));
	if (n == std::string::npos || !n)
		return false;
	uint8_t tc;
	char bc;
	if (!AirspacesDb::Airspace::set_class_string(tc, bc, x.substr(n + 1)))
		return false;;
	m_icao = x.substr(0, n);
	m_bdryclass = bc;
	m_typecode = tc;
	return true;
}

DbXmlImporter::RecalcComposites::AirspaceStatus::AirspaceStatus(const std::string& icao, char bdryclass, uint8_t tc, const AirspacesDb::Address& addr, time_t modtime, bool dirty)
	: AirspaceId(icao, bdryclass, tc), m_addr(addr), m_dirty(dirty), m_modtime(modtime)
{
}

std::string DbXmlImporter::RecalcComposites::AirspaceStatus::addrstring(const AirspacesDb::Address& addr)
{
	std::ostringstream oss;
	switch (addr.get_table()) {
	case AirspacesDb::Address::table_main:
		oss << 'M';
		break;

	case AirspacesDb::Address::table_aux:
		oss << 'A';
		break;

	default:
		oss << '?';
		break;
	}
	oss << addr.get_id();
	return oss.str();
}

DbXmlImporter::RecalcComposites::RecalcComposites(AirspacesDb& aspcdb)
	: m_airspacesdb(aspcdb)
{
}

AirspacesDb::elementvector_t DbXmlImporter::RecalcComposites::find_airspaces(const AirspaceId& id, unsigned int limit, unsigned int loadsubtables)
{
	AirspacesDb::elementvector_t ev(m_airspacesdb.find_by_icao(id.get_icao(), 0, AirspacesDb::comp_exact, 0, loadsubtables));
	for (AirspacesDb::elementvector_t::iterator i(ev.begin()), e(ev.end()); i != e; ) {
		if (i->is_valid() && i->get_icao() == id.get_icao() && i->get_bdryclass() == id.get_bdryclass() && i->get_typecode() == id.get_typecode()) {
			++i;
			continue;
		}
		i = ev.erase(i);
		e = ev.end();
	}
	return ev;
}

AirspacesDb::element_t DbXmlImporter::RecalcComposites::find_airspace(const AirspaceId& id, unsigned int loadsubtables)
{
	AirspacesDb::elementvector_t ev(find_airspaces(id, 0, loadsubtables));
	if (!ev.size()) {
		std::cerr << "Warning: Airspace " << id.to_str() << " not found" << std::endl;
		return AirspacesDb::element_t();
	}
	if (ev.size() > 1) {
		std::cerr << "Warning: Airspace " << id.to_str() << " exists " << ev.size() << " times" << std::endl;
	}
	return ev[0];
}

void DbXmlImporter::RecalcComposites::recalc(recalc_t mode)
{
	m_airspaces.clear();
	m_airspaceindex.clear();
	std::cout << "Scanning Airspaces" << std::endl;
	typedef boost::adjacency_list<boost::vecS,boost::vecS,boost::directedS,boost::property<boost::vertex_color_t,boost::default_color_type> > Graph;
	typedef boost::graph_traits<Graph>::vertex_descriptor Vertex;
	Graph topograph;
	{
		AirspacesDb::Airspace aspc;
		m_airspacesdb.loadfirst(aspc, false, AirspacesDb::element_t::subtables_all);
		while (aspc.is_valid()) {
			bool dirty((mode == recalc_all) ||
				   (mode == recalc_composite && aspc.get_nrcomponents()) ||
				   ((aspc.get_nrsegments() || aspc.get_nrcomponents()) && !aspc.get_polygon().size()));
			AirspaceId aspcid(aspc.get_icao(), aspc.get_bdryclass(), aspc.get_typecode());
			std::pair<airspaceindex_t::iterator,bool> ins(m_airspaceindex.insert(std::make_pair(aspcid, m_airspaces.size())));
			if (!ins.second) {
				AirspaceStatus& stat(m_airspaces[ins.first->second]);
				if (stat.get_address()) {
					std::cerr << "Warning: Duplicate Airspace " << aspcid.to_str() << " existing address " << stat.addrstring()
						  << " new address " << AirspaceStatus::addrstring(aspc.get_address()) << std::endl;
				} else {
					stat.set_address(aspc.get_address());
					stat.set_modtime(aspc.get_modtime());
					stat.set_dirty(dirty);
				}
			} else {
				m_airspaces.push_back(AirspaceStatus(aspc.get_icao(), aspc.get_bdryclass(), aspc.get_typecode(), aspc.get_address(), aspc.get_modtime(), dirty));
				boost::add_vertex(topograph);
			}
			for (unsigned int i = 0; i < aspc.get_nrcomponents(); ++i) {
				const AirspacesDb::Airspace::Component& comp(aspc.get_component(i));
				if (!comp.is_valid())
					continue;
				AirspaceId aspcid(comp.get_icao(), comp.get_bdryclass(), comp.get_typecode());
				std::pair<airspaceindex_t::iterator,bool> insc(m_airspaceindex.insert(std::make_pair(aspcid, m_airspaces.size())));
				if (insc.second) {
					m_airspaces.push_back(AirspaceStatus(aspcid.get_icao(), aspcid.get_bdryclass(), aspcid.get_typecode(), AirspacesDb::Airspace::Address(), 0, false));
					boost::add_vertex(topograph);
				}
				boost::add_edge(ins.first->second, insc.first->second, topograph);
			}
			m_airspacesdb.loadnext(aspc, false, AirspacesDb::element_t::subtables_all);
		}
		std::cout << "Airspaces found: " << m_airspaces.size() << std::endl;
	}
	typedef std::vector<Vertex> container_t;
	container_t c;
	topological_sort(topograph, std::back_inserter(c));
	for (container_t::const_iterator ci(c.begin()), ce(c.end()); ci != ce; ++ci) {
		AirspaceStatus& stat(m_airspaces[*ci]);
		if (!stat.get_address()) {
			std::cerr << "Warning: Airspace " << stat.to_str() << " not found" << std::endl;
			continue;
		}
		if (!stat.is_dirty()) {
			Graph::out_edge_iterator e0, e1;
			for (boost::tie(e0, e1) = boost::out_edges(*ci, topograph); e0 != e1; ++e0) {
				const AirspaceStatus& stat1(m_airspaces[boost::target(*e0, topograph)]);
				if (stat1.is_dirty() || stat1.get_modtime() > stat.get_modtime()) {
					stat.set_dirty(true);
					break;
				}
			}
			if (!stat.is_dirty())
				continue;
		}
		AirspacesDb::Airspace el(m_airspacesdb(stat.get_address()));
		recalc(el);
		stat.set_modtime(el.get_modtime());
	}
}

void DbXmlImporter::RecalcComposites::recalc(const AirspaceId& aspcid)
{
	AirspacesDb::elementvector_t ev(find_airspaces(aspcid));
	for (AirspacesDb::elementvector_t::iterator i(ev.begin()), e(ev.end()); i != e; ++i)
		recalc(*i);
}

void DbXmlImporter::RecalcComposites::recalc(const AirspacesDb::Address& addr)
{
	AirspacesDb::Airspace el(m_airspacesdb(addr));
	recalc(el);
}

void DbXmlImporter::RecalcComposites::recalc(AirspacesDb::Airspace& el)
{
	AirspaceId id(el.get_icao(), el.get_bdryclass(), el.get_typecode());
	if (!el.is_valid()) {
		std::cerr << "Warning: Airspace " << id.to_str() << " cannot be loaded" << std::endl;
		return;
	}
	if (true)
		std::cout << "Recompute Airspace " << id.to_str() << ' ' << el.get_name() << ' ' << el.get_sourceid() << std::endl;
	el.recompute_lineapprox(m_airspacesdb);
	el.recompute_bbox();
	if (!el.get_bbox().is_inside(el.get_labelcoord()) || !el.get_polygon().windingnumber(el.get_labelcoord()))
		el.compute_initial_label_placement();
	el.set_modtime(time(0));
	if (true)
		std::cerr << "Recomputed Airspace " << id.to_str() << ' ' << el.get_name() << ' ' << el.get_sourceid() << std::endl;
	try {
		m_airspacesdb.save(el);
	} catch (const std::exception& e) {
		std::cerr << "I/O Error: " << e.what() << std::endl;
		throw;
	}
}

bool DbXmlImporter::Borders::Border::is_valid(void) const
{
	return get_mid() != ~0UL && !get_name().empty() && get_nrvertex();
}

bool DbXmlImporter::Borders::Border::operator<(const Border& b) const
{
	if (get_mid() < b.get_mid())
		return true;
	if (get_mid() > b.get_mid())
		return false;
	return get_name() < b.get_name();
}

void DbXmlImporter::Borders::Border::clear(void)
{
	m_mid = ~0UL;
	m_vertex.clear();
	m_name.clear();
	m_remark.clear();
	m_codetype.clear();
}

void DbXmlImporter::Borders::Border::Vertex::set_type(const Glib::ustring& typ)
{
	if (typ == "CIR") {
		set_type('C');
		return;
	}
	if (typ == "CCA") {
		set_type('L');
		return;
	}
	if (typ == "CWA") {
		set_type('R');
		return;
	}
	if (typ == "GRC") {
		set_type('B');
		return;
	}
	if (typ == "RHL") {
		set_type('H');
		return;
	}
	if (typ == "END") {
		set_type('E');
		return;
	}
	if (typ == "FNT") {
		set_type('F');
		return;
	}
	throw std::runtime_error("Vertex::set_type: invalid type " + typ);
}

void DbXmlImporter::Borders::Border::Vertex::set_lon(const Glib::ustring& t)
{
	Point pt(get_coord());
	if (Point::setstr_lon != pt.set_str(t))
		throw std::runtime_error("Cannot parse longitude " + t);
	set_lon(pt.get_lon());
}

void DbXmlImporter::Borders::Border::Vertex::set_lat(const Glib::ustring& t)
{
	Point pt(get_coord());
	if (Point::setstr_lat != pt.set_str(t))
		throw std::runtime_error("Cannot parse latitude " + t);
	set_lat(pt.get_lat());
}

DbXmlImporter::Borders::FindNearest::FindNearest(const Point& p1, const Point& p2)
	: m_p1(p1), m_p2(p2), m_dist(std::numeric_limits<uint64_t>::max()), m_idx1(~0U), m_idx2(~0U)
{
}

void DbXmlImporter::Borders::FindNearest::process(const polygons_t& polys)
{
	for (polygons_t::const_iterator pi(polys.begin()), pe(polys.end()); pi != pe; ++pi)
		process(*pi);
}

void DbXmlImporter::Borders::FindNearest::process(const PolygonSimple& poly)
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

DbXmlImporter::Borders::Borders(const Glib::ustring& bordersfile)
	: m_bordersfile(bordersfile)
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

void DbXmlImporter::Borders::add(const Border& brd)
{
	std::pair<borders_t::iterator,bool> ins(m_borders.insert(brd));
	if (!ins.second)
		std::cerr << "Warning: duplicate border " << brd.get_mid() << ' ' << brd.get_name() << std::endl;
}

const DbXmlImporter::Borders::Border *DbXmlImporter::Borders::find(unsigned long mid, const Glib::ustring& name) const
{
	Border brd;
	brd.set_mid(mid);
	brd.set_name(name);
	borders_t::iterator it(m_borders.find(brd));
	if (it == m_borders.end())
		return 0;
	return &*it;
}

const DbXmlImporter::Borders::polygons_t& DbXmlImporter::Borders::find(const Glib::ustring& name) const
{
	static const polygons_t empty;
	const_cast<Borders *>(this)->loadborders();
	countrymap_t::const_iterator mi(m_countrymap.find(name.uppercase()));
	countries_t::const_iterator i(m_countries.find((mi == m_countrymap.end()) ? (std::string)name : mi->second));
	if (i == m_countries.end())
		return empty;
	return i->second;
}

void DbXmlImporter::Borders::loadborders(void)
{
	if (m_bordersfile.empty())
		return;
#ifdef HAVE_GDAL
#if defined(HAVE_GDAL2)
	GDALDataset *ds((GDALDataset *)GDALOpenEx(m_bordersfile.c_str(), GDAL_OF_READONLY, 0, 0, 0));
#else
	OGRSFDriver *drv(0);
	OGRDataSource *ds(OGRSFDriverRegistrar::Open(m_bordersfile.c_str(), false, &drv));
#endif
	if (!ds) {
		std::cerr << "Cannot open borders file " << m_bordersfile << std::endl;
		m_bordersfile.clear();
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
		std::cerr << "Borders file " << m_bordersfile << " Countries: ";
		bool subseq(false);
		for (countries_t::const_iterator ci(m_countries.begin()), ce(m_countries.end()); ci != ce; ++ci) {
			if (subseq)
				std::cerr << ", ";
			subseq = true;
			std::cerr << "\"" << ci->first << "\"";
		}
		std::cerr << std::endl;
	}
	m_bordersfile.clear();
#else
	std::cerr << "Cannot process borders file " << m_bordersfile << std::endl;
	m_bordersfile.clear();
#endif
}

#ifdef HAVE_GDAL
void DbXmlImporter::Borders::loadborder(const std::string& country, OGRGeometry *geom, OGRFeatureDefn *layerdef, OGRFeature *feature)
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

void DbXmlImporter::Borders::loadborder(const std::string& country, OGRPolygon *poly, OGRFeatureDefn *layerdef, OGRFeature *feature)
{
	if (!poly)
		return;
       	loadborder(country, poly->getExteriorRing(), layerdef, feature);
	for (int i = 0, j = poly->getNumInteriorRings(); i < j; ++i)
		loadborder(country, poly->getInteriorRing(i), layerdef, feature);
}

void DbXmlImporter::Borders::loadborder(const std::string& country, OGRLinearRing *ring, OGRFeatureDefn *layerdef, OGRFeature *feature)
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


DbXmlImporter::ParseAirspace::ParseAirspace(void)
	: m_lwrscale(std::numeric_limits<double>::quiet_NaN()), m_uprscale(std::numeric_limits<double>::quiet_NaN()),
	  m_mid(~0UL), m_lwralt(std::numeric_limits<int32_t>::max()), m_upralt(std::numeric_limits<int32_t>::min()),
	  m_lwrflags(0), m_uprflags(0), m_bdryclass(0)
{
}

bool DbXmlImporter::ParseAirspace::is_valid(void) const
{
	return get_mid() != ~0UL;
}

void DbXmlImporter::ParseAirspace::clear(void)
{
	m_icao.clear();
	m_name.clear();
	m_efftime.clear();
	m_note.clear();
	m_ident.clear();
	m_lwrscale = m_uprscale = std::numeric_limits<double>::quiet_NaN();
	m_mid = ~0UL;
	m_lwralt = std::numeric_limits<int32_t>::max();
	m_upralt = std::numeric_limits<int32_t>::min();
	m_lwrflags = m_uprflags = 0;
	m_bdryclass = 0;
}

void DbXmlImporter::ParseAirspace::set_bdryclass(const Glib::ustring& t)
{
	m_bdryclass = 0;
	{
		uint8_t tc(0);
		char bc(0);
		Glib::ustring tt(t);
		if (tt == "D")
			tt = "D-EAD";
		if (AirspacesDb::Airspace::set_class_string(tc, bc, tt) && tc == AirspacesDb::Airspace::typecode_ead) {
			m_bdryclass = bc;
			return;
		}
	}
	throw std::runtime_error("Invalid Boundary Type: " + t);
}

Glib::ustring DbXmlImporter::ParseAirspace::get_codetype(void) const
{
	if (get_bdryclass() == AirspacesDb::Airspace::bdryclass_ead_d)
		return "D";
	return AirspacesDb::Airspace::get_class_string(get_bdryclass(), AirspacesDb::Airspace::typecode_ead);
}

void DbXmlImporter::ParseAirspace::set_lwrscale(const Glib::ustring& t)
{
	if (t == "FT") {
		set_lwrscale(1);
		return;
	}
	if (t == "FL") {
		set_lwrscale(100);
		return;
	}
	if (t == "M") {
		set_lwrscale(Point::m_to_ft);
		return;
	}
	set_lwrscale();
}

void DbXmlImporter::ParseAirspace::set_uprscale(const Glib::ustring& t)
{
	if (t == "FT") {
		set_uprscale(1);
		return;
	}
	if (t == "FL") {
		set_uprscale(100);
		return;
	}
	if (t == "M") {
		set_uprscale(Point::m_to_ft);
		return;
	}
	set_uprscale();
}

void DbXmlImporter::ParseAirspace::set_lwralt(const Glib::ustring& t)
{
	char *ep(0);
	set_lwralt(strtol(t.c_str(), &ep, 10));
	if (*ep)
		set_lwralt();
}

void DbXmlImporter::ParseAirspace::set_upralt(const Glib::ustring& t)
{
	char *ep(0);
	set_upralt(strtol(t.c_str(), &ep, 10));
	if (*ep)
		set_upralt();
}

void DbXmlImporter::ParseAirspace::set_lwrflags(const Glib::ustring& t)
{
	if (t == "STD") {
		set_lwrflags(AirspacesDb::Airspace::altflag_fl);
		return;
	}
	if (t == "ALT" || t == "QNH") {
		set_lwrflags(0);
		return;
	}
	if (t == "HEI" || t == "QFE") {
		set_lwrflags(AirspacesDb::Airspace::altflag_agl);
		return;
	}
	set_lwrflags();
}

void DbXmlImporter::ParseAirspace::set_uprflags(const Glib::ustring& t)
{
	if (t == "STD") {
		set_uprflags(AirspacesDb::Airspace::altflag_fl);
		return;
	}
	if (t == "ALT" || t == "QNH") {
		set_uprflags(0);
		return;
	}
	if (t == "HEI" || t == "QFE") {
		set_uprflags(AirspacesDb::Airspace::altflag_agl);
		return;
	}
	set_uprflags();
}

bool DbXmlImporter::ParseTimesheet::is_valid(void) const
{
	return !get_dateeff().empty() && !get_dateend().empty() &&
		!get_timeeff().empty() && !get_timeend().empty() &&
		!get_day().empty() && !get_timeref().empty();
}

void DbXmlImporter::ParseTimesheet::clear(void)
{
	m_timeref.clear();
	m_dateeff.clear();
	m_dateend.clear();
	m_day.clear();
	m_dayend.clear();
	m_timeeff.clear();
	m_timeend.clear();
	m_eventtimeeff.clear();
	m_eventtimeend.clear();
	m_eventcodeeff.clear();
	m_eventcodeend.clear();
	m_eventopeff.clear();
	m_eventopend.clear();
}

Glib::ustring DbXmlImporter::ParseTimesheet::to_string(void) const
{
	std::ostringstream oss;
	oss << get_day();
	if (!get_dayend().empty())
		oss << '-' << get_dayend();
	oss << ' ' << get_dateeff() << '-' << get_dateend()
	    << ' ';
	if (get_eventtimeeff().empty() || get_eventtimeend().empty() ||
	    get_eventcodeeff().empty() || get_eventcodeend().empty() ||
	    (get_eventopeff() != "E" && get_eventopeff() != "L") ||
	    (get_eventopend() != "E" && get_eventopend() != "L"))
		oss << get_timeeff() << '-' << get_timeend();
	else
		oss << get_eventcodeeff() << (get_eventopeff() == "E" ? '-' : '+') << get_eventtimeeff() << '-'
		    << get_eventcodeend() << (get_eventopend() == "E" ? '-' : '+') << get_eventtimeend();
	oss << " (" << get_timeref() << ')';
	return oss.str();
}

DbXmlImporter::ParseVertices::Segment::Segment(void)
	: m_bordermid(~0UL), m_radius(std::numeric_limits<double>::quiet_NaN()), m_radiusscale(std::numeric_limits<double>::quiet_NaN())
{
	Point pt;
	pt.set_invalid();
	set_coord0(pt);
	set_coord1(pt);
	set_coord2(pt);
}

void DbXmlImporter::ParseVertices::Segment::set_shape(const Glib::ustring& shp)
{
	Glib::ustring ct(shp.uppercase());
	if (ct == "CIR") {
		set_shape('C');
		return;
	}
	if (ct == "CCA") {
		set_shape('L');
		return;
	}
	if (ct == "CWA") {
		set_shape('R');
		return;
	}
	if (ct == "GRC") {
		set_shape('B');
		return;
	}
	if (ct == "RHL") {
		set_shape('H');
		return;
	}
	if (ct == "END") {
		set_shape('E');
		return;
	}
	if (ct == "FNT") {
		set_shape('F');
		return;
	}
	// FIXME
	if (ct == "OTHER") {
		set_shape('-');
		return;
	}
	throw std::runtime_error("Unknown airspace segment shape: " + shp);
}

void DbXmlImporter::ParseVertices::Segment::set_radius(const Glib::ustring& t)
{
	char *ep(0);
	set_radius(strtod(t.c_str(), &ep));
	if (t.empty() || *ep)
		throw std::runtime_error("Cannot parse radius: " + t);
}

void DbXmlImporter::ParseVertices::Segment::set_radiusscale(const Glib::ustring& t)
{
	Glib::ustring x(t.uppercase());
	if (x == "FT") {
		set_radiusscale(Point::ft_to_m_dbl * 1e-3 * Point::km_to_nmi_dbl * (Point::from_deg_dbl / 60.0));
		return;
	}
	if (x == "M") {
		set_radiusscale(1e-3 * Point::km_to_nmi_dbl * (Point::from_deg_dbl / 60.0));
		return;
	}
	if (x == "KM") {
		set_radiusscale(Point::km_to_nmi_dbl * (Point::from_deg_dbl / 60.0));
		return;
	}
	if (x == "NM") {
		set_radiusscale(Point::from_deg_dbl / 60.0);
		return;
	}
	set_radiusscale();
	throw std::runtime_error("Unknown radius unit: " + t);
}

void DbXmlImporter::ParseVertices::Segment::set_radius(double v)
{
	m_radius = v;
	update_radius();
}

void DbXmlImporter::ParseVertices::Segment::set_radiusscale(double v)
{
	m_radiusscale = v;
	update_radius();
}

void DbXmlImporter::ParseVertices::Segment::update_radius(void)
{
	double r(get_radius() * get_radiusscale());
	if (std::isnan(r)) {
		set_radius1(0);
		set_radius2(0);
		return;
	}
	int32_t ri(Point::round<int32_t,double>(r));
	set_radius1(ri);
	set_radius2(ri);
}

void DbXmlImporter::ParseVertices::Segment::set_lon(const Glib::ustring& t)
{
	Point pt(get_coord1());
	if (Point::setstr_lon != pt.set_str(t))
		throw std::runtime_error("Cannot parse longitude " + t);
	pt.set_lat(get_coord1().get_lat());
	set_coord1(pt);
}

void DbXmlImporter::ParseVertices::Segment::set_lat(const Glib::ustring& t)
{
	Point pt(get_coord1());
	if (Point::setstr_lat != pt.set_str(t))
		throw std::runtime_error("Cannot parse latitude " + t);
	pt.set_lon(get_coord1().get_lon());
	set_coord1(pt);
}

void DbXmlImporter::ParseVertices::Segment::set_centerlon(const Glib::ustring& t)
{
	Point pt(get_coord0());
	if (Point::setstr_lon != pt.set_str(t))
		throw std::runtime_error("Cannot parse longitude " + t);
	pt.set_lat(get_coord0().get_lat());
	set_coord0(pt);
}

void DbXmlImporter::ParseVertices::Segment::set_centerlat(const Glib::ustring& t)
{
	Point pt(get_coord0());
	if (Point::setstr_lat != pt.set_str(t))
		throw std::runtime_error("Cannot parse latitude " + t);
	pt.set_lon(get_coord0().get_lon());
	set_coord0(pt);
}

bool DbXmlImporter::ParseVertices::is_valid(void) const
{
	return get_mid() != ~0UL && get_asemid() != ~0UL && !get_icao().empty() && size();
}

void DbXmlImporter::ParseVertices::clear(void)
{
	m_segments.clear();
	m_icao.clear();
	m_mid = ~0UL;
	m_asemid = ~0UL;
	m_width = std::numeric_limits<double>::quiet_NaN();
	m_widthscale = std::numeric_limits<double>::quiet_NaN();
	m_bdryclass = 0;
}

void DbXmlImporter::ParseVertices::set_width(const Glib::ustring& t)
{
	char *ep(0);
	set_width(strtod(t.c_str(), &ep));
	if (t.empty() || *ep)
		throw std::runtime_error("Cannot parse width: " + t);
}

void DbXmlImporter::ParseVertices::set_widthscale(const Glib::ustring& t)
{
	Glib::ustring x(t.uppercase());
	if (x == "FT") {
		set_widthscale(Point::ft_to_m_dbl * 1e-3 * Point::km_to_nmi_dbl);
		return;
	}
	if (x == "M") {
		set_widthscale(1e-3 * Point::km_to_nmi_dbl);
		return;
	}
	if (x == "KM") {
		set_widthscale(Point::km_to_nmi_dbl);
		return;
	}
	if (x == "NM") {
		set_widthscale(1);
		return;
	}
	set_widthscale();
	throw std::runtime_error("Unknown width unit: " + t);

}

void DbXmlImporter::ParseVertices::set_bdryclass(const Glib::ustring& t)
{
	ParseAirspace aspc;
	aspc.set_bdryclass(t);
	set_bdryclass(aspc.get_bdryclass());
}

Glib::ustring DbXmlImporter::ParseVertices::get_codetype(void) const
{
	ParseAirspace aspc;
	aspc.set_bdryclass(get_bdryclass());
	return aspc.get_codetype();
}

void DbXmlImporter::ParseVertices::recompute_coord2(void)
{
	for (unsigned int n = size(), j = n - 1, i = 0; i < n; j = i, ++i) {
		operator[](j).set_coord2(operator[](i).get_coord1());
	}
}

void DbXmlImporter::ParseVertices::resolve_borders(const Borders& borders)
{
	for (unsigned int i = 0; i < size(); ++i) {
		Segment& seg(operator[](i));
		if (seg.get_shape() != 'F')
			continue;
		if (seg.get_bordername().empty()) {
			std::cerr << "Warning: Airspace " << get_icao() << '/' << get_codetype() << '/' << get_mid()
				  << ": border segment but no border name" << std::endl;
			continue;
		}
		if (seg.get_bordermid() != ~0UL) {
			const Borders::Border *brd(borders.find(seg.get_bordermid(), seg.get_bordername()));
			if (brd && brd->get_nrvertex()) {
				unsigned int n(brd->get_nrvertex());
				unsigned int i1(0);
				unsigned int i2(0);
				Point pt1((*brd)[0].get_coord());
				Point pt2(pt1);
				{
					uint64_t d1(std::numeric_limits<uint64_t>::max());
					uint64_t d2(std::numeric_limits<uint64_t>::max());
					for (unsigned int i = 0; i < n; ++i) {
						const Borders::Border::Vertex& v((*brd)[i]);
						uint64_t d(seg.get_coord1().simple_distance_rel(v.get_coord()));
						if (d <= d1) {
							d1 = d;
							i1 = i;
							pt1 = v.get_coord();
						}
						d = seg.get_coord2().simple_distance_rel(v.get_coord());
						if (d <= d2) {
							d2 = d;
							i2 = i;
							pt2 = v.get_coord();
						}
						if (i < 1)
							continue;
						// check intersection
						const Borders::Border::Vertex& vp((*brd)[i-1]);
						Point pt(seg.get_coord1().spheric_nearest_on_line(vp.get_coord(), v.get_coord()));
						if (!pt.is_invalid() && pt.in_box(vp.get_coord(), v.get_coord())) {
							d = seg.get_coord1().simple_distance_rel(pt);
							if (d < d1) {
								d1 = d;
								i1 = i - 1;
								pt1 = pt;
							}
						}
						pt = seg.get_coord2().spheric_nearest_on_line(vp.get_coord(), v.get_coord());
						if (!pt.is_invalid() && pt.in_box(vp.get_coord(), v.get_coord())) {
							d = seg.get_coord2().simple_distance_rel(pt);
							if (d < d2) {
								d2 = d;
								i2 = i;
								pt2 = pt;
							}
						}
						// try non-spheric
						pt = seg.get_coord1().nearest_on_line(vp.get_coord(), v.get_coord());
						if (!pt.is_invalid() && pt.in_box(vp.get_coord(), v.get_coord())) {
							d = seg.get_coord1().simple_distance_rel(pt);
							if (d < d1) {
								d1 = d;
								i1 = i - 1;
								pt1 = pt;
							}
						}
						pt = seg.get_coord2().nearest_on_line(vp.get_coord(), v.get_coord());
						if (!pt.is_invalid() && pt.in_box(vp.get_coord(), v.get_coord())) {
							d = seg.get_coord2().simple_distance_rel(pt);
							if (d < d2) {
								d2 = d;
								i2 = i;
								pt2 = pt;
							}
						}
					}
				}
				{
					bool dumpborder(false);
					{
						double d(seg.get_coord1().spheric_distance_nmi_dbl(pt1));
						if (d >= 0.1) {
							std::cerr << "Warning: Airspace " << get_icao() << '/' << get_codetype() << '/' << get_mid()
								  << ": border " << seg.get_bordername() << " first point distance " << d << "nmi seg "
								  << seg.get_coord1() << " nearest pt " << pt1 << std::endl;
							dumpborder = true;
						}
					}
					{
						double d(seg.get_coord2().spheric_distance_nmi_dbl(pt2));
						if (d >= 0.1) {
							std::cerr << "Warning: Airspace " << get_icao() << '/' << get_codetype() << '/' << get_mid()
								  << ": border " << seg.get_bordername() << " second point distance " << d << "nmi seg "
								  << seg.get_coord2() << " nearest pt " << pt2 << std::endl;
							dumpborder = true;
						}
					}
					if (dumpborder) {
						std::cerr << "Border: " << brd->get_name() << ':';
						for (unsigned int i = 0; i < n; ++i) {
							const Borders::Border::Vertex& v((*brd)[i]);
							std::cerr << ' ' << v.get_coord();
						}
						std::cerr << std::endl;
					}
				}
				m_segments.insert(m_segments.begin() + (i + 1), Segment());
				{
					Segment& seg1(operator[](i + 1));
					// look up again after insert, may be remapped
					Segment& seg(operator[](i));
					seg1.set_coord1(pt2);
					seg1.set_coord2(seg.get_coord2());
					{
						Point pt;
						pt.set_invalid();
						seg1.set_coord0(pt);
					}
					seg1.set_shape('-');
					seg1.set_radius1(0);
					seg1.set_radius2(0);
					seg1.set_bordername("");
					seg1.set_bordermid();
					seg.set_coord2(pt1);
					{
						Point pt;
						pt.set_invalid();
						seg.set_coord0(pt);
					}
					seg.set_shape('-');
					seg.set_radius1(0);
					seg.set_radius2(0);
					seg.set_bordername("");
					seg.set_bordermid();
				}
				for (unsigned int icur = i1;;) {
					unsigned int inext;
					if (icur > i2)
						inext = icur - 1;
					else if (icur < i2)
						inext = icur + 1;
					else if (i1 == i2 && pt1 != pt2)
						inext = icur;
					else
						break;
					++i;
					m_segments.insert(m_segments.begin() + i, Segment());
					Segment& seg(operator[](i));
					seg.set_shape('-');
					seg.set_coord1((icur == i1) ? pt1 : (*brd)[icur].get_coord());
					seg.set_coord2((inext == i2) ? pt2 : (*brd)[inext].get_coord());
					{
						Point pt;
						pt.set_invalid();
						seg.set_coord0(pt);
					}
					seg.set_radius1(0);
					seg.set_radius2(0);
					seg.set_bordername("");
					seg.set_bordermid();
					icur = inext;
					if (i1 == i2)
						break;
				}
				continue;
			} else {
				std::cerr << "Warning: Airspace " << get_icao() << '/' << get_codetype() << '/' << get_mid()
					  << ": border mid " << seg.get_bordermid() << " name " << seg.get_bordername()
					  << " not found" << std::endl;
			}
		}
		Borders::FindNearest f(seg.get_coord1(), seg.get_coord2());
		std::string country0(seg.get_bordername()), country1;
		{
		        std::string::size_type n(country0.find_first_of("-_:"));
			if (n != std::string::npos) {
				country1 = country0.substr(n + 1);
				country0.erase(n);
			}
		}
		{
			const Borders::polygons_t& p(borders.find(country0));
			if (p.empty())
				std::cerr << "Warning: Airspace " << get_icao() << '/' << get_codetype() << '/' << get_mid()
					  << " Country " << country0 << " not found in borders file" << std::endl;
		        else
				f.process(p);
		}
		if (!country1.empty()) {
			const Borders::polygons_t& p(borders.find(country1));
			if (p.empty())
				std::cerr << "Warning: Airspace " << get_icao() << '/' << get_codetype() << '/' << get_mid()
					  << " Country " << country1 << " not found in borders file" << std::endl;
		        else
				f.process(p);
		}
		if (f.get_idx1() >= f.get_poly().size() || f.get_idx2() >= f.get_poly().size()) {
			std::cerr << "Warning: Airspace " << get_icao() << '/' << get_codetype() << '/' << get_mid()
				  << " Countries " << country0 << '/' << country1 << " Border not found" << std::endl;
			continue;
		}
		{
			double dist1(seg.get_coord1().spheric_distance_nmi_dbl(f.get_poly()[f.get_idx1()]));
			double dist2(seg.get_coord2().spheric_distance_nmi_dbl(f.get_poly()[f.get_idx2()]));
			std::cerr << "Border: " << country0 << '/' << country1 << " dist " << f.get_dist() << " points "
				  << seg.get_coord1() << ' ' << dist1 << "nm " << f.get_poly()[f.get_idx1()] << " - "
				  << f.get_poly()[f.get_idx2()] << ' ' << dist2 << "nm " << seg.get_coord2() << std::endl;
			if (dist1 > 10 || dist2 > 10) {
				std::cerr << "Warning: Airspace " << get_icao() << '/' << get_codetype() << '/' << get_mid()
					  << " Distance to border too big, skipping" << std::endl;
				continue;
			}
		}
		m_segments.insert(m_segments.begin() + (i + 1), Segment());
		{
			Segment& seg1(operator[](i + 1));
			// look up again after insert, may be remapped
			Segment& seg(operator[](i));
			seg1.set_coord1(f.get_poly()[f.get_idx2()]);
			seg1.set_coord2(seg.get_coord2());
			{
				Point pt;
				pt.set_invalid();
				seg1.set_coord0(pt);
			}
			seg1.set_shape('-');
			seg1.set_radius1(0);
			seg1.set_radius2(0);
			seg1.set_bordername("");
			seg1.set_bordermid();
			seg.set_coord2(f.get_poly()[f.get_idx1()]);
			{
				Point pt;
				pt.set_invalid();
				seg.set_coord0(pt);
			}
			seg.set_shape('-');
			seg.set_radius1(0);
			seg.set_radius2(0);
			seg.set_bordername("");
			seg.set_bordermid();
		}
		for (unsigned int j = f.get_idx1(); j != f.get_idx2();) {
			Point p1(f.get_poly()[j]);
			j = f.get_poly().next(j);
			Point p2(f.get_poly()[j]);
			if (p1 != p2) {
				++i;
				m_segments.insert(m_segments.begin() + i, Segment());
				Segment& seg(operator[](i));
				seg.set_coord1(p1);
				seg.set_coord2(p2);
				{
					Point pt;
					pt.set_invalid();
					seg.set_coord0(pt);
				}
				seg.set_shape('-');
				seg.set_radius1(0);
				seg.set_radius2(0);
				seg.set_bordername("");
				seg.set_bordermid();
			}
		}
	}
}

void DbXmlImporter::ParseComposite::set_operator_aixm(const Glib::ustring& opr)
{
	Glib::ustring opru(opr.uppercase());
	if (opru == "UNION") {
		set_operator(AirspacesDb::Airspace::Component::operator_union);
		return;
	}
	if (opru == "SUBTR") {
		set_operator(AirspacesDb::Airspace::Component::operator_subtract);
		return;
	}
	if (opru == "INTERS") {
		set_operator(AirspacesDb::Airspace::Component::operator_intersect);
		return;
	}
	set_operator(AirspacesDb::Airspace::Component::operator_set);
	throw std::runtime_error("Unknown composite airspace operator: " + opr);
}

void DbXmlImporter::ParseComposite::Component::set_bdryclass(const Glib::ustring& t)
{
	ParseAirspace aspc;
	aspc.set_bdryclass(t);
	set_bdryclass(aspc.get_bdryclass());
}

Glib::ustring DbXmlImporter::ParseComposite::Component::get_codetype(void) const
{
	ParseAirspace aspc;
	aspc.set_bdryclass(get_bdryclass());
	return aspc.get_codetype();
}

bool DbXmlImporter::ParseComposite::is_valid(void) const
{
	return get_mid() != ~0UL && get_asemid() != ~0UL && !get_icao().empty() && size();
}

void DbXmlImporter::ParseComposite::set_bdryclass(const Glib::ustring& t)
{
	ParseAirspace aspc;
	aspc.set_bdryclass(t);
	set_bdryclass(aspc.get_bdryclass());
}

Glib::ustring DbXmlImporter::ParseComposite::get_codetype(void) const
{
	ParseAirspace aspc;
	aspc.set_bdryclass(get_bdryclass());
	return aspc.get_codetype();
}

DbXmlImporter::AirspaceMapping::AirspaceMapping(const char *name, char bc, const char *name0, char bc0,
								  const char *name1, char bc1, const char *name2, char bc2)
{
	m_names[0] = name;
	m_bdryclass[0] = bc;
	m_names[1] = name0;
	m_bdryclass[1] = bc0;
	m_names[2] = name1;
	m_bdryclass[2] = bc1;
	m_names[3] = name2;
	m_bdryclass[3] = bc2;
}

unsigned int DbXmlImporter::AirspaceMapping::size(void) const
{
	unsigned int i(0);
	for (; i < 4; ++i)
		if (!m_names[i] || !m_bdryclass[i])
			break;
	return i;
}

bool DbXmlImporter::AirspaceMapping::operator<(const AirspaceMapping& x) const
{
	if (!m_names[0] && x.m_names[0])
		return true;
	if (m_names[0] && !x.m_names[0])
		return true;
	if (m_names[0] && x.m_names[0]) {
		int c(strcmp(m_names[0], x.m_names[0]));
		if (c)
			return c < 0;
	}
	return m_bdryclass[0] < x.m_bdryclass[0];
}

std::string DbXmlImporter::AirspaceMapping::get_name(unsigned int i) const
{
	if (i >= 4)
		return "";
	return m_names[i];
}

char DbXmlImporter::AirspaceMapping::get_bdryclass(unsigned int i) const
{
	if (i >= 4)
		return 0;
	return m_bdryclass[i];
}

DbXmlImporter::DbXmlImporter(const Glib::ustring & output_dir, const Glib::ustring& borderfile)
        : xmlpp::SaxParser(false), m_state(state_document_c), m_outputdir(output_dir), m_purgedb(false),
	  m_airportsdbopen(false), m_airspacesdbopen(false), m_navaidsdbopen(false), m_waypointsdbopen(false), m_airwaysdbopen(false),
	  m_topodbopen(false), m_starttime(0), m_borders(borderfile)
{
	time(&m_starttime);
	check_airspace_mapping();
}

DbXmlImporter::~DbXmlImporter()
{
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
		if (name == "AIXM-Snapshot") {
			m_state = state_aixmsnapshot_c;
			return;
		}
		break;

	case state_aixmsnapshot_c:
		if (name == "Gbr") {
			m_state = state_gbr_c;
			m_curborder.clear();
			return;
		}
		if (name == "Ase") {
			m_state = state_ase_c;
			m_curairspace.clear();
			return;
		}
		if (name == "Adg") {
			m_state = state_adg_c;
			m_composites.push_back(ParseComposite());
			return;
		}
		if (name == "Abd") {
			m_state = state_abd_c;
			m_curvertices.clear();
			return;
		}
		if (name == "Acr") {
			m_state = state_acr_c;
			m_curvertices.clear();
			return;
		}
		if (name == "Aas") {
			m_state = state_aas_c;
			return;
		}
		break;

	case state_gbr_c:
		if (name == "GbrUid") {
			m_state = state_gbr_uid_c;
			for (AttributeList::const_iterator i(properties.begin()), e(properties.end()); i != e; ++i) {
				if (i->name != "mid")
					continue;
				char *ep(0);
				m_curborder.set_mid(strtoul(i->value.c_str(), &ep, 10));
				if (*ep)
					throw std::runtime_error("XML Parser: cannot parse mid " + i->value);
			}
			return;
		}
		if (name == "codeType") {
			m_state = state_gbr_codetype_c;
			return;
		}
		if (name == "txtRmk") {
			m_state = state_gbr_txtrmk_c;
			return;
		}
		if (name == "Gbv") {
			m_state = state_gbr_gbv_c;
			m_curborder.add_vertex(Borders::Border::Vertex());
			return;
		}
		break;

	case state_gbr_uid_c:
		if (name == "txtName") {
			m_state = state_gbr_uid_txtname_c;
			return;
		}
		break;

	case state_gbr_gbv_c:
		if (name == "codeType") {
			m_state = state_gbr_gbv_codetype_c;
			m_characters.clear();
			return;
		}
		if (name == "geoLat") {
			m_state = state_gbr_gbv_geolat_c;
			m_characters.clear();
			return;
		}
		if (name == "geoLong") {
			m_state = state_gbr_gbv_geolon_c;
			m_characters.clear();
			return;
		}
		if (name == "codeDatum") {
			m_state = state_gbr_gbv_codedatum_c;
			m_characters.clear();
			return;
		}
		if (name == "valCrc") {
			m_state = state_gbr_gbv_valcrc_c;
			m_characters.clear();
			return;
		}
		if (name == "txtRmk") {
			m_state = state_gbr_gbv_txtrmk_c;
			m_characters.clear();
			return;
		}
		break;

	case state_ase_c:
		if (name == "AseUid") {
			m_state = state_ase_aseuid_c;
			for (AttributeList::const_iterator i(properties.begin()), e(properties.end()); i != e; ++i) {
				if (i->name != "mid")
					continue;
				char *ep(0);
				m_curairspace.set_mid(strtoul(i->value.c_str(), &ep, 10));
				if (*ep)
					throw std::runtime_error("XML Parser: cannot parse mid " + i->value);
			}
			return;
		}
		if (name == "UasUid") {
			m_state = state_ase_uasuid_c;
			return;
		}
		if (name == "RsgUid") {
			m_state = state_ase_rsguid_c;
			return;
		}
		if (name == "txtLocalType") {
			m_state = state_ase_txtlocaltype_c;
			m_characters.clear();
			return;
		}
		if (name == "txtName") {
			m_state = state_ase_txtname_c;
			m_characters.clear();
			return;
		}
		if (name == "txtRmk") {
			m_state = state_ase_txtrmk_c;
			m_characters.clear();
			return;
		}
		if (name == "codeActivity") {
			m_state = state_ase_codeactivity_c;
			m_characters.clear();
			return;
		}
		if (name == "codeClass") {
			m_state = state_ase_codeclass_c;
			m_characters.clear();
			return;
		}
		if (name == "codeLocInd") {
			m_state = state_ase_codelocind_c;
			m_characters.clear();
			return;
		}
		if (name == "codeMil") {
			m_state = state_ase_codemil_c;
			m_characters.clear();
			return;
		}
		if (name == "codeDistVerUpper") {
			m_state = state_ase_codedistverupper_c;
			m_characters.clear();
			return;
		}
		if (name == "valDistVerUpper") {
			m_state = state_ase_valdistverupper_c;
			m_characters.clear();
			return;
		}
		if (name == "uomDistVerUpper") {
			m_state = state_ase_uomdistverupper_c;
			m_characters.clear();
			return;
		}
		if (name == "codeDistVerLower") {
			m_state = state_ase_codedistverlower_c;
			m_characters.clear();
			return;
		}
		if (name == "valDistVerLower") {
			m_state = state_ase_valdistverlower_c;
			m_characters.clear();
			return;
		}
		if (name == "uomDistVerLower") {
			m_state = state_ase_uomdistverlower_c;
			m_characters.clear();
			return;
		}
		if (name == "codeDistVerMnm") {
			m_state = state_ase_codedistvermnm_c;
			m_characters.clear();
			return;
		}
		if (name == "valDistVerMnm") {
			m_state = state_ase_valdistvermnm_c;
			m_characters.clear();
			return;
		}
		if (name == "uomDistVerMnm") {
			m_state = state_ase_uomdistvermnm_c;
			m_characters.clear();
			return;
		}
		if (name == "codeDistVerMax") {
			m_state = state_ase_codedistvermax_c;
			m_characters.clear();
			return;
		}
		if (name == "valDistVerMax") {
			m_state = state_ase_valdistvermax_c;
			m_characters.clear();
			return;
		}
		if (name == "uomDistVerMax") {
			m_state = state_ase_uomdistvermax_c;
			m_characters.clear();
			return;
		}
		if (name == "Att") {
			m_state = state_ase_att_c;
			return;
		}
		break;

	case state_ase_aseuid_c:
		if (name == "codeType") {
			m_state = state_ase_aseuid_codetype_c;
			m_characters.clear();
			return;
		}
		if (name == "codeId") {
			m_state = state_ase_aseuid_codeid_c;
			m_characters.clear();
			return;
		}
		break;

	case state_ase_uasuid_c:
		if (name == "UniUid") {
			m_state = state_ase_uasuid_uniuid_c;
			return;
		}
		if (name == "codeType") {
			m_state = state_ase_uasuid_codetype_c;
			m_characters.clear();
			return;
		}
		if (name == "noSeq") {
			m_state = state_ase_uasuid_noseq_c;
			m_characters.clear();
			return;
		}
		break;

	case state_ase_uasuid_uniuid_c:
		if (name == "txtName") {
			m_state = state_ase_uasuid_uniuid_txtname_c;
			m_characters.clear();
			return;
		}
		break;

	case state_ase_rsguid_c:
		if (name == "RteUid") {
			m_state = state_ase_rsguid_rteuid_c;
			return;
		}
		if (name == "DpnUidSta") {
			m_state = state_ase_rsguid_dpnuidsta_c;
			return;
		}
		if (name == "DpnUidEnd") {
			m_state = state_ase_rsguid_dpnuidend_c;
			return;
		}
		break;

	case state_ase_rsguid_rteuid_c:
		if (name == "txtDesig") {
			m_state = state_ase_rsguid_rteuid_txtdesig_c;
			m_characters.clear();
			return;
		}
		if (name == "txtLocDesig") {
			m_state = state_ase_rsguid_rteuid_txtlocdesig_c;
			m_characters.clear();
			return;
		}
		break;

	case state_ase_rsguid_dpnuidsta_c:
		if (name == "codeId") {
			m_state = state_ase_rsguid_dpnuidsta_codeid_c;
			m_characters.clear();
			return;
		}
		if (name == "geoLat") {
			m_state = state_ase_rsguid_dpnuidsta_geolat_c;
			m_characters.clear();
			return;
		}
		if (name == "geoLong") {
			m_state = state_ase_rsguid_dpnuidsta_geolon_c;
			m_characters.clear();
			return;
		}
		break;

	case state_ase_rsguid_dpnuidend_c:
		if (name == "codeId") {
			m_state = state_ase_rsguid_dpnuidend_codeid_c;
			m_characters.clear();
			return;
		}
		if (name == "geoLat") {
			m_state = state_ase_rsguid_dpnuidend_geolat_c;
			m_characters.clear();
			return;
		}
		if (name == "geoLong") {
			m_state = state_ase_rsguid_dpnuidend_geolon_c;
			m_characters.clear();
			return;
		}
		break;

	case state_ase_att_c:
		if (name == "codeWorkHr") {
			m_state = state_ase_att_codeworkhr_c;
			m_characters.clear();
			return;
		}
		if (name == "txtRmkWorkHr") {
			m_state = state_ase_att_txtrmkworkhr_c;
			m_characters.clear();
			if (!m_curairspace.get_efftime().empty())
				m_curairspace.set_efftime(m_curairspace.get_efftime() + "; ");
			return;
		}
		if (name == "Timsh") {
			m_state = state_ase_att_timsh_c;
			m_curtimesheet.clear();
			return;
		}
		break;

	case state_ase_att_timsh_c:
		if (name == "codeTimeRef") {
			m_state = state_ase_att_timsh_codetimeref_c;
			return;
		}
		if (name == "dateValidWef") {
			m_state = state_ase_att_timsh_datevalidwef_c;
			return;
		}
		if (name == "dateValidTil") {
			m_state = state_ase_att_timsh_datevalidtil_c;
			return;
		}
		if (name == "codeDay") {
			m_state = state_ase_att_timsh_codeday_c;
			return;
		}
		if (name == "codeDayTil") {
			m_state = state_ase_att_timsh_codedaytil_c;
			return;
		}
		if (name == "timeWef") {
			m_state = state_ase_att_timsh_timewef_c;
			return;
		}
		if (name == "timeTil") {
			m_state = state_ase_att_timsh_timetil_c;
			return;
		}
		if (name == "codeEventWef") {
			m_state = state_ase_att_timsh_codeeventwef_c;
			return;
		}
		if (name == "codeEventTil") {
			m_state = state_ase_att_timsh_codeeventtil_c;
			return;
		}
		if (name == "timeRelEventWef") {
			m_state = state_ase_att_timsh_timereleventwef_c;
			return;
		}
		if (name == "timeRelEventTil") {
			m_state = state_ase_att_timsh_timereleventtil_c;
			return;
		}
		if (name == "codeCombWef") {
			m_state = state_ase_att_timsh_codecombwef_c;
			return;
		}
		if (name == "codeCombTil") {
			m_state = state_ase_att_timsh_codecombtil_c;
			return;
		}
		break;

	case state_adg_c:
		if (name == "AdgUid") {
			m_state = state_adg_adguid_c;
			if (!m_composites.size())
				throw std::runtime_error("No composite airspaces");
			for (AttributeList::const_iterator i(properties.begin()), e(properties.end()); i != e; ++i) {
				if (i->name != "mid")
					continue;
				char *ep(0);
			        m_composites.back().set_mid(strtoul(i->value.c_str(), &ep, 10));
				if (*ep)
					throw std::runtime_error("XML Parser: cannot parse mid " + i->value);
			}
			return;
		}
		if (name == "AseUidBase") {
			m_state = state_adg_aseuidbase_c;
			if (!m_composites.size())
				throw std::runtime_error("No composite airspaces");
			{
				ParseComposite::Component comp;
				comp.set_operator(m_composites.back().get_operator());
				for (AttributeList::const_iterator i(properties.begin()), e(properties.end()); i != e; ++i) {
					if (i->name != "mid")
						continue;
					char *ep(0);
					comp.set_mid(strtoul(i->value.c_str(), &ep, 10));
					if (*ep)
						throw std::runtime_error("XML Parser: cannot parse mid " + i->value);
				}
				m_composites.back().append_component(comp);
			}
			return;
		}
		if (name == "AseUidComponent") {
			m_state = state_adg_aseuidcomponent_c;
			if (!m_composites.size())
				throw std::runtime_error("No composite airspaces");
			{
				ParseComposite::Component comp;
				comp.set_operator(m_composites.back().get_operator());
				for (AttributeList::const_iterator i(properties.begin()), e(properties.end()); i != e; ++i) {
					if (i->name != "mid")
						continue;
					char *ep(0);
					comp.set_mid(strtoul(i->value.c_str(), &ep, 10));
					if (*ep)
						throw std::runtime_error("XML Parser: cannot parse mid " + i->value);
				}
				m_composites.back().append_component(comp);
			}
			return;
		}
		if (name == "codeOpr") {
			m_state = state_adg_codeopr_c;
			m_characters.clear();
			return;
		}
		if (name == "AseUidSameExtent") {
			m_state = state_adg_aseuidsameextent_c;
			if (!m_composites.size())
				throw std::runtime_error("No composite airspaces");
			{
				ParseComposite::Component comp;
				comp.set_operator(m_composites.back().get_operator());
				for (AttributeList::const_iterator i(properties.begin()), e(properties.end()); i != e; ++i) {
					if (i->name != "mid")
						continue;
					char *ep(0);
					comp.set_mid(strtoul(i->value.c_str(), &ep, 10));
					if (*ep)
						throw std::runtime_error("XML Parser: cannot parse mid " + i->value);
				}
				m_composites.back().append_component(comp);
			}
			return;
		}
		if (name == "txtRmk") {
			m_state = state_adg_txtrmk_c;
			return;
		}
		break;

	case state_adg_adguid_c:
		if (name == "AseUid") {
			m_state = state_adg_adguid_aseuid_c;
			if (!m_composites.size())
				throw std::runtime_error("No composite airspaces");
			for (AttributeList::const_iterator i(properties.begin()), e(properties.end()); i != e; ++i) {
				if (i->name != "mid")
					continue;
				char *ep(0);
			        m_composites.back().set_asemid(strtoul(i->value.c_str(), &ep, 10));
				if (*ep)
					throw std::runtime_error("XML Parser: cannot parse mid " + i->value);
			}
			return;
		}
		break;

	case state_adg_adguid_aseuid_c:
		if (name == "codeType") {
			m_state = state_adg_adguid_aseuid_codetype_c;
			m_characters.clear();
			return;
		}
		if (name == "codeId") {
			m_state = state_adg_adguid_aseuid_codeid_c;
			return;
		}
		break;

	case state_adg_aseuidbase_c:
		if (name == "codeType") {
			m_state = state_adg_aseuidbase_codetype_c;
			m_characters.clear();
			return;
		}
		if (name == "codeId") {
			m_state = state_adg_aseuidbase_codeid_c;
			return;
		}
		break;

	case state_adg_aseuidcomponent_c:
		if (name == "codeType") {
			m_state = state_adg_aseuidcomponent_codetype_c;
			m_characters.clear();
			return;
		}
		if (name == "codeId") {
			m_state = state_adg_aseuidcomponent_codeid_c;
			return;
		}
		break;

	case state_adg_aseuidsameextent_c:
		if (name == "codeType") {
			m_state = state_adg_aseuidsameextent_codetype_c;
			m_characters.clear();
			return;
		}
		if (name == "codeId") {
			m_state = state_adg_aseuidsameextent_codeid_c;
			return;
		}
		break;

	case state_abd_c:
		if (name == "AbdUid") {
			m_state = state_abd_abduid_c;
			for (AttributeList::const_iterator i(properties.begin()), e(properties.end()); i != e; ++i) {
				if (i->name != "mid")
					continue;
				char *ep(0);
			        m_curvertices.set_mid(strtoul(i->value.c_str(), &ep, 10));
				if (*ep)
					throw std::runtime_error("XML Parser: cannot parse mid " + i->value);
			}
			return;
		}
		if (name == "Avx") {
			m_state = state_abd_avx_c;
			m_curvertices.append_segment();
			return;
		}
		if (name == "Circle") {
			m_state = state_abd_circle_c;
			m_curvertices.append_segment();
			m_curvertices.back().set_shape('C');
			return;
		}
		if (name == "txtRmk") {
			m_state = state_abd_txtrmk_c;
			m_characters.clear();
			return;
		}
		break;

	case state_abd_abduid_c:
		if (name == "AseUid") {
			m_state = state_abd_abduid_aseuid_c;
			for (AttributeList::const_iterator i(properties.begin()), e(properties.end()); i != e; ++i) {
				if (i->name != "mid")
					continue;
				char *ep(0);
				m_curvertices.set_asemid(strtoul(i->value.c_str(), &ep, 10));
				if (*ep)
					throw std::runtime_error("XML Parser: cannot parse mid " + i->value);
			}
			return;
		}
		break;

	case state_abd_abduid_aseuid_c:
		if (name == "codeType") {
			m_state = state_abd_abduid_aseuid_codetype_c;
			m_characters.clear();
			return;
		}
		if (name == "codeId") {
			m_state = state_abd_abduid_aseuid_codeid_c;
			return;
		}
		break;

	case state_abd_avx_c:
		if (name == "GbrUid") {
			m_state = state_abd_avx_gbruid_c;
			if (!m_curvertices.size())
				throw std::runtime_error("Vertex Set does not contain a segment");
			for (AttributeList::const_iterator i(properties.begin()), e(properties.end()); i != e; ++i) {
				if (i->name != "mid")
					continue;
				char *ep(0);
				m_curvertices.back().set_bordermid(strtoul(i->value.c_str(), &ep, 10));
				if (*ep)
					throw std::runtime_error("XML Parser: cannot parse mid " + i->value);
			}
			return;
		}
		if (name == "DpnUidSpn") {
			m_state = state_abd_avx_dpnuidspn_c;
			return;
		}
		if (name == "DmeUidSpn") {
			m_state = state_abd_avx_dmeuidspn_c;
			return;
		}
		if (name == "TcnUidSpn") {
			m_state = state_abd_avx_tcnuidspn_c;
			return;
		}
		if (name == "NdbUidSpn") {
			m_state = state_abd_avx_ndbuidspn_c;
			return;
		}
		if (name == "VorUidSpn") {
			m_state = state_abd_avx_voruidspn_c;
			return;
		}
		if (name == "DmeUidCen") {
			m_state = state_abd_avx_dmeuidcen_c;
			return;
		}
		if (name == "TcnUidCen") {
			m_state = state_abd_avx_tcnuidcen_c;
			return;
		}
		if (name == "NdbUidCen") {
			m_state = state_abd_avx_ndbuidcen_c;
			return;
		}
		if (name == "VorUidCen") {
			m_state = state_abd_avx_voruidcen_c;
			return;
		}
		if (name == "codeType") {
			m_state = state_abd_avx_codetype_c;
			m_characters.clear();
			return;
		}
		if (name == "geoLat") {
			m_state = state_abd_avx_geolat_c;
			m_characters.clear();
			return;
		}
		if (name == "geoLong") {
			m_state = state_abd_avx_geolon_c;
			m_characters.clear();
			return;
		}
		if (name == "codeDatum") {
			m_state = state_abd_avx_codedatum_c;
			m_characters.clear();
			return;
		}
		if (name == "geoLatArc") {
			m_state = state_abd_avx_geolatarc_c;
			m_characters.clear();
			return;
		}
		if (name == "geoLongArc") {
			m_state = state_abd_avx_geolonarc_c;
			m_characters.clear();
			return;
		}
		if (name == "valRadiusArc") {
			m_state = state_abd_avx_valradiusarc_c;
			m_characters.clear();
			return;
		}
		if (name == "uomRadiusArc") {
			m_state = state_abd_avx_uomradiusarc_c;
			m_characters.clear();
			return;
		}
		if (name == "valGeoAccuracy") {
			m_state = state_abd_avx_valgeoaccuracy_c;
			return;
		}
		if (name == "uomGeoAccuracy") {
			m_state = state_abd_avx_uomgeoaccuracy_c;
			return;
		}
		if (name == "valCrc") {
			m_state = state_abd_avx_valcrc_c;
			return;
		}
		if (name == "txtRmk") {
			m_state = state_abd_avx_txtrmk_c;
			return;
		}
		break;

	case state_abd_avx_gbruid_c:
		if (name == "txtName") {
			m_state = state_abd_avx_gbruid_txtname_c;
			m_characters.clear();
			return;
		}
		break;

	case state_abd_avx_dpnuidspn_c:
		if (name == "codeId") {
			m_state = state_abd_avx_dpnuidspn_codeid_c;
			m_characters.clear();
			return;
		}
		if (name == "geoLat") {
			m_state = state_abd_avx_dpnuidspn_geolat_c;
			m_characters.clear();
			return;
		}
		if (name == "geoLong") {
			m_state = state_abd_avx_dpnuidspn_geolon_c;
			m_characters.clear();
			return;
		}
		break;

	case state_abd_avx_dmeuidspn_c:
		if (name == "codeId") {
			m_state = state_abd_avx_dmeuidspn_codeid_c;
			return;
		}
		if (name == "geoLat") {
			m_state = state_abd_avx_dmeuidspn_geolat_c;
			return;
		}
		if (name == "geoLong") {
			m_state = state_abd_avx_dmeuidspn_geolon_c;
			return;
		}
		break;

	case state_abd_avx_tcnuidspn_c:
		if (name == "codeId") {
			m_state = state_abd_avx_tcnuidspn_codeid_c;
			return;
		}
		if (name == "geoLat") {
			m_state = state_abd_avx_tcnuidspn_geolat_c;
			return;
		}
		if (name == "geoLong") {
			m_state = state_abd_avx_tcnuidspn_geolon_c;
			return;
		}
		break;

	case state_abd_avx_ndbuidspn_c:
		if (name == "codeId") {
			m_state = state_abd_avx_ndbuidspn_codeid_c;
			return;
		}
		if (name == "geoLat") {
			m_state = state_abd_avx_ndbuidspn_geolat_c;
			return;
		}
		if (name == "geoLong") {
			m_state = state_abd_avx_ndbuidspn_geolon_c;
			return;
		}
		break;

	case state_abd_avx_voruidspn_c:
		if (name == "codeId") {
			m_state = state_abd_avx_voruidspn_codeid_c;
			return;
		}
		if (name == "geoLat") {
			m_state = state_abd_avx_voruidspn_geolat_c;
			return;
		}
		if (name == "geoLong") {
			m_state = state_abd_avx_voruidspn_geolon_c;
			return;
		}
		break;

	case state_abd_avx_dmeuidcen_c:
		if (name == "codeId") {
			m_state = state_abd_avx_dmeuidcen_codeid_c;
			return;
		}
		if (name == "geoLat") {
			m_state = state_abd_avx_dmeuidcen_geolat_c;
			return;
		}
		if (name == "geoLong") {
			m_state = state_abd_avx_dmeuidcen_geolon_c;
			return;
		}
		break;

	case state_abd_avx_tcnuidcen_c:
		if (name == "codeId") {
			m_state = state_abd_avx_tcnuidcen_codeid_c;
			return;
		}
		if (name == "geoLat") {
			m_state = state_abd_avx_tcnuidcen_geolat_c;
			return;
		}
		if (name == "geoLong") {
			m_state = state_abd_avx_tcnuidcen_geolon_c;
			return;
		}
		break;

	case state_abd_avx_ndbuidcen_c:
		if (name == "codeId") {
			m_state = state_abd_avx_ndbuidcen_codeid_c;
			return;
		}
		if (name == "geoLat") {
			m_state = state_abd_avx_ndbuidcen_geolat_c;
			return;
		}
		if (name == "geoLong") {
			m_state = state_abd_avx_ndbuidcen_geolon_c;
			return;
		}
		break;

	case state_abd_avx_voruidcen_c:
		if (name == "codeId") {
			m_state = state_abd_avx_voruidcen_codeid_c;
			return;
		}
		if (name == "geoLat") {
			m_state = state_abd_avx_voruidcen_geolat_c;
			return;
		}
		if (name == "geoLong") {
			m_state = state_abd_avx_voruidcen_geolon_c;
			return;
		}
		break;

	case state_abd_circle_c:
		if (name == "DmeUidCen") {
			m_state = state_abd_circle_dmeuidcen_c;
			return;
		}
		if (name == "TcnUidCen") {
			m_state = state_abd_circle_tcnuidcen_c;
			return;
		}
		if (name == "NdbUidCen") {
			m_state = state_abd_circle_ndbuidcen_c;
			return;
		}
		if (name == "VorUidCen") {
			m_state = state_abd_circle_voruidcen_c;
			return;
		}
		if (name == "geoLatCen") {
			m_state = state_abd_circle_geolatcen_c;
			m_characters.clear();
			return;
		}
		if (name == "geoLongCen") {
			m_state = state_abd_circle_geoloncen_c;
			m_characters.clear();
			return;
		}
		if (name == "codeDatum") {
			m_state = state_abd_circle_codedatum_c;
			m_characters.clear();
			return;
		}
		if (name == "valRadius") {
			m_state = state_abd_circle_valradius_c;
			m_characters.clear();
			return;
		}
		if (name == "uomRadius") {
			m_state = state_abd_circle_uomradius_c;
			m_characters.clear();
			return;
		}
		if (name == "valGeoAccuracy") {
			m_state = state_abd_circle_valgeoaccuracy_c;
			return;
		}
		if (name == "uomGeoAccuracy") {
			m_state = state_abd_circle_uomgeoaccuracy_c;
			return;
		}
		if (name == "valCrc") {
			m_state = state_abd_circle_valcrc_c;
			return;
		}
		if (name == "txtRmk") {
			m_state = state_abd_circle_txtrmk_c;
			return;
		}
		break;

	case state_abd_circle_dmeuidcen_c:
		if (name == "codeId") {
			m_state = state_abd_circle_dmeuidcen_codeid_c;
			return;
		}
		if (name == "geoLat") {
			m_state = state_abd_circle_dmeuidcen_geolat_c;
			return;
		}
		if (name == "geoLong") {
			m_state = state_abd_circle_dmeuidcen_geolon_c;
			return;
		}
		break;

	case state_abd_circle_tcnuidcen_c:
		if (name == "codeId") {
			m_state = state_abd_circle_tcnuidcen_codeid_c;
			return;
		}
		if (name == "geoLat") {
			m_state = state_abd_circle_tcnuidcen_geolat_c;
			return;
		}
		if (name == "geoLong") {
			m_state = state_abd_circle_tcnuidcen_geolon_c;
			return;
		}
		break;

	case state_abd_circle_ndbuidcen_c:
		if (name == "codeId") {
			m_state = state_abd_circle_ndbuidcen_codeid_c;
			return;
		}
		if (name == "geoLat") {
			m_state = state_abd_circle_ndbuidcen_geolat_c;
			return;
		}
		if (name == "geoLong") {
			m_state = state_abd_circle_ndbuidcen_geolon_c;
			return;
		}
		break;

	case state_abd_circle_voruidcen_c:
		if (name == "codeId") {
			m_state = state_abd_circle_voruidcen_codeid_c;
			return;
		}
		if (name == "geoLat") {
			m_state = state_abd_circle_voruidcen_geolat_c;
			return;
		}
		if (name == "geoLong") {
			m_state = state_abd_circle_voruidcen_geolon_c;
			return;
		}
		break;

	case state_acr_c:
		if (name == "AcrUid") {
			m_state = state_acr_acruid_c;
			for (AttributeList::const_iterator i(properties.begin()), e(properties.end()); i != e; ++i) {
				if (i->name != "mid")
					continue;
				char *ep(0);
			        m_curvertices.set_mid(strtoul(i->value.c_str(), &ep, 10));
				if (*ep)
					throw std::runtime_error("XML Parser: cannot parse mid " + i->value);
			}
			return;
		}
		if (name == "Avx") {
			m_state = state_acr_avx_c;
			m_curvertices.append_segment();
			return;
		}
		if (name == "valWidth") {
			m_state = state_acr_valwidth_c;
			m_characters.clear();
			return;
		}
		if (name == "uomWidth") {
			m_state = state_acr_uomwidth_c;
			m_characters.clear();
			return;
		}
		if (name == "txtRmk") {
			m_state = state_acr_txtrmk_c;
			m_characters.clear();
			return;
		}
		break;

	case state_acr_acruid_c:
		if (name == "AseUid") {
			m_state = state_acr_acruid_aseuid_c;
			for (AttributeList::const_iterator i(properties.begin()), e(properties.end()); i != e; ++i) {
				if (i->name != "mid")
					continue;
				char *ep(0);
			        m_curvertices.set_asemid(strtoul(i->value.c_str(), &ep, 10));
				if (*ep)
					throw std::runtime_error("XML Parser: cannot parse mid " + i->value);
			}
			return;
		}
		break;

	case state_acr_acruid_aseuid_c:
		if (name == "codeType") {
			m_state = state_acr_acruid_aseuid_codetype_c;
			return;
		}
		if (name == "codeId") {
			m_state = state_acr_acruid_aseuid_codeid_c;
			return;
		}
		break;

	case state_acr_avx_c:
		if (name == "GbrUid") {
			m_state = state_acr_avx_gbruid_c;
			if (!m_curvertices.size())
				throw std::runtime_error("Vertex Set does not contain a segment");
			for (AttributeList::const_iterator i(properties.begin()), e(properties.end()); i != e; ++i) {
				if (i->name != "mid")
					continue;
				char *ep(0);
				m_curvertices.back().set_bordermid(strtoul(i->value.c_str(), &ep, 10));
				if (*ep)
					throw std::runtime_error("XML Parser: cannot parse mid " + i->value);
			}
			return;
		}
		if (name == "DpnUidSpn") {
			m_state = state_acr_avx_dpnuidspn_c;
			return;
		}
		if (name == "DmeUidSpn") {
			m_state = state_acr_avx_dmeuidspn_c;
			return;
		}
		if (name == "TcnUidSpn") {
			m_state = state_acr_avx_tcnuidspn_c;
			return;
		}
		if (name == "NdbUidSpn") {
			m_state = state_acr_avx_ndbuidspn_c;
			return;
		}
		if (name == "VorUidSpn") {
			m_state = state_acr_avx_voruidspn_c;
			return;
		}
		if (name == "codeType") {
			m_state = state_acr_avx_codetype_c;
			m_characters.clear();
			return;
		}
		if (name == "geoLat") {
			m_state = state_acr_avx_geolat_c;
			m_characters.clear();
			return;
		}
		if (name == "geoLong") {
			m_state = state_acr_avx_geolon_c;
			m_characters.clear();
			return;
		}
		if (name == "codeDatum") {
			m_state = state_acr_avx_codedatum_c;
			m_characters.clear();
			return;
		}
		if (name == "geoLatArc") {
			m_state = state_acr_avx_geolatarc_c;
			m_characters.clear();
			return;
		}
		if (name == "geoLongArc") {
			m_state = state_acr_avx_geolonarc_c;
			m_characters.clear();
			return;
		}
		if (name == "valRadiusArc") {
			m_state = state_acr_avx_valradiusarc_c;
			m_characters.clear();
			return;
		}
		if (name == "uomRadiusArc") {
			m_state = state_acr_avx_uomradiusarc_c;
			m_characters.clear();
			return;
		}
		if (name == "valGeoAccuracy") {
			m_state = state_acr_avx_valgeoaccuracy_c;
			return;
		}
		if (name == "uomGeoAccuracy") {
			m_state = state_acr_avx_uomgeoaccuracy_c;
			return;
		}
		if (name == "valCrc") {
			m_state = state_acr_avx_valcrc_c;
			return;
		}
		if (name == "txtRmk") {
			m_state = state_acr_avx_txtrmk_c;
			return;
		}
		break;

	case state_acr_avx_gbruid_c:
		if (name == "txtName") {
			m_state = state_acr_avx_gbruid_txtname_c;
			m_characters.clear();
			return;
		}
		break;

	case state_acr_avx_dpnuidspn_c:
		if (name == "codeId") {
			m_state = state_acr_avx_dpnuidspn_codeid_c;
			m_characters.clear();
			return;
		}
		if (name == "geoLat") {
			m_state = state_acr_avx_dpnuidspn_geolat_c;
			m_characters.clear();
			return;
		}
		if (name == "geoLong") {
			m_state = state_acr_avx_dpnuidspn_geolon_c;
			m_characters.clear();
			return;
		}
		break;

	case state_acr_avx_dmeuidspn_c:
		if (name == "codeId") {
			m_state = state_acr_avx_dmeuidspn_codeid_c;
			return;
		}
		if (name == "geoLat") {
			m_state = state_acr_avx_dmeuidspn_geolat_c;
			return;
		}
		if (name == "geoLong") {
			m_state = state_acr_avx_dmeuidspn_geolon_c;
			return;
		}
		break;

	case state_acr_avx_tcnuidspn_c:
		if (name == "codeId") {
			m_state = state_acr_avx_tcnuidspn_codeid_c;
			return;
		}
		if (name == "geoLat") {
			m_state = state_acr_avx_tcnuidspn_geolat_c;
			return;
		}
		if (name == "geoLong") {
			m_state = state_acr_avx_tcnuidspn_geolon_c;
			return;
		}
		break;

	case state_acr_avx_ndbuidspn_c:
		if (name == "codeId") {
			m_state = state_acr_avx_ndbuidspn_codeid_c;
			return;
		}
		if (name == "geoLat") {
			m_state = state_acr_avx_ndbuidspn_geolat_c;
			return;
		}
		if (name == "geoLong") {
			m_state = state_acr_avx_ndbuidspn_geolon_c;
			return;
		}
		break;

	case state_acr_avx_voruidspn_c:
		if (name == "codeId") {
			m_state = state_acr_avx_voruidspn_codeid_c;
			return;
		}
		if (name == "geoLat") {
			m_state = state_acr_avx_voruidspn_geolat_c;
			return;
		}
		if (name == "geoLong") {
			m_state = state_acr_avx_voruidspn_geolon_c;
			return;
		}
		break;

	case state_aas_c:
		if (name == "AasUid") {
			m_state = state_aas_aasuid_c;
			return;
		}
		if (name == "txtRmk") {
			m_state = state_aas_txtrmk_c;
			m_characters.clear();
			return;
		}
		break;

	case state_aas_aasuid_c:
		if (name == "AseUid1") {
			m_state = state_aas_aasuid_aseuid1_c;
			return;
		}
		if (name == "AseUid2") {
			m_state = state_aas_aasuid_aseuid2_c;
			return;
		}
		if (name == "codeType") {
			m_state = state_aas_aasuid_codetype_c;
			m_characters.clear();
			return;
		}
		break;

	case state_aas_aasuid_aseuid1_c:
		if (name == "codeType") {
			m_state = state_aas_aasuid_aseuid1_codetype_c;
			m_characters.clear();
			return;
		}
		if (name == "codeId") {
			m_state = state_aas_aasuid_aseuid1_codeid_c;
			m_characters.clear();
			return;
		}
		break;

	case state_aas_aasuid_aseuid2_c:
		if (name == "codeType") {
			m_state = state_aas_aasuid_aseuid2_codetype_c;
			m_characters.clear();
			return;
		}
		if (name == "codeId") {
			m_state = state_aas_aasuid_aseuid2_codeid_c;
			m_characters.clear();
			return;
		}
		break;

	default:
		break;
        }
        std::ostringstream oss;
        oss << "XML Parser: Invalid element " << name << " in state " << (int)m_state;
	if (true)
		std::cerr << oss.str() << std::endl;
        throw std::runtime_error(oss.str());
}

void DbXmlImporter::on_end_element(const Glib::ustring& name)
{
	if (false)
		std::cerr << "on_end_element: " << name << " state " << m_state << std::endl;
        switch (m_state) {
	case state_aixmsnapshot_c:
		m_state = state_document_c;
		process_composite_airspaces();
		return;

	case state_gbr_c:
		m_state = state_aixmsnapshot_c;
		if (!m_curborder.is_valid())
			throw std::runtime_error("Invalid border");
		m_borders.add(m_curborder);
		m_curborder.clear();
		m_characters.clear();
		return;

	case state_gbr_uid_c:
		m_state = state_gbr_c;
		return;

	case state_gbr_uid_txtname_c:
		m_state = state_gbr_uid_c;
		return;

	case state_gbr_codetype_c:
		m_state = state_gbr_c;
		return;

	case state_gbr_txtrmk_c:
		m_state = state_gbr_c;
		return;

	case state_gbr_gbv_c:
		m_state = state_gbr_c;
		return;

	case state_gbr_gbv_codetype_c:
		m_state = state_gbr_gbv_c;
		if (!m_curborder.get_nrvertex())
			throw std::runtime_error("Border does not contain a vertex");
		m_curborder.back().set_type(m_characters);
		return;

	case state_gbr_gbv_geolat_c:
		m_state = state_gbr_gbv_c;
		if (!m_curborder.get_nrvertex())
			throw std::runtime_error("Border does not contain a vertex");
		m_curborder.back().set_lat(m_characters);
		m_characters.clear();
		return;

	case state_gbr_gbv_geolon_c:
		m_state = state_gbr_gbv_c;
		if (!m_curborder.get_nrvertex())
			throw std::runtime_error("Border does not contain a vertex");
        	m_curborder.back().set_lon(m_characters);
		m_characters.clear();
		return;

	case state_gbr_gbv_codedatum_c:
		m_state = state_gbr_gbv_c;
		return;

	case state_gbr_gbv_valcrc_c:
		m_state = state_gbr_gbv_c;
		return;

	case state_gbr_gbv_txtrmk_c:
		m_state = state_gbr_gbv_c;
		return;

	case state_ase_c:
		m_state = state_aixmsnapshot_c;
		process_airspace();
		m_curairspace.clear();
		m_characters.clear();
		return;

	case state_ase_aseuid_c:
		m_state = state_ase_c;
		return;

	case state_ase_aseuid_codetype_c:
		m_state = state_ase_aseuid_c;
		m_curairspace.set_bdryclass(m_characters);
		return;

	case state_ase_aseuid_codeid_c:
		m_state = state_ase_aseuid_c;
		return;

	case state_ase_uasuid_c:
		m_state = state_ase_c;
		return;

	case state_ase_uasuid_uniuid_c:
		m_state = state_ase_uasuid_c;
		return;

	case state_ase_uasuid_uniuid_txtname_c:
		m_state = state_ase_uasuid_uniuid_c;
		return;

	case state_ase_uasuid_codetype_c:
		m_state = state_ase_uasuid_c;
		return;

	case state_ase_uasuid_noseq_c:
		m_state = state_ase_uasuid_c;
		return;

	case state_ase_rsguid_c:
		m_state = state_ase_c;
		return;

	case state_ase_rsguid_rteuid_c:
		m_state = state_ase_rsguid_c;
		return;

	case state_ase_rsguid_rteuid_txtdesig_c:
		m_state = state_ase_rsguid_rteuid_c;
		return;

	case state_ase_rsguid_rteuid_txtlocdesig_c:
		m_state = state_ase_rsguid_rteuid_c;
		return;

	case state_ase_rsguid_dpnuidsta_c:
		m_state = state_ase_rsguid_c;
		return;

	case state_ase_rsguid_dpnuidsta_codeid_c:
		m_state = state_ase_rsguid_dpnuidsta_c;
		return;

	case state_ase_rsguid_dpnuidsta_geolat_c:
		m_state = state_ase_rsguid_dpnuidsta_c;
		return;

	case state_ase_rsguid_dpnuidsta_geolon_c:
		m_state = state_ase_rsguid_dpnuidsta_c;
		return;

	case state_ase_rsguid_dpnuidend_c:
		m_state = state_ase_rsguid_c;
		return;

	case state_ase_rsguid_dpnuidend_codeid_c:
		m_state = state_ase_rsguid_dpnuidend_c;
		return;

	case state_ase_rsguid_dpnuidend_geolat_c:
		m_state = state_ase_rsguid_dpnuidend_c;
		return;

	case state_ase_rsguid_dpnuidend_geolon_c:
		m_state = state_ase_rsguid_dpnuidend_c;
		return;

	case state_ase_txtlocaltype_c:
		m_state = state_ase_c;
		return;

	case state_ase_txtname_c:
		m_state = state_ase_c;
		return;

	case state_ase_txtrmk_c:
		m_state = state_ase_c;
		return;

	case state_ase_codeactivity_c:
		m_state = state_ase_c;
		return;

	case state_ase_codeclass_c:
		m_state = state_ase_c;
		return;

	case state_ase_codelocind_c:
		m_state = state_ase_c;
		return;

	case state_ase_codemil_c:
		m_state = state_ase_c;
		return;

	case state_ase_codedistverupper_c:
		m_state = state_ase_c;
		m_curairspace.set_uprflags(m_characters);
		return;

	case state_ase_valdistverupper_c:
		m_state = state_ase_c;
		m_curairspace.set_upralt(m_characters);
		return;

	case state_ase_uomdistverupper_c:
		m_state = state_ase_c;
		m_curairspace.set_uprscale(m_characters);
		return;

	case state_ase_codedistverlower_c:
		m_state = state_ase_c;
		m_curairspace.set_lwrflags(m_characters);
		return;

	case state_ase_valdistverlower_c:
		m_state = state_ase_c;
		m_curairspace.set_lwralt(m_characters);
		return;

	case state_ase_uomdistverlower_c:
		m_state = state_ase_c;
		m_curairspace.set_lwrscale(m_characters);
		return;

	case state_ase_codedistvermnm_c:
		m_state = state_ase_c;
		return;

	case state_ase_valdistvermnm_c:
		m_state = state_ase_c;
		return;

	case state_ase_uomdistvermnm_c:
		m_state = state_ase_c;
		return;

	case state_ase_codedistvermax_c:
		m_state = state_ase_c;
		return;

	case state_ase_valdistvermax_c:
		m_state = state_ase_c;
		return;

	case state_ase_uomdistvermax_c:
		m_state = state_ase_c;
		return;

	case state_ase_att_c:
		m_state = state_ase_c;
		return;

	case state_ase_att_codeworkhr_c:
		m_state = state_ase_att_c;
		return;

	case state_ase_att_txtrmkworkhr_c:
		m_state = state_ase_att_c;
		return;

	case state_ase_att_timsh_c:
		m_state = state_ase_att_c;
		if (m_curtimesheet.is_valid()) {
			if (m_curairspace.get_efftime() == "TIMSH")
				m_curairspace.set_efftime("");
			if (!m_curairspace.get_efftime().empty())
				m_curairspace.set_efftime(m_curairspace.get_efftime() + ", ");
			m_curairspace.set_efftime(m_curairspace.get_efftime() + m_curtimesheet.to_string());
		}
		m_curtimesheet.clear();
		m_characters.clear();
		return;

	case state_ase_att_timsh_codetimeref_c:
		m_state = state_ase_att_timsh_c;
		return;

	case state_ase_att_timsh_datevalidwef_c:
		m_state = state_ase_att_timsh_c;
		return;

	case state_ase_att_timsh_datevalidtil_c:
		m_state = state_ase_att_timsh_c;
		return;

	case state_ase_att_timsh_codeday_c:
		m_state = state_ase_att_timsh_c;
		return;

	case state_ase_att_timsh_codedaytil_c:
		m_state = state_ase_att_timsh_c;
		return;

	case state_ase_att_timsh_timewef_c:
		m_state = state_ase_att_timsh_c;
		return;

	case state_ase_att_timsh_timetil_c:
		m_state = state_ase_att_timsh_c;
		return;

	case state_ase_att_timsh_codeeventwef_c:
		m_state = state_ase_att_timsh_c;
		return;

	case state_ase_att_timsh_codeeventtil_c:
		m_state = state_ase_att_timsh_c;
		return;

	case state_ase_att_timsh_timereleventwef_c:
		m_state = state_ase_att_timsh_c;
		return;

	case state_ase_att_timsh_timereleventtil_c:
		m_state = state_ase_att_timsh_c;
		return;

	case state_ase_att_timsh_codecombwef_c:
		m_state = state_ase_att_timsh_c;
		return;

	case state_ase_att_timsh_codecombtil_c:
		m_state = state_ase_att_timsh_c;
		return;

	case state_adg_c:
		m_state = state_aixmsnapshot_c;
		m_characters.clear();
		return;

	case state_adg_adguid_c:
		m_state = state_adg_c;
		return;

	case state_adg_adguid_aseuid_c:
		m_state = state_adg_adguid_c;
		return;

	case state_adg_adguid_aseuid_codetype_c:
		m_state = state_adg_adguid_aseuid_c;
		if (!m_composites.size())
			throw std::runtime_error("No composite airspaces");
		m_composites.back().set_bdryclass(m_characters);
		m_characters.clear();
		return;

	case state_adg_adguid_aseuid_codeid_c:
		m_state = state_adg_adguid_aseuid_c;
		return;

	case state_adg_aseuidbase_c:
		m_state = state_adg_c;
		return;

	case state_adg_aseuidbase_codetype_c:
		m_state = state_adg_aseuidbase_c;
		if (!m_composites.size())
			throw std::runtime_error("No composite airspaces");
		if (!m_composites.back().size())
			throw std::runtime_error("Composite airspace has no components");
		m_composites.back().back().set_bdryclass(m_characters);
		m_characters.clear();
		return;

	case state_adg_aseuidbase_codeid_c:
		m_state = state_adg_aseuidbase_c;
		return;

	case state_adg_aseuidcomponent_c:
		m_state = state_adg_c;
		return;

	case state_adg_aseuidcomponent_codetype_c:
		m_state = state_adg_aseuidcomponent_c;
		if (!m_composites.size())
			throw std::runtime_error("No composite airspaces");
		if (!m_composites.back().size())
			throw std::runtime_error("Composite airspace has no components");
		m_composites.back().back().set_bdryclass(m_characters);
		m_characters.clear();
		return;

	case state_adg_aseuidcomponent_codeid_c:
		m_state = state_adg_aseuidcomponent_c;
		return;

	case state_adg_aseuidsameextent_c:
		m_state = state_adg_c;
		return;

	case state_adg_aseuidsameextent_codetype_c:
		m_state = state_adg_aseuidsameextent_c;
		if (!m_composites.size())
			throw std::runtime_error("No composite airspaces");
		if (!m_composites.back().size())
			throw std::runtime_error("Composite airspace has no components");
		m_composites.back().back().set_bdryclass(m_characters);
		m_characters.clear();
		return;

	case state_adg_aseuidsameextent_codeid_c:
		m_state = state_adg_aseuidsameextent_c;
		return;

	case state_adg_codeopr_c:
		m_state = state_adg_c;
		if (!m_composites.size())
			throw std::runtime_error("No composite airspaces");
		if (!m_composites.back().size())
			throw std::runtime_error("Composite airspace has no components");
		m_composites.back().set_operator_aixm(m_characters);
		m_characters.clear();
		return;

	case state_adg_txtrmk_c:
		m_state = state_adg_c;
		return;

	case state_abd_c:
		m_state = state_aixmsnapshot_c;
		process_airspace_vertices(false);
		m_curvertices.clear();
		m_characters.clear();
		return;

	case state_abd_abduid_c:
		m_state = state_abd_c;
		return;

	case state_abd_abduid_aseuid_c:
		m_state = state_abd_abduid_c;
		return;

	case state_abd_abduid_aseuid_codetype_c:
		m_state = state_abd_abduid_aseuid_c;
	        m_curvertices.set_bdryclass(m_characters);
		m_characters.clear();
		return;

	case state_abd_abduid_aseuid_codeid_c:
		m_state = state_abd_abduid_aseuid_c;
		return;

	case state_abd_txtrmk_c:
		m_state = state_abd_c;
		return;

	case state_abd_avx_c:
		m_state = state_abd_c;
		return;

	case state_abd_avx_gbruid_c:
		m_state = state_abd_avx_c;
		return;

	case state_abd_avx_gbruid_txtname_c:
		m_state = state_abd_avx_gbruid_c;
		if (!m_curvertices.size())
			throw std::runtime_error("Vertex Set does not contain a segment");
	        m_curvertices.back().set_bordername(m_characters);
		m_characters.clear();
		return;

	case state_abd_avx_dmeuidcen_c:
		m_state = state_abd_avx_c;
		return;

	case state_abd_avx_dmeuidcen_codeid_c:
		m_state = state_abd_avx_dmeuidcen_c;
		return;

	case state_abd_avx_dmeuidcen_geolat_c:
		m_state = state_abd_avx_dmeuidcen_c;
		return;

	case state_abd_avx_dmeuidcen_geolon_c:
		m_state = state_abd_avx_dmeuidcen_c;
		return;

	case state_abd_avx_tcnuidcen_c:
		m_state = state_abd_avx_c;
		return;

	case state_abd_avx_tcnuidcen_codeid_c:
		m_state = state_abd_avx_tcnuidcen_c;
		return;

	case state_abd_avx_tcnuidcen_geolat_c:
		m_state = state_abd_avx_tcnuidcen_c;
		return;

	case state_abd_avx_tcnuidcen_geolon_c:
		m_state = state_abd_avx_tcnuidcen_c;
		return;

	case state_abd_avx_ndbuidcen_c:
		m_state = state_abd_avx_c;
		return;

	case state_abd_avx_ndbuidcen_codeid_c:
		m_state = state_abd_avx_ndbuidcen_c;
		return;

	case state_abd_avx_ndbuidcen_geolat_c:
		m_state = state_abd_avx_ndbuidcen_c;
		return;

	case state_abd_avx_ndbuidcen_geolon_c:
		m_state = state_abd_avx_ndbuidcen_c;
		return;

	case state_abd_avx_voruidcen_c:
		m_state = state_abd_avx_c;
		return;

	case state_abd_avx_voruidcen_codeid_c:
		m_state = state_abd_avx_voruidcen_c;
		return;

	case state_abd_avx_voruidcen_geolat_c:
		m_state = state_abd_avx_voruidcen_c;
		return;

	case state_abd_avx_voruidcen_geolon_c:
		m_state = state_abd_avx_voruidcen_c;
		return;

	case state_abd_avx_codetype_c:
		m_state = state_abd_avx_c;
		if (!m_curvertices.size())
			throw std::runtime_error("Vertex Set does not contain a segment");
	        m_curvertices.back().set_shape(m_characters);
		m_characters.clear();
		return;

	case state_abd_avx_geolat_c:
		m_state = state_abd_avx_c;
		if (!m_curvertices.size())
			throw std::runtime_error("Vertex Set does not contain a segment");
	        m_curvertices.back().set_lat(m_characters);
		m_characters.clear();
		return;

	case state_abd_avx_geolon_c:
		m_state = state_abd_avx_c;
		if (!m_curvertices.size())
			throw std::runtime_error("Vertex Set does not contain a segment");
	        m_curvertices.back().set_lon(m_characters);
		m_characters.clear();
		return;

	case state_abd_avx_codedatum_c:
		m_state = state_abd_avx_c;
		return;

	case state_abd_avx_geolatarc_c:
		m_state = state_abd_avx_c;
		if (!m_curvertices.size())
			throw std::runtime_error("Vertex Set does not contain a segment");
	        m_curvertices.back().set_centerlat(m_characters);
		m_characters.clear();
		return;

	case state_abd_avx_geolonarc_c:
		m_state = state_abd_avx_c;
		if (!m_curvertices.size())
			throw std::runtime_error("Vertex Set does not contain a segment");
	        m_curvertices.back().set_centerlon(m_characters);
		m_characters.clear();
		return;

	case state_abd_avx_valradiusarc_c:
		m_state = state_abd_avx_c;
		if (!m_curvertices.size())
			throw std::runtime_error("Vertex Set does not contain a segment");
	        m_curvertices.back().set_radius(m_characters);
		m_characters.clear();
		return;

	case state_abd_avx_uomradiusarc_c:
		m_state = state_abd_avx_c;
		if (!m_curvertices.size())
			throw std::runtime_error("Vertex Set does not contain a segment");
	        m_curvertices.back().set_radiusscale(m_characters);
		m_characters.clear();
		return;

	case state_abd_avx_valgeoaccuracy_c:
		m_state = state_abd_avx_c;
		return;

	case state_abd_avx_uomgeoaccuracy_c:
		m_state = state_abd_avx_c;
		return;

	case state_abd_avx_valcrc_c:
		m_state = state_abd_avx_c;
		return;

	case state_abd_avx_txtrmk_c:
		m_state = state_abd_avx_c;
		return;

	case state_abd_avx_dpnuidspn_c:
		m_state = state_abd_avx_c;
		return;

	case state_abd_avx_dpnuidspn_codeid_c:
		m_state = state_abd_avx_dpnuidspn_c;
		return;

	case state_abd_avx_dpnuidspn_geolat_c:
		m_state = state_abd_avx_dpnuidspn_c;
		return;

	case state_abd_avx_dpnuidspn_geolon_c:
		m_state = state_abd_avx_dpnuidspn_c;
		return;

	case state_abd_avx_dmeuidspn_c:
		m_state = state_abd_avx_c;
		return;

	case state_abd_avx_dmeuidspn_codeid_c:
		m_state = state_abd_avx_dmeuidspn_c;
		return;

	case state_abd_avx_dmeuidspn_geolat_c:
		m_state = state_abd_avx_dmeuidspn_c;
		return;

	case state_abd_avx_dmeuidspn_geolon_c:
		m_state = state_abd_avx_dmeuidspn_c;
		return;

	case state_abd_avx_tcnuidspn_c:
		m_state = state_abd_avx_c;
		return;

	case state_abd_avx_tcnuidspn_codeid_c:
		m_state = state_abd_avx_tcnuidspn_c;
		return;

	case state_abd_avx_tcnuidspn_geolat_c:
		m_state = state_abd_avx_tcnuidspn_c;
		return;

	case state_abd_avx_tcnuidspn_geolon_c:
		m_state = state_abd_avx_tcnuidspn_c;
		return;

	case state_abd_avx_ndbuidspn_c:
		m_state = state_abd_avx_c;
		return;

	case state_abd_avx_ndbuidspn_codeid_c:
		m_state = state_abd_avx_ndbuidspn_c;
		return;

	case state_abd_avx_ndbuidspn_geolat_c:
		m_state = state_abd_avx_ndbuidspn_c;
		return;

	case state_abd_avx_ndbuidspn_geolon_c:
		m_state = state_abd_avx_ndbuidspn_c;
		return;

	case state_abd_avx_voruidspn_c:
		m_state = state_abd_avx_c;
		return;

	case state_abd_avx_voruidspn_codeid_c:
		m_state = state_abd_avx_voruidspn_c;
		return;

	case state_abd_avx_voruidspn_geolat_c:
		m_state = state_abd_avx_voruidspn_c;
		return;

	case state_abd_avx_voruidspn_geolon_c:
		m_state = state_abd_avx_voruidspn_c;
		return;

	case state_abd_circle_c:
		m_state = state_abd_c;
		return;

	case state_abd_circle_dmeuidcen_c:
		m_state = state_abd_circle_c;
		return;

	case state_abd_circle_dmeuidcen_codeid_c:
		m_state = state_abd_circle_dmeuidcen_c;
		return;

	case state_abd_circle_dmeuidcen_geolat_c:
		m_state = state_abd_circle_dmeuidcen_c;
		return;

	case state_abd_circle_dmeuidcen_geolon_c:
		m_state = state_abd_circle_dmeuidcen_c;
		return;

	case state_abd_circle_tcnuidcen_c:
		m_state = state_abd_circle_c;
		return;

	case state_abd_circle_tcnuidcen_codeid_c:
		m_state = state_abd_circle_tcnuidcen_c;
		return;

	case state_abd_circle_tcnuidcen_geolat_c:
		m_state = state_abd_circle_tcnuidcen_c;
		return;

	case state_abd_circle_tcnuidcen_geolon_c:
		m_state = state_abd_circle_tcnuidcen_c;
		return;

	case state_abd_circle_ndbuidcen_c:
		m_state = state_abd_circle_c;
		return;

	case state_abd_circle_ndbuidcen_codeid_c:
		m_state = state_abd_circle_ndbuidcen_c;
		return;

	case state_abd_circle_ndbuidcen_geolat_c:
		m_state = state_abd_circle_ndbuidcen_c;
		return;

	case state_abd_circle_ndbuidcen_geolon_c:
		m_state = state_abd_circle_ndbuidcen_c;
		return;

	case state_abd_circle_voruidcen_c:
		m_state = state_abd_circle_c;
		return;

	case state_abd_circle_voruidcen_codeid_c:
		m_state = state_abd_circle_voruidcen_c;
		return;

	case state_abd_circle_voruidcen_geolat_c:
		m_state = state_abd_circle_voruidcen_c;
		return;

	case state_abd_circle_voruidcen_geolon_c:
		m_state = state_abd_circle_voruidcen_c;
		return;

	case state_abd_circle_geolatcen_c:
		m_state = state_abd_circle_c;
		if (!m_curvertices.size())
			throw std::runtime_error("Vertex Set does not contain a segment");
	        m_curvertices.back().set_centerlat(m_characters);
		m_characters.clear();
		return;

	case state_abd_circle_geoloncen_c:
		m_state = state_abd_circle_c;
		if (!m_curvertices.size())
			throw std::runtime_error("Vertex Set does not contain a segment");
	        m_curvertices.back().set_centerlon(m_characters);
		m_characters.clear();
		return;

	case state_abd_circle_codedatum_c:
		m_state = state_abd_circle_c;
		return;

	case state_abd_circle_valradius_c:
		m_state = state_abd_circle_c;
		if (!m_curvertices.size())
			throw std::runtime_error("Vertex Set does not contain a segment");
	        m_curvertices.back().set_radius(m_characters);
		m_characters.clear();
		return;

	case state_abd_circle_uomradius_c:
		m_state = state_abd_circle_c;
		if (!m_curvertices.size())
			throw std::runtime_error("Vertex Set does not contain a segment");
	        m_curvertices.back().set_radiusscale(m_characters);
		m_characters.clear();
		return;

	case state_abd_circle_valgeoaccuracy_c:
		m_state = state_abd_circle_c;
		return;

	case state_abd_circle_uomgeoaccuracy_c:
		m_state = state_abd_circle_c;
		return;

	case state_abd_circle_valcrc_c:
		m_state = state_abd_circle_c;
		return;

	case state_abd_circle_txtrmk_c:
		m_state = state_abd_circle_c;
		return;

	case state_acr_c:
		m_state = state_aixmsnapshot_c;
		process_airspace_vertices(true);
		m_curvertices.clear();
		m_characters.clear();
		return;

	case state_acr_acruid_c:
		m_state = state_acr_c;
		return;

	case state_acr_acruid_aseuid_c:
		m_state = state_acr_acruid_c;
		return;

	case state_acr_acruid_aseuid_codetype_c:
		m_state = state_acr_acruid_aseuid_c;
	        m_curvertices.set_bdryclass(m_characters);
		m_characters.clear();
		return;

	case state_acr_acruid_aseuid_codeid_c:
		m_state = state_acr_acruid_aseuid_c;
		return;

	case state_acr_valwidth_c:
		m_state = state_acr_c;
	        m_curvertices.set_width(m_characters);
		m_characters.clear();
		return;

	case state_acr_uomwidth_c:
		m_state = state_acr_c;
	        m_curvertices.set_widthscale(m_characters);
		m_characters.clear();
		return;

	case state_acr_txtrmk_c:
		m_state = state_acr_c;
		return;

	case state_acr_avx_c:
		m_state = state_acr_c;
		return;

	case state_acr_avx_gbruid_c:
		m_state = state_acr_avx_c;
		return;

	case state_acr_avx_gbruid_txtname_c:
		m_state = state_acr_avx_gbruid_c;
		if (!m_curvertices.size())
			throw std::runtime_error("Vertex Set does not contain a segment");
	        m_curvertices.back().set_bordername(m_characters);
		m_characters.clear();
		return;

	case state_acr_avx_codetype_c:
		m_state = state_acr_avx_c;
		if (!m_curvertices.size())
			throw std::runtime_error("Vertex Set does not contain a segment");
	        m_curvertices.back().set_shape(m_characters);
		m_characters.clear();
		return;

	case state_acr_avx_geolat_c:
		m_state = state_acr_avx_c;
		if (!m_curvertices.size())
			throw std::runtime_error("Vertex Set does not contain a segment");
	        m_curvertices.back().set_lat(m_characters);
		m_characters.clear();
		return;

	case state_acr_avx_geolon_c:
		m_state = state_acr_avx_c;
		if (!m_curvertices.size())
			throw std::runtime_error("Vertex Set does not contain a segment");
	        m_curvertices.back().set_lon(m_characters);
		m_characters.clear();
		return;

	case state_acr_avx_codedatum_c:
		m_state = state_acr_avx_c;
		return;

	case state_acr_avx_geolatarc_c:
		m_state = state_acr_avx_c;
		if (!m_curvertices.size())
			throw std::runtime_error("Vertex Set does not contain a segment");
	        m_curvertices.back().set_centerlat(m_characters);
		m_characters.clear();
		return;

	case state_acr_avx_geolonarc_c:
		m_state = state_acr_avx_c;
		if (!m_curvertices.size())
			throw std::runtime_error("Vertex Set does not contain a segment");
	        m_curvertices.back().set_centerlon(m_characters);
		m_characters.clear();
		return;

	case state_acr_avx_valradiusarc_c:
		m_state = state_acr_avx_c;
		if (!m_curvertices.size())
			throw std::runtime_error("Vertex Set does not contain a segment");
	        m_curvertices.back().set_radius(m_characters);
		m_characters.clear();
		return;

	case state_acr_avx_uomradiusarc_c:
		m_state = state_acr_avx_c;
		if (!m_curvertices.size())
			throw std::runtime_error("Vertex Set does not contain a segment");
	        m_curvertices.back().set_radiusscale(m_characters);
		m_characters.clear();
		return;

	case state_acr_avx_valgeoaccuracy_c:
		m_state = state_acr_avx_c;
		return;

	case state_acr_avx_uomgeoaccuracy_c:
		m_state = state_acr_avx_c;
		return;

	case state_acr_avx_valcrc_c:
		m_state = state_acr_avx_c;
		return;

	case state_acr_avx_txtrmk_c:
		m_state = state_acr_avx_c;
		return;

	case state_acr_avx_dpnuidspn_c:
		m_state = state_acr_avx_c;
		return;

	case state_acr_avx_dpnuidspn_codeid_c:
		m_state = state_acr_avx_dpnuidspn_c;
		return;

	case state_acr_avx_dpnuidspn_geolat_c:
		m_state = state_acr_avx_dpnuidspn_c;
		return;

	case state_acr_avx_dpnuidspn_geolon_c:
		m_state = state_acr_avx_dpnuidspn_c;
		return;

	case state_acr_avx_dmeuidspn_c:
		m_state = state_acr_avx_c;
		return;

	case state_acr_avx_dmeuidspn_codeid_c:
		m_state = state_acr_avx_dmeuidspn_c;
		return;

	case state_acr_avx_dmeuidspn_geolat_c:
		m_state = state_acr_avx_dmeuidspn_c;
		return;

	case state_acr_avx_dmeuidspn_geolon_c:
		m_state = state_acr_avx_dmeuidspn_c;
		return;

	case state_acr_avx_tcnuidspn_c:
		m_state = state_acr_avx_c;
		return;

	case state_acr_avx_tcnuidspn_codeid_c:
		m_state = state_acr_avx_tcnuidspn_c;
		return;

	case state_acr_avx_tcnuidspn_geolat_c:
		m_state = state_acr_avx_tcnuidspn_c;
		return;

	case state_acr_avx_tcnuidspn_geolon_c:
		m_state = state_acr_avx_tcnuidspn_c;
		return;

	case state_acr_avx_ndbuidspn_c:
		m_state = state_acr_avx_c;
		return;

	case state_acr_avx_ndbuidspn_codeid_c:
		m_state = state_acr_avx_ndbuidspn_c;
		return;

	case state_acr_avx_ndbuidspn_geolat_c:
		m_state = state_acr_avx_ndbuidspn_c;
		return;

	case state_acr_avx_ndbuidspn_geolon_c:
		m_state = state_acr_avx_ndbuidspn_c;
		return;

	case state_acr_avx_voruidspn_c:
		m_state = state_acr_avx_c;
		return;

	case state_acr_avx_voruidspn_codeid_c:
		m_state = state_acr_avx_voruidspn_c;
		return;

	case state_acr_avx_voruidspn_geolat_c:
		m_state = state_acr_avx_voruidspn_c;
		return;

	case state_acr_avx_voruidspn_geolon_c:
		m_state = state_acr_avx_voruidspn_c;
		return;

	case state_aas_c:
		m_state = state_aixmsnapshot_c;
		m_characters.clear();
		return;

	case state_aas_aasuid_c:
		m_state = state_aas_c;
		return;

	case state_aas_aasuid_aseuid1_c:
		m_state = state_aas_aasuid_c;
		return;

	case state_aas_aasuid_aseuid1_codetype_c:
		m_state = state_aas_aasuid_aseuid1_c;
		return;

	case state_aas_aasuid_aseuid1_codeid_c:
		m_state = state_aas_aasuid_aseuid1_c;
		return;

	case state_aas_aasuid_aseuid2_c:
		m_state = state_aas_aasuid_c;
		return;

	case state_aas_aasuid_aseuid2_codetype_c:
		m_state = state_aas_aasuid_aseuid2_c;
		return;

	case state_aas_aasuid_aseuid2_codeid_c:
		m_state = state_aas_aasuid_aseuid2_c;
		return;

	case state_aas_aasuid_codetype_c:
		m_state = state_aas_aasuid_c;
		return;

	case state_aas_txtrmk_c:
		m_state = state_aas_c;
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
	case state_document_c:
	case state_aixmsnapshot_c:
	case state_gbr_c:
	case state_gbr_uid_c:
	case state_gbr_gbv_c:
	case state_ase_c:
	case state_ase_aseuid_c:
	case state_ase_uasuid_c:
	case state_ase_uasuid_uniuid_c:
	case state_ase_rsguid_c:
	case state_ase_rsguid_rteuid_c:
	case state_ase_rsguid_dpnuidsta_c:
	case state_ase_rsguid_dpnuidend_c:
	case state_ase_att_c:
	case state_ase_att_timsh_c:
	case state_adg_c:
	case state_adg_adguid_c:
	case state_adg_adguid_aseuid_c:
	case state_adg_aseuidbase_c:
	case state_adg_aseuidcomponent_c:
	case state_adg_aseuidsameextent_c:
	case state_abd_c:
	case state_abd_abduid_c:
	case state_abd_abduid_aseuid_c:
	case state_abd_avx_c:
	case state_abd_avx_gbruid_c:
	case state_abd_avx_dpnuidspn_c:
	case state_abd_avx_dmeuidspn_c:
	case state_abd_avx_tcnuidspn_c:
	case state_abd_avx_ndbuidspn_c:
	case state_abd_avx_voruidspn_c:
	case state_abd_avx_dmeuidcen_c:
	case state_abd_avx_tcnuidcen_c:
	case state_abd_avx_ndbuidcen_c:
	case state_abd_avx_voruidcen_c:
	case state_abd_circle_c:
	case state_abd_circle_dmeuidcen_c:
	case state_abd_circle_tcnuidcen_c:
	case state_abd_circle_ndbuidcen_c:
	case state_abd_circle_voruidcen_c:
	case state_acr_c:
	case state_acr_acruid_c:
	case state_acr_acruid_aseuid_c:
	case state_acr_avx_c:
	case state_acr_avx_gbruid_c:
	case state_acr_avx_dpnuidspn_c:
	case state_acr_avx_dmeuidspn_c:
	case state_acr_avx_tcnuidspn_c:
	case state_acr_avx_ndbuidspn_c:
	case state_acr_avx_voruidspn_c:
	case state_aas_c:
	case state_aas_aasuid_c:
	case state_aas_aasuid_aseuid1_c:
	case state_aas_aasuid_aseuid2_c:
		break;

	case state_gbr_uid_txtname_c:
		m_curborder.set_name(m_curborder.get_name() + characters);
		break;

	case state_gbr_codetype_c:
		m_curborder.set_codetype(m_curborder.get_codetype() + characters);
		break;

	case state_gbr_txtrmk_c:
		m_curborder.set_remark(m_curborder.get_remark() + characters);
		break;

	case state_ase_aseuid_codeid_c:
		m_curairspace.set_icao(m_curairspace.get_icao() + characters);
		break;

	case state_ase_txtname_c:
		m_curairspace.set_name(m_curairspace.get_name() + characters);
		break;

	case state_ase_txtrmk_c:
		m_curairspace.set_note(m_curairspace.get_note() + characters);
		break;

	case state_ase_codelocind_c:
		m_curairspace.set_ident(m_curairspace.get_ident() + characters);
		break;

	case state_ase_att_codeworkhr_c:
		m_curairspace.set_efftime(m_curairspace.get_efftime() + characters);
		break;

	case state_ase_att_txtrmkworkhr_c:
		m_curairspace.set_efftime(m_curairspace.get_efftime() + characters);
		break;

	case state_ase_att_timsh_codetimeref_c:
		m_curtimesheet.set_timeref(m_curtimesheet.get_timeref() + characters);
		break;

	case state_ase_att_timsh_datevalidwef_c:
		m_curtimesheet.set_dateeff(m_curtimesheet.get_dateeff() + characters);
		break;

	case state_ase_att_timsh_datevalidtil_c:
		m_curtimesheet.set_dateend(m_curtimesheet.get_dateend() + characters);
		break;

	case state_ase_att_timsh_codeday_c:
		m_curtimesheet.set_day(m_curtimesheet.get_day() + characters);
		break;

	case state_ase_att_timsh_codedaytil_c:
		m_curtimesheet.set_dayend(m_curtimesheet.get_dayend() + characters);
		break;

	case state_ase_att_timsh_timewef_c:
		m_curtimesheet.set_timeeff(m_curtimesheet.get_timeeff() + characters);
		break;

	case state_ase_att_timsh_timetil_c:
		m_curtimesheet.set_timeend(m_curtimesheet.get_timeend() + characters);
		break;

	case state_ase_att_timsh_codeeventwef_c:
		m_curtimesheet.set_eventcodeeff(m_curtimesheet.get_eventcodeeff() + characters);
		break;

	case state_ase_att_timsh_codeeventtil_c:
		m_curtimesheet.set_eventcodeend(m_curtimesheet.get_eventcodeend() + characters);
		break;

	case state_ase_att_timsh_timereleventwef_c:
		m_curtimesheet.set_eventtimeeff(m_curtimesheet.get_eventtimeeff() + characters);
		break;

	case state_ase_att_timsh_timereleventtil_c:
		m_curtimesheet.set_eventtimeend(m_curtimesheet.get_eventtimeend() + characters);
		break;

	case state_ase_att_timsh_codecombwef_c:
		m_curtimesheet.set_eventopeff(m_curtimesheet.get_eventopeff() + characters);
		break;

	case state_ase_att_timsh_codecombtil_c:
		m_curtimesheet.set_eventopend(m_curtimesheet.get_eventopend() + characters);
		break;

	case state_abd_abduid_aseuid_codeid_c:
	case state_acr_acruid_aseuid_codeid_c:
	        m_curvertices.set_icao(m_curairspace.get_icao() + characters);
		break;

	case state_adg_adguid_aseuid_codeid_c:
		if (!m_composites.size())
			throw std::runtime_error("No composite airspaces");
		m_composites.back().set_icao(m_composites.back().get_icao() + characters);
		break;

	case state_adg_aseuidbase_codeid_c:
	case state_adg_aseuidcomponent_codeid_c:
	case state_adg_aseuidsameextent_codeid_c:
		if (!m_composites.size())
			throw std::runtime_error("No composite airspaces");
		if (!m_composites.back().size())
			throw std::runtime_error("Composite airspace has no components");
		m_composites.back().back().set_icao(m_composites.back().back().get_icao() + characters);
		break;

	case state_gbr_gbv_codetype_c:
	case state_gbr_gbv_geolat_c:
	case state_gbr_gbv_geolon_c:
	case state_gbr_gbv_codedatum_c:
	case state_gbr_gbv_valcrc_c:
	case state_gbr_gbv_txtrmk_c:
	case state_ase_aseuid_codetype_c:
	case state_ase_rsguid_rteuid_txtdesig_c:
	case state_ase_rsguid_rteuid_txtlocdesig_c:
	case state_ase_rsguid_dpnuidsta_codeid_c:
	case state_ase_rsguid_dpnuidsta_geolat_c:
	case state_ase_rsguid_dpnuidsta_geolon_c:
	case state_ase_rsguid_dpnuidend_codeid_c:
	case state_ase_rsguid_dpnuidend_geolat_c:
	case state_ase_rsguid_dpnuidend_geolon_c:
	case state_ase_txtlocaltype_c:
	case state_ase_codeactivity_c:
	case state_ase_codeclass_c:
	case state_ase_codemil_c:
	case state_ase_codedistverupper_c:
	case state_ase_valdistverupper_c:
	case state_ase_uomdistverupper_c:
	case state_ase_codedistverlower_c:
	case state_ase_valdistverlower_c:
	case state_ase_uomdistverlower_c:
	case state_ase_codedistvermnm_c:
	case state_ase_valdistvermnm_c:
	case state_ase_uomdistvermnm_c:
	case state_ase_codedistvermax_c:
	case state_ase_valdistvermax_c:
	case state_ase_uomdistvermax_c:
	case state_ase_uasuid_uniuid_txtname_c:
	case state_ase_uasuid_codetype_c:
	case state_ase_uasuid_noseq_c:
	case state_adg_adguid_aseuid_codetype_c:
	case state_adg_aseuidbase_codetype_c:
	case state_adg_aseuidcomponent_codetype_c:
	case state_adg_aseuidsameextent_codetype_c:
	case state_adg_codeopr_c:
	case state_adg_txtrmk_c:
	case state_abd_abduid_aseuid_codetype_c:
	case state_abd_txtrmk_c:
	case state_abd_avx_gbruid_txtname_c:
	case state_abd_avx_dmeuidcen_codeid_c:
	case state_abd_avx_dmeuidcen_geolat_c:
	case state_abd_avx_dmeuidcen_geolon_c:
	case state_abd_avx_tcnuidcen_codeid_c:
	case state_abd_avx_tcnuidcen_geolat_c:
	case state_abd_avx_tcnuidcen_geolon_c:
	case state_abd_avx_ndbuidcen_codeid_c:
	case state_abd_avx_ndbuidcen_geolat_c:
	case state_abd_avx_ndbuidcen_geolon_c:
	case state_abd_avx_voruidcen_codeid_c:
	case state_abd_avx_voruidcen_geolat_c:
	case state_abd_avx_voruidcen_geolon_c:
	case state_abd_avx_codetype_c:
	case state_abd_avx_geolat_c:
	case state_abd_avx_geolon_c:
	case state_abd_avx_codedatum_c:
	case state_abd_avx_geolatarc_c:
	case state_abd_avx_geolonarc_c:
	case state_abd_avx_valradiusarc_c:
	case state_abd_avx_uomradiusarc_c:
	case state_abd_avx_valgeoaccuracy_c:
	case state_abd_avx_uomgeoaccuracy_c:
	case state_abd_avx_valcrc_c:
	case state_abd_avx_txtrmk_c:
	case state_abd_avx_dpnuidspn_codeid_c:
	case state_abd_avx_dpnuidspn_geolat_c:
	case state_abd_avx_dpnuidspn_geolon_c:
	case state_abd_avx_dmeuidspn_codeid_c:
	case state_abd_avx_dmeuidspn_geolat_c:
	case state_abd_avx_dmeuidspn_geolon_c:
	case state_abd_avx_tcnuidspn_codeid_c:
	case state_abd_avx_tcnuidspn_geolat_c:
	case state_abd_avx_tcnuidspn_geolon_c:
	case state_abd_avx_ndbuidspn_codeid_c:
	case state_abd_avx_ndbuidspn_geolat_c:
	case state_abd_avx_ndbuidspn_geolon_c:
	case state_abd_avx_voruidspn_codeid_c:
	case state_abd_avx_voruidspn_geolat_c:
	case state_abd_avx_voruidspn_geolon_c:
	case state_abd_circle_dmeuidcen_codeid_c:
	case state_abd_circle_dmeuidcen_geolat_c:
	case state_abd_circle_dmeuidcen_geolon_c:
	case state_abd_circle_tcnuidcen_codeid_c:
	case state_abd_circle_tcnuidcen_geolat_c:
	case state_abd_circle_tcnuidcen_geolon_c:
	case state_abd_circle_ndbuidcen_codeid_c:
	case state_abd_circle_ndbuidcen_geolat_c:
	case state_abd_circle_ndbuidcen_geolon_c:
	case state_abd_circle_voruidcen_codeid_c:
	case state_abd_circle_voruidcen_geolat_c:
	case state_abd_circle_voruidcen_geolon_c:
	case state_abd_circle_geolatcen_c:
	case state_abd_circle_geoloncen_c:
	case state_abd_circle_codedatum_c:
	case state_abd_circle_valradius_c:
	case state_abd_circle_uomradius_c:
	case state_abd_circle_valgeoaccuracy_c:
	case state_abd_circle_uomgeoaccuracy_c:
	case state_abd_circle_valcrc_c:
	case state_abd_circle_txtrmk_c:
	case state_acr_acruid_aseuid_codetype_c:
	case state_acr_valwidth_c:
	case state_acr_uomwidth_c:
	case state_acr_txtrmk_c:
	case state_acr_avx_gbruid_txtname_c:
	case state_acr_avx_codetype_c:
	case state_acr_avx_geolat_c:
	case state_acr_avx_geolon_c:
	case state_acr_avx_codedatum_c:
	case state_acr_avx_geolatarc_c:
	case state_acr_avx_geolonarc_c:
	case state_acr_avx_valradiusarc_c:
	case state_acr_avx_uomradiusarc_c:
	case state_acr_avx_valgeoaccuracy_c:
	case state_acr_avx_uomgeoaccuracy_c:
	case state_acr_avx_valcrc_c:
	case state_acr_avx_txtrmk_c:
	case state_acr_avx_dpnuidspn_codeid_c:
	case state_acr_avx_dpnuidspn_geolat_c:
	case state_acr_avx_dpnuidspn_geolon_c:
	case state_acr_avx_dmeuidspn_codeid_c:
	case state_acr_avx_dmeuidspn_geolat_c:
	case state_acr_avx_dmeuidspn_geolon_c:
	case state_acr_avx_tcnuidspn_codeid_c:
	case state_acr_avx_tcnuidspn_geolat_c:
	case state_acr_avx_tcnuidspn_geolon_c:
	case state_acr_avx_ndbuidspn_codeid_c:
	case state_acr_avx_ndbuidspn_geolat_c:
	case state_acr_avx_ndbuidspn_geolon_c:
	case state_acr_avx_voruidspn_codeid_c:
	case state_acr_avx_voruidspn_geolat_c:
	case state_acr_avx_voruidspn_geolon_c:
	case state_aas_aasuid_aseuid1_codetype_c:
	case state_aas_aasuid_aseuid1_codeid_c:
	case state_aas_aasuid_aseuid2_codetype_c:
	case state_aas_aasuid_aseuid2_codeid_c:
	case state_aas_aasuid_codetype_c:
	case state_aas_txtrmk_c:
		m_characters += characters;
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

void DbXmlImporter::process_airspace(void)
{
	if (!m_curairspace.is_valid()) {
		std::cerr << "process_airspace: Invalid Airspace " << m_curairspace.get_codetype() << ' '
			  << m_curairspace.get_icao() << ' ' << m_curairspace.get_name() << ' ' << m_curairspace.get_mid() << std::endl;
		return;
	}
	open_airspaces_db();
	AirspacesDb::element_t el;
	el.set_sourceid(m_curairspace.get_codetype() + "_" + m_curairspace.get_icao() + "@EAD");
	el.set_label_placement(AirspacesDb::element_t::label_e);
	// check for existing airspace
	bool updated(false);
	{
		AirspacesDb::elementvector_t ev(m_airspacesdb.find_by_sourceid(el.get_sourceid()));
		for (AirspacesDb::elementvector_t::const_iterator evi(ev.begin()), eve(ev.end()); evi != eve; ++evi) {
			if (evi->get_sourceid() != el.get_sourceid())
				continue;
			el = *evi;
			updated = true;
			break;
		}
	}
	el.set_icao(m_curairspace.get_icao());
	if (m_curairspace.is_name_valid())
		el.set_name(m_curairspace.get_name());
	el.set_typecode(AirspacesDb::Airspace::typecode_ead);
	el.set_bdryclass(m_curairspace.get_bdryclass());
	if (m_curairspace.is_efftime_valid())
		el.set_efftime(m_curairspace.get_efftime());
	if (m_curairspace.is_note_valid())
		el.set_note(m_curairspace.get_note());
	if (m_curairspace.is_lwr_valid()) {
		el.set_altlwr(m_curairspace.get_scaled_lwralt());
		el.set_altlwrflags(m_curairspace.get_lwrflags());
	}
	if (m_curairspace.is_upr_valid()) {
		el.set_altupr(m_curairspace.get_scaled_upralt());
		el.set_altuprflags(m_curairspace.get_uprflags());
	}
	if (m_curairspace.is_ident_valid())
		el.set_ident(m_curairspace.get_ident());
	if (true)
		std::cerr << (updated ? "Modified" : "New") << " Airspace " << el.get_type_string() << '/' << el.get_class_string() << ' '
			  << el.get_icao() << ' ' << el.get_name() << ' ' << el.get_sourceid() << ' ' << m_curairspace.get_mid() << std::endl;
	try {
		m_airspacesdb.save(el);
	} catch (const std::exception& e) {
		std::cerr << "I/O Error: " << e.what() << std::endl;
		throw;
	}
}

void DbXmlImporter::process_airspace_vertices(bool expand)
{
	if (!m_curvertices.is_valid()) {
		std::cerr << "Warning: Invalid Vertices " << m_curvertices.get_codetype() << ' '
			  << m_curvertices.get_icao() << ' ' << m_curvertices.get_mid() << std::endl;
		return;
	}
	open_airspaces_db();
	AirspacesDb::element_t el;
	el.set_sourceid(m_curvertices.get_codetype() + "_" + m_curvertices.get_icao() + "@EAD");
	// check for existing airspace
	{
		bool found(false);
		AirspacesDb::elementvector_t ev(m_airspacesdb.find_by_sourceid(el.get_sourceid()));
		for (AirspacesDb::elementvector_t::const_iterator evi(ev.begin()), eve(ev.end()); evi != eve; ++evi) {
			if (evi->get_sourceid() != el.get_sourceid())
				continue;
			el = *evi;
			found = true;
			break;
		}
		if (!found) {
			std::cerr << "Warning: Cannot find airspace " << m_curvertices.get_codetype() << ' '
				  << m_curvertices.get_icao() << ' ' << m_curvertices.get_mid() << std::endl;
			return;
		}
	}
	m_curvertices.recompute_coord2();
	m_curvertices.resolve_borders(m_borders);
	if (expand)
		el.set_width_nmi(m_curvertices.get_scaled_width());
	else
		el.set_width(0);
	el.clear_segments();
	for (unsigned int i = 0; i < m_curvertices.size(); ++i) {
		const ParseVertices::Segment& seg(m_curvertices[i]);
		if (seg.get_coord1() != seg.get_coord2() ||
		    (seg.get_shape() != '-' && seg.get_shape() != 'B' && seg.get_shape() != 'H'))
			el.add_segment(m_curvertices[i]);
	}
	el.recompute_lineapprox(m_airspacesdb);
	el.recompute_bbox();
	if (!el.get_bbox().is_inside(el.get_labelcoord()) || !el.get_polygon().windingnumber(el.get_labelcoord()))
		el.compute_initial_label_placement();
	el.set_modtime(time(0));
	if (true)
		std::cerr << "Updated " << (expand ? "Expanded " : "") << "Airspace " << el.get_type_string() << '/' << el.get_class_string() << ' '
			  << el.get_icao() << ' ' << el.get_name() << ' ' << el.get_sourceid() << ' ' << m_curvertices.get_mid() << std::endl;
	try {
		m_airspacesdb.save(el);
	} catch (const std::exception& e) {
		std::cerr << "I/O Error: " << e.what() << std::endl;
		throw;
	}
}

void DbXmlImporter::process_composite_airspaces(void)
{
	if (m_composites.empty())
		return;
	// need to topologically sort the vector first
	typedef boost::adjacency_list<boost::vecS,boost::vecS,boost::directedS,boost::property<boost::vertex_color_t,boost::default_color_type> > Graph;
	typedef boost::graph_traits<Graph>::vertex_descriptor Vertex;
	Graph G(m_composites.size());
	{
		typedef std::map<unsigned long,unsigned int> midmap_t;
		midmap_t midmap;
		for (unsigned int i = 0; i < m_composites.size(); ++i) {
			const ParseComposite& comp(m_composites[i]);
			std::pair<midmap_t::iterator,bool> ins(midmap.insert(std::make_pair(comp.get_asemid(), i)));
			if (ins.second)
				continue;
			std::cerr << "Warning: Duplicate airspace mid " << comp.get_codetype() << ' '
				  << comp.get_icao() << ' ' << comp.get_mid()
				  << ": " << comp.get_asemid() << std::endl;
		}
		for (unsigned int i = 0; i < m_composites.size(); ++i) {
			const ParseComposite& comp(m_composites[i]);
			for (unsigned int j = 0; j < comp.size(); ++j) {
				const ParseComposite::Component& cmp(comp[j]);
				midmap_t::const_iterator mi(midmap.find(cmp.get_mid()));
				if (mi == midmap.end())
					continue;
				boost::add_edge(i, mi->second, G);
			}
		}
	}
	typedef std::vector<Vertex> container_t;
	container_t c;
	topological_sort(G, std::back_inserter(c));
	typedef std::list<AirspacesDb::Airspace> aspclist_t;
	aspclist_t aspclist;
	for (container_t::const_iterator ci(c.begin()), ce(c.end()); ci != ce; ++ci) {
		AirspacesDb::Airspace el(create_composite_airspace(m_composites[*ci]));
		if (el.is_valid())
			aspclist.push_back(el);
	}
	m_composites.clear();
	while (!aspclist.empty()) {
		recalc_composite_airspace(*aspclist.begin());
		aspclist.erase(aspclist.begin());
	}
}

AirspacesDb::Airspace DbXmlImporter::create_composite_airspace(const ParseComposite& comp)
{
	open_airspaces_db();
	AirspacesDb::element_t el;
	el.set_sourceid(comp.get_codetype() + "_" + comp.get_icao() + "@EAD");
	// check for existing airspace
	{
		bool found(false);
		AirspacesDb::elementvector_t ev(m_airspacesdb.find_by_sourceid(el.get_sourceid()));
		for (AirspacesDb::elementvector_t::const_iterator evi(ev.begin()), eve(ev.end()); evi != eve; ++evi) {
			if (evi->get_sourceid() != el.get_sourceid())
				continue;
			el = *evi;
			found = true;
			break;
		}
		if (!found) {
			std::cerr << "Warning: Cannot find airspace " << comp.get_codetype() << ' '
				  << comp.get_icao() << ' ' << comp.get_mid() << std::endl;
			return AirspacesDb::element_t();
		}
	}
	for (unsigned int i = 0; i < comp.size(); ++i)
		el.add_component(comp[i]);
	if (true)
		std::cerr << "Created Composite Airspace " << el.get_type_string() << '/' << el.get_class_string() << ' '
			  << el.get_icao() << ' ' << el.get_name() << ' ' << el.get_sourceid() << std::endl;
	try {
		m_airspacesdb.save(el);
	} catch (const std::exception& e) {
		std::cerr << "I/O Error: " << e.what() << std::endl;
		throw;
	}
	return el;
}

void DbXmlImporter::recalc_composite_airspace(AirspacesDb::Airspace& el)
{
	open_airspaces_db();
	try {
		el.recompute_lineapprox(m_airspacesdb);
		el.recompute_bbox();
		if (!el.get_bbox().is_inside(el.get_labelcoord()) || !el.get_polygon().windingnumber(el.get_labelcoord()))
			el.compute_initial_label_placement();
		el.set_modtime(time(0));
	} catch (const std::exception& e) {
		std::cerr << "Calculated Composite Airspace " << el.get_type_string() << '/' << el.get_class_string() << ' '
			  << el.get_icao() << ' ' << el.get_name() << ' ' << el.get_sourceid() << " failed: " << e.what() << std::endl;
		return;
	} catch (...) {
		std::cerr << "Calculated Composite Airspace " << el.get_type_string() << '/' << el.get_class_string() << ' '
			  << el.get_icao() << ' ' << el.get_name() << ' ' << el.get_sourceid() << " failed" << std::endl;
		return;
	}
	if (true)
		std::cerr << "Calculated Composite Airspace " << el.get_type_string() << '/' << el.get_class_string() << ' '
			  << el.get_icao() << ' ' << el.get_name() << ' ' << el.get_sourceid() << std::endl;
	try {
		m_airspacesdb.save(el);
	} catch (const std::exception& e) {
		std::cerr << "I/O Error: " << e.what() << std::endl;
		throw;
	}
}

/*

"EDGGN", AirspacesDb::Airspace::bdryclass_ead_cta, "not loaded"
"EDGGS", AirspacesDb::Airspace::bdryclass_ead_cta, "not loaded"
"EDYYB", AirspacesDb::Airspace::bdryclass_ead_uta, "not loaded"
"EDYYD", AirspacesDb::Airspace::bdryclass_ead_uta, "not loaded"
"EDYYH", AirspacesDb::Airspace::bdryclass_ead_uta, "not loaded"
"EEPU", AirspacesDb::Airspace::bdryclass_ead_ctr, "not loaded"
"EFMI", AirspacesDb::Airspace::bdryclass_ead_ctr, "not loaded"
"EFSA", AirspacesDb::Airspace::bdryclass_ead_ctr, "not loaded"
"EFVR", AirspacesDb::Airspace::bdryclass_ead_ctr, "not loaded"
"EFSI", AirspacesDb::Airspace::bdryclass_ead_ctr, "not loaded"
"EFET", AirspacesDb::Airspace::bdryclass_ead_ctr, "not loaded"
"EFKI", AirspacesDb::Airspace::bdryclass_ead_ctr, "not loaded"
"EGNH", AirspacesDb::Airspace::bdryclass_ead_atz, "not loaded"
"EGPE", AirspacesDb::Airspace::bdryclass_ead_ctr, "not loaded"
"EGTTT", AirspacesDb::Airspace::bdryclass_ead_cta, "not loaded"
"EGHH", AirspacesDb::Airspace::bdryclass_ead_tma, "not loaded"
"EGHI", AirspacesDb::Airspace::bdryclass_ead_tma, "not loaded"
"EGMH", AirspacesDb::Airspace::bdryclass_ead_tma, "not loaded"
"EHAAI", AirspacesDb::Airspace::bdryclass_ead_cta, "not loaded"
"EKVG", AirspacesDb::Airspace::bdryclass_ead_ctr, "not loaded"
"ENFL", AirspacesDb::Airspace::bdryclass_ead_ctr, "not loaded"
"ENLI", AirspacesDb::Airspace::bdryclass_ead_ctr, "not loaded"
"ENSG", AirspacesDb::Airspace::bdryclass_ead_ctr, "not loaded"
"ENSB", AirspacesDb::Airspace::bdryclass_ead_ctr, "not loaded"
"ENJA", AirspacesDb::Airspace::bdryclass_ead_ctr, "not loaded"
"EPWWI", AirspacesDb::Airspace::bdryclass_ead_cta, "not loaded"
"ESMM", AirspacesDb::Airspace::bdryclass_ead_cta, "not loaded"
"ESND", AirspacesDb::Airspace::bdryclass_ead_tma, "not loaded"
"LCRA", AirspacesDb::Airspace::bdryclass_ead_ctr, "not loaded"
"LGMD", AirspacesDb::Airspace::bdryclass_ead_cta, "not loaded"
"LICD", AirspacesDb::Airspace::bdryclass_ead_ctr, "not loaded"
"LSZA", AirspacesDb::Airspace::bdryclass_ead_tma, "not loaded"
"LOVB", AirspacesDb::Airspace::bdryclass_ead_cta, "not loaded"
"LSAZF", AirspacesDb::Airspace::bdryclass_ead_tma, "not loaded"
"LSES", AirspacesDb::Airspace::bdryclass_ead_cta, "not loaded"
"LSZL", AirspacesDb::Airspace::bdryclass_ead_tma, "not loaded"
"ULMMN", AirspacesDb::Airspace::bdryclass_ead_cta, "not loaded"
"ULMMS", AirspacesDb::Airspace::bdryclass_ead_cta, "not loaded"

"EHAM",  AirspacesDb::Airspace::bdryclass_ead_ctr, "loaded"
"EHAM",  AirspacesDb::Airspace::bdryclass_ead_tma, "loaded"
"EHBK",  AirspacesDb::Airspace::bdryclass_ead_ctr, "loaded"
"EHBK",  AirspacesDb::Airspace::bdryclass_ead_tma, "loaded"
"EHRD",  AirspacesDb::Airspace::bdryclass_ead_tma, "loaded"

"EVLA",  AirspacesDb::Airspace::bdryclass_ead_tma, "withdrawn wef 13dec2012"
"ULOL",  AirspacesDb::Airspace::bdryclass_ead_cta, "withdrawn wef 7mar2013"

*/

const DbXmlImporter::AirspaceMapping DbXmlImporter::airspacemapping[] = {
	DbXmlImporter::AirspaceMapping("EBBUI", AirspacesDb::Airspace::bdryclass_ead_cta, "EBBU",  AirspacesDb::Airspace::bdryclass_ead_cta),
	DbXmlImporter::AirspaceMapping("EDMM",  AirspacesDb::Airspace::bdryclass_ead_cta, "EDMM",  AirspacesDb::Airspace::bdryclass_ead_fir),
	DbXmlImporter::AirspaceMapping("EDUU",  AirspacesDb::Airspace::bdryclass_ead_uta, "EDUU",  AirspacesDb::Airspace::bdryclass_ead_uir),
	DbXmlImporter::AirspaceMapping("EDWW",  AirspacesDb::Airspace::bdryclass_ead_cta, "EDWW",  AirspacesDb::Airspace::bdryclass_ead_fir),
	DbXmlImporter::AirspaceMapping("EGBB",  AirspacesDb::Airspace::bdryclass_ead_tma, "EGBB",  AirspacesDb::Airspace::bdryclass_ead_cta),
	DbXmlImporter::AirspaceMapping("EGFF",  AirspacesDb::Airspace::bdryclass_ead_tma, "EGFF",  AirspacesDb::Airspace::bdryclass_ead_cta),
	DbXmlImporter::AirspaceMapping("EGGP",  AirspacesDb::Airspace::bdryclass_ead_tma, "EGGP",  AirspacesDb::Airspace::bdryclass_ead_cta),
	DbXmlImporter::AirspaceMapping("EGJJ",  AirspacesDb::Airspace::bdryclass_ead_tma, "EGJJ",  AirspacesDb::Airspace::bdryclass_ead_cta),
	DbXmlImporter::AirspaceMapping("EGNM",  AirspacesDb::Airspace::bdryclass_ead_tma, "EGNM",  AirspacesDb::Airspace::bdryclass_ead_cta),
	DbXmlImporter::AirspaceMapping("EGNS",  AirspacesDb::Airspace::bdryclass_ead_tma, "EGNS",  AirspacesDb::Airspace::bdryclass_ead_cta),
	DbXmlImporter::AirspaceMapping("EGNT",  AirspacesDb::Airspace::bdryclass_ead_tma, "EGNT",  AirspacesDb::Airspace::bdryclass_ead_cta),
	DbXmlImporter::AirspaceMapping("EGNV",  AirspacesDb::Airspace::bdryclass_ead_tma, "EGNV",  AirspacesDb::Airspace::bdryclass_ead_cta),
	DbXmlImporter::AirspaceMapping("EGNX",  AirspacesDb::Airspace::bdryclass_ead_tma, "EGNX",  AirspacesDb::Airspace::bdryclass_ead_cta),
	DbXmlImporter::AirspaceMapping("EGPB",  AirspacesDb::Airspace::bdryclass_ead_tma, "EGPB",  AirspacesDb::Airspace::bdryclass_ead_cta),
	DbXmlImporter::AirspaceMapping("EGPD",  AirspacesDb::Airspace::bdryclass_ead_tma, "EGPD",  AirspacesDb::Airspace::bdryclass_ead_cta),
	DbXmlImporter::AirspaceMapping("EGPF",  AirspacesDb::Airspace::bdryclass_ead_tma, "EGPF",  AirspacesDb::Airspace::bdryclass_ead_cta),
	DbXmlImporter::AirspaceMapping("EGPH",  AirspacesDb::Airspace::bdryclass_ead_tma, "EGPH",  AirspacesDb::Airspace::bdryclass_ead_cta),
	DbXmlImporter::AirspaceMapping("EGPX",  AirspacesDb::Airspace::bdryclass_ead_cta, "EGPX",  AirspacesDb::Airspace::bdryclass_ead_fir),
	DbXmlImporter::AirspaceMapping("EGSS",  AirspacesDb::Airspace::bdryclass_ead_tma, "EGSS",  AirspacesDb::Airspace::bdryclass_ead_cta),
	DbXmlImporter::AirspaceMapping("EGTT",  AirspacesDb::Airspace::bdryclass_ead_cta, "EGTT",  AirspacesDb::Airspace::bdryclass_ead_fir),
	DbXmlImporter::AirspaceMapping("EHAA",  AirspacesDb::Airspace::bdryclass_ead_cta, "EHAA",  AirspacesDb::Airspace::bdryclass_ead_fir),
	DbXmlImporter::AirspaceMapping("EHMC",  AirspacesDb::Airspace::bdryclass_ead_cta, "EHMCN", AirspacesDb::Airspace::bdryclass_ead_cta),
	DbXmlImporter::AirspaceMapping("EKDK",  AirspacesDb::Airspace::bdryclass_ead_cta, "EKDK",  AirspacesDb::Airspace::bdryclass_ead_fir),
	//DbXmlImporter::AirspaceMapping("EKEB",  AirspacesDb::Airspace::bdryclass_ead_tma, "EKEB",  AirspacesDb::Airspace::bdryclass_ead_rad),
	//DbXmlImporter::AirspaceMapping("EKMB",  AirspacesDb::Airspace::bdryclass_ead_tma, "EKMB",  AirspacesDb::Airspace::bdryclass_ead_rad),
	//DbXmlImporter::AirspaceMapping("EKOD",  AirspacesDb::Airspace::bdryclass_ead_tma, "EKOD",  AirspacesDb::Airspace::bdryclass_ead_rad),
	//DbXmlImporter::AirspaceMapping("EKSB",  AirspacesDb::Airspace::bdryclass_ead_tma, "EKSB",  AirspacesDb::Airspace::bdryclass_ead_rad),
	//DbXmlImporter::AirspaceMapping("EKVD",  AirspacesDb::Airspace::bdryclass_ead_tma, "EKVD",  AirspacesDb::Airspace::bdryclass_ead_rad),
	//DbXmlImporter::AirspaceMapping("EKVJ",  AirspacesDb::Airspace::bdryclass_ead_tma, "EKVJ",  AirspacesDb::Airspace::bdryclass_ead_rad),
	DbXmlImporter::AirspaceMapping("ELLXI", AirspacesDb::Airspace::bdryclass_ead_cta, "ELLX",  AirspacesDb::Airspace::bdryclass_ead_tma),
	//DbXmlImporter::AirspaceMapping("ENHE",  AirspacesDb::Airspace::bdryclass_ead_tma, "ENHE",  AirspacesDb::Airspace::bdryclass_ead_rad),
	DbXmlImporter::AirspaceMapping("ENOS",  AirspacesDb::Airspace::bdryclass_ead_cta, "ENOS",  AirspacesDb::Airspace::bdryclass_ead_fir),
	DbXmlImporter::AirspaceMapping("EPPO",  AirspacesDb::Airspace::bdryclass_ead_tma, "EPPO_N",AirspacesDb::Airspace::bdryclass_ead_tma, "EPPO_S",AirspacesDb::Airspace::bdryclass_ead_tma),
	DbXmlImporter::AirspaceMapping("EPWW",  AirspacesDb::Airspace::bdryclass_ead_cta, "EPWW",  AirspacesDb::Airspace::bdryclass_ead_fir),
	DbXmlImporter::AirspaceMapping("ESIB",  AirspacesDb::Airspace::bdryclass_ead_tma, "ESIB",  AirspacesDb::Airspace::bdryclass_ead_ctr),
	DbXmlImporter::AirspaceMapping("ESOS",  AirspacesDb::Airspace::bdryclass_ead_cta, "ESOS",  AirspacesDb::Airspace::bdryclass_ead_fir),
	DbXmlImporter::AirspaceMapping("EYKA",  AirspacesDb::Airspace::bdryclass_ead_cta, "EYKA",  AirspacesDb::Airspace::bdryclass_ead_tma),
	DbXmlImporter::AirspaceMapping("EYPA",  AirspacesDb::Airspace::bdryclass_ead_cta, "EYPA",  AirspacesDb::Airspace::bdryclass_ead_tma),
	DbXmlImporter::AirspaceMapping("EYSA",  AirspacesDb::Airspace::bdryclass_ead_cta, "EYSA",  AirspacesDb::Airspace::bdryclass_ead_tma),
	DbXmlImporter::AirspaceMapping("EYVI",  AirspacesDb::Airspace::bdryclass_ead_cta, "EYVI",  AirspacesDb::Airspace::bdryclass_ead_tma),
	DbXmlImporter::AirspaceMapping("GCCC",  AirspacesDb::Airspace::bdryclass_ead_cta, "GCCC",  AirspacesDb::Airspace::bdryclass_ead_fir),
	DbXmlImporter::AirspaceMapping("GCTS",  AirspacesDb::Airspace::bdryclass_ead_tma, "GCTS",  AirspacesDb::Airspace::bdryclass_ead_ctr),
	DbXmlImporter::AirspaceMapping("GCXO",  AirspacesDb::Airspace::bdryclass_ead_tma, "GCXO",  AirspacesDb::Airspace::bdryclass_ead_ctr),
	DbXmlImporter::AirspaceMapping("LBSRI", AirspacesDb::Airspace::bdryclass_ead_cta, "LBSR",  AirspacesDb::Airspace::bdryclass_ead_cta),
	DbXmlImporter::AirspaceMapping("LCCC",  AirspacesDb::Airspace::bdryclass_ead_cta, "LCCC",  AirspacesDb::Airspace::bdryclass_ead_fir),
	DbXmlImporter::AirspaceMapping("LEAB",  AirspacesDb::Airspace::bdryclass_ead_tma, "LEAB",  AirspacesDb::Airspace::bdryclass_ead_ctr, "LEAB",  AirspacesDb::Airspace::bdryclass_ead_cta),
	DbXmlImporter::AirspaceMapping("LECB",  AirspacesDb::Airspace::bdryclass_ead_cta, "LECB",  AirspacesDb::Airspace::bdryclass_ead_fir),
	DbXmlImporter::AirspaceMapping("LECM",  AirspacesDb::Airspace::bdryclass_ead_cta, "LECM",  AirspacesDb::Airspace::bdryclass_ead_fir),
	DbXmlImporter::AirspaceMapping("LECP",  AirspacesDb::Airspace::bdryclass_ead_cta, "LECP",  AirspacesDb::Airspace::bdryclass_ead_tma),
	DbXmlImporter::AirspaceMapping("LECS",  AirspacesDb::Airspace::bdryclass_ead_cta, "LECS",  AirspacesDb::Airspace::bdryclass_ead_tma),
	DbXmlImporter::AirspaceMapping("LEGE",  AirspacesDb::Airspace::bdryclass_ead_tma, "LEGE",  AirspacesDb::Airspace::bdryclass_ead_ctr),
	DbXmlImporter::AirspaceMapping("LEGR",  AirspacesDb::Airspace::bdryclass_ead_tma, "LEGR",  AirspacesDb::Airspace::bdryclass_ead_ctr),
	DbXmlImporter::AirspaceMapping("LELN",  AirspacesDb::Airspace::bdryclass_ead_tma, "LELN",  AirspacesDb::Airspace::bdryclass_ead_ctr),
	DbXmlImporter::AirspaceMapping("LEMD",  AirspacesDb::Airspace::bdryclass_ead_tma, "LEMD",  AirspacesDb::Airspace::bdryclass_ead_ctr),
	DbXmlImporter::AirspaceMapping("LEMG",  AirspacesDb::Airspace::bdryclass_ead_tma, "LEMG",  AirspacesDb::Airspace::bdryclass_ead_ctr),
	DbXmlImporter::AirspaceMapping("LEMH",  AirspacesDb::Airspace::bdryclass_ead_tma, "LEMH",  AirspacesDb::Airspace::bdryclass_ead_ctr),
	DbXmlImporter::AirspaceMapping("LEPA",  AirspacesDb::Airspace::bdryclass_ead_tma, "LEPA",  AirspacesDb::Airspace::bdryclass_ead_ctr),
	DbXmlImporter::AirspaceMapping("LEPP",  AirspacesDb::Airspace::bdryclass_ead_tma, "LEPP",  AirspacesDb::Airspace::bdryclass_ead_ctr, "LEPP",  AirspacesDb::Airspace::bdryclass_ead_cta),
	DbXmlImporter::AirspaceMapping("LERS",  AirspacesDb::Airspace::bdryclass_ead_tma, "LERS",  AirspacesDb::Airspace::bdryclass_ead_ctr),
	DbXmlImporter::AirspaceMapping("LESA",  AirspacesDb::Airspace::bdryclass_ead_tma, "LESA",  AirspacesDb::Airspace::bdryclass_ead_ctr),
	DbXmlImporter::AirspaceMapping("LESO",  AirspacesDb::Airspace::bdryclass_ead_tma, "LESO",  AirspacesDb::Airspace::bdryclass_ead_ctr, "LESO",  AirspacesDb::Airspace::bdryclass_ead_cta),
	DbXmlImporter::AirspaceMapping("LEVT",  AirspacesDb::Airspace::bdryclass_ead_tma, "LEVT",  AirspacesDb::Airspace::bdryclass_ead_ctr, "LEVT",  AirspacesDb::Airspace::bdryclass_ead_cta),
	DbXmlImporter::AirspaceMapping("LFBB",  AirspacesDb::Airspace::bdryclass_ead_cta, "LFBB",  AirspacesDb::Airspace::bdryclass_ead_fir),
	DbXmlImporter::AirspaceMapping("LFBP",  AirspacesDb::Airspace::bdryclass_ead_tma, "LFBP",  AirspacesDb::Airspace::bdryclass_ead_ctr),
	DbXmlImporter::AirspaceMapping("LFEE",  AirspacesDb::Airspace::bdryclass_ead_cta, "LFEE",  AirspacesDb::Airspace::bdryclass_ead_fir),
	DbXmlImporter::AirspaceMapping("LFFF",  AirspacesDb::Airspace::bdryclass_ead_cta, "LFFF",  AirspacesDb::Airspace::bdryclass_ead_fir),
	DbXmlImporter::AirspaceMapping("LFMI",  AirspacesDb::Airspace::bdryclass_ead_tma, "LFMI",  AirspacesDb::Airspace::bdryclass_ead_ctr),
	DbXmlImporter::AirspaceMapping("LFMM",  AirspacesDb::Airspace::bdryclass_ead_cta, "LFMM",  AirspacesDb::Airspace::bdryclass_ead_fir),
	DbXmlImporter::AirspaceMapping("LFMNF", AirspacesDb::Airspace::bdryclass_ead_cta, "LFMN",  AirspacesDb::Airspace::bdryclass_ead_tma),
	DbXmlImporter::AirspaceMapping("LFPM",  AirspacesDb::Airspace::bdryclass_ead_tma, "LFPM",  AirspacesDb::Airspace::bdryclass_ead_ctr),
	DbXmlImporter::AirspaceMapping("LFRB",  AirspacesDb::Airspace::bdryclass_ead_tma, "LFRB",  AirspacesDb::Airspace::bdryclass_ead_ctr),
	DbXmlImporter::AirspaceMapping("LFRR",  AirspacesDb::Airspace::bdryclass_ead_cta, "LFRR",  AirspacesDb::Airspace::bdryclass_ead_fir),
	DbXmlImporter::AirspaceMapping("LFTH",  AirspacesDb::Airspace::bdryclass_ead_tma, "LFTH",  AirspacesDb::Airspace::bdryclass_ead_ctr),
	DbXmlImporter::AirspaceMapping("LFTW",  AirspacesDb::Airspace::bdryclass_ead_tma, "LFTW",  AirspacesDb::Airspace::bdryclass_ead_ctr),
	DbXmlImporter::AirspaceMapping("LGGG",  AirspacesDb::Airspace::bdryclass_ead_cta, "LGGG",  AirspacesDb::Airspace::bdryclass_ead_fir),
	DbXmlImporter::AirspaceMapping("LIBA",  AirspacesDb::Airspace::bdryclass_ead_tma, "LIBA",  AirspacesDb::Airspace::bdryclass_ead_ctr),
	DbXmlImporter::AirspaceMapping("LIBB",  AirspacesDb::Airspace::bdryclass_ead_cta, "LIBB",  AirspacesDb::Airspace::bdryclass_ead_fir),
	DbXmlImporter::AirspaceMapping("LIBN",  AirspacesDb::Airspace::bdryclass_ead_tma, "LIBN",  AirspacesDb::Airspace::bdryclass_ead_ctr),
	DbXmlImporter::AirspaceMapping("LIBR",  AirspacesDb::Airspace::bdryclass_ead_tma, "LIBR",  AirspacesDb::Airspace::bdryclass_ead_ctr),
	DbXmlImporter::AirspaceMapping("LIBV",  AirspacesDb::Airspace::bdryclass_ead_tma, "LIBV",  AirspacesDb::Airspace::bdryclass_ead_ctr),
	DbXmlImporter::AirspaceMapping("LICT",  AirspacesDb::Airspace::bdryclass_ead_tma, "LICT",  AirspacesDb::Airspace::bdryclass_ead_ctr),
	DbXmlImporter::AirspaceMapping("LICZ",  AirspacesDb::Airspace::bdryclass_ead_tma, "LICZ",  AirspacesDb::Airspace::bdryclass_ead_ctr),
	DbXmlImporter::AirspaceMapping("LIED",  AirspacesDb::Airspace::bdryclass_ead_tma, "LIED",  AirspacesDb::Airspace::bdryclass_ead_ctr),
	DbXmlImporter::AirspaceMapping("LIMM",  AirspacesDb::Airspace::bdryclass_ead_cta, "LIMM",  AirspacesDb::Airspace::bdryclass_ead_fir),
	DbXmlImporter::AirspaceMapping("LIMS",  AirspacesDb::Airspace::bdryclass_ead_tma, "LIMS",  AirspacesDb::Airspace::bdryclass_ead_ctr),
	DbXmlImporter::AirspaceMapping("LIPA",  AirspacesDb::Airspace::bdryclass_ead_tma, "LIPA",  AirspacesDb::Airspace::bdryclass_ead_ctr),
	DbXmlImporter::AirspaceMapping("LIPP",  AirspacesDb::Airspace::bdryclass_ead_cta, "LIPP",  AirspacesDb::Airspace::bdryclass_ead_tma),
	DbXmlImporter::AirspaceMapping("LIPR",  AirspacesDb::Airspace::bdryclass_ead_tma, "LIPR",  AirspacesDb::Airspace::bdryclass_ead_ctr),
	DbXmlImporter::AirspaceMapping("LIPS",  AirspacesDb::Airspace::bdryclass_ead_tma, "LIPS",  AirspacesDb::Airspace::bdryclass_ead_ctr),
	DbXmlImporter::AirspaceMapping("LIPX",  AirspacesDb::Airspace::bdryclass_ead_tma, "LIPX",  AirspacesDb::Airspace::bdryclass_ead_ctr),
	DbXmlImporter::AirspaceMapping("LIRL",  AirspacesDb::Airspace::bdryclass_ead_tma, "LIRL",  AirspacesDb::Airspace::bdryclass_ead_ctr),
	DbXmlImporter::AirspaceMapping("LIRM",  AirspacesDb::Airspace::bdryclass_ead_tma, "LIRM",  AirspacesDb::Airspace::bdryclass_ead_ctr),
	DbXmlImporter::AirspaceMapping("LIRP",  AirspacesDb::Airspace::bdryclass_ead_tma, "LIRP",  AirspacesDb::Airspace::bdryclass_ead_ctr),
	DbXmlImporter::AirspaceMapping("LIRR",  AirspacesDb::Airspace::bdryclass_ead_cta, "LIRR",  AirspacesDb::Airspace::bdryclass_ead_fir),
	DbXmlImporter::AirspaceMapping("LIRS",  AirspacesDb::Airspace::bdryclass_ead_tma, "LIRS",  AirspacesDb::Airspace::bdryclass_ead_ctr),
	DbXmlImporter::AirspaceMapping("LJLA",  AirspacesDb::Airspace::bdryclass_ead_cta, "LJLA",  AirspacesDb::Airspace::bdryclass_ead_fir),
	DbXmlImporter::AirspaceMapping("LJMB",  AirspacesDb::Airspace::bdryclass_ead_tma, "LJMB",  AirspacesDb::Airspace::bdryclass_ead_ctr),
	DbXmlImporter::AirspaceMapping("LKAA",  AirspacesDb::Airspace::bdryclass_ead_cta, "LKAA",  AirspacesDb::Airspace::bdryclass_ead_fir),
	DbXmlImporter::AirspaceMapping("LKAA",  AirspacesDb::Airspace::bdryclass_ead_uta, "LKAA",  AirspacesDb::Airspace::bdryclass_ead_fir),
	DbXmlImporter::AirspaceMapping("LMMM",  AirspacesDb::Airspace::bdryclass_ead_cta, "LMMM",  AirspacesDb::Airspace::bdryclass_ead_fir),
	DbXmlImporter::AirspaceMapping("LOVV",  AirspacesDb::Airspace::bdryclass_ead_cta, "LOVV",  AirspacesDb::Airspace::bdryclass_ead_fir),
	DbXmlImporter::AirspaceMapping("LOWI",  AirspacesDb::Airspace::bdryclass_ead_tma, "LOWIE", AirspacesDb::Airspace::bdryclass_ead_cta, "LOWIS", AirspacesDb::Airspace::bdryclass_ead_cta, "LOWIW", AirspacesDb::Airspace::bdryclass_ead_cta),
	DbXmlImporter::AirspaceMapping("LOWW",  AirspacesDb::Airspace::bdryclass_ead_tma, "LOWW",  AirspacesDb::Airspace::bdryclass_ead_cta),
	DbXmlImporter::AirspaceMapping("LPLA",  AirspacesDb::Airspace::bdryclass_ead_tma, "LPLA",  AirspacesDb::Airspace::bdryclass_ead_ctr, "LPLA",  AirspacesDb::Airspace::bdryclass_ead_cta),
	DbXmlImporter::AirspaceMapping("LPPC",  AirspacesDb::Airspace::bdryclass_ead_cta, "LPPC",  AirspacesDb::Airspace::bdryclass_ead_fir),
	DbXmlImporter::AirspaceMapping("LQSB",  AirspacesDb::Airspace::bdryclass_ead_cta, "LQSB",  AirspacesDb::Airspace::bdryclass_ead_fir),
	DbXmlImporter::AirspaceMapping("LRBB",  AirspacesDb::Airspace::bdryclass_ead_cta, "LRBB",  AirspacesDb::Airspace::bdryclass_ead_fir),
	DbXmlImporter::AirspaceMapping("LSAG",  AirspacesDb::Airspace::bdryclass_ead_uta, "LSAGALL",  AirspacesDb::Airspace::bdryclass_ead_sector_c),
	DbXmlImporter::AirspaceMapping("LSAZ",  AirspacesDb::Airspace::bdryclass_ead_uta, "LSAZM16",  AirspacesDb::Airspace::bdryclass_ead_sector_c),
	DbXmlImporter::AirspaceMapping("LSAZF", AirspacesDb::Airspace::bdryclass_ead_tma, "LSAZEDNY", AirspacesDb::Airspace::bdryclass_ead_sector),
	DbXmlImporter::AirspaceMapping("LSES",  AirspacesDb::Airspace::bdryclass_ead_cta, "LSESLFIS", AirspacesDb::Airspace::bdryclass_ead_sector),
	DbXmlImporter::AirspaceMapping("LSZL",  AirspacesDb::Airspace::bdryclass_ead_tma, "LSZLTA",   AirspacesDb::Airspace::bdryclass_ead_sector),
	DbXmlImporter::AirspaceMapping("LTAA",  AirspacesDb::Airspace::bdryclass_ead_cta, "LTAA",  AirspacesDb::Airspace::bdryclass_ead_fir),
	DbXmlImporter::AirspaceMapping("LTAH",  AirspacesDb::Airspace::bdryclass_ead_tma, "LTAH",  AirspacesDb::Airspace::bdryclass_ead_ctr),
	DbXmlImporter::AirspaceMapping("LTBB",  AirspacesDb::Airspace::bdryclass_ead_cta, "LTBB",  AirspacesDb::Airspace::bdryclass_ead_fir),
	DbXmlImporter::AirspaceMapping("LUUU",  AirspacesDb::Airspace::bdryclass_ead_cta, "LUUU",  AirspacesDb::Airspace::bdryclass_ead_fir),
	DbXmlImporter::AirspaceMapping("UBBA",  AirspacesDb::Airspace::bdryclass_ead_cta, "UBBA",  AirspacesDb::Airspace::bdryclass_ead_fir),
	DbXmlImporter::AirspaceMapping("UDDD",  AirspacesDb::Airspace::bdryclass_ead_cta, "UDDD",  AirspacesDb::Airspace::bdryclass_ead_fir),
	DbXmlImporter::AirspaceMapping("UKBV",  AirspacesDb::Airspace::bdryclass_ead_tma, "UKBV",  AirspacesDb::Airspace::bdryclass_ead_fir, "UKBV",  AirspacesDb::Airspace::bdryclass_ead_cta),
	DbXmlImporter::AirspaceMapping("UKDV",  AirspacesDb::Airspace::bdryclass_ead_cta, "UKDV",  AirspacesDb::Airspace::bdryclass_ead_fir),
	DbXmlImporter::AirspaceMapping("UKLV",  AirspacesDb::Airspace::bdryclass_ead_tma, "UKLV",  AirspacesDb::Airspace::bdryclass_ead_fir, "UKLV",  AirspacesDb::Airspace::bdryclass_ead_cta),
	DbXmlImporter::AirspaceMapping("UKOV",  AirspacesDb::Airspace::bdryclass_ead_tma, "UKOV",  AirspacesDb::Airspace::bdryclass_ead_fir, "UKOV",  AirspacesDb::Airspace::bdryclass_ead_cta),
	DbXmlImporter::AirspaceMapping("ULLL",  AirspacesDb::Airspace::bdryclass_ead_cta, "ULLL",  AirspacesDb::Airspace::bdryclass_ead_fir),
	DbXmlImporter::AirspaceMapping("UMKK",  AirspacesDb::Airspace::bdryclass_ead_cta, "UMKK",  AirspacesDb::Airspace::bdryclass_ead_fir),
	DbXmlImporter::AirspaceMapping("URRV",  AirspacesDb::Airspace::bdryclass_ead_cta, "URRV",  AirspacesDb::Airspace::bdryclass_ead_fir),
	DbXmlImporter::AirspaceMapping("UUWV",  AirspacesDb::Airspace::bdryclass_ead_cta, "UUWV",  AirspacesDb::Airspace::bdryclass_ead_fir)
};

const DbXmlImporter::AirspaceMapping *DbXmlImporter::find_airspace_mapping(const std::string& name, char bc) const
{
	static const AirspaceMapping *first(&airspacemapping[0]);
	static const AirspaceMapping *last(&airspacemapping[sizeof(airspacemapping)/sizeof(airspacemapping[0])]);
	AirspaceMapping x(name.c_str(), bc);
	const AirspaceMapping *r(std::lower_bound(first, last, x));
	if (r == last)
		return 0;
	if (x < *r)
		return 0;
	if (true && (name != r->get_name(0) || bc != r->get_bdryclass(0)))
		throw std::runtime_error("find_airspace_mapping error");
	return r;
}

void DbXmlImporter::check_airspace_mapping(void)
{
	for (unsigned int i = 0; i < sizeof(airspacemapping)/sizeof(airspacemapping[0]); ++i) {
		if (airspacemapping[i].size() >= 1)
			continue;
		std::ostringstream oss;
		oss << "Airspace mapping " << airspacemapping[i].get_name(0) << '/'
		    << AirspacesDb::Airspace::get_class_string(airspacemapping[i].get_bdryclass(0), AirspacesDb::Airspace::typecode_ead)
		    << " is invalid";
		std::cerr << oss.str() << std::endl;
		throw std::runtime_error(oss.str());
	}
	for (unsigned int i = 1; i < sizeof(airspacemapping)/sizeof(airspacemapping[0]); ++i) {
		if (airspacemapping[i-1] < airspacemapping[i])
			continue;
		std::ostringstream oss;
		oss << "Airspace mapping " << airspacemapping[i].get_name(0) << '/'
		    << AirspacesDb::Airspace::get_class_string(airspacemapping[i].get_bdryclass(0), AirspacesDb::Airspace::typecode_ead)
		    << " has non-ascending sort order";
		std::cerr << oss.str() << std::endl;
		throw std::runtime_error(oss.str());
	}
}

void DbXmlImporter::parse_auag(const Glib::ustring& auagfile)
{
	std::ifstream ifs(auagfile.c_str());
	if (!ifs.is_open()) {
		std::cerr << "Cannot open " << auagfile << std::endl;
		return;
	}
	while (!ifs.eof()) {
		typedef std::vector<std::string> token_t;
		token_t token;
		{
			std::string line;
			std::getline(ifs, line);
			std::string::size_type n(0);
			while (n < line.size()) {
				std::string::size_type n1(line.find(',', n));
				if (n1 == std::string::npos) {
				        token.push_back(line.substr(n));
					break;
				}
				token.push_back(line.substr(n, n1 - n));
				n = n1 + 1;
			}
			for (token_t::iterator i(token.begin()), e(token.end()); i != e; ++i) {
				std::string& x(*i);
				while (!x.empty() && std::isspace(x[0]))
					x.erase(0, 1);
				while (!x.empty() && std::isspace(x[x.size()-1]))
					x.erase(x.size()-1, 1);
			}
		}
		if (token.empty())
			continue;
		open_airspaces_db();
		AirspacesDb::element_t el;
		el.set_sourceid("RAS_" + token[0] + "@EAD");
		el.set_label_placement(AirspacesDb::element_t::label_e);
		// check for existing airspace
		bool updated(false);
		{
			AirspacesDb::elementvector_t ev(m_airspacesdb.find_by_sourceid(el.get_sourceid()));
			for (AirspacesDb::elementvector_t::const_iterator evi(ev.begin()), eve(ev.end()); evi != eve; ++evi) {
				if (evi->get_sourceid() != el.get_sourceid())
					continue;
				el = *evi;
				updated = true;
				break;
			}
		}
		el.set_icao(token[0]);
		if (el.get_name().empty())
			el.set_name(token[0]);
		el.set_typecode(AirspacesDb::Airspace::typecode_ead);
		el.set_bdryclass(AirspacesDb::Airspace::bdryclass_ead_ras);
		if (!updated) {
			el.set_altlwr(0);
			el.set_altlwrflags(AirspacesDb::Airspace::altflag_gnd);
			el.set_altupr(66000);
			el.set_altuprflags(AirspacesDb::Airspace::altflag_fl);
		}
		if (el.get_ident().empty())
			el.set_ident(token[0]);
		{
			token_t::const_iterator ti(token.begin()), te(token.end());
			++ti;
			if (ti != te) {
				el.clear_segments();
				el.clear_components();
				AirspacesDb::Airspace::Component::operator_t opr(AirspacesDb::Airspace::Component::operator_set);
				for (; ti != te; ++ti) {
					static const char bdryclasses[] = {
						AirspacesDb::Airspace::bdryclass_ead_adiz,
						AirspacesDb::Airspace::bdryclass_ead_ama,
						AirspacesDb::Airspace::bdryclass_ead_arinc,
						AirspacesDb::Airspace::bdryclass_ead_asr,
						AirspacesDb::Airspace::bdryclass_ead_awy,
						AirspacesDb::Airspace::bdryclass_ead_cba,
						AirspacesDb::Airspace::bdryclass_ead_class,
						AirspacesDb::Airspace::bdryclass_ead_cta,
						AirspacesDb::Airspace::bdryclass_ead_cta_p,
						AirspacesDb::Airspace::bdryclass_ead_ctr,
						AirspacesDb::Airspace::bdryclass_ead_ctr_p,
						AirspacesDb::Airspace::bdryclass_ead_d_amc,
						AirspacesDb::Airspace::bdryclass_ead_d_other,
						AirspacesDb::Airspace::bdryclass_ead_ead,
						AirspacesDb::Airspace::bdryclass_ead_fir,
						AirspacesDb::Airspace::bdryclass_ead_fir_p,
						AirspacesDb::Airspace::bdryclass_ead_nas,
						AirspacesDb::Airspace::bdryclass_ead_no_fir,
						AirspacesDb::Airspace::bdryclass_ead_oca,
						AirspacesDb::Airspace::bdryclass_ead_oca_p,
						AirspacesDb::Airspace::bdryclass_ead_ota,
						AirspacesDb::Airspace::bdryclass_ead_part,
						AirspacesDb::Airspace::bdryclass_ead_political,
						AirspacesDb::Airspace::bdryclass_ead_protect,
						AirspacesDb::Airspace::bdryclass_ead_r_amc,
						AirspacesDb::Airspace::bdryclass_ead_ras,
						AirspacesDb::Airspace::bdryclass_ead_rca,
						AirspacesDb::Airspace::bdryclass_ead_sector_c,
						AirspacesDb::Airspace::bdryclass_ead_sector,
						AirspacesDb::Airspace::bdryclass_ead_tma,
						AirspacesDb::Airspace::bdryclass_ead_tma_p,
						AirspacesDb::Airspace::bdryclass_ead_tra,
						AirspacesDb::Airspace::bdryclass_ead_tsa,
						AirspacesDb::Airspace::bdryclass_ead_uir,
						AirspacesDb::Airspace::bdryclass_ead_uir_p,
						AirspacesDb::Airspace::bdryclass_ead_uta,
						AirspacesDb::Airspace::bdryclass_ead_p,
						AirspacesDb::Airspace::bdryclass_ead_r,
						AirspacesDb::Airspace::bdryclass_ead_d,
						AirspacesDb::Airspace::bdryclass_ead_a,
						AirspacesDb::Airspace::bdryclass_ead_w
					};
					char bc(0);
					std::string id(*ti);
					for (unsigned int i = 0; i < sizeof(bdryclasses)/sizeof(bdryclasses[0]); ++i) {
						Glib::ustring cs(AirspacesDb::Airspace::get_class_string(bdryclasses[i], AirspacesDb::Airspace::typecode_ead));
						if (bdryclasses[i] == AirspacesDb::Airspace::bdryclass_ead_d)
							cs = "D";
						if (cs.size() >= id.size())
							continue;
						if (id.compare(id.size() - cs.size(), cs.size(), cs))
							continue;
						bc = bdryclasses[i];
						id.erase(id.size() - cs.size(), cs.size());
						break;
					}
					if (!bc)
						std::cerr << "WARNING: " << id << " airspace suffix not found" << std::endl;
					AirspacesDb::Airspace aspc;
					AirspacesDb::elementvector_t ev(m_airspacesdb.find_by_icao(id, 0, AirspacesDb::comp_exact, 0, AirspacesDb::Airspace::subtables_all));
					for (AirspacesDb::elementvector_t::const_iterator evi(ev.begin()), eve(ev.end()); evi != eve; ++evi) {
						if (evi->get_typecode() != AirspacesDb::Airspace::typecode_ead)
							continue;
						if (evi->get_bdryclass() == bc || (!bc && !aspc.is_valid()))
							aspc = *evi;
					}
					if (aspc.is_valid()) {
						el.add_component(AirspacesDb::Airspace::Component(id, AirspacesDb::Airspace::typecode_ead, bc, opr));
						opr = AirspacesDb::Airspace::Component::operator_union;
						continue;
					}
					const AirspaceMapping *m(find_airspace_mapping(id, bc));
					if (!m || m->size() < 1) {
						std::cerr << "WARNING: " << *ti << " not found" << std::endl;
						continue;
					}
					for (unsigned int i = 1; i < m->size(); ++i) {
						AirspacesDb::elementvector_t ev(m_airspacesdb.find_by_icao(m->get_name(i), 0, AirspacesDb::comp_exact, 0, AirspacesDb::Airspace::subtables_all));
						aspc = AirspacesDb::Airspace();
						for (AirspacesDb::elementvector_t::const_iterator evi(ev.begin()), eve(ev.end()); evi != eve; ++evi) {
							if (evi->get_typecode() != AirspacesDb::Airspace::typecode_ead)
								continue;
							if (evi->get_bdryclass() == m->get_bdryclass(i))
								aspc = *evi;
						}
						if (aspc.is_valid()) {
							el.add_component(AirspacesDb::Airspace::Component(m->get_name(i), AirspacesDb::Airspace::typecode_ead, m->get_bdryclass(i), opr));
							opr = AirspacesDb::Airspace::Component::operator_union;
							continue;
						}
						std::cerr << "WARNING: " << *ti << ": substitute airspace " << m->get_name(i) << '/'
							  << AirspacesDb::Airspace::get_class_string(m->get_bdryclass(i), AirspacesDb::Airspace::typecode_ead)
							  << " not found" << std::endl;
					}
				}
				el.recompute_lineapprox(m_airspacesdb);
				el.recompute_bbox();
			}
		}
		if (!el.get_bbox().is_inside(el.get_labelcoord()) || !el.get_polygon().windingnumber(el.get_labelcoord()))
			el.compute_initial_label_placement();
		el.set_modtime(time(0));
		if (true)
			std::cerr << (updated ? "Modified" : "New") << " Airspace " << el.get_type_string() << '/' << el.get_class_string() << ' '
				  << el.get_icao() << ' ' << el.get_name() << ' ' << el.get_sourceid() << std::endl;
		try {
			m_airspacesdb.save(el);
		} catch (const std::exception& e) {
			std::cerr << "I/O Error: " << e.what() << std::endl;
			throw;
		}
	}
	ifs.close();
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
	    e.get_bdryclass() != AirspacesDb::Airspace::bdryclass_ead_ead ||
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

void DbXmlImporter::purge_ead_airspaces(void)
{
	open_airspaces_db();
	long cntbefore(-1), cntafter(-1);
	{
		sqlite3x::sqlite3_command cmd(m_airspacesdb.get_db(), "SELECT COUNT(*) FROM airspaces;");
		sqlite3x::sqlite3_cursor cursor = cmd.executecursor();
		if (cursor.step())
			cntbefore = cursor.getint(0);
	}
	try {
		sqlite3x::sqlite3_command cmd(m_airspacesdb.get_db(), "DELETE FROM airspaces_rtree WHERE ID IN (SELECT ID FROM airspaces WHERE SRCID LIKE \"%@EAD\");");
		cmd.executenonquery();
	} catch (const sqlite3x::database_error& ex) {
		std::cerr << "Airspaces: cannot modify rtree: " << ex.what() << std::endl;
	}
	{
		sqlite3x::sqlite3_command cmd(m_airspacesdb.get_db(), "DELETE FROM airspaces WHERE SRCID LIKE \"%@EAD\";");
		cmd.executenonquery();
	}
	{
		sqlite3x::sqlite3_command cmd(m_airspacesdb.get_db(), "SELECT COUNT(*) FROM airspaces;");
		sqlite3x::sqlite3_cursor cursor = cmd.executecursor();
		if (cursor.step())
			cntafter = cursor.getint(0);
	}
	std::cerr << "Airspaces: " << cntbefore << " before, " << cntafter << " after deletion of @EAD airspaces" << std::endl;
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

void DbXmlImporter::recalc(recalc_t mode)
{
	open_airspaces_db();
	DbXmlImporter::RecalcComposites r(m_airspacesdb);
	r.recalc(mode);
}

void DbXmlImporter::recalc(const AirspaceId& aspcid)
{
	open_airspaces_db();
	DbXmlImporter::RecalcComposites r(m_airspacesdb);
	r.recalc(aspcid);
}

void DbXmlImporter::recalc(const AirspacesDb::Address& addr)
{
	open_airspaces_db();
	DbXmlImporter::RecalcComposites r(m_airspacesdb);
	r.recalc(addr);
}

void DbXmlImporter::test(void)
{
}

int main(int argc, char *argv[])
{
        static struct option long_options[] = {
                { "dir", required_argument, 0, 'd' },
                { "authority", no_argument, 0, 'a' },
                { "killead", no_argument, 0, 'k' },
                { "purgeead", no_argument, 0, 'k' },
                { "recalc-empty", no_argument, 0, 0x100 + DbXmlImporter::recalc_empty },
                { "recalc-composite", no_argument, 0, 0x100 + DbXmlImporter::recalc_composite },
                { "recalc-all", no_argument, 0, 0x100 + DbXmlImporter::recalc_all },
		{ "recalc", required_argument, 0, 'R' },
                { "auag", required_argument, 0, 'A' },
                {0, 0, 0, 0}
        };
        Glib::ustring db_dir(".");
	Glib::ustring borderfile("");
	Glib::ustring auagfile("");
	std::vector<DbXmlImporter::AirspaceId> recalcaspcid;
	std::vector<AirspacesDb::Address> recalcaspcaddr;
	DbXmlImporter::recalc_t recalcmode(DbXmlImporter::recalc_empty);
	bool updateauth(false), killead(false), dorecalc(false);
        int c, err(0);

        while ((c = getopt_long(argc, argv, "d:b:akA:R:", long_options, 0)) != EOF) {
                switch (c) {
		case 'd':
			if (optarg)
				db_dir = optarg;
			break;

		case 'b':
			if (optarg)
				borderfile = optarg;
			break;

		case 'a':
			updateauth = true;
			break;

		case 'k':
			killead = true;
			break;

		case 'A':
			if (optarg)
				auagfile = optarg;
			break;

		case 'R':
		{
			if (!optarg)
				break;
			{
				DbXmlImporter::AirspaceId id("", 0, 0);
				if (id.parse(optarg)) {
					recalcaspcid.push_back(id);
					break;
				}
			}
			char *cp(0);
			if (*optarg == 'A' || *optarg == 'M') {
				int64_t id;
			        id = strtoll(optarg + 1, &cp, 10);
				if (!*cp && cp != optarg + 1) {
					recalcaspcaddr.push_back(AirspacesDb::Address(id, *optarg == 'A' ? AirspacesDb::Address::table_aux : AirspacesDb::Address::table_main));
					break;
				}
			}
			std::cerr << "cannot parse airspace ID " << optarg << std::endl;
			break;
		}

		case 0x100 + DbXmlImporter::recalc_empty:
		case 0x100 + DbXmlImporter::recalc_composite:
		case 0x100 + DbXmlImporter::recalc_all:
			recalcmode = (DbXmlImporter::recalc_t)(c - 0x100);
			dorecalc = true;
			break;

		default:
			err++;
			break;
                }
        }
        if (err) {
                std::cerr << "usage: vfrdbaixmimport [-d <dir>] [-a] [-b <borderfile>] [-A <auagfile>] [--recalc-empty] [--recalc-composite] [--recalc-all]" << std::endl;
                return EX_USAGE;
        }
	if (false) {
		DbXmlImporter::test();
		return EX_OK;
	}
        try {
#ifdef HAVE_GDAL
                OGRRegisterAll();
#endif
                DbXmlImporter parser(db_dir, borderfile);
                parser.set_validate(false); // Do not validate, we do not have a DTD
                parser.set_substitute_entities(true);
		if (killead)
			parser.purge_ead_airspaces();
                for (; optind < argc; optind++) {
                        std::cerr << "Parsing file " << argv[optind] << std::endl;
                        parser.parse_file(argv[optind]);
                }
		if (!auagfile.empty())
			parser.parse_auag(auagfile);
		if (dorecalc)
			parser.recalc(recalcmode);
		for (std::vector<DbXmlImporter::AirspaceId>::const_iterator i(recalcaspcid.begin()), e(recalcaspcid.end()); i != e; ++i)
			parser.recalc(*i);
		for (std::vector<AirspacesDb::Address>::const_iterator i(recalcaspcaddr.begin()), e(recalcaspcaddr.end()); i != e; ++i)
			parser.recalc(*i);
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
