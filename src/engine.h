//
// C++ Interface: engine
//
// Description: Asynchronous Database Query Engine
//
//
// Author: Thomas Sailer <t.sailer@alumni.ethz.ch>, (C) 2007, 2009, 2012, 2013, 2014, 2016
//
// Copyright: See COPYING file that comes with this distribution
//
//

#ifndef ENGINE_H
#define ENGINE_H

#include "sysdeps.h"

#include "fplan.h"
#include "dbobj.h"
#include "grib2.h"
#include "bitmapmaps.h"
#ifdef HAVE_PILOTLINK
#include "palm.h"
#endif
#ifdef HAVE_BOOST
#include <boost/config.hpp>
#include <boost/graph/adjacency_list.hpp>
#endif

class Preferences;

class Preferences {
private:
	template <typename T, typename LT = T> class PreferencesValue {
	public:
		typedef T datatype_t;
		typedef LT loadtype_t;

		explicit PreferencesValue(const char *name) : m_name(name), m_dirty(false) {}
		void operator=(datatype_t v);
		operator datatype_t() { return m_value; }
		sigc::signal<void,datatype_t>& signal_change(void) { return m_change; }
		const sigc::signal<void,datatype_t>& signal_change(void) const { return m_change; }
		const char *name(void) const { return m_name; }

	private:
		const char *m_name;
		datatype_t m_value;
		sigc::signal<void,datatype_t> m_change;
		bool m_dirty;

		bool is_dirty(void) const { return m_dirty; }
		void clear_dirty(void) { m_dirty = false; }
		void set_dirty(void) { m_dirty = true; }

		friend class Preferences;
	};

	static const char curplan_name[];
	static const char vcruise_name[];
	static const char windspeed_name[];
	static const char winddir_name[];
	static const char qnh_name[];
	static const char temp_name[];
	static const char gps_name[];
	static const char autowpt_name[];
	static const char vblock_name[];
	static const char maxalt_name[];
	static const char mapflags_name[];
	static const char mapscale_name[];
	static const char mapscaleairportdiagram_name[];
	static const char globaldbdir_name[];
	static const char gpshost_name[];
	static const char gpsport_name[];

public:
	Preferences(void);
	~Preferences(void);
	void commit(void);

	PreferencesValue<FPlanRoute::id_t, long long> curplan;
	PreferencesValue<double> vcruise;
	PreferencesValue<bool, long long> gps;
	PreferencesValue<bool, long long> autowpt;
	PreferencesValue<double> vblock;
	PreferencesValue<int, long long> maxalt;
	PreferencesValue<int, long long> mapflags;
	PreferencesValue<double> mapscale;
	PreferencesValue<double> mapscaleairportdiagram;
	PreferencesValue<Glib::ustring> globaldbdir;
	PreferencesValue<Glib::ustring> gpshost;
	PreferencesValue<Glib::ustring> gpsport;

private:
	sqlite3x::sqlite3_connection db;

	static const std::string insert_command;
	static const std::string select_command;

	void savepv(const char *name, long long v);
	void savepv(const char *name, const std::string& v);
	void savepv(const char *name, double v);
	bool loadpv(const char *name, long long& v, long long dflt);
	bool loadpv(const char *name, std::string& v, const std::string& dflt);
	bool loadpv(const char *name, Glib::ustring& v, const Glib::ustring& dflt);
	bool loadpv(const char *name, double& v, double dflt);
	template<class T> bool load(T& t, const typename T::loadtype_t& dflt);
	template<class T> void save(T& t);
};

class Engine {
public:
	class DbObject {
	public:
		static const Glib::ustring empty;
		typedef Glib::RefPtr<DbObject> ptr_t;
		typedef Glib::RefPtr<const DbObject> const_ptr_t;
		DbObject(void);
		virtual ~DbObject();
		void reference(void) const;
		void unreference(void) const;

		virtual Point get_coord(void) const { return Point::invalid; }
		virtual const Glib::ustring& get_ident(void) const { return empty; }
		virtual const Glib::ustring& get_icao(void) const { return empty; }
		virtual const Glib::ustring& get_name(void) const { return empty; }

		virtual bool is_airport(void) const { return false; }
		virtual bool is_navaid(void) const { return false; }
		virtual bool is_intersection(void) const { return false; }
		virtual bool is_mapelement(void) const { return false; }
		virtual bool is_fplwaypoint(void) const { return false; }

		virtual bool get(AirportsDb::Airport& el) const { return false; }
		virtual bool get(NavaidsDb::Navaid& el) const { return false; }
		virtual bool get(WaypointsDb::Waypoint& el) const { return false; }
		virtual bool get(MapelementsDb::Mapelement& el) const { return false; }
		virtual bool get(FPlanWaypoint& el) const { return false; }

		virtual bool set(FPlanWaypoint& el) const = 0;
		virtual unsigned int insert(FPlanRoute& route, uint32_t nr = ~0U) const = 0;

		static Glib::RefPtr<DbObject> create(const AirportsDb::Airport& el);
		static Glib::RefPtr<DbObject> create(const NavaidsDb::Navaid& el);
		static Glib::RefPtr<DbObject> create(const WaypointsDb::Waypoint& el);
		static Glib::RefPtr<DbObject> create(const MapelementsDb::Mapelement& el);
		static Glib::RefPtr<DbObject> create(const FPlanWaypoint& el);

		template<class Archive> static void hibernate_binary(ptr_t& p, Archive& ar) {
			uint8_t t(0);
			if (ar.is_save()) {
				if (p) {
					{
						AirportsDb::Airport el;
						if (p->get(el)) {
							t = 1;
							ar.io(t);
							el.hibernate_binary(ar);
							return;
						}
					}
					{
						NavaidsDb::Navaid el;
						if (p->get(el)) {
							t = 2;
							ar.io(t);
							el.hibernate_binary(ar);
							return;
						}
					}
					{
						WaypointsDb::Waypoint el;
						if (p->get(el)) {
							t = 3;
							ar.io(t);
							el.hibernate_binary(ar);
							return;
						}
					}
					{
						MapelementsDb::Mapelement el;
						if (p->get(el)) {
							t = 4;
							ar.io(t);
							el.hibernate_binary(ar);
							return;
						}
					}
					{
						FPlanWaypoint el;
						if (p->get(el)) {
							t = 5;
							ar.io(t);
							el.hibernate_binary(ar);
							return;
						}
					}
				}
				ar.io(t);
				return;
			}
			if (ar.is_load()) {
				p = ptr_t();
				ar.io(t);
				switch (t) {
				case 1:
				{
					AirportsDb::Airport el;
					el.hibernate_binary(ar);
					p = create(el);
					return;
				}

				case 2:
				{
					NavaidsDb::Navaid el;
					el.hibernate_binary(ar);
					p = create(el);
					return;
				}

				case 3:
				{
					WaypointsDb::Waypoint el;
					el.hibernate_binary(ar);
					p = create(el);
					return;
				}

				case 4:
				{
					MapelementsDb::Mapelement el;
					el.hibernate_binary(ar);
					p = create(el);
					return;
				}

				case 5:
				{
					FPlanWaypoint el;
					el.hibernate_binary(ar);
					p = create(el);
					return;
				}

				default:
					return;
				}
			}
		}

	protected:
		mutable gint m_refcount;

