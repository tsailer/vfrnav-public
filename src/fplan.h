//
// C++ Interface: fplan
//
// Description: 
//
//
// Author: Thomas Sailer <t.sailer@alumni.ethz.ch>, (C) 2007, 2012, 2013, 2014, 2015, 2016
//
// Copyright: See COPYING file that comes with this distribution
//
//

#ifndef FPLAN_H
#define FPLAN_H

#include <limits>
#include <sqlite3x.hpp>
#include <glibmm.h>
#include <time.h>
#include "aircraft.h"
#include "geom.h"

#define USERDBPATH ".vfrnav"

namespace DbBaseElements {
	class Airport;
	class Airspace;
	class Navaid;
	class Waypoint;
	class Mapelement;
}
template <class T> class DbQueryInterface;
class TopoDb30;
class Engine;
template<typename T> class IcaoAtmosphere;
class NWXWeather;
class GRIB2;
class FPlanLeg;
class FPlan;

namespace DbBaseElements {
	class Airport;
};
template <class T> class DbQueryInterface;

class FPlanWaypoint {
public:
	static const time_t unix_minus_palmos_epoch = 24472 * 24 * 60 * 60;
	static const uint16_t altflag_standard = 1 << 0;
	static const uint16_t altflag_climb = 1 << 1;
	static const uint16_t altflag_descent = 1 << 2;
	static const uint16_t altflag_generated = 1 << 3;
	static const uint16_t altflag_altvfr = 1 << 11;
	static const uint16_t altflag_partialstandardroute = 1 << 12;
	static const uint16_t altflag_standardroute = 1 << 13;
	static const uint16_t altflag_turnpoint = 1 << 14;
	static const uint16_t altflag_ifr = 1 << 15;
	static const int32_t invalid_altitude;
	static constexpr float ft_to_m = 0.3048;
	static constexpr float m_to_ft = 1.0 / 0.3048;
	static constexpr float to_rad = M_PI / (1 << 15);
	static constexpr float to_deg = 180.0 / (1 << 15);
	static constexpr float from_rad = (1 << 15) / M_PI;
	static constexpr float from_deg = (1 << 15) / 180.0;
	static const std::string reader_fields;

	typedef enum {
		pathcode_none,
		pathcode_sid,
		pathcode_star,
		pathcode_approach,
		pathcode_transition,
		pathcode_vfrdeparture,
		pathcode_vfrarrival,
		pathcode_vfrtransition,
		pathcode_airway,
		pathcode_directto
	} pathcode_t;

	typedef enum {
		navaid_invalid,
		navaid_vor,
		navaid_vordme,
		navaid_dme,
		navaid_ndb,
		navaid_ndbdme,
		navaid_vortac,
		navaid_tacan
	} navaid_type_t;

	typedef enum {
		type_airport = 0,
		type_navaid = 1,
		type_intersection = 2,
		type_mapelement = 3,
		type_undefined = 4,
		type_fplwaypoint = 5,
		type_vfrreportingpt = 6,
		type_user = 7
	} type_t;

	FPlanWaypoint(void);
	FPlanWaypoint(sqlite3x::sqlite3_cursor& cursor);

