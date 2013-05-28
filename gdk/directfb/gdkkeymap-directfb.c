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

#include "gdkdevice-directfb.h"
#include "gdkdevicemanager-directfb.h"
#include "gdkdisplay-directfb.h"
#include "gdkkeymap-directfb.h"
#include "gdkprivate-directfb.h"

G_DEFINE_TYPE (GdkDirectfbKeymap, gdk_directfb_keymap, GDK_TYPE_KEYMAP)

static void
gdk_directfb_keymap_init (GdkDirectfbKeymap *keymap)
{
}

static void
gdk_directfb_keymap_dispose (GObject *object)
{
  GdkDirectfbKeymap *directfb_keymap = GDK_DIRECTFB_KEYMAP (object);

  if (directfb_keymap->device)
    g_object_unref (directfb_keymap->device);

  G_OBJECT_CLASS (gdk_directfb_keymap_parent_class)->dispose (object);

  directfb_keymap->device = NULL;
}

static void
gdk_directfb_keymap_finalize (GObject *object)
{
  GdkDirectfbKeymap *directfb_keymap = GDK_DIRECTFB_KEYMAP (object);

  g_free (directfb_keymap->keymap);

  G_OBJECT_CLASS (gdk_directfb_keymap_parent_class)->finalize (object);
}

static PangoDirection
gdk_directfb_keymap_get_direction (GdkKeymap *keymap)
{
  return PANGO_DIRECTION_NEUTRAL;
}

static gboolean
gdk_directfb_keymap_have_bidi_layouts (GdkKeymap *keymap)
{
  return FALSE;
}

static gboolean
gdk_directfb_keymap_get_lock_state (GdkKeymap              *keymap,
				    DFBInputDeviceLockState key)
{
  GdkDirectfbKeymap *directfb_keymap = GDK_DIRECTFB_KEYMAP (keymap);
  IDirectFBInputDevice *input_device;
  DFBInputDeviceLockState state;
  DFBResult result;

  input_device = gdk_directfb_device_get_input_device (directfb_keymap->device);
  if (G_UNLIKELY (! input_device))
    return FALSE;

  result = input_device->GetLockState (input_device, &state);
  if (DFB_OK != result)
    {
      g_warning ("%s", DirectFBErrorString (result));
      return FALSE;
    }

  return state & key ? TRUE : FALSE;
}

static gboolean
gdk_directfb_keymap_get_caps_lock_state (GdkKeymap *keymap)
{
  return gdk_directfb_keymap_get_lock_state (keymap, DILS_CAPS);
}

static gboolean
gdk_directfb_keymap_get_num_lock_state (GdkKeymap *keymap)
{
  return gdk_directfb_keymap_get_lock_state (keymap, DILS_NUM);
}

static gboolean
gdk_directfb_keymap_get_entries_for_keyval (GdkKeymap     *keymap,
					    guint          keyval,
					    GdkKeymapKey **keys,
					    gint          *n_keys)
{
  GdkDirectfbKeymap *directfb_keymap = GDK_DIRECTFB_KEYMAP (keymap);
  gboolean result;
  GArray *entries;
  gsize i;

  if (G_UNLIKELY (! directfb_keymap->keymap))
    return FALSE;

  entries = g_array_new (FALSE, FALSE, sizeof (GdkKeymapKey));

  for (i = 0; i <= directfb_keymap->max_keycode; ++i)
    {
      gsize j, index;

      index = i - directfb_keymap->min_keycode;

      for (j = 0; j < 4; ++j)
	if (directfb_keymap->keymap[index * 4 + j] == keyval)
	  {
	    GdkKeymapKey key = {
		i,
		j % 2,
		j < DIKSI_BASE_SHIFT ? 1 : 0
	    };

	    g_array_append_val (entries, key);
	  }
    }

  if (G_LIKELY (entries->len))
    {
      *keys = (GdkKeymapKey *) entries->data;
      *n_keys = entries->len;
      g_array_free (entries, FALSE);
      result = TRUE;
    }
  else
    {
      *keys = NULL;
      *n_keys = 0;
      g_array_free (entries, TRUE);
      result = FALSE;
    }

  return result;
}

