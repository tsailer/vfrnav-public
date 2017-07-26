/*****************************************************************************/

/*
 *      adds.cc  --  FAA ADDS weather service.
 *
 *      Copyright (C) 2012, 2013  Thomas Sailer (t.sailer@alumni.ethz.ch)
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

#include "adds.h"
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
#include <libxml/tree.h>
#include <curl/curl.h>

const char ADDS::adds_url[] = "http://weather.aero/dataserver_current/httpparam?";
const char ADDS::adds_request_metar[] = "dataSource=metars&requestType=retrieve&format=xml";
const char ADDS::adds_request_taf[] = "dataSource=tafs&requestType=retrieve&format=xml";

ADDS::ADDS(void)
	: m_thread(0), m_maxage(0), m_mode(mode_abort)
{
	m_dispatch.connect(sigc::mem_fun(*this, &ADDS::complete));
}

ADDS::~ADDS(void)
{
	abort();
}

void ADDS::abort(void)
{
	if (!m_thread)
		return;
	m_mode = mode_abort;
	m_thread->join();
	m_thread = 0;
}

void ADDS::complete(void)
{
	if (m_thread)
		m_thread->join();
	m_thread = 0;
	if (false)
		std::cerr << "ADDS::complete: " << std::string(m_rxbuffer.begin(), m_rxbuffer.end()) << std::endl;
	//xmlDocPtr doc(xmlParseMemory(const_cast<char *>(&m_rxbuffer[0]), m_rxbuffer.size()));
	xmlDocPtr doc(xmlReadMemory(const_cast<char *>(&m_rxbuffer[0]), m_rxbuffer.size(), m_url.c_str(), 0, 0));
	xmlCleanupParser();
	m_rxbuffer.clear();
	xmlNodePtr reqnode(0);
	{	
		xmlNodePtr node(xmlDocGetRootElement(doc));
		for (; node && !reqnode; node = node->next) {
			if (node->type != XML_ELEMENT_NODE || xmlStrcmp(node->name, BAD_CAST "response"))
				continue;
			for (xmlNodePtr node2(node->children); node2; node2 = node2->next) {
				if (node2->type != XML_ELEMENT_NODE || xmlStrcmp(node2->name, BAD_CAST "data"))
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

	case mode_metarortaf:
		parse_metartaf(doc, reqnode);
		break;

	case mode_metartaf:
	{
		parse_metartaf(doc, reqnode);
		metartaf_t mt;
		mt.swap(m_metartaf);
		taf(m_bbox, m_metartaf_done, m_maxage);
		m_metartaf.swap(mt);
		break;
	}
	}
	xmlFreeDoc(doc);
}

size_t ADDS::my_write_func_1(const void *ptr, size_t size, size_t nmemb, ADDS *wx)
{
	return wx->my_write_func(ptr, size, nmemb);
}

size_t ADDS::my_read_func_1(void *ptr, size_t size, size_t nmemb, ADDS *wx)
{
	return wx->my_read_func(ptr, size, nmemb);
}

int ADDS::my_progress_func_1(ADDS *wx, double t, double d, double ultotal, double ulnow)
{
	return wx->my_progress_func(t, d, ultotal, ulnow);
}

size_t ADDS::my_write_func(const void *ptr, size_t size, size_t nmemb)
{
	unsigned int sz(size * nmemb);
	unsigned int p(m_rxbuffer.size());
	m_rxbuffer.resize(p + sz);
	memcpy(&m_rxbuffer[p], ptr, sz);
	return sz;
}

size_t ADDS::my_read_func(void *ptr, size_t size, size_t nmemb)
{
	return 0;
}

int ADDS::my_progress_func(double t, double d, double ultotal, double ulnow)
{
	return m_mode == mode_abort;
}

void ADDS::start_request(mode_t mode)
{
	if (m_url.empty())
		return;
	if (mode == mode_abort)
		return;
	if (m_thread)
		throw std::runtime_error("ADDS: request ongoing");
	m_rxbuffer.clear();
	if (true)
		std::cerr << "ADDS::start_request: " << m_url << std::endl;
	m_mode = mode;
	m_thread = Glib::Threads::Thread::create(sigc::mem_fun(*this, &ADDS::curlio));
}

void ADDS::curlio(void)
{
	CURL *curl(curl_easy_init());
	if (!curl) {
		m_dispatch();
		return;
	}
	struct curl_slist *headers(0);
	//headers = curl_slist_append(headers, "Content-Type: application/xml");
	curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);
	curl_easy_setopt(curl, CURLOPT_URL, m_url.c_str());
	curl_easy_setopt(curl, CURLOPT_WRITEDATA, this);
	curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, my_write_func_1);
	curl_easy_setopt(curl, CURLOPT_READDATA, this);
	curl_easy_setopt(curl, CURLOPT_READFUNCTION, my_read_func_1);
	curl_easy_setopt(curl, CURLOPT_NOPROGRESS, 0L);
	curl_easy_setopt(curl, CURLOPT_PROGRESSFUNCTION, my_progress_func_1);
	curl_easy_setopt(curl, CURLOPT_PROGRESSDATA, this);
	//curl_easy_setopt(curl, CURLOPT_POSTFIELDS, &m_txbuffer[0]);
	//curl_easy_setopt(curl, CURLOPT_POSTFIELDSIZE, m_txbuffer.size());
	CURLcode res = curl_easy_perform(curl);
	curl_easy_cleanup(curl);
	curl_slist_free_all(headers);
	if (res != CURLE_OK) {
		std::cerr << "ADDS::curlio: error " << res << std::endl;
		m_rxbuffer.clear();
	}	
	m_dispatch();
}

void ADDS::url_add_bbox(const Rect& bbox)
{
	std::ostringstream oss;
	oss << std::fixed << std::setprecision(6)
	    << "&minLat=" << bbox.get_southwest().get_lat_deg_dbl()
	    << "&minLon=" << bbox.get_southwest().get_lon_deg_dbl()
	    << "&maxLat=" << bbox.get_northeast().get_lat_deg_dbl()
	    << "&maxLon=" << bbox.get_northeast().get_lat_deg_dbl();
	m_url += oss.str();
}

void ADDS::url_add_maxage(unsigned int maxage)
{
	std::ostringstream oss;
	oss << "&hoursBeforeNow=" << maxage;
	m_url += oss.str();
}

void ADDS::metar(const Rect& bbox, const sigc::slot<void,const metartaf_t&>& cb, unsigned int maxage)
{
	m_metartaf.clear();
	if (bbox.is_empty()) {
		cb(m_metartaf);
		return;
	}
	m_url = adds_url;
	m_url += adds_request_metar;
	url_add_bbox(bbox);
	url_add_maxage(maxage);
	start_request(mode_metarortaf);
	m_metartaf_done = cb;
	m_bbox = bbox;
	m_maxage = maxage;
}

void ADDS::taf(const Rect& bbox, const sigc::slot<void,const metartaf_t&>& cb, unsigned int maxage)
{
	m_metartaf.clear();
	if (bbox.is_empty()) {
		cb(m_metartaf);
		return;
	}
	m_url = adds_url;
	m_url += adds_request_taf;
	url_add_bbox(bbox);
	url_add_maxage(maxage);
	start_request(mode_metarortaf);
	m_metartaf_done = cb;
	m_bbox = bbox;
	m_maxage = maxage;
}

void ADDS::metar_taf(const Rect& bbox, const sigc::slot<void,const metartaf_t&>& cb, unsigned int maxage_metar, unsigned int maxage_taf)
{
	metar(bbox, cb, maxage_metar);
	m_mode = mode_metartaf;
	m_maxage = maxage_taf;
}

void ADDS::parse_metartaf(xmlDocPtr doc, xmlNodePtr reqnode)
{
	if (!reqnode || !doc)
		return;
	metartaf_t mt;
	for (xmlNodePtr node(reqnode->children); node; node = node->next) {
		if (node->type != XML_ELEMENT_NODE)
			continue;
		MetarTaf::type_t typ;
		if (!xmlStrcmp(node->name, BAD_CAST "METAR"))
			typ = MetarTaf::type_metar;
		else if (!xmlStrcmp(node->name, BAD_CAST "TAF"))
			typ = MetarTaf::type_taf;
		else
			continue;
		Point pt;
		pt.set_invalid();
		std::string icao, msg;
		time_t epoch(0);
		for (xmlNodePtr node2(node->children); node2; node2 = node2->next) {
			xmlChar *txt(xmlNodeListGetString(doc, node2->xmlChildrenNode, 1));
			if (!txt)
				continue;
			char *txt1(reinterpret_cast<char *>(txt));
			if (false)
				std::cerr << "txt node " << (char *)node2->name << '=' << txt1 << std::endl;
			if (!xmlStrcmp(node2->name, BAD_CAST "longitude")) {
				char *cp;
				double x(strtod(txt1, &cp));
				if (txt1 != cp)
					pt.set_lon_deg_dbl(x);
			} else if (!xmlStrcmp(node2->name, BAD_CAST "latitude")) {
				char *cp;
				double x(strtod(txt1, &cp));
				if (txt1 != cp)
					pt.set_lat_deg_dbl(x);
			} else if (!xmlStrcmp(node2->name, BAD_CAST "station_id")) {
				icao = txt1;
			} else if ((typ == MetarTaf::type_metar && !xmlStrcmp(node2->name, BAD_CAST "observation_time")) ||
				   (typ == MetarTaf::type_taf && !xmlStrcmp(node2->name, BAD_CAST "issue_time"))) {
				Glib::TimeVal tv;
				if (tv.assign_from_iso8601(txt1))
					epoch = tv.tv_sec;
			} else if (!xmlStrcmp(node2->name, BAD_CAST "raw_text")) {
				msg = txt1;
			}
			xmlFree(txt);
		}
		if (false)
			std::cerr << (typ == MetarTaf::type_metar ? "METAR" : (typ == MetarTaf::type_taf ? "TAF" : "?""?"))
				  << " icao " << icao << " epoch " << epoch << ' ' << pt.get_lat_str2() << ' ' << pt.get_lon_str2()
				  << ' ' << msg << std::endl;
		if (pt.is_invalid() || !epoch || icao.empty() || msg.empty())
			continue;
		m_metartaf.insert(MetarTaf(icao, typ, epoch, msg, pt));
	}
	if (m_mode == mode_metarortaf) {
		m_metartaf_done(m_metartaf);
		m_metartaf.clear();
	}
}
