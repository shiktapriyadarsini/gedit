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
 *
 * $Id$
 */
 
#include "gedit-panel.h"

#include <glib/gi18n.h>
#include <gtk/gtk.h>
#include <gdk/gdk.h>
#include <gdk/gdkkeysyms.h>

#define PANEL_ITEM_KEY "GeditPanelItemKey"

struct _GeditPanelPrivate 
{
	/* Title bar */
	GtkWidget *close_button;
	GtkWidget *title_image;
	GtkWidget *title_label;
	
	/* Notebook */
	GtkWidget *notebook;
	
	GtkTooltips *tooltips;
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

/* Signals */
enum {
	CLOSE,
	LAST_SIGNAL
};

static guint signals[LAST_SIGNAL];

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
	GeditPanel *panel = GEDIT_PANEL (obj);
	
	g_object_unref (panel->priv->tooltips);

	if (G_OBJECT_CLASS (parent_class)->finalize)
		(*G_OBJECT_CLASS (parent_class)->finalize) (obj);
}

static void
gedit_panel_close (GeditPanel *panel)
{
	gtk_widget_hide (GTK_WIDGET (panel));
}

static void
gedit_panel_grab_focus (GtkWidget *w)
{
	gint n;
	GtkWidget *tab;
	GeditPanel *panel = GEDIT_PANEL (w);
	
	g_print ("gedit_panel_grab_focus\n");
	
	n = gtk_notebook_get_current_page (GTK_NOTEBOOK (panel->priv->notebook));
	if (n == -1)
		return;
		
	tab = gtk_notebook_get_nth_page (GTK_NOTEBOOK (panel->priv->notebook),
					 n);
	g_return_if_fail (tab != NULL);
	
	gtk_widget_grab_focus (tab);
}

static void
gedit_panel_class_init (GeditPanelClass *klass)
{
	GtkBindingSet *binding_set;
	GObjectClass *g_object_class = G_OBJECT_CLASS (klass);
	GtkWidgetClass *widget_class = GTK_WIDGET_CLASS (klass);
	
	g_type_class_add_private (klass, sizeof (GeditPanelPrivate));

	parent_class = g_type_class_ref (GTK_TYPE_VBOX);

	g_object_class->finalize = gedit_panel_finalize;
	
	widget_class->grab_focus = gedit_panel_grab_focus;
	
	klass->close = gedit_panel_close;
	
	signals[CLOSE] =  g_signal_new ("close",
					G_OBJECT_CLASS_TYPE (klass),
					G_SIGNAL_RUN_LAST | G_SIGNAL_ACTION,
					G_STRUCT_OFFSET (GeditPanelClass, close),
		  			NULL, NULL,
		  			g_cclosure_marshal_VOID__VOID,
					G_TYPE_NONE, 0);
					
	binding_set = gtk_binding_set_by_class (klass);
  
	gtk_binding_entry_add_signal (binding_set, GDK_Escape, 0, "close", 0);
}

#define TAB_WIDTH_N_CHARS 10

static void
tab_label_style_set_cb (GtkWidget *label,
			GtkStyle  *previous_style,
			GtkWidget *hbox)
{
	PangoFontMetrics *metrics;
	PangoContext *context;
	gint char_width, h, w, len;
	const gchar *str;

	context = gtk_widget_get_pango_context (label);
	metrics = pango_context_get_metrics (context,
			                     label->style->font_desc,
					     pango_context_get_language (context));

	char_width = pango_font_metrics_get_approximate_digit_width (metrics);
	pango_font_metrics_unref (metrics);

	str = gtk_label_get_text (GTK_LABEL (label));
	len = g_utf8_strlen (str, -1);
	
	gtk_icon_size_lookup_for_settings (gtk_widget_get_settings (label),
					   GTK_ICON_SIZE_MENU, &w, &h);

	if (GTK_WIDGET_VISIBLE (label))
		gtk_widget_set_size_request
			(hbox, MIN (TAB_WIDTH_N_CHARS, len) * PANGO_PIXELS(char_width) + w, -1);
	else
		gtk_widget_set_size_request (hbox, w, -1);
}

static void
notebook_page_changed (GtkNotebook     *notebook,
                       GtkNotebookPage *page,
                       guint            page_num,
                       GeditPanel      *panel)
{
	GtkWidget *item;
	GeditPanelItem *data;
	gint i;
	gint pages;
	
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
	
	pages = gtk_notebook_get_n_pages (notebook);
	for (i = 0; i < pages; ++i)
	{
		GtkWidget *p;
		GtkWidget *label;
		GtkWidget *hbox;
		
		p = gtk_notebook_get_nth_page (notebook, i);
		
		label = GTK_WIDGET (g_object_get_data (G_OBJECT (p), "label"));
		
		if (p != item)
			gtk_widget_hide (label);
		else
			gtk_widget_show (label);
			
		hbox = GTK_WIDGET (g_object_get_data (G_OBJECT (p), "hbox"));
		
		tab_label_style_set_cb (label, NULL, hbox);
	}
}


