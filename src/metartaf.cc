#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <sqlite3x.hpp>

#ifdef HAVE_PQXX
#include <pqxx/connection.hxx>
#include <pqxx/transaction.hxx>
#include <pqxx/transactor.hxx>
#include <pqxx/result.hxx>
#include <pqxx/except.hxx>
#endif

#include "metartaf.hh"

MetarTafSet::METARTAF::METARTAF(time_t t, const std::string& rawtxt)
	: m_rawtext(rawtxt), m_time(t)
{
}

int MetarTafSet::METARTAF::compare(const METARTAF& x) const
{
	if (get_time() < x.get_time())
		return -1;
	if (x.get_time() < get_time())
		return 1;
	return 0;
}

MetarTafSet::METAR::METAR(time_t t, const std::string& rawtxt, frules_t frules)
	: METARTAF(t, rawtxt), m_frules(frules)
{
}

MetarTafSet::METAR::METAR(time_t t, const std::string& rawtxt, const std::string& frules)
	: METARTAF(t, rawtxt), m_frules(frules_unknown)
{
	for (frules_t f(frules_lifr); f <= frules_vfr; f = (frules_t)(f + 1)) {
		if (frules != get_frules_str(f))
			continue;
		m_frules = f;
		break;
	}
}

const std::string& MetarTafSet::METAR::get_frules_str(frules_t frules)
{
	switch (frules) {
	default:
	{
		static const std::string r("-");
		return r;
	}

	case frules_lifr:
	{
		static const std::string r("LIFR");
		return r;
	}

	case frules_ifr:
	{
		static const std::string r("IFR");
		return r;
	}

	case frules_mvfr:
	{
		static const std::string r("MVFR");
		return r;
	}

	case frules_vfr:
	{
		static const std::string r("VFR");
		return r;
	}
	}
}

const std::string& MetarTafSet::METAR::get_latex_col(frules_t frules)
{
	switch (frules) {
	default:
	{
		static const std::string r("colunknown");
		return r;
	}

	case frules_lifr:
	{
		static const std::string r("collifr");
		return r;
	}

	case frules_ifr:
	{
		static const std::string r("colifr");
		return r;
	}

	case frules_mvfr:
	{
		static const std::string r("colmvfr");
		return r;
	}

	case frules_vfr:
	{
		static const std::string r("colvfr");
		return r;
	}
	}
}

MetarTafSet::TAF::TAF(time_t t, const std::string& rawtxt)
	: METARTAF(t, rawtxt)
{
}

// SIGMET/AIRMET: http://www.paris.icao.int/documents_open/show_file.php?id=345

MetarTafSet::SIGMET::SIGMET(const std::string& typ, const std::string& ctry, const std::string& dissem, const std::string& orig,
			      time_t txtime, time_t validfrom, time_t validto, int bullnr, const std::string& seq, const std::string& msg)
	: m_country(ctry), m_dissemiator(dissem), m_originator(orig), m_msg(msg), m_sequence(seq),
	  m_transmissiontime(txtime), m_validfrom(validfrom), m_validto(validto), m_bulletinnr(bullnr)
{
	for (m_type = type_sigmet; m_type != type_invalid; m_type = (type_t)(m_type + 1))
		if (typ == get_type_str(m_type))
			break;
}

const std::string& MetarTafSet::SIGMET::get_type_str(type_t t)
{
	switch (t) {
	case type_sigmet:
	{
		static const std::string r("WS");
		return r;
	}

	case type_tropicalcyclone:
	{
		static const std::string r("WC");
		return r;
	}

	case type_volcanicash:
	{
		static const std::string r("WV");
		return r;
	}

	default:
	{
		static const std::string r("?""?");
		return r;
	}
	}
}

