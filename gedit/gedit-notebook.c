/*
 * gedit-notebook.h
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

/* This file is a modified version of the epiphany file ephy-notebook.c
 * Here the relevant copyright:
 *
 *  Copyright (C) 2002 Christophe Fergeau
 *  Copyright (C) 2003 Marco Pesenti Gritti
 *  Copyright (C) 2003, 2004 Christian Persch
 *
 */
 
#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include "gedit-notebook.h"
#include "gedit-marshal.h"
#include "gedit-window.h"
#include "gedit-tooltips.h"

#include <glib-object.h>
#include <gtk/gtkeventbox.h>
#include <gtk/gtknotebook.h>
#include <gtk/gtkhbox.h>
#include <gtk/gtklabel.h>
#include <gtk/gtkwidget.h>
#include <gtk/gtktooltips.h>
#include <gtk/gtkmain.h>
#include <gtk/gtkstock.h>
#include <gtk/gtkimage.h>
#include <gtk/gtkbutton.h>
#include <gtk/gtkiconfactory.h>
#include <glib/gi18n.h>
#include <libgnomevfs/gnome-vfs-uri.h>

#define TAB_WIDTH_N_CHARS 15

#define AFTER_ALL_TABS -1
#define NOT_IN_APP_WINDOWS -2

#define INSANE_NUMBER_OF_URLS 20

#define GEDIT_NOTEBOOK_GET_PRIVATE(object)(G_TYPE_INSTANCE_GET_PRIVATE ((object), GEDIT_TYPE_NOTEBOOK, GeditNotebookPrivate))

struct _GeditNotebookPrivate
{
	GList         *focused_pages;
	GeditTooltips *title_tips;
	gulong         motion_notify_handler_id;
	gint           x_start;
	gint           y_start;
	gboolean       drag_in_progress;
	gboolean       always_show_tabs;
};

static void gedit_notebook_init           (GeditNotebook      *notebook);
static void gedit_notebook_class_init     (GeditNotebookClass *klass);
static void gedit_notebook_finalize       (GObject            *object);
static void move_current_tab_to_another_notebook  (
					   GeditNotebook      *src,
			                   GeditNotebook      *dest,
					   GdkEventMotion     *event,
					   gint                dest_position);

/* Local variables */
static GdkCursor *cursor = NULL;

#if 0 /* CHECK */
static GtkTargetEntry url_drag_types [] = 
{
        { EPHY_DND_URI_LIST_TYPE,   0, 0 },
        { EPHY_DND_URL_TYPE,        0, 1 }
};
static guint n_url_drag_types = G_N_ELEMENTS (url_drag_types);
#endif

/* Signals */
enum
{
	TAB_ADDED,
	TAB_REMOVED,
	TABS_REORDERED,
	TAB_DETACHED,
	TAB_DELETE,
	LAST_SIGNAL
};

static guint signals[LAST_SIGNAL] = { 0 };

static GtkNotebookClass *parent_class = NULL;

GType
gedit_notebook_get_type (void)
{
	static GType type = 0;

	if (G_UNLIKELY (type == 0))
	{
		static const GTypeInfo our_info =
		{
			sizeof (GeditNotebookClass),
			NULL, /* base_init */
			NULL, /* base_finalize */
			(GClassInitFunc) gedit_notebook_class_init,
			NULL,
			NULL, /* class_data */
			sizeof (GeditNotebook),
			0, /* n_preallocs */
			(GInstanceInitFunc) gedit_notebook_init
		};
			

		type = g_type_register_static (GTK_TYPE_NOTEBOOK,
					       "GeditNotebook",
					       &our_info, 0);
	}

	return type;
}

