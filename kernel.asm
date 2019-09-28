[bits 32]
;org 0x10000

MBALIGN           equ  (1 << 0)            ; align loaded modules on page boundaries
MBMEMINFO         equ  (1 << 1)            ; provide memory map
MBMAGIC           equ 0x1BADB002         ; define the magic number constant
MBFLAGS           equ MBALIGN | MBMEMINFO    ; multiboot flags
MBCHECKSUM        equ  -(MBMAGIC + MBFLAGS)  ; calculate the checksum

section .multiboot
align 4
    dd MBMAGIC             ; write the magic number to the machine code,
    dd MBFLAGS                    ; the flags,
    dd MBCHECKSUM                 ; and the checksum

# Reserve a stack for the initial thread.
section .bss
align 16
    stack_bottom:
    resb 16384 ; 16 KiB
    stack_top:

section .data
hello_string                db 'Hello Minimal Kernel World',0
real_mode_string            db 'Real mode',0
prot_mode_string            db 'Protected mode',0
a20_enabled_string          db 'A20 enabled',0
a20_not_enabled_string      db 'A20 not enabled',0

hex_digits          db '0123456789abcdef'

section .rodata
; =====================================================
;  a simple GDT fo a non-paged flat environment
flat_gdt:
.gdt_null:
   dq 0
.gdt_code:
   dw 0FFFFh        ; 4Gig limit
   dw 0             ; base 0
   db 0             ; base contd.
   db 10011010b     ; type + privilege level + present
   db 11001111b     ; limits + attributes + granularity + base
   db 0             ; last byte of base address
.gdt_data:
   dw 0FFFFh        ; 4Gig limit
   dw 0             ; base 0
   db 0             ; base contd.
   db 10010010b
   db 11001111b
   db 0
gdt_desc dw $ - flat_gdt
         dd flat_gdt

section .text

VGA_MEM equ 0xb8000
GREEN           equ 0x2e
RED             equ 0x4e
WHITEONBLUE     equ 0x1f

%define BOCHS_BREAK xchg bx,bx

_clear_screen:
    push eax
    push ebx
    mov ebx, VGA_MEM
    mov eax, 0x20202020
    mov ecx, 80*25
    shr ecx,2
    rep stosd
    pop ebx
    pop eax
    ret

; esi = string
; al = color
; cl = line
_prot_print_string:
    push eax
    push ebx
    push ecx
    push esi
    mov ebx, VGA_MEM    
    test cl,cl
    jz .prot_print_string.1

.prot_print_string.0:
    add ebx,80*2
    dec cl
    jnz .prot_print_string.0

.prot_print_string.1:    
    shl eax,8

.prot_print_string.2:
    mov al, [esi]
    test al,al
    jz .prot_print_string.3
    mov [ebx],ax
    add ebx,2
    inc esi
    jmp .prot_print_string.2
.prot_print_string.3:
    pop esi
    pop ecx
    pop ebx
    pop eax
    ret

check_a20_enabled:
    push eax
    push esi
    push edi

    ; NOTE: this bombs, presumably because the IDT is invalid and we can't do *any* interrupts
    ; int 15h 
    ;mov ax, 2403h
    ;int 15h
    ;jb .int15_ns
    ;cmp ah, 0
    ;jnz .int15_ns
    ;mov ax,2402h
    ;int 15h
    ;jb .int15_ns
    ;cmp ah,0
    ;jnz .int15_ns
    ;cmp al,1
    ;jmp .done

.int15_ns:
    ; check values one megabyte apart
    lea edi,[0x112345] ;odd megabyte address.
    lea esi,[0x012345] ;even megabyte address.
    mov [esi],esi     ;making sure that both addresses contain diffrent values.
    mov [edi],edi
    cmpsd 
.done:
    ; if ZF=0 here A20 is set
    pop edi
    pop esi
    pop eax
    ret

; ============================================================================================================
global _start:function (_start.end - _start)
_start:
        mov esp, stack_top

        cld
        call _clear_screen
        lea esi, [dword hello_string]
        mov al, GREEN
        xor cl,cl
        call _prot_print_string

        lea esi, [dword real_mode_string]
        mov al, GREEN
        mov cl, 1
        call _prot_print_string

;.switch_to_protected_mode:
        cli
        lea esi, [dword gdt_desc]
        lgdt [dword gdt_desc]
        mov eax, cr0
        or eax,1
        mov cr0, eax
        
        jmp dword 08h:.protected_mode
        nop
        nop
        nop

.protected_mode:        
        ; data segment selector
        mov eax, 10h
        mov ds, ax
        mov es, ax
        mov fs, ax
        mov gs, ax
        mov ss, ax
        mov esp, stack_top
        sti

        ;BOCHS_BREAK

        lea esi, [dword prot_mode_string]
        mov eax, WHITEONBLUE
        mov ecx, 3
        call _prot_print_string
        
    ;    call check_a20_enabled
    ;    je .a20_not_enabled
    ;    lea esi, [dword a20_enabled_string]
    ;    mov al, GREEN
    ;    mov cl, 4
    ;    call _prot_print_string

        jmp .fi

.a20_not_enabled:
        lea esi, [dword a20_not_enabled_string]
        mov al, RED
        mov cl, 4
        call _prot_print_string        

.fi:
        BOCHS_BREAK

        cli
.exit:  hlt
        jmp .exit
.end:

