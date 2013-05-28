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

#include "gdkkeys.h"
#include "gdkprivate-directfb.h"
#include <cairo-directfb.h>

/**
 * gdk_directfb_translate_key:
 * @key_id: A #DFBInputDeviceKeyIdentifier
 * @key_symbol: A #DFBInputDeviceKeySymbol
 *
 * Translate a DirectFB key ID/symbol into a GDK key code.
 *
 * Returns: A GDK key code
 *
 * Since: 3.10
 */
guint
gdk_directfb_translate_key (DFBInputDeviceKeyIdentifier key_id,
			    DFBInputDeviceKeySymbol     key_symbol)
{
  if (key_id >= DIKI_KP_DIV && key_id <= DIKI_KP_9)
    {
      switch (key_symbol)
	{
	case DIKS_SLASH: return GDK_KEY_KP_Divide;
	case DIKS_ASTERISK: return GDK_KEY_KP_Multiply;
	case DIKS_PLUS_SIGN: return GDK_KEY_KP_Add;
	case DIKS_MINUS_SIGN: return GDK_KEY_KP_Subtract;
	case DIKS_ENTER: return GDK_KEY_KP_Enter;
	case DIKS_SPACE: return GDK_KEY_KP_Space;
	case DIKS_TAB: return GDK_KEY_KP_Tab;
	case DIKS_EQUALS_SIGN: return GDK_KEY_KP_Equal;
	case DIKS_COMMA: return GDK_KEY_KP_Decimal;
	case DIKS_PERIOD: return GDK_KEY_KP_Decimal;
	case DIKS_HOME: return GDK_KEY_KP_Home;
	case DIKS_END: return GDK_KEY_KP_End;
	case DIKS_PAGE_UP: return GDK_KEY_KP_Page_Up;
	case DIKS_PAGE_DOWN: return GDK_KEY_KP_Page_Down;
	case DIKS_CURSOR_LEFT: return GDK_KEY_KP_Left;
	case DIKS_CURSOR_RIGHT: return GDK_KEY_KP_Right;
	case DIKS_CURSOR_UP: return GDK_KEY_KP_Up;
	case DIKS_CURSOR_DOWN: return GDK_KEY_KP_Down;
	case DIKS_BEGIN: return GDK_KEY_KP_Begin;
	case DIKS_0: return GDK_KEY_KP_0;
	case DIKS_1: return GDK_KEY_KP_1;
	case DIKS_2: return GDK_KEY_KP_2;
	case DIKS_3: return GDK_KEY_KP_3;
	case DIKS_4: return GDK_KEY_KP_4;
	case DIKS_5: return GDK_KEY_KP_5;
	case DIKS_6: return GDK_KEY_KP_6;
	case DIKS_7: return GDK_KEY_KP_7;
	case DIKS_8: return GDK_KEY_KP_8;
	case DIKS_9: return GDK_KEY_KP_9;
	case DIKS_F1: return GDK_KEY_KP_F1;
	case DIKS_F2: return GDK_KEY_KP_F2;
	case DIKS_F3: return GDK_KEY_KP_F3;
	case DIKS_F4: return GDK_KEY_KP_F4;
	default: return GDK_KEY_VoidSymbol;
	}
    }
  else
    {
      switch (DFB_KEY_TYPE (key_symbol))
	{
	case DIKT_UNICODE:
	  switch (key_symbol)
	    {
	    case DIKS_NULL: return GDK_KEY_VoidSymbol;
	    case DIKS_BACKSPACE: return GDK_KEY_BackSpace;
	    case DIKS_TAB: return GDK_KEY_Tab;
	    case DIKS_RETURN: return GDK_KEY_Return;
	    case DIKS_CANCEL: return GDK_KEY_Cancel;
	    case DIKS_ESCAPE: return GDK_KEY_Escape;
	    case DIKS_SPACE: return GDK_KEY_space;
	    case DIKS_DELETE: return GDK_KEY_Delete;
	    default: return gdk_unicode_to_keyval (key_symbol);
	    }
	  break;

	case DIKT_SPECIAL:
	  switch (key_symbol)
	    {
	    case DIKS_CURSOR_LEFT: return GDK_KEY_Left;
	    case DIKS_CURSOR_RIGHT: return GDK_KEY_Right;
	    case DIKS_CURSOR_UP: return GDK_KEY_Up;
	    case DIKS_CURSOR_DOWN: return GDK_KEY_Down;
	    case DIKS_INSERT: return GDK_KEY_Insert;
	    case DIKS_HOME: return GDK_KEY_Home;
	    case DIKS_END: return GDK_KEY_End;
	    case DIKS_PAGE_UP: return GDK_KEY_Page_Up;
	    case DIKS_PAGE_DOWN: return GDK_KEY_Page_Down;
	    case DIKS_PRINT: return GDK_KEY_Print;
	    case DIKS_PAUSE: return GDK_KEY_Pause;
	    case DIKS_SELECT: return GDK_KEY_Select;
	    case DIKS_CLEAR: return GDK_KEY_Clear;
	    case DIKS_MENU: return GDK_KEY_Menu;
	    case DIKS_HELP: return GDK_KEY_Help;
	    case DIKS_NEXT: return GDK_KEY_Next;
	    case DIKS_BEGIN: return GDK_KEY_Begin;
	    case DIKS_BREAK: return GDK_KEY_Break;
	    default: return GDK_KEY_VoidSymbol;
	    }
	  break;

	case DIKT_FUNCTION:
	  switch (key_symbol)
	    {
	    case DIKS_F1: return GDK_KEY_F1;
	    case DIKS_F2: return GDK_KEY_F2;
	    case DIKS_F3: return GDK_KEY_F3;
	    case DIKS_F4: return GDK_KEY_F4;
	    case DIKS_F5: return GDK_KEY_F5;
	    case DIKS_F6: return GDK_KEY_F6;
	    case DIKS_F7: return GDK_KEY_F7;
	    case DIKS_F8: return GDK_KEY_F8;
	    case DIKS_F9: return GDK_KEY_F9;
	    case DIKS_F10: return GDK_KEY_F10;
	    case DIKS_F11: return GDK_KEY_F11;
	    case DIKS_F12: return GDK_KEY_F12;
	    case DFB_FUNCTION_KEY (13): return GDK_KEY_F13;
	    case DFB_FUNCTION_KEY (14): return GDK_KEY_F14;
	    case DFB_FUNCTION_KEY (15): return GDK_KEY_F15;
	    case DFB_FUNCTION_KEY (16): return GDK_KEY_F16;
	    case DFB_FUNCTION_KEY (17): return GDK_KEY_F17;
	    case DFB_FUNCTION_KEY (18): return GDK_KEY_F18;
	    case DFB_FUNCTION_KEY (19): return GDK_KEY_F19;
	    case DFB_FUNCTION_KEY (20): return GDK_KEY_F20;
	    case DFB_FUNCTION_KEY (21): return GDK_KEY_F21;
	    case DFB_FUNCTION_KEY (22): return GDK_KEY_F22;
	    case DFB_FUNCTION_KEY (23): return GDK_KEY_F23;
	    case DFB_FUNCTION_KEY (24): return GDK_KEY_F24;
	    case DFB_FUNCTION_KEY (25): return GDK_KEY_F25;
	    case DFB_FUNCTION_KEY (26): return GDK_KEY_F26;
	    case DFB_FUNCTION_KEY (27): return GDK_KEY_F27;
	    case DFB_FUNCTION_KEY (28): return GDK_KEY_F28;
	    case DFB_FUNCTION_KEY (29): return GDK_KEY_F29;
	    case DFB_FUNCTION_KEY (30): return GDK_KEY_F30;
	    case DFB_FUNCTION_KEY (31): return GDK_KEY_F31;
	    case DFB_FUNCTION_KEY (32): return GDK_KEY_F32;
	    case DFB_FUNCTION_KEY (33): return GDK_KEY_F33;
	    case DFB_FUNCTION_KEY (34): return GDK_KEY_F34;
	    case DFB_FUNCTION_KEY (35): return GDK_KEY_F35;
	    default: return GDK_KEY_VoidSymbol;
	    }
	  break;

	case DIKT_MODIFIER:
	  switch (key_id)
	    {
	    case DIKI_SHIFT_L: return GDK_KEY_Shift_L;
	    case DIKI_SHIFT_R: return GDK_KEY_Shift_R;
	    case DIKI_CONTROL_L: return GDK_KEY_Control_L;
	    case DIKI_CONTROL_R: return GDK_KEY_Control_R;
	    case DIKI_ALT_L: return GDK_KEY_Alt_L;
	    case DIKI_ALT_R: return GDK_KEY_Alt_R;
	    case DIKI_META_L: return GDK_KEY_Meta_L;
	    case DIKI_META_R: return GDK_KEY_Meta_R;
	    case DIKI_SUPER_L: return GDK_KEY_Super_L;
	    case DIKI_SUPER_R: return GDK_KEY_Super_R;
	    case DIKI_HYPER_L: return GDK_KEY_Hyper_L;
	    case DIKI_HYPER_R: return GDK_KEY_Hyper_R;
	    default: return GDK_KEY_VoidSymbol;
	    }
	  break;

	case DIKT_LOCK:
	  switch (key_id)
	    {
	    case DIKS_CAPS_LOCK: return GDK_KEY_Caps_Lock;
	    case DIKS_NUM_LOCK: return GDK_KEY_Num_Lock;
	    case DIKS_SCROLL_LOCK: return GDK_KEY_Scroll_Lock;
	    default: return GDK_KEY_VoidSymbol;
	    }
	  break;

	case DIKT_DEAD:
	  break;

	case DIKT_CUSTOM:
	  break;

	default:
	  return GDK_KEY_VoidSymbol;
	}
    }

  return GDK_KEY_VoidSymbol;
}

/**
 * gdk_directfb_modifier_mask:
 * @modifiers: A DirectFB modifier key mask
 * @locks: A DirectFB lock key mask
 * @buttons: A DirectFB mouse button mask
 *
 * Translates the DirectFB button/locks into a #GdkModifierType.
 *
 * Returns: A #GdkModifierType
 *
 * Since: 3.10
 */
GdkModifierType
gdk_directfb_modifier_mask (DFBInputDeviceModifierMask modifiers,
			    DFBInputDeviceLockState    locks,
			    DFBInputDeviceButtonMask   buttons)
{
  GdkModifierType state = 0;

  if (DIMM_SHIFT & modifiers)
    state |= GDK_SHIFT_MASK;

  if (DIMM_CONTROL & modifiers)
    state |= GDK_CONTROL_MASK;

  if (DIMM_ALT & modifiers)
    state |= GDK_MOD1_MASK;

  if (DIMM_ALTGR & modifiers)
    state |= GDK_MOD2_MASK;

  if (DIMM_SUPER & modifiers)
    state |= GDK_SUPER_MASK;

  if (DIMM_HYPER & modifiers)
    state |= GDK_HYPER_MASK;

  if (DIMM_META & modifiers)
    state |= GDK_META_MASK;

  if (DILS_CAPS & locks)
    state |= GDK_LOCK_MASK;

  if (DIBM_LEFT & buttons)
    state |= GDK_BUTTON1_MASK;

  if (DIBM_MIDDLE & buttons)
    state |= GDK_BUTTON2_MASK;

  if (DIBM_MIDDLE & buttons)
    state |= GDK_BUTTON3_MASK;

  return state;
}

/**
 * gdk_directfb_parse_modifier_mask:
 * @state: The requested modifier state
 * @modifiers: (out): A #DFBInputDeviceModifierMask
 * @locks: (out): A #DFBInputDeviceLockState
 * @buttons: (out): A #DFBInputDeviceButtonMask
 *
 * Parses the GDK modifier state into the appropriate DirectFB types.
 *
 * Since: 3.10
 */
void
gdk_directfb_parse_modifier_mask (GdkModifierType             state,
				  DFBInputDeviceModifierMask *modifiers,
				  DFBInputDeviceLockState    *locks,
				  DFBInputDeviceButtonMask   *buttons)
{
  *modifiers = *locks = *buttons = 0;

  if (state & GDK_SHIFT_MASK)
    *modifiers |= DIMM_SHIFT;

  if (state & GDK_CONTROL_MASK)
    *modifiers |= DIMM_CONTROL;

  if (state & GDK_MOD1_MASK)
    *modifiers |= DIMM_ALT;

  if (state & GDK_MOD2_MASK)
    *modifiers |= DIMM_ALTGR;

  if (state & GDK_SUPER_MASK)
    *modifiers |= DIMM_SUPER;

  if (state & GDK_HYPER_MASK)
    *modifiers |= DIMM_HYPER;

  if (state & GDK_META_MASK)
    *modifiers |= DIMM_META;

  if (state & GDK_LOCK_MASK)
    *locks |= DILS_CAPS;

  if (state & GDK_BUTTON1_MASK)
    *buttons |= DIBM_LEFT;

  if (state & GDK_BUTTON2_MASK)
    *buttons |= DIBM_MIDDLE;

  if (state & GDK_BUTTON3_MASK)
    *buttons |= DIBM_MIDDLE;
}

/**
 * gdk_directfb_premultiply:
 * @alpha: An alpha value
 * @color: A color value
 *
 * Premultiply a single color value by the given alpha value.
 *
 * Returns: The premultiplied color value.
 *
 * Since: 3.10
 */
guchar
gdk_directfb_premultiply (guchar alpha,
			  guchar color)
{
  return (alpha * color + 127) / 255;
}
