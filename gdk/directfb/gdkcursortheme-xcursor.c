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
#include "gdkcursortheme-x11.h"
#include "gdkcursortheme-xcursor.h"
#include "gdkdisplay-directfb.h"
#include "gdkwindow-directfb.h"

G_DEFINE_TYPE (GdkXcursorCursorTheme, gdk_xcursor_cursor_theme, GDK_TYPE_CURSOR_THEME)

#define IMAGES_BUFLEN ((GDK_LAST_CURSOR + 1) / 2)

static void
gdk_xcursor_cursor_theme_init (GdkXcursorCursorTheme *cursor_theme)
{
  cursor_theme->images = g_malloc0_n (sizeof (GdkCursor *), IMAGES_BUFLEN);
}

static void
gdk_xcursor_cursor_theme_dispose (GObject *object)
{
  GdkXcursorCursorTheme *xcursor_theme = GDK_XCURSOR_CURSOR_THEME (object);

  if (xcursor_theme->fallback)
    g_object_unref (xcursor_theme->fallback);

  G_OBJECT_CLASS (gdk_xcursor_cursor_theme_parent_class)->dispose (object);

  xcursor_theme->fallback = NULL;
}

static void
gdk_xcursor_cursor_theme_finalize (GObject *object)
{
  GdkXcursorCursorTheme *xcursor_theme = GDK_XCURSOR_CURSOR_THEME (object);
  gsize i;

  for (i = 0; i < IMAGES_BUFLEN; ++i)
    if (xcursor_theme->images[i])
      XcursorImagesDestroy (xcursor_theme->images[i]);

  g_free (xcursor_theme->images);

  G_OBJECT_CLASS (gdk_xcursor_cursor_theme_parent_class)->finalize (object);
}

/**
 * gdk_xcursor_cursor_theme_add:
 * @cursor_theme: A #GdkXcursorCursorTheme object
 * @cursor: A #GdkDirectfbCursor object
 * @cursor_index: The cursor image index
 * @image_index: The cursor animation index
 *
 * Loads the Xcursor image at @cursor_index into a DirectFB surface and then
 * creates a #GdkDirectfbCursor at @animation_index for @cursor. After the
 * DirectFB surface is created, the Xcursor image data is freed.
 *
 * Since: 3.10
 */
static void
gdk_xcursor_cursor_theme_add (GdkCursorTheme *cursor_theme,
			      GdkCursor      *cursor,
			      gsize           cursor_index,
			      gsize           animation_index)
{
  GdkXcursorCursorTheme *xcursor_theme =
    GDK_XCURSOR_CURSOR_THEME (cursor_theme);
  guchar *src, *src_pixels, *dst, *dst_pixels;
  DFBSurfaceDescription desc;
  IDirectFBSurface *surface;
  XcursorImage *image;
  DFBResult result;
  IDirectFB *dfb;
  gint pitch;
  gsize i, j;

  image = xcursor_theme->images[cursor_index]->images[animation_index];
  desc.flags = DSDESC_WIDTH | DSDESC_HEIGHT | DSDESC_PIXELFORMAT | DSDESC_CAPS;
  desc.width = image->width;
  desc.height = image->height;
  desc.pixelformat = DSPF_ARGB;
  desc.caps = DSCAPS_PREMULTIPLIED;

  dfb = gdk_directfb_display_get_context (cursor_theme->display);
  result = dfb->CreateSurface (dfb, &desc, &surface);
  if (G_UNLIKELY (DFB_OK != result))
    {
      g_warning ("%s", DirectFBErrorString (result));
      return;
    }

  result = surface->Lock (surface, DSLF_WRITE, (void **) &dst_pixels, &pitch);
  if (G_UNLIKELY (DFB_OK != result))
    {
      g_warning ("%s", DirectFBErrorString (result));
      (void) surface->Release (surface);
      return;
    }

#if G_BYTE_ORDER == G_LITTLE_ENDIAN
#define A (3)
#define R (2)
#define G (1)
#define B (0)
#else
#define A (0)
#define R (1)
#define G (2)
#define B (3)
#endif

  src_pixels = (guchar *) image->pixels;
  for (i = 0; i < image->height; ++i)
    {
      src = (guchar *) (src_pixels + i * pitch);
      dst = (guchar *) (dst_pixels + i * pitch);
      for (j = 0; j < image->width; ++j)
	{
	  dst[A] = src[A];
	  switch (dst[A])
	    {
	    case 0x00:
	      dst[R] = dst[G] = dst[B] = 0x00;
	      break;

	    case 0xff:
	      dst[R] = src[R];
	      dst[G] = src[G];
	      dst[B] = src[B];
	      break;

	    default:
	      dst[R] = gdk_directfb_premultiply (dst[A], src[R]);
	      dst[G] = gdk_directfb_premultiply (dst[A], src[G]);
	      dst[B] = gdk_directfb_premultiply (dst[A], src[B]);
	      break;
	    }

	  src += 4;
	  dst += 4;
	}
    }

#undef A
#undef R
#undef G
#undef B

  (void) surface->Unlock (surface);
  gdk_directfb_animated_cursor_add (cursor, surface,
				    image->xhot, image->yhot,
				    image->delay, animation_index);
  (void) surface->Release (surface);
}

