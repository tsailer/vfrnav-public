#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <iostream>
#include <iomanip>
#include <gdkmm.h>
#include <gtkmm.h>

#if defined(HAVE_WEBKITGTK4) && defined(HAVE_WEBKITGTKWEBEXT)

#include <webkit2/webkit2.h>
#include "cfmu-web-extension-proxy.h"

#elif defined(HAVE_WEBKITGTK)

#include <webkit/webkit.h>

#endif

#include "cfmuvalidatewidget.hh"

CFMUValidateWidget::CFMUValidateWidget(BaseObjectType *castitem, const Glib::RefPtr<Builder>& refxml)
        : Gtk::Notebook(castitem), m_state(state_unloaded),
#if defined(HAVE_WEBKITGTK4) && defined(HAVE_WEBKITGTKWEBEXT)
	m_portal_webview(0), m_validate_webview(0),
	m_portal_loaded(false), m_validate_loaded(false),
	m_webext_sighandler(0), m_webext_dbus_server(0),
	m_portal_webview_close_sighandler(0), m_portal_webview_create_sighandler(0),
	m_portal_webview_ready_sighandler(0), m_portal_webview_loadchanged_sighandler(0),
	m_portal_webview_loadfailed_sighandler(0), m_portal_webview_decidepolicy_sighandler(0),
	m_validate_webview_close_sighandler(0), m_validate_webview_create_sighandler(0),
	m_validate_webview_ready_sighandler(0), m_validate_webview_loadchanged_sighandler(0),
	m_validate_webview_loadfailed_sighandler(0), m_validate_webview_decidepolicy_sighandler(0)
#elif defined(HAVE_WEBKITGTK)
	m_portal_webview(0), m_validate_webview(0),
	m_portal_webview_close_sighandler(0), m_portal_webview_create_sighandler(0),
	m_portal_webview_ready_sighandler(0), m_portal_webview_loadstatus_sighandler(0),
	m_portal_webview_decidepolicy_sighandler(0), m_portal_webview_newwndpolicy_sighandler(0),
	m_portal_webview_rsrcload_sighandler(0), m_portal_webview_consolemsg_sighandler(0),
	m_validate_webview_close_sighandler(0), m_validate_webview_create_sighandler(0),
	m_validate_webview_ready_sighandler(0), m_validate_webview_loadstatus_sighandler(0),
	m_validate_webview_decidepolicy_sighandler(0), m_validate_webview_newwndpolicy_sighandler(0),
	m_validate_webview_rsrcload_sighandler(0), m_validate_webview_consolemsg_sighandler(0)
#endif
{
	initialize();
}

CFMUValidateWidget::CFMUValidateWidget(void)
        : Gtk::Notebook(), m_state(state_unloaded),
#if defined(HAVE_WEBKITGTK4) && defined(HAVE_WEBKITGTKWEBEXT)
	m_portal_webview(0), m_validate_webview(0),
	m_webext_sighandler(0), m_webext_dbus_server(0),
	m_portal_webview_close_sighandler(0), m_portal_webview_create_sighandler(0),
	m_portal_webview_ready_sighandler(0), m_portal_webview_loadchanged_sighandler(0),
	m_portal_webview_loadfailed_sighandler(0), m_portal_webview_decidepolicy_sighandler(0),
	m_validate_webview_close_sighandler(0), m_validate_webview_create_sighandler(0),
	m_validate_webview_ready_sighandler(0), m_validate_webview_loadchanged_sighandler(0),
	m_validate_webview_loadfailed_sighandler(0), m_validate_webview_decidepolicy_sighandler(0)
#elif defined(HAVE_WEBKITGTK)
	m_portal_webview(0), m_validate_webview(0),
	m_portal_webview_close_sighandler(0), m_portal_webview_create_sighandler(0),
	m_portal_webview_ready_sighandler(0), m_portal_webview_loadstatus_sighandler(0),
	m_portal_webview_decidepolicy_sighandler(0), m_portal_webview_newwndpolicy_sighandler(0),
	m_portal_webview_rsrcload_sighandler(0), m_portal_webview_consolemsg_sighandler(0),
	m_validate_webview_close_sighandler(0), m_validate_webview_create_sighandler(0),
	m_validate_webview_ready_sighandler(0), m_validate_webview_loadstatus_sighandler(0),
	m_validate_webview_decidepolicy_sighandler(0), m_validate_webview_newwndpolicy_sighandler(0),
	m_validate_webview_rsrcload_sighandler(0), m_validate_webview_consolemsg_sighandler(0)
#endif
{
	initialize();
}

CFMUValidateWidget::~CFMUValidateWidget()
{
	m_timeout.disconnect();
	m_vtimeout.disconnect();
	empty_queue_error("WEBPAGE_DESTRUCTOR");
	while (get_n_pages() > 0)
		remove_page(0);
	finalize();
}

bool CFMUValidateWidget::timeout(void)
{
	m_timeout.disconnect();
	if (get_state() == state_failed || get_state() == state_ready) {
		set_state(state_unloaded);
		return false;
	}
	set_state(state_failed);
	m_timeout = Glib::signal_timeout().connect_seconds(sigc::mem_fun(*this, &CFMUValidateWidget::timeout), 15);
	return false;
}

bool CFMUValidateWidget::vtimeout(void)
{
	m_vtimeout.disconnect();
	empty_queue_error("WEBPAGE_TIMEOUT");
	return false;
}

void CFMUValidateWidget::empty_queue_error(const std::string& msg)
{
	cancelvalidate();
	validate_result_t r;
	r.push_back(msg);
	for (valqueue_t::const_iterator i(m_valqueue.begin()), e(m_valqueue.end()); i != e; ++i)
		i->second(r);
	m_valqueue.clear();
}

void CFMUValidateWidget::validate(const std::string& fpl, const validate_callback_t& cb)
{
	if (m_valqueue.empty()) {
		m_vtimeout.disconnect();
		m_vtimeout = Glib::signal_timeout().connect_seconds(sigc::mem_fun(*this, &CFMUValidateWidget::vtimeout), 60);
	}
	m_valqueue.push_back(valqueueentry_t(fpl, cb));
	if (get_state() == state_ready)
		set_state(state_query);
}

void CFMUValidateWidget::start(void)
{
	if (get_state() == state_unloaded)
		set_state(state_loadmainpage);
}

#if defined(HAVE_WEBKITGTK4) && defined(HAVE_WEBKITGTKWEBEXT)

CFMUValidateWidget::WebExtProxy::WebExtProxy(GDBusConnection *connection, CFMUValidateWidget *w)
	: m_proxy(0), m_widget(w), m_validatecancel(0), m_deleted(false)
{
	if (!w || !connection)
		return;
	m_proxy = cfmu_web_extension_proxy_new(connection);
	init();
}

CFMUValidateWidget::WebExtProxy::WebExtProxy(CFMUWebExtensionProxy *ext, CFMUValidateWidget *w)
	: m_proxy(ext), m_widget(w), m_validatecancel(0), m_deleted(false)
{	
	init();
}

CFMUValidateWidget::WebExtProxy::WebExtProxy(const WebExtProxy& p)
	: m_proxy(p.m_proxy), m_widget(p.m_widget), m_pages(p.m_pages), m_validatecancel(0), m_deleted(p.m_deleted)
{
	init();
}

CFMUValidateWidget::WebExtProxy& CFMUValidateWidget::WebExtProxy::operator=(const WebExtProxy& p)
{
	fini();
	m_proxy = p.m_proxy;
	m_widget = p.m_widget;
	m_pages = p.m_pages;
	m_validatecancel = 0;
	m_deleted = p.m_deleted;
	init();
	return *this;
}

CFMUValidateWidget::WebExtProxy::~WebExtProxy()
{
	fini();
}

bool CFMUValidateWidget::WebExtProxy::is_portaldom(guint64& pageid) const
{
	for (pages_t::const_iterator i(m_pages.begin()), e(m_pages.end()); i != e; ++i) {
		if (!i->is_portaldom())
			continue;
		pageid = i->get_id();
		return true;
	}
	pageid = 0;
	return false;
}

bool CFMUValidateWidget::WebExtProxy::is_validatedom(guint64& pageid) const
{
	for (pages_t::const_iterator i(m_pages.begin()), e(m_pages.end()); i != e; ++i) {
		if (!i->is_validatedom())
			continue;
		pageid = i->get_id();
		return true;
	}
	pageid = 0;
	return false;
}

void CFMUValidateWidget::WebExtProxy::launch_validate(guint64 pageid)
{
	if (!m_proxy)
		return;
	if (false)
		std::cerr << "launch_validate: " << pageid << std::endl;
	cfmu_web_extension_proxy_launch_validate(m_proxy, pageid);
}

