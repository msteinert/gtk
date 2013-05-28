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

#include "gdkdirectfbdisplay.h"
#include "gdkprivate-directfb.h"
#include "gdkscreen-directfb.h"
#include "gdkvisualprivate.h"
#include "gdkwindow-directfb.h"

G_DEFINE_TYPE (GdkDirectfbScreen, gdk_directfb_screen, GDK_TYPE_SCREEN)

static void
gdk_directfb_screen_init (GdkDirectfbScreen *screen)
{
}

static void
gdk_directfb_screen_dispose (GObject *object)
{
  GdkDirectfbScreen *directfb_screen = GDK_DIRECTFB_SCREEN (object);

  if (G_LIKELY (directfb_screen->root_window))
    _gdk_window_destroy (directfb_screen->root_window, TRUE);

  g_list_free_full (directfb_screen->windows, g_object_unref);

  G_OBJECT_CLASS (gdk_directfb_screen_parent_class)->dispose (object);

  directfb_screen->display = NULL;
  directfb_screen->windows = NULL;
}

static void
gdk_directfb_screen_finalize (GObject *object)
{
  GdkDirectfbScreen *directfb_screen = GDK_DIRECTFB_SCREEN (object);

  g_list_free_full (directfb_screen->visuals, g_object_unref);
  g_array_free (directfb_screen->depths, TRUE);
  g_array_free (directfb_screen->visual_types, TRUE);

  if (G_LIKELY (directfb_screen->root_window))
    g_object_unref (directfb_screen->root_window);

  if (G_LIKELY (directfb_screen->layer))
    (void) directfb_screen->layer->Release (directfb_screen->layer);

  if (G_LIKELY (directfb_screen->screen))
    (void) directfb_screen->screen->Release (directfb_screen->screen);

  G_OBJECT_CLASS (gdk_directfb_screen_parent_class)->finalize (object);
}

static GdkDisplay *
gdk_directfb_screen_get_display (GdkScreen *screen)
{
  return GDK_DIRECTFB_SCREEN (screen)->display;
}

static gint
gdk_directfb_screen_get_width (GdkScreen *screen)
{
  GdkDirectfbScreen *directfb_screen = GDK_DIRECTFB_SCREEN (screen);
  DFBDisplayLayerConfig config;
  DFBResult result;

  result =  directfb_screen->layer->GetConfiguration (directfb_screen->layer,
						      &config);
  if (G_UNLIKELY (DFB_OK != result))
    {
      g_warning ("%s", DirectFBErrorString (result));
      return 0;
    }

  return config.width;
}

static gint
gdk_directfb_screen_get_height (GdkScreen *screen)
{
  GdkDirectfbScreen *directfb_screen = GDK_DIRECTFB_SCREEN (screen);
  DFBDisplayLayerConfig config;
  DFBResult result;

  result = directfb_screen->layer->GetConfiguration (directfb_screen->layer,
						     &config);
  if (G_UNLIKELY (DFB_OK != result))
    {
      g_warning ("%s", DirectFBErrorString (result));
      return 0;
    }

  return config.height;
}

static gint
gdk_directfb_screen_get_width_mm (GdkScreen *screen)
{
  return gdk_directfb_screen_get_width (screen) * 25.4 / 72;
}

static gint
gdk_directfb_screen_get_height_mm (GdkScreen *screen)
{
  return gdk_directfb_screen_get_height (screen) * 25.4 / 72;
}

static gint
gdk_directfb_screen_get_number (GdkScreen *screen)
{
  GdkDirectfbScreen *directfb_screen = GDK_DIRECTFB_SCREEN (screen);
  DFBResult result;
  DFBScreenID id;

  result = directfb_screen->screen->GetID (directfb_screen->screen, &id);
  if (G_UNLIKELY (DFB_OK != result))
    {
      g_warning ("%s", DirectFBErrorString (result));
      return 0;
    }

  return id;
}

static GdkWindow *
gdk_directfb_screen_get_root_window (GdkScreen *screen)
{
  return GDK_DIRECTFB_SCREEN (screen)->root_window;
}

static gint
gdk_directfb_screen_get_n_monitors (GdkScreen *screen)
{
  return 1;
}

static gint
gdk_directfb_screen_get_primary_monitor (GdkScreen *screen)
{
  return 0;
}

