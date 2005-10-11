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
#include <libgnomevfs/gnome-vfs.h>

#include "gedit-commands.h"
#include "gedit-window.h"
#include "gedit-window-private.h"
#include "gedit-statusbar.h"
#include "gedit-debug.h"
#include "gedit-view.h"
#include "gedit-utils.h"
#include "gedit-help.h"
#include "gedit-print.h"
#include "gedit-recent.h"
#include "dialogs/gedit-page-setup-dialog.h"
#include "dialogs/gedit-preferences-dialog.h"
#include "dialogs/gedit-close-confirmation-dialog.h"
#include "dialogs/gedit-open-location-dialog.h"
#include "gedit-file-chooser-dialog.h"
#include "gedit-notebook.h"
#include "gedit-app.h"
#include "gedit-search-panel.h"

#define GEDIT_OPEN_DIALOG_KEY "gedit-open-dialog-key"
#define GEDIT_OPEN_LOCATION_DIALOG_KEY "gedit-open-location-dialog-key"
#define GEDIT_TAB_TO_SAVE_AS  "gedit-tab-to-save-as"

static void 		file_save_as (GeditTab    *tab, 
				      GeditWindow *window);

static gint
load_file_list (GeditWindow         *window,
		const GSList        *uris,
		const GeditEncoding *encoding,
		gint                 line_pos,
		gboolean             create)
{
	gint loaded_files = 0;
	gboolean ret;
	GeditTab *tab;
	GeditDocument *doc;
	gboolean jump_to = TRUE;
	gboolean flash = TRUE;

	g_return_val_if_fail (GEDIT_IS_WINDOW (window), 0);
	g_return_val_if_fail (uris != NULL && uris->data != NULL, 0);

	tab = gedit_window_get_active_tab (window);
	if (tab != NULL)
	{
		doc = gedit_tab_get_document (tab);

		if (gedit_document_is_untouched (doc) &&
		    (gedit_tab_get_state (tab) == GEDIT_TAB_STATE_NORMAL))
		{
			const gchar *uri;

			uri = (const gchar *)uris->data;
			ret = _gedit_tab_load (tab,
					       uri,
					       encoding,
					       line_pos,
					       create);

			uris = g_slist_next (uris);
			jump_to = FALSE;

			if (ret)
			{
				if (uris == NULL)
				{
					/* There is only a single file to load */
					gchar *uri_for_display;

					uri_for_display = gnome_vfs_format_uri_for_display (uri);

					gedit_statusbar_flash_message (GEDIT_STATUSBAR (window->priv->statusbar),
								       window->priv->generic_message_cid,
								       _("Loading file '%s'..."),
								       uri_for_display);

					g_free (uri_for_display);

					flash = FALSE;								       
				}

				++loaded_files;
			}
		}
	}

	while (uris != NULL)
	{
		g_return_val_if_fail (uris->data != NULL, 0);

		tab = gedit_window_create_tab_from_uri (window,
							(const gchar *)uris->data,
							encoding,
							line_pos,
							create,
							jump_to);

		if (tab != NULL)
		{
			jump_to = FALSE;
			++loaded_files;	
		}

		uris = g_slist_next (uris);
	}

	if (flash)
	{
		if (loaded_files == 1)
		{
			GeditDocument *doc;
			g_return_val_if_fail (tab != NULL, loaded_files);
			
			doc = gedit_tab_get_document (tab);
			gedit_statusbar_flash_message (GEDIT_STATUSBAR (window->priv->statusbar),
						       window->priv->generic_message_cid,
						       _("Loading file '%s'..."),
						       gedit_document_get_uri_for_display (doc));
		}
		else
		{
			gedit_statusbar_flash_message (GEDIT_STATUSBAR (window->priv->statusbar),
						       window->priv->generic_message_cid,
						       ngettext("Loading %d file...",
								"Loading %d files...", 
								loaded_files),
						       loaded_files);
		}
	}				     

	return loaded_files;
}

/* exported so it can be used for drag'n'drop */
gint
gedit_cmd_load_files (GeditWindow         *window,
		      const GSList        *uris,
		      const GeditEncoding *encoding)
{
	return load_file_list (window, uris, encoding, 0, FALSE);
}

/*
 * From the command line we can specify a line position for the 
 * first doc. Beside specifying a not existing uri crates a 
 * titled document.
 */
gint
gedit_cmd_load_files_from_prompt (GeditWindow         *window,
				  const GSList        *uris,
				  const GeditEncoding *encoding,
				  gint                 line_pos)
{
	return load_file_list (window, uris, encoding, line_pos, TRUE);
}

void
gedit_cmd_file_new (GtkAction   *action,
		    GeditWindow *window)
{
	gedit_window_create_tab (window, TRUE);
}

static void
open_dialog_destroyed (GeditWindow            *window,
		       GeditFileChooserDialog *dialog)
{
	g_object_set_data (G_OBJECT (window),
			   GEDIT_OPEN_DIALOG_KEY,
			   NULL);
}

