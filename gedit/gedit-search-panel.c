/*
 * gedit-search-panel.c
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

#include <stdlib.h>
#include <string.h>

#include "gedit-search-panel.h"
#include "gedit-utils.h"
#include "gedit-window.h"
#include "gedit-debug.h"
#include "gedit-statusbar.h"

#include <glib/gi18n.h>
#include <glade/glade-xml.h>

#define GEDIT_SEARCH_PANEL_GET_PRIVATE(object)(G_TYPE_INSTANCE_GET_PRIVATE ((object), GEDIT_TYPE_SEARCH_PANEL, GeditSearchPanelPrivate))

#define MIN_KEY_LEN 3

struct _GeditSearchPanelPrivate
{
	GeditWindow  *window;
	
	GtkWidget    *replace_expander;
	GtkWidget    *goto_line_expander;
	GtkWidget    *search_options_expander;
	
	GtkWidget    *search_entry;
	GtkWidget    *replace_entry;
	GtkWidget    *line_number_entry;
	
	GtkWidget    *find_button;
	GtkWidget    *replace_button;
	GtkWidget    *replace_all_button;
	
	GtkWidget    *search_options_vbox;
	
	GtkWidget    *match_case_checkbutton;
	GtkWidget    *entire_word_checkbutton;
	GtkWidget    *search_backwards_checkbutton;
	GtkWidget    *wrap_around_checkbutton;

	guint         message_cid;	
	
	gboolean      new_search_text : 1;
};

G_DEFINE_TYPE(GeditSearchPanel, gedit_search_panel, GTK_TYPE_VBOX)

enum
{
	PROP_0,
	PROP_WINDOW,
};

static gboolean
get_selected_text (GtkTextBuffer *doc, gchar **selected_text, gint *len)
{
	GtkTextIter start, end;

	g_return_val_if_fail (selected_text != NULL, FALSE);
	g_return_val_if_fail (*selected_text == NULL, FALSE);

	if (!gtk_text_buffer_get_selection_bounds (doc, &start, &end))
	{
		if (len != NULL)
			len = 0;

		return FALSE;
	}

	*selected_text = gtk_text_buffer_get_slice (doc, &start, &end, TRUE);

	if (len != NULL)
		*len = g_utf8_strlen (*selected_text, -1);

	return TRUE;
}

static void
update_buttons_sensitivity (GeditSearchPanel *panel)
{
	const gchar *replace_text;
	const gchar *search_text;

	search_text = gtk_entry_get_text (GTK_ENTRY (panel->priv->search_entry));
	replace_text = gtk_entry_get_text (GTK_ENTRY (panel->priv->replace_entry));

	gtk_widget_set_sensitive (panel->priv->find_button, 
				  (*search_text != '\0'));	
	gtk_widget_set_sensitive (panel->priv->replace_button,
				  (*replace_text != '\0') && (*search_text != '\0'));
	gtk_widget_set_sensitive (panel->priv->replace_all_button, 
				  (*replace_text != '\0') && (*search_text != '\0'));
}

static void
window_tab_removed (GeditWindow      *window,
		    GeditTab         *tab,
		    GeditSearchPanel *panel)
{
	gedit_debug_message (DEBUG_SEARCH, "Window: %p - Tab: %p", window, tab);

	if (gedit_window_get_active_tab (window) == NULL)
	{
		gtk_widget_set_sensitive (panel->priv->search_entry,
					  FALSE);
		gtk_widget_set_sensitive (panel->priv->replace_entry,
					  FALSE);			  
		gtk_widget_set_sensitive (panel->priv->line_number_entry,
					  FALSE);	
		gtk_widget_set_sensitive (panel->priv->find_button,
					  FALSE);
		gtk_widget_set_sensitive (panel->priv->replace_button,
					  FALSE);			  
		gtk_widget_set_sensitive (panel->priv->replace_all_button,
					  FALSE);
		gtk_widget_set_sensitive (panel->priv->search_options_vbox,
					  FALSE);
	}
}

static void
window_tab_added (GeditWindow      *window,
		  GeditTab         *tab,
		  GeditSearchPanel *panel)
{
	GeditTabState  state;
	gboolean state_normal;
	
	gedit_debug_message (DEBUG_SEARCH, "Window: %p - Tab: %p", window, tab);
	
	state = gedit_tab_get_state (tab);
		
	state_normal = (state == GEDIT_TAB_STATE_NORMAL);
	
	gtk_widget_set_sensitive (panel->priv->search_entry,
				  state_normal);
	gtk_widget_set_sensitive (panel->priv->replace_entry,
				  state_normal);
	gtk_widget_set_sensitive (panel->priv->line_number_entry,
				  state_normal);	
	gtk_widget_set_sensitive (panel->priv->search_options_vbox,
				  state_normal);

	if (state_normal)
		update_buttons_sensitivity (panel);
	else
	{
		gtk_widget_set_sensitive (panel->priv->find_button,
					  FALSE);
		gtk_widget_set_sensitive (panel->priv->replace_button,
					  FALSE);			  
		gtk_widget_set_sensitive (panel->priv->replace_all_button,
					  FALSE);
	}
}

static void
window_active_tab_changed (GeditWindow      *window,
		  	   GeditTab         *tab,
		  	   GeditSearchPanel *panel)
{
	gedit_debug_message (DEBUG_SEARCH, "Window: %p - Tab: %p", window, tab);
	
	g_return_if_fail (tab != NULL);
	
	window_tab_added (window, tab, panel);
}

static void
window_active_tab_state_changed (GeditWindow      *window,
		  	   	 GeditSearchPanel *panel)
{
	gedit_debug_message (DEBUG_SEARCH, "Window: %p", window);
	
	window_tab_added (window, 
			  gedit_window_get_active_tab (window), 
			  panel);
}

static void
set_window (GeditSearchPanel *panel,
	    GeditWindow      *window)
{
	g_return_if_fail (panel->priv->window == NULL);
	g_return_if_fail (GEDIT_IS_WINDOW (window));
	
	panel->priv->window = window;

	panel->priv->message_cid = gtk_statusbar_get_context_id
					(GTK_STATUSBAR (gedit_window_get_statusbar (window)), 
					 "search_replace_message");
	
	g_signal_connect (window,
			  "tab_added",
			  G_CALLBACK (window_tab_added),
			  panel);
	g_signal_connect (window,
			  "tab_removed",
			  G_CALLBACK (window_tab_removed),
			  panel);
	g_signal_connect (window,
			  "active_tab_changed",
			  G_CALLBACK (window_active_tab_changed),
			  panel);
	g_signal_connect (window,
			  "active_tab_state_changed",
			  G_CALLBACK (window_active_tab_state_changed),
			  panel);			  			  
}


static void
gedit_search_panel_set_property (GObject      *object,
		        	 guint         prop_id,
		        	 const GValue *value,
		        	 GParamSpec   *pspec)
{
	GeditSearchPanel *panel = GEDIT_SEARCH_PANEL (object);

	switch (prop_id)
	{
		case PROP_WINDOW:
			set_window (panel, g_value_get_object (value));
			break;

		default:
			G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
			break;
	}
}

static void
gedit_search_panel_get_property (GObject    *object,
				 guint       prop_id,
				 GValue     *value,
				 GParamSpec *pspec)
{
	GeditSearchPanel *panel = GEDIT_SEARCH_PANEL (object);

	switch (prop_id)
	{
		case PROP_WINDOW:
			g_value_set_object (value,
					    GEDIT_SEARCH_PANEL_GET_PRIVATE (panel)->window);
			break;
		default:
			G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
			break;			
	}
}

static void
gedit_search_panel_finalize (GObject *object)
{
	/* GeditSearchPanel *tab = GEDIT_SEARCH_PANEL (object); */
	
	// TODO: disconnect signal with window

	G_OBJECT_CLASS (gedit_search_panel_parent_class)->finalize (object);
}

