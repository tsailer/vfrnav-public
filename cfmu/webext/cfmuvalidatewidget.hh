#ifndef CFMUVALIDATEWIDGET_H
#define CFMUVALIDATEWIDGET_H

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "sysdepsgui.h"

#include <set>
#include <gtkmm.h>
#include <sigc++/sigc++.h>

#if defined(HAVE_WEBKITGTK4) && defined(HAVE_WEBKITGTKWEBEXT)
#include <webkit2/webkit2.h>
#include "cfmu-web-extension-proxy.h"
#elif defined(HAVE_WEBKITGTK)
#include <webkit/webkit.h>
#endif

class CFMUValidateWidget : public Gtk::Notebook {
public:
	explicit CFMUValidateWidget(BaseObjectType *castitem, const Glib::RefPtr<Builder>& refxml);
	explicit CFMUValidateWidget(void);
	virtual ~CFMUValidateWidget();

	void set_cookiefile(const std::string& cf) { m_cookiefile = cf; }
	const std::string& get_cookiefile(void) const { return m_cookiefile; }

	void start(void);

	typedef std::vector<std::string> validate_result_t;
	typedef sigc::slot<void,const validate_result_t&> validate_callback_t;
	void validate(const std::string& fpl, const validate_callback_t& cb);

protected:
	std::string m_cookiefile;
	sigc::connection m_timeout;
	sigc::connection m_vtimeout;
	typedef std::pair<std::string,validate_callback_t> valqueueentry_t;
	typedef std::list<valqueueentry_t> valqueue_t;
	valqueue_t m_valqueue;
	typedef enum {
		state_unloaded,
		state_loadmainpage,
		state_loadquerypage,
		state_ready,
		state_query,
		state_failed
	} state_t;
	state_t m_state;

	void set_state(state_t newstate);
	state_t get_state(void) const { return m_state; }

	bool timeout(void);
	bool vtimeout(void);
	void empty_queue_error(const std::string& msg);
	void initialize(void);
	void finalize(void);
	void cancelvalidate(void);

#if defined(HAVE_WEBKITGTK4) && defined(HAVE_WEBKITGTKWEBEXT)

	class WebExtProxy {
	public:
		WebExtProxy(GDBusConnection *connection, CFMUValidateWidget *w);
		WebExtProxy(CFMUWebExtensionProxy *ext = 0, CFMUValidateWidget *w = 0);
		WebExtProxy(const WebExtProxy& p);
		WebExtProxy& operator=(const WebExtProxy& p);
		~WebExtProxy();
		bool is_deleted(void) const { return m_deleted; }
		bool operator<(const WebExtProxy& p) const { return m_proxy < p.m_proxy; }
		bool operator>(const WebExtProxy& p) const { return m_proxy > p.m_proxy; }
		bool operator==(const WebExtProxy& p) const { return m_proxy == p.m_proxy; }
		bool operator!=(const WebExtProxy& p) const { return m_proxy != p.m_proxy; }
		bool is_portaldom(guint64& pageid) const;
		bool is_validatedom(guint64& pageid) const;
		bool is_portaldom(void) const { guint64 id; return is_portaldom(id); }
		bool is_validatedom(void) const { guint64 id; return is_validatedom(id); }
		guint64 get_portalpage(void) const { guint64 id; return is_portaldom(id) ? id : 0; }
		guint64 get_validatepage(void) const { guint64 id; return is_validatedom(id) ? id : 0; }
		void launch_validate(guint64 pageid);
		void validate(guint64 pageid, const std::string& fpl);
		void validate_cancel(void);

	protected:
		class Page {
		public:
			Page(guint64 id = 0, gboolean p = FALSE, gboolean v = FALSE) : m_id(id), m_portaldom(p), m_validatedom(v) {}
			bool operator<(const Page& p) const { return m_id < p.m_id; }
			bool operator>(const Page& p) const { return m_id > p.m_id; }
			bool operator==(const Page& p) const { return m_id == p.m_id; }
			bool operator!=(const Page& p) const { return m_id != p.m_id; }
			guint64 get_id(void) const { return m_id; }
			bool is_portaldom(void) const { return m_portaldom; }
			bool is_validatedom(void) const { return m_validatedom; }
			void set_portaldom(bool d) { m_portaldom = d; }
			void set_validatedom(bool d) { m_validatedom = d; }

