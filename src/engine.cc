/***************************************************************************
 *   Copyright (C) 2007, 2009, 2011, 2012, 2013, 2014, 2016                *
 *     by Thomas Sailer t.sailer@alumni.ethz.ch                            *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 *   This program is distributed in the hope that it will be useful,       *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *   GNU General Public License for more details.                          *
 *                                                                         *
 *   You should have received a copy of the GNU General Public License     *
 *   along with this program; if not, write to the                         *
 *   Free Software Foundation, Inc.,                                       *
 *   59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.             *
 ***************************************************************************/

#include "sysdeps.h"
#include "engine.h"
#include "baro.h"

#include <unistd.h>
#include <sys/types.h>
#ifdef HAVE_SYS_SYSCALL_H
#include <sys/syscall.h>
#endif

Engine::Engine(const std::string& dir_main, auxdb_mode_t auxdbmode, const std::string& dir_aux, bool loadgrib2, bool loadbitmapmaps)
	: m_prefs(), m_dir_main(dir_main), m_dir_aux(get_aux_dir(auxdbmode, dir_aux)), m_dbthread(m_dir_main, m_dir_aux, auxdbmode == auxdb_postgres), m_fplandb()
{
	// m_fplandb opens in its constructor
	try {
		m_tracksdb.open(m_dir_main);
		if (loadgrib2) {
			if (auxdbmode == auxdb_postgres) {
				if (!m_dir_aux.empty()) {
					Glib::Thread::create(sigc::bind(sigc::mem_fun(*this, &Engine::load_grib2), m_dir_aux),
							     0, false, true, Glib::THREAD_PRIORITY_NORMAL);
				}
			} else {
				std::string path1(m_dir_main);
				if (path1.empty())
					path1 = FPlan::get_userdbpath();
				path1 = Glib::build_filename(path1, "gfs");
				std::string path2(m_dir_aux);
				if (path2.empty()) {
					Glib::Thread::create(sigc::bind(sigc::mem_fun(*this, &Engine::load_grib2), path1),
							     0, false, true, Glib::THREAD_PRIORITY_NORMAL);
				} else {
					path2 = Glib::build_filename(path2, "gfs");
					Glib::Thread::create(sigc::bind(sigc::bind(sigc::mem_fun(*this, &Engine::load_grib2_2), path2), path1),
							     0, false, true, Glib::THREAD_PRIORITY_NORMAL);
				}
			}
		}
		if (loadbitmapmaps) {
			if (auxdbmode == auxdb_postgres) {
				if (!m_dir_aux.empty()) {
					Glib::Thread::create(sigc::bind(sigc::mem_fun(*this, &Engine::load_bitmapmaps), m_dir_aux),
							     0, false, true, Glib::THREAD_PRIORITY_NORMAL);
				}
			} else {
				std::string path1(m_dir_main);
				if (path1.empty())
					path1 = FPlan::get_userdbpath();
				path1 = Glib::build_filename(path1, "bitmapmaps");
				std::string path2(m_dir_aux);
				if (path2.empty()) {
					Glib::Thread::create(sigc::bind(sigc::mem_fun(*this, &Engine::load_bitmapmaps), path1),
							     0, false, true, Glib::THREAD_PRIORITY_NORMAL);
				} else {
					path2 = Glib::build_filename(path2, "bitmapmaps");
					Glib::Thread::create(sigc::bind(sigc::bind(sigc::mem_fun(*this, &Engine::load_bitmapmaps_2), path2), path1),
							     0, false, true, Glib::THREAD_PRIORITY_NORMAL);
				}
			}
		}
	} catch (const std::exception& e) {
		std::cerr << "Error opening tracks database: " << e.what() << std::endl;
	}
}

std::string Engine::get_aux_dir(auxdb_mode_t auxdbmode, const std::string& dir_aux)
{
	switch (auxdbmode) {
	case auxdb_prefs:
		return (Glib::ustring)m_prefs.globaldbdir;

	case auxdb_override:
		return dir_aux;

	case auxdb_postgres:
		if (dir_aux.empty())
			return (Glib::ustring)m_prefs.globaldbdir;
		return dir_aux;

	default:
		return std::string();
	}
}

std::string Engine::get_default_aux_dir(void)
{
#ifdef __MINGW32__
	{
		gchar *instdir(g_win32_get_package_installation_directory_of_module(0));
		if (instdir)
			return Glib::build_filename(instdir, "db");
	}
#endif /* __MINGW32__ */
	return PACKAGE_DATA_DIR;
}

void Engine::load_grib2(std::string path)
{
	GRIB2::Parser p(m_grib2);
	p.parse(path);
	unsigned int count(m_grib2.find_layers().size());
	if (true)
		std::cout << "Loaded " << count << " GRIB2 Layers from " << path << std::endl;
	unsigned int count3(m_grib2.expire_cache());
	if (true && count3)
		std::cout << "Expired " << count3 << " cached GRIB2 Layers" << std::endl;
}

void Engine::load_grib2_2(std::string path1, std::string path2)
{
	GRIB2::Parser p(m_grib2);
	p.parse(path1);
	unsigned int count1(m_grib2.find_layers().size());
	p.parse(path2);
	unsigned int count2(m_grib2.find_layers().size() - count1);
	if (true)
		std::cout << "Loaded " << count1 << " GRIB2 Layers from " << path1
			  << " and " << count2 << " GRIB2 Layers from " << path2 << std::endl;
	unsigned int count3(m_grib2.expire_cache());
	if (true && count3)
		std::cout << "Expired " << count3 << " cached GRIB2 Layers" << std::endl;
}

void Engine::load_bitmapmaps(std::string path)
{
	m_bitmapmaps.parse(path);
	unsigned int count(m_bitmapmaps.size());
	m_bitmapmaps.sort();
	m_bitmapmaps.check_files();
	if (true)
		std::cout << "Loaded " << count << " Bitmap Maps Layers from " << path << std::endl;
}

void Engine::load_bitmapmaps_2(std::string path1, std::string path2)
{
	m_bitmapmaps.parse(path1);
	unsigned int count1(m_bitmapmaps.size());
	m_bitmapmaps.parse(path2);
	unsigned int count2(m_bitmapmaps.size() - count1);
	m_bitmapmaps.sort();
	m_bitmapmaps.check_files();
	if (true)
		std::cout << "Loaded " << count1 << " Bitmap Maps from " << path1
			  << " and " << count2 << " Bitmap Maps from " << path2 << std::endl;
}

void Engine::update_bitmapmaps(bool synchronous)
{
	std::string path1(get_dir_main());
	if (path1.empty())
		path1 = FPlan::get_userdbpath();
	path1 = Glib::build_filename(path1, "bitmapmaps");
	std::string path2(get_dir_aux());
	if (path2.empty()) {
		if (synchronous)
			load_bitmapmaps(path1);
		else
			Glib::Thread::create(sigc::bind(sigc::mem_fun(*this, &Engine::load_bitmapmaps), path1),
					     0, false, true, Glib::THREAD_PRIORITY_NORMAL);
	} else {
		path2 = Glib::build_filename(path2, "bitmapmaps");
		if (synchronous)
			load_bitmapmaps_2(path1, path2);
		else
			Glib::Thread::create(sigc::bind(sigc::bind(sigc::mem_fun(*this, &Engine::load_bitmapmaps_2), path2), path1),
					     0, false, true, Glib::THREAD_PRIORITY_NORMAL);
	}
}

#ifdef HAVE_PILOTLINK
PalmDbBase::PalmStringCompare::comp_t Engine::to_palm_compare(DbQueryInterfaceCommon::comp_t comp)
{
	switch (comp) {
	default:
	case DbQueryInterfaceCommon::comp_startswith:
		return PalmDbBase::PalmStringCompare::comp_startswith;

	case DbQueryInterfaceCommon::comp_contains:
		return PalmDbBase::PalmStringCompare::comp_contains;

	case DbQueryInterfaceCommon::comp_exact:
		return PalmDbBase::PalmStringCompare::comp_exact;
	}
}
#endif

