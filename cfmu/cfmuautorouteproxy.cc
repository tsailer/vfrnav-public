#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <iostream>
#include <iomanip>
#include <limits>

#include "cfmuautorouteproxy.hh"

const Glib::ustring& to_str(CFMUAutorouteProxy::proxystate_t ps)
{
	switch (ps) {
	case CFMUAutorouteProxy::proxystate_stopped:
	{
		static const Glib::ustring r("stopped");
		return r;
	}

	case CFMUAutorouteProxy::proxystate_started:
	{
		static const Glib::ustring r("started");
		return r;
	}

	case CFMUAutorouteProxy::proxystate_error:
	{
		static const Glib::ustring r("error");
		return r;
	}

	default:
	{
		static const Glib::ustring r("?""?");
		return r;
	}
	}
}

const std::string CFMUAutorouteProxy::Command::empty_string("");

template void CFMUAutorouteProxy::Command::set_option(const std::string& name, unsigned short value, bool replace = false);
template void CFMUAutorouteProxy::Command::set_option(const std::string& name, short value, bool replace = false);
template void CFMUAutorouteProxy::Command::set_option(const std::string& name, unsigned int value, bool replace = false);
template void CFMUAutorouteProxy::Command::set_option(const std::string& name, int value, bool replace = false);
template void CFMUAutorouteProxy::Command::set_option(const std::string& name, unsigned long value, bool replace = false);
template void CFMUAutorouteProxy::Command::set_option(const std::string& name, long value, bool replace = false);
template void CFMUAutorouteProxy::Command::set_option(const std::string& name, unsigned long long value, bool replace = false);
template void CFMUAutorouteProxy::Command::set_option(const std::string& name, long long value, bool replace = false);
template void CFMUAutorouteProxy::Command::set_option(const std::string& name, std::set<std::string>::const_iterator b, std::set<std::string>::const_iterator e, bool replace = false);

CFMUAutorouteProxy::Command::Command(void)
{
}

CFMUAutorouteProxy::Command::Command(const std::string& cmd)
{
	std::string::size_type pos(cmd.find(' '));
	m_cmdname = std::string(cmd, 0, pos);
	while (pos < cmd.size()) {
		std::string::size_type pos1(cmd.find_first_not_of(' ', pos));
		pos = pos1;
		if (pos >= cmd.size())
			break;
		pos1 = cmd.find_first_of("= ", pos);
		if (pos1 == std::string::npos) {
			set_option(std::string(cmd, pos), "");
			pos = pos1;
			break;
		}
		std::string name(cmd, pos, pos1 - pos);
		pos = pos1 + 1;
		if (cmd[pos1] == ' ' || pos >= cmd.size()) {
			set_option(name, "");
			continue;
		}
		std::string val;
		if (!unquote_str(val, cmd, pos))
			error("invalid string escape in option " + name);
		set_option(name, val);
	}
}

bool CFMUAutorouteProxy::Command::is_str_needs_quotes(const std::string& s, bool nocomma)
{
	for (std::string::const_iterator si(s.begin()), se(s.end()); si != se; ++si)
		if (*si <= ' ' || *si >= 0x7f ||
		    *si == '\\' || *si == '\"' ||
		    (nocomma && *si == ','))
			return true;
	return false;
}

std::string CFMUAutorouteProxy::Command::quote_str(const std::string& s)
{
	std::string val("\"");
	for (std::string::const_iterator vi(s.begin()), ve(s.end()); vi != ve; ++vi) {
		bool asciirange(*vi >= ' ' && *vi < 0x7f);
		if (*vi == '\\' || *vi == '"' || !asciirange)
			val.push_back('\\');
		if (asciirange) {
			val.push_back(*vi);
			continue;
		}
		val.push_back('0' + ((*vi >> 6) & 3));
		val.push_back('0' + ((*vi >> 3) & 7));
		val.push_back('0' + (*vi & 7));
	}
	val.push_back('"');
	return val;
}

bool CFMUAutorouteProxy::Command::unquote_str(std::string& res, const std::string& s, std::string::size_type& pos, bool nocomma)
{
	res.clear();
	if (pos >= s.size())
		return false;
	std::string::size_type pos1;
	if (s[pos] != '"') {
		if (nocomma)
			pos1 = s.find_first_of(", ", pos);
		else
			pos1 = s.find(' ', pos);
		if (pos1 == std::string::npos) {
			res = std::string(s, pos);
			pos = pos1;
			return true;
		}
		res = std::string(s, pos, pos1 - pos);
		pos = pos1;
		return true;
	}
	++pos;
	while (pos < s.size() && s[pos] != '"') {
		if (s[pos] != '\\') {
			res.push_back(s[pos++]);
			continue;
		}
		++pos;
		if (pos >= s.size())
			break;
		if (s[pos] < '0' || s[pos] > '9') {
			res.push_back(s[pos++]);
			continue;
		}
		if (pos + 2 >= s.size())
			break;
		res.push_back(((s[pos] & 3) << 6) | ((s[pos+1] & 7) << 3) | (s[pos+2] & 7));
		pos += 3;
	}
	if (pos < s.size() && s[pos] == '"') {
		++pos;
		return true;
	}
	return false;
}

std::string CFMUAutorouteProxy::Command::get_cmdstring(void) const
{
	std::string cs(get_cmdname());
	if (cs.empty())
		cs = "autoroute";
	for (opts_t::const_iterator oi(m_opts.begin()), oe(m_opts.end()); oi != oe; ++oi) {
		cs += " " + oi->first + "=";
		if (is_str_needs_quotes(oi->second)) {
			cs += quote_str(oi->second);
			continue;
		}
		cs += oi->second;
	}
	return cs;
}

void CFMUAutorouteProxy::Command::unset_option(const std::string& name, bool require)
{
	opts_t::iterator oi(m_opts.find(name));
	if (oi != m_opts.end()) {
		m_opts.erase(oi);
		return;
	}
	if (!require)
		return;
	error("option " + name + " not found");
}

void CFMUAutorouteProxy::Command::set_option(const std::string& name, const std::string& value, bool replace)
{
	std::pair<opts_t::iterator,bool> ins(m_opts.insert(opts_t::value_type(name, value)));
	if (ins.second)
		return;
	if (replace) {
		ins.first->second = value;
		return;
	}
	error("option " + name + " already set");
}

void CFMUAutorouteProxy::Command::set_option(const std::string& name, const Glib::ustring& value, bool replace)
{
	set_option(name, static_cast<const std::string&>(value), replace);
}

void CFMUAutorouteProxy::Command::set_option(const std::string& name, const bytearray_t& value, bool replace)
{
	bool subseq(false);
	std::ostringstream oss;
	for (bytearray_t::const_iterator bi(value.begin()), be(value.end()); bi != be; ++bi) {
		if (subseq)
			oss << ',';
		subseq = true;
		oss << (unsigned int)*bi;
	}
	set_option(name, oss.str(), replace);
}

template<> void CFMUAutorouteProxy::Command::set_option<float>(const std::string& name, float value, bool replace)
{
	std::ostringstream oss;
	if (std::isnan(value))
		oss << "NaN";
	else
		oss << value;
	set_option(name, oss.str(), replace);
}

template<> void CFMUAutorouteProxy::Command::set_option<double>(const std::string& name, double value, bool replace)
{
	std::ostringstream oss;
	if (std::isnan(value))
		oss << "NaN";
	else
		oss << value;
	set_option(name, oss.str(), replace);
}

template<typename T> void CFMUAutorouteProxy::Command::set_option(const std::string& name, T value, bool replace)
{
	std::ostringstream oss;
	oss << value;
	set_option(name, oss.str(), replace);
}

