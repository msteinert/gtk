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
#include "gdkcursortheme-xcursor.h"
#include "gdkdevicemanager-directfb.h"
#include "gdkdisplay-directfb.h"
#include "gdkkeymap-directfb.h"
#include "gdkkeysprivate.h"
#include "gdkprivate-directfb.h"
#include "gdkscreen-directfb.h"
#include "gdksource-directfb.h"
#include "gdkwindow-directfb.h"
#include <cairo-directfb.h>

G_DEFINE_TYPE (GdkDirectfbDisplay, gdk_directfb_display, GDK_TYPE_DISPLAY)

static void
gdk_directfb_display_init (GdkDirectfbDisplay *directfb_display)
{
}

static void
gdk_directfb_display_dispose (GObject *object)
{
  GdkDirectfbDisplay *directfb_display = GDK_DIRECTFB_DISPLAY (object);

  if (directfb_display->cursor_theme)
    g_object_unref (directfb_display->cursor_theme);

  if (directfb_display->keymap)
    g_object_unref (directfb_display->keymap);

  if (directfb_display->screen)
    g_object_unref (directfb_display->screen);

  G_OBJECT_CLASS (gdk_directfb_display_parent_class)->dispose (object);

  directfb_display->cursor_theme = NULL;
  directfb_display->keymap = NULL;
  directfb_display->screen = NULL;
}

static void
gdk_directfb_display_finalize (GObject *object)
{
  GdkDirectfbDisplay *directfb_display = GDK_DIRECTFB_DISPLAY (object);

  if (G_LIKELY (directfb_display->buffer))
    (void) directfb_display->buffer->Release (directfb_display->buffer);

  if (G_LIKELY (directfb_display->dfb))
    (void) directfb_display->dfb->Release (directfb_display->dfb);

  G_OBJECT_CLASS (gdk_directfb_display_parent_class)->finalize (object);
}

static const gchar *
gdk_directfb_display_get_name (GdkDisplay *display)
{
  GdkDirectfbDisplay *directfb_display = GDK_DIRECTFB_DISPLAY (display);
  static char name[DFB_GRAPHICS_DEVICE_DESC_NAME_LENGTH];
  DFBGraphicsDeviceDescription desc;

  (void) directfb_display->dfb->GetDeviceDescription (directfb_display->dfb,
						      &desc);

  (void) memcpy (name, desc.name, DFB_GRAPHICS_DEVICE_DESC_NAME_LENGTH);
  name[DFB_GRAPHICS_DEVICE_DESC_NAME_LENGTH - 1] = '\0';

  return name;
}

static GdkScreen *
gdk_directfb_display_get_default_screen (GdkDisplay *display)
{
  return GDK_DIRECTFB_DISPLAY (display)->screen;
}

static void
gdk_directfb_display_beep (GdkDisplay *display)
{
}

static void
gdk_directfb_display_sync (GdkDisplay *display)
{
}

static void
gdk_directfb_display_flush (GdkDisplay *display)
{
}

static gboolean
gdk_directfb_display_has_pending (GdkDisplay *display)
{
  return FALSE;
}

static void
gdk_directfb_display_queue_events (GdkDisplay *display)
{
}

static GdkWindow *
gdk_directfb_display_get_default_group (GdkDisplay *display)
{
  return NULL;
}

static gboolean
gdk_directfb_display_supports_selection_notification (GdkDisplay *display)
{
  return FALSE;
}

static gboolean
gdk_directfb_display_request_selection_notification (GdkDisplay *display,
						     GdkAtom     selection)

{
  return FALSE;
}

static gboolean
gdk_directfb_display_supports_clipboard_persistence (GdkDisplay *display)
{
  return FALSE;
}

static void
gdk_directfb_display_store_clipboard (GdkDisplay    *display,
				      GdkWindow     *clipboard_window,
				      guint32        time_,
				      const GdkAtom *targets,
				      gint           n_targets)
{
}

