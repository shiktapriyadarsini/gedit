/*
 * gedit-window.c
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

#include <time.h>
#include <sys/types.h>
#include <unistd.h>
#include <string.h>

#include <glib/gi18n.h>
 
#include "gedit-window.h"
#include "gedit-notebook.h"
#include "gedit-statusbar.h"
#include "gedit-utils.h"
#include "gedit-commands.h"
#include "gedit-debug.h"
#include "gedit-prefs-manager-app.h"

#define GEDIT_WINDOW_GET_PRIVATE(object)(G_TYPE_INSTANCE_GET_PRIVATE ((object), GEDIT_TYPE_WINDOW, GeditWindowPrivate))

struct _GeditWindowPrivate
{
	GtkWidget      *notebook;

	/* statusbar and context ids for statusbar messages */
	GtkWidget      *statusbar;	
	guint           generic_message_cid;
	guint           tip_message_cid;

	/* Menus & Toolbars */
	GtkUIManager   *manager;
	GtkActionGroup *action_group;
	GtkWidget      *toolbar;
	
	GeditTab       *active_tab;
	gint            num_tabs;
	
	gint            width;
	gint            height;	
	GdkWindowState  state;
};

G_DEFINE_TYPE(GeditWindow, gedit_window, GTK_TYPE_WINDOW)

static void
gedit_window_finalize (GObject *object)
{
	/* GeditWindow *window = GEDIT_WINDOW (object); */

	G_OBJECT_CLASS (gedit_window_parent_class)->finalize (object);
}

static void
gedit_window_destroy (GtkObject *object)
{
	GeditWindow *window;
	
	window = GEDIT_WINDOW (object);
	
	if (gedit_prefs_manager_window_height_can_set ())
		gedit_prefs_manager_set_window_height (window->priv->height);

	if (gedit_prefs_manager_window_width_can_set ())
		gedit_prefs_manager_set_window_width (window->priv->width);

	if (gedit_prefs_manager_window_state_can_set ())
		gedit_prefs_manager_set_window_state (window->priv->state);
	
	GTK_OBJECT_CLASS (gedit_window_parent_class)->destroy (object);
}

static void 
gedit_window_class_init (GeditWindowClass *klass)
{
	GObjectClass *object_class = G_OBJECT_CLASS (klass);
	GtkObjectClass *gobject_class = GTK_OBJECT_CLASS (klass);	

	object_class->finalize = gedit_window_finalize;
	gobject_class->destroy = gedit_window_destroy;
	
	g_type_class_add_private (object_class, sizeof(GeditWindowPrivate));
}

/* Menu & Toolbar */