		protected:
			guint64 m_id;
			bool m_portaldom;
			bool m_validatedom;
		};

		CFMUWebExtensionProxy *m_proxy;
		CFMUValidateWidget *m_widget;
		typedef std::set<Page> pages_t;
		pages_t m_pages;
		GCancellable *m_validatecancel;
		bool m_deleted;

		void init(void);
		void fini(void);
		static void web_extension_destroyed_1(WebExtProxy *self, GObject *web_extension);
		void web_extension_destroyed(GObject *web_extension);
		static void web_extension_page_created_1(CFMUWebExtensionProxy *extension, guint64 page_id, WebExtProxy *self);
		void web_extension_page_created(CFMUWebExtensionProxy *extension, guint64 page_id);
		static void web_extension_dom_changed_1(CFMUWebExtensionProxy *extension, guint64 page_id, gboolean portaldom, gboolean validatedom, WebExtProxy *self);
		void web_extension_dom_changed(CFMUWebExtensionProxy *extension, guint64 page_id, gboolean portaldom, gboolean validatedom);
		static void web_extension_validate_results_1(GObject *source, GAsyncResult *async_result, gpointer user_data);
		void web_extension_validate_results(GObject *source, GAsyncResult *async_result);
	};

	Gtk::Widget *m_portal_webview;
	Gtk::Widget *m_validate_webview;
	bool m_portal_loaded;
	bool m_validate_loaded;
	gulong m_webext_sighandler;
	GDBusServer *m_webext_dbus_server;
	typedef std::set<WebExtProxy> webext_proxy_t;
	webext_proxy_t m_webext_proxy;
	gulong m_portal_webview_close_sighandler;
	gulong m_portal_webview_create_sighandler;
	gulong m_portal_webview_ready_sighandler;
	gulong m_portal_webview_loadchanged_sighandler;
	gulong m_portal_webview_loadfailed_sighandler;
	gulong m_portal_webview_decidepolicy_sighandler;
	gulong m_validate_webview_close_sighandler;
	gulong m_validate_webview_create_sighandler;
	gulong m_validate_webview_ready_sighandler;
	gulong m_validate_webview_loadchanged_sighandler;
	gulong m_validate_webview_loadfailed_sighandler;
	gulong m_validate_webview_decidepolicy_sighandler;

	void add_portal_sighandlers(void);
	void remove_portal_sighandlers(void);
	void add_validate_sighandlers(void);
	void remove_validate_sighandlers(void);
	void prepare_webextension(void);
	void remove_webextension(void);
	void parse_results(void);
	void zap_errors(void);
	static gboolean close_webview_1(WebKitWebView* webview, gpointer userdata);
	gboolean close_webview(WebKitWebView* webview);
	static gboolean webview_ready_1(WebKitWebView* webview, gpointer userdata);
	gboolean webview_ready(WebKitWebView* webview);
	static gboolean authorize_authenticated_peer_1(GDBusAuthObserver *observer, GIOStream *stream, GCredentials *credentials, gpointer userdata);
	gboolean authorize_authenticated_peer(GDBusAuthObserver *observer, GIOStream *stream, GCredentials *credentials);
	static void initialize_webext_1(WebKitWebContext *web_context, gpointer userdata);
	void initialize_webext(WebKitWebContext *web_context);
	static gboolean new_connection_1(GDBusServer *server, GDBusConnection *connection, gpointer userdata);
	gboolean new_connection(GDBusServer *server, GDBusConnection *connection);
	void webext_cleanup(void);
	void webext_domchange(void);
	void webext_validateresult(const validate_result_t& result);
	static WebKitWebView *create_webview_1(WebKitWebView *webview, WebKitNavigationAction *navigation_action, gpointer userdata);
	WebKitWebView *create_webview(WebKitWebView *webview, WebKitNavigationAction *navigation_action);
	static void loadchanged_webview_1(WebKitWebView *webview, WebKitLoadEvent load_event, gpointer userdata);
	void loadchanged_webview(WebKitWebView *webview, WebKitLoadEvent load_event);
	static gboolean loadfailed_webview_1(WebKitWebView *webview, WebKitLoadEvent load_event, gchar *failing_uri, GError *error, gpointer user_data);
	gboolean loadfailed_webview(WebKitWebView *webview, WebKitLoadEvent load_event, gchar *failing_uri, GError *error);
	static gboolean decide_policy_1(WebKitWebView *webview, WebKitPolicyDecision *decision, WebKitPolicyDecisionType decision_type, gpointer user_data);
	gboolean decide_policy(WebKitWebView *webview, WebKitPolicyDecision *decision, WebKitPolicyDecisionType decision_type);

#elif defined(HAVE_WEBKITGTK)

