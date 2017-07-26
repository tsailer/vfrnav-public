//
// C++ Interface: Sensors
//
// Description: Sensor Handling
//
//
// Author: Thomas Sailer <t.sailer@alumni.ethz.ch>, (C) 2011, 2012, 2013, 2014, 2016
//
// Copyright: See COPYING file that comes with this distribution
//
//

#ifndef SENSORS_H
#define SENSORS_H

#include "sysdeps.h"

#include <fstream>
#include <limits>
#include <set>
#include <glibmm.h>
#include <sigc++/sigc++.h>

#include "geom.h"
#include "airdata.h"
#include "wmm.h"
#include "engine.h"
#include "aircraft.h"

class Sensors : public sigc::trackable {
 public:
	class ConfigFile : public Glib::KeyFile {
	public:
		ConfigFile(const ConfigFile& cf);
		ConfigFile(const std::string& read_filename, const std::string& write_filename);
		ConfigFile(const std::string& write_filename);
		const std::string& get_filename(void) const { return m_filename; }
		bool is_ok(void) const { return m_ok; }
		int compare(const ConfigFile& cfg) const { return get_filename().compare(cfg.get_filename()); }
		bool operator<(const ConfigFile& cfg) const { return compare(cfg) < 0; }
		Glib::ustring get_name(void) const;
		Glib::ustring get_description(void) const;

	protected:
		std::string m_filename;
		bool m_ok;
	};

	static std::set<ConfigFile> find_configs(const std::string& dir_main = "", const std::string& dir_aux = "");

	class Sensor : public sigc::trackable {
	  public:
		Sensor(void);
		virtual ~Sensor();

		virtual void init(void) {}

		virtual const Glib::ustring& get_name(void) const { return empty; }
		virtual const Glib::ustring& get_description(void) const { return empty; }
		virtual bool get_position(Point& pt) const;
		virtual unsigned int get_position_priority(void) const;
		virtual Glib::TimeVal get_position_time(void) const;
		virtual bool is_position_ok(const Glib::TimeVal& tv) const;
		virtual bool is_position_ok(void) const;

		virtual void get_truealt(double& alt, double& altrate) const;
		virtual unsigned int get_truealt_priority(void) const;
		virtual Glib::TimeVal get_truealt_time(void) const;
		virtual bool is_truealt_ok(const Glib::TimeVal& tv) const;
		virtual bool is_truealt_ok(void) const;

		virtual void get_baroalt(double& alt, double& altrate) const;
		virtual unsigned int get_baroalt_priority(void) const;
		virtual Glib::TimeVal get_baroalt_time(void) const;
		virtual bool is_baroalt_ok(const Glib::TimeVal& tv) const;
		virtual bool is_baroalt_ok(void) const;

		virtual void get_course(double& crs, double& gs) const;
		virtual unsigned int get_course_priority(void) const;
		virtual Glib::TimeVal get_course_time(void) const;
		virtual bool is_course_ok(const Glib::TimeVal& tv) const;
		virtual bool is_course_ok(void) const;

		virtual void get_heading(double& hdg) const;
		virtual unsigned int get_heading_priority(void) const;
		virtual Glib::TimeVal get_heading_time(void) const;
		virtual bool is_heading_ok(const Glib::TimeVal& tv) const;
		virtual bool is_heading_ok(void) const;
		virtual bool is_heading_true(void) const;

		virtual void get_attitude(double& pitch, double& bank, double& slip, double& rate) const;
		virtual unsigned int get_attitude_priority(void) const;
		virtual Glib::TimeVal get_attitude_time(void) const;
		virtual bool is_attitude_ok(const Glib::TimeVal& tv) const;
		virtual bool is_attitude_ok(void) const;

		class ParamDesc {
		  public:
			typedef enum {
				type_section,
				type_string,
				type_switch,
				type_integer,
				type_double,
				type_choice,
				type_button,
				type_gyro,
				type_satellites,
				type_magcalib,
				type_adsbtargets
			} type_t;

			typedef enum {
				editable_readonly,
				editable_user,
				editable_admin
			} editable_t;