static void
panel_show (GeditPanel *panel,
	    gpointer    user_data)
{
	gint page;
	GtkNotebook *nb;

	nb = GTK_NOTEBOOK (panel->priv->notebook);
	
	page = gtk_notebook_get_current_page (nb);
	
	notebook_page_changed (nb, NULL, page, panel);
}

static void
close_button_clicked_cb (GtkWidget *widget, GtkWidget *panel)
{
	gtk_widget_hide (panel);
}

static void
gedit_panel_init (GeditPanel *panel)
{
	GtkWidget *title_hbox;
	GtkWidget *icon_name_hbox;
	GtkWidget *image;
	GtkWidget *dummy_label;
	GtkSettings *settings;
	gint w, h;
	
	panel->priv = G_TYPE_INSTANCE_GET_PRIVATE (panel, 
						   GEDIT_TYPE_PANEL,
					 	   GeditPanelPrivate);
	g_return_if_fail (panel->priv != NULL);	
	
	panel->priv->tooltips = gtk_tooltips_new ();
	g_object_ref (G_OBJECT (panel->priv->tooltips));
	gtk_object_sink (GTK_OBJECT (panel->priv->tooltips));
	
	/* Create title hbox */
	title_hbox = gtk_hbox_new (FALSE, 12);
	gtk_container_set_border_width (GTK_CONTAINER (title_hbox),
					6);
					
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

	gtk_box_pack_start (GTK_BOX (icon_name_hbox), 
			    panel->priv->title_image, 
			    FALSE, 
			    FALSE, 
			    0);
	
	dummy_label = gtk_label_new (" ");
	
	gtk_box_pack_start (GTK_BOX (icon_name_hbox), 
			    dummy_label, 
			    FALSE, 
			    FALSE, 
			    0);	
			    
	panel->priv->title_label = gtk_label_new (_("Empty"));

	/* FIXME: elipsize */

	gtk_box_pack_start (GTK_BOX (icon_name_hbox), 
			    panel->priv->title_label, 
			    FALSE, 
			    FALSE, 
			    0);			   
	
	panel->priv->close_button = gtk_button_new ();						    
	gtk_button_set_relief (GTK_BUTTON (panel->priv->close_button), 
			       GTK_RELIEF_NONE);
	/* fetch the size of an icon */
	settings = gtk_widget_get_settings (GTK_WIDGET (panel));
	gtk_icon_size_lookup_for_settings (settings,
					   GTK_ICON_SIZE_MENU,
					   &w, &h);
	image = gtk_image_new_from_stock (GTK_STOCK_CLOSE, GTK_ICON_SIZE_MENU);
	gtk_widget_set_size_request (panel->priv->close_button, w + 2, h + 2); 
	gtk_container_add (GTK_CONTAINER (panel->priv->close_button), image);
	gtk_button_set_focus_on_click (GTK_BUTTON (panel->priv->close_button), 
				       FALSE);
				       
	gtk_box_pack_start (GTK_BOX (title_hbox), 
			    panel->priv->close_button, 
			    FALSE, 
			    FALSE, 
			    0);
	
	gtk_tooltips_set_tip (panel->priv->tooltips, 
			      panel->priv->close_button,
			      _("Hide panel"), 
			      NULL);

	g_signal_connect (G_OBJECT (panel->priv->close_button), 
			  "clicked",
                          G_CALLBACK (close_button_clicked_cb),
                          panel);
                          
	gtk_widget_show_all (GTK_WIDGET (title_hbox));                          
                                
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
  	
  	gtk_widget_show (GTK_WIDGET (panel->priv->notebook));
  	
  	g_signal_connect (panel->priv->notebook,
  			  "switch-page",
  			  G_CALLBACK (notebook_page_changed),
  			  panel);
  			  
	g_signal_connect (panel,
			  "show",
			  G_CALLBACK (panel_show),
			  NULL);
}

GtkWidget *
gedit_panel_new (void)
{
	return GTK_WIDGET (g_object_new (GEDIT_TYPE_PANEL, NULL));
}

