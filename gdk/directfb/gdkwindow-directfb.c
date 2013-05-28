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

#include "gdkdeviceprivate.h"
#include "gdkdisplay-directfb.h"
#include "gdkdragcontext-directfb.h"
#include "gdkframeclockprivate.h"
#include "gdkprivate-directfb.h"
#include "gdkscreen-directfb.h"
#include "gdkwindow-directfb.h"
#include <cairo-directfb.h>

G_DEFINE_TYPE (GdkWindowImplDirectfb, gdk_window_impl_directfb, GDK_TYPE_WINDOW_IMPL)

static void
gdk_window_impl_directfb_init (GdkWindowImplDirectfb *impl)
{
}

static void
gdk_window_impl_directfb_dispose (GObject *object)
{
  GdkWindowImplDirectfb *impl = GDK_WINDOW_IMPL_DIRECTFB (object);

  G_OBJECT_CLASS (gdk_window_impl_directfb_parent_class)->dispose (object);

  impl->screen = NULL;
}

static void
gdk_window_impl_directfb_finalize (GObject *object)
{
  GdkWindowImplDirectfb *impl = GDK_WINDOW_IMPL_DIRECTFB (object);

  if (G_LIKELY (impl->area))
    cairo_region_destroy (impl->area);

  if (G_LIKELY (impl->damage))
    cairo_region_destroy (impl->damage);

  if (G_LIKELY (impl->surface))
    cairo_surface_destroy (impl->surface);

  if (G_LIKELY (impl->window))
    (void) impl->window->Release (impl->window);

  G_OBJECT_CLASS (gdk_window_impl_directfb_parent_class)->finalize (object);
}

static cairo_surface_t *
gdk_directfb_window_ref_cairo_surface (GdkWindow *window)
{
  GdkWindowImplDirectfb *impl;

  if (G_UNLIKELY (GDK_WINDOW_DESTROYED (window)))
    return NULL;

  impl = GDK_WINDOW_IMPL_DIRECTFB (window->impl);

  if (G_UNLIKELY (! impl->surface))
    {
      IDirectFB *dfb;
      DFBResult result;
      GdkDisplay *display;
      IDirectFBSurface *surface;

      if (G_UNLIKELY (! impl->window))
	return cairo_image_surface_create (CAIRO_FORMAT_ARGB32,
					   window->width,
					   window->height);

      result = impl->window->GetSurface (impl->window, &surface);
      if (G_UNLIKELY (DFB_OK != result))
	{
	  g_warning ("%s", DirectFBErrorString (result));
	  return cairo_image_surface_create (CAIRO_FORMAT_ARGB32,
					     window->width,
					     window->height);
	}

      display = gdk_screen_get_display (impl->screen);
      dfb = gdk_directfb_display_get_context (display);
      impl->surface = cairo_directfb_surface_create (dfb, surface);
      (void) surface->Release (surface);
    }

  return cairo_surface_reference (impl->surface);
}

static void
gdk_directfb_window_show (GdkWindow *window,
			  gboolean   already_mapped)
{
  GdkWindowImplDirectfb *impl;
  DFBResult result;

  if (G_UNLIKELY (GDK_WINDOW_DESTROYED (window)))
    return;

  impl = GDK_WINDOW_IMPL_DIRECTFB (window->impl);

  if (window->event_mask & GDK_STRUCTURE_MASK)
    (void) _gdk_make_event (window, GDK_MAP, NULL, FALSE);

  if (window->parent && window->parent->event_mask & GDK_SUBSTRUCTURE_MASK)
    (void) _gdk_make_event (window, GDK_MAP, NULL, FALSE);

  result = impl->window->SetOpacity (impl->window, impl->opacity);
  if (G_UNLIKELY (DFB_OK != result))
    g_warning ("%s", DirectFBErrorString (result));
}

static void
gdk_directfb_window_hide (GdkWindow *window)
{
  GdkWindowImplDirectfb *impl;
  DFBResult result;

  if (G_UNLIKELY (GDK_WINDOW_DESTROYED (window)))
    return;

  impl = GDK_WINDOW_IMPL_DIRECTFB (window->impl);

  if (window->event_mask & GDK_STRUCTURE_MASK)
    (void) _gdk_make_event (window, GDK_UNMAP, NULL, FALSE);

  if (window->parent && window->parent->event_mask & GDK_SUBSTRUCTURE_MASK)
    (void) _gdk_make_event (window, GDK_UNMAP, NULL, FALSE);

  result = impl->window->SetOpacity (impl->window, 0x00);
  if (G_UNLIKELY (DFB_OK != result))
    g_warning ("%s", DirectFBErrorString (result));
}

static void
gdk_directfb_window_withdraw (GdkWindow *window)
{
  gdk_directfb_window_hide (window);
}

