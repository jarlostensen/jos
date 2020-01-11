#!/bin/sh
set -e
. ./headers.sh

for PROJECT in $PROJECTS; do
  (cd $PROJECT && DESTDIR="$SYSROOT" $MAKE install)
done


if [ "$BUILD_CONFIG" = "DEBUG" ]; then
  echo "generating symbol file jos.sym"
  objcopy --only-keep-debug $SYSROOT/boot/jos.kernel jos.sym
fi