/*
 * gedit-tab.c
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
 
#include "gedit-tab.h"

#define GEDIT_TAB_GET_PRIVATE(object)(G_TYPE_INSTANCE_GET_PRIVATE ((object), GEDIT_TYPE_TAB, GeditTabPrivate))

struct _GeditTabPrivate
{
	GtkWidget *view;
};

G_DEFINE_TYPE(GeditTab, gedit_tab, GTK_TYPE_BIN)


static void
gedit_tab_finalize (GObject *object)
{
	/* GeditTab *tab = GEDIT_TAB (object); */

	G_OBJECT_CLASS (gedit_tab_parent_class)->finalize (object);
}

static void 
gedit_tab_class_init (GeditTabClass *klass)
{
	GObjectClass *object_class = G_OBJECT_CLASS (klass);

	object_class->finalize = gedit_tab_finalize;
	
	g_type_class_add_private (object_class, sizeof(GeditTabPrivate));
}

static void
gedit_tab_init (GeditTab *tab)
{
	GtkWidget *sw;
	GeditDocument *doc;
	
	tab->priv = GEDIT_TAB_GET_PRIVATE (tab);
	
	/* Create the scrolled window */
	sw = gtk_scrolled_window_new (NULL, NULL);
	g_return_if_fail (sw != NULL);
	
	gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW (sw),
					GTK_POLICY_AUTOMATIC,
					GTK_POLICY_AUTOMATIC);
					
	/* Create the view */
	doc = gedit_document_new ();
	tab->priv->view = gedit_view_new (doc);

	gtk_container_add (GTK_CONTAINER (tab), sw);
	gtk_container_add (GTK_CONTAINER (sw), tab->priv->view);
	gtk_scrolled_window_set_shadow_type (GTK_SCROLLED_WINDOW (sw),
                                             GTK_SHADOW_IN);	
	gtk_widget_show_all (sw);                                             
}

GtkWidget *
gedit_tab_new (void)
{
	return GTK_WIDGET (g_object_new (GEDIT_TYPE_TAB, NULL));
}

/* Whether create is TRUE, creates a new empty document if location does 
   not refer to an existing file */
GtkWidget *
gedit_tab_new_from_uri (const gchar *location,
			gboolean     create)
{
	GeditTab *tab;
	GeditDocument *doc;
	
	g_return_val_if_fail (location != NULL, NULL);
	
	tab = GEDIT_TAB (gedit_tab_new ());
	
	doc = gedit_tab_get_document (tab);

	// TODO
	
	return NULL;
}		

GeditView *
gedit_tag_get_view (GeditTab *tab)
{
	return GEDIT_VIEW (tab->priv->view);
}

/* This is only an helper function */
GeditDocument *
gedit_tab_get_document (GeditTab *tab)
{
	return GEDIT_DOCUMENT (gtk_text_view_get_buffer (
					GTK_TEXT_VIEW (tab->priv->view)));
}
