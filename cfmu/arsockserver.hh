#ifndef ARSOCKSERVER_H
#define ARSOCKSERVER_H

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "sysdeps.h"
#include <glibmm.h>
#include <giomm.h>
#include <giomm/socketservice.h>
#include <json/json.h>
#include <set>

#include "cfmuautoroute.hh"
#include "metgraph.h"

#include "adr.hh"
#include "adrdb.hh"
#include "adrfplan.hh"
#include "adrrestriction.hh"

class SocketServer {
public:
	SocketServer(CFMUAutoroute *autoroute, const Glib::RefPtr<Glib::MainLoop>& mainloop, unsigned int connlimit = ~0U,
		     unsigned int actlimit = ~0U, unsigned int timeout = ~0U, unsigned int maxruntime = ~0U, int xdisplay = -1,
		     bool logclient = true, bool logrouter = true, bool logsockcli = true, bool setprocname = false);
	~SocketServer();
	void listen(const std::string& path, uid_t socketuid, gid_t socketgid, mode_t socketmode, bool sdterminate);
	void sockclient(int fd, int xdisplay);
	bool is_sockclient(void) const { return !m_listenservice; }

protected:
	CFMUAutoroute *m_autoroute;
	Glib::RefPtr<Glib::MainLoop> m_mainloop;
	Glib::RefPtr<Gio::Socket> m_listensock;
	Glib::RefPtr<Gio::SocketAddress> m_listensockaddr;
	Glib::RefPtr<Gio::SocketService> m_listenservice;
	sigc::connection m_rtrtimeoutconn;
	std::string m_logdir;
        unsigned int m_connectlimit;
        unsigned int m_activelimit;
        unsigned int m_timeout;
	unsigned int m_maxruntime;
	int m_xdisplay;
	bool m_quit;
	bool m_logclient;
	bool m_logrouter;
	bool m_logsockcli;
	bool m_setprocname;

	void remove_listensockaddr(void);
	bool on_connect(const Glib::RefPtr<Gio::SocketConnection>& connection, const Glib::RefPtr<Glib::Object>& source_object);
	static int decode_json(Json::Value& root, std::string& rxbuf);

	class Router;

	class Client : public sigc::trackable {
	public:
		typedef Glib::RefPtr<Client> ptr_t;
		typedef Glib::RefPtr<const Client> const_ptr_t;
		Client(SocketServer *server, const Glib::RefPtr<Gio::SocketConnection>& connection, bool log);
		~Client();
		void reference(void) const;
		void unreference(void) const;
		void close(void);
		void shutdown(void);
		void send(const Json::Value& v);
		void msgavailable(void);
		ptr_t get_ptr(void) { reference(); return ptr_t(this); }
		const_ptr_t get_ptr(void) const { reference(); return const_ptr_t(this); }

	protected:
		SocketServer *m_server;
		std::string m_rxbuf;
		std::string m_txbuf;
		Glib::RefPtr<Gio::SocketConnection> m_connection;
		sigc::connection m_connin;
		sigc::connection m_connout;
		std::list<Json::Value> m_sendqueue;
		Glib::RefPtr<Router> m_longpoll;
		typedef std::set<std::string> stringset_t;
		stringset_t m_logfilter;
		stringset_t m_logdiscard;
		mutable gint m_refcount;
		unsigned int m_closeprotect;
		bool m_firstreply;
		bool m_shutdown;
		bool m_log;

		bool on_input(Glib::IOCondition iocond);
		bool on_output(Glib::IOCondition iocond);
		bool on_input_eof(Glib::IOCondition iocond);
	};

	typedef std::set<Client::ptr_t> clients_t;
	clients_t m_clients;

	void add_client(const Client::ptr_t& p);
	void remove_client(const Client::ptr_t& p);

	class Router : public sigc::trackable {
	public:
		typedef Glib::RefPtr<Router> ptr_t;
		typedef Glib::RefPtr<const Router> const_ptr_t;
		Router(SocketServer *server, pid_t pid, const Glib::RefPtr<Gio::Socket>& sock, const std::string& sess, int xdisplay, bool log);
		~Router();
		void reference(void) const;
		void unreference(void) const;
		void send(const Json::Value& v);
		typedef std::set<std::string> stringset_t;
		void receive(Json::Value& reply, const stringset_t& logfilter = stringset_t(), const stringset_t& logdiscard = stringset_t());
		void zap(void);
		void close(const std::string& status = "");
		bool is_running(void) const { return m_lifecycle == lifecycle_run; }
		bool is_dead(void) const { return m_lifecycle == lifecycle_dead; }
		bool handle_timeout(const Glib::TimeVal& tvacc, const Glib::TimeVal& tvstart);
		void set_longpoll(const Client::ptr_t& clnt);
		void unset_longpoll(const Client::ptr_t& clnt);
		const std::string& get_session(void) const { return m_session; }
		int get_xdisplay(void) const { return m_xdisplay; }
		unsigned int get_recvqueue_size(void) const { return m_recvqueue.size(); }
		const Glib::TimeVal& get_accesstime(void) const { return m_accesstime; }
		const Glib::TimeVal& get_starttime(void) const { return m_starttime; }
		pid_t get_pid(void) const { return m_pid; }

	protected:
		SocketServer *m_server;
		Glib::RefPtr<Gio::Socket> m_socket;
		sigc::connection m_connin;
		sigc::connection m_connout;
		sigc::connection m_connchildwatch;
		std::list<Json::Value> m_sendqueue;
		std::list<Json::Value> m_recvqueue;
		Glib::TimeVal m_accesstime;
		Glib::TimeVal m_starttime;
		Client::ptr_t m_longpoll;
		std::string m_session;
		mutable gint m_refcount;
		int m_xdisplay;
		pid_t m_pid;
		typedef enum {
			lifecycle_run,
			lifecycle_status,
			lifecycle_dead
		} lifecycle_t;
		lifecycle_t m_lifecycle;
		bool m_log;

		bool on_input(Glib::IOCondition iocond);
		bool on_output(Glib::IOCondition iocond);
		void child_watch(GPid pid, int child_status);
		static void child_reaper(GPid pid, int child_status);
		void notify_longpoll(void);
	};

	typedef std::map<std::string,Router::ptr_t> routers_t;
	routers_t m_routers;

	void count_routers(unsigned int& total, unsigned int& active);
	Router::ptr_t find_router(const std::string& session, bool create = true);
	bool stop_router(const std::string& session);
	void reap_died_routers(void);
	bool router_timeout_handler(void);

	void sockcli_close(void);
	void sockcli_send(const Json::Value& v);
	static void sockcli_timestamp(Json::Value& v);
	bool sockcli_input(Glib::IOCondition iocond);
	void sockcli_statuschange(CFMUAutoroute::statusmask_t status);
	void sockcli_autoroutelog(CFMUAutoroute::log_t item, const std::string& line);

	typedef void (SocketServer::*cmd_t)(const Json::Value& cmdin, Json::Value& cmdout);
	typedef std::map<std::string,cmd_t> cmdlist_t;
	cmdlist_t m_cmdlist;
	cmdlist_t m_servercmdlist;

	static Point get_point(const Json::Value& cmd, const std::string& name);
	static void set_point(Json::Value& cmd, const std::string& name, const Point& pt);
	static void set_double(Json::Value& cmd, const std::string& name, double x);

	void encode_wpt(Json::Value& fplwpt, const FPlanWaypoint& wpt);
	void encode_wpt(Json::Value& fplwpt, const ADR::FPLWaypoint& wpt);
	void decode_wpt(FPlanWaypoint& wpt, const Json::Value& json);
	void decode_wpt(ADR::FPLWaypoint& wpt, const Json::Value& json);
	void encode_fpl(Json::Value& reply, const FPlanRoute& route);
	void encode_fpl(Json::Value& reply, const ADR::FlightPlan& route);
	void decode_fpl(FPlanRoute& route, const Json::Value& json);
	void decode_fpl(ADR::FlightPlan& route, const Json::Value& json);
	void encode_rule(Json::Value& reply, const TrafficFlowRestrictions::RuleResult& rule);
	void encode_rule(Json::Value& rsequences, const ADR::RuleSequence& alt2, ADR::timetype_t tm);
	void encode_rule(Json::Value& reply, const ADR::RestrictionResult& rule);
	void check_fplan(Json::Value& localvalidate, const FPlanRoute& route);
	void check_fplan(Json::Value& localvalidate, const ADR::FlightPlan& route);
	void routeweather(Json::Value& wxval, const FPlanRoute& route);
	void weathercharts(Json::Value& wxval, const FPlanRoute& route, double width, double height, double maxprofilepagedist,
			   MeteoProfile::yaxis_t profileyaxis, double profilemaxalt,
			   bool svg, bool allowportrait, bool gramet, const std::vector<double>& hpa);
	void navlog(Json::Value& nlog, const FPlanRoute& route, const std::string& templ = PACKAGE_DATA_DIR "/navlog.xml",
		    const FPlanRoute::GFSResult& gfsr = FPlanRoute::GFSResult(), bool hidewbpage = false);
	void cmd_nop(const Json::Value& cmdin, Json::Value& cmdout);
	void cmd_quit(const Json::Value& cmdin, Json::Value& cmdout);
	void cmd_departure(const Json::Value& cmdin, Json::Value& cmdout);
	void cmd_destination(const Json::Value& cmdin, Json::Value& cmdout);
	void cmd_crossing(const Json::Value& cmdin, Json::Value& cmdout);
	void cmd_enroute(const Json::Value& cmdin, Json::Value& cmdout);
	void cmd_levels(const Json::Value& cmdin, Json::Value& cmdout);
	void cmd_exclude(const Json::Value& cmdin, Json::Value& cmdout);
	void cmd_tfr(const Json::Value& cmdin, Json::Value& cmdout);
	void cmd_atmosphere(const Json::Value& cmdin, Json::Value& cmdout);
	void cmd_cruise(const Json::Value& cmdin, Json::Value& cmdout);
	void cmd_optimization(const Json::Value& cmdin, Json::Value& cmdout);
	void cmd_preferred(const Json::Value& cmdin, Json::Value& cmdout);
	void cmd_aircraft(const Json::Value& cmdin, Json::Value& cmdout);
	void cmd_start(const Json::Value& cmdin, Json::Value& cmdout);
	void cmd_stop(const Json::Value& cmdin, Json::Value& cmdout);
	void cmd_continue(const Json::Value& cmdin, Json::Value& cmdout);
	void cmd_preload(const Json::Value& cmdin, Json::Value& cmdout);
	void cmd_clear(const Json::Value& cmdin, Json::Value& cmdout);
	void cmd_fplparse(const Json::Value& cmdin, Json::Value& cmdout);
	void cmd_fplparseadr(const Json::Value& cmdin, Json::Value& cmdout);

	Json::Value servercmd(const Json::Value& cmdin);
	void servercmd_scangrib2(const Json::Value& cmdin, Json::Value& cmdout);
	void servercmd_reloaddb(const Json::Value& cmdin, Json::Value& cmdout);
	void servercmd_processes(const Json::Value& cmdin, Json::Value& cmdout);
	void servercmd_ruleinfo(const Json::Value& cmdin, Json::Value& cmdout);
};

#endif /* ARSOCKSERVER_H */
