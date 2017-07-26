//
// C++ Implementation: xplaneexport.cc
//
// Description: X-Plane Export
//
//
// Author: Thomas Sailer <t.sailer@alumni.ethz.ch>, (C) 2007, 2009
//
// Copyright: See COPYING file that comes with this distribution
//
//

#include "sysdeps.h"

#include <libxml++/libxml++.h>
#include <unistd.h>
#include <getopt.h>
#include <iostream>
#include <sstream>
#include <fstream>
#include <sqlite3.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <errno.h>
#include <stdexcept>
#include <stdarg.h>
#include <libxml/encoding.h>
#include <libxml/xmlwriter.h>

#include "dbobj.h"
#include "wmm.h"

class XplaneWriter {
        public:
                XplaneWriter(const Glib::ustring& hdrmsg = "");
                ~XplaneWriter();
                void open(const Glib::ustring& fn);
                void close(void);
                void write_db(const Glib::ustring& dbfile);

        protected:
                std::ofstream m_os;
                bool m_writehdr;
                Glib::ustring m_headermsg;

                class ForEachAirport : public AirportsDb::ForEach {
                        public:
                                ForEachAirport(XplaneWriter& xmlw) : m_xplanew(xmlw) {}
                                bool operator()(const AirportsDb::Airport& e);
                                bool operator()(const std::string& id);

                        private:
                                XplaneWriter& m_xplanew;
                                static std::string convert_surface_code(const Glib::ustring& sfc);
                                static std::string convert_shoulder_code(const Glib::ustring& sfc);
                                static std::string rwy_center_light(const AirportsDb::Airport::Runway& rwy);
                                static std::string rwy_edge_light(const AirportsDb::Airport::Runway& rwy);
                                static std::string rwy_marking(const AirportsDb::Airport::Runway& rwy, bool he);
                                static std::string rwy_approach_lights(const AirportsDb::Airport::Runway& rwy, bool he);
                                static std::string rwy_touchdownzone_lights(const AirportsDb::Airport::Runway& rwy, bool he);
                                static std::string rwy_end_lights(const AirportsDb::Airport::Runway& rwy, bool he);
                                static std::string helipad_marking(const AirportsDb::Airport::Helipad& hp);
                };

                class ForEachNavaid : public NavaidsDb::ForEach {
                        public:
                                ForEachNavaid(XplaneWriter& xmlw) : m_xplanew(xmlw) {}
                                bool operator()(const NavaidsDb::Navaid& e);
                                bool operator()(const std::string& id);

                        private:
                                XplaneWriter& m_xplanew;
                };

                class ForEachWaypoint : public WaypointsDb::ForEach {
                        public:
                                ForEachWaypoint(XplaneWriter& xmlw) : m_xplanew(xmlw) {}
                                bool operator()(const WaypointsDb::Waypoint& e);
                                bool operator()(const std::string& id);

                        private:
                                XplaneWriter& m_xplanew;
                };

                class ForEachAirway : public AirwaysDb::ForEach {
                        public:
                                ForEachAirway(XplaneWriter& xmlw) : m_xplanew(xmlw) {}
                                bool operator()(const AirwaysDb::Airway& e);
                                bool operator()(const std::string& id);

                        private:
                                XplaneWriter& m_xplanew;
                };

                friend class ForEachAirport;
                friend class ForEachNavaid;
                friend class ForEachWaypoint;
                friend class ForEachAirway;

                void write(AirportsDb& db, bool include_aux = false) { ForEachAirport cb(*this); db.for_each(cb, include_aux); }
                void write(NavaidsDb& db, bool include_aux = false) { ForEachNavaid cb(*this); db.for_each(cb, include_aux); }
                void write(WaypointsDb& db, bool include_aux = false) { ForEachWaypoint cb(*this); db.for_each(cb, include_aux); }
                void write(AirwaysDb& db, bool include_aux = false) { ForEachAirway cb(*this); db.for_each(cb, include_aux); }
                void writehdr(const std::string& code);
                std::ostream& operator()(void) { return m_os; }
};

XplaneWriter::XplaneWriter(const Glib::ustring& hdrmsg)
        : m_os(), m_writehdr(true), m_headermsg(hdrmsg)
{
        m_os.exceptions(std::ofstream::failbit | std::ofstream::badbit);
}

void XplaneWriter::close(void)
{
        if (m_os.is_open()) {
                m_os << "99" << std::endl;
                m_os.close();
        }
}

void XplaneWriter::open(const Glib::ustring & fn)
{
        close();
        m_os.open(fn.c_str(), std::ofstream::out | std::ofstream::trunc);
        m_os << "I" << std::endl;
        m_writehdr = true;
}

void XplaneWriter::writehdr(const std::string & code)
{
        if (m_writehdr) {
                m_writehdr = false;
                m_os << code << " Version - " << m_headermsg << std::endl;
        }
}

/*
 HARD (PERMANENT) SURFACE

 ASP ASPHALT, ASPHALTIC CONCRETE, TAR MACADAM, OR BITUMEN BOUND MACADAM (INCLUDING ANY OF THESE SURFACE TYPES WITH CONCRETE ENDS).
 BRI BRICK, LAID OR MORTARED.
 CON CONCRETE.
 COP COMPOSITE, 50 PERCENT OR MORE OF THE RUNWAY LENGTH IS PERMANENT.
 PEM PART CONCRETE, PART ASPHALT, OR PART BITUMEN-BOUND MACADAM.
 PER PERMANENT, SURFACE TYPE UNKNOWN.

 SOFT(NON-PERMANENT) SURFACE

 BIT BITUMINOUS, TAR OR ASPHALT MIXED IN PLACE, OILED.
 CLA CLAY
 COM COMPOSITE, LESS THAN 50 PERCENT OF THE RUNWAY LENGTH IS PERMANENT.
 COR CORAL.
 GRE GRADED OR ROLLED EARTH, GRASS ON GRADED EARTH.
 GRS GRASS OR EARTH NOT GRADED OR ROLLED.
 GVL GRAVEL.
 ICE ICE.
 LAT LATERITE.
 MAC MACADAM - CRUSHED ROCK WATER BOUND.
 MEM MEMBRANE - PLASTIC OR OTHER COATED FIBER MATERIAL.
 MIX MIX IN PLACE USING NONBITUMIOUS BINDERS SUCH AS PORTLAND CEMENT
 PSP PIECED STEEL PLANKING.
 SAN SAND, GRADED, ROLLED OR OILED.
 SNO SNOW.
 U SURFACE UNKNOWN.
*/

std::string XplaneWriter::ForEachAirport::convert_surface_code(const Glib::ustring & sfc)
{
        Glib::ustring sfc1(sfc.uppercase());
        if (sfc1 == "ASP" || sfc1 == "BRI" || sfc1 == "COP" || sfc1 == "PEM" || sfc1 == "PER" || sfc1 == "BIT")
                return "01"; // Asphalt
        if (sfc1 == "CON")
                return "02"; // Concrete
        if (sfc1 == "GRE" || sfc1 == "GRS")
                return "03"; // Turf/grass
        if (sfc1 == "COM" || sfc1 == "COR" || sfc1 == "LAT" || sfc1 == "MAC" || sfc1 == "MEM" || sfc1 == "MIX" || sfc1 == "PSP" || sfc1 == "SAN")
                return "04"; // Dirt
        if (sfc1 == "CLA" || sfc1 == "GVL")
                return "05"; // Gravel
        //if (sfc1 == "")
        //        return "12"; // Dry lakebed runway (eg. at KEDW Edwards AFB)
        //if (sfc1 == "")
        //        return "13"; // Water runways
        if (sfc1 == "ICE" || sfc1 == "SNO")
                return "14"; // Snow or ice runways
        return "15"; // Transparent.  Implies a hard surface but with no texture
}

std::string XplaneWriter::ForEachAirport::convert_shoulder_code(const Glib::ustring & sfc)
{
        Glib::ustring sfc1(sfc.uppercase());
        if (sfc1 == "ASP" || sfc1 == "BRI" || sfc1 == "COP" || sfc1 == "PEM" || sfc1 == "PER" || sfc1 == "BIT")
                return "1"; // Asphalt
        if (sfc1 == "CON")
                return "2"; // Concrete
        return "0";
}