template<typename T> void CFMUAutorouteProxy::Command::set_option(const std::string& name, T b, T e, bool replace)
{
	bool subseq(false);
	std::string val;
	for (; b != e; ++b) {
		if (subseq)
			val.push_back(',');
		subseq = true;
		std::ostringstream oss;
		oss << *b;
		const std::string& val1(oss.str());
		if (is_str_needs_quotes(val1, true))
			val += quote_str(val1);
		else
			val += val1;
	}
	set_option(name, val, replace);
}

std::pair<const std::string&,bool> CFMUAutorouteProxy::Command::get_option(const std::string& name) const
{
	opts_t::const_iterator i(m_opts.find(name));
	if (i == m_opts.end())
		return std::pair<const std::string&,bool>(empty_string, false);
	return std::pair<const std::string&,bool>(i->second, true);
}

std::pair<long,bool> CFMUAutorouteProxy::Command::get_option_int(const std::string& name) const
{
	std::pair<const std::string&,bool> o(get_option(name));
	std::pair<int,bool> ret(0, o.second);
	if (!ret.second)
		return ret;
	const char *cp(o.first.c_str());
	char *cpe;
	ret.first = strtol(cp, &cpe, 0);
	if ((size_t)(cpe - cp) >= o.first.length())
		return ret;
	error("option " + name + " is not an integer");
	return std::pair<int,bool>(0, false);
}

std::pair<unsigned long,bool> CFMUAutorouteProxy::Command::get_option_uint(const std::string& name) const
{
	std::pair<const std::string&,bool> o(get_option(name));
	std::pair<int,bool> ret(0, o.second);
	if (!ret.second)
		return ret;
	const char *cp(o.first.c_str());
	char *cpe;
	ret.first = strtoul(cp, &cpe, 0);
	if ((size_t)(cpe - cp) >= o.first.length())
		return ret;
	error("option " + name + " is not an integer");
	return std::pair<int,bool>(0, false);
}

std::pair<long long,bool> CFMUAutorouteProxy::Command::get_option_long(const std::string& name) const
{
	std::pair<const std::string&,bool> o(get_option(name));
	std::pair<int,bool> ret(0, o.second);
	if (!ret.second)
		return ret;
	const char *cp(o.first.c_str());
	char *cpe;
	ret.first = strtoll(cp, &cpe, 0);
	if ((size_t)(cpe - cp) >= o.first.length())
		return ret;
	error("option " + name + " is not an integer");
	return std::pair<int,bool>(0, false);
}

std::pair<unsigned long long,bool> CFMUAutorouteProxy::Command::get_option_ulong(const std::string& name) const
{
	std::pair<const std::string&,bool> o(get_option(name));
	std::pair<unsigned long long,bool> ret(0, o.second);
	if (!ret.second)
		return ret;
	const char *cp(o.first.c_str());
	char *cpe;
	ret.first = strtoull(cp, &cpe, 0);
	if ((size_t)(cpe - cp) >= o.first.length())
		return ret;
	error("option " + name + " is not an integer");
	return std::pair<unsigned long long,bool>(0, false);
}

std::pair<double,bool> CFMUAutorouteProxy::Command::get_option_float(const std::string& name) const
{
	std::pair<const std::string&,bool> o(get_option(name));
	std::pair<double,bool> ret(0, o.second);
	if (!ret.second)
		return ret;
	if (o.first == "NaN") {
		ret.first = std::numeric_limits<double>::quiet_NaN();
		return ret;
	}
	const char *cp(o.first.c_str());
	char *cpe;
	ret.first = strtod(cp, &cpe);
	if ((size_t)(cpe - cp) >= o.first.length())
		return ret;
	error("option " + name + " is not a float");
	return std::pair<double,bool>(0, false);
}

