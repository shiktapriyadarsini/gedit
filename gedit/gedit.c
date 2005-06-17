/*
 * gedit.c
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

#include <locale.h>
#include <string.h>

#include <glib/gi18n.h>
#include <gdk/gdkx.h>
#include <libgnome/libgnome.h>
#include <libgnomeui/libgnomeui.h>
#include <libgnomevfs/gnome-vfs-utils.h>

#include "gedit2.h"
#include "gedit-mdi.h"
#include "gedit-prefs-manager-app.h"
#include "gedit-debug.h"
#include "gedit-commands.h"
#include "gedit-encodings.h"
#include "gedit-file.h"
#include "gedit-utils.h"
#include "gedit-session.h"
#include "gedit-plugins-engine.h"
#include "gedit-convert.h"
#include "gedit-window.h"
#include "gedit-app.h"
#include "gedit-metadata-manager.h"

#include "bacon-message-connection.h"

static guint32 startup_timestamp = 0;
static BaconMessageConnection *connection;

/* command line */
static gint line_position = 0;
static gchar *encoding_charset = NULL;
static const GeditEncoding *encoding;
static gboolean new_window_option = FALSE;
static gboolean new_document_option = FALSE;
static GSList *file_list = NULL;

static const struct poptOption options [] =
{
	{ "encoding", '\0', POPT_ARG_STRING, &encoding_charset,	0,
	  N_("Set the character encoding to be used to open the files listed on the command line"), NULL },

	{ "new-window", '\0', POPT_ARG_NONE, &new_window_option, 0,
	  N_("Create a new toplevel window in an existing instance of gedit"), NULL },

	{ "new-document", '\0', POPT_ARG_NONE, &new_document_option, 0,
	  N_("Create a new document in an existing instance of gedit"), NULL },

	{NULL, '\0', 0, NULL, 0}
};

static void
gedit_get_command_line_data (GnomeProgram *program)
{
	GValue value = { 0, };
	poptContext ctx;
	gchar **args;

	g_value_init (&value, G_TYPE_POINTER);
	g_object_get_property (G_OBJECT (program),
			       GNOME_PARAM_POPT_CONTEXT,
			       &value);
	ctx = g_value_get_pointer (&value);
	g_value_unset (&value);

	args = (gchar **) poptGetArgs(ctx);

	if (args)
	{
		gint i;

		for (i = 0; args[i]; i++) 
		{
			if (*args[i] == '+')
			{
				if (*(args[i] + 1) == '\0')
					/* goto the last line of the document */
					line_position = G_MAXINT;
				else
					line_position = atoi (args[i] + 1);
			}
			else
			{
				file_list = g_slist_prepend (file_list, 
					gnome_vfs_make_uri_from_shell_arg (args[i]));
			}
		}

		file_list = g_slist_reverse (file_list);
	}


	if (encoding_charset)
	{
		encoding = gedit_encoding_get_from_charset (encoding_charset);
		if (encoding == NULL)
			g_print (_("The specified encoding \"%s\" is not valid\n"),
				 encoding_charset);

		g_free (encoding_charset);
		encoding_charset = NULL;
	}

	poptFreeContext (ctx);
}

static guint32
get_startup_timestamp ()
{
	const gchar *startup_id_env;
	gchar *startup_id = NULL;
	gchar *time_str;
	gchar *end;
	gulong retval = 0;

	/* we don't unset the env, since startup-notification
	 * may still need it */
	startup_id_env = g_getenv ("DESKTOP_STARTUP_ID");
	if (startup_id_env == NULL)
		goto out;

	startup_id = g_strdup (startup_id_env);

	time_str = g_strrstr (startup_id, "_TIME");
	if (time_str == NULL)
		goto out;

	errno = 0;

	/* Skip past the "_TIME" part */
	time_str += 5;

	retval = strtoul (time_str, &end, 0);
	if (end == time_str || errno != 0)
		retval = 0;

 out:
	g_free (startup_id);

	return (retval > 0) ? retval : 0;
}