std::string XplaneWriter::ForEachAirport::rwy_center_light(const AirportsDb::Airport::Runway & rwy)
{
        for (unsigned int i = 0; i < AirportsDb::Airport::Runway::nr_lights; ++i) {
                AirportsDb::Airport::Runway::light_t ll(rwy.get_le_light(i));
                AirportsDb::Airport::Runway::light_t lh(rwy.get_he_light(i));
                if (ll == AirportsDb::Airport::Runway::light_centerrow || lh == AirportsDb::Airport::Runway::light_centerrow ||
                    ll == AirportsDb::Airport::Runway::light_centerrow2 || lh == AirportsDb::Airport::Runway::light_centerrow2 ||
                    ll == AirportsDb::Airport::Runway::light_centerrow3 || lh == AirportsDb::Airport::Runway::light_centerrow3 ||
                    ll == AirportsDb::Airport::Runway::light_centerrow4 || lh == AirportsDb::Airport::Runway::light_centerrow4)
                        return "1";
        }
        return "0";
}

std::string XplaneWriter::ForEachAirport::rwy_edge_light(const AirportsDb::Airport::Runway & rwy)
{
        for (unsigned int i = 0; i < AirportsDb::Airport::Runway::nr_lights; ++i) {
                AirportsDb::Airport::Runway::light_t ll(rwy.get_le_light(i));
                AirportsDb::Airport::Runway::light_t lh(rwy.get_he_light(i));
                if (ll == AirportsDb::Airport::Runway::light_hirl || lh == AirportsDb::Airport::Runway::light_hirl ||
                    ll == AirportsDb::Airport::Runway::light_mirl || lh == AirportsDb::Airport::Runway::light_mirl ||
                    ll == AirportsDb::Airport::Runway::light_lirl || lh == AirportsDb::Airport::Runway::light_lirl)
                        return "1";
        }
        return "0";
}

std::string XplaneWriter::ForEachAirport::rwy_marking(const AirportsDb::Airport::Runway & rwy, bool he)
{
        Glib::ustring sfc1(rwy.get_surface().uppercase());
        if (sfc1 == "ASP" || sfc1 == "BRI" || sfc1 == "COP" || sfc1 == "PEM" || sfc1 == "PER" || sfc1 == "BIT" || sfc1 == "CON")
                return "2";
        return "0";
}

std::string XplaneWriter::ForEachAirport::helipad_marking(const AirportsDb::Airport::Helipad & hp)
{
        Glib::ustring sfc1(hp.get_surface().uppercase());
        if (sfc1 == "ASP" || sfc1 == "BRI" || sfc1 == "COP" || sfc1 == "PEM" || sfc1 == "PER" || sfc1 == "BIT" || sfc1 == "CON")
                return "2";
        return "0";
}

