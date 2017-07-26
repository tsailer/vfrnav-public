/* -*- Mode: C; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*- */
/*
 *  Copyright © 2014 Igalia S.L.
 *  Copyright © 2016 autorouter.eu
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2, or (at your option)
 *  any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "config.h"
#include "cfmu-web-extension.h"

#include "cfmu-web-extension-names.h"

#include <gio/gio.h>
#include <gtk/gtk.h>
#include <libsoup/soup.h>
#include <string.h>
#include <webkit2/webkit-web-extension.h>
#define WEBKIT_DOM_USE_UNSTABLE_API
#include <webkitdom/WebKitDOMDOMWindowUnstable.h>
#include <JavaScriptCore/JavaScript.h>

struct _CFMUWebExtension {
  GObject parent_instance;

  WebKitWebExtension *extension;
  gboolean initialized;

  GDBusConnection *dbus_connection;
  GCancellable *cancellable;
  GArray *page_created_signals_pending;
  GDBusMethodInvocation *validate_invocation;
};

static const char introspection_xml[] =
  "<node>"
  " <interface name='eu.autorouter.cfmu.WebExtension'>"
  "  <signal name='PageCreated'>"
  "   <arg type='t' name='page_id' direction='out'/>"
  "  </signal>"
  "  <signal name='DomChanged'>"
  "   <arg type='t' name='page_id' direction='out'/>"
  "   <arg type='b' name='portal_dom' direction='out'/>"
  "   <arg type='b' name='validate_dom' direction='out'/>"
  "  </signal>"
  "  <method name='LaunchValidate'>"
  "   <arg type='t' name='page_id' direction='in'/>"
  "  </method>"
  "  <method name='Validate'>"
  "   <arg type='t' name='page_id' direction='in'/>"
  "   <arg type='s' name='fpl' direction='in'/>"
  "   <arg type='as' name='errors' direction='out'/>"
  "  </method>"
  " </interface>"
  "</node>";

G_DEFINE_TYPE (CFMUWebExtension, cfmu_web_extension, G_TYPE_OBJECT)

static gboolean
web_page_send_request (WebKitWebPage     *web_page,
                       WebKitURIRequest  *request,
                       WebKitURIResponse *redirected_response,
                       CFMUWebExtension  *extension)
{
  const char *request_uri;
  const char *page_uri;
  gboolean ret;

  request_uri = webkit_uri_request_get_uri (request);
  if (FALSE)
    g_message("send_request: %s\n", request_uri);

  /* do not modify requests for now */
  return FALSE;
}

static void
web_page_console_message (WebKitWebPage        *web_page,
                          WebKitConsoleMessage *console_message,
                          CFMUWebExtension     *extension)
{
  g_message("console: %s\n", webkit_console_message_get_text(console_message));
}

static void
cfmu_web_extension_emit_dom_changed (CFMUWebExtension *extension,
                                     guint64           page_id,
                                     gboolean          portal_dom,
                                     gboolean          validate_dom)
{
  GError *error = NULL;

  g_dbus_connection_emit_signal (extension->dbus_connection,
                                 NULL,
                                 CFMU_WEB_EXTENSION_OBJECT_PATH,
                                 CFMU_WEB_EXTENSION_INTERFACE,
                                 "DomChanged",
                                 g_variant_new ("(tbb)", page_id, portal_dom, validate_dom),
                                 &error);
  if (error) {
    g_warning ("Error emitting signal DomChanged: %s\n", error->message);
    g_error_free (error);
  }
}

static void
print_dom (WebKitDOMElement *el, guint indent)
{
  if (!el)
    return;
  g_message("%*sID: %s Class: %s\n", indent, "", webkit_dom_element_get_id(el), webkit_dom_element_get_class_name(el));
  indent += 2;
  for (el = webkit_dom_element_get_first_element_child(el); el; el = webkit_dom_element_get_next_element_sibling(el))
    print_dom(el, indent);
}

typedef struct _ParseNode {
  WebKitDOMNode *node;
  guint level;
  gboolean visible;
} ParseNode;

static void
web_page_parse_results (CFMUWebExtension *extension, WebKitDOMNode *node)
{
  static const gboolean trace_subtree = FALSE;
  static const gboolean trace_results = FALSE;
  static const gboolean trace_dom = FALSE;
  GVariantBuilder builder;
  GArray *subtree;
  guint result_count = 0;
  if (!node || !extension->validate_invocation)
    return;
  g_variant_builder_init (&builder, G_VARIANT_TYPE ("as"));
  subtree = g_array_new (FALSE, FALSE, sizeof (ParseNode));
  {
    ParseNode pn;
    pn.node = node;
    pn.level = 0;
    pn.visible = TRUE;
    subtree = g_array_append_val (subtree, pn);
  }
	while (subtree->len) {
    gchar *class_name;
    ParseNode pn = g_array_index (subtree, ParseNode, 0);
    if (trace_subtree)
      g_message ("subtree len %u node %p level %u visible %u\n", subtree->len, pn.node, pn.level, pn.visible);
    subtree = g_array_remove_index (subtree, 0);
    if (trace_subtree)
      g_message ("subtree len %u after remove\n", subtree->len);
 		if (WEBKIT_DOM_IS_ELEMENT (pn.node)) {
			WebKitDOMCSSStyleDeclaration *style = webkit_dom_element_get_style (WEBKIT_DOM_ELEMENT(pn.node));
			if (style) {
				gchar *disp = webkit_dom_css_style_declaration_get_property_value (style, "display");
				if (disp) {
					pn.visible = !!g_strcmp0 (disp, "none");
					g_free(disp);
				}
			}
		}
		if (pn.visible && webkit_dom_node_has_child_nodes (pn.node)) {
      ParseNode pn1;
      pn1.level = pn.level + 1;
      pn1.visible = pn.visible;
      pn1.node = webkit_dom_node_get_first_child (pn.node);
			while (pn1.node) {
        subtree = g_array_append_val (subtree, pn1);
        if (trace_subtree)
          g_message ("subtree add len %u node %p level %u visible %u\n", subtree->len, pn1.node, pn1.level, pn1.visible);
        pn1.node = webkit_dom_node_get_next_sibling (pn1.node);
      }
    }
		if (!WEBKIT_DOM_IS_ELEMENT (pn.node))
			continue;
    class_name = webkit_dom_element_get_class_name (WEBKIT_DOM_ELEMENT (pn.node));
		if (trace_dom) {
      gchar *msg = g_strdup_printf ("%*selement:", 2U * pn.level, "");
			gchar *lname = webkit_dom_node_get_local_name (WEBKIT_DOM_NODE (pn.node));
      const gchar *typ;
      gchar *txt;
      WebKitDOMCSSStyleDeclaration *style;
      gchar *msg2;
			if (lname) {
				if (*lname) {
          msg2 = g_strdup_printf ("%s name \"%s\"", msg, lname);
          g_free (msg);
          msg = msg2;
        }
				g_free(lname);
			}
			lname = webkit_dom_element_get_id (WEBKIT_DOM_ELEMENT (pn.node));
			if (lname) {
				if (*lname) {
          msg2 = g_strdup_printf ("%s id \"%s\"", msg, lname);
          g_free (msg);
          msg = msg2;
        }
				g_free(lname);
			}
			typ = g_type_name( G_TYPE_FROM_INSTANCE (pn.node));
			if (typ && *typ) {
        msg2 = g_strdup_printf ("%s type \"%s\"", msg, typ);
        g_free (msg);
        msg = msg2;
      }
			if (class_name && *class_name) {
        msg2 = g_strdup_printf ("%s class \"%s\"", msg, class_name);
        g_free (msg);
        msg = msg2;
      }
			style = webkit_dom_element_get_style (WEBKIT_DOM_ELEMENT (node));
			if (style) {
				gchar *disp = webkit_dom_css_style_declaration_get_property_value (style, "display");
				if (disp) {
					if (*disp) {
            msg2 = g_strdup_printf ("%s display \"%s\"", msg, disp);
            g_free (msg);
            msg = msg2;
          }
          g_free(disp);
				}
			}
			txt = webkit_dom_html_element_get_inner_text (WEBKIT_DOM_HTML_ELEMENT(node));
			if (txt) {
				if (*txt) {
          msg2 = g_strdup_printf ("%s text \"%s\"", msg, txt);
          g_free (msg);
          msg = msg2;
        }
				g_free(txt);
			}
      g_message("%s", msg);
      g_free (msg);
		}
    if (!class_name)
			continue;
		if (!pn.visible) {
			g_free (class_name);
			continue;
		}
		if (!g_strcmp0 (class_name, "portal_dataValue") && WEBKIT_DOM_IS_HTML_DIV_ELEMENT (pn.node)) {
			gchar *txt = webkit_dom_html_element_get_inner_text (WEBKIT_DOM_HTML_ELEMENT (pn.node));
			if (txt) {
        for (gchar *txt1 = txt; *txt1; ++txt1)
          if (*txt1 == 9 || *txt1 == 10) {
            *txt1 = 0;
            break;
          }
				txt = g_strstrip(txt);
				if (*txt) {
          g_variant_builder_add (&builder, "s", txt);
          ++result_count;
					if (trace_results)
            g_message("*** %s\n", txt);
				}
				g_free(txt);
			}
		}
		if (false && !g_strcmp0(class_name, "portal_decotableCell ") && WEBKIT_DOM_IS_HTML_TABLE_CELL_ELEMENT (pn.node)) {
			gchar *txt = webkit_dom_html_element_get_inner_text (WEBKIT_DOM_HTML_ELEMENT(pn.node));
			if (txt) {
        for (gchar *txt1 = txt; *txt1; ++txt1)
          if (*txt1 == 9 || *txt1 == 10) {
            *txt1 = 0;
            break;
          }
				txt = g_strstrip(txt);
				if (*txt) {
          g_variant_builder_add (&builder, "s", txt);
          ++result_count;
					if (trace_results)
            g_message ("*** %s\n", txt);
				}
				g_free(txt);
			}
		}
		if (true && (!g_strcmp0 (class_name, "portal_rowOdd") || !g_strcmp0 (class_name, "portal_rowEven")) &&
		    WEBKIT_DOM_IS_HTML_TABLE_ROW_ELEMENT (pn.node)) {
			gchar *txt = webkit_dom_html_element_get_inner_text (WEBKIT_DOM_HTML_ELEMENT (pn.node));
			if (txt) {
        for (gchar *txt1 = txt; *txt1; ++txt1)
          if (*txt1 == 9 || *txt1 == 10) {
            *txt1 = 0;
            break;
          }
				txt = g_strstrip(txt);
				if (*txt) {
          g_variant_builder_add (&builder, "s", txt);
          ++result_count;
					if (trace_results)
            g_message ("*** %s\n", txt);
				}
				g_free(txt);
			}
		}
		g_free(class_name);
	}
  g_array_free (subtree, TRUE);
  if (result_count) {
    g_dbus_method_invocation_return_value (extension->validate_invocation, g_variant_new ("(as)", &builder));
    extension->validate_invocation = 0;
    return;
  }
  g_variant_builder_clear (&builder);
}

