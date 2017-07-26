//
// C++ Interface: metgraph
//
// Description: Meteo Graphs
//
//
// Author: Thomas Sailer <t.sailer@alumni.ethz.ch>, (C) 2014, 2015, 2016
//
// Copyright: See COPYING file that comes with this distribution
//
//

#ifndef METGRAPH_H
#define METGRAPH_H

#include <glibmm.h>
#include <iostream>
#include <cairomm/cairomm.h>

#include "sysdeps.h"
#include "baro.h"
#include "geom.h"
#include "dbobj.h"
#include "fplan.h"
#include "grib2.h"
#include "metartaf.hh"

class MeteoGraph {
};

class MeteoProfile : public MeteoGraph {
  public:
	typedef enum {
		yaxis_altitude,
		yaxis_pressure
	} yaxis_t;

	MeteoProfile(const FPlanRoute& route = FPlanRoute(*(FPlan *)0),
		     const TopoDb30::RouteProfile& routeprofile = TopoDb30::RouteProfile(),
		     const GRIB2::WeatherProfile& wxprofile = GRIB2::WeatherProfile());

	const FPlanRoute& get_route(void) const { return m_route; }
	void set_route(const FPlanRoute& r = FPlanRoute(*(FPlan *)0)) { m_route = r; }
	const TopoDb30::RouteProfile& get_routeprofile(void) const { return m_routeprofile; }
	void set_routeprofile(const TopoDb30::RouteProfile& rteprof = TopoDb30::RouteProfile()) { m_routeprofile = rteprof; }
	const GRIB2::WeatherProfile& get_wxprofile(void) const { return m_wxprofile; }
	void set_wxprofile(const GRIB2::WeatherProfile& wxprof = GRIB2::WeatherProfile()) { m_wxprofile = wxprof; }

	double get_scaledist(const Cairo::RefPtr<Cairo::Context>& cr, int width, double dist) const;
	double get_scaleelev(const Cairo::RefPtr<Cairo::Context>& cr, int height, double elev,
			     yaxis_t yaxis = yaxis_altitude) const;
	void draw(const Cairo::RefPtr<Cairo::Context>& cr, int width, int height,
		  double origindist, double scaledist, double originelev, double scaleelev,
		  yaxis_t yaxis = yaxis_altitude, bool usetruealt = false,
		  const std::string& servicename = "autorouter.eu") const;

  protected:
	FPlanRoute m_route;
	TopoDb30::RouteProfile m_routeprofile;
	GRIB2::WeatherProfile m_wxprofile;

	static inline void set_color_sky(const Cairo::RefPtr<Cairo::Context>& cr) {
		cr->set_source_rgb(0x76 / 255.0, 0xa3 / 255.0, 0xee / 255.0);
	}

	static inline void set_color_sky(const Cairo::RefPtr<Cairo::LinearGradient>& g) {
		g->add_color_stop_rgb(0.0, 0x76 / 255.0, 0xa3 / 255.0, 0xee / 255.0);
		g->add_color_stop_rgb(1.0, 0x0a / 255.0, 0x37 / 255.0, 0x82 / 255.0);
	}

	static inline void set_color_sky_night(const Cairo::RefPtr<Cairo::Context>& cr) {
		cr->set_source_rgb(0x76 / 255.0 * 0.2, 0xa3 / 255.0 * 0.2, 0xee / 255.0 * 0.2);
	}

	static inline void set_color_sky_duskdawn(const Cairo::RefPtr<Cairo::Context>& cr) {
		cr->set_source_rgb(0x76 / 255.0 * 0.6, 0xa3 / 255.0 * 0.6, 0xee / 255.0 * 0.6);
	}

	static inline void set_color_sky_duskdawn(const Cairo::RefPtr<Cairo::LinearGradient>& g) {
		g->add_color_stop_rgba(0.0, 0x76 / 255.0 * 0.2, 0xa3 / 255.0 * 0.2, 0xee / 255.0 * 0.2, 0.0);
		g->add_color_stop_rgba(1.0, 0x76 / 255.0 * 0.2, 0xa3 / 255.0 * 0.2, 0xee / 255.0 * 0.2, 1.0);
	}

	static inline void set_color_terrain(const Cairo::RefPtr<Cairo::Context>& cr) {
		cr->set_source_rgb(0.31765, 0.21176, 0.13725);
	}

	static inline void set_color_terrain_fill(const Cairo::RefPtr<Cairo::Context>& cr) {
		cr->set_source_rgba(0.31765, 0.21176, 0.13725, 0.333);
	}

	static inline void set_color_terrain_fill_maxelev(const Cairo::RefPtr<Cairo::Context>& cr) {
		cr->set_source_rgb(0.77278, 0.73752, 0.71270);
	}

	static inline void set_color_terrain_fill_elev(const Cairo::RefPtr<Cairo::Context>& cr) {
		cr->set_source_rgb(0.62122, 0.56244, 0.52108);
	}

	static inline void set_color_fplan(const Cairo::RefPtr<Cairo::Context>& cr) {
		cr->set_source_rgb(1., 0., 1.);
	}

	static inline void set_color_marking(const Cairo::RefPtr<Cairo::Context>& cr) {
		cr->set_source_rgb(0., 0., 0.);
	}

	static inline void set_color_grid(const Cairo::RefPtr<Cairo::Context>& cr) {
		cr->set_source_rgb(0x82 / 255., 0x82 / 255., 0x82 / 255.);
	}

	static inline void set_color_0degisotherm(const Cairo::RefPtr<Cairo::Context>& cr) {
		cr->set_source_rgb(0x82 / 255., 0., 0.);
	}

	static inline void set_color_otherisotherm(const Cairo::RefPtr<Cairo::Context>& cr) {
		cr->set_source_rgb(0.0, 0x82 / 255., 0.0);
	}

	static inline void set_color_cloud(const Cairo::RefPtr<Cairo::Context>& cr, double cover) {
		cover = std::max(0., std::min(1., cover));
		double gray(1.0 - 0.25 * cover);
		cr->set_source_rgba(gray, gray, gray, 0.5 + 2.0 * cover);
	}

	static inline void set_color_cloud(const Cairo::RefPtr<Cairo::LinearGradient>& g, double pos, double cover) {
		cover = std::max(0., std::min(1., cover));
		double gray(1.0 - 0.25 * cover);
		g->add_color_stop_rgba(pos, gray, gray, gray, 0.5 + 2.0 * cover);
	}

	static inline void set_color_isotach(const Cairo::RefPtr<Cairo::Context>& cr) {
		cr->set_source_rgb(156 / 256.0, 85 / 256.0, 27 / 256.0);
	}