Engine::DbThread::DbThread(const std::string & dir_main, const std::string & dir_aux, bool pg)
	: m_terminate(false), m_thread(0)
{
#ifdef HAVE_PQXX
	m_pgconn.reset();
	if (pg)
		m_pgconn = pgconn_t(new pqxx::lazyconnection(dir_main));
#endif
	std::cerr << "Database directories: main " << dir_main << " aux " << dir_aux << std::endl;
	try {
#ifdef HAVE_PQXX
		if (pg) {
			m_airportdb.reset(new AirportsPGDb(*m_pgconn, AirportsPGDb::read_only));
		} else
#endif
		{
			m_airportdb.reset(new AirportsDb());
			AirportsDb *db(static_cast<AirportsDb *>(m_airportdb.get()));
			db->open_readonly(dir_main);
			if (!dir_aux.empty() && db->is_dbfile_exists(dir_aux))
				db->attach_readonly(dir_aux);
			if (false && db->is_aux_attached())
				std::cerr << "Auxillary airports database attached" << std::endl;
		}
	} catch (const std::exception& e) {
		std::cerr << "Error opening airports database: " << e.what() << std::endl;
	}
	try {
#ifdef HAVE_PQXX
		if (pg) {
			m_navaiddb.reset(new NavaidsPGDb(*m_pgconn, NavaidsPGDb::read_only));
		} else
#endif
		{
			m_navaiddb.reset(new NavaidsDb());
			NavaidsDb *db(static_cast<NavaidsDb *>(m_navaiddb.get()));
			db->open_readonly(dir_main);
			if (!dir_aux.empty() && db->is_dbfile_exists(dir_aux))
				db->attach_readonly(dir_aux);
			if (false && db->is_aux_attached())
				std::cerr << "Auxillary navaids database attached" << std::endl;
		}
	} catch (const std::exception& e) {
		std::cerr << "Error opening navaids database: " << e.what() << std::endl;
	}
	try {
#ifdef HAVE_PQXX
		if (pg) {
			m_waypointdb.reset(new WaypointsPGDb(*m_pgconn, WaypointsPGDb::read_only));
		} else
#endif
		{
			m_waypointdb.reset(new WaypointsDb());
			WaypointsDb *db(static_cast<WaypointsDb *>(m_waypointdb.get()));
			db->open_readonly(dir_main);
			if (!dir_aux.empty() && db->is_dbfile_exists(dir_aux))
				db->attach_readonly(dir_aux);
			if (false && db->is_aux_attached())
				std::cerr << "Auxillary waypoints database attached" << std::endl;
		}
	} catch (const std::exception& e) {
		std::cerr << "Error opening waypoints database: " << e.what() << std::endl;
	}
	try {
#ifdef HAVE_PQXX
		if (pg) {
			m_airspacedb.reset(new AirspacesPGDb(*m_pgconn, AirspacesPGDb::read_only));
		} else
#endif
		{
			m_airspacedb.reset(new AirspacesDb());
			AirspacesDb *db(static_cast<AirspacesDb *>(m_airspacedb.get()));
			db->open_readonly(dir_main);
			if (!dir_aux.empty() && db->is_dbfile_exists(dir_aux))
				db->attach_readonly(dir_aux);
			if (false && db->is_aux_attached())
				std::cerr << "Auxillary airspaces database attached" << std::endl;
		}
	} catch (const std::exception& e) {
		std::cerr << "Error opening airspaces database: " << e.what() << std::endl;
	}
	try {
#ifdef HAVE_PQXX
		if (pg) {
			m_airwaydb.reset(new AirwaysPGDb(*m_pgconn, AirwaysPGDb::read_only));
		} else
#endif
		{
			m_airwaydb.reset(new AirwaysDb());
			AirwaysDb *db(static_cast<AirwaysDb *>(m_airwaydb.get()));
			db->open_readonly(dir_main);
			if (!dir_aux.empty() && db->is_dbfile_exists(dir_aux))
				db->attach_readonly(dir_aux);
			if (false && db->is_aux_attached())
				std::cerr << "Auxillary airways database attached" << std::endl;
		}
	} catch (const std::exception& e) {
		std::cerr << "Error opening airways database: " << e.what() << std::endl;
	}
	try {
#ifdef HAVE_PQXX
		if (pg) {
			m_mapelementdb.reset(new MapelementsPGDb(*m_pgconn, MapelementsPGDb::read_only));
		} else
#endif
		{
			m_mapelementdb.reset(new MapelementsDb());
			MapelementsDb *db(static_cast<MapelementsDb *>(m_mapelementdb.get()));
			db->open_readonly(dir_aux.empty() ? get_default_aux_dir() : dir_aux);
		}
	} catch (const std::exception& e) {
		std::cerr << "Error opening mapelements database: " << e.what() << std::endl;
	}
	try {
		m_topodb.open(dir_aux.empty() ? get_default_aux_dir() : dir_aux);
	} catch (const std::exception& e) {
		std::cerr << "Error opening topo database: " << e.what() << std::endl;
	}
#ifdef HAVE_PILOTLINK
	try {
		m_palmairportdb.open(Glib::build_filename(get_default_aux_dir(), "AirportsDB.pdb"));
	} catch (const std::exception& e) {
		if (false)
			std::cerr << "Error opening palm airports database: " << e.what() << std::endl;
	}
	try {
		m_palmnavaiddb.open(Glib::build_filename(get_default_aux_dir(), "NavaidsDB.pdb"));
	} catch (const std::exception& e) {
		if (false)
			std::cerr << "Error opening palm navaids database: " << e.what() << std::endl;
	}
	try {
		m_palmwaypointdb.open(Glib::build_filename(get_default_aux_dir(), "WaypointsDB.pdb"));
	} catch (const std::exception& e) {
		if (false)
			std::cerr << "Error opening palm waypoints database: " << e.what() << std::endl;
	}
	try {
		m_palmairspacedb.open(Glib::build_filename(get_default_aux_dir(), "AirspaceDB.pdb"));
	} catch (const std::exception& e) {
		if (false)
			std::cerr << "Error opening palm airspace database: " << e.what() << std::endl;
	}
	try {
		m_palmmapelementdb.open(Glib::build_filename(get_default_aux_dir(), "MapelementDB.pdb"));
	} catch (const std::exception& e) {
		if (false)
			std::cerr << "Error opening palm map elements database: " << e.what() << std::endl;
	}
	try {
		m_palmgtopodb.open(Glib::build_filename(get_default_aux_dir(), "GTopoDB.pdb"));
	} catch (const std::exception& e) {
		if (false)
			std::cerr << "Error opening palm gtopo database: " << e.what() << std::endl;
	}
#endif
	m_thread = Glib::Thread::create(sigc::mem_fun(*this, &Engine::DbThread::thread), 0, true, true, Glib::THREAD_PRIORITY_NORMAL);
}

Engine::DbThread::~DbThread()
{
	if (m_thread) {
		m_terminate = true;
		m_cond.broadcast();
		m_thread->join();
		m_thread = 0;
	}
}

void Engine::DbThread::thread(void)
{
	m_mutex.lock();
	while (!m_terminate) {
		if (m_tasklist.empty()) {
			m_cond.wait(m_mutex);
			if (m_tasklist.empty())
				continue;
		}
		Glib::RefPtr<ResultBase> task(m_tasklist.front());
		m_tasklist.pop_front();
		m_mutex.unlock();
		if (!task->is_error())
			task->execute();
		m_mutex.lock();
	}
	while (!m_tasklist.empty()) {
		Glib::RefPtr<ResultBase> task(m_tasklist.front());
		m_tasklist.pop_front();
		m_mutex.unlock();
		task->seterror();
		m_mutex.lock();
	}
	m_mutex.unlock();
}

template<class T> void Engine::DbThread::queue(Glib::RefPtr<T>& p)
{
	if (!p)
		return;
	if (m_terminate) {
		p->seterror();
		return;
	}
	{
		Glib::Mutex::Lock lock(m_mutex);
		m_tasklist.push_back(p);
		m_cond.broadcast();
	}
}

Engine::DbThread::ResultBase::ResultBase(const sigc::slot<void> & dbaction, const sigc::slot<void>& dbcancel)
	: m_dbcancel(dbcancel), m_dbaction(dbaction), m_refcount(1), m_done(false), m_error(false), m_executing(false)
{
#ifdef HAVE_SYS_SYSCALL_H
	//std::cerr << "Engine::DbThread::ResultBase::ResultBase " << (unsigned long)this << " TID " << syscall(SYS_gettid) << std::endl;
#endif
}

Engine::DbThread::ResultBase::~ResultBase()
{
#ifdef HAVE_SYS_SYSCALL_H
	//std::cerr << "Engine::DbThread::ResultBase::~ResultBase " << (unsigned long)this << " TID " << syscall(SYS_gettid) << std::endl;
#endif
}

void Engine::DbThread::ResultBase::reference(void) const
{
	g_atomic_int_inc(&m_refcount);
}

void Engine::DbThread::ResultBase::unreference(void) const
{
	if (!g_atomic_int_dec_and_test(&m_refcount))
		return;
	const_cast<ResultBase *>(this)->disconnect_all();
	delete this;
}

sigc::connection Engine::DbThread::ResultBase::connect(const sigc::signal< void >::slot_type & slot)
{
	sigc::connection conn;
	bool done;
	{
		Glib::Mutex::Lock lock(m_mutex);
		done = m_done;
		conn = m_donecb.connect(slot);
	}
	if (done)
		slot();
	return conn;
}

void Engine::DbThread::ResultBase::disconnect_all(void)
{
	Glib::Mutex::Lock lock(m_mutex);
	m_donecb.clear();
}

void Engine::DbThread::ResultBase::cancel(void)
{
	Glib::Mutex::Lock lock(m_mutex);
	m_done = true;
	m_error = true;
	if (m_executing)
		m_dbcancel();
}

template<class T> void Engine::DbThread::ResultBase::cancel(Glib::RefPtr<T> & q)
{
	if (q) {
		{
			Glib::Mutex::Lock lock(q->m_mutex);
			q->m_donecb.clear();
			if (!q->is_done()) {
				q->m_done = true;
				q->m_error = true;
				if (q->m_executing)
					q->m_dbcancel();
			}
		}
		q = Glib::RefPtr<T>();
	}
}

template void Engine::DbThread::ResultBase::cancel(Glib::RefPtr<AirportResult> & q);
template void Engine::DbThread::ResultBase::cancel(Glib::RefPtr<AirspaceResult> & q);
template void Engine::DbThread::ResultBase::cancel(Glib::RefPtr<NavaidResult> & q);
template void Engine::DbThread::ResultBase::cancel(Glib::RefPtr<WaypointResult> & q);
template void Engine::DbThread::ResultBase::cancel(Glib::RefPtr<AirwayResult> & q);
template void Engine::DbThread::ResultBase::cancel(Glib::RefPtr<MapelementResult> & q);
template void Engine::DbThread::ResultBase::cancel(Glib::RefPtr<ElevationResult> & q);
template void Engine::DbThread::ResultBase::cancel(Glib::RefPtr<ElevationMinMaxResult> & q);
template void Engine::DbThread::ResultBase::cancel(Glib::RefPtr<ElevationProfileResult> & q);
template void Engine::DbThread::ResultBase::cancel(Glib::RefPtr<ElevationRouteProfileResult> & q);
template void Engine::DbThread::ResultBase::cancel(Glib::RefPtr<ElevationMapResult> & q);
template void Engine::DbThread::ResultBase::cancel(Glib::RefPtr<ElevationMapCairoResult> & q);
#ifdef HAVE_BOOST
template void Engine::DbThread::ResultBase::cancel(Glib::RefPtr<AirwayGraphResult> & q);
template void Engine::DbThread::ResultBase::cancel(Glib::RefPtr<AreaGraphResult> & q);
#endif

void Engine::DbThread::ResultBase::execute(void)
{
	//std::cerr << "Engine::DbThread::ResultBase::execute " << (unsigned long)this << std::endl;
	if (!m_error) {
		m_executing = true;
		try {
			m_dbaction();
		} catch (const std::exception& e) {
			std::cerr << "DbThread::execute: exception " << e.what() << std::endl;
			m_error = true;
		}
	}
	m_executing = false;
	//std::cerr << "Engine::DbThread::ResultBase::execute done " << (unsigned long)this << std::endl;
	Glib::Mutex::Lock lock(m_mutex);
	m_done = true;
	m_donecb();
}

void Engine::DbThread::ResultBase::seterror(void)
{
	//std::cerr << "Engine::DbThread::ResultBase::seterror " << (unsigned long)this << std::endl;
	Glib::Mutex::Lock lock(m_mutex);
	m_done = true;
	m_error = true;
	if (m_executing)
		m_dbcancel();
}

template<class Db> Engine::DbThread::Result<Db>::Result(const sigc::slot<elementvector_t> & dbaction, const sigc::slot<void>& dbcancel)
	: ResultBase(sigc::compose(sigc::mem_fun(*this, &Engine::DbThread::Result<Db>::set_result), dbaction), dbcancel)
{
}

template<class Db> Engine::DbThread::Result<Db>::~Result()
{
}

template<class Db> Engine::DbThread::ResultObject<Db>::ResultObject(const sigc::slot<elementvector_t>& dbaction, const sigc::slot<void>& dbcancel)
	: Result<Db>(dbaction, dbcancel)
{
}

template <class Db> typename Engine::DbThread::ResultObject<Db>::objects_t Engine::DbThread::ResultObject<Db>::get_objects(void) const
{
	const elementvector_t& ev(get_result());
	objects_t o;
	for (typename elementvector_t::const_iterator i(ev.begin()), e(ev.end()); i != e; ++i)
		o.push_back(DbObject::create(*i));
	return o;
}

template class Engine::DbThread::ResultObject<NavaidsDb>;
template class Engine::DbThread::ResultObject<WaypointsDb>;
template class Engine::DbThread::Result<AirwaysDb>;
template class Engine::DbThread::ResultObject<AirportsDb>;
template class Engine::DbThread::Result<AirspacesDb>;
template class Engine::DbThread::ResultObject<MapelementsDb>;


Engine::DbThread::ElevationResult::ElevationResult(const Point & pt, TopoDb30 & topodb)
	: ResultBase(sigc::bind(sigc::mem_fun(*this, &Engine::DbThread::ElevationResult::execute), sigc::ref(topodb), pt), sigc::mem_fun(topodb, &TopoDb30::interrupt)),
	  m_result(0)
{
}

#ifdef HAVE_PILOTLINK
Engine::DbThread::ElevationResult::ElevationResult(const Point & pt, PalmGtopo & topodb)
	: ResultBase(sigc::bind(sigc::mem_fun(*this, &Engine::DbThread::ElevationResult::execute_palm), sigc::ref(topodb), pt)),
	  m_result(0)
{
}
#endif