		static FPlanWaypoint insert_prepare(FPlanRoute& route, uint32_t nr);
	};

protected:
	class DbObjectAirport;
	class DbObjectNavaid;
	class DbObjectWaypoint;
	class DbObjectMapelement;
	class DbObjectFPlanWaypoint;

private:
	class DbThread {
	private:
		class ResultBase {
		public:
			ResultBase(const sigc::slot<void>& dbaction, const sigc::slot<void>& dbcancel = sigc::slot<void>());
			~ResultBase();

			bool is_done(void) const { return m_done; }
			bool is_error(void) const { return m_error; }
			sigc::connection connect(const sigc::signal<void>::slot_type& slot);
			void disconnect_all(void);
			void cancel(void);
			template<class T> static void cancel(Glib::RefPtr<T>& q);

			void reference(void) const;
			void unreference(void) const;

			void execute(void);
			void seterror(void);

		protected:
			mutable Glib::Mutex m_mutex;
			sigc::slot<void> m_dbcancel;
			sigc::slot<void> m_dbaction;
			sigc::signal<void> m_donecb;
			mutable gint m_refcount;
			bool m_done;
			bool m_error;
			bool m_executing;
		};

	public:
		template <class Db> class Result : public ResultBase {
		public:
			typedef typename Db::element_t element_t;
			typedef typename Db::elementvector_t elementvector_t;

			Result(const sigc::slot<elementvector_t>& dbaction, const sigc::slot<void>& dbcancel = sigc::slot<void>());
			~Result();
			const elementvector_t& get_result(void) const { return m_result; }
			elementvector_t& get_result(void) { return m_result; }

			void set_result(const elementvector_t& ev) { m_result = ev; }

		protected:
			elementvector_t m_result;
		};

		template <class Db> class ResultObject : public Result<Db> {
		public:
			typedef typename Result<Db>::element_t element_t;
			typedef typename Result<Db>::elementvector_t elementvector_t;
			typedef typename std::vector<DbObject::ptr_t> objects_t;

			ResultObject(const sigc::slot<elementvector_t>& dbaction, const sigc::slot<void>& dbcancel = sigc::slot<void>());
			using Result<Db>::get_result;
			using Result<Db>::set_result;

			objects_t get_objects(void) const;
		};

		typedef ResultObject<AirportsDb> AirportResult;
		typedef Result<AirspacesDb> AirspaceResult;
		typedef ResultObject<NavaidsDb> NavaidResult;
		typedef ResultObject<WaypointsDb> WaypointResult;
		typedef Result<AirwaysDb> AirwayResult;
		typedef ResultObject<MapelementsDb> MapelementResult;

		class ElevationResult : public ResultBase {
		public:
			typedef TopoDb30::elev_t elev_t;

			ElevationResult(const Point& pt, TopoDb30& topodb);
#ifdef HAVE_PILOTLINK
			ElevationResult(const Point& pt, PalmGtopo& topodb);
#endif
			~ElevationResult();
			elev_t get_result(void) const { return m_result; }

		protected:
			elev_t m_result;

			void execute(TopoDb30& topodb, const Point& pt);
#ifdef HAVE_PILOTLINK
			void execute_palm(PalmGtopo& topodb, const Point& pt);
#endif
		};

		class ElevationMinMaxResult : public ResultBase {
		public:
			typedef TopoDb30::elev_t elev_t;
			typedef TopoDb30::minmax_elev_t minmax_elev_t;

			ElevationMinMaxResult(const Rect& r, TopoDb30& topodb);
			ElevationMinMaxResult(const PolygonSimple& p, TopoDb30& topodb);
			ElevationMinMaxResult(const PolygonHole& p, TopoDb30& topodb);
			ElevationMinMaxResult(const MultiPolygonHole& p, TopoDb30& topodb);
#ifdef HAVE_PILOTLINK
			ElevationMinMaxResult(const Rect& r, PalmGtopo& topodb);
			ElevationMinMaxResult(const PolygonSimple& p, PalmGtopo& topodb);
			ElevationMinMaxResult(const PolygonHole& p, PalmGtopo& topodb);
			ElevationMinMaxResult(const MultiPolygonHole& p, PalmGtopo& topodb);
#endif
			~ElevationMinMaxResult();
			minmax_elev_t get_result(void) const { return m_result; }

		protected:
			minmax_elev_t m_result;

			void execute_rect(TopoDb30& topodb, const Rect& r);
			void execute_poly(TopoDb30& topodb, const PolygonSimple& p);
			void execute_polyhole(TopoDb30& topodb, const PolygonHole& p);
			void execute_multipolyhole(TopoDb30& topodb, const MultiPolygonHole& p);
#ifdef HAVE_PILOTLINK
			void execute_rect_palm(PalmGtopo& topodb, const Rect& r);
			void execute_poly_palm(PalmGtopo& topodb, const PolygonSimple& p);
			void execute_polyhole_palm(PalmGtopo& topodb, const PolygonHole& p);
			void execute_multipolyhole_palm(PalmGtopo& topodb, const MultiPolygonHole& p);
#endif
		};

		class ElevationProfileResult : public ResultBase {
		public:
			typedef TopoDb30::elev_t elev_t;
			typedef TopoDb30::ProfilePoint ProfilePoint;
			typedef TopoDb30::Profile Profile;

			ElevationProfileResult(const Point& p0, const Point& p1, double corridor_nmi, TopoDb30& topodb);
#ifdef HAVE_PILOTLINK
			ElevationProfileResult(const Point& p0, const Point& p1, double corridor_nmi, PalmGtopo& topodb);
#endif
			~ElevationProfileResult();
			Profile get_result(void) const { return m_result; }

#ifdef HAVE_PILOTLINK
			static Profile get_profile(PalmGtopo& topodb, const Point& p0, const Point& p1, double corridor_nmi);
#endif

		protected:
			Profile m_result;

			void execute(TopoDb30& topodb, const Point& p0, const Point& p1, double corridor_nmi);
#ifdef HAVE_PILOTLINK
			void execute_palm(PalmGtopo& topodb, const Point& p0, const Point& p1, double corridor_nmi);
#endif
		};

		class ElevationRouteProfileResult : public ResultBase {
		public:
			typedef TopoDb30::elev_t elev_t;
			typedef TopoDb30::RouteProfilePoint RouteProfilePoint;
			typedef TopoDb30::RouteProfile RouteProfile;

			ElevationRouteProfileResult(const FPlanRoute& fpl, double corridor_nmi, TopoDb30& topodb);
#ifdef HAVE_PILOTLINK
			ElevationRouteProfileResult(const FPlanRoute& fpl, double corridor_nmi, PalmGtopo& topodb);
#endif
			~ElevationRouteProfileResult();
			RouteProfile get_result(void) const { return m_result; }

#ifdef HAVE_PILOTLINK
			static RouteProfile get_profile(PalmGtopo& topodb, const FPlanRoute& fpl, double corridor_nmi);
#endif

		protected:
			RouteProfile m_result;

			void execute(TopoDb30& topodb, const FPlanRoute& fpl, double corridor_nmi);
#ifdef HAVE_PILOTLINK
			void execute_palm(PalmGtopo& topodb, const FPlanRoute& fpl, double corridor_nmi);
#endif
		};

