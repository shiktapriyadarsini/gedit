/*
 * gedit-tab.c
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

#include <glib/gi18n.h>

#include <libgnomeui/libgnomeui.h>
#include <libgnomevfs/gnome-vfs-mime-handlers.h>
#include <libgnomevfs/gnome-vfs-utils.h>

#include "gedit-app.h"
#include "gedit-notebook.h"
#include "gedit-tab.h"
#include "gedit-utils.h"
#include "gedit-message-area.h"
#include "gedit-io-error-message-area.h"
#include "gedit-print-job-preview.h"
#include "gedit-progress-message-area.h"
#include "gedit-debug.h"
#include "gedit-prefs-manager-app.h"
#include "gedit-recent.h"

#define GEDIT_TAB_GET_PRIVATE(object)(G_TYPE_INSTANCE_GET_PRIVATE ((object), GEDIT_TYPE_TAB, GeditTabPrivate))

#define GEDIT_TAB_KEY "GEDIT_TAB_KEY"

struct _GeditTabPrivate
{
	GeditTabState	 state;
	GtkWidget	*view;
	GtkWidget	*view_scrolled_window;

	GtkWidget	*message_area;
	GtkWidget	*print_preview;

	GeditPrintJob   *print_job;

	/* tmp data for saving */
	gchar		*save_uri;
	
	GTimer 		*timer;
	guint		 times_called;
	
	gboolean	 not_editable;
};

G_DEFINE_TYPE(GeditTab, gedit_tab, GTK_TYPE_VBOX)

enum
{
	PROP_0,
	PROP_NAME,
	PROP_STATE
};

static void
gedit_tab_set_property (GObject      *object,
		        guint         prop_id,
		        const GValue *value,
		        GParamSpec   *pspec)
{
	GeditTab *tab = GEDIT_TAB (object);

	switch (prop_id)
	{
		/* FIXME: do we really need state to be a writable property */
		case PROP_STATE:
			gedit_tab_set_state (tab,
					     g_value_get_int (value));
			break;			
		default:
			G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
			break;			
	}
}

static void
gedit_tab_get_property (GObject    *object,
		        guint       prop_id,
		        GValue     *value,
		        GParamSpec *pspec)
{
	GeditTab *tab = GEDIT_TAB (object);

	switch (prop_id)
	{
		case PROP_NAME:
			g_value_take_string (value,
					     _gedit_tab_get_name (tab));
			break;
		case PROP_STATE:
			g_value_set_int (value,
					 gedit_tab_get_state (tab));
			break;			
		default:
			G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
			break;			
	}
}

static void
gedit_tab_finalize (GObject *object)
{
	GeditTab *tab = GEDIT_TAB (object);
	
	if (tab->priv->print_job != NULL)
	{
		g_print ("Cancelling printing\n");
		
		gtk_source_print_job_cancel (GTK_SOURCE_PRINT_JOB (tab->priv->print_job));
		g_object_unref (tab->priv->print_job);
	}
	
	if (tab->priv->timer != NULL)
		g_timer_destroy (tab->priv->timer);

	g_free (tab->priv->save_uri);

	G_OBJECT_CLASS (gedit_tab_parent_class)->finalize (object);
}

static void 
gedit_tab_class_init (GeditTabClass *klass)
{
	GObjectClass *object_class = G_OBJECT_CLASS (klass);

	object_class->finalize = gedit_tab_finalize;
	object_class->get_property = gedit_tab_get_property;
	object_class->set_property = gedit_tab_set_property;
	
	g_object_class_install_property (object_class,
					 PROP_NAME,
					 g_param_spec_string ("name",
							      "Name",
							      "The tab's name",
							      "",
							      G_PARAM_READABLE));
							      
	g_object_class_install_property (object_class,
					 PROP_STATE,
					 g_param_spec_int ("state",
							   "State",
							   "The tab's state",
							   0, /* GEDIT_TAB_STATE_NORMAL */
							   GEDIT_TAB_NUM_OF_STATES - 1,
							   0, /* GEDIT_TAB_STATE_NORMAL */
							   G_PARAM_READWRITE));							      	
							      
	g_type_class_add_private (object_class, sizeof(GeditTabPrivate));
}

GeditTabState
gedit_tab_get_state (GeditTab *tab)
{
	g_return_val_if_fail (GEDIT_IS_TAB (tab), GEDIT_TAB_STATE_NORMAL);
	
	return tab->priv->state;
}

static void
view_realized (GtkTextView *view,
	       GeditTab    *tab)
{
	g_return_if_fail (gtk_text_view_get_window (view, GTK_TEXT_WINDOW_TEXT) != NULL);
	g_return_if_fail (gtk_text_view_get_window (view, GTK_TEXT_WINDOW_LEFT) != NULL);
	
	if ((tab->priv->state == GEDIT_TAB_STATE_LOADING) ||
	    (tab->priv->state == GEDIT_TAB_STATE_REVERTING) ||
	    (tab->priv->state == GEDIT_TAB_STATE_SAVING) ||
	    (tab->priv->state == GEDIT_TAB_STATE_PRINTING) ||
	    (tab->priv->state == GEDIT_TAB_STATE_PRINT_PREVIEWING))
	{
		GdkCursor *cursor;
	
		cursor = gdk_cursor_new_for_display (
				gtk_widget_get_display (GTK_WIDGET (view)),
				GDK_WATCH);
				
		gdk_window_set_cursor (gtk_text_view_get_window (view, GTK_TEXT_WINDOW_TEXT),
				       cursor);
				       
		gdk_window_set_cursor (gtk_text_view_get_window (view, GTK_TEXT_WINDOW_LEFT),
				       cursor);

		gdk_cursor_unref (cursor);		
	}
	else
	{
		gdk_window_set_cursor (gtk_text_view_get_window (view, GTK_TEXT_WINDOW_TEXT),
				       NULL);
				       
		gdk_window_set_cursor (gtk_text_view_get_window (view, GTK_TEXT_WINDOW_LEFT),
				       NULL);
	}
}

void
gedit_tab_set_state (GeditTab *tab,
		     GeditTabState state)
{
	g_return_if_fail (GEDIT_IS_TAB (tab));
	g_return_if_fail ((state >= 0) && (state < GEDIT_TAB_NUM_OF_STATES));
		
	if (tab->priv->state == state)
		return;

	tab->priv->state = state;

	if (state == GEDIT_TAB_STATE_NORMAL)
		gtk_text_view_set_editable (GTK_TEXT_VIEW (tab->priv->view), 
					    !tab->priv->not_editable);
	else		
		gtk_text_view_set_editable (GTK_TEXT_VIEW (tab->priv->view), 
					    FALSE);

	if (state == GEDIT_TAB_STATE_LOADING) // FIXME: add other states if needed
	{
		g_object_set (G_OBJECT (tab->priv->view),
			      "highlight_current_line", FALSE,
			      "cursor-visible", FALSE,
			      NULL);
	}
	else
	{
		g_object_set (G_OBJECT (tab->priv->view),
			      "highlight_current_line", gedit_prefs_manager_get_highlight_current_line (),
			      "cursor-visible", TRUE,
			      NULL);
	}
		
	if ((state == GEDIT_TAB_STATE_LOADING_ERROR) ||
	    (state == GEDIT_TAB_STATE_SHOWING_PRINT_PREVIEW))	
		gtk_widget_hide (tab->priv->view_scrolled_window);
	else
	{
		if (tab->priv->print_preview == NULL)
			gtk_widget_show (tab->priv->view_scrolled_window);
	}
	
	if (gtk_text_view_get_window (GTK_TEXT_VIEW (tab->priv->view), GTK_TEXT_WINDOW_TEXT) != NULL)
		view_realized (GTK_TEXT_VIEW (tab->priv->view), tab);
	
	g_object_notify (G_OBJECT (tab), "state");		
}

static void 
document_name_modified_changed (GeditDocument *document, GeditTab *tab)
{
	g_object_notify (G_OBJECT (tab), "name");
}

static void
set_message_area (GeditTab  *tab,
		  GtkWidget *message_area)
{
	if (tab->priv->message_area == message_area)
		return;

	if (tab->priv->message_area != NULL)
		gtk_widget_destroy (tab->priv->message_area);

	tab->priv->message_area = message_area;

	if (message_area == NULL)
		return;

	gtk_box_pack_start (GTK_BOX (tab),
			    tab->priv->message_area,
			    FALSE,
			    FALSE,
			    0);		

	g_object_add_weak_pointer (G_OBJECT (tab->priv->message_area), 
				   (gpointer *)&tab->priv->message_area);
}

static void 
unrecoverable_loading_error_message_area_response (GeditMessageArea *message_area,
						   gint              response_id,
						   GtkWidget        *tab)
{
	GeditNotebook *notebook;
	gboolean can_close = TRUE;

	notebook = GEDIT_NOTEBOOK (gtk_widget_get_parent (tab));
	
	g_signal_emit_by_name (G_OBJECT (notebook), "tab_delete", tab, &can_close);

	if (can_close)
		gedit_notebook_remove_tab (notebook, GEDIT_TAB (tab));
}

static void 
file_already_open_warning_message_area_response (GtkWidget   *message_area,
						 gint         response_id,
						 GeditTab    *tab)
{
	GeditView *view;
	
	view = gedit_tab_get_view (tab);
	
	if (response_id == GTK_RESPONSE_YES)
	{
		tab->priv->not_editable = FALSE;
		
		gtk_text_view_set_editable (GTK_TEXT_VIEW (view),
					    TRUE);
	}

	gtk_widget_destroy (message_area);

	gtk_widget_grab_focus (GTK_WIDGET (view));	
}

static void
load_cancelled (GeditMessageArea *area,
                gint              response_id,
                GeditTab         *tab)
{
	g_return_if_fail (GEDIT_IS_PROGRESS_MESSAGE_AREA (tab->priv->message_area));
	
	g_object_ref (tab);
	gedit_document_load_cancel (gedit_tab_get_document (tab));
	g_object_unref (tab);	
}

#define MAX_MSG_LENGTH 100

static void
show_loading_message_area (GeditTab *tab)
{
	GtkWidget *area;
	GeditDocument *doc = NULL;
	const gchar *short_name;
	gchar *name;
	gchar *dirname = NULL;
	gchar *msg = NULL;
	gint len;
	
	if (tab->priv->message_area != NULL)
		return;
	
	gedit_debug (DEBUG_DOCUMENT);
		
	doc = gedit_tab_get_document (tab);
	g_return_if_fail (doc != NULL);

	short_name = gedit_document_get_short_name_for_display (doc);

	len = g_utf8_strlen (short_name, -1);

	/* if the name is awfully long, truncate it and be done with it,
	 * otherwise also show the directory (ellipsized if needed)
	 */
	if (len > MAX_MSG_LENGTH)
	{
		name = gedit_utils_str_middle_truncate (short_name, 
							MAX_MSG_LENGTH);
	}
	else
	{
		const gchar *uri;
		gchar *str;

		name = g_strdup (short_name);

		uri = gedit_document_get_uri_for_display (doc);
		str = gedit_utils_uri_get_dirname (uri);

		if (str != NULL)
		{
			/* use the remaining space for the dir, but use a min of 20 chars
			 * so that we do not end up with a dirname like "(a...b)".
			 * This means that in the worst case when the filename is long 99
			 * we have a title long 99 + 20, but I think it's a rare enough
			 * case to be acceptable. It's justa darn title afterall :)
			 */
			dirname = gedit_utils_str_middle_truncate (str, 
								   MAX (20, MAX_MSG_LENGTH - len));
			g_free (str);
		}
	}

	if (tab->priv->state == GEDIT_TAB_STATE_REVERTING)
	{
		if (dirname != NULL)
		{
			/* Translators: the first %s is a file name (e.g. test.txt) the second one
			   is a directory (e.g. ssh://master.gnome.org/home/users/paolo) */
			msg = g_strdup_printf ("Reverting <b>%s</b> from <b>%s</b>",
					       name, dirname);
		}
		else
		{
			msg = g_strdup_printf ("Reverting <b>%s</b>", name);
		}
		
		area = gedit_progress_message_area_new (GTK_STOCK_REVERT_TO_SAVED,
							msg,
							TRUE);
	}
	else
	{
		if (dirname != NULL)
		{
			/* Translators: the first %s is a file name (e.g. test.txt) the second one
			   is a directory (e.g. ssh://master.gnome.org/home/users/paolo) */
			msg = g_strdup_printf ("Loading <b>%s</b> from <b>%s</b>",
					       name, dirname);
		}
		else
		{
			msg = g_strdup_printf ("Loading <b>%s</b>", name);
		}

		area = gedit_progress_message_area_new (GTK_STOCK_OPEN,
							msg,
							TRUE);
	}

	g_signal_connect (area,
			  "response",
			  G_CALLBACK (load_cancelled),
			  tab);
						 
	gtk_widget_show (area);

	set_message_area (tab, area);

	g_free (msg);
	g_free (name);
	g_free (dirname);
}