	const Glib::ustring& get_icao(void) const { return m_icao; }
	void set_icao(const Glib::ustring& t) { m_icao = t; m_dirty = true; }
	const Glib::ustring& get_name(void) const { return m_name; }
	void set_name(const Glib::ustring& t) { m_name = t; m_dirty = true; }
	Glib::ustring get_icao_name(void) const;
	const Glib::ustring& get_icao_or_name(void) const;
	const Glib::ustring& get_ident(void) const;
	const Glib::ustring& get_pathname(void) const { return m_pathname; }
	void set_pathname(const Glib::ustring& t) { m_pathname = t; m_dirty = true; }
	pathcode_t get_pathcode(void) const { return m_pathcode; }
	void set_pathcode(pathcode_t pc = pathcode_none) { m_pathcode = pc; m_dirty = true; }
	static const std::string& get_pathcode_name(pathcode_t pc);
	const std::string& get_pathcode_name(void) const { return get_pathcode_name(get_pathcode()); }
	bool set_pathcode_name(const std::string& pc);
	static bool is_stay(const Glib::ustring& pn, unsigned int& nr, unsigned int& tm);
	static bool is_stay(const Glib::ustring& pn);
	bool is_stay(unsigned int& nr, unsigned int& tm) const;
	bool is_stay(void) const;
	const Glib::ustring& get_note(void) const { return m_note; }
	void set_note(const Glib::ustring& t) { m_note = t; m_dirty = true; }
	uint32_t get_time(void) const { return m_time; }
	void set_time(uint32_t t) { m_time = t; m_dirty = true; }
	time_t get_time_unix(void) const { return m_time - unix_minus_palmos_epoch; }
	void set_time_unix(time_t t) { m_time = t + unix_minus_palmos_epoch; m_dirty = true; }
	uint32_t get_save_time(void) const { return m_savetime; }
	void set_save_time(uint32_t t) { m_savetime = t; m_dirty = true; }
	time_t get_save_time_unix(void) const { return m_savetime - unix_minus_palmos_epoch; }
	void set_save_time_unix(time_t t) { m_savetime = t + unix_minus_palmos_epoch; m_dirty = true; }
	uint32_t get_flighttime(void) const { return m_flighttime; }
	void set_flighttime(uint32_t t) { m_flighttime = t; m_dirty = true; }
	uint32_t get_frequency(void) const { return m_frequency; }
	void set_frequency(uint32_t f) { m_frequency = f; m_dirty = true; }
	Point get_coord(void) const { return m_coord; }
	Point::coord_t get_lon(void) const { return m_coord.get_lon(); }
	Point::coord_t get_lat(void) const { return m_coord.get_lat(); }
	void set_coord(const Point& c) { m_coord = c; m_dirty = true; }
	void set_lon(Point::coord_t lo) { m_coord.set_lon(lo); m_dirty = true; }
	void set_lat(Point::coord_t la) { m_coord.set_lat(la); m_dirty = true; }
	void set_lon_rad(float lo) { m_coord.set_lon_rad(lo); m_dirty = true; }
	void set_lat_rad(float la) { m_coord.set_lat_rad(la); m_dirty = true; }
	void set_lon_deg(float lo) { m_coord.set_lon_deg(lo); m_dirty = true; }
	void set_lat_deg(float la) { m_coord.set_lat_deg(la); m_dirty = true; }
	int32_t get_altitude(void) const { return m_alt; }
	void set_altitude(int32_t a) { m_alt = a; m_dirty = true; }
	void unset_altitude(void) { set_altitude(invalid_altitude); }
	bool is_altitude_valid(void) const { return get_altitude() != invalid_altitude; }
	static std::string get_fpl_altstr(int32_t a, uint16_t f);
	std::string get_fpl_altstr(void) const { return get_fpl_altstr(get_altitude(), get_flags()); }
	uint16_t get_flags(void) const { return m_flags; }
	void set_flags(uint16_t f) { m_flags = f; m_dirty = true; }
	void frob_flags(uint16_t f, uint16_t m) { m_flags = (m_flags & ~m) ^ f; m_dirty = true; }
	bool is_standard(void) const { return !!(get_flags() & altflag_standard); }
	bool is_climb(void) const { return !!(get_flags() & altflag_climb); }
	bool is_descent(void) const { return !!(get_flags() & altflag_descent); }
	bool is_altvfr(void) const { return !!(get_flags() & altflag_altvfr); }
	bool is_altvfr_not_ifr(void) const { return !((get_flags() ^ altflag_altvfr) & (altflag_altvfr|altflag_ifr)); }
	bool is_partialstandardroute(void) const { return !!(get_flags() & altflag_partialstandardroute); }
	bool is_standardroute(void) const { return !!(get_flags() & altflag_standardroute); }
	bool is_turnpoint(void) const { return !!(get_flags() & altflag_turnpoint); }
	bool is_ifr(void) const { return !!(get_flags() & altflag_ifr); }
	uint16_t get_winddir(void) const { return m_winddir; }
	float get_winddir_deg(void) const { return get_winddir() * to_deg; }
	float get_winddir_rad(void) const { return get_winddir() * to_rad; }
	void set_winddir(uint16_t dir) { m_winddir = dir; m_dirty = true; }
	void set_winddir_deg(float dir) { set_winddir(dir * from_deg); }
	void set_winddir_rad(float dir) { set_winddir(dir * from_rad); }
	uint16_t get_windspeed(void) const { return m_windspeed; }
	float get_windspeed_kts(void) const { return get_windspeed() * (1.f / 64.f); }
	void set_windspeed(uint16_t speed) { m_windspeed = speed; m_dirty = true; }
	void set_windspeed_kts(float speed) { set_windspeed(std::min(speed * 64.f, 65535.f)); }
	uint16_t get_qff(void) const { return m_qff; }
	float get_qff_hpa(void) const { return get_qff() * (1.f / 32.f); }
	void set_qff(uint16_t press) { m_qff = press; m_dirty = true; }
	void set_qff_hpa(float press) { set_qff(std::min(press * 32.f, 65535.f)); }
	int16_t get_isaoffset(void) const { return m_isaoffset; }
	float get_isaoffset_kelvin(void) const { return get_isaoffset() * (1.f / 256.f); }
	void set_isaoffset(int16_t temp) { m_isaoffset = temp; m_dirty = true; }
	void set_isaoffset_kelvin(float temp) { set_isaoffset(std::min(std::max(temp * 256.f, -32768.f), 32767.f)); }
	float get_sltemp_kelvin(void) const;
	void set_sltemp_kelvin(float temp);
	float get_oat_kelvin(void) const;
	void set_oat_kelvin(float temp);
	// these are computed from the desired altitude
	float get_pressure_altitude(void) const;
	float get_true_altitude(void) const;
	float get_density_altitude(void) const;
	// this is the true altitude achieved according to the aircraft climb model, etc.
	int32_t get_truealt(void) const { return m_truealt; }
	void set_truealt(int32_t ta) { m_truealt = ta; m_dirty = true; }
	int32_t get_terrain(void) const { return m_terrain; }
	void set_terrain(int32_t ta) { m_terrain = ta; m_dirty = true; }
	void unset_terrain(void) { set_terrain(invalid_altitude); }
	bool is_terrain_valid(void) const { return get_terrain() != invalid_altitude; }
	int32_t get_msa(void) const;
	uint16_t get_truetrack(void) const { return m_tt; }
	float get_truetrack_deg(void) const { return get_truetrack() * to_deg; }
	float get_truetrack_rad(void) const { return get_truetrack() * to_rad; }
	void set_truetrack(uint16_t tt) { m_tt = tt; m_dirty = true; }
	void set_truetrack_deg(float tt) { set_truetrack(tt * from_deg); }
	void set_truetrack_rad(float tt) { set_truetrack(tt * from_rad); }
	uint16_t get_trueheading(void) const { return m_th; }
	float get_trueheading_deg(void) const { return get_trueheading() * to_deg; }
	float get_trueheading_rad(void) const { return get_trueheading() * to_rad; }
	void set_trueheading(uint16_t th) { m_th = th; m_dirty = true; }
	void set_trueheading_deg(float th) { set_trueheading(th * from_deg); }
	void set_trueheading_rad(float th) { set_trueheading(th * from_rad); }
	uint16_t get_magnetictrack(void) const { return get_truetrack() - get_declination(); }
	float get_magnetictrack_deg(void) const { return get_magnetictrack() * to_deg; }
	float get_magnetictrack_rad(void) const { return get_magnetictrack() * to_rad; }
	uint16_t get_magneticheading(void) const { return get_trueheading() - get_declination(); }
	float get_magneticheading_deg(void) const { return get_magneticheading() * to_deg; }
	float get_magneticheading_rad(void) const { return get_magneticheading() * to_rad; }
	uint16_t get_declination(void) const { return m_decl; }
	float get_declination_deg(void) const { return get_declination() * to_deg; }
	float get_declination_rad(void) const { return get_declination() * to_rad; }
	void set_declination(uint16_t decl) { m_decl = decl; m_dirty = true; }
	void set_declination_deg(float decl) { set_declination(decl * from_deg); }
	void set_declination_rad(float decl) { set_declination(decl * from_rad); }
	uint32_t get_dist(void) const { return m_dist; }
	float get_dist_nmi(void) const { return get_dist() * (1.0 / 256.0); }
	void set_dist(uint32_t d) { m_dist = d; m_dirty = true; }
	void set_dist_nmi(float d) { set_dist(Point::round<uint32_t,float>(d * 256.0)); }
	uint32_t get_mass(void) const { return m_mass; }
	float get_mass_kg(void) const { return get_mass() * (1.0 / 1024.0); }
	void set_mass(uint32_t f) { m_mass = f; m_dirty = true; }
	void set_mass_kg(float f) { set_mass(Point::round<uint32_t,float>(f * 1024.0)); }
	uint32_t get_fuel(void) const { return m_fuel; }
	float get_fuel_usg(void) const { return get_fuel() * (1.0 / 256.0); }
	void set_fuel(uint32_t f) { m_fuel = f; m_dirty = true; }
	void set_fuel_usg(float f) { set_fuel(Point::round<uint32_t,float>(f * 256.0)); }
	uint16_t get_tas(void) const { return m_tas; }
	float get_tas_kts(void) const { return get_tas() * (1.0 / 16.0); }
	void set_tas(uint16_t tas) { m_tas = tas; m_dirty = true; }
	void set_tas_kts(uint16_t tas) { set_tas(Point::round<uint16_t,float>(tas * 16.0)); }
	const Glib::ustring& get_engineprofile(void) const { return m_engineprofile; }
	void set_engineprofile(const Glib::ustring& s) { m_engineprofile = s; m_dirty = true; }
	uint32_t get_bhp_raw(void) const { return m_bhp; }
	void set_bhp_raw(uint32_t b) { m_bhp = b; m_dirty = true; }
	float get_bhp(void) const { return m_bhp * (1.0 / 256.); }
	void set_bhp(float b) { set_bhp_raw(Point::round<uint32_t,float>(b * 256.0)); }
	uint16_t get_rpm(void) const { return m_rpm; }
	void set_rpm(uint16_t rpm) { m_rpm = rpm; m_dirty = true; }
	uint16_t get_mp(void) const { return m_mp; }
	float get_mp_inhg(void) const { return get_mp() * (1.0 / 256.0); }
	void set_mp(uint16_t mp) { m_mp = mp; m_dirty = true; }
	void set_mp_inhg(float mp) { set_mp(Point::round<uint16_t,float>(mp * 256.0)); }
	type_t get_type(void) const { return m_type; }
	navaid_type_t get_navaid(void) const { return m_navaid; }
	void set_type(type_t typ = type_undefined, navaid_type_t navaid = navaid_invalid) { m_type = typ; m_navaid = navaid; m_dirty = true; }
	static const std::string& get_type_string(type_t typ, navaid_type_t navaid = navaid_invalid);
	const std::string& get_type_string(void) const { return get_type_string(get_type(), get_navaid()); }
	bool set_type_string(const std::string& x);

