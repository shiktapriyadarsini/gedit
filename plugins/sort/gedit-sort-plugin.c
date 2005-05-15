/*
 * gedit-sort-plugin.c
 * 
 * Original author: Carlo Borreo <borreo@softhome.net>
 * Ported to Gedit2 by Lee Mallabone <gnome@fonicmonkey.net>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2, or (at your option)
 * any later version.
 * 
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 *
 * $Id$
 */

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include "gedit-sort-plugin.h"

#include <string.h>
#include <glib/gi18n-lib.h>
#include <gmodule.h>
#include <libgnome/gnome-help.h>
#include <glade/glade-xml.h>

#include <gedit/gedit-debug.h>
#include <gedit/gedit-utils.h>

#define GEDIT_SORT_PLUGIN_GET_PRIVATE(object)(G_TYPE_INSTANCE_GET_PRIVATE ((object), GEDIT_TYPE_SORT_PLUGIN, GeditSortPluginPrivate))

/* Key in case the plugin ever needs any settings. */
#define SORT_BASE_KEY "/apps/gedit-2/plugins/sort"

#define PLUGIN_DATA_KEY "GeditSortPluginData"
#define MENU_PATH "/MenuBar/EditMenu/EditOps_6"

GEDIT_PLUGIN_REGISTER_TYPE(GeditSortPlugin, gedit_sort_plugin)

typedef struct
{
	GtkWidget *dialog;
	GtkWidget *col_num_spinbutton;
	GtkWidget *reverse_order_checkbutton;
	GtkWidget *ignore_case_checkbutton;
	GtkWidget *remove_dups_checkbutton;
} SortDialog;

typedef struct
{
	GtkActionGroup *ui_action_group;
	guint ui_id;
	SortDialog *ui_dialog;
	GeditWindow *ui_window;
} PluginData;

typedef struct
{
	gboolean ignore_case;
	gboolean reverse_order;
	gboolean remove_duplicates;
	gint starting_column;
} SortInfo;

static void sort_cb (GtkAction *action, PluginData *data);
static void sort_real (PluginData *data);

/* Menu items. */
static const GtkActionEntry action_entries[] =
{
	{ "Sort",
	  GTK_STOCK_SORT_ASCENDING,
	  N_("S_ort..."),
	  NULL,
	  N_("Sort the current document or selection"),
	  G_CALLBACK (sort_cb) }
};

static void
sort_dialog_destroy (GtkObject  *dialog,
		     PluginData *data)
{
	gedit_debug (DEBUG_PLUGINS);

	if (data != NULL)
	{
		g_free (data->ui_dialog);
		data->ui_dialog = NULL;
	}
}

static void
sort_dialog_response_handler (GtkDialog  *dialog,
			      gint        res_id,
			      PluginData *data)
{
	GError *error = NULL;

	gedit_debug (DEBUG_PLUGINS);

	switch (res_id)
	{
		case GTK_RESPONSE_OK:
			sort_real (data);
			gtk_widget_destroy (data->ui_dialog->dialog);
			
			break;

		case GTK_RESPONSE_HELP:
			gnome_help_display ("gedit.xml",
					    "gedit-sort-plugin",
					    &error);

			if (error != NULL)
			{
				g_warning (error->message);
				g_error_free (error);
			}

			break;

		case GTK_RESPONSE_CANCEL:
			gtk_widget_destroy (data->ui_dialog->dialog);

			break;
	}
}