static void
open_dialog_response_cb (GeditFileChooserDialog *dialog,
                         gint                    response_id,
                         GeditWindow            *window)
{
	GSList *uris, *l;
	const GeditEncoding *encoding;
	gint n;

	gedit_debug (DEBUG_COMMANDS);
	
	if (response_id != GTK_RESPONSE_OK)
	{
		gtk_widget_destroy (GTK_WIDGET (dialog));

		return;
	}

	uris = gtk_file_chooser_get_uris (GTK_FILE_CHOOSER (dialog));
	
	l = uris;
	while (l != NULL)
	{
		g_print ("%s\n", (char *)l->data);
		l = g_slist_next (l);
	}
	
	encoding = gedit_file_chooser_dialog_get_encoding (dialog);

	gtk_widget_destroy (GTK_WIDGET (dialog));

	g_return_if_fail (uris != NULL); /* CHECK */
		
	n = gedit_cmd_load_files (window,
		 		  uris,
				  encoding);

	g_slist_foreach (uris, (GFunc) g_free, NULL);
	g_slist_free (uris);
}

void
gedit_cmd_file_open (GtkAction   *action,
		     GeditWindow *window)
{
	GtkWidget *open_dialog;
	gpointer data;
	GeditDocument *doc;
	gchar *default_path = NULL;
	
	data = g_object_get_data (G_OBJECT (window), GEDIT_OPEN_DIALOG_KEY);

	if (data != NULL)
	{
		g_return_if_fail (GEDIT_IS_FILE_CHOOSER_DIALOG (data));

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

	doc = gedit_window_get_active_document (window);
	if (doc != NULL)
	{
		const gchar *uri;
		
		uri = gedit_document_get_uri_ (doc);
		
		if ((uri != NULL) && gedit_utils_uri_has_file_scheme (uri))
		{	
			default_path = g_path_get_dirname (uri);

			g_return_if_fail (strlen (default_path) >= 5 /* strlen ("file:") */);
			if (strcmp (default_path, "file:") == 0)
			{
				g_free (default_path);
		
				default_path = g_strdup ("file:///");
			}
		}
	}
	
	if (default_path == NULL)
		default_path = g_strdup (_gedit_window_get_default_path (window));
		
	if (default_path != NULL)
		gtk_file_chooser_set_current_folder_uri (GTK_FILE_CHOOSER (open_dialog),
							 default_path);
	
	g_free (default_path);
						  	
	g_signal_connect (open_dialog,
			  "response",
			  G_CALLBACK (open_dialog_response_cb),
			  window);

	gtk_widget_show (open_dialog);
}

static void
open_location_dialog_destroyed (GeditWindow *window,
				gpointer     data)
{
	g_object_set_data (G_OBJECT (window),
			   GEDIT_OPEN_LOCATION_DIALOG_KEY,
			   NULL);
}

static void
open_location_dialog_response_cb (GeditOpenLocationDialog *dlg,
				  gint                    response_id,
				  GeditWindow             *window)
{
	gchar *uri;
	GSList *uris = NULL;
	const GeditEncoding *encoding;
	
	gedit_debug (DEBUG_COMMANDS);
	
	if (response_id != GTK_RESPONSE_OK)
	{
		gtk_widget_destroy (GTK_WIDGET (dlg));

		return;
	}
	
	uri = gedit_open_location_dialog_get_uri (dlg);
	if (uri == NULL)
	{
		GtkWidget *msg;
		GtkWindowGroup *wg;
		
		wg = GTK_WINDOW (dlg)->group;
		
		if (wg == NULL)
		{
			wg = gtk_window_group_new ();
			
			gtk_window_group_add_window (wg, GTK_WINDOW (dlg));
		}
		
		msg = gtk_message_dialog_new (GTK_WINDOW (dlg),
					      GTK_DIALOG_DESTROY_WITH_PARENT,
					      GTK_MESSAGE_ERROR,
					      GTK_BUTTONS_CLOSE,
					      _("The entered location is not valid."));

		gtk_message_dialog_format_secondary_text (GTK_MESSAGE_DIALOG (msg),
							  _("Please, check that you typed the location correctly and try again."));
		
		gtk_window_group_add_window (wg, GTK_WINDOW (msg));
		
		g_signal_connect (G_OBJECT (msg),
				  "response",
				  G_CALLBACK (gtk_widget_destroy),
				  NULL);

		gtk_window_set_resizable (GTK_WINDOW (msg), FALSE);
		gtk_window_set_modal (GTK_WINDOW (msg), TRUE);
		
		gtk_widget_show (msg);

		return;
	}

	encoding = gedit_open_location_dialog_get_encoding (dlg);
	uris = g_slist_prepend (uris, uri);

	gtk_widget_destroy (GTK_WIDGET (dlg));
		
	gedit_cmd_load_files (window,
		 	      uris,
			      encoding);

	g_slist_foreach (uris, (GFunc) g_free, NULL);
	g_slist_free (uris);
}

void
gedit_cmd_file_open_uri (GtkAction   *action,
			 GeditWindow *window)
{
	GtkWidget *dlg;
	gpointer data;

	data = g_object_get_data (G_OBJECT (window), GEDIT_OPEN_LOCATION_DIALOG_KEY);

	if (data != NULL)
	{
		g_return_if_fail (GEDIT_IS_OPEN_LOCATION_DIALOG (data));

		gtk_window_present (GTK_WINDOW (data));

		return;
	}
		
	dlg = gedit_open_location_dialog_new (GTK_WINDOW (window));

	g_object_set_data (G_OBJECT (window),
			   GEDIT_OPEN_LOCATION_DIALOG_KEY,
			   dlg);

	g_object_weak_ref (G_OBJECT (dlg),
			   (GWeakNotify) open_location_dialog_destroyed,
			   window);
	
	g_signal_connect (dlg,
			  "response",
			  G_CALLBACK (open_location_dialog_response_cb),
			  window);
			  			      
	gtk_widget_show (dlg);
}

void
gedit_cmd_file_open_recent (EggRecentItem *item,
			    GeditWindow   *window)
{
	GSList *uris = NULL;
	gchar *uri;
	gint n;

	gedit_debug (DEBUG_COMMANDS);

	uri = egg_recent_item_get_uri (item);

	uris = g_slist_prepend (uris, uri);

	n = gedit_cmd_load_files (window,
		 		  uris,
				  NULL);

	if (n != 1)
	{
		gedit_recent_remove (uri);
	}

	g_free (uri);
	g_slist_free (uris);
}


static void
file_save (GeditTab    *tab,
	   GeditWindow *window)
{
	GeditDocument *doc;

	gedit_debug (DEBUG_COMMANDS);

	tab = gedit_window_get_active_tab (window);
	if (tab == NULL)
		return;

	doc = gedit_tab_get_document (tab);
	g_return_if_fail (GEDIT_IS_DOCUMENT (doc));

	if (gedit_document_is_untitled (doc))
	{
		gedit_debug_message (DEBUG_COMMANDS, "Untitled");

		return file_save_as (tab, window);
	}

	gedit_statusbar_flash_message (GEDIT_STATUSBAR (window->priv->statusbar),
				        window->priv->generic_message_cid,
				       _("Saving file \"%s\"..."),
				       gedit_document_get_uri_for_display (doc));

	_gedit_tab_save (tab);
}

void
gedit_cmd_file_save (GtkAction   *action,
		     GeditWindow *window)
{
	GeditTab *tab;

	gedit_debug (DEBUG_COMMANDS);

	tab = gedit_window_get_active_tab (window);
	if (tab == NULL)
		return;
	
	file_save (tab, window);
}

// CHECK: move to utils? If so, do not include vfs.h
static gboolean
is_read_only (const gchar *uri)
{
	gboolean ret = TRUE; /* default to read only */
	GnomeVFSFileInfo *info;

	g_return_val_if_fail (uri != NULL, FALSE);

	info = gnome_vfs_file_info_new ();

	/* FIXME: is GNOME_VFS_FILE_INFO_FOLLOW_LINKS right in this case? - Paolo */
	if (gnome_vfs_get_file_info (uri,
				     info,
				     GNOME_VFS_FILE_INFO_FOLLOW_LINKS | 
				     GNOME_VFS_FILE_INFO_GET_ACCESS_RIGHTS) == GNOME_VFS_OK)
	{
		if (info->valid_fields & GNOME_VFS_FILE_INFO_FIELDS_ACCESS)
		{
			ret = !(info->permissions & GNOME_VFS_PERM_ACCESS_WRITABLE);
		}
	}

	gnome_vfs_file_info_unref (info);

	return ret;
}

static gboolean
replace_read_only_file (GtkWindow   *parent,
			const gchar *uri)
{
	GtkWidget *dialog;
	gint ret;
	gchar *full_formatted_uri;
	gchar *uri_for_display	;
	gchar *message_with_uri;

	full_formatted_uri = gnome_vfs_format_uri_for_display (uri);
	g_return_val_if_fail (full_formatted_uri != NULL, FALSE);

	/* Truncate the URI so it doesn't get insanely wide. Note that even
	 * though the dialog uses wrapped text, if the URI doesn't contain
	 * white space then the text-wrapping code is too stupid to wrap it.
	 */
	uri_for_display = gedit_utils_str_middle_truncate (full_formatted_uri, 50);
	g_return_val_if_fail (uri_for_display != NULL, FALSE);
	g_free (full_formatted_uri);

	message_with_uri = g_strdup_printf (_("The file \"%s\" is read-only."),
					    uri_for_display);
	g_free (uri_for_display);

	dialog = gtk_message_dialog_new (parent,
					 GTK_DIALOG_DESTROY_WITH_PARENT,
					 GTK_MESSAGE_QUESTION,
					 GTK_BUTTONS_NONE,
					 message_with_uri);
	gtk_message_dialog_format_secondary_text (GTK_MESSAGE_DIALOG (dialog),
						  _("Do you want to try to replace it "
						    "with the one you are saving?"));
	g_free (message_with_uri);

	gtk_dialog_add_button (GTK_DIALOG (dialog),
			       GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL);

	gedit_dialog_add_button (GTK_DIALOG (dialog),
				 _("_Replace"),
			  	 GTK_STOCK_SAVE_AS,
			  	 GTK_RESPONSE_YES);

	gtk_dialog_set_default_response	(GTK_DIALOG (dialog),
					 GTK_RESPONSE_CANCEL);

	gtk_window_set_resizable (GTK_WINDOW (dialog), FALSE);

	ret = gtk_dialog_run (GTK_DIALOG (dialog));

	gtk_widget_destroy (dialog);

	return (ret == GTK_RESPONSE_YES);
}

static void
save_dialog_response_cb (GeditFileChooserDialog *dialog,
                         gint                    response_id,
                         GeditWindow            *window)
{
	gchar *uri;
	const GeditEncoding *encoding;
	GeditTab *tab;
	gboolean do_save = TRUE;

	gedit_debug (DEBUG_COMMANDS);

	if (response_id != GTK_RESPONSE_OK)
	{
		gtk_widget_destroy (GTK_WIDGET (dialog));

		return;
	}

	uri = gtk_file_chooser_get_uri (GTK_FILE_CHOOSER (dialog));
	encoding = gedit_file_chooser_dialog_get_encoding (dialog);

	g_return_if_fail (uri != NULL); /* CHECK */

	tab = GEDIT_TAB (g_object_get_data (G_OBJECT (dialog), GEDIT_TAB_TO_SAVE_AS));
	if (tab != NULL && do_save)
	{
		GeditDocument *doc;

		gchar *uri_for_display;

		doc = gedit_tab_get_document (tab);
		g_return_if_fail (GEDIT_IS_DOCUMENT (doc));

		uri_for_display = gnome_vfs_format_uri_for_display (uri);
		gedit_statusbar_flash_message (GEDIT_STATUSBAR (window->priv->statusbar),
					        window->priv->generic_message_cid,
					       _("Saving file as \"%s\"..."),
					       uri_for_display);

		_gedit_tab_save_as (tab, uri, encoding);	
	}

	gtk_widget_destroy (GTK_WIDGET (dialog));

	g_free (uri);
}

static GtkFileChooserConfirmation
confirm_overwrite_callback (GtkFileChooser *dialog,
			    gpointer        data)
{
	gchar *uri;

	uri = gtk_file_chooser_get_uri (dialog);

	if (is_read_only (uri))
	{
		if (replace_read_only_file (GTK_WINDOW (dialog), uri))
			return GTK_FILE_CHOOSER_CONFIRMATION_ACCEPT_FILENAME;
		else
			return GTK_FILE_CHOOSER_CONFIRMATION_SELECT_AGAIN; 
	}
	else
	{
		/* fall back to the default confirmation dialog */
		return GTK_FILE_CHOOSER_CONFIRMATION_CONFIRM;
	}
}

/* Save As dialog is modal to its main window */
static void
file_save_as (GeditTab    *tab,
	      GeditWindow *window)
{
	GtkWidget *save_dialog;
	GtkWindowGroup *wg;

	save_dialog = gedit_file_chooser_dialog_new (_("Save As..."),
						     GTK_WINDOW (window),
						     GTK_FILE_CHOOSER_ACTION_SAVE,
						     NULL,
						     GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL,
						     GTK_STOCK_SAVE, GTK_RESPONSE_OK,
						     NULL);

	gtk_file_chooser_set_do_overwrite_confirmation (GTK_FILE_CHOOSER (save_dialog), TRUE);
	g_signal_connect (save_dialog,
			  "confirm-overwrite",
			  G_CALLBACK (confirm_overwrite_callback),
			  NULL);

	wg = gedit_window_get_group (window);

	gtk_window_group_add_window (wg,
				     GTK_WINDOW (save_dialog));

	gtk_window_set_modal (GTK_WINDOW (save_dialog), TRUE);

	/* TODO: set the default path/name */

	g_object_set_data (G_OBJECT (save_dialog), GEDIT_TAB_TO_SAVE_AS, tab);
	
	g_signal_connect (save_dialog,
			  "response",
			  G_CALLBACK (save_dialog_response_cb),
			  window);
			  

	gtk_widget_show (save_dialog);
}

void
gedit_cmd_file_save_as (GtkAction   *action,
			GeditWindow *window)
{
	GeditTab *tab;

	gedit_debug (DEBUG_COMMANDS);

	tab = gedit_window_get_active_tab (window);
	if (tab == NULL)
		return;
	
	file_save_as (tab, window);
}

void
gedit_cmd_file_save_all (GtkAction   *action,
			 GeditWindow *window)
{
	GList *tabs, *l;

	g_return_if_fail (!(gedit_window_get_state (window) & GEDIT_WINDOW_STATE_PRINTING));
	
	tabs = gtk_container_get_children (
			GTK_CONTAINER (_gedit_window_get_notebook (window)));

	l = tabs;			
	while (l != NULL)
	{
		GeditTab *t;
		GeditTabState state;
		GeditDocument *doc;
		
		t = GEDIT_TAB (l->data);
		
		state = gedit_tab_get_state (t);
		doc = gedit_tab_get_document (t);
		
		g_return_if_fail (state != GEDIT_TAB_STATE_PRINTING);
		g_return_if_fail (state != GEDIT_TAB_STATE_PRINT_PREVIEWING);
				
		/* FIXME: manage readonly files */
		/* FIXME: what does it happen if there is for example a LOAD_ERROR or
		   a REVERTING_ERROR? */
		/* FIXME: what should we do if a file is being REVERTED? */
		   
		if ((state == GEDIT_TAB_STATE_NORMAL) ||
		    (state == GEDIT_TAB_STATE_SHOWING_PRINT_PREVIEW))
		{
			if (gedit_document_is_untitled (doc))
			{
				/* TODO: creare una list di file untitled */
			}
			else
			{
				_gedit_tab_save (t);	
			}
		}
		else
		{
			gedit_debug_message (DEBUG_COMMANDS,
					     "File '%s' not saved. State: %d",
					     gedit_document_get_uri_for_display (doc),
					     state);
		}
		
		l = g_list_next (l);
	}
/*
<paolo> sto implementando il Save All
<paolo> è molto meno banale di cosa pensassi
<paolo> e non so bene come risolvere un problema
<paolo> la mia implementazione funziona così
<paolo> - prendo la lista di tutte le tab
<paolo> - creo una lista di documenti untitled
<paolo> - salvo tutti i documenti con titolo e il cui stato è
<paolo> if ((state == GEDIT_TAB_STATE_NORMAL) ||
<paolo>     (state == GEDIT_TAB_STATE_SHOWING_PRINT_PREVIEW) ||
<paolo>     (state == GEDIT_TAB_STATE_GENERIC_NOT_EDITABLE))
<pbor> (io stavo leggendo http://bugzilla.gnome.org/show_bug.cgi?id=148218, che parla di mmap in glib... e mi ha fatto venire un dubbio: new_mdi funziona se il file e' lungo esattamente come una memory page? ossia non e' null term)
<paolo> - potenzialmente rimangono altri documenti
<paolo> (direi di sì, ma bisogna provare)
<paolo> a questo punto faccio save_as per tutti i documenti untitled
<paolo> uno dopo l'altro
<pbor> perche' non salvi se il doc e PRINT_PREVIEW?
<paolo> hmmm... in che senso?
<paolo> salvo nei tre casi NORMAL, SHOWING_PRINT_PREVIEW e GENERIC_NOT_EDITABLE
<pbor> ugh... scusa
<pbor> ok
<paolo> cmq... fino qui mi sembra tutto lineare (anche se non banale da implementare senza usare dialog_run)
<paolo> il problema sono i file che rimangono ossia quelli che erano per esempio "PRINTING" quando è stato attivato il save_all
<paolo> cosa ne faccio?
<paolo> io vedo due possibilità:
<pbor> e se disattivassimo il save_all?
<paolo> 1. fare finta di niente
<paolo> 2. mostrare una dlg con una lista di file non salvati
<paolo> disattivare la save_all è una possibilità... ma non mi attizza troppo
<paolo> ma forse alla fine è la più lineare
<paolo> perchè aggira il problema
<pbor> io invece opterei per disattivarlo come con il close_all
<paolo> in realtà il problema si pone solo per i file che sono nei seguenti stati:
<paolo> GEDIT_TAB_STATE_PRINTING, GEDIT_TAB_STATE_PRINT_PREVIEWING e GEDIT_TAB_STATE_SAVING_ERROR 
<paolo> se il file e LOADING o REVERTING non è il caso di salvarlo
<paolo> se è SAVING idem
<paolo> se è LOADING_ERROR o REVERTING error, c'è poco da salvare
<paolo> se è GENERIC_ERROR ... non so
<pbor> non puoi salvare mentre e' in print/print_preview?
<paolo> in teoria sì, il problema che è un casino da gestire con la message_area
<paolo> potremmo però salvare non appena escono dallo stato printing/print_previewing
<pbor> saving error non e' un problema: si skippa il file, come se la save fosse fallita
<paolo> infatti
<pbor> boh... io disabiliterei per ora
<pbor> e' la cosa piu semplice
<paolo> rimane solo il problema dei documenti untitled che sono in uno degli stati di cui sopra
<pbor> e non mi sembra un caso molto probabile
<pbor> al max metti un buon vecchio FIXME
<pbor> :-)
<paolo> ossia tu dici... quando lo stato della window e PRINTING disabilita Save All
<pbor> yep
<paolo> tanto gli altri casi non creano problemi
<paolo> ok, concordo
*/
	g_list_free (tabs);
}

static void
revert_dialog_response_cb (GtkDialog     *dialog,
			   gint           response_id,
			   GeditWindow   *window)
{
	GeditTab *tab;
	GeditDocument *doc;

	gedit_debug (DEBUG_COMMANDS);

	// CHECK: we are relying on the fact that the dialog is modal
	// so the active tab can't have changed... not very nice

	tab = gedit_window_get_active_tab (window);
	if (tab == NULL)
		return;

	if (response_id != GTK_RESPONSE_OK)
	{
		gtk_widget_destroy (GTK_WIDGET (dialog));

		return;
	}

	doc = gedit_tab_get_document (tab);
	gedit_statusbar_flash_message (GEDIT_STATUSBAR (window->priv->statusbar),
				        window->priv->generic_message_cid,
				       _("Reverting the document \"%s\"..."),
				       gedit_document_get_uri_for_display (doc));

	_gedit_tab_revert (tab);

	gtk_widget_destroy (GTK_WIDGET (dialog));
}

static GtkWidget *
revert_dialog (GeditWindow   *window,
	       GeditDocument *doc)
{
	GtkWidget *dialog;
	const gchar *name;
	gchar *primary_msg;
	gchar *secondary_msg;
	glong seconds;

	gedit_debug (DEBUG_COMMANDS);

	name = gedit_document_get_short_name_for_display (doc);
	primary_msg = g_strdup_printf (_("Revert unsaved changes to document \"%s\"?"),
	                               name);

	seconds = MAX (1, _gedit_document_get_seconds_since_last_save_or_load (doc));

	if (seconds < 55)
	{
		secondary_msg = g_strdup_printf (
					ngettext ("Changes made to the document in the last %ld second "
					    	  "will be permanently lost.",
						  "Changes made to the document in the last %ld seconds "
					    	  "will be permanently lost.",
						  seconds),
					seconds);
	}
	else if (seconds < 75) /* 55 <= seconds < 75 */
	{
		secondary_msg = g_strdup (_("Changes made to the document in the last minute "
					    "will be permanently lost."));
	}
	else if (seconds < 110) /* 75 <= seconds < 110 */
	{
		secondary_msg = g_strdup_printf (
					ngettext ("Changes made to the document in the last minute and "
						  "%ld second will be permanently lost.",
						  "Changes made to the document in the last minute and "
						  "%ld seconds will be permanently lost.",
						  seconds - 60 ),
					seconds - 60);
	}
	else if (seconds < 3600)
	{
		secondary_msg = g_strdup_printf (
					ngettext ("Changes made to the document in the last %ld minute "
					    	  "will be permanently lost.",
						  "Changes made to the document in the last %ld minutes "
					    	  "will be permanently lost.",
						  seconds / 60),
					seconds / 60);
	}
	else if (seconds < 7200)
	{
		gint minutes;
		seconds -= 3600;

		minutes = seconds / 60;
		if (minutes < 5)
		{
			secondary_msg = g_strdup (_("Changes made to the document in the last hour "
						    "will be permanently lost."));
		}
		else
		{
			secondary_msg = g_strdup_printf (
					ngettext ("Changes made to the document in the last hour and "
						  "%d minute will be permanently lost.",
						  "Changes made to the document in the last hour and "
						  "%d minutes will be permanently lost.",
						  minutes),
					minutes);
		}
	}
	else
	{
		gint hours;

		hours = seconds / 3600;

		secondary_msg = g_strdup_printf (
					ngettext ("Changes made to the document in the last hour "
					    	  "will be permanently lost.",
						  "Changes made to the document in the last %d hours "
					    	  "will be permanently lost.",
						  hours),
					hours);
	}

	dialog = gtk_message_dialog_new (GTK_WINDOW (window),
					 GTK_DIALOG_DESTROY_WITH_PARENT,
					 GTK_MESSAGE_QUESTION,
					 GTK_BUTTONS_NONE,
					 primary_msg);
	gtk_message_dialog_format_secondary_text (GTK_MESSAGE_DIALOG (dialog),
						  secondary_msg);
	g_free (primary_msg);
	g_free (secondary_msg);

	gtk_window_set_resizable (GTK_WINDOW (dialog), FALSE);

	gtk_dialog_add_button (GTK_DIALOG (dialog),
			       GTK_STOCK_CANCEL,
			       GTK_RESPONSE_CANCEL);

	gedit_dialog_add_button (GTK_DIALOG (dialog), 
				 _("_Revert"),
				 GTK_STOCK_REVERT_TO_SAVED,
				 GTK_RESPONSE_OK);

	gtk_dialog_set_default_response	(GTK_DIALOG (dialog),
					 GTK_RESPONSE_CANCEL);

	return dialog;
}

void
gedit_cmd_file_revert (GtkAction   *action,
		       GeditWindow *window)
{
	GeditDocument *doc;
	GtkWidget *dialog;
	GtkWindowGroup *wg;

	gedit_debug (DEBUG_COMMANDS);

	doc = gedit_window_get_active_document (window);
	g_return_if_fail (doc != NULL);

	// CHECK: do not show the confirmation dialog if doc is unmodified?

	dialog = revert_dialog (window, doc);

	wg = gedit_window_get_group (window);

	gtk_window_group_add_window (wg, GTK_WINDOW (dialog));

	gtk_window_set_modal (GTK_WINDOW (dialog), TRUE);

	g_signal_connect (dialog,
			  "response",
			  G_CALLBACK (revert_dialog_response_cb),
			  window);

	gtk_widget_show (dialog);
}

void
gedit_cmd_file_page_setup (GtkAction   *action,
			   GeditWindow *window)
{
	gedit_show_page_setup_dialog (GTK_WINDOW (window));
}

void
gedit_cmd_file_print_preview (GtkAction   *action,
			      GeditWindow *window)
{
	GeditDocument *doc;
	GeditTab      *tab;
	GeditPrintJob *pjob;
	GtkTextIter    start;
	GtkTextIter    end;	
	
	gedit_debug (DEBUG_COMMANDS);

	tab = gedit_window_get_active_tab (window);
	if (tab == NULL)	
		return;

	doc = gedit_tab_get_document (tab);

	pjob = gedit_print_job_new (doc);

	gtk_text_buffer_get_bounds (GTK_TEXT_BUFFER (doc) , &start, &end);

	_gedit_tab_print_preview (tab, pjob, &start, &end);
	g_object_unref (pjob);
}

static void
print_dialog_response_cb (GtkWidget *dialog, 
			  gint response, 
			  GeditPrintJob *pjob)
{
	GtkTextIter start, end;
	gint line_start, line_end;
	GnomePrintRangeType range_type;
	GtkTextBuffer *buffer;
	GeditTab *tab;
	
	range_type = gnome_print_dialog_get_range (GNOME_PRINT_DIALOG (dialog));
	
	buffer = GTK_TEXT_BUFFER (gtk_source_print_job_get_buffer (GTK_SOURCE_PRINT_JOB (pjob)));
	
	gtk_text_buffer_get_bounds (buffer, &start, &end);

	tab = gedit_tab_get_from_document (GEDIT_DOCUMENT (buffer));
	
	switch (range_type)
	{
		case GNOME_PRINT_RANGE_ALL:
			break;

		case GNOME_PRINT_RANGE_SELECTION:
			gtk_text_buffer_get_selection_bounds (buffer,
							      &start, 
							      &end);
			break;

		case GNOME_PRINT_RANGE_RANGE:
			gnome_print_dialog_get_range_page (GNOME_PRINT_DIALOG (dialog),
							   &line_start, 
							   &line_end);

			gtk_text_iter_set_line (&start, line_start - 1);
			gtk_text_iter_set_line (&end, line_end - 1);
			
			gtk_text_iter_forward_to_line_end (&end);
			break;

		default:
			g_return_if_reached ();
	}

	switch (response)
	{
		case GNOME_PRINT_DIALOG_RESPONSE_PRINT:
			gedit_debug_message (DEBUG_PRINT, "Print button pressed.");			
			
			_gedit_tab_print (tab, pjob, &start, &end);
			
			break;

		case GNOME_PRINT_DIALOG_RESPONSE_PREVIEW:
			gedit_debug_message (DEBUG_PRINT, "Preview button pressed.");
			
			_gedit_tab_print_preview (tab, pjob, &start, &end);

			break;
        }
        
        g_object_unref (pjob);
	gtk_widget_destroy (dialog);
} 

void
gedit_cmd_file_print (GtkAction   *action,
		      GeditWindow *window)
{
	GeditDocument *doc;
	GeditPrintJob *pjob;
	GtkWidget *print_dialog;
	GtkWindowGroup *wg;
	
	gedit_debug (DEBUG_COMMANDS);

	doc = gedit_window_get_active_document (window);
	if (doc == NULL)
		return;

	pjob = gedit_print_job_new (doc);

	print_dialog = gedit_print_dialog_new (pjob);

	wg = gedit_window_get_group (window);

	gtk_window_group_add_window (wg,
				     GTK_WINDOW (print_dialog));

	gtk_window_set_transient_for (GTK_WINDOW (print_dialog),
				      GTK_WINDOW (window));
	gtk_window_set_modal (GTK_WINDOW (print_dialog), TRUE);

	g_signal_connect (print_dialog,
			  "response",
			  G_CALLBACK (print_dialog_response_cb),
			  pjob);

	gtk_widget_show (print_dialog);
}

static gboolean 
really_close_tab (GeditTab *tab)
{
	GtkWidget *toplevel; 
	g_return_val_if_fail (gedit_tab_get_state (tab) == GEDIT_TAB_STATE_CLOSING, 
			      FALSE);
	
	toplevel = gtk_widget_get_toplevel (GTK_WIDGET (tab));
	g_return_val_if_fail (GEDIT_IS_WINDOW (toplevel), FALSE);
	
	gedit_window_close_tab (GEDIT_WINDOW (toplevel), tab);
	
	return FALSE;
}

static void
tab_state_changed_while_saving (GeditTab    *tab, 
				GParamSpec  *pspec,
				GeditWindow *window)
{
	GeditTabState ts;
	
	ts = gedit_tab_get_state (tab);
	
	gedit_debug_message (DEBUG_COMMANDS, "State while saving: %d\n", ts);
	
	if (ts == GEDIT_TAB_STATE_NORMAL)
	{
		GeditDocument *doc;
		
		g_signal_handlers_disconnect_by_func (tab,
						      G_CALLBACK (tab_state_changed_while_saving), 
					      	      window);

		doc = gedit_tab_get_document (tab);
		g_return_if_fail (doc != NULL);
		
		if (gtk_text_buffer_get_modified (GTK_TEXT_BUFFER (doc)) || 
		    gedit_document_get_deleted (doc))
			return;
			
		/* Close the document only if it has been succesfully saved */		
		_gedit_tab_mark_for_closing (tab);

		g_idle_add_full (G_PRIORITY_HIGH_IDLE,
				 (GSourceFunc)really_close_tab,
				 tab,
				 NULL);
	}
}

static void
save_and_close (GeditTab    *tab,
		GeditWindow *window)
{
	g_signal_connect (tab, 
			  "notify::state",
			  G_CALLBACK (tab_state_changed_while_saving),
			  window);

	file_save (tab, window);
	
}
/* Returns TRUE if the tab can be immediately closed */
gboolean
_gedit_cmd_file_can_close (GeditTab  *tab,
			   GtkWindow *window)
{
	GeditDocument *doc;
	gboolean       close = TRUE;
	GeditTabState  ts;

	gedit_debug (DEBUG_COMMANDS);

	doc = gedit_tab_get_document (tab);
	ts = gedit_tab_get_state (tab);
	
	/* TODO: we need to save the file also if it has been externally 
	   modified - Paolo (Oct 10, 2005) */
	if ((ts != GEDIT_TAB_STATE_LOADING_ERROR) && 
	    (gtk_text_buffer_get_modified (GTK_TEXT_BUFFER (doc)) || 
	     gedit_document_get_deleted (doc)))	     
	{
		GtkWidget *dlg;

		dlg = gedit_close_confirmation_dialog_new_single (
					window, 
					doc);
				 
		close = gedit_close_confirmation_dialog_run (
					GEDIT_CLOSE_CONFIRMATION_DIALOG (dlg));
		
		if (close)
		{
			if (gedit_close_confirmation_dialog_get_selected_documents (
					GEDIT_CLOSE_CONFIRMATION_DIALOG (dlg)) != NULL)
			{
				close = FALSE;
				save_and_close (tab, GEDIT_WINDOW (window));
			}
		}
		
		gtk_widget_destroy (dlg);		
	}
	
	return close;
}

void
gedit_cmd_file_close (GtkAction   *action,
		      GeditWindow *window)
{
	GeditTab *active_tab;

	gedit_debug (DEBUG_COMMANDS);

	active_tab = gedit_window_get_active_tab (window);
	if (active_tab == NULL)
		return;	

	if (_gedit_cmd_file_can_close (active_tab, GTK_WINDOW (window)))
		gedit_window_close_tab (window, active_tab);
}

void 
gedit_cmd_file_close_all (GtkAction   *action,
			  GeditWindow *window)
{
	GSList *unsaved_docs;
	GList *docs;
	GList *l;
	gboolean close = FALSE;

	g_return_if_fail (!(gedit_window_get_state (window) & GEDIT_WINDOW_STATE_SAVING));
	
	gedit_debug (DEBUG_COMMANDS);

	unsaved_docs = NULL;
	docs = gedit_window_get_documents (window);

	for (l = docs; l != NULL; l = l->next)
	{
		GeditDocument *doc = GEDIT_DOCUMENT (l->data);

		if (gtk_text_buffer_get_modified (GTK_TEXT_BUFFER (doc)) || 
		    gedit_document_get_deleted (doc))
		{
			unsaved_docs = g_slist_prepend (unsaved_docs, doc);
		}
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
		
		GeditTab *tab;
		GtkWidget *dlg;
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
gedit_cmd_file_quit (GtkAction   *action,
		     GeditWindow *window)
{
	g_return_if_fail (!(gedit_window_get_state (window) & GEDIT_WINDOW_STATE_SAVING));
	
	gedit_cmd_file_close_all (NULL, window);

	gtk_widget_destroy (GTK_WIDGET (window));
}

void
gedit_cmd_edit_undo (GtkAction   *action,
		     GeditWindow *window)
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
gedit_cmd_edit_redo (GtkAction   *action,
		     GeditWindow *window)
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
gedit_cmd_edit_cut (GtkAction   *action,
		    GeditWindow *window)
{
	GeditView *active_view;

	active_view = gedit_window_get_active_view (window);
	g_return_if_fail (active_view);

	gedit_view_cut_clipboard (active_view); 

	gtk_widget_grab_focus (GTK_WIDGET (active_view));
}

void 
gedit_cmd_edit_copy (GtkAction   *action,
		     GeditWindow *window)
{
	GeditView *active_view;

	active_view = gedit_window_get_active_view (window);
	g_return_if_fail (active_view);

	gedit_view_copy_clipboard (active_view);

	gtk_widget_grab_focus (GTK_WIDGET (active_view));
}

void 
gedit_cmd_edit_paste (GtkAction   *action,
		      GeditWindow *window)
{
	GeditView *active_view;

	active_view = gedit_window_get_active_view (window);
	g_return_if_fail (active_view);

	gedit_view_paste_clipboard (active_view);

	gtk_widget_grab_focus (GTK_WIDGET (active_view));
}

void 
gedit_cmd_edit_delete (GtkAction   *action,
		       GeditWindow *window)
{
	GeditView *active_view;

	active_view = gedit_window_get_active_view (window);
	g_return_if_fail (active_view);

	gedit_view_delete_selection (active_view);

	gtk_widget_grab_focus (GTK_WIDGET (active_view));
}

void
gedit_cmd_edit_select_all (GtkAction   *action,
			   GeditWindow *window)
{
	GeditView *active_view;

	active_view = gedit_window_get_active_view (window);
	g_return_if_fail (active_view);

	gedit_view_select_all (active_view);

	gtk_widget_grab_focus (GTK_WIDGET (active_view));
}

void
gedit_cmd_edit_preferences (GtkAction   *action,
			    GeditWindow *window)
{
	gedit_debug (DEBUG_COMMANDS);

	gedit_show_preferences_dialog (window);
}

void
gedit_cmd_view_show_toolbar (GtkAction   *action,
			     GeditWindow *window)
{
	gboolean visible;

	gedit_debug (DEBUG_COMMANDS);

	visible = gtk_toggle_action_get_active (GTK_TOGGLE_ACTION (action));

	_gedit_window_set_toolbar_visible (window, visible);
}

void
gedit_cmd_view_show_statusbar (GtkAction   *action,
			       GeditWindow *window)
{
	gboolean visible;

	gedit_debug (DEBUG_COMMANDS);

	visible = gtk_toggle_action_get_active (GTK_TOGGLE_ACTION (action));

	_gedit_window_set_statusbar_visible (window, visible);
}

void
gedit_cmd_view_show_side_pane (GtkAction   *action,
			       GeditWindow *window)
{
	gboolean visible;

	gedit_debug (DEBUG_COMMANDS);

	visible = gtk_toggle_action_get_active (GTK_TOGGLE_ACTION (action));

	_gedit_window_set_side_panel_visible (window, visible);
}

void
gedit_cmd_view_show_bottom_panel (GtkAction   *action,
				  GeditWindow *window)
{
	gboolean visible;

	gedit_debug (DEBUG_COMMANDS);

	visible = gtk_toggle_action_get_active (GTK_TOGGLE_ACTION (action));

	_gedit_window_set_bottom_panel_visible (window, visible);
}


void
gedit_cmd_documents_move_to_new_window (GtkAction   *action,
					GeditWindow *window)
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

