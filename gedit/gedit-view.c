/*
 * gedit-view.c
 * This file is part of gedit
 *
 * Copyright (C) 1998, 1999 Alex Roberts, Evan Lawrence
 * Copyright (C) 2000, 2002 Chema Celorio, Paolo Maggi 
 * Copyright (C) 2003-2005 Paolo Maggi  
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
 */

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <string.h>

#include <gdk/gdkkeysyms.h>

#include "gedit-view.h"
#include "gedit-debug.h"
#include "gedit-prefs-manager-app.h"

#define GEDIT_VIEW_SCROLL_MARGIN 0.02

struct _GeditViewPrivate
{
	gint dummy;
};

static void gedit_view_class_init		(GeditViewClass *klass);
static void gedit_view_init 			(GeditView      *view);
static void gedit_view_destroy			(GtkObject        *object);
static void gedit_view_finalize			(GObject        *object);

static void doc_readonly_changed_handler 	(GeditDocument  *document, 
						 gboolean        readonly, 
						 GeditView      *view);

static GtkSourceViewClass *parent_class = NULL;

GType
gedit_view_get_type (void)
{
	static GType view_type = 0;

  	if (G_UNLIKELY (view_type == 0))
    	{
      		static const GTypeInfo our_info =
      		{
        		sizeof (GeditViewClass),
        		NULL,		/* base_init */
        		NULL,		/* base_finalize */
        		(GClassInitFunc) gedit_view_class_init,
        		NULL,           /* class_finalize */
        		NULL,           /* class_data */
        		sizeof (GeditView),
        		0,              /* n_preallocs */
        		(GInstanceInitFunc) gedit_view_init
      		};

      		view_type = g_type_register_static (GTK_TYPE_SOURCE_VIEW,
                				    "GeditView",
                                       	 	    &our_info,
                                       		    0);
    	}

	return view_type;
}

static void
doc_readonly_changed_handler (GeditDocument *document, 
			      gboolean       readonly,
			      GeditView     *view)
{
	gedit_debug (DEBUG_VIEW, "");

	gtk_text_view_set_editable (GTK_TEXT_VIEW (view), 
				    !readonly);
}

static void
gedit_view_class_init (GeditViewClass *klass)
{
	GObjectClass *object_class = G_OBJECT_CLASS (klass);
	GtkObjectClass *gtkobject_class = GTK_OBJECT_CLASS (klass);

	/* Note: it is commented out since we don't use it - Paolo */
	/* g_type_class_add_private (klass, sizeof (GeditViewPrivate)); */

  	parent_class = g_type_class_peek_parent (klass);

  	gtkobject_class->destroy = gedit_view_destroy;
  	object_class->finalize = gedit_view_finalize;
}

static void
move_cursor (GtkTextView       *text_view,
	     const GtkTextIter *new_location,
	     gboolean           extend_selection)
{
	GtkTextBuffer *buffer = text_view->buffer;

	if (extend_selection)
		gtk_text_buffer_move_mark_by_name (buffer, "insert",
						   new_location);
	else
		gtk_text_buffer_place_cursor (buffer, new_location);

	gtk_text_view_scroll_to_mark (GTK_TEXT_VIEW (text_view),
				      gtk_text_buffer_get_insert (buffer),
				      GEDIT_VIEW_SCROLL_MARGIN,
				      FALSE,
				      0.0,
				      0.0);
}

/*
 * This feature is implemented in gedit and not in gtksourceview since the latter
 * has a similar feature called smart home/end that it is non-capatible with this 
 * one and is more "invasive". May be in the future we will move this feature in 
 * gtksourceview.
 */
static void
gedit_view_move_cursor (GtkTextView    *text_view,
			GtkMovementStep step,
			gint            count,
			gboolean        extend_selection,
			gpointer	data)
{
	GtkTextBuffer *buffer = text_view->buffer;
	GtkTextMark *mark;
	GtkTextIter cur, iter;

	if (step != GTK_MOVEMENT_DISPLAY_LINE_ENDS)
		return;

	g_return_if_fail (!gtk_source_view_get_smart_home_end (
						GTK_SOURCE_VIEW (text_view)));	
			
	mark = gtk_text_buffer_get_insert (buffer);
	gtk_text_buffer_get_iter_at_mark (buffer, &cur, mark);
	iter = cur;

	if ((count == -1) && gtk_text_iter_starts_line (&iter))
	{
		/* Find the iter of the first character on the line. */
		while (!gtk_text_iter_ends_line (&cur))
		{
			gunichar c = gtk_text_iter_get_char (&cur);
			if (g_unichar_isspace (c))
				gtk_text_iter_forward_char (&cur);
			else
				break;
		}

		if (!gtk_text_iter_equal (&cur, &iter))
		{
			move_cursor (text_view, &cur, extend_selection);
			g_signal_stop_emission_by_name (text_view, 
							"move-cursor");
		}

		return;
	}

	if ((count == 1) && gtk_text_iter_ends_line (&iter))
	{
		/* Find the iter of the last character on the line. */
		while (!gtk_text_iter_starts_line (&cur))
		{
			gunichar c;
			gtk_text_iter_backward_char (&cur);
			c = gtk_text_iter_get_char (&cur);
			if (!g_unichar_isspace (c))
			{
				/* We've gone one character too far. */
				gtk_text_iter_forward_char (&cur);
				break;
			}
		}

		if (!gtk_text_iter_equal (&cur, &iter))
		{
			move_cursor (text_view, &cur, extend_selection);
			g_signal_stop_emission_by_name (text_view, 
							"move-cursor");
		}
	}
}

static gboolean
gedit_view_expose (GtkTextView    *widget, 
		   GdkEventExpose *event,
                   GeditView      *view)
{
	GtkTextBuffer* buffer = NULL;

	gedit_debug (DEBUG_VIEW, "");

	g_return_val_if_fail (GEDIT_IS_VIEW (view), FALSE);
	
	buffer = gtk_text_view_get_buffer (GTK_TEXT_VIEW (view));
	g_return_val_if_fail (buffer != NULL, FALSE);

	gtk_text_view_scroll_to_mark (GTK_TEXT_VIEW (view),
				      gtk_text_buffer_get_insert (buffer),
				      0.25,
				      FALSE,
				      0.0,
				      0.0);

	g_signal_handlers_disconnect_by_func (widget, 
					      G_CALLBACK (gedit_view_expose), 
					      view);

	return FALSE;
}

static void 
gedit_view_init (GeditView *view)
{	
#if 0
	GeditDocument *doc;
#endif	
	gedit_debug (DEBUG_VIEW, "");
	
	/*
	 *  Set tab, fonts, wrap mode, colors, etc. according
	 *  to preferences 
	 */
	if (!gedit_prefs_manager_get_use_default_font ())
	{
		gchar *editor_font;
		
		editor_font = gedit_prefs_manager_get_editor_font ();
		
		gedit_view_set_font (view, FALSE, editor_font);

		g_free (editor_font);
	}

	if (!gedit_prefs_manager_get_use_default_colors ())
	{
		GdkColor background;
		GdkColor text;
		GdkColor selection;
		GdkColor sel_text;
		
		background = gedit_prefs_manager_get_background_color ();
		text = gedit_prefs_manager_get_text_color ();
		selection = gedit_prefs_manager_get_selection_color ();
		sel_text = gedit_prefs_manager_get_selected_text_color ();

		gedit_view_set_colors (view, 
				       FALSE,
				       &background, 
				       &text, 
				       &selection, 
				       &sel_text);
	}	

	g_object_set (G_OBJECT (view), 
		      "wrap_mode", gedit_prefs_manager_get_wrap_mode (),
		      "show_line_numbers", gedit_prefs_manager_get_display_line_numbers (),
		      "auto_indent", gedit_prefs_manager_get_auto_indent (),
		      "tabs_width", gedit_prefs_manager_get_tabs_size (),
		      "insert_spaces_instead_of_tabs", gedit_prefs_manager_get_insert_spaces (),
		      "show_margin", gedit_prefs_manager_get_display_right_margin (), 
		      "margin", gedit_prefs_manager_get_right_margin_position (),
		      "highlight_current_line", gedit_prefs_manager_get_highlight_current_line (), 
		      "smart_home_end", FALSE, /* Never changes this */
		      NULL);

	g_signal_connect (G_OBJECT (view), 
			  "move-cursor",
			  G_CALLBACK (gedit_view_move_cursor), 
			  view);

#if 0
	doc = GEDIT_DOCUMENT (gtk_text_view_get_buffer (GTK_TEXT_VIEW (view)));
	g_return_if_fail (doc != NULL);
	
	g_signal_connect (G_OBJECT (doc),
			  "readonly_changed",
			  G_CALLBACK (doc_readonly_changed_handler),
			  view);
#endif
	g_signal_connect_after (G_OBJECT (view),
				"expose-event",
				G_CALLBACK (gedit_view_expose),
				view);
#if 0
	gtk_text_view_set_editable (GTK_TEXT_VIEW (view), 
				    !gedit_document_is_readonly (doc));
#endif				   
}

static void
gedit_view_destroy (GtkObject *object)
{
	GeditView *view;
	GtkTextBuffer *doc;

	view = GEDIT_VIEW (object);

	doc = gtk_text_view_get_buffer (GTK_TEXT_VIEW (view));
	g_return_if_fail (doc != NULL);

	g_signal_handlers_disconnect_by_func (G_OBJECT (doc),
					      G_CALLBACK (gedit_view_move_cursor),
					      view);

	(* GTK_OBJECT_CLASS (parent_class)->destroy) (object);
}

static void
gedit_view_finalize (GObject *object)
{
	GeditView *view;

	view = GEDIT_VIEW (object);

	(* G_OBJECT_CLASS (parent_class)->finalize) (object);
}

/**
 * gedit_view_new:
 * @doc: a #GeditDocument
 * 
 * Creates a new #GeditView object displaying the @doc document. 
 * @doc cannot be NULL.
 *
 * Return value: a new #GeditView
 **/
GtkWidget *
gedit_view_new (GeditDocument *doc)
{
	GtkWidget *view;
	
	gedit_debug (DEBUG_VIEW, "START");

	g_return_val_if_fail (GEDIT_IS_DOCUMENT (doc), NULL);
	
	view = GTK_WIDGET (g_object_new (GEDIT_TYPE_VIEW, NULL));
	
	gtk_text_view_set_buffer (GTK_TEXT_VIEW (view),
				  GTK_TEXT_BUFFER (doc));
				  		
	g_signal_connect (G_OBJECT (doc),
			  "readonly_changed",
			  G_CALLBACK (doc_readonly_changed_handler),
			  view);
			  
	gtk_text_view_set_editable (GTK_TEXT_VIEW (view), 
				    !gedit_document_is_readonly (doc));					  
				    
	gedit_debug (DEBUG_VIEW, "END: %d", G_OBJECT (view)->ref_count);

	gtk_widget_show_all (view);

	return view;
}

void
gedit_view_cut_clipboard (GeditView *view)
{
	GtkTextBuffer *buffer = NULL;

	gedit_debug (DEBUG_VIEW, "");

	g_return_if_fail (GEDIT_IS_VIEW (view));

	buffer = gtk_text_view_get_buffer (GTK_TEXT_VIEW (view));
	g_return_if_fail (buffer != NULL);

	/* FIXME: what is default editability of a buffer? */
  	gtk_text_buffer_cut_clipboard (buffer,
  				       gtk_clipboard_get (GDK_NONE),
				       !gedit_document_is_readonly (
				       		GEDIT_DOCUMENT (buffer)));
  	
	gtk_text_view_scroll_to_mark (GTK_TEXT_VIEW (view),
				      gtk_text_buffer_get_insert (buffer),
				      GEDIT_VIEW_SCROLL_MARGIN,
				      FALSE,
				      0.0,
				      0.0);
}

void
gedit_view_copy_clipboard (GeditView *view)
{
	GtkTextBuffer *buffer = NULL;

	gedit_debug (DEBUG_VIEW, "");

	g_return_if_fail (GEDIT_IS_VIEW (view));

	buffer = gtk_text_view_get_buffer (GTK_TEXT_VIEW (view));
	g_return_if_fail (buffer != NULL);
	
  	gtk_text_buffer_copy_clipboard (buffer,
  					gtk_clipboard_get (GDK_NONE));

	gtk_text_view_scroll_to_mark (GTK_TEXT_VIEW (view),
				      gtk_text_buffer_get_insert (buffer),
				      GEDIT_VIEW_SCROLL_MARGIN,
				      FALSE,
				      0.0,
				      0.0);
}

void
gedit_view_paste_clipboard (GeditView *view)
{
  	GtkTextBuffer *buffer = NULL;

	gedit_debug (DEBUG_VIEW, "");

	g_return_if_fail (GEDIT_IS_VIEW (view));

	buffer = gtk_text_view_get_buffer (GTK_TEXT_VIEW (view));
	g_return_if_fail (buffer != NULL);

	/* FIXME: what is default editability of a buffer? */
  	gtk_text_buffer_paste_clipboard (buffer,
					 gtk_clipboard_get (GDK_NONE),
					 NULL,
					 !gedit_document_is_readonly (
						GEDIT_DOCUMENT (buffer)));

	gtk_text_view_scroll_to_mark (GTK_TEXT_VIEW (view),
				      gtk_text_buffer_get_insert (buffer),
				      GEDIT_VIEW_SCROLL_MARGIN,
				      FALSE,
				      0.0,
				      0.0);
}

void
gedit_view_delete_selection (GeditView *view)
{
  	GtkTextBuffer *buffer = NULL;

	gedit_debug (DEBUG_VIEW, "");

	g_return_if_fail (GEDIT_IS_VIEW (view));

	buffer = gtk_text_view_get_buffer (GTK_TEXT_VIEW (view));
	g_return_if_fail (buffer != NULL);

	/* FIXME: what is default editability of a buffer? */
	gtk_text_buffer_delete_selection (buffer,
					  TRUE,
					  !gedit_document_is_readonly (
						GEDIT_DOCUMENT (buffer)));
						
	gtk_text_view_scroll_to_mark (GTK_TEXT_VIEW (view),
				      gtk_text_buffer_get_insert (buffer),
				      GEDIT_VIEW_SCROLL_MARGIN,
				      FALSE,
				      0.0,
				      0.0);
}

void
gedit_view_select_all (GeditView *view)
{
	GtkTextBuffer *buffer = NULL;
	GtkTextIter start, end;

	gedit_debug (DEBUG_VIEW, "");

	g_return_if_fail (GEDIT_IS_VIEW (view));

	buffer = gtk_text_view_get_buffer (GTK_TEXT_VIEW (view));
	g_return_if_fail (buffer != NULL);

	gtk_text_buffer_get_bounds (buffer, &start, &end);
	gtk_text_buffer_select_range (buffer, &start, &end);
}

void
gedit_view_scroll_to_cursor (GeditView *view)
{
	GtkTextBuffer* buffer = NULL;

	gedit_debug (DEBUG_VIEW, "");

	g_return_if_fail (GEDIT_IS_VIEW (view));
	
	buffer = gtk_text_view_get_buffer (GTK_TEXT_VIEW (view));
	g_return_if_fail (buffer != NULL);

	gtk_text_view_scroll_to_mark (GTK_TEXT_VIEW (view),
				      gtk_text_buffer_get_insert (buffer),
				      GEDIT_VIEW_SCROLL_MARGIN,
				      FALSE,
				      0.0,
				      0.0);
}

/* assign a unique name */
static G_CONST_RETURN gchar *
get_widget_name (GtkWidget *w)
{
	const gchar *name;	

	name = gtk_widget_get_name (w);
	g_return_val_if_fail (name != NULL, NULL);

	if (strcmp (name, g_type_name (GTK_WIDGET_TYPE (w))) == 0)
	{
		static guint d = 0;
		gchar *n;

		n = g_strdup_printf ("%s_%u_%u", name, d, (guint) g_random_int);
		d++;

		gtk_widget_set_name (w, n);
		g_free (n);

		name = gtk_widget_get_name (w);
	}

	return name;
}

/* There is no clean way to set the cursor-color, so we are stuck
 * with the following hack: set the name of each widget and parse
 * a gtkrc string.
 */
