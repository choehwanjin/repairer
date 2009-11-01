AC_INIT([nautilus-filename-repairer], [0.0.6], [choe.hwanjin@gmail.com], [nautilus-filename-repairer])
AC_CONFIG_SRCDIR([Makefile.am])
AC_CONFIG_HEADERS(config.h)
AM_INIT_AUTOMAKE([dist-bzip2])

# Checks for programs.
AC_PROG_CC
AC_DISABLE_STATIC
IT_PROG_INTLTOOL([0.35.0])
AC_PROG_LIBTOOL
PKG_PROG_PKG_CONFIG

AH_TEMPLATE([GETTEXT_PACKAGE], [Package name for gettext])
GETTEXT_PACKAGE=nautilus-filename-repairer
AC_DEFINE_UNQUOTED(GETTEXT_PACKAGE, "$GETTEXT_PACKAGE")
AC_SUBST(GETTEXT_PACKAGE)
AM_GLIB_GNU_GETTEXT

AC_SUBST(CFLAGS)
AC_SUBST(CPPFLAGS)
AC_SUBST(LDFLAGS)

NAUTILUS_REQUIRED=2.22.0
GLIB_REQUIRED=2.16.0
GTK_REQUIRED=2.6.0

PKG_CHECK_MODULES(NAUTILUS, [
    libnautilus-extension >= $NAUTILUS_REQUIRED
    glib-2.0 >= $GLIB_REQUIRED
    gtk+-2.0 >= $GTK_REQUIRED
])
AC_SUBST(NAUTILUS_CFLAGS)
AC_SUBST(NAUTILUS_LIBS)

AC_ARG_WITH(nautilus-extension-dir,
	      [AS_HELP_STRING([--with-nautilus-extension-dir],
		    [specify the nautilus extension directory])])

AC_MSG_CHECKING([for nautilus extension directory])
if test -n "$with_nautilus_extension_dir"; then
    NAUTILUS_EXTENSION_DIR=$with_nautilus_extension_dir
else
    NAUTILUS_EXTENSION_DIR=`$PKG_CONFIG --variable=extensiondir libnautilus-extension`
fi
if test -z "$NAUTILUS_EXTENSION_DIR"; then
    NAUTILUS_EXTENSION_DIR='${exec_prefix}/lib/nautilus/extensions-2.0'
fi

AC_MSG_RESULT([${NAUTILUS_EXTENSION_DIR}])
AC_SUBST(NAUTILUS_EXTENSION_DIR)

AC_CONFIG_FILES([Makefile
		 src/Makefile
		 po/Makefile.in])

AC_OUTPUT