static void
gedit_notebook_class_init (GeditNotebookClass *klass)
{
	GObjectClass *object_class = G_OBJECT_CLASS (klass);

	parent_class = g_type_class_peek_parent (klass);

	object_class->finalize = gedit_notebook_finalize;

	signals[TAB_ADDED] =
		g_signal_new ("tab_added",
			      G_OBJECT_CLASS_TYPE (object_class),
			      G_SIGNAL_RUN_FIRST,
			      G_STRUCT_OFFSET (GeditNotebookClass, tab_added),
			      NULL, NULL,
			      g_cclosure_marshal_VOID__OBJECT,
			      G_TYPE_NONE,
			      1,
			      GEDIT_TYPE_TAB);
	signals[TAB_REMOVED] =
		g_signal_new ("tab_removed",
			      G_OBJECT_CLASS_TYPE (object_class),
			      G_SIGNAL_RUN_FIRST,
			      G_STRUCT_OFFSET (GeditNotebookClass, tab_removed),
			      NULL, NULL,
			      g_cclosure_marshal_VOID__OBJECT,
			      G_TYPE_NONE,
			      1,
			      GEDIT_TYPE_TAB);
	signals[TAB_DETACHED] =
		g_signal_new ("tab_detached",
			      G_OBJECT_CLASS_TYPE (object_class),
			      G_SIGNAL_RUN_FIRST,
			      G_STRUCT_OFFSET (GeditNotebookClass, tab_detached),
			      NULL, NULL,
			      g_cclosure_marshal_VOID__OBJECT,
			      G_TYPE_NONE,
			      1,
			      GEDIT_TYPE_TAB);
	signals[TABS_REORDERED] =
		g_signal_new ("tabs_reordered",
			      G_OBJECT_CLASS_TYPE (object_class),
			      G_SIGNAL_RUN_FIRST,
			      G_STRUCT_OFFSET (GeditNotebookClass, tabs_reordered),
			      NULL, NULL,
			      g_cclosure_marshal_VOID__VOID,
			      G_TYPE_NONE,
			      0);
	signals[TAB_DELETE] =
		g_signal_new ("tab_delete",
			      G_OBJECT_CLASS_TYPE (object_class),
			      G_SIGNAL_RUN_LAST,
			      G_STRUCT_OFFSET (GeditNotebookClass, tab_delete),
			      g_signal_accumulator_true_handled, NULL,
			      gedit_marshal_BOOLEAN__OBJECT,
			      G_TYPE_BOOLEAN,
			      1,
			      GEDIT_TYPE_TAB);

	g_type_class_add_private (object_class, sizeof(GeditNotebookPrivate));
}

static GeditNotebook *
find_notebook_at_pointer (gint abs_x, gint abs_y)
{
	GdkWindow *win_at_pointer;
	GdkWindow *toplevel_win;
	gpointer toplevel = NULL;
	gint x, y;

	/* FIXME multi-head */
	win_at_pointer = gdk_window_at_pointer (&x, &y);
	if (win_at_pointer == NULL)
	{
		/* We are outside all windows of the same application */
		return NULL;
	}

	toplevel_win = gdk_window_get_toplevel (win_at_pointer);

	/* get the GtkWidget which owns the toplevel GdkWindow */
	gdk_window_get_user_data (toplevel_win, &toplevel);

	/* toplevel should be an GeditWindow */
	if ((toplevel != NULL) && 
	    GEDIT_IS_WINDOW (toplevel))
	{
		return GEDIT_NOTEBOOK (_gedit_window_get_notebook
						(GEDIT_WINDOW (toplevel)));
	}

	/* We are outside all windows containing a notebook */
	return NULL;
}

static gboolean
is_in_notebook_window (GeditNotebook *notebook,
		       gint           abs_x, 
		       gint           abs_y)
{
	GeditNotebook *nb_at_pointer;
	
	g_return_val_if_fail (notebook != NULL, FALSE);

	nb_at_pointer = find_notebook_at_pointer (abs_x, abs_y);

	return (nb_at_pointer == notebook);
}

static gint
find_tab_num_at_pos (GeditNotebook *notebook, 
		     gint           abs_x, 
		     gint           abs_y)
{
	GtkPositionType tab_pos;
	int page_num = 0;
	GtkNotebook *nb = GTK_NOTEBOOK (notebook);
	GtkWidget *page;

	tab_pos = gtk_notebook_get_tab_pos (GTK_NOTEBOOK (notebook));

	if (GTK_NOTEBOOK (notebook)->first_tab == NULL)
	{
		return AFTER_ALL_TABS;
	}

	/* For some reason unfullscreen + quick click can
	   cause a wrong click event to be reported to the tab */
	if (!is_in_notebook_window (notebook, abs_x, abs_y))
	{
		return NOT_IN_APP_WINDOWS;
	}

	while ((page = gtk_notebook_get_nth_page (nb, page_num)) != NULL)
	{
		GtkWidget *tab;
		gint max_x, max_y;
		gint x_root, y_root;

		tab = gtk_notebook_get_tab_label (nb, page);
		g_return_val_if_fail (tab != NULL, AFTER_ALL_TABS);

		if (!GTK_WIDGET_MAPPED (GTK_WIDGET (tab)))
		{
			++page_num;
			continue;
		}

		gdk_window_get_origin (GDK_WINDOW (tab->window),
				       &x_root, &y_root);

		max_x = x_root + tab->allocation.x + tab->allocation.width;
		max_y = y_root + tab->allocation.y + tab->allocation.height;

		if (((tab_pos == GTK_POS_TOP) || 
		     (tab_pos == GTK_POS_BOTTOM)) &&
		    (abs_x <= max_x))
		{
			return page_num;
		}
		else if (((tab_pos == GTK_POS_LEFT) || 
		          (tab_pos == GTK_POS_RIGHT)) && 
		         (abs_y <= max_y))
		{
			return page_num;
		}

		++page_num;
	}
	
	return AFTER_ALL_TABS;
}

