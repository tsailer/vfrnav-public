#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <iostream>
#include <iomanip>

#include <boost/graph/dijkstra_shortest_paths.hpp>
#include <boost/graph/johnson_all_pairs_shortest.hpp>
#include <boost/graph/floyd_warshall_shortest.hpp>
#include <boost/graph/filtered_graph.hpp>

#include "cfmuautoroute45.hh"
#include "interval.hh"

const bool CFMUAutoroute45::lgraphawyvertdct;
constexpr double CFMUAutoroute45::localforbiddenpenalty;

class CFMUAutoroute45::AirspaceCache {
public:
	class Airspace : public AirspacesDb::Airspace {
	public:
		Airspace(void) : AirspacesDb::Airspace() {}
		Airspace(const AirspacesDb::Airspace& aspc) : AirspacesDb::Airspace() { static_cast<AirspacesDb::Airspace&>(*this) = aspc; }
		typedef enum {
			cache_false = false,
			cache_true = true,
			cache_uncached
		} cache_t;

		cache_t query(const Point& pt) const;
		cache_t query(const Point& pt0, const Point& pt1) const;
		void insert(const Point& pt = Point(), bool inside = false) const;
		void insert(const Point& pt0 = Point(), const Point& pt1 = Point(), bool isect = false) const;

	protected:
		class PointCache {
		public:
			PointCache(const Point& pt = Point(), bool inside = false);
			const Point& get_point(void) const { return m_point; }
			bool is_inside(void) const { return m_inside; }
			bool operator<(const PointCache& x) const;

		protected:
			Point m_point;
			bool m_inside;
		};

		class LineCache {
		public:
			LineCache(const Point& pt0 = Point(), const Point& pt1 = Point(), bool isect = false);
			const Point& get_point0(void) const { return m_point0; }
			const Point& get_point1(void) const { return m_point1; }
			bool is_intersect(void) const { return m_intersect; }
			bool operator<(const LineCache& x) const;

		protected:
			Point m_point0;
			Point m_point1;
			bool m_intersect;
		};

		typedef std::set<PointCache> pointcache_t;
		mutable pointcache_t m_pointcache;
		typedef std::set<LineCache> linecache_t;
		mutable linecache_t m_linecache;
	};

	AirspaceCache(AirspacesDb& aspcdb);

	const Airspace& find_airspace(const std::string& ident, char bclass, uint8_t tc = AirspacesDb::Airspace::typecode_ead);
	bool is_inside(const Airspace& aspc, const Point& coord) {
		return is_inside(aspc, coord, 0, 0, std::numeric_limits<int32_t>::max());
	}
	bool is_inside(const Airspace& aspc, const Point& coord, int32_t alt) {
		return is_inside(aspc, coord, alt, std::numeric_limits<int32_t>::min(), std::numeric_limits<int32_t>::min());
	}
	bool is_inside(const Airspace& aspc, const Point& coord, int32_t alt, int32_t altlwr, int32_t altupr);
	bool is_intersection(const Airspace& aspc, const Point& la, const Point& lb) {
		return is_intersection(aspc, la, lb, 0, 0, std::numeric_limits<int32_t>::max());
	}
	bool is_intersection(const Airspace& aspc, const Point& la, const Point& lb, int32_t alt) {
		return is_intersection(aspc, la, lb, 0, std::numeric_limits<int32_t>::min(), std::numeric_limits<int32_t>::min());
	}
	bool is_intersection(const Airspace& aspc, const Point& la, const Point& lb, int32_t alt, int32_t altlwr, int32_t altupr);
	IntervalSet<int32_t> get_altrange(const Airspace& aspc, const Point& coord,
					  int32_t altlwr = std::numeric_limits<int32_t>::min(),
					  int32_t altupr = std::numeric_limits<int32_t>::min());
	IntervalSet<int32_t> get_altrange(const Airspace& aspc, const Point& coord0, const Point& coord1,
					  int32_t altlwr = std::numeric_limits<int32_t>::min(),
					  int32_t altupr = std::numeric_limits<int32_t>::min());

protected:
	static const Airspace invalid;

	AirspacesDb& m_airspacedb;

	struct AirspaceOrder : public std::binary_function<const AirspacesDb::Airspace&, const AirspacesDb::Airspace&, bool> {
		bool operator()(const Airspace& x, const Airspace& y);
	};

	typedef std::set<Airspace, AirspaceOrder> airspacecache_t;
	airspacecache_t m_airspacecache;
};

bool CFMUAutoroute45::AirspaceCache::AirspaceOrder::operator()(const Airspace& x, const Airspace& y)
{
	{
		int c(x.get_icao().compare(y.get_icao()));
		if (c)
			return c < 0;
	}
	if (x.get_bdryclass() < y.get_bdryclass())
		return true;
	if (y.get_bdryclass() < x.get_bdryclass())
		return false;
	return x.get_typecode() < y.get_typecode();
}

CFMUAutoroute45::AirspaceCache::Airspace::PointCache::PointCache(const Point& pt, bool inside)
	: m_point(pt), m_inside(inside)
{
}

bool CFMUAutoroute45::AirspaceCache::Airspace::PointCache::operator<(const PointCache& x) const
{
        if (m_point.get_lat() < x.m_point.get_lat())
                return true;
        if (x.m_point.get_lat() < m_point.get_lat())
                return false;
        return m_point.get_lon() < x.m_point.get_lon();
}

CFMUAutoroute45::AirspaceCache::Airspace::LineCache::LineCache(const Point& pt0, const Point& pt1, bool isect)
	: m_point0(pt0), m_point1(pt1), m_intersect(isect)
{
	if (m_point0.get_lat() < m_point1.get_lat())
		return;
	if (m_point1.get_lat() < m_point0.get_lat()) {
		std::swap(m_point0, m_point1);
		return;
	}
	if (m_point1.get_lon() < m_point0.get_lon()) {
		std::swap(m_point0, m_point1);
		return;
	}
}

bool CFMUAutoroute45::AirspaceCache::Airspace::LineCache::operator<(const LineCache& x) const
{
        if (m_point0.get_lat() < x.m_point0.get_lat())
                return true;
        if (x.m_point0.get_lat() < m_point0.get_lat())
                return false;
        if (m_point0.get_lon() < x.m_point0.get_lon())
                return true;
        if (x.m_point0.get_lon() < m_point0.get_lon())
                return false;
        if (m_point1.get_lat() < x.m_point1.get_lat())
                return true;
        if (x.m_point1.get_lat() < m_point1.get_lat())
                return false;
        return m_point1.get_lon() < x.m_point1.get_lon();
}

CFMUAutoroute45::AirspaceCache::Airspace::cache_t CFMUAutoroute45::AirspaceCache::Airspace::query(const Point& pt) const
{
	PointCache q(pt);
	pointcache_t::const_iterator i(m_pointcache.find(q));
	if (i == m_pointcache.end())
		return cache_uncached;
	return (cache_t)i->is_inside();
}

CFMUAutoroute45::AirspaceCache::Airspace::cache_t CFMUAutoroute45::AirspaceCache::Airspace::query(const Point& pt0, const Point& pt1) const
{
	LineCache q(pt0, pt1);
	linecache_t::const_iterator i(m_linecache.find(q));
	if (i == m_linecache.end())
		return cache_uncached;
	return (cache_t)i->is_intersect();
}

void CFMUAutoroute45::AirspaceCache::Airspace::insert(const Point& pt, bool inside) const
{
	m_pointcache.insert(PointCache(pt, inside));
}

void CFMUAutoroute45::AirspaceCache::Airspace::insert(const Point& pt0, const Point& pt1, bool isect) const
{
	m_linecache.insert(LineCache(pt0, pt1, isect));
}

const CFMUAutoroute45::AirspaceCache::Airspace CFMUAutoroute45::AirspaceCache::invalid;

CFMUAutoroute45::AirspaceCache::AirspaceCache(AirspacesDb& aspcdb)
	: m_airspacedb(aspcdb)
{
}

const CFMUAutoroute45::AirspaceCache::Airspace& CFMUAutoroute45::AirspaceCache::find_airspace(const std::string& ident, char bclass, uint8_t tc)
{
	bool ld(false);
	for (;;) {
		{
			AirspacesDb::Airspace aspc;
			aspc.set_icao(ident);
			aspc.set_bdryclass(bclass);
			aspc.set_typecode(tc);
			airspacecache_t::const_iterator i(m_airspacecache.find(aspc));
			if (i != m_airspacecache.end())
				return *i;
		}
		if (ld)
			return invalid;
		ld = true;
		AirspacesDb::elementvector_t ev(m_airspacedb.find_by_icao(ident, 0, AirspacesDb::comp_exact, 0, AirspacesDb::Airspace::subtables_all));
                for (AirspacesDb::elementvector_t::const_iterator i(ev.begin()), e(ev.end()); i != e; ++i) {
			if (!i->is_valid())
				continue;
			m_airspacecache.insert(*i);
		}
	}
}

bool CFMUAutoroute45::AirspaceCache::is_inside(const Airspace& aspc, const Point& coord, int32_t alt, int32_t altlwr, int32_t altupr)
{
	if (!aspc.is_valid())
		return false;
	if (!aspc.get_nrcomponents()) {
		if (altlwr == std::numeric_limits<int32_t>::min())
			altlwr = aspc.get_altlwr();
		if (altupr == std::numeric_limits<int32_t>::min())
			altupr = aspc.get_altupr();
		bool val(alt >= altlwr && alt <= altupr);
		val = val && aspc.get_bbox().is_inside(coord);
		if (val && !true) {
			Airspace::cache_t c(aspc.query(coord));
			if (c == Airspace::cache_uncached) {
				val = !!aspc.get_polygon().windingnumber(coord);
				aspc.insert(coord, val);
			} else {
				val = c;
			}
		} else {
			val = val && aspc.get_polygon().windingnumber(coord);
		}
		if (false && !aspc.get_icao().compare(0, 4, "LSAZ")) {
			std::cerr << "is_inside: " << aspc.get_icao() << ": " << coord
				  << " F" << std::setw(3) << std::setfill('0') << (alt / 100)
				  << " F" << std::setw(3) << std::setfill('0') << (altlwr / 100)
				  << "...F" << std::setw(3) << std::setfill('0') << (altupr / 100)
				  << " (F" << std::setw(3) << std::setfill('0') << (aspc.get_altlwr() / 100)
				  << "...F" << std::setw(3) << std::setfill('0') << (aspc.get_altupr() / 100)
				  << "): result " << (val ? 'T' : 'F') << std::endl;
		}
		return val;
	}
	bool val(false);
	for (unsigned int cnr(0); cnr < aspc.get_nrcomponents(); ++cnr) {
		const Airspace::Component& comp(aspc.get_component(cnr));
		const Airspace& aspcc(find_airspace(comp.get_icao(), comp.get_bdryclass(), comp.get_typecode()));
		switch (comp.get_operator()) {
		default:
		case Airspace::Component::operator_set:
			val = is_inside(aspcc, coord, alt, altlwr, altupr);
			break;

		case Airspace::Component::operator_union:
			val = val || is_inside(aspcc, coord, alt, altlwr, altupr);
			break;

		case Airspace::Component::operator_subtract:
			val = val && !is_inside(aspcc, coord, alt, altlwr, altupr);
			break;

		case Airspace::Component::operator_intersect:
			val = val && is_inside(aspcc, coord, alt, altlwr, altupr);
			break;
		}
	}
	if (false && !aspc.get_icao().compare(0, 4, "LSAZ")) {
		std::cerr << "is_inside(C): " << aspc.get_icao() << ": " << coord
			  << " F" << std::setw(3) << std::setfill('0') << (alt / 100)
			  << " F" << std::setw(3) << std::setfill('0') << (altlwr / 100)
			  << "...F" << std::setw(3) << std::setfill('0') << (altupr / 100)
			  << " (F" << std::setw(3) << std::setfill('0') << (aspc.get_altlwr() / 100)
			  << "...F" << std::setw(3) << std::setfill('0') << (aspc.get_altupr() / 100)
			  << "): result " << (val ? 'T' : 'F') << std::endl;;
	}
	return val;
}

bool CFMUAutoroute45::AirspaceCache::is_intersection(const Airspace& aspc, const Point& la, const Point& lb, int32_t alt, int32_t altlwr, int32_t altupr)
{
	if (!aspc.is_valid())
		return false;
	if (!aspc.get_nrcomponents()) {
		if (altlwr == std::numeric_limits<int32_t>::min())
			altlwr = aspc.get_altlwr();
		if (altupr == std::numeric_limits<int32_t>::min())
			altupr = aspc.get_altupr();
		bool val(alt >= altlwr && alt <= altupr);
		if (val && la.get_lat() > aspc.get_bbox().get_north() && lb.get_lat() > aspc.get_bbox().get_north())
			val = false;
		if (val && la.get_lat() < aspc.get_bbox().get_south() && lb.get_lat() < aspc.get_bbox().get_south())
			val = false;
		if (val) {
			Point::coord_t lalon(la.get_lon() - aspc.get_bbox().get_west());
			Point::coord_t lblon(lb.get_lon() - aspc.get_bbox().get_west());
			Point::coord_t w(aspc.get_bbox().get_east() - aspc.get_bbox().get_west());
			if (lalon < 0 && lblon < 0)
				val = false;
			if (lalon > w && lblon > w)
				val = false;
		}
		if (val && !true) {
			Airspace::cache_t c(aspc.query(la, lb));
			if (c == Airspace::cache_uncached) {
				val = !!aspc.get_polygon().is_intersection(la, lb);
				aspc.insert(la, lb, val);
			} else {
				val = c;
			}
		} else {
			val = val && aspc.get_polygon().is_intersection(la, lb);
		}
		if (false && !aspc.get_icao().compare(0, 4, "LSAZ")) {
			std::cerr << "is_intersection: " << aspc.get_icao() << ": " << la << " -> " << lb
				  << " F" << std::setw(3) << std::setfill('0') << (alt / 100)
				  << " F" << std::setw(3) << std::setfill('0') << (altlwr / 100)
				  << "...F" << std::setw(3) << std::setfill('0') << (altupr / 100)
				  << " (F" << std::setw(3) << std::setfill('0') << (aspc.get_altlwr() / 100)
				  << "...F" << std::setw(3) << std::setfill('0') << (aspc.get_altupr() / 100)
				  << "): result " << (val ? 'T' : 'F') << std::endl;
		}	
		return val;
	}
	bool val(false);
	for (unsigned int cnr(0); cnr < aspc.get_nrcomponents(); ++cnr) {
		const Airspace::Component& comp(aspc.get_component(cnr));
		const Airspace& aspcc(find_airspace(comp.get_icao(), comp.get_bdryclass(), comp.get_typecode()));
		switch (comp.get_operator()) {
		default:
		case Airspace::Component::operator_set:
			val = is_intersection(aspcc, la, lb, alt, altlwr, altupr);
			break;

		case Airspace::Component::operator_union:
			val = val || is_intersection(aspcc, la, lb, alt, altlwr, altupr);
			break;

		case Airspace::Component::operator_subtract:
			val = val && !is_intersection(aspcc, la, lb, alt, altlwr, altupr);
			break;

		case Airspace::Component::operator_intersect:
			val = val && is_intersection(aspcc, la, lb, alt, altlwr, altupr);
			break;
		}
	}
	if (false && !aspc.get_icao().compare(0, 4, "LSAZ")) {
		std::cerr << "is_intersection(C): " << aspc.get_icao() << ": " << la << " -> " << lb
			  << " F" << std::setw(3) << std::setfill('0') << (alt / 100)
			  << " F" << std::setw(3) << std::setfill('0') << (altlwr / 100)
			  << "...F" << std::setw(3) << std::setfill('0') << (altupr / 100)
			  << " (F" << std::setw(3) << std::setfill('0') << (aspc.get_altlwr() / 100)
			  << "...F" << std::setw(3) << std::setfill('0') << (aspc.get_altupr() / 100)
			  << "): result " << (val ? 'T' : 'F') << std::endl;
	}
	return val;
}

IntervalSet<int32_t> CFMUAutoroute45::AirspaceCache::get_altrange(const Airspace& aspc, const Point& coord, int32_t altlwr, int32_t altupr)
{
	IntervalSet<int32_t> r;
	r.set_empty();
	if (!aspc.is_valid())
		return r;
	if (!aspc.get_nrcomponents()) {
		if (!aspc.get_bbox().is_inside(coord) || !aspc.get_polygon().windingnumber(coord))
			return r;
		if (altlwr == std::numeric_limits<int32_t>::min())
			altlwr = aspc.get_altlwr();
		if (altupr == std::numeric_limits<int32_t>::min())
			altupr = aspc.get_altupr();
		r = IntervalSet<int32_t>(IntervalSet<int32_t>::Intvl(altlwr, altupr+1));
		return r;
	}
	for (unsigned int cnr(0); cnr < aspc.get_nrcomponents(); ++cnr) {
		const Airspace::Component& comp(aspc.get_component(cnr));
		const Airspace& aspcc(find_airspace(comp.get_icao(), comp.get_bdryclass(), comp.get_typecode()));
		IntervalSet<int32_t> r1(get_altrange(aspcc, coord, altlwr, altupr));
		switch (comp.get_operator()) {
		default:
		case Airspace::Component::operator_set:
			r = r1;
			break;

		case Airspace::Component::operator_union:
			r |= r1;
			break;

		case Airspace::Component::operator_subtract:
			r &= ~r1;
			break;

		case Airspace::Component::operator_intersect:
			r &= r1;
			break;
		}
	}
	return r;
}

IntervalSet<int32_t> CFMUAutoroute45::AirspaceCache::get_altrange(const Airspace& aspc, const Point& coord0, const Point& coord1, int32_t altlwr, int32_t altupr)
{
	IntervalSet<int32_t> r;
	r.set_empty();
	if (!aspc.is_valid())
		return r;
	if (!aspc.get_nrcomponents()) {
		if (!((aspc.get_bbox().is_inside(coord0) && aspc.get_polygon().windingnumber(coord0)) ||
		      (aspc.get_bbox().is_inside(coord1) && aspc.get_polygon().windingnumber(coord1)) ||
		      is_intersection(aspc, coord0, coord1)))
			return r;
		if (altlwr == std::numeric_limits<int32_t>::min())
			altlwr = aspc.get_altlwr();
		if (altupr == std::numeric_limits<int32_t>::min())
			altupr = aspc.get_altupr();
		r = IntervalSet<int32_t>(IntervalSet<int32_t>::Intvl(altlwr, altupr+1));
		return r;
	}
	for (unsigned int cnr(0); cnr < aspc.get_nrcomponents(); ++cnr) {
		const Airspace::Component& comp(aspc.get_component(cnr));
		const Airspace& aspcc(find_airspace(comp.get_icao(), comp.get_bdryclass(), comp.get_typecode()));
		IntervalSet<int32_t> r1(get_altrange(aspcc, coord0, coord1, altlwr, altupr));
		switch (comp.get_operator()) {
		default:
		case Airspace::Component::operator_set:
			r = r1;
			break;

		case Airspace::Component::operator_union:
			r |= r1;
			break;

		case Airspace::Component::operator_subtract:
			r &= ~r1;
			break;

		case Airspace::Component::operator_intersect:
			r &= r1;
			break;
		}
	}
	return r;
}

CFMUAutoroute45::LVertex::LVertex(Engine::DbObject::ptr_t obj)
	: m_obj(obj)
{
	m_coord.set_invalid();
	if (m_obj) {
		m_coord = m_obj->get_coord();
		m_ident = m_obj->get_ident();
	}
}

bool CFMUAutoroute45::LVertex::is_airport(void) const
{
	if (!m_obj)
		return false;
	return m_obj->is_airport();
}

bool CFMUAutoroute45::LVertex::is_navaid(void) const
{
	if (!m_obj)
		return false;
	return m_obj->is_navaid();
}

bool CFMUAutoroute45::LVertex::is_intersection(void) const
{
	if (!m_obj)
		return false;
	return m_obj->is_intersection();
}

bool CFMUAutoroute45::LVertex::is_mapelement(void) const
{
	if (!m_obj)
		return false;
	return m_obj->is_mapelement();
}

bool CFMUAutoroute45::LVertex::get(AirportsDb::Airport& el) const
{
	if (!m_obj)
		return false;
	return m_obj->get(el);
}

bool CFMUAutoroute45::LVertex::get(NavaidsDb::Navaid& el) const
{
	if (!m_obj)
		return false;
	return m_obj->get(el);
}

bool CFMUAutoroute45::LVertex::get(WaypointsDb::Waypoint& el) const
{
	if (!m_obj)
		return false;
	return m_obj->get(el);
}

bool CFMUAutoroute45::LVertex::get(MapelementsDb::Mapelement& el) const
{
	if (!m_obj)
		return false;
	return m_obj->get(el);
}

bool CFMUAutoroute45::LVertex::set(FPlanWaypoint& el) const
{
	if (!m_obj)
		return false;
	return m_obj->set(el);
}

unsigned int CFMUAutoroute45::LVertex::insert(FPlanRoute& route, uint32_t nr) const
{
	if (!m_obj)
		return false;
	return m_obj->insert(route, nr);
}

bool CFMUAutoroute45::LVertex::is_ident_valid(void) const
{
	const std::string& id(get_ident());
	return id.size() >= 2 && !Engine::AirwayGraphResult::Vertex::is_ident_numeric(id);
}

const float CFMUAutoroute45::LEdge::maxmetric = std::numeric_limits<float>::max();
const float CFMUAutoroute45::LEdge::invalidmetric = std::numeric_limits<float>::quiet_NaN();
const CFMUAutoroute45::lgraphairwayindex_t CFMUAutoroute45::LEdge::identmask;
const CFMUAutoroute45::lgraphairwayindex_t CFMUAutoroute45::LEdge::levelsmask;
const CFMUAutoroute45::lgraphairwayindex_t CFMUAutoroute45::LEdge::solutionmask;

CFMUAutoroute45::LEdge::LEdge(unsigned int levels, lgraphairwayindex_t id, float dist, float truetrack)
	: m_ident((id & identmask) | ((levels << 18) & levelsmask) | solutionmask), m_dist(dist), m_truetrack(truetrack), m_metric(0)
{
	const unsigned int pis(get_levels());
	if (pis) {
		m_metric = new float[pis];
		for (unsigned int pi = 0; pi < pis; ++pi)
			m_metric[pi] = invalidmetric;
	}
}

CFMUAutoroute45::LEdge::LEdge(const LEdge& x)
	: m_ident(x.m_ident), m_dist(x.m_dist), m_truetrack(x.m_truetrack), m_metric(0)
{
	const unsigned int pis(get_levels());
	if (pis) {
		m_metric = new float[pis];
		for (unsigned int pi = 0; pi < pis; ++pi)
			m_metric[pi] = x.m_metric[pi];
	}
}

CFMUAutoroute45::LEdge::~LEdge()
{
	delete[] m_metric;
	m_metric = 0;
}

CFMUAutoroute45::LEdge& CFMUAutoroute45::LEdge::operator=(const LEdge& x)
{
	delete[] m_metric;
	m_metric = 0;
	m_ident = x.m_ident;
	m_dist = x.m_dist;
	m_truetrack = x.m_truetrack;
	const unsigned int pis(get_levels());
	if (pis) {
		m_metric = new float[pis];
		for (unsigned int pi = 0; pi < pis; ++pi)
			m_metric[pi] = x.m_metric[pi];
	}
	return *this;
}

void CFMUAutoroute45::LEdge::swap(LEdge& x)
{
	std::swap(m_metric, x.m_metric);
	std::swap(m_ident, x.m_ident);
	std::swap(m_dist, x.m_dist);
	std::swap(m_truetrack, x.m_truetrack);
}

void CFMUAutoroute45::LEdge::resize(unsigned int levels)
{
	const unsigned int pisold(get_levels());
	m_ident ^= (m_ident ^ (levels << 18)) & levelsmask;
	const unsigned int pis(get_levels());
	float *mold(0);
	if (pis)
		mold = new float[pis];
	std::swap(m_metric, mold);
	unsigned int i(0);
	for (unsigned int n(std::min(pis, pisold)); i < n; ++i)
		m_metric[i] = mold[i];
	for (; i < pis; ++i)
		m_metric[i] = invalidmetric;
	delete[] mold;
}

bool CFMUAutoroute45::LEdge::is_match(lgraphairwayindex_t ident, lgraphairwayindex_t id)
{
	switch (id) {
	case airwaymatchall:
		return true;

	case airwaymatchnone:
		return false;

	case airwaymatchawy:
		return ident < airwaydct;

	case airwaymatchdctawy:
		return ident <= airwaydct;

	case airwaymatchdctawysidstar:
		return ident <= airwaysid;

	case airwaymatchawysidstar:
		return (ident >= airwaystar && ident <= airwaysid) || (ident < airwaydct);

	case airwaymatchsidstar:
		return ident >= airwaystar && ident <= airwaysid;

	default:
		return ident == id;
	}
}

void CFMUAutoroute45::LEdge::set_metric(unsigned int pi, float metric)
{
	if (pi >= get_levels())
		return;
	m_metric[pi] = metric;
}

float CFMUAutoroute45::LEdge::get_metric(unsigned int pi) const
{
	if (pi >= get_levels())
		return invalidmetric;
	return m_metric[pi];
}

void CFMUAutoroute45::LEdge::set_all_metric(float metric)
{
	const unsigned int pis(get_levels());
	for (unsigned int pi = 0; pi < pis; ++pi)
		m_metric[pi] = metric;
}

void CFMUAutoroute45::LEdge::scale_metric(float scale, float offset)
{
	const unsigned int pis(get_levels());
	for (unsigned int pi = 0; pi < pis; ++pi)
		if (!is_metric_invalid(m_metric[pi]))
			m_metric[pi] = m_metric[pi] * scale + offset;
}

bool CFMUAutoroute45::LEdge::is_valid(unsigned int pi) const
{
	if (pi >= get_levels())
		return false;
	return !is_metric_invalid(m_metric[pi]);
}

bool CFMUAutoroute45::LEdge::is_valid(void) const
{
	for (unsigned int pi(0), pis(get_levels()); pi < pis; ++pi)
		if (!is_metric_invalid(m_metric[pi]))
			return true;
	return false;
}

bool CFMUAutoroute45::LGraph::is_valid_connection_precheck(unsigned int piu, unsigned int piv, edge_descriptor e) const
{
	const LEdge& ee((*this)[e]);
	if (ee.is_match(airwaymatchdctawy)) {
		if (!ee.is_valid(piu))
			return false;
	} else if (ee.is_match(airwaysid)) {
		// check that the destination level is part of the SID
		if (!ee.is_valid(piv))
			return false;
	} else {
		// check that the originating level is part of the airway/STAR
		if (!ee.is_valid(piu))
			return false;
	}
	return true;
}

bool CFMUAutoroute45::LGraph::is_valid_connection_precheck(vertex_descriptor u, unsigned int piu, vertex_descriptor v, unsigned int piv, edge_descriptor e) const
{
	if (boost::source(e, *this) != u || boost::target(e, *this) != v)
		return false;
	return is_valid_connection(piu, piv, e);
}

bool CFMUAutoroute45::LGraph::is_valid_connection(unsigned int piu, unsigned int piv, edge_descriptor e) const
{
	const LEdge& ee((*this)[e]);
	if (ee.is_match(airwaymatchdctawy)) {
		if (!ee.is_valid(piu))
			return false;
		if (piu == piv)
			return true;
		// check that level changes followed by DCT edges do not violate DCT level restrictions
		unsigned int pi0(piu), pi1(piv);
		if (pi0 > pi1)
			std::swap(pi0, pi1);
		for (; pi0 <= pi1; ++pi0) {
			if (ee.is_valid(pi0))
				continue;
			if (!ee.is_dct())
				return false;
			vertex_descriptor u(boost::source(e, *this));
			vertex_descriptor v(boost::target(e, *this));
			bool ok(false);
			out_edge_iterator e0, e1;
			for (boost::tie(e0, e1) = boost::out_edges(u, *this); e0 != e1; ++e0) {
				if (boost::target(*e0, *this) != v)
					continue;
				const LEdge& ee((*this)[*e0]);
				if (!ee.is_match(airwaymatchawy))
					continue;
				ok = ee.is_valid(pi0);
				if (ok)
					break;
			}
			if (!ok)
				return false;
		}
	} else if (ee.is_match(airwaysid)) {
		// check that the destination level is part of the SID
		if (!ee.is_valid(piv))
			return false;
	} else {
		// check that the originating level is part of the airway/STAR
		if (!ee.is_valid(piu))
			return false;
	}
	return true;
}

bool CFMUAutoroute45::LGraph::is_valid_connection(vertex_descriptor u, unsigned int piu, vertex_descriptor v, unsigned int piv, edge_descriptor e) const
{
	if (boost::source(e, *this) != u || boost::target(e, *this) != v)
		return false;
	return is_valid_connection(piu, piv, e);
}

std::pair<CFMUAutoroute45::LGraph::edge_descriptor,bool> CFMUAutoroute45::LGraph::find_edge(vertex_descriptor u, vertex_descriptor v, lgraphairwayindex_t awy) const
{
	out_edge_iterator e0, e1;
	for (boost::tie(e0, e1) = boost::out_edges(u, *this); e0 != e1; ++e0) {
		if (boost::target(*e0, *this) != v)
			continue;
		const LEdge& ee((*this)[*e0]);
		if (!ee.is_match(awy))
			continue;
		return std::pair<edge_descriptor,bool>(*e0, true);
	}
	return std::pair<edge_descriptor,bool>(edge_descriptor(), false);
}

bool CFMUAutoroute45::LVertexLevel::operator<(const LVertexLevel& x) const
{
	if (get_vertex() < x.get_vertex())
		return true;
	if (get_vertex() > x.get_vertex())
		return false;
        return get_perfindex() < x.get_perfindex();
}

bool CFMUAutoroute45::LVertexLevel::operator==(const LVertexLevel& x) const
{
	return (get_vertex() == x.get_vertex()) && (get_perfindex() == x.get_perfindex());
}

bool CFMUAutoroute45::LVertexLevels::operator<(const LVertexLevels& x) const
{
	if (get_vertex() < x.get_vertex())
		return true;
	if (get_vertex() > x.get_vertex())
		return false;
	if (get_perfindex0() < x.get_perfindex0())
		return true;
 	if (get_perfindex0() > x.get_perfindex0())
		return false;
	return get_perfindex1() < x.get_perfindex1();
}

bool CFMUAutoroute45::LVertexLevels::operator==(const LVertexLevels& x) const
{
	return (get_vertex() == x.get_vertex()) && (get_perfindex0() == x.get_perfindex0()) && (get_perfindex1() == x.get_perfindex1());
}

bool CFMUAutoroute45::CompareVertexLevelsVertexOnly::operator()(const CFMUAutoroute45::LVertexLevels& a, const CFMUAutoroute45::LVertexLevels& b)
{
	return a.get_vertex() < b.get_vertex();
}

bool CFMUAutoroute45::LVertexEdge::operator<(const LVertexEdge& x) const
{
	if (LVertexLevel::operator<(x))
		return true;
	if (LVertexLevel::operator>(x))
		return false;
	return get_edge() < x.get_edge();
}

bool CFMUAutoroute45::LRoute::operator<(const LRoute& x) const
{
	if (get_metric() < x.get_metric())
		return true;
	if (get_metric() > x.get_metric())
		return false;
	for (size_type i(0), n(std::min(size(), x.size())); i < n; ++i) {
		if ((*this)[i] < x[i])
			return true;
		if ((*this)[i] > x[i])
			return false;
	}
	return size() < x.size();
}

bool CFMUAutoroute45::LRoute::is_repeated_nodes(const LGraph& g) const
{
	typedef std::set<LGraph::vertex_descriptor> nmap_t;
	nmap_t nmap;
	typedef std::set<std::string> imap_t;
	imap_t imap;
	for (const_iterator i(begin()), e(end()); i != e;) {
		LGraph::vertex_descriptor vd(i->get_vertex());
		{
			std::pair<nmap_t::iterator,bool> x(nmap.insert(vd));
			if (!x.second)
				return true;
		}
		const LVertex& v(g[vd]);
		{
			std::pair<imap_t::iterator,bool> x(imap.insert(v.get_ident()));
			if (!x.second)
				return true;
		}
		++i;
	}
	return false;
}

bool CFMUAutoroute45::LRoute::is_existing(const LGraph& g) const
{
	for (LRoute::size_type i(1); i < this->size(); ++i) {
		LGraph::edge_descriptor e;
		bool ok;
		boost::tie(e, ok) = this->get_edge(i, g);
		if (!ok)
			return false;
		if (!g.is_valid_connection((*this)[i - 1].get_perfindex(), (*this)[i].get_perfindex(), e))
			return false;
	}
	return true;
}

std::pair<CFMUAutoroute45::LGraph::edge_descriptor,bool> CFMUAutoroute45::LRoute::get_edge(const LVertexEdge& u, const LVertexEdge& v, const LGraph& g)
{
	return get_edge(u.get_vertex(), v, g);
}

std::pair<CFMUAutoroute45::LGraph::edge_descriptor,bool> CFMUAutoroute45::LRoute::get_edge(LGraph::vertex_descriptor u, const LVertexEdge& v, const LGraph& g)
{
	return g.find_edge(u, v.get_vertex(), v.get_edge());
}

std::pair<CFMUAutoroute45::LGraph::edge_descriptor,bool> CFMUAutoroute45::LRoute::get_edge(size_type i, const LGraph& g) const
{
	if (i < 1 || i >= size())
		return std::pair<LGraph::edge_descriptor,bool>(LGraph::edge_descriptor(), false);
	return get_edge((*this)[i - 1], (*this)[i], g);
}

CFMUAutoroute45::LEdgeAdd::LEdgeAdd(LGraph::vertex_descriptor v0, LGraph::vertex_descriptor v1, double metric)
	: m_v0(v0), m_v1(v1), m_metric(metric)
{
}

bool CFMUAutoroute45::LEdgeAdd::operator<(const LEdgeAdd& x) const
{
	if (get_v0() < x.get_v0())
		return true;
	if (get_v0() > x.get_v0())
		return false;
	return get_v1() < x.get_v1();
}

void CFMUAutoroute45::clearlgraph(void)
{
	m_lgraph.clear();
	m_lverticesidentmap.clear();
	m_lgraphairwaytable.clear();
	m_lgraphairwayrevtable.clear();
	m_solutionvertices.clear();
	m_vertexdep = m_vertexdest = 0;
	lgraphmodified();
}

CFMUAutoroute45::lgraphairwayindex_t CFMUAutoroute45::lgraphairway2index(const std::string& awy, bool create)
{
	std::string awyu(Glib::ustring(awy).uppercase());
	if (awyu.empty())
		return airwaydct;
	if (create) {
		std::pair<lgraphairwayrevtable_t::iterator,bool> ins(m_lgraphairwayrevtable.insert(std::make_pair(awyu, m_lgraphairwaytable.size())));
		if (ins.second)
			m_lgraphairwaytable.push_back(awyu);
		return ins.first->second;
	}
	lgraphairwayrevtable_t::const_iterator i(m_lgraphairwayrevtable.find(awyu));
	if (i == m_lgraphairwayrevtable.end())
		return airwaymatchnone;
	return i->second;
}

const std::string& CFMUAutoroute45::lgraphindex2airway(lgraphairwayindex_t idx, bool specialnames)
{
	static const std::string empty;
	switch (idx) {
	case airwaydct:
		if (specialnames) {
			static const std::string r("DCT");
			return r;
		}
		return empty;

	case airwaysid:
		if (specialnames) {
			static const std::string r("SID");
			return r;
		}
		return empty;

	case airwaystar:
		if (specialnames) {
			static const std::string r("STAR");
			return r;
		}
		return empty;

	case airwaymatchall:
	{
		static const std::string r("[MATCHALL]");
		return r;
	}

	case airwaymatchnone:
	{
		static const std::string r("[MATCHNONE]");
		return r;
	}

	case airwaymatchdctawy:
	{
		static const std::string r("[MATCHDCTAWY]");
		return r;
	}

	case airwaymatchdctawysidstar:
	{
		static const std::string r("[MATCHDCTAWYSIDSTAR]");
		return r;
	}

	case airwaymatchawysidstar:
	{
		static const std::string r("[MATCHAWYSIDSTAR]");
		return r;
	}

	case airwaymatchsidstar:
	{
		static const std::string r("[MATCHSIDSTAR]");
		return r;
	}

	default:
		break;
	}
	if (idx < m_lgraphairwaytable.size())
		return m_lgraphairwaytable[idx];
	{
		static const std::string r("[?""?]");
		return r;
	}
}

CFMUAutoroute45::LGraph::vertex_descriptor CFMUAutoroute45::lgraphaddvertex(Engine::DbObject::ptr_t obj)
{
	if (!obj)
		throw std::runtime_error("LGraph Vertex: object null");
	LGraph::vertex_descriptor v(boost::add_vertex(LVertex(obj), m_lgraph));
	const LVertex& vv(m_lgraph[v]);
	{
		const std::string& id(vv.get_ident());
		if (!id.empty())
			m_lverticesidentmap.insert(std::make_pair(id, v));
	}
	return v;
}

