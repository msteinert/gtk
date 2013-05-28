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

#ifndef GDK_WINDOW_DIRECTFB_
#define GDK_WINDOW_DIRECTFB_

#include "gdk/directfb/gdkprivate-directfb.h"
#include "gdk/gdkwindowimpl.h"
#include "gdkdirectfbwindow.h"
#include <directfb.h>

G_BEGIN_DECLS

struct GdkDirectfbWindow_ {
  GdkWindow parent;
};

struct GdkDirectfbWindowClass_ {
  GdkWindowClass parent_class;
};

#define GDK_TYPE_WINDOW_IMPL_DIRECTFB              (gdk_window_impl_directfb_get_type ())
#define GDK_WINDOW_IMPL_DIRECTFB(object)           (G_TYPE_CHECK_INSTANCE_CAST ((object), GDK_TYPE_WINDOW_IMPL_DIRECTFB, GdkWindowImplDirectfb))
#define GDK_WINDOW_IMPL_DIRECTFB_CLASS(klass)      (G_TYPE_CHECK_CLASS_CAST ((klass), GDK_TYPE_WINDOW_IMPL_DIRECTFB, GdkWindowImplDirectfbClass))
#define GDK_IS_WINDOW_IMPL_DIRECTFB(object)        (G_TYPE_CHECK_INSTANCE_TYPE ((object), GDK_TYPE_WINDOW_IMPL_DIRECTFB))
#define GDK_IS_WINDOW_IMPL_DIRECTFB_CLASS(klass)   (G_TYPE_CHECK_CLASS_TYPE ((klass), GDK_TYPE_WINDOW_IMPL_DIRECTFB))
#define GDK_WINDOW_IMPL_DIRECTFB_GET_CLASS(object) (G_TYPE_INSTANCE_GET_CLASS ((object), GDK_TYPE_WINDOW_IMPL_DIRECTFB, GdkWindowImplDirectfbClass))

typedef struct GdkWindowImplDirectfb_ GdkWindowImplDirectfb;
typedef struct GdkWindowImplDirectfbClass_ GdkWindowImplDirectfbClass;

struct GdkWindowImplDirectfb_
{
  GdkWindowImpl parent_instance;
  IDirectFBWindow *window;
  GdkScreen *screen;
  cairo_surface_t *surface;
  cairo_region_t *area;
  cairo_region_t *damage;
  guint8 opacity;
  gboolean fullscreen;
  cairo_rectangle_int_t original;
  GdkEventMask event_mask;
};

struct GdkWindowImplDirectfbClass_
{
  GdkWindowImplClass parent_class;
};

G_GNUC_INTERNAL
GType gdk_window_impl_directfb_get_type (void) G_GNUC_CONST;

G_GNUC_INTERNAL
G_GNUC_WARN_UNUSED_RESULT
GdkWindow * gdk_directfb_root_window_new (GdkScreen *screen);

G_GNUC_INTERNAL
void gdk_directfb_window_impl_new (GdkDisplay  *display,
                                   GdkWindow   *window,
                                   GdkWindow   *real_parent,
                                   GdkScreen   *screen,
                                   GdkEventMask event_mask);

G_END_DECLS

#endif /* GDK_WINDOW_DIRECTFB_ */
