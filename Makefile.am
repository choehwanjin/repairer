
SUBDIRS = po src

EXTRA_DIST = \
        intltool-extract.in \
        intltool-update.in \
        intltool-merge.in

DISTCLEANFILES = \
        intltool-extract \
        intltool-update \
        intltool-merge

ChangeLog:
	svn2cl --include-rev --break-before-msg=2 --authors=svnauthors
