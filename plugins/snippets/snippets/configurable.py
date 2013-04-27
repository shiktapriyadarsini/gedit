#    Gedit snippets plugin
#    Copyright (C) 2005-2006  Jesse van den Kieboom <jesse@icecrew.nl>
#    Copyright (C) 2013       Ignacio Casal Quinteiro <icq@gnome.org>
#
#    This program is free software; you can redistribute it and/or modify
#    it under the terms of the GNU General Public License as published by
#    the Free Software Foundation; either version 2 of the License, or
#    (at your option) any later version.
#
#    This program is distributed in the hope that it will be useful,
#    but WITHOUT ANY WARRANTY; without even the implied warranty of
#    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
#    GNU General Public License for more details.
#
#    You should have received a copy of the GNU General Public License
#    along with this program; if not, write to the Free Software
#    Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA

import os
from gi.repository import Gtk, Gedit, GObject

class Configurable(GObject.Object, Gedit.Configurable):

        def __init__(self):
                GObject.Object.__init__(self)

        def do_get_page_id(self):
                return "snippets"

        def do_get_page_name(self):
                return _("Snippets")

        def do_create_configure_widget(self):
                builder = Gtk.Builder()
                builder.add_from_file(os.path.join(self.plugin_info.get_data_dir(), 'ui', 'snippets.ui'))
                return builder.get_object('snippets_manager')

# ex:ts=8:et:
