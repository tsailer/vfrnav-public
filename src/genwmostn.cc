/*****************************************************************************/

/*
 *      genwmostn.cc  --  Generate WMO Station List.
 *
 *      Copyright (C) 2014  Thomas Sailer (t.sailer@alumni.ethz.ch)
 *
 *      This program is free software; you can redistribute it and/or modify
 *      it under the terms of the GNU General Public License as published by
 *      the Free Software Foundation; either version 2 of the License, or
 *      (at your option) any later version.
 *
 *      This program is distributed in the hope that it will be useful,
 *      but WITHOUT ANY WARRANTY; without even the implied warranty of
 *      MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *      GNU General Public License for more details.
 *
 *      You should have received a copy of the GNU General Public License
 *      along with this program; if not, write to the Free Software
 *      Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 *
 */

/*****************************************************************************/

#include "sysdeps.h"
#include "metgraph.h"
#include <iostream>
#include <sstream>
#include <fstream>
#include <getopt.h>

#ifdef HAVE_CURL
#include <curl/curl.h>

class GetCURL {
public:
	GetCURL(const std::string& url);
	~GetCURL();
	operator bool(void) const { return m_res == CURLE_OK; }
	const std::string& get_string(void) const { return m_data; }

protected:
	CURL *m_curl;
	std::string m_data;
	CURLcode m_res;

	static size_t my_write_func_1(const void *ptr, size_t size, size_t nmemb, GetCURL *curl);
	size_t my_write_func(const void *ptr, size_t size, size_t nmemb);
};

GetCURL::GetCURL(const std::string& url)
{
	CURL *curl(curl_easy_init());
	if (!curl)
		return;
	curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
	curl_easy_setopt(curl, CURLOPT_WRITEDATA, this);
	curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, my_write_func_1);
        curl_easy_setopt(curl, CURLOPT_NOPROGRESS, 1L);
	m_res = curl_easy_perform(curl);
	curl_easy_cleanup(curl);
}

GetCURL::~GetCURL()
{
}

size_t GetCURL::my_write_func_1(const void *ptr, size_t size, size_t nmemb, GetCURL *curl)
{
	return curl->my_write_func(ptr, size, nmemb);
}

size_t GetCURL::my_write_func(const void *ptr, size_t size, size_t nmemb)
{
	unsigned int sz(size * nmemb);
	m_data.insert(m_data.end(), (char *)ptr, ((char *)ptr) + sz);
	return sz;
}

#else

class GetCURL {
public:
	GetCURL(const std::string& url) {}
	~GetCURL() {}
	operator bool(void) const { return false; }
	const std::string& get_string(void) const { return empty; }

protected:
	static const std::string empty;
};

const std::string GetCURL::empty;

#endif

static std::string crtolf(const std::string& s)
{
	std::string r(s);
	for (std::string::iterator i(r.begin()), e(r.end()); i != e; ++i)
		if (*i == '\r')
			*i = '\n';
	return r;
}

static std::vector<std::string> tokenize(const std::string& s)
{
	std::vector<std::string> r;
	for (std::string::size_type n(0); n < s.size(); ) {
		std::string::size_type n1(s.find(';', n));
		if (n1 == std::string::npos) {
			r.push_back(s.substr(n));
			break;
		}
		r.push_back(s.substr(n, n1 - n));
		n = n1 + 1;
	}
	return r;
}

static void trimspace(std::string& s)
{
	{
		std::string::size_type n(0), l(s.size());
		for (; n != l && std::isspace(s[n]); ++n);
		if (n)
			s.erase(0, n);
	}
	{
		std::string::size_type n(s.size());
		for (; n && std::isspace(s[n-1]); --n);
		s.erase(n);
	}
}

static unsigned int getuint(const std::string& s)
{
	const char *cp(s.c_str());
	char *cp1(0);
	unsigned int x(strtoul(cp, &cp1, 10));
	if (cp == cp1 || *cp1)
		return ~0U;
	return x;
}

static double getonecoord(const std::string& s, const std::string& sgn)
{
	double val(0);
	const char *cp(s.c_str());
	char *cp1(0);
	unsigned int x(strtoul(cp, &cp1, 10));
	if (cp == cp1)
		return std::numeric_limits<double>::quiet_NaN();
	cp = cp1;
	val = x;
	if (*cp == '-') {
		++cp;
		x = strtoul(cp, &cp1, 10);
		if (cp == cp1)
			return std::numeric_limits<double>::quiet_NaN();
		cp = cp1;
		val += x * (1.0 / 60);
		if (*cp == '-') {
			++cp;
			x = strtoul(cp, &cp1, 10);
			if (cp == cp1)
				return std::numeric_limits<double>::quiet_NaN();
			cp = cp1;
			val += x * (1.0 / 60 / 60);
		}
	}
	std::string::size_type n(sgn.find(*cp));
	switch (n) {
	case 0:
		return val;

	case 1:
		return -val;

	default:
		return std::numeric_limits<double>::quiet_NaN();
	}
}

static Point getcoord(const std::string& slat, const std::string& slon)
{
	double lat(getonecoord(slat, "NS"));
	double lon(getonecoord(slon, "EW"));
	if (std::isnan(lat) || std::isnan(lon))
		return Point::invalid;
	Point pt;
	pt.set_lat_deg_dbl(lat);
	pt.set_lon_deg_dbl(lon);
	return pt;
}

static std::string getstr(std::map<std::string, unsigned int>& str, const char *s)
{
	if (!s)
		return "0";
	std::map<std::string, unsigned int>::const_iterator si(str.find(s));
	if (si == str.end())
		return "0";
	std::ostringstream oss;
	oss << "str" << si->second;
	return oss.str();
}

static std::string cquote(const std::string& s)
{
	std::string r;
	for (std::string::const_iterator i(s.begin()), e(s.end()); i != e; ++i) {
		if (*i < 32 || *i >= 127) {
			r.push_back('\\');
			r.push_back('0' + ((*i >> 6) & 3));
			r.push_back('0' + ((*i >> 3) & 7));
			r.push_back('0' + ((*i >> 0) & 7));
			continue;
		}
		switch (*i) {
		case '"':
		case '\\':
			r.push_back('\\');
			// fall back

		default:
			r.push_back(*i);
			break;
		}
	}
	return r;
}