	class LinePoint {
	public:
		LinePoint(uint32_t x = 0, int32_t y = 0, bool dir = false) : m_x(x), m_y(y), m_dir(dir) {}
		uint32_t get_x(void) const { return m_x; }
		int32_t get_y(void) const { return m_y; }
		bool is_dir(void) const { return m_dir; }
		int compare(const LinePoint& x) const;
		bool operator==(const LinePoint& x) const { return !compare(x); }
		bool operator!=(const LinePoint& x) const { return !operator==(x); }
		bool operator<(const LinePoint& x) const { return compare(x) < 0; }
		bool operator<=(const LinePoint& x) const { return compare(x) <= 0; }
		bool operator>(const LinePoint& x) const { return compare(x) > 0; }
		bool operator>=(const LinePoint& x) const { return compare(x) >= 0; }

	protected:
		uint32_t m_x;
		int32_t m_y;
		bool m_dir;
	};

	class CloudPoint {
	public:
		CloudPoint(double dist = 0, int32_t base = 0, int32_t top = 0, double cover = 0)
			: m_dist(dist), m_cover(cover), m_base(base), m_top(top) {}

		double get_dist(void) const { return m_dist; }
		double get_cover(void) const { return m_cover; }
		int32_t get_base(void) const { return m_base; }
		int32_t get_top(void) const { return m_top; }

	protected:
		double m_dist;
		double m_cover;
		int32_t m_base;
		int32_t m_top;
	};

	class CloudIcons {
	public:
		CloudIcons(const std::string& basefile, const Cairo::RefPtr<Cairo::Surface> sfc);
		operator bool(void) const { return m_ok; }
		Cairo::RefPtr<Cairo::Pattern> operator[](unsigned int i) const;

	protected:
		Cairo::RefPtr<Cairo::Surface> m_icons[4];
		bool m_ok;
	};

	class ConvCloudIcons {
	public:
		ConvCloudIcons(const std::string& basefile, const Cairo::RefPtr<Cairo::Surface> sfc);
		operator bool(void) const { return !m_icons.empty(); }
		Cairo::RefPtr<Cairo::Pattern> get_cloud(double& width, double& height, double& area,
							int32_t top = -1, int32_t tropopause = -1) const;

	protected:
		class Icon {
		public:
			Icon(const Cairo::RefPtr<Cairo::Surface>& sfc, double width, double height);
			const Cairo::RefPtr<Cairo::Surface>& get_surface(void) const { return m_surface; }
			double get_width(void) const { return m_width; }
			double get_height(void) const { return m_height; }
			double get_area(void) const { return m_area; }
			bool operator<(const Icon& x) const { return get_height() < x.get_height(); }

		protected:
			Cairo::RefPtr<Cairo::Surface> m_surface;
			double m_width;
			double m_height;
			double m_area;
		};

		typedef std::multiset<Icon> icons_t;
		icons_t m_icons;
	};

	class StratusCloudBlueNoise {
	public:
		StratusCloudBlueNoise(const std::string& filename, const Cairo::RefPtr<Cairo::Surface> sfc);
		operator bool(void) const { return get_width() && get_height() && !m_groups.empty(); }
		unsigned int get_width(void) const { return m_width; }
		unsigned int get_height(void) const { return m_height; }
		int get_x(void) const { return m_x; }
		int get_y(void) const { return m_y; }

		class Group {
		public:
			Group(const Cairo::RefPtr<Cairo::Surface>& sfc,
			      const Cairo::RefPtr<Cairo::Surface>& alphasfc,
			      unsigned int nr, double x, double y);
			const Cairo::RefPtr<Cairo::Surface>& get_surface(void) const { return m_surface; }
			const Cairo::RefPtr<Cairo::Surface>& get_alphasurface(void) const { return m_alphasurface; }
			Cairo::RefPtr<Cairo::Pattern> get_pattern(void) const;
			Cairo::RefPtr<Cairo::Pattern> get_alphapattern(void) const;
			unsigned int get_nr(void) const { return m_nr; }
			double get_x(void) const { return m_x; }
			double get_y(void) const { return m_y; }
			double get_density(unsigned int idx) const { return m_density[idx & 15]; }
			void set_density(unsigned int idx, double d) { m_density[idx & 15] = d; }
			int compare(const Group& x) const;
			bool operator<(const Group& x) const { return compare(x) < 0; }

		protected:
			Cairo::RefPtr<Cairo::Surface> m_surface;
			Cairo::RefPtr<Cairo::Surface> m_alphasurface;
			double m_density[16];
			double m_x;
			double m_y;
			unsigned int m_nr;
		};

		unsigned int size(void) const { return m_groups.size(); }
		const Group& operator[](unsigned int i) const { return m_groups[i]; }

	protected:
		typedef std::vector<Group> groups_t;
		groups_t m_groups;
		unsigned int m_width;
		unsigned int m_height;
		int m_x;
		int m_y;
	};

	static void extract_remainder(std::vector<LinePoint>& ln, std::multiset<LinePoint>& lp, uint32_t maxydiff = 1000);
	static std::vector<LinePoint> extract_line(std::multiset<LinePoint>& lp, uint32_t maxydiff = 1000);
	static std::vector<LinePoint> extract_area(std::multiset<LinePoint>& lp, uint32_t maxydiff = 1000);
	static void draw_clouds_old(const Cairo::RefPtr<Cairo::Context>& cr, const std::vector<CloudPoint>& cldpt,
				    yaxis_t yaxis = yaxis_altitude);
	static void draw_clouds(const Cairo::RefPtr<Cairo::Context>& cr, const std::vector<CloudPoint>& cldpt,
				yaxis_t yaxis = yaxis_altitude, bool preservepath = false);
	static void draw_clouds(const Cairo::RefPtr<Cairo::Context>& cr, const std::vector<CloudPoint>& cldpt,
				yaxis_t yaxis, const CloudIcons& icons, double scale);
	static void draw_clouds(const Cairo::RefPtr<Cairo::Context>& cr, const std::vector<CloudPoint>& cldpt,
				yaxis_t yaxis, const StratusCloudBlueNoise& icon, double scale);
	static void draw_clouds(const Cairo::RefPtr<Cairo::Context>& cr, const std::vector<CloudPoint>& cldpt,
				yaxis_t yaxis, const ConvCloudIcons& icons, double scale, double aspectratio, int32_t tropopause = 0);
	static void gaussian_blur(const Cairo::RefPtr<Cairo::ImageSurface>& sfc, double sigma2);
	static void center_of_mass(double& x, double& y, const Cairo::RefPtr<Cairo::ImageSurface>& sfc);
	static double yconvert(double y, yaxis_t yaxis) {
		switch (yaxis) {
		//case yaxis_altitude:
		default:
			break;

		case yaxis_pressure:
		{
			float p;
			IcaoAtmosphere<float>::std_altitude_to_pressure(&p, 0, std::max(0., std::min(80000., y)) * Point::ft_to_m_dbl);
			return p;
		}
		}
		return y;
	}
	void errorpage(const Cairo::RefPtr<Cairo::Context>& cr, int width, int height, const std::string& errmsg) const;
};

