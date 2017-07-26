//
// C++ Implementation: camelcase
//
// Description: CamelCase databases intelligently
//
//
// Author: Thomas Sailer <t.sailer@alumni.ethz.ch>, (C) 2007, 2009
//
// Copyright: See COPYING file that comes with this distribution
//
//

#include "sysdeps.h"

#include <unistd.h>
#include <getopt.h>
#include <iostream>
#include <sstream>
#include <sqlite3.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <errno.h>
#include <stdexcept>
#include <stdarg.h>

#include "dbobj.h"

class CamelCase {
        public:
                CamelCase(bool logconv = false);
                ~CamelCase();
                void convert_db(const Glib::ustring& dbfile);

        private:
                void convert(AirportsDb& db);
                void convert(AirspacesDb& db);
                void convert(AirwaysDb& db);
                void convert(NavaidsDb& db);
                void convert(WaypointsDb& db);
                void convert(MapelementsDb& db);
                void convert(TracksDb& db);

                typedef std::pair<Glib::ustring,bool> convcaseresult_t;
                convcaseresult_t convcase(const Glib::ustring& s) const;

                void addfixed(const Glib::ustring& s);
                Glib::ustring getfixed(const Glib::ustring& s) const;

                typedef std::map<Glib::ustring,Glib::ustring> fixed_t;
                fixed_t m_fixed;

                bool m_logconv;
};

CamelCase::CamelCase(bool logconv)
        : m_logconv(logconv)
{
        addfixed("TWR");
        addfixed("GND");
        addfixed("APRON");
        addfixed("RDO");
        addfixed("APP");
        addfixed("ARR");
        addfixed("DEP");
        addfixed("OPS");
        addfixed("HX");
        addfixed("ATIS");
        addfixed("INFO");
        addfixed("GCA");
        addfixed("CNTR");
        addfixed("MCTR");
        addfixed("CTR");
        addfixed("SCTR");
        addfixed("MTCA");
        addfixed("TCA");
        addfixed("MTMA");
        addfixed("TMA");
        addfixed("MCAS");
        addfixed("MCTLZ");
        addfixed("CTLZ");
        addfixed("CTA");
        addfixed("CTAF");
        addfixed("CLNC");
        addfixed("DEL");
        addfixed("ALTN");
        addfixed("RADAR");
        addfixed("CON");
        addfixed("UTA");
        addfixed("UIR");
        addfixed("FIR");
        addfixed("FIS");
        addfixed("FIC");
        addfixed("MIL");
        addfixed("VDF");
        addfixed("UNICOM");
        addfixed("TRML");
        addfixed("ADVSY");
        addfixed("MSL");
        addfixed("ASL");
        addfixed("AMSL");
        addfixed("ETA");
        addfixed("ETD");
        addfixed("nm");
        addfixed("ft");
        addfixed("lbs");
        addfixed("MF");
        addfixed("SR");
        addfixed("SS");
        addfixed("OPR");
        addfixed("HR");
        addfixed("VOR");
        addfixed("DME");
        addfixed("NDB");
        addfixed("TACAN");
        addfixed("GPS");
        addfixed("BP");
        addfixed("QNH");
        addfixed("QDF");
        addfixed("ACC");
        addfixed("VFR");
        addfixed("IFR");
        addfixed("VMC");
        addfixed("IMC");
        addfixed("SVC");
        addfixed("RWY");
        addfixed("AB");
        addfixed("AMCC");
        addfixed("AFB");
        addfixed("SRA");
        addfixed("RVSM");
        addfixed("NOTAM");
        addfixed("ADIZ");
        addfixed("AD");
        addfixed("ILS");
        addfixed("IM");
        addfixed("MM");
        addfixed("OM");
}

CamelCase::~CamelCase()
{
}

void CamelCase::addfixed(const Glib::ustring & s)
{
        m_fixed.insert(fixed_t::value_type(s.casefold(), s));
}

Glib::ustring CamelCase::getfixed(const Glib::ustring & s) const
{
        fixed_t::const_iterator i(m_fixed.find(s.casefold()));
        if (i == m_fixed.end())
                return Glib::ustring();
        return i->second;
}

void CamelCase::convert_db(const Glib::ustring & dbfile)
{
        sqlite3x::sqlite3_connection db(dbfile);
        sqlite3x::sqlite3_command cmd(db, "SELECT name FROM sqlite_master WHERE type='table' AND name NOT LIKE 'sqlite%';");
        sqlite3x::sqlite3_cursor cursor(cmd.executecursor());
        while (cursor.step()) {
                std::string tblname(cursor.getstring(0));
                if (tblname == "fplan") {
                        cursor.close();
                        throw std::runtime_error("fplan databases not supported");
                }
                if (tblname == "airports") {
                        cursor.close();
                        cmd.finalize();
                        AirportsDb arptdb;
                        arptdb.DbBase<AirportsDb::element_t>::take(db.take());
                        convert(arptdb);
                        return;
                }
                if (tblname == "airspaces") {
                        cursor.close();
                        cmd.finalize();
                        AirspacesDb adb;
                        adb.DbBase<AirspacesDb::element_t>::take(db.take());
                        convert(adb);
                        return;
                }
                if (tblname == "airways") {
                        cursor.close();
                        cmd.finalize();
                        AirwaysDb adb;
                        adb.DbBase<AirwaysDb::element_t>::take(db.take());
                        convert(adb);
                        return;
                }
                if (tblname == "navaids") {
                        cursor.close();
                        cmd.finalize();
                        NavaidsDb navdb;
                        navdb.DbBase<NavaidsDb::element_t>::take(db.take());
                        convert(navdb);
                        return;
                }
                if (tblname == "waypoints") {
                        cursor.close();
                        cmd.finalize();
                        WaypointsDb wdb;
                        wdb.DbBase<WaypointsDb::element_t>::take(db.take());
                        convert(wdb);
                        return;
                }
                if (tblname == "mapelements") {
                        cursor.close();
                        cmd.finalize();
                        MapelementsDb wdb;
                        wdb.DbBase<MapelementsDb::element_t>::take(db.take());
                        convert(wdb);
                        return;
                }
                if (tblname == "tracks") {
                        cursor.close();
                        cmd.finalize();
                        TracksDb wdb;
                        wdb.DbBase<TracksDb::element_t>::take(db.take());
                        convert(wdb);
                        return;
                }
        }
        throw std::runtime_error("cannot determine database type " + dbfile);
}

void CamelCase::convert(AirportsDb & db)
{
        AirportsDb::Airport e;
        db.loadfirst(e, false, AirportsDb::Airport::subtables_all);
        unsigned int nrchanges(0), nrtotal(0);
        while (e.is_valid()) {
                ++nrtotal;
                bool dirty(false);
                convcaseresult_t r(convcase(e.get_name()));
                if (r.second) {
                        e.set_name(r.first);
                        dirty = true;
                }
                r = convcase(e.get_vfrrmk());
                if (r.second) {
                        e.set_vfrrmk(r.first);
                        dirty = true;
                }
                for (unsigned int i = 0; i < e.get_nrcomm(); i++) {
                        AirportsDb::Airport::Comm& comm(e.get_comm(i));
                        r = convcase(comm.get_name());
                        if (r.second) {
                                comm.set_name(r.first);
                                dirty = true;
                        }
                        r = convcase(comm.get_sector());
                        if (r.second) {
                                comm.set_sector(r.first);
                                dirty = true;
                        }
                        r = convcase(comm.get_oprhours());
                        if (r.second) {
                                comm.set_oprhours(r.first);
                                dirty = true;
                        }
                }
                for (unsigned int i = 0; i < e.get_nrvfrrte(); i++) {
                        AirportsDb::Airport::VFRRoute& rte(e.get_vfrrte(i));
                        r = convcase(rte.get_name());
                        if (r.second) {
                                rte.set_name(r.first);
                                dirty = true;
                        }
                        for (unsigned int j = 0; j < rte.size(); j++) {
                                AirportsDb::Airport::VFRRoute::VFRRoutePoint& pt(rte[j]);
                                r = convcase(pt.get_name());
                                if (r.second) {
                                        pt.set_name(r.first);
                                        dirty = true;
                                }

                        }
                }
                if (dirty) {
                        db.save(e);
                        ++nrchanges;
                }
                db.loadnext(e, false, AirportsDb::Airport::subtables_all);
        }
        std::cerr << nrchanges << " of " << nrtotal << " records changed" << std::endl;
}

void CamelCase::convert(AirspacesDb & db)
{
        AirspacesDb::Airspace e;
        db.loadfirst(e, false, AirspacesDb::Airspace::subtables_all);
        unsigned int nrchanges(0), nrtotal(0);
        while (e.is_valid()) {
                ++nrtotal;
                bool dirty(false);
                convcaseresult_t r(convcase(e.get_name()));
                if (r.second) {
                        e.set_name(r.first);
                        dirty = true;
                }
                r = convcase(e.get_classexcept());
                if (r.second) {
                        e.set_classexcept(r.first);
                        dirty = true;
                }
                r = convcase(e.get_efftime());
                if (r.second) {
                        e.set_efftime(r.first);
                        dirty = true;
                }
                r = convcase(e.get_note());
                if (r.second) {
                        e.set_note(r.first);
                        dirty = true;
                }
                r = convcase(e.get_commname());
                if (r.second) {
                        e.set_commname(r.first);
                        dirty = true;
                }
                if (dirty) {
                        db.save(e);
                        ++nrchanges;
                }
                db.loadnext(e, false, AirspacesDb::Airspace::subtables_all);
        }
        std::cerr << nrchanges << " of " << nrtotal << " records changed" << std::endl;
}

void CamelCase::convert(AirwaysDb & db)
{
        AirwaysDb::Airway e;
        db.loadfirst(e, false, AirwaysDb::Airway::subtables_all);
        unsigned int nrchanges(0), nrtotal(0);
        while (e.is_valid()) {
                ++nrtotal;
                bool dirty(false);
#if 0
                convcaseresult_t r(convcase(e.get_name()));
                if (r.second) {
                        e.set_name(r.first);
                        dirty = true;
                }
#endif
                if (dirty) {
                        db.save(e);
                        ++nrchanges;
                }
                db.loadnext(e, false, AirwaysDb::Airway::subtables_all);
        }
        std::cerr << nrchanges << " of " << nrtotal << " records changed" << std::endl;
}

void CamelCase::convert(NavaidsDb & db)
{
        NavaidsDb::Navaid e;
        db.loadfirst(e, false, NavaidsDb::Navaid::subtables_all);
        unsigned int nrchanges(0), nrtotal(0);
        while (e.is_valid()) {
                ++nrtotal;
                bool dirty(false);
                convcaseresult_t r(convcase(e.get_name()));
                if (r.second) {
                        e.set_name(r.first);
                        dirty = true;
                }
                if (dirty) {
                        db.save(e);
                        ++nrchanges;
                }
                db.loadnext(e, false, NavaidsDb::Navaid::subtables_all);
        }
        std::cerr << nrchanges << " of " << nrtotal << " records changed" << std::endl;
}

void CamelCase::convert(WaypointsDb & db)
{
        WaypointsDb::Waypoint e;
        db.loadfirst(e, false, WaypointsDb::Waypoint::subtables_all);
        unsigned int nrchanges(0), nrtotal(0);
        while (e.is_valid()) {
                ++nrtotal;
                bool dirty(false);
                convcaseresult_t r(convcase(e.get_name()));
                if (r.second) {
                        e.set_name(r.first);
                        dirty = true;
                }
                if (dirty) {
                        db.save(e);
                        ++nrchanges;
                }
                db.loadnext(e, false, WaypointsDb::Waypoint::subtables_all);
        }
        std::cerr << nrchanges << " of " << nrtotal << " records changed" << std::endl;
}

void CamelCase::convert(MapelementsDb & db)
{
        MapelementsDb::Mapelement e;
        db.loadfirst(e, false, MapelementsDb::Mapelement::subtables_all);
        unsigned int nrchanges(0), nrtotal(0);
        while (e.is_valid()) {
                ++nrtotal;
                bool dirty(false);
                convcaseresult_t r(convcase(e.get_name()));
                if (r.second) {
                        e.set_name(r.first);
                        dirty = true;
                }
                if (dirty) {
                        db.save(e);
                        ++nrchanges;
                }
                db.loadnext(e, false, MapelementsDb::Mapelement::subtables_all);
        }
        std::cerr << nrchanges << " of " << nrtotal << " records changed" << std::endl;
}

void CamelCase::convert(TracksDb & db)
{
        TracksDb::Track e;
        db.loadfirst(e, false, TracksDb::Track::subtables_all);
        unsigned int nrchanges(0), nrtotal(0);
        while (e.is_valid()) {
                ++nrtotal;
                bool dirty(false);
                convcaseresult_t r(convcase(e.get_fromname()));
                if (r.second) {
                        e.set_fromname(r.first);
                        dirty = true;
                }
                r = convcase(e.get_toname());
                if (r.second) {
                        e.set_toname(r.first);
                        dirty = true;
                }
                r = convcase(e.get_notes());
                if (r.second) {
                        e.set_notes(r.first);
                        dirty = true;
                }
                if (dirty) {
                        db.save(e);
                        ++nrchanges;
                }
                db.loadnext(e, false, TracksDb::Track::subtables_all);
        }
        std::cerr << nrchanges << " of " << nrtotal << " records changed" << std::endl;
}

CamelCase::convcaseresult_t CamelCase::convcase(const Glib::ustring & s) const
{
        {
                bool uppercase(false);
                for (Glib::ustring::const_iterator si(s.begin()), se(s.end()); si != se; ++si) {
                        if (Glib::Unicode::islower(*si))
                                return convcaseresult_t(s, false);
                        if (Glib::Unicode::isupper(*si))
                                uppercase = true;
                }
                if (!uppercase)
                        return convcaseresult_t(s, false);
        }
        convcaseresult_t r("", false);
        for (Glib::ustring::const_iterator si(s.begin()), se(s.end()); si != se;) {
                if (Glib::Unicode::isdigit(*si)) {
                        do {
                                r.first += *si;
                                ++si;
                        } while (si != se && Glib::Unicode::isalnum(*si));
                        continue;
                }
                if (!Glib::Unicode::isalpha(*si)) {
                        r.first += *si;
                        ++si;
                        continue;
                }
                // check for fixed case words
                {
                        Glib::ustring::const_iterator si2(si);
                        ++si2;
                        bool hasnum(false);
                        while (si2 != se && Glib::Unicode::isalnum(*si2)) {
                                hasnum |= Glib::Unicode::isdigit(*si2);
                                ++si2;
                        }
                        Glib::ustring ss(si, si2);
                        Glib::ustring ss2(getfixed(ss));
                        if (!ss2.empty()) {
                                si = si2;
                                r.first += ss2;
                                r.second |= ss != ss2;
                                continue;
                        }
                        if (hasnum) {
                                si = si2;
                                r.first += ss;
                                continue;
                        }
                        if (ss.length() > 2 && ss[0] == 'M' && ss[1] == 'C') {
                                r.first += "Mc";
                                ++si;
                                ++si;
                                continue;
                        }
                }
                r.first += *si;
                ++si;
                while (si != se && Glib::Unicode::isalnum(*si)) {
                        r.second |= !!Glib::Unicode::isupper(*si);
                        r.first += Glib::Unicode::tolower(*si);
                        ++si;
                }
        }
        if (m_logconv && r.second)
                std::cout << "\"" << s << "\" -> \"" << r.first << "\"" << std::endl;
        if (s.casefold() != r.first.casefold())
                throw std::runtime_error("string conversion error");
        return r;
}



int main(int argc, char *argv[])
{
        static struct option long_options[] = {
                {"logconv", no_argument, 0, 'l' },
                {0, 0, 0, 0}
        };
        bool logconv(false);
        int c, err(0);

        while ((c = getopt_long(argc, argv, "o:l", long_options, 0)) != EOF) {
                switch (c) {
                        case 'l':
                                logconv = true;
                                break;

                        default:
                                err++;
                                break;
                }
        }
        if (err) {
                std::cerr << "usage: vfrdbcamelcase [-l] <dbfile(s)>" << std::endl;
                return 1;
        }
        try {
                CamelCase cc(logconv);
                for (; optind < argc; optind++) {
                        std::cerr << "Converting file " << argv[optind] << std::endl;
                        cc.convert_db(argv[optind]);
                }
        } catch (const std::exception& ex) {
                std::cerr << "exception: " << ex.what() << std::endl;
                return 1;
        }
        return 0;
}
