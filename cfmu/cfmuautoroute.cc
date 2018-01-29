#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <fstream>
#include <sstream>
#include <iostream>
#include <iomanip>

#include "cfmuautoroute.hh"
#include "icaofpl.h"
#include "airdata.h"

constexpr double CFMUAutoroute::Crossing::maxradius;

CFMUAutoroute::Performance::Cruise::Cruise(int alt, double da, double ta, double rpm, double mp,
					   double secpernmi, double fuelpersec, double metricpernmi,
					   Glib::RefPtr<GRIB2::LayerResult> windu, Glib::RefPtr<GRIB2::LayerResult> windv,
					   Glib::RefPtr<GRIB2::LayerResult> temp)
	: m_windu(windu), m_windv(windv), m_temp(temp), m_secpernmi(secpernmi), m_fuelpersec(fuelpersec),
	  m_metricpernmi(metricpernmi), m_rpm(rpm), m_mp(mp), m_densityaltitude(da), m_truealt(ta), m_altitude(alt)
{
}

std::string CFMUAutoroute::Performance::Cruise::get_fpltas(double tas)
{
	std::ostringstream oss;
	oss << 'N';
	if (tas < 0 || std::isnan(tas))
		oss << "----";
	else
		oss << std::setw(4) << std::setfill('0') << Point::round<int,double>(tas);
	return oss.str();
}

std::string CFMUAutoroute::Performance::Cruise::get_fplalt(int alt)
{
	std::ostringstream oss;
	oss << 'F';
	if (alt < 0)
		oss << "---";
	else
		oss << std::setw(3) << std::setfill('0') << ((alt + 50) / 100);
	return oss.str();
}

Wind<double> CFMUAutoroute::Performance::Cruise::get_wind(const Point& pt) const
{
	Wind<double> wnd;
	if (!m_windu || !m_windv) {
		wnd.set_wind(0, 0);
		return wnd;
	}
	float wu(m_windu->operator()(pt));
	float wv(m_windv->operator()(pt));
	{
		std::pair<float,float> w(m_windu->get_layer()->get_grid()->transform_axes(wu, wv));
		wu = w.first;
		wv = w.second;
	}
	// convert from m/s to kts
	wu *= (-1e-3f * Point::km_to_nmi * 3600);
	wv *= (-1e-3f * Point::km_to_nmi * 3600);
	wnd.set_wind((M_PI - atan2f(wu, wv)) * Wind<double>::from_rad, sqrtf(wu * wu + wv * wv));
	return wnd;
}

double CFMUAutoroute::Performance::Cruise::get_temp(const Point& pt) const
{
	if (!m_temp)
		return std::numeric_limits<double>::quiet_NaN();
	return m_temp->operator()(pt);
}

CFMUAutoroute::Performance::LevelChange::LevelChange(double tracknmi, double timepenalty, double fuelpenalty, double metricpenalty, double optracknmi)
	: m_tracknmi(tracknmi), m_timepenalty(timepenalty), m_fuelpenalty(fuelpenalty), m_metricpenalty(metricpenalty), m_opsperftracknmi(optracknmi)
{
}

const CFMUAutoroute::Performance::Cruise CFMUAutoroute::Performance::nullcruise(-1, std::numeric_limits<double>::quiet_NaN());

CFMUAutoroute::Performance::Performance(void)
{
}

const CFMUAutoroute::Performance::Cruise& CFMUAutoroute::Performance::get_cruise(unsigned int pi) const
{
	if (pi >= size())
		return nullcruise;
	return m_cruise[pi];
}

const CFMUAutoroute::Performance::Cruise& CFMUAutoroute::Performance::get_cruise(unsigned int piu, unsigned int piv) const
{
	const unsigned int pis(size());
	if (piu >= pis) {
		piu = piv;
		if (piu >= pis)
			return nullcruise;
	}
	return m_cruise[piu];
}

const CFMUAutoroute::Performance::LevelChange& CFMUAutoroute::Performance::get_levelchange(unsigned int piu, unsigned int piv) const
{
	const unsigned int pis(size());
	return m_levelchange[std::min(piu, pis)][std::min(piv, pis)];
}
		
void CFMUAutoroute::Performance::clear(void)
{
	m_cruise.clear();
	m_levelchange.clear();
}

void CFMUAutoroute::Performance::add(const Cruise& x)
{
	m_cruise.push_back(x);
}

void CFMUAutoroute::Performance::set(unsigned int piu, unsigned int piv, const LevelChange& lvlchg)
{
	const unsigned int pis(size());
	if (!pis)
		return;
	if (!m_levelchange.size()) {
		m_levelchange.resize(pis + 1);
		for (unsigned int pi = 0; pi <= pis; ++pi)
			m_levelchange[pi].resize(pis + 1);
	}
	m_levelchange[std::min(piu, pis)][std::min(piv, pis)] = lvlchg;
}

void CFMUAutoroute::Performance::resize(unsigned int pis)
{
	unsigned int pisold(size());
	if (pis == pisold)
		return;
	m_cruise.resize(pis);
	if (pis < pisold) {
		m_levelchange.erase(m_levelchange.begin() + pis, m_levelchange.begin() + pisold);
		for (levelchange_t::iterator i(m_levelchange.begin()), e(m_levelchange.end()); i != e; ++i)
			i->erase(i->begin() + pis, i->begin() + pisold);
		return;
	}
	unsigned int n(pis - pisold);
	for (levelchange_t::iterator i(m_levelchange.begin()), e(m_levelchange.end()); i != e; ++i)
		i->insert(i->begin() + pisold, n, LevelChange());
	m_levelchange.insert(m_levelchange.begin() + pisold, n, std::vector<LevelChange>(pis + 1, LevelChange()));
}

unsigned int CFMUAutoroute::Performance::findcruiseindex(int alt) const
{
	int adiff(std::numeric_limits<int>::max());
	unsigned int idx(0);
	const unsigned int pis(size());
	for (unsigned int i = 0; i < pis; ++i) {
		int ad(abs(alt - m_cruise[i].get_altitude()));
		if (ad > adiff)
			continue;
		adiff = ad;
		idx = i;
	}
	return idx;
}

int CFMUAutoroute::Performance::get_minaltitude(void) const
{
	if (empty())
		return 0;
	return m_cruise.front().get_altitude();
}

int CFMUAutoroute::Performance::get_maxaltitude(void) const
{
	if (empty())
		return 0;
	return m_cruise.back().get_altitude();
}

const CFMUAutoroute::Performance::Cruise& CFMUAutoroute::Performance::get_lowest_cruise(void) const
{
	if (empty())
		return nullcruise;
	return m_cruise.front();
}

const CFMUAutoroute::Performance::Cruise& CFMUAutoroute::Performance::get_highest_cruise(void) const
{
	if (empty())
		return nullcruise;
	return m_cruise.back();
}

const bool CFMUAutoroute::lgraphrestartedgescan;

CFMUAutoroute::CFMUAutoroute()
	: m_logprefix("/tmp/cfmuautoroute-XXXXXX"), m_route(*(FPlan *)0),
	  m_routetime(0), m_routezerowindtime(0), m_routefuel(0), m_routezerowindfuel(0),
	  m_maxdescent(1000), m_dctlimit(50.0), m_dctpenalty(1.0), m_dctoffset(0.0), m_vfraspclimit(100.0),
	  m_qnh(IcaoAtmosphere<double>::std_sealevel_pressure), m_isaoffs(0),
	  m_enginerpm(2400), m_enginemp(std::numeric_limits<double>::quiet_NaN()), m_enginebhp(130), m_starttime(0),
	  m_preferredlevel(100), m_preferredpenalty(1.1), m_preferredclimb(3), m_preferreddescent(1),
	  m_childtimeoutcount(0), m_childxdisplay(-1), m_pathprobe(-1), m_opttarget(opttarget_time),
	  m_validator(validator_default), m_tvelapsed(0, 0), m_tvvalidator(0, 0), m_tvvalidatestart(0, 0),
	  m_maxlocaliteration(~0U), m_maxremoteiteration(~0U), m_childrun(false), m_forceenrifr(true),
	  m_tfravailable(true), m_tfrenable(true), m_tfrloaded(false), m_honourawylevels(true), m_honourprofilerules(false),
	  m_honourlevelchangetrackmiles(true), m_honouropsperftrackmiles(true), m_precompgraphenable(true),
	  m_windenable(false), m_grib2loaded(false), m_opsperfloaded(false), m_running(false), m_done(false)
{
#ifdef __MINGW32__
	std::string workdir;
	{
		gchar *instdir(g_win32_get_package_installation_directory_of_module(0));
		if (instdir)
			m_childbinary = Glib::build_filename(instdir, "bin", "cfmuvalidateserver.exe");
		else
			m_childbinary = Glib::build_filename(PACKAGE_PREFIX_DIR "/bin", "cfmuvalidateserver.exe");
	}
#else
	m_childbinary = Glib::build_filename(PACKAGE_LIBEXEC_DIR, "cfmuvalidateserver");
#endif
#ifdef G_OS_UNIX
	m_childsockaddr = Gio::UnixSocketAddress::create(PACKAGE_RUN_DIR "/validator/socket", Gio::UNIX_SOCKET_ADDRESS_PATH);
#else
	m_childsockaddr = Gio::InetSocketAddress::create(Gio::InetAddress::create_loopback(Gio::SOCKET_FAMILY_IPV4), 53181);
#endif
	m_iterationcount[0] = m_iterationcount[1] = 0;
	for (unsigned int i = 0; i < 2; ++i) {
		m_airport[i] = AirportsDb::Airport();
		m_airportconn[i].set_invalid();
		m_airportconnlimit[i] = 40.0;
		m_airportconnpenalty[i] = 1.0;
		m_airportconnoffset[i] = 0.0;
		m_airportconnminimum[i] = 0.0;
		m_airportconnident[i].clear();
		m_airportconntype[i] = Engine::AirwayGraphResult::Vertex::type_undefined;
		m_airportconndb[i] = true;
		m_airportifr[i] = true;
		m_airportproconly[i] = false;
	}
	m_levels[0] = 50;
	m_levels[1] = 120;
	time(&m_starttime);
	clear();
	// 10 o'clock
	set_deptime(3600*10);
	check_tfr_available();
}

CFMUAutoroute::~CFMUAutoroute()
{
	child_close();
	clear();
}

template bool CFMUAutoroute::is_nan_or_inf(float x);
template bool CFMUAutoroute::is_nan_or_inf(double x);

void CFMUAutoroute::set_opttarget(opttarget_t t)
{
	if (m_opttarget != t)
		clear();
	m_opttarget = t;
}

const std::string& to_str(CFMUAutoroute::opttarget_t t)
{
	switch (t) {
	case CFMUAutoroute::opttarget_time:
	{
		static const std::string r("time");
		return r;
	}

	case CFMUAutoroute::opttarget_fuel:
	{
		static const std::string r("fuel");
		return r;
	}

	case CFMUAutoroute::opttarget_preferred:
	{
		static const std::string r("preferred");
		return r;
	}

	default:
	{
		static const std::string r("?""?");
		return r;
	}
	}
}

const std::string& to_str(CFMUAutoroute::log_t l)
{
	switch (l) {
	case CFMUAutoroute::log_fplproposal:
	{
		static const std::string r("fplproposal");
		return r;
	}

	case CFMUAutoroute::log_fpllocalvalidation:
	{
		static const std::string r("fpllocalvalidation");
		return r;
	}

	case CFMUAutoroute::log_fplremotevalidation:
	{
		static const std::string r("fplremotevalidation");
		return r;
	}

	case CFMUAutoroute::log_graphrule:
	{
		static const std::string r("graphrule");
		return r;
	}

	case CFMUAutoroute::log_graphruledesc:
	{
		static const std::string r("graphruledesc");
		return r;
	}

	case CFMUAutoroute::log_graphruleoprgoal:
	{
		static const std::string r("graphruleoprgoal");
		return r;
	}

	case CFMUAutoroute::log_graphchange:
	{
		static const std::string r("graphchange");
		return r;
	}

	case CFMUAutoroute::log_precompgraph:
	{
		static const std::string r("precompgraph");
		return r;
	}

	case CFMUAutoroute::log_weatherdata:
	{
		static const std::string r("weatherdata");
		return r;
	}

	case CFMUAutoroute::log_normal:
	{
		static const std::string r("normal");
		return r;
	}

	case CFMUAutoroute::log_debug0:
	{
		static const std::string r("debug0");
		return r;
	}

	case CFMUAutoroute::log_debug1:
	{
		static const std::string r("debug1");
		return r;
	}

	default:
	{
		static const std::string r("?""?");
		return r;
	}
	}
}

void CFMUAutoroute::set_departure(const AirportsDb::Airport& el)
{
	if (m_airport[0].is_valid() != el.is_valid() ||
	    m_airport[0].get_icao() != el.get_icao() ||
	    m_airport[0].get_name() != el.get_name() ||
	    m_airport[0].get_coord() != el.get_coord())
		clear();
	m_airport[0] = el;
}

bool CFMUAutoroute::set_departure(const std::string& icao, const std::string& name)
{
	AirportsDb::Airport el;
	if (!find_airport(el, icao, name))
		return false;
	set_departure(el);
	return true;
}

void CFMUAutoroute::set_departure_ifr(bool ifr)
{
	ifr = !!ifr;
	if (m_airportifr[0] != ifr)
		clear();
	m_airportifr[0] = ifr;
}

void CFMUAutoroute::set_sid(const Point& pt)
{
	if (m_airportconn[0] != pt)
		clear();
	m_airportconn[0] = pt;
	m_airportconnident[0].clear();
	m_airportconntype[0] = Engine::AirwayGraphResult::Vertex::type_undefined;
}

bool CFMUAutoroute::set_sid(const std::string& name)
{
	Point ptnear(m_airport[0].get_coord());
	if (!m_airport[0].is_valid())
		ptnear.set_invalid();
	Point pt;
	std::string ident;
	Engine::AirwayGraphResult::Vertex::type_t typ;
	bool ifr(m_airportifr[0] || m_airportifr[1] || m_forceenrifr);
	if (!find_point(pt, ident, typ, name, ifr, ptnear, ifr))
		return false;
	set_sid(pt);
	m_airportconnident[0] = ident;
	m_airportconntype[0] = typ;
	return true;
}

void CFMUAutoroute::set_sidlimit(double l)
{
	if (std::isnan(l))
		l = 1.;
	l = std::max(l, 0.);
	if (m_airportconnlimit[0] != l)
		clear();
	m_airportconnlimit[0] = l;
}

void CFMUAutoroute::set_sidpenalty(double p)
{
	if (std::isnan(p))
		p = 1.;
	p = std::max(p, 0.);
	if (m_airportconnpenalty[0] != p)
		clear();
	m_airportconnpenalty[0] = p;
}

void CFMUAutoroute::set_sidoffset(double o)
{
	if (std::isnan(o))
		o = 0.;
	o = std::max(o, 0.);
	if (m_airportconnoffset[0] != o)
		clear();
	m_airportconnoffset[0] = o;
}

void CFMUAutoroute::set_sidminimum(double m)
{
	if (std::isnan(m))
		m = 0.;
	m = std::max(m, 0.);
	if (m_airportconnminimum[0] != m)
		clear();
	m_airportconnminimum[0] = m;
}

void CFMUAutoroute::set_siddb(bool db)
{
	db = !!db;
	if (m_airportconndb[0] != db)
		clear();
	m_airportconndb[0] = db;
}

void CFMUAutoroute::set_sidonly(bool o)
{
	o = !!o;
	if (m_airportproconly[0] != o)
		clear();
	m_airportproconly[0] = o;
}

void CFMUAutoroute::set_sidfilter(const sidstarfilter_t& filt)
{
	if (m_airportconnfilter[0] != filt)
		clear();
	m_airportconnfilter[0] = filt;
}

void CFMUAutoroute::clear_sidfilter(void)
{
	if (m_airportconnfilter[0].empty())
		return;
	clear();
	m_airportconnfilter[0].clear();
}

void CFMUAutoroute::add_sidfilter(const std::string& procname)
{
	if (m_airportconnfilter[0].find(procname) != m_airportconnfilter[0].end())
		return;
	clear();
	m_airportconnfilter[0].insert(procname);
}

void CFMUAutoroute::set_destination(const AirportsDb::Airport& el)
{
	if (m_airport[1].is_valid() != el.is_valid() ||
	    m_airport[1].get_icao() != el.get_icao() ||
	    m_airport[1].get_name() != el.get_name() ||
	    m_airport[1].get_coord() != el.get_coord())
		clear();
	m_airport[1] = el;
}

bool CFMUAutoroute::set_destination(const std::string& icao, const std::string& name)
{
	AirportsDb::Airport el;
	if (!find_airport(el, icao, name))
		return false;
	set_destination(el);
	return true;
}

void CFMUAutoroute::set_destination_ifr(bool ifr)
{
	ifr = !!ifr;
	if (m_airportifr[1] != ifr)
		clear();
	m_airportifr[1] = ifr;
}

void CFMUAutoroute::set_star(const Point& pt)
{
	if (m_airportconn[1] != pt)
		clear();
	m_airportconn[1] = pt;
	m_airportconnident[1].clear();
	m_airportconntype[1] = Engine::AirwayGraphResult::Vertex::type_undefined;
}

