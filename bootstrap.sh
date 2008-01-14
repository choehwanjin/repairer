#!/bin/sh

libtoolize --copy
intltoolize --copy
aclocal
autoheader
automake --add-missing --copy
autoconf
