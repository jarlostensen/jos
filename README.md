# jos
My own minimal OS.<br/>
This is very much a learning project, it will probably never go beyond that. My ultimate goal is to get it to a point where I can switch to 64-bit long mode and have full MP APIC support so that I can play with timers and perf in various VM environments.<br/>
Beyond that, who knows...

Some features (some done, some constantly in flux)
* 32-bit protected mode kernel, virtual memory mapped ("high" kernel).
* PIC and PIT support (clock and interrupts).
* libc functionality (like printf).
* memory management (arenas, pools).
* basic task management (in-kernel switching only).
* multi-cpu support.

Built on knowledge from:
* https://wiki.osdev.org/Main_Page
* http://www.osdever.net
* www.jamesmolloy.co.uk
* Tanebaum & Woodhull "Operating Systems"
* countless blog posts and stackoverflow snippets...

## Dependencies/Tools used

* GCC cross compiler built locally.
* GRUB 2 loader.
* make and build scripts, based on https://wiki.osdev.org/Meaty_Skeleton.
* NASM (because I just can't stomach AT&T syntax).
* Bochs debugger http://bochs.sourceforge.net/doc/docbook/user/internal-debugger.html .
* VSCode & VisualStudio.
* WSL.
* BOCHS.
* QEMU and GDB.
* VMWare Workstation Player (free).

## Dev setup
The kernel itself is built in Ubuntu running in WSL, using the locally built GCC cross compiler (9.2.0 at the time of writing). 
I edit the code in VSCode or VisualStudio, using the latter for the "lab" project which is where I experiment with various pieces of functionality in a more easy-to-debug-and-test environment (at least for me). </br>
The project itself is managed using makefiles and built using a couple of bash scripts, ```build.sh``` and ```iso.sh``` being the main ones. 
<br/>
To debug I use a mix of ```bochsdbg``` and ```gdb```, the former runs directly in Windows and is quick to get up and running and since it's an emulator it is ideal for debugging hardware state issues. In the code I use a macro ```JOS_BOCHS_DBGBREAK``` to break at particular points, this works but is of course not as smooth as in-debugger source level breakpoints. For that you need GDB. <br/>
To use GDB I run the kernel in ```qemu```, in a VMWare Ubuntu VM, and start it in suspended, listening mode, with the parameters ```-s -S```. The debug build configuration for the kernel outputs a symbol file (as ```jos.sym```) which I load into GDB for source level debugging.
<br/>
In addition I use serial port tracing (```K_TRACE``` in the code) and good ol' ```printf```.

## Reading the code
The best place to start is in ```kernel\arch\i386\kernel_loader.asm```. This is where the kernel is first loaded by GRUB and where it sets up the initial descriptor tables, memory mappings, and switches to protected mode.
It invokes code in ```kernel\kernel\kernel.c``` which is where all the kernel subsystems are initialised and test code runs, etc.
