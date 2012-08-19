/*
 * gedit-floating-occurrence.h
 * This file is part of gedit
 *
 * Copyright (C) 2012 - Ignacio Casal Quinteiro
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

#ifndef __GEDIT_FLOATING_OCCURRENCE_H__
#define __GEDIT_FLOATING_OCCURRENCE_H__

#include <glib-object.h>
#include <gtk/gtk.h>
#include "theatrics/gedit-theatrics-choreographer.h"

G_BEGIN_DECLS

#define GEDIT_TYPE_FLOATING_OCCURRENCE			(gedit_floating_occurrence_get_type ())
#define GEDIT_FLOATING_OCCURRENCE(obj)			(G_TYPE_CHECK_INSTANCE_CAST ((obj), GEDIT_TYPE_FLOATING_OCCURRENCE, GeditFloatingOccurrence))
#define GEDIT_FLOATING_OCCURRENCE_CONST(obj)		(G_TYPE_CHECK_INSTANCE_CAST ((obj), GEDIT_TYPE_FLOATING_OCCURRENCE, GeditFloatingOccurrence const))
#define GEDIT_FLOATING_OCCURRENCE_CLASS(klass)		(G_TYPE_CHECK_CLASS_CAST ((klass), GEDIT_TYPE_FLOATING_OCCURRENCE, GeditFloatingOccurrenceClass))
#define GEDIT_IS_FLOATING_OCCURRENCE(obj)		(G_TYPE_CHECK_INSTANCE_TYPE ((obj), GEDIT_TYPE_FLOATING_OCCURRENCE))
#define GEDIT_IS_FLOATING_OCCURRENCE_CLASS(klass)	(G_TYPE_CHECK_CLASS_TYPE ((klass), GEDIT_TYPE_FLOATING_OCCURRENCE))
#define GEDIT_FLOATING_OCCURRENCE_GET_CLASS(obj)	(G_TYPE_INSTANCE_GET_CLASS ((obj), GEDIT_TYPE_FLOATING_OCCURRENCE, GeditFloatingOccurrenceClass))

typedef struct _GeditFloatingOccurrence			GeditFloatingOccurrence;
typedef struct _GeditFloatingOccurrenceClass		GeditFloatingOccurrenceClass;
typedef struct _GeditFloatingOccurrencePrivate		GeditFloatingOccurrencePrivate;
typedef struct _GeditFloatingOccurrenceClassPrivate	GeditFloatingOccurrenceClassPrivate;

struct _GeditFloatingOccurrence
{
	GtkBin parent;

	GeditFloatingOccurrencePrivate *priv;
};

struct _GeditFloatingOccurrenceClass
{
	GtkBinClass parent_class;

	GeditFloatingOccurrenceClassPrivate *priv;
};

GType            gedit_floating_occurrence_get_type     (void) G_GNUC_CONST;

GtkWidget       *gedit_floating_occurrence_new          (void);

void             gedit_floating_occurrence_set_text     (GeditFloatingOccurrence *occurrence,
                                                         const gchar             *text);

G_END_DECLS

#endif /* __GEDIT_FLOATING_OCCURRENCE_H__ */
