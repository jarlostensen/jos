# jos
My own minimal OS.<br/>
This is very much a learning project, it will probably never go beyond that. My ultimate goal is to get it to a point where I can switch to 64-bit long mode and have full MP APIC support so that I can play with timers and perf in various VM environments.<br/>
Beyond that, who knows...

Some features (some done, some constantly in flux)
* 32-bit protected mode kernel, virtual memory mapped ("high" kernel).
* PIC and PIT support (clock and interrupts).
* libc functionality (like printf).
* memory management (arenas, pools).
* task management (in-kernel switching only).
* multi-cpu support.

Built on knowledge from:
* https://wiki.osdev.org/Main_Page
* http://www.osdever.net
* www.jamesmolloy.co.uk
* Tanebaum & Woodhull "Operating Systems"

## Dependencies/Tools used

* GCC cross compiler.
* GRUB 2 loader.
* make and build scripts, libc framework from https://wiki.osdev.org/Meaty_Skeleton.
* NASM (because I just can't stomach AT&T syntax).
* Bochs debugger http://bochs.sourceforge.net/doc/docbook/user/internal-debugger.html .
* VSCode.
* WSL.
* BOCHS.

## Dev setup
My dev environment is Windows 10 so I have Ubuntu runing in WSL where the build environment (and GCC cross compiler) live.
I write code using VSCode in Windows, having a working directory mounted in WSL using standard ```ln```. 
I am using the windows version of Bochs, but so far only the non-GUI version, and run it from a command prompt.

In order to debug I use a combination of bochsdbg, printfs, and a serial-port trace feature I've built in. Both Bochs and VMWare supports capturing serial output to one of the COM ports to a file which helps greatly. <br/>

## Reading the code
The best place to start is in ```kernel\arch\i386\kernel_loader.asm```. This is where the kernel is first loaded by GRUB and where it sets up the initial descriptor tables, memory mappings, and switches to protected mode.
It invokes code in ```kernel\kernel\kernel.c``` which is where all the kernel subsystems are initialised and test code runs, etc.
