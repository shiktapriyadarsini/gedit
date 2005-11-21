/*
 * gedit-session.c - Basic session management for gedit
 * This file is part of gedit
 *
 * Copyright (C) 2002 Ximian, Inc.
 * Copyright (C) 2005 - Paolo Maggi 
 *
 * Author: Federico Mena-Quintero <federico@ximian.com>
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
 * Modified by the gedit Team, 2002-2005. See the AUTHORS file for a 
 * list of people on the gedit Team.  
 * See the ChangeLog files for a list of changes.
 *
 * $Id$
 */


#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <unistd.h>
#include <string.h>

#include <libgnome/gnome-util.h>
#include <libgnomeui/gnome-client.h>
#include <libxml/tree.h>
#include <libxml/xmlwriter.h>

#include "gedit-session.h"
#include "gedit-file.h"
#include "gedit-debug.h"
#include "gedit-plugins-engine.h"
#include "gedit-prefs-manager-app.h"
#include "gedit-metadata-manager.h"
#include "gedit-window.h"
#include "gedit-app.h"

/* The master client we use for SM */
static GnomeClient *master_client = NULL;

/* argv[0] from main(); used as the command to restart the program */
static const char *program_argv0 = NULL;

static void
ensure_session_directory (void)
{
	gchar *gedit_dir;
	gchar *dir;

	gedit_debug (DEBUG_SESSION);

	dir = gnome_util_home_file ("gedit");
	if (g_file_test (dir, G_FILE_TEST_EXISTS) == FALSE)
	{
		if (mkdir (dir, 488) != 0)
		{
			g_warning ("Unable to create directory '%s'\n", dir);
		}
	}

	gedit_dir = dir;

	dir = g_build_filename (gedit_dir, "sessions", NULL);	
	if (g_file_test (dir, G_FILE_TEST_EXISTS) == FALSE)
	{
		if (mkdir (dir, 488) != 0)
		{
			g_warning ("Unable to create directory '%s'\n", dir);
		}
	}

	g_free (dir);
	g_free (gedit_dir);
}

static gchar *
get_session_file_path (GnomeClient *client)
{
	const gchar *prefix;
	
	gchar *session_file;
	gchar *session_path;	
	gchar *gedit_dir;

	prefix = gnome_client_get_config_prefix (client);
	gedit_debug_message (DEBUG_SESSION, "Prefix: %s", prefix);
	
	session_file = g_strndup (prefix, strlen (prefix) - 1);
	gedit_debug_message (DEBUG_SESSION, "Session File: %s", session_file);
	
	gedit_dir = gnome_util_home_file ("gedit");
	session_path = g_build_filename (gedit_dir, "sessions", session_file, NULL);
	g_free (gedit_dir);
	
	g_free (session_file);

	gedit_debug_message (DEBUG_SESSION, "Session Path: %s", session_path);
	
	return session_path;
}

static int
save_window_session (xmlTextWriterPtr  writer,
		     GeditWindow      *window)
{
	const gchar *role;
	GeditPanel *panel;
	GList *docs, *l;
	int ret;
	GeditDocument *active_document;

	gedit_debug (DEBUG_SESSION);

	active_document = gedit_window_get_active_document (window);

	ret = xmlTextWriterStartElement (writer, (xmlChar *) "window");
	if (ret < 0)
		return ret;
	
	role = gtk_window_get_role (GTK_WINDOW (window));
	if (role != NULL)
	{
		ret = xmlTextWriterWriteAttribute (writer, "role", role);
		if (ret < 0)
			return ret;
	}

	ret = xmlTextWriterStartElement (writer, (xmlChar *) "side-pane");
	if (ret < 0)
		return ret;

	panel = gedit_window_get_side_panel (window);
	ret = xmlTextWriterWriteAttribute (writer,
					   "visible", 
					   GTK_WIDGET_VISIBLE (panel) ? "yes": "no");
	if (ret < 0)
		return ret;

	ret = xmlTextWriterEndElement (writer); /* side-pane */
	if (ret < 0)
		return ret;

	ret = xmlTextWriterStartElement (writer, (xmlChar *) "bottom-panel");
	if (ret < 0)
		return ret;

	panel = gedit_window_get_bottom_panel (window);
	ret = xmlTextWriterWriteAttribute (writer,
					   "visible", 
					   GTK_WIDGET_VISIBLE (panel) ? "yes" : "no");
	if (ret < 0)
		return ret;

	ret = xmlTextWriterEndElement (writer); /* bottom-panel */
	if (ret < 0)
		return ret;

	docs = gedit_window_get_documents (window);
	l = docs;
	while (l != NULL)
	{
		const gchar *uri;
		
		ret = xmlTextWriterStartElement (writer, (xmlChar *) "document");
		if (ret < 0)
			return ret;

		uri = gedit_document_get_uri (GEDIT_DOCUMENT (l->data));

		if (uri != NULL)
		{
			ret = xmlTextWriterWriteAttribute (writer,
							   "uri", 
							   uri);

			g_free (uri);

			if (ret < 0)
				return ret;
		}

		if (active_document == GEDIT_DOCUMENT (l->data))
		{
			ret = xmlTextWriterWriteAttribute (writer,
							   "active", 
							   "yes");
			if (ret < 0)
				return ret;
		}

		ret = xmlTextWriterEndElement (writer); /* document */
		if (ret < 0)
			return ret;

		l = g_list_next (l);
	}
	g_list_free (docs);	

	ret = xmlTextWriterEndElement (writer); /* window */

	return ret;
}

