/*
 * gedit-floating-occurrence.c
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

#include "gedit-floating-occurrence.h"
#include "gedit-animatable.h"

struct _GeditFloatingOccurrencePrivate
{
	GtkWidget *label;
	GeditTheatricsChoreographerEasing easing;
	GeditTheatricsChoreographerBlocking blocking;
	GeditTheatricsAnimationState animation_state;
	guint duration;
	gdouble bias;
	gdouble percent;
};

struct _GeditFloatingOccurrenceClassPrivate
{
	GtkCssProvider *css;
};

enum
{
	PROP_0,
	PROP_EASING,
	PROP_BLOCKING,
	PROP_ANIMATION_STATE,
	PROP_DURATION,
	PROP_PERCENT,
	PROP_BIAS
};

G_DEFINE_TYPE_WITH_CODE (GeditFloatingOccurrence, gedit_floating_occurrence, GTK_TYPE_BIN,
                         g_type_add_class_private (g_define_type_id, sizeof (GeditFloatingOccurrenceClassPrivate));
                         G_IMPLEMENT_INTERFACE (GEDIT_TYPE_ANIMATABLE,
                                                NULL))

static void
gedit_floating_occurrence_finalize (GObject *object)
{
	G_OBJECT_CLASS (gedit_floating_occurrence_parent_class)->finalize (object);
}

static void
gedit_floating_occurrence_get_property (GObject    *object,
                                        guint       prop_id,
                                        GValue     *value,
                                        GParamSpec *pspec)
{
	GeditFloatingOccurrencePrivate *priv = GEDIT_FLOATING_OCCURRENCE (object)->priv;

	switch (prop_id)
	{
		case PROP_EASING:
			g_value_set_enum (value, priv->easing);
			break;
		case PROP_BLOCKING:
			g_value_set_enum (value, priv->blocking);
			break;
		case PROP_ANIMATION_STATE:
			g_value_set_enum (value, priv->animation_state);
			break;
		case PROP_DURATION:
			g_value_set_uint (value, priv->duration);
			break;
		case PROP_PERCENT:
			g_value_set_double (value, priv->percent);
			break;
		case PROP_BIAS:
			g_value_set_double (value, priv->bias);
			break;
		default:
			G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
			break;
	}
}

static void
gedit_floating_occurrence_set_property (GObject      *object,
                                        guint         prop_id,
                                        const GValue *value,
                                        GParamSpec   *pspec)
{
	GeditFloatingOccurrencePrivate *priv = GEDIT_FLOATING_OCCURRENCE (object)->priv;

	switch (prop_id)
	{
		case PROP_EASING:
			priv->easing = g_value_get_enum (value);
			break;
		case PROP_BLOCKING:
			priv->blocking = g_value_get_enum (value);
			break;
		case PROP_ANIMATION_STATE:
			priv->animation_state = g_value_get_enum (value);
			break;
		case PROP_DURATION:
			priv->duration = g_value_get_uint (value);
			break;
		case PROP_PERCENT:
			priv->percent = g_value_get_double (value);

			if (priv->percent >= 0)
			{
				gtk_container_set_border_width (GTK_CONTAINER (object), (gint)(100 * priv->percent));
			}

			gtk_widget_queue_resize_no_redraw (GTK_WIDGET (object));
			break;
		case PROP_BIAS:
			priv->bias = g_value_get_double (value);
			break;
		default:
			G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
			break;
	}
}

static void
gedit_floating_occurrence_get_preferred_width (GtkWidget *widget,
                                               gint      *minimum,
                                               gint      *natural)
{
	GtkWidget *child;
	gint width = gtk_container_get_border_width (GTK_CONTAINER (widget)) * 2;

	child = gtk_bin_get_child (GTK_BIN (widget));

	if (child != NULL)
	{
		gint child_min, child_nat;

		gtk_widget_get_preferred_width (child, &child_min, &child_nat);

		width += child_nat;
	}

	*minimum = *natural = width;
}

static void
gedit_floating_occurrence_get_preferred_height (GtkWidget *widget,
                                                gint      *minimum,
                                                gint      *natural)
{
	GtkWidget *child;
	gint height = gtk_container_get_border_width (GTK_CONTAINER (widget)) * 2;

	child = gtk_bin_get_child (GTK_BIN (widget));

	if (child != NULL)
	{
		gint child_min, child_nat;

		gtk_widget_get_preferred_height (child, &child_min, &child_nat);

		height += child_nat;
	}

	*minimum = *natural = height;
}

static void
gedit_floating_occurrence_size_allocate (GtkWidget     *widget,
                                         GtkAllocation *allocation)
{
	GtkWidget *child;

	gtk_widget_set_allocation (widget, allocation);

	child = gtk_bin_get_child (GTK_BIN (widget));

	if (child != NULL && gtk_widget_get_visible (child))
	{
		GtkAllocation child_alloc;
		gint child_min, child_nat;
		gint border;

		border = gtk_container_get_border_width (GTK_CONTAINER (widget));

		gtk_widget_get_preferred_width (child, &child_min, &child_nat);
		child_alloc.width = child_nat;
		gtk_widget_get_preferred_height (child, &child_min, &child_nat);
		child_alloc.height = child_nat;
		child_alloc.x = allocation->x + border;
		child_alloc.y = allocation->y + border;

		if (child_alloc.height > 0 && child_alloc.width > 0)
		{
			gtk_widget_size_allocate (child, &child_alloc);
		}
	}
}

static gboolean
gedit_floating_occurrence_draw (GtkWidget *widget,
                                cairo_t   *cr)
{
	GtkStyleContext *context;

	context = gtk_widget_get_style_context (widget);

	gtk_style_context_save (context);

	gtk_render_background (context, cr, 0, 0,
	                       gtk_widget_get_allocated_width (widget),
	                       gtk_widget_get_allocated_height (widget));

	gtk_render_frame (context, cr, 0, 0,
	                  gtk_widget_get_allocated_width (widget),
	                  gtk_widget_get_allocated_height (widget));

	gtk_style_context_restore (context);

	return GTK_WIDGET_CLASS (gedit_floating_occurrence_parent_class)->draw (widget, cr);
}

static void
gedit_floating_occurrence_class_init (GeditFloatingOccurrenceClass *klass)
{
	GObjectClass *object_class = G_OBJECT_CLASS (klass);
	GtkWidgetClass *widget_class = GTK_WIDGET_CLASS (klass);
	static const gchar style[] =
	"* {"
	  "background-color: @info_bg_color;\n"

	  "border-color: shade (@info_bg_color, 0.80);\n"

	  "border-radius: 3px 3px 3px 3px;\n"
	  "border-width: 1px 1px 1px 1px;\n"
	  "border-style: solid;\n"
	"}";

	object_class->finalize = gedit_floating_occurrence_finalize;
	object_class->get_property = gedit_floating_occurrence_get_property;
	object_class->set_property = gedit_floating_occurrence_set_property;

	widget_class->get_preferred_width = gedit_floating_occurrence_get_preferred_width;
	widget_class->get_preferred_height = gedit_floating_occurrence_get_preferred_height;
	widget_class->size_allocate = gedit_floating_occurrence_size_allocate;
	widget_class->draw = gedit_floating_occurrence_draw;

	g_object_class_override_property (object_class, PROP_EASING, "easing");

	g_object_class_override_property (object_class, PROP_BLOCKING, "blocking");

	g_object_class_override_property (object_class, PROP_ANIMATION_STATE,
	                                  "animation-state");

	g_object_class_override_property (object_class, PROP_DURATION, "duration");

	g_object_class_override_property (object_class, PROP_PERCENT, "percent");

	g_object_class_override_property (object_class, PROP_BIAS, "bias");

	g_type_class_add_private (object_class, sizeof (GeditFloatingOccurrencePrivate));

	klass->priv = G_TYPE_CLASS_GET_PRIVATE (klass, GEDIT_TYPE_FLOATING_OCCURRENCE,
	                                        GeditFloatingOccurrenceClassPrivate);

	klass->priv->css = gtk_css_provider_new ();
	gtk_css_provider_load_from_data (klass->priv->css, style, -1, NULL);
}

static void
gedit_floating_occurrence_init (GeditFloatingOccurrence *occurrence)
{
	GtkStyleContext *context;

	occurrence->priv = G_TYPE_INSTANCE_GET_PRIVATE (occurrence,
	                                                GEDIT_TYPE_FLOATING_OCCURRENCE,
	                                                GeditFloatingOccurrencePrivate);

	context = gtk_widget_get_style_context (GTK_WIDGET (occurrence));
	gtk_style_context_add_provider (context,
	                                GTK_STYLE_PROVIDER (GEDIT_FLOATING_OCCURRENCE_GET_CLASS (occurrence)->priv->css),
	                                GTK_STYLE_PROVIDER_PRIORITY_APPLICATION);

	occurrence->priv->label = gtk_label_new ("Hello");
	gtk_widget_show (occurrence->priv->label);
	gtk_container_add (GTK_CONTAINER (occurrence), occurrence->priv->label);
}

GtkWidget *
gedit_floating_occurrence_new (void)
{
	return g_object_new (GEDIT_TYPE_FLOATING_OCCURRENCE, NULL);
}

/* ex:set ts=8 noet: */