		class ElevationMapResult : public ResultBase {
		public:
			ElevationMapResult(const Rect& bbox, TopoDb30& topodb);
#ifdef HAVE_PILOTLINK
			ElevationMapResult(const Rect& bbox, PalmGtopo& topodb);
#endif
			~ElevationMapResult();
			Rect get_bbox(void) const { return m_bbox; }
			unsigned int get_width(void) const { return m_width; }
			unsigned int get_height(void) const { return m_height; }
			TopoDb30::elev_t operator()(unsigned int x, unsigned int y);
			//Glib::RefPtr<Gdk::Drawable> get_pixmap(void) const { return m_pixmap; }

		protected:
			Rect m_bbox;
			//Glib::RefPtr<Gdk::Drawable> m_pixmap;
			unsigned int m_width;
			unsigned int m_height;
			TopoDb30::elev_t *m_elev;

			void execute(TopoDb30& topodb);
#ifdef HAVE_PILOTLINK
			void execute_palm(PalmGtopo& topodb);
#endif
		};

		class ElevationMapCairoResult : public ResultBase {
		public:
			ElevationMapCairoResult(const Rect& bbox, TopoDb30& topodb);
#ifdef HAVE_PILOTLINK
			ElevationMapCairoResult(const Rect& bbox, PalmGtopo& topodb);
#endif
			~ElevationMapCairoResult();
			Rect get_bbox(void) const { return m_bbox; }
			Cairo::RefPtr<Cairo::ImageSurface> get_surface(void) const { return m_surface; }

		protected:
			Rect m_bbox;
			Cairo::RefPtr<Cairo::ImageSurface> m_surface;

			void execute(TopoDb30& topodb);
#ifdef HAVE_PILOTLINK
			void execute_palm(PalmGtopo& topodb);
#endif
		};

#ifdef HAVE_BOOST
		class AirwayGraphResult : public ResultBase {
		public:
			typedef AirwaysDb::element_t element_t;
			typedef AirwaysDb::elementvector_t elementvector_t;

			class Graph;

			class Edge {
			public:
				Edge(const Glib::ustring& ident = "", int16_t blevel = 0, int16_t tlevel = 0,
				     element_t::elev_t telev = element_t::nodata, element_t::elev_t c5elev = element_t::nodata,
				     element_t::airway_type_t typ = element_t::airway_invalid,
				     double dist = std::numeric_limits<double>::quiet_NaN());
				const Glib::ustring& get_ident(void) const { return m_ident; }
				double get_dist(void) const { return m_dist; }
				double get_metric(void) const { return m_metric; }
				void set_metric(double m) { m_metric = m; }
				int16_t get_base_level(void) const { return m_baselevel; }
				int16_t get_top_level(void) const { return m_toplevel; }
				element_t::elev_t get_terrain_elev(void) const { return m_terrainelev; }
				element_t::elev_t get_corridor5_elev(void) const { return m_corridor5elev; }
				element_t::airway_type_t get_type(void) const { return m_type; }
				// no altitude intervals, for now
				bool is_level_ok(int16_t lvl) const { return lvl >= m_baselevel && lvl <= m_toplevel; }
				void set_base_level(int16_t b) { m_baselevel = b; }
				void set_top_level(int16_t t) { m_toplevel = t; }

			protected:
				Glib::ustring m_ident;
			public:
				double m_dist;
				double m_metric;
			protected:
				int16_t m_baselevel;
				int16_t m_toplevel;
				element_t::elev_t m_terrainelev;
				element_t::elev_t m_corridor5elev;
 				element_t::airway_type_t m_type;
			};

			class Vertex {
			public:
				typedef enum {
					type_airport = FPlanWaypoint::type_airport,
					type_navaid = FPlanWaypoint::type_navaid,
					type_intersection = FPlanWaypoint::type_intersection,
					type_mapelement = FPlanWaypoint::type_mapelement,
					type_undefined = FPlanWaypoint::type_undefined,
					type_fplwaypoint = FPlanWaypoint::type_fplwaypoint,
					type_vfrreportingpt = FPlanWaypoint::type_vfrreportingpt
				} type_t;

				typedef enum {
					typemask_none = 0U,
					typemask_airport = 1U << type_airport,
					typemask_navaid = 1U << type_navaid,
					typemask_intersection = 1U << type_intersection,
					typemask_mapelement = 1U << type_mapelement,
					typemask_undefined = 1U << type_undefined,
					typemask_airway = 1U << type_undefined,
					typemask_fplwaypoint = 1U << type_fplwaypoint,
					typemask_vfrreportingpt = 1U << type_vfrreportingpt,
					typemask_awyendpoints = (1U << type_navaid) | (1U << type_intersection) | (1U << type_undefined),
					typemask_all = (1U << type_airport) | (1U << type_navaid) | (1U << type_intersection) | (1U << type_mapelement) | (1U << type_undefined) | (1U << type_vfrreportingpt)
				} typemask_t;

				Vertex(const Glib::ustring& ident = "", const Point& pt = Point(), type_t typ = type_undefined)
					: m_ident(ident), m_coord(pt), m_type(typ) {}
				Vertex(const AirportsDb::Airport& el);
				Vertex(const NavaidsDb::Navaid& el);
				Vertex(const WaypointsDb::Waypoint& el);
				Vertex(const MapelementsDb::Mapelement& el);
				Vertex(const FPlanWaypoint& el);

				// Object Stutff
				Engine::DbObject::ptr_t get_object(void) const { return m_obj; }
				void clear_object(void) { Engine::DbObject::ptr_t p; m_obj.swap(p); }

				bool is_airport(void) const;
				bool is_navaid(void) const;
				bool is_intersection(void) const;
				bool is_mapelement(void) const;
				bool is_fplwaypoint(void) const;

				bool get(AirportsDb::Airport& el) const;
				bool get(NavaidsDb::Navaid& el) const;
				bool get(WaypointsDb::Waypoint& el) const;
				bool get(MapelementsDb::Mapelement& el) const;
				bool get(FPlanWaypoint& el) const;

				bool set(FPlanWaypoint& el) const;
				unsigned int insert(FPlanRoute& route, uint32_t nr = ~0U) const;

				// Cached data
				const Glib::ustring& get_ident(void) const { return m_ident; }
				const Point& get_coord(void) const { return m_coord; }
				type_t get_type(void) const { return m_type; }
				bool is_match(typemask_t mask = typemask_all) const { return !!(mask & (1U << m_type)); }
				static bool is_ident_numeric(const Glib::ustring& id);
				static bool is_ident_numeric(const std::string& id);
				bool is_ident_numeric(void) const { return is_ident_numeric(m_ident); }
				bool is_ifr_fpl(void) const;

				static type_t from_type_string(const std::string& x);
				static const std::string& get_type_string(type_t typ) {
					return FPlanWaypoint::get_type_string((FPlanWaypoint::type_t)typ);
				}
				const std::string& get_type_string(void) const { return get_type_string(get_type()); }

			protected:
				Engine::DbObject::ptr_t m_obj;
				Glib::ustring m_ident;
				Point m_coord;
				type_t m_type;
				friend class Graph;
			};