static void 
gedit_search_panel_class_init (GeditSearchPanelClass *klass)
{
	GObjectClass *object_class = G_OBJECT_CLASS (klass);

	object_class->finalize = gedit_search_panel_finalize;
	object_class->get_property = gedit_search_panel_get_property;
	object_class->set_property = gedit_search_panel_set_property;
	
	g_object_class_install_property (object_class,
					 PROP_WINDOW,
					 g_param_spec_object ("window",
							 "Window",
							 "The GeditWindow this GeditSearchPanel is associated with",
							 GEDIT_TYPE_WINDOW,
							 G_PARAM_READWRITE |
							 G_PARAM_CONSTRUCT_ONLY));	
							      
	g_type_class_add_private (object_class, sizeof(GeditSearchPanelPrivate));
}

static void
line_number_entry_insert_text (GtkEditable *editable, 
			       const char  *text, 
			       gint         length, 
			       gint        *position)
{
	gunichar c;
	const gchar *p;
 	const gchar *end;

	p = text;
	end = text + length;

	while (p != end) {
		const gchar *next;
		next = g_utf8_next_char (p);

		c = g_utf8_get_char (p);

		if (!g_unichar_isdigit (c)) {
			g_signal_stop_emission_by_name (editable, "insert_text");
			break;
		}

		p = next;
	}
}

static void
line_number_entry_changed (GtkEditable      *editable,
			   GeditSearchPanel *panel)
{
	gchar     *line_str;
	GeditView *active_view; 

	active_view = gedit_window_get_active_view (panel->priv->window);
	if (active_view == NULL)
		return;

	line_str = gtk_editable_get_chars (editable, 0, -1);

	if ((line_str != NULL) && (line_str[0] != 0))
	{
		gboolean exceeded;
		GeditDocument *active_document;
		gint line;

		active_document = gedit_window_get_active_document (panel->priv->window);

		line = MAX (atoi (line_str) - 1, 0);
		exceeded = gedit_document_goto_line (active_document, line);
		gedit_view_scroll_to_cursor (active_view);

		gedit_statusbar_flash_message (GEDIT_STATUSBAR (gedit_window_get_statusbar (panel->priv->window)),
					       panel->priv->message_cid,
					       ngettext("The document has less than %d line. Moving to last line.",
					     	        "The document has less than %d lines. Moving to last line.",
					     	        line),
					       line);
	}
	
	g_free (line_str);
}

static void
line_number_entry_activate (GtkEntry         *entry,
			    GeditSearchPanel *panel)
{
	GeditView *active_view; 
	
	active_view = gedit_window_get_active_view (panel->priv->window);	
	if (active_view == NULL)
		return;
			
	gtk_widget_grab_focus (GTK_WIDGET (active_view));
}

/* FIXME: do we need to set a limit on the number of items? */
static void
add_entry_completion_entry (GeditSearchPanel *panel,
			    GeditDocument    *doc,
			    gboolean          replace_entry)
{
	const gchar *text;
	gboolean valid;
	GtkTreeModel *model;
	GtkTreeIter iter;
	
	if (!replace_entry)
		text = gtk_entry_get_text (GTK_ENTRY (panel->priv->search_entry));
	else
		text = gtk_entry_get_text (GTK_ENTRY (panel->priv->replace_entry));
		
	/* g_print ("Text: %s\n", text); */
	
	if (g_utf8_strlen (text, -1) < MIN_KEY_LEN)
		return;
	
	if (!replace_entry)
		model = gtk_entry_completion_get_model (
				gtk_entry_get_completion (GTK_ENTRY (panel->priv->search_entry)));
	else
		model = gtk_entry_completion_get_model (
				gtk_entry_get_completion (GTK_ENTRY (panel->priv->replace_entry)));
				
	g_return_if_fail (model != NULL);		
		
	/* Get the first iter in the list */
	valid = gtk_tree_model_get_iter_first (model, &iter);

	while (valid)
	{
		/* Walk through the list, reading each row */
     		gchar *str_data;
      
		gtk_tree_model_get (model, &iter, 
                          	    0, &str_data,
                          	    -1);

		/* Do something with the data */
		if (strcmp (text, str_data) == 0)
		{
			g_free (str_data);
			gtk_list_store_move_after (GTK_LIST_STORE (model),
						   &iter,
						   NULL);

			return;
		}

		g_free (str_data);

		valid = gtk_tree_model_iter_next (model, &iter);
    	}
    
	/* g_print ("Insert Text: %s\n", text); */
    
	gtk_list_store_prepend (GTK_LIST_STORE (model), &iter);
	gtk_list_store_set (GTK_LIST_STORE (model), 
			    &iter, 
			    0, 
			    text, 
			    -1);
}				   

/* Use occurences only for Replace All */
static void
phrase_found (GeditSearchPanel *panel,
	      gint              occurrences)
{
	if (occurrences > 0)
	{
		gedit_statusbar_flash_message (GEDIT_STATUSBAR (gedit_window_get_statusbar (panel->priv->window)),
					       panel->priv->message_cid,
					       ngettext("Found and replaced %d occurrence.",
					     	        "Found and replaced %d occurrences.",
					     	        occurrences),
					       occurrences);
	}
	else
	{
		gedit_statusbar_flash_message (GEDIT_STATUSBAR (gedit_window_get_statusbar (panel->priv->window)),
					       panel->priv->message_cid, 
					       " ");
	}
				   
	gtk_widget_modify_base (panel->priv->search_entry,
			        GTK_STATE_NORMAL,
			        NULL);
	gtk_widget_modify_text (panel->priv->search_entry,
			        GTK_STATE_NORMAL,
			        NULL);	
}

static void
phrase_not_found (GeditSearchPanel *panel)
{
	GdkColor red;
	GdkColor white;
	
	/* FIXME: a11y and theme */
	
	gedit_statusbar_flash_message (GEDIT_STATUSBAR (gedit_window_get_statusbar (panel->priv->window)),
				       panel->priv->message_cid,
				       _("Phrase not found"));
				
	gdk_color_parse ("#FF6666", &red);
	gdk_color_parse ("white", &white);		

	gtk_widget_modify_base (panel->priv->search_entry,
			        GTK_STATE_NORMAL,
			        &red);
	gtk_widget_modify_text (panel->priv->search_entry,
			        GTK_STATE_NORMAL,
			        &white);			
}