Engine::DbThread::ElevationResult::~ElevationResult()
{
}

void Engine::DbThread::ElevationResult::execute(TopoDb30& topodb, const Point& pt)
{
	m_result = topodb.get_elev(pt);
}

#ifdef HAVE_PILOTLINK
void Engine::DbThread::ElevationResult::execute_palm(PalmGtopo& topodb, const Point& pt)
{
	m_result = topodb[pt];
}
#endif

Engine::DbThread::ElevationMinMaxResult::ElevationMinMaxResult(const Rect& r, TopoDb30& topodb)
	: ResultBase(sigc::bind(sigc::mem_fun(*this, &Engine::DbThread::ElevationMinMaxResult::execute_rect), sigc::ref(topodb), r), sigc::mem_fun(topodb, &TopoDb30::interrupt)),
	  m_result(TopoDb30::nodata, TopoDb30::nodata)
{
}

Engine::DbThread::ElevationMinMaxResult::ElevationMinMaxResult(const PolygonSimple& p, TopoDb30& topodb)
	: ResultBase(sigc::bind(sigc::mem_fun(*this, &Engine::DbThread::ElevationMinMaxResult::execute_poly), sigc::ref(topodb), p), sigc::mem_fun(topodb, &TopoDb30::interrupt)),
	  m_result(TopoDb30::nodata, TopoDb30::nodata)
{
}

Engine::DbThread::ElevationMinMaxResult::ElevationMinMaxResult(const PolygonHole& p, TopoDb30& topodb)
	: ResultBase(sigc::bind(sigc::mem_fun(*this, &Engine::DbThread::ElevationMinMaxResult::execute_polyhole), sigc::ref(topodb), p), sigc::mem_fun(topodb, &TopoDb30::interrupt)),
	  m_result(TopoDb30::nodata, TopoDb30::nodata)
{
}

Engine::DbThread::ElevationMinMaxResult::ElevationMinMaxResult(const MultiPolygonHole& p, TopoDb30& topodb)
	: ResultBase(sigc::bind(sigc::mem_fun(*this, &Engine::DbThread::ElevationMinMaxResult::execute_multipolyhole), sigc::ref(topodb), p), sigc::mem_fun(topodb, &TopoDb30::interrupt)),
	  m_result(TopoDb30::nodata, TopoDb30::nodata)
{
}

#ifdef HAVE_PILOTLINK
Engine::DbThread::ElevationMinMaxResult::ElevationMinMaxResult(const Rect& r, PalmGtopo& topodb)
	: ResultBase(sigc::bind(sigc::mem_fun(*this, &Engine::DbThread::ElevationMinMaxResult::execute_rect_palm), sigc::ref(topodb), r)),
	  m_result(TopoDb30::nodata, TopoDb30::nodata)
{
}

Engine::DbThread::ElevationMinMaxResult::ElevationMinMaxResult(const PolygonSimple& p, PalmGtopo& topodb)
	: ResultBase(sigc::bind(sigc::mem_fun(*this, &Engine::DbThread::ElevationMinMaxResult::execute_poly_palm), sigc::ref(topodb), p)),
	  m_result(TopoDb30::nodata, TopoDb30::nodata)
{
}

Engine::DbThread::ElevationMinMaxResult::ElevationMinMaxResult(const PolygonHole& p, PalmGtopo& topodb)
	: ResultBase(sigc::bind(sigc::mem_fun(*this, &Engine::DbThread::ElevationMinMaxResult::execute_polyhole_palm), sigc::ref(topodb), p)),
	  m_result(TopoDb30::nodata, TopoDb30::nodata)
{
}

Engine::DbThread::ElevationMinMaxResult::ElevationMinMaxResult(const MultiPolygonHole& p, PalmGtopo& topodb)
	: ResultBase(sigc::bind(sigc::mem_fun(*this, &Engine::DbThread::ElevationMinMaxResult::execute_multipolyhole_palm), sigc::ref(topodb), p)),
	  m_result(TopoDb30::nodata, TopoDb30::nodata)
{
}
#endif

Engine::DbThread::ElevationMinMaxResult::~ElevationMinMaxResult()
{
}

#ifdef HAVE_PILOTLINK
void Engine::DbThread::ElevationMinMaxResult::execute_poly_palm(PalmGtopo& topodb, const PolygonSimple& p)
{
	m_result = topodb.get_minmax_elev(p);
}

void Engine::DbThread::ElevationMinMaxResult::execute_polyhole_palm(PalmGtopo& topodb, const PolygonHole& p)
{
	m_result = topodb.get_minmax_elev(p);
}

void Engine::DbThread::ElevationMinMaxResult::execute_multipolyhole_palm(PalmGtopo& topodb, const MultiPolygonHole& p)
{
	m_result = topodb.get_minmax_elev(p);
}

void Engine::DbThread::ElevationMinMaxResult::execute_rect_palm(PalmGtopo& topodb, const Rect& r)
{
	m_result = topodb.get_minmax_elev(r);
}
#endif

void Engine::DbThread::ElevationMinMaxResult::execute_poly(TopoDb30& topodb, const PolygonSimple& p)
{
	m_result = topodb.get_minmax_elev(p);
}

void Engine::DbThread::ElevationMinMaxResult::execute_polyhole(TopoDb30& topodb, const PolygonHole& p)
{
	m_result = topodb.get_minmax_elev(p);
}

void Engine::DbThread::ElevationMinMaxResult::execute_multipolyhole(TopoDb30& topodb, const MultiPolygonHole& p)
{
	m_result = topodb.get_minmax_elev(p);
}

void Engine::DbThread::ElevationMinMaxResult::execute_rect(TopoDb30& topodb, const Rect& r)
{
	m_result = topodb.get_minmax_elev(r);
}

Engine::DbThread::ElevationProfileResult::ElevationProfileResult(const Point& p0, const Point& p1, double corridor_nmi, TopoDb30& topodb)
	: ResultBase(sigc::bind(sigc::mem_fun(*this, &Engine::DbThread::ElevationProfileResult::execute), sigc::ref(topodb), p0, p1, corridor_nmi),
		     sigc::mem_fun(topodb, &TopoDb30::interrupt))
{
}

#ifdef HAVE_PILOTLINK
Engine::DbThread::ElevationProfileResult::ElevationProfileResult(const Point& p0, const Point& p1, double corridor_nmi, PalmGtopo& topodb)
	: ResultBase(sigc::bind(sigc::mem_fun(*this, &Engine::DbThread::ElevationProfileResult::execute_palm), sigc::ref(topodb), p0, p1, corridor_nmi))
{
}
#endif

Engine::DbThread::ElevationProfileResult::~ElevationProfileResult()
{
}

void Engine::DbThread::ElevationProfileResult::execute(TopoDb30& topodb, const Point& p0, const Point& p1, double corridor_nmi)
{
	m_result = topodb.get_profile(p0, p1, corridor_nmi);
	if (m_result.empty())
		seterror();
}

#ifdef HAVE_PILOTLINK
void Engine::DbThread::ElevationProfileResult::execute_palm(PalmGtopo& topodb, const Point& p0, const Point& p1, double corridor_nmi)
{
	m_result = get_profile(topodb, p0, p1, corridor_nmi);
	if (m_result.empty())
		seterror();
}

Engine::DbThread::ElevationProfileResult::Profile Engine::DbThread::ElevationProfileResult::get_profile(PalmGtopo& topodb, const Point& p0, const Point& p1, double corridor_nmi)
{
	Profile res;
	if (p0.is_invalid() || p1.is_invalid() || corridor_nmi < 0 || corridor_nmi > 10)
		return Profile();
	double gcdist(p0.spheric_distance_nmi_dbl(p1));
	if (gcdist > 1000)
		return Profile();
	PalmGtopo::GtopoCoord l0(p0), l1(p1), c0, c1;
	// corridor line coordinates
	{
		double crs(p0.spheric_true_course_dbl(p1));
		Point px0(p0.spheric_course_distance_nmi(crs + 90, corridor_nmi));
		Point px1(p0 + (p0 - px0));
		if (px0.is_invalid() || px1.is_invalid())
			return Profile();
		c0 = PalmGtopo::GtopoCoord(px0);
		c1 = PalmGtopo::GtopoCoord(px1);
	}
	// line step distance map
	double distmap[4];
	distmap[0] = 0;
	{
		Point ptsz(PalmGtopo::GtopoCoord::get_pointsize());
		//Point p(p0.halfway(p1));
		Point p(p0.get_gcnav(p1, 0.5).first);
		for (unsigned int i = 1; i < 4; ++i) {
			Point p1((i & 1) ? ptsz.get_lon() : 0, (i & 2) ? ptsz.get_lat() : 0);
			p1 += p;
			distmap[i] = p.spheric_distance_nmi_dbl(p1);
		}
	}
	int ldx(l0.lon_dist_signed(l1)), ldy(l0.lat_dist_signed(l1));
	bool lsx(ldx < 0), lsy(ldy < 0);
	ldx = abs(ldx);
	ldy = abs(ldy);
	int cdx(c0.lon_dist_signed(c1)), cdy(c0.lat_dist_signed(c1));
	bool csx(cdx < 0), csy(cdy < 0);
	cdx = abs(cdx);
	cdy = abs(cdy);
	double dist(0);
	int lerr(ldx - ldy);
	PalmGtopo::GtopoCoord ll(l0);
	for (;;) {
		PalmGtopo::elevation_t elev(topodb[ll]);
		if (elev == PalmGtopo::nodata)
			elev = TopoDb30::nodata;
		res.push_back(ProfilePoint(dist, elev));
		int cerr(cdx - cdy);
		PalmGtopo::GtopoCoord cc(c0);
		for (;;) {
			PalmGtopo::elevation_t elev(topodb[cc]);
			if (elev == PalmGtopo::nodata)
				elev = TopoDb30::nodata;
			res.back().add(elev);
			if (cc == c1)
				break;
			int cerr2(2 * cerr);
			if (cerr2 > -cdy) {
				cerr -= cdy;
				if (csx)
					cc -= PalmGtopo::GtopoCoord(1, 0);
				else
					cc += PalmGtopo::GtopoCoord(1, 0);
			}
			if (cerr2 < cdx) {
				cerr += cdx;
				if (csy)
					cc -= PalmGtopo::GtopoCoord(0, 1);
				else
					cc += PalmGtopo::GtopoCoord(0, 1);
			}
		}
		if (ll == l1)
			break;
		int lerr2(2 * lerr);
		unsigned int dmi(0);
		if (lerr2 > -ldy) {
			lerr -= ldy;
			if (lsx)
				ll -= PalmGtopo::GtopoCoord(1, 0);
			else
				ll += PalmGtopo::GtopoCoord(1, 0);
			dmi |= 1U;
		}
		if (lerr2 < ldx) {
			lerr += ldx;
			if (lsy)
				ll -= PalmGtopo::GtopoCoord(0, 1);
			else
				ll += PalmGtopo::GtopoCoord(0, 1);
			dmi |= 2U;
		}
		dist += distmap[dmi];
	}
	if (dist > 0)
		res.scaledist(gcdist / dist);
	return res;
}
#endif

