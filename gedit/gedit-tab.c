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

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <glib/gi18n.h>

#include <libgnomeui/libgnomeui.h>
#include <libgnomevfs/gnome-vfs-mime-handlers.h>

#include "gedit-tab.h"
#include "gedit-utils.h"

#define GEDIT_TAB_GET_PRIVATE(object)(G_TYPE_INSTANCE_GET_PRIVATE ((object), GEDIT_TYPE_TAB, GeditTabPrivate))

#define GEDIT_TAB_KEY "GEDIT_TAB_KEY"

struct _GeditTabPrivate
{
	GtkWidget *view;
	
	GtkWidget *message_area;
};

G_DEFINE_TYPE(GeditTab, gedit_tab, GTK_TYPE_VBOX)

enum
{
	PROP_0,
	PROP_NAME,
};

static void
gedit_tab_set_property (GObject      *object,
		        guint         prop_id,
		        const GValue *value,
		        GParamSpec   *pspec)
{
	/* GeditTab *tab = GEDIT_TAB (object); */

	switch (prop_id)

	{
		/* All properties are READONLY */
		default:
			G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
			break;
	}
}

static void
gedit_tab_get_property (GObject    *object,
		        guint       prop_id,
		        GValue     *value,
		        GParamSpec *pspec)
{
	GeditTab *tab = GEDIT_TAB (object);

	switch (prop_id)
	{
		case PROP_NAME:
			g_value_take_string (value,
					     _gedit_tab_get_name (tab));
			break;
		default:
			G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
			break;			
	}
}

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
	object_class->get_property = gedit_tab_get_property;
	object_class->set_property = gedit_tab_set_property;
	
	g_object_class_install_property (object_class,
					 PROP_NAME,
					 g_param_spec_string ("name",
							      "Name",
							      "The tab's name",
							      "",
							      G_PARAM_READABLE));	
							      
	g_type_class_add_private (object_class, sizeof(GeditTabPrivate));
}

static void 
document_name_modified_changed (GeditDocument *document, GeditTab *tab)
{
	g_object_notify (G_OBJECT (tab), "name");
}

#if 0
static GtkWidget *
create_progress_message_area_content ()
{
  GtkWidget *hbox1;
  GtkWidget *vbox1;
  GtkWidget *hbox2;
  GtkWidget *image1;
  GtkWidget *label1;
  GtkWidget *progressbar1;
  GtkWidget *vbox2;
  GtkWidget *button1;
  
  hbox1 = gtk_hbox_new (FALSE, 16);
  gtk_widget_show (hbox1);
  gtk_container_set_border_width (GTK_CONTAINER (hbox1), 6);

  vbox1 = gtk_vbox_new (FALSE, 4);
  gtk_widget_show (vbox1);
  gtk_box_pack_start (GTK_BOX (hbox1), vbox1, TRUE, TRUE, 0);

  hbox2 = gtk_hbox_new (FALSE, 8);
  gtk_widget_show (hbox2);
  gtk_box_pack_start (GTK_BOX (vbox1), hbox2, FALSE, FALSE, 0);

  image1 = gtk_image_new_from_icon_name (GTK_STOCK_OPEN, GTK_ICON_SIZE_SMALL_TOOLBAR);
  gtk_widget_show (image1);
  gtk_box_pack_start (GTK_BOX (hbox2), image1, FALSE, FALSE, 0);

  label1 = gtk_label_new (_("Loading <b>pippo.txt</b> from <b>http://www.gnome.org/~paolo/</b>"));
  gtk_widget_show (label1);
  gtk_box_pack_start (GTK_BOX (hbox2), label1, TRUE, TRUE, 0);
  gtk_label_set_use_markup (GTK_LABEL (label1), TRUE);
  gtk_misc_set_alignment (GTK_MISC (label1), 0, 0.5);
  gtk_label_set_ellipsize (GTK_LABEL (label1), PANGO_ELLIPSIZE_END);
  gtk_label_set_single_line_mode (GTK_LABEL (label1), TRUE);

  progressbar1 = gtk_progress_bar_new ();
  gtk_widget_show (progressbar1);
  gtk_box_pack_start (GTK_BOX (vbox1), progressbar1, TRUE, FALSE, 0);
  gtk_widget_set_size_request (progressbar1, -1, 15);
  gtk_progress_bar_set_fraction (GTK_PROGRESS_BAR (progressbar1), 0.5);
  gtk_progress_bar_set_ellipsize (GTK_PROGRESS_BAR (progressbar1), PANGO_ELLIPSIZE_MIDDLE);

  vbox2 = gtk_vbox_new (TRUE, 0);
  gtk_widget_show (vbox2);
  gtk_box_pack_start (GTK_BOX (hbox1), vbox2, FALSE, TRUE, 0);

  button1 = gtk_button_new_from_stock ("gtk-cancel");
  gtk_widget_show (button1);
  gtk_box_pack_start (GTK_BOX (vbox2), button1, FALSE, FALSE, 0);
  gtk_button_set_relief (GTK_BUTTON (button1), GTK_RELIEF_NONE);
  
  return hbox1;
}