bool CFMUAutoroute::set_star(const std::string& name)
{
	Point ptnear(m_airport[1].get_coord());
	if (!m_airport[1].is_valid())
		ptnear.set_invalid();
	Point pt;
	std::string ident;
	Engine::AirwayGraphResult::Vertex::type_t typ;
	bool ifr(m_airportifr[0] || m_airportifr[1] || m_forceenrifr);
	if (!find_point(pt, ident, typ, name, ifr, ptnear, ifr))
		return false;
	set_star(pt);
	m_airportconnident[1] = ident;
	m_airportconntype[1] = typ;
	return true;
}

void CFMUAutoroute::set_starlimit(double l)
{
	if (std::isnan(l))
		l = 0.;
	l = std::max(l, 0.);
	if (m_airportconnlimit[1] != l)
		clear();
	m_airportconnlimit[1] = l;
}

void CFMUAutoroute::set_starpenalty(double p)
{
	if (std::isnan(p))
		p = 1.;
	p = std::max(p, 0.);
	if (m_airportconnpenalty[1] != p)
		clear();
	m_airportconnpenalty[1] = p;
}

void CFMUAutoroute::set_staroffset(double o)
{
	if (std::isnan(o))
		o = 0.;
	o = std::max(o, 0.);
	if (m_airportconnoffset[1] != o)
		clear();
	m_airportconnoffset[1] = o;
}

void CFMUAutoroute::set_starminimum(double m)
{
	if (std::isnan(m))
		m = 0.;
	m = std::max(m, 0.);
	if (m_airportconnminimum[1] != m)
		clear();
	m_airportconnminimum[1] = m;
}

void CFMUAutoroute::set_stardb(bool db)
{
	db = !!db;
	if (m_airportconndb[1] != db)
		clear();
	m_airportconndb[1] = db;
}

void CFMUAutoroute::set_staronly(bool o)
{
	o = !!o;
	if (m_airportproconly[1] != o)
		clear();
	m_airportproconly[1] = o;
}

void CFMUAutoroute::set_starfilter(const sidstarfilter_t& filt)
{
	if (m_airportconnfilter[1] != filt)
		clear();
	m_airportconnfilter[1] = filt;
}

void CFMUAutoroute::clear_starfilter(void)
{
	if (m_airportconnfilter[1].empty())
		return;
	clear();
	m_airportconnfilter[1].clear();
}

void CFMUAutoroute::add_starfilter(const std::string& procname)
{
	if (m_airportconnfilter[1].find(procname) != m_airportconnfilter[1].end())
		return;
	clear();
	m_airportconnfilter[1].insert(procname);
}

const std::string& CFMUAutoroute::get_alternate(unsigned int nr) const
{
	static const std::string empty;
	if (nr >= 2)
		return empty;
	return m_alternate[nr];
}

void CFMUAutoroute::set_alternate(unsigned int nr, const std::string& icao)
{
	if (nr >= 2)
		return;
	m_alternate[nr] = icao;
}

void CFMUAutoroute::set_crossing(unsigned int index, const Point& pt)
{
	if (index <= get_crossing_size())
		set_crossing_size(index + 1);
	if (m_crossing[index].get_coord() != pt)
		clear();
	m_crossing[index].set_coord(pt);
	m_crossing[index].set_ident();
	m_crossing[index].set_type();
}

bool CFMUAutoroute::set_crossing(unsigned int index, const std::string& ident)
{
	if (index <= get_crossing_size())
		set_crossing_size(index + 1);
	Point ptnear;
	ptnear.set_invalid();
	if (m_airport[0].is_valid() && m_airport[1].is_valid())
		ptnear = m_airport[0].get_coord().halfway(m_airport[1].get_coord());
	else if (m_airport[0].is_valid())
		ptnear = m_airport[0].get_coord();
	else if (m_airport[1].is_valid())
		ptnear = m_airport[1].get_coord();
	Point pt;
	std::string ident2;
	Engine::AirwayGraphResult::Vertex::type_t typ;
	if (!find_point(pt, ident2, typ, ident, m_airportifr[1], ptnear, true))
		return false;
	set_crossing(index, pt);
	m_crossing[index].set_ident(ident2);
	m_crossing[index].set_type(typ);
	return true;
}

void CFMUAutoroute::set_crossing_radius(unsigned int index, double r)
{
	if (index <= get_crossing_size())
		set_crossing_size(index + 1);
	if (m_crossing[index].get_radius() != r)
		clear();
	m_crossing[index].set_radius(r);
}

void CFMUAutoroute::set_crossing_level(unsigned int index, int minlevel, int maxlevel)
{
	if (index <= get_crossing_size())
		set_crossing_size(index + 1);
	if (m_crossing[index].get_minlevel() != minlevel || m_crossing[index].get_maxlevel() != maxlevel)
		clear();
	m_crossing[index].set_level(minlevel, maxlevel);
}

const Point& CFMUAutoroute::get_crossing(unsigned int index) const
{
	if (index >= get_crossing_size())
		return Point::invalid;
	return m_crossing[index].get_coord();
}

const std::string& CFMUAutoroute::get_crossing_ident(unsigned int index) const
{
	static const std::string empty;
	if (index >= get_crossing_size())
		return empty;
	return m_crossing[index].get_ident();
}

double CFMUAutoroute::get_crossing_radius(unsigned int index) const
{
	if (index >= get_crossing_size())
		return 0;
	return m_crossing[index].get_radius();
}

int CFMUAutoroute::get_crossing_minlevel(unsigned int index) const
{
	if (index >= get_crossing_size())
		return 0;
	return m_crossing[index].get_minlevel();
}

int CFMUAutoroute::get_crossing_maxlevel(unsigned int index) const
{
	if (index >= get_crossing_size())
		return 600;
	return m_crossing[index].get_maxlevel();
}

Engine::AirwayGraphResult::Vertex::type_t CFMUAutoroute::get_crossing_type(unsigned int index) const
{
	if (index >= get_crossing_size())
		return Engine::AirwayGraphResult::Vertex::type_undefined;
	return m_crossing[index].get_type();
}

void CFMUAutoroute::set_crossing_size(unsigned int sz)
{
	if (sz == get_crossing_size())
		return;
	m_crossing.resize(sz);
	clear();
}

void CFMUAutoroute::set_dctlimit(double l)
{
	if (std::isnan(l))
		l = 0.;
	l = std::max(l, 0.);
	if (m_dctlimit != l)
		clear();
	m_dctlimit = l;
}

void CFMUAutoroute::set_dctpenalty(double l)
{
	if (std::isnan(l))
		l = 1.;
	l = std::max(std::min(l, 1.e6), 1.e-6);
	if (m_dctpenalty != l)
		clear();
	m_dctpenalty = l;
}

void CFMUAutoroute::set_dctoffset(double o)
{
	if (std::isnan(o))
		o = 0.;
	o = std::max(o, 0.);
	if (m_dctoffset != o)
		clear();
	m_dctoffset = o;
}

void CFMUAutoroute::set_vfraspclimit(double l)
{
	if (std::isnan(l))
		l = 0.;
	l = std::max(l, 0.);
	if (m_vfraspclimit != l)
		clear();
	m_vfraspclimit = l;
}

void CFMUAutoroute::set_preferred_level(int level)
{
	if (m_preferredlevel != level)
		clear();
	m_preferredlevel = level;
}

void CFMUAutoroute::set_preferred_penalty(double p)
{
	if (std::isnan(p))
		p = 1.1;
	p = std::max(1.0, std::min(10.0, p));
	if (m_preferredpenalty != p)
		clear();
	m_preferredpenalty = p;
}

void CFMUAutoroute::set_preferred_climb(double c)
{
	if (std::isnan(c))
		c = 10.;
	c = std::max(0.1, std::min(10.0, c));
	double d(m_preferreddescent);
	d = std::min(0.75 * c, d);
	if (m_preferredclimb != c || m_preferreddescent != d)
		clear();
	m_preferredclimb = c;
	m_preferreddescent = d;
}

void CFMUAutoroute::set_preferred_descent(double d)
{
	if (std::isnan(d))
		d = 0.;
	d = std::max(0.0, std::min(0.75 * m_preferredclimb, d));
	if (m_preferreddescent != d)
		clear();
	m_preferreddescent = d;
}

void CFMUAutoroute::set_excluderegions(const excluderegions_t& r)
{
	m_excluderegions = r;
	clear();
}

void CFMUAutoroute::add_excluderegion(const ExcludeRegion& r)
{
	m_excluderegions.push_back(r);
	clear();
}

void CFMUAutoroute::clear_excluderegions(void)
{
	m_excluderegions.clear();
	clear();
}

void CFMUAutoroute::set_force_enroute_ifr(bool ifr)
{
	ifr = !!ifr;
	if (m_forceenrifr != ifr)
		clear();
	m_forceenrifr = ifr;
}

void CFMUAutoroute::check_tfr_available(void)
{
	std::string fnamex(get_tfr_xml_filename());
	std::string fnameb(get_tfr_bin_filename());
	if (false)
		std::cerr << "TFR Filename " << fnamex << " exists "
			  << (Glib::file_test(fnamex, Glib::FILE_TEST_EXISTS) ? "yes" : "no") << " dir "
			  << (Glib::file_test(fnamex, Glib::FILE_TEST_IS_DIR) ? "yes" : "no") << std::endl
			  << "TFR Filename " << fnameb << " exists "
			  << (Glib::file_test(fnameb, Glib::FILE_TEST_EXISTS) ? "yes" : "no") << " dir "
			  << (Glib::file_test(fnameb, Glib::FILE_TEST_IS_DIR) ? "yes" : "no") << std::endl;
	m_tfravailable = (Glib::file_test(fnamex, Glib::FILE_TEST_EXISTS) && !Glib::file_test(fnamex, Glib::FILE_TEST_IS_DIR)) ||
		(Glib::file_test(fnameb, Glib::FILE_TEST_EXISTS) && !Glib::file_test(fnameb, Glib::FILE_TEST_IS_DIR));
}

void CFMUAutoroute::set_tfr_trace(const std::string& rules)
{
	m_tracerules.clear();
	std::string::size_type nd(0);
	for (;;) {
		std::string::size_type nb(nd);
		while (nb < rules.size() && std::isspace(rules[nb]))
			++nb;
		if (nb >= rules.size())
			break;
		std::string::size_type ne(rules.find(',', nb));
		if (ne == std::string::npos)
			nd = ne = rules.size();
		else
			nd = ne + 1;
		while (ne > nb && std::isspace(rules[ne - 1]))
			--ne;
		m_tracerules.insert(rules.substr(nb, ne - nb));
	}
}

std::string CFMUAutoroute::get_tfr_trace(void) const
{
	std::string r;
	bool subseq(false);
	for (ruleset_t::const_iterator i(m_tracerules.begin()), e(m_tracerules.end()); i != e; ++i) {
		if (subseq)
			r += ",";
		subseq = true;
		r += *i;
	}
	return r;
}

void CFMUAutoroute::set_tfr_disable(const std::string& rules)
{
	m_disabledrules.clear();
	std::string::size_type nd(0);
	for (;;) {
		std::string::size_type nb(nd);
		while (nb < rules.size() && std::isspace(rules[nb]))
			++nb;
		if (nb >= rules.size())
			break;
		std::string::size_type ne(rules.find(',', nb));
		if (ne == std::string::npos)
			nd = ne = rules.size();
		else
			nd = ne + 1;
		while (ne > nb && std::isspace(rules[ne - 1]))
			--ne;
		m_disabledrules.insert(rules.substr(nb, ne - nb));
	}
}

std::string CFMUAutoroute::get_tfr_disable(void) const
{
	std::string r;
	bool subseq(false);
	for (ruleset_t::const_iterator i(m_disabledrules.begin()), e(m_disabledrules.end()); i != e; ++i) {
		if (subseq)
			r += ",";
		subseq = true;
		r += *i;
	}
	return r;
}

void CFMUAutoroute::set_precompgraph_enabled(bool ena)
{
	m_precompgraphenable = ena;
}

void CFMUAutoroute::set_wind_enabled(bool ena)
{
	m_windenable = ena;
}

void CFMUAutoroute::set_qnh(double q)
{
	if (std::isnan(q))
		q = IcaoAtmosphere<double>::std_sealevel_pressure;
	q = std::max(std::min(q, 1300.), 700.);
	if (m_qnh != q)
		clear();
	m_qnh = q;
}

void CFMUAutoroute::set_isaoffs(double t)
{
	if (std::isnan(t))
		t = 0.;
	t = std::max(std::min(t, 100.), -100.);
	if (m_isaoffs != t)
		clear();
	m_isaoffs = t;
}

void CFMUAutoroute::set_engine_rpm(double l)
{
	if (m_enginerpm != l)
		clear();
	m_enginerpm = l;
}

void CFMUAutoroute::set_engine_mp(double l)
{
	if (m_enginemp != l)
		clear();
	m_enginemp = l;
}

void CFMUAutoroute::set_engine_bhp(double l)
{
	if (m_enginebhp != l)
		clear();
	m_enginebhp = l;
}

void CFMUAutoroute::set_levels(int base, int top)
{
	if (m_levels[0] != base ||
	    m_levels[1] != top)
		clear();
	m_levels[0] = base;
	m_levels[1] = top;
}

void CFMUAutoroute::set_maxdescent(double d)
{
	if (std::isnan(d))
		d = 1000;
	if (d < 100)
		d = 100;
	if (m_maxdescent != d)
		clear();
	m_maxdescent = d;
}

void CFMUAutoroute::set_honour_levelchangetrackmiles(bool lvltrk)
{
	lvltrk = !!lvltrk;
	if (m_honourlevelchangetrackmiles != lvltrk)
		clear();
	m_honourlevelchangetrackmiles = lvltrk;
}

void CFMUAutoroute::set_honour_opsperftrackmiles(bool lvltrk)
{
	lvltrk = !!lvltrk;
	if (m_honouropsperftrackmiles != lvltrk)
		clear();
	m_honouropsperftrackmiles = lvltrk;
}

void CFMUAutoroute::set_honour_awy_levels(bool awylvl)
{
	awylvl = !!awylvl;
	if (m_honourawylevels != awylvl)
		clear();
	m_honourawylevels = awylvl;
}

void CFMUAutoroute::set_honour_profilerules(bool prules)
{
	prules = !!prules;
	if (m_honourprofilerules != prules)
		clear();
	m_honourprofilerules = prules;
}

void CFMUAutoroute::set_deptime(time_t t)
{
	if (m_route.get_time_offblock_unix() != t)
		clear();
	m_route.set_time_offblock_unix(t);
}

void CFMUAutoroute::set_validator(validator_t v)
{
	m_validator = v;
}

std::string CFMUAutoroute::get_validator_socket(void) const
{
	if (!m_childsockaddr)
		return std::string();
#ifdef G_OS_UNIX
	return m_childsockaddr->get_path();
#else
	std::ostringstream oss;
	oss << m_childsockaddr->get_port();
	return oss.str();
#endif
}

void CFMUAutoroute::set_validator_socket(const std::string& path)
{
	m_childsockaddr.reset();
	if (path.empty())
		return;
#ifdef G_OS_UNIX
	m_childsockaddr = Gio::UnixSocketAddress::create(path, Gio::UNIX_SOCKET_ADDRESS_PATH);
#else
	m_childsockaddr = Gio::InetSocketAddress::create(Gio::InetAddress::create_loopback(Gio::SOCKET_FAMILY_IPV4), strtoul(path.c_str(), 0, 0));
#endif
}

void CFMUAutoroute::set_validator_binary(const std::string& path)
{
	m_childbinary = path;
}

void CFMUAutoroute::set_validator_xdisplay(int xdisp)
{
	m_childxdisplay = xdisp;
}

void CFMUAutoroute::set_db_directories(const std::string& dir_main, Engine::auxdb_mode_t auxdbmode, const std::string& dir_aux)
{
	closedb();
	m_dbdir_main = dir_main;
        switch (auxdbmode) {
	case Engine::auxdb_prefs:
		m_dbdir_aux = (Glib::ustring)m_prefs.globaldbdir;
		break;

	case Engine::auxdb_override:
		m_dbdir_aux = dir_aux;
		break;

	default:
		m_dbdir_aux.clear();
		break;
        }
	check_tfr_available();
}

