/*
 * gedit-window.h
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
 * MERCHANWINDOWILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
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

#ifndef __GEDIT_WINDOW_H__
#define __GEDIT_WINDOW_H__

#include <gtk/gtk.h>

#include <gedit/gedit-tab.h>

G_BEGIN_DECLS

/*
 * Type checking and casting macros
 */
#define GEDIT_TYPE_WINDOW              (gedit_window_get_type())
#define GEDIT_WINDOW(obj)              (G_TYPE_CHECK_INSTANCE_CAST((obj), GEDIT_TYPE_WINDOW, GeditWindow))
#define GEDIT_WINDOW_CONST(obj)        (G_TYPE_CHECK_INSTANCE_CAST((obj), GEDIT_TYPE_WINDOW, GeditWindow const))
#define GEDIT_WINDOW_CLASS(klass)      (G_TYPE_CHECK_CLASS_CAST((klass), GEDIT_TYPE_WINDOW, GeditWindowClass))
#define GEDIT_IS_WINDOW(obj)           (G_TYPE_CHECK_INSTANCE_TYPE((obj), GEDIT_TYPE_WINDOW))
#define GEDIT_IS_WINDOW_CLASS(klass)   (G_TYPE_CHECK_CLASS_TYPE ((klass), GEDIT_TYPE_WINDOW))
#define GEDIT_WINDOW_GET_CLASS(obj)    (G_TYPE_INSTANCE_GET_CLASS((obj), GEDIT_TYPE_WINDOW, GeditWindowClass))

/* Private structure type */
typedef struct _GeditWindowPrivate GeditWindowPrivate;

/*
 * Main object structure
 */
typedef struct _GeditWindow GeditWindow;

struct _GeditWindow 
{
	GtkWindow window;

	/*< private > */
	GeditWindowPrivate *priv;
};

/*
 * Class definition
 */
typedef struct _GeditWindowClass GeditWindowClass;

struct _GeditWindowClass 
{
	GtkWindowClass parent_class;
};

/*
 * Public methods
 */
GType 		 gedit_window_get_type 			(void) G_GNUC_CONST;

GeditTab	*gedit_window_create_tab		(GeditWindow *window,
							 gboolean     jump_to);

void		 gedit_window_close_tab			(GeditWindow *window,
							 GeditTab    *tab);
							 
GeditTab	*gedit_window_get_active_tab		(GeditWindow *window);

/* Helper functions */
GeditView	*gedit_window_get_active_view		(GeditWindow *window);
GeditDocument	*gedit_window_get_active_document	(GeditWindow *window);

/* Returns a newly allocated list with all the documents in the window */
GList		*gedit_window_get_documents		(GeditWindow *window);

/* Returns a newly allocated list with all the views in the window */
GList		*gedit_window_get_views			(GeditWindow *window);

/*
 * Non exported functions
 */
GtkWidget	*_gedit_window_get_notebook		(GeditWindow *window);

void		 _gedit_window_set_statusbar_visible	(GeditWindow *window,
							 gboolean     visible);
void		 _gedit_window_set_toolbar_visible	(GeditWindow *window,
							 gboolean     visible);							 
G_END_DECLS

#endif  /* __GEDIT_WINDOW_H__  */