class MeteoChart : public MeteoGraph {
  public:
	typedef std::vector<FPlanAlternate> alternates_t;
	typedef std::vector<FPlanRoute> altroutes_t;

	MeteoChart(const FPlanRoute& route = FPlanRoute(*(FPlan *)0),
		   const alternates_t& altn = alternates_t(),
		   const altroutes_t& altnfpl = altroutes_t(),
		   const GRIB2& wxdb = *(GRIB2 *)0);
	const FPlanRoute& get_route(void) const { return m_route; }
	void set_route(const FPlanRoute& r = FPlanRoute(*(FPlan *)0)) { m_route = r; }
	const alternates_t& get_alternates(void) const { return m_altn; }
	void set_alternates(const alternates_t& a = alternates_t()) { m_altn = a; }
	const altroutes_t& get_altnroutes(void) const { return m_altnroutes; }
	void set_altnroutes(const altroutes_t& r) { m_altnroutes = r; }
	const GRIB2& get_wxdb(void) const { return *m_wxdb; }
	void set_wxdb(const GRIB2& wxdb = *(GRIB2 *)0) { m_wxdb = &wxdb; clear(); }

	void get_fullscale(int width, int height, Point& center, double& scalelon, double& scalelat) const;
	gint64 get_midefftime(void) const;
	gint64 get_minefftime(void) const { return m_minefftime; }
	gint64 get_maxefftime(void) const { return m_maxefftime; }
	gint64 get_minreftime(void) const { return m_minreftime; }
	gint64 get_maxreftime(void) const { return m_maxreftime; }

	void clear(void);
	void draw(const Cairo::RefPtr<Cairo::Context>& cr, int width, int height,
		  const Point& center, double scalelon, double scalelat, double pressure = 0,
		  const std::string& servicename = "autorouter.eu");

  protected:
	FPlanRoute m_route;
	alternates_t m_altn;
	altroutes_t m_altnroutes;
	const GRIB2 *m_wxdb;
	Glib::RefPtr<GRIB2::LayerInterpolateResult> m_wxwindu;
	Glib::RefPtr<GRIB2::LayerInterpolateResult> m_wxwindv;
	Glib::RefPtr<GRIB2::LayerInterpolateResult> m_wxprmsl;
	Glib::RefPtr<GRIB2::LayerInterpolateResult> m_wxtemperature;
	Glib::RefPtr<GRIB2::LayerInterpolateResult> m_wxtropopause;
	Glib::RefPtr<GRIB2::LayerInterpolateResult> m_wxcldbdrycover;
	Glib::RefPtr<GRIB2::LayerInterpolateResult> m_wxcldlowcover;
	Glib::RefPtr<GRIB2::LayerInterpolateResult> m_wxcldlowbase;
	Glib::RefPtr<GRIB2::LayerInterpolateResult> m_wxcldlowtop;
	Glib::RefPtr<GRIB2::LayerInterpolateResult> m_wxcldmidcover;
	Glib::RefPtr<GRIB2::LayerInterpolateResult> m_wxcldmidbase;
	Glib::RefPtr<GRIB2::LayerInterpolateResult> m_wxcldmidtop;
	Glib::RefPtr<GRIB2::LayerInterpolateResult> m_wxcldhighcover;
	Glib::RefPtr<GRIB2::LayerInterpolateResult> m_wxcldhighbase;
	Glib::RefPtr<GRIB2::LayerInterpolateResult> m_wxcldhightop;
	Glib::RefPtr<GRIB2::LayerInterpolateResult> m_wxcldconvcover;
	Glib::RefPtr<GRIB2::LayerInterpolateResult> m_wxcldconvbase;
	Glib::RefPtr<GRIB2::LayerInterpolateResult> m_wxcldconvtop;
	gint64 m_minefftime;
	gint64 m_maxefftime;
	gint64 m_minreftime;
	gint64 m_maxreftime;
	bool m_gndchart;

	void loaddb(const Rect& bbox, gint64 efftime, double pressure);

	static inline void set_color_fplan(const Cairo::RefPtr<Cairo::Context>& cr) {
		cr->set_source_rgb(1., 0., 1.);
	}

	static inline void set_color_marking(const Cairo::RefPtr<Cairo::Context>& cr) {
		cr->set_source_rgb(0., 0., 0.);
	}

	static inline void set_color_temp(const Cairo::RefPtr<Cairo::Context>& cr) {
		cr->set_source_rgb(1., 0., 0.);
	}

	static inline void set_color_qnh(const Cairo::RefPtr<Cairo::Context>& cr) {
		cr->set_source_rgb(0., 0., 1.);
	}

	static inline void set_color_wxnote(const Cairo::RefPtr<Cairo::Context>& cr) {
		cr->set_source_rgb(0., 0., 0.);
	}

	static inline void set_color_isotach(const Cairo::RefPtr<Cairo::Context>& cr) {
		cr->set_source_rgb(156 / 256.0, 85 / 256.0, 27 / 256.0);
	}

	class AreaExtract {
	  public:
		AreaExtract(const Glib::RefPtr<GRIB2::LayerInterpolateResult>& lay, float lim, gint64 efftime, double sfc1value);
		AreaExtract(const Glib::RefPtr<GRIB2::LayerInterpolateResult>& layu,
			    const Glib::RefPtr<GRIB2::LayerInterpolateResult>& layv, float lim, gint64 efftime, double sfc1value);
		AreaExtract(const Glib::RefPtr<GRIB2::LayerInterpolateResult>& laycover,
			    const Glib::RefPtr<GRIB2::LayerInterpolateResult>& laybase,
			    const Glib::RefPtr<GRIB2::LayerInterpolateResult>& laytop, gint64 efftime, double pressure);
		static std::pair<float,float> minmax(const Glib::RefPtr<GRIB2::LayerInterpolateResult>& lay, gint64 efftime, double sfc1value);
		static std::pair<float,float> minmax(const Glib::RefPtr<GRIB2::LayerInterpolateResult>& layu,
						     const Glib::RefPtr<GRIB2::LayerInterpolateResult>& layv, gint64 efftime, double sfc1value);
		std::vector<Point> extract_contour(void);
		void extract_contours(const Cairo::RefPtr<Cairo::Context>& cr, const Point& center, double scalelon, double scalelat);

	  protected:
		class ScalarGridPt {
		  public:
			ScalarGridPt();
			ScalarGridPt(const Glib::RefPtr<GRIB2::LayerInterpolateResult>& lay, unsigned int x, unsigned int y,
				     float lim, gint64 efftime, double sfc1value);
			ScalarGridPt(const ScalarGridPt& x, const ScalarGridPt& y, float lim);
			const Point& get_coord(void) const { return m_coord; }
			void set_coord(const Point& c) { m_coord = c; }
			float get_val(void) const { return m_val; }
			bool is_inside(void) const { return m_inside; }
			Point contour_point(const ScalarGridPt& x, float lim) const;

