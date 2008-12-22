/*
 * gedit-dirs.c
 * This file is part of gedit
 *
 * Copyright (C) 2008 Ignacio Casal Quinteiro
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

#include "gedit-dirs.h"

gchar *
gedit_dirs_get_config_dir ()
{
#ifndef G_OS_WIN32
	const gchar *home;
	
	home = g_get_home_dir ();
	
	if (home != NULL)
	{
		return g_build_filename (home,
					 ".gnome2",
					 NULL);
	}
#else
	return g_strdup (g_get_user_config_dir ());
#endif
	return NULL;
}

gchar *
gedit_dirs_get_gedit_data_dir (void)
{
	gchar *data_dir;

#ifndef G_OS_WIN32
	data_dir = g_build_filename (DATADIR,
				     "gedit-2",
				     NULL);
#else
	gchar *win32_dir;
	
	win32_dir = g_win32_get_package_installation_directory_of_module (NULL)

	data_dir = g_build_filename (win32_dir,
				     "share",
				     "gedit-2",
				     NULL);
	
	g_free (win32_dir);
#endif

	return data_dir;
}

gchar *
gedit_dirs_get_gedit_lib_dir (void)
{
	gchar *lib_dir;

#ifndef G_OS_WIN32
	lib_dir = g_build_filename (LIBDIR,
				    "gedit-2",
				    NULL);
#else
	gchar *win32_dir;
	
	win32_dir = g_win32_get_package_installation_directory_of_module (NULL)

	lib_dir = g_build_filename (win32_dir,
				    "lib",
				    "gedit-2",
				    NULL);
	
	g_free (win32_dir);
#endif

	return lib_dir;
}

gchar *
gedit_dirs_get_ui_file (const gchar *file)
{
	gchar *datadir;
	gchar *ui_file;

	g_return_val_if_fail (file != NULL, NULL);
	
	datadir = gedit_dirs_get_gedit_data_dir ();
	ui_file = g_build_filename (datadir,
				    "ui",
				    file,
				    NULL);
	g_free (datadir);
	
	return ui_file;
}
