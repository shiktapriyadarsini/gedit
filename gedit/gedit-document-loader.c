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

#include <sys/mman.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <string.h>
#include <errno.h>

#include <glib/gi18n.h>
#include <libgnomevfs/gnome-vfs.h>

#include "gedit-document-loader.h"
#include "gedit-convert.h"
#include "gedit-metadata-manager.h"

#include "gedit-marshal.h"

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
	
	/* Handle for remote files */
	GnomeVFSAsyncHandle 	 *handle;
	
	/* Handle for local files */
	gint                      fd;
	
	GnomeVFSResult            last_result;
	GError			 *last_error;
	const GeditEncoding      *auto_detected_encoding;
};

G_DEFINE_TYPE(GeditDocumentLoader, gedit_document_loader, G_TYPE_OBJECT)

/* Signals */

enum {
	LOADING,
	LAST_SIGNAL
};

static guint signals[LAST_SIGNAL] = { 0 };

/* Properties */

enum
{
	PROP_0,
	PROP_DOCUMENT,
};

static void
gedit_document_loader_set_property (GObject      *object,
				    guint         prop_id,
				    const GValue *value,
				    GParamSpec   *pspec)
{
	GeditDocumentLoader *dl = GEDIT_DOCUMENT_LOADER (object);

	switch (prop_id)
	{
		case PROP_DOCUMENT:
			g_return_if_fail (dl->priv->document == NULL);
			dl->priv->document = g_value_get_object (value);
			break;
		default:
			G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
			break;
	}
}

static void
gedit_document_loader_get_property (GObject    *object,
				    guint       prop_id,
				    GValue     *value,
				    GParamSpec *pspec)
{
	GeditDocumentLoader *dl = GEDIT_DOCUMENT_LOADER (object);

	switch (prop_id)
	{
		case PROP_DOCUMENT:
			g_value_set_object (value,
					    dl->priv->document);
			break;
		default:
			G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
			break;		
	}
}

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
	object_class->get_property = gedit_document_loader_get_property;
	object_class->set_property = gedit_document_loader_set_property;

	g_object_class_install_property (object_class,
					 PROP_DOCUMENT,
					 g_param_spec_object ("document",
							 "Document",
							 "The GeditDocument this GeditDocumentLoader is associated with",
							 GEDIT_TYPE_DOCUMENT,
							 G_PARAM_READWRITE |
							 G_PARAM_CONSTRUCT_ONLY));

	signals[LOADING] =
   		g_signal_new ("loading",
			      G_OBJECT_CLASS_TYPE (object_class),
			      G_SIGNAL_RUN_LAST,
			      G_STRUCT_OFFSET (GeditDocumentLoaderClass, loading),
			      NULL, NULL,
			      gedit_marshal_VOID__BOOLEAN_POINTER,
			      G_TYPE_NONE,
			      2,
			      G_TYPE_BOOLEAN,
			      G_TYPE_POINTER);

	g_type_class_add_private (object_class, sizeof(GeditDocumentLoaderPrivate));
}

static void
gedit_document_loader_init (GeditDocumentLoader *loader)
{
	loader->priv = GEDIT_DOCUMENT_LOADER_GET_PRIVATE (loader);
	
	loader->priv->phase = GEDIT_DOCUMENT_LOADER_IDLE;
	
	loader->priv->fd = -1;
}

GeditDocumentLoader *
gedit_document_loader_new (GeditDocument *doc)
{
	GeditDocumentLoader *dl;
	
	g_return_val_if_fail (GEDIT_IS_DOCUMENT (doc), NULL);
	
	dl = GEDIT_DOCUMENT_LOADER (g_object_new (GEDIT_TYPE_DOCUMENT_LOADER, 
						  "document", doc,
						  NULL));
						  
	return dl;						  
}

static void
insert_text_in_document (GeditDocumentLoader *loader,
			 const gchar         *text,
			 gint                 len)
{
	gtk_source_buffer_begin_not_undoable_action (
				GTK_SOURCE_BUFFER (loader->priv->document));

	/* Insert text in the buffer */
	gtk_text_buffer_set_text (GTK_TEXT_BUFFER (loader->priv->document), 
				  text, 
				  len);

	gtk_text_buffer_set_modified (GTK_TEXT_BUFFER (loader->priv->document),
				      FALSE);
	
	gtk_source_buffer_end_not_undoable_action (
				GTK_SOURCE_BUFFER (loader->priv->document));
}

static gboolean
update_document_contents (GeditDocumentLoader  *loader, 
		          const gchar          *file_contents,
			  gint                  file_size,
			  GError              **error)
{
	g_return_val_if_fail (file_size > 0, FALSE);
	g_return_val_if_fail (file_contents != NULL, FALSE);

	if (loader->priv->encoding == gedit_encoding_get_utf8 ())
	{
		if (g_utf8_validate (file_contents, file_size, NULL))
		{
			insert_text_in_document (loader,
						 file_contents,
						 file_size);
			
			return TRUE;
		}
		else
		{
			g_set_error (error,
				     GEDIT_CONVERT_ERROR, 
				     GEDIT_CONVERT_ERROR_ILLEGAL_SEQUENCE,
				     _("The file you are trying to open contains an invalid byte sequence."));
				     
			return FALSE;
		}
	}
	else
	{
		GError *conv_error = NULL;
		gchar *converted_text = NULL;
		
		if (loader->priv->encoding == NULL)
		{
			/* Autodetecting the encoding: first try with the encoding
			stored in the metadata, if any */
			
			const GeditEncoding *enc;
			gchar *charset;

			charset = gedit_metadata_manager_get (loader->priv->uri, "encoding");

			if (charset != NULL)
			{
				enc = gedit_encoding_get_from_charset (charset);

				if (enc != NULL)
				{	
					converted_text = gedit_convert_to_utf8 (
									file_contents,
									file_size,
									&enc,
									NULL);

					if (converted_text != NULL)
						loader->priv->auto_detected_encoding = enc;
				}

				g_free (charset);
			}
		}
		
		if (converted_text == NULL)				
		{
			loader->priv->auto_detected_encoding = loader->priv->encoding;
			
			converted_text = gedit_convert_to_utf8 (
							file_contents,
							file_size,
							&loader->priv->auto_detected_encoding,
							&conv_error);
		}
	
		if (converted_text == NULL)
		{
			/* bail out */
			if (conv_error == NULL)
				g_set_error (error,
					     GEDIT_CONVERT_ERROR, 
					     GEDIT_CONVERT_ERROR_ILLEGAL_SEQUENCE,
					     _("The file you are trying to open contains an invalid byte sequence."));
			else
				g_propagate_error (error, conv_error);
	
			return FALSE;
		}
		else
		{
			insert_text_in_document (loader,
						 converted_text,
						 -1);

			return TRUE;
		}
	}

	g_return_val_if_reached (FALSE);
}

/* The following function has been copied from gnome-vfs 
   (gnome-vfs-module-shared.c) file */
static void
stat_to_file_info (GnomeVFSFileInfo *file_info,
		   const struct stat *statptr)
{
	if (S_ISDIR (statptr->st_mode))
		file_info->type = GNOME_VFS_FILE_TYPE_DIRECTORY;
	else if (S_ISCHR (statptr->st_mode))
		file_info->type = GNOME_VFS_FILE_TYPE_CHARACTER_DEVICE;
	else if (S_ISBLK (statptr->st_mode))
		file_info->type = GNOME_VFS_FILE_TYPE_BLOCK_DEVICE;
	else if (S_ISFIFO (statptr->st_mode))
		file_info->type = GNOME_VFS_FILE_TYPE_FIFO;
	else if (S_ISSOCK (statptr->st_mode))
		file_info->type = GNOME_VFS_FILE_TYPE_SOCKET;
	else if (S_ISREG (statptr->st_mode))
		file_info->type = GNOME_VFS_FILE_TYPE_REGULAR;
	else if (S_ISLNK (statptr->st_mode))
		file_info->type = GNOME_VFS_FILE_TYPE_SYMBOLIC_LINK;
	else
		file_info->type = GNOME_VFS_FILE_TYPE_UNKNOWN;

	file_info->permissions
		= statptr->st_mode & (GNOME_VFS_PERM_USER_ALL
				      | GNOME_VFS_PERM_GROUP_ALL
				      | GNOME_VFS_PERM_OTHER_ALL
				      | GNOME_VFS_PERM_SUID
				      | GNOME_VFS_PERM_SGID
				      | GNOME_VFS_PERM_STICKY);

	file_info->device = statptr->st_dev;
	file_info->inode = statptr->st_ino;

	file_info->link_count = statptr->st_nlink;

	file_info->uid = statptr->st_uid;
	file_info->gid = statptr->st_gid;

	file_info->size = statptr->st_size;
	file_info->block_count = statptr->st_blocks;
	file_info->io_block_size = statptr->st_blksize;
	if (file_info->io_block_size > 0 &&
	    file_info->io_block_size < 4096) {
		/* Never use smaller block than 4k,
		   should probably be pagesize.. */
		file_info->io_block_size = 4096;
	}

	file_info->atime = statptr->st_atime;
	file_info->ctime = statptr->st_ctime;
	file_info->mtime = statptr->st_mtime;

	file_info->valid_fields |= GNOME_VFS_FILE_INFO_FIELDS_TYPE |
	  GNOME_VFS_FILE_INFO_FIELDS_PERMISSIONS | GNOME_VFS_FILE_INFO_FIELDS_FLAGS |
	  GNOME_VFS_FILE_INFO_FIELDS_DEVICE | GNOME_VFS_FILE_INFO_FIELDS_INODE |
	  GNOME_VFS_FILE_INFO_FIELDS_LINK_COUNT | GNOME_VFS_FILE_INFO_FIELDS_SIZE |
	  GNOME_VFS_FILE_INFO_FIELDS_BLOCK_COUNT | GNOME_VFS_FILE_INFO_FIELDS_IO_BLOCK_SIZE |
	  GNOME_VFS_FILE_INFO_FIELDS_ATIME | GNOME_VFS_FILE_INFO_FIELDS_MTIME |
	  GNOME_VFS_FILE_INFO_FIELDS_CTIME;
}

/* FIXME: this is ugly for multple reasons: it refetches all the info,
 * it doesn't use fd etc... we need something better, possibly in gnome-vfs
 * public api.
 */
static gchar *
get_slow_mime_type (const char *text_uri)
{
	GnomeVFSFileInfo *info;
	char *mime_type;
	GnomeVFSResult result;

	info = gnome_vfs_file_info_new ();
	result = gnome_vfs_get_file_info (text_uri, info,
					  GNOME_VFS_FILE_INFO_GET_MIME_TYPE |
					  GNOME_VFS_FILE_INFO_FORCE_SLOW_MIME_TYPE |
					  GNOME_VFS_FILE_INFO_FOLLOW_LINKS);
	if (info->mime_type == NULL || result != GNOME_VFS_OK) {
		mime_type = NULL;
	} else {
		mime_type = g_strdup (info->mime_type);
	}
	gnome_vfs_file_info_unref (info);

	return mime_type;
}

static gboolean
load_local_file_real (GeditDocumentLoader *loader)
{
	struct stat statbuf;
	GError *error = NULL;
	gint ret;

	g_print ("load_local_file_real\n");
	if (loader->priv->fd == -1)
	{
		/* loader->priv->last_result was already be set */
		loader->priv->phase = GEDIT_DOCUMENT_LOADER_PHASE_OPEN;

		error = g_error_new (GEDIT_DOCUMENT_ERROR,
				     loader->priv->last_result,
				     gnome_vfs_result_to_string (loader->priv->last_result));
		goto done;
	}

	if (fstat (loader->priv->fd, &statbuf) != 0) 
	{
		loader->priv->last_result = gnome_vfs_result_from_errno ();
		loader->priv->phase = GEDIT_DOCUMENT_LOADER_PHASE_GETTING_INFO;

		error = g_error_new (GEDIT_DOCUMENT_ERROR,
				     loader->priv->last_result,
				     gnome_vfs_result_to_string (loader->priv->last_result));
		goto done;
	}
	
	loader->priv->info = gnome_vfs_file_info_new ();
	stat_to_file_info (loader->priv->info, &statbuf);
	GNOME_VFS_FILE_INFO_SET_LOCAL (loader->priv->info, TRUE);

	if (loader->priv->info->size == 0)
	{
		if (loader->priv->encoding == NULL)
			loader->priv->auto_detected_encoding = gedit_encoding_get_current (); 
	}
	else
	{
		gchar *mapped_file;
		
		/* CHECK: should we lock the file */
		g_print ("load_local_file_real: before mmap\n");
		
		mapped_file = mmap (0, /* start */
				    loader->priv->info->size, 
				    PROT_READ,
				    MAP_PRIVATE, /* flags */
				    loader->priv->fd,
				    0 /* offset */);
				    
		if (mapped_file == MAP_FAILED)
		{
			loader->priv->last_result = gnome_vfs_result_from_errno ();
			loader->priv->phase = GEDIT_DOCUMENT_LOADER_PHASE_READING;

			error = g_error_new (GEDIT_DOCUMENT_ERROR,
					     loader->priv->last_result,
					     gnome_vfs_result_to_string (loader->priv->last_result));
			goto done;
		}
		
		loader->priv->bytes_read = loader->priv->info->size;
		
		if (!update_document_contents (loader,
					       mapped_file,
					       loader->priv->info->size,
					       &error))
		{
			loader->priv->phase = GEDIT_DOCUMENT_LOADER_PHASE_CONVERTING;
			loader->priv->last_result = GNOME_VFS_OK; /* Reset */
			g_return_val_if_fail (loader->priv->last_error != NULL,
					      FALSE);

			ret = munmap (mapped_file, loader->priv->info->size);
			if (ret != 0)
				g_warning ("File '%s' has not been correctly unmapped: %s",
					   loader->priv->uri,
					   strerror (errno));

			goto done;
		}

		ret = munmap (mapped_file, loader->priv->info->size);
			
		if (ret != 0)
			g_warning ("File '%s' has not been correctly unmapped: %s",
				   loader->priv->uri,
				   strerror (errno));
	}
	
	ret = close (loader->priv->fd);

	/* mime type hack */
	loader->priv->info->mime_type = get_slow_mime_type (loader->priv->uri);
	loader->priv->info->valid_fields |= GNOME_VFS_FILE_INFO_FIELDS_MIME_TYPE;

	if (ret != 0)
		g_warning ("File '%s' has not been correctly closed: %s",
			   loader->priv->uri,
			   strerror (errno));
		
	loader->priv->fd = -1;
	
	g_return_val_if_fail (loader->priv->last_error == NULL, FALSE);

	loader->priv->phase = GEDIT_DOCUMENT_LOADER_PHASE_COMPLETED;
	loader->priv->last_result = GNOME_VFS_OK;

 done:
	/* the object will be unrefed in the callback of the loading
	 * signal, so we need to prevent finalization.
	 */
	g_object_ref (loader);

	g_signal_emit (loader, 
		       signals[LOADING],
		       0,
		       TRUE, /* completed */
		       error);

	/* each loader can be used only once: put the loader in 
	 * phase END so that we can check if a loader has already
	 * been used in case it is still alive after the unref
	 */
	loader->priv->phase = GEDIT_DOCUMENT_LOADER_PHASE_END;

	if (error)
		g_error_free (error);

	g_object_unref (loader);

	return FALSE;
}

static void
load_local_file (GeditDocumentLoader *loader,
		 const gchar         *fname)
{
	loader->priv->phase = GEDIT_DOCUMENT_LOADER_PHASE_READY_TO_GO;

	g_signal_emit (loader,
		       signals[LOADING],
		       0,
		       FALSE,
		       NULL);

	loader->priv->fd = open (fname, O_RDONLY);
	if (loader->priv->fd == -1)
	{
		g_print ("Error opening file");
		/* The error signal will be emitted later */
		loader->priv->last_result = gnome_vfs_result_from_errno ();
	}

	g_timeout_add_full (G_PRIORITY_HIGH,
			    0,
			    (GSourceFunc)load_local_file_real,
			    loader,
			    NULL);		    
}

/* If enconding == NULL, the encoding will be autodetected */
gboolean
gedit_document_loader_load (GeditDocumentLoader *loader,
			    const gchar         *uri,
			    const GeditEncoding *encoding)
{
	gchar *local_path;
	g_return_val_if_fail (GEDIT_IS_DOCUMENT_LOADER (loader), FALSE);
	g_return_val_if_fail (loader->priv->phase == GEDIT_DOCUMENT_LOADER_IDLE, FALSE);
	g_return_val_if_fail (uri != NULL, FALSE);

	loader->priv->encoding = encoding;

	loader->priv->uri = g_strdup (uri);

	// TODO: returns FALSE if uri is not valid?

	local_path = gnome_vfs_get_local_path_from_uri (uri);
	if (local_path != NULL)
		load_local_file (loader, local_path);
	else
		// TODO
		g_return_val_if_reached (FALSE);
	
	g_free (local_path);

	return TRUE;
}

GeditDocumentLoaderPhase 
gedit_document_loader_get_phase (GeditDocumentLoader  *loader)
{
	g_return_val_if_fail (GEDIT_IS_DOCUMENT_LOADER (loader), 
			      GEDIT_DOCUMENT_LOADER_IDLE);

	return loader->priv->phase;
}

/* Returns STDIN_URI if loading from stdin */
const gchar *
gedit_document_loader_get_uri (GeditDocumentLoader  *loader)
{
	g_return_val_if_fail (GEDIT_IS_DOCUMENT_LOADER (loader), NULL);

	return loader->priv->uri;
}

/* it may return NULL, it's up to gedit-document handle it */
const gchar *
gedit_document_loader_get_mime_type (GeditDocumentLoader  *loader)
{
	g_return_val_if_fail (GEDIT_IS_DOCUMENT_LOADER (loader), NULL);

	if (loader->priv->info &&
	    (loader->priv->info->valid_fields & GNOME_VFS_FILE_INFO_FIELDS_MIME_TYPE))
		return loader->priv->info->mime_type;
	else
		return NULL;
}

/* Returns 0 if file size is unknown */
GnomeVFSFileSize 
gedit_document_loader_get_file_size (GeditDocumentLoader  *loader)
{
	g_return_val_if_fail (GEDIT_IS_DOCUMENT_LOADER (loader), 0);
	
	if (loader->priv->info == NULL)
		return 0;
		
	return loader->priv->info->size;
}									 

GnomeVFSFileSize
gedit_document_loader_get_bytes_read (GeditDocumentLoader  *loader)
{
	g_return_val_if_fail (GEDIT_IS_DOCUMENT_LOADER (loader), 0);
	
	return loader->priv->bytes_read;
}