void CFMUValidateWidget::WebExtProxy::validate(guint64 pageid, const std::string& fpl)
{
	if (!m_proxy)
		return;
	if (m_validatecancel)
		return;
	m_validatecancel = g_cancellable_new();
	cfmu_web_extension_proxy_validate(m_proxy, pageid, fpl.c_str(), m_validatecancel,
					  &WebExtProxy::web_extension_validate_results_1, this);
}

void CFMUValidateWidget::WebExtProxy::validate_cancel(void)
{
	if (!m_validatecancel)
		return;
	g_cancellable_cancel(m_validatecancel);
	g_object_unref(m_validatecancel);
	m_validatecancel = 0;
}

void CFMUValidateWidget::WebExtProxy::init(void)
{
	if (!m_proxy || is_deleted())
		return;
	g_object_weak_ref(G_OBJECT(m_proxy), (GWeakNotify)web_extension_destroyed_1, this);
	g_signal_connect(m_proxy, "page-created", G_CALLBACK(web_extension_page_created_1), this);
	g_signal_connect(m_proxy, "dom-changed", G_CALLBACK(web_extension_dom_changed_1), this);	
}

void CFMUValidateWidget::WebExtProxy::fini(void)
{
	validate_cancel();
	if (!m_proxy || is_deleted())
		return;
	g_signal_handlers_disconnect_by_data(m_proxy, this);
	g_object_weak_unref(G_OBJECT(m_proxy), (GWeakNotify)web_extension_destroyed_1, this);
}

void CFMUValidateWidget::WebExtProxy::web_extension_destroyed_1(WebExtProxy *self, GObject *web_extension)
{
	self->web_extension_destroyed(web_extension);
}

void CFMUValidateWidget::WebExtProxy::web_extension_destroyed(GObject *web_extension)
{
	g_assert(G_OBJECT(m_proxy) == web_extension);
	m_deleted = true;
	validate_cancel();
	if (m_proxy)
		g_signal_handlers_disconnect_by_data(m_proxy, this);
	if (m_widget)
		m_widget->webext_cleanup();
}

void CFMUValidateWidget::WebExtProxy::web_extension_page_created_1(CFMUWebExtensionProxy *extension, guint64 page_id, WebExtProxy *self)
{
	self->web_extension_page_created(extension, page_id);
}

void CFMUValidateWidget::WebExtProxy::web_extension_page_created(CFMUWebExtensionProxy *extension, guint64 page_id)
{
	m_pages.insert(Page(page_id));
	if (false)
		std::cerr << "web extension " << (guint64)extension << " page created " << page_id << std::endl;
}

void CFMUValidateWidget::WebExtProxy::web_extension_dom_changed_1(CFMUWebExtensionProxy *extension, guint64 page_id, gboolean portaldom, gboolean validatedom, WebExtProxy *self)
{
	self->web_extension_dom_changed(extension, page_id, portaldom, validatedom);
}

void CFMUValidateWidget::WebExtProxy::web_extension_dom_changed(CFMUWebExtensionProxy *extension, guint64 page_id, gboolean portaldom, gboolean validatedom)
{
	pages_t::iterator it(m_pages.insert(Page(page_id, portaldom, validatedom)).first);
	Page& pg(const_cast<Page&>(*it));
	pg.set_portaldom(portaldom);
	pg.set_validatedom(validatedom);
	if (m_widget)
		m_widget->webext_domchange();
	if (false)
		std::cerr << "web extension " << (guint64)extension << " page " << page_id << " portal " << (int)portaldom << " validate " << (int)validatedom << std::endl;
}

void CFMUValidateWidget::WebExtProxy::web_extension_validate_results_1(GObject *source, GAsyncResult *async_result, gpointer user_data)
{
	static_cast<WebExtProxy *>(user_data)->web_extension_validate_results(source, async_result);
}

void CFMUValidateWidget::WebExtProxy::web_extension_validate_results(GObject *source, GAsyncResult *async_result)
{
	if (!m_proxy || is_deleted())
		return;
	GError *err(0);
	gchar **r(cfmu_web_extension_proxy_validate_finish(m_proxy, async_result, &err));
	validate_result_t result;
	if (err) {
		result.push_back(err->message);
		g_error_free(err);
	} else {
		for (gchar **r1(r); *r1; ++r1)
			result.push_back(*r1);
	}
	g_strfreev(r);
	if (m_validatecancel) {
		g_object_unref(m_validatecancel);
		m_validatecancel = 0;
		if (m_widget)
			m_widget->webext_validateresult(result);
	}
}

void CFMUValidateWidget::add_portal_sighandlers(void)
{
	remove_portal_sighandlers();
	m_portal_webview_close_sighandler = g_signal_connect(m_portal_webview->gobj(), "close", G_CALLBACK(&CFMUValidateWidget::close_webview_1), this);
	m_portal_webview_ready_sighandler = g_signal_connect(m_portal_webview->gobj(), "ready-to-show", G_CALLBACK(&CFMUValidateWidget::webview_ready_1), this);
	m_portal_webview_loadchanged_sighandler = g_signal_connect(m_portal_webview->gobj(), "load-changed", G_CALLBACK(&CFMUValidateWidget::loadchanged_webview_1), this);
	m_portal_webview_loadfailed_sighandler = g_signal_connect(m_portal_webview->gobj(), "load-failed", G_CALLBACK(&CFMUValidateWidget::loadfailed_webview_1), this);
	m_portal_webview_decidepolicy_sighandler = g_signal_connect(m_portal_webview->gobj(), "decide-policy", G_CALLBACK(&CFMUValidateWidget::decide_policy_1), this);
	m_portal_webview_create_sighandler = g_signal_connect(m_portal_webview->gobj(), "create", G_CALLBACK(&CFMUValidateWidget::create_webview_1), this);
}

void CFMUValidateWidget::remove_portal_sighandlers(void)
{
	if (m_portal_webview_close_sighandler) {
		g_signal_handler_disconnect(m_portal_webview->gobj(), m_portal_webview_close_sighandler);
		m_portal_webview_close_sighandler = 0;
	}
	if (m_portal_webview_create_sighandler) {
		g_signal_handler_disconnect(m_portal_webview->gobj(), m_portal_webview_create_sighandler);
		m_portal_webview_create_sighandler = 0;
	}
	if (m_portal_webview_ready_sighandler) {
		g_signal_handler_disconnect(m_portal_webview->gobj(), m_portal_webview_ready_sighandler);
		m_portal_webview_ready_sighandler = 0;
	}
	if (m_portal_webview_loadchanged_sighandler) {
		g_signal_handler_disconnect(m_portal_webview->gobj(), m_portal_webview_loadchanged_sighandler);
		m_portal_webview_loadchanged_sighandler = 0;
	}
	if (m_portal_webview_loadfailed_sighandler) {
		g_signal_handler_disconnect(m_portal_webview->gobj(), m_portal_webview_loadfailed_sighandler);
		m_portal_webview_loadfailed_sighandler = 0;
	}
	if (m_portal_webview_decidepolicy_sighandler) {
		g_signal_handler_disconnect(m_portal_webview->gobj(), m_portal_webview_decidepolicy_sighandler);
		m_portal_webview_decidepolicy_sighandler = 0;
	}
}

void CFMUValidateWidget::add_validate_sighandlers(void)
{
	remove_validate_sighandlers();
	m_validate_webview_close_sighandler = g_signal_connect(m_validate_webview->gobj(), "close", G_CALLBACK(&CFMUValidateWidget::close_webview_1), this);
	m_validate_webview_ready_sighandler = g_signal_connect(m_validate_webview->gobj(), "ready-to-show", G_CALLBACK(&CFMUValidateWidget::webview_ready_1), this);
	m_validate_webview_loadchanged_sighandler = g_signal_connect(m_validate_webview->gobj(), "load-changed", G_CALLBACK(&CFMUValidateWidget::loadchanged_webview_1), this);
	m_validate_webview_loadfailed_sighandler = g_signal_connect(m_validate_webview->gobj(), "load-failed", G_CALLBACK(&CFMUValidateWidget::loadfailed_webview_1), this);
	m_validate_webview_decidepolicy_sighandler = g_signal_connect(m_validate_webview->gobj(), "decide-policy", G_CALLBACK(&CFMUValidateWidget::decide_policy_1), this);
	m_validate_webview_create_sighandler = g_signal_connect(m_validate_webview->gobj(), "create", G_CALLBACK(&CFMUValidateWidget::create_webview_1), this);
}