#endif
#if 0
static GtkWidget *
create_error_message_area_content (void)
{
  GtkWidget *hbox3;
  GtkWidget *hbox4;
  GtkWidget *image2;
  GtkWidget *vbox5;
  GtkWidget *label2;
  GtkWidget *label3;
  GtkWidget *vbox4;
  GtkWidget *button2;

  hbox3 = gtk_hbox_new (FALSE, 16);
  gtk_widget_show (hbox3);
  gtk_container_set_border_width (GTK_CONTAINER (hbox3), 6);

  hbox4 = gtk_hbox_new (FALSE, 8);
  gtk_widget_show (hbox4);
  gtk_box_pack_start (GTK_BOX (hbox3), hbox4, TRUE, TRUE, 0);

  image2 = gtk_image_new_from_stock ("gtk-dialog-error", GTK_ICON_SIZE_DIALOG);
  gtk_widget_show (image2);
  gtk_box_pack_start (GTK_BOX (hbox4), image2, FALSE, FALSE, 0);
  gtk_misc_set_alignment (GTK_MISC (image2), 0.5, 0);

  vbox5 = gtk_vbox_new (FALSE, 6);
  gtk_widget_show (vbox5);
  gtk_box_pack_start (GTK_BOX (hbox4), vbox5, TRUE, TRUE, 0);

  label2 = gtk_label_new (_("<b>Could not find the file <i>http://www.gnome.org/pippo.txt</i></b>"));
  gtk_widget_show (label2);
  gtk_box_pack_start (GTK_BOX (vbox5), label2, TRUE, TRUE, 0);
  gtk_label_set_use_markup (GTK_LABEL (label2), TRUE);
  gtk_label_set_line_wrap (GTK_LABEL (label2), TRUE);
  gtk_misc_set_alignment (GTK_MISC (label2), 0, 0.5);
  GTK_WIDGET_SET_FLAGS (label2, GTK_CAN_FOCUS);
  gtk_label_set_selectable (GTK_LABEL (label2), TRUE);
  
  label3 = gtk_label_new (_("<small>Please, check that you typed the location correctly and try again.</small>"));
  gtk_widget_show (label3);
  gtk_box_pack_start (GTK_BOX (vbox5), label3, TRUE, TRUE, 0);
  GTK_WIDGET_SET_FLAGS (label3, GTK_CAN_FOCUS);
  gtk_label_set_use_markup (GTK_LABEL (label3), TRUE);
  gtk_label_set_line_wrap (GTK_LABEL (label3), TRUE);
  gtk_label_set_selectable (GTK_LABEL (label3), TRUE);
  gtk_misc_set_alignment (GTK_MISC (label3), 0, 0.5);

  vbox4 = gtk_vbox_new (TRUE, 0);
  gtk_widget_show (vbox4);
  gtk_box_pack_start (GTK_BOX (hbox3), vbox4, FALSE, TRUE, 0);

  button2 = gtk_button_new_from_stock ("gtk-close");
  gtk_widget_show (button2);
  gtk_box_pack_start (GTK_BOX (vbox4), button2, FALSE, FALSE, 0);
  gtk_button_set_relief (GTK_BUTTON (button2), GTK_RELIEF_NONE);

  return hbox3;
}


static GtkWidget *
create_retry_message_area_content (void)
{
  GtkWidget *hbox5;
  GtkWidget *hbox6;
  GtkWidget *image3;
  GtkWidget *vbox6;
  GtkWidget *label4;
  GtkWidget *label5;
  GtkWidget *hbox9;
  GtkWidget *label8;
  GtkWidget *combobox1;
  GtkWidget *vbox7;
  GtkWidget *button3;
  GtkWidget *alignment1;
  GtkWidget *hbox10;
  GtkWidget *image4;
  GtkWidget *label9;
  GtkWidget *button4;

  hbox5 = gtk_hbox_new (FALSE, 16);
  gtk_widget_show (hbox5);
  gtk_container_set_border_width (GTK_CONTAINER (hbox5), 6);
  gtk_container_set_resize_mode (GTK_CONTAINER (hbox5), GTK_RESIZE_QUEUE);
  
  hbox6 = gtk_hbox_new (FALSE, 8);
  gtk_widget_show (hbox6);
  gtk_box_pack_start (GTK_BOX (hbox5), hbox6, TRUE, TRUE, 0);

  image3 = gtk_image_new_from_stock ("gtk-dialog-error", GTK_ICON_SIZE_DIALOG);
  gtk_widget_show (image3);
  gtk_box_pack_start (GTK_BOX (hbox6), image3, FALSE, FALSE, 0);
  gtk_misc_set_alignment (GTK_MISC (image3), 0.5, 0);

  vbox6 = gtk_vbox_new (FALSE, 6);
  gtk_widget_show (vbox6);
  gtk_box_pack_start (GTK_BOX (hbox6), vbox6, TRUE, TRUE, 0);

  label4 = gtk_label_new (_("<b>Could not open the file <i>/gnome/gnome-210/\342\200\246/stock_about_16.png\"</i></b>"));
  gtk_widget_show (label4);
  gtk_box_pack_start (GTK_BOX (vbox6), label4, TRUE, TRUE, 0);
  GTK_WIDGET_SET_FLAGS (label4, GTK_CAN_FOCUS);
  gtk_label_set_use_markup (GTK_LABEL (label4), TRUE);
  gtk_label_set_line_wrap (GTK_LABEL (label4), TRUE);
  gtk_label_set_selectable (GTK_LABEL (label4), TRUE);
  gtk_misc_set_alignment (GTK_MISC (label4), 0, 0.5);

  label5 = gtk_label_new (_("<small>gedit was not able to automatically detect the character coding. Do you want to retry using the specified character coding?</small>"));
  gtk_widget_show (label5);
  gtk_box_pack_start (GTK_BOX (vbox6), label5, TRUE, TRUE, 0);
  GTK_WIDGET_SET_FLAGS (label5, GTK_CAN_FOCUS);
  gtk_label_set_use_markup (GTK_LABEL (label5), TRUE);
  gtk_label_set_line_wrap (GTK_LABEL (label5), TRUE);
  gtk_label_set_selectable (GTK_LABEL (label5), TRUE);
  gtk_misc_set_alignment (GTK_MISC (label5), 0, 0.5);

  hbox9 = gtk_hbox_new (FALSE, 8);
  gtk_widget_show (hbox9);
  gtk_box_pack_start (GTK_BOX (vbox6), hbox9, TRUE, TRUE, 0);

  label8 = gtk_label_new_with_mnemonic (_("<small>Ch_aracter coding:</small>"));
  gtk_widget_show (label8);
  gtk_box_pack_start (GTK_BOX (hbox9), label8, FALSE, FALSE, 0);
  gtk_label_set_use_markup (GTK_LABEL (label8), TRUE);
  gtk_misc_set_alignment (GTK_MISC (label8), 0, 0.5);

  combobox1 = gtk_combo_box_new_text ();
  gtk_widget_show (combobox1);
  gtk_box_pack_start (GTK_BOX (hbox9), combobox1, TRUE, TRUE, 0);
  gtk_combo_box_append_text (GTK_COMBO_BOX (combobox1), _("Western (ISO-8859-15)"));
  gtk_combo_box_append_text (GTK_COMBO_BOX (combobox1), _("Unicode (UTF-16)"));

  vbox7 = gtk_vbox_new (FALSE, 8);
  gtk_widget_show (vbox7);
  gtk_box_pack_start (GTK_BOX (hbox5), vbox7, FALSE, TRUE, 0);

  button3 = gtk_button_new ();
  gtk_widget_show (button3);
  gtk_box_pack_start (GTK_BOX (vbox7), button3, FALSE, FALSE, 0);
  gtk_button_set_relief (GTK_BUTTON (button3), GTK_RELIEF_NONE);

  alignment1 = gtk_alignment_new (0.5, 0.5, 0, 0);
  gtk_widget_show (alignment1);
  gtk_container_add (GTK_CONTAINER (button3), alignment1);

  hbox10 = gtk_hbox_new (TRUE, 2);
  gtk_widget_show (hbox10);
  gtk_container_add (GTK_CONTAINER (alignment1), hbox10);

  image4 = gtk_image_new_from_stock ("gtk-refresh", GTK_ICON_SIZE_BUTTON);
  gtk_widget_show (image4);
  gtk_box_pack_start (GTK_BOX (hbox10), image4, FALSE, FALSE, 0);

  label9 = gtk_label_new_with_mnemonic ("_Retry");
  gtk_widget_show (label9);
  gtk_box_pack_start (GTK_BOX (hbox10), label9, FALSE, FALSE, 0);

  button4 = gtk_button_new_from_stock ("gtk-close");
  gtk_widget_show (button4);
  gtk_box_pack_start (GTK_BOX (vbox7), button4, FALSE, FALSE, 0);
  gtk_button_set_relief (GTK_BUTTON (button4), GTK_RELIEF_NONE);
  
  return hbox5;
}

static void
create_test_message_area (GeditTab *tab)
{
	GdkColor color;
	GtkWidget *eb;
	
	gdk_color_parse ("#E1C43A", &color);

	eb = gtk_event_box_new ();
	gtk_widget_modify_bg (eb, GTK_STATE_NORMAL, &color);
	gtk_container_set_border_width (GTK_CONTAINER (eb), 
					1);		
					
	gtk_box_pack_start (GTK_BOX (tab), 
			    eb, 
			    FALSE, 
			    FALSE, 
			    0);		
			    			
	tab->priv->message_area = gtk_event_box_new ();
	gdk_color_parse ("#FFF1BE", &color);
	gtk_widget_modify_bg (tab->priv->message_area, GTK_STATE_NORMAL, &color);
	
	gtk_container_set_border_width (GTK_CONTAINER (tab->priv->message_area), 
					1);

	gtk_container_add (GTK_CONTAINER (eb), tab->priv->message_area);
	

	gtk_container_add (GTK_CONTAINER (tab->priv->message_area),
			   create_retry_message_area_content ());

			    
	gtk_widget_show_all (eb);
}
#endif
static void
gedit_tab_init (GeditTab *tab)
{
	GtkWidget *sw;
	GeditDocument *doc;
	
	tab->priv = GEDIT_TAB_GET_PRIVATE (tab);
#if 0	
	create_test_message_area (tab);
#endif			  
	/* Create the scrolled window */
	sw = gtk_scrolled_window_new (NULL, NULL);
	
	gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW (sw),
					GTK_POLICY_AUTOMATIC,
					GTK_POLICY_AUTOMATIC);
					
	/* Create the view */
	doc = gedit_document_new ();
	g_object_set_data (G_OBJECT (doc), GEDIT_TAB_KEY, tab);
	
	tab->priv->view = gedit_view_new (doc);
	g_object_unref (doc);
	gtk_widget_show (tab->priv->view);
	g_object_set_data (G_OBJECT (tab->priv->view), GEDIT_TAB_KEY, tab);
	
	gtk_box_pack_start (GTK_BOX (tab), sw, TRUE, TRUE, 0);
	gtk_container_add (GTK_CONTAINER (sw), tab->priv->view);
	gtk_scrolled_window_set_shadow_type (GTK_SCROLLED_WINDOW (sw),
                                             GTK_SHADOW_IN);	
	gtk_widget_show (sw);
	
	g_signal_connect (G_OBJECT (doc),
			  "name_changed",
			  G_CALLBACK (document_name_modified_changed),
			  tab);
	g_signal_connect (G_OBJECT (doc),
			  "modified_changed",
			  G_CALLBACK (document_name_modified_changed),
			  tab);			                                   
}

