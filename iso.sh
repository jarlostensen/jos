#!/bin/sh
set -e
. ./build.sh

mkdir -p isodir
mkdir -p isodir/boot
mkdir -p isodir/boot/grub

cp sysroot/boot/jos.kernel isodir/boot/jos.kernel
cat > isodir/boot/grub/grub.cfg << EOF
menuentry "jos" {
	multiboot /boot/jos.kernel
}
EOF
grub-mkrescue -o jos.iso isodir
