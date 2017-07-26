//
// C++ Interface: adds
//
// Description: FAA ADDS weather service
//
//
// Author: Thomas Sailer <t.sailer@alumni.ethz.ch>, (C) 2012, 2013
//
// Copyright: See COPYING file that comes with this distribution
//
//

#ifndef ADDS_H
#define ADDS_H

#include "sysdeps.h"

#include <limits>
#include <set>
#include <stdint.h>
#include <glibmm.h>
#include <gdkmm.h>
#include <libxml/tree.h>

#include "geom.h"
#include "fplan.h"
#include "wxdb.h"

class ADDS {
  public:
	ADDS(void);
	~ADDS(void);

	typedef WXDatabase::MetarTaf MetarTaf;

	void abort(void);
	typedef std::set<MetarTaf> metartaf_t;
	void metar(const Rect& bbox, const sigc::slot<void,const metartaf_t&>& cb, unsigned int maxage = 3);
	void taf(const Rect& bbox, const sigc::slot<void,const metartaf_t&>& cb, unsigned int maxage = 12);
	void metar_taf(const Rect& bbox, const sigc::slot<void,const metartaf_t&>& cb, unsigned int maxage_metar = 3, unsigned int maxage_taf = 12);

  protected:
	static const char adds_url[];
	static const char adds_request_metar[];
	static const char adds_request_taf[];
	Glib::Dispatcher m_dispatch;
	sigc::slot<void,const metartaf_t&> m_metartaf_done;
	std::vector<char> m_rxbuffer;
	std::string m_url;
	Glib::Threads::Thread *m_thread;
	metartaf_t m_metartaf;
	Rect m_bbox;
	unsigned int m_maxage;
	typedef enum {
		mode_abort,
		mode_metarortaf,
		mode_metartaf
	} mode_t;
	mode_t m_mode;

	void complete(void);
	static size_t my_write_func_1(const void *ptr, size_t size, size_t nmemb, ADDS *wx);
	static size_t my_read_func_1(void *ptr, size_t size, size_t nmemb, ADDS *wx);
	static int my_progress_func_1(ADDS *wx, double t, double d, double ultotal, double ulnow);
	size_t my_write_func(const void *ptr, size_t size, size_t nmemb);
	size_t my_read_func(void *ptr, size_t size, size_t nmemb);
	int my_progress_func(double t, double d, double ultotal, double ulnow);
	void start_request(mode_t mode);
	void curlio(void);
	void url_add_bbox(const Rect& bbox);
	void url_add_maxage(unsigned int maxage);
	void parse_metartaf(xmlDocPtr doc, xmlNodePtr reqnode);
};

#endif /* ADDS_H */
