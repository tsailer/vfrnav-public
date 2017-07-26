#ifndef METARTAF_H
#define METARTAF_H

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <limits>
#include <string>
#include <vector>
#include <set>

#include "sysdeps.h"
#include "geom.h"

class MetarTafSet {
public:
	class METARTAF {
	  public:
		METARTAF(time_t t = 0, const std::string& rawtxt = "");
		const std::string& get_rawtext(void) const { return m_rawtext; }
		time_t get_time(void) const { return m_time; }
		int compare(const METARTAF& x) const;
		bool operator==(const METARTAF& x) const { return compare(x) == 0; }
		bool operator!=(const METARTAF& x) const { return compare(x) != 0; }
		bool operator<(const METARTAF& x) const { return compare(x) < 0; }
		bool operator<=(const METARTAF& x) const { return compare(x) <= 0; }
		bool operator>(const METARTAF& x) const { return compare(x) > 0; }
		bool operator>=(const METARTAF& x) const { return compare(x) >= 0; }

	  protected:
		std::string m_rawtext;
		time_t m_time;
	};

	class METAR : public METARTAF {
	  public:
		typedef enum {
			frules_unknown,
			frules_lifr,
			frules_ifr,
			frules_mvfr,
			frules_vfr
		} frules_t;

		METAR(time_t t, const std::string& rawtxt, frules_t frules);
		METAR(time_t t = 0, const std::string& rawtxt = "", const std::string& frules = "");

		frules_t get_frules(void) const { return m_frules; }
		static const std::string& get_frules_str(frules_t frules);
		const std::string& get_frules_str(void) const { return get_frules_str(get_frules()); }
		static const std::string& get_latex_col(frules_t frules);
		const std::string& get_latex_col(void) const { return get_latex_col(get_frules()); }

	  protected:
		frules_t m_frules;
	};

	class TAF : public METARTAF {
	  public:
		TAF(time_t t = 0, const std::string& rawtxt = "");
	};

	class SIGMET {
	  public:
		typedef enum {
			type_sigmet,
			type_tropicalcyclone,
			type_volcanicash,
			type_invalid
		} type_t;

		SIGMET(const std::string& typ, const std::string& ctry, const std::string& dissem, const std::string& orig,
		       time_t txtime, time_t validfrom, time_t validto, int bullnr, const std::string& seq, const std::string& msg);

	        type_t get_type(void) const { return m_type; }
	        static const std::string& get_type_str(type_t t);
		const std::string& get_type_str(void) const { return get_type_str(get_type()); }
		const std::string& get_country(void) const { return m_country; }
		const std::string& get_dissemiator(void) const { return m_dissemiator; }
		const std::string& get_originator(void) const { return m_originator; }
		const std::string& get_msg(void) const { return m_msg; }
		const std::string& get_sequence(void) const { return m_sequence; }
		time_t get_transmissiontime(void) const { return m_transmissiontime; }
		time_t get_validfrom(void) const { return m_validfrom; }
		time_t get_validto(void) const { return m_validto; }
		int get_bulletinnr(void) const { return m_bulletinnr; }

		int compare(const SIGMET& x) const;
		bool operator==(const SIGMET& x) const { return compare(x) == 0; }
		bool operator!=(const SIGMET& x) const { return compare(x) != 0; }
		bool operator<(const SIGMET& x) const { return compare(x) < 0; }
		bool operator<=(const SIGMET& x) const { return compare(x) <= 0; }
		bool operator>(const SIGMET& x) const { return compare(x) > 0; }
		bool operator>=(const SIGMET& x) const { return compare(x) >= 0; }

		static std::vector<std::string> tokenize(const std::string& s);
		static bool is_latitude(const std::string& s);
		static bool is_longitude(const std::string& s);

		const MultiPolygonHole& get_poly(void) const { return m_poly; }
		void compute_poly(const MultiPolygonHole& firpoly);

	  protected:
		MultiPolygonHole m_poly;
		std::string m_country;
		std::string m_dissemiator;
		std::string m_originator;
		std::string m_msg;
		std::string m_sequence;
		time_t m_transmissiontime;
		time_t m_validfrom;
		time_t m_validto;
	        type_t m_type;
		int m_bulletinnr;
	};