	  		ParamDesc(type_t t, editable_t e, unsigned int pnr, const Glib::ustring& lbl, const Glib::ustring& desc = "", const Glib::ustring& unit = "", unsigned int maxlen = 0)
				: m_label(lbl), m_description(desc), m_unit(unit), m_min(std::numeric_limits<double>::quiet_NaN()),
				m_max(std::numeric_limits<double>::quiet_NaN()), m_stepinc(std::numeric_limits<double>::quiet_NaN()),
				m_pageinc(std::numeric_limits<double>::quiet_NaN()), m_parnumber(pnr), m_digits(maxlen), m_type(t), m_editable(e) {}
	  		ParamDesc(type_t t, editable_t e, unsigned int pnr, const Glib::ustring& lbl, const Glib::ustring& desc, const Glib::ustring& unit, const Glib::ustring& buttonlbl)
				: m_label(lbl), m_description(desc), m_unit(unit), m_min(std::numeric_limits<double>::quiet_NaN()),
				m_max(std::numeric_limits<double>::quiet_NaN()), m_stepinc(std::numeric_limits<double>::quiet_NaN()),
				m_pageinc(std::numeric_limits<double>::quiet_NaN()), m_parnumber(pnr), m_digits(0), m_type(t), m_editable(e) {}
			template <typename T>
			ParamDesc(type_t t, editable_t e, unsigned int pnr, const Glib::ustring& lbl, const Glib::ustring& desc, const Glib::ustring& unit, T cb, T ce)
				: m_label(lbl), m_description(desc), m_unit(unit), m_choices(cb, ce), m_min(std::numeric_limits<double>::quiet_NaN()),
				m_max(std::numeric_limits<double>::quiet_NaN()), m_stepinc(std::numeric_limits<double>::quiet_NaN()),
				m_pageinc(std::numeric_limits<double>::quiet_NaN()), m_parnumber(pnr), m_digits(0), m_type(t), m_editable(e) {}
			ParamDesc(type_t t, editable_t e, unsigned int pnr, const Glib::ustring& lbl, const Glib::ustring& desc, const Glib::ustring& unit,
				  unsigned int digits, double min, double max, double step, double page)
				: m_label(lbl), m_description(desc), m_unit(unit), m_min(min), m_max(max), m_stepinc(step), m_pageinc(page),
				m_parnumber(pnr), m_digits(digits), m_type(t), m_editable(e) {}
			type_t get_type(void) const { return m_type; }
			editable_t get_editable(void) const { return m_editable; }
			const Glib::ustring& get_label(void) const { return m_label; }
			const Glib::ustring& get_description(void) const { return m_description; }
			const Glib::ustring& get_unit(void) const { return m_unit; }
			void get_range(unsigned int& digits, double& min, double& max, double& stepinc, double& pageinc) const {
				digits = m_digits; min = m_min; max = m_max; stepinc = m_stepinc; pageinc = m_pageinc;
			}
			unsigned int get_parnumber(void) const { return m_parnumber; }
			unsigned int get_maxlength(void) const { return m_digits; }
			typedef std::vector<Glib::ustring> choices_t;
			const choices_t& get_choices(void) const { return m_choices; }

		  protected:
			Glib::ustring m_label;
			Glib::ustring m_description;
			Glib::ustring m_unit;
			choices_t m_choices;
			double m_min;
			double m_max;
			double m_stepinc;
			double m_pageinc;
			unsigned int m_parnumber;
			unsigned int m_digits;
			type_t m_type;
			editable_t m_editable;
		};

		typedef std::vector<ParamDesc> paramdesc_t;
		virtual void get_param_desc(unsigned int pagenr, paramdesc_t& pd);
		virtual unsigned int get_param_pages(void) const { return 0; }

		class Satellite {
		  public:
			Satellite(int prn, double az, double elev, double snr, bool used = false)
				: m_azimuth(az), m_elevation(elev), m_snr(snr), m_prn(prn), m_used(used) {}

			double get_azimuth(void) const { return m_azimuth; }
			double get_elevation(void) const { return m_elevation; }
			double get_snr(void) const { return m_snr; }
			int get_prn(void) const { return m_prn; }
			bool is_used(void) const { return m_used; }
			int compare(const Satellite& sat) const;
			int compare_all(const Satellite& sat) const;
			bool operator==(const Satellite& sat) const { return !compare(sat); }
			bool operator!=(const Satellite& sat) const { return !operator==(sat); }
			bool operator<(const Satellite& sat) const { return compare(sat) < 0; }
			bool operator<=(const Satellite& sat) const { return compare(sat) <= 0; }
			bool operator>(const Satellite& sat) const { return compare(sat) > 0; }
			bool operator>=(const Satellite& sat) const { return compare(sat) >= 0; }

		  protected:
			double m_azimuth;
			double m_elevation;
			double m_snr;
			int m_prn;
			bool m_used;
		};

		typedef std::vector<Satellite> satellites_t;

		class GPSFlightPlanWaypoint {
		  public:
			typedef enum {
				type_invalid,
				type_undefined,
				type_airport,
				type_military,
				type_heliport,
				type_seaplane,
				type_vor,
				type_ndb,
				type_intersection,
				type_userdefined
			} type_t;

			GPSFlightPlanWaypoint(const Point& coord, const std::string& ident = "", bool dest = false, double var = std::numeric_limits<double>::quiet_NaN());
			GPSFlightPlanWaypoint(const uint8_t *p = 0);
			void set_type(type_t typ);
			const Point& get_coord(void) const { return m_coord; }
			const std::string& get_ident(void) const { return m_ident; }
			double get_variation(void) const { return m_var; }
			bool is_destination(void) const { return m_dest; }
			type_t get_type(void) const { return m_type; }
			operator bool(void) const { return get_type() != type_invalid && get_type() != type_undefined; }

