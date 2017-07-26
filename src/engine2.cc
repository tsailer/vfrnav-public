/***************************************************************************
 *   Copyright (C) 2012, 2013 by Thomas Sailer                             *
 *   t.sailer@alumni.ethz.ch                                               *
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

#include <iomanip>

class Engine::DbObjectAirport : public DbObject {
  public:
	DbObjectAirport(const AirportsDb::Airport& el) : m_element(el) {}
	virtual Point get_coord(void) const { return m_element.get_coord(); }
	virtual const Glib::ustring& get_ident(void) const { return m_element.get_icao(); }
	virtual const Glib::ustring& get_icao(void) const { return m_element.get_icao(); }
	virtual const Glib::ustring& get_name(void) const { return m_element.get_name(); }
	virtual bool is_airport(void) const { return true; }
	using DbObject::get;
	virtual bool get(AirportsDb::Airport& el) const { el = m_element; return true; }
	using DbObject::set;
	virtual bool set(FPlanWaypoint& el) const { el.set(m_element); return true; }
	using DbObject::insert;
	virtual unsigned int insert(FPlanRoute& route, uint32_t nr = ~0U) const { return route.insert(m_element, nr); }

  protected:
	AirportsDb::Airport m_element;
};

class Engine::DbObjectNavaid : public DbObject {
  public:
	DbObjectNavaid(const NavaidsDb::Navaid& el) : m_element(el) {}
	virtual Point get_coord(void) const { return m_element.get_coord(); }
	virtual const Glib::ustring& get_ident(void) const { return m_element.get_icao(); }
	virtual const Glib::ustring& get_icao(void) const { return m_element.get_icao(); }
	virtual const Glib::ustring& get_name(void) const { return m_element.get_name(); }
	virtual bool is_navaid(void) const { return true; }
	using DbObject::get;
	virtual bool get(NavaidsDb::Navaid& el) const { el = m_element; return true; }
	using DbObject::set;
	virtual bool set(FPlanWaypoint& el) const { el.set(m_element); return true; }
	using DbObject::insert;
	virtual unsigned int insert(FPlanRoute& route, uint32_t nr = ~0U) const { return route.insert(m_element, nr); }

  protected:
	NavaidsDb::Navaid m_element;
};

class Engine::DbObjectWaypoint : public DbObject {
  public:
	DbObjectWaypoint(const WaypointsDb::Waypoint& el) : m_element(el) {}
	virtual Point get_coord(void) const { return m_element.get_coord(); }
	virtual const Glib::ustring& get_ident(void) const { return m_element.get_name(); }
	virtual const Glib::ustring& get_icao(void) const { return m_element.get_icao(); }
	virtual const Glib::ustring& get_name(void) const { return m_element.get_name(); }
	virtual bool is_intersection(void) const { return true; }
	using DbObject::get;
	virtual bool get(WaypointsDb::Waypoint& el) const { el = m_element; return true; }
	using DbObject::set;
	virtual bool set(FPlanWaypoint& el) const { el.set(m_element); return true; }
	using DbObject::insert;
	virtual unsigned int insert(FPlanRoute& route, uint32_t nr = ~0U) const { return route.insert(m_element, nr); }

  protected:
	WaypointsDb::Waypoint m_element;
};

class Engine::DbObjectMapelement : public DbObject {
  public:
	DbObjectMapelement(const MapelementsDb::Mapelement& el) : m_element(el) {}
	virtual Point get_coord(void) const { return m_element.get_coord(); }
	virtual const Glib::ustring& get_ident(void) const { return m_element.get_name(); }
	virtual const Glib::ustring& get_name(void) const { return m_element.get_name(); }
	virtual bool is_mapelement(void) const { return true; }
	using DbObject::get;
	virtual bool get(MapelementsDb::Mapelement& el) const { el = m_element; return true; }
	using DbObject::set;
	virtual bool set(FPlanWaypoint& el) const { el.set(m_element); return true; }
	using DbObject::insert;
	virtual unsigned int insert(FPlanRoute& route, uint32_t nr = ~0U) const { return route.insert(m_element, nr); }

  protected:
	MapelementsDb::Mapelement m_element;
};

class Engine::DbObjectFPlanWaypoint : public DbObject {
  public:
	DbObjectFPlanWaypoint(const FPlanWaypoint& el) : m_element(el) {}
	virtual Point get_coord(void) const { return m_element.get_coord(); }
	virtual const Glib::ustring& get_ident(void) const { return m_element.get_icao().empty() ? m_element.get_name() : m_element.get_icao(); }
	virtual const Glib::ustring& get_icao(void) const { return m_element.get_icao(); }
	virtual const Glib::ustring& get_name(void) const { return m_element.get_name(); }
	virtual bool is_fplwaypoint(void) const { return true; }
	using DbObject::get;
	virtual bool get(FPlanWaypoint& el) const { el = m_element; return true; }
	using DbObject::set;
	virtual bool set(FPlanWaypoint& el) const { el.set(m_element); return true; }
	using DbObject::insert;
	virtual unsigned int insert(FPlanRoute& route, uint32_t nr = ~0U) const { return route.insert(m_element, nr); }

  protected:
	FPlanWaypoint m_element;
};

const Glib::ustring Engine::DbObject::empty;

Engine::DbObject::DbObject(void)
	: m_refcount(1)
{
}

Engine::DbObject::~DbObject()
{
}

void Engine::DbObject::reference(void) const
{
        ++m_refcount;
}

void Engine::DbObject::unreference(void) const
{
        if (--m_refcount)
                return;
        delete this;
}


Glib::RefPtr<Engine::DbObject> Engine::DbObject::create(const AirportsDb::Airport& el)
{
	return Glib::RefPtr<Engine::DbObject>(new DbObjectAirport(el));
}

Glib::RefPtr<Engine::DbObject> Engine::DbObject::create(const NavaidsDb::Navaid& el)
{
	return Glib::RefPtr<Engine::DbObject>(new DbObjectNavaid(el));
}

Glib::RefPtr<Engine::DbObject> Engine::DbObject::create(const WaypointsDb::Waypoint& el)
{
	return Glib::RefPtr<Engine::DbObject>(new DbObjectWaypoint(el));
}

Glib::RefPtr<Engine::DbObject> Engine::DbObject::create(const MapelementsDb::Mapelement& el)
{
	return Glib::RefPtr<Engine::DbObject>(new DbObjectMapelement(el));
}

Glib::RefPtr<Engine::DbObject> Engine::DbObject::create(const FPlanWaypoint& el)
{
	return Glib::RefPtr<Engine::DbObject>(new DbObjectFPlanWaypoint(el));
}
