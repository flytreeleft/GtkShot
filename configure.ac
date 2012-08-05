AC_INIT([GtkShot], [2.0], [crazydan0@126.com])
AM_INIT_AUTOMAKE([-Wall -Werror foreign])
AC_CONFIG_HEADERS([config.h])
AC_CONFIG_FILES([Makefile src/Makefile po/Makefile])

AC_LANG_C
AM_PROG_LIBTOOL

AC_ISC_POSIX
AC_PROG_CC
AM_PROG_CC_STDC
AM_PROG_CC_C_O
AC_HEADER_STDC

#AM_GLIB_GNU_GETTEXT
#IT_PROG_INTLTOOL([0.40.0])

AC_DEFINE([GTK_SHOT_NAME], ["@S|@{PACKAGE_NAME}"], [The name of programme])
AC_DEFINE([GTK_SHOT_VERSION], ["@S|@{PACKAGE_VERSION}"], [The version of programme])
AC_DEFINE([SYSTEM_CURSOR_SIZE], [16], [The size of cursor])
AC_DEFINE([GTK_SHOT_ANCHOR_BORDER], [6], [The border of anchor])
AC_DEFINE([GTK_SHOT_SECTION_BORDER], [2], [The border of selection of capture])
AC_DEFINE([GTK_SHOT_SECTION_COLOR], [0x00ff00], [The color of selection of capture])
AC_DEFINE([GTK_SHOT_OPACITY], [0.8], [The opacity of mask])
AC_DEFINE([GTK_SHOT_COLOR], [0x000000], [The color of mask])
AC_DEFINE([GTK_SHOT_DEFAULT_PEN_FONT], ["Sans 10"], [The font name of pen])
AC_DEFINE([GTK_SHOT_DEFAULT_PEN_COLOR], [0xff0000], [The color of pen])
AC_DEFINE([GTK_SHOT_DEFAULT_PEN_SIZE], [2], [The size of pen])

AC_ARG_ENABLE(anchor,
              AC_HELP_STRING([--enable-anchor], [enable to use anchor to resize area of capture]),
              [case "${enableval}" in
                yes) enable_anchor=yes ;;
                no) enable_anchor=no ;;
                *) AC_MSG_ERROR([bad value "${enableval}" for --enable-anchor, use "yes" (default) or "no".]) ;;
               esac], [enable_anchor=yes])
if test "x${enable_anchor}" = "xyes" ; then
  no_anchor=0
else
  no_anchor=1
fi
AC_DEFINE([GTK_SHOT_NO_ANCHOR], [@S|@{no_anchor}], [Whether using anchor around area of caputure])

AC_ARG_ENABLE(debug,
              AC_HELP_STRING([--enable-debug], [enable to print debug information]),
              [case "${enableval}" in
                yes) enable_debug = yes ;;
                no) enable_debug = no ;;
                *) AC_MSG_ERROR([bad value "${enableval}" for --enable-debug, use "yes" (default) or "no".]) ;;
               esac], [enable_debug=no])
if test "x$enable_debug" = "xyes" ; then
  AC_DEFINE([GTK_SHOT_DEBUG], [], [Print debug information when programme is running])
fi

gtk_modules="gtk+-2.0 >= 2.12.0"
PKG_CHECK_MODULES(GTK, [$gtk_modules])
AC_SUBST(GTK_CFLAGS)
AC_SUBST(GTK_LIBS)

pkg_modules="x11"
PKG_CHECK_MODULES(X11, [$pkg_modules])
AC_SUBST(X11_CFLAGS)
AC_SUBST(X11_LIBS)

AC_OUTPUT

echo
echo $PACKAGE_NAME....................... : Version $PACKAGE_VERSION
echo Prefix..........................: $prefix
echo The binary will be installed in $prefix/bin
echo http://crazydan.org/

