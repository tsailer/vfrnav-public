//
// C++ Implementation: dbser
//
// Description: Database Object Serialization
//
//
// Author: Thomas Sailer <t.sailer@alumni.ethz.ch>, (C) 2012, 2014
//
// Copyright: See COPYING file that comes with this distribution
//
//

#include "sysdeps.h"

#include <limits>
#include <cstring>
#include <sstream>
#include <locale>
#include <vector>
#include <algorithm>

#include <zlib.h>

#include <sqlite3.h>

#include "dbobj.h"
#include "dbser.h"

Glib::ustring DbBaseCommon::DbSchemaArchiver::Table::Field::get_typeattr(void) const
{
	Glib::ustring t(get_type());
	if (get_flags() & DbBaseElements::DbElementBase::hibernate_id)
		t += " PRIMARY KEY";
	if (get_flags() & DbBaseElements::DbElementBase::hibernate_sqlunique)
		t += " UNIQUE";
	if (get_flags() & DbBaseElements::DbElementBase::hibernate_sqlnotnull)
		t += " NOT NULL";
	return t;
}

DbBaseCommon::DbSchemaArchiver::Table::Table(const std::vector<std::string>& tok)
	: m_fieldnr(0)
{
	if (tok.size() < 4 ||
	    !strcompare(tok[0], "create"))
		throw std::runtime_error("invalid table creation SQL statement (1)");
	std::vector<std::string>::size_type idx(1);
	bool virttbl(strcompare(tok[1], "virtual"));
	if (virttbl)
		idx = 2;
	if (!strcompare(tok[idx], "table"))
		throw std::runtime_error("invalid table creation SQL statement (2)");
	++idx;
	if (tok.size() >= idx + 3 && tok[idx] == "\"" && is_label(tok[idx + 1]) && tok[idx + 2] == "\"") {
		m_name = tok[idx + 1];
		idx += 3;
	} else if (tok.size() >= idx + 1 && is_label(tok[idx])) {
		m_name = tok[idx];
		++idx;
	} else 
		throw std::runtime_error("invalid table creation SQL statement (3)");
	if (tok.size() >= idx + 2 && strcompare(tok[idx], "using") && is_label(tok[idx + 1]))
		idx += 2;
	if (tok.size() <= idx || tok[idx] != "(")
		throw std::runtime_error("invalid table creation SQL statement (4)");
	++idx;
	while (tok.size() > idx && is_label(tok[idx]) &&
	       (virttbl || (tok.size() >= idx + 2 && is_label(tok[idx + 1])))) {
		std::string name(tok[idx++]);
		std::string type;
		if (!virttbl)
			type = tok[idx++];
		unsigned int flags(0);
		while (tok.size() > idx) {
			if (strcompare(tok[idx], "unique")) {
				flags |= DbBaseElements::DbElementBase::hibernate_sqlunique;
				++idx;
				continue;
			}
			if (idx + 1 >= tok.size())
				break;
			if (strcompare(tok[idx], "primary") && strcompare(tok[idx + 1], "key")) {
				flags |= DbBaseElements::DbElementBase::hibernate_id;
				idx += 2;
				continue;
			}
			if (strcompare(tok[idx], "not") && strcompare(tok[idx + 1], "null")) {
				flags |= DbBaseElements::DbElementBase::hibernate_sqlnotnull;
				idx += 2;
				continue;
			}
			break;
		}
		add_field(name, type, flags);
		if (tok.size() <= idx || tok[idx] != ",")
			break;
		++idx;
	}
	if (tok.size() != idx + 1 || tok[idx] != ")")
		throw std::runtime_error("invalid table creation SQL statement (5)");
}

void DbBaseCommon::DbSchemaArchiver::Table::add_field(const Glib::ustring& name, const Glib::ustring& type, unsigned int flags)
{
	std::pair<fields_t::iterator,bool> ins(m_fields.insert(Field(name, type, flags, m_fieldnr)));
	if (!ins.second)
		throw std::runtime_error("add_field: duplicate database field " + name);
	++m_fieldnr;
}

