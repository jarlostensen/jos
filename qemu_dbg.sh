#!/bin/sh
qemu-system-i386 -m 620M -s -S -cdrom jos.iso -serial file:qemu_serial.txt

