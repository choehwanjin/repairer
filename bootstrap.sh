#!/bin/sh

libtoolize --copy --force
intltoolize --copy --force --automake
aclocal
autoheader
automake --add-missing --copy
autoconf
