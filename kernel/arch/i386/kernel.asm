[bits 32]
section .bss
; empty
section .data
; empty
section .rodata
; empty 

section .text

; returns 1 if protected mode enabled (PE bit in CR0)
global k_is_protected_mode:function
k_is_protected_mode:
    mov eax, cr0
    and eax, 1    
    ret

; outb(port, byte)
; port = [esp+4]
; byte = [esp+8]
global k_outb:function
k_outb:
    mov dx, [esp+4]
    mov al, [esp+8]
    out dx,al
    ret

; outw(port, word)
; port = [esp+4]
; word = [esp+8]
global k_outw:function
k_outw:
    mov dx, [esp+4]
    mov ax, [esp+8]
    out dx,ax
    ret

; inb(port) -> byte
; port = [esp+4]
global k_inb:function
k_inb:
    mov dx, [esp+4]
    in al,dx
    ret

; inw(port) -> word
; port = [esp+4]
global k_inw:function
k_inw:
    mov dx, [esp+4]
    in ax, dx
    ret

; issue a NOP write to port 0x80 (POST status) to idle for a few cycles
global k_io_wait:function
k_io_wait:
    ; doesn't matter what al contains
    out 0x80, al
    ret

global _k_enable_interrupts:function
_k_enable_interrupts:
    sti 
    ret

global _k_disable_interrupts:function
_k_disable_interrupts:
    cli
    ret

; hard stop, used by kernel_panic
global _k_halt_cpu:function
_k_halt_cpu:
    cli
.halt_cpu:
    hlt 
    jmp .halt_cpu