Engine::DbThread::ElevationRouteProfileResult::ElevationRouteProfileResult(const FPlanRoute& fpl, double corridor_nmi, TopoDb30& topodb)
	: ResultBase(sigc::bind(sigc::mem_fun(*this, &Engine::DbThread::ElevationRouteProfileResult::execute), sigc::ref(topodb), fpl, corridor_nmi),
		     sigc::mem_fun(topodb, &TopoDb30::interrupt))
{
}

#ifdef HAVE_PILOTLINK
Engine::DbThread::ElevationRouteProfileResult::ElevationRouteProfileResult(const FPlanRoute& fpl, double corridor_nmi, PalmGtopo& topodb)
	: ResultBase(sigc::bind(sigc::mem_fun(*this, &Engine::DbThread::ElevationRouteProfileResult::execute_palm), sigc::ref(topodb), fpl, corridor_nmi))
{
}
#endif

Engine::DbThread::ElevationRouteProfileResult::~ElevationRouteProfileResult()
{
}

void Engine::DbThread::ElevationRouteProfileResult::execute(TopoDb30& topodb, const FPlanRoute& fpl, double corridor_nmi)
{
	m_result = topodb.get_profile(fpl, corridor_nmi);
	if (m_result.empty())
		seterror();
}

#ifdef HAVE_PILOTLINK
void Engine::DbThread::ElevationRouteProfileResult::execute_palm(PalmGtopo& topodb, const FPlanRoute& fpl, double corridor_nmi)
{
	m_result = get_profile(topodb, fpl, corridor_nmi);
	if (m_result.empty())
		seterror();
}

Engine::DbThread::ElevationRouteProfileResult::RouteProfile Engine::DbThread::ElevationRouteProfileResult::get_profile(PalmGtopo& topodb, const FPlanRoute& fpl, double corridor_nmi)
{
	const unsigned int nrwpt(fpl.get_nrwpt());
	if (nrwpt < 2)
		return RouteProfile();
	RouteProfile rp;
	double cumdist(0);
	for (unsigned int nr = 1;;) {
		const FPlanWaypoint& wpt0(fpl[nr - 1]), wpt1(fpl[nr]);
		if (wpt0.get_coord().is_invalid() || wpt1.get_coord().is_invalid())
			return RouteProfile();
		TopoDb30::Profile p(ElevationProfileResult::get_profile(topodb, wpt0.get_coord(), wpt1.get_coord(), corridor_nmi));
		TopoDb30::Profile::const_iterator pi(p.begin()), pe(p.end());
		double legdist(wpt0.get_coord().spheric_distance_nmi_dbl(wpt1.get_coord()));
		for (; pi != pe && pi->get_dist() < legdist; ++pi)
			rp.push_back(RouteProfilePoint(*pi, nr - 1, cumdist));
		cumdist += legdist;
		++nr;
		if (nr < nrwpt)
			continue;
		for (; pi != pe && pi->get_dist() >= legdist; ++pi)
			rp.push_back(RouteProfilePoint(ElevationProfileResult::ProfilePoint(pi->get_dist() - legdist, pi->get_elev(), pi->get_minelev(), pi->get_maxelev()), nr - 1, cumdist));
		break;
	}
	return rp;
}
#endif

Engine::DbThread::ElevationMapResult::ElevationMapResult(const Rect & bbox, TopoDb30 & topodb)
	: ResultBase(sigc::bind(sigc::mem_fun(*this, &Engine::DbThread::ElevationMapResult::execute), sigc::ref(topodb)), sigc::mem_fun(topodb, &TopoDb30::interrupt)),
	  m_bbox(bbox), m_width(0), m_height(0), m_elev(0)
{
}

#ifdef HAVE_PILOTLINK
Engine::DbThread::ElevationMapResult::ElevationMapResult(const Rect & bbox, PalmGtopo & topodb)
	: ResultBase(sigc::bind(sigc::mem_fun(*this, &Engine::DbThread::ElevationMapResult::execute_palm), sigc::ref(topodb))),
	  m_bbox(bbox), m_width(0), m_height(0), m_elev(0)
{
}
#endif

Engine::DbThread::ElevationMapResult::~ElevationMapResult()
{
	delete[] m_elev;
}

TopoDb30::elev_t Engine::DbThread::ElevationMapResult::operator()(unsigned int x, unsigned int y)
{
	if (!m_elev || x >= m_width || y >= m_height)
		return TopoDb30::nodata;
	return m_elev[y * m_width + x];
}

void Engine::DbThread::ElevationMapResult::execute(TopoDb30& topodb)
{
	TopoDb30::TopoCoordinate gmin(m_bbox.get_southwest()), gmax(m_bbox.get_northeast());
	gmin.advance_west();
	gmin.advance_south();
	gmax.advance_east();
	gmax.advance_north();
	Point pixeldim(TopoDb30::TopoCoordinate::get_pointsize());
	std::cerr << "TopoDb: from " << gmin << " to " << gmax << std::endl;
	unsigned int pbufx(gmin.lon_dist(gmax)+1);
	unsigned int pbufy(gmin.lat_dist(gmax)+1);
	delete[] m_elev;
	m_elev = 0;
	m_width = 0;
	m_height = 0;
	if ((pbufx * pbufy) > (2048 * 2048)) {
		seterror();
		return;
	}
	m_elev = new TopoDb30::elev_t[pbufx * pbufy];
	if (!m_elev) {
		seterror();
		return;
	}
	m_width = pbufx;
	m_height = pbufy;
	for (unsigned int i = 0; i < pbufx * pbufy; i++)
		m_elev[i] = 0x5555;
	TopoDb30::TopoTileCoordinate tmin(gmin), tmax(gmax);
	unsigned int pboffsx(0), pboffsy(0);
	for (TopoDb30::TopoTileCoordinate tc(tmin);;) {
		TopoDb30::pixel_index_t ymin((tc.get_lat_tile() == tmin.get_lat_tile()) ? tmin.get_lat_offs() : 0);
		TopoDb30::pixel_index_t ymax((tc.get_lat_tile() == tmax.get_lat_tile()) ? tmax.get_lat_offs() : TopoDb30::tile_size - 1);
		TopoDb30::pixel_index_t xmin((tc.get_lon_tile() == tmin.get_lon_tile()) ? tmin.get_lon_offs() : 0);
		TopoDb30::pixel_index_t xmax((tc.get_lon_tile() == tmax.get_lon_tile()) ? tmax.get_lon_offs() : TopoDb30::tile_size - 1);
		xmax -= xmin;
		ymax -= ymin;
		for (TopoDb30::pixel_index_t y(0); y <= ymax; y++) {
			tc.set_lat_offs(ymin + y);
			for (TopoDb30::pixel_index_t x(0); x <= xmax; x++) {
				tc.set_lon_offs(xmin + x);
				TopoDb30::elev_t elev(topodb.get_elev(tc));
				//std::cerr << "TopoDb: point " << (TopoDb30::TopoCoordinate)tc << ' ' << (Point)(TopoDb30::TopoCoordinate)tc << " elev " << elev << std::endl;
				unsigned int xx(pboffsx + x), yy(pbufy - 1 - pboffsy - y);
				if (xx < m_width && yy < m_height)
					m_elev[yy * m_width + xx] = elev;
			}
		}
		if (tc.get_lon_tile() != tmax.get_lon_tile()) {
			tc.advance_tile_east();
			pboffsx += xmax + 1;
			continue;
		}
		if (tc.get_lat_tile() == tmax.get_lat_tile())
			break;
		tc.set_lon_tile(tmin.get_lon_tile());
		pboffsx = 0;
		tc.advance_tile_north();
		pboffsy += ymax + 1;
	}
	m_bbox = Rect((TopoDb30::TopoCoordinate)tmin, pixeldim + (TopoDb30::TopoCoordinate)tmax);
}

#ifdef HAVE_PILOTLINK
void Engine::DbThread::ElevationMapResult::execute_palm(PalmGtopo& topodb)
{
	PalmGtopo::GtopoCoord gmin(m_bbox.get_southwest()), gmax(m_bbox.get_northeast());
	gmin -= PalmGtopo::GtopoCoord(1, 1);
	gmax += PalmGtopo::GtopoCoord(1, 1);
	std::cerr << "Gtopo: from " << gmin << " to " << gmax << std::endl;
	unsigned int pbufx((gmax-gmin).get_lon()+1);
	unsigned int pbufy((gmax-gmin).get_lat()+1);
	delete[] m_elev;
	m_elev = 0;
	m_width = 0;
	m_height = 0;
	if ((pbufx * pbufy) > (1024 * 1024)) {
		seterror();
		return;
	}
	m_elev = new TopoDb30::elev_t[pbufx * pbufy];
	if (!m_elev) {
		seterror();
		return;
	}
	m_width = pbufx;
	m_height = pbufy;
	for (PalmGtopo::GtopoCoord gp(gmin);;) {
		//std::cerr << "Gtopo: point " << gp << std::endl;
		PalmGtopo::elevation_t elev(topodb[gp]);
		PalmGtopo::GtopoCoord gidx(gp - gmin);
		unsigned int xx(gidx.get_lon()), yy(pbufy - 1 - gidx.get_lat());
		if (xx < m_width && yy < m_height)
			m_elev[yy * m_width + xx] = elev;
		if (gp == gmax)
			break;
		if (gp.get_lon() != gmax.get_lon()) {
			gp.set_lon(gp.get_lon()+1);
			continue;
		}
		gp.set_lon(gmin.get_lon());
		gp.set_lat(gp.get_lat()+1);
	}
	m_bbox = Rect(gmin, gmax + PalmGtopo::GtopoCoord(1, 1));
}
#endif

Engine::DbThread::ElevationMapCairoResult::ElevationMapCairoResult(const Rect & bbox, TopoDb30 & topodb)
	: ResultBase(sigc::bind(sigc::mem_fun(*this, &Engine::DbThread::ElevationMapCairoResult::execute), sigc::ref(topodb)), sigc::mem_fun(topodb, &TopoDb30::interrupt)),
	  m_bbox(bbox)
{
}

#ifdef HAVE_PILOTLINK
Engine::DbThread::ElevationMapCairoResult::ElevationMapCairoResult(const Rect & bbox, PalmGtopo & topodb)
	: ResultBase(sigc::bind(sigc::mem_fun(*this, &Engine::DbThread::ElevationMapCairoResult::execute_palm), sigc::ref(topodb))),
	  m_bbox(bbox)
{
}
#endif

Engine::DbThread::ElevationMapCairoResult::~ElevationMapCairoResult()
{
}

