#ifndef ADRBDRY_H
#define ADRBDRY_H

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <limits>

#include "adr.hh"
#include "adrdb.hh"
#include "adraerodrome.hh"
#include "adrunit.hh"
#include "adrpoint.hh"
#include "adrroute.hh"
#include "adrairspace.hh"
#include "adrrestriction.hh"

struct OGRGeometry;
struct OGRPolygon;
struct OGRLinearRing;
struct OGRFeatureDefn;
struct OGRFeature;

namespace ADR {

class ParseOGR {
public:
	ParseOGR(Database& db, bool verbose = true);
	virtual ~ParseOGR();

	static void global_init(void);

	void parse(const std::string& file);
	void addcomposite(void);

	unsigned int get_errorcnt(void) const { return m_errorcnt; }
	unsigned int get_warncnt(void) const { return m_warncnt; }

	Database& get_db(void) { return m_db; }

protected:
	typedef std::map<std::string,std::string> namemap_t;
	namemap_t m_namemap;
	typedef std::set<std::string> components_t;
	typedef std::map<std::string,components_t> componentmap_t;
	componentmap_t m_componentmap;
	Database& m_db;
	unsigned int m_errorcnt;
	unsigned int m_warncnt;
	bool m_verbose;

	void init_thematicmapping_tables(void);
	void loadborder(MultiPolygonHole& poly, OGRGeometry *geom, OGRFeatureDefn *layerdef = 0, OGRFeature *feature = 0);
	void loadborder(PolygonHole& ph, OGRPolygon *poly, OGRFeatureDefn *layerdef = 0, OGRFeature *feature = 0);
	void loadborder(PolygonSimple& poly, OGRLinearRing *ring, OGRFeatureDefn *layerdef = 0, OGRFeature *feature = 0);

};

};

#endif /* ADRBDRY_H */