static GtkActionEntry gedit_menu_entries[] =
{
	/* Toplevel */
	{ "File", NULL, N_("_File") },
	{ "Edit", NULL, N_("_Edit") },
	{ "View", NULL, N_("_View") },
	{ "Search", NULL, N_("_Search") },
	{ "Tools", NULL, N_("_Tools") },
	{ "Documents", NULL, N_("_Documents") },
	{ "Help", NULL, N_("_Help") },

	/* File menu */
	{ "FileNew", GTK_STOCK_NEW, N_("_New"), "<control>N",
	  N_("Create a new document"), /* G_CALLBACK (gedit_cmd_file_new) */ NULL },
	{ "FileOpen", GTK_STOCK_OPEN, N_("_Open..."), "<control>O",
	  N_("Open a file"), /* G_CALLBACK (gedit_cmd_file_open)*/ NULL },
	{ "FileOpenURI", NULL, N_("Open _Location..."), "<control>L",
	  N_("Open a file from a specified location"), /* G_CALLBACK (gedit_cmd_file_open_uri)*/ NULL },
	{ "FileSave", GTK_STOCK_SAVE, N_("Save"), "<control>S",
	  N_("Save the current file"), /* G_CALLBACK (gedit_cmd_file_save) */ NULL},
	{ "FileSaveAs", GTK_STOCK_SAVE_AS, N_("Save _As..."), "<shift><control>S",
	  N_("Save the current file with a different name"), /* G_CALLBACK (gedit_cmd_file_save_as)*/ NULL },
	{ "FileRevert", GTK_STOCK_REVERT_TO_SAVED, N_("_Revert"), NULL,
	  N_("Revert to a saved version of the file"), /* G_CALLBACK (gedit_cmd_file_revert)*/ NULL },
	{ "FilePageSetup", NULL, N_("Page Set_up..."), NULL,
	  N_("Setup the page settings"), /* G_CALLBACK (gedit_cmd_file_page_setup) */ NULL},
	{ "FilePrintPreview", GTK_STOCK_PRINT_PREVIEW, N_("Print Previe_w"),"<control><shift>P",
	  N_("Print preview"), /* G_CALLBACK (window_cmd_file_print_preview) */ NULL },
	 { "FilePrint", GTK_STOCK_PRINT, N_("_Print..."), "<control>P",
	  N_("Print the current page"), /* G_CALLBACK (window_cmd_file_print) */ NULL},
	{ "FileClose", GTK_STOCK_CLOSE, N_("_Close"), "<control>W",
	  N_("Close the current file"), /* G_CALLBACK (window_cmd_file_close_window) */ NULL},
	{ "FileQuit", GTK_STOCK_QUIT, N_("_Quit"), "<control>Q",
	  N_("Quit the program"), /* G_CALLBACK (window_cmd_file_quit) */ NULL},

	/* Edit menu */
	{ "EditUndo", GTK_STOCK_UNDO, N_("_Undo"), "<control>Z",
	  N_("Undo the last action"), /* G_CALLBACK (window_cmd_edit_undo) */ NULL},
	{ "EditRedo", GTK_STOCK_REDO, N_("_Redo"), "<shift><control>Z",
	  N_("Redo the last undone action"), /* G_CALLBACK (window_cmd_edit_redo) */ NULL},
	{ "EditCut", GTK_STOCK_CUT, N_("Cu_t"), "<control>X",
	  N_("Cut the selection"), NULL},
	{ "EditCopy", GTK_STOCK_COPY, N_("_Copy"), "<control>C",
	  N_("Copy the selection"), NULL},
	{ "EditPaste", GTK_STOCK_PASTE, N_("_Paste"), "<control>V",
	  N_("Paste the clipboard"), NULL},
	{ "EditDelete", GTK_STOCK_DELETE, N_("_Delete"), NULL,
	  N_("Delete the selected text"), NULL},
	{ "EditSelectAll", NULL, N_("Select _All"), "<control>A",
	  N_("Select the entire document"), NULL},
	{ "EditPreferences", GTK_STOCK_PREFERENCES, N_("Pr_eferences"), NULL,
	  N_("Configure the application"), NULL},

	/* View menu */
	{ "ViewToolbar", NULL, N_("_Toolbar"), NULL,
	  N_("Show or hide the toolbar in the current window"), NULL},
	{ "ViewStatusbar", NULL, N_("_Statusbar"), NULL,
	  N_("Show or hide the statusbar in the current window"), NULL},
	{ "ViewOutputWindow", NULL, N_("_OutputWindow"), "<control><alt>O",
	  N_("Show or hide the output window in the current window"), NULL},

	/* Search menu */
	{ "SearchFind", GTK_STOCK_FIND, N_("_Find..."), "<control>F",
	  N_("Search for text"), NULL},
	{ "SearchFindNext", NULL, N_("Find Ne_xt"), "<control>G",
	  N_("Search forwards for the same text"), NULL},
	{ "SearchFindPrevious", NULL, N_("Find Pre_vious"), "<shift><control>G",
	  N_("Search backwards for the same text"), NULL},
	{ "SearchReplace", GTK_STOCK_FIND_AND_REPLACE, N_("_Replace..."), "<control>R",
	  N_("Search for and replace text"), NULL},
	{ "SearchGoToLine", GTK_STOCK_JUMP_TO, N_("Go to _Line..."), "<control>I",
	  N_("Go to a specific line"), NULL},

	/* Help menu */
	{"HelpContents", GTK_STOCK_HELP, N_("_Contents"), "F1",
	 N_("Open the gedit manual"), G_CALLBACK (gedit_cmd_help_contents)},
	{ "HelpAbout", GTK_STOCK_ABOUT, N_("_About"), NULL,
	  N_("About this application"), G_CALLBACK (gedit_cmd_help_about)}  
};