		  protected:
			std::string m_ident;
			Point m_coord;
			double m_var;
			type_t m_type;
			bool m_dest;
		};

		typedef std::vector<GPSFlightPlanWaypoint> gpsflightplan_t;

		class ADSBTarget {
		  public:
			class Position {
			  public:
				typedef enum {
					altmode_invalid,
					altmode_baro,
					altmode_gnss
				} altmode_t;

				Position(const Glib::TimeVal& ts, const Point& coord, uint8_t type,
					 uint32_t cprlat, uint32_t cprlon, bool cprformat, bool timebit,
					 altmode_t altmode = altmode_invalid, uint16_t alt = 0);
				const Glib::TimeVal& get_timestamp(void) const { return m_timestamp; }
				const Point& get_coord(void) const { return m_coord; }
				uint8_t get_type(void) const { return m_type; }
				uint32_t get_cprlat(void) const { return m_cprlat; }
				uint32_t get_cprlon(void) const { return m_cprlon; }
				uint16_t get_alt(void) const { return m_alt; }
				altmode_t get_altmode(void) const { return m_altmode; }
				bool get_cprformat(void) const { return m_cprformat; }
				bool get_timebit(void) const { return m_timebit; }

			  protected:
				Glib::TimeVal m_timestamp;
				Point m_coord;
				uint32_t m_cprlat;
				uint32_t m_cprlon;
				uint16_t m_alt;
				altmode_t m_altmode;
				uint8_t m_type;
				bool m_cprformat;
				bool m_timebit;
			};

			typedef enum {
				lmotion_invalid,
				lmotion_gnd,
				lmotion_ias,
				lmotion_tas
			} lmotion_t;

			typedef enum {
				vmotion_invalid,
				vmotion_gnss,
				vmotion_baro
			} vmotion_t;

			ADSBTarget(uint32_t icaoaddr = 0, bool ownship = false);
			ADSBTarget(const Glib::TimeVal& ts, uint32_t icaoaddr = 0, bool ownship = false);
			bool operator<(const ADSBTarget& x) const { return get_icaoaddr() < x.get_icaoaddr(); }
			uint32_t get_icaoaddr(void) const { return m_icaoaddr; }
			bool is_ownship(void) const { return m_ownship; }
			bool empty(void) const { return m_positions.empty(); }
			unsigned int size(void) const { return m_positions.size(); }
			const Position& operator[](unsigned int idx) const { return m_positions[idx]; }
			const Position& back(void) const { return m_positions.back(); }
			const std::string& get_ident(void) const { return m_ident; }
			const Glib::TimeVal& get_timestamp(void) const { return m_timestamp; }
			bool is_modea_valid(void) const { return m_modea != std::numeric_limits<uint16_t>::max(); }
			uint16_t get_modea(void) const { return m_modea; }
			const Glib::TimeVal& get_motiontimestamp(void) const { return m_motiontimestamp; }
			lmotion_t get_lmotion(void) const { return m_lmotion; }
			int16_t get_speed(void) const { return m_speed; }
			uint16_t get_direction(void) const { return m_direction; }
			vmotion_t get_vmotion(void) const { return m_vmotion; }
			int32_t get_verticalspeed(void) const { return m_verticalspeed; }
			const Glib::TimeVal& get_baroalttimestamp(void) const { return m_baroalttimestamp; }
			int32_t get_baroalt(void) const { return m_baroalt; }
			const Glib::TimeVal& get_gnssalttimestamp(void) const { return m_gnssalttimestamp; }
			int32_t get_gnssalt(void) const { return m_gnssalt; }
			bool is_capability_valid(void) const { return m_capability != std::numeric_limits<uint8_t>::max(); }
			uint8_t get_capability(void) const { return m_capability; }
			bool is_emergencystate_valid(void) const { return m_emergencystate != std::numeric_limits<uint8_t>::max(); }
			uint8_t get_emergencystate(void) const { return m_emergencystate; }
			bool is_emittercategory_valid(void) const { return m_emittercategory != std::numeric_limits<uint8_t>::max(); }
			uint8_t get_emittercategory(void) const { return m_emittercategory; }

			uint8_t get_nicsuppa(void) const { return m_nicsuppa; }
			uint8_t get_nicsuppb(void) const { return m_nicsuppb; }
			uint8_t get_nicsuppc(void) const { return m_nicsuppc; }

			void set_ownship(bool os) { m_ownship = os; }
			void set_baroalt(const Glib::TimeVal& tv, int32_t baroalt);
			void set_baroalt(const Glib::TimeVal& tv, int32_t baroalt, int32_t verticalspeed);
			void set_gnssalt(const Glib::TimeVal& tv, int32_t gnssalt);
			void set_gnssalt(const Glib::TimeVal& tv, int32_t gnssalt, int32_t verticalspeed);
			void set_modea(const Glib::TimeVal& tv, uint16_t modea);
			void set_modea(const Glib::TimeVal& tv, uint16_t modea, uint8_t emergencystate);
			void set_ident(const Glib::TimeVal& tv, const std::string& ident, uint8_t emittercategory);
			void set_motion(const Glib::TimeVal& tv, int16_t speed, uint16_t direction, lmotion_t lmotion,
					int32_t verticalspeed, vmotion_t vmotion);
			void add_position(const Position& pos);