static void
save_session (const gchar *fname)
{
	int ret;
	xmlTextWriterPtr writer;
	const GSList *windows;
	
	gedit_debug_message (DEBUG_SESSION, "Session file: %s", fname);
	
	writer = xmlNewTextWriterFilename (fname, 0);
	if (writer == NULL)
	{
		g_warning ("Cannot write the session file '%s'", fname);
		return;

	}

	ret = xmlTextWriterSetIndent (writer, 1);
	if (ret < 0)
		goto out;

	ret = xmlTextWriterSetIndentString (writer, (const xmlChar *) " ");
	if (ret < 0)
		goto out;

	/* create and set the root node for the session */
	ret = xmlTextWriterStartElement (writer, (const xmlChar *) "session");
	if (ret < 0)
		goto out;

	windows = gedit_app_get_windows (gedit_app_get_default ());
	while (windows != NULL)
	{
		ret = save_window_session (writer, 
					   GEDIT_WINDOW (windows->data));
		if (ret < 0)
			goto out;

		windows = g_slist_next (windows);
	}

	ret = xmlTextWriterEndElement (writer); /* session */
	if (ret < 0)
		goto out;

	ret = xmlTextWriterEndDocument (writer);		

out:
	xmlFreeTextWriter (writer);

	if (ret < 0)
		unlink (fname);
}

static void 
interaction_function (GnomeClient     *client,
		      gint             key,
		      GnomeDialogType  dialog_type,
		      gpointer         shutdown)
{
	gchar *fname;

	gedit_debug (DEBUG_SESSION);
#if 0
	/* Save all unsaved files */
	if (GPOINTER_TO_INT (shutdown))		
		gedit_file_save_all ();
#endif

	/* Save session data */
	fname = get_session_file_path (client);

	save_session (fname);

	g_free (fname);

	gnome_interaction_key_return (key, FALSE);

	gedit_debug_message (DEBUG_SESSION, "END");
}

/* save_yourself handler for the master client */
static gboolean
client_save_yourself_cb (GnomeClient        *client,
			 gint                phase,
			 GnomeSaveStyle      save_style,
			 gboolean            shutdown,
			 GnomeInteractStyle  interact_style,
			 gboolean            fast,
			 gpointer            data)
{
	gchar *argv[] = { "rm", "-r", NULL };

	gedit_debug (DEBUG_SESSION);

	gnome_client_request_interaction (client, 
					  GNOME_DIALOG_NORMAL, 
					  interaction_function,
					  GINT_TO_POINTER (shutdown));
	
	/* Tell the session manager how to discard this save */
	argv[2] = get_session_file_path (client);
	gnome_client_set_discard_command (client, 3, argv);

	g_free (argv[2]);
	
	/* Tell the session manager how to clone or restart this instance */

	argv[0] = (char *) program_argv0;
	argv[1] = NULL; /* "--debug-session"; */

	gnome_client_set_clone_command (client, 1 /*2*/, argv);
	gnome_client_set_restart_command (client, 1 /*2*/, argv);

	gedit_debug_message (DEBUG_SESSION, "END");

	return TRUE;
}

/* die handler for the master client */
static void
client_die_cb (GnomeClient *client, gpointer data)
{
#if 0
	gedit_debug (DEBUG_SESSION);

	if (!client->save_yourself_emitted)
		gedit_file_close_all ();

	gedit_debug_message (DEBUG_FILE, "All files closed.");
	
	bonobo_mdi_destroy (BONOBO_MDI (gedit_mdi));
	
	gedit_debug_message (DEBUG_FILE, "Unref gedit_mdi.");

	g_object_unref (G_OBJECT (gedit_mdi));

	gedit_debug_message (DEBUG_FILE, "Unref gedit_mdi: DONE");

	gedit_debug_message (DEBUG_FILE, "Unref gedit_app_server.");

	bonobo_object_unref (gedit_app_server);

	gedit_debug_message (DEBUG_FILE, "Unref gedit_app_server: DONE");
#endif
	gedit_prefs_manager_app_shutdown ();
	gedit_metadata_manager_shutdown ();
	gedit_plugins_engine_shutdown ();

	gtk_main_quit ();
}

/**
 * gedit_session_init:
 * 
 * Initializes session management support.  This function should be called near
 * the beginning of the program.
 **/
