
ENTRY(_start)
 
SECTIONS
{
	/*TODO: high kernel */
	. = 1M;
	_k_virtual_start = .;
	_k_phys_start = .;
 
 	/* multiboot header and code */
	.text BLOCK(4K) : ALIGN(4K)
	{
		*(.multiboot)
		code = .; _code = .; __code = .;
		*(.text)
	}
 
	/* Read-only data. */
	.rodata BLOCK(4K) : ALIGN(4K)
	{
		*(.rodata)
	}
 
	/* Read-write data (initialized) */
	.data BLOCK(4K) : ALIGN(4K)
	{
		*(.data)
	}
 
	/* Read-write data (uninitialized) and stack */
	.bss BLOCK(4K) : ALIGN(4K)
	{
		*(COMMON)
		*(.bss)
	}

	/* as long as nothing is automatically added, this will be the end */
	end = .; _end = .; __end = .;

	/* NOTE: when we go high-kernel these will be different */
	_k_virtual_end = .;
	_k_phys_end = .;
}