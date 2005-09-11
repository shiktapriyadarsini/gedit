gladedir = $(datadir)/gedit-2/glade/

INCLUDES = 							\
	-I$(top_srcdir)						\
	-I$(top_srcdir)/gedit 					\
	$(GEDIT_CFLAGS) 					\
	$(WARN_CFLAGS)						\
	$(DISABLE_DEPRECATED_CFLAGS)				\
	-DGNOME_ICONDIR=\""$(datadir)/pixmaps"\" 		\
	-DGEDIT_GLADEDIR=\""$(datadir)/gedit-2/glade/"\"	

noinst_LTLIBRARIES = libdialogs.la

libdialogs_la_SOURCES = 			\
	gedit-page-setup-dialog.c		\
	gedit-page-setup-dialog.h		\
	gedit-plugin-manager.h			\
	gedit-plugin-manager.c			\
	gedit-preferences-dialog.h		\
	gedit-preferences-dialog.c		\
	gedit-close-confirmation-dialog.c 	\
	gedit-close-confirmation-dialog.h 	\
	gedit-encodings-dialog.c		\
	gedit-encodings-dialog.h		\
	gedit-open-location-dialog.c		\
	gedit-open-location-dialog.h


glade_DATA =				\
	gedit-encodings-dialog.glade2	\
	gedit-preferences.glade2	\
	page-setup-dialog.glade2	\
	plugin-manager.glade2		\
	uri.glade2			\
	goto-line.glade2

#	replace.glade2
#	program-location-dialog.glade2


EXTRA_DIST = $(glade_DATA) 

