//
// C++ Interface: maps
//
// Description: Maps Drawing, Non-Threaded
//
//
// Author: Thomas Sailer <t.sailer@alumni.ethz.ch>, (C) 2007, 2008, 2012, 2013, 2015
//
// Copyright: See COPYING file that comes with this distribution
//
//

#ifndef MAPSNT_H
#define MAPSNT_H

#include <limits>
#include <glibmm.h>
#include <gtkmm/drawingarea.h>
#include <gtkmm/offscreenwindow.h>
#include <sigc++/sigc++.h>
#include <libsoup/soup.h>

#ifdef HAVE_GTKMM2
#include <libglademm.h>
typedef Gnome::Glade::Xml Builder;
#endif

#ifdef HAVE_GTKMM3
#include <gtkmm/builder.h>
typedef Gtk::Builder Builder;
#endif

#include "engine.h"
#include "bitmapmaps.h"

class GRIB2;

class MapRenderer : public sigc::trackable {
        public:
                typedef enum {
                                drawflags_none = 0,
                                drawflags_topo = 1 << 0,
                                drawflags_terrain = 1 << 1,
                                drawflags_terrain_names = 1 << 2,
                                drawflags_terrain_borders = 1 << 3,
				drawflags_terrain_all = drawflags_terrain | drawflags_terrain_names | drawflags_terrain_borders,
				drawflags_navaids_vor = 1 << 4,
                                drawflags_navaids_dme = 1 << 5,
                                drawflags_navaids_ndb = 1 << 6,
                                drawflags_navaids = drawflags_navaids_vor | drawflags_navaids_dme | drawflags_navaids_ndb,
                                drawflags_waypoints_low = 1 << 7,
                                drawflags_waypoints_high = 1 << 8,
                                drawflags_waypoints_rnav = 1 << 9,
                                drawflags_waypoints_terminal = 1 << 10,
                                drawflags_waypoints_vfr = 1 << 11,
                                drawflags_waypoints_user = 1 << 12,
                                drawflags_waypoints = drawflags_waypoints_low | drawflags_waypoints_high | drawflags_waypoints_rnav | drawflags_waypoints_terminal | drawflags_waypoints_vfr | drawflags_waypoints_user,
                                drawflags_airports_civ = 1 << 13,
                                drawflags_airports_mil = 1 << 14,
                                drawflags_airports = drawflags_airports_civ | drawflags_airports_mil,
                                drawflags_airports_localizercone = 1 << 15,
                                drawflags_airspaces_ab = 1 << 16,
                                drawflags_airspaces_cd = 1 << 17,
                                drawflags_airspaces_efg = 1 << 18,
                                drawflags_airspaces_specialuse = 1 << 19,
                                drawflags_airspaces = drawflags_airspaces_ab | drawflags_airspaces_cd | drawflags_airspaces_efg | drawflags_airspaces_specialuse,
                                drawflags_airspaces_fill_ground = 1 << 20,
                                drawflags_route = 1 << 21,
                                drawflags_route_labels = 1 << 22,
                                drawflags_track = 1 << 23,
                                drawflags_track_points = 1 << 24,
                                drawflags_airways_low = 1 << 25,
                                drawflags_airways_high = 1 << 26,
				drawflags_northpointer = 1 << 27,
				drawflags_weather = 1 << 28,
                                drawflags_all = drawflags_topo | drawflags_terrain | drawflags_terrain_names | drawflags_terrain_borders |
                                                drawflags_navaids | drawflags_waypoints | drawflags_airports | drawflags_airports_localizercone |
                                                drawflags_airspaces | drawflags_airspaces_fill_ground |
                                                drawflags_route | drawflags_route_labels | drawflags_track | drawflags_track_points |
                                                drawflags_airways_low | drawflags_airways_high | drawflags_northpointer | drawflags_weather
                } DrawFlags;

                template <typename T> class ScreenCoordTemplate {
		  public:
			typedef T scalar_t;
			ScreenCoordTemplate(T xx = 0, T yy = 0) : x(xx), y(yy) {}
			T getx(void) const { return x; }
			T gety(void) const { return y; }
			void setx(T xx = 0) { x = xx; }
			void sety(T yy = 0) { y = yy; }
			bool operator==(const ScreenCoordTemplate &p) const { return x == p.x && y == p.y; }
			bool operator!=(const ScreenCoordTemplate &p) const { return !operator==(p); }
			ScreenCoordTemplate operator+(const ScreenCoordTemplate &p) const { return ScreenCoordTemplate(x + p.x, y + p.y); }
			ScreenCoordTemplate operator-(const ScreenCoordTemplate &p) const { return ScreenCoordTemplate(x - p.x, y - p.y); }
			ScreenCoordTemplate operator-() const { return ScreenCoordTemplate(-x, -y); }
			ScreenCoordTemplate& operator+=(const ScreenCoordTemplate &p) { x += p.x; y += p.y; return *this; }
			ScreenCoordTemplate& operator-=(const ScreenCoordTemplate &p) { x -= p.x; y -= p.y; return *this; }
			bool operator<(const ScreenCoordTemplate &p) const { return x < p.x && y < p.y; }
			bool operator<=(const ScreenCoordTemplate &p) const { return x <= p.x && y <= p.y; }
			bool operator>(const ScreenCoordTemplate &p) const { return x > p.x && y > p.y; }
			bool operator>=(const ScreenCoordTemplate &p) const { return x >= p.x && y >= p.y; }
			ScreenCoordTemplate operator*(T z) const { return ScreenCoordTemplate(x * z, y * z); }
			ScreenCoordTemplate& operator*=(T z) { x *= z; y *= z; return *this; }
			ScreenCoordTemplate operator/(T z) const { return ScreenCoordTemplate(x / z, y / z); }
			ScreenCoordTemplate& operator/=(T z) { x /= z; y /= z; return *this; }
		  private:
			T x, y;
                };
                typedef ScreenCoordTemplate<int> ScreenCoord;
                typedef ScreenCoordTemplate<float> ScreenCoordFloat;

		static ScreenCoordFloat to_screencoordfloat(const ScreenCoord& sci) { return ScreenCoordFloat(sci.getx(), sci.gety()); }
		static ScreenCoord to_screencoord(const ScreenCoordFloat& scf) { return ScreenCoord(Point::round<ScreenCoord::scalar_t,ScreenCoordFloat::scalar_t>(scf.getx()),
												    Point::round<ScreenCoord::scalar_t,ScreenCoordFloat::scalar_t>(scf.gety())); }
		static ScreenCoordFloat rotate(const ScreenCoordFloat& sc, uint16_t angle);

		static constexpr float to_deg_16bit = 90.0 / (1U << 14);
		static constexpr float to_rad_16bit = M_PI / (1U << 15);
		static constexpr float from_deg_16bit = (1U << 14) / 90.0;
		static constexpr float from_rad_16bit = (1U << 15) / M_PI;

		static constexpr double to_deg_16bit_dbl = 90.0 / (1U << 14);
		static constexpr double to_rad_16bit_dbl = M_PI / (1U << 15);
		static constexpr double from_deg_16bit_dbl = (1U << 14) / 90.0;
		static constexpr double from_rad_16bit_dbl = (1U << 15) / M_PI;