static void
gdk_directfb_window_set_events (GdkWindow   *window,
				GdkEventMask event_mask)
{
  DFBWindowEventType dfb_event_mask =
    DWET_POSITION | DWET_SIZE | DWET_POSITION_SIZE;
  GdkWindowImplDirectfb *impl;
  DFBResult result;

  if (G_UNLIKELY (GDK_WINDOW_DESTROYED (window)))
    return;

  impl = GDK_WINDOW_IMPL_DIRECTFB (window->impl);

  impl->event_mask = GDK_STRUCTURE_MASK;

  if (event_mask & GDK_POINTER_MOTION_MASK)
    {
      dfb_event_mask |= DWET_MOTION;
      impl->event_mask |=
	GDK_POINTER_MOTION_MASK |
	GDK_BUTTON1_MOTION_MASK |
	GDK_BUTTON2_MOTION_MASK |
	GDK_BUTTON3_MOTION_MASK;
    }

  if (event_mask & GDK_BUTTON1_MOTION_MASK)
    {
      dfb_event_mask |= DWET_MOTION;
      impl->event_mask |=
	GDK_POINTER_MOTION_MASK |
	GDK_BUTTON1_MOTION_MASK |
	GDK_BUTTON2_MOTION_MASK |
	GDK_BUTTON3_MOTION_MASK;
    }

  if (event_mask & GDK_BUTTON2_MOTION_MASK)
    {
      dfb_event_mask |= DWET_MOTION;
      impl->event_mask |=
	GDK_POINTER_MOTION_MASK |
	GDK_BUTTON1_MOTION_MASK |
	GDK_BUTTON2_MOTION_MASK |
	GDK_BUTTON3_MOTION_MASK;
    }

  if (event_mask & GDK_BUTTON3_MOTION_MASK)
    {
      dfb_event_mask |= DWET_MOTION;
      impl->event_mask |=
	GDK_POINTER_MOTION_MASK |
	GDK_BUTTON1_MOTION_MASK |
	GDK_BUTTON2_MOTION_MASK |
	GDK_BUTTON3_MOTION_MASK;
    }

  if (event_mask & GDK_BUTTON_PRESS_MASK)
    {
      dfb_event_mask |= DWET_BUTTONDOWN;
      impl->event_mask |= GDK_BUTTON_PRESS_MASK;
    }

  if (event_mask & GDK_BUTTON_RELEASE_MASK)
    {
      dfb_event_mask |= DWET_BUTTONUP;
      impl->event_mask |= GDK_BUTTON_RELEASE_MASK;
    }

  if (event_mask & GDK_KEY_PRESS_MASK)
    {
      dfb_event_mask |= DWET_KEYDOWN;
      impl->event_mask |= GDK_KEY_PRESS_MASK;
    }

  if (event_mask & GDK_KEY_RELEASE_MASK)
    {
      dfb_event_mask |= DWET_KEYUP;
      impl->event_mask |= GDK_KEY_RELEASE_MASK;
    }

  if (event_mask & GDK_ENTER_NOTIFY_MASK)
    {
      dfb_event_mask |= DWET_ENTER;
      impl->event_mask |= GDK_ENTER_NOTIFY_MASK;
    }

  if (event_mask & GDK_LEAVE_NOTIFY_MASK)
    {
      dfb_event_mask |= DWET_LEAVE;
      impl->event_mask |= GDK_LEAVE_NOTIFY_MASK;
    }

  if (event_mask & GDK_FOCUS_CHANGE_MASK)
    {
      dfb_event_mask |= DWET_GOTFOCUS | DWET_LOSTFOCUS;
      impl->event_mask |= GDK_FOCUS_CHANGE_MASK;
    }

  if (event_mask & GDK_SCROLL_MASK)
    {
      dfb_event_mask |= DWET_WHEEL;
      impl->event_mask |= GDK_SCROLL_MASK;
    }

  window->event_mask = impl->event_mask;
  result = impl->window->EnableEvents (impl->window, dfb_event_mask);
  if (G_UNLIKELY (DFB_OK != result))
    g_warning ("%s", DirectFBErrorString (result));
}

static GdkEventMask
gdk_directfb_window_get_events (GdkWindow *window)
{
  GdkWindowImplDirectfb *impl;

  if (G_UNLIKELY (GDK_WINDOW_DESTROYED (window)))
    return 0;

  impl = GDK_WINDOW_IMPL_DIRECTFB (window->impl);

  return impl->event_mask;
}

static void
gdk_directfb_window_raise (GdkWindow *window)
{
  GdkWindowImplDirectfb *impl;
  DFBResult result;

  if (G_UNLIKELY (GDK_WINDOW_DESTROYED (window)))
    return;

  impl = GDK_WINDOW_IMPL_DIRECTFB (window->impl);

  result = impl->window->RaiseToTop (impl->window);
  if (G_UNLIKELY (DFB_OK != result))
    g_warning ("%s", DirectFBErrorString (result));
}

static void
gdk_directfb_window_lower (GdkWindow *window)
{
  GdkWindowImplDirectfb *impl;
  DFBResult result;

  if (G_UNLIKELY (GDK_WINDOW_DESTROYED (window)))
    return;

  impl = GDK_WINDOW_IMPL_DIRECTFB (window->impl);

  result = impl->window->LowerToBottom (impl->window);
  if (G_UNLIKELY (DFB_OK != result))
    g_warning ("%s", DirectFBErrorString (result));
}

static void
gdk_directfb_window_restack_under (GdkWindow *window,
				   GList     *native_siblings)
{
  GList *node;

  if (G_UNLIKELY (GDK_WINDOW_DESTROYED (window)))
    return;

  for (node = g_list_last (native_siblings); node; node = node->prev)
    {
      GdkWindowImplDirectfb *impl, *impl_sibling;
      GdkWindow *sibling;
      DFBResult result;

      sibling = node->data;
      impl = GDK_WINDOW_IMPL_DIRECTFB (window->impl);
      impl_sibling = GDK_WINDOW_IMPL_DIRECTFB (sibling->impl);

      result = impl->window->PutBelow (impl->window,
				       impl_sibling->window);
      if (G_UNLIKELY (DFB_OK != result))
	g_warning ("%s", DirectFBErrorString (result));

      window = sibling;
    }
}

static void
gdk_directfb_window_restack_toplevel (GdkWindow *window,
				      GdkWindow *sibling,
				      gboolean   above)
{
  if (G_UNLIKELY (GDK_WINDOW_DESTROYED (window)))
    return;

  if (sibling)
    {
      DFBResult result;
      GdkWindowImplDirectfb *impl, *impl_sibling;

      impl = GDK_WINDOW_IMPL_DIRECTFB (window->impl);
      impl_sibling = GDK_WINDOW_IMPL_DIRECTFB (sibling->impl);

      if (above)
	result = impl->window->PutAtop (impl->window,
					impl_sibling->window);
      else
	result = impl->window->PutBelow (impl->window,
					 impl_sibling->window);

      if (G_UNLIKELY (DFB_OK != result))
	g_warning ("%s", DirectFBErrorString (result));
    }
  else
    {
      if (above)
	gdk_directfb_window_raise (window);
      else
	gdk_directfb_window_lower (window);
    }
}

static void
gdk_directfb_window_move_resize (GdkWindow *window,
				 gboolean   with_move,
				 gint       x,
				 gint       y,
				 gint       width,
				 gint       height)
{
  gboolean with_resize = FALSE;
  GdkWindowImplDirectfb *impl;
  DFBResult result;

  if (G_UNLIKELY (GDK_WINDOW_DESTROYED (window)))
    return;

  impl = GDK_WINDOW_IMPL_DIRECTFB (window->impl);

  if (with_move)
    {
      if (width >= 0 && height >= 0)
	{
	  result = impl->window->SetBounds (impl->window, x, y,
					    width, height);
	  if (G_UNLIKELY (DFB_OK != result))
	    g_warning ("%s", DirectFBErrorString (result));

	  with_resize = TRUE;
	}
      else
	{
	  result = impl->window->MoveTo (impl->window, x, y);
	  if (G_UNLIKELY (DFB_OK != result))
	    g_warning ("%s", DirectFBErrorString (result));
	}

      window->x = x;
      window->y = y;
    }
  else
    if (width >= 0 && height >= 0)
      {
	result = impl->window->Resize (impl->window, width, height);
	if (G_UNLIKELY (DFB_OK != result))
	  g_warning ("%s", DirectFBErrorString (result));

	with_resize = TRUE;
      }

  if (with_resize)
    {
      cairo_rectangle_int_t rect;

      window->width = width;
      window->height = height;

      rect.x = 0;
      rect.y = 0;
      rect.width = window->width;
      rect.height = window->height;

      (void) cairo_region_subtract (impl->area, impl->area);
      (void) cairo_region_union_rectangle (impl->area, &rect);
    }
}

static void
gdk_directfb_window_set_background (GdkWindow       *window,
				    cairo_pattern_t *pattern)
{
}

static gboolean
gdk_directfb_window_reparent (GdkWindow *window,
			      GdkWindow *new_parent,
			      gint       x,
			      gint       y)
{
  GdkDisplay *display;
  GdkWindowImplDirectfb *impl;
  IDirectFBDisplayLayer *layer;
  IDirectFBEventBuffer *buffer;
  IDirectFBWindow *child, *parent;
  DFBWindowDescription desc =
    {
      .flags = DWDESC_POSX | DWDESC_POSY
	| DWDESC_WIDTH | DWDESC_HEIGHT
	| DWDESC_CAPS | DWDESC_SURFACE_CAPS | DWDESC_OPTIONS,
      .posx = window->x,
      .posy = window->y,
      .width = window->width,
      .height = window->height,
      .caps = DWCAPS_ALPHACHANNEL | DWCAPS_NODECORATION,
      .surface_caps = DSCAPS_PREMULTIPLIED,
      .options = DWOP_ALPHACHANNEL
    };
  DFBResult result;

  if (G_UNLIKELY (GDK_WINDOW_DESTROYED (window)))
    return FALSE;

  impl = GDK_WINDOW_IMPL_DIRECTFB (window->impl);

  if (G_UNLIKELY (! impl->window))
    return FALSE;

  parent = gdk_directfb_window_get_window (new_parent);

  if (G_LIKELY (parent))
    {
      desc.flags |= DWDESC_PARENT;
      desc.caps |= DWCAPS_SUBWINDOW;
      result = parent->GetID (parent, &desc.parent_id);
      if (G_UNLIKELY (DFB_OK != result))
	{
	  g_warning ("%s", DirectFBErrorString(result));
	  return FALSE;
	}
    }

  layer = gdk_directfb_screen_get_display_layer (impl->screen);
  result = layer->CreateWindow (layer, &desc, &child);
  if (G_UNLIKELY (DFB_OK != result))
    {
      g_warning ("%s", DirectFBErrorString(result));
      return FALSE;
    }

  display = gdk_screen_get_display (impl->screen);
  buffer = gdk_directfb_display_get_event_buffer (display);
  result = child->AttachEventBuffer (child, buffer);
  if (G_UNLIKELY (DFB_OK != result))
    {
      g_warning ("%s", DirectFBErrorString(result));
      return FALSE;
    }

  if (G_LIKELY (impl->surface))
    {
      cairo_surface_destroy (impl->surface);
      impl->surface = NULL;
    }

  (void) impl->window->Release (impl->window);
  impl->window = child;

  gdk_directfb_window_set_events (window, impl->event_mask);

  return TRUE;
}

static void
gdk_directfb_window_set_device_cursor (GdkWindow *window,
				       GdkDevice *device,
				       GdkCursor *cursor)
{
  if (!GDK_WINDOW_DESTROYED (window))
    GDK_DEVICE_GET_CLASS (device)->set_window_cursor (device, window, cursor);
}

static void
gdk_directfb_window_get_geometry (GdkWindow *window,
				  gint      *x,
				  gint      *y,
				  gint      *width,
				  gint      *height)
{
  if (G_LIKELY (x))
    *x = window->x;

  if (G_LIKELY (y))
    *y = window->y;

  if (G_LIKELY (width))
    *width = window->width;

  if (G_LIKELY (height))
    *height = window->height;
}

static gboolean
gdk_directfb_window_get_device_state (GdkWindow       *window,
				      GdkDevice       *device,
				      gdouble         *x,
				      gdouble         *y,
				      GdkModifierType *mask)
{
  GdkWindow *child;

  if (G_UNLIKELY (GDK_WINDOW_DESTROYED (window)))
    return FALSE;

  GDK_DEVICE_GET_CLASS (device)->query_state (device, window,
                                              NULL, &child,
                                              NULL, NULL,
                                              x, y, mask);

  return child ? TRUE : FALSE;
}

static void
gdk_directfb_window_shape_combine_region (GdkWindow            *window,
					  const cairo_region_t *shape_region,
					  gint                  offset_x,
					  gint                  offset_y)
{
}

static void
gdk_directfb_window_input_shape_combine_region (GdkWindow            *window,
						const cairo_region_t *shape_region,
						gint                  offset_x,
						gint                  offset_y)
{
}

static gboolean
gdk_directfb_window_set_static_gravities (GdkWindow *window,
					  gboolean   use_static)
{
  return TRUE;
}

static gboolean
gdk_directfb_window_queue_antiexpose (GdkWindow      *window,
				      cairo_region_t *area)
{
  return FALSE;
}

static void
gdk_directfb_window_destroy (GdkWindow *window,
			     gboolean   recursing,
			     gboolean   foreign_destroy)
{
  GdkWindowImplDirectfb *impl;
  IDirectFBEventBuffer *buffer;
  GdkDisplay *display;
  GdkScreen *screen;

  if (G_UNLIKELY (GDK_WINDOW_DESTROYED (window)))
    return;

  impl = GDK_WINDOW_IMPL_DIRECTFB (window->impl);

  screen = gdk_window_get_screen (window);
  gdk_directfb_screen_remove_window (screen, window);

  display = gdk_screen_get_display (screen);
  gdk_directfb_display_window_destroyed (display, window);
  buffer = gdk_directfb_display_get_event_buffer (display);

  (void) impl->window->DetachEventBuffer (impl->window, buffer);

  cairo_surface_destroy (impl->surface);
  impl->surface = NULL;

  if (! recursing && ! foreign_destroy)
    {
      impl->window->SetOpacity (impl->window, 0x0);
      impl->window->Close(impl->window);
      impl->window->Release(impl->window);
      impl->window = NULL;
    }
}

static void
gdk_directfb_window_destroy_foreign (GdkWindow *window)
{
}

static cairo_surface_t *
gdk_directfb_window_resize_cairo_surface (GdkWindow       *window,
					  cairo_surface_t *surface,
					  gint             width,
					  gint             height)
{
  return surface;
}

static cairo_region_t *
gdk_directfb_window_get_shape (GdkWindow *window)
{
  return NULL;
}

static cairo_region_t *
gdk_directfb_window_get_input_shape (GdkWindow *window)
{
  return NULL;
}

static gboolean
gdk_directfb_window_beep (GdkWindow *window)
{
  return FALSE;
}

static void
gdk_directfb_window_focus (GdkWindow *window,
			   guint32    timestamp)
{
}

static void
gdk_directfb_window_set_type_hint (GdkWindow        *window,
				   GdkWindowTypeHint hint)
{
}

static GdkWindowTypeHint
gdk_directfb_window_get_type_hint (GdkWindow *window)
{
  return GDK_WINDOW_TYPE_HINT_NORMAL;
}

static void
gdk_directfb_window_set_modal_hint (GdkWindow *window,
				    gboolean   modal)
{
  GdkWindowImplDirectfb *impl;
  DFBResult result;

  if (G_UNLIKELY (GDK_WINDOW_DESTROYED (window)))
    return;

  impl = GDK_WINDOW_IMPL_DIRECTFB (window->impl);

  result = impl->window->SetStackingClass (impl->window,
					   modal ? DWSC_UPPER : DWSC_MIDDLE);
  if (G_UNLIKELY (DFB_OK != result))
    g_warning ("%s", DirectFBErrorString (result));
}

static void
gdk_directfb_window_set_skip_taskbar_hint (GdkWindow *window,
					   gboolean   skips_taskbar)
{
}

static void
gdk_directfb_window_set_skip_pager_hint (GdkWindow *window,
					 gboolean   skips_pager)
{
}

static void
gdk_directfb_window_set_urgency_hint (GdkWindow *window,
				      gboolean   urgent)
{
}

static void
gdk_directfb_window_set_geometry_hints (GdkWindow         *window,
					const GdkGeometry *geometry,
					GdkWindowHints     geom_mask)
{
}

static void
gdk_directfb_window_set_title (GdkWindow   *window,
			       const gchar *title)
{
}

static void
gdk_directfb_window_set_role (GdkWindow   *window,
			      const gchar *role)
{
}

static void
gdk_directfb_window_set_startup_id (GdkWindow   *window,
				    const gchar *startup_id)
{
}

static void
gdk_directfb_window_set_transient_for (GdkWindow *window,
				       GdkWindow *parent)
{
  gint x, y;

  x = (parent->x + parent->width / 2) - window->width / 2;
  y = (parent->y + parent->height / 2) - window->height / 2;

  gdk_window_move (window, x, y);
}

static void
gdk_directfb_window_get_root_origin (GdkWindow *window,
				     gint      *x,
				     gint      *y)
{
  GdkWindowImplDirectfb *impl = GDK_WINDOW_IMPL_DIRECTFB (window->impl);
  gint x_offset, y_offset;

  (void) impl->window->GetPosition (impl->window, &x_offset, &y_offset);

  if (G_LIKELY (x))
    *x = x_offset;

  if (G_LIKELY (y))
    *y = y_offset;
}

static gint
gdk_directfb_window_get_root_coords (GdkWindow *window,
				     gint       x,
				     gint       y,
				     gint      *root_x,
				     gint      *root_y)
{
  gint x_offset, y_offset;

  gdk_directfb_window_get_root_origin (window, &x_offset, &y_offset);

  if (G_LIKELY (root_x))
      *root_x = x + x_offset;

  if (G_LIKELY (root_y))
      *root_y = y + y_offset;

  return 1;
}

static void
gdk_directfb_window_get_frame_extents (GdkWindow    *window,
				       GdkRectangle *rect)
{

  if (G_LIKELY (rect))
    {
      gdk_directfb_window_get_root_origin  (window, &rect->x, &rect->y);

      rect->width = window->width;
      rect->height = window->height;
    }
}

static void
gdk_directfb_window_set_override_redirect (GdkWindow *window,
					   gboolean   override_redirect)
{
}

static void
gdk_directfb_window_set_accept_focus (GdkWindow *window,
				      gboolean   accept_focus)
{
}

static void
gdk_directfb_window_set_focus_on_map (GdkWindow *window,
				      gboolean   focus_on_map)
{
}

static void
gdk_directfb_window_set_icon_list (GdkWindow *window,
				   GList     *pixbufs)
{
}

static void
gdk_directfb_window_set_icon_name (GdkWindow   *window,
				   const gchar *name)
{
}

static void
gdk_directfb_window_iconify (GdkWindow *window)
{
  gdk_directfb_window_hide (window);
}

static void
gdk_directfb_window_deiconify (GdkWindow *window)
{
  gdk_directfb_window_show (window, TRUE);
}

static void
gdk_directfb_window_stick (GdkWindow *window)
{
}

static void
gdk_directfb_window_unstick (GdkWindow *window)
{
}

static void
gdk_directfb_window_maximize (GdkWindow *window)
{
  gdk_window_fullscreen (window);
}

static void
gdk_directfb_window_unmaximize (GdkWindow *window)
{
  gdk_window_unfullscreen (window);
}

static void
gdk_directfb_window_fullscreen (GdkWindow *window)
{
  GdkWindowImplDirectfb *impl;
  gint width, height;

  if (G_UNLIKELY (GDK_WINDOW_DESTROYED (window)))
    return;

  impl = GDK_WINDOW_IMPL_DIRECTFB (window->impl);

  if (impl->fullscreen)
    return;

  width = gdk_screen_get_width (impl->screen);
  height = gdk_screen_get_height (impl->screen);

  impl->original.x = window->x;
  impl->original.y = window->y;
  impl->original.width = window->width;
  impl->original.height = window->height;

  gdk_window_move_resize (window, 0, 0, width, height);

  gdk_synthesize_window_state (window, 0, GDK_WINDOW_STATE_FULLSCREEN);
  impl->fullscreen = TRUE;
}

static void
gdk_directfb_window_unfullscreen (GdkWindow *window)
{
  GdkWindowImplDirectfb *impl;

  if (G_UNLIKELY (GDK_WINDOW_DESTROYED (window)))
    return;

  impl = GDK_WINDOW_IMPL_DIRECTFB (window->impl);

  if (! impl->fullscreen)
    return;

  gdk_window_move_resize (window, impl->original.x, impl->original.y,
			  impl->original.width, impl->original.height);

  gdk_synthesize_window_state (window, GDK_WINDOW_STATE_FULLSCREEN, 0);
  impl->fullscreen = FALSE;
}

static void
gdk_directfb_window_set_keep_above (GdkWindow *window,
				    gboolean   setting)
{
  GdkWindowImplDirectfb *impl;
  DFBResult result;

  if (G_UNLIKELY (GDK_WINDOW_DESTROYED (window)))
    return;

  impl = GDK_WINDOW_IMPL_DIRECTFB (window->impl);

  result = impl->window->SetStackingClass (impl->window, DWSC_UPPER);
  if (G_UNLIKELY (DFB_OK != result))
    g_warning ("%s", DirectFBErrorString (result));
}

static void
gdk_directfb_window_set_keep_below (GdkWindow *window,
				    gboolean   setting)
{
  GdkWindowImplDirectfb *impl;
  DFBResult result;

  if (G_UNLIKELY (GDK_WINDOW_DESTROYED (window)))
    return;

  impl = GDK_WINDOW_IMPL_DIRECTFB (window->impl);

  result = impl->window->SetStackingClass (impl->window, DWSC_LOWER);
  if (G_UNLIKELY (DFB_OK != result))
    g_warning ("%s", DirectFBErrorString (result));
}

static GdkWindow *
gdk_directfb_window_get_group (GdkWindow *window)
{
  return window;
}

static void
gdk_directfb_window_set_group (GdkWindow *window,
			       GdkWindow *leader)
{
}

static void
gdk_directfb_window_set_decorations (GdkWindow      *window,
				     GdkWMDecoration decorations)
{
}

static gboolean
gdk_directfb_window_get_decorations (GdkWindow       *window,
				     GdkWMDecoration *decorations)
{
  return FALSE;
}

static void
gdk_directfb_window_set_functions (GdkWindow    *window,
				   GdkWMFunction functions)
{
}

static void
gdk_directfb_window_begin_resize_drag (GdkWindow    *window,
				       GdkWindowEdge edge,
                                       GdkDevice    *device,
				       gint          button,
				       gint          root_x,
				       gint          root_y,
				       guint32       timestamp)
{
}

static void
gdk_directfb_window_begin_move_drag (GdkWindow *window,
                                     GdkDevice *device,
				     gint       button,
				     gint       root_x,
				     gint       root_y,
				     guint32    timestamp)
{
}

static void
gdk_directfb_window_set_opacity (GdkWindow *window,
				 gdouble    opacity)
{
  GdkWindowImplDirectfb *impl;
  DFBResult result;

  if (G_UNLIKELY (GDK_WINDOW_DESTROYED (window)))
    return;

  impl = GDK_WINDOW_IMPL_DIRECTFB (window->impl);

  if (opacity < 0.0)
    opacity = 0.0;
  else if (opacity > 1.0)
    opacity = 1.0;

  impl->opacity = opacity * 0xff;

  result =  impl->window->SetOpacity (impl->window, impl->opacity);
  if (G_UNLIKELY (DFB_OK != result))
    g_warning ("%s", DirectFBErrorString (result));
}

static void
gdk_directfb_window_set_composited (GdkWindow *window,
				    gboolean   composited)
{
}

static void
gdk_directfb_window_destroy_notify (GdkWindow *window)
{
  if (! GDK_WINDOW_DESTROYED (window))
    {
      if (GDK_WINDOW_TYPE (window) != GDK_WINDOW_FOREIGN)
	g_warning ("GdkWindow %p unexpectedly destroyed", window);

      _gdk_window_destroy (window, TRUE);
    }

  g_object_unref (window);
}

static void
gdk_directfb_window_register_dnd (GdkWindow *window)
{
}

static GdkDragContext *
gdk_directfb_window_drag_begin (GdkWindow *window,
				GdkDevice *device,
				GList     *targets)
{
  return gdk_directfb_drag_context_new (window, device, targets);
}

static void
gdk_directfb_window_process_updates_recurse (GdkWindow      *window,
					     cairo_region_t *region)
{
  GdkWindowImplDirectfb *impl;

  if (G_UNLIKELY (GDK_WINDOW_DESTROYED (window)))
    return;

  impl = GDK_WINDOW_IMPL_DIRECTFB (window->impl);

  _gdk_window_process_updates_recurse (window, region);

  (void) cairo_region_union (impl->damage, region);
}

static void
gdk_directfb_window_sync_rendering (GdkWindow *window)
{
  GdkWindowImplDirectfb *impl;
  IDirectFBSurface *surface;
  DFBResult result;

  if (G_UNLIKELY (GDK_WINDOW_DESTROYED (window)))
    return;

  impl = GDK_WINDOW_IMPL_DIRECTFB (window->impl);

  if (cairo_region_is_empty (impl->damage))
    return;

  result = impl->window->GetSurface (impl->window, &surface);
  if (G_UNLIKELY (DFB_OK != result))
    {
      g_warning ("%s", DirectFBErrorString (result));
      return;
    }

  if (! cairo_region_equal (impl->area, impl->damage))
    {
      cairo_rectangle_int_t extents;
      DFBRegion damage;

      cairo_region_get_extents (impl->damage, &extents);
      damage.x1 = extents.x;
      damage.y1 = extents.y;
      damage.x2 = extents.x + extents.width - 1;
      damage.y2 = extents.y + extents.height - 1;

      result = surface->Flip (surface, &damage, DSFLIP_ONSYNC);
    }
  else
    result = surface->Flip (surface, NULL, DSFLIP_ONSYNC);

  if (G_UNLIKELY (DFB_OK != result))
    g_warning ("%s", DirectFBErrorString (result));

  (void) surface->Release (surface);
  (void) cairo_region_subtract (impl->damage, impl->damage);
}

/**
 * gdk_directfb_window_simulate_event:
 * @window: The #GdkWindow receiving the event
 * @event: A #DFBWindowEvent
 * @x: X-coordinate within the @window
 * @y: Y-coordinate within the @window
 * @modifiers: Active keyboard modifiers
 *
 * This function implements the common functionality for
 * gdk_directfb_window_simulate_key() and
 * gdk_directfb_window_simulate_button().
 *
 * Returns: #TRUE if the event was posted.
 *
 * Since: 3.10
 */
static gboolean
gdk_directfb_window_simulate_event (GdkWindow      *window,
				    DFBWindowEvent *event,
				    gint            x,
				    gint            y,
				    GdkModifierType modifiers)
{
  IDirectFBDisplayLayer *layer;
  IDirectFBEventBuffer *buffer;
  GdkWindowImplDirectfb *impl;
  GdkDisplay *display;
  int root_x, root_y;
  GdkScreen *screen;
  DFBResult result;

  if (G_UNLIKELY (GDK_WINDOW_DESTROYED (window)))
    return FALSE;

  impl = GDK_WINDOW_IMPL_DIRECTFB (window->impl);

  result = impl->window->GetPosition (impl->window, &root_x, &root_y);
  if (G_UNLIKELY (DFB_OK != result))
    {
      g_warning ("%s", DirectFBErrorString (result));
      return FALSE;
    }

  screen = gdk_window_get_screen (window);
  layer = gdk_directfb_screen_get_display_layer (screen);
  result = layer->WarpCursor (layer, root_x + x, root_y + y);
  if (G_UNLIKELY (DFB_OK != result))
    {
      g_warning ("%s", DirectFBErrorString (result));
      return FALSE;
    }

  event->clazz = DFEC_WINDOW;
  event->flags = DWEF_NONE;

  result = impl->window->GetID (impl->window, &event->window_id);
  if (G_UNLIKELY (DFB_OK != result))
    {
      g_warning ("%s", DirectFBErrorString (result));
      return FALSE;
    }

  result = layer->GetCursorPosition (layer, &event->cx, &event->cy);
  if (G_UNLIKELY (DFB_OK != result))
    {
      g_warning ("%s", DirectFBErrorString (result));
      return FALSE;
    }

  event->x = event->cx - root_x;
  event->y = event->cy - root_y;
  event->step = 0;

  result = impl->window->GetSize (impl->window, &event->w, &event->h);
  if (G_UNLIKELY (DFB_OK != result))
    {
      g_warning ("%s", DirectFBErrorString (result));
      return FALSE;
    }

  gdk_directfb_parse_modifier_mask (modifiers,
				    &event->modifiers,
				    &event->locks,
				    &event->buttons);

  (void) gettimeofday (&event->timestamp, NULL);

  display = gdk_screen_get_display (screen);
  buffer = gdk_directfb_display_get_event_buffer (display);

  result = buffer->PostEvent (buffer, (DFBEvent *) event);
  if (G_UNLIKELY (DFB_OK != result))
    {
      g_warning ("%s", DirectFBErrorString (result));
      return FALSE;
    }

  return TRUE;
}

static gboolean
gdk_directfb_window_simulate_key (GdkWindow      *window,
				  gint            x,
				  gint            y,
				  guint           keyval,
				  GdkModifierType modifiers,
				  GdkEventType    key_pressrelease)
{
  DFBWindowEvent event;

  switch (key_pressrelease)
    {
    case GDK_KEY_PRESS:
      event.type = DWET_KEYDOWN;
      break;

    case GDK_KEY_RELEASE:
      event.type = DWET_KEYUP;
      break;

    default:
      return FALSE;
    }

  event.key_id = DFB_KEY (IDENTIFIER, 0);
  event.key_symbol = DFB_KEY (UNICODE, gdk_keyval_to_unicode (keyval));

  return gdk_directfb_window_simulate_event (window, &event, x, y, modifiers);
}

static gboolean
gdk_directfb_window_simulate_button (GdkWindow      *window,
				     gint            x,
				     gint            y,
				     guint           button,
				     GdkModifierType modifiers,
				     GdkEventType    button_pressrelease)
{
  DFBWindowEvent event;

  switch (button_pressrelease)
    {
    case GDK_BUTTON_PRESS:
      event.type = DWET_BUTTONDOWN;
      break;

    case GDK_BUTTON_RELEASE:
      event.type = DWET_BUTTONUP;
      break;

    default:
      return FALSE;
    }

  switch (button)
    {
    case GDK_BUTTON_PRIMARY:
      event.button = DIBI_LEFT;
      break;

    case GDK_BUTTON_MIDDLE:
      event.button = DIBI_MIDDLE;
      break;

    case GDK_BUTTON_SECONDARY:
      event.button = DIBI_RIGHT;
      break;

    default:
      return FALSE;
    }

  return gdk_directfb_window_simulate_event (window, &event, x, y, modifiers);
}

static gboolean
gdk_directfb_window_get_property (GdkWindow *window,
				  GdkAtom    property,
				  GdkAtom    type,
				  gulong     offset,
				  gulong     length,
				  gint       pdelete,
				  GdkAtom   *actual_property_type,
				  gint      *actual_format_type,
				  gint      *actual_length,
				  guchar   **data)
{
  return FALSE;
}

static void
gdk_directfb_window_change_property (GdkWindow    *window,
				     GdkAtom       property,
				     GdkAtom       type,
				     gint          format,
				     GdkPropMode   mode,
				     const guchar *data,
				     gint          nelements)
{
}

static void
gdk_directfb_window_delete_property (GdkWindow *window,
				     GdkAtom    property)
{
}

static GdkDragProtocol
gdk_directfb_window_get_drag_protocol (GdkWindow  *window,
				       GdkWindow **target)
{
  return GDK_DRAG_PROTO_NONE;
}

static void
gdk_directfb_window_set_opaque_region (GdkWindow      *window,
				       cairo_region_t *region)
{
}

