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
