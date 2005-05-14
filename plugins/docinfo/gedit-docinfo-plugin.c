/*
 * gedit-docinfo-plugin.h
 * 
 * Copyright (C) 2002-2005 Paolo Maggi 
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

#include "gedit-docinfo-plugin.h"

#include <string.h> /* For strlen (...) */

#include <glib/gi18n-lib.h>
#include <glade/glade-xml.h>
#include <pango/pango-break.h>
#include <gmodule.h>

#include <gedit/gedit-debug.h>
#include <gedit/gedit-utils.h>

#define PLUGIN_DATA_KEY "GeditDocInfoPluginData"
#define MENU_PATH "/MenuBar/Tools/ToolsOps_2"

GEDIT_PLUGIN_REGISTER_TYPE(GeditDocInfoPlugin, gedit_docinfo_plugin)

typedef struct
{
	GtkWidget *dialog;
	GtkWidget *file_name_label;
	GtkWidget *lines_label;
	GtkWidget *words_label;
	GtkWidget *chars_label;
	GtkWidget *chars_ns_label;
	GtkWidget *bytes_label;
} PluginDialog;

typedef struct
{
	GtkActionGroup	*ui_action_group;
	guint ui_id;

	PluginDialog *ui_dialog;
} DocInfoPluginData;

static void	dialog_response_callback	(GtkDialog         *dialog,
						 gint               res_id,
						 DocInfoPluginData *data);

static void
dialog_destroy_callback (GtkObject         *object,
			 DocInfoPluginData *data)
{
	gedit_debug (DEBUG_PLUGINS);

	if (data != NULL)
	{
		if (data->ui_dialog != NULL)
		{
			g_free (data->ui_dialog);
			data->ui_dialog = NULL;
		}
	}
}

static PluginDialog *
get_dialog (GeditWindow       *window,
	    DocInfoPluginData *data)
{
	GladeXML *gladexml;
	GtkWidget *content;

	gedit_debug (DEBUG_PLUGINS);

	if (data->ui_dialog != NULL)
	{
		gtk_window_present (GTK_WINDOW (data->ui_dialog->dialog));
		gtk_widget_grab_focus (data->ui_dialog->dialog);

		return data->ui_dialog;
	}

	gladexml = glade_xml_new (GEDIT_GLADEDIR "docinfo.glade2",
				  "dialog",
				  NULL);
	if (!gladexml)
	{
		gedit_warning (GTK_WINDOW (window),
			       MISSING_FILE,
			       GEDIT_GLADEDIR "docinfo.glade2");

		data->ui_dialog = NULL;
		return NULL;
	}

	data->ui_dialog = g_new (PluginDialog, 1);

	data->ui_dialog->dialog = glade_xml_get_widget (gladexml, "dialog");
	g_return_val_if_fail (data->ui_dialog->dialog != NULL, NULL);

	content	= glade_xml_get_widget (gladexml, "docinfo_dialog_content");
	data->ui_dialog->file_name_label = glade_xml_get_widget (gladexml, "file_name_label");
	data->ui_dialog->words_label = glade_xml_get_widget (gladexml, "words_label");
	data->ui_dialog->bytes_label = glade_xml_get_widget (gladexml, "bytes_label");
	data->ui_dialog->lines_label = glade_xml_get_widget (gladexml, "lines_label");
	data->ui_dialog->chars_label = glade_xml_get_widget (gladexml, "chars_label");
	data->ui_dialog->chars_ns_label = glade_xml_get_widget (gladexml, "chars_ns_label");

	if (!content ||
	    !data->ui_dialog->file_name_label ||
	    !data->ui_dialog->words_label     ||
	    !data->ui_dialog->bytes_label     ||
	    !data->ui_dialog->lines_label     ||
	    !data->ui_dialog->chars_label     ||
	    !data->ui_dialog->chars_ns_label)
	{
		gedit_warning (GTK_WINDOW (window),
			       MISSING_WIDGETS,
			       GEDIT_GLADEDIR "docinfo.glade2");

		return NULL;
	}

	g_object_unref (gladexml);

	g_signal_connect (data->ui_dialog->dialog,
			  "destroy",
			  G_CALLBACK (dialog_destroy_callback),
			  data);

	g_signal_connect (data->ui_dialog->dialog,
			  "response",
			  G_CALLBACK (dialog_response_callback),
			  data);

	gtk_window_set_transient_for (GTK_WINDOW (data->ui_dialog->dialog),
				      GTK_WINDOW (window));

	gtk_dialog_set_default_response (GTK_DIALOG (data->ui_dialog->dialog),
					 GTK_RESPONSE_OK);

	gtk_widget_show (data->ui_dialog->dialog);

	return data->ui_dialog;
}

static void
document_statistics_cb_real (GeditWindow       *window,
			     DocInfoPluginData *data)
{
	PluginDialog *dialog;
	GeditDocument *doc;
	GtkTextIter start, end;
	gchar *text;
	PangoLogAttr *attrs;
	gint words = 0;
	gint chars = 0;
	gint white_chars = 0;
	gint lines = 0;
	gint bytes = 0;
	gint i;
	gchar *tmp_str;
	const gchar *file_name;

	gedit_debug (DEBUG_PLUGINS);

	dialog = get_dialog (window, data);
	g_return_if_fail (dialog != NULL);

	doc = gedit_window_get_active_document (window);

	if (doc == NULL)
	{
		gtk_widget_destroy (dialog->dialog);
		return;
	}

	gtk_text_buffer_get_bounds (GTK_TEXT_BUFFER (doc), &start, &end);
	text = gtk_text_buffer_get_slice (GTK_TEXT_BUFFER (doc),
					  &start,
					  &end,
					  TRUE);

	lines = gtk_text_buffer_get_line_count (GTK_TEXT_BUFFER (doc));

	chars = g_utf8_strlen (text, -1);
 	attrs = g_new0 (PangoLogAttr, chars + 1);

	pango_get_log_attrs (text,
			     -1,
			     0,
			     pango_language_from_string ("C"),
			     attrs,
			     chars + 1);

	for (i = 0; i < chars; i++)
	{
		if (attrs [i].is_white)
			++white_chars;

		if (attrs [i].is_word_start)
			++words;
	}

	if (chars == 0)
		lines = 0;

	bytes = strlen (text);

	gedit_debug_message (DEBUG_PLUGINS, "Chars: %d", chars);
	gedit_debug_message (DEBUG_PLUGINS, "Lines: %d", lines);
	gedit_debug_message (DEBUG_PLUGINS, "Words: %d", words);
	gedit_debug_message (DEBUG_PLUGINS, "Chars non-space: %d", chars - white_chars);

	g_free (attrs);
	g_free (text);

	file_name = gedit_document_get_short_name_for_display (doc);
	tmp_str = g_strdup_printf ("<span weight=\"bold\">%s</span>", file_name);
	gtk_label_set_markup (GTK_LABEL (dialog->file_name_label), tmp_str);
	g_free (tmp_str);

	tmp_str = g_strdup_printf("%d", lines);
	gtk_label_set_text (GTK_LABEL (dialog->lines_label), tmp_str);
	g_free (tmp_str);
	
	tmp_str = g_strdup_printf("%d", words);
	gtk_label_set_text (GTK_LABEL (dialog->words_label), tmp_str);
	g_free (tmp_str);

	tmp_str = g_strdup_printf("%d", chars);
	gtk_label_set_text (GTK_LABEL (dialog->chars_label), tmp_str);
	g_free (tmp_str);

	tmp_str = g_strdup_printf("%d", chars - white_chars);
	gtk_label_set_text (GTK_LABEL (dialog->chars_ns_label), tmp_str);
	g_free (tmp_str);

	tmp_str = g_strdup_printf("%d", bytes);
	gtk_label_set_text (GTK_LABEL (dialog->bytes_label), tmp_str);
	g_free (tmp_str);
}

static void
document_statistics_cb (GtkAction   *action,
			GeditWindow *window)
{
	DocInfoPluginData *data;

	gedit_debug (DEBUG_PLUGINS);

	data = (DocInfoPluginData *) g_object_get_data (G_OBJECT (window), PLUGIN_DATA_KEY);
	g_return_if_fail (data != NULL);

	document_statistics_cb_real (window, data);
}

