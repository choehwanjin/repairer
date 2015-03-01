INCLUDES = \
	-DGNOMELOCALEDIR=\""$(localedir)"\"   \
	$(WARN_CFLAGS)                        \
	$(DISABLE_DEPRECATED_CFLAGS)          \
	$(NAUTILUS_CFLAGS)

nautilus_extensiondir=$(NAUTILUS_EXTENSION_DIR)
nautilus_extension_LTLIBRARIES=libnautilus-filename-repairer.la

libnautilus_filename_repairer_la_SOURCES =    \
	filename-repairer.c                   \
	nautilus-filename-repairer.c          \
	nautilus-filename-repairer.h          \
	nautilus-filename-repairer-i18n.h     \
	$(NULL)

libnautilus_filename_repairer_la_LDFLAGS = -module -avoid-version
libnautilus_filename_repairer_la_LIBADD  = $(NAUTILUS_LIBS)

bin_PROGRAMS = nautilus-filename-repairer

nautilus_filename_repairer_SOURCES = \
	repairer.c \
	repair-dialog.h \
	repair-dialog.c \
	encoding-dialog.h \
	encoding-dialog.c \
	repairer-utils.h \
	repairer-utils.c \
	$(NULL)

nautilus_filename_repairer_CFLAGS = \
	-DPKGDATADIR=\"$(pkgdatadir)\" \
	$(NAUTILUS_CFLAGS) \
	$(NULL)

nautilus_filename_repairer_LDADD = $(NAUTILUS_LIBS)

pkgdata_DATA = \
	repair-dialog.ui \
	encoding-dialog.ui \
	$(NULL)

EXTRA_DIST = \
	$(pkgdata_DATA) \
	$(NULL)
