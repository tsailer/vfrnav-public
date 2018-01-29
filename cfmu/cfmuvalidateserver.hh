#ifndef CFMUVALIDATESERVER_H
#define CFMUVALIDATESERVER_H

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "sysdepsgui.h"

#include <gtkmm.h>
#include <giomm/socketservice.h>
#include <sigc++/sigc++.h>
#ifdef HAVE_CURL
#include <curl/curl.h>
#endif
#include <libxml/tree.h>
#include <set>

#include <json/json.h>

#include <adr.hh>
#include <adrrestriction.hh>
#include <cfmuvalidatewidget.hh>

// Xlib last because it defines Status to int...

#ifdef HAVE_X11_XLIB_H
#include <X11/Xlib.h>
#undef None
#endif

class CFMUValidateServer : public Gtk::Application {
public:
	CFMUValidateServer(int& argc, char**& argv);
	~CFMUValidateServer();
	void run(void);
        const Glib::RefPtr<Glib::MainLoop>& get_mainloop(void) const { return m_mainloop; }

protected:
	typedef std::set<Glib::ustring> rules_t;

	class Results : public std::vector<Glib::ustring> {
	public:
		void print(const ADR::Object::ptr_t& p, ADR::timetype_t tm);
		void print(const rules_t& r, const Glib::ustring& pfx);
		void print_tracerules(ADR::RestrictionEval& reval);
		void print_disabledrules(ADR::RestrictionEval& reval);
		void print(const ADR::Message& m);
		void print(const ADR::RestrictionResult& res, ADR::timetype_t tm);
		void annotate(ADR::RestrictionEval& reval);
	};

	class OptionalArgFilename : public sigc::trackable {
	public:
		OptionalArgFilename(void) : m_value(false), m_set(false) {}
		bool on_option_arg_filename(const Glib::ustring& option_name, const std::string& value, bool has_value);
		const std::string& get_arg(void) const { return m_arg; }
		bool is_value(void) const { return m_value; }
		bool is_set(void) const { return m_set; }

	protected:
		std::string m_arg;
		bool m_value;
		bool m_set;
	};

	class OptionalArgString : public sigc::trackable {
	public:
		OptionalArgString(void) : m_value(false), m_set(false) {}
		bool on_option_arg_string(const Glib::ustring& option_name, const Glib::ustring& value, bool has_value);
		const Glib::ustring& get_arg(void) const { return m_arg; }
		bool is_value(void) const { return m_value; }
		bool is_set(void) const { return m_set; }

	protected:
		Glib::ustring m_arg;
		bool m_value;
		bool m_set;
	};

	class Client : public sigc::trackable {
	public:
		typedef Glib::RefPtr<Client> ptr_t;
		typedef Glib::RefPtr<const Client> const_ptr_t;
		Client(CFMUValidateServer *server);
		virtual ~Client();
		void reference(void) const;
		void unreference(void) const;
		ptr_t get_ptr(void) { reference(); return ptr_t(this); }
		const_ptr_t get_ptr(void) const { reference(); return const_ptr_t(this); }

		typedef enum {
			validator_cfmu,
			validator_cfmub2bop,
			validator_cfmub2bpreop,
			validator_eurofpl,
			validator_local
		} validator_t;
		validator_t get_validator(void) const { return m_validator; }
		void set_validator(validator_t v) { m_validator = v; }
		static const std::string& to_str(validator_t v);

		const std::string& get_fplan(void) const { return m_fplan; }
		virtual void set_result(const Results& res) = 0;
#ifdef HAVE_CURL
		void curl_done(CURLcode result);
#endif

	protected:
		CFMUValidateServer *m_server;
		rules_t m_tracerules;
		rules_t m_disabledrules;
		mutable gint m_refcount;
		std::string m_fplan;
		Json::Value m_jsonfpl;
#ifdef HAVE_CURL
		CURL *m_curl;
		struct curl_slist *m_curl_headers;
		std::vector<char> m_curlrxbuffer;
		std::vector<char> m_curltxbuffer;
		std::string m_cfmub2bopurl;
		std::string m_cfmub2bopcert;
		std::string m_cfmub2bpreopurl;
		std::string m_cfmub2bpreopcert;
#endif
		validator_t m_validator;
		bool m_annotate;

		void eurocontrol_textual(xmlpp::Element *fplel);
		void eurocontrol_textual_json(xmlpp::Element *fplel);
		static void eurocontrol_structural_aerodrome(xmlpp::Element *el, const Json::Value& wpt);
		void eurocontrol_structural(xmlpp::Element *fplel);
		void handle_input(void);
#ifdef HAVE_CURL
		void close_curl(void);
		void find_eurofpl_results(std::string& result, xmlNode *a_node, unsigned int level = 0);
		void parse_eurofpl(void);
 		void parse_cfmub2b(void);
		static std::string get_text(const xmlpp::Node *n);
		static size_t my_write_func_1(const void *ptr, size_t size, size_t nmemb, Client *clnt);
		static size_t my_read_func_1(void *ptr, size_t size, size_t nmemb, Client *clnt);
		static int my_progress_func_1(Client *clnt, double t, double d, double ultotal, double ulnow);
		size_t my_write_func(const void *ptr, size_t size, size_t nmemb);
		size_t my_read_func(void *ptr, size_t size, size_t nmemb);
		int my_progress_func(double t, double d, double ultotal, double ulnow);
#endif
	};