static void
on_message_received (const char *message,
		     gpointer    data)
{
	gchar **commands;
	gint workspace;
	GeditApp *app;
	GeditWindow *window;

	g_return_if_fail (message != NULL);

	g_print ("%s", message);

	commands = g_strsplit (message, ":", 6);

	startup_timestamp = atoi (commands[0]); //this sucks, right?
	workspace = atoi (commands[1]);
	new_window_option = atoi (commands[2]);
	new_document_option = atoi (commands[3]);
	line_position = atoi (commands[4]);
	encoding_charset = commands[5];

	app = gedit_app_get_default ();

	if (new_window_option)
	{
		window = gedit_app_create_window (app);
	}
	else
	{
		/* get a window in the current workspace (if exists) and raise it */
		window = gedit_app_get_window_in_workspace (app, workspace);

		/* fall back to roundtripping to the X server. lame. */
		if (startup_timestamp <= 0)
		{
			if (!GTK_WIDGET_REALIZED (window))
				gtk_widget_realize (GTK_WIDGET (window));

			startup_timestamp = gdk_x11_get_server_time (GTK_WIDGET (window)->window);
		}

		gdk_x11_window_set_user_time (GTK_WIDGET (window)->window,
					      startup_timestamp);
	}

// also handle --new-doc
//	if (file_list != NULL)
//		gedit_cmd_load_files_from_prompt (window, file_list, encoding, line_position);
//	else
		gedit_window_create_tab (window, TRUE);

	gtk_window_present (GTK_WINDOW (window));
}

int
main (int argc, char *argv[])
{
	GnomeProgram *program;
	GeditWindow *window;
	GeditApp *app;
	gboolean restored = FALSE;

	setlocale (LC_ALL, "");

	bindtextdomain (GETTEXT_PACKAGE, GEDIT_LOCALEDIR);
	textdomain (GETTEXT_PACKAGE);

	startup_timestamp = get_startup_timestamp();

	/* Initialize gnome program */
	program = gnome_program_init ("gedit", VERSION,
			    LIBGNOMEUI_MODULE, argc, argv,
			    GNOME_PARAM_POPT_TABLE, options,
			    GNOME_PARAM_HUMAN_READABLE_NAME,
		            _("Text Editor"),
			    GNOME_PARAM_APP_DATADIR, DATADIR,
			    NULL);

	/* Must be called after gnome_program_init to avoid problem with the
         * translation of --help messages */
	bind_textdomain_codeset (GETTEXT_PACKAGE, "UTF-8");

	connection = bacon_message_connection_new ("gedit");

	if (connection != NULL)
	{
		if (!bacon_message_connection_get_is_server (connection)) 
		{
			gint ws;
			GSList *l;
			GString *command;

			gedit_debug_message (DEBUG_APP, "I'm a client");

			ws = gedit_utils_get_current_workspace (gdk_screen_get_default ());
			gedit_get_command_line_data (program);

			command = g_string_new (NULL);

			/* send a command with the timestamp, the options, 
			 * and the file list using : as a separator.
			 */
			g_string_append_printf (command,
					"%" G_GUINT32_FORMAT ":%d:%d:%d:%d:%s",
					startup_timestamp,
					ws,
					new_window_option,
					new_document_option,
					line_position,
					encoding_charset ? encoding_charset : "");

			for (l = file_list; l; l = l->next)
			{
				command = g_string_append_c (command, ':');
				command = g_string_append (command, l->data);
			}

			bacon_message_connection_send (connection,
						       command->str);

			/* we never popup a window... tell startup-notification
			 * that we are done.
			 */
			gdk_notify_startup_complete ();

			g_string_free (command, TRUE);
			bacon_message_connection_free (connection);

			exit (0);
		}
		else 
		{
		  	gedit_debug_message (DEBUG_APP, "I'm a server");

			bacon_message_connection_set_callback (connection,
							       on_message_received,
							       NULL);
		}
	}
	else
		g_warning ("Cannot create the 'gedit' connection.");

	/* we don't need this anymore */
	g_free (encoding_charset);

	/* Setup debugging */
	gedit_debug_init ();

	/* Set default icon */
	gtk_window_set_default_icon_name ("text-editor");

	/* Load user preferences */
	gedit_prefs_manager_app_init ();

	gedit_recent_init ();

	/* Init plugins engine */
	gedit_plugins_engine_init ();

	gnome_authentication_manager_init ();
	gtk_about_dialog_set_url_hook (gedit_utils_activate_url, NULL, NULL);
	
	/* Initialize session management */
	gedit_session_init (argv[0]);

	if (gedit_session_is_restored ())
		restored = gedit_session_load ();

	if (!restored)
	{
		gedit_get_command_line_data (program);

		app = gedit_app_get_default ();

		window = gedit_app_create_window (app);
		if (file_list != NULL)
			gedit_cmd_load_files_from_prompt (window, file_list, encoding, line_position);
		else
			gedit_window_create_tab (window, TRUE);

		gtk_widget_show (GTK_WIDGET (window));
	}

	gtk_main();

	gedit_plugins_engine_shutdown ();
	gedit_prefs_manager_app_shutdown ();
	gedit_metadata_manager_shutdown ();

	g_object_unref (program);

	return 0;
}