	void save_time(void) { m_savetime = m_time; m_dirty = true; }
	void restore_time(void) { m_time = m_savetime; m_dirty = true; }

	bool enforce_pathcode_vfrifr(void);

	static char get_ruleschar(uint16_t flags);
	char get_ruleschar(void) const { return get_ruleschar(get_flags()); }
	static char get_altchar(uint16_t flags);
	char get_altchar(void) const { return get_ruleschar(get_flags()); }

	void set_airport(const DbBaseElements::Airport& el);
	void set_navaid(const DbBaseElements::Navaid& el);
	void set_waypoint(const DbBaseElements::Waypoint& el);
	void set_mapelement(const DbBaseElements::Mapelement& el);
	void set_wpt(const FPlanWaypoint& el);

	void set(const DbBaseElements::Airport& el) { set_airport(el); }
	void set(const DbBaseElements::Navaid& el) { set_navaid(el); }
	void set(const DbBaseElements::Waypoint& el) { set_waypoint(el); }
	void set(const DbBaseElements::Mapelement& el) { set_mapelement(el); }
	void set(const FPlanWaypoint& el) { set_wpt(el); }

	std::string to_str(void) const;

	DbBaseElements::Airport find_airport(DbQueryInterface<DbBaseElements::Airport>& db) const;
	DbBaseElements::Mapelement find_mapelement(DbQueryInterface<DbBaseElements::Mapelement>& db, double maxdist = 100, uint64_t minarea = 0,
						   bool prefer_area = true, const Point& ptanchor = Point::invalid) const;

