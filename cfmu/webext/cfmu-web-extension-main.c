/* -*- Mode: C; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*- */
/*
 *  Copyright © 2012 Igalia S.L.
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

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wmissing-prototypes"

static CFMUWebExtension *extension = NULL;

G_MODULE_EXPORT void
webkit_web_extension_initialize_with_user_data (WebKitWebExtension *webkit_extension,
                                                GVariant           *user_data)
{
  const char *server_address;
  GError *error = NULL;

  g_variant_get (user_data, "(m&s)", &server_address);

  if (!server_address) {
    g_warning ("UI process did not start D-Bus server, giving up.");
    return;
  }

  //cfmu_debug_init ();

  extension = cfmu_web_extension_get ();

  cfmu_web_extension_initialize (extension,
                                 webkit_extension,
                                 server_address);
}

static void __attribute__((destructor))
cfmu_web_extension_shutdown (void)
{
  if (extension)
    g_object_unref (extension);
}

#pragma GCC diagnostic pop
