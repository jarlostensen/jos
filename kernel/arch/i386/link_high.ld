
ENTRY(_start)
 
SECTIONS
{
	/* high kernel
		we're mapping it to the "standard" 3Gig+1Meg location
	 */
	. = 0xc0100000;	
	_k_virtual_start = .;
	/* kernel physical memory starts at 1MB */
	_k_phys_start = . - 0xc0000000;
 
 	/* multiboot header and code */
	.text ALIGN(4K) : AT(ADDR(.text) - 0xc0000000)
	{
		*(.multiboot)
		code = .; _code = .; __code = .;
		*(.text)
	}
 
	/* read-only data. */
	.rodata ALIGN(4K) : AT(ADDR(.rodata) - 0xc0000000)
	{
		*(.rodata)
	}
 
	/* read-write data (initialized) */
	.data ALIGN(4K) : AT(ADDR(.data) - 0xc0000000)
	{
		*(.data)
	}
 
	/* read-write data (uninitialized) and stack */
	.bss ALIGN(4K) : AT(ADDR(.bss) - 0xc0000000)
	{
		*(COMMON)
		*(.bss)
	}

	end = .; _end = .; __end = .;	
	_k_virtual_end = .;
	_k_phys_end = . - 0xc0000000;
}