void CFMUAutoroute::opendb(void)
{
	if (!m_airportdb.is_open()) {
		try {
			m_airportdb.open(get_db_maindir());
			if (!get_db_auxdir().empty() && m_airportdb.is_dbfile_exists(get_db_auxdir()))
				m_airportdb.attach(get_db_auxdir());
			if (m_airportdb.is_aux_attached())
				std::cerr << "Auxillary airports database attached" << std::endl;
		} catch (const std::exception& e) {
			m_signal_log(log_normal, std::string("Error opening airports database: ") + e.what());
		}
	}
	if (!m_navaiddb.is_open()) {
		try {
			m_navaiddb.open(get_db_maindir());
			if (!get_db_auxdir().empty() && m_navaiddb.is_dbfile_exists(get_db_auxdir()))
				m_navaiddb.attach(get_db_auxdir());
			if (m_navaiddb.is_aux_attached())
				std::cerr << "Auxillary navaids database attached" << std::endl;
		} catch (const std::exception& e) {
			m_signal_log(log_normal, std::string("Error opening navaids database: ") + e.what());
		}
	}
	if (!m_waypointdb.is_open()) {
		try {
			m_waypointdb.open(get_db_maindir());
			if (!get_db_auxdir().empty() && m_waypointdb.is_dbfile_exists(get_db_auxdir()))
				m_waypointdb.attach(get_db_auxdir());
			if (m_waypointdb.is_aux_attached())
				std::cerr << "Auxillary waypoints database attached" << std::endl;
		} catch (const std::exception& e) {
			m_signal_log(log_normal, std::string("Error opening waypoints database: ") + e.what());
		}
	}
	if (!m_airspacedb.is_open()) {
		try {
			m_airspacedb.open(get_db_maindir());
			if (!get_db_auxdir().empty() && m_airspacedb.is_dbfile_exists(get_db_auxdir()))
				m_airspacedb.attach(get_db_auxdir());
			if (m_airspacedb.is_aux_attached())
				std::cerr << "Auxillary airspaces database attached" << std::endl;
		} catch (const std::exception& e) {
			m_signal_log(log_normal, std::string("Error opening airspaces database: ") + e.what());
		}
	}
	if (!m_airwaydb.is_open()) {
		try {
			m_airwaydb.open(get_db_maindir());
			if (!get_db_auxdir().empty() && m_airwaydb.is_dbfile_exists(get_db_auxdir()))
				m_airwaydb.attach(get_db_auxdir());
			if (m_airwaydb.is_aux_attached())
				std::cerr << "Auxillary airways database attached" << std::endl;
		} catch (const std::exception& e) {
			m_signal_log(log_normal, std::string("Error opening airways database: ") + e.what());
		}
	}
	if (!m_mapelementdb.is_open()) {
		try {
			m_mapelementdb.open(get_db_auxdir().empty() ? Engine::get_default_aux_dir() : get_db_auxdir());
		} catch (const std::exception& e) {
			m_signal_log(log_normal, std::string("Error opening mapelements database: ") + e.what());
		}
	}
	try {
		m_topodb.open(get_db_auxdir().empty() ? Engine::get_default_aux_dir() : get_db_auxdir());
	} catch (const std::exception& e) {
		m_signal_log(log_normal, std::string("Error opening topo database: ") + e.what());
	}
}

void CFMUAutoroute::closedb(void)
{
	if (m_airportdb.is_open())
		m_airportdb.close();
	if (m_navaiddb.is_open())
		m_navaiddb.close();
	if (m_waypointdb.is_open())
		m_waypointdb.close();
	if (m_airwaydb.is_open())
		m_airwaydb.close();
	if (m_airspacedb.is_open())
		m_airspacedb.close();
	if (m_mapelementdb.is_open())
		m_mapelementdb.close();
	m_topodb.close();
}

bool CFMUAutoroute::find_airport(AirportsDb::Airport& el, const std::string& icao, const std::string& name)
{
	opendb();
	AirportsDb::elementvector_t ev;
	if (!icao.empty())
		ev = m_airportdb.find_by_icao(icao, 0, AirportsDb::comp_contains, 0, AirportsDb::Airport::subtables_all);
	if (!name.empty()) {
		AirportsDb::elementvector_t ev1(m_airportdb.find_by_name(name, 0, AirportsDb::comp_contains, 0, AirportsDb::Airport::subtables_all));
		ev.insert(ev.end(), ev1.begin(), ev1.end());
	}
	if (ev.empty())
		return false;
	// remove duplicate airports
	for (unsigned int i = 0; i < ev.size(); ++i)
		for (unsigned int j = i + 1; j < ev.size();) {
			if (ev[i].get_sourceid() != ev[j].get_sourceid()) {
				++j;
				continue;
			}
			ev.erase(ev.begin() + j);
		}
	if (ev.size() != 1) {
		typedef std::multimap<int,unsigned int> priomap_t;
		priomap_t priomap;
		std::string icaou(Glib::ustring(icao).uppercase()), nameu(Glib::ustring(name).uppercase());
		for (unsigned int i = 0; i < ev.size(); ++i) {
			const AirportsDb::Airport& arpt(ev[i]);
			std::string iu(arpt.get_icao().uppercase());
			std::string nu(arpt.get_name().uppercase());
			int prio(0);
			if (iu == icaou)
				prio -= 2;
			else if (iu.find(icaou) != std::string::npos)
				--prio;
			if (nu == nameu)
				prio -= 2;
			else if (nu.find(nameu) != std::string::npos)
				--prio;
			priomap.insert(std::make_pair(prio, i));
		}
		priomap_t::const_iterator it(priomap.begin());
		if (it == priomap.end() || it->first == 0)
			return false;
		priomap_t::const_iterator it2(it);
		++it2;
		if (it2 != priomap.end() && it2->first == it->first)
			return false;
		el = ev[it->second];
		return true;
	}
	el = ev[0];
	return true;
}

bool CFMUAutoroute::find_point(Point& pt, std::string& ident, Engine::AirwayGraphResult::Vertex::type_t& typ, const std::string& name, bool ifr, const Point& ptnear, bool exact)
{
	static const unsigned int limit = 1024;
	ident.clear();
	typ = Engine::AirwayGraphResult::Vertex::type_undefined;
	if (name.empty())
		return false;
	typedef std::vector<Point> result_t;
	result_t result;
	typedef std::vector<Engine::AirwayGraphResult::Vertex::type_t> resulttype_t;
	resulttype_t resulttype;
	typedef std::vector<std::string> resultident_t;
	resultident_t resultident;
	std::string nameu(Glib::ustring(name).uppercase());
	{
		NavaidsDb::elementvector_t ev(m_navaiddb.find_by_text(nameu, limit, exact ? NavaidsDb::comp_exact : NavaidsDb::comp_contains, 0, NavaidsDb::Navaid::subtables_all));
		for (NavaidsDb::elementvector_t::const_iterator ei(ev.begin()), ee(ev.end()); ei != ee; ++ei) {
			const NavaidsDb::Navaid& el(*ei);
			if (!el.is_valid() ||
			    (!exact && el.get_icao().uppercase().find(nameu) == std::string::npos &&
			     el.get_name().uppercase().find(nameu) == std::string::npos) ||
			    (exact && el.get_icao().uppercase() != nameu &&
			     el.get_name().uppercase() != nameu))
				continue;
			result.push_back(el.get_coord());
			resulttype.push_back(Engine::AirwayGraphResult::Vertex::type_navaid);
			resultident.push_back(el.get_icao());
		}
	}
	{
		WaypointsDb::elementvector_t ev(m_waypointdb.find_by_text(nameu, limit, exact ? WaypointsDb::comp_exact : WaypointsDb::comp_contains, 0, WaypointsDb::Waypoint::subtables_all));
		for (WaypointsDb::elementvector_t::const_iterator ei(ev.begin()), ee(ev.end()); ei != ee; ++ei) {
			const WaypointsDb::Waypoint& el(*ei);
			if (!el.is_valid() ||
			    (!exact && el.get_name().uppercase().find(nameu) == std::string::npos) ||
			    (exact && el.get_name().uppercase() != nameu))
				continue;
			result.push_back(el.get_coord());
			resulttype.push_back(Engine::AirwayGraphResult::Vertex::type_intersection);
			resultident.push_back(el.get_name());
		}
	}
	if (!ifr) {
		{
			Point pt;
			if (!((Point::setstr_lat | Point::setstr_lon) & ~pt.set_str(name)) && !pt.is_invalid()) {
				result.push_back(pt);
				resulttype.push_back(Engine::AirwayGraphResult::Vertex::type_undefined);
				resultident.push_back(std::string());
			}
		}
		{
			AirportsDb::elementvector_t ev(m_airportdb.find_by_text(nameu, limit, exact ? AirportsDb::comp_exact : AirportsDb::comp_contains, 0, AirportsDb::Airport::subtables_all));
			for (AirportsDb::elementvector_t::const_iterator ei(ev.begin()), ee(ev.end()); ei != ee; ++ei) {
				const AirportsDb::Airport& el(*ei);
				if (!el.is_valid() ||
				    (!exact && el.get_icao().uppercase().find(nameu) == std::string::npos &&
				     el.get_name().uppercase().find(nameu) == std::string::npos) ||
				    (exact && el.get_icao().uppercase() != nameu &&
				     el.get_name().uppercase() != nameu))
					continue;
				result.push_back(el.get_coord());
				resulttype.push_back(Engine::AirwayGraphResult::Vertex::type_airport);
				resultident.push_back(el.get_icao());
			}
		}
		{
			MapelementsDb::elementvector_t ev(m_mapelementdb.find_by_text(nameu, limit, exact ? MapelementsDb::comp_exact : MapelementsDb::comp_contains, 0, MapelementsDb::Mapelement::subtables_all));
			for (MapelementsDb::elementvector_t::const_iterator ei(ev.begin()), ee(ev.end()); ei != ee; ++ei) {
				const MapelementsDb::Mapelement& el(*ei);
				if (!el.is_valid() ||
				    (!exact && el.get_name().uppercase().find(nameu) == std::string::npos) ||
				    (exact && el.get_name().uppercase() != nameu))
					continue;
				switch (el.get_typecode()) {
				case MapelementsDb::Mapelement::maptyp_city:
				case MapelementsDb::Mapelement::maptyp_village:
				case MapelementsDb::Mapelement::maptyp_spot:
				case MapelementsDb::Mapelement::maptyp_landmark:
				case MapelementsDb::Mapelement::maptyp_lake:
				case MapelementsDb::Mapelement::maptyp_lake_t:
					break;

				default:
					continue;
				}
				result.push_back(el.get_coord());
				resulttype.push_back(Engine::AirwayGraphResult::Vertex::type_mapelement);
				resultident.push_back(el.get_name());
			}
		}
	}
	if (ptnear.is_invalid()) {
		if (result.size() == 1) {
			pt = result[0];
			ident = resultident[0];
			typ = resulttype[0];
			return true;
		}
		return false;
	}
	if (result.empty())
		return false;
	uint64_t dist(ptnear.simple_distance_rel(result[0]));
	unsigned int idx(0);
	for (unsigned i = 1, j = result.size(); i < j; ++i) {
		uint64_t d(ptnear.simple_distance_rel(result[i]));
		if (d >= dist)
			continue;
		dist = d;
		idx = i;
	}
	pt = result[idx];
	ident = resultident[idx];
	typ = resulttype[idx];
	return true;
}

std::string CFMUAutoroute::get_simplefpl(void) const
{
	if (m_performance.empty() || m_route.get_nrwpt() < 2)
		return "";
	std::ostringstream fpl;
	std::ostringstream otherinfo;
	char flightrules(m_route.get_flightrules());
	const FPlanWaypoint& wptdep(m_route[0]);
	const FPlanWaypoint& wptdest(m_route[m_route.get_nrwpt() - 1]);
	// find first route point to report
	unsigned int idx(1);
	for (;;) {
		if (idx + 2 >= m_route.get_nrwpt())
			break;
		const FPlanWaypoint& wpt(m_route[idx]);
		if (wpt.get_pathcode() != FPlanWaypoint::pathcode_sid &&
		    wpt.get_ident().size() >= 2 &&
		    !Engine::AirwayGraphResult::Vertex::is_ident_numeric(wpt.get_ident()))
			break;
		++idx;
	}
	int alt(std::numeric_limits<int>::min());
	uint16_t flags(wptdep.get_flags());
	if (flightrules == 'V') {
		alt = m_route.max_altitude(flags);
	} else if (flags & FPlanWaypoint::altflag_ifr) {
		if (m_route.get_nrwpt() == 2) {
			alt = std::max(m_route[0].get_altitude(), m_route[1].get_altitude());
			if (alt >= 5000)
				alt += 1000;
			alt += 1999;
			alt /= 1000;
			alt *= 1000;
		} else if (m_route.get_nrwpt() > 2) {
			alt = m_route[idx].get_altitude();
			flags ^= (flags ^ m_route[idx].get_flags()) & FPlanWaypoint::altflag_standard;
			if (m_pathprobe >= 0) {
				if (m_pathprobe != 0)
					flags &= ~FPlanWaypoint::altflag_ifr;
				if (m_pathprobe >= m_route.get_nrwpt() || !(m_route[m_pathprobe].is_ifr()))
					flightrules = 'V';
				else
					flightrules = (flags & FPlanWaypoint::altflag_ifr) ? 'Y' : 'Z';
			}
		}
	}
	fpl << "-(FPL-";
	if (!m_done || m_aircraft.get_callsign().empty())
		fpl << m_callsign;
	else
		fpl << m_aircraft.get_callsign();
	fpl << '-' << flightrules << "G -1" << m_aircraft.get_icaotype()
	    << '/' << m_aircraft.get_wakecategory() << " -" << m_aircraft.get_equipment_string() << '/' << m_aircraft.get_transponder_string()
	    << " -";
	if (AirportsDb::Airport::is_fpl_zzzz(wptdep.get_icao())) {
		fpl << "ZZZZ";
		otherinfo << "DEP/" << wptdep.get_name() << ' ' << wptdep.get_coord().get_fpl_str() << ' ';
	} else {
		fpl << wptdep.get_icao();
	}
	if (m_aircraft.get_pbn() != Aircraft::pbn_none)
		otherinfo << "PBN/" << m_aircraft.get_pbn_string() << ' ';
	{
		struct tm utm;
		time_t deptime(wptdep.get_time_unix());
		gmtime_r(&deptime, &utm);
		fpl << std::setw(2) << std::setfill('0') << utm.tm_hour
		    << std::setw(2) << std::setfill('0') << utm.tm_min
		    << " -";
		if ((deptime >= (m_starttime + 60*60)) && (deptime <= (m_starttime + 5*24*60*60))) {
			// also add date if in the future
			otherinfo << "DOF/" << std::setw(2) << std::setfill('0') << (utm.tm_year % 100)
				  << std::setw(2) << std::setfill('0') << (utm.tm_mon + 1)
				  << std::setw(2) << std::setfill('0') << utm.tm_mday << ' ';
		}
	}
	if (alt == std::numeric_limits<int>::min())
		fpl << m_performance.get_cruise(findperfindex(m_route.max_altitude())).get_fpltas() << "VFR";
	else
		fpl << m_performance.get_cruise(findperfindex(alt)).get_fpltas()
		    << ((flags & FPlanWaypoint::altflag_standard) ? 'F' : 'A')
		    << std::setw(3) << std::setfill('0') << ((alt + 50) / 100);
	if (m_route.get_nrwpt() > 2) {
		alt = m_route[1].get_altitude();
		flags ^= (flags ^ m_route[1].get_flags()) & FPlanWaypoint::altflag_standard;
	}
	std::ostringstream eet;
	if (m_route.get_nrwpt() == 2) {
		if (wptdep.get_flags() & wptdest.get_flags() & FPlanWaypoint::altflag_ifr)
			fpl << " DCT";
	} else {
		if (true || (flags & FPlanWaypoint::altflag_ifr)) {
			switch (wptdep.get_pathcode()) {
			case FPlanWaypoint::pathcode_directto:
				fpl << " DCT";
				break;

			case FPlanWaypoint::pathcode_sid:
				fpl << ' ' << wptdep.get_pathname();
				break;

			default:
				break;
			}
		}
		for (;;) {
			const FPlanWaypoint& wpt(m_route[idx]);
			++idx;
			std::string ident(wpt.get_icao());
			if (ident.empty() || ident == wptdep.get_icao() || ident == wptdest.get_icao())
				ident = wpt.get_name();
			{
				const FPlanWaypoint& wptprev(m_route[idx-2]);
				if (ident.empty() || 
				    ((Engine::AirwayGraphResult::Vertex::is_ident_numeric(ident) || ident.size() < 2) &&
				     wpt.get_pathcode() == FPlanWaypoint::pathcode_airway &&
				     wptprev.get_pathcode() == FPlanWaypoint::pathcode_airway &&
				     wpt.get_pathname() == wptprev.get_pathname() &&
				     (wpt.get_flags() & wptprev.get_flags() & FPlanWaypoint::altflag_ifr))) {
					if (idx + 1 >= m_route.get_nrwpt())
						break;
					continue;
				}
			}
			fpl << ' ' << ident;
			uint16_t flgchg(flags);
			flags = wpt.get_flags();
			if (m_pathprobe >= 0 && idx != m_pathprobe + 1)
				flags &= ~FPlanWaypoint::altflag_ifr;
			flgchg ^= flags;
			{
				int a(wpt.get_altitude());
				if (alt != a)
					flgchg |= FPlanWaypoint::altflag_standard;
				alt = a;
			}
			if (idx + 1 < m_route.get_nrwpt() && wpt.get_pathcode() != FPlanWaypoint::pathcode_star) {
				const FPlanWaypoint& wptnext(m_route[idx]);
				uint16_t flagsnext(wptnext.get_flags());
				int altnext(wptnext.get_altitude());
				if ((flags ^ flagsnext) & FPlanWaypoint::altflag_standard ||
				    alt != altnext || flgchg & flags & FPlanWaypoint::altflag_ifr)
					fpl << '/' << m_performance.get_cruise(findperfindex(altnext)).get_fpltas()
					    << FPlanWaypoint::get_altchar(flagsnext)
					    << std::setw(3) << std::setfill('0') << ((altnext + 50) / 100);
			}
			if (flgchg & FPlanWaypoint::altflag_ifr) {
				fpl << ' ' << FPlanWaypoint::get_ruleschar(flags) << "FR";
				if (flags & FPlanWaypoint::altflag_ifr) {
					unsigned int tm(wpt.get_time_unix() - wptdep.get_time_unix());
					tm += 30;
					tm /= 60;
					unsigned int hr(tm / 60);
					tm -= hr * 60;
					eet << ident << std::setw(2) << std::setfill('0') << hr
					    << std::setw(2) << std::setfill('0') << tm << ' ';
				}
			}
			if (true || (flags & FPlanWaypoint::altflag_ifr)) {
				switch (wpt.get_pathcode()) {
				case FPlanWaypoint::pathcode_directto:
					fpl << " DCT";
					break;

				case FPlanWaypoint::pathcode_airway:
				case FPlanWaypoint::pathcode_star:
					fpl << ' ' << wpt.get_pathname();
					break;

				default:
					break;
				}
			}
			if (idx + 1 >= m_route.get_nrwpt() || wpt.get_pathcode() == FPlanWaypoint::pathcode_star)
				break;
		}
		{
			uint16_t flgchg(flags);
			flags = wptdest.get_flags();
			flgchg ^= flags;
			if (flgchg & FPlanWaypoint::altflag_ifr && m_pathprobe < 0)
				fpl << ' ' << FPlanWaypoint::get_ruleschar(flags) << "FR";
		}
	}
	fpl << " -";
	if (AirportsDb::Airport::is_fpl_zzzz(wptdest.get_icao())) {
		fpl << "ZZZZ";
		otherinfo << "DEST/" << wptdest.get_name() << ' ' << wptdest.get_coord().get_fpl_str() << ' ';
	} else {
		fpl << wptdest.get_icao();
	}
	{
		//unsigned int tm(wptdest.get_time_unix() - wptdep.get_time_unix());
		unsigned int tm(m_routezerowindtime);
		tm += 30;
		tm /= 60;
		unsigned int hr(tm / 60);
		tm -= hr * 60;
		fpl << std::setw(2) << std::setfill('0') << hr
		    << std::setw(2) << std::setfill('0') << tm;
	}
	for (unsigned int i = 0; i < 2; ++i) {
		if (get_alternate(i).empty())
			continue;
		fpl << ' ' << get_alternate(i);
	}
	fpl << " -" << otherinfo.str();
	if (!eet.str().empty())
		fpl << "EET/" << eet.str();
	// add remark: POGO
	if (wptdep.is_ifr() && wptdest.is_ifr() && 
	    IcaoFlightPlan::is_route_pogo(wptdep.get_icao(), wptdest.get_icao()))
		fpl << " RMK/POGO";
	fpl << ')';
	return fpl.str();
}

void CFMUAutoroute::updatefpl(void)
{
	m_routefuel = 0;
	m_routezerowindfuel = 0;
	double rttime(0), rtzwtime(0);
	time_t tm(m_route.get_time_offblock_unix());
	tm += 5 * 60;
        const unsigned int pis(m_performance.size());
	unsigned int prevpi(pis);
	for (unsigned int i = 0; i < m_route.get_nrwpt(); ++i) {
		FPlanWaypoint& wpt(m_route[i]);
		unsigned int pi(pis);
		bool isenroute(false);
		if (i > 0) {
			isenroute = (i + 1) < m_route.get_nrwpt();
			if (isenroute)
				pi = findperfindex(wpt.get_altitude());
		}
		const Performance::Cruise& cruise(m_performance.get_cruise(prevpi, pi));
		{
			Wind<double> wnd(cruise.get_wind(wpt.get_coord()));
			wpt.set_winddir_deg(wnd.get_winddir());
			wpt.set_windspeed_kts(wnd.get_windspeed());
		}
		{
			double temp(cruise.get_temp(wpt.get_coord()));
			if (!std::isnan(temp))
				wpt.set_oat_kelvin(temp);
			else
				wpt.set_isaoffset_kelvin(m_isaoffs);
		}
		if (m_grib2prmsl)
			wpt.set_qff_hpa(m_grib2prmsl->operator()(wpt.get_coord()) * 0.01f);
		else
			wpt.set_qff_hpa(m_qnh);
		if (i > 0) {
			FPlanWaypoint& wptprev(m_route[i - 1]);
			if (prevpi == pis || (prevpi < pi && pi != pis))
				wptprev.frob_flags(FPlanWaypoint::altflag_climb, FPlanWaypoint::altflag_climb);
			if (pi == pis || (pi < prevpi && prevpi != pis))
				wptprev.frob_flags(FPlanWaypoint::altflag_descent, FPlanWaypoint::altflag_descent);
			double tmadd(0), tmaddzw(0);
			if (pi < pis || prevpi < pis) {
				const Performance::Cruise& cruise(m_performance.get_cruise(prevpi, pi));
				double dist(wptprev.get_coord().spheric_distance_nmi_dbl(wpt.get_coord()));
				double tt(wptprev.get_coord().spheric_true_course_dbl(wpt.get_coord()));
				double wdist(dist);
				wptprev.set_dist_nmi(dist);
				wptprev.set_truetrack_deg(tt);
				wptprev.set_trueheading_deg(tt);
				wptprev.set_tas_kts(cruise.get_tas());
				wptprev.set_truealt(cruise.get_truealtitude());
				if (i > 1) {
					wptprev.set_rpm(cruise.get_rpm());
					wptprev.set_mp(cruise.get_mp());
				}
				if (m_windenable) {
					Wind<double> wnd;
					wnd.set_wind(wptprev.get_winddir_deg(), wptprev.get_windspeed_kts(), wpt.get_winddir_deg(), wpt.get_windspeed_kts());
					wnd.set_crs_tas(tt, cruise.get_tas());
					wptprev.set_trueheading_deg(wnd.get_hdg());
					if (wnd.get_gs() >= 0.1)
						wdist *= cruise.get_tas() / wnd.get_gs();
					if (false) {
						std::ostringstream oss;
						oss << "Leg " << i << " Wind " << std::fixed << std::setprecision(0)
						    << std::setw(3) << std::setfill('0') << wnd.get_winddir() << '/'
						    << std::setw(2) << std::setfill('0') << wnd.get_windspeed()
						    << " TT " << wnd.get_crs() << " TH " << wnd.get_hdg()
						    << " TAS " << wnd.get_tas() << " GS " << wnd.get_gs();
						m_signal_log(log_debug0, oss.str());
					}
				}
				tmadd = wdist * cruise.get_secpernmi();
				tmaddzw = dist * cruise.get_secpernmi();
				m_routefuel += tmadd * cruise.get_fuelpersec();
				m_routezerowindfuel += tmaddzw * cruise.get_fuelpersec();
				const Performance::LevelChange& lvl(m_performance.get_levelchange(prevpi, pi));
				tmadd += lvl.get_timepenalty();
				tmaddzw += lvl.get_timepenalty();
				m_routefuel += lvl.get_fuelpenalty();
				m_routezerowindfuel += lvl.get_fuelpenalty();
			}
			tm += Point::round<time_t,double>(tmadd);
			rttime += tmadd;
			rtzwtime += tmaddzw;
		}
		wpt.set_time_unix(tm);
		wpt.set_flighttime(Point::round<time_t,double>(rttime));
		wpt.set_fuel_usg(m_routefuel);
		prevpi = pi;
	}
	tm += 5 * 60;
	m_route.set_time_onblock_unix(tm);
	m_routetime = Point::round<unsigned int,double>(rttime);
	m_routezerowindtime = Point::round<unsigned int,double>(rtzwtime);
	m_route.turnpoints();
}

bool CFMUAutoroute::is_running(void) const
{
	return m_running;
}

bool CFMUAutoroute::is_errorfree(void) const
{
	return m_done;
}

Glib::TimeVal CFMUAutoroute::get_wallclocktime(void) const
{
	if (m_running) {
		Glib::TimeVal tv;
		tv.assign_current_time();
		tv -= m_tvelapsed;
		return tv;
	}
	return m_tvelapsed;
}

Glib::TimeVal CFMUAutoroute::get_validatortime(void) const
{
	return m_tvvalidator;
}

double CFMUAutoroute::get_gcdistance(void) const
{
	if (!m_airport[0].is_valid() || !m_airport[1].is_valid())
		return std::numeric_limits<double>::quiet_NaN();
	return m_airport[0].get_coord().spheric_distance_nmi_dbl(m_airport[1].get_coord());
}

double CFMUAutoroute::get_routedistance(void) const
{
	if (m_route.get_nrwpt())
		return m_route.total_distance_nmi();
	return std::numeric_limits<double>::quiet_NaN();
}

unsigned int CFMUAutoroute::get_routetime(bool wind) const
{
	if (m_route.get_nrwpt())
		return wind ? m_routetime : m_routezerowindtime;
	return 0;
}

double CFMUAutoroute::get_routefuel(bool wind) const
{
	if (m_route.get_nrwpt())
		return wind ? m_routefuel : m_routezerowindfuel;
	return std::numeric_limits<double>::quiet_NaN();
}

unsigned int CFMUAutoroute::get_mintime(bool wind) const
{
	if (!m_airport[0].is_valid() || !m_airport[1].is_valid() || m_performance.empty())
		return 0;
	double dist(m_airport[0].get_coord().spheric_distance_nmi_dbl(m_airport[1].get_coord()));
	double crs(m_airport[0].get_coord().spheric_true_course_dbl(m_airport[1].get_coord()));
	Point pthalfway(m_airport[0].get_coord().halfway(m_airport[1].get_coord()));
	double x(std::numeric_limits<double>::max());
	const unsigned int pis(m_performance.size());
	for (unsigned int pi = 0; pi < m_performance.size(); ++pi) {
		const Performance::Cruise& cruise(m_performance.get_cruise(pi));
		double y(dist);
		if (m_windenable && wind) {
			Wind<double> wnd(cruise.get_wind(pthalfway));
			wnd.set_crs_tas(crs, cruise.get_tas());
			if (wnd.get_gs() >= 0.1)
				y *= cruise.get_tas() / wnd.get_gs();
		}
		y *= cruise.get_secpernmi();
		y += m_performance.get_levelchange(pis, pi).get_timepenalty() + m_performance.get_levelchange(pi, pis).get_timepenalty();
		if (!is_metric_invalid(y))
			x = std::min(x, y);
	}
	if (x == std::numeric_limits<double>::max())
		return 0;
	return Point::round<unsigned int, double>(x);
}

double CFMUAutoroute::get_minfuel(bool wind) const
{
	if (!m_airport[0].is_valid() || !m_airport[1].is_valid() || m_performance.empty())
		return std::numeric_limits<double>::quiet_NaN();
	double dist(m_airport[0].get_coord().spheric_distance_nmi_dbl(m_airport[1].get_coord()));
	double crs(m_airport[0].get_coord().spheric_true_course_dbl(m_airport[1].get_coord()));
	Point pthalfway(m_airport[0].get_coord().halfway(m_airport[1].get_coord()));
	double x(std::numeric_limits<double>::max());
	const unsigned int pis(m_performance.size());
	for (unsigned int pi = 0; pi < m_performance.size(); ++pi) {
		const Performance::Cruise& cruise(m_performance.get_cruise(pi));
		double y(dist);
		if (m_windenable && wind) {
			Wind<double> wnd(cruise.get_wind(pthalfway));
			wnd.set_crs_tas(crs, cruise.get_tas());
			if (wnd.get_gs() >= 0.1)
				y *= cruise.get_tas() / wnd.get_gs();
		}
		y *= cruise.get_secpernmi() * cruise.get_fuelpersec();
		y += m_performance.get_levelchange(pis, pi).get_fuelpenalty() + m_performance.get_levelchange(pi, pis).get_fuelpenalty();
		if (!is_metric_invalid(y))
			x = std::min(x, y);
	}
	if (x == std::numeric_limits<double>::max())
		return std::numeric_limits<double>::quiet_NaN();
	return x;
}

void CFMUAutoroute::get_cruise_performance(int& alt, double& densityalt, double& tas) const
{
	densityalt = tas = std::numeric_limits<double>::quiet_NaN();
        const unsigned int pis(m_performance.size());
	if (!pis)
		alt = std::numeric_limits<int>::min();
	unsigned int pibest(0);
	unsigned int diffbest;
	{
		const Performance::Cruise& cruise(m_performance.get_cruise(pibest));
		diffbest = abs(cruise.get_altitude() - alt);
	}
	for (unsigned int pi = 1; pi < pis; ++pi) {
		const Performance::Cruise& cruise(m_performance.get_cruise(pi));
		unsigned int diff(abs(cruise.get_altitude() - alt));
		if (diff > diffbest)
			continue;
		diffbest = diff;
		pibest = pi;
	}
	const Performance::Cruise& cruise(m_performance.get_cruise(pibest));
	alt = cruise.get_altitude();
	densityalt = cruise.get_densityaltitude();
	tas = cruise.get_tas();
}

void CFMUAutoroute::reload(void)
{
	m_opsperfloaded = m_grib2loaded = false;
	closedb();
	opendb();
	preload(false);
}

void CFMUAutoroute::preload(bool validator)
{
	if (validator)
		child_run();
	preload_opsperf();
	preload_wind();
}

void CFMUAutoroute::preload_wind(void)
{
	if (!m_windenable || m_grib2loaded)
		return;
	GRIB2::Parser p(m_grib2);
	std::ostringstream oss;
	std::string path(m_dbdir_main);
	if (path.empty())
		path = FPlan::get_userdbpath();
	path = Glib::build_filename(path, "gfs");
	p.parse(path);
	unsigned int count1(m_grib2.find_layers().size());
	oss << "Loaded " << count1 << " GRIB2 Layers from " << path;
	if (!m_dbdir_aux.empty()) {
		p.parse(m_dbdir_aux);
		unsigned int count2(m_grib2.find_layers().size() - count1);
		oss << " and " << count2 << " GRIB2 Layers from " << m_dbdir_aux;
	}
	m_signal_log(log_normal, oss.str());
	m_grib2loaded = true;
}

void CFMUAutoroute::preload_opsperf(void)
{
	if (m_opsperfloaded)
		return;
	m_opsperf.clear();
	if (m_dbdir_aux.empty())
		m_opsperf.load();
	else
		m_opsperf.load(m_dbdir_aux + "/opsperf");
	m_opsperfloaded = true;
}

void CFMUAutoroute::start(void)
{
	start(false);
}

void CFMUAutoroute::cont(void)
{
	start(true);
}

void CFMUAutoroute::start(bool cont)
{
	stop(statusmask_stoppingerroruser);
	{
		std::string logdir(m_logprefix);
		unsigned int i;
		for (i = 0; i < 2; ++i, logdir = Glib::build_filename(Glib::get_tmp_dir(), "cfmuautoroute-XXXXXX")) {
			if (logdir.empty())
				continue;
			if (logdir.find("XXXXXX") != std::string::npos) {
				if (!g_mkdtemp_full(const_cast<char *>(logdir.c_str()), 0750))
					continue;
			} else {
				if (g_mkdir_with_parents(logdir.c_str(), 0750) && false)
					continue;
			}
			if (Glib::file_test(logdir, Glib::FILE_TEST_EXISTS) && Glib::file_test(logdir, Glib::FILE_TEST_IS_DIR))
				break;
		}
		if (i >= 2) {
			m_signal_log(log_normal, "Cannot open logging directory " + m_logprefix);
			logdir.clear();
		}
		m_logdir.swap(logdir);
	}
	child_run();
	m_done = false;
	time(&m_starttime);
	bool ok(get_departure().is_valid() && get_destination().is_valid());
	bool perferr(false);
	if (ok) {
		computeperformance();
		perferr = m_performance.empty();
		ok = ok && !perferr;
	}
	if (!ok) {
		m_tvelapsed.assign_current_time();
		m_tvvalidator = m_tvvalidatestart = Glib::TimeVal(0, 0);
		m_running = true;
		m_signal_statuschange(statusmask_starting);
		if (!get_departure().is_valid())
			m_signal_log(log_normal, "Invalid Departure Aerodrome");
		if (!get_destination().is_valid())
			m_signal_log(log_normal, "Invalid Destination Aerodrome");
		if (perferr)
			m_signal_log(log_normal, "Performance calculation error - aircraft unable to climb to minimum level");
		clear();
		return;
	}
	if (m_airportifr[0] &&
	    (get_departure().get_flightrules() & (AirportsDb::Airport::flightrules_dep_ifr | AirportsDb::Airport::flightrules_dep_vfr))
	    == AirportsDb::Airport::flightrules_dep_vfr) {
		m_airportifr[0] = false;
		m_forceenrifr = true;
	}
	if (m_airportifr[1] &&
	    (get_destination().get_flightrules() & (AirportsDb::Airport::flightrules_arr_ifr | AirportsDb::Airport::flightrules_arr_vfr))
	    == AirportsDb::Airport::flightrules_arr_vfr) {
		m_airportifr[1] = false;
		m_forceenrifr = true;
	}
	if (!(m_airportifr[0] || m_airportifr[1] || m_forceenrifr)) {
		clear();
		m_tvelapsed.assign_current_time();
		m_tvvalidator = m_tvvalidatestart = Glib::TimeVal(0, 0);
		m_running = true;
		m_signal_statuschange(statusmask_starting);
		m_signal_log(log_normal, "Starting VFR router");
		m_iterationcount[0] = m_iterationcount[1] = 0;
		m_done = start_vfr();
		if (m_done) {
			m_iterationcount[0] = 1;
			m_signal_log(log_fplproposal, get_simplefpl());
		}
		stop(statusmask_stoppingerrorenroute);
		return;
	}
	if (!start_ifr(cont))
		stop(statusmask_stoppingerrorinternalerror);
}