void CFMUValidateWidget::remove_validate_sighandlers(void)
{
	if (m_validate_webview_close_sighandler) {
		g_signal_handler_disconnect(m_validate_webview->gobj(), m_validate_webview_close_sighandler);
		m_validate_webview_close_sighandler = 0;
	}
	if (m_validate_webview_create_sighandler) {
		g_signal_handler_disconnect(m_validate_webview->gobj(), m_validate_webview_create_sighandler);
		m_validate_webview_create_sighandler = 0;
	}
	if (m_validate_webview_ready_sighandler) {
		g_signal_handler_disconnect(m_validate_webview->gobj(), m_validate_webview_ready_sighandler);
		m_validate_webview_ready_sighandler = 0;
	}
	if (m_validate_webview_loadchanged_sighandler) {
		g_signal_handler_disconnect(m_validate_webview->gobj(), m_validate_webview_loadchanged_sighandler);
		m_validate_webview_loadchanged_sighandler = 0;
	}
	if (m_validate_webview_loadfailed_sighandler) {
		g_signal_handler_disconnect(m_validate_webview->gobj(), m_validate_webview_loadfailed_sighandler);
		m_validate_webview_loadfailed_sighandler = 0;
	}
	if (m_validate_webview_decidepolicy_sighandler) {
		g_signal_handler_disconnect(m_validate_webview->gobj(), m_validate_webview_decidepolicy_sighandler);
		m_validate_webview_decidepolicy_sighandler = 0;
	}
}

void CFMUValidateWidget::prepare_webextension(void)
{
	remove_webextension();
	{
		char *address(g_strdup_printf("unix:tmpdir=%s", g_get_tmp_dir()));
		char *guid(g_dbus_generate_guid());
		GDBusAuthObserver *observer(g_dbus_auth_observer_new());
		g_signal_connect(observer, "authorize-authenticated-peer", G_CALLBACK(authorize_authenticated_peer_1), this);
		GError *error(0);
		m_webext_dbus_server = g_dbus_server_new_sync(address, G_DBUS_SERVER_FLAGS_NONE, guid, observer, 0, &error);
		if (error) {
			g_warning("Failed to start web extension server on %s: %s", address, error->message);
			g_error_free(error);
		} else {
			g_signal_connect(m_webext_dbus_server, "new-connection", G_CALLBACK(new_connection_1), this);
			g_dbus_server_start(m_webext_dbus_server);
		}
		g_free(address);
		g_free(guid);
		g_object_unref(observer);
	}
	{
		WebKitWebContext *ctx(webkit_web_context_get_default());
		if (!ctx)
			return;
		m_webext_sighandler = g_signal_connect(ctx, "initialize-web-extensions", G_CALLBACK(&CFMUValidateWidget::initialize_webext_1), this);
	}
}

void CFMUValidateWidget::remove_webextension(void)
{
	if (m_webext_sighandler) {
		WebKitWebContext *ctx(webkit_web_context_get_default());
		g_signal_handler_disconnect(ctx, m_webext_sighandler);
		m_webext_sighandler = 0;
	}
	g_clear_object(&m_webext_dbus_server);
}

gboolean CFMUValidateWidget::authorize_authenticated_peer_1(GDBusAuthObserver *observer, GIOStream *stream, GCredentials *credentials, gpointer userdata)
{
	return static_cast<CFMUValidateWidget *>(userdata)->authorize_authenticated_peer(observer, stream, credentials);
}
	
gboolean CFMUValidateWidget::authorize_authenticated_peer(GDBusAuthObserver *observer, GIOStream *stream, GCredentials *credentials)
{
	static GCredentials *own_credentials(0);
	if (!own_credentials)
		own_credentials = g_credentials_new();
	GError *error(0);
	if (credentials && g_credentials_is_same_user(credentials, own_credentials, &error))
		return TRUE;
	if (error) {
		g_warning ("Failed to authorize web extension connection: %s", error->message);
		g_error_free (error);
	}
	return FALSE;
}

void CFMUValidateWidget::initialize_webext_1(WebKitWebContext *web_context, gpointer userdata)
{
	static_cast<CFMUValidateWidget *>(userdata)->initialize_webext(web_context);
}

void CFMUValidateWidget::initialize_webext(WebKitWebContext *web_context)
{
	webkit_web_context_set_web_extensions_directory(web_context, PACKAGE_LIBEXEC_DIR "/web-extensions");
	const char *address(m_webext_dbus_server ? g_dbus_server_get_client_address(m_webext_dbus_server) : 0);
	GVariant *user_data(g_variant_new("(ms)", address));
	webkit_web_context_set_web_extensions_initialization_user_data(web_context, user_data);
}

gboolean CFMUValidateWidget::new_connection_1(GDBusServer *server, GDBusConnection *connection, gpointer userdata)
{
	return static_cast<CFMUValidateWidget *>(userdata)->new_connection(server, connection);
}

gboolean CFMUValidateWidget::new_connection(GDBusServer *server, GDBusConnection *connection)
{
	if (false)
		std::cerr << "new dbus connection" << std::endl;
	m_webext_proxy.insert(WebExtProxy(connection, this));
	return TRUE;
}

void CFMUValidateWidget::webext_cleanup(void)
{
	for (webext_proxy_t::iterator i(m_webext_proxy.begin()), e(m_webext_proxy.end()); i != e; ) {
		webext_proxy_t::iterator i0(i);
		++i;
		if (!i0->is_deleted())
			continue;
		m_webext_proxy.erase(i0);
	}
}

void CFMUValidateWidget::webext_domchange(void)
{
	switch (get_state()) {
	case state_loadmainpage:
		set_state(state_loadquerypage);
		break;

	case state_loadquerypage:
		set_state(state_ready);
		break;

	case state_ready:
	case state_query:
	{
		bool portal(false), validate(false);
		for (webext_proxy_t::const_iterator i(m_webext_proxy.begin()), e(m_webext_proxy.end()); i != e; ++i) {
			portal = portal || i->is_portaldom();
			validate = validate || i->is_validatedom();
		}
		if (!portal || !validate)
			set_state(state_failed);
		break;
	}

	default:
		break;
	}	
}

void CFMUValidateWidget::webext_validateresult(const validate_result_t& result)
{
	if (get_state() != state_query)
		return;
	if (result.empty())
		return;
	if (!m_valqueue.empty()) {
		m_valqueue.begin()->second(result);
		m_valqueue.erase(m_valqueue.begin());
	}
	set_state(state_ready);
}

gboolean CFMUValidateWidget::close_webview_1(WebKitWebView* webview, gpointer userdata)
{
	return static_cast<CFMUValidateWidget *>(userdata)->close_webview(webview);
}

gboolean CFMUValidateWidget::close_webview(WebKitWebView* webview)
{
	if ((m_portal_webview && webview == WEBKIT_WEB_VIEW(m_portal_webview->gobj())) ||
	    (m_validate_webview && webview == WEBKIT_WEB_VIEW(m_validate_webview->gobj()))) {
		set_state(state_failed);
		return true;
	}
	{
		int i(0), n(get_n_pages());
		for (; i < n; ++i) {
			const Gtk::Widget *pg(get_nth_page(i));
			const Gtk::ScrolledWindow *pgs(dynamic_cast<const Gtk::ScrolledWindow *>(pg));
			if (!pgs)
				continue;
			const Gtk::Widget *chld(pgs->get_child());
			if (!chld)
				continue;
			if (WEBKIT_WEB_VIEW(chld->gobj()) != webview)
				continue;
			remove_page(i);
			break;
		}
		if (i >= n)
			std::cerr << "close_webview: page not found" << std::endl;
	}	
	return true;
}

gboolean CFMUValidateWidget::webview_ready_1(WebKitWebView* webview, gpointer userdata)
{
	return static_cast<CFMUValidateWidget *>(userdata)->webview_ready(webview);
}

gboolean CFMUValidateWidget::webview_ready(WebKitWebView* webview)
{
	if (false)
		std::cerr << "WebView: ready" << std::endl;
	gtk_widget_show_all(GTK_WIDGET(webview));
	return false;
}

WebKitWebView *CFMUValidateWidget::create_webview_1(WebKitWebView *webview, WebKitNavigationAction *navigation_action, gpointer userdata)
{
	return static_cast<CFMUValidateWidget *>(userdata)->create_webview(webview, navigation_action);
}

WebKitWebView *CFMUValidateWidget::create_webview(WebKitWebView *webview, WebKitNavigationAction *navigation_action)
{
	if (false)
		std::cerr << "new webview! state " << (int)get_state() << std::endl;
	switch (get_state()) {
	case state_loadmainpage:
	case state_loadquerypage:
	case state_ready:
	case state_query:
		break;

	default:
		return 0;
	}
	Gtk::Widget *wvw(Gtk::manage(Glib::wrap(webkit_web_view_new())));
	WebKitWebView *wv(WEBKIT_WEB_VIEW(wvw->gobj()));
	Gtk::ScrolledWindow *scr(Gtk::manage(new Gtk::ScrolledWindow()));
	scr->show();
	scr->add(*wvw);
	if (get_state() == state_loadquerypage &&
	    m_portal_webview && webview == WEBKIT_WEB_VIEW(m_portal_webview->gobj())) {
		remove_validate_sighandlers();
		m_validate_loaded = false;
		m_validate_webview = wvw;
		m_validate_webview->show();
		webkit_web_view_set_settings(WEBKIT_WEB_VIEW(m_validate_webview->gobj()), webkit_web_view_get_settings(WEBKIT_WEB_VIEW(m_portal_webview->gobj())));
		add_validate_sighandlers();
		set_current_page(append_page(*scr, "Validate", false));
		return wv;
	}
	wvw->show();
	append_page(*scr, "Unknown", false);
	return wv;
}

void CFMUValidateWidget::loadchanged_webview_1(WebKitWebView *webview, WebKitLoadEvent load_event, gpointer userdata)
{
	static_cast<CFMUValidateWidget *>(userdata)->loadchanged_webview(webview, load_event);
}

void CFMUValidateWidget::loadchanged_webview(WebKitWebView *webview, WebKitLoadEvent load_event)
{
	GObject *object(G_OBJECT(webview));
	bool is_portal(m_portal_webview && webview == WEBKIT_WEB_VIEW(m_portal_webview->gobj()));
	bool is_validate(m_validate_webview && webview == WEBKIT_WEB_VIEW(m_validate_webview->gobj()));
	g_assert(is_portal || is_validate);
	g_object_freeze_notify(object);
	switch (load_event) {
	case WEBKIT_LOAD_FINISHED:
		if (is_portal) {
			m_portal_loaded = true;
			if (get_state() == state_loadmainpage)
				set_state(state_loadquerypage);
		}
		if (is_validate) {
			m_validate_loaded = true;
			if (get_state() == state_loadquerypage)
				set_state(state_ready);
		}
		break;

	default:
		break;
       	}
	g_object_thaw_notify(object);
}

gboolean CFMUValidateWidget::loadfailed_webview_1(WebKitWebView *webview, WebKitLoadEvent load_event, gchar *failing_uri, GError *error, gpointer userdata)
{
	return static_cast<CFMUValidateWidget *>(userdata)->loadfailed_webview(webview, load_event, failing_uri, error);
}

gboolean CFMUValidateWidget::loadfailed_webview(WebKitWebView *webview, WebKitLoadEvent load_event, gchar *failing_uri, GError *error)
{
	if ((m_portal_webview && webview == WEBKIT_WEB_VIEW(m_portal_webview->gobj())) ||
	    (m_validate_webview && webview == WEBKIT_WEB_VIEW(m_validate_webview->gobj()))) {
		set_state(state_failed);
		return true;
	}
	{
		int i(0), n(get_n_pages());
		for (; i < n; ++i) {
			const Gtk::Widget *pg(get_nth_page(i));
			const Gtk::ScrolledWindow *pgs(dynamic_cast<const Gtk::ScrolledWindow *>(pg));
			if (!pgs)
				continue;
			const Gtk::Widget *chld(pgs->get_child());
			if (!chld)
				continue;
			if (WEBKIT_WEB_VIEW(chld->gobj()) != webview)
				continue;
			remove_page(i);
			break;
		}
		if (i >= n)
			std::cerr << "close_webview: page not found" << std::endl;
	}	
	return false;
}

gboolean CFMUValidateWidget::decide_policy_1(WebKitWebView *webview, WebKitPolicyDecision *decision, WebKitPolicyDecisionType decision_type, gpointer userdata)
{
	return static_cast<CFMUValidateWidget *>(userdata)->decide_policy(webview, decision, decision_type);
}

gboolean CFMUValidateWidget::decide_policy(WebKitWebView *webview, WebKitPolicyDecision *decision, WebKitPolicyDecisionType decision_type)
{
	static const bool trace_policy = false;
	g_assert((m_portal_webview && webview == WEBKIT_WEB_VIEW(m_portal_webview->gobj())) ||
		 (m_validate_webview && webview == WEBKIT_WEB_VIEW(m_validate_webview->gobj())));
	switch (decision_type) {
	case WEBKIT_POLICY_DECISION_TYPE_NAVIGATION_ACTION:
	{
		WebKitNavigationPolicyDecision *navigation_decision = WEBKIT_NAVIGATION_POLICY_DECISION (decision);
		/* Make a policy decision here. */
		WebKitNavigationAction *action = webkit_navigation_policy_decision_get_navigation_action(navigation_decision);
		if (trace_policy)
			std::cerr << "decide_policy: navigation action: nav type "
				  << webkit_navigation_action_get_navigation_type(action)
				  << " button " << webkit_navigation_action_get_mouse_button(action)
				  << " modifiers " << webkit_navigation_action_get_modifiers(action)
				  << " uri " << webkit_uri_request_get_uri(webkit_navigation_action_get_request(action))
				  << std::endl;
		break;
	}

	case WEBKIT_POLICY_DECISION_TYPE_NEW_WINDOW_ACTION:
	{
		WebKitNavigationPolicyDecision *navigation_decision = WEBKIT_NAVIGATION_POLICY_DECISION (decision);
		/* Make a policy decision here. */
		WebKitNavigationAction *action = webkit_navigation_policy_decision_get_navigation_action(navigation_decision);
		if (trace_policy)
			std::cerr << "decide_policy: new window action: nav type "
				  << webkit_navigation_action_get_navigation_type(action)
				  << " button " << webkit_navigation_action_get_mouse_button(action)
				  << " modifiers " << webkit_navigation_action_get_modifiers(action)
				  << " uri " << webkit_uri_request_get_uri(webkit_navigation_action_get_request(action))
				  << std::endl;
		break;
	}

	case WEBKIT_POLICY_DECISION_TYPE_RESPONSE:
	{
		WebKitResponsePolicyDecision *response = WEBKIT_RESPONSE_POLICY_DECISION (decision);
		/* Make a policy decision here. */
		break;
	}

	default:
		/* Making no decision results in webkit_policy_decision_use(). */
		return false;
	}
	return false;
}

void CFMUValidateWidget::set_state(state_t newstate)
{
	static const bool log_statechange = false;
	switch (newstate) {
	case state_loadmainpage:
	case state_failed:
	case state_unloaded:
	default:
		if (newstate == state_loadmainpage && newstate == m_state)
			break;
		remove_portal_sighandlers();
		remove_validate_sighandlers();
		while (get_n_pages() > 0)
			remove_page(0);
		m_portal_webview = 0;
		m_validate_webview = 0;
		m_portal_loaded = false;
		m_validate_loaded = false;
		m_timeout.disconnect();
		if (newstate == state_failed) {
			m_timeout = Glib::signal_timeout().connect_seconds(sigc::mem_fun(*this, &CFMUValidateWidget::timeout), 15);
			if (log_statechange)
				std::cerr << "set_state: " << m_state << " -> " << state_failed << std::endl;
			m_state = state_failed;
			break;
		}
		if (newstate != state_loadmainpage) {
			if (log_statechange)
				std::cerr << "set_state: " << m_state << " -> " << state_unloaded << std::endl;
			m_state = state_unloaded;
			break;
		}
		// open webview
		{
			WebKitWebContext *ctx(webkit_web_context_get_default());
			WebKitCookieManager *cm(webkit_web_context_get_cookie_manager(ctx));
			webkit_cookie_manager_set_persistent_storage(cm, m_cookiefile.c_str(), WEBKIT_COOKIE_PERSISTENT_STORAGE_TEXT);
		}
		m_portal_webview = Gtk::manage(Glib::wrap(webkit_web_view_new()));
		m_portal_webview->show();
		add_portal_sighandlers();
		{
			WebKitWebView *webview = WEBKIT_WEB_VIEW(m_portal_webview->gobj());
			WebKitSettings *settings = webkit_web_view_get_settings(webview);
			webkit_settings_set_javascript_can_open_windows_automatically(settings, TRUE);
			webkit_web_view_load_uri(webview, "https://www.public.nm.eurocontrol.int/PUBPORTAL/gateway/spec/index.html");
		}
		{
			Gtk::ScrolledWindow *scr(Gtk::manage(new Gtk::ScrolledWindow()));
			scr->show();
			scr->add(*m_portal_webview);
			append_page(*scr, "Portal", false);
		}
		m_timeout = Glib::signal_timeout().connect_seconds(sigc::mem_fun(*this, &CFMUValidateWidget::timeout), 30);
		if (log_statechange)
			std::cerr << "set_state: " << m_state << " -> " << state_loadmainpage << std::endl;
		m_state = state_loadmainpage;
		break;

	case state_loadquerypage:
	{
		if (m_state != state_loadmainpage)
			break;
		if (!m_portal_loaded)
			break;
		guint64 page_id(0);
		WebExtProxy *proxy(0);
		{
			webext_proxy_t::const_iterator i(m_webext_proxy.begin()), e(m_webext_proxy.end());
			for (; i != e; ++i) {
				if (!i->is_portaldom(page_id))
					continue;
				proxy = &const_cast<WebExtProxy&>(*i);
				break;
			}
			if (i == e)
				break;
		}
		if (false)
			std::cerr << "Button found! Injecting script" << std::endl;
		m_timeout.disconnect();
		m_timeout = Glib::signal_timeout().connect_seconds(sigc::mem_fun(*this, &CFMUValidateWidget::timeout), 30);
		if (log_statechange)
			std::cerr << "set_state: " << m_state << " -> " << state_loadquerypage << std::endl;
		m_state = state_loadquerypage;
		proxy->launch_validate(page_id);
		break;
	}

	case state_ready:
	{
		if (m_state == state_query) {
			if (!m_validate_webview || !m_validate_webview->gobj()) {
				set_state(state_failed);
				break;
			}
			m_timeout.disconnect();
			m_timeout = Glib::signal_timeout().connect_seconds(sigc::mem_fun(*this, &CFMUValidateWidget::timeout), 10 * 60);
			if (log_statechange)
				std::cerr << "set_state: " << m_state << " -> " << state_ready << std::endl;
			m_state = state_ready;
			set_state(state_query);
			break;
		}
		if (m_state != state_loadquerypage)
			break;
		if (!m_validate_webview || !m_validate_webview->gobj())
			break;
		if (!m_validate_loaded)
			break;
		{
			webext_proxy_t::const_iterator i(m_webext_proxy.begin()), e(m_webext_proxy.end());
			for (; i != e; ++i)
				if (i->is_validatedom())
					break;
			if (i == e)
				break;
		}
		m_timeout.disconnect();
		m_timeout = Glib::signal_timeout().connect_seconds(sigc::mem_fun(*this, &CFMUValidateWidget::timeout), 10 * 60);
		if (log_statechange)
			std::cerr << "set_state: " << m_state << " -> " << state_ready << std::endl;
		m_state = state_ready;
		set_state(state_query);
		break;
	}

	case state_query:
	{
		if (m_state != state_ready)
			break;
		if (!m_validate_webview || !m_validate_webview->gobj()) {
			set_state(state_failed);
			break;
		}
		if (m_valqueue.empty())
			break;
		guint64 page_id(0);
		WebExtProxy *proxy(0);
		{
			webext_proxy_t::const_iterator i(m_webext_proxy.begin()), e(m_webext_proxy.end());
			for (; i != e; ++i) {
				if (!i->is_validatedom(page_id))
					continue;
				proxy = &const_cast<WebExtProxy&>(*i);
				break;
			}
			if (i == e)
				break;
		}
		m_timeout.disconnect();
		m_timeout = Glib::signal_timeout().connect_seconds(sigc::mem_fun(*this, &CFMUValidateWidget::timeout), 30);
		if (log_statechange)
			std::cerr << "set_state: " << m_state << " -> " << state_query << std::endl;
		m_state = state_query;
		proxy->validate(page_id, m_valqueue.begin()->first);
		break;
	}
	}
}

void CFMUValidateWidget::initialize(void)
{
	prepare_webextension();
}

void CFMUValidateWidget::finalize(void)
{
	m_webext_proxy.clear();
	remove_webextension();
	//remove_portal_sighandlers();
	//remove_validate_sighandlers();
}

void CFMUValidateWidget::cancelvalidate(void)
{
	for (webext_proxy_t::iterator i(m_webext_proxy.begin()), e(m_webext_proxy.end()); i != e; ++i) {
		WebExtProxy& proxy(const_cast<WebExtProxy&>(*i));
		proxy.validate_cancel();
	}
}

#elif defined(HAVE_WEBKITGTK)

void CFMUValidateWidget::add_portal_sighandlers(void)
{
	remove_portal_sighandlers();
	m_portal_webview_close_sighandler = g_signal_connect(m_portal_webview->gobj(), "close-web-view", G_CALLBACK(&CFMUValidateWidget::close_webview_1), this);
	m_portal_webview_ready_sighandler = g_signal_connect(m_portal_webview->gobj(), "web-view-ready", G_CALLBACK(&CFMUValidateWidget::webview_ready_1), this);
	m_portal_webview_loadstatus_sighandler = g_signal_connect(m_portal_webview->gobj(), "notify::load-status", G_CALLBACK(&CFMUValidateWidget::loadstatus_webview_1), this);
	m_portal_webview_decidepolicy_sighandler = g_signal_connect(m_portal_webview->gobj(), "navigation-policy-decision-requested", G_CALLBACK(&CFMUValidateWidget::policy_decision_required_1), this);
	m_portal_webview_newwndpolicy_sighandler = g_signal_connect(m_portal_webview->gobj(), "new-window-policy-decision-requested", G_CALLBACK(&CFMUValidateWidget::policy_decision_required_1), this);
	m_portal_webview_rsrcload_sighandler = g_signal_connect(m_portal_webview->gobj(), "resource-load-finished", G_CALLBACK(&CFMUValidateWidget::resourceload_finished_1), this);
	m_portal_webview_consolemsg_sighandler = g_signal_connect(m_portal_webview->gobj(), "console-message", G_CALLBACK(&CFMUValidateWidget::console_message_1), this);
	m_portal_webview_create_sighandler = g_signal_connect(m_portal_webview->gobj(), "create-web-view", G_CALLBACK(&CFMUValidateWidget::create_webview_1), this);
}

void CFMUValidateWidget::remove_portal_sighandlers(void)
{
	if (m_portal_webview_close_sighandler) {
		g_signal_handler_disconnect(m_portal_webview->gobj(), m_portal_webview_close_sighandler);
		m_portal_webview_close_sighandler = 0;
	}
	if (m_portal_webview_create_sighandler) {
		g_signal_handler_disconnect(m_portal_webview->gobj(), m_portal_webview_create_sighandler);
		m_portal_webview_create_sighandler = 0;
	}
	if (m_portal_webview_ready_sighandler) {
		g_signal_handler_disconnect(m_portal_webview->gobj(), m_portal_webview_ready_sighandler);
		m_portal_webview_ready_sighandler = 0;
	}
	if (m_portal_webview_loadstatus_sighandler) {
		g_signal_handler_disconnect(m_portal_webview->gobj(), m_portal_webview_loadstatus_sighandler);
		m_portal_webview_loadstatus_sighandler = 0;
	}
	if (m_portal_webview_decidepolicy_sighandler) {
		g_signal_handler_disconnect(m_portal_webview->gobj(), m_portal_webview_decidepolicy_sighandler);
		m_portal_webview_decidepolicy_sighandler = 0;
	}
	if (m_portal_webview_newwndpolicy_sighandler) {
		g_signal_handler_disconnect(m_portal_webview->gobj(), m_portal_webview_newwndpolicy_sighandler);
		m_portal_webview_newwndpolicy_sighandler = 0;
	}
	if (m_portal_webview_rsrcload_sighandler) {
		g_signal_handler_disconnect(m_portal_webview->gobj(), m_portal_webview_rsrcload_sighandler);
		m_portal_webview_rsrcload_sighandler = 0;
	}
	if (m_portal_webview_consolemsg_sighandler) {
		g_signal_handler_disconnect(m_portal_webview->gobj(), m_portal_webview_consolemsg_sighandler);
		m_portal_webview_consolemsg_sighandler = 0;
	}
}

void CFMUValidateWidget::add_validate_sighandlers(void)
{
	remove_validate_sighandlers();
	m_validate_webview_close_sighandler = g_signal_connect(m_validate_webview->gobj(), "close-web-view", G_CALLBACK(&CFMUValidateWidget::close_webview_1), this);
	m_validate_webview_ready_sighandler = g_signal_connect(m_validate_webview->gobj(), "web-view-ready", G_CALLBACK(&CFMUValidateWidget::webview_ready_1), this);
	m_validate_webview_loadstatus_sighandler = g_signal_connect(m_validate_webview->gobj(), "notify::load-status", G_CALLBACK(&CFMUValidateWidget::loadstatus_webview_1), this);
	m_validate_webview_decidepolicy_sighandler = g_signal_connect(m_validate_webview->gobj(), "navigation-policy-decision-requested", G_CALLBACK(&CFMUValidateWidget::policy_decision_required_1), this);
	m_validate_webview_newwndpolicy_sighandler = g_signal_connect(m_validate_webview->gobj(), "new-window-policy-decision-requested", G_CALLBACK(&CFMUValidateWidget::policy_decision_required_1), this);
	m_validate_webview_rsrcload_sighandler = g_signal_connect(m_validate_webview->gobj(), "resource-load-finished", G_CALLBACK(&CFMUValidateWidget::resourceload_finished_1), this);
	m_validate_webview_consolemsg_sighandler = g_signal_connect(m_validate_webview->gobj(), "console-message", G_CALLBACK(&CFMUValidateWidget::console_message_1), this);
	m_validate_webview_create_sighandler = g_signal_connect(m_validate_webview->gobj(), "create-web-view", G_CALLBACK(&CFMUValidateWidget::create_webview_1), this);
}

void CFMUValidateWidget::remove_validate_sighandlers(void)
{
	if (m_validate_webview_close_sighandler) {
		g_signal_handler_disconnect(m_validate_webview->gobj(), m_validate_webview_close_sighandler);
		m_validate_webview_close_sighandler = 0;
	}
	if (m_validate_webview_create_sighandler) {
		g_signal_handler_disconnect(m_validate_webview->gobj(), m_validate_webview_create_sighandler);
		m_validate_webview_create_sighandler = 0;
	}
	if (m_validate_webview_ready_sighandler) {
		g_signal_handler_disconnect(m_validate_webview->gobj(), m_validate_webview_ready_sighandler);
		m_validate_webview_ready_sighandler = 0;
	}
	if (m_validate_webview_loadstatus_sighandler) {
		g_signal_handler_disconnect(m_validate_webview->gobj(), m_validate_webview_loadstatus_sighandler);
		m_validate_webview_loadstatus_sighandler = 0;
	}
	if (m_validate_webview_decidepolicy_sighandler) {
		g_signal_handler_disconnect(m_validate_webview->gobj(), m_validate_webview_decidepolicy_sighandler);
		m_validate_webview_decidepolicy_sighandler = 0;
	}
	if (m_validate_webview_newwndpolicy_sighandler) {
		g_signal_handler_disconnect(m_validate_webview->gobj(), m_validate_webview_newwndpolicy_sighandler);
		m_validate_webview_newwndpolicy_sighandler = 0;
	}
	if (m_validate_webview_rsrcload_sighandler) {
		g_signal_handler_disconnect(m_validate_webview->gobj(), m_validate_webview_rsrcload_sighandler);
		m_validate_webview_rsrcload_sighandler = 0;
	}
	if (m_validate_webview_consolemsg_sighandler) {
		g_signal_handler_disconnect(m_validate_webview->gobj(), m_validate_webview_consolemsg_sighandler);
		m_validate_webview_consolemsg_sighandler = 0;
	}
}

gboolean CFMUValidateWidget::close_webview_1(WebKitWebView* webview, gpointer userdata)
{
	return static_cast<CFMUValidateWidget *>(userdata)->close_webview(webview);
}

gboolean CFMUValidateWidget::close_webview(WebKitWebView* webview)
{
	if ((m_portal_webview && webview == WEBKIT_WEB_VIEW(m_portal_webview->gobj())) ||
	    (m_validate_webview && webview == WEBKIT_WEB_VIEW(m_validate_webview->gobj()))) {
		set_state(state_failed);
		return true;
	}
	{
		int i(0), n(get_n_pages());
		for (; i < n; ++i) {
			const Gtk::Widget *pg(get_nth_page(i));
			const Gtk::ScrolledWindow *pgs(dynamic_cast<const Gtk::ScrolledWindow *>(pg));
			if (!pgs)
				continue;
			const Gtk::Widget *chld(pgs->get_child());
			if (!chld)
				continue;
			if (WEBKIT_WEB_VIEW(chld->gobj()) != webview)
				continue;
			remove_page(i);
			break;
		}
		if (i >= n)
			std::cerr << "close_webview: page not found" << std::endl;
	}	
	return true;
}

gboolean CFMUValidateWidget::webview_ready_1(WebKitWebView* webview, gpointer userdata)
{
	return static_cast<CFMUValidateWidget *>(userdata)->webview_ready(webview);
}

gboolean CFMUValidateWidget::webview_ready(WebKitWebView* webview)
{
	if (true)
		std::cerr << "WebView: ready" << std::endl;
	gtk_widget_show_all(GTK_WIDGET(webview));
	return false;
}

gboolean CFMUValidateWidget::result_change_1(WebKitDOMElement *target, WebKitDOMEvent *e, gpointer userdata)
{
	return static_cast<CFMUValidateWidget *>(userdata)->result_change(target, e);
}

gboolean CFMUValidateWidget::result_change(WebKitDOMElement *target, WebKitDOMEvent *e)
{
	std::cerr << "result change! state " << (int)get_state() << std::endl;
	switch (get_state()) {
	case state_query:
		set_state(state_ready);
		break;

	default:
		break;
	}
	return true;
}

WebKitWebView *CFMUValidateWidget::create_webview_1(WebKitWebView *webview, WebKitWebFrame *frame, gpointer userdata)
{
	return static_cast<CFMUValidateWidget *>(userdata)->create_webview(webview, frame);
}

WebKitWebView *CFMUValidateWidget::create_webview(WebKitWebView *webview, WebKitWebFrame *frame)
{
	std::cerr << "new webview! state " << (int)get_state() << std::endl;
	switch (get_state()) {
	case state_loadmainpage:
	case state_loadquerypage:
	case state_ready:
	case state_query:
	case state_waitresult:
		break;

	default:
		return 0;
	}
	Gtk::Widget *wvw(Gtk::manage(Glib::wrap(webkit_web_view_new())));
	WebKitWebView *wv(WEBKIT_WEB_VIEW(wvw->gobj()));
	Gtk::ScrolledWindow *scr(Gtk::manage(new Gtk::ScrolledWindow()));
	scr->show();
	scr->add(*wvw);
	if (get_state() == state_loadquerypage &&
	    m_portal_webview && webview == WEBKIT_WEB_VIEW(m_portal_webview->gobj())) {
		remove_validate_sighandlers();
		m_validate_webview = wvw;
		m_validate_webview->show();
		webkit_web_view_set_settings(WEBKIT_WEB_VIEW(m_validate_webview->gobj()), webkit_web_view_get_settings(WEBKIT_WEB_VIEW(m_portal_webview->gobj())));
		add_validate_sighandlers();
		set_current_page(append_page(*scr, "Validate", false));
		return wv;
	}
	wvw->show();
	append_page(*scr, "Unknown", false);
	return wv;
}

void CFMUValidateWidget::loadstatus_webview_1(WebKitWebView *webview, GParamSpec *pspec, gpointer userdata)
{
	static_cast<CFMUValidateWidget *>(userdata)->loadstatus_webview(webview, pspec);
}

void CFMUValidateWidget::loadstatus_webview(WebKitWebView *webview, GParamSpec *pspec)
{
	WebKitLoadStatus status(webkit_web_view_get_load_status(webview));
	GObject *object(G_OBJECT(webview));
	g_object_freeze_notify(object);
	switch (status) {
	case WEBKIT_LOAD_FINISHED:
		if (true)
			std::cerr << "WebView: load finished" << std::endl;
		gtk_widget_show_all(GTK_WIDGET(webview));
		break;

	case WEBKIT_LOAD_PROVISIONAL:
		if (true)
			std::cerr << "WebView: load provisional" << std::endl;
		break;

	case WEBKIT_LOAD_COMMITTED:
		if (true)
			std::cerr << "WebView: load committed" << std::endl;
		break;

	case WEBKIT_LOAD_FIRST_VISUALLY_NON_EMPTY_LAYOUT:
		if (true)
			std::cerr << "WebView: load first visually nonempty layout" << std::endl;
		break;

	case WEBKIT_LOAD_FAILED:
		if (true)
			std::cerr << "WebView: load failed" << std::endl;
		if ((m_validate_webview && webview == WEBKIT_WEB_VIEW(m_validate_webview->gobj())) ||
		    (m_portal_webview && webview == WEBKIT_WEB_VIEW(m_portal_webview->gobj())))
			set_state(state_failed);
		break;

	default:
		break;
	}
	g_object_thaw_notify(object);
}

gboolean CFMUValidateWidget::policy_decision_required_1(WebKitWebView *webview, WebKitWebFrame *webframe, WebKitNetworkRequest *request,
							      WebKitWebNavigationAction *action, WebKitWebPolicyDecision *decision, gpointer userdata)
{
	return static_cast<CFMUValidateWidget *>(userdata)->policy_decision_required(webview, webframe, request, action, decision);
}

gboolean CFMUValidateWidget::policy_decision_required(WebKitWebView *webview, WebKitWebFrame *webframe, WebKitNetworkRequest *request,
							    WebKitWebNavigationAction *action, WebKitWebPolicyDecision *decision)
{
	g_assert((m_portal_webview && webview == WEBKIT_WEB_VIEW(m_portal_webview->gobj())) ||
		 (m_validate_webview && webview == WEBKIT_WEB_VIEW(m_validate_webview->gobj())));
	WebKitWebNavigationReason reason(webkit_web_navigation_action_get_reason(action));
	gint button(webkit_web_navigation_action_get_button(action));
	gint state(webkit_web_navigation_action_get_modifier_state(action));
	const char *uri(webkit_network_request_get_uri(request));
	if (true)
		std::cerr << "policy_decision_required: reason " << reason << " button " << button << " state " << state
			  << " uri " << uri << std::endl;
	return false;
}

void CFMUValidateWidget::resourceload_finished_1(WebKitWebView *webview, WebKitWebFrame *webframe, WebKitWebResource *webresource, gpointer userdata)
{
	static_cast<CFMUValidateWidget *>(userdata)->resourceload_finished(webview, webframe, webresource);
}

void CFMUValidateWidget::resourceload_finished(WebKitWebView *webview, WebKitWebFrame *webframe, WebKitWebResource *webresource)
{
	bool is_portal(m_portal_webview && webview == WEBKIT_WEB_VIEW(m_portal_webview->gobj()));
	bool is_validate(m_validate_webview && webview == WEBKIT_WEB_VIEW(m_validate_webview->gobj()));
	if (!is_portal && !is_validate)
		return;
 	if (true) {
		time_t t(time(0));
		char buf[64];
		std::cerr << "resource load finished! " << ctime_r(&t, buf) << " state " << (int)get_state() << std::endl;
	}
	switch (get_state()) {
	case state_loadmainpage:
	{
		if (!is_portal)
			break;
		set_state(state_loadquerypage);
		break;
	}

	case state_loadquerypage:
	{
		if (!is_validate)
			break;
		set_state(state_ready);
		break;
	}

	case state_query:
	{
		if (!is_validate)
			break;
		set_state(state_ready);
		break;
	}

	default:
		break;
	}
}

gboolean CFMUValidateWidget::console_message_1(WebKitWebView *webview, gchar *message, gint line, gchar *source_id, gpointer userdata)
{
	return static_cast<CFMUValidateWidget *>(userdata)->console_message(webview, message, line, source_id);
}

gboolean CFMUValidateWidget::console_message(WebKitWebView *webview, gchar *message, gint line, gchar *source_id)
{
	if (true) {
		std::cerr << message << " (" << (source_id ? "*null*" : source_id) << ':' << line << ')' << std::endl;
	}
	return true;
}

CFMUValidateWidget::validate_result_t CFMUValidateWidget::parse_results(void)
{
	if (!m_validate_webview) {
		set_state(state_failed);
		return validate_result_t();
	}
	typedef std::pair<guint,bool> lvlvisible_t;
	typedef std::pair<WebKitDOMNode *,lvlvisible_t> nodelvlvisible_t;
	std::stack<nodelvlvisible_t> subtree;
	{
		WebKitWebView *webview(WEBKIT_WEB_VIEW(m_validate_webview->gobj()));
		WebKitDOMDocument *document(webkit_web_view_get_dom_document(webview));
		WebKitDOMElement *element(webkit_dom_document_get_element_by_id(document, "FREE_TEXT_EDITOR.VALIDATION_RESULTS_AREA"));
		if (!element || !WEBKIT_DOM_IS_NODE(element)) {
			set_state(state_failed);
			return validate_result_t();
		}
		subtree.push(std::make_pair(WEBKIT_DOM_NODE(element), std::make_pair(0U, true)));
	}
        validate_result_t results;
	while (!subtree.empty()) {
		WebKitDOMNode *node;
		guint level;
		bool visible;
		{
			nodelvlvisible_t nl(subtree.top());
			node = nl.first;
			level = nl.second.first;
			visible = nl.second.second;
		}
		subtree.pop();
		if (WEBKIT_DOM_IS_HTML_ELEMENT(node)) {
			WebKitDOMCSSStyleDeclaration *style(webkit_dom_element_get_style(WEBKIT_DOM_ELEMENT(node)));
			if (style) {
				gchar *disp(webkit_dom_css_style_declaration_get_property_value(style, "display"));
				if (disp) {
					visible = !!strcmp(disp, "none");
					g_free(disp);
				}
			}
		}
		if (visible) {
			WebKitDOMNodeList *list(webkit_dom_node_get_child_nodes(node));
			gulong i(webkit_dom_node_list_get_length(list));
			while (i) {
				--i;
				WebKitDOMNode *ptr(webkit_dom_node_list_item(list, i));
				if (!ptr)
					continue;
				subtree.push(std::make_pair(ptr, std::make_pair(level + 1U, visible)));
			}
		}
		if (!WEBKIT_DOM_IS_HTML_ELEMENT(node))
			continue;
		char *class_name(webkit_dom_html_element_get_class_name(WEBKIT_DOM_HTML_ELEMENT(node)));
		if (false) {
			std::cerr << std::string(2U * level, ' ') << "element:";
			gchar *lname(webkit_dom_node_get_local_name(WEBKIT_DOM_NODE(node)));
			if (lname) {
				if (*lname)
					std::cerr << " name \"" << lname << "\"";
				g_free(lname);
			}
			lname = webkit_dom_html_element_get_id(WEBKIT_DOM_HTML_ELEMENT(node));
			if (lname) {
				if (*lname)
					std::cerr << " id \"" << lname << "\"";
				g_free(lname);
			}
			const gchar *typ(g_type_name(G_TYPE_FROM_INSTANCE(node)));
			if (typ && *typ)
				std::cerr << " type \"" << typ << "\"";
			if (class_name && *class_name)
				std::cerr << " class \"" << class_name << "\"";
			WebKitDOMCSSStyleDeclaration *style(webkit_dom_element_get_style(WEBKIT_DOM_ELEMENT(node)));
			if (style) {
				gchar *disp(webkit_dom_css_style_declaration_get_property_value(style, "display"));
				if (disp) {
					if (*disp)
						std::cerr << " display \"" << disp << "\"" << std::endl;
					g_free(disp);
				}
			}
			gchar *txt(webkit_dom_html_element_get_inner_text(WEBKIT_DOM_HTML_ELEMENT(node)));
			if (txt) {
				if (*txt)
					std::cerr << " text \"" << txt << "\"";
				g_free(txt);
			}
			std::cerr << std::endl;
		}
		if (!class_name)
			continue;
		if (!visible) {
			g_free(class_name);
			continue;
		}
		if (!strcmp(class_name, "portal_dataValue") && WEBKIT_DOM_IS_HTML_DIV_ELEMENT(node)) {
			gchar *txt(webkit_dom_html_element_get_inner_text(WEBKIT_DOM_HTML_ELEMENT(node)));
			if (txt) {
				txt = g_strstrip(txt);
				if (*txt) {
					results.push_back(Glib::ustring(txt));
					if (true)
						std::cerr << "*** " << txt << std::endl;
				}
				g_free(txt);
			}
		}
		if (false && !strcmp(class_name, "portal_decotableCell ") && WEBKIT_DOM_IS_HTML_TABLE_CELL_ELEMENT(node)) {
			gchar *txt(webkit_dom_html_element_get_inner_text(WEBKIT_DOM_HTML_ELEMENT(node)));
			if (txt) {
				txt = g_strstrip(txt);
				if (*txt) {
					results.push_back(Glib::ustring(txt));
					if (true)
						std::cerr << "*** " << txt << std::endl;
				}
				g_free(txt);
			}
		}
		if (true && (!strcmp(class_name, "portal_rowOdd") || !strcmp(class_name, "portal_rowEven")) &&
		    WEBKIT_DOM_IS_HTML_TABLE_ROW_ELEMENT(node)) {
			gchar *txt(webkit_dom_html_element_get_inner_text(WEBKIT_DOM_HTML_ELEMENT(node)));
			if (txt) {
				txt = g_strstrip(txt);
				if (*txt) {
					results.push_back(Glib::ustring(txt));
					if (true)
						std::cerr << "*** " << txt << std::endl;
				}
				g_free(txt);
			}
		}
		g_free(class_name);
	}
	return results;
}

void CFMUValidateWidget::zap_errors(void)
{
	if (!m_validate_webview)
		return;
	std::stack<WebKitDOMNode *> subtree;
	{
		WebKitWebView *webview(WEBKIT_WEB_VIEW(m_validate_webview->gobj()));
		WebKitDOMDocument *document(webkit_web_view_get_dom_document(webview));
		WebKitDOMElement *element(webkit_dom_document_get_element_by_id(document, "FREE_TEXT_EDITOR.VALIDATION_RESULTS_AREA"));
		if (!element || !WEBKIT_DOM_IS_NODE(element))
			return;
		subtree.push(WEBKIT_DOM_NODE(element));
	}
	while (!subtree.empty()) {
		WebKitDOMNode *node(subtree.top());
		subtree.pop();
		if (WEBKIT_DOM_IS_HTML_ELEMENT(node)) {
			char *class_name(webkit_dom_html_element_get_class_name(WEBKIT_DOM_HTML_ELEMENT(node)));
			if (class_name) {
		     		if ((!strcmp(class_name, "portal_rowOdd") || !strcmp(class_name, "portal_rowEven")) &&
				    WEBKIT_DOM_IS_HTML_TABLE_ROW_ELEMENT(node)) {
					g_free(class_name);
					while (webkit_dom_node_has_child_nodes(node)) {
						GError *error(0);
						webkit_dom_node_remove_child(node, webkit_dom_node_get_first_child(node), &error);
						if (error) {
							std::cerr << "Cannot remove child: " << error->message << std::endl;
							g_error_free(error);
						}
					}
					continue;
				}
				g_free(class_name);
			}
		}
		{
			WebKitDOMNodeList *list(webkit_dom_node_get_child_nodes(node));
			gulong i(webkit_dom_node_list_get_length(list));
			while (i) {
				--i;
				WebKitDOMNode *ptr(webkit_dom_node_list_item(list, i));
				if (!ptr)
					continue;
				subtree.push(ptr);
			}
		}
	}
}