		  protected:
			typedef std::vector<Position> positions_t;
			positions_t m_positions;
			std::string m_ident;
			Glib::TimeVal m_timestamp;
			Glib::TimeVal m_motiontimestamp;
			Glib::TimeVal m_baroalttimestamp;
			Glib::TimeVal m_gnssalttimestamp;
			uint32_t m_icaoaddr;
			int32_t m_baroalt;
			int32_t m_gnssalt;
			int32_t m_verticalspeed;
			uint16_t m_modea;
			int16_t m_speed;
			uint16_t m_direction;
			uint8_t m_capability;
			uint8_t m_emergencystate;
			uint8_t m_emittercategory;
			uint8_t m_nicsuppa;
			uint8_t m_nicsuppb;
			uint8_t m_nicsuppc;
			lmotion_t m_lmotion;
			vmotion_t m_vmotion;
			bool m_ownship;
		};

		typedef std::set<ADSBTarget> adsbtargets_t;

		typedef enum {
			paramfail_ok,
			paramfail_fail
		} paramfail_t;
		virtual paramfail_t get_param(unsigned int nr, Glib::ustring& v) const;
		virtual paramfail_t get_param(unsigned int nr, int& v) const;
		virtual paramfail_t get_param(unsigned int nr, double& v) const;
		virtual paramfail_t get_param(unsigned int nr, double& pitch, double& bank, double& slip, double& hdg, double& rate) const;
		virtual paramfail_t get_param(unsigned int nr, satellites_t& sat) const;
		virtual paramfail_t get_param(unsigned int nr, gpsflightplan_t& fpl, unsigned int& curwpt) const;
		virtual paramfail_t get_param(unsigned int nr, adsbtargets_t& targets) const;
		virtual void set_param(unsigned int nr, const Glib::ustring& v);
		virtual void set_param(unsigned int nr, int v);
		virtual void set_param(unsigned int nr, double v);

		class ParamChanged {
		  public:
		        ParamChanged();
			bool is_changed(unsigned int nr) const;
			bool is_changed(unsigned int from, unsigned int to) const;
			void set_changed(unsigned int nr);
			void set_changed(unsigned int from, unsigned int to);
			void set_all_changed(void);
			void clear_changed(unsigned int nr);
			void clear_changed(unsigned int from, unsigned int to);
			void clear_all_changed(void);
			ParamChanged& operator&=(const ParamChanged& x);
			ParamChanged& operator|=(const ParamChanged& x);
			ParamChanged operator&(const ParamChanged& x) const;
			ParamChanged operator|(const ParamChanged& x) const;
			ParamChanged operator~(void) const;
			operator bool(void) const;

		  protected:
			uint32_t m_changed[4];
		};
		virtual sigc::connection connect_update_params(const sigc::slot<void,const ParamChanged&>& s) { return sigc::connection(); }

		virtual std::string logfile_header(void) const = 0;
		virtual std::string logfile_records(void) const = 0;

	  protected:
		static const Glib::ustring empty;
	};

	class SensorInstance : public Sensor {
	  public:
		SensorInstance(Sensors& sensors, const Glib::ustring& configgroup);
		void reference(void) const;
		void unreference(void) const;
		Sensors& get_sensors(void) const { return m_sensors; }
		const Glib::ustring& get_configgroup(void) const { return m_configgroup; }
		const Glib::ustring& get_name(void) const { return m_name; }
		const Glib::ustring& get_description(void) const { return m_description; }
		virtual sigc::connection connect_update_params(const sigc::slot<void,const ParamChanged&>& s) { return m_updateparams.connect(s); }

		virtual void get_param_desc(unsigned int pagenr, paramdesc_t& pd);
		virtual unsigned int get_param_pages(void) const { return 1; }
		using Sensor::get_param;
		virtual paramfail_t get_param(unsigned int nr, Glib::ustring& v) const;
		using Sensor::set_param;
		virtual void set_param(unsigned int nr, const Glib::ustring& v);

		virtual std::string logfile_header(void) const { return std::string(); }
		virtual std::string logfile_records(void) const { return std::string(); }

	  protected:
		void update(const ParamChanged& chg);

		enum {
			parnrstart = 0,
			parnrname = parnrstart,
			parnrdesc,
			parnrend
		};

		Sensors& m_sensors;
		Glib::ustring m_configgroup;
		Glib::ustring m_name;
		Glib::ustring m_description;
		sigc::signal<void,const ParamChanged&> m_updateparams;
		mutable unsigned int m_refcount;
	};

	class DummySensor;
	class SensorAttitude;
	class SensorAttitudePSMove;
	class SensorAttitudePSMoveBT;
	class SensorAttitudePSMoveHID;
	class SensorAttitudeIIO;
	class SensorAttitudeSTMSensorHub;
	class SensorGPS;
	class SensorGPSD;
	class SensorGypsy;
	class SensorLibLocation;
	class SensorGeoclue;
	class SensorGeoclue2;
	class SensorKingGPS;
	class SensorKingGPSTTY;
	class SensorKingGPSFTDI;
	class SensorNMEAGPS;
	class SensorMS5534;
	class SensorWinGPS;
	class SensorWinBaro;
	class SensorWinAttitude;
	class SensorADSB;
	class SensorRTLADSB;
	class SensorRemoteADSB;

	class MapAircraft {
	  public:
		MapAircraft(uint32_t icaoaddr = 0);
		MapAircraft(const Sensor::ADSBTarget& acft);
		bool operator<(const MapAircraft& x) const;
		bool is_changed(const MapAircraft& x) const;

		const std::string& get_registration(void) const { return m_registration; }
		const std::string& get_ident(void) const { return m_ident; }
		const Glib::TimeVal& get_timestamp(void) const { return m_timestamp; }
		const Point& get_coord(void) const { return m_coord; }
		uint32_t get_icaoaddr(void) const { return m_icaoaddr; }
		int32_t get_baroalt(void) const { return m_baroalt; }
		int32_t get_truealt(void) const { return m_truealt; }
		int32_t get_vs(void) const { return m_vs; }
		uint16_t get_modea(void) const { return m_modea; }
		uint16_t get_crs(void) const { return m_crs; }
		uint16_t get_speed(void) const { return m_speed; }

		void set_registration(const std::string& x = "") { m_registration = x; }
		void set_ident(const std::string& x = "") { m_ident = x; }
		void set_timestamp(const Glib::TimeVal& x = Glib::TimeVal(-1, 0)) { m_timestamp = x; }
		void set_timestamp_current(void) { m_timestamp.assign_current_time(); }
		void set_coord(const Point& x) { m_coord = x; }
		void set_coord(void) { m_coord.set_invalid(); }
		void set_baroalt(int32_t x = std::numeric_limits<int32_t>::min()) { m_baroalt = x; }
		void set_truealt(int32_t x = std::numeric_limits<int32_t>::min()) { m_truealt = x; }
		void set_vs(int32_t x = std::numeric_limits<int32_t>::min()) { m_vs = x; }
		void set_modea(uint16_t x = std::numeric_limits<uint16_t>::max()) { m_modea = x; }
		void set_crs_speed(uint16_t crs = 0, uint16_t spd = std::numeric_limits<uint16_t>::max()) { m_crs = crs; m_speed = spd; }

		bool is_registration_valid(void) const { return !m_registration.empty(); }
		bool is_ident_valid(void) const { return !m_ident.empty(); }
		bool is_timestamp_valid(void) const { return !m_timestamp.negative(); }
		bool is_coord_valid(void) const { return !m_coord.is_invalid(); }
		bool is_baroalt_valid(void) const { return m_baroalt != std::numeric_limits<int32_t>::min(); }
		bool is_truealt_valid(void) const { return m_truealt != std::numeric_limits<int32_t>::min(); }
		bool is_vs_valid(void) const { return m_vs != std::numeric_limits<int32_t>::min(); }
		bool is_modea_valid(void) const { return m_modea != std::numeric_limits<uint16_t>::max(); }
		bool is_crs_speed_valid(void) const { return m_speed != std::numeric_limits<uint16_t>::max(); }

		std::string get_line1(void) const;
		std::string get_line2(void) const;

	  protected:
		std::string m_registration;
		std::string m_ident;
		Glib::TimeVal m_timestamp;
		Point m_coord;
		uint32_t m_icaoaddr;
		int32_t m_baroalt;
		int32_t m_truealt;
		int32_t m_vs;
		uint16_t m_modea;
		uint16_t m_crs;
		uint16_t m_speed;
	};

	Sensors(void);
	~Sensors();
	void init(const Glib::ustring& cfgdata, const std::string& cfgfile);
	const Glib::ustring& get_name(void) const { return m_name; }
	const Glib::ustring& get_description(void) const { return m_description; }

	void set_engine(Engine& eng);
	Engine *get_engine(void) { return m_engine; }

	FPlanRoute& get_route(void) { return m_route; }
	const FPlanRoute& get_route(void) const { return m_route; }
	TracksDb::Track& get_track(void) { return m_track; }
	const TracksDb::Track& get_track(void) const { return m_track; }

	unsigned int size(void) const { return m_sensors.size(); }
	Sensor& operator[](unsigned int idx);
	const Sensor& operator[](unsigned int idx) const;

	Glib::KeyFile& get_configfile(void);
	const Glib::KeyFile& get_configfile(void) const;
	void save_config(void);

	void set_altimeter_std(bool setstd = true);
	void set_altimeter_qnh(double qnh = 1013.15);
	bool is_altimeter_std(void) const { return m_altimeterstd; }
	double get_altimeter_qnh(void) const;
	double get_altimeter_tempoffs(void) const;

