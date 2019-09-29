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
; empty
section .rodata
; empty 

section .text

; C-code kernel main, called with one parameter pointing to multiboot structure
extern _kmain

%define BOCHS_BREAK xchg bx,bx

global _bochs_debugbreak
_bochs_debugbreak:
    xchg bx,bx
    ret

global _lgdt
; gdt = [esp+4]
_lgdt:
    push ebx
    mov ebx, [esp+8]
    lgdt [dword ebx]
    pop ebx
    ret

;TODO: selector for code and and data as arguments?
global _switch_to_protected_mode
_switch_to_protected_mode:
    mov eax, cr0
    or eax,1
    mov cr0, eax
    
    jmp dword 08h:.protected_mode
    nop
    nop    
.protected_mode:        
    mov eax, 10h
    mov ds, ax
    mov es, ax
    mov fs, ax
    mov gs, ax
    mov ss, ax
    mov esp, stack_top
    sti
    xor eax,eax
    ret

global _start:function (_start.end - _start)
_start:
        mov esp, stack_top
        cld
        push ebx
        call _kmain        
.fi:
        cli
.exit:  hlt
        jmp .exit
.end:

