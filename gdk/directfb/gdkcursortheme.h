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

#ifndef GDK_CURSOR_THEME_H_
#define GDK_CURSOR_THEME_H_

#include "gdkcursor-directfb.h"

G_BEGIN_DECLS

#define GDK_TYPE_CURSOR_THEME              (gdk_cursor_theme_get_type ())
#define GDK_CURSOR_THEME(object)           (G_TYPE_CHECK_INSTANCE_CAST ((object), GDK_TYPE_CURSOR_THEME, GdkCursorTheme))
#define GDK_CURSOR_THEME_CLASS(klass)      (G_TYPE_CHECK_CLASS_CAST ((klass), GDK_TYPE_CURSOR_THEME, GdkCursorThemeClass))
#define GDK_IS_CURSOR_THEME(object)        (G_TYPE_CHECK_INSTANCE_TYPE ((object), GDK_TYPE_CURSOR_THEME))
#define GDK_IS_CURSOR_THEME_CLASS(klass)   (G_TYPE_CHECK_CLASS_TYPE ((klass), GDK_TYPE_CURSOR_THEME))
#define GDK_CURSOR_THEME_GET_CLASS(object) (G_TYPE_INSTANCE_GET_CLASS ((object), GDK_TYPE_CURSOR_THEME, GdkCursorThemeClass))

typedef struct GdkCursorTheme_ GdkCursorTheme;
typedef struct GdkCursorThemeClass_ GdkCursorThemeClass;

struct GdkCursorTheme_
{
  GObject object;
  GdkDisplay *display;
  GdkCursor **cursors;
  GdkCursor *blank;
};

struct GdkCursorThemeClass_
{
  GObjectClass object_class;

  GdkCursor * (*get_cursor) (GdkCursorTheme *cursor_theme,
                             GdkCursorType   cursor_type);
};

G_GNUC_INTERNAL
GType gdk_cursor_theme_get_type (void) G_GNUC_CONST;

G_GNUC_INTERNAL
GdkCursor * gdk_cursor_theme_get_cursor (GdkCursorTheme *cursor_theme,
                                         GdkCursorType   cursor_type);

G_END_DECLS

#endif /* GDK_CURSOR_THEME_H_ */