static void
zap_errors (WebKitDOMDocument *document)
{
  GArray *subtree = g_array_new (FALSE, FALSE, sizeof (WebKitDOMNode *));
	if (document) {
		WebKitDOMElement *element = webkit_dom_document_get_element_by_id (document, "FREE_TEXT_EDITOR.VALIDATION_RESULTS_AREA");
    if (element && WEBKIT_DOM_IS_NODE (element)) {
      WebKitDOMNode *node = WEBKIT_DOM_NODE (element);
      subtree = g_array_append_val (subtree, node);
    }
	}
	while (subtree->len) {
    WebKitDOMNode *node = g_array_index (subtree, WebKitDOMNode *, 0);
    subtree = g_array_remove_index (subtree, 0);
		if (WEBKIT_DOM_IS_ELEMENT (node)) {
			gchar *class_name = webkit_dom_element_get_class_name (WEBKIT_DOM_ELEMENT(node));
			if (class_name) {
        if ((!g_strcmp0 (class_name, "portal_rowOdd") || !g_strcmp0 (class_name, "portal_rowEven")) &&
				    WEBKIT_DOM_IS_HTML_TABLE_ROW_ELEMENT (node)) {
					g_free(class_name);
					while (webkit_dom_node_has_child_nodes (node)) {
						GError *error = 0;
						webkit_dom_node_remove_child(node, webkit_dom_node_get_first_child (node), &error);
						if (error) {
              g_message ("Cannot remove child: %s\n", error->message);
							g_error_free (error);
						}
					}
					continue;
				}
				g_free (class_name);
			}
		}
    if (webkit_dom_node_has_child_nodes (node)) {
      WebKitDOMNode *ptr = webkit_dom_node_get_first_child (node);
			while (ptr) {
        subtree = g_array_append_val (subtree, ptr);
        ptr = webkit_dom_node_get_next_sibling (ptr);
      }
		}
	}
  g_array_free (subtree, TRUE);
}

