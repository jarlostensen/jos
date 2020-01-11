#!/bin/sh
set -e
#. ./iso.sh
#qemu-system-$(./target-triplet-to-arch.sh $HOST) -cdrom jos.iso -serial file:qemu_serial.txt
qemu-system-i386 -cdrom jos.iso -serial file:qemu_serial.txt
