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

#ifndef GDK_DIRECTFB_KEYMAP_
#define GDK_DIRECTFB_KEYMAP_

#include "gdkkeysprivate.h"
#include <directfb.h>

G_BEGIN_DECLS

typedef struct GdkDirectfbKeymap_ GdkDirectfbKeymap;
typedef struct GdkDirectfbKeymapClass_ GdkDirectfbKeymapClass;

#define GDK_TYPE_DIRECTFB_KEYMAP              (gdk_directfb_keymap_get_type ())
#define GDK_DIRECTFB_KEYMAP(object)           (G_TYPE_CHECK_INSTANCE_CAST ((object), GDK_TYPE_DIRECTFB_KEYMAP, GdkDirectfbKeymap))
#define GDK_DIRECTFB_KEYMAP_CLASS(klass)      (G_TYPE_CHECK_CLASS_CAST ((klass), GDK_TYPE_DIRECTFB_KEYMAP, GdkDirectfbKeymapClass))
#define GDK_IS_DIRECTFB_KEYMAP(object)        (G_TYPE_CHECK_INSTANCE_TYPE ((object), GDK_TYPE_DIRECTFB_KEYMAP))
#define GDK_IS_DIRECTFB_KEYMAP_CLASS(klass)   (G_TYPE_CHECK_CLASS_TYPE ((klass), GDK_TYPE_DIRECTFB_KEYMAP))
#define GDK_DIRECTFB_KEYMAP_GET_CLASS(object) (G_TYPE_INSTANCE_GET_CLASS ((object), GDK_TYPE_DIRECTFB_KEYMAP, GdkDirectfbKeymapClass))

struct GdkDirectfbKeymap_
{
  GdkKeymap parent_instance;
  GdkDevice *device;
  guint *keymap;
  gsize min_keycode;
  gsize max_keycode;
};

struct GdkDirectfbKeymapClass_
{
  GdkKeymapClass parent_class;
};

G_GNUC_INTERNAL
GType gdk_directfb_keymap_get_type (void) G_GNUC_CONST;

G_GNUC_INTERNAL
G_GNUC_WARN_UNUSED_RESULT
GdkKeymap * gdk_directfb_keymap_new (GdkDisplay *display);

G_GNUC_INTERNAL
guint16 gdk_directfb_keymap_lookup_hardware_keycode (GdkKeymap *keymap,
                                                     guint      keyval);

G_END_DECLS

#endif /* GDK_DIRECTFB_KEYMAP_ */
