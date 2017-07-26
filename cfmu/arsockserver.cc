#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <sys/types.h>
#include <sys/socket.h>
#include <sys/wait.h>
#include <fstream>
#include <sstream>
#include <iostream>
#include <iomanip>
#include <errno.h>

#ifdef HAVE_SYS_PRCTL_H
#include <sys/prctl.h>
#endif

#ifdef HAVE_SYSTEMD_SD_DAEMON_H
#include <systemd/sd-daemon.h>
#endif

#include <boost/uuid/string_generator.hpp>

#include "cfmuautoroute45.hh"
#include "cfmuautoroute51.hh"
#include "arsockserver.hh"
#include "airdata.h"
#include "icaofpl.h"
#include "wmm.h"

SocketServer::Client::Client(SocketServer *server, const Glib::RefPtr<Gio::SocketConnection>& connection, bool log)
: m_server(server), m_connection(connection), m_refcount(1), m_closeprotect(0), m_firstreply(true), m_shutdown(false), m_log(log)
{
	if (true)
		std::cout << "client create " << (unsigned long long)this << " pid " << getpid() << std::endl;
	if (!m_connection || !m_server) {
		m_server = 0;
		m_connection.reset();
		return;
	}
	m_connection->get_socket()->property_blocking() = false;
	m_connin = Glib::signal_io().connect(sigc::mem_fun(*this, &Client::on_input), m_connection->get_socket()->get_fd(), Glib::IO_IN | Glib::IO_ERR | Glib::IO_HUP);
	m_server->add_client(get_ptr());
	if (true)
		std::cout << "client connect " << (unsigned long long)this << " pid " << getpid() << " fd " << m_connection->get_socket()->get_fd() << std::endl;
}

SocketServer::Client::~Client()
{
	close();
	if (true)
		std::cout << "client destroy " << (unsigned long long)this << " pid " << getpid() << std::endl;
}

void SocketServer::Client::reference(void) const
{
        g_atomic_int_inc(&m_refcount);
	if (false)
		std::cout << "client " << (unsigned long long)this << " pid " << getpid() << " reference " << m_refcount << std::endl;
}

void SocketServer::Client::unreference(void) const
{
	bool nz(!g_atomic_int_dec_and_test(&m_refcount));
	if (false)
		std::cout << "client " << (unsigned long long)this << " pid " << getpid() << " unreference " << m_refcount << std::endl;
        if (nz)
                return;
        delete this;
}

void SocketServer::Client::close(void)
{
	m_rxbuf.clear();
	m_connin.disconnect();
	m_connout.disconnect();
	m_sendqueue.clear();
	if (m_longpoll)
		m_longpoll->unset_longpoll(get_ptr());
	m_longpoll.reset();
	m_logfilter.clear();
	m_logdiscard.clear();
	if (!m_connection)
		return;
	if (true)
		std::cout << "client " << (unsigned long long)this << " pid " << getpid()
			  << " close fd " << m_connection->get_socket()->get_fd() << std::endl;
	m_connection->get_socket()->close();
	m_connection.reset();
	if (m_closeprotect == 1)
		m_closeprotect = 2;
	else if (m_server)
		m_server->remove_client(get_ptr());
}

void SocketServer::Client::shutdown(void)
{
	m_shutdown = true;
	if (!m_connection) {
		close();
		return;
	}
	m_rxbuf.clear();
	m_connin.disconnect();
	m_connin = Glib::signal_io().connect(sigc::mem_fun(*this, &Client::on_input_eof), m_connection->get_socket()->get_fd(), Glib::IO_IN | Glib::IO_ERR | Glib::IO_HUP);
	if (!m_sendqueue.empty())
		return;
	m_connout.disconnect();
	if (m_longpoll)
		m_longpoll->unset_longpoll(get_ptr());
	m_longpoll.reset();
	if (true)
		std::cout << "client " << (unsigned long long)this << " pid " << getpid() << " shutdown fd " << m_connection->get_socket()->get_fd() << std::endl;
	m_connection->get_socket()->shutdown(false, true);
}

bool SocketServer::Client::on_input_eof(Glib::IOCondition iocond)
{
	try {
		char rxbuf[16384];
		gssize r(m_connection->get_socket()->receive(rxbuf, sizeof(rxbuf)));
		if (r <= 0) {
			if (true)
				std::cerr << "client " << (unsigned long long)this << " pid " << getpid() << " socket input return value: " << r << std::endl;
			close();
			return true;
		}
		if (m_log)
			std::cout << "client " << (unsigned long long)this  << " pid " << getpid()<< " discarding rx " << std::string(rxbuf, rxbuf + r) << std::endl;
	} catch (const Gio::Error& error) {
		if (error.code() == Gio::Error::WOULD_BLOCK) {
			if (true)
				std::cerr << "client " << (unsigned long long)this << " pid " << getpid() << " socket input would block" << std::endl;
			return true;
		} else {
			if (true)
				std::cerr << "client " << (unsigned long long)this << " pid " << getpid() << " socket input error: " << error.what() << std::endl;
			close();
			return true;
		}
	}
	return true;
}

void SocketServer::Client::send(const Json::Value& v)
{
	if (!m_connection)
		return;
	m_sendqueue.push_back(v);
	m_connout.disconnect();
	m_connout = Glib::signal_io().connect(sigc::mem_fun(*this, &Client::on_output), m_connection->get_socket()->get_fd(), Glib::IO_OUT | Glib::IO_ERR);
	m_firstreply = false;
}

int SocketServer::decode_json(Json::Value& root, std::string& rxbuf)
{
	std::string::size_type n(0);
	while (n < rxbuf.size()) {
		char delimo(rxbuf[n]);
		if (std::isspace(delimo) || delimo == '\r' || delimo == '\n') {
			++n;
			continue;
		}
		if (delimo == '{' || delimo == '[') {
			char delimc((delimo == '[') ? ']' : '}');
			int quote(0);
			std::string::size_type n1(n + 1);
			unsigned int delimcnt(1);
			bool nobksp(true);
			while (n1 < rxbuf.size()) {
				char ch(rxbuf[n1]);
				++n1;
				if (nobksp && ch == '\\') {
					nobksp = false;
					continue;
				}
				if (nobksp && ch == '"') {
					if (quote == 1)
						quote = 0;
					else
						quote = 1;
					continue;
				}
				if (nobksp && ch == '\'') {
					if (quote == 2)
						quote = 0;
					else
						quote = 2;
					continue;
				}
				nobksp = true;
				if (quote)
					continue;
				if (ch == delimo) {
					++delimcnt;
					continue;
				}
				if (ch != delimc)
					continue;
				--delimcnt;
				if (delimcnt)
					continue;
				Json::Reader reader;
				bool ok(reader.parse(&rxbuf[n], &rxbuf[n1], root, false));
				rxbuf.erase(0, n1);
				return ok ? n1 : -1;
			}
			break;
		}
		if (rxbuf.compare(n, 5, "size:")) {
			++n;
			continue;
		}
		std::string::size_type n1(rxbuf.find_first_of("\r\n", n));
		if (n1 == std::string::npos)
			break;
		unsigned int sz;
		{
			std::istringstream iss(rxbuf.substr(n, n1 - n));
			iss >> sz;
		}
		++n1;
		if (!sz || sz > 65536) {
			n = n1;
			continue;
		}
		while (n1 < rxbuf.size() && (rxbuf[n1] == '\n' || rxbuf[n1] == '\r'))
			++n1;
		if (rxbuf.size() < n1 + sz)
			break;
		Json::Reader reader;
		bool ok(reader.parse(&rxbuf[n1], &rxbuf[n1 + sz], root, false));
		rxbuf.erase(0, n1 + sz);
		return ok ? n1 + sz : -1;
	}
	rxbuf.erase(0, n);
	return 0;
}

bool SocketServer::Client::on_output(Glib::IOCondition iocond)
{
	m_connout.disconnect();
	if (m_txbuf.empty()) {
		if (m_sendqueue.empty())
			return true;
		Json::FastWriter writer;
		std::ostringstream msg;
		{
			std::string msg1(writer.write(*m_sendqueue.begin()));
			msg << "size: " << msg1.size() << '\n' << msg1 << '\n';
		}
		m_txbuf = msg.str();
		m_sendqueue.erase(m_sendqueue.begin());
	}
	try {
		gssize r(m_connection->get_socket()->send(m_txbuf.c_str(), m_txbuf.size()));
		if (r == -1) {
			if (true)
				std::cerr << "client " << (unsigned long long)this << " pid " << getpid() << " socket output return value: " << r << std::endl;
			close();
			return true;
		}
		if (r >= 0)
			m_txbuf.erase(0, r);
		if (m_txbuf.empty() && m_sendqueue.empty() && m_shutdown)
			shutdown();
		if (m_log)
			std::cout << "client " << (unsigned long long)this << " pid " << getpid() << " tx " << m_txbuf << std::endl;
	} catch (const Gio::Error& error) {
		if (error.code() == Gio::Error::WOULD_BLOCK) {
			if (true)
				std::cerr << "client " << (unsigned long long)this << " pid " << getpid() << " socket output would block" << std::endl;
		} else {
			if (true)
				std::cerr << "client " << (unsigned long long)this << " pid " << getpid() << " socket output error: " << error.what() << std::endl;
			close();
			return true;
		}
	}
	if (!m_txbuf.empty() || !m_sendqueue.empty())
		m_connout = Glib::signal_io().connect(sigc::mem_fun(*this, &Client::on_output), m_connection->get_socket()->get_fd(), Glib::IO_OUT | Glib::IO_ERR);
	return true;
}

bool SocketServer::Client::on_input(Glib::IOCondition iocond)
{
	if (!m_connection) {
		m_connin.disconnect();
		return true;
	}
	if (!m_server)
		return false;
	if (m_server->is_sockclient()) {
		if (true)
			std::cerr << "client " << (unsigned long long)this << " pid " << getpid() << " receive in socket client" << std::endl;
		close();
		return false;
	}
	Json::Value root;
	Json::Value reply;
	if (m_firstreply) {
		reply["version"] = VERSION;
		reply["provider"] = "cfmu";
	}
	sockcli_timestamp(reply);
	try {
		std::string::size_type sz(m_rxbuf.size());
		m_rxbuf.resize(sz + 16384);
		gssize r(m_connection->get_socket()->receive(&m_rxbuf[sz], m_rxbuf.size() - sz));
		if (r <= 0) {
			if (true)
				std::cerr << "client " << (unsigned long long)this << " pid " << getpid() << " socket input return value: " << r << std::endl;
			m_rxbuf.clear();
			close();
			return true;
		}
		if (r >= m_rxbuf.size() - sz) {
			if (true)
				std::cerr << "client " << (unsigned long long)this << " pid " << getpid() << " socket input return value: " << r << std::endl;
			reply["error"] = std::string("invalid message size");
			send(reply);
			m_rxbuf.clear();
			return true;
		}
		m_rxbuf.resize(sz + r);
		std::string rxbufcopy;
		if (m_log)
			rxbufcopy = m_rxbuf;
		int d(decode_json(root, m_rxbuf));
		if (d < 0) {
			if (true)
				std::cerr << "client " << (unsigned long long)this << " pid " << getpid() << " cannot parse JSON: " << m_rxbuf.substr(sz);
			reply["error"] = std::string("cannot parse JSON");
			send(reply);
			return true;
		}
		if (!d)
			return true;
		if (m_log)
			std::cout << "client " << (unsigned long long)this  << " pid " << getpid()<< " rx " << rxbufcopy.substr(0, d) << std::endl;
	} catch (const Gio::Error& error) {
		if (error.code() == Gio::Error::WOULD_BLOCK) {
			if (true)
				std::cerr << "client " << (unsigned long long)this << " pid " << getpid() << " socket input would block" << std::endl;
			return true;
		} else {
			if (true)
				std::cerr << "client " << (unsigned long long)this << " pid " << getpid() << " socket input error: " << error.what() << std::endl;
			close();
			return true;
		}
	}
	if (m_longpoll)
		m_longpoll->unset_longpoll(get_ptr());
	m_longpoll.reset();
	m_logfilter.clear();
	m_logdiscard.clear();
	if (root.isMember("logfilter")) {
		const Json::Value& filters(root["logfilter"]);
		if (filters.isArray()) {
			for (Json::ArrayIndex i = 0; i < filters.size(); ++i) {
				const Json::Value& filter(filters[i]);
				if (!filter.isString())
					continue;
				std::string s(filter.asString());
				if (s.empty())
					continue;
				m_logfilter.insert(s);
			}
		}
	}
	if (root.isMember("logdiscard")) {
		const Json::Value& filters(root["logdiscard"]);
		if (filters.isArray()) {
			for (Json::ArrayIndex i = 0; i < filters.size(); ++i) {
				const Json::Value& filter(filters[i]);
				if (!filter.isString())
					continue;
				std::string s(filter.asString());
				if (s.empty())
					continue;
				m_logdiscard.insert(s);
			}
		}
	}
	bool quit;
	{
		std::string sess;
		if (root.isMember("session"))
                        sess = root.get("session", "").asString();
		if (sess.empty()) {
			if (root.isMember("cmds")) {
				const Json::Value& cmds(root["cmds"]);
				if (cmds.isObject()) {
					Json::Value& replycmds(reply["cmds"]);
					bool answer(!cmds.isMember("reply") || !cmds["reply"].isBool() || cmds["reply"].asBool());
					Json::Value cmdout(m_server->servercmd(cmds));
					if (cmdout.isNull()) {
						cmdout["error"] = std::string("invalid command");
						answer = true;
						if (cmds.isMember("cmdseq"))
							cmdout["cmdseq"] = cmds["cmdseq"];
						if (cmds.isMember("cmdname"))
							cmdout["cmdname"] = cmds["cmdname"];
					}
					if (answer) {
						sockcli_timestamp(cmdout);
						replycmds.append(cmdout);
					}
				} else if (cmds.isArray()) {
					Json::Value& replycmds(reply["cmds"]);
					for (Json::ArrayIndex i = 0; i < cmds.size(); ++i) {
						const Json::Value& cmd(cmds[i]);
						if (!cmd.isObject())
							continue;
						bool answer(!cmd.isMember("reply") || !cmd["reply"].isBool() || cmd["reply"].asBool());
						Json::Value cmdout(m_server->servercmd(cmd));
						if (cmdout.isNull()) {
							cmdout["error"] = std::string("invalid command");
							answer = true;
							if (cmd.isMember("cmdseq"))
								cmdout["cmdseq"] = cmd["cmdseq"];
							if (cmd.isMember("cmdname"))
								cmdout["cmdname"] = cmd["cmdname"];
						}
						if (answer) {
							sockcli_timestamp(cmdout);
							replycmds.append(cmdout);
						}
					}
				}
			}
			send(reply);
			return true;
		}
		reply["session"] = sess;
		if (!m_server) {
			reply["error"] = std::string("no server");
			send(reply);
			return true;
		}
		quit = root.isMember("quit");
		if (quit) {
			const Json::Value& q(root["quit"]);
			quit = q.isBool() && q.asBool();
		}
		m_closeprotect = 1;
		m_longpoll = m_server->find_router(sess, !quit);
		if (m_closeprotect == 2) {
			m_closeprotect = 0;
			if (m_server)
				m_server->remove_client(get_ptr());
			return false;
		}
		m_closeprotect = 0;
	}
	if (!m_longpoll) {
		if (quit) {
			reply["quit"] = false;
		} else {
			reply["error"] = std::string("server busy");
		}
		send(reply);
		return true;
	}
       	if (root.isMember("cmds")) {
		const Json::Value& cmds(root["cmds"]);
		if (cmds.isObject()) {
			Json::Value cmdout(m_server->servercmd(cmds));
			if (!cmdout.isNull())
				send(cmdout);
			else
				m_longpoll->send(cmds);
		} else if (cmds.isArray()) {
			for (Json::ArrayIndex i = 0; i < cmds.size(); ++i) {
				const Json::Value& cmd(cmds[i]);
				if (!cmd.isObject())
					continue;
				Json::Value cmdout(m_server->servercmd(cmd));
				if (!cmdout.isNull())
					send(cmdout);
				else
					m_longpoll->send(cmd);
			}
		}
	}
	if (!quit && root.isMember("noreply")) {
		const Json::Value& noreply(root["noreply"]);
		if (noreply.isBool() && noreply.asBool()) {
			m_longpoll.reset();
			send(reply);
			return true;
		}
	}
	if (quit)
		m_longpoll->close("quit");
	m_longpoll->receive(reply, m_logfilter, m_logdiscard);
	Json::Value& replycmds(reply["cmds"]);
	if (quit) {
		reply["quit"] = true;
		unsigned int rxsz(m_longpoll->get_recvqueue_size());
		reply["rxqueue"] = rxsz;
		if (!rxsz || (root.isMember("clearrxqueue") && root["clearrxqueue"].isBool() && root["clearrxqueue"].asBool()))
			m_longpoll->zap();
		send(reply);
		return true;
	}
	if (replycmds.size()) {
		send(reply);
		return true;
	}
	m_longpoll->set_longpoll(get_ptr());
	return true;
}

void SocketServer::Client::msgavailable(void)
{
	if (!m_longpoll)
		return;
	Json::Value reply;
	if (m_firstreply) {
		reply["version"] = VERSION;
		reply["provider"] = "cfmu";
	}
	m_longpoll->unset_longpoll(get_ptr());
	reply["session"] = m_longpoll->get_session();
	m_longpoll->receive(reply, m_logfilter, m_logdiscard);
	m_longpoll.reset();
	send(reply);
}

SocketServer::Router::Router(SocketServer *server, pid_t pid, const Glib::RefPtr<Gio::Socket>& sock, const std::string& sess, int xdisplay, bool log)
	: m_server(server), m_socket(sock), m_session(sess), m_refcount(1), m_xdisplay(xdisplay), m_pid(pid), m_lifecycle(lifecycle_run), m_log(log)
{
	m_accesstime.assign_current_time();
	m_starttime = m_accesstime;
	if (true)
		std::cout << "router create " << (unsigned long long)this << " pid " << getpid() << " session " << m_session
			  << " xdisplay " << m_xdisplay << std::endl;
	if (!m_server || !m_socket || m_pid == (pid_t)-1) {
		m_server = 0;
		m_socket.reset();
		m_pid = -1;
		m_lifecycle = lifecycle_dead;
		return;
	}
	m_connin = Glib::signal_io().connect(sigc::mem_fun(*this, &Router::on_input), m_socket->get_fd(), Glib::IO_IN | Glib::IO_ERR | Glib::IO_HUP);
	m_connchildwatch = Glib::signal_child_watch().connect(sigc::mem_fun(*this, &Router::child_watch), m_pid);
	if (true)
		std::cout << "router " << (unsigned long long)this << " pid " << getpid() << " started pid " << m_pid
			  << " fd " << m_socket->get_fd() << std::endl;
}

SocketServer::Router::~Router()
{
	if (true)
		std::cout << "router destroy " << (unsigned long long)this << " session " << m_session << std::endl;
	close();
	m_connchildwatch.disconnect();
	if (m_pid != (pid_t)-1) {
		Glib::signal_child_watch().connect(sigc::ptr_fun(&Router::child_reaper), m_pid);
		m_pid = -1;
	}
}

void SocketServer::Router::reference(void) const
{
        g_atomic_int_inc(&m_refcount);
	if (false)
		std::cout << "router " << (unsigned long long)this << " pid " << getpid() << " reference " << m_refcount << std::endl;
}

void SocketServer::Router::unreference(void) const
{
        bool nz(!g_atomic_int_dec_and_test(&m_refcount));
	if (false)
		std::cout << "router " << (unsigned long long)this << " pid " << getpid() << " unreference " << m_refcount << std::endl;
	if (nz)
                return;
        delete this;
}

bool SocketServer::Router::on_output(Glib::IOCondition iocond)
{
	m_connout.disconnect();
	if (m_sendqueue.empty())
		return true;
	Json::FastWriter writer;
	std::string msg(writer.write(*m_sendqueue.begin()));
	try {
		gssize r(m_socket->send(msg.c_str(), msg.size()));
		if (r == -1) {
			if (true)
				std::cerr << "router " << (unsigned long long)this << " pid " << getpid() << " socket output return value: " << r << std::endl;
			close();
			return true;
		}
		m_sendqueue.erase(m_sendqueue.begin());
		if (m_log)
			std::cout << "router " << (unsigned long long)this << " pid " << getpid() << " tx " << msg << std::endl;
	} catch (const Gio::Error& error) {
		if (error.code () == Gio::Error::WOULD_BLOCK) {
			if (true)
				std::cerr << "router " << (unsigned long long)this << " pid " << getpid() << " socket output would block" << std::endl;
		} else {
			if (true)
				std::cerr << "router " << (unsigned long long)this << " pid " << getpid() << " socket output error: " << error.what() << std::endl;
			close();
			return true;
		}
	}
	if (!m_sendqueue.empty())
		m_connout = Glib::signal_io().connect(sigc::mem_fun(*this, &Router::on_output), m_socket->get_fd(), Glib::IO_OUT | Glib::IO_ERR);
	return true;
}

