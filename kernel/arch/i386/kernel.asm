[bits 32]
section .bss
; empty
section .data
_k_fpu_state_save:
    resb 28


section .rodata
; the real mode IDT is used to switch to real mode, whenever we need that
real_mode_idt:
    dw 3fffh
    dd 0        ; real mode ivt

section .text

global k_bios_call:function
; k_bios_call(int_num, ax, bx, cx, dx, si, di)
k_bios_call:
    push ebp
    mov ebp, esp
    pushad

    ; TODO: https://wiki.osdev.org/Real_Mode

    pop ebp
    popad
    ret

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

global k_eflags:function
k_eflags:
    pushf
    pop eax
    ret

;TODO:
global _k_check_fpu:function
_k_check_fpu:
    fninit 
    fnstenv [_k_fpu_state_save]
    ret

; this is the core clock IRQ update function, it counts milliseconds using 32.32 fixed point fractions
; clock.c
; running sum
extern _k_clock_frac
; clock frequency fraction and whole part
extern _k_clock_freq_frac
extern _k_clock_freq_whole
extern _k_ms_elapsed
global _k_update_clock:function
_k_update_clock: 
    push eax
    push ebx
    mov ebx, [_k_clock_frac]
    mov eax, [_k_ms_elapsed]
    ; update fraction, CF=1 if we're rolling over
    add ebx, [_k_clock_freq_frac]
    ; update ms if fraction rolled over
    adc eax, [_k_clock_freq_whole]
    mov [_k_clock_frac], ebx
    mov [_k_ms_elapsed], eax
    pop ebx
    pop eax
    ret


