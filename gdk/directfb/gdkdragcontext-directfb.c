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

#include "gdkdragcontext-directfb.h"

G_DEFINE_TYPE (GdkDirectfbDragContext, gdk_directfb_drag_context, GDK_TYPE_DRAG_CONTEXT)

static void
gdk_directfb_drag_context_init (GdkDirectfbDragContext *drag_context)
{
}

static GdkWindow *
gdk_directfb_drag_context_find_window (GdkDragContext  *context,
				       GdkWindow       *drag_window,
				       GdkScreen       *screen,
				       gint             x_root,
				       gint             y_root,
				       GdkDragProtocol *protocol)
{
  return NULL;
}

static gboolean
gdk_directfb_drag_context_drag_motion (GdkDragContext *context,
				       GdkWindow      *dest_window,
				       GdkDragProtocol protocol,
				       gint            x_root,
				       gint            y_root,
				       GdkDragAction   suggested_action,
				       GdkDragAction   possible_actions,
				       guint32         time)
{
  return FALSE;
}

static void
gdk_directfb_drag_context_drag_abort (GdkDragContext *context,
				      guint32         time)
{
}

static void
gdk_directfb_drag_context_drag_drop (GdkDragContext *context,
				     guint32         time)
{
}

static void
gdk_directfb_drag_context_drag_status (GdkDragContext *context,
				       GdkDragAction   action,
				       guint32         time_)
{
}

static void
gdk_directfb_drag_context_drop_reply (GdkDragContext *context,
				      gboolean        accepted,
				      guint32         time_)
{
}

static void
gdk_directfb_drag_context_drop_finish (GdkDragContext *context,
				       gboolean        success,
				       guint32         time)
{
}

static gboolean
gdk_directfb_drag_context_drop_status (GdkDragContext *context)
{
  return FALSE;
}

static GdkAtom
gdk_directfb_drag_context_get_selection (GdkDragContext *context)
{
    return GDK_NONE;
}

static void
gdk_directfb_drag_context_class_init (GdkDirectfbDragContextClass *klass)
{
  GdkDragContextClass *context_class = GDK_DRAG_CONTEXT_CLASS (klass);

  context_class->find_window = gdk_directfb_drag_context_find_window;
  context_class->drag_motion = gdk_directfb_drag_context_drag_motion;
  context_class->drag_abort = gdk_directfb_drag_context_drag_abort;
  context_class->drag_drop = gdk_directfb_drag_context_drag_drop;
  context_class->drag_status = gdk_directfb_drag_context_drag_status;
  context_class->drop_reply = gdk_directfb_drag_context_drop_reply;
  context_class->drop_finish = gdk_directfb_drag_context_drop_finish;
  context_class->drop_status = gdk_directfb_drag_context_drop_status;
  context_class->get_selection = gdk_directfb_drag_context_get_selection;
}

/**
 * gdk_directfb_drag_context_new:
 * @source_window: The source of the drag
 * @device: The device that started the drag
 * @targets: The targets
 *
 * Create a new drag context.
 *
 * Returns: A new drag context for the specified @device
 */
GdkDragContext *
gdk_directfb_drag_context_new (GdkWindow *source_window,
			       GdkDevice *device,
			       GList     *targets)
{
  GdkDragContext *drag_context;

  drag_context = g_object_new (GDK_TYPE_DIRECTFB_DRAG_CONTEXT, NULL);

  if (source_window)
    drag_context->source_window = g_object_ref (source_window);

  gdk_drag_context_set_device (drag_context, device);

  return drag_context;
}