static void
web_page_check_dom (CFMUWebExtension *extension, WebKitWebPage *web_page)
{
  const gchar *uri = webkit_web_page_get_uri(web_page);
  WebKitDOMDocument *document = webkit_web_page_get_dom_document(web_page);
  //print_dom(webkit_dom_document_get_document_element(document), 2);

  if (FALSE)
    g_message("DOM changed: %s\n", webkit_web_page_get_uri(web_page));
  if (!g_strcmp0(uri, "https://www.public.nm.eurocontrol.int/PUBPORTAL/gateway/spec/index.html")) {
    WebKitDOMElement *element = webkit_dom_document_get_element_by_id(document, "IFPUV_LAUNCH_AREA.FREE_TEXT_EDIT_LINK_LABEL");
    if (extension->dbus_connection)
      cfmu_web_extension_emit_dom_changed (extension, webkit_web_page_get_id(web_page), !!element, FALSE);
    return;
  }
  if (g_strrstr(uri, "_view_id=IFPUV_DETACHED_LIST")) {
    WebKitDOMElement *element1 = webkit_dom_document_get_element_by_id(document, "FREE_TEXT_EDITOR.FLIGHT_DATA_AREA.GENERAL_DATA_ENTRY.INTRODUCE_FLIGHT_PLAN_FIELD");
    WebKitDOMElement *element2 = webkit_dom_document_get_element_by_id(document, "FREE_TEXT_EDITOR.VALIDATION_RESULTS_AREA");
    WebKitDOMElement *element3 = webkit_dom_document_get_element_by_id(document, "FREE_TEXT_EDITOR.FLIGHT_DATA_AREA.VALIDATE_ACTION_LABEL");
    if (extension->dbus_connection)
      cfmu_web_extension_emit_dom_changed (extension, webkit_web_page_get_id(web_page), FALSE, element1 && element2 && element3);
    if (element2)
      web_page_parse_results(extension, WEBKIT_DOM_NODE(element2));
    return;
  }
  if (extension->dbus_connection)
    cfmu_web_extension_emit_dom_changed (extension, webkit_web_page_get_id(web_page), FALSE, FALSE);
}

typedef struct _PageRef PageRef;
struct _PageRef {
  WebKitWebPage    *web_page;
  CFMUWebExtension *extension;
};

static void
closure_deallocate_page_ref (gpointer data, GClosure *closure)
{
  g_free(data);
  if (FALSE)
    g_message("dispose DOM event listener closure %p data %p\n", closure, data);
}

static gboolean
web_page_dom_change (WebKitDOMElement *target, WebKitDOMEvent *e, PageRef *p)
{
  web_page_check_dom (p->extension, p->web_page);
  return TRUE;
}

static void
web_page_document_loaded (WebKitWebPage    *web_page,
                          CFMUWebExtension *extension)
{
  WebKitDOMDocument *document = 0;
  WebKitDOMElement *element = 0;
  PageRef *pref = 0;
  GClosure *cls = 0;

  if (FALSE)
    g_message("document loaded: %s\n", webkit_web_page_get_uri(web_page));
  document = webkit_web_page_get_dom_document(web_page);
  element = webkit_dom_document_get_document_element(document);
  pref = g_new0(PageRef, 1);
  pref->web_page = web_page;
  pref->extension = extension;
  cls = g_cclosure_new(G_CALLBACK(web_page_dom_change), pref, closure_deallocate_page_ref);
  webkit_dom_event_target_add_event_listener_with_closure(WEBKIT_DOM_EVENT_TARGET(element), "DOMSubtreeModified", cls, true);
  if (FALSE)
    g_message("create DOM event listener closure %p data %p\n", cls, pref);
}

static void
web_page_uri_changed (WebKitWebPage    *web_page,
                      GParamSpec       *param_spec,
                      CFMUWebExtension *extension)
{
  if (FALSE)
    g_message("uri_changed: %s\n", webkit_web_page_get_uri (web_page));
}

static void
cfmu_web_extension_emit_page_created (CFMUWebExtension *extension,
                                      guint64           page_id)
{
  GError *error = NULL;

  g_dbus_connection_emit_signal (extension->dbus_connection,
                                 NULL,
                                 CFMU_WEB_EXTENSION_OBJECT_PATH,
                                 CFMU_WEB_EXTENSION_INTERFACE,
                                 "PageCreated",
                                 g_variant_new ("(t)", page_id),
                                 &error);
  if (error) {
    g_warning ("Error emitting signal PageCreated: %s\n", error->message);
    g_error_free (error);
  }
}

static void
cfmu_web_extension_emit_page_created_signals_pending (CFMUWebExtension *extension)
{
  guint i;

  if (!extension->page_created_signals_pending)
    return;

  for (i = 0; i < extension->page_created_signals_pending->len; i++) {
    guint64 page_id;
    WebKitWebPage *web_page;

    page_id = g_array_index (extension->page_created_signals_pending, guint64, i);
    cfmu_web_extension_emit_page_created (extension, page_id);

    web_page = webkit_web_extension_get_page (extension->extension, page_id);
    if (web_page)
      web_page_check_dom (extension, web_page);
  }

  g_array_free (extension->page_created_signals_pending, TRUE);
  extension->page_created_signals_pending = NULL;
}

static void
cfmu_web_extension_queue_page_created_signal_emission (CFMUWebExtension *extension,
                                                       guint64           page_id)
{
  if (!extension->page_created_signals_pending)
    extension->page_created_signals_pending = g_array_new (FALSE, FALSE, sizeof (guint64));
  extension->page_created_signals_pending = g_array_append_val (extension->page_created_signals_pending, page_id);
}

static void
cfmu_web_extension_page_created_cb (CFMUWebExtension *extension,
                                    WebKitWebPage    *web_page)
{
  guint64 page_id;

  page_id = webkit_web_page_get_id (web_page);
  if (extension->dbus_connection)
    cfmu_web_extension_emit_page_created (extension, page_id);
  else
    cfmu_web_extension_queue_page_created_signal_emission (extension, page_id);

  g_signal_connect (web_page, "send-request",
                    G_CALLBACK (web_page_send_request),
                    extension);
  g_signal_connect (web_page, "document-loaded",
                    G_CALLBACK (web_page_document_loaded),
                    extension);
  g_signal_connect (web_page, "notify::uri",
                    G_CALLBACK (web_page_uri_changed),
                    extension);
  g_signal_connect (web_page, "console-message-sent",
                    G_CALLBACK (web_page_console_message),
                    extension);
}

static WebKitWebPage *
get_webkit_web_page_or_return_dbus_error (GDBusMethodInvocation *invocation,
                                          WebKitWebExtension    *web_extension,
                                          guint64                page_id)
{
  WebKitWebPage *web_page = webkit_web_extension_get_page (web_extension, page_id);
  if (!web_page) {
    g_dbus_method_invocation_return_error (invocation, G_DBUS_ERROR, G_DBUS_ERROR_INVALID_ARGS,
                                           "Invalid page ID: %"G_GUINT64_FORMAT, page_id);
  }
  return web_page;
}

static void
handle_method_call (GDBusConnection       *connection,
                    const char            *sender,
                    const char            *object_path,
                    const char            *interface_name,
                    const char            *method_name,
                    GVariant              *parameters,
                    GDBusMethodInvocation *invocation,
                    gpointer               user_data)
{
  CFMUWebExtension *extension = CFMU_WEB_EXTENSION (user_data);

  if (g_strcmp0 (interface_name, CFMU_WEB_EXTENSION_INTERFACE) != 0)
    return;

  if (g_strcmp0 (method_name, "LaunchValidate") == 0) {
    WebKitWebPage *web_page;
    WebKitDOMDocument *document;
    WebKitDOMElement *element;
    guint64 page_id;
    JSGlobalContextRef jsContext;
    JSStringRef script;

    g_variant_get (parameters, "(t)", &page_id);
    web_page = get_webkit_web_page_or_return_dbus_error (invocation, extension->extension, page_id);
    if (!web_page) {
      g_dbus_method_invocation_return_error (invocation, G_IO_ERROR, G_IO_ERROR_NOT_FOUND, "page ID %lu not found", page_id);
      return;
    }
    document = webkit_web_page_get_dom_document(web_page);
    element = webkit_dom_document_get_element_by_id(document, "IFPUV_LAUNCH_AREA.FREE_TEXT_EDIT_LINK_LABEL");
    if (!element) {
      g_dbus_method_invocation_return_error_literal (invocation, G_IO_ERROR, G_IO_ERROR_NOT_FOUND, "IFPUV_LAUNCH_AREA.FREE_TEXT_EDIT_LINK_LABEL not found");
      return;
    }
    jsContext = webkit_frame_get_javascript_context_for_script_world (webkit_web_page_get_main_frame(web_page), webkit_script_world_get_default());
    //jsContext = webkit_frame_get_javascript_global_context (webkit_web_page_get_main_frame(web_page));
    //script = JSStringCreateWithUTF8CString("document.getElementById(\"IFPUV_LAUNCH_AREA.FREE_TEXT_EDIT_LINK_LABEL\").click(); console.log(\"click IFPUV\");");
    script = JSStringCreateWithUTF8CString("document.getElementById(\"IFPUV_LAUNCH_AREA.FREE_TEXT_EDIT_LINK_LABEL\").click();");
    JSEvaluateScript(jsContext, script, 0, 0, 0, 0);
    g_dbus_method_invocation_return_value (invocation, 0);
    return;
  }
  if (g_strcmp0 (method_name, "Validate") == 0) {
    WebKitWebPage *web_page;
    WebKitDOMDocument *document;
    WebKitDOMElement *element;
    guint64 page_id;
    char *fpl = NULL;
    JSGlobalContextRef jsContext;
    JSStringRef script;

    g_variant_get (parameters, "(ts)", &page_id, &fpl);
    web_page = get_webkit_web_page_or_return_dbus_error (invocation, extension->extension, page_id);
    if (!web_page) {
      g_dbus_method_invocation_return_error (invocation, G_IO_ERROR, G_IO_ERROR_NOT_FOUND, "page ID %lu not found", page_id);
      return;
    }
    if (extension->validate_invocation) {
      g_dbus_method_invocation_return_error (invocation, G_IO_ERROR, G_IO_ERROR_BUSY, "validation in progress");
      return;
    }
    if (!fpl || !*fpl) {
      g_dbus_method_invocation_return_error (invocation, G_IO_ERROR, G_IO_ERROR_INVALID_DATA, "flight plan empty");
      return;
    }

    document = webkit_web_page_get_dom_document (web_page);
    zap_errors (document);
    element = webkit_dom_document_get_element_by_id(document, "FREE_TEXT_EDITOR.FLIGHT_DATA_AREA.GENERAL_DATA_ENTRY.INTRODUCE_FLIGHT_PLAN_FIELD");
		if (!element || !WEBKIT_DOM_IS_HTML_TEXT_AREA_ELEMENT(element)) {
			g_dbus_method_invocation_return_error (invocation, G_IO_ERROR, G_IO_ERROR_FAILED, "FREE_TEXT_EDITOR.FLIGHT_DATA_AREA.GENERAL_DATA_ENTRY.INTRODUCE_FLIGHT_PLAN_FIELD not found");
      return;
		}
		webkit_dom_html_text_area_element_set_value(WEBKIT_DOM_HTML_TEXT_AREA_ELEMENT(element), fpl);
		element = webkit_dom_document_get_element_by_id(document, "FREE_TEXT_EDITOR.FLIGHT_DATA_AREA.VALIDATE_ACTION_LABEL");
		if (!element) {
			g_dbus_method_invocation_return_error (invocation, G_IO_ERROR, G_IO_ERROR_FAILED, "FREE_TEXT_EDITOR.FLIGHT_DATA_AREA.VALIDATE_ACTION_LABEL not found");
      return;
		}
    jsContext = webkit_frame_get_javascript_context_for_script_world(webkit_web_page_get_main_frame(web_page), webkit_script_world_get_default());
    script = JSStringCreateWithUTF8CString("document.getElementById(\"FREE_TEXT_EDITOR.FLIGHT_DATA_AREA.VALIDATE_ACTION_LABEL\").click();");
    JSEvaluateScript(jsContext, script, 0, 0, 0, 0);
    extension->validate_invocation = invocation;
    return;
  }
}

static const GDBusInterfaceVTable interface_vtable = {
  handle_method_call,
  NULL,
  NULL
};

static void
cfmu_web_extension_dispose (GObject *object)
{
  CFMUWebExtension *extension = CFMU_WEB_EXTENSION (object);

  if (extension->validate_invocation) {
    GVariantBuilder *builder = g_variant_builder_new (G_VARIANT_TYPE ("(as)"));
    g_variant_builder_add (builder, "s", "WEBPAGE_DISPOSE");
    g_dbus_method_invocation_return_value (extension->validate_invocation, g_variant_new ("(as)", builder));
    g_variant_builder_unref (builder);
    extension->validate_invocation = 0;
  }

  if (extension->page_created_signals_pending) {
    g_array_free (extension->page_created_signals_pending, TRUE);
    extension->page_created_signals_pending = NULL;
  }

  g_clear_object (&extension->cancellable);
  g_clear_object (&extension->dbus_connection);

  g_clear_object (&extension->extension);

  G_OBJECT_CLASS (cfmu_web_extension_parent_class)->dispose (object);
}

static void
cfmu_web_extension_class_init (CFMUWebExtensionClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);

  object_class->dispose = cfmu_web_extension_dispose;
}

static void
cfmu_web_extension_init (CFMUWebExtension *extension)
{
}

static gpointer
cfmu_web_extension_create_instance (gpointer data)
{
  return g_object_new (CFMU_TYPE_WEB_EXTENSION, NULL);
}

CFMUWebExtension *
cfmu_web_extension_get (void)
{
  static GOnce once_init = G_ONCE_INIT;
  return CFMU_WEB_EXTENSION (g_once (&once_init, cfmu_web_extension_create_instance, NULL));
}

static void
dbus_connection_created_cb (GObject          *source_object,
                            GAsyncResult     *result,
                            CFMUWebExtension *extension)
{
  static GDBusNodeInfo *introspection_data = NULL;
  GDBusConnection *connection;
  guint registration_id;
  GError *error = NULL;

  if (!introspection_data)
    introspection_data = g_dbus_node_info_new_for_xml (introspection_xml, NULL);

  connection = g_dbus_connection_new_for_address_finish (result, &error);
  if (error) {
    g_warning ("Failed to connect to UI process: %s", error->message);
    g_error_free (error);
    return;
  }

  registration_id =
    g_dbus_connection_register_object (connection,
                                       CFMU_WEB_EXTENSION_OBJECT_PATH,
                                       introspection_data->interfaces[0],
                                       &interface_vtable,
                                       extension,
                                       NULL,
                                       &error);
  if (!registration_id) {
    g_warning ("Failed to register web extension object: %s\n", error->message);
    g_error_free (error);
    g_object_unref (connection);
    return;
  }

  extension->dbus_connection = connection;
  cfmu_web_extension_emit_page_created_signals_pending (extension);
}

void
cfmu_web_extension_initialize (CFMUWebExtension   *extension,
                               WebKitWebExtension *wk_extension,
                               const char         *server_address)
{
  GDBusAuthObserver *observer;

  g_return_if_fail (CFMU_IS_WEB_EXTENSION (extension));

  if (extension->initialized)
    return;

  extension->initialized = TRUE;

  extension->extension = g_object_ref (wk_extension);

  g_signal_connect_swapped (extension->extension, "page-created",
                            G_CALLBACK (cfmu_web_extension_page_created_cb),
                            extension);

  extension->cancellable = g_cancellable_new ();

  observer = g_dbus_auth_observer_new ();

  g_dbus_connection_new_for_address (server_address,
                                     G_DBUS_CONNECTION_FLAGS_AUTHENTICATION_CLIENT,
                                     observer,
                                     extension->cancellable,
                                     (GAsyncReadyCallback)dbus_connection_created_cb,
                                     extension);
  g_object_unref (observer);
}