	double true_to_pressure_altitude(double truealt) const { return m_airdata.true_to_pressure_altitude(truealt); }
	double pressure_to_true_altitude(double baroalt) const { return m_airdata.pressure_to_true_altitude(baroalt); }

	static void std_altitude_to_pressure(float *press, float *temp, float alt) { AirData<float>::std_altitude_to_pressure(press, temp, alt); }
	static void std_pressure_to_altitude(float *alt, float *temp, float press) { AirData<float>::std_pressure_to_altitude(alt, temp, press); }
	void qnh_altitude_to_pressure(float *press, float *temp, float alt) { m_airdata.altitude_to_pressure(press, temp, alt); }
	void qnh_pressure_to_altitude(float *alt, float *temp, float press) { m_airdata.pressure_to_altitude(alt, temp, press); }

	double get_tatprobe_ct(void) const { return m_airdata.get_tatprobe_ct(); }
	void set_tatprobe_ct(double ct = 1);

	double get_airdata_oat(void) const { return m_airdata.get_oat(); }
	void set_airdata_oat(double oat);
	double get_airdata_rat(void) const { return m_airdata.get_rat(); }
	void set_airdata_rat(double oat);

	double get_airdata_cas(void) const { return m_airdata.get_cas(); }
	double get_airdata_eas(void) const { return m_airdata.get_eas(); }
	double get_airdata_tas(void) const { return m_airdata.get_tas(); }
	double get_airdata_mach(void) const { return m_airdata.get_mach(); }
	void set_airdata_cas(double cas);
	void set_airdata_tas(double tas);
	double get_airdata_densityalt(void) const { return m_airdata.get_density_altitude(); }

	void set_manualheading(double hdg, bool istrue);
	double get_manualheading(void) const { return m_manualhdg; }
	bool is_manualheading_true(void) const { return m_manualhdgtrue; }

	double get_declination(void) const { double d(m_wmm.get_dec()); if (std::isnan(d)) d = 0; return d; }
	bool is_declination_ok(void) const { return m_wmm; }

	bool get_position(Point& pt) const;
	bool get_position(Point& pt, TopoDb30::elev_t& elev) const;
	const Glib::TimeVal& get_position_time(void) const { return m_positiontime; }
	void get_altitude(double& baroalt, double& truealt, double& altrate) const;
	void get_altitude(double& baroalt, double& truealt) const;
	const Glib::TimeVal& get_altitude_time(void) const { return m_alttime; }
	void get_course(double& crsmag, double& crstrue, double& groundspeed) const;
	void get_course(double& crsmag, double& groundspeed) const;
	void get_course(double& crsmag) const;
	const Glib::TimeVal& get_course_time(void) const { return m_coursetime; }
	void get_heading(double& hdgmag, double& hdgtrue) const;
	void get_heading(double& hdgmag) const;
	const Glib::TimeVal& get_heading_time(void) const { return m_headingtime; }
	void get_attitude(double& pitch, double& bank, double& slip, double& rate) const;
	const Glib::TimeVal& get_attitude_time(void) const { return m_attitudetime; }
	typedef enum {
		mapangle_true = (1 << 0),
		mapangle_course = (1 << 1),
		mapangle_heading = (1 << 2),
		mapangle_manualheading = (1 << 3)
	} mapangle_t;
	mapangle_t get_mapangle(double& angle, mapangle_t flags) const;

	typedef enum {
		change_none = 0,
		change_position = (1 << 0),
		change_altitude = (1 << 1),
		change_course = (1 << 2),
		change_heading = (1 << 3),
		change_attitude = (1 << 4),
		change_airdata = (1 << 5),
		change_navigation = (1 << 6),
		change_fplan = (1 << 7),
		change_all = change_position | change_altitude | change_heading | change_attitude | change_airdata
		| change_navigation | change_fplan		
	} change_t;
	sigc::connection connect_change(const sigc::slot<void,change_t>& slot);
	void update_sensors(void) { update_sensors_core(change_none); }

	sigc::connection connect_mapaircraft(const sigc::slot<void,const MapAircraft&>& slot);
	void set_mapaircraft(const MapAircraft& acft);

	typedef enum {
		loglevel_notice,
		loglevel_warning,
		loglevel_error,
		loglevel_fatal
	} loglevel_t;
	void log(loglevel_t lvl, const Glib::ustring& msg);

	sigc::signal<void,Glib::TimeVal,loglevel_t,const Glib::ustring&>& signal_logmessage(void) { return m_logmessage; }

	Aircraft& get_aircraft(void) { return m_aircraft; }
	const Aircraft& get_aircraft(void) const { return m_aircraft; }
	const Glib::ustring& get_aircraftfile(void) const { return m_aircraftfile; }

	Aircraft::Cruise::EngineParams get_acft_cruise_params(void) const;
	const Aircraft::Cruise::EngineParams& get_acft_cruise(void) const { return m_cruise; }
	void set_acft_cruise(const Aircraft::Cruise::EngineParams& x);

