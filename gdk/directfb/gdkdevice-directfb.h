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

#ifndef GDK_DIRECTFB_DEVICE_
#define GDK_DIRECTFB_DEVICE_

#include "gdkdeviceprivate.h"
#include "gdkdirectfbdevice.h"
#include <directfb.h>

G_BEGIN_DECLS

struct GdkDirectfbDevice_
{
  GdkDevice parent_instance;
  IDirectFBInputDevice *input_device;
  GdkWindow *grab_window;
  GdkWindow *focus_window;
  GdkEventMask event_mask;
  GdkEventMask grab_mask;
  GdkCursor *cursor;
  gint x, y, win_x, win_y;
};

struct GdkDirectfbDeviceClass_
{
  GdkDeviceClass parent_class;
};

G_GNUC_INTERNAL
G_GNUC_WARN_UNUSED_RESULT
GdkDevice * gdk_directfb_keyboard_new (GdkDisplay           *display,
                                       GdkDeviceManager     *device_manager,
                                       IDirectFBInputDevice *input_device,
                                       const gchar          *name);

G_GNUC_INTERNAL
G_GNUC_WARN_UNUSED_RESULT
GdkDevice * gdk_directfb_pointer_new (GdkDisplay           *display,
                                      GdkDeviceManager     *device_manager,
                                      IDirectFBInputDevice *input_device,
                                      const gchar          *name);

G_GNUC_INTERNAL
void gdk_directfb_device_set_focus_window (GdkDevice *device,
                                           GdkWindow *window);

G_GNUC_INTERNAL
void gdk_directfb_device_set_position (GdkDevice *device,
                                       gint       x,
                                       gint       y,
                                       gint       win_x,
                                       gint       win_y);

G_END_DECLS

#endif /* GDK_DIRECTFB_DEVICE_ */
