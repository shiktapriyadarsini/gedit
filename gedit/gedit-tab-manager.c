/*
 * gedit-tab-manager.c
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


#include "gedit-tab-manager.h"


#define GEDIT_TAB_MANAGER_GET_PRIVATE(object)(G_TYPE_INSTANCE_GET_PRIVATE((object), GEDIT_TYPE_TAB_MANAGER, GeditTabManagerPrivate))

struct _GeditTabManagerPrivate
{
	GSList        *notebooks;
	GeditNotebook *active_notebook;
};

/* Signals */
enum
{
	NOTEBOOK_ADDED,
	NOTEBOOK_REMOVED,
	LAST_SIGNAL
};

static guint signals [LAST_SIGNAL] = { 0 };

G_DEFINE_TYPE (GeditTabManager, gedit_tab_manager, GTK_TYPE_VBOX)

static void
gedit_tab_manager_finalize (GObject *object)
{
	G_OBJECT_CLASS (gedit_tab_manager_parent_class)->finalize (object);
}

static void
gedit_tab_manager_class_init (GeditTabManagerClass *klass)
{
	GObjectClass *object_class = G_OBJECT_CLASS (klass);
	
	object_class->finalize = gedit_tab_manager_finalize;

	signals[NOTEBOOK_ADDED] =
		g_signal_new ("notebook-added",
			      G_TYPE_FROM_CLASS (object_class),
			      G_SIGNAL_RUN_FIRST,
			      G_STRUCT_OFFSET (GeditTabManagerClass, notebook_added),
			      NULL, NULL,
			      g_cclosure_marshal_VOID__OBJECT,
			      G_TYPE_NONE,
			      1,
			      GEDIT_TYPE_NOTEBOOK);

	signals[NOTEBOOK_REMOVED] =
		g_signal_new ("notebook-removed",
			      G_TYPE_FROM_CLASS (object_class),
			      G_SIGNAL_RUN_FIRST,
			      G_STRUCT_OFFSET (GeditTabManagerClass, notebook_removed),
			      NULL, NULL,
			      g_cclosure_marshal_VOID__OBJECT,
			      G_TYPE_NONE,
			      1,
			      GEDIT_TYPE_NOTEBOOK);

	g_type_class_add_private (object_class, sizeof (GeditTabManagerPrivate));
}

static void
add_notebook (GeditTabManager *manager,
	      GtkWidget       *notebook,
	      gboolean         main_container)
{
	if (main_container)
	{
		gtk_box_pack_start (GTK_BOX (manager), notebook,
		                    TRUE, TRUE, 0);
	}
	else
	{
		GtkWidget *active_notebook;
		GtkWidget *paned;
		GtkWidget *parent;

		active_notebook = GTK_WIDGET (manager->priv->active_notebook);

		paned = gtk_hpaned_new ();
		gtk_widget_show (paned);
	
		/* First we remove the active container from its parent to make
		   this we add a ref to it*/
		g_object_ref (active_notebook);
		parent = gtk_widget_get_parent (active_notebook);
	
		gtk_container_remove (GTK_CONTAINER (parent), active_notebook);
		gtk_container_add (GTK_CONTAINER (parent), paned);
	
		gtk_paned_pack1 (GTK_PANED (paned), active_notebook, FALSE, TRUE);
		g_object_unref (active_notebook);
	
		gtk_paned_pack2 (GTK_PANED (paned), notebook, FALSE, TRUE);
	}

	manager->priv->notebooks = g_slist_append (manager->priv->notebooks,
						   notebook);

	g_signal_emit (G_OBJECT (manager),
		       signals[NOTEBOOK_ADDED],
		       0,
		       notebook);
}

static void
gedit_tab_manager_init (GeditTabManager *manager)
{
	GeditNotebook *notebook;

	manager->priv = GEDIT_TAB_MANAGER_GET_PRIVATE (manager);
	manager->priv->notebooks = NULL;

	notebook = GEDIT_NOTEBOOK (gedit_notebook_new ());
	gtk_widget_show (GTK_WIDGET (notebook));

	add_notebook (manager, GTK_WIDGET (notebook), TRUE);
	manager->priv->active_notebook = notebook;
}

GtkWidget *
gedit_tab_manager_new ()
{
	return GTK_WIDGET (g_object_new (GEDIT_TYPE_TAB_MANAGER, NULL));
}

GeditNotebook *
gedit_tab_manager_get_active_notebook (GeditTabManager *manager)
{
	g_return_val_if_fail (GEDIT_IS_TAB_MANAGER (manager), NULL);

	return manager->priv->active_notebook;
}

GeditNotebook *
gedit_tab_manager_split (GeditTabManager *manager)
{
	GtkWidget *notebook;

	g_return_val_if_fail (GEDIT_IS_TAB_MANAGER (manager), NULL);

	notebook = gedit_notebook_new ();
	gtk_widget_show (notebook);

	add_notebook (manager, notebook, FALSE);

	return GEDIT_NOTEBOOK (notebook);
}

void
gedit_tab_manager_remove_page (GeditTabManager *manager,
			       GeditPage       *page)
{
	GeditNotebook *notebook;

	
}

void
gedit_tab_manager_remove_all_pages (GeditTabManager *manager)
{
	
}

GList *
gedit_tab_manager_get_pages (GeditTabManager *manager)
{
	g_return_val_if_fail (GEDIT_IS_TAB_MANAGER (manager), NULL);

	/* FIXME: Iterate all notebooks */
	return gtk_container_get_children (GTK_CONTAINER (manager->priv->active_notebook));
}