static SortDialog *
get_sort_dialog (PluginData *data)
{
	GladeXML *gladexml;
	SortDialog *dialog;

	gedit_debug (DEBUG_PLUGINS);

	gladexml = glade_xml_new (GEDIT_GLADEDIR "sort.glade2",
				  "sort_dialog",
				  NULL);

	if (gladexml == NULL)
	{
		gedit_warning (GTK_WINDOW (data->ui_window),
			       MISSING_FILE,
			       GEDIT_GLADEDIR "sort.glade2");
		return NULL;
	}

	dialog = g_new (SortDialog, 1);

	dialog->dialog = glade_xml_get_widget (gladexml, "sort_dialog");
	dialog->reverse_order_checkbutton = glade_xml_get_widget (gladexml, "reverse_order_checkbutton");
	dialog->col_num_spinbutton = glade_xml_get_widget (gladexml, "col_num_spinbutton");
	dialog->ignore_case_checkbutton = glade_xml_get_widget (gladexml, "ignore_case_checkbutton");
	dialog->remove_dups_checkbutton = glade_xml_get_widget (gladexml, "remove_dups_checkbutton");

	if (!dialog->dialog ||
	    !dialog->reverse_order_checkbutton ||
	    !dialog->col_num_spinbutton        ||
	    !dialog->ignore_case_checkbutton   ||
	    !dialog->remove_dups_checkbutton)
	{
		gedit_warning (GTK_WINDOW (data->ui_window),
			       MISSING_WIDGETS,
			       GEDIT_GLADEDIR "sort.glade2");

		g_free (dialog);
		return NULL;
	}

	g_object_unref (gladexml);

	g_signal_connect (dialog->dialog,
			  "destroy",
			  G_CALLBACK (sort_dialog_destroy),
			  data);
	g_signal_connect (dialog->dialog,
			  "response",
			  G_CALLBACK (sort_dialog_response_handler),
			  data);

	return dialog;
}

static void
sort_cb (GtkAction  *action,
	 PluginData *data)
{	
	gedit_debug (DEBUG_PLUGINS);

	g_return_if_fail (data != NULL);

	if (data->ui_dialog != NULL)
	{
		gtk_widget_grab_focus (data->ui_dialog->dialog);
		gtk_window_present (GTK_WINDOW (data->ui_dialog->dialog));
	}
	else
	{
		GtkWindowGroup *wg;
		
		data->ui_dialog = get_sort_dialog (data);
		g_return_if_fail (data->ui_dialog != NULL);

		wg = gedit_window_get_group (data->ui_window);
		g_return_if_fail (wg != NULL);
				
		gtk_window_group_add_window (wg, GTK_WINDOW (data->ui_dialog->dialog));

		gtk_window_set_transient_for (GTK_WINDOW (data->ui_dialog->dialog),
					      GTK_WINDOW (data->ui_window));

		gtk_window_set_modal (GTK_WINDOW (data->ui_dialog->dialog), TRUE);
		gtk_widget_show (data->ui_dialog->dialog);
	}
}

/* Compares two strings for the sorting algorithm. Uses the UTF-8 processing
 * functions in GLib to be as correct as possible.*/
static gint
compare_algorithm (gconstpointer s1, gconstpointer s2, gpointer data)
{
	gint length1, length2;
	gint ret;
	gchar *string1, *string2;
	gchar *substring1, *substring2;
	gchar *key1, *key2;
	SortInfo *sort_info;

	gedit_debug (DEBUG_PLUGINS);

	sort_info = (SortInfo *) data;
	g_return_val_if_fail (sort_info != NULL, -1);

	if (!sort_info->ignore_case)
	{
		string1 = *((gchar **) s1);
		string2 = *((gchar **) s2);
	}
	else
	{
		string1 = g_utf8_casefold (*((gchar **) s1), -1);
		string2 = g_utf8_casefold (*((gchar **) s2), -1);
	}

	length1 = g_utf8_strlen (string1, -1);
	length2 = g_utf8_strlen (string2, -1);

	if ((length1 < sort_info->starting_column) &&
	    (length2 < sort_info->starting_column))
	{
		ret = 0;
	}
	else if (length1 < sort_info->starting_column)
	{
		ret = -1;
	}
	else if (length2 < sort_info->starting_column)
	{
		ret = 1;
	}
	else if (sort_info->starting_column < 1)
	{
		key1 = g_utf8_collate_key (string1, -1);
		key2 = g_utf8_collate_key (string2, -1);
		ret = strcmp (key1, key2);

		g_free (key1);
		g_free (key2);
	}
	else
	{
		/* A character column offset is required, so figure out
		 * the correct offset into the UTF-8 string. */
		substring1 = g_utf8_offset_to_pointer (string1, sort_info->starting_column);
		substring2 = g_utf8_offset_to_pointer (string2, sort_info->starting_column);

		key1 = g_utf8_collate_key (substring1, -1);
		key2 = g_utf8_collate_key (substring2, -1);
		ret = strcmp (key1, key2);

		g_free (key1);
		g_free (key2);
	}

	/* Do the necessary cleanup. */
	if (sort_info->ignore_case)
	{
		g_free (string1);
		g_free (string2);
	}

	if (sort_info->reverse_order)
	{
		ret = -1 * ret;
	}

	return ret;
}

static void
sort_real (PluginData *data)
{
	GeditDocument *doc;
	GtkTextIter start, end;
	gchar *buffer;
	gchar *p;
	gunichar c;
	gchar *last_row = NULL;
	gpointer *lines;
	gint cont;
	SortInfo *sort_info;

	gedit_debug (DEBUG_PLUGINS);

	doc = gedit_window_get_active_document (data->ui_window);
	g_return_if_fail (doc != NULL);

	sort_info = g_new0 (SortInfo, 1);
	sort_info->ignore_case = gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (data->ui_dialog->ignore_case_checkbutton));
	sort_info->reverse_order = gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (data->ui_dialog->reverse_order_checkbutton));
	sort_info->remove_duplicates = gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (data->ui_dialog->remove_dups_checkbutton));
	sort_info->starting_column = gtk_spin_button_get_value_as_int (GTK_SPIN_BUTTON (data->ui_dialog->col_num_spinbutton)) - 1;

	if (!gtk_text_buffer_get_selection_bounds (GTK_TEXT_BUFFER (doc),
						   &start, &end))
	{
		/* No selection, get the whole document. */
		gtk_text_buffer_get_bounds (GTK_TEXT_BUFFER (doc),
					    &start, &end);
	}

	buffer = gtk_text_buffer_get_slice (GTK_TEXT_BUFFER (doc),
					    &start, &end, TRUE);

	lines = g_new0 (gpointer, gtk_text_buffer_get_line_count (GTK_TEXT_BUFFER (doc)) + 1);

	gedit_debug_message (DEBUG_PLUGINS, "Building list...");

	cont = 0;
	p = buffer;
	c = g_utf8_get_char (p);

	while (c != '\0')
	{
		if (c == '\n')
		{
			gchar *old_p;

			old_p = p;
			p = g_utf8_next_char (p);

			*old_p = '\0';

			lines[cont] = p;
			++cont;
		} else
		{
			p = g_utf8_next_char (p);
		}

		c = g_utf8_get_char (p);
	}

	lines[cont] = buffer;
	++cont;

	gedit_debug_message (DEBUG_PLUGINS, "Sort list...");

	g_qsort_with_data (lines, cont, sizeof (gpointer), compare_algorithm, sort_info);

	gedit_debug_message (DEBUG_PLUGINS, "Rebuilding document...");

	gtk_source_buffer_begin_not_undoable_action (GTK_SOURCE_BUFFER (doc));

	gtk_text_buffer_delete (GTK_TEXT_BUFFER (doc), &start, &end);

	cont = 0;

	while (lines[cont] != NULL)
	{
		gchar *current_row = lines[cont];

		/* Don't insert this row if it's the same as the last
		 * one and the user has specified to remove duplicates. */
		if (!sort_info->remove_duplicates ||
		    last_row == NULL ||
		    (strcmp (last_row, current_row) != 0))
		{
			gtk_text_buffer_insert (GTK_TEXT_BUFFER (doc),
						&start,
						current_row, -1);

			if (lines[cont + 1] != NULL)
			{
				gtk_text_buffer_insert (GTK_TEXT_BUFFER (doc),
							&start,
							"\n",
							-1);
			}
		}

		last_row = current_row;
		++cont;
	}

	gtk_source_buffer_end_not_undoable_action (GTK_SOURCE_BUFFER (doc));

	g_free (lines);
	g_free (buffer);
	g_free (sort_info);

	gedit_debug_message (DEBUG_PLUGINS, "Done.");
}

