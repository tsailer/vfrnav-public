/*
 * xplane.cc:  xplane to DB conversion utility
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation; either version 2 of the License, or (at your
 * option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General
 * Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program; if not, write to the Free Software Foundation, Inc.,
 * 59 Temple Place, Suite 330, Boston, MA 02111-1307 USA
 *
 */

#include "sysdeps.h"

#include <getopt.h>
#include <iostream>
#include <sstream>
#include <limits>
#include <sqlite3x.hpp>
#include <sqlite3.h>
#include <unistd.h>
#include <stdexcept>
#include <glibmm.h>
#include <giomm.h>

#include "geom.h"
#include "dbobj.h"


/* ---------------------------------------------------------------------- */

class XPlaneParser {
public:
	XPlaneParser(const Glib::ustring& output_dir = ".", bool purgedb = false);
	~XPlaneParser();
	void parse_file(const Glib::ustring& fn);

protected:
	Glib::RefPtr<Glib::IOChannel> m_chan;
	Glib::IOStatus m_status;
	time_t m_filetime;
	typedef std::vector<Glib::ustring> fld_t;
	fld_t m_fld;
	int m_rectype;
	typedef std::map<int,int> reclength_t;
	reclength_t m_reclength;
	Glib::ustring m_outputdir;
	bool m_purgedb;
	bool m_airportsdbopen;
	AirportsDb m_airportsdb;
	bool m_navaidsdbopen;
	NavaidsDb m_navaidsdb;
	Glib::ustring m_navaidlastname;
	unsigned int m_navaidlastcount;
	bool m_waypointsdbopen;
	WaypointsDb m_waypointsdb;
	Glib::ustring m_waypointlastname;
	unsigned int m_waypointlastcount;
	AirwaysDb m_airwaysdb;
	bool m_airwaysdbopen;

	bool checkheader(void);
	bool getline(Glib::ustring& ln);
	static void trim(Glib::ustring& s);
	static void removectrlchars(Glib::ustring& s);
	void open(const Glib::ustring& fn);
	void close(void);
	time_t get_filetime(void) const { return m_filetime; }
	operator bool(void) const;
	bool nextrec(int nrfields = -1);
	int rectype(void) const { return m_rectype; }
	unsigned int size(void) const { return m_fld.size(); }
	const Glib::ustring& get_string(unsigned int x) const { return m_fld[x]; }
	double get_double(unsigned int x) const;
	unsigned int get_uint(unsigned int x) const;
	int get_int(unsigned int x) const;
	Point get_point(unsigned int lonidx, unsigned int latidx) const;
	unsigned int get_frequency(unsigned int x) const;
	std::string get_line(void) const;
	bool parse_airport(void);
	bool parse_navaid(void);
	void parse_waypoint(void);
	void parse_airway(void);
	static Glib::ustring convert_surface_code(unsigned int code);
	void save_airport(AirportsDb::Airport& e);
	static void split_beziernodes(AirportsDb::Airport::Polyline& pl);
};

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

Glib::ustring XPlaneParser::convert_surface_code(unsigned int code)
{
        switch (code) {
        case 1:
                return "ASP";

        case 2:
                return "CON";

        case 3:
                return "GRS";

        case 4:
                return "SAN";

        case 5:
                return "GVL";

        case 12: // Dry lakebed runway (eg. at KEDW Edwards AFB).
                return "LAK";

        case 13: // Water runways.  Nothing will be displayed in X-Plane.  May use buoys as edge markings.
                return "WTR";

        case 14: // Snow or ice runways.
                return "ICE";

        case 15: // Transparent.  Implies a hard surface but with no texture
                return "TRA";

        default:
                return "U";
        }
}

XPlaneParser::XPlaneParser(const Glib::ustring& output_dir, bool purgedb)
        : m_status(Glib::IO_STATUS_ERROR), m_filetime(0), m_rectype(-1), m_outputdir(output_dir), m_purgedb(purgedb),
          m_airportsdbopen(false), m_navaidsdbopen(false), m_navaidlastname(), m_navaidlastcount(0),
          m_waypointsdbopen(false), m_waypointlastname(), m_waypointlastcount(0), m_airwaysdbopen(false)
{
        // Navaids
        m_reclength[2] = 8; // NDB
        m_reclength[3] = 8; // VOR, VORTAC or VOR-DME
        m_reclength[4] = 8; // Localiser that is part of a full ILS
        m_reclength[5] = 8; // Stand-alone localiser (LOC), also including a LDA (Landing Directional Aid) or SDF (Simplified Directional Facility)
        m_reclength[6] = 8; // Glideslope (GS)
        m_reclength[7] = 8; // Outer marker (OM)
        m_reclength[8] = 8; // Middle marker (MM)
        m_reclength[9] = 8; // Inner marker (IM)
        m_reclength[12] = 8; // DME (suppressed frequency)
        m_reclength[13] = 8; // DME (displayed frequency)
        // Airports
        m_reclength[10] = 15; // Runway or taxiway at an airport.
        m_reclength[1] = 5; // Airport header data (unchanged from 810 version)
        m_reclength[16] = 5; // Seaplane base header data
        m_reclength[17] = 5; // Heliport header data
        m_reclength[100] = 25; // Runway.
        m_reclength[101] = 8; // Seaplane base water runway.
        m_reclength[102] = 11; // Helipad.
        m_reclength[110] = 4; // Pavement header.  (The definition of a pavement chunk must be a closed loop - it must end in a node type 113  or 114.)
        m_reclength[120] = 1; // Line header.  (These definitions may be strings or closed loops.)
        m_reclength[130] = 1; // Airport boundary header (must be a closed loop - it must end in a node type 113  or 114.)
        m_reclength[111] = 100; // Node
        m_reclength[112] = 100; // Node with Bezier control point.
        m_reclength[113] = 100; // Node (close loop) point (eg. to close a pavement chunk boundary).
        m_reclength[114] = 100; // Node (close loop) point with Bezier control point (eg. to close a pavement chunk boundary).
        m_reclength[115] = 100; // Node (end) point to terminate a linear feature.
        m_reclength[116] = 100; // Node (end) point with Bezier control point (to terminate a linear feature).
        m_reclength[14] = 5; // Tower view location.
        m_reclength[15] = 4; // Ramp startup positions.
        m_reclength[18] = 4; // Airport light beacons (usually "rotating beacons" in the USA).  Different colours may be defined.
        m_reclength[19] = 4; // Airport windsocks.
        m_reclength[20] = 6; // Taxiway sign.
        m_reclength[21] = 7; // Lightning objects, including VASI, PAPI and wig-wags.
        m_reclength[0] = 2;
        m_reclength[50] = 2; // Airport ATC (Air Traffic Control) frequencies. AWOS (Automatic Weather Observation System), ASOS (Automatic Surface Observation System) or ATIS (Automated Terminal Information System).
        m_reclength[51] = 2; // Airport ATC (Air Traffic Control) frequencies. Unicom or CTAF (USA), radio (UK) - open channel for pilot position reporting at uncontrolled airports.
        m_reclength[52] = 2; // Airport ATC (Air Traffic Control) frequencies. Clearance delivery (CLD).
        m_reclength[53] = 2; // Airport ATC (Air Traffic Control) frequencies. Ground.
        m_reclength[54] = 2; // Airport ATC (Air Traffic Control) frequencies. Tower.
        m_reclength[55] = 2; // Airport ATC (Air Traffic Control) frequencies. Approach.
        m_reclength[56] = 2; // Airport ATC (Air Traffic Control) frequencies. Departure.
	m_reclength[1000] = 1; // Traffic Flow
	m_reclength[1001] = 4; // Traffic Flow Wind Rule
	m_reclength[1002] = 2; // Traffic Flow Ceiling Rule
	m_reclength[1003] = 2; // Traffic Flow Visibility Rule
	m_reclength[1004] = 2; // Traffic Flow Time Rule
	m_reclength[1100] = 7; // Runway in Use Rule
	m_reclength[1101] = 2; // VFR Pattern Rule
	m_reclength[1200] = 0; // Taxi Routing Network
	m_reclength[1201] = 5; // Taxi Routing Node
	m_reclength[1202] = 5; // Taxi Routing Edge
	m_reclength[1204] = 2; // Edge Active Zone
	m_reclength[1300] = 6; // Taxi Location
}