static gboolean
run_search (GeditSearchPanel *panel,
            GeditView        *view,
            const gchar      *entry_text,
	    gboolean          wrap_around,
	    gboolean          search_backwards,
            gboolean          button_pressed)
{
	GtkTextIter start_iter;
	GtkTextIter match_start;
	GtkTextIter match_end;	
	gboolean found = FALSE;

	GeditDocument *doc;

	doc = GEDIT_DOCUMENT (gtk_text_view_get_buffer (GTK_TEXT_VIEW (view)));
	
	if (*entry_text != '\0')
	{	
		if (!search_backwards)
		{
			gtk_text_buffer_get_selection_bounds (GTK_TEXT_BUFFER (doc),
							      &start_iter,
							      &match_end);
		
			if (button_pressed)
				gtk_text_iter_order (&match_end, &start_iter);
		
			/* run search */
			found = gedit_document_search_forward (doc,
							       &start_iter,
							       NULL,
							       &match_start,
							       &match_end);
		}						       
		else if (button_pressed)
		{
			/* backward and button pressed */
		
			gtk_text_buffer_get_selection_bounds (GTK_TEXT_BUFFER (doc),
							      &start_iter,
							      &match_end);
			
			/* run search */
			found = gedit_document_search_backward (doc,
							        NULL,
							        &start_iter,
							        &match_start,
							        &match_end);
		} 
		else
		{
			/* backward (while typing) */
	
			GtkTextIter sel_end;
			gboolean sel;

			sel = gtk_text_buffer_get_selection_bounds (GTK_TEXT_BUFFER (doc),
							            &start_iter,
							            &sel_end);

			if (sel)
			{			
				/* First search forward */
				GtkTextIter end_iter;
						
				end_iter = start_iter;
				/* This is an upper bound */
				gtk_text_iter_forward_chars (&end_iter,
							     g_utf8_strlen (entry_text, -1) + 1);
							     
				match_start = end_iter;
									     
				/* run search */
				found = gedit_document_search_forward (doc,
								       &start_iter,
								       &end_iter,
								       &match_start,
								       &match_end);
					
				/* g_print ("Found: %s\n", found ? "T" : "F"); */
			}
									       
			if (!sel ||
			    !found ||
			    !gtk_text_iter_equal (&start_iter, &match_start))
			{
				/* Really search backward */
				start_iter = sel_end;
				
				found = gedit_document_search_backward (doc,
							        NULL,
							        &start_iter,
							        &match_start,
							        &match_end);
			}
		}
		/* g_print ("Found: %s\n", found ? "TRUE" : "FALSE"); */
		
		if (!found && wrap_around)
		{
			if (!search_backwards)
				found = gedit_document_search_forward (doc,
								       NULL,
								       NULL, /* FIXME: set the end_inter */
								       &match_start,
								       &match_end);
			else
				found = gedit_document_search_backward (doc,
								        NULL, /* FIXME: set the start_inter */
								        NULL, 
								        &match_start,
								        &match_end);
		}
	}
	else
	{
		gtk_text_buffer_get_selection_bounds (GTK_TEXT_BUFFER (doc),
						      &start_iter, 
						      NULL);	
	}	
	
	if (found)
	{
		gtk_text_buffer_place_cursor (GTK_TEXT_BUFFER (doc),
					&match_start);

		gtk_text_buffer_move_mark_by_name (GTK_TEXT_BUFFER (doc),
					"selection_bound", &match_end);
	}
	else
		gtk_text_buffer_place_cursor (GTK_TEXT_BUFFER (doc),
					      &start_iter);
						      
	if (found || (*entry_text == '\0'))
	{				   
		gedit_view_scroll_to_cursor (view);

		phrase_found (panel, 0); 
	}
	else
	{
		phrase_not_found (panel);		        
	}
		
	if (found && panel->priv->new_search_text && button_pressed)
	{
		add_entry_completion_entry (panel, doc, FALSE);
		panel->priv->new_search_text = FALSE;
	}
	
	return found;
}

static gboolean
search (GeditSearchPanel *panel, gboolean button_pressed)
{
	GeditView *active_view; 
	GeditDocument *doc;
	gchar *search_text;
	const gchar *entry_text;
	gboolean case_sensitive;
	gboolean entire_word;
	gboolean wrap_around;
	gboolean search_backwards;	
	gint flags = 0;
	gint old_flags = 0;
	
	active_view = gedit_window_get_active_view (panel->priv->window);
	if (active_view == NULL)
		return FALSE;
	
	doc = GEDIT_DOCUMENT (gtk_text_view_get_buffer (GTK_TEXT_VIEW (active_view)));
	
	entry_text = gtk_entry_get_text (GTK_ENTRY (panel->priv->search_entry));
	
	/* retrieve search settings from the toggle buttons */
	case_sensitive = gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (panel->priv->match_case_checkbutton));
	entire_word = gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (panel->priv->entire_word_checkbutton));
	wrap_around = gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (panel->priv->wrap_around_checkbutton));
	search_backwards = gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (panel->priv->search_backwards_checkbutton));
	
	GEDIT_SEARCH_SET_CASE_SENSITIVE (flags, case_sensitive);
	GEDIT_SEARCH_SET_ENTIRE_WORD (flags, entire_word);
	
	search_text = gedit_document_get_search_text (doc, &old_flags);
	
	if ((search_text == NULL) || (strcmp (search_text, entry_text) != 0))
	{
		panel->priv->new_search_text = TRUE;
		gedit_document_set_search_text (doc, entry_text, flags);
	}
	else if (flags != old_flags)
	{
		gedit_document_set_search_text (doc, entry_text, flags);
	}
	
	g_free (search_text);
		
	return run_search (panel, 
			   active_view,
			   entry_text,
			   wrap_around,
			   search_backwards,
			   button_pressed);
}

static void
replace_selected_text (GtkTextBuffer *buffer, const gchar *replace)
{
	g_return_if_fail (gtk_text_buffer_get_selection_bounds (buffer, NULL, NULL));
	g_return_if_fail (replace != NULL);
	
	gtk_text_buffer_begin_user_action (buffer);

	gtk_text_buffer_delete_selection (buffer, FALSE, TRUE);

	gtk_text_buffer_insert_at_cursor (buffer, replace, strlen (replace));

	gtk_text_buffer_end_user_action (buffer);
}

