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

#include "gdkcursor-directfb.h"
#include "gdkcursortheme.h"
#include "gdkdisplay-directfb.h"

G_DEFINE_TYPE (GdkCursorTheme, gdk_cursor_theme, G_TYPE_OBJECT)

#define CURSORS_BUFLEN ((GDK_LAST_CURSOR + 1) / 2)

static void
gdk_cursor_theme_init (GdkCursorTheme *cursor_theme)
{
  cursor_theme->cursors = g_malloc0_n (sizeof (GdkCursor *), CURSORS_BUFLEN);
}

static void
gdk_cursor_theme_dispose (GObject *object)
{
  GdkCursorTheme *cursor_theme = GDK_CURSOR_THEME (object);
  gsize i;

  if (cursor_theme->blank)
    g_object_unref (cursor_theme->blank);

  for (i = 0; i < CURSORS_BUFLEN; ++i)
    if (cursor_theme->cursors[i])
      {
	g_object_unref (cursor_theme->cursors[i]);
	cursor_theme->cursors[i] = NULL;
      }

  G_OBJECT_CLASS (gdk_cursor_theme_parent_class)->dispose (object);

  cursor_theme->blank = NULL;
}

static void
gdk_cursor_theme_finalize (GObject *object)
{
  GdkCursorTheme *cursor_theme = GDK_CURSOR_THEME (object);

  g_free (cursor_theme->cursors);

  G_OBJECT_CLASS (gdk_cursor_theme_parent_class)->finalize (object);
}

static void
gdk_cursor_theme_class_init (GdkCursorThemeClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);

  object_class->dispose = gdk_cursor_theme_dispose;
  object_class->finalize = gdk_cursor_theme_finalize;
}

/**
 * gdk_cursor_theme_get_cursor:
 * @cursor_theme: A #GdkCursorTheme object
 * @cursor_type: The type of cursor to return
 *
 * Finds the specified cursor type within a theme.
 *
 * Returns: (transfer full): A #GdkCursor
 *
 * Since: 3.10
 */
GdkCursor *
gdk_cursor_theme_get_cursor (GdkCursorTheme *cursor_theme,
			     GdkCursorType   cursor_type)
{
  gsize index = cursor_type / 2;
  GdkCursor *cursor;

  g_return_val_if_fail (GDK_IS_CURSOR_THEME (cursor_theme), NULL);

  switch (cursor_type)
    {
    case GDK_BLANK_CURSOR:
      if (G_UNLIKELY (! cursor_theme->blank))
	  cursor_theme->blank =
	    gdk_directfb_blank_cursor_new (cursor_theme->display);

      cursor = cursor_theme->blank;
      break;

    default:
      if (G_UNLIKELY (index >= CURSORS_BUFLEN))
	return NULL;

      if (G_UNLIKELY (! cursor_theme->cursors[index]))
	cursor_theme->cursors[index] =
	  GDK_CURSOR_THEME_GET_CLASS (cursor_theme)->get_cursor (cursor_theme,
								 cursor_type);

      cursor = cursor_theme->cursors[index];
      break;
    }

  return g_object_ref (cursor);
}
