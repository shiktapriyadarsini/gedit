/*
 * gedit-panel.h
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
 
#include "gedit-panel.h"

#include <glib/gi18n.h>
#define PANEL_ITEM_KEY "GeditPanelItemKey"

struct _GeditPanelPrivate 
{
	/* Title bar */
	GtkWidget *close_button;
	GtkWidget *title_image;
	GtkWidget *title_label;
	
	/* Notebook */
	GtkWidget *notebook;
};

typedef struct _GeditPanelItem GeditPanelItem;

struct _GeditPanelItem 
{
	gchar *name;
	gchar *stock_id;
};

/* Local prototypes */
static void gedit_panel_class_init (GeditPanelClass * c);
static void gedit_panel_init (GeditPanel * panel);

/* Pointer to the parent class */
static GtkVBoxClass *parent_class = NULL;

GType
gedit_panel_get_type (void)
{
	static GType type = 0;

	if (G_UNLIKELY (type == 0)) 
	{
		static const GTypeInfo info = 
		{
			sizeof (GeditPanelClass),
			(GBaseInitFunc) NULL,
			(GBaseFinalizeFunc) NULL,
			(GClassInitFunc) gedit_panel_class_init,
			(GClassFinalizeFunc) NULL,
			NULL /* class_data */ ,
			sizeof (GeditPanel),
			0 /* n_preallocs */ ,
			(GInstanceInitFunc) gedit_panel_init,
			NULL
		};

		type = g_type_register_static (GTK_TYPE_VBOX, 
					       "GeditPanel",
					       &info, 
					       (GTypeFlags) 0);
	}

	return type;
}

static void
gedit_panel_finalize (GObject *obj)
{
	/* GeditPanel *panel = GEDIT_PANEL (obj); */

	if (G_OBJECT_CLASS (parent_class)->finalize)
		(*G_OBJECT_CLASS (parent_class)->finalize) (obj);
}

static void
gedit_panel_class_init (GeditPanelClass *c)
{
	GObjectClass *g_object_class = G_OBJECT_CLASS (c);

	g_type_class_add_private (c, sizeof (GeditPanelPrivate));

	parent_class = g_type_class_ref (GTK_TYPE_VBOX);

	g_object_class->finalize = gedit_panel_finalize;
}

static void
notebook_page_changed (GtkNotebook     *notebook,
                       GtkNotebookPage *page,
                       guint            page_num,
                       GeditPanel      *panel)
{
	GtkWidget *item;
	GeditPanelItem *data;
	
	item = gtk_notebook_get_nth_page (notebook, page_num);
	g_return_if_fail (item != NULL);
	
	data = (GeditPanelItem *)g_object_get_data (G_OBJECT (item),
						    PANEL_ITEM_KEY);
	g_return_if_fail (data != NULL);
	
	gtk_label_set_text (GTK_LABEL (panel->priv->title_label), 
			    data->name);
	
	/* FIXME: manage the case stock_id is invalid - Paolo */		    
	if (data->stock_id == NULL)
	{
		gtk_image_set_from_stock (GTK_IMAGE (panel->priv->title_image),
					  GTK_STOCK_FILE,
					  GTK_ICON_SIZE_MENU);
	}
	else
	{
		gtk_image_set_from_stock (GTK_IMAGE (panel->priv->title_image),
					  data->stock_id,
					  GTK_ICON_SIZE_MENU);
	}
}