static GtkWidget *
build_tab_label (GeditPanel  *panel,
		 GtkWidget   *item,
		 const gchar *name,
		 const gchar *stock_id)
{
	GtkWidget *hbox, *label_hbox, *label_ebox;
	GtkWidget *label, *icon;

	/* set hbox spacing and label padding (see below) so that there's an
	 * equal amount of space around the label */
	hbox = gtk_hbox_new (FALSE, 4);

	label_ebox = gtk_event_box_new ();
	gtk_event_box_set_visible_window (GTK_EVENT_BOX (label_ebox), FALSE);
	gtk_box_pack_start (GTK_BOX (hbox), label_ebox, TRUE, TRUE, 0);

	label_hbox = gtk_hbox_new (FALSE, 4);
	gtk_container_add (GTK_CONTAINER (label_ebox), label_hbox);
	
	/* setup site icon, empty by default */
	icon = gtk_image_new ();
	if (stock_id == NULL)
	{
		gtk_image_set_from_stock (GTK_IMAGE (icon),
					  GTK_STOCK_FILE,
					  GTK_ICON_SIZE_MENU);
	}
	else
	{
		gtk_image_set_from_stock (GTK_IMAGE (icon),
					  stock_id,
					  GTK_ICON_SIZE_MENU);
	}
	gtk_box_pack_start (GTK_BOX (label_hbox), icon, FALSE, FALSE, 0);

	/* setup label */
        label = gtk_label_new (name);
	gtk_label_set_ellipsize (GTK_LABEL (label), PANGO_ELLIPSIZE_END);
	gtk_misc_set_alignment (GTK_MISC (label), 0.0, 0.5);
        gtk_misc_set_padding (GTK_MISC (label), 0, 0);
	gtk_box_pack_start (GTK_BOX (label_hbox), label, TRUE, TRUE, 0);

	/* Set minimal size */
	g_signal_connect (label, "style-set",
			  G_CALLBACK (tab_label_style_set_cb), hbox);

	gtk_tooltips_set_tip (panel->priv->tooltips,
			      label_ebox,
			      name,
			      NULL);
			      
	gtk_widget_show_all (hbox);
	
	g_object_set_data (G_OBJECT (item), "label", label);
	g_object_set_data (G_OBJECT (item), "hbox", hbox);
	g_object_set_data (G_OBJECT (item), "label-ebox", label_ebox);

	return hbox;
}

void
gedit_panel_add_item (GeditPanel  *panel, 
		      GtkWidget   *item, 
		      const gchar *name,
		      const gchar *stock_id)
{
	GeditPanelItem *data;
	GtkWidget *tab_label;
	GtkWidget *menu_label;

	g_return_if_fail (GEDIT_IS_PANEL (panel));
	g_return_if_fail (GTK_IS_WIDGET (item));
	g_return_if_fail (name != NULL);

	data = g_new (GeditPanelItem, 1);
	
	data->name = g_strdup (name);
	data->stock_id = g_strdup (stock_id);
	
	g_object_set_data (G_OBJECT (item),
		           PANEL_ITEM_KEY,
		           data);
		      
	tab_label = build_tab_label (panel, item, name, stock_id);
					 
	menu_label = gtk_label_new (name);
	gtk_misc_set_alignment (GTK_MISC (menu_label), 0.0, 0.5);
	
	if (!GTK_WIDGET_VISIBLE (item))
		gtk_widget_show (item);
		
	gtk_notebook_append_page_menu (GTK_NOTEBOOK (panel->priv->notebook),
				       item,
				       tab_label,
				       menu_label);		
}

gboolean
gedit_panel_remove_item (GeditPanel *panel, GtkWidget *item)
{
	GeditPanelItem *data;
	gint page_num;
	GtkWidget *ebox;
	
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
		           
	ebox = g_object_get_data (G_OBJECT (item), "label-ebox");
	gtk_tooltips_set_tip (GTK_TOOLTIPS (panel->priv->tooltips), 
			      ebox, 
			      NULL, 
			      NULL);  
	
	gtk_notebook_remove_page (GTK_NOTEBOOK (panel->priv->notebook), 
				  page_num);

	return TRUE;
}

gboolean
gedit_panel_activate_item (GeditPanel *panel, GtkWidget *item)
{
	gint page_num;
	
	g_return_val_if_fail (GEDIT_IS_PANEL (panel), FALSE);
	g_return_val_if_fail (GTK_IS_WIDGET (item), FALSE);

	page_num = gtk_notebook_page_num (GTK_NOTEBOOK (panel->priv->notebook),
					  item);
					  
	if (page_num == -1)
		return FALSE;
		
	gtk_notebook_set_current_page (GTK_NOTEBOOK (panel->priv->notebook),
				       page_num);
	
	return TRUE;
}

gboolean
gedit_panel_item_is_active (GeditPanel     *panel,
			    GtkWidget      *item)
{
	gint cur_page;
	gint page_num;
	
	g_return_val_if_fail (GEDIT_IS_PANEL (panel), FALSE);
	g_return_val_if_fail (GTK_IS_WIDGET (item), FALSE);
	
	page_num = gtk_notebook_page_num (GTK_NOTEBOOK (panel->priv->notebook),
					  item);
					  
	if (page_num == -1)
		return FALSE;
		
	cur_page = gtk_notebook_get_current_page (
				GTK_NOTEBOOK (panel->priv->notebook));

	return (page_num == cur_page);
}