	typedef enum {
		iasmode_manual,
		iasmode_rpm_mp,
		iasmode_bhp_rpm
	} iasmode_t;

	iasmode_t get_acft_iasmode(void) const { return m_iasmode; }
	void set_acft_iasmode(iasmode_t iasmode);
	double get_acft_fuel(void) const { return m_fuel; }
	double get_acft_fuelflow(void) const { return m_fuelflow; }
	double get_acft_bhp(void) const { return m_bhp; }

	typedef std::pair<double,double> fplanwind_t;
	fplanwind_t get_fplan_wind(void) const;
	fplanwind_t get_fplan_wind(const Point& coord) const;

	void nav_load_fplan(FPlanRoute::id_t id);
	void nav_new_fplan(void);
	void nav_duplicate_fplan(void);
	void nav_delete_fplan(void);
	void nav_reverse_fplan(void);
        void nav_insert_wpt(uint32_t nr = ~0, const FPlanWaypoint& wpt = FPlanWaypoint());
	void nav_delete_wpt(uint32_t nr);
	void nav_fplan_modified(void);
	void nav_fplan_setdeparture(time_t offblock, unsigned int taxitime = 5U * 60U);
	bool nav_is_brg2_enabled(void) const { return m_navbrg2ena; }
	const Point& nav_get_brg2(void) const { return m_navbrg2; }
	const Glib::ustring& nav_get_brg2_dest(void) const { return m_navbrg2dest; }
	double nav_get_brg2_track_true(void) const { return m_navbrg2tracktrue; }
	double nav_get_brg2_track_mag(void) const { return m_navbrg2trackmag; }
	double nav_get_brg2_dist(void) const { return m_navbrg2dist; }
	void nav_set_brg2(const Glib::ustring& dest);
	void nav_set_brg2(const Point& pt);
	void nav_set_brg2(bool enable = false);
	void nav_directto(const Glib::ustring& dest, const Point& coord, double track, int32_t alt, uint16_t altflags = 0, uint16_t wptnr = ~0U);
	void nav_finalapproach(const AirportsDb::Airport::FASNavigate& fas);
	void nav_off(void);
	void nav_nearestleg(void);
	void nav_set_hold(double track);
	void nav_set_hold(bool enable = false);
	bool nav_is_hold(void) const { return m_navmode == navmode_fplhold || m_navmode == navmode_directhold; }
	bool nav_is_fpl(void) const { return m_navmode == navmode_fpl || m_navmode == navmode_fplhold; }
	bool nav_is_directto(void) const { return m_navmode == navmode_direct || m_navmode == navmode_directhold; }
	bool nav_is_finalapproach(void) const { return m_navmode == navmode_finalapproach; }
	bool nav_is_on(void) const { return nav_is_fpl() || nav_is_directto() || nav_is_finalapproach(); }
	const Glib::ustring& nav_get_curdest(void) const { return m_navcurdest; }
	const Point& nav_get_curcoord(void) const { return m_navcur; }
	double nav_get_curdist(void) const { return m_navcurdist; }
	int32_t nav_get_curalt(void) const { return m_navcuralt; }
	uint16_t nav_get_curflags(void) const { return m_navcuraltflags; }
	double nav_get_track_true(void) const;
	double nav_get_track_mag(void) const;
	double nav_get_xtk(void) const { return m_navcurxtk; }
	double nav_get_maxxtk(void) const { return m_navmaxxtk; }
	int32_t nav_get_trkerr(void) const { return m_navcurtrackerr; }
	double nav_get_glideslope(void) const { return m_navgs; }
	double nav_get_roc(void) const { return m_navroc; }
	double nav_get_timetowpt(void) const { return m_navtimetowpt; }
	time_t nav_get_lastwpttime(void) const { return m_navlastwpttime; }
	uint16_t nav_get_wptnr(void) const { return nav_is_fpl() ? m_navcurwpt : ~0U; }
	double nav_get_hold_track(void) const { return m_navholdtrack * track_to_deg; }
	time_t nav_get_flighttime(void) const;
	bool nav_is_flighttime_running(void) const { return m_navflighttimerunning; }
	double nav_get_tkoffldgspeed(void) const { return m_tkoffldgspeed; }

	bool log_is_open(void) const;
	void log_open(void);
	void log_close(void);

  protected:
	static constexpr double track_to_deg = 90.0 / (1 << 30);
	static constexpr double deg_to_track = (1 << 30) / 90.0;
	static constexpr double track_to_rad = M_PI_2 / (1 << 30);
	static constexpr double rad_to_track = (1 << 30) / M_PI_2;