	template<class Archive> void hibernate_binary(Archive& ar) {
		ar.io(m_icao);
		ar.io(m_name);
		ar.io(m_pathname);
		ar.io(m_note);
		ar.io(m_engineprofile);
		ar.io(m_time);
		ar.io(m_savetime);
		ar.io(m_flighttime);
		ar.io(m_frequency);
		m_coord.hibernate_binary(ar);
		ar.io(m_alt);
		ar.io(m_flags);
		ar.io(m_winddir);
		ar.io(m_windspeed);
		ar.io(m_qff);
		ar.io(m_isaoffset);
		ar.iouint8(m_pathcode);
		ar.io(m_dist);
		ar.io(m_mass);
		ar.io(m_fuel);
		ar.io(m_truealt);
		ar.io(m_terrain);
		ar.io(m_tt);
		ar.io(m_th);
		ar.io(m_decl);
		ar.io(m_tas);
		ar.io(m_bhp);
		ar.io(m_rpm);
		ar.io(m_mp);
		ar.iouint8(m_type);
		ar.iouint8(m_navaid);
	}

protected:
	Glib::ustring m_icao;
	Glib::ustring m_name;
	Glib::ustring m_pathname;
	Glib::ustring m_note;
	Glib::ustring m_engineprofile;
	Point m_coord;
	uint32_t m_time;
	uint32_t m_savetime;
	uint32_t m_flighttime;
	uint32_t m_frequency;
	int32_t m_alt;
	int32_t m_truealt;
	int32_t m_terrain;
	uint32_t m_dist;
	uint32_t m_mass;
	uint32_t m_fuel;
	uint32_t m_bhp;
	uint16_t m_flags;
	uint16_t m_winddir;
	uint16_t m_windspeed;
	uint16_t m_qff;
	int16_t m_isaoffset;
	uint16_t m_tt;
	uint16_t m_th;
	uint16_t m_decl;
	uint16_t m_tas;
	uint16_t m_rpm;
	uint16_t m_mp;
	pathcode_t m_pathcode;
	type_t m_type;
	navaid_type_t m_navaid;
	bool m_dirty;

	friend class FPlan;
	friend class FPlanRoute;
};

class FPlanAlternate : public FPlanWaypoint {
public:
	FPlanAlternate(void);
	FPlanAlternate(const FPlanWaypoint& w);

	int32_t get_cruisealtitude(void) const { return m_cruisealt; }
	void set_cruisealtitude(int32_t a) { m_cruisealt = a; m_dirty = true; }
	void unset_cruisealtitude(void) { set_cruisealtitude(invalid_altitude); }
	bool is_cruisealtitude_valid(void) const { return get_cruisealtitude() != invalid_altitude; }

	int32_t get_holdaltitude(void) const { return m_holdalt; }
	void set_holdaltitude(int32_t a) { m_holdalt = a; m_dirty = true; }
	void unset_holdaltitude(void) { set_holdaltitude(invalid_altitude); }
	bool is_holdaltitude_valid(void) const { return get_holdaltitude() != invalid_altitude; }
	uint32_t get_holdtime(void) const { return m_holdtime; }
	void set_holdtime(uint32_t t) { m_holdtime = t; m_dirty = true; }
	uint32_t get_holdfuel(void) const { return m_holdfuel; }
	float get_holdfuel_usg(void) const { return get_holdfuel() * (1.0 / 256.0); }
	void set_holdfuel(uint32_t f) { m_holdfuel = f; m_dirty = true; }
	void set_holdfuel_usg(float f) { set_holdfuel(Point::round<uint32_t,float>(f * 256.0)); }

	uint32_t get_totaltime(void) const { return get_flighttime() + get_holdtime(); }
	uint32_t get_totalfuel(void) const { return get_fuel() + get_holdfuel(); }
	float get_totalfuel_usg(void) const { return get_totalfuel() * (1.0 / 256.0); }