		  protected:
			Point m_coord;
			float m_val;
			bool m_inside;
		};

		class WindGridPt : public ScalarGridPt {
		  public:
			WindGridPt();
			WindGridPt(const Glib::RefPtr<GRIB2::LayerInterpolateResult>& layu,
				   const Glib::RefPtr<GRIB2::LayerInterpolateResult>& layv,
				   unsigned int x, unsigned int y, float lim, gint64 efftime, double sfc1value);
			WindGridPt(const WindGridPt& x, const WindGridPt& y, float lim);
		};

		class CloudGridPt {
		  public:
			static constexpr float mincover = 0.4;

			CloudGridPt();
			CloudGridPt(const Glib::RefPtr<GRIB2::LayerInterpolateResult>& laycover,
				    const Glib::RefPtr<GRIB2::LayerInterpolateResult>& laybase,
				    const Glib::RefPtr<GRIB2::LayerInterpolateResult>& laytop,
				    unsigned int x, unsigned int y, gint64 efftime, float pressure);
			CloudGridPt(const CloudGridPt& x, const CloudGridPt& y, float pressure);
			const Point& get_coord(void) const { return m_coord; }
			void set_coord(const Point& c) { m_coord = c; }
			float get_cover(void) const { return m_cover; }
			float get_base(void) const { return m_base; }
			float get_top(void) const { return m_top; }
			bool is_inside(void) const { return m_inside; }
			Point contour_point(const CloudGridPt& x, float lim) const;

		  protected:
			Point m_coord;
			float m_cover;
			float m_base;
			float m_top;
			bool m_inside;
		};

		template <typename PT> class Grid {
		  public:
			Grid(unsigned int w, unsigned int h);
			PT& operator()(unsigned int x, unsigned int y);
			const PT& operator()(unsigned int x, unsigned int y) const;
			unsigned int get_width(void) const { return m_width; }
			unsigned int get_height(void) const { return m_height; }
			void set_border(void);
			unsigned int get_index(unsigned int x, unsigned int y) const;
			unsigned int get_edgenr(unsigned int x0, unsigned int y0, unsigned int x1, unsigned int y1) const;

		  protected:
			std::vector<PT> m_v;
			unsigned int m_width;
			unsigned int m_height;
		};

		class Edge {
		  public:
			Edge(const Point& st = Point::invalid, unsigned int ste = 0, const Point& en = Point::invalid, unsigned int ene = 0,
			     const Point& mid = Point::invalid) : m_start(st), m_end(en), m_intermediate(mid), m_startedge(ste), m_endedge(ene) {}
			const Point& get_start(void) const { return m_start; }
			const Point& get_end(void) const { return m_end; }
			const Point& get_intermediate(void) const { return m_intermediate; }
			unsigned int get_startedge(void) const { return m_startedge; }
			unsigned int get_endedge(void) const { return m_endedge; }
			int compare(const Edge& x) const;
			bool operator==(const Edge& x) const { return compare(x) == 0; }
			bool operator!=(const Edge& x) const { return compare(x) != 0; }
			bool operator<(const Edge& x) const { return compare(x) < 0; }
			bool operator<=(const Edge& x) const { return compare(x) <= 0; }
			bool operator>(const Edge& x) const { return compare(x) > 0; }
			bool operator>=(const Edge& x) const { return compare(x) >= 0; }

		  protected:
			Point m_start;
			Point m_end;
			Point m_intermediate;
			unsigned int m_startedge;
			unsigned int m_endedge;
		};

		typedef std::set<Edge> edges_t;
		edges_t m_edges;

		template <typename PT> void compute(Grid<PT>& grid, float lim);
	};
};

class WMOStation {
  public:
	WMOStation(const char *icao = 0, const char *name = 0, const char *state = 0, const char *country = 0,
		   unsigned int wmonr = ~0U, unsigned int wmoregion = ~0U, const Point& coord = Point::invalid,
		   TopoDb30::elev_t elev = TopoDb30::nodata, bool basic = false)
		: m_icao(icao), m_name(name), m_state(state), m_country(country),
		  m_wmonr(wmonr), m_wmoregion(wmoregion), m_coord(coord), m_elev(elev), m_basic(basic) {}

	const char *get_icao(void) const { return m_icao; }
	const char *get_name(void) const { return m_name; }
	const char *get_state(void) const { return m_state; }
	const char *get_country(void) const { return m_country; }
	unsigned int get_wmonr(void) const { return m_wmonr; }
	unsigned int get_wmoregion(void) const { return m_wmoregion; }
	Point get_coord(void) const { return m_coord; }
	TopoDb30::elev_t get_elev(void) const { return m_elev; }
	bool is_basic(void) const { return m_basic; }

	typedef const WMOStation *const_iterator;
	typedef const_iterator iterator;
	static const_iterator begin(void) { return stations; }
	static const_iterator end(void) { return stations + nrstations; }
	static const_iterator find(unsigned int wmonr);

	struct WMONrSort {
		bool operator()(const WMOStation& a, const WMOStation& b) {
			return a.get_wmonr() < b.get_wmonr();
		}
	};

  protected:
	static const unsigned int nrstations;
	static const WMOStation stations[];

	const char *m_icao;
	const char *m_name;
	const char *m_state;
	const char *m_country;
	unsigned int m_wmonr;
	unsigned int m_wmoregion;
	Point m_coord;
	TopoDb30::elev_t m_elev;
	bool m_basic;
};

class RadioSounding {
  public:
	RadioSounding(unsigned int wmonr = ~0U, time_t obstm = 0);

	int compare(const RadioSounding& x) const;
	bool operator==(const RadioSounding& x) const { return compare(x) == 0; }
	bool operator!=(const RadioSounding& x) const { return compare(x) != 0; }
	bool operator<(const RadioSounding& x) const { return compare(x) < 0; }
	bool operator<=(const RadioSounding& x) const { return compare(x) <= 0; }
	bool operator>(const RadioSounding& x) const { return compare(x) > 0; }
	bool operator>=(const RadioSounding& x) const { return compare(x) >= 0; }

	time_t get_obstime(void) const { return m_obstime; }
	unsigned int get_wmonr(void) const { return m_wmonr; }

	class Level {
	  public:
		Level(uint16_t press = 0, int16_t temp = std::numeric_limits<int16_t>::min(),
		      int16_t dewpt = std::numeric_limits<int16_t>::min(),
		      uint16_t winddir = std::numeric_limits<uint16_t>::max(),
		      uint16_t windspd = std::numeric_limits<uint16_t>::max());

