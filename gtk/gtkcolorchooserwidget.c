/* GTK - The GIMP Toolkit
 *
 * Copyright (C) 2012 Red Hat, Inc.
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
 * License along with this library; if not, write to the
 * Free Software Foundation, Inc., 59 Temple Place - Suite 330,
 * Boston, MA 02111-1307, USA.
 */

#include "config.h"

#include "gtkcolorchooserprivate.h"
#include "gtkcolorchooserwidget.h"
#include "gtkcoloreditor.h"
#include "gtkcolorswatch.h"
#include "gtkbox.h"
#include "gtkgrid.h"
#include "gtkhsv.h"
#include "gtklabel.h"
#include "gtkorientable.h"
#include "gtkintl.h"

struct _GtkColorChooserWidgetPrivate
{
  GtkWidget *palette;
  GtkWidget *editor;

  GtkWidget *colors;
  GtkWidget *grays;
  GtkWidget *custom;

  GtkColorSwatch *current;

  GSettings *settings;
};

enum
{
  PROP_ZERO,
  PROP_COLOR
};

static void gtk_color_chooser_widget_iface_init (GtkColorChooserInterface *iface);

G_DEFINE_TYPE_WITH_CODE (GtkColorChooserWidget, gtk_color_chooser_widget, GTK_TYPE_BOX,
                         G_IMPLEMENT_INTERFACE (GTK_TYPE_COLOR_CHOOSER,
                                                gtk_color_chooser_widget_iface_init))

static void
select_swatch (GtkColorChooserWidget *cc,
               GtkColorSwatch        *swatch)
{
  GdkRGBA color;

  if (cc->priv->current == swatch)
    return;
  if (cc->priv->current != NULL)
    gtk_color_swatch_set_selected (cc->priv->current, FALSE);
  gtk_color_swatch_set_selected (swatch, TRUE);
  cc->priv->current = swatch;
  gtk_color_swatch_get_color (swatch, &color);
  g_settings_set (cc->priv->settings, "selected-color", "(bdddd)",
                  TRUE, color.red, color.green, color.blue, color.alpha);

  g_object_notify (G_OBJECT (cc), "color");
}

static void add_custom (GtkColorChooserWidget *cc, GtkColorSwatch *swatch);
static void save_custom (GtkColorChooserWidget *cc);

static void
button_activate (GtkColorSwatch        *swatch,
                 GtkColorChooserWidget *cc)
{
  add_custom (cc, swatch);
}

static void
swatch_activate (GtkColorSwatch        *swatch,
                 GtkColorChooserWidget *cc)
{
  GdkRGBA color;

  gtk_color_swatch_get_color (swatch, &color);
  _gtk_color_chooser_color_activated (GTK_COLOR_CHOOSER (cc), &color);
}

static void
swatch_customize (GtkColorSwatch        *swatch,
                  GtkColorChooserWidget *cc)
{
  GdkRGBA color;

  gtk_color_swatch_get_color (swatch, &color);
  gtk_color_chooser_set_color (GTK_COLOR_CHOOSER (cc->priv->editor), &color);

  gtk_widget_hide (cc->priv->palette);
  gtk_widget_show (cc->priv->editor);
}

static void
swatch_selected (GtkColorSwatch        *swatch,
                 GParamSpec            *pspec,
                 GtkColorChooserWidget *cc)
{
  select_swatch (cc, swatch);
}

static void
connect_swatch_signals (GtkWidget *p, gpointer data)
{
  g_signal_connect (p, "activate", G_CALLBACK (swatch_activate), data);
  g_signal_connect (p, "customize", G_CALLBACK (swatch_customize), data);
  g_signal_connect (p, "notify::selected", G_CALLBACK (swatch_selected), data);
}

static void
connect_button_signals (GtkWidget *p, gpointer data)
{
  g_signal_connect (p, "activate", G_CALLBACK (button_activate), data);
}

static void
connect_custom_signals (GtkWidget *p, gpointer data)
{
  connect_swatch_signals (p, data);
  g_signal_connect_swapped (p, "notify::color", G_CALLBACK (save_custom), data);
}

static void
save_custom (GtkColorChooserWidget *cc)
{
  GVariantBuilder builder;
  GVariant *variant;
  GdkRGBA color;
  GList *children, *l;

  g_variant_builder_init (&builder, G_VARIANT_TYPE ("a(dddd)"));

  children = gtk_container_get_children (GTK_CONTAINER (cc->priv->custom));
  for (l = children; l; l = l->next)
    {
      if (gtk_color_swatch_get_color (GTK_COLOR_SWATCH (l->data), &color))
        {
          g_variant_builder_add (&builder, "(dddd)",
                                 color.red, color.green, color.blue, color.alpha);
        }
    }

  g_list_free (children);

  variant = g_variant_builder_end (&builder);
  g_settings_set_value (cc->priv->settings, "custom-colors", variant);
}

static void
add_custom (GtkColorChooserWidget *cc, GtkColorSwatch *swatch)
{
  GList *children;
  gint n_children;
  GdkRGBA color;
  GtkWidget *p;

  children = gtk_container_get_children (GTK_CONTAINER (cc->priv->custom));
  n_children = g_list_length (children);
  g_list_free (children);
  if (n_children >= 9)
    {
      gtk_widget_error_bell (GTK_WIDGET (cc));
      return;
    }

  gtk_color_swatch_set_corner_radii (GTK_COLOR_SWATCH (swatch), 10, 1, 1, 10);

  color.red = g_random_double_range (0, 1);
  color.green = g_random_double_range (0, 1);
  color.blue = g_random_double_range (0, 1);
  color.alpha = 1.0;

  p = gtk_color_swatch_new ();
  gtk_color_swatch_set_color (GTK_COLOR_SWATCH (p), &color);
  gtk_color_swatch_set_can_drop (GTK_COLOR_SWATCH (p), TRUE);
  connect_custom_signals (p, cc);

  if (n_children == 1)
    gtk_color_swatch_set_corner_radii (GTK_COLOR_SWATCH (p), 1, 10, 10, 1);
  else
    gtk_color_swatch_set_corner_radii (GTK_COLOR_SWATCH (p), 1, 1, 1, 1);

  gtk_grid_insert_next_to (GTK_GRID (cc->priv->custom), GTK_WIDGET (swatch), GTK_POS_RIGHT);
  gtk_grid_attach (GTK_GRID (cc->priv->custom), p, 1, 0, 1, 1);

  gtk_widget_show (p);

  save_custom (cc);
}

