/*
 * gedit-tab-manager.h
 * This file is part of gedit
 *
 * Copyright (C) 2010 - Ignacio Casal Quinteiro
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
 * along with gedit; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, 
 * Boston, MA  02110-1301  USA
 */

#ifndef __GEDIT_TAB_MANAGER_H__
#define __GEDIT_TAB_MANAGER_H__

#include <glib-object.h>

#include "gedit-notebook.h"

G_BEGIN_DECLS

#define GEDIT_TYPE_TAB_MANAGER			(gedit_tab_manager_get_type ())
#define GEDIT_TAB_MANAGER(obj)			(G_TYPE_CHECK_INSTANCE_CAST ((obj), GEDIT_TYPE_TAB_MANAGER, GeditTabManager))
#define GEDIT_TAB_MANAGER_CONST(obj)		(G_TYPE_CHECK_INSTANCE_CAST ((obj), GEDIT_TYPE_TAB_MANAGER, GeditTabManager const))
#define GEDIT_TAB_MANAGER_CLASS(klass)		(G_TYPE_CHECK_CLASS_CAST ((klass), GEDIT_TYPE_TAB_MANAGER, GeditTabManagerClass))
#define GEDIT_IS_TAB_MANAGER(obj)		(G_TYPE_CHECK_INSTANCE_TYPE ((obj), GEDIT_TYPE_TAB_MANAGER))
#define GEDIT_IS_TAB_MANAGER_CLASS(klass)	(G_TYPE_CHECK_CLASS_TYPE ((klass), GEDIT_TYPE_TAB_MANAGER))
#define GEDIT_TAB_MANAGER_GET_CLASS(obj)	(G_TYPE_INSTANCE_GET_CLASS ((obj), GEDIT_TYPE_TAB_MANAGER, GeditTabManagerClass))

typedef struct _GeditTabManager		GeditTabManager;
typedef struct _GeditTabManagerClass	GeditTabManagerClass;
typedef struct _GeditTabManagerPrivate	GeditTabManagerPrivate;

struct _GeditTabManager
{
	GtkVBox parent;
	
	GeditTabManagerPrivate *priv;
};

struct _GeditTabManagerClass
{
	GtkVBoxClass parent_class;

	void		(*notebook_added)	(GeditTabManager *manager,
						 GeditNotebook   *notebook);
	void		(*notebook_removed)	(GeditTabManager *manager,
						 GeditNotebook   *notebook);
};

GType			 gedit_tab_manager_get_type			(void) G_GNUC_CONST;

GtkWidget		*gedit_tab_manager_new				(void);

GeditNotebook		*gedit_tab_manager_get_active_notebook		(GeditTabManager *manager);

GeditNotebook		*gedit_tab_manager_split			(GeditTabManager *manager);

void			 gedit_tab_manager_remove_page			(GeditTabManager *manager,
									 GeditPage       *page);

GList			*gedit_tab_manager_get_pages			(GeditTabManager *manager);

G_END_DECLS

#endif /* __GEDIT_TAB_MANAGER_H__ */