static void
replace (GeditSearchPanel *panel)
{
	GeditDocument *doc;
	const gchar *search_entry_text;
	const gchar *replace_entry_text;	
	gchar *unescaped_search_text;
	gchar *unescaped_replace_text;
	gchar *selected_text = NULL;
	gboolean case_sensitive;
	gboolean search_backwards;
	
	doc = gedit_window_get_active_document (panel->priv->window);
	if (doc == NULL)
		return;
			
	replace_entry_text = gtk_entry_get_text (GTK_ENTRY (panel->priv->replace_entry));
	g_return_if_fail ((*replace_entry_text) != '\0');
	search_entry_text = gtk_entry_get_text (GTK_ENTRY (panel->priv->search_entry));
	g_return_if_fail ((*search_entry_text) != '\0');
		
	unescaped_search_text = gedit_utils_unescape_search_text (search_entry_text);
	
	get_selected_text (GTK_TEXT_BUFFER (doc), 
			   &selected_text, 
			   NULL);
	
	/* retrieve search settings from the toggle buttons */
	case_sensitive = gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (panel->priv->match_case_checkbutton));	
	search_backwards = gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (panel->priv->search_backwards_checkbutton));
	
	if ((selected_text == NULL) ||
	    (case_sensitive && (strcmp (selected_text, unescaped_search_text) != 0)) || 
	    (!case_sensitive && !g_utf8_caselessnmatch (selected_text, unescaped_search_text, 
						        strlen (selected_text), 
						        strlen (unescaped_search_text)) != 0))
	{
		search (panel, search_backwards);
		g_free (unescaped_search_text);
		g_free (selected_text);	
		
		return;
	}

	unescaped_replace_text = gedit_utils_unescape_search_text (replace_entry_text);	
	replace_selected_text (GTK_TEXT_BUFFER (doc), unescaped_replace_text);

	add_entry_completion_entry (panel, doc, TRUE);
	
	g_free (unescaped_search_text);
	g_free (selected_text);
	g_free (unescaped_replace_text);
	
	search (panel, search_backwards);
}

static void
replace_all (GeditSearchPanel *panel)
{
	GeditDocument *doc;
	const gchar *search_entry_text;
	const gchar *replace_entry_text;	
	gboolean case_sensitive;
	gboolean entire_word;
	gint flags = 0;
	gint cont;
	
	doc = gedit_window_get_active_document (panel->priv->window);
	if (doc == NULL)
		return;
			
	replace_entry_text = gtk_entry_get_text (GTK_ENTRY (panel->priv->replace_entry));
	g_return_if_fail ((*replace_entry_text) != '\0');
	search_entry_text = gtk_entry_get_text (GTK_ENTRY (panel->priv->search_entry));
	g_return_if_fail ((*search_entry_text) != '\0');
		
	/* retrieve search settings from the toggle buttons */
	case_sensitive = gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (panel->priv->match_case_checkbutton));
	entire_word = gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (panel->priv->entire_word_checkbutton));
	
	GEDIT_SEARCH_SET_CASE_SENSITIVE (flags, case_sensitive);
	GEDIT_SEARCH_SET_ENTIRE_WORD (flags, entire_word);
	
	cont = gedit_document_replace_all (doc, 
					   search_entry_text,
					   replace_entry_text,
					   flags);
					   
	if (cont > 0)
	{
		add_entry_completion_entry (panel, doc, TRUE);
		phrase_found (panel, cont);
	}
	else
		phrase_not_found (panel);
}

static void
find_button_clicked (GtkButton        *widget,
		     GeditSearchPanel *panel)
{
	search (panel, TRUE);
}

static void
replace_button_clicked (GtkButton        *widget,
			GeditSearchPanel *panel)
{
	replace (panel);
}

static void
replace_all_button_clicked (GtkButton        *widget,
			    GeditSearchPanel *panel)
{
	replace_all (panel);
}
		       
static void
search_entry_changed (GtkEditable      *editable,
		      GeditSearchPanel *panel)
{
	search (panel, FALSE);
	update_buttons_sensitivity (panel);
}

static void
search_entry_activate (GtkEntry         *entry,
		       GeditSearchPanel *panel)
{
	if (GTK_WIDGET_SENSITIVE (panel->priv->find_button))
		gtk_widget_activate (panel->priv->find_button);
}

static void
replace_entry_changed (GtkEditable      *editable,
		       GeditSearchPanel *panel)
{
	update_buttons_sensitivity (panel);
}

static void
replace_entry_activate (GtkEntry         *entry,
		        GeditSearchPanel *panel)
{
	if (GTK_WIDGET_SENSITIVE (panel->priv->replace_button))
		gtk_widget_activate (panel->priv->replace_button);		
}

static void
option_button_toggled (GtkToggleButton  *togglebutton,
		       GeditSearchPanel *panel)
{
	gtk_widget_set_sensitive (panel->priv->find_button, TRUE);
}