std::pair<CFMUAutorouteProxy::Command::bytearray_t,bool> CFMUAutorouteProxy::Command::get_option_bytearray(const std::string& name) const
{
	std::pair<const std::string&,bool> o(get_option(name));
	std::pair<bytearray_t,bool> ret(bytearray_t(), o.second);
	if (!ret.second)
		return ret;
	const char *cp(o.first.c_str()), *cpb(cp);
	char *cpe;
	while (*cp) {
		ret.first.push_back(strtoul(cp, &cpe, 0));
		cp = cpe;
		if (*cp != ',')
			break;
		++cp;
	}
	if ((size_t)(cp - cpb) >= o.first.length())
		return ret;
	error("option " + name + " is not a byte array");
	return std::pair<bytearray_t,bool>(bytearray_t(), false);
}

std::pair<CFMUAutorouteProxy::Command::intarray_t,bool> CFMUAutorouteProxy::Command::get_option_intarray(const std::string& name) const
{
	std::pair<const std::string&,bool> o(get_option(name));
	std::pair<intarray_t,bool> ret(intarray_t(), o.second);
	if (!ret.second)
		return ret;
	const char *cp(o.first.c_str()), *cpb(cp);
	char *cpe;
	while (*cp) {
		ret.first.push_back(strtol(cp, &cpe, 0));
		cp = cpe;
		if (*cp != ',')
			break;
		++cp;
	}
	if ((size_t)(cp - cpb) >= o.first.length())
		return ret;
	error("option " + name + " is not an int array");
	return std::pair<intarray_t,bool>(intarray_t(), false);
}

std::pair<CFMUAutorouteProxy::Command::ulongarray_t,bool> CFMUAutorouteProxy::Command::get_option_ulongarray(const std::string& name) const
{
	std::pair<const std::string&,bool> o(get_option(name));
	std::pair<ulongarray_t,bool> ret(ulongarray_t(), o.second);
	if (!ret.second)
		return ret;
	const char *cp(o.first.c_str()), *cpb(cp);
	char *cpe;
	while (*cp) {
		ret.first.push_back(strtoull(cp, &cpe, 0));
		cp = cpe;
		if (*cp != ',')
			break;
		++cp;
	}
	if ((size_t)(cp - cpb) >= o.first.length())
		return ret;
	error("option " + name + " is not an ulong array");
	return std::pair<ulongarray_t,bool>(ulongarray_t(), false);
}

std::pair<CFMUAutorouteProxy::Command::stringarray_t,bool> CFMUAutorouteProxy::Command::get_option_stringarray(const std::string& name) const
{
	std::pair<const std::string&,bool> o(get_option(name));
	std::pair<stringarray_t,bool> ret(stringarray_t(), o.second);
	if (!ret.second)
		return ret;
	std::string::size_type pos(0);
	std::string res;
	while (unquote_str(res, o.first, pos, true))
		ret.first.push_back(res);
	return ret;
}

void CFMUAutorouteProxy::Command::set_option(const std::string& name, proxystate_t pss, bool replace)
{
	set_option(name, ::to_str(pss), replace);
}

std::pair<CFMUAutorouteProxy::proxystate_t,bool> CFMUAutorouteProxy::Command::get_option_proxystate(const std::string& name) const
{
        std::pair<const std::string&,bool> o(get_option(name));
        std::pair<proxystate_t,bool> ret(proxystate_error, o.second);
        if (!ret.second)
                return ret;
        static const proxystate_t pss[] = {
		CFMUAutorouteProxy::proxystate_stopped,
		CFMUAutorouteProxy::proxystate_started,
		CFMUAutorouteProxy::proxystate_error
        };
        unsigned int i;
        for (i = 0; i < sizeof(pss)/sizeof(pss[0]); ++i)
                if (::to_str(pss[i]) == o.first)
                        break;
        if (i >= sizeof(pss)/sizeof(pss[0])) {
                error("invalid " + name + "=" + o.first);
                ret.second = false;
        } else {
                ret.first = pss[i];
        }
        return ret;
}

void CFMUAutorouteProxy::Command::set_option(const std::string& name, const Point& value, bool replace)
{
	std::ostringstream oss;
	oss << value.get_lat() << ',' << value.get_lon();
	set_option(name, oss.str(), replace);
}

std::pair<Point,bool> CFMUAutorouteProxy::Command::get_option_coord(const std::string& name) const
{
	std::pair<intarray_t,bool> o(get_option_intarray(name));
	std::pair<Point,bool> ret(Point::invalid, o.second);
	ret.first.set_invalid();
        if (!ret.second || o.first.size() != 2)
                return ret;
	ret.first.set_lat(o.first[0]);
	ret.first.set_lon(o.first[1]);
	ret.second = true;
	return ret;
}

void CFMUAutorouteProxy::Command::error(const std::string& text)
{
	throw std::runtime_error(text);
}

CFMUAutorouteProxy::CFMUAutorouteProxy()
	: m_route(*(FPlan *)0), m_childrun(false)
{
}

CFMUAutorouteProxy::~CFMUAutorouteProxy()
{
	child_close();
}

void CFMUAutorouteProxy::child_run(void)
{
	if (m_childrun)
		return;
	{
		Command cmd("proxy");
		cmd.set_option("state", proxystate_started);
		recvcmd(cmd);
	}
	int childstdin(-1), childstdout(-1);
	try {
		std::vector<std::string> argv, env;
#ifdef __MINGW32__
		std::string workdir;
		{
			gchar *instdir(g_win32_get_package_installation_directory_of_module(0));
			if (instdir) {
				workdir = instdir;
				argv.push_back(Glib::build_filename(instdir, "bin", "cfmuautoroute.exe"));
			} else {
				argv.push_back(Glib::build_filename(PACKAGE_PREFIX_DIR "/bin", "cfmuautoroute.exe"));
				workdir = PACKAGE_PREFIX_DIR;
			}
		}
#else
		std::string workdir(PACKAGE_DATA_DIR);
		argv.push_back(Glib::build_filename(PACKAGE_PREFIX_DIR "/bin", "cfmuautoroute"));
#endif
		argv.push_back("--machineinterface");
#ifndef __MINGW32__
		env.push_back("PATH=/bin:/usr/bin");
		{
			bool found(false);
			std::string disp(Glib::getenv("DISPLAY", found));
			if (found)
				env.push_back("DISPLAY=" + disp);
		}
#endif
		if (false)
			std::cerr << "spwaning: " << argv[0] << " wd: " << workdir << std::endl;
#ifdef __MINGW32__
		// inherit environment; otherwise, this may fail on WindowsXP with STATUS_SXS_ASSEMBLY_NOT_FOUND (C0150004)
		Glib::spawn_async_with_pipes(workdir, argv,
					     Glib::SPAWN_DO_NOT_REAP_CHILD | Glib::SPAWN_STDERR_TO_DEV_NULL,
					     sigc::slot<void>(), &m_childpid, &childstdin, &childstdout, 0);
#else
		Glib::spawn_async_with_pipes(workdir, argv, env,
					     Glib::SPAWN_DO_NOT_REAP_CHILD | Glib::SPAWN_STDERR_TO_DEV_NULL,
					     sigc::slot<void>(), &m_childpid, &childstdin, &childstdout, 0);
#endif
		m_childrun = true;
	} catch (Glib::SpawnError& e) {
		m_childrun = false;
		{
			Command cmd("proxy");
			cmd.set_option("state", proxystate_error);
			recvcmd(cmd);
		}
		if (true)
			std::cerr << "cannot spawn: " << e.what() << std::endl;
		return;
	}
	m_connchildwatch = Glib::signal_child_watch().connect(sigc::mem_fun(this, &CFMUAutorouteProxy::child_watch), m_childpid);
	m_childchanstdin = Glib::IOChannel::create_from_fd(childstdin);
	m_childchanstdout = Glib::IOChannel::create_from_fd(childstdout);
	m_childchanstdin->set_encoding();
	m_childchanstdout->set_encoding();
	m_childchanstdin->set_close_on_unref(true);
	m_childchanstdout->set_close_on_unref(true);
#ifndef G_OS_WIN32
	m_childchanstdin->set_flags(m_childchanstdin->get_flags() | Glib::IO_FLAG_NONBLOCK);
#endif
	m_childchanstdin->set_buffered(true);
	m_childchanstdin->set_buffer_size(32768);
	m_connchildstdout = Glib::signal_io().connect(sigc::mem_fun(this, &CFMUAutorouteProxy::child_stdout_handler), m_childchanstdout,
						      Glib::IO_IN | Glib::IO_ERR | Glib::IO_HUP);
}

bool CFMUAutorouteProxy::child_stdin_handler(Glib::IOCondition iocond)
{
	if (m_sendbuf.empty())
		return false;
	gsize written(0);
	if (false)
		std::cerr << "cfmuautorouteproxy: tx bytes: " << m_sendbuf.size() << std::endl;
	Glib::IOStatus stat(m_childchanstdin->write(m_sendbuf.c_str(), m_sendbuf.size(), written));
	if (false)
		std::cerr << "cfmuautorouteproxy: tx result: " << (int)stat << " bytes: " << written << std::endl;
	if (written > 0)
		m_sendbuf.erase(0, written);
	switch (stat) {
	default:
	case Glib::IO_STATUS_ERROR:
	case Glib::IO_STATUS_EOF:
		child_close();
		return false;

	case Glib::IO_STATUS_NORMAL:
	case Glib::IO_STATUS_AGAIN:
		if (m_sendbuf.empty()) {
			m_childchanstdin->flush();
			return false;
		}
		return true;
	}
}

bool CFMUAutorouteProxy::child_stdout_handler(Glib::IOCondition iocond)
{
        Glib::ustring line;
        Glib::IOStatus iostat(m_childchanstdout->read_line(line));
        if (iostat == Glib::IO_STATUS_ERROR ||
            iostat == Glib::IO_STATUS_EOF) {
                child_close();
		return true;
        }
	{
		Glib::ustring::size_type n(line.size());
		while (n > 0) {
			--n;
			if (!isspace(line[n]) && line[n] != '\n' && line[n] != '\r') {
				++n;
				break;
			}
		}
		line = Glib::ustring(line, 0, n);
	}
	Command cmd(line);
	if (!cmd.get_cmdname().empty())
		recvcmd(cmd);
        return true;
}

void CFMUAutorouteProxy::child_watch(GPid pid, int child_status)
{
	if (!m_childrun)
		return;
	if (m_childpid != pid) {
		Glib::spawn_close_pid(pid);
		return;
	}
	Glib::spawn_close_pid(pid);
	m_childrun = false;
	m_connchildwatch.disconnect();
	m_connchildstdout.disconnect();
	m_connchildstdin.disconnect();
	child_close();
	{
		Command cmd("proxy");
		cmd.set_option("state", proxystate_stopped);
		recvcmd(cmd);
	}
}

void CFMUAutorouteProxy::child_close(void)
{
	m_connchildstdin.disconnect();
	m_connchildstdout.disconnect();
	if (m_childchanstdin) {
		m_childchanstdin->close();
		m_childchanstdin.reset();
	}
	if (m_childchanstdout) {
		m_childchanstdout->close();
		m_childchanstdout.reset();
	}
	m_sendbuf.clear();
	if (!m_childrun)
		return;
	m_childrun = false;
	m_connchildwatch.disconnect();
	Glib::signal_child_watch().connect(sigc::hide(sigc::ptr_fun(&Glib::spawn_close_pid)), m_childpid);
	{
		Command cmd("proxy");
		cmd.set_option("state", proxystate_stopped);
		recvcmd(cmd);
	}
}

void CFMUAutorouteProxy::clear_fplan(void)
{
	m_route = FPlanRoute(*(FPlan *)0);
}

void CFMUAutorouteProxy::set_fplan(const FPlanRoute& fpl)
{
	m_route = fpl;
}

void CFMUAutorouteProxy::sendcmd(const Command& cmd)
{
	child_run();
	if (!m_childrun)
		throw std::runtime_error("cfmuautorouteproxy: cannot start child");
	if (false)
		std::cerr << "cfmuautorouteproxy: tx: " << cmd.get_cmdstring() << std::endl;
	{
		bool sendempty(m_sendbuf.empty());
		m_sendbuf += cmd.get_cmdstring();
		m_sendbuf.push_back('\n');
		if (!sendempty)
			return;
	}
	gsize written(0);
	if (false)
		std::cerr << "cfmuautorouteproxy: tx bytes: " << m_sendbuf.size() << std::endl;
	Glib::IOStatus stat(m_childchanstdin->write(m_sendbuf.c_str(), m_sendbuf.size(), written));
	if (false)
		std::cerr << "cfmuautorouteproxy: tx result: " << (int)stat << " bytes: " << written << std::endl;
	if (written > 0)
		m_sendbuf.erase(0, written);
	switch (stat) {
	default:
	case Glib::IO_STATUS_ERROR:
	case Glib::IO_STATUS_EOF:
		child_close();
		break;

	case Glib::IO_STATUS_NORMAL:
	case Glib::IO_STATUS_AGAIN:
		if (m_sendbuf.empty()) {
			m_childchanstdin->flush();
			break;
		}
	       	m_connchildstdin = Glib::signal_io().connect(sigc::mem_fun(this, &CFMUAutorouteProxy::child_stdin_handler), m_childchanstdin,
							     Glib::IO_OUT | Glib::IO_ERR | Glib::IO_HUP);
		break;
	}
}

void CFMUAutorouteProxy::recvcmd(const Command& cmd)
{
	if (false)
		std::cerr << "cfmuautorouteproxy: rx: " << cmd.get_cmdstring() << std::endl;
	if (cmd.get_cmdname() == "fplbegin") {
		m_route = FPlanRoute(*(FPlan *)0);
		{
			std::pair<const std::string&,bool> o(cmd.get_option("note"));
			if (o.second)
				m_route.set_note(o.first);
		}
		{
			std::pair<unsigned long,bool> o(cmd.get_option_uint("curwpt"));
			if (o.second)
				m_route.set_curwpt(o.first);
		}
		{
			std::pair<unsigned long,bool> o(cmd.get_option_uint("offblock"));
			if (o.second)
				m_route.set_time_offblock_unix(o.first);
		}
		{
			std::pair<unsigned long,bool> o(cmd.get_option_uint("save_offblock"));
			if (o.second)
				m_route.set_save_time_offblock_unix(o.first);
		}
		{
			std::pair<unsigned long,bool> o(cmd.get_option_uint("onblock"));
			if (o.second)
				m_route.set_time_onblock_unix(o.first);
		}
		{
			std::pair<unsigned long,bool> o(cmd.get_option_uint("save_onblock"));
			if (o.second)
				m_route.set_save_time_onblock_unix(o.first);
		}
	}
	if (cmd.get_cmdname() == "fplwpt") {
		FPlanWaypoint wpt;
		{
			std::pair<const std::string&,bool> o(cmd.get_option("icao"));
			if (o.second)
				wpt.set_icao(o.first);
		}
		{
			std::pair<const std::string&,bool> o(cmd.get_option("name"));
			if (o.second)
				wpt.set_name(o.first);
		}
		{
			std::pair<const std::string&,bool> o(cmd.get_option("pathname"));
			if (o.second)
				wpt.set_pathname(o.first);
		}
		{
			std::pair<const std::string&,bool> o(cmd.get_option("pathcode"));
			if (o.second)
				wpt.set_pathcode_name(o.first);
		}
		{
			std::pair<const std::string&,bool> o(cmd.get_option("note"));
			if (o.second)
				wpt.set_note(o.first);
		}
		{
			std::pair<unsigned long,bool> o(cmd.get_option_uint("time"));
			if (o.second)
				wpt.set_time_unix(o.first);
		}
		{
			std::pair<unsigned long,bool> o(cmd.get_option_uint("save_time"));
			if (o.second)
				wpt.set_save_time_unix(o.first);
		}
		{
			std::pair<unsigned long,bool> o(cmd.get_option_uint("flighttime"));
			if (o.second)
				wpt.set_flighttime(o.first);
		}
		{
			std::pair<unsigned long,bool> o(cmd.get_option_uint("frequency"));
			if (o.second)
				wpt.set_frequency(o.first);
		}
		{
			std::pair<Point,bool> o(cmd.get_option_coord("coord"));
			if (o.second) {
				wpt.set_coord(o.first);
			} else {
				std::pair<double,bool> o(cmd.get_option_float("lat"));
				if (o.second)
					wpt.set_lat_deg(o.first);
				o = cmd.get_option_float("lon");
				if (o.second)
					wpt.set_lon_deg(o.first);
			}
		}
		{
			std::pair<long,bool> o(cmd.get_option_int("altitude"));
			if (o.second)
				wpt.set_altitude(o.first);
		}
		{
			std::pair<unsigned long,bool> o(cmd.get_option_uint("flags"));
			if (o.second)
				wpt.set_flags(o.first);
		}
		{
			std::pair<double,bool> o(cmd.get_option_float("winddir"));
			if (o.second)
				wpt.set_winddir_deg(o.first);
		}
		{
			std::pair<double,bool> o(cmd.get_option_float("windspeed"));
			if (o.second)
				wpt.set_windspeed_kts(o.first);
		}
		{
			std::pair<double,bool> o(cmd.get_option_float("qff"));
			if (o.second)
				wpt.set_qff_hpa(o.first);
		}
		{
			std::pair<double,bool> o(cmd.get_option_float("isaoffset"));
			if (o.second)
				wpt.set_isaoffset_kelvin(o.first);
		}
		{
			std::pair<long,bool> o(cmd.get_option_int("truealt"));
			if (o.second)
				wpt.set_truealt(o.first);
		}
		{
			std::pair<double,bool> o(cmd.get_option_float("dist"));
			if (o.second)
				wpt.set_dist_nmi(o.first);
		}
		{
			std::pair<double,bool> o(cmd.get_option_float("fuel"));
			if (o.second)
				wpt.set_fuel_usg(o.first);
		}
		{
			std::pair<double,bool> o(cmd.get_option_float("tt"));
			if (o.second)
				wpt.set_truetrack_deg(o.first);
		}
		{
			std::pair<double,bool> o(cmd.get_option_float("th"));
			if (o.second)
				wpt.set_trueheading_deg(o.first);
		}
		{
			std::pair<double,bool> o(cmd.get_option_float("decl"));
			if (o.second)
				wpt.set_declination_deg(o.first);
		}
		{
			std::pair<double,bool> o(cmd.get_option_float("tas"));
			if (o.second)
				wpt.set_tas_kts(o.first);
		}
		{
			std::pair<unsigned long,bool> o(cmd.get_option_uint("rpm"));
			if (o.second)
				wpt.set_rpm(o.first);
		}
		{
			std::pair<double,bool> o(cmd.get_option_float("mp"));
			if (o.second)
				wpt.set_mp_inhg(o.first);
		}
		{
			std::pair<const std::string&,bool> o(cmd.get_option("type"));
			if (o.second)
				wpt.set_type_string(o.first);
		}
		m_route.insert_wpt(~0U, wpt);
	}
	m_signal_recvcmd(cmd);
}
