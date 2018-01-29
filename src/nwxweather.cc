/*****************************************************************************/

/*
 *      nwxweather.cc  --  nwx weather service.
 *
 *      Copyright (C) 2012, 2016  Thomas Sailer (t.sailer@alumni.ethz.ch)
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

#include "nwxweather.h"
#include "baro.h"

#include <cmath>
#include <stdexcept>
#include <algorithm>
#include <fstream>
#include <iostream>
#include <iomanip>
#include <cstring>
#include <set>

#include <libxml/parser.h>
#include <curl/curl.h>

NWXWeather::WindsAloft::WindsAloft(const Point& coord, int16_t alt, uint16_t flags, time_t tm,
		       double winddir, double windspeed, double qff, double temp)
	: m_coord(coord), m_alt(alt), m_flags(flags), m_time(tm),
	  m_winddir(winddir), m_windspeed(windspeed), m_qff(qff), m_temp(temp)
{
}

NWXWeather::WindsAloft::WindsAloft(const FPlanWaypoint& wpt)
	: m_winddir(std::numeric_limits<double>::quiet_NaN()),
	  m_windspeed(std::numeric_limits<double>::quiet_NaN()),
	  m_qff(std::numeric_limits<double>::quiet_NaN()),
	  m_temp(std::numeric_limits<double>::quiet_NaN())
{
	m_coord = wpt.get_coord();
	m_alt = wpt.get_altitude();
	m_flags = wpt.get_flags();
	m_time = wpt.get_time_unix();
}

NWXWeather::EpochsLevels::EpochsLevels(void)
{
}

NWXWeather::EpochsLevels::epoch_iterator NWXWeather::EpochsLevels::epoch_find_nearest(time_t t)
{
	epoch_iterator i1(m_epochs.upper_bound(t));
	if (i1 == m_epochs.begin())
		return i1;
	epoch_iterator i0(i1);
	--i0;
	if (i1 == m_epochs.end() || t - *i0 <= *i1 - t)
		return i0;
	return i1;
}

NWXWeather::EpochsLevels::epoch_const_iterator NWXWeather::EpochsLevels::epoch_find_nearest(time_t t) const
{
	epoch_const_iterator i1(m_epochs.upper_bound(t));
	if (i1 == m_epochs.begin())
		return i1;
	epoch_const_iterator i0(i1);
	--i0;
	if (i1 == m_epochs.end() || t - *i0 <= *i1 - t)
		return i0;
	return i1;
}

NWXWeather::EpochsLevels::level_iterator NWXWeather::EpochsLevels::level_find_nearest(unsigned int l)
{
	level_iterator i1(m_levels.upper_bound(l));
	if (i1 == m_levels.begin())
		return i1;
	level_iterator i0(i1);
	--i0;
	if (i1 == m_levels.end() || l - *i0 <= *i1 - l)
		return i0;
	return i1;
}

NWXWeather::EpochsLevels::level_const_iterator NWXWeather::EpochsLevels::level_find_nearest(unsigned int l) const
{
	level_const_iterator i1(m_levels.upper_bound(l));
	if (i1 == m_levels.begin())
		return i1;
	level_const_iterator i0(i1);
	--i0;
	if (i1 == m_levels.end() || l - *i0 <= *i1 - l)
		return i0;
	return i1;
}

time_t NWXWeather::EpochsLevels::parse_epoch(xmlChar *txt)
{
	if (!txt)
		return 0;
	struct tm utm;
#ifdef __MINGW32__
	utm.tm_year = 1970;
	utm.tm_mon = 1;
	utm.tm_mday = 1;
	utm.tm_hour = utm.tm_min = utm.tm_sec = 0;
	sscanf(reinterpret_cast<const char *>(txt), "%u-%u-%u %u:%u:%u", &utm.tm_year, &utm.tm_mon, &utm.tm_mday, &utm.tm_hour, &utm.tm_min, &utm.tm_sec);
	--utm.tm_mon;
	utm.tm_year -= 1900;
#else
	strptime(reinterpret_cast<const char *>(txt), "%Y-%m-%d %H:%M:%S", &utm);
#endif
	return timegm(&utm);
}

void NWXWeather::EpochsLevels::add_epoch(time_t t)
{
	m_epochs.insert(t);
}

void NWXWeather::EpochsLevels::add_epoch(xmlChar *txt)
{
	if (!txt)
		return;
	add_epoch(parse_epoch(txt));
}

void NWXWeather::EpochsLevels::add_level(unsigned int l)
{
	m_levels.insert(l);
}

void NWXWeather::EpochsLevels::add_level(xmlChar *txt)
{
	if (!txt)
		return;
	const char *cp(reinterpret_cast<const char *>(txt));
	char *ep;
	unsigned int lvl(strtoul(cp, &ep, 10));
	if (cp == ep)
		return;
	add_level(lvl);
}

const char NWXWeather::nwxweather_url[] = "http://navlost.eu/nwxs/";

NWXWeather::NWXWeather(void)
	: m_width(0), m_height(0), m_profilegraph_epoch(0), m_thread(0), m_mode(mode_abort), m_profilegraph_svg(true)
{
	m_dispatch.connect(sigc::mem_fun(*this, &NWXWeather::complete));
}

NWXWeather::~NWXWeather(void)
{
	abort();
}

void NWXWeather::abort(void)
{
	if (!m_thread)
		return;
	m_mode = mode_abort;
	m_thread->join();
	m_thread = 0;
}

void NWXWeather::complete(void)
{
	if (m_thread)
		m_thread->join();
	m_thread = 0;
	m_txbuffer.clear();
	if (true)
		std::cerr << "NWXWeather::complete: " << std::string(m_rxbuffer.begin(), m_rxbuffer.end()) << std::endl;
	//xmlDocPtr doc(xmlParseMemory(const_cast<char *>(&m_rxbuffer[0]), m_rxbuffer.size()));
	xmlDocPtr doc(xmlReadMemory(const_cast<char *>(&m_rxbuffer[0]), m_rxbuffer.size(), nwxweather_url, 0, 0));
	xmlCleanupParser();
	m_rxbuffer.clear();
	xmlNodePtr reqnode(0);
	{
		xmlNodePtr node(xmlDocGetRootElement(doc));
		for (; node && !reqnode; node = node->next) {
			if (node->type != XML_ELEMENT_NODE || xmlStrcmp(node->name, BAD_CAST "nwx"))
				continue;
			for (xmlNodePtr node2(node->children); node2; node2 = node2->next) {
				if (node2->type != XML_ELEMENT_NODE || xmlStrcmp(node2->name, BAD_CAST "Response"))
					continue;
				reqnode = node2;
				break;
			}
		}
	}
	switch (m_mode) {
	default:
	case mode_abort:
		break;

	case mode_windsaloft:
		parse_windsaloft(doc, reqnode);
		break;

	case mode_epochslevels:
		parse_epochslevels(doc, reqnode);
		break;

	case mode_chart:
		parse_chart(doc, reqnode);
		break;

	case mode_metartaf:
		parse_metartaf(doc, reqnode);
		break;

	case mode_pgepochs:
		parse_profilegraph_epochs(doc, reqnode);
		break;
	}
	xmlFreeDoc(doc);
}

size_t NWXWeather::my_write_func_1(const void *ptr, size_t size, size_t nmemb, NWXWeather *wx)
{
	return wx->my_write_func(ptr, size, nmemb);
}

size_t NWXWeather::my_read_func_1(void *ptr, size_t size, size_t nmemb, NWXWeather *wx)
{
	return wx->my_read_func(ptr, size, nmemb);
}

int NWXWeather::my_progress_func_1(NWXWeather *wx, double t, double d, double ultotal, double ulnow)
{
	return wx->my_progress_func(t, d, ultotal, ulnow);
}

size_t NWXWeather::my_write_func(const void *ptr, size_t size, size_t nmemb)
{
	unsigned int sz(size * nmemb);
	unsigned int p(m_rxbuffer.size());
	m_rxbuffer.resize(p + sz);
	memcpy(&m_rxbuffer[p], ptr, sz);
	return sz;
}

size_t NWXWeather::my_read_func(void *ptr, size_t size, size_t nmemb)
{
	return 0;
}

int NWXWeather::my_progress_func(double t, double d, double ultotal, double ulnow)
{
	return m_mode == mode_abort;
}

xmlDocPtr NWXWeather::empty_request(xmlNodePtr& reqnode)
{
	xmlDocPtr doc(xmlNewDoc(BAD_CAST "1.0"));
	xmlNodePtr root_node(xmlNewNode(0, BAD_CAST "nwx"));
	xmlDocSetRootElement(doc, root_node);
	xmlNewProp(root_node, BAD_CAST "version", BAD_CAST "0.3.4");
	reqnode = xmlNewChild(root_node, 0, BAD_CAST "Request", 0);
	return doc;
}

void NWXWeather::start_request(xmlDocPtr doc, mode_t mode)
{
	if (!doc)
		return;
	std::vector<char> buf;
	{
		xmlChar *xmlbuff;
		int buffersize;
		xmlDocDumpFormatMemory(doc, &xmlbuff, &buffersize, 0);
		if (buffersize > 22)
			buf.insert(buf.end(), xmlbuff + 22, xmlbuff + buffersize);
		xmlFree(xmlbuff);
	}
	xmlFreeDoc(doc);
	if (mode == mode_abort)
		return;
	if (m_thread)
		throw std::runtime_error("NWXWeather: request ongoing");
	m_rxbuffer.clear();
	m_txbuffer.swap(buf);
	if (true)
		std::cerr << "NWXWeather::start_request: " << std::string(m_txbuffer.begin(), m_txbuffer.end()) << std::endl;
	m_mode = mode;
	m_thread = Glib::Threads::Thread::create(sigc::mem_fun(*this, &NWXWeather::curlio));
}

void NWXWeather::curlio(void)
{
	CURL *curl(curl_easy_init());
	if (!curl) {
		m_dispatch();
		return;
	}
	struct curl_slist *headers(0);
	headers = curl_slist_append(headers, "Content-Type: application/xml");
	curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);
	curl_easy_setopt(curl, CURLOPT_URL, nwxweather_url);
	curl_easy_setopt(curl, CURLOPT_WRITEDATA, this);
	curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, my_write_func_1);
	curl_easy_setopt(curl, CURLOPT_READDATA, this);
	curl_easy_setopt(curl, CURLOPT_READFUNCTION, my_read_func_1);
	curl_easy_setopt(curl, CURLOPT_NOPROGRESS, 0L);
	curl_easy_setopt(curl, CURLOPT_PROGRESSFUNCTION, my_progress_func_1);
	curl_easy_setopt(curl, CURLOPT_PROGRESSDATA, this);
	curl_easy_setopt(curl, CURLOPT_POSTFIELDS, &m_txbuffer[0]);
	curl_easy_setopt(curl, CURLOPT_POSTFIELDSIZE, m_txbuffer.size());
	CURLcode res = curl_easy_perform(curl);
	curl_easy_cleanup(curl);
	curl_slist_free_all(headers);
	if (res != CURLE_OK) {
		std::cerr << "NWXWeather::curlio: error " << res << std::endl;
		m_rxbuffer.clear();
	}
	m_dispatch();
}

void NWXWeather::winds_aloft(const FPlanRoute& route, const sigc::slot<void,const windvector_t&>& cb)
{
	if (route.get_nrwpt() <= 0) {
		cb(windvector_t());
		return;
	}
	xmlNodePtr reqnode;
	xmlDocPtr doc(empty_request(reqnode));
	for (unsigned int i = 0; i < route.get_nrwpt(); ++i) {
		const FPlanWaypoint& wpt(route[i]);
		xmlNodePtr node(xmlNewChild(reqnode, 0, BAD_CAST "Wind", 0));
		xmlNodePtr qnode(xmlNewChild(reqnode, 0, BAD_CAST "qff", 0));
		xmlNodePtr tnode(xmlNewChild(reqnode, 0, BAD_CAST "temp", 0));
		{
			std::ostringstream oss;
			oss << i;
			xmlNewProp(node, BAD_CAST "id", BAD_CAST oss.str().c_str());
			xmlNewProp(qnode, BAD_CAST "id", BAD_CAST oss.str().c_str());
			xmlNewProp(tnode, BAD_CAST "id", BAD_CAST oss.str().c_str());
		}
		{
			std::ostringstream oss;
			oss << std::fixed << std::setprecision(10) << wpt.get_coord().get_lat_deg_dbl()
			    << ',' << wpt.get_coord().get_lon_deg_dbl();
			xmlNewProp(node, BAD_CAST "p", BAD_CAST oss.str().c_str());
			xmlNewProp(qnode, BAD_CAST "p", BAD_CAST oss.str().c_str());
			xmlNewProp(tnode, BAD_CAST "p", BAD_CAST oss.str().c_str());
		}
		{
			std::ostringstream oss;
			if (wpt.get_flags() & FPlanWaypoint::altflag_standard) {
				xmlNewProp(node, BAD_CAST "u", BAD_CAST "F");
				xmlNewProp(tnode, BAD_CAST "u", BAD_CAST "F");
				oss << ((wpt.get_altitude() + 50) / 100);
			} else {
				xmlNewProp(node, BAD_CAST "u", BAD_CAST "A");
				xmlNewProp(tnode, BAD_CAST "u", BAD_CAST "A");
				oss << wpt.get_altitude();
			}
			xmlNewProp(node, BAD_CAST "z", BAD_CAST oss.str().c_str());
			xmlNewProp(tnode, BAD_CAST "z", BAD_CAST oss.str().c_str());
		}
		{
			char buf[64];
			struct tm utm;
			time_t t(wpt.get_time_unix());
			strftime(buf, sizeof(buf), "%Y-%m-%d %H:%M:%S", gmtime_r(&t, &utm));
			xmlNewProp(node, BAD_CAST "e", BAD_CAST buf);
			xmlNewProp(qnode, BAD_CAST "e", BAD_CAST buf);
			xmlNewProp(tnode, BAD_CAST "e", BAD_CAST buf);
		}
	}
	start_request(doc, mode_windsaloft);
	m_windsaloft_done = cb;
}

void NWXWeather::parse_windsaloft(xmlDocPtr doc, xmlNodePtr reqnode)
{
	if (!reqnode || !doc)
		return;
	windvector_t wind;
	for (xmlNodePtr node(reqnode->children); node; node = node->next) {
		if (node->type != XML_ELEMENT_NODE)
			continue;
		if (false)
			std::cerr << "parse_windsaloft: " << node->name << std::endl;
		bool isqff(!xmlStrcmp(node->name, BAD_CAST "qff"));
		bool istemp(!xmlStrcmp(node->name, BAD_CAST "temp"));
		bool iswind(!xmlStrcmp(node->name, BAD_CAST "Wind"));
		if (!isqff && !istemp && !iswind)
			continue;
		WindsAloft *wnd(0);
		{
			xmlChar *id = xmlGetProp(node, BAD_CAST "id");
			if (id) {
				unsigned int id1(strtoul(reinterpret_cast<const char *>(id), 0, 10));
				xmlFree(id);
				if (id1 < 1000) {
					if (wind.size() <= id1)
						wind.resize(id1 + 1, WindsAloft());
					wnd = &wind[id1];
				}
			}
		}
		if (!wnd)
			continue;
		{
			xmlChar *attr = xmlGetProp(node, BAD_CAST "p");
			if (attr) {
				xmlChar *plon(const_cast<xmlChar *>(xmlStrchr(attr, ',')));
				if (plon) {
					*plon++ = 0;
					double d(strtod(reinterpret_cast<const char *>(plon), 0));
					if (!std::isnan(d))
						wnd->set_lon_deg_dbl(d);
				}
				double d(strtod(reinterpret_cast<const char *>(attr), 0));
				if (!std::isnan(d))
					wnd->set_lat_deg_dbl(d);
				xmlFree(attr);
			}
		}
		{
			xmlChar *attr = xmlGetProp(node, BAD_CAST "u");
			if (attr) {
				if (!xmlStrcmp(attr, BAD_CAST "A"))
					wnd->set_flags(wnd->get_flags() & ~FPlanWaypoint::altflag_standard);
				else if (!xmlStrcmp(attr, BAD_CAST "F"))
					wnd->set_flags(wnd->get_flags() | FPlanWaypoint::altflag_standard);
				xmlFree(attr);
			}
		}
		{
			xmlChar *attr = xmlGetProp(node, BAD_CAST "z");
			if (attr) {
				double d(strtod(reinterpret_cast<const char *>(attr), 0));
				if (!std::isnan(d)) {
					if (wnd->get_flags() & FPlanWaypoint::altflag_standard)
						d *= 100;
					wnd->set_altitude(d);
				}
				xmlFree(attr);
			}
		}
		{
			xmlChar *attr = xmlGetProp(node, BAD_CAST "e");
			if (attr) {
				wnd->set_time(EpochsLevels::parse_epoch(attr));
				xmlFree(attr);
			}
		}
		if (isqff) {
			xmlChar *txt = xmlNodeListGetString(doc, node->xmlChildrenNode, 1);
			if (txt) {
				double d(strtod(reinterpret_cast<const char *>(txt), 0));
				if (!std::isnan(d))
					wnd->set_qff(d);
			}
			xmlFree(txt);
			continue;
		}
		if (istemp) {
			xmlChar *txt = xmlNodeListGetString(doc, node->xmlChildrenNode, 1);
			if (txt) {
				double d(strtod(reinterpret_cast<const char *>(txt), 0));
				if (!std::isnan(d))
					wnd->set_temp(d + IcaoAtmosphere<double>::degc_to_kelvin);
			}
			xmlFree(txt);
			continue;
		}
		if (iswind) {
			for (xmlNodePtr node2(node->children); node2; node2 = node2->next) {
				if (node2->type != XML_ELEMENT_NODE)
					continue;
				if (!xmlStrcmp(node2->name, BAD_CAST "dir")) {
					xmlChar *txt = xmlNodeListGetString(doc, node2->xmlChildrenNode, 1);
					if (txt) {
						double d(strtod(reinterpret_cast<const char *>(txt), 0));
						if (!std::isnan(d))
							wnd->set_winddir(d);
					}
					xmlFree(txt);
					continue;
				}
				if (!xmlStrcmp(node2->name, BAD_CAST "speed")) {
					xmlChar *txt = xmlNodeListGetString(doc, node2->xmlChildrenNode, 1);
					if (txt) {
						double d(strtod(reinterpret_cast<const char *>(txt), 0));
						if (!std::isnan(d))
							wnd->set_windspeed(d);
					}
					xmlFree(txt);
					continue;
				}
			}
		}
	}
	m_windsaloft_done(wind);
}

void NWXWeather::epochs_and_levels(const sigc::slot<void,const EpochsLevels&>& cb)
{
	xmlNodePtr reqnode;
	xmlDocPtr doc(empty_request(reqnode));
	xmlNewChild(reqnode, 0, BAD_CAST "AvailableEpochs", 0);
	xmlNewChild(reqnode, 0, BAD_CAST "AvailableLevels", 0);
	start_request(doc, mode_epochslevels);
	m_epochslevels_done = cb;
}

void NWXWeather::parse_epochslevels(xmlDocPtr doc, xmlNodePtr reqnode)
{
	EpochsLevels el;
	for (xmlNodePtr node(reqnode->children); node; node = node->next) {
		if (node->type != XML_ELEMENT_NODE)
			continue;
		if (!xmlStrcmp(node->name, BAD_CAST "AvailableEpochs")) {
			for (xmlNodePtr node2(node->children); node2; node2 = node2->next) {
				if (node2->type != XML_ELEMENT_NODE || xmlStrcmp(node2->name, BAD_CAST "epoch"))
					continue;
				xmlChar *txt = xmlNodeListGetString(doc, node2->xmlChildrenNode, 1);
				el.add_epoch(txt);
				xmlFree(txt);
			}
			continue;
		}
		if (!xmlStrcmp(node->name, BAD_CAST "AvailableLevels")) {
			for (xmlNodePtr node2(node->children); node2; node2 = node2->next) {
				if (node2->type != XML_ELEMENT_NODE || xmlStrcmp(node2->name, BAD_CAST "level"))
					continue;
				{
					xmlChar *attr = xmlGetProp(node2, BAD_CAST "u");
					if (!attr)
						continue;
					bool ok(!xmlStrcmp(attr, BAD_CAST "hPa"));
					xmlFree(attr);
					if (!ok)
						continue;
				}
				xmlChar *txt = xmlNodeListGetString(doc, node2->xmlChildrenNode, 1);
				el.add_level(txt);
				xmlFree(txt);
			}
			continue;
		}
	}
	m_epochslevels_done(el);
}

bool NWXWeather::add_route(xmlNodePtr node, const FPlanRoute& route)
{
	if (!node || route.get_nrwpt() < 1)
		return false;
	xmlNodePtr rnode(xmlNewChild(node, 0, BAD_CAST "route", 0));
	for (unsigned int i = 0; i < route.get_nrwpt(); ++i)
		add_waypoint(rnode, route[i]);
	return true;
}

bool NWXWeather::add_route(xmlNodePtr node, const std::vector<FPlanWaypoint>& route)
{
	if (!node || route.size() < 1)
		return false;
	xmlNodePtr rnode(xmlNewChild(node, 0, BAD_CAST "route", 0));
	for (unsigned int i = 0; i < route.size(); ++i)
		add_waypoint(rnode, route[i]);
	return true;
}

void NWXWeather::add_waypoint(xmlNodePtr node, const FPlanWaypoint& wpt)
{
	if (!node)
		return;
	xmlNodePtr wnode(xmlNewChild(node, 0, BAD_CAST "wpt", 0));
	{
		Glib::ustring icaoname(wpt.get_icao_name());
		if (!icaoname.empty())
			xmlNewProp(wnode, BAD_CAST "name", BAD_CAST icaoname.c_str());
	}
	{
		std::ostringstream oss;
		oss << std::fixed << std::setprecision(10) << wpt.get_coord().get_lat_deg_dbl()
		    << ',' << wpt.get_coord().get_lon_deg_dbl();
		xmlNewProp(wnode, BAD_CAST "p", BAD_CAST oss.str().c_str());
	}
	{
		std::ostringstream oss;
		if (wpt.get_flags() & FPlanWaypoint::altflag_standard) {
			xmlNewProp(wnode, BAD_CAST "u", BAD_CAST "F");
			oss << ((wpt.get_altitude() + 50) / 100);
		} else {
			xmlNewProp(wnode, BAD_CAST "u", BAD_CAST "A");
			oss << wpt.get_altitude();
		}
		xmlNewProp(wnode, BAD_CAST "z", BAD_CAST oss.str().c_str());
	}
}

void NWXWeather::metar_taf(const FPlanRoute& route, const sigc::slot<void,const metartaf_t&>& cb,
			   unsigned int metar_maxage, unsigned int metar_count, unsigned int taf_maxage, unsigned int taf_count)
{
	if (route.get_nrwpt() < 1 || (!metar_count && !taf_count)) {
		cb(metartaf_t());
		return;
	}
	xmlNodePtr reqnode;
	xmlDocPtr doc(empty_request(reqnode));
	for (unsigned int i = 0; i < route.get_nrwpt(); ++i) {
		const FPlanWaypoint& wpt(route[i]);
		if (metar_count) {
			xmlNodePtr node(xmlNewChild(reqnode, 0, BAD_CAST "Metar", 0));
			{
				std::ostringstream oss;
				oss << std::fixed << std::setprecision(10) << wpt.get_coord().get_lat_deg_dbl()
				    << ',' << wpt.get_coord().get_lon_deg_dbl();
				xmlNewProp(node, BAD_CAST "p", BAD_CAST oss.str().c_str());
			}
			if (metar_count > 1) {
				std::ostringstream oss;
				oss << metar_count;
				xmlNewProp(node, BAD_CAST "count", BAD_CAST oss.str().c_str());
			}
			if (metar_maxage) {
				std::ostringstream oss;
				oss << metar_maxage;
				xmlNewProp(node, BAD_CAST "maxage", BAD_CAST oss.str().c_str());
			}
		}
		if (taf_count) {
			xmlNodePtr node(xmlNewChild(reqnode, 0, BAD_CAST "Taf", 0));
			{
				std::ostringstream oss;
				oss << std::fixed << std::setprecision(10) << wpt.get_coord().get_lat_deg_dbl()
				    << ',' << wpt.get_coord().get_lon_deg_dbl();
				xmlNewProp(node, BAD_CAST "p", BAD_CAST oss.str().c_str());
			}
			if (taf_count > 1) {
				std::ostringstream oss;
				oss << taf_count;
				xmlNewProp(node, BAD_CAST "count", BAD_CAST oss.str().c_str());
			}
			if (taf_maxage) {
				std::ostringstream oss;
				oss << taf_maxage;
				xmlNewProp(node, BAD_CAST "maxage", BAD_CAST oss.str().c_str());
			}
		}

	}
	start_request(doc, mode_metartaf);
	m_metartaf_done = cb;
}

void NWXWeather::parse_metartaf(xmlDocPtr doc, xmlNodePtr reqnode)
{
	if (!reqnode || !doc)
		return;
	metartaf_t mt;
	for (xmlNodePtr node(reqnode->children); node; node = node->next) {
		if (node->type != XML_ELEMENT_NODE)
			continue;
		MetarTaf::type_t typ;
		if (!xmlStrcmp(node->name, BAD_CAST "Metar"))
			typ = MetarTaf::type_metar;
		else if (!xmlStrcmp(node->name, BAD_CAST "Taf"))
			typ = MetarTaf::type_taf;
		else
			continue;
		Point pt;
		pt.set_invalid();
		{
			xmlChar *attr = xmlGetProp(node, BAD_CAST "p");
			if (attr) {
				std::istringstream is(reinterpret_cast<const char *>(attr));
				xmlFree(attr);
				double lat, lon;
				is >> lat;
				if (!is.fail()) {
					char ch;
					is >> ch;
					if (!is.fail() && ch == ',') {
						is >> lon;
						if (!is.fail()) {
							pt.set_lat_deg_dbl(lat);
							pt.set_lon_deg_dbl(lon);
						}
					}
				}
			}
		}
		std::string icao;
		{
			xmlChar *attr = xmlGetProp(node, BAD_CAST "icao");
			if (attr) {
				icao = reinterpret_cast<const char *>(attr);
				xmlFree(attr);
			}
		}
		if (icao.empty())
			continue;
		for (xmlNodePtr node2(node->children); node2; node2 = node2->next) {
			if (node2->type != XML_ELEMENT_NODE || xmlStrcmp(node2->name, BAD_CAST "report"))
				continue;
			time_t epoch(0);
			{
				EpochsLevels el;
				xmlChar *attr = xmlGetProp(node2, BAD_CAST "epoch");
				el.add_epoch(attr);
				xmlFree(attr);
				if (el.epoch_begin() != el.epoch_end())
					epoch = *el.epoch_begin();
			}
			if (!epoch)
				continue;
			xmlChar *txt = xmlNodeListGetString(doc, node2->xmlChildrenNode, 1);
			if (!txt)
				continue;
			mt.insert(MetarTaf(icao, typ, epoch, reinterpret_cast<const char *>(txt), pt));
			xmlFree(txt);
		}
	}
	m_metartaf_done(mt);
}

void NWXWeather::chart(const FPlanRoute& route, const Rect& bbox, time_t epoch, unsigned int level, const themes_t& themes, bool svg,
		       const sigc::slot<void,const std::string&,const std::string&,time_t,unsigned int>& cb, unsigned int width, unsigned int height)
{
	m_charts_done = sigc::slot<void,const std::string&,const std::string&,time_t,unsigned int>();
	xmlNodePtr reqnode;
	xmlDocPtr doc(empty_request(reqnode));
	xmlNodePtr cnode(xmlNewChild(reqnode, 0, BAD_CAST "Chart", 0));
	if (false && svg)
		xmlNewProp(cnode, BAD_CAST "mimetype", BAD_CAST "image/svg+xml");
	{
		std::ostringstream oss;
		oss << width;
		xmlNewProp(cnode, BAD_CAST "x", BAD_CAST oss.str().c_str());
	}
	{
		std::ostringstream oss;
		oss << height;
		xmlNewProp(cnode, BAD_CAST "y", BAD_CAST oss.str().c_str());
	}
	{
		char buf[64];
		struct tm utm;
		strftime(buf, sizeof(buf), "%Y-%m-%d %H:%M:%S", gmtime_r(&epoch, &utm));
		xmlNewProp(cnode, BAD_CAST "e", BAD_CAST buf);
	}
	{
		std::ostringstream oss;
		oss << level;
		xmlNewProp(cnode, BAD_CAST "z", BAD_CAST oss.str().c_str());
	}
	{
		std::ostringstream oss;
		oss << std::fixed << std::setprecision(9) << bbox.get_southwest().get_lat_deg_dbl();
		xmlNewProp(cnode, BAD_CAST "lat0", BAD_CAST oss.str().c_str());
	}
	{
		std::ostringstream oss;
		oss << std::fixed << std::setprecision(9) << bbox.get_southwest().get_lon_deg_dbl();
		xmlNewProp(cnode, BAD_CAST "lon0", BAD_CAST oss.str().c_str());
	}
	{
		std::ostringstream oss;
		oss << std::fixed << std::setprecision(9) << bbox.get_northeast().get_lat_deg_dbl();
		xmlNewProp(cnode, BAD_CAST "lat1", BAD_CAST oss.str().c_str());
	}
	{
		std::ostringstream oss;
		oss << std::fixed << std::setprecision(9) << bbox.get_northeast().get_lon_deg_dbl();
		xmlNewProp(cnode, BAD_CAST "lon1", BAD_CAST oss.str().c_str());
	}
	xmlNewProp(cnode, BAD_CAST "layout", BAD_CAST "map");
	for (themes_t::const_iterator ti(themes.begin()), te(themes.end()); ti != te; ++ti) {
		if (ti->empty())
			continue;
		xmlAddChild(xmlNewChild(cnode, 0, BAD_CAST "theme", 0), xmlNewText(BAD_CAST ti->c_str()));
	}
	if (add_route(cnode, route))
		xmlAddChild(xmlNewChild(cnode, 0, BAD_CAST "theme", 0), xmlNewText(BAD_CAST "route"));
	start_request(doc, mode_chart);
        m_charts_done = cb;
}

void NWXWeather::profile_graph(const FPlanRoute& route, time_t epoch, const themes_t& themes, bool svg,
			       const sigc::slot<void,const std::string&,const std::string&,time_t>& cb, unsigned int width, unsigned int height)
{
	if (route.get_nrwpt() <= 0) {
		cb(std::string(), std::string(), (time_t)0);
		return;
	}
	m_charts_done = sigc::slot<void,const std::string&,const std::string&,time_t,unsigned int>();
	xmlNodePtr reqnode;
	xmlDocPtr doc(empty_request(reqnode));
	xmlNodePtr pgnode(xmlNewChild(reqnode, 0, BAD_CAST "ProfileGraph", 0));
	if (svg)
		xmlNewProp(pgnode, BAD_CAST "mimetype", BAD_CAST "image/svg+xml");
	{
		std::ostringstream oss;
		oss << m_width;
		xmlNewProp(pgnode, BAD_CAST "width", BAD_CAST oss.str().c_str());
	}
	{
		std::ostringstream oss;
		oss << m_height;
		xmlNewProp(pgnode, BAD_CAST "height", BAD_CAST oss.str().c_str());
	}
	{
		int32_t maxalt(route.max_altitude());
		maxalt = std::max(maxalt, (int32_t)0);
		maxalt += 5999;
		maxalt = (maxalt / 5000) * 5000;
		std::ostringstream oss;
		oss << maxalt;
		xmlNewProp(pgnode, BAD_CAST "hmin", BAD_CAST "0");
		xmlNewProp(pgnode, BAD_CAST "hmax", BAD_CAST oss.str().c_str());
	}
	xmlNewProp(pgnode, BAD_CAST "xmargin", BAD_CAST "5");
	xmlNewProp(pgnode, BAD_CAST "ymargin", BAD_CAST "5");
	xmlNewProp(pgnode, BAD_CAST "hdiv", BAD_CAST "3");
	{
		char buf[64];
		struct tm utm;
		strftime(buf, sizeof(buf), "%Y-%m-%d %H:%M:%S", gmtime_r(&epoch, &utm));
		xmlNewProp(reqnode, BAD_CAST "e", BAD_CAST buf);
	}
	add_route(pgnode, route);
	{
		xmlNodePtr wnode(0);
		for (themes_t::const_iterator ti(themes.begin()), te(themes.end()); ti != te; ++ti) {
			if (ti->empty())
				continue;
			if (*ti == "flightpath" || *ti == "terrain") {
				xmlNewChild(pgnode, 0, BAD_CAST ti->c_str(), 0);
				continue;
			}
			if (!wnode)
				wnode = xmlNewChild(pgnode, 0, BAD_CAST "Weather", 0);
			xmlNewTextChild(wnode, 0, BAD_CAST "theme", BAD_CAST ti->c_str());
		}
	}
	start_request(doc, mode_chart);
        m_charts_done = sigc::hide(cb);
}

void NWXWeather::parse_chart(xmlDocPtr doc, xmlNodePtr reqnode)
{
	if (!reqnode || !doc)
		return;
	for (xmlNodePtr node(reqnode->children); node; node = node->next) {
		if (node->type != XML_ELEMENT_NODE || (xmlStrcmp(node->name, BAD_CAST "Chart") && xmlStrcmp(node->name, BAD_CAST "ProfileGraph")))
			continue;
		EpochsLevels el;
		el.add_epoch((time_t)0);
		el.add_level((unsigned int)0U);
		{
			xmlChar *attr = xmlGetProp(node, BAD_CAST "e");
			el.add_epoch(attr);
			xmlFree(attr);
		}
		{
			xmlChar *attr = xmlGetProp(node, BAD_CAST "z");
			el.add_level(attr);
			xmlFree(attr);
		}
		std::string mimetype("image/png");
		{
			xmlChar *attr = xmlGetProp(node, BAD_CAST "mimetype");
			if (attr) {
				mimetype = reinterpret_cast<const char *>(attr);
				xmlFree(attr);
			}
		}
		for (xmlNodePtr node2(node->children); node2; node2 = node2->next) {
			if (node2->type != XML_ELEMENT_NODE || xmlStrcmp(node2->name, BAD_CAST "data"))
				continue;
			{
				xmlChar *attr = xmlGetProp(node2, BAD_CAST "mimetype");
				if (attr) {
					mimetype = reinterpret_cast<const char *>(attr);
					xmlFree(attr);
				}
			}
			for (xmlNodePtr node3(node2->children); node3; node3 = node3->next) {
				if (node3->type != XML_ELEMENT_NODE || xmlStrcmp(node3->name, BAD_CAST "svg"))
					continue;
#ifdef HAVE_OPEN_MEMSTREAM
				char *buf(0);
				size_t bufsz(0);
				FILE *f(open_memstream(&buf, &bufsz));
				if (!f)
					return;
#else
				char *tmpf(tempnam(0, "nwxch"));
				FILE *f(0);
				if (tmpf)
					f = fopen(tmpf, "w+");
				if (!f) {
					free(tmpf);
					return;
				}
#endif
				xmlElemDump(f, doc, node3);
#ifdef HAVE_OPEN_MEMSTREAM
				fclose(f);
#else
				fseek(f, 0L, SEEK_END);
				size_t bufsz(ftell(f));
				rewind(f);
				char *buf(0);
				if (bufsz > 0) {
					buf = (char *)malloc(bufsz);
					fread(buf, 1, bufsz, f);
				}
				fclose(f);
				unlink(tmpf);
				free(tmpf);
#endif
				if (buf && bufsz)
					m_charts_done(std::string(buf, bufsz), mimetype, *el.epoch_rbegin(), *el.level_rbegin());
				free(buf);
				return;
			}
			xmlChar *txt = xmlNodeListGetString(doc, node2->xmlChildrenNode, 1);
			if (!txt)
				continue;
			gsize len(0);
			guchar *pic(g_base64_decode(reinterpret_cast<const char *>(txt), &len));
			xmlFree(txt);
			m_charts_done(std::string(reinterpret_cast<const char *>(pic), len), mimetype, *el.epoch_rbegin(), *el.level_rbegin());
			g_free(pic);
			return;
		}
	}
	m_charts_done(std::string(), "", 0, 0);
}

void NWXWeather::profile_graph(const FPlanRoute& route, bool svg, const sigc::slot<void,const std::string&,const std::string&,time_t>& cb,
			       unsigned int width, unsigned int height)
{
	m_profilegraph_epoch = 0;
	if (route.get_nrwpt() <= 0) {
		cb(std::string(), std::string(), m_profilegraph_epoch);
		return;
	}
	m_charts_done = sigc::slot<void,const std::string&,const std::string&,time_t,unsigned int>();
	m_route.clear();
	for (unsigned int i = 0; i < route.get_nrwpt(); ++i)
		m_route.push_back(route[i]);
	m_width = width;
	m_height = height;
	m_profilegraph_svg = svg;
	xmlNodePtr reqnode;
	xmlDocPtr doc(empty_request(reqnode));
	xmlNewChild(reqnode, 0, BAD_CAST "AvailableEpochs", 0);
	start_request(doc, mode_pgepochs);
        m_charts_done = sigc::hide(cb);
}

void NWXWeather::parse_profilegraph_epochs(xmlDocPtr doc, xmlNodePtr reqnode)
{
	if (!reqnode || !doc)
		return;
	typedef std::set<time_t> epochs_t;
	epochs_t epochs;
	for (xmlNodePtr node(reqnode->children); node; node = node->next) {
		if (node->type != XML_ELEMENT_NODE || xmlStrcmp(node->name, BAD_CAST "AvailableEpochs"))
			continue;
		for (xmlNodePtr node2(node->children); node2; node2 = node2->next) {
			if (node2->type != XML_ELEMENT_NODE || xmlStrcmp(node2->name, BAD_CAST "epoch"))
				continue;
			xmlChar *txt = xmlNodeListGetString(doc, node2->xmlChildrenNode, 1);
			if (!txt)
				continue;
			epochs.insert(EpochsLevels::parse_epoch(txt));
			xmlFree(txt);
		}
	}
	if (true) {
		std::cerr << "Epochs available:";
		for (epochs_t::const_iterator ei(epochs.begin()), ee(epochs.end()); ei != ee; ++ei) {
			char buf[64];
			struct tm utm;
			time_t t(*ei);
			strftime(buf, sizeof(buf), "%Y-%m-%d %H:%M:%S", gmtime_r(&t, &utm));
			std::cerr << ' ' << buf;
		}
		std::cerr << std::endl;
	}
	if (m_route.empty())
		return;
	m_profilegraph_epoch = 0;
	xmlNodePtr reqnode2;
	xmlDocPtr doc2(empty_request(reqnode2));
	xmlNodePtr pgnode(xmlNewChild(reqnode2, 0, BAD_CAST "ProfileGraph", 0));
	if (m_profilegraph_svg)
		xmlNewProp(pgnode, BAD_CAST "mimetype", BAD_CAST "image/svg+xml");
	{
		std::ostringstream oss;
		oss << m_width;
		xmlNewProp(pgnode, BAD_CAST "width", BAD_CAST oss.str().c_str());
	}
	{
		std::ostringstream oss;
		oss << m_height;
		xmlNewProp(pgnode, BAD_CAST "height", BAD_CAST oss.str().c_str());
	}
	{
		int32_t maxalt(0);
		for (unsigned int i = 0; i < m_route.size(); ++i)
			maxalt = std::max(maxalt, (int32_t)m_route[i].get_altitude());
		maxalt += 5999;
		maxalt = (maxalt / 5000) * 5000;
		std::ostringstream oss;
		oss << maxalt;
		xmlNewProp(pgnode, BAD_CAST "hmin", BAD_CAST "0");
		xmlNewProp(pgnode, BAD_CAST "hmax", BAD_CAST oss.str().c_str());
	}
	xmlNewProp(pgnode, BAD_CAST "xmargin", BAD_CAST "5");
	xmlNewProp(pgnode, BAD_CAST "ymargin", BAD_CAST "5");
	xmlNewProp(pgnode, BAD_CAST "hdiv", BAD_CAST "3");
	{
		epochs_t::const_iterator eb(epochs.begin()), ee(epochs.end()), ei(eb);
		while (ei != ee && *ei < m_route[0].get_time_unix())
			++ei;
		if (ei == ee && ei != eb)
			--ei;
		if (ei != ee) {
			char buf[64];
			struct tm utm;
			m_profilegraph_epoch = *ei;
			strftime(buf, sizeof(buf), "%Y-%m-%d %H:%M:%S", gmtime_r(&m_profilegraph_epoch, &utm));
			xmlNewProp(pgnode, BAD_CAST "e", BAD_CAST buf);
		}
	}
	add_route(pgnode, m_route);
	xmlNewChild(pgnode, 0, BAD_CAST "flightpath", 0);
	xmlNewChild(pgnode, 0, BAD_CAST "terrain", 0);
	{
		xmlNodePtr wnode(xmlNewChild(pgnode, 0, BAD_CAST "Weather", 0));
		xmlNewTextChild(wnode, 0, BAD_CAST "theme", BAD_CAST "clouds");
		xmlNewTextChild(wnode, 0, BAD_CAST "theme", BAD_CAST "frzlvl");
	}
	start_request(doc2, mode_chart);
}
