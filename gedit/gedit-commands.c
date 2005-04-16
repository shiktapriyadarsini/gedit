/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */
/*
 * gedit-commands.c
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
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, 
 * Boston, MA 02111-1307, USA. 
 */
 
/*
 * Modified by the gedit Team, 1998-2005. See the AUTHORS file for a 
 * list of people on the gedit Team.  
 * See the ChangeLog files for a list of changes. 
 *
 * $Id$
 */

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <string.h>

#include <glib/gi18n.h>
#include <gtk/gtk.h>
#include <libgnome/gnome-help.h>
#include <libgnome/gnome-url.h>

#include "gedit-commands.h"
#include "gedit-debug.h"
#include "gedit-view.h"
#include "gedit-utils.h"
#include "gedit-print.h"
#include "dialogs/gedit-page-setup-dialog.h"
#include "dialogs/gedit-dialogs.h"
#include "dialogs/gedit-preferences-dialog.h"
#include "dialogs/gedit-close-confirmation-dialog.h"
#include "gedit-file-chooser-dialog.h"
#include "gedit-notebook.h"
#include "gedit-app.h"
#include "gedit-search-panel.h"

#define GEDIT_OPEN_DIALOG_KEY "gedit-open-dialog-key"

void
gedit_cmd_file_new (GtkAction *action, GeditWindow *window)
{
	gedit_window_create_tab (window, TRUE);
}

static void
open_dialog_destroyed (GeditWindow *window, GeditFileChooserDialog *dialog)
{
	g_object_set_data (G_OBJECT (window),
			   GEDIT_OPEN_DIALOG_KEY,
			   NULL);
}

static void
open_dialog_response_cb (GeditFileChooserDialog *dialog,
                         gint response_id,
                         GeditWindow *window)
{
	if (response_id != GTK_RESPONSE_OK)
	{
		gtk_widget_destroy (GTK_WIDGET (dialog));

		return;
	}
}        
                 
void
gedit_cmd_file_open (GtkAction *action, GeditWindow *window)
{
	GtkWidget *open_dialog;
	gpointer data;
	
	data = g_object_get_data (G_OBJECT (window), GEDIT_OPEN_DIALOG_KEY);
	
	if ((data != NULL) && (GEDIT_IS_FILE_CHOOSER_DIALOG (data)))
	{
		gtk_window_present (GTK_WINDOW (data));
		
		return;
	}

	open_dialog = gedit_file_chooser_dialog_new (_("Open Files..."),
						     GTK_WINDOW (window),
						     GTK_FILE_CHOOSER_ACTION_OPEN,
						     NULL,
						     GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL,
						     GTK_STOCK_OPEN, GTK_RESPONSE_OK,
						     NULL);

	g_object_set_data (G_OBJECT (window),
			   GEDIT_OPEN_DIALOG_KEY,
			   open_dialog);

	g_object_weak_ref (G_OBJECT (open_dialog),
			   (GWeakNotify) open_dialog_destroyed,
			   window);
		
	/* TODO: set the default path */
		   
	g_signal_connect (open_dialog,
			  "response",
			  G_CALLBACK (open_dialog_response_cb),
			  window);
			   						     
	gtk_widget_show (open_dialog);

#if 0
	BonoboMDIChild *active_child;

	gedit_debug (DEBUG_COMMANDS);

	active_child = bonobo_mdi_get_active_child (BONOBO_MDI (gedit_mdi));

	gedit_file_open ((GeditMDIChild*) active_child);
#endif
#if 0
	GeditDocument *doc;

	gedit_debug (DEBUG_COMMANDS);

	doc = gedit_window_get_active_document (window);
	if (doc == NULL)	
		return;

	gedit_document_load (doc, 
			     "file:///mnt/nslu2/paolo/gnome/gnome-210/cvs/gedit/ChangeLog",
			     NULL,
			     FALSE);
#endif			     
}

void
gedit_cmd_file_save (GtkAction *action, GeditWindow *window)
{
#if 0
	GeditMDIChild *active_child;
	GtkWidget *active_view;

	gedit_debug (DEBUG_COMMANDS);

	active_view = gedit_get_active_view ();

	active_child = GEDIT_MDI_CHILD (bonobo_mdi_get_active_child (BONOBO_MDI (gedit_mdi)));
	if (active_child == NULL)
		return;

	if (active_view != NULL)
		gtk_widget_grab_focus (active_view);

	gedit_file_save (active_child, TRUE);
#endif
}