	class Station {
	  public:
		Station(const std::string& stationid = "", const Point& coord = Point(),
			double elevm = std::numeric_limits<double>::quiet_NaN());
		const std::string& get_stationid(void) const { return m_stationid; }
		const Point& get_coord(void) const { return m_coord; }
		double get_elev(void) const { return m_elev; }
		unsigned int get_wmonr(void) const { return m_wmonr; }
		void set_wmonr(unsigned int w) { m_wmonr = w; }
		// valid if we have a radio sounding for it
		bool is_wmonr_valid(void) const { return get_wmonr() != ~0U; }

		typedef std::set<METAR> metar_t;
		const metar_t& get_metar(void) const { return m_metar; }
		metar_t& get_metar(void) { return m_metar; }
		typedef std::set<TAF> taf_t;
		const taf_t& get_taf(void) const { return m_taf; }
		taf_t& get_taf(void) { return m_taf; }

		int compare(const Station& x) const { return get_stationid().compare(x.get_stationid()); }
		bool operator==(const Station& x) const { return compare(x) == 0; }
		bool operator!=(const Station& x) const { return compare(x) != 0; }
		bool operator<(const Station& x) const { return compare(x) < 0; }
		bool operator<=(const Station& x) const { return compare(x) <= 0; }
		bool operator>(const Station& x) const { return compare(x) > 0; }
		bool operator>=(const Station& x) const { return compare(x) >= 0; }

	  protected:
		metar_t m_metar;
		taf_t m_taf;
		std::string m_stationid;
		Point m_coord;
		double m_elev;
		unsigned int m_wmonr;
	};

	class FIR {
	  public:
		FIR(const std::string& id = "", const MultiPolygonHole& poly = MultiPolygonHole());
		const std::string& get_ident(void) const { return m_ident; }
		const MultiPolygonHole& get_poly(void) const { return m_poly; }

		typedef std::set<SIGMET> sigmet_t;
		const sigmet_t& get_sigmet(void) const { return m_sigmet; }
		sigmet_t& get_sigmet(void) { return m_sigmet; }

		int compare(const FIR& x) const { return get_ident().compare(x.get_ident()); }
		bool operator==(const FIR& x) const { return compare(x) == 0; }
		bool operator!=(const FIR& x) const { return compare(x) != 0; }
		bool operator<(const FIR& x) const { return compare(x) < 0; }
		bool operator<=(const FIR& x) const { return compare(x) <= 0; }
		bool operator>(const FIR& x) const { return compare(x) > 0; }
		bool operator>=(const FIR& x) const { return compare(x) >= 0; }

		void compute_poly(void);

	  protected:
		sigmet_t m_sigmet;
		std::string m_ident;
		MultiPolygonHole m_poly;
	};

	typedef std::vector<Station> stations_t;
	typedef std::set<FIR> firs_t;
	
	MetarTafSet(void);
	void add_fir(const std::string& id = "", const MultiPolygonHole& poly = MultiPolygonHole());
	void loadstn_sqlite(const std::string& dbpath, const Rect& bbox,
			    unsigned int metarhistory, unsigned int tafhistory);
	void loadstn_pg(const std::string& dbpath, const Rect& bbox, time_t tmin, time_t tmax,
			unsigned int metarhistory, unsigned int tafhistory);

	const stations_t& get_stations(void) const { return m_stations; }
	stations_t& get_stations(void) { return m_stations; }
	const firs_t& get_firs(void) const { return m_firs; }
	firs_t& get_firs(void) { return m_firs; }

	void compute_poly(void);

	class OrderStadionId {
	public:
		bool operator()(const Station& a, const Station& b) const;
	};

protected:
	stations_t m_stations;
	firs_t m_firs;

	class PGTransactor;
	class PGMetarTransactor;
	class PGTafTransactor;
	class PGSigmetTransactor;
#ifdef HAVE_PQXX
	std::unique_ptr<pqxx::connection> m_conn;
#endif
};

#endif /* METARTAF_H */
