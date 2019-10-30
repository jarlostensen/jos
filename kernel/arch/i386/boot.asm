[bits 32]

; multiboot header 
MBALIGN           equ  (1 << 0)              ; align loaded modules on page boundaries
MBMEMINFO         equ  (1 << 1)              ; provide memory map
MBMAGIC           equ 0x1BADB002             ; define the magic number constant
MBFLAGS           equ MBALIGN | MBMEMINFO    ; multiboot flags
MBCHECKSUM        equ  -(MBMAGIC + MBFLAGS)  ; calculate the checksum

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

section .data
extern _k_gdt_desc

section .rodata
; empty

section .text

; C-code kernel init and main, called with one parameter pointing to multiboot structure
extern _k_init
extern _k_main

global _start:function (_start.end - _start)
_start:
        ;NOTE: assuming we're coming in here via multiboot the machine state is as follows:
        ; https://www.gnu.org/software/grub/manual/multiboot/html_node/Machine-state.html#Machine-state

        mov ebp, stack_top
        mov esp, ebp

        cld
        push ebx
        push eax        
        call _k_init
        add esp, 8 

        ; switch GDT
        cli
        lgdt [dword _k_gdt_desc]
        mov eax, 10h
        mov ds, ax
        mov es, ax
        mov fs, ax
        mov gs, ax
        ; this obviously depends on the new GDTs layout for data segments matching whatever we came from, or the ret won't happen
        mov ss, ax            
        jmp dword 08h:.flush
    .flush:    
        mov eax, cr0
        or eax,1
        mov cr0, eax
        
        jmp dword 08h:.protected_mode
        nop
        nop    
    .protected_mode:
        call _k_main

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
