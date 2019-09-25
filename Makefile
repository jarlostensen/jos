AS := nasm
ASFLAGS := -f elf32
#ASFLAGS := -f bin

LD := ld
LDFLAGS := -melf_i386
LDFILE := link.ld

MKRESCUE := grub-mkrescue

all: kernel

kernel: kernel.o 
	$(LD) $(LDFLAGS) -T $(LDFILE) $< -o $@
kernel.o: kernel.asm
#kernel: kernel.asm
	$(AS) $(ASFLAGS) $< -o $@

iso: kernel
	mkdir -p isodir/boot/grub
	cp kernel isodir/boot/
	cp grub.cfg isodir/boot/grub
	$(MKRESCUE) -o minimal.iso isodir

iso2: kernel
	cp kernel isodir/boot/
	genisoimage -R -b boot/grub/stage2_eltorito -no-emul-boot -boot-load-size 4 -A os -input-charset utf8 -quiet -boot-info-table -o minimal.iso isodir

clean:
	rm -f *.o *.iso kernel isodir/boot/kernel 