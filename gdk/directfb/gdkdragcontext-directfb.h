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

#ifndef GDK_DIRECTFB_DRAG_CONTEXT_
#define GDK_DIRECTFB_DRAG_CONTEXT_

#include "gdkdndprivate.h"

G_BEGIN_DECLS

#define GDK_TYPE_DIRECTFB_DRAG_CONTEXT              (gdk_directfb_drag_context_get_type ())
#define GDK_DIRECTFB_DRAG_CONTEXT(object)           (G_TYPE_CHECK_INSTANCE_CAST ((object), GDK_TYPE_DIRECTFB_DRAG_CONTEXT, GdkDirectfbDragContext))
#define GDK_DIRECTFB_DRAG_CONTEXT_CLASS(klass)      (G_TYPE_CHECK_CLASS_CAST ((klass), GDK_TYPE_DIRECTFB_DRAG_CONTEXT, GdkDirectfbDragContextClass))
#define GDK_IS_DIRECTFB_DRAG_CONTEXT(object)        (G_TYPE_CHECK_INSTANCE_TYPE ((object), GDK_TYPE_DIRECTFB_DRAG_CONTEXT))
#define GDK_IS_DIRECTFB_DRAG_CONTEXT_CLASS(klass)   (G_TYPE_CHECK_CLASS_TYPE ((klass), GDK_TYPE_DIRECTFB_DRAG_CONTEXT))
#define GDK_DIRECTFB_DRAG_CONTEXT_GET_CLASS(object) (G_TYPE_INSTANCE_GET_CLASS ((object), GDK_TYPE_DIRECTFB_DRAG_CONTEXT, GdkDirectfbDragContextClass))

typedef struct GdkDirectfbDragContext_ GdkDirectfbDragContext;
typedef struct GdkDirectfbDragContextClass_ GdkDirectfbDragContextClass;

struct GdkDirectfbDragContext_
{
  GdkDragContext parent_instance;
};

struct GdkDirectfbDragContextClass_
{
  GdkDragContextClass parent_class;
};

G_GNUC_INTERNAL
G_GNUC_WARN_UNUSED_RESULT
GdkDragContext * gdk_directfb_drag_context_new (GdkWindow *source_window,
                                                GdkDevice *device,
                                                GList     *targets);

G_END_DECLS

#endif /* GDK_DIRECTFB_DRAG_CONTEXT_ */