static void
gdk_window_impl_directfb_class_init (GdkWindowImplDirectfbClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);
  GdkWindowImplClass *impl_class = GDK_WINDOW_IMPL_CLASS (klass);

  object_class->dispose = gdk_window_impl_directfb_dispose;
  object_class->finalize = gdk_window_impl_directfb_finalize;

  impl_class->ref_cairo_surface = gdk_directfb_window_ref_cairo_surface;
  impl_class->show = gdk_directfb_window_show;
  impl_class->hide = gdk_directfb_window_hide;
  impl_class->withdraw = gdk_directfb_window_withdraw;
  impl_class->set_events = gdk_directfb_window_set_events;
  impl_class->get_events = gdk_directfb_window_get_events;
  impl_class->raise = gdk_directfb_window_raise;
  impl_class->lower = gdk_directfb_window_lower;
  impl_class->restack_under = gdk_directfb_window_restack_under;
  impl_class->restack_toplevel = gdk_directfb_window_restack_toplevel;
  impl_class->move_resize = gdk_directfb_window_move_resize;
  impl_class->set_background = gdk_directfb_window_set_background;
  impl_class->reparent = gdk_directfb_window_reparent;
  impl_class->set_device_cursor = gdk_directfb_window_set_device_cursor;
  impl_class->get_geometry = gdk_directfb_window_get_geometry;
  impl_class->get_device_state = gdk_directfb_window_get_device_state;
  impl_class->shape_combine_region = gdk_directfb_window_shape_combine_region;
  impl_class->input_shape_combine_region = gdk_directfb_window_input_shape_combine_region;
  impl_class->set_static_gravities = gdk_directfb_window_set_static_gravities;
  impl_class->queue_antiexpose = gdk_directfb_window_queue_antiexpose;
  impl_class->destroy = gdk_directfb_window_destroy;
  impl_class->destroy_foreign = gdk_directfb_window_destroy_foreign;
  impl_class->resize_cairo_surface = gdk_directfb_window_resize_cairo_surface;
  impl_class->get_shape = gdk_directfb_window_get_shape;
  impl_class->get_input_shape = gdk_directfb_window_get_input_shape;
  impl_class->beep = gdk_directfb_window_beep;
  impl_class->focus = gdk_directfb_window_focus;
  impl_class->set_type_hint = gdk_directfb_window_set_type_hint;
  impl_class->get_type_hint = gdk_directfb_window_get_type_hint;
  impl_class->set_modal_hint = gdk_directfb_window_set_modal_hint;
  impl_class->set_skip_taskbar_hint = gdk_directfb_window_set_skip_taskbar_hint;
  impl_class->set_skip_pager_hint = gdk_directfb_window_set_skip_pager_hint;
  impl_class->set_urgency_hint = gdk_directfb_window_set_urgency_hint;
  impl_class->set_geometry_hints = gdk_directfb_window_set_geometry_hints;
  impl_class->set_title = gdk_directfb_window_set_title;
  impl_class->set_role = gdk_directfb_window_set_role;
  impl_class->set_startup_id = gdk_directfb_window_set_startup_id;
  impl_class->set_transient_for = gdk_directfb_window_set_transient_for;
  impl_class->get_root_origin = gdk_directfb_window_get_root_origin;
  impl_class->get_root_coords = gdk_directfb_window_get_root_coords;
  impl_class->get_frame_extents = gdk_directfb_window_get_frame_extents;
  impl_class->set_override_redirect = gdk_directfb_window_set_override_redirect;
  impl_class->set_accept_focus = gdk_directfb_window_set_accept_focus;
  impl_class->set_focus_on_map = gdk_directfb_window_set_focus_on_map;
  impl_class->set_icon_list = gdk_directfb_window_set_icon_list;
  impl_class->set_icon_name = gdk_directfb_window_set_icon_name;
  impl_class->iconify = gdk_directfb_window_iconify;
  impl_class->deiconify = gdk_directfb_window_deiconify;
  impl_class->stick = gdk_directfb_window_stick;
  impl_class->unstick = gdk_directfb_window_unstick;
  impl_class->maximize = gdk_directfb_window_maximize;
  impl_class->unmaximize = gdk_directfb_window_unmaximize;
  impl_class->fullscreen = gdk_directfb_window_fullscreen;
  impl_class->unfullscreen = gdk_directfb_window_unfullscreen;
  impl_class->set_keep_above = gdk_directfb_window_set_keep_above;
  impl_class->set_keep_below = gdk_directfb_window_set_keep_below;
  impl_class->get_group = gdk_directfb_window_get_group;
  impl_class->set_group = gdk_directfb_window_set_group;
  impl_class->set_decorations = gdk_directfb_window_set_decorations;
  impl_class->get_decorations = gdk_directfb_window_get_decorations;
  impl_class->set_functions = gdk_directfb_window_set_functions;
  impl_class->begin_resize_drag = gdk_directfb_window_begin_resize_drag;
  impl_class->begin_move_drag = gdk_directfb_window_begin_move_drag;
  impl_class->set_opacity = gdk_directfb_window_set_opacity;
  impl_class->set_composited = gdk_directfb_window_set_composited;
  impl_class->destroy_notify = gdk_directfb_window_destroy_notify;
  impl_class->register_dnd = gdk_directfb_window_register_dnd;
  impl_class->drag_begin = gdk_directfb_window_drag_begin;
  impl_class->process_updates_recurse = gdk_directfb_window_process_updates_recurse;
  impl_class->sync_rendering = gdk_directfb_window_sync_rendering;
  impl_class->simulate_key = gdk_directfb_window_simulate_key;
  impl_class->simulate_button = gdk_directfb_window_simulate_button;
  impl_class->get_property = gdk_directfb_window_get_property;
  impl_class->change_property = gdk_directfb_window_change_property;
  impl_class->delete_property = gdk_directfb_window_delete_property;
  impl_class->get_drag_protocol = gdk_directfb_window_get_drag_protocol;
  impl_class->set_opaque_region = gdk_directfb_window_set_opaque_region;
}

G_DEFINE_TYPE (GdkDirectfbWindow, gdk_directfb_window, GDK_TYPE_WINDOW)

static void
gdk_directfb_window_init (GdkDirectfbWindow *directfb_window)
{
}

static void
gdk_directfb_window_class_init (GdkDirectfbWindowClass *klass)
{
}

/**
 * gdk_directfb_root_window_new:
 * @screen: The #GdkScreen for which this root window is being created
 *
 * Creates a new root window.
 *
 * Returns: (transfer full): A new #GdkWindow.
 *
 * Since: 3.10
 */
GdkWindow *
gdk_directfb_root_window_new (GdkScreen *screen)
{
  IDirectFB *dfb;
  DFBResult result;
  GdkDisplay *display;
  GdkWindow *window = NULL;
  IDirectFBSurface *surface;
  GdkWindowImplDirectfb *impl;
  IDirectFBDisplayLayer *layer;
  DFBSurfacePixelFormat format;
  cairo_rectangle_int_t area =
    {
      0, 0,
      gdk_screen_get_width (screen),
      gdk_screen_get_height (screen)
    };

  layer = gdk_directfb_screen_get_display_layer (screen);
  result = layer->GetSurface (layer, &surface);
  if (G_UNLIKELY (DFB_OK != result))
    {
      g_warning ("%s", DirectFBErrorString (result));
      goto exit;
    }

  result = surface->GetPixelFormat (surface, &format);
  if (G_UNLIKELY (DFB_OK != result))
    {
      g_warning ("%s", DirectFBErrorString (result));
      goto exit;
    }

  window = g_object_new (GDK_TYPE_DIRECTFB_WINDOW, NULL);
  window->impl = g_object_new (GDK_TYPE_WINDOW_IMPL_DIRECTFB, NULL);
  window->impl_window = window;
  window->visual = gdk_screen_get_system_visual (screen);
  window->window_type = GDK_WINDOW_ROOT;
  window->depth = DFB_BITS_PER_PIXEL (format);
  window->x = 0;
  window->y = 0;
  window->abs_x = 0;
  window->abs_y = 0;
  window->width = area.width;
  window->height = area.height;
  window->viewable = TRUE;
  window->event_mask = GDK_STRUCTURE_MASK;

  impl = GDK_WINDOW_IMPL_DIRECTFB (window->impl);
  impl->window = NULL;
  impl->screen = screen;
  impl->damage = cairo_region_create ();
  impl->area = cairo_region_create_rectangle (&area);
  impl->opacity = 0xff;
  impl->fullscreen = FALSE;
  impl->event_mask = GDK_STRUCTURE_MASK;

  (void) surface->Clear (surface, 0x00, 0x00, 0x00, 0x00);
  display = gdk_screen_get_display (impl->screen);
  dfb = gdk_directfb_display_get_context (display);
  impl->surface = cairo_directfb_surface_create (dfb, surface);
  (void) surface->Release (surface);

  _gdk_window_update_size (window);

exit:

  return window;
}

/**
 * gdk_directfb_window_after_paint:
 * @clock: A #GdkFrameClock
 * @window: A #GdkWindow
 *
 * Handles the "after-paint" signal for a #GdkWindow. This flips the DirectFB
 * window so it is displayed on the screen.
 *
 * Since: 3.10
 */
static void
gdk_directfb_window_after_paint (GdkFrameClock *clock,
				 GdkWindow     *window)
{
  GdkFrameTimings *timings;

  gdk_directfb_window_sync_rendering (window);
  timings = gdk_frame_clock_get_current_timings (clock);
  timings->complete = TRUE;
}

/**
 * gdk_directfb_window_impl_new:
 * @display: The #GdkDisplay for which the window was created
 * @window: The #GdkWindow for which this implementation is being created
 * @real_parent: The parent window
 * @screen: The #GdkScreen this window is on
 * @event_mask: The #GdkEventMask for this window
 *
 * Creates a new window implementation object. The implementation object is
 * stored inside the specified window after creation.
 *
 * Since: 3.10
 */
void
gdk_directfb_window_impl_new (GdkDisplay  *display,
			      GdkWindow   *window,
			      GdkWindow   *real_parent,
			      GdkScreen   *screen,
			      GdkEventMask event_mask)
{
  IDirectFBWindow *parent;
  GdkFrameClock *frame_clock;
  IDirectFBSurface *surface;
  IDirectFBDisplayLayer *layer;
  GdkWindowImplDirectfb *impl;
  IDirectFBEventBuffer *event_buffer;
  DFBWindowDescription desc =
    {
      .flags = DWDESC_POSX | DWDESC_POSY
	     | DWDESC_WIDTH | DWDESC_HEIGHT
	     | DWDESC_CAPS | DWDESC_SURFACE_CAPS | DWDESC_OPTIONS,
      .posx = window->x,
      .posy = window->y,
      .width = window->width,
      .height = window->height,
      .caps = DWCAPS_ALPHACHANNEL | DWCAPS_NODECORATION,
      .surface_caps = DSCAPS_PREMULTIPLIED,
      .options = DWOP_ALPHACHANNEL
    };
  cairo_rectangle_int_t area =
    {
      0, 0,
      window->width,
      window->height
    };
  DFBResult result;

  window->impl = g_object_new (GDK_TYPE_WINDOW_IMPL_DIRECTFB, NULL);
  impl = GDK_WINDOW_IMPL_DIRECTFB (window->impl);
  impl->screen = screen;
  impl->damage = cairo_region_create ();
  impl->area = cairo_region_create_rectangle (&area);
  impl->opacity = 0xff;
  impl->fullscreen = FALSE;

  if (window->input_only)
    desc.caps |= DWCAPS_INPUTONLY;

  switch (window->window_type)
    {
    case GDK_WINDOW_TEMP:
    case GDK_WINDOW_TOPLEVEL:
      break;

    case GDK_WINDOW_CHILD:
    case GDK_WINDOW_FOREIGN:
    case GDK_WINDOW_OFFSCREEN:
      parent = gdk_directfb_window_get_window (real_parent);
      if (G_LIKELY (parent))
	{
	  desc.flags |= DWDESC_PARENT;
	  desc.caps |= DWCAPS_SUBWINDOW;
	  result = parent->GetID (parent, &desc.parent_id);
	  if (G_UNLIKELY (DFB_OK != result))
	    {
	      g_warning ("%s", DirectFBErrorString(result));
	      return;
	    }
	}
      break;

    case GDK_WINDOW_ROOT:
      g_assert_not_reached ();
    }

  layer = gdk_directfb_screen_get_display_layer (screen);
  result = layer->CreateWindow (layer, &desc, &impl->window);
  if (G_UNLIKELY (DFB_OK != result))
    {
      g_warning ("%s", DirectFBErrorString(result));
      return;
    }

  result = impl->window->GetSurface (impl->window, &surface);
  if (G_LIKELY (DFB_OK == result))
    {
      (void) surface->Clear (surface, 0x00, 0x00, 0x00, 0x00);
      (void) surface->Flip (surface, NULL, DSFLIP_NONE);
      (void) surface->Clear (surface, 0x00, 0x00, 0x00, 0x00);
      (void) surface->Release (surface);
    }

  event_buffer = gdk_directfb_display_get_event_buffer (display);
  result = impl->window->AttachEventBuffer (impl->window, event_buffer);
  if (G_UNLIKELY (DFB_OK != result))
    {
      g_warning ("%s", DirectFBErrorString(result));
      return;
    }

  gdk_directfb_window_set_events (window, event_mask);

  frame_clock = gdk_window_get_frame_clock (window);

  (void) g_signal_connect (frame_clock, "after-paint",
			   G_CALLBACK (gdk_directfb_window_after_paint),
			   window);

  gdk_directfb_screen_add_window (screen, window);
}

/**
 * gdk_directfb_window_get_window:
 * @window: A #GdkWindow object
 *
 * Get the underlying DirectFB window object.
 *
 * Returns: (transfer none): A #IDirectFBWindow
 *
 * Since: 3.10
 */
IDirectFBWindow *
gdk_directfb_window_get_window (GdkWindow *window)
{
  g_return_val_if_fail (GDK_IS_WINDOW_IMPL_DIRECTFB (window->impl), NULL);

  return GDK_WINDOW_IMPL_DIRECTFB (window->impl)->window;
}
