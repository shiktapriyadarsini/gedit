/*
 * gedit-document-loader.c
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

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <glib/gi18n.h>

#include "gedit-document-loader.h"

#define GEDIT_DOCUMENT_LOADER_GET_PRIVATE(object)(G_TYPE_INSTANCE_GET_PRIVATE ((object), GEDIT_TYPE_DOCUMENT_LOADER, GeditDocumentLoaderPrivate))

struct _GeditDocumentLoaderPrivate
{
	GeditDocument		 *document;
	GeditDocumentLoaderPhase  phase;
	
	/* Info on the current file */
	gchar			 *uri;
	const GeditEncoding      *encoding;
	GnomeVFSFileInfo	 *info;
	GnomeVFSFileSize	  bytes_read;
	
	GnomeVFSAsyncHandle 	 *handle;
}

G_DEFINE_TYPE(GeditDocumentLoader, gedit_document_loader, G_TYPE_OBJECT)

static void
gedit_document_loader_finalize (GObject *object)
{
	G_OBJECT_CLASS (gedit_document_loader_parent_class)->finalize (object);
}

static void 
gedit_document_loader_class_init (GeditDocumentLoaderClass *klass)
{
	GObjectClass *object_class = G_OBJECT_CLASS (klass);

	object_class->finalize = gedit_document_loader_finalize;
							      
	g_type_class_add_private (object_class, sizeof(GeditDocumentLoaderPrivate));
}

static void
gedit_document_loader_init (GeditDocumentLoader *loader)
{
	loader->priv = EDIT_DOCUMENT_LOADER_GET_PRIVATE (loader);
	
	loader->priv->phase = GEDIT_DOCUMENT_LOADER_IDLE;	
}