	class StdioClient : public Client {
	public:
		typedef Glib::RefPtr<StdioClient> ptr_t;
		typedef Glib::RefPtr<const StdioClient> const_ptr_t;
		StdioClient(CFMUValidateServer *server);
		virtual ~StdioClient();

		virtual void set_result(const Results& res);

	protected:
		char m_inbuffer[16384];
		Glib::RefPtr<Glib::IOChannel> m_inputchan;
		Glib::RefPtr<Glib::IOChannel> m_outputchan;
		sigc::connection m_inputreaderconn;
		unsigned int m_inbufptr;

		bool input_line_handler(Glib::IOCondition iocond);
		void handle_input_loop(void);
	};

	class SocketClient : public Client {
	public:
		typedef Glib::RefPtr<SocketClient> ptr_t;
		typedef Glib::RefPtr<const SocketClient> const_ptr_t;
		SocketClient(CFMUValidateServer *server, const Glib::RefPtr<Gio::SocketConnection>& connection);
		virtual ~SocketClient();

		virtual void set_result(const Results& res);

	protected:
		char m_inbuffer[16384];
		Glib::RefPtr<Gio::SocketConnection> m_connection;
		sigc::connection m_inputreaderconn;
		unsigned int m_inbufptr;

		bool input_line_handler(Glib::IOCondition iocond);
		void handle_input_loop(void);
	};

	class SingleFplClient : public Client {
	public:
		typedef Glib::RefPtr<SingleFplClient> ptr_t;
		typedef Glib::RefPtr<const SingleFplClient> const_ptr_t;
		SingleFplClient(CFMUValidateServer *server);
		virtual ~SingleFplClient();

		virtual void set_result(const Results& res);

		void set_fplan(const std::string& fpl);

		bool is_closeatend(void) const { return m_closeatend; }
		void set_closeatend(bool c = true) { m_closeatend = c; }

	protected:
		Glib::RefPtr<Glib::IOChannel> m_outputchan;
		typedef std::list<std::string> fplans_t;
		fplans_t m_fplans;
		bool m_closeatend;

		void handle_input_loop(void);
	};

	std::unique_ptr<AirportsDbQueryInterface> m_airportdb;
	std::unique_ptr<NavaidsDbQueryInterface> m_navaiddb;
	std::unique_ptr<WaypointsDbQueryInterface> m_waypointdb;
	std::unique_ptr<AirwaysDbQueryInterface> m_airwaydb;
	std::unique_ptr<AirspacesDbQueryInterface> m_airspacedb;
        TopoDb30 m_topodb;
	Engine *m_engine;
#ifdef HAVE_PQXX
	std::unique_ptr<pqxx::lazyconnection> m_pgconn;
#endif
	ADR::Database m_adrdb;
	ADR::RestrictionEval m_reval;
	std::string m_dir_main;
	std::string m_dir_aux;
        Glib::RefPtr<Glib::MainLoop> m_mainloop;
	typedef std::set<Client::ptr_t> clients_t;
	clients_t m_clients;
	typedef std::list<Client::ptr_t> cfmuclients_t;
	cfmuclients_t m_cfmuclients;
        sigc::connection m_stateconn;
	unsigned int m_statecnt;
	CFMUValidateWidget *m_valwidget;
	guint m_verbose;
#ifdef HAVE_X11_XLIB_H
	sigc::connection m_x11srvconnwatch;
	int m_xdisplay;
        Glib::Pid m_x11srvpid;
	bool m_x11srvrun;
#endif
#ifdef HAVE_CURL
	typedef std::map<CURL *,Client::ptr_t> curlhandlemap_t;
	curlhandlemap_t m_curlhandlemap;
	typedef std::list<sigc::connection> curlio_t;
	curlio_t m_curlio;
	CURLM *m_curl;
#endif
	Glib::RefPtr<Gio::Socket> m_listensock;
#ifdef G_OS_UNIX
	Glib::RefPtr<Gio::UnixSocketAddress> m_listensockaddr;
#endif
	Glib::RefPtr<Gio::SocketService> m_listenservice;
	Engine::auxdb_mode_t m_auxdbmode;
	bool m_adrloaded;
	bool m_quit;

	void start_cfmu(void);
	void prepare_webview(void);
	void destroy_webview(void);
	void submit_webview(void);
	void results_webview(const CFMUValidateWidget::validate_result_t& results);

#ifdef HAVE_X11_XLIB_H
	void start_x11(void);
	void stop_x11(void);
	void x11srv_child_watch(GPid pid, int child_status);
	bool poll_xserver(void);
	static int x11_error(Display *dpy, XErrorEvent *err);
	static int x11_ioerror(Display *dpy);
	static bool x11_fail;
#endif
#ifdef HAVE_CURL
	void curl_add(CURL *handle, const Client::ptr_t& p);
	void curl_remove(CURL *handle);
	void curl_removeio(void);
	void curl_io(void);
#endif

	void annotate(Results& res);
	void validate_locally(const Client::ptr_t& p);
	void validate_cfmu(const Client::ptr_t& p);
	void close(const Client::ptr_t& p);
	void logmessage(unsigned int loglevel, const Glib::ustring& msg);

	void remove_listensockaddr(void);
#ifdef G_OS_UNIX
	void listen(const std::string& path, uid_t socketuid, gid_t socketgid, mode_t socketmode);
#else
	void listen(const std::string& path);
#endif
	bool on_connect(const Glib::RefPtr<Gio::SocketConnection>& connection, const Glib::RefPtr<Glib::Object>& source_object);
};

#endif /* CFMUVALIDATESERVER_H */
