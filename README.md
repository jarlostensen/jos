# jos
My own minimal OS.<br/>
This is very much a learning project, it will probably never go beyond that.

Built on knowledge from:
* https://wiki.osdev.org/Main_Page
* http://www.osdever.net
* www.jamesmolloy.co.uk
* Tanebaum & Woodhull "Operating Systems"

## Dependencies/Tools used

* GCC cross compiler
* make and build scripts, libc framework from https://wiki.osdev.org/Meaty_Skeleton
* NASM (because I just can't stomach AT&T syntax)
* Bochs debugger http://bochs.sourceforge.net/doc/docbook/user/internal-debugger.html 
* VSCode
* WSL

## Setup
My dev environment is Windows 10 so I have Ubuntu runing in WSL where the build environment (and GCC cross compiler) live.
I write code using VSCode in Windows, having a working directory mounted in WSL using standard ```ln```. 
I am using the windows version of Bochs, but so far only the non-GUI version, and run it from a command prompt.
