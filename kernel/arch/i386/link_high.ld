
ENTRY(_start)
 
SECTIONS
{
	/* high kernel
		we're assuming 1Gig of physical RAM here and we're telling the linker to 
		map the kernel to the virtual address range starting at 512+1Meg  (0x20000000+0x100000)		
	 */
	. = 0x20100000;	
	_k_virtual_start = .;
	/* kernel physical memory starts at 1MB */
	_k_phys_start = . - 0x20000000;
 
 	/* multiboot header and code */
	.text ALIGN(4K) : AT(ADDR(.text) - 0x20000000)
	{
		*(.multiboot)
		code = .; _code = .; __code = .;
		*(.text)
	}
 
	/* read-only data. */
	.rodata ALIGN(4K) : AT(ADDR(.rodata) - 0x20000000)
	{
		*(.rodata)
	}
 
	/* read-write data (initialized) */
	.data ALIGN(4K) : AT(ADDR(.data) - 0x20000000)
	{
		*(.data)
	}
 
	/* read-write data (uninitialized) and stack */
	.bss ALIGN(4K) : AT(ADDR(.bss) - 0x20000000)
	{
		*(COMMON)
		*(.bss)
	}

	end = .; _end = .; __end = .;	
	_k_virtual_end = .;
	_k_phys_end = . - 0x20000000;
}