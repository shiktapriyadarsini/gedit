/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */
/*
 * gedit-plugin-program-location-dialog.c
 * This file is part of gedit
 *
 * Copyright (C) 2002-2003 Paolo Maggi 
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
 * Modified by the gedit Team, 2002-2003. See the AUTHORS file for a 
 * list of people on the gedit Team.  
 * See the ChangeLog files for a list of changes. 
 */

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <glib/gi18n.h>
#include <libgnomeui/libgnomeui.h>
#include <gconf/gconf-client.h>

#include "gedit-dialogs.h"
#include "gedit-debug.h"
#include "gedit-utils.h"
#include "gedit-help.h"

#define PLUGIN_MANAGER_LOGO "/gedit-plugin-manager.png"


/* Return a newly allocated string containing the full location
 * of program_name
 */
gchar *
gedit_plugin_program_location_dialog (const gchar *program_name, const gchar *plugin_name,
		GtkWindow *parent, const gchar *gconf_key, const gchar *help_id)
{
	GtkWidget *dialog;	
	GtkWidget *content;
	GtkWidget *label;
	GtkWidget *logo;
	GtkWidget *program_location_entry;
	GConfClient *gconf_client;
	GtkWidget *error_widget;
	gboolean ret;

	gchar *str_label;
	gchar *program_location;

	gint ret;

	gedit_debug (DEBUG_PLUGINS);

	g_return_val_if_fail (program_name != NULL, NULL);
	g_return_val_if_fail (plugin_name != NULL, NULL);
	g_return_val_if_fail (gconf_key != NULL, NULL);

	gconf_client = gconf_client_get_default ();

	dialog = gtk_dialog_new_with_buttons (_("Set program location..."),
					      parent,
					      GTK_DIALOG_MODAL | GTK_DIALOG_DESTROY_WITH_PARENT,
					      GTK_STOCK_CANCEL,
					      GTK_RESPONSE_CANCEL,
					      GTK_STOCK_OK,
					      GTK_RESPONSE_OK,
					      GTK_STOCK_HELP,
					      GTK_RESPONSE_HELP,
					      NULL);

	g_return_val_if_fail (dialog != NULL, NULL);

	gui = gedit_utils_get_glade_widgets (GEDIT_GLADEDIR "program-location-dialog.glade2",
			     		     "dialog_content",
					     &error_widget,
					     "dialog_content", &content,
					     "program_location_file_entry", &program_location_entry,
					     "label", &label,
					     "logo", &logo,
					     NULL);

	if (!ret)
	{
		gedit_warning (parent,
			       gtk_label_get_label (GTK_LABEL (error_widget)));

		g_object_unref (gconf_client);
		gtk_widget_destroy (error_widget);
		g_free (dialog);

		return NULL;
	}

	gtk_label_set_use_markup (GTK_LABEL (label), TRUE);

	str_label = g_strdup_printf(_("The %s plugin uses an external program, "
				      "called <tt>%s</tt>, to perform its task.\n\n"
                                      "Please, specify the location of the <tt>%s</tt> program."),
				    plugin_name, program_name, program_name);

	gtk_label_set_markup (GTK_LABEL (label), str_label);
	g_free(str_label);

	/* stick the plugin manager logo in there */
	gtk_image_set_from_file (GTK_IMAGE (logo), GNOME_ICONDIR PLUGIN_MANAGER_LOGO);

	program_location = gconf_client_get_string (gconf_client,
						    gconf_key,
						    NULL);

	if (program_location == NULL)
		program_location = g_find_program_in_path (program_name);

	if (program_location != NULL)
	{
		gnome_file_entry_set_filename (GNOME_FILE_ENTRY (program_location_entry),
				program_location);

		g_free (program_location);
	}

	gtk_box_pack_start (GTK_BOX (GTK_DIALOG (dialog)->vbox),
			    content, FALSE, FALSE, 0);

	gtk_dialog_set_default_response (GTK_DIALOG (dialog),
					 GTK_RESPONSE_OK);

	gtk_window_set_resizable (GTK_WINDOW (dialog), FALSE);

	gtk_widget_grab_focus (gnome_file_entry_gtk_entry (
				GNOME_FILE_ENTRY (program_location_entry)));

	do
	{
		program_location = NULL;

		ret = gtk_dialog_run (GTK_DIALOG (dialog));	

		switch (ret) {
			case GTK_RESPONSE_OK:
				program_location = gnome_file_entry_get_full_path (
						GNOME_FILE_ENTRY (program_location_entry), FALSE);

				if (!g_file_test (program_location, G_FILE_TEST_IS_EXECUTABLE))
				{
					gedit_warning (GTK_WINDOW (dialog),
						       _("The selected file is not executable."));
				}
				else
				{
					gtk_widget_hide (dialog);
				}
				
				break;
				
			case GTK_RESPONSE_HELP:
				gedit_help_display (GTK_WINDOW (dialog),
						    "gedit.xml",
						    help_id != NULL ? help_id : "gedit-plugins");
				break;

			default:
				gtk_widget_hide (dialog);

		}

	} while (GTK_WIDGET_VISIBLE (dialog));

	gtk_widget_destroy (dialog);

	if (program_location != NULL)
	{
		gconf_client_set_string (gconf_client,
					 gconf_key,
					 program_location,
					 NULL);
	}

	g_object_unref (gconf_client);

	return program_location;
}