void CFMUAutoroute::logwxlayers(const GRIB2::layerlist_t& ll, int alt)
{
	for (GRIB2::layerlist_t::const_iterator li(ll.begin()), le(ll.end()); li != le; ++li) {
		if (!*li)
			continue;
		std::ostringstream oss;
		if (alt >= 0)
			oss << 'F' << Glib::ustring::format(std::setw(3), std::setfill(L'0'), (alt + 50) / 100);
		else
			oss << '-';
		{
			Glib::TimeVal tvr((*li)->get_reftime(), 0);
			Glib::TimeVal tve((*li)->get_efftime(), 0);
			oss << ' ' << tvr.as_iso8601() << ' ' << tve.as_iso8601() << ' ';
		}
		{
			const GRIB2::Parameter *param((*li)->get_parameter());
			oss << param->get_abbrev("-") << " \""
			    << GRIB2::find_surfacetype_str((*li)->get_surface1type(), "-") << "\" "
			    << (*li)->get_surface1value() << " \""
			    << GRIB2::find_surfacetype_str((*li)->get_surface2type(), "-") << "\" "
			    << (*li)->get_surface2value();
		}
		if (true) {
			oss << " \"" << GRIB2::find_centerid_str((*li)->get_centerid(), "") << "\"";
		}
		m_signal_log(log_weatherdata, oss.str());
	}
}

void CFMUAutoroute::computeperformance(void)
{
	preload_wind();
	preload_opsperf();
	m_grib2prmsl = Glib::RefPtr<GRIB2::LayerResult>();
	const GRIB2::Parameter *grib2windu(0);
	const GRIB2::Parameter *grib2windv(0);
	const GRIB2::Parameter *grib2temp(0);
	double halfgcdist(0);
	Rect bbox;
	if (m_windenable) {
		grib2windu = GRIB2::find_parameter(GRIB2::param_meteorology_momentum_ugrd);
		grib2windv = GRIB2::find_parameter(GRIB2::param_meteorology_momentum_vgrd);
		grib2temp = GRIB2::find_parameter(GRIB2::param_meteorology_temperature_tmp);
		halfgcdist = 0.5 * get_gcdistance();
		bbox = get_bbox().oversize_nmi(100.f);
	}
	m_performance.clear();
	int level(m_levels[0]);
	if (m_airportifr[0] || m_airportifr[1] || m_forceenrifr)
		level = ((level + 4) / 10) * 10 + 5;
	else
		level = ((level + 9) / 10) * 10;
	double mass(m_aircraft.get_mtom()), isaoffs(get_isaoffs()), qnh(get_qnh());
	Aircraft::ClimbDescent climb(m_aircraft.calculate_climb("", mass, isaoffs, qnh));
	Aircraft::ClimbDescent descent(m_aircraft.calculate_descent("", mass, isaoffs, qnh));
	AirData<double> ad;
	ad.set_qnh_tempoffs(get_qnh(), get_isaoffs());
	while (level <= m_levels[1]) {
		double pa(level * 100);
		ad.set_pressure_altitude(pa);
		double da(ad.get_density_altitude());
		if (pa > climb.get_ceiling())
			break;
		if (climb.time_to_climbrate(climb.altitude_to_time(pa)) < 100)
			break;
		double truealt(ad.get_true_altitude());
		double tas(0), fuelflow(0), rpm, mp;
		// compute cruise performance
                {
			Aircraft::Cruise::EngineParams ep(get_engine_bhp(), get_engine_rpm(), get_engine_mp());
			double pa1(pa), mass(m_aircraft.get_mtom()), isaoffs(get_isaoffs()), qnh(get_qnh());
			m_aircraft.calculate_cruise(tas, fuelflow, pa1, mass, isaoffs, qnh, ep);
			m_signal_log(log_normal, Glib::ustring::format("Cruise: PA ", std::fixed, std::setprecision(0), pa1) +
				     Glib::ustring::format(" TAS ", std::fixed, std::setprecision(0), tas) + 
				     Glib::ustring::format(" Fuelflow ", std::fixed, std::setprecision(1), fuelflow) + 
				     Glib::ustring::format(" RPM ", std::fixed, std::setprecision(0), ep.get_rpm()) + 
				     Glib::ustring::format(" MP ", std::fixed, std::setprecision(1), ep.get_mp()) + 
				     Glib::ustring::format(" BHP ", std::fixed, std::setprecision(0), ep.get_bhp()) + 
				     Glib::ustring::format(" Fuel/NMI ", std::fixed, std::setprecision(4), fuelflow / tas));
			rpm = ep.get_rpm();
			mp = ep.get_mp();
                }
		if (tas <= 0)
			break;
		double secpernmi(3600.0 / tas);
		double ff(fuelflow * (1.0 / 3600.0));
		double metric(secpernmi);
		if (is_optpreferred()) {
			// preferred level model
        		metric = pow(get_preferred_penalty(), abs(level - get_preferred_level()) * 0.1);
		} else if (is_optfuel()) {
			metric *= ff;
		}
		// wind
		time_t t(get_deptime() + halfgcdist * secpernmi);
		float press(0);
		IcaoAtmosphere<float>::std_altitude_to_pressure(&press, 0, level * (100 * Point::ft_to_m));
		press *= 100;
		if (m_performance.empty()) {
			GRIB2::layerlist_t ll(m_grib2.find_layers(GRIB2::find_parameter(GRIB2::param_meteorology_mass_prmsl), t));
			Glib::RefPtr<GRIB2::LayerInterpolateResult> prmsl(GRIB2::interpolate_results(bbox, ll, t));
			if (prmsl) {
				m_grib2prmsl = prmsl->get_results(t);
				logwxlayers(ll);
			}
		}
		Glib::RefPtr<GRIB2::LayerResult> windu, windv, temp;
		if (grib2windu && grib2windv) {
			{
				GRIB2::layerlist_t ll(m_grib2.find_layers(grib2windu, t, GRIB2::surface_isobaric_surface, press));
				Glib::RefPtr<GRIB2::LayerInterpolateResult> intwindu(GRIB2::interpolate_results(bbox, ll, t, press));
				if (intwindu) {
					windu = intwindu->get_results(t, press);
					logwxlayers(ll, level * 100);
				}
                        }
                        {
				GRIB2::layerlist_t ll(m_grib2.find_layers(grib2windv, t, GRIB2::surface_isobaric_surface, press));
				Glib::RefPtr<GRIB2::LayerInterpolateResult> intwindv(GRIB2::interpolate_results(bbox, ll, t, press));
				if (intwindv) {
					windv = intwindv->get_results(t, press);
					logwxlayers(ll, level * 100);
				}
                        }
		}
		if (grib2temp) {
			GRIB2::layerlist_t ll(m_grib2.find_layers(grib2temp, t, GRIB2::surface_isobaric_surface, press));
			Glib::RefPtr<GRIB2::LayerInterpolateResult> inttemp(GRIB2::interpolate_results(bbox, ll, t, press));
			if (inttemp) {
				temp = inttemp->get_results(t, press);
				logwxlayers(ll, level * 100);
			}
		}
		m_performance.add(Performance::Cruise(level * 100, da, truealt, rpm, mp, secpernmi, ff, metric, windu, windv, temp));
		level += 10;
		if (level >= 415)
			level += 10;
	}
	if (true) {
		double da(ad.get_density_altitude());
		m_signal_log(log_normal, Glib::ustring::format("Cruise: Ceiling ", std::fixed, std::setprecision(0), climb.get_ceiling()) +
			     Glib::ustring::format(" DA ", std::fixed, std::setprecision(0), da) +
			     Glib::ustring::format(" ROC ", std::fixed, std::setprecision(0), climb.time_to_climbrate(climb.altitude_to_time(da))));
	}
	const unsigned int pis(m_performance.size());
	if (!pis)
		return;
	{
		const OperationsPerformance::Aircraft& opsperfacft(m_opsperf.find_aircraft(m_aircraft.get_icaotype()));
		// climb/descent between cruise levels
		for (unsigned int pi0 = 0; pi0 < pis; ++pi0) {
			for (unsigned int pi1 = 0; pi1 < pis; ++pi1) {
				if (pi0 == pi1)
					continue;
				const Performance::Cruise& cruise0(m_performance.get_cruise(pi0));
				const Performance::Cruise& cruise1(m_performance.get_cruise(pi1));
				if (pi0 > pi1) {
					// descent
					double t0(descent.altitude_to_time(cruise1.get_altitude()));
					double t1(descent.altitude_to_time(cruise0.get_altitude()));
					double tracknmi(descent.time_to_distance(t1) - descent.time_to_distance(t0));
					double timepenalty(-tracknmi * cruise1.get_secpernmi());
					double fuelpenalty(timepenalty * cruise1.get_fuelpersec());
					timepenalty += t1 - t0;
					fuelpenalty += descent.time_to_fuel(t1) - descent.time_to_fuel(t0);
					if (std::isnan(timepenalty) || timepenalty < 0)
						timepenalty = 0;
					if (std::isnan(fuelpenalty) || fuelpenalty < 0)
						fuelpenalty = 0;
					double metricpenalty(0);
					if (is_optpreferred())
						metricpenalty = -((pi0 - pi1) * m_preferreddescent);
					else if (is_optfuel())
						metricpenalty = fuelpenalty;
					else
						metricpenalty = timepenalty;
					if (std::isnan(metricpenalty))
						metricpenalty = 0;
					if (!m_honourlevelchangetrackmiles)
						tracknmi = 0;
					// opsperf model
					double tracknmiop(0);
					if (m_honouropsperftrackmiles && opsperfacft.is_valid()) {
						for (int alt = cruise0.get_altitude(); alt > cruise1.get_altitude(); ) {
							int altd = alt - cruise1.get_altitude();
							if (altd > 500)
								altd = 500;
							OperationsPerformance::Aircraft::ComputeResult res(opsperfacft.get_massref());
							res.set_qnh_temp(); // ISA
							res.set_pressure_altitude(alt);
							alt -= altd;
							if (!opsperfacft.compute(res, OperationsPerformance::Aircraft::compute_descent) ||
							    !(res.get_rocd() < 0))
								continue;
							tracknmiop += (altd / -res.get_rocd()) * (1.0 / 60) * res.get_tas();
							if (false)
								std::cerr << "OpsPerf: alt " << alt << " altd " << altd << " ROCD " << res.get_rocd()
									  << " TAS " << res.get_tas() << std::endl;
						}
					}
					m_performance.set(pi0, pi1, Performance::LevelChange(tracknmi, timepenalty, fuelpenalty, metricpenalty, tracknmiop));
					continue;
				}
				// climb
				double t0(climb.altitude_to_time(cruise0.get_altitude()));
				double t1(climb.altitude_to_time(cruise1.get_altitude()));
				double tracknmi(climb.time_to_distance(t1) - climb.time_to_distance(t0));
				double timepenalty(-tracknmi * cruise1.get_secpernmi());
				double fuelpenalty(timepenalty * cruise1.get_fuelpersec());
				timepenalty += t1 - t0;
				fuelpenalty += climb.time_to_fuel(t1) - climb.time_to_fuel(t0);
				if (std::isnan(timepenalty) || timepenalty < 0)
					timepenalty = 0;
				if (std::isnan(fuelpenalty) || fuelpenalty < 0)
					fuelpenalty = 0;
				double metricpenalty(0);
				if (is_optpreferred())
					metricpenalty = (pi1 - pi0) * m_preferredclimb;
				else if (is_optfuel())
					metricpenalty = fuelpenalty;
				else
					metricpenalty = timepenalty;
				if (std::isnan(metricpenalty))
					metricpenalty = 0;
				if (!m_honourlevelchangetrackmiles)
					tracknmi = 0;
				// opsperf model
				double tracknmiop(0);
				if (m_honouropsperftrackmiles && opsperfacft.is_valid()) {
					for (int alt = cruise0.get_altitude(); alt < cruise1.get_altitude(); ) {
						int altd = cruise1.get_altitude() - alt;
						if (altd > 500)
							altd = 500;
						OperationsPerformance::Aircraft::ComputeResult res(opsperfacft.get_massref());
						res.set_qnh_temp(); // ISA
						res.set_pressure_altitude(alt);
						alt += altd;
						if (!opsperfacft.compute(res, OperationsPerformance::Aircraft::compute_climb) ||
						    !(res.get_rocd() > 0))
							continue;
						tracknmiop += (altd / res.get_rocd()) * (1.0 / 60) * res.get_tas();
						if (false)
							std::cerr << "OpsPerf: alt " << alt << " altd " << altd << " ROCD " << res.get_rocd()
								  << " TAS " << res.get_tas() << std::endl;
					}
				}
				m_performance.set(pi0, pi1, Performance::LevelChange(tracknmi, timepenalty, fuelpenalty, metricpenalty, tracknmiop));
			}
		}
		// climb from Departure to Enroute Level
		{
			ad.set_true_altitude(get_departure().get_elevation());
			double t0(climb.altitude_to_time(ad.get_pressure_altitude()));
			for (unsigned int pi = 0; pi < pis; ++pi) {
				const Performance::Cruise& cruise(m_performance.get_cruise(pi));
				if (cruise.get_altitude() < get_departure().get_elevation()) {
					double nan(std::numeric_limits<double>::quiet_NaN());
					m_performance.set(pis, pi, Performance::LevelChange(nan, nan, nan, nan));
					continue;
				}
				double t1(climb.altitude_to_time(cruise.get_altitude()));
				double tracknmi(climb.time_to_distance(t1) - climb.time_to_distance(t0));
				double timepenalty(-tracknmi * cruise.get_secpernmi());
				double fuelpenalty(timepenalty * cruise.get_fuelpersec());
				timepenalty += t1 - t0;
				fuelpenalty += climb.time_to_fuel(t1) - climb.time_to_fuel(t0);
				if (std::isnan(timepenalty) || timepenalty < 0)
					timepenalty = 0;
				if (std::isnan(fuelpenalty) || fuelpenalty < 0)
					fuelpenalty = 0;
				double metricpenalty(0);
				if (is_optpreferred())
					metricpenalty = pi * m_preferredclimb;
				else if (is_optfuel())
					metricpenalty = fuelpenalty;
				else
					metricpenalty = timepenalty;
				if (std::isnan(metricpenalty))
					metricpenalty = 0;
				if (!m_honourlevelchangetrackmiles)
					tracknmi = 0;
				// opsperf model
				double tracknmiop(0);
				if (m_honouropsperftrackmiles && opsperfacft.is_valid()) {
					for (int alt = get_departure().get_elevation() + 1000; alt < cruise.get_altitude(); ) {
						int altd = cruise.get_altitude() - alt;
						if (altd > 500)
							altd = 500;
						OperationsPerformance::Aircraft::ComputeResult res(opsperfacft.get_massref());
						res.set_qnh_temp(); // ISA
						res.set_pressure_altitude(alt);
						alt += altd;
						if (!opsperfacft.compute(res, OperationsPerformance::Aircraft::compute_climb) ||
						    !(res.get_rocd() > 0))
							continue;
						tracknmiop += (altd / res.get_rocd()) * (1.0 / 60) * res.get_tas();
						if (false)
							std::cerr << "OpsPerf: alt " << alt << " altd " << altd << " ROCD " << res.get_rocd()
								  << " TAS " << res.get_tas() << std::endl;
					}
				}
				m_performance.set(pis, pi, Performance::LevelChange(tracknmi, timepenalty, fuelpenalty, metricpenalty, tracknmiop));
			}
		}
		// descent from Enroute Level to Destination
		{
			ad.set_true_altitude(get_destination().get_elevation());
			double t0(descent.altitude_to_time(ad.get_pressure_altitude()));
			for (unsigned int pi = 0; pi < pis; ++pi) {
				const Performance::Cruise& cruise(m_performance.get_cruise(pi));
				if (cruise.get_altitude() < get_destination().get_elevation()) {
					double nan(std::numeric_limits<double>::quiet_NaN());
					m_performance.set(pi, pis, Performance::LevelChange(nan, nan, nan, nan));
					continue;
				}
				double t1(descent.altitude_to_time(cruise.get_altitude()));
				double tracknmi(descent.time_to_distance(t1) - descent.time_to_distance(t0));
				double timepenalty(-tracknmi * cruise.get_secpernmi());
				double fuelpenalty(timepenalty * cruise.get_fuelpersec());
				timepenalty += t1 - t0;
				fuelpenalty += descent.time_to_fuel(t1) - descent.time_to_fuel(t0);
				if (std::isnan(timepenalty) || timepenalty < 0)
					timepenalty = 0;
				if (std::isnan(fuelpenalty) || fuelpenalty < 0)
					fuelpenalty = 0;
				double metricpenalty(0);
				if (is_optpreferred())
					metricpenalty = -pi * m_preferreddescent;
				else if (is_optfuel())
					metricpenalty = fuelpenalty;
				else
					metricpenalty = timepenalty;
				if (std::isnan(metricpenalty))
					metricpenalty = 0;
				if (!m_honourlevelchangetrackmiles)
					tracknmi = 0;
				// opsperf model
				double tracknmiop(0);
				if (m_honouropsperftrackmiles && opsperfacft.is_valid()) {
					for (int alt = cruise.get_altitude(); alt > get_destination().get_elevation() + 1000; ) {
						int altd = alt - get_destination().get_elevation() - 1000;
						if (altd > 500)
							altd = 500;
						OperationsPerformance::Aircraft::ComputeResult res(opsperfacft.get_massref());
						res.set_qnh_temp(); // ISA
						res.set_pressure_altitude(alt);
						alt -= altd;
						if (!opsperfacft.compute(res, OperationsPerformance::Aircraft::compute_descent) ||
						    !(res.get_rocd() < 0))
							continue;
						tracknmiop += (altd / -res.get_rocd()) * (1.0 / 60) * res.get_tas();
						if (false)
							std::cerr << "OpsPerf: alt " << alt << " altd " << altd << " ROCD " << res.get_rocd()
								  << " TAS " << res.get_tas() << std::endl;
					}
				}
				m_performance.set(pi, pis, Performance::LevelChange(tracknmi, timepenalty, fuelpenalty, metricpenalty, tracknmiop));
			}
		}
	}
	{
		Aircraft::Cruise::EngineParams ep(get_engine_bhp(), get_engine_rpm(), get_engine_mp());
		std::ostringstream oss;
		oss << "Cruise Engine Parameters:";
		if (!ep.get_name().empty())
			oss << " Name " << ep.get_name();
		if (!std::isnan(ep.get_bhp()))
			oss << " BHP " << ep.get_bhp();
		if (!std::isnan(ep.get_rpm()))
			oss << " RPM " << ep.get_rpm();
		if (!std::isnan(ep.get_mp()))
			oss << " MP " << ep.get_mp();
		if (ep.get_flags() & Aircraft::Cruise::Curve::flags_interpolate)
			oss << " interpolate";
		if (ep.get_flags() & Aircraft::Cruise::Curve::flags_hold)
			oss << " hold";
		m_signal_log(log_normal, oss.str());
	}
	// print metric tables
	{
		m_signal_log(log_normal, "Cruise Table");
		{
			std::ostringstream oss;
			oss << "          ";
			for (unsigned int pi = 0; pi < pis; ++pi)
				oss << std::setw(8) << Glib::ustring::format("FL", (m_performance.get_cruise(pi).get_altitude() + 50) / 100);
			oss << std::setw(8) << "Dest";
			m_signal_log(log_normal, oss.str());
		}
		{
			std::ostringstream oss;
			oss << "DA        ";
			for (unsigned int pi = 0; pi < pis; ++pi)
				oss << std::setw(8) << Glib::ustring::format(std::fixed, std::setprecision(0), m_performance.get_cruise(pi).get_densityaltitude());
			m_signal_log(log_normal, oss.str());
		}
		{
			std::ostringstream oss;
			oss << "TAS       ";
			for (unsigned int pi = 0; pi < pis; ++pi)
				oss << std::setw(8) << Glib::ustring::format(std::fixed, std::setprecision(0), m_performance.get_cruise(pi).get_tas());
			m_signal_log(log_normal, oss.str());
		}
		{
			std::ostringstream oss;
			oss << "s/nmi     ";
			for (unsigned int pi = 0; pi < pis; ++pi)
				oss << std::setw(8) << Glib::ustring::format(std::fixed, std::setprecision(3), m_performance.get_cruise(pi).get_secpernmi());
			m_signal_log(log_normal, oss.str());
		}
		{
			std::ostringstream oss;
			oss << "Fuel/s    ";
			for (unsigned int pi = 0; pi < pis; ++pi)
				oss << std::setw(8) << Glib::ustring::format(std::fixed, std::setprecision(5), m_performance.get_cruise(pi).get_fuelpersec());
			m_signal_log(log_normal, oss.str());
		}
		{
			std::ostringstream oss;
			oss << "Metric    ";
			for (unsigned int pi = 0; pi < pis; ++pi)
				oss << std::setw(8) << Glib::ustring::format(std::fixed, std::setprecision(3), m_performance.get_cruise(pi).get_metricpernmi());
			m_signal_log(log_normal, oss.str());
		}
		if (m_windenable) {
			std::ostringstream ossdepw, ossdestw, ossdept, ossdestt;
			ossdepw << "Dep Wind  ";
			ossdestw << "Dest Wind ";
			ossdept << "Dep ISA   ";
			ossdestt << "Dest ISA  ";
			for (unsigned int pi = 0; pi < pis; ++pi) {
				if (m_performance.get_cruise(pi).is_temp()) {
					float isatemp;
					IcaoAtmosphere<float>::std_altitude_to_pressure(0, &isatemp, m_performance.get_cruise(pi).get_altitude() * Point::ft_to_m);
					ossdept << std::setw(8)
						<< Glib::ustring::format(std::fixed, std::setprecision(1),
									 m_performance.get_cruise(pi).get_temp(get_departure().get_coord()) - isatemp);
					ossdestt << std::setw(8)
						 << Glib::ustring::format(std::fixed, std::setprecision(1),
									  m_performance.get_cruise(pi).get_temp(get_destination().get_coord()) - isatemp);
				} else {
					ossdept << std::setw(8) << "---";
					ossdestt << std::setw(8) << "---";
				}
				if (!m_performance.get_cruise(pi).is_wind()) {
					ossdepw << std::setw(8) << "---";
					ossdestw << std::setw(8) << "---";
					continue;
				}
				Wind<double> wdep(m_performance.get_cruise(pi).get_wind(get_departure().get_coord()));
				Wind<double> wdest(m_performance.get_cruise(pi).get_wind(get_destination().get_coord()));
				ossdepw << std::setw(8)
					<< (Glib::ustring::format(std::setw(3), std::setfill(L'0'), Point::round<int,double>(wdep.get_winddir())) +
					    '/' + Glib::ustring::format(std::setw(2), std::setfill(L'0'), Point::round<int,double>(wdep.get_windspeed())));
				ossdestw << std::setw(8)
					 << (Glib::ustring::format(std::setw(3), std::setfill(L'0'), Point::round<int,double>(wdest.get_winddir())) +
					     '/' + Glib::ustring::format(std::setw(2), std::setfill(L'0'), Point::round<int,double>(wdest.get_windspeed())));
			}
			m_signal_log(log_normal, ossdepw.str());
			m_signal_log(log_normal, ossdept.str());
			m_signal_log(log_normal, ossdestw.str());
			m_signal_log(log_normal, ossdestt.str());
		}
		m_signal_log(log_normal, "Level Change Table");
		for (unsigned int pi0 = 0; pi0 <= pis; ++pi0) {
			{
				std::ostringstream oss;
				if (pi0 >= pis)
					oss << "Dep       ";
				else
					oss << std::setw(10) << std::left << Glib::ustring::format("FL", (m_performance.get_cruise(pi0).get_altitude() + 50) / 100) << std::right;
				for (unsigned int pi1 = 0; pi1 <= pis; ++pi1)
					oss << std::setw(8) << Glib::ustring::format(std::fixed, std::setprecision(3), m_performance.get_levelchange(pi0, pi1).get_metricpenalty());
				m_signal_log(log_normal, oss.str());
			}
			{
				std::ostringstream oss;
				oss << "          ";
				for (unsigned int pi1 = 0; pi1 <= pis; ++pi1)
					oss << std::setw(8) << Glib::ustring::format(std::fixed, std::setprecision(1), m_performance.get_levelchange(pi0, pi1).get_tracknmi());
				m_signal_log(log_normal, oss.str());
			}
			{
				std::ostringstream oss;
				oss << "          ";
				for (unsigned int pi1 = 0; pi1 <= pis; ++pi1)
					oss << std::setw(8) << Glib::ustring::format(std::fixed, std::setprecision(3), m_performance.get_levelchange(pi0, pi1).get_timepenalty());
				m_signal_log(log_normal, oss.str());
			}
			{
				std::ostringstream oss;
				oss << "          ";
				for (unsigned int pi1 = 0; pi1 <= pis; ++pi1)
					oss << std::setw(8) << Glib::ustring::format(std::fixed, std::setprecision(3), m_performance.get_levelchange(pi0, pi1).get_fuelpenalty());
				m_signal_log(log_normal, oss.str());
			}
			{
				std::ostringstream oss;
				oss << "          ";
				for (unsigned int pi1 = 0; pi1 <= pis; ++pi1)
					oss << std::setw(8) << Glib::ustring::format(std::fixed, std::setprecision(1), m_performance.get_levelchange(pi0, pi1).get_opsperftracknmi());
				m_signal_log(log_normal, oss.str());
			}
		}
	}
}

