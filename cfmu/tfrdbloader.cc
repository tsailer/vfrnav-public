#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <iomanip>

#include "tfr.hh"

TrafficFlowRestrictions::DbLoader::AirspaceCache::AirspaceCache(const std::string& id, char bc, uint8_t tc)
	: m_id(id), m_airspace(), m_bdryclass(bc), m_typecode(tc)
{
}

TrafficFlowRestrictions::DbLoader::AirspaceCache::AirspaceCache(TFRAirspace::const_ptr_t aspc)
	: m_id(), m_airspace(aspc), m_bdryclass(0), m_typecode(0)
{
	if (!aspc)
		return;
	m_id = aspc->get_ident();
	m_bdryclass = aspc->get_bdryclass();
	m_typecode = aspc->get_typecode();
}

bool TrafficFlowRestrictions::DbLoader::AirspaceCache::operator<(const AirspaceCache& x) const
{
	int c(get_id().compare(x.get_id()));
	if (c < 0)
		return true;
	if (c > 0)
		return false;
	if (get_typecode() < x.get_typecode())
		return true;
	if (get_typecode() > x.get_typecode())
		return false;
	return get_bdryclass() < x.get_bdryclass();
}

const Glib::ustring& TrafficFlowRestrictions::DbLoader::AirspaceCache::get_type(void) const
{
	return AirspacesDb::Airspace::get_class_string(get_bdryclass(), get_typecode());
}

bool TrafficFlowRestrictions::DbLoader::AirportCache::operator<(const AirportCache& x) const
{
	return get_icao() < x.get_icao();
}

TrafficFlowRestrictions::DbLoader::DbLoader(TrafficFlowRestrictions& tfrs, const sigc::slot<void,Message::type_t,const std::string&,const std::string&>& msg,
					    AirportsDb& airportdb, NavaidsDb& navaiddb, WaypointsDb& waypointdb, AirwaysDb& airwaydb, AirspacesDb& airspacedb)
	: m_tfrs(tfrs), m_message(msg), m_airportdb(airportdb), m_navaiddb(navaiddb), m_waypointdb(waypointdb),
	  m_airwaydb(airwaydb), m_airspacedb(airspacedb), m_airportcache_filled(false)
{
	check_airspace_mapping();
}

TrafficFlowRestrictions::DbLoader::~DbLoader()
{
}

void TrafficFlowRestrictions::DbLoader::error(const std::string& text) const
{
	error("", text);
}

void TrafficFlowRestrictions::DbLoader::error(const std::string& child, const std::string& text) const
{
	message(Message::type_error, child, text);
}

void TrafficFlowRestrictions::DbLoader::warning(const std::string& text) const
{
	warning("", text);
}

void TrafficFlowRestrictions::DbLoader::warning(const std::string& child, const std::string& text) const
{
	message(Message::type_warning, child, text);
}

void TrafficFlowRestrictions::DbLoader::info(const std::string& text) const
{
	info("", text);
}

void TrafficFlowRestrictions::DbLoader::info(const std::string& child, const std::string& text) const
{
	message(Message::type_info, child, text);
}

TrafficFlowRestrictions::TFRAirspace::const_ptr_t TrafficFlowRestrictions::DbLoader::find_airspace(const std::string& id, const std::string& type)
{
	char bdryclass(0);
	{
		std::string t(type);
		if (t == "D")
			t = "D-EAD";
		uint8_t tc(0);
		char bc(0);
		if (AirspacesDb::Airspace::set_class_string(tc, bc, t) &&
		    tc == AirspacesDb::Airspace::typecode_ead)
			bdryclass = bc;
	}
	return find_airspace(id, bdryclass);
}

TrafficFlowRestrictions::TFRAirspace::const_ptr_t TrafficFlowRestrictions::DbLoader::find_airspace(const std::string& id, char bdryclass, uint8_t typecode)
{
	if (!bdryclass || id.empty())
		return TFRAirspace::ptr_t();
	{
		airspacecache_t::const_iterator i(m_airspacecache.find(AirspaceCache(id, bdryclass, typecode)));
		if (i != m_airspacecache.end()) {
			if (false)
				std::cerr << "find_airspace " << id << '/' << AirspacesDb::Airspace::get_class_string(bdryclass, typecode)
					  << ": cached" << std::endl;
			return i->get_airspace();
		}
	}
	{
		AirspacesDb::elementvector_t ev(m_airspacedb.find_by_icao(id, 0, AirspacesDb::comp_exact, 0, AirspacesDb::Airspace::subtables_all));
		for (AirspacesDb::elementvector_t::iterator i(ev.begin()), e(ev.end()); i != e; ++i) {
			if (false)
				std::cerr << "find_airspace " << id << '/' << AirspacesDb::Airspace::get_class_string(bdryclass, typecode)
					  << ": found " << i->get_icao() << ' ' << i->get_class_string() << (i->is_valid() ? " V" : "") << std::endl;
			if (i->get_typecode() != typecode || i->get_bdryclass() != bdryclass || !i->is_valid())
				continue;
			TFRAirspace::ptr_t aspc(new TFRAirspace(id, i->get_class_string(), i->get_polygon(), i->get_bbox(),
								i->get_bdryclass(), i->get_typecode(), i->get_altlwr(), i->get_altupr()));
			if (false && id == "LSAZESL")
				std::cerr << "Airspace: " << id << ": comps " << i->get_nrcomponents() << " segs " << i->get_nrsegments() << std::endl;
			if (i->get_nrcomponents() && !i->get_nrsegments()) {
				for (unsigned int cnr = 0; cnr < i->get_nrcomponents(); ++cnr) {
					const AirspacesDb::Airspace::Component& comp(i->get_component(cnr));
					if (!comp.is_valid())
						continue;
					TFRAirspace::const_ptr_t aspc1(find_airspace(comp.get_icao(), comp.get_bdryclass(), comp.get_typecode()));
					if (!aspc1)
						continue;
					aspc->add_component(aspc1, comp.get_operator());
				}
			}
			m_airspacecache.insert(AirspaceCache(aspc));
			return aspc;
		}
	}
	// find mapped airspace
	if (typecode == AirspacesDb::Airspace::typecode_ead) {
		const AirspaceMapping *m(find_airspace_mapping(id, bdryclass));
		if (m && m->size() >= 2) {
			if (true) {
				std::ostringstream oss;
				oss << "Airspace " << m->get_name(0) << '/'
				    << AirspacesDb::Airspace::get_class_string(m->get_bdryclass(0), AirspacesDb::Airspace::typecode_ead)
				    << " maps to";
				for (unsigned int i = 1; i < m->size(); ++i)
					oss << ' ' << m->get_name(i) << '/'
					    << AirspacesDb::Airspace::get_class_string(m->get_bdryclass(i), AirspacesDb::Airspace::typecode_ead);
				info(oss.str());
			}
			if (m->size() == 2) {
				TFRAirspace::const_ptr_t pc(find_airspace(m->get_name(1), m->get_bdryclass(1)));
				if (!pc) {
					if (true) {
						std::ostringstream oss;
						oss << "Constituent Airspace " << m->get_name(1) << '/'
						    << AirspacesDb::Airspace::get_class_string(m->get_bdryclass(1), AirspacesDb::Airspace::typecode_ead)
						    << " of " << m->get_name(0) << '/'
						    << AirspacesDb::Airspace::get_class_string(m->get_bdryclass(0), AirspacesDb::Airspace::typecode_ead)
						    << " not found";
						info(oss.str());
					}
					return pc;
				}
				if (false) {
					std::ostringstream oss;
					oss << "Airspace " << pc->get_ident() << '/' << pc->get_type() << " altitude F"
					    << std::setw(3) << std::setfill('0') << (pc->get_lower_alt() / 100) << "...F"
					    << std::setw(3) << std::setfill('0') << (pc->get_upper_alt() / 100);
					info(oss.str());
				}
				if (bdryclass == AirspacesDb::Airspace::bdryclass_ead_uta &&
				    m->get_bdryclass(1) != AirspacesDb::Airspace::bdryclass_ead_uta &&
				    pc->get_upper_alt() < 66000) {
					TFRAirspace::ptr_t p(new TFRAirspace(pc->get_ident(), pc->get_type(), pc->get_poly(), pc->get_bbox(),
									     bdryclass, pc->get_typecode(), pc->get_upper_alt(), 66000));
					m_airspacecache.insert(AirspaceCache(p));
					if (true) {
						std::ostringstream oss;
						oss << "Airspace " << p->get_ident() << '/' << p->get_type() << " altitude heuristic F"
						    << std::setw(3) << std::setfill('0') << (p->get_lower_alt() / 100) << "...F"
						    << std::setw(3) << std::setfill('0') << (p->get_upper_alt() / 100);
						info(oss.str());
					}
					return p;
				}
				m_airspacecache.insert(AirspaceCache(pc));
				return pc;
			}
			AirspacesDb::Airspace aspc;
			aspc.set_icao(m->get_name(0));
			aspc.set_typecode(AirspacesDb::Airspace::typecode_ead);
			aspc.set_bdryclass(m->get_bdryclass(0));
			for (unsigned int i = 1; i < m->size(); ++i)
				aspc.add_component(AirspacesDb::Airspace::Component(m->get_name(i), AirspacesDb::Airspace::typecode_ead,
										    m->get_bdryclass(i), (i <= 1) ? AirspacesDb::Airspace::Component::operator_set :
										    AirspacesDb::Airspace::Component::operator_union));
			aspc.recompute_lineapprox(get_airspacedb());
			aspc.recompute_bbox();
			TFRAirspace::ptr_t aspc1(new TFRAirspace(aspc.get_icao(), aspc.get_class_string(), aspc.get_polygon(), aspc.get_bbox(),
								 aspc.get_bdryclass(), aspc.get_typecode(), aspc.get_altlwr(), aspc.get_altupr()));
			for (unsigned int cnr = 0; cnr < aspc.get_nrcomponents(); ++cnr) {
				const AirspacesDb::Airspace::Component& comp(aspc.get_component(cnr));
				if (!comp.is_valid())
					continue;
				TFRAirspace::const_ptr_t aspc2(find_airspace(comp.get_icao(), comp.get_bdryclass(), comp.get_typecode()));
				if (!aspc2) {
					if (true) {
						std::ostringstream oss;
						oss << "Constituent Airspace " << comp.get_icao() << '/'
						    << AirspacesDb::Airspace::get_class_string(comp.get_bdryclass(), comp.get_typecode())
						    << " of " << m->get_name(0) << '/'
						    << AirspacesDb::Airspace::get_class_string(m->get_bdryclass(0), AirspacesDb::Airspace::typecode_ead)
						    << " not found";
						info(oss.str());
					}
					continue;
				}
				aspc1->add_component(aspc2, comp.get_operator());
			}
			m_airspacecache.insert(AirspaceCache(aspc1));
			return aspc1;
		}
	}
	if (false)
		std::cerr << "find_airspace " << id << '/' << AirspacesDb::Airspace::get_class_string(bdryclass, typecode)
			  << ": no airspace found" << std::endl;
	// cache negative answers
	m_airspacecache.insert(AirspaceCache(id, bdryclass, typecode));
	return TFRAirspace::ptr_t();
}

const AirportsDb::Airport& TrafficFlowRestrictions::DbLoader::find_airport(const std::string& icao)
{
	{
		AirportCache ac;
		ac.set_icao(icao);
		airportcache_t::const_iterator i(m_airportcache.find(AirportCache(ac)));
		if (i != m_airportcache.end())
			return *i;
	}
	AirportsDb::elementvector_t ev;
	if (!m_airportcache_filled)
		ev = get_airportdb().find_by_icao(icao, 0, AirportsDb::comp_exact, 2, AirportsDb::element_t::subtables_none);
	if (ev.size() == 1) {
		std::pair<airportcache_t::iterator,bool> i(m_airportcache.insert(AirportCache(ev[0])));
		return *i.first;
	}
	{
		// cache negative answers
		AirportsDb::Airport arpt;
		arpt.set_icao(icao);
		std::pair<airportcache_t::iterator,bool> i(m_airportcache.insert(AirportCache(arpt)));
		return *i.first;
	}
}