	Engine *m_engine;
	Aircraft m_aircraft;
	Glib::ustring m_aircraftfile;
	typedef std::vector<Glib::RefPtr<SensorInstance> > sensors_t;
	sensors_t m_sensors;
	sigc::connection m_sensorsupdate;
	sigc::signal<void,change_t> m_sensorschange;
	sigc::signal<void,const MapAircraft&> m_mapaircraft;
	Glib::ustring m_name;
	Glib::ustring m_description;
	sigc::signal<void,Glib::TimeVal,loglevel_t,const Glib::ustring&> m_logmessage;
	AirData<float> m_airdata;
	WMM m_wmm;
	FPlanRoute m_route;
	TracksDb::Track m_track;
	Glib::RefPtr<Engine::ElevationMapResult> m_elevation;
	Glib::RefPtr<Engine::ElevationMapResult> m_elevationquery;
	Glib::KeyFile m_configfile;
	std::string m_configfilename;
	bool m_configfiledirty;
	bool m_altimeterstd;
	iasmode_t m_iasmode;
	bool m_positionok;
	bool m_manualhdgtrue;
	Glib::TimeVal m_positiontime;
	Glib::TimeVal m_alttime;
	Glib::TimeVal m_coursetime;
	Glib::TimeVal m_headingtime;
	Glib::TimeVal m_attitudetime;
	Point m_position;
	double m_altrate;
	double m_crsmag;
	double m_crstrue;
	double m_groundspeed;
	double m_hdgmag;
	double m_hdgtrue;
	double m_pitch;
	double m_bank;
	double m_slip;
	double m_rate;
	double m_manualhdg;
	Aircraft::Cruise::EngineParams m_cruise;
	Glib::TimeVal m_fueltime;
	double m_fuel;
	double m_fuelflow;
	double m_bhp;
	Glib::ustring m_navbrg2dest;
	Glib::ustring m_navcurdest;
	AirportsDb::Airport::FASNavigate m_navfas;
	double m_tkoffldgspeed;
	double m_navbrg2tracktrue;
	double m_navbrg2trackmag;
	double m_navbrg2dist;
	double m_navcurdist;
	double m_navcurxtk;
	double m_navmaxxtk;
	double m_navgs;
	double m_navroc;
	double m_navtimetowpt;
	Point m_navbrg2;
	Point m_navcur;
	Point m_navnext;
	Rect m_navblock;
	int32_t m_navcuralt;
	uint32_t m_navcurtrack;
	uint32_t m_navnexttrack;
	uint32_t m_navholdtrack;
	int32_t m_navcurtrackerr;
	uint32_t m_navnexttrackerr;
	uint16_t m_navcuraltflags;
	uint16_t m_navcurwpt;
	time_t m_navlastwpttime;
	time_t m_navflighttime;
	bool m_navflighttimerunning;
	bool m_navbrg2ena;
	typedef enum {
		navmode_off,
		navmode_direct,
		navmode_directhold,
		navmode_fpl,
		navmode_fplhold,
		navmode_finalapproach
	} navmode_t;
	navmode_t m_navmode;

	std::ofstream m_logfile;
	Glib::TimeVal m_logtime;

	void initacft(void);
	void update_sensors_core(change_t changemask);
	void update_elevation_map(void);
	change_t update_navigation(change_t changemask = (change_t)(change_position | change_altitude | change_heading | change_attitude | change_airdata));
	void recompute_wptextra(void);
	void recompute_times(unsigned int wpt);
	void close_track(void);

	void log_internal_open(void);
	void log_internal_close(void);
	void log_writerecord(void);
};

inline Sensors::change_t& operator|=(Sensors::change_t& a, Sensors::change_t b) { return a = (Sensors::change_t)(a | b); }
inline Sensors::change_t& operator&=(Sensors::change_t& a, Sensors::change_t b) { return a = (Sensors::change_t)(a & b); }
inline Sensors::change_t& operator^=(Sensors::change_t& a, Sensors::change_t b) { return a = (Sensors::change_t)(a ^ b); }
inline Sensors::change_t operator|(Sensors::change_t a, Sensors::change_t b) { a |= b; return a; }
inline Sensors::change_t operator&(Sensors::change_t a, Sensors::change_t b) { a &= b; return a; }
inline Sensors::change_t operator^(Sensors::change_t a, Sensors::change_t b) { a ^= b; return a; }

inline Sensors::mapangle_t& operator|=(Sensors::mapangle_t& a, Sensors::mapangle_t b) { return a = (Sensors::mapangle_t)(a | b); }
inline Sensors::mapangle_t& operator&=(Sensors::mapangle_t& a, Sensors::mapangle_t b) { return a = (Sensors::mapangle_t)(a & b); }
inline Sensors::mapangle_t& operator^=(Sensors::mapangle_t& a, Sensors::mapangle_t b) { return a = (Sensors::mapangle_t)(a ^ b); }
inline Sensors::mapangle_t operator|(Sensors::mapangle_t a, Sensors::mapangle_t b) { a |= b; return a; }
inline Sensors::mapangle_t operator&(Sensors::mapangle_t a, Sensors::mapangle_t b) { a &= b; return a; }
inline Sensors::mapangle_t operator^(Sensors::mapangle_t a, Sensors::mapangle_t b) { a ^= b; return a; }

#endif /* SENSORS_H */
