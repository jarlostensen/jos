#!/bin/sh

SYSTEM_HEADER_PROJECTS="libc kernel"
export PREFIX=/usr
export EXEC_PREFIX=$PREFIX
export BOOTDIR=/boot
export LIBDIR=$EXEC_PREFIX/lib
export INCLUDEDIR=$PREFIX/include

export SYSROOT=$(pwd)/sysroot
mkdir -p "$SYSROOT"

# copy system headers to sysroot as needed
for PROJECT in $SYSTEM_HEADER_PROJECTS; do
  (cd $PROJECT && DESTDIR="$SYSROOT" make install-headers)
done
