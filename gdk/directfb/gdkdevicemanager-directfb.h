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

#ifndef GDK_DEVICE_MANAGER_DIRECTFB_
#define GDK_DEVICE_MANAGER_DIRECTFB_

#include "gdkdevicemanagerprivate.h"

G_BEGIN_DECLS

#define GDK_TYPE_DIRECTFB_DEVICE_MANAGER              (gdk_directfb_device_manager_get_type ())
#define GDK_DIRECTFB_DEVICE_MANAGER(object)           (G_TYPE_CHECK_INSTANCE_CAST ((object), GDK_TYPE_DIRECTFB_DEVICE_MANAGER, GdkDirectfbDeviceManager))
#define GDK_DIRECTFB_DEVICE_MANAGER_CLASS(klass)      (G_TYPE_CHECK_CLASS_CAST ((klass), GDK_TYPE_DIRECTFB_DEVICE_MANAGER, GdkDirectfbDeviceManagerClass))
#define GDK_IS_DIRECTFB_DEVICE_MANAGER(object)        (G_TYPE_CHECK_INSTANCE_TYPE ((object), GDK_TYPE_DIRECTFB_DEVICE_MANAGER))
#define GDK_IS_DIRECTFB_DEVICE_MANAGER_CLASS(klass)   (G_TYPE_CHECK_CLASS_TYPE ((klass), GDK_TYPE_DIRECTFB_DEVICE_MANAGER))
#define GDK_DIRECTFB_DEVICE_MANAGER_GET_CLASS(object) (G_TYPE_INSTANCE_GET_CLASS ((object), GDK_TYPE_DIRECTFB_DEVICE_MANAGER, GdkDirectfbDeviceManagerClass))

typedef struct GdkDirectfbDeviceManager_ GdkDirectfbDeviceManager;
typedef struct GdkDirectfbDeviceManagerClass_ GdkDirectfbDeviceManagerClass;

struct GdkDirectfbDeviceManager_
{
  GdkDeviceManager parent_object;
  GList *devices;
  GdkDevice *core_pointer;
  GdkDevice *core_keyboard;
};

struct GdkDirectfbDeviceManagerClass_
{
  GdkDeviceManagerClass parent_class;
};

G_GNUC_INTERNAL
GType gdk_directfb_device_manager_get_type (void) G_GNUC_CONST;

G_GNUC_INTERNAL
G_GNUC_WARN_UNUSED_RESULT
GdkDeviceManager * gdk_directfb_device_manager_new (GdkDisplay *display);

G_GNUC_INTERNAL
GdkDevice * gdk_directfb_device_manager_get_core_pointer (GdkDeviceManager *device_manager);

G_GNUC_INTERNAL
GdkDevice * gdk_directfb_device_manager_get_core_keyboard (GdkDeviceManager *device_manager);

G_END_DECLS

#endif /* GDK_DEVICE_MANAGER_DIRECTFB_ */