static void
show_saving_message_area (GeditTab *tab)
{
	GtkWidget *area;
	GeditDocument *doc = NULL;
	const gchar *short_name;
	gchar *from;
	gchar *to = NULL;
	gchar *msg = NULL;
	gint len;
	
	if (tab->priv->message_area != NULL)
		return;
	
	gedit_debug (DEBUG_DOCUMENT);
		
	doc = gedit_tab_get_document (tab);
	g_return_if_fail (doc != NULL);

	short_name = gedit_document_get_short_name_for_display (doc);

	len = g_utf8_strlen (short_name, -1);

	/* if the name is awfully long, truncate it and be done with it,
	 * otherwise also show the directory (ellipsized if needed)
	 */
	if (len > MAX_MSG_LENGTH)
	{
		from = gedit_utils_str_middle_truncate (short_name, 
							MAX_MSG_LENGTH);
	}
	else
	{
		gchar *str;
		
		from = g_strdup (short_name);

		to = gnome_vfs_format_uri_for_display (tab->priv->save_uri);

		str = gedit_utils_str_middle_truncate (to, 
						       MAX (20, MAX_MSG_LENGTH - len));
		g_free (to);
			
		to = str;
	}

	if (to != NULL)
	{
		/* Translators: the first %s is a file name (e.g. test.txt) the second one
		   is a directory (e.g. ssh://master.gnome.org/home/users/paolo) */
		msg = g_strdup_printf ("Saving <b>%s</b> to <b>%s</b>",
				       from, to);
	}
	else
	{
		msg = g_strdup_printf ("Saving <b>%s</b>", to);
	}

	area = gedit_progress_message_area_new (GTK_STOCK_SAVE,
						msg,
						FALSE);

	gtk_widget_show (area);

	set_message_area (tab, area);

	g_free (msg);
	g_free (to);
	g_free (from);
}

static void
message_area_set_progress (GeditTab	    *tab,
			   GnomeVFSFileSize  size,
			   GnomeVFSFileSize  total_size)
{
	if (tab->priv->message_area == NULL)
		return;
	
	gedit_debug_message (DEBUG_DOCUMENT, "%Ld/%Ld", size, total_size);
	
	g_return_if_fail (GEDIT_IS_PROGRESS_MESSAGE_AREA (tab->priv->message_area));
	
	if (total_size == 0)
	{
		if (size != 0)
			gedit_progress_message_area_pulse (
					GEDIT_PROGRESS_MESSAGE_AREA (tab->priv->message_area));	
		else
			gedit_progress_message_area_set_fraction (
				GEDIT_PROGRESS_MESSAGE_AREA (tab->priv->message_area),
				0);
	}
	else
	{
		gdouble frac;

		frac = (gdouble)size / (gdouble)total_size;

		gedit_progress_message_area_set_fraction (
				GEDIT_PROGRESS_MESSAGE_AREA (tab->priv->message_area),
				frac);		
	}
}

static void
document_loading (GeditDocument    *document,
		  GnomeVFSFileSize  size,
		  GnomeVFSFileSize  total_size,
		  GeditTab         *tab)
{
	double et;

	g_return_if_fail ((tab->priv->state == GEDIT_TAB_STATE_LOADING) ||
		 	  (tab->priv->state == GEDIT_TAB_STATE_REVERTING));

	gedit_debug_message (DEBUG_DOCUMENT, "%Ld/%Ld", size, total_size);
	
	if (tab->priv->timer == NULL)
	{
		g_return_if_fail (tab->priv->times_called == 0);
		tab->priv->timer = g_timer_new ();
	}

	et = g_timer_elapsed (tab->priv->timer, NULL);

	if (tab->priv->times_called == 1)
	{
		if ((total_size == 0) || (total_size > 51200UL) /* 50 KB */)
		{
			show_loading_message_area (tab);
		}
	}
	else
	{
		if ((tab->priv->times_called == 3) && (total_size != 0))
		{
			gdouble total_time;

			/* et : total_time = size : total_size */
			total_time = (et * total_size)/size;

			if ((total_time - et) > 3.0)
			{
				show_loading_message_area (tab);
			}
		}
		else
		{
			if (et > 3.0)
			{
				show_loading_message_area (tab);
			}
		}
	}
	
	message_area_set_progress (tab, size, total_size);

	tab->priv->times_called++;
}

// TODO: different error messages if tab->priv->state == GEDIT_TAB_STATE_REVERTING
static void
document_loaded (GeditDocument *document,
		 const GError  *error,
		 GeditTab      *tab)
{
	GtkWidget *emsg;
	const GeditEncoding *encoding;
	const gchar *uri;

	g_return_if_fail ((tab->priv->state == GEDIT_TAB_STATE_LOADING) ||
			  (tab->priv->state == GEDIT_TAB_STATE_REVERTING));

	g_timer_destroy (tab->priv->timer);
	tab->priv->timer = NULL;
	tab->priv->times_called = 0;
	
	set_message_area (tab, NULL);
	
	uri = gedit_document_get_uri_ (document);

	if (error != NULL)
	{
		if (tab->priv->state == GEDIT_TAB_STATE_LOADING)
			gedit_tab_set_state (tab, GEDIT_TAB_STATE_LOADING_ERROR);
		else
			gedit_tab_set_state (tab, GEDIT_TAB_STATE_REVERTING_ERROR);
			
		encoding = gedit_document_get_encoding (document);

		if (error->domain == GEDIT_DOCUMENT_ERROR)
		{
			if (error->code == GNOME_VFS_ERROR_CANCELLED)
			{	
				unrecoverable_loading_error_message_area_response (NULL,
										   0,
										   GTK_WIDGET (tab));
										   
				return;
			}
			else
			{
				gedit_recent_remove (uri);
				
				emsg = gedit_unrecoverable_loading_error_message_area_new (uri, 
										   error);

				set_message_area (tab, emsg);

				g_signal_connect (emsg,
						  "response",
						  G_CALLBACK (unrecoverable_loading_error_message_area_response),
						  tab);
			}
		}					  
		else
		{
			emsg = gedit_conversion_error_while_loading_message_area_new (
									uri,
									encoding,
									error);

			set_message_area (tab, emsg);

			// FIXME
			g_signal_connect (emsg,
					  "response",
					  G_CALLBACK (unrecoverable_loading_error_message_area_response),
					  tab);
					  
			// FIXME: ricordarsi di chiamare gedit_recent_remove (uri) nel response
		}

		gedit_message_area_set_default_response (GEDIT_MESSAGE_AREA (emsg),
							 GTK_RESPONSE_CANCEL);

		gtk_widget_show (emsg);

		return;
	}
	else
	{
		GList *all_documents;
		GList *l;

		g_return_if_fail (uri != NULL);
		
		gedit_recent_add (uri);

		all_documents = gedit_app_get_documents (gedit_app_get_default ());

		for (l = all_documents; l != NULL; l = g_list_next (l))
		{
			GeditDocument *d = GEDIT_DOCUMENT (l->data);
			
			if (d != document)
			{
				const gchar *u;
				
				u = gedit_document_get_uri_ (d);
								
				if ((u != NULL) &&
			    	    gnome_vfs_uris_match (uri, u))
			    	{
			    		GtkWidget *w;
			    		GeditView *view;

			    		view = gedit_tab_get_view (tab);

			    		tab->priv->not_editable = TRUE;

			    		w = gedit_file_already_open_warning_message_area_new (uri);

					set_message_area (tab, w);

					gedit_message_area_set_default_response (GEDIT_MESSAGE_AREA (w),
							 GTK_RESPONSE_CANCEL);

					gtk_widget_show (w);

					g_signal_connect (w,
							  "response",
							  G_CALLBACK (file_already_open_warning_message_area_response),
							  tab);
			    		break;
			    	}
			}
		}

		g_list_free (all_documents);

		gedit_tab_set_state (tab, GEDIT_TAB_STATE_NORMAL);
	}
}

static void
document_saving (GeditDocument    *document,
		 GnomeVFSFileSize  size,
		 GnomeVFSFileSize  total_size,
		 GeditTab         *tab)
{
	double et;

	g_return_if_fail (tab->priv->state == GEDIT_TAB_STATE_SAVING);

	gedit_debug_message (DEBUG_DOCUMENT, "%Ld/%Ld", size, total_size);
	
	if (tab->priv->timer == NULL)
	{
		g_return_if_fail (tab->priv->times_called == 0);
		tab->priv->timer = g_timer_new ();
	}

	et = g_timer_elapsed (tab->priv->timer, NULL);

	if (tab->priv->times_called == 1)
	{
		if ((total_size == 0) || (total_size > 51200UL) /* 50 KB */)
		{
			show_saving_message_area (tab);
		}
	}
	else
	{
		if ((tab->priv->times_called == 3) && (total_size != 0))
		{
			gdouble total_time;

			/* et : total_time = size : total_size */
			total_time = (et * total_size)/size;

			if ((total_time - et) > 3.0)
			{
				show_saving_message_area (tab);
			}
		}
		else
		{
			if (et > 3.0)
			{
				show_saving_message_area (tab);
			}
		}
	}
	
	message_area_set_progress (tab, size, total_size);

	tab->priv->times_called++;
}

static void
document_saved (GeditDocument *document,
		const GError  *error,
		GeditTab      *tab)
{
	GtkWidget *emsg;

	g_return_if_fail (tab->priv->state == GEDIT_TAB_STATE_SAVING);

	g_return_if_fail (tab->priv->save_uri != NULL);

	g_timer_destroy (tab->priv->timer);
	tab->priv->timer = NULL;
	tab->priv->times_called = 0;
	
	set_message_area (tab, NULL);
	
	if (error != NULL)
	{
		gedit_recent_remove (tab->priv->save_uri);
		
		gedit_tab_set_state (tab, GEDIT_TAB_STATE_SAVING_ERROR);

		emsg = gedit_unrecoverable_saving_error_message_area_new (tab->priv->save_uri, 
										  error);

		set_message_area (tab, emsg);

		g_signal_connect (emsg,
				  "response",
				  G_CALLBACK (gtk_widget_destroy),
				  tab);

		gtk_widget_show (emsg);
	}
	else
	{
		gedit_recent_add (tab->priv->save_uri);
		
		if (tab->priv->print_preview != NULL)
			gedit_tab_set_state (tab, GEDIT_TAB_STATE_SHOWING_PRINT_PREVIEW);
		else
			gedit_tab_set_state (tab, GEDIT_TAB_STATE_NORMAL);
	}

	g_free (tab->priv->save_uri);
	tab->priv->save_uri = NULL;
}

static void
gedit_tab_init (GeditTab *tab)
{
	GtkWidget *sw;
	GeditDocument *doc;

	tab->priv = GEDIT_TAB_GET_PRIVATE (tab);

	tab->priv->state = GEDIT_TAB_STATE_NORMAL;

	tab->priv->not_editable = FALSE;
	
	/* Create the scrolled window */
	sw = gtk_scrolled_window_new (NULL, NULL);
	tab->priv->view_scrolled_window = sw;

	gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW (sw),
					GTK_POLICY_AUTOMATIC,
					GTK_POLICY_AUTOMATIC);

	/* Create the view */
	doc = gedit_document_new ();
	g_object_set_data (G_OBJECT (doc), GEDIT_TAB_KEY, tab);

	tab->priv->view = gedit_view_new (doc);
	g_object_unref (doc);
	gtk_widget_show (tab->priv->view);
	g_object_set_data (G_OBJECT (tab->priv->view), GEDIT_TAB_KEY, tab);

	gtk_box_pack_end (GTK_BOX (tab), sw, TRUE, TRUE, 0);
	gtk_container_add (GTK_CONTAINER (sw), tab->priv->view);
	gtk_scrolled_window_set_shadow_type (GTK_SCROLLED_WINDOW (sw),
					     GTK_SHADOW_IN);	
	gtk_widget_show (sw);

	g_signal_connect (doc,
			  "name_changed",
			  G_CALLBACK (document_name_modified_changed),
			  tab);
	g_signal_connect (doc,
			  "modified_changed",
			  G_CALLBACK (document_name_modified_changed),
			  tab);
	g_signal_connect (doc,
			  "loading",
			  G_CALLBACK (document_loading),
			  tab);
	g_signal_connect (doc,
			  "loaded",
			  G_CALLBACK (document_loaded),
			  tab);
	g_signal_connect (doc,
			  "saving",
			  G_CALLBACK (document_saving),
			  tab);
	g_signal_connect (doc,
			  "saved",
			  G_CALLBACK (document_saved),
			  tab);
			  
	g_signal_connect_after(tab->priv->view,
			       "realize",
			       G_CALLBACK (view_realized),
			       tab);
}

GtkWidget *
_gedit_tab_new (void)
{
	return GTK_WIDGET (g_object_new (GEDIT_TYPE_TAB, NULL));
}

/* Whether create is TRUE, creates a new empty document if location does 
   not refer to an existing file */
GtkWidget *
_gedit_tab_new_from_uri (const gchar         *uri,
			 const GeditEncoding *encoding,
			 gint                 line_pos,			
			 gboolean             create)
{
	GeditTab *tab;
	GeditDocument *doc;
	gboolean ret;
	
	g_return_val_if_fail (uri != NULL, NULL);
	
	tab = GEDIT_TAB (_gedit_tab_new ());
	
	doc = gedit_tab_get_document (tab);

	gedit_tab_set_state (tab, GEDIT_TAB_STATE_LOADING);

	ret = gedit_document_load (doc, 
				   uri,
				   encoding,
				   line_pos,
				   create);

	if (!ret)
	{
		g_object_unref (tab);
		return NULL;
	}

	return GTK_WIDGET (tab);
}		

GeditView *
gedit_tab_get_view (GeditTab *tab)
{
	return GEDIT_VIEW (tab->priv->view);
}

/* This is only an helper function */
GeditDocument *
gedit_tab_get_document (GeditTab *tab)
{
	return GEDIT_DOCUMENT (gtk_text_view_get_buffer (
					GTK_TEXT_VIEW (tab->priv->view)));
}

#define MAX_DOC_NAME_LENGTH 40

gchar *
_gedit_tab_get_name (GeditTab *tab)
{
	GeditDocument *doc;
	const gchar* name = NULL;
	gchar* docname = NULL;
	gchar* tab_name = NULL;

	g_return_val_if_fail (GEDIT_IS_TAB (tab), NULL);

	doc = gedit_tab_get_document (tab);
	
	name = gedit_document_get_short_name_for_display (doc);

	/* Truncate the name so it doesn't get insanely wide. */
	docname = gedit_utils_str_middle_truncate (name, MAX_DOC_NAME_LENGTH);

	if (gtk_text_buffer_get_modified (GTK_TEXT_BUFFER (doc)))
	{
		tab_name = g_strdup_printf ("*%s", docname);
	} 
	else 
	{
 #if 0		
		if (gedit_document_is_readonly (doc)) 
		{
			tab_name = g_strdup_printf ("%s [%s]", docname, 
						/*Read only*/ _("RO"));
		} 
		else 
		{
			tab_name = g_strdup_printf ("%s", docname);
		}
#endif
		tab_name = g_strdup (docname);

	}
	
	g_free (docname);

	return tab_name;
}

gchar *
_gedit_tab_get_tooltips	(GeditTab *tab)
{
	GeditDocument *doc;
	gchar *tip;
	const gchar *uri;
	gchar *ruri;
	const gchar *mime_type;
	const gchar *mime_description = NULL;
	gchar *mime_full_description; 
	gchar *encoding;
	const GeditEncoding *enc;
	
	g_return_val_if_fail (GEDIT_IS_TAB (tab), NULL);

	doc = gedit_tab_get_document (tab);
	
	uri = gedit_document_get_uri_for_display (doc);
	g_return_val_if_fail (uri != NULL, NULL);

	ruri = 	gedit_utils_replace_home_dir_with_tilde (uri);
	
	switch (tab->priv->state)
	{
		case GEDIT_TAB_STATE_LOADING_ERROR:
			tip =  g_markup_printf_escaped(_("Error opening file <i>%s</i>."),
						       ruri);
			break;
		case GEDIT_TAB_STATE_REVERTING_ERROR:
			tip =  g_markup_printf_escaped(_("Error reverting file <i>%s</i>."),
						       ruri);
			break;			
		case GEDIT_TAB_STATE_SAVING_ERROR:
			tip =  g_markup_printf_escaped(_("Error saving file <i>%s</i>."),
						       ruri);
			break;
		default:
			mime_type = gedit_document_get_mime_type (doc);
			mime_description = gnome_vfs_mime_get_description (mime_type);

			if (mime_description == NULL)
				mime_full_description = g_strdup (mime_type);
			else
				mime_full_description = g_strdup_printf ("%s (%s)", 
						mime_description, mime_type);

			enc = gedit_document_get_encoding (doc);

			if (enc == NULL)
				encoding = g_strdup (_("Unicode (UTF-8)"));
			else
				encoding = gedit_encoding_to_string (enc);

			tip =  g_markup_printf_escaped("<b>%s</b> %s\n\n"
						       "<b>%s</b> %s\n"
						       "<b>%s</b> %s",
						       _("Name:"), ruri,
						       _("MIME Type:"), mime_full_description,
						       _("Encoding:"), encoding);

			g_free (encoding);
			g_free (mime_full_description);
			
			break;
	}
	
	g_free (ruri);	
	
	return tip;
}

static GdkPixbuf *
resize_icon (GdkPixbuf *pixbuf,
	     gint       size)
{
	guint width, height;
	
	width = gdk_pixbuf_get_width (pixbuf); 
	height = gdk_pixbuf_get_height (pixbuf);
	/* if the icon is larger than the nominal size, scale down */
	if (MAX (width, height) > size) 
	{
		GdkPixbuf *scaled_pixbuf;
		
		if (width > height) 
		{
			height = height * size / width;
			width = size;
		} 
		else 
		{
			width = width * size / height;
			height = size;
		}
		
		scaled_pixbuf = gdk_pixbuf_scale_simple	(pixbuf, 
							 width, 
							 height, 
							 GDK_INTERP_BILINEAR);
		g_object_unref (pixbuf);
		pixbuf = scaled_pixbuf;
	}
	
	return pixbuf;
}

static GdkPixbuf *
get_icon (GtkIconTheme *theme, 
	  const gchar  *uri,
	  const gchar  *mime_type, 
	  gint          size)
{
	gchar *icon;
	GdkPixbuf *pixbuf;
	
	icon = gnome_icon_lookup (theme, NULL, uri, NULL, NULL,
				  mime_type, 0, NULL);
	

	g_return_val_if_fail (icon != NULL, NULL);

	pixbuf = gtk_icon_theme_load_icon (theme, icon, size, 0, NULL);
	g_free (icon);
	if (pixbuf == NULL)
		return NULL;
		
	return resize_icon (pixbuf, size);
}

static GdkPixbuf *
get_stock_icon (GtkIconTheme *theme, 
		const gchar  *stock,
		gint          size)
{
	GdkPixbuf *pixbuf;
	
	pixbuf = gtk_icon_theme_load_icon (theme, stock, size, 0, NULL);
	if (pixbuf == NULL)
		return NULL;
		
	return resize_icon (pixbuf, size);
}

/* FIXME: add support for theme changed. I think it should be as easy as
   call g_object_notify (tab, "name") when the icon theme changes */
GdkPixbuf *
_gedit_tab_get_icon (GeditTab *tab)
{
	GdkPixbuf *pixbuf;
	GtkIconTheme *theme;
	GdkScreen *screen;
	gint icon_size;
	
	g_return_val_if_fail (GEDIT_IS_TAB (tab), NULL);

	screen = gtk_widget_get_screen (GTK_WIDGET (tab));
		
	/* CHECK: should we unref the theme? It is not clear from the documentation */
	theme = gtk_icon_theme_get_for_screen (screen);
	g_return_val_if_fail (theme != NULL, NULL);

	gtk_icon_size_lookup_for_settings (gtk_widget_get_settings (GTK_WIDGET (tab)),
					   GTK_ICON_SIZE_MENU, 
					   NULL,
					   &icon_size);

	switch (tab->priv->state)
	{
		case GEDIT_TAB_STATE_LOADING:
			pixbuf = get_stock_icon (theme, 
						 GTK_STOCK_OPEN, 
						 icon_size);
			break;
		case GEDIT_TAB_STATE_REVERTING:
			pixbuf = get_stock_icon (theme, 
						 GTK_STOCK_REVERT_TO_SAVED, 
						 icon_size);						 
			break;
		case GEDIT_TAB_STATE_SAVING:
			pixbuf = get_stock_icon (theme, 
						 GTK_STOCK_SAVE, 
						 icon_size);
			break;
		case GEDIT_TAB_STATE_PRINTING:
			pixbuf = get_stock_icon (theme, 
						 GTK_STOCK_PRINT, 
						 icon_size);
			break;
		case GEDIT_TAB_STATE_PRINT_PREVIEWING:
		case GEDIT_TAB_STATE_SHOWING_PRINT_PREVIEW:		
			pixbuf = get_stock_icon (theme, 
						 GTK_STOCK_PRINT_PREVIEW, 
						 icon_size);
			break;

		case GEDIT_TAB_STATE_LOADING_ERROR:
		case GEDIT_TAB_STATE_REVERTING_ERROR:		
		case GEDIT_TAB_STATE_SAVING_ERROR:
		case GEDIT_TAB_STATE_GENERIC_ERROR:
			pixbuf = get_stock_icon (theme, 
						 GTK_STOCK_DIALOG_ERROR, 
						 icon_size);
			break;
		default:
		{	
			const gchar *raw_uri;
			const gchar *mime_type;
			GeditDocument *doc;
		
			doc = gedit_tab_get_document (tab);
			
			raw_uri = gedit_document_get_uri_ (doc);
			mime_type = gedit_document_get_mime_type (doc);

			pixbuf = get_icon (theme, raw_uri, mime_type, icon_size);
		}
	}
	
	return pixbuf;	
}

GeditTab *
gedit_tab_get_from_document (GeditDocument *doc)
{
	gpointer res;
	
	g_return_val_if_fail (GEDIT_IS_DOCUMENT (doc), NULL);
	
	res = g_object_get_data (G_OBJECT (doc), GEDIT_TAB_KEY);
	
	return (res != NULL) ? GEDIT_TAB (res) : NULL;
}