void CFMUValidateWidget::set_state(state_t newstate)
{
	switch (newstate) {
	case state_loadmainpage:
	case state_failed:
	case state_unloaded:
	default:
		if (newstate == state_loadmainpage && newstate == m_state)
			break;
		remove_portal_sighandlers();
		remove_validate_sighandlers();
		while (get_n_pages() > 0)
			remove_page(0);
		m_portal_webview = 0;
		m_validate_webview = 0;
		m_timeout.disconnect();
		if (newstate == state_failed) {
			m_timeout = Glib::signal_timeout().connect_seconds(sigc::mem_fun(*this, &CFMUValidateWidget::timeout), 15);
			m_state = state_failed;
			break;
		}
		if (newstate != state_loadmainpage) {
			m_state = state_unloaded;
			break;
		}
		// add cookie jar
		if (!m_cookiefile.empty()) {
			SoupSession *session(webkit_get_default_session());
			SoupCookieJar *cookiejar(soup_cookie_jar_text_new(m_cookiefile.c_str(), false));
			soup_session_add_feature(session, SOUP_SESSION_FEATURE(cookiejar));
			SoupCookie *bwarn(soup_cookie_new("eurocontrol_nm_portal_hideBroswerWarning", "hide", "www.public.nm.eurocontrol.int", "/", -1));
			soup_cookie_jar_add_cookie(cookiejar, bwarn);
			bwarn = soup_cookie_new("eurocontrol_nm_portal_hideCookieWarning", "hide", "www.public.nm.eurocontrol.int", "/", -1);
			soup_cookie_jar_add_cookie(cookiejar, bwarn);
			g_object_unref(cookiejar);
		}
		// open webview
		m_portal_webview = Gtk::manage(Glib::wrap(webkit_web_view_new()));
		m_portal_webview->show();
		add_portal_sighandlers();
		{
			WebKitWebView *webview = WEBKIT_WEB_VIEW(m_portal_webview->gobj());
			webkit_web_view_load_uri(webview, "https://www.public.nm.eurocontrol.int/PUBPORTAL/gateway/spec/index.html");
		}
		{
			Gtk::ScrolledWindow *scr(Gtk::manage(new Gtk::ScrolledWindow()));
			scr->show();
			scr->add(*m_portal_webview);
			append_page(*scr, "Portal", false);
		}
		m_timeout = Glib::signal_timeout().connect_seconds(sigc::mem_fun(*this, &CFMUValidateWidget::timeout), 30);
		m_state = state_loadmainpage;
		break;

	case state_loadquerypage:
	{
		if (m_state != state_loadmainpage)
			break;
		if (!m_portal_webview || !m_portal_webview->gobj())
			break;
		WebKitDOMDocument *document(webkit_web_view_get_dom_document(WEBKIT_WEB_VIEW(m_portal_webview->gobj())));
		if (!document)
			break;
		WebKitDOMElement *element(webkit_dom_document_get_element_by_id(document, "IFPUV_LAUNCH_AREA.FREE_TEXT_EDIT_LINK_LABEL"));
		if (!element)
			break;
		if (false)
			std::cerr << "Button found! Injecting script" << std::endl;
		m_timeout.disconnect();
		m_timeout = Glib::signal_timeout().connect_seconds(sigc::mem_fun(*this, &CFMUValidateWidget::timeout), 30);
		m_state = state_loadquerypage;
		webkit_web_view_execute_script(WEBKIT_WEB_VIEW(m_portal_webview->gobj()), "document.getElementById(\"IFPUV_LAUNCH_AREA.FREE_TEXT_EDIT_LINK_LABEL\").click();");
		break;
	}

	case state_ready:
	{
		if (!m_validate_webview || !m_validate_webview->gobj()) {
			set_state(state_failed);
			break;
		}
		if (m_state == state_query) {
			validate_result_t vr(parse_results());
			if (vr.empty())
				break;
			if (!m_valqueue.empty()) {
				m_valqueue.begin()->second(vr);
				m_valqueue.erase(m_valqueue.begin());
			}
			m_timeout.disconnect();
			m_timeout = Glib::signal_timeout().connect_seconds(sigc::mem_fun(*this, &CFMUValidateWidget::timeout), 10 * 60);
			m_state = state_ready;
			set_state(state_query);
			break;
		}
		if (m_state != state_loadquerypage)
			break;
		WebKitDOMDocument *document(webkit_web_view_get_dom_document(WEBKIT_WEB_VIEW(m_validate_webview->gobj())));
		if (!document)
			break;
		WebKitDOMElement *element(webkit_dom_document_get_element_by_id(document, "FREE_TEXT_EDITOR.FLIGHT_DATA_AREA.VALIDATE_ACTION_LABEL"));
		if (!element)
			break;
		if (true) {
			const gchar *typ(g_type_name(G_TYPE_FROM_INSTANCE(element)));
			if (typ)
				std::cerr << "Validate element type " << typ << std::endl;
		}
		element = webkit_dom_document_get_element_by_id(document, "FREE_TEXT_EDITOR.FLIGHT_DATA_AREA.GENERAL_DATA_ENTRY.INTRODUCE_FLIGHT_PLAN_FIELD");
		if (!element)
			break;
		if (true) {
			const gchar *typ(g_type_name(G_TYPE_FROM_INSTANCE(element)));
			if (typ)
				std::cerr << "FPL entry element type " << typ << std::endl;
		}
		if (!WEBKIT_DOM_IS_HTML_TEXT_AREA_ELEMENT(element))
			break;
		element = webkit_dom_document_get_element_by_id(document, "FREE_TEXT_EDITOR.VALIDATION_RESULTS_AREA.ERRORS_DATA_TABLE");
		if (!element)
			break;
		if (true) {
			const gchar *typ(g_type_name(G_TYPE_FROM_INSTANCE(element)));
			if (typ)
				std::cerr << "Result Errors element type " << typ << std::endl;
		}
		element = webkit_dom_document_get_element_by_id(document, "FREE_TEXT_EDITOR.VALIDATION_RESULTS_AREA");
		if (!element)
			break;
		webkit_dom_event_target_add_event_listener(WEBKIT_DOM_EVENT_TARGET(element), "DOMSubtreeModified", G_CALLBACK(&CFMUValidateWidget::result_change_1), true, this);
		if (true) {
			const gchar *typ(g_type_name(G_TYPE_FROM_INSTANCE(element)));
			if (typ)
				std::cerr << "Results element type " << typ << std::endl;
		}
		m_timeout.disconnect();
		m_timeout = Glib::signal_timeout().connect_seconds(sigc::mem_fun(*this, &CFMUValidateWidget::timeout), 10 * 60);
		m_state = state_ready;
		set_state(state_query);
		break;
	}

	case state_query:
	{
		if (m_state != state_ready)
			break;
		if (m_valqueue.empty())
			break;
		if (!m_validate_webview || !m_validate_webview->gobj()) {
			set_state(state_failed);
			break;
		}
		WebKitWebView *webview(WEBKIT_WEB_VIEW(m_validate_webview->gobj()));
		WebKitDOMDocument *document(webkit_web_view_get_dom_document(webview));
		if (!document) {
			set_state(state_failed);
			break;
		}
		WebKitDOMElement *element(webkit_dom_document_get_element_by_id(document, "FREE_TEXT_EDITOR.FLIGHT_DATA_AREA.GENERAL_DATA_ENTRY.INTRODUCE_FLIGHT_PLAN_FIELD"));
		if (!element || !WEBKIT_DOM_IS_HTML_TEXT_AREA_ELEMENT(element)) {
			set_state(state_failed);
			break;
		}
		webkit_dom_html_text_area_element_set_value(WEBKIT_DOM_HTML_TEXT_AREA_ELEMENT(element), m_valqueue.begin()->first.c_str());
		element = webkit_dom_document_get_element_by_id(document, "FREE_TEXT_EDITOR.FLIGHT_DATA_AREA.VALIDATE_ACTION_LABEL");
		if (!element) {
			set_state(state_failed);
			break;
		}
		zap_errors();
		webkit_web_view_execute_script(webview, "document.getElementById(\"FREE_TEXT_EDITOR.FLIGHT_DATA_AREA.VALIDATE_ACTION_LABEL\").click();");
		m_timeout.disconnect();
		m_timeout = Glib::signal_timeout().connect_seconds(sigc::mem_fun(*this, &CFMUValidateWidget::timeout), 30);
		m_state = state_query;
		break;
	}
	}
}

void CFMUValidateWidget::initialize(void)
{
}

void CFMUValidateWidget::finalize(void)
{
	//remove_portal_sighandlers();
	//remove_validate_sighandlers();
}

void CFMUValidateWidget::cancelvalidate(void)
{
}

#endif
