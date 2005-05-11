/*
 * gedit-taglist-plugin.h
 * 
 * Copyright (C) 2002-2005 - Paolo Maggi
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
 */

/*
 * Modified by the gedit Team, 2002-2005. See the AUTHORS file for a 
 * list of people on the gedit Team.  
 * See the ChangeLog files for a list of changes. 
 *
 * $Id$
 */

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include "gedit-taglist-plugin.h"
#include "gedit-taglist-plugin-panel.h"
#include "gedit-taglist-plugin-parser.h"

#include <glib/gi18n-lib.h>
#include <gmodule.h>

#include <gedit/gedit-debug.h>

#define GEDIT_TAGLIST_PLUGIN_GET_PRIVATE(object)(G_TYPE_INSTANCE_GET_PRIVATE ((object), GEDIT_TYPE_TAGLIST_PLUGIN, GeditTaglistPluginPrivate))

struct _GeditTaglistPluginPrivate
{
	gpointer dummy;
};

static GType gedit_taglist_plugin_type = 0;

GType
gedit_taglist_plugin_get_type (void)
{
	return gedit_taglist_plugin_type;
}

static void     gedit_taglist_plugin_init              (GeditTaglistPlugin      *self);
static void     gedit_taglist_plugin_class_init        (GeditTaglistPluginClass *klass);
static gpointer gedit_taglist_plugin_parent_class = NULL;
static void     gedit_taglist_plugin_class_intern_init (gpointer klass)
{
	gedit_taglist_plugin_parent_class = g_type_class_peek_parent (klass);		
	gedit_taglist_plugin_class_init ((GeditTaglistPluginClass *) klass);			
}										
										
G_MODULE_EXPORT GType								
register_gedit_plugin (GTypeModule *module)					
{										
	static const GTypeInfo our_info =					
	{									
		sizeof (GeditTaglistPluginClass),					
		NULL, /* base_init */						
		NULL, /* base_finalize */					
		(GClassInitFunc) gedit_taglist_plugin_class_intern_init,		
		NULL,								
		NULL, /* class_data */						
		sizeof (GeditTaglistPlugin),						
		0, /* n_preallocs */						
		(GInstanceInitFunc) gedit_taglist_plugin_init				
	};									
	
	gedit_debug_message (DEBUG_PLUGINS, "Registering GeditTaglistPlugin");	
										
	/* Initialise the i18n stuff */						
	bindtextdomain (GETTEXT_PACKAGE, GEDIT_LOCALEDIR);			
	bind_textdomain_codeset (GETTEXT_PACKAGE, "UTF-8");			
										
	gedit_taglist_plugin_type = g_type_module_register_type (module,		
					    GEDIT_TYPE_PLUGIN,			
					    "GeditTaglistPlugin",			
					    &our_info,				
					    0);

	gedit_taglist_plugin_panel_register_type (module);

	return gedit_taglist_plugin_type;						
}

static void
gedit_taglist_plugin_init (GeditTaglistPlugin *plugin)
{
	plugin->priv = GEDIT_TAGLIST_PLUGIN_GET_PRIVATE (plugin);

	gedit_debug_message (DEBUG_PLUGINS, "GeditTaglistPlugin initializing");
	
	create_taglist ();
}

static void
gedit_taglist_plugin_finalize (GObject *object)
{
/*
	GeditTaglistPlugin *plugin = GEDIT_TAGLIST_PLUGIN (object);
*/
	gedit_debug_message (DEBUG_PLUGINS, "GeditTaglistPlugin finalizing");

	free_taglist ();
	
	G_OBJECT_CLASS (gedit_taglist_plugin_parent_class)->finalize (object);
}

static void
impl_activate (GeditPlugin *plugin,
	       GeditWindow *window)
{
	GeditPanel *side_panel;
	GtkWidget *taglist_panel;
	
	gedit_debug (DEBUG_PLUGINS);
	
	side_panel = gedit_window_get_side_panel (window);
	taglist_panel = gedit_taglist_plugin_panel_new (window);
	
	gedit_panel_add_item (side_panel, 
			      taglist_panel, 
			      "Tags", 
			      GTK_STOCK_ADD);
}

static void
impl_deactivate	(GeditPlugin *plugin,
		 GeditWindow *window)
{
	gedit_debug (DEBUG_PLUGINS);
}

static void
impl_update_ui	(GeditPlugin *plugin,
		 GeditWindow *window)
{
	gedit_debug (DEBUG_PLUGINS);
}

/*
static GtkWidget *
impl_create_configure_dialog (GeditPlugin *plugin)
{
	* Implements this function only and only if the plugin
	* is configurable. Otherwise you can safely remove it. *
}
*/

static void
gedit_taglist_plugin_class_init (GeditTaglistPluginClass *klass)
{
	GObjectClass *object_class = G_OBJECT_CLASS (klass);
	GeditPluginClass *plugin_class = GEDIT_PLUGIN_CLASS (klass);

	object_class->finalize = gedit_taglist_plugin_finalize;

	plugin_class->activate = impl_activate;
	plugin_class->deactivate = impl_deactivate;
	plugin_class->update_ui = impl_update_ui;

	/* Only if the plugin is configurable */
	/* plugin_class->create_configure_dialog = impl_create_configure_dialog; */

	g_type_class_add_private (object_class, sizeof (GeditTaglistPluginPrivate));
}
