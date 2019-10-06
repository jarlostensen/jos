[bits 32]
section .bss
; empty
section .data
; empty
section .rodata
; empty 

section .text

; returns 1 if protected mode enabled (PE bit in CR0)
global is_protected_mode:function
is_protected_mode:
    mov eax, cr0
    and eax, 1    
    ret

; outb(port, byte)
; port = [esp+4]
; byte = [esp+8]
global outb:function
outb:
    mov dx, [esp+4]
    mov al, [esp+8]
    out dx,al
    ret

; outw(port, word)
; port = [esp+4]
; word = [esp+8]
global outw:function
outw:
    mov dx, [esp+4]
    mov ax, [esp+8]
    out dx,ax
    ret

; inb(port) -> byte
; port = [esp+4]
global inb:function
inb:
    mov dx, [esp+4]
    in al,dx
    ret

; inw(port) -> word
; port = [esp+4]
global inw:function
inw:
    mov dx, [esp+4]
    in ax, dx
    ret

global enable_interrupts:function
enable_interrupts:
    sti 
    ret

global disable_interrupts:function
disable_interrupts:
    cli
    ret