static gboolean
gdk_directfb_display_supports_shapes (GdkDisplay *display)
{
  return FALSE;
}

static gboolean
gdk_directfb_display_supports_input_shapes (GdkDisplay *display)
{
  return FALSE;
}

static gboolean
gdk_directfb_display_supports_composite (GdkDisplay *display)
{
  return FALSE;
}

static GList *
gdk_directfb_display_list_devices (GdkDisplay *display)
{
  return gdk_device_manager_list_devices (display->device_manager,
					  GDK_DEVICE_TYPE_MASTER);
}

static GdkCursor *
gdk_directfb_display_get_cursor_for_type (GdkDisplay   *display,
					  GdkCursorType cursor_type)
{
  GdkCursorTheme *cursor_theme =
    gdk_directfb_display_get_cursor_theme (display);

  return gdk_cursor_theme_get_cursor (cursor_theme, cursor_type);
}

static GdkCursor *
gdk_directfb_display_get_cursor_for_name (GdkDisplay  *display,
					  const gchar *cursor_name)
{
  GdkCursorType cursor_type;
  GEnumClass *enum_class;
  GEnumValue *enum_value;

  enum_class = g_type_class_ref (GDK_TYPE_CURSOR_TYPE);
  enum_value = g_enum_get_value_by_nick (enum_class, cursor_name);
  if (! enum_value)
    return NULL;

  cursor_type = enum_value->value;
  return gdk_directfb_display_get_cursor_for_type (display, cursor_type);
}

GdkCursor *
gdk_directfb_display_get_cursor_for_surface (GdkDisplay      *display,
					     cairo_surface_t *surface,
					     gdouble          x,
					     gdouble          y)
{
  cairo_t *cr;
  IDirectFB *dfb;
  DFBResult result;
  GdkCursor *cursor = NULL;
  DFBSurfaceDescription desc;
  cairo_surface_t *cr_dfb_surface;
  IDirectFBSurface *dfb_surface = NULL;

  switch (cairo_surface_get_type (surface))
    {
    case CAIRO_SURFACE_TYPE_IMAGE:
      desc.flags = DSDESC_WIDTH | DSDESC_HEIGHT |
	DSDESC_PIXELFORMAT | DSDESC_CAPS;
      desc.width = cairo_image_surface_get_width (surface);
      desc.height = cairo_image_surface_get_height (surface);
      desc.caps = DSCAPS_PREMULTIPLIED;

      switch (cairo_surface_get_content (surface))
	{
	case CAIRO_CONTENT_COLOR:
	  desc.pixelformat = DSPF_RGB32;
	  break;

	case CAIRO_CONTENT_ALPHA:
	  desc.pixelformat = DSPF_A8;
	  break;

	case CAIRO_CONTENT_COLOR_ALPHA:
	  desc.pixelformat = DSPF_ARGB;
	  break;
	}

      dfb = gdk_directfb_display_get_context (display);
      result = dfb->CreateSurface (dfb, &desc, &dfb_surface);
      if (G_UNLIKELY (DFB_OK != result))
	{
	  g_warning ("%s", DirectFBErrorString (result));
	  break;
	}

      cr_dfb_surface = cairo_directfb_surface_create (dfb, dfb_surface);
      cr = cairo_create (cr_dfb_surface);
      cairo_surface_destroy (cr_dfb_surface);
      cairo_set_source_surface (cr, surface, 0, 0);
      cairo_paint (cr);
      cairo_destroy (cr);
      break;

    case CAIRO_SURFACE_TYPE_DIRECTFB:
      dfb_surface = cairo_directfb_surface_get_surface (surface);
      (void) dfb_surface->AddRef (dfb_surface);
      break;

    default:
      break;
    }

  if (G_LIKELY (dfb_surface))
    {
      cursor = gdk_directfb_cursor_new (display, GDK_CURSOR_IS_PIXMAP,
					dfb_surface, x, y);
      (void) dfb_surface->Release (dfb_surface);
    }

  return cursor;
}

