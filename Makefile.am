## Process this file with automake to produce Makefile.in

SUBDIRS = macros po intl pixmaps plugins src help

man_MANS = gedit.1

dist-hook: gedit.spec
	cp gedit.spec $(distdir)

sysdir   = $(datadir)/gnome/apps/Applications
sys_in_files = gedit.desktop.in
sys_DATA = $(sys_in_files:.desktop.in=.desktop)
@XML_I18N_MERGE_DESKTOP_RULE@

mimedir = $(datadir)/mime-info
mime_DATA = gedit.keys

EXTRA_DIST = BUGS FAQ ChangeLog README README.plugins \
	INSTALL gedit.1 \
	ABOUT-NLS TODO gedit.spec.in app-docs.make \
	$(sys_DATA) $(sys_in_files) $(mime_DATA) \
	xml-i18n-extract xml-i18n-merge xml-i18n-update

## to automatically rebuild aclocal.m4 if any of the macros in `macros/' change
@MAINT@include macros/macros.dep
@MAINT@macros/macros.dep: macros/Makefile.am
@MAINT@	cd macros && $(MAKE) macros.dep
