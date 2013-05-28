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

#include "gdkdevice-directfb.h"
#include "gdkdevicemanager-directfb.h"
#include "gdkkeymap-directfb.h"
#include "gdkkeysyms.h"
#include "gdkscreen-directfb.h"
#include "gdksource-directfb.h"
#include "gdkwindow-directfb.h"

#define EVENT_BUFSIZ (BUFSIZ - (BUFSIZ % sizeof (DFBEvent)))

typedef struct GdkDirectfbSource_ GdkDirectfbSource;

struct GdkDirectfbSource_
{
  GSource source;
  GdkDisplay *display;
  GIOChannel *channel;
  DFBEvent *event_buf;
  guint next_serial;
  GPollFD event_poll_fd;
};

/**
 * gdk_directfb_source_key:
 * @source: A #GdkSource object
 * @type: The type of event to create
 * @dfb_event: A #DFBWindowEvent object
 *
 * Creates a new #GDK_KEY_PRESS or #GDK_KEY_RELEASE event.
 *
 * Returns: (transfer full): A new #GdkEvent
 *
 * Since: 3.10
 */
static GdkEvent *
gdk_directfb_source_key (GSource        *source,
			 GdkEventType    type,
			 DFBWindowEvent *dfb_event)
{
  GdkDirectfbSource *directfb_source = (GdkDirectfbSource *) source;
  GdkDeviceManager *manager;
  GdkDevice *device;
  GdkScreen *screen;
  GdkWindow *window;
  GdkEvent *event;
  gchar outbuf[6];

  screen = gdk_display_get_default_screen (directfb_source->display);
  window = gdk_directfb_screen_get_window (screen, dfb_event->window_id);
  if (! window)
    return NULL;

  event = gdk_event_new (type);

  event->key.window = g_object_ref (window);
  event->key.send_event = FALSE;
  event->key.time = (dfb_event->timestamp.tv_sec * 1000) +
    (dfb_event->timestamp.tv_usec / 1000);
  event->key.state = gdk_directfb_modifier_mask (dfb_event->modifiers,
						 dfb_event->locks,
						 dfb_event->buttons);
  event->key.keyval = gdk_directfb_translate_key (dfb_event->key_id,
						  dfb_event->key_symbol);
  event->key.length = g_unichar_to_utf8 (dfb_event->key_symbol, outbuf);
  event->key.string = g_strndup (outbuf, event->key.length);
  event->key.group = (dfb_event->modifiers & DIMM_ALTGR) ? 1 : 0;

  if (G_LIKELY (-1 != dfb_event->key_code))
      event->key.hardware_keycode = dfb_event->key_code;
  else
    {
      GdkKeymap *keymap;

      keymap = gdk_keymap_get_for_display (directfb_source->display);

      event->key.hardware_keycode =
	gdk_directfb_keymap_lookup_hardware_keycode (keymap, event->key.keyval);
    }

  manager = gdk_display_get_device_manager (directfb_source->display);
  device = gdk_directfb_device_manager_get_core_keyboard (manager);
  gdk_event_set_device (event, device);

  gdk_event_set_screen (event, screen);

  return event;
}

/**
 * gdk_directfb_source_button:
 * @source: A #GdkSource object
 * @type: The type of event to create
 * @dfb_event: A #DFBWindowEvent object
 *
 * Creates a new #GDK_BUTTON_PRESS or #GDK_BUTTON_RELEASE event.
 *
 * Returns: (transfer full): A new #GdkEvent
 *
 * Since: 3.10
 */
static GdkEvent *
gdk_directfb_source_button (GSource        *source,
			    GdkEventType    type,
			    DFBWindowEvent *dfb_event)
{
  GdkDirectfbSource *directfb_source = (GdkDirectfbSource *) source;
  GdkDeviceManager *manager;
  GdkScreen *screen;
  GdkWindow *window;
  GdkEvent *event;

  screen = gdk_display_get_default_screen (directfb_source->display);
  window = gdk_directfb_screen_get_window (screen, dfb_event->window_id);
  if (! window)
    return NULL;

  event = gdk_event_new (type);

  event->button.window = g_object_ref (window);
  event->button.send_event = FALSE;
  event->button.time = (dfb_event->timestamp.tv_sec * 1000) +
    (dfb_event->timestamp.tv_usec / 1000);
  event->button.x = dfb_event->x;
  event->button.y = dfb_event->y;
  event->button.axes = NULL;
  event->button.state = gdk_directfb_modifier_mask (dfb_event->modifiers,
						    dfb_event->locks,
						    dfb_event->buttons);

  switch (dfb_event->button)
    {
    case DIBI_LEFT:
      event->button.button = GDK_BUTTON_PRIMARY;
      break;

    case DIBI_MIDDLE:
      event->button.button = GDK_BUTTON_MIDDLE;
      break;

    case DIBI_RIGHT:
      event->button.button = GDK_BUTTON_SECONDARY;
      break;

    default:
      event->button.button = dfb_event->button + 1;
      break;
    }

  manager = gdk_display_get_device_manager (directfb_source->display);
  event->button.device = gdk_directfb_device_manager_get_core_pointer (manager);

  event->button.x_root = dfb_event->cx;
  event->button.y_root = dfb_event->cy;

  gdk_event_set_screen (event, screen);

  return event;
}

/**
 * gdk_directfb_source_scroll:
 * @source: A #GdkSource object
 * @dfb_event: A #DFBWindowEvent object
 *
 * Creates a new #GDK_SCROLL event.
 *
 * Returns: (transfer full): A new #GdkEvent
 *
 * Since: 3.10
 */
static GdkEvent *
gdk_directfb_source_scroll (GSource        *source,
			    DFBWindowEvent *dfb_event)
{
  GdkDirectfbSource *directfb_source = (GdkDirectfbSource *) source;
  GdkDeviceManager *manager;
  GdkScreen *screen;
  GdkWindow *window;
  GdkEvent *event;

  screen = gdk_display_get_default_screen (directfb_source->display);
  window = gdk_directfb_screen_get_window (screen, dfb_event->window_id);
  if (! window)
    return NULL;

  event = gdk_event_new (GDK_SCROLL);

  event->scroll.window = g_object_ref (window);
  event->scroll.send_event = FALSE;
  event->scroll.time = (dfb_event->timestamp.tv_sec * 1000) +
    (dfb_event->timestamp.tv_usec / 1000);
  event->scroll.x = dfb_event->x;
  event->scroll.y = dfb_event->y;
  event->scroll.state = gdk_directfb_modifier_mask (dfb_event->modifiers,
						    dfb_event->locks,
						    dfb_event->buttons);
  event->scroll.direction = GDK_SCROLL_SMOOTH;

  manager = gdk_display_get_device_manager (directfb_source->display);
  event->scroll.device = gdk_directfb_device_manager_get_core_pointer (manager);

  event->scroll.x_root = dfb_event->cx;
  event->scroll.y_root = dfb_event->cy;
  event->scroll.delta_x = 0;
  event->scroll.delta_y = -dfb_event->step;

  gdk_event_set_screen (event, screen);

  return event;
}

/**
 * gdk_directfb_source_motion:
 * @source: A #GdkSource object
 * @dfb_event: A #DFBWindowEvent object
 *
 * Creates a new #GDK_MOTION_NOTIFY event.
 *
 * Returns: (transfer full): A new #GdkEvent
 *
 * Since: 3.10
 */
static GdkEvent *
gdk_directfb_source_motion (GSource        *source,
			    DFBWindowEvent *dfb_event)
{
  GdkDirectfbSource *directfb_source = (GdkDirectfbSource *) source;
  GdkDeviceManager *manager;
  GdkWindow *window;
  GdkScreen *screen;
  GdkEvent *event;

  screen = gdk_display_get_default_screen (directfb_source->display);
  window = gdk_directfb_screen_get_window (screen, dfb_event->window_id);
  if (! window)
    return NULL;

  event = gdk_event_new (GDK_MOTION_NOTIFY);

  event->motion.window = g_object_ref (window);
  event->motion.send_event = FALSE;
  event->motion.time = (dfb_event->timestamp.tv_sec * 1000) +
    (dfb_event->timestamp.tv_usec / 1000);
  event->motion.x = dfb_event->x;
  event->motion.y = dfb_event->y;
  event->motion.axes = NULL;
  event->motion.state = gdk_directfb_modifier_mask (dfb_event->modifiers,
						    dfb_event->locks,
						    dfb_event->buttons);
  event->motion.is_hint = 0;

  manager = gdk_display_get_device_manager (directfb_source->display);
  event->motion.device = gdk_directfb_device_manager_get_core_pointer (manager);

  event->motion.x_root = dfb_event->cx;
  event->motion.y_root = dfb_event->cy;

  gdk_directfb_device_set_position (event->motion.device,
				    event->motion.x_root, event->motion.y_root,
				    event->motion.x, event->motion.y);

  gdk_event_set_screen (event, screen);

  return event;
}

/**
 * gdk_directfb_source_focus_change:
 * @source: A #GdkSource object
 * @dfb_event: A #DFBWindowEvent object
 *
 * Creates a new #GDK_FOCUS_CHANGE event.
 *
 * Returns: (transfer full): A new #GdkEvent
 *
 * Since: 3.10
 */
static GdkEvent *
gdk_directfb_source_focus_change (GSource        *source,
				  gboolean        in,
				  DFBWindowEvent *dfb_event)
{
  GdkDirectfbSource *directfb_source = (GdkDirectfbSource *) source;
  GdkDeviceManager *manager;
  GdkDevice *device;
  GdkScreen *screen;
  GdkWindow *window;
  GdkEvent *event;

  screen = gdk_display_get_default_screen (directfb_source->display);
  window = gdk_directfb_screen_get_window (screen, dfb_event->window_id);
  if (! window)
    return NULL;

  event = gdk_event_new (GDK_FOCUS_CHANGE);

  event->focus_change.window = g_object_ref (window);
  event->focus_change.send_event = FALSE;
  event->focus_change.in = in;

  manager = gdk_display_get_device_manager (directfb_source->display);
  device = gdk_directfb_device_manager_get_core_pointer (manager);
  gdk_event_set_device (event, device);

  gdk_event_set_screen (event, screen);

  return event;
}

/**
 * gdk_directfb_source_configure:
 * @source: A #GdkSource object
 * @dfb_event: A #DFBWindowEvent object
 *
 * Creates a new #GDK_CONFIGURE event.
 *
 * Returns: (transfer full): A new #GdkEvent
 *
 * Since: 3.10
 */
static GdkEvent *
gdk_directfb_source_configure (GSource        *source,
			       gboolean        position,
			       gboolean        size,
			       DFBWindowEvent *dfb_event)
{
  GdkDirectfbSource *directfb_source = (GdkDirectfbSource *) source;
  GdkScreen *screen;
  GdkWindow *window;
  GdkEvent *event;

  screen = gdk_display_get_default_screen (directfb_source->display);
  window = gdk_directfb_screen_get_window (screen, dfb_event->window_id);
  if (! window)
    return NULL;

  event = gdk_event_new (GDK_CONFIGURE);

  event->configure.window = g_object_ref (window);
  event->configure.send_event = FALSE;

  if (position)
    {
      event->configure.x = dfb_event->x;
      event->configure.y = dfb_event->y;
    }
  else
    {
      event->configure.x = window->x;
      event->configure.y = window->y;
    }

  if (size)
    {
      event->configure.width = dfb_event->w;
      event->configure.height = dfb_event->h;
    }
  else
    {
      event->configure.width = window->width;
      event->configure.height = window->height;
    }

  gdk_event_set_screen (event, screen);

  return event;
}

/**
 * gdk_directfb_source_crossing:
 * @source: A #GdkSource object
 * @type: The type of event to create
 * @dfb_event: A #DFBWindowEvent object
 *
 * Creates a new #GDK_CROSSING event.
 *
 * Returns: (transfer full): A new #GdkEvent
 *
 * Since: 3.10
 */
static GdkEvent *
gdk_directfb_source_crossing (GSource        *source,
			      GdkEventType    type,
			      DFBWindowEvent *dfb_event)
{
  GdkDirectfbSource *directfb_source = (GdkDirectfbSource *) source;
  GdkDeviceManager *manager;
  GdkDevice *device;
  GdkScreen *screen;
  GdkWindow *window;
  GdkEvent *event;

  screen = gdk_display_get_default_screen (directfb_source->display);
  window = gdk_directfb_screen_get_window (screen, dfb_event->window_id);
  if (! window)
    return NULL;

  event = gdk_event_new (type);

  event->crossing.window = g_object_ref (window);
  event->crossing.send_event = FALSE;
  event->crossing.subwindow = NULL;
  event->crossing.time = (dfb_event->timestamp.tv_sec * 1000) +
    (dfb_event->timestamp.tv_usec / 1000);
  event->crossing.x = dfb_event->x;
  event->crossing.y = dfb_event->y;
  event->crossing.x_root = dfb_event->cx;
  event->crossing.y_root = dfb_event->cy;
  event->crossing.mode = GDK_CROSSING_NORMAL;
  event->crossing.detail = GDK_NOTIFY_ANCESTOR;
  event->crossing.focus = TRUE;
  event->crossing.state = gdk_directfb_modifier_mask (dfb_event->modifiers,
						      dfb_event->locks,
						      dfb_event->buttons);

  manager = gdk_display_get_device_manager (directfb_source->display);
  device = gdk_directfb_device_manager_get_core_pointer (manager);
  gdk_event_set_device (event, device);

  if (GDK_ENTER_NOTIFY == type)
    gdk_directfb_device_set_focus_window (device, window);
  else
    gdk_directfb_device_set_focus_window (device, NULL);

  gdk_event_set_screen (event, screen);

  return event;
}

/**
 * gdk_directfb_source_delete:
 * @source: A #GdkSource object
 * @dfb_event: A #DFBWindowEvent object
 *
 * Creates a new #GDK_DELETE event.
 *
 * Returns: (transfer full): A new #GdkEvent
 *
 * Since: 3.10
 */
static GdkEvent *
gdk_directfb_source_delete (GSource        *source,
			    DFBWindowEvent *dfb_event)
{
  GdkDirectfbSource *directfb_source = (GdkDirectfbSource *) source;
  GdkScreen *screen;
  GdkWindow *window;
  GdkEvent *event;

  screen = gdk_display_get_default_screen (directfb_source->display);
  window = gdk_directfb_screen_get_window (screen, dfb_event->window_id);
  if (! window)
    return NULL;

  event = gdk_event_new (GDK_DELETE);

  event->any.window = g_object_ref (window);
  event->any.send_event = FALSE;

  gdk_event_set_screen (event, screen);

  return event;
}

/**
 * gdk_directfb_source_destroy:
 * @source: A #GdkSource object
 * @dfb_event: A #DFBWindowEvent object
 *
 * Creates a new #GDK_DESTROY event.
 *
 * Returns: (transfer full): A new #GdkEvent
 *
 * Since: 3.10
 */