void CFMUAutoroute45::lgraphaddedge(LGraph& g, LEdge& edge, LGraph::vertex_descriptor u, LGraph::vertex_descriptor v, setmetric_t setmetric, bool log)
{
	if (!edge.is_valid())
		return;
	if (!edge.is_match(airwaymatchdctawysidstar))
		return;
	const unsigned int pis(m_performance.size());
	bool hasawy[pis];
	for (unsigned int pi = 0; pi < pis; ++pi)
		hasawy[pi] = false;
	LGraph::edge_descriptor eedge, edct;
	bool foundedge(false), founddct(false);
	{
		const LVertex& uu(g[u]);
		const LVertex& vv(g[v]);
		bool first(true);
		LGraph::out_edge_iterator e0, e1;
		for (boost::tie(e0, e1) = boost::out_edges(u, g); e0 != e1; ++e0) {
			if (boost::target(*e0, g) != v)
				continue;
			LEdge& ee(g[*e0]);
			if (first) {
				edge.set_dist(ee.get_dist());
				edge.set_truetrack(ee.get_truetrack());
				first = false;
			}
			if (ee.is_match(edge.get_ident())) {
				foundedge = true;
				eedge = *e0;
			}
			if (ee.is_dct()) {
				founddct = true;
				edct = *e0;
			}
			if (ee.is_match(airwaymatchawysidstar))
				for (unsigned int pi = 0; pi < pis; ++pi)
					if (ee.is_valid(pi))
						hasawy[pi] = true;
		}
		if (first) {
			edge.set_dist(uu.get_coord().spheric_distance_nmi(vv.get_coord()));
		        edge.set_truetrack(uu.get_coord().spheric_true_course(vv.get_coord()));
		}
		for (unsigned int pi = 0; pi < pis; ++pi) {
			if (!edge.is_valid(pi))
				continue;
			if (!edge.is_dct()) {
				hasawy[pi] = true;
			} else if (hasawy[pi]) {
				edge.clear_metric(pi);
				continue;
			}
			if (setmetric == setmetric_dist || setmetric == setmetric_metric) {
				float m(edge.get_dist());
				if (edge.is_dct()) {
					m *= get_dctpenalty();
					m += get_dctoffset();
				}
				if (setmetric == setmetric_metric) {
					const Performance::Cruise& cruise(m_performance.get_cruise(pi));
 					if (m_windenable) {
						Wind<double> wnd(cruise.get_wind(uu.get_coord().halfway(vv.get_coord())));
						wnd.set_crs_tas(edge.get_truetrack(), cruise.get_tas());
						if (wnd.get_gs() >= 0.1)
							m *= cruise.get_tas() / wnd.get_gs();
					}
					m *= cruise.get_metricpernmi();
				}
				edge.set_metric(pi, m);
			}
		}
	}
	if (foundedge) {
		LEdge& ee(g[eedge]);
		for (unsigned int pi = 0; pi < pis; ++pi) {
			if (!edge.is_valid(pi))
				continue;
			if (!ee.is_valid(pi) || edge.get_metric(pi) < ee.get_metric(pi)) {
				ee.set_metric(pi, edge.get_metric(pi));
				if (log)
					lgraphlogaddedge(g, eedge, pi);
			}
		}
	} else {
		boost::tie(eedge, foundedge) = boost::add_edge(u, v, edge, g);
		if (foundedge && log) {
			LEdge& ee(g[eedge]);
			for (unsigned int pi = 0; pi < pis; ++pi) {
				if (!ee.is_valid(pi))
					continue;
				lgraphlogaddedge(g, eedge, pi);
			}
		}
	}
	if (founddct) {
		LEdge& ee(g[edct]);
		for (unsigned int pi = 0; pi < pis; ++pi) {
			if (!hasawy[pi] || !ee.is_valid(pi))
				continue;
			if (log)
				lgraphlogkilledge(g, edct, pi);
			ee.clear_metric(pi);
		}
	}
}

void CFMUAutoroute45::lgraphkilledge(LGraph& g, LGraph::edge_descriptor e, unsigned int pi, bool log)
{
	LEdge& ee(g[e]);
	if (!ee.is_valid(pi))
		return;
	if (log)
		lgraphlogkilledge(g, e, pi);
	ee.clear_metric(pi);
}

void CFMUAutoroute45::lgraphkillalledges(LGraph& g, LGraph::edge_descriptor e, bool log)
{
	LEdge& ee(g[e]);
	const unsigned int pis(m_performance.size());
	for (unsigned int pi = 0; pi < pis; ++pi) {
		if (!ee.is_valid(pi))
			continue;
		if (log)
			lgraphlogkilledge(g, e, pi);
		ee.clear_metric(pi);
	}
}

CFMUAutoroute45::LEdge CFMUAutoroute45::lgraphbestedgemetric(LGraph::vertex_descriptor u, LGraph::vertex_descriptor v, lgraphairwayindex_t awy)
{
	const unsigned int pis(m_performance.size());
	LEdge metric(pis, awy);
	bool first(true);
	LGraph::out_edge_iterator e0, e1;
        for (boost::tie(e0, e1) = boost::out_edges(u, m_lgraph); e0 != e1; ++e0) {
                if (boost::target(*e0, m_lgraph) != v)
                        continue;
                const LEdge& ee(m_lgraph[*e0]);
		if (first) {
			metric.set_dist(ee.get_dist());
			metric.set_truetrack(ee.get_truetrack());
			first = false;
		}
		if (!ee.is_match(awy))
			continue;
		for (unsigned int pi = 0; pi < pis; ++pi) {
			if (!ee.is_valid(pi))
				continue;
			float m(ee.get_metric(pi));
			if (!metric.is_valid(pi) || m < metric.get_metric(pi))
				metric.set_metric(pi, m);
		}
	}
	return metric;
}

bool CFMUAutoroute45::lgraphkillemptyedges(void)
{
	bool work(false);
	LGraph::vertex_iterator vi, ve;
	for (boost::tie(vi, ve) = boost::vertices(m_lgraph); vi != ve; ++vi) {
		for (;;) {
			bool done(true);
			LGraph::out_edge_iterator e0, e1;
			for (boost::tie(e0, e1) = boost::out_edges(*vi, m_lgraph); e0 != e1;) {
				LGraph::out_edge_iterator ex(e0);
				++e0;
				LEdge& ee(m_lgraph[*ex]);
				if (ee.is_valid())
					continue;
				remove_edge(ex, m_lgraph);
				work = true;
				if (lgraphrestartedgescan) {
					done = false;
					break;
				}
			}
			if (done)
				break;
		}
	}
	return work;
}

bool CFMUAutoroute45::lgraphload(const Rect& bbox)
{
	typedef Engine::AirwayGraphResult::Graph Graph;
	typedef Engine::AirwayGraphResult::Vertex Vertex;
	typedef Engine::AirwayGraphResult::Edge Edge;

	m_signal_log(log_precompgraph, "");
	if (true) {
		std::ostringstream oss;
		oss << "Loading Airway graph: [" << bbox.get_southwest().get_lat_str2() << ' ' << bbox.get_southwest().get_lon_str2()
		    << ' ' << bbox.get_northeast().get_lat_str2() << ' ' << bbox.get_northeast().get_lon_str2() << ']';
		m_signal_log(log_debug0, oss.str());
	}
	Graph g;
	opendb();
	g.clear();
	{
		NavaidsDb::elementvector_t ev(m_navaiddb.find_by_rect(bbox, ~0, NavaidsDb::Navaid::subtables_none));
		for (NavaidsDb::elementvector_t::iterator i(ev.begin()); i != ev.end();) {
			switch (i->get_navaid_type()) {
			case NavaidsDb::Navaid::navaid_vor:
			case NavaidsDb::Navaid::navaid_vordme:
			case NavaidsDb::Navaid::navaid_ndb:
			case NavaidsDb::Navaid::navaid_ndbdme:
			case NavaidsDb::Navaid::navaid_vortac:
                        case NavaidsDb::Navaid::navaid_dme:
				++i;
				break;

			default:
				i = ev.erase(i);
				break;
			}
		}
		g.add(ev);
	}
	{
		WaypointsDb::elementvector_t ev(m_waypointdb.find_by_rect(bbox, ~0, WaypointsDb::Waypoint::subtables_none));
		for (WaypointsDb::elementvector_t::iterator i(ev.begin()); i != ev.end();) {
			switch (i->get_type()) {
			case WaypointsDb::Waypoint::type_icao:
				++i;
				break;

                        case WaypointsDb::Waypoint::type_other:
				if (i->get_icao().size() == 5) {
					++i;
					break;
				}
				i = ev.erase(i);
                                break;

			default:
				i = ev.erase(i);
				break;
			}
		}
		g.add(ev);
	}
	g.add(m_airwaydb.find_by_area(bbox, ~0, AirwaysDb::Airway::subtables_all));
	if (true) {
		{
			std::ostringstream oss;
			oss << "Airway area graph: [" << bbox.get_southwest().get_lat_str2() << ' ' << bbox.get_southwest().get_lon_str2()
			    << ' ' << bbox.get_northeast().get_lat_str2() << ' ' << bbox.get_northeast().get_lon_str2() << "], "
			    << boost::num_vertices(g) << " vertices, " << boost::num_edges(g) << " edges";
			m_signal_log(log_debug0, oss.str());
		}
		unsigned int counts[4];
		for (unsigned int i = 0; i < sizeof(counts)/sizeof(counts[0]); ++i)
			counts[i] = 0;
		Graph::vertex_iterator u0, u1;
		for (boost::tie(u0, u1) = boost::vertices(g); u0 != u1; ++u0) {
			const Vertex& uu(g[*u0]);
			unsigned int i(uu.get_type());
			if (i < sizeof(counts)/sizeof(counts[0]))
				++counts[i];
		}
		{
			std::ostringstream oss;
			oss << "Airway area graph: " << counts[Vertex::type_airport] << " airports, " << counts[Vertex::type_navaid] << " navaids, "
			    << counts[Vertex::type_intersection] << " intersections, " << counts[Vertex::type_mapelement] << " mapelements";
			m_signal_log(log_debug0, oss.str());
		}
	}
	typedef std::map<Graph::vertex_descriptor,LGraph::vertex_descriptor> vmap_t;
	vmap_t vmap;
	// create vertices
	typedef std::set<std::string> artobj_t;
	artobj_t artobj;
	Graph::vertex_iterator u0, u1;
	for (boost::tie(u0, u1) = boost::vertices(g); u0 != u1; ++u0) {
		const Vertex& uu(g[*u0]);
		if (uu.get_type() == Vertex::type_intersection && uu.is_ident_numeric())
			continue;
		Engine::DbObject::ptr_t obj(uu.get_object());
		if (!obj) {
			{
				WaypointsDb::Waypoint el;
				el.set_icao("");
				el.set_name(uu.get_ident());
				el.set_coord(uu.get_coord());
                                el.set_usage(WaypointsDb::Waypoint::usage_user);
                                el.set_type(WaypointsDb::Waypoint::type_other);
				Engine::DbObject::ptr_t p(Engine::DbObject::create(el));
				obj.swap(p);
			}
			artobj.insert(uu.get_ident());
		}
		if (false && !boost::out_degree(*u0, g) && !boost::in_degree(*u0, g))
			continue;
		vmap.insert(std::make_pair(*u0, lgraphaddvertex(obj)));
	}
	if (true) {
		std::ostringstream oss;
		oss << "lgraphload: creating artificial object for vertices ";
		bool subseq(false);
		for (artobj_t::const_iterator i(artobj.begin()), e(artobj.end()); i != e; ++i) {
			if (subseq)
				oss << ',';
			else
				subseq = true;
			oss << *i;
		}
		m_signal_log(log_debug0, oss.str());
	}
	// create edges
	const unsigned int pis(m_performance.size());
	Graph::edge_iterator e0, e1;
	for (boost::tie(e0, e1) = boost::edges(g); e0 != e1; ++e0) {
		Edge& ee(g[*e0]);
		LGraph::vertex_descriptor u, v;
		{
			vmap_t::const_iterator i(vmap.find(boost::source(*e0, g)));
			if (i == vmap.end())
				continue;
			u = i->second;
			i = vmap.find(boost::target(*e0, g));
			if (i == vmap.end())
				continue;
			v = i->second;
		}
		lgraphairwayindex_t awyname(lgraphairway2index(ee.get_ident(), true));
		const LVertex& uu(m_lgraph[u]);
		const LVertex& vv(m_lgraph[v]);
		if (u == v) {
			std::ostringstream oss;
			oss << "Routing Graph: refusing to add self-edge " << uu.get_ident();
			m_signal_log(log_debug0, oss.str());
			continue;
		}
		int minaltitude(std::numeric_limits<int>::min());
		int maxaltitude(std::numeric_limits<int>::max());
		if (ee.get_corridor5_elev() != AirwaysDb::Airway::nodata) {
			minaltitude = ee.get_corridor5_elev();
			if (minaltitude > 5000)
				minaltitude += 1000;
			minaltitude += 1000;
		}
		int mindctaltitude(minaltitude);
		if (get_honour_awy_levels()) {
			int mi(ee.get_base_level()), ma(ee.get_top_level());
			mi *= 100;
			ma *= 100;
			minaltitude = std::max(minaltitude, mi);
			maxaltitude = std::min(maxaltitude, ma);
		}
		TrafficFlowRestrictions::DctParameters dctrule(uu.get_ident(), uu.get_coord(), vv.get_ident(), vv.get_coord(),
							       mindctaltitude, m_performance.get_maxaltitude()+1);
		LEdge edgeawy(pis, awyname);
		LEdge edgedct(pis, airwaydct);
		bool dctcalc(!get_tfr_available_enabled());
		for (unsigned int pi = 0; pi < pis; ++pi) {
			const Performance::Cruise& cruise(m_performance.get_cruise(pi));
			if (cruise.get_altitude() < mindctaltitude)
				continue;
			bool outsideawyvert(cruise.get_altitude() < minaltitude || cruise.get_altitude() > maxaltitude);
			if (outsideawyvert || awyname == airwaydct) {
				if (!lgraphawyvertdct)
					continue;
				if (!dctcalc) {
					std::vector<TrafficFlowRestrictions::Message> msg;
					m_tfr.check_dct(msg, dctrule);
					for (unsigned int i = 0; i < msg.size(); ++i)
						logmessage(msg[i]);
					dctcalc = true;
				}
			}
			if (outsideawyvert) {
				if (!dctrule.get_alt().is_inside(0, cruise.get_altitude()))
					continue;
				edgedct.set_metric(pi, 1);
				continue;
			}
			if (awyname == airwaydct) {
				if (!dctrule.get_alt().is_inside(0, cruise.get_altitude()))
					continue;
				edgedct.set_metric(pi, 1);
				continue;
			}
			edgeawy.set_metric(pi, 1);
		}
		lgraphaddedge(edgeawy, u, v, setmetric_dist, false);
		lgraphaddedge(edgedct, u, v, setmetric_dist, false);
		if (false && (ee.get_ident() == "Y53" || ee.get_ident() == "L10")) {
			std::ostringstream oss;
			oss << "lgraphload: adding " << lgraphvertexname(u)
			    << ' ' << lgraphindex2airway(edgeawy.get_ident())
			    << ' ' << lgraphvertexname(v)
			    << " metric";
			for (unsigned int pi = 0; pi < pis; ++pi)
				oss << ' ' << edgeawy.get_metric(pi);
			m_signal_log(log_debug0, oss.str());
		}
	}
	lgraphmodified();
	{
		std::ostringstream oss;
		oss << "Routing Graph after airway population: " << boost::num_vertices(m_lgraph) << " V, " << boost::num_edges(m_lgraph) << " E";
		if (false)
			oss << ", descriptor sizes: vertex " << sizeof(LGraph::vertex_descriptor) << " edge " << sizeof(LGraph::edge_descriptor);
		m_signal_log(log_debug0, oss.str());
	}
	return true;
}

void CFMUAutoroute45::lgraphkillinvalidsuper(void)
{
	bool work(false);
	typedef std::set<std::string> invident_t;
	invident_t invident;
	{
		const unsigned int pis(m_performance.size());
		LGraph::vertex_iterator vi, ve;
		for (boost::tie(vi, ve) = boost::vertices(m_lgraph); vi != ve; ++vi) {
			const LVertex& vv(m_lgraph[*vi]);
			bool valid(true);
			if (!vv.is_ident_valid())
				valid = false;
			// see TrafficFlowRestrictions::load_airway_graph
			if (valid) {
				NavaidsDb::Navaid el;
				if (vv.get(el)) {
					switch (el.get_navaid_type()) {
					case NavaidsDb::Navaid::navaid_vor:
					case NavaidsDb::Navaid::navaid_vordme:
					case NavaidsDb::Navaid::navaid_ndb:
					case NavaidsDb::Navaid::navaid_ndbdme:
					case NavaidsDb::Navaid::navaid_vortac:
					case NavaidsDb::Navaid::navaid_dme:
						break;

					default:
						valid = false;
						break;
					}
				}
			}
			if (valid) {
				WaypointsDb::Waypoint el;
				if (vv.get(el)) {
					switch (el.get_type()) {
					case WaypointsDb::Waypoint::type_icao:
						break;

					case WaypointsDb::Waypoint::type_other:
						valid = el.get_icao().size() == 5;
						break;

					default:
						valid = false;
						break;
					}
				}
			}
			if (false && vv.get_ident().substr(0, 3) == "WAL") {
				std::ostringstream oss;
				oss << "lgraphkillinvalidsuper: " << lgraphvertexname(*vi) << ": " << (valid ? "" : "in") << "valid";
				m_signal_log(log_debug0, oss.str());
			}
			if (valid)
				continue;
			invident.insert(vv.get_ident());
			work = work || boost::out_degree(*vi, m_lgraph) || boost::in_degree(*vi, m_lgraph);
			LGraph::in_edge_iterator e0, z0;
			for(boost::tie(e0, z0) = boost::in_edges(*vi, m_lgraph); e0 != z0; ++e0) {
				if (boost::target(*e0, m_lgraph) != *vi || boost::source(*e0, m_lgraph) == *vi)
					continue;
				const LEdge& ee0(m_lgraph[*e0]);
				if (!ee0.is_match(airwaymatchawy))
					continue;
				LGraph::out_edge_iterator e1, z1;
				for(boost::tie(e1, z1) = boost::out_edges(*vi, m_lgraph); e1 != z1; ++e1) {
					if (boost::source(*e1, m_lgraph) != *vi || boost::target(*e1, m_lgraph) == *vi)
						continue;
					const LEdge& ee1(m_lgraph[*e1]);
					if (!ee1.is_match(ee0.get_ident()))
						continue;
					if (boost::source(*e0, m_lgraph) == boost::target(*e1, m_lgraph))
						continue;
					LEdge edge(pis, ee0.get_ident());
					for (unsigned int pi = 0; pi < pis; ++pi)
						edge.set_metric(pi, ee0.get_metric(pi) + ee1.get_metric(pi));
					lgraphaddedge(edge, boost::source(*e0, m_lgraph), boost::target(*e1, m_lgraph), setmetric_none, false);
					if (false) {
						std::ostringstream oss;
						oss << "lgraphkillinvalidsuper: avoiding " << lgraphvertexname(*vi)
						    << ": " << lgraphvertexname(boost::source(*e0, m_lgraph))
						    << ' ' << lgraphindex2airway(ee0.get_ident())
						    << ' ' << lgraphvertexname(boost::target(*e1, m_lgraph))
						    << " metric";
						for (unsigned int pi = 0; pi < pis; ++pi)
							oss << ' ' << edge.get_metric(pi);
						m_signal_log(log_debug0, oss.str());
					}
				}
			}
			boost::clear_vertex(*vi, m_lgraph);
		}
	}
	if (true) {
		std::ostringstream oss;
		oss << "lgraphkillinvalidsuper: avoiding ";
		bool subseq(false);
		for (invident_t::const_iterator i(invident.begin()), e(invident.end()); i != e; ++i) {
			if (subseq)
				oss << ',';
			else
				subseq = true;
			oss << *i;
		}
		m_signal_log(log_debug0, oss.str());
	}
	if (!work)
		return;
	lgraphmodified();
	{
		std::ostringstream oss;
		oss << "Routing Graph after invalid vertex removal: " << boost::num_vertices(m_lgraph)
		    << " V, " << boost::num_edges(m_lgraph) << " E";
		m_signal_log(log_debug0, oss.str());
	}
}

bool CFMUAutoroute45::lgraphcheckintersect(const Point& pt0, const Point& pt1, const Rect& bbox)
{
	return bbox.is_inside(pt0) || bbox.is_inside(pt1) || bbox.is_intersect(pt0, pt1);
}

bool CFMUAutoroute45::lgraphcheckintersect(const Point& pt0, const Point& pt1, const Rect& bbox, const MultiPolygonHole& poly)
{
	{
		Rect r(pt0, pt0);
		r = r.add(pt1);
		if (!r.is_intersect(bbox))
			return false;
	}
	return ((bbox.is_inside(pt0) && poly.windingnumber(pt0)) ||
		(bbox.is_inside(pt1) && poly.windingnumber(pt1)) ||
		poly.is_intersection(pt0, pt1));
}

void CFMUAutoroute45::lgraphexcluderegions(void)
{
	bool work(false);
	for (excluderegions_t::const_iterator ei(m_excluderegions.begin()), ee(m_excluderegions.end()); ei != ee; ++ei) {
		AirspacesDb::Airspace aspc;
		Rect bbox(ei->get_bbox());
		if (ei->is_airspace()) {
			AirspacesDb::elementvector_t ev(m_airspacedb.find_by_icao(ei->get_airspace_id(), 0, AirspacesDb::comp_exact, 0, AirspacesDb::Airspace::subtables_all));
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
				m_signal_log(log_debug0, oss.str());
				continue;
			}
			aspc = ev[eadidx];
			bbox = aspc.get_bbox();
			if (ev.size() > 1) {
				std::ostringstream oss;
				oss << "Exclude Regions: Multiple Airspaces " << ei->get_airspace_id() << '/' << ei->get_airspace_type()
				    << " found";
				m_signal_log(log_debug0, oss.str());
			}
		}
		const unsigned int pis(m_performance.size());
		unsigned int pi0(~0U), pi1(~0U);
		for (unsigned int pi = 0; pi < pis; ++pi) {
			const Performance::Cruise& cruise(m_performance.get_cruise(pi));
			if (cruise.get_altitude() < 100 * ei->get_minlevel() || cruise.get_altitude() > 100 * ei->get_maxlevel())
				continue;
			if (pi0 == ~0U)
				pi0 = pi;
			pi1 = pi;
		}
		{
			std::ostringstream oss;
			oss << "Exclude ";
			if (aspc.is_valid())
				oss << "Airspace " << aspc.get_icao() << '/' << aspc.get_class_string();
			else
				oss << "Region " << bbox;
			if (pi0 != ~0U)
				oss << ' ' << m_performance.get_cruise(pi0).get_fplalt()
				    << ".." << m_performance.get_cruise(pi1).get_fplalt();
			oss << " AWY limit " << ei->get_awylimit()
			    << " DCT limit " << ei->get_dctlimit()
			    << " offset " << ei->get_dctoffset()
			    << " scale " << ei->get_dctscale();
			m_signal_log(log_graphrule, oss.str());
		}
		if (pi0 == ~0U)
			continue;
		unsigned int edgeremoved(0), edgedct(0), edgeawy(0);
		{
			LGraph::vertex_iterator v0i, ve;
			for (boost::tie(v0i, ve) = boost::vertices(m_lgraph); v0i != ve; ++v0i) {
				const LVertex& vv0(m_lgraph[*v0i]);
				if (*v0i == m_vertexdep || *v0i == m_vertexdest)
					continue;
				LGraph::vertex_iterator v1i(v0i);
				for (++v1i; v1i != ve; ++v1i) {
					const LVertex& vv1(m_lgraph[*v1i]);
					if (*v1i == m_vertexdep || *v1i == m_vertexdest)
						continue;
					int inters(-1);
					LGraph::out_edge_iterator e0, e1;
					for (tie(e0, e1) = out_edges(*v0i, m_lgraph); e0 != e1; ++e0) {
						if (target(*e0, m_lgraph) != *v1i)
							continue;
						if (inters == -1) {
							if (aspc.is_valid())
								inters = lgraphcheckintersect(vv0.get_coord(), vv1.get_coord(), bbox, aspc.get_polygon());
							else
								inters = lgraphcheckintersect(vv0.get_coord(), vv1.get_coord(), bbox);
							if (!inters)
								break;
						}
						LEdge& ee(m_lgraph[*e0]);
						for (unsigned int pi = pi0; pi <= pi1; ++pi) {
							if (!ee.is_valid(pi))
								continue;
							if (false) {
								std::ostringstream oss;
								oss << "Airspace Segment: " << lgraphvertexname(source(*e0, m_lgraph), pi)
								    << ' ' << lgraphindex2airway(ee.get_ident(), true)
								    << ' ' << lgraphvertexname(target(*e0, m_lgraph), pi)
								    << " metric " << ee.get_metric(pi) << " limit " << (ee.is_dct() ? ei->get_dctlimit() : ei->get_awylimit());
								m_signal_log(log_debug0, oss.str());
							}
							if (ee.is_dct()) {
								if (ee.get_metric(pi) <= ei->get_dctlimit()) {
									ee.set_metric(pi, ee.get_metric(pi) * ei->get_dctscale() + ei->get_dctoffset());
									++edgedct;
									continue;
								}
							} else {
								if (ee.get_metric(pi) <= ei->get_awylimit()) {
									++edgeawy;
									continue;
								}
							}
							if (false) {
								std::ostringstream oss;
								oss << "Deleting Airspace Segment: " << lgraphvertexname(source(*e0, m_lgraph), pi)
								    << ' ' << lgraphindex2airway(ee.get_ident(), true)
								    << ' ' << lgraphvertexname(target(*e0, m_lgraph), pi)
								    << " metric " << ee.get_metric(pi) << " limit " << (ee.is_dct() ? ei->get_dctlimit() : ei->get_awylimit());
								m_signal_log(log_debug0, oss.str());
							}
							ee.set_metric(pi, LEdge::invalidmetric);
						}
					}
					if (!inters)
						break;
					for (tie(e0, e1) = out_edges(*v1i, m_lgraph); e0 != e1; ++e0) {
						if (target(*e0, m_lgraph) != *v0i)
							continue;
						if (inters == -1) {
							if (aspc.is_valid())
								inters = lgraphcheckintersect(vv0.get_coord(), vv1.get_coord(), bbox, aspc.get_polygon());
							else
								inters = lgraphcheckintersect(vv0.get_coord(), vv1.get_coord(), bbox);
							if (!inters)
								break;
						}
						LEdge& ee(m_lgraph[*e0]);
						for (unsigned int pi = pi0; pi <= pi1; ++pi) {
							if (!ee.is_valid(pi))
								continue;
							if (false) {
								std::ostringstream oss;
								oss << "Airspace Segment: " << lgraphvertexname(source(*e0, m_lgraph), pi)
								    << ' ' << lgraphindex2airway(ee.get_ident(), true)
								    << ' ' << lgraphvertexname(target(*e0, m_lgraph), pi)
								    << " metric " << ee.get_metric(pi) << " limit " << (ee.is_dct() ? ei->get_dctlimit() : ei->get_awylimit());
								m_signal_log(log_debug0, oss.str());
							}
							if (ee.is_dct()) {
								if (ee.get_metric(pi) <= ei->get_dctlimit()) {
									ee.set_metric(pi, ee.get_metric(pi) * ei->get_dctscale() + ei->get_dctoffset());
									++edgedct;
									continue;
								}
							} else {
								if (ee.get_metric(pi) <= ei->get_awylimit()) {
									++edgeawy;
									continue;
								}
							}
							if (false) {
								std::ostringstream oss;
								oss << "Deleting Airspace Segment: " << lgraphvertexname(source(*e0, m_lgraph), pi)
								    << ' ' << lgraphindex2airway(ee.get_ident(), true)
								    << ' ' << lgraphvertexname(target(*e0, m_lgraph), pi)
								    << " metric " << ee.get_metric(pi) << " limit " << (ee.is_dct() ? ei->get_dctlimit() : ei->get_awylimit());
								m_signal_log(log_debug0, oss.str());
							}
							ee.set_metric(pi, LEdge::invalidmetric);
						}
					}
				}
			}
		}
		{
			std::ostringstream oss;
			oss << "Exclude ";
			if (aspc.is_valid())
				oss << "Airspace " << aspc.get_icao() << '/' << aspc.get_class_string();
			else
				oss << "Region " << bbox;
			if (pi0 != ~0U)
				oss << ' ' << m_performance.get_cruise(pi0).get_fplalt()
				    << ".." << m_performance.get_cruise(pi1).get_fplalt();
			oss << " AWY limit " << ei->get_awylimit()
			    << " DCT limit " << ei->get_dctlimit()
			    << " offset " << ei->get_dctoffset()
			    << " scale " << ei->get_dctscale()
			    << " edges removed " << edgeremoved
			    << " dct " << edgedct
			    << " awy " << edgeawy;
			m_signal_log(log_normal, oss.str());
		}
	}
	lgraphmodified();
	{
		std::ostringstream oss;
		oss << "Routing Graph after exclude regions: " << boost::num_vertices(m_lgraph) << " V, " << boost::num_edges(m_lgraph) << " E";
		m_signal_log(log_debug0, oss.str());
	}
}

class CFMUAutoroute45::LevelEdgeWeightMap
{
public:
	typedef LGraph::edge_descriptor key_type;
	typedef double value_type;
	typedef double reference;
	typedef boost::lvalue_property_map_tag   category;

	LevelEdgeWeightMap(const LGraph& g, unsigned int pi, lgraphairwayindex_t awy = airwaymatchall,
			   const Rect& bbox = Rect(Point(Point::lon_min, Point::lon_min), Point(Point::lon_max, Point::lon_max)))
		: m_g(g), m_bbox(bbox), m_pi(pi), m_awy(awy) {}
	reference operator[](const key_type& k) const {
		const LEdge& ee(m_g[k]);
		if (!ee.is_match(m_awy))
			return std::numeric_limits<value_type>::max();
		const LVertex& uu(m_g[boost::source(k, m_g)]);
		if (!m_bbox.is_inside(uu.get_coord()))
			return std::numeric_limits<value_type>::max();
		const LVertex& vv(m_g[boost::target(k, m_g)]);
		if (!m_bbox.is_inside(vv.get_coord()))
			return std::numeric_limits<value_type>::max();
		return ee.get_metric(m_pi);
	}

protected:
	const LGraph& m_g;
	Rect m_bbox;
	unsigned int m_pi;
	lgraphairwayindex_t m_awy;
};	

inline double get(const CFMUAutoroute45::LevelEdgeWeightMap& wm, CFMUAutoroute45::LGraph::edge_descriptor e)
{
	return wm[e];
}

