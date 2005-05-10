/*
 * gedit-plugin.h
 * This file is part of gedit
 *
 * Copyright (C) 2002-2005 - Paolo Maggi 
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
 * Modified by the gedit Team, 2002-2005. See the AUTHORS file for a 
 * list of people on the gedit Team.  
 * See the ChangeLog files for a list of changes. 
 *
 * $Id$
 */

#ifndef __GEDIT_PLUGIN_H__
#define __GEDIT_PLUGIN_H__

#include <glib-object.h>

#include <gedit/gedit-window.h>

/* TODO: add a .h file that includes all the .h files normally needed to
 * develop a plugin */ 

G_BEGIN_DECLS

/*
 * Type checking and casting macros
 */
#define GEDIT_TYPE_PLUGIN              (gedit_plugin_get_type())
#define GEDIT_PLUGIN(obj)              (G_TYPE_CHECK_INSTANCE_CAST((obj), GEDIT_TYPE_PLUGIN, GeditPlugin))
#define GEDIT_PLUGIN_CONST(obj)        (G_TYPE_CHECK_INSTANCE_CAST((obj), GEDIT_TYPE_PLUGIN, GeditPlugin const))
#define GEDIT_PLUGIN_CLASS(klass)      (G_TYPE_CHECK_CLASS_CAST((klass), GEDIT_TYPE_PLUGIN, GeditPluginClass))
#define GEDIT_IS_PLUGIN(obj)           (G_TYPE_CHECK_INSTANCE_TYPE((obj), GEDIT_TYPE_PLUGIN))
#define GEDIT_IS_PLUGIN_CLASS(klass)   (G_TYPE_CHECK_CLASS_TYPE ((klass), GEDIT_TYPE_PLUGIN))
#define GEDIT_PLUGIN_GET_CLASS(obj)    (G_TYPE_INSTANCE_GET_CLASS((obj), GEDIT_TYPE_PLUGIN, GeditPluginClass))

/*
 * Main object structure
 */
typedef struct _GeditPlugin GeditPlugin;

struct _GeditPlugin 
{
	GObject parent;
};

/*
 * Class definition
 */
typedef struct _GeditPluginClass GeditPluginClass;

struct _GeditPluginClass 
{
	GObjectClass parent_class;
	
	/* Virtual public methods */
	
	void 		(*activate)		(GeditPlugin *plugin,
						 GeditWindow *window);
	void 		(*deactivate)		(GeditPlugin *plugin,
						 GeditWindow *window);
				 
	void 		(*update_ui)		(GeditPlugin *plugin,
						 GeditWindow *window);

	GtkWidget      *(*create_configure_dialog)	
						(GeditPlugin *plugin);

	/* TODO: add place holders */						
};

/*
 * Public methods
 */
GType 		 gedit_plugin_get_type 		(void) G_GNUC_CONST;

void 		 gedit_plugin_activate		(GeditPlugin *plugin,
						 GeditWindow *window);
void 		 gedit_plugin_deactivate	(GeditPlugin *plugin,
						 GeditWindow *window);
				 
void 		 gedit_plugin_update_ui		(GeditPlugin *plugin,
						 GeditWindow *window);

gboolean	 gedit_plugin_is_configurable	(GeditPlugin *plugin);
GtkWidget	*gedit_plugin_create_configure_dialog		
						(GeditPlugin *plugin);

G_END_DECLS

#endif  /* __GEDIT_PLUGIN_H__ */


