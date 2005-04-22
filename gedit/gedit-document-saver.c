/*
 * gedit-document-saver.c
 * This file is part of gedit
 *
 * Copyright (C) 2005 - Paolo Borelli and Paolo Maggi
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
#include <glib/gfileutils.h>
#include <libgnomevfs/gnome-vfs.h>

#include "gedit-document-saver.h"
#include "gedit-convert.h"
#include "gedit-metadata-manager.h"
#include "gedit-marshal.h"
#include "gedit-utils.h"

#define GEDIT_DOCUMENT_SAVER_GET_PRIVATE(object)(G_TYPE_INSTANCE_GET_PRIVATE ((object), GEDIT_TYPE_DOCUMENT_SAVER, GeditDocumentSaverPrivate))

struct _GeditDocumentSaverPrivate
{
	GeditDocument		 *document;

	gchar			 *uri;
	const GeditEncoding      *encoding;
	gboolean		  keep_backup;
	gchar			 *backup_ext;
	gboolean                  backups_in_curr_dir;

	gint			 fd;

	/* temp data for local files */
	gchar			 *local_path;

	GError                   *error;
};

G_DEFINE_TYPE(GeditDocumentSaver, gedit_document_saver, G_TYPE_OBJECT)

/* Signals */

enum {
	SAVING,
	LAST_SIGNAL
};

static guint signals[LAST_SIGNAL] = { 0 };

static void
gedit_document_saver_finalize (GObject *object)
{
	GeditDocumentSaverPrivate *priv = GEDIT_DOCUMENT_SAVER (object)->priv;

	g_free (priv->uri);
	g_free (priv->local_path);

	if (priv->error)
		g_error_free (priv->error);

	G_OBJECT_CLASS (gedit_document_saver_parent_class)->finalize (object);
}

static void 
gedit_document_saver_class_init (GeditDocumentSaverClass *klass)
{
	GObjectClass *object_class = G_OBJECT_CLASS (klass);

	object_class->finalize = gedit_document_saver_finalize;

	signals[SAVING] =
   		g_signal_new ("saving",
			      G_OBJECT_CLASS_TYPE (object_class),
			      G_SIGNAL_RUN_LAST,
			      G_STRUCT_OFFSET (GeditDocumentSaverClass, saving),
			      NULL, NULL,
			      gedit_marshal_VOID__BOOLEAN_POINTER,
			      G_TYPE_NONE,
			      2,
			      G_TYPE_BOOLEAN,
			      G_TYPE_POINTER);

	g_type_class_add_private (object_class, sizeof(GeditDocumentSaverPrivate));
}

static void
gedit_document_saver_init (GeditDocumentSaver *saver)
{
	saver->priv = GEDIT_DOCUMENT_SAVER_GET_PRIVATE (saver);

	saver->priv->fd = -1;

	saver->priv->error = NULL;
}

GeditDocumentSaver *
gedit_document_saver_new (GeditDocument *doc)
{
	GeditDocumentSaver *saver;

	g_return_val_if_fail (GEDIT_IS_DOCUMENT (doc), NULL);

	saver = GEDIT_DOCUMENT_SAVER (g_object_new (GEDIT_TYPE_DOCUMENT_SAVER, NULL));

	saver->priv->document = doc;

	return saver;
}

gboolean
gedit_document_saver_reset (GeditDocumentSaver *saver)
{
	// TODO: needs checking that the saver is not active... or maybe redesign stuff

	g_return_val_if_fail (GEDIT_IS_DOCUMENT_SAVER (saver), FALSE);

	saver->priv->fd = -1;

	g_free (saver->priv->uri);
	saver->priv->uri = NULL;

	g_free (saver->priv->local_path);
	saver->priv->local_path = NULL;

	if (saver->priv->error)
		g_error_free (saver->priv->error);
	saver->priv->error = NULL;

	return TRUE;
}

/*
 * Write the document contents in fd.
 */
