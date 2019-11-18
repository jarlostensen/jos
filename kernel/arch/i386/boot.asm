[bits 32]

; multiboot header 
MBALIGN           equ  (1 << 0)              ; align loaded modules on page boundaries
MBMEMINFO         equ  (1 << 1)              ; provide memory map
MBMAGIC           equ 0x1BADB002             ; define the magic number constant
MBFLAGS           equ MBALIGN | MBMEMINFO    ; multiboot flags
MBCHECKSUM        equ  -(MBMAGIC + MBFLAGS)  ; calculate the checksum

;NOTE: always keep this up-to-date with the value in link_high.ld!
KERNEL_VMA        equ 0x20100000             ; the virtual address of our "high" kernel @ 512 + 1Meg
KERNEL_VMA_OFFSET equ 0x20000000             ; subtracting this from a virtual address gives us the 0-relative offset

section .multiboot
global _mboot
_mboot:
align 4
    dd MBMAGIC
    dd MBFLAGS
    dd MBCHECKSUM
    dd _mboot
._mboot_text:
    dd 0
._mboot_data:
    dd 0 
._mboot_bss:
    dd 0

# kernel startup stack
section .bss
align 16
    stack_bottom:
    resb 16384
    stack_top:

; this is our boot page directory and table used to map our high kernel virtual addresses to physical memory.
; the vritual address range starts at KERNEL_VMA and is mapped to offset 1Meg in physical RAM
align 4096
    boot_page_directory:
    resb 4096
    ; we'll use this to identity map the first 1Meg of physical space
    boot_page_table_0:
    resb 4096
    ; we'll use this one to map the kernel itself
    boot_page_table_1:
    resb 4096

section .data
extern _k_gdt_desc

section .rodata
; empty

section .text

; void _k_main(uint32_t magic, multiboot_info_t *mboot)
extern _k_main

; used to store away multiboot arguments
main_args:
    dd  0
    dd  0

; these are defined in linker_high.ld
extern _k_phys_start
extern _k_virtual_start
extern _k_virtual_end
extern _k_phys_end

; -----------------------------------------------------------------------
; helpers

; eax = virt
; -> ebx = page table entry or 0
_virt_to_page_table_entry:
        push eax
        push edx
        push ecx

        lea ebx,[dword (boot_page_directory - KERNEL_VMA_OFFSET)]
        mov ecx, 400000h
        xor edx, edx
        idiv ecx            ; eax = virt / 4Megs = page directory entry, edx = rem
        shl eax,2           ; eax = 4*eax; byte offset        
        add ebx, eax        ; edi = offset into page directory 
        mov eax, [ebx]      ; edi = page table entry
        test eax, 1         ; is it valid?
        jz .invalid

        and eax, 0xfffff000 ; mask out flags
        mov ebx, eax

        mov eax, edx        ; eax = remainder, offset in 4Meg region
        xor edx,edx 
        mov ecx, 1000h      
        idiv ecx            ; eax = page table entry, edx = rem = byte offset from physical address
        shl eax, 2          ; eax = 4*eax; byte offset
        add ebx, eax        ; edi = physical address of page table entry
        jmp .done

    .invalid:
        xor ebx,ebx
    .done:
        pop ecx
        pop edx
        pop eax
        ret

; eax = virt
; -> eax = phys or 0
_virt_to_phys:
        push ebx
        
        ; map to the right page table
        call _virt_to_page_table_entry
        test ebx,ebx
        jz .virt_to_phys_not_present

        ; get the entry
        mov eax, [ebx]
        test eax,1          ; present?
        jnz .virt_to_phys_end
    .virt_to_phys_not_present:
        xor eax,eax
    .virt_to_phys_end:
        and eax, 0xfffff000
        
        pop ebx
        ret