bool SocketServer::Router::on_input(Glib::IOCondition iocond)
{
	if (!m_socket) {
		m_connin.disconnect();
		return true;
	}
	if (!m_server)
		return false;
	if (m_server->is_sockclient()) {
		if (true)
			std::cerr << "router " << (unsigned long long)this << " pid " << getpid() << " receive in socket client" << std::endl;
		close();
		return false;
	}
	Json::Value root;
	try {
		char buf[262144];
 		gssize r(m_socket->receive(buf, sizeof(buf)));
		if (r <= 0) {
			if (true)
				std::cerr << "router " << (unsigned long long)this << " pid " << getpid() << " socket input return value: " << r << std::endl;
			close();
			return true;
		}
		if (r >= sizeof(buf)) {
			if (true)
				std::cerr << "router " << (unsigned long long)this << " pid " << getpid() << " socket input return value: " << r << std::endl;
			close();
			return true;
		}
		{
			Json::Reader reader;
			if (!reader.parse(buf, buf + r, root, false)) {
				if (true)
					std::cerr << "router " << (unsigned long long)this << " pid " << getpid() << " cannot parse JSON: " << std::string(buf, buf + r);
				close();
				return true;
			}
		}
		if (m_log)
			std::cout << "router " << (unsigned long long)this << " pid " << getpid() << " rx " << std::string(buf, buf + r) << std::endl;
	} catch (const Gio::Error& error) {
		if (error.code () == Gio::Error::WOULD_BLOCK) {
			if (true)
				std::cerr << "router " << (unsigned long long)this << " pid " << getpid() << " socket input would block" << std::endl;
			return true;
		} else {
			if (true)
				std::cerr << "router " << (unsigned long long)this << " pid " << getpid() << " socket input error: " << error.what() << std::endl;
			close();
			return true;
		}
	}
	if (true) {
		Glib::TimeVal tv;
		tv.assign_current_time();
		root["timestampenqueue"] = (std::string)tv.as_iso8601();
	}
	m_recvqueue.push_back(root);
	notify_longpoll();
	return true;
}

void SocketServer::Router::send(const Json::Value& v)
{
	if (!is_running())
		return;
	if (!m_socket)
		return;
	m_accesstime.assign_current_time();
	m_sendqueue.push_back(v);
	if (false) {
		Json::FastWriter writer;
		std::cout << "router " << (unsigned long long)this << " pid " << getpid() << " txqueue " << writer.write(*m_sendqueue.end()) << std::endl;
	}
	m_connout.disconnect();
	m_connout = Glib::signal_io().connect(sigc::mem_fun(*this, &Router::on_output), m_socket->get_fd(), Glib::IO_OUT | Glib::IO_ERR);
}

void SocketServer::Router::receive(Json::Value& reply, const stringset_t& logfilter, const stringset_t& logdiscard)
{
	Json::Value& replycmds(reply["cmds"]);
	unsigned int cnt(0), cntdiscarded(0), cntfiltered(0), cntpending(0);
	for (std::list<Json::Value>::iterator i(m_recvqueue.begin()), e(m_recvqueue.end()); i != e; ) {
		std::list<Json::Value>::iterator ii(i);
		++i;
		if (ii->isMember("cmdname")) {
			const Json::Value& cmdname((*ii)["cmdname"]);
			if (cmdname.isString() && cmdname.asString() == "log" && ii->isMember("item")) {
				const Json::Value& item((*ii)["item"]);
				if (item.isString()) {
					if (logdiscard.find(item.asString()) != logdiscard.end()) {
						m_recvqueue.erase(ii);
						++cntdiscarded;
						continue;
					}
					if (logfilter.find(item.asString()) != logfilter.end()) {
						++cntfiltered;
						continue;
					}
				}
			}
		}
		if (cnt >= 128) {
			++cntpending;
			continue;
		}
		replycmds.append(*ii);
		if (true) {
			Glib::TimeVal tv;
			tv.assign_current_time();
			replycmds[replycmds.size() - 1U]["timestampdequeue"] = (std::string)tv.as_iso8601();
		}
		m_recvqueue.erase(ii);
		++cnt;
	}
	if (cnt)
		m_accesstime.assign_current_time();
	reply["dequeued"] = cnt;
	reply["queuepending"] = cntpending;
	reply["queuediscarded"] = cntdiscarded;
	reply["queuefiltered"] = cntfiltered;
	if (m_lifecycle == lifecycle_status && m_recvqueue.empty()) {
		m_lifecycle = lifecycle_dead;
		if (m_server)
			m_server->reap_died_routers();
	}
}

void SocketServer::Router::close(const std::string& status)
{
	if (true) {
		std::cout << "router " << (unsigned long long)this << " pid " << getpid() << " close pid " << m_pid;
		if (m_socket)
			std::cout << " fd " << m_socket->get_fd();
		std::cout << std::endl;
	}
	m_connin.disconnect();
	m_connout.disconnect();
	m_sendqueue.clear();
	m_socket.reset();
	if (m_pid != (pid_t)-1)
		kill(m_pid, SIGHUP);
	if (m_lifecycle == lifecycle_run) {
		m_lifecycle = lifecycle_status;
		m_accesstime.assign_current_time();
		m_starttime = m_accesstime;
		Json::Value reply;
		reply["cmdname"] = "autoroute";
		reply["status"] = status.empty() ? "terminate" : status;
		m_recvqueue.push_back(reply);
	}
	notify_longpoll();
}

void SocketServer::Router::zap(void)
{
	if (true) {
		std::cout << "router " << (unsigned long long)this << " pid " << getpid() << " zap pid " << m_pid;
		if (m_socket)
			std::cout << " fd " << m_socket->get_fd();
		std::cout << std::endl;
	}
	m_connchildwatch.disconnect();
	m_connin.disconnect();
	m_connout.disconnect();
	m_sendqueue.clear();
	m_recvqueue.clear();
	if (m_pid != (pid_t)-1) {
		Glib::signal_child_watch().connect(sigc::ptr_fun(&Router::child_reaper), m_pid);
		m_pid = -1;
	}
	m_socket.reset();
	m_lifecycle = lifecycle_dead;
	notify_longpoll();
}

void SocketServer::Router::child_watch(GPid pid, int child_status)
{
	if (pid != m_pid || pid == (pid_t)-1) {
		Glib::spawn_close_pid(pid);
		return;
	}
	m_connchildwatch.disconnect();
	Glib::spawn_close_pid(pid);
	if (true)
		std::cout << "router " << (unsigned long long)this << " pid " << getpid() << " reaped pid " << m_pid
			  << " status " << child_status << std::endl;
        m_pid = -1;
	if (m_lifecycle == lifecycle_run) {
		m_lifecycle = lifecycle_status;
		m_accesstime.assign_current_time();
		m_starttime = m_accesstime;
		Json::Value reply;
		reply["cmdname"] = "autoroute";
		reply["status"] = "terminate";
		m_recvqueue.push_back(reply);
	}
	notify_longpoll();
}

void SocketServer::Router::child_reaper(GPid pid, int child_status)
{
	Glib::spawn_close_pid(pid);
}

bool SocketServer::Router::handle_timeout(const Glib::TimeVal& tvacc, const Glib::TimeVal& tvstart)
{
	if (m_lifecycle == lifecycle_dead)
		return false;
	if (tvacc < m_accesstime && tvstart < m_starttime)
		return false;
	if (m_lifecycle == lifecycle_status)
		m_lifecycle = lifecycle_dead;
	else
		close();
	return true;
}

void SocketServer::Router::set_longpoll(const Client::ptr_t& clnt)
{
	if (m_longpoll == clnt)
		return;
	notify_longpoll();
	m_longpoll = clnt;
	if (true)
		std::cout << "router " << (unsigned long long)this << " pid " << getpid() << " set longpoll client "
			  << (unsigned long)m_longpoll.operator->() << std::endl;
}

void SocketServer::Router::unset_longpoll(const Client::ptr_t& clnt)
{
	if (m_longpoll == clnt) {
		if (true)
			std::cout << "router " << (unsigned long long)this << " pid " << getpid() << " unset longpoll client "
				  << (unsigned long)m_longpoll.operator->() << std::endl;
		m_longpoll.reset();
	}
}

void SocketServer::Router::notify_longpoll(void)
{
	if (m_longpoll) {
		if (true)
			std::cout << "router " << (unsigned long long)this << " pid " << getpid() << " notify client "
				  << (unsigned long)m_longpoll.operator->() << std::endl;
		m_longpoll->msgavailable();
	}
	m_longpoll.reset();
}

SocketServer::SocketServer(CFMUAutoroute *autoroute, const Glib::RefPtr<Glib::MainLoop>& mainloop, unsigned int connlimit,
			   unsigned int actlimit, unsigned int timeout, unsigned int maxruntime, int xdisplay,
			   bool logclient, bool logrouter, bool logsockcli, bool setprocname)
	: m_autoroute(autoroute), m_mainloop(mainloop), m_logdir("/tmp/cfmuautoroute"),
	  m_connectlimit(connlimit), m_activelimit(actlimit),
	  m_timeout(timeout), m_maxruntime(maxruntime), m_xdisplay(xdisplay), m_quit(false),
	  m_logclient(logclient), m_logrouter(logrouter), m_logsockcli(logsockcli), m_setprocname(setprocname)
{
	m_cmdlist["nop"] = &SocketServer::cmd_nop;
	m_cmdlist["quit"] = &SocketServer::cmd_quit;
	m_cmdlist["departure"] = &SocketServer::cmd_departure;
	m_cmdlist["destination"] = &SocketServer::cmd_destination;
	m_cmdlist["crossing"] = &SocketServer::cmd_crossing;
	m_cmdlist["enroute"] = &SocketServer::cmd_enroute;
	m_cmdlist["levels"] = &SocketServer::cmd_levels;
	m_cmdlist["exclude"] = &SocketServer::cmd_exclude;
	m_cmdlist["tfr"] = &SocketServer::cmd_tfr;
	m_cmdlist["atmosphere"] = &SocketServer::cmd_atmosphere;
	m_cmdlist["cruise"] = &SocketServer::cmd_cruise;
	m_cmdlist["optimization"] = &SocketServer::cmd_optimization;
	m_cmdlist["preferred"] = &SocketServer::cmd_preferred;
	m_cmdlist["aircraft"] = &SocketServer::cmd_aircraft;
	m_cmdlist["start"] = &SocketServer::cmd_start;
	m_cmdlist["stop"] = &SocketServer::cmd_stop;
	m_cmdlist["continue"] = &SocketServer::cmd_continue;
	m_cmdlist["preload"] = &SocketServer::cmd_preload;
	m_cmdlist["clear"] = &SocketServer::cmd_clear;
	m_cmdlist["fplparse"] = &SocketServer::cmd_fplparse;
	m_cmdlist["fplparseadr"] = &SocketServer::cmd_fplparseadr;
	m_servercmdlist["scangrib2"] = &SocketServer::servercmd_scangrib2;
	m_servercmdlist["reloaddb"] = &SocketServer::servercmd_reloaddb;
	m_servercmdlist["processes"] = &SocketServer::servercmd_processes;
	m_servercmdlist["ruleinfo"] = &SocketServer::servercmd_ruleinfo;
	if (autoroute && !autoroute->get_logprefix().empty()) {
		m_logdir = autoroute->get_logprefix();
		if (m_logdir.size() > 7 && !m_logdir.compare(m_logdir.size() - 7, 7, "-XXXXXX"))
			m_logdir.resize(m_logdir.size() - 7);
	}
}

SocketServer::~SocketServer()
{
	m_rtrtimeoutconn.disconnect();
	m_clients.clear();
	m_routers.clear();
	remove_listensockaddr();
}

void SocketServer::add_client(const Client::ptr_t& p)
{
	if (p)
		m_clients.insert(p);
}

void SocketServer::remove_client(const Client::ptr_t& p)
{
	clients_t::iterator i(m_clients.find(p));
	if (i != m_clients.end())
		m_clients.erase(i);
}

void SocketServer::remove_listensockaddr(void)
{
	if (!m_listensockaddr)
		return;
	Glib::RefPtr<Gio::UnixSocketAddress> sa(Glib::RefPtr<Gio::UnixSocketAddress>::cast_dynamic(m_listensockaddr));
	if (sa && sa->property_address_type() == Gio::UNIX_SOCKET_ADDRESS_PATH) {
		Glib::RefPtr<Gio::File> f(Gio::File::create_for_path(sa->property_path()));
		try {
			f->remove();
		} catch (...) {
		}
	}
	m_listensockaddr.reset();
}

void SocketServer::listen(const std::string& path, uid_t socketuid, gid_t socketgid, mode_t socketmode, bool sdterminate)
{
	m_listenservice.reset();
	m_listensock.reset();
	remove_listensockaddr();
	m_quit = sdterminate;
#ifdef HAVE_SYSTEMDDAEMON
	{
		int n = sd_listen_fds(0);
		if (n > 1)
			throw std::runtime_error("SystemD: too many file descriptors received");
		if (n == 1) {
			int fd = SD_LISTEN_FDS_START + 0;
			m_listensock = Gio::Socket::create_from_fd(fd);
		}
	}
#endif
	if (!m_listensock) {
		bool inet(!path.compare(0, 5, "inet:"));
		bool inet4(!path.compare(0, 6, "inet4:"));
		bool inet6(!path.compare(0, 6, "inet6:"));
		if (inet || inet4 || inet6) {
			std::string::size_type n1(5 + !inet);
			std::string::size_type n2(path.find(':', n1));
			Glib::RefPtr<Gio::InetAddress> addr;
			guint16 port(0);
			if (n2 == std::string::npos) {
				port = strtoul(path.substr(n1).c_str(), 0, 10);
				addr = Gio::InetAddress::create_any(inet6 ? Gio::SOCKET_FAMILY_IPV6 : Gio::SOCKET_FAMILY_IPV4);
			} else {
				port = strtoul(path.substr(n2 + 1).c_str(), 0, 10);
				addr = Gio::InetAddress::create(path.substr(n1, n2 - n1));
			}
			m_listensock = Gio::Socket::create(addr->get_family(), Gio::SOCKET_TYPE_STREAM, Gio::SOCKET_PROTOCOL_DEFAULT);
			m_listensockaddr = Gio::InetSocketAddress::create(addr, port);
		} else {
			m_listensock = Gio::Socket::create(Gio::SOCKET_FAMILY_UNIX, Gio::SOCKET_TYPE_SEQPACKET, Gio::SOCKET_PROTOCOL_DEFAULT);
			m_listensockaddr = Gio::UnixSocketAddress::create(path, Gio::UNIX_SOCKET_ADDRESS_PATH);
			if (true) {
				Glib::RefPtr<Gio::File> f(Gio::File::create_for_path(path));
				try {
					f->remove();
				} catch (...) {
				}
			}
		}
		m_listensock->bind(m_listensockaddr, true);
		m_listensock->listen();
		if (!(inet || inet4 || inet6)) {
			//if (fchown(m_listensock->get_fd(), socketuid, socketgid))
			if (chown(path.c_str(), socketuid, socketgid))
				std::cerr << "Cannot set socket " << path << " uid/gid to " << socketuid << '/' << socketgid
					  << ": " << strerror(errno) << " (" << errno << ')' << std::endl;
			//if (fchmod(m_listensock->get_fd(), socketmode))
			if (chmod(path.c_str(), socketmode))
				std::cerr << "Cannot set socket " << path << " mode to " << std::oct << '0'
					  << (unsigned int)socketmode << std::dec << ": " << strerror(errno) << " (" << errno << ')' << std::endl;
		}
		m_quit = false;
	}
	m_listenservice = Gio::SocketService::create();
	m_listenservice->signal_incoming().connect(sigc::mem_fun(*this, &SocketServer::on_connect));
	if (!m_listenservice->add_socket(m_listensock))
		throw std::runtime_error("Cannot listen to socket");
	m_listenservice->start();
}

bool SocketServer::on_connect(const Glib::RefPtr<Gio::SocketConnection>& connection, const Glib::RefPtr<Glib::Object>& source_object)
{
	Client::ptr_t p(new Client(this, connection, m_logclient));
	return true;
}

void SocketServer::count_routers(unsigned int& total, unsigned int& active)
{
	unsigned int t(0), a(0);
	for (routers_t::iterator i(m_routers.begin()), e(m_routers.end()); i != e;) {
		if (!i->second || i->second->is_dead()) {
			if (true)
				std::cout << "router " << (unsigned long)i->second.operator->() << " session " << i->first << " remove" << std::endl;
			routers_t::iterator ii(i);
			++i;
			m_routers.erase(ii);
			continue;
		}
		++t;
		if (i->second->is_running())
			++a;
		++i;
	}
	if (m_routers.empty())
		m_rtrtimeoutconn.disconnect();
	total = t;
	active = a;
}

SocketServer::Router::ptr_t SocketServer::find_router(const std::string& session, bool create)
{
	if (session.empty())
		return Router::ptr_t();
	if (true) {
		std::cout << "find_router: pid " << getpid() << std::endl;
		for (routers_t::iterator i(m_routers.begin()), e(m_routers.end()); i != e; ++i)
			std::cout << "find_router " << (unsigned long)i->second.operator->() << " session " << i->first << std::endl;
	}
	routers_t::iterator i(m_routers.find(session));
	if (i != m_routers.end()) {
		if (i->second && !i->second->is_dead())
			return i->second;
		if (true)
			std::cout << "router " << (unsigned long)i->second.operator->() << " session " << i->first << " remove" << std::endl;
		m_routers.erase(i);
		if (m_routers.empty())
			m_rtrtimeoutconn.disconnect();
	}
	if (!create)
		return Router::ptr_t();
	{
		unsigned int total, active;
		count_routers(total, active);
		if (!create || total >= m_connectlimit || active >= m_activelimit)
			return Router::ptr_t();
	}
	// find xdisplay
	int xdisplay(-1);
	if (m_xdisplay >= 0) {
		typedef std::set<int> xdisps_t;
		xdisps_t xdisps;
		for (int i(m_xdisplay), e(m_xdisplay + m_routers.size()); i <= e; ++i)
			xdisps.insert(i);
		for (routers_t::const_iterator i(m_routers.begin()), e(m_routers.end()); i != e; ++i)
			xdisps.erase(i->second->get_xdisplay());
		if (!xdisps.empty())
			xdisplay = *xdisps.begin();
	}
	// create socketpair
	int socks[2];
	if (socketpair(AF_LOCAL, SOCK_SEQPACKET, 0, socks))
		return Router::ptr_t();
	// fork
	pid_t pid = fork();
	if (!pid) {
		// child
		close(socks[0]);
		m_listenservice->stop();
		m_listensockaddr.reset();
		m_listenservice.reset();
		m_listensock.reset();
		m_logdir = Glib::build_filename(m_logdir, session);
		if (true)
			std::cout << "New session " << session << " kill clients" << std::endl;
		for (clients_t::iterator i(m_clients.begin()), e(m_clients.end()); i != e; ) {
			clients_t::iterator i2(i);
			++i;
			(*i2)->close();
		}
		if (true)
			std::cout << "New session " << session << " kill routers" << std::endl;
		while (!m_routers.empty()) {
			m_routers.begin()->second->zap();
			m_routers.erase(m_routers.begin());
		}
		if (true)
			std::cout << "New session " << session << " start sockclient" << std::endl;
		if (m_setprocname) {
#ifdef HAVE_SYS_PRCTL_H
			prctl(PR_SET_NAME, (unsigned long)session.c_str(), 0, 0, 0);
#endif
		}
		sockclient(socks[1], xdisplay);
		return Router::ptr_t();
	}
	close(socks[1]);
	if (pid == -1) {
		close(socks[0]);
		return Router::ptr_t();
	}
	if (m_routers.empty()) {
		m_rtrtimeoutconn.disconnect();
		m_rtrtimeoutconn = Glib::signal_timeout().connect_seconds(sigc::mem_fun(*this, &SocketServer::router_timeout_handler), std::min(std::max(m_timeout / 10U, 1U), 10U));
	}
	Router::ptr_t p(new Router(this, pid, Gio::Socket::create_from_fd(socks[0]), session, xdisplay, m_logrouter));
	m_routers[session] = p;
	if (true) {
		std::cout << "find_router: after creation: pid " << getpid() << std::endl;
		for (routers_t::iterator i(m_routers.begin()), e(m_routers.end()); i != e; ++i)
			std::cout << "find_router " << (unsigned long)i->second.operator->() << " session " << i->first << std::endl;
	}
	return p;
}

bool SocketServer::stop_router(const std::string& session)
{
	Router::ptr_t p(find_router(session, false));
	if (!p)
		return false;
	p->close();
	return true;
}

void SocketServer::reap_died_routers(void)
{
	for (routers_t::iterator i(m_routers.begin()), e(m_routers.end()); i != e;) {
		routers_t::iterator ii(i);
		++i;
		if (ii->second && !ii->second->is_dead())
			continue;
		if (true)
			std::cout << "router " << (unsigned long)ii->second.operator->() << " session " << ii->first << " remove" << std::endl;
		m_routers.erase(ii);
	}
	if (m_routers.empty()) {
		m_rtrtimeoutconn.disconnect();
		if (m_quit)
			m_mainloop->quit();
	}
}

bool SocketServer::router_timeout_handler(void)
{
	Glib::TimeVal tvacc, tvrun;
	tvacc.assign_current_time();
	tvrun = tvacc;
	tvacc.subtract_seconds(m_timeout);
	tvrun.subtract_seconds(m_maxruntime);
	for (routers_t::iterator i(m_routers.begin()), e(m_routers.end()); i != e;) {
		Router::ptr_t p(i->second);
		++i;
		p->handle_timeout(tvacc, tvrun);
	}
	return true;
}

void SocketServer::sockclient(int fd, int xdisplay)
{
	if (!m_autoroute)
		return;
	g_mkdir_with_parents(m_logdir.c_str(), 0750);
	m_autoroute->set_logprefix(m_logdir);
	m_autoroute->set_validator_xdisplay(xdisplay);
	m_listensock = Gio::Socket::create_from_fd(fd);
        m_listensock->property_blocking() = true;
	Glib::signal_io().connect(sigc::mem_fun(*this, &SocketServer::sockcli_input), m_listensock->get_fd(), Glib::IO_IN | Glib::IO_ERR | Glib::IO_HUP);
	m_autoroute->signal_statuschange().clear();
	m_autoroute->signal_log().clear();
	m_autoroute->signal_statuschange().connect(sigc::mem_fun(*this, &SocketServer::sockcli_statuschange));
	m_autoroute->signal_log().connect(sigc::mem_fun(*this, &SocketServer::sockcli_autoroutelog));
	if (m_xdisplay >= 0) {
		std::ostringstream disp;
		disp << ':' << m_xdisplay;
		Glib::setenv("DISPLAY", disp.str(), true);
	}
}

void SocketServer::sockcli_close(void)
{
	m_listensock.reset();
	if (m_mainloop)
		m_mainloop->quit();
}

void SocketServer::sockcli_send(const Json::Value& v)
{
	Json::FastWriter writer;
	std::string msg(writer.write(v));
	try {
		gssize r(m_listensock->send(msg.c_str(), msg.size()));
		if (r == -1) {
			if (true)
				std::cerr << "sockcli " << getpid() << " socket output return value: " << r << std::endl;
			sockcli_close();
			return;
		}
		if (m_logsockcli)
			std::cerr << "sockcli " << getpid() << " tx " << msg << std::endl;
	} catch (const Gio::Error& error) {
		if (true)
			std::cerr << "sockcli " << getpid() << " socket output error: " << error.what() << std::endl;
		sockcli_close();
		return;
	}
}

void SocketServer::sockcli_timestamp(Json::Value& v)
{
	Glib::TimeVal tv;
	tv.assign_current_time();
	v["timestamp"] = (std::string)tv.as_iso8601();
}

bool SocketServer::sockcli_input(Glib::IOCondition iocond)
{
	Json::Value root;
	Json::Value reply;
	try {
		char buf[262144];
 		gssize r(m_listensock->receive(buf, sizeof(buf)));
		if (r <= 0) {
			if (true)
				std::cerr << "sockcli " << getpid() << " socket input return value: " << r << std::endl;
			sockcli_close();
			return true;
		}
		if (r >= sizeof(buf)) {
			if (true)
				std::cerr << "sockcli " << getpid() << " socket input return value: " << r << std::endl;
			reply["error"] = std::string("invalid message size");
			sockcli_timestamp(reply);
			sockcli_send(reply);
			return true;
		}
		{
			Json::Reader reader;
			if (!reader.parse(buf, buf + r, root, false)) {
				if (true)
					std::cerr << "sockcli " << getpid() << " cannot parse JSON: " << std::string(buf, buf + r);
				reply["error"] = std::string("cannot parse JSON");
				sockcli_timestamp(reply);
				sockcli_send(reply);
				return true;
			}
		}
		if (m_logsockcli)
			std::cout << "sockcli " << getpid() << " rx " << std::string(buf, buf + r) << std::endl;
	} catch (const Gio::Error& error) {
		if (error.code () == Gio::Error::WOULD_BLOCK) {
			if (true)
				std::cerr << "sockcli " << getpid() << " socket input would block" << std::endl;
			return true;
		} else {
			if (true)
				std::cerr << "sockcli " << getpid() << " socket input error: " << error.what() << std::endl;
			sockcli_close();
			return true;
		}
	}
	try {
		if (root.isMember("cmdseq"))
			reply["cmdseq"] = root["cmdseq"];
		std::string cmdname(root.get("cmdname", "").asString());
		reply["cmdname"] = cmdname.empty() ? "autoroute" : cmdname;
		Json::Value reply1(reply);
		bool answer(!root.isMember("reply") || !root["reply"].isBool() || root["reply"].asBool());
		cmdlist_t::const_iterator cmdi(m_cmdlist.find(cmdname));
                if (cmdi == m_cmdlist.end()) {
			reply1["error"] = "command not found";
			answer = true;
                } else {
                        (this->*(cmdi->second))(root, reply1);
                }
		if (answer) {
			sockcli_timestamp(reply1);
			sockcli_send(reply1);
		}
        } catch (const std::runtime_error& e) {
		reply["error"] = e.what();
		sockcli_timestamp(reply);
		sockcli_send(reply);
        } catch (const Glib::Exception& e) {
		reply["error"] = (std::string)e.what();
		sockcli_timestamp(reply);
                sockcli_send(reply);
        }
	return true;
}

void SocketServer::sockcli_statuschange(CFMUAutoroute::statusmask_t status)
{
        if (status & (CFMUAutoroute::statusmask_starting |
                      CFMUAutoroute::statusmask_stoppingdone |
                      CFMUAutoroute::statusmask_stoppingerror)) {
		Json::Value reply;
		reply["cmdname"] = "autoroute";
                if (status & CFMUAutoroute::statusmask_starting) {
                        reply["status"] =  "starting";
                } else if (status & (CFMUAutoroute::statusmask_stoppingdone |
                                     CFMUAutoroute::statusmask_stoppingerror)) {
                        reply["status"] = "stopping";
                        reply["routesuccess"] = !!(status & CFMUAutoroute::statusmask_stoppingdone);
			reply["localiteration"] = m_autoroute->get_local_iterationcount();
			reply["remoteiteration"] = m_autoroute->get_remote_iterationcount();
			reply["wallclocktime"] = m_autoroute->get_wallclocktime().as_double();
			reply["validatortime"] = m_autoroute->get_validatortime().as_double();
			reply["siderror"] = !!(status & CFMUAutoroute::statusmask_stoppingerrorsid);
			reply["starerror"] = !!(status & CFMUAutoroute::statusmask_stoppingerrorstar);
			reply["enrouteerror"] = !!(status & CFMUAutoroute::statusmask_stoppingerrorenroute);
			reply["validatorerror"] = !!(status & CFMUAutoroute::statusmask_stoppingerrorvalidatortimeout);
			reply["internalerror"] = !!(status & CFMUAutoroute::statusmask_stoppingerrorinternalerror);
			reply["iterationerror"] = !!(status & CFMUAutoroute::statusmask_stoppingerroriteration);
			reply["userstop"] = !!(status & CFMUAutoroute::statusmask_stoppingerroruser);
                }
		sockcli_timestamp(reply);
                sockcli_send(reply);
        }
}

void SocketServer::sockcli_autoroutelog(CFMUAutoroute::log_t item, const std::string& line)
{
	Json::Value reply;
	if (item != CFMUAutoroute::log_fplproposal) {
		reply["item"] = (std::string)to_str(item);
		reply["text"] = (std::string)line;
		sockcli_timestamp(reply);
		switch (item) {
		case CFMUAutoroute::log_graphrule:
		case CFMUAutoroute::log_graphruledesc:
		case CFMUAutoroute::log_graphruleoprgoal:
		case CFMUAutoroute::log_graphchange:
		{
			std::ofstream os(Glib::build_filename(m_logdir, "graph.log").c_str(),
					 std::ofstream::out | std::ofstream::app);
			if (!os.is_open() || !os.good())
				break;
			Json::FastWriter writer;
			os << writer.write(reply);
			return;
		}

		case CFMUAutoroute::log_weatherdata:
		case CFMUAutoroute::log_normal:
		case CFMUAutoroute::log_debug0:
		case CFMUAutoroute::log_debug1:
		{
			std::ofstream os(Glib::build_filename(m_logdir, "message.log").c_str(),
					 std::ofstream::out | std::ofstream::app);
			if (!os.is_open() || !os.good())
				break;
			Json::FastWriter writer;
			os << writer.write(reply);
			return;
		}

		case CFMUAutoroute::log_fpllocalvalidation:
		case CFMUAutoroute::log_fplremotevalidation:
			if (m_autoroute) {
				const FPlanRoute& route(m_autoroute->get_route());
				encode_fpl(reply, route);
				reply["mintime"] = m_autoroute->get_mintime(true);
				reply["routetime"] = m_autoroute->get_routetime(true);
				reply["minfuel"] = m_autoroute->get_minfuel(true);
				reply["routefuel"] = m_autoroute->get_routefuel(true);
				reply["mintimezerowind"] = m_autoroute->get_mintime(false);
				reply["routetimezerowind"] = m_autoroute->get_routetime(false);
				reply["minfuelzerowind"] = m_autoroute->get_minfuel(false);
				reply["routefuelzerowind"] = m_autoroute->get_routefuel(false);
				reply["fpl"] = (std::string)m_autoroute->get_simplefpl();
				reply["iteration"] = m_autoroute->get_iterationcount();
				reply["localiteration"] = m_autoroute->get_local_iterationcount();
				reply["remoteiteration"] = m_autoroute->get_remote_iterationcount();
				reply["wallclocktime"] = m_autoroute->get_wallclocktime().as_double();
				reply["validatortime"] = m_autoroute->get_validatortime().as_double();
			}
			break;

		default:
			break;
		}
	        reply["cmdname"] = "log";
		sockcli_send(reply);
		return;
	}
	// new flight plan
	reply["cmdname"] = "fpl";
	if (m_autoroute) {
		const FPlanRoute& route(m_autoroute->get_route());
		encode_fpl(reply, route);
		reply["mintime"] = m_autoroute->get_mintime(true);
		reply["routetime"] = m_autoroute->get_routetime(true);
		reply["minfuel"] = m_autoroute->get_minfuel(true);
		reply["routefuel"] = m_autoroute->get_routefuel(true);
		reply["mintimezerowind"] = m_autoroute->get_mintime(false);
		reply["routetimezerowind"] = m_autoroute->get_routetime(false);
		reply["minfuelzerowind"] = m_autoroute->get_minfuel(false);
		reply["routefuelzerowind"] = m_autoroute->get_routefuel(false);
		reply["fpl"] = (std::string)m_autoroute->get_simplefpl();
		reply["iteration"] = m_autoroute->get_iterationcount();
		reply["localiteration"] = m_autoroute->get_local_iterationcount();
		reply["remoteiteration"] = m_autoroute->get_remote_iterationcount();
		reply["wallclocktime"] = m_autoroute->get_wallclocktime().as_double();
		reply["validatortime"] = m_autoroute->get_validatortime().as_double();
	} else {
		reply["error"] = "no autoroute instance";
	}
	sockcli_timestamp(reply);
	sockcli_send(reply);
}

Point SocketServer::get_point(const Json::Value& cmd, const std::string& name)
{
	Point pt;
	pt.set_invalid();
	std::string s;
	if (cmd.isMember(name))
		s = cmd.get(name, "").asString();
	std::string lat, lon;
	if (s.empty()) {
		{
			const Json::Value& lat(cmd[name + "lat"]);
			const Json::Value& lon(cmd[name + "lon"]);
			if (lat.isIntegral() && lon.isIntegral()) {
				pt.set_lat(lat.asInt());
				pt.set_lon(lon.asInt());
				return pt;
			}
		}
		{
			const Json::Value& lat(cmd[name + "latdeg"]);
			const Json::Value& lon(cmd[name + "londeg"]);
			if (lat.isNumeric() && lon.isNumeric()) {
				pt.set_lat_deg_dbl(lat.asDouble());
				pt.set_lon_deg_dbl(lon.asDouble());
				return pt;
			}
		}
		lat = cmd.get(name + "latstr", "").asString();
		lon = cmd.get(name + "lonstr", "").asString();
		unsigned int x(pt.set_str(lat) | pt.set_str(lon));
		if (!(x & ~(Point::setstr_lat | Point::setstr_lon)))
			return pt;
		pt.set_invalid();
		return pt;
	} else {
		std::string::size_type n(s.find(','));
		if (n == std::string::npos)
			return pt;
		lat = s.substr(0, n);
		lon = s.substr(n + 1);
	}
	if (lat.empty() || lon.empty())
		return pt;
	char *cp1(0), *cp2(0);
	pt.set_lat(strtol(lat.c_str(), &cp1, 10));
	pt.set_lon(strtol(lon.c_str(), &cp2, 10));
	if (*cp1 || *cp2)
		pt.set_invalid();
	return pt;
}

void SocketServer::set_point(Json::Value& cmd, const std::string& name, const Point& pt)
{
	std::ostringstream oss;
	if (!pt.is_invalid()) {
		oss << pt.get_lat() << ',' << pt.get_lon();
		cmd[name + "lat"] = pt.get_lat();
		cmd[name + "lon"] = pt.get_lon();
		cmd[name + "latdeg"] = pt.get_lat_deg_dbl();
		cmd[name + "londeg"] = pt.get_lon_deg_dbl();
		cmd[name + "latstr"] = (std::string)pt.get_lat_str2();
		cmd[name + "lonstr"] = (std::string)pt.get_lon_str2();
	}
	cmd[name] = oss.str();
}

void SocketServer::set_double(Json::Value& cmd, const std::string& name, double x)
{
	if (std::isnan(x) || std::isinf(x))
		return;
	cmd[name] = x;
}

void SocketServer::encode_wpt(Json::Value& fplwpt, const FPlanWaypoint& wpt)
{
	fplwpt["icao"] = (std::string)wpt.get_icao();
	fplwpt["name"] = (std::string)wpt.get_name();
	{
		unsigned int nr, tm;
		if (wpt.is_stay(nr, tm)) {
			fplwpt["staynr"] = (Json::LargestInt)(nr + 1);
			fplwpt["staytime"] = (Json::LargestInt)tm;
		} else {
			fplwpt["pathname"] = (std::string)wpt.get_pathname();
			fplwpt["pathcode"] = (std::string)wpt.get_pathcode_name();
		}
	}
	fplwpt["note"] = (std::string)wpt.get_note();
	fplwpt["time"] = (Json::LargestInt)wpt.get_time_unix();
	fplwpt["save_time"] = (Json::LargestInt)wpt.get_save_time_unix();
	fplwpt["flighttime"] = (Json::LargestInt)wpt.get_flighttime();
	fplwpt["frequency"] = wpt.get_frequency();
	set_point(fplwpt, "coord", wpt.get_coord());
	if (wpt.get_altitude() != std::numeric_limits<int32_t>::min())
		fplwpt["altitude"] = wpt.get_altitude();
	fplwpt["flags"] = wpt.get_flags();
	fplwpt["flightrules"] = wpt.is_ifr() ? "ifr" : "vfr";
	fplwpt["altmode"] = wpt.is_standard() ? "std" : "qnh";
	fplwpt["climb"] = wpt.is_climb();
	fplwpt["descent"] = wpt.is_descent();
	fplwpt["turnpoint"] = wpt.is_turnpoint();
	fplwpt["winddir"] = wpt.get_winddir_deg();
	fplwpt["windspeed"] = wpt.get_windspeed_kts();
	fplwpt["qff"] = wpt.get_qff_hpa();
	fplwpt["isaoffset"] = wpt.get_isaoffset_kelvin();
	fplwpt["truealt"] = wpt.get_truealt();
	fplwpt["truetrack"] = wpt.get_truetrack_deg();
	fplwpt["trueheading"] = wpt.get_trueheading_deg();
	fplwpt["declination"] = wpt.get_declination_deg();
	fplwpt["magnetictrack"] = wpt.get_magnetictrack_deg();
	fplwpt["magneticheading"] = wpt.get_magneticheading_deg();
	fplwpt["dist"] = wpt.get_dist_nmi();
	fplwpt["fuel"] = wpt.get_fuel_usg();
	fplwpt["tas"] = wpt.get_tas_kts();
	fplwpt["rpm"] = wpt.get_rpm();
	fplwpt["mp"] = wpt.get_mp_inhg();
	fplwpt["type"] = wpt.get_type_string();
	{
		Wind<double> wind;
		wind.set_wind(wpt.get_winddir_deg(), wpt.get_windspeed_kts());
		wind.set_crs_tas(wpt.get_truetrack_deg(), wpt.get_tas_kts());
		if (!std::isnan(wind.get_gs()))
			fplwpt["gs"] = wind.get_gs();
	}
}

void SocketServer::encode_wpt(Json::Value& fplwpt, const ADR::FPLWaypoint& wpt)
{
	encode_wpt(fplwpt, (const FPlanWaypoint&)wpt);
	fplwpt["expanded"] = wpt.is_expanded();
	fplwpt["wptnr"] = wpt.get_wptnr();
	if (wpt.get_ptobj())
		fplwpt["pointuuid"] = to_string(wpt.get_ptobj()->get_uuid());
	if (wpt.get_pathobj())
		fplwpt["pathuuid"] = to_string(wpt.get_pathobj()->get_uuid());
}

void SocketServer::encode_fpl(Json::Value& reply, const FPlanRoute& route)
{
	reply["note"] = (std::string)route.get_note();
	reply["curwpt"] = route.get_curwpt();
	reply["offblock"] = (Json::LargestInt)route.get_time_offblock_unix();
	reply["save_offblock"] = (Json::LargestInt)route.get_save_time_offblock_unix();
	reply["onblock"] = (Json::LargestInt)route.get_time_onblock_unix();
	reply["save_onblock"] = (Json::LargestInt)route.get_save_time_onblock_unix();
	{
		Json::Value& fplan(reply["fplan"]);
		for (unsigned int i = 0; i < route.get_nrwpt(); ++i) {
			const FPlanWaypoint& wpt(route[i]);
			Json::Value fplwpt;
			encode_wpt(fplwpt, wpt);
			if (m_autoroute) {
				int alt(wpt.get_altitude());
				double densityalt, tas;
				m_autoroute->get_cruise_performance(alt, densityalt, tas);
				if (alt == wpt.get_altitude()) {
					if (!std::isnan(densityalt))
						fplwpt["densityalt"] = densityalt;
				}
			}
			fplan.append(fplwpt);
		}
	}
	reply["gcdist"] = route.gc_distance_nmi_dbl();
	reply["routedist"] = route.total_distance_nmi_dbl();
}

void SocketServer::encode_fpl(Json::Value& reply, const ADR::FlightPlan& route)
{
	reply["offblock"] = (Json::LargestInt)route.get_departuretime();
	{
		Json::Value& fplan(reply["fplan"]);
		for (ADR::FlightPlan::const_iterator wi(route.begin()), we(route.end()); wi != we; ++wi) {
			Json::Value fplwpt;
			encode_wpt(fplwpt, *wi);
			fplan.append(fplwpt);
		}
	}
	reply["gcdist"] = route.gc_distance_nmi_dbl();
	reply["routedist"] = route.total_distance_nmi_dbl();
	reply["aircraftid"] = route.get_aircraftid();
	reply["flightrules"] = std::string(1, route.get_flightrules());
	reply["flighttype"] = std::string(1, route.get_flighttype());
	reply["number"] = route.get_number();
	reply["aircrafttype"] = route.get_aircrafttype();
	reply["wakecategory"] = std::string(1, route.get_wakecategory());
	reply["equipment"] = route.get_equipment();
	reply["transponder"] = route.get_transponder();
	reply["pbn"] = route.get_pbn_string();
	reply["cruisespeed"] = route.get_cruisespeed();
	if (!route.empty()) {
		reply["departure"] = (std::string)route.front().get_icao();
		reply["destination"] = (std::string)route.back().get_icao();
	}
	reply["alternate1"] = route.get_alternate1();
	reply["alternate2"] = route.get_alternate2();
	reply["totaleet"] = (Json::LargestInt)route.get_totaleet();
	reply["endurance"] = (Json::LargestInt)route.get_endurance();
	if (route.get_personsonboard() == ~0U)
		reply["personsonboard"] = "TBN";
	else
		reply["personsonboard"] = route.get_personsonboard();
	reply["colourmarkings"] = route.get_colourmarkings();
	reply["remarks"] = route.get_remarks();
	reply["picname"] = route.get_picname();
	reply["emergencyradio"] = route.get_emergencyradio();
	reply["survival"] = route.get_survival();
	reply["lifejackets"] = route.get_lifejackets();
	reply["dinghies"] = route.get_dinghies();
	{
		Json::Value& otherinfo(reply["otherinfo"]);
		for (ADR::FlightPlan::otherinfo_t::const_iterator oi(route.get_otherinfo().begin()), oe(route.get_otherinfo().end()); oi != oe; ++oi)
			otherinfo[oi->get_category()] = (std::string)oi->get_text();
	}
}

void SocketServer::decode_wpt(FPlanWaypoint& wpt, const Json::Value& fplwpt)
{
	if (fplwpt.isMember("icao") && fplwpt["icao"].isString())
		wpt.set_icao(fplwpt["icao"].asString());
	if (fplwpt.isMember("name") && fplwpt["name"].isString())
		wpt.set_name(fplwpt["name"].asString());
	if (fplwpt.isMember("staynr") && fplwpt["staynr"].isIntegral() &&
	    fplwpt.isMember("staytime") && fplwpt["staytime"].isIntegral()) {
		unsigned int nr(fplwpt["staynr"].asInt()), tm(fplwpt["staytime"].asInt());
		tm = (tm + 59) / 60;
		wpt.set_pathcode(FPlanWaypoint::pathcode_airway);
		std::ostringstream oss;
		oss << "STAY" << std::max(1U, std::min(nr, 9U)) << '/'
		    << std::setw(2) << std::setfill('0') << (tm / 60)
		    << std::setw(2) << std::setfill('0') << (tm % 60);
	} else {
		if (fplwpt.isMember("pathname") && fplwpt["pathname"].isString())
			wpt.set_pathname(fplwpt["pathname"].asString());
		if (fplwpt.isMember("pathcode") && fplwpt["pathcode"].isString())
			wpt.set_pathcode_name(fplwpt["pathcode"].asString());
	}
	if (fplwpt.isMember("note") && fplwpt["note"].isString())
		wpt.set_note(fplwpt["note"].asString());
	if (fplwpt.isMember("time") && fplwpt["time"].isIntegral())
		wpt.set_time_unix(fplwpt["time"].asInt());
	if (fplwpt.isMember("save_time") && fplwpt["save_time"].isIntegral())
		wpt.set_save_time_unix(fplwpt["save_time"].asInt());
	if (fplwpt.isMember("flighttime") && fplwpt["flighttime"].isIntegral())
		wpt.set_flighttime(fplwpt["flighttime"].asInt());
	if (fplwpt.isMember("frequency") && fplwpt["frequency"].isIntegral())
		wpt.set_frequency(fplwpt["frequency"].asInt());
	wpt.set_coord(get_point(fplwpt, "coord"));
	if (fplwpt.isMember("altitude") && fplwpt["altitude"].isIntegral())
		wpt.set_altitude(fplwpt["altitude"].asInt());
	if (fplwpt.isMember("flags") && fplwpt["flags"].isIntegral())
		wpt.set_flags(fplwpt["flags"].asInt());
	if (fplwpt.isMember("flightrules") && fplwpt["flightrules"].isString() &&
	    (fplwpt["flightrules"].asString() == "vfr" || fplwpt["flightrules"].asString() == "ifr"))
		wpt.frob_flags((fplwpt["flightrules"].asString() == "ifr") ? FPlanWaypoint::altflag_ifr : 0, FPlanWaypoint::altflag_ifr);
	if (fplwpt.isMember("altmode") && fplwpt["altmode"].isString() &&
	    (fplwpt["altmode"].asString() == "qnh" || fplwpt["altmode"].asString() == "std"))
		wpt.frob_flags((fplwpt["altmode"].asString() == "std") ? FPlanWaypoint::altflag_standard : 0, FPlanWaypoint::altflag_standard);
	if (fplwpt.isMember("climb") && fplwpt["climb"].isBool())
		wpt.frob_flags(fplwpt["climb"].asBool() ? FPlanWaypoint::altflag_climb : 0, FPlanWaypoint::altflag_climb);
	if (fplwpt.isMember("descent") && fplwpt["descent"].isBool())
		wpt.frob_flags(fplwpt["descent"].asBool() ? FPlanWaypoint::altflag_descent : 0, FPlanWaypoint::altflag_descent);
	if (fplwpt.isMember("turnpoint") && fplwpt["turnpoint"].isBool())
		wpt.frob_flags(fplwpt["turnpoint"].asBool() ? FPlanWaypoint::altflag_turnpoint : 0, FPlanWaypoint::altflag_turnpoint);
	if (fplwpt.isMember("winddir") && fplwpt["winddir"].isNumeric())
		wpt.set_winddir_deg(fplwpt["winddir"].asDouble());
	if (fplwpt.isMember("windspeed") && fplwpt["windspeed"].isNumeric())
		wpt.set_windspeed_kts(fplwpt["windspeed"].asDouble());
	if (fplwpt.isMember("qff") && fplwpt["qff"].isNumeric())
		wpt.set_qff_hpa(fplwpt["qff"].asDouble());
	if (fplwpt.isMember("isaoffset") && fplwpt["isaoffset"].isNumeric())
		wpt.set_isaoffset_kelvin(fplwpt["isaoffset"].asDouble());
	if (fplwpt.isMember("truealt") && fplwpt["truealt"].isNumeric())
		wpt.set_truealt(fplwpt["truealt"].asDouble());
	if (fplwpt.isMember("trueheading") && fplwpt["trueheading"].isNumeric())
		wpt.set_trueheading_deg(fplwpt["trueheading"].asDouble());
	if (fplwpt.isMember("declination") && fplwpt["declination"].isNumeric())
		wpt.set_declination_deg(fplwpt["declination"].asDouble());
	if (fplwpt.isMember("fuel") && fplwpt["fuel"].isNumeric())
		wpt.set_fuel_usg(fplwpt["fuel"].asDouble());
	if (fplwpt.isMember("tas") && fplwpt["tas"].isNumeric())
		wpt.set_tas_kts(fplwpt["tas"].asDouble());
	if (fplwpt.isMember("rpm") && fplwpt["rpm"].isNumeric())
		wpt.set_rpm(fplwpt["rpm"].asDouble());
	if (fplwpt.isMember("mp") && fplwpt["mp"].isNumeric())
		wpt.set_mp_inhg(fplwpt["mp"].asDouble());
	if (fplwpt.isMember("type") && fplwpt["type"].isString())
		wpt.set_type_string(fplwpt["type"].asString());
}

void SocketServer::decode_wpt(ADR::FPLWaypoint& wpt, const Json::Value& json)
{
	decode_wpt((FPlanWaypoint&)wpt, json);
	if (json.isMember("pointuuid") && json["pointuuid"].isString()) {
		try {
			boost::uuids::string_generator gen;
			ADR::UUID u(gen(json["pointuuid"].asString()));
			if (!u.is_nil()) {
				ADR::Object::ptr_t p(new ADR::ObjectImpl<ADR::TimeSlice,ADR::Object::type_invalid>(u));
				wpt.set_ptobj(p);
			}
		} catch (const std::runtime_error& e) {
		}
	}
}

void SocketServer::decode_fpl(FPlanRoute& route, const Json::Value& json)
{
	if (json.isMember("note") && json["note"].isString())
		route.set_note(json["note"].asString());
	if (json.isMember("curwpt") && json["curwpt"].isIntegral())
		route.set_curwpt(json["curwpt"].asInt());
	if (json.isMember("offblock") && json["offblock"].isIntegral())
		route.set_time_offblock_unix(json["offblock"].asInt());
	if (json.isMember("save_offblock") && json["save_offblock"].isIntegral())
		route.set_save_time_offblock_unix(json["save_offblock"].asInt());
	if (json.isMember("onblock") && json["onblock"].isIntegral())
		route.set_time_onblock_unix(json["onblock"].asInt());
	if (json.isMember("save_onblock") && json["save_onblock"].isIntegral())
		route.set_save_time_onblock_unix(json["save_onblock"].asInt());
	if (json.isMember("fplan") && json["fplan"].isArray()) {
		const Json::Value& fplan(json["fplan"]);
		for (unsigned int i = 0; i < fplan.size(); ++i) {
			FPlanWaypoint wpt;
			decode_wpt(wpt, fplan[i]);
			route.insert_wpt(~0U, wpt);
		}
	}
	route.recompute_dist();
}

void SocketServer::decode_fpl(ADR::FlightPlan& route, const Json::Value& json)
{
	if (json.isMember("fplan") && json["fplan"].isArray()) {
		const Json::Value& fplan(json["fplan"]);
		for (unsigned int i = 0; i < fplan.size(); ++i) {
			ADR::FPLWaypoint wpt;
			decode_wpt(wpt, fplan[i]);
			route.push_back(wpt);
		}
	}
	if (json.isMember("aircraftid") && json["aircraftid"].isString())
		route.set_aircraftid(json["aircraftid"].asString());
	if (json.isMember("flighttype") && json["flighttype"].isString()) {
		std::string x(json["flighttype"].asString());
		if (!x.empty())
			route.set_flighttype(x[0]);
	}
	if (json.isMember("number") && json["number"].isIntegral())
		route.set_number(json["number"].asInt());
	if (json.isMember("aircrafttype") && json["aircrafttype"].isString())
		route.set_aircrafttype(json["aircrafttype"].asString());
	if (json.isMember("wakecategory") && json["wakecategory"].isString()) {
		std::string x(json["wakecategory"].asString());
		if (!x.empty())
			route.set_wakecategory(x[0]);
	}
	if (json.isMember("equipment") && json["equipment"].isString())
		route.set_equipment(json["equipment"].asString());
	if (json.isMember("transponder") && json["transponder"].isString())
		route.set_transponder(json["transponder"].asString());
	if (json.isMember("pbn") && json["pbn"].isString())
		route.set_pbn(json["pbn"].asString());
	if (json.isMember("cruisespeed") && json["cruisespeed"].isString())
		route.set_cruisespeed(json["cruisespeed"].asString());
	if (json.isMember("alternate1") && json["alternate1"].isString())
		route.set_alternate1(json["alternate1"].asString());
	if (json.isMember("alternate2") && json["alternate2"].isString())
		route.set_alternate2(json["alternate2"].asString());
	if (json.isMember("totaleet") && json["totaleet"].isIntegral())
		route.set_totaleet(json["totaleet"].asInt());
	if (json.isMember("endurance") && json["endurance"].isIntegral())
		route.set_endurance(json["endurance"].asInt());
	if (json.isMember("personsonboard") && json["personsonboard"].isString() && json["personsonboard"].asString() == "TBN")
		route.set_personsonboard_tbn();
	else if (json.isMember("personsonboard") && json["personsonboard"].isIntegral())
		route.set_personsonboard(json["personsonboard"].asInt());
	if (json.isMember("colourmarkings") && json["colourmarkings"].isString())
		route.set_colourmarkings(json["colourmarkings"].asString());
	if (json.isMember("remarks") && json["remarks"].isString())
		route.set_remarks(json["remarks"].asString());
	if (json.isMember("picname") && json["picname"].isString())
		route.set_picname(json["picname"].asString());
	if (json.isMember("emergencyradio") && json["emergencyradio"].isString())
		route.set_emergencyradio(json["emergencyradio"].asString());
	if (json.isMember("survival") && json["survival"].isString())
		route.set_survival(json["survival"].asString());
	if (json.isMember("lifejackets") && json["lifejackets"].isString())
		route.set_lifejackets(json["lifejackets"].asString());
	if (json.isMember("dinghies") && json["dinghies"].isString())
		route.set_dinghies(json["dinghies"].asString());
	if (json.isMember("offblock") && json["offblock"].isIntegral())
		route.set_departuretime(json["offblock"].asInt());
	if (json.isMember("otherinfo") && json["otherinfo"].isObject()) {
		route.otherinfo_clear();
		const Json::Value& otherinfo(json["otherinfo"]);
		Json::Value::Members m(otherinfo.getMemberNames());
		for (Json::Value::Members::const_iterator mi(m.begin()), me(m.end()); mi != me; ++mi) {
			if (otherinfo.isMember(*mi) && otherinfo[*mi].isString()) {
				route.otherinfo_add(*mi, otherinfo[*mi].asString());
			}
		}
	}
	route.recompute_dist();
}

void SocketServer::encode_rule(Json::Value& reply, const TrafficFlowRestrictions::RuleResult& rule)
{
	switch (rule.get_codetype()) {
	case TrafficFlowRestrictions::RuleResult::codetype_mandatory:
		reply["codetype"] = "mandatory";
		break;

	case TrafficFlowRestrictions::RuleResult::codetype_forbidden:
		reply["codetype"] = "forbidden";
		break;

	case TrafficFlowRestrictions::RuleResult::codetype_closed:
		reply["codetype"] = "closedforcruising";
		break;

	default:
		reply["codetype"] = "?";
		break;
	}
	reply["dct"] = rule.is_dct();
	reply["unconditional"] = rule.is_unconditional();
	reply["routestatic"] = rule.is_routestatic();
	reply["mandatoryinbound"] = rule.is_mandatoryinbound();
	reply["disabled"] = rule.is_disabled();
	{
		Json::Value& vset(reply["vertices"]);
		for (TrafficFlowRestrictions::RuleResult::set_t::const_iterator i(rule.get_vertexset().begin()), e(rule.get_vertexset().end()); i != e; ++i)
			vset.append(Json::Value(*i));
		Json::Value& eset(reply["edges"]);
		for (TrafficFlowRestrictions::RuleResult::set_t::const_iterator i(rule.get_edgeset().begin()), e(rule.get_edgeset().end()); i != e; ++i)
			eset.append(Json::Value(*i));
		Json::Value& rset(reply["reflocs"]);
		for (TrafficFlowRestrictions::RuleResult::set_t::const_iterator i(rule.get_reflocset().begin()), e(rule.get_reflocset().end()); i != e; ++i)
			rset.append(Json::Value(*i));
	}
	reply["desc"] = rule.get_desc();
	reply["oprgoal"] = rule.get_oprgoal();
	reply["cond"] = rule.get_cond();
	Json::Value& ralternatives(reply["alternatives"]);
	for (unsigned int i = 0; i < rule.size(); ++i) {
		const TrafficFlowRestrictions::RuleResult::Alternative& alt(rule[i]);
		Json::Value ralt;
		{
			Json::Value& vset(ralt["vertices"]);
			for (TrafficFlowRestrictions::RuleResult::set_t::const_iterator i(alt.get_vertexset().begin()), e(alt.get_vertexset().end()); i != e; ++i)
				vset.append(Json::Value(*i));
			Json::Value& eset(ralt["edges"]);
			for (TrafficFlowRestrictions::RuleResult::set_t::const_iterator i(alt.get_edgeset().begin()), e(alt.get_edgeset().end()); i != e; ++i)
				eset.append(Json::Value(*i));
		}
		Json::Value& rsequences(ralt["sequences"]);
		for (unsigned int j = 0; j < alt.size(); ++j) {
			const TrafficFlowRestrictions::RuleResult::Alternative::Sequence& el(alt[j]);
			Json::Value rseq;
			rseq["type"] = el.get_type_string();
			switch (el.get_type()) {
			case TrafficFlowRestrictions::RuleResult::Alternative::Sequence::type_sid:
			case TrafficFlowRestrictions::RuleResult::Alternative::Sequence::type_star:
				rseq["ident"] = el.get_ident();
				rseq["airport"] = el.get_airport();
				break;

			case TrafficFlowRestrictions::RuleResult::Alternative::Sequence::type_airspace:
				rseq["ident"] = el.get_ident();
				rseq["class"] = (std::string)AirspacesDb::Airspace::get_class_string(el.get_bdryclass(), AirspacesDb::Airspace::typecode_ead);
				break;

			case TrafficFlowRestrictions::RuleResult::Alternative::Sequence::type_airway:
				rseq["ident"] = el.get_ident();
				// fall through

			case TrafficFlowRestrictions::RuleResult::Alternative::Sequence::type_dct:
			{
				const TrafficFlowRestrictions::RuleResult::Alternative::Sequence::RulePoint& pt(el.get_point1());
				Json::Value& rpt(rseq["point1"]);
				rpt["ident"] = pt.get_ident();
				set_point(rpt, "coord", pt.get_coord());
				rpt["type"] = to_str(pt.get_type());
				rpt["vor"] = pt.is_vor();
				rpt["ndb"] = pt.is_ndb();
				rpt["valid"] = pt.is_valid();
				// fall through
			}

			case TrafficFlowRestrictions::RuleResult::Alternative::Sequence::type_point:
			{
				const TrafficFlowRestrictions::RuleResult::Alternative::Sequence::RulePoint& pt(el.get_point0());
				Json::Value& rpt(rseq["point0"]);
				rpt["ident"] = pt.get_ident();
				set_point(rpt, "coord", pt.get_coord());
				rpt["type"] = to_str(pt.get_type());
				rpt["vor"] = pt.is_vor();
				rpt["ndb"] = pt.is_ndb();
				rpt["valid"] = pt.is_valid();
				break;
			}

			default:
				break;
			}
			if (el.is_lower_alt_valid())
				rseq["altlower"] = el.get_lower_alt();
			if (el.is_upper_alt_valid())
				rseq["altupper"] = el.get_upper_alt();
			rsequences.append(rseq);
		}
		ralternatives.append(ralt);
	}
}

void SocketServer::encode_rule(Json::Value& rsequences, const ADR::RuleSequence& alt2, ADR::timetype_t tm)
{
	for (unsigned int j = 0; j < alt2.size(); ++j) {
		const ADR::RuleSegment& el(alt2[j]);
		Json::Value rseq;
		rseq["text"] = el.to_shortstr(tm);
		rseq["type"] = to_str(el.get_type());
		switch (el.get_type()) {
		case ADR::RuleSegment::type_airway:
		{
			const ADR::IdentTimeSlice& ts(el.get_airway()->operator()(tm).as_ident());
			rseq["ident"] = ts.get_ident();
			rseq["airwayuuid"] = el.get_airway_uuid().to_str();
			// fall through
		}

		case ADR::RuleSegment::type_dct:
		{
			Json::Value& rpt(rseq["point1"]);
			const ADR::PointIdentTimeSlice& ts(el.get_wpt1()->operator()(tm).as_point());
			rpt["ident"] = ts.get_ident();
			set_point(rpt, "coord", ts.get_coord());
			const ADR::NavaidTimeSlice& tsnav(ts.as_navaid());
			if (tsnav.is_valid()) {
				rpt["type"] = to_str(tsnav.get_type());
				rpt["name"] = tsnav.get_name();
				rpt["vor"] = tsnav.is_vor();
				rpt["dme"] = tsnav.is_dme();
				rpt["tacan"] = tsnav.is_tacan();
				rpt["ils"] = tsnav.is_ils();
				rpt["loc"] = tsnav.is_loc();
				rpt["ndb"] = tsnav.is_ndb();
				rpt["mkr"] = tsnav.is_mkr();
			}
			const ADR::DesignatedPointTimeSlice& tsdes(ts.as_designatedpoint());
			if (tsdes.is_valid()) {
				rpt["type"] = to_str(tsdes.get_type());
				rpt["name"] = tsdes.get_name();
			}
			rpt["valid"] = ts.is_valid();
			rpt["uuid"] = el.get_uuid1().to_str();
			// fall through
		}

		case ADR::RuleSegment::type_point:
		{
			Json::Value& rpt(rseq["point0"]);
			const ADR::PointIdentTimeSlice& ts(el.get_wpt0()->operator()(tm).as_point());
			rpt["ident"] = ts.get_ident();
			set_point(rpt, "coord", ts.get_coord());
			const ADR::NavaidTimeSlice& tsnav(ts.as_navaid());
			if (tsnav.is_valid()) {
				rpt["type"] = to_str(tsnav.get_type());
				rpt["name"] = tsnav.get_name();
				rpt["vor"] = tsnav.is_vor();
				rpt["dme"] = tsnav.is_dme();
				rpt["tacan"] = tsnav.is_tacan();
				rpt["ils"] = tsnav.is_ils();
				rpt["loc"] = tsnav.is_loc();
				rpt["ndb"] = tsnav.is_ndb();
				rpt["mkr"] = tsnav.is_mkr();
			}
			const ADR::DesignatedPointTimeSlice& tsdes(ts.as_designatedpoint());
			if (tsdes.is_valid()) {
				rpt["type"] = to_str(tsdes.get_type());
				rpt["name"] = tsdes.get_name();
			}
			rpt["valid"] = ts.is_valid();
			rpt["uuid"] = el.get_uuid0().to_str();
			break;
		}

		case ADR::RuleSegment::type_sid:
		case ADR::RuleSegment::type_star:
		{
			const ADR::StandardInstrumentTimeSlice& ts(el.get_airway()->operator()(tm).as_sidstar());
			rseq["valid"] = ts.is_valid();
			rseq["ident"] = ts.get_ident();
			rseq["uuid"] = el.get_airway_uuid().to_str();
			const ADR::AirportTimeSlice& tsarpt(ts.get_airport().get_obj()->operator()(tm).as_airport());
			rseq["airport"] = tsarpt.get_ident();
			rseq["airportuuid"] = ts.get_airport().to_str();
			break;
		}

		case ADR::RuleSegment::type_airspace:
		{
			const ADR::AirspaceTimeSlice& ts(el.get_wpt0()->operator()(tm).as_airspace());
			rseq["valid"] = ts.is_valid();
			rseq["ident"] = ts.get_ident();
			rseq["uuid"] = el.get_uuid0().to_str();
			rseq["aspctype"] = to_str(ts.get_type());
			rseq["aspclocaltype"] = to_str(ts.get_localtype());
			rseq["icao"] = ts.is_icao();
			rseq["flexibleuse"] = ts.is_flexibleuse();
			rseq["level1"] = ts.is_level(1);
			rseq["level2"] = ts.is_level(2);
			rseq["level3"] = ts.is_level(3);
			break;
		}

		default:
			break;
		}
		if (el.get_alt().is_lower_valid())
			rseq["altlower"] = el.get_alt().get_lower_alt();
		if (el.get_alt().is_upper_valid())
			rseq["altupper"] = el.get_alt().get_upper_alt();
		rsequences.append(rseq);
	}
}

void SocketServer::encode_rule(Json::Value& reply, const ADR::RestrictionResult& rule)
{
	const ADR::FlightRestrictionTimeSlice& tsrule(rule.get_timeslice());
	if (!tsrule.is_valid()) {
		reply["codetype"] = "invalid";
		return;
	}
	reply["codetype"] = to_str(tsrule.get_type());
	reply["procind"] = to_str(tsrule.get_procind());
	reply["dct"] = tsrule.is_dct();
	reply["strictdct"] = tsrule.is_strict_dct();
	reply["unconditional"] = tsrule.is_unconditional();
	reply["routestatic"] = tsrule.is_routestatic();
	reply["mandatoryinbound"] = tsrule.is_mandatoryinbound();
	reply["mandatoryoutbound"] = tsrule.is_mandatoryoutbound();
	reply["disabled"] = !tsrule.is_enabled();
	reply["trace"] = tsrule.is_trace();
	reply["time"] = (Json::LargestInt)rule.get_time();
	{
		Json::Value& vset(reply["vertices"]);
		for (ADR::RestrictionResult::set_t::const_iterator i(rule.get_vertexset().begin()), e(rule.get_vertexset().end()); i != e; ++i)
			vset.append(Json::Value(*i));
		Json::Value& eset(reply["edges"]);
		for (ADR::RestrictionResult::set_t::const_iterator i(rule.get_edgeset().begin()), e(rule.get_edgeset().end()); i != e; ++i)
			eset.append(Json::Value(*i));
	}
	if (rule.has_refloc()) {
		reply["refloc"] = rule.get_refloc();
		// backward compatibility, should be removed in the future
		Json::Value& rset(reply["reflocs"]);
		rset.append(Json::Value(rule.get_refloc()));
	}
	reply["desc"] = tsrule.get_instruction();
	// backward compatibility, should be removed in the future
	reply["oprgoal"] = "";
	if (tsrule.get_condition())
		reply["cond"] = tsrule.get_condition()->to_str(rule.get_time());
	Json::Value& ralternatives(reply["alternatives"]);
	for (unsigned int i = 0; i < rule.size(); ++i) {
		const ADR::RestrictionSequenceResult& alt(rule[i]);
		Json::Value ralt;
		{
			Json::Value& vset(ralt["vertices"]);
			for (ADR::RestrictionSequenceResult::set_t::const_iterator i(alt.get_vertexset().begin()), e(alt.get_vertexset().end()); i != e; ++i)
				vset.append(Json::Value(*i));
			Json::Value& eset(ralt["edges"]);
			for (ADR::RestrictionSequenceResult::set_t::const_iterator i(alt.get_edgeset().begin()), e(alt.get_edgeset().end()); i != e; ++i)
				eset.append(Json::Value(*i));
		}
		encode_rule(ralt["sequences"], alt.get_sequence(), rule.get_time());
		ralternatives.append(ralt);
	}
}

void SocketServer::check_fplan(Json::Value& localvalidate, const FPlanRoute& route)
{
	{
		CFMUAutoroute45 *ar45(dynamic_cast<CFMUAutoroute45 *>(m_autoroute));
		if (ar45) {
			TrafficFlowRestrictions::Result res;
			bool chk(ar45->check_fplan(res, route));
			if (!chk) {
				localvalidate["error"] = "TFR disabled";
				return;
			}
			{
				Json::Value& messages(localvalidate["messages"]);
				for (unsigned int i = 0, n = res.messages_size(); i < n; ++i) {
					const TrafficFlowRestrictions::Message& msg(res.get_message(i));
					Json::Value rmsg;
					Json::Value& vset(messages["vertices"]);
					for (TrafficFlowRestrictions::Message::set_t::const_iterator
						     i(msg.get_vertexset().begin()), e(msg.get_vertexset().end()); i != e; ++i)
						vset.append(Json::Value(*i));
					Json::Value& eset(messages["edges"]);
					for (TrafficFlowRestrictions::Message::set_t::const_iterator
						     i(msg.get_edgeset().begin()), e(msg.get_edgeset().end()); i != e; ++i)
						eset.append(Json::Value(*i));
					rmsg["text"] = msg.get_text();
					rmsg["rule"] = msg.get_rule();
					rmsg["type"] = msg.get_type_string();
				}
			}
			{
				Json::Value& rules(localvalidate["rules"]);
				for (unsigned int i = 0, n = res.size(); i < n; ++i) {
					Json::Value rrule;
					encode_rule(rrule, res[i]);
					rules.append(rrule);
				}
			}
			{
				const TrafficFlowRestrictions::Fpl& fpl(res.get_fplan());
				Json::Value& expfpl(localvalidate["expandedfpl"]);
				for (unsigned int i = 0; i < fpl.size(); ++i) {
					const TrafficFlowRestrictions::FplWpt& wpt(fpl[i]);
					Json::Value fplwpt;
					encode_wpt(fplwpt, wpt);
					fplwpt["wptnr"] = wpt.get_wptnr();
					fplwpt["ndb"] = wpt.has_ndb();
					fplwpt["vor"] = wpt.has_vor();
					fplwpt["dme"] = wpt.has_dme();
					fplwpt["ident"] = (std::string)wpt.get_ident();
					expfpl.append(fplwpt);
				}
			}
			localvalidate["aircrafttype"] = res.get_aircrafttype();
			localvalidate["aircrafttypeclass"] = res.get_aircrafttypeclass();
			localvalidate["equipment"] = res.get_equipment();
			localvalidate["pbn"] = res.get_pbn_string();
			localvalidate["typeofflight"] = res.get_typeofflight();
			return;
		}
	}
	{
		CFMUAutoroute51 *ar51(dynamic_cast<CFMUAutoroute51 *>(m_autoroute));
		if (ar51) {
			ADR::FlightPlan route2;
			for (unsigned int i(0), n(route.get_nrwpt()); i < n; ++i)
				route2.push_back(route[i]);
			route2.set_aircraft(ar51->get_aircraft(), ar51->get_engine_rpm(), ar51->get_engine_mp(), ar51->get_engine_bhp());
			route2.set_flighttype('G');
			route2.set_number(1);
			route2.set_alternate1("");
			route2.set_alternate2("");
			route2.set_departuretime(ar51->get_deptime());
			route2.set_personsonboard_tbn();
			route2.set_remarks("");
			check_fplan(localvalidate, route2);
			return;
		}
	}
}

void SocketServer::check_fplan(Json::Value& localvalidate, const ADR::FlightPlan& route)
{
	{
		CFMUAutoroute51 *ar51(dynamic_cast<CFMUAutoroute51 *>(m_autoroute));
		if (ar51) {
			std::vector<ADR::Message> msgs;
			ADR::RestrictionResults res;
			ADR::FlightPlan fpl(route);
			localvalidate["ok"] = ar51->check_fplan(msgs, res, fpl);
			{
				Json::Value& messages(localvalidate["messages"]);
				for (unsigned int i = 0, n = msgs.size(); i < n; ++i) {
					const ADR::Message& msg(msgs[i]);
					Json::Value rmsg;
					Json::Value& vset(messages["vertices"]);
					for (TrafficFlowRestrictions::Message::set_t::const_iterator
						     i(msg.get_vertexset().begin()), e(msg.get_vertexset().end()); i != e; ++i)
						vset.append(Json::Value(*i));
					Json::Value& eset(messages["edges"]);
					for (TrafficFlowRestrictions::Message::set_t::const_iterator
						     i(msg.get_edgeset().begin()), e(msg.get_edgeset().end()); i != e; ++i)
						eset.append(Json::Value(*i));
					rmsg["text"] = msg.get_text();
					rmsg["rule"] = msg.get_rule();
					rmsg["type"] = msg.get_type_string();
					rmsg["time"] = (Json::LargestInt)msg.get_time();
					if (msg.get_obj())
						rmsg["uuid"] = msg.get_obj()->get_uuid().to_str();
				}
			}
			{
				Json::Value& rules(localvalidate["rules"]);
				for (unsigned int i = 0, n = res.size(); i < n; ++i) {
					Json::Value rrule;
					encode_rule(rrule, res[i]);
					rules.append(rrule);
				}
			}
			{
				Json::Value& expfpl(localvalidate["expandedfpl"]);
				for (unsigned int i = 0; i < fpl.size(); ++i) {
					const ADR::FPLWaypoint& wpt(fpl[i]);
					Json::Value fplwpt;
					encode_wpt(fplwpt, wpt);
					expfpl.append(fplwpt);
				}
			}
			localvalidate["aircrafttype"] = route.get_aircrafttype();
			localvalidate["aircrafttypeclass"] = Aircraft::get_aircrafttypeclass(route.get_aircrafttype());
			localvalidate["equipment"] = route.get_equipment();
			localvalidate["pbn"] = route.get_pbn_string();
			localvalidate["typeofflight"] = route.get_flighttype();
			return;
		}
	}
	{
		CFMUAutoroute45 *ar45(dynamic_cast<CFMUAutoroute45 *>(m_autoroute));
		if (ar45) {
			FPlanRoute route2(*(FPlan *)0);
			route2.clear_wpt();
			for (ADR::FlightPlan::const_iterator i(route.begin()), e(route.end()); i != e; ++i)
				route2.insert_wpt(~0U, *i);
			if (route2.get_nrwpt()) {
				route2.set_time_offblock_unix(route2[0].get_time_unix() - 5 * 60);
				route2.set_time_onblock_unix(route2[0].get_time_unix() + route2[route2.get_nrwpt() - 1].get_flighttime() + 5 * 60);
			}
			check_fplan(localvalidate, route2);
			return;
		}
	}
}

void SocketServer::routeweather(Json::Value& wxval, const FPlanRoute& route)
{
	if (!m_autoroute) {
		wxval["error"] = "no autoroute instance";
		return;
	}
	std::string fname(Glib::build_filename(m_autoroute->get_logprefix(), "weather-XXXXXX"));
	int fd(Glib::mkstemp(fname));
	if (fd == -1) {
		wxval["error"] = "cannot open file " + fname;
		return;
	}
	wxval["file"] = fname;
	Json::Value wx;
	// Route Profile
	try {
		TopoDb30 topodb;
                topodb.open(m_autoroute->get_db_auxdir().empty() ? Engine::get_default_aux_dir() : m_autoroute->get_db_auxdir());
		TopoDb30::RouteProfile p(topodb.get_profile(route, 5));
		topodb.close();
		Json::Value& wxterrain(wx["terrain"]);
		for (TopoDb30::RouteProfile::const_iterator pi(p.begin()), pe(p.end()); pi != pe; ++pi) {
			Json::Value v;
			v.append(Json::Value(pi->get_dist()));
			v.append(Json::Value(pi->get_routedist()));
			v.append(Json::Value(pi->get_routeindex()));
			v.append(Json::Value(pi->get_elev()));
			v.append(Json::Value(pi->get_minelev()));
			v.append(Json::Value(pi->get_maxelev()));
			wxterrain.append(v);
		}
        } catch (const std::exception& e) {
		wxval["error"] = std::string("topo db error: ") + e.what();
        }
	// Weather Profile
	{
		static const unsigned int nrsfc(sizeof(GRIB2::WeatherProfilePoint::isobaric_levels)/sizeof(GRIB2::WeatherProfilePoint::isobaric_levels[0]));
		{
			Json::Value& wxsfc(wx["wxsurfaces"]);
			for (unsigned int i = 0; i < nrsfc; ++i) {
				Json::Value v;
				if (GRIB2::WeatherProfilePoint::isobaric_levels[i] >= 0)
					v["pressure"] = GRIB2::WeatherProfilePoint::isobaric_levels[i];
				v["altitude"] = GRIB2::WeatherProfilePoint::altitudes[i];
				wxsfc.append(v);
			}
		}
		GRIB2::WeatherProfile p(m_autoroute->get_grib2().get_profile(route));
		Json::Value& wxprof(wx["wxprofile"]);
		for (GRIB2::WeatherProfile::const_iterator pi(p.begin()), pe(p.end()); pi != pe; ++pi) {
			Json::Value v;
			v.append(Json::Value(pi->get_pt().get_lat_deg_dbl()));
			v.append(Json::Value(pi->get_pt().get_lon_deg_dbl()));
			v.append(Json::Value((Json::LargestInt)pi->get_efftime()));
			v.append(Json::Value(pi->get_alt()));
			v.append(Json::Value(pi->get_dist()));
			v.append(Json::Value(pi->get_routedist()));
			v.append(Json::Value(pi->get_routeindex()));
			v.append(Json::Value(pi->get_zerodegisotherm()));
			v.append(Json::Value(pi->get_tropopause()));
			v.append(Json::Value(pi->get_cldbdrycover()));
			v.append(Json::Value(pi->get_cldlowcover()));
			v.append(Json::Value(pi->get_cldlowbase()));
			v.append(Json::Value(pi->get_cldlowtop()));
			v.append(Json::Value(pi->get_cldmidcover()));
			v.append(Json::Value(pi->get_cldmidbase()));
			v.append(Json::Value(pi->get_cldmidtop()));
			v.append(Json::Value(pi->get_cldhighcover()));
			v.append(Json::Value(pi->get_cldhighbase()));
			v.append(Json::Value(pi->get_cldhightop()));
			v.append(Json::Value(pi->get_cldconvcover()));
			v.append(Json::Value(pi->get_cldconvbase()));
			v.append(Json::Value(pi->get_cldconvtop()));
			v.append(Json::Value(pi->get_precip()));
			v.append(Json::Value(pi->get_preciprate()));
			v.append(Json::Value(pi->get_convprecip()));
			v.append(Json::Value(pi->get_convpreciprate()));
			v.append(Json::Value(pi->get_flags() & GRIB2::WeatherProfilePoint::flags_daymask));
			v.append(Json::Value(!!(pi->get_flags() & GRIB2::WeatherProfilePoint::flags_rain)));
			v.append(Json::Value(!!(pi->get_flags() & GRIB2::WeatherProfilePoint::flags_freezingrain)));
			v.append(Json::Value(!!(pi->get_flags() & GRIB2::WeatherProfilePoint::flags_icepellets)));
			v.append(Json::Value(!!(pi->get_flags() & GRIB2::WeatherProfilePoint::flags_snow)));
			Json::Value vs;
			for (unsigned int i = 0; i < nrsfc; ++i) {
				const GRIB2::WeatherProfilePoint::Surface& sfc(pi->operator[](i));
				Json::Value vv;
				vv.append(Json::Value(sfc.get_uwind()));
				vv.append(Json::Value(sfc.get_vwind()));
				vv.append(Json::Value(sfc.get_temp()));
				vv.append(Json::Value(sfc.get_rh()));
				vv.append(Json::Value(sfc.get_hwsh()));
				vv.append(Json::Value(sfc.get_vwsh()));
				vs.append(vv);
			}
			v.append(vs);
			wxprof.append(v);
		}
	}
	// write file
	{
		Json::FastWriter writer;
		std::string msg(writer.write(wx));
		write(fd, msg.c_str(), msg.size());
	}
	close(fd);
}

void SocketServer::weathercharts(Json::Value& wxval, const FPlanRoute& route, double width, double height,
				 double maxprofilepagedist, MeteoProfile::yaxis_t profileyaxis, double profilemaxalt,
				 bool svg, bool allowportrait, bool gramet, const std::vector<double>& hpa)
{
	if (!m_autoroute) {
		wxval["error"] = "no autoroute instance";
		return;
	}
	if (!gramet && hpa.empty()) {
		wxval["error"] = "no charts requested";
		return;
	}
	MeteoProfile prof(route);
	MeteoChart chart(route, std::vector<FPlanAlternate>(), std::vector<FPlanRoute>(), m_autoroute->get_grib2());
	if (gramet) {
		try {
			TopoDb30 topodb;
			topodb.open(m_autoroute->get_db_auxdir().empty() ? Engine::get_default_aux_dir() : m_autoroute->get_db_auxdir());
			prof.set_routeprofile(topodb.get_profile(route, 5));
			topodb.close();
		} catch (const std::exception& e) {
			wxval["error"] = std::string("topo db error: ") + e.what();
			return;
		}
		try {
			prof.set_wxprofile(m_autoroute->get_grib2().get_profile(route));
		} catch (const std::exception& e) {
			wxval["error"] = std::string("wx profile error: ") + e.what();
			return;
		}
	}
	std::string fname(Glib::build_filename(m_autoroute->get_logprefix(), "weather-XXXXXX"));
	int fd(Glib::mkstemp(fname));
	if (fd == -1) {
		wxval["error"] = "cannot open file " + fname;
		return;
	}
	wxval["file"] = fname;
	Cairo::RefPtr<Cairo::Surface> surface;
	Cairo::RefPtr<Cairo::PdfSurface> pdfsurface;
	if (svg) {
		surface = Cairo::SvgSurface::create(fname, width, height);
	} else {
		surface = pdfsurface = Cairo::PdfSurface::create(fname, width, height);
	}
	close(fd);
	Cairo::RefPtr<Cairo::Context> ctx(Cairo::Context::create(surface));
	ctx->translate(20, 20);
	if (gramet) {
		static const double maxprofilepagedist(200);
		double pagedist(route.total_distance_nmi_dbl());
		unsigned int nrpages(1);
		if (!std::isnan(maxprofilepagedist) && maxprofilepagedist > 0 && pagedist > maxprofilepagedist) {
			nrpages = std::ceil(pagedist / maxprofilepagedist);
			pagedist /= nrpages;
		}
		double scaledist(prof.get_scaledist(ctx, width - 40, pagedist));
		double scaleelev, originelev;
		if (profileyaxis == MeteoProfile::yaxis_pressure) {
			scaleelev = prof.get_scaleelev(ctx, height - 40, profilemaxalt, profileyaxis);
			originelev = IcaoAtmosphere<double>::std_sealevel_pressure;
		} else {
			scaleelev = prof.get_scaleelev(ctx, height - 40, profilemaxalt, profileyaxis);
			originelev = 0;
		}
		for (unsigned int pgnr(0); pgnr < nrpages; ++pgnr) {
			prof.draw(ctx, width - 40, height - 40, pgnr * pagedist, scaledist, originelev, scaleelev, profileyaxis);
			ctx->show_page();
			//surface->show_page();
		}
	}
	if (!hpa.empty()) {
		Point center;
		double scalelon(1e-5), scalelat(1e-5);
		chart.get_fullscale(width - 40, height - 40, center, scalelon, scalelat);
		if (pdfsurface && allowportrait) {
			Point center1;
			double scalelon1(1e-5), scalelat1(1e-5);
			chart.get_fullscale(height - 40, width - 40, center1, scalelon1, scalelat1);
			if (fabs(scalelat1) > fabs(scalelat)) {
				center = center1;
				scalelon = scalelon1;
				scalelat = scalelat1;
				std::swap(width, height);
				pdfsurface->set_size(width, height);
				ctx = Cairo::Context::create(surface);
				ctx->translate(20, 20);
			}
		}
		for (std::vector<double>::const_iterator i(hpa.begin()), e(hpa.end()); i != e; ++i) {
			chart.draw(ctx, width - 40, height - 40, center, scalelon, scalelat, 100.0 * *i);
			ctx->show_page();
			//surface->show_page();
		}
	}
	surface->finish();
}

void SocketServer::navlog(Json::Value& nlog, const FPlanRoute& route, const std::string& templ, const FPlanRoute::GFSResult& gfsr, bool hidewbpage)
{
	if (!m_autoroute) {
		nlog["error"] = "no autoroute instance";
		return;
	}
	std::string fname(Glib::build_filename(m_autoroute->get_logprefix(), "navlog-XXXXXX"));
	int fd(Glib::mkstemp(fname));
	if (fd == -1) {
		nlog["error"] = "cannot open file " + fname;
		return;
	}
	nlog["file"] = fname;
	gint64 minreftime(-1), maxreftime(-1), minefftime(-1), maxefftime(-1);
	if (gfsr.is_modified() && !gfsr.is_partial()) {
		minreftime = gfsr.get_minreftime();
		maxreftime = gfsr.get_maxreftime();
		minefftime = gfsr.get_minefftime();
		maxefftime = gfsr.get_maxefftime();
	}
	Engine eng(m_autoroute->get_db_maindir(), Engine::auxdb_override, m_autoroute->get_db_auxdir(), false, false);
	m_autoroute->get_aircraft().navfplan_gnumeric(fname, eng, route, std::vector<FPlanAlternate>(),
						      Aircraft::Cruise::EngineParams(m_autoroute->get_engine_bhp(), m_autoroute->get_engine_rpm(),
										     m_autoroute->get_engine_mp()),
						      std::numeric_limits<double>::quiet_NaN(),
						      Aircraft::WeightBalance::elementvalues_t(), templ,
						      minreftime, maxreftime, minefftime, maxefftime,
						      hidewbpage);
	close(fd);
}

void SocketServer::cmd_nop(const Json::Value& cmdin, Json::Value& cmdout)
{
}

void SocketServer::cmd_quit(const Json::Value& cmdin, Json::Value& cmdout)
{
	if (m_mainloop)
		m_mainloop->quit();
}

void SocketServer::cmd_departure(const Json::Value& cmdin, Json::Value& cmdout)
{
	if (!m_autoroute) {
		cmdout["error"] = "no autoroute instance";
		return;
	}
	if (cmdin.isMember("icao") || cmdin.isMember("name")) {
		std::string icao(cmdin.get("icao", "").asString());
		std::string name(cmdin.get("name", "").asString());
		if (!m_autoroute->set_departure(icao, name))
			throw std::runtime_error("departure aerodrome " + icao + " " + name + " not found");
	}
	if (cmdin.isMember("ifr")) {
		const Json::Value& x(cmdin["ifr"]);
		m_autoroute->set_departure_ifr(!x.isBool() || x.asBool());
	}
	if (cmdin.isMember("vfr")) {
		const Json::Value& x(cmdin["vfr"]);
		m_autoroute->set_departure_ifr(x.isBool() && !x.asBool());
	}
	{
		Point pt(get_point(cmdin, "sid"));
		if (pt.is_invalid()) {
			if (cmdin.isMember("sidident"))
				m_autoroute->set_sid(cmdin.get("sidident", "").asString());
		} else {
			m_autoroute->set_sid(pt);
		}
	}
	if (cmdin.isMember("sidlimit"))
		m_autoroute->set_sidlimit(cmdin["sidlimit"].asDouble());
	if (cmdin.isMember("sidpenalty"))
		m_autoroute->set_sidpenalty(cmdin["sidpenalty"].asDouble());
	if (cmdin.isMember("sidoffset"))
		m_autoroute->set_sidoffset(cmdin["sidoffset"].asDouble());
	if (cmdin.isMember("sidminimum"))
		m_autoroute->set_sidminimum(cmdin["sidminimum"].asDouble());
	if (cmdin.isMember("siddb"))
		m_autoroute->set_siddb(cmdin["siddb"].asBool());
	if (cmdin.isMember("sidonly"))
		m_autoroute->set_sidonly(cmdin["sidonly"].asBool());
	if (cmdin.isMember("sidfilter")) {
		const Json::Value& filters(cmdin["sidfilter"]);
		if (filters.isArray()) {
			m_autoroute->clear_sidfilter();
                        for (Json::ArrayIndex i = 0; i < filters.size(); ++i) {
                                const Json::Value& filter(filters[i]);
                                if (!filter.isString())
                                        continue;
                                std::string s(filter.asString());
                                if (s.empty())
                                        continue;
                                m_autoroute->add_sidfilter(s);
                        }
                }
	}
	if (cmdin.isMember("time"))
		m_autoroute->set_deptime(cmdin["time"].asUInt());
	if (cmdin.isMember("date")) {
		Glib::TimeVal tv;
		if (tv.assign_from_iso8601(cmdin["date"].asString()))
			m_autoroute->set_deptime(tv.tv_sec);
	}
	if (m_autoroute->get_departure().is_valid()) {
		cmdout["icao"] = (std::string)m_autoroute->get_departure().get_icao();
		cmdout["name"] = (std::string)m_autoroute->get_departure().get_name();
		set_point(cmdout, "coord", m_autoroute->get_departure().get_coord());
	}
	if (m_autoroute->get_sid().is_invalid()) {
		cmdout.removeMember("sid");
	} else {
	        set_point(cmdout, "sid", m_autoroute->get_sid());
		cmdout["sidtype"] = to_str(m_autoroute->get_sidtype());
		cmdout["sidident"] = m_autoroute->get_sidident();
	}
	cmdout["sidlimit"] = m_autoroute->get_sidlimit();
	cmdout["sidpenalty"] = m_autoroute->get_sidpenalty();
	cmdout["sidoffset"] = m_autoroute->get_sidoffset();
	cmdout["sidminimum"] = m_autoroute->get_sidminimum();
	cmdout["siddb"] = m_autoroute->get_siddb();
	cmdout["sidonly"] = m_autoroute->get_sidonly();
	{
		const CFMUAutoroute::sidstarfilter_t& fi(m_autoroute->get_sidfilter());
		Json::Value filt;
		for (CFMUAutoroute::sidstarfilter_t::const_iterator i(fi.begin()), e(fi.end()); i != e; ++i)
			filt.append(Json::Value(*i));
		cmdout["sidfilter"] = filt;
	}
	if (m_autoroute->get_departure_ifr()) {
		cmdout["ifr"] = true;
		cmdout.removeMember("vfr");
	} else {
		cmdout["vfr"] = true;
		cmdout.removeMember("ifr");
	}
	cmdout["time"] = (Json::LargestInt)m_autoroute->get_deptime();
	{
		Glib::TimeVal tv(m_autoroute->get_deptime(), 0);
		cmdout["date"] = (std::string)tv.as_iso8601();
	}
}

void SocketServer::cmd_destination(const Json::Value& cmdin, Json::Value& cmdout)
{
	if (!m_autoroute) {
		cmdout["error"] = "no autoroute instance";
		return;
	}
	if (cmdin.isMember("icao") || cmdin.isMember("name")) {
		std::string icao(cmdin.get("icao", "").asString());
		std::string name(cmdin.get("name", "").asString());
		if (!m_autoroute->set_destination(icao, name))
			throw std::runtime_error("destination aerodrome " + icao + " " + name + " not found");
	}
	if (cmdin.isMember("ifr")) {
		const Json::Value& x(cmdin["ifr"]);
		m_autoroute->set_destination_ifr(!x.isBool() || x.asBool());
	}
	if (cmdin.isMember("vfr")) {
		const Json::Value& x(cmdin["vfr"]);
		m_autoroute->set_destination_ifr(x.isBool() && !x.asBool());
	}
	{
		Point pt(get_point(cmdin, "star"));
		if (pt.is_invalid()) {
			if (cmdin.isMember("starident"))
				m_autoroute->set_star(cmdin.get("starident", "").asString());
		} else {
			m_autoroute->set_star(pt);
		}
	}
	if (cmdin.isMember("starlimit"))
		m_autoroute->set_starlimit(cmdin["starlimit"].asDouble());
	if (cmdin.isMember("starpenalty"))
		m_autoroute->set_starpenalty(cmdin["starpenalty"].asDouble());
	if (cmdin.isMember("staroffset"))
		m_autoroute->set_staroffset(cmdin["staroffset"].asDouble());
	if (cmdin.isMember("starminimum"))
		m_autoroute->set_starminimum(cmdin["starminimum"].asDouble());
	if (cmdin.isMember("stardb"))
		m_autoroute->set_stardb(cmdin["stardb"].asBool());
	if (cmdin.isMember("staronly"))
		m_autoroute->set_staronly(cmdin["staronly"].asBool());
	if (cmdin.isMember("starfilter")) {
		const Json::Value& filters(cmdin["starfilter"]);
		if (filters.isArray()) {
			m_autoroute->clear_starfilter();
                        for (Json::ArrayIndex i = 0; i < filters.size(); ++i) {
                                const Json::Value& filter(filters[i]);
                                if (!filter.isString())
                                        continue;
                                std::string s(filter.asString());
                                if (s.empty())
                                        continue;
                                m_autoroute->add_starfilter(s);
                        }
                }
	}
	if (cmdin.isMember("alternate1"))
		m_autoroute->set_alternate(0, cmdin.get("alternate1", "").asString());
	if (cmdin.isMember("alternate2"))
		m_autoroute->set_alternate(1, cmdin.get("alternate2", "").asString());
	if (m_autoroute->get_destination().is_valid()) {
		cmdout["icao"] = (std::string)m_autoroute->get_destination().get_icao();
		cmdout["name"] = (std::string)m_autoroute->get_destination().get_name();
		set_point(cmdout, "coord", m_autoroute->get_destination().get_coord());
	}
	if (m_autoroute->get_star().is_invalid()) {
		cmdout.removeMember("star");
	} else {
	        set_point(cmdout, "star", m_autoroute->get_star());
		cmdout["startype"] = to_str(m_autoroute->get_startype());
		cmdout["starident"] = m_autoroute->get_starident();
	}
	cmdout["starlimit"] = m_autoroute->get_starlimit();
	cmdout["starpenalty"] = m_autoroute->get_starpenalty();
	cmdout["staroffset"] = m_autoroute->get_staroffset();
	cmdout["starminimum"] = m_autoroute->get_starminimum();
	cmdout["stardb"] = m_autoroute->get_stardb();
	cmdout["staronly"] = m_autoroute->get_staronly();
	{
		const CFMUAutoroute::sidstarfilter_t& fi(m_autoroute->get_starfilter());
		Json::Value filt;
		for (CFMUAutoroute::sidstarfilter_t::const_iterator i(fi.begin()), e(fi.end()); i != e; ++i)
			filt.append(Json::Value(*i));
		cmdout["starfilter"] = filt;
	}
	if (m_autoroute->get_destination_ifr()) {
		cmdout["ifr"] = true;
		cmdout.removeMember("vfr");
	} else {
		cmdout["vfr"] = true;
		cmdout.removeMember("ifr");
	}
	if (!m_autoroute->get_alternate(0).empty())
		cmdout["alternate1"] = m_autoroute->get_alternate(0);
	if (!m_autoroute->get_alternate(1).empty())
		cmdout["alternate2"] = m_autoroute->get_alternate(1);
}

void SocketServer::cmd_crossing(const Json::Value& cmdin, Json::Value& cmdout)
{
	if (!m_autoroute) {
		cmdout["error"] = "no autoroute instance";
		return;
	}
	if (cmdin.isMember("crossing")) {
		const Json::Value& crossing(cmdin["crossing"]);
		if (crossing.isArray()) {
			unsigned int index = 0;
			m_autoroute->set_crossing_size(0);
			for (unsigned int i = 0; i < crossing.size(); ++i) {
				const Json::Value& el(crossing[i]);
				if (!el.isObject())
					continue;
				{
					bool ok(false);
					Point pt(get_point(el, "coord"));
					if (pt.is_invalid()) {
						if (el.isMember("ident"))
							ok = m_autoroute->set_crossing(index, el.get("ident", "").asString());
					} else {
						m_autoroute->set_crossing(index, pt);
					        ok = true;
					}
					if (!ok)
						continue;
				}
				if (el.isMember("radius"))
					m_autoroute->set_crossing_radius(index, el["radius"].asDouble());
				if (el.isMember("minlevel") || el.isMember("maxlevel")) {
					int minlevel(m_autoroute->get_crossing_minlevel(index));
					int maxlevel(m_autoroute->get_crossing_maxlevel(index));
					if (el.isMember("minlevel"))
						minlevel = el["minlevel"].asInt();
					if (el.isMember("maxlevel"))
						maxlevel = el["maxlevel"].asInt();
					m_autoroute->set_crossing_level(index, minlevel, maxlevel);
				}
				++index;
			}
		}
	}
	Json::Value crossings;
	for (unsigned int i = 0; i < m_autoroute->get_crossing_size(); ++i) {
		Json::Value crossing;
		set_point(crossing, "coord", m_autoroute->get_crossing(i));
		crossing["type"] = to_str(m_autoroute->get_crossing_type(i));
		crossing["ident"] = m_autoroute->get_crossing_ident(i);
		crossing["radius"] = m_autoroute->get_crossing_radius(i);
		crossing["minlevel"] = m_autoroute->get_crossing_minlevel(i);
		crossing["maxlevel"] = m_autoroute->get_crossing_maxlevel(i);
		crossings.append(crossing);
	}
	cmdout["crossing"] = crossings;
}

void SocketServer::cmd_enroute(const Json::Value& cmdin, Json::Value& cmdout)
{
	if (!m_autoroute) {
		cmdout["error"] = "no autoroute instance";
		return;
	}
	if (cmdin.isMember("dctlimit"))
		m_autoroute->set_dctlimit(cmdin["dctlimit"].asDouble());
	if (cmdin.isMember("dctpenalty"))
		m_autoroute->set_dctpenalty(cmdin["dctpenalty"].asDouble());
	if (cmdin.isMember("dctoffset"))
		m_autoroute->set_dctoffset(cmdin["dctoffset"].asDouble());
	if (cmdin.isMember("vfraspclimit"))
		m_autoroute->set_vfraspclimit(cmdin["vfraspclimit"].asDouble());
	if (cmdin.isMember("forceenrouteifr"))
		m_autoroute->set_force_enroute_ifr(!!cmdin["forceenrouteifr"].asBool());
	if (cmdin.isMember("honourawylevels"))
		m_autoroute->set_honour_awy_levels(!!cmdin["honourawylevels"].asBool());
	if (cmdin.isMember("honourprofilerules"))
		m_autoroute->set_honour_profilerules(!!cmdin["honourprofilerules"].asBool());
	cmdout["dctlimit"] = m_autoroute->get_dctlimit();
	cmdout["dctpenalty"] = m_autoroute->get_dctpenalty();
	cmdout["dctoffset"] = m_autoroute->get_dctoffset();
	cmdout["vfraspclimit"] = m_autoroute->get_vfraspclimit();
	cmdout["forceenrouteifr"] = !!m_autoroute->get_force_enroute_ifr();
	cmdout["honourawylevels"] = !!m_autoroute->get_honour_awy_levels();
	cmdout["honourprofilerules"] = !!m_autoroute->get_honour_profilerules();
}

void SocketServer::cmd_levels(const Json::Value& cmdin, Json::Value& cmdout)
{
	if (!m_autoroute) {
		cmdout["error"] = "no autoroute instance";
		return;
	}
	{
		int b(m_autoroute->get_baselevel());
		int t(m_autoroute->get_toplevel());
		if (cmdin.isMember("base"))
			b = cmdin["base"].asInt();
		if (cmdin.isMember("top"))
			t = cmdin["top"].asInt();
		m_autoroute->set_levels(b, t);
	}
	if (cmdin.isMember("maxdescent"))
		m_autoroute->set_maxdescent(cmdin["maxdescent"].asDouble());
	if (cmdin.isMember("honourlevelchangetrackmiles"))
		m_autoroute->set_honour_levelchangetrackmiles(!!cmdin["honourlevelchangetrackmiles"].asBool());
	if (cmdin.isMember("honouropsperftrackmiles"))
		m_autoroute->set_honour_opsperftrackmiles(!!cmdin["honouropsperftrackmiles"].asBool());
	cmdout["base"] = m_autoroute->get_baselevel();
	cmdout["top"] = m_autoroute->get_toplevel();
	cmdout["maxdescent"] = m_autoroute->get_maxdescent();
	cmdout["honouropsperftrackmiles"] = !!m_autoroute->get_honour_opsperftrackmiles();
}

void SocketServer::cmd_exclude(const Json::Value& cmdin, Json::Value& cmdout)
{
	if (!m_autoroute) {
		cmdout["error"] = "no autoroute instance";
		return;
	}
	if (cmdin.isMember("clear")) {
		const Json::Value& v(cmdin["clear"]);
		if (!v.isBool() || v.asBool())
			m_autoroute->clear_excluderegions();
	}
	bool indexf(cmdin.isMember("index"));
	unsigned int indexv(0);
	if (indexf)
		indexv = cmdin["index"].asUInt();
	{
		int ob(0), ot(999);
		double awylimit(0), dctlimit(0), dctoffset(0), dctscale(1);
		if (cmdin.isMember("base"))
			ob = cmdin["base"].asInt();
		if (cmdin.isMember("top"))
			ot = cmdin["top"].asInt();
		if (cmdin.isMember("awylimit"))
			awylimit = cmdin["awylimit"].asDouble();
		if (cmdin.isMember("dctlimit"))
			dctlimit = cmdin["dctlimit"].asDouble();
		if (cmdin.isMember("dctoffset"))
			dctoffset = cmdin["dctoffset"].asDouble();
		if (cmdin.isMember("dctscale"))
			dctscale = cmdin["dctscale"].asDouble();
		if (cmdin.isMember("aspcid")) {
			m_autoroute->add_excluderegion(CFMUAutoroute::ExcludeRegion(cmdin["aspcid"].asString(), cmdin.get("aspctype", "").asString(),
										    ob, ot, awylimit, dctlimit, dctoffset, dctscale));
			if (!indexf) {
				indexv = m_autoroute->get_excluderegions().size() - 1;
				indexf = true;
			}
		}
		Point ptsw(get_point(cmdin, "sw"));
		Point ptne(get_point(cmdin, "ne"));
		if (!ptsw.is_invalid() && !ptne.is_invalid()) {
			m_autoroute->add_excluderegion(CFMUAutoroute::ExcludeRegion(Rect(ptsw, ptne), ob, ot, awylimit, dctlimit, dctoffset, dctscale));
			if (!indexf) {
				indexv = m_autoroute->get_excluderegions().size() - 1;
				indexf = true;
			}
		}
	}
	cmdout["count"] = (Json::LargestUInt)m_autoroute->get_excluderegions().size();
	if (indexf && indexv < m_autoroute->get_excluderegions().size()) {
		const CFMUAutoroute::ExcludeRegion& er(m_autoroute->get_excluderegions()[indexv]);
		cmdout["base"] = er.get_minlevel();
		cmdout["top"] = er.get_maxlevel();
		cmdout["awylimit"] = er.get_awylimit();
		cmdout["dctlimit"] = er.get_dctlimit();
		cmdout["dctoffset"] = er.get_dctoffset();
		cmdout["dctscale"] = er.get_dctscale();
		if (er.is_airspace()) {
			cmdout["aspcid"] = er.get_airspace_id();
			cmdout["aspctype"] = er.get_airspace_type();
		} else {
			set_point(cmdout, "sw", er.get_bbox().get_southwest());
			set_point(cmdout, "ne", er.get_bbox().get_northeast());
		}
	}
}

void SocketServer::cmd_tfr(const Json::Value& cmdin, Json::Value& cmdout)
{
	if (!m_autoroute) {
		cmdout["error"] = "no autoroute instance";
		return;
	}
	if (cmdin.isMember("enabled"))
		m_autoroute->set_tfr_enabled(!!cmdin["enabled"].asBool());
	if (cmdin.isMember("trace"))
		m_autoroute->set_tfr_trace(cmdin["trace"].asString());
	if (cmdin.isMember("disable"))
		m_autoroute->set_tfr_disable(cmdin["disable"].asString());
	if (cmdin.isMember("precompgraph"))
		m_autoroute->set_precompgraph_enabled(!!cmdin["precompgraph"].asBool());
	if (cmdin.isMember("maxlocaliterations"))
		m_autoroute->set_maxlocaliteration(cmdin["maxlocaliterations"].asUInt());
	if (cmdin.isMember("maxremoteiterations"))
		m_autoroute->set_maxremoteiteration(cmdin["maxremoteiterations"].asUInt());
	cmdout["enabled"] = !!m_autoroute->get_tfr_enabled();
	cmdout["available"] = !!m_autoroute->get_tfr_available();
	cmdout["trace"] = m_autoroute->get_tfr_trace();
	cmdout["disable"] = m_autoroute->get_tfr_disable();
	cmdout["precompgraph"] = !!m_autoroute->get_precompgraph_enabled();
	cmdout["maxlocaliterations"] = m_autoroute->get_maxlocaliteration();
	cmdout["maxremoteiterations"] = m_autoroute->get_maxremoteiteration();
}

void SocketServer::cmd_atmosphere(const Json::Value& cmdin, Json::Value& cmdout)
{
	if (!m_autoroute) {
		cmdout["error"] = "no autoroute instance";
		return;
	}
	if (cmdin.isMember("qnh"))
		m_autoroute->set_qnh(cmdin["qnh"].asDouble());
	if (cmdin.isMember("isa"))
		m_autoroute->set_isaoffs(cmdin["isa"].asDouble());
	if (cmdin.isMember("wind"))
		m_autoroute->set_wind_enabled(!!cmdin["wind"].asBool());
	cmdout["qnh"] = m_autoroute->get_qnh();
	cmdout["isa"] = m_autoroute->get_isaoffs();
	cmdout["wind"] = !!m_autoroute->get_wind_enabled();
}

void SocketServer::cmd_cruise(const Json::Value& cmdin, Json::Value& cmdout)
{
	if (!m_autoroute) {
		cmdout["error"] = "no autoroute instance";
		return;
	}
	if (cmdin.isMember("rpm") || cmdin.isMember("mp") || cmdin.isMember("bhp")) {
		double rpm(std::numeric_limits<double>::quiet_NaN());
		double mp(rpm), bhp(rpm);
		if (cmdin.isMember("rpm"))
			rpm = cmdin["rpm"].asDouble();
		if (cmdin.isMember("mp"))
			mp = cmdin["mp"].asDouble();
		if (cmdin.isMember("bhp"))
			bhp = cmdin["bhp"].asDouble();
		m_autoroute->set_engine_rpm(rpm);
		m_autoroute->set_engine_mp(mp);
     		m_autoroute->set_engine_bhp(bhp);
	}
	double rpm(m_autoroute->get_engine_rpm()), mp(m_autoroute->get_engine_mp()), bhp(m_autoroute->get_engine_bhp());
	if (!std::isnan(rpm))
		cmdout["rpm"] = rpm;
	if (!std::isnan(mp))
		cmdout["mp"] = mp;
	if (!std::isnan(bhp))
		cmdout["bhp"] = bhp;
}

void SocketServer::cmd_optimization(const Json::Value& cmdin, Json::Value& cmdout)
{
	if (!m_autoroute) {
		cmdout["error"] = "no autoroute instance";
		return;
	}
	if (cmdin.isMember("target")) {
		std::string tgt(cmdin["target"].asString());
		static const CFMUAutoroute::opttarget_t ot[] = {
			CFMUAutoroute::opttarget_time,
			CFMUAutoroute::opttarget_fuel,
			CFMUAutoroute::opttarget_preferred
		};
		unsigned int i;
		for (i = 0; i < sizeof(ot)/sizeof(ot[0]); ++i)
			if (::to_str(ot[i]) == tgt)
				break;
		if (i >= sizeof(ot)/sizeof(ot[0])) {
			cmdout["error"] = "invalid target=" + tgt;
			return;
		}
		m_autoroute->set_opttarget(ot[i]);
	}
	cmdout["target"] = (std::string)to_str(m_autoroute->get_opttarget());
}

void SocketServer::cmd_preferred(const Json::Value& cmdin, Json::Value& cmdout)
{
	if (!m_autoroute) {
		cmdout["error"] = "no autoroute instance";
		return;
	}
	if (cmdin.isMember("level"))
		m_autoroute->set_preferred_level(cmdin["level"].asInt());
	if (cmdin.isMember("penalty"))
		m_autoroute->set_preferred_penalty(cmdin["penalty"].asDouble());
	if (cmdin.isMember("climb"))
		m_autoroute->set_preferred_climb(cmdin["climb"].asDouble());
	if (cmdin.isMember("descent"))
		m_autoroute->set_preferred_descent(cmdin["descent"].asDouble());
	cmdout["level"] = m_autoroute->get_preferred_level();
	cmdout["penalty"] = m_autoroute->get_preferred_penalty();
	cmdout["climb"] = m_autoroute->get_preferred_climb();
	cmdout["descent"] = m_autoroute->get_preferred_descent();
}

void SocketServer::cmd_aircraft(const Json::Value& cmdin, Json::Value& cmdout)
{
	if (!m_autoroute) {
		cmdout["error"] = "no autoroute instance";
		return;
	}
	bool dirty(false);
	if (cmdin.isMember("file")) {
		bool status(!!m_autoroute->get_aircraft().load_file(cmdin["file"].asString()));
		cmdout["status"] = status;
		dirty = status && (m_autoroute->get_aircraft().get_dist().recalculatepoly(false) ||
				   m_autoroute->get_aircraft().get_climb().recalculatepoly(false) ||
				   m_autoroute->get_aircraft().get_descent().recalculatepoly(false));
	}
	if (cmdin.isMember("data")) {
		bool status(!!m_autoroute->get_aircraft().load_string(cmdin["data"].asString()));
		cmdout["status"] = status;
		dirty = status && (m_autoroute->get_aircraft().get_dist().recalculatepoly(false) ||
				   m_autoroute->get_aircraft().get_climb().recalculatepoly(false) ||
				   m_autoroute->get_aircraft().get_descent().recalculatepoly(false));
	}
	if (cmdin.isMember("registration"))
		m_autoroute->get_aircraft().set_callsign(cmdin["registration"].asString());
	if (cmdin.isMember("type"))
		m_autoroute->get_aircraft().set_icaotype(cmdin["type"].asString());
	if (cmdin.isMember("transponder"))
		m_autoroute->get_aircraft().set_transponder(cmdin["transponder"].asString());
	{
		bool fixeq(false);
		if (cmdin.isMember("equipment")) {
			m_autoroute->get_aircraft().set_equipment(cmdin["equipment"].asString());
			fixeq = true;
		}
		if (cmdin.isMember("pbn")) {
			m_autoroute->get_aircraft().set_pbn(cmdin["pbn"].asString());
			fixeq = true;
		}
		if (fixeq)
			m_autoroute->get_aircraft().pbn_fix_equipment();
	}
	cmdout["registration"] = (std::string)m_autoroute->get_aircraft().get_callsign();
	cmdout["manufacturer"] = (std::string)m_autoroute->get_aircraft().get_manufacturer();
	cmdout["model"] = (std::string)m_autoroute->get_aircraft().get_model();
	cmdout["year"] = (std::string)m_autoroute->get_aircraft().get_year();
	cmdout["description"] = (std::string)m_autoroute->get_aircraft().get_description();
	cmdout["type"] = (std::string)m_autoroute->get_aircraft().get_icaotype();
	cmdout["equipment"] = (std::string)m_autoroute->get_aircraft().get_equipment();
	cmdout["transponder"] = (std::string)m_autoroute->get_aircraft().get_transponder();
	cmdout["pbn"] = (std::string)m_autoroute->get_aircraft().get_pbn_string();
	cmdout["data"] = (std::string)m_autoroute->get_aircraft().save_string();
	cmdout["dirty"] = dirty;
}

void SocketServer::cmd_start(const Json::Value& cmdin, Json::Value& cmdout)
{
	if (!m_autoroute) {
		cmdout["error"] = "no autoroute instance";
		return;
	}
	m_autoroute->start();
}

void SocketServer::cmd_stop(const Json::Value& cmdin, Json::Value& cmdout)
{
	if (!m_autoroute) {
		cmdout["error"] = "no autoroute instance";
		return;
	}
	m_autoroute->stop(CFMUAutoroute::statusmask_stoppingerroruser);
}

void SocketServer::cmd_continue(const Json::Value& cmdin, Json::Value& cmdout)
{
	if (!m_autoroute) {
		cmdout["error"] = "no autoroute instance";
		return;
	}
	m_autoroute->cont();
}

void SocketServer::cmd_preload(const Json::Value& cmdin, Json::Value& cmdout)
{
	if (!m_autoroute) {
		cmdout["error"] = "no autoroute instance";
		return;
	}
	m_autoroute->preload();
}

void SocketServer::cmd_clear(const Json::Value& cmdin, Json::Value& cmdout)
{
	if (!m_autoroute) {
		cmdout["error"] = "no autoroute instance";
		return;
	}
	m_autoroute->clear();
}

void SocketServer::cmd_fplparse(const Json::Value& cmdin, Json::Value& cmdout)
{
	if (!m_autoroute) {
		cmdout["error"] = "no autoroute instance";
		return;
	}
	if (!cmdin.isMember("fpl") || (!cmdin["fpl"].isString() && !cmdin["fpl"].isObject())) {
		cmdout["error"] = "no flight plan given";
		return;
	}
	FPlanRoute route(*(FPlan *)0);
	if (cmdin["fpl"].isString()) {
		std::string fpl(cmdin["fpl"].asString());
		cmdout["fpl"] = fpl;
		Engine eng(m_autoroute->get_db_maindir(), Engine::auxdb_override, m_autoroute->get_db_auxdir(), false, false);
		IcaoFlightPlan f(eng);
	        IcaoFlightPlan::errors_t err(f.parse(fpl, true));
		if (!err.empty()) {
			if (err.size() == 1) {
				cmdout["error"] = err[0];
			} else {
				Json::Value& er(cmdout["error"]);
				for (IcaoFlightPlan::errors_t::const_iterator i(err.begin()), e(err.end()); i != e; ++i)
					er.append(Json::Value(*i));
			}
		}
		if (cmdin.isMember("recompute") && cmdin["recompute"].isBool() && cmdin["recompute"].asBool())
			f.set_aircraft(m_autoroute->get_aircraft(),
				       Aircraft::Cruise::EngineParams(m_autoroute->get_engine_bhp(), m_autoroute->get_engine_rpm(),
								      m_autoroute->get_engine_mp()));
		f.set_route(route);
		cmdout["aircraftid"] = f.get_aircraftid();
		cmdout["flightrules"] = f.get_flightrules();
		cmdout["flighttype"] = f.get_flighttype();
		cmdout["number"] = f.get_number();
		cmdout["aircrafttype"] = f.get_aircrafttype();
		cmdout["wakecategory"] = std::string(1, f.get_wakecategory());
		cmdout["equipment"] = f.get_equipment();
		cmdout["transponder"] = f.get_transponder();
		cmdout["pbn"] = f.get_pbn_string();
		cmdout["cruisespeed"] = f.get_cruisespeed();
		cmdout["departure"] = f.get_departure();
		cmdout["destination"] = f.get_destination();
		cmdout["alternate1"] = f.get_alternate1();
		cmdout["alternate2"] = f.get_alternate2();
		cmdout["departuretime"] = (Json::LargestInt)f.get_departuretime();
		cmdout["totaleet"] = (Json::LargestInt)f.get_totaleet();
		cmdout["endurance"] = (Json::LargestInt)f.get_endurance();
		cmdout["personsonboard"] = f.get_personsonboard();
		cmdout["colourmarkings"] = f.get_colourmarkings();
		cmdout["remarks"] = f.get_remarks();
		cmdout["picname"] = f.get_picname();
		cmdout["emergencyradio"] = f.get_emergencyradio();
		cmdout["survival"] = f.get_survival();
		cmdout["lifejackets"] = f.get_lifejackets();
		cmdout["dinghies"] = f.get_dinghies();
		cmdout["item15"] = f.get_item15();
	} else if (cmdin["fpl"].isObject()) {
		decode_fpl(route, cmdin["fpl"]);
	}
	route.turnpoints();
	route.set_winddir(0);
	route.set_windspeed(0);
	route.set_qff_hpa(m_autoroute->get_qnh());
	route.set_isaoffset_kelvin(m_autoroute->get_isaoffs());
	FPlanRoute::GFSResult gfsr;
	if (cmdin.isMember("recompute") && cmdin["recompute"].isBool() && cmdin["recompute"].asBool()) {
		route.recompute(m_autoroute->get_aircraft(), m_autoroute->get_qnh(), m_autoroute->get_isaoffs(),
				Aircraft::Cruise::EngineParams(m_autoroute->get_engine_bhp(), m_autoroute->get_engine_rpm(), m_autoroute->get_engine_mp()));
		bool gfs(cmdin.isMember("gfs") && cmdin["gfs"].isBool() && cmdin["gfs"].asBool());
		if (gfs) {
			Json::Value zw;
			encode_fpl(zw, route);
			gfsr = route.gfs(m_autoroute->get_grib2());
			if (gfsr.is_modified() && !gfsr.is_partial()) {
				cmdout["zerowind"] = zw;
				route.recompute(m_autoroute->get_aircraft(), m_autoroute->get_qnh(), m_autoroute->get_isaoffs(),
						Aircraft::Cruise::EngineParams(m_autoroute->get_engine_bhp(), m_autoroute->get_engine_rpm(), m_autoroute->get_engine_mp()));
				cmdout["gfsminreftime"] = (std::string)Glib::TimeVal(gfsr.get_minreftime(), 0).as_iso8601();
				cmdout["gfsmaxreftime"] = (std::string)Glib::TimeVal(gfsr.get_maxreftime(), 0).as_iso8601();
				cmdout["gfsminefftime"] = (std::string)Glib::TimeVal(gfsr.get_minefftime(), 0).as_iso8601();
				cmdout["gfsmaxefftime"] = (std::string)Glib::TimeVal(gfsr.get_maxefftime(), 0).as_iso8601();
			} else {
				gfs = false;
				route.set_winddir(0);
				route.set_windspeed(0);
				route.set_qff_hpa(m_autoroute->get_qnh());
				route.set_isaoffset_kelvin(m_autoroute->get_isaoffs());
			}
		}
	}
	encode_fpl(cmdout, route);
	if (cmdin.isMember("localvalidate") && cmdin["localvalidate"].isBool() && cmdin["localvalidate"].asBool())
		check_fplan(cmdout["localvalidate"], route);
	if (cmdin.isMember("wx") && cmdin["wx"].isBool() && cmdin["wx"].asBool())
		routeweather(cmdout["wx"], route);
	bool gramet(cmdin.isMember("gramet") && cmdin["gramet"].isBool() && cmdin["gramet"].asBool());
	std::vector<double> hpa;
	if (cmdin.isMember("wxchart")) {
		const Json::Value& wxch(cmdin["wxchart"]);
		if (wxch.isDouble()) {
			hpa.push_back(wxch.asDouble());
		} else if (wxch.isArray()) {
			for (Json::ArrayIndex i = 0; i < wxch.size(); ++i) {
                                const Json::Value& wxch1(wxch[i]);
				if (wxch1.isDouble())
					hpa.push_back(wxch1.asDouble());
			}
		}
	}
	if (gramet || !hpa.empty()) {
		bool svg(false);
		if (cmdin.isMember("wxchartformat") && cmdin["wxchartformat"].isString()) {
			if (cmdin["wxchartformat"].asString() == "svg")
				svg = true;
			else if (cmdin["wxchartformat"].asString() == "pdf")
				svg = false;
		}
		bool allowportrait(cmdin.isMember("wxchartportrait") && cmdin["wxchartportrait"].isBool() && cmdin["wxchartportrait"].asBool());
		double hdim(595.28), wdim(841.89), maxprofilepagedist(std::numeric_limits<double>::max());
		if (cmdin.isMember("wxchartwidth") && cmdin["wxchartwidth"].isDouble())
			wdim = cmdin["wxchartwidth"].asDouble();
		if (cmdin.isMember("wxchartheight") && cmdin["wxchartheight"].isDouble())
			hdim = cmdin["wxchartheight"].asDouble();
		if (cmdin.isMember("maxprofilepagedist") && cmdin["maxprofilepagedist"].isDouble())
			maxprofilepagedist = cmdin["maxprofilepagedist"].asDouble();
		double profilemaxalt(route.max_altitude() + 4000);
		MeteoProfile::yaxis_t profileymode(MeteoProfile::yaxis_altitude);
		if (cmdin.isMember("profileymode") && cmdin["profileymode"].isString()) {
			if (cmdin["profileymode"].asString() == "altitude")
				profileymode = MeteoProfile::yaxis_altitude;
			else if (cmdin["profileymode"].asString() == "pressure")
				profileymode = MeteoProfile::yaxis_pressure;
		}
		if (profileymode == MeteoProfile::yaxis_pressure)
			profilemaxalt = 60000;
		if (cmdin.isMember("profilemaxalt") && cmdin["profilemaxalt"].isDouble())
			profilemaxalt = cmdin["profilemaxalt"].asDouble();
		weathercharts(cmdout["wxcharts"], route, wdim, hdim, maxprofilepagedist, profileymode, profilemaxalt, svg, allowportrait, gramet, hpa);
	}
	if (cmdin.isMember("plog") && cmdin["plog"].isBool() && cmdin["plog"].asBool()) {
		std::string templ(PACKAGE_DATA_DIR "/navlog.xml");
		if (cmdin.isMember("plogtemplate") && cmdin["plogtemplate"].isString())
			templ = cmdin["plogtemplate"].asString();
		bool wbpage(true);
		if (cmdin.isMember("plogwbpage") && cmdin["plogwbpage"].isBool())
			wbpage = cmdin["plogwbpage"].asBool();
		navlog(cmdout["plog"], route, templ, gfsr, !wbpage);
	}
	{
		Engine eng(m_autoroute->get_db_maindir(), Engine::auxdb_override, m_autoroute->get_db_auxdir(), false, false);
		IcaoFlightPlan f(eng);
		f.populate(route, IcaoFlightPlan::awymode_keep, 50.0);
		f.set_aircraft(m_autoroute->get_aircraft(),
			       Aircraft::Cruise::EngineParams(m_autoroute->get_engine_bhp(), m_autoroute->get_engine_rpm(),
							      m_autoroute->get_engine_mp()));
		f.set_personsonboard(0);
		cmdout["fplall"] = f.get_fpl();
		if (!cmdin["fpl"].isString())
			cmdout["fpl"] = f.get_fpl();
		f.populate(route, IcaoFlightPlan::awymode_collapse, 50.0);
		f.set_aircraft(m_autoroute->get_aircraft(),
			       Aircraft::Cruise::EngineParams(m_autoroute->get_engine_bhp(), m_autoroute->get_engine_rpm(),
							      m_autoroute->get_engine_mp()));
		f.set_personsonboard(0);
		cmdout["fplawy"] = f.get_fpl();
		cmdout["fplatc"] = f.get_fpl();
		f.populate(route, IcaoFlightPlan::awymode_collapse_dct, 50.0);
		f.set_aircraft(m_autoroute->get_aircraft(),
			       Aircraft::Cruise::EngineParams(m_autoroute->get_engine_bhp(), m_autoroute->get_engine_rpm(),
							      m_autoroute->get_engine_mp()));
		f.set_personsonboard(0);
		cmdout["fplawydct"] = f.get_fpl();
		f.populate(route, IcaoFlightPlan::awymode_collapse_all, 50.0);
		f.set_aircraft(m_autoroute->get_aircraft(),
			       Aircraft::Cruise::EngineParams(m_autoroute->get_engine_bhp(), m_autoroute->get_engine_rpm(),
							      m_autoroute->get_engine_mp()));
		f.set_personsonboard(0);
		cmdout["fplmin"] = f.get_fpl();
	}
}

