#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <iomanip>
#include <sstream>

#include "adraup.hh"
#include "adrhibernate.hh"

using namespace ADR;

const std::string& to_str(ADR::ADRAUPObjectBase::type_t t)
{
	switch (t) {
	case ADR::ADRAUPObjectBase::type_cdr:
	{
		static const std::string r("cdr");
		return r;
	}

	case ADR::ADRAUPObjectBase::type_rsa:
	{
		static const std::string r("rsa");
		return r;
	}

	case ADR::ADRAUPObjectBase::type_invalid:
	{
		static const std::string r("invalid");
		return r;
	}

	default:
	{
		static const std::string r("?");
		return r;
	}
	}
}

const std::string& to_str(ADR::AUPRSA::Activation::status_t s)
{
	switch (s) {
	case ADR::AUPRSA::Activation::status_invalid:
	{
		static const std::string r("invalid");
		return r;
	}

	case ADR::AUPRSA::Activation::status_active:
	{
		static const std::string r("active");
		return r;
	}

	default:
	{
		static const std::string r("?""?");
		return r;
	}
	}
}

ADRAUPObjectBase::ADRAUPObjectBase(const Link& obj, timetype_t starttime, timetype_t endtime)
	: m_obj(obj), m_time(starttime, endtime), m_refcount(1)
{
}

ADRAUPObjectBase::~ADRAUPObjectBase()
{
}

void ADRAUPObjectBase::reference(void) const
{
	g_atomic_int_inc(&m_refcount);
}

void ADRAUPObjectBase::unreference(void) const
{
	if (!g_atomic_int_dec_and_test(&m_refcount))
		return;
	delete this;
}

ADRAUPObjectBase::ptr_t ADRAUPObjectBase::clone_obj(void) const
{
	ptr_t p(new ADRAUPObjectBase(get_obj(), get_starttime(), get_endtime()));
	return p;
}

bool ADRAUPObjectBase::is_overlap(uint64_t tm0, uint64_t tm1) const
{
	if (!is_valid() || !(tm0 < tm1))
		return false;
	return tm1 > get_starttime() && tm0 < get_endtime();
}

uint64_t ADRAUPObjectBase::get_overlap(uint64_t tm0, uint64_t tm1) const
{
	if (!is_valid() || !(tm0 < tm1))
		return 0;
	if (tm0 > get_starttime()) {
		if (tm0 >= get_endtime())
			return 0;
		return std::min(get_endtime(), tm1) - tm0;
	}
	if (get_starttime() >= tm1)
		return 0;
	return std::min(get_endtime(), tm1) - get_starttime();
}

void ADRAUPObjectBase::link(Database& db, unsigned int depth)
{
	LinkLoader ll(db, depth);
	hibernate(ll);
}

void ADRAUPObjectBase::dependencies(UUID::set_t& dep) const
{
	DependencyGenerator<UUID::set_t> gen(dep);
	const_cast<ADRAUPObjectBase *>(this)->hibernate(gen);	
}

void ADRAUPObjectBase::dependencies(Link::LinkSet& dep) const
{
	DependencyGenerator<Link::LinkSet> gen(dep);
	const_cast<ADRAUPObjectBase *>(this)->hibernate(gen);
}

bool ADRAUPObjectBase::is_unlinked(void) const
{
	Link::LinkSet dep;
	dependencies(dep);
	return dep.has_unlinked();
}

timeset_t ADRAUPObjectBase::timediscontinuities(void) const
{
	timeset_t r;
	if (!is_valid())
		return r;
	r.insert(get_starttime());
	r.insert(get_endtime());
	Link::LinkSet dep;
	dependencies(dep);
	for (Link::LinkSet::const_iterator di(dep.begin()), de(dep.end()); di != de; ++di) {
		if (!di->get_obj())
			continue;
		timeset_t r1(di->get_obj()->timediscontinuities());
		r.insert(r1.begin(), r1.end());
	}
	while (!r.empty() && *r.begin() < get_starttime())
		r.erase(r.begin());
	while (!r.empty()) {
		timeset_t::iterator i(r.end());
		--i;
		if (*i <= get_endtime())
			break;
		r.erase(i);
	}
	return r;
}