std::string DbBaseCommon::DbSchemaArchiver::Table::create_sql(void) const
{
	std::string r;
	bool subseq(false);
	typedef std::vector<Table::Field> tblflds_t;
	tblflds_t tblfld(begin(), end());
	std::sort(tblfld.begin(), tblfld.end(), DbBaseCommon::DbSchemaArchiver::Table::SortFieldsByNr());
	for (tblflds_t::const_iterator ti(tblfld.begin()), te(tblfld.end()); ti != te; ++ti) {
		if (subseq)
			r += ", ";
		else
			r += "CREATE TABLE IF NOT EXISTS " + get_name() + " (";
		subseq = true;
		r += ti->get_name() + " " + ti->get_typeattr();
	}
	if (subseq)
		r += ");";
	return r;
}

void DbBaseCommon::DbSchemaArchiver::Table::create_sql(sqlite3x::sqlite3_connection& db) const
{
	sqlite3x::sqlite3_command cmd(db, create_sql());
	cmd.executenonquery();
}

std::string DbBaseCommon::DbSchemaArchiver::Table::drop_sql(void) const
{
	return "DROP TABLE IF EXISTS " + get_name() + ";";
}

void DbBaseCommon::DbSchemaArchiver::Table::drop_sql(sqlite3x::sqlite3_connection& db) const
{
	sqlite3x::sqlite3_command cmd(db, drop_sql());
	cmd.executenonquery();
}

DbBaseCommon::DbSchemaArchiver::Index::Index(const std::vector<std::string>& tok)
	: m_flags(0)
{
	if (tok.size() < 6 ||
	    !strcompare(tok[0], "create") ||
	    !strcompare(tok[1], "index") ||
	    !is_label(tok[2]) ||
	    !strcompare(tok[3], "on") ||
	    !is_label(tok[4]) ||
	    tok[5] != "(")
		throw std::runtime_error("invalid index creation SQL statement");
	m_name = tok[2];
	m_table = tok[4];
	std::vector<std::string>::size_type idx(6);
	while (tok.size() > idx && is_label(tok[idx])) {
		if (!m_fields.empty())
			m_fields.push_back(',');
		m_fields += tok[idx++];
		if (idx >= tok.size() || tok[idx] != ",")
			break;
		++idx;
	}
	if (tok.size() >= idx + 2 &&
	    strcompare(tok[idx], "collate") &&
	    strcompare(tok[idx + 1], "nocase")) {
		idx += 2;
		m_flags |= DbBaseElements::DbElementBase::hibernate_sqlnocase;
	}
	if (tok.size() != idx + 1 || tok[idx] != ")")
		throw std::runtime_error("invalid index creation SQL statement (2)");
}

Glib::ustring DbBaseCommon::DbSchemaArchiver::Index::get_fieldattr(void) const
{
	Glib::ustring flds1(get_fields());
	if (get_flags() & DbBaseElements::DbElementBase::hibernate_sqlnocase)
		flds1 += " COLLATE NOCASE";
	flds1 = get_table() + "(" + flds1 + ")";
	return flds1;
}

std::string DbBaseCommon::DbSchemaArchiver::Index::create_sql(void) const
{
	return "CREATE INDEX IF NOT EXISTS " + get_name() + " ON " + get_fieldattr() + ";";
}

void DbBaseCommon::DbSchemaArchiver::Index::create_sql(sqlite3x::sqlite3_connection& db) const
{
	sqlite3x::sqlite3_command cmd(db, create_sql());
	cmd.executenonquery();
}

std::string DbBaseCommon::DbSchemaArchiver::Index::drop_sql(void) const
{
	return "DROP INDEX IF EXISTS " + get_name() + ";";
}

void DbBaseCommon::DbSchemaArchiver::Index::drop_sql(sqlite3x::sqlite3_connection& db) const
{
	sqlite3x::sqlite3_command cmd(db, drop_sql());
	cmd.executenonquery();
}

DbBaseCommon::DbSchemaArchiver::DbSchemaArchiver(const char *main_table_name)
	: m_currenttable(0)
{
	m_tablestack.push(main_table_name);
	{
		std::pair<tables_t::iterator,bool> ins(m_tables.insert(Table(main_table_name)));
		m_currenttable = const_cast<Table *>(&*ins.first);
	}
}

void DbBaseCommon::DbSchemaArchiver::add_key(const Glib::ustring& key, const Glib::ustring& type, unsigned int flags)
{
	if (!m_currenttable)
		throw std::runtime_error("add_key: no current table");
	m_currenttable->add_field(key, type, flags);
	add_index(key, key, flags, false);
	if (flags & DbBaseElements::DbElementBase::hibernate_sqldelete) {
		std::pair<tables_t::iterator,bool> ins(m_tables.insert(Table(m_tablestack.top() + "_deleted")));
		if (!ins.second)
			throw std::runtime_error("add_key: duplicate table " + ins.first->get_name());
		Table *tbl(const_cast<Table *>(&*ins.first));
		tbl->add_field(key, type, DbBaseElements::DbElementBase::hibernate_sqlnotnull | DbBaseElements::DbElementBase::hibernate_id);
	}
}

void DbBaseCommon::DbSchemaArchiver::add_index(const Glib::ustring& name, const Glib::ustring& flds, unsigned int flags, bool legacy)
{
	if (!(flags & DbBaseElements::DbElementBase::hibernate_sqlindex))
		return;
	flags &= DbBaseElements::DbElementBase::hibernate_sqlnocase;
	Glib::ustring name1(m_tablestack.top() + "_" + name.lowercase());
	std::pair<indices_t::iterator,bool> ins;
	if (legacy)
		ins = m_legacyindices.insert(Index(name1, m_tablestack.top(), flds, flags));
	else
		ins = m_indices.insert(Index(name1, m_tablestack.top(), flds, flags));
	if (!ins.second)
		throw std::runtime_error("add_index: duplicate index " + name1);
}

void DbBaseCommon::DbSchemaArchiver::push_subtable(const Glib::ustring& subtblname, const Glib::ustring& superid)
{
	std::pair<tables_t::iterator,bool> ins(m_tables.insert(Table(subtblname)));
	if (!ins.second)
		throw std::runtime_error("push_subtable: duplicate table " + subtblname);
	m_tablestack.push(subtblname);
	m_superidstack.push_back(superid);
	m_currenttable = const_cast<Table *>(&*ins.first);
	m_currenttable->add_field("ID", "INTEGER", DbBaseElements::DbElementBase::hibernate_sqlnotnull);
	for (superidstack_t::const_iterator i(m_superidstack.begin()), e(m_superidstack.end()); i != e; ++i)
		m_currenttable->add_field(*i, "INTEGER", DbBaseElements::DbElementBase::hibernate_sqlnotnull);
	{
		Glib::ustring fld;
		for (superidstack_t::const_iterator i(m_superidstack.begin()), e(m_superidstack.end()); i != e; ++i)
			fld += *i + ",";
		fld += "ID";
		add_index("ID", fld, DbBaseElements::DbElementBase::hibernate_sqlindex);
	}
}

void DbBaseCommon::DbSchemaArchiver::pop_subtable(void)
{
	if (m_tablestack.size() <= 1)
		throw std::runtime_error("pop_subtable: bottom of table stack");
	m_tablestack.pop();
	if (m_superidstack.empty())
		throw std::runtime_error("pop_subtable: bottom of superid stack");
	m_superidstack.pop_back();
	tables_t::iterator i(m_tables.find(m_tablestack.top()));
	if (i == m_tables.end())
		throw std::runtime_error("pop_subtable: cannot find table " + m_tablestack.top());
	m_currenttable = const_cast<Table *>(&*i);
}

void DbBaseCommon::DbSchemaArchiver::io(Point& pt, const char *latname, const char *lonname, const char *latxmlname, const char *lonxmlname, unsigned int flags)
{
	if (flags & DbBaseElements::DbElementBase::hibernate_sqlindex) {
		add_index(Glib::ustring(latname) + Glib::ustring(lonname), Glib::ustring(latname) + "," + Glib::ustring(lonname), flags, false);
		add_index(Glib::ustring(latname), Glib::ustring(latname), flags, true);
		add_index(Glib::ustring(lonname), Glib::ustring(lonname), flags, true);
		flags &= ~DbBaseElements::DbElementBase::hibernate_sqlindex;
	}
	add_key(latname, "INTEGER", flags);
	add_key(lonname, "INTEGER", flags);
}

void DbBaseCommon::DbSchemaArchiver::io(Rect& r, unsigned int flags)
{
	if (flags & DbBaseElements::DbElementBase::hibernate_sqlindex) {
		add_index("bbox", "SWLAT,NELAT,SWLON,NELON", flags, false);
		add_index("SWLAT", "SWLAT", flags, true);
		add_index("SWLON", "SWLON", flags, true);
		add_index("NELAT", "NELAT", flags, true);
		add_index("NELON", "NELON", flags, true);
		flags &= ~DbBaseElements::DbElementBase::hibernate_sqlindex;
	}
	add_key("SWLAT", "INTEGER", flags);
	add_key("SWLON", "INTEGER", flags);
	add_key("NELAT", "INTEGER", flags);
	add_key("NELON", "INTEGER", flags);
}

void DbBaseCommon::DbSchemaArchiver::io(uint8_t& tc, char& cls, const char *tcname, const char *clsname, const char *xmlname, unsigned int flags)
{
	add_key(tcname, "INTEGER", flags);
	add_key(clsname, "INTEGER", flags);
}

void DbBaseCommon::DbSchemaArchiver::io(DbBaseElements::Airspace::Component::operator_t& opr, const char *name, const char *xmlname, unsigned int flags)
{
	add_key(name, "INTEGER", flags);
}

std::ostream& DbBaseCommon::DbSchemaArchiver::print_schema(std::ostream& os) const
{
	for (tables_t::const_iterator di(m_tables.begin()), de(m_tables.end()); di != de; ++di) {
		const Table& tbl(*di);
		os << "Table " << tbl.get_name() << std::endl;
		typedef std::vector<Table::Field> tblflds_t;
		tblflds_t tblfld(tbl.begin(), tbl.end());
		std::sort(tblfld.begin(), tblfld.end(), DbBaseCommon::DbSchemaArchiver::Table::SortFieldsByNr());
		for (tblflds_t::const_iterator ti(tblfld.begin()), te(tblfld.end()); ti != te; ++ti)
			os << '\t' << ti->get_name() << ' ' << ti->get_typeattr() << std::endl;
	}
	os << "Indices:" << std::endl;
	for (indices_t::const_iterator ti(m_indices.begin()), te(m_indices.end()); ti != te; ++ti)
		os << '\t' << ti->get_name() << ' ' << ti->get_fieldattr() << std::endl;
	os << "Legacy Indices:" << std::endl;
	for (indices_t::const_iterator ti(m_legacyindices.begin()), te(m_legacyindices.end()); ti != te; ++ti)
		os << '\t' << ti->get_name() << ' ' << ti->get_fieldattr() << std::endl;
	return os;
}

bool DbBaseCommon::DbSchemaArchiver::is_label(char ch)
{
	return std::isalnum(ch) || ch == '_';
}

std::vector<std::string> DbBaseCommon::DbSchemaArchiver::tokenize(const std::string& s)
{
	std::vector<std::string> r;
	for (std::string::const_iterator si(s.begin()), se(s.end()); si != se; ) {
		if (is_label(*si)) {
			std::string::const_iterator si2(si);
			++si2;
			while (si2 != se && is_label(*si2))
				++si2;
			r.push_back(std::string(si, si2));
			si = si2;
			continue;
		}
		if (!std::isspace(*si))
			r.push_back(std::string(1, *si));
		++si;
	}
	return r;
}

std::string DbBaseCommon::DbSchemaArchiver::untokenize(const std::vector<std::string>& s, bool suppressspace)
{
	bool nospace(true);
	std::string r;
	for (std::vector<std::string>::const_iterator si(s.begin()), se(s.end()); si != se; ++si) {
		bool nospc2(!is_label(*si) && suppressspace);
		if (!(nospace || nospc2))
			r.push_back(' ');
		nospace = nospc2;
		r += *si;		
	}
	return r;
}

bool DbBaseCommon::DbSchemaArchiver::strcompare(const Glib::ustring& s, const char *s1)
{
	if (!s1)
		return s.empty();
	return s.casefold() == Glib::ustring(s1).casefold();
}

bool DbBaseCommon::DbSchemaArchiver::strcompare(const std::vector<std::string>& tok, std::vector<std::string>::size_type idx, const char *s1)
{
	if (idx >= tok.size())
		return strcompare("", s1);
	return strcompare(tok[idx], s1);
}

void DbBaseCommon::DbSchemaArchiver::verify_schema(sqlite3x::sqlite3_connection& db) const
{
	const bool log = false;
	const bool logsql = false;
	if (log)
		std::cerr << "Table Schema Modification:" << std::endl;
	tables_t tables(m_tables);
	indices_t indices(m_indices);
	indices.insert(m_legacyindices.begin(), m_legacyindices.end());
        sqlite3x::sqlite3_command cmd(db, "SELECT type,name,tbl_name,rootpage,sql FROM sqlite_master;");
	sqlite3x::sqlite3_cursor cursor(cmd.executecursor());
	while (cursor.step()) {
		std::string type(cursor.getstring(0));
		std::string name(cursor.getstring(1));
		std::string tblname(cursor.getstring(2));
		int rootpage(cursor.getint(3));
		std::vector<std::string> sqltok(tokenize(cursor.getstring(4)));
		if (!Glib::ustring(name).lowercase().compare(0, 6, "sqlite"))
			continue;
		if (false && log)
			std::cerr << "  -- " << type << ',' << name << ',' << tblname << ','
				  << rootpage << ',' << untokenize(sqltok) << std::endl;
		if (type == "table") {
			Table tbl(sqltok);
			tables_t::const_iterator tbli(tables.find(tbl.get_name()));
			if (tbli == tables.end()) {
				if (logsql)
					std::cerr << "  " << tbl.drop_sql() << std::endl;
				continue;
			}
			Table::const_iterator f1b(tbl.begin()), f1e(tbl.end()), f2b(tbli->begin()), f2e(tbli->end());
			while (f1b != f1e || f2b != f2e) {
				if (true && log) {
					std::cerr << "  --tbl field: ";
					if (f1b == f1e)
						std::cerr << "-,";
					else
						std::cerr << f1b->get_name() << ',';
					if (f2b == f2e)
						std::cerr << '-' << std::endl;
					else
						std::cerr << f2b->get_name() << std::endl;
				}
				if (f1b != f1e && (f2b == f2e || f1b->get_name() < f2b->get_name())) {
					if (log)
						std::cerr << "  --ALTER TABLE " << tbl.get_name() << " DROP COLUMN "
							  << f1b->get_name() << " " << f1b->get_typeattr() << ';' << std::endl;
					++f1b;
					continue;
				}
				if (f2b != f2e && (f1b == f1e || f2b->get_name() < f1b->get_name())) {
					if (log)
						std::cerr << "  ALTER TABLE " << tbl.get_name() << " ADD COLUMN "
							  << f2b->get_name() << " " << f1b->get_typeattr() << ';' << std::endl;
					++f2b;
					continue;
				}
				++f1b;
				++f2b;
			}
			tables.erase(tbli);
			continue;
		}
		if (type == "index") {
			Index idx(sqltok);
			indices_t::const_iterator idxi(indices.find(idx));
			if (idxi == indices.end()) {
				if (logsql)
					std::cerr << "  " << idx.drop_sql() << std::endl;
				continue;
			}
			if (idx.get_table() == idxi->get_table() &&
			    idx.get_fields() == idxi->get_fields() &&
			    idx.get_flags() == idxi->get_flags()) {
				indices.erase(idxi);
				continue;
			}
			if (false && log)
				std::cerr << "  -- replace index " << idx.get_table() << ',' << idx.get_fields() << ',' << idx.get_flags()
					  << " with " << idxi->get_table() << ',' << idxi->get_fields() << ',' << idxi->get_flags() << std::endl;
			if (logsql)
				std::cerr << "  " << idx.drop_sql() << std::endl
					  << "  " << idxi->create_sql() << std::endl;
			indices.erase(idxi);
			continue;
		}
	}
	if (log) {
		for (tables_t::const_iterator ti(tables.begin()), te(tables.end()); ti != te; ++ti)
			std::cerr << "  " << ti->create_sql() << std::endl;
		for (indices_t::const_iterator ii(indices.begin()), ie(indices.end()); ii != ie; ++ii)
			std::cerr << "  " << ii->create_sql() << std::endl;
	}
}
