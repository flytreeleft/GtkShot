AM_CPPFLAGS = \
		$(all_includes) \
		$(X11_CFLAGS) \
		$(GTK_CFLAGS) \
		-I$(top_srcdir) \
		-I$(top_srcdir)/include \
		-I$(top_srcdir)/resource \
		-DPACKAGE_DATA_DIR=\""$(datadir)"\" \
		-DPACKAGE_LOCALE_DIR=\""$(prefix)/$(DATADIRNAME)/locale"\"

bin_PROGRAMS = gtkshot
gtkshot_SOURCES = \
		main.c \
		shot.c \
		toolbar.c \
		pen.c \
		pen-editor.c \
		input.c \
		utils.c
gtkshot_LDADD = $(X11_LIBS) $(GTK_LIBS) -lm
