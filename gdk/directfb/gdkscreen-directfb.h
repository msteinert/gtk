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

#ifndef GDK_DIRECTFB_SCREEN_
#define GDK_DIRECTFB_SCREEN_

#include "gdkscreenprivate.h"
#include "gdkdirectfbscreen.h"
#include <directfb.h>

G_BEGIN_DECLS

struct GdkDirectfbScreen_
{
  GdkScreen parent_instance;
  GdkDisplay *display;
  IDirectFBScreen *screen;
  IDirectFBDisplayLayer *layer;
  GdkWindow *root_window;
  GList *visuals;
  GdkVisual *rgba_visual;
  GdkVisual *system_visual;
  GArray *depths;
  GArray *visual_types;
  GList *windows;
};

struct GdkDirectfbScreenClass_
{
  GdkScreenClass parent_class;
};

G_GNUC_INTERNAL
G_GNUC_WARN_UNUSED_RESULT
GdkScreen * gdk_directfb_screen_new (GdkDisplay *display,
				     DFBScreenID id);

G_GNUC_INTERNAL
void gdk_directfb_screen_add_window (GdkScreen *screen,
                                     GdkWindow *window);

G_GNUC_INTERNAL
void gdk_directfb_screen_remove_window (GdkScreen *screen,
                                        GdkWindow *window);

G_GNUC_INTERNAL
GdkWindow * gdk_directfb_screen_get_window (GdkScreen  *screen,
                                            DFBWindowID id);

G_END_DECLS

#endif /* GDK_DIRECTFB_SCREEN_ */