void Engine::DbThread::ElevationMapCairoResult::execute(TopoDb30& topodb)
{
	TopoDb30::TopoCoordinate gmin(m_bbox.get_southwest()), gmax(m_bbox.get_northeast());
	gmin.advance_west();
	gmin.advance_south();
	gmax.advance_east();
	gmax.advance_north();
	Point pixeldim(TopoDb30::TopoCoordinate::get_pointsize());
	std::cerr << "TopoDb: from " << gmin << " to " << gmax << std::endl;
	unsigned int pbufx(gmin.lon_dist(gmax)+1);
	unsigned int pbufy(gmin.lat_dist(gmax)+1);
	m_surface = Cairo::ImageSurface::create(Cairo::FORMAT_RGB24, pbufx, pbufy);
	unsigned char *pbuf_data(m_surface->get_data());
	int pbuf_stride(m_surface->get_stride());
	TopoDb30::TopoTileCoordinate tmin(gmin), tmax(gmax);
	unsigned int pboffsx(0), pboffsy(0);
	for (TopoDb30::TopoTileCoordinate tc(tmin);;) {
		TopoDb30::pixel_index_t ymin((tc.get_lat_tile() == tmin.get_lat_tile()) ? tmin.get_lat_offs() : 0);
		TopoDb30::pixel_index_t ymax((tc.get_lat_tile() == tmax.get_lat_tile()) ? tmax.get_lat_offs() : TopoDb30::tile_size - 1);
		TopoDb30::pixel_index_t xmin((tc.get_lon_tile() == tmin.get_lon_tile()) ? tmin.get_lon_offs() : 0);
		TopoDb30::pixel_index_t xmax((tc.get_lon_tile() == tmax.get_lon_tile()) ? tmax.get_lon_offs() : TopoDb30::tile_size - 1);
		xmax -= xmin;
		ymax -= ymin;
		for (TopoDb30::pixel_index_t y(0); y <= ymax; y++) {
			tc.set_lat_offs(ymin + y);
			for (TopoDb30::pixel_index_t x(0); x <= xmax; x++) {
				tc.set_lon_offs(xmin + x);
				TopoDb30::elev_t elev(topodb.get_elev(tc));
				//std::cerr << "TopoDb: point " << (TopoDb30::TopoCoordinate)tc << ' ' << (Point)(TopoDb30::TopoCoordinate)tc << " elev " << elev << std::endl;
				if (elev == TopoDb30::nodata) {
					((TopoDb30::TopoCoordinate)tc).print(std::cerr << "No topo data for " << (Point)(TopoDb30::TopoCoordinate)tc << " TC ") << " ti " << tc.get_tile_index() << " pi " << tc.get_pixel_index() << std::endl;
				} else {
					unsigned char *d = pbuf_data + 4 * (pboffsx + x) + pbuf_stride * (pbufy - 1 - pboffsy - y);
					uint32_t *d32((uint32_t *)d);
					*d32 = TopoDb30::color(elev);
					//std::cerr << "Pixel X " << (pboffsx + x) << " Y " << (pboffsy + y) << " elev " << elev << std::endl;
				}
			}
		}
		if (tc.get_lon_tile() != tmax.get_lon_tile()) {
			tc.advance_tile_east();
			pboffsx += xmax + 1;
			continue;
		}
		if (tc.get_lat_tile() == tmax.get_lat_tile())
			break;
		tc.set_lon_tile(tmin.get_lon_tile());
		pboffsx = 0;
		tc.advance_tile_north();
		pboffsy += ymax + 1;
	}
	m_bbox = Rect((TopoDb30::TopoCoordinate)tmin, pixeldim + (TopoDb30::TopoCoordinate)tmax);
}

#ifdef HAVE_PILOTLINK
void Engine::DbThread::ElevationMapCairoResult::execute_palm(PalmGtopo& topodb)
{
	PalmGtopo::GtopoCoord gmin(m_bbox.get_southwest()), gmax(m_bbox.get_northeast());
	gmin -= PalmGtopo::GtopoCoord(1, 1);
	gmax += PalmGtopo::GtopoCoord(1, 1);
	std::cerr << "Gtopo: from " << gmin << " to " << gmax << std::endl;
	unsigned int pbufx((gmax-gmin).get_lon()+1);
	unsigned int pbufy((gmax-gmin).get_lat()+1);
	m_surface = Cairo::ImageSurface::create(Cairo::FORMAT_RGB24, pbufx, pbufy);
	unsigned char *pbuf_data(m_surface->get_data());
	int pbuf_stride(m_surface->get_stride());
	for (PalmGtopo::GtopoCoord gp(gmin);;) {
		//std::cerr << "Gtopo: point " << gp << std::endl;
		PalmGtopo::elevation_t elev(topodb[gp]);
		PalmGtopo::GtopoCoord gidx(gp - gmin);
		unsigned char *d = pbuf_data + 4 * gidx.get_lon() + pbuf_stride * (pbufy - 1 - gidx.get_lat());
		uint32_t *d32((uint32_t *)d);
		*d32 = TopoDb30::color(elev);
		if (gp == gmax)
			break;
		if (gp.get_lon() != gmax.get_lon()) {
			gp.set_lon(gp.get_lon()+1);
			continue;
		}
		gp.set_lon(gmin.get_lon());
		gp.set_lat(gp.get_lat()+1);
	}
	m_bbox = Rect(gmin, gmax + PalmGtopo::GtopoCoord(1, 1));
}
#endif


#ifdef HAVE_BOOST

Engine::DbThread::AirwayGraphResult::AirwayGraphResult(const sigc::slot<elementvector_t>& dbaction, const sigc::slot<void>& dbcancel)
	: ResultBase(sigc::compose(sigc::mem_fun(*this, &Engine::DbThread::AirwayGraphResult::set_result), dbaction), dbcancel)
{
}

Engine::DbThread::AirwayGraphResult::~AirwayGraphResult()
{
}

void Engine::DbThread::AirwayGraphResult::set_result(const elementvector_t& ev)
{
	m_graph.add(ev);
}

void Engine::DbThread::AirwayGraphResult::set_result_airports(const AirportsDb::elementvector_t& ev)
{
	m_graph.add(ev);
}

void Engine::DbThread::AirwayGraphResult::set_result_navaids(const NavaidsDb::elementvector_t& ev)
{
	m_graph.add(ev);
}

void Engine::DbThread::AirwayGraphResult::set_result_waypoints(const WaypointsDb::elementvector_t& ev)
{
	m_graph.add(ev);
}

void Engine::DbThread::AirwayGraphResult::set_result_mapelements(const MapelementsDb::elementvector_t& ev)
{
	m_graph.add(ev);
}

Engine::DbThread::AreaGraphResult::AreaGraphResult(const Rect& rect, bool area, AirwaysDbQueryInterface *airwaydb,
						   AirportsDbQueryInterface *airportdb, NavaidsDbQueryInterface *navaiddb,
						   WaypointsDbQueryInterface *waypointdb, MapelementsDbQueryInterface *mapelementdb,
						   const sigc::slot<void>& dbcancel)
	: AirwayGraphResult(sigc::mem_fun(*this, &Engine::DbThread::AreaGraphResult::dbaction), dbcancel),
	  m_rect(rect), m_airwaydb(airwaydb), m_airportdb(airportdb), m_navaiddb(navaiddb), m_waypointdb(waypointdb), m_mapelementdb(mapelementdb), m_area(area)
{
}

Engine::DbThread::AreaGraphResult::elementvector_t Engine::DbThread::AreaGraphResult::dbaction(void)
{
	if (m_airportdb)
		set_result_airports(m_airportdb->find_by_rect(m_rect, ~0, AirportsDb::Airport::subtables_none));
	if (m_navaiddb)
		set_result_navaids(m_navaiddb->find_by_rect(m_rect, ~0, NavaidsDb::Navaid::subtables_none));
	if (m_waypointdb)
		set_result_waypoints(m_waypointdb->find_by_rect(m_rect, ~0, WaypointsDb::Waypoint::subtables_none));
	if (m_mapelementdb)
		set_result_mapelements(m_mapelementdb->find_by_rect(m_rect, ~0, MapelementsDb::Mapelement::subtables_none));
	if (!m_airwaydb)
		return elementvector_t();
	if (m_area)
		return m_airwaydb->find_by_area(m_rect, ~0, AirwaysDb::Airway::subtables_all);
	return m_airwaydb->find_by_rect(m_rect, ~0, AirwaysDb::Airway::subtables_all);
}
#endif

Glib::RefPtr<Engine::DbThread::AirportResult> Engine::DbThread::airport_find_by_icao(const Glib::ustring & nm, unsigned int limit, unsigned int skip, AirportsDb::comp_t comp, unsigned int loadsubtables)
{
	Glib::RefPtr<AirportResult> p;
#ifdef HAVE_PILOTLINK
	if (m_palmairportdb.is_open())
		p = Glib::RefPtr<AirportResult>(new AirportResult(sigc::bind(sigc::mem_fun(m_palmairportdb, &PalmAirports::find_by_icao), nm, limit, skip, to_palm_compare(comp))));
	else
#endif
		p = Glib::RefPtr<AirportResult>(new AirportResult(sigc::bind(sigc::mem_fun(m_airportdb.get(), &AirportsDbQueryInterface::find_by_icao), nm, 0, comp, limit, loadsubtables),
								  sigc::mem_fun(m_airportdb.get(), &AirportsDbQueryInterface::interrupt)));
	queue(p);
	return p;
}

Glib::RefPtr<Engine::DbThread::AirportResult> Engine::DbThread::airport_find_by_name(const Glib::ustring & nm, unsigned int limit, unsigned int skip, AirportsDb::comp_t comp, unsigned int loadsubtables)
{
	Glib::RefPtr<AirportResult> p;
#ifdef HAVE_PILOTLINK
	if (m_palmairportdb.is_open())
		p = Glib::RefPtr<AirportResult>(new AirportResult(sigc::bind(sigc::mem_fun(m_palmairportdb, &PalmAirports::find_by_name), nm, limit, skip, to_palm_compare(comp))));
	else
#endif
		p = Glib::RefPtr<AirportResult>(new AirportResult(sigc::bind(sigc::mem_fun(m_airportdb.get(), &AirportsDbQueryInterface::find_by_name), nm, 0, comp, limit, loadsubtables),
								  sigc::mem_fun(m_airportdb.get(), &AirportsDbQueryInterface::interrupt)));
	queue(p);
	return p;
}

Glib::RefPtr<Engine::DbThread::AirportResult> Engine::DbThread::airport_find_by_text(const Glib::ustring & nm, unsigned int limit, unsigned int skip, AirportsDb::comp_t comp, unsigned int loadsubtables)
{
	Glib::RefPtr<AirportResult> p;
#ifdef HAVE_PILOTLINK
	if (m_palmairportdb.is_open())
		p = Glib::RefPtr<AirportResult>(new AirportResult(sigc::bind(sigc::mem_fun(m_palmairportdb, &PalmAirports::find_by_text), nm, limit, skip, to_palm_compare(comp))));
	else
#endif
		p = Glib::RefPtr<AirportResult>(new AirportResult(sigc::bind(sigc::mem_fun(m_airportdb.get(), &AirportsDbQueryInterface::find_by_text), nm, 0, comp, limit, loadsubtables),
								  sigc::mem_fun(m_airportdb.get(), &AirportsDbQueryInterface::interrupt)));
	queue(p);
	return p;
}

Glib::RefPtr<Engine::DbThread::AirportResult> Engine::DbThread::airport_find_bbox(const Rect & rect, unsigned int limit, unsigned int loadsubtables)
{
	Glib::RefPtr<AirportResult> p;
#ifdef HAVE_PILOTLINK
	if (m_palmairportdb.is_open())
		p = Glib::RefPtr<AirportResult>(new AirportResult(sigc::bind(sigc::mem_fun(m_palmairportdb, &PalmAirports::find_bbox), rect, limit)));
	else
#endif
		p = Glib::RefPtr<AirportResult>(new AirportResult(sigc::bind(sigc::mem_fun(m_airportdb.get(), &AirportsDbQueryInterface::find_by_rect), rect, limit, loadsubtables),
								  sigc::mem_fun(m_airportdb.get(), &AirportsDbQueryInterface::interrupt)));
	queue(p);
	return p;
}