		template <typename T> class ImageBuffer {
		  public:
			typedef T float_t;

			static constexpr float_t min_scale = 0.1f/20;
			static constexpr float_t max_scale = 1000.0f/20;
			static constexpr float_t arpt_min_scale = 0.01f/20;
			static constexpr float_t arpt_max_scale = 100.0f/20;

			ImageBuffer(void);
			ImageBuffer(const ScreenCoord& imgsize, const Point& center, float_t nmi_per_pixel = 0.1, uint16_t upangle = 0,
				    int alt = 0, int maxalt = std::numeric_limits<int>::max(), int64_t time = 0, float_t glideslope = std::numeric_limits<float_t>::quiet_NaN(),
				    float_t tas = 0, float_t winddir = 0, float_t windspeed = 0, DrawFlags flags = drawflags_none, const ScreenCoord& offset = ScreenCoord(0, 0));
			operator bool(void) const { return !!m_surface; }
			void reset(void);
			void reset(const ScreenCoord& imgsize, const Point& center, float_t nmi_per_pixel = 0.1, uint16_t upangle = 0, 
				   int alt = 0, int maxalt = std::numeric_limits<int>::max(), int64_t time = 0, float_t glideslope = std::numeric_limits<float_t>::quiet_NaN(),
				   float_t tas = 0, float_t winddir = 0, float_t windspeed = 0, DrawFlags flags = drawflags_none, const ScreenCoord& offset = ScreenCoord(0, 0));
			void flush_surface(void);
			const ScreenCoord& get_imagesize(void) const { return m_imgsize; }
			const Point& get_center(void) const { return m_center; }
			const Point& get_imagecenter(void) const { return m_imgcenter; }
			float_t get_scale(void) const { return m_scale; }
			uint16_t get_upangle(void) const { return m_upangle; }
			int get_altitude(void) const { return m_alt; }
			int get_maxalt(void) const { return m_maxalt; }
			int64_t get_time(void) const { return m_time; }
			float_t get_glideslope(void) const { return m_glideslope; }
			float_t get_tas(void) const { return m_tas; }
			float_t get_winddir(void) const { return m_winddir; }
			float_t get_windspeed(void) const { return m_windspeed; }
			DrawFlags get_drawflags(void) const { return m_drawflags; }
			Cairo::RefPtr<Cairo::Surface> get_surface(void) { return m_surface; }
			Cairo::RefPtr<Cairo::Context> create_context(void);
			Cairo::RefPtr<Cairo::SurfacePattern> create_pattern(void);
			Rect coverrect(float_t xmul = 1, float_t ymul = 1) const;
			Cairo::Matrix cairo_matrix(void) const;
			Cairo::Matrix cairo_matrix(const Point& origin) const;
			Cairo::Matrix cairo_matrix_inverse(void) const;
			ScreenCoordTemplate<float_t> operator()(const Point& pt) const;
			Point operator()(const ScreenCoord& sc) const;
			Point operator()(const ScreenCoordFloat& sc) const;
 			Point operator()(float_t x, float_t y) const;
			Point transform_distance(const ScreenCoord& sc) const;
			Point transform_distance(const ScreenCoordFloat& sc) const;
			Point transform_distance(float_t x, float_t y) const;
			ScreenCoordTemplate<float_t> transform_distance(const Point& pt) const;
			inline void move_to(Cairo::RefPtr<Cairo::Context> ctxt, const Point& pt) const { ScreenCoordTemplate<float_t> c((*this)(pt)); ctxt->move_to(c.getx(), c.gety()); }
			inline void line_to(Cairo::RefPtr<Cairo::Context> ctxt, const Point& pt) const { ScreenCoordTemplate<float_t> c((*this)(pt)); ctxt->line_to(c.getx(), c.gety()); }
			inline void translate(Cairo::RefPtr<Cairo::Context> ctxt, const Point& pt) const { ScreenCoordTemplate<float_t> c((*this)(pt)); ctxt->translate(c.getx(), c.gety()); }
			template <typename IT> inline void path(Cairo::RefPtr<Cairo::Context> ctxt, IT b, IT e) const {
				if (b == e)
					return;
				move_to(ctxt, *b);
				for (++b; b != e; ++b)
					line_to(ctxt, *b);
			}
			inline void move_to(Cairo::Context *ctxt, const Point& pt) const { ScreenCoordTemplate<float_t> c((*this)(pt)); ctxt->move_to(c.getx(), c.gety()); }
			inline void line_to(Cairo::Context *ctxt, const Point& pt) const { ScreenCoordTemplate<float_t> c((*this)(pt)); ctxt->line_to(c.getx(), c.gety()); }
			inline void translate(Cairo::Context *ctxt, const Point& pt) const { ScreenCoordTemplate<float_t> c((*this)(pt)); ctxt->translate(c.getx(), c.gety()); }
			template <typename IT> inline void path(Cairo::Context *ctxt, IT b, IT e) const {
				if (b == e)
					return;
				move_to(ctxt, *b);
				for (++b; b != e; ++b)
					line_to(ctxt, *b);
			}
			float_t getmatrix(unsigned int row, unsigned int col) const { return m_matrix[((row & 1) << 1) | (col & 1)]; }
			float_t getinvmatrix(unsigned int row, unsigned int col) const { return m_invmatrix[((row & 1) << 1) | (col & 1)]; }

		  protected:
			Cairo::RefPtr<Cairo::ImageSurface> m_surface;
			ScreenCoord m_imgsize;
			Point m_center;
			Point m_imgcenter;
			float_t m_scale;
			float_t m_glideslope;
			float_t m_tas;
			float_t m_winddir;
			float_t m_windspeed;
			float_t m_matrix[4];
			float_t m_invmatrix[4];
			int m_alt;
			int m_maxalt;
			int64_t m_time;
			uint16_t m_upangle;
			DrawFlags m_drawflags;
			// Transforms work like this:
			// [ x ; y ; 1 ] = M * [lat - latcenter; lon - loncenter; 1 ]
			// [ x ; y ] = A * [lat - latcenter; lon - loncenter ] + t
			// inverse: [lat - latcenter ; lon - loncenter ] = inv(A) * ([ x ; y ] - t)
			// M = [ A  t ; 0 0 1 ]  (matrix, consisting of affine and translation block matrices)
			// A = [ Axx Axy ; Ayx Ayy ]  (affine transformation matrix)
			// t = [ tx ; ty ]

			Point transform_inv(float_t x, float_t y) const;
		};

                MapRenderer(Engine& eng, const ScreenCoord& scrsize = ScreenCoord(0, 0), const Point& center = Point(), int alt = 0, uint16_t upangle = 0, int64_t time = 0);
                virtual ~MapRenderer() {}
                virtual void set_route(const FPlanRoute *route) { m_route = route; }
                virtual void set_track(const TracksDb::Track *track) { m_track = track; }
                virtual void redraw_route(void) = 0;
                virtual void redraw_track(void) = 0;
                Point get_center(void) const { return m_center; }
                int get_altitude(void) const { return m_altitude; }
		uint16_t get_upangle(void) const { return m_upangle; }
		int64_t get_time(void) const { return m_time; }
                virtual void set_center(const Point& pt, int alt, uint16_t upangle = 0, int64_t time = 0) { m_center = pt; m_altitude = alt; m_upangle = upangle; m_time = time; }
                ScreenCoord get_screensize(void) const { return m_screensize; }
                virtual void set_screensize(const ScreenCoord& scrsize) { m_screensize = scrsize; }
                virtual void set_map_scale(float nmi_per_pixel) = 0;
                virtual float get_map_scale(void) const { return 1; }
                virtual void zoom_out(void) { set_map_scale(get_map_scale() * 2.0); }
                virtual void zoom_in(void) { set_map_scale(get_map_scale() * 0.5); }
                virtual void pan_up(void);
                virtual void pan_down(void);
                virtual void pan_left(void);
                virtual void pan_right(void);
                virtual void set_maxalt(int alt = std::numeric_limits<int>::max()) { }
                virtual int get_maxalt(void) const { return std::numeric_limits<int>::max(); }
                virtual void set_glideslope(float gs = std::numeric_limits<float>::quiet_NaN()) = 0;
                virtual float get_glideslope(void) const { return std::numeric_limits<float>::quiet_NaN(); }
                virtual void set_tas(float tas = std::numeric_limits<float>::quiet_NaN()) = 0;
                virtual float get_tas(void) const { return std::numeric_limits<float>::quiet_NaN(); }
		virtual void set_wind(float dir = 0, float speed = 0) = 0;
                virtual float get_winddir(void) const { return 0; }
		virtual float get_windspeed(void) const { return 0; }
		virtual operator DrawFlags() const = 0;
                virtual MapRenderer& operator=(DrawFlags f) = 0;
                virtual MapRenderer& operator&=(DrawFlags f) = 0;
                virtual MapRenderer& operator|=(DrawFlags f) = 0;
                virtual MapRenderer& operator^=(DrawFlags f) = 0;
		virtual void set_grib2(const Glib::RefPtr<GRIB2::Layer>& layer) = 0;
      		virtual void set_bitmap(const Glib::RefPtr<BitmapMaps::Map>& map = Glib::RefPtr<BitmapMaps::Map>()) = 0;
		virtual void airspaces_changed(void) = 0;
                virtual void airports_changed(void) = 0;
                virtual void navaids_changed(void) = 0;
                virtual void waypoints_changed(void) = 0;
                virtual void airways_changed(void) = 0;
                virtual void draw(const Cairo::RefPtr<Cairo::Context>& cr, ScreenCoord offs = ScreenCoord(0, 0));
                virtual Point screen_coord(const ScreenCoord& sc) = 0;
                virtual Point screen_coord(const ScreenCoordFloat& sc) = 0;
                virtual ScreenCoord coord_screen(const Point& pt) = 0;
                virtual void hide(void) {}
                virtual void show(void) {}
                virtual sigc::connection connect_update(const sigc::slot<void>& slot) = 0;