void TrafficFlowRestrictions::DbLoader::fill_airport_cache(void)
{
	if (m_airportcache_filled)
		return;
	AirportsDb::Airport arpt;
	get_airportdb().loadfirst(arpt, true, AirportsDb::element_t::subtables_none);
	while (arpt.is_valid()) {
		m_airportcache.insert(AirportCache(arpt));
		get_airportdb().loadnext(arpt, true, AirportsDb::element_t::subtables_none);
	}
	m_airportcache_filled = true;
	if (true) {
		std::ostringstream oss;
		oss << m_airportcache.size() << " airports cached";
		info(oss.str());
	}
}

TrafficFlowRestrictions::DbLoader::AirspaceMapping::AirspaceMapping(const char *name, char bc, const char *name0, char bc0,
								    const char *name1, char bc1, const char *name2, char bc2)
{
	m_names[0] = name;
	m_bdryclass[0] = bc;
	m_names[1] = name0;
	m_bdryclass[1] = bc0;
	m_names[2] = name1;
	m_bdryclass[2] = bc1;
	m_names[3] = name2;
	m_bdryclass[3] = bc2;
}

unsigned int TrafficFlowRestrictions::DbLoader::AirspaceMapping::size(void) const
{
	unsigned int i(0);
	for (; i < 4; ++i)
		if (!m_names[i] || !m_bdryclass[i])
			break;
	return i;
}

bool TrafficFlowRestrictions::DbLoader::AirspaceMapping::operator<(const AirspaceMapping& x) const
{
	if (!m_names[0] && x.m_names[0])
		return true;
	if (m_names[0] && !x.m_names[0])
		return true;
	if (m_names[0] && x.m_names[0]) {
		int c(strcmp(m_names[0], x.m_names[0]));
		if (c)
			return c < 0;
	}
	return m_bdryclass[0] < x.m_bdryclass[0];
}

std::string TrafficFlowRestrictions::DbLoader::AirspaceMapping::get_name(unsigned int i) const
{
	if (i >= 4)
		return "";
	return m_names[i];
}

char TrafficFlowRestrictions::DbLoader::AirspaceMapping::get_bdryclass(unsigned int i) const
{
	if (i >= 4)
		return 0;
	return m_bdryclass[i];
}

const TrafficFlowRestrictions::DbLoader::AirspaceMapping TrafficFlowRestrictions::DbLoader::airspacemapping[] = {
	TrafficFlowRestrictions::DbLoader::AirspaceMapping("DAAA",      AirspacesDb::Airspace::bdryclass_ead_cta,   "DAAA",      AirspacesDb::Airspace::bdryclass_ead_fir),
	TrafficFlowRestrictions::DbLoader::AirspaceMapping("DTTC",      AirspacesDb::Airspace::bdryclass_ead_cta,   "DTTC",      AirspacesDb::Airspace::bdryclass_ead_fir),
	TrafficFlowRestrictions::DbLoader::AirspaceMapping("EDMM",      AirspacesDb::Airspace::bdryclass_ead_cta,   "EDMM",      AirspacesDb::Airspace::bdryclass_ead_fir),
	TrafficFlowRestrictions::DbLoader::AirspaceMapping("EDUU",      AirspacesDb::Airspace::bdryclass_ead_uta,   "EDUU",      AirspacesDb::Airspace::bdryclass_ead_uir),
	TrafficFlowRestrictions::DbLoader::AirspaceMapping("EDWW",      AirspacesDb::Airspace::bdryclass_ead_cta,   "EDWW",      AirspacesDb::Airspace::bdryclass_ead_fir),
	TrafficFlowRestrictions::DbLoader::AirspaceMapping("EDYY",      AirspacesDb::Airspace::bdryclass_ead_ras,   "EDYY",      AirspacesDb::Airspace::bdryclass_ead_uir),
	TrafficFlowRestrictions::DbLoader::AirspaceMapping("EFD44",     AirspacesDb::Airspace::bdryclass_ead_d_amc, "EFD44",     AirspacesDb::Airspace::bdryclass_ead_d),
	TrafficFlowRestrictions::DbLoader::AirspaceMapping("EFD46",     AirspacesDb::Airspace::bdryclass_ead_d_amc, "EFD46",     AirspacesDb::Airspace::bdryclass_ead_d),
	TrafficFlowRestrictions::DbLoader::AirspaceMapping("EFD50",     AirspacesDb::Airspace::bdryclass_ead_d_amc, "EFD50",     AirspacesDb::Airspace::bdryclass_ead_d),
	TrafficFlowRestrictions::DbLoader::AirspaceMapping("EFD51",     AirspacesDb::Airspace::bdryclass_ead_d_amc, "EFD51",     AirspacesDb::Airspace::bdryclass_ead_d),
	TrafficFlowRestrictions::DbLoader::AirspaceMapping("EFD52",     AirspacesDb::Airspace::bdryclass_ead_d_amc, "EFD52",     AirspacesDb::Airspace::bdryclass_ead_d),
	TrafficFlowRestrictions::DbLoader::AirspaceMapping("EFD54",     AirspacesDb::Airspace::bdryclass_ead_d_amc, "EFD54",     AirspacesDb::Airspace::bdryclass_ead_d),
	TrafficFlowRestrictions::DbLoader::AirspaceMapping("EFKKCTR",   AirspacesDb::Airspace::bdryclass_ead_ctr,   "EFKK",      AirspacesDb::Airspace::bdryclass_ead_ctr),
	TrafficFlowRestrictions::DbLoader::AirspaceMapping("EFPOCTR",   AirspacesDb::Airspace::bdryclass_ead_ctr,   "EFPO",      AirspacesDb::Airspace::bdryclass_ead_ctr),
	TrafficFlowRestrictions::DbLoader::AirspaceMapping("EFTRA900L", AirspacesDb::Airspace::bdryclass_ead_tra,   "EFT900",    AirspacesDb::Airspace::bdryclass_ead_tra),
	TrafficFlowRestrictions::DbLoader::AirspaceMapping("EFTRA900U", AirspacesDb::Airspace::bdryclass_ead_tra,   "EFT900",    AirspacesDb::Airspace::bdryclass_ead_tra),
	TrafficFlowRestrictions::DbLoader::AirspaceMapping("EFTRA901",  AirspacesDb::Airspace::bdryclass_ead_tra,   "EFT901",    AirspacesDb::Airspace::bdryclass_ead_tra),
	TrafficFlowRestrictions::DbLoader::AirspaceMapping("EFTRA902L", AirspacesDb::Airspace::bdryclass_ead_tra,   "EFT902",    AirspacesDb::Airspace::bdryclass_ead_tra),
	TrafficFlowRestrictions::DbLoader::AirspaceMapping("EFTRA902U", AirspacesDb::Airspace::bdryclass_ead_tra,   "EFT902",    AirspacesDb::Airspace::bdryclass_ead_tra),
	TrafficFlowRestrictions::DbLoader::AirspaceMapping("EFTRA903L", AirspacesDb::Airspace::bdryclass_ead_tra,   "EFT903",    AirspacesDb::Airspace::bdryclass_ead_tra),
	TrafficFlowRestrictions::DbLoader::AirspaceMapping("EFTRA903U", AirspacesDb::Airspace::bdryclass_ead_tra,   "EFT903",    AirspacesDb::Airspace::bdryclass_ead_tra),
	TrafficFlowRestrictions::DbLoader::AirspaceMapping("EFTRA904L", AirspacesDb::Airspace::bdryclass_ead_tra,   "EFT904",    AirspacesDb::Airspace::bdryclass_ead_tra),
	TrafficFlowRestrictions::DbLoader::AirspaceMapping("EFTRA904U", AirspacesDb::Airspace::bdryclass_ead_tra,   "EFT904",    AirspacesDb::Airspace::bdryclass_ead_tra),
	TrafficFlowRestrictions::DbLoader::AirspaceMapping("EFTRA905",  AirspacesDb::Airspace::bdryclass_ead_tra,   "EFT905",    AirspacesDb::Airspace::bdryclass_ead_tra),
	TrafficFlowRestrictions::DbLoader::AirspaceMapping("EFTRA906",  AirspacesDb::Airspace::bdryclass_ead_tra,   "EFT906",    AirspacesDb::Airspace::bdryclass_ead_tra),
	TrafficFlowRestrictions::DbLoader::AirspaceMapping("EFTRA907",  AirspacesDb::Airspace::bdryclass_ead_tra,   "EFT907",    AirspacesDb::Airspace::bdryclass_ead_tra),
	TrafficFlowRestrictions::DbLoader::AirspaceMapping("EFTRA908",  AirspacesDb::Airspace::bdryclass_ead_tra,   "EFT908",    AirspacesDb::Airspace::bdryclass_ead_tra),
	TrafficFlowRestrictions::DbLoader::AirspaceMapping("EFTRA909",  AirspacesDb::Airspace::bdryclass_ead_tra,   "EFT909",    AirspacesDb::Airspace::bdryclass_ead_tra),
	TrafficFlowRestrictions::DbLoader::AirspaceMapping("EFTRA910",  AirspacesDb::Airspace::bdryclass_ead_tra,   "EFT910",    AirspacesDb::Airspace::bdryclass_ead_tra),
	TrafficFlowRestrictions::DbLoader::AirspaceMapping("EFTRA911",  AirspacesDb::Airspace::bdryclass_ead_tra,   "EFT911",    AirspacesDb::Airspace::bdryclass_ead_tra),
	TrafficFlowRestrictions::DbLoader::AirspaceMapping("EFTRA912",  AirspacesDb::Airspace::bdryclass_ead_tra,   "EFT912",    AirspacesDb::Airspace::bdryclass_ead_tra),
	TrafficFlowRestrictions::DbLoader::AirspaceMapping("EFTRA913",  AirspacesDb::Airspace::bdryclass_ead_tra,   "EFT913",    AirspacesDb::Airspace::bdryclass_ead_tra),
	TrafficFlowRestrictions::DbLoader::AirspaceMapping("EFTRA914",  AirspacesDb::Airspace::bdryclass_ead_tra,   "EFT914",    AirspacesDb::Airspace::bdryclass_ead_tra),
	TrafficFlowRestrictions::DbLoader::AirspaceMapping("EFTRA915",  AirspacesDb::Airspace::bdryclass_ead_tra,   "EFT915",    AirspacesDb::Airspace::bdryclass_ead_tra),
	TrafficFlowRestrictions::DbLoader::AirspaceMapping("EFTRA916",  AirspacesDb::Airspace::bdryclass_ead_tra,   "EFT916",    AirspacesDb::Airspace::bdryclass_ead_tra),
	TrafficFlowRestrictions::DbLoader::AirspaceMapping("EFTSA106",  AirspacesDb::Airspace::bdryclass_ead_tsa,   "EFT106",    AirspacesDb::Airspace::bdryclass_ead_tsa),
	TrafficFlowRestrictions::DbLoader::AirspaceMapping("EFTSA201",  AirspacesDb::Airspace::bdryclass_ead_tsa,   "EFT201",    AirspacesDb::Airspace::bdryclass_ead_tsa),
	TrafficFlowRestrictions::DbLoader::AirspaceMapping("EFTSA202",  AirspacesDb::Airspace::bdryclass_ead_tsa,   "EFT202",    AirspacesDb::Airspace::bdryclass_ead_tsa),
	TrafficFlowRestrictions::DbLoader::AirspaceMapping("EFTSA203",  AirspacesDb::Airspace::bdryclass_ead_tsa,   "EFT203",    AirspacesDb::Airspace::bdryclass_ead_tsa),
	TrafficFlowRestrictions::DbLoader::AirspaceMapping("EFTSA204",  AirspacesDb::Airspace::bdryclass_ead_tsa,   "EFT204",    AirspacesDb::Airspace::bdryclass_ead_tsa),
	TrafficFlowRestrictions::DbLoader::AirspaceMapping("EFTSA300",  AirspacesDb::Airspace::bdryclass_ead_tsa,   "EFT300",    AirspacesDb::Airspace::bdryclass_ead_tsa),
	TrafficFlowRestrictions::DbLoader::AirspaceMapping("EFTSA301",  AirspacesDb::Airspace::bdryclass_ead_tsa,   "EFT301",    AirspacesDb::Airspace::bdryclass_ead_tsa),
	TrafficFlowRestrictions::DbLoader::AirspaceMapping("EFTSA302",  AirspacesDb::Airspace::bdryclass_ead_tsa,   "EFT302",    AirspacesDb::Airspace::bdryclass_ead_tsa),
	TrafficFlowRestrictions::DbLoader::AirspaceMapping("EFTSA303",  AirspacesDb::Airspace::bdryclass_ead_tsa,   "EFT303",    AirspacesDb::Airspace::bdryclass_ead_tsa),
	TrafficFlowRestrictions::DbLoader::AirspaceMapping("EFTSA307",  AirspacesDb::Airspace::bdryclass_ead_tsa,   "EFT307",    AirspacesDb::Airspace::bdryclass_ead_tsa),
	TrafficFlowRestrictions::DbLoader::AirspaceMapping("EFTSA308",  AirspacesDb::Airspace::bdryclass_ead_tsa,   "EFT308",    AirspacesDb::Airspace::bdryclass_ead_tsa),
	TrafficFlowRestrictions::DbLoader::AirspaceMapping("EFTSA309",  AirspacesDb::Airspace::bdryclass_ead_tsa,   "EFT309",    AirspacesDb::Airspace::bdryclass_ead_tsa),
	TrafficFlowRestrictions::DbLoader::AirspaceMapping("EFTSA310",  AirspacesDb::Airspace::bdryclass_ead_tsa,   "EFT310",    AirspacesDb::Airspace::bdryclass_ead_tsa),
	TrafficFlowRestrictions::DbLoader::AirspaceMapping("EFTSA312",  AirspacesDb::Airspace::bdryclass_ead_tsa,   "EFT312",    AirspacesDb::Airspace::bdryclass_ead_tsa),
	TrafficFlowRestrictions::DbLoader::AirspaceMapping("EFTSA313",  AirspacesDb::Airspace::bdryclass_ead_tsa,   "EFT313",    AirspacesDb::Airspace::bdryclass_ead_tsa),
	TrafficFlowRestrictions::DbLoader::AirspaceMapping("EFTSA317",  AirspacesDb::Airspace::bdryclass_ead_tsa,   "EFT317",    AirspacesDb::Airspace::bdryclass_ead_tsa),
	TrafficFlowRestrictions::DbLoader::AirspaceMapping("EFTSA318",  AirspacesDb::Airspace::bdryclass_ead_tsa,   "EFT318",    AirspacesDb::Airspace::bdryclass_ead_tsa),
	TrafficFlowRestrictions::DbLoader::AirspaceMapping("EFTSA319",  AirspacesDb::Airspace::bdryclass_ead_tsa,   "EFT319",    AirspacesDb::Airspace::bdryclass_ead_tsa),
	TrafficFlowRestrictions::DbLoader::AirspaceMapping("EFTSA320",  AirspacesDb::Airspace::bdryclass_ead_tsa,   "EFT320",    AirspacesDb::Airspace::bdryclass_ead_tsa),
	TrafficFlowRestrictions::DbLoader::AirspaceMapping("EFTSA321",  AirspacesDb::Airspace::bdryclass_ead_tsa,   "EFT321",    AirspacesDb::Airspace::bdryclass_ead_tsa),
	TrafficFlowRestrictions::DbLoader::AirspaceMapping("EFTSA323",  AirspacesDb::Airspace::bdryclass_ead_tsa,   "EFT323",    AirspacesDb::Airspace::bdryclass_ead_tsa),
	TrafficFlowRestrictions::DbLoader::AirspaceMapping("EFTSA324",  AirspacesDb::Airspace::bdryclass_ead_tsa,   "EFT324",    AirspacesDb::Airspace::bdryclass_ead_tsa),
	TrafficFlowRestrictions::DbLoader::AirspaceMapping("EFTSA329",  AirspacesDb::Airspace::bdryclass_ead_tsa,   "EFT329",    AirspacesDb::Airspace::bdryclass_ead_tsa),
	TrafficFlowRestrictions::DbLoader::AirspaceMapping("EFTSA331",  AirspacesDb::Airspace::bdryclass_ead_tsa,   "EFT331",    AirspacesDb::Airspace::bdryclass_ead_tsa),
	TrafficFlowRestrictions::DbLoader::AirspaceMapping("EFTSA332",  AirspacesDb::Airspace::bdryclass_ead_tsa,   "EFT332",    AirspacesDb::Airspace::bdryclass_ead_tsa),
	TrafficFlowRestrictions::DbLoader::AirspaceMapping("EFTSA401",  AirspacesDb::Airspace::bdryclass_ead_tsa,   "EFT401",    AirspacesDb::Airspace::bdryclass_ead_tsa),
	TrafficFlowRestrictions::DbLoader::AirspaceMapping("EFTSA402",  AirspacesDb::Airspace::bdryclass_ead_tsa,   "EFT402",    AirspacesDb::Airspace::bdryclass_ead_tsa),
	TrafficFlowRestrictions::DbLoader::AirspaceMapping("EFTSA403",  AirspacesDb::Airspace::bdryclass_ead_tsa,   "EFT403",    AirspacesDb::Airspace::bdryclass_ead_tsa),
	TrafficFlowRestrictions::DbLoader::AirspaceMapping("EFTSA404",  AirspacesDb::Airspace::bdryclass_ead_tsa,   "EFT404",    AirspacesDb::Airspace::bdryclass_ead_tsa),
	TrafficFlowRestrictions::DbLoader::AirspaceMapping("EFTSA405",  AirspacesDb::Airspace::bdryclass_ead_tsa,   "EFT405",    AirspacesDb::Airspace::bdryclass_ead_tsa),
	TrafficFlowRestrictions::DbLoader::AirspaceMapping("EFTSA406",  AirspacesDb::Airspace::bdryclass_ead_tsa,   "EFT406",    AirspacesDb::Airspace::bdryclass_ead_tsa),
	TrafficFlowRestrictions::DbLoader::AirspaceMapping("EFTSA407",  AirspacesDb::Airspace::bdryclass_ead_tsa,   "EFT407",    AirspacesDb::Airspace::bdryclass_ead_tsa),
	TrafficFlowRestrictions::DbLoader::AirspaceMapping("EFTSA408",  AirspacesDb::Airspace::bdryclass_ead_tsa,   "EFT408",    AirspacesDb::Airspace::bdryclass_ead_tsa),
	TrafficFlowRestrictions::DbLoader::AirspaceMapping("EFTSA409",  AirspacesDb::Airspace::bdryclass_ead_tsa,   "EFT409",    AirspacesDb::Airspace::bdryclass_ead_tsa),
	TrafficFlowRestrictions::DbLoader::AirspaceMapping("EFTSA410",  AirspacesDb::Airspace::bdryclass_ead_tsa,   "EFT410",    AirspacesDb::Airspace::bdryclass_ead_tsa),
	TrafficFlowRestrictions::DbLoader::AirspaceMapping("EFTSA411",  AirspacesDb::Airspace::bdryclass_ead_tsa,   "EFT411",    AirspacesDb::Airspace::bdryclass_ead_tsa),
	TrafficFlowRestrictions::DbLoader::AirspaceMapping("EFTSA412",  AirspacesDb::Airspace::bdryclass_ead_tsa,   "EFT412",    AirspacesDb::Airspace::bdryclass_ead_tsa),
	TrafficFlowRestrictions::DbLoader::AirspaceMapping("EFTSA413",  AirspacesDb::Airspace::bdryclass_ead_tsa,   "EFT413",    AirspacesDb::Airspace::bdryclass_ead_tsa),
	TrafficFlowRestrictions::DbLoader::AirspaceMapping("EFTSA414",  AirspacesDb::Airspace::bdryclass_ead_tsa,   "EFT414",    AirspacesDb::Airspace::bdryclass_ead_tsa),
	TrafficFlowRestrictions::DbLoader::AirspaceMapping("EFTSA415",  AirspacesDb::Airspace::bdryclass_ead_tsa,   "EFT415",    AirspacesDb::Airspace::bdryclass_ead_tsa),
	TrafficFlowRestrictions::DbLoader::AirspaceMapping("EFTSA416",  AirspacesDb::Airspace::bdryclass_ead_tsa,   "EFT416",    AirspacesDb::Airspace::bdryclass_ead_tsa),
	TrafficFlowRestrictions::DbLoader::AirspaceMapping("EFTSA417",  AirspacesDb::Airspace::bdryclass_ead_tsa,   "EFT417",    AirspacesDb::Airspace::bdryclass_ead_tsa),
	TrafficFlowRestrictions::DbLoader::AirspaceMapping("EFTSA418",  AirspacesDb::Airspace::bdryclass_ead_tsa,   "EFT418",    AirspacesDb::Airspace::bdryclass_ead_tsa),
	TrafficFlowRestrictions::DbLoader::AirspaceMapping("EFTSA419",  AirspacesDb::Airspace::bdryclass_ead_tsa,   "EFT419",    AirspacesDb::Airspace::bdryclass_ead_tsa),
	TrafficFlowRestrictions::DbLoader::AirspaceMapping("EFTSA420",  AirspacesDb::Airspace::bdryclass_ead_tsa,   "EFT420",    AirspacesDb::Airspace::bdryclass_ead_tsa),
	TrafficFlowRestrictions::DbLoader::AirspaceMapping("EFTSA421",  AirspacesDb::Airspace::bdryclass_ead_tsa,   "EFT421",    AirspacesDb::Airspace::bdryclass_ead_tsa),
	TrafficFlowRestrictions::DbLoader::AirspaceMapping("EFTSA422",  AirspacesDb::Airspace::bdryclass_ead_tsa,   "EFT422",    AirspacesDb::Airspace::bdryclass_ead_tsa),
	TrafficFlowRestrictions::DbLoader::AirspaceMapping("EFTSA423",  AirspacesDb::Airspace::bdryclass_ead_tsa,   "EFT423",    AirspacesDb::Airspace::bdryclass_ead_tsa),
	TrafficFlowRestrictions::DbLoader::AirspaceMapping("EFTSA424",  AirspacesDb::Airspace::bdryclass_ead_tsa,   "EFT424",    AirspacesDb::Airspace::bdryclass_ead_tsa),
	TrafficFlowRestrictions::DbLoader::AirspaceMapping("EFTSA425",  AirspacesDb::Airspace::bdryclass_ead_tsa,   "EFT425",    AirspacesDb::Airspace::bdryclass_ead_tsa),
	TrafficFlowRestrictions::DbLoader::AirspaceMapping("EFTSA426",  AirspacesDb::Airspace::bdryclass_ead_tsa,   "EFT426",    AirspacesDb::Airspace::bdryclass_ead_tsa),
	TrafficFlowRestrictions::DbLoader::AirspaceMapping("EFTSA427",  AirspacesDb::Airspace::bdryclass_ead_tsa,   "EFT427",    AirspacesDb::Airspace::bdryclass_ead_tsa),
	TrafficFlowRestrictions::DbLoader::AirspaceMapping("EFTSA428",  AirspacesDb::Airspace::bdryclass_ead_tsa,   "EFT428",    AirspacesDb::Airspace::bdryclass_ead_tsa),
	TrafficFlowRestrictions::DbLoader::AirspaceMapping("EFTSA429",  AirspacesDb::Airspace::bdryclass_ead_tsa,   "EFT429",    AirspacesDb::Airspace::bdryclass_ead_tsa),
	TrafficFlowRestrictions::DbLoader::AirspaceMapping("EFTSA601",  AirspacesDb::Airspace::bdryclass_ead_tsa,   "EFT601",    AirspacesDb::Airspace::bdryclass_ead_tsa),
	TrafficFlowRestrictions::DbLoader::AirspaceMapping("EFTSA602",  AirspacesDb::Airspace::bdryclass_ead_tsa,   "EFT602",    AirspacesDb::Airspace::bdryclass_ead_tsa),
	TrafficFlowRestrictions::DbLoader::AirspaceMapping("EFTSA603",  AirspacesDb::Airspace::bdryclass_ead_tsa,   "EFT603",    AirspacesDb::Airspace::bdryclass_ead_tsa),
	TrafficFlowRestrictions::DbLoader::AirspaceMapping("EFTSA604",  AirspacesDb::Airspace::bdryclass_ead_tsa,   "EFT604",    AirspacesDb::Airspace::bdryclass_ead_tsa),
	TrafficFlowRestrictions::DbLoader::AirspaceMapping("EFTSA609",  AirspacesDb::Airspace::bdryclass_ead_tsa,   "EFT609",    AirspacesDb::Airspace::bdryclass_ead_tsa),
	TrafficFlowRestrictions::DbLoader::AirspaceMapping("EFTSA610",  AirspacesDb::Airspace::bdryclass_ead_tsa,   "EFT610",    AirspacesDb::Airspace::bdryclass_ead_tsa),
	TrafficFlowRestrictions::DbLoader::AirspaceMapping("EFTSA611",  AirspacesDb::Airspace::bdryclass_ead_tsa,   "EFT611",    AirspacesDb::Airspace::bdryclass_ead_tsa),
	TrafficFlowRestrictions::DbLoader::AirspaceMapping("EFTSA612",  AirspacesDb::Airspace::bdryclass_ead_tsa,   "EFT612",    AirspacesDb::Airspace::bdryclass_ead_tsa),
	TrafficFlowRestrictions::DbLoader::AirspaceMapping("EFTSA613",  AirspacesDb::Airspace::bdryclass_ead_tsa,   "EFT613",    AirspacesDb::Airspace::bdryclass_ead_tsa),
	TrafficFlowRestrictions::DbLoader::AirspaceMapping("EFTSA614",  AirspacesDb::Airspace::bdryclass_ead_tsa,   "EFT614",    AirspacesDb::Airspace::bdryclass_ead_tsa),
	TrafficFlowRestrictions::DbLoader::AirspaceMapping("EFTSA615",  AirspacesDb::Airspace::bdryclass_ead_tsa,   "EFT615",    AirspacesDb::Airspace::bdryclass_ead_tsa),
	TrafficFlowRestrictions::DbLoader::AirspaceMapping("EFTSA616",  AirspacesDb::Airspace::bdryclass_ead_tsa,   "EFT616",    AirspacesDb::Airspace::bdryclass_ead_tsa),
	TrafficFlowRestrictions::DbLoader::AirspaceMapping("EFTSA617",  AirspacesDb::Airspace::bdryclass_ead_tsa,   "EFT617",    AirspacesDb::Airspace::bdryclass_ead_tsa),
	TrafficFlowRestrictions::DbLoader::AirspaceMapping("EFTSA619",  AirspacesDb::Airspace::bdryclass_ead_tsa,   "EFT619",    AirspacesDb::Airspace::bdryclass_ead_tsa),
	TrafficFlowRestrictions::DbLoader::AirspaceMapping("EFTSA620",  AirspacesDb::Airspace::bdryclass_ead_tsa,   "EFT620",    AirspacesDb::Airspace::bdryclass_ead_tsa),
	TrafficFlowRestrictions::DbLoader::AirspaceMapping("EFTSA621",  AirspacesDb::Airspace::bdryclass_ead_tsa,   "EFT621",    AirspacesDb::Airspace::bdryclass_ead_tsa),
	TrafficFlowRestrictions::DbLoader::AirspaceMapping("EFTSA622",  AirspacesDb::Airspace::bdryclass_ead_tsa,   "EFT622",    AirspacesDb::Airspace::bdryclass_ead_tsa),
	TrafficFlowRestrictions::DbLoader::AirspaceMapping("EFTSA701",  AirspacesDb::Airspace::bdryclass_ead_tsa,   "EFT701",    AirspacesDb::Airspace::bdryclass_ead_tsa),
	TrafficFlowRestrictions::DbLoader::AirspaceMapping("EFTSA702",  AirspacesDb::Airspace::bdryclass_ead_tsa,   "EFT702",    AirspacesDb::Airspace::bdryclass_ead_tsa),
	TrafficFlowRestrictions::DbLoader::AirspaceMapping("EFTSA703",  AirspacesDb::Airspace::bdryclass_ead_tsa,   "EFT703",    AirspacesDb::Airspace::bdryclass_ead_tsa),
	TrafficFlowRestrictions::DbLoader::AirspaceMapping("EFTSA708",  AirspacesDb::Airspace::bdryclass_ead_tsa,   "EFT708",    AirspacesDb::Airspace::bdryclass_ead_tsa),
	TrafficFlowRestrictions::DbLoader::AirspaceMapping("EFTSA709",  AirspacesDb::Airspace::bdryclass_ead_tsa,   "EFT709",    AirspacesDb::Airspace::bdryclass_ead_tsa),
	TrafficFlowRestrictions::DbLoader::AirspaceMapping("EFTSA710",  AirspacesDb::Airspace::bdryclass_ead_tsa,   "EFT710",    AirspacesDb::Airspace::bdryclass_ead_tsa),
	TrafficFlowRestrictions::DbLoader::AirspaceMapping("EFTSA712",  AirspacesDb::Airspace::bdryclass_ead_tsa,   "EFT712",    AirspacesDb::Airspace::bdryclass_ead_tsa),
	TrafficFlowRestrictions::DbLoader::AirspaceMapping("EFTSA713",  AirspacesDb::Airspace::bdryclass_ead_tsa,   "EFT713",    AirspacesDb::Airspace::bdryclass_ead_tsa),
	TrafficFlowRestrictions::DbLoader::AirspaceMapping("EFTSA714",  AirspacesDb::Airspace::bdryclass_ead_tsa,   "EFT714",    AirspacesDb::Airspace::bdryclass_ead_tsa),
	TrafficFlowRestrictions::DbLoader::AirspaceMapping("EFTSA715",  AirspacesDb::Airspace::bdryclass_ead_tsa,   "EFT715",    AirspacesDb::Airspace::bdryclass_ead_tsa),
	TrafficFlowRestrictions::DbLoader::AirspaceMapping("EFTSA719",  AirspacesDb::Airspace::bdryclass_ead_tsa,   "EFT719",    AirspacesDb::Airspace::bdryclass_ead_tsa),
	TrafficFlowRestrictions::DbLoader::AirspaceMapping("EFTSA720",  AirspacesDb::Airspace::bdryclass_ead_tsa,   "EFT720",    AirspacesDb::Airspace::bdryclass_ead_tsa),
	TrafficFlowRestrictions::DbLoader::AirspaceMapping("EFTSA721",  AirspacesDb::Airspace::bdryclass_ead_tsa,   "EFT721",    AirspacesDb::Airspace::bdryclass_ead_tsa),
	TrafficFlowRestrictions::DbLoader::AirspaceMapping("EFTSA722",  AirspacesDb::Airspace::bdryclass_ead_tsa,   "EFT722",    AirspacesDb::Airspace::bdryclass_ead_tsa),
	TrafficFlowRestrictions::DbLoader::AirspaceMapping("EFTSA723",  AirspacesDb::Airspace::bdryclass_ead_tsa,   "EFT723",    AirspacesDb::Airspace::bdryclass_ead_tsa),
	TrafficFlowRestrictions::DbLoader::AirspaceMapping("EFTSA724",  AirspacesDb::Airspace::bdryclass_ead_tsa,   "EFT724",    AirspacesDb::Airspace::bdryclass_ead_tsa),
	TrafficFlowRestrictions::DbLoader::AirspaceMapping("EFTSA725",  AirspacesDb::Airspace::bdryclass_ead_tsa,   "EFT725",    AirspacesDb::Airspace::bdryclass_ead_tsa),
	TrafficFlowRestrictions::DbLoader::AirspaceMapping("EFTSA726",  AirspacesDb::Airspace::bdryclass_ead_tsa,   "EFT726",    AirspacesDb::Airspace::bdryclass_ead_tsa),
	TrafficFlowRestrictions::DbLoader::AirspaceMapping("EFTSA727",  AirspacesDb::Airspace::bdryclass_ead_tsa,   "EFT727",    AirspacesDb::Airspace::bdryclass_ead_tsa),
	TrafficFlowRestrictions::DbLoader::AirspaceMapping("EFTSA728",  AirspacesDb::Airspace::bdryclass_ead_tsa,   "EFT728",    AirspacesDb::Airspace::bdryclass_ead_tsa),
	TrafficFlowRestrictions::DbLoader::AirspaceMapping("EFTSA729",  AirspacesDb::Airspace::bdryclass_ead_tsa,   "EFT729",    AirspacesDb::Airspace::bdryclass_ead_tsa),
	TrafficFlowRestrictions::DbLoader::AirspaceMapping("EFTSA730",  AirspacesDb::Airspace::bdryclass_ead_tsa,   "EFT730",    AirspacesDb::Airspace::bdryclass_ead_tsa),
	TrafficFlowRestrictions::DbLoader::AirspaceMapping("EGD003",    AirspacesDb::Airspace::bdryclass_ead_d_amc, "EGD003",    AirspacesDb::Airspace::bdryclass_ead_d),
	TrafficFlowRestrictions::DbLoader::AirspaceMapping("EGD004",    AirspacesDb::Airspace::bdryclass_ead_d_amc, "EGD004",    AirspacesDb::Airspace::bdryclass_ead_d),
	TrafficFlowRestrictions::DbLoader::AirspaceMapping("EGD006A",   AirspacesDb::Airspace::bdryclass_ead_d_amc, "EGD006A",   AirspacesDb::Airspace::bdryclass_ead_d),
	TrafficFlowRestrictions::DbLoader::AirspaceMapping("EGD007",    AirspacesDb::Airspace::bdryclass_ead_d_amc, "EGD007",    AirspacesDb::Airspace::bdryclass_ead_d),
	TrafficFlowRestrictions::DbLoader::AirspaceMapping("EGD007A",   AirspacesDb::Airspace::bdryclass_ead_d_amc, "EGD007A",   AirspacesDb::Airspace::bdryclass_ead_d),
	TrafficFlowRestrictions::DbLoader::AirspaceMapping("EGD007B",   AirspacesDb::Airspace::bdryclass_ead_d_amc, "EGD007B",   AirspacesDb::Airspace::bdryclass_ead_d),
	TrafficFlowRestrictions::DbLoader::AirspaceMapping("EGD008",    AirspacesDb::Airspace::bdryclass_ead_d_amc, "EGD008",    AirspacesDb::Airspace::bdryclass_ead_d),
	TrafficFlowRestrictions::DbLoader::AirspaceMapping("EGD008A",   AirspacesDb::Airspace::bdryclass_ead_d_amc, "EGD008A",   AirspacesDb::Airspace::bdryclass_ead_d),
	TrafficFlowRestrictions::DbLoader::AirspaceMapping("EGD008B",   AirspacesDb::Airspace::bdryclass_ead_d_amc, "EGD008B",   AirspacesDb::Airspace::bdryclass_ead_d),
	TrafficFlowRestrictions::DbLoader::AirspaceMapping("EGD009",    AirspacesDb::Airspace::bdryclass_ead_d_amc, "EGD009",    AirspacesDb::Airspace::bdryclass_ead_d),
	TrafficFlowRestrictions::DbLoader::AirspaceMapping("EGD009A",   AirspacesDb::Airspace::bdryclass_ead_d_amc, "EGD009A",   AirspacesDb::Airspace::bdryclass_ead_d),
	TrafficFlowRestrictions::DbLoader::AirspaceMapping("EGD011A",   AirspacesDb::Airspace::bdryclass_ead_d_amc, "EGD011A",   AirspacesDb::Airspace::bdryclass_ead_d),
	TrafficFlowRestrictions::DbLoader::AirspaceMapping("EGD011B",   AirspacesDb::Airspace::bdryclass_ead_d_amc, "EGD011B",   AirspacesDb::Airspace::bdryclass_ead_d),
	TrafficFlowRestrictions::DbLoader::AirspaceMapping("EGD011C",   AirspacesDb::Airspace::bdryclass_ead_d_amc, "EGD011C",   AirspacesDb::Airspace::bdryclass_ead_d),
	TrafficFlowRestrictions::DbLoader::AirspaceMapping("EGD012",    AirspacesDb::Airspace::bdryclass_ead_d_amc, "EGD012",    AirspacesDb::Airspace::bdryclass_ead_d),
	TrafficFlowRestrictions::DbLoader::AirspaceMapping("EGD013",    AirspacesDb::Airspace::bdryclass_ead_d_amc, "EGD013",    AirspacesDb::Airspace::bdryclass_ead_d),
	TrafficFlowRestrictions::DbLoader::AirspaceMapping("EGD017",    AirspacesDb::Airspace::bdryclass_ead_d_amc, "EGD017",    AirspacesDb::Airspace::bdryclass_ead_d),
	TrafficFlowRestrictions::DbLoader::AirspaceMapping("EGD023",    AirspacesDb::Airspace::bdryclass_ead_d_amc, "EGD023",    AirspacesDb::Airspace::bdryclass_ead_d),
	TrafficFlowRestrictions::DbLoader::AirspaceMapping("EGD036",    AirspacesDb::Airspace::bdryclass_ead_d_amc, "EGD036",    AirspacesDb::Airspace::bdryclass_ead_d),
	TrafficFlowRestrictions::DbLoader::AirspaceMapping("EGD037",    AirspacesDb::Airspace::bdryclass_ead_d_amc, "EGD037",    AirspacesDb::Airspace::bdryclass_ead_d),
	TrafficFlowRestrictions::DbLoader::AirspaceMapping("EGD038",    AirspacesDb::Airspace::bdryclass_ead_d_amc, "EGD038",    AirspacesDb::Airspace::bdryclass_ead_d),
	TrafficFlowRestrictions::DbLoader::AirspaceMapping("EGD039",    AirspacesDb::Airspace::bdryclass_ead_d_amc, "EGD039",    AirspacesDb::Airspace::bdryclass_ead_d),
	TrafficFlowRestrictions::DbLoader::AirspaceMapping("EGD040",    AirspacesDb::Airspace::bdryclass_ead_d_amc, "EGD040",    AirspacesDb::Airspace::bdryclass_ead_d),
	TrafficFlowRestrictions::DbLoader::AirspaceMapping("EGD064A",   AirspacesDb::Airspace::bdryclass_ead_d_amc, "EGD064A",   AirspacesDb::Airspace::bdryclass_ead_d),
	TrafficFlowRestrictions::DbLoader::AirspaceMapping("EGD064B",   AirspacesDb::Airspace::bdryclass_ead_d_amc, "EGD064B",   AirspacesDb::Airspace::bdryclass_ead_d),
	TrafficFlowRestrictions::DbLoader::AirspaceMapping("EGD064C",   AirspacesDb::Airspace::bdryclass_ead_d_amc, "EGD064C",   AirspacesDb::Airspace::bdryclass_ead_d),
	TrafficFlowRestrictions::DbLoader::AirspaceMapping("EGD113A",   AirspacesDb::Airspace::bdryclass_ead_d_amc, "EGD113A",   AirspacesDb::Airspace::bdryclass_ead_d),
	TrafficFlowRestrictions::DbLoader::AirspaceMapping("EGD113B",   AirspacesDb::Airspace::bdryclass_ead_d_amc, "EGD113B",   AirspacesDb::Airspace::bdryclass_ead_d),
	TrafficFlowRestrictions::DbLoader::AirspaceMapping("EGD115A",   AirspacesDb::Airspace::bdryclass_ead_d_amc, "EGD115A",   AirspacesDb::Airspace::bdryclass_ead_d),
	TrafficFlowRestrictions::DbLoader::AirspaceMapping("EGD115B",   AirspacesDb::Airspace::bdryclass_ead_d_amc, "EGD115B",   AirspacesDb::Airspace::bdryclass_ead_d),
	TrafficFlowRestrictions::DbLoader::AirspaceMapping("EGD123",    AirspacesDb::Airspace::bdryclass_ead_d_amc, "EGD123",    AirspacesDb::Airspace::bdryclass_ead_d),
	TrafficFlowRestrictions::DbLoader::AirspaceMapping("EGD124",    AirspacesDb::Airspace::bdryclass_ead_d_amc, "EGD124",    AirspacesDb::Airspace::bdryclass_ead_d),
	TrafficFlowRestrictions::DbLoader::AirspaceMapping("EGD125",    AirspacesDb::Airspace::bdryclass_ead_d_amc, "EGD125",    AirspacesDb::Airspace::bdryclass_ead_d),
	TrafficFlowRestrictions::DbLoader::AirspaceMapping("EGD128",    AirspacesDb::Airspace::bdryclass_ead_d_amc, "EGD128",    AirspacesDb::Airspace::bdryclass_ead_d),
	TrafficFlowRestrictions::DbLoader::AirspaceMapping("EGD201",    AirspacesDb::Airspace::bdryclass_ead_d_amc, "EGD201",    AirspacesDb::Airspace::bdryclass_ead_d),
	TrafficFlowRestrictions::DbLoader::AirspaceMapping("EGD201A",   AirspacesDb::Airspace::bdryclass_ead_d_amc, "EGD201A",   AirspacesDb::Airspace::bdryclass_ead_d),
	TrafficFlowRestrictions::DbLoader::AirspaceMapping("EGD201B",   AirspacesDb::Airspace::bdryclass_ead_d_amc, "EGD201B",   AirspacesDb::Airspace::bdryclass_ead_d),
	TrafficFlowRestrictions::DbLoader::AirspaceMapping("EGD201C",   AirspacesDb::Airspace::bdryclass_ead_d_amc, "EGD201C",   AirspacesDb::Airspace::bdryclass_ead_d),
	TrafficFlowRestrictions::DbLoader::AirspaceMapping("EGD201D",   AirspacesDb::Airspace::bdryclass_ead_d_amc, "EGD201D",   AirspacesDb::Airspace::bdryclass_ead_d),
	TrafficFlowRestrictions::DbLoader::AirspaceMapping("EGD201E",   AirspacesDb::Airspace::bdryclass_ead_d_amc, "EGD201E",   AirspacesDb::Airspace::bdryclass_ead_d),
	TrafficFlowRestrictions::DbLoader::AirspaceMapping("EGD203",    AirspacesDb::Airspace::bdryclass_ead_d_amc, "EGD203",    AirspacesDb::Airspace::bdryclass_ead_d),
	TrafficFlowRestrictions::DbLoader::AirspaceMapping("EGD323A",   AirspacesDb::Airspace::bdryclass_ead_d_amc, "EGD323A",   AirspacesDb::Airspace::bdryclass_ead_d),
	TrafficFlowRestrictions::DbLoader::AirspaceMapping("EGD323B",   AirspacesDb::Airspace::bdryclass_ead_d_amc, "EGD323B",   AirspacesDb::Airspace::bdryclass_ead_d),
	TrafficFlowRestrictions::DbLoader::AirspaceMapping("EGD323C",   AirspacesDb::Airspace::bdryclass_ead_d_amc, "EGD323C",   AirspacesDb::Airspace::bdryclass_ead_d),
	TrafficFlowRestrictions::DbLoader::AirspaceMapping("EGD323D",   AirspacesDb::Airspace::bdryclass_ead_d_amc, "EGD323D",   AirspacesDb::Airspace::bdryclass_ead_d),
	TrafficFlowRestrictions::DbLoader::AirspaceMapping("EGD323E",   AirspacesDb::Airspace::bdryclass_ead_d_amc, "EGD323E",   AirspacesDb::Airspace::bdryclass_ead_d),
	TrafficFlowRestrictions::DbLoader::AirspaceMapping("EGD323F",   AirspacesDb::Airspace::bdryclass_ead_d_amc, "EGD323F",   AirspacesDb::Airspace::bdryclass_ead_d),
	TrafficFlowRestrictions::DbLoader::AirspaceMapping("EGD402A",   AirspacesDb::Airspace::bdryclass_ead_d_amc, "EGD402A",   AirspacesDb::Airspace::bdryclass_ead_d),
	TrafficFlowRestrictions::DbLoader::AirspaceMapping("EGD402B",   AirspacesDb::Airspace::bdryclass_ead_d_amc, "EGD402B",   AirspacesDb::Airspace::bdryclass_ead_d),
	TrafficFlowRestrictions::DbLoader::AirspaceMapping("EGD403",    AirspacesDb::Airspace::bdryclass_ead_d_amc, "EGD403",    AirspacesDb::Airspace::bdryclass_ead_d),
	TrafficFlowRestrictions::DbLoader::AirspaceMapping("EGD403A",   AirspacesDb::Airspace::bdryclass_ead_d_amc, "EGD403A",   AirspacesDb::Airspace::bdryclass_ead_d),
	TrafficFlowRestrictions::DbLoader::AirspaceMapping("EGD405A",   AirspacesDb::Airspace::bdryclass_ead_d_amc, "EGD405A",   AirspacesDb::Airspace::bdryclass_ead_d),
	TrafficFlowRestrictions::DbLoader::AirspaceMapping("EGD406",    AirspacesDb::Airspace::bdryclass_ead_d_amc, "EGD406",    AirspacesDb::Airspace::bdryclass_ead_d),
	TrafficFlowRestrictions::DbLoader::AirspaceMapping("EGD406B",   AirspacesDb::Airspace::bdryclass_ead_d_amc, "EGD406B",   AirspacesDb::Airspace::bdryclass_ead_d),
	TrafficFlowRestrictions::DbLoader::AirspaceMapping("EGD406C",   AirspacesDb::Airspace::bdryclass_ead_d_amc, "EGD406C",   AirspacesDb::Airspace::bdryclass_ead_d),
	TrafficFlowRestrictions::DbLoader::AirspaceMapping("EGD509",    AirspacesDb::Airspace::bdryclass_ead_d_amc, "EGD509",    AirspacesDb::Airspace::bdryclass_ead_d),
	TrafficFlowRestrictions::DbLoader::AirspaceMapping("EGD510",    AirspacesDb::Airspace::bdryclass_ead_d_amc, "EGD510",    AirspacesDb::Airspace::bdryclass_ead_d),
	TrafficFlowRestrictions::DbLoader::AirspaceMapping("EGD510A",   AirspacesDb::Airspace::bdryclass_ead_d_amc, "EGD510A",   AirspacesDb::Airspace::bdryclass_ead_d),
	TrafficFlowRestrictions::DbLoader::AirspaceMapping("EGD512",    AirspacesDb::Airspace::bdryclass_ead_d_amc, "EGD512",    AirspacesDb::Airspace::bdryclass_ead_d),
	TrafficFlowRestrictions::DbLoader::AirspaceMapping("EGD512A",   AirspacesDb::Airspace::bdryclass_ead_d_amc, "EGD512A",   AirspacesDb::Airspace::bdryclass_ead_d),
	TrafficFlowRestrictions::DbLoader::AirspaceMapping("EGD513",    AirspacesDb::Airspace::bdryclass_ead_d_amc, "EGD513",    AirspacesDb::Airspace::bdryclass_ead_d),
	TrafficFlowRestrictions::DbLoader::AirspaceMapping("EGD513A",   AirspacesDb::Airspace::bdryclass_ead_d_amc, "EGD513A",   AirspacesDb::Airspace::bdryclass_ead_d),
	TrafficFlowRestrictions::DbLoader::AirspaceMapping("EGD513B",   AirspacesDb::Airspace::bdryclass_ead_d_amc, "EGD513B",   AirspacesDb::Airspace::bdryclass_ead_d),
	TrafficFlowRestrictions::DbLoader::AirspaceMapping("EGD601",    AirspacesDb::Airspace::bdryclass_ead_d_amc, "EGD601",    AirspacesDb::Airspace::bdryclass_ead_d),
	TrafficFlowRestrictions::DbLoader::AirspaceMapping("EGD613A",   AirspacesDb::Airspace::bdryclass_ead_d_amc, "EGD613A",   AirspacesDb::Airspace::bdryclass_ead_d),
	TrafficFlowRestrictions::DbLoader::AirspaceMapping("EGD613B",   AirspacesDb::Airspace::bdryclass_ead_d_amc, "EGD613B",   AirspacesDb::Airspace::bdryclass_ead_d),
	TrafficFlowRestrictions::DbLoader::AirspaceMapping("EGD613C",   AirspacesDb::Airspace::bdryclass_ead_d_amc, "EGD613C",   AirspacesDb::Airspace::bdryclass_ead_d),
	TrafficFlowRestrictions::DbLoader::AirspaceMapping("EGD701",    AirspacesDb::Airspace::bdryclass_ead_d_amc, "EGD701",    AirspacesDb::Airspace::bdryclass_ead_d),
	TrafficFlowRestrictions::DbLoader::AirspaceMapping("EGD701A",   AirspacesDb::Airspace::bdryclass_ead_d_amc, "EGD701A",   AirspacesDb::Airspace::bdryclass_ead_d),
	TrafficFlowRestrictions::DbLoader::AirspaceMapping("EGD701B",   AirspacesDb::Airspace::bdryclass_ead_d_amc, "EGD701B",   AirspacesDb::Airspace::bdryclass_ead_d),
	TrafficFlowRestrictions::DbLoader::AirspaceMapping("EGD701C",   AirspacesDb::Airspace::bdryclass_ead_d_amc, "EGD701C",   AirspacesDb::Airspace::bdryclass_ead_d),
	TrafficFlowRestrictions::DbLoader::AirspaceMapping("EGD701D",   AirspacesDb::Airspace::bdryclass_ead_d_amc, "EGD701D",   AirspacesDb::Airspace::bdryclass_ead_d),
	TrafficFlowRestrictions::DbLoader::AirspaceMapping("EGD701E",   AirspacesDb::Airspace::bdryclass_ead_d_amc, "EGD701E",   AirspacesDb::Airspace::bdryclass_ead_d),
	TrafficFlowRestrictions::DbLoader::AirspaceMapping("EGD701N1",  AirspacesDb::Airspace::bdryclass_ead_d_amc, "EGD701N1",  AirspacesDb::Airspace::bdryclass_ead_d),
	TrafficFlowRestrictions::DbLoader::AirspaceMapping("EGD701N2",  AirspacesDb::Airspace::bdryclass_ead_d_amc, "EGD701N2",  AirspacesDb::Airspace::bdryclass_ead_d),
	TrafficFlowRestrictions::DbLoader::AirspaceMapping("EGD701N3",  AirspacesDb::Airspace::bdryclass_ead_d_amc, "EGD701N3",  AirspacesDb::Airspace::bdryclass_ead_d),
	TrafficFlowRestrictions::DbLoader::AirspaceMapping("EGD701S1",  AirspacesDb::Airspace::bdryclass_ead_d_amc, "EGD701S1",  AirspacesDb::Airspace::bdryclass_ead_d),
	TrafficFlowRestrictions::DbLoader::AirspaceMapping("EGD701S2",  AirspacesDb::Airspace::bdryclass_ead_d_amc, "EGD701S2",  AirspacesDb::Airspace::bdryclass_ead_d),
	TrafficFlowRestrictions::DbLoader::AirspaceMapping("EGD701S3",  AirspacesDb::Airspace::bdryclass_ead_d_amc, "EGD701S3",  AirspacesDb::Airspace::bdryclass_ead_d),
	TrafficFlowRestrictions::DbLoader::AirspaceMapping("EGD701W1",  AirspacesDb::Airspace::bdryclass_ead_d_amc, "EGD701W1",  AirspacesDb::Airspace::bdryclass_ead_d),
	TrafficFlowRestrictions::DbLoader::AirspaceMapping("EGD701W2",  AirspacesDb::Airspace::bdryclass_ead_d_amc, "EGD701W2",  AirspacesDb::Airspace::bdryclass_ead_d),
	TrafficFlowRestrictions::DbLoader::AirspaceMapping("EGD701W3",  AirspacesDb::Airspace::bdryclass_ead_d_amc, "EGD701W3",  AirspacesDb::Airspace::bdryclass_ead_d),
	TrafficFlowRestrictions::DbLoader::AirspaceMapping("EGD712A",   AirspacesDb::Airspace::bdryclass_ead_d_amc, "EGD712A",   AirspacesDb::Airspace::bdryclass_ead_d),
	TrafficFlowRestrictions::DbLoader::AirspaceMapping("EGD712B",   AirspacesDb::Airspace::bdryclass_ead_d_amc, "EGD712B",   AirspacesDb::Airspace::bdryclass_ead_d),
	TrafficFlowRestrictions::DbLoader::AirspaceMapping("EGD712C",   AirspacesDb::Airspace::bdryclass_ead_d_amc, "EGD712C",   AirspacesDb::Airspace::bdryclass_ead_d),
	TrafficFlowRestrictions::DbLoader::AirspaceMapping("EGD712D",   AirspacesDb::Airspace::bdryclass_ead_d_amc, "EGD712D",   AirspacesDb::Airspace::bdryclass_ead_d),
	TrafficFlowRestrictions::DbLoader::AirspaceMapping("EGD801",    AirspacesDb::Airspace::bdryclass_ead_d_amc, "EGD801",    AirspacesDb::Airspace::bdryclass_ead_d),
	TrafficFlowRestrictions::DbLoader::AirspaceMapping("EGD802",    AirspacesDb::Airspace::bdryclass_ead_d_amc, "EGD802",    AirspacesDb::Airspace::bdryclass_ead_d),
	TrafficFlowRestrictions::DbLoader::AirspaceMapping("EGD803",    AirspacesDb::Airspace::bdryclass_ead_d_amc, "EGD803",    AirspacesDb::Airspace::bdryclass_ead_d),
	TrafficFlowRestrictions::DbLoader::AirspaceMapping("EGD809C",   AirspacesDb::Airspace::bdryclass_ead_d_amc, "EGD809C",   AirspacesDb::Airspace::bdryclass_ead_d),
	TrafficFlowRestrictions::DbLoader::AirspaceMapping("EGD809N",   AirspacesDb::Airspace::bdryclass_ead_d_amc, "EGD809N",   AirspacesDb::Airspace::bdryclass_ead_d),
	TrafficFlowRestrictions::DbLoader::AirspaceMapping("EGD809S",   AirspacesDb::Airspace::bdryclass_ead_d_amc, "EGD809S",   AirspacesDb::Airspace::bdryclass_ead_d),
	TrafficFlowRestrictions::DbLoader::AirspaceMapping("EGDFJAN",   AirspacesDb::Airspace::bdryclass_ead_d_amc, "EGDFJAN",   AirspacesDb::Airspace::bdryclass_ead_d),
	TrafficFlowRestrictions::DbLoader::AirspaceMapping("EGDFJAS",   AirspacesDb::Airspace::bdryclass_ead_d_amc, "EGDFJAS",   AirspacesDb::Airspace::bdryclass_ead_d),
	TrafficFlowRestrictions::DbLoader::AirspaceMapping("EGDFJASE",  AirspacesDb::Airspace::bdryclass_ead_d_amc, "EGDFJASE",  AirspacesDb::Airspace::bdryclass_ead_d),
	TrafficFlowRestrictions::DbLoader::AirspaceMapping("EGDJ1",     AirspacesDb::Airspace::bdryclass_ead_d_amc, "EGDJ1",     AirspacesDb::Airspace::bdryclass_ead_d),
	TrafficFlowRestrictions::DbLoader::AirspaceMapping("EGDJ4",     AirspacesDb::Airspace::bdryclass_ead_d_amc, "EGDJ4",     AirspacesDb::Airspace::bdryclass_ead_d),
	TrafficFlowRestrictions::DbLoader::AirspaceMapping("EGDJ4TGT",  AirspacesDb::Airspace::bdryclass_ead_d_amc, "EGDJ4TGT",  AirspacesDb::Airspace::bdryclass_ead_d),
	TrafficFlowRestrictions::DbLoader::AirspaceMapping("EGDJ5",     AirspacesDb::Airspace::bdryclass_ead_d_amc, "EGDJ5",     AirspacesDb::Airspace::bdryclass_ead_d),
	TrafficFlowRestrictions::DbLoader::AirspaceMapping("EGDJ6",     AirspacesDb::Airspace::bdryclass_ead_d_amc, "EGDJ6",     AirspacesDb::Airspace::bdryclass_ead_d),
	TrafficFlowRestrictions::DbLoader::AirspaceMapping("EGGXOCA",   AirspacesDb::Airspace::bdryclass_ead_oca,   "EGGX",      AirspacesDb::Airspace::bdryclass_ead_oca),
	TrafficFlowRestrictions::DbLoader::AirspaceMapping("EGTRA008",  AirspacesDb::Airspace::bdryclass_ead_tra,   "EGTRA008A", AirspacesDb::Airspace::bdryclass_ead_tra, "EGTRA008B", AirspacesDb::Airspace::bdryclass_ead_tra, "EGTRA008C"),
	TrafficFlowRestrictions::DbLoader::AirspaceMapping("EHAA",      AirspacesDb::Airspace::bdryclass_ead_cta,   "EHAA",      AirspacesDb::Airspace::bdryclass_ead_fir),
	TrafficFlowRestrictions::DbLoader::AirspaceMapping("EID1",      AirspacesDb::Airspace::bdryclass_ead_d_amc, "EID1",      AirspacesDb::Airspace::bdryclass_ead_d),
	TrafficFlowRestrictions::DbLoader::AirspaceMapping("EID14",     AirspacesDb::Airspace::bdryclass_ead_d_amc, "EID14",     AirspacesDb::Airspace::bdryclass_ead_d),
	TrafficFlowRestrictions::DbLoader::AirspaceMapping("EID5",      AirspacesDb::Airspace::bdryclass_ead_d_amc, "EID5",      AirspacesDb::Airspace::bdryclass_ead_d),
	TrafficFlowRestrictions::DbLoader::AirspaceMapping("EKD301",    AirspacesDb::Airspace::bdryclass_ead_d_amc, "EKD301",    AirspacesDb::Airspace::bdryclass_ead_d),
	TrafficFlowRestrictions::DbLoader::AirspaceMapping("EKD302",    AirspacesDb::Airspace::bdryclass_ead_d_amc, "EKD302",    AirspacesDb::Airspace::bdryclass_ead_d),
	TrafficFlowRestrictions::DbLoader::AirspaceMapping("EKD303",    AirspacesDb::Airspace::bdryclass_ead_d_amc, "EKD303",    AirspacesDb::Airspace::bdryclass_ead_d),
	TrafficFlowRestrictions::DbLoader::AirspaceMapping("EKD350",    AirspacesDb::Airspace::bdryclass_ead_d_amc, "EKD350",    AirspacesDb::Airspace::bdryclass_ead_d),
	TrafficFlowRestrictions::DbLoader::AirspaceMapping("EKD351",    AirspacesDb::Airspace::bdryclass_ead_d_amc, "EKD351",    AirspacesDb::Airspace::bdryclass_ead_d),
	TrafficFlowRestrictions::DbLoader::AirspaceMapping("EKD352",    AirspacesDb::Airspace::bdryclass_ead_d_amc, "EKD352",    AirspacesDb::Airspace::bdryclass_ead_d),
	TrafficFlowRestrictions::DbLoader::AirspaceMapping("EKD353",    AirspacesDb::Airspace::bdryclass_ead_d_amc, "EKD353",    AirspacesDb::Airspace::bdryclass_ead_d),
	TrafficFlowRestrictions::DbLoader::AirspaceMapping("EKD370",    AirspacesDb::Airspace::bdryclass_ead_d_amc, "EKD370",    AirspacesDb::Airspace::bdryclass_ead_d),
	TrafficFlowRestrictions::DbLoader::AirspaceMapping("EKD371",    AirspacesDb::Airspace::bdryclass_ead_d_amc, "EKD371",    AirspacesDb::Airspace::bdryclass_ead_d),
	TrafficFlowRestrictions::DbLoader::AirspaceMapping("EKD373",    AirspacesDb::Airspace::bdryclass_ead_d_amc, "EKD373",    AirspacesDb::Airspace::bdryclass_ead_d),
	TrafficFlowRestrictions::DbLoader::AirspaceMapping("EKD380",    AirspacesDb::Airspace::bdryclass_ead_d_amc, "EKD380",    AirspacesDb::Airspace::bdryclass_ead_d),
	TrafficFlowRestrictions::DbLoader::AirspaceMapping("EKD381",    AirspacesDb::Airspace::bdryclass_ead_d_amc, "EKD381",    AirspacesDb::Airspace::bdryclass_ead_d),
	TrafficFlowRestrictions::DbLoader::AirspaceMapping("EKD389",    AirspacesDb::Airspace::bdryclass_ead_d_amc, "EKD389",    AirspacesDb::Airspace::bdryclass_ead_d),
	TrafficFlowRestrictions::DbLoader::AirspaceMapping("EKR11",     AirspacesDb::Airspace::bdryclass_ead_r_amc, "EKR11",     AirspacesDb::Airspace::bdryclass_ead_r),
	TrafficFlowRestrictions::DbLoader::AirspaceMapping("EKR12",     AirspacesDb::Airspace::bdryclass_ead_r_amc, "EKR12",     AirspacesDb::Airspace::bdryclass_ead_r),
	TrafficFlowRestrictions::DbLoader::AirspaceMapping("EKR13",     AirspacesDb::Airspace::bdryclass_ead_r_amc, "EKR13",     AirspacesDb::Airspace::bdryclass_ead_r),
	TrafficFlowRestrictions::DbLoader::AirspaceMapping("EKR14",     AirspacesDb::Airspace::bdryclass_ead_r_amc, "EKR14",     AirspacesDb::Airspace::bdryclass_ead_r),
	TrafficFlowRestrictions::DbLoader::AirspaceMapping("EKR32",     AirspacesDb::Airspace::bdryclass_ead_r_amc, "EKR32",     AirspacesDb::Airspace::bdryclass_ead_r),
	TrafficFlowRestrictions::DbLoader::AirspaceMapping("EKR33",     AirspacesDb::Airspace::bdryclass_ead_r_amc, "EKR33",     AirspacesDb::Airspace::bdryclass_ead_r),
	TrafficFlowRestrictions::DbLoader::AirspaceMapping("EKR34",     AirspacesDb::Airspace::bdryclass_ead_r_amc, "EKR34",     AirspacesDb::Airspace::bdryclass_ead_r),
	TrafficFlowRestrictions::DbLoader::AirspaceMapping("EKR35",     AirspacesDb::Airspace::bdryclass_ead_r_amc, "EKR35",     AirspacesDb::Airspace::bdryclass_ead_r),
	TrafficFlowRestrictions::DbLoader::AirspaceMapping("EKR38",     AirspacesDb::Airspace::bdryclass_ead_r_amc, "EKR38",     AirspacesDb::Airspace::bdryclass_ead_r),
	TrafficFlowRestrictions::DbLoader::AirspaceMapping("EKR39",     AirspacesDb::Airspace::bdryclass_ead_r_amc, "EKR39",     AirspacesDb::Airspace::bdryclass_ead_r),
	TrafficFlowRestrictions::DbLoader::AirspaceMapping("EKR40",     AirspacesDb::Airspace::bdryclass_ead_r_amc, "EKR40",     AirspacesDb::Airspace::bdryclass_ead_r),
	TrafficFlowRestrictions::DbLoader::AirspaceMapping("EKR48",     AirspacesDb::Airspace::bdryclass_ead_r_amc, "EKR48",     AirspacesDb::Airspace::bdryclass_ead_r),
	TrafficFlowRestrictions::DbLoader::AirspaceMapping("ENOS",      AirspacesDb::Airspace::bdryclass_ead_cta,   "ENOS",      AirspacesDb::Airspace::bdryclass_ead_fir),
	TrafficFlowRestrictions::DbLoader::AirspaceMapping("ESMM",      AirspacesDb::Airspace::bdryclass_ead_cta,   "ESMM",      AirspacesDb::Airspace::bdryclass_ead_fir),
	TrafficFlowRestrictions::DbLoader::AirspaceMapping("ESTRA80",   AirspacesDb::Airspace::bdryclass_ead_cba,   "ESTRA80",   AirspacesDb::Airspace::bdryclass_ead_tra),
	TrafficFlowRestrictions::DbLoader::AirspaceMapping("GCCC",      AirspacesDb::Airspace::bdryclass_ead_cta,   "GCCC",      AirspacesDb::Airspace::bdryclass_ead_fir),
	TrafficFlowRestrictions::DbLoader::AirspaceMapping("GMMM",      AirspacesDb::Airspace::bdryclass_ead_cta,   "GMMM",      AirspacesDb::Airspace::bdryclass_ead_fir),
	TrafficFlowRestrictions::DbLoader::AirspaceMapping("GOOO",      AirspacesDb::Airspace::bdryclass_ead_cta,   "GOOO",      AirspacesDb::Airspace::bdryclass_ead_fir),
	TrafficFlowRestrictions::DbLoader::AirspaceMapping("GVSC",      AirspacesDb::Airspace::bdryclass_ead_cta,   "GVSC",      AirspacesDb::Airspace::bdryclass_ead_fir),
	TrafficFlowRestrictions::DbLoader::AirspaceMapping("HECC",      AirspacesDb::Airspace::bdryclass_ead_cta,   "HECC",      AirspacesDb::Airspace::bdryclass_ead_fir),
	TrafficFlowRestrictions::DbLoader::AirspaceMapping("HLLL",      AirspacesDb::Airspace::bdryclass_ead_cta,   "HLLL",      AirspacesDb::Airspace::bdryclass_ead_fir),
	TrafficFlowRestrictions::DbLoader::AirspaceMapping("LECM",      AirspacesDb::Airspace::bdryclass_ead_cta,   "LECM",      AirspacesDb::Airspace::bdryclass_ead_fir),
	TrafficFlowRestrictions::DbLoader::AirspaceMapping("LED104",    AirspacesDb::Airspace::bdryclass_ead_d_amc, "LED104",    AirspacesDb::Airspace::bdryclass_ead_d),
	TrafficFlowRestrictions::DbLoader::AirspaceMapping("LED131",    AirspacesDb::Airspace::bdryclass_ead_d_amc, "LED131",    AirspacesDb::Airspace::bdryclass_ead_d),
	TrafficFlowRestrictions::DbLoader::AirspaceMapping("LED132",    AirspacesDb::Airspace::bdryclass_ead_d_amc, "LED132",    AirspacesDb::Airspace::bdryclass_ead_d),
	TrafficFlowRestrictions::DbLoader::AirspaceMapping("LED21B",    AirspacesDb::Airspace::bdryclass_ead_d_amc, "LED21B",    AirspacesDb::Airspace::bdryclass_ead_d),
	TrafficFlowRestrictions::DbLoader::AirspaceMapping("LFRR",      AirspacesDb::Airspace::bdryclass_ead_cta,   "LFRR",      AirspacesDb::Airspace::bdryclass_ead_fir),
	TrafficFlowRestrictions::DbLoader::AirspaceMapping("LKAA",      AirspacesDb::Airspace::bdryclass_ead_uta,   "LKAA",      AirspacesDb::Airspace::bdryclass_ead_fir),
	TrafficFlowRestrictions::DbLoader::AirspaceMapping("LLLL",      AirspacesDb::Airspace::bdryclass_ead_cta,   "LLLL",      AirspacesDb::Airspace::bdryclass_ead_fir),
	TrafficFlowRestrictions::DbLoader::AirspaceMapping("LOVV",      AirspacesDb::Airspace::bdryclass_ead_cta,   "LOVV",      AirspacesDb::Airspace::bdryclass_ead_fir),
	TrafficFlowRestrictions::DbLoader::AirspaceMapping("LPPC",      AirspacesDb::Airspace::bdryclass_ead_cta,   "LPPC",      AirspacesDb::Airspace::bdryclass_ead_fir),
	TrafficFlowRestrictions::DbLoader::AirspaceMapping("LPPOOCA",   AirspacesDb::Airspace::bdryclass_ead_oca,   "LPPO",      AirspacesDb::Airspace::bdryclass_ead_oca),
	TrafficFlowRestrictions::DbLoader::AirspaceMapping("LSAG",      AirspacesDb::Airspace::bdryclass_ead_uta,   "LSAGALL",   AirspacesDb::Airspace::bdryclass_ead_sector_c),
	TrafficFlowRestrictions::DbLoader::AirspaceMapping("LSAZ",      AirspacesDb::Airspace::bdryclass_ead_uta,   "LSAZM16",   AirspacesDb::Airspace::bdryclass_ead_sector_c),
	TrafficFlowRestrictions::DbLoader::AirspaceMapping("LTAA",      AirspacesDb::Airspace::bdryclass_ead_cta,   "LTAA",      AirspacesDb::Airspace::bdryclass_ead_fir),
	TrafficFlowRestrictions::DbLoader::AirspaceMapping("LTBB",      AirspacesDb::Airspace::bdryclass_ead_cta,   "LTBB",      AirspacesDb::Airspace::bdryclass_ead_fir),
	TrafficFlowRestrictions::DbLoader::AirspaceMapping("OLBB",      AirspacesDb::Airspace::bdryclass_ead_cta,   "OLBB",      AirspacesDb::Airspace::bdryclass_ead_fir),
	TrafficFlowRestrictions::DbLoader::AirspaceMapping("ORBB",      AirspacesDb::Airspace::bdryclass_ead_cta,   "ORBB",      AirspacesDb::Airspace::bdryclass_ead_fir),
	TrafficFlowRestrictions::DbLoader::AirspaceMapping("UGGG",      AirspacesDb::Airspace::bdryclass_ead_cta,   "UGGG",      AirspacesDb::Airspace::bdryclass_ead_fir),
	TrafficFlowRestrictions::DbLoader::AirspaceMapping("UMKK",      AirspacesDb::Airspace::bdryclass_ead_cta,   "UMKK",      AirspacesDb::Airspace::bdryclass_ead_fir)
};

