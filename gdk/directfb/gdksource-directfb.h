/* GDK - The GIMP Drawing Kit
 * Copyright © 2013 EchoStar Corporation
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library. If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef GDK_DIRECTFB_SOURCE_
#define GDK_DIRECTFB_SOURCE_

#include "gdkprivate-directfb.h"
#include "gdkdisplay-directfb.h"

G_BEGIN_DECLS

G_GNUC_INTERNAL
G_GNUC_WARN_UNUSED_RESULT
GSource * gdk_directfb_source_new (GdkDisplay *display);

G_GNUC_INTERNAL
guint gdk_directfb_source_get_next_serial (GSource *source);

G_END_DECLS

#endif /* GDK_DIRECTFB_SOURCE_ */