static gint 
find_notebook_and_tab_at_pos (gint            abs_x, 
			      gint            abs_y,
			      GeditNotebook **notebook,
			      gint           *page_num)
{
	*notebook = find_notebook_at_pointer (abs_x, abs_y);
	if (*notebook == NULL)
	{
		return NOT_IN_APP_WINDOWS;
	}
	
	*page_num = find_tab_num_at_pos (*notebook, abs_x, abs_y);

	if (*page_num < 0)
	{
		return *page_num;
	}
	else
	{
		return 0;
	}
}

/* If dest_position is greater than or equal to the number of tabs 
   of the destination nootebook or negative, tab will be moved to the 
   end of the tabs. */
void
gedit_notebook_move_tab (GeditNotebook *src,
			 GeditNotebook *dest,
			 GeditTab      *tab,
			 gint           dest_position)
{
	g_return_if_fail (GEDIT_IS_NOTEBOOK (src));	
	g_return_if_fail (GEDIT_IS_NOTEBOOK (dest));
	g_return_if_fail (src != dest);
	g_return_if_fail (GEDIT_IS_TAB (tab));

	/* make sure the tab isn't destroyed while we move it */
	g_object_ref (tab);
	gedit_notebook_remove_tab (src, tab);
	gedit_notebook_add_tab (dest, tab, dest_position, TRUE);
	g_object_unref (tab);
}

/* If dest_position is greater than or equal to the number of tabs 
   of the destination nootebook or negative, tab will be moved to the 
   end of the tabs. */
void
gedit_notebook_reorder_tab (GeditNotebook *src,
			    GeditTab      *tab,
			    gint           dest_position)
{
	g_return_if_fail (GEDIT_IS_NOTEBOOK (src));	
	g_return_if_fail (GEDIT_IS_TAB (tab));

	gtk_notebook_reorder_child (GTK_NOTEBOOK (src), 
					    GTK_WIDGET (tab), 
					    dest_position);
		
	if (!src->priv->drag_in_progress)
	{
		g_signal_emit (G_OBJECT (src), 
			       signals[TABS_REORDERED], 
			       0);
	}
}

static void
drag_start (GeditNotebook *notebook,
	    guint32        time)
{
	notebook->priv->drag_in_progress = TRUE;

	/* get a new cursor, if necessary */
	/* FIXME multi-head */
	if (!cursor) cursor = gdk_cursor_new (GDK_FLEUR);

	/* grab the pointer */
	gtk_grab_add (GTK_WIDGET (notebook));
	/* FIXME multi-head */
	if (!gdk_pointer_is_grabbed ())
	{
		gdk_pointer_grab (GTK_WIDGET (notebook)->window,
				  FALSE,
				  GDK_BUTTON1_MOTION_MASK | GDK_BUTTON_RELEASE_MASK,
				  NULL, 
				  cursor, 
				  time);
	}
}

static void
drag_stop (GeditNotebook *notebook)
{
	if (notebook->priv->drag_in_progress)
	{
		g_signal_emit (G_OBJECT (notebook), 
			       signals[TABS_REORDERED], 
			       0);
	}

	notebook->priv->drag_in_progress = FALSE;
	if (notebook->priv->motion_notify_handler_id != 0)
	{
		g_signal_handler_disconnect (G_OBJECT (notebook),
					     notebook->priv->motion_notify_handler_id);
		notebook->priv->motion_notify_handler_id = 0;
	}
}

/* This function is only called during dnd, we don't need to emit TABS_REORDERED
 * here, instead we do it on drag_stop
 */
static void
move_current_tab (GeditNotebook *notebook,
	          gint           dest_position)
{
	gint cur_page_num;

	cur_page_num = gtk_notebook_get_current_page (GTK_NOTEBOOK (notebook));

	if (dest_position != cur_page_num)
	{
		GtkWidget *cur_tab;
		
		cur_tab = gtk_notebook_get_nth_page (GTK_NOTEBOOK (notebook),
						     cur_page_num);
						     
		gedit_notebook_reorder_tab (GEDIT_NOTEBOOK (notebook),
					    GEDIT_TAB (cur_tab),
					    dest_position);
	}
}

static gboolean
motion_notify_cb (GeditNotebook  *notebook,
		  GdkEventMotion *event,
		  gpointer        data)
{
	GeditNotebook *dest;
	gint page_num;
	gint result;

	if (notebook->priv->drag_in_progress == FALSE)
	{
		if (gtk_drag_check_threshold (GTK_WIDGET (notebook),
					      notebook->priv->x_start,
					      notebook->priv->y_start,
					      event->x_root, 
					      event->y_root))
		{
			drag_start (notebook, event->time);
			return TRUE;
		}

		return FALSE;
	}

	result = find_notebook_and_tab_at_pos ((gint)event->x_root,
					       (gint)event->y_root,
					       &dest, 
					       &page_num);

	if (result != NOT_IN_APP_WINDOWS)
	{
		if (dest != notebook)
		{
			move_current_tab_to_another_notebook (notebook, 
							      dest,
						      	      event, 
						      	      page_num);
		}
		else
		{
			g_return_val_if_fail (page_num >= -1, FALSE);
			move_current_tab (notebook, page_num);
		}
	}

	return FALSE;
}

static void
move_current_tab_to_another_notebook (GeditNotebook  *src,
				      GeditNotebook  *dest,
				      GdkEventMotion *event,
				      gint            dest_position)
{
	GeditTab *tab;
	gint cur_page;

	/* This is getting tricky, the tab was dragged in a notebook
	 * in another window of the same app, we move the tab
	 * to that new notebook, and let this notebook handle the
	 * drag
	*/
	g_return_if_fail (GEDIT_IS_NOTEBOOK (dest));
	g_return_if_fail (dest != src);

	cur_page = gtk_notebook_get_current_page (GTK_NOTEBOOK (src));
	tab = GEDIT_TAB (gtk_notebook_get_nth_page (GTK_NOTEBOOK (src), 
						    cur_page));

	/* stop drag in origin window */
	/* ungrab the pointer if it's grabbed */
	drag_stop (src);
	if (gdk_pointer_is_grabbed ())
	{
		gdk_pointer_ungrab (event->time);
	}
	gtk_grab_remove (GTK_WIDGET (src));

	gedit_notebook_move_tab (src, dest, tab, dest_position);

	/* start drag handling in dest notebook */
	dest->priv->motion_notify_handler_id =
		g_signal_connect (G_OBJECT (dest),
				  "motion-notify-event",
				  G_CALLBACK (motion_notify_cb),
				  NULL);

	drag_start (dest, event->time);
}

static gboolean
button_release_cb (GeditNotebook  *notebook,
		   GdkEventButton *event,
		   gpointer        data)
{
	if (notebook->priv->drag_in_progress)
	{
		gint cur_page_num;
		GtkWidget *cur_page;

		cur_page_num =
			gtk_notebook_get_current_page (GTK_NOTEBOOK (notebook));
		cur_page = gtk_notebook_get_nth_page (GTK_NOTEBOOK (notebook),
						      cur_page_num);

		/* CHECK: I don't follow the code here -- Paolo  */
		if (!is_in_notebook_window (notebook, event->x_root, event->y_root) &&
		    (gtk_notebook_get_n_pages (GTK_NOTEBOOK (notebook)) > 1))
		{
			/* Tab was detached */
			g_signal_emit (G_OBJECT (notebook),
				       signals[TAB_DETACHED], 
				       0, 
				       cur_page);
		}

		/* ungrab the pointer if it's grabbed */
		if (gdk_pointer_is_grabbed ())
		{
			gdk_pointer_ungrab (event->time);
		}
		gtk_grab_remove (GTK_WIDGET (notebook));
	}

	/* This must be called even if a drag isn't happening */
	drag_stop (notebook);

	return FALSE;
}

static gboolean
button_press_cb (GeditNotebook  *notebook,
		 GdkEventButton *event,
		 gpointer        data)
{
	gint tab_clicked;

	if (notebook->priv->drag_in_progress)
		return TRUE;

	tab_clicked = find_tab_num_at_pos (notebook,
					   event->x_root,
					   event->y_root);
					   
	if ((event->button == 1) && 
	    (event->type == GDK_BUTTON_PRESS) && 
	    (tab_clicked >= 0))
	{
		notebook->priv->x_start = event->x_root;
		notebook->priv->y_start = event->y_root;
		
		notebook->priv->motion_notify_handler_id =
			g_signal_connect (G_OBJECT (notebook),
					  "motion-notify-event",
					  G_CALLBACK (motion_notify_cb), 
					  NULL);
	}
	else if ((event->type == GDK_BUTTON_PRESS) && 
		 (event->button == 3))
	{
		if (tab_clicked == -1)
		{
			// CHECK: do we really need it?
			
			/* consume event, so that we don't pop up the context menu when
			 * the mouse if not over a tab label
			 */
			return TRUE;
		}
		else
		{
			/* Switch to the page the mouse is over, but don't consume the event */
			gtk_notebook_set_current_page (GTK_NOTEBOOK (notebook), 
						       tab_clicked);
		}
	}

	return FALSE;
}

GtkWidget *
gedit_notebook_new (void)
{
	return GTK_WIDGET (g_object_new (GEDIT_TYPE_NOTEBOOK, NULL));
}

static void
ephy_notebook_switch_page_cb (GtkNotebook     *notebook,
                              GtkNotebookPage *page,
                              guint            page_num,
                              gpointer         data)
{
	GeditNotebook *nb = GEDIT_NOTEBOOK (notebook);
	GtkWidget *child;

	child = gtk_notebook_get_nth_page (notebook, page_num);

	/* Remove the old page, we dont want to grow unnecessarily
	 * the list */
	if (nb->priv->focused_pages)
	{
		nb->priv->focused_pages =
			g_list_remove (nb->priv->focused_pages, child);
	}

	nb->priv->focused_pages = g_list_append (nb->priv->focused_pages,
						 child);
}

#if 0
static void
notebook_drag_data_received_cb (GtkWidget        *widget, 
				GdkDragContext   *context,
				gint              x, 
				gint              y, 
				GtkSelectionData *selection_data,
				guint             info, 
				guint             time, 
				GeditTab         *tab)
{
	EphyWindow *window;
	GtkWidget *notebook;

	g_signal_stop_emission_by_name (widget, "drag_data_received");

	if (eel_gconf_get_boolean (CONF_LOCKDOWN_DISABLE_ARBITRARY_URL)) return;

	if (selection_data->length <= 0 || selection_data->data == NULL) return;

	window = EPHY_WINDOW (gtk_widget_get_toplevel (widget));
	notebook = ephy_window_get_notebook (window);

	if (selection_data->target == gdk_atom_intern (EPHY_DND_URL_TYPE, FALSE))
	{
		char **split;

		/* URL_TYPE has format: url \n title */
		split = g_strsplit (selection_data->data, "\n", 2);
		if (split != NULL && split[0] != NULL && split[0][0] != '\0')
		{
			ephy_link_open (EPHY_LINK (notebook), split[0], tab,
					tab ? 0 : EPHY_LINK_NEW_TAB);
		}
		g_strfreev (split);
	}
	else if (selection_data->target == gdk_atom_intern (EPHY_DND_URI_LIST_TYPE, FALSE))
	{
		char **uris;
		int i;

		uris = gtk_selection_data_get_uris (selection_data);
		if (uris == NULL) return;

		for (i = 0; uris[i] != NULL && i < INSANE_NUMBER_OF_URLS; i++)
		{
			tab = ephy_link_open (EPHY_LINK (notebook), uris[i],
					      tab, i == 0 ? 0 : EPHY_LINK_NEW_TAB);
		}

		g_strfreev (uris);
	}
	else
	{
		g_return_if_reached ();
	}
}
#endif

/*
 * update_tabs_visibility: Hide tabs if there is only one tab
 * and the pref is not set.
 */
static void
update_tabs_visibility (GeditNotebook *nb, 
			gboolean       before_inserting)
{
	gboolean show_tabs;
	guint num;

	num = gtk_notebook_get_n_pages (GTK_NOTEBOOK (nb));

	if (before_inserting) num++;

	show_tabs = (nb->priv->always_show_tabs || num > 1);

	gtk_notebook_set_show_tabs (GTK_NOTEBOOK (nb), show_tabs);
}

static void
gedit_notebook_init (GeditNotebook *notebook)
{
	notebook->priv = GEDIT_NOTEBOOK_GET_PRIVATE (notebook);

	gtk_notebook_set_scrollable (GTK_NOTEBOOK (notebook), TRUE);
	gtk_notebook_set_show_border (GTK_NOTEBOOK (notebook), FALSE);
	gtk_notebook_set_show_tabs (GTK_NOTEBOOK (notebook), FALSE);

	notebook->priv->title_tips = gedit_tooltips_new ();
	g_object_ref (G_OBJECT (notebook->priv->title_tips));
	gtk_object_sink (GTK_OBJECT (notebook->priv->title_tips));

	notebook->priv->always_show_tabs = TRUE;

	g_signal_connect (notebook, 
			  "button-press-event",
			  (GCallback)button_press_cb, 
			  NULL);
	g_signal_connect (notebook, 
			  "button-release-event",
			  (GCallback)button_release_cb,
			  NULL);
	gtk_widget_add_events (GTK_WIDGET (notebook), 
			       GDK_BUTTON1_MOTION_MASK);

	g_signal_connect_after (G_OBJECT (notebook), 
				"switch_page",
                                G_CALLBACK (ephy_notebook_switch_page_cb),
                                NULL);
#if 0
	/* Set up drag-and-drop target */
	g_signal_connect (G_OBJECT(notebook), "drag_data_received",
			  G_CALLBACK(notebook_drag_data_received_cb),
			  NULL);
        gtk_drag_dest_set (GTK_WIDGET(notebook), GTK_DEST_DEFAULT_MOTION |
			   GTK_DEST_DEFAULT_DROP,
                           url_drag_types,n_url_drag_types,
                           GDK_ACTION_MOVE | GDK_ACTION_COPY);
#endif
}

static void
gedit_notebook_finalize (GObject *object)
{
	GeditNotebook *notebook = GEDIT_NOTEBOOK (object);

	if (notebook->priv->focused_pages)
		g_list_free (notebook->priv->focused_pages);
		
	g_object_unref (notebook->priv->title_tips);

	G_OBJECT_CLASS (parent_class)->finalize (object);
}


static void
sync_load_status (GeditTab *tab, GParamSpec *pspec, GtkWidget *proxy)
{
#if 0
	GtkWidget *spinner, *icon;

	spinner = GTK_WIDGET (g_object_get_data (G_OBJECT (proxy), "spinner"));
	icon = GTK_WIDGET (g_object_get_data (G_OBJECT (proxy), "icon"));
	g_return_if_fail (spinner != NULL && icon != NULL);

	if (ephy_tab_get_load_status (tab))
	{
		gtk_widget_hide (icon);
		gtk_widget_show (spinner);
		ephy_spinner_start (EPHY_SPINNER (spinner));
	}
	else
	{
		ephy_spinner_stop (EPHY_SPINNER (spinner));
		gtk_widget_hide (spinner);
		gtk_widget_show (icon);
	}
#endif
}

static void
sync_icon (GeditTab *tab, GParamSpec *pspec, GtkWidget *proxy)
{
#if 0
	EphyFaviconCache *cache;
	GdkPixbuf *pixbuf = NULL;
	GtkImage *icon = NULL;
	const char *address;

	cache = EPHY_FAVICON_CACHE
		(ephy_embed_shell_get_favicon_cache (EPHY_EMBED_SHELL (ephy_shell)));
	address = ephy_tab_get_icon_address (tab);

	if (address)
	{
		pixbuf = ephy_favicon_cache_get (cache, address);
	}

	icon = GTK_IMAGE (g_object_get_data (G_OBJECT (proxy), "icon"));
	if (icon)
	{
		gtk_image_set_from_pixbuf (icon, pixbuf);
	}

	if (pixbuf)
	{
		g_object_unref (pixbuf);
	}
#endif 
}

static void
sync_name (GeditTab *tab, GParamSpec *pspec, GtkWidget *hbox)
{
	GtkWidget *label;
	GtkWidget *ebox;
	GeditTooltips *tips;	
	gchar *str;
	GtkImage *icon;
	GdkPixbuf *pixbuf;
	
	tips = GEDIT_TOOLTIPS (g_object_get_data (G_OBJECT (hbox), "tooltips"));
	label = GTK_WIDGET (g_object_get_data (G_OBJECT (hbox), "label"));
	ebox = GTK_WIDGET (g_object_get_data (G_OBJECT (hbox), "label-ebox"));
	icon = GTK_IMAGE (g_object_get_data (G_OBJECT (hbox), "icon"));
	
	g_return_if_fail ((tips != NULL) && (label != NULL) && (ebox != NULL));

	str = _gedit_tab_get_name (tab);
	g_return_if_fail (str != NULL);
	
	gtk_label_set_text (GTK_LABEL (label), str);
	g_free (str);
	
	str = _gedit_tab_get_tooltips (tab);
	g_return_if_fail (str != NULL);
	
	gedit_tooltips_set_tip (tips, ebox, str, NULL);
	g_free (str);
	
	g_return_if_fail (icon != NULL);
	
	pixbuf = _gedit_tab_get_icon (tab);
	gtk_image_set_from_pixbuf (icon, pixbuf);

	if (pixbuf)
		g_object_unref (pixbuf);
}

static void
close_button_clicked_cb (GtkWidget *widget, GtkWidget *tab)
{
	GeditNotebook *notebook;
	gboolean can_close = TRUE;

	notebook = GEDIT_NOTEBOOK (gtk_widget_get_parent (tab));
	g_signal_emit (G_OBJECT (notebook), signals[TAB_DELETE], 0, tab, &can_close);

	if (can_close)
		gedit_notebook_remove_tab (notebook, GEDIT_TAB (tab));
}

static GtkWidget *
build_tab_label (GeditNotebook *nb, 
		 GeditTab      *tab)
{
	GtkWidget *hbox, *label_hbox, *label_ebox;
	GtkWidget *label, *dummy_label;
	GtkWidget *close_button;
	GtkSettings *settings;
	gint w, h;
	GtkWidget *image;
	// GtkWidget *spinner;
	GtkWidget *icon;

	hbox = gtk_hbox_new (FALSE, 0);

	label_ebox = gtk_event_box_new ();
	gtk_event_box_set_visible_window (GTK_EVENT_BOX (label_ebox), FALSE);
	gtk_box_pack_start (GTK_BOX (hbox), label_ebox, TRUE, TRUE, 0);

	label_hbox = gtk_hbox_new (FALSE, 0);
	gtk_container_add (GTK_CONTAINER (label_ebox), label_hbox);

	/* setup close button */
	close_button = gtk_button_new ();
	gtk_button_set_relief (GTK_BUTTON (close_button),
			       GTK_RELIEF_NONE);
	/* don't allow focus on the close button */
	gtk_button_set_focus_on_click (GTK_BUTTON (close_button), FALSE);

	/* fetch the size of an icon */
	settings = gtk_widget_get_settings (GTK_WIDGET (tab));
	gtk_icon_size_lookup_for_settings (settings,
					   GTK_ICON_SIZE_MENU,
					   &w, &h);
	image = gtk_image_new_from_stock (GTK_STOCK_CLOSE, GTK_ICON_SIZE_MENU);
	gtk_widget_set_size_request (close_button, w + 2, h + 2);
	gtk_container_add (GTK_CONTAINER (close_button), image);
	gtk_box_pack_start (GTK_BOX (hbox), close_button, FALSE, FALSE, 0);

	gedit_tooltips_set_tip (nb->priv->title_tips, close_button,
			      _("Close document"), NULL);

	g_signal_connect (G_OBJECT (close_button), "clicked",
                          G_CALLBACK (close_button_clicked_cb),
                          tab);
#if 0
	/* setup load feedback */
	spinner = ephy_spinner_new ();
	ephy_spinner_set_size (EPHY_SPINNER (spinner), GTK_ICON_SIZE_MENU);
	gtk_box_pack_start (GTK_BOX (label_hbox), spinner, FALSE, FALSE, 0);
#endif
	/* setup site icon, empty by default */
	icon = gtk_image_new ();
	gtk_box_pack_start (GTK_BOX (label_hbox), icon, FALSE, FALSE, 0);
	
	/* setup label */
        label = gtk_label_new ("");
	gtk_misc_set_alignment (GTK_MISC (label), 0.0, 0.5);
        gtk_misc_set_padding (GTK_MISC (label), 4, 0);
	gtk_box_pack_start (GTK_BOX (label_hbox), label, FALSE, FALSE, 0);

	dummy_label = gtk_label_new ("");
	gtk_box_pack_start (GTK_BOX (label_hbox), dummy_label, TRUE, TRUE, 0);
	
	gtk_widget_show (hbox);
	gtk_widget_show (label_ebox);
	gtk_widget_show (label_hbox);
	gtk_widget_show (label);
	gtk_widget_show (dummy_label);	
	gtk_widget_show (image);
	gtk_widget_show (close_button);
	gtk_widget_show (icon);
	
	g_object_set_data (G_OBJECT (hbox), "label", label);
	g_object_set_data (G_OBJECT (hbox), "label-ebox", label_ebox);
//	g_object_set_data (G_OBJECT (hbox), "spinner", spinner);
	g_object_set_data (G_OBJECT (hbox), "icon", icon);
	g_object_set_data (G_OBJECT (hbox), "close-button", close_button);
	g_object_set_data (G_OBJECT (hbox), "tooltips", nb->priv->title_tips);

	return hbox;
}

void
gedit_notebook_set_always_show_tabs (GeditNotebook *nb, 
				     gboolean       show_tabs)
{
	g_return_if_fail (GEDIT_NOTEBOOK (nb));
	
	// FIXME: normalizzare show_tabs
	
	nb->priv->always_show_tabs = show_tabs;

	update_tabs_visibility (nb, FALSE);
}

void
gedit_notebook_add_tab (GeditNotebook *nb,
		        GeditTab      *tab,
		        gint           position,
		        gboolean       jump_to)
{
	GtkWidget *label;

	g_return_if_fail (GEDIT_IS_NOTEBOOK (nb));
	g_return_if_fail (GEDIT_IS_TAB (tab));

	label = build_tab_label (nb, tab);

	update_tabs_visibility (nb, TRUE);

	gtk_notebook_insert_page (GTK_NOTEBOOK (nb), 
				  GTK_WIDGET (tab),
				  label, 
				  position);

#if 0
	/* Set up drag-and-drop target */
	g_signal_connect (G_OBJECT(label), "drag_data_received",
			  G_CALLBACK(notebook_drag_data_received_cb), tab);
	gtk_drag_dest_set (label, GTK_DEST_DEFAULT_ALL,
			   url_drag_types,n_url_drag_types,
			   GDK_ACTION_MOVE | GDK_ACTION_COPY);
#endif
	sync_icon (tab, NULL, label);
	sync_name (tab, NULL, label);
	sync_load_status (tab, NULL, label);
/*
	g_signal_connect_object (tab, 
				 "notify::icon",
			         G_CALLBACK (sync_icon), 
			         label, 
			         0);
*/			         
	g_signal_connect_object (tab, 
				 "notify::name",
			         G_CALLBACK (sync_name), 
			         label, 
			         0);
/*			         
	g_signal_connect_object (tab, 
				 "notify::load-status",
				 G_CALLBACK (sync_load_status), 
				 label, 
				 0);
*/
	g_signal_emit (G_OBJECT (nb), signals[TAB_ADDED], 0, tab);

	/* The signal handler may have reordered the tabs */
	position = gtk_notebook_page_num (GTK_NOTEBOOK (nb), 
					  GTK_WIDGET (tab));

	if (jump_to)
	{
		GeditView *view;
		
		gtk_notebook_set_current_page (GTK_NOTEBOOK (nb), position);
		g_object_set_data (G_OBJECT (tab), 
				   "jump_to",
				   GINT_TO_POINTER (jump_to));
		view = gedit_tab_get_view (tab);
		
		gtk_widget_grab_focus (GTK_WIDGET (view));
	}
}

static void
smart_tab_switching_on_closure (GeditNotebook *nb,
				GeditTab      *tab)
{
	gboolean jump_to;

	jump_to = GPOINTER_TO_INT (g_object_get_data
				   (G_OBJECT (tab), "jump_to"));

	if (!jump_to || !nb->priv->focused_pages)
	{
		gtk_notebook_next_page (GTK_NOTEBOOK (nb));
	}
	else
	{
		GList *l;
		GtkWidget *child;
		int page_num;

		/* activate the last focused tab */
		l = g_list_last (nb->priv->focused_pages);
		child = GTK_WIDGET (l->data);
		page_num = gtk_notebook_page_num (GTK_NOTEBOOK (nb),
						  child);
		gtk_notebook_set_current_page (GTK_NOTEBOOK (nb), 
					       page_num);
	}
}

void
gedit_notebook_remove_tab (GeditNotebook *nb,
			   GeditTab *tab)
{
	int position, curr;
	GtkWidget *label, *ebox;

	g_return_if_fail (GEDIT_IS_NOTEBOOK (nb));
	g_return_if_fail (GEDIT_IS_TAB (tab));

	/* Remove the page from the focused pages list */
	nb->priv->focused_pages =  g_list_remove (nb->priv->focused_pages,
						  tab);

	position = gtk_notebook_page_num (GTK_NOTEBOOK (nb), GTK_WIDGET (tab));
	curr = gtk_notebook_get_current_page (GTK_NOTEBOOK (nb));

	if (position == curr)
	{
		smart_tab_switching_on_closure (nb, tab);
	}

	label = gtk_notebook_get_tab_label (GTK_NOTEBOOK (nb), GTK_WIDGET (tab));
	ebox = GTK_WIDGET (g_object_get_data (G_OBJECT (label), "label-ebox"));
	gedit_tooltips_set_tip (GEDIT_TOOLTIPS (nb->priv->title_tips), 
			        ebox, 
			        NULL, 
			        NULL);
/*
	g_signal_handlers_disconnect_by_func (tab,
					      G_CALLBACK (sync_icon), 
					      label);
*/					      
	g_signal_handlers_disconnect_by_func (tab,
					      G_CALLBACK (sync_name), 
					      label);
/*
	g_signal_handlers_disconnect_by_func (tab,
					      G_CALLBACK (sync_load_status), 
					      label);
*/
	/**
	 * we ref the tab so that it's still alive while the tabs_removed
	 * signal is processed.
	 */
	g_object_ref (tab);

	gtk_notebook_remove_page (GTK_NOTEBOOK (nb), position);

	update_tabs_visibility (nb, FALSE);

	g_signal_emit (G_OBJECT (nb), signals[TAB_REMOVED], 0, tab);

	g_object_unref (tab);
}
