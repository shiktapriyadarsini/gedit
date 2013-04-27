/*
 * gedit-configurable.c
 * This file is part of gedit
 *
 * Copyright (C) 2009 - 2010 Steve Frécinaux
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

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include "gedit-configurable.h"

/**
 * SECTION:gedit-configurable
 * @short_description: Interface for providing a plugin configuration UI.
 *
 * The #GeditConfigurable interface will allow a plugin to provide a
 * graphical interface for the user to configure the plugin through the
 * gedit preferences dialog.
 **/

G_DEFINE_INTERFACE (GeditConfigurable, gedit_configurable, G_TYPE_OBJECT)

static void
gedit_configurable_default_init (GeditConfigurableInterface *iface)
{
}

/**
 * gedit_configurable_get_page_id:
 * @configurable: A #GeditConfigurable.
 *
 * Returns: (transfer none):
 */
const gchar *
gedit_configurable_get_page_id (GeditConfigurable *configurable)
{
	GeditConfigurableInterface *iface;

	g_return_val_if_fail (GEDIT_IS_CONFIGURABLE (configurable), NULL);

	iface = GEDIT_CONFIGURABLE_GET_IFACE (configurable);

	if (G_LIKELY (iface->get_page_id != NULL))
	{
		return iface->get_page_id (configurable);
	}

	/* Default implementation */
	return NULL;
}

/**
 * gedit_configurable_get_page_name:
 * @configurable: A #GeditConfigurable.
 *
 * Returns: (transfer none):
 */
const gchar *
gedit_configurable_get_page_name (GeditConfigurable *configurable)
{
	GeditConfigurableInterface *iface;

	g_return_val_if_fail (GEDIT_IS_CONFIGURABLE (configurable), NULL);

	iface = GEDIT_CONFIGURABLE_GET_IFACE (configurable);

	if (G_LIKELY (iface->get_page_name != NULL))
	{
		return iface->get_page_name (configurable);
	}

	/* Default implementation */
	return NULL;
}

/**
 * gedit_configurable_create_configure_widget:
 * @configurable: A #GeditConfigurable.
 *
 * Creates the configure widget for the plugin. The returned widget
 * should allow configuring all the relevant aspects of the plugin, and should
 * allow instant-apply, as promoted by the Gnome Human Interface Guidelines.
 *
 * The returned widget will be embedded into gedit's preferences dialog.
 *
 * This method should always return a valid #GtkWidget instance, never %NULL.
 *
 * Returns: (transfer full): A #GtkWidget used for configuration.
 */
GtkWidget *
gedit_configurable_create_configure_widget (GeditConfigurable *configurable)
{
	GeditConfigurableInterface *iface;

	g_return_val_if_fail (GEDIT_IS_CONFIGURABLE (configurable), NULL);

	iface = GEDIT_CONFIGURABLE_GET_IFACE (configurable);

	if (G_LIKELY (iface->create_configure_widget != NULL))
	{
		return iface->create_configure_widget (configurable);
	}

	/* Default implementation */
	return NULL;
}