gboolean
_gedit_tab_load (GeditTab            *tab,
		 const gchar         *uri,
		 const GeditEncoding *encoding,
		 gint                 line_pos,
		 gboolean             create)
{
	GeditDocument *doc;

	g_return_val_if_fail (GEDIT_IS_TAB (tab), FALSE);
	g_return_val_if_fail (tab->priv->state == GEDIT_TAB_STATE_NORMAL, FALSE);

	doc = gedit_tab_get_document (tab);
	g_return_val_if_fail (GEDIT_IS_DOCUMENT (doc), FALSE);

	gedit_tab_set_state (tab, GEDIT_TAB_STATE_LOADING);

	return gedit_document_load (doc,
				    uri,
				    encoding,
				    line_pos,
				    create);
}

void
_gedit_tab_revert (GeditTab *tab)
{
	GeditDocument *doc;

	g_return_if_fail (GEDIT_IS_TAB (tab));
	g_return_if_fail (tab->priv->state == GEDIT_TAB_STATE_NORMAL);

	doc = gedit_tab_get_document (tab);
	g_return_if_fail (GEDIT_IS_DOCUMENT (doc));

	gedit_tab_set_state (tab, GEDIT_TAB_STATE_REVERTING);

	gedit_document_load (doc,
			     gedit_document_get_uri_ (doc),
			     gedit_document_get_encoding (doc),
			     0,
			     FALSE);
}

void
_gedit_tab_save (GeditTab *tab)
{
	GeditDocument *doc;

	g_return_if_fail (GEDIT_IS_TAB (tab));
	g_return_if_fail ((tab->priv->state == GEDIT_TAB_STATE_NORMAL) ||
			  (tab->priv->state == GEDIT_TAB_STATE_SHOWING_PRINT_PREVIEW));

	doc = gedit_tab_get_document (tab);
	g_return_if_fail (GEDIT_IS_DOCUMENT (doc));
	g_return_if_fail (!gedit_document_is_untitled (doc));

	gedit_tab_set_state (tab, GEDIT_TAB_STATE_SAVING);

	/* uri used in error messages... strdup because errors are async
	 * and the string can go away, will be freed in documnt_loaded */
	tab->priv->save_uri = g_strdup (gedit_document_get_uri_ (doc));

	gedit_document_save (doc);
}

void
_gedit_tab_save_as (GeditTab            *tab,
		    const gchar         *uri,
		    const GeditEncoding *encoding)
{
	GeditDocument *doc;

	g_return_if_fail (GEDIT_IS_TAB (tab));
	g_return_if_fail ((tab->priv->state == GEDIT_TAB_STATE_NORMAL) ||
			  (tab->priv->state == GEDIT_TAB_STATE_SHOWING_PRINT_PREVIEW));


	doc = gedit_tab_get_document (tab);
	g_return_if_fail (GEDIT_IS_DOCUMENT (doc));

	gedit_tab_set_state (tab, GEDIT_TAB_STATE_SAVING);

	/* uri used in error messages... strdup because errors are async
	 * and the string can go away, will be freed in documnt_loaded */
	tab->priv->save_uri = g_strdup (uri);

	gedit_document_save_as (doc, uri, encoding);
}

static void
print_preview_destroyed (GtkWidget *preview,
			 GeditTab  *tab)
{
	tab->priv->print_preview = NULL;
	
	if (tab->priv->state == GEDIT_TAB_STATE_SHOWING_PRINT_PREVIEW)
		gedit_tab_set_state (tab, GEDIT_TAB_STATE_NORMAL);
	else
		gtk_widget_show (tab->priv->view_scrolled_window);		
}

static void
set_print_preview (GeditTab  *tab, GtkWidget *print_preview)
{
	if (tab->priv->print_preview == print_preview)
		return;
		
	if (tab->priv->print_preview != NULL)
		gtk_widget_destroy (tab->priv->print_preview);
		
	
	tab->priv->print_preview = print_preview;

	gtk_box_pack_end (GTK_BOX (tab),
			  tab->priv->print_preview,
			  TRUE,
			  TRUE,
			  0);		

	gtk_widget_grab_focus (tab->priv->print_preview);

	g_signal_connect (tab->priv->print_preview,
			  "destroy",
			  G_CALLBACK (print_preview_destroyed),
			  tab);
}

#define MIN_PAGES 15

static void
print_page_cb (GtkSourcePrintJob *pjob, GeditTab *tab)
{
	gchar *str;
	gint page_num;
	gint total;

	g_return_if_fail (GEDIT_IS_PROGRESS_MESSAGE_AREA (tab->priv->message_area));	

	total = gtk_source_print_job_get_page_count (pjob);
	
	if (total < MIN_PAGES)
		return;

	page_num = gtk_source_print_job_get_page (pjob);
			
	g_return_if_fail (GEDIT_IS_PROGRESS_MESSAGE_AREA (tab->priv->message_area));	
	
	gtk_widget_show (tab->priv->message_area);
	
	str = g_strdup_printf (_("Rendering page %d of %d..."), page_num, total);
	/* sleep (1); */
	gedit_progress_message_area_set_markup (GEDIT_PROGRESS_MESSAGE_AREA (tab->priv->message_area),
						str);
	g_free (str);
	
	gedit_progress_message_area_set_fraction (GEDIT_PROGRESS_MESSAGE_AREA (tab->priv->message_area),
				       1.0 * page_num / total);
}

static void
preview_finished_cb (GtkSourcePrintJob *pjob, GeditTab *tab)
{
	GnomePrintJob *gjob;
	GtkWidget *preview = NULL;

	g_return_if_fail (GEDIT_IS_PROGRESS_MESSAGE_AREA (tab->priv->message_area));
	set_message_area (tab, NULL); /* destroy the message area */
	
	gjob = gtk_source_print_job_get_print_job (pjob);

	preview = gedit_print_job_preview_new (gjob);	
 	g_object_unref (gjob);
	
	set_print_preview (tab, preview);
	
	gtk_widget_show (preview);
	g_object_unref (pjob);
	
	gedit_tab_set_state (tab, GEDIT_TAB_STATE_SHOWING_PRINT_PREVIEW);
}