static void
gedit_panel_init (GeditPanel *panel)
{
	GtkWidget *title_hbox;
	GtkWidget *icon_name_hbox;
	GtkWidget *image;
	
	panel->priv = G_TYPE_INSTANCE_GET_PRIVATE (panel, 
						   GEDIT_TYPE_PANEL,
					 	   GeditPanelPrivate);
	g_return_if_fail (panel->priv != NULL);	
	
	/* Create title hbox */
	title_hbox = gtk_hbox_new (FALSE, 12);
	gtk_box_pack_start (GTK_BOX (panel), title_hbox, FALSE, FALSE, 0);
	
	icon_name_hbox = gtk_hbox_new (FALSE, 0);
	gtk_box_pack_start (GTK_BOX (title_hbox), 
			    icon_name_hbox, 
			    TRUE, 
			    TRUE, 
			    0);
	
	panel->priv->title_image = 
			gtk_image_new_from_stock (GTK_STOCK_FILE,
						  GTK_ICON_SIZE_MENU); 							     
	/* FIXME: I don't like using padding very much - Paolo */							    
	gtk_misc_set_padding (GTK_MISC (panel->priv->title_image), 6, 0);
	gtk_box_pack_start (GTK_BOX (icon_name_hbox), 
			    panel->priv->title_image, 
			    FALSE, 
			    FALSE, 
			    0);
	
	panel->priv->title_label = gtk_label_new (_("Empty"));

	gtk_box_pack_start (GTK_BOX (icon_name_hbox), 
			    panel->priv->title_label, 
			    FALSE, 
			    FALSE, 
			    0);			   
	
	panel->priv->close_button = gtk_button_new ();						    
	gtk_button_set_relief (GTK_BUTTON (panel->priv->close_button), 
			       GTK_RELIEF_NONE);
	image = gtk_image_new_from_stock (GTK_STOCK_CLOSE, GTK_ICON_SIZE_MENU); 
	gtk_container_add (GTK_CONTAINER (panel->priv->close_button), image);
	
	gtk_box_pack_start (GTK_BOX (title_hbox), 
			    panel->priv->close_button, 
			    FALSE, 
			    FALSE, 
			    0);
	
	/* Create the notebook */
	panel->priv->notebook = gtk_notebook_new ();
  	gtk_box_pack_start (GTK_BOX (panel), 
  			    panel->priv->notebook, 
  			    TRUE, 
  			    TRUE, 
  			    0);
	gtk_notebook_set_tab_pos (GTK_NOTEBOOK (panel->priv->notebook), 
				  GTK_POS_BOTTOM);		       
  	gtk_notebook_set_scrollable (GTK_NOTEBOOK (panel->priv->notebook), TRUE);
  	gtk_notebook_popup_enable (GTK_NOTEBOOK (panel->priv->notebook));
  	
  	gtk_widget_show_all (GTK_WIDGET (panel));
  	
  	g_signal_connect (G_OBJECT (panel->priv->notebook),
  			  "switch-page",
  			  G_CALLBACK (notebook_page_changed),
  			  panel);
}


GtkWidget *
gedit_panel_new (void)
{
	return GTK_WIDGET (g_object_new (GEDIT_TYPE_PANEL, NULL));
}

void
gedit_panel_add_item (GeditPanel  *panel, 
		      GtkWidget   *item, 
		      const gchar *name,
		      const gchar *stock_id)
{
	GeditPanelItem *data;
	GtkWidget *label;

	g_return_if_fail (GEDIT_IS_PANEL (panel));
	g_return_if_fail (GTK_IS_WIDGET (item));
	g_return_if_fail (name != NULL);

	data = g_new (GeditPanelItem, 1);
	
	data->name = g_strdup (name);
	data->stock_id = g_strdup (stock_id);
	
	g_object_set_data (G_OBJECT (item),
		           PANEL_ITEM_KEY,
		           data);
		      
	label = gtk_label_new (name);	
	gtk_label_set_ellipsize (GTK_LABEL (label), 
				 PANGO_ELLIPSIZE_END);
					 
	gtk_notebook_append_page (GTK_NOTEBOOK (panel->priv->notebook),
				  item,
				  GTK_WIDGET (gtk_label_new (name)));		
}

gboolean
gedit_panel_remove_item (GeditPanel *panel, GtkWidget *item)
{
	GeditPanelItem *data;
	gint page_num;
	
	g_return_val_if_fail (GEDIT_IS_PANEL (panel), FALSE);
	g_return_val_if_fail (GTK_IS_WIDGET (item), FALSE);

	page_num = gtk_notebook_page_num (GTK_NOTEBOOK (panel->priv->notebook),
					  item);
					  
	if (page_num == -1)
		return FALSE;
		
	data = (GeditPanelItem *)g_object_get_data (G_OBJECT (item),
					            PANEL_ITEM_KEY);
	g_return_val_if_fail (data != NULL, FALSE);
	
	g_free (data->name);
	g_free (data->stock_id);
	g_free (data);
	g_object_set_data (G_OBJECT (item),
		           PANEL_ITEM_KEY,
		           NULL);
		      
	gtk_notebook_remove_page (GTK_NOTEBOOK (panel->priv->notebook), 
				  page_num);

	return TRUE;
}

gboolean
gedit_panel_activate_item (GeditPanel *panel, GtkWidget *item)
{
	g_return_val_if_fail (GEDIT_IS_PANEL (panel), FALSE);
	g_return_val_if_fail (GTK_IS_WIDGET (item), FALSE);
	
	/* TODO */
	return FALSE;
}