std::string XplaneWriter::ForEachAirport::rwy_approach_lights(const AirportsDb::Airport::Runway & rwy, bool he)
{
        for (unsigned int i = 0; i < AirportsDb::Airport::Runway::nr_lights; ++i) {
                AirportsDb::Airport::Runway::light_t l(he ? rwy.get_he_light(i) : rwy.get_le_light(i));
                switch (l) {
                        case AirportsDb::Airport::Runway::light_alsf1:
                                return "1"; // ALSF-I (high intensity Approach Light System with sequenced Flashing lights)

                        case AirportsDb::Airport::Runway::light_alsf2:
                                return "2"; // ALSF-II (high intensity Approach Light System with sequenced Flashing lights and red side bar lights (barettes) the last 1000’, that align with touch down zone lighting.

                        case AirportsDb::Airport::Runway::light_calvert:
                                return "4"; // Calvert ILS Cat II and Cat II (British) (High intensity with red side bar lights (barettes) the last 1000’ - barettes align with touch down zone lighting)

                        case AirportsDb::Airport::Runway::light_ssalr:
                                return "5"; // SSALR (high intensity, Simplified Short Approach Light System with Runway Alignment Indicator Lights [RAIL])

                        //case AirportsDb::Airport::Runway:::
                        //        return "6"; // SSALF (high intensity, Simplified Short Approach Light System with sequenced flashing lights)

                        case AirportsDb::Airport::Runway::light_sals_salsf:
                                return "7"; // SALS (high intensity, Short Approach Light System)

                        case AirportsDb::Airport::Runway::light_malsr:
                                return "8"; // MALSR (Medium-intensity Approach Light System with Runway Alignment Indicator Lights [RAIL])

                        //case AirportsDb::Airport::Runway:::
                        //        return "9"; // MALSF (Medium-intensity Approach Light System with sequenced flashing lights)

                        case AirportsDb::Airport::Runway::light_mals_malsf_ssals_ssalsf:
                                return "10"; // MALS (Medium-intensity Approach Light System)

                        case AirportsDb::Airport::Runway::light_odals:
                                return "11"; // ODALS (Omni-directional approach light system) (flashing lights, not strobes, not sequenced)

                        case AirportsDb::Airport::Runway::light_rail:
                                return "12"; // RAIL (Runway Alignment Indicator Lights - sequenced strobes and green threshold lights, with no other approach lights)

                        default:
                                break;
                }
        }
        return "0";
}

std::string XplaneWriter::ForEachAirport::rwy_touchdownzone_lights(const AirportsDb::Airport::Runway & rwy, bool he)
{
        for (unsigned int i = 0; i < AirportsDb::Airport::Runway::nr_lights; ++i) {
                AirportsDb::Airport::Runway::light_t l(he ? rwy.get_he_light(i) : rwy.get_le_light(i));
                if (l == AirportsDb::Airport::Runway::light_tdzl)
                        return "2";
        }
        return "0";
}

std::string XplaneWriter::ForEachAirport::rwy_end_lights(const AirportsDb::Airport::Runway & rwy, bool he)
{
        for (unsigned int i = 0; i < AirportsDb::Airport::Runway::nr_lights; ++i) {
                AirportsDb::Airport::Runway::light_t l(he ? rwy.get_he_light(i) : rwy.get_le_light(i));
                if (l == AirportsDb::Airport::Runway::light_reil)
                        return "2";
        }
        return "0";
}

bool XplaneWriter::ForEachAirport::operator()(const AirportsDb::Airport & e)
{
        if (e.get_icao().empty())
                return true;
        m_xplanew.writehdr("850");
        m_xplanew() << ((!e.get_nrrwy() && e.get_nrhelipads()) ? "17 " : "1 ") << e.get_elevation() << " 0 0 " << e.get_icao() << ' ' << e.get_name() << std::endl;
        for (unsigned int irwy = 0; irwy < e.get_nrrwy(); ++irwy) {
                const AirportsDb::Airport::Runway& rwy(e.get_rwy(irwy));
                m_xplanew() << "100 " << (rwy.get_width() * Point::ft_to_m) << ' ' << convert_surface_code(rwy.get_surface()) << ' '
                            << convert_shoulder_code(rwy.get_surface()) << " 0.25 " << rwy_center_light(rwy) << ' ' << rwy_edge_light(rwy) << " 0"
                            // LE
                            << ' ' << rwy.get_ident_le() << ' ' << rwy.get_le_coord().get_lat_deg() << ' ' << rwy.get_le_coord().get_lon_deg()
                            << ' ' << (rwy.get_le_disp() * Point::ft_to_m) << " 0 " << rwy_marking(rwy, false)
                            << ' ' << rwy_approach_lights(rwy, false) << ' ' << rwy_touchdownzone_lights(rwy, false) << ' ' << rwy_end_lights(rwy, false)
                            // HE
                            << ' ' << rwy.get_ident_he() << ' ' << rwy.get_he_coord().get_lat_deg() << ' ' << rwy.get_he_coord().get_lon_deg()
                            << ' ' << (rwy.get_he_disp() * Point::ft_to_m) << " 0 " << rwy_marking(rwy, true)
                            << ' ' << rwy_approach_lights(rwy, true) << ' ' << rwy_touchdownzone_lights(rwy, true) << ' ' << rwy_end_lights(rwy, true)
                            << std::endl;
        }
        for (unsigned int ihp = 0; ihp < e.get_nrhelipads(); ++ihp) {
                const AirportsDb::Airport::Helipad& hp(e.get_helipad(ihp));
                m_xplanew() << "102 " << hp.get_ident() << ' ' << hp.get_coord().get_lat_deg() << ' ' << hp.get_coord().get_lon_deg()
                            << ' ' << (hp.get_hdg() * FPlanLeg::to_deg) << ' ' << (hp.get_length() * Point::ft_to_m) << ' ' << (hp.get_width() * Point::ft_to_m) 
                            << ' ' << convert_surface_code(hp.get_surface()) << ' ' << helipad_marking(hp) << ' ' << convert_shoulder_code(hp.get_surface())
                            << " 0.25 0" << std::endl;
        }
        for (unsigned int icomm = 0; icomm < e.get_nrcomm(); ++icomm) {
                const AirportsDb::Airport::Comm& comm(e.get_comm(icomm));
                m_xplanew() << "51 " << (comm[0] / 10000) << ' ' << comm.get_name() << std::endl;
        }
        return true;
}

bool XplaneWriter::ForEachAirport::operator()(const std::string & id)
{
        // do not support removed records
        return true;
}

bool XplaneWriter::ForEachNavaid::operator()(const NavaidsDb::Navaid & e)
{
        m_xplanew.writehdr("810");

        switch (e.get_navaid_type()) {
                case NavaidsDb::Navaid::navaid_vordme:
                        m_xplanew() << "12 " << e.get_coord().get_lat_deg() << ' ' << e.get_coord().get_lon_deg() << ' '
                                    << e.get_elev() << ' ' << e.get_frequency() / 10000 << ' ' << e.get_range()
                                    << " 0.0 " << e.get_icao() << ' ' << e.get_name() << std::endl;
                        // fall through

                case NavaidsDb::Navaid::navaid_vor:
                {
                        WMM wmm;
                        double var(0);
                        if (wmm.compute(e.get_elev() * (Point::ft_to_m * 1e-3), e.get_coord(), time(0)))
                                var = wmm.get_dec();
                        m_xplanew() << "3 " << e.get_coord().get_lat_deg() << ' ' << e.get_coord().get_lon_deg() << ' '
                                    << e.get_elev() << ' ' << e.get_frequency() / 10000 << ' ' << e.get_range()
                                    << ' ' << var << ' ' << e.get_icao() << ' ' << e.get_name() << std::endl;
                        break;
                }

                case NavaidsDb::Navaid::navaid_dme:
                        m_xplanew() << "13 " << e.get_coord().get_lat_deg() << ' ' << e.get_coord().get_lon_deg() << ' '
                                    << e.get_elev() << ' ' << e.get_frequency() / 10000 << ' ' << e.get_range()
                                    << " 0.0 " << e.get_icao() << ' ' << e.get_name() << std::endl;
                        break;

                case NavaidsDb::Navaid::navaid_ndb:
                        m_xplanew() << "2 " << e.get_coord().get_lat_deg() << ' ' << e.get_coord().get_lon_deg() << ' '
                                    << e.get_elev() << ' ' << e.get_frequency() / 1000 << ' ' << e.get_range()
                                    << " 0.0 " << e.get_icao() << ' ' << e.get_name() << std::endl;
                        break;

                default:
                        break;
        }
        return true;
}

bool XplaneWriter::ForEachNavaid::operator()(const std::string & id)
{
        // do not support removed records
        return true;
}

bool XplaneWriter::ForEachWaypoint::operator()(const WaypointsDb::Waypoint & e)
{
        m_xplanew.writehdr("600");
        m_xplanew() << e.get_coord().get_lat_deg() << ' ' << e.get_coord().get_lon_deg() << ' ' << e.get_icao() << std::endl;
        return true;
}

bool XplaneWriter::ForEachWaypoint::operator()(const std::string & id)
{
        // do not support removed records
        return true;
}