        protected:
                Point m_center;
		int64_t m_time;
                int m_altitude;
		uint16_t m_upangle;
                Engine& m_engine;
                const FPlanRoute *m_route;
                const TracksDb::Track *m_track;
                ScreenCoord m_screensize;
};

template<typename T> std::ostream& operator<<(std::ostream& os, const MapRenderer::ScreenCoordTemplate<T>& sc);

class VectorMapRenderer : public MapRenderer {
        public:
                class TimeMeasurement {
                        public:
                                TimeMeasurement(bool start = true);
                                void start(void);
                                void stop(void);
                                operator unsigned int();
                                std::ostream& print(std::ostream& os);

                        private:
                                Glib::TimeVal m_start, m_stop;
                                bool m_running;
                };
                friend std::ostream& operator<<(std::ostream& os, VectorMapRenderer::TimeMeasurement& tm);

		class GRIB2Layer {
		  public:
			GRIB2Layer(void);
			~GRIB2Layer(void);
			void reference(void) const;
			void unreference(void) const;
			bool compute(Glib::RefPtr<GRIB2::Layer> layer = Glib::RefPtr<GRIB2::Layer>(), const Rect& bbox = Rect());
			void draw_scale(Cairo::RefPtr<Cairo::Context> cr);
			static void jet(uint8_t d[3], float x);
			const Glib::RefPtr<GRIB2::Layer>& get_layer(void) const { return m_layer; }
			const Cairo::RefPtr<Cairo::ImageSurface>& get_surface(void) const { return m_surface; }
			const Rect& get_bbox(void) const { return m_bbox; }
			float get_valmin(void) const { return m_valmin; }
			float get_valmax(void) const { return m_valmax; }

		  protected:
			mutable gint m_refcount;
			Glib::RefPtr<GRIB2::Layer> m_layer;
			Cairo::RefPtr<Cairo::ImageSurface> m_surface;
			Rect m_bbox;
			float m_valmin;
			float m_valmax;
		};

        private:
		class Corner {
		  public:
			Corner(void);
			Point& operator[](unsigned int i) { return m_pt[i & 3]; }
			const Point& operator[](unsigned int i) const { return m_pt[i & 3]; }
			operator Rect() const;
			
		  protected:
			Point m_pt[4];
		};

		class DrawThread : public sigc::trackable {
                        public:
			        DrawThread(Engine& eng, Glib::Dispatcher& dispatch, const ScreenCoord& scrsize = ScreenCoord(0, 0), const Point& center = Point(), int alt = 0, uint16_t upangle = 0, int64_t time = 0);
                                virtual ~DrawThread();

				const Point& get_center(void) const { return m_center; }
				void set_center(const Point& pt);
				void set_center(const Point& pt, int alt);
				void set_center(const Point& pt, int alt, uint16_t upangle);
				void set_center(const Point& pt, int alt, uint16_t upangle, int64_t time);
				const ScreenCoord& get_screensize(void) const { return m_screensize; }
				void set_screensize(const ScreenCoord& scrsize);
				const ScreenCoord& get_offset(void) const { return m_offset; }
				void set_offset(const ScreenCoord& offs);
				uint16_t get_upangle(void) const { return m_upangle; }
				void set_upangle(uint16_t upangle = 0);
				int64_t get_time(void) const { return m_time; }
				void set_time(int64_t time = 0);
                                virtual void set_map_scale(float nmi_per_pixel);
                                float get_map_scale(void) const { return m_scale; }
				virtual std::pair<float,float> get_map_scale_bounds(void) const {
					return std::pair<float,float>(ImageBuffer<float>::min_scale, ImageBuffer<float>::max_scale);
				}
				int get_altitude(void) const { return m_altitude; }
				void set_altitude(int alt);
				int get_maxalt(void) const { return m_maxalt; }
				void set_maxalt(int alt = std::numeric_limits<int>::max()) { m_maxalt = alt; }
				DrawFlags get_drawflags(void) const { return m_drawflags; }
				DrawFlags set_drawflags(DrawFlags df);
				float get_glideslope(void) const { return m_glideslope; }
				void set_glideslope(float gs = std::numeric_limits<float>::quiet_NaN());
				float get_tas(void) const { return m_tas; }
				void set_tas(float tas = std::numeric_limits<float>::quiet_NaN());
				float get_winddir(void) const { return m_winddir; }
				float get_windspeed(void) const { return m_windspeed; }
				void set_wind(float winddir = 0, float windspeed = 0);
				bool is_hidden(void) const { return m_hidden; }
				void set_hidden(bool hidden);

                                void hide(void) { set_hidden(true); }
                                void show(void) { set_hidden(false); }

				void set_grib2(const Glib::RefPtr<GRIB2::Layer>& layer);
				virtual void set_bitmap(const Glib::RefPtr<BitmapMaps::Map>& map = Glib::RefPtr<BitmapMaps::Map>()) {}

				void draw(const Cairo::RefPtr<Cairo::Context>& cr);

                                void draw_scale(Cairo::Context *cr, float target_pixel = 100);
				void draw_north(Cairo::Context *cr);
                                void draw_route(Cairo::Context *cr, const FPlanRoute *route);
                                void draw_track(Cairo::Context *cr, const TracksDb::Track *track);

                                Point screen_coord(const ScreenCoord& sc) const { return (*this)(sc); }
                                Point screen_coord(const ScreenCoordFloat& sc) const { return (*this)(sc); }
                                ScreenCoord coord_screen(const Point& pt) const { return to_screencoord((*this)(pt)); }

                                sigc::connection connect_update(const sigc::slot<void>& slot) { return m_dispatch.connect(slot); }

                                virtual void airspaces_changed(void);
                                virtual void airports_changed(void);
                                virtual void navaids_changed(void);
                                virtual void waypoints_changed(void);
                                virtual void airways_changed(void);

                        protected:
                                class AirportRoutePoint {
                                        public:
                                                AirportRoutePoint(const Glib::ustring& name = "", const Point& coord = Point(), AirportsDb::Airport::label_placement_t lp = AirportsDb::Airport::label_off);
                                                AirportRoutePoint(const AirportsDb::Airport::VFRRoute::VFRRoutePoint& pt);
                                                bool operator<(const AirportRoutePoint& p) const;
                                                Glib::ustring get_name(void) const { return m_name; }
                                                const Point& get_coord(void) const { return m_coord; }
                                                AirportsDb::Airport::label_placement_t get_label_placement(void) const { return m_labelplacement; }

                                        protected:
                                                Glib::ustring m_name;
                                                Point m_coord;
                                                AirportsDb::Airport::label_placement_t m_labelplacement;
                                };

				static const Rect empty_rect;

                                // variables shared by the rendering and the gui threads
				Engine& m_engine;
				Glib::RefPtr<GRIB2::Layer> m_grib2layer;
				Glib::RefPtr<GRIB2Layer> m_grib2surface;

                        private:
                                Glib::Dispatcher& m_dispatch;
                                Glib::Dispatcher m_dbdispatch;
				Rect m_dbrectangle;

				// owned by draw thread
				ImageBuffer<float> m_drawbuf;
				// owned by main thread
				ImageBuffer<float> m_screenbuf;
				Point m_center;
				ScreenCoord m_screensize;
				ScreenCoord m_offset;
				ScreenCoord m_drawoffset; // this is integer on purpose - avoid resampling
				int64_t m_time;
				float m_scale;
				float m_glideslope;
				float m_tas;
				float m_winddir;
				float m_windspeed;
				int m_altitude;
				int m_maxalt;
				DrawFlags m_drawflags;
				uint16_t m_upangle;
				typedef enum {
					thread_idle,
					thread_busy,
					thread_terminate
				} threadstate_t;
				threadstate_t m_threadstate;
				bool m_hidden;

				ScreenCoordFloat get_screencenter(void) const { return to_screencoordfloat(get_screensize()) * 0.5; }
				void update_drawoffset(void);
				ScreenCoord calc_image_drawoffset(const ImageBuffer<float>& imgbuf) const;
				bool check_inside(const ImageBuffer<float>& imgbuf) const;
				void check_restart(void);
				template <typename Ts> static inline bool check_window(Ts x, Ts a, Ts b) { return a <= x && x <= b; }
				template <typename Ts> static inline bool check_window_relative(Ts x, Ts a) { return check_window(x, ((Ts)1) / (a + (Ts)1), a + (Ts)1); }
				template <typename Ts> static inline bool check_window_relative_or_nan(Ts x, Ts a) { return std::isnan(x) || check_window_relative(x, a); }
				void dbdone(void);

                       protected:
 				// functions to control drawing state machine
                                void async_done(void);
				void terminate(void);
				void restart(void);
                                void restart_db(void);

                                virtual void draw_restart(bool dbquery) = 0;
                                virtual void draw_iterate(void) = 0;
				const Rect& get_dbrectangle(void) const { return m_dbrectangle; }
				void clear_dbrectangle(void) { m_dbrectangle = empty_rect; }
				void recompute_dbrectangle(void);
				virtual bool need_glideslope(void) { return false; }
				virtual bool need_altitude(void) { return false; }

                                static Glib::ustring freqstr(uint32_t f = 0);

                                float nmi_to_pixel(float nmi) const;

				// coordinate transformation
				ScreenCoordFloat operator()(const Point& pt) const;
				Point operator()(const ScreenCoord& sc) const;
				Point operator()(const ScreenCoordFloat& sc) const;
				Point operator()(float_t x, float_t y) const;

				float get_screen_scale(void) const { return m_screenbuf.get_scale(); }

				inline void move_to(Cairo::RefPtr<Cairo::Context> ctxt, const Point& pt) const { ScreenCoordFloat c((*this)(pt)); ctxt->move_to(c.getx(), c.gety()); }
				inline void line_to(Cairo::RefPtr<Cairo::Context> ctxt, const Point& pt) const { ScreenCoordFloat c((*this)(pt)); ctxt->line_to(c.getx(), c.gety()); }
				inline void translate(Cairo::RefPtr<Cairo::Context> ctxt, const Point& pt) const { ScreenCoordFloat c((*this)(pt)); ctxt->translate(c.getx(), c.gety()); }
				template <typename IT> inline void path(Cairo::RefPtr<Cairo::Context> ctxt, IT b, IT e) const {
					if (b == e)
						return;
					draw_move_to(ctxt, *b);
					for (++b; b != e; ++b)
						draw_line_to(ctxt, *b);
				}
				inline void move_to(Cairo::Context *ctxt, const Point& pt) const { ScreenCoordFloat c((*this)(pt)); ctxt->move_to(c.getx(), c.gety()); }
				inline void line_to(Cairo::Context *ctxt, const Point& pt) const { ScreenCoordFloat c((*this)(pt)); ctxt->line_to(c.getx(), c.gety()); }
				inline void translate(Cairo::Context *ctxt, const Point& pt) const { ScreenCoordFloat c((*this)(pt)); ctxt->translate(c.getx(), c.gety()); }
				template <typename IT> inline void path(Cairo::Context *ctxt, IT b, IT e) const {
					if (b == e)
						return;
					draw_move_to(ctxt, *b);
					for (++b; b != e; ++b)
						draw_line_to(ctxt, *b);
				}
				inline const ScreenCoord& screen_get_imagesize(void) const { return m_screenbuf.get_imagesize(); }
				inline const Point& screen_get_imagecenter(void) const { return m_screenbuf.get_imagecenter(); }
				float screen_get_scale(void) const { return m_screenbuf.get_scale(); }
				uint16_t screen_get_upangle(void) const { return m_screenbuf.get_upangle(); }
				int screen_get_altitude(void) const { return m_screenbuf.get_altitude(); }
				int screen_get_maxalt(void) const { return m_screenbuf.get_maxalt(); }
				DrawFlags screen_get_drawflags(void) const { return m_screenbuf.get_drawflags(); }

				// draw thread commands
				bool draw_checkabort(void);
				void draw_done(void);
				inline Cairo::RefPtr<Cairo::Context> draw_create_context(void) { return m_drawbuf.create_context(); }
				Cairo::Matrix draw_cairo_matrix(void) const { return m_drawbuf.cairo_matrix(); }
				Cairo::Matrix draw_cairo_matrix(const Point& origin) const { return m_drawbuf.cairo_matrix(origin); }
				Cairo::Matrix draw_cairo_matrix_inverse(void) const { return m_drawbuf.cairo_matrix_inverse(); }
				inline ScreenCoordFloat draw_transform(const Point& pt) const { return m_drawbuf(pt); }
				Point draw_transform_distance(const ScreenCoord& sc) const { return m_drawbuf.transform_distance(sc); }
				Point draw_transform_distance(const ScreenCoordFloat& sc) const { return m_drawbuf.transform_distance(sc); }
				Point draw_transform_distance(float_t x, float_t y) const { return m_drawbuf.transform_distance(x, y); }
				ScreenCoordFloat draw_transform_distance(const Point& pt) const { return m_drawbuf.transform_distance(pt); }
				inline void draw_move_to(Cairo::RefPtr<Cairo::Context> ctxt, const Point& pt) const { m_drawbuf.move_to(ctxt, pt); }
				inline void draw_line_to(Cairo::RefPtr<Cairo::Context> ctxt, const Point& pt) const { m_drawbuf.line_to(ctxt, pt); }
				inline void draw_translate(Cairo::RefPtr<Cairo::Context> ctxt, const Point& pt) const { m_drawbuf.translate(ctxt, pt); }
				template <typename IT> inline void draw_path(Cairo::RefPtr<Cairo::Context> ctxt, IT b, IT e) const { m_drawbuf.path(ctxt, b, e); }
				inline void draw_move_to(Cairo::Context *ctxt, const Point& pt) const { m_drawbuf.move_to(ctxt, pt); }
				inline void draw_line_to(Cairo::Context *ctxt, const Point& pt) const { m_drawbuf.line_to(ctxt, pt); }
				inline void draw_translate(Cairo::Context *ctxt, const Point& pt) const { m_drawbuf.translate(ctxt, pt); }
				template <typename IT> inline void draw_path(Cairo::Context *ctxt, IT b, IT e) const { m_drawbuf.path(ctxt, b, e); }
				inline const ScreenCoord& draw_get_imagesize(void) const { return m_drawbuf.get_imagesize(); }
				inline const Point& draw_get_imagecenter(void) const { return m_drawbuf.get_imagecenter(); }
				inline const Point& draw_get_center(void) const { return m_drawbuf.get_center(); }
				float draw_get_scale(void) const { return m_drawbuf.get_scale(); }
				uint16_t draw_get_upangle(void) const { return m_drawbuf.get_upangle(); }
				int draw_get_altitude(void) const { return m_drawbuf.get_altitude(); }
				int draw_get_maxalt(void) const { return m_drawbuf.get_maxalt(); }
				int64_t draw_get_time(void) const { return m_drawbuf.get_time(); }
				float draw_get_glideslope(void) const { return m_drawbuf.get_glideslope(); }
				float draw_get_tas(void) const { return m_drawbuf.get_tas(); }
				float draw_get_winddir(void) const { return m_drawbuf.get_winddir(); }
				float draw_get_windspeed(void) const { return m_drawbuf.get_windspeed(); }
				DrawFlags draw_get_drawflags(void) const { return m_drawbuf.get_drawflags(); }
				Rect draw_coverrect(float_t xmul = 1, float_t ymul = 1) const { return m_drawbuf.coverrect(xmul, ymul); }

                                void polypath(Cairo::Context *cr, const PolygonSimple& poly, bool closeit = false);
                                void polypath(Cairo::Context *cr, const PolygonHole& poly, bool closeit = false);
                                void polypath(Cairo::Context *cr, const MultiPolygonHole& poly, bool closeit = false);
                                void draw_surface(Cairo::Context *cr, const Rect& cbbox, const Cairo::RefPtr<Cairo::ImageSurface>& pbuf, double alpha);
                                void draw_pixbuf(const Cairo::RefPtr<Cairo::Context>& cr, const Rect& cbbox, const Glib::RefPtr<Gdk::Pixbuf>& pbuf, double alpha);
                                void draw_pixbuf(const Cairo::RefPtr<Cairo::Context>& cr, const Corner& corner, const Glib::RefPtr<Gdk::Pixbuf>& pbuf, double alpha);
                                void draw_waypoint(Cairo::Context *cr, const Point& pt, const Glib::ustring& str, bool mandatory = true,
						   WaypointsDb::Waypoint::label_placement_t lp = WaypointsDb::Waypoint::label_e, bool setcolor = true);
                                void draw_label(Cairo::Context *cr, const Glib::ustring& text, const Point& pt, NavaidsDb::Navaid::label_placement_t lp = NavaidsDb::Navaid::label_center, int dist = 2);
                                void draw_label(Cairo::Context *cr, const Glib::ustring& text, const ScreenCoordFloat& pos, NavaidsDb::Navaid::label_placement_t lp, int dist, uint16_t upangle);
                                void draw_airport_vfrreppt(Cairo::Context *cr, const AirportsDb::Airport& arpt);
                                void draw_navaid(Cairo::Context *cr, const NavaidsDb::Navaid& n, const ScreenCoordFloat& pos);
				virtual void draw_wxinfo(Cairo::Context *cr) {}
                };

                class DrawThreadOverlay : public DrawThread {
                        public:
                                DrawThreadOverlay(Engine& eng, Glib::Dispatcher& dispatch, const ScreenCoord& scrsize = ScreenCoord(0, 0), const Point& center = Point(), int alt = 0, uint16_t upangle = 0, int64_t time = 0);
                                virtual ~DrawThreadOverlay();

                                virtual void airspaces_changed(void);
                                virtual void airports_changed(void);
                                virtual void navaids_changed(void);
                                virtual void waypoints_changed(void);
                                virtual void airways_changed(void);

				static bool is_ground(const AirspacesDb::Airspace& a);

                                static constexpr float airport_loccone_centerlength = 5.5;
                                static constexpr float airport_loccone_sidelength = 6.0;
                                static constexpr float airport_loccone_sideangle = 5.0;

                        protected:
                                Glib::RefPtr<Engine::NavaidResult> m_navaidquery;
                                Glib::RefPtr<Engine::WaypointResult> m_waypointquery;
                                Glib::RefPtr<Engine::AirportResult> m_airportquery;
                                Glib::RefPtr<Engine::AirspaceResult> m_airspacequery;
                                Glib::RefPtr<Engine::AirwayResult> m_airwayquery;
				Glib::RefPtr<GRIB2::LayerResult> m_wxwindu;
				Glib::RefPtr<GRIB2::LayerResult> m_wxwindv;
				Glib::RefPtr<GRIB2::LayerResult> m_wxtemp;
				Glib::RefPtr<GRIB2::LayerResult> m_wxqff;
				Glib::RefPtr<GRIB2::LayerResult> m_wxrh;
				Glib::RefPtr<GRIB2::LayerResult> m_wxprecip;
				Glib::RefPtr<GRIB2::LayerResult> m_wxcldbdrycover;
				Glib::RefPtr<GRIB2::LayerResult> m_wxcldlowcover;
				Glib::RefPtr<GRIB2::LayerResult> m_wxcldlowbase;
				Glib::RefPtr<GRIB2::LayerResult> m_wxcldlowtop;
				Glib::RefPtr<GRIB2::LayerResult> m_wxcldmidcover;
				Glib::RefPtr<GRIB2::LayerResult> m_wxcldmidbase;
				Glib::RefPtr<GRIB2::LayerResult> m_wxcldmidtop;
				Glib::RefPtr<GRIB2::LayerResult> m_wxcldhighcover;
				Glib::RefPtr<GRIB2::LayerResult> m_wxcldhighbase;
				Glib::RefPtr<GRIB2::LayerResult> m_wxcldhightop;
				Glib::RefPtr<GRIB2::LayerResult> m_wxcldconvcover;
				Glib::RefPtr<GRIB2::LayerResult> m_wxcldconvbase;
				Glib::RefPtr<GRIB2::LayerResult> m_wxcldconvtop;
				gint64 m_wxreftime;
				gint64 m_wxefftime;
				int m_wxaltitude;

                                void async_cancel(void);
				virtual bool need_altitude(void) { return !!(draw_get_drawflags() & drawflags_weather); }
				bool need_dbquery(void);
				void db_restart(void);
				bool do_draw_navaids(Cairo::Context *cr);
				bool do_draw_waypoints(Cairo::Context *cr);
				bool do_draw_airports(Cairo::Context *cr);
				bool do_draw_airspaces(Cairo::Context *cr);
				bool do_draw_airways(Cairo::Context *cr);
				void do_draw_weather(Cairo::Context *cr);
				void do_draw_grib2layer(Cairo::Context *cr);
                                void draw(Cairo::Context *cr, const std::vector<NavaidsDb::Navaid>& navaids);
                                void draw(Cairo::Context *cr, const std::vector<AirspacesDb::Airspace>& airspaces);
                                void draw(Cairo::Context *cr, const std::vector<AirportsDb::Airport>& airports);
                                void draw(Cairo::Context *cr, const std::vector<WaypointsDb::Waypoint>& waypoints);
                                void draw(Cairo::Context *cr, const std::vector<AirwaysDb::Airway>& airways);
				void draw_weather(Cairo::Context *cr);
				virtual void draw_wxinfo(Cairo::Context *cr);
		};

                class DrawThreadVMap : public DrawThreadOverlay {
                        public:
                                DrawThreadVMap(Engine& eng, Glib::Dispatcher& dispatch, const ScreenCoord& scrsize = ScreenCoord(0, 0), const Point& center = Point(), int alt = 0, uint16_t upangle = 0, int64_t time = 0);
                                virtual ~DrawThreadVMap();

                        protected:
                                class mapel_render_comp : public std::binary_function<const MapelementsDb::Mapelement&, const MapelementsDb::Mapelement&, bool> {
                                        public:
                                                mapel_render_comp(void) {}
                                                bool operator()(const MapelementsDb::Mapelement&, const MapelementsDb::Mapelement&) const;
                                        private:
                                                Point pt;
                                };

                                // private variables of the rendering thread
                                Glib::RefPtr<Engine::ElevationMapCairoResult> m_topocairoquery;
                                Glib::RefPtr<Engine::MapelementResult> m_mapelementquery;

				typedef enum {
					drawstate_topo,
					drawstate_mapelements,
					drawstate_navaids,
					drawstate_waypoints,
					drawstate_airports,
					drawstate_airspaces,
					drawstate_airways,
					drawstate_weather,
					drawstate_grib2layer,
					drawstate_done
				} drawstate_t;
				drawstate_t m_drawstate;

                                void draw_restart(bool dbquery);
                                void draw_iterate(void);
                                void async_cancel(void);

                                void draw(Cairo::Context *cr, const MapelementsDb::Mapelement& mapel);
                                void draw_text(Cairo::Context *cr, const MapelementsDb::Mapelement& mapel);
                                static DrawFlags drawflags(const MapelementsDb::Mapelement& mapel) { return drawflags_terrain; }
		};

                class DrawThreadTerrain : public DrawThread {
                        public:
                                DrawThreadTerrain(Engine& eng, Glib::Dispatcher& dispatch, const ScreenCoord& scrsize = ScreenCoord(0, 0), const Point& center = Point(), int alt = 0, uint16_t upangle = 0, int64_t time = 0);
                                virtual ~DrawThreadTerrain();

                                virtual void airspaces_changed(void);
                                virtual void airports_changed(void);
                                virtual void navaids_changed(void);
                                virtual void waypoints_changed(void);
                                virtual void airways_changed(void);

                        protected:
				int m_current_surfacealtitude;
                                Glib::RefPtr<Engine::ElevationMapResult> m_topoquery;
                                Glib::RefPtr<Engine::NavaidResult> m_navaidquery;
                                Glib::RefPtr<Engine::WaypointResult> m_waypointquery;
                                Glib::RefPtr<Engine::AirportResult> m_airportquery;

				typedef enum {
					drawstate_topo,
					drawstate_navaids,
					drawstate_waypoints,
					drawstate_airports,
					drawstate_done
				} drawstate_t;
				drawstate_t m_drawstate;

                                bool check_abort(void);
                                void draw_restart(bool dbquery);
                                void draw_iterate(void);
                                void async_cancel(void);
				virtual bool need_glideslope(void) { return true; }
				virtual bool need_altitude(void) { return true; }

                                void draw(Cairo::Context *cr, const std::vector<NavaidsDb::Navaid>& navaids);
                                void draw(Cairo::Context *cr, const std::vector<WaypointsDb::Waypoint>& waypoints);
                                void draw(Cairo::Context *cr, const std::vector<AirportsDb::Airport>& airports);
				void draw_glidearea(const Cairo::RefPtr<Cairo::ImageSurface> surface);
                };

                class DrawThreadAirportDiagram : public DrawThread {
                        public:
                                DrawThreadAirportDiagram(Engine& eng, Glib::Dispatcher& dispatch, const ScreenCoord& scrsize = ScreenCoord(0, 0), const Point& center = Point(), int alt = 0, uint16_t upangle = 0, int64_t time = 0);
                                virtual ~DrawThreadAirportDiagram();

				virtual std::pair<float,float> get_map_scale_bounds(void) const {
					return std::pair<float,float>(ImageBuffer<float>::arpt_min_scale, ImageBuffer<float>::arpt_max_scale);
				}

                                virtual void airspaces_changed(void);
                                virtual void airports_changed(void);
                                virtual void navaids_changed(void);
                                virtual void waypoints_changed(void);
                                virtual void airways_changed(void);

                        protected:
                                class LabelPart : public Glib::ustring {
                                        public:
                                                LabelPart(void) : m_fgr(0.0), m_fgg(0.0), m_fgb(0.0), m_bgr(1.0), m_bgg(1.0), m_bgb(0.0) {}
                                                void set_fg(double r, double g, double b) { m_fgr = r; m_fgg = g; m_fgb = b; }
                                                void set_bg(double r, double g, double b) { m_bgr = r; m_bgg = g; m_bgb = b; }
                                                void set_fg(Cairo::Context *cr) const { cr->set_source_rgb(m_fgr, m_fgg, m_fgb); }
                                                void set_bg(Cairo::Context *cr) const { cr->set_source_rgb(m_bgr, m_bgg, m_bgb); }

                                        private:
                                                double m_fgr, m_fgg, m_fgb;
                                                double m_bgr, m_bgg, m_bgb;
                                };

                                Glib::RefPtr<Engine::AirportResult> m_airportquery;
                                Glib::RefPtr<Engine::NavaidResult> m_navaidquery;
                                Glib::RefPtr<Engine::WaypointResult> m_waypointquery;

				typedef enum {
					drawstate_airports,
					drawstate_navaids,
					drawstate_waypoints,
					drawstate_airspaces,
					drawstate_airways,
					drawstate_done
				} drawstate_t;
				drawstate_t m_drawstate;

                                bool check_abort(void);
				void draw_restart(bool dbquery);
                                void draw_iterate(void);
				void async_cancel(void);

                                void draw(Cairo::Context *cr, const AirportsDb::Airport& arpt);
                                void draw(Cairo::Context *cr, const NavaidsDb::Navaid& n);
                                void draw(Cairo::Context *cr, const WaypointsDb::Waypoint& w);
                                void polyline_to_path(Cairo::Context *cr, const AirportsDb::Airport::Polyline& pl);
                                static void draw_taxiwaysign(Cairo::Context *cr, const Glib::ustring& text);
                };

                class DrawThreadTMS : public DrawThreadOverlay {
                        public:
                                DrawThreadTMS(Engine& eng, Glib::Dispatcher& dispatch, unsigned int source, const ScreenCoord& scrsize = ScreenCoord(0, 0), const Point& center = Point(), int alt = 0, uint16_t upangle = 0, int64_t time = 0);
                                virtual ~DrawThreadTMS();

				virtual std::pair<float,float> get_map_scale_bounds(void) const {
					return std::pair<float,float>(ImageBuffer<float>::arpt_min_scale, ImageBuffer<float>::max_scale);
				}

				class TileIndex {
				public:
					typedef enum {
						mode_tms,
						mode_svsm_vfr,
						mode_svsm_ifrl,
						mode_svsm_ifrh,
						mode_tmsch1903
					} mode_t;

					TileIndex(mode_t mode = mode_tms, unsigned int zoom = 0, unsigned int x = 0, unsigned int y = 0);
					TileIndex(mode_t mode, unsigned int zoom, const Point& pt);
					mode_t get_mode(void) const { return m_mode; }
					void set_zoom(unsigned int zoom) { m_z = zoom; }
					void set_x(unsigned int x) { m_x = x; }
					void set_y(unsigned int y) { m_y = y; }
					void set_rounddown(const Point& pt);
					void set_roundup(const Point& pt);
					unsigned int get_zoom(void) const { return m_z; }
					unsigned int get_x(void) const { return m_x; }
					unsigned int get_y(void) const { return m_y; }
					unsigned int get_xsize(void) const;
					unsigned int get_ysize(void) const;
					unsigned int get_invy(void) const;
					operator Point() const;
					operator Rect() const;
					operator Corner() const;
					void nextx(void);
					void prevx(void);
					void nexty(void);
					void prevy(void);
					double nmi_per_pixel(unsigned int zoom, const Point& pt);
					unsigned int nearest_zoom(double nmiperpixel, const Point& pt);
					operator std::string() const;
					bool operator<(const TileIndex& x) const;
					bool operator>(const TileIndex& x) const { return x < *this; }
					bool operator<=(const TileIndex& x) const { return !(x > *this); }
					bool operator>=(const TileIndex& x) const { return !(*this < x); }
					bool operator==(const TileIndex& x) const;
					bool operator!=(const TileIndex& x) const { return !(*this == x); }
					std::string quadtree_string(const char *quadrants) const;

				protected:
					unsigned int m_z;
					unsigned int m_x;
					unsigned int m_y;
					mode_t m_mode;

					unsigned int get_mask(void) const;
					static double get_sv_scale_vfr(unsigned int z);
					double get_sv_scale_vfr(void) const { return get_sv_scale_vfr(get_zoom()); }
					static unsigned int get_sv_xsize_vfr(unsigned int z);
					unsigned int get_sv_xsize_vfr(void) const { return get_sv_xsize_vfr(get_zoom()); }
					static unsigned int get_sv_ysize_vfr(unsigned int z);
					unsigned int get_sv_ysize_vfr(void) const { return get_sv_ysize_vfr(get_zoom()); }
					static double get_sv_scale_ifr(unsigned int z);
					double get_sv_scale_ifr(void) const { return get_sv_scale_ifr(get_zoom()); }
					static unsigned int get_sv_xsize_ifrl(unsigned int z);
					unsigned int get_sv_xsize_ifrl(void) const { return get_sv_xsize_ifrl(get_zoom()); }
					static unsigned int get_sv_ysize_ifrl(unsigned int z);
					unsigned int get_sv_ysize_ifrl(void) const { return get_sv_ysize_ifrl(get_zoom()); }
					static unsigned int get_sv_xsize_ifrh(unsigned int z);
					unsigned int get_sv_xsize_ifrh(void) const { return get_sv_xsize_ifrh(get_zoom()); }
					static unsigned int get_sv_ysize_ifrh(unsigned int z);
					unsigned int get_sv_ysize_ifrh(void) const { return get_sv_ysize_ifrh(get_zoom()); }

					static constexpr double sv_wgs_a = 6378137.0;
					static constexpr double sv_x_0 = -20037508.3428;
					static constexpr double sv_y_0 = 20037508.3428;
					static constexpr double sv_xr_vfr = 76.43702829;
					static constexpr double sv_yr_vfr = -76.43702829;
					static constexpr double sv_xr_ifrl = 101.91603771;
					static constexpr double sv_yr_ifrl = -101.91603771;
					static constexpr double sv_xr_ifrh = 203.83207543;
					static constexpr double sv_yr_ifrh = -203.83207543;
					static const unsigned int sv_tilesize = 256;
					static const unsigned int sv_chartwidth_vfr = 524288;
					static const unsigned int sv_chartheight_vfr = 524288;
					static const unsigned int sv_chartwidth_ifrl = 393216;
					static const unsigned int sv_chartheight_ifrl = 393216;
					static const unsigned int sv_chartwidth_ifrh = 196608;
					static const unsigned int sv_chartheight_ifrh = 196608;
					static constexpr double ch1903_origin_x = 350000;
					static constexpr double ch1903_origin_y = 420000;
					static const unsigned int ch1903_tilesize = 256;
					struct TileLayerInfo {
						unsigned int width;
						unsigned int height;
						double resolution;
					};
					static const TileLayerInfo ch1903_layerinfo[29];
					static const TileLayerInfo& get_ch1903_info(unsigned int z);
					const TileLayerInfo& get_ch1903_info(void) const { return get_ch1903_info(get_zoom()); }
				};

				class TileCache {
				public:
					TileCache(unsigned int source = 0);
					TileCache(const TileCache& x);
					~TileCache();
					TileCache& operator=(const TileCache& x);
					unsigned int get_source(void) const { return m_source; }
					std::string get_filename(const TileIndex& idx) const;
					std::string get_uri(const TileIndex& idx);
					void mkdir(const TileIndex& idx) const;
					bool exists(const TileIndex& idx) const;
					Glib::RefPtr<Gdk::Pixbuf> get_pixbuf(const TileIndex& idx);
					sigc::connection connect_queuechange(const sigc::slot<void>& slot);
					unsigned int queue_size(void) const { return m_queue.size(); }
					void clear_queue(void);
					void queue_rect(unsigned int zoom, const Rect& bbox);
					unsigned int rect_tiles(unsigned int zoom, const Rect& bbox, bool download_only = false);
					unsigned int get_minzoom(void) const;
					unsigned int get_maxzoom(void) const;
					const char *get_shortname(void) const;
					const char *get_friendlyname(void) const;
					const char *get_uri(void) const;

				protected:
					Glib::Rand m_random;
					Glib::Dispatcher m_dispatch;
					Glib::Mutex m_mutex;
					typedef std::vector<TileIndex> queue_t;
					queue_t m_queue;
					typedef std::set<TileIndex> failed_t;
					failed_t m_failed;
					sigc::signal<void> m_queuechange;
					SoupSession *m_session;
					SoupMessage *m_msg;
					unsigned int m_source;
					bool m_dispatched;

					void queue(const TileIndex& idx);
					void runqueue(void);
					static void download_complete_1(SoupSession *session, SoupMessage *msg, gpointer user_data);
					void download_complete(SoupSession *session, SoupMessage *msg);
				};

                        protected:
				struct TileSource {
					unsigned int minzoom;
					unsigned int maxzoom;
					typedef enum {
						imgfmt_png,
						imgfmt_jpg,
					} imgfmt_t;
					imgfmt_t imgfmt;
					TileIndex::mode_t idxmode;
					const char *shortname;
					const char *friendlyname;
					const char *uri;
				};
				static const TileSource tilesources[];

				struct AIRACDates {
					time_t tm;
					const char *airac;

					bool operator==(const AIRACDates& x) const { return tm == x.tm; }
					bool operator!=(const AIRACDates& x) const { return tm != x.tm; }
					bool operator<(const AIRACDates& x) const { return tm < x.tm; }
					bool operator<=(const AIRACDates& x) const { return tm <= x.tm; }
					bool operator>(const AIRACDates& x) const { return tm > x.tm; }
					bool operator>=(const AIRACDates& x) const { return tm >= x.tm; }
				};
				static const AIRACDates airacdates[];

                                // private variables of the rendering thread
				TileCache m_cache;

				typedef enum {
					drawstate_ogm,
					drawstate_navaids,
					drawstate_waypoints,
					drawstate_airports,
					drawstate_airspaces,
					drawstate_airways,
					drawstate_weather,
					drawstate_grib2layer,
					drawstate_done
				} drawstate_t;
				drawstate_t m_drawstate;

                                void draw_restart(bool dbquery);
                                void draw_iterate(void);
				virtual bool need_altitude(void) { return !!(draw_get_drawflags() & drawflags_weather); }
				void tile_queue_changed(void);
                };

                class DrawThreadBitmap : public DrawThreadOverlay {
                        public:
                                DrawThreadBitmap(Engine& eng, Glib::Dispatcher& dispatch, const Glib::RefPtr<BitmapMaps::Map>& map = Glib::RefPtr<BitmapMaps::Map>(), const ScreenCoord& scrsize = ScreenCoord(0, 0), const Point& center = Point(), int alt = 0, uint16_t upangle = 0, int64_t time = 0);
                                virtual ~DrawThreadBitmap();

				virtual std::pair<float,float> get_map_scale_bounds(void) const {
					return std::pair<float,float>(ImageBuffer<float>::arpt_min_scale, ImageBuffer<float>::max_scale);
				}

				virtual void set_bitmap(const Glib::RefPtr<BitmapMaps::Map>& map = Glib::RefPtr<BitmapMaps::Map>());

                        protected:
				Glib::RefPtr<BitmapMaps::Map> m_map;
				typedef enum {
					drawstate_bitmap,
					drawstate_navaids,
					drawstate_waypoints,
					drawstate_airports,
					drawstate_airspaces,
					drawstate_airways,
					drawstate_weather,
					drawstate_grib2layer,
					drawstate_done
				} drawstate_t;
				drawstate_t m_drawstate;

                                void draw_restart(bool dbquery);
                                void draw_iterate(void);
				virtual bool need_altitude(void) { return !!(draw_get_drawflags() & drawflags_weather); }
                };

        public:
                typedef enum {
                        renderer_vmap,
                        renderer_terrain,
                        renderer_airportdiagram,
                        renderer_bitmap,
			renderer_openstreetmap,
			renderer_opencyclemap,
			renderer_osm_public_transport,
			renderer_osmc_trails,
			renderer_maps_for_free,
			renderer_google_street,
			renderer_google_satellite,
			renderer_google_hybrid,
			renderer_virtual_earth_street,
			renderer_virtual_earth_satellite,
			renderer_virtual_earth_hybrid,
			renderer_skyvector_worldvfr,
			renderer_skyvector_worldifrlow,
			renderer_skyvector_worldifrhigh,
			renderer_cartabossy,
			renderer_cartabossy_weekend,
			renderer_ign,
			renderer_openaip,
			renderer_icao_switzerland,
			renderer_glider_switzerland,
			renderer_obstacle_switzerland,
			renderer_icao_germany,
			renderer_vfr_germany
		} renderer_t;

                VectorMapRenderer(Engine& eng, Glib::Dispatcher& dispatch, int depth, const ScreenCoord& scrsize = ScreenCoord(0, 0),
				  const Point& center = Point(), renderer_t renderer = renderer_vmap, int alt = 0, uint16_t upangle = 0, int64_t time = 0);
                virtual ~VectorMapRenderer();
		virtual void stop(void);
                virtual void redraw_route(void);
                virtual void redraw_track(void);
                virtual void set_center(const Point& pt, int alt, uint16_t upangle = 0, int64_t time = 0);
                virtual void set_screensize(const ScreenCoord& scrsize);
                virtual void set_map_scale(float nmi_per_pixel) { if (m_thread) m_thread->set_map_scale(nmi_per_pixel); }
                virtual float get_map_scale(void) const { return m_thread ? m_thread->get_map_scale() : 0.1; }
		virtual std::pair<float,float> get_map_scale_bounds(void) const {
			return m_thread ? m_thread->get_map_scale_bounds() :
				std::pair<float,float>(std::numeric_limits<float>::quiet_NaN(), std::numeric_limits<float>::quiet_NaN());
		}
                virtual void set_maxalt(int alt = std::numeric_limits<int>::max()) { if (m_thread) m_thread->set_maxalt(alt); }
                virtual int get_maxalt(void) const { return m_thread ? m_thread->get_maxalt() : std::numeric_limits<int>::max(); }
		virtual void set_glideslope(float gs = std::numeric_limits<float>::quiet_NaN()) { if (m_thread) m_thread->set_glideslope(gs); }
		virtual float get_glideslope(void) const { return m_thread ? m_thread->get_glideslope() : std::numeric_limits<float>::quiet_NaN(); }
		virtual void set_tas(float tas = std::numeric_limits<float>::quiet_NaN()) { if (m_thread) m_thread->set_tas(tas); }
		virtual float get_tas(void) const { return m_thread ? m_thread->get_tas() : std::numeric_limits<float>::quiet_NaN(); }
		virtual void set_wind(float dir = 0, float speed = 0) { if (m_thread) m_thread->set_wind(dir, speed); }
		virtual float get_winddir(void) const { return m_thread ? m_thread->get_winddir() : 0; }
		virtual float get_windspeed(void) const { return m_thread ? m_thread->get_windspeed() : 0; }

                virtual operator DrawFlags() const { return m_drawflags; }
                virtual MapRenderer& operator=(DrawFlags f);
                virtual MapRenderer& operator&=(DrawFlags f);
                virtual MapRenderer& operator|=(DrawFlags f);
                virtual MapRenderer& operator^=(DrawFlags f);
		virtual void set_grib2(const Glib::RefPtr<GRIB2::Layer>& layer);
		virtual void set_bitmap(const Glib::RefPtr<BitmapMaps::Map>& map = Glib::RefPtr<BitmapMaps::Map>());
		virtual void airspaces_changed(void);
                virtual void airports_changed(void);
                virtual void navaids_changed(void);
                virtual void waypoints_changed(void);
                virtual void airways_changed(void);
                virtual void draw(const Cairo::RefPtr<Cairo::Context>& cr, ScreenCoord offs = ScreenCoord(0, 0));
                virtual Point screen_coord(const ScreenCoord& sc) { return m_thread ? m_thread->screen_coord(sc) : Point(); }
                virtual Point screen_coord(const ScreenCoordFloat& sc) { return m_thread ? m_thread->screen_coord(sc) : Point(); }
                virtual ScreenCoord coord_screen(const Point& pt) { return m_thread ? m_thread->coord_screen(pt) : ScreenCoord(0, 0); }
                virtual void hide(void) { if (m_thread) m_thread->hide(); }
                virtual void show(void) { if (m_thread) m_thread->show(); }
                virtual sigc::connection connect_update(const sigc::slot<void>& slot) { return m_thread ? m_thread->connect_update(slot) : sigc::connection(); }

		class TileDownloader : protected DrawThreadTMS::TileCache {
		  public:
			TileDownloader(renderer_t renderer);

			using DrawThreadTMS::TileCache::connect_queuechange;
			using DrawThreadTMS::TileCache::queue_size;
			using DrawThreadTMS::TileCache::clear_queue;
			using DrawThreadTMS::TileCache::get_minzoom;
			using DrawThreadTMS::TileCache::get_maxzoom;
			using DrawThreadTMS::TileCache::get_shortname;
			using DrawThreadTMS::TileCache::get_friendlyname;
			using DrawThreadTMS::TileCache::get_uri;
			void queue_rect(unsigned int zoom, const Rect& bbox);
			unsigned int rect_tiles(unsigned int zoom, const Rect& bbox, bool download_only = false);

		  protected:
			renderer_t m_renderer;
		};

        protected:
                DrawThread *m_thread;
                DrawFlags m_drawflags;
};

inline MapRenderer::DrawFlags operator|(MapRenderer::DrawFlags x, MapRenderer::DrawFlags y) { return (MapRenderer::DrawFlags)((unsigned int)x | (unsigned int)y); }
inline MapRenderer::DrawFlags operator&(MapRenderer::DrawFlags x, MapRenderer::DrawFlags y) { return (MapRenderer::DrawFlags)((unsigned int)x & (unsigned int)y); }
inline MapRenderer::DrawFlags operator^(MapRenderer::DrawFlags x, MapRenderer::DrawFlags y) { return (MapRenderer::DrawFlags)((unsigned int)x ^ (unsigned int)y); }
inline MapRenderer::DrawFlags operator~(MapRenderer::DrawFlags x){ return MapRenderer::drawflags_all ^ x; }
inline MapRenderer::DrawFlags& operator|=(MapRenderer::DrawFlags& x, MapRenderer::DrawFlags y) { x = x | y; return x; }
inline MapRenderer::DrawFlags& operator&=(MapRenderer::DrawFlags& x, MapRenderer::DrawFlags y) { x = x & y; return x; }
inline MapRenderer::DrawFlags& operator^=(MapRenderer::DrawFlags& x, MapRenderer::DrawFlags y) { x = x ^ y; return x; }

inline std::ostream& operator<<(std::ostream& os, VectorMapRenderer::TimeMeasurement& tm) { return tm.print(os); }

class VectorMapArea : public Gtk::DrawingArea {
        public:
                explicit VectorMapArea(BaseObjectType *castitem, const Glib::RefPtr<Builder>& refxml);
                virtual ~VectorMapArea();

                typedef enum {
                        cursor_invalid,
                        cursor_mouse,
                        cursor_set
                } cursor_validity_t;

                typedef enum {
                        renderer_vmap,
                        renderer_terrain,
                        renderer_airportdiagram,
                        renderer_bitmap,
			renderer_openstreetmap,
			renderer_opencyclemap,
			renderer_osm_public_transport,
			renderer_osmc_trails,
			renderer_maps_for_free,
			renderer_google_street,
			renderer_google_satellite,
			renderer_google_hybrid,
			renderer_virtual_earth_street,
			renderer_virtual_earth_satellite,
			renderer_virtual_earth_hybrid,
			renderer_skyvector_worldvfr,
			renderer_skyvector_worldifrlow,
			renderer_skyvector_worldifrhigh,
			renderer_cartabossy,
			renderer_cartabossy_weekend,
			renderer_ign,
			renderer_openaip,
			renderer_icao_switzerland,
			renderer_glider_switzerland,
			renderer_obstacle_switzerland,
			renderer_icao_germany,
			renderer_vfr_germany,
                        renderer_none
               } renderer_t;

                void set_engine(Engine& eng);
                void set_renderer(renderer_t render);
                renderer_t get_renderer(void) const;
		static bool is_renderer_enabled(renderer_t rnd);
		static VectorMapRenderer::renderer_t convert_renderer(renderer_t render);
                void set_route(const FPlanRoute& route);
                void set_track(const TracksDb::Track *track);
                void redraw_route(void);
                void redraw_track(void);
                virtual void set_center(const Point& pt, int alt, uint16_t upangle = 0, int64_t time = 0);
                virtual Point get_center(void) const;
                virtual int get_altitude(void) const;
		uint16_t get_upangle(void) const;
		int64_t get_time(void) const;
		void set_map_scale(float nmi_per_pixel);
                float get_map_scale(void) const;
		std::pair<float,float> get_map_scale_bounds(void) const;
 		void set_glideslope(float gs = std::numeric_limits<float>::quiet_NaN());
		float get_glideslope(void) const;
		void set_tas(float tas = std::numeric_limits<float>::quiet_NaN());
		float get_tas(void) const;
		void set_wind(float dir = 0, float speed = 0);
		float get_winddir(void) const;
		float get_windspeed(void) const;
		void zoom_out(void);
                void zoom_in(void);
                void pan_up(void);
                void pan_down(void);
                void pan_left(void);
                void pan_right(void);
                void set_dragbutton(unsigned int dragbutton);
                bool is_cursor_valid(void) const { return m_cursorvalid; }
                Point get_cursor(void) const { return m_cursorvalid ? m_cursor : Point(); }
                void set_cursor(const Point& pt);
                Rect get_cursor_rect(void) const;
                void invalidate_cursor(void);
                void set_cursor_angle(float angle);
                float get_cursor_angle(void) const { return m_cursorangle * MapRenderer::to_deg_16bit; }
                bool is_cursor_angle_valid(void) const { return m_cursoranglevalid; }
                void invalidate_cursor_angle(void);
                virtual operator MapRenderer::DrawFlags() const;
                virtual VectorMapArea& operator=(MapRenderer::DrawFlags f);
                virtual VectorMapArea& operator&=(MapRenderer::DrawFlags f);
                virtual VectorMapArea& operator|=(MapRenderer::DrawFlags f);
                virtual VectorMapArea& operator^=(MapRenderer::DrawFlags f);
		void set_grib2(const Glib::RefPtr<GRIB2::Layer>& layer = Glib::RefPtr<GRIB2::Layer>());
		void set_bitmap(const Glib::RefPtr<BitmapMaps::Map>& map = Glib::RefPtr<BitmapMaps::Map>());
		const Glib::RefPtr<BitmapMaps::Map>& get_bitmap(void) const { return m_map; }
                void airspaces_changed(void);
                void airports_changed(void);
                void navaids_changed(void);
                void waypoints_changed(void);
                void airways_changed(void);
                sigc::signal<void,Point,cursor_validity_t> signal_cursor(void) { return m_signal_cursor; }
                sigc::signal<void,Point> signal_pointer(void) { return m_signal_pointer; }

#ifdef HAVE_GTKMM2
		inline bool get_is_drawable(void) const { return is_drawable(); }
		inline bool get_visible(void) const { return is_visible(); }
#endif

        protected:
#ifdef HAVE_GTKMM2
                virtual bool on_expose_event(GdkEventExpose *event);
#endif
		virtual bool on_draw(const Cairo::RefPtr<Cairo::Context>& cr);
                virtual void on_size_allocate(Gtk::Allocation& allocation);
                virtual void on_show(void);
                virtual void on_hide(void);
                static unsigned int button_translate(GdkEventButton* event);
                virtual bool on_button_press_event(GdkEventButton* event);
                virtual bool on_button_release_event(GdkEventButton* event);
                virtual bool on_motion_notify_event(GdkEventMotion* event);
                virtual bool on_scroll_event(GdkEventScroll *event);

                void redraw(void) { if (get_is_drawable()) queue_draw(); }
                void renderer_update(void);
                void renderer_alloc(void);

        protected:
                Engine *m_engine;
                MapRenderer *m_renderer;
                renderer_t m_rendertype;
                unsigned int m_dragbutton;
                MapRenderer::ScreenCoordFloat m_dragstart;
                MapRenderer::ScreenCoord m_dragoffset;
                bool m_draginprogress, m_cursorvalid, m_cursoranglevalid;
                Point m_cursor;
                uint16_t m_cursorangle;
                sigc::signal<void,Point,cursor_validity_t> m_signal_cursor;
                sigc::signal<void,Point> m_signal_pointer;
		Glib::Dispatcher m_dispatch;
		sigc::connection m_connupdate;
                // Caching
                Point m_center;
                float m_mapscale;
		float m_glideslope;
		float m_tas;
		float m_winddir;
		float m_windspeed;
		int64_t m_time;
                int m_altitude;
		uint16_t m_upangle;
		Glib::RefPtr<GRIB2::Layer> m_grib2layer;
		Glib::RefPtr<BitmapMaps::Map> m_map;
                const FPlanRoute *m_route;
                const TracksDb::Track *m_track;
                MapRenderer::DrawFlags m_drawflags;
};

const std::string& to_str(VectorMapArea::renderer_t r);
const std::string& to_short_str(VectorMapArea::renderer_t r);

#endif /* MAPSNT_H */
