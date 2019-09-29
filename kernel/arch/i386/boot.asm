[bits 32]

; multiboot header 
MBALIGN           equ  (1 << 0)            ; align loaded modules on page boundaries
MBMEMINFO         equ  (1 << 1)            ; provide memory map
MBMAGIC           equ 0x1BADB002         ; define the magic number constant
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

# kernel startup stack
section .bss
align 16
    stack_bottom:
    resb 16384
    stack_top:

section .data
extern _gdt_desc

section .rodata
; empty 

section .text

; C-code kernel init and main, called with one parameter pointing to multiboot structure
extern _kinit
extern _kmain

global _start:function (_start.end - _start)
_start:
        mov ebp, stack_top
        mov esp, ebp

        cld
        push ebx
        call _kinit
        add esp, 4 

        ; switch GDT
        cli
        lgdt [dword _gdt_desc]
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
        sti    
        call _kmain

.fi:
        cli
.exit:  hlt
        jmp .exit
.end:

