/*
 * gedit-app.c
 * This file is part of gedit
 *
 * Copyright (C) 2005 - Paolo Maggi 
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
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, 
 * Boston, MA 02111-1307, USA.
 */
 
/*
 * Modified by the gedit Team, 2005. See the AUTHORS file for a 
 * list of people on the gedit Team.  
 * See the ChangeLog files for a list of changes. 
 *
 * $Id$
 */

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <glib/gi18n.h>

#include "gedit-app.h"
#include "gedit-prefs-manager-app.h"
#include "gedit-commands.h"
#include "gedit-notebook.h"
#include "gedit-utils.h"

#define GEDIT_APP_GET_PRIVATE(object)(G_TYPE_INSTANCE_GET_PRIVATE ((object), GEDIT_TYPE_APP, GeditAppPrivate))

struct _GeditAppPrivate
{
	gboolean     is_restoring;
	
	GSList	    *windows;
	GeditWindow *active_window;
};

G_DEFINE_TYPE(GeditApp, gedit_app, G_TYPE_OBJECT)

static void
gedit_app_finalize (GObject *object)
{
	gboolean quit = FALSE;	
	GeditApp *app = GEDIT_APP (object); 
	
	quit = (app == gedit_app_get_default ());

	g_slist_free (app->priv->windows);

	G_OBJECT_CLASS (gedit_app_parent_class)->finalize (object);
	
	if (quit)
		gtk_main_quit ();
}

static void 
gedit_app_class_init (GeditAppClass *klass)
{
	GObjectClass *object_class = G_OBJECT_CLASS (klass);

	object_class->finalize = gedit_app_finalize;
							      
	g_type_class_add_private (object_class, sizeof(GeditAppPrivate));
}

static void
gedit_app_init (GeditApp *app)
{
	app->priv = GEDIT_APP_GET_PRIVATE (app);	
	
	app->priv->is_restoring = FALSE;
}

GeditApp *
gedit_app_get_default (void)
{
	static GeditApp *app = NULL;
	
	if (app != NULL)
		return app;
		
	app = GEDIT_APP (g_object_new (GEDIT_TYPE_APP, NULL));	

	// FIXME: add_weak_pointer 
	
	return app;
}

static gboolean
window_focus_in_event (GeditWindow   *window, 
		       GdkEventFocus *event, 
		       GeditApp      *app)
{
	/* updates active_view and active_child when a new toplevel receives focus */
	g_return_val_if_fail (GEDIT_IS_WINDOW (window), FALSE);
	
	app->priv->active_window = window;

	return FALSE;
}

static gboolean 
window_delete_event (GeditWindow *window,
                     GdkEvent    *event,
                     GeditApp    *app)
{
	GeditWindowState ws;
	
	ws = gedit_window_get_state (window);
	
	if (ws & GEDIT_WINDOW_STATE_SAVING)
		return TRUE; 
	
	gedit_cmd_file_close_all (NULL, window);
	
	/* Check if all the tabs have been closed */
	if (gedit_window_get_active_tab	(window) == NULL)
	{
		/* Return FALSE so GTK+ will emit the "destroy" signal */
		return FALSE;
	}
	
	/* Do not destroy the window */
	return TRUE;
}

static void 
window_destroy (GeditWindow *window, 
		GeditApp    *app)
{
	app->priv->windows = g_slist_remove (app->priv->windows,
					     window);

/* CHECK: I don't think we have to disconnect this function, since windows
   is being destroyed */
/*					     
	g_signal_handlers_disconnect_by_func (window, 
					      G_CALLBACK (window_focus_in_event),
					      app);
	g_signal_handlers_disconnect_by_func (window, 
					      G_CALLBACK (window_destroy),
					      app);
*/					      
	if (app->priv->windows == NULL)
	{
		g_object_unref (app);
	}
}

static gboolean 
notebook_tab_delete (GeditNotebook *notebook,
		     GeditTab      *tab,
		     GtkWindow     *window)
{
	return _gedit_cmd_file_can_close (tab, window);
}
	     
