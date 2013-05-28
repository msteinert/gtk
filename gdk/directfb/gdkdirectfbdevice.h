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

#ifndef GDK_DIRECTFB_DEVICE_H
#define GDK_DIRECTFB_DEVICE_H

#if !defined (GDKDIRECTFB_H_INSIDE) && !defined (GDK_COMPILATION)
#error "Only <gdk/gdkdirectfb.h> can be included directly."
#endif

#include <directfb.h>
#include <gdk.h>

G_BEGIN_DECLS

#define GDK_TYPE_DIRECTFB_DEVICE              (gdk_directfb_device_get_type ())
#define GDK_DIRECTFB_DEVICE(object)           (G_TYPE_CHECK_INSTANCE_CAST ((object), GDK_TYPE_DIRECTFB_DEVICE, GdkDirectfbDevice))
#define GDK_DIRECTFB_DEVICE_CLASS(klass)      (G_TYPE_CHECK_CLASS_CAST ((klass), GDK_TYPE_DIRECTFB_DEVICE, GdkDirectfbDeviceClass))
#define GDK_IS_DIRECTFB_DEVICE(object)        (G_TYPE_CHECK_INSTANCE_TYPE ((object), GDK_TYPE_DIRECTFB_DEVICE))
#define GDK_IS_DIRECTFB_DEVICE_CLASS(klass)   (G_TYPE_CHECK_CLASS_TYPE ((klass), GDK_TYPE_DIRECTFB_DEVICE))
#define GDK_DIRECTFB_DEVICE_GET_CLASS(object) (G_TYPE_INSTANCE_GET_CLASS ((object), GDK_TYPE_DIRECTFB_DEVICE, GdkDirectfbDeviceClass))

#ifdef GDK_COMPILATION
typedef struct GdkDirectfbDevice_ GdkDirectfbDevice;
#else
typedef GdkDevice GdkDirectfbDevice;
#endif
typedef struct GdkDirectfbDeviceClass_ GdkDirectfbDeviceClass;

GDK_AVAILABLE_IN_ALL
GType gdk_directfb_device_get_type (void) G_GNUC_CONST;

GDK_AVAILABLE_IN_ALL
IDirectFBInputDevice * gdk_directfb_device_get_input_device (GdkDevice *device);

G_END_DECLS

#endif /* GDK_DIRECTFB_DEVICE_H */