GtkWidget *
_gedit_tab_new (void)
{
	return GTK_WIDGET (g_object_new (GEDIT_TYPE_TAB, NULL));
}

/* Whether create is TRUE, creates a new empty document if location does 
   not refer to an existing file */
GtkWidget *
_gedit_tab_new_from_uri (const gchar *location,
			gboolean     create)
{
	GeditTab *tab;
	GeditDocument *doc;
	
	g_return_val_if_fail (location != NULL, NULL);
	
	tab = GEDIT_TAB (_gedit_tab_new ());
	
	doc = gedit_tab_get_document (tab);

	// TODO
	
	return NULL;
}		

GeditView *
gedit_tab_get_view (GeditTab *tab)
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

#define MAX_DOC_NAME_LENGTH 40

gchar *
_gedit_tab_get_name (GeditTab *tab)
{
	GeditDocument *doc;
	const gchar* name = NULL;
	gchar* docname = NULL;
	gchar* tab_name = NULL;

	g_return_val_if_fail (GEDIT_IS_TAB (tab), NULL);

	doc = gedit_tab_get_document (tab);
	
	name = gedit_document_get_short_name_for_display (doc);

	/* Truncate the name so it doesn't get insanely wide. */
	docname = gedit_utils_str_middle_truncate (name, MAX_DOC_NAME_LENGTH);

	if (gtk_text_buffer_get_modified (GTK_TEXT_BUFFER (doc)))
	{
		tab_name = g_strdup_printf ("*%s", docname);
	} 
	else 
	{
 #if 0		
		if (gedit_document_is_readonly (doc)) 
		{
			tab_name = g_strdup_printf ("%s [%s]", docname, 
						/*Read only*/ _("RO"));
		} 
		else 
		{
			tab_name = g_strdup_printf ("%s", docname);
		}
#endif
		tab_name = g_strdup_printf ("%s", docname);

	}
	
	g_free (docname);

	return tab_name;
}

