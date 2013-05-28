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

#ifndef GDK_DIRECTFB_CURSOR_H_
#define GDK_DIRECTFB_CURSOR_H_

#include "gdkcursorprivate.h"
#include <directfb.h>

G_BEGIN_DECLS

#define GDK_TYPE_DIRECTFB_CURSOR              (gdk_directfb_cursor_get_type ())
#define GDK_DIRECTFB_CURSOR(object)           (G_TYPE_CHECK_INSTANCE_CAST ((object), GDK_TYPE_DIRECTFB_CURSOR, GdkDirectfbCursor))
#define GDK_DIRECTFB_CURSOR_CLASS(klass)      (G_TYPE_CHECK_CLASS_CAST ((klass), GDK_TYPE_DIRECTFB_CURSOR, GdkDirectfbCursorClass))
#define GDK_IS_DIRECTFB_CURSOR(object)        (G_TYPE_CHECK_INSTANCE_TYPE ((object), GDK_TYPE_DIRECTFB_CURSOR))
#define GDK_IS_DIRECTFB_CURSOR_CLASS(klass)   (G_TYPE_CHECK_CLASS_TYPE ((klass), GDK_TYPE_DIRECTFB_CURSOR))
#define GDK_DIRECTFB_CURSOR_GET_CLASS(object) (G_TYPE_INSTANCE_GET_CLASS ((object), GDK_TYPE_DIRECTFB_CURSOR, GdkDirectfbCursorClass))

typedef struct GdkDirectfbCursor_ GdkDirectfbCursor;
typedef struct GdkDirectfbCursorClass_ GdkDirectfbCursorClass;
typedef struct GdkDirectfbCursorData_ GdkDirectfbCursorData;

struct GdkDirectfbCursor_
{
  GdkCursor cursor;
  guint id;
  gsize n, i;
  GdkDirectfbCursorData *cursors;
  IDirectFBDisplayLayer *layer;
};

struct GdkDirectfbCursorClass_
{
  GdkCursorClass cursor_class;
};

G_GNUC_INTERNAL
GType gdk_directfb_cursor_get_type (void) G_GNUC_CONST;

G_GNUC_INTERNAL
GdkCursor * gdk_directfb_cursor_new (GdkDisplay       *display,
                                     GdkCursorType     cursor_type,
                                     IDirectFBSurface *surface,
                                     gint              x,
                                     gint              y);

G_GNUC_INTERNAL
GdkCursor * gdk_directfb_blank_cursor_new (GdkDisplay *display);

G_GNUC_INTERNAL
GdkCursor * gdk_directfb_animated_cursor_new (GdkDisplay   *display,
                                              GdkCursorType cursor_type,
                                              gsize         n);

G_GNUC_INTERNAL
void gdk_directfb_animated_cursor_add (GdkCursor        *cursor,
                                       IDirectFBSurface *surface,
                                       gint              x,
                                       gint              y,
                                       guint             delay,
                                       gsize             i);

G_GNUC_INTERNAL
void gdk_directfb_cursor_activate (GdkCursor *cursor,
                                   GdkScreen *screen);

G_GNUC_INTERNAL
void gdk_directfb_cursor_deactivate (GdkCursor *cursor);

G_END_DECLS

#endif /* GDK_DIRECTFB_CURSOR_H_ */
