[bits 32]
section .bss
; in kernel_loader.asm
extern _k_stack_top

section .data
_k_fpu_state_save:
    resb 28


section .rodata
; the real mode IDT is used to switch to real mode, whenever we need that
real_mode_idt:
    dw 3fffh
    dd 0        ; real mode ivt

section .text

; various helper macros, like BOCHS_BREAKPOINT and LOAD_ARG etc.
%include "arch/i386/macros.inc"

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

; void _k_load_page_directory(uint32_t physPageDirStart)
global _k_load_page_directory:function
_k_load_page_directory:
    mov eax, [esp+4]
    mov cr3, eax
    ret

global _k_enable_paging:function
_k_enable_paging:
    mov eax, cr0
    or eax, 0x80000001
    mov cr0, eax
    ret

; void _k_move_stack(uintptr_t virt_top)
; NOTE: this function EXPECTS INTERRUPTS TO BE CLEARED
; it is inly invoked from memory.c for initialisation purposes
global _k_move_stack:function
_k_move_stack:
    push ebp
    mov ebp, esp

    push eax
    push ecx
    push esi
    push edi

    LOAD_ARG eax, 0             ; new stack top

    ; cli
    lea ecx, [_k_stack_top]
    sub ecx, esp                ; bytes pushed on stack until this point
    mov esi, esp                ; start of current stack to copy
    sub eax, ecx
    and eax, ~0xf               ; make sure it's 16 byte aligned
    mov edi, eax                ; start of target stack area
    shr ecx, 2
    rep movsd                   ; copy stack
    mov esp, eax                ; switch to new stack
    ; sti

    pop edi
    pop esi
    pop ecx
    pop eax
    pop ebp

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


