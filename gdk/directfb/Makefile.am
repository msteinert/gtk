## Process this file with automake to produce Makefile.in
include $(top_srcdir)/Makefile.decl

libgdkincludedir = $(includedir)/gtk-3.0/gdk
libgdkdirectfbincludedir = $(includedir)/gtk-3.0/gdk/directfb

AM_CPPFLAGS =					\
	-DG_LOG_DOMAIN=\"Gdk\"			\
	-DGDK_COMPILATION			\
	-I$(top_srcdir)				\
	-I$(top_srcdir)/gdk			\
	-I$(top_builddir)/gdk			\
	$(GDK_HIDDEN_VISIBILITY_CFLAGS)		\
	$(GTK_DEBUG_FLAGS)			\
	$(GDK_DEP_CFLAGS)

LDADDS = $(GDK_DEP_LIBS)

noinst_LTLIBRARIES =				\
	libgdk-directfb.la

libgdk_directfb_la_SOURCES =			\
	gdkcursor-directfb.c			\
	gdkcursor-directfb.h			\
	gdkcursortheme.c			\
	gdkcursortheme.h			\
	gdkcursortheme-x11.c			\
	gdkcursortheme-x11.h			\
	gdkcursortheme-xcursor.c		\
	gdkcursortheme-xcursor.h		\
	gdkdevice-directfb.c			\
	gdkdevice-directfb.h			\
	gdkdevicemanager-directfb.c		\
	gdkdevicemanager-directfb.h		\
	gdkdisplay-directfb.c			\
	gdkdisplay-directfb.h			\
	gdkdragcontext-directfb.c		\
	gdkdragcontext-directfb.h		\
	gdkkeymap-directfb.c			\
	gdkkeymap-directfb.h			\
	gdkprivate-directfb.c			\
	gdkprivate-directfb.h			\
	gdkscreen-directfb.c			\
	gdkscreen-directfb.h			\
	gdksource-directfb.c			\
	gdksource-directfb.h			\
	gdkwindow-directfb.c			\
	gdkwindow-directfb.h			\
	xcursor.c				\
	xcursor.h

libgdkinclude_HEADERS =				\
	gdkdirectfb.h

libgdkdirectfbinclude_HEADERS = 		\
	gdkdirectfbdevice.h			\
	gdkdirectfbdisplay.h			\
	gdkdirectfbscreen.h			\
	gdkdirectfbwindow.h

-include $(top_srcdir)/git.mk