	template<class Archive> void hibernate_binary(Archive& ar) {
		FPlanWaypoint::hibernate_binary(ar);
		ar.io(m_cruisealt);
		ar.io(m_holdalt);
		ar.io(m_holdtime);
		ar.io(m_holdfuel);
	}

protected:
	int32_t m_cruisealt;
	int32_t m_holdalt;
	uint32_t m_holdtime;
	uint32_t m_holdfuel;
};

class FPlanLeg {
public:
	typedef enum {
		legtype_invalid,
		legtype_offblock_takeoff,
		legtype_landing_onblock,
		legtype_normal,
	} legtype_t;

	FPlanLeg(void);
	FPlanLeg(const Glib::ustring& fromicao, const Glib::ustring& fromname, const Point& fromcoord,
		 int16_t fromalt, uint16_t fromflags, uint32_t fromtime, uint32_t fromfreq,
		 const Glib::ustring& toicao, const Glib::ustring& toname, const Point& tocoord,
		 int16_t toalt, uint16_t toflags, uint32_t totime, uint32_t tofreq,
		 float winddir, float windspeed, float qnh, float isaoffset,
		 legtype_t legtype, bool future);

	void wind_correction(float vcruise_kts = 100.0);
	static int16_t baro_correction(float qnh, float isaoffset, int16_t alt, uint16_t altflags);
	void update(time_t curtime, const Point& coord = Point(), bool coordvalid = false, int16_t alt = 0, bool altvalid = false, float vgnd = 0.0 );

	const Glib::ustring& get_fromicao(void) const { return m_fromicao; }
	const Glib::ustring& get_fromname(void) const { return m_fromname; }
	Glib::ustring get_fromicaoname(void) const;
	const Glib::ustring& get_toicao(void) const { return m_toicao; }
	const Glib::ustring& get_toname(void) const { return m_toname; }
	Glib::ustring get_toicaoname(void) const;
	uint32_t get_fromtime(void) const { return m_fromtime; }
	time_t get_fromtime_unix(void) const { return m_fromtime - FPlanWaypoint::unix_minus_palmos_epoch; }
	uint32_t get_totime(void) const { return m_fromtime + m_legtime; }
	time_t get_totime_unix(void) const { return m_fromtime + m_legtime - FPlanWaypoint::unix_minus_palmos_epoch; }
	uint32_t get_legtime(void) const { return m_legtime; }
	Point get_fromcoord(void) const { return m_fromcoord; }
	Point get_tocoord(void) const { return m_tocoord; }
	int16_t get_fromaltitude(void) const { return m_fromalt; }
	int16_t get_toaltitude(void) const { return m_toalt; }
	uint16_t get_fromflags(void) const { return m_fromflags; }
	uint16_t get_toflags(void) const { return m_toflags; }
	int16_t get_fromtruealtitude(void) const { return m_fromtruealt; }
	int16_t get_totruealtitude(void) const { return m_totruealt; }
	uint32_t get_fromfrequency(void) const { return m_fromfreq; }
	uint32_t get_tofrequency(void) const { return m_tofreq; }

	float get_dist(void) const { return m_dist; }
	float get_vdist(void) const { return m_vdist; }
	float get_truetrack(void) const { return m_truetrack * to_deg; }
	float get_variance(void) const { return m_var * to_deg; }
	float get_wca(void) const { return m_wca * to_deg; }
	float get_heading(void) const { return (m_truetrack + m_var + m_wca) * to_deg; }

	bool is_future(void) const { return m_future; }
	legtype_t get_legtype(void) const { return m_legtype; }

	float get_crosstrack(void) const { return m_crosstrack; }
	float get_disttodest(void) const { return m_disttodest; }
	float get_legfraction(void) const { return m_legfraction * (1.0 / (1U << 15)); }
	int16_t get_altdiff(void) const { return m_altdiff; }
	int16_t get_referencealt(void) const { return m_referencealt; }
	int16_t get_mapalt(void) const { return m_mapalt; }
	float get_coursetodest(void) const { return m_coursetodest * to_deg; }
	Point get_mapcoord(void) const { return m_mapcoord; }
	bool is_gpsvalid(void) const { return m_gpsvalid; }
	bool is_gpsaltvalid(void) const { return m_gpsaltvalid; }
	float get_vgnd(void) const { return m_vgnd; }
	float get_verticalspeed(void) const;
	float get_verticalspeedtodest(void) const;

	static constexpr float from_rad = (1 << 15) / M_PI;
	static constexpr float to_rad = M_PI / (1 << 15);
	static constexpr float from_deg = (1 << 15) / 180.0;
	static constexpr float to_deg = 180.0 / (1 << 15);

private:
	// Static Data
	Glib::ustring m_fromicao, m_toicao;
	Glib::ustring m_fromname, m_toname;
	Point m_fromcoord, m_tocoord;
	int16_t m_fromalt, m_toalt;
	int16_t m_fromtruealt, m_totruealt;
	uint16_t m_fromflags, m_toflags;
	uint32_t m_fromfreq, m_tofreq;
	uint32_t m_fromtime, m_legtime;
	float m_winddir;
	float m_windspeed;
	float m_dist;
	float m_vdist; /* speed (cruise+wind) projected onto the line connecting the two endpoints */
	uint16_t m_truetrack;  /* 2^16 = 360deg = 2pi rad */
	uint16_t m_var;  /* 2^16 = 360deg = 2pi rad */
	uint16_t m_wca;  /* wind correction angle, 2^16 = 360deg = 2pi rad */
	legtype_t m_legtype;
	bool m_future;
	// Dynamic Data
	float m_crosstrack;
	float m_disttodest;
	float m_vgnd;
	uint16_t m_legfraction;
	int16_t m_altdiff;
	int16_t m_referencealt;
	int16_t m_mapalt;
	Point m_mapcoord;
	uint16_t m_coursetodest;
	bool m_gpsvalid;
	bool m_gpsaltvalid;