			class Graph : public boost::adjacency_list<boost::multisetS, boost::vecS, boost::bidirectionalS, Vertex, Edge> {
			public:
				typedef boost::vertex_bundle_type<Graph>::type Vertex;
				typedef boost::edge_bundle_type<Graph>::type Edge;
				typedef AirwaysDb::element_t element_t;
				typedef AirwaysDb::elementvector_t elementvector_t;

				void clear(void);
				vertex_descriptor add(const Vertex& vn);
				std::pair<vertex_descriptor,vertex_descriptor> add(const element_t& el);
				template <typename T> void add(T b, T e) { for (; b != e; ++b) add(*b); }
				void add(const elementvector_t& ev);
				vertex_descriptor add(const AirportsDb::element_t& el);
				void add(const AirportsDb::elementvector_t& ev);
				vertex_descriptor add(const NavaidsDb::element_t& el);
				void add(const NavaidsDb::elementvector_t& ev);
				vertex_descriptor add(const WaypointsDb::element_t& el);
				void add(const WaypointsDb::elementvector_t& ev);
				vertex_descriptor add(const MapelementsDb::element_t& el);
				void add(const MapelementsDb::elementvector_t& ev);
				bool find_intersection(vertex_descriptor& u, const Glib::ustring& name, Vertex::typemask_t tmask = Vertex::typemask_all,
						       bool require_outedge = false, bool require_inedge = false);
				bool find_intersection(vertex_descriptor& u, const Glib::ustring& name, const Point& pt, Vertex::typemask_t tmask = Vertex::typemask_all,
						       bool require_outedge = false, bool require_inedge = false);
				bool find_intersection(vertex_descriptor& u, const Glib::ustring& name, const Glib::ustring& awy, Vertex::typemask_t tmask = Vertex::typemask_all,
						       bool require_outedge = false, bool require_inedge = false);
				typedef std::set<vertex_descriptor> vertex_descriptor_set_t;
				vertex_descriptor_set_t find_airway_terminals(const Glib::ustring& awyname);
				vertex_descriptor_set_t find_airway_begin(const Glib::ustring& awyname);
				vertex_descriptor_set_t find_airway_end(const Glib::ustring& awyname);
				vertex_descriptor_set_t find_airway_vertices(const Glib::ustring& awyname);
				vertex_descriptor_set_t find_intersections(const Glib::ustring& name);
				bool find_nearest(vertex_descriptor& u, const Point& pt, Vertex::typemask_t tmask = Vertex::typemask_all,
						  bool require_outedge = false, bool require_inedge = false);
				bool find_nearest(vertex_descriptor& u, const Point& pt, const Glib::ustring& awyname, Vertex::typemask_t tmask = Vertex::typemask_all,
						  bool require_outedge = false, bool require_inedge = false);
				bool find_edge(edge_descriptor& u, const Glib::ustring& beginname, const Glib::ustring& endname);
				bool find_edge(edge_descriptor& u, const Glib::ustring& beginname, const Glib::ustring& endname, const Glib::ustring& awyname);
				void exclude_levels(edge_descriptor e, int16_t fromlevel, int16_t tolevel);
				void shortest_paths(vertex_descriptor u, std::vector<vertex_descriptor>& predecessors);
				void shortest_paths(vertex_descriptor u, std::vector<vertex_descriptor>& predecessors, std::vector<double>& distances);
				void shortest_paths(vertex_descriptor u, const Glib::ustring& awyname, std::vector<vertex_descriptor>& predecessors);
				void shortest_paths(vertex_descriptor u, const Glib::ustring& awyname, std::vector<vertex_descriptor>& predecessors, std::vector<double>& distances);
				void shortest_paths_metric(vertex_descriptor u, std::vector<vertex_descriptor>& predecessors);
				void shortest_paths_metric(vertex_descriptor u, std::vector<vertex_descriptor>& predecessors, std::vector<double>& distances);
				void shortest_paths_metric(vertex_descriptor u, const Glib::ustring& awyname, std::vector<vertex_descriptor>& predecessors);
				void shortest_paths_metric(vertex_descriptor u, const Glib::ustring& awyname, std::vector<vertex_descriptor>& predecessors, std::vector<double>& distances);
				unsigned int add_dct_edges(vertex_descriptor u, double maxdist = 20, bool efrom = true, bool eto = true,
							   Vertex::typemask_t tmask = Vertex::typemask_all, int16_t baselevel = 0, int16_t toplevel = 600,
							   bool skip_numeric_int = false);
				unsigned int add_dct_edges(double maxdist = 50, Vertex::typemask_t tmask = Vertex::typemask_all, int16_t baselevel = 0, int16_t toplevel = 600,
							   bool skip_numeric_int = false);
				void delete_disconnected_vertices(void);
				void clear_objects(void);

				class AwyEdgeFilter {
				public:
					AwyEdgeFilter(void) : m_graph(0) {}
					AwyEdgeFilter(Graph& g, const Glib::ustring& awyname) : m_graph(&g), m_awyname(awyname.uppercase()) {}
					template <typename E> bool operator()(const E& e) const { return (*m_graph)[e].get_ident() == m_awyname; }

				protected:
					Graph *m_graph;
					Glib::ustring m_awyname;
				};

			protected:
				static constexpr double intersection_tolerance = 0.1;
				static constexpr double navaid_tolerance = 0.1;
				typedef boost::adjacency_list<boost::multisetS, boost::vecS, boost::bidirectionalS, Vertex, Edge> boostgraph_t;
				typedef std::multimap<Glib::ustring,vertex_descriptor> intersectionmap_t;
				intersectionmap_t m_intersectionmap;
			};

			AirwayGraphResult(const sigc::slot<elementvector_t>& dbaction, const sigc::slot<void>& dbcancel = sigc::slot<void>());
			~AirwayGraphResult();
			const Graph& get_result(void) const { return m_graph; }
			Graph& get_result(void) { return m_graph; }

			void set_result(const elementvector_t& ev);
			void set_result_airports(const AirportsDb::elementvector_t& ev);
			void set_result_navaids(const NavaidsDb::elementvector_t& ev);
			void set_result_waypoints(const WaypointsDb::elementvector_t& ev);
			void set_result_mapelements(const MapelementsDb::elementvector_t& ev);

		protected:
			Graph m_graph;
		};

		class AreaGraphResult : public AirwayGraphResult {
		public:
			AreaGraphResult(const Rect& rect, bool area = false, AirwaysDbQueryInterface *airwaydb = 0,
					AirportsDbQueryInterface *airportdb = 0, NavaidsDbQueryInterface *navaiddb = 0,
					WaypointsDbQueryInterface *waypointdb = 0, MapelementsDbQueryInterface *mapelementdb = 0,
					const sigc::slot<void>& dbcancel = sigc::slot<void>());

		protected:
			Rect m_rect;
			AirwaysDbQueryInterface *m_airwaydb;
			AirportsDbQueryInterface *m_airportdb;
			NavaidsDbQueryInterface *m_navaiddb;
			WaypointsDbQueryInterface *m_waypointdb;
			MapelementsDbQueryInterface *m_mapelementdb;
			bool m_area;

			elementvector_t dbaction(void);
		};
#endif

		typedef ResultBase AirportSaveResult;
		typedef ResultBase AirspaceSaveResult;
		typedef ResultBase NavaidSaveResult;
		typedef ResultBase WaypointSaveResult;
		typedef ResultBase AirwaySaveResult;
		typedef ResultBase MapelementSaveResult;

