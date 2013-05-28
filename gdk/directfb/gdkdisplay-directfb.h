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

#ifndef GDK_DIRECTFB_DISPLAY_
#define GDK_DIRECTFB_DISPLAY_

#include "gdkcursortheme.h"
#include "gdkdirectfbdisplay.h"
#include "gdkdisplayprivate.h"
#include <directfb.h>

G_BEGIN_DECLS

struct GdkDirectfbDisplay_
{
  GdkDisplay parent_instance;
  IDirectFB *dfb;
  IDirectFBEventBuffer *buffer;
  GSource *source;
  GdkScreen *screen;
  GdkKeymap *keymap;
  GdkCursorTheme *cursor_theme;
  GdkWindow *selection_owner;
};

struct GdkDirectfbDisplayClass_
{
  GdkDisplayClass parent_class;
};

G_GNUC_INTERNAL
GdkCursorTheme * gdk_directfb_display_get_cursor_theme (GdkDisplay *display);

G_GNUC_INTERNAL
void gdk_directfb_display_window_destroyed (GdkDisplay *display,
                                            GdkWindow  *window);

G_END_DECLS

#endif  /* GDK_DIRECTFB_DISPLAY_ */