	void wind_correction(float vcruise_kts, float winddir_deg, float vwind_kts);
	void baro_correction(float qnh, float isaoffset);
};

class FPlanRoute {
public:
	class GFSResult {
	public:
		GFSResult(void);
		gint64 get_minreftime(void) const { return m_minreftime; }
		gint64 get_maxreftime(void) const { return m_maxreftime; }
		gint64 get_minefftime(void) const { return m_minefftime; }
		gint64 get_maxefftime(void) const { return m_maxefftime; }
		bool is_reftime_valid(void) const { return get_minreftime() <= get_maxreftime(); }
		bool is_efftime_valid(void) const { return get_minefftime() <= get_maxefftime(); }
		bool is_modified(void) const { return m_modified; }
		bool is_partial(void) const { return m_partial; }
		void set_modified(bool x = true) { m_modified = x; }
		void set_partial(bool x = true) { m_partial = x; }
		void add_reftime(gint64 x) { m_minreftime = std::min(m_minreftime, x); m_maxreftime = std::max(m_maxreftime, x); }
		void add_efftime(gint64 x) { m_minefftime = std::min(m_minefftime, x); m_maxefftime = std::max(m_maxefftime, x); }
		void add(const GFSResult& r);

	protected:
		gint64 m_minreftime;
		gint64 m_maxreftime;
		gint64 m_minefftime;
		gint64 m_maxefftime;
		bool m_modified;
		bool m_partial;
	};

	typedef int32_t id_t;
	FPlanRoute(FPlan& fpp);

	id_t get_id(void) const { return m_id; }
	const Glib::ustring& get_note(void) const { return m_note; }
	void set_note(const Glib::ustring& t) { m_note = t; m_dirty = true; }
	uint16_t get_nrwpt(void) const { return m_wpts.size(); }
	uint16_t get_curwpt(void) const { return m_curwpt; }
	void set_curwpt(uint16_t cw) { m_curwpt = cw; m_dirty = true; }
	FPlanWaypoint& operator[](uint32_t nr);
	const FPlanWaypoint& operator[](uint32_t nr) const;
	FPlanWaypoint& get_wpt(uint32_t nr) { return operator[](nr); }
	const FPlanWaypoint& get_wpt(uint32_t nr) const { return operator[](nr); }
	FPlanWaypoint& get_wpt(void) { return get_wpt(m_curwpt); }
	const FPlanWaypoint& get_wpt(void) const { return get_wpt(m_curwpt); }
	void insert_wpt(uint32_t nr = ~0, const FPlanWaypoint& wpt = FPlanWaypoint());
	void delete_wpt(uint32_t nr);
	void clear_wpt(void);
	uint32_t get_time_offblock(void) const { return m_time_offblock; }
	void set_time_offblock(uint32_t t) { m_time_offblock = t; m_dirty = true; }
	time_t get_time_offblock_unix(void) const { return m_time_offblock - FPlanWaypoint::unix_minus_palmos_epoch; }
	void set_time_offblock_unix(time_t t) { m_time_offblock = t + FPlanWaypoint::unix_minus_palmos_epoch; m_dirty = true; }
	uint32_t get_save_time_offblock(void) const { return m_savetime_offblock; }
	void set_save_time_offblock(uint32_t t) { m_savetime_offblock = t; m_dirty = true; }
	time_t get_save_time_offblock_unix(void) const { return m_savetime_offblock - FPlanWaypoint::unix_minus_palmos_epoch; }
	void set_save_time_offblock_unix(time_t t) { m_savetime_offblock = t + FPlanWaypoint::unix_minus_palmos_epoch; m_dirty = true; }
	uint32_t get_time_onblock(void) const { return m_time_onblock; }
	void set_time_onblock(uint32_t t) { m_time_onblock = t; m_dirty = true; }
	time_t get_time_onblock_unix(void) const { return m_time_onblock - FPlanWaypoint::unix_minus_palmos_epoch; }
	void set_time_onblock_unix(time_t t) { m_time_onblock = t + FPlanWaypoint::unix_minus_palmos_epoch; m_dirty = true; }
	uint32_t get_save_time_onblock(void) const { return m_savetime_onblock; }
	void set_save_time_onblock(uint32_t t) { m_savetime_onblock = t; m_dirty = true; }
	time_t get_save_time_onblock_unix(void) const { return m_savetime_onblock - FPlanWaypoint::unix_minus_palmos_epoch; }
	void set_save_time_onblock_unix(time_t t) { m_savetime_onblock = t + FPlanWaypoint::unix_minus_palmos_epoch; m_dirty = true; }

