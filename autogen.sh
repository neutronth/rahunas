#!/bin/sh -e

CURDIR=`pwd`

for i in ./ xtables/setrawnat; do
  cd $CURDIR/$i
  libtoolize --force
  aclocal -I m4
  automake --add-missing
  autoconf
done

cd $CURDIR