void
gedit_session_init (const char *argv0)
{
	gedit_debug (DEBUG_SESSION);
	
	if (master_client)
		return;

	program_argv0 = argv0;

	ensure_session_directory ();
	
	master_client = gnome_master_client ();

	g_signal_connect (master_client,
			  "save_yourself",
			  G_CALLBACK (client_save_yourself_cb),
			  NULL);
	g_signal_connect (master_client,
			  "die",
			  G_CALLBACK (client_die_cb),
			  NULL);		  
}

/**
 * gedit_session_is_restored:
 * 
 * Returns whether this gedit is running from a restarted session.
 * 
 * Return value: TRUE if the session manager restarted us, FALSE otherwise.
 * This should be used to determine whether to pay attention to command line
 * arguments in case the session was not restored.
 **/
gboolean
gedit_session_is_restored (void)
{
	gboolean restored;

	gedit_debug (DEBUG_SESSION);

	if (!master_client)
		return FALSE;

	restored = (gnome_client_get_flags (master_client) & GNOME_CLIENT_RESTORED) != 0;

	gedit_debug_message (DEBUG_SESSION, restored ? "RESTORED" : "NOT RESTORED");

	return restored;
}

static void
parse_window (xmlNodePtr node)
{
	GeditWindow *window;
	xmlChar *role;
	xmlNodePtr child;

	role = xmlGetProp (node, (const xmlChar *) "role");
	gedit_debug_message (DEBUG_SESSION, "Window role: %s", role);

	window = _gedit_app_restore_window (gedit_app_get_default (), role);

	if (role != NULL)
		xmlFree (role);

	if (window == NULL)
	{
		g_warning ("Couldn't restore window");
		return;
	}

	child = node->children;

	while (child != NULL)
	{
		if (strcmp ((char *) child->name, "side-pane") == 0)
		{
			xmlChar *visible;

			visible = xmlGetProp (child, (const xmlChar *) "visible");

			if ((visible != NULL) &&
			    (strcmp ((char *) visible, "yes") == 0))
			{
				gedit_debug_message (DEBUG_SESSION, "Side panel visible");
				_gedit_window_set_side_panel_visible (window, 
								      TRUE);
			}
			else
			{
				gedit_debug_message (DEBUG_SESSION, "Side panel _NOT_ visible");
				_gedit_window_set_side_panel_visible (window, 
								      FALSE);

			}

			if (visible != NULL)
				xmlFree (visible);	
		}
		else if (strcmp ((char *) child->name, "bottom-panel") == 0)
		{
			xmlChar *visible;

			visible = xmlGetProp (child, (const xmlChar *) "visible");

			if ((visible != NULL) &&
			    (strcmp ((char *) visible, "yes") == 0))
			{
				gedit_debug_message (DEBUG_SESSION, "Bottom panel visible");
				_gedit_window_set_bottom_panel_visible (window, 
									TRUE);
			}
			else
			{
				gedit_debug_message (DEBUG_SESSION, "Bottom panel _NOT_ visible");
				_gedit_window_set_bottom_panel_visible (window, 
									FALSE);

			}

			if (visible != NULL)
				xmlFree (visible);
		}
		else if  (strcmp ((char *) child->name, "document") == 0)
		{
			xmlChar *uri;
			xmlChar *active;

			uri = xmlGetProp (child, (const xmlChar *) "uri");
			if (uri != NULL)
			{
				gboolean jump_to;

				active =  xmlGetProp (child, (const xmlChar *) "active");
				if (active != NULL)
				{
					jump_to = (strcmp ((char *) active, "yes") == 0);
					xmlFree (active);
				}
				else
				{
					jump_to = FALSE;
				}

				gedit_debug_message (DEBUG_SESSION,
						     "URI: %s (%s)",
						     (gchar *) uri,
						     jump_to ? "active" : "not active");

				gedit_window_create_tab_from_uri (window,
								  (const gchar *)uri,
								  NULL,
								  0,
								  FALSE,
								  jump_to);

				xmlFree (uri);
			}
		}
		
		child = child->next;
	}
	gtk_widget_show (GTK_WIDGET (window));
}

/**
 * gedit_session_load:
 * 
 * Loads the session by fetching the necessary information from the session
 * manager and opening files.
 * 
 * Return value: TRUE if the session was loaded successfully, FALSE otherwise.
 **/
gboolean
gedit_session_load (void)
{
	xmlDocPtr doc;
        xmlNodePtr child;
	gchar *fname;

	gedit_debug (DEBUG_SESSION);

	fname = get_session_file_path (master_client);
	gedit_debug_message (DEBUG_SESSION, "Session file: %s", fname);
	
	doc = xmlParseFile (fname);
	g_free (fname);

	if (doc == NULL)
		return FALSE;

	child = xmlDocGetRootElement (doc);

	/* skip the session node */
	child = child->children;

	while (child != NULL)
	{
		if (xmlStrEqual (child->name, (const xmlChar *) "window"))
		{
			gedit_debug_message (DEBUG_SESSION, "Restore window");

			parse_window (child);

			// ephy_gui_window_update_user_time (widget, user_time);

			//gtk_widget_show (widget);
		}

		child = child->next;
	}

	xmlFreeDoc (doc);

	return TRUE;
}
