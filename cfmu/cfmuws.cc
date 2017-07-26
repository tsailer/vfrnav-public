#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <sys/time.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <fcntl.h>
#include <unistd.h>
#include <getopt.h>
#include <sysexits.h>
#include <signal.h>
#include <pwd.h>
#include <grp.h>

#include <iomanip>
#include <iostream>
#include <fstream>
#include <sstream>

#include <websocketpp/config/asio_no_tls.hpp>
#include <websocketpp/concurrency/none.hpp>
#include <websocketpp/server.hpp>

#include <json/json.h>
#include <openssl/sha.h>

using websocketpp::lib::placeholders::_1;
using websocketpp::lib::placeholders::_2;
using websocketpp::lib::bind;
using websocketpp::connection_hdl;

struct config : public websocketpp::config::asio {
	typedef config type;
	typedef websocketpp::config::asio base;

	typedef websocketpp::concurrency::none concurrency_type;

	typedef base::request_type request_type;
	typedef base::response_type response_type;

	typedef base::message_type message_type;
	typedef base::con_msg_manager_type con_msg_manager_type;
	typedef base::endpoint_msg_manager_type endpoint_msg_manager_type;
    
	typedef base::alog_type alog_type;
	typedef base::elog_type elog_type;
    
	typedef base::rng_type rng_type;

	struct transport_config : public base::transport_config {
		typedef type::concurrency_type concurrency_type;
		typedef type::alog_type alog_type;
		typedef type::elog_type elog_type;
		typedef type::request_type request_type;
		typedef type::response_type response_type;
		typedef base::transport_config::socket_type socket_type;
	};

	typedef websocketpp::transport::asio::endpoint<transport_config> transport_type;
};

class Server : public websocketpp::server<config> {
public:
	Server(boost::asio::io_service& ioserv);
	~Server();
	void add_user(const std::string& user, const std::string& passwd);
	void set_connectlimit(unsigned int limit = ~0U) { m_connectlimit = limit; }
	unsigned int get_connectlimit(void) const { return m_connectlimit; }
	void set_timeout(unsigned int tmo = ~0U) { m_timeout = tmo; }
	unsigned int get_timeout(void) const { return m_timeout; }
	void set_xdisplay(int disp);
	void start_xdisplay(void);

protected:
	class Command {
	public:
		Command(void);
		Command(const std::string& cmd);
		Command(const Json::Value& root);

		const std::string& get_cmdname(void) const { return m_cmdname; }
		void set_cmdname(const std::string cmdname) { m_cmdname = cmdname; }

		std::string get_cmdstring(void) const;
		Json::Value get_json(void) const;

		void unset_option(const std::string& name, bool require = false);
		void set_option(const std::string& name, const std::string& value, bool replace = false);
		template<typename T> void set_option(const std::string& name, T value, bool replace = false);

		std::pair<const std::string&,bool> get_option(const std::string& name) const;
		bool is_option(const std::string& name) const { return get_option(name).second; }
		std::pair<long,bool> get_option_int(const std::string& name) const;
		std::pair<unsigned long,bool> get_option_uint(const std::string& name) const;
		std::pair<long long,bool> get_option_long(const std::string& name) const;
		std::pair<unsigned long long,bool> get_option_ulong(const std::string& name) const;
		std::pair<double,bool> get_option_float(const std::string& name) const;

		static void error(const std::string& text);

		bool empty(void) const { return m_opts.empty(); }

	protected:
		static const std::string empty_string;
		std::string m_cmdname;
		typedef std::map<std::string,std::string> opts_t;
		opts_t m_opts;

		static bool is_str_needs_quotes(const std::string& s, bool nocomma = false);
		static std::string quote_str(const std::string& s);
		static bool unquote_str(std::string& res, const std::string& s, std::string::size_type& pos, bool nocomma = false);
	};
 
	class Router {
	public:
		Router(Server *srv, boost::asio::io_service& ioserv, websocketpp::connection_hdl hdl, const std::string& user = "");
		~Router();
		bool is_running(void) const { return m_pid != -1; }
		void sendcmd(const Command& cmd);
		const std::string& get_user(void) const { return m_user; }
		void run(int xdisplay = -1);
		pid_t terminate(void);

	protected:
		Server *m_server;
		boost::asio::posix::stream_descriptor m_stdin;
		boost::asio::posix::stream_descriptor m_stdout;
		boost::asio::signal_set m_sigchld;
		boost::asio::deadline_timer m_timer;
		websocketpp::connection_hdl m_hdl;
		pid_t m_pid;
		boost::asio::streambuf  m_stdout_buffer;
		std::string m_cmdassembly;
		std::string m_user;

		std::string m_logdep;
		std::string m_logdepsid;
		std::string m_logdest;
		std::string m_logdeststar;
		bool m_logdepifr;
		bool m_logdestifr;
		std::string m_logacft;
		std::string m_logacftreg;
		std::string m_loglevelbase;
		std::string m_logleveltop;
		std::string m_logopt;
		std::string m_logtfrena;
		std::string m_logpreflevel;
		std::string m_logprefpenalty;
		std::string m_logprefclimb;
		std::string m_logprefdescent;
		std::string m_logfpl;

		void handle_sigchld(const boost::system::error_code& error, int signal_number);
		void handle_timeout(boost::system::error_code ec);
		void handle_stdout(const boost::system::error_code& error, std::size_t bytes_transferred);
		void sendcmd(const std::string& cmd);
		void logdatarecv(const Command& cmd);
		void logdatasend(const Command& cmd);
	};

	boost::asio::io_service& m_ioserv;
	typedef std::map<std::string, std::string> userdb_t;
	userdb_t m_userdb;
	typedef std::map<connection_hdl, std::string> loginchallenge_t;
	loginchallenge_t m_loginchallenge;
	typedef std::map<connection_hdl, boost::shared_ptr<Router> > routers_t;
	routers_t m_routers;
	unsigned int m_connectlimit;
	unsigned int m_timeout;
	typedef std::set<pid_t> pidset_t;
	pidset_t m_reappids;
	boost::asio::signal_set m_xsigchld;
	pid_t m_xpid;
	int m_xdisplay;

	void transmit(websocketpp::connection_hdl hdl, const Json::Value& msg);
	Json::Value receive(server::message_ptr msg);

	unsigned int remove_closed_routers(void);
	unsigned int count_user_routers(const std::string& user);

	void on_open(connection_hdl hdl);
	void on_close(connection_hdl hdl);
	void on_login_message(websocketpp::connection_hdl hdl, server::message_ptr msg);
	void on_message(websocketpp::connection_hdl hdl, server::message_ptr msg);

	void xserver_sigchld(const boost::system::error_code& error, int signal_number);
};

const std::string Server::Command::empty_string("");

template void Server::Command::set_option(const std::string& name, unsigned short value, bool replace = false);
template void Server::Command::set_option(const std::string& name, short value, bool replace = false);
template void Server::Command::set_option(const std::string& name, unsigned int value, bool replace = false);
template void Server::Command::set_option(const std::string& name, int value, bool replace = false);
template void Server::Command::set_option(const std::string& name, unsigned long value, bool replace = false);
template void Server::Command::set_option(const std::string& name, long value, bool replace = false);
template void Server::Command::set_option(const std::string& name, unsigned long long value, bool replace = false);
template void Server::Command::set_option(const std::string& name, long long value, bool replace = false);
template void Server::Command::set_option(const std::string& name, float value, bool replace = false);
template void Server::Command::set_option(const std::string& name, double value, bool replace = false);
//template void Server::Command::set_option(const std::string& name, std::set<std::string>::const_iterator b, std::set<std::string>::const_iterator e, bool replace = false);

Server::Command::Command(void)
{
}

Server::Command::Command(const std::string& cmd)
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

Server::Command::Command(const Json::Value& root)
	: m_cmdname("error")
{
	if (!root.isObject())
		return;
	Json::Value::Members m(root.getMemberNames());
	for (Json::Value::Members::const_iterator mi(m.begin()), me(m.end()); mi != me; ++mi) {
		if (*mi == "cmd") {
			m_cmdname = root.get(*mi, "").asString();
			continue;
		}
		const Json::Value& v(root.get(*mi, ""));
		if (v.isArray() || v.isObject())
			continue;
		if (v.isInt()) {
			std::ostringstream oss;
			oss << v.asLargestInt();
			set_option(*mi, oss.str());
			continue;
		}
		if (v.isUInt()) {
			std::ostringstream oss;
			oss << v.asLargestInt();
			set_option(*mi, oss.str());
			continue;
		}
		if (v.isIntegral()) {
			std::ostringstream oss;
			oss << v.asLargestUInt();
			set_option(*mi, oss.str());
			continue;
		}
		if (v.isDouble() ) {
			std::ostringstream oss;
			oss << std::setprecision(15) << v.asDouble();
			set_option(*mi, oss.str());
			continue;
		}
		set_option(*mi, v.asString());
	}
}

bool Server::Command::is_str_needs_quotes(const std::string& s, bool nocomma)
{
	for (std::string::const_iterator si(s.begin()), se(s.end()); si != se; ++si)
		if (*si <= ' ' || *si >= 0x7f ||
		    *si == '\\' || *si == '\"' ||
		    (nocomma && *si == ','))
			return true;
	return false;
}

std::string Server::Command::quote_str(const std::string& s)
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

bool Server::Command::unquote_str(std::string& res, const std::string& s, std::string::size_type& pos, bool nocomma)
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

std::string Server::Command::get_cmdstring(void) const
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

void Server::Command::unset_option(const std::string& name, bool require)
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

void Server::Command::set_option(const std::string& name, const std::string& value, bool replace)
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

template<typename T> void Server::Command::set_option(const std::string& name, T value, bool replace)
{
	std::ostringstream oss;
	oss << value;
	set_option(name, oss.str(), replace);
}

std::pair<const std::string&,bool> Server::Command::get_option(const std::string& name) const
{
	opts_t::const_iterator i(m_opts.find(name));
	if (i == m_opts.end())
		return std::pair<const std::string&,bool>(empty_string, false);
	return std::pair<const std::string&,bool>(i->second, true);
}

std::pair<long,bool> Server::Command::get_option_int(const std::string& name) const
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

std::pair<unsigned long,bool> Server::Command::get_option_uint(const std::string& name) const
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

std::pair<long long,bool> Server::Command::get_option_long(const std::string& name) const
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

std::pair<unsigned long long,bool> Server::Command::get_option_ulong(const std::string& name) const
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

std::pair<double,bool> Server::Command::get_option_float(const std::string& name) const
{
	std::pair<const std::string&,bool> o(get_option(name));
	std::pair<double,bool> ret(0, o.second);
	if (!ret.second)
		return ret;
	const char *cp(o.first.c_str());
	char *cpe;
	ret.first = strtod(cp, &cpe);
	if ((size_t)(cpe - cp) >= o.first.length())
		return ret;
	error("option " + name + " is not a float");
	return std::pair<double,bool>(0, false);
}