void CFMUAutoroute45::lgraphadddct(void)
{
	if (get_dctlimit() <= 0)
		return;
	const unsigned int pis(m_performance.size());
	const unsigned int numvert(boost::num_vertices(m_lgraph));
	unsigned int count_distpair(0);
	unsigned int count_dctseg(0);
	unsigned int count_total_dctseg(0);
	unsigned int count_awy(0);
	unsigned int count_dct(0);
	unsigned int count_dctrules(0);
	unsigned int count_uncached(0);
	unsigned int count_cached(0);

	Rect dctbbox(get_bbox().oversize_nmi(100.f));
	DctCache dctcache("", m_tfr.get_created());
	std::vector<DctCache::pointid_t> dctpointids;
	const bool have_dctcache(true);
	if (have_dctcache) {
		Glib::TimeVal tv;
		tv.assign_current_time();
		dctcache.load(dctbbox);
		{
			Glib::TimeVal tv1;
			tv1.assign_current_time();
			tv = tv1 - tv;
		}
		{
			std::ostringstream oss;
			oss << "loaded DCT cache, " << dctcache.get_nrpoints() << " points, " << dctcache.get_nrdcts() << " DCTs, area ["
			    << dctbbox.get_southwest().get_lat_str2() << ' ' << dctbbox.get_southwest().get_lon_str2() << ' '
			    << dctbbox.get_northeast().get_lat_str2() << ' ' << dctbbox.get_northeast().get_lon_str2()
			    << "], " << std::fixed << std::setprecision(3) << tv.as_double() << 's';
			m_signal_log(log_normal, oss.str());
		}
		tv.assign_current_time();
		dctpointids.resize(boost::num_vertices(m_lgraph), 0);
		LGraph::vertex_iterator vi, ve;
		for (boost::tie(vi, ve) = boost::vertices(m_lgraph); vi != ve; ++vi) {
			const LVertex& vv(m_lgraph[*vi]);
			if (!vv.is_ident_valid())
				continue;
			if (!dctbbox.is_inside(vv.get_coord()))
				continue;
			dctpointids[*vi] = dctcache.find_point(vv.get_ident(), vv.get_coord());
		}
		{
			Glib::TimeVal tv1;
			tv1.assign_current_time();
			tv = tv1 - tv;
		}
		{
			std::ostringstream oss;
			oss << "allocated DCT cache points, " << dctcache.get_nrpoints() << " points, "
			    << std::fixed << std::setprecision(3) << tv.as_double() << 's';
			m_signal_log(log_normal, oss.str());
		}
	}
	double maxdctdist(0);
	{
		typedef TrafficFlowRestrictions::DctSegments::DctSegment DctSegment;
		typedef std::set<DctSegment> dctseg_t;
		dctseg_t dctseg;
		double maxdctsegdist(0);
		if (true) {
			TrafficFlowRestrictions::DctSegments dctseg1(m_tfr.get_dct_segments());
			for (unsigned int i = 0, sz = dctseg1.size(); i < sz; ++i) {
				std::pair<dctseg_t::iterator,bool> ins(dctseg.insert(dctseg1[i]));
				if (!ins.second)
					continue;
				double dist((*ins.first)[0].get_coord().spheric_distance_nmi_dbl((*ins.first)[1].get_coord()));
				maxdctsegdist = std::max(maxdctsegdist, dist);
			}
			count_total_dctseg = dctseg.size();
			maxdctsegdist += 10;
		}
		if (false) {
			for (dctseg_t::const_iterator i(dctseg.begin()), e(dctseg.end()); i != e; ++i) {
				std::ostringstream oss;
				oss << "TFR DCT " << (*i)[0].get_ident() << " (" << (*i)[0].get_coord().get_lat_str2() << ' '
				    << (*i)[0].get_coord().get_lon_str2() << ") -> " << (*i)[1].get_ident() << " ("
				    << (*i)[1].get_coord().get_lat_str2() << ' ' << (*i)[1].get_coord().get_lon_str2() << ')';
				m_signal_log(log_debug0, oss.str());
			}
		}
		LGraph::vertex_iterator v0i, ve;
		for (boost::tie(v0i, ve) = boost::vertices(m_lgraph); v0i != ve; ++v0i) {
			const LVertex& vv0(m_lgraph[*v0i]);
			if (!vv0.is_ident_valid())
				continue;
			if (!dctbbox.is_inside(vv0.get_coord()))
				continue;
			LGraph::vertex_iterator v1i(v0i);
			for (++v1i; v1i != ve; ++v1i) {
				const LVertex& vv1(m_lgraph[*v1i]);
				if (!vv1.is_ident_valid())
					continue;
				if (vv0.get_ident() == vv1.get_ident())
					continue;
				if (!dctbbox.is_inside(vv1.get_coord()))
					continue;
				double dist(vv0.get_coord().spheric_distance_nmi_dbl(vv1.get_coord()));
				if (dist > get_dctlimit()) {
					bool found(false);
					if (dist > maxdctsegdist)
						continue;
					DctSegment dct(vv0.get_ident(), Point(Point::lon_min, Point::lon_min),
						       Engine::AirwayGraphResult::Vertex::type_undefined, false, false,
						       vv1.get_ident(), Point(Point::lon_min, Point::lon_min),
						       Engine::AirwayGraphResult::Vertex::type_undefined, false, false);
					dctseg_t::const_iterator i(dctseg.lower_bound(dct));
					dctseg_t::const_iterator e(dctseg.end());
					for (; i != e; ++i) {
						if (false) {
							std::ostringstream oss;
							oss << "TFR DCT " << vv0.get_ident() << " -> " << vv1.get_ident() << ": " << std::fixed
							    << (*i)[0].get_ident() << " (" << (*i)[0].get_coord().simple_distance_nmi(vv0.get_coord()) << ") -> "
							    << (*i)[1].get_ident() << " (" << (*i)[1].get_coord().simple_distance_nmi(vv1.get_coord()) << ')';
							m_signal_log(log_debug0, oss.str());
						}
						if ((*i)[0].get_ident() != vv0.get_ident() ||
						    (*i)[1].get_ident() != vv1.get_ident())
							break;
						if ((*i)[0].get_coord().simple_distance_nmi(vv0.get_coord()) > 0.1 ||
						    (*i)[1].get_coord().simple_distance_nmi(vv1.get_coord()) > 0.1)
							continue;
						found = true;
						if (true) {
							std::ostringstream oss;
							oss << "TFR DCT " << vv0.get_ident() << " -> " << vv1.get_ident() << " dist " << std::fixed << dist;
							m_signal_log(log_debug0, oss.str());
						}
						break;
					}
					if (!found)
						continue;
					++count_dctseg;
				}
				++count_distpair;
				if (true && !(count_distpair & ((1 << 12) - 1))) {
					std::ostringstream oss;
					oss << "DCT: processed " << count_distpair << " pairs";
					m_signal_log(log_debug0, oss.str());
				}
				int minalt(0);
				TrafficFlowRestrictions::DctParameters dctrule(vv0.get_ident(), vv0.get_coord(), vv1.get_ident(), vv1.get_coord(),
									       m_performance.get_minaltitude(), m_performance.get_maxaltitude()+1);
				bool dctcachefound(false);
				if (have_dctcache) {
					dctcachefound = true;
					const DctCache::DctAlt *a;
					a = dctcache.find_dct(dctpointids[*v0i], dctpointids[*v1i]);
					if (a)
						dctrule.get_alt(0) = a->get_alt();
					else
						dctcachefound = false;
					a = dctcache.find_dct(dctpointids[*v1i], dctpointids[*v0i]);
					if (a)
						dctrule.get_alt(1) = a->get_alt();
					else
						dctcachefound = false;
					if (dctcachefound)
						++count_cached;
					else
						++count_uncached;
				}
				if (true && !dctcachefound && get_tfr_available_enabled()) {
					std::vector<TrafficFlowRestrictions::Message> msg;
					m_tfr.check_dct(msg, dctrule, false && (vv0.get_ident() == "BAGSO" || vv1.get_ident() == "RAMOX"));
					for (unsigned int i = 0; i < msg.size(); ++i)
						logmessage(msg[i]);
					if (false && (vv0.get_ident() == "BAGSO" || vv0.get_ident() == "RAMOX" ||
						      vv1.get_ident() == "BAGSO" || vv1.get_ident() == "RAMOX")) {
						std::ostringstream oss;
						oss << "DCT: rules: " << vv0.get_ident() << ' ' << vv1.get_ident()
						    << " alt " << dctrule.get_alt(0).to_str() << " / " << dctrule.get_alt(1).to_str();
						if (dctrule.get_alt().is_empty())
							oss << " (E)";
						m_signal_log(log_debug0, oss.str());
					}
				}
				if (dctrule.get_alt().is_empty()) {
					if (have_dctcache && !dctcachefound) {
						dctcache.add_dct(dctpointids[*v0i], dctpointids[*v1i], dctrule.get_alt(0));
						dctcache.add_dct(dctpointids[*v1i], dctpointids[*v0i], dctrule.get_alt(1));
					}
					count_dctrules += 2 * m_performance.size();
					continue;
				}
				if (true && !dctcachefound) {
					TopoDb30::Profile prof(m_topodb.get_profile(vv0.get_coord(), vv1.get_coord(), 5));
					if (prof.empty()) {
						std::ostringstream oss;
						oss << "DCT: Error computing ground clearance: " << vv0.get_ident() << ' '
						    << vv1.get_ident() << " dist " << std::fixed << std::setprecision(1) << dist << "nmi";
						m_signal_log(log_debug0, oss.str());
					} else {
						TopoDb30::elev_t el(prof.get_maxelev());
						if (el != TopoDb30::nodata) {
							minalt = prof.get_maxelev() * Point::m_to_ft;
							if (minalt >= 5000)
								minalt += 1000;
							minalt += 1000;
						}
					}
				}
				if (have_dctcache && !dctcachefound) {
					DctCache::set_t::Intvl malt(minalt, std::numeric_limits<DctCache::set_t::type_t>::max());
					dctrule.get_alt(0) &= malt;
					dctrule.get_alt(1) &= malt;
					dctcache.add_dct(dctpointids[*v0i], dctpointids[*v1i], dctrule.get_alt(0));
					dctcache.add_dct(dctpointids[*v1i], dctpointids[*v0i], dctrule.get_alt(1));
				}
				LEdge edgeawy0(lgraphbestedgemetric(*v0i, *v1i, airwaymatchawysidstar));
				LEdge edgeawy1(lgraphbestedgemetric(*v1i, *v0i, airwaymatchawysidstar));
				LEdge edge0(pis, airwaydct);
				LEdge edge1(pis, airwaydct);
				for (unsigned int pi = 0; pi < pis; ++pi) {
					const Performance::Cruise& cruise(m_performance.get_cruise(pi));
					if (cruise.get_altitude() < minalt)
						continue;
					if (false && ((vv0.get_ident() == "BAGSO" && vv1.get_ident() == "RAMOX") ||
						      (vv0.get_ident() == "RAMOX" && vv1.get_ident() == "BAGSO"))) {
						std::ostringstream oss;
						oss << "DCT: " << vv0.get_ident() << ' ' << vv1.get_ident()
						    << " alt " << cruise.get_altitude() << " dist " << dist << " awy "
						    << edgeawy0.get_metric(pi) << " / " << edgeawy1.get_metric(pi)
						    << " dctrule " << (int)dctrule.get_alt().is_inside(0, cruise.get_altitude())
						    << " / " << (int)dctrule.get_alt().is_inside(1, cruise.get_altitude())
						    << " metric " << dist;
						m_signal_log(log_debug0, oss.str());
					}
					// omit DCT if the airway distance is not (significantly) longer
					// i.e. prefer airways
					if (dctrule.get_alt().is_inside(0, cruise.get_altitude())) {
						if (!edgeawy0.is_valid(pi)) {
							edge0.set_metric(pi, dist);
							++count_dct;
							maxdctdist = std::max(maxdctdist, dist);
						}
					} else {
						++count_dctrules;
					}
					if (dctrule.get_alt().is_inside(1, cruise.get_altitude())) {
						if (!edgeawy1.is_valid(pi)) {
							edge1.set_metric(pi, dist);
							++count_dct;
							maxdctdist = std::max(maxdctdist, dist);
						}
					} else {
						++count_dctrules;
					}
				}
				lgraphaddedge(edge0, *v0i, *v1i, setmetric_none, false);
				lgraphaddedge(edge1, *v1i, *v0i, setmetric_none, false);
			}
		}
	}
	// zap DCT edges again that don't offer much benefit over an airway route
	if (false) {
		float maxdctdist1(maxdctdist + 10);
		LGraph::vertex_iterator vi, ve;
		for (boost::tie(vi, ve) = boost::vertices(m_lgraph); vi != ve; ++vi) {
			const LVertex& vvi(m_lgraph[*vi]);
			if (true && !(*vi & ((1 << 10) - 1))) {
				std::ostringstream oss;
				oss << "DCT: prefer airways node " << ((unsigned int)*vi);
				m_signal_log(log_debug0, oss.str());
			}
			for (unsigned int pi = 0; pi < pis; ++pi) {
				LevelEdgeWeightMap wmap(m_lgraph, pi, airwaymatchawysidstar, vvi.get_coord().simple_box_nmi(maxdctdist1));
				std::vector<LGraph::vertex_descriptor> predecessors;
				std::vector<double> distmap;
				LGraph::out_edge_iterator e0, e1;
				for (boost::tie(e0, e1) = boost::out_edges(*vi, m_lgraph); e0 != e1; ++e0) {
					LEdge& ee(m_lgraph[*e0]);
					if (!ee.is_dct())
						continue;
					if (predecessors.empty()) {
						predecessors.resize(boost::num_vertices(m_lgraph), 0);
						distmap.resize(boost::num_vertices(m_lgraph), 0);
						dijkstra_shortest_paths(m_lgraph, *vi, boost::weight_map(wmap).
									predecessor_map(&predecessors[0]).distance_map(&distmap[0]));
					}
					LGraph::vertex_descriptor vj(boost::target(*e0, m_lgraph));
					if (predecessors[vj] == vj)
						continue;
					if (distmap[vj] > 1.01 * ee.get_dist())
						continue;
					ee.clear_metric(pi);
					++count_awy;
					--count_dct;
				}
			}
		}
	}
	if (true) {
		for (unsigned int pi = 0; pi < pis; ++pi) {
			std::vector<std::vector<float> > awydist(numvert);
			for (unsigned int i = 0; i < numvert; ++i)
				awydist[i].resize(numvert, 0.0f);
			typedef boost::adjacency_list<boost::listS, boost::vecS, boost::directedS, boost::no_property, boost::property<boost::edge_weight_t, float> > DGraph;
			DGraph dg(boost::num_vertices(m_lgraph));
			boost::property_map<DGraph, boost::edge_weight_t>::type dgdistmap(boost::get(boost::edge_weight, dg));
			{
				LGraph::edge_iterator e0, e1;
				for (boost::tie(e0, e1) = boost::edges(m_lgraph); e0 != e1; ++e0) {
					const LEdge& ee(m_lgraph[*e0]);
					if (ee.is_dct())
						continue;
					if (!ee.is_valid(pi))
						continue;
					DGraph::edge_descriptor e;
					bool ok;
					boost::tie(e, ok) = add_edge((DGraph::vertex_descriptor)boost::source(*e0, m_lgraph),
								     (DGraph::vertex_descriptor)boost::target(*e0, m_lgraph), dg);
					if (!ok)
						continue;
					dgdistmap[e] = ee.get_metric(pi);
				}
			}
			if (!boost::johnson_all_pairs_shortest_paths(dg, awydist)) {
				m_signal_log(log_debug0, "DCT: airway distances contain negative cycles, ignoring");
				for (unsigned int j = 0; j < numvert; ++j)
					for (unsigned int k = 0; k < numvert; ++k)
						awydist[j][k] = std::numeric_limits<float>::max();
			}
			LGraph::vertex_iterator vi, ve;
			for (boost::tie(vi, ve) = boost::vertices(m_lgraph); vi != ve; ++vi) {
				LGraph::out_edge_iterator e0, e1;
				for (boost::tie(e0, e1) = boost::out_edges(*vi, m_lgraph); e0 != e1; ++e0) {
					LEdge& ee(m_lgraph[*e0]);
					if (!ee.is_dct())
						continue;
					LGraph::vertex_descriptor vj(boost::target(*e0, m_lgraph));
					if (awydist[*vi][vj] > 1.01 * ee.get_dist())
						continue;
					ee.clear_metric(pi);
					++count_awy;
					--count_dct;
				}
			}
		}
	}
	lgraphmodified();
	{
		std::ostringstream oss;
		oss << "Routing Graph after DCT population: " << boost::num_vertices(m_lgraph) << " V, " << boost::num_edges(m_lgraph) << " E, "
		    << count_distpair << " vertex pairs, " << count_dct << " DCT edges, " << count_awy << " AWY edges preferred, "
		    << count_dctseg << " oversize edges mandated by TFR rules (" << count_total_dctseg << " TFR DCT edges), "
		    << count_dctrules << " DCT edges prevented by TFR rules, max DCT distance " << std::fixed << std::setprecision(1) << maxdctdist;
		m_signal_log(log_debug0, oss.str());
	}
	if (have_dctcache) {
		Glib::TimeVal tv;
		tv.assign_current_time();
		dctcache.flush();
		{
			Glib::TimeVal tv1;
			tv1.assign_current_time();
			tv = tv1 - tv;
		}
		std::ostringstream oss;
		oss << "DCT cache flush: " << count_uncached << " uncached, " << count_cached << " cached, "
		    << dctcache.get_nrpoints() << " points, " << dctcache.get_nrdcts() << " DCTs, "
		    << std::fixed << std::setprecision(3) << tv.as_double() << 's';
		m_signal_log(log_debug0, oss.str());
	}
}

void CFMUAutoroute45::lgraphkilllongdct(void)
{
	bool work(false);
	const unsigned int pis(m_performance.size());
	LGraph::edge_iterator e0, e1;
	for (boost::tie(e0, e1) = boost::edges(m_lgraph); e0 != e1; ++e0) {
		LEdge& ee(m_lgraph[*e0]);
		if (!ee.is_dct())
			continue;
		if (ee.get_dist() <= get_dctlimit())
			continue;
		for (unsigned int pi = 0; pi < pis; ++pi) {
			work = work || ee.is_valid(pi);
			ee.set_metric(pi, LEdge::invalidmetric);
		}
	}
	if (work)
		lgraphmodified();
}

void CFMUAutoroute45::lgraphedgemetric(void)
{
	const unsigned int pis(m_performance.size());
     	LGraph::edge_iterator e0, e1;
	for (boost::tie(e0, e1) = boost::edges(m_lgraph); e0 != e1; ++e0) {
		LEdge& ee(m_lgraph[*e0]);
		if (!ee.is_match(airwaymatchdctawysidstar))
			continue;
		LGraph::vertex_descriptor u(boost::source(*e0, m_lgraph));
		LGraph::vertex_descriptor v(boost::target(*e0, m_lgraph));
		const LVertex& uu(m_lgraph[u]);
		const LVertex& vv(m_lgraph[v]);
		for (unsigned int pi = 0; pi < pis; ++pi) {
			if (!ee.is_valid(pi))
				continue;
			const Performance::Cruise& cruise(m_performance.get_cruise(pi));
			float m(ee.get_metric(pi));
			if (ee.is_dct()) {
				m *= get_dctpenalty();
				m += get_dctoffset();
			}
			if (m_windenable) {
				Wind<double> wnd(cruise.get_wind(uu.get_coord().halfway(vv.get_coord())));
				wnd.set_crs_tas(ee.get_truetrack(), cruise.get_tas());
				if (wnd.get_gs() >= 0.1)
					m *= cruise.get_tas() / wnd.get_gs();
			}
			m *= cruise.get_metricpernmi();
			if (false && ee.is_match(airwaymatchsidstar)) {
				std::ostringstream oss;
				oss << "lgraphedgemetric: " << lgraphvertexname(u, pi) << "--"
				    << lgraphindex2airway(ee.get_ident(), true) << "->"
				    << lgraphvertexname(v, pi) << " metric " << ee.get_metric(pi)
				    << " --> " << m;
				m_signal_log(log_debug0, oss.str());
			}
			ee.set_metric(pi, m);
		}
	}
	lgraphmodified();
}

bool CFMUAutoroute45::lgraphaddsidstar(void)
{
	if (!get_departure().is_valid() || !get_destination().is_valid())
		return false;
	m_vertexdep = lgraphaddvertex(Engine::DbObject::create(get_departure()));
	m_vertexdest = lgraphaddvertex(Engine::DbObject::create(get_destination()));
	LGraph::vertex_descriptor lastvertex(boost::num_vertices(m_lgraph));
	const LVertex& sdep(m_lgraph[m_vertexdep]);
	const LVertex& sdest(m_lgraph[m_vertexdest]);
        const unsigned int pis(m_performance.size());
	typedef std::set<LVertexDescEdge> ss_t;
	ss_t sidv, starv;
	ss_t sida, stara;
	{
	        LVertexDescEdge sidsv(pis, lastvertex), starsv(pis, lastvertex);
		double sidd(std::numeric_limits<double>::max()), stard(sidd);
		LGraph::vertex_iterator vi, ve;
		for (boost::tie(vi, ve) = boost::vertices(m_lgraph); vi != ve; ++vi) {
			if (*vi == m_vertexdep || *vi == m_vertexdest)
				continue;
			const LVertex& vv(m_lgraph[*vi]);
			if (!vv.is_ident_valid())
				continue;
			double depdist(vv.get_coord().spheric_distance_nmi_dbl(sdep.get_coord()));
			double deptt(sdep.get_coord().spheric_true_course(vv.get_coord()));
			bool depincircle(depdist <= get_sidlimit());
			depdist = std::max(depdist, m_airportconnminimum[0]);
			if (depincircle) {
				std::pair<ss_t::iterator,bool> ins(sidv.insert(LVertexDescEdge(pis, *vi, airwaysid, depdist, deptt)));
				ins.first->get_edge().set_all_metric(depdist + m_airportconnoffset[0]);
			}
			double destdist(vv.get_coord().spheric_distance_nmi_dbl(sdest.get_coord()));
			double desttt(vv.get_coord().spheric_true_course(sdest.get_coord()));
			bool destincircle(destdist <= get_starlimit());
			destdist = std::max(destdist, m_airportconnminimum[1]);
			if (destincircle) {
				std::pair<ss_t::iterator,bool> ins(starv.insert(LVertexDescEdge(pis, *vi, airwaystar, destdist, desttt)));
				ins.first->get_edge().set_all_metric(destdist + m_airportconnoffset[1]);
			}
			if (!get_sid().is_invalid()) {
				double dist(vv.get_coord().spheric_distance_nmi_dbl(get_sid()));
				if (dist < sidd) {
					if (false)
						std::cerr << "Manual SID: " << vv.get_ident() << ' ' << vv.get_coord().get_lat_str2()
							  << ' ' << vv.get_coord().get_lon_str2() << " dist " << dist << std::endl;
					sidd = dist;
					sidsv = LVertexDescEdge(pis, *vi, airwaysid, depdist, deptt);
					sidsv.get_edge().set_all_metric(depdist);
				}
			}
			if (!get_star().is_invalid()) {
				double dist(vv.get_coord().spheric_distance_nmi_dbl(get_star()));
				if (dist < stard) {
					if (false)
						std::cerr << "Manual STAR: " << vv.get_ident() << ' ' << vv.get_coord().get_lat_str2()
							  << ' ' << vv.get_coord().get_lon_str2() << " dist " << dist << std::endl;
					stard = dist;
					starsv = LVertexDescEdge(pis, *vi, airwaystar, destdist, desttt);
					starsv.get_edge().set_all_metric(destdist);
				}
			}
			if (get_siddb()) {
				for (unsigned int i = 0, j = get_departure().get_nrvfrrte(); i < j; ++i) {
					const AirportsDb::Airport::VFRRoute& rte(get_departure().get_vfrrte(i));
					if (!rte.size())
						continue;
					const AirportsDb::Airport::VFRRoute::VFRRoutePoint& rtept(rte[rte.size()-1]);
					if (rtept.get_pathcode() != AirportsDb::Airport::VFRRoute::VFRRoutePoint::pathcode_sid)
						continue;
					if (vv.get_coord().spheric_distance_nmi_dbl(rtept.get_coord()) > 0.1)
						continue;
					std::pair<ss_t::iterator,bool> ins(sida.insert(LVertexDescEdge(pis, *vi, airwaysid, depdist, deptt)));
					if (!(get_departure().get_flightrules() & (AirportsDb::Airport::flightrules_dep_vfr |
										   AirportsDb::Airport::flightrules_dep_ifr)) ||
					    rte.get_minalt() < 0 || rte.get_maxalt() < 0) {
						ins.first->get_edge().set_all_metric(depdist);
						break;
					}
					for (unsigned int pi = 0; pi < pis; ++pi) {
						const Performance::Cruise& cruise(m_performance.get_cruise(pi));
						if (cruise.get_altitude() >= rte.get_minalt() && cruise.get_altitude() <= rte.get_maxalt())
							ins.first->get_edge().set_metric(pi, depdist);
					}
					break;
				}
			}
			if (get_stardb()) {
				for (unsigned int i = 0, j = get_destination().get_nrvfrrte(); i < j; ++i) {
					const AirportsDb::Airport::VFRRoute& rte(get_destination().get_vfrrte(i));
					if (!rte.size())
						continue;
					const AirportsDb::Airport::VFRRoute::VFRRoutePoint& rtept(rte[0]);
					if (rtept.get_pathcode() != AirportsDb::Airport::VFRRoute::VFRRoutePoint::pathcode_star)
						continue;
					if (vv.get_coord().spheric_distance_nmi_dbl(rtept.get_coord()) > 0.1)
						continue;
					std::pair<ss_t::iterator,bool> ins(stara.insert(LVertexDescEdge(pis, *vi, airwaystar, destdist, desttt)));
					if (!(get_destination().get_flightrules() & (AirportsDb::Airport::flightrules_arr_vfr |
										     AirportsDb::Airport::flightrules_arr_ifr)) ||
					    rte.get_minalt() < 0 || rte.get_maxalt() < 0) {
						ins.first->get_edge().set_all_metric(destdist);
						break;
					}
					for (unsigned int pi = 0; pi < pis; ++pi) {
						const Performance::Cruise& cruise(m_performance.get_cruise(pi));
						if (cruise.get_altitude() >= rte.get_minalt() && cruise.get_altitude() <= rte.get_maxalt())
							ins.first->get_edge().set_metric(pi, destdist);
					}
					break;
				}
			}
		}
		if (sidd > 0.1)
			sidsv.set_vertex(lastvertex);
		if (stard > 0.1)
			starsv.set_vertex(lastvertex);
		if (sidsv.get_vertex() != lastvertex) {
			sida.clear();
			sidv.clear();
			sida.insert(sidsv);
			const LVertex& sv(m_lgraph[sidsv.get_vertex()]);
			m_airportconnident[0] = sv.get_ident();
			if (sv.is_airport())
				m_airportconntype[0] = Engine::AirwayGraphResult::Vertex::type_airport;
			else if (sv.is_navaid())
				m_airportconntype[0] = Engine::AirwayGraphResult::Vertex::type_navaid;
			else if (sv.is_intersection())
				m_airportconntype[0] = Engine::AirwayGraphResult::Vertex::type_intersection;
			else if (sv.is_mapelement())
				m_airportconntype[0] = Engine::AirwayGraphResult::Vertex::type_mapelement;
		} else if (!get_sid().is_invalid()) {
			if (true) {
				std::ostringstream oss;
				oss << "IFR: SID point " << get_sid().get_lat_str2() << ' ' << get_sid().get_lon_str2() << " not found";
				m_signal_log(log_debug0, oss.str());
			}
			return false;
		}
		if (starsv.get_vertex() != lastvertex) {
			stara.clear();
			starv.clear();
			stara.insert(starsv);
			const LVertex& sv(m_lgraph[starsv.get_vertex()]);
			m_airportconnident[1] = sv.get_ident();
			if (sv.is_airport())
				m_airportconntype[1] = Engine::AirwayGraphResult::Vertex::type_airport;
			else if (sv.is_navaid())
				m_airportconntype[1] = Engine::AirwayGraphResult::Vertex::type_navaid;
			else if (sv.is_intersection())
				m_airportconntype[1] = Engine::AirwayGraphResult::Vertex::type_intersection;
			else if (sv.is_mapelement())
				m_airportconntype[1] = Engine::AirwayGraphResult::Vertex::type_mapelement;
		} else if (!get_star().is_invalid()) {
			if (true) {
				std::ostringstream oss;
				oss << "IFR: STAR point " << get_star().get_lat_str2() << ' ' << get_star().get_lon_str2() << " not found";
				m_signal_log(log_debug0, oss.str());
			}
			return false;
		}
	}
	for (ss_t::const_iterator svi(sida.begin()), sve(sida.end()); svi != sve; ++svi) {
		lgraphaddedge(svi->get_edge(), m_vertexdep, svi->get_vertex(), setmetric_none, false);
		ss_t::iterator i(sidv.find(LVertexDescEdge(pis, svi->get_vertex())));
		if (i == sidv.end())
			continue;
		for (unsigned int pi = 0; pi < pis; ++pi) {
			if (!svi->get_edge().is_valid(pi))
				continue;
			i->get_edge().clear_metric(pi);
		}
		if (!i->get_edge().is_valid())
			sidv.erase(i);
	}
	for (ss_t::const_iterator svi(stara.begin()), sve(stara.end()); svi != sve; ++svi) {
		lgraphaddedge(svi->get_edge(), svi->get_vertex(), m_vertexdest, setmetric_none, false);
		ss_t::iterator i(starv.find(LVertexDescEdge(pis, svi->get_vertex())));
		if (i == starv.end())
			continue;
		for (unsigned int pi = 0; pi < pis; ++pi) {
			if (!svi->get_edge().is_valid(pi))
				continue;
			i->get_edge().clear_metric(pi);
		}
		if (!i->get_edge().is_valid())
			starv.erase(i);
	}
	if (sida.empty() || (get_sidpenalty() < 1000. &&
			     !(get_departure().get_flightrules() & (AirportsDb::Airport::flightrules_dep_vfr |
								    AirportsDb::Airport::flightrules_dep_ifr)))) {
		double penalty(1), offset(0);
		if (!sida.empty()) {
			penalty = get_sidpenalty();
			offset = get_sidoffset();
		}
		const LVertex& vdep(m_lgraph[m_vertexdep]);
		for (ss_t::const_iterator svi(sidv.begin()), sve(sidv.end()); svi != sve; ++svi) {
			svi->get_edge().scale_metric(penalty, offset);
			lgraphaddedge(svi->get_edge(), m_vertexdep, svi->get_vertex(), setmetric_none, false);
		}
	}
	if (stara.empty() || (get_starpenalty() < 1000. &&
			      !(get_destination().get_flightrules() & (AirportsDb::Airport::flightrules_arr_vfr |
								       AirportsDb::Airport::flightrules_arr_ifr)))) {
		double penalty(1), offset(0);
		if (!stara.empty()) {
			penalty = get_starpenalty();
			offset = get_staroffset();
		}
		const LVertex& vdest(m_lgraph[m_vertexdest]);
		for (ss_t::const_iterator svi(starv.begin()), sve(starv.end()); svi != sve; ++svi) {
			svi->get_edge().scale_metric(penalty, offset);
			lgraphaddedge(svi->get_edge(), svi->get_vertex(), m_vertexdest, setmetric_none, false);
		}
	}
	lgraphmodified();
	{
		std::ostringstream oss;
		oss << "Routing Graph after SID/STAR population: " << boost::num_vertices(m_lgraph) << " V, " << boost::num_edges(m_lgraph) << " E";
		m_signal_log(log_debug0, oss.str());
	}
	return true;
}

void CFMUAutoroute45::lgraphapplycfmuintel(const Rect& bbox)
{
	m_cfmuintel.open(get_db_maindir(), get_db_auxdir());
	{
		sigc::slot<void,const CFMUIntel::ForbiddenPoint&> func(sigc::mem_fun(*this, &CFMUAutoroute45::lgraphapplycfmuintelpt));
		m_cfmuintel.find(bbox, func);
	}
	{
		sigc::slot<void,const CFMUIntel::ForbiddenSegment&> func(sigc::mem_fun(*this, &CFMUAutoroute45::lgraphapplycfmuintelseg));
		m_cfmuintel.find(bbox, func);
	}
	m_cfmuintel.close();
	lgraphkillemptyedges();
}

void CFMUAutoroute45::lgraphapplycfmuintelpt(const CFMUIntel::ForbiddenPoint& p)
{
	LGraph::vertex_descriptor v0(~0U);
	{
		double bestdist(std::numeric_limits<double>::max());
		for (LVerticesIdentMap::const_iterator i(m_lverticesidentmap.lower_bound(p.get_ident())),
			     e(m_lverticesidentmap.upper_bound(p.get_ident())); i != e; ++i) {
			LGraph::vertex_descriptor v(i->second);
			const LVertex& vv(m_lgraph[v]);
			double dist(vv.get_coord().simple_distance_nmi(p.get_pt()));
			if (!(dist < bestdist))
				continue;
			bestdist = dist;
			if (dist > 0.1)
				continue;
			v0 = v;
		}
	}
	if (v0 >= boost::num_edges(m_lgraph)) {
	       	if (true) {
			std::ostringstream oss;
			oss << "stored restrictions: vertex " << p.get_ident() << ' ' << p.get_pt().get_lat_str2()
			    << ' ' << p.get_pt().get_lon_str2() << " not found";
			m_signal_log(log_debug0, oss.str());
		}
		return;
	}
	boost::clear_vertex(v0, m_lgraph);
	if (true) {
		std::ostringstream oss;
		oss << "stored restrictions: killing vertex " << p.get_ident() << ' ' << p.get_pt().get_lat_str2()
		    << ' ' << p.get_pt().get_lon_str2();
		m_signal_log(log_debug0, oss.str());
	}
}

void CFMUAutoroute45::lgraphapplycfmuintelseg(const CFMUIntel::ForbiddenSegment& s)
{
	LGraph::vertex_descriptor v0(~0U), v1(~0U);
	{
		double bestdist(std::numeric_limits<double>::max());
		for (LVerticesIdentMap::const_iterator i(m_lverticesidentmap.lower_bound(s.get_fromident())),
			     e(m_lverticesidentmap.upper_bound(s.get_fromident())); i != e; ++i) {
			LGraph::vertex_descriptor v(i->second);
			const LVertex& vv(m_lgraph[v]);
			double dist(vv.get_coord().simple_distance_nmi(s.get_frompt()));
			if (!(dist < bestdist))
				continue;
			bestdist = dist;
			if (dist > 0.1)
				continue;
			v0 = v;
		}
	}
	if (v0 >= boost::num_edges(m_lgraph)) {
	       	if (false) {
			std::ostringstream oss;
			oss << "stored restrictions: vertex " << s.get_fromident() << ' ' << s.get_frompt().get_lat_str2()
			    << ' ' << s.get_frompt().get_lon_str2() << " not found";
			m_signal_log(log_debug0, oss.str());
		}
		return;
	}
	{
		double bestdist(std::numeric_limits<double>::max());
		for (LVerticesIdentMap::const_iterator i(m_lverticesidentmap.lower_bound(s.get_toident())),
			     e(m_lverticesidentmap.upper_bound(s.get_toident())); i != e; ++i) {
			LGraph::vertex_descriptor v(i->second);
			const LVertex& vv(m_lgraph[v]);
			double dist(vv.get_coord().simple_distance_nmi(s.get_topt()));
			if (!(dist < bestdist))
				continue;
			bestdist = dist;
			if (dist > 0.1)
				continue;
			v1 = v;
		}
	}
	if (v1 >= boost::num_edges(m_lgraph)) {
	       	if (false) {
			std::ostringstream oss;
			oss << "stored restrictions: vertex " << s.get_toident() << ' ' << s.get_topt().get_lat_str2()
			    << ' ' << s.get_topt().get_lon_str2() << " not found";
			m_signal_log(log_debug0, oss.str());
		}
		return;
	}
        const unsigned int pis(m_performance.size());
	unsigned int pi0(~0U), pi1(~0U);
        for (unsigned int pi = 0; pi < pis; ++pi) {
		const Performance::Cruise& cruise(m_performance.get_cruise(pi));
		if (cruise.get_altitude() == s.get_fromalt())
			pi0 = pi;
		if (cruise.get_altitude() == s.get_toalt())
			pi1 = pi;
	}
	if (pi0 >= pis) {
	       	if (false) {
			std::ostringstream oss;
			oss << "stored restrictions: vertex " << s.get_fromident() << ' ' << s.get_frompt().get_lat_str2()
			    << ' ' << s.get_frompt().get_lon_str2() << " altitude " << s.get_fromalt() << " not found";
			m_signal_log(log_debug0, oss.str());
		}
		return;
	}
	if (pi1 >= pis) {
	       	if (false) {
			std::ostringstream oss;
			oss << "stored restrictions: vertex " << s.get_toident() << ' ' << s.get_topt().get_lat_str2()
			    << ' ' << s.get_topt().get_lon_str2() << " altitude " << s.get_toalt() << " not found";
			m_signal_log(log_debug0, oss.str());
		}
		return;
	}
	lgraphairwayindex_t awy;
	{
		std::string id;
		awy = s.get_awyident(id);
		if (awy == airwaymatchawy)
			awy = lgraphairway2index(id, false);
	}
	if (!LEdge::is_match(awy, airwaymatchdctawysidstar)) {
		if (false) {
			std::ostringstream oss;
			oss << "stored restrictions: airway " << s.get_awyident() << " not found";
			m_signal_log(log_debug0, oss.str());
		}
		return;
	}
	if (pi0 != pi1) {
		if (true) {
			std::ostringstream oss;
			oss << "stored restrictions: ignoring level changes for " << lgraphvertexname(v0, pi0)
                            << ' ' << lgraphindex2airway(awy, true) << ' ' << lgraphvertexname(v1, pi1);
			m_signal_log(log_debug0, oss.str());
		}
		return;
	}
	if (v0 == m_vertexdep && !get_departure_ifr())
		return;
	if (v1 == m_vertexdest && !get_destination_ifr())
		return;
	if (false) {
		std::ostringstream oss;
		oss << "stored restrictions: killing " << lgraphvertexname(v0, pi0)
		    << ' ' << lgraphindex2airway(awy, true) << ' ' << lgraphvertexname(v1, pi1);
		m_signal_log(log_debug0, oss.str());
	}
	{
		bool first(true);
                LGraph::out_edge_iterator e0, e1;
                for (boost::tie(e0, e1) = boost::out_edges(v0, m_lgraph); e0 != e1; ++e0) {
                        if (boost::target(*e0, m_lgraph) != v1)
                                continue;
                        LEdge& ee(m_lgraph[*e0]);
			if (!ee.is_match(awy))
				continue;
			if (!ee.is_valid(pi0))
				continue;
			if (true) {
				std::ostringstream oss;
				oss << "stored restrictions: killing edge " << lgraphvertexname(v0, pi0)
				    << ' ' << lgraphindex2airway(ee.get_ident(), true) << ' ' << lgraphvertexname(v1, pi1);
				m_signal_log(log_debug0, oss.str());
			}
			lgraphkilledge(*e0, pi0, false);
		}
	}
}

void CFMUAutoroute45::lgraphcrossing(void)
{
	m_crossingmandatory.clear();
	const unsigned int pis(m_performance.size());
	for (crossing_t::iterator ci(m_crossing.begin()), ce(m_crossing.end()); ci != ce; ++ci) {
		if (ci->get_coord().is_invalid())
			continue;
		unsigned int pimin(~0U), pimax(0);
                for (unsigned int pi = 0; pi < pis; ++pi) {
                        const Performance::Cruise& cruise(m_performance.get_cruise(pi));
                        if (cruise.get_altitude() < 100 * ci->get_minlevel())
				continue;
			pimin = pi;
			break;
		}
                for (unsigned int pi = pis; pi > 0; ) {
                        const Performance::Cruise& cruise(m_performance.get_cruise(--pi));
 			if (cruise.get_altitude() > 100 * ci->get_maxlevel())
				continue;
			pimax = pi;
			break;
		}
		if (pimin > pimax) {
			std::ostringstream oss;
                        oss << "Crossing Point " << ci->get_coord().get_lat_str2() << ' ' << ci->get_coord().get_lon_str2()
			    << " F" << std::setw(3) << std::setfill('0') << ci->get_minlevel()
			    << "..F" << std::setw(3) << std::setfill('0') << ci->get_maxlevel()
			    << " R" << ci->get_radius()
			    << " levels cannot be satisfied, dropping";
                        m_signal_log(log_normal, oss.str());
			continue;
		}
		typedef std::multimap<double,LGraph::vertex_descriptor> vertexmap_t;
		vertexmap_t vertexmap;
		{
			LGraph::vertex_iterator vi, ve;
			std::string ident;
			Engine::AirwayGraphResult::Vertex::type_t type(Engine::AirwayGraphResult::Vertex::type_undefined);
			double iddist(std::numeric_limits<double>::max());
			for (boost::tie(vi, ve) = boost::vertices(m_lgraph); vi != ve; ++vi) {
				const LVertex& vv(m_lgraph[*vi]);
				double dist(vv.get_coord().spheric_distance_nmi_dbl(ci->get_coord()));
				if (dist > Crossing::maxradius)
					continue;
				vertexmap.insert(std::make_pair(dist, *vi));
				if (dist >= iddist)
					continue;
				iddist = dist;
				ident = vv.get_ident();
				if (vv.is_airport())
					type = Engine::AirwayGraphResult::Vertex::type_airport;
				else if (vv.is_navaid())
					type = Engine::AirwayGraphResult::Vertex::type_navaid;
				else if (vv.is_intersection())
					type = Engine::AirwayGraphResult::Vertex::type_intersection;
				else if (vv.is_mapelement())
					type = Engine::AirwayGraphResult::Vertex::type_mapelement;
				else 
					type = Engine::AirwayGraphResult::Vertex::type_undefined;
			}
			if (ci->get_ident().empty() && iddist <= 0.1) {
				ci->set_ident(ident);
				ci->set_type(type);
			}
		}
		if (vertexmap.empty()) {
			std::ostringstream oss;
                        oss << "Crossing Point " << ci->get_coord().get_lat_str2() << ' ' << ci->get_coord().get_lon_str2()
			    << " F" << std::setw(3) << std::setfill('0') << ci->get_minlevel()
			    << "..F" << std::setw(3) << std::setfill('0') << ci->get_maxlevel()
			    << " R" << ci->get_radius()
			    << " lies more than " << Crossing::maxradius << "nm from any significant point/navaid, dropping";
                        m_signal_log(log_normal, oss.str());
			continue;
		}
		LGMandatoryAlternative alt;
		{
			std::ostringstream oss;
			oss << "USER:" << ci->get_ident() << " F" << std::setw(3) << std::setfill('0') << ci->get_minlevel()
			    << "..F" << std::setw(3) << std::setfill('0') << ci->get_maxlevel()
			    << " R" << ci->get_radius();
			alt.set_rule(oss.str());
		}
		vertexmap_t::const_iterator vi(vertexmap.upper_bound(ci->get_radius()));
		if (vi == vertexmap.begin())
			++vi;
		for (vertexmap_t::const_iterator vi2(vertexmap.begin()); vi2 != vi; ++vi2) {
			LGMandatorySequence seq;
			seq.push_back(LGMandatoryPoint(vi2->second, pimin, pimax, airwaydct));
			alt.push_back(seq);
		}
		m_crossingmandatory.push_back(alt);
	}
	if (true) {
		m_signal_log(log_debug0, "Crossing Points");
		for (unsigned int i = 0; i < m_crossingmandatory.size(); ++i) {
			const LGMandatoryAlternative& alt(m_crossingmandatory[i]);
			{
				std::ostringstream oss;
				oss << "  Crossing Rule " << i;
				m_signal_log(log_debug0, oss.str());
			}
			for (unsigned int j = 0; j < alt.size(); ++j) {
				const LGMandatorySequence& seq(alt[j]);
				std::ostringstream oss;
				oss << "    ";
				for (unsigned int k = 0; k < seq.size(); ++k) {
					const LGMandatoryPoint& pt(seq[k]);
					const LVertex& vv(m_lgraph[pt.get_v()]);
					oss << vv.get_ident();
					unsigned int pi0(pt.get_pi0());
					unsigned int pi1(pt.get_pi1());
					if (pi0 < pis && pi1 < pis)
						oss << ' ' << m_performance.get_cruise(pi0).get_fplalt()
						    << ".." << m_performance.get_cruise(pi1).get_fplalt();
					if (k + 1 < seq.size())
						oss << ' ' << lgraphindex2airway(pt.get_airway(), true) << ' ';
				}
				m_signal_log(log_debug0, oss.str());
			}
		}
	}
}

void CFMUAutoroute45::lgraphedgemetric(double& cruisemetric, double& levelchgmetric, double& trknmi, unsigned int piu, unsigned int piv,
				       const LEdge& ee, const Performance& perf)
{
	cruisemetric = levelchgmetric = trknmi = 0;
	const unsigned int pis(ee.get_levels());
	if (piu >= pis || piv >= pis) {
		cruisemetric = levelchgmetric = std::numeric_limits<double>::quiet_NaN();
		return;
	}
	unsigned int pic(piu);
	// level change penalty
	if (ee.is_match(airwaysid)) {
		pic = piv;
		piu = pis;
	} else if (ee.is_match(airwaystar)) {
		piv = pis;
	}
	cruisemetric = ee.get_metric(pic);
	const Performance::LevelChange& lvlchg(perf.get_levelchange(piu, piv));
	levelchgmetric = lvlchg.get_metricpenalty();
	trknmi = std::max(lvlchg.get_tracknmi(), lvlchg.get_opsperftracknmi());
}

