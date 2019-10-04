#!/bin/sh

PROJECTS="libc kernel"

export SYSROOT=$(pwd)/sysroot
rm -rf $SYSROOT

for PROJECT in $PROJECTS; do
  (cd $PROJECT make clean)
done