static GdkCursor *
gdk_xcursor_cursor_theme_get_cursor (GdkCursorTheme *cursor_theme,
				     GdkCursorType   cursor_type)
{
  GdkXcursorCursorTheme *xcursor_theme =
    GDK_XCURSOR_CURSOR_THEME (cursor_theme);
  gsize i, index = cursor_type / 2;
  GdkCursor *cursor = NULL;
  XcursorImages *images;

  if (! xcursor_theme->images[index])
    return gdk_cursor_theme_get_cursor (xcursor_theme->fallback, cursor_type);

  images = xcursor_theme->images[index];
  cursor = gdk_directfb_animated_cursor_new (cursor_theme->display,
					     cursor_type, images->nimage);

  for (i = 0; i < images->nimage; ++i)
    gdk_xcursor_cursor_theme_add (cursor_theme, cursor, index, i);

  XcursorImagesDestroy (xcursor_theme->images[index]);
  xcursor_theme->images[index] = NULL;

  return cursor;
}

static void
gdk_xcursor_cursor_theme_class_init (GdkXcursorCursorThemeClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);
  GdkCursorThemeClass *cursor_theme_class = GDK_CURSOR_THEME_CLASS (klass);

  object_class->dispose = gdk_xcursor_cursor_theme_dispose;
  object_class->finalize = gdk_xcursor_cursor_theme_finalize;

  cursor_theme_class->get_cursor = gdk_xcursor_cursor_theme_get_cursor;
}

/**
 * gdk_xcursor_cursor_theme_load:
 * @images: An #XcursorImages object
 * @data: A #GdkXcursorCursorTheme object
 *
 * This is a callback function for xcursor_load_theme(). Each image found in
 * the theme is stored for later use. Surface creation is delayed until a
 * cursor is actually used.
 *
 * Since: 3.10
 */
static void
gdk_xcursor_cursor_theme_load (XcursorImages *images, void *data)
{
  GdkXcursorCursorTheme *xcursor_theme = data;
  GdkCursorType cursor_type;
  GEnumClass *enum_class;
  GEnumValue *enum_value;
  gsize index;

  (void) g_strdelimit (images->name, "_", '-');
  enum_class = g_type_class_ref (GDK_TYPE_CURSOR_TYPE);
  enum_value = g_enum_get_value_by_nick (enum_class, images->name);
  if (! enum_value)
    return;

  cursor_type = enum_value->value;
  index = cursor_type / 2;

  if (G_UNLIKELY (index >= IMAGES_BUFLEN))
    return;

  if (! xcursor_theme->images[index])
    xcursor_theme->images[index] = images;
  else
    XcursorImagesDestroy (images);
}

/**
 * gdk_xcursor_cursor_theme_new:
 * @display: The #GdkDisplay for which the theme will be created
 * @name: The name of the Xcursor theme to load
 * @size: The desired nominal size of the Xcursor theme
 * @fallback: (allow-none): The fallback theme
 *
 * Attempts to load the Xcursor theme identified by @name. Cursors that do not
 * appear in the loaded theme will be loaded from @fallback. If no fallback
 * theme is specified then the default X11 cursor set is used for fallbacks.
 *
 * Returns: (transfer full): A new #GdkCursorTheme
 *
 * Since: 3.10
 */
GdkCursorTheme *
gdk_xcursor_cursor_theme_new (GdkDisplay     *display,
			      const gchar    *name,
			      gint            size,
			      GdkCursorTheme *fallback)
{
  GdkCursorTheme *cursor_theme;
  GdkXcursorCursorTheme *xcursor_theme;

  cursor_theme = g_object_new (GDK_TYPE_XCURSOR_CURSOR_THEME, NULL);
  cursor_theme->display = display;

  xcursor_theme = GDK_XCURSOR_CURSOR_THEME (cursor_theme);
  xcursor_theme->fallback = fallback ? g_object_ref (fallback)
    : gdk_x11_cursor_theme_new (display);

  xcursor_load_theme (name, size, gdk_xcursor_cursor_theme_load, xcursor_theme);

  return cursor_theme;
}
