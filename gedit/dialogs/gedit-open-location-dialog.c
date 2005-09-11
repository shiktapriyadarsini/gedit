/*
 * gedit-open-location-dialog.c
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
 * Modified by the gedit Team, 2001-2005. See the AUTHORS file for a 
 * list of people on the gedit Team.  
 * See the ChangeLog files for a list of changes. 
 *
 * $Id$
 */

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include "gedit-open-location-dialog.h"

#include <glib/gi18n.h>
#include <glade/glade-xml.h>

#include <libgnomeui/gnome-entry.h>
#include <libgnomevfs/gnome-vfs-utils.h>

#include "gedit-encodings-option-menu.h"
#include "gedit-utils.h"

#define GEDIT_OPEN_LOCATION_DIALOG_GET_PRIVATE(object)(G_TYPE_INSTANCE_GET_PRIVATE ((object), GEDIT_TYPE_OPEN_LOCATION_DIALOG, GeditOpenLocationDialogPrivate))

struct _GeditOpenLocationDialogPrivate 
{
	GtkWidget *uri;
	GtkWidget *uri_list;
	GtkWidget *encoding_menu;
};

G_DEFINE_TYPE(GeditOpenLocationDialog, gedit_open_location_dialog, GTK_TYPE_DIALOG)

static void 
gedit_open_location_dialog_class_init (GeditOpenLocationDialogClass *klass)
{
	GObjectClass *object_class = G_OBJECT_CLASS (klass);
					      								      
	g_type_class_add_private (object_class, sizeof (GeditOpenLocationDialogPrivate));
}

static void
show_error_message (GeditOpenLocationDialog *dlg,
		    const gchar             *str)
{
	GtkWidget *label;

	label = gtk_label_new (str);

	gtk_label_set_line_wrap (GTK_LABEL (label), TRUE);
	gtk_label_set_use_markup (GTK_LABEL (label), TRUE);
	gtk_misc_set_padding (GTK_MISC (label), 6, 6);
	
	gtk_widget_show (label);
			
	gtk_box_pack_start (GTK_BOX (GTK_DIALOG (dlg)->vbox),
		    	    label, 
		    	    FALSE, 
		    	    FALSE,
		    	    0);
}

static void 
entry_changed (GtkEditable             *editable, 
	       GeditOpenLocationDialog *dlg)
{
	const gchar *str;
	
	str = gtk_entry_get_text (GTK_ENTRY (dlg->priv->uri));		
	g_return_if_fail (str != NULL);
	
	gtk_dialog_set_response_sensitive (GTK_DIALOG (dlg), 
					   GTK_RESPONSE_OK,
					   (str[0] != '\0'));
}

static void
response_handler (GeditOpenLocationDialog *dlg,
                  gint                     response_id,
                  gpointer                 data)
{
	gchar *uri;

	switch (response_id)
	{
		case GTK_RESPONSE_HELP:
			gedit_help_display (GTK_WINDOW (dlg),
					    NULL,
					    NULL);

			g_signal_stop_emission_by_name (dlg, "response");
			
			break;
			
		case GTK_RESPONSE_OK:
			uri = gedit_open_location_dialog_get_uri (dlg);
			if (uri != NULL)
			{	
				g_free (uri);
				uri = gtk_editable_get_chars (GTK_EDITABLE (dlg->priv->uri),
							      0,
							      -1);
				gnome_entry_prepend_history (GNOME_ENTRY (dlg->priv->uri_list), 
						     	     TRUE,
						     	     uri);
				g_free (uri);
			}
			
			break;					     	     
	}
}

static void
gedit_open_location_dialog_init (GeditOpenLocationDialog *dlg)
{
	GladeXML  *gui;
	
	GtkWidget *content;
	GtkWidget *encoding_label;
	GtkWidget *encoding_hbox;
	
	dlg->priv = GEDIT_OPEN_LOCATION_DIALOG_GET_PRIVATE (dlg);
	
	gtk_dialog_add_buttons (GTK_DIALOG (dlg),
				GTK_STOCK_CANCEL, 
				GTK_RESPONSE_CANCEL,
				GTK_STOCK_OPEN,
				GTK_RESPONSE_OK,
				GTK_STOCK_HELP,
				GTK_RESPONSE_HELP,
				NULL);

	gtk_window_set_title (GTK_WINDOW (dlg), _("Open Location"));
	gtk_window_set_resizable (GTK_WINDOW (dlg), FALSE);
	gtk_dialog_set_has_separator (GTK_DIALOG (dlg), FALSE);
	gtk_window_set_destroy_with_parent (GTK_WINDOW (dlg), TRUE);

	gtk_dialog_set_default_response (GTK_DIALOG (dlg),
					 GTK_RESPONSE_OK);
	gtk_dialog_set_response_sensitive (GTK_DIALOG (dlg), 
					   GTK_RESPONSE_OK, FALSE);
	
	g_signal_connect (G_OBJECT (dlg), 
			  "response",
			  G_CALLBACK (response_handler),
			  NULL);
				   
	gui = glade_xml_new (GEDIT_GLADEDIR "uri.glade2",
			     "open_uri_dialog_content", 
			     NULL);
			
	if (!gui)
	{
		gchar *error_str;
		
		error_str = g_strdup_printf (GEDIT_MISSING_FILE, 
					     GEDIT_GLADEDIR "uri.glade2");
					     
		show_error_message (dlg,
				    error_str);
				
		g_free (error_str);
						    
		return;
	}

	content = glade_xml_get_widget (gui, "open_uri_dialog_content");
	dlg->priv->uri = glade_xml_get_widget (gui, "uri");
	dlg->priv->uri_list = glade_xml_get_widget (gui, "uri_list");
	encoding_label = glade_xml_get_widget (gui, "encoding_label");
	encoding_hbox = glade_xml_get_widget (gui, "encoding_hbox");
	
	if (!content                ||
	    !dlg->priv->uri         ||
	    !dlg->priv->uri_list    ||
	    !encoding_label         ||
	    !encoding_hbox) 
	{
		gchar *error_str;
		
		error_str = g_strdup_printf (GEDIT_MISSING_WIDGETS, 
					     GEDIT_GLADEDIR "uri.glade2");
					     
		show_error_message (dlg,
				    error_str);

		g_free (error_str);
		g_object_unref (gui);
				    
		return;
	}
		
	dlg->priv->encoding_menu = gedit_encodings_option_menu_new (FALSE);

	gtk_label_set_mnemonic_widget (GTK_LABEL (encoding_label),
				       dlg->priv->encoding_menu);

	gtk_box_pack_end (GTK_BOX (encoding_hbox), 
			  dlg->priv->encoding_menu,
			  TRUE,
			  TRUE,
			  0);

	gtk_widget_show (dlg->priv->encoding_menu);

	g_signal_connect (G_OBJECT (dlg->priv->uri), 
			  "changed",
			  G_CALLBACK (entry_changed), 
			  dlg);
			  
	gtk_box_pack_start (GTK_BOX (GTK_DIALOG (dlg)->vbox),
			    content, FALSE, FALSE, 0);

	g_object_unref (gui);
}

GtkWidget *
gedit_open_location_dialog_new (GtkWindow *parent)
{
	GtkWidget *dlg;
	
	dlg = GTK_WIDGET (g_object_new (GEDIT_TYPE_OPEN_LOCATION_DIALOG, NULL));
	
	if (parent != NULL)
		gtk_window_set_transient_for (GTK_WINDOW (dlg),
					      parent);
					      
	return dlg;
}

/* Always return a valid gnome vfs uri or NULL */
gchar *
gedit_open_location_dialog_get_uri (GeditOpenLocationDialog *dlg)
{
	const gchar *str;
	gchar *uri;
	gchar *canonical_uri;
	GnomeVFSURI *vfs_uri;
	
	g_return_val_if_fail (GEDIT_IS_OPEN_LOCATION_DIALOG (dlg), NULL);
	
	str = gtk_entry_get_text (GTK_ENTRY (dlg->priv->uri));		
	g_return_val_if_fail (str != NULL, NULL);
	
	if (str[0] == '\0')
		return NULL;
		
	uri = gnome_vfs_make_uri_from_input_with_dirs (str,
						       GNOME_VFS_MAKE_URI_DIR_CURRENT);
	g_return_val_if_fail (uri != NULL, NULL);
	
	canonical_uri = gnome_vfs_make_uri_canonical (uri);
	g_free (uri);
	
	g_return_val_if_fail (canonical_uri != NULL, NULL);
	
	vfs_uri = gnome_vfs_uri_new (canonical_uri);
	if (vfs_uri == NULL)
	{
		g_free (canonical_uri);
		return NULL;
	}

	gnome_vfs_uri_unref (vfs_uri);
	return canonical_uri;
}

const GeditEncoding *
gedit_open_location_dialog_get_encoding	(GeditOpenLocationDialog *dlg)
{
	g_return_val_if_fail (GEDIT_IS_OPEN_LOCATION_DIALOG (dlg), NULL);
	
	return gedit_encodings_option_menu_get_selected_encoding (
				GEDIT_ENCODINGS_OPTION_MENU (dlg->priv->encoding_menu));
}