static void
gdk_directfb_display_get_default_cursor_size (GdkDisplay *display,
					      guint      *width,
					      guint      *height)
{
  *width = *height = 32;
}

static void
gdk_directfb_display_get_maximal_cursor_size (GdkDisplay *display,
					      guint      *width,
					      guint      *height)
{
  *width = *height = 256;
}

static gboolean
gdk_directfb_display_supports_cursor_alpha (GdkDisplay *display)
{
  return TRUE;
}

static gboolean
gdk_directfb_display_supports_cursor_color (GdkDisplay *display)
{
  return TRUE;
}

static void
gdk_directfb_display_before_process_all_updates (GdkDisplay *display)
{
}

static void
gdk_directfb_display_after_process_all_updates (GdkDisplay *display)
{
}

static gulong
gdk_directfb_display_get_next_serial (GdkDisplay *display)
{
  GdkDirectfbDisplay *directfb_display = GDK_DIRECTFB_DISPLAY (display);

  return gdk_directfb_source_get_next_serial (directfb_display->source);
}

static void
gdk_directfb_display_notify_startup_complete (GdkDisplay  *display,
					      const gchar *startup_id)
{
}

static void
gdk_directfb_display_event_data_copy (GdkDisplay     *display,
				      const GdkEvent *src,
				      GdkEvent       *dst)
{
}

static void
gdk_directfb_display_event_data_free (GdkDisplay *display,
				      GdkEvent   *event)
{
}

static void
gdk_directfb_display_create_window_impl (GdkDisplay    *display,
					 GdkWindow     *window,
					 GdkWindow     *real_parent,
					 GdkScreen     *screen,
					 GdkEventMask   event_mask,
					 GdkWindowAttr *attributes,
					 gint           attributes_mask)
{
  gdk_directfb_window_impl_new (display,
				window,
				real_parent,
				screen,
				event_mask);
}

static GdkKeymap *
gdk_directfb_display_get_keymap (GdkDisplay *display)
{
  GdkDirectfbDisplay *directfb_display = GDK_DIRECTFB_DISPLAY (display);

  if (G_UNLIKELY (! directfb_display->keymap))
    directfb_display->keymap = gdk_directfb_keymap_new (display);

  return directfb_display->keymap;
}

static GdkWindow *
gdk_directfb_display_get_selection_owner (GdkDisplay *display,
					  GdkAtom     selection)
{
  GdkDirectfbDisplay *directfb_display = GDK_DIRECTFB_DISPLAY (display);
  GdkWindow *owner = directfb_display->selection_owner;

  if (selection == GDK_SELECTION_CLIPBOARD)
    {
      GdkScreen *screen = gdk_display_get_screen (display, 0);
      gpointer user_data = NULL;

      owner = gdk_screen_get_root_window (screen);
      gdk_window_get_user_data (owner, &user_data);

      if (! user_data)
        {
	  user_data = g_object_get_data (G_OBJECT(display),
					 "gtk-clipboard-widget");
          gdk_window_set_user_data (owner, user_data);
        }
    }

  return owner;
}

static gboolean
gdk_directfb_display_set_selection_owner (GdkDisplay *display,
					  GdkWindow  *owner,
					  GdkAtom     selection,
					  guint32     time,
					  gboolean    send_event)
{
  GdkDirectfbDisplay *directfb_display = GDK_DIRECTFB_DISPLAY (display);

  directfb_display->selection_owner = owner;

  return TRUE;
}

static void
gdk_directfb_display_send_selection_notify (GdkDisplay *display,
					    GdkWindow  *requestor,
					    GdkAtom     selection,
					    GdkAtom     target,
					    GdkAtom     property,
					    guint32     time)
{
}

static gint
gdk_directfb_display_get_selection_property (GdkDisplay *display,
					     GdkWindow  *requestor,
					     guchar    **data,
					     GdkAtom    *ret_type,
					     gint       *ret_format)
{
  return 0;
}