void Server::Command::error(const std::string& text)
{
	throw std::runtime_error(text);
}

Json::Value Server::Command::get_json(void) const
{
	Json::Value root;
	{
		std::string cs(get_cmdname());
		if (cs.empty())
			cs = "autoroute";
		root["cmd"] = cs;
	}
	for (opts_t::const_iterator oi(m_opts.begin()), oe(m_opts.end()); oi != oe; ++oi)
		root[oi->first] = oi->second;
	return root;
}

Server::Router::Router(Server *srv, boost::asio::io_service& ioserv, websocketpp::connection_hdl hdl, const std::string& user)
	: m_server(srv), m_stdin(ioserv), m_stdout(ioserv), m_sigchld(ioserv, SIGCHLD), m_timer(ioserv),
	  m_hdl(hdl), m_pid(-1), m_user(user), m_logdepifr(false), m_logdestifr(false)
{
}

Server::Router::~Router()
{
	pid_t mpid(terminate());
	if (mpid != (pid_t)-1) {
		int status(0);
		pid_t p(waitpid(mpid, &status, WNOHANG));
		if (p == 0)
			waitpid(mpid, &status, 0);
	}
}

void Server::Router::run(int xdisplay)
{
	if (m_pid != (pid_t)-1)
		return;
	int stdin[2], stdout[2];
	if (pipe(stdin))
		return;
	if (pipe(stdout)) {
		::close(stdin[0]);
		::close(stdin[1]);
		return;
	}
	boost::asio::io_service& ioserv(m_sigchld.get_io_service());
	m_sigchld.async_wait(bind(&Router::handle_sigchld, this, ::_1, ::_2));
	ioserv.notify_fork(boost::asio::io_service::fork_prepare);
	m_pid = fork();
	switch (m_pid) {
	case -1:
		ioserv.notify_fork(boost::asio::io_service::fork_parent);
		::close(stdin[0]);
		::close(stdin[1]);
		::close(stdout[0]);
		::close(stdout[1]);
		return;

	case 0:
		ioserv.notify_fork(boost::asio::io_service::fork_child);
		::close(stdin[1]);
		::close(stdout[0]);
		dup2(stdin[0], 0);
		dup2(stdout[1], 1);
		::close(stdin[0]);
		::close(stdout[1]);
		{
			int fd(open("/dev/null", O_WRONLY));
			if (fd != 2) {
				dup2(fd, 2);
				::close(fd);
			}
		}
		{
			long fd(sysconf(_SC_OPEN_MAX));
			while (fd > 3) {
				--fd;
				::close(fd);
			}
		}
		chdir(PACKAGE_DATA_DIR);
		{
			char *argv[3] = { strdupa(PACKAGE_PREFIX_DIR "/bin/cfmuautoroute"), strdupa("--machineinterface"), 0 };
			char *envp[3] = { strdupa("PATH=/bin:/usr/bin"), 0, 0 };
			if (xdisplay < 0) {
				char *disp(getenv("DISPLAY"));
				if (disp) {
					char *x(static_cast<char *>(alloca(10 + strlen(disp))));
					strcpy(x, "DISPLAY=");
					strcat(x, disp);
					envp[1] = x;
				}
			} else {
				std::ostringstream disp;
				disp << "DISPLAY=:" << xdisplay;
				envp[1] = strdupa(disp.str().c_str());
			}
			execve(argv[0], argv, envp);
		}
		exit(EX_UNAVAILABLE);
		break;

	default:
		break;
	}
	ioserv.notify_fork(boost::asio::io_service::fork_parent);
	::close(stdin[0]);
	::close(stdout[1]);
	m_stdin.assign(stdin[1]);
	m_stdout.assign(stdout[0]);
	boost::asio::async_read(m_stdout, m_stdout_buffer, boost::asio::transfer_at_least(1), bind(&Router::handle_stdout, this, ::_1, ::_2));
	m_timer.expires_from_now(boost::posix_time::seconds(m_server->get_timeout()));
	m_timer.async_wait(bind(&Router::handle_timeout, this, ::_1));
}

pid_t Server::Router::terminate(void)
{
	if (m_pid == (pid_t)-1)
		return m_pid;
	pid_t mpid(m_pid);
	m_pid = -1;
	m_sigchld.cancel();
	m_stdout.cancel();
	m_stdin.close();
	m_stdout.close();
	m_timer.cancel();
	kill(mpid, SIGHUP);
	return mpid;
}

void Server::Router::handle_sigchld(const boost::system::error_code& error, int signal_number)
{
	if (error)
		return;
	if (m_pid == (pid_t)-1)
		return;
	int status(0);
	pid_t pid(waitpid(m_pid, &status, WNOHANG));
	if (pid != m_pid)
		return;
	m_pid = -1;
	m_stdout.cancel();
	m_stdin.close();
	m_stdout.close();
	m_timer.cancel();
}

void Server::Router::handle_timeout(boost::system::error_code ec)
{
	if (ec)
		return;
	if (m_pid == (pid_t)-1)
		return;
	pid_t mpid(m_pid);
	m_pid = -1;
	m_stdin.close();
	m_stdout.close();
	m_timer.cancel();
	kill(mpid, SIGHUP);
	int status(0);
	waitpid(mpid, &status, 0);
	Json::Value tx;
	tx["cmd"] = "error";
	tx["error"] = "timeout";
	m_server->transmit(m_hdl, tx);
}

void Server::Router::handle_stdout(const boost::system::error_code& error, std::size_t bytes_transferred)
{
	if (error) {
		if (true) {
			try {
				std::ostringstream oss;
				oss << "handle_stdout: error " << error;
				m_server->get_alog().write(websocketpp::log::alevel::app, oss.str());
			} catch (const boost::lock_error& e) {
			}
		}
		return;
	}
	{
		std::size_t s(0);
		boost::asio::streambuf::const_buffers_type bufs(m_stdout_buffer.data());
		boost::asio::streambuf::const_buffers_type::const_iterator i = bufs.begin();
		while (i != bufs.end())	{
			boost::asio::const_buffer buf(*i++);
			const char *bp(boost::asio::buffer_cast<const char*>(buf));
			std::size_t bs(boost::asio::buffer_size(buf));
			if (bs > bytes_transferred)
				bs = bytes_transferred;
			m_cmdassembly += std::string(bp, bp + bs);
			s += bs;
			bytes_transferred -= bs;
			if (!bytes_transferred)
				break;
		}
		m_stdout_buffer.consume(s);
	}
	boost::asio::async_read(m_stdout, m_stdout_buffer, boost::asio::transfer_at_least(1), bind(&Router::handle_stdout, this, ::_1, ::_2));
	if (false) {
		std::ostringstream oss;
		oss << "handle_stdout: command assembly " << m_cmdassembly.size();
		m_server->get_alog().write(websocketpp::log::alevel::control, oss.str());
	}
	for (;;) {
		std::size_t n(m_cmdassembly.find_first_of("\r\n"));
		if (n == std::string::npos)
			break;
		if (n) {
			Command cmd(m_cmdassembly.substr(0, n));
			if (true)
				m_server->get_alog().write(websocketpp::log::alevel::frame_payload, "Receive cmd: " + cmd.get_cmdstring());
			logdatarecv(cmd);
			m_server->transmit(m_hdl, cmd.get_json());
		}
		m_cmdassembly.erase(0, n + 1);
	}
	if (false) {
		std::ostringstream oss;
		oss << "handle_stdout: remaining command assembly " << m_cmdassembly.size();
		m_server->get_alog().write(websocketpp::log::alevel::control, oss.str());
	}
}

void Server::Router::sendcmd(const Command& cmd)
{
	if (true)
		m_server->get_alog().write(websocketpp::log::alevel::frame_payload, "Transmit cmd: " + cmd.get_cmdstring());
	logdatasend(cmd);
	sendcmd(cmd.get_cmdstring() + "\n");
}

void Server::Router::sendcmd(const std::string& cmd)
{
	if (m_pid == -1)
		return;
	boost::asio::write(m_stdin, boost::asio::buffer(cmd.data(), cmd.length()));
	m_timer.cancel();
	m_timer.expires_from_now(boost::posix_time::seconds(m_server->get_timeout()));
	m_timer.async_wait(bind(&Router::handle_timeout, this, ::_1));
}

void Server::Router::logdatarecv(const Command& cmd)
{
	if (cmd.get_cmdname() == "departure") {
		std::pair<std::string,bool> icao(cmd.get_option("icao"));
		if (icao.second)
			m_logdep = icao.first;
		std::pair<std::string,bool> sid(cmd.get_option("sid"));
		if (sid.second)
			m_logdepsid = sid.first;
		if (cmd.is_option("ifr"))
			m_logdepifr = true;
		else if (cmd.is_option("vfr"))
			m_logdepifr = false;
		return;
	}
	if (cmd.get_cmdname() == "destination") {
		std::pair<std::string,bool> icao(cmd.get_option("icao"));
		if (icao.second)
			m_logdest = icao.first;
		std::pair<std::string,bool> star(cmd.get_option("star"));
		if (star.second)
			m_logdeststar = star.first;
		if (cmd.is_option("ifr"))
			m_logdestifr = true;
		else if (cmd.is_option("vfr"))
			m_logdestifr = false;
		return;
	}
	if (cmd.get_cmdname() == "levels") {
		std::pair<std::string,bool> base(cmd.get_option("base"));
		if (base.second)
			m_loglevelbase = base.first;
		std::pair<std::string,bool> top(cmd.get_option("top"));
		if (top.second)
			m_logleveltop = top.first;
		return;
	}
	if (cmd.get_cmdname() == "optimization") {
		std::pair<std::string,bool> target(cmd.get_option("target"));
		if (target.second)
			m_logopt = target.first;
		return;
	}
	if (cmd.get_cmdname() == "aircraft") {
		std::pair<std::string,bool> reg(cmd.get_option("registration"));
		if (reg.second)
		        m_logacftreg = reg.first;
		return;
	}
	if (cmd.get_cmdname() == "tfr") {
		std::pair<std::string,bool> enabled(cmd.get_option("enabled"));
		if (enabled.second)
			m_logtfrena = enabled.first;
		return;
	}
	if (cmd.get_cmdname() == "preferred") {
		std::pair<std::string,bool> level(cmd.get_option("level"));
		if (level.second)
			m_logpreflevel = level.first;
		std::pair<std::string,bool> penalty(cmd.get_option("penalty"));
		if (penalty.second)
			m_logprefpenalty = penalty.first;
		std::pair<std::string,bool> climb(cmd.get_option("climb"));
		if (climb.second)
			m_logprefclimb = climb.first;
		std::pair<std::string,bool> descent(cmd.get_option("descent"));
		if (descent.second)
			m_logprefdescent = descent.first;
		return;
	}
	if (cmd.get_cmdname() == "fplend") {
		std::pair<std::string,bool> fpl(cmd.get_option("fpl"));
		if (fpl.second)
			m_logfpl = fpl.first;
		return;
	}
	{
		bool cmdstart(cmd.get_cmdname() == "start");
		bool cmdstop(cmd.get_cmdname() == "autoroute");
		if (cmdstop) {
			std::pair<std::string,bool> status(cmd.get_option("status"));
			cmdstop = status.second && status.first == "stopping";
		}
		if (cmdstart || cmdstop) {
			std::ostringstream oss;
			if (cmdstart)
				oss << "Start: ";
			if (cmdstop)
				oss << "Stop: ";
			oss << "DEP " << m_logdep << ' ' << (m_logdepifr ? 'I' : 'V') << "FR SID " << m_logdepsid
			    << " DEST " << m_logdest << ' ' << (m_logdestifr ? 'I' : 'V') << "FR SID " << m_logdeststar
			    << " ACFT " << m_logacft << ' ' << m_logacftreg << " LEVEL " << m_loglevelbase << ".." << m_logleveltop
			    << " OPT " << m_logopt << " TFR " << m_logtfrena
			    << " PREF " << m_logpreflevel << ' ' << m_logprefpenalty << ' ' << m_logprefclimb << ' ' << m_logprefdescent;
			if (cmdstop)
				oss << " FPL " << m_logfpl;
			m_server->get_alog().write(websocketpp::log::alevel::app, oss.str());
		}
	}
}

void Server::Router::logdatasend(const Command& cmd)
{
	if (cmd.get_cmdname() == "aircraft") {
		std::pair<std::string,bool> templ(cmd.get_option("template"));
		if (templ.second)
			m_logacft = templ.first;
	}

}

Server::Server(boost::asio::io_service& ioserv)
	: m_ioserv(ioserv), m_connectlimit(~0U), m_timeout(600), m_xsigchld(ioserv), m_xpid(-1), m_xdisplay(-1)
{
	set_open_handler(bind(&Server::on_open, this, ::_1));
        set_close_handler(bind(&Server::on_close, this, ::_1));
 	set_message_handler(bind(&Server::on_login_message, this, ::_1, ::_2));
}

Server::~Server()
{
	if (m_xpid != -1)
		kill(m_xpid, SIGTERM);
}

void Server::add_user(const std::string& user, const std::string& passwd)
{
	if (user.empty())
		return;
	m_userdb[user] = passwd;
}

void Server::transmit(websocketpp::connection_hdl hdl, const Json::Value& msg)
{
	try {
		Json::FastWriter writer;
		std::string msg1(writer.write(msg));
		send(hdl, msg1, websocketpp::frame::opcode::text);
		if (true)
			get_alog().write(websocketpp::log::alevel::frame_payload,
					 "Transmit JSON: " + msg1);
	} catch (const websocketpp::lib::error_code& e) {
		get_alog().write(websocketpp::log::alevel::app, "Command Transmission Failed: " + e.message());
	}
}

Json::Value Server::receive(server::message_ptr msg)
{
	if (msg->get_opcode() != websocketpp::frame::opcode::text) {
		get_alog().write(websocketpp::log::alevel::app,
				 "Binary Message Received: " + websocketpp::utility::to_hex(msg->get_payload()));
		return Json::Value();
	}
	Json::Value root;
	Json::Reader reader;
	if (!reader.parse(msg->get_payload(), root)) {
		get_alog().write(websocketpp::log::alevel::app,
				 "Cannot parse JSON: " + msg->get_payload());
		return Json::Value();
	}
	if (!root.isObject() || !root.isMember("cmd")) {
		get_alog().write(websocketpp::log::alevel::app,
				 "JSON object invalid: " + msg->get_payload());
		return Json::Value();
	}
	if (true)
		get_alog().write(websocketpp::log::alevel::frame_payload,
				 "Receive JSON: " + msg->get_payload());
	return root;
}

unsigned int Server::remove_closed_routers(void)
{
	unsigned int cnt(0);
	for (routers_t::iterator i(m_routers.begin()), e(m_routers.end()); i != e; ) {
		routers_t::iterator i2(i);
		++i;
		if (i2->second->is_running()) {
			++cnt;
			continue;
		}
		pid_t pid(i2->second->terminate());
		if (pid != (pid_t)-1)
			m_reappids.insert(pid);
		m_routers.erase(i2);
	}
	return cnt;
}

unsigned int Server::count_user_routers(const std::string& user)
{
	unsigned int cnt(0);
	for (routers_t::iterator i(m_routers.begin()), e(m_routers.end()); i != e; ) {
		routers_t::iterator i2(i);
		++i;
		if (!i2->second->is_running())
			continue;
		if (i2->second->get_user() == user) {
			++cnt;
			continue;
		}
	}
	return cnt;
}

void Server::on_open(connection_hdl hdl)
{
}

void Server::on_close(connection_hdl hdl)
{
	m_loginchallenge.erase(hdl);
	{
		routers_t::iterator i(m_routers.find(hdl));
		if (i != m_routers.end()) {
			pid_t pid(i->second->terminate());
			if (pid != (pid_t)-1)
				m_reappids.insert(pid);
			m_routers.erase(i);
		}
	}
}

void Server::on_login_message(websocketpp::connection_hdl hdl, server::message_ptr msg)
{
	Json::Value root(receive(msg));
	std::string cmd(root.get("cmd", "error").asString());
	if (cmd == "loginstart") {
		std::string chall;
		{
			struct timeval tv;
			gettimeofday(&tv, 0);
			uint8_t msg[32];
			unsigned int p(0);
			msg[p++] = tv.tv_sec;
			msg[p++] = tv.tv_sec >> 8;
			msg[p++] = tv.tv_sec >> 16;
			msg[p++] = tv.tv_sec >> 24;
			msg[p++] = tv.tv_sec >> 32;
			msg[p++] = tv.tv_sec >> 40;
			msg[p++] = tv.tv_sec >> 48;
			msg[p++] = tv.tv_sec >> 56;
			msg[p++] = tv.tv_usec;
			msg[p++] = tv.tv_usec >> 8;
			msg[p++] = tv.tv_usec >> 16;
			msg[p++] = tv.tv_usec >> 24;
			pid_t pid(getpid());
			msg[p++] = pid;
			msg[p++] = pid >> 8;
			msg[p++] = pid >> 16;
			msg[p++] = pid >> 24;
			uint8_t md[SHA256_DIGEST_LENGTH];
			SHA256(msg, p, md);
			std::ostringstream oss;
			oss << std::hex;
			for (unsigned int i = 0; i < SHA256_DIGEST_LENGTH; ++i)
				oss << std::setfill('0') << std::setw(2) << (unsigned int)md[i];
			chall = oss.str();
		}
		m_loginchallenge[hdl] = chall;
		Json::Value tx;
		tx["cmd"] = "loginchallenge";
		tx["challenge"] = chall;
		if (root.isMember("cmdseq"))
			tx["cmdseq"] = root.get("cmdseq", "").asString();
		transmit(hdl, tx);
		return;
	}
	if (cmd == "login") {
		Json::Value tx;
		tx["cmd"] = "loginresult";
		if (root.isMember("cmdseq"))
			tx["cmdseq"] = root.get("cmdseq", "").asString();
		tx["result"] = 0;
		if (!root.isMember("user") || !root.isMember("seq") || !root.isMember("hash")) {
			tx["error"] = "Login Protocol Error";
			transmit(hdl, tx);
			return;
		}
		loginchallenge_t::const_iterator chi(m_loginchallenge.find(hdl));
		userdb_t::const_iterator ui(m_userdb.find(root.get("user", "").asString()));
		std::string hash(root.get("hash", "").asString());
		if (chi == m_loginchallenge.end() || ui == m_userdb.end() || hash.size() != 2 * SHA256_DIGEST_LENGTH) {
			tx["error"] = "Invalid User/Password";
			transmit(hdl, tx);
			if (true) {
				std::ostringstream oss;
				oss << "Connection Authentication Failure";
				if (ui == m_userdb.end())
					oss << " user " << root.get("user", "").asString() << " unknown user";
				else
					oss << " protocol error";
				get_alog().write(websocketpp::log::alevel::app, oss.str());
			}
			return;
		}
		std::string t(ui->second);
		t += chi->second;
		t += root.get("seq", "").asString();
		uint8_t md[SHA256_DIGEST_LENGTH];
		SHA256((const unsigned char *)(t.c_str()), t.size(), md);
		uint8_t md1[SHA256_DIGEST_LENGTH];
		for (unsigned int i = 0; i < SHA256_DIGEST_LENGTH; ++i) {
			std::istringstream is(hash.substr(2 * i, 2));
			unsigned int val;
			is >> std::hex >> val;
			md1[i] = val;
		}
		for (unsigned int i = 0; i < SHA256_DIGEST_LENGTH; ++i) {
			if (md[i] == md1[i])
				continue;
			tx["error"] = "Invalid User/Password";
			transmit(hdl, tx);
			if (true) {
				std::ostringstream oss;
				oss << "Connection Authentication Failure user " << ui->first << " invalid passwd/digest";
				get_alog().write(websocketpp::log::alevel::app, oss.str());
			}
			return;
		}
		if (remove_closed_routers() >= m_connectlimit) {
			tx["error"] = "Server busy, please try again later";
			transmit(hdl, tx);
			if (true) {
				std::ostringstream oss;
				oss << "Connection Authenticated user " << ui->first << " but server busy";
				get_alog().write(websocketpp::log::alevel::app, oss.str());
			}
			return;
		}
		if (count_user_routers(ui->first) >= 1) {
			tx["error"] = "Account is already connected";
			transmit(hdl, tx);
			if (true) {
				std::ostringstream oss;
				oss << "Connection Authenticated user " << ui->first << " but user already connected";
				get_alog().write(websocketpp::log::alevel::app, oss.str());
			}
			return;
		}
		tx["result"] = 1;
		tx["error"] = "Success";
		transmit(hdl, tx);
		server::connection_ptr con(get_con_from_hdl(hdl));
                con->set_message_handler(bind(&Server::on_message, this, ::_1, ::_2));
		boost::shared_ptr<Router> rtr(new Router(this, m_ioserv, hdl, ui->first));
		m_routers[hdl] = rtr;
		rtr->run(m_xdisplay);
		if (true) {
			std::ostringstream oss;
			oss << "Connection Authenticated user " << ui->first;
			get_alog().write(websocketpp::log::alevel::app, oss.str());
		}
		return;
	}
	{
		Json::Value tx;
		tx["cmd"] = cmd;
		tx["error"] = "Invalid command";
		if (root.isMember("cmdseq"))
			tx["cmdseq"] = root.get("cmdseq", "").asString();
		transmit(hdl, tx);
	}
}

void Server::on_message(websocketpp::connection_hdl hdl, server::message_ptr msg)
{
	Json::Value root(receive(msg));
	std::string cmd(root.get("cmd", "error").asString());
	if (root.isNull() || !root.isMember("cmd")) {
		Json::Value tx;
		tx["cmd"] = cmd;
		tx["error"] = "Invalid command";
		if (root.isMember("cmdseq"))
			tx["cmdseq"] = root.get("cmdseq", "").asString();
		transmit(hdl, tx);
		return;
	}
	routers_t::const_iterator ri(m_routers.find(hdl));
	if (ri == m_routers.end() || !ri->second->is_running()) {
		remove_closed_routers();
		Json::Value tx;
		tx["cmd"] = cmd;
		tx["error"] = "Router Process not running";
		if (root.isMember("cmdseq"))
			tx["cmdseq"] = root.get("cmdseq", "").asString();
		transmit(hdl, tx);
		return;
	}
	if (cmd == "aircraft") {
		if (root.isMember("file"))
			root.removeMember("file");
		if (root.isMember("template")) {
			std::string reg(root.get("template", "").asString());
			std::string::iterator i(reg.begin());
			while (i != reg.end()) {
				if (std::isalnum(*i)) {
					++i;
					continue;
				}
				i = reg.erase(i);
			}
			reg = (PACKAGE_DATA_DIR "/aircraft/") + reg + ".xml";
			root["file"] = reg;
		}
	}
	ri->second->sendcmd(Command(root));
}

void Server::set_xdisplay(int disp)
{
	if (m_xpid != (pid_t)-1)
		return;
	m_xdisplay = disp;
}

void Server::xserver_sigchld(const boost::system::error_code& error, int signal_number)
{
	if (error)
		return;
	for (pidset_t::iterator pi(m_reappids.begin()), pe(m_reappids.end()); pi != pe; ) {
		pidset_t::iterator pi2(pi);
		++pi;
		int status(0);
		pid_t pid(waitpid(*pi2, &status, WNOHANG));
		if (pid != *pi2)
			continue;
		m_reappids.erase(pi2);
	}
	if (m_xpid == (pid_t)-1)
		return;
	int status(0);
	pid_t pid(waitpid(m_xpid, &status, WNOHANG));
	if (pid != m_xpid)
		return;
	m_xpid = -1;
	get_alog().write(websocketpp::log::alevel::app, "X server died");
}

void Server::start_xdisplay(void)
{
	if (m_xpid != (pid_t)-1 || m_xdisplay < 0)
		return;
	m_xsigchld.async_wait(bind(&Server::xserver_sigchld, this, ::_1, ::_2));
	m_ioserv.notify_fork(boost::asio::io_service::fork_prepare);
	m_xpid = fork();
	switch (m_xpid) {
	case -1:
		m_ioserv.notify_fork(boost::asio::io_service::fork_parent);
		return;

	case 0:
		m_ioserv.notify_fork(boost::asio::io_service::fork_child);
		{
			long fd(sysconf(_SC_OPEN_MAX));
			while (fd > 0) {
				--fd;
				::close(fd);
			}
		}
		open("/dev/null", O_WRONLY);
		open("/dev/null", O_WRONLY);
		open("/dev/null", O_WRONLY);
		chdir("/tmp");
		{
			std::ostringstream disp;
			disp << ':' << m_xdisplay;
			char *argv[6] = { strdupa("/usr/bin/Xvfb"), strdupa(disp.str().c_str()), strdupa("-screen"), strdupa("0"), strdupa("100x100x16"), 0 };
			char *envp[2] = { strdupa("PATH=/bin:/usr/bin"), 0 };
			execve(argv[0], argv, envp);
		}
		exit(EX_UNAVAILABLE);
		break;

	default:
		break;
	}
	m_ioserv.notify_fork(boost::asio::io_service::fork_parent);
}

int main(int argc, char *argv[])
{
	boost::asio::io_service ioserv;
	Server srv(ioserv);
	srv.clear_access_channels(websocketpp::log::alevel::all);
	uint16_t port(9002);
	{
		std::string userfile(PACKAGE_SYSCONF_DIR "/cfmuwsusers.json");
		static struct option long_options[] = {
			{ "userfile", required_argument, 0, 'u' },
			{ "userdb", required_argument, 0, 'u' },
			{ "add", no_argument, 0, 'a' },
			{ "delete", no_argument, 0, 'd' },
			{ "log", no_argument, 0, 'l' },
			{ "log-aggressive", no_argument, 0, 'L' },
			{ "user", required_argument, 0, 'U' },
			{ "group", required_argument, 0, 'G' },
			{ "port", required_argument, 0, 'p' },
			{ "timeout", required_argument, 0, 't' },
			{0, 0, 0, 0}
		};
		int c, err(0);
		typedef enum {
			mode_normal,
			mode_add,
			mode_delete
		} mode_t;
		mode_t mode(mode_normal);
		uid_t uid(0);
		gid_t gid(0);

		while ((c = getopt_long(argc, argv, "u:adlLU:G:p:x:t:", long_options, 0)) != EOF) {
			switch (c) {
                        case 'u':
                                if (optarg)
                                        userfile = optarg;
                                break;

                        case 'a':
                                mode = mode_add;
                                break;

                        case 'd':
                                mode = mode_delete;
                                break;

			case 'l':
				srv.set_access_channels(websocketpp::log::alevel::connect |
							websocketpp::log::alevel::disconnect |
							websocketpp::log::alevel::app);
				break;

			case 'L':
				srv.set_access_channels(websocketpp::log::alevel::connect |
							websocketpp::log::alevel::disconnect |
							websocketpp::log::alevel::control |
							websocketpp::log::alevel::frame_header |
							websocketpp::log::alevel::frame_payload |
							websocketpp::log::alevel::app);
				break;

			case 'U':
				if (optarg) {
					char *cp;
					uid_t uid1(strtoul(optarg, &cp, 0));
					if (*cp) {
						struct passwd *pw = getpwnam(optarg);
						if (pw) {
							uid = pw->pw_uid;
							gid = pw->pw_gid;
						} else {
							std::cerr << "cfmuws: user " << optarg << " not found" << std::endl;
						}
					} else {
						uid = uid1;
					}
				}
				break;

			case 'G':
				if (optarg) {
					char *cp;
					uid_t gid1(strtoul(optarg, &cp, 0));
					if (*cp) {
						struct group *gr = getgrnam(optarg);
						if (gr) {
							gid = gr->gr_gid;
						} else {
							std::cerr << "cfmuws: group " << optarg << " not found" << std::endl;
						}
					} else {
						gid = gid1;
					}
				}
				break;

			case 'p':
				if (optarg)
					port = strtoul(optarg, 0, 0);
				break;

			case 'x':
				if (optarg)
					srv.set_xdisplay(strtol(optarg, 0, 0));
				break;

			case 'c':
				if (optarg)
					srv.set_connectlimit(strtoul(optarg, 0, 0));
				break;

			case 't':
				if (optarg)
					srv.set_timeout(strtoul(optarg, 0, 0));
				break;

                        default:
                                ++err;
                                break;
			}
		}
		if (err) {
			std::cerr << "usage: cfmuws [-c <connlimit>] [-u <userfile>] [-U <user>] [-G <group>] [-x <xdisplay>] [-t <timeout>] [-a] [-d] [-l] [-L]" << std::endl;
			return EX_USAGE;
		}
		if (gid && setgid(gid))
			std::cerr << "cfmuws: cannot set gid to " << gid << ": " << strerror(errno) << " (" << errno << ')' << std::endl;
		if (uid && setuid(uid))
			std::cerr << "cfmuws: cannot set uid to " << uid << ": " << strerror(errno) << " (" << errno << ')' << std::endl;
       		Json::Value users;
		{
			std::ifstream f(userfile.c_str());
			if (!f.is_open()) {
				std::cerr << "Cannot open users file " << userfile << std::endl;
			} else {
				Json::Reader reader;
				if (!reader.parse(f, users)) {
					std::cerr << "Cannot parse users file: " << reader.getFormattedErrorMessages() << std::endl;
				} else if (!users.empty() && !users.isArray()) {
					users.clear();
					std::cerr << "Invalid users file " << userfile << std::endl;
				}
			}
		}
		bool save(false);
		switch (mode) {
		case mode_add:
			for (int i = optind + 1; i < argc; i += 2) {
				std::string user(argv[i-1]), passwd(argv[i]);
				if (user.empty())
					continue;
				for (Json::Value::ArrayIndex ai(0); ai < users.size(); ++ai) {
					Json::Value& v(users[ai]);
					if (v.get("user", "").asString() != user)
						continue;
					v["user"] = user;
					v["passwd"] = passwd;
					user.clear();
					save = true;
					break;
				}
				if (user.empty())
					continue;
				Json::Value u;
				u["user"] = user;
				u["passwd"] = passwd;
				users.append(u);
				save = true;
			}
			break;

		case mode_delete:
			for (int i = optind; i < argc; ++i) {
				std::string user(argv[i]);
				for (Json::Value::ArrayIndex ai(0); ai < users.size(); ++ai) {
					const Json::Value& v(users[ai]);
					if (v.get("user", "").asString() != user)
						continue;
					Json::Value uf;
					for (Json::Value::ArrayIndex ai2(0); ai2 < users.size(); ++ai2)
						if (ai != ai2)
							uf.append(users[ai2]);
					users.swap(uf);
					save = true;
					break;
				}
			}
			break;

		default:
			break;
		}
		if (save) {
			std::ofstream f(userfile.c_str());
			Json::StyledStreamWriter writer;
			writer.write(f, users);
		}
		if (mode != mode_normal)
			return EX_OK;
		for (Json::Value::ArrayIndex ai(0); ai < users.size(); ++ai) {
			const Json::Value& v(users[ai]);
			if (!v.isMember("user"))
				continue;
			srv.add_user(v.get("user", "").asString(), v.get("passwd", "").asString());
		}
	}

	/* Cancel certain signals */
	signal(SIGTSTP, SIG_IGN); /* Various TTY signals */
	signal(SIGTTOU, SIG_IGN);
	signal(SIGTTIN, SIG_IGN);
	signal(SIGHUP,  SIG_IGN); /* Ignore hangup signal */
	signal(SIGTERM, SIG_DFL); /* Die on SIGTERM */

	/* Create a new SID for the child process */
	if (getpgrp() != getpid() && setsid() == (pid_t)-1)
		std::cerr << "cfmuws: cannot create new session: " << strerror(errno) << " (" << errno << ')' << std::endl;

	/* Change the file mode mask */
	umask(0);
	/* Change the current working directory.  This prevents the current
	   directory from being locked; hence not being able to remove it. */
	if (chdir("/tmp") < 0)
		std::cerr << "cfmuws: cannot change directory to /tmp: " << strerror(errno) << " (" << errno << ')' << std::endl;

	srv.start_xdisplay();
	srv.init_asio(&ioserv);
	srv.listen(port);
	srv.start_accept();
	srv.run();
	return EX_OK;
}
