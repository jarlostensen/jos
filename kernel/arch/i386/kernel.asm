[bits 32]
section .bss
; empty
section .data
; empty
section .rodata
; empty 

section .text

global _bochs_debugbreak:function
_bochs_debugbreak:
    xchg bx,bx
    ret

global _gdt_flush:function
; gdt = [esp+4]
_gdt_flush:
    mov eax, [esp+4]
    cli
    lgdt [dword eax]
    mov eax, 10h
    mov ds, ax
    mov es, ax
    mov fs, ax
    mov gs, ax
    ; this obviously depends on the new GDTs layout for data segments matching whatever we came from, or the ret won't happen
    mov ss, ax    
    jmp dword 08h:.flush
.flush: 
    sti
    ret

;TODO: selector for code and and data as arguments?
global _switch_to_protected_mode:function
_switch_to_protected_mode:
    cli
    mov eax, cr0
    or eax,1
    mov cr0, eax
    
    jmp dword 08h:.protected_mode
    nop
    nop    
.protected_mode:
    sti    
    ret
