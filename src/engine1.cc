/***************************************************************************
 *   Copyright (C) 2007, 2009, 2011, 2012 by Thomas Sailer                 *
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

#include "engine.h"
#include "maps.h"

template <typename T, typename LT>
void Preferences::PreferencesValue<T,LT>::operator=(datatype_t v)
{
	if (v == m_value)
		return;
	m_value = v;
	set_dirty();
	m_change(m_value);
}

const char Preferences::curplan_name[] = { "curplan" };
const char Preferences::vcruise_name[] = { "vcruise" };
const char Preferences::gps_name[] = { "gps" };
const char Preferences::autowpt_name[] = { "autowpt" };
const char Preferences::vblock_name[] = { "vblock" };
const char Preferences::maxalt_name[] = { "maxalt" };
const char Preferences::mapflags_name[] = { "mapflags" };
const char Preferences::mapscale_name[] = { "mapscale" };
const char Preferences::mapscaleairportdiagram_name[] = { "mapscaleairportdiagram" };
const char Preferences::globaldbdir_name[] = { "globaldbdir" };
const char Preferences::gpshost_name[] = { "gpshost" };
const char Preferences::gpsport_name[] = { "gpsport" };

Preferences::Preferences(void)
        : curplan(curplan_name),
          vcruise(vcruise_name),
          gps(gps_name),
          autowpt(autowpt_name),
          vblock(vblock_name),
          maxalt(maxalt_name),
          mapflags(mapflags_name),
          mapscale(mapscale_name),
          mapscaleairportdiagram(mapscaleairportdiagram_name),
          globaldbdir(globaldbdir_name),
          gpshost(gpshost_name),
          gpsport(gpsport_name)
{
        Glib::ustring dbname(Glib::build_filename(FPlan::get_userdbpath(), "prefs.db"));
        db.open(dbname);
        {
                sqlite3x::sqlite3_command cmd(db, "CREATE TABLE IF NOT EXISTS prefs (ID TEXT PRIMARY KEY NOT NULL, VALUE);");
                cmd.executenonquery();
        }
        load(curplan, -1);
        load(vcruise, 110.0);
        load(gps, true);
        load(autowpt, true);
        load(vblock, 40.0);
        load(maxalt, -1);
        load(mapflags, (long long)MapRenderer::drawflags_all);
        load(mapscale, 0.1);
        load(mapscaleairportdiagram, 0.01);
        load(globaldbdir, Engine::get_default_aux_dir());
        load(gpshost, "localhost");
#ifdef HAVE_GYPSY
	load(gpsport, "/dev/ttyUSB0");
#else
	load(gpsport, "2947");
#endif
        try {
                commit();
        } catch (...) {
        }
}

Preferences::~Preferences(void)
{
        try {
                commit();
        } catch (...) {
        }
}

template<class T> bool Preferences::load(T& t, const typename T::loadtype_t& dflt)
{
	typename T::loadtype_t x;
	bool r(loadpv(t.name(), x, dflt));
	t = x;
	if (r)
		t.clear_dirty();
	else
		t.set_dirty();
	return r;
}

template<class T> void Preferences::save(T& t)
{
	if (t.is_dirty())
		savepv(t.name(), (typename T::loadtype_t)t);
}

void Preferences::commit(void)
{
        sqlite3x::sqlite3_transaction tran(db, true);
        save(curplan);
        save(vcruise);
        save(gps);
        save(autowpt);
        save(vblock);
        save(maxalt);
        save(mapflags);
        save(mapscale);
        save(mapscaleairportdiagram);
        save(globaldbdir);
        save(gpshost);
        save(gpsport);
        tran.commit();
        curplan.clear_dirty();
        vcruise.clear_dirty();
        gps.clear_dirty();
        autowpt.clear_dirty();
        vblock.clear_dirty();
        maxalt.clear_dirty();
        mapflags.clear_dirty();
        mapscale.clear_dirty();
        mapscaleairportdiagram.clear_dirty();
        globaldbdir.clear_dirty();
        gpshost.clear_dirty();
        gpsport.clear_dirty();
}

const std::string Preferences::insert_command = "INSERT OR REPLACE INTO prefs (ID,VALUE) VALUES (?,?);";
const std::string Preferences::select_command = "SELECT VALUE FROM prefs WHERE ID = ?;";

void Preferences::savepv(const char *name, long long v)
{
        sqlite3x::sqlite3_command cmd(db, insert_command);
        cmd.bind(1, name);
        cmd.bind(2, v);
        cmd.executenonquery();
        std::cerr << "Preferences::save: name " << name << " value " << v << std::endl;
}

void Preferences::savepv(const char *name, const std::string & v)
{
        sqlite3x::sqlite3_command cmd(db, insert_command);
        cmd.bind(1, name);
        cmd.bind(2, v);
        cmd.executenonquery();
        std::cerr << "Preferences::save: name " << name << " value " << v << std::endl;
}

void Preferences::savepv(const char *name, double v)
{
        sqlite3x::sqlite3_command cmd(db, insert_command);
        cmd.bind(1, name);
        cmd.bind(2, v);
        cmd.executenonquery();
        std::cerr << "Preferences::save: name " << name << " value " << v << std::endl;
}

bool Preferences::loadpv(const char *name, long long& v, long long dflt)
{
        sqlite3x::sqlite3_command cmd(db, select_command);
        cmd.bind(1, name);
        sqlite3x::sqlite3_cursor cursor = cmd.executecursor();
        if (!cursor.step()) {
		v = dflt;
		if (false)
	                std::cerr << "Preferences::load: name " << name << " value not found (default " << v << ")" << std::endl;
                return false;
        }
        v = cursor.getint64(0);
	if (false)
	        std::cerr << "Preferences::load: name " << name << " value " << v << std::endl;
	return true;
}

bool Preferences::loadpv(const char *name, std::string& v, const std::string& dflt)
{
        sqlite3x::sqlite3_command cmd(db, select_command);
        cmd.bind(1, name);
        sqlite3x::sqlite3_cursor cursor = cmd.executecursor();
        if (!cursor.step()) {
		v = dflt;
		if (false)
			std::cerr << "Preferences::load: name " << name << " value not found (default " << v << ")" << std::endl;
                return false;
        }
        v = cursor.getstring(0);
	if (false)
	        std::cerr << "Preferences::load: name " << name << " value " << v << std::endl;
	return true;
}

bool Preferences::loadpv(const char *name, Glib::ustring& v, const Glib::ustring& dflt)
{
	std::string v1;
	bool r(loadpv(name, v1, dflt));
	v = v1;
	return r;
}

bool Preferences::loadpv(const char *name, double& v, double dflt)
{
        sqlite3x::sqlite3_command cmd(db, select_command);
        cmd.bind(1, name);
        sqlite3x::sqlite3_cursor cursor = cmd.executecursor();
        if (!cursor.step()) {
		v = dflt;
		if (false)
			std::cerr << "Preferences::load: name " << name << " value not found (default " << v << ")" << std::endl;
                return false;
        }
        v = cursor.getdouble(0);
	if (false)
	        std::cerr << "Preferences::load: name " << name << " value " << v << std::endl;
	return true;
}

template class Preferences::PreferencesValue<double>;
template class Preferences::PreferencesValue<bool, long long>;
template class Preferences::PreferencesValue<Glib::ustring>;
template class Preferences::PreferencesValue<int, long long>;
