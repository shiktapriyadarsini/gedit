/*
 * gedit-configurable.h
 * This file is part of gedit
 *
 * Copyright (C) 2009 Steve Frécinaux
 * Copyright (C) 2013 - Ignacio Casal Quinteiro
 *
 * gedit is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * gedit is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with gedit. If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef __GEDIT_CONFIGURABLE_H__
#define __GEDIT_CONFIGURABLE_H__

#include <gtk/gtk.h>

G_BEGIN_DECLS

#define GEDIT_TYPE_CONFIGURABLE            (gedit_configurable_get_type ())
#define GEDIT_CONFIGURABLE(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), GEDIT_TYPE_CONFIGURABLE, GeditConfigurable))
#define GEDIT_CONFIGURABLE_IFACE(obj)      (G_TYPE_CHECK_CLASS_CAST ((obj), GEDIT_TYPE_CONFIGURABLE, GeditConfigurableInterface))
#define GEDIT_IS_CONFIGURABLE(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), GEDIT_TYPE_CONFIGURABLE))
#define GEDIT_CONFIGURABLE_GET_IFACE(obj)  (G_TYPE_INSTANCE_GET_INTERFACE ((obj), GEDIT_TYPE_CONFIGURABLE, GeditConfigurableInterface))

/**
 * GeditConfigurable:
 *
 * Interface for configurable plugins.
 */
typedef struct _GeditConfigurable           GeditConfigurable; /* dummy typedef */
typedef struct _GeditConfigurableInterface  GeditConfigurableInterface;

/**
 * GeditConfigurableInterface:
 * @g_iface: The parent interface.
 * @create_configure_widget: Creates the configure widget for the plugin.
 *
 * Provides an interface for configurable plugins.
 */
struct _GeditConfigurableInterface
{
	GTypeInterface g_iface;

	const gchar *(*get_page_name)		 (GeditConfigurable  *configurable);
	GtkWidget   *(*create_configure_widget)  (GeditConfigurable  *configurable);
};

GType       gedit_configurable_get_type                 (void)  G_GNUC_CONST;

const gchar *gedit_configurable_get_page_name           (GeditConfigurable  *configurable);
GtkWidget   *gedit_configurable_create_configure_widget (GeditConfigurable  *configurable);

G_END_DECLS

#endif /* __GEDIT_PLUGIN_MANAGER_H__  */
