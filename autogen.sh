#!/bin/sh

libtoolize --force
autopoint --force
aclocal
automake --add-missing
autoconf