static guint gedit_n_menu_entries = G_N_ELEMENTS (gedit_menu_entries);

static void
menu_item_select_cb (GtkMenuItem *proxy,
                     GeditWindow *window)
{
	GtkAction *action;
	char *message;

	action = g_object_get_data (G_OBJECT (proxy),  "gtk-action");
	g_return_if_fail (action != NULL);

	g_object_get (G_OBJECT (action), "tooltip", &message, NULL);
	if (message)
	{
		gtk_statusbar_push (GTK_STATUSBAR (window->priv->statusbar),
				    window->priv->tip_message_cid, message);
		g_free (message);
	}
}

static void
menu_item_deselect_cb (GtkMenuItem *proxy,
                       GeditWindow *window)
{
	gtk_statusbar_pop (GTK_STATUSBAR (window->priv->statusbar),
			   window->priv->tip_message_cid);
}

static void
connect_proxy_cb (GtkUIManager *manager,
                  GtkAction *action,
                  GtkWidget *proxy,
                  GeditWindow *window)
{
	if (GTK_IS_MENU_ITEM (proxy))
	{
		g_signal_connect (proxy, "select",
				  G_CALLBACK (menu_item_select_cb), window);
		g_signal_connect (proxy, "deselect",
				  G_CALLBACK (menu_item_deselect_cb), window);
	}
}

static void
disconnect_proxy_cb (GtkUIManager *manager,
                     GtkAction *action,
                     GtkWidget *proxy,
                     GeditWindow *window)
{
	if (GTK_IS_MENU_ITEM (proxy))
	{
		g_signal_handlers_disconnect_by_func
			(proxy, G_CALLBACK (menu_item_select_cb), window);
		g_signal_handlers_disconnect_by_func
			(proxy, G_CALLBACK (menu_item_deselect_cb), window);
	}
}

static void
create_menu_bar_and_toolbar (GeditWindow *window, 
			     GtkWidget   *main_box)
{
	GtkActionGroup *action_group;
	GtkAction      *action;
	GtkUIManager   *manager;
	GtkWidget      *menubar;
	GError         *error = NULL;

	manager = gtk_ui_manager_new ();
	window->priv->manager = manager;

	g_signal_connect (manager, "connect_proxy",
			  G_CALLBACK (connect_proxy_cb), window);
	g_signal_connect (manager, "disconnect_proxy",
			 G_CALLBACK (disconnect_proxy_cb), window);

	action_group = gtk_action_group_new ("WindowActions");
	gtk_action_group_set_translation_domain (action_group, NULL);
	gtk_action_group_add_actions (action_group,
				      gedit_menu_entries,
				      gedit_n_menu_entries,
				      window);
        /* TODO: add more action groups... toggles etc */

	gtk_ui_manager_insert_action_group (manager, action_group, 0);
	window->priv->action_group = action_group;

	/* set short labels to use in the toolbar */
	action = gtk_action_group_get_action (action_group, "FileSave");
	g_object_set (action, "short_label", _("Save"), NULL);
	/* TODO more */

	/* set which actions are important */
	action = gtk_action_group_get_action (action_group, "FileSave");
	g_object_set (action, "is_important", TRUE, NULL);
	/* TODO more */

	gtk_window_add_accel_group (GTK_WINDOW (window),
				    gtk_ui_manager_get_accel_group (manager));

        /* now load the UI definition */
	gtk_ui_manager_add_ui_from_file (manager, /* GEDIT_UI_DIR */ "gedit-ui.xml", &error);
	if (error != NULL)
	{
		g_warning ("Could not merge gedit-ui.xml: %s", error->message);
		g_error_free (error);
	}

	menubar = gtk_ui_manager_get_widget (manager, "/MenuBar");
	gtk_box_pack_start (GTK_BOX (main_box), menubar, FALSE, FALSE, 0);

	window->priv->toolbar = gtk_ui_manager_get_widget (manager, "/ToolBar");
	gtk_box_pack_start (GTK_BOX (main_box), 
			    window->priv->toolbar, 
			    FALSE, 
			    FALSE, 
			    0);	
}

static void
create_statusbar (GeditWindow *window, 
		  GtkWidget   *main_box)
{
	window->priv->statusbar = gedit_statusbar_new ();

	window->priv->generic_message_cid = gtk_statusbar_get_context_id
		(GTK_STATUSBAR (window->priv->statusbar), "generic_message");
	window->priv->tip_message_cid = gtk_statusbar_get_context_id
		(GTK_STATUSBAR (window->priv->statusbar), "tip_message");

	gtk_box_pack_end (GTK_BOX (main_box),
			  window->priv->statusbar,
			  FALSE, 
			  TRUE, 
			  0);			
}

static GeditWindow *
clone_window (GeditWindow *origin)
{
	GtkWindow *window;
	
	window = GTK_WINDOW (g_object_new (GEDIT_TYPE_WINDOW, NULL));
	
	gtk_window_set_default_size (window, 
				     origin->priv->width,
				     origin->priv->height);
				     
	if ((origin->priv->state & GDK_WINDOW_STATE_MAXIMIZED) != 0)
	{
		gtk_window_set_default_size (window, 
					     gedit_prefs_manager_get_default_window_width (),
					     gedit_prefs_manager_get_default_window_height ());
					     
		gtk_window_maximize (window);
	}
	else
	{
		gtk_window_set_default_size (window, 
				     origin->priv->width,
				     origin->priv->height);

		gtk_window_unmaximize (window);
	}		

	if ((origin->priv->state & GDK_WINDOW_STATE_STICKY ) != 0)
		gtk_window_stick (window);
	else
		gtk_window_unstick (window);

	// FIXME: clone the visibility of panels

	return GEDIT_WINDOW (window);
}

static void
update_cursor_position_statusbar (GtkTextBuffer *buffer, 
				  GeditWindow   *window)
{
	gint row, col;
	GtkTextIter iter;
	GtkTextIter start;
	guint tab_size;
	GeditView *view;

	gedit_debug (DEBUG_MDI, "");
  
 	if (buffer != GTK_TEXT_BUFFER (gedit_window_get_active_document (window)))
 		return;
 		
 	view = gedit_window_get_active_view (window);
 	
	gtk_text_buffer_get_iter_at_mark (buffer,
					  &iter,
					  gtk_text_buffer_get_insert (buffer));
	
	row = gtk_text_iter_get_line (&iter);
	
	start = iter;
	gtk_text_iter_set_line_offset (&start, 0);
	col = 0;

	tab_size = gtk_source_view_get_tabs_width (GTK_SOURCE_VIEW (view));

	while (!gtk_text_iter_equal (&start, &iter))
	{
		/* FIXME: Are we Unicode compliant here? */
		if (gtk_text_iter_get_char (&start) == '\t')
					
			col += (tab_size - (col  % tab_size));
		else
			++col;

		gtk_text_iter_forward_char (&start);
	}
	
	gedit_statusbar_set_cursor_position (
				GEDIT_STATUSBAR (window->priv->statusbar),
				row + 1,
				col + 1);
}

static void
cursor_moved (GtkTextBuffer     *buffer,
	      const GtkTextIter *new_location,
	      GtkTextMark       *mark,
	      GeditWindow       *window)
{
	if (mark == gtk_text_buffer_get_insert (buffer))
		update_cursor_position_statusbar (buffer, window);
}

static void
update_overwrite_mode_statusbar (GtkTextView *view, 
				 GeditWindow *window)
{
	if (view != GTK_TEXT_VIEW (gedit_window_get_active_view (window)))
		return;
		
	/* Note that we have to use !gtk_text_view_get_overwrite since we
	   are in the in the signal handler of "toggle overwrite" that is
	   G_SIGNAL_RUN_LAST
	*/
	gedit_statusbar_set_overwrite (
			GEDIT_STATUSBAR (window->priv->statusbar),
			!gtk_text_view_get_overwrite (view));
}

#define MAX_TITLE_LENGTH 100

static gchar *
get_dirname (const gchar *uri)
{
	gchar *res;
	gchar *str;

	str = g_path_get_dirname (uri);
	g_return_val_if_fail (str != NULL, ".");

	if ((strlen (str) == 1) && (*str == '.'))
	{
		g_free (str);
		
		return NULL;
	}

	res = gedit_utils_replace_home_dir_with_tilde (str);

	g_free (str);
	
	return res;
}

static void 
set_title (GeditWindow *window)
{
	GeditDocument *doc = NULL;
	gchar *short_name;
	gchar *name;
	gchar *dirname = NULL;
	gchar *title = NULL;
	gint len;

	if (window->priv->active_tab == NULL)
	{
		gtk_window_set_title (GTK_WINDOW (window), "gedit");
		return;
	}

	doc = gedit_tab_get_document (window->priv->active_tab);
	g_return_if_fail (doc != NULL);

	short_name = gedit_document_get_short_name (doc);
	g_return_if_fail (short_name != NULL);

	len = g_utf8_strlen (short_name, -1);

	/* if the name is awfully long, truncate it and be done with it,
	 * otherwise also show the directory (ellipsized if needed)
	 */
	if (len > MAX_TITLE_LENGTH)
	{
		name = gedit_utils_str_middle_truncate (short_name, 
							MAX_TITLE_LENGTH);
		g_free (short_name);
	}
	else
	{
		gchar *uri;
		gchar *str;

		name = short_name;

		uri = gedit_document_get_uri (doc);
		g_return_if_fail (uri != NULL);

		str = get_dirname (uri);
		g_free (uri);

		if (str != NULL)
		{
			/* use the remaining space for the dir, but use a min of 20 chars
			 * so that we do not end up with a dirname like "(a...b)".
			 * This means that in the worst case when the filename is long 99
			 * we have a title long 99 + 20, but I think it's a rare enough
			 * case to be acceptable. It's justa darn title afterall :)
			 */
			dirname = gedit_utils_str_middle_truncate (str, 
								   MAX (20, MAX_TITLE_LENGTH - len));
			g_free (str);
		}
	}

	if (gedit_document_get_modified (doc))
	{
		if (dirname != NULL)
			title = g_strdup_printf ("*%s (%s) - gedit", 
						 name, 
						 dirname);
		else
			title = g_strdup_printf ("*%s - gedit", 
						 name);
	} 
	else 
	{
		if (gedit_document_is_readonly (doc)) 
		{
			if (dirname != NULL)
				title = g_strdup_printf ("%s [%s] (%s) - gedit", 
							 name, 
							 _("Read Only"), 
							 dirname);
			else
				title = g_strdup_printf ("%s [%s] - gedit", 
							 name, 
							 _("Read Only"));
		} 
		else 
		{
			if (dirname != NULL)
				title = g_strdup_printf ("%s (%s) - gedit", 
							 name, 
							 dirname);
			else
				title = g_strdup_printf ("%s - gedit", 
							 name);
		}
	}

	gtk_window_set_title (GTK_WINDOW (window), title);

	g_free (dirname);
	g_free (name);
	g_free (title);
}

#undef MAX_TITLE_LENGTH

static void 
notebook_switch_page (GtkNotebook     *book, 
		      GtkNotebookPage *pg,
		      gint             page_num, 
		      GeditWindow     *window)
{
	GeditView *view;
	window->priv->active_tab = GEDIT_TAB (
					gtk_notebook_get_nth_page (book, 
								   page_num));

	set_title (window);
	
	view = gedit_tab_get_view (window->priv->active_tab);
	
	update_cursor_position_statusbar (
			GTK_TEXT_BUFFER (gedit_tab_get_document (window->priv->active_tab)),
			window);
	gedit_statusbar_set_overwrite (
			GEDIT_STATUSBAR (window->priv->statusbar),
			gtk_text_view_get_overwrite (GTK_TEXT_VIEW (view)));
}

static void
sync_name (GeditTab *tab, GParamSpec *pspec, GeditWindow *window)
{
	set_title (window);
}

static void
notebook_tab_added (GeditNotebook *notebook,
		    GeditTab      *tab,
		    GeditWindow   *window)
{
	GeditView *view;
	GeditDocument *doc;
	
	gedit_debug (DEBUG_MDI, "");
	
	++window->priv->num_tabs;
	g_signal_connect (tab, 
			 "notify::name",
			  G_CALLBACK (sync_name), 
			  window);
			  
	view = gedit_tab_get_view (tab);
	doc = gedit_tab_get_document (tab);
	
	/* CHECK: in the old gedit-view we also connected doc "changed" */

	g_signal_connect (doc, 
			  "changed",
			  G_CALLBACK (update_cursor_position_statusbar),
			  window);
	g_signal_connect (doc,
			  "mark_set",/* cursor moved */
			  G_CALLBACK (cursor_moved),
			  window);			  
	g_signal_connect (view,
			  "toggle_overwrite",
			  G_CALLBACK (update_overwrite_mode_statusbar),
			  window);			  
}

static void
notebook_tab_removed (GeditNotebook *notebook,
		      GeditTab      *tab,
		      GeditWindow   *window)
{
	GeditDocument *doc;
	
	gedit_debug (DEBUG_MDI, "");
	
	--window->priv->num_tabs;
	
	doc = gedit_tab_get_document (tab);
	
	g_signal_handlers_disconnect_by_func (tab,
					      G_CALLBACK (sync_name), 
					      window);
	g_signal_handlers_disconnect_by_func (doc,
					      G_CALLBACK (cursor_moved), 
					      window);					
	g_signal_handlers_disconnect_by_func (doc, 
					      G_CALLBACK (update_cursor_position_statusbar),
					      window);
					      
	g_return_if_fail (window->priv->num_tabs >= 0);
	if (window->priv->num_tabs == 0)
	{
		window->priv->active_tab = NULL;
		set_title (window);
		
		/* Remove line and col info */
		gedit_statusbar_set_cursor_position (
				GEDIT_STATUSBAR (window->priv->statusbar),
				-1,
				-1);
				
		gedit_statusbar_clear_overwrite (
				GEDIT_STATUSBAR (window->priv->statusbar));				
	}
}

static void
notebook_tab_detached (GeditNotebook *notebook,
		       GeditTab      *tab,
		       GeditWindow   *window)
{
	GeditWindow *new_window;
	
	new_window = clone_window (window);
		
	gedit_notebook_move_tab (notebook,
				 GEDIT_NOTEBOOK (gedit_window_get_notebook (new_window)),
				 tab, 0);
				 
	gtk_window_set_position (GTK_WINDOW (new_window), 
				 GTK_WIN_POS_MOUSE);
					 
	gtk_widget_show (GTK_WIDGET (new_window));
}		      

static gboolean 
configure_event_handler (GeditWindow *window, GdkEventConfigure *event)
{	
	window->priv->width = event->width;
	window->priv->height = event->height;

	return FALSE;
}

static gboolean 
window_state_event_handler (GeditWindow *window, GdkEventWindowState *event)
{	
	window->priv->state = event->new_window_state;
	
	return FALSE;
}

/* Returns TRUE if status bar is visible */
static gboolean
set_statusbar_style (GeditWindow *window)
{
	gboolean visible;
	
	visible = gedit_prefs_manager_get_statusbar_visible ();
	
	if (visible)
		gtk_widget_show (window->priv->statusbar);
	else
		gtk_widget_hide (window->priv->statusbar);	
		
	// TODO: show overwrite mode, etc. 
		
	return visible;
}

/* Returns TRUE if toolbar is visible */
static gboolean
set_toolbar_style (GeditWindow *window)
{
	gboolean visible;
	GeditToolbarSetting style;
	
	visible = gedit_prefs_manager_get_toolbar_visible ();
	
	/* Set visibility */
	if (visible)
		gtk_widget_show (window->priv->toolbar);
	else
	{
		gtk_widget_hide (window->priv->toolbar);
	
		return FALSE;
	}
	
	/* Set style */
	style = gedit_prefs_manager_get_toolbar_buttons_style ();
	switch (style)
	{
		case GEDIT_TOOLBAR_SYSTEM:
			gedit_debug (DEBUG_MDI, "GEDIT: SYSTEM");
			gtk_toolbar_unset_style (
					GTK_TOOLBAR (window->priv->toolbar));
			break;
			
		case GEDIT_TOOLBAR_ICONS:
			gedit_debug (DEBUG_MDI, "GEDIT: ICONS");
			gtk_toolbar_set_style (
					GTK_TOOLBAR (window->priv->toolbar),
					GTK_TOOLBAR_ICONS);
			break;
			
		case GEDIT_TOOLBAR_ICONS_AND_TEXT:
			gedit_debug (DEBUG_MDI, "GEDIT: ICONS_AND_TEXT");
			gtk_toolbar_set_style (
					GTK_TOOLBAR (window->priv->toolbar),
					GTK_TOOLBAR_BOTH);			
			break;
			
		case GEDIT_TOOLBAR_ICONS_BOTH_HORIZ:
			gedit_debug (DEBUG_MDI, "GEDIT: ICONS_BOTH_HORIZ");
			gtk_toolbar_set_style (
					GTK_TOOLBAR (window->priv->toolbar),
					GTK_TOOLBAR_BOTH_HORIZ);	
			break;       
	}
	
	return visible;
}

/* Generates a unique string for a window role.
 *
 * Taken from EOG.
 */
static gchar *
gen_role (void)
{
        gchar *ret;
	static gchar *hostname;
	time_t t;
	static gint serial;

	t = time (NULL);

	if (!hostname)
	{
		static char buffer [512];

		if ((gethostname (buffer, sizeof (buffer) - 1) == 0) &&
		    (buffer [0] != 0))
			hostname = buffer;
		else
			hostname = "localhost";
	}

	ret = g_strdup_printf ("gedit-window-%d-%d-%d-%ld-%d@%s",
			       getpid (),
			       getgid (),
			       getppid (),
			       (long) t,
			       serial++,
			       hostname);

	return ret;
}

