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

#pragma once

#include <glib-object.h>
#include <webkit2/webkit-web-extension.h>

G_BEGIN_DECLS

#define CFMU_TYPE_WEB_EXTENSION (cfmu_web_extension_get_type())

G_DECLARE_FINAL_TYPE (CFMUWebExtension, cfmu_web_extension, CFMU, WEB_EXTENSION, GObject)

CFMUWebExtension *cfmu_web_extension_get            (void);
void              cfmu_web_extension_initialize     (CFMUWebExtension   *extension,
                                                     WebKitWebExtension *wk_extension,
                                                     const char         *server_address);


G_END_DECLS