static void
gtk_color_chooser_widget_init (GtkColorChooserWidget *cc)
{
  GtkWidget *grid;
  GtkWidget *p;
  GtkWidget *button;
  GtkWidget *label;
  gint i;
  GdkRGBA color, color1, color2;
  gdouble h, s, v;
  GVariant *variant;
  GVariantIter iter;
  gboolean selected;
  const gchar *default_palette[9] = {
    "red",
    "orange",
    "yellow",
    "green",
    "blue",
    "purple",
    "brown",
    "darkgray",
    "gray"
  };

  cc->priv = G_TYPE_INSTANCE_GET_PRIVATE (cc, GTK_TYPE_COLOR_CHOOSER_WIDGET, GtkColorChooserWidgetPrivate);

  gtk_orientable_set_orientation (GTK_ORIENTABLE (cc), GTK_ORIENTATION_VERTICAL);
  cc->priv->palette = gtk_box_new (GTK_ORIENTATION_VERTICAL, 0);
  gtk_container_add (GTK_CONTAINER (cc), cc->priv->palette);

  cc->priv->colors = grid = gtk_grid_new ();
  g_object_set (grid, "margin", 12, NULL);
  gtk_widget_set_margin_bottom (grid, 12);
  gtk_grid_set_row_spacing (GTK_GRID (grid), 2);
  gtk_grid_set_column_spacing (GTK_GRID (grid), 4);
  gtk_container_add (GTK_CONTAINER (cc->priv->palette), grid);

  for (i = 0; i < 9; i++)
    {
       gdk_rgba_parse (&color, default_palette[i]);
       gtk_rgb_to_hsv (color.red, color.green, color.blue, &h, &s, &v);
       gtk_hsv_to_rgb (h, s / 2, (v + 1) / 2, &color1.red, &color1.green, &color1.blue);
       color1.alpha = color.alpha;

       gtk_hsv_to_rgb (h, s, v * 3 / 4, &color2.red, &color2.green, &color2.blue);
       color2.alpha = color.alpha;

       p = gtk_color_swatch_new ();
       connect_swatch_signals (p, cc);
       gtk_color_swatch_set_corner_radii (GTK_COLOR_SWATCH (p), 10, 10, 1, 1);
       gtk_color_swatch_set_color (GTK_COLOR_SWATCH (p), &color1);

       gtk_grid_attach (GTK_GRID (grid), p, i, 0, 1, 1);

       p = gtk_color_swatch_new ();
       connect_swatch_signals (p, cc);
       gtk_color_swatch_set_corner_radii (GTK_COLOR_SWATCH (p), 1, 1, 1, 1);
       gtk_color_swatch_set_color (GTK_COLOR_SWATCH (p), &color);
       gtk_grid_attach (GTK_GRID (grid), p, i, 1, 1, 1);

       p = gtk_color_swatch_new ();
       connect_swatch_signals (p, cc);
       gtk_color_swatch_set_corner_radii (GTK_COLOR_SWATCH (p), 1, 1, 10, 10);
       gtk_color_swatch_set_color (GTK_COLOR_SWATCH (p), &color2);
       gtk_grid_attach (GTK_GRID (grid), p, i, 2, 1, 1);
    }

  cc->priv->grays = grid = gtk_grid_new ();
  g_object_set (grid, "margin", 12, "margin-top", 0, NULL);
  gtk_grid_set_column_spacing (GTK_GRID (grid), 4);
  gtk_container_add (GTK_CONTAINER (cc->priv->palette), grid);

  for (i = 0; i < 9; i++)
    {
       color.red = color.green = color.blue = i / 8.0;
       color.alpha = 1.0;

       p = gtk_color_swatch_new ();
       connect_swatch_signals (p, cc);
       if (i == 0)
         gtk_color_swatch_set_corner_radii (GTK_COLOR_SWATCH (p), 10, 1, 1, 10);
       else if (i == 8)
         gtk_color_swatch_set_corner_radii (GTK_COLOR_SWATCH (p), 1, 10, 10, 1);
       else
         gtk_color_swatch_set_corner_radii (GTK_COLOR_SWATCH (p), 1, 1, 1, 1);

       gtk_color_swatch_set_color (GTK_COLOR_SWATCH (p), &color);
       gtk_grid_attach (GTK_GRID (grid), p, i, 0, 1, 1);
    }

  label = gtk_label_new (_("Custom color"));
  gtk_widget_set_halign (label, GTK_ALIGN_START);
  gtk_container_add (GTK_CONTAINER (cc->priv->palette), label);
  g_object_set (grid, "margin", 12, "margin-top", 0, NULL);

  cc->priv->custom = grid = gtk_grid_new ();
  g_object_set (grid, "margin", 12, "margin-top", 0, NULL);
  gtk_grid_set_column_spacing (GTK_GRID (grid), 4);
  gtk_container_add (GTK_CONTAINER (cc->priv->palette), grid);

  button = gtk_color_swatch_new ();
  gtk_color_swatch_set_corner_radii (GTK_COLOR_SWATCH (button), 10, 10, 10, 10);
  connect_button_signals (button, cc);
  gtk_color_swatch_set_icon (GTK_COLOR_SWATCH (button), "list-add-symbolic");
  gtk_grid_attach (GTK_GRID (grid), button, 0, 0, 1, 1);

  cc->priv->settings = g_settings_new_with_path ("org.gtk.Settings.ColorChooser",
                                                 "/org/gtk/settings/color-chooser/");
  variant = g_settings_get_value (cc->priv->settings, "custom-colors");
  g_variant_iter_init (&iter, variant);
  i = 0;
  while (g_variant_iter_loop (&iter, "(dddd)", &color.red, &color.green, &color.blue, &color.alpha))
    {
      i++;
      p = gtk_color_swatch_new ();
      gtk_color_swatch_set_corner_radii (GTK_COLOR_SWATCH (p), 1, 1, 1, 1);
      gtk_color_swatch_set_color (GTK_COLOR_SWATCH (p), &color);
      gtk_color_swatch_set_can_drop (GTK_COLOR_SWATCH (p), TRUE);
      connect_custom_signals (p, cc);
      gtk_grid_attach (GTK_GRID (grid), p, i, 0, 1, 1);

      if (i == 8)
        break;
    }
  g_variant_unref (variant);

  if (i > 0)
    {
      gtk_color_swatch_set_corner_radii (GTK_COLOR_SWATCH (p), 1, 10, 10, 1);
      gtk_color_swatch_set_corner_radii (GTK_COLOR_SWATCH (button), 10, 1, 1, 10);
    }

  cc->priv->editor = gtk_color_editor_new ();
  gtk_container_add (GTK_CONTAINER (cc), cc->priv->editor);

  g_settings_get (cc->priv->settings, "selected-color", "(bdddd)",
                  &selected,
                  &color.red, &color.green, &color.blue, &color.alpha);
  if (selected)
    gtk_color_chooser_set_color (GTK_COLOR_CHOOSER (cc), &color);

  gtk_widget_show_all (GTK_WIDGET (cc));
  gtk_widget_hide (GTK_WIDGET (cc->priv->editor));
  gtk_widget_hide (GTK_WIDGET (cc));

  gtk_widget_set_no_show_all (cc->priv->palette, TRUE);
  gtk_widget_set_no_show_all (cc->priv->editor, TRUE);
}

