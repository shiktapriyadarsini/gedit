/*
 * gedit-highlight-mode-widget.h
 * This file is part of gedit
 *
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


#ifndef __GEDIT_HIGHLIGHT_MODE_WIDGET_H__
#define __GEDIT_HIGHLIGHT_MODE_WIDGET_H__

#include <glib-object.h>
#include <gtksourceview/gtksource.h>
#include "gedit-window.h"

G_BEGIN_DECLS

#define GEDIT_TYPE_HIGHLIGHT_MODE_WIDGET		(gedit_highlight_mode_widget_get_type ())
#define GEDIT_HIGHLIGHT_MODE_WIDGET(obj)		(G_TYPE_CHECK_INSTANCE_CAST ((obj), GEDIT_TYPE_HIGHLIGHT_MODE_WIDGET, GeditHighlightModeWidget))
#define GEDIT_HIGHLIGHT_MODE_WIDGET_CONST(obj)		(G_TYPE_CHECK_INSTANCE_CAST ((obj), GEDIT_TYPE_HIGHLIGHT_MODE_WIDGET, GeditHighlightModeWidget const))
#define GEDIT_HIGHLIGHT_MODE_WIDGET_CLASS(klass)	(G_TYPE_CHECK_CLASS_CAST ((klass), GEDIT_TYPE_HIGHLIGHT_MODE_WIDGET, GeditHighlightModeWidgetClass))
#define GEDIT_IS_HIGHLIGHT_MODE_WIDGET(obj)		(G_TYPE_CHECK_INSTANCE_TYPE ((obj), GEDIT_TYPE_HIGHLIGHT_MODE_WIDGET))
#define GEDIT_IS_HIGHLIGHT_MODE_WIDGET_CLASS(klass)	(G_TYPE_CHECK_CLASS_TYPE ((klass), GEDIT_TYPE_HIGHLIGHT_MODE_WIDGET))
#define GEDIT_HIGHLIGHT_MODE_WIDGET_GET_CLASS(obj)	(G_TYPE_INSTANCE_GET_CLASS ((obj), GEDIT_TYPE_HIGHLIGHT_MODE_WIDGET, GeditHighlightModeWidgetClass))

typedef struct _GeditHighlightModeWidget	GeditHighlightModeWidget;
typedef struct _GeditHighlightModeWidgetClass	GeditHighlightModeWidgetClass;
typedef struct _GeditHighlightModeWidgetPrivate	GeditHighlightModeWidgetPrivate;

struct _GeditHighlightModeWidget
{
	GtkGrid parent;

	GeditHighlightModeWidgetPrivate *priv;
};

struct _GeditHighlightModeWidgetClass
{
	GtkGridClass parent_class;

	void (* language_selected) (GeditHighlightModeWidget *widget,
	                            GtkSourceLanguage        *language);
};

GType                    gedit_highlight_mode_widget_get_type        (void) G_GNUC_CONST;

GtkWidget               *gedit_highlight_mode_widget_new             (void);

void                     gedit_highlight_mode_widget_select_language (GeditHighlightModeWidget *hmwidget,
                                                                      GtkSourceLanguage        *language);

G_END_DECLS

#endif /* __GEDIT_HIGHLIGHT_MODE_WIDGET_H__ */

/* ex:set ts=8 noet: */