static void
gdk_directfb_display_convert_selection (GdkDisplay *display,
					GdkWindow  *requestor,
					GdkAtom     selection,
					GdkAtom     target,
					guint32     time)
{
}

static gint
make_list (const gchar *text,
	   gint         length,
	   gboolean     latin1,
	   gchar     ***list)
{
  GSList *strings = NULL, *tmp_list;
  const gchar *p = text, *q;
  gint n_strings = 0, i;
  GError *error = NULL;

  while (p < text + length)
    {
      gchar *str;

      q = p;
      while (*q && q < text + length)
        q++;

      if (latin1)
        {
          str = g_convert (p, q - p, "UTF-8", "ISO-8859-1", NULL, NULL, &error);
          if (! str)
            {
              g_warning ("%s", error->message);
              g_error_free (error);
            }
        }
      else
        str = g_strndup (p, q - p);

      if (str)
        {
          strings = g_slist_prepend (strings, str);
          n_strings++;
        }

      p = q + 1;
    }

  if (list)
    *list = g_new0 (gchar *, n_strings + 1);

  i = n_strings;
  tmp_list = strings;
  while (tmp_list)
    {
      if (list)
        (*list)[--i] = tmp_list->data;
      else
        g_free (tmp_list->data);

      tmp_list = tmp_list->next;
    }

  g_slist_free (strings);

  return n_strings;
}

static gint
gdk_directfb_display_text_property_to_utf8_list (GdkDisplay   *display,
						 GdkAtom       encoding,
						 gint          format,
						 const guchar *text,
						 gint          length,
						 gchar      ***list)
{
  g_return_val_if_fail (text != NULL, 0);
  g_return_val_if_fail (length >= 0, 0);

  if (encoding == GDK_TARGET_STRING)
    return make_list ((gchar *) text, length, TRUE, list);
  else if (encoding == gdk_atom_intern_static_string ("UTF8_STRING"))
    return make_list ((gchar *) text, length, FALSE, list);
  else
    {
      gchar *enc_name = gdk_atom_name (encoding);

      g_warning ("encoding %s not handled\n", enc_name);
      g_free (enc_name);

      if (list)
        *list = NULL;

      return 0;
    }
}

static gchar *
gdk_directfb_display_utf8_to_string_target (GdkDisplay  *display,
					    const gchar *str)
{
  return NULL;
}

static void
gdk_directfb_display_class_init (GdkDirectfbDisplayClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);
  GdkDisplayClass *display_class = GDK_DISPLAY_CLASS (klass);

  object_class->dispose = gdk_directfb_display_dispose;
  object_class->finalize = gdk_directfb_display_finalize;

  display_class->window_type = GDK_TYPE_DIRECTFB_WINDOW;
  display_class->get_name = gdk_directfb_display_get_name;
  display_class->get_default_screen = gdk_directfb_display_get_default_screen;
  display_class->beep = gdk_directfb_display_beep;
  display_class->sync = gdk_directfb_display_sync;
  display_class->flush = gdk_directfb_display_flush;
  display_class->has_pending = gdk_directfb_display_has_pending;
  display_class->queue_events = gdk_directfb_display_queue_events;
  display_class->get_default_group = gdk_directfb_display_get_default_group;
  display_class->supports_selection_notification = gdk_directfb_display_supports_selection_notification;
  display_class->request_selection_notification = gdk_directfb_display_request_selection_notification;
  display_class->supports_clipboard_persistence = gdk_directfb_display_supports_clipboard_persistence;
  display_class->store_clipboard = gdk_directfb_display_store_clipboard;
  display_class->supports_shapes = gdk_directfb_display_supports_shapes;
  display_class->supports_input_shapes = gdk_directfb_display_supports_input_shapes;
  display_class->supports_composite = gdk_directfb_display_supports_composite;
  display_class->list_devices = gdk_directfb_display_list_devices;
  display_class->get_cursor_for_type = gdk_directfb_display_get_cursor_for_type;
  display_class->get_cursor_for_name = gdk_directfb_display_get_cursor_for_name;
  display_class->get_cursor_for_surface = gdk_directfb_display_get_cursor_for_surface;
  display_class->get_default_cursor_size = gdk_directfb_display_get_default_cursor_size;
  display_class->get_maximal_cursor_size = gdk_directfb_display_get_maximal_cursor_size;
  display_class->supports_cursor_alpha = gdk_directfb_display_supports_cursor_alpha;
  display_class->supports_cursor_color = gdk_directfb_display_supports_cursor_color;
  display_class->before_process_all_updates = gdk_directfb_display_before_process_all_updates;
  display_class->after_process_all_updates = gdk_directfb_display_after_process_all_updates;
  display_class->get_next_serial = gdk_directfb_display_get_next_serial;
  display_class->notify_startup_complete = gdk_directfb_display_notify_startup_complete;
  display_class->event_data_copy = gdk_directfb_display_event_data_copy;
  display_class->event_data_free = gdk_directfb_display_event_data_free;
  display_class->create_window_impl = gdk_directfb_display_create_window_impl;
  display_class->get_keymap = gdk_directfb_display_get_keymap;
  display_class->get_selection_owner = gdk_directfb_display_get_selection_owner;
  display_class->set_selection_owner = gdk_directfb_display_set_selection_owner;
  display_class->send_selection_notify = gdk_directfb_display_send_selection_notify;
  display_class->get_selection_property = gdk_directfb_display_get_selection_property;
  display_class->convert_selection = gdk_directfb_display_convert_selection;
  display_class->text_property_to_utf8_list = gdk_directfb_display_text_property_to_utf8_list;
  display_class->utf8_to_string_target = gdk_directfb_display_utf8_to_string_target;
}

/**
 * _gdk_directfb_display_open:
 * @display_name: (allow-none): The display/screen to open
 *
 * Attempts to open the specified display. The display name is an integer
 * representing the screen ID that DirectFB should use. If the screen ID is
 * not specified then the primary screen ID (#DSCID_PRIMARY) is used.
 *
 * Returns: (transfer full): A new #GdkDisplay
 *
 * Since: 3.10
 */
G_GNUC_INTERNAL
GdkDisplay *
_gdk_directfb_display_open (const gchar *display_name)
{
  DFBResult result;
  GdkDisplay *display = NULL;
  DFBScreenID id = DSCID_PRIMARY;
  GdkDirectfbDisplay *directfb_display;

  if (display_name)
    {
      gchar *end = NULL;

      errno = 0;
      id = g_ascii_strtoull (display_name, &end, 0);

      if (errno)
	{
	  g_warning ("%s: %s", display_name, g_strerror (errno));
	  goto error;
	}

      if (id > G_MAXUINT)
	{
	  g_warning ("%s: invalid display name", display_name);
	  goto error;
	}


      if (display_name == end)
	{
	  g_warning ("%s: invalid display name", display_name);
	  goto error;
	}
    }

  display = g_object_new (GDK_TYPE_DIRECTFB_DISPLAY, NULL);
  directfb_display = GDK_DIRECTFB_DISPLAY (display);

  result = DirectFBInit (NULL, NULL);
  if (G_UNLIKELY (DFB_OK != result))
    {
      g_warning ("%s", DirectFBErrorString (result));
      goto error;
    }

  result = DirectFBCreate (&directfb_display->dfb);
  if (G_UNLIKELY (DFB_OK != result))
    {
      g_warning ("%s", DirectFBErrorString (result));
      goto error;
    }

  result = directfb_display->dfb->CreateEventBuffer (directfb_display->dfb,
						     &directfb_display->buffer);
  if (G_UNLIKELY (DFB_OK != result))
    {
      g_warning ("%s", DirectFBErrorString (result));
      goto error;
    }

  directfb_display->screen = gdk_directfb_screen_new (display, id);
  if (G_UNLIKELY (! directfb_display->screen))
    goto error;

  display->device_manager = gdk_directfb_device_manager_new (display);
  if (G_UNLIKELY (! display->device_manager))
    goto error;

  directfb_display->source = gdk_directfb_source_new (display);
  if (G_UNLIKELY (! directfb_display->source))
    goto error;

  g_signal_emit_by_name (display, "opened");

  return display;

error:

  if (display)
    g_object_unref (display);

  return NULL;
}

/**
 * gdk_directfb_display_set_cursor_theme:
 * @display: A #GdkDisplay object
 * @theme (allow-none): The name of the Xcursor theme to set
 * @size: The desired nomial size
 *
 * Set an Xcursor theme for the specified display. If no theme name is
 * specified then the "default" theme is loaded.
 *
 * Since: 3.10
 */
void
gdk_directfb_display_set_cursor_theme (GdkDisplay  *display,
				       const gchar *theme,
				       const gint   size)
{
  GdkDirectfbDisplay *directfb_display;

  g_return_if_fail (GDK_IS_DIRECTFB_DISPLAY (display));

  directfb_display = GDK_DIRECTFB_DISPLAY (display);

  if (G_LIKELY (directfb_display->cursor_theme))
    g_object_unref (directfb_display->cursor_theme);

  directfb_display->cursor_theme =
    gdk_xcursor_cursor_theme_new (display, theme, size, NULL);
}

/**
 * gdk_directfb_display_get_context:
 * @display: A #GdkDisplay object
 *
 * Get the DirectFB context for the specified display.
 *
 * Returns: (transfer none): A #IDirectFB object
 *
 * Since: 3.10
 */
IDirectFB *
gdk_directfb_display_get_context (GdkDisplay *display)
{
  g_return_val_if_fail (GDK_IS_DIRECTFB_DISPLAY (display), NULL);

  return GDK_DIRECTFB_DISPLAY (display)->dfb;
}

/**
 * gdk_directfb_display_get_event_buffer:
 * @display: A #GdkDisplay object
 *
 * Get the DirectFB event buffer for the specified display.
 *
 * Returns: (transfer none): A #IDirectFBEventBuffer object
 *
 * Since: 3.10
 */
IDirectFBEventBuffer *
gdk_directfb_display_get_event_buffer (GdkDisplay *display)
{
  g_return_val_if_fail (GDK_IS_DIRECTFB_DISPLAY (display), NULL);

  return GDK_DIRECTFB_DISPLAY (display)->buffer;
}

/**
 * gdk_directfb_display_get_cursor_theme:
 * @display: A #GdkDisplay object
 *
 * Get the currently active cursor theme for the specified display.
 *
 * Returns: (transfer none): A #GdkCursorTheme object
 *
 * Since: 3.10
 */
GdkCursorTheme *
gdk_directfb_display_get_cursor_theme (GdkDisplay *display)
{
  GdkDirectfbDisplay *directfb_display;

  g_return_val_if_fail (GDK_IS_DIRECTFB_DISPLAY (display), NULL);

  directfb_display = GDK_DIRECTFB_DISPLAY (display);

  if (G_UNLIKELY (! directfb_display->cursor_theme))
    directfb_display->cursor_theme =
      gdk_xcursor_cursor_theme_new (display, NULL, 0, NULL);

  return directfb_display->cursor_theme;
}

/**
 * gdk_directfb_display_window_destroyed:
 * @display: A #GdkDisplay object
 * @window: A #GdkWindow object
 *
 * Notify the display that a window has been destroyed so that it can remove
 * any reference to the current selection window.
 *
 * Since: 3.10
 */
void
gdk_directfb_display_window_destroyed (GdkDisplay *display,
				       GdkWindow  *window)
{
  GdkDirectfbDisplay *directfb_display;

  g_return_if_fail (GDK_IS_DIRECTFB_DISPLAY (display));

  directfb_display = GDK_DIRECTFB_DISPLAY (display);

  if (directfb_display->selection_owner == window)
    directfb_display->selection_owner = NULL;
}