		DbThread(const std::string& dir_main = "", const std::string& dir_aux = "", bool pg = false);
		~DbThread();

		Glib::RefPtr<AirportResult> airport_find_by_icao(const Glib::ustring& nm = "", unsigned int limit = ~0, unsigned int skip = 0, AirportsDb::comp_t comp = AirportsDb::comp_contains, unsigned int loadsubtables = ~0);
		Glib::RefPtr<AirportResult> airport_find_by_name(const Glib::ustring& nm = "", unsigned int limit = ~0, unsigned int skip = 0, AirportsDb::comp_t comp = AirportsDb::comp_contains, unsigned int loadsubtables = ~0);
		Glib::RefPtr<AirportResult> airport_find_by_text(const Glib::ustring& nm = "", unsigned int limit = ~0, unsigned int skip = 0, AirportsDb::comp_t comp = AirportsDb::comp_contains, unsigned int loadsubtables = ~0);
		Glib::RefPtr<AirportResult> airport_find_bbox(const Rect& rect, unsigned int limit = ~0, unsigned int loadsubtables = ~0);
		Glib::RefPtr<AirportResult> airport_find_nearest(const Point& pt, unsigned int limit = ~0, const Rect& rect = Rect(), unsigned int loadsubtables = ~0);
		Glib::RefPtr<AirportSaveResult> airport_save(const AirportsDb::Airport& e);

		Glib::RefPtr<AirspaceResult> airspace_find_by_icao(const Glib::ustring& nm = "", unsigned int limit = ~0, unsigned int skip = 0, AirspacesDb::comp_t comp = AirspacesDb::comp_contains, unsigned int loadsubtables = ~0);
		Glib::RefPtr<AirspaceResult> airspace_find_by_name(const Glib::ustring& nm = "", unsigned int limit = ~0, unsigned int skip = 0, AirspacesDb::comp_t comp = AirspacesDb::comp_contains, unsigned int loadsubtables = ~0);
		Glib::RefPtr<AirspaceResult> airspace_find_by_text(const Glib::ustring& nm = "", unsigned int limit = ~0, unsigned int skip = 0, AirspacesDb::comp_t comp = AirspacesDb::comp_contains, unsigned int loadsubtables = ~0);
		Glib::RefPtr<AirspaceResult> airspace_find_bbox(const Rect& rect, unsigned int limit = ~0, unsigned int loadsubtables = ~0);
		Glib::RefPtr<AirspaceResult> airspace_find_nearest(const Point& pt, unsigned int limit = ~0, const Rect& rect = Rect(), unsigned int loadsubtables = ~0);
		Glib::RefPtr<AirspaceSaveResult> airspace_save(const AirspacesDb::Airspace& e);

		Glib::RefPtr<NavaidResult> navaid_find_by_icao(const Glib::ustring& nm = "", unsigned int limit = ~0, unsigned int skip = 0, NavaidsDb::comp_t comp = NavaidsDb::comp_contains, unsigned int loadsubtables = ~0);
		Glib::RefPtr<NavaidResult> navaid_find_by_name(const Glib::ustring& nm = "", unsigned int limit = ~0, unsigned int skip = 0, NavaidsDb::comp_t comp = NavaidsDb::comp_contains, unsigned int loadsubtables = ~0);
		Glib::RefPtr<NavaidResult> navaid_find_by_text(const Glib::ustring& nm = "", unsigned int limit = ~0, unsigned int skip = 0, NavaidsDb::comp_t comp = NavaidsDb::comp_contains, unsigned int loadsubtables = ~0);
		Glib::RefPtr<NavaidResult> navaid_find_bbox(const Rect& rect, unsigned int limit = ~0, unsigned int loadsubtables = ~0);
		Glib::RefPtr<NavaidResult> navaid_find_nearest(const Point& pt, unsigned int limit = ~0, const Rect& rect = Rect(), unsigned int loadsubtables = ~0);
		Glib::RefPtr<NavaidSaveResult> navaid_save(const NavaidsDb::Navaid& e);

		Glib::RefPtr<WaypointResult> waypoint_find_by_name(const Glib::ustring& nm = "", unsigned int limit = ~0, unsigned int skip = 0, WaypointsDb::comp_t comp = WaypointsDb::comp_contains, unsigned int loadsubtables = ~0);
		Glib::RefPtr<WaypointResult> waypoint_find_bbox(const Rect& rect, unsigned int limit = ~0, unsigned int loadsubtables = ~0);
		Glib::RefPtr<WaypointResult> waypoint_find_nearest(const Point& pt, unsigned int limit = ~0, const Rect& rect = Rect(), unsigned int loadsubtables = ~0);
		Glib::RefPtr<WaypointSaveResult> waypoint_save(const WaypointsDb::Waypoint& e);

		Glib::RefPtr<AirwayResult> airway_find_by_name(const Glib::ustring& nm = "", unsigned int limit = ~0, unsigned int skip = 0, AirwaysDb::comp_t comp = AirwaysDb::comp_contains, unsigned int loadsubtables = ~0);
		Glib::RefPtr<AirwayResult> airway_find_by_text(const Glib::ustring& nm = "", unsigned int limit = ~0, unsigned int skip = 0, AirwaysDb::comp_t comp = AirwaysDb::comp_contains, unsigned int loadsubtables = ~0);
		Glib::RefPtr<AirwayResult> airway_find_bbox(const Rect& rect, unsigned int limit = ~0, unsigned int loadsubtables = ~0);
		Glib::RefPtr<AirwayResult> airway_find_area(const Rect& rect, unsigned int limit = ~0, unsigned int loadsubtables = ~0);
		Glib::RefPtr<AirwayResult> airway_find_nearest(const Point& pt, unsigned int limit = ~0, const Rect& rect = Rect(), unsigned int loadsubtables = ~0);
		Glib::RefPtr<AirwaySaveResult> airway_save(const AirwaysDb::Airway& e);

#ifdef HAVE_BOOST
		Glib::RefPtr<AirwayGraphResult> airway_graph_find_by_name(const Glib::ustring& nm = "", unsigned int limit = ~0, unsigned int skip = 0, AirwaysDb::comp_t comp = AirwaysDb::comp_contains, unsigned int loadsubtables = ~0);
		Glib::RefPtr<AirwayGraphResult> airway_graph_find_by_text(const Glib::ustring& nm = "", unsigned int limit = ~0, unsigned int skip = 0, AirwaysDb::comp_t comp = AirwaysDb::comp_contains, unsigned int loadsubtables = ~0);
		Glib::RefPtr<AirwayGraphResult> airway_graph_find_bbox(const Rect& rect, unsigned int limit = ~0, unsigned int loadsubtables = ~0);
		Glib::RefPtr<AirwayGraphResult> airway_graph_find_area(const Rect& rect, unsigned int limit = ~0, unsigned int loadsubtables = ~0);
		Glib::RefPtr<AirwayGraphResult> airway_graph_find_nearest(const Point& pt, unsigned int limit = ~0, const Rect& rect = Rect(), unsigned int loadsubtables = ~0);

		Glib::RefPtr<AreaGraphResult> area_graph_find_bbox(const Rect& rect, AreaGraphResult::Vertex::typemask_t tmask = AreaGraphResult::Vertex::typemask_all);
		Glib::RefPtr<AreaGraphResult> area_graph_find_area(const Rect& rect, AreaGraphResult::Vertex::typemask_t tmask = AreaGraphResult::Vertex::typemask_all);
#endif

		Glib::RefPtr<MapelementResult> mapelement_find_by_name(const Glib::ustring& nm = "", unsigned int limit = ~0, unsigned int skip = 0, MapelementsDb::comp_t comp = MapelementsDb::comp_contains, unsigned int loadsubtables = ~0);
		Glib::RefPtr<MapelementResult> mapelement_find_bbox(const Rect& rect, unsigned int limit = ~0, unsigned int loadsubtables = ~0);
		Glib::RefPtr<MapelementResult> mapelement_find_nearest(const Point& pt, unsigned int limit = ~0, const Rect& rect = Rect(), unsigned int loadsubtables = ~0);
		Glib::RefPtr<MapelementSaveResult> mapelement_save(const MapelementsDb::Mapelement& e);

		Glib::RefPtr<ElevationResult> elevation_point(const Point& pt);
		Glib::RefPtr<ElevationMapResult> elevation_map(const Rect& bbox);
		Glib::RefPtr<ElevationMapCairoResult> elevation_map_cairo(const Rect& bbox);
		Glib::RefPtr<ElevationMinMaxResult> elevation_minmax(const Rect& r);
		Glib::RefPtr<ElevationMinMaxResult> elevation_minmax(const PolygonSimple& p);
		Glib::RefPtr<ElevationMinMaxResult> elevation_minmax(const PolygonHole& p);
		Glib::RefPtr<ElevationMinMaxResult> elevation_minmax(const MultiPolygonHole& p);
		Glib::RefPtr<ElevationProfileResult> elevation_profile(const Point& p0, const Point& p1, double corridor_width = 5);
		Glib::RefPtr<ElevationRouteProfileResult> elevation_profile(const FPlanRoute& fpl, double corridor_width = 5);

	private:
		std::unique_ptr<AirportsDbQueryInterface> m_airportdb;
		std::unique_ptr<NavaidsDbQueryInterface> m_navaiddb;
		std::unique_ptr<WaypointsDbQueryInterface> m_waypointdb;
		std::unique_ptr<AirwaysDbQueryInterface> m_airwaydb;
		std::unique_ptr<AirspacesDbQueryInterface> m_airspacedb;
		std::unique_ptr<MapelementsDbQueryInterface> m_mapelementdb;
		TopoDb30 m_topodb;
#ifdef HAVE_PQXX
		typedef std::unique_ptr<pqxx::lazyconnection> pgconn_t;
		pgconn_t m_pgconn;
#endif
#ifdef HAVE_PILOTLINK
		PalmAirports m_palmairportdb;
		PalmNavaids m_palmnavaiddb;
		PalmWaypoints m_palmwaypointdb;
		PalmAirspaces m_palmairspacedb;
		PalmMapelements m_palmmapelementdb;
		PalmGtopo m_palmgtopodb;
#endif
		bool m_terminate;
		Glib::Mutex m_mutex;
		Glib::Cond m_cond;
		Glib::Thread *m_thread;
		std::list<Glib::RefPtr<ResultBase> > m_tasklist;

		void thread(void);
		template<class T> void queue(Glib::RefPtr<T>& p);
	};

public:
	typedef enum {
		auxdb_none = 0,
		auxdb_prefs = 1,
		auxdb_override = 2,
		auxdb_postgres = 3
	} auxdb_mode_t;

	Engine(const std::string& dir_main = "", auxdb_mode_t auxdbmode = auxdb_prefs, const std::string& dir_aux = "", bool loadgrib2 = false, bool loadbitmapmaps = false);

	Preferences& get_prefs(void) { return m_prefs; }
	FPlan& get_fplan_db(void) { return m_fplandb; }
	TracksDb& get_tracks_db(void) { return m_tracksdb; }
	GRIB2& get_grib2_db(void) { return m_grib2; }
	BitmapMaps& get_bitmapmaps_db(void) { return m_bitmapmaps; }

	const std::string& get_dir_main(void) const { return m_dir_main; }
	const std::string& get_dir_aux(void) const { return m_dir_aux; }

	void update_bitmapmaps(bool synchronous);

public:
	// Asynchronous interface
	typedef DbThread::AirportResult AirportResult;
	typedef DbThread::AirspaceResult AirspaceResult;
	typedef DbThread::NavaidResult NavaidResult;
	typedef DbThread::WaypointResult WaypointResult;
	typedef DbThread::AirwayResult AirwayResult;
#ifdef HAVE_BOOST
	typedef DbThread::AirwayGraphResult AirwayGraphResult;
	typedef DbThread::AreaGraphResult AreaGraphResult;
#endif
	typedef DbThread::MapelementResult MapelementResult;
	typedef DbThread::AirportSaveResult AirportSaveResult;
	typedef DbThread::AirspaceSaveResult AirspaceSaveResult;
	typedef DbThread::NavaidSaveResult NavaidSaveResult;
	typedef DbThread::WaypointSaveResult WaypointSaveResult;
	typedef DbThread::AirwaySaveResult AirwaySaveResult;
	typedef DbThread::MapelementSaveResult MapelementSaveResult;
	typedef DbThread::ElevationResult ElevationResult;
	typedef DbThread::ElevationMinMaxResult ElevationMinMaxResult;
	typedef DbThread::ElevationProfileResult ElevationProfileResult;
	typedef DbThread::ElevationRouteProfileResult ElevationRouteProfileResult;
	typedef DbThread::ElevationMapResult ElevationMapResult;
	typedef DbThread::ElevationMapCairoResult ElevationMapCairoResult;

	Glib::RefPtr<AirportResult> async_airport_find_by_icao(const Glib::ustring& nm = "", unsigned int limit = ~0, unsigned int skip = 0, AirportsDb::comp_t comp = AirportsDb::comp_contains, unsigned int loadsubtables = ~0) { return m_dbthread.airport_find_by_icao(nm, limit, skip, comp, loadsubtables); }
	Glib::RefPtr<AirportResult> async_airport_find_by_name(const Glib::ustring& nm = "", unsigned int limit = ~0, unsigned int skip = 0, AirportsDb::comp_t comp = AirportsDb::comp_contains, unsigned int loadsubtables = ~0) { return m_dbthread.airport_find_by_name(nm, limit, skip, comp, loadsubtables); }
	Glib::RefPtr<AirportResult> async_airport_find_by_text(const Glib::ustring& nm = "", unsigned int limit = ~0, unsigned int skip = 0, AirportsDb::comp_t comp = AirportsDb::comp_contains, unsigned int loadsubtables = ~0) { return m_dbthread.airport_find_by_text(nm, limit, skip, comp, loadsubtables); }
	Glib::RefPtr<AirportResult> async_airport_find_bbox(const Rect& rect, unsigned int limit = ~0, unsigned int loadsubtables = ~0) { return m_dbthread.airport_find_bbox(rect, limit, loadsubtables); }
	Glib::RefPtr<AirportResult> async_airport_find_nearest(const Point& pt, unsigned int limit = ~0, const Rect& rect = Rect(), unsigned int loadsubtables = ~0) { return m_dbthread.airport_find_nearest(pt, limit, rect, loadsubtables); }
	Glib::RefPtr<AirportSaveResult> async_airport_save(const AirportsDb::Airport& e) { return m_dbthread.airport_save(e); }

	Glib::RefPtr<AirspaceResult> async_airspace_find_by_icao(const Glib::ustring& nm = "", unsigned int limit = ~0, unsigned int skip = 0, AirspacesDb::comp_t comp = AirspacesDb::comp_contains, unsigned int loadsubtables = ~0) { return m_dbthread.airspace_find_by_icao(nm, limit, skip, comp, loadsubtables); }
	Glib::RefPtr<AirspaceResult> async_airspace_find_by_name(const Glib::ustring& nm = "", unsigned int limit = ~0, unsigned int skip = 0, AirspacesDb::comp_t comp = AirspacesDb::comp_contains, unsigned int loadsubtables = ~0) { return m_dbthread.airspace_find_by_name(nm, limit, skip, comp, loadsubtables); }
	Glib::RefPtr<AirspaceResult> async_airspace_find_by_text(const Glib::ustring& nm = "", unsigned int limit = ~0, unsigned int skip = 0, AirspacesDb::comp_t comp = AirspacesDb::comp_contains, unsigned int loadsubtables = ~0) { return m_dbthread.airspace_find_by_text(nm, limit, skip, comp, loadsubtables); }
	Glib::RefPtr<AirspaceResult> async_airspace_find_bbox(const Rect& rect, unsigned int limit = ~0, unsigned int loadsubtables = ~0) { return m_dbthread.airspace_find_bbox(rect, limit, loadsubtables); }
	Glib::RefPtr<AirspaceResult> async_airspace_find_nearest(const Point& pt, unsigned int limit = ~0, const Rect& rect = Rect(), unsigned int loadsubtables = ~0) { return m_dbthread.airspace_find_nearest(pt, limit, rect, loadsubtables); }
	Glib::RefPtr<AirspaceSaveResult> async_airspace_save(const AirspacesDb::Airspace& e) { return m_dbthread.airspace_save(e); }

	Glib::RefPtr<NavaidResult> async_navaid_find_by_icao(const Glib::ustring& nm = "", unsigned int limit = ~0, unsigned int skip = 0, NavaidsDb::comp_t comp = NavaidsDb::comp_contains, unsigned int loadsubtables = ~0) { return m_dbthread.navaid_find_by_icao(nm, limit, skip, comp, loadsubtables); }
	Glib::RefPtr<NavaidResult> async_navaid_find_by_name(const Glib::ustring& nm = "", unsigned int limit = ~0, unsigned int skip = 0, NavaidsDb::comp_t comp = NavaidsDb::comp_contains, unsigned int loadsubtables = ~0) { return m_dbthread.navaid_find_by_name(nm, limit, skip, comp, loadsubtables); }
	Glib::RefPtr<NavaidResult> async_navaid_find_by_text(const Glib::ustring& nm = "", unsigned int limit = ~0, unsigned int skip = 0, NavaidsDb::comp_t comp = NavaidsDb::comp_contains, unsigned int loadsubtables = ~0) { return m_dbthread.navaid_find_by_text(nm, limit, skip, comp, loadsubtables); }
	Glib::RefPtr<NavaidResult> async_navaid_find_bbox(const Rect& rect, unsigned int limit = ~0, unsigned int loadsubtables = ~0) { return m_dbthread.navaid_find_bbox(rect, limit, loadsubtables); }
	Glib::RefPtr<NavaidResult> async_navaid_find_nearest(const Point& pt, unsigned int limit = ~0, const Rect& rect = Rect(), unsigned int loadsubtables = ~0) { return m_dbthread.navaid_find_nearest(pt, limit, rect, loadsubtables); }
	Glib::RefPtr<NavaidSaveResult> async_navaid_save(const NavaidsDb::Navaid& e) { return m_dbthread.navaid_save(e); }

	Glib::RefPtr<WaypointResult> async_waypoint_find_by_name(const Glib::ustring& nm = "", unsigned int limit = ~0, unsigned int skip = 0, WaypointsDb::comp_t comp = WaypointsDb::comp_contains, unsigned int loadsubtables = ~0) { return m_dbthread.waypoint_find_by_name(nm, limit, skip, comp, loadsubtables); }
	Glib::RefPtr<WaypointResult> async_waypoint_find_bbox(const Rect& rect, unsigned int limit = ~0, unsigned int loadsubtables = ~0) { return m_dbthread.waypoint_find_bbox(rect, limit, loadsubtables); }
	Glib::RefPtr<WaypointResult> async_waypoint_find_nearest(const Point& pt, unsigned int limit = ~0, const Rect& rect = Rect(), unsigned int loadsubtables = ~0) { return m_dbthread.waypoint_find_nearest(pt, limit, rect, loadsubtables); }
	Glib::RefPtr<WaypointSaveResult> async_waypoint_save(const WaypointsDb::Waypoint& e) { return m_dbthread.waypoint_save(e); }

	Glib::RefPtr<AirwayResult> async_airway_find_by_name(const Glib::ustring& nm = "", unsigned int limit = ~0, unsigned int skip = 0, AirwaysDb::comp_t comp = AirwaysDb::comp_contains, unsigned int loadsubtables = ~0) { return m_dbthread.airway_find_by_name(nm, limit, skip, comp, loadsubtables); }
	Glib::RefPtr<AirwayResult> async_airway_find_by_text(const Glib::ustring& nm = "", unsigned int limit = ~0, unsigned int skip = 0, AirwaysDb::comp_t comp = AirwaysDb::comp_contains, unsigned int loadsubtables = ~0) { return m_dbthread.airway_find_by_text(nm, limit, skip, comp, loadsubtables); }
	Glib::RefPtr<AirwayResult> async_airway_find_bbox(const Rect& rect, unsigned int limit = ~0, unsigned int loadsubtables = ~0) { return m_dbthread.airway_find_bbox(rect, limit, loadsubtables); }
	Glib::RefPtr<AirwayResult> async_airway_find_area(const Rect& rect, unsigned int limit = ~0, unsigned int loadsubtables = ~0) { return m_dbthread.airway_find_area(rect, limit, loadsubtables); }
	Glib::RefPtr<AirwayResult> async_airway_find_nearest(const Point& pt, unsigned int limit = ~0, const Rect& rect = Rect(), unsigned int loadsubtables = ~0) { return m_dbthread.airway_find_nearest(pt, limit, rect, loadsubtables); }
	Glib::RefPtr<AirwaySaveResult> async_airway_save(const AirwaysDb::Airway& e) { return m_dbthread.airway_save(e); }

#ifdef HAVE_BOOST
	Glib::RefPtr<AirwayGraphResult> async_airway_graph_find_by_name(const Glib::ustring& nm = "", unsigned int limit = ~0, unsigned int skip = 0, AirwaysDb::comp_t comp = AirwaysDb::comp_contains, unsigned int loadsubtables = ~0) { return m_dbthread.airway_graph_find_by_name(nm, limit, skip, comp, loadsubtables); }
	Glib::RefPtr<AirwayGraphResult> async_airway_graph_find_by_text(const Glib::ustring& nm = "", unsigned int limit = ~0, unsigned int skip = 0, AirwaysDb::comp_t comp = AirwaysDb::comp_contains, unsigned int loadsubtables = ~0) { return m_dbthread.airway_graph_find_by_text(nm, limit, skip, comp, loadsubtables); }
	Glib::RefPtr<AirwayGraphResult> async_airway_graph_find_bbox(const Rect& rect, unsigned int limit = ~0, unsigned int loadsubtables = ~0) { return m_dbthread.airway_graph_find_bbox(rect, limit, loadsubtables); }
	Glib::RefPtr<AirwayGraphResult> async_airway_graph_find_area(const Rect& rect, unsigned int limit = ~0, unsigned int loadsubtables = ~0) { return m_dbthread.airway_graph_find_area(rect, limit, loadsubtables); }
	Glib::RefPtr<AirwayGraphResult> async_airway_graph_find_nearest(const Point& pt, unsigned int limit = ~0, const Rect& rect = Rect(), unsigned int loadsubtables = ~0) { return m_dbthread.airway_graph_find_nearest(pt, limit, rect, loadsubtables); }

