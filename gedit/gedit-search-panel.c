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

#include "gedit-search-panel.h"
#include "gedit-utils.h"

#include <glib/gi18n.h>
#include <glade/glade-xml.h>

#define GEDIT_SEARCH_PANEL_GET_PRIVATE(object)(G_TYPE_INSTANCE_GET_PRIVATE ((object), GEDIT_TYPE_SEARCH_PANEL, GeditSearchPanelPrivate))

struct _GeditSearchPanelPrivate
{
	GeditWindow  *window;
	
	GtkWidget    *replace_expander;
	GtkWidget    *goto_line_expander;
};

G_DEFINE_TYPE(GeditSearchPanel, gedit_search_panel, GTK_TYPE_VBOX)

enum
{
	PROP_0,
	PROP_WINDOW,
};

static void
set_window (GeditSearchPanel *panel,
	    GeditWindow      *window)
{
	g_return_if_fail (panel->priv->window == NULL);
	g_return_if_fail (GEDIT_IS_WINDOW (window));
	
	panel->priv->window = window;
/*	
	g_signal_connect (window,
			  "tab_added",
			  G_CALLBACK (window_tab_added),
			  panel);
	g_signal_connect (window,
			  "tab_removed",
			  G_CALLBACK (window_tab_removed),
			  panel);
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
 	
 	if (!find_vbox				||
 	    !search_panel_vbox			||
 	    !panel->priv->replace_expander 	||
 	    !panel->priv->goto_line_expander)
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
 	
 	g_print ("1");
 	gtk_box_pack_start (GTK_BOX (panel), 
			    search_panel_vbox, 
			    TRUE, 
			    TRUE, 
			    0);
}

GtkWidget *
gedit_search_panel_new (GeditWindow *window)
{
	g_return_val_if_fail (GEDIT_IS_WINDOW (window), NULL);

	return GTK_WIDGET (g_object_new (GEDIT_TYPE_SEARCH_PANEL, 
					 "window", window,
					 NULL));
}
