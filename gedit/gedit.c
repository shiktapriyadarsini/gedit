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
 */

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <locale.h>
#include <string.h>

#include <glib/gi18n.h>
#include <libgnome/libgnome.h>
#include <libgnomeui/libgnomeui.h>
#include <libgnomevfs/gnome-vfs-utils.h>

#include "gedit2.h"
#include "gedit-mdi.h"
#include "gedit-prefs-manager-app.h"
#include "gedit-debug.h"
#include "gedit-encodings.h"
#include "gedit-file.h"
#include "gedit-utils.h"
#include "gedit-session.h"
#include "gedit-plugins-engine.h"
#include "gedit-convert.h"
#include "gedit-window.h"
#include "gedit-app.h"
#include "gedit-metadata-manager.h"

static struct poptOption options[] =
{
	{ NULL, 0, 0, NULL, 0, NULL, NULL }
};

int
main (int argc, char *argv[])
{
	GnomeProgram *program;
	GeditWindow *window;
	GeditApp *app;

	setlocale (LC_ALL, "");

	bindtextdomain (GETTEXT_PACKAGE, GEDIT_LOCALEDIR);
	textdomain (GETTEXT_PACKAGE);

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

	/* Initialize session management */
//	gedit_session_init (argv[0]);

#if 0
	if (gedit_session_is_restored ())
		restored = gedit_session_load ();

	if (!restored) 
	{
		CommandLineData *data;

		data = gedit_get_command_line_data (program);
		
		gtk_init_add ((GtkFunction)gedit_load_file_list, (gpointer)data);

		/* Open the first top level window */
		bonobo_mdi_open_toplevel (BONOBO_MDI (gedit_mdi), NULL);
	}

	gedit_app_server = gedit_application_server_new (gdk_screen_get_default ());
#endif
	app = gedit_app_get_default ();
	
	window = gedit_app_create_window (app);
	gedit_window_create_tab (window, TRUE);
	
	gtk_widget_show (GTK_WIDGET (window));
	
	gtk_main();
	
	gedit_prefs_manager_app_shutdown ();
	gedit_metadata_manager_shutdown ();
	// gedit_plugins_engine_shutdown ();
	
	g_object_unref (program);
	
	g_print ("Quit!\n");
	
	return 0;
}