	Glib::RefPtr<AreaGraphResult> async_area_graph_find_bbox(const Rect& rect, AreaGraphResult::Vertex::typemask_t tmask = AreaGraphResult::Vertex::typemask_all) { return m_dbthread.area_graph_find_bbox(rect, tmask); }
	Glib::RefPtr<AreaGraphResult> async_area_graph_find_area(const Rect& rect, AreaGraphResult::Vertex::typemask_t tmask = AreaGraphResult::Vertex::typemask_all) { return m_dbthread.area_graph_find_area(rect, tmask); }
#endif

	Glib::RefPtr<MapelementResult> async_mapelement_find_by_name(const Glib::ustring& nm = "", unsigned int limit = ~0, unsigned int skip = 0, MapelementsDb::comp_t comp = MapelementsDb::comp_contains, unsigned int loadsubtables = ~0) { return m_dbthread.mapelement_find_by_name(nm, limit, skip, comp, loadsubtables); }
	Glib::RefPtr<MapelementResult> async_mapelement_find_bbox(const Rect& rect, unsigned int limit = ~0, unsigned int loadsubtables = ~0) { return m_dbthread.mapelement_find_bbox(rect, limit, loadsubtables); }
	Glib::RefPtr<MapelementResult> async_mapelement_find_nearest(const Point& pt, unsigned int limit = ~0, const Rect& rect = Rect(), unsigned int loadsubtables = ~0) { return m_dbthread.mapelement_find_nearest(pt, limit, rect, loadsubtables); }
	Glib::RefPtr<MapelementSaveResult> async_mapelement_save(const MapelementsDb::Mapelement& e) { return m_dbthread.mapelement_save(e); }

	Glib::RefPtr<ElevationResult> async_elevation_point(const Point& pt) { return m_dbthread.elevation_point(pt); }
	Glib::RefPtr<ElevationMapResult> async_elevation_map(const Rect& bbox) { return m_dbthread.elevation_map(bbox); }
	Glib::RefPtr<ElevationMapCairoResult> async_elevation_map_cairo(const Rect& bbox) { return m_dbthread.elevation_map_cairo(bbox); }
	Glib::RefPtr<ElevationMinMaxResult> async_elevation_minmax(const Rect& r) { return m_dbthread.elevation_minmax(r); }
	Glib::RefPtr<ElevationMinMaxResult> async_elevation_minmax(const PolygonSimple& p) { return m_dbthread.elevation_minmax(p); }
	Glib::RefPtr<ElevationMinMaxResult> async_elevation_minmax(const PolygonHole& p) { return m_dbthread.elevation_minmax(p); }
	Glib::RefPtr<ElevationMinMaxResult> async_elevation_minmax(const MultiPolygonHole& p) { return m_dbthread.elevation_minmax(p); }
	Glib::RefPtr<ElevationProfileResult> async_elevation_profile(const Point& p0, const Point& p1, double corridor_width = 5) { return m_dbthread.elevation_profile(p0, p1, corridor_width); }
	Glib::RefPtr<ElevationRouteProfileResult> async_elevation_profile(const FPlanRoute& fpl, double corridor_width = 5) { return m_dbthread.elevation_profile(fpl, corridor_width); }

	std::string get_aux_dir(auxdb_mode_t auxdbmode = auxdb_prefs, const std::string& dir_aux = "");
	static std::string get_default_aux_dir(void);

protected:
	void load_grib2(std::string path);
	void load_grib2_2(std::string path1, std::string path2);
	void load_bitmapmaps(std::string path);
	void load_bitmapmaps_2(std::string path1, std::string path2);
#ifdef HAVE_PILOTLINK
	static PalmDbBase::PalmStringCompare::comp_t to_palm_compare(DbQueryInterfaceCommon::comp_t comp);
#endif

private:
	Preferences m_prefs;
	std::string m_dir_main;
	std::string m_dir_aux;
	DbThread m_dbthread;
	FPlan m_fplandb;
	TracksDb m_tracksdb;
	GRIB2 m_grib2;
	BitmapMaps m_bitmapmaps;
};

inline Engine::AirwayGraphResult::Vertex::typemask_t operator|(Engine::AirwayGraphResult::Vertex::typemask_t x,
							       Engine::AirwayGraphResult::Vertex::typemask_t y)
{
	return (Engine::AirwayGraphResult::Vertex::typemask_t)((unsigned int)x | (unsigned int)y);
}

inline Engine::AirwayGraphResult::Vertex::typemask_t operator&(Engine::AirwayGraphResult::Vertex::typemask_t x,
							       Engine::AirwayGraphResult::Vertex::typemask_t y)
{
	return (Engine::AirwayGraphResult::Vertex::typemask_t)((unsigned int)x & (unsigned int)y);
}

inline Engine::AirwayGraphResult::Vertex::typemask_t operator^(Engine::AirwayGraphResult::Vertex::typemask_t x,
							       Engine::AirwayGraphResult::Vertex::typemask_t y)
{
	return (Engine::AirwayGraphResult::Vertex::typemask_t)((unsigned int)x ^ (unsigned int)y);
}

inline Engine::AirwayGraphResult::Vertex::typemask_t operator~(Engine::AirwayGraphResult::Vertex::typemask_t x)
{
	return (Engine::AirwayGraphResult::Vertex::typemask_t)~(unsigned int)x;
}

inline Engine::AirwayGraphResult::Vertex::typemask_t& operator|=(Engine::AirwayGraphResult::Vertex::typemask_t& x,
								 Engine::AirwayGraphResult::Vertex::typemask_t y)
{
	x = x | y;
	return x;
}

inline Engine::AirwayGraphResult::Vertex::typemask_t& operator&=(Engine::AirwayGraphResult::Vertex::typemask_t& x,
								 Engine::AirwayGraphResult::Vertex::typemask_t y)
{
	x = x & y;
	return x;
}

inline Engine::AirwayGraphResult::Vertex::typemask_t& operator^=(Engine::AirwayGraphResult::Vertex::typemask_t& x,
								 Engine::AirwayGraphResult::Vertex::typemask_t y)
{
	x = x ^ y;
	return x;
}

const std::string& to_str(Engine::AirwayGraphResult::Vertex::type_t t);
std::string to_str(Engine::AirwayGraphResult::Vertex::typemask_t tm);

#endif /* ENGINE_H */