XPlaneParser::~XPlaneParser()
{
        if (m_airportsdbopen) {
                m_airportsdb.analyze();
                m_airportsdb.vacuum();
                m_airportsdb.close();
        }
        m_airportsdbopen = false;
        if (m_navaidsdbopen) {
                m_navaidsdb.analyze();
                m_navaidsdb.vacuum();
                m_navaidsdb.close();
        }
        m_navaidsdbopen = false;
        if (m_waypointsdbopen) {
                m_waypointsdb.analyze();
                m_waypointsdb.vacuum();
                m_waypointsdb.close();
        }
        m_waypointsdbopen = false;
        if (m_airwaysdbopen) {
                m_airwaysdb.analyze();
                m_airwaysdb.vacuum();
                m_airwaysdb.close();
        }
        m_airwaysdbopen = false;
}

void XPlaneParser::open(const Glib::ustring& fn)
{
        m_fld.clear();
        m_status = Glib::IO_STATUS_ERROR;
#ifdef GLIBMM_EXCEPTIONS_ENABLED
        m_chan = Glib::IOChannel::create_from_file(fn, "r");
        m_chan->set_encoding("ISO8859-1");
#else
        std::auto_ptr<Glib::Error> ex;
        m_chan = Glib::IOChannel::create_from_file(fn, "r", ex);
        if (ex.get())
                throw *ex;
        m_chan->set_encoding("ISO8859-1", ex);
        if (ex.get())
                throw *ex;
#endif
        m_filetime = time(0);
        try {
                Glib::RefPtr<Gio::File> df(Gio::File::create_for_path(fn));
		Glib::RefPtr<Gio::FileInfo> fi(df->query_info(G_FILE_ATTRIBUTE_TIME_MODIFIED, Gio::FILE_QUERY_INFO_NONE));
                m_filetime = fi->modification_time().tv_sec;
        } catch (const Gio::Error& e) {
                std::cerr << "cannot get modified time of file " << fn << "(" << e.what() << "), using current time" << std::endl;
        }
        // read header record
        if (!checkheader())
                throw std::runtime_error("XPlaneParser: cannot read header");
}

void XPlaneParser::close(void)
{
        m_fld.clear();
        m_status = Glib::IO_STATUS_ERROR;
        m_chan.clear();
}

XPlaneParser::operator bool(void) const
{
        return m_status == Glib::IO_STATUS_NORMAL;
}

void XPlaneParser::trim(Glib::ustring & s)
{
        while (!s.empty() && Glib::Unicode::isspace(*s.begin()))
                s.erase(s.begin());
        while (!s.empty() && Glib::Unicode::isspace(*--s.end()))
                s.erase(--s.end());
}

void XPlaneParser::removectrlchars(Glib::ustring& s)
{
	for (Glib::ustring::iterator i(s.begin()); i != s.end(); ) {
		if (*i >= ' ') {
			++i;
			continue;
		}
		i = s.erase(i);
	}
}

bool XPlaneParser::getline(Glib::ustring& ln)
{
        ln.clear();
        if (!m_chan) {
                m_status = Glib::IO_STATUS_ERROR;
                return false;
        }
#ifdef GLIBMM_EXCEPTIONS_ENABLED
        m_status = m_chan->read_line(ln);
#else
        std::auto_ptr<Glib::Error> ex;
        m_status = m_chan->read_line(ln, ex);
        if (ex.get())
                throw *ex;
#endif
        if (m_status != Glib::IO_STATUS_NORMAL)
                return false;
        return true;
}

bool XPlaneParser::checkheader(void)
{
        Glib::ustring buf;
        if (!getline(buf))
                return false;
        trim(buf);
        if (buf != "I")
                throw std::runtime_error("XPlane file header not recognized");
        if (!nextrec())
                return false;
        if (rectype() != 1000 && rectype() != 850 && rectype() != 810 && rectype() != 600 && rectype() != 640) {
                std::ostringstream oss;
                oss << "Unsupported XPlane database version " << rectype();
                throw std::runtime_error(oss.str());
        }
        return true;
}

bool XPlaneParser::nextrec(int nrfields)
{
        m_rectype = -1;
        m_fld.clear();
        Glib::ustring buf;
        do {
                if (!getline(buf))
                        return false;
                trim(buf);
        } while (buf.empty());
	if (false)
		std::cerr << "nextrec: line \"" << (std::string)buf << "\"" << std::endl;
        std::istringstream iss(buf);
        if (nrfields == -1) {
                iss >> m_rectype;
                reclength_t::const_iterator reclen(m_reclength.find(m_rectype));
                nrfields = 1;
                if (reclen != m_reclength.end())
                        nrfields = reclen->second;
        }
        while (nrfields > 1 && iss.good()) {
                Glib::ustring s;
                iss >> s;
		removectrlchars(s);
                m_fld.push_back(s);
                --nrfields;
        }
        if (!iss.good())
                return true;
        {
                std::string s1;
                std::getline(iss, s1);
                Glib::ustring s(s1);
                trim(s);
		removectrlchars(s);
                m_fld.push_back(s);
        }
        return true;
}

std::string XPlaneParser::get_line(void) const
{
	std::ostringstream oss;
	oss << m_rectype;
	for (fld_t::const_iterator fi(m_fld.begin()), fe(m_fld.end()); fi != fe; ++fi)
		oss << ' ' << *fi;
	return oss.str();
}

double XPlaneParser::get_double( unsigned int x ) const
{
        const char *f = get_string(x).c_str();
        char *cp;
        double r = strtod(f, &cp);
        if (*cp)
                throw std::runtime_error("XPlaneParser::get_double: invalid field \"" + get_string(x) + "\"");
        return r;
}

unsigned int XPlaneParser::get_uint( unsigned int x ) const
{
        const char *f = get_string(x).c_str();
        char *cp;
        unsigned long r = strtoul(f, &cp, 10);
        if (*cp)
                throw std::runtime_error("XPlaneParser::get_uint: invalid field \"" + get_string(x) + "\"");
        return r;
}

int XPlaneParser::get_int( unsigned int x ) const
{
        const char *f = get_string(x).c_str();
        char *cp;
        long r = strtol(f, &cp, 10);
        if (*cp)
                throw std::runtime_error("XPlaneParser::get_int: invalid field \"" + get_string(x) + "\"");
        return r;
}

unsigned int XPlaneParser::get_frequency( unsigned int x ) const
{
        return get_uint(x) * 10000;
}

Point XPlaneParser::get_point( unsigned int lonidx, unsigned int latidx ) const
{
        Point pt;
        pt.set_lon_deg(get_double(lonidx));
        pt.set_lat_deg(get_double(latidx));
        return pt;
}

void XPlaneParser::parse_file(const Glib::ustring & fn)
{
        close();
        open(fn);
        if (rectype() == 600) {
                parse_waypoint();
                close();
                return;
        }
        if (rectype() == 640) {
                parse_airway();
                close();
                return;
        }
        bool ok = nextrec();
        while (ok) {
                switch (rectype()) {
                        case 1:
                        case 16:
                        case 17:
                                ok = parse_airport();
                                break;

                        case 2:
                        case 3:
                        case 4:
                        case 5:
                        case 6:
                        case 7:
                        case 8:
                        case 9:
                        case 12:
                        case 13:
                                ok = parse_navaid();
                                break;

                        default:
                                std::cerr << "Unknown record " << rectype() << std::endl;
                                ok = nextrec();
                                break;
                }
        }
        close();
}

