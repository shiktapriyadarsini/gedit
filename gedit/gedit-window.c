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
 */

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <glib/gi18n.h>
 
#include "gedit-window.h"
#include "gedit-notebook.h"
#include "gedit-statusbar.h"

#include <time.h>
#include <sys/types.h>
#include <unistd.h>
 
#define GEDIT_WINDOW_GET_PRIVATE(object)(G_TYPE_INSTANCE_GET_PRIVATE ((object), GEDIT_TYPE_WINDOW, GeditWindowPrivate))

struct _GeditWindowPrivate
{
	GtkWidget      *statusbar;	
	GtkWidget      *notebook;
	
	GtkUIManager   *manager;
        GtkActionGroup *action_group;
};

G_DEFINE_TYPE(GeditWindow, gedit_window, GTK_TYPE_WINDOW)


static void
gedit_window_finalize (GObject *object)
{
	/* GeditWindow *window = GEDIT_WINDOW (object); */

	G_OBJECT_CLASS (gedit_window_parent_class)->finalize (object);
}

static void 
gedit_window_class_init (GeditWindowClass *klass)
{
	GObjectClass *object_class = G_OBJECT_CLASS (klass);

	object_class->finalize = gedit_window_finalize;
	
	g_type_class_add_private (object_class, sizeof(GeditWindowPrivate));
}

/* Menu & Toolbar */

static GtkActionEntry gedit_menu_entries[] = {

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

#if 0
        { "FilePrintSetup", STOCK_PRINT_SETUP, N_("Print Set_up..."), NULL,
          N_("Setup the page settings for printing"),
          G_CALLBACK (window_cmd_file_print_setup) },
        { "FilePrintPreview", GTK_STOCK_PRINT_PREVIEW, N_("Print Pre_view"),"<control><shift>P",
          N_("Print preview"),
          G_CALLBACK (window_cmd_file_print_preview) },
        { "FilePrint", GTK_STOCK_PRINT, N_("_Print..."), "<control>P",
          N_("Print the current page"),
          G_CALLBACK (window_cmd_file_print) },
        { "FileSendTo", STOCK_SEND_MAIL, N_("S_end To..."), "<control>M",
          N_("Send a link of the current page"),
          G_CALLBACK (window_cmd_file_send_to) },
        { "FileCloseTab", GTK_STOCK_CLOSE, N_("_Close"), "<control>W",
          N_("Close this tab"),
          G_CALLBACK (window_cmd_file_close_window) },

        /* Edit menu */
        { "EditUndo", GTK_STOCK_UNDO, N_("_Undo"), "<control>Z",
          N_("Undo the last action"),
          G_CALLBACK (window_cmd_edit_undo) },
        { "EditRedo", GTK_STOCK_REDO, N_("Re_do"), "<shift><control>Z",
          N_("Redo the last undone action"),
          G_CALLBACK (window_cmd_edit_redo) },
        { "EditCut", GTK_STOCK_CUT, N_("Cu_t"), "<control>X",
          N_("Cut the selection"),
          G_CALLBACK (window_cmd_edit_cut) },
        { "EditCopy", GTK_STOCK_COPY, N_("_Copy"), "<control>C",
          N_("Copy the selection"),
          G_CALLBACK (window_cmd_edit_copy) },
        { "EditPaste", GTK_STOCK_PASTE, N_("_Paste"), "<control>V",
          N_("Paste clipboard"),
          G_CALLBACK (window_cmd_edit_paste) },
        { "EditSelectAll", NULL, N_("Select _All"), "<control>A",
          N_("Select the entire page"),
          G_CALLBACK (window_cmd_edit_select_all) },
        { "EditFind", GTK_STOCK_FIND, N_("_Find..."), "<control>F",
          N_("Find a word or phrase in the page"),
          G_CALLBACK (window_cmd_edit_find) },
        { "EditFindNext", NULL, N_("Find Ne_xt"), "<control>G",
          N_("Find next occurrence of the word or phrase"),
          G_CALLBACK (window_cmd_edit_find_next) },
        { "EditFindPrev", NULL, N_("Find Pre_vious"), "<shift><control>G",
          N_("Find previous occurrence of the word or phrase"),
          G_CALLBACK (window_cmd_edit_find_prev) },
        { "EditPersonalData", NULL, N_("P_ersonal Data"), NULL,
          N_("View and remove cookies and passwords"),
          G_CALLBACK (window_cmd_edit_personal_data) },
        { "EditToolbar", NULL, N_("T_oolbars"), NULL,
          N_("Customize toolbars"),
          G_CALLBACK (window_cmd_edit_toolbar) },
        { "EditPrefs", GTK_STOCK_PREFERENCES, N_("P_references"), NULL,
          N_("Configure the web browser"),
          G_CALLBACK (window_cmd_edit_prefs) },

        /* View menu */
        { "ViewStop", GTK_STOCK_STOP, N_("_Stop"), "Escape",
          N_("Stop current data transfer"),
          G_CALLBACK (window_cmd_view_stop) },
        { "ViewReload", GTK_STOCK_REFRESH, N_("_Reload"), "<control>R",
          N_("Display the latest content of the current page"),
          G_CALLBACK (window_cmd_view_reload) },
        { "ViewZoomIn", GTK_STOCK_ZOOM_IN, N_("Zoom _In"), "<control>plus",
          N_("Increase the text size"),
          G_CALLBACK (window_cmd_view_zoom_in) },
        { "ViewZoomOut", GTK_STOCK_ZOOM_OUT, N_("Zoom _Out"), "<control>minus",
          N_("Decrease the text size"),
          G_CALLBACK (window_cmd_view_zoom_out) },
        { "ViewZoomNormal", GTK_STOCK_ZOOM_100, N_("_Normal Size"), "<control>0",
          N_("Use the normal text size"),
          G_CALLBACK (window_cmd_view_zoom_normal) },
        { "ViewEncoding", NULL, N_("Text _Encoding"), NULL,
          N_("Change the text encoding"),
          NULL },
        { "ViewPageSource", STOCK_VIEW_SOURCE, N_("_Page Source"), "<control>U",
          N_("View the source code of the page"),
          G_CALLBACK (window_cmd_view_page_source) },
#endif

        /* Help menu */
        {"HelpContents", GTK_STOCK_HELP, N_("_Contents"), "F1",
         N_("Open the gedit manual"), /* G_CALLBACK (gedut_cmd_help_contents) */ NULL},
        { "HelpAbout", GTK_STOCK_ABOUT, N_("_About"), NULL,
          N_("About this application"), /* G_CALLBACK (gedit_cmd_help_about) */ NULL}
          
};

