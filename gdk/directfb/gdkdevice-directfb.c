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
#include "gdkcursortheme.h"
#include "gdkdevice-directfb.h"
#include "gdkdisplay-directfb.h"
#include "gdkprivate-directfb.h"
#include "gdkscreen-directfb.h"
#include "gdkwindow-directfb.h"

G_DEFINE_TYPE (GdkDirectfbDevice, gdk_directfb_device, GDK_TYPE_DEVICE)

static void
gdk_directfb_device_init (GdkDirectfbDevice *device)
{
}

static void
gdk_directfb_device_dispose (GObject *object)
{
  GdkDirectfbDevice *directfb_device = GDK_DIRECTFB_DEVICE (object);

  if (directfb_device->grab_window)
    g_object_unref (directfb_device->grab_window);

  if (directfb_device->focus_window)
    g_object_unref (directfb_device->focus_window);

  if (directfb_device->cursor)
    g_object_unref (directfb_device->cursor);

  G_OBJECT_CLASS (gdk_directfb_device_parent_class)->dispose (object);

  directfb_device->grab_window = NULL;
  directfb_device->focus_window = NULL;
  directfb_device->cursor = NULL;
}

static void
gdk_directfb_device_finalize (GObject *object)
{
  GdkDirectfbDevice *directfb_device = GDK_DIRECTFB_DEVICE (object);

  if (G_LIKELY (directfb_device->input_device))
    {
      (void) directfb_device->input_device->
	Release (directfb_device->input_device);
    }

  G_OBJECT_CLASS (gdk_directfb_device_parent_class)->finalize (object);
}

static GdkModifierType
gdk_directfb_device_modifier_mask (GdkDevice *device)
{
  GdkDirectfbDevice *directfb_device = GDK_DIRECTFB_DEVICE (device);
  GdkModifierType mask = 0;

  if (G_LIKELY (directfb_device->input_device))
    {
      DFBInputDeviceModifierMask modifiers;
      DFBInputDeviceLockState locks;
      DFBInputDeviceButtonMask buttons;

      (void) directfb_device->input_device->
	GetModifiers (directfb_device->input_device, &modifiers);

      (void) directfb_device->input_device->
	GetLockState (directfb_device->input_device, &locks);

      (void) directfb_device->input_device->
	GetButtons (directfb_device->input_device, &buttons);

      mask = gdk_directfb_modifier_mask (modifiers, locks, buttons);
    }

  return mask;
}

static void
gdk_directfb_device_get_state (GdkDevice       *device,
			       GdkWindow       *window,
			       gdouble         *axes,
			       GdkModifierType *mask)
{
  if (gdk_device_get_source (device) != GDK_SOURCE_MOUSE)
    return;

  if (axes)
    {
      GdkDirectfbDevice *directfb_device = GDK_DIRECTFB_DEVICE (device);

      axes[0] = directfb_device->win_x;
      axes[1] = directfb_device->win_y;
    }

  if (mask)
    *mask = gdk_directfb_device_modifier_mask (device);
}

static void
gdk_directfb_device_set_window_cursor (GdkDevice *device,
				       GdkWindow *window,
				       GdkCursor *cursor)
{
  GdkDirectfbDevice *directfb_device = GDK_DIRECTFB_DEVICE (device);
  GdkScreen *screen;

  if (gdk_device_get_source (device) != GDK_SOURCE_MOUSE)
    return;

  if (G_LIKELY (cursor))
    (void) g_object_ref (cursor);
  else
    {
      GdkCursorTheme *cursor_theme;

      cursor_theme = gdk_directfb_display_get_cursor_theme (device->display);
      cursor = gdk_cursor_theme_get_cursor (cursor_theme, GDK_LEFT_PTR);
      if (G_UNLIKELY (! cursor))
	return;
    }

  if (G_LIKELY (directfb_device->cursor))
    {
      if (cursor == directfb_device->cursor)
	{
	  g_object_unref (cursor);
	  return;
	}

      gdk_directfb_cursor_deactivate (directfb_device->cursor);
      g_object_unref (directfb_device->cursor);
    }

  screen = gdk_window_get_screen (window);
  gdk_directfb_cursor_activate (cursor, screen);
  directfb_device->cursor = cursor;
}

static void
gdk_directfb_device_warp (GdkDevice *device,
			  GdkScreen *screen,
			  gdouble    x,
			  gdouble    y)
{
  IDirectFBDisplayLayer *layer;
  DFBResult result;

  if (gdk_device_get_source (device) != GDK_SOURCE_MOUSE)
    return;

  layer = gdk_directfb_screen_get_display_layer (screen);
  result = layer->WarpCursor (layer, x, y);
  if (G_UNLIKELY (DFB_OK != result))
    g_warning ("%s", DirectFBErrorString (result));
}

static void
gdk_directfb_device_query_state (GdkDevice       *device,
				 GdkWindow       *window,
				 GdkWindow      **root_window,
				 GdkWindow      **child_window,
				 gdouble         *root_x,
				 gdouble         *root_y,
				 gdouble         *win_x,
				 gdouble         *win_y,
				 GdkModifierType *mask)
{
  GdkDirectfbDevice *directfb_device = GDK_DIRECTFB_DEVICE (device);

  if (gdk_device_get_source (device) != GDK_SOURCE_MOUSE)
    return;

  if (root_window)
    {
      GdkScreen *screen;

      screen = gdk_window_get_screen (window);
      *root_window = gdk_screen_get_root_window (screen);
    }

  if (child_window)
    *child_window = directfb_device->focus_window;

  if (root_x)
    *root_x = directfb_device->x;

  if (root_y)
    *root_y = directfb_device->y;

  if (win_x)
    *win_x = directfb_device->win_x;

  if (win_y)
    *win_y = directfb_device->win_y;

  if (mask)
    *mask = gdk_directfb_device_modifier_mask (device);
}

static GdkGrabStatus
gdk_directfb_device_grab (GdkDevice   *device,
			  GdkWindow   *window,
			  gboolean     owner_events,
			  GdkEventMask event_mask,
			  GdkWindow   *confine_to,
			  GdkCursor   *cursor,
			  guint32      time_)
{
  GdkDirectfbDevice *directfb_device = GDK_DIRECTFB_DEVICE (device);
  DFBResult result = DFB_FAILURE;
  IDirectFBWindow *dfb_window;

  if (directfb_device->grab_window)
    {
      if (directfb_device->grab_window == window)
	return GDK_GRAB_SUCCESS;
      else
	gdk_device_ungrab (device, GDK_CURRENT_TIME);
    }

  dfb_window = gdk_directfb_window_get_window (window);

  switch (gdk_device_get_source (device))
    {
    case GDK_SOURCE_KEYBOARD:
      result = dfb_window->GrabKeyboard (dfb_window);
      break;

    case GDK_SOURCE_MOUSE:
      if (! owner_events)
	result = dfb_window->GrabPointer (dfb_window);
      else
	result = DFB_OK;
      break;

    default:
      return GDK_GRAB_ALREADY_GRABBED;
    }

  if (G_UNLIKELY (DFB_OK != result))
    {
      g_warning ("%s", DirectFBErrorString (result));

      return GDK_GRAB_ALREADY_GRABBED;
    }

  directfb_device->grab_window = g_object_ref (window);
  directfb_device->grab_mask = gdk_window_get_events (window);
  gdk_window_set_events (window, event_mask);

  return GDK_GRAB_SUCCESS;
}