void XPlaneParser::split_beziernodes(AirportsDb::Airport::Polyline& pl)
{
        for (unsigned int i = 0; i < pl.size(); ++i) {
                AirportsDb::Airport::PolylineNode& pn0(pl[i]);
                unsigned int j = i;
                for (;;) {
                        AirportsDb::Airport::PolylineNode& pnj(pl[j]);
                        if (pnj.is_closepath() || pnj.is_endpath())
                                break;
                        if (j + 1 >= pl.size())
                                break;
                        AirportsDb::Airport::PolylineNode& pnj1(pl[j+1]);
                        if (pnj1.get_point() != pn0.get_point())
                                break;
                        if (i == j && pnj1.is_bezier())
                                break;
                        ++j;
                }
                if (j == i)
                        continue;
                AirportsDb::Airport::PolylineNode& pn1(pl[j]);
                pn0.set_next_controlpoint(pn1.get_next_controlpoint());
                pn0.set_flags((pn0.get_flags() & ~(AirportsDb::Airport::PolylineNode::flag_bezier_next |
                                                   AirportsDb::Airport::PolylineNode::flag_endpath |
                                                   AirportsDb::Airport::PolylineNode::flag_closepath)) |
                              (pn1.get_flags() & (AirportsDb::Airport::PolylineNode::flag_bezier_next |
                                                  AirportsDb::Airport::PolylineNode::flag_endpath |
                                                  AirportsDb::Airport::PolylineNode::flag_closepath)));
                for (; j > i; --j)
                        pl.erase(pl.begin() + j);
        }
}

void XPlaneParser::save_airport(AirportsDb::Airport & e)
{
        if (e.get_nrrwy() > 0) {
                const AirportsDb::Airport::Runway& rwy(e.get_rwy(0));
                e.set_coord(rwy.get_le_coord().halfway(rwy.get_he_coord()));
        } else if (e.get_nrhelipads() > 0) {
                const AirportsDb::Airport::Helipad& hp(e.get_helipad(0));
                e.set_coord(hp.get_coord());
        }
        for (unsigned int i = 0; i < e.get_nrlinefeatures(); ++i) {
                AirportsDb::Airport::Polyline& pl(e.get_linefeature(i));
                split_beziernodes(pl);
        }
        m_airportsdb.save(e);
}