const TrafficFlowRestrictions::DbLoader::AirspaceMapping *TrafficFlowRestrictions::DbLoader::find_airspace_mapping(const std::string& name, char bc)
{
	static const AirspaceMapping *first(&airspacemapping[0]);
	static const AirspaceMapping *last(&airspacemapping[sizeof(airspacemapping)/sizeof(airspacemapping[0])]);
	AirspaceMapping x(name.c_str(), bc);
	const AirspaceMapping *r(std::lower_bound(first, last, x));
	if (r == last)
		return 0;
	if (x < *r)
		return 0;
	if (true && (name != r->get_name(0) || bc != r->get_bdryclass(0)))
		throw std::runtime_error("find_airspace_mapping error");
	return r;
}

void TrafficFlowRestrictions::DbLoader::check_airspace_mapping(void)
{
	for (unsigned int i = 0; i < sizeof(airspacemapping)/sizeof(airspacemapping[0]); ++i) {
		if (airspacemapping[i].size() >= 1)
			continue;
		std::ostringstream oss;
		oss << "Airspace mapping " << airspacemapping[i].get_name(0) << '/'
		    << AirspacesDb::Airspace::get_class_string(airspacemapping[i].get_bdryclass(0), AirspacesDb::Airspace::typecode_ead)
		    << " is invalid";
		std::cerr << oss.str() << std::endl;
		throw std::runtime_error(oss.str());
	}
	for (unsigned int i = 1; i < sizeof(airspacemapping)/sizeof(airspacemapping[0]); ++i) {
		if (airspacemapping[i-1] < airspacemapping[i])
			continue;
		std::ostringstream oss;
		oss << "Airspace mapping " << airspacemapping[i].get_name(0) << '/'
		    << AirspacesDb::Airspace::get_class_string(airspacemapping[i].get_bdryclass(0), AirspacesDb::Airspace::typecode_ead)
		    << " has non-ascending sort order";
		std::cerr << oss.str() << std::endl;
		throw std::runtime_error(oss.str());
	}
}

/*

"DKSEFAB",AirspacesDb::Airspace::bdryclass_ead_ras,"CFMU internal airspace - not loaded or withdrawn"
"EDGG1",AirspacesDb::Airspace::bdryclass_ead_sector_c,"CFMU internal airspace - not loaded or withdrawn"
"EDGG6",AirspacesDb::Airspace::bdryclass_ead_sector_c,"CFMU internal airspace - not loaded or withdrawn"
"EDUUHVL1H",AirspacesDb::Airspace::bdryclass_ead_sector_c,"CFMU internal airspace - not loaded or withdrawn"
"EDUUOSE1O",AirspacesDb::Airspace::bdryclass_ead_sector_c,"CFMU internal airspace - not loaded or withdrawn"
"EDUUSAL1A",AirspacesDb::Airspace::bdryclass_ead_sector_c,"CFMU internal airspace - not loaded or withdrawn"
"EDUUSPE1P",AirspacesDb::Airspace::bdryclass_ead_sector_c,"CFMU internal airspace - not loaded or withdrawn"
"EDUUWEST",AirspacesDb::Airspace::bdryclass_ead_sector_c,"CFMU internal airspace - not loaded or withdrawn"
"EDWWDSTC",AirspacesDb::Airspace::bdryclass_ead_sector_c,"CFMU internal airspace - not loaded or withdrawn"
"EDYYDELHI",AirspacesDb::Airspace::bdryclass_ead_sector,"CFMU internal airspace - not loaded or withdrawn"
"EDYYDELLO",AirspacesDb::Airspace::bdryclass_ead_sector,"CFMU internal airspace - not loaded or withdrawn"
"EDYYDELTA",AirspacesDb::Airspace::bdryclass_ead_sector_c,"CFMU internal airspace - not loaded or withdrawn"
"EDYYHOL",AirspacesDb::Airspace::bdryclass_ead_sector_c,"CFMU internal airspace - not loaded or withdrawn"
"EDYYMRCS",AirspacesDb::Airspace::bdryclass_ead_sector_c,"CFMU internal airspace - not loaded or withdrawn"
"EFES3",AirspacesDb::Airspace::bdryclass_ead_sector,"CFMU internal airspace - not loaded or withdrawn"
"EFES67",AirspacesDb::Airspace::bdryclass_ead_sector_c,"CFMU internal airspace - not loaded or withdrawn"
"EGATLANT",AirspacesDb::Airspace::bdryclass_ead_d_amc,"CFMU internal airspace - not loaded or withdrawn"
"EGDJ1TGT",AirspacesDb::Airspace::bdryclass_ead_d_amc,"CFMU internal airspace - not loaded or withdrawn"
"EGMTANWNL",AirspacesDb::Airspace::bdryclass_ead_tra,"CFMU internal airspace - not loaded or withdrawn"
"EGMTANWNU",AirspacesDb::Airspace::bdryclass_ead_tra,"CFMU internal airspace - not loaded or withdrawn"
"EGMTANWSL",AirspacesDb::Airspace::bdryclass_ead_tra,"CFMU internal airspace - not loaded or withdrawn"
"EGMTANWSU",AirspacesDb::Airspace::bdryclass_ead_tra,"CFMU internal airspace - not loaded or withdrawn"
"EGOCEAN",AirspacesDb::Airspace::bdryclass_ead_d_amc,"CFMU internal airspace - not loaded or withdrawn"
"EGSOYUZ",AirspacesDb::Airspace::bdryclass_ead_d_amc,"CFMU internal airspace - not loaded or withdrawn"
"EGTRAGNY",AirspacesDb::Airspace::bdryclass_ead_tra,"CFMU internal airspace - not loaded or withdrawn"
"EGTRASTER",AirspacesDb::Airspace::bdryclass_ead_tra,"CFMU internal airspace - not loaded or withdrawn"
"EISNNOTA",AirspacesDb::Airspace::bdryclass_ead_sector,"CFMU internal airspace - not loaded or withdrawn"
"EISNSOTA",AirspacesDb::Airspace::bdryclass_ead_sector,"CFMU internal airspace - not loaded or withdrawn"
"EKDMULTEX",AirspacesDb::Airspace::bdryclass_ead_d_amc,"CFMU internal airspace - not loaded or withdrawn"
"EKRNAV195",AirspacesDb::Airspace::bdryclass_ead_sector,"CFMU internal airspace - not loaded or withdrawn"
"EKRNAV95",AirspacesDb::Airspace::bdryclass_ead_sector,"CFMU internal airspace - not loaded or withdrawn"
"ESMM2",AirspacesDb::Airspace::bdryclass_ead_sector,"CFMU internal airspace - not loaded or withdrawn"
"ESMMK",AirspacesDb::Airspace::bdryclass_ead_sector,"CFMU internal airspace - not loaded or withdrawn"
"ESOSNF",AirspacesDb::Airspace::bdryclass_ead_sector_c,"CFMU internal airspace - not loaded or withdrawn"
"ESTPIRTTI",AirspacesDb::Airspace::bdryclass_ead_tra,"CFMU internal airspace - not loaded or withdrawn"
"ESTRAFVO",AirspacesDb::Airspace::bdryclass_ead_tra,"CFMU internal airspace - not loaded or withdrawn"
"ESTRATTP",AirspacesDb::Airspace::bdryclass_ead_tra,"CFMU internal airspace - not loaded or withdrawn"
"GCCCIZA",AirspacesDb::Airspace::bdryclass_ead_sector,"CFMU internal airspace - not loaded or withdrawn"
"GCCCROQ",AirspacesDb::Airspace::bdryclass_ead_sector,"CFMU internal airspace - not loaded or withdrawn"
"LFATOCA",AirspacesDb::Airspace::bdryclass_ead_d_amc,"CFMU internal airspace - not loaded or withdrawn"
"LFATOCB",AirspacesDb::Airspace::bdryclass_ead_d_amc,"CFMU internal airspace - not loaded or withdrawn"
"LFBBZ2",AirspacesDb::Airspace::bdryclass_ead_sector,"CFMU internal airspace - not loaded or withdrawn"
"LFBBZ3",AirspacesDb::Airspace::bdryclass_ead_sector,"CFMU internal airspace - not loaded or withdrawn"
"LFDIAM",AirspacesDb::Airspace::bdryclass_ead_tra,"CFMU internal airspace - not loaded or withdrawn"
"LFEE4N",AirspacesDb::Airspace::bdryclass_ead_sector_c,"CFMU internal airspace - not loaded or withdrawn"
"LFEE5EH",AirspacesDb::Airspace::bdryclass_ead_sector_c,"CFMU internal airspace - not loaded or withdrawn"
"LFEEKD",AirspacesDb::Airspace::bdryclass_ead_sector,"CFMU internal airspace - not loaded or withdrawn"
"LFEERFUE",AirspacesDb::Airspace::bdryclass_ead_sector_c,"CFMU internal airspace - not loaded or withdrawn"
"LFEESE",AirspacesDb::Airspace::bdryclass_ead_sector,"CFMU internal airspace - not loaded or withdrawn"
"LFEEURMN",AirspacesDb::Airspace::bdryclass_ead_sector_c,"CFMU internal airspace - not loaded or withdrawn"
"LFFFLMH",AirspacesDb::Airspace::bdryclass_ead_sector_c,"CFMU internal airspace - not loaded or withdrawn"
"LFFFTL",AirspacesDb::Airspace::bdryclass_ead_sector,"CFMU internal airspace - not loaded or withdrawn"
"LFFFUJ",AirspacesDb::Airspace::bdryclass_ead_sector,"CFMU internal airspace - not loaded or withdrawn"
"LFMMB1",AirspacesDb::Airspace::bdryclass_ead_sector,"CFMU internal airspace - not loaded or withdrawn"
"LFMMB2",AirspacesDb::Airspace::bdryclass_ead_sector,"CFMU internal airspace - not loaded or withdrawn"
"LFMMB3",AirspacesDb::Airspace::bdryclass_ead_sector,"CFMU internal airspace - not loaded or withdrawn"
"LFMME2",AirspacesDb::Airspace::bdryclass_ead_sector,"CFMU internal airspace - not loaded or withdrawn"
"LFMME3",AirspacesDb::Airspace::bdryclass_ead_sector,"CFMU internal airspace - not loaded or withdrawn"
"LFMMG1",AirspacesDb::Airspace::bdryclass_ead_sector,"CFMU internal airspace - not loaded or withdrawn"
"LFMMG2",AirspacesDb::Airspace::bdryclass_ead_sector,"CFMU internal airspace - not loaded or withdrawn"
"LFMMG3",AirspacesDb::Airspace::bdryclass_ead_sector,"CFMU internal airspace - not loaded or withdrawn"
"LFMMML",AirspacesDb::Airspace::bdryclass_ead_sector,"CFMU internal airspace - not loaded or withdrawn"
"LFMMWW",AirspacesDb::Airspace::bdryclass_ead_sector_c,"CFMU internal airspace - not loaded or withdrawn"
"LFMMY1",AirspacesDb::Airspace::bdryclass_ead_sector,"CFMU internal airspace - not loaded or withdrawn"
"LFMMY2",AirspacesDb::Airspace::bdryclass_ead_sector,"CFMU internal airspace - not loaded or withdrawn"
"LFMMY3",AirspacesDb::Airspace::bdryclass_ead_sector,"CFMU internal airspace - not loaded or withdrawn"
"LFRRFWEST",AirspacesDb::Airspace::bdryclass_ead_sector_c,"CFMU internal airspace - not loaded or withdrawn"
"LFRRV",AirspacesDb::Airspace::bdryclass_ead_sector_c,"CFMU internal airspace - not loaded or withdrawn"
"LFRUBIS",AirspacesDb::Airspace::bdryclass_ead_tra,"CFMU internal airspace - not loaded or withdrawn"
"LFSBBM",AirspacesDb::Airspace::bdryclass_ead_sector,"CFMU internal airspace - not loaded or withdrawn"
"LIBBUND",AirspacesDb::Airspace::bdryclass_ead_sector_c,"CFMU internal airspace - not loaded or withdrawn"
"LIMME",AirspacesDb::Airspace::bdryclass_ead_sector_c,"CFMU internal airspace - not loaded or withdrawn"
"LIMMW",AirspacesDb::Airspace::bdryclass_ead_sector_c,"CFMU internal airspace - not loaded or withdrawn"
"LIRRMINW",AirspacesDb::Airspace::bdryclass_ead_sector_c,"CFMU internal airspace - not loaded or withdrawn"


"EDYY",AirspacesDb::Airspace::bdryclass_ead_uta,"not loaded"
"EFETCTR",AirspacesDb::Airspace::bdryclass_ead_ctr,"not loaded"
"EFKICTR",AirspacesDb::Airspace::bdryclass_ead_ctr,"not loaded"
"EFMICTR",AirspacesDb::Airspace::bdryclass_ead_ctr,"not loaded"
"EFSACTR",AirspacesDb::Airspace::bdryclass_ead_ctr,"not loaded"
"EFSICTR",AirspacesDb::Airspace::bdryclass_ead_ctr,"not loaded"
"EFVRCTR",AirspacesDb::Airspace::bdryclass_ead_ctr,"not loaded"
"EGJJ",AirspacesDb::Airspace::bdryclass_ead_tma,"not loaded"
"ENTSAB7A",AirspacesDb::Airspace::bdryclass_ead_tsa,"not loaded"
"ENTSAT2B",AirspacesDb::Airspace::bdryclass_ead_tsa,"not loaded"
"LEBAZH",AirspacesDb::Airspace::bdryclass_ead_tsa,"not loaded"
"LEBAZL",AirspacesDb::Airspace::bdryclass_ead_tsa,"not loaded"
"LEBAZM",AirspacesDb::Airspace::bdryclass_ead_tsa,"not loaded"
"LEBLEN",AirspacesDb::Airspace::bdryclass_ead_tsa,"not loaded"
"LEICOL",AirspacesDb::Airspace::bdryclass_ead_tsa,"not loaded"
"LELMAN",AirspacesDb::Airspace::bdryclass_ead_tsa,"not loaded"
"LELORH",AirspacesDb::Airspace::bdryclass_ead_tsa,"not loaded"
"LELORL",AirspacesDb::Airspace::bdryclass_ead_tsa,"not loaded"
"LEMEDI",AirspacesDb::Airspace::bdryclass_ead_tsa,"not loaded"
"LEMURH",AirspacesDb::Airspace::bdryclass_ead_tsa,"not loaded"
"LEMURL",AirspacesDb::Airspace::bdryclass_ead_tsa,"not loaded"
"LETA01",AirspacesDb::Airspace::bdryclass_ead_tsa,"not loaded"
"LETA02",AirspacesDb::Airspace::bdryclass_ead_tsa,"not loaded"
"LETA03",AirspacesDb::Airspace::bdryclass_ead_tsa,"not loaded"

"GL",        AirspacesDb::Airspace::bdryclass_ead_nas,  "loaded"
"HA",        AirspacesDb::Airspace::bdryclass_ead_nas,  "loaded"
"HB",        AirspacesDb::Airspace::bdryclass_ead_nas,  "loaded"
"HC",        AirspacesDb::Airspace::bdryclass_ead_nas,  "loaded"
"HH",        AirspacesDb::Airspace::bdryclass_ead_nas,  "loaded"
"HK",        AirspacesDb::Airspace::bdryclass_ead_nas,  "loaded"
"HR",        AirspacesDb::Airspace::bdryclass_ead_nas,  "loaded"
"HT",        AirspacesDb::Airspace::bdryclass_ead_nas,  "loaded"
"HU",        AirspacesDb::Airspace::bdryclass_ead_nas,  "loaded"
"M",         AirspacesDb::Airspace::bdryclass_ead_nas,  "loaded"
"N",         AirspacesDb::Airspace::bdryclass_ead_nas,  "loaded"
"SAM",       AirspacesDb::Airspace::bdryclass_ead_ras,  "loaded"
"S",         AirspacesDb::Airspace::bdryclass_ead_nas,  "loaded"
"V",         AirspacesDb::Airspace::bdryclass_ead_nas,  "loaded"

*/