static gint
gdk_directfb_screen_get_monitor_width_mm (GdkScreen *screen,
					  gint       monitor_num)
{

  return gdk_directfb_screen_get_width_mm (screen);
}

static gint
gdk_directfb_screen_get_monitor_height_mm (GdkScreen *screen,
					   gint       monitor_num)
{
  return gdk_directfb_screen_get_height_mm (screen);
}

static gchar *
gdk_directfb_screen_get_monitor_plug_name (GdkScreen *screen,
					   gint       monitor_num)
{
  GdkDirectfbScreen *directfb_screen = GDK_DIRECTFB_SCREEN (screen);
  DFBScreenDescription desc;
  DFBResult result;

  result = directfb_screen->screen->GetDescription (directfb_screen->screen,
						    &desc);
  if (G_UNLIKELY (DFB_OK != result))
    {
      g_warning ("%s", DirectFBErrorString (result));
      return g_strdup ("DirectFB");
    }

  return g_strdup (desc.name);
}

static void
gdk_directfb_screen_get_monitor_geometry (GdkScreen    *screen,
					  gint          monitor_num,
					  GdkRectangle *dest)
{
  GdkDirectfbScreen *directfb_screen = GDK_DIRECTFB_SCREEN (screen);
  DFBDisplayLayerConfig config;
  DFBResult result;

  if (G_UNLIKELY (! dest))
    return;

  dest->x = dest->y = 0;
  result = directfb_screen->layer->GetConfiguration (directfb_screen->layer,
						     &config);
  if (G_UNLIKELY (DFB_OK != result))
    {
      g_warning ("%s", DirectFBErrorString (result));
      dest->width = dest->height = 0;
      return;
    }

  dest->width = config.width;
  dest->height = config.height;
}

static gboolean
gdk_directfb_screen_is_composited (GdkScreen *screen)
{
  return TRUE;
}

static gchar *
gdk_directfb_screen_make_display_name (GdkScreen *screen)
{
  return g_strdup_printf ("0x%04X", gdk_directfb_screen_get_number (screen));
}

static GdkWindow *
gdk_directfb_screen_get_active_window (GdkScreen *screen)
{
  return NULL;
}

static GList *
gdk_directfb_screen_get_window_stack (GdkScreen *screen)
{
  return NULL;
}

static void
gdk_directfb_screen_broadcast_client_message (GdkScreen *screen,
					      GdkEvent  *event)
{
}

static gboolean
gdk_directfb_screen_get_setting (GdkScreen   *screen,
				 const gchar *name,
				 GValue      *value)
{
  return FALSE;
}

static GdkVisual *
gdk_directfb_screen_get_rgba_visual (GdkScreen *screen)
{
  return GDK_DIRECTFB_SCREEN (screen)->rgba_visual;
}

static GdkVisual *
gdk_directfb_screen_get_system_visual (GdkScreen *screen)
{
  return GDK_DIRECTFB_SCREEN (screen)->system_visual;
}

static gint
gdk_directfb_screen_visual_get_best_depth (GdkScreen *screen)
{
  GdkVisual *visual = GDK_DIRECTFB_SCREEN (screen)->visuals->data;

  return visual->depth;
}

static GdkVisualType
gdk_directfb_screen_visual_get_best_type (GdkScreen *screen)
{
  GdkVisual *visual = GDK_DIRECTFB_SCREEN (screen)->visuals->data;

  return visual->type;
}

static GdkVisual *
gdk_directfb_screen_visual_get_best (GdkScreen *screen)
{
  return GDK_DIRECTFB_SCREEN (screen)->visuals->data;
}

static gint
gdk_directfb_screen_visual_compare_depth (gconstpointer a,
					  gconstpointer b)
{
  const GdkVisual *visual = a;
  const gint *depth = b;

  return visual->depth == *depth ? 0 : -1;
}

static GdkVisual *
gdk_directfb_screen_visual_get_best_with_depth (GdkScreen *screen,
						gint       depth)
{
  GList *node;

  node = g_list_find_custom (GDK_DIRECTFB_SCREEN (screen)->visuals,
			     &depth,
			     gdk_directfb_screen_visual_compare_depth);

  return node ? node->data : NULL;
}

static gint
gdk_directfb_screen_visual_compare_type (gconstpointer a,
					 gconstpointer b)
{
  const GdkVisual *visual = a;
  const GdkVisualType *type = b;

  return visual->type == *type ? 0 : -1;
}

static GdkVisual *
gdk_directfb_screen_visual_get_best_with_type (GdkScreen    *screen,
					       GdkVisualType type)
{
  GList *node;

  node = g_list_find_custom (GDK_DIRECTFB_SCREEN (screen)->visuals,
			     &type,
			     gdk_directfb_screen_visual_compare_type);

  return node ? node->data : NULL;
}

struct gdk_directfb_screen_visual_compare_both_data {
    gint depth;
    GdkVisualType type;
};

static gint
gdk_directfb_screen_visual_compare_both (gconstpointer a,
					 gconstpointer b)
{
  const GdkVisual *visual = a;
  const struct gdk_directfb_screen_visual_compare_both_data *data = b;

  return visual->depth == data->depth && visual->type == data->type ? 0 : -1;
}

static GdkVisual *
gdk_directfb_screen_visual_get_best_with_both (GdkScreen    *screen,
					       gint          depth,
					       GdkVisualType type)
{
  GList *node;
  struct gdk_directfb_screen_visual_compare_both_data data = {
      .depth = depth,
      .type = type,
  };

  node = g_list_find_custom (GDK_DIRECTFB_SCREEN (screen)->visuals,
			     &data,
			     gdk_directfb_screen_visual_compare_both);

  return node ? node->data : NULL;
}

static void
gdk_directfb_screen_query_depths  (GdkScreen *screen,
				   gint     **depths,
				   gint      *count)
{
  GdkDirectfbScreen *directfb_screen = GDK_DIRECTFB_SCREEN (screen);

  if (G_UNLIKELY (! directfb_screen->depths))
    {
      GList *node;

      directfb_screen->depths =
	g_array_sized_new (FALSE, FALSE, sizeof (gint),
			   g_list_length (directfb_screen->visuals));

      for (node = directfb_screen->visuals; node; node = node->next)
	g_array_append_val (directfb_screen->depths,
			    GDK_VISUAL (node->data)->depth);
    }

  *depths = (gint *) directfb_screen->depths->data;
  *count = directfb_screen->depths->len;
}

static void
gdk_directfb_screen_query_visual_types (GdkScreen      *screen,
					GdkVisualType **visual_types,
					gint           *count)
{
  GdkDirectfbScreen *directfb_screen = GDK_DIRECTFB_SCREEN (screen);

  if (G_UNLIKELY (! directfb_screen->depths))
    {
      GList *node;

      directfb_screen->visual_types =
	g_array_sized_new (FALSE, FALSE, sizeof (GdkVisualType),
			   g_list_length (directfb_screen->visuals));

      for (node = directfb_screen->visuals; node; node = node->next)
	g_array_append_val (directfb_screen->visual_types,
			    GDK_VISUAL (node->data)->type);
    }

  *visual_types = (GdkVisualType *) directfb_screen->visual_types->data;
  *count = directfb_screen->visual_types->len;
}

static GList *
gdk_directfb_screen_list_visuals (GdkScreen *screen)
{
  return g_list_copy (GDK_DIRECTFB_SCREEN (screen)->visuals);
}

static void
gdk_directfb_screen_class_init (GdkDirectfbScreenClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);
  GdkScreenClass *screen_class = GDK_SCREEN_CLASS (klass);

  object_class->dispose = gdk_directfb_screen_dispose;
  object_class->finalize = gdk_directfb_screen_finalize;

  screen_class->get_display = gdk_directfb_screen_get_display;
  screen_class->get_width = gdk_directfb_screen_get_width;
  screen_class->get_height = gdk_directfb_screen_get_height;
  screen_class->get_width_mm = gdk_directfb_screen_get_width_mm;
  screen_class->get_height_mm = gdk_directfb_screen_get_height_mm;
  screen_class->get_number = gdk_directfb_screen_get_number;
  screen_class->get_root_window = gdk_directfb_screen_get_root_window;
  screen_class->get_n_monitors = gdk_directfb_screen_get_n_monitors;
  screen_class->get_primary_monitor = gdk_directfb_screen_get_primary_monitor;
  screen_class->get_monitor_width_mm = gdk_directfb_screen_get_monitor_width_mm;
  screen_class->get_monitor_height_mm = gdk_directfb_screen_get_monitor_height_mm;
  screen_class->get_monitor_plug_name = gdk_directfb_screen_get_monitor_plug_name;
  screen_class->get_monitor_geometry = gdk_directfb_screen_get_monitor_geometry;
  screen_class->get_monitor_workarea = gdk_directfb_screen_get_monitor_geometry;
  screen_class->is_composited = gdk_directfb_screen_is_composited;
  screen_class->make_display_name = gdk_directfb_screen_make_display_name;
  screen_class->get_active_window = gdk_directfb_screen_get_active_window;
  screen_class->get_window_stack = gdk_directfb_screen_get_window_stack;
  screen_class->broadcast_client_message = gdk_directfb_screen_broadcast_client_message;
  screen_class->get_setting = gdk_directfb_screen_get_setting;
  screen_class->get_rgba_visual = gdk_directfb_screen_get_rgba_visual;
  screen_class->get_system_visual = gdk_directfb_screen_get_system_visual;
  screen_class->visual_get_best_depth = gdk_directfb_screen_visual_get_best_depth;
  screen_class->visual_get_best_type = gdk_directfb_screen_visual_get_best_type;
  screen_class->visual_get_best = gdk_directfb_screen_visual_get_best;
  screen_class->visual_get_best_with_depth = gdk_directfb_screen_visual_get_best_with_depth;
  screen_class->visual_get_best_with_type = gdk_directfb_screen_visual_get_best_with_type;
  screen_class->visual_get_best_with_both = gdk_directfb_screen_visual_get_best_with_both;
  screen_class->query_depths = gdk_directfb_screen_query_depths;
  screen_class->query_visual_types = gdk_directfb_screen_query_visual_types;
  screen_class->list_visuals = gdk_directfb_screen_list_visuals;
}

/**
 * gdk_visual_decompose_mask:
 * @mask: A color mask
 * @shift: (out): The computed color shift
 * @prec: (out): The computed color precision
 *
 * Calculate the shift and precision values for the specified color mask.
 *
 * Since: 3.10
 */
static void
gdk_visual_decompose_mask (gulong mask,
			   gint  *shift,
			   gint  *prec)
{
  *shift = 0;
  *prec  = 0;

  while (!(mask & 0x1))
    {
      ++(*shift);
      mask >>= 1;
    }

  while (mask & 0x1)
    {
      ++(*prec);
      mask >>= 1;
    }
}

/**
 * gdk_directfb_screen_visual_new:
 * @screen: The #GdkScreen for which this visual is being created
 * @format: The DirectFB pixel format to support
 *
 * Creates a new #GdkVisual for the specified screen that supports the given
 * DirectFB pixel format.
 *
 * Since: 3.10
 */
static GdkVisual *
gdk_directfb_screen_visual_new (GdkScreen            *screen,
				DFBSurfacePixelFormat format)
{
  GdkVisual *visual = NULL;

  switch (format)
    {
    case DSPF_ARGB:
    case DSPF_RGB32:
      visual = g_object_new (GDK_TYPE_VISUAL, NULL);
      visual->type = GDK_VISUAL_TRUE_COLOR;
      visual->red_mask = 0x00FF0000;
      visual->green_mask = 0x0000FF00;
      visual->blue_mask = 0x000000FF;
      visual->bits_per_rgb = 8;
      break;

    case DSPF_A8:
    case DSPF_RGB24:
    case DSPF_ARGB1555:
    case DSPF_RGB16:
    case DSPF_YUY2:
    case DSPF_RGB332:
    case DSPF_YV12:
    case DSPF_ARGB4444:
    case DSPF_A4:
    case DSPF_RGB444:
    case DSPF_RGB555:
    case DSPF_BGR555:
    case DSPF_UYVY:
    case DSPF_I420:
    case DSPF_LUT8:
    case DSPF_ALUT44:
    case DSPF_AiRGB:
    case DSPF_A1:
    case DSPF_NV12:
    case DSPF_NV16:
    case DSPF_ARGB2554:
    case DSPF_NV21:
    case DSPF_AYUV:
    case DSPF_ARGB1666:
    case DSPF_ARGB6666:
    case DSPF_RGB18:
    case DSPF_LUT2:
#if DFB_NUM_PIXELFORMATS > 29
    case DSPF_RGBA4444:
    case DSPF_LUT4:
    case DSPF_RGBA5551:
    case DSPF_YUV444P:
    case DSPF_ARGB8565:
    case DSPF_AVYU:
    case DSPF_VYU:
    case DSPF_A1_LSB:
    case DSPF_YV16:
    case DSPF_ABGR:
    case DSPF_ALUT8:
#if DFB_NUM_PIXELFORMATS > 40
    case DSPF_RGBAF88871:
#endif
#endif
    case DSPF_UNKNOWN:
      g_assert_not_reached ();
    }

  visual->screen = screen;
  visual->depth = DFB_BITS_PER_PIXEL (format);
#if G_BYTE_ORDER == G_BIG_ENDIAN
  visual->byte_order = GDK_MSB_FIRST;
#else
  visual->byte_order = GDK_LSB_FIRST;
#endif

  switch (visual->type)
    {
    case GDK_VISUAL_TRUE_COLOR:
    case GDK_VISUAL_DIRECT_COLOR:
      gdk_visual_decompose_mask (visual->red_mask,
				 &visual->red_shift,
				 &visual->red_prec);
      gdk_visual_decompose_mask (visual->green_mask,
				 &visual->green_shift,
				 &visual->green_prec);
      gdk_visual_decompose_mask (visual->blue_mask,
				 &visual->blue_shift,
				 &visual->blue_prec);
      visual->colormap_size = 1 << MAX (visual->red_prec,
					MAX (visual->green_prec,
					     visual->blue_prec));
      break;

    case GDK_VISUAL_PSEUDO_COLOR:
    case GDK_VISUAL_STATIC_COLOR:
    case GDK_VISUAL_GRAYSCALE:
    case GDK_VISUAL_STATIC_GRAY:
      visual->red_mask = visual->green_mask = visual->blue_mask = 0;
      visual->red_shift = visual->green_shift = visual->blue_shift = 0;
      visual->red_prec = visual->red_prec = visual->blue_prec = 0;
      visual->colormap_size = 1 << visual->depth;
      break;
    }

  return visual;
}

