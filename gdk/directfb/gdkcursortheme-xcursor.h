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

#ifndef GDK_XCURSOR_CURSOR_THEME_H_
#define GDK_XCURSOR_CURSOR_THEME_H_

#include "gdkcursortheme.h"
#include "xcursor.h"

G_BEGIN_DECLS

#define GDK_TYPE_XCURSOR_CURSOR_THEME              (gdk_xcursor_cursor_theme_get_type ())
#define GDK_XCURSOR_CURSOR_THEME(object)           (G_TYPE_CHECK_INSTANCE_CAST ((object), GDK_TYPE_XCURSOR_CURSOR_THEME, GdkXcursorCursorTheme))
#define GDK_XCURSOR_CURSOR_THEME_CLASS(klass)      (G_TYPE_CHECK_CLASS_CAST ((klass), GDK_TYPE_XCURSOR_CURSOR_THEME, GdkXcursorCursorThemeClass))
#define GDK_IS_XCURSOR_CURSOR_THEME(object)        (G_TYPE_CHECK_INSTANCE_TYPE ((object), GDK_TYPE_XCURSOR_CURSOR_THEME))
#define GDK_IS_XCURSOR_CURSOR_THEME_CLASS(klass)   (G_TYPE_CHECK_CLASS_TYPE ((klass), GDK_TYPE_XCURSOR_CURSOR_THEME))
#define GDK_XCURSOR_CURSOR_THEME_GET_CLASS(object) (G_TYPE_INSTANCE_GET_CLASS ((object), GDK_TYPE_XCURSOR_CURSOR_THEME, GdkXcursorCursorThemeClass))

typedef struct GdkXcursorCursorTheme_ GdkXcursorCursorTheme;
typedef struct GdkXcursorCursorThemeClass_ GdkXcursorCursorThemeClass;

struct GdkXcursorCursorTheme_
{
  GdkCursorTheme cursor_theme;
  XcursorImages **images;
  GdkCursorTheme *fallback;
};

struct GdkXcursorCursorThemeClass_
{
  GdkCursorThemeClass cursor_theme_class;
};

G_GNUC_INTERNAL
GType gdk_xcursor_cursor_theme_get_type (void) G_GNUC_CONST;

G_GNUC_INTERNAL
GdkCursorTheme * gdk_xcursor_cursor_theme_new (GdkDisplay     *display,
                                               const gchar    *name,
                                               gint            size,
                                               GdkCursorTheme *fallback);

G_END_DECLS

#endif /* GDK_XCURSOR_CURSOR_THEME_H_ */
