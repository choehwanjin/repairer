
SUBDIRS = po src

EXTRA_DIST = \
        intltool-extract.in \
        intltool-update.in \
        intltool-merge.in

DISTCLEANFILES = \
        intltool-extract \
        intltool-update \
        intltool-merge

DISTCHECK_CONFIGURE_FLAGS = --with-nautilus-extension-dir=$${dc_install_base}/lib/nautilus/extension-2.0


dist-hook:
	@if test -d .svn ; then \
		svn2cl --include-rev --break-before-msg=2 --authors=svnauthors -o $(distdir)/ChangeLog ; \
	else \
		touch $(distdir)/ChangeLog ; \
	fi

.PHONY: log
log:
	@if test -d .svn ; then \
		svn2cl --include-rev --break-before-msg=2 --authors=svnauthors ; \
	else \
		touch ChangeLog ; \
	fi
