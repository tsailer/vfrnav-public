//
// C++ Interface: bitmapmaps
//
// Description: Bitmap Maps
//
//
// Author: Thomas Sailer <t.sailer@alumni.ethz.ch>, (C) 2003, 2004, 2013
//
// Copyright: See COPYING file that comes with this distribution
//
//

#ifndef BITMAPMAPS_H
#define BITMAPMAPS_H

#include "sysdeps.h"

#include <libxml++/libxml++.h>
#include <gdkmm.h>
#include <glibmm.h>

#include "geom.h"

class BitmapMaps {
  public:
	class Ellipsoid {
	  public:
		Ellipsoid(void);
		bool set(const std::string& name);
		void diffwgs84(double& dlat, double& dlon, double& dh, double lat, double lon, double h);
		void diffwgs84(double& dlat, double& dlon, double& dh, const Point& pt, double h);
		void diffwgs84(Point& dpt, double& dh, const Point& pt, double h);

	  protected:
		struct ellipsoids {
			const char *name;
			double a;
			double f;
			double dx, dy, dz;
		};

		static const struct ellipsoids ellipsoids[];

		bool set(const struct ellipsoids *ell);

		double m_a;
		double m_f;
		double m_dx;
		double m_dy;
		double m_dz;
		double m_b;
		double m_e2;
		double m_da;
		double m_df;
	};

	class GeoRefPoint {
	  public:
		GeoRefPoint(unsigned int x = 0, unsigned int y = 0, const Point& pt = Point::invalid,
			    const std::string& name = "");
		const std::string& get_name(void) const { return m_name; }
		const Point& get_point(void) const { return m_point; }
		Point::coord_t get_lat(void) const { return m_point.get_lat(); }
		Point::coord_t get_lon(void) const { return m_point.get_lon(); }
		unsigned int get_x(void) const { return m_x; }
		unsigned int get_y(void) const { return m_y; }

		int load_xml(const xmlpp::Element *el);

	  protected:
		std::string m_name;
		Point m_point;
		unsigned int m_x;
		unsigned int m_y;
	};

	typedef std::vector<GeoRefPoint> georef_t;

	class Projection {
	  public:
		typedef Glib::RefPtr<Projection> ptr_t;
		typedef Glib::RefPtr<const Projection> const_ptr_t;

		Projection(Point::coord_t reflon);
		virtual ~Projection();

                void reference(void) const;
                void unreference(void) const;

		int init_affine_transform(void);
		int init_affine_transform(const georef_t& ref, int northx = 0, int northy = 0,
					   double northdist = std::numeric_limits<double>::quiet_NaN());

		void proj_normal(double& x, double& y, double lat, double lon) const;
		void proj_normal(double& x, double& y, const Point& pt) const;
		void proj_reverse(double& lat, double& lon, double x, double y) const;
		void proj_reverse(Point& pt, double x, double y) const;

          protected:
		virtual void proj_core_normal(double& x, double& y, double lat, double lon) const = 0;
		virtual void proj_core_reverse(double& lat, double& lon, double x, double y) const = 0;

                mutable gint m_refcount;
		Point::coord_t m_reflon;
		double m_atfwd[6];  // affine transform in forward direction
		double m_atrev[4];  // affine transform in reverse direction
        };

	class ProjectionLinear : public Projection {
	  public:
		ProjectionLinear(Point::coord_t reflon);

	  protected:
		void proj_core_normal(double& x, double& y, double lat, double lon) const;
		void proj_core_reverse(double& lat, double& lon, double x, double y) const;
	};

	class ProjectionLambert : public Projection {
	  public:
		ProjectionLambert(const Point& refpt, Point::coord_t stdpar1, Point::coord_t stdpar2);

	  protected:
		void proj_core_normal(double& x, double& y, double lat, double lon) const;
		void proj_core_reverse(double& lat, double& lon, double x, double y) const;

		double m_n;
		double m_f;
		double m_rho0;
	};

	class Map {
	  public:
		typedef Glib::RefPtr<Map> ptr_t;
		typedef Glib::RefPtr<const Map> const_ptr_t;

		class Tile {
		  public:
			typedef Glib::RefPtr<Tile> ptr_t;
			typedef Glib::RefPtr<const Tile> const_ptr_t;

			Tile(void);
			~Tile();

			void reference(void) const;
			void unreference(void) const;

			Glib::RefPtr<Gdk::Pixbuf> get_pixbuf(void);
			const Rect& get_bbox(void) const { return m_bbox; }
			double get_nmi_per_pixel(void) const;
			Projection::const_ptr_t get_proj(void) const { return m_projection; }
			const std::string& get_file(void) const { return m_file; }
			unsigned int get_width(void) const { return m_width; }
			unsigned int get_height(void) const { return m_height; }
			unsigned int get_xmin(void) const { return m_xmin; }
			unsigned int get_ymin(void) const { return m_ymin; }
			unsigned int get_xmax(void) const { return m_xmax; }
			unsigned int get_ymax(void) const { return m_ymax; }
			time_t get_time(void) const { return m_time; }

			void flush_cache(void);
			bool check_file(void) const;
			int load_xml(const xmlpp::Element *el, const std::string& name);

		  protected:
			void zap_border(void);

			georef_t m_georef;
			Projection::ptr_t m_projection;
			Glib::RefPtr<Gdk::Pixbuf> m_pixbufcache;
			std::string m_file;
			Rect m_bbox;
			mutable gint m_refcount;
			time_t m_time;
			unsigned int m_width;
			unsigned int m_height;
			unsigned int m_xmin;
			unsigned int m_ymin;
			unsigned int m_xmax;
			unsigned int m_ymax;
		};

		Map(void);
		~Map();

                void reference(void) const;
                void unreference(void) const;

		bool empty(void) const { return m_tiles.empty(); }
		unsigned int size(void) const { return m_tiles.size(); }
		Tile::ptr_t operator[](unsigned int x);
		Tile::const_ptr_t operator[](unsigned int x) const;

		double get_nmi_per_pixel(void) const;
		const std::string& get_name(void) const { return m_name; }
		unsigned int get_scale(void) const { return m_scale; }
		time_t get_time(void) const;
		Rect get_bbox(void) const;

		void flush_cache(void);
		bool check_file(void) const;
		void clear_nonexisting_tiles(void);
		int load_xml(const xmlpp::Element *el);

	  protected:
		typedef std::vector<Tile::ptr_t> tiles_t;
		tiles_t m_tiles;
		std::string m_name;
                mutable gint m_refcount;
		unsigned int m_scale;
	};

	BitmapMaps(void);
	unsigned int size(void) const;
	bool empty(void) const;
	Map::ptr_t operator[](unsigned int x) const;
	unsigned int find_index(const Map::const_ptr_t& p) const;
	unsigned int find_index(const Map::ptr_t& p) const;
	unsigned int find_index(const std::string& nm) const;
	int parse_file(const std::string& fn);
	int parse_directory(const std::string& dir);
	int parse(const std::string& p);
	void sort(void);
	void check_files(void);

  protected:
	typedef std::vector<Map::ptr_t> maps_t;
	maps_t m_maps;
	mutable Glib::Mutex m_mutex;
};

#endif /* BITMAPMAPS_H */