static void
gtk_color_chooser_widget_get_property (GObject    *object,
                                       guint       prop_id,
                                       GValue     *value,
                                       GParamSpec *pspec)
{
  GtkColorChooser *cc = GTK_COLOR_CHOOSER (object);

  switch (prop_id)
    {
    case PROP_COLOR:
      {
        GdkRGBA color;

        gtk_color_chooser_get_color (cc, &color);
        g_value_set_boxed (value, &color);
      }
    break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
    }
}

static void
gtk_color_chooser_widget_set_property (GObject      *object,
                                       guint         prop_id,
                                       const GValue *value,
                                       GParamSpec   *pspec)
{
  GtkColorChooserWidget *cc = GTK_COLOR_CHOOSER_WIDGET (object);

  switch (prop_id)
    {
    case PROP_COLOR:
      gtk_color_chooser_set_color (GTK_COLOR_CHOOSER (cc),
                                   g_value_get_boxed (value));
    break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
    }
}

static void
gtk_color_chooser_widget_finalize (GObject *object)
{
  GtkColorChooserWidget *cc = GTK_COLOR_CHOOSER_WIDGET (object);

  g_object_unref (cc->priv->settings);

  G_OBJECT_CLASS (gtk_color_chooser_widget_parent_class)->finalize (object);
}

static void
gtk_color_chooser_widget_class_init (GtkColorChooserWidgetClass *class)
{
  GObjectClass *object_class = G_OBJECT_CLASS (class);

  object_class->get_property = gtk_color_chooser_widget_get_property;
  object_class->set_property = gtk_color_chooser_widget_set_property;
  object_class->finalize = gtk_color_chooser_widget_finalize;

  g_object_class_override_property (object_class, PROP_COLOR, "color");

  g_type_class_add_private (object_class, sizeof (GtkColorChooserWidgetPrivate));
}

static void
gtk_color_chooser_widget_get_color (GtkColorChooser *chooser,
                                    GdkRGBA         *color)
{
  GtkColorChooserWidget *cc = GTK_COLOR_CHOOSER_WIDGET (chooser);

  if (gtk_widget_get_visible (cc->priv->editor))
    gtk_color_chooser_get_color (GTK_COLOR_CHOOSER (cc->priv->editor), color);
  else if (cc->priv->current)
    gtk_color_swatch_get_color (cc->priv->current, color);
  else
    {
      color->red = 1.0;
      color->green = 1.0;
      color->blue = 1.0;
      color->alpha = 1.0;
    }
}

static void
gtk_color_chooser_widget_set_color (GtkColorChooser *chooser,
                                    const GdkRGBA   *color)
{
  GtkColorChooserWidget *cc = GTK_COLOR_CHOOSER_WIDGET (chooser);
  GList *children, *l;
  GtkColorSwatch *swatch;
  GdkRGBA c;
  GtkWidget *grids[3];
  gint i;

  grids[0] = cc->priv->colors;
  grids[1] = cc->priv->grays;
  grids[2] = cc->priv->custom;

  for (i = 0; i < 3; i++)
    {
      children = gtk_container_get_children (GTK_CONTAINER (grids[i]));
      for (l = children; l; l = l->next)
        {
          swatch = l->data;
          gtk_color_swatch_get_color (swatch, &c);
          if (gdk_rgba_equal (color, &c))
            {
              select_swatch (cc, swatch);
              g_list_free (children);
              return;
            }
        }
      g_list_free (children);
    }

  /* FIXME: add new custom color */
}

static void
gtk_color_chooser_widget_iface_init (GtkColorChooserInterface *iface)
{
  iface->get_color = gtk_color_chooser_widget_get_color;
  iface->set_color = gtk_color_chooser_widget_set_color;
}

GtkWidget *
gtk_color_chooser_widget_new (void)
{
  return g_object_new (GTK_TYPE_COLOR_CHOOSER_WIDGET, NULL);
}