static void
gedit_window_init (GeditWindow *window)
{
	GtkWidget *main_box;
	GtkWidget *hpaned;
	GtkWidget *vpaned;

	/* FIXME */
	GtkWidget *label1;
	GtkWidget *label2;

	window->priv = GEDIT_WINDOW_GET_PRIVATE (window);
	window->priv->active_tab = NULL;
	window->priv->num_tabs = 0;
	
	main_box = gtk_vbox_new (FALSE, 0);
	gtk_container_add (GTK_CONTAINER (window), main_box);
	gtk_widget_show (main_box);

	/* Add menu bar and toolbar bar */
	create_menu_bar_and_toolbar (window, main_box);

	/* Add the main area */
	hpaned = gtk_hpaned_new ();
  	gtk_box_pack_start (GTK_BOX (main_box), 
  			    hpaned, 
  			    TRUE, 
  			    TRUE, 
  			    0);
	gtk_paned_set_position (GTK_PANED (hpaned), 0);
	gtk_widget_show (hpaned);

	/* FIXME */
	label1 = gtk_label_new ("Side Panel");
  	gtk_paned_pack1 (GTK_PANED (hpaned), label1, TRUE, TRUE);
  	gtk_widget_show (label1);

	vpaned = gtk_vpaned_new ();
  	gtk_paned_pack2 (GTK_PANED (hpaned), vpaned, TRUE, FALSE);
  	gtk_widget_show (vpaned);
  	
	window->priv->notebook = gedit_notebook_new ();
  	gtk_paned_pack1 (GTK_PANED (vpaned), 
  			 window->priv->notebook,
  			 TRUE, 
  			 TRUE);
  	gtk_widget_show (window->priv->notebook);  			 

	/* FIXME */
	label2 = gtk_label_new ("Bottom Panel");
  	gtk_paned_pack2 (GTK_PANED (vpaned), label2, TRUE, TRUE);
	gtk_widget_show (label2);
	
	/* Add status bar */
	create_statusbar (window, main_box);

	/* Set the statusbar style according to prefs */
	set_statusbar_style (window);
	
	/* Set the toolbar style according to prefs */
	set_toolbar_style (window);
	
	/* Set visibility of panels */
	// TODO

	if (gtk_window_get_role (GTK_WINDOW (window)) == NULL)
	{
		gchar *role;

		role = gen_role ();
		gtk_window_set_role (GTK_WINDOW (window), role);
		g_free (role);
	}

	/* Connect signals */
	g_signal_connect (G_OBJECT (window->priv->notebook),
			  "switch_page",
			  G_CALLBACK (notebook_switch_page),
			  window);
	g_signal_connect (G_OBJECT (window->priv->notebook),
			  "tab_added",
			  G_CALLBACK (notebook_tab_added),
			  window);
	g_signal_connect (G_OBJECT (window->priv->notebook),
			  "tab_removed",
			  G_CALLBACK (notebook_tab_removed),
			  window);
	g_signal_connect (G_OBJECT (window->priv->notebook),
			  "tab_detached",
			  G_CALLBACK (notebook_tab_detached),
			  window);			  
	g_signal_connect (G_OBJECT (window), 
			  "configure_event",
	                  G_CALLBACK (configure_event_handler), 
	                  NULL);
	g_signal_connect (G_OBJECT (window), 
			  "window_state_event",
	                  G_CALLBACK (window_state_event_handler), 
	                  NULL);			  
}

GtkWidget *
gedit_window_new (void)
{
	GtkWindow *window;
	
	window = GTK_WINDOW (g_object_new (GEDIT_TYPE_WINDOW, NULL));
	
	/* Set window state and size, but only if the session is not being restored */
	// FIXME
	// if (!bonobo_mdi_get_restoring_state (mdi))
	{
		GdkWindowState state;
		
		state = gedit_prefs_manager_get_window_state ();

		if ((state & GDK_WINDOW_STATE_MAXIMIZED) != 0)
		{
			gtk_window_set_default_size (window,
						     gedit_prefs_manager_get_default_window_width (),
						     gedit_prefs_manager_get_default_window_height ());

			gtk_window_maximize (window);
		}
		else
		{
			gtk_window_set_default_size (window, 
						     gedit_prefs_manager_get_window_width (),
						     gedit_prefs_manager_get_window_height ());

			gtk_window_unmaximize (window);
		}

		if ((state & GDK_WINDOW_STATE_STICKY ) != 0)
			gtk_window_stick (window);
		else
			gtk_window_unstick (window);
	}
	
	return GTK_WIDGET (window);
}

GeditView *
gedit_window_get_active_view (GeditWindow *window)
{
	GeditView *view;
	
	g_return_val_if_fail (GEDIT_IS_WINDOW (window), NULL);
	
	if (window->priv->active_tab == NULL)
		return NULL;
		
	view = gedit_tab_get_view (GEDIT_TAB (window->priv->active_tab));
	
	return view;
}

GeditDocument *
gedit_window_get_active_document (GeditWindow *window)
{
	GeditView *view;
	
	g_return_val_if_fail (GEDIT_IS_WINDOW (window), NULL);
	
	view = gedit_window_get_active_view (window);
	if (view == NULL)
		return NULL;
	
	return GEDIT_DOCUMENT (gtk_text_view_get_buffer (GTK_TEXT_VIEW (view)));
}

GtkWidget *
gedit_window_get_notebook (GeditWindow *window)
{
	g_return_val_if_fail (GEDIT_IS_WINDOW (window), NULL);

	return window->priv->notebook;
}