void SocketServer::cmd_fplparseadr(const Json::Value& cmdin, Json::Value& cmdout)
{
	if (!m_autoroute) {
		cmdout["error"] = "no autoroute instance";
		return;
	}
	if (!cmdin.isMember("fpl") || (!cmdin["fpl"].isString() && !cmdin["fpl"].isObject())) {
		cmdout["error"] = "no flight plan given";
		return;
	}
	ADR::FlightPlan route;
	if (cmdin["fpl"].isString()) {
		std::string fpl(cmdin["fpl"].asString());
		cmdout["fpl"] = fpl;
		ADR::Database db(m_autoroute->get_db_auxdir());
		ADR::FlightPlan::errors_t err(route.parse(db, fpl, true));
		if (!err.empty()) {
			if (err.size() == 1) {
				cmdout["error"] = err[0];
			} else {
				Json::Value& er(cmdout["error"]);
				for (ADR::FlightPlan::errors_t::const_iterator i(err.begin()), e(err.end()); i != e; ++i)
					er.append(Json::Value(*i));
			}
		}
		cmdout["aircraftid"] = route.get_aircraftid();
		cmdout["flightrules"] = route.get_flightrules();
		cmdout["flighttype"] = route.get_flighttype();
		cmdout["number"] = route.get_number();
		cmdout["aircrafttype"] = route.get_aircrafttype();
		cmdout["wakecategory"] = std::string(1, route.get_wakecategory());
		cmdout["equipment"] = route.get_equipment();
		cmdout["transponder"] = route.get_transponder();
		cmdout["pbn"] = route.get_pbn_string();
		cmdout["cruisespeed"] = route.get_cruisespeed();
		if (!route.empty()) {
			cmdout["departure"] = (std::string)route.front().get_icao();
			cmdout["destination"] = (std::string)route.back().get_icao();
		}
		cmdout["alternate1"] = route.get_alternate1();
		cmdout["alternate2"] = route.get_alternate2();
		cmdout["departuretime"] = (Json::LargestInt)route.get_departuretime();
		cmdout["totaleet"] = (Json::LargestInt)route.get_totaleet();
		cmdout["endurance"] = (Json::LargestInt)route.get_endurance();
		cmdout["personsonboard"] = route.get_personsonboard();
		cmdout["colourmarkings"] = route.get_colourmarkings();
		cmdout["remarks"] = route.get_remarks();
		cmdout["picname"] = route.get_picname();
		cmdout["emergencyradio"] = route.get_emergencyradio();
		cmdout["survival"] = route.get_survival();
		cmdout["lifejackets"] = route.get_lifejackets();
		cmdout["dinghies"] = route.get_dinghies();
		cmdout["item15"] = route.get_item15();
		//encode_fpl(cmdout["parsedfpl"], route);
	} else if (cmdin["fpl"].isObject()) {
		decode_fpl(route, cmdin["fpl"]);
		if (!cmdin.isMember("reparse") || !cmdin["reparse"].isBool() || cmdin["reparse"].asBool()) {
			ADR::Database db(m_autoroute->get_db_auxdir());
			ADR::FlightPlan::errors_t err(route.reparse(db, true));
			if (!err.empty()) {
				if (err.size() == 1) {
					cmdout["error"] = err[0];
				} else {
					Json::Value& er(cmdout["error"]);
					for (ADR::FlightPlan::errors_t::const_iterator i(err.begin()), e(err.end()); i != e; ++i)
						er.append(Json::Value(*i));
				}
			}
		}
	}
	route.turnpoints();
	route.set_winddir(0);
	route.set_windspeed(0);
	route.set_qff_hpa(m_autoroute->get_qnh());
	route.set_isaoffset_kelvin(m_autoroute->get_isaoffs());
	FPlanRoute::GFSResult gfsr;
	double secpernmi(std::numeric_limits<double>::quiet_NaN()), fuelpernmi(std::numeric_limits<double>::quiet_NaN());
	if (cmdin.isMember("recompute") && cmdin["recompute"].isBool() && cmdin["recompute"].asBool()) {
		route.set_aircraft(m_autoroute->get_aircraft(), m_autoroute->get_engine_rpm(), m_autoroute->get_engine_mp(),
				   m_autoroute->get_engine_bhp());
		route.recompute(m_autoroute->get_aircraft(), m_autoroute->get_qnh(), m_autoroute->get_isaoffs(),
				m_autoroute->get_engine_bhp(), m_autoroute->get_engine_rpm(), m_autoroute->get_engine_mp());
		bool gfs(cmdin.isMember("gfs") && cmdin["gfs"].isBool() && cmdin["gfs"].asBool());
		if (gfs) {
			Json::Value zw;
			encode_fpl(zw, route);
			gfsr = route.gfs(m_autoroute->get_grib2());
			if (gfsr.is_modified() && !gfsr.is_partial()) {
				cmdout["zerowind"] = zw;
				route.recompute(m_autoroute->get_aircraft(), m_autoroute->get_qnh(), m_autoroute->get_isaoffs(),
						m_autoroute->get_engine_bhp(), m_autoroute->get_engine_rpm(), m_autoroute->get_engine_mp());
				cmdout["gfsminreftime"] = (std::string)Glib::TimeVal(gfsr.get_minreftime(), 0).as_iso8601();
				cmdout["gfsmaxreftime"] = (std::string)Glib::TimeVal(gfsr.get_maxreftime(), 0).as_iso8601();
				cmdout["gfsminefftime"] = (std::string)Glib::TimeVal(gfsr.get_minefftime(), 0).as_iso8601();
				cmdout["gfsmaxefftime"] = (std::string)Glib::TimeVal(gfsr.get_maxefftime(), 0).as_iso8601();
			} else {
				gfs = false;
				route.set_winddir(0);
				route.set_windspeed(0);
				route.set_qff_hpa(m_autoroute->get_qnh());
				route.set_isaoffset_kelvin(m_autoroute->get_isaoffs());
			}
		}
		secpernmi = fuelpernmi = std::numeric_limits<double>::max(); 
		{
			double qnh(m_autoroute->get_qnh()), isaoffs(m_autoroute->get_isaoffs());
			if (std::isnan(qnh))
				qnh = IcaoAtmosphere<double>::std_sealevel_pressure;
			if (std::isnan(isaoffs))
				isaoffs = 0;
			double mass(m_autoroute->get_aircraft().get_mlm());
			if (!std::isnan(route.back().get_mass_kg()) && route.back().get_mass_kg())
				mass = Aircraft::convert(Aircraft::unit_kg, m_autoroute->get_aircraft().get_wb().get_massunit(), route.back().get_mass_kg());
			AirData<double> ad;
			ad.set_qnh_tempoffs(qnh, isaoffs);
			Aircraft::ClimbDescent climb(m_autoroute->get_aircraft().calculate_climb("", mass, isaoffs));
			for (int lvl(m_autoroute->get_baselevel()); lvl <= m_autoroute->get_toplevel(); lvl += 10) {
				double pa(lvl * 100);
				if (pa > climb.get_ceiling())
					break;
				double tas(0), fuelflow(0), rpm(m_autoroute->get_engine_rpm()), mp(m_autoroute->get_engine_mp());
				double bhp(m_autoroute->get_engine_bhp()), pa1(pa), mass1(mass), isaoffs1(isaoffs), qnh1(qnh);
				Aircraft::Cruise::EngineParams ep(bhp, rpm, mp);
				m_autoroute->get_aircraft().calculate_cruise(tas, fuelflow, pa1, mass1, isaoffs1, qnh1, ep);
				bhp = ep.get_bhp();
				rpm = ep.get_rpm();
				mp = ep.get_mp();
				double secpernmi1(3600.0 / tas);
				double fuelpernmi1(secpernmi1 * fuelflow * (1.0 / 3600.0));
				secpernmi = std::min(secpernmi, secpernmi1);
				fuelpernmi = std::min(fuelpernmi, fuelpernmi1);
			}
		}
	}
	if (cmdin.isMember("fireet") && cmdin["fireet"].isString()) {
		if (cmdin["fireet"].asString() == "add") {
			ADR::Database db(m_autoroute->get_db_auxdir());
			ADR::TimeTableSpecialDateEval ttsde;
			ttsde.load(db);
			route.add_fir_eet(db, ttsde);
		} else if (cmdin["fireet"].asString() == "clear") {
			ADR::Database db(m_autoroute->get_db_auxdir());
			route.remove_fir_eet(db);
		}
	}
	encode_fpl(cmdout, route);
	if (!route.empty()) {
		cmdout["routetime"] = route.back().get_flighttime();
		cmdout["routefuel"] = route.back().get_fuel_usg();
	}
	{
		double gcd(route.gc_distance_nmi_dbl());
		set_double(cmdout, "gcdist", gcd);
		set_double(cmdout, "routedist", route.total_distance_nmi_dbl());
		set_double(cmdout, "mintime", gcd * secpernmi);
		set_double(cmdout, "minfuel", gcd * fuelpernmi);
	}
	if (cmdin.isMember("localvalidate") && cmdin["localvalidate"].isBool() && cmdin["localvalidate"].asBool())
		check_fplan(cmdout["localvalidate"], route);
	if (cmdin.isMember("wx") && cmdin["wx"].isBool() && cmdin["wx"].asBool()) {
		FPlanRoute route2(*(FPlan *)0);
		for (ADR::FlightPlan::const_iterator ri(route.begin()), re(route.end()); ri != re; ++ri)
			route2.insert_wpt(~0, *ri);
		if (route2.get_nrwpt()) {
			route2.set_time_offblock_unix(route2[0].get_time_unix() - 5 * 60);
			route2.set_time_onblock_unix(route2[0].get_time_unix() + route2[route2.get_nrwpt() - 1].get_flighttime() + 5 * 60);
		}
		routeweather(cmdout["wx"], route2);
	}
	bool gramet(cmdin.isMember("gramet") && cmdin["gramet"].isBool() && cmdin["gramet"].asBool());
	std::vector<double> hpa;
	if (cmdin.isMember("wxchart")) {
		const Json::Value& wxch(cmdin["wxchart"]);
		if (wxch.isDouble()) {
			hpa.push_back(wxch.asDouble());
		} else if (wxch.isArray()) {
			for (Json::ArrayIndex i = 0; i < wxch.size(); ++i) {
                                const Json::Value& wxch1(wxch[i]);
				if (wxch1.isDouble())
					hpa.push_back(wxch1.asDouble());
			}
		}
	}
	if (gramet || !hpa.empty()) {
		bool svg(false);
		if (cmdin.isMember("wxchartformat") && cmdin["wxchartformat"].isString()) {
			if (cmdin["wxchartformat"].asString() == "svg")
				svg = true;
			else if (cmdin["wxchartformat"].asString() == "pdf")
				svg = false;
		}
		bool allowportrait(cmdin.isMember("wxchartportrait") && cmdin["wxchartportrait"].isBool() && cmdin["wxchartportrait"].asBool());
		double hdim(595.28), wdim(841.89), maxprofilepagedist(std::numeric_limits<double>::max());
		if (cmdin.isMember("wxchartwidth") && cmdin["wxchartwidth"].isDouble())
			wdim = cmdin["wxchartwidth"].asDouble();
		if (cmdin.isMember("wxchartheight") && cmdin["wxchartheight"].isDouble())
			hdim = cmdin["wxchartheight"].asDouble();
		if (cmdin.isMember("maxprofilepagedist") && cmdin["maxprofilepagedist"].isDouble())
			maxprofilepagedist = cmdin["maxprofilepagedist"].asDouble();
		FPlanRoute route2(*(FPlan *)0);
		for (ADR::FlightPlan::const_iterator ri(route.begin()), re(route.end()); ri != re; ++ri)
			route2.insert_wpt(~0, *ri);
		if (route2.get_nrwpt()) {
			route2.set_time_offblock_unix(route2[0].get_time_unix() - 5 * 60);
			route2.set_time_onblock_unix(route2[0].get_time_unix() + route2[route2.get_nrwpt() - 1].get_flighttime() + 5 * 60);
		}
		double profilemaxalt(route2.max_altitude() + 4000);
		MeteoProfile::yaxis_t profileymode(MeteoProfile::yaxis_altitude);
		if (cmdin.isMember("profileymode") && cmdin["profileymode"].isString()) {
			if (cmdin["profileymode"].asString() == "altitude")
				profileymode = MeteoProfile::yaxis_altitude;
			else if (cmdin["profileymode"].asString() == "pressure")
				profileymode = MeteoProfile::yaxis_pressure;
		}
		if (profileymode == MeteoProfile::yaxis_pressure)
			profilemaxalt = 60000;
		if (cmdin.isMember("profilemaxalt") && cmdin["profilemaxalt"].isDouble())
			profilemaxalt = cmdin["profilemaxalt"].asDouble();
		weathercharts(cmdout["wxcharts"], route2, wdim, hdim, maxprofilepagedist, profileymode, profilemaxalt, svg, allowportrait, gramet, hpa);
	}
	if (cmdin.isMember("plog") && cmdin["plog"].isBool() && cmdin["plog"].asBool()) {
		FPlanRoute route2(*(FPlan *)0);
		for (ADR::FlightPlan::const_iterator ri(route.begin()), re(route.end()); ri != re; ++ri)
			route2.insert_wpt(~0, *ri);
		std::string templ(PACKAGE_DATA_DIR "/navlog.xml");
		if (cmdin.isMember("plogtemplate") && cmdin["plogtemplate"].isString())
			templ = cmdin["plogtemplate"].asString();
		bool wbpage(true);
		if (cmdin.isMember("plogwbpage") && cmdin["plogwbpage"].isBool())
			wbpage = cmdin["plogwbpage"].asBool();
		navlog(cmdout["plog"], route2, templ, gfsr, !wbpage);
	}
	route.disable_none();
	cmdout["fplall"] = route.get_fpl();
	route.disable_unnecessary(true, false);
	cmdout["fplawy"] = route.get_fpl();
	route.disable_unnecessary(true, true);
	cmdout["fplawydct"] = route.get_fpl();
	route.disable_unnecessary(false, true);
	cmdout["fplmin"] = route.get_fpl();
	route.disable_unnecessary(false, false);
	cmdout["fplatc"] = route.get_fpl();
}

Json::Value SocketServer::servercmd(const Json::Value& cmdin)
{
	std::string cmdname(cmdin.get("cmdname", "").asString());
	cmdlist_t::const_iterator cmdi(m_servercmdlist.find(cmdname));
	if (cmdi == m_servercmdlist.end())
		return Json::Value();
	Json::Value reply;
	if (cmdin.isMember("cmdseq"))
		reply["cmdseq"] = cmdin["cmdseq"];
	reply["cmdname"] = cmdname;
	(this->*(cmdi->second))(cmdin, reply);
	return reply;
}

void SocketServer::servercmd_scangrib2(const Json::Value& cmdin, Json::Value& cmdout)
{
	if (!m_autoroute) {
		cmdout["error"] = "no autoroute instance";
		return;
	}
	typedef std::vector<std::string> pathlist_t;
	pathlist_t pathlist;
	if (cmdin.isMember("path")) {
		const Json::Value& paths(cmdin["path"]);
		if (paths.isString()) {
			pathlist.push_back(paths.asString());
		} else if (paths.isArray()) {
			for (Json::ArrayIndex i = 0; i < paths.size(); ++i) {
				const Json::Value& path(paths[i]);
				if (!path.isString())
					continue;
				pathlist.push_back(path.asString());
			}
		}
	}
	if (pathlist.empty()) {
		std::string path(m_autoroute->get_db_maindir());
		if (path.empty())
			path = FPlan::get_userdbpath();
		path = Glib::build_filename(path, "gfs");
		pathlist.push_back(path);
		if (!m_autoroute->get_db_auxdir().empty())
			pathlist.push_back(m_autoroute->get_db_auxdir());
	}
	Json::Value& paths(cmdout["paths"]);
	GRIB2& grib2(m_autoroute->get_grib2());
	GRIB2::Parser p(grib2);
	unsigned int count(grib2.find_layers().size());
	for (pathlist_t::const_iterator pi(pathlist.begin()), pe(pathlist.end()); pi != pe; ++pi) {
		p.parse(*pi);
		unsigned int count1(grib2.find_layers().size());
		Json::Value path;
		path["path"] = *pi;
		path["count"] = count1 - count;
		count = count1;
		paths.append(path);
	}
	cmdout["missing"] = grib2.remove_missing_layers();
	cmdout["obsolete"] = grib2.remove_obsolete_layers();
	m_autoroute->reload();
}

void SocketServer::servercmd_reloaddb(const Json::Value& cmdin, Json::Value& cmdout)
{
	if (!m_autoroute) {
		cmdout["error"] = "no autoroute instance";
		return;
	}
	m_autoroute->reload();	
}

void SocketServer::servercmd_processes(const Json::Value& cmdin, Json::Value& cmdout)
{
	Json::Value& rtrs(cmdout["router"]);
	{
		Glib::TimeVal tv;
		tv.assign_current_time();
		cmdout["time"] = (std::string)tv.as_iso8601();
	}
	for (routers_t::const_iterator ri(m_routers.begin()), re(m_routers.end()); ri != re; ++ri) {
		if (!ri->second)
			continue;
		Json::Value rtr;
		rtr["rxqueuesize"] = ri->second->get_recvqueue_size();
		rtr["accesstime"] = (std::string)ri->second->get_accesstime().as_iso8601();
		rtr["starttime"] = (std::string)ri->second->get_starttime().as_iso8601();
		rtr["session"] = ri->second->get_session();
		rtr["pid"] = ri->second->get_pid();
		rtr["running"] = ri->second->is_running();
		rtr["dead"] = ri->second->is_dead();
		rtrs.append(rtr);
	}
}

void SocketServer::servercmd_ruleinfo(const Json::Value& cmdin, Json::Value& cmdout)
{
	if (!m_autoroute) {
		cmdout["error"] = "no autoroute instance";
		return;
	}
	{
		CFMUAutoroute45 *ar45(dynamic_cast<CFMUAutoroute45 *>(m_autoroute));
		if (ar45) {
			cmdout["nrrules"] = ar45->get_tfr().count_rules();
			cmdout["nrdctrules"] = ar45->get_tfr().count_dct_rules();
			bool exact(!cmdin.isMember("exact") || !cmdin["exact"].isBool() || cmdin["exact"].asBool());
			std::vector<TrafficFlowRestrictions::RuleResult> res;
			if (cmdin.isMember("rules")) {
				const Json::Value& rules(cmdin["rules"]);
				if (rules.isString()) {
					res = ar45->get_tfr().find_rules(rules.asString(), exact);
				} else if (rules.isArray()) {
					for (unsigned int i = 0, n = rules.size(); i < n; ++i) {
						const Json::Value& rule(rules[i]);
						if (!rule.isString())
							continue;
						std::vector<TrafficFlowRestrictions::RuleResult> res1(ar45->get_tfr().find_rules(rule.asString(), exact));
						res.insert(res.end(), res1.begin(), res1.end());
					}
				}
			}
			{
				Json::Value& rules(cmdout["rules"]);
				for (std::vector<TrafficFlowRestrictions::RuleResult>::const_iterator i(res.begin()), e(res.end()); i != e; ++i) {
					Json::Value rule;
					encode_rule(rule, *i);
					rules.append(rule);
				}
			}
			return;
		}
	}
	{
		CFMUAutoroute51 *ar51(dynamic_cast<CFMUAutoroute51 *>(m_autoroute));
		if (ar51) {
			ADR::timetype_t tm(time(0));
			if (cmdin.isMember("time")) {
				if (cmdin["time"].isString()) {
					Glib::TimeVal tv;
					if (!tv.assign_from_iso8601(cmdin["time"].asString())) {
						cmdout["error"] = "cannot parse time " + cmdin["time"].asString();
						return;
					}
					tm = tv.tv_sec;
				} else if (cmdin["time"].isUInt()) {
					tm = cmdin["time"].asUInt();
				} else {
					cmdout["error"] = "time has unknown type";
					return;
				}
			}
			typedef std::vector<Glib::RefPtr<ADR::FlightRestriction> > rules_t;
			const rules_t& rules(ar51->get_tfr());
			cmdout["nrrules"] = (Json::LargestInt)rules.size();
			rules_t selected;
			bool exact(!cmdin.isMember("exact") || !cmdin["exact"].isBool() || cmdin["exact"].asBool());
			bool all(true);
			std::set<std::string> rulestr;
			if (cmdin.isMember("rules")) {
				const Json::Value& rules(cmdin["rules"]);
				if (rules.isString()) {
					all = false;
					std::string x(rules.asString());
					std::string::size_type n;
					while ((n = x.find(',')) != std::string::npos) {
						std::string y(x, 0, n);
						x.erase(0, n + 1);
						while (!y.empty() && std::isspace(y[0]))
							y.erase(0, 1);
						while (!y.empty() && std::isspace(y[y.size()-1]))
							y.erase(y.size() - 1);
						if (!y.empty())
							rulestr.insert(y);
					}
					while (!x.empty() && std::isspace(x[0]))
						x.erase(0, 1);
					while (!x.empty() && std::isspace(x[x.size()-1]))
						x.erase(x.size() - 1);
					if (!x.empty())
						rulestr.insert(x);
				} else if (rules.isArray()) {
					all = false;
					for (unsigned int i = 0, n = rules.size(); i < n; ++i) {
						const Json::Value& rule(rules[i]);
						if (!rule.isString())
							continue;
						std::string x(rules.asString());
						while (!x.empty() && std::isspace(x[0]))
						x.erase(0, 1);
						while (!x.empty() && std::isspace(x[x.size()-1]))
							x.erase(x.size() - 1);
						if (!x.empty())
							rulestr.insert(x);
					}
				}
			}
			{
				unsigned int dct(0);
				for (rules_t::const_iterator ri(rules.begin()), re(rules.end()); ri != re; ++ri) {
					const ADR::FlightRestrictionTimeSlice& ts((*ri)->operator()(tm).as_flightrestriction());
					if (!ts.is_valid())
						continue;
					dct += ts.is_dct();
					bool x(all);
					if (!x) {
						if (exact) {
							x = rulestr.find(ts.get_ident()) != rulestr.end();
						} else {
							for (std::set<std::string>::const_iterator ri(rulestr.begin()), re(rulestr.end()); ri != re && !x; ++ri) {
								x = ts.get_ident().find(*ri) != std::string::npos;
							}
						}
					}
					if (!x)
						continue;
					selected.push_back(*ri);
				}
				cmdout["nrdctrules"] = dct;
			}
			{
				Json::Value& rules(cmdout["rules"]);
				for (rules_t::const_iterator i(selected.begin()), e(selected.end()); i != e; ++i) {
					Json::Value rule;
					const ADR::FlightRestrictionTimeSlice& tsrule((*i)->operator()(tm).as_flightrestriction());
					if (!tsrule.is_valid()) {
						rule["codetype"] = "invalid";
						rules.append(rule);
						continue;
					}
					rule["codetype"] = to_str(tsrule.get_type());
					rule["procind"] = to_str(tsrule.get_procind());
					rule["dct"] = tsrule.is_dct();
					rule["strictdct"] = tsrule.is_strict_dct();
					rule["unconditional"] = tsrule.is_unconditional();
					rule["routestatic"] = tsrule.is_routestatic();
					rule["mandatoryinbound"] = tsrule.is_mandatoryinbound();
					rule["mandatoryoutbound"] = tsrule.is_mandatoryoutbound();
					rule["disabled"] = !tsrule.is_enabled();
					rule["trace"] = tsrule.is_trace();
					rule["time"] = (Json::LargestInt)tm;
					rule["desc"] = tsrule.get_instruction();
					// backward compatibility, should be removed in the future
					rule["oprgoal"] = "";
					if (tsrule.get_condition())
						rule["cond"] = tsrule.get_condition()->to_str(tm);
					Json::Value& ralternatives(rule["alternatives"]);
					for (unsigned int i = 0, n = tsrule.get_restrictions().size(); i < n; ++i) {
						Json::Value ralt;
						encode_rule(ralt["sequences"], tsrule.get_restrictions()[i].get_rule(), tm);
						ralternatives.append(ralt);
					}
					rules.append(rule);
				}
			}
			return;
		}
	}
}