void CFMUAutoroute45::lgraphupdatemetric(LGraph& g, LRoute& r)
{
	if (r.size() < 2) {
		r.set_metric(std::numeric_limits<double>::quiet_NaN());
		return;
	}
	const unsigned int pis(m_performance.size());
	double m(0);
	for (LRoute::size_type i(1); i < r.size(); ++i) {
		unsigned int piu(r[i-1].get_perfindex());
		unsigned int piv(r[i].get_perfindex());
		LGraph::edge_descriptor e;
		bool ok;
		tie(e, ok) = r.get_edge(i, g);
		if (!ok) {
			r.set_metric(std::numeric_limits<double>::infinity());
			std::ostringstream oss;
			oss << "Route Update Metric: Leg " << lgraphvertexname(g, r[i-1].get_vertex(), piu)
			    << "--" << lgraphindex2airway(r[i].get_edge(), true) << "->"
			    << lgraphvertexname(g, r[i].get_vertex(), piv) << " does not exist";
			m_signal_log(log_debug0, oss.str());
			return;
		}
		if (!g.is_valid_connection(piu, piv, e)) {
			r.set_metric(std::numeric_limits<double>::infinity());
			std::ostringstream oss;
			oss << "Route Update Metric: Leg " << lgraphvertexname(g, r[i-1].get_vertex(), piu)
			    << "--" << lgraphindex2airway(r[i].get_edge(), true) << "->"
			    << lgraphvertexname(g, r[i].get_vertex(), piv) << " has no connection";
			m_signal_log(log_debug0, oss.str());
			return;
		}
		const LEdge& ee(g[e]);
		double cruisemetric, levelchgmetric, trknmi;
		lgraphedgemetric(cruisemetric, levelchgmetric, trknmi, piu, piv, ee, m_performance);
		m += cruisemetric + levelchgmetric;
		if (is_metric_invalid(m)) {
			r.set_metric(std::numeric_limits<double>::infinity());
			std::ostringstream oss;
			oss << "Route Update Metric: Leg " << lgraphvertexname(g, r[i-1].get_vertex(), piu)
			    << "--" << lgraphindex2airway(r[i].get_edge(), true) << "->"
			    << lgraphvertexname(g, r[i].get_vertex(), piv) << " has invalid metric " << m
			    << " (cruise " << cruisemetric << " level change " << levelchgmetric << ')';
			m_signal_log(log_debug0, oss.str());
			return;
		}
	}
	r.set_metric(m);
}

void CFMUAutoroute45::lgraphclearcurrent(void)
{
	while (m_route.get_nrwpt())
		m_route.delete_wpt(0);
	m_routetime = 0;
	m_routefuel = 0;
	m_validationresponse.clear();
	m_solutionvertices.clear();
	{
		LGraph::edge_iterator e0, e1;
		for (tie(e0, e1) = edges(m_lgraph); e0 != e1; ++e0) {
			LEdge& ee(m_lgraph[*e0]);
			ee.clear_solution();
		}
	}
}

bool CFMUAutoroute45::lgraphsetcurrent(const LRoute& r)
{
	lgraphclearcurrent();
	if (r.size() < 2)
		return false;
	{
		LGraph::vertex_descriptor v(r.back().get_vertex());
		m_solutionvertices.insert((LVertexLevel)r.back());
		{
			const LVertex& vv(m_lgraph[v]);
			if (!vv.insert(m_route, 0)) {
				if (true) {
					std::ostringstream oss;
					oss << "Cannot insert " << vv.get_ident() << " into fpl";
					m_signal_log(log_debug0, oss.str());
				}
				lgraphclearcurrent();
				return false;
			}
			m_route[0].frob_flags(get_destination_ifr() ? FPlanWaypoint::altflag_ifr : 0, FPlanWaypoint::altflag_ifr);
		}
	}
	for (LRoute::size_type i(r.size()); i > 1; ) {
		--i;
		LGraph::vertex_descriptor u(r[i-1].get_vertex()), v(r[i].get_vertex());
		unsigned int pi0(r[i-1].get_perfindex()), pi1(r[i].get_perfindex());
		m_solutionvertices.insert((LVertexLevel)r[i-1]);
		const LVertex& uu(m_lgraph[u]);
		const LVertex& vv(m_lgraph[v]);
		LGraph::edge_descriptor e;
		{
			bool ok;
			tie(e, ok) = r.get_edge(i, m_lgraph);
			if (!ok) {
				if (true) {
					std::ostringstream oss;
					oss << "k shortest path: Solution Edge " << lgraphvertexname(u)
					    << "--" << lgraphindex2airway(r[i].get_edge(), true) << "->"
					    << lgraphvertexname(v) << " not found";
					m_signal_log(log_debug0, oss.str());
				}
				lgraphclearcurrent();
				return false;
			}
		}
		LEdge& ee(m_lgraph[e]);
		if (ee.is_match(airwaysid))
			ee.set_solution(pi1);
		else
			ee.set_solution(pi0);
		if (false) {
			const LEdge& ee(m_lgraph[e]);
			std::ostringstream oss;
			oss << "Solution Edge: " << lgraphvertexname(u, pi0) << uu.get_coord().get_lat_str2() << ' ' << uu.get_coord().get_lon_str2()
			    << " --" << lgraphindex2airway(ee.get_ident()) << "-> "
			    << lgraphvertexname(v, pi1) << vv.get_coord().get_lat_str2() << ' ' << vv.get_coord().get_lon_str2();
			m_signal_log(log_debug0, oss.str());
		}
		if (!uu.insert(m_route, 0)) {
			if (true) {
				std::ostringstream oss;
				oss << "Cannot insert " << uu.get_ident() << " into fpl";
				m_signal_log(log_debug0, oss.str());
			}
			lgraphclearcurrent();
			return false;
		}
		m_route[0].set_pathcode(FPlanWaypoint::pathcode_none);
		if (i <= 1)
			break;
		if (pi0 >= m_performance.size()) {
			if (true)
				m_signal_log(log_debug0, "Route level index error");
			lgraphclearcurrent();
			return false;
		}
		{
			const CFMUAutoroute45::Performance::Cruise& cruise(m_performance.get_cruise(pi0));
			m_route[0].set_altitude(cruise.get_altitude());
			m_route[0].set_flags(m_route[0].get_flags() | (FPlanWaypoint::altflag_ifr | FPlanWaypoint::altflag_standard));
		}
		if (ee.is_match(airwaydct)) {
			m_route[0].set_pathcode(FPlanWaypoint::pathcode_directto);
		} else if (ee.is_match(airwaymatchawy)) {
			m_route[0].set_pathname(lgraphindex2airway(ee.get_ident()));
			m_route[0].set_pathcode(FPlanWaypoint::pathcode_airway);
		}
	}
	m_route[0].frob_flags(get_departure_ifr() ? FPlanWaypoint::altflag_ifr : 0, FPlanWaypoint::altflag_ifr);
	if (m_route.get_nrwpt() >= 2) {
		const FPlanWaypoint& wpt1(m_route[m_route.get_nrwpt() - 1]);
		FPlanWaypoint& wpt0(m_route[m_route.get_nrwpt() - 2]);
		wpt0.set_flags(wpt0.get_flags() ^ ((wpt0.get_flags() ^ wpt1.get_flags()) & FPlanWaypoint::altflag_ifr));
	}
	updatefpl();
	return true;
}

void CFMUAutoroute45::lgraphmodified(void)
{
	lgraphkillemptyedges();
	m_solutiontree.clear();
	m_solutionpool.clear();
}

bool CFMUAutoroute45::lgraphisinsolutiontree(const LRoute& r) const
{
	const LGraphSolutionTreeNode *tree(&m_solutiontree);
	for (LRoute::size_type i(0); i < r.size(); ++i) {
		LGraphSolutionTreeNode::const_iterator ti(tree->find(LVertexEdge(r[i])));
		if (ti == tree->end())
			return false;
		tree = &ti->second;
	}
	return true;
}

bool CFMUAutoroute45::lgraphinsertsolutiontree(const LRoute& r)
{
	bool intree(true);
	LGraphSolutionTreeNode *tree(&m_solutiontree);
	for (LRoute::const_iterator ri(r.begin()), re(r.end()); ri != re; ++ri) {
		std::pair<LGraphSolutionTreeNode::iterator,bool> ins(tree->insert(LGraphSolutionTreeNode::value_type(LVertexEdge(*ri), LGraphSolutionTreeNode())));
		tree = &ins.first->second;
		intree = intree && !ins.second;
	}
	return intree;
}

std::string CFMUAutoroute45::lgraphprint(LGraph& g, const LRoute& r)
{
	std::ostringstream oss;
	for (LRoute::const_iterator ri(r.begin()), re(r.end()); ri != re; ++ri) {
		if (ri->get_edge() != airwaymatchnone)
			oss << ' ' << lgraphindex2airway(ri->get_edge(), true);
		oss << ' ' << lgraphvertexname(g, *ri);
	}
	if (oss.str().empty())
		return std::string();
	return oss.str().substr(1);
}

class CFMUAutoroute45::DijkstraCore {
public:
	typedef enum {
		white,
		gray,
		black
	} color_t;

	class DijkstraQueue : public LVertexLevel {
	public:
		DijkstraQueue(const LVertexLevel& v, double d = 0) : LVertexLevel(v), m_dist(d) {}
		DijkstraQueue(LGraph::vertex_descriptor v = 0, unsigned int pi = 0, double d = 0) : LVertexLevel(v, pi), m_dist(d) {}
		double get_dist(void) const { return m_dist; }
		bool operator<(const DijkstraQueue& x) const {
			if (get_dist() < x.get_dist())
				return true;
			if (x.get_dist() < get_dist())
				return false;
			return LVertexLevel::operator<(x);
		}

	protected:
		double m_dist;
	};

	DijkstraCore(const LGraph& g, const Performance& performance);
	void init(void);
	void set_source(const LVertexLevel& u);
	void set_source_nodist(const LVertexLevel& u);
	void rebuild_queue(void);
	void run(lgraphairwayindex_t awy = airwaymatchall, bool solutionedgeonly = false);
	void run_once(lgraphairwayindex_t awy = airwaymatchall, bool solutionedgeonly = false);
	LRoute get_route(const LVertexLevel& v);
	void mark_all_white(void);
	void mark_path(const LVertexLevel& v);
	void mark_white_infinite(void);
	void mark_white_selfpred(void);
	void mark_white_infinite_selfpred(void);
	void copy_gray_paths(const DijkstraCore& x);
	bool path_contains(const LVertexLevel& v, const LVertexLevel& x);
	bool path_contains(const LVertexLevel& v, LGraph::vertex_descriptor x);
	void swap(DijkstraCore& x);
	unsigned int get_pisize(void) const { return m_state.get_pisize(); }

	class LVertexLevelState {
	public:
		LVertexLevelState(const LVertexLevel& v = LVertexLevel(0, 0), lgraphairwayindex_t e = airwaymatchnone,
				  double dist = std::numeric_limits<double>::max(), color_t color = white)
			: m_vertex(v.get_vertex()), m_dist(dist), m_edge(e), m_perfindex(v.get_perfindex()), m_color(color) {}
		LVertexLevel get_pred(void) const { return LVertexLevel(m_vertex, m_perfindex); }
		void set_pred(const LVertexLevel& v) { m_vertex = v.get_vertex(); m_perfindex = v.get_perfindex(); }
	        lgraphairwayindex_t get_edge(void) const { return m_edge; }
		void set_edge(lgraphairwayindex_t e) { m_edge = e; }
		double get_dist(void) const { return m_dist; }
		void set_dist(double dist = std::numeric_limits<double>::quiet_NaN()) { m_dist = dist; }
		void set_dist_max(void) { set_dist(std::numeric_limits<double>::max()); }
		color_t get_color(void) const { return m_color; }
		void set_color(color_t c) { m_color = c; }
	protected:
		LGraph::vertex_descriptor m_vertex; /* 8 bytes */
		double m_dist; /* 8 bytes */
		lgraphairwayindex_t m_edge; /* 4 bytes */
		uint8_t m_perfindex;
		color_t m_color;
	};

	class StateVector {
	public:
		StateVector(unsigned int nrvert = 0, unsigned int pisize = 0) { init(nrvert, pisize); }
		void init(unsigned int nrvert = 0, unsigned int pisize = 0);
		LVertexLevelState& operator[](const LVertexLevel& x);
		const LVertexLevelState& operator[](const LVertexLevel& x) const;
		LVertexLevelState& operator[](unsigned int x);
		const LVertexLevelState& operator[](unsigned int x) const;
		unsigned int get_pisize(void) const { return m_pisize; }
		unsigned int get_size(void) const { return m_pred.size(); }
		void swap(StateVector& x) { m_pred.swap(x.m_pred); std::swap(m_pisize, x.m_pisize); }

	protected:
		std::vector<LVertexLevelState> m_pred;
		unsigned int m_pisize;
	};

	static inline const std::string& to_str(CFMUAutoroute45::DijkstraCore::color_t c)
	{
		switch (c) {
		case CFMUAutoroute45::DijkstraCore::white:
		{
			static const std::string r("white");
			return r;
		}

		case CFMUAutoroute45::DijkstraCore::gray:
		{
			static const std::string r("gray");
			return r;
		}

		case CFMUAutoroute45::DijkstraCore::black:
		{
			static const std::string r("black");
			return r;
		}

		default:
		{
			static const std::string r("?");
			return r;
		}
		}
	}

	const LGraph& m_lgraph;
	const Performance& m_performance;
        StateVector m_state;
	typedef std::set<DijkstraQueue> queue_t;
	queue_t m_queue;
};

void CFMUAutoroute45::DijkstraCore::StateVector::init(unsigned int nrvert, unsigned int pisize)
{
	m_pred.clear();
	m_pisize = pisize;
	m_pred.resize(m_pisize * nrvert);
	for (LGraph::vertex_descriptor i = 0; i < nrvert; ++i)
		for (unsigned int pi = 0; pi < m_pisize; ++pi) {
			LVertexLevel v(i, pi);
			(*this)[v] = LVertexLevelState(v, airwaymatchnone, std::numeric_limits<double>::max(), white);
		}
}

CFMUAutoroute45::DijkstraCore::LVertexLevelState& CFMUAutoroute45::DijkstraCore::StateVector::operator[](const LVertexLevel& x)
{
	unsigned int idx(x.get_vertex() * m_pisize + x.get_perfindex());
	if (idx >= m_pred.size() || x.get_perfindex() >= m_pisize)
		throw std::runtime_error("DijkstraCore::StateVector index error");
	return m_pred[idx];
}

const CFMUAutoroute45::DijkstraCore::LVertexLevelState& CFMUAutoroute45::DijkstraCore::StateVector::operator[](const LVertexLevel& x) const
{
	unsigned int idx(x.get_vertex() * m_pisize + x.get_perfindex());
	if (idx >= m_pred.size() || x.get_perfindex() >= m_pisize)
		throw std::runtime_error("DijkstraCore::StateVector index error");
	return m_pred[idx];
}

CFMUAutoroute45::DijkstraCore::LVertexLevelState& CFMUAutoroute45::DijkstraCore::StateVector::operator[](unsigned int x)
{
	if (x >= m_pred.size())
		throw std::runtime_error("DijkstraCore::StateVector index error");
	return m_pred[x];
}

const CFMUAutoroute45::DijkstraCore::LVertexLevelState& CFMUAutoroute45::DijkstraCore::StateVector::operator[](unsigned int x) const
{
	if (x >= m_pred.size())
		throw std::runtime_error("DijkstraCore::StateVector index error");
	return m_pred[x];
}

CFMUAutoroute45::DijkstraCore::DijkstraCore(const LGraph& g, const Performance& performance)
	: m_lgraph(g), m_performance(performance)
{
	init();
}

void CFMUAutoroute45::DijkstraCore::init(void)
{
	m_state.init(boost::num_vertices(m_lgraph), m_performance.size());
}

void CFMUAutoroute45::DijkstraCore::set_source(const LVertexLevel& u)
{
	LVertexLevelState& stateu(m_state[u]);
	stateu.set_color(gray);
	stateu.set_dist(0);
	m_queue.insert(DijkstraQueue(u, stateu.get_dist()));
}

void CFMUAutoroute45::DijkstraCore::set_source_nodist(const LVertexLevel& u)
{
	LVertexLevelState& stateu(m_state[u]);
	stateu.set_color(gray);
	m_queue.insert(DijkstraQueue(u, stateu.get_dist()));
}

void CFMUAutoroute45::DijkstraCore::rebuild_queue(void)
{
	m_queue.clear();
	const unsigned int pis(get_pisize());
	for (unsigned int i = 0, n = boost::num_vertices(m_lgraph); i < n; ++i)
		for (unsigned int pi = 0; pi < pis; ++pi) {
			LVertexLevel u(i, pi);
			const LVertexLevelState& stateu(m_state[u]);
			if (stateu.get_color() != gray)
				continue;
			m_queue.insert(DijkstraQueue(u, stateu.get_dist()));
		}
}

void CFMUAutoroute45::DijkstraCore::run_once(lgraphairwayindex_t awy, bool solutionedgeonly)
{
	const unsigned int pis(get_pisize());
	LVertexLevel u(*m_queue.begin());
	m_queue.erase(m_queue.begin());
	LVertexLevelState& stateu(m_state[u]);
	LGraph::out_edge_iterator e0, e1;
	for (boost::tie(e0, e1) = boost::out_edges(u.get_vertex(), m_lgraph); e0 != e1; ++e0) {
		const LEdge& ee(m_lgraph[*e0]);
		if (!ee.is_match(awy))
			continue;
		if (solutionedgeonly && !ee.is_solution())
			continue;
		LGraph::vertex_descriptor vdv(boost::target(*e0, m_lgraph));
		for (unsigned int piv = 0; piv < pis; ++piv) {
			LVertexLevel v(vdv, piv);
			LVertexLevelState& statev(m_state[v]);
			if (!false) {
				const LVertex& uu(m_lgraph[u.get_vertex()]);
				if (uu.get_ident() == "LSZH") {
					const LVertex& vv(m_lgraph[v.get_vertex()]);
					const LEdge& ee(m_lgraph[*e0]);
					double cruisemetric, levelchgmetric, trknmi;
					lgraphedgemetric(cruisemetric, levelchgmetric, trknmi, u.get_perfindex(), v.get_perfindex(), ee, m_performance);
					std::cerr << "run_once: " << uu.get_ident() << '/' << u.get_perfindex()
						  << "->" << vv.get_ident() << '/' << v.get_perfindex()
						  << (ee.is_match(awy) ? " awymatch" : "")
						  << (solutionedgeonly ? " solonly" : "")
						  << (ee.is_solution() ? " solution" : "")
						  << ' ' << to_str(statev.get_color())
						  << " metric old " << statev.get_dist()
						  << " new " << (stateu.get_dist() + cruisemetric + levelchgmetric)
						  << " (prev " << stateu.get_dist() << " cruise " << cruisemetric
						  << " lvlchg " << levelchgmetric << " tracknmi " << trknmi << ") dist " << ee.get_dist() << std::endl;
				}
			}
			if (statev.get_color() == black)
				continue;
			if (!m_lgraph.is_valid_connection_precheck(u.get_perfindex(), v.get_perfindex(), *e0))
				continue;
			double cruisemetric, levelchgmetric, trknmi;
			lgraphedgemetric(cruisemetric, levelchgmetric, trknmi, u.get_perfindex(), v.get_perfindex(), ee, m_performance);
			if (trknmi > ee.get_dist() && u.get_perfindex() < pis && v.get_perfindex() < pis)
				continue;
			double newmetric(stateu.get_dist() + cruisemetric + levelchgmetric);
			if (is_metric_invalid(newmetric)) {
				const LVertex& uu(m_lgraph[u.get_vertex()]);
				const LVertex& vv(m_lgraph[v.get_vertex()]);
				const LEdge& ee(m_lgraph[*e0]);
				std::cerr << "run_once: invalid metric " << uu.get_ident() << '/' << u.get_perfindex()
					  << "->" << vv.get_ident() << '/' << v.get_perfindex()
					  << (ee.is_match(awy) ? " awymatch" : "")
					  << (solutionedgeonly ? " solonly" : "")
					  << (ee.is_solution() ? " solution" : "")
					  << ' ' << to_str(statev.get_color())
					  << " prev " << stateu.get_dist() << " cruise " << cruisemetric
					  << " lvlchg " << levelchgmetric << " tracknmi " << trknmi
					  << " dist " << ee.get_dist() << std::endl;
				continue;
			}
			if (newmetric >= statev.get_dist())
				continue;
			if (!m_lgraph.is_valid_connection(u.get_perfindex(), v.get_perfindex(), *e0))
				continue;
			// check whether new edge does not violate some flight plan requirements
			// this violates the optimality principle, hence this dijkstra implementation
			// is not strictly optimal. However, we do not want to incur the cost of a full
			// global optimization.
			// check the flight plan does not contain duplicate nodes
			{
				const LVertex& vv(m_lgraph[v.get_vertex()]);
				LVertexLevel w(u);
				bool ok(true);
				for (;;) {
	        			const LVertex& ww(m_lgraph[w.get_vertex()]);
					if (ww.get_ident() == vv.get_ident()) {
						ok = false;
						break;
					}
					LVertexLevel wold(w);
					w = m_state[w].get_pred();
					if (w == wold)
						break;
				}
				if (!ok)
					continue;
			}
			// edge is a new better edge
			if (statev.get_color() == gray) {
				queue_t::iterator i(m_queue.find(DijkstraQueue(v, statev.get_dist())));
				if (i == m_queue.end()) {
					const LVertex& vv(m_lgraph[v.get_vertex()]);
					std::ostringstream oss;
					oss << "lgraphdijkstra: gray node " << vv.get_ident() << '/' << v.get_perfindex()
					    << " dist " << statev.get_dist() << " not found in m_queue";
					throw std::runtime_error(oss.str());
				} else {
					m_queue.erase(i);
				}
			}
			statev = LVertexLevelState(u, ee.get_ident(), newmetric, gray);
			m_queue.insert(DijkstraQueue(v, statev.get_dist()));
		}
	}
	stateu.set_color(black);
}

void CFMUAutoroute45::DijkstraCore::run(lgraphairwayindex_t awy, bool solutionedgeonly)
{
	// DIJKSTRA(G, s, w)
	//   for each vertex u in V (This loop is not run in dijkstra_shortest_paths_no_init)
	//     d[u] := infinity 
	//     p[u] := u 
	//     color[u] := WHITE
	//   end for
	//   color[s] := GRAY 
	//   d[s] := 0 
	//   INSERT(Q, s)
	//   while (Q != Ø)
	//     u := EXTRACT-MIN(Q)
	//     S := S U { u }
	//     for each vertex v in Adj[u]
	//       if (w(u,v) + d[u] < d[v])
	//         d[v] := w(u,v) + d[u]
	//         p[v] := u 
	//         if (color[v] = WHITE) 
	//           color[v] := GRAY
	//           INSERT(Q, v) 
	//         else if (color[v] = GRAY)
	//           DECREASE-KEY(Q, v)
	//       else
	//         ...
	//     end for
	//     color[u] := BLACK
	//   end while
	//   return (d, p)
	while (!m_queue.empty())
		run_once(awy, solutionedgeonly);
}

CFMUAutoroute45::LRoute CFMUAutoroute45::DijkstraCore::get_route(const LVertexLevel& v)
{
	LRoute r;
	{
		LVertexLevel w(v);
		for (;;) {
			const LVertexLevelState& statew(m_state[w]);
			LVertexLevel u(statew.get_pred());
			if (false) {
				const LVertex& uu(m_lgraph[u.get_vertex()]);
				const LVertex& ww(m_lgraph[w.get_vertex()]);
				std::cerr << "CFMUAutoroute45::DijkstraCore::get_route: "
					  << uu.get_ident() << '/' << u.get_perfindex() << ' '
					  << statew.get_edge() << ' '
					  << ww.get_ident() << '/' << w.get_perfindex() << std::endl;
			}
			if (u == w) {
				r.insert(r.begin(), LVertexEdge(w));
				break;
			}
			r.insert(r.begin(), LVertexEdge(w, statew.get_edge()));
			w = u;
		}
	}
	if (r.size() <= 1)
		r.clear();
	return r;
}

void CFMUAutoroute45::DijkstraCore::mark_all_white(void)
{
	const unsigned int pis(get_pisize());
	for (unsigned int i = 0, n = boost::num_vertices(m_lgraph); i < n; ++i)
		for (unsigned int pi = 0; pi < pis; ++pi)
			m_state[LVertexLevel(i, pi)].set_color(white);
}

void CFMUAutoroute45::DijkstraCore::mark_path(const LVertexLevel& v)
{
	{
		LVertexLevelState& state(m_state[v]);
		if (state.get_pred() == v)
			return;
		state.set_color(gray);
	}
	LVertexLevel vv(v);
	for (;;) {
		LVertexLevel u(m_state[vv].get_pred());
		if (u == vv)
			break;
		LVertexLevelState& stateu(m_state[u]);
		if (stateu.get_color() == white)
			stateu.set_color(black);
		vv = u;
	}
}

bool CFMUAutoroute45::DijkstraCore::path_contains(const LVertexLevel& v, const LVertexLevel& x)
{
	LVertexLevel vv(v);
	for (;;) {
		if (vv == x)
			return true;
		LVertexLevel u(m_state[vv].get_pred());
		if (u == vv)
			break;
		vv = u;
	}
	return false;
}

bool CFMUAutoroute45::DijkstraCore::path_contains(const LVertexLevel& v, LGraph::vertex_descriptor x)
{
	LVertexLevel vv(v);
	for (;;) {
		if (vv.get_vertex() == x)
			return true;
		LVertexLevel u(m_state[vv].get_pred());
		if (u == vv)
			break;
		vv = u;
	}
	return false;
}

void CFMUAutoroute45::DijkstraCore::mark_white_infinite(void)
{
	const unsigned int pis(get_pisize());
	for (unsigned int i = 0, n = boost::num_vertices(m_lgraph); i < n; ++i)
		for (unsigned int pi = 0; pi < pis; ++pi) {
			LVertexLevelState& state(m_state[LVertexLevel(i, pi)]);
			if (state.get_color() == white)
				state.set_dist_max();
		}
}

void CFMUAutoroute45::DijkstraCore::mark_white_selfpred(void)
{
	const unsigned int pis(get_pisize());
	for (unsigned int i = 0, n = boost::num_vertices(m_lgraph); i < n; ++i)
		for (unsigned int pi = 0; pi < pis; ++pi) {
			LVertexLevel lv(i, pi);
			LVertexLevelState& state(m_state[lv]);
			if (state.get_color() == white)
				state.set_pred(lv);
		}
}

void CFMUAutoroute45::DijkstraCore::mark_white_infinite_selfpred(void)
{
	const unsigned int pis(get_pisize());
	for (unsigned int i = 0, n = boost::num_vertices(m_lgraph); i < n; ++i)
		for (unsigned int pi = 0; pi < pis; ++pi) {
			LVertexLevel lv(i, pi);
			LVertexLevelState& state(m_state[lv]);
			if (state.get_color() != white)
				continue;
			state.set_dist_max();
			state.set_pred(lv);
		}
}


void CFMUAutoroute45::DijkstraCore::copy_gray_paths(const DijkstraCore& x)
{
	if (boost::num_vertices(m_lgraph) != boost::num_vertices(x.m_lgraph) ||
	    (boost::num_vertices(m_lgraph) && (m_state.get_size() != x.m_state.get_size() ||
					       m_state.get_pisize() != x.m_state.get_pisize())))
		throw std::runtime_error("copy_gray_paths: invalid graphs");
	const unsigned int pis(get_pisize());
	for (unsigned int i = 0, n = boost::num_vertices(m_lgraph); i < n; ++i) {
		for (unsigned int pi = 0; pi < pis; ++pi) {
			LVertexLevel v(i, pi);
			{
				const LVertexLevelState& statev(x.m_state[v]);
				if (statev.get_color() != gray)
					continue;
				if (statev.get_pred() == v)
					continue;
			}
			for (;;) {
				const LVertexLevelState& statev(x.m_state[v]);
				LVertexLevel u(statev.get_pred());
				color_t col;
				{
					const LVertexLevelState& statevv(m_state[v]);
					if (!(statev.get_dist() < statevv.get_dist()))
						break;
					col = statevv.get_color();
				}
				if (col == white)
					col = black;
				m_state[v] = LVertexLevelState(u, statev.get_edge(), statev.get_dist(), col);
				if (u == v)
					break;
				v = u;
			}
			m_state[LVertexLevel(i, pi)].set_color(gray);
		}
	}
}

void CFMUAutoroute45::DijkstraCore::swap(DijkstraCore& x)
{
	if (boost::num_vertices(m_lgraph) != boost::num_vertices(x.m_lgraph) ||
	    (boost::num_vertices(m_lgraph) && (m_state.get_size() != x.m_state.get_size() ||
					       m_state.get_pisize() != x.m_state.get_pisize())))
		throw std::runtime_error("swap: invalid graphs");
	m_state.swap(x.m_state);
	m_queue.swap(x.m_queue);
}

CFMUAutoroute45::LRoute CFMUAutoroute45::lgraphdijkstra(LGraph& g, const LVertexLevel& u, const LVertexLevel& v, const LRoute& baseroute, bool solutionedgeonly, LGMandatory mandatory)
{
	const unsigned int pis(m_performance.size());
	DijkstraCore d(g, m_performance);
	d.m_state[u].set_dist(0);
	// start at specific route
	if (!baseroute.empty()) {
		LRoute::size_type i(1), n(baseroute.size());
		if (((LVertexLevel)baseroute[0]) != u) {
			d.m_state[(LVertexLevel)baseroute[0]].set_dist(0);
			for (; i < n; ++i) {
				LVertexLevel vv(baseroute[i]);
				LVertexLevel uu(baseroute[i - 1]);
				DijkstraCore::LVertexLevelState& stateuu(d.m_state[uu]);
				DijkstraCore::LVertexLevelState& statevv(d.m_state[vv]);
				statevv.set_pred(uu);
				std::pair<LGraph::edge_descriptor,bool> ee(LRoute::get_edge(baseroute[i - 1], baseroute[i], g));
				if (!ee.second) {
					std::ostringstream oss;
					oss << "lgraphdijkstra: base route edge not found";
					m_signal_log(log_debug0, oss.str());
					return LRoute();
				}
				stateuu.set_color(DijkstraCore::black);
				const LEdge& eee(g[ee.first]);
				statevv.set_edge(eee.get_ident());
				double cruisemetric, levelchgmetric, trknmi;
				lgraphedgemetric(cruisemetric, levelchgmetric, trknmi, uu.get_perfindex(), vv.get_perfindex(), eee, m_performance);
				double newmetric(stateuu.get_dist() + cruisemetric + levelchgmetric);
				if (is_metric_invalid(newmetric)) {
					std::ostringstream oss;
					oss << "lgraphdijkstra: base route edge metric " << newmetric << " invalid";
					m_signal_log(log_debug0, oss.str());
					return LRoute();
				}
				statevv.set_dist(newmetric);
				if (vv == u)
					break;
			}
			if (i >= n) {
				std::ostringstream oss;
				oss << "lgraphdijkstra: start vertex not found in base route";
				m_signal_log(log_debug0, oss.str());
			}
		}
	}
	d.set_source_nodist(u);
	for (;;) {
		try {
			d.run(airwaymatchall, solutionedgeonly);
		} catch (const std::exception& e) {
			m_signal_log(log_debug0, e.what());
			return LRoute();
		}
		if (mandatory.empty())
			break;
		// find the mandatory rule that is closest to the source
		unsigned int mrule(~0U);
		{
			double bestdist(std::numeric_limits<double>::max());
			for (unsigned int mr = 0; mr < mandatory.size(); ++mr) {
				const LGMandatoryAlternative& alt(mandatory[mr]);
				for (unsigned int ma = 0; ma < alt.size(); ++ma) {
					const LGMandatorySequence& seq(alt[ma]);
					if (seq.empty())
						continue;
					if (true) {
						std::ostringstream oss;
						oss << "lgraphdijkstra: trying mandatory sequence";
						for (unsigned int si = 0; si < seq.size(); ++si)
							oss << ' ' << lgraphvertexname(g, seq[si].get_v(), seq[si].get_pi0(), seq[si].get_pi1())
							    << ' ' << lgraphindex2airway(seq[si].get_airway(), true);
						m_signal_log(log_debug0, oss.str());
					}
					double dist(std::numeric_limits<double>::max());
					for (unsigned int pi = seq[0].get_pi0(); pi <= seq[0].get_pi1(); ++pi) {
						LVertexLevel v(seq[0].get_v(), pi);
						const DijkstraCore::LVertexLevelState& statev(d.m_state[v]);
						if (v == statev.get_pred())
							continue;
						if (!(statev.get_dist() < dist) || !(statev.get_dist() < bestdist))
							continue;
						if (seq.size() > 1) {
							LGraph::out_edge_iterator e0, e1;
							for (boost::tie(e0, e1) = boost::out_edges(v.get_vertex(), g); e0 != e1; ++e0) {
								const LEdge& ee(g[*e0]);
								if (!ee.is_match(seq[0].get_airway()))
									continue;
								if (!ee.is_valid(v.get_perfindex()))
									continue;
								break;
							}
							if (e0 == e1)
								continue;
						}
						dist = statev.get_dist();
					}
					if (!(dist < bestdist))
						continue;
					bestdist = dist;
					mrule = mr;
				}
			}
		}
		if (mrule >= mandatory.size()) {
			std::ostringstream oss;
			oss << "lgraphdijkstra: cannot satisfy mandatory rule";
			if (mandatory.size() >= 2)
				oss << 's';
			bool subseq(false);
			for (LGMandatory::const_iterator mi(mandatory.begin()), me(mandatory.end()); mi != me; ++mi) {
				if (subseq)
					oss << ',';
				subseq = true;
				oss << ' ' << mi->get_rule();
			}
			m_signal_log(log_debug0, oss.str());
			return LRoute();
		}
		// remove all but paths to beginning of mandatory alternatives from dijkstra results
		d.mark_all_white();
		{
			const LGMandatoryAlternative& alt(mandatory[mrule]);
			for (unsigned int ma = 0; ma < alt.size(); ++ma) {
				const LGMandatorySequence& seq(alt[ma]);
				if (seq.empty())
					continue;
				for (unsigned int pi = seq[0].get_pi0(); pi <= seq[0].get_pi1(); ++pi) {
					LVertexLevel v(seq[0].get_v(), pi);
					// avoid marking a path that already contains points from the mandatory sequence
					{
						unsigned int mp;
						for (mp = 1; mp < seq.size(); ++mp) {
							const LGMandatoryPoint& pt(seq[mp]);
							if (d.path_contains(v, pt.get_v()))
								break;
						}
						if (mp < seq.size())
							continue;
					}
					const DijkstraCore::LVertexLevelState& statev(d.m_state[v]);
					d.mark_path(v);
					if (true && statev.get_pred() != v) {
						std::ostringstream oss;
						oss << "lgraphdijkstra: mandatory rule " << mrule << " alt " << ma << " marking "
						    << lgraphvertexname(g, v) << " dist " << statev.get_dist();
						m_signal_log(log_debug0, oss.str());
					}
				}
			}
		}
		d.mark_white_infinite_selfpred();
		{
			const LGMandatoryAlternative& alt(mandatory[mrule]);
			DijkstraCore dr(g, m_performance);
			for (unsigned int ma = 0; ma < alt.size(); ++ma) {
				const LGMandatorySequence& seq(alt[ma]);
				if (seq.empty())
					continue;
				DijkstraCore ds(d);
				for (unsigned int mp = 0; ; ++mp) {
					const LGMandatoryPoint& pt(seq[mp]);
					ds.mark_all_white();
					for (unsigned int pi = pt.get_pi0(); pi <= pt.get_pi1(); ++pi) {
						LVertexLevel v(pt.get_v(), pi);
						const DijkstraCore::LVertexLevelState& statev(d.m_state[v]);
						ds.mark_path(v);
						if (true && statev.get_pred() != v) {
							std::ostringstream oss;
							oss << "lgraphdijkstra: mandatory rule " << mrule << " alt " << ma << " point " << mp << " marking "
							    << lgraphvertexname(g, v) << " dist " << statev.get_dist();
							if (false) {
								LRoute r(ds.get_route(v));
								if (!r.empty()) {
									lgraphupdatemetric(g, r);
									oss << " route " << lgraphprint(g, r) << " metric " << r.get_metric();
								}
							}
							m_signal_log(log_debug0, oss.str());
						}
					}
					if (mp + 1 >= seq.size())
						break;
					ds.mark_white_infinite_selfpred();
					ds.rebuild_queue();
					if (pt.get_airway() == airwaydct) {
						LGraph::out_edge_iterator e0, e1;
						for (boost::tie(e0, e1) = boost::out_edges(pt.get_v(), g); e0 != e1; ++e0) {
							if (boost::target(*e0, g) != seq[mp + 1].get_v())
								continue;
							const LEdge& ee(g[*e0]);
							if (!ee.is_dct())
								continue;
							break;
						}
						if (e0 == e1) {
							const LVertex& uu(g[pt.get_v()]);
							const LVertex& vv(g[seq[mp + 1].get_v()]);
							int minalt(-1);
							double dist(uu.get_coord().spheric_distance_nmi_dbl(vv.get_coord()));
							TopoDb30::Profile prof(m_topodb.get_profile(uu.get_coord(), vv.get_coord(), 5));
							if (prof.empty()) {
								std::ostringstream oss;
								oss << "lgraphdijkstra: DCT: Error computing ground clearance: " << uu.get_ident() << ' '
								    << vv.get_ident() << " dist " << std::setprecision(1) << dist << "nmi";
								m_signal_log(log_debug0, oss.str());
							} else {
								TopoDb30::elev_t el(prof.get_maxelev());
								if (el != TopoDb30::nodata) {
									minalt = prof.get_maxelev() * Point::m_to_ft;
									minalt = std::max(minalt, 0);
									if (minalt >= 5000)
										minalt += 1000;
									minalt += 1000;
								}
							}
							LEdge edge(pis, airwaydct);
							for (unsigned int pi = 0; pi < pis; ++pi) {
								if (m_performance.get_cruise(pi).get_altitude() < minalt)
									continue;
								edge.set_metric(pi, 1);
							}
							lgraphaddedge(g, edge, pt.get_v(), seq[mp + 1].get_v(), setmetric_metric, false);
							for (boost::tie(e0, e1) = boost::out_edges(pt.get_v(), g); e0 != e1; ++e0) {
								if (boost::target(*e0, g) != seq[mp + 1].get_v())
									continue;
								const LEdge& ee(g[*e0]);
								if (!ee.is_dct())
									continue;
								break;
							}
						}
						if (e0 != e1) {
							const LEdge& ee(g[*e0]);
							if (!solutionedgeonly || ee.is_solution()) {
								for (unsigned int pi = pt.get_pi0(); pi <= pt.get_pi1(); ++pi) {
									if (!ee.is_valid(pi))
										continue;
									LVertexLevel u(pt.get_v(), pi);
									const DijkstraCore::LVertexLevelState& stateu(ds.m_state[u]);
									if (stateu.get_pred() == u || is_metric_invalid(stateu.get_dist()))
										continue;
									LVertexLevel v(seq[mp + 1].get_v(), pi);
									DijkstraCore::LVertexLevelState& statev(ds.m_state[v]);
									if (statev.get_color() == DijkstraCore::black)
										continue;
									statev.set_pred(u);
								        statev.set_dist(stateu.get_dist() + ee.get_metric(pi));
									statev.set_color(DijkstraCore::gray);
									statev.set_edge(ee.get_ident());
								}
							}
						}
					} else {
						ds.run(pt.get_airway(), solutionedgeonly);
					}
				}
				dr.copy_gray_paths(ds);
				if (true) {
					for (unsigned int i = 0, n = boost::num_vertices(g); i < n; ++i) {
						for (unsigned int pi = 0; pi < pis; ++pi) {
							LVertexLevel ii(i, pi);
							if (ds.m_state[ii].get_color() != DijkstraCore::gray)
								continue;
							LRoute r(ds.get_route(ii));
							if (r.empty())
								continue;
							lgraphupdatemetric(g, r);
							std::ostringstream oss;
							oss << "lgraphdijkstra: mandatory rule " << mrule << " alt " << ma << " route " << lgraphprint(g, r) << " metric " << r.get_metric();
							m_signal_log(log_debug0, oss.str());
						}
					}
				}
			}
			d.swap(dr);
			d.rebuild_queue();
			if (true) {
				for (unsigned int i = 0, n = boost::num_vertices(g); i < n; ++i) {
					for (unsigned int pi = 0; pi < pis; ++pi) {
						LVertexLevel ii(i, pi);
						if (d.m_state[ii].get_color() != DijkstraCore::gray)
							continue;
						if (false) {
							std::ostringstream oss;
							oss << "lgraphdijkstra: gray node ";
						        LVertexLevel v(ii);
							for (;;) {
								oss << lgraphvertexname(g, v) << " metric " << d.m_state[v].get_dist();
							        LVertexLevel u(d.m_state[v].get_pred());
								if (u == v)
									break;
								v = u;
								oss << " <-- " << DijkstraCore::to_str(d.m_state[v].get_color()) << ' ';
							}
							m_signal_log(log_debug0, oss.str());
						}
						LRoute r(d.get_route(ii));
						if (r.empty())
							continue;
						lgraphupdatemetric(g, r);
						std::ostringstream oss;
						oss << "lgraphdijkstra: mandatory rule " << mrule << " route " << lgraphprint(g, r) << " metric " << r.get_metric();
						m_signal_log(log_debug0, oss.str());
					}
				}
			}
		}
		mandatory.erase(mandatory.begin() + mrule);
		if (d.m_queue.empty()) {
			std::ostringstream oss;
			oss << "lgraphdijkstra: no route after mandatory processing";
			m_signal_log(log_debug0, oss.str());
			return LRoute();
		}
	}
	LRoute r(d.get_route(v));
	if (r.empty())
		return r;
	lgraphupdatemetric(g, r);
	{
		LVertexLevel x(baseroute.empty() ? u : (LVertexLevel)baseroute[0]);
		if ((LVertexLevel)r[0] != x) {
			std::ostringstream oss;
			oss << "lgraphdijkstra: final route invalid starting point "
			    << lgraphvertexname(g, (LVertexLevel)r[0]) << " expected " << lgraphvertexname(x);
			m_signal_log(log_debug0, oss.str());
		}
	}
	return r;
}

CFMUAutoroute45::LRoute CFMUAutoroute45::lgraphnextsolution(void)
{
	LRoute r;
	for (;;) {
		if (m_solutionpool.empty()) {
			r = lgraphdijkstra(LVertexLevel(m_vertexdep, 0), LVertexLevel(m_vertexdest, 0), LRoute(), false, m_crossingmandatory);
			lgraphupdatemetric(r);
			if (is_metric_invalid(r.get_metric())) {
				r.clear();
				return r;
			}
			m_solutionpool.insert(r);
			// check if already in solution tree; return if not, otherwise use as solution to base other solutions off
			if (!lgraphinsertsolutiontree(r))
				return r;
		}
		r = *m_solutionpool.begin();
		m_solutionpool.erase(m_solutionpool.begin());
		// verify if solution still exists in graph
		bool ok(r.is_existing(m_lgraph));
		if (true) {
			std::ostringstream oss;
			oss << "k shortest path: Route " << lgraphprint(r) << " metric " << r.get_metric();
			if (ok)
				oss << " still exists";
			else
				oss << " no longer exists";
			m_signal_log(log_debug0, oss.str());
		}
		if (ok)
			break;
	}
	lgraphinsertsolutiontree(r);
	// generate new solutions
	// "abuse" solution flag to enable/disable edges
	{
		LGraph::edge_iterator e0, e1;
		for (tie(e0, e1) = edges(m_lgraph); e0 != e1; ++e0) {
			LEdge& ee(m_lgraph[*e0]);
			ee.set_solution(0);
		}
	}
        LGraphSolutionEdgeFilter filter(m_lgraph);
        typedef boost::filtered_graph<LGraph, LGraphSolutionEdgeFilter> fg_t;
        fg_t fg(m_lgraph, filter);
	LGraphSolutionTreeNode *tree(&m_solutiontree);
	for (LRoute::size_type i(0); i + 1 < r.size(); ++i) {
		if (tree) {
			LGraphSolutionTreeNode::iterator ti(tree->find((LVertexEdge)r[i]));
			if (ti == tree->end())
				tree = 0;
			else
				tree = &ti->second;
		}
		if (tree) {
			for (LGraphSolutionTreeNode::iterator ti(tree->begin()), te(tree->end()); ti != te;) {
				LGraphSolutionTreeNode::iterator tix(ti);
				++ti;
				LGraph::edge_descriptor e;
                                bool ok;
				boost::tie(e, ok) = LRoute::get_edge(r[i], tix->first, m_lgraph);
				if (!ok) {
					if (true) {
						std::ostringstream oss;
						oss << "k shortest path: Leg " << lgraphvertexname(r[i])
						    << "--" << lgraphindex2airway(tix->first.get_edge(), true) << "->"
						    << lgraphvertexname(tix->first) << " does not exist in graph";
						m_signal_log(log_debug0, oss.str());
					}
					tree->erase(tix);
					continue;
				}
				m_lgraph[e].clear_solution();
			}
		}
		// kill other vertices with the same ident
		{
			LGraph::vertex_descriptor v(r[i].get_vertex());
			const LVertex& vv(m_lgraph[v]);
			const std::string& ident(vv.get_ident());
			for (LGraph::vertex_descriptor i = 0; i < boost::num_vertices(m_lgraph); ++i) {
				if (i == v)
					continue;
				const LVertex& ii(m_lgraph[i]);
				if (ii.get_ident() != ident)
					continue;
				LGraph::out_edge_iterator e0, e1;
				for (boost::tie(e0, e1) = boost::out_edges(v, m_lgraph); e0 != e1; ++e0) {
					LEdge& ee(m_lgraph[*e0]);
					ee.clear_solution();
					if (false) {
						std::ostringstream oss;
						oss << "k shortest path: Killing edge " << lgraphvertexname(boost::source(*e0, m_lgraph))
						    << '(' << (unsigned int)boost::source(*e0, m_lgraph) << ')'
						    << "--" << lgraphindex2airway(ee.get_ident(), true)
						    << "->" << lgraphvertexname(boost::target(*e0, m_lgraph))
						    << '(' << (unsigned int)boost::target(*e0, m_lgraph) << ')';
						m_signal_log(log_debug0, oss.str());
					}
				}
			}
		}
		LRoute r1;
		r1 = lgraphdijkstra((LVertexEdge)r[i], LVertexLevel(m_vertexdest, 0), r, true, m_crossingmandatory);
		if (false && tree) {
			LGraph::out_edge_iterator e0, e1;
			for (boost::tie(e0, e1) = boost::out_edges(r[i].get_vertex(), m_lgraph); e0 != e1; ++e0) {
				LEdge& ee(m_lgraph[*e0]);
				if (boost::target(*e0, m_lgraph) == r[i+1].get_vertex() &&
				    ee.get_ident() == r[i+1].get_edge())
					continue;
				ee.clear_solution();
			}
		}
		// kill all out-edges to avoid revisiting a node already in the solution
		{
			LGraph::out_edge_iterator e0, e1;
			for (boost::tie(e0, e1) = boost::out_edges(r[i].get_vertex(), m_lgraph); e0 != e1; ++e0) {
				LEdge& ee(m_lgraph[*e0]);
				ee.clear_solution();
				if (false) {
					std::ostringstream oss;
					oss << "k shortest path: Killing edge " << lgraphvertexname(boost::source(*e0, m_lgraph))
					    << '(' << (unsigned int)boost::source(*e0, m_lgraph) << ')'
					    << "--" << lgraphindex2airway(ee.get_ident(), true)
					    << "->" << lgraphvertexname(boost::target(*e0, m_lgraph))
					    << '(' << (unsigned int)boost::target(*e0, m_lgraph) << ')';
					m_signal_log(log_debug0, oss.str());
				}
			}
		}
		if (r1.size() < 2)
			continue;
		lgraphupdatemetric(r1);
		if (is_metric_invalid(r1.get_metric())) {
			if (true) {
				std::ostringstream oss;
				oss << "k shortest path: Route";
		        	for (LRoute::const_iterator ri(r1.begin()), re(r1.end()); ri != re; ++ri) {
					if (ri->get_edge() != airwaymatchnone)
						oss << ' ' << lgraphindex2airway(ri->get_edge(), true);
					oss << ' ' << lgraphvertexname((LVertexLevel)*ri);
				}
				oss << " has no metric";
				m_signal_log(log_debug0, oss.str());
			}
			continue;
		}
		if (true) {
			std::ostringstream oss;
			oss << "k shortest path: Route";
			for (LRoute::const_iterator ri(r1.begin()), re(r1.end()); ri != re; ++ri) {
				if (ri->get_edge() != airwaymatchnone)
					oss << ' ' << lgraphindex2airway(ri->get_edge(), true);
				oss << ' ' << lgraphvertexname((LVertexLevel)*ri) << '(' << (unsigned int)ri->get_vertex() << ')';
			}
			oss << " metric " << r1.get_metric() << " added to solution pool";
			m_signal_log(log_debug0, oss.str());
		}
		m_solutionpool.insert(r1);
	}
	// check for too many solutions in the pool
	while (!m_solutionpool.empty() && !m_solutionpool.begin()->is_existing(m_lgraph))
		m_solutionpool.erase(m_solutionpool.begin());
	{
		static const lgraphsolutionpool_t::size_type maxsz(16384);
		lgraphsolutionpool_t::size_type sz(m_solutionpool.size());
		if (sz > maxsz) {
			lgraphsolutionpool_t::size_type n(0);
			for (lgraphsolutionpool_t::iterator i(m_solutionpool.begin()), e(m_solutionpool.end()); i != e; ) {
				lgraphsolutionpool_t::iterator i2(i);
				++i;
				if (n >= maxsz || !i2->is_existing(m_lgraph)) {
					m_solutionpool.erase(i2);
					continue;
				}
				++n;
			}
		}
	}
	if (m_solutionpool.empty())
		return LRoute();
	return *m_solutionpool.begin();
}

void CFMUAutoroute45::lgraphroute(void)
{
	for (;;) {
		if (m_vertexdep >= boost::num_vertices(m_lgraph) || m_vertexdest >= boost::num_vertices(m_lgraph) ||
		    m_vertexdep == m_vertexdest) {
			m_signal_log(log_debug0, "Invalid departure and/or destination, stopping...");
			stop(statusmask_stoppingerrorenroute);
			return;
		}
		if (m_iterationcount[0] > m_maxlocaliteration || m_iterationcount[1] > m_maxremoteiteration) {
			stop(statusmask_stoppingerroriteration);
			return;
		}
		if (is_stopped())
			return;
		lgraphclearcurrent();
		LRoute r;
		{
			Glib::TimeVal tv;
			tv.assign_current_time();
			r = lgraphnextsolution();
			{
				Glib::TimeVal tv1;
				tv1.assign_current_time();
				tv = tv1 - tv;
			}
			if (true) {
				std::ostringstream oss;
				oss << "Next route computation: " << std::fixed << std::setprecision(3) << tv.as_double()
				    << "s, solution pool size " << m_solutionpool.size();
				m_signal_log(log_debug0, oss.str());
			}
		}
		if (r.size() < 2) {
			if (true) {
				std::ostringstream oss;
				oss << "No route found; "
				    << boost::out_degree(m_vertexdep, m_lgraph) << " SID edges, "
				    << boost::in_degree(m_vertexdest, m_lgraph) << " STAR edges";
				m_signal_log(log_normal, oss.str());
			}
			statusmask_t sm(statusmask_none);
			if (!boost::out_degree(m_vertexdep, m_lgraph))
				sm |= statusmask_stoppingerrorsid;
			if (!boost::in_degree(m_vertexdest, m_lgraph))
				sm |= statusmask_stoppingerrorstar;
			if (!sm)
				sm |= statusmask_stoppingerrorenroute;
			stop(sm);
			return;
		}
		if (!lgraphsetcurrent(r)) {
			stop(statusmask_stoppingerrorinternalerror);
			return;
		}
		if (r.is_repeated_nodes(m_lgraph)) {
			if (true) {
				std::ostringstream oss;
				oss << "Flight Plan has duplicate points " << lgraphprint(r);
				m_signal_log(log_debug0, oss.str());
			}
			continue;
		}
		m_signal_log(log_fplproposal, get_simplefpl());
		if (true) {
			std::ostringstream oss;
			oss << "Proposed metric " << r.get_metric() << " fpl " << get_simplefpl();
			m_signal_log(log_debug0, oss.str());
		}
		bool nochange(lgraphroutelocaltfr());
		if (is_stopped())
			return;
		if (nochange)
			break;
		m_signal_log(log_fpllocalvalidation, "");
		++m_iterationcount[0];
		if (true) {
			Glib::signal_idle().connect_once(sigc::mem_fun(*this, &CFMUAutoroute45::lgraphroute), Glib::PRIORITY_HIGH_IDLE);
			return;
		}
	}
	lgraphsubmitroute();
}

bool CFMUAutoroute45::lgraphroutelocaltfr(void)
{
	if (!get_tfr_available_enabled())
		return true;
	TrafficFlowRestrictions::Result res;
	bool work(lgraphlocaltfrcheck(res));
	bool nochange(res);
	if (nochange)
		return true;
	LGMandatory mrules(m_crossingmandatory);
	LGraph graph(m_lgraph);
	for (unsigned int mdepth = 0; mdepth < 3; ++mdepth) {
		LGMandatory mrules1(lgraphmandatoryroute(res));
		LRoute rbest;
		LGMandatory mrulesbest;
		LGraph graphbest;
		double metricbest(std::numeric_limits<double>::max());
		for (LGMandatory::const_iterator mi(mrules1.begin()), me(mrules1.end()); mi != me; ++mi) {
			LGMandatory mrules2(mrules);
			mrules2.insert(mrules2.end(), *mi);
			if (true) {
				m_signal_log(log_debug0, "Trying Mandatory Routing");
				for (unsigned int i = 0; i < mrules2.size(); ++i) {
					const LGMandatoryAlternative& alt(mrules2[i]);
					{
						std::ostringstream oss;
						oss << "  Mandatory Rule " << i << ' ' << alt.get_rule();
						m_signal_log(log_debug0, oss.str());
					}
					for (unsigned int j = 0; j < alt.size(); ++j) {
						const LGMandatorySequence& seq(alt[j]);
						std::ostringstream oss;
						oss << "    ";
						for (unsigned int k = 0; k < seq.size(); ++k) {
							const LGMandatoryPoint& pt(seq[k]);
							const LVertex& vv(m_lgraph[pt.get_v()]);
							oss << vv.get_ident();
							unsigned int pi0(pt.get_pi0());
							unsigned int pi1(pt.get_pi1());
							if (pi0 < m_performance.size() && pi1 < m_performance.size())
								oss << ' ' << m_performance.get_cruise(pi0).get_fplalt()
								    << ".." << m_performance.get_cruise(pi1).get_fplalt();
							if (k + 1 < seq.size())
								oss << ' ' << lgraphindex2airway(pt.get_airway(), true) << ' ';
						}
						m_signal_log(log_debug0, oss.str());
					}
				}
			}
			LRoute r(lgraphdijkstra(graph, LVertexLevel(m_vertexdep, 0), LVertexLevel(m_vertexdest, 0), LRoute(), false, mrules2));
			if (r.empty()) {
				m_signal_log(log_debug0, "No Mandatory Route found");
			} else {
				lgraphupdatemetric(r);
				if (true) {
					std::ostringstream oss;
					oss << "Mandatory Route " << lgraphprint(graph, r) << " metric " << r.get_metric();
					m_signal_log(log_debug0, oss.str());
				}
				if (r.get_metric() < metricbest) {
					metricbest = r.get_metric();
					mrulesbest = mrules2;
					rbest = r;
					graphbest = graph;
				}
			}
			if (is_stopped())
				return nochange;
			// check whether avoiding the mandatory rule instead of complying with it gives a better route
			for (LGMandatory::const_iterator mi(mrules1.begin()), me(mrules1.end()); mi != me; ++mi) {
				for (unsigned int i = 0; i < res.size(); ++i) {
					const TrafficFlowRestrictions::RuleResult& rule(res[i]);
					if (rule.is_disabled())
						continue;
					if (rule.get_codetype() != TrafficFlowRestrictions::RuleResult::codetype_mandatory)
						continue;
					if (rule.get_rule() != mi->get_rule())
						continue;
					if (rule.crossingcond_empty())
						continue;
					LGraph g(graph);
					bool work(false);
					for (unsigned int j = 0; j < rule.crossingcond_size(); ++j) {
						const TrafficFlowRestrictions::RuleResult::Alternative::Sequence&
							cc(rule.crossingcond(j));
						work = lgraphtfrrulekillforbidden(g, cc, false) || work;
					}
					if (!work)
						continue;
					r = lgraphdijkstra(g, LVertexLevel(m_vertexdep, 0), LVertexLevel(m_vertexdest, 0), LRoute(), false, mrules);
					if (r.empty()) {
						m_signal_log(log_debug0, "No Avoiding Mandatory Route found");
					} else {
						lgraphupdatemetric(r);
						if (true) {
							std::ostringstream oss;
							oss << "Avoiding Mandatory Route " << lgraphprint(r) << " metric " << r.get_metric();
							m_signal_log(log_debug0, oss.str());
						}
						if (r.get_metric() < metricbest) {
							metricbest = r.get_metric();
							mrulesbest = mrules;
							rbest = r;
							graphbest.swap(g);
						}
					}
				}
			}
			if (is_stopped())
				return nochange;
		}
		if (metricbest == std::numeric_limits<double>::max()) {
			// try to find a mandatory route that can be prevented by preventing its conditions
			for (LGMandatory::const_iterator mi(mrules1.begin()), me(mrules1.end()); mi != me; ++mi) {
				for (unsigned int i = 0; i < res.size(); ++i) {
					const TrafficFlowRestrictions::RuleResult& rule(res[i]);
					if (rule.is_disabled())
						continue;
					if (rule.get_codetype() != TrafficFlowRestrictions::RuleResult::codetype_mandatory)
						continue;
					if (rule.get_rule() != mi->get_rule())
						continue;
					if (rule.crossingcond_empty())
						continue;
					lgraphloglocaltfr(rule, false);
					bool work(false);
					for (unsigned int j = 0; j < rule.crossingcond_size(); ++j) {
						const TrafficFlowRestrictions::RuleResult::Alternative::Sequence&
							cc(rule.crossingcond(j));
						work = lgraphtfrrulekillforbidden(cc) || work;
					}
				}
			}
			break;
		}
		mrules.swap(mrulesbest);
		graph.swap(graphbest);
		lgraphinsertsolutiontree(rbest);
		if (!lgraphsetcurrent(rbest))
			break;
		m_signal_log(log_fplproposal, get_simplefpl());
		if (true) {
			std::ostringstream oss;
			oss << "Proposed metric " << rbest.get_metric() << " fpl " << get_simplefpl();
			m_signal_log(log_debug0, oss.str());
		}
		work = lgraphlocaltfrcheck(res) || work;
		nochange = res;
		if (nochange)
			break;
	}
	return nochange;
}

std::string CFMUAutoroute45::lgraphlocaltfrseqstr(const TrafficFlowRestrictions::RuleResult::Alternative::Sequence& el)
{
	std::ostringstream oss;
	oss << ' ' << el.get_type_string();
	switch (el.get_type()) {
	case TrafficFlowRestrictions::RuleResult::Alternative::Sequence::type_airway:
		oss << ' ' << el.get_ident();
		break;

	case TrafficFlowRestrictions::RuleResult::Alternative::Sequence::type_sid:
		oss << ' ' << el.get_ident() << ' ' << el.get_airport();
		break;

	case TrafficFlowRestrictions::RuleResult::Alternative::Sequence::type_star:
		oss << ' ' << el.get_ident() << ' ' << el.get_airport();
		break;

	case TrafficFlowRestrictions::RuleResult::Alternative::Sequence::type_airspace:
		oss << ' ' << el.get_ident() << '/'
		    << AirspacesDb::Airspace::get_class_string(el.get_bdryclass(), AirspacesDb::Airspace::typecode_ead);
		break;

	case TrafficFlowRestrictions::RuleResult::Alternative::Sequence::type_dct:
	case TrafficFlowRestrictions::RuleResult::Alternative::Sequence::type_point:
	default:
		break;
	}
	oss << " F" << std::setw(3) << std::setfill('0') << ((el.get_lower_alt() + 50) / 100);
	if (el.is_lower_alt_valid())
		oss << '*';
	oss << "...F"
	    << std::setw(3) << std::setfill('0') << ((std::min((int32_t)el.get_upper_alt(), (int32_t)99900) + 50) / 100);
	if (el.is_upper_alt_valid())
		oss << '*';
	if (el.get_type() == TrafficFlowRestrictions::RuleResult::Alternative::Sequence::type_airway ||
	    el.get_type() == TrafficFlowRestrictions::RuleResult::Alternative::Sequence::type_dct ||
	    el.get_type() == TrafficFlowRestrictions::RuleResult::Alternative::Sequence::type_point) {
		const TrafficFlowRestrictions::RuleResult::Alternative::Sequence::RulePoint& pt(el.get_point0());
		oss << ' ' << pt.get_ident();
	}
	if (el.get_type() == TrafficFlowRestrictions::RuleResult::Alternative::Sequence::type_airway ||
	    el.get_type() == TrafficFlowRestrictions::RuleResult::Alternative::Sequence::type_dct) {
		const TrafficFlowRestrictions::RuleResult::Alternative::Sequence::RulePoint& pt(el.get_point1());
		oss << ' ' << pt.get_ident();
	}
	return oss.str();
}

void CFMUAutoroute45::lgraphloglocaltfr(const TrafficFlowRestrictions::RuleResult& rule, bool logvalidation)
{
	typedef TrafficFlowRestrictions::RuleResult::set_t set_t;
	std::ostringstream oss;
	oss << rule.get_rule() << ' ';
	switch (rule.get_codetype()) {
	case TrafficFlowRestrictions::RuleResult::codetype_mandatory:
		oss << 'M';
		break;

	case TrafficFlowRestrictions::RuleResult::codetype_forbidden:
		oss << 'F';
		break;

	case TrafficFlowRestrictions::RuleResult::codetype_closed:
		oss << 'C';
		break;

	default:
		oss << '?';
		break;
	}
	if (rule.is_dct() || rule.is_unconditional() || rule.is_routestatic() || rule.is_mandatoryinbound() || rule.is_disabled()) {
		oss << " (";
		if(rule.is_disabled())
			oss << '-';
		if(rule.is_dct())
			oss << 'D';
		if (rule.is_unconditional())
			oss << 'U';
		if (rule.is_routestatic())
			oss << 'S';
		if (rule.is_mandatoryinbound())
			oss << 'I';
		oss << ')';
	}
	if (!rule.crossingcond_empty()) {
		oss << " X:";
		for (unsigned int i = 0; i < rule.crossingcond_size(); ++i)
			oss << lgraphlocaltfrseqstr(rule.crossingcond(i));
	}
	if (!rule.get_vertexset().empty()) {
		oss << " V:";
		bool subseq(false);
		for (set_t::const_iterator i(rule.get_vertexset().begin()), e(rule.get_vertexset().end()); i != e; ++i) {
			if (subseq)
				oss << ',';
			subseq = true;
			oss << *i;
		}
	}
	if (!rule.get_edgeset().empty()) {
		oss << " E:";
		bool subseq(false);
		for (set_t::const_iterator i(rule.get_edgeset().begin()), e(rule.get_edgeset().end()); i != e; ++i) {
			if (subseq)
				oss << ',';
			subseq = true;
			oss << *i;
		}
	}
	if (!rule.get_reflocset().empty()) {
		oss << " L:";
		bool subseq(false);
		for (set_t::const_iterator i(rule.get_reflocset().begin()), e(rule.get_reflocset().end()); i != e; ++i) {
			if (subseq)
				oss << ',';
			subseq = true;
			oss << *i;
		}
	}
	for (unsigned int i = 0; i < rule.size(); ++i) {
		const TrafficFlowRestrictions::RuleResult::Alternative& alt(rule[i]);
		if (i)
			oss << " /";
		if (!alt.get_vertexset().empty()) {
			oss << " V:";
			bool subseq(false);
			for (set_t::const_iterator i(alt.get_vertexset().begin()), e(alt.get_vertexset().end()); i != e; ++i) {
				if (subseq)
					oss << ',';
				subseq = true;
				oss << *i;
			}
		}
		if (!alt.get_edgeset().empty()) {
			oss << " E:";
			bool subseq(false);
			for (set_t::const_iterator i(alt.get_edgeset().begin()), e(alt.get_edgeset().end()); i != e; ++i) {
				if (subseq)
					oss << ',';
				subseq = true;
				oss << *i;
			}
		}
		for (unsigned int j = 0; j < alt.size(); ++j) {
			const TrafficFlowRestrictions::RuleResult::Alternative::Sequence& el(alt[j]);
			oss << lgraphlocaltfrseqstr(el);
		}
	}
	m_signal_log(log_graphrule, oss.str());
	m_signal_log(log_graphruledesc, rule.get_desc());
	m_signal_log(log_graphruleoprgoal, rule.get_oprgoal());
	m_signal_log(log_fpllocalvalidation, oss.str());
}

bool CFMUAutoroute45::lgraphlocaltfrcheck(TrafficFlowRestrictions::Result& res)
{
	if (!get_tfr_available_enabled()) {
		res = TrafficFlowRestrictions::Result();
		return false;
	}
	{
		Glib::TimeVal tv;
		tv.assign_current_time();
		res = m_tfr.check_fplan(m_route, 'G', m_aircraft.get_icaotype(), m_aircraft.get_equipment(), m_aircraft.get_pbn(),
					m_airportdb, m_navaiddb, m_waypointdb, m_airwaydb, m_airspacedb);
		{
			Glib::TimeVal tv1;
			tv1.assign_current_time();
			tv = tv1 - tv;
		}
		if (true) {
			std::ostringstream oss;
			oss << "Local TFR Check: " << std::fixed << std::setprecision(3) << tv.as_double()
			    << 's';
			m_signal_log(log_debug0, oss.str());
		}
	}
        // Parsed Flight Plan
        if (false) {
                for (unsigned int i = 0; i < m_route.get_nrwpt(); ++i) {
                        const FPlanWaypoint& wpt(m_route[i]);
                        std::ostringstream oss;
                        oss << "PFPL[" << i << "]: " << wpt.get_icao() << '/' << wpt.get_name() << ' ';
                        switch (wpt.get_pathcode()) {
                        case FPlanWaypoint::pathcode_sid:
                        case FPlanWaypoint::pathcode_star:
                        case FPlanWaypoint::pathcode_airway:
                                oss << wpt.get_pathname();
                                break;

                        case FPlanWaypoint::pathcode_directto:
                                oss << "DCT";
                                break;

                        default:
                                oss << '-';
                                break;
                        }
                        oss << ' ' << wpt.get_coord().get_lat_str2() << ' ' << wpt.get_coord().get_lon_str2()
                            << ' ' << (wpt.is_standard() ? 'F' : 'A') << std::setw(3) << std::setfill('0') << ((wpt.get_altitude() + 50) / 100)
                            << ' ' << (wpt.is_ifr() ? 'I' : 'V') << "FR";
			m_signal_log(log_debug0, oss.str());
		}
        }
	// Flight Plan
	if (false) {
                {
                        std::ostringstream oss;
                        oss << "FR: " << res.get_aircrafttype() << ' ' << res.get_aircrafttypeclass() << ' '
                            << res.get_equipment() << " PBN/" << res.get_pbn_string() << ' ' << res.get_typeofflight();
			m_signal_log(log_debug0, oss.str());
                }
                const TrafficFlowRestrictions::Fpl& fpl(res.get_fplan());
                for (unsigned int i = 0; i < fpl.size(); ++i) {
                        const TrafficFlowRestrictions::FplWpt& wpt(fpl[i]);
                        std::ostringstream oss;
                        oss << "F[" << i << '/' << wpt.get_wptnr() << "]: " << wpt.get_ident() << ' ' << wpt.get_type_string() << ' ';
                        switch (wpt.get_pathcode()) {
                        case FPlanWaypoint::pathcode_sid:
                        case FPlanWaypoint::pathcode_star:
                        case FPlanWaypoint::pathcode_airway:
                                oss << wpt.get_pathname();
                                break;

			case FPlanWaypoint::pathcode_directto:
                                oss << "DCT";
                                break;

                        default:
                                oss << '-';
                                break;
                        }
                        oss << ' ' << wpt.get_coord().get_lat_str2() << ' ' << wpt.get_coord().get_lon_str2()
                            << ' ' << (wpt.is_standard() ? 'F' : 'A') << std::setw(3) << std::setfill('0') << ((wpt.get_altitude() + 50) / 100)
                            << ' ' << wpt.get_icao() << '/' << wpt.get_name() << ' ' << (wpt.is_ifr() ? 'I' : 'V') << "FR";
			{
				time_t t(wpt.get_time_unix());
				struct tm tm;
				if (gmtime_r(&t, &tm)) {
					oss << ' ' << std::setw(2) << std::setfill('0') << tm.tm_hour
					    << ':' << std::setw(2) << std::setfill('0') << tm.tm_min;
					if (t >= 48 * 60 * 60)
						oss << ' ' << std::setw(4) << std::setfill('0') << (tm.tm_year + 1900)
						    << '-' << std::setw(2) << std::setfill('0') << (tm.tm_mon + 1)
						    << '-' << std::setw(2) << std::setfill('0') << tm.tm_mday;
				}
			}
			m_signal_log(log_debug0, oss.str());
                 }
        }
 	// Log Messages
	bool nochange(res);
	for (unsigned int i = 0; i < res.messages_size(); ++i) {
		const TrafficFlowRestrictions::Message& msg(res.get_message(i));
		logmessage(msg);
		if (!nochange)
			lgraphtfrmessage(msg, res.get_fplan());
	}
	bool work(false);
	for (unsigned int i = 0; i < res.size(); ++i) {
		const TrafficFlowRestrictions::RuleResult& rule(res[i]);
		lgraphloglocaltfr(rule, true);
		if (!nochange && !rule.is_disabled())
			work = lgraphtfrruleresult(rule, res.get_fplan()) || work;
	}
	m_signal_log(log_fpllocalvalidation, "");
	if (work)
		lgraphmodified();
	return work;
}

CFMUAutoroute45::LGMandatory CFMUAutoroute45::lgraphmandatoryroute(const TrafficFlowRestrictions::Result& res)
{
	// check whether the remaining violated rules are all mandatory and can be handled
	if (!res.size())
		return LGMandatory();
	LGMandatory m;
	for (unsigned int i = 0; i < res.size(); ++i) {
		const TrafficFlowRestrictions::RuleResult& rule(res[i]);
		if (rule.is_disabled())
			continue;
		if (rule.get_codetype() != TrafficFlowRestrictions::RuleResult::codetype_mandatory)
			return LGMandatory();
		if (rule.is_dct())
			return LGMandatory();
		m.push_back(LGMandatoryAlternative(rule.get_rule()));
		for (unsigned int i = 0; i < rule.size(); ++i) {
			const TrafficFlowRestrictions::RuleResult::Alternative& alt(rule[i]);
			if (!alt.size())
				continue;
			bool altok(true);
			LGMandatorySequence mseq;
			for (unsigned int j = 0; j < alt.size();) {
				const TrafficFlowRestrictions::RuleResult::Alternative::Sequence& seq(alt[j]);
				++j;
				LGraph::vertex_descriptor v0(~0U), v1(~0U);
				unsigned int pimin(std::numeric_limits<unsigned int>::max()), pimax(std::numeric_limits<unsigned int>::min());
				lgraphairwayindex_t awy(seq.get_type() == TrafficFlowRestrictions::RuleResult::Alternative::Sequence::type_dct ? airwaydct :
							seq.get_type() == TrafficFlowRestrictions::RuleResult::Alternative::Sequence::type_sid ? airwaysid :
							seq.get_type() == TrafficFlowRestrictions::RuleResult::Alternative::Sequence::type_star ? airwaystar :
							airwaymatchdctawy);
				switch (seq.get_type()) {
				default:
					altok = false;
					j = ~0U;
					break;

				case TrafficFlowRestrictions::RuleResult::Alternative::Sequence::type_airway:
					awy = lgraphairway2index(seq.get_ident(), false);
					if (awy >= airwaydct) {
						altok = false;
						j = ~0U;
						break;
					}
					// fall through

				case TrafficFlowRestrictions::RuleResult::Alternative::Sequence::type_dct:
					{
						const std::string& id(seq.get_point1().get_ident());
						double bestdist(std::numeric_limits<double>::max());
						for (LVerticesIdentMap::const_iterator i(m_lverticesidentmap.lower_bound(id)),
							     e(m_lverticesidentmap.upper_bound(id)); i != e; ++i) {
							LGraph::vertex_descriptor v(i->second);
							const LVertex& vv(m_lgraph[v]);
							double dist(vv.get_coord().simple_distance_nmi(seq.get_point1().get_coord()));
							if (!(dist < bestdist))
								continue;
							bestdist = dist;
							if (dist > 0.1)
								continue;
							v1 = v;
						}
					}
					if (v1 >= boost::num_edges(m_lgraph)) {
						altok = false;
						j = ~0U;
						break;
					}
					if (v1 == m_vertexdest && awy == airwaydct)
						awy = airwaystar;
					// fall through

				case TrafficFlowRestrictions::RuleResult::Alternative::Sequence::type_point:
					{
						const std::string& id(seq.get_point0().get_ident());
						double bestdist(std::numeric_limits<double>::max());
						for (LVerticesIdentMap::const_iterator i(m_lverticesidentmap.lower_bound(id)),
							     e(m_lverticesidentmap.upper_bound(id)); i != e; ++i) {
							LGraph::vertex_descriptor v(i->second);
							const LVertex& vv(m_lgraph[v]);
							double dist(vv.get_coord().simple_distance_nmi(seq.get_point0().get_coord()));
							if (!(dist < bestdist))
								continue;
							bestdist = dist;
							if (dist > 0.1)
								continue;
							v0 = v;
						}
					}
					if (v0 >= boost::num_edges(m_lgraph)) {
						altok = false;
						j = ~0U;
						break;
					}
					if (v0 == m_vertexdep && awy == airwaydct)
						awy = airwaysid;
				insertaltpts:
					for (unsigned int pi = 0; pi < m_performance.size(); ++pi) {
						const Performance::Cruise& cruise(m_performance.get_cruise(pi));
						if (cruise.get_altitude() < seq.get_lower_alt() || cruise.get_altitude() > seq.get_upper_alt())
							continue;
						pimin = std::min(pimin, pi);
 						pimax = std::max(pimax, pi);
					}
					if (pimin > pimax) {
						altok = false;
						j = ~0U;
						break;
					}
					mseq.push_back(LGMandatoryPoint(v0, pimin, pimax, awy));
					if (v1 < boost::num_edges(m_lgraph))
						mseq.push_back(LGMandatoryPoint(v1, pimin, pimax, airwaymatchnone));
					break;

				case TrafficFlowRestrictions::RuleResult::Alternative::Sequence::type_sid:
					if (seq.get_airport() != get_departure().get_icao()) {
						altok = false;
						j = ~0U;
						break;
					}
					v0 = m_vertexdest;
					{
						std::string id(seq.get_ident());
						{
							std::string::size_type n(id.find_first_of("0123456789"));
							if (n != std::string::npos)
								id.erase(n);
						}
						double bestdist(std::numeric_limits<double>::max());
						for (LVerticesIdentMap::const_iterator i(m_lverticesidentmap.lower_bound(id)),
							     e(m_lverticesidentmap.upper_bound(id)); i != e; ++i) {
							LGraph::vertex_descriptor v(i->second);
							const LVertex& vv(m_lgraph[v]);
							double dist(vv.get_coord().simple_distance_nmi(get_departure().get_coord()));
							if (!(dist < bestdist))
								continue;
							bestdist = dist;
							if (dist > 500.)
								continue;
							v1 = v;
						}
					}
					if (v1 >= boost::num_edges(m_lgraph)) {
						altok = false;
						j = ~0U;
						break;
					}
					goto insertaltpts;

				case TrafficFlowRestrictions::RuleResult::Alternative::Sequence::type_star:
					if (seq.get_airport() != get_destination().get_icao()) {
						altok = false;
						j = ~0U;
						break;
					}
					v1 = m_vertexdest;
					{
						std::string id(seq.get_ident());
						{
							std::string::size_type n(id.find_first_of("0123456789"));
							if (n != std::string::npos)
								id.erase(n);
						}
						double bestdist(std::numeric_limits<double>::max());
						for (LVerticesIdentMap::const_iterator i(m_lverticesidentmap.lower_bound(id)),
							     e(m_lverticesidentmap.upper_bound(id)); i != e; ++i) {
							LGraph::vertex_descriptor v(i->second);
							const LVertex& vv(m_lgraph[v]);
							double dist(vv.get_coord().simple_distance_nmi(get_destination().get_coord()));
							if (!(dist < bestdist))
								continue;
							bestdist = dist;
							if (dist > 500.)
								continue;
							v0 = v;
						}
					}
					if (v0 >= boost::num_edges(m_lgraph)) {
						altok = false;
						j = ~0U;
						break;
					}
					goto insertaltpts;
				}
			}
			if (!altok || mseq.empty())
				continue;
			for (unsigned int i = 1, n = mseq.size(); i < n;) {
				if (mseq[i-1].get_v() != mseq[i].get_v()) {
					++i;
					continue;
				}
				mseq.erase(mseq.begin() + (i - 1));
				n = mseq.size();
			}
			m.back().push_back(mseq);
		}
		if (m.back().empty())
			return LGMandatory();
	}
	return m;
}

void CFMUAutoroute45::lgraphsubmitroute(void)
{
	if (m_route.get_flightrules() == 'V') {
		m_validationresponse.push_back("NO ERRORS");
		m_signal_log(log_fpllocalvalidation, m_validationresponse.back());
		m_signal_log(log_fpllocalvalidation, "");
		parse_response();
		return;
	}
	m_tvvalidatestart.assign_current_time();
	++m_iterationcount[1];
	if (!child_is_running()) {
		child_close();
		child_run();
	}
	if (!child_is_running()) {
		m_signal_log(log_debug0, "Cannot restart child, stopping...");
		stop(statusmask_stoppingerrorvalidatortimeout);
		return;
	}
	child_send(get_simplefpl() + "\n");
	m_childtimeoutcount = 0;
	m_connchildtimeout.disconnect();
	m_connchildtimeout = Glib::signal_timeout().connect_seconds(sigc::mem_fun(*this, &CFMUAutoroute45::child_timeout), child_get_timeout());
}

bool CFMUAutoroute45::lgraphtfrrulekillforbidden(LGraph& g, const TrafficFlowRestrictions::RuleResult::Alternative::Sequence& seq, bool log)
{
	switch (seq.get_type()) {
	case TrafficFlowRestrictions::RuleResult::Alternative::Sequence::type_airspace:
	{
		opendb();
		AirspaceCache aspccache(m_airspacedb);
		const AirspacesDb::Airspace& aspc(aspccache.find_airspace(seq.get_ident(), seq.get_bdryclass(), AirspacesDb::Airspace::typecode_ead));
		if (!aspc.is_valid())
			return false;
		Rect bbox(aspc.get_bbox());
		unsigned int pi0(~0U), pi1(~0U);
		{
			int32_t altlwr(aspc.get_altlwr());
			int32_t altupr(aspc.get_altupr());
			if (seq.is_lower_alt_valid())
				altlwr = seq.get_lower_alt();
			if (seq.is_upper_alt_valid())
				altupr = seq.get_upper_alt();
			for (unsigned int pi = 0; pi < m_performance.size(); ++pi) {
				const Performance::Cruise& cruise(m_performance.get_cruise(pi));
				if (cruise.get_altitude() < altlwr || cruise.get_altitude() > altupr)
					continue;
				if (pi0 == ~0U)
					pi0 = pi;
				pi1 = pi;
			}
		}
		if (log) {
			std::ostringstream oss;
			oss << "lgraphtfrruleresult: excluding airspace " << aspc.get_icao()
			    << '/' << aspc.get_class_string() << " F" << std::setw(3) << std::setfill('0') << (aspc.get_altlwr() / 100)
			    << "..F" << std::setw(3) << std::setfill('0') << ((aspc.get_altupr() + 99) / 100);
			if (pi0 != ~0U)
				oss << " (" << m_performance.get_cruise(pi0).get_fplalt()
				    << ".." << m_performance.get_cruise(pi1).get_fplalt()
				    << ')';
			m_signal_log(log_debug0, oss.str());
		}
		if (pi0 == ~0U)
			return false;
		Glib::TimeVal tv;
		tv.assign_current_time();
		unsigned int edgecnt(0);
		LGraph::vertex_iterator v0i, ve;
		for (boost::tie(v0i, ve) = boost::vertices(g); v0i != ve; ++v0i) {
			if (*v0i == m_vertexdep || *v0i == m_vertexdest)
				continue;
			const LVertex& vv0(g[*v0i]);
			LGraph::vertex_iterator v1i(v0i);
			for (++v1i; v1i != ve; ++v1i) {
				if (*v1i == m_vertexdep || *v1i == m_vertexdest)
					continue;
				const LVertex& vv1(g[*v1i]);
				{
					Rect r(vv0.get_coord(), vv0.get_coord());
					r = r.add(vv1.get_coord());
					if (!r.is_intersect(bbox)) {
						if (false && (vv0.get_ident() == "UTOPO" || vv1.get_ident() == "UTOPO")) {
							std::ostringstream oss;
							oss << "lgraphtfrruleresult: " << aspc.get_icao()
							    << '/' << aspc.get_class_string() << ' '
							    << vv0.get_ident() << " <-> " << vv1.get_ident()
							    << " does not intersect airspace?""? " << bbox << " / " << r;
							m_signal_log(log_debug0, oss.str());
						}
						continue;
					}
				}
				IntervalSet<int32_t> altrange;
				bool altrangenotcalc(true);
				LGraph::out_edge_iterator e0, e1;
				for (tie(e0, e1) = out_edges(*v0i, g); e0 != e1; ++e0) {
					if (target(*e0, g) != *v1i)
						continue;
					if (altrangenotcalc) {
						altrange = aspccache.get_altrange(aspc, vv0.get_coord(), vv1.get_coord(),
										  seq.get_lower_alt_if_valid(),
										  seq.get_upper_alt_if_valid());
						altrangenotcalc = false;
						if (false && (vv0.get_ident() == "UTOPO" || vv1.get_ident() == "UTOPO")) {
							std::ostringstream oss;
							oss << "lgraphtfrruleresult: " << aspc.get_icao()
							    << '/' << aspc.get_class_string() << ' '
							    << vv0.get_ident() << " <-> " << vv1.get_ident()
							    << ": " << altrange.to_str();
							m_signal_log(log_debug0, oss.str());
						}
					}
					LEdge& ee(g[*e0]);
					for (unsigned int pi = pi0; pi <= pi1; ++pi) {
						const Performance::Cruise& cruise(m_performance.get_cruise(pi));
						if (!altrange.is_inside(cruise.get_altitude()))
							break;
						if (ee.is_valid(pi))
							++edgecnt;
						ee.set_metric(pi, LEdge::invalidmetric);
					}
				}
				for (tie(e0, e1) = out_edges(*v1i, g); e0 != e1; ++e0) {
					if (target(*e0, g) != *v0i)
						continue;
					if (altrangenotcalc) {
						altrange = aspccache.get_altrange(aspc, vv0.get_coord(), vv1.get_coord(),
										  seq.get_lower_alt_if_valid(),
										  seq.get_upper_alt_if_valid());
						altrangenotcalc = false;
						if (false && (vv0.get_ident() == "UTOPO" || vv1.get_ident() == "UTOPO")) {
							std::ostringstream oss;
							oss << "lgraphtfrruleresult: " << aspc.get_icao()
							    << '/' << aspc.get_class_string() << ' '
							    << vv0.get_ident() << " <-> " << vv1.get_ident()
							    << ": " << altrange.to_str();
							m_signal_log(log_debug0, oss.str());
						}
					}
					LEdge& ee(g[*e0]);
					for (unsigned int pi = pi0; pi <= pi1; ++pi) {
						const Performance::Cruise& cruise(m_performance.get_cruise(pi));
						if (!altrange.is_inside(cruise.get_altitude()))
							break;
						if (ee.is_valid(pi))
							++edgecnt;
						ee.set_metric(pi, LEdge::invalidmetric);
					}
				}
			}
		}
		if (log) {
			Glib::TimeVal tv1;
			tv1.assign_current_time();
			tv1 -= tv;
			std::ostringstream oss;
			oss << "lgraphtfrruleresult: excluding airspace " << aspc.get_icao()
			    << '/' << aspc.get_class_string() << ": " << edgecnt << " edges removed, "
			    << std::fixed << std::setprecision(3) << tv1.as_double() << 's';
			m_signal_log(log_debug0, oss.str());
		}
		return !!edgecnt;
	}

	case TrafficFlowRestrictions::RuleResult::Alternative::Sequence::type_airway:
	case TrafficFlowRestrictions::RuleResult::Alternative::Sequence::type_dct:
	case TrafficFlowRestrictions::RuleResult::Alternative::Sequence::type_point:
	{
		const unsigned int pis(m_performance.size());
		if (!pis)
			return false;
		unsigned int pi0(0), pi1(pis);
		if (seq.is_lower_alt_valid()) {
			while (pi0 < pis) {
				const Performance::Cruise& cruise(m_performance.get_cruise(pi0));
				if (cruise.get_altitude() >= seq.get_lower_alt())
					break;
				++pi0;
			}
		}
		if (seq.is_upper_alt_valid()) {
			while (pi1 > 0) {
				const Performance::Cruise& cruise(m_performance.get_cruise(pi1 - 1));
				if (cruise.get_altitude() <= seq.get_upper_alt())
					break;
				--pi1;
			}
		}
		if (pi0 >= pi1)
			return false;
		unsigned int edgecnt(0);
		for (LVerticesIdentMap::const_iterator i(m_lverticesidentmap.lower_bound(seq.get_point0().get_ident())),
                             e(m_lverticesidentmap.upper_bound(seq.get_point0().get_ident())); i != e; ++i) {
                        LGraph::vertex_descriptor u(i->second);
                        const LVertex& uu(g[u]);
                        if (uu.get_coord().simple_distance_nmi(seq.get_point0().get_coord()) >= 0.1)
				continue;
			switch (seq.get_type()) {
			case TrafficFlowRestrictions::RuleResult::Alternative::Sequence::type_point:
			{
				{
					LGraph::out_edge_iterator e0, e1;
					for (boost::tie(e0, e1) = boost::out_edges(u, g); e0 != e1; ++e0) {
						const LEdge& ee(g[*e0]);
						for (unsigned int pi = pi0; pi < pi1; ++pi) {
							if (!ee.is_valid(pi))
								continue;
							lgraphkilledge(g, *e0, pi, log);
							++edgecnt;
						}
					}
				}
				{
					LGraph::in_edge_iterator e0, e1;
					for (boost::tie(e0, e1) = boost::in_edges(u, g); e0 != e1; ++e0) {
						LEdge& ee(g[*e0]);
						for (unsigned int pi = pi0; pi < pi1; ++pi) {
							if (!ee.is_valid(pi))
								continue;
							lgraphkilledge(g, *e0, pi, log);
							++edgecnt;
						}
					}
				}
				break;
			}

			case TrafficFlowRestrictions::RuleResult::Alternative::Sequence::type_dct:
			{
				LGraph::out_edge_iterator e0, e1;
				for (boost::tie(e0, e1) = boost::out_edges(u, g); e0 != e1; ++e0) {
					const LEdge& ee(g[*e0]);
					if (!ee.is_dct())
						continue;
					LGraph::vertex_descriptor v(boost::target(*e0, g));
					const LVertex& vv(g[v]);
					if (vv.get_ident() != seq.get_point1().get_ident() ||
					    vv.get_coord().simple_distance_nmi(seq.get_point1().get_coord()) >= 0.1)
						continue;
					for (unsigned int pi = pi0; pi < pi1; ++pi) {
						if (!ee.is_valid(pi))
							continue;
						lgraphkilledge(g, *e0, pi, log);
						++edgecnt;
					}
				}
				break;
			}

			case TrafficFlowRestrictions::RuleResult::Alternative::Sequence::type_airway:
			{
				lgraphairwayindex_t awy(lgraphairway2index(seq.get_ident(), false));
				if (awy >= airwaydct)
					break;
				LGraphAwyEdgeFilter filter(g, awy);
				typedef boost::filtered_graph<LGraph, LGraphAwyEdgeFilter> fg_t;
				fg_t fg(g, filter);
				for (LVerticesIdentMap::const_iterator i(m_lverticesidentmap.lower_bound(seq.get_point1().get_ident())),
					     e(m_lverticesidentmap.upper_bound(seq.get_point1().get_ident())); i != e; ++i) {
					LGraph::vertex_descriptor v(i->second);
					const LVertex& vv(g[v]);
					if (vv.get_coord().simple_distance_nmi(seq.get_point1().get_coord()) >= 0.1)
						continue;
					std::vector<LGraph::vertex_descriptor> predecessors(boost::num_vertices(g), 0);
					dijkstra_shortest_paths(fg, u, boost::weight_map(boost::get(&LEdge::m_dist, g)).
								predecessor_map(&predecessors[0]));
					for (;;) {
						LGraph::vertex_descriptor uu(predecessors[v]);
						if (uu == v || u == v)
							break;
						LGraph::out_edge_iterator e0, e1;
						for (boost::tie(e0, e1) = boost::out_edges(uu, g); e0 != e1; ++e0) {
							if (boost::target(*e0, g) != v)
								continue;
							const LEdge& ee(g[*e0]);
							if (!ee.is_match(awy))
								continue;
							for (unsigned int pi = pi0; pi < pi1; ++pi) {
								if (!ee.is_valid(pi))
									continue;
								lgraphkilledge(g, *e0, pi, log);
								++edgecnt;
							}
						}
						v = uu;
					}
				}
				break;
			}

			default:
				break;
			}
		}
		return !!edgecnt;
	}

	default:
		return false;
	}
}

IntervalSet<int32_t> CFMUAutoroute45::lgraphcrossinggate(const TrafficFlowRestrictions::RuleResult::Alternative::Sequence& seq,
						       const TrafficFlowRestrictions::RuleResult::Alternative::Sequence& cc)
{
	switch (seq.get_type()) {
	case TrafficFlowRestrictions::RuleResult::Alternative::Sequence::type_airway:
	case TrafficFlowRestrictions::RuleResult::Alternative::Sequence::type_dct:
	case TrafficFlowRestrictions::RuleResult::Alternative::Sequence::type_point:
		switch (cc.get_type()) {
		case TrafficFlowRestrictions::RuleResult::Alternative::Sequence::type_airspace:
		{
			opendb();
			AirspaceCache aspccache(m_airspacedb);
			const AirspacesDb::Airspace& aspc(aspccache.find_airspace(cc.get_ident(), cc.get_bdryclass(), AirspacesDb::Airspace::typecode_ead));
			if (!aspc.is_valid())
				return IntervalSet<int32_t>();
			IntervalSet<int32_t> altrange(aspccache.get_altrange(aspc, seq.get_point0().get_coord(),
									     cc.get_lower_alt_if_valid(), cc.get_upper_alt_if_valid()));
			if (seq.get_type() == TrafficFlowRestrictions::RuleResult::Alternative::Sequence::type_airway ||
			    seq.get_type() == TrafficFlowRestrictions::RuleResult::Alternative::Sequence::type_dct)
				altrange |= aspccache.get_altrange(aspc, seq.get_point1().get_coord(),
								   cc.get_lower_alt_if_valid(), cc.get_upper_alt_if_valid());
			if (seq.is_lower_alt_valid() || seq.is_upper_alt_valid()) {
				int32_t smin(std::numeric_limits<int32_t>::min());
				int32_t smax(std::numeric_limits<int32_t>::max());
				if (seq.is_lower_alt_valid())
					smin = seq.get_lower_alt();
				if (seq.is_upper_alt_valid())
					smax = seq.get_upper_alt() + 1;
				altrange &= IntervalSet<int32_t>::Intvl(smin, smax);
			}
			return altrange;
		}

		case TrafficFlowRestrictions::RuleResult::Alternative::Sequence::type_airway:
		case TrafficFlowRestrictions::RuleResult::Alternative::Sequence::type_dct:
		case TrafficFlowRestrictions::RuleResult::Alternative::Sequence::type_point:
		{
			{
				bool peq = (seq.get_point0().get_ident() == cc.get_point0().get_ident() &&
					    seq.get_point0().get_type() == cc.get_point0().get_type() &&
					    seq.get_point0().get_coord().simple_distance_nmi(cc.get_point0().get_coord()) <= 0.1);
				if (!peq && cc.get_type() == TrafficFlowRestrictions::RuleResult::Alternative::Sequence::type_point &&
				    (seq.get_type() == TrafficFlowRestrictions::RuleResult::Alternative::Sequence::type_airway ||
				     seq.get_type() == TrafficFlowRestrictions::RuleResult::Alternative::Sequence::type_dct))
					peq = (seq.get_point1().get_ident() == cc.get_point0().get_ident() &&
					       seq.get_point1().get_type() == cc.get_point0().get_type() &&
					       seq.get_point1().get_coord().simple_distance_nmi(cc.get_point0().get_coord()) <= 0.1);
				if (!peq)
					return IntervalSet<int32_t>();
			}
			int32_t altlwr(std::numeric_limits<int32_t>::min());
			int32_t altupr(std::numeric_limits<int32_t>::max());
			if (seq.is_lower_alt_valid())
				altlwr = seq.get_lower_alt();
			if (seq.is_upper_alt_valid())
				altupr = std::min(std::numeric_limits<int32_t>::max() - 1, seq.get_upper_alt()) + 1;
			if (cc.is_lower_alt_valid() && cc.get_lower_alt() > altlwr)
				altlwr = cc.get_lower_alt();
			if (cc.is_upper_alt_valid() && cc.get_upper_alt() < altupr)
				altupr = std::min(std::numeric_limits<int32_t>::max() - 1, cc.get_upper_alt()) + 1;
			if (cc.get_type() == TrafficFlowRestrictions::RuleResult::Alternative::Sequence::type_point)
				return IntervalSet<int32_t>(IntervalSet<int32_t>::Intvl(altlwr, altupr));
			if (seq.get_type() == TrafficFlowRestrictions::RuleResult::Alternative::Sequence::type_point ||
			    seq.get_type() != cc.get_type())
				return IntervalSet<int32_t>();
			if (seq.get_point1().get_ident() != cc.get_point1().get_ident() ||
			    seq.get_point1().get_type() != cc.get_point1().get_type() ||
			    seq.get_point1().get_coord().simple_distance_nmi(cc.get_point1().get_coord()) > 0.1)
				return IntervalSet<int32_t>();
			if (seq.get_type() == TrafficFlowRestrictions::RuleResult::Alternative::Sequence::type_dct)
				return IntervalSet<int32_t>(IntervalSet<int32_t>::Intvl(altlwr, altupr));
			if (seq.get_ident() != cc.get_ident())
				return IntervalSet<int32_t>();
			return IntervalSet<int32_t>(IntervalSet<int32_t>::Intvl(altlwr, altupr));
		}

		default:
			return IntervalSet<int32_t>();
		}

	default:
		return IntervalSet<int32_t>();
	}
}

bool CFMUAutoroute45::lgraphtfrruleresult(const TrafficFlowRestrictions::RuleResult& rule, const TrafficFlowRestrictions::Fpl& fpl)
{
	if (rule.is_disabled())
		return false;
	// actually modify the graph
	typedef TrafficFlowRestrictions::RuleResult::set_t set_t;
	switch (rule.get_codetype()) {
	case TrafficFlowRestrictions::RuleResult::codetype_forbidden:
	{
		bool work(false);
		for (unsigned int j = 0; j < rule.size(); ++j) {
			const TrafficFlowRestrictions::RuleResult::Alternative& alt(rule[j]);
			if (alt.size() == 1 && rule.is_routestatic()) {
				work = lgraphtfrrulekillforbidden(alt[0]) || work;
				continue;
			}
			for (unsigned int k = 0; k < rule.crossingcond_size(); ++k) {
				const TrafficFlowRestrictions::RuleResult::Alternative::Sequence& seq(alt[0]);
				IntervalSet<int32_t> altrange(lgraphcrossinggate(seq, rule.crossingcond(k)));
				if (false && rule.get_rule() == "ED2202A") {
					std::ostringstream oss;
					oss << "lgraphtfrruleresult: " << rule.get_rule() << " crossingcond "
					    << lgraphlocaltfrseqstr(rule.crossingcond(k)) << " restriction "
					    << lgraphlocaltfrseqstr(seq) << " result " << altrange.to_str();
					m_signal_log(log_debug0, oss.str());
				}
				for (IntervalSet<int32_t>::const_iterator ii(altrange.begin()), ie(altrange.end()); ii != ie; ++ii) {
					TrafficFlowRestrictions::RuleResult::Alternative::Sequence
						seq1(seq.get_type(), seq.get_point0(), seq.get_point1(),
						     ii->get_lower(), ii->get_upper() - 1,
						     seq.get_ident(), seq.get_airport(), seq.get_bdryclass(),
						     true, true);
					work = lgraphtfrrulekillforbidden(seq1) || work;
				}	
			}
		}
		if (work) {
			lgraphmodified();
			return true;
		}
		for (unsigned int j = 0; j < rule.size(); ++j) {
			const TrafficFlowRestrictions::RuleResult::Alternative& alt(rule[j]);
			if (rule.is_routestatic()) {
				if (alt.size() == 1 && lgraphtfrrulekillforbidden(alt[0])) {
					work = true;
					continue;
				}
				for (set_t::const_iterator it(alt.get_edgeset().begin()), e(alt.get_edgeset().end()); it != e; ++it) {
					unsigned int i(*it);
					const TrafficFlowRestrictions::FplWpt& wpt0(fpl[i]);
					const TrafficFlowRestrictions::FplWpt& wpt1(fpl[i + 1U]);
					work = lgraphdisconnectsolutionedge(wpt0.get_ident(), wpt1.get_ident()) || work;
				}
				if (!alt.get_edgeset().empty())
					continue;
			}
			if (alt.get_edgeset().empty()) {
				unsigned int i;
				{
					set_t::const_iterator it(alt.get_vertexset().begin()), e(alt.get_vertexset().end());
					if (it == e)
						continue;
					i = *it;
					++it;
					if (it != e)
						continue;
				}
				if (i >= fpl.size())
					continue;
				if (!rule.get_edgeset().empty())
					continue;
				{
					set_t vset, vset1;
					vset1.insert(i);
					std::set_difference(alt.get_vertexset().begin(), alt.get_vertexset().end(), vset1.begin(), vset1.end(),
							    std::insert_iterator<set_t>(vset, vset.begin()));
					if (!vset.empty())
						continue;
					vset.clear();
					vset1.insert(0U);
					vset1.insert(fpl.size() - 1U);
					std::set_difference(rule.get_vertexset().begin(), rule.get_vertexset().end(), vset1.begin(), vset1.end(),
							    std::insert_iterator<set_t>(vset, vset.begin()));
					if (!vset.empty())
						continue;
				}
				const TrafficFlowRestrictions::FplWpt& wpt(fpl[i]);
				work = lgraphdisconnectsolutionvertex(wpt.get_ident()) || work;
				continue;
			}
			unsigned int i;
			{
				set_t::const_iterator it(alt.get_edgeset().begin()), e(alt.get_edgeset().end());
				if (it == e)
					continue;
				i = *it;
				++it;
				if (it != e)
					continue;
			}
			if (i + 1U >= fpl.size())
				continue;
			{
				set_t vset, vset1;
				vset1.insert(i);
				vset1.insert(i + 1U);
				std::set_difference(alt.get_vertexset().begin(), alt.get_vertexset().end(), vset1.begin(), vset1.end(),
						    std::insert_iterator<set_t>(vset, vset.begin()));
				if (!vset.empty())
					continue;
				vset.clear();
				vset1.insert(0U);
				vset1.insert(fpl.size() - 1U);
				std::set_difference(rule.get_vertexset().begin(), rule.get_vertexset().end(), vset1.begin(), vset1.end(),
						    std::insert_iterator<set_t>(vset, vset.begin()));
				if (!vset.empty())
					continue;
				vset.clear();
				std::set_difference(rule.get_edgeset().begin(), rule.get_edgeset().end(),
						    alt.get_edgeset().begin(), alt.get_edgeset().end(),
						    std::insert_iterator<set_t>(vset, vset.begin()));
				if (!vset.empty())
					continue;
			}
			const TrafficFlowRestrictions::FplWpt& wpt0(fpl[i]);
			const TrafficFlowRestrictions::FplWpt& wpt1(fpl[i + 1U]);
			work = lgraphdisconnectsolutionedge(wpt0.get_ident(), wpt1.get_ident()) || work;
		}
		if (work) {
			lgraphmodified();
			return true;
		}
		break;
	}

	case TrafficFlowRestrictions::RuleResult::codetype_closed:
	{
		{
			bool work(false);
		        for (unsigned int j = 0; j < rule.crossingcond_size(); ++j) {
				const TrafficFlowRestrictions::RuleResult::Alternative::Sequence& cc(rule.crossingcond(j));
				work = lgraphtfrrulekillforbidden(cc, true) || work;
			}
			if (work) {
				lgraphmodified();
				return true;
			}
		}
		if (rule.get_edgeset().empty()) {
			unsigned int i;
			{
				set_t vset, vset1;
				vset1.insert(0U);
				vset1.insert(fpl.size() - 1U);
				std::set_difference(rule.get_vertexset().begin(), rule.get_vertexset().end(), vset1.begin(), vset1.end(),
						    std::insert_iterator<set_t>(vset, vset.begin()));
				set_t::const_iterator it(vset.begin()), e(vset.end());
				if (it == e)
					break;
				i = *it;
				++it;
				if (it != e)
					break;
			}
			if (i >= fpl.size())
				break;
			const TrafficFlowRestrictions::FplWpt& wpt(fpl[i]);
			if (lgraphdisconnectsolutionvertex(wpt.get_ident())) {
				lgraphmodified();
				return true;
			}
			break;
		}
		unsigned int i;
		{
			set_t::const_iterator it(rule.get_edgeset().begin()), e(rule.get_edgeset().end());
			if (it == e)
				break;
			i = *it;
			++it;
			if (it != e)
				break;
		}
		if (i + 1U >= fpl.size())
			break;
		{
			set_t vset, vset1;
			vset1.insert(i);
			vset1.insert(i + 1U);
			vset1.insert(0U);
			vset1.insert(fpl.size() - 1U);
			std::set_difference(rule.get_vertexset().begin(), rule.get_vertexset().end(), vset1.begin(), vset1.end(),
					    std::insert_iterator<set_t>(vset, vset.begin()));
			if (!vset.empty())
				break;
		}
		const TrafficFlowRestrictions::FplWpt& wpt0(fpl[i]);
		const TrafficFlowRestrictions::FplWpt& wpt1(fpl[i + 1U]);
		if (lgraphdisconnectsolutionedge(wpt0.get_ident(), wpt1.get_ident())) {
			lgraphmodified();
			return true;
		}
		break;
	}

	default:
	case TrafficFlowRestrictions::RuleResult::codetype_mandatory:
		if (rule.is_mandatoryinbound()) {
			const unsigned int pis(m_performance.size());
			bool work(false);
			for (unsigned int j = 0; j < rule.size(); ++j) {
				const TrafficFlowRestrictions::RuleResult::Alternative& alt(rule[j]);
				if (!alt.size())
					continue;
				const TrafficFlowRestrictions::RuleResult::Alternative::Sequence& seq(alt[alt.size()-1]);
				if (seq.get_type() != TrafficFlowRestrictions::RuleResult::Alternative::Sequence::type_dct &&
				    seq.get_type() != TrafficFlowRestrictions::RuleResult::Alternative::Sequence::type_airway)
					continue;
				for (LVerticesIdentMap::const_iterator i(m_lverticesidentmap.lower_bound(seq.get_point1().get_ident())),
					     e(m_lverticesidentmap.upper_bound(seq.get_point1().get_ident())); i != e; ++i) {
					LGraph::vertex_descriptor v(i->second);
					const LVertex& vv(m_lgraph[v]);
					if (vv.get_coord().simple_distance_nmi(seq.get_point1().get_coord()) >= 0.1)
						continue;
					LGraph::in_edge_iterator e0, e1;
					for (boost::tie(e0, e1) = boost::in_edges(v, m_lgraph); e0 != e1; ++e0) {
						LEdge& ee(m_lgraph[*e0]);
						for (unsigned int pi = 0; pi < pis; ++pi) {
							if (!ee.is_valid(pi))
								continue;
							bool ok(false);
							for (unsigned int k = 0; k < rule.size(); ++k) {
								const TrafficFlowRestrictions::RuleResult::Alternative& alt(rule[k]);
								if (!alt.size())
									continue;
								const TrafficFlowRestrictions::RuleResult::Alternative::Sequence& seq(alt[alt.size()-1]);
								switch (seq.get_type()) {
								case TrafficFlowRestrictions::RuleResult::Alternative::Sequence::type_dct:
									if (!ee.is_dct())
										continue;
									break;

								case TrafficFlowRestrictions::RuleResult::Alternative::Sequence::type_airway:
								{
									lgraphairwayindex_t awy(lgraphairway2index(seq.get_ident(), false));
									if (awy >= airwaydct || !ee.is_match(awy))
										continue;
									break;
								}

								default:
									continue;
								}
								if (seq.get_point1().get_ident() != vv.get_ident() ||
								    vv.get_coord().simple_distance_nmi(seq.get_point1().get_coord()) >= 0.1)
									continue;
								const Performance::Cruise& cruise(m_performance.get_cruise(pi));
								if (seq.is_lower_alt_valid() && cruise.get_altitude() < seq.get_lower_alt())
									continue;
								if (seq.is_upper_alt_valid() && cruise.get_altitude() > seq.get_upper_alt())
									continue;
								ok = true;
								break;
							}
							if (ok)
								continue;
							lgraphkilledge(*e0, pi, true);
							work = true;
						}
					}
				}
			}
			if (work) {
				lgraphmodified();
				return true;
			}
		}
		if (rule.is_dct()) {
			bool work(false);
			for (set_t::const_iterator it(rule.get_edgeset().begin()), e(rule.get_edgeset().end()); it != e; ++it) {
				unsigned int i(*it);
				if (i + 1U >= fpl.size())
					continue;
				const TrafficFlowRestrictions::FplWpt& wpt0(fpl[i]);
				const TrafficFlowRestrictions::FplWpt& wpt1(fpl[i + 1U]);
				work = lgraphdisconnectsolutionedge(wpt0.get_ident(), wpt1.get_ident(), airwaydct) || work;
			}
			if (work) {
				lgraphmodified();
				return true;
			}
		}
		break;
	}
	double scale(localforbiddenpenalty);
	bool work(false);
	switch (rule.get_codetype()) {
	case TrafficFlowRestrictions::RuleResult::codetype_mandatory:
		scale = 1.0 / localforbiddenpenalty;
		// fall through
		
	case TrafficFlowRestrictions::RuleResult::codetype_forbidden:
		for (unsigned int j = 0; j < rule.size(); ++j) {
			const TrafficFlowRestrictions::RuleResult::Alternative& alt(rule[j]);
			if (alt.get_edgeset().empty()) {
				for (set_t::const_iterator i(alt.get_vertexset().begin()), e(alt.get_vertexset().end()); i != e; ++i) {
					if (*i >= fpl.size())
						continue;
					const TrafficFlowRestrictions::FplWpt& wpt(fpl[*i]);
					work = lgraphscalesolutionvertex(wpt.get_ident(), scale) || work;
				}
				continue;
			}
			for (set_t::const_iterator i(alt.get_edgeset().begin()), e(alt.get_edgeset().end()); i != e; ++i) {
				if (*i >= fpl.size() + 1U)
					continue;
				const TrafficFlowRestrictions::FplWpt& wpt0(fpl[*i]);
				const TrafficFlowRestrictions::FplWpt& wpt1(fpl[*i + 1U]);
				work = lgraphscalesolutionedge(wpt0.get_ident(), wpt1.get_ident(), scale) || work;
			}
		}
		break;

	case TrafficFlowRestrictions::RuleResult::codetype_closed:
		if (rule.get_edgeset().empty()) {
			for (set_t::const_iterator i(rule.get_vertexset().begin()), e(rule.get_vertexset().end()); i != e; ++i) {
				if (*i >= fpl.size())
					continue;
				const TrafficFlowRestrictions::FplWpt& wpt(fpl[*i]);
				work = lgraphscalesolutionvertex(wpt.get_ident(), scale) || work;
			}
			break;
		}
		for (set_t::const_iterator i(rule.get_edgeset().begin()), e(rule.get_edgeset().end()); i != e; ++i) {
			if (*i >= fpl.size() + 1U)
				continue;
			const TrafficFlowRestrictions::FplWpt& wpt0(fpl[*i]);
			const TrafficFlowRestrictions::FplWpt& wpt1(fpl[*i + 1U]);
			work = lgraphscalesolutionedge(wpt0.get_ident(), wpt1.get_ident(), scale) || work;
		}
		break;

	default:
		break;
	}
	if (work)
		lgraphmodified();
	return work;
}

void CFMUAutoroute45::lgraphtfrmessage(const TrafficFlowRestrictions::Message& msg, const TrafficFlowRestrictions::Fpl& fpl)
{
	if (msg.get_type() != TrafficFlowRestrictions::Message::type_error)
		return;
	// actually modify the graph
	typedef TrafficFlowRestrictions::RuleResult::set_t set_t;
	if (msg.get_edgeset().empty()) {
		unsigned int i;
		{
			set_t vset, vset1;
			vset1.insert(0U);
			vset1.insert(fpl.size() - 1U);
			std::set_difference(msg.get_vertexset().begin(), msg.get_vertexset().end(), vset1.begin(), vset1.end(),
					    std::insert_iterator<set_t>(vset, vset.begin()));
			set_t::const_iterator it(vset.begin()), e(vset.end());
			if (it == e)
				return;
			i = *it;
			++it;
			if (it != e)
				return;
		}
		if (i >= fpl.size())
			return;
		const TrafficFlowRestrictions::FplWpt& wpt(fpl[i]);
		if (lgraphdisconnectsolutionvertex(wpt.get_ident()))
			return;
		return;
	}
	unsigned int i;
	{
		set_t::const_iterator it(msg.get_edgeset().begin()), e(msg.get_edgeset().end());
		if (it == e)
			return;
		i = *it;
		++it;
		if (it != e)
			return;
	}
	if (i + 1U >= fpl.size())
		return;
	{
		set_t vset, vset1;
		vset1.insert(i);
		vset1.insert(i + 1U);
		vset1.insert(0U);
		vset1.insert(fpl.size() - 1U);
		std::set_difference(msg.get_vertexset().begin(), msg.get_vertexset().end(), vset1.begin(), vset1.end(),
				    std::insert_iterator<set_t>(vset, vset.begin()));
		if (!vset.empty())
			return;
	}
	const TrafficFlowRestrictions::FplWpt& wpt0(fpl[i]);
	const TrafficFlowRestrictions::FplWpt& wpt1(fpl[i + 1U]);
	if (lgraphdisconnectsolutionedge(wpt0.get_ident(), wpt1.get_ident()))
		return;
}

std::string CFMUAutoroute45::lgraphvertexname(const LGraph& g, LGraph::vertex_descriptor v)
{
	const LVertex& vv(g[v]);
	std::ostringstream oss;
	oss << vv.get_ident();
	return oss.str();
}

std::string CFMUAutoroute45::lgraphvertexname(const LGraph& g, LGraph::vertex_descriptor v, unsigned int pi)
{
	const LVertex& vv(g[v]);
	std::ostringstream oss;
	oss << vv.get_ident();
	if (pi >= m_performance.size() || v == m_vertexdep || v == m_vertexdest)
		return oss.str();
	const Performance::Cruise& cruise(m_performance.get_cruise(pi));
	oss << ':' << cruise.get_fplalt();
	return oss.str();
}

std::string CFMUAutoroute45::lgraphvertexname(const LGraph& g, LGraph::vertex_descriptor v, unsigned int pi0, unsigned int pi1)
{
	const LVertex& vv(g[v]);
	std::ostringstream oss;
	oss << vv.get_ident();
	if ((pi0 >= m_performance.size() && pi1 >= m_performance.size()) || v == m_vertexdep || v == m_vertexdest)
		return oss.str();
	oss << ':';
	if (pi0 < m_performance.size()) {
		const Performance::Cruise& cruise(m_performance.get_cruise(pi0));
		oss << cruise.get_fplalt();
	}
	oss << "...";
	if (pi1 < m_performance.size()) {
		const Performance::Cruise& cruise(m_performance.get_cruise(pi1));
		oss << cruise.get_fplalt();
	}
	return oss.str();
}

void CFMUAutoroute45::lgraphlogkilledge(const LGraph& g, LGraph::edge_descriptor e, unsigned int pi)
{
	const LEdge& ee(g[e]);
	std::ostringstream oss;
	oss << "- " << lgraphvertexname(source(e, g), pi)
	    << ' ' << lgraphindex2airway(ee.get_ident(), true) << ' '
	    << lgraphvertexname(target(e, g), pi);
	m_signal_log(log_graphchange, oss.str());
}

void CFMUAutoroute45::lgraphlogkillalledges(const LGraph& g, LGraph::edge_descriptor e)
{
        const unsigned int pis(m_performance.size());
	const LEdge& ee(g[e]);
	for (unsigned int pi = 0; pi < pis; ++pi) {
		if (!ee.is_valid(pi))
			continue;
		lgraphlogkilledge(e, pi);
	}
}

void CFMUAutoroute45::lgraphlogaddedge(const LGraph& g, LGraph::edge_descriptor e, unsigned int pi)
{
	const LEdge& ee(g[e]);
	std::ostringstream oss;
	oss << "+ " << lgraphvertexname(source(e, g), pi)
	    << ' ' << lgraphindex2airway(ee.get_ident(), true) << ' '
	    << lgraphvertexname(target(e, g), pi);
	m_signal_log(log_graphchange, oss.str());
}

void CFMUAutoroute45::lgraphlogrenameedge(const LGraph& g, LGraph::edge_descriptor e, unsigned int pi, lgraphairwayindex_t awyname)
{
	const LEdge& ee(g[e]);
	std::ostringstream oss;
	oss << "R " << lgraphvertexname(source(e, g), pi)
	    << ' ' << lgraphindex2airway(ee.get_ident(), true) << ' '
	    << lgraphvertexname(target(e, g), pi) << " -> " << lgraphindex2airway(awyname, true);
	m_signal_log(log_graphchange, oss.str());
}

void CFMUAutoroute45::lgraphlogscaleedge(const LGraph& g, LGraph::edge_descriptor e, unsigned int pi, double scale)
{
	const LEdge& ee(g[e]);
	std::ostringstream oss;
	oss << "S " << lgraphvertexname(source(e, g), pi)
	    << ' ' << lgraphindex2airway(ee.get_ident(), true) << ' '
	    << lgraphvertexname(target(e, g), pi) << " : "
	    << ee.get_metric(pi) << " * " << scale << " -> " << (ee.get_metric(pi) * scale);
	m_signal_log(log_graphchange, oss.str());
}

bool CFMUAutoroute45::lgraphdisconnectvertex(const std::string& ident, int lvlfrom, int lvlto, bool intel)
{
	bool work(false);
	std::string identu(Glib::ustring(ident).uppercase());
	lvlfrom *= 100;
	lvlto *= 100;
	bool allalt(lvlfrom == 0 && lvlto >= 60000);
        const unsigned int pis(m_performance.size());
	for (solutionvertices_t::const_iterator svi(m_solutionvertices.begin()), sve(m_solutionvertices.end()); svi != sve; ++svi) {
		LVertexLevel v(*svi);
		const LVertex& vv(m_lgraph[v.get_vertex()]);
		if (vv.get_ident() != identu)
			continue;
		// prevent departure or destination from being disconnected if an intersection has the same ident
		if (allalt && (v.get_vertex() == m_vertexdep || v.get_vertex() == m_vertexdest))
			continue;
		if (intel && allalt)
			m_cfmuintel.add(CFMUIntel::ForbiddenPoint(vv.get_ident(), vv.get_coord()));
		const Performance::Cruise& cruise(m_performance.get_cruise(v.get_perfindex()));
		if (cruise.get_altitude() < lvlfrom || cruise.get_altitude() > lvlto)
			continue;
		{
			LGraph::out_edge_iterator e0, e1;
			for (tie(e0, e1) = out_edges(v.get_vertex(), m_lgraph); e0 != e1; ++e0) {
				for (unsigned int pi = 0; pi < pis; ++pi) {
					const Performance::Cruise& cruise(m_performance.get_cruise(pi));
					if (cruise.get_altitude() < lvlfrom || cruise.get_altitude() > lvlto)
						continue;
					lgraphkilledge(*e0, pi, true);
					work = true;
				}
			}
		}
		{
			LGraph::in_edge_iterator e0, e1;
			for (tie(e0, e1) = in_edges(v.get_vertex(), m_lgraph); e0 != e1; ++e0) {
				for (unsigned int pi = 0; pi < pis; ++pi) {
					const Performance::Cruise& cruise(m_performance.get_cruise(pi));
					if (cruise.get_altitude() < lvlfrom || cruise.get_altitude() > lvlto)
						continue;
					lgraphkilledge(*e0, pi, true);
					work = true;
				}
			}
		}
	}
	return work;
}

bool CFMUAutoroute45::lgraphdisconnectsolutionvertex(const std::string& ident)
{
	bool work(false);
	std::string identu(Glib::ustring(ident).uppercase());
	LGraph::edge_iterator e0, e1;
	for (tie(e0, e1) = edges(m_lgraph); e0 != e1; ++e0) {
		LEdge& ee(m_lgraph[*e0]);
		if (!ee.is_solution())
			continue;
		const LVertex& uu(m_lgraph[source(*e0, m_lgraph)]);
		if (uu.get_ident() != identu) {
			const LVertex& vv(m_lgraph[target(*e0, m_lgraph)]);
			if (vv.get_ident() != identu)
				continue;
		}
		lgraphkillsolutionedge(*e0, true);
		work = true;
	}
	return work;
}

bool CFMUAutoroute45::lgraphdisconnectsolutionedge(const std::string& id0, const std::string& id1, lgraphairwayindex_t awyname)
{
	bool work(false);
	std::string id0u(Glib::ustring(id0).uppercase());
	std::string id1u(Glib::ustring(id1).uppercase());
	if (false) {
		std::ostringstream oss;
		oss << "lgraphdisconnectsolutionedge: " << id0u << " --> " << id1u;
		m_signal_log(log_debug0, oss.str());
	}
	const unsigned int pis(m_performance.size());
	for (;;) {
		bool done(true);
		LGraph::edge_iterator e0, e1;
		for (tie(e0, e1) = edges(m_lgraph); e0 != e1; ++e0) {
			LEdge& ee(m_lgraph[*e0]);
			if (!ee.is_solution())
				continue;
			if (!ee.is_match(awyname))
				continue;
			if (false) {
				LGraph::vertex_descriptor u(source(*e0, m_lgraph));
				const LVertex& uu(m_lgraph[u]);
				LGraph::vertex_descriptor v(target(*e0, m_lgraph));
				const LVertex& vv(m_lgraph[v]);
				std::ostringstream oss;
				oss << "lgraphdisconnectsolutionedge: edge " << uu.get_ident() << " --> " << vv.get_ident();
				if (ee.is_match(airwaymatchawy))
					oss << " (AWY)";
				m_signal_log(log_debug0, oss.str());
			}
			LGraph::vertex_descriptor u(source(*e0, m_lgraph));
			const LVertex& uu(m_lgraph[u]);
			if (uu.get_ident() != id0u)
				continue;
			LGraph::vertex_descriptor v(target(*e0, m_lgraph));
			const LVertex& vv(m_lgraph[v]);
			if (vv.get_ident() != id1u)
				continue;
			if (ee.is_match(airwaymatchawy)) {
				LEdge edge(pis, airwaydct);
				edge.set_metric(ee.get_solution(), 1);
				lgraphaddedge(edge, u, v, setmetric_metric, true);
				work = true;
			}
			lgraphkillsolutionedge(*e0, true);
			work = true;
		}
		if (done)
			break;
	}
	return work;
}

bool CFMUAutoroute45::lgraphscalesolutionvertex(const std::string& ident, double scale)
{
	bool work(false);
	std::string identu(Glib::ustring(ident).uppercase());
	LGraph::edge_iterator e0, e1;
	for (tie(e0, e1) = edges(m_lgraph); e0 != e1; ++e0) {
		LEdge& ee(m_lgraph[*e0]);
		if (!ee.is_solution())
			continue;
		const LVertex& uu(m_lgraph[source(*e0, m_lgraph)]);
		if (uu.get_ident() != identu) {
			const LVertex& vv(m_lgraph[target(*e0, m_lgraph)]);
			if (vv.get_ident() != identu)
				continue;
		}
		double newmetric(ee.get_solution_metric() * scale);
		if (newmetric != ee.get_solution_metric()) {
			lgraphlogscaleedge(*e0, ee.get_solution(), scale);
			work = true;
		}
		ee.set_solution_metric(newmetric);
	}
	return work;
}

bool CFMUAutoroute45::lgraphscalesolutionedge(const std::string& id0, const std::string& id1, double scale)
{
	bool work(false);
	std::string id0u(Glib::ustring(id0).uppercase());
	std::string id1u(Glib::ustring(id1).uppercase());
	LGraph::edge_iterator e0, e1;
	for (tie(e0, e1) = edges(m_lgraph); e0 != e1; ++e0) {
		LEdge& ee(m_lgraph[*e0]);
		if (!ee.is_solution())
			continue;
		const LVertex& uu(m_lgraph[source(*e0, m_lgraph)]);
		if (uu.get_ident() != id0u)
			continue;
		const LVertex& vv(m_lgraph[target(*e0, m_lgraph)]);
		if (vv.get_ident() != id1u)
			continue;
		double newmetric(ee.get_solution_metric() * scale);
		if (newmetric != ee.get_solution_metric()) {
			lgraphlogscaleedge(*e0, ee.get_solution(), scale);
			work = true;
		}
		ee.set_solution_metric(newmetric);
	}
	return work;
}

bool CFMUAutoroute45::lgraphmodifyedge(const std::string& id0, const std::string& id1, lgraphairwayindex_t awyremove,
				     lgraphairwayindex_t awyadd, int lvlfrom, int lvlto, bool bidir)
{
	bool work(false);
	std::string id0u(Glib::ustring(id0).uppercase());
	std::string id1u(Glib::ustring(id1).uppercase());
	lvlfrom *= 100;
	lvlto *= 100;
        const unsigned int pis(m_performance.size());
	for (solutionvertices_t::const_iterator sui(m_solutionvertices.begin()), sue(m_solutionvertices.end()); sui != sue; ++sui) {
		LVertexLevel u(*sui);
		const LVertex& uu(m_lgraph[u.get_vertex()]);
		if (uu.get_ident() != id0u)
			continue;
		for (solutionvertices_t::const_iterator svi(m_solutionvertices.begin()), sve(m_solutionvertices.end()); svi != sve; ++svi) {
			LVertexLevel v(*svi);
			if (u == v)
				continue;
			const LVertex& vv(m_lgraph[v.get_vertex()]);
		        if (vv.get_ident() != id1u)
				continue;
			LEdge edge(pis, awyadd);
			LGraph::out_edge_iterator e0, e1;
			for (tie(e0, e1) = out_edges(u.get_vertex(), m_lgraph); e0 != e1; ++e0) {
				if (target(*e0, m_lgraph) != v.get_vertex())
					continue;
				const LEdge& ee(m_lgraph[*e0]);
				if (ee.is_match(awyadd))
					continue;
				if (!ee.is_match(awyremove))
					continue;
				for (unsigned int pi = 0; pi < pis; ++pi) {
					if (!ee.is_valid(pi))
						continue;
					const Performance::Cruise& cruise(m_performance.get_cruise(pi));
					int alt(cruise.get_altitude());
					if (alt < lvlfrom || alt > lvlto)
						continue;
					edge.set_metric(pi, 1);
					lgraphkilledge(*e0, pi, true);
					work = true;
				}
			}
			lgraphaddedge(edge, u.get_vertex(), v.get_vertex(), setmetric_metric, true);
			if (!bidir)
				continue;
			edge.clear_metric();
			for (tie(e0, e1) = out_edges(v.get_vertex(), m_lgraph); e0 != e1; ++e0) {
				if (target(*e0, m_lgraph) != u.get_vertex())
					continue;
				const LEdge& ee(m_lgraph[*e0]);
				if (ee.is_match(awyadd))
					continue;
				if (!ee.is_match(awyremove))
					continue;
				for (unsigned int pi = 0; pi < pis; ++pi) {
					if (!ee.is_valid(pi))
						continue;
					const Performance::Cruise& cruise(m_performance.get_cruise(pi));
					int alt(cruise.get_altitude());
					if (alt < lvlfrom || alt > lvlto)
						continue;
					edge.set_metric(pi, 1);
					lgraphkilledge(*e0, pi, true);
					work = true;
				}
			}
			lgraphaddedge(edge, v.get_vertex(), u.get_vertex(), setmetric_metric, true);
		}
	}
	return work;
}

bool CFMUAutoroute45::lgraphmodifyawysegment(const std::string& id0, const std::string& id1, lgraphairwayindex_t awymatch,
					   lgraphairwayindex_t awyrename, int lvlfrom, int lvlto, bool bidir, bool solutiononly)
{
	if (awymatch == awyrename)
		return false;
	bool work(false);
	std::string id0u(Glib::ustring(id0).uppercase());
	std::string id1u(Glib::ustring(id1).uppercase());
	lvlfrom *= 100;
	lvlto *= 100;
	if (false && id0u == "RUDAK" && id1u == "KLF")
		std::cerr << "lgraphmodifyawysegment: " << id0u << "--" << lgraphindex2airway(awymatch, true) << "->" << id1u
			  << " FL " << lvlfrom << ".." << lvlto << std::endl;
	LGraphAwyEdgeFilter filter(m_lgraph, awymatch);
        typedef boost::filtered_graph<LGraph, LGraphAwyEdgeFilter> fg_t;
        fg_t fg(m_lgraph, filter);
        const unsigned int pis(m_performance.size());
	for (solutionvertices_t::const_iterator sui(m_solutionvertices.begin()), sue(m_solutionvertices.end()); sui != sue; ++sui) {
		LVertexLevel u(*sui);
		const LVertex& uu(m_lgraph[u.get_vertex()]);
		if (uu.get_ident() != id0u)
			continue;
		for (solutionvertices_t::const_iterator svi(m_solutionvertices.begin()), sve(m_solutionvertices.end()); svi != sve; ++svi) {
			LVertexLevel v(*svi);
			if (u == v)
				continue;
			const LVertex& vv(m_lgraph[v.get_vertex()]);
			if (vv.get_ident() != id1u)
				continue;
			if (false && id0u == "RUDAK" && id1u == "KLF")
				std::cerr << "lgraphmodifyawysegment: try path " << lgraphvertexname(u)
					  << " -> " << lgraphvertexname(v) << std::endl;
			std::vector<LGraph::vertex_descriptor> predecessors(boost::num_vertices(m_lgraph), 0);
			dijkstra_shortest_paths(fg, u.get_vertex(), boost::weight_map(boost::get(&LEdge::m_dist, m_lgraph)).
						predecessor_map(&predecessors[0]));
			LGraph::vertex_descriptor ve(v.get_vertex());
			while (ve != u.get_vertex()) {
				LGraph::vertex_descriptor ue(predecessors[ve]);
				if (false && id0u == "RUDAK" && id1u == "KLF")
					std::cerr << "lgraphmodifyawysegment: path found " << lgraphvertexname(ue)
						  << " -> " << lgraphvertexname(ve) << std::endl;
				LEdge edge(pis, awyrename);
				LGraph::out_edge_iterator e0, e1;
				for (tie(e0, e1) = out_edges(ue, m_lgraph); e0 != e1; ++e0) {
					if (target(*e0, m_lgraph) != ve)
						continue;
					LEdge& ee(m_lgraph[*e0]);
					if (ee.is_match(awyrename))
						continue;
					if (!ee.is_match(awymatch))
						continue;
					if (solutiononly && !ee.is_solution())
						continue;
					for (unsigned int pi = 0; pi < pis; ++pi) {
						if (!ee.is_valid(pi))
							continue;
						const Performance::Cruise& cruise(m_performance.get_cruise(pi));
						int alt(cruise.get_altitude());
						if (alt < lvlfrom || alt > lvlto)
							continue;
						edge.set_metric(pi, 1);
						lgraphkilledge(*e0, pi, true);
						work = true;
					}
				}
				lgraphaddedge(edge, ue, ve, setmetric_metric, true);
				if (!bidir) {
					if (ue == ve)
						break;
					ve = ue;
					continue;
				}
				edge.clear_metric();
				for (tie(e0, e1) = out_edges(ve, m_lgraph); e0 != e1; ++e0) {
					if (target(*e0, m_lgraph) != ue)
						continue;
					LEdge& ee(m_lgraph[*e0]);
					if (ee.is_match(awyrename))
						continue;
					if (!ee.is_match(awymatch))
						continue;
					if (solutiononly && !ee.is_solution())
						continue;
					for (unsigned int pi = 0; pi < pis; ++pi) {
						if (!ee.is_valid(pi))
							continue;
						const Performance::Cruise& cruise(m_performance.get_cruise(pi));
						int alt(cruise.get_altitude());
						if (alt < lvlfrom || alt > lvlto)
							continue;
						edge.set_metric(pi, 1);
						lgraphkilledge(*e0, pi, true);
						work = true;
					}
				}
				lgraphaddedge(edge, ve, ue, setmetric_metric, true);
				if (ue == ve)
					break;
				ve = ue;
			}
		}
	}
	return work;
}

bool CFMUAutoroute45::lgraphmodifysolutionsegment(const std::string& id0, const std::string& id1, lgraphairwayindex_t awymatch,
						lgraphairwayindex_t awyrename, int lvlfrom, int lvlto, bool matchbothid)
{
	bool work(false);
	std::string id0u(Glib::ustring(id0).uppercase());
	std::string id1u(Glib::ustring(id1).uppercase());
	const unsigned int pis(m_performance.size());
	LGraph::edge_iterator e0, e1;
	for (tie(e0, e1) = edges(m_lgraph); e0 != e1; ++e0) {
		LEdge& ee(m_lgraph[*e0]);
		if (!ee.is_solution())
			continue;
		if (!ee.is_match(awymatch))
			continue;
		if (!ee.is_solution_valid())
			continue;
		LGraph::vertex_descriptor u(source(*e0, m_lgraph));
		const LVertex& uu(m_lgraph[u]);
		unsigned int matchcnt(uu.get_ident() == id0u);
		if (!matchcnt && matchbothid)
			continue;
		LGraph::vertex_descriptor v(target(*e0, m_lgraph));
		const LVertex& vv(m_lgraph[v]);
		matchcnt += (vv.get_ident() == id1u);
		if (matchcnt < 1 + !!matchbothid)
			continue;
		LEdge edge(pis, awyrename);
		edge.set_metric(ee.get_solution(), 1);
		lgraphkillsolutionedge(*e0, true);
		lgraphaddedge(edge, u, v, setmetric_metric, true);
		work = true;
	}
	return work;
}

bool CFMUAutoroute45::lgraphchangeoutgoing(const std::string& id, lgraphairwayindex_t awyname)
{
	bool work(false);
	std::string identu(Glib::ustring(id).uppercase());
	const unsigned int pis(m_performance.size());
	for (solutionvertices_t::const_iterator svi(m_solutionvertices.begin()), sve(m_solutionvertices.end()); svi != sve; ++svi) {
		LVertexLevel v(*svi);
		const LVertex& vv(m_lgraph[v.get_vertex()]);
		if (vv.get_ident() != identu)
			continue;
		LGraph::out_edge_iterator e0, e1;
		for (tie(e0, e1) = out_edges(v.get_vertex(), m_lgraph); e0 != e1; ++e0) {
			LEdge& ee(m_lgraph[*e0]);
			if (!ee.is_match(awyname))
				continue;
			LEdge edge(pis, (awyname != airwaydct) ? airwaydct : airwaymatchnone);
			for (unsigned int pi = 0; pi < pis; ++pi) {
				if (!ee.is_valid(pi))
					continue;
				edge.set_metric(pi, 1);
				lgraphkilledge(*e0, pi, true);
				work = true;
			}
			lgraphaddedge(edge, source(*e0, m_lgraph), target(*e0, m_lgraph), setmetric_metric, true);
		}
	}
	return work;
}

bool CFMUAutoroute45::lgraphchangeincoming(const std::string& id, lgraphairwayindex_t awyname)
{
	bool work(false);
	std::string identu(Glib::ustring(id).uppercase());
	const unsigned int pis(m_performance.size());
	for (solutionvertices_t::const_iterator svi(m_solutionvertices.begin()), sve(m_solutionvertices.end()); svi != sve; ++svi) {
		LVertexLevel v(*svi);
		const LVertex& vv(m_lgraph[v.get_vertex()]);
		if (vv.get_ident() != identu)
			continue;
		LGraph::in_edge_iterator e0, e1;
		for (tie(e0, e1) = in_edges(v.get_vertex(), m_lgraph); e0 != e1; ++e0) {
			LEdge& ee(m_lgraph[*e0]);
			if (!ee.is_match(awyname))
				continue;
			LEdge edge(pis, (awyname != airwaydct) ? airwaydct : airwaymatchnone);
			for (unsigned int pi = 0; pi < pis; ++pi) {
				if (!ee.is_valid(pi))
					continue;
				edge.set_metric(pi, 1);
				lgraphkilledge(*e0, pi, true);
				work = true;
			}
			lgraphaddedge(edge, source(*e0, m_lgraph), target(*e0, m_lgraph), setmetric_metric, true);
		}
	}
	return work;
}

bool CFMUAutoroute45::lgraphedgetodct(lgraphairwayindex_t awy)
{
	if (awy == airwaydct)
		return false;
	bool work(false);
	const unsigned int pis(m_performance.size());
	LGraph::edge_iterator e0, e1;
	for (tie(e0, e1) = edges(m_lgraph); e0 != e1; ++e0) {
		LEdge& ee(m_lgraph[*e0]);
		if (!ee.is_solution())
			continue;
		if (!ee.is_match(awy))
			continue;
		if (!ee.is_solution_valid())
			continue;
		LEdge edge(pis, airwaydct);
		edge.set_metric(ee.get_solution());
		lgraphkillsolutionedge(*e0, true);
		lgraphaddedge(edge, source(*e0, m_lgraph), target(*e0, m_lgraph), setmetric_metric, true);
		work = true;
	}
	return work;
}

bool CFMUAutoroute45::lgraphkilloutgoing(const std::string& id)
{
	std::string identu(Glib::ustring(id).uppercase());
	bool work(false);
	LGraph::edge_iterator e0, e1;
	for (tie(e0, e1) = edges(m_lgraph); e0 != e1; ++e0) {
		LEdge& ee(m_lgraph[*e0]);
		if (!ee.is_solution())
			continue;
		LGraph::vertex_descriptor v(source(*e0, m_lgraph));
		const LVertex& vv(m_lgraph[v]);
		if (vv.get_ident() != identu)
			continue;
		lgraphkillsolutionedge(*e0);
		work = true;
	}
	return work;
}

bool CFMUAutoroute45::lgraphkillincoming(const std::string& id)
{
	std::string identu(Glib::ustring(id).uppercase());
	bool work(false);
	LGraph::edge_iterator e0, e1;
	for (tie(e0, e1) = edges(m_lgraph); e0 != e1; ++e0) {
		LEdge& ee(m_lgraph[*e0]);
		if (!ee.is_solution())
			continue;
		LGraph::vertex_descriptor v(target(*e0, m_lgraph));
		const LVertex& vv(m_lgraph[v]);
		if (vv.get_ident() != identu)
			continue;
		lgraphkillsolutionedge(*e0);
		work = true;
	}
	return work;
}

bool CFMUAutoroute45::lgraphkillsid(const std::string& id)
{
	std::string identu(Glib::ustring(id).uppercase());
	bool work(false);
	LGraph::edge_iterator e0, e1;
	for (tie(e0, e1) = edges(m_lgraph); e0 != e1; ++e0) {
		LEdge& ee(m_lgraph[*e0]);
		if (!ee.is_solution())
			continue;
		if (!ee.is_match(airwaysid))
			continue;
		LGraph::vertex_descriptor v(target(*e0, m_lgraph));
		const LVertex& vv(m_lgraph[v]);
		if (vv.get_ident() != identu)
			continue;
		lgraphkillsolutionedge(*e0);
		work = true;
	}
	return work;
}

bool CFMUAutoroute45::lgraphkillstar(const std::string& id)
{
	std::string identu(Glib::ustring(id).uppercase());
	bool work(false);
	LGraph::edge_iterator e0, e1;
	for (tie(e0, e1) = edges(m_lgraph); e0 != e1; ++e0) {
		LEdge& ee(m_lgraph[*e0]);
		if (!ee.is_solution())
			continue;
		if (!ee.is_match(airwaystar))
			continue;
		LGraph::vertex_descriptor u(source(*e0, m_lgraph));
		const LVertex& uu(m_lgraph[u]);
		if (uu.get_ident() != identu)
			continue;
		lgraphkillsolutionedge(*e0);
		work = true;
	}
	return work;
}

bool CFMUAutoroute45::lgraphkillclosesegments(const std::string& id0, const std::string& id1, const std::string& awyid, lgraphairwayindex_t solutionmatch)
{
	lgraphairwayindex_t awy(lgraphairway2index(awyid, false));
	return lgraphkillclosesegments(id0, id1, awy);
}

bool CFMUAutoroute45::lgraphkillclosesegments(const std::string& id0, const std::string& id1, lgraphairwayindex_t awy, lgraphairwayindex_t solutionmatch)
{
	if (awy == airwaymatchnone)
		return false;
        std::string id0u(Glib::ustring(id0).uppercase());
        std::string id1u(Glib::ustring(id1).uppercase());
	bool work(false);
        LGraphAwyEdgeFilter filter(m_lgraph, awy);
        typedef boost::filtered_graph<LGraph, LGraphAwyEdgeFilter> fg_t;
        fg_t fg(m_lgraph, filter);
	if (false) {
		std::ostringstream oss;
		oss << "lgraphkillclosesegments: " << id0u << "--" << lgraphindex2airway(awy, true) << "->" << id1u;
		m_signal_log(log_debug0, oss.str());
	}
	// find combinations of source and destination supernodes
	LGraph::vertex_iterator v0i, v0e;
	for (boost::tie(v0i, v0e) = boost::vertices(m_lgraph); v0i != v0e; ++v0i) {
		const LVertex& vv0(m_lgraph[*v0i]);
		if (vv0.get_ident() != id0u)
                        continue;
		LGraph::vertex_iterator v1i, v1e;
		for (boost::tie(v1i, v1e) = boost::vertices(m_lgraph); v1i != v1e; ++v1i) {
			if (*v0i == *v1i)
				continue;
			const LVertex& vv1(m_lgraph[*v1i]);
			if (vv1.get_ident() != id1u)
                                continue;
			if (false) {
				std::ostringstream oss;
				oss << "lgraphkillclosesegments: trying route "
				    << vv0.get_ident() << ' ' << vv0.get_coord().get_lat_str2() << ' ' << vv0.get_coord().get_lon_str2()
				    << " -> "
				    << vv1.get_ident() << ' ' << vv1.get_coord().get_lat_str2() << ' ' << vv1.get_coord().get_lon_str2();
				m_signal_log(log_debug0, oss.str());
			}
			// find path from source to destination
			std::vector<LGraph::vertex_descriptor> predecessors(boost::num_vertices(m_lgraph), 0);
			dijkstra_shortest_paths(fg, *v0i, boost::weight_map(boost::get(&LEdge::m_dist, m_lgraph)).
						predecessor_map(&predecessors[0]));
			LGraph::vertex_descriptor ve(*v1i);
			while (ve != *v0i) {
				LGraph::vertex_descriptor ue(predecessors[ve]);
				if (ve == ue)
					break;
				const LVertex& uue(m_lgraph[ue]);
				const LVertex& vve(m_lgraph[ve]);
				PolygonSimple poly(PolygonSimple::line_width_rectangular(uue.get_coord(), vve.get_coord(), 5.0));
				Rect bbox(poly.get_bbox());
				if (false) {
					std::ostringstream oss;
					oss << "lgraphkillclosesegments: trying segment "
					    << lgraphvertexname(ue) << ' ' << uue.get_coord().get_lat_str2() << ' ' << uue.get_coord().get_lon_str2()
					    << " -> "
					    << lgraphvertexname(ve) << ' ' << vve.get_coord().get_lat_str2() << ' ' << vve.get_coord().get_lon_str2()
					    << " poly";
					for (unsigned int i = 0; i < poly.size(); ++i)
						oss << ' ' << poly[i].get_lat_str2() << ' ' << poly[i].get_lon_str2();
					m_signal_log(log_debug0, oss.str());
				}
				LGraph::edge_iterator e0, e1;
				for (tie(e0, e1) = edges(m_lgraph); e0 != e1; ++e0) {
					LEdge& ee(m_lgraph[*e0]);
					if (!ee.is_solution())
						continue;
					if (!ee.is_match(solutionmatch))
						continue;
					if (!ee.is_solution_valid())
						continue;
					LGraph::vertex_descriptor u(source(*e0, m_lgraph));
					LGraph::vertex_descriptor v(target(*e0, m_lgraph));
					const LVertex& uu(m_lgraph[u]);
					const LVertex& vv(m_lgraph[v]);
					{
						Rect bboxe(uu.get_coord(), uu.get_coord());
						bboxe = bboxe.add(vv.get_coord());
						if (!bbox.is_intersect(bboxe))
							continue;
					}
					if (!poly.windingnumber(uu.get_coord()) &&
					    !poly.windingnumber(vv.get_coord()) &&
					    !poly.is_intersection(uu.get_coord(), vv.get_coord()))
						continue;
					lgraphkillsolutionedge(*e0, true);
					work = true;
				}
				ve = ue;
			}
		}
	}
	return work;
}

bool CFMUAutoroute45::lgraphsolutiongroundclearance(void)
{
	m_signal_log(log_graphrule, "Ground Clearance");
	bool work(false);
        const unsigned int pis(m_performance.size());
	LGraph::edge_iterator e0, e1;
	for (tie(e0, e1) = edges(m_lgraph); e0 != e1; ++e0) {
		LEdge& ee(m_lgraph[*e0]);
		if (!ee.is_solution())
			continue;
		if (!ee.is_solution_valid())
			continue;
		LGraph::vertex_descriptor u(source(*e0, m_lgraph));
		if (u == m_vertexdep)
			continue;
		LGraph::vertex_descriptor v(target(*e0, m_lgraph));
		if (v == m_vertexdest)
			continue;
		if (u == v)
			continue;
		const LVertex& uu(m_lgraph[u]);
		const LVertex& vv(m_lgraph[v]);
		int32_t minalt(0);
		{
			TopoDb30::Profile prof(m_topodb.get_profile(uu.get_coord(), vv.get_coord(), 5));
			if (prof.empty()) {
				std::ostringstream oss;
				oss << "Error computing ground clearance: " << lgraphvertexname(source(*e0, m_lgraph), ee.get_solution())
				    << ' ' << lgraphindex2airway(ee.get_ident(), true) << ' '
				    << lgraphvertexname(target(*e0, m_lgraph), ee.get_solution());
				m_signal_log(log_normal, oss.str());
				continue;
			}
			int32_t alt(prof.get_maxelev() * Point::m_to_ft);
			minalt = alt + 1000;
			if (alt >= 5000)
				minalt += 1000;
			{
				std::ostringstream oss;
				oss << "Leg: " << lgraphvertexname(source(*e0, m_lgraph), ee.get_solution())
				    << ' ' << lgraphindex2airway(ee.get_ident(), true) << ' '
				    << lgraphvertexname(target(*e0, m_lgraph), ee.get_solution())
				    << " ground elevation " << alt << "ft minimum level FL" << ((minalt + 99) / 100);
				m_signal_log(log_normal, oss.str());
			}
		}
		for (unsigned int pi = 0; pi < pis; ++pi) {
			const Performance::Cruise& cruise(m_performance.get_cruise(pi));
			if (cruise.get_altitude() >= minalt)
				break;
			LGraph::out_edge_iterator e0, e1;
			tie(e0, e1) = boost::edge_range(u, v, m_lgraph);
			for (; e0 != e1; ++e0) {
				LEdge& ee(m_lgraph[*e0]);
				if (ee.is_solution(pi))
					work = true;
				lgraphkilledge(*e0, pi, true);
			}
			tie(e0, e1) = boost::edge_range(v, u, m_lgraph);
			for (; e0 != e1; ++e0) {
				LEdge& ee(m_lgraph[*e0]);
				if (ee.is_solution(pi))
					work = true;
				lgraphkilledge(*e0, pi, true);
			}
		}
	}
	return work;
}

void CFMUAutoroute45::parse_response(void)
{
	m_connchildtimeout.disconnect();
	if (is_stopped())
		return;
	m_signal_statuschange(statusmask_newvalidateresponse);
	bool work(m_validationresponse.size() == 1 && (m_validationresponse[0] == "NO ERRORS" ||
						       m_validationresponse[0] == "(R)ROUTE152: FLIGHT NOT APPLICABLE TO IFPS"));
	if (!work) {
		work = true;
		for (validationresponse_t::const_iterator i(m_validationresponse.begin()), e(m_validationresponse.end()); i != e; ++i) {
			if (i->empty())
				continue;
			const char * const *ignrx(ignoreregex);
			work = false;
			while (*ignrx) {
				Glib::RefPtr<Glib::Regex> rx(Glib::Regex::create(*ignrx++));
				Glib::MatchInfo minfo;
				if (rx->match(*i, minfo)) {
					work = true;
					break;
				}
			}
			if (!work)
				break;
		}
	}
	if (work) {
		if (m_pathprobe < 0) {
			if (lgraphsolutiongroundclearance()) {
				lgraphroute();
				return;
			}
			m_done = true;
			m_signal_log(log_fplproposal, get_simplefpl());
			stop(statusmask_none);
			return;
		}
		for (;;) {
			++m_pathprobe;
			if (m_pathprobe >= m_route.get_nrwpt())
				break;
			const FPlanWaypoint& wpt(m_route[m_pathprobe]);
			if (wpt.get_flags() & FPlanWaypoint::altflag_ifr &&
			    wpt.get_pathcode() != FPlanWaypoint::pathcode_sid &&
			    wpt.get_pathcode() != FPlanWaypoint::pathcode_star)
				break;
		}
		if (m_pathprobe >= m_route.get_nrwpt()) {
			m_pathprobe = -1;
			m_signal_log(log_fplproposal, get_simplefpl());
			m_signal_log(log_normal, "No progress made; stopping...");
			stop(statusmask_stoppingerrorenroute);
			return;
		}
		lgraphroute();
		return;
	}
	work = false;
	for (validationresponse_t::const_iterator i(m_validationresponse.begin()), e(m_validationresponse.end()); i != e; ++i) {
		if (i->empty())
			continue;
		m_signal_log(log_graphrule, *i);
		{
			std::string::size_type n(i->rfind('['));
			if (n != std::string::npos) {
				std::string::size_type ne(i->find(']', n + 1));
				if (ne != std::string::npos && ne - n >= 2 && ne - n <= 11) {
					std::vector<TrafficFlowRestrictions::RuleResult> rr(m_tfr.find_rules(i->substr(n + 1, ne - n - 1), true));
					if (!rr.empty()) {
						m_signal_log(log_graphruledesc, rr[0].get_desc());
						m_signal_log(log_graphruleoprgoal, rr[0].get_oprgoal());
					}
				}
			}
		}
		if (false)
			std::cerr << "Message: " << *i << std::endl;
		const struct parsefunctions *pf(parsefunctions);
		for (; pf->regex; ++pf) {
			Glib::RefPtr<Glib::Regex> rx(Glib::Regex::create(pf->regex));
			Glib::MatchInfo minfo;
			if (!rx->match(*i, minfo))
				continue;
			if (pf->func) {
				work = (this->*(pf->func))(minfo) || work;
				if (m_running)
					break;
				m_signal_log(log_fplproposal, get_simplefpl());
				m_signal_log(log_normal, "Stopping...");
				return;
			}
			break;
		}
		if (!pf->regex) {
			const char * const *ignrx(ignoreregex);
			while (*ignrx) {
				Glib::RefPtr<Glib::Regex> rx(Glib::Regex::create(*ignrx++));
				Glib::MatchInfo minfo;
				if (!rx->match(*i, minfo))
					continue;
				break;
			}
			if (!*ignrx)
				m_signal_log(log_normal, "Error Message not understood: \"" + *i + "\"");
		}
	}
	if (work)
		lgraphmodified();
	if (!work && m_pathprobe >= 0 && m_pathprobe + 1 < m_route.get_nrwpt()) {
		std::string id0(m_route[m_pathprobe].get_icao());
		if (id0.empty())
			id0 = m_route[m_pathprobe].get_name();
		std::string id1(m_route[m_pathprobe + 1].get_icao());
		if (id1.empty())
			id1 = m_route[m_pathprobe + 1].get_name();
		work = lgraphmodifyawysegment(id0, id1, airwaymatchall, airwaymatchnone, 0, 600, false, true);
	}
	// disable path probing
	if (work || true) {
		if (m_pathprobe >= 0)
			m_signal_log(log_normal, "Resuming normal operation...");
		m_pathprobe = -1;
		lgraphroute();
		return;
	}
	for (;;) {
		++m_pathprobe;
		if (m_pathprobe >= m_route.get_nrwpt())
			break;
		const FPlanWaypoint& wpt(m_route[m_pathprobe]);
		if (wpt.get_flags() & FPlanWaypoint::altflag_ifr &&
		    wpt.get_pathcode() != FPlanWaypoint::pathcode_sid &&
		    wpt.get_pathcode() != FPlanWaypoint::pathcode_star)
			break;
	}
	if (m_pathprobe >= m_route.get_nrwpt()) {
		m_pathprobe = -1;
		m_signal_log(log_fplproposal, get_simplefpl());
		m_signal_log(log_normal, "No progress made; stopping...");
		stop(statusmask_stoppingerrorenroute);
		return;
	}
	if (m_pathprobe == 0)
		m_signal_log(log_normal, "No progress made; starting path probing...");
	lgraphroute();
}

const struct CFMUAutoroute45::parsefunctions CFMUAutoroute45::parsefunctions[] = {
	{ "^ROUTE49: THE POINT ([[:alnum:]]+) IS UNKNOWN IN THE CONTEXT OF THE ROUTE", &CFMUAutoroute45::parse_response_route49 },
	{ "^ROUTE52: THE DCT SEGMENT ([[:alnum:]]+)..([[:alnum:]]+) IS FORBIDDEN", &CFMUAutoroute45::parse_response_route52 },
	{ "^ROUTE130: UNKNOWN DESIGNATOR ([[:alnum:]]+)", &CFMUAutoroute45::parse_response_route130 },
	{ "^ROUTE134: THE STAR LIMIT IS EXCEEDED FOR AERODROME .*? CONNECTING TO ([[:alnum:]]+).", &CFMUAutoroute45::parse_response_route134 },
	{ "^ROUTE135: THE SID LIMIT IS EXCEEDED FOR AERODROME .*? CONNECTING TO ([[:alnum:]]+).", &CFMUAutoroute45::parse_response_route135 },
	{ "^ROUTE139: ([[:alnum:]]+) IS PRECEDED BY ([[:alnum:]]+) WHICH IS NOT ONE OF ITS POINTS", &CFMUAutoroute45::parse_response_route139 },
	{ "^ROUTE140: ([[:alnum:]]+) IS FOLLOWED BY ([[:alnum:]]+) WHICH IS NOT ONE OF ITS POINTS", &CFMUAutoroute45::parse_response_route140 },
	{ "^ROUTE165: THE DCT SEGMENT ([[:alnum:]]+)..([[:alnum:]]+)", &CFMUAutoroute45::parse_response_route165 },
	{ "^ROUTE168: INVALID DCT ([[:alnum:]]+)..([[:alnum:]]+).", &CFMUAutoroute45::parse_response_route168 },
	{ "^ROUTE171: CANNOT EXPAND THE ROUTE ([[:alnum:]]+)", &CFMUAutoroute45::parse_response_route171 },
	{ "^ROUTE172: MULTIPLE ROUTES BETWEEN ([[:alnum:]]+) AND ([[:alnum:]]+). ([[:alnum:]]+) IS SUGGESTED.", &CFMUAutoroute45::parse_response_route172 },
	{ "^ROUTE179: CRUISING FLIGHT LEVEL INVALID OR INCOMPATIBLE WITH AIRCRAFT PERFORMANCE", &CFMUAutoroute45::parse_response_route179 },
	{ "^PROF50: CLIMBING/DESCENDING OUTSIDE THE VERTICAL LIMITS OF SEGMENT ([[:alnum:]]+) ([[:alnum:]]+) ([[:alnum:]]+)", &CFMUAutoroute45::parse_response_prof50 },
	{ "^PROF193: IFR OPERATIONS AT AERODROME ([[:alnum:]]+) ARE NOT PERMITTED", &CFMUAutoroute45::parse_response_prof193 },
	{ "^PROF194: ([[:alnum:]]+) ([[:alnum:]]+) ([[:alnum:]]+) IS NOT AVAILABLE IN FL RANGE F([[:digit:]]+)..F([[:digit:]]+)", &CFMUAutoroute45::parse_response_prof194 },
	{ "^PROF195: ([[:alnum:]]+) ([[:alnum:]]+) ([[:alnum:]]+) DOES NOT EXIST IN FL RANGE F([[:digit:]]+)..F([[:digit:]]+)", &CFMUAutoroute45::parse_response_prof195 },
	{ "^PROF195: ([[:alnum:]]+) ([[:alnum:]]+) ([[:alnum:]]+) DOES NOT EXIST IN FL RANGE", &CFMUAutoroute45::parse_response_prof195b },
	{ "^PROF197: RS: ([[:alnum:]]+) ([[:alnum:]]+) ([[:alnum:]]+):F([[:digit:]]+)..F([[:digit:]]+) IS CLOSED FOR CRUISING", &CFMUAutoroute45::parse_response_prof197 },
	{ "^PROF198: ([[:alnum:]]+) ([[:alnum:]]+) ([[:alnum:]]+) IS A CDR 3 IN FL RANGE F([[:digit:]]+)..F([[:digit:]]+)", &CFMUAutoroute45::parse_response_prof198 },
	{ "^PROF199: ([[:alnum:]]+) ([[:alnum:]]+) ([[:alnum:]]+) IS A CLOSED CDR 2 IN FL RANGE F([[:digit:]]+)..F([[:digit:]]+)", &CFMUAutoroute45::parse_response_prof199 },
	{ "^PROF201: CANNOT CLIMB OR DESCEND ON ([[:alnum:]]+) ([[:alnum:]]+) ([[:alnum:]]+) IN FL RANGE CLOSED", &CFMUAutoroute45::parse_response_prof201 },
	{ "^PROF201: CANNOT CLIMB OR DESCEND ON ([[:alnum:]]+) ([[:alnum:]]+) ([[:alnum:]]+) IN FL RANGE F([[:digit:]]+)..F([[:digit:]]+)", &CFMUAutoroute45::parse_response_prof201b },
	{ "^PROF204: RS: TRAFFIC VIA ([[:alnum:]]+) IS ON FORBIDDEN ROUTE REF:\\[([[:alnum:]]+)\\] ([[:alnum:]]+)", &CFMUAutoroute45::parse_response_prof204e },
	{ "^PROF204: RS: TRAFFIC VIA ((?:[[:alnum:]]+)(?: [[:alnum:]]+)*?) IS ON FORBIDDEN ROUTE", &CFMUAutoroute45::parse_response_prof204 },
	{ "^PROF204: RS: TRAFFIC VIA ((?:[[:alnum:]]+)(?: [[:alnum:]]+)*?):F([[:digit:]]+)..F([[:digit:]]+) IS ON FORBIDDEN ROUTE", &CFMUAutoroute45::parse_response_prof204b },
	{ "^PROF204: RS: TRAFFIC VIA ([[:alnum:]]+) ([[:alnum:]]+):F([[:digit:]]+)..F([[:digit:]]+) IS ON FORBIDDEN ROUTE", &CFMUAutoroute45::parse_response_prof204c },
	{ "^PROF204: RS: TRAFFIC VIA ([[:alnum:]]+) ([[:alnum:]]+) ([[:alnum:]]+):F([[:digit:]]+)..F([[:digit:]]+) IS ON FORBIDDEN ROUTE", &CFMUAutoroute45::parse_response_prof204d },
	{ "^PROF205: RS: TRAFFIC VIA ([[:alnum:]]+) IS OFF MANDATORY ROUTE", &CFMUAutoroute45::parse_response_prof205 },
	{ "^PROF205: RS: TRAFFIC VIA ([[:alnum:]]+) ([[:alnum:]]+) ([[:alnum:]]+) IS OFF MANDATORY ROUTE", &CFMUAutoroute45::parse_response_prof205b },
	{ "^PROF205: RS: TRAFFIC VIA ([[:alnum:]]+) ([[:alnum:]]+) ([[:alnum:]]+):F([[:digit:]]+)..F([[:digit:]]+) IS OFF MANDATORY ROUTE", &CFMUAutoroute45::parse_response_prof205c },
	{ "^PROF206: THE DCT SEGMENT ([[:alnum:]]+) .. ([[:alnum:]]+) IS NOT AVAILABLE IN FL RANGE F([[:digit:]]+)..F([[:digit:]]+)", &CFMUAutoroute45::parse_response_prof206 },
	{ "^EFPM228: INVALID VALUE \\(([[:alnum:]]+)\\)", &CFMUAutoroute45::parse_response_efpm228 },
	{ "^FAIL: (.*)", &CFMUAutoroute45::parse_response_fail },
	{ 0, 0 }
};

const char *CFMUAutoroute45::ignoreregex[] = {
	// ignore 8.33 carriage errors
	"^PROF188:",
	"^PROF189:",
	"^PROF190:",
	0
};

bool CFMUAutoroute45::parse_response_route49(Glib::MatchInfo& minfo)
{
	return lgraphdisconnectvertex(minfo.fetch(1).uppercase(), 0, 600, true);
}

bool CFMUAutoroute45::parse_response_route52(Glib::MatchInfo& minfo)
{
	return lgraphmodifyedge(minfo.fetch(1), minfo.fetch(2), airwaydct, airwaymatchnone, 0, 600, true);
}

bool CFMUAutoroute45::parse_response_route130(Glib::MatchInfo& minfo)
{
	std::string idu(minfo.fetch(1).uppercase());
	bool work(lgraphdisconnectvertex(idu, 0, 600, true));
	lgraphairwayindex_t awy(lgraphairway2index(idu));
	if (awy != airwaydct && awy != airwaymatchnone)
		work = lgraphedgetodct(awy) || work;
	return work;
}

bool CFMUAutoroute45::parse_response_route134(Glib::MatchInfo& minfo)
{
	bool work(false);
	LGraph::edge_iterator e0, e1;
	for (tie(e0, e1) = edges(m_lgraph); e0 != e1; ++e0) {
		LEdge& ee(m_lgraph[*e0]);
		if (!ee.is_solution())
			continue;
		if (!ee.is_match(airwaystar))
			continue;
		if (!ee.is_solution_valid())
			continue;
		unsigned int pi(ee.get_solution());
		lgraphkillsolutionedge(*e0, true);
		work = true;
		{
			const LVertex& vv0(m_lgraph[boost::source(*e0, m_lgraph)]);
			const LVertex& vv1(m_lgraph[boost::target(*e0, m_lgraph)]);
			const Performance::Cruise& cruise(m_performance.get_cruise(pi));
			m_cfmuintel.add(CFMUIntel::ForbiddenSegment(vv0.get_ident(), vv0.get_coord(), cruise.get_altitude(),
								    vv1.get_ident(), vv1.get_coord(), cruise.get_altitude(),
								    airwaystar));
		}
	}
	return work;
}

bool CFMUAutoroute45::parse_response_route135(Glib::MatchInfo& minfo)
{
	bool work(false);
	LGraph::edge_iterator e0, e1;
	for (tie(e0, e1) = edges(m_lgraph); e0 != e1; ++e0) {
		LEdge& ee(m_lgraph[*e0]);
		if (!ee.is_solution())
			continue;
		if (!ee.is_match(airwaysid))
			continue;
		if (!ee.is_solution_valid())
			continue;
		unsigned int pi(ee.get_solution());
		lgraphkillsolutionedge(*e0, true);
		work = true;
		{
			const LVertex& vv0(m_lgraph[boost::source(*e0, m_lgraph)]);
			const LVertex& vv1(m_lgraph[boost::target(*e0, m_lgraph)]);
			const Performance::Cruise& cruise(m_performance.get_cruise(pi));
			m_cfmuintel.add(CFMUIntel::ForbiddenSegment(vv0.get_ident(), vv0.get_coord(), cruise.get_altitude(),
								    vv1.get_ident(), vv1.get_coord(), cruise.get_altitude(),
								    airwaysid));
		}
	}
	return work;
}

bool CFMUAutoroute45::parse_response_route139(Glib::MatchInfo& minfo)
{
	lgraphairwayindex_t awy(lgraphairway2index(minfo.fetch(1)));
	if (awy == airwaymatchnone || awy == airwaydct)
		return false;
	return lgraphchangeoutgoing(minfo.fetch(2), awy);
}

bool CFMUAutoroute45::parse_response_route140(Glib::MatchInfo& minfo)
{
	lgraphairwayindex_t awy(lgraphairway2index(minfo.fetch(1)));
	if (awy == airwaymatchnone || awy == airwaydct)
		return false;
	return lgraphchangeincoming(minfo.fetch(2), awy);
}

bool CFMUAutoroute45::parse_response_route165(Glib::MatchInfo& minfo)
{
	return lgraphmodifyedge(minfo.fetch(1), minfo.fetch(2), airwaydct, airwaymatchnone, 0, 600, true);
}

bool CFMUAutoroute45::parse_response_route168(Glib::MatchInfo& minfo)
{
	return lgraphmodifyedge(minfo.fetch(1), minfo.fetch(2), airwaydct, airwaymatchnone, 0, 600, true);
}

bool CFMUAutoroute45::parse_response_route171(Glib::MatchInfo& minfo)
{
	lgraphairwayindex_t awy(lgraphairway2index(minfo.fetch(1), true));
	if (awy == airwaydct)
		return false;
	return lgraphedgetodct(awy);
}

bool CFMUAutoroute45::parse_response_route172(Glib::MatchInfo& minfo)
{
	lgraphairwayindex_t awy(lgraphairway2index(minfo.fetch(3), true));
	if (awy == airwaydct)
		return false;
	return lgraphmodifyedge(minfo.fetch(1), minfo.fetch(2), airwaydct, awy, 0, 600, false);
}

bool CFMUAutoroute45::parse_response_route179(Glib::MatchInfo& minfo)
{
	unsigned int pis(0);
	{
		LGraph::edge_iterator e0, e1;
		for (tie(e0, e1) = edges(m_lgraph); e0 != e1; ++e0) {
			LEdge& ee(m_lgraph[*e0]);
			if (!ee.is_solution())
				continue;
			if (!ee.is_solution_valid())
				continue;
			pis = std::max(pis, ee.get_solution());
		}
	}
	if (pis >= m_performance.size())
		return false;
	if (!pis) {
		stop(statusmask_stoppingerrorinternalerror);
		return false;
	}
	m_performance.resize(pis);
	{
		LGraph::edge_iterator e0, e1;
		for (tie(e0, e1) = edges(m_lgraph); e0 != e1; ++e0) {
			LEdge& ee(m_lgraph[*e0]);
			ee.resize(pis);
		}
	}
	return true;
}

bool CFMUAutoroute45::parse_response_prof50(Glib::MatchInfo& minfo)
{
	lgraphairwayindex_t awy(lgraphairway2index(minfo.fetch(2)));
	if (awy != airwaymatchnone && lgraphmodifyawysegment(minfo.fetch(1), minfo.fetch(3), awy, airwaymatchnone, 0, 600, true, true))
		return true;
	return lgraphmodifyawysegment(minfo.fetch(1), minfo.fetch(3), airwaydct, airwaymatchnone, 0, 600, true, true);
}

bool CFMUAutoroute45::parse_response_prof193(Glib::MatchInfo& minfo)
{
	bool work(false);
	std::cerr << "Invalid IFR: " << minfo.fetch(1) << " DEP " << get_departure().get_icao() << " DEST " << get_destination().get_icao() << std::endl;
	if (get_departure().is_valid() && get_departure().get_icao() == minfo.fetch(1)) {
		work = work || get_departure_ifr();
	        m_airportifr[0] = false;
	}
	if (get_destination().is_valid() && get_destination().get_icao() == minfo.fetch(1)) {
		work = work || get_destination_ifr();
	        m_airportifr[1] = false;
	}
	return work;
}

bool CFMUAutoroute45::parse_response_prof194(Glib::MatchInfo& minfo)
{
	return parse_response_prof195(minfo);
}

bool CFMUAutoroute45::parse_response_prof195(Glib::MatchInfo& minfo)
{
	int altfrom(strtol(minfo.fetch(4).c_str(), 0, 10));
	int altto(strtol(minfo.fetch(5).c_str(), 0, 10));
	if (altfrom > altto)
		return false;
	lgraphairwayindex_t awy(lgraphairway2index(minfo.fetch(2)));
	if (awy != airwaymatchnone) {
		if (lgraphmodifyawysegment(minfo.fetch(1), minfo.fetch(3), awy, airwaymatchnone, altfrom, altto, true, false))
			return true;
		if (lgraphmodifysolutionsegment(minfo.fetch(1), minfo.fetch(3), awy, airwaymatchnone, altfrom, altto, false))
			return true;
	}
	return lgraphmodifyawysegment(minfo.fetch(1), minfo.fetch(3), airwaydct, airwaymatchnone, altfrom, altto, true, false);
}

bool CFMUAutoroute45::parse_response_prof195b(Glib::MatchInfo& minfo)
{
	lgraphairwayindex_t awy(lgraphairway2index(minfo.fetch(2)));
	if (awy == airwaymatchnone || awy == airwaydct)
		return false;
	return lgraphmodifyedge(minfo.fetch(1), minfo.fetch(3), awy, airwaymatchnone);
}

bool CFMUAutoroute45::parse_response_prof197(Glib::MatchInfo& minfo)
{
	int altfrom(strtol(minfo.fetch(4).c_str(), 0, 10));
	int altto(strtol(minfo.fetch(5).c_str(), 0, 10));
	if (altfrom > altto)
		return false;
	lgraphairwayindex_t awy(lgraphairway2index(minfo.fetch(2)));
	if (awy != airwaymatchnone && lgraphmodifyawysegment(minfo.fetch(1), minfo.fetch(3), awy, airwaymatchnone, altfrom, altto, true, true))
		return true;
	return lgraphmodifyawysegment(minfo.fetch(1), minfo.fetch(3), airwaydct, airwaymatchnone, altfrom, altto, true, true);
}

bool CFMUAutoroute45::parse_response_prof198(Glib::MatchInfo& minfo)
{
	return parse_response_prof195(minfo);
}

bool CFMUAutoroute45::parse_response_prof199(Glib::MatchInfo& minfo)
{
	return parse_response_prof195(minfo);
}

bool CFMUAutoroute45::parse_response_prof201(Glib::MatchInfo& minfo)
{
	if (parse_response_prof50(minfo))
		return true;
	if (lgraphkillclosesegments(minfo.fetch(0), minfo.fetch(2), minfo.fetch(1), airwaydct))
		return true;
	bool work(lgraphkillsid(minfo.fetch(1)));
	work = lgraphkillsid(minfo.fetch(3)) || work;
	return work;
}

bool CFMUAutoroute45::parse_response_prof201b(Glib::MatchInfo& minfo)
{
	int altfrom(strtol(minfo.fetch(4).c_str(), 0, 10));
	int altto(strtol(minfo.fetch(5).c_str(), 0, 10));
	if (altfrom > altto)
		return false;
	lgraphairwayindex_t awy(lgraphairway2index(minfo.fetch(2)));
	if (awy != airwaymatchnone && lgraphmodifyawysegment(minfo.fetch(1), minfo.fetch(3), awy, airwaymatchnone, altfrom, altto, true, true))
		return true;
	if (lgraphmodifyawysegment(minfo.fetch(1), minfo.fetch(3), airwaydct, airwaymatchnone, altfrom, altto, true, true))
		return true;
	if (lgraphkillclosesegments(minfo.fetch(0), minfo.fetch(2), minfo.fetch(1), airwaydct))
		return true;
	bool work(lgraphkillsid(minfo.fetch(1)));
	work = lgraphkillsid(minfo.fetch(3)) || work;
	return work;
}

bool CFMUAutoroute45::parse_response_prof204(Glib::MatchInfo& minfo)
{
	bool work(false);
	if (false)
		std::cerr << "PROF204: \"" << minfo.fetch(1) << "\" \"" << minfo.fetch(0) << "\"" << std::endl;
	typedef std::vector<std::string> token_t;
	token_t token(Glib::Regex::split_simple(" +", minfo.fetch(1)));
 	for (int i = 0; i < token.size(); ++i) {
 	 	if (i + 2 < token.size() &&
		    !Engine::AirwayGraphResult::Vertex::is_ident_numeric(token[i]) &&
		    Engine::AirwayGraphResult::Vertex::is_ident_numeric(token[i + 1]) &&
		    !Engine::AirwayGraphResult::Vertex::is_ident_numeric(token[i + 2])) {
			if (true)
				std::cerr << "PROF204: kill " << token[i] << "--" << token[i + 1] << "->" << token[i + 2] << std::endl;
			lgraphairwayindex_t awy(lgraphairway2index(token[i + 1]));
			if (awy != airwaymatchnone && awy != airwaydct) {
				bool work2(lgraphmodifyawysegment(token[i], token[i + 2], awy, airwaymatchnone, 0, 600, true, true));
				if (!work2) {
					if (true)
						std::cerr << "PROF204: " << token[i] << "--" << lgraphindex2airway(awy) << "->" << token[i + 2]
							  << " did not match, trying DCT" << std::endl;
					work2 = lgraphmodifyawysegment(token[i], token[i + 2], airwaydct, airwaymatchnone, 0, 600, true, true);
					if (!work2) {
						if (true)
							std::cerr << "PROF204: " << token[i] << "--->" << token[i + 2] << " did not match, trying airway convert to DCT" << std::endl;
						work2 = lgraphedgetodct(awy);
						if (!work2) {
							std::cerr << "PROF204: could not convert " << lgraphindex2airway(awy)
								  << " to DCT, removing incoming/outgoing edges" << std::endl;
							work2 = lgraphkilloutgoing(token[i]);
							work2 = lgraphkillincoming(token[i + 2]) || work2;
						}
					}
				}
				work = work2 || work;
			}
			i += 2;
			continue;
		}
		if (true)
			std::cerr << "PROF204: kill " << token[i] << std::endl;
		work = lgraphdisconnectsolutionvertex(token[i]) || work;
		if (!work && token[i].size() >= 4) {
			opendb();
			AirspacesDb::elementvector_t ev(m_airspacedb.find_by_icao(token[i], 0, /* AirspacesDb::comp_startswith */ AirspacesDb::comp_exact,
										  0, AirspacesDb::Airspace::subtables_all));
			for (AirspacesDb::elementvector_t::const_iterator ei(ev.begin()), ee(ev.end()); ei != ee; ++ei) {
				const AirspacesDb::Airspace& aspc(*ei);
				if (!aspc.is_valid() || aspc.get_typecode() != AirspacesDb::Airspace::typecode_ead)
					continue;
				{
					std::ostringstream oss;
					oss << "A " << aspc.get_icao_name();
					m_signal_log(log_graphchange, oss.str());
				}
				const MultiPolygonHole& poly(aspc.get_polygon());
				Rect bbox(poly.get_bbox());
				LGraph::edge_iterator e0, e1;
				for (tie(e0, e1) = edges(m_lgraph); e0 != e1; ++e0) {
					LEdge& ee(m_lgraph[*e0]);
					if (!ee.is_solution())
						continue;
					LGraph::vertex_descriptor u(source(*e0, m_lgraph));
					const LVertex& uu(m_lgraph[u]);
					LGraph::vertex_descriptor v(target(*e0, m_lgraph));
					const LVertex& vv(m_lgraph[v]);
					bool insideu(bbox.is_inside(uu.get_coord()) && poly.windingnumber(uu.get_coord()));
					bool insidev(bbox.is_inside(vv.get_coord()) && poly.windingnumber(vv.get_coord()));
					bool deledge(insideu || insidev || poly.is_intersection(uu.get_coord(), vv.get_coord()));
					if (!deledge)
						continue;
					lgraphkillsolutionedge(*e0, true);
					work = true;
				}
			}
		}
	}
	if (work)
		return true;
	if (token.size() >= 3 && 
	    !Engine::AirwayGraphResult::Vertex::is_ident_numeric(token[0]) &&
	    Engine::AirwayGraphResult::Vertex::is_ident_numeric(token[1]) &&
	    !Engine::AirwayGraphResult::Vertex::is_ident_numeric(token[2])) {
		work = lgraphkillclosesegments(token[0], token[2], token[1], airwaydct);
	}
	return work;
}

bool CFMUAutoroute45::parse_response_prof204b(Glib::MatchInfo& minfo)
{
	int altfrom(strtol(minfo.fetch(2).c_str(), 0, 10));
	int altto(strtol(minfo.fetch(3).c_str(), 0, 10));
	if (altfrom > altto)
		return false;
	bool work(false);
	if (false)
		std::cerr << "PROF204: \"" << minfo.fetch(1) << "\" \"" << minfo.fetch(0) << "\"" << std::endl;
	typedef std::vector<std::string> token_t;
	token_t token(Glib::Regex::split_simple(" +", minfo.fetch(1)));
 	for (int i = 0; i < token.size(); ++i) {
 	 	if (i + 2 < token.size() &&
		    !Engine::AirwayGraphResult::Vertex::is_ident_numeric(token[i]) &&
		    Engine::AirwayGraphResult::Vertex::is_ident_numeric(token[i + 1]) &&
		    !Engine::AirwayGraphResult::Vertex::is_ident_numeric(token[i + 2])) {
			if (true)
				std::cerr << "PROF204: kill " << token[i] << "--" << token[i + 1] << "->" << token[i + 2] << std::endl;
			lgraphairwayindex_t awy(lgraphairway2index(token[i + 1]));
			if (awy != airwaymatchnone && awy != airwaydct) {
				bool work2(lgraphmodifyawysegment(token[i], token[i + 2], awy, airwaymatchnone, altfrom, altto, true, true));
				if (!work2) {
					if (true)
						std::cerr << "PROF204: " << token[i] << "--" << lgraphindex2airway(awy) << "->" << token[i + 2]
							  << " did not match, trying DCT" << std::endl;
					work2 = lgraphmodifyawysegment(token[i], token[i + 2], airwaydct, airwaymatchnone, altfrom, altto, true, true);
					if (!work2) {
						if (true)
							std::cerr << "PROF204: " << token[i] << "--->" << token[i + 2] << " did not match, trying airway convert to DCT" << std::endl;
						work2 = lgraphedgetodct(awy);
						if (!work2) {
							std::cerr << "PROF204: could not convert " << lgraphindex2airway(awy)
								  << " to DCT, removing incoming/outgoing edges" << std::endl;
							work2 = lgraphkilloutgoing(token[i]);
							work2 = lgraphkillincoming(token[i + 2]) || work2;
						}
					}
				}
				work = work2 || work;
			}
			i += 2;
			continue;
		}
		if (true)
			std::cerr << "PROF204: kill " << token[i] << std::endl;
		work = lgraphdisconnectvertex(token[i], altfrom, altto) || work;
	}
	return work;
}

bool CFMUAutoroute45::parse_response_prof204c(Glib::MatchInfo& minfo)
{
	int altfrom(strtol(minfo.fetch(3).c_str(), 0, 10));
	int altto(strtol(minfo.fetch(4).c_str(), 0, 10));
	if (altfrom > altto)
		return false;
	return lgraphmodifyedge(minfo.fetch(1), minfo.fetch(2), airwaymatchdctawy, airwaymatchnone, altfrom, altto, true);
}

bool CFMUAutoroute45::parse_response_prof204d(Glib::MatchInfo& minfo)
{
	int altfrom(strtol(minfo.fetch(4).c_str(), 0, 10));
	int altto(strtol(minfo.fetch(5).c_str(), 0, 10));
	if (altfrom > altto)
		return false;
	lgraphairwayindex_t awy(lgraphairway2index(minfo.fetch(2)));
	if (awy != airwaymatchnone && lgraphmodifyawysegment(minfo.fetch(1), minfo.fetch(3), awy, airwaymatchnone, altfrom, altto, true, true))
		return true;
	return lgraphmodifyawysegment(minfo.fetch(1), minfo.fetch(3), airwaydct, airwaymatchnone, altfrom, altto, true, true);
}

bool CFMUAutoroute45::parse_response_prof204e(Glib::MatchInfo& minfo)
{
	return lgraphdisconnectsolutionvertex(minfo.fetch(3));
}

bool CFMUAutoroute45::parse_response_prof205(Glib::MatchInfo& minfo)
{
	return lgraphdisconnectsolutionvertex(minfo.fetch(1));
}

bool CFMUAutoroute45::parse_response_prof205b(Glib::MatchInfo& minfo)
{
	return parse_response_prof50(minfo);
}

bool CFMUAutoroute45::parse_response_prof205c(Glib::MatchInfo& minfo)
{
	return parse_response_prof204d(minfo);
}

bool CFMUAutoroute45::parse_response_prof206(Glib::MatchInfo& minfo)
{
	int altfrom(strtol(minfo.fetch(3).c_str(), 0, 10));
	int altto(strtol(minfo.fetch(4).c_str(), 0, 10));
	if (altfrom > altto)
		return false;
	return lgraphmodifyedge(minfo.fetch(1), minfo.fetch(2), airwaydct, airwaymatchnone, altfrom, altto, true);
}

bool CFMUAutoroute45::parse_response_efpm228(Glib::MatchInfo& minfo)
{
	if (minfo.fetch(1).uppercase() == "ADEP") {
		m_signal_log(log_debug0, "Invalid departure, stopping...");
		stop(statusmask_stoppingerrorinternalerror);
	} else if (minfo.fetch(1).uppercase() == "ADES") {
		m_signal_log(log_debug0, "Invalid destination, stopping...");
		stop(statusmask_stoppingerrorinternalerror);
	}
	return false;
}

bool CFMUAutoroute45::parse_response_fail(Glib::MatchInfo& minfo)
{
	m_signal_log(log_debug0, "Fatal Error: " + minfo.fetch(1));
	stop(statusmask_stoppingerrorinternalerror);
	return false;
}

//PROF204: RS: TRAFFIC VIA DKB N869 RONIG IS ON FORBIDDEN ROUTE REF:[ED2129A] N869 DKB RONIG
// airway Q295: CLN 5150N00057E DAGGA
//PROF195: 5150N00057E Q295 DAGGA DOES NOT EXIST IN FL RANGE F000..F085
//PROF201: CANNOT CLIMB OR DESCEND ON SOPER N851 PIXOS IN FL RANGE CLOSED, BECAUSE OF UNAVAILABLE LEVELS ON N851
//PROF50: CLIMBING/DESCENDING OUTSIDE THE VERTICAL LIMITS OF SEGMENT NUNRI T103 KUDIS
//PROF205: RS: TRAFFIC VIA EGHL+4 IS OFF MANDATORY ROUTE REF:[EG2380D] GWC COMP FOR TRAFFIC 1.DEP FARNBOROUGH G
//PROF205: RS: TRAFFIC VIA EGHH EGHI EGHO EGHL+4 IS OFF MANDATORY ROUTE REF:[EGOG5112A] GWC DCT SFD APP.4
//PROF204: RS: TRAFFIC VIA EDGGADF IS ON FORBIDDEN ROUTE REF:[ED3115A] EDGGADF
//ROUTE171: CANNOT EXPAND THE ROUTE L613
//PROF193: IFR OPERATIONS AT AERODROME LSZK ARE NOT PERMITTED

//this happens without DOF if the time of day is around the time of dep in the FPL
//EFPM51: FPL PROCESSED AFTER ESTIMATED TIME OF ARRIVAL

// Airspace BG has 49 navaids, 108 intersections
// Airspace BI has 82 navaids, 77 intersections
// Airspace CFMU1 CFMU1 has 3893 navaids, 14369 intersections
// Airspace CY-AERODB has 1294 navaids, 14666 intersections
// Airspace CY has 987 navaids, 10145 intersections
// Airspace DA has 89 navaids, 128 intersections
// Airspace DT has 25 navaids, 154 intersections
// Airspace EB MIL EB MIL DOWNLOAD has 114 navaids, 322 intersections
// Airspace EB-EAD-1 has 85 navaids, 194 intersections
// Airspace EB-EAD-2 has 0 navaids, 3 intersections
// Airspace EB-EAD-3 has 1 navaids, 3 intersections
// Airspace EB-EAD-5 has 3 navaids, 18 intersections
// Airspace EBDWNLOAD1 EBDOWNLOAD1 has 681 navaids, 3320 intersections
// Airspace EBDWNLOAD2 EBDOWNLOAD2 has 231 navaids, 1035 intersections
// Airspace ED has 355 navaids, 3156 intersections
// Airspace EE has 20 navaids, 94 intersections
// Airspace EF has 104 navaids, 385 intersections
// Airspace EG2 has 56 navaids, 143 intersections
// Airspace EG has 362 navaids, 1126 intersections
// Airspace EH-EAD-1 EH-EAD-1 has 101 navaids, 605 intersections
// Airspace EH-EAD-2 EH-EAD-2 has 0 navaids, 33 intersections
// Airspace EI-EAD-1 EI-EAD-1 has 60 navaids, 217 intersections
// Airspace EI-EAD-2 EI-EAD-2 has 0 navaids, 6 intersections
// Airspace EI-EAD-3 EI-EAD-3 has 0 navaids, 1 intersections
// Airspace EI-EAD-4 EI-EAD-4 has 0 navaids, 22 intersections
// Airspace EI-EAD-5 EI-EAD-5 has 0 navaids, 19 intersections
// Airspace EK-EAD-1 EK-EAD-1 has 74 navaids, 403 intersections
// Airspace EK-EAD-2 EK-EAD-2 has 0 navaids, 51 intersections
// Airspace EK-EAD-3 EK-EAD-3 has 5 navaids, 26 intersections
// Airspace EK-EAD-4 EK-EAD-4 has 6 navaids, 20 intersections
// Airspace EN-EAD-1 EN EAD FIR PLUS DELEGATED AREAS has 270 navaids, 1185 intersections
// Airspace EN-EAD-2 NO_FIR AREA NEAR NORWAY has 0 navaids, 10 intersections
// Airspace EN-EAD-3 JAN MAYEN ISLAND (JAN NDB LOCATION) has 1 navaids, 0 intersections
// Airspace EP has 146 navaids, 584 intersections
// Airspace ES has 241 navaids, 1028 intersections
// Airspace EV-EAD-1 EV-EAD-1 has 14 navaids, 137 intersections
// Airspace EV-EAD-3 EV-EAD-3 has 3 navaids, 17 intersections
// Airspace EV-EAD-4 EV-EAD-4 has 2 navaids, 9 intersections
// Airspace EV-EAD-5 EV-EAD-5 has 0 navaids, 1 intersections
// Airspace EY has 17 navaids, 159 intersections
// Airspace FA has 187 navaids, 256 intersections
// Airspace FABEC1N FABEC PROJECT PHASE 1 NORTH has 57 navaids, 531 intersections
// Airspace FABEC1W FABEC PROJECT PHASE 1 WEST has 102 navaids, 305 intersections
// Airspace FABEC2N FABEC PROJECT PHASE 2 NORTH has 159 navaids, 814 intersections
// Airspace FABECSOUTH FABEC PROJECT PHASE SOUTH has 184 navaids, 852 intersections
// Airspace FD_EAD has 4 navaids, 6 intersections
// Airspace FX_EAD has 4 navaids, 3 intersections
// Airspace FY_EAD has 4 navaids, 3 intersections
// Airspace G1AA AREA OF RESPONSIBILITY 1 has 5384 navaids, 18698 intersections
// Airspace G1BB AREA OF RESPONSIBILITY 2 has 453 navaids, 6887 intersections
// Airspace G1CC AREA OF RESPONSIBILITY 3 has 2333 navaids, 9117 intersections
// Airspace GM has 59 navaids, 158 intersections
// Airspace GO has 97 navaids, 334 intersections
// Airspace GV has 7 navaids, 39 intersections
// Airspace HE has 56 navaids, 93 intersections
// Airspace HL has 61 navaids, 123 intersections
// Airspace KCHART1 COVERAGE OF CHARTS U.S.A. has 969 navaids, 13909 intersections
// Airspace LA has 5 navaids, 30 intersections
// Airspace LB has 36 navaids, 118 intersections
// Airspace LC has 10 navaids, 67 intersections
// Airspace LD-EAD-1 EAD AIRSPACE CROATIA has 43 navaids, 131 intersections
// Airspace LD-EAD-2 EAD AIRSPACE DELEGATED TO CROATIA has 1 navaids, 4 intersections
// Airspace LD-EAD-3 AIRSPACE DELEGATION TO CROATIA has 0 navaids, 10 intersections
// Airspace LE_1 has 265 navaids, 826 intersections
// Airspace LE_2 has 50 navaids, 115 intersections
// Airspace LF has 482 navaids, 1549 intersections
// Airspace LG has 119 navaids, 225 intersections
// Airspace LH has 79 navaids, 252 intersections
// Airspace LI has 269 navaids, 766 intersections
// Airspace LICD LAMPEDUSA has 4 navaids, 5 intersections
// Airspace LJ has 17 navaids, 79 intersections
// Airspace LK has 97 navaids, 443 intersections
// Airspace LL has 28 navaids, 51 intersections
// Airspace LM has 14 navaids, 93 intersections
// Airspace LO has 50 navaids, 214 intersections
// Airspace LP has 65 navaids, 351 intersections
// Airspace LQ has 29 navaids, 65 intersections
// Airspace LR has 72 navaids, 360 intersections
// Airspace LS has 42 navaids, 204 intersections
// Airspace LT has 207 navaids, 403 intersections
// Airspace LU has 11 navaids, 49 intersections
// Airspace LW has 12 navaids, 29 intersections
// Airspace LY has 61 navaids, 172 intersections
// Airspace LZ-EAD-1 LZ-EAD-1 has 32 navaids, 138 intersections
// Airspace LZ-EAD-2 LZ-EAD-2 has 0 navaids, 0 intersections
// Airspace NZ-LNIA LOWER NORTH ISLAND ARC has 40 navaids, 434 intersections
// Airspace NZ-NOEN NORTH ENRC has 64 navaids, 885 intersections
// Airspace NZ-SOEN SOUTH ENRC has 74 navaids, 939 intersections
// Airspace NZ-UNIA UPPER NORTH ISLAND ARC has 33 navaids, 500 intersections
// Airspace NZ-USIA UPPER SOUTH ISLAND ARC has 36 navaids, 366 intersections
// Airspace NZ has 557 navaids, 4093 intersections
// Airspace OI has 180 navaids, 305 intersections
// Airspace OJ has 22 navaids, 59 intersections
// Airspace OL has 7 navaids, 30 intersections
// Airspace OS has 24 navaids, 40 intersections
// Airspace RC has 71 navaids, 97 intersections
// Airspace RP has 92 navaids, 295 intersections
// Airspace UB-EAD-1 AZERBAIJAN EAD AREA EAST has 30 navaids, 80 intersections
// Airspace UB-EAD-2 AZERBAIJAN EAD AREA WEST has 5 navaids, 3 intersections
// Airspace UD has 13 navaids, 36 intersections
// Airspace UG has 20 navaids, 35 intersections
// Airspace UK has 149 navaids, 401 intersections
// Airspace UM BELARUS has 32 navaids, 153 intersections
// Airspace UT-2 TAJIKISTAN has 9 navaids, 26 intersections
// Airspace U_ADR-1 has 187 navaids, 594 intersections
// Airspace U_ADR-2 has 2 navaids, 24 intersections
// Airspace WS SINGAPORE has 26 navaids, 164 intersections
// Airspace XX has 8 navaids, 3 intersections
