/* -*- Mode: C; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*- */
/*
 *  Copyright © 2014 Igalia S.L.
 *  Copyright 2016 autorouter.eu
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

#include <config.h>
#include "cfmu-web-extension-proxy.h"

#include "cfmu-web-extension-names.h"

struct _CFMUWebExtensionProxy {
  GObject parent_instance;

  GCancellable *cancellable;
  GDBusProxy *proxy;
  GDBusConnection *connection;

  guint page_created_signal_id;
  guint dom_changed_signal_id;
};

enum {
  PAGE_CREATED,
  DOM_CHANGED,

  LAST_SIGNAL
};

static guint signals[LAST_SIGNAL];

G_DEFINE_TYPE (CFMUWebExtensionProxy, cfmu_web_extension_proxy, G_TYPE_OBJECT)

static void
cfmu_web_extension_proxy_dispose (GObject *object)
{
  CFMUWebExtensionProxy *web_extension = CFMU_WEB_EXTENSION_PROXY (object);

  if (web_extension->page_created_signal_id > 0) {
    g_dbus_connection_signal_unsubscribe (web_extension->connection,
                                          web_extension->page_created_signal_id);
    web_extension->page_created_signal_id = 0;
  }

  if (web_extension->dom_changed_signal_id > 0) {
    g_dbus_connection_signal_unsubscribe (web_extension->connection,
                                          web_extension->dom_changed_signal_id);
    web_extension->dom_changed_signal_id = 0;
  }

  g_clear_object (&web_extension->cancellable);
  g_clear_object (&web_extension->proxy);
  g_clear_object (&web_extension->connection);

  G_OBJECT_CLASS (cfmu_web_extension_proxy_parent_class)->dispose (object);
}

static void
cfmu_web_extension_proxy_init (CFMUWebExtensionProxy *web_extension)
{
}

static void
cfmu_web_extension_proxy_class_init (CFMUWebExtensionProxyClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);

  object_class->dispose = cfmu_web_extension_proxy_dispose;

  /**
   * CFMUWebExtensionProxy::page-created:
   * @web_extension: the #CFMUWebExtensionProxy
   * @page_id: the identifier of the web page created
   *
   * Emitted when a web page is created in the web process.
   */
  signals[PAGE_CREATED] =
    g_signal_new ("page-created",
                  CFMU_TYPE_WEB_EXTENSION_PROXY,
                  G_SIGNAL_RUN_FIRST,
                  0, NULL, NULL, NULL,
                  G_TYPE_NONE, 1,
                  G_TYPE_UINT64);
  signals[DOM_CHANGED] =
    g_signal_new ("dom-changed",
                  CFMU_TYPE_WEB_EXTENSION_PROXY,
                  G_SIGNAL_RUN_FIRST,
                  0, NULL, NULL, NULL,
                  G_TYPE_NONE, 3,
                  G_TYPE_UINT64,
                  G_TYPE_BOOLEAN,
                  G_TYPE_BOOLEAN);
}

static void
web_extension_page_created (GDBusConnection       *connection,
                            const char            *sender_name,
                            const char            *object_path,
                            const char            *interface_name,
                            const char            *signal_name,
                            GVariant              *parameters,
                            CFMUWebExtensionProxy *web_extension)
{
  guint64 page_id;
  g_variant_get (parameters, "(t)", &page_id);
  g_signal_emit (web_extension, signals[PAGE_CREATED], 0, page_id);
}

static void
web_extension_dom_changed (GDBusConnection       *connection,
                           const char            *sender_name,
                           const char            *object_path,
                           const char            *interface_name,
                           const char            *signal_name,
                           GVariant              *parameters,
                           CFMUWebExtensionProxy *web_extension)
{
  guint64 page_id;
  gboolean portaldom, validatedom;
  g_variant_get (parameters, "(tbb)", &page_id, &portaldom, &validatedom);
  g_signal_emit (web_extension, signals[DOM_CHANGED], 0, page_id, portaldom, validatedom);
}