bool CFMUAutoroute::start_vfr(void)
{
	VFRRouter r(*this);
	if (!r.load_graph(get_bbox())) {
		std::ostringstream oss;
		oss << "VFR router: cannot load routing graph for bbox " << get_bbox();
		m_signal_log(log_debug0, oss.str());
		return false;
	}
	{
		std::ostringstream oss;
		oss << "VFR area graph after loading: " << boost::num_vertices(r.get_graph()) << " vertices, "
		    << boost::num_edges(r.get_graph()) << " edges";
		m_signal_log(log_debug0, oss.str());
	}
	if (!r.set_departure(get_departure())) {
		std::ostringstream oss;
		oss << "VFR router: cannot find departure airport " << get_departure().get_icao_name();
		m_signal_log(log_debug0, oss.str());
		return false;
	}
	if (!r.set_destination(get_destination())) {
		std::ostringstream oss;
		oss << "VFR router: cannot find destination airport " << get_destination().get_icao_name();
		m_signal_log(log_debug0, oss.str());
		return false;
	}
	bool havesid(r.add_sid(get_sid()));
	if (!havesid && !get_sid().is_invalid()) {
		std::ostringstream oss;
		oss << "VFR router: cannot find SID point " << get_sid().get_lat_str2() << ' ' << get_sid().get_lon_str2();
		m_signal_log(log_debug0, oss.str());
		return false;
	}
	havesid = havesid || r.add_sid(get_departure());
	bool havestar(r.add_star(get_star()));
	if (!havestar && !get_star().is_invalid()) {
		std::ostringstream oss;
		oss << "VFR router: cannot find STAR point " << get_star().get_lat_str2() << ' ' << get_star().get_lon_str2();
		m_signal_log(log_debug0, oss.str());
		return false;
	}
	havestar = havestar || r.add_star(get_destination());
	{
		std::ostringstream oss;
		oss << "VFR area graph after SID/STAR: " << boost::num_vertices(r.get_graph()) << " vertices, "
		    << boost::num_edges(r.get_graph()) << " edges";
		m_signal_log(log_debug0, oss.str());
	}
	r.add_dct(get_dctlimit(),
		  havesid ? std::numeric_limits<double>::quiet_NaN() : get_sidlimit(),
		  havestar ? std::numeric_limits<double>::quiet_NaN() : get_starlimit());
	{
		std::ostringstream oss;
		oss << "VFR area graph after DCT: " << boost::num_vertices(r.get_graph()) << " vertices, "
		    << boost::num_edges(r.get_graph()) << " edges";
		m_signal_log(log_debug0, oss.str());
	}
	r.set_metric();
	r.exclude_airspace(m_levels[0], m_levels[1], get_vfraspclimit());
	r.exclude_regions();
	if (!r.autoroute(m_route)) {
		std::ostringstream oss;
		oss << "VFR router: cannot find route " << get_departure().get_icao_name()
		    << "->" << get_destination().get_icao_name();
		m_signal_log(log_debug0, oss.str());
		return false;
	}
	{
		const Performance::Cruise& cruise(m_performance.get_cruise(m_performance.size() / 2U));
		for (unsigned int i = 0; i < m_route.get_nrwpt(); ++i) {
			FPlanWaypoint& wpt(m_route[i]);
			uint16_t flags(wpt.get_flags());
			flags &= ~FPlanWaypoint::altflag_ifr;
			if (i > 0 && i + 1 < m_route.get_nrwpt()) {
				flags |= FPlanWaypoint::altflag_standard;
				wpt.set_altitude(cruise.get_altitude());
			}
			wpt.set_flags(flags);
		}
	}
	updatefpl();
	return true;
}

void CFMUAutoroute::stop(statusmask_t sm)
{
	m_childtimeoutcount = 0;
	m_connchildtimeout.disconnect();
	m_pathprobe = -1;
	if (m_running) {
		{
			Glib::TimeVal tv;
			tv.assign_current_time();
			tv -= m_tvelapsed;
			m_tvelapsed = tv;
		}
		m_running = false;
		if (m_done) {
			m_signal_statuschange(statusmask_stoppingdone);
		} else {
			sm &= statusmask_stoppingerror;
			if (!sm)
				sm |= statusmask_stoppingerrorinternalerror;
			m_signal_statuschange(sm);
		}
	}
}

void CFMUAutoroute::clear(void)
{
	stop(statusmask_stoppingerroruser);
	while (m_route.get_nrwpt())
		m_route.delete_wpt(0);
	m_routetime = 0;
	m_routefuel = 0;
	m_validationresponse.clear();
	m_done = false;
}

bool CFMUAutoroute::is_stopped(void)
{
	// dispatch events, check for stop command
	if (true) {
		Glib::RefPtr<Glib::MainContext> ctx(Glib::MainContext::get_default());
		while (ctx->iteration(false));
	}
	if (m_running)
		return false;
	m_signal_log(log_debug0, "Interrupted by user, stopping...");
	return true;
}

bool CFMUAutoroute::child_is_running(void) const
{
	return m_childrun || m_childsock;
}

void CFMUAutoroute::child_run(void)
{
	if (child_is_running())
		return;
	m_childstdout.clear();
	m_signal_log(log_normal, "Connecting to validation server...");
	if (m_childsockaddr) {
		m_childsock = Gio::Socket::create(m_childsockaddr->get_family(), Gio::SOCKET_TYPE_STREAM, Gio::SOCKET_PROTOCOL_DEFAULT);
		try {
			m_childsock->connect(m_childsockaddr);
		} catch (...) {
			m_childsock.reset();
		}
		if (m_childsock) {
			m_connchildstdout = Glib::signal_io().connect(sigc::mem_fun(this, &CFMUAutoroute::child_socket_handler), m_childsock->get_fd(), Glib::IO_IN | Glib::IO_ERR | Glib::IO_HUP);
			child_configure();
			return;
		}
	}
	if (m_childbinary.empty()) {
		m_childrun = false;
		m_signal_log(log_normal, "Cannot run validator: no validator configured");
		return;
	}
	int childstdin(-1), childstdout(-1);
	try {
		std::vector<std::string> argv, env;
#ifdef __MINGW32__
		std::string workdir;
		{
			gchar *instdir(g_win32_get_package_installation_directory_of_module(0));
			if (instdir)
				workdir = instdir;
			else
				workdir = PACKAGE_PREFIX_DIR;
		}
#else
		std::string workdir(PACKAGE_DATA_DIR);
		env.push_back("PATH=/bin:/usr/bin");
		if (m_childxdisplay >= 0) {
			std::ostringstream oss;
			oss << "DISPLAY=:" << m_childxdisplay;
			env.push_back(oss.str());
		} else {
			bool found(false);
			std::string disp(Glib::getenv("DISPLAY", found));
			if (found)
				env.push_back("DISPLAY=" + disp);
		}
#endif
		argv.push_back(m_childbinary);
		if (m_childxdisplay >= 0) {
			std::ostringstream oss;
			oss << "--xdisplay=" << m_childxdisplay;
			argv.push_back(oss.str());
		}
		if (false) {
			std::cerr << "Spawn: workdir " << workdir << " argv:" << std::endl;
			for (std::vector<std::string>::const_iterator i(argv.begin()), e(argv.end()); i != e; ++i)
				std::cerr << "  " << *i << std::endl;
			std::cerr << "env:" << std::endl;
			for (std::vector<std::string>::const_iterator i(env.begin()), e(env.end()); i != e; ++i)
				std::cerr << "  " << *i << std::endl;
		}
#ifdef __MINGW32__
		// inherit environment; otherwise, this may fail on WindowsXP with STATUS_SXS_ASSEMBLY_NOT_FOUND (C0150004)
		Glib::spawn_async_with_pipes(workdir, argv,
					     Glib::SPAWN_DO_NOT_REAP_CHILD | Glib::SPAWN_STDERR_TO_DEV_NULL,
					     sigc::slot<void>(), &m_childpid, &childstdin, &childstdout, 0);
#else
		Glib::spawn_async_with_pipes(workdir, argv, env,
					     Glib::SPAWN_DO_NOT_REAP_CHILD | Glib::SPAWN_STDERR_TO_DEV_NULL,
					     sigc::slot<void>(), &m_childpid, &childstdin, &childstdout, 0);
#endif
		m_childrun = true;
	} catch (Glib::SpawnError& e) {
		m_childrun = false;
		m_signal_log(log_normal, "Cannot run validator: " + e.what());
		return;
	}
	m_connchildwatch = Glib::signal_child_watch().connect(sigc::mem_fun(this, &CFMUAutoroute::child_watch), m_childpid);
	m_childchanstdin = Glib::IOChannel::create_from_fd(childstdin);
	m_childchanstdout = Glib::IOChannel::create_from_fd(childstdout);
	m_childchanstdin->set_encoding();
	m_childchanstdout->set_encoding();
	m_childchanstdin->set_close_on_unref(true);
	m_childchanstdout->set_close_on_unref(true);
	m_connchildstdout = Glib::signal_io().connect(sigc::mem_fun(this, &CFMUAutoroute::child_stdout_handler), m_childchanstdout,
						      Glib::IO_IN | Glib::IO_ERR | Glib::IO_HUP);
	child_configure();
}