gchar *
_gedit_tab_get_tooltips	(GeditTab *tab)
{
	GeditDocument *doc;
	gchar *tip;
	const gchar *uri;
	gchar *ruri;
	const gchar *mime_type;
	const gchar *mime_description = NULL;
	gchar *mime_full_description; 
	gchar *encoding;
	const GeditEncoding *enc;
	
	g_return_val_if_fail (GEDIT_IS_TAB (tab), NULL);

	doc = gedit_tab_get_document (tab);
	
	uri = gedit_document_get_uri_for_display (doc);
	g_return_val_if_fail (uri != NULL, NULL);

	mime_type = gedit_document_get_mime_type (doc);
	mime_description = gnome_vfs_mime_get_description (mime_type);

	if (mime_description == NULL)
		mime_full_description = g_strdup (mime_type);
	else
		mime_full_description = g_strdup_printf ("%s (%s)", 
				mime_description, mime_type);

	enc = gedit_document_get_encoding (doc);

	if (enc == NULL)
		encoding = g_strdup (_("Unicode (UTF-8)"));
	else
		encoding = gedit_encoding_to_string (enc);
	
	ruri = 	gedit_utils_replace_home_dir_with_tilde (uri);

	tip =  g_markup_printf_escaped("<b>%s</b> %s\n\n"
				       "<b>%s</b> %s\n"
				       "<b>%s</b> %s",
				       _("Name:"), ruri,
				       _("MIME Type:"), mime_full_description,
				       _("Encoding:"), encoding);

	g_free (ruri);
	g_free (encoding);
	g_free (mime_full_description);
	
	return tip;
}

static GdkPixbuf *
get_icon (GtkIconTheme *theme, 
	  const gchar  *uri,
	  const gchar  *mime_type, 
	  gint          size)
{
	gchar *icon;
	GdkPixbuf *pixbuf;
	guint width, height;
	
	icon = gnome_icon_lookup (theme, NULL, uri, NULL, NULL,
				  mime_type, 0, NULL);
	

	g_return_val_if_fail (icon != NULL, NULL);

	pixbuf = gtk_icon_theme_load_icon (theme, icon, size, 0, NULL);
	g_free (icon);
	if (pixbuf == NULL)
		return NULL;
		
	width = gdk_pixbuf_get_width (pixbuf); 
	height = gdk_pixbuf_get_height (pixbuf);
	/* if the icon is larger than the nominal size, scale down */
	if (MAX (width, height) > size) 
	{
		GdkPixbuf *scaled_pixbuf;
		
		if (width > height) 
		{
			height = height * size / width;
			width = size;
		} 
		else 
		{
			width = width * size / height;
			height = size;
		}
		
		scaled_pixbuf = gdk_pixbuf_scale_simple	(pixbuf, 
							 width, 
							 height, 
							 GDK_INTERP_BILINEAR);
		g_object_unref (pixbuf);
		pixbuf = scaled_pixbuf;
	}
	
	return pixbuf;
}

/* FIXME: add support for theme changed. I think it should be as easy as
   call g_object_notify (tab, "name") when the icon theme changes */
GdkPixbuf *
_gedit_tab_get_icon (GeditTab *tab)
{
	GtkIconTheme *theme;
	GdkScreen *screen;
	GdkPixbuf *pixbuf;
	const gchar *raw_uri;
	const gchar *mime_type;
	gint icon_size;
	GeditDocument *doc;

	g_return_val_if_fail (GEDIT_IS_TAB (tab), NULL);

	doc = gedit_tab_get_document (tab);
	
	screen = gtk_widget_get_screen (GTK_WIDGET (tab));
	
	theme = gtk_icon_theme_get_for_screen (screen);
	g_return_val_if_fail (theme != NULL, NULL);

	raw_uri = gedit_document_get_uri_ (doc);
	mime_type = gedit_document_get_mime_type (doc);

	gtk_icon_size_lookup_for_settings (gtk_widget_get_settings (GTK_WIDGET (tab)),
					   GTK_ICON_SIZE_MENU, 
					   NULL,
					   &icon_size);
	pixbuf = get_icon (theme, raw_uri, mime_type, icon_size);

	return pixbuf;	
}

GeditTab *
gedit_tab_get_from_document (GeditDocument *doc)
{
	gpointer res;
	
	g_return_val_if_fail (GEDIT_IS_DOCUMENT (doc), NULL);
	
	res = g_object_get_data (G_OBJECT (doc), GEDIT_TAB_KEY);
	
	return (res != NULL) ? GEDIT_TAB (res) : NULL;
}