GeditWindow *
gedit_app_create_window	(GeditApp *app)
{
	GtkWindow *window;
	GtkWidget *notebook;
	
	window = GTK_WINDOW (g_object_new (GEDIT_TYPE_WINDOW, NULL));
	
	/* Set window state and size, but only if the session is not being restored */
	if (!app->priv->is_restoring)
	{
		GdkWindowState state;
		
		state = gedit_prefs_manager_get_window_state ();

		if ((state & GDK_WINDOW_STATE_MAXIMIZED) != 0)
		{
			gtk_window_set_default_size (window,
						     gedit_prefs_manager_get_default_window_width (),
						     gedit_prefs_manager_get_default_window_height ());

			gtk_window_maximize (window);
		}
		else
		{
			gtk_window_set_default_size (window, 
						     gedit_prefs_manager_get_window_width (),
						     gedit_prefs_manager_get_window_height ());

			gtk_window_unmaximize (window);
		}

		if ((state & GDK_WINDOW_STATE_STICKY ) != 0)
			gtk_window_stick (window);
		else
			gtk_window_unstick (window);
	}
	
	app->priv->windows = g_slist_prepend (app->priv->windows,
					      window);
	
	g_signal_connect (window, 
			  "focus_in_event",
			  G_CALLBACK (window_focus_in_event), 
			  app);
	g_signal_connect (window,
			  "delete_event",
			  G_CALLBACK (window_delete_event),
			  app);			  
	g_signal_connect (window, 
			  "destroy",
			  G_CALLBACK (window_destroy),
			  app);
			  
	notebook = _gedit_window_get_notebook (GEDIT_WINDOW (window));
	
	g_signal_connect (notebook,
			  "tab_delete",
			  G_CALLBACK (notebook_tab_delete),
			  window);
			  
	return GEDIT_WINDOW (window);
}

const GSList *
gedit_app_get_windows (GeditApp *app)
{
	g_return_val_if_fail (GEDIT_IS_APP (app), NULL);

	return app->priv->windows;
}

GeditWindow *
gedit_app_get_active_window (GeditApp *app)
{
	g_return_val_if_fail (GEDIT_IS_APP (app), NULL);

	return app->priv->active_window;
}

GeditWindow *
_gedit_app_get_window_in_workspace (GeditApp *app,
				    gint      workspace)
{
	GeditWindow *window;
	gint ws;

	g_return_val_if_fail (GEDIT_IS_APP (app), NULL);

	/* first try if the active window */
	window = app->priv->active_window;
	ws = gedit_utils_get_window_workspace (GTK_WINDOW (window));

	if (ws != workspace && ws != GEDIT_ALL_WORKSPACES)
	{
		GSList *l;

		/* try to see if there is a window on this workspace */
		l = app->priv->windows;
		while (l != NULL)
		{
			window = l->data;
			ws = gedit_utils_get_window_workspace (GTK_WINDOW (window));
			if (ws == workspace || ws == GEDIT_ALL_WORKSPACES)
				break;

			l = l->next;
		}

		/* no window on this workspace... create a new one */
		if (l == NULL)
			window = gedit_app_create_window (app);
	}

	return window;
}

/* Returns a newly allocated list with all the documents */
GList *
gedit_app_get_documents	(GeditApp *app)
{
	GList *res = NULL;
	GSList *windows;
	
	g_return_val_if_fail (GEDIT_IS_APP (app), NULL);
	
	windows = app->priv->windows;
	
	while (windows != NULL)
	{
		res = g_list_concat (res,
				     gedit_window_get_documents (GEDIT_WINDOW (windows->data)));
				     
		windows = g_slist_next (windows);
	}
	
	return res;
}

/* Returns a newly allocated list with all the views */
GList *
gedit_app_get_views (GeditApp *app)
{
	GList *res = NULL;
	GSList *windows;
	
	g_return_val_if_fail (GEDIT_IS_APP (app), NULL);
	
	windows = app->priv->windows;
	
	while (windows != NULL)
	{
		res = g_list_concat (res,
				     gedit_window_get_views (GEDIT_WINDOW (windows->data)));
				     
		windows = g_slist_next (windows);
	}
	
	return res;
}