bool CFMUAutoroute::child_stdout_handler(Glib::IOCondition iocond)
{
        Glib::ustring line;
        Glib::IOStatus iostat(m_childchanstdout->read_line(line));
        if (iostat == Glib::IO_STATUS_ERROR ||
            iostat == Glib::IO_STATUS_EOF) {
		if (iostat == Glib::IO_STATUS_ERROR)
			m_signal_log(log_debug0, "Child stdout error, stopping...");
		else
			m_signal_log(log_debug0, "Child stdout eof, stopping...");
                child_close();
		stop(statusmask_stoppingerrorvalidatortimeout);
		return true;
        }
	if (false)
		std::cerr << "validator rx: " << line << std::endl;
	{
		std::string::size_type n(line.size());
		while (n > 0) {
			--n;
			if (!isspace(line[n]) && line[n] != '\n' && line[n] != '\r') {
				++n;
				break;
			}
		}
		line = std::string(line, 0, n);
	}
	if (!line.empty())
		m_validationresponse.push_back(line);
	m_signal_log(log_fplremotevalidation, line);
	if (line.empty()) {
		if (false)
			std::cerr << "validator: response finished" << std::endl;
		{
			Glib::TimeVal tv;
			tv.assign_current_time();
			tv -= m_tvvalidatestart;
			m_tvvalidator += tv;
		}
		parse_response();
	}
        return true;
}

bool CFMUAutoroute::child_socket_handler(Glib::IOCondition iocond)
{
	{
		char buf[4096];
		gssize r(m_childsock->receive_with_blocking(buf, sizeof(buf), false));
		if (r < 0) {
			m_signal_log(log_debug0, "Child stdout error, stopping...");
			child_close();
			stop(statusmask_stoppingerrorvalidatortimeout);
			return true;
		}
		if (!r) {
			child_close();
			child_timeout();
			return true;
		}
		m_childstdout += std::string(buf, buf + r);
		if (false)
			std::cerr << "validator rx: " << std::string(buf, buf + r) << std::endl;
	}
	for (;;) {
		std::string::size_type n(m_childstdout.find_first_of("\r\n"));
		if (n == std::string::npos)
			break;
		std::string line(m_childstdout, 0, n);
		{
			uint8_t flg(0);
			while (n < m_childstdout.size()) {
				bool quit(false);
				switch (m_childstdout[n]) {
				case '\r':
					quit = quit || (flg & 1);
					flg |= 1;
					break;

				case '\n':
					quit = quit || (flg & 2);
					flg |= 2;
					break;

				default:
					quit = true;
					break;
				}
				if (quit)
					break;
				++n;
			}
		}
		m_childstdout.erase(0, n);
		if (!line.empty())
			m_validationresponse.push_back(line);
		m_signal_log(log_fplremotevalidation, line);
		if (line.empty()) {
			if (false)
				std::cerr << "validator: response finished" << std::endl;
			{
				Glib::TimeVal tv;
				tv.assign_current_time();
				tv -= m_tvvalidatestart;
				m_tvvalidator += tv;
			}
			parse_response();
		}
	}
	return true;
}

void CFMUAutoroute::child_watch(GPid pid, int child_status)
{
	if (!m_childrun)
		return;
	if (m_childpid != pid) {
		Glib::spawn_close_pid(pid);
		return;
	}
	Glib::spawn_close_pid(pid);
	m_childrun = false;
	m_connchildwatch.disconnect();
	m_connchildstdout.disconnect();
	child_close();
	child_timeout();
}

void CFMUAutoroute::child_close(void)
{
	m_connchildstdout.disconnect();
	m_childstdout.clear();
	if (m_childsock) {
		m_childsock->close();
		m_childsock.reset();
	}
	if (m_childchanstdin) {
		m_childchanstdin->close();
		m_childchanstdin.reset();
	}
	if (m_childchanstdout) {
		m_childchanstdout->close();
		m_childchanstdout.reset();
	}
	if (!m_childrun)
		return;
	m_childrun = false;
	m_connchildwatch.disconnect();
	Glib::signal_child_watch().connect(sigc::hide(sigc::ptr_fun(&Glib::spawn_close_pid)), m_childpid);
}

void CFMUAutoroute::child_send(const std::string& tx)
{
	if (false)
		std::cerr << "validator tx: " << tx << std::endl;
	if (m_childsock) {
		gssize r(m_childsock->send_with_blocking(const_cast<char *>(tx.c_str()), tx.size(), true));
		if (r == -1) {
			child_close();
			child_timeout();
		}
	} else if (m_childchanstdin) {
		m_childchanstdin->write(tx);
		m_childchanstdin->flush();
	}
}

void CFMUAutoroute::child_configure(void)
{
	switch (m_validator) {
	case validator_cfmu:
		child_send("validate*:cfmu\n");
		break;

	case validator_eurofpl:
		child_send("validate*:eurofpl\n");
		break;

	default:
		break;
	}
}

bool CFMUAutoroute::child_timeout(void)
{
	m_connchildtimeout.disconnect();
	if (is_stopped())
		return false;
	++m_childtimeoutcount;
	if (m_childtimeoutcount > 5) {
		m_signal_log(log_debug0, "Child timed out, stopping...");
		stop(statusmask_stoppingerrorvalidatortimeout);
		child_close();
		return false;
	}
	child_close();
	child_run();
	if (!child_is_running()) {
		m_signal_log(log_debug0, "Cannot restart child after timeout, stopping...");
		stop(statusmask_stoppingerrorvalidatortimeout);
		return false;
	}		
	child_send(get_simplefpl() + "\n");
	m_connchildtimeout = Glib::signal_timeout().connect_seconds(sigc::mem_fun(*this, &CFMUAutoroute::child_timeout), child_get_timeout());
	return true;
}

Rect CFMUAutoroute::get_bbox(void) const
{
	Rect bbox;
	unsigned int cnt(0);
	if (get_departure().is_valid()) {
		bbox = Rect(get_departure().get_coord(), get_departure().get_coord());
		cnt = 1;
	}
	if (get_destination().is_valid()) {
		if (cnt)
			bbox = bbox.add(get_destination().get_coord());
		else
			bbox = Rect(get_destination().get_coord(), get_destination().get_coord());
		++cnt;
	}
	for (crossing_t::const_iterator ci(m_crossing.begin()), ce(m_crossing.end()); ci != ce; ++ci) {
		if (ci->get_coord().is_invalid())
			continue;
		if (std::isnan(ci->get_radius()))
			continue;
		for (unsigned int i = 0; i < 360; i += 90) {
			Point pt(ci->get_coord().spheric_course_distance_nmi(i, ci->get_radius()));
			if (cnt)
				bbox = bbox.add(pt);
			else
				bbox = Rect(pt, pt);
			++cnt;
		}
	}
	return bbox;
}

CFMUAutoroute::VFRRouter::VFRRouter(CFMUAutoroute& ar)
	: m_ar(ar)
{
}

bool CFMUAutoroute::VFRRouter::load_graph(const Rect& bbox)
{
	m_bbox = bbox.oversize_nmi(100.f);
	if (false) {
		std::ostringstream oss;
		oss << "Loading VFR area graph: " << m_bbox;
		m_ar.m_signal_log(log_debug0, oss.str());
	}
	m_ar.opendb();
	m_graph.clear();
	m_graph.add(m_ar.m_airportdb.find_by_rect(m_bbox, ~0, AirportsDb::Airport::subtables_none));
	m_graph.add(m_ar.m_navaiddb.find_by_rect(m_bbox, ~0, NavaidsDb::Navaid::subtables_none));
	m_graph.add(m_ar.m_waypointdb.find_by_rect(m_bbox, ~0, WaypointsDb::Waypoint::subtables_none));
	m_graph.add(m_ar.m_mapelementdb.find_by_rect(m_bbox, ~0, MapelementsDb::Mapelement::subtables_none));
	// We do not need airways for VFR routing
	//m_graph.add(m_ar.m_airwaydb.find_by_area(m_bbox, ~0, AirwaysDb::Airway::subtables_all));
	//m_graph.add(m_ar.m_airwaydb.find_by_rect(m_bbox, ~0, AirwaysDb::Airway::subtables_all));
	if (true) {
		if (false) {
			std::ostringstream oss;
			oss << "VFR area graph: " << m_bbox << ", "
			    << boost::num_vertices(m_graph) << " vertices, " << boost::num_edges(m_graph) << " edges";
			m_ar.m_signal_log(log_debug0, oss.str());
		}
		unsigned int counts[4];
		for (unsigned int i = 0; i < sizeof(counts)/sizeof(counts[0]); ++i)
			counts[i] = 0;
		Graph::vertex_iterator u0, u1;
		for (boost::tie(u0, u1) = boost::vertices(m_graph); u0 != u1; ++u0) {
			const Vertex& uu(m_graph[*u0]);
			unsigned int i(uu.get_type());
			if (i < sizeof(counts)/sizeof(counts[0]))
				++counts[i];
		}
		{
			std::ostringstream oss;
			oss << "VFR area graph: " << counts[Vertex::type_airport] << " airports, " << counts[Vertex::type_navaid] << " navaids, "
			    << counts[Vertex::type_intersection] << " intersections, " << counts[Vertex::type_mapelement] << " mapelements";
			m_ar.m_signal_log(log_debug0, oss.str());
		}
	}
	return true;
}

CFMUAutoroute::VFRRouter::Graph::vertex_descriptor CFMUAutoroute::VFRRouter::find_airport(const AirportsDb::Airport& arpt)
{
	if (!arpt.is_valid())
		return boost::num_vertices(m_graph);
	Graph::vertex_iterator u0, u1;
	for (boost::tie(u0, u1) = boost::vertices(m_graph); u0 != u1; ++u0) {
		const Vertex& uu(m_graph[*u0]);
		AirportsDb::Airport a;
		if (!uu.get(a))
			continue;
		if (!a.is_valid())
			continue;
		if (arpt.get_icao() != a.get_icao() ||
		    arpt.get_name() != a.get_name() ||
		    arpt.get_coord() != a.get_coord())
			continue;
		return *u0;
	}
	return boost::num_vertices(m_graph);
}

bool CFMUAutoroute::VFRRouter::set_departure(const AirportsDb::Airport& arpt)
{
	m_dep = find_airport(arpt);
	return m_dep < boost::num_vertices(m_graph);
}

bool CFMUAutoroute::VFRRouter::set_destination(const AirportsDb::Airport& arpt)
{
	m_dest = find_airport(arpt);
	return m_dest < boost::num_vertices(m_graph);
}

void CFMUAutoroute::VFRRouter::add_dct(double dctlim, double sidlim, double starlim)
{
	Graph::vertex_iterator u0, u1;
	for (boost::tie(u0, u1) = boost::vertices(m_graph); u0 != u1; ++u0) {
		Graph::vertex_descriptor u(*u0);
		const Vertex& uu(m_graph[u]);
		Graph::vertex_iterator v0, v1;
		for (boost::tie(v0, v1) = boost::vertices(m_graph); v0 != v1; ++v0) {
			Graph::vertex_descriptor v(*v0);
			if (u == v)
				continue;
			const Vertex& vv(m_graph[*v0]);
			bool kill(u == m_dep && v == m_dest);
			kill = kill || (uu.get_type() == Vertex::type_intersection && uu.is_ident_numeric());
			kill = kill || (vv.get_type() == Vertex::type_intersection && vv.is_ident_numeric());
			double dist, lim(dctlim);
			if (!kill) {
				dist = uu.get_coord().spheric_distance_nmi_dbl(vv.get_coord());
				if (u == m_dep)
					lim = sidlim;
				if (v == m_dest)
					lim = starlim;
				if (!std::isnan(lim))
					kill = dist > lim;
			}
			if (kill) {
				for (;;) {
					bool done(true);
					Graph::out_edge_iterator e0, e1;
					for (boost::tie(e0, e1) = boost::out_edges(u, m_graph); e0 != e1;) {
						Graph::out_edge_iterator ex(e0);
						++e0;
						if (boost::target(*ex, m_graph) != v)
							continue;
						boost::remove_edge(ex, m_graph);
						if (lgraphrestartedgescan) {
							done = false;
							break;
						}
					}
					if (done)
						break;
				}
				continue;
			}
			if (std::isnan(lim))
				continue;
			Graph::edge_descriptor e;
			bool hase;
			tie(e, hase) = boost::edge(u, v, m_graph);
			if (hase)
				continue;
			boost::add_edge(u, v, Edge("", 0, 600, AirwaysDb::Airway::airway_invalid, dist), m_graph);
		}
	}
}

void CFMUAutoroute::VFRRouter::remove_sid(void)
{
	for (;;) {
		bool done(true);
		Graph::out_edge_iterator e0, e1;
		for (boost::tie(e0, e1) = boost::out_edges(m_dep, m_graph); e0 != e1;) {
			Graph::out_edge_iterator ex(e0);
			++e0;
			boost::remove_edge(ex, m_graph);
			if (lgraphrestartedgescan) {
				done = false;
				break;
			}
		}
		if (done)
			break;
	}
}

bool CFMUAutoroute::VFRRouter::add_sid(const Point& pt)
{
	if (pt.is_invalid())
		return false;
	Graph::vertex_descriptor v;
	if (!m_graph.find_nearest(v, pt, Vertex::typemask_all))
		return false;
	if (v == m_dest)
		return false;
	const Vertex& vv(m_graph[v]);
	double dist(pt.spheric_distance_nmi_dbl(vv.get_coord()));
	if (dist > 0.1)
		return false;
	m_ar.m_airportconnident[0] = vv.get_ident();
	m_ar.m_airportconntype[0] = vv.get_type();
	// remove departure out edges
	remove_sid();
	// add edge
	boost::add_edge(m_dep, v, Edge("", 0, 600, AirwaysDb::Airway::airway_invalid, dist), m_graph);
	m_sid.clear();
	return true;
}

bool CFMUAutoroute::VFRRouter::add_sid(const AirportsDb::Airport& arpt)
{
	bool added(false);
	m_sid.clear();
	for (unsigned int i = 0, j = arpt.get_nrvfrrte(); i < j; ++i) {
		const AirportsDb::Airport::VFRRoute& rte(arpt.get_vfrrte(i));
		if (!rte.size() ||
		    rte.get_name().empty())
			continue;
		const AirportsDb::Airport::VFRRoute::VFRRoutePoint& rtept(rte[rte.size()-1]);
		if (rtept.get_pathcode() != AirportsDb::Airport::VFRRoute::VFRRoutePoint::pathcode_departure)
			continue;
		Graph::vertex_descriptor v;
		{
			bool found(m_graph.find_nearest(v, rtept.get_coord(), Vertex::typemask_all));
			if (found) {
				const Vertex& vv(m_graph[v]);
				double dist(rtept.get_coord().spheric_distance_nmi_dbl(vv.get_coord()));
				if (dist > 0.1)
					found = false;
			}
			if (!found)
				v = boost::add_vertex(Vertex(rte.get_name(), rtept.get_coord(), Vertex::type_vfrreportingpt), m_graph);
		}
		if (!added)
			remove_sid();
		added = true;
		boost::add_edge(m_dep, v, Edge("", 0, 600, AirwaysDb::Airway::airway_invalid, rte.compute_distance()), m_graph);
		m_sid.push_back(rte);
	}
	return added;
}

void CFMUAutoroute::VFRRouter::remove_star(void)
{
	for (;;) {
		bool done(true);
		Graph::in_edge_iterator e0, e1;
		for (boost::tie(e0, e1) = boost::in_edges(m_dest, m_graph); e0 != e1;) {
			Graph::in_edge_iterator ex(e0);
			++e0;
			boost::remove_edge(*ex, m_graph);
			if (lgraphrestartedgescan) {
				done = false;
				break;
			}
		}
		if (done)
			break;
	}
}

bool CFMUAutoroute::VFRRouter::add_star(const Point& pt)
{
	if (pt.is_invalid())
		return false;
	Graph::vertex_descriptor u;
	if (!m_graph.find_nearest(u, pt, Vertex::typemask_all))
		return false;
	if (u == m_dep)
		return false;
	const Vertex& uu(m_graph[u]);
	double dist(pt.spheric_distance_nmi_dbl(uu.get_coord()));
	if (dist > 0.1)
		return false;
	m_ar.m_airportconnident[1] = uu.get_ident();
	m_ar.m_airportconntype[1] = uu.get_type();
	// remove destination in edges
	remove_star();
	// add edge
	boost::add_edge(u, m_dest, Edge("", 0, 600, AirwaysDb::Airway::airway_invalid, dist), m_graph);
	m_star.clear();
	return true;
}