; -----------------------------------------------------------------------
; 
global _start:function (_start.end - _start)
_start:                
        ; safety check against being loaded in a non-multiboot way
        cmp eax, 2badb002h
        ;TODO: write something to screen
        jne .fi

        ; the multiboot the machine state is as follows:
        ; https://www.gnu.org/software/grub/manual/multiboot/html_node/Machine-state.html#Machine-state

        cld
        mov ebp, stack_top
        mov esp, ebp
        ; store these for later, when we call main
        ; note: ebx is the LMA and will point somewhere in <1Meg RAM
        mov [main_args- KERNEL_VMA_OFFSET], ebx
        mov [(main_args+4) - KERNEL_VMA_OFFSET], eax
        
        ; --------------------------------------------
        ; set up the initial page mapping from 0->0
        ; --------------------------------------------
        ; identity map the first meg in boot_page_table_0

        lea edi,[dword (boot_page_directory - KERNEL_VMA_OFFSET)]       ; edi = physical address of boot_page_directory
        lea esi,[dword (boot_page_table_0 -  KERNEL_VMA_OFFSET)]          ; esi = physical address of boot_page_table_0
        mov edx, esi
        and edx, 0xfffff000
        or edx,1
        mov [edi], edx                                                  ; physical 4K aligned address of first page table entry and "present" bit set
        xor edx,edx
    .map_1st_meg:
        ; set each page table entry to the next physical address with "present" bit set
        mov ecx, edx
        or ecx,1
        mov [esi], ecx
        add esi,4
        ; next frame (4K increments) until we've covered the first meg
        add edx, 0x1000
        cmp edx, 0x100000
        jle .map_1st_meg

        ; --------------------------------------------
        ; set up the kernel mapping from KERNEL_VMA->0x100000
        ; --------------------------------------------       
        ; map kernel into boot_page_table_1

        lea ecx,[dword _k_virtual_end]
        sub ecx,dword _k_virtual_start                                  ; ecx = size of kernel
        mov edx, ecx
        and edx, 0xfff
        jz .map_kernel
        ; adjust for remainder frame (kernel size mod 4K)
        inc ecx
    .map_kernel:
        ; we need to map ecx pages
        add edi, 4*(KERNEL_VMA/400000h)     ; edi = physical address of page directory entry for the start of our kernel in virtual memory

        ; calculate the offset into the page table for the kernel's virtual start address
        lea esi,[ dword (boot_page_table_1 -  KERNEL_VMA_OFFSET)]
        mov edx, _k_virtual_start
        and edx, 3fffffh
        shr edx, 12
        shl edx, 2
        add edx, esi
        mov esi, edx

        or edx,1
        mov [edi], edx                  ; entry for our kernel in the page directory ->  boot_page_table_1[_k_virtual_start]

        mov edx, _k_phys_start
    .map_kernel_page:
        mov ebx, edx
        or ebx,1
        mov [esi], ebx
        add esi, 4
        add edx, 0x1000
        cmp edx, _k_phys_end
        jle .map_kernel_page
        
        ; now switch on paging
        lea ebx, [dword(boot_page_directory - KERNEL_VMA_OFFSET)]
        mov cr3, ebx
        mov ebx, cr0
        or ebx, 0x80000001
        mov cr0, ebx
        jmp dword 10h:.high_half
    .high_half:
        
        ; at this point all kernel virtual addresses are valid through our mappings

        ; switch to our own GDT
        cli
        lgdt [dword _k_gdt_desc]
        mov eax, 10h
        mov ds, ax
        mov es, ax
        mov fs, ax
        mov gs, ax
        ; this obviously depends on the new GDTs layout for data segments matching whatever we came from, or the ret won't happen
        mov ss, ax
        jmp dword 08h:.flush_gdt
    .flush_gdt:        
        mov eax, cr0        
        test eax,1
        jnz .protected_mode

        ; GRUB has already switched us into this mode so should never be required, but we'll leave it for now
        or eax,1
        mov cr0, eax        
        jmp dword 08h:.protected_mode
        nop
        nop    
    .protected_mode:        
        push dword [main_args]              ; ebx = multiboot pointer
        push dword [main_args+4]            ; eax = multiboot magic number
        call _k_main
        add esp, 2*4
.fi:
        cli
.exit:  hlt
        jmp .exit
.end:


; ====================================================================
; left here for reference; getting a switch to real mode is fiddly.
; it requries code to be located in < 1MB memory so this piece of 
; code has to be copied or loaded there. We're not using this because 
; multiboot provides us with the information we need on boot, eliminating the
; need for making BIOS calls etc.
 
[bits 16]
_k_16_bit_data_start:
    dw 0x3ff		; 256 entries, 4b each = 1K
	dd 0			; Real Mode IVT @ 0x0000
_k_16_bit_data_jmp_offs:
    dw 1234h    
global _16bit_main:function
_16bit_main:
    ;  just need this temporarely
    mov eax, 10h
    mov ds, ax
    mov es, ax
    
    ; load the IVT 
    lidt [500h]

    ; now switch out of protected mode so that we can use the BIOS for some of our initial set up
    mov eax, cr0
    xor eax, 1
    mov cr0, eax
    mov ebx,[506h]
    jmp far ebx
_16bit_main_real_mode:
    mov eax, 0
    mov ds, ax
    mov es, ax
    mov fs, ax
    mov gs, ax
    mov ss, ax
    ; top of conventional RAM block, should be enough (see for example https://wiki.osdev.org/Memory_Map_(x86)) or 
    ; https://web.archive.org/web/20130609073242/http://www.osdever.net/tutorials/rm_addressing.php?the_id=50
    mov sp, 0x7bff
    sti

    xor eax,eax
    mov eax, 0e820h
    int 15h

    xchg bx,bx
    
_16bit_main_end:    
    nop