void
gedit_cmd_file_save_as (GtkAction *action, GeditWindow *window)
{
#if 0
	GeditMDIChild *active_child;

	gedit_debug (DEBUG_COMMANDS);

	active_child = GEDIT_MDI_CHILD (bonobo_mdi_get_active_child (BONOBO_MDI (gedit_mdi)));
	if (active_child == NULL)
		return;

	gedit_file_save_as (active_child);
#endif
}

void
gedit_cmd_file_save_all (GtkAction *action, GeditWindow *window)
{
#if 0
	gedit_debug (DEBUG_COMMANDS);

	gedit_file_save_all ();
#endif
}

void
gedit_cmd_file_revert (GtkAction *action, GeditWindow *window)
{
#if 0
	GeditMDIChild *active_child;

	gedit_debug (DEBUG_COMMANDS);

	active_child = GEDIT_MDI_CHILD (bonobo_mdi_get_active_child (BONOBO_MDI (gedit_mdi)));
	if (active_child == NULL)
		return;

	gedit_file_revert (active_child);
#endif
}

void
gedit_cmd_file_open_uri (GtkAction *action, GeditWindow *window)
{
#if 0
	gedit_debug (DEBUG_COMMANDS);

	gedit_dialog_open_uri ();
#endif
}

void
gedit_cmd_file_page_setup (GtkAction *action, GeditWindow *window)
{
	gedit_debug (DEBUG_COMMANDS);

	gedit_show_page_setup_dialog (GTK_WINDOW (window));
}

void
gedit_cmd_file_print_preview (GtkAction *action, GeditWindow *window)
{
	GeditDocument *doc;

	gedit_debug (DEBUG_COMMANDS);

	doc = gedit_window_get_active_document (window);
	if (doc == NULL)	
		return;

	gedit_print_preview (GTK_WINDOW (window), doc);
}

void
gedit_cmd_file_print (GtkAction *action, GeditWindow *window)
{
	GeditDocument *doc;

	gedit_debug (DEBUG_COMMANDS);

	doc = gedit_window_get_active_document (window);
	if (doc == NULL)	
		return;

	gedit_print (GTK_WINDOW (window), doc);
}

gboolean
_gedit_cmd_file_can_close (GeditTab *tab, GtkWindow *window)
{
	GeditDocument *doc;
	gboolean       close = TRUE;
	
	gedit_debug (DEBUG_COMMANDS);
	
	doc = gedit_tab_get_document (tab);
	
	if (gtk_text_buffer_get_modified (GTK_TEXT_BUFFER (doc)) || 
	    gedit_document_get_deleted (doc))
	{
		GtkWidget *dlg;
		
		dlg = gedit_close_confirmation_dialog_new_single (
					window, 
					doc);
				 
		close = gedit_close_confirmation_dialog_run (
					GEDIT_CLOSE_CONFIRMATION_DIALOG (dlg));
		
		// TODO: salvare il documenta se necessario
		g_print ("Close? %s\n", close ? "TRUE": "FALSE");
		
		gtk_widget_destroy (dlg);		
	}
	
	return close;
}

void
gedit_cmd_file_close (GtkAction *action, GeditWindow *window)
{
	GeditTab      *active_tab;
	
	gedit_debug (DEBUG_COMMANDS);
	
	active_tab = gedit_window_get_active_tab (window);
	if (active_tab == NULL)
		return;	
			
	if (_gedit_cmd_file_can_close (active_tab, GTK_WINDOW (window)))
		gedit_window_close_tab (window, active_tab);
}

void 
gedit_cmd_file_close_all (GtkAction *action, GeditWindow *window)
{
	GSList   *unsaved_docs;
	GList    *docs;
	GList    *l;
	gboolean  close = FALSE;
	
	gedit_debug (DEBUG_COMMANDS);
	
	unsaved_docs = NULL;
	docs = gedit_window_get_documents (window);
	
	l = docs;
	
	while (l != NULL)
	{
		GeditDocument *doc = GEDIT_DOCUMENT (l->data);
		
		if (gtk_text_buffer_get_modified (GTK_TEXT_BUFFER (doc)) || 
		    gedit_document_get_deleted (doc))
		{
			unsaved_docs = g_slist_prepend (unsaved_docs, doc);
		}
		
		l = g_list_next (l);
	}
	g_list_free (docs);
	
	if (unsaved_docs == NULL)
	{
		/* Close all documents */
		gedit_window_close_all_tabs (window);
		return;
	}
	
	unsaved_docs = g_slist_reverse (unsaved_docs);
	
	if (unsaved_docs->next == NULL)
	{
		/* There is only one usaved doc */
		
		GeditTab      *tab;
		GtkWidget     *dlg;
		GeditDocument *doc;
		
		doc = GEDIT_DOCUMENT (unsaved_docs->data);
		
		tab = gedit_tab_get_from_document (doc);
		g_return_if_fail (tab != NULL);
		
		gedit_window_set_active_tab (window, tab);
				
		dlg = gedit_close_confirmation_dialog_new_single (
					GTK_WINDOW (window), 
					doc);
				 
		close = gedit_close_confirmation_dialog_run (
					GEDIT_CLOSE_CONFIRMATION_DIALOG (dlg));
		
		gtk_widget_hide (dlg);
		
		if (close)
		{
			// TODO: salvare il documento se necessario
		}
		
		gtk_widget_destroy (dlg);
	}
	else
	{
		/* There are more than one unsaved docs */
		
		GtkWidget *dlg;
		
		dlg = gedit_close_confirmation_dialog_new (GTK_WINDOW (window), 
							   unsaved_docs);

		close = gedit_close_confirmation_dialog_run (
					GEDIT_CLOSE_CONFIRMATION_DIALOG (dlg));
		
		gtk_widget_hide (dlg);
		
		if (close)
		{
			GSList *sel_docs;

			sel_docs = gedit_close_confirmation_dialog_get_selected_documents
							(GEDIT_CLOSE_CONFIRMATION_DIALOG (dlg));

			if (sel_docs != NULL)
			{
				// TODO: salvare i documenti se necessario
			}
			
			g_slist_free (sel_docs);
		}
		
		gtk_widget_destroy (dlg);
	}
	
	if (close)
		/* Close all documents */
		gedit_window_close_all_tabs (window);
}

void
gedit_cmd_file_quit (GtkAction *action, GeditWindow *window)
{
#if 0
	gedit_debug (DEBUG_COMMANDS);

	gedit_file_exit ();	
#endif
}

void
gedit_cmd_edit_undo (GtkAction *action, GeditWindow *window)
{
	GeditView *active_view;
	GtkSourceBuffer *active_document;

	active_view = gedit_window_get_active_view (window);
	g_return_if_fail (active_view);

	active_document = GTK_SOURCE_BUFFER (gtk_text_view_get_buffer (GTK_TEXT_VIEW (active_view)));

	gtk_source_buffer_undo (active_document);

	gedit_view_scroll_to_cursor (active_view);

	gtk_widget_grab_focus (GTK_WIDGET (active_view));
}

void
gedit_cmd_edit_redo (GtkAction *action, GeditWindow *window)
{
	GeditView *active_view;
	GtkSourceBuffer *active_document;

	active_view = gedit_window_get_active_view (window);
	g_return_if_fail (active_view);

	active_document = GTK_SOURCE_BUFFER (gtk_text_view_get_buffer (GTK_TEXT_VIEW (active_view)));

	gtk_source_buffer_redo (active_document);

	gedit_view_scroll_to_cursor (active_view);

	gtk_widget_grab_focus (GTK_WIDGET (active_view));
}

void 
gedit_cmd_edit_cut (GtkAction *action, GeditWindow *window)
{
	GeditView *active_view;

	active_view = gedit_window_get_active_view (window);
	g_return_if_fail (active_view);

	gedit_view_cut_clipboard (active_view); 

	gtk_widget_grab_focus (GTK_WIDGET (active_view));
}

void 
gedit_cmd_edit_copy (GtkAction *action, GeditWindow *window)
{
	GeditView *active_view;

	active_view = gedit_window_get_active_view (window);
	g_return_if_fail (active_view);

	gedit_view_copy_clipboard (active_view);

	gtk_widget_grab_focus (GTK_WIDGET (active_view));
}

void 
gedit_cmd_edit_paste (GtkAction *action, GeditWindow *window)
{
	GeditView *active_view;

	active_view = gedit_window_get_active_view (window);
	g_return_if_fail (active_view);

	gedit_view_paste_clipboard (active_view);

	gtk_widget_grab_focus (GTK_WIDGET (active_view));
}

void 
gedit_cmd_edit_delete (GtkAction *action, GeditWindow *window)
{
	GeditView *active_view;

	active_view = gedit_window_get_active_view (window);
	g_return_if_fail (active_view);

	gedit_view_delete_selection (active_view);

	gtk_widget_grab_focus (GTK_WIDGET (active_view));
}

void
gedit_cmd_edit_select_all (GtkAction *action, GeditWindow *window)
{
	GeditView *active_view;

	active_view = gedit_window_get_active_view (window);
	g_return_if_fail (active_view);

	gedit_view_select_all (active_view);

	gtk_widget_grab_focus (GTK_WIDGET (active_view));
}

void
gedit_cmd_edit_preferences (GtkAction *action, GeditWindow *window)
{
	gedit_debug (DEBUG_COMMANDS);

	gedit_show_preferences_dialog (GTK_WINDOW (window));
}

void
gedit_cmd_view_show_toolbar (GtkAction *action, GeditWindow *window)
{
	gboolean visible;

	gedit_debug (DEBUG_COMMANDS);

	visible = gtk_toggle_action_get_active (GTK_TOGGLE_ACTION (action));

	_gedit_window_set_toolbar_visible (window, visible);
}

void
gedit_cmd_view_show_statusbar (GtkAction *action, GeditWindow *window)
{
	gboolean visible;

	gedit_debug (DEBUG_COMMANDS);

	visible = gtk_toggle_action_get_active (GTK_TOGGLE_ACTION (action));

	_gedit_window_set_statusbar_visible (window, visible);
}

void
gedit_cmd_view_show_side_pane (GtkAction *action, GeditWindow *window)
{
	gboolean visible;

	gedit_debug (DEBUG_COMMANDS);

	visible = gtk_toggle_action_get_active (GTK_TOGGLE_ACTION (action));

	_gedit_window_set_side_panel_visible (window, visible);
}

void
gedit_cmd_view_show_bottom_panel (GtkAction *action, GeditWindow *window)
{
	gboolean visible;

	gedit_debug (DEBUG_COMMANDS);

	visible = gtk_toggle_action_get_active (GTK_TOGGLE_ACTION (action));

	// TODO
}

void 
gedit_cmd_search_find (GtkAction *action, GeditWindow *window)
{
	GeditSearchPanel *sp;
	
	gedit_debug (DEBUG_COMMANDS);
	
	sp = GEDIT_SEARCH_PANEL (_gedit_window_get_search_panel (window));
	
	gedit_search_panel_focus_search (sp);
}

#if 0
static void
search_find_again (GeditWindow   *window,
		   GeditDocument *doc,
		   gchar         *last_searched_text,
		   gboolean       backward)
{

	gpointer data;
	gboolean found;
	gboolean was_wrap_around;
	gint flags = 0;

	data = g_object_get_qdata (G_OBJECT (doc), gedit_was_wrap_around_quark ());
	if (data == NULL)
		was_wrap_around = TRUE;
	else
		was_wrap_around = GPOINTER_TO_BOOLEAN (data);

	GEDIT_SEARCH_SET_FROM_CURSOR (flags, TRUE);

	if (!backward)
		found = gedit_document_find_next (doc, flags);
	else
		found = gedit_document_find_prev (doc, flags);
		
	if (!found && was_wrap_around)
	{
		GEDIT_SEARCH_SET_FROM_CURSOR (flags, FALSE);
		
		if (!backward)
			found = gedit_document_find_next (doc, flags);
		else
			found = gedit_document_find_prev (doc, flags);
	}

	if (!found)
	{	
		GtkWidget *message_dlg;

		message_dlg = gtk_message_dialog_new (GTK_WINDOW (window),
						      GTK_DIALOG_MODAL | GTK_DIALOG_DESTROY_WITH_PARENT,
						      GTK_MESSAGE_INFO,
						      GTK_BUTTONS_OK,
						      _("The text \"%s\" was not found."),
						      last_searched_text);

		gtk_dialog_set_default_response (GTK_DIALOG (message_dlg),
						 GTK_RESPONSE_OK);

		gtk_window_set_resizable (GTK_WINDOW (message_dlg), FALSE);

		gtk_dialog_run (GTK_DIALOG (message_dlg));
		gtk_widget_destroy (message_dlg);
	}
	else
	{
		GeditView *active_view;

		active_view = gedit_window_get_active_view (window);
		g_return_if_fail (active_view != NULL);

		gedit_view_scroll_to_cursor (active_view);
	}
}
#endif

void 
gedit_cmd_search_find_next (GtkAction *action, GeditWindow *window)
{
#if 0
	GeditDocument *doc;
	gchar* last_searched_text;

	gedit_debug (DEBUG_COMMANDS);

	doc = gedit_window_get_active_document (window);
	g_return_if_fail (doc);

	last_searched_text = gedit_document_get_last_searched_text (doc);

	if (last_searched_text != NULL)
		search_find_again (window, doc, last_searched_text, FALSE);
	else
		gedit_dialog_find (window);

	g_free (last_searched_text);
#endif
}

void 
gedit_cmd_search_find_prev (GtkAction *action, GeditWindow *window)
{
#if 0
	GeditDocument *doc;
	gchar* last_searched_text;

	gedit_debug (DEBUG_COMMANDS);

	doc = gedit_window_get_active_document (window);
	if (doc == NULL)
		return;

	last_searched_text = gedit_document_get_last_searched_text (doc);

	if (last_searched_text != NULL)
		search_find_again (window, doc, last_searched_text, TRUE);
	else
		gedit_dialog_find (window);

	g_free (last_searched_text);
#endif
}

void 
gedit_cmd_search_replace (GtkAction *action, GeditWindow *window)
{
#if 0
	GeditView *active_view;

	gedit_debug (DEBUG_COMMANDS);

	active_view = gedit_window_get_active_view (window);

	if (active_view != NULL)
		gtk_widget_grab_focus (GTK_WIDGET (active_view));

	gedit_dialog_replace (window);
#endif
}

void
gedit_cmd_search_goto_line (GtkAction *action, GeditWindow *window)
{
	GeditSearchPanel *sp;
	
	gedit_debug (DEBUG_COMMANDS);
	
	sp = GEDIT_SEARCH_PANEL (_gedit_window_get_search_panel (window));
	
	gedit_search_panel_focus_goto_line (sp);
}

void
gedit_cmd_documents_move_to_new_window (GtkAction *action, GeditWindow *window)
{
	GeditNotebook *old_notebook;
	GeditTab *tab;

	tab = gedit_window_get_active_tab (window);
	
	if (tab == NULL)
		return;
		
	old_notebook = GEDIT_NOTEBOOK (_gedit_window_get_notebook (window));
	
	if (gtk_notebook_get_n_pages (GTK_NOTEBOOK (old_notebook)) <= 1)
		return;
		
	_gedit_window_move_tab_to_new_window (window, tab);			 
}

void 
gedit_cmd_help_contents (GtkAction *action, GeditWindow *window)
{
	GError *error = NULL;

	gedit_debug (DEBUG_COMMANDS);

	gnome_help_display ("gedit.xml", NULL, &error);

	if (error != NULL)
	{
		gedit_warning (GTK_WINDOW (window), error->message);
		g_error_free (error);
	}
}

static void
activate_url (GtkAboutDialog *about, const gchar *url, gpointer data)
{
	gnome_url_show (url, NULL);
}

void
gedit_cmd_help_about (GtkAction *action, GeditWindow *window)
{
	static const gchar * const authors[] = {
		"Paolo Maggi <paolo@gnome.org>",
		"Paolo Borelli <pborelli@katamail.com>",
		"James Willcox <jwillcox@gnome.org>",
		"Chema Celorio",
		"Federico Mena Quintero <federico@ximian.com>",
		NULL
	};

	static const gchar * const documenters[] = {
		"Sun GNOME Documentation Team <gdocteam@sun.com>",
		"Eric Baudais <baudais@okstate.edu>",
		NULL
	};

	static const gchar copyright[] = \
		"Copyright \xc2\xa9 1998-2000 Evan Lawrence, Alex Robert\n"
		"Copyright \xc2\xa9 2000-2002 Chema Celorio, Paolo Maggi\n"
		"Copyright \xc2\xa9 2003-2005 Paolo Maggi";

	static const gchar comments[] = \
		N_("gedit is a small and lightweight text editor for the "
		   "GNOME Desktop");

	static GdkPixbuf *logo = NULL;

	if(!logo) {
		logo = gdk_pixbuf_new_from_file (
			GNOME_ICONDIR "/gedit-logo.png",
			NULL);

		gtk_about_dialog_set_url_hook (activate_url, NULL, NULL);
	}

	gtk_show_about_dialog (
		GTK_WINDOW (window),
		"authors",		authors,
		"comments",		_(comments),
		"copyright",		copyright,
		"documenters",		documenters,
		"logo",			logo,
		"translator-credits",   _("translator-credits"),
		"version",		VERSION,
		"website",		"http://www.gedit.org",
		"name",			_("gedit"),
		NULL
		);
}