/**
 * gdk_directfb_screen_visual_compare:
 * @a_: A #GdkVisual object
 * @b_: A #GdkVisual object
 *
 * This function is a callback for g_list_insert_sorted(). Compares two visuals
 * and determines which is better.
 *
 * Returns: An integer per the #GCompareFunc
 *
 * Since: 3.10
 */
static gint
gdk_directfb_screen_visual_compare (gconstpointer a_,
				    gconstpointer b_)
{
  const GdkVisual *a = a_;
  const GdkVisual *b = b_;

  if (a->depth < b->depth)
    return -1;
  else if (a->depth > b->depth)
    return 1;
  else
    {
      if (a->type < b->type)
	return -1;
      else if (a->type > b->type)
	return 1;
      else
	return 0;
    }
}

/**
 * gdk_directfb_screen_find_display_layer:
 * @layer_id: A #DFBDisplayLayerID
 * @desc: A #DFBDisplayLayerDescription structure
 * @data: A #GdkDirectFBScreen object
 *
 * This function is a callback for IDirectFBScreen::EnumDisplayLayers(). It
 * attempts to find the DirectFB display layer for the current screen which
 * supports graphics.
 *
 * Returns: DFENUM_CANCEL if successful
 */
static DFBEnumerationResult
gdk_directfb_screen_find_display_layer (DFBDisplayLayerID          layer_id,
					DFBDisplayLayerDescription desc,
					void                      *data)
{
  GdkDirectfbScreen *directfb_screen = data;
  DFBResult result;
  IDirectFB *dfb;

  if (! (DLTF_GRAPHICS & desc.type))
    return DFENUM_OK;

  dfb = gdk_directfb_display_get_context (directfb_screen->display);
  result = dfb->GetDisplayLayer (dfb, layer_id, &directfb_screen->layer);
  if (G_UNLIKELY (DFB_OK != result))
    {
      g_warning ("%s", DirectFBErrorString (result));
      return DFENUM_OK;
    }

  return DFENUM_CANCEL;
}

/**
 * gdk_directfb_screen_new:
 * @display: The #GdkDisplay for which this screen is being created
 * @id: A DirectFB screen ID
 *
 * Creates a new #GdkScreen for the specified DirectFB screen ID. If the
 * specified screen ID is found and a corresponding display layer with graphics
 * capability is found then the required visuals are created.
 *
 * Returns: (transfer full): A new #GdkScreen
 *
 * Since: 3.10
 */