static GdkEvent *
gdk_directfb_source_destroy (GSource        *source,
			     DFBWindowEvent *dfb_event)
{
  GdkDirectfbSource *directfb_source = (GdkDirectfbSource *) source;
  GdkScreen *screen;
  GdkWindow *window;
  GdkEvent *event;

  screen = gdk_display_get_default_screen (directfb_source->display);
  window = gdk_directfb_screen_get_window (screen, dfb_event->window_id);
  if (! window)
    return NULL;

  event = gdk_event_new (GDK_DESTROY);

  event->any.window = g_object_ref (window);
  event->any.send_event = FALSE;

  gdk_event_set_screen (event, screen);

  return event;
}

static gboolean
gdk_directfb_source_prepare (GSource *source,
			     gint    *timeout)
{
  GdkDisplay *display = ((GdkDirectfbSource *) source)->display;
  gboolean retval;

  *timeout = -1;

  if (display->event_pause_count > 0)
    retval = FALSE;

  if (_gdk_event_queue_find_first (display) != NULL)
    return TRUE;

  return FALSE;
}

static gboolean
gdk_directfb_source_check (GSource *source)
{
  GdkDirectfbSource *directfb_source = (GdkDirectfbSource *) source;

  if (directfb_source->display->event_pause_count > 0)
    return FALSE;

  return _gdk_event_queue_find_first (directfb_source->display) != NULL ||
    directfb_source->event_poll_fd.revents;
}

static gboolean
gdk_directfb_source_dispatch (GSource     *source,
			      GSourceFunc  callback,
			      gpointer     user_data)
{
  GdkDirectfbSource *directfb_source = (GdkDirectfbSource *) source;
  GError *err = NULL;
  GIOStatus status;
  GdkEvent *event;
  gsize read, i;

  if (! directfb_source->event_poll_fd.revents & G_IO_IN)
    goto exit;

  status = g_io_channel_read_chars (directfb_source->channel,
				    (gchar *) directfb_source->event_buf,
				    EVENT_BUFSIZ,
				    &read, &err);
  switch (status)
    {
    case G_IO_STATUS_NORMAL:
      break;

    case G_IO_STATUS_AGAIN:
      goto exit;

    case G_IO_STATUS_EOF:
      g_warning ("DirectFB connection closed");
      goto exit;

    case G_IO_STATUS_ERROR:
      g_warning ("%s", err->message);
      g_error_free (err);
      goto exit;
    }

  read /= sizeof (DFBEvent);
  for (i = 0; i < read; ++i)
    {
      GList *node;
      DFBWindowEvent *dfb_event;

      if (DFEC_WINDOW != directfb_source->event_buf[i].clazz)
	continue;

      dfb_event = (DFBWindowEvent *) &directfb_source->event_buf[i];
      switch (dfb_event->type)
	{
	case DWET_KEYDOWN:
	  event = gdk_directfb_source_key (source, GDK_KEY_PRESS, dfb_event);
	  break;

	case DWET_KEYUP:
	  event = gdk_directfb_source_key (source, GDK_KEY_RELEASE, dfb_event);
	  break;

	case DWET_BUTTONDOWN:
	  event = gdk_directfb_source_button (source, GDK_BUTTON_PRESS,
					      dfb_event);
	  break;

	case DWET_BUTTONUP:
	  event = gdk_directfb_source_button (source, GDK_BUTTON_RELEASE,
					      dfb_event);
	  break;

	case DWET_WHEEL:
	  event = gdk_directfb_source_scroll (source, dfb_event);
	  break;

	case DWET_MOTION:
	  event = gdk_directfb_source_motion (source, dfb_event);
	  break;

	case DWET_GOTFOCUS:
	  event = gdk_directfb_source_focus_change (source, TRUE, dfb_event);
	  break;

	case DWET_LOSTFOCUS:
	  event = gdk_directfb_source_focus_change (source, FALSE, dfb_event);
	  break;

	case DWET_POSITION_SIZE:
	  event = gdk_directfb_source_configure (source, TRUE, TRUE,
						 dfb_event);
	  break;

	case DWET_POSITION:
	  event = gdk_directfb_source_configure (source, TRUE, FALSE,
						 dfb_event);
	  break;

	case DWET_SIZE:
	  event = gdk_directfb_source_configure (source, FALSE, TRUE,
						 dfb_event);
	  break;

	case DWET_ENTER:
	  event = gdk_directfb_source_crossing (source, GDK_ENTER_NOTIFY,
						dfb_event);
	  break;

	case DWET_LEAVE:
	  event = gdk_directfb_source_crossing (source, GDK_LEAVE_NOTIFY,
						dfb_event);
	  break;

	case DWET_CLOSE:
	  event = gdk_directfb_source_delete (source, dfb_event);
	  break;

	case DWET_DESTROYED:
	  event = gdk_directfb_source_destroy (source, dfb_event);
	  break;

	default:
	  continue;
	}

      if (! event)
	continue;

      node = _gdk_event_queue_append (directfb_source->display, event);
      _gdk_windowing_got_event (directfb_source->display, node, event,
				directfb_source->next_serial++);
    }

exit:

  event = gdk_display_get_event (directfb_source->display);

  if (event)
    {
      _gdk_event_emit (event);
      gdk_event_free (event);
    }

  return TRUE;
}

static void
gdk_directfb_source_finalize (GSource *source)
{
  GdkDirectfbSource *directfb_source = (GdkDirectfbSource *) source;

  g_io_channel_unref (directfb_source->channel);
  g_free (directfb_source->event_buf);
}

/**
 * gdk_directfb_source_new:
 * @display: The #GdkDisplay for which this source is being created
 *
 * Creates a new event source that translates DirectFB events into GDK events.
 *
 * Returns: (transfer full): A new #GSource object
 *
 * Since: 3.10
 */
GSource *
gdk_directfb_source_new (GdkDisplay *display)
{
  GSource *source;
  GdkDirectfbSource *directfb_source;
  IDirectFBEventBuffer *buffer;
  DFBResult result;
  gchar *name;
  gint fd;

  static GSourceFuncs event_funcs =
    {
      gdk_directfb_source_prepare,
      gdk_directfb_source_check,
      gdk_directfb_source_dispatch,
      gdk_directfb_source_finalize
    };

  buffer = gdk_directfb_display_get_event_buffer (display);
  result = buffer->CreateFileDescriptor (buffer, &fd);
  if (G_UNLIKELY (DFB_OK != result))
    {
      g_warning ("%s", DirectFBErrorString (result));
      return NULL;
    }

  source = g_source_new (&event_funcs, sizeof (GdkDirectfbSource));
  name = g_strdup_printf ("GDK DirectFB Event source (%s)",
			  gdk_display_get_name (display));
  g_source_set_name (source, name);
  g_free (name);
  directfb_source = (GdkDirectfbSource *) source;
  directfb_source->display = display;
  directfb_source->event_buf = g_malloc0 (EVENT_BUFSIZ);
  directfb_source->next_serial = 1;

  directfb_source->channel = g_io_channel_unix_new (fd);
  g_io_channel_set_encoding (directfb_source->channel, NULL, NULL);
  g_io_channel_set_buffered (directfb_source->channel, FALSE);

  directfb_source->event_poll_fd.fd = fd;
  directfb_source->event_poll_fd.events = G_IO_IN;
  g_source_add_poll (source, &directfb_source->event_poll_fd);

  g_source_set_priority (source, GDK_PRIORITY_EVENTS);
  g_source_set_can_recurse (source, TRUE);
  g_source_attach (source, NULL);

  return source;
}

/**
 * gdk_directfb_source_get_next_serial:
 * @source: A #GSource object
 *
 * Get the serial number for the next event.
 *
 * Returns: A serial number
 *
 * Since: 3.10
 */
guint
gdk_directfb_source_get_next_serial (GSource *source)
{
  GdkDirectfbSource *directfb_source = (GdkDirectfbSource *) source;

  return directfb_source->next_serial;
}
