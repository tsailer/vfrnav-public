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

#pragma once

#include <gio/gio.h>

G_BEGIN_DECLS

#define CFMU_TYPE_WEB_EXTENSION_PROXY (cfmu_web_extension_proxy_get_type ())

G_DECLARE_FINAL_TYPE (CFMUWebExtensionProxy, cfmu_web_extension_proxy, CFMU, WEB_EXTENSION_PROXY, GObject)

CFMUWebExtensionProxy *cfmu_web_extension_proxy_new                                       (GDBusConnection       *connection);
void                   cfmu_web_extension_proxy_launch_validate                           (CFMUWebExtensionProxy *web_extension,
                                                                                           guint64                page_id);
void                   cfmu_web_extension_proxy_validate                                  (CFMUWebExtensionProxy *web_extension,
                                                                                           guint64                page_id,
                                                                                           const char            *fpl,
                                                                                           GCancellable          *cancellable,
                                                                                           GAsyncReadyCallback    callback,
                                                                                           gpointer               user_data);
gchar                **cfmu_web_extension_proxy_validate_finish                           (CFMUWebExtensionProxy *web_extension,
                                                                                           GAsyncResult          *result,
                                                                                           GError               **error);
G_END_DECLS