static void
web_extension_proxy_created_cb (GDBusProxy            *proxy,
                                GAsyncResult          *result,
                                CFMUWebExtensionProxy *web_extension)
{
  GError *error = NULL;

  web_extension->proxy = g_dbus_proxy_new_finish (result, &error);
  if (!web_extension->proxy) {
    g_warning ("Error creating web extension proxy: %s", error->message);
    g_error_free (error);

    /* Attempt to trigger connection_closed_cb, which will destroy us, and ensure that
     * that CFMUEmbedShell will remove us from its extensions list.
     */
    g_dbus_connection_close (web_extension->connection,
                             web_extension->cancellable,
                             NULL /* GAsyncReadyCallback */,
                             NULL);
    return;
  }

  web_extension->page_created_signal_id =
    g_dbus_connection_signal_subscribe (web_extension->connection,
                                        NULL,
                                        CFMU_WEB_EXTENSION_INTERFACE,
                                        "PageCreated",
                                        CFMU_WEB_EXTENSION_OBJECT_PATH,
                                        NULL,
                                        G_DBUS_SIGNAL_FLAGS_NONE,
                                        (GDBusSignalCallback)web_extension_page_created,
                                        web_extension,
                                        NULL);

  web_extension->dom_changed_signal_id =
    g_dbus_connection_signal_subscribe (web_extension->connection,
                                        NULL,
                                        CFMU_WEB_EXTENSION_INTERFACE,
                                        "DomChanged",
                                        CFMU_WEB_EXTENSION_OBJECT_PATH,
                                        NULL,
                                        G_DBUS_SIGNAL_FLAGS_NONE,
                                        (GDBusSignalCallback)web_extension_dom_changed,
                                        web_extension,
                                        NULL);
}

static void
connection_closed_cb (GDBusConnection       *connection,
                      gboolean               remote_peer_vanished,
                      GError                *error,
                      CFMUWebExtensionProxy *web_extension)
{
  if (error) {
    if (!remote_peer_vanished)
      g_warning ("Unexpectedly lost connection to web extension: %s", error->message);
  }

  g_object_unref (web_extension);
}

CFMUWebExtensionProxy *
cfmu_web_extension_proxy_new (GDBusConnection *connection)
{
  CFMUWebExtensionProxy *web_extension;

  g_return_val_if_fail (G_IS_DBUS_CONNECTION (connection), NULL);

  web_extension = g_object_new (CFMU_TYPE_WEB_EXTENSION_PROXY, NULL);

  g_signal_connect (connection, "closed",
                    G_CALLBACK (connection_closed_cb), web_extension);

  web_extension->cancellable = g_cancellable_new ();
  web_extension->connection = g_object_ref (connection);

  g_dbus_proxy_new (connection,
                    G_DBUS_PROXY_FLAGS_DO_NOT_LOAD_PROPERTIES | G_DBUS_PROXY_FLAGS_DO_NOT_CONNECT_SIGNALS,
                    NULL,
                    NULL,
                    CFMU_WEB_EXTENSION_OBJECT_PATH,
                    CFMU_WEB_EXTENSION_INTERFACE,
                    web_extension->cancellable,
                    (GAsyncReadyCallback)web_extension_proxy_created_cb,
                    web_extension);

  return web_extension;
}

static void
validate_cb (GDBusProxy   *proxy,
             GAsyncResult *result,
             GTask        *task)
{
  GVariant *retval;
  GError *error = NULL;

  retval = g_dbus_proxy_call_finish (proxy, result, &error);
  if (!retval) {
    g_task_return_error (task, error);
  } else {
    gchar **errs;

    g_variant_get (retval, "(^as)", &errs);
    g_task_return_pointer (task, errs, (GDestroyNotify)g_strfreev);
    g_variant_unref (retval);
  }
  g_object_unref (task);
}

void
cfmu_web_extension_proxy_launch_validate (CFMUWebExtensionProxy *web_extension,
                                          guint64                page_id)
{
  if (!web_extension->proxy)
    return;

  g_dbus_proxy_call (web_extension->proxy,
                     "LaunchValidate",
                     g_variant_new ("(t)", page_id),
                     G_DBUS_CALL_FLAGS_NONE,
                     -1,
                     web_extension->cancellable,
                     NULL, NULL);
}

void
cfmu_web_extension_proxy_validate (CFMUWebExtensionProxy *web_extension,
                                   guint64                page_id,
                                   const char            *fpl,
                                   GCancellable          *cancellable,
                                   GAsyncReadyCallback    callback,
                                   gpointer               user_data)
{
  GTask *task;

  g_return_if_fail (CFMU_IS_WEB_EXTENSION_PROXY (web_extension));

  task = g_task_new (web_extension, cancellable, callback, user_data);

  if (web_extension->proxy) {
    g_dbus_proxy_call (web_extension->proxy,
                       "Validate",
                       g_variant_new ("(ts)", page_id, fpl),
                       G_DBUS_CALL_FLAGS_NONE,
                       -1,
                       web_extension->cancellable,
                       (GAsyncReadyCallback)validate_cb,
                       g_object_ref (task));
  } else {
    g_task_return_pointer (task, NULL, NULL);
  }

  g_object_unref (task);
}

gchar **
cfmu_web_extension_proxy_validate_finish (CFMUWebExtensionProxy *web_extension,
                                          GAsyncResult          *result,
                                          GError               **error)
{
  g_return_val_if_fail (g_task_is_valid (result, web_extension), FALSE);

  return g_task_propagate_pointer (G_TASK (result), error);
}