static void
search_entry_insert_text (GtkEditable *editable, 
			  const gchar *text, 
			  gint         length, 
			  gint        *position)
{
	static gboolean insert_text = FALSE;
	gchar *escaped_text;
	gint new_len;

	gedit_debug_message (DEBUG_SEARCH, "Text: %s", text);

	/* To avoid recursive behavior */
	if (insert_text)
		return;

	escaped_text = gedit_utils_escape_search_text (text);

	gedit_debug_message (DEBUG_SEARCH, "Escaped Text: %s", escaped_text);

	new_len = strlen (escaped_text);

	if (new_len == length)
	{
		g_free (escaped_text);
		return;
	}

	insert_text = TRUE;

	g_signal_stop_emission_by_name (editable, "insert_text");
	
	gtk_editable_insert_text (editable, escaped_text, new_len, position);

	insert_text = FALSE;

	g_free (escaped_text);
}

static void
gedit_search_panel_init (GeditSearchPanel *panel)
{
	GladeXML *gui;
	GtkWidget *find_vbox;
	GtkWidget *search_panel_vbox;
	GtkListStore *store;
	GtkEntryCompletion *completion;

	panel->priv = GEDIT_SEARCH_PANEL_GET_PRIVATE (panel);

	panel->priv->new_search_text = FALSE;

	gui = glade_xml_new (GEDIT_GLADEDIR "search-panel.glade",
			     "search_panel_vbox", NULL);
	if (!gui)
	{
		gchar *msg;
		GtkWidget *label;		
		msg = g_strdup_printf (MISSING_FILE,
			       GEDIT_GLADEDIR "search-panel.glade");

		g_warning (msg);

		label = gtk_label_new (msg);

		gtk_box_pack_start (GTK_BOX (panel), 
				    label, 
				    TRUE, 
				    TRUE, 
				    0);
		    
		g_free (msg);

		return ;
	}

	search_panel_vbox = glade_xml_get_widget (gui, "search_panel_vbox");
	find_vbox = glade_xml_get_widget (gui, "find_vbox");
	panel->priv->search_options_expander = glade_xml_get_widget (gui, "search_options_expander");
 	panel->priv->replace_expander = glade_xml_get_widget (gui, "replace_expander");
 	panel->priv->goto_line_expander = glade_xml_get_widget (gui, "goto_line_expander");

 	panel->priv->search_entry = glade_xml_get_widget (gui, "search_entry");
 	panel->priv->replace_entry = glade_xml_get_widget (gui, "replace_entry");
 	panel->priv->line_number_entry = glade_xml_get_widget (gui, "line_number_entry");
 	
 	panel->priv->find_button = glade_xml_get_widget (gui, "find_button");
 	panel->priv->replace_button = glade_xml_get_widget (gui, "replace_button");
 	panel->priv->replace_all_button = glade_xml_get_widget (gui, "replace_all_button");
 	panel->priv->search_options_vbox = glade_xml_get_widget (gui, "search_options_vbox");

	panel->priv->match_case_checkbutton = glade_xml_get_widget (gui, "match_case_checkbutton");
	panel->priv->entire_word_checkbutton = glade_xml_get_widget (gui, "entire_word_checkbutton");
	panel->priv->search_backwards_checkbutton = glade_xml_get_widget (gui, "search_backwards_checkbutton");
	panel->priv->wrap_around_checkbutton = glade_xml_get_widget (gui, "wrap_around_checkbutton");

	g_object_unref (gui);	 	 	 	

 	if (!find_vbox					||
 	    !search_panel_vbox				||
 	    !panel->priv->replace_expander		||
 	    !panel->priv->search_options_expander	||
 	    !panel->priv->goto_line_expander		||
 	    !panel->priv->search_entry			||
 	    !panel->priv->replace_entry			||
 	    !panel->priv->line_number_entry		||
 	    !panel->priv->find_button			||
 	    !panel->priv->replace_button		||
 	    !panel->priv->replace_all_button		||
 	    !panel->priv->search_options_vbox		||
 	    !panel->priv->match_case_checkbutton	||
 	    !panel->priv->entire_word_checkbutton	||      
 	    !panel->priv->search_backwards_checkbutton	||
 	    !panel->priv->wrap_around_checkbutton)
 	{
 		gchar *msg;
		GtkWidget *label;		
		msg = g_strdup_printf (MISSING_WIDGETS,
			       GEDIT_GLADEDIR "search-panel.glade");

		g_warning (msg);
		
		label = gtk_label_new (msg);
		
		gtk_box_pack_start (GTK_BOX (panel), 
				    label, 
				    TRUE, 
				    TRUE, 
				    0);
				    
		g_free (msg);
		
		return ;
 	}
 	
 	gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (panel->priv->wrap_around_checkbutton),
 				      TRUE);

	/* Create the completion object for the search entry */
	completion = gtk_entry_completion_new ();

	gtk_entry_completion_set_minimum_key_length (completion,
						     MIN_KEY_LEN);
					     		    
	/* Assign the completion to the entry */
	gtk_entry_set_completion (GTK_ENTRY (panel->priv->search_entry), 
				  completion);
	g_object_unref (completion);
    
	/* Create a tree model and use it as the completion model */
	store = gtk_list_store_new (1, G_TYPE_STRING);
	gtk_entry_completion_set_model (completion, GTK_TREE_MODEL (store));
	g_object_unref (store);
		
	/* Use model column 0 as the text column */
	gtk_entry_completion_set_text_column (completion, 0);
	
	/* Create the completion object for the replace entry */
	completion = gtk_entry_completion_new ();

	gtk_entry_completion_set_minimum_key_length (completion,
						     MIN_KEY_LEN);
					     		    
	/* Assign the completion to the entry */
	gtk_entry_set_completion (GTK_ENTRY (panel->priv->replace_entry), 
				  completion);
	g_object_unref (completion);
    
	/* Create a tree model and use it as the completion model */
	store = gtk_list_store_new (1, G_TYPE_STRING);
	gtk_entry_completion_set_model (completion, GTK_TREE_MODEL (store));
	g_object_unref (store);
		
	/* Use model column 0 as the text column */
	gtk_entry_completion_set_text_column (completion, 0);	
	
 	gtk_box_pack_start (GTK_BOX (panel), 
			    search_panel_vbox, 
			    TRUE, 
			    TRUE, 
			    0);
			    
	g_signal_connect (panel->priv->line_number_entry, 
			  "insert_text",
			  G_CALLBACK (line_number_entry_insert_text), 
			  NULL);
	g_signal_connect (panel->priv->line_number_entry, 
			  "changed",
			  G_CALLBACK (line_number_entry_changed), 
			  panel);
	g_signal_connect (panel->priv->line_number_entry, 
			  "activate",
			  G_CALLBACK (line_number_entry_activate), 
			  panel);
	g_signal_connect (panel->priv->search_entry, 
			  "insert_text",
			  G_CALLBACK (search_entry_insert_text), 
			  NULL);
	g_signal_connect (panel->priv->search_entry, 
			  "changed",
			  G_CALLBACK (search_entry_changed), 
			  panel);
	g_signal_connect (panel->priv->search_entry, 
			  "activate",
			  G_CALLBACK (search_entry_activate), 
			  panel);
	g_signal_connect (panel->priv->replace_entry, 
			  "changed",
			  G_CALLBACK (replace_entry_changed), 
			  panel);
	g_signal_connect (panel->priv->replace_entry, 
			  "activate",
			  G_CALLBACK (replace_entry_activate), 
			  panel);			  
	g_signal_connect (panel->priv->find_button, 
			  "clicked",
			  G_CALLBACK (find_button_clicked), 
			  panel);
	g_signal_connect (panel->priv->replace_button, 
			  "clicked",
			  G_CALLBACK (replace_button_clicked), 
			  panel);
	g_signal_connect (panel->priv->replace_all_button, 
			  "clicked",
			  G_CALLBACK (replace_all_button_clicked), 
			  panel);			  			  		  
	g_signal_connect (panel->priv->match_case_checkbutton,
			  "toggled",
			  G_CALLBACK (option_button_toggled),
			  panel);
	g_signal_connect (panel->priv->entire_word_checkbutton,
			  "toggled",
			  G_CALLBACK (option_button_toggled),
			  panel);
	g_signal_connect (panel->priv->search_backwards_checkbutton,
			  "toggled",
			  G_CALLBACK (option_button_toggled),
			  panel);			  
	g_signal_connect (panel->priv->wrap_around_checkbutton,
			  "toggled",
			  G_CALLBACK (option_button_toggled),
			  panel);
}