static void 
modify_cursor_color (GtkWidget *textview, 
		     GdkColor  *color)
{
	static const char cursor_color_rc[] =
		"style \"svs-cc\"\n"
		"{\n"
			"GtkSourceView::cursor-color=\"#%04x%04x%04x\"\n"
		"}\n"
		"widget \"*.%s\" style : application \"svs-cc\"\n";

	const gchar *name;
	gchar *rc_temp;

	gedit_debug (DEBUG_VIEW, "");

	name = get_widget_name (textview);
	g_return_if_fail (name != NULL);

	if (color != NULL)
	{
		rc_temp = g_strdup_printf (cursor_color_rc,
					   color->red, 
					   color->green, 
					   color->blue,
					   name);
	}
	else
	{
		GtkRcStyle *rc_style;

 		rc_style = gtk_widget_get_modifier_style (textview);

		rc_temp = g_strdup_printf (cursor_color_rc,
					   rc_style->text [GTK_STATE_NORMAL].red,
					   rc_style->text [GTK_STATE_NORMAL].green,
					   rc_style->text [GTK_STATE_NORMAL].blue,
					   name);
	}

	gtk_rc_parse_string (rc_temp);
	gtk_widget_reset_rc_styles (textview);

	g_free (rc_temp);
}

void
gedit_view_set_colors (GeditView *view,
		       gboolean   def,
		       GdkColor  *backgroud,
		       GdkColor  *text,
		       GdkColor  *selection,
		       GdkColor  *sel_text)
{
	gedit_debug (DEBUG_VIEW, "");

	g_return_if_fail (GEDIT_IS_VIEW (view));

	/* just a bit of paranoia */
	gtk_widget_ensure_style (GTK_WIDGET (view));

	if (!def)
	{
		if (backgroud != NULL)
			gtk_widget_modify_base (GTK_WIDGET (view), 
						GTK_STATE_NORMAL, backgroud);

		if (selection != NULL)
		{
			gtk_widget_modify_base (GTK_WIDGET (view), 
						GTK_STATE_SELECTED, selection);

			gtk_widget_modify_base (GTK_WIDGET (view), 
						GTK_STATE_ACTIVE, selection);
		}

		if (sel_text != NULL)
		{
			gtk_widget_modify_text (GTK_WIDGET (view), 
						GTK_STATE_SELECTED, sel_text);		

			gtk_widget_modify_text (GTK_WIDGET (view), 
						GTK_STATE_ACTIVE, sel_text);		
		}

		if (text != NULL)
		{
			gtk_widget_modify_text (GTK_WIDGET (view), 
						GTK_STATE_NORMAL, text);
			modify_cursor_color (GTK_WIDGET (view), text);
		}
	}
	else
	{
		GtkRcStyle *rc_style;

		rc_style = gtk_widget_get_modifier_style (GTK_WIDGET (view));

		rc_style->color_flags [GTK_STATE_NORMAL] = 0;
		rc_style->color_flags [GTK_STATE_SELECTED] = 0;
		rc_style->color_flags [GTK_STATE_ACTIVE] = 0;

		gtk_widget_modify_style (GTK_WIDGET (view), rc_style);

		/* It must be called after the text color has been modified */
		modify_cursor_color (GTK_WIDGET (view), NULL);
	}
}

void
gedit_view_set_font (GeditView   *view, 
		     gboolean     def, 
		     const gchar *font_name)
{
	gedit_debug (DEBUG_VIEW, "");

	g_return_if_fail (GEDIT_IS_VIEW (view));

	if (!def)
	{
		PangoFontDescription *font_desc = NULL;

		g_return_if_fail (font_name != NULL);
		
		font_desc = pango_font_description_from_string (font_name);
		g_return_if_fail (font_desc != NULL);

		gtk_widget_modify_font (GTK_WIDGET (view), font_desc);
		
		pango_font_description_free (font_desc);		
	}
	else
	{
		GtkRcStyle *rc_style;

		rc_style = gtk_widget_get_modifier_style (GTK_WIDGET (view));

		if (rc_style->font_desc)
			pango_font_description_free (rc_style->font_desc);

		rc_style->font_desc = NULL;
		
		gtk_widget_modify_style (GTK_WIDGET (view), rc_style);
	}
}