int MetarTafSet::SIGMET::compare(const SIGMET& x) const
{
	int c(get_originator().compare(x.get_originator()));
	if (c)
		return c;
	if (get_type() < x.get_type())
		return -1;
	if (x.get_type() < get_type())
		return 1;
	c = get_sequence().compare(x.get_sequence());
	if (c)
		return c;
	if (get_validfrom() < x.get_validfrom())
		return -1;
	if (x.get_validfrom() < get_validfrom())
		return 1;
	if (get_validto() < x.get_validto())
		return -1;
	if (x.get_validto() < get_validto())
		return 1;
	if (get_transmissiontime() < x.get_transmissiontime())
		return -1;
	if (x.get_transmissiontime() < get_transmissiontime())
		return 1;
	return 0;
}

std::vector<std::string> MetarTafSet::SIGMET::tokenize(const std::string& s)
{
        std::vector<std::string> r;
        for (std::string::const_iterator si(s.begin()), se(s.end()); si != se; ) {
                if (std::isspace(*si)) {
                        ++si;
                        continue;
                }
                std::string::const_iterator si2(si);
                ++si2;
		if (std::isalnum(*si))
			while (si2 != se && std::isalnum(*si2))
				++si2;
		r.push_back(std::string(si, si2));
                si = si2;
        }
        return r;
}

bool MetarTafSet::SIGMET::is_latitude(const std::string& s)
{
	if (s.size() != 3 && s.size() != 5)
		return false;
	if (s[0] != 'N' && s[0] != 'S')
		return false;
	for (std::string::size_type i(1), n(s.size()); i < n; ++i)
		if (!std::isdigit(s[i]))
			return false;
	return true;
}

bool MetarTafSet::SIGMET::is_longitude(const std::string& s)
{
	if (s.size() != 4 && s.size() != 6)
		return false;
	if (s[0] != 'E' && s[0] != 'W')
		return false;
	for (std::string::size_type i(1), n(s.size()); i < n; ++i)
		if (!std::isdigit(s[i]))
			return false;
	return true;
}