bool XplaneWriter::ForEachAirway::operator()(const AirwaysDb::Airway & e)
{
        m_xplanew.writehdr("640");
        if (e.get_type() == AirwaysDb::Airway::airway_low || e.get_type() == AirwaysDb::Airway::airway_both)
                m_xplanew() << e.get_begin_name() << e.get_begin_coord().get_lat_deg() << ' ' << e.get_begin_coord().get_lon_deg()
                            << ' ' << e.get_end_name() << e.get_end_coord().get_lat_deg() << ' ' << e.get_end_coord().get_lon_deg()
                            << " 1 " << e.get_base_level() << ' ' << e.get_top_level() << ' ' << e.get_name() << std::endl;
        if (e.get_type() == AirwaysDb::Airway::airway_high || e.get_type() == AirwaysDb::Airway::airway_both)
                m_xplanew() << e.get_begin_name() << e.get_begin_coord().get_lat_deg() << ' ' << e.get_begin_coord().get_lon_deg()
                            << ' ' << e.get_end_name() << e.get_end_coord().get_lat_deg() << ' ' << e.get_end_coord().get_lon_deg()
                            << " 2 " << e.get_base_level() << ' ' << e.get_top_level() << ' ' << e.get_name() << std::endl;
        return true;
}

bool XplaneWriter::ForEachAirway::operator()(const std::string & id)
{
        // do not support removed records
        return true;
}

void XplaneWriter::write_db(const Glib::ustring & dbfile)
{
        sqlite3x::sqlite3_connection db(dbfile);
        sqlite3x::sqlite3_command cmd(db, "SELECT name FROM sqlite_master WHERE type='table' AND name NOT LIKE 'sqlite%';");
        sqlite3x::sqlite3_cursor cursor(cmd.executecursor());
        while (cursor.step()) {
                std::string tblname(cursor.getstring(0));
                if (tblname == "airports") {
                        cursor.close();
                        cmd.finalize();
                        AirportsDb arptdb;
                        arptdb.DbBase<AirportsDb::element_t>::take(db.take());
                        write(arptdb);
                        return;
                }
                if (tblname == "navaids") {
                        cursor.close();
                        cmd.finalize();
                        NavaidsDb navdb;
                        navdb.DbBase<NavaidsDb::element_t>::take(db.take());
                        write(navdb);
                        return;
                }
                if (tblname == "waypoints") {
                        cursor.close();
                        cmd.finalize();
                        WaypointsDb wdb;
                        wdb.DbBase<WaypointsDb::element_t>::take(db.take());
                        write(wdb);
                        return;
                }
                if (tblname == "airways") {
                        cursor.close();
                        cmd.finalize();
                        AirwaysDb awdb;
                        awdb.DbBase<AirwaysDb::element_t>::take(db.take());
                        write(awdb);
                        return;
                }
        }
        throw std::runtime_error("cannot determine database type " + dbfile);
}

XplaneWriter::~XplaneWriter()
{
        close();
}

int main(int argc, char *argv[])
{
        static struct option long_options[] = {
                { "output", required_argument, 0, 'o' },
                {0, 0, 0, 0}
        };
        Glib::ustring outfile("out.xml");
        Glib::ustring hdrmsg;
        int c, err(0);

        while ((c = getopt_long(argc, argv, "o:m:", long_options, 0)) != EOF) {
                switch (c) {
                        case 'o':
                                if (optarg)
                                        outfile = optarg;
                                break;

                        case 'm':
                                if (optarg)
                                        hdrmsg = optarg;
                                break;

                        default:
                                err++;
                                break;
                }
        }
        if (err) {
                std::cerr << "usage: vfrdbxplaneexport [-o <fn>]" << std::endl;
                return EX_USAGE;
        }
        try {
                XplaneWriter xw(hdrmsg);
                xw.open(outfile);
                for (; optind < argc; optind++) {
                        std::cerr << "Parsing file " << argv[optind] << std::endl;
                        xw.write_db(argv[optind]);
                }
                xw.close();
        } catch (const std::exception& ex) {
                std::cerr << "exception: " << ex.what() << std::endl;
                return EX_DATAERR;
        }
        return EX_OK;
}