int ADRAUPObjectBase::compare(const ADRAUPObjectBase& x) const
{
	{
		int c(get_obj().compare(x.get_obj()));
		if (c)
			return c;
	}
	if (get_starttime() < x.get_starttime())
		return -1;
	if (x.get_starttime() < get_starttime())
		return 1;
	if (get_endtime() < x.get_endtime())
		return -1;
	if (x.get_endtime() < get_endtime())
		return 1;
	return 0;
}

ADRAUPObjectBase::ptr_t ADRAUPObjectBase::create(type_t t, const Link& obj, timetype_t starttime, timetype_t endtime)
{
	switch (t) {
	case type_invalid:
	{
		ptr_t p(new ADRAUPObjectBase(obj, starttime, endtime));
		return p;
	}

	case type_rsa:
	{
		ptr_t p(new AUPRSA(obj, starttime, endtime));
		return p;
	}

	case type_cdr:
	{
		ptr_t p(new AUPCDR(obj, starttime, endtime));
		return p;
	}

	default:
	{
		std::ostringstream oss;
		oss << "ADRAUPObjectBase::create: invalid type " << to_str(t) << " (" << (unsigned int)t << ')';
		throw std::runtime_error(oss.str());
	}
	}
	return ptr_t();
}

ADRAUPObjectBase::ptr_t ADRAUPObjectBase::create(ArchiveReadStream& ar, const Link& obj, timetype_t starttime, timetype_t endtime)
{
	type_t t;
	ar.iouint8(t);
	ptr_t p(create(t, obj, starttime, endtime));
	p->load(ar);
	return p;
}

ADRAUPObjectBase::ptr_t ADRAUPObjectBase::create(ArchiveReadBuffer& ar, const Link& obj, timetype_t starttime, timetype_t endtime)
{
	type_t t;
	ar.iouint8(t);
	ptr_t p(create(t, obj, starttime, endtime));
	p->load(ar);
	return p;
}

void ADRAUPObjectBase::load(ArchiveReadStream& ar)
{
	hibernate(ar);
}

void ADRAUPObjectBase::load(ArchiveReadBuffer& ar)
{
	hibernate(ar);
}

void ADRAUPObjectBase::save(ArchiveWriteStream& ar) const
{
	type_t t(get_type());
	ar.iouint8(t);
	(const_cast<ADRAUPObjectBase *>(this))->hibernate(ar);	
}

std::ostream& ADRAUPObjectBase::print(std::ostream& os) const
{
	os << get_obj() << ' ';
	if (get_starttime() == std::numeric_limits<uint64_t>::max()) {
		os << "unlimited           ";
	} else {
		Glib::TimeVal tv(get_starttime(), 0);
		os << tv.as_iso8601();
	}
	if (get_endtime() == std::numeric_limits<uint64_t>::max()) {
		os << " unlimited           ";
	} else {
		Glib::TimeVal tv(get_endtime(), 0);
		os << ' ' << tv.as_iso8601();
	}
	return os << ' ' << get_type();
}

std::ostream& AUPRSA::Activation::print(std::ostream& os, const timepair_t& tp) const
{
	os << " altitude " << m_altrange.to_str() << ' ' << get_status();
	for (unsigned int i(0), n(m_hostairspaces.size()); i < n; ++i) {
		const Link& l(m_hostairspaces[i]);
		if (l.is_nil())
			continue;
		os << std::endl << "      host airspace " << l;
		const AirspaceTimeSlice& ts(l.get_obj()->operator()(tp.first).as_airspace());
		if (!ts.is_valid())
			continue;
		os << ' ' << ts.get_ident() << ' ' << ts.get_type();
	}
	return os;
}

AUPRSA::AUPRSA(const Link& obj, timetype_t starttime, timetype_t endtime)
	: ADRAUPObjectBase(obj, starttime, endtime)
{
}

AUPRSA::ptr_t AUPRSA::clone_obj(void) const
{
	ptr_t p(new AUPRSA(get_obj(), get_starttime(), get_endtime()));
	return p;
}

void AUPRSA::load(ArchiveReadStream& ar)
{
	hibernate(ar);
}

void AUPRSA::load(ArchiveReadBuffer& ar)
{
	hibernate(ar);
}

void AUPRSA::save(ArchiveWriteStream& ar) const
{
	type_t t(get_type());
	ar.iouint8(t);
	(const_cast<AUPRSA *>(this))->hibernate(ar);	
}

std::ostream& AUPRSA::print(std::ostream& os) const
{
	ADRAUPObjectBase::print(os);
	os << (is_icao() ? " icao" : "") << (is_level(1) ? " level1" : "")
	   << (is_level(2) ? " level2" : "") << (is_level(3) ? " level3" : "");
	m_activation.print(os, get_timeinterval());
	return os << std::endl;
}

std::ostream& AUPCDR::Availability::print(std::ostream& os, const timepair_t& tp) const
{
	os << " altitude " << m_altrange.to_str();
	if (get_cdr())
		os << " cdr" << get_cdr();
	if (is_forward())
		os << " forward";
	if (is_backward())
		os << " backward";
	for (unsigned int i(0), n(m_hostairspaces.size()); i < n; ++i) {
		const Link& l(m_hostairspaces[i]);
		if (l.is_nil())
			continue;
		os << std::endl << "      host airspace " << l;
		const AirspaceTimeSlice& ts(l.get_obj()->operator()(tp.first).as_airspace());
		if (!ts.is_valid())
			continue;
		os << ' ' << ts.get_ident() << ' ' << ts.get_type();
	}
	return os;
}

AUPCDR::AUPCDR(const Link& obj, timetype_t starttime, timetype_t endtime)
	: ADRAUPObjectBase(obj, starttime, endtime)
{
}

AUPCDR::ptr_t AUPCDR::clone_obj(void) const
{
	ptr_t p(new AUPCDR(get_obj(), get_starttime(), get_endtime()));
	return p;
}

void AUPCDR::load(ArchiveReadStream& ar)
{
	hibernate(ar);
}

void AUPCDR::load(ArchiveReadBuffer& ar)
{
	hibernate(ar);
}

void AUPCDR::save(ArchiveWriteStream& ar) const
{
	type_t t(get_type());
	ar.iouint8(t);
	(const_cast<AUPCDR *>(this))->hibernate(ar);	
}

void AUPCDR::add_availability(const Availability& a)
{
	for (availability_t::iterator i(m_availability.begin()), e(m_availability.end()); i != e; ++i) {
		if (i->get_altrange() != a.get_altrange() || i->is_forward() != a.is_forward())
			continue;
		i->set_hostairspaces(a.get_hostairspaces());
		i->set_cdr(a.get_cdr());
		return;
	}
	m_availability.push_back(a);
}

void AUPCDR::add_availability(const availability_t& a)
{
	for (availability_t::const_iterator i(a.begin()), e(a.end()); i != e; ++i)
		add_availability(*i);
}

std::ostream& AUPCDR::print(std::ostream& os) const
{
	ADRAUPObjectBase::print(os);
	for (unsigned int i(0), n(m_availability.size()); i < n; ++i)
		m_availability[i].print(os << std::endl << "    Availability[" << i << ']', get_timeinterval());
	return os << std::endl;
}

AUPDatabase::AUPDatabase(const std::string& path)
	: m_path(path)
{
	if (m_path.empty())
		m_path = PACKAGE_DATA_DIR;
}

AUPDatabase::~AUPDatabase(void)
{
	close();
}

void AUPDatabase::set_path(const std::string& path)
{
	if (path == m_path)
		return;
	close();
	m_path = path;
}

void AUPDatabase::save_helper(const ADRAUPObjectBase::const_ptr_t& p)
{
	if (!p)
		return;
	std::ostringstream blob;
	{
		ArchiveWriteStream ar(blob);
		p->save(ar);
	}
	sqlite3x::sqlite3_command cmd(m_db, "INSERT OR REPLACE INTO aup"
				      " (UUID0,UUID1,UUID2,UUID3,TYPE,STARTTIME,ENDTIME,DATA)"
				      " VALUES (?,?,?,?,?,?,?,?);");
	for (unsigned int i = 0; i < 4; ++i)
		cmd.bind(i + 1, (long long int)p->get_obj().get_word(i));
	cmd.bind(5, (int)p->get_type());
	cmd.bind(6, (long long int)std::min(p->get_starttime(), (timetype_t)std::numeric_limits<long long int>::max()));
	cmd.bind(7, (long long int)std::min(p->get_endtime(), (timetype_t)std::numeric_limits<long long int>::max()));
	cmd.bind(8, blob.str().c_str(), blob.str().size());
	cmd.executenonquery();
}

void AUPDatabase::save(const ADRAUPObjectBase::const_ptr_t& p)
{
	if (!p)
		return;
	open();
	sqlite3x::sqlite3_transaction tran(m_db);
	{
		findresults_t r(find(p->get_obj(), p->get_starttime(), p->get_endtime()));
		for (findresults_t::iterator ri(r.begin()), re(r.end()); ri != re;) {
			ADRAUPObjectBase::ptr_t p1(ADRAUPObjectBase::ptr_t::cast_const(*ri));
			if (!p1 || p->get_obj() != p1->get_obj() || !p->is_overlap(p1) || p->get_type() != p1->get_type()) {
				ri = r.erase(ri);
				re = r.end();
				continue;
			}
			erase(p1);
			++ri;
		}
		AUPCDR::const_ptr_t pc(AUPCDR::const_ptr_t::cast_dynamic(p));
		if (pc) {
			timeset_t tdisc;
			for (findresults_t::const_iterator ri(r.begin()), re(r.end()); ri != re; ++ri) {
				ADRAUPObjectBase::ptr_t p1(ADRAUPObjectBase::ptr_t::cast_const(*ri));
				if (!p1)
					continue;
				tdisc.insert(p1->get_starttime());
				tdisc.insert(p1->get_endtime());
			}
			for (timeset_t::const_iterator ti(tdisc.begin()), te(tdisc.end()); ti != te;) {
				timeset_t::const_iterator tis(ti);
				++ti;
				if (ti == te)
					break;
				AUPCDR::ptr_t pcn(new AUPCDR(pc->get_obj(), *tis, *ti));
				if (pcn->is_overlap(pc))
					pcn->add_availability(pc->get_availability());
				for (findresults_t::const_iterator ri(r.begin()), re(r.end()); ri != re; ++ri) {
					AUPCDR::ptr_t p1(AUPCDR::ptr_t::cast_dynamic(ADRAUPObjectBase::ptr_t::cast_const(*ri)));
					if (!p1 || !pcn->is_overlap(p1))
						continue;
					pcn->add_availability(p1->get_availability());
				}
				if (pcn->is_valid() && !pcn->get_availability().empty())
					save_helper(pcn);
			}
		} else {
			for (findresults_t::const_iterator ri(r.begin()), re(r.end()); ri != re; ++ri) {
				ADRAUPObjectBase::ptr_t p1(ADRAUPObjectBase::ptr_t::cast_const(*ri));
				if (p1->get_starttime() < p->get_starttime())
					p1->set_endtime(p->get_starttime());
				else
					p1->set_starttime(p->get_endtime());
				if (p1->is_valid())
					save_helper(p1);
			}
		}
	}
	save_helper(p);
	tran.commit();
}

void AUPDatabase::erase(const ADRAUPObjectBase::const_ptr_t& p)
{
	if (!p)
		return;
	open();
	{
		sqlite3x::sqlite3_command cmd(m_db, "DELETE FROM aup WHERE UUID0=? AND UUID1=? AND UUID2=? AND UUID3=? AND STARTTIME=? AND ENDTIME=?;");
		for (unsigned int i = 0; i < 4; ++i)
			cmd.bind(i + 1, (long long int)p->get_obj().get_word(i));
		cmd.bind(5, (long long int)std::min(p->get_starttime(), (timetype_t)std::numeric_limits<long long int>::max()));
		cmd.bind(6, (long long int)std::min(p->get_endtime(), (timetype_t)std::numeric_limits<long long int>::max()));
		cmd.executenonquery();
	}
}

void AUPDatabase::erase(timetype_t tmin, timetype_t tmax)
{
	open();
	{
		sqlite3x::sqlite3_command cmd(m_db, "DELETE FROM aup WHERE ENDTIME>? AND STARTTIME<?;");
		cmd.bind(1, (long long int)std::min(tmin, (timetype_t)std::numeric_limits<long long int>::max()));
		cmd.bind(2, (long long int)std::min(tmax, (timetype_t)std::numeric_limits<long long int>::max()));
		cmd.executenonquery();
	}
}