	void save_time_offblock(void) { m_savetime_offblock = m_time_offblock; m_dirty = true; }
	void restore_time_offblock(void) { m_time_offblock = m_savetime_offblock; m_dirty = true; }
	void save_time_onblock(void) { m_savetime_onblock = m_time_onblock; m_dirty = true; }
	void restore_time_onblock(void) { m_time_onblock = m_savetime_onblock; m_dirty = true; }

	void save_time(uint32_t nr);
	void restore_time(uint32_t nr);
	void set_time(uint32_t nr, uint32_t t);
	void set_time_unix(uint32_t nr, time_t t) { set_time(nr, t + FPlanWaypoint::unix_minus_palmos_epoch); }
	uint32_t get_time(uint32_t nr) const;
	time_t get_time_unix(uint32_t nr) const { return get_time(nr) - FPlanWaypoint::unix_minus_palmos_epoch; }

	FPlanLeg get_leg(uint32_t nr);
	FPlanLeg get_directleg(void);

	void load_fp(void);
	void load_fp(id_t nid);
	void save_fp(void);
	void new_fp(void);
	void duplicate_fp(void);
	void delete_fp(void);
	void reverse_fp(void);
	bool load_first_fp(void);
	bool load_next_fp(void);

	float total_distance_km(void) const;
	float total_distance_nmi(void) const { return total_distance_km() * Point::km_to_nmi; }
	double total_distance_km_dbl(void) const;
	double total_distance_nmi_dbl(void) const { return total_distance_km_dbl() * Point::km_to_nmi_dbl; }
	float gc_distance_km(void) const;
	float gc_distance_nmi(void) const { return gc_distance_km() * Point::km_to_nmi; }
	double gc_distance_km_dbl(void) const;
	double gc_distance_nmi_dbl(void) const { return gc_distance_km_dbl() * Point::km_to_nmi_dbl; }
	int32_t max_altitude(void) const;
	int32_t max_altitude(uint16_t& flags) const;
	Rect get_bbox(void) const;
	Rect get_bbox(unsigned int wptidx0, unsigned int wptidx1) const;
	char get_flightrules(void) const;

	bool has_pathcodes(void) const;
	bool enforce_pathcode_vfrdeparr(void);
	bool enforce_pathcode_vfrifr(void);
	void recompute_dist(void);
	void recompute_decl(void);
	void recompute_time(void);

	void set_winddir(uint16_t dir);
	void set_winddir_deg(float dir) { set_winddir(dir * FPlanWaypoint::from_deg); }
	void set_winddir_rad(float dir) { set_winddir(dir * FPlanWaypoint::from_rad); }
	void set_windspeed(uint16_t speed);
	void set_windspeed_kts(float speed) { set_windspeed(std::min(speed * 64.f, 65535.f)); }
	void set_qff(uint16_t press);
	void set_qff_hpa(float press) { set_qff(std::min(press * 32.f, 65535.f)); }
	void set_isaoffset(int16_t temp);
	void set_isaoffset_kelvin(float temp) { set_isaoffset(std::min(std::max(temp * 256.f, -32768.f), 32767.f)); }
	void set_declination(uint16_t decl);
	void set_declination_deg(float decl) { set_declination(decl * FPlanWaypoint::from_deg); }
	void set_declination_rad(float decl) { set_declination(decl * FPlanWaypoint::from_rad); }
	void set_rpm(uint16_t rpm);
	void set_mp(uint16_t mp);
	void set_mp_inhg(float mp) { set_mp(Point::round<uint16_t,float>(mp * 256.0)); }
	void set_tas(uint16_t tas);
	void set_tas_kts(uint16_t tas) { set_tas(Point::round<uint16_t,float>(tas * 16.0)); }
	void set_tomass(uint32_t x);
	void set_tomass_kg(float f) { if (!std::isnan(f) && f >= 0) set_tomass(Point::round<uint32_t,float>(f * 1024.0)); }

	unsigned int insert(const DbBaseElements::Airport& el, uint32_t nr = ~0U);
	unsigned int insert(unsigned int vfrrtenr, const DbBaseElements::Airport& el, uint32_t nr = ~0U);
	unsigned int insert(unsigned int vfrrtenr, unsigned int vfrrteptnr, const DbBaseElements::Airport& el, uint32_t nr = ~0U);
	unsigned int insert(const DbBaseElements::Navaid& el, uint32_t nr = ~0U);
	unsigned int insert(const DbBaseElements::Waypoint& el, uint32_t nr = ~0U);
	unsigned int insert(const DbBaseElements::Mapelement& el, uint32_t nr = ~0U);
	unsigned int insert(const FPlanWaypoint& el, uint32_t nr = ~0U);

	class LevelChange {
	public:
		typedef enum {
			type_boc,
			type_toc,
			type_bod,
			type_tod,
			type_invalid
		} type_t;

		LevelChange(type_t t = type_invalid, double dist = std::numeric_limits<double>::quiet_NaN(),
			    int32_t alt = std::numeric_limits<int32_t>::min(), time_t flttime = 0);
		double get_dist(void) const { return m_dist; }
		time_t get_flttime(void) const { return m_flttime; }
		int32_t get_alt(void) const { return m_alt; }
		type_t get_type(void) const { return m_type; }