Glib::RefPtr<Engine::DbThread::AirportResult> Engine::DbThread::airport_find_nearest(const Point & pt, unsigned int limit, const Rect & rect, unsigned int loadsubtables)
{
	Glib::RefPtr<AirportResult> p;
#ifdef HAVE_PILOTLINK
	if (m_palmairportdb.is_open())
		p = Glib::RefPtr<AirportResult>(new AirportResult(sigc::bind(sigc::mem_fun(m_palmairportdb, &PalmAirports::find_nearest), pt, limit, rect)));
	else
#endif
		p = Glib::RefPtr<AirportResult>(new AirportResult(sigc::bind(sigc::mem_fun(m_airportdb.get(), &AirportsDbQueryInterface::find_nearest), pt, rect, limit, loadsubtables),
								  sigc::mem_fun(m_airportdb.get(), &AirportsDbQueryInterface::interrupt)));
	queue(p);
	return p;
}

Glib::RefPtr<Engine::DbThread::AirportSaveResult> Engine::DbThread::airport_save(const AirportsDb::Airport & e)
{
	Glib::RefPtr<AirportSaveResult> p(new AirportSaveResult(sigc::bind(sigc::mem_fun(m_airportdb.get(), &AirportsDbQueryInterface::save), e),
								sigc::mem_fun(m_airportdb.get(), &AirportsDbQueryInterface::interrupt)));
	queue(p);
	return p;
}

Glib::RefPtr<Engine::DbThread::AirspaceResult> Engine::DbThread::airspace_find_by_icao(const Glib::ustring & nm, unsigned int limit, unsigned int skip, AirspacesDb::comp_t comp, unsigned int loadsubtables)
{
	Glib::RefPtr<AirspaceResult> p;
#ifdef HAVE_PILOTLINK
	if (m_palmairspacedb.is_open())
		p = Glib::RefPtr<AirspaceResult>(new AirspaceResult(sigc::bind(sigc::mem_fun(m_palmairspacedb, &PalmAirspaces::find_by_icao), nm, limit, skip, to_palm_compare(comp))));
	else
#endif
		p = Glib::RefPtr<AirspaceResult>(new AirspaceResult(sigc::bind(sigc::mem_fun(m_airspacedb.get(), &AirspacesDbQueryInterface::find_by_icao), nm, 0, comp, limit, loadsubtables),
								    sigc::mem_fun(m_airspacedb.get(), &AirspacesDbQueryInterface::interrupt)));
	queue(p);
	return p;
}

Glib::RefPtr<Engine::DbThread::AirspaceResult> Engine::DbThread::airspace_find_by_name(const Glib::ustring & nm, unsigned int limit, unsigned int skip, AirspacesDb::comp_t comp, unsigned int loadsubtables)
{
	Glib::RefPtr<AirspaceResult> p;
#ifdef HAVE_PILOTLINK
	if (m_palmairspacedb.is_open())
		p = Glib::RefPtr<AirspaceResult>(new AirspaceResult(sigc::bind(sigc::mem_fun(m_palmairspacedb, &PalmAirspaces::find_by_name), nm, limit, skip, to_palm_compare(comp))));
	else
#endif
		p = Glib::RefPtr<AirspaceResult>(new AirspaceResult(sigc::bind(sigc::mem_fun(m_airspacedb.get(), &AirspacesDbQueryInterface::find_by_name), nm, 0, comp, limit, loadsubtables),
								    sigc::mem_fun(m_airspacedb.get(), &AirspacesDbQueryInterface::interrupt)));
	queue(p);
	return p;
}

Glib::RefPtr<Engine::DbThread::AirspaceResult> Engine::DbThread::airspace_find_by_text(const Glib::ustring & nm, unsigned int limit, unsigned int skip, AirspacesDb::comp_t comp, unsigned int loadsubtables)
{
	Glib::RefPtr<AirspaceResult> p;
#ifdef HAVE_PILOTLINK
	if (m_palmairspacedb.is_open())
		p = Glib::RefPtr<AirspaceResult>(new AirspaceResult(sigc::bind(sigc::mem_fun(m_palmairspacedb, &PalmAirspaces::find_by_text), nm, limit, skip, to_palm_compare(comp))));
	else
#endif
		p = Glib::RefPtr<AirspaceResult>(new AirspaceResult(sigc::bind(sigc::mem_fun(m_airspacedb.get(), &AirspacesDbQueryInterface::find_by_text), nm, 0, comp, limit, loadsubtables),
								    sigc::mem_fun(m_airspacedb.get(), &AirspacesDbQueryInterface::interrupt)));
	queue(p);
	return p;
}

Glib::RefPtr<Engine::DbThread::AirspaceResult> Engine::DbThread::airspace_find_bbox(const Rect & rect, unsigned int limit, unsigned int loadsubtables)
{
	Glib::RefPtr<AirspaceResult> p;
#ifdef HAVE_PILOTLINK
	if (m_palmairspacedb.is_open())
		p = Glib::RefPtr<AirspaceResult>(new AirspaceResult(sigc::bind(sigc::mem_fun(m_palmairspacedb, &PalmAirspaces::find_bbox), rect, limit)));
	else
#endif
		p = Glib::RefPtr<AirspaceResult>(new AirspaceResult(sigc::bind(sigc::mem_fun(m_airspacedb.get(), &AirspacesDbQueryInterface::find_by_rect), rect, limit, loadsubtables),
								    sigc::mem_fun(m_airspacedb.get(), &AirspacesDbQueryInterface::interrupt)));
	queue(p);
	return p;
}

Glib::RefPtr<Engine::DbThread::AirspaceResult> Engine::DbThread::airspace_find_nearest(const Point & pt, unsigned int limit, const Rect & rect, unsigned int loadsubtables)
{
	Glib::RefPtr<AirspaceResult> p;
#ifdef HAVE_PILOTLINK
	if (m_palmairspacedb.is_open())
		p = Glib::RefPtr<AirspaceResult>(new AirspaceResult(sigc::bind(sigc::mem_fun(m_palmairspacedb, &PalmAirspaces::find_nearest), pt, limit, rect)));
	else
#endif
		p = Glib::RefPtr<AirspaceResult>(new AirspaceResult(sigc::bind(sigc::mem_fun(m_airspacedb.get(), &AirspacesDbQueryInterface::find_nearest), pt, rect, limit, loadsubtables),
								    sigc::mem_fun(m_airspacedb.get(), &AirspacesDbQueryInterface::interrupt)));
	queue(p);
	return p;
}

Glib::RefPtr<Engine::DbThread::AirspaceSaveResult> Engine::DbThread::airspace_save(const AirspacesDb::Airspace & e)
{
	Glib::RefPtr<AirspaceSaveResult> p(new AirspaceSaveResult(sigc::bind(sigc::mem_fun(m_airspacedb.get(), &AirspacesDbQueryInterface::save), e),
								  sigc::mem_fun(m_airspacedb.get(), &AirspacesDbQueryInterface::interrupt)));
	queue(p);
	return p;
}

Glib::RefPtr<Engine::DbThread::NavaidResult> Engine::DbThread::navaid_find_by_icao(const Glib::ustring & nm, unsigned int limit, unsigned int skip, NavaidsDb::comp_t comp, unsigned int loadsubtables)
{
	Glib::RefPtr<NavaidResult> p;
#ifdef HAVE_PILOTLINK
	if (m_palmnavaiddb.is_open())
		p = Glib::RefPtr<NavaidResult>(new NavaidResult(sigc::bind(sigc::mem_fun(m_palmnavaiddb, &PalmNavaids::find_by_icao), nm, limit, skip, to_palm_compare(comp))));
	else
#endif
		p = Glib::RefPtr<NavaidResult>(new NavaidResult(sigc::bind(sigc::mem_fun(m_navaiddb.get(), &NavaidsDbQueryInterface::find_by_icao), nm, 0, comp, limit, loadsubtables),
								sigc::mem_fun(m_navaiddb.get(), &NavaidsDbQueryInterface::interrupt)));
	queue(p);
	return p;
}

Glib::RefPtr<Engine::DbThread::NavaidResult> Engine::DbThread::navaid_find_by_name(const Glib::ustring & nm, unsigned int limit, unsigned int skip, NavaidsDb::comp_t comp, unsigned int loadsubtables)
{
	Glib::RefPtr<NavaidResult> p;
#ifdef HAVE_PILOTLINK
	if (m_palmnavaiddb.is_open())
		p = Glib::RefPtr<NavaidResult>(new NavaidResult(sigc::bind(sigc::mem_fun(m_palmnavaiddb, &PalmNavaids::find_by_name), nm, limit, skip, to_palm_compare(comp))));
	else
#endif
		p = Glib::RefPtr<NavaidResult>(new NavaidResult(sigc::bind(sigc::mem_fun(m_navaiddb.get(), &NavaidsDbQueryInterface::find_by_name), nm, 0, comp, limit, loadsubtables),
								sigc::mem_fun(m_navaiddb.get(), &NavaidsDbQueryInterface::interrupt)));
	queue(p);
	return p;
}

Glib::RefPtr<Engine::DbThread::NavaidResult> Engine::DbThread::navaid_find_by_text(const Glib::ustring & nm, unsigned int limit, unsigned int skip, NavaidsDb::comp_t comp, unsigned int loadsubtables)
{
	Glib::RefPtr<NavaidResult> p;
#ifdef HAVE_PILOTLINK
	if (m_palmnavaiddb.is_open())
		p = Glib::RefPtr<NavaidResult>(new NavaidResult(sigc::bind(sigc::mem_fun(m_palmnavaiddb, &PalmNavaids::find_by_name), nm, limit, skip, to_palm_compare(comp))));
	else
#endif
		p = Glib::RefPtr<NavaidResult>(new NavaidResult(sigc::bind(sigc::mem_fun(m_navaiddb.get(), &NavaidsDbQueryInterface::find_by_text), nm, 0, comp, limit, loadsubtables),
								sigc::mem_fun(m_navaiddb.get(), &NavaidsDbQueryInterface::interrupt)));
	queue(p);
	return p;
}

Glib::RefPtr<Engine::DbThread::NavaidResult> Engine::DbThread::navaid_find_bbox(const Rect & rect, unsigned int limit, unsigned int loadsubtables)
{
	Glib::RefPtr<NavaidResult> p;
#ifdef HAVE_PILOTLINK
	if (m_palmnavaiddb.is_open())
		p = Glib::RefPtr<NavaidResult>(new NavaidResult(sigc::bind(sigc::mem_fun(m_palmnavaiddb, &PalmNavaids::find_bbox), rect, limit)));
	else
#endif
		p = Glib::RefPtr<NavaidResult>(new NavaidResult(sigc::bind(sigc::mem_fun(m_navaiddb.get(), &NavaidsDbQueryInterface::find_by_rect), rect, limit, loadsubtables),
								sigc::mem_fun(m_navaiddb.get(), &NavaidsDbQueryInterface::interrupt)));
	queue(p);
	return p;
}