static void
print_finished_cb (GtkSourcePrintJob *pjob, GeditTab *tab)
{
	GnomePrintJob *gjob;

	g_return_if_fail (GEDIT_IS_PROGRESS_MESSAGE_AREA (tab->priv->message_area));
	set_message_area (tab, NULL); /* destroy the message area */
	
	gjob = gtk_source_print_job_get_print_job (pjob);

	gnome_print_job_print (gjob);
 	g_object_unref (gjob);

	gedit_print_job_save_config (GEDIT_PRINT_JOB (pjob));
	
	g_object_unref (pjob);
	
	gedit_tab_set_state (tab, GEDIT_TAB_STATE_NORMAL);
}

static void
print_cancelled (GeditMessageArea *area,
                 gint              response_id,
                 GeditTab         *tab)
{
	g_return_if_fail (GEDIT_IS_PROGRESS_MESSAGE_AREA (tab->priv->message_area));
	
	gtk_source_print_job_cancel (GTK_SOURCE_PRINT_JOB (tab->priv->print_job));
	g_object_unref (tab->priv->print_job);

	set_message_area (tab, NULL); /* destroy the message area */
	
	gedit_tab_set_state (tab, GEDIT_TAB_STATE_NORMAL);
}

static void
show_printing_message_area (GeditTab      *tab,
			    gboolean       preview)
{
	GtkWidget *area;
	
	if (preview)
		area = gedit_progress_message_area_new (GTK_STOCK_PRINT_PREVIEW,
							"",
							TRUE);
	else
		area = gedit_progress_message_area_new (GTK_STOCK_PRINT,
							"",
							TRUE);
	

	g_signal_connect (area,
			  "response",
			  G_CALLBACK (print_cancelled),
			  tab);
	  
	set_message_area (tab, area);
}

void 
_gedit_tab_print (GeditTab      *tab,
		  GeditPrintJob *pjob,
		  GtkTextIter   *start, 
		  GtkTextIter   *end)
{
	GeditDocument *doc;
	
	g_return_if_fail (GEDIT_IS_TAB (tab));
	g_return_if_fail (GEDIT_IS_PRINT_JOB (pjob));
	g_return_if_fail (start != NULL);
	g_return_if_fail (end != NULL);	
		
	doc = GEDIT_DOCUMENT (gtk_source_print_job_get_buffer (GTK_SOURCE_PRINT_JOB (pjob)));
	g_return_if_fail (doc != NULL);
	g_return_if_fail (gedit_tab_get_document (tab) == doc);
	g_return_if_fail (gtk_text_iter_get_buffer (start) == GTK_TEXT_BUFFER (doc));
	g_return_if_fail (gtk_text_iter_get_buffer (end) == GTK_TEXT_BUFFER (doc));	
	
	g_return_if_fail (tab->priv->print_job == NULL);
	g_return_if_fail ((tab->priv->state == GEDIT_TAB_STATE_NORMAL) ||
			  (tab->priv->state == GEDIT_TAB_STATE_SHOWING_PRINT_PREVIEW));
	
	g_object_ref (pjob);
	tab->priv->print_job = pjob;
	g_object_add_weak_pointer (G_OBJECT (pjob), 
				   (gpointer *) &tab->priv->print_job);
	
	show_printing_message_area (tab, FALSE);

	g_signal_connect (pjob, "begin_page", (GCallback) print_page_cb, tab);
	g_signal_connect (pjob, "finished", (GCallback) print_finished_cb, tab);

	gedit_tab_set_state (tab, GEDIT_TAB_STATE_PRINTING);
	
	if (!gtk_source_print_job_print_range_async (GTK_SOURCE_PRINT_JOB (pjob), start, end))
	{
		/* FIXME: go in error state */
		gtk_text_view_set_editable (GTK_TEXT_VIEW (tab->priv->view), 
					    !tab->priv->not_editable);
		g_warning ("Async print preview failed");
		g_object_unref (pjob);
	}
}

void
_gedit_tab_print_preview (GeditTab      *tab,
			  GeditPrintJob *pjob,
			  GtkTextIter   *start, 
			  GtkTextIter   *end)		  
{
	GeditDocument *doc;
	
	g_return_if_fail (GEDIT_IS_TAB (tab));
	g_return_if_fail (GEDIT_IS_PRINT_JOB (pjob));
	g_return_if_fail (start != NULL);
	g_return_if_fail (end != NULL);	
		
	doc = GEDIT_DOCUMENT (gtk_source_print_job_get_buffer (GTK_SOURCE_PRINT_JOB (pjob)));
	g_return_if_fail (doc != NULL);
	g_return_if_fail (gedit_tab_get_document (tab) == doc);
	g_return_if_fail (gtk_text_iter_get_buffer (start) == GTK_TEXT_BUFFER (doc));
	g_return_if_fail (gtk_text_iter_get_buffer (end) == GTK_TEXT_BUFFER (doc));	
	
	g_return_if_fail (tab->priv->print_job == NULL);
	g_return_if_fail (tab->priv->state == GEDIT_TAB_STATE_NORMAL);
	
	g_object_ref (pjob);
	tab->priv->print_job = pjob;
	g_object_add_weak_pointer (G_OBJECT (pjob), 
				   (gpointer *) &tab->priv->print_job);
	
	show_printing_message_area (tab, TRUE);

	g_signal_connect (pjob, "begin_page", (GCallback) print_page_cb, tab);
	g_signal_connect (pjob, "finished", (GCallback) preview_finished_cb, tab);

	gedit_tab_set_state (tab, GEDIT_TAB_STATE_PRINT_PREVIEWING);
	
	if (!gtk_source_print_job_print_range_async (GTK_SOURCE_PRINT_JOB (pjob), start, end))
	{
		/* FIXME: go in error state */
		gedit_tab_set_state (tab, GEDIT_TAB_STATE_NORMAL);
		g_warning ("Async print preview failed");
		g_object_unref (pjob);
	}
}
