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

#include "config.h"

#include "gdkcursor-directfb.h"
#include "gdkdisplay-directfb.h"
#include "gdkprivate-directfb.h"
#include "gdkscreen-directfb.h"
#include "gdkwindow-directfb.h"
#include <cairo-directfb.h>

struct GdkDirectfbCursorData_
{
  IDirectFBSurface *surface;
  guint hot_x;
  guint hot_y;
  guint delay;
};

G_DEFINE_TYPE (GdkDirectfbCursor, gdk_directfb_cursor, GDK_TYPE_CURSOR)

static void
gdk_directfb_cursor_init (GdkDirectfbCursor *cursor)
{
}

static void
gdk_directfb_cursor_finalize (GObject *object)
{
  GdkDirectfbCursor *directfb_cursor = GDK_DIRECTFB_CURSOR (object);
  gsize i;

  if (directfb_cursor->id > 0)
    g_source_remove (directfb_cursor->id);

  if (directfb_cursor->layer)
    (void) directfb_cursor->layer->Release (directfb_cursor->layer);

  for (i = 0; i < directfb_cursor->n; ++i)
    {
      IDirectFBSurface *surface = directfb_cursor->cursors[i].surface;

      if (G_LIKELY (surface))
	(void) surface->Release (surface);
    }

  g_free (directfb_cursor->cursors);

  G_OBJECT_CLASS (gdk_directfb_cursor_parent_class)->finalize (object);
}

static cairo_surface_t *
gdk_directfb_cursor_get_surface (GdkCursor *cursor,
				 gdouble   *x_hot,
				 gdouble   *y_hot)
{
  GdkDirectfbCursor *directfb_cursor = GDK_DIRECTFB_CURSOR (cursor);
  GdkDisplay *display;
  IDirectFB *dfb;

  if (G_UNLIKELY (0 == directfb_cursor->n))
    return NULL;

  if (G_LIKELY (x_hot))
    *x_hot = (gdouble) directfb_cursor->cursors[0].hot_x;

  if (G_LIKELY (y_hot))
    *y_hot = (gdouble) directfb_cursor->cursors[0].hot_y;

  display = gdk_cursor_get_display (cursor);
  dfb = gdk_directfb_display_get_context (display);

  return cairo_directfb_surface_create (dfb,
					directfb_cursor->cursors[0].surface);
}

static void
gdk_directfb_cursor_class_init (GdkDirectfbCursorClass *klass)
{
  GdkCursorClass *cursor_class = GDK_CURSOR_CLASS (klass);
  GObjectClass *object_class = G_OBJECT_CLASS (klass);

  object_class->finalize = gdk_directfb_cursor_finalize;

  cursor_class->get_surface = gdk_directfb_cursor_get_surface;
}

/**
 * gdk_directfb_cursor_new:
 * @display: The #GdkDisplay for which the cursor will be created
 * @cursor_type: The #GdkCursorType of the new cursor
 * @surface: The #IDirectFBSurface containing the cursor image
 * @x: The X-coordinate of the cursor 'hotspot'
 * @y: The Y-coordinate of the cursor 'hotspot'
 *
 * Creates a new cursor from a DirectFB surface.
 *
 * Returns: (transfer full): A new #GdkCursor
 *
 * Since: 3.10
 */
GdkCursor *
gdk_directfb_cursor_new (GdkDisplay       *display,
			 GdkCursorType     cursor_type,
			 IDirectFBSurface *surface,
			 gint              x,
			 gint              y)
{
  GdkCursor *cursor;

  cursor = g_object_new (GDK_TYPE_DIRECTFB_CURSOR,
			 "cursor-type", cursor_type,
			 "display", display,
			 NULL);

  cursor = gdk_directfb_animated_cursor_new (display, cursor_type, 1);
  gdk_directfb_animated_cursor_add (cursor, surface, x, y, 0, 0);

  return cursor;
}

/**
 * gdk_directfb_blank_cursor_new:
 * @display: The #GdkDisplay for which the cursor will be created
 *
 * Creates a new blank cursor.
 *
 * Returns: A new #GdkCursor
 *
 * Since: 3.10
 */
GdkCursor *
gdk_directfb_blank_cursor_new (GdkDisplay *display)
{
  GdkCursor *cursor;
  GdkDirectfbCursor *directfb_cursor;

  cursor = g_object_new (GDK_TYPE_DIRECTFB_CURSOR,
			 "cursor-type", GDK_BLANK_CURSOR,
			 "display", display,
			 NULL);

  directfb_cursor = GDK_DIRECTFB_CURSOR (cursor);
  directfb_cursor->n = 0;
  directfb_cursor->i = 0;
  directfb_cursor->id = 0;
  directfb_cursor->cursors = NULL;

  return cursor;
}

/**
 * gdk_directfb_animated_cursor_new:
 * @display: The #GdkDisplay for which the cursor will be created
 * @cursor_type: The #GdkCursorType of the new cursor
 *
 * Creates a new animated cursor. Images must be added to the newly created
 * cursor using gdk_directfb_animated_cursor_add().
 *
 * Returns: A new #GdkCursor
 *
 * Since: 3.10
 */