static void
free_plugin_data (PluginData *data)
{
	g_return_if_fail (data != NULL);

	g_object_unref (data->ui_action_group);
	g_free (data);
}

static void
update_ui_real (PluginData *data)
{
	GeditView *view;

	gedit_debug (DEBUG_PLUGINS);

	view = gedit_window_get_active_view (data->ui_window);

	gtk_action_group_set_sensitive (data->ui_action_group,
					(view != NULL) &&
					gtk_text_view_get_editable (GTK_TEXT_VIEW (view)));
}

static void
impl_activate (GeditPlugin *plugin,
	       GeditWindow *window)
{
	GtkUIManager *manager;
	PluginData *plugin_data;

	gedit_debug (DEBUG_PLUGINS);

	plugin_data = g_new (PluginData, 1);
	plugin_data->ui_dialog = NULL;
	plugin_data->ui_window = window;

	manager = gedit_window_get_ui_manager (window);

	plugin_data->ui_action_group = gtk_action_group_new ("GeditSortPluginActions");
	gtk_action_group_set_translation_domain (plugin_data->ui_action_group, 
						 GETTEXT_PACKAGE);
	gtk_action_group_add_actions (plugin_data->ui_action_group, 
				      action_entries,
				      G_N_ELEMENTS (action_entries), 
				      plugin_data);

	gtk_ui_manager_insert_action_group (manager, plugin_data->ui_action_group, -1);

	plugin_data->ui_id = gtk_ui_manager_new_merge_id (manager);

	g_object_set_data_full (G_OBJECT (window), 
				PLUGIN_DATA_KEY, 
				plugin_data,
				(GDestroyNotify) free_plugin_data);

	gtk_ui_manager_add_ui (manager, 
			       plugin_data->ui_id, 
			       MENU_PATH,
			       "Sort", 
			       "Sort",
			       GTK_UI_MANAGER_MENUITEM, 
			       TRUE);

	update_ui_real (plugin_data);
}

static void
impl_deactivate	(GeditPlugin *plugin,
		 GeditWindow *window)
{
	GtkUIManager *manager;
	PluginData *plugin_data;

	gedit_debug (DEBUG_PLUGINS);

	manager = gedit_window_get_ui_manager (window);

	plugin_data = (PluginData *) g_object_get_data (G_OBJECT (window), PLUGIN_DATA_KEY);
	g_return_if_fail (plugin_data != NULL);

	gtk_ui_manager_remove_ui (manager, plugin_data->ui_id);
	gtk_ui_manager_remove_action_group (manager, plugin_data->ui_action_group);

	if (plugin_data->ui_dialog != NULL)
	{
		gtk_widget_destroy (plugin_data->ui_dialog->dialog);
	}

	g_object_set_data (G_OBJECT (window), PLUGIN_DATA_KEY, NULL);
}

static void
impl_update_ui (GeditPlugin *plugin,
		GeditWindow *window)
{
	PluginData *data;

	gedit_debug (DEBUG_PLUGINS);

	data = (PluginData *) g_object_get_data (G_OBJECT (window), PLUGIN_DATA_KEY);
	g_return_if_fail (data != NULL);
	
	update_ui_real (data);
}

static void
gedit_sort_plugin_init (GeditSortPlugin *plugin)
{
	gedit_debug_message (DEBUG_PLUGINS, "GeditSortPlugin initializing");
}

static void
gedit_sort_plugin_finalize (GObject *object)
{
	gedit_debug_message (DEBUG_PLUGINS, "GeditSortPlugin finalizing");

	G_OBJECT_CLASS (gedit_sort_plugin_parent_class)->finalize (object);
}

static void
gedit_sort_plugin_class_init (GeditSortPluginClass *klass)
{
	GObjectClass *object_class = G_OBJECT_CLASS (klass);
	GeditPluginClass *plugin_class = GEDIT_PLUGIN_CLASS (klass);

	object_class->finalize = gedit_sort_plugin_finalize;

	plugin_class->activate = impl_activate;
	plugin_class->deactivate = impl_deactivate;
	plugin_class->update_ui = impl_update_ui;
}