static gboolean
write_document_contents (gint                  fd,
			 GtkTextBuffer        *doc,
			 const GeditEncoding  *encoding,
			 GError              **error)
{
	GtkTextIter start_iter;
	GtkTextIter end_iter;
	gchar *contents;
	gsize len;
	gboolean add_cr;
	ssize_t written;
	gboolean res = FALSE;

	gtk_text_buffer_get_bounds (doc, &start_iter, &end_iter);
	contents = gtk_text_buffer_get_slice (doc, &start_iter, &end_iter, TRUE);

	len = strlen (contents);

	if (len >= 1)
		add_cr = (*(contents + len - 1) != '\n');
	else
		add_cr = FALSE;

	if (encoding != gedit_encoding_get_utf8 ())
	{
		gchar *converted_contents;
		gsize new_len;

		converted_contents = gedit_convert_from_utf8 (contents, 
							      len, 
							      encoding,
							      &new_len,
							      error);

		if (error != NULL)
		{
			/* Conversion error */
			goto out;
		}
		else
		{
			g_free (contents);
			contents = converted_contents;
			len = new_len;
		}
	}

	/* make sure we are at the start */
	res = (lseek (fd, 0, SEEK_SET) != -1);

	/* Truncate the file to 0, in case it was not empty */
	if (res)
		res = (ftruncate (fd, 0) == 0);

	/* Save the file content */
	if (res)
	{
		written = write (fd, contents, len);
		res = ((written != -1) && ((gsize) written == len));
	}

	/* Add \n at the end if needed */
	if (res && add_cr)
	{
		if (encoding != gedit_encoding_get_utf8 ())
		{
			gchar *converted_n = NULL;
			gsize n_len;

			converted_n = gedit_convert_from_utf8 ("\n", 
							       -1, 
							       encoding,
							       &n_len,
							       NULL);

			if (converted_n == NULL)
			{
				/* we do not error out for this */
				g_warning ("Cannot add '\\n' at the end of the file.");
			}
			else
			{
				written = write (fd, converted_n, n_len);
				res = ((written != -1) && ((gsize) written == n_len));
				g_free (converted_n);
			}
		}
		else
		{
			res = (write (fd, "\n", 1) == 1);
		}
	}

 out:
	g_free (contents);

	if (!res)
	{
		GnomeVFSResult result = gnome_vfs_result_from_errno ();

		g_set_error (error,
			     GEDIT_DOCUMENT_ERROR,
			     result,
			     gnome_vfs_result_to_string (result));
	}

	return res;
}

static void
save_completed_or_failed (GeditDocumentSaver *saver)
{
	g_signal_emit (saver, 
		       signals[SAVING],
		       0,
		       TRUE, /* completed */
		       saver->priv->error);
}

static gchar *
get_backup_filename (GeditDocumentSaver *saver)
{
	gchar *fname;
	gchar *bak_ext = NULL;

	if ((saver->priv->backup_ext != NULL) &&
	    (strlen (saver->priv->backup_ext) > 0))
		bak_ext = saver->priv->backup_ext;
	else
		bak_ext = "~";

	fname = g_strconcat (saver->priv->local_path, bak_ext, NULL);

	/* If we are not going to keep the backup file and fname
	 * already exists, try to use another name.
	 * Change one character, just before the extension.
	 */
	if (!saver->priv->keep_backup &&
	    g_file_test (fname, G_FILE_TEST_EXISTS))
	{
		gchar *wp;

		wp = fname + strlen (fname) - 1 - strlen (bak_ext);
		g_return_val_if_fail (wp > fname, NULL);

		*wp = 'z';
		while ((*wp > 'a') && g_file_test (fname, G_FILE_TEST_EXISTS))
			--*wp;

		/* They all exist??? Must be something wrong. */
		if (*wp == 'a')
		{
			g_free (fname);
			fname = NULL;
		}
	}

	return fname;
}

/* like unlink, but doesn't fail if the file wasn't there at all */
static gboolean
remove_file (const gchar *name)
{
	gint res;

	res = unlink (name);

	return (res == 0) || ((res == -1) && (errno == ENOENT));
}

#define BUFSIZE	8192 /* size of normal write buffer */

static gboolean
copy_file_data (gint     sfd,
		gint     dfd,
		GError **error)
{
	gboolean ret = TRUE;
	gpointer buffer;
	const gchar *write_buffer;
	ssize_t bytes_read;
	ssize_t bytes_to_write;
	ssize_t bytes_written;

	buffer = g_malloc (BUFSIZE);

	do
	{
		bytes_read = read (sfd, buffer, BUFSIZE);
		if (bytes_read == -1)
		{
			GnomeVFSResult result = gnome_vfs_result_from_errno ();

			g_set_error (error,
				     GEDIT_DOCUMENT_ERROR,
				     result,
				     gnome_vfs_result_to_string (result));

			ret = FALSE;

			break;
		}

		bytes_to_write = bytes_read;
		write_buffer = buffer;

		do
		{
			bytes_written = write (dfd, write_buffer, bytes_to_write);
			if (bytes_written == -1)
			{
				GnomeVFSResult result = gnome_vfs_result_from_errno ();

				g_set_error (error,
					     GEDIT_DOCUMENT_ERROR,
					     result,
					     gnome_vfs_result_to_string (result));

				ret = FALSE;

				break;
			}

			bytes_to_write -= bytes_written;
			write_buffer += bytes_written;
		}
		while (bytes_to_write > 0);

	} while ((bytes_read != 0) && (ret == TRUE));

	return ret;
}