int main(int argc, char *argv[])
{
	static struct option long_options[] = {
		{ "help",               no_argument,       NULL, 'h' },
		{ "stations",           required_argument, NULL, 's' },
		{ "csv",                required_argument, NULL, 'c' },
		{ NULL,                 0,                 NULL, 0 }
	};
	int c, err(0);
	// http://weather.noaa.gov/tg/site.shtml
	std::string stnlist("http://weather.noaa.gov/data/nsd_bbsss.txt");
	std::ofstream csvout;

	while ((c = getopt_long(argc, argv, "hi:c:", long_options, NULL)) != -1) {
		switch (c) {
		case 's':
			stnlist = optarg;
			break;

		case 'c':
			csvout.open(optarg);
			break;

		case 'h':
		default:
			++err;
			break;
		}
	}
	if (err) {
		std::cerr << "usage: dafif [-q]" << std::endl
			  << "     -h, --help        Display this information" << std::endl
			  << "     -s, --stations    US Stations file" << std::endl
			  << "     -c, --csv         Dump as CSV" << std::endl << std::endl;
                return EX_USAGE;
        }
	std::istringstream iss;
	{
		GetCURL curl(stnlist);
		if (!curl) {
			std::cerr << "cannot get station list from \"" << stnlist << "\"" << std::endl;
			return EX_DATAERR;
		}
		iss.str(crtolf(curl.get_string()));
	}
	typedef std::map<std::string, unsigned int> strings_t;
	strings_t str;
	typedef std::vector<WMOStation> stn_t;
	stn_t stn;
	while (iss.good()) {
		std::string line;
		getline(iss, line);
		for (;;) {
			std::string::size_type n(line.find('\r'));
			if (n == std::string::npos)
				break;
			line.erase(n, 1);
		}
		std::vector<std::string> tok(tokenize(line));
		if (tok.empty())
			continue;
		if (csvout.is_open()) {
			for (std::vector<std::string>::const_iterator ti(tok.begin()), te(tok.end()); ti != te;) {
				if (ti->find(',') == std::string::npos) {
					csvout << *ti;
				} else {
					csvout << "\"" << *ti << "\"";
				}
				++ti;
				if (ti == te)
					break;
				csvout << ',';
			}
			csvout << std::endl;
		}
		if (tok.size() < 12)
			continue;
		const char *icao(0);
		const char *name(0);
		const char *state(0);
		const char *country(0);
		unsigned int wmonr(~0U);
		unsigned int wmoregion(~0U);
		Point coord(Point::invalid);
		TopoDb30::elev_t elev(TopoDb30::nodata);
		bool basic(false);
		{
			unsigned int blknr(getuint(tok[0])), stnnr(getuint(tok[1]));
			if (blknr != ~0U && stnnr != ~0U)
				wmonr = blknr * 1000 + stnnr;
		}
		for (unsigned int i(2); i < 6; ++i)
			trimspace(tok[i]);
		if (!tok[2].empty() && tok[2] != "----")
			icao = str.insert(strings_t::value_type(tok[2], 0)).first->first.c_str();
		if (!tok[3].empty())
			name = str.insert(strings_t::value_type(tok[3], 0)).first->first.c_str();
		if (!tok[4].empty())
			state = str.insert(strings_t::value_type(tok[4], 0)).first->first.c_str();
		if (!tok[5].empty())
			country = str.insert(strings_t::value_type(tok[5], 0)).first->first.c_str();
		wmoregion = getuint(tok[6]);
		coord = getcoord(tok[7], tok[8]);
		{
			unsigned int e(getuint(tok[11]));
			if (e != ~0U)
				elev = e;
		}
		basic = (tok.size() >= 14 && !tok[13].empty() && tok[13][0] == 'P');
		stn.push_back(WMOStation(icao, name, state, country, wmonr, wmoregion, coord, elev, basic));
	}
	// sort stations according to WMO number
	std::sort(stn.begin(), stn.end(), WMOStation::WMONrSort());
	// number strings
	{
		unsigned int nr(0);
		for (strings_t::iterator si(str.begin()), se(str.end()); si != se; ++si)
			si->second = nr++;
	}
	// dump header
	std::cout << "// do not edit, automatically generated by genwmostn.cc" << std::endl << std::endl
		  << "#include \"metgraph.h\"" << std::endl << std::endl;
	// dump string constants
	for (strings_t::const_iterator si(str.begin()), se(str.end()); si != se; ++si)
		std::cout << "static const char str" << si->second << "[] = \"" << cquote(si->first) << "\";" << std::endl;
	// dump array of WMO stations
	std::cout << std::endl
		  << "const unsigned int WMOStation::nrstations = " << stn.size() << ';' << std::endl
		  << std::endl
		  << "const WMOStation WMOStation::stations[" << (stn.size() + 1) << "] = {" << std::endl;
	for (stn_t::const_iterator si(stn.begin()), se(stn.end()); si != se; ++si) {
		std::cout << "\tWMOStation(" << getstr(str, si->get_icao()) << ", " << getstr(str, si->get_name()) << ", "
			  << getstr(str, si->get_state()) << ", " << getstr(str, si->get_country()) << ", "
			  << si->get_wmonr() << ", " << si->get_wmoregion()
			  << ", Point(" << si->get_coord().get_lon() << ", " << si->get_coord().get_lat() << "), "
			  << si->get_elev() << ", " << (si->is_basic() ? "true" : "false") << ')';
		std::cout << ',' << std::endl;
	}
	std::cout << "\tWMOStation(0, 0, 0, 0, ~0U, 0, Point::invalid, TopoDb30::nodata, false)" << std::endl << "};" << std::endl;
	return 0;
}