static void
gdk_directfb_device_ungrab (GdkDevice *device,
			    guint32    time_)
{
  GdkDirectfbDevice *directfb_device = GDK_DIRECTFB_DEVICE (device);
  IDirectFBWindow *dfb_window;
  DFBResult result = DFB_OK;

  if (G_UNLIKELY (! directfb_device->grab_window))
      return;

  dfb_window = gdk_directfb_window_get_window (directfb_device->grab_window);

  switch (gdk_device_get_source (device))
    {
    case GDK_SOURCE_KEYBOARD:
      result = dfb_window->UngrabKeyboard (dfb_window);
      break;

    case GDK_SOURCE_MOUSE:
      result = dfb_window->UngrabPointer (dfb_window);
      break;

    default:
      break;
    }

  if (G_UNLIKELY (DFB_OK != result))
    g_warning ("%s", DirectFBErrorString (result));

  gdk_window_set_events (directfb_device->grab_window,
			 directfb_device->grab_mask);

  directfb_device->grab_mask =
    gdk_window_get_events (directfb_device->grab_window);

  g_object_unref (directfb_device->grab_window);
  directfb_device->grab_window = NULL;
}

static GdkWindow *
gdk_directfb_device_window_at_position (GdkDevice       *device,
					gdouble         *win_x,
					gdouble         *win_y,
					GdkModifierType *mask,
					gboolean         get_toplevel)
{
  GdkDirectfbDevice *directfb_device = GDK_DIRECTFB_DEVICE (device);

  if (gdk_device_get_source (device) != GDK_SOURCE_MOUSE)
    return NULL;

  if (win_x)
    *win_x = directfb_device->win_x;

  if (win_y)
    *win_y = directfb_device->win_y;

  if (mask)
    *mask = gdk_directfb_device_modifier_mask (device);

  return directfb_device->focus_window;
}

static void
gdk_directfb_device_select_window_events (GdkDevice   *device,
					  GdkWindow   *window,
					  GdkEventMask event_mask)
{
  GDK_DIRECTFB_DEVICE (device)->event_mask = event_mask;
}

static void
gdk_directfb_device_class_init (GdkDirectfbDeviceClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);
  GdkDeviceClass *device_class = GDK_DEVICE_CLASS (klass);

  object_class->dispose = gdk_directfb_device_dispose;
  object_class->finalize = gdk_directfb_device_finalize;

  device_class->get_state = gdk_directfb_device_get_state;
  device_class->set_window_cursor = gdk_directfb_device_set_window_cursor;
  device_class->warp = gdk_directfb_device_warp;
  device_class->query_state = gdk_directfb_device_query_state;
  device_class->grab = gdk_directfb_device_grab;
  device_class->ungrab = gdk_directfb_device_ungrab;
  device_class->window_at_position = gdk_directfb_device_window_at_position;
  device_class->select_window_events = gdk_directfb_device_select_window_events;
}

/**
 * gdk_directfb_keyboard_new:
 * @display: The display for which this keybaord device is being created
 * @device_manager: The device manager this device is associated with
 * @input_device: (allow-none): The #IDirectFBInputDevice for this device
 * @name: (allow-none): The name of the input device
 *
 * Creates a new #GdkDirectfbDevice.
 *
 * Returns: (transfer full): A new #GdkDevice
 *
 * Since: 3.10
 */
GdkDevice *
gdk_directfb_keyboard_new (GdkDisplay           *display,
			   GdkDeviceManager     *device_manager,
			   IDirectFBInputDevice *input_device,
			   const gchar          *name)
{
  GdkDirectfbDevice *directfb_device;
  GdkDevice *device;

  device = g_object_new (GDK_TYPE_DIRECTFB_DEVICE,
			 "name", name,
			 "type", GDK_DEVICE_TYPE_MASTER,
			 "input-source", GDK_SOURCE_KEYBOARD,
			 "input-mode", GDK_MODE_SCREEN,
			 "has-cursor", FALSE,
			 "display", display,
			 "device-manager", device_manager,
			 NULL);

  directfb_device = GDK_DIRECTFB_DEVICE (device);

  if (G_LIKELY (input_device))
    (void) input_device->AddRef (input_device);

  directfb_device->input_device = input_device;
  directfb_device->grab_window = NULL;
  directfb_device->event_mask = GDK_ALL_EVENTS_MASK;
  directfb_device->grab_mask = GDK_ALL_EVENTS_MASK;
  directfb_device->x = 0;
  directfb_device->y = 0;
  directfb_device->win_x = 0;
  directfb_device->win_y = 0;

  return device;
}

/**
 * gdk_directfb_pointer_new:
 * @display: The display for which this pointer device is being created
 * @device_manager: The device manager this device is associated with
 * @input_device: (allow-none): The #IDirectFBInputDevice for this device
 * @name: (allow-none): The name of the input device
 *
 * Creates a new #GdkDirectfbDevice.
 *
 * Returns: (transfer full): A new #GdkDevice
 *
 * Since: 3.10
 */
GdkDevice *
gdk_directfb_pointer_new (GdkDisplay           *display,
			  GdkDeviceManager     *device_manager,
			  IDirectFBInputDevice *input_device,
			  const gchar          *name)
{
  GdkDirectfbDevice *directfb_device;
  GdkDevice *device;

  device = g_object_new (GDK_TYPE_DIRECTFB_DEVICE,
			 "name", name,
			 "type", GDK_DEVICE_TYPE_MASTER,
			 "input-source", GDK_SOURCE_MOUSE,
			 "input-mode", GDK_MODE_SCREEN,
			 "has-cursor", FALSE,
			 "display", display,
			 "device-manager", device_manager,
			 NULL);

  directfb_device = GDK_DIRECTFB_DEVICE (device);

  if (G_LIKELY (input_device))
    (void) input_device->AddRef (input_device);

  directfb_device->input_device = input_device;
  directfb_device->grab_window = NULL;
  directfb_device->event_mask = GDK_ALL_EVENTS_MASK;
  directfb_device->grab_mask = GDK_ALL_EVENTS_MASK;
  directfb_device->x = 0;
  directfb_device->y = 0;
  directfb_device->win_x = 0;
  directfb_device->win_y = 0;

  return device;
}

/**
 * gdk_directfb_device_get_input_device:
 * @device: A #GdkDevice object
 *
 * Get the underlying DirectFB input device.
 *
 * Returns: (transfer none): A #IDirectFBInputDevice
 *
 * Since: 3.10
 */
IDirectFBInputDevice *
gdk_directfb_device_get_input_device (GdkDevice *device)
{
  g_return_val_if_fail (GDK_IS_DIRECTFB_DEVICE (device), NULL);

  return GDK_DIRECTFB_DEVICE (device)->input_device;
}

/**
 * gdk_directfb_device_set_focus_window:
 * @device: A #GdkDevice object
 * @window: A #GdkWindow object
 *
 * Set the window that has focus for this device.
 *
 * Since: 3.10
 */
void
gdk_directfb_device_set_focus_window (GdkDevice *device,
				      GdkWindow *window)
{
  GdkDirectfbDevice *directfb_device;

  g_return_if_fail (GDK_IS_DIRECTFB_DEVICE (device));

  directfb_device = GDK_DIRECTFB_DEVICE (device);

  if (directfb_device->focus_window)
    g_object_unref (directfb_device->focus_window);

  directfb_device->focus_window = window ? g_object_ref (window) : NULL;
}

/**
 * gdk_directfb_device_set_position:
 * @device: A #GdkDevice object
 * @x: The X-coordinate of the pointer inside the root window
 * @y: The Y-coordinate of the pointer inside the root window
 * @win_x: The X-coordinate of the pointer inside the focused window
 * @win_y: The Y-coordinate of the pointer inside the focused window
 *
 * Set the current position of the pointer.
 *
 * Since: 3.10
 */
void
gdk_directfb_device_set_position (GdkDevice *device,
				  gint       x,
				  gint       y,
				  gint       win_x,
				  gint       win_y)
{
  GdkDirectfbDevice *directfb_device;

  g_return_if_fail (GDK_IS_DIRECTFB_DEVICE (device));

  directfb_device = GDK_DIRECTFB_DEVICE (device);

  directfb_device->x = x;
  directfb_device->y = y;
  directfb_device->win_x = win_x;
  directfb_device->win_y = win_y;
}