	protected:
		double m_dist;
		time_t m_flttime;
		int32_t m_alt;
		type_t m_type;
	};

	class Profile : public std::vector<LevelChange> {
	protected:
		typedef std::map<unsigned int,double> distat_t;

	public:
		Profile(void);
		double get_dist(void) const { return m_dist; }
		void set_dist(double d) { m_dist = d; }
		double get_distat(unsigned int x) const;
		void set_distat(unsigned int x, double d);
		typedef distat_t::const_iterator const_distat_iterator;
		const_distat_iterator begin_distat(void) const { return m_distat.begin(); }
		const_distat_iterator end_distat(void) const { return m_distat.end(); }

	protected:
		double m_dist;
		distat_t m_distat;
	};

	Profile recompute(const Aircraft& acft, float qnh = std::numeric_limits<float>::quiet_NaN(), float tempoffs = std::numeric_limits<float>::quiet_NaN(),
			  const Aircraft::Cruise::EngineParams& ep = Aircraft::Cruise::EngineParams(), bool insert_tocbod = false);
	Profile recompute(FPlanWaypoint& wptcenter, const Aircraft& acft, float qnh = std::numeric_limits<float>::quiet_NaN(),
			  float tempoffs = std::numeric_limits<float>::quiet_NaN(),
			  const Aircraft::Cruise::EngineParams& ep = Aircraft::Cruise::EngineParams(), bool insert_tocbod = false);
	void turnpoints(bool include_dct = true, float maxdev = 0.5f);
	void nwxweather(NWXWeather& nwx, const sigc::slot<void,bool>& cb = sigc::slot<void,bool>());
	GFSResult gfs(GRIB2& grib2);
	GFSResult gfs(GRIB2& grib2, std::vector<FPlanAlternate>& altn);
	void fix_invalid_altitudes(unsigned int nr);
	void fix_invalid_altitudes(unsigned int nr, TopoDb30& topodb, DbQueryInterface<DbBaseElements::Airspace>& aspc);
	void fix_invalid_altitudes(unsigned int nr, Engine& eng);
	void fix_invalid_altitudes(void);
	void fix_invalid_altitudes(TopoDb30& topodb, DbQueryInterface<DbBaseElements::Airspace>& aspc);
	void fix_invalid_altitudes(Engine& eng);
	void compute_terrain(TopoDb30& topodb, double corridor_nmi = 5., bool roundcaps = false);
	void delete_nocoord_waypoints(void);
	void delete_colocated_waypoints(void);
	void delete_sametime_waypoints(void);

	bool is_dirty(void) const;

	template<class Archive> void hibernate_binary(Archive& ar) {
		ar.io(m_id);
		ar.io(m_note);
		ar.io(m_time_offblock);
		ar.io(m_savetime_offblock);
		ar.io(m_time_onblock);
		ar.io(m_savetime_onblock);
		ar.io(m_curwpt);
		ar.io(m_dirty);
		uint32_t sz(m_wpts.size());
		ar.io(sz);
		if (ar.is_load())
			m_wpts.resize(sz);
		for (waypoints_t::iterator i(m_wpts.begin()), e(m_wpts.end()); i != e; ++i)
			i->hibernate_binary(ar);
	}

private:
	void set_dirty(bool d = true);

	FPlanWaypoint insert_prepare(uint32_t nr);

private:
	FPlan *m_fp;
	id_t m_id;
	Glib::ustring m_note;
	uint32_t m_time_offblock;
	uint32_t m_savetime_offblock;
	uint32_t m_time_onblock;
	uint32_t m_savetime_onblock;
	uint32_t m_curwpt;
	typedef std::vector<FPlanWaypoint> waypoints_t;
	waypoints_t m_wpts;
	bool m_dirty;

	friend class FPlan;
};

class FPlan {
public:
	typedef enum {
		comp_startswith,
		comp_contains,
		comp_exact
	} comp_t;

	FPlan(void);
	FPlan(sqlite3 *dbh);
	~FPlan() throw ();
	static Glib::ustring get_userdbpath(void);

	void delete_database(void);

	std::vector<FPlanWaypoint> find_by_name(const Glib::ustring& nm = "", unsigned int limit = ~0, unsigned int skip = 0, comp_t comp = comp_contains);
	std::vector<FPlanWaypoint> find_by_icao(const Glib::ustring& ic = "", unsigned int limit = ~0, unsigned int skip = 0, comp_t comp = comp_contains);
	std::vector<FPlanWaypoint> find_nearest(const Point& pt, unsigned int limit = ~0, unsigned int skip = 0, const Rect& rect = Rect());

private:
	sqlite3x::sqlite3_connection db;

	void init_database(void);
	sqlite3x::sqlite3_connection& get_db(void) { return db; }

	static std::string escape_string(const std::string& s);

	static void dbfunc_simpledist(sqlite3_context *ctxt, int, sqlite3_value **values);

	friend class FPlanRoute;
	friend class FPlanWaypoint;
};

const std::string& to_str(FPlanRoute::LevelChange::type_t t);
inline std::ostream& operator<<(std::ostream& os, FPlanRoute::LevelChange::type_t t) { return os << to_str(t); }

#endif /* FPLAN_H */