bool CFMUAutoroute::VFRRouter::add_star(const AirportsDb::Airport& arpt)
{
	bool added(false);
	m_star.clear();
	for (unsigned int i = 0, j = arpt.get_nrvfrrte(); i < j; ++i) {
		const AirportsDb::Airport::VFRRoute& rte(arpt.get_vfrrte(i));
		if (!rte.size() || rte.get_name().empty())
			continue;
		const AirportsDb::Airport::VFRRoute::VFRRoutePoint& rtept(rte[0]);
		if (rtept.get_pathcode() != AirportsDb::Airport::VFRRoute::VFRRoutePoint::pathcode_arrival)
			continue;
		Graph::vertex_descriptor v;
		{
			bool found(m_graph.find_nearest(v, rtept.get_coord(), Vertex::typemask_all));
			if (found) {
				const Vertex& vv(m_graph[v]);
				double dist(rtept.get_coord().spheric_distance_nmi_dbl(vv.get_coord()));
				if (dist > 0.1)
					found = false;
			}
			if (!found)
				v = boost::add_vertex(Vertex(rte.get_name(), rtept.get_coord(), Vertex::type_vfrreportingpt), m_graph);
		}
		if (!added)
			remove_star();
		added = true;
		boost::add_edge(v, m_dest, Edge("", 0, 600, AirwaysDb::Airway::airway_invalid, rte.compute_distance()), m_graph);
		m_star.push_back(rte);
	}
	return added;
}

void CFMUAutoroute::VFRRouter::set_metric(void)
{
	Graph::edge_iterator e0, e1;
	for (boost::tie(e0, e1) = boost::edges(m_graph); e0 != e1; ++e0) {
		Edge& ee(m_graph[*e0]);
		ee.set_metric(ee.get_dist());
	}
}

void CFMUAutoroute::VFRRouter::exclude_airspace(int baselevel, int toplevel, double maxarea)
{
	if (maxarea <= 0)
		return;
	AirspacesDb::elementvector_t airspaces(m_ar.m_airspacedb.find_by_rect(m_bbox, 0, AirspacesDb::element_t::subtables_none));
	baselevel *= 100;
	toplevel *= 100;
	for (AirspacesDb::elementvector_t::const_iterator ai(airspaces.begin()), ae(airspaces.end()); ai != ae; ++ai) {
		const AirspacesDb::Airspace& aspc(*ai);
		{
			char bc(aspc.get_bdryclass());
			uint8_t tc(aspc.get_typecode());
			switch (tc) {
			default:
				// do not care about airspace classes E, F, G
				if (bc < 'A' || bc > 'D') {
					if (false) {
						std::ostringstream oss;
						oss << "VFR: exclude_airspace: skipping " << aspc.get_icao_name() << ' ' << aspc.get_class_string()
							  << " altitude: " << baselevel << "..." << toplevel << " due to class" << std::endl;
						m_ar.m_signal_log(log_debug0, oss.str());
					}
					continue;
				}
				break;

			case AirspacesDb::Airspace::typecode_specialuse:
				// Special Use airspaces; only care about prohibited, reserved and danger areas
				if (bc != 'P' && bc != 'R' && bc != 'D') {
					if (false) {
						std::ostringstream oss;
						oss << "VFR: exclude_airspace: skipping " << aspc.get_icao_name() << ' ' << aspc.get_class_string()
						    << " altitude: " << baselevel << "..." << toplevel << " due to class" << std::endl;
						m_ar.m_signal_log(log_debug0, oss.str());
					}
					continue;
				}
				break;

			case AirspacesDb::Airspace::typecode_ead:
				// EAD airspaces; only care about prohibited, reserved and danger areas
                                if (bc != AirspacesDb::Airspace::bdryclass_ead_d_amc && 
				    bc != AirspacesDb::Airspace::bdryclass_ead_d && 
				    bc != AirspacesDb::Airspace::bdryclass_ead_d_other && 
				    bc != AirspacesDb::Airspace::bdryclass_ead_p && 
				    bc != AirspacesDb::Airspace::bdryclass_ead_r_amc && 
				    bc != AirspacesDb::Airspace::bdryclass_ead_r) {
					if (false) {
						std::ostringstream oss;
						oss << "VFR: exclude_airspace: skipping " << aspc.get_icao_name() << ' ' << aspc.get_class_string()
						    << " altitude: " << baselevel << "..." << toplevel << " due to class" << std::endl;
						m_ar.m_signal_log(log_debug0, oss.str());
					}
					continue;
				}
				break;
			}
		}
		if (baselevel >= aspc.get_altupr_corr() || toplevel <= aspc.get_altlwr_corr()) {
			if (false) {
				std::ostringstream oss;
				oss << "VFR: exclude_airspace: skipping " << aspc.get_icao_name() << ' ' << aspc.get_class_string()
				    << " due to altitude: " << baselevel << "..." << toplevel << " fully outside "
				    << aspc.get_altlwr_corr() << "..." << aspc.get_altupr_corr() << std::endl;
				m_ar.m_signal_log(log_debug0, oss.str());
			}
			continue;
		}
		if (false && !(baselevel >= aspc.get_altlwr_corr() && toplevel <= aspc.get_altupr_corr())) {
			if (false) {
				std::ostringstream oss;
				oss << "VFR: exclude_airspace: skipping " << aspc.get_icao_name() << ' ' << aspc.get_class_string()
				    << " due to altitude: " << baselevel << "..." << toplevel << " not inside "
				    << aspc.get_altlwr_corr() << "..." << aspc.get_altupr_corr() << std::endl;
				m_ar.m_signal_log(log_debug0, oss.str());
			}
			continue;
		}
		Rect bbox(aspc.get_bbox());
		const MultiPolygonHole& poly(aspc.get_polygon());
		double area(fabs(poly.get_simple_area_nmi2_dbl()));
		if (area > maxarea) {
			if (false) {
				std::ostringstream oss;
				oss << "VFR: exclude_airspace: skipping " << aspc.get_icao_name() << ' ' << aspc.get_class_string()
				    << " due to area: " << area << "nmi^2" << std::endl;
				m_ar.m_signal_log(log_debug0, oss.str());
			}
			continue;
		}
		unsigned int killcnt(0);
		Graph::edge_iterator e0, e1;
		for (boost::tie(e0, e1) = boost::edges(m_graph); e0 != e1; ++e0) {
			Edge& ee(m_graph[*e0]);
			if (ee.get_metric() == std::numeric_limits<double>::max())
				continue;
			Graph::vertex_descriptor u(boost::source(*e0, m_graph));
			if (u == m_dep)
				continue;
			Graph::vertex_descriptor v(boost::target(*e0, m_graph));
			if (v == m_dest)
				continue;
			const Vertex& uu(m_graph[u]);
			const Vertex& vv(m_graph[v]);
			bool uui(bbox.is_inside(uu.get_coord()));
			bool vvi(bbox.is_inside(vv.get_coord()));
			{
				Rect r(uu.get_coord(), uu.get_coord());
				r = r.add(vv.get_coord());
				if (!r.is_intersect(bbox))
					continue;
			}
			if ((uui && poly.windingnumber(uu.get_coord())) ||
			    (vvi && poly.windingnumber(vv.get_coord())) ||
			    poly.is_intersection(uu.get_coord(), vv.get_coord())) {
				//ee.set_metric(std::numeric_limits<double>::max());
				ee.set_metric(ee.get_dist() * 1000);
				++killcnt;
				if (false) {
					std::ostringstream oss;
					oss << "VFR: airspace: kill edge " << uu.get_ident() << "->" << vv.get_ident() << std::endl;
					m_ar.m_signal_log(log_debug0, oss.str());
				}
			}
		}
		if (false || killcnt) {
			std::ostringstream oss;
			oss << "VFR: exclude_airspace: " << aspc.get_icao_name() << ' ' << aspc.get_class_string()
			    << " altitude: " << aspc.get_altlwr_corr() << "..." << aspc.get_altupr_corr()
			    << " area: " << area << "nmi^2: removed " << killcnt << " edges";
			m_ar.m_signal_log(log_debug0, oss.str());
		}
	}
}

void CFMUAutoroute::VFRRouter::exclude_regions(void)
{
	for (excluderegions_t::const_iterator ei(m_ar.m_excluderegions.begin()), ee(m_ar.m_excluderegions.end()); ei != ee; ++ei) {
		AirspacesDb::Airspace aspc;
		Rect bbox(ei->get_bbox());
		if (ei->is_airspace()) {
			AirspacesDb::elementvector_t ev(m_ar.m_airspacedb.find_by_icao(ei->get_airspace_id(), 0, AirspacesDb::comp_exact, 0, AirspacesDb::Airspace::subtables_all));
			int eadidx(-1);
			for (unsigned int evi = 0; evi < ev.size(); ) {
				if (!ev[evi].is_valid() || ev[evi].get_class_string() != ei->get_airspace_type()) {
					ev.erase(ev.begin() + evi);
					continue;
				}
				if (ev[evi].get_typecode() == AirspacesDb::Airspace::typecode_ead && eadidx == -1)
					eadidx = evi;
				++evi;
			}
			if (eadidx == -1)
				eadidx = 0;
			if (ev.empty()) {
				std::ostringstream oss;
				oss << "Exclude Regions: Airspace " << ei->get_airspace_id() << '/' << ei->get_airspace_type()
				    << " not found";
				m_ar.m_signal_log(log_debug0, oss.str());
				continue;
			}
			aspc = ev[eadidx];
			bbox = aspc.get_bbox();
			if (ev.size() > 1) {
				std::ostringstream oss;
				oss << "Exclude Regions: Multiple Airspaces " << ei->get_airspace_id() << '/' << ei->get_airspace_type()
				    << " found";
				m_ar.m_signal_log(log_debug0, oss.str());
			}
		}
		unsigned int killcnt(0);
		Graph::edge_iterator e0, e1;
		for (boost::tie(e0, e1) = boost::edges(m_graph); e0 != e1; ++e0) {
			Edge& ee(m_graph[*e0]);
			if (ee.get_metric() == std::numeric_limits<double>::max())
				continue;
			Graph::vertex_descriptor u(boost::source(*e0, m_graph));
			if (u == m_dep)
				continue;
			Graph::vertex_descriptor v(boost::target(*e0, m_graph));
			if (v == m_dest)
				continue;
			const Vertex& uu(m_graph[u]);
			const Vertex& vv(m_graph[v]);
			bool uui(bbox.is_inside(uu.get_coord()));
			bool vvi(bbox.is_inside(vv.get_coord()));
			if (aspc.is_valid()) {
				{
					Rect r(uu.get_coord(), uu.get_coord());
					r = r.add(vv.get_coord());
					if (!r.is_intersect(bbox))
						continue;
				}
				if ((!uui || !aspc.get_polygon().windingnumber(uu.get_coord())) &&
				    (!vvi || !aspc.get_polygon().windingnumber(vv.get_coord())) &&
				    !aspc.get_polygon().is_intersection(uu.get_coord(), vv.get_coord()))
					continue;
			} else {
				if (!uui && !vvi && !bbox.is_intersect(uu.get_coord(), vv.get_coord()))
					continue;
			}
			//ee.set_metric(std::numeric_limits<double>::max());
			ee.set_metric(ee.get_dist() * 1000);
			++killcnt;
			if (false) {
				std::ostringstream oss;
				oss << "VFR: airspace: kill edge " << uu.get_ident() << "->" << vv.get_ident() << std::endl;
				m_ar.m_signal_log(log_debug0, oss.str());
			}
		}
		if (false || killcnt) {
			std::ostringstream oss;
			oss << "VFR: exclude_regions: ";
			if (aspc.is_valid())
				oss << aspc.get_icao_name() << ' ' << aspc.get_class_string();
			else
				oss << bbox;
			oss << " removed " << killcnt << " edges";
			m_ar.m_signal_log(log_debug0, oss.str());
		}
	}
}

bool CFMUAutoroute::VFRRouter::autoroute(FPlanRoute& route)
{
	while (route.get_nrwpt())
		route.delete_wpt(0);
	if (m_dep == m_dest) {
		if (true) {
			std::ostringstream oss;
			oss << "VFR: departure and destination are the same";
			m_ar.m_signal_log(log_debug0, oss.str());
		}
		return false;
	}
	std::vector<Graph::vertex_descriptor> predecessors;
	m_graph.shortest_paths_metric(m_dep, predecessors);
	{
		const Vertex& vv(m_graph[m_dest]);
		if (!vv.insert(route, 0)) {
			if (true) {
				std::ostringstream oss;
				oss << "VFR: cannot insert " << vv.get_ident() << " into fpl" ;
				m_ar.m_signal_log(log_debug0, oss.str());
			}
			return false;
		}
	}
	Graph::vertex_descriptor v(m_dest);
	while (v != m_dep) {
		Graph::vertex_descriptor u(predecessors[v]);
		if (u == v) {
			if (true) {
				std::ostringstream oss;
				oss << "VFR: no route found; "
				    << boost::out_degree(m_dep, m_graph) << " SID edges, "
				    << boost::in_degree(m_dest, m_graph) << " STAR edges";
				m_ar.m_signal_log(log_debug0, oss.str());
			}
			return false;
		}
		const Vertex& uu(m_graph[u]);
		const Vertex& vv(m_graph[v]);
		bool rteadded(false);
		if (v == m_dest && uu.is_match(Vertex::typemask_vfrreportingpt)) {
			for (sidstar_t::const_iterator si(m_star.begin()), se(m_star.end()); si != se; ++si) {
				const AirportsDb::Airport::VFRRoute& rte(*si);
				if (!rte.size() || rte.get_name() != uu.get_ident())
					continue;
				const AirportsDb::Airport::VFRRoute::VFRRoutePoint& rtept(rte[0]);
				if (rtept.get_pathcode() != AirportsDb::Airport::VFRRoute::VFRRoutePoint::pathcode_arrival)
					continue;
				AirportsDb::Airport arpt;
				m_graph[m_dest].get(arpt);
				for (unsigned int i = 0, j = rte.size() - 1U; i < j; ++i) {
					const AirportsDb::Airport::VFRRoute::VFRRoutePoint& rtept(rte[i]);
					FPlanWaypoint wpt;
					if (rtept.is_at_airport() && arpt.is_valid()) {
						wpt.set(arpt);
					} else {
						if (arpt.is_valid())
							wpt.set_icao(arpt.get_icao());
						wpt.set_name(rtept.get_name());
						wpt.set_coord(rtept.get_coord());
						wpt.set_altitude(rtept.get_altitude());
					}
					wpt.set_pathcode(FPlanWaypoint::pathcode_vfrarrival);
					wpt.set_pathname(rte.get_name());
					route.insert_wpt(i, wpt);
				}
				rteadded = true;
				break;
			}
		}
		if (u == m_dep && vv.is_match(Vertex::typemask_vfrreportingpt)) {
			for (sidstar_t::const_iterator si(m_sid.begin()), se(m_sid.end()); si != se; ++si) {
				const AirportsDb::Airport::VFRRoute& rte(*si);
				if (!rte.size() || rte.get_name() != vv.get_ident())
					continue;
				const AirportsDb::Airport::VFRRoute::VFRRoutePoint& rtept(rte[rte.size()-1]);
				if (rtept.get_pathcode() != AirportsDb::Airport::VFRRoute::VFRRoutePoint::pathcode_departure)
					continue;
				AirportsDb::Airport arpt;
				m_graph[m_dep].get(arpt);
				for (unsigned int i = 0, j = rte.size() - 1U; i < j; ++i) {
					const AirportsDb::Airport::VFRRoute::VFRRoutePoint& rtept(rte[i]);
					FPlanWaypoint wpt;
					if (rtept.is_at_airport() && arpt.is_valid()) {
						wpt.set(arpt);
					} else {
						if (arpt.is_valid())
							wpt.set_icao(arpt.get_icao());
						wpt.set_name(rtept.get_name());
						wpt.set_coord(rtept.get_coord());
						wpt.set_altitude(rtept.get_altitude());
					}
					wpt.set_pathcode(FPlanWaypoint::pathcode_vfrdeparture);
					wpt.set_pathname(rte.get_name());
					route.insert_wpt(i, wpt);
				}
				rteadded = true;
				break;
			}
		}
		if (!rteadded && uu.is_match(Vertex::typemask_vfrreportingpt)) {
			// FIXME!!
			std::ostringstream oss;
			oss << "VFR: do not know what to do with VFR arr/dep point " << uu.get_ident();
			m_ar.m_signal_log(log_debug0, oss.str());
			rteadded = true;
		}
		if (!rteadded && !uu.insert(route, 0)) {
			if (true) {
				std::ostringstream oss;
				oss << "VFR: cannot insert " << uu.get_ident() << " into fpl";
				m_ar.m_signal_log(log_debug0, oss.str());
			}
			return false;
		}
		v = u;
	}
	return true;
}

template bool CFMUAutoroute::is_metric_invalid(float m);
template bool CFMUAutoroute::is_metric_invalid(double m);