static gboolean
gdk_directfb_keymap_get_entries_for_keycode (GdkKeymap     *keymap,
					     guint          hardware_keycode,
					     GdkKeymapKey **keys,
					     guint        **keyvals,
					     gint          *n_entries)
{
  GdkDirectfbKeymap *directfb_keymap = GDK_DIRECTFB_KEYMAP (keymap);
  gsize n = 0;

  if (G_UNLIKELY (! directfb_keymap->keymap))
    return FALSE;

  if (hardware_keycode >= directfb_keymap->min_keycode &&
      hardware_keycode <= directfb_keymap->max_keycode)
    {
      gsize i, j, index;

      index = (hardware_keycode - directfb_keymap->min_keycode) * 4;

      for (i = 0; i < 4; ++i)
	if (GDK_KEY_VoidSymbol != directfb_keymap->keymap[index + i])
	  ++n;

      if (n)
	{
	  if (keys)
	    {
	      *keys = g_new (GdkKeymapKey, n);

	      for (i = 0, j = 0; i < 4; ++i)
		if (GDK_KEY_VoidSymbol != directfb_keymap->keymap[index + i])
		  {
		    (*keys)[j].keycode = hardware_keycode;
		    (*keys)[j].level = j % 2;
		    (*keys)[j].group = j > DIKSI_BASE_SHIFT ? 1 : 0;
		    ++j;
		  }
	    }

	  if (keyvals)
	    {
	      *keyvals = g_new (guint, n);

	      for (i = 0, j = 0; i < 4; ++i)
		if (GDK_KEY_VoidSymbol != directfb_keymap->keymap[index + i])
		  {
		    (*keyvals)[j] = directfb_keymap->keymap[index + i];
		    ++j;
		  }
	    }
	}
    }

  *n_entries = n;
  return n > 0;
}

static guint
gdk_directfb_keymap_lookup_key (GdkKeymap          *keymap,
				const GdkKeymapKey *key)
{
  return key->keycode;
}

static gboolean
gdk_directfb_keymap_translate_keyboard_state (GdkKeymap       *keymap,
					      guint            hardware_keycode,
					      GdkModifierType  state,
					      gint             group,
					      guint           *keyval,
					      gint            *effective_group,
					      gint            *level,
					      GdkModifierType *consumed_modifiers)
{
  GdkDirectfbKeymap *directfb_keymap = GDK_DIRECTFB_KEYMAP (keymap);

  if (G_UNLIKELY (! directfb_keymap->keymap))
    goto exit;

  if ((hardware_keycode >= directfb_keymap->min_keycode &&
       hardware_keycode <= directfb_keymap->max_keycode) &&
      (group == 0 || group == 1))
    {
      gsize i, index;

      index = (hardware_keycode - directfb_keymap->min_keycode) * 4;
      i = (state & GDK_SHIFT_MASK) ? 1 : 0;

      if (GDK_KEY_VoidSymbol != directfb_keymap->keymap[index + i + 2 * group])
	{
	  if (keyval)
	    *keyval = directfb_keymap->keymap[index + i + 2 * group];

	  if (group && directfb_keymap->keymap[index + i] == *keyval)
	    {
	      if (effective_group)
		*effective_group = 0;

	      if(consumed_modifiers)
		*consumed_modifiers = 0;
	    }
	  else
	    {
	      if (effective_group)
		*effective_group = group;

	      if(consumed_modifiers)
		*consumed_modifiers = GDK_MOD2_MASK;
	    }

	  if (level)
	    *level = i;

	  if (i && directfb_keymap->keymap[index + 2 * *effective_group] != *keyval)
	    if(consumed_modifiers)
	      *consumed_modifiers |= GDK_SHIFT_MASK;

	  return TRUE;
	}
    }

exit:

  if (keyval)
    *keyval = 0;

  if (effective_group)
    *effective_group = 0;

  if (level)
    *level = 0;

  if(consumed_modifiers)
    *consumed_modifiers = 0;

  return FALSE;
}

static void
gdk_directfb_keymap_add_virtual_modifiers (GdkKeymap       *keymap,
					   GdkModifierType *state)
{
}

static gboolean
gdk_directfb_keymap_map_virtual_modifiers (GdkKeymap       *keymap,
					   GdkModifierType *state)
{
  return TRUE;
}

static void
gdk_directfb_keymap_class_init (GdkDirectfbKeymapClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);
  GdkKeymapClass *keymap_class = GDK_KEYMAP_CLASS (klass);

  object_class->dispose = gdk_directfb_keymap_dispose;
  object_class->finalize = gdk_directfb_keymap_finalize;

  keymap_class->get_direction = gdk_directfb_keymap_get_direction;
  keymap_class->have_bidi_layouts = gdk_directfb_keymap_have_bidi_layouts;
  keymap_class->get_caps_lock_state = gdk_directfb_keymap_get_caps_lock_state;
  keymap_class->get_num_lock_state = gdk_directfb_keymap_get_num_lock_state;
  keymap_class->get_entries_for_keyval = gdk_directfb_keymap_get_entries_for_keyval;
  keymap_class->get_entries_for_keycode = gdk_directfb_keymap_get_entries_for_keycode;
  keymap_class->lookup_key = gdk_directfb_keymap_lookup_key;
  keymap_class->translate_keyboard_state = gdk_directfb_keymap_translate_keyboard_state;
  keymap_class->add_virtual_modifiers = gdk_directfb_keymap_add_virtual_modifiers;
  keymap_class->map_virtual_modifiers = gdk_directfb_keymap_map_virtual_modifiers;
}

/**
 * gdk_directfb_keymap_new:
 * @display: The #GdkDisplay for which this keymap is being created
 *
 * Creates a new keymap object.
 *
 * Returns: (transfer full): A new #GdkKeymap
 *
 * Since: 3.10
 */
GdkKeymap *
gdk_directfb_keymap_new (GdkDisplay *display)
{
  gsize i, length;
  DFBResult result;
  GdkKeymap *keymap;
  GdkDevice *keyboard;
  GdkDeviceManager *manager;
  DFBInputDeviceDescription desc;
  IDirectFBInputDevice *input_device;
  GdkDirectfbKeymap *directfb_keymap;

  keymap = g_object_new (GDK_TYPE_DIRECTFB_KEYMAP, NULL);
  keymap->display = display;

  manager = gdk_display_get_device_manager (keymap->display);
  keyboard = gdk_directfb_device_manager_get_core_keyboard (manager);
  if (G_UNLIKELY (! keyboard))
    {
      g_warning ("core keyboard not present");
      goto exit;
    }

  directfb_keymap = GDK_DIRECTFB_KEYMAP (keymap);
  directfb_keymap->device = g_object_ref (keyboard);

  input_device = gdk_directfb_device_get_input_device (keyboard);
  if (G_UNLIKELY (! input_device))
    goto exit;

  result = input_device->GetDescription (input_device, &desc);
  if (G_UNLIKELY (DFB_OK != result))
    {
      g_warning ("%s", DirectFBErrorString (result));
      goto exit;
    }

  if (desc.min_keycode < 0 ||
      desc.max_keycode < desc.min_keycode)
    {
      directfb_keymap->min_keycode = 0;
      directfb_keymap->max_keycode = 0;
      length = 1;
    }
  else
    {
      directfb_keymap->min_keycode = desc.min_keycode;
      directfb_keymap->max_keycode = desc.max_keycode;
      length = desc.max_keycode - desc.min_keycode + 1;
    }

  directfb_keymap->keymap = g_new0 (guint, 4 * length);

  for (i = 0; i < length; ++i)
    {
      gsize j;
      DFBInputDeviceKeymapEntry  entry;

      result = input_device->GetKeymapEntry (input_device,
					     i + desc.min_keycode,
					     &entry);
      if (G_UNLIKELY (DFB_OK != result))
	{
	  g_warning ("%s", DirectFBErrorString (result));
	  continue;
	}

      for (j = 0; j < 4; ++j)
	{
	  directfb_keymap->keymap[i * 4 + j] =
	    gdk_directfb_translate_key (entry.identifier, entry.symbols[j]);
	}
    }

exit:

  return keymap;
}

/**
 * gdk_directfb_keymap_lookup_hardware_keycode:
 * @keymap: A #GdkKeymap object
 * @keyval: A GDK key code
 *
 * Get the hardware key code for the specified GDK key code.
 *
 * Returns: The hardware key code
 *
 * Since: 3.10
 */
guint16
gdk_directfb_keymap_lookup_hardware_keycode (GdkKeymap *keymap,
					     guint      keyval)
{
  GdkDirectfbKeymap *directfb_keymap;
  guint16 i;

  g_return_val_if_fail (GDK_IS_DIRECTFB_KEYMAP (keymap), 0);

  directfb_keymap = GDK_DIRECTFB_KEYMAP (keymap);

  if (G_UNLIKELY (! directfb_keymap->keymap))
    return -1;

  for (i = directfb_keymap->min_keycode; i <= directfb_keymap->max_keycode; ++i)
    {
      gsize index = (i - directfb_keymap->min_keycode) * 4;

      if (directfb_keymap->keymap[index] == keyval)
	return i;
    }

  return -1;
}