GtkWidget *
gedit_search_panel_new (GeditWindow *window)
{
	g_return_val_if_fail (GEDIT_IS_WINDOW (window), NULL);

	return GTK_WIDGET (g_object_new (GEDIT_TYPE_SEARCH_PANEL, 
					 "window", window,
					 NULL));
}

static gboolean
show_side_pane (GeditSearchPanel *panel)
{
	GtkWidget *sp;
	gboolean was_visible;
	
	sp = GTK_WIDGET (gedit_window_get_side_panel (panel->priv->window));
		
	was_visible = GTK_WIDGET_VISIBLE (sp);
	if (!was_visible)
		_gedit_window_set_side_panel_visible (panel->priv->window,
						      TRUE);
						      
	if (!gedit_panel_item_is_active (GEDIT_PANEL (sp), GTK_WIDGET (panel)))
	{
		gedit_panel_activate_item (GEDIT_PANEL (sp), GTK_WIDGET (panel));
		return FALSE;
	}
	
	return was_visible;
}

void
gedit_search_panel_focus_search	(GeditSearchPanel *panel)
{
	gboolean was_visible;
	const gchar *text;
	GeditDocument *doc;
	gboolean selection_exists;
	gchar *find_text = NULL;
	gint sel_len = 0;
		
	g_return_if_fail (GEDIT_IS_SEARCH_PANEL (panel));

	doc = gedit_window_get_active_document (panel->priv->window);
	
	if (doc == NULL)
		return;

	selection_exists = get_selected_text (GTK_TEXT_BUFFER (doc), 
					      &find_text, 
					      &sel_len);
						      			
	was_visible = show_side_pane (panel);
				
	text = gtk_entry_get_text (GTK_ENTRY (panel->priv->search_entry));
	
	if (!was_visible)
		text = NULL;
		
	if ((text == NULL) || (text[0] == 0))
	{		
		if (!selection_exists   ||
		    (find_text == NULL) ||
		    (sel_len > 160))
		{
			gchar *lst;

			lst = gedit_document_get_search_text (doc, NULL);
			
			if (lst != NULL)
			{
				g_free (find_text);
				find_text = lst;
			}
			else if (sel_len > 160)
			{
				g_free (find_text);
				find_text = NULL;
			}
		}
		
		if (find_text != NULL)
			gtk_entry_set_text (GTK_ENTRY (panel->priv->search_entry), 
					    find_text);
		
		g_free (find_text);
	}
	
	gtk_widget_grab_focus (panel->priv->search_entry);
}