bool XPlaneParser::parse_airport(void)
{
        if (!m_airportsdbopen) {
                m_airportsdb.open(m_outputdir);
                m_airportsdb.sync_off();
                if (m_purgedb)
                        m_airportsdb.purgedb();
                m_airportsdbopen = true;
        }
        AirportsDb::element_t e;
        if (rectype() != 1 && rectype() != 16 && rectype() != 17) // Airport or Seaport or Heliport
                return nextrec();
        if (size() < 5) {
                std::cerr << "invalid airport record" << std::endl;
                return nextrec();
        }
        {
                AirportsDb::elementvector_t ev(m_airportsdb.find_by_sourceid(get_string(3) + "@XPlane"));
                if (ev.size() == 1) {
                        e = ev.front();
                        if (e.get_modtime() >= get_filetime()) {
				std::cerr << "airport " << get_string(3) << " (" << e.get_sourceid()
					  << ") has modification date after file date, skipping" << std::endl;
				return nextrec();
			}
                }
        }
        if (!e.is_valid()) {
                AirportsDb::elementvector_t ev(m_airportsdb.find_by_icao(get_string(3)));
                if (ev.size() == 1) {
                        e = ev.front();
                        if (e.get_modtime() >= get_filetime()) {
				std::cerr << "airport " << get_string(3) << " (" << e.get_sourceid()
					  << ") has modification date after file date, skipping" << std::endl;
                                return nextrec();
			}
                }
        }
        if (!e.is_valid()) {
                e.set_label_placement(AirportsDb::Airport::label_e);
                e.set_typecode('A');
                e.set_sourceid(get_string(3) + "@XPlane");
        }
        e.set_elevation(get_int(0));
        if (e.get_icao().empty())
                e.set_icao(get_string(3));
        if (e.get_name().empty())
                e.set_name(get_string(4));
        e.set_modtime(get_filetime());
        std::cerr << "Airport: " << e.get_icao() /* << ' ' << e.get_name() */ << std::endl;
        bool graphicsdeleted(false);
        for (;;) {
                if (!nextrec()) {
			if (false)
				std::cerr << "Airport: no new record: done" << std::endl;
                        save_airport(e);
                        return false;
                }
		if (false)
			std::cerr << "Airport: next record " << rectype() << std::endl;
                switch (rectype()) {
		default:
			if (false)
				std::cerr << "Airport: unknown record type " << rectype() << ": done" << std::endl;
 			save_airport(e);
			return true;

		case 100: // Runway
		{
			if (size() < 25) {
				std::cerr << "invalid airport runway record" << std::endl;
				continue;
			}
			AirportsDb::Airport::Runway *rwy(0);
			for (unsigned int i = 0; i < e.get_nrrwy(); ++i) {
				AirportsDb::Airport::Runway& rwy1(e.get_rwy(i));
				if (rwy1.get_ident_he() == get_string(7) || rwy1.get_ident_le() == get_string(16)) {
					rwy = &rwy1;
					break;
				}
				if (rwy1.get_ident_le() == get_string(7) || rwy1.get_ident_he() == get_string(16)) {
					rwy1.swap_he_le();
					rwy = &rwy1;
					break;
				}
			}
			if (!rwy) {
				e.add_rwy(AirportsDb::Airport::Runway("", "", 0, 0, "", e.get_coord(), e.get_coord(), 0, 0, 0, 0, e.get_elevation(), 0, 0, 0, 0, e.get_elevation(), 0));
				rwy = &e.get_rwy(e.get_nrrwy()-1);
			}
			rwy->set_width(Point::round<unsigned int,double>(get_double(0) * Point::m_to_ft));
			rwy->set_surface(convert_surface_code(get_uint(1)));
			rwy->set_he_light(0, get_uint(4) ? AirportsDb::Airport::Runway::light_cl : AirportsDb::Airport::Runway::light_off);
			rwy->set_he_light(1, get_uint(5) ? AirportsDb::Airport::Runway::light_mirl : AirportsDb::Airport::Runway::light_off);
			rwy->set_le_light(0, rwy->get_he_light(0));
			rwy->set_le_light(1, rwy->get_he_light(1));
			rwy->set_ident_he(get_string(7));
			{
				Point pt;
				pt.set_lat_deg(get_double(8));
				pt.set_lon_deg(get_double(9));
				rwy->set_he_coord(pt);
			}
			rwy->set_he_disp(Point::round<unsigned int,double>(get_double(10) * Point::m_to_ft));
			switch (get_uint(13)) {
			default:
			case 0: // No approach lights.
				rwy->set_he_light(2, AirportsDb::Airport::Runway::light_off);
				break;

			case 1: // ALSF-I (high intensity Approach Light System with sequenced Flashing lights)
				rwy->set_he_light(2, AirportsDb::Airport::Runway::light_alsf1);
				break;

			case 2: // ALSF-II (high intensity Approach Light System with sequenced Flashing lights and red side bar lights (barettes) the last 1000’, that align with touch down zone lighting.
				rwy->set_he_light(2, AirportsDb::Airport::Runway::light_alsf2);
				break;

			case 3: // Calvert  (British) (High intensity)
				rwy->set_he_light(2, AirportsDb::Airport::Runway::light_calvert);
				break;

			case 4: // Calvert ILS Cat II and Cat II) (British) (High intensity with red side bar lights (barettes) the last 1000’ - barettes align with touch down zone lighting)
				rwy->set_he_light(2, AirportsDb::Airport::Runway::light_calvert);
				break;

			case 5: // SSALR (high intensity, Simplified Short Approach Light System with Runway Alignment Indicator Lights [RAIL])
				rwy->set_he_light(2, AirportsDb::Airport::Runway::light_ssalr);
				break;

			case 6: // SSALF (high intensity, Simplified Short Approach Light System with sequenced flashing lights)
				rwy->set_he_light(2, AirportsDb::Airport::Runway::light_mals_malsf_ssals_ssalsf);
				break;

			case 7: // SALS (high intensity, Short Approach Light System)
				rwy->set_he_light(2, AirportsDb::Airport::Runway::light_sals_salsf);
				break;

			case 8: // MALSR (Medium-intensity Approach Light System with Runway Alignment Indicator Lights [RAIL])
				rwy->set_he_light(2, AirportsDb::Airport::Runway::light_malsr);
				break;

			case 9: // MALSF (Medium-intensity Approach Light System with sequenced flashing lights)
				rwy->set_he_light(2, AirportsDb::Airport::Runway::light_mals_malsf_ssals_ssalsf);
				break;

			case 10: // MALS (Medium-intensity Approach Light System)
				rwy->set_he_light(2, AirportsDb::Airport::Runway::light_mals_malsf_ssals_ssalsf);
				break;

			case 11: // ODALS (Omni-directional approach light system) (flashing lights, not strobes, not sequenced)
				rwy->set_he_light(2, AirportsDb::Airport::Runway::light_odals);
				break;

			case 12: // RAIL (Runway Alignment Indicator Lights - sequenced strobes and green threshold lights, with no other approach lights). (changed from a code value of 13 on 27-Nov-2006)
				rwy->set_he_light(2, AirportsDb::Airport::Runway::light_rail);
				break;
			}
			rwy->set_he_light(3, get_uint(14) ? AirportsDb::Airport::Runway::light_tdzl : AirportsDb::Airport::Runway::light_off);
			switch (get_uint(15)) {
			default:
			case 0: // No REIL.
				rwy->set_he_light(4, AirportsDb::Airport::Runway::light_off);
				break;

			case 1: // Has omni-directional REIL
			case 2: // Has unidirectional REIL
				rwy->set_he_light(4, AirportsDb::Airport::Runway::light_reil);
				break;
			}
			rwy->set_ident_le(get_string(16));
			{
				Point pt;
				pt.set_lat_deg(get_double(17));
				pt.set_lon_deg(get_double(18));
				rwy->set_le_coord(pt);
			}
			rwy->set_le_disp(Point::round<unsigned int,double>(get_double(19) * Point::m_to_ft));
			switch (get_uint(22)) {
			default:
			case 0: // No approach lights.
				rwy->set_le_light(2, AirportsDb::Airport::Runway::light_off);
				break;

			case 1: // ALSF-I (high intensity Approach Light System with sequenced Flashing lights)
				rwy->set_le_light(2, AirportsDb::Airport::Runway::light_alsf1);
				break;

			case 2: // ALSF-II (high intensity Approach Light System with sequenced Flashing lights and red side bar lights (barettes) the last 1000’, that align with touch down zone lighting.
				rwy->set_le_light(2, AirportsDb::Airport::Runway::light_alsf2);
				break;

			case 3: // Calvert  (British) (High intensity)
				rwy->set_le_light(2, AirportsDb::Airport::Runway::light_calvert);
				break;

			case 4: // Calvert ILS Cat II and Cat II) (British) (High intensity with red side bar lights (barettes) the last 1000’ - barettes align with touch down zone lighting)
				rwy->set_le_light(2, AirportsDb::Airport::Runway::light_calvert);
				break;

			case 5: // SSALR (high intensity, Simplified Short Approach Light System with Runway Alignment Indicator Lights [RAIL])
				rwy->set_le_light(2, AirportsDb::Airport::Runway::light_ssalr);
				break;

			case 6: // SSALF (high intensity, Simplified Short Approach Light System with sequenced flashing lights)
				rwy->set_le_light(2, AirportsDb::Airport::Runway::light_mals_malsf_ssals_ssalsf);
				break;

			case 7: // SALS (high intensity, Short Approach Light System)
				rwy->set_le_light(2, AirportsDb::Airport::Runway::light_sals_salsf);
				break;

			case 8: // MALSR (Medium-intensity Approach Light System with Runway Alignment Indicator Lights [RAIL])
				rwy->set_le_light(2, AirportsDb::Airport::Runway::light_malsr);
				break;

			case 9: // MALSF (Medium-intensity Approach Light System with sequenced flashing lights)
				rwy->set_le_light(2, AirportsDb::Airport::Runway::light_mals_malsf_ssals_ssalsf);
				break;

			case 10: // MALS (Medium-intensity Approach Light System)
				rwy->set_le_light(2, AirportsDb::Airport::Runway::light_mals_malsf_ssals_ssalsf);
				break;

			case 11: // ODALS (Omni-directional approach light system) (flashing lights, not strobes, not sequenced)
				rwy->set_le_light(2, AirportsDb::Airport::Runway::light_odals);
				break;

			case 12: // RAIL (Runway Alignment Indicator Lights - sequenced strobes and green threshold lights, with no other approach lights). (changed from a code value of 13 on 27-Nov-2006)
				rwy->set_le_light(2, AirportsDb::Airport::Runway::light_rail);
				break;
			}
			rwy->set_le_light(3, get_uint(23) ? AirportsDb::Airport::Runway::light_tdzl : AirportsDb::Airport::Runway::light_off);
			switch (get_uint(24)) {
			default:
			case 0: // No REIL.
				rwy->set_le_light(4, AirportsDb::Airport::Runway::light_off);
				break;

			case 1: // Has omni-directional REIL
			case 2: // Has unidirectional REIL
				rwy->set_le_light(4, AirportsDb::Airport::Runway::light_reil);
				break;
			}
			int rwylength = Point::round<int,double>(rwy->get_he_coord().spheric_distance_km_dbl(rwy->get_le_coord()) * (1000.0 * Point::m_to_ft));
			if (rwylength) {
				rwy->set_length(rwylength);
			} else {
				std::cerr << "Warning: Airport: " << e.get_icao() << ' ' << e.get_name()
					  << " Runway length zero: " << rwy->get_ident_le() << '/' << rwy->get_ident_he()
					  << ' ' << rwy->get_le_coord() << '/' << rwy->get_he_coord() << " dist "
					  << rwy->get_he_coord().spheric_distance_km(rwy->get_le_coord()) << "km" << std::endl;
			}
			rwy->set_he_hdg(Point::round<int,double>(rwy->get_he_coord().spheric_true_course(rwy->get_le_coord()) * FPlanLeg::from_deg));
			rwy->set_le_hdg(rwy->get_he_hdg() + 0x8000);
			rwy->set_he_lda(rwy->get_length() - rwy->get_he_disp());
			rwy->set_he_tda(rwy->get_length() - rwy->get_le_disp());
			rwy->set_le_lda(rwy->get_length() - rwy->get_le_disp());
			rwy->set_le_tda(rwy->get_length() - rwy->get_he_disp());
			rwy->set_flags(AirportsDb::Airport::Runway::flag_le_usable | AirportsDb::Airport::Runway::flag_he_usable);
			rwy->normalize_he_le();
			break;
		}

		case 101: // Water Runway
		{
			if (size() < 8) {
				std::cerr << "invalid airport water runway record" << std::endl;
				continue;
			}
			AirportsDb::Airport::Runway *rwy(0);
			for (unsigned int i = 0; i < e.get_nrrwy(); ++i) {
				AirportsDb::Airport::Runway& rwy1(e.get_rwy(i));
				if (rwy1.get_ident_he() == get_string(2) || rwy1.get_ident_le() == get_string(5)) {
					rwy = &rwy1;
					break;
				}
				if (rwy1.get_ident_le() == get_string(2) || rwy1.get_ident_he() == get_string(5)) {
					rwy1.swap_he_le();
					rwy = &rwy1;
					break;
				}
			}
			if (!rwy) {
				e.add_rwy(AirportsDb::Airport::Runway("", "", 0, 0, "", e.get_coord(), e.get_coord(), 0, 0, 0, 0, e.get_elevation(), 0, 0, 0, 0, e.get_elevation(), 0));
				rwy = &e.get_rwy(e.get_nrrwy()-1);
			}
			rwy->set_width(Point::round<unsigned int,double>(get_double(0) * Point::m_to_ft));
			rwy->set_surface("WTR");
			rwy->set_ident_he(get_string(2));
			{
				Point pt;
				pt.set_lat_deg(get_double(3));
				pt.set_lon_deg(get_double(4));
				rwy->set_he_coord(pt);
			}
			rwy->set_he_disp(0);
			rwy->set_ident_le(get_string(5));
			{
				Point pt;
				pt.set_lat_deg(get_double(6));
				pt.set_lon_deg(get_double(7));
				rwy->set_le_coord(pt);
			}
			rwy->set_le_disp(0);
			int rwylength = Point::round<int,double>(rwy->get_he_coord().spheric_distance_km_dbl(rwy->get_le_coord()) * (1000.0 * Point::m_to_ft));
			if (rwylength) {
				rwy->set_length(rwylength);
			} else {
				std::cerr << "Warning: Airport: " << e.get_icao() << ' ' << e.get_name()
					  << " Runway length zero: " << rwy->get_ident_le() << '/' << rwy->get_ident_he()
					  << ' ' << rwy->get_le_coord() << '/' << rwy->get_he_coord() << " dist "
					  << rwy->get_he_coord().spheric_distance_km(rwy->get_le_coord()) << "km" << std::endl;
			}
			rwy->set_he_hdg(Point::round<int,double>(rwy->get_he_coord().spheric_true_course(rwy->get_le_coord()) * FPlanLeg::from_deg));
			rwy->set_le_hdg(rwy->get_he_hdg() + 0x8000);
			rwy->set_he_lda(rwy->get_length() - rwy->get_he_disp());
			rwy->set_he_tda(rwy->get_length() - rwy->get_le_disp());
			rwy->set_le_lda(rwy->get_length() - rwy->get_le_disp());
			rwy->set_le_tda(rwy->get_length() - rwy->get_he_disp());
			rwy->set_flags(AirportsDb::Airport::Runway::flag_le_usable | AirportsDb::Airport::Runway::flag_he_usable);
			rwy->normalize_he_le();
			break;
		}

		case 102: // Helipad
		{
			if (size() < 10) {
				std::cerr << "invalid airport helipad record" << std::endl;
				continue;
			}
			AirportsDb::Airport::Helipad *hp(0);
			for (unsigned int i = 0; i < e.get_nrhelipads(); ++i) {
				AirportsDb::Airport::Helipad& hp1(e.get_helipad(i));
				if (hp1.get_ident() == get_string(0)) {
					hp = &hp1;
					break;
				}
			}
			if (!hp) {
				e.add_helipad(AirportsDb::Airport::Helipad("", 0, 0, "", e.get_coord(), 0, e.get_elevation(), 0));
				hp = &e.get_helipad(e.get_nrhelipads()-1);
			}
			hp->set_ident(get_string(0));
			{
				Point pt;
				pt.set_lat_deg(get_double(1));
				pt.set_lon_deg(get_double(2));
				hp->set_coord(pt);
			}
			hp->set_hdg(Point::round<unsigned int,double>(get_double(3) * FPlanLeg::from_deg));
			hp->set_length(Point::round<unsigned int,double>(get_double(4) * Point::m_to_ft));
			hp->set_width(Point::round<unsigned int,double>(get_double(5) * Point::m_to_ft));
			hp->set_surface(convert_surface_code(get_uint(6)));
			hp->set_flags(AirportsDb::Airport::Helipad::flag_usable);
			break;
		}

		case 110: // Pavement
		{
			if (size() < 3) {
				std::cerr << "invalid airport pavement record" << std::endl;
				continue;
			}
			if (!graphicsdeleted) {
				graphicsdeleted = true;
				while (e.get_nrlinefeatures())
					e.erase_linefeature(0);
				while (e.get_nrpointfeatures())
					e.erase_pointfeature(0);
			}
			AirportsDb::Airport::Polyline pl;
			pl.set_surface(convert_surface_code(get_uint(0)));
			if (size() >= 4)
				pl.set_name(get_string(3));
			else
				pl.set_name("");
			pl.set_flags(AirportsDb::Airport::Polyline::flag_area);
			e.add_linefeature(pl);
			break;
		}

		case 120: // Linear Feature
		{
			if (size() < 1) {
				std::cerr << "invalid airport linear feature record" << std::endl;
				continue;
			}
			if (!graphicsdeleted) {
				graphicsdeleted = true;
				while (e.get_nrlinefeatures())
					e.erase_linefeature(0);
				while (e.get_nrpointfeatures())
					e.erase_pointfeature(0);
			}
			AirportsDb::Airport::Polyline pl;
			pl.set_name(get_string(0));
			e.add_linefeature(pl);
			break;
		}

		case 130: // Airport Boundary
		{
			if (size() < 1) {
				std::cerr << "invalid airport airport boundary record" << std::endl;
				continue;
			}
			if (!graphicsdeleted) {
				graphicsdeleted = true;
				while (e.get_nrlinefeatures())
					e.erase_linefeature(0);
				while (e.get_nrpointfeatures())
					e.erase_pointfeature(0);
			}
			AirportsDb::Airport::Polyline pl;
			pl.set_name(get_string(0));
			pl.set_flags(AirportsDb::Airport::Polyline::flag_airportboundary);
			e.add_linefeature(pl);
			break;
		}

		case 111: // Node (simple point).
		case 112: // Node with Bezier control point.
		case 113: // Node (close loop) point (to close a pavement boundary).
		case 114: // Node (close loop) point with Bezier control point (to close a pavement boundary).
		case 115: // Node (end) point to terminate a linear feature (so has no descriptive codes).
		case 116: // Node (end) point with Bezier control point, to terminate a linear feature (so has no descriptive codes).
		{
			uint16_t flags(0);
			unsigned int sz(2);
			if (!(rectype() & 1)) {
				flags |= AirportsDb::Airport::PolylineNode::flag_bezier_prev | AirportsDb::Airport::PolylineNode::flag_bezier_next;
				sz += 2;
			}
			if (rectype() >= 113 && rectype() <= 114)
				flags |= AirportsDb::Airport::PolylineNode::flag_closepath;
			if (rectype() >= 115 && rectype() <= 116)
				flags |= AirportsDb::Airport::PolylineNode::flag_endpath;
			if (size() < sz) {
				std::cerr << "invalid airport linear feature node record" << std::endl;
				continue;
			}
			if (!e.get_nrlinefeatures()) {
				std::cerr << "airport linear feature node record without linear feature header" << std::endl;
				continue;
			}
			if (!graphicsdeleted) {
				graphicsdeleted = true;
				while (e.get_nrlinefeatures())
					e.erase_linefeature(0);
				while (e.get_nrpointfeatures())
					e.erase_pointfeature(0);
			}
			AirportsDb::Airport::Polyline& pl(e.get_linefeature(e.get_nrlinefeatures()-1));
			AirportsDb::Airport::PolylineNode pn;
			pn.set_flags(flags);
			{
				Point pt;
				pt.set_lat_deg(get_double(0));
				pt.set_lon_deg(get_double(1));
				pn.set_point(pt);
			}
			if (flags & (AirportsDb::Airport::PolylineNode::flag_bezier_prev | AirportsDb::Airport::PolylineNode::flag_bezier_next)) {
				Point pt;
				pt.set_lat_deg(get_double(2));
				pt.set_lon_deg(get_double(3));
				pn.set_next_controlpoint(pt);
				Point pt2(pn.get_point());
				pt2 += pt2 - pt;
				pn.set_prev_controlpoint(pt2);
			}
			for (unsigned int idx = 0; sz < size() && idx < AirportsDb::Airport::PolylineNode::nr_features; ++idx, ++sz) {
				pn.set_feature(idx, (AirportsDb::Airport::PolylineNode::feature_t)get_uint(sz));
			}
			pl.push_back(pn);
			break;
		}

		case 21: // VASI / PAPI / Wig-Wag
		{
			if (size() < 6) {
				std::cerr << "invalid airport VASI / PAPI / Wig-Wag record" << std::endl;
				continue;
			}
			if (!graphicsdeleted) {
				graphicsdeleted = true;
				while (e.get_nrlinefeatures())
					e.erase_linefeature(0);
				while (e.get_nrpointfeatures())
					e.erase_pointfeature(0);
			}
			Point pt;
			pt.set_lat_deg(get_double(0));
			pt.set_lon_deg(get_double(1));
			Glib::ustring name(get_string(5)), rwyident;
			if (size() >= 7) {
				name = get_string(6);
				rwyident = get_string(5);
			}
			AirportsDb::Airport::PointFeature pf(AirportsDb::Airport::PointFeature::feature_vasi, pt,
							     Point::round<int,double>(get_double(3) * FPlanLeg::from_deg),
							     e.get_elevation(), get_uint(2),
							     Point::round<int,double>(get_double(4) * FPlanLeg::from_deg), 0,
							     name, rwyident);
			e.add_pointfeature(pf);
			break;
		}

		case 20: // Taxiway signs
		{
			if (size() < 6) {
				std::cerr << "invalid airport taxiway sign record" << std::endl;
				continue;
			}
			if (!graphicsdeleted) {
				graphicsdeleted = true;
				while (e.get_nrlinefeatures())
					e.erase_linefeature(0);
				while (e.get_nrpointfeatures())
					e.erase_pointfeature(0);
			}
			Point pt;
			pt.set_lat_deg(get_double(0));
			pt.set_lon_deg(get_double(1));
			AirportsDb::Airport::PointFeature pf(AirportsDb::Airport::PointFeature::feature_taxiwaysign, pt,
							     Point::round<int,double>(get_double(2) * FPlanLeg::from_deg),
							     e.get_elevation(), 0,
							     get_uint(3), get_uint(4),
							     get_string(5), "");
			e.add_pointfeature(pf);
			break;
		}

		case 15: // startup location
		{
			if (size() < 4) {
				std::cerr << "invalid airport startup location record" << std::endl;
				continue;
			}
			if (!graphicsdeleted) {
				graphicsdeleted = true;
				while (e.get_nrlinefeatures())
					e.erase_linefeature(0);
				while (e.get_nrpointfeatures())
					e.erase_pointfeature(0);
			}
			Point pt;
			pt.set_lat_deg(get_double(0));
			pt.set_lon_deg(get_double(1));
			AirportsDb::Airport::PointFeature pf(AirportsDb::Airport::PointFeature::feature_startuploc, pt,
							     Point::round<int,double>(get_double(2) * FPlanLeg::from_deg),
							     e.get_elevation(), 0,
							     0, 0,
							     get_string(3), "");
			e.add_pointfeature(pf);
			break;
		}

		case 14: // Tower viewpoints
		{
			if (size() < 5) {
				std::cerr << "invalid airport tower viewpoint record" << std::endl;
				continue;
			}
			if (!graphicsdeleted) {
				graphicsdeleted = true;
				while (e.get_nrlinefeatures())
					e.erase_linefeature(0);
				while (e.get_nrpointfeatures())
					e.erase_pointfeature(0);
			}
			Point pt;
			pt.set_lat_deg(get_double(0));
			pt.set_lon_deg(get_double(1));
			AirportsDb::Airport::PointFeature pf(AirportsDb::Airport::PointFeature::feature_tower, pt,
							     0,
							     Point::round<int,double>(get_double(2)), 0,
							     get_uint(3), 0,
							     get_string(4), "");
			e.add_pointfeature(pf);
			break;
		}

		case 18: // Light Beacon
		{
			if (size() < 4) {
				std::cerr << "invalid airport light beacon record" << std::endl;
				continue;
			}
			if (!get_uint(2))
				continue;
			if (!graphicsdeleted) {
				graphicsdeleted = true;
				while (e.get_nrlinefeatures())
					e.erase_linefeature(0);
				while (e.get_nrpointfeatures())
					e.erase_pointfeature(0);
			}
			Point pt;
			pt.set_lat_deg(get_double(0));
			pt.set_lon_deg(get_double(1));
			AirportsDb::Airport::PointFeature pf(AirportsDb::Airport::PointFeature::feature_lightbeacon, pt,
							     0,
							     e.get_elevation(), 0,
							     get_uint(2), 0,
							     get_string(3), "");
			e.add_pointfeature(pf);
			break;
		}

		case 19: // Airport windsocks
		{
			if (size() < 4) {
				std::cerr << "invalid airport windsock record" << std::endl;
				continue;
			}
			if (!graphicsdeleted) {
				graphicsdeleted = true;
				while (e.get_nrlinefeatures())
					e.erase_linefeature(0);
				while (e.get_nrpointfeatures())
					e.erase_pointfeature(0);
			}
			Point pt;
			pt.set_lat_deg(get_double(0));
			pt.set_lon_deg(get_double(1));
			AirportsDb::Airport::PointFeature pf(AirportsDb::Airport::PointFeature::feature_windsock, pt,
							     0,
							     e.get_elevation(), 0,
							     get_uint(2), 0,
							     get_string(3), "");
			e.add_pointfeature(pf);
			break;
		}

		case 10: // Old Taxiway / Runway segment
		{
			if (size() < 14) {
				std::cerr << "invalid airport old taxiway / runway record" << std::endl;
				continue;
			}
			if (get_string(2) != "xxx")
				continue;
			if (!graphicsdeleted) {
				graphicsdeleted = true;
				while (e.get_nrlinefeatures())
					e.erase_linefeature(0);
				while (e.get_nrpointfeatures())
					e.erase_pointfeature(0);
			}
			Point pt;
			pt.set_lat_deg(get_double(0));
			pt.set_lon_deg(get_double(1));
			double hdg(get_double(3));
			double hlen(get_double(4) * (Point::ft_to_m / 2000.0 * Point::km_to_nmi));
			double hwidth(get_double(7) * (Point::ft_to_m / 2000.0 * Point::km_to_nmi));
			Point pt01 = pt.spheric_course_distance_nmi(hdg + 180.0, hlen);
			Point pt23 = pt.spheric_course_distance_nmi(hdg, hlen);
			Point pt0 = pt01.spheric_course_distance_nmi(hdg - 90.0, hwidth);
			Point pt1 = pt01.spheric_course_distance_nmi(hdg + 90.0, hwidth);
			Point pt2 = pt23.spheric_course_distance_nmi(hdg + 90.0, hwidth);
			Point pt3 = pt23.spheric_course_distance_nmi(hdg - 90.0, hwidth);
			AirportsDb::Airport::Polyline pl;
			pl.set_surface(convert_surface_code(get_uint(9)));
			pl.set_flags(AirportsDb::Airport::Polyline::flag_area);
			pl.push_back(AirportsDb::Airport::PolylineNode(pt0, Point::invalid, 0));
			pl.push_back(AirportsDb::Airport::PolylineNode(pt1, Point::invalid, 0));
			pl.push_back(AirportsDb::Airport::PolylineNode(pt2, Point::invalid, 0));
			pl.push_back(AirportsDb::Airport::PolylineNode(pt3, Point::invalid, AirportsDb::Airport::PolylineNode::flag_closepath));
			e.add_linefeature(pl);
			break;
		}

		case 0:
		case 50:
		case 51:
		case 52:
		case 53:
		case 54:
		case 55:
		case 56:
		case 57:
		case 58:
		case 59:
		{
			if (size() < 2) {
				std::cerr << "invalid airport comm record" << std::endl;
				continue;
			}
			int idxn(-1), idxf(-1);
			uint32_t freq(get_uint(0) * 10000);
			freq = ((freq + 12500) / 25000) * 25000;
			Glib::ustring name(get_string(1));
			for (unsigned int i = 0; i < e.get_nrcomm(); ++i) {
				const AirportsDb::Airport::Comm& comm(e.get_comm(i));
				for (unsigned int j = 0; j < 5; ++j)
					if (comm[j] == freq)
						idxf = i;
				if (comm.get_name().casefold() == name.casefold())
					idxn = i;
			}
			if (idxn == -1)
				idxn = idxf;
			if (idxn == -1) {
				e.add_comm(AirportsDb::Airport::Comm(name, "", "", freq));
				break;
			}
			AirportsDb::Airport::Comm& comm(e.get_comm(idxn));
			{
				unsigned int i;
				for (i = 0; i < 5; ++i)
					if (comm[i] == freq)
						break;
				if (i == 5)
					comm.set_freq(0, freq);
			}
			if (comm.get_name().empty())
				comm.set_name(name);
			break;
		}

		case 1000: // Airport traffic flow
		case 1001: // Traffic flow wind rule
		case 1002: // Traffic flow minimum ceiling rule
		case 1003: // Traffic flow minimum visibility rule
		case 1004: // Traffic flow time rule
		case 1100: // Runway-in-use arrival/departure constraints
		case 1101: // VFR traffic pattern
		case 1200: // Header indicating that taxi route network data follows
		case 1201: // Taxi route network node Must connect two nodes
		case 1202: // Taxi route network edge Can refer to up to 4 runway ends
		case 1204: // Taxi route edge active zone
		case 1300: // Airport location Not explicitly connected to taxi route network
			break;
                }
        }
}