static gboolean
save_existing_local_file (GeditDocumentSaver *saver)
{
	struct stat statbuf;
	gchar *backup_filename = NULL;
	gint bfd;

	if (fstat (saver->priv->fd, &statbuf) != 0) 
	{
		GnomeVFSResult result = gnome_vfs_result_from_errno ();

		g_set_error (&saver->priv->error,
			     GEDIT_DOCUMENT_ERROR,
			     result,
			     gnome_vfs_result_to_string (result));

		goto out;
	}

	/* not a regular file */
	if (!S_ISREG (statbuf.st_mode))
	{
		GnomeVFSResult result = S_ISDIR (statbuf.st_mode) ?
					GNOME_VFS_ERROR_IS_DIRECTORY :
					GNOME_VFS_ERROR_GENERIC; // choose better err?

		g_set_error (&saver->priv->error,
			     GEDIT_DOCUMENT_ERROR,
			     result,
			     gnome_vfs_result_to_string (result));

		goto out;
	}

	/* check if the file is actually writable */
	if ((statbuf.st_mode & 0222) == 0) //FIXME... check better what else vim does
	{
		g_set_error (&saver->priv->error,
			     GEDIT_DOCUMENT_ERROR,
			     GNOME_VFS_ERROR_READ_ONLY,
			     gnome_vfs_result_to_string (GNOME_VFS_ERROR_READ_ONLY));

		goto out;
	}

	/* prepare the backup name */
	backup_filename = get_backup_filename (saver);

	/* We now use two backup strategies.
	 * The first one (which is faster) consist in saving to a
	 * tmp file then rename the original file to the backup and the
	 * tmp file to the original name. This is fast but doesn't work
	 * when the file is a link (hard or symbolic) or when we can't
	 * write to the current dir or can't set the permissions on the
	 * new file. We also do not use it when the backup is not in the
	 * current dir, since if it isn't on the same FS rename wont work.
	 * The second strategy consist simply in copying the old file
	 * to a backup file and rewrite the contents of the file.
	 */

	if (saver->priv->backups_in_curr_dir &&
	    !(statbuf.st_nlink > 1) &&
	    !g_file_test (saver->priv->local_path, G_FILE_TEST_IS_SYMLINK))
	{
		gchar *dirname;
		gchar *tmp_filename;
		gint tmpfd;

		dirname = g_path_get_dirname (saver->priv->local_path);
		tmp_filename = g_build_filename (dirname, ".gedit-save-XXXXXX", NULL);
		g_free (dirname);

		/* this modifies temp_filename to the used name */
		tmpfd = g_mkstemp (tmp_filename);
		if (tmpfd == -1)
			goto fallback_strategy;

		/* try to set permissions */
		if (fchown (tmpfd, statbuf.st_uid, statbuf.st_gid) == -1 &&
		    fchmod (tmpfd, statbuf.st_mode) == -1)
		{
			close (tmpfd);
			goto fallback_strategy;
		}

		if (!write_document_contents (tmpfd,
					      GTK_TEXT_BUFFER (saver->priv->document),
			 	 	      saver->priv->encoding,
				 	      &saver->priv->error))
		{
			close (tmpfd);
			goto out;
		}

		/* original -> backup */
		if (rename (saver->priv->local_path, backup_filename) != 0)
		{
			GnomeVFSResult result = gnome_vfs_result_from_errno ();

			g_set_error (&saver->priv->error,
				     GEDIT_DOCUMENT_ERROR,
				     result,
				     gnome_vfs_result_to_string (result));

			close (tmpfd);
			unlink (tmp_filename);
			goto out;
		}

		/* tmp -> original */
		if (rename (tmp_filename, saver->priv->local_path) != 0)
		{
			GnomeVFSResult result = gnome_vfs_result_from_errno ();

			g_set_error (&saver->priv->error,
				     GEDIT_DOCUMENT_ERROR,
				     result,
				     gnome_vfs_result_to_string (result));

			/* try to restore... no error checking */
			rename (backup_filename, saver->priv->local_path);

			close (tmpfd);
			unlink (tmp_filename);
			goto out;
		}

		if (!saver->priv->keep_backup)
			unlink (backup_filename);

		goto out;
	}

 fallback_strategy:

	/* move away old backups */
	if (!remove_file (backup_filename))
	{
		GnomeVFSResult result = gnome_vfs_result_from_errno ();

		g_set_error (&saver->priv->error,
			     GEDIT_DOCUMENT_ERROR,
			     result,
			     gnome_vfs_result_to_string (result));

		goto out;
	}

	bfd = open (backup_filename,
		    O_WRONLY | O_CREAT | O_EXCL,
		    statbuf.st_mode & 0777);

	if (bfd == -1)
	{
		GnomeVFSResult result = gnome_vfs_result_from_errno ();

		g_set_error (&saver->priv->error,
			     GEDIT_DOCUMENT_ERROR,
			     result,
			     gnome_vfs_result_to_string (result));

		goto out;
	}	

	/* Try to set the group of the backup same as the
	 * original file. If this fails, set the protection
	 * bits for the group same as the protection bits for
	 * others. */
	if (fchown (bfd, (uid_t) -1, statbuf.st_gid) != 0)
	{
		if (fchmod (bfd,
		            (statbuf.st_mode& 0707) |
		            ((statbuf.st_mode & 07) << 3)) != 0)
		{
			GnomeVFSResult result = gnome_vfs_result_from_errno ();

			g_set_error (&saver->priv->error,
				     GEDIT_DOCUMENT_ERROR,
				     result,
				     gnome_vfs_result_to_string (result));

			goto out;
		}
	}

	if (!copy_file_data (saver->priv->fd, bfd, &saver->priv->error))
		goto out;

	/* finally overwrite the original */
	write_document_contents (saver->priv->fd,
				 GTK_TEXT_BUFFER (saver->priv->document),
			 	 saver->priv->encoding,
				 &saver->priv->error);	

 out:
	save_completed_or_failed (saver);

	g_free (backup_filename);

	/* stop the timeout */
	return FALSE;
}

static gboolean
save_new_local_file (GeditDocumentSaver *saver)
{
	write_document_contents (saver->priv->fd,
				 GTK_TEXT_BUFFER (saver->priv->document),
			 	 saver->priv->encoding,
				 &saver->priv->error);

	save_completed_or_failed (saver);

	/* stop the timeout */
	return FALSE;
}

static gboolean
open_local_failed (GeditDocumentSaver *saver)
{
	save_completed_or_failed (saver);

	/* stop the timeout */
	return FALSE;
}

static void
save_local_file (GeditDocumentSaver *saver)
{
	GSourceFunc next_phase;

	/* saving start */
	g_signal_emit (saver,
		       signals[SAVING],
		       0,
		       FALSE,
		       NULL);

	/* the file doesn't exist, create it */
	saver->priv->fd = open (saver->priv->local_path,
			        O_CREAT | O_EXCL | O_WRONLY,
			        0666);
	if (saver->priv->fd != -1)
	{
		next_phase = (GSourceFunc) save_new_local_file;
		goto out;
	}

	/* the file already exist */
	else if (errno == EEXIST)
	{
		saver->priv->fd = open (saver->priv->local_path, O_RDWR);
		if (saver->priv->fd != -1)
		{
			next_phase = (GSourceFunc) save_existing_local_file;
			goto out;
		}
	}

	/* else error */
	GnomeVFSResult result = gnome_vfs_result_from_errno (); //may it happen that no errno?

	g_set_error (&saver->priv->error,
		     GEDIT_DOCUMENT_ERROR,
		     result,
		     gnome_vfs_result_to_string (result));

	next_phase = (GSourceFunc) open_local_failed;

 out:
	g_timeout_add_full (G_PRIORITY_HIGH,
			    0,
			    next_phase,
			    saver,
			    NULL);
}

gboolean
gedit_document_saver_save (GeditDocumentSaver  *saver,
			   const gchar         *uri,
//			   const gchar         *uri,
//			   gboolean	        keep_backup,
			   const GeditEncoding *encoding)
{
	gchar *local_path;

	g_return_val_if_fail (GEDIT_IS_DOCUMENT_SAVER (saver), FALSE);
	g_return_val_if_fail ((uri != NULL) && (strlen (uri) > 0), FALSE);

	// TODO: returns FALSE if uri is not valid?

	// CHECK: sanity check a max len for the uri?

	saver->priv->uri = g_strdup (uri); // needed?

// provvisorio... still to decide if fetch prefs here or in gedit-document
	saver->priv->backup_ext = g_strdup ("~"); // g_strdup (backup_extension);
	saver->priv->keep_backup = TRUE; //keep_backup;
	saver->priv->backups_in_curr_dir = TRUE;

	if (encoding != NULL)
		saver->priv->encoding = encoding;
	else
		saver->priv->encoding = gedit_encoding_get_utf8 ();

	local_path = gnome_vfs_get_local_path_from_uri (uri);
	if (local_path != NULL)
	{
		saver->priv->local_path = local_path;
		save_local_file (saver);
	}
	else
		// TODO: for remote files the plan is
		// to save to a local file in /tmp
		// and then xfer it to the remote location
		// asyncronously.
		g_return_val_if_reached (FALSE);

	return TRUE;
}