static void
dialog_response_callback (GtkDialog         *dialog,
			  gint               res_id,
			  DocInfoPluginData *data)
{
	GeditWindow *window;

	gedit_debug (DEBUG_PLUGINS);

	switch (res_id)
	{
		case GTK_RESPONSE_OK:
			window = GEDIT_WINDOW (gtk_window_get_transient_for (GTK_WINDOW (data->ui_dialog->dialog)));
			g_return_if_fail (window != NULL);

			document_statistics_cb_real (window, data);

			break;

		case GTK_RESPONSE_CLOSE:
			gtk_widget_destroy (data->ui_dialog->dialog);

			break;
	}
}

static const GtkActionEntry action_entries[] =
{
	{ "DocumentStatistics",
	  NULL,
	  N_("_Document Statistics"),
	  NULL,
	  N_("Get statistic info on current document"),
	  G_CALLBACK (document_statistics_cb) }
};

static void
free_plugin_data (DocInfoPluginData *data)
{
	g_return_if_fail (data != NULL);
	
	g_object_unref (data->ui_action_group);
	g_free (data);
}

static void
update_ui_real (GeditWindow *window,
		DocInfoPluginData *data)
{
	GeditView *view;

	gedit_debug (DEBUG_PLUGINS);

	view = gedit_window_get_active_view (window);

	gtk_action_group_set_sensitive (data->ui_action_group,
					(view != NULL));
}

static void
gedit_docinfo_plugin_init (GeditDocInfoPlugin *plugin)
{
	gedit_debug_message (DEBUG_PLUGINS, "GeditDocInfoPlugin initializing");
}

static void
gedit_docinfo_plugin_finalize (GObject *object)
{
	gedit_debug_message (DEBUG_PLUGINS, "GeditDocInfoPlugin finalizing");

	G_OBJECT_CLASS (gedit_docinfo_plugin_parent_class)->finalize (object);
}

static void
impl_activate (GeditPlugin *plugin,
	       GeditWindow *window)
{
	GtkUIManager *manager;
	DocInfoPluginData *plugin_data;

	gedit_debug (DEBUG_PLUGINS);

	plugin_data = g_new (DocInfoPluginData, 1);
	plugin_data->ui_dialog = NULL;

	manager = gedit_window_get_ui_manager (window);

	plugin_data->ui_action_group = gtk_action_group_new ("GeditDocInfoPluginActions");
	gtk_action_group_set_translation_domain (plugin_data->ui_action_group, 
						 GETTEXT_PACKAGE);
	gtk_action_group_add_actions (plugin_data->ui_action_group, 
				      action_entries,
				      G_N_ELEMENTS (action_entries), 
				      window);

	gtk_ui_manager_insert_action_group (manager,
					    plugin_data->ui_action_group,
					    -1);

	plugin_data->ui_id = gtk_ui_manager_new_merge_id (manager);

	g_object_set_data_full (G_OBJECT (window),
				PLUGIN_DATA_KEY,
				plugin_data,
				(GDestroyNotify) free_plugin_data);

	gtk_ui_manager_add_ui (manager,
			       plugin_data->ui_id, 
			       MENU_PATH,
			       "DocumentStatistics", 
			       "DocumentStatistics",
			       GTK_UI_MANAGER_MENUITEM, 
			       TRUE);

	update_ui_real (window, plugin_data);
}

static void
impl_deactivate	(GeditPlugin *plugin,
		 GeditWindow *window)
{
	GtkUIManager *manager;
	DocInfoPluginData *plugin_data;

	gedit_debug (DEBUG_PLUGINS);

	manager = gedit_window_get_ui_manager (window);

	plugin_data = (DocInfoPluginData *) g_object_get_data (G_OBJECT (window), PLUGIN_DATA_KEY);
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
impl_update_ui	(GeditPlugin *plugin,
		 GeditWindow *window)
{
	DocInfoPluginData *data;

	gedit_debug (DEBUG_PLUGINS);

	data = (DocInfoPluginData *) g_object_get_data (G_OBJECT (window), PLUGIN_DATA_KEY);
	g_return_if_fail (data != NULL);

	update_ui_real (window, data);
}

static void
gedit_docinfo_plugin_class_init (GeditDocInfoPluginClass *klass)
{
	GObjectClass *object_class = G_OBJECT_CLASS (klass);
	GeditPluginClass *plugin_class = GEDIT_PLUGIN_CLASS (klass);

	object_class->finalize = gedit_docinfo_plugin_finalize;

	plugin_class->activate = impl_activate;
	plugin_class->deactivate = impl_deactivate;
	plugin_class->update_ui = impl_update_ui;
}