bool XPlaneParser::parse_navaid(void)
{
        if (!m_navaidsdbopen) {
                m_navaidsdb.open(m_outputdir);
                m_navaidsdb.sync_off();
                if (m_purgedb)
                        m_navaidsdb.purgedb();
                m_navaidsdbopen = true;
        }
        if (size() < 8) {
                std::cerr << "invalid navaid record" << std::endl;
                return nextrec();
        }
        if (m_navaidlastname == get_string(7)) {
                ++m_navaidlastcount;
        } else {
                m_navaidlastname = get_string(7);
                m_navaidlastcount = 0;
        }
        std::cerr << "navaid: " << get_string(7) << std::endl;
	Point coord(get_point(1, 0));
        NavaidsDb::element_t e;
        {
                NavaidsDb::elementvector_t ev(m_navaidsdb.find_by_icao(get_string(6)));
                if (ev.size() >= 1) {
                        Point ptrev(coord.get_lat(), coord.get_lon());
                        uint64_t d(std::numeric_limits<uint64_t>::max());
                        for (NavaidsDb::elementvector_t::const_iterator ei(ev.begin()), ee(ev.end()); ei != ee; ++ei) {
                                // FIXME: due to import bug, also accept reversed lat/lon
                                if (ei->get_coord().simple_distance_nmi(coord) >= 0.5 &&
                                    ei->get_coord().simple_distance_nmi(ptrev) >= 0.5)
                                        continue;
                                switch (rectype()) {
                                        case 2: // NDB
                                                if (ei->get_navaid_type() != NavaidsDb::element_t::navaid_ndb ||
						    ei->get_navaid_type() != NavaidsDb::element_t::navaid_ndbdme ||
						    ei->get_navaid_type() != NavaidsDb::element_t::navaid_dme)
                                                        continue;
                                                break;

                                        case 3: // VOR, VORTAC or VOR-DME
                                        case 12: // DME (suppressed frequency)
                                        case 13: // DME (displayed frequency)
                                                if (ei->get_navaid_type() != NavaidsDb::element_t::navaid_vor &&
                                                    ei->get_navaid_type() != NavaidsDb::element_t::navaid_dme &&
                                                    ei->get_navaid_type() != NavaidsDb::element_t::navaid_vordme &&
						    ei->get_navaid_type() != NavaidsDb::element_t::navaid_tacan &&
                                                    ei->get_navaid_type() != NavaidsDb::element_t::navaid_vortac)
                                                        continue;
                                                break;

                                        default:
                                                break;
                                }
                                uint64_t d1(ei->get_coord().simple_distance_rel(coord));
                                if (d < d1)
                                        continue;
                                d = d1;
                                e = *ei;
                        }
                        if (e.get_modtime() >= get_filetime())
                                return nextrec();
                }
        }
        if (!e.is_valid()) {
                std::ostringstream oss;
                if (!get_string(6).empty())
                        oss << get_string(6) << '_';
                oss << get_string(7);
                if (m_navaidlastcount)
                        oss << "_" << m_navaidlastcount;
                oss << "@XPlane";
                NavaidsDb::elementvector_t ev(m_navaidsdb.find_by_sourceid(oss.str()));
                if (ev.size() == 1)
                        e = ev.front();
                else
                        e.set_sourceid(oss.str());
                e.set_label_placement(NavaidsDb::element_t::label_e);
        }
        switch (rectype()) {
        case 2: // NDB
                e.set_frequency(get_uint(3) * 1000);
                e.set_coord(coord);
                e.set_navaid_type(NavaidsDb::element_t::navaid_ndb);
                break;

        case 3: // VOR, VORTAC or VOR-DME
                e.set_frequency(get_frequency(3));
		e.set_coord(coord);
		if (e.get_navaid_type() == NavaidsDb::element_t::navaid_dme)
                        e.set_navaid_type(NavaidsDb::element_t::navaid_vordme);
                else if (e.get_navaid_type() == NavaidsDb::element_t::navaid_tacan)
                        e.set_navaid_type(NavaidsDb::element_t::navaid_vortac);
		else if (e.get_navaid_type() == NavaidsDb::element_t::navaid_dme)
                        e.set_navaid_type(NavaidsDb::element_t::navaid_vordme);
                else if (e.get_navaid_type() != NavaidsDb::element_t::navaid_vordme &&
			 e.get_navaid_type() != NavaidsDb::element_t::navaid_vortac)
                        e.set_navaid_type(NavaidsDb::element_t::navaid_vor);
                break;

        case 4: // Localiser that is part of a full ILS
        case 5: // Stand-alone localiser (LOC), also including a LDA (Landing Directional Aid) or SDF (Simplified Directional Facility)
        case 6: // Glideslope (GS)
        case 7: // Outer marker (OM)
        case 8: // Middle marker (MM)
        case 9: // Inner marker (IM)
        default:
                return nextrec();

        case 12: // DME (suppressed frequency)
        case 13: // DME (displayed frequency)
                e.set_dmecoord(coord);
                if (e.get_navaid_type() == NavaidsDb::element_t::navaid_vor) {
                        e.set_navaid_type(NavaidsDb::element_t::navaid_vordme);
                } else if (e.get_navaid_type() == NavaidsDb::element_t::navaid_ndb) {
                        e.set_navaid_type(NavaidsDb::element_t::navaid_ndbdme);
                } else if (e.get_navaid_type() != NavaidsDb::element_t::navaid_vordme &&
			   e.get_navaid_type() != NavaidsDb::element_t::navaid_vortac &&
			   e.get_navaid_type() != NavaidsDb::element_t::navaid_ndbdme) {
			e.set_coord(coord);
                        e.set_navaid_type(NavaidsDb::element_t::navaid_dme);
		}
                e.set_frequency(get_frequency(3));
                break;
        }
        if (e.is_valid()) {
                e.set_elev(get_int(2));
                e.set_range(get_int(4));
                e.set_icao(get_string(6));
                if (e.get_name().empty())
                        e.set_name(get_string(7));
                e.set_inservice(true);
                e.set_modtime(get_filetime());
                m_navaidsdb.save(e);
        }
        return nextrec();
}

void XPlaneParser::parse_waypoint(void)
{
        if (!m_waypointsdbopen) {
                m_waypointsdb.open(m_outputdir);
                m_waypointsdb.sync_off();
                if (m_purgedb)
                        m_waypointsdb.purgedb();
                m_waypointsdbopen = true;
        }
        while (nextrec(3)) {
                if (size() < 3) {
                        std::cerr << "invalid waypoint record" << std::endl;
                        continue;
                }
                if (get_string(2) == m_waypointlastname) {
                        ++m_waypointlastcount;
                } else {
                        m_waypointlastname = get_string(2);
                        m_waypointlastcount = 0;
                }
                std::cerr << "waypoint: " << get_string(2) << std::endl;
                WaypointsDb::elementvector_t ev(m_waypointsdb.find_by_name(get_string(2)));
                WaypointsDb::element_t e;
                if (ev.size() >= 1) {
                        Point pt(get_point(1, 0));
                        Point ptrev(pt.get_lat(), pt.get_lon());
                        uint64_t d(std::numeric_limits<uint64_t>::max());
                        for (WaypointsDb::elementvector_t::const_iterator ei(ev.begin()), ee(ev.end()); ei != ee; ++ei) {
                                // FIXME: due to import bug, also accept reversed lat/lon
                                if (ei->get_coord().simple_distance_nmi(pt) >= 1.0 &&
                                    ei->get_coord().simple_distance_nmi(ptrev) >= 1.0)
                                        continue;
                                uint64_t d1(ei->get_coord().simple_distance_rel(pt));
                                if (d < d1)
                                        continue;
                                d = d1;
                                e = *ei;
                        }
                        if (!e.is_valid()) {
                                std::ostringstream oss;
                                oss << get_string(2);
                                if (m_waypointlastcount)
                                        oss << "_" << m_waypointlastcount;
                                oss << "@XPlane";
                                for (WaypointsDb::elementvector_t::const_iterator ei(ev.begin()), ee(ev.end()); ei != ee; ++ei) {
                                        if (ei->get_sourceid() != oss.str())
                                                continue;
                                        e = *ei;
                                        std::cerr << "XPlane waypoint " << get_string(2) << " / " << oss.str() << " has changed position" << std::endl;
                                        break;
                                }
                        }
                        if (e.is_valid() && e.get_modtime() >= get_filetime())
                                continue;
                }
                if (!e.is_valid()) {
                        std::ostringstream oss;
                        oss << get_string(2);
                        if (m_waypointlastcount)
                                oss << "_" << m_waypointlastcount;
                        oss << "@XPlane";
                        e.set_sourceid(oss.str());
                        e.set_usage(WaypointsDb::element_t::usage_bothlevel);
                        e.set_label_placement(WaypointsDb::element_t::label_e);
                }
                e.set_coord(get_point(1, 0));
                e.set_name(get_string(2));
                e.set_modtime(get_filetime());
                m_waypointsdb.save(e);
        }
}

void XPlaneParser::parse_airway(void)
{
        if (!m_airwaysdbopen) {
                m_airwaysdb.open(m_outputdir);
                m_airwaysdb.sync_off();
                if (m_purgedb)
                        m_airwaysdb.purgedb();
                m_airwaysdbopen = true;
        }
        while (nextrec(10)) {
                if (size() < 10) {
                        std::cerr << "invalid airway record" << std::endl;
                        continue;
                }
                AirwaysDb::element_t e;
                Point ptb(get_point(2, 1)), pte(get_point(5, 4));
                Glib::ustring name(get_string(9)), nameb(get_string(0)), namee(get_string(3));
                std::cerr << "airway: " << name << ": " << nameb << "->" << namee << std::endl;
                {
                        AirwaysDb::elementvector_t ev(m_airwaysdb.find_by_name(name));
                        uint64_t d(std::numeric_limits<uint64_t>::max());
                        for (AirwaysDb::elementvector_t::const_iterator ei(ev.begin()), ee(ev.end()); ei != ee; ++ei) {
                                const AirwaysDb::element_t& e1(*ei);
                                if (e1.get_begin_name() != nameb || e1.get_end_name() != namee || !e1.is_valid())
                                        continue;
                                uint64_t d1((e1.get_begin_coord().simple_distance_rel(ptb) >> 1) + (e1.get_end_coord().simple_distance_rel(pte) >> 1));
                                if (d < d1)
                                        continue;
                                d = d1;
                                e = e1;
                        }
                }
                if (!e.is_valid()) {
                        std::ostringstream oss;
                        oss << name << '_' << nameb << '_' << namee << "@XPlane";
                        e.set_sourceid(oss.str());
                        e.set_type(AirwaysDb::element_t::airway_user);
                }
                e.set_begin_coord(ptb);
                e.set_end_coord(pte);
                e.set_begin_name(nameb);
                e.set_end_name(namee);
                e.set_name(name);
                e.set_label_placement(AirwaysDb::element_t::label_center);
                switch (get_int(6)) {
                        case 1:
                                e.set_type((e.get_type() == AirwaysDb::element_t::airway_high) ? AirwaysDb::element_t::airway_both : AirwaysDb::element_t::airway_low);
                                break;

                        case 2:
                                e.set_type((e.get_type() == AirwaysDb::element_t::airway_low) ? AirwaysDb::element_t::airway_both : AirwaysDb::element_t::airway_high);
                                break;

                        default:
                                break;
                }
                e.set_base_level(get_int(7));
                e.set_top_level(get_int(8));
                e.set_modtime(get_filetime());
                try {
                        m_airwaysdb.save(e);
                } catch (std::exception& ex) {
                        std::cerr << "error saving airway: " << e.get_name() << ": " << e.get_begin_name() << "->" << e.get_end_name()
                                  << " (" << e.get_sourceid() << "): " << ex.what() << " possibly duplicate airway?" << std::endl;
                }
        }
}

/* ---------------------------------------------------------------------- */


/* ---------------------------------------------------------------------- */

int main(int argc, char *argv[])
{
        static struct option long_options[] = {
                { "dir", required_argument, 0, 'd' },
                { "purgedb", no_argument, 0, 'p' },
                {0, 0, 0, 0}
        };
	Glib::init();
	Gio::init();
        Glib::ustring db_dir(".");
        bool purgedb(false);
        int c, err(0);

        while ((c = getopt_long(argc, argv, "d:p", long_options, 0)) != EOF) {
                switch (c) {
                        case 'd':
                                if (optarg)
                                        db_dir = optarg;
                                break;

                        case 'p':
                                purgedb = true;
                                break;

                        default:
                                err++;
                                break;
                }
        }
        if (err) {
                std::cerr << "usage: vfrdbxplane [-d <dir>]" << std::endl;
                return EX_USAGE;
        }
        try {
                XPlaneParser parser(db_dir, purgedb);
                for (; optind < argc; optind++) {
                        std::cerr << "Parsing file " << argv[optind] << std::endl;
                        parser.parse_file(argv[optind]);
                }
        } catch (const Glib::Exception& ex) {
                std::cerr << "Glib exception: " << ex.what() << std::endl;
                return EX_DATAERR;
        } catch (const std::exception& ex) {
                std::cerr << "exception: " << ex.what() << std::endl;
                return EX_DATAERR;
        }
        return EX_OK;
}
