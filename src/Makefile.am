INCLUDES = \
	-DGNOMELOCALEDIR=\""$(localedir)"\"   \
	$(WARN_CFLAGS)                        \
	$(DISABLE_DEPRECATED_CFLAGS)          \
	$(NAUTILUS_CFLAGS)

nautilus_extensiondir=$(libdir)/nautilus/extensions-1.0
nautilus_extension_LTLIBRARIES=libnautilus-filename-repairer.la

libnautilus_filename_repairer_la_SOURCES =    \
	filename-repairer.c                   \
	nautilus-filename-repairer.c          \
	nautilus-filename-repairer.h          \
	nautilus-filename-repairer-i18n.h

libnautilus_filename_repairer_la_LDFLAGS = -module -avoid-version
libnautilus_filename_repairer_la_LIBADD  = $(NAUTILUS_LIBS)
