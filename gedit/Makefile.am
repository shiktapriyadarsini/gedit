## Process this file with automake to produce Makefile.in
SUBDIRS= dialogs recent-files

bin_PROGRAMS = gedit

INCLUDES =							\
	-I$(top_srcdir)						\
	-I$(srcdir)						\
	$(GEDIT_CFLAGS)						\
	$(WARN_CFLAGS)						\
	$(DISABLE_DEPRECATED_CFLAGS)				\
	-DDATADIR=\""$(datadir)"\"				\
	-DGNOME_ICONDIR=\""$(datadir)/pixmaps"\"		\
	-DGEDIT_LOCALEDIR=\""$(prefix)/$(DATADIRNAME)/locale"\"	\
	-DGEDIT_UI_DIR=\""$(datadir)/gedit-2/ui/"\"		\
	-DGEDIT_PLUGINDIR=\""$(libdir)/gedit-2/plugins"\"	\
	-DGEDIT_GLADEDIR=\""$(datadir)/gedit-2/glade/"\"

noinst_LTLIBRARIES = libgedit.la

gedit_LDFLAGS = -export-dynamic

gedit_LDADD =				\
	$(GEDIT_LIBS)			\
	libgedit.la			\
	dialogs/libdialogs.la		\
	recent-files/librecent.la

BUILT_SOURCES = 			\
	gedit-marshal.c			\
	gedit-marshal.h              

NON_INST_H_FILS =			\
	gedit-commands.h		\
	gedit-file-selector-util.h 	\
	gedit-io-error-dialogs.h	\
	gedit-tooltips.h		\
	gedit-languages-manager.h	\
	gedit-plugins-engine.h		\
	gedit-ui.h			\
	gedit-documents-panel.h		\
	gedit-search-panel.h

#	gedit-session.h

INST_H_FILES =				\
	gedit-utils.h 			\
	gedit-document.h 		\
	gedit-view.h 			\
	gedit-tab.h 			\
	gedit-notebook.h		\
	gedit-prefs-manager-app.h	\
	gedit-recent.h			\
	gedit-window.h			\
	gedit-panel.h			\
	gedit-statusbar.h		\
	gedit-print.h			\
	gedit-app.h

#	gedit2.h			\
#	gedit-file.h			\
#	gedit-encodings-option-menu.h	\
#	gedit-menus.h			\
#	bonobo-mdi.h			\
#	gedit-plugin.h			\
#	gedit-output-window.h


LIB_H_FILES =				\
	gedit-encodings.h		\
	gedit-convert.h			\
	gedit-debug.h			\
	gedit-prefs-manager.h		\
	gedit-metadata-manager.h	


headerdir = $(prefix)/include/gedit-@GEDIT_MAJOR@/gedit

header_DATA = 				\
	$(INST_H_FILES)			\
	$(LIB_H_FILES)

gedit_SOURCES = 			\
	$(BUILT_SOURCES)		\
	gedit.c				\
	gedit-commands.c		\
	gedit-document.c 		\
	gedit-view.c 			\
	gedit-tab.c 			\
	gedit-notebook.c		\
	gedit-prefs-manager-app.c	\
	gedit-tooltips.c		\
	gedit-window.c			\
	gedit-panel.c			\
	gedit-statusbar.c		\
	gedit-utils.c 			\
	gedit-languages-manager.c	\
	gedit-plugins-engine.c		\
	gedit-print.c			\
	gedit-recent.c			\
	gedit-app.c			\
	gedit-documents-panel.c		\
	gedit-search-panel.c		\
	$(NON_INST_H_FILS)		\
	$(INST_H_FILES)


#	gedit2.c			\
#	gedit-menus.c			\
#	gedit-file.c			\
#	gedit-session.c			\
#	gedit-file-selector-util.c 	\
#	gedit-io-error-dialogs.c	\
#	gedit-plugin.c			\
#	gedit-output-window.c		\
#	gedit-encodings-option-menu.c

libgedit_la_SOURCES = 			\
	gedit-debug.c			\
	gedit-convert.c			\
	gedit-encodings.c		\
	gedit-prefs-manager.c		\
	gedit-prefs-manager-private.h	\
	gedit-metadata-manager.c	\
	gedit-encodings.c		\
	gedit-convert.c			\
	$(LIB_H_FILES)

gedit-marshal.h: gedit-marshal.list $(GLIB_GENMARSHAL)
	$(GLIB_GENMARSHAL) $< --header --prefix=gedit_marshal > $@

gedit-marshal.c: gedit-marshal.list $(GLIB_GENMARSHAL)
	echo "#include \"gedit-marshal.h\"" > $@ && \
	$(GLIB_GENMARSHAL) $< --body --prefix=gedit_marshal >> $@

ui_DATA = gedit-ui.xml
uidir = $(datadir)/gedit-2/ui/

EXTRA_DIST = 				\
	$(ui_DATA)			\
	gedit-marshal.list

CLEANFILES = $(BUILT_SOURCES)

dist-hook:
	cd $(distdir); rm -f $(BUILT_SOURCES)

install-exec-local:
	rm -f $(DESTDIR)$(bindir)/gnome-text-editor
	ln -s gedit $(DESTDIR)$(bindir)/gnome-text-editor

