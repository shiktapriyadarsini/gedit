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

#include <glib/gi18n.h>
#include <glade/glade-xml.h>

#define GEDIT_SEARCH_PANEL_GET_PRIVATE(object)(G_TYPE_INSTANCE_GET_PRIVATE ((object), GEDIT_TYPE_SEARCH_PANEL, GeditSearchPanelPrivate))

struct _GeditSearchPanelPrivate
{
	GeditWindow  *window;
	
	GtkWidget    *replace_expander;
	GtkWidget    *goto_line_expander;
	
	GtkWidget    *search_entry;
	GtkWidget    *replace_entry;
	GtkWidget    *line_number_entry;
	
	GtkWidget    *find_button;
	GtkWidget    *replace_button;
	GtkWidget    *replace_all_button;
	
	GtkWidget    *search_options_vbox;		
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
window_tab_removed (GeditWindow      *window,
		    GeditTab         *tab,
		    GeditSearchPanel *panel)
{
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
	gtk_widget_set_sensitive (panel->priv->search_entry,
				  TRUE);
	gtk_widget_set_sensitive (panel->priv->replace_entry,
				  TRUE);
	gtk_widget_set_sensitive (panel->priv->line_number_entry,
				  TRUE);	
	gtk_widget_set_sensitive (panel->priv->find_button,
				  TRUE);
	gtk_widget_set_sensitive (panel->priv->replace_button,
				  TRUE);			  
	gtk_widget_set_sensitive (panel->priv->replace_all_button,
				  TRUE);
	gtk_widget_set_sensitive (panel->priv->search_options_vbox,
				  TRUE);				  
}

static void
set_window (GeditSearchPanel *panel,
	    GeditWindow      *window)
{
	g_return_if_fail (panel->priv->window == NULL);
	g_return_if_fail (GEDIT_IS_WINDOW (window));
	
	panel->priv->window = window;
	
	g_signal_connect (window,
			  "tab_added",
			  G_CALLBACK (window_tab_added),
			  panel);
	g_signal_connect (window,
			  "tab_removed",
			  G_CALLBACK (window_tab_removed),
			  panel);
/*			  
	g_signal_connect (window,
			  "tabs_reordered",
			  G_CALLBACK (window_tabs_reordered),
			  panel);
	g_signal_connect (window,
			  "active_tab_changed",
			  G_CALLBACK (window_active_tab_changed),
			  panel);		  
*/
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
		GeditDocument *active_document;
		gint line;
		
		active_document = gedit_window_get_active_document (panel->priv->window);
		
		line = MAX (atoi (line_str) - 1, 0);
		gedit_document_goto_line (active_document, line);
		gedit_view_scroll_to_cursor (active_view);
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

static void
run_search (GeditSearchPanel *panel,
            GeditView        *view)
{
	GtkTextIter start_iter;
	GtkTextIter match_start;
	GtkTextIter match_end;	
	gboolean found;
	gboolean wrap_around;
	const gchar *entry_text;
	
	GeditDocument *doc;

	doc = GEDIT_DOCUMENT (gtk_text_view_get_buffer (GTK_TEXT_VIEW (view)));
	
	gtk_text_buffer_get_selection_bounds (GTK_TEXT_BUFFER (doc),
					      &start_iter,
					      &match_end);
	
	gtk_text_iter_order (&start_iter, &match_end);
	
	/* run search */
	found = gedit_document_search_forward (doc,
					       &start_iter,
					       NULL,
					       &match_start,
					       &match_end);

	/* FIXME */
	wrap_around = TRUE;
	
	/* g_print ("Found: %s\n", found ? "TRUE" : "FALSE"); */
	
	if (!found && wrap_around)
	{
		found = gedit_document_search_forward (doc,
					       NULL,
					       NULL, /* FIXME: set the end_inter */
					       &match_start,
					       &match_end);
	}
	
	if (found)
	{
		gtk_text_buffer_place_cursor (GTK_TEXT_BUFFER (doc),
					&match_start);

		gtk_text_buffer_move_mark_by_name (GTK_TEXT_BUFFER (doc),
					"selection_bound", &match_end);
	}
	
	entry_text  = gtk_entry_get_text (GTK_ENTRY (panel->priv->search_entry));

	if (!found)
		gtk_text_buffer_place_cursor (GTK_TEXT_BUFFER (doc),
					      &start_iter);
					      
	if (found || (*entry_text == '\0'))
	{
		gedit_view_scroll_to_cursor (view);
		gtk_widget_set_sensitive (panel->priv->find_button, TRUE);
		gtk_widget_modify_base (panel->priv->search_entry,
				        GTK_STATE_NORMAL,
				        NULL);
		gtk_widget_modify_text (panel->priv->search_entry,
				        GTK_STATE_NORMAL,
				        NULL);	
				        
			        
	}
	else
	{
		GdkColor red;
		GdkColor white;
		
		/* FIXME: a11y */
		
					
		gdk_color_parse ("#FF6666", &red);
		gdk_color_parse ("white", &white);		
		gtk_widget_set_sensitive (panel->priv->find_button, FALSE);
		gtk_widget_modify_base (panel->priv->search_entry,
				        GTK_STATE_NORMAL,
				        &red);
		gtk_widget_modify_text (panel->priv->search_entry,
				        GTK_STATE_NORMAL,
				        &white);				        
	}
#if 0
	GeditView *active_view;
	GeditDocument *doc;
	const gchar *search_string = NULL;
	gboolean found;
	gboolean case_sensitive;
	gboolean entire_word;
	gboolean wrap_around;
	gboolean search_backwards;
	gint flags = 0;

	gedit_debug (DEBUG_SEARCH);
	
	active_view = gedit_window_get_active_view (panel->priv->window);
	if (active_view == NULL)
		return;

	doc = GEDIT_DOCUMENT (gtk_text_view_get_buffer (GTK_TEXT_VIEW (active_view)));
	g_return_if_fail (doc != NULL);
			
	search_string = gtk_entry_get_text (GTK_ENTRY (panel->priv->search_entry));		
	g_return_if_fail (search_string != NULL);

	if (strlen (search_string) <= 0)
		return;
		
	/* retrieve search settings from the dialog */
	case_sensitive = FALSE; //gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (dialog->match_case_checkbutton));
	entire_word = FALSE; //gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (dialog->entire_word_checkbutton));
	wrap_around = TRUE; //gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (dialog->wrap_around_checkbutton));
	search_backwards = FALSE; //gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (dialog->search_backwards_checkbutton));

	/* setup quarks for next invocation */
	/*
	g_object_set_qdata (G_OBJECT (doc), was_search_backwards_id, GBOOLEAN_TO_POINTER (search_backwards));
	g_object_set_qdata (G_OBJECT (doc), was_wrap_around_id, GBOOLEAN_TO_POINTER (wrap_around));
	g_object_set_qdata (G_OBJECT (doc), was_entire_word_id, GBOOLEAN_TO_POINTER (entire_word));
	g_object_set_qdata (G_OBJECT (doc), was_case_sensitive_id, GBOOLEAN_TO_POINTER (case_sensitive));
	*/

	/* setup search parameter bitfield */
	GEDIT_SEARCH_SET_FROM_CURSOR (flags, TRUE);
	GEDIT_SEARCH_SET_CASE_SENSITIVE (flags, case_sensitive);
	GEDIT_SEARCH_SET_BACKWARDS (flags, search_backwards);
	GEDIT_SEARCH_SET_ENTIRE_WORD (flags, entire_word);

	/* run search */
	found = gedit_document_find (doc, search_string, flags);

	/* if we're able to wrap, don't use the cursor position */
	if (!found && wrap_around)
	{
		GEDIT_SEARCH_SET_FROM_CURSOR (flags, FALSE);
		found = gedit_document_find (doc, search_string, flags);
	}

	if (found)
	{
		gedit_view_scroll_to_cursor (active_view);
		gtk_widget_set_sensitive (panel->priv->find_button, TRUE);
		gtk_widget_modify_bg (panel->priv->search_entry,
				      GTK_STATE_NORMAL,
				      NULL);
	}
	else
	{
		GdkColor red;
		
		gdk_color_parse ("red", &red);
		gtk_widget_set_sensitive (panel->priv->find_button, FALSE);
		gtk_widget_modify_bg (panel->priv->search_entry,
				      GTK_STATE_NORMAL,
				      &red);
	}
#endif	
}

static void
search_entry_changed (GtkEditable      *editable,
		      GeditSearchPanel *panel)
{
	GeditView *active_view; 
	GeditDocument *doc;
	gchar *search_text;
	const gchar *entry_text;
	
	active_view = gedit_window_get_active_view (panel->priv->window);
	if (active_view == NULL)
		return;
	
	doc = GEDIT_DOCUMENT (gtk_text_view_get_buffer (GTK_TEXT_VIEW (active_view)));
	
	entry_text = gtk_entry_get_text (GTK_ENTRY (panel->priv->search_entry));
	
	search_text = gedit_document_get_search_text (doc, NULL);
	
	if ((search_text == NULL) || (strcmp (search_text, entry_text) != 0))
		/* FIXME: write get_search_flags */
		gedit_document_set_search_text (doc, entry_text, 0);
		
	run_search (panel, active_view);
}

/* FIXME: I think we have the same function in gedit-utils */
static gchar* 
escape_search_text (const gchar* text)
{
	GString *str;
	gint length;
	const gchar *p;
 	const gchar *end;

  	g_return_val_if_fail (text != NULL, NULL);

	gedit_debug_message (DEBUG_SEARCH, "Text: %s", text);

    	length = strlen (text);

	str = g_string_new ("");

  	p = text;
  	end = text + length;

  	while (p != end)
    	{
      		const gchar *next;
      		next = g_utf8_next_char (p);

		switch (*p)
        	{
       			case '\n':
          			g_string_append (str, "\\n");
          			break;
			case '\r':
          			g_string_append (str, "\\r");
          			break;
			case '\t':
          			g_string_append (str, "\\t");
          			break;
        		default:
          			g_string_append_len (str, p, next - p);
          			break;
        	}

      		p = next;
    	}

	return g_string_free (str, FALSE);
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

	escaped_text = escape_search_text (text);

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

#define GEDIT_GLADEDIR "./dialogs/"

static void
gedit_search_panel_init (GeditSearchPanel *panel)
{
	GladeXML *gui;
	GtkWidget *find_vbox;
	GtkWidget *search_panel_vbox;
	
	panel->priv = GEDIT_SEARCH_PANEL_GET_PRIVATE (panel);
	
	gui = glade_xml_new ( GEDIT_GLADEDIR "search-panel.glade",
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
 	panel->priv->replace_expander = glade_xml_get_widget (gui, "replace_expander");
 	panel->priv->goto_line_expander = glade_xml_get_widget (gui, "goto_line_expander");
 	
 	panel->priv->search_entry = glade_xml_get_widget (gui, "search_entry");
 	panel->priv->replace_entry = glade_xml_get_widget (gui, "replace_entry");
 	panel->priv->line_number_entry = glade_xml_get_widget (gui, "line_number_entry");
 	
 	panel->priv->find_button = glade_xml_get_widget (gui, "find_button");
 	panel->priv->replace_button = glade_xml_get_widget (gui, "replace_button");
 	panel->priv->replace_all_button = glade_xml_get_widget (gui, "replace_all_button");
 	panel->priv->search_options_vbox = glade_xml_get_widget (gui, "search_options_vbox");
 	 	 	 	
 	if (!find_vbox				||
 	    !search_panel_vbox			||
 	    !panel->priv->replace_expander 	||
 	    !panel->priv->goto_line_expander	||
 	    !panel->priv->search_entry		||
 	    !panel->priv->replace_entry		||
 	    !panel->priv->line_number_entry	||
 	    !panel->priv->find_button		||
 	    !panel->priv->replace_button	||
 	    !panel->priv->replace_all_button	||
 	    !panel->priv->search_options_vbox)
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
 	
 	gtk_box_pack_start (GTK_BOX (panel), 
			    search_panel_vbox, 
			    TRUE, 
			    TRUE, 
			    0);
			    
	g_signal_connect (G_OBJECT (panel->priv->line_number_entry), 
			  "insert_text",
			  G_CALLBACK (line_number_entry_insert_text), 
			  NULL);
	g_signal_connect (G_OBJECT (panel->priv->line_number_entry), 
			  "changed",
			  G_CALLBACK (line_number_entry_changed), 
			  panel);
	g_signal_connect (G_OBJECT (panel->priv->line_number_entry), 
			  "activate",
			  G_CALLBACK (line_number_entry_activate), 
			  panel);
	g_signal_connect (G_OBJECT (panel->priv->search_entry), 
			  "insert_text",
			  G_CALLBACK (search_entry_insert_text), 
			  NULL);
	g_signal_connect (G_OBJECT (panel->priv->search_entry), 
			  "changed",
			  G_CALLBACK (search_entry_changed), 
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
	
	
	 /* FIXME: set the focus on the replace entry only if the text in the
	    search entry matches some text in the current document */			   
	if (g_utf8_strlen (
		gtk_entry_get_text (GTK_ENTRY (panel->priv->search_entry)), -1) > 0) 
	{
		gtk_widget_grab_focus (panel->priv->search_entry);
	}
	else
	{
		gtk_widget_grab_focus (panel->priv->replace_entry);
	}
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