void MetarTafSet::SIGMET::compute_poly(const MultiPolygonHole& firpoly)
{
	m_poly = firpoly;
	if (false)
		return;
	std::vector<std::string> tok(tokenize(get_msg()));
	if (false) {
		std::cerr << "sigmet: tok:";
		for (std::vector<std::string>::const_iterator ti(tok.begin()), te(tok.end()); ti != te; ++ti)
			std::cerr << " \"" << *ti << "\"";
		std::cerr << std::endl;
	}
	Rect bbox(firpoly.get_bbox());
	for (std::vector<std::string>::size_type i(0), n(tok.size()); i < n; ++i) {
		if (i + 2 < n && tok[i] == "N" && tok[i+1] == "OF" && is_latitude(tok[i+2])) {
			Point pt;
			pt.set_str(tok[i+2], 0);
			PolygonSimple p;
			p.push_back(Point(bbox.get_west(), pt.get_lat()));
			p.push_back(Point(bbox.get_east(), pt.get_lat()));
			p.push_back(bbox.get_northeast());
			p.push_back(bbox.get_northwest());
			m_poly.geos_intersect(PolygonHole(p));
			if (false) {
				p.print(std::cerr << "Intersect Poly: ") << std::endl;
				m_poly.print(std::cerr << "Result Poly: ") << std::endl;
			}
			i += 2;
			continue;
		}
		if (i + 2 < n && tok[i] == "S" && tok[i+1] == "OF" && is_latitude(tok[i+2])) {
			Point pt;
			pt.set_str(tok[i+2], 0);
			PolygonSimple p;
			p.push_back(Point(bbox.get_west(), pt.get_lat()));
			p.push_back(Point(bbox.get_east(), pt.get_lat()));
			p.push_back(bbox.get_southeast());
			p.push_back(bbox.get_southwest());
			m_poly.geos_intersect(PolygonHole(p));
			if (false) {
				p.print(std::cerr << "Intersect Poly: ") << std::endl;
				m_poly.print(std::cerr << "Result Poly: ") << std::endl;
			}
			i += 2;
			continue;
		}
		if (i + 2 < n && tok[i] == "E" && tok[i+1] == "OF" && is_longitude(tok[i+2])) {
			Point pt;
			pt.set_str(tok[i+2], 0);
			PolygonSimple p;
			p.push_back(Point(pt.get_lat(), bbox.get_north()));
			p.push_back(Point(pt.get_lat(), bbox.get_south()));
			p.push_back(bbox.get_southeast());
			p.push_back(bbox.get_northeast());
			m_poly.geos_intersect(PolygonHole(p));
			if (false) {
				p.print(std::cerr << "Intersect Poly: ") << std::endl;
				m_poly.print(std::cerr << "Result Poly: ") << std::endl;
			}
			i += 2;
			continue;
		}
		if (i + 2 < n && tok[i] == "W" && tok[i+1] == "OF" && is_longitude(tok[i+2])) {
			Point pt;
			pt.set_str(tok[i+2], 0);
			PolygonSimple p;
			p.push_back(Point(pt.get_lat(), bbox.get_north()));
			p.push_back(Point(pt.get_lat(), bbox.get_south()));
			p.push_back(bbox.get_southwest());
			p.push_back(bbox.get_northwest());
			m_poly.geos_intersect(PolygonHole(p));
			if (false) {
				p.print(std::cerr << "Intersect Poly: ") << std::endl;
				m_poly.print(std::cerr << "Result Poly: ") << std::endl;
			}
			i += 2;
			continue;
		}
		if (i + 7 < n && (tok[i] == "N" || tok[i] == "NE" || tok[i] == "E" || tok[i] == "SE" ||
				  tok[i] == "S" || tok[i] == "SW" || tok[i] == "W" || tok[i] == "NW") &&
		    tok[i+1] == "OF" && tok[i+2] == "LINE" && is_latitude(tok[i+3]) && is_longitude(tok[i+4]) &&
		    tok[i+5] == "-" && is_latitude(tok[i+6]) && is_longitude(tok[i+7])) {
			PolygonSimple p;
			p.push_back(Point::invalid);
			p.back().set_str(tok[i+3]);
			p.back().set_str(tok[i+4]);
			p.push_back(Point::invalid);
			p.back().set_str(tok[i+6]);
			p.back().set_str(tok[i+7]);
			if (tok[i] == "N") {
				p.push_back(bbox.get_northwest());
				p.push_back(bbox.get_northeast());
			} else if (tok[i] == "NE") {
				p.push_back(bbox.get_northwest());
				p.push_back(bbox.get_northeast());
				p.push_back(bbox.get_southeast());
			} else if (tok[i] == "E") {
				p.push_back(bbox.get_northeast());
				p.push_back(bbox.get_southeast());
			} else if (tok[i] == "SE") {
				p.push_back(bbox.get_northeast());
				p.push_back(bbox.get_southeast());
				p.push_back(bbox.get_southwest());
			} else if (tok[i] == "S") {
				p.push_back(bbox.get_southeast());
				p.push_back(bbox.get_southwest());
			} else if (tok[i] == "SW") {
				p.push_back(bbox.get_southeast());
				p.push_back(bbox.get_southwest());
				p.push_back(bbox.get_northwest());
			} else if (tok[i] == "W") {
				p.push_back(bbox.get_northwest());
				p.push_back(bbox.get_southwest());
			} else if (tok[i] == "NW") {
				p.push_back(bbox.get_southwest());
				p.push_back(bbox.get_northwest());
				p.push_back(bbox.get_northeast());
			}
			if (p.is_self_intersecting())
				std::swap(p[0], p[1]);
			m_poly.geos_intersect(PolygonHole(p));
			i += 7;
			continue;
		}
		if (i + 5 < n && tok[i] == "WI" && is_latitude(tok[i+1]) && is_longitude(tok[i+2]) &&
		    tok[i+3] == "-" && is_latitude(tok[i+4]) && is_longitude(tok[i+5])) {
			PolygonSimple p;
			p.push_back(Point::invalid);
			p.back().set_str(tok[i+1]);
			p.back().set_str(tok[i+2]);
			p.push_back(Point::invalid);
			p.back().set_str(tok[i+4]);
			p.back().set_str(tok[i+5]);
			i += 5;
			for (; i + 3 < n && tok[i+1] == "-" && is_latitude(tok[i+2]) && is_longitude(tok[i+3]); i += 3) {
				p.push_back(Point::invalid);
				p.back().set_str(tok[i+2]);
				p.back().set_str(tok[i+3]);
			}
			if (p.size() < 3)
				continue;
			m_poly.geos_intersect(PolygonHole(p));
			continue;
		}
	}
	// for now; in the future reduce polygon if location is known in the sigmet text
}

