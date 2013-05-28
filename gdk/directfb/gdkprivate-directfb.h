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

#ifndef GDK_PRIVATE_DIRECTFB_
#define GDK_PRIVATE_DIRECTFB_

#include "gdkdisplayprivate.h"
#include "gdkinternals.h"
#include <directfb.h>

G_GNUC_INTERNAL
G_GNUC_WARN_UNUSED_RESULT
GdkDisplay * _gdk_directfb_display_open (const gchar *display_name);

G_GNUC_INTERNAL
guint gdk_directfb_translate_key (DFBInputDeviceKeyIdentifier key_id,
                                  DFBInputDeviceKeySymbol     key_symbol);

G_GNUC_INTERNAL
GdkModifierType gdk_directfb_modifier_mask (DFBInputDeviceModifierMask modifiers,
                                            DFBInputDeviceLockState    locks,
                                            DFBInputDeviceButtonMask   buttons);

G_GNUC_INTERNAL
void gdk_directfb_parse_modifier_mask (GdkModifierType             state,
                                       DFBInputDeviceModifierMask *modifiers,
                                       DFBInputDeviceLockState    *locks,
                                       DFBInputDeviceButtonMask   *buttons);

G_GNUC_INTERNAL
guchar gdk_directfb_premultiply (guchar alpha,
                                 guchar color);

#endif /* GDK_PRIVATE_DIRECTFB_ */