GdkScreen *
gdk_directfb_screen_new (GdkDisplay *display,
			 DFBScreenID id)
{
  gsize i;
  IDirectFB *dfb;
  DFBResult result;
  GdkScreen *screen;
  DFBDisplayLayerConfig config;
  GdkDirectfbScreen *directfb_screen;

  const DFBSurfacePixelFormat formats[] = {
    DSPF_ARGB,
    DSPF_RGB32
  };

  screen = g_object_new (GDK_TYPE_DIRECTFB_SCREEN, NULL);
  directfb_screen = GDK_DIRECTFB_SCREEN (screen);
  directfb_screen->display = display;

  dfb = gdk_directfb_display_get_context (display);
  result = dfb->GetScreen (dfb, id, &directfb_screen->screen);
  if (G_UNLIKELY (DFB_OK != result))
    {
      g_warning ("%s", DirectFBErrorString (result));
      goto error;
    }

  directfb_screen->layer = NULL;
  result = directfb_screen->screen->EnumDisplayLayers (directfb_screen->screen,
						       gdk_directfb_screen_find_display_layer,
						       directfb_screen);
  if (G_UNLIKELY (DFB_OK != result))
    {
      g_warning ("%s", DirectFBErrorString (result));
      goto error;
    }

  if (G_UNLIKELY (! directfb_screen->layer))
    {
      g_warning ("unable to find graphics layer for screen 0x%04x", id);
      goto error;
    }

  result = directfb_screen->layer->SetCooperativeLevel (directfb_screen->layer,
							DLSCL_ADMINISTRATIVE);
  if (G_UNLIKELY (DFB_OK != result))
    g_warning ("%s", DirectFBErrorString (result));

  result = directfb_screen->layer->GetConfiguration (directfb_screen->layer,
						     &config);
  if (G_UNLIKELY (DFB_OK != result))
    {
      g_warning ("%s", DirectFBErrorString (result));
      goto error;
    }

  for (i = 0; i < G_N_ELEMENTS (formats); ++i)
    {
      GdkVisual *visual;

      visual = gdk_directfb_screen_visual_new (screen, formats[i]);
      if (G_UNLIKELY (! visual))
	continue;

      if (DSPF_ARGB == formats[i])
	directfb_screen->rgba_visual = visual;

      if (config.pixelformat == formats[i])
	directfb_screen->system_visual = visual;

      directfb_screen->visuals =
	g_list_insert_sorted (directfb_screen->visuals,
			      visual,
			      gdk_directfb_screen_visual_compare);
    }

  if (G_UNLIKELY (! directfb_screen->rgba_visual))
    {
      GdkVisual *visual;

      visual = gdk_directfb_screen_visual_new (screen, DSPF_ARGB);
      if (G_UNLIKELY (! visual))
	{
	  g_warning ("failed to create RGBA visual");
	  goto error;
	}

      if (DSPF_ARGB == config.pixelformat)
	directfb_screen->system_visual = visual;

      directfb_screen->rgba_visual = visual;
      directfb_screen->visuals =
	g_list_insert_sorted (directfb_screen->visuals,
			      visual,
			      gdk_directfb_screen_visual_compare);
    }

  if (G_UNLIKELY (! directfb_screen->system_visual))
    {
      GdkVisual *visual;

      visual = gdk_directfb_screen_visual_new (screen, config.pixelformat);
      if (G_UNLIKELY (! visual))
	{
	  g_warning ("failed to create system visual");
	  goto error;
	}

      directfb_screen->system_visual = visual;
      directfb_screen->visuals =
	g_list_insert_sorted (directfb_screen->visuals,
			      visual,
			      gdk_directfb_screen_visual_compare);
    }

  (void) directfb_screen->layer->EnableCursor (directfb_screen->layer, 1);
  directfb_screen->root_window = gdk_directfb_root_window_new (screen);
  gdk_directfb_screen_add_window (screen, directfb_screen->root_window);

  return screen;

error:

  if (screen)
    g_object_unref (screen);

  return NULL;
}

/**
 * gdk_directfb_screen_get_screen:
 * @screen: A #GdkScreen object
 *
 * Get the underlying #IDirectFBScreen object.
 *
 * Returns: (transfer none): A #IDirectFBScreen
 *
 * Since: 3.10
 */
IDirectFBScreen *
gdk_directfb_screen_get_screen (const GdkScreen *screen)
{
  g_return_val_if_fail (GDK_IS_DIRECTFB_SCREEN (screen), NULL);

  return GDK_DIRECTFB_SCREEN (screen)->screen;
}

/**
 * gdk_directfb_screen_get_display_layer:
 * @screen: A #GdkScreen object
 *
 * Get the underlying #IDirectFBDisplayLayer object.
 *
 * Returns: (transfer none): A #IDirectFBDisplayLayer
 *
 * Since: 3.10
 */
IDirectFBDisplayLayer *
gdk_directfb_screen_get_display_layer (const GdkScreen *screen)
{
  g_return_val_if_fail (GDK_IS_DIRECTFB_SCREEN (screen), NULL);

  return GDK_DIRECTFB_SCREEN (screen)->layer;
}

/**
 * gdk_directfb_screen_add_window:
 * @screen: A #GdkScreen object
 * @window: A #GdkWindow object
 *
 * As windows are created they are stored within the associated screen object
 * so they can be retrieved by DirectFB screen ID by the event handler.
 *
 * This function adds a newly created window to the window list for the
 * specified screen.
 *
 * Since: 3.10
 */
void
gdk_directfb_screen_add_window (GdkScreen *screen,
				GdkWindow *window)
{
  GdkDirectfbScreen *directfb_screen;

  g_return_if_fail (GDK_IS_DIRECTFB_SCREEN (screen));

  directfb_screen = GDK_DIRECTFB_SCREEN (screen);

  if (window)
    directfb_screen->windows = g_list_prepend (directfb_screen->windows,
					       g_object_ref (window));
}

/**
 * gdk_directfb_screen_find_window:
 * @a_: A #GdkWindow object
 * @b_: A #DFBWindowID pointer
 *
 * This function is a callback for g_list_find_custom(). It identifies the
 * window with the specified ID.
 *
 * Returns: An integer per #GCompareFunc
 *
 * Since: 3.10
 */
static gint
gdk_directfb_screen_find_window (gconstpointer a_,
				 gconstpointer b_)
{
  DFBWindowID a, *b = (DFBWindowID *) b_;
  GdkWindow *window = (GdkWindow *) a_;
  IDirectFBWindow *dfb_window;

  dfb_window = gdk_directfb_window_get_window (window);
  if (dfb_window)
    {
      (void) dfb_window->GetID (dfb_window, &a);
      return a == *b ? 0 : -1;
    }

  return -1;
}

/**
 * gdk_directfb_screen_remove_window:
 * @screen: A #GdkScreen object
 * @window: A #GdkWindow object
 *
 * Removes the specified window from the associated window list for a screen.
 *
 * Since: 3.10
 */
void
gdk_directfb_screen_remove_window (GdkScreen *screen,
				   GdkWindow *window)
{
  GdkDirectfbScreen *directfb_screen;
  IDirectFBWindow *dfb_window;
  DFBWindowID id;
  GList *node;

  g_return_if_fail (GDK_IS_DIRECTFB_SCREEN (screen));

  directfb_screen = GDK_DIRECTFB_SCREEN (screen);

  dfb_window = gdk_directfb_window_get_window (window);
  (void) dfb_window->GetID (dfb_window, &id);

  node = g_list_find_custom (directfb_screen->windows,
			     &id, gdk_directfb_screen_find_window);
  if (node)
    {
      directfb_screen->windows =
	g_list_remove_link (directfb_screen->windows, node);

      g_list_free_full (node, g_object_unref);
    }
}

/**
 * gdk_directfb_screen_get_window:
 * @screen: A #GdkScreen object
 * @id: A #DFBWindowID
 *
 * Attempts to find the window identified by the specified ID in the window
 * list for a screen.
 *
 * Returns: (transfer none): A #GdkWindow or #NULL if none exists
 *
 * Since: 3.10
 */
GdkWindow *
gdk_directfb_screen_get_window (GdkScreen   *screen,
				DFBWindowID  id)
{
  GdkDirectfbScreen *directfb_screen;
  GdkWindow *window = NULL;
  GList *node;

  g_return_val_if_fail (GDK_IS_DIRECTFB_SCREEN (screen), NULL);

  directfb_screen = GDK_DIRECTFB_SCREEN (screen);

  node = g_list_find_custom (directfb_screen->windows, &id,
			     gdk_directfb_screen_find_window);
  if (node)
    {
      window = node->data;

      directfb_screen->windows =
	g_list_remove_link (directfb_screen->windows, node);

      directfb_screen->windows =
	g_list_concat (node, directfb_screen->windows);
    }

  return window;
}
