/***************************************************************************
 *   Copyright (C) 2012 by Thomas Sailer                                   *
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

#include <iomanip>
#include <glibmm/datetime.h>

#include "flightdeckwindow.h"

#ifdef HAVE_EVINCE

FlightDeckWindow::DocumentsDirectoryModelColumns::DocumentsDirectoryModelColumns(void)
{
	add(m_text);
	add(m_weight);
	add(m_style);
	add(m_document);
}

FlightDeckWindow::Document::Document(const Glib::ustring& filename)
	: m_filename(filename), m_creation_date(0), m_modified_date(0), m_n_pages(0), m_page(0),
	  m_rotation(0), m_scale(1), m_sizingmode(EV_SIZING_FIT_WIDTH), m_deleted(false)
{
}

FlightDeckWindow::Document::~Document()
{
}

void FlightDeckWindow::Document::set_info(EvDocument *doc)
{
	if (!doc)
		return;
	m_n_pages = ev_document_get_n_pages(doc);;
	EvDocumentInfo *info(ev_document_get_info(doc));
	if (!info)
		return;
	if (info->title)
		m_title = info->title;
	if (info->format)
		m_format = info->format;
	if (info->author)
		m_author = info->author;
	if (info->subject)
		m_subject = info->subject;
	if (info->keywords)
		m_keywords = info->keywords;
	if (info->creator)
		m_creator = info->creator;
	if (info->producer)
		m_producer = info->producer;
	if (info->linearized)
		m_linearized = info->linearized;
	if (info->security)
		m_security = info->security;
	m_creation_date = info->creation_date;
	m_modified_date = info->modified_date;
}

bool FlightDeckWindow::Document::verify(void)
{
	GError *error(0);
	Glib::ustring uri(Glib::filename_to_uri(get_pathname()));
	if (true)
		std::cerr << "Checking document: " << uri << std::endl;
	EvDocument *evdoc(ev_document_factory_get_document(uri.c_str(), &error));
	if (!evdoc) {
		std::cerr << "Cannot open document " << uri;
		if (error)
			std::cerr << ": " << error->message;
		std::cerr << std::endl;
		g_error_free(error);
		error = 0;
		return false;
	}
	set_info(evdoc);
	g_object_unref(G_OBJECT(evdoc));
	Glib::RefPtr<Gio::File> gfile(Gio::File::create_for_path(get_pathname()));
	Glib::RefPtr<Gio::FileInfo> ginfo(gfile->query_info(G_FILE_ATTRIBUTE_TIME_MODIFIED));
	set_modification_time(ginfo->modification_time());
	return true;
}

void FlightDeckWindow::Document::mark_deleted(void)
{
	m_deleted = true;
	m_token.clear();
}

void FlightDeckWindow::Document::undelete(void)
{
	m_deleted = false;
}

Glib::ustring FlightDeckWindow::Document::get_filename(void) const
{
	Glib::ustring fn(get_pathname());
	Glib::ustring::size_type i(fn.rfind('/'));
	if (i == Glib::ustring::npos)
		return fn;
	fn.erase(0, i + 1);
	return fn;
}

Glib::ustring FlightDeckWindow::Document::get_uri(void) const
{
	return Glib::filename_to_uri(get_pathname());
}

int FlightDeckWindow::Document::compare(const Document& x) const
{
	if (get_pathname() < x.get_pathname())
		return -1;
	if (get_pathname() > x.get_pathname())
		return 1;
	return 0;
}

template<typename T> void FlightDeckWindow::Document::add_token(T b, T e)
{
	m_token.clear();
	for (; b != e; ++b)
		m_token.insert(b->casefold());
}

bool FlightDeckWindow::Document::has_token(const Glib::ustring& x) const
{
	return m_token.find(x.casefold()) != m_token.end();
}

template<typename T> bool FlightDeckWindow::Document::has_any_token(T b, T e) const
{
	for (; b != e; ++b)
		if (has_token(*b))
			return true;
	return false;
}

FlightDeckWindow::Document::tokenvec_t FlightDeckWindow::Document::split_filename(const Glib::ustring& fn)
{
	tokenvec_t r;
	Glib::ustring::const_iterator b(fn.begin()), e(fn.end());
	while (b != e) {
		if (*b == '/') {
			++b;
			continue;
		}
		Glib::ustring::const_iterator c(b);
		++c;
		while (c != e && *c != '/')
			++c;
		r.push_back(Glib::ustring(b, c));
		b = c;
	}
	return r;
}

FlightDeckWindow::Document::tokenvec_t FlightDeckWindow::Document::split_token(const Glib::ustring& fn)
{
	tokenvec_t r;
	Glib::ustring::const_iterator b(fn.begin()), e(fn.end());
	while (b != e) {
		if (!std::isalnum(*b)) {
			++b;
			continue;
		}
		Glib::ustring::const_iterator c(b);
		++c;
		while (c != e && std::isalnum(*c))
			++c;
		r.push_back(Glib::ustring(b, c).casefold());
		b = c;
	}
	return r;
}

void FlightDeckWindow::Document::insert_token(token_t& tok, const Glib::ustring& fn)
{
	tokenvec_t tv(split_token(fn));
	tok.insert(tv.begin(), tv.end());
}

bool FlightDeckWindow::Document::has_globchars(const Glib::ustring& fn)
{
	return fn.find('*') != Glib::ustring::npos || fn.find('?') != Glib::ustring::npos;
}

void FlightDeckWindow::Document::filter_token(token_t& tok, unsigned int minlength)
{
	token_t::iterator i(tok.begin());
	while (i != tok.end()) {
		if (i->size() >= minlength) {
			++i;
			continue;
		}
		token_t::iterator i1(i);
		++i;
		tok.erase(i1);
	}
}

void FlightDeckWindow::Document::filter_token_generic(token_t& tok, const char * const *wordlist)
{
	static const char * const default_wordlist[] = {
		"VOR", "DME", "NDB", "LOC", "Localizer", "GS", "Glideslope", 0
	};
	if (!wordlist)
		wordlist = default_wordlist;
	for (; *wordlist; ++wordlist) {
		token_t::iterator i(tok.find(Glib::ustring(*wordlist).casefold()));
		if (i == tok.end())
			continue;
		tok.erase(i);
	}
}

void FlightDeckWindow::Document::save_view(EvDocumentModel *model)
{
	if (!model)
		return;
	m_page = ev_document_model_get_page(model);
	m_rotation = ev_document_model_get_rotation(model);
	m_scale = ev_document_model_get_scale(model);
	m_sizingmode = ev_document_model_get_sizing_mode(model);
}

void FlightDeckWindow::Document::restore_view(EvDocumentModel *model) const
{
	if (!model)
		return;
	ev_document_model_set_page(model, m_page);
	ev_document_model_set_rotation(model, m_rotation);
	ev_document_model_set_sizing_mode(model, m_sizingmode);
	if (m_sizingmode == EV_SIZING_FREE)
		ev_document_model_set_scale(model, m_scale);
}

bool FlightDeckWindow::docdir_select_function(const Glib::RefPtr<Gtk::TreeModel>& model, const Gtk::TreeModel::Path& path, bool currently_selected)
{
        if (currently_selected)
                return true;
        Gtk::TreeModel::iterator iter(model->get_iter(path));
        if (!iter)
                return false;
        Gtk::TreeModel::Row row(*iter);
        if (!row[m_docdircolumns.m_document])
                return false;
        return true;
}

void FlightDeckWindow::docdir_selection_changed(void)
{
}

void FlightDeckWindow::docdir_page_changed(EvDocumentModel *evdocumentmodel, gint arg1, gint arg2, gpointer userdata)
{
	if (!userdata)
		return;
	static_cast<FlightDeckWindow *>(userdata)->docdir_save_view();
}

void FlightDeckWindow::docdir_save_view(void)
{
	if (!m_docevdocmodel)
	       return;
	EvDocument *evdoc(ev_document_model_get_document(m_docevdocmodel));
	if (!evdoc)
		return;
	const gchar *uri(ev_document_get_uri(evdoc));
	if (!uri)
		return;
	Glib::ustring fname(Glib::filename_from_uri(uri));
	documents_t::iterator di(m_documents.find(fname));
	if (di == m_documents.end()) {
		std::cerr << "docdir_save_view: cannot find filename " << fname << std::endl;
		return;
	}
	Document *doc(const_cast<Document *>(&*di));
	doc->save_view(m_docevdocmodel);
}

bool FlightDeckWindow::docdir_display_document(void)
{
       if (!m_docdirtreeview || !m_docevdocmodel)
	       return false;
       Glib::RefPtr<Gtk::TreeSelection> selection(m_docdirtreeview->get_selection());
       if (!selection)
	       return false;
       return docdir_display_document(selection->get_selected());
}

bool FlightDeckWindow::docdir_display_document(Gtk::TreeIter iter)
{
       if (!iter)
                return false;
       Gtk::TreeModel::Row row(*iter);
       Document *doc(row[m_docdircolumns.m_document]);
       if (!doc)
	       return false;
       GError *error(0);
       EvDocument *evdoc(ev_document_factory_get_document(doc->get_uri().c_str(), &error));
       if (!evdoc) {
	       std::cerr << "Cannot open document " << doc->get_uri();
	       if (error)
		       std::cerr << ": " << error->message;
	       std::cerr << std::endl;
	       g_error_free(error);
	       error = 0;
	       return false;
       }
       ev_document_model_set_document(m_docevdocmodel, evdoc);
       g_object_unref(evdoc);
       ev_document_model_set_sizing_mode(m_docevdocmodel, EV_SIZING_FIT_WIDTH);
       ev_document_model_set_continuous(m_docevdocmodel, true);
       ev_document_model_set_dual_page(m_docevdocmodel, false);
       doc->restore_view(m_docevdocmodel);
       if (m_docframelabel)
	       m_docframelabel->set_markup("<b>Document:</b> " + doc->get_filename());
       return true;
}

void FlightDeckWindow::docdirpage_scandirs(void)
{
	if (!m_sensors)
		return;
	if (!m_sensors->get_configfile().has_key(cfggroup_mainwindow, cfgkey_mainwindow_docdirs)) {
		std::vector<Glib::ustring> dirs;
		m_sensors->get_configfile().set_string_list(cfggroup_mainwindow, cfgkey_mainwindow_docdirs, dirs);
	}
	std::vector<Glib::ustring> dirs(m_sensors->get_configfile().get_string_list(cfggroup_mainwindow, cfgkey_mainwindow_docdirs));
	bool alltoken(false);
	if (!m_sensors->get_configfile().has_key(cfggroup_mainwindow, cfgkey_mainwindow_docalltoken))
		m_sensors->get_configfile().set_integer(cfggroup_mainwindow, cfgkey_mainwindow_docalltoken, alltoken);
	alltoken = !!m_sensors->get_configfile().get_integer(cfggroup_mainwindow, cfgkey_mainwindow_docalltoken);
	m_sensors->save_config();
	for (documents_t::iterator di(m_documents.begin()), de(m_documents.end()); di != de; ++di) {
		Document *doc(const_cast<Document *>(&*di));
		doc->mark_deleted();
	}
	m_documentsunverified.clear();
	for (std::vector<Glib::ustring>::const_iterator di(dirs.begin()), de(dirs.end()); di != de; ++di)
		docdirpage_scandir(*di, alltoken);
	if (m_documentsunverified.empty()) {
		docdirpage_updatefilelist();
	} else {
		m_docdirverifyconn.disconnect();
		m_docdirverifyconn = Glib::signal_idle().connect(sigc::mem_fun(*this, &FlightDeckWindow::docdirpage_verify));
		m_docdirupdatetime.assign_current_time();
		m_docdirupdatetime.add_seconds(5);
	}
}

void FlightDeckWindow::docdirpage_scandir(const Glib::ustring& dir, bool alltoken)
{
	Document::tokenvec_t fncomp(Document::split_filename(dir));
	if (fncomp.empty())
		return;
	if (fncomp[0] == "~") {
		Document::tokenvec_t fncomp1(Document::split_filename(Glib::get_home_dir()));
		fncomp1.insert(fncomp1.end(), fncomp.begin() + 1, fncomp.end());
		fncomp.swap(fncomp1);
	}
	typedef std::map<Glib::ustring,Document::token_t> fnames_t;
	fnames_t fnames;
	fnames.insert(std::make_pair("/", Document::token_t()));
	for (Document::tokenvec_t::const_iterator fncompb(fncomp.begin()), fncompe(fncomp.end()); fncompb != fncompe; ++fncompb) {
		if (true)
			std::cerr << "Iterating over path component: " << *fncompb << std::endl;
		fnames_t fnames1;
		if (!Document::has_globchars(*fncompb)) {
			Document::tokenvec_t fncomptok;
			if (alltoken)
				fncomptok = Document::split_token(*fncompb);
			for (fnames_t::const_iterator fi(fnames.begin()), fe(fnames.end()); fi != fe; ++fi) {
				Glib::ustring fn(fi->first);
				if (fn.empty() || fn[fn.size() - 1] != '/')
					fn += '/';
				fn += *fncompb;
				std::pair<fnames_t::iterator,bool> ins(fnames1.insert(std::make_pair(fn, fi->second)));
				ins.first->second.insert(fncomptok.begin(), fncomptok.end());
				if (true)
					std::cerr << "New path: " << fn << std::endl;
			}
			fnames.swap(fnames1);
			continue;
		}
		for (fnames_t::const_iterator fi(fnames.begin()), fe(fnames.end()); fi != fe; ++fi) {
			Glib::ustring fn(fi->first);
			if (!Glib::file_test(fn, Glib::FILE_TEST_EXISTS))
				continue;
			if (!Glib::file_test(fn, Glib::FILE_TEST_IS_DIR))
				continue;
			Glib::Dir dir(fn);
			if (fn.empty() || fn[fn.size() - 1] != '/')
				fn += '/';
			Glib::PatternSpec pspec(*fncompb);
			for (;;) {
				Glib::ustring dirc(dir.read_name());
				if (dirc.empty())
					break;
				if (!pspec.match(dirc))
					continue;
				std::pair<fnames_t::iterator,bool> ins(fnames1.insert(std::make_pair(fn + dirc, fi->second)));
				Document::insert_token(ins.first->second, dirc);
				if (true)
					std::cerr << "New path: " << fn + dirc << std::endl;
			}
		}
		fnames.swap(fnames1);
	}
	for (fnames_t::const_iterator fi(fnames.begin()), fe(fnames.end()); fi != fe; ++fi) {
		{
			documents_t::iterator di(m_documents.find(fi->first));
			if (di != m_documents.end()) {
				Document *doc(const_cast<Document *>(&*di));
				bool ok(true);
				if (doc->is_deleted()) {
					Glib::RefPtr<Gio::File> gfile(Gio::File::create_for_path(doc->get_pathname()));
					Glib::RefPtr<Gio::FileInfo> ginfo(gfile->query_info(G_FILE_ATTRIBUTE_TIME_MODIFIED));
					ok = doc->get_modification_time() == ginfo->modification_time();
				}
				if (ok) {
					doc->undelete();
					doc->add_token(fi->second.begin(), fi->second.end());
					continue;
				}
			}
		}
		if (!Glib::file_test(fi->first, Glib::FILE_TEST_EXISTS))
			continue;
		if (Glib::file_test(fi->first, Glib::FILE_TEST_IS_DIR))
			continue;
		std::pair<documents_t::iterator,bool> ins(m_documentsunverified.insert(Document(fi->first)));
		Document *doc(const_cast<Document *>(&*ins.first));
		doc->undelete();
		doc->add_token(fi->second.begin(), fi->second.end());
	}
}

bool FlightDeckWindow::docdirpage_verify(void)
{
	m_docdirverifyconn.disconnect();
	if (m_documentsunverified.empty())
		return false;
	Document *doc(const_cast<Document *>(&*m_documentsunverified.begin()));
	if (true)
		std::cerr << "docdirpage_verify: " << doc->get_pathname() << std::endl;
	if (doc->verify())
		m_documents.insert(*m_documentsunverified.begin());
	m_documentsunverified.erase(m_documentsunverified.begin());
	if (m_documentsunverified.empty()) {
		docdirpage_updatefilelist();
		return false;
	}
	{
		Glib::TimeVal tv;
		tv.assign_current_time();
		tv -= m_docdirupdatetime;
		if (!tv.negative()) {
			docdirpage_updatefilelist();
			m_docdirupdatetime.assign_current_time();
			m_docdirupdatetime.add_seconds(5);
		}
	}
	m_docdirverifyconn = Glib::signal_idle().connect(sigc::mem_fun(*this, &FlightDeckWindow::docdirpage_verify));
	return false;
}

void FlightDeckWindow::docdirpage_addpage(Gtk::TreeIter iter, Document *doc)
{
	if (!iter || !doc)
		return;
	Gtk::TreeModel::Row row(*iter);
	row[m_docdircolumns.m_text] = doc->get_filename();
	row[m_docdircolumns.m_weight] = 600;
	row[m_docdircolumns.m_style] = Pango::STYLE_NORMAL;
	row[m_docdircolumns.m_document] = doc;
	Gtk::TreeIter iter1(row.children().begin());
	if (!iter1)
		iter1 = m_docdirstore->append(row.children());
	Gtk::TreeModel::Row row1(*iter1);
	row1[m_docdircolumns.m_text] = doc->get_pathname();
	row1[m_docdircolumns.m_weight] = 400;
	row1[m_docdircolumns.m_style] = Pango::STYLE_NORMAL;
	row1[m_docdircolumns.m_document] = doc;
	++iter1;
	if (!iter1)
		iter1 = m_docdirstore->append(row.children());
	row1 = *iter1;
	{
		Glib::DateTime cdate(Glib::DateTime::create_now_utc(doc->get_creation_date()));
		Glib::DateTime mdate(Glib::DateTime::create_now_utc(doc->get_modified_date()));
		Glib::DateTime mtime(Glib::DateTime::create_now_utc(doc->get_modification_time()));
		// %Y-%b-%d or %Y-%m-%d
		row1[m_docdircolumns.m_text] = Glib::ustring::format("Pages: ", doc->get_n_pages()) +
			" File Modified: " + mtime.format("%Y-%b-%d %H:%M:%Sz") +
			" Created: " + cdate.format("%Y-%b-%d %H:%M:%Sz") +
			" Modified: " + mdate.format("%Y-%b-%d %H:%M:%Sz") +
			" Format: " + doc->get_format();
	}
	row1[m_docdircolumns.m_weight] = 400;
	row1[m_docdircolumns.m_style] = Pango::STYLE_NORMAL;
	row1[m_docdircolumns.m_document] = doc;
	++iter1;
	{
		Glib::ustring t(doc->get_title());
		if (!t.empty() && t.validate()) {
			if (!iter1)
				iter1 = m_docdirstore->append(row.children());
			row1 = *iter1;
			row1[m_docdircolumns.m_text] = t;
			row1[m_docdircolumns.m_weight] = 400;
			row1[m_docdircolumns.m_style] = Pango::STYLE_NORMAL;
			row1[m_docdircolumns.m_document] = doc;
			++iter1;
		}
	}
	{
		Glib::ustring t(doc->get_subject());
		if (!t.empty() && t.validate()) {
			if (!iter1)
				iter1 = m_docdirstore->append(row.children());
			row1 = *iter1;
			row1[m_docdircolumns.m_text] = t;
			row1[m_docdircolumns.m_weight] = 400;
			row1[m_docdircolumns.m_style] = Pango::STYLE_NORMAL;
			row1[m_docdircolumns.m_document] = doc;
			++iter1;
		}
	}
	{
		Glib::ustring t(doc->get_author());
		if (!t.empty() && t.validate()) {
			if (!iter1)
				iter1 = m_docdirstore->append(row.children());
			row1 = *iter1;
			row1[m_docdircolumns.m_text] = t;
			row1[m_docdircolumns.m_weight] = 400;
			row1[m_docdircolumns.m_style] = Pango::STYLE_NORMAL;
			row1[m_docdircolumns.m_document] = doc;
			++iter1;
		}
	}
	while (iter1)
		iter1 = m_docdirstore->erase(iter1);
}

void FlightDeckWindow::docdirpage_updatefilelist(void)
{
	m_tokendocumentmap.clear();
	for (documents_t::iterator di(m_documents.begin()), de(m_documents.end()); di != de;) {
		if (di->is_deleted()) {
			documents_t::iterator i(di);
			++di;
			m_documents.erase(i);
			continue;
		}
		Document *doc(const_cast<Document *>(&*di));
		++di;
		doc->filter_token(3);
		doc->filter_token_generic();
		const Document::token_t& tok(doc->get_token());
		for (Document::token_t::const_iterator ti(tok.begin()), te(tok.end()); ti != te; ++ti)
			m_tokendocumentmap.insert(std::make_pair(*ti, doc));
	}
	Gtk::TreeIter iter(m_docdirstore->children().begin());
	Gtk::TreeIter seliter;
	if (m_sensors) {
		typedef std::map<Glib::ustring,Gtk::TreeIter> alreadyset_t;
		alreadyset_t alreadyset;
		const FPlanRoute& fpl(m_sensors->get_route());
		for (unsigned int nr = 0; nr < fpl.get_nrwpt(); ++nr) {
			Glib::ustring name(fpl[nr].get_icao_name());
			if (name.empty())
				continue;
			std::pair<alreadyset_t::iterator,bool> ins;
			{
				ins = alreadyset.insert(std::make_pair(name.casefold(), Gtk::TreeIter()));
				if (!ins.second) {
					if (nr + 1 == fpl.get_curwpt())
						seliter = ins.first->second;
					continue;
				}
			}
			typedef std::set<Document *> docs_t;
			docs_t docs;
			{
				Document::token_t tok;
				{
					Document::tokenvec_t tok1(Document::split_token(name));
					tok.insert(tok1.begin(), tok1.end());
				}
				Document::filter_token(tok, 3);
				Document::filter_token_generic(tok);
				if (tok.empty())
					continue;
				for (Document::token_t::const_iterator ti(tok.begin()), te(tok.end()); ti != te; ++ti) {
					std::pair<tokendocumentmap_t::const_iterator,tokendocumentmap_t::const_iterator> m(m_tokendocumentmap.equal_range(*ti));
					for (; m.first != m.second; ++m.first)
						docs.insert(m.first->second);
				}
			}
			if (docs.empty())
				continue;
			typedef std::map<Glib::ustring, Document *> docssort_t;
			docssort_t docssort;
			for (docs_t::const_iterator di(docs.begin()), de(docs.end()); di != de; ++di)
				docssort.insert(std::make_pair((*di)->get_pathname(), *di));
			bool newrow(!iter);
			if (newrow) {
				iter = m_docdirstore->append();
			} else {
				Gtk::TreeIter iter2(iter);
				++iter2;
				if (!iter2) {
					iter = m_docdirstore->insert(iter);
					newrow = true;
				}
			}
			Gtk::TreeModel::Row row(*iter);
			row[m_docdircolumns.m_text] = name;
			row[m_docdircolumns.m_weight] = 600;
			row[m_docdircolumns.m_style] = Pango::STYLE_ITALIC;
			row[m_docdircolumns.m_document] = 0;
			ins.first->second = iter;
			if (nr + 1 == fpl.get_curwpt())
				seliter = iter;
			Gtk::TreeIter iter2(row.children().begin());
			for (docssort_t::const_iterator di(docssort.begin()), de(docssort.end()); di != de; ++di) {
				if (!iter2)
					iter2 = m_docdirstore->append(row.children());
				docdirpage_addpage(iter2, di->second);
				++iter2;
			}
			while (iter2)
				iter2 = m_docdirstore->erase(iter2);
			if (newrow && m_docdirtreeview)
				m_docdirtreeview->expand_row(Gtk::TreePath(iter), false);
			++iter;
		}
	}
	{
		bool newrow(!iter);
		if (newrow)
			iter = m_docdirstore->append();
		Gtk::TreeModel::Row row(*iter);
		row[m_docdircolumns.m_text] = "All Documents";
		row[m_docdircolumns.m_weight] = 600;
		row[m_docdircolumns.m_style] = Pango::STYLE_ITALIC;
		row[m_docdircolumns.m_document] = 0;
		Gtk::TreeIter iter2(row.children().begin());
		for (documents_t::iterator di(m_documents.begin()), de(m_documents.end()); di != de; ++di) {
			if (!iter2)
				iter2 = m_docdirstore->append(row.children());
			docdirpage_addpage(iter2, const_cast<Document *>(&*di));
			++iter2;
		}
		if (newrow && m_docdirtreeview)
			m_docdirtreeview->expand_row(Gtk::TreePath(iter), false);
		++iter;
	}
	while (iter)
		iter = m_docdirstore->erase(iter);
	if (m_docdirtreeview) {
		Glib::RefPtr<Gtk::TreeSelection> selection(m_docdirtreeview->get_selection());
		if (selection) {
			Gtk::TreeModel::iterator iter(selection->get_selected());
			if (seliter) {
				Gtk::TreeModel::Row row(*seliter);
				seliter = row.children().begin();
				if (!iter)
					selection->select(seliter);
			} else {
				seliter = selection->get_selected();
			}
		}
		if (seliter)
			m_docdirtreeview->scroll_to_row(Gtk::TreePath(seliter));
	}
}

void FlightDeckWindow::docdirpage_updatemenu(void)
{
	if (m_menuid != 300)
		return;
	docdirpage_scandirs();
}

#else

void FlightDeckWindow::docdirpage_updatemenu(void)
{
}

#endif

#ifdef HAVE_EVINCE

bool FlightDeckWindow::docpage_on_motion_notify_event(GdkEventMotion *event)
{
#if 0
	std::cerr << "DocPage: Motion: x=" << event->x << " y=" << event->y << std::endl;
	if (!m_docevdocmodel || !m_docevview)
		return false;
	EvDocument *doc(ev_document_model_get_document(m_docevdocmodel));
	if (!doc)
		return false;
	gint n_pages(ev_document_get_n_pages(doc));
	if (n_pages <= 0)
		return false;
	EvView *view((EvView *)m_docevview->gobj());
	if (!view)
		return false;
	int dx(event->x), dy(event->y);
	if (m_docscrolledwindow) {
		Glib::RefPtr<const Gtk::Adjustment> hadj(m_docscrolledwindow->get_hadjustment());
		if (hadj)
			dx += hadj->get_value();
		Glib::RefPtr<const Gtk::Adjustment> vadj(m_docscrolledwindow->get_vadjustment());
		if (vadj)
			dy += vadj->get_value();
	}
	for (gint page_nr = 0; page_nr < n_pages; ++page_nr) {
		GdkRectangle page_area;
		GtkBorder border;
		ev_view_get_page_extents(view, page_nr, &page_area, &border);
		int width(page_area.width - border.left - border.right);
		int height(page_area.height - border.top - border.bottom);
		int x(page_area.x + border.left);
		int y(page_area.y + border.top);
		if (dx >= x && dx < x + width && dy >= y && dy < y + height) {
			std::cerr << "DocPage: PDF page " << page_nr << " x " << (event->x - x) << '/' << width << '/' << (dx - x)
				  << " y " << (event->y - y) << '/' << height << '/' << (dy - y) << std::endl;
		}
	}
#endif
	return false;
}

#endif
