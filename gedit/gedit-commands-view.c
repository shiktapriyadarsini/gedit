/*
 * gedit-view-commands.c
 * This file is part of gedit
 *
 * Copyright (C) 1998, 1999 Alex Roberts, Evan Lawrence
 * Copyright (C) 2000, 2001 Chema Celorio, Paolo Maggi
 * Copyright (C) 2002-2005 Paolo Maggi
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, see <http://www.gnu.org/licenses/>.
 */

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <gtk/gtk.h>

#include "gedit-commands.h"
#include "gedit-debug.h"
#include "gedit-window.h"
#include "gedit-window-private.h"

void
_gedit_cmd_view_toggle_side_panel (GSimpleAction *action,
                                   GVariant      *state,
                                   gpointer       user_data)
{
	GeditWindow *window = GEDIT_WINDOW (user_data);
	GtkWidget *panel;
	gboolean visible;

	gedit_debug (DEBUG_COMMANDS);

	panel = gedit_window_get_side_panel (window);

	visible = g_variant_get_boolean (state);
	gtk_widget_set_visible (panel, visible);

	if (visible)
	{
		gtk_widget_grab_focus (panel);
	}

	g_simple_action_set_state (action, state);
}

void
_gedit_cmd_view_toggle_bottom_panel (GSimpleAction *action,
                                     GVariant      *state,
                                     gpointer       user_data)
{
	GeditWindow *window = GEDIT_WINDOW (user_data);
	GtkWidget *panel_box;
	gboolean visible;

	gedit_debug (DEBUG_COMMANDS);

	panel_box = window->priv->bottom_panel_box;

	visible = g_variant_get_boolean (state);
	gtk_widget_set_visible (panel_box, visible);

	if (visible)
	{
		gtk_widget_grab_focus (window->priv->bottom_panel);
	}

	g_simple_action_set_state (action, state);
}

void
_gedit_cmd_view_toggle_fullscreen_mode (GSimpleAction *action,
                                        GVariant      *state,
                                        gpointer       user_data)
{
	GeditWindow *window = GEDIT_WINDOW (user_data);

	gedit_debug (DEBUG_COMMANDS);

	if (g_variant_get_boolean (state))
	{
		_gedit_window_fullscreen (window);
	}
	else
	{
		_gedit_window_unfullscreen (window);
	}

	g_simple_action_set_state (action, state);
}

void
_gedit_cmd_view_leave_fullscreen_mode (GSimpleAction *action,
                                       GVariant      *parameter,
                                       gpointer       user_data)
{
	GeditWindow *window = GEDIT_WINDOW (user_data);
	GAction *fullscreen_action;

	_gedit_window_unfullscreen (window);

	fullscreen_action = g_action_map_lookup_action (G_ACTION_MAP (window),
	                                                "fullscreen");

	g_simple_action_set_state (G_SIMPLE_ACTION (fullscreen_action),
	                           g_variant_new_boolean (FALSE));
}

void
_gedit_cmd_view_highlight_mode (GSimpleAction *action,
                                GVariant      *parameter,
                                gpointer       user_data)
{
	GeditWindow *window = GEDIT_WINDOW (user_data);

	gtk_button_clicked (GTK_BUTTON (window->priv->language_button));
}

/* ex:set ts=8 noet: */