		int compare(const Level& x) const;
		bool operator==(const Level& x) const { return compare(x) == 0; }
		bool operator!=(const Level& x) const { return compare(x) != 0; }
		bool operator<(const Level& x) const { return compare(x) < 0; }
		bool operator<=(const Level& x) const { return compare(x) <= 0; }
		bool operator>(const Level& x) const { return compare(x) > 0; }
		bool operator>=(const Level& x) const { return compare(x) >= 0; }

		uint16_t get_press_dpa(void) const { return m_press; }
		void set_press_dpa(uint16_t x) { m_press = x; }
		int16_t get_temp_degc10(void) const { return m_temp; }
		void set_temp_degc10(int16_t x = std::numeric_limits<int16_t>::min()) { m_temp = x; }
		bool is_temp_valid(void) const { return get_temp_degc10() != std::numeric_limits<int16_t>::min(); }
		int16_t get_dewpt_degc10(void) const { return m_dewpt; }
		void set_dewpt_degc10(int16_t x = std::numeric_limits<int16_t>::min()) { m_dewpt = x; }
		bool is_dewpt_valid(void) const { return get_dewpt_degc10() != std::numeric_limits<int16_t>::min(); }
		uint16_t get_winddir_deg(void) const { return m_winddir; }
		void set_winddir_deg(uint16_t x = std::numeric_limits<uint16_t>::max()) { m_winddir = x; }
		uint16_t get_windspeed_kts16(void) const { return m_windspeed; }
		void set_windspeed_kts16(uint16_t x = std::numeric_limits<uint16_t>::max()) { m_windspeed = x; }
		bool is_wind_valid(void) const {
			return get_winddir_deg() != std::numeric_limits<uint16_t>::max() &&
				get_windspeed_kts16() != std::numeric_limits<uint16_t>::max();
		}
		bool is_surface(void) const { return !!(m_flags & 1); }
		void set_surface(bool x) { m_flags ^= (m_flags ^ -!!x) & 1; }
		bool is_tropopause(void) const { return !!(m_flags & 2); }
		void set_tropopause(bool x) { m_flags ^= (m_flags ^ -!!x) & 2; }
		bool is_maxwind(void) const { return !!(m_flags & 4); }
		void set_maxwind(bool x) { m_flags ^= (m_flags ^ -!!x) & 4; }

		std::ostream& print(std::ostream& os) const;

		operator GRIB2::WeatherProfilePoint::SoundingSurface(void) const;

	  protected:
		uint16_t m_press;
		int16_t m_temp;
		int16_t m_dewpt;
		uint16_t m_winddir;
		uint16_t m_windspeed;
		uint16_t m_flags;
	};

	typedef std::set<Level> levels_t;
	levels_t& get_levels(void) { return m_levels; }
	const levels_t& get_levels(void) const { return m_levels; }
	void set_levels(const levels_t& l) { m_levels = l; }
	Level& insert_level(uint16_t press);

	typedef GRIB2::WeatherProfilePoint::soundingsurfaces_t soundingsurfaces_t;
	operator soundingsurfaces_t(void) const;

	std::ostream& print(std::ostream& os) const;

  protected:
	time_t m_obstime;
	unsigned int m_wmonr;
	levels_t m_levels;
};

class RadioSoundings : public std::set<RadioSounding> {
  public:
	unsigned int parse_wmo(std::istream& is, time_t filedate);
	unsigned int parse_wmo(const std::string& fn, time_t filedate);
	unsigned int parse_wmo(const std::string& dir, const std::string& fn);
	unsigned int parse_dir(const std::string& dir);
	unsigned int parse_dirs(const std::string& maindir = "", const std::string& auxdir = "");

	std::ostream& print(std::ostream& os) const;

  protected:
	unsigned int parse_ttaa(const std::vector<std::string>& tok, time_t filedate, uint16_t pressmul);
	unsigned int parse_ttbb(const std::vector<std::string>& tok, time_t filedate, uint16_t pressmul);

	static unsigned int getint(const std::string& s, unsigned int i, unsigned int n);
	static time_t getdate(bool& knots, const std::string& s, time_t filedate);
	RadioSounding& parse_header(bool& knots, bool& newrs, const std::vector<std::string>& tok, time_t filedate);
};

class METARTAFChart : public MeteoGraph {
  public:
	typedef enum {
		dbmode_sqlite,
		dbmode_pg
	} dbmode_t;
	typedef std::vector<FPlanAlternate> alternates_t;
	typedef std::vector<FPlanRoute> altroutes_t;

	METARTAFChart(const FPlanRoute& route = FPlanRoute(*(FPlan *)0),
		      const alternates_t& altn = alternates_t(),
		      const altroutes_t& altnfpl = altroutes_t(),
		      const std::string& db = PACKAGE_DATA_DIR "/metartaf.db",
		      dbmode_t dbmode = dbmode_sqlite,
		      const RadioSoundings& raobs = *(const RadioSoundings *)0,
		      const std::string& tempdir = "");
	const FPlanRoute& get_route(void) const { return m_route; }
	void set_route(const FPlanRoute& r = FPlanRoute(*(FPlan *)0)) { m_route = r; }
	const alternates_t& get_alternates(void) const { return m_altn; }
	void set_alternates(const alternates_t& a = alternates_t()) { m_altn = a; }
	const altroutes_t& get_altnroutes(void) const { return m_altnroutes; }
	void set_altnroutes(const altroutes_t& r) { m_altnroutes = r; }
	void add_fir(const std::string& id = "", const MultiPolygonHole& poly = MultiPolygonHole());
	Rect get_bbox(void) const;
	Rect get_bbox(unsigned int wptidx0, unsigned int wptidx1) const;
	Rect get_extended_bbox(void) const;
	Rect get_extended_bbox(unsigned int wptidx0, unsigned int wptidx1) const;
	std::pair<time_t,time_t> get_timebound(void) const;

	void draw(const Cairo::RefPtr<Cairo::Context>& cr, int width, int height);
	void draw(std::ostream& os, double width, double height);

	virtual void draw_bgndlayer(const Cairo::RefPtr<Cairo::Context>& cr, const Rect& bbox,
				    const Point& center, double scalelon, double scalelat) {}
	virtual void draw_bgndlayer(std::ostream& os, const Rect& bbox, const Point& center,
				    double width, double height, double scalelon, double scalelat) {}

	static Glib::ustring latex_string_meta(const Glib::ustring& x);
	static std::string latex_string_meta(const std::string& x);
	static std::string pgfcoord(double x, double y);
	static std::string georefcoord(double x, double y);
	static std::string georefcoord(const Point& pt, double x, double y);
	static std::string georefcoord(const Point& pt, const Point& center, double width, double height, double scalelon, double scalelat);

	static void polypath(const Cairo::RefPtr<Cairo::Context>& cr, const PolygonSimple& poly,
			     const Rect& bbox, const Point& center, double scalelon, double scalelat);
	static void polypath(const Cairo::RefPtr<Cairo::Context>& cr, const PolygonHole& poly,
			     const Rect& bbox, const Point& center, double scalelon, double scalelat);
	static void polypath(const Cairo::RefPtr<Cairo::Context>& cr, const MultiPolygonHole& poly,
			     const Rect& bbox, const Point& center, double scalelon, double scalelat);
	static void polypath(const Cairo::RefPtr<Cairo::Context>& cr, const LineString& line,
			     const Rect& bbox, const Point& center, double scalelon, double scalelat);
	static void polypath(const Cairo::RefPtr<Cairo::Context>& cr, const MultiLineString& line,
			     const Rect& bbox, const Point& center, double scalelon, double scalelat);
	static void polypath(std::ostream& os, const std::string& attr, const PolygonSimple& poly,
			     const Rect& bbox, const Point& center, double width, double height, double scalelon, double scalelat);
	static void polypath(std::ostream& os, const std::string& attr, const PolygonHole& poly,
			     const Rect& bbox, const Point& center, double width, double height, double scalelon, double scalelat);
	static void polypath(std::ostream& os, const std::string& attr, const MultiPolygonHole& poly,
			     const Rect& bbox, const Point& center, double width, double height, double scalelon, double scalelat);
	static void polypath(std::ostream& os, const std::string& attr, const LineString& line,
			     const Rect& bbox, const Point& center, double width, double height, double scalelon, double scalelat);
	static void polypath(std::ostream& os, const std::string& attr, const MultiLineString& line,
			     const Rect& bbox, const Point& center, double width, double height, double scalelon, double scalelat);
	static void polypath_fill_helper(std::ostream& os, const PolygonSimple& poly,
					 const Point& center, double width, double height, double scalelon, double scalelat);
	static void polypath_fill(std::ostream& os, const std::string& attr, const PolygonHole& poly,
				  const Point& center, double width, double height, double scalelon, double scalelat);
	static void polypath_fill(std::ostream& os, const std::string& attr, const MultiPolygonHole& poly,
				  const Point& center, double width, double height, double scalelon, double scalelat);

	class Label {
	  public:
		typedef enum {
			placement_center,
			placement_north,
			placement_northeast,
			placement_east,
			placement_southeast,
			placement_south,
			placement_southwest,
			placement_west,
			placement_northwest
		} placement_t;

		Label(const std::string& txt = "", const std::string& code = "",
		      double x = std::numeric_limits<double>::quiet_NaN(),
		      double y = std::numeric_limits<double>::quiet_NaN(),
		      placement_t placement = placement_center, bool fixed = false, bool keep = false);
		double get_anchorx(void) const { return m_ax; }
		double get_anchory(void) const { return m_ay; }
		double get_nominalx(void) const { return m_nx; }
		double get_nominaly(void) const { return m_ny; }
		double get_x(void) const { return m_x; }
		double get_y(void) const { return m_y; }
		double get_diffx(void) const { return get_x() - get_nominalx(); }
		double get_diffy(void) const { return get_y() - get_nominaly(); }
		const std::string& get_text(void) const { return m_txt; }
		const std::string& get_code(void) const { return m_code; }
		placement_t get_placement(void) const { return m_placement; }
		static const std::string& get_tikz_anchor(placement_t p);
		const std::string& get_tikz_anchor(void) const { return get_tikz_anchor(get_placement()); }
		static placement_t placement_from_quadrant(unsigned int q);
		bool is_fixed(void) const { return m_fixed; }
		bool is_keep(void) const { return m_keep; }
		double attractive_force(void) const;
		double repulsive_force(const Label& lbl, double maxx, double maxy) const;
		void move(double dx, double dy, double minx, double maxx, double miny, double maxy);

	  protected:
		std::string m_txt;
		std::string m_code;
		double m_ax;
		double m_ay;
		double m_nx;
		double m_ny;
		double m_x;
		double m_y;
		placement_t m_placement;
		bool m_fixed;
		bool m_keep;
	};

	class LabelLayout : public std::vector<Label> {
	  public:
		LabelLayout(double width, double height, double maxx, double maxy);

		void setk(double kr);
		void iterate(double temp);
		double total_repulsive_force(void) const;
		double total_attractive_force(void) const;
		double repulsive_force(size_type i) const;
		void remove_overlapping_labels(void);

	  protected:
		double m_width;
		double m_height;
		double m_maxx;
		double m_maxy;
		double m_ka;
		double m_kr;
	};

  protected:
	class OrderFlightPath {
	  public:
		OrderFlightPath(const Point& p0, const Point& p1);
		bool operator()(const MetarTafSet::Station& a, const MetarTafSet::Station& b) const;

	  protected:
		Point m_pt0;
		Point m_pt1;
		double m_lonscale2;
	};

	static constexpr double max_dist_from_route = 50.;
	static constexpr double max_raob_dist_from_route = 100.;
	static constexpr double chart_strip_length = 500.;
	static const unsigned int metar_history_size = 1;
	static const unsigned int metar_depdestalt_history_size = 2;
	static const unsigned int taf_history_size = 1;
	// \usepackage{calc}
	// \newlength\xw
	// \newlength\xd
	// \newlength\xh
	// \settowidth{\xw}{XXXX}
	// \settodepth{\xd}{XXXX}
	// \settoheight{\xh}{XXXX}
	// \the\xw
	// \the\xh

	static constexpr double tex_char_width = 0.25 * 24.01172 * 0.035277778;
	static constexpr double tex_char_height = 6.50241 * 0.035277778;
	static constexpr double tex_anchor_x = 0.04;
	static constexpr double tex_anchor_y = 0.04;

	static inline void set_color_fplan(const Cairo::RefPtr<Cairo::Context>& cr) {
		cr->set_source_rgb(1., 0., 1.);
	}

	static inline void set_color_ctrybdry(const Cairo::RefPtr<Cairo::Context>& cr) {
		cr->set_source_rgb(0., 0., 0.);
	}

	static inline void set_color_frules(const Cairo::RefPtr<Cairo::Context>& cr, MetarTafSet::METAR::frules_t frules) {
		switch (frules) {
		case MetarTafSet::METAR::frules_lifr:
			cr->set_source_rgb(0.5, 0., 0.5);
			break;

		case MetarTafSet::METAR::frules_ifr:
			cr->set_source_rgb(0.5, 0., 0.);
			break;

		case MetarTafSet::METAR::frules_mvfr:
			cr->set_source_rgb(0., 0., 0.5);
			break;

		case MetarTafSet::METAR::frules_vfr:
			cr->set_source_rgb(0., 0.5, 0.);
			break;

		default:
			cr->set_source_rgb(0., 0., 0.);
			break;
		}
	}

	void loadstn_sqlite(const Rect& bbox);
	void loadstn_pg(const Rect& bbox);
	void loadstn_raob(const Rect& bbox);
	void loadstn(const Rect& bbox);
	void filterstn(void);
	std::string findstn(const std::string& icao, const Point& coord = Point::invalid) const;
	std::string findstn(const FPlanWaypoint& wpt) const;
	typedef std::set<std::string> stnfilter_t;
	std::ostream& dump_latex(std::ostream& os, const stnfilter_t& filter, bool exclude);
	std::ostream& dump_latex_stnlist(std::ostream& os);
	std::ostream& dump_latex_sigmet(std::ostream& os);
	virtual std::ostream& dump_latex_legend(std::ostream& os);
	std::ostream& dump_latex_raobs(std::ostream& os);
	void draw_firs(std::ostream& os, LabelLayout& lbl, const Rect& bbox, const Point& center,
		       double width, double height, double scalelon, double scalelat);
	void draw(unsigned int wptidx0, unsigned int wptidx1, const Cairo::RefPtr<Cairo::Context>& cr, int width, int height);
	void draw(unsigned int wptidx0, unsigned int wptidx1, std::ostream& os, double width, double height);
	unsigned int get_chart_strip(unsigned int wptstart) const;

	const RadioSoundings& m_raobs;
	FPlanRoute m_route;
	alternates_t m_altn;
	altroutes_t m_altnroutes;
	MetarTafSet m_set;
	std::string m_tempdir;
	std::string m_db;
	dbmode_t m_dbmode;
};

class SkewTChart : public MeteoGraph {
  public:
	SkewTChart(int32_t alt = std::numeric_limits<int32_t>::min());
	virtual void calc_stability(void) = 0;

	void draw(const Cairo::RefPtr<Cairo::Context>& cr, double width, double height, bool vlabels = true);
	std::ostream& draw(std::ostream& os, double width, double height, const std::string& boxname = "");
	std::ostream& drawtikz(std::ostream& os, double width, double height, const std::string& boxname = "");
	std::ostream& drawtikzhlabels(std::ostream& os, double width, double height);
	std::ostream& drawtikzvlabels(std::ostream& os, double width, double height, bool right = false);
	virtual std::ostream& drawtikzwindbarbs(std::ostream& os, double xorigin, double height,
						double xorigin2 = std::numeric_limits<double>::quiet_NaN()) = 0;
	static std::ostream& prerender(std::ostream& os, const std::string& boxname, const std::string& opt, double width, double height);

	static std::pair<double,double> transform(double p_mbar, double t_degc);
	static std::string pgfcoord(std::pair<double,double> r, double width, double height);
	static std::string pgfcoord(double p_mbar, double t_degc, double width, double height) { return pgfcoord(transform(p_mbar, t_degc), width, height); }
	static void cairopath(const Cairo::RefPtr<Cairo::Context>& cr, bool move, std::pair<double,double> r, double width, double height);
	static void cairopath(const Cairo::RefPtr<Cairo::Context>& cr, bool move, double p_mbar, double t_degc, double width, double height) {
		cairopath(cr, move, transform(p_mbar, t_degc), width, height);
	}

	static double dry_adiabat(double t_kelvin, double p_mbar);
	static double sat_adiabat(double t_kelvin, double p_mbar);
	static double mixing_ratio(double w, double p_mbar);

	static std::ostream& define_latex_colors(std::ostream& os);
	static std::ostream& define_latex_styles(std::ostream& os);
	static std::ostream& draw_legend(std::ostream& os);
	static std::ostream& draw_windbarbuv(std::ostream& os, float wu, float wv, unsigned int mask = 3);
	static std::ostream& draw_windbarb(std::ostream& os, float wdirdeg, float wspeedkts, unsigned int mask = 3);
	static void draw_windbarbuv(const Cairo::RefPtr<Cairo::Context>& cr, float wu, float wv, unsigned int mask = 3);
	static void draw_windbarb(const Cairo::RefPtr<Cairo::Context>& cr, float wdirdeg, float wspeedkts, unsigned int mask = 3);

	static inline void set_color_gnd(const Cairo::RefPtr<Cairo::Context>& cr) {
		cr->set_source_rgb(0.77278, 0.73752, 0.71270);
	}

	static inline void set_color_press(const Cairo::RefPtr<Cairo::Context>& cr) {
		cr->set_source_rgb(0, 0, 1);
	}

	static inline void set_color_temp(const Cairo::RefPtr<Cairo::Context>& cr) {
		cr->set_source_rgb(1, 0, 0);
	}

	static inline void set_color_dryad(const Cairo::RefPtr<Cairo::Context>& cr) {
		cr->set_source_rgb(0, 1, 0);
	}

	static inline void set_color_satad(const Cairo::RefPtr<Cairo::Context>& cr) {
		cr->set_source_rgb(0, 1, 0);
	}

	static inline void set_color_mixr(const Cairo::RefPtr<Cairo::Context>& cr) {
		cr->set_source_rgb(0, 1, 1);
	}

	static inline void set_color_tempicao(const Cairo::RefPtr<Cairo::Context>& cr) {
		cr->set_source_rgb(1, 0, 0);
	}

	static inline void set_color_tracetemp(const Cairo::RefPtr<Cairo::Context>& cr) {
		cr->set_source_rgb(0, 0, 0);
	}

	static inline void set_color_tracedewpt(const Cairo::RefPtr<Cairo::Context>& cr) {
		cr->set_source_rgb(0, 0, 0);
	}

	static inline void set_color_liftcurve(const Cairo::RefPtr<Cairo::Context>& cr) {
		cr->set_source_rgb(0.62745, 0.50196, 0);
	}

	static inline void set_color_cin(const Cairo::RefPtr<Cairo::Context>& cr) {
		cr->set_source_rgba(0, 0.6, 0.1, 0.2);
	}

	static inline void set_color_cape(const Cairo::RefPtr<Cairo::Context>& cr) {
		cr->set_source_rgba(1.0, 0.5, 0, 0.2);
	}

	static inline void set_color_fplan(const Cairo::RefPtr<Cairo::Context>& cr) {
		cr->set_source_rgb(1., 0., 1.);
	}

	static inline void set_style_press(const Cairo::RefPtr<Cairo::Context>& cr) {
		set_color_press(cr);
		cr->set_line_width(0.5);
		cr->unset_dash();
	}

	static inline void set_style_temp(const Cairo::RefPtr<Cairo::Context>& cr, bool zero = false) {
		set_color_temp(cr);
		cr->set_line_width(zero ? 1.0 : 0.5);
		cr->unset_dash();
	}

	static inline void set_style_dryad(const Cairo::RefPtr<Cairo::Context>& cr) {
		set_color_dryad(cr);
		cr->set_line_width(0.5);
		cr->unset_dash();
	}

	static inline void set_style_satad(const Cairo::RefPtr<Cairo::Context>& cr) {
		set_color_satad(cr);
		cr->set_line_width(0.5);
		std::vector<double> dashes;
		dashes.push_back(2);
		dashes.push_back(2);
		cr->set_dash(dashes, 0);
	}

	static inline void set_style_mixr(const Cairo::RefPtr<Cairo::Context>& cr) {
		set_color_mixr(cr);
		cr->set_line_width(0.5);
		cr->unset_dash();
	}

	static inline void set_style_tempicao(const Cairo::RefPtr<Cairo::Context>& cr) {
		set_color_tempicao(cr);
		cr->set_line_width(1.0);
		cr->unset_dash();
	}

	static inline void set_style_tracetemp(const Cairo::RefPtr<Cairo::Context>& cr, bool sec = false) {
		set_color_tracetemp(cr);
		cr->set_line_width(1.0);
		if (sec) {
			std::vector<double> dashes;
			dashes.push_back(2);
			dashes.push_back(2);
			cr->set_dash(dashes, 0);
		} else {
			cr->unset_dash();
		}
	}

	static inline void set_style_tracedewpt(const Cairo::RefPtr<Cairo::Context>& cr, bool sec = false) {
		set_color_tracedewpt(cr);
		cr->set_line_width(1.0);
		if (sec) {
			std::vector<double> dashes;
			dashes.push_back(2);
			dashes.push_back(2);
			cr->set_dash(dashes, 0);
		} else {
			cr->unset_dash();
		}
	}

	static inline void set_style_liftcurve(const Cairo::RefPtr<Cairo::Context>& cr) {
		set_color_liftcurve(cr);
		cr->set_line_width(1.0);
		cr->unset_dash();
	}

	static inline void set_style_fplan(const Cairo::RefPtr<Cairo::Context>& cr) {
		set_color_fplan(cr);
		cr->set_line_width(1.0);
		cr->unset_dash();
	}

	static double refractivity(double press_mb, double temp_kelvin, double dewpt_kelvin);
	static double refractivity(const GRIB2::WeatherProfilePoint& wpp, unsigned int i);
	static double refractivity(const RadioSounding::Level& lvl);

	const GRIB2::WeatherProfilePoint::Stability& get_stability(void) const { return m_stability; }

  protected:
	static const double draw_mixr[];
	GRIB2::WeatherProfilePoint::Stability m_stability;
	int32_t m_alt;

	static double sign(double x);
	static double esat(double t_kelvin);
	static double wfunc(double t_kelvin, double p_mbar);
	std::ostream& drawtikzstability(std::ostream& os, double width, double height);
	static std::ostream& drawtikzgrid(std::ostream& os, double width, double height);
	virtual std::ostream& drawtikztraces(std::ostream& os, double width, double height) = 0;
	virtual std::ostream& drawtikzterrain(std::ostream& os, double width, double height) = 0;
	virtual std::ostream& drawtikzlegend(std::ostream& os, double width, double height) = 0;
	void drawstability(const Cairo::RefPtr<Cairo::Context>& cr, double width, double height);
	static void drawgrid(const Cairo::RefPtr<Cairo::Context>& cr, double width, double height);
	static void drawhlabels(const Cairo::RefPtr<Cairo::Context>& cr, double width, double height);
	static void drawvlabels(const Cairo::RefPtr<Cairo::Context>& cr, double width, double height);
	virtual void drawtraces(const Cairo::RefPtr<Cairo::Context>& cr, double width, double height) = 0;
	virtual void drawterrain(const Cairo::RefPtr<Cairo::Context>& cr, double width, double height) = 0;
	virtual void drawlegend(const Cairo::RefPtr<Cairo::Context>& cr, double width, double height) = 0;
	virtual void drawwindbarbs(const Cairo::RefPtr<Cairo::Context>& cr, double xorigin, double height,
				   double xorigin2 = std::numeric_limits<double>::quiet_NaN()) = 0;
};

class SkewTChartPrognostic : public SkewTChart
{
  public:
	SkewTChartPrognostic(const Point& pt = Point::invalid, gint64 efftime = 0,
			     const std::string& name = "",
			     int32_t alt = std::numeric_limits<int32_t>::min(),
			     TopoDb30::elev_t elev = TopoDb30::nodata,
			     GRIB2 *wxdb = (GRIB2 *)0);
	virtual void calc_stability(void);

	virtual std::ostream& drawtikzwindbarbs(std::ostream& os, double xorigin, double height,
						double xorigin2 = std::numeric_limits<double>::quiet_NaN());

  protected:
	GRIB2::WeatherProfilePoint m_wpp;
	Point m_pt;
	gint64 m_efftime;
	std::string m_name;
	TopoDb30::elev_t m_elev;

	virtual std::ostream& drawtikztraces(std::ostream& os, double width, double height);
	virtual std::ostream& drawtikzterrain(std::ostream& os, double width, double height);
	virtual std::ostream& drawtikzlegend(std::ostream& os, double width, double height);
	virtual void drawtraces(const Cairo::RefPtr<Cairo::Context>& cr, double width, double height);
	virtual void drawterrain(const Cairo::RefPtr<Cairo::Context>& cr, double width, double height);
	virtual void drawlegend(const Cairo::RefPtr<Cairo::Context>& cr, double width, double height);
	virtual void drawwindbarbs(const Cairo::RefPtr<Cairo::Context>& cr, double xorigin, double height,
				   double xorigin2 = std::numeric_limits<double>::quiet_NaN());
};

class SkewTChartSounding : public SkewTChart
{
  public:
	SkewTChartSounding(const RadioSounding& prim = RadioSounding(),
			   const RadioSounding& sec = RadioSounding(),
			   int32_t alt = std::numeric_limits<int32_t>::min());
	virtual void calc_stability(void);

	virtual std::ostream& drawtikzwindbarbs(std::ostream& os, double xorigin, double height,
						double xorigin2 = std::numeric_limits<double>::quiet_NaN());

  protected:
	RadioSounding m_data[2];

	virtual std::ostream& drawtikztraces(std::ostream& os, double width, double height);
	virtual std::ostream& drawtikzterrain(std::ostream& os, double width, double height);
	virtual std::ostream& drawtikzlegend(std::ostream& os, double width, double height);
	virtual void drawtraces(const Cairo::RefPtr<Cairo::Context>& cr, double width, double height);
	virtual void drawterrain(const Cairo::RefPtr<Cairo::Context>& cr, double width, double height);
	virtual void drawlegend(const Cairo::RefPtr<Cairo::Context>& cr, double width, double height);
	virtual void drawwindbarbs(const Cairo::RefPtr<Cairo::Context>& cr, double xorigin, double height,
				   double xorigin2 = std::numeric_limits<double>::quiet_NaN());
};

const std::string& to_str(Cairo::SurfaceType st);
inline std::ostream& operator<<(std::ostream& os, Cairo::SurfaceType st) { return os << to_str(st); }
const std::string& to_str(Cairo::Format f);
inline std::ostream& operator<<(std::ostream& os, Cairo::Format f) { return os << to_str(f); }

#endif /* METGRAPH_H */