GdkCursor *
gdk_directfb_animated_cursor_new (GdkDisplay   *display,
				  GdkCursorType cursor_type,
				  gsize         n)
{
  GdkCursor *cursor;
  GdkDirectfbCursor *directfb_cursor;

  cursor = g_object_new (GDK_TYPE_DIRECTFB_CURSOR,
			 "cursor-type", cursor_type,
			 "display", display,
			 NULL);

  directfb_cursor = GDK_DIRECTFB_CURSOR (cursor);
  directfb_cursor->n = n;
  directfb_cursor->i = 0;
  directfb_cursor->id = 0;
  directfb_cursor->cursors = g_new0 (GdkDirectfbCursorData, n);

  return cursor;
}

/**
 * gdk_directfb_animated_cursor_add:
 * @cursor: A #GdkCursor object
 * @surface: The #IDirectFBSurface containing the cursor image
 * @x: The X-coordinate of the cursor 'hotspot'
 * @y: The Y-coordinate of the cursor 'hotspot'
 * @delay: The delay (in milliseconds) until the next frame
 * @index: The image index
 *
 * Adds an image to an animated cursor at the specified @index.
 *
 * Since: 3.10
 */
void
gdk_directfb_animated_cursor_add (GdkCursor        *cursor,
				  IDirectFBSurface *surface,
				  gint              x,
				  gint              y,
				  guint             delay,
				  gsize             index)
{
  GdkDirectfbCursor *directfb_cursor;
  GdkDirectfbCursorData *data;

  directfb_cursor = GDK_DIRECTFB_CURSOR (cursor);
  data = &directfb_cursor->cursors[index];
  (void) surface->AddRef (surface);
  data->surface = surface;
  data->hot_x = x;
  data->hot_y = y;
  data->delay = delay;
}

/**
 * gdk_directfb_cursor_update:
 * @user_data: A #GdkDirectfbCursor object
 *
 * Updates the currently displayed cursor image. If @user_data is an animated
 * cursor then a timeout is set to update the next frame.
 *
 * This function is suitable as a callback for g_timeout_add().
 *
 * Returns: FALSE (the timeout is removed)
 *
 * Since: 3.10
 */
static gboolean
gdk_directfb_cursor_update (void *user_data)
{
  GdkDirectfbCursor *directfb_cursor = user_data;
  GdkDirectfbCursorData *data;

  data = &directfb_cursor->cursors[directfb_cursor->i];
  (void) directfb_cursor->layer->SetCursorShape (directfb_cursor->layer,
						 data->surface,
						 data->hot_x, data->hot_y);

  (void) directfb_cursor->layer->SetCursorOpacity (directfb_cursor->layer, 255);

  if (directfb_cursor->n > 1)
    {
      directfb_cursor->id = g_timeout_add (data->delay,
					   gdk_directfb_cursor_update,
					   directfb_cursor);

      if (++directfb_cursor->i == directfb_cursor->n)
	directfb_cursor->i = 0;
    }

  return FALSE;
}

/**
 * gdk_directfb_cursor_activate:
 * @cursor: A #GdkCursor object
 * @screen: A #GdkScreen object
 *
 * Activate a @cursor for the specified @screen.
 *
 * Since: 3.10
 */
void
gdk_directfb_cursor_activate (GdkCursor *cursor,
			      GdkScreen *screen)
{
  GdkDirectfbCursor *directfb_cursor;
  IDirectFBDisplayLayer *layer;

  g_return_if_fail (GDK_IS_DIRECTFB_CURSOR (cursor));

  directfb_cursor = GDK_DIRECTFB_CURSOR (cursor);

  layer = gdk_directfb_screen_get_display_layer (screen);
  if (G_LIKELY (directfb_cursor->n > 0))
    {
      layer->AddRef (layer);
      directfb_cursor->layer = layer;
      (void) gdk_directfb_cursor_update (directfb_cursor);
    }
  else
    (void) layer->SetCursorOpacity (layer, 0);
}

/**
 * gdk_directfb_cursor_deactivate:
 * @cursor: A #GdkCursor object
 *
 * Deactivate the specified cursor. If @cursor is an animated cursor then any
 * pending image updates will be canceled.
 *
 * Since: 3.10
 */
void
gdk_directfb_cursor_deactivate (GdkCursor *cursor)
{
  GdkDirectfbCursor *directfb_cursor;

  g_return_if_fail (GDK_IS_DIRECTFB_CURSOR (cursor));

  directfb_cursor = GDK_DIRECTFB_CURSOR (cursor);

  if (directfb_cursor->id > 0)
    {
      g_source_remove (directfb_cursor->id);
      directfb_cursor->id = 0;
    }

  if (directfb_cursor->layer)
    {
      (void) directfb_cursor->layer->Release (directfb_cursor->layer);
      directfb_cursor->layer = NULL;
    }

  directfb_cursor->i = 0;
}