MetarTafSet::Station::Station(const std::string& stationid, const Point& coord, double elevm)
	: m_stationid(stationid), m_coord(coord), m_elev(elevm), m_wmonr(~0U)
{
}

MetarTafSet::FIR::FIR(const std::string& id, const MultiPolygonHole& poly)
	: m_ident(id), m_poly(poly)
{
}

void MetarTafSet::FIR::compute_poly(void)
{
	for (sigmet_t::iterator si(m_sigmet.begin()), se(m_sigmet.end()); si != se; ++si)
		const_cast<SIGMET&>(*si).compute_poly(m_poly);
}

bool MetarTafSet::OrderStadionId::operator()(const Station& a, const Station& b) const
{
	return a.get_stationid() < b.get_stationid();
}

#ifdef HAVE_PQXX
class MetarTafSet::PGTransactor : public pqxx::transactor<pqxx::read_transaction> {
public:
	typedef pqxx::read_transaction transaction_t;

	PGTransactor(MetarTafSet& mset, const Rect& bbox, unsigned int metarhistory, unsigned int tafhistory, const std::string& tname = "Metar Taf Set");
	void operator()(transaction_t& tran);
	void on_abort(const char msg[]) throw();
	void on_doubt(void) throw();
	void on_commit(void);

protected:
	typedef pqxx::transactor<transaction_t> base_t;
	MetarTafSet& m_mset;
	Rect m_bbox;
	unsigned int m_metarhistory;
	unsigned int m_tafhistory;
	typedef std::set<Station> result_t;
	result_t m_result;

	std::string get_querycoord(transaction_t& tran) const;
	void get_metar(transaction_t& tran);
	void get_taf(transaction_t& tran);
};

MetarTafSet::PGTransactor::PGTransactor(MetarTafSet& mset, const Rect& bbox, unsigned int metarhistory, unsigned int tafhistory, const std::string& tname)
	: base_t(tname), m_mset(mset), m_bbox(bbox), m_metarhistory(metarhistory), m_tafhistory(tafhistory)
{
}

void MetarTafSet::PGTransactor::operator()(transaction_t& tran)
{
	get_metar(tran);
	get_taf(tran);
}

void MetarTafSet::PGTransactor::get_metar(transaction_t& tran)
{
	pqxx::result r(tran.exec("select * from (select station_id,latitude,longitude,elevation_m,observation_time,raw_text,flight_category "
				 "from metar where " + get_querycoord(tran) + ") T1 where T1.observation_time in "
				 "(select T2.observation_time from metar T2 where T2.station_id = T1.station_id "
				 "order by T2.observation_time desc limit " +
				 tran.quote(m_metarhistory) + ")", "Metar Taf Set METAR"));
	for (pqxx::result::const_iterator ri(r.begin()), re(r.end()); ri != re; ++ri) {
		std::pair<result_t::iterator,bool> ins(m_result.insert(Station(ri[0].as<std::string>(),
									       Point(ri[2].as<Point::coord_t>(), ri[1].as<Point::coord_t>()),
									       ri[3].as<double>(std::numeric_limits<double>::quiet_NaN()))));
		Station& stn(const_cast<Station&>(*ins.first));
		if (false)
			std::cerr << "pg: station " << ri[0].as<std::string>() << " fltcat \"" << ri[6].as<std::string>() << "\"" << std::endl;
		stn.get_metar().insert(METAR(ri[4].as<time_t>(), ri[5].as<std::string>(), ri[6].as<std::string>()));
	}
}

void MetarTafSet::PGTransactor::get_taf(transaction_t& tran)
{
	pqxx::result r(tran.exec("select * from (select station_id,latitude,longitude,elevation_m,issue_time,raw_text "
				 "from taf where " + get_querycoord(tran) + ") T1 where T1.issue_time in "
				 "(select T2.issue_time from taf T2 where T2.station_id = T1.station_id "
				 "order by T2.issue_time desc limit " +
				 tran.quote(m_tafhistory) + ")", "Metar Taf Set TAF"));
	for (pqxx::result::const_iterator ri(r.begin()), re(r.end()); ri != re; ++ri) {
		std::pair<result_t::iterator,bool> ins(m_result.insert(Station(ri[0].as<std::string>(),
									       Point(ri[2].as<Point::coord_t>(), ri[1].as<Point::coord_t>()),
									       ri[3].as<double>(std::numeric_limits<double>::quiet_NaN()))));
		Station& stn(const_cast<Station&>(*ins.first));
		stn.get_taf().insert(TAF(ri[4].as<time_t>(), ri[5].as<std::string>()));
	}
}

void MetarTafSet::PGTransactor::on_abort(const char msg[]) throw()
{
	std::cerr << "pg: " << Name() << ": transaction aborted: " << msg << std::endl;
}

void MetarTafSet::PGTransactor::on_doubt(void) throw()
{
	std::cerr << "pg: " << Name() << ": transaction in doubt" << std::endl;
}

void MetarTafSet::PGTransactor::on_commit(void)
{
	for (result_t::const_iterator ri(m_result.begin()), re(m_result.end()); ri != re; ++ri) {
		stations_t::iterator si(lower_bound(m_mset.m_stations.begin(), m_mset.m_stations.end(), *ri, OrderStadionId()));
		if (si == m_mset.m_stations.end() || si->get_stationid() != ri->get_stationid())
			si = m_mset.m_stations.insert(si, *ri);
		si->get_metar().insert(ri->get_metar().begin(), ri->get_metar().end());
		si->get_taf().insert(ri->get_taf().begin(), ri->get_taf().end());
	}
}

std::string MetarTafSet::PGTransactor::get_querycoord(transaction_t& tran) const
{
	std::string coordselect("(latitude between " + tran.quote(m_bbox.get_south()) + " and " +
				tran.quote(m_bbox.get_north()) + ") and ((longitude between ");
	int64_t x(m_bbox.get_west()), y(m_bbox.get_east_unwrapped());
	coordselect += tran.quote(x) + " and " +
		tran.quote(static_cast<Point::coord_t>(std::min(y, static_cast<int64_t>(Point::lon_max)))) + ")";
	x -= (1LL << 32);
	y -= (1LL << 32);
	if (y >= Point::lon_min)
		coordselect += " or (longitude between " +
			tran.quote(static_cast<Point::coord_t>(std::max(x, static_cast<int64_t>(Point::lon_min)))) +
			" and " + tran.quote(y) + ")";
	coordselect += ")";
	return coordselect;
}

class MetarTafSet::PGMetarTransactor : public PGTransactor {
public:
	PGMetarTransactor(MetarTafSet& mset, const Rect& bbox, unsigned int history);
	void operator()(transaction_t& tran);
};

MetarTafSet::PGMetarTransactor::PGMetarTransactor(MetarTafSet& mset, const Rect& bbox, unsigned int history)
	: PGTransactor(mset, bbox, history, 0, "Metar Taf Set METAR")
{
}

void MetarTafSet::PGMetarTransactor::operator()(transaction_t& tran)
{
	get_metar(tran);
}

class MetarTafSet::PGTafTransactor : public PGTransactor {
public:
	PGTafTransactor(MetarTafSet& mset, const Rect& bbox, unsigned int history);
	void operator()(transaction_t& tran);

};

MetarTafSet::PGTafTransactor::PGTafTransactor(MetarTafSet& mset, const Rect& bbox, unsigned int history)
	: PGTransactor(mset, bbox, 0, history, "Metar Taf Set TAF")
{
}

void MetarTafSet::PGTafTransactor::operator()(transaction_t& tran)
{
	get_taf(tran);
}

class MetarTafSet::PGSigmetTransactor : public pqxx::transactor<pqxx::read_transaction> {
public:
	typedef pqxx::read_transaction transaction_t;