void
gedit_search_panel_focus_replace (GeditSearchPanel *panel)
{
	GeditDocument *doc;
	
	g_return_if_fail (GEDIT_IS_SEARCH_PANEL (panel));

	doc = gedit_window_get_active_document (panel->priv->window);
	if (doc == NULL)
		return;
			
	show_side_pane (panel);
	
	gtk_expander_set_expanded (GTK_EXPANDER (panel->priv->replace_expander),
				   TRUE);
	
	gtk_widget_grab_focus (panel->priv->replace_entry);
}

void 
gedit_search_panel_focus_goto_line (GeditSearchPanel *panel)
{
	GeditDocument *doc;
	
	g_return_if_fail (GEDIT_IS_SEARCH_PANEL (panel));
	
	doc = gedit_window_get_active_document (panel->priv->window);
	if (doc == NULL)
		return;
		
	show_side_pane (panel);
	
	gtk_expander_set_expanded (GTK_EXPANDER (panel->priv->goto_line_expander),
				   TRUE);
	
	gtk_widget_grab_focus (panel->priv->line_number_entry);
}

void
gedit_search_panel_search_again	(GeditSearchPanel *panel,
				 gboolean          backward)
{
	GeditView *active_view;
	GeditDocument *doc;
	GtkWidget *sp;
	gboolean is_visible;
	gchar *old_search_text;
	gint old_flags = 0;
	
	g_return_if_fail (GEDIT_IS_SEARCH_PANEL (panel));

	active_view = gedit_window_get_active_view (panel->priv->window);
	if (active_view == NULL)
		return;
	
	doc = GEDIT_DOCUMENT (gtk_text_view_get_buffer (GTK_TEXT_VIEW (active_view)));
		
	sp = GTK_WIDGET (gedit_window_get_side_panel (panel->priv->window));
		
	is_visible = GTK_WIDGET_VISIBLE (sp) &&
		     gedit_panel_item_is_active (GEDIT_PANEL (sp), GTK_WIDGET (panel));
		
	old_search_text = gedit_document_get_search_text (doc, &old_flags);
			
	if (is_visible)			
	{
		const gchar *entry_text;
		gboolean wrap_around;
		gboolean case_sensitive;
		gboolean entire_word;	
		gint flags = 0;
		
		entry_text = gtk_entry_get_text (GTK_ENTRY (panel->priv->search_entry));
		if ((*entry_text == 0))
		{
			g_free (old_search_text);
			gtk_widget_grab_focus (panel->priv->search_entry);
			return;
		}
			
		/* retrieve search settings from the toggle buttons */
		case_sensitive = gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (panel->priv->match_case_checkbutton));
		entire_word = gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (panel->priv->entire_word_checkbutton));
		wrap_around = gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (panel->priv->wrap_around_checkbutton));
		
		flags = 0;
		
		GEDIT_SEARCH_SET_CASE_SENSITIVE (flags, case_sensitive);
		GEDIT_SEARCH_SET_ENTIRE_WORD (flags, entire_word);
				
		if ((old_search_text == NULL) || (strcmp (old_search_text, entry_text) != 0))
		{
			gedit_document_set_search_text (doc, entry_text, flags);
		}
		else if (flags != old_flags)
		{
			gedit_document_set_search_text (doc, entry_text, flags);
		}
				
		run_search (panel, 
		   	    active_view,
		   	    entry_text,
		   	    wrap_around,
		   	    backward,
		   	    TRUE);
	}
	else if (old_search_text != NULL)
	{
		run_search (panel, 
		   	    active_view,
		   	    old_search_text,
		   	    TRUE,
		   	    backward,
		   	    TRUE);
	}
	else
	{
		gedit_search_panel_focus_search (panel);
		
		if (backward)
		{
			if (!gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (panel->priv->search_backwards_checkbutton)))
			{
				gtk_expander_set_expanded (GTK_EXPANDER (panel->priv->search_options_expander),
							   TRUE);
			
				gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (panel->priv->search_backwards_checkbutton),
							      TRUE);
			}
		}
		else
		{
			if (gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (panel->priv->search_backwards_checkbutton)))
			{
				gtk_expander_set_expanded (GTK_EXPANDER (panel->priv->search_options_expander),
							   TRUE);
			
				gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (panel->priv->search_backwards_checkbutton),
							      FALSE);
			}
		}
	}
	
	g_free (old_search_text);
}				 
