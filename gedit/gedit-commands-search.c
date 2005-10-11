/*
 * gedit-search-commands.c
 * This file is part of gedit
 *
 * Copyright (C) 1998, 1999 Alex Roberts, Evan Lawrence
 * Copyright (C) 2000, 2001 Chema Celorio, Paolo Maggi
 * Copyright (C) 2002-2005 Paolo Maggi
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
 * Modified by the gedit Team, 1998-2005. See the AUTHORS file for a
 * list of people on the gedit Team.
 * See the ChangeLog files for a list of changes.
 *
 * $Id$
 */

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include "gedit-commands.h"
#include "gedit-window.h"
#include "gedit-debug.h"
#include "gedit-search-panel.h"

void
gedit_cmd_search_find (GtkAction   *action,
		       GeditWindow *window)
{
	GeditSearchPanel *sp;

	gedit_debug (DEBUG_COMMANDS);

	sp = GEDIT_SEARCH_PANEL (_gedit_window_get_search_panel (window));

	gedit_search_panel_focus_search (sp);
}

void
gedit_cmd_search_find_next (GtkAction   *action,
			    GeditWindow *window)
{
	GeditSearchPanel *sp;

	gedit_debug (DEBUG_COMMANDS);

	sp = GEDIT_SEARCH_PANEL (_gedit_window_get_search_panel (window));

	gedit_search_panel_search_again (sp, FALSE);
}

void
gedit_cmd_search_find_prev (GtkAction   *action,
			    GeditWindow *window)
{
	GeditSearchPanel *sp;

	gedit_debug (DEBUG_COMMANDS);

	sp = GEDIT_SEARCH_PANEL (_gedit_window_get_search_panel (window));

	gedit_search_panel_search_again (sp, TRUE);
}

void
gedit_cmd_search_replace (GtkAction   *action,
			  GeditWindow *window)
{
	GeditSearchPanel *sp;

	gedit_debug (DEBUG_COMMANDS);

	sp = GEDIT_SEARCH_PANEL (_gedit_window_get_search_panel (window));

	gedit_search_panel_focus_replace (sp);
}

void
gedit_cmd_search_goto_line (GtkAction   *action,
			    GeditWindow *window)
{
	GeditSearchPanel *sp;

	gedit_debug (DEBUG_COMMANDS);

	sp = GEDIT_SEARCH_PANEL (_gedit_window_get_search_panel (window));

	gedit_search_panel_focus_goto_line (sp);
}
