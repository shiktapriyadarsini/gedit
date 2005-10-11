## Process this file with automake to produce Makefile.in
SUBDIRS = dialogs recent-files

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
	
if ENABLE_PYTHON
INCLUDES += \
	$(NO_STRICT_ALIASING_CFLAGS)	\
	$(PYGTK_CFLAGS)			\
	$(PYTHON_CFLAGS)		\
	$(AM_CFLAGS)
endif

noinst_LTLIBRARIES = libgedit.la

gedit_LDFLAGS = -export-dynamic

gedit_LDADD =				\
	$(GEDIT_LIBS)			\
	libgedit.la			\
	dialogs/libdialogs.la		\
	recent-files/librecent.la

if ENABLE_PYTHON
gedit_LDADD += \
	$(top_srcdir)/bindings/python/gedit.la
endif

BUILT_SOURCES = 			\
	gedit-marshal.c			\
	gedit-marshal.h              

NOINST_H_FILES =			\
	gedit-commands.h		\
	gedit-file-selector-util.h 	\
	gedit-io-error-dialogs.h	\
	gedit-tooltips.h		\
	gedit-languages-manager.h	\
	gedit-plugin-manager.h		\
	gedit-plugins-engine.h		\
	gedit-module.h			\
	gedit-ui.h			\
	gedit-window-private.h		\
	gedit-documents-panel.h		\
	gedit-document-loader.h		\
	gedit-document-saver.h		\
	gedit-io-error-message-area.h	\
	gedit-search-panel.h		\
	gedit-spinner.h			\
	gedit-session.h

if ENABLE_PYTHON
NOINST_H_FILES += \
	gedit-python-module.h \
	gedit-python-plugin.h 
endif

INST_H_FILES =				\
	gedit-utils.h 			\
	gedit-help.h 			\
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
	gedit-encodings-option-menu.h	\
	gedit-file-chooser-dialog.h	\
	gedit-message-area.h		\
	gedit-print-job-preview.h	\
	gedit-progress-message-area.h	\
	gedit-plugin.h			\
	gedit-app.h

#	gedit2.h			\
#	gedit-file.h			\
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
	$(BACON_FILES)			\
	gedit.c				\
	gedit-commands.c		\
	gedit-commands-file-print.c	\
	gedit-commands-edit.c		\
	gedit-view-commands.c		\
	gedit-documents-commands.c	\
	gedit-search-commands.c		\
	gedit-help-commands.c		\
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
	gedit-help.c			\
	gedit-languages-manager.c	\
	gedit-print.c			\
	gedit-recent.c			\
	gedit-app.c			\
	gedit-documents-panel.c		\
	gedit-search-panel.c		\
	gedit-document-loader.c		\
	gedit-document-saver.c		\
	gedit-file-chooser-dialog.c	\
	gedit-encodings-option-menu.c	\
	gedit-message-area.c		\
	gedit-io-error-message-area.c	\
	gedit-print-job-preview.c	\
	gedit-progress-message-area.c	\
	gedit-plugin.c			\
	gedit-module.c			\
	gedit-plugins-engine.c		\
	gedit-plugin-manager.c		\
	gedit-spinner.c			\
	gedit-session.c			\
	$(NOINST_H_FILES)		\
	$(INST_H_FILES)


#	gedit2.c			\
#	gedit-menus.c			\
#	gedit-file.c			\
#	gedit-file-selector-util.c 	\
#	gedit-io-error-dialogs.c	\
#	gedit-output-window.c

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

if ENABLE_PYTHON
libgedit_la_SOURCES += \
	gedit-python-module.c	\
	gedit-python-module.h	\
	gedit-python-plugin.c	\
	gedit-python-plugin.h	
endif

gedit-marshal.h: gedit-marshal.list $(GLIB_GENMARSHAL)
	$(GLIB_GENMARSHAL) $< --header --prefix=gedit_marshal > $@

gedit-marshal.c: gedit-marshal.list $(GLIB_GENMARSHAL)
	echo "#include \"gedit-marshal.h\"" > $@ && \
	$(GLIB_GENMARSHAL) $< --body --prefix=gedit_marshal >> $@

ui_DATA = gedit-ui.xml
uidir = $(datadir)/gedit-2/ui/

gladedir = $(datadir)/gedit-2/glade/

glade_DATA = search-panel.glade

EXTRA_DIST = 				\
	$(ui_DATA)			\
	$(glade_DATA)			\
	gedit-marshal.list

CLEANFILES = $(BUILT_SOURCES)

dist-hook:
	cd $(distdir); rm -f $(BUILT_SOURCES)

install-exec-local:
	rm -f $(DESTDIR)$(bindir)/gnome-text-editor
	ln -s gedit $(DESTDIR)$(bindir)/gnome-text-editor

BACON_DIR=$(srcdir)/../../libbacon/src/
BACON_FILES=bacon-message-connection.h bacon-message-connection.c

regenerate-built-sources:
	BACONFILES="$(BACON_FILES)" BACONDIR="$(BACON_DIR)" $(top_srcdir)/gedit/update-from-bacon.sh