	PGSigmetTransactor(MetarTafSet& mset, const firs_t& firs, time_t tmin, time_t tmax, const std::string& tname = "Metar Taf Set");
	void operator()(transaction_t& tran);
	void on_abort(const char msg[]) throw();
	void on_doubt(void) throw();
	void on_commit(void);

protected:
	typedef pqxx::transactor<transaction_t> base_t;
	MetarTafSet& m_mset;
	firs_t m_result;
	time_t m_tmin;
	time_t m_tmax;
};

MetarTafSet::PGSigmetTransactor::PGSigmetTransactor(MetarTafSet& mset, const firs_t& firs, time_t tmin, time_t tmax, const std::string& tname)
	: base_t(tname), m_mset(mset), m_tmin(tmin), m_tmax(tmax)
{
	for (firs_t::const_iterator fi(firs.begin()), fe(firs.end()); fi != fe; ++fi)
		m_result.insert(FIR(fi->get_ident()));
}

void MetarTafSet::PGSigmetTransactor::operator()(transaction_t& tran)
{
	std::string q("select fir,type,country,disseminator,originator,transmission_time,valid_from,valid_to,bulletin_number,sequence,message from sigmet where ");
	{
		bool subseq(false);
		for (firs_t::const_iterator fi(m_result.begin()), fe(m_result.end()); fi != fe; ++fi) {
			if (subseq)
				q += " or ";
			else
				q += "(";
			subseq = true;
			q += "fir=" + tran.quote(fi->get_ident());
		}
		if (subseq)
			q += ") and ";
	}
	q += "valid_from <= " + tran.quote(m_tmax) + " and valid_to >= " + tran.quote(m_tmin);
	if (!false)
		std::cerr << "pg: sigmet query " << q << std::endl;
	pqxx::result r(tran.exec(q, "Briefing Pack SIGMET"));
	for (pqxx::result::const_iterator ri(r.begin()), re(r.end()); ri != re; ++ri) {
		if (!false)
			std::cerr << "pg: sigmet for " << ri[0].as<std::string>() << " seq " << ri[8].as<std::string>()
				  << " msg " << ri[9].as<std::string>() << std::endl;
		firs_t::iterator fi(m_result.find(FIR(ri[0].as<std::string>())));
		if (fi == m_result.end())
			continue;
		FIR& fir(const_cast<FIR&>(*fi));
		fir.get_sigmet().insert(SIGMET(ri[1].as<std::string>(), ri[2].as<std::string>(), ri[3].as<std::string>(), ri[4].as<std::string>(),
					       ri[5].as<time_t>(), ri[6].as<time_t>(), ri[7].as<time_t>(), ri[8].as<int>(),
					       ri[9].as<std::string>(), ri[10].as<std::string>()));
	}
}

void MetarTafSet::PGSigmetTransactor::on_abort(const char msg[]) throw()
{
	std::cerr << "pg: " << Name() << ": transaction aborted: " << msg << std::endl;
}

void MetarTafSet::PGSigmetTransactor::on_doubt(void) throw()
{
	std::cerr << "pg: " << Name() << ": transaction in doubt" << std::endl;
}

void MetarTafSet::PGSigmetTransactor::on_commit(void)
{
	for (firs_t::const_iterator fi(m_result.begin()), fe(m_result.end()); fi != fe; ++fi) {
		if (fi->get_sigmet().empty())
			continue;
		firs_t::iterator fi2(m_mset.m_firs.find(FIR(fi->get_ident())));
		if (fi2 == m_mset.m_firs.end())
			continue;
		const_cast<FIR &>(*fi2).get_sigmet().insert(fi->get_sigmet().begin(), fi->get_sigmet().end());
	}
}
#endif

MetarTafSet::MetarTafSet(void)
{
}

void MetarTafSet::add_fir(const std::string& id, const MultiPolygonHole& poly)
{
	if (id.empty())
		return;
	m_firs.insert(FIR(id, poly));
}

void MetarTafSet::loadstn_sqlite(const std::string& dbpath, const Rect& bbox,
				 unsigned int metarhistory, unsigned int tafhistory)
{
	sqlite3x::sqlite3_connection db;
	if (true) {
		sqlite3 *h(0);
		if (SQLITE_OK == sqlite3_open_v2(dbpath.c_str(), &h, SQLITE_OPEN_READONLY, 0))
			db.take(h);
		else
			throw sqlite3x::database_error("unable to open database");
	} else {
		db.open(dbpath);
	}
	{
		sqlite3x::sqlite3_command cmd(db, "select * from (select station_id,latitude,longitude,elevation_m,observation_time,raw_text,flight_category "
					      "from metar where (latitude between ?2 and ?4) and "
					      "((longitude between ?1-4294967296 and ?3-4294967296) or "
					      "(longitude between ?1 and ?3))) T1 where T1.observation_time in "
					      "(select T2.observation_time from metar T2 where T2.station_id = T1.station_id "
					      "order by T2.observation_time desc limit ?5);");
		cmd.bind(1, (long long int)bbox.get_west());
		cmd.bind(2, (long long int)bbox.get_south());
		cmd.bind(3, (long long int)bbox.get_east_unwrapped());
		cmd.bind(4, (long long int)bbox.get_north());
		cmd.bind(5, (int)metarhistory);
		sqlite3x::sqlite3_cursor cursor(cmd.executecursor());
		while (cursor.step()) {
			Station stn(cursor.getstring(0), Point(cursor.getint(2), cursor.getint(1)), cursor.getdouble(3));
			stations_t::iterator si(lower_bound(m_stations.begin(), m_stations.end(), stn, OrderStadionId()));
			if (si == m_stations.end() || si->get_stationid() != stn.get_stationid())
				si = m_stations.insert(si, stn);
			si->get_metar().insert(METAR(cursor.getint(4), cursor.getstring(5), cursor.getstring(6)));
		}
	}
	{
		sqlite3x::sqlite3_command cmd(db, "select * from (select station_id,latitude,longitude,elevation_m,issue_time,raw_text "
					      "from taf where (latitude between ?2 and ?4) and "
					      "((longitude between ?1-4294967296 and ?3-4294967296) or "
					      "(longitude between ?1 and ?3))) T1 where T1.issue_time in "
					      "(select T2.issue_time from taf T2 where T2.station_id = T1.station_id "
					      "order by T2.issue_time desc limit ?5);");
		cmd.bind(1, (long long int)bbox.get_west());
		cmd.bind(2, (long long int)bbox.get_south());
		cmd.bind(3, (long long int)bbox.get_east_unwrapped());
		cmd.bind(4, (long long int)bbox.get_north());
		cmd.bind(5, (int)tafhistory);
		sqlite3x::sqlite3_cursor cursor(cmd.executecursor());
		while (cursor.step()) {
			Station stn(cursor.getstring(0), Point(cursor.getint(2), cursor.getint(1)), cursor.getdouble(3));
			stations_t::iterator si(lower_bound(m_stations.begin(), m_stations.end(), stn, OrderStadionId()));
			if (si == m_stations.end() || si->get_stationid() != stn.get_stationid())
				si = m_stations.insert(si, stn);
			si->get_taf().insert(TAF(cursor.getint(4), cursor.getstring(5)));
		}
	}
}

#ifdef HAVE_PQXX

void MetarTafSet::loadstn_pg(pqxx::connection_base& conn, const Rect& bbox, time_t tmin, time_t tmax,
			     unsigned int metarhistory, unsigned int tafhistory)
{
	try {
		{
			PGTransactor metartaf(*this, bbox, metarhistory, tafhistory);
			conn.perform(metartaf);
		}
		if (tmin <= tmax) {
			PGSigmetTransactor sigmet(*this, m_firs, tmin, tmax);
			conn.perform(sigmet);
		}
		compute_poly();
	} catch (const pqxx::pqxx_exception& e) {
		std::cerr << "pqxx exception: " << e.base().what() << std::endl;
	}
}

#endif

void MetarTafSet::compute_poly(void)
{
	for (firs_t::iterator fi(m_firs.begin()), fe(m_firs.end()); fi != fe; ++fi)
		const_cast<FIR&>(*fi).compute_poly();
}