static guint gedit_n_menu_entries = G_N_ELEMENTS (gedit_menu_entries);

#if 0
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
                                    window->priv->help_message_cid, message);
                g_free (message);
        }
}

static void
menu_item_deselect_cb (GtkMenuItem *proxy,
                       GeditWindow *window)
{
        gtk_statusbar_pop (GTK_STATUSBAR (window->priv->statusbar),
                           window->priv->help_message_cid);
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
#endif

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

	if (!hostname) {
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
create_menu_bar_and_toolbar (GeditWindow *window, 
			     GtkWidget   *main_box)
{
        GtkActionGroup *action_group;
        GtkAction      *action;
        GtkUIManager   *manager;
        GtkWidget      *menubar;
        GtkWidget      *toolbar;
        GError         *error = NULL;

        manager = gtk_ui_manager_new ();
        window->priv->manager = manager;

/* TODO... when we have the statusbar in place */
/*      g_signal_connect (manager, "connect_proxy",
                          G_CALLBACK (connect_proxy_cb), window);
        g_signal_connect (manager, "disconnect_proxy",
                          G_CALLBACK (disconnect_proxy_cb), window);
*/
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

        toolbar = gtk_ui_manager_get_widget (manager, "/ToolBar");
        gtk_box_pack_start (GTK_BOX (main_box), toolbar, FALSE, FALSE, 0);	
}

static void
create_statusbar (GeditWindow *window, 
		  GtkWidget   *main_box)
{
	window->priv->statusbar = gedit_statusbar_new ();

	gtk_box_pack_end (GTK_BOX (main_box),
			  window->priv->statusbar,
			  FALSE, 
			  TRUE, 
			  0);
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

	main_box = gtk_vbox_new (FALSE, 0);
	gtk_container_add (GTK_CONTAINER (window), main_box);

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

	/* FIXME */
	label1 = gtk_label_new ("Side Panel");
  	gtk_paned_pack1 (GTK_PANED (hpaned), label1, TRUE, TRUE);

	vpaned = gtk_vpaned_new ();
  	gtk_paned_pack2 (GTK_PANED (hpaned), vpaned, TRUE, FALSE);

	window->priv->notebook = gedit_notebook_new ();
  	gtk_paned_pack1 (GTK_PANED (vpaned), 
  			 window->priv->notebook,
  			 TRUE, 
  			 TRUE);

	/* FIXME */
	label2 = gtk_label_new ("Bottom Panel");
  	gtk_paned_pack2 (GTK_PANED (vpaned), label2, TRUE, TRUE);

	/* Add status bar */
	create_statusbar (window, main_box);

	if (gtk_window_get_role (GTK_WINDOW (window)) == NULL)
	{
		gchar *role;

		role = gen_role ();
		gtk_window_set_role (GTK_WINDOW (window), role);
		g_free (role);
	}

	/* show the window */
	gtk_widget_show_all (GTK_WIDGET (window));
}

GtkWidget *
gedit_window_new (void)
{
	return GTK_WIDGET (g_object_new (GEDIT_TYPE_WINDOW, NULL));
}

GeditView *
gedit_tag_get_active_view (GeditWindow *window)
{
	g_return_val_if_fail (GEDIT_IS_WINDOW (window), NULL);
	
	// TODO 
	return NULL;
}

GtkWidget *
gedit_window_get_notebook (GeditWindow *window)
{
	g_return_val_if_fail (GEDIT_IS_WINDOW (window), NULL);

	return window->priv->notebook;
}

