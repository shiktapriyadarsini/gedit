/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */
/*
 * gedit-dialog-uri.c
 * This file is part of gedit
 *
 * Copyright (C) 2001-2005 Paolo Maggi 
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
 *
 * $Id$
 */

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <glib/gi18n.h>
#include <glade/glade-xml.h>
#include <libgnome/gnome-help.h>
#include <libgnomeui/gnome-entry.h>

#include "gedit-dialogs.h"
#include "gedit-encodings-option-menu.h"
#include "gedit-window.h"
#include "gedit-utils.h"

#define GEDIT_OPEN_URI_DIALOG_KEY "gedit-open-uri-dialog-key"

typedef struct _GeditDialogOpenUri GeditDialogOpenUri;

struct _GeditDialogOpenUri {
	GtkWidget *dialog;

	GtkWidget *uri;
	GtkWidget *uri_list;
	GtkWidget *encoding_menu;

	GeditWindow *gedit_window;
};

static void
dialog_response_cb (GtkDialog          *dialog,
                    gint                response_id,
                    GeditDialogOpenUri *dlg)
{
	GError *error = NULL;
	gchar *uri = NULL;
	GSList *uris = NULL;
	const GeditEncoding *encoding;
	gint n;

	switch (response_id)
	{
	case GTK_RESPONSE_HELP:
		gnome_help_display ("gedit.xml", "gedit-open-from-uri", &error);

		if (error != NULL)
		{
			g_warning (error->message);

			g_error_free (error);
		}

		break;

	case GTK_RESPONSE_OK:
		uri = gtk_editable_get_chars (GTK_EDITABLE (dlg->uri),
					      0,
					      -1);

		uris = g_slist_prepend (uris, uri);

		gnome_entry_prepend_history (GNOME_ENTRY (dlg->uri_list), 
					     TRUE,
					     uri);

		encoding = gedit_encodings_option_menu_get_selected_encoding (
				GEDIT_ENCODINGS_OPTION_MENU (dlg->encoding_menu));

		n = gedit_window_load_files (dlg->gedit_window,
			 		     uris,
					     encoding,
					     FALSE);
		g_free (uri);
		g_slist_free (uris);

		/* fall through */
	default:
		gtk_widget_destroy (GTK_WIDGET (dialog));
	}
}

static GeditDialogOpenUri *
dialog_open_uri_get_dialog (GtkWindow *parent)
{
	GeditDialogOpenUri *dialog;
	GladeXML *gui;
	GtkWidget *content;
	GtkWidget *encoding_label;
	GtkWidget *encoding_hbox;

	gui = glade_xml_new (GEDIT_GLADEDIR "uri.glade2",
			     "open_uri_dialog_content", NULL);
	if (!gui)
	{
		gedit_warning (parent,
			       MISSING_FILE,
		    	       GEDIT_GLADEDIR "uri.glade2");

		return NULL;
	}

	dialog = g_new0 (GeditDialogOpenUri, 1);

	dialog->dialog = gtk_dialog_new_with_buttons (_("Open Location"),
						      parent,
						      GTK_DIALOG_DESTROY_WITH_PARENT,
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

	if (!content          ||
	    !dialog->uri      ||
	    !dialog->uri_list ||
	    !encoding_label   ||
	    !encoding_hbox) 
	{
		gedit_warning (parent,
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
dialog_destroyed (GtkObject          *obj,
                  GeditDialogOpenUri *dialog)
{
	g_object_set_data (G_OBJECT (dialog->gedit_window),
			   GEDIT_OPEN_URI_DIALOG_KEY,
			   NULL);

	g_free (dialog);
}

void
gedit_dialog_open_uri (GeditWindow *parent)
{
	GeditDialogOpenUri *dialog;

	dialog = (GeditDialogOpenUri *) g_object_get_data (G_OBJECT (parent),
							   GEDIT_OPEN_URI_DIALOG_KEY);
	if (dialog != NULL)
	{
		g_return_if_fail (GTK_IS_DIALOG (dialog->dialog));

		gtk_window_present (GTK_WINDOW (dialog->dialog));

		return;
	}

	dialog = dialog_open_uri_get_dialog (GTK_WINDOW (parent));
	if (!dialog)
		return;

	dialog->gedit_window = parent;

	g_object_set_data (G_OBJECT (parent),
			   GEDIT_OPEN_URI_DIALOG_KEY,
			   dialog);

	gedit_encodings_option_menu_set_selected_encoding (
		GEDIT_ENCODINGS_OPTION_MENU (dialog->encoding_menu),
		NULL);

	gtk_widget_grab_focus (dialog->uri);

	gtk_entry_set_text (GTK_ENTRY (dialog->uri), "");

	g_signal_connect (dialog->dialog,
			  "response",
			  G_CALLBACK (dialog_response_cb),
			  dialog);
	g_signal_connect (dialog->dialog,
			  "destroy",
			  G_CALLBACK (dialog_destroyed),
			  dialog);

	gtk_widget_show (dialog->dialog);
}
