/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */
/*
 * gedit-dialog-uri.c
 * This file is part of gedit
 *
 * Copyright (C) 2001 Paolo Maggi 
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
 * Modified by the gedit Team, 1998-2001. See the AUTHORS file for a 
 * list of people on the gedit Team.  
 * See the ChangeLog files for a list of changes. 
 */

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <glib/gi18n.h>
#include <glade/glade-xml.h>
#include <libgnome/gnome-help.h>
#include <libgnomeui/gnome-entry.h>

#include "gedit-utils.h"
#include "gedit-encodings-option-menu.h"
#include "gedit-dialogs.h"

#define GEDIT_OPEN_URI_DIALOG_KEY "gedit-open-uri-dialog-key"

typedef struct _GeditDialogOpenUri GeditDialogOpenUri;

struct _GeditDialogOpenUri {
	GtkWidget *dialog;

	GtkWidget *uri;
	GtkWidget *uri_list;
	GtkWidget *encoding_menu;
};

static void
open_button_pressed (GeditDialogOpenUri * dialog)
{
	gchar *file_name = NULL;
	const GeditEncoding *encoding;

	g_return_if_fail (dialog != NULL);

	file_name = gtk_editable_get_chars (GTK_EDITABLE (dialog->uri),
					    0, -1);

	gnome_entry_prepend_history (GNOME_ENTRY (dialog->uri_list), 
				     TRUE,
				     file_name);

	encoding = gedit_encodings_option_menu_get_selected_encoding (
			GEDIT_ENCODINGS_OPTION_MENU (dialog->encoding_menu));

	gedit_file_open_single_uri (file_name, encoding);

	g_free (file_name);
}

static void
help_button_pressed (GeditDialogOpenUri * dialog)
{
	GError *error = NULL;

	gnome_help_display ("gedit.xml", "gedit-open-from-uri", &error);

	if (error != NULL)
	{
		g_warning (error->message);

		g_error_free (error);
	}
}

static GeditDialogOpenUri *
dialog_open_uri_get_dialog (void)
{
	static GeditDialogOpenUri *dialog = NULL;
	GladeXML *gui;
	GtkWindow *window;
	GtkWidget *content;
	GtkWidget *encoding_label;
	GtkWidget *encoding_hbox;
	
	window = GTK_WINDOW (gedit_get_active_window ());

	if (dialog != NULL)
	{
		gtk_window_set_transient_for (GTK_WINDOW (dialog->dialog),
					      GTK_WINDOW (window));
		return dialog;
	}

	gui = glade_xml_new (GEDIT_GLADEDIR "uri.glade2",
			     "open_uri_dialog_content", NULL);
	if (!gui)
	{
		gedit_warning (window,
			       MISSING_FILE,
		    	       GEDIT_GLADEDIR "uri.glade2");
		return NULL;
	}

	dialog = g_new0 (GeditDialogOpenUri, 1);

	dialog->dialog = gtk_dialog_new_with_buttons (_("Open Location"),
						      window,
						      GTK_DIALOG_MODAL,
						      GTK_STOCK_CANCEL,
						      GTK_RESPONSE_CANCEL,
						      GTK_STOCK_OPEN,
						      GTK_RESPONSE_OK,
						      GTK_STOCK_HELP,
						      GTK_RESPONSE_HELP,
						      NULL);

	g_return_val_if_fail (dialog->dialog != NULL, NULL);

	gtk_window_set_resizable (GTK_WINDOW (dialog->dialog), FALSE);
	gtk_dialog_set_has_separator (GTK_DIALOG (dialog->dialog), FALSE);

	content = glade_xml_get_widget (gui, "open_uri_dialog_content");

	dialog->uri = glade_xml_get_widget (gui, "uri");
	dialog->uri_list = glade_xml_get_widget (gui, "uri_list");
	encoding_label = glade_xml_get_widget (gui, "encoding_label");
	encoding_hbox = glade_xml_get_widget (gui, "encoding_hbox");
	
	if (!dialog->uri || !dialog->uri_list || !encoding_label || !encoding_hbox) 
	{
		gedit_warning (window,
			       MISSING_WIDGETS,
			       GEDIT_GLADEDIR "uri.glade2");
		return NULL;
	}

	dialog->encoding_menu = gedit_encodings_option_menu_new (FALSE);

	gtk_label_set_mnemonic_widget (GTK_LABEL (encoding_label),
				       dialog->encoding_menu);

	gtk_box_pack_end (GTK_BOX (encoding_hbox), 
			  dialog->encoding_menu,
			  TRUE,
			  TRUE,
			  0);

	gtk_widget_show (dialog->encoding_menu);

	gtk_box_pack_start (GTK_BOX (GTK_DIALOG (dialog->dialog)->vbox),
			    content, FALSE, FALSE, 0);

	gtk_dialog_set_default_response (GTK_DIALOG (dialog->dialog),
					 GTK_RESPONSE_OK);

	g_object_unref (gui);

	return dialog;
}

static void
dialog_destroyed (GeditWindow *window, GeditFileChooserDialog *dialog)
{
	g_object_set_data (G_OBJECT (window),
			   GEDIT_OPEN_URI_DIALOG_KEY,
			   NULL);
}

void
gedit_dialog_open_uri (GtkWindow *parent)
{
	GeditDialogOpenUri *dialog;
	gpointer data;
	gint response;

	data = g_object_get_data (G_OBJECT (window), GEDIT_OPEN_URI_DIALOG_KEY);

	if ((data != NULL) && (GTK_IS_DIALOG (data)))
	{
		gtk_window_present (GTK_WINDOW (data));

		return;
	}

	dialog = dialog_open_uri_get_dialog ();
	if (!dialog)
		return;

	g_object_set_data (G_OBJECT (parent),
			   GEDIT_OPEN_DIALOG_KEY,
			   dialog);

	g_object_weak_ref (G_OBJECT (dialog),
			   (GWeakNotify) dialog_destroyed,
			   parent);

	if (parent != NULL)
	{
		GtkWindowGroup *wg;

		gtk_window_set_transient_for (GTK_WINDOW (dialog),
					      parent);
  	 
  	                         wg = GTK_WINDOW (toplevel)->group;
  	                         if (wg == NULL)
  	                         {
  	                                 wg = gtk_window_group_new ();
  	                                 gtk_window_group_add_window (wg,
  	                                                              GTK_WINDOW (toplevel));
  	                         }
  	 
  	                         gtk_window_group_add_window (wg,
  	                                                      GTK_WINDOW (dialog));
  	                 }




	gedit_encodings_option_menu_set_selected_encoding (
		GEDIT_ENCODINGS_OPTION_MENU (dialog->encoding_menu),
		NULL);

	gtk_widget_grab_focus (dialog->uri);

	gtk_entry_set_text (GTK_ENTRY (dialog->uri), "");

	do {
		response = gtk_dialog_run (GTK_DIALOG (dialog->dialog));

		switch (response) {
		case GTK_RESPONSE_OK:
			open_button_pressed (dialog);
			break;

		case GTK_RESPONSE_HELP:
			help_button_pressed (dialog);
			break;

		default:
			gtk_widget_hide (dialog->dialog);
		}

	} while (response == GTK_RESPONSE_HELP);
}