	Gtk::Widget *m_portal_webview;
	Gtk::Widget *m_validate_webview;
	gulong m_portal_webview_close_sighandler;
	gulong m_portal_webview_create_sighandler;
	gulong m_portal_webview_ready_sighandler;
	gulong m_portal_webview_loadstatus_sighandler;
	gulong m_portal_webview_decidepolicy_sighandler;
	gulong m_portal_webview_newwndpolicy_sighandler;
	gulong m_portal_webview_rsrcload_sighandler;
	gulong m_portal_webview_consolemsg_sighandler;
	gulong m_validate_webview_close_sighandler;
	gulong m_validate_webview_create_sighandler;
	gulong m_validate_webview_ready_sighandler;
	gulong m_validate_webview_loadstatus_sighandler;
	gulong m_validate_webview_decidepolicy_sighandler;
	gulong m_validate_webview_newwndpolicy_sighandler;
	gulong m_validate_webview_rsrcload_sighandler;
	gulong m_validate_webview_consolemsg_sighandler;

	void add_portal_sighandlers(void);
	void remove_portal_sighandlers(void);
	void add_validate_sighandlers(void);
	void remove_validate_sighandlers(void);
	validate_result_t parse_results(void);
	void zap_errors(void);
	static gboolean close_webview_1(WebKitWebView* webview, gpointer userdata);
	gboolean close_webview(WebKitWebView* webview);
	static gboolean webview_ready_1(WebKitWebView* webview, gpointer userdata);
	gboolean webview_ready(WebKitWebView* webview);
	static WebKitWebView *create_webview_1(WebKitWebView *webview, WebKitWebFrame *frame, gpointer userdata);
	WebKitWebView *create_webview(WebKitWebView *webview, WebKitWebFrame *frame);
	static void loadstatus_webview_1(WebKitWebView *webview, GParamSpec *pspec, gpointer userdata);
	void loadstatus_webview(WebKitWebView *webview, GParamSpec *pspec);
	static gboolean policy_decision_required_1(WebKitWebView *webview, WebKitWebFrame *webframe, WebKitNetworkRequest *request,
						   WebKitWebNavigationAction *action, WebKitWebPolicyDecision *decision, gpointer userdata);
	gboolean policy_decision_required(WebKitWebView *webview, WebKitWebFrame *webframe, WebKitNetworkRequest *request,
					  WebKitWebNavigationAction *action, WebKitWebPolicyDecision *decision);
	static void resourceload_finished_1(WebKitWebView *webview, WebKitWebFrame *webframe, WebKitWebResource *webresource, gpointer userdata);
	void resourceload_finished(WebKitWebView *webview, WebKitWebFrame *webframe, WebKitWebResource *webresource);
	static gboolean console_message_1(WebKitWebView *webview, gchar *message, gint line, gchar *source_id, gpointer userdata);
	gboolean console_message(WebKitWebView *webview, gchar *message, gint line, gchar *source_id);
	static gboolean result_change_1(WebKitDOMElement *target, WebKitDOMEvent *e, gpointer userdata);
	gboolean result_change(WebKitDOMElement *target, WebKitDOMEvent *e);


#endif
};











#endif /* CFMUVALIDATEWIDGET_H */
