#!/bin/sh
# from meaty_skeleton build configuration
SYSTEM_HEADER_PROJECTS="libc kernel"
export PREFIX=/usr
export EXEC_PREFIX=$PREFIX
export BOOTDIR=/boot
export LIBDIR=$EXEC_PREFIX/lib
export INCLUDEDIR=$PREFIX/include
# for cross compiling without any system libraries, expects this folder to contain stdio.h etc.
export SYSROOT=$(pwd)/sysroot
echo $SYSROOT
mkdir -p "$SYSROOT"
#for PROJECT in $SYSTEM_HEADER_PROJECTS; do
#  (cd $PROJECT && DESTDIR="$SYSROOT" make install-headers)
#done