Glib::RefPtr<Engine::DbThread::NavaidResult> Engine::DbThread::navaid_find_nearest(const Point & pt, unsigned int limit, const Rect & rect, unsigned int loadsubtables)
{
	Glib::RefPtr<NavaidResult> p;
#ifdef HAVE_PILOTLINK
	if (m_palmnavaiddb.is_open())
		p = Glib::RefPtr<NavaidResult>(new NavaidResult(sigc::bind(sigc::mem_fun(m_palmnavaiddb, &PalmNavaids::find_nearest), pt, limit, rect)));
	else
#endif
		p = Glib::RefPtr<NavaidResult>(new NavaidResult(sigc::bind(sigc::mem_fun(m_navaiddb.get(), &NavaidsDbQueryInterface::find_nearest), pt, rect, limit, loadsubtables),
								sigc::mem_fun(m_navaiddb.get(), &NavaidsDbQueryInterface::interrupt)));
	queue(p);
	return p;
}

Glib::RefPtr<Engine::DbThread::NavaidSaveResult> Engine::DbThread::navaid_save(const NavaidsDb::Navaid & e)
{
	Glib::RefPtr<NavaidSaveResult> p(new NavaidSaveResult(sigc::bind(sigc::mem_fun(m_navaiddb.get(), &NavaidsDbQueryInterface::save), e),
							      sigc::mem_fun(m_navaiddb.get(), &NavaidsDbQueryInterface::interrupt)));
	queue(p);
	return p;
}

Glib::RefPtr<Engine::DbThread::WaypointResult> Engine::DbThread::waypoint_find_by_name(const Glib::ustring & nm, unsigned int limit, unsigned int skip, WaypointsDb::comp_t comp, unsigned int loadsubtables)
{
	Glib::RefPtr<WaypointResult> p;
#ifdef HAVE_PILOTLINK
	if (m_palmwaypointdb.is_open())
		p = Glib::RefPtr<WaypointResult>(new WaypointResult(sigc::bind(sigc::mem_fun(m_palmwaypointdb, &PalmWaypoints::find_by_name), nm, limit, skip, to_palm_compare(comp))));
	else
#endif
		p = Glib::RefPtr<WaypointResult>(new WaypointResult(sigc::bind(sigc::mem_fun(m_waypointdb.get(), &WaypointsDbQueryInterface::find_by_name), nm, 0, comp, limit, loadsubtables),
								    sigc::mem_fun(m_waypointdb.get(), &WaypointsDbQueryInterface::interrupt)));
	queue(p);
	return p;
}

Glib::RefPtr<Engine::DbThread::WaypointResult> Engine::DbThread::waypoint_find_bbox(const Rect & rect, unsigned int limit, unsigned int loadsubtables)
{
	Glib::RefPtr<WaypointResult> p;
#ifdef HAVE_PILOTLINK
	if (m_palmwaypointdb.is_open())
		p = Glib::RefPtr<WaypointResult>(new WaypointResult(sigc::bind(sigc::mem_fun(m_palmwaypointdb, &PalmWaypoints::find_bbox), rect, limit)));
	else
#endif
		p = Glib::RefPtr<WaypointResult>(new WaypointResult(sigc::bind(sigc::mem_fun(m_waypointdb.get(), &WaypointsDbQueryInterface::find_by_rect), rect, limit, loadsubtables),
								    sigc::mem_fun(m_waypointdb.get(), &WaypointsDbQueryInterface::interrupt)));
	queue(p);
	return p;
}

Glib::RefPtr<Engine::DbThread::WaypointResult> Engine::DbThread::waypoint_find_nearest(const Point & pt, unsigned int limit, const Rect & rect, unsigned int loadsubtables)
{
	Glib::RefPtr<WaypointResult> p;
#ifdef HAVE_PILOTLINK
	if (m_palmwaypointdb.is_open())
		p = Glib::RefPtr<WaypointResult>(new WaypointResult(sigc::bind(sigc::mem_fun(m_palmwaypointdb, &PalmWaypoints::find_nearest), pt, limit, rect)));
	else
#endif
		p = Glib::RefPtr<WaypointResult>(new WaypointResult(sigc::bind(sigc::mem_fun(m_waypointdb.get(), &WaypointsDbQueryInterface::find_nearest), pt, rect, limit, loadsubtables),
								    sigc::mem_fun(m_waypointdb.get(), &WaypointsDbQueryInterface::interrupt)));
	queue(p);
	return p;
}

Glib::RefPtr<Engine::DbThread::WaypointSaveResult> Engine::DbThread::waypoint_save(const WaypointsDb::Waypoint & e)
{
	Glib::RefPtr<WaypointSaveResult> p(new WaypointSaveResult(sigc::bind(sigc::mem_fun(m_waypointdb.get(), &WaypointsDbQueryInterface::save), e),
								  sigc::mem_fun(m_waypointdb.get(), &WaypointsDbQueryInterface::interrupt)));
	queue(p);
	return p;
}

Glib::RefPtr<Engine::DbThread::AirwayResult> Engine::DbThread::airway_find_by_name(const Glib::ustring & nm, unsigned int limit, unsigned int skip, AirwaysDb::comp_t comp, unsigned int loadsubtables)
{
	Glib::RefPtr<AirwayResult> p;
	p = Glib::RefPtr<AirwayResult>(new AirwayResult(sigc::bind(sigc::mem_fun(m_airwaydb.get(), &AirwaysDbQueryInterface::find_by_name), nm, 0, comp, limit, loadsubtables),
				       sigc::mem_fun(m_airwaydb.get(), &AirwaysDbQueryInterface::interrupt)));
	queue(p);
	return p;
}

Glib::RefPtr<Engine::DbThread::AirwayResult> Engine::DbThread::airway_find_by_text(const Glib::ustring& nm, unsigned int limit, unsigned int skip, AirwaysDb::comp_t comp, unsigned int loadsubtables)
{
	Glib::RefPtr<AirwayResult> p;
	p = Glib::RefPtr<AirwayResult>(new AirwayResult(sigc::bind(sigc::mem_fun(m_airwaydb.get(), &AirwaysDbQueryInterface::find_by_text), nm, 0, comp, limit, loadsubtables),
				       sigc::mem_fun(m_airwaydb.get(), &AirwaysDbQueryInterface::interrupt)));
	queue(p);
	return p;
}

Glib::RefPtr<Engine::DbThread::AirwayResult> Engine::DbThread::airway_find_bbox(const Rect & rect, unsigned int limit, unsigned int loadsubtables)
{
	Glib::RefPtr<AirwayResult> p;
	p = Glib::RefPtr<AirwayResult>(new AirwayResult(sigc::bind(sigc::mem_fun(m_airwaydb.get(), &AirwaysDbQueryInterface::find_by_rect), rect, limit, loadsubtables),
				       sigc::mem_fun(m_airwaydb.get(), &AirwaysDbQueryInterface::interrupt)));
	queue(p);
	return p;
}

Glib::RefPtr<Engine::DbThread::AirwayResult> Engine::DbThread::airway_find_area(const Rect & rect, unsigned int limit, unsigned int loadsubtables)
{
	Glib::RefPtr<AirwayResult> p;
	p = Glib::RefPtr<AirwayResult>(new AirwayResult(sigc::bind(sigc::mem_fun(m_airwaydb.get(), &AirwaysDbQueryInterface::find_by_area), rect, limit, loadsubtables),
				       sigc::mem_fun(m_airwaydb.get(), &AirwaysDbQueryInterface::interrupt)));
	queue(p);
	return p;
}

Glib::RefPtr<Engine::DbThread::AirwayResult> Engine::DbThread::airway_find_nearest(const Point & pt, unsigned int limit, const Rect & rect, unsigned int loadsubtables)
{
	Glib::RefPtr<AirwayResult> p;
	p = Glib::RefPtr<AirwayResult>(new AirwayResult(sigc::bind(sigc::mem_fun(m_airwaydb.get(), &AirwaysDbQueryInterface::find_nearest), pt, rect, limit, loadsubtables),
				       sigc::mem_fun(m_airwaydb.get(), &AirwaysDbQueryInterface::interrupt)));
	queue(p);
	return p;
}

Glib::RefPtr<Engine::DbThread::AirwaySaveResult> Engine::DbThread::airway_save(const AirwaysDb::Airway & e)
{
	Glib::RefPtr<AirwaySaveResult> p(new AirwaySaveResult(sigc::bind(sigc::mem_fun(m_airwaydb.get(), &AirwaysDbQueryInterface::save), e),
					 sigc::mem_fun(m_airwaydb.get(), &AirwaysDbQueryInterface::interrupt)));
	queue(p);
	return p;
}

#ifdef HAVE_BOOST

Glib::RefPtr<Engine::DbThread::AirwayGraphResult> Engine::DbThread::airway_graph_find_by_name(const Glib::ustring & nm, unsigned int limit, unsigned int skip, AirwaysDb::comp_t comp, unsigned int loadsubtables)
{
	Glib::RefPtr<AirwayGraphResult> p;
	p = Glib::RefPtr<AirwayGraphResult>(new AirwayGraphResult(sigc::bind(sigc::mem_fun(m_airwaydb.get(), &AirwaysDbQueryInterface::find_by_name), nm, 0, comp, limit, loadsubtables),
								  sigc::mem_fun(m_airwaydb.get(), &AirwaysDbQueryInterface::interrupt)));
	queue(p);
	return p;
}

Glib::RefPtr<Engine::DbThread::AirwayGraphResult> Engine::DbThread::airway_graph_find_by_text(const Glib::ustring& nm, unsigned int limit, unsigned int skip, AirwaysDb::comp_t comp, unsigned int loadsubtables)
{
	Glib::RefPtr<AirwayGraphResult> p;
	p = Glib::RefPtr<AirwayGraphResult>(new AirwayGraphResult(sigc::bind(sigc::mem_fun(m_airwaydb.get(), &AirwaysDbQueryInterface::find_by_text), nm, 0, comp, limit, loadsubtables),
								  sigc::mem_fun(m_airwaydb.get(), &AirwaysDbQueryInterface::interrupt)));
	queue(p);
	return p;
}

Glib::RefPtr<Engine::DbThread::AirwayGraphResult> Engine::DbThread::airway_graph_find_bbox(const Rect & rect, unsigned int limit, unsigned int loadsubtables)
{
	Glib::RefPtr<AirwayGraphResult> p;
	p = Glib::RefPtr<AirwayGraphResult>(new AirwayGraphResult(sigc::bind(sigc::mem_fun(m_airwaydb.get(), &AirwaysDbQueryInterface::find_by_rect), rect, limit, loadsubtables),
								  sigc::mem_fun(m_airwaydb.get(), &AirwaysDbQueryInterface::interrupt)));
	queue(p);
	return p;
}

Glib::RefPtr<Engine::DbThread::AirwayGraphResult> Engine::DbThread::airway_graph_find_area(const Rect & rect, unsigned int limit, unsigned int loadsubtables)
{
	Glib::RefPtr<AirwayGraphResult> p;
	p = Glib::RefPtr<AirwayGraphResult>(new AirwayGraphResult(sigc::bind(sigc::mem_fun(m_airwaydb.get(), &AirwaysDbQueryInterface::find_by_area), rect, limit, loadsubtables),
								  sigc::mem_fun(m_airwaydb.get(), &AirwaysDbQueryInterface::interrupt)));
	queue(p);
	return p;
}

Glib::RefPtr<Engine::DbThread::AirwayGraphResult> Engine::DbThread::airway_graph_find_nearest(const Point & pt, unsigned int limit, const Rect & rect, unsigned int loadsubtables)
{
	Glib::RefPtr<AirwayGraphResult> p;
	p = Glib::RefPtr<AirwayGraphResult>(new AirwayGraphResult(sigc::bind(sigc::mem_fun(m_airwaydb.get(), &AirwaysDbQueryInterface::find_nearest), pt, rect, limit, loadsubtables),
								  sigc::mem_fun(m_airwaydb.get(), &AirwaysDbQueryInterface::interrupt)));
	queue(p);
	return p;
}

Glib::RefPtr<Engine::DbThread::AreaGraphResult> Engine::DbThread::area_graph_find_bbox(const Rect& rect, AreaGraphResult::Vertex::typemask_t tmask)
{
	Glib::RefPtr<AreaGraphResult> p;
	p = Glib::RefPtr<AreaGraphResult>(new AreaGraphResult(rect, false,
							      (tmask & AreaGraphResult::Vertex::typemask_airway) ? m_airwaydb.get() : 0,
							      (tmask & AreaGraphResult::Vertex::typemask_airport) ? m_airportdb.get() : 0,
							      (tmask & AreaGraphResult::Vertex::typemask_navaid) ? m_navaiddb.get() : 0,
							      (tmask & AreaGraphResult::Vertex::typemask_intersection) ? m_waypointdb.get() : 0,
							      (tmask & AreaGraphResult::Vertex::typemask_mapelement) ? m_mapelementdb.get() : 0,
							      sigc::mem_fun(m_airwaydb.get(), &AirwaysDbQueryInterface::interrupt)));
	queue(p);
	return p;
}

Glib::RefPtr<Engine::DbThread::AreaGraphResult> Engine::DbThread::area_graph_find_area(const Rect& rect, AreaGraphResult::Vertex::typemask_t tmask)
{
	Glib::RefPtr<AreaGraphResult> p;
	p = Glib::RefPtr<AreaGraphResult>(new AreaGraphResult(rect, true,
							      (tmask & AreaGraphResult::Vertex::typemask_airway) ? m_airwaydb.get() : 0,
							      (tmask & AreaGraphResult::Vertex::typemask_airport) ? m_airportdb.get() : 0,
							      (tmask & AreaGraphResult::Vertex::typemask_navaid) ? m_navaiddb.get() : 0,
							      (tmask & AreaGraphResult::Vertex::typemask_intersection) ? m_waypointdb.get() : 0,
							      (tmask & AreaGraphResult::Vertex::typemask_mapelement) ? m_mapelementdb.get() : 0,
							      sigc::mem_fun(m_airwaydb.get(), &AirwaysDbQueryInterface::interrupt)));
	queue(p);
	return p;
}
#endif

Glib::RefPtr<Engine::DbThread::MapelementResult> Engine::DbThread::mapelement_find_by_name(const Glib::ustring & nm, unsigned int limit, unsigned int skip, WaypointsDb::comp_t comp, unsigned int loadsubtables)
{
	Glib::RefPtr<MapelementResult> p;
#ifdef HAVE_PILOTLINK
	if (m_palmmapelementdb.is_open())
		p = Glib::RefPtr<MapelementResult>(new MapelementResult(sigc::bind(sigc::mem_fun(m_palmmapelementdb, &PalmMapelements::find_by_name), nm, limit, skip, to_palm_compare(comp))));
	else
#endif
		p = Glib::RefPtr<MapelementResult>(new MapelementResult(sigc::bind(sigc::mem_fun(m_mapelementdb.get(), &MapelementsDbQueryInterface::find_by_name), nm, 0, comp, limit, loadsubtables),
									sigc::mem_fun(m_mapelementdb.get(), &MapelementsDbQueryInterface::interrupt)));
	queue(p);
	return p;
}

Glib::RefPtr<Engine::DbThread::MapelementResult> Engine::DbThread::mapelement_find_bbox(const Rect & rect, unsigned int limit, unsigned int loadsubtables)
{
	Glib::RefPtr<MapelementResult> p;
#ifdef HAVE_PILOTLINK
	if (m_palmmapelementdb.is_open())
		p = Glib::RefPtr<MapelementResult>(new MapelementResult(sigc::bind(sigc::mem_fun(m_palmmapelementdb, &PalmMapelements::find_bbox), rect, limit)));
	else
#endif
		p = Glib::RefPtr<MapelementResult>(new MapelementResult(sigc::bind(sigc::mem_fun(m_mapelementdb.get(), &MapelementsDbQueryInterface::find_by_rect), rect, limit, loadsubtables),
									sigc::mem_fun(m_mapelementdb.get(), &MapelementsDbQueryInterface::interrupt)));
	queue(p);
	return p;
}

Glib::RefPtr<Engine::DbThread::MapelementResult> Engine::DbThread::mapelement_find_nearest(const Point & pt, unsigned int limit, const Rect & rect, unsigned int loadsubtables)
{
	Glib::RefPtr<MapelementResult> p;
#ifdef HAVE_PILOTLINK
	if (m_palmmapelementdb.is_open())
		p = Glib::RefPtr<MapelementResult>(new MapelementResult(sigc::bind(sigc::mem_fun(m_palmmapelementdb, &PalmMapelements::find_nearest), pt, limit, rect)));
	else
#endif
		p = Glib::RefPtr<MapelementResult>(new MapelementResult(sigc::bind(sigc::mem_fun(m_mapelementdb.get(), &MapelementsDbQueryInterface::find_nearest), pt, rect, limit, loadsubtables),
									sigc::mem_fun(m_mapelementdb.get(), &MapelementsDbQueryInterface::interrupt)));
	queue(p);
	return p;
}

Glib::RefPtr<Engine::DbThread::MapelementSaveResult> Engine::DbThread::mapelement_save(const MapelementsDb::Mapelement & e)
{
	Glib::RefPtr<MapelementSaveResult> p(new MapelementSaveResult(sigc::bind(sigc::mem_fun(m_mapelementdb.get(), &MapelementsDbQueryInterface::save), e),
								      sigc::mem_fun(m_mapelementdb.get(), &MapelementsDbQueryInterface::interrupt)));
	queue(p);
	return p;
}

Glib::RefPtr<Engine::DbThread::ElevationResult> Engine::DbThread::elevation_point(const Point & pt)
{
	Glib::RefPtr<ElevationResult> p;
#ifdef HAVE_PILOTLINK
	if (m_palmgtopodb.is_open())
		p = Glib::RefPtr<ElevationResult>(new ElevationResult(pt, m_palmgtopodb));
	else
#endif
		p = Glib::RefPtr<ElevationResult>(new ElevationResult(pt, m_topodb));
	queue(p);
	return p;
}

Glib::RefPtr<Engine::DbThread::ElevationMapResult> Engine::DbThread::elevation_map(const Rect & bbox)
{
	Glib::RefPtr<ElevationMapResult> p;
#ifdef HAVE_PILOTLINK
	if (m_palmgtopodb.is_open())
		p = Glib::RefPtr<ElevationMapResult>(new ElevationMapResult(bbox, m_palmgtopodb));
	else
#endif
		p = Glib::RefPtr<ElevationMapResult>(new ElevationMapResult(bbox, m_topodb));
	queue(p);
	return p;
}

Glib::RefPtr<Engine::DbThread::ElevationMapCairoResult> Engine::DbThread::elevation_map_cairo(const Rect & bbox)
{
	Glib::RefPtr<ElevationMapCairoResult> p;
#ifdef HAVE_PILOTLINK
	if (m_palmgtopodb.is_open())
		p = Glib::RefPtr<ElevationMapCairoResult>(new ElevationMapCairoResult(bbox, m_palmgtopodb));
	else
#endif
		p = Glib::RefPtr<ElevationMapCairoResult>(new ElevationMapCairoResult(bbox, m_topodb));
	queue(p);
	return p;
}

Glib::RefPtr<Engine::DbThread::ElevationMinMaxResult> Engine::DbThread::elevation_minmax(const Rect& r)
{
	Glib::RefPtr<ElevationMinMaxResult> p;
#ifdef HAVE_PILOTLINK
	if (m_palmgtopodb.is_open())
		p = Glib::RefPtr<ElevationMinMaxResult>(new ElevationMinMaxResult(r, m_palmgtopodb));
	else
#endif
		p = Glib::RefPtr<ElevationMinMaxResult>(new ElevationMinMaxResult(r, m_topodb));
	queue(p);
	return p;
}

Glib::RefPtr<Engine::DbThread::ElevationMinMaxResult> Engine::DbThread::elevation_minmax(const PolygonSimple& po)
{
	Glib::RefPtr<ElevationMinMaxResult> p;
#ifdef HAVE_PILOTLINK
	if (m_palmgtopodb.is_open())
		p = Glib::RefPtr<ElevationMinMaxResult>(new ElevationMinMaxResult(po, m_palmgtopodb));
	else
#endif
		p = Glib::RefPtr<ElevationMinMaxResult>(new ElevationMinMaxResult(po, m_topodb));
	queue(p);
	return p;
}

Glib::RefPtr<Engine::DbThread::ElevationMinMaxResult> Engine::DbThread::elevation_minmax(const PolygonHole& po)
{
	Glib::RefPtr<ElevationMinMaxResult> p;
#ifdef HAVE_PILOTLINK
	if (m_palmgtopodb.is_open())
		p = Glib::RefPtr<ElevationMinMaxResult>(new ElevationMinMaxResult(po, m_palmgtopodb));
	else
#endif
		p = Glib::RefPtr<ElevationMinMaxResult>(new ElevationMinMaxResult(po, m_topodb));
	queue(p);
	return p;
}

Glib::RefPtr<Engine::DbThread::ElevationMinMaxResult> Engine::DbThread::elevation_minmax(const MultiPolygonHole& po)
{
	Glib::RefPtr<ElevationMinMaxResult> p;
#ifdef HAVE_PILOTLINK
	if (m_palmgtopodb.is_open())
		p = Glib::RefPtr<ElevationMinMaxResult>(new ElevationMinMaxResult(po, m_palmgtopodb));
	else
#endif
		p = Glib::RefPtr<ElevationMinMaxResult>(new ElevationMinMaxResult(po, m_topodb));
	queue(p);
	return p;
}

Glib::RefPtr<Engine::DbThread::ElevationProfileResult> Engine::DbThread::elevation_profile(const Point& p0, const Point& p1, double corridor_width)
{
	Glib::RefPtr<ElevationProfileResult> p;
#ifdef HAVE_PILOTLINK
	if (m_palmgtopodb.is_open())
		p = Glib::RefPtr<ElevationProfileResult>(new ElevationProfileResult(p0, p1, corridor_width, m_palmgtopodb));
	else
#endif
		p = Glib::RefPtr<ElevationProfileResult>(new ElevationProfileResult(p0, p1, corridor_width, m_topodb));
	queue(p);
	return p;
}

Glib::RefPtr<Engine::DbThread::ElevationRouteProfileResult> Engine::DbThread::elevation_profile(const FPlanRoute& fpl, double corridor_width)
{
	Glib::RefPtr<ElevationRouteProfileResult> p;
#ifdef HAVE_PILOTLINK
	if (m_palmgtopodb.is_open())
		p = Glib::RefPtr<ElevationRouteProfileResult>(new ElevationRouteProfileResult(fpl, corridor_width, m_palmgtopodb));
	else
#endif
		p = Glib::RefPtr<ElevationRouteProfileResult>(new ElevationRouteProfileResult(fpl, corridor_width, m_topodb));
	queue(p);
	return p;
}
