/*
 * gedit-statusbar.c
 * This file is part of gedit
 *
 * Copyright (C) 2005 - Paolo Borelli
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

#include <string.h>
#include <glib/gi18n.h>
#include <gtk/gtkwidget.h>
#include <gtk/gtkprogressbar.h>

#include "gedit-statusbar.h"

#define GEDIT_STATUSBAR_GET_PRIVATE(object)(G_TYPE_INSTANCE_GET_PRIVATE ((object), GEDIT_TYPE_STATUSBAR, GeditStatusbarPrivate))

struct _GeditStatusbarPrivate
{
	GtkWidget *progressbar;
	GtkWidget *overwrite_mode_statusbar;
	GtkWidget *cursor_position_statusbar;
};

G_DEFINE_TYPE(GeditStatusbar, gedit_statusbar, GTK_TYPE_STATUSBAR)

static void
gedit_statusbar_class_init (GeditStatusbarClass *klass)
{
	GObjectClass *object_class = G_OBJECT_CLASS (klass);

	g_type_class_add_private (object_class, sizeof (GeditStatusbarPrivate));
}

static void
gedit_statusbar_init (GeditStatusbar *statusbar)
{
	statusbar->priv = GEDIT_STATUSBAR_GET_PRIVATE (statusbar);

	gtk_statusbar_set_has_resize_grip (GTK_STATUSBAR (statusbar), FALSE);

	statusbar->priv->overwrite_mode_statusbar = gtk_statusbar_new ();
	gtk_widget_show (statusbar->priv->overwrite_mode_statusbar);
	gtk_widget_set_size_request (statusbar->priv->overwrite_mode_statusbar, 
				     80, 
				     10);
	
	gtk_statusbar_set_has_resize_grip (GTK_STATUSBAR (statusbar->priv->overwrite_mode_statusbar),
					   TRUE);
	gtk_box_pack_end (GTK_BOX (statusbar),
			  statusbar->priv->overwrite_mode_statusbar,
			  FALSE, TRUE, 0);

	statusbar->priv->cursor_position_statusbar = gtk_statusbar_new ();
	gtk_widget_show (statusbar->priv->cursor_position_statusbar);	
	gtk_widget_set_size_request (statusbar->priv->cursor_position_statusbar, 
				     160, 
				     10);	
	gtk_statusbar_set_has_resize_grip (GTK_STATUSBAR (statusbar->priv->cursor_position_statusbar),
					   FALSE);
	gtk_box_pack_end (GTK_BOX (statusbar),
			  statusbar->priv->cursor_position_statusbar,
			  FALSE, TRUE, 0);

	statusbar->priv->progressbar = gtk_progress_bar_new ();
	gtk_box_pack_end (GTK_BOX (statusbar),
			  statusbar->priv->progressbar,
			  FALSE, TRUE, 0);
}

/**
 * gedit_statusbar_new:
 * 
 * Creates a new #GeditStatusbar.
 * 
 * Return value: the new #GeditStatusbar object
 **/
GtkWidget *
gedit_statusbar_new (void)
{
	return GTK_WIDGET (g_object_new (GEDIT_TYPE_STATUSBAR, NULL));
}

/**
 * gedit_statusbar_set_overwrite:
 * @statusbar: an #GeditStatusbar
 * @overwrite: if the overwrite mode is set
 *
 * Sets the overwrite mode on the statusbar.
 **/
void
gedit_statusbar_set_overwrite (GeditStatusbar *statusbar, gboolean overwrite)
{
	gchar *msg;

	g_return_if_fail (GEDIT_IS_STATUSBAR (statusbar));

	gtk_statusbar_pop (GTK_STATUSBAR (statusbar->priv->overwrite_mode_statusbar), 0); 

	if (overwrite)
		msg = g_strdup (_("  OVR"));
	else
		msg = g_strdup (_("  INS"));

	gtk_statusbar_push (GTK_STATUSBAR (statusbar->priv->overwrite_mode_statusbar), 0, msg);

      	g_free (msg);
}

/**
 * gedit_statusbar_cursor_position:
 * @statusbar: an #GeditStatusbar
 * @line: line position
 * @col: column position
 *
 * Sets the cursor position on the statusbar.
 **/
void
gedit_statusbar_set_cursor_position (GeditStatusbar *statusbar,
				     gint line,
				     gint col)
{
	gchar *msg;

	g_return_if_fail (GEDIT_IS_STATUSBAR (statusbar));

	gtk_statusbar_pop (GTK_STATUSBAR (statusbar->priv->cursor_position_statusbar), 0); 

	if ((line == -1) && (col == -1))
		return;
		
	/* Translators: "Ln" is an abbreviation for "Line", Col is an abbreviation for "Column". Please,
	use abbreviations if possible to avoid space problems. */
	msg = g_strdup_printf (_("  Ln %d, Col %d"), line, col);

	gtk_statusbar_push (GTK_STATUSBAR (statusbar->priv->cursor_position_statusbar), 0, msg);

      	g_free (msg);
}

/**
 * gedit_statusbar_get_progress:
 * @statusbar: a #GeditStatusbar
 * 
 * Gets the statusbar's progressbar.
 **/
GtkWidget *
gedit_statusbar_get_progress (GeditStatusbar *statusbar)
{
	g_return_val_if_fail (GEDIT_IS_STATUSBAR (statusbar), NULL);

	return statusbar->priv->progressbar;
}

/* utility struct */
typedef struct {
	GtkStatusbar *statusbar;
	guint context_id;
	guint message_id;
} FlashInfo;

static const guint32 flash_length = 3000;

static gboolean
remove_message_timeout (FlashInfo * fi) 
{
	gtk_statusbar_remove (fi->statusbar, fi->context_id, fi->message_id);
	g_free (fi);

	/* remove the timeout */
  	return FALSE;
}

/**
 * gedit_statusbar_flash_message:
 * @statusbar: a #GeditStatusbar
 * @context_id: message context_id
 * @format: message to flash on the statusbar
 *
 * Flash a temporary message on the statusbar.
 */
void
gedit_statusbar_flash_message (GeditStatusbar *statusbar,
			       guint context_id,
			       gchar *format,
			       ...)
{
	va_list args;
	FlashInfo *fi;
	gchar *msg;

	g_return_if_fail (GEDIT_IS_STATUSBAR (statusbar));
	g_return_if_fail (format != NULL);

	va_start (args, format);
	msg = g_strdup_vprintf (format, args);
	va_end (args);

	fi = g_new (FlashInfo, 1);
	fi->statusbar = GTK_STATUSBAR (statusbar);
	fi->context_id = context_id;
	fi->message_id = gtk_statusbar_push (fi->statusbar, fi->context_id, msg);

	g_timeout_add (flash_length, (GtkFunction) remove_message_timeout, fi);

	g_free (msg);
}