void AUPDatabase::open(void)
{
	if (m_db.db())
		return;
	m_db.open(Glib::build_filename(m_path, "aup.db"));
	{
		sqlite3x::sqlite3_command cmd(m_db, "CREATE TABLE IF NOT EXISTS aup (UUID0 INTEGER NOT NULL, "
					      "UUID1 INTEGER NOT NULL,"
					      "UUID2 INTEGER NOT NULL,"
					      "UUID3 INTEGER NOT NULL,"
					      "TYPE INTEGER NOT NULL,"
					      "STARTTIME INTEGER NOT NULL,"
					      "ENDTIME INTEGER NOT NULL,"
					      "DATA BLOB NOT NULL,"
					      "UNIQUE (UUID0,UUID1,UUID2,UUID3,STARTTIME,ENDTIME) ON CONFLICT REPLACE);");
		cmd.executenonquery();
	}
	{
		sqlite3x::sqlite3_command cmd(m_db, "CREATE INDEX IF NOT EXISTS aup_uuid ON aup(UUID0,UUID1,UUID2,UUID3,STARTTIME,ENDTIME);");
		cmd.executenonquery();
	}
}

void AUPDatabase::close(void)
{
	if (!m_db.db())
		return;
	m_db.close();
}

void AUPDatabase::sync_off(void)
{
	open();
	{
		sqlite3x::sqlite3_command cmd(m_db, "PRAGMA synchronous = OFF;");
		cmd.executenonquery();
	}
}

void AUPDatabase::analyze(void)
{
	open();
	{
		sqlite3x::sqlite3_command cmd(m_db, "ANALYZE;");
		cmd.executenonquery();
	}
}

void AUPDatabase::vacuum(void)
{
	open();
	{
		sqlite3x::sqlite3_command cmd(m_db, "VACUUM;");
		cmd.executenonquery();
	}
}

void AUPDatabase::set_wal(bool wal)
{
	open();
	{
		sqlite3x::sqlite3_command cmd(m_db, wal ? "PRAGMA journal_mode=WAL;" : "PRAGMA journal_mode=DELETE;");
		cmd.executenonquery();
	}
}

void AUPDatabase::set_exclusive(bool excl)
{
	open();
	{
		sqlite3x::sqlite3_command cmd(m_db, excl ? "PRAGMA locking_mode=EXCLUSIVE;" :
					      "PRAGMA locking_mode=NORMAL;");
		cmd.executenonquery();
	}
}

AUPDatabase::findresults_t AUPDatabase::find_tail(sqlite3x::sqlite3_command& cmd)
{
	findresults_t r;
	sqlite3x::sqlite3_cursor cursor(cmd.executecursor());
	while (cursor.step()) {
		if (cursor.isnull(6))
			continue;
		int sz;
		uint8_t const *data((uint8_t const *)cursor.getblob(6, sz));
		if (sz < 1)
			continue;
		ADRAUPObjectBase::ptr_t p;
		ArchiveReadBuffer ar(data, data + sz);
		try {
			p = ADRAUPObjectBase::create(ar, UUID(cursor.getint(0), cursor.getint(1),
							      cursor.getint(2), cursor.getint(3)),
						     cursor.getint64(4), cursor.getint64(5));
		} catch (const std::exception& e) {
			if (false)
				throw;
			std::ostringstream oss;
			oss << e.what() << "; blob" << std::hex;
			for (const uint8_t *p0(data), *p1(data + sz); p0 != p1; ++p0)
				oss << ' ' << std::setfill('0') << std::setw(2) << (unsigned int)*p0;
			throw std::runtime_error(oss.str());
		}
		r.push_back(p);
	}
	return r;
}

AUPDatabase::findresults_t AUPDatabase::find(const UUID& uuid, timetype_t tmin, timetype_t tmax)
{
	open();
	sqlite3x::sqlite3_command cmd(m_db, "SELECT UUID0,UUID1,UUID2,UUID3,STARTTIME,ENDTIME,DATA FROM"
				      " aup WHERE UUID0=? AND UUID1=? AND UUID2=? AND UUID3=? AND ENDTIME>? AND STARTTIME<?"
				      " ORDER BY STARTTIME,ENDTIME;");
	for (unsigned int i = 0; i < 4; ++i)
		cmd.bind(i + 1, (long long int)uuid.get_word(i));
	cmd.bind(5, (long long int)std::min(tmin, (timetype_t)std::numeric_limits<long long int>::max()));
	cmd.bind(6, (long long int)std::min(tmax, (timetype_t)std::numeric_limits<long long int>::max()));
	return find_tail(cmd);
}

AUPDatabase::findresults_t AUPDatabase::find(timetype_t tmin, timetype_t tmax)
{
	open();
	sqlite3x::sqlite3_command cmd(m_db, "SELECT UUID0,UUID1,UUID2,UUID3,STARTTIME,ENDTIME,DATA FROM"
				      " aup WHERE ENDTIME>? AND STARTTIME<?"
				      " ORDER BY UUID0,UUID1,UUID2,UUID3,STARTTIME,ENDTIME;");
	cmd.bind(1, (long long int)std::min(tmin, (timetype_t)std::numeric_limits<long long int>::max()));
	cmd.bind(2, (long long int)std::min(tmax, (timetype_t)std::numeric_limits<long long int>::max()));
	return find_tail(cmd);
}

void ConditionalAvailability::clear(void)
{
	m_objects.clear();
}

void ConditionalAvailability::load(Database& db, AUPDatabase& aupdb, timetype_t starttime, timetype_t endtime)
{
	AUPDatabase::findresults_t r(aupdb.find(starttime, endtime));
	for (AUPDatabase::findresults_t::const_iterator ri(r.begin()), re(r.end()); ri != re; ++ri) {
		if (!*ri)
			continue;
		ADRAUPObjectBase::ptr_t::cast_const(*ri)->link(db);
		m_objects.insert(*ri);
	}
	if (false) {
		std::cerr << "AUP Database: " << Glib::TimeVal(starttime, 0).as_iso8601() << ".."
			  << Glib::TimeVal(endtime, 0).as_iso8601() << std::endl;
		for (objects_t::const_iterator oi(m_objects.begin()), oe(m_objects.end()); oi != oe; ++oi)
			std::cerr << "  " << (*oi)->get_obj() << ' ' << Glib::TimeVal((*oi)->get_starttime(), 0).as_iso8601()
				  << ".." << Glib::TimeVal((*oi)->get_endtime(), 0).as_iso8601() << std::endl;
	}
}

const ADRAUPObjectBase::const_ptr_t& ConditionalAvailability::find(const UUID& uuid, timetype_t tm) const
{
	static ADRAUPObjectBase::const_ptr_t nullaupptr;
	ADRAUPObjectBase::const_ptr_t ps(new ADRAUPObjectBase(uuid, 0, 0));
	for (objects_t::const_iterator i(m_objects.lower_bound(ps)); i != m_objects.end(); ++i) {
		if (!*i)
			continue;
		{
			int c((*i)->get_obj().compare(uuid));
			if (false)
				std::cerr << "CondAvail:find: " << uuid << " obj " << (*i)->get_obj() << " res " << c << std::endl;
			if (c > 0)
				break;
			if (c < 0)
				continue;
		}
		if ((*i)->is_inside(tm))
			return *i;
	}
	return nullaupptr;
}

ConditionalAvailability::results_t ConditionalAvailability::find(const UUID& uuid, timetype_t tstart, timetype_t tend) const
{
	results_t r;
	ADRAUPObjectBase::const_ptr_t ps(new ADRAUPObjectBase(uuid, 0, 0));
	for (objects_t::const_iterator i(m_objects.lower_bound(ps)); i != m_objects.end(); ++i) {
		if (!*i)
			continue;
		{
			int c((*i)->get_obj().compare(uuid));
			if (false)
				std::cerr << "CondAvail:find: " << uuid << " obj " << (*i)->get_obj() << " res " << c << std::endl;
			if (c > 0)
				break;
			if (c < 0)
				continue;
		}
		if ((*i)->is_overlap(tstart, tend))
			r.push_back(*i);
	}
	return r;
}
