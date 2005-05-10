#!/usr/bin/env python

import os
import sys
import getopt

usage = """usage: generate-plugin.py [ options ] plugin-name (words must be separated by '-' )"""

help = """generate scheleton sources for a gedit plugin.

Options:
  -i, --internal        adds the plugin to gedit's configure.ac (TODO)
"""

tmpl_dir = 'plugin_template'

tmpl_files = (
    'gedit-xyz-plugin.h',
    'gedit-xyz-plugin.c',
    'xyz.gedit-plugin.desktop.in',
    'Makefile.am'
)


def copy_template_file(tmpl_file, replacements):
    dest_file = tmpl_file

    for a, b in replacements.items():
        dest_file = dest_file.replace(a, b)

    print "generating " + dest_file + " from " + tmpl_file

    dest_dir = os.path.dirname(dest_file)

    if not os.path.exists(dest_dir):
        os.makedirs(dest_dir)

    r = open(tmpl_file, 'r')
    w = open(dest_file, 'w')

    for line in r:
        for a, b in replacements.items():
            line = line.replace(a, b)
        w.write(line)

    r.close()
    w.close()

#main
internal = False

try:
    opts, args = getopt.getopt(sys.argv[1:], 'i:', ['internal', 'help'])
except getopt.error, exc:
    sys.stderr.write('gen-plugin: %s\n' % str(exc))
    sys.stderr.write(usage + '\n')
    sys.exit(1)

for opt in opts:
    if opt == '--help':
        print usage
        print help
        sys.exit(0)
    elif opt in ('-i', '--internal'):
        internal = True

if len(args) != 1:
    sys.stderr.write(usage + '\n')
    sys.exit(1)

new_name = args[0]

replacements = {
    'plugin_template': new_name,
    'gedit-xyz-plugin.c': 'gedit-' + new_name + '-plugin.c',
    'gedit-xyz-plugin.h': 'gedit-' + new_name + '-plugin.h',
    'xyz.gedit-plugin.desktop.in': new_name + '.gedit-plugin.desktop.in',
    'libxyz': 'lib' + new_name.replace('-', ''),
    'xyz': new_name,
    'xyz/': new_name + '/',
    'xyz-': new_name + '-',
    'xyz_': new_name.replace('-', '_') + '_',
    'Xyz': new_name.title().replace('-', ''),
    'XYZ': new_name.upper().replace('-', '_'),
}

tmpl_paths = []

for f in tmpl_files:
    tmpl_paths.append(os.path.join(os.path.dirname(__file__), tmpl_dir, f))

#if internal:
#    TODO: edit configure.in
#else
#    tmpl_paths.append(os.path.join(root, "configure.in"))

for f in tmpl_paths:
    copy_template_file(f, replacements)

