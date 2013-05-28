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
#include "gdkdisplay-directfb.h"
#include "gdkprivate-directfb.h"

G_DEFINE_TYPE (GdkDirectfbDeviceManager, gdk_directfb_device_manager, GDK_TYPE_DEVICE_MANAGER)

static void
gdk_directfb_device_manager_init (GdkDirectfbDeviceManager *device_manager)
{
}

static void
gdk_directfb_device_manager_dispose (GObject *object)
{
  GdkDirectfbDeviceManager *directfb_device_manager =
    GDK_DIRECTFB_DEVICE_MANAGER (object);

  g_list_free_full (directfb_device_manager->devices, g_object_unref);

  G_OBJECT_CLASS (gdk_directfb_device_manager_parent_class)->dispose (object);

  directfb_device_manager->devices = NULL;
  directfb_device_manager->core_pointer = NULL;
  directfb_device_manager->core_keyboard = NULL;
}

static GList *
gdk_directfb_device_manager_list_devices (GdkDeviceManager *device_manager,
					  GdkDeviceType     type)
{
  GdkDirectfbDeviceManager *directfb_device_manager;
  GList *devices = NULL;

  if (GDK_DEVICE_TYPE_MASTER == type)
    {
      directfb_device_manager = GDK_DIRECTFB_DEVICE_MANAGER (device_manager);
      devices = g_list_copy (directfb_device_manager ->devices);
    }

  return devices;
}

static GdkDevice *
gdk_directfb_device_manager_get_client_pointer (GdkDeviceManager *device_manager)
{
  return GDK_DIRECTFB_DEVICE_MANAGER (device_manager)->core_pointer;
}

static void
gdk_directfb_device_manager_class_init (GdkDirectfbDeviceManagerClass *klass)
{
  GdkDeviceManagerClass *device_manager_class = GDK_DEVICE_MANAGER_CLASS (klass);
  GObjectClass *object_class = G_OBJECT_CLASS (klass);

  object_class->dispose = gdk_directfb_device_manager_dispose;

  device_manager_class->list_devices = gdk_directfb_device_manager_list_devices;
  device_manager_class->get_client_pointer = gdk_directfb_device_manager_get_client_pointer;
}

/**
 * enum_input_devices:
 * @device_id: A DirectFB device ID
 * @desc: A DirectFB input device description
 * @data: A #GdkDeviceManager object
 *
 * This is a callback function for IDirectFB::EnumInputDevices(). A new
 * #GdkDevice will be created for each keyboard/pointer device, which is
 * added to the device manager object.
 *
 * Returns: DFENUM_OK (to continue the enumeration)
 *
 * Since: 3.10
 */
static DFBEnumerationResult
enum_input_devices (DFBInputDeviceID          device_id,
		    DFBInputDeviceDescription desc,
		    void                     *data)
{
  GdkDeviceManager *device_manager = data;
  GdkDirectfbDeviceManager *directfb_device_manager;
  IDirectFBInputDevice *dfb_device;
  IDirectFBEventBuffer *buffer;
  GdkDevice *device = NULL;
  DFBResult result;
  IDirectFB *dfb;

  dfb = gdk_directfb_display_get_context (device_manager->display);
  result = dfb->GetInputDevice (dfb, device_id, &dfb_device);
  if (G_UNLIKELY (DFB_OK != result))
    {
      g_warning ("%s", DirectFBErrorString (result));
      return DFENUM_OK;
    }

  if (desc.type & DIDTF_MOUSE)
    {
      device = gdk_directfb_pointer_new (device_manager->display,
					 device_manager,
					 dfb_device,
					 desc.name);
      if (G_UNLIKELY (! device))
	goto exit;

      directfb_device_manager = GDK_DIRECTFB_DEVICE_MANAGER (device_manager);
      directfb_device_manager->core_pointer = device;
      directfb_device_manager->devices =
	g_list_prepend (directfb_device_manager->devices, device);
    }

  if (desc.type & DIDTF_KEYBOARD)
    {
      device = gdk_directfb_keyboard_new (device_manager->display,
					  device_manager,
					  dfb_device,
					  desc.name);
      if (G_UNLIKELY (! device))
	goto exit;

      directfb_device_manager = GDK_DIRECTFB_DEVICE_MANAGER (device_manager);
      directfb_device_manager->core_keyboard = device;
      directfb_device_manager->devices =
	g_list_prepend (directfb_device_manager->devices, device);
    }

  if (G_UNLIKELY (! device))
    {
      g_warning ("unsupported device: %s", desc.name);
      goto exit;
    }

  g_message ("found device: %s", desc.name);

  buffer = gdk_directfb_display_get_event_buffer (device_manager->display);
  result = dfb_device->AttachEventBuffer (dfb_device, buffer);
  if (G_UNLIKELY (DFB_OK != result))
    g_warning ("%s", DirectFBErrorString (result));

exit:

  (void) dfb_device->Release (dfb_device);

  return DFENUM_OK;
}

/**
 * gdk_directfb_device_manager_new:
 * @display: The #GdkDisplay for which this device manager is being created
 *
 * Create a new device manager object. When the device manager is created it is
 * populated with the keyboard/pointer devices detected by DirectFB.
 *
 * Returns: (transfer full): A new #GdkDeviceManager
 *
 * Since: 3.10
 */
GdkDeviceManager *
gdk_directfb_device_manager_new (GdkDisplay *display)
{
  GdkDeviceManager *device_manager;
  GdkDirectfbDeviceManager *directfb_device_manager;
  DFBResult result;
  IDirectFB *dfb;

  device_manager = g_object_new (GDK_TYPE_DIRECTFB_DEVICE_MANAGER,
				 "display", display,
				 NULL);

  dfb = gdk_directfb_display_get_context (display);
  result = dfb->EnumInputDevices (dfb, enum_input_devices, device_manager);
  if (G_UNLIKELY (DFB_OK != result))
    g_warning ("%s", DirectFBErrorString (result));

  directfb_device_manager = GDK_DIRECTFB_DEVICE_MANAGER (device_manager);

  if (G_UNLIKELY (! directfb_device_manager->core_pointer))
    {
      directfb_device_manager->core_pointer =
	gdk_directfb_pointer_new (device_manager->display,
				  device_manager,
				  NULL,
				  "Dummy Pointer");

      directfb_device_manager->devices =
	g_list_prepend (directfb_device_manager->devices,
			directfb_device_manager->core_pointer);
    }

  if (G_UNLIKELY (! directfb_device_manager->core_keyboard))
    {
      directfb_device_manager->core_keyboard =
	gdk_directfb_keyboard_new (device_manager->display,
				   device_manager,
				   NULL,
				   "Dummy Keyboard");

      directfb_device_manager->devices =
	g_list_prepend (directfb_device_manager->devices,
			directfb_device_manager->core_keyboard);
    }

  _gdk_device_set_associated_device (directfb_device_manager->core_pointer,
				     directfb_device_manager->core_keyboard);
  _gdk_device_set_associated_device (directfb_device_manager->core_keyboard,
				     directfb_device_manager->core_pointer);

  return device_manager;
}

/**
 * gdk_directfb_device_manager_get_core_pointer:
 * @device_manager: A #GdkDeviceManager object
 *
 * Get the core pointer device object.
 *
 * Returns: (transfer none): A #GdkDevice
 *
 * Since: 3.10
 */
GdkDevice *
gdk_directfb_device_manager_get_core_pointer (GdkDeviceManager *device_manager)
{
  g_return_val_if_fail (GDK_IS_DIRECTFB_DEVICE_MANAGER (device_manager), NULL);

  return GDK_DIRECTFB_DEVICE_MANAGER (device_manager)->core_pointer;
}

/**
 * gdk_directfb_device_manager_get_core_keyboard:
 * @device_manager: A #GdkDeviceManager object
 *
 * Get the core keyboard device object.
 *
 * Returns: (transfer none): A #GdkDevice
 *
 * Since: 3.10
 */
GdkDevice *
gdk_directfb_device_manager_get_core_keyboard (GdkDeviceManager *device_manager)
{
  g_return_val_if_fail (GDK_IS_DIRECTFB_DEVICE_MANAGER (device_manager), NULL);

  return GDK_DIRECTFB_DEVICE_MANAGER (device_manager)->core_keyboard;
}
