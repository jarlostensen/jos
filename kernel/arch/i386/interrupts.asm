[bits 32]

section .text

; =====================================================================================
; IDT

; the idt descriptor
extern _idt_desc

; load the _idt table 
global _k_load_idt:function
_k_load_idt:
    mov eax, dword _idt_desc
    lidt [eax]
    ret

global _k_store_idt:function
_k_store_idt:
    sidt [esp+4]
    ret

; =====================================================================================
; IRQs

PIC1_COMMAND equ 0x20
PIC2_COMMAND equ 0x0a

; handler; forwards call to the registered handler via argument 0
extern _k_irq_handler

irq_handler_stub:

    ; put another copy of the IRQ number on the stack for later
    push dword [esp]
    
    ; chain to handler, irq number is first arg
    call _k_irq_handler
    add esp, 4

    ; send EOI to the right PIC    
    pop eax
    cmp al, 8
    jl .irq_handler_stub_1
    ; EOI to PIC2
    mov al, 0x20
    out PIC2_COMMAND, al
    ; always send EOI to master (PIC1)
.irq_handler_stub_1:
    ; EOI to PIC1
    mov al, 0x20
    out PIC1_COMMAND, al
.irq_handler_stub_2:
    iret

%macro IRQ_HANDLER 1
global _k_irq%1:function
_k_irq%1:        
    push dword %1
    jmp irq_handler_stub
%endmacro

IRQ_HANDLER 0 
IRQ_HANDLER 1
IRQ_HANDLER 2
IRQ_HANDLER 3
IRQ_HANDLER 4
IRQ_HANDLER 5
IRQ_HANDLER 6
IRQ_HANDLER 7
IRQ_HANDLER 8
IRQ_HANDLER 9
IRQ_HANDLER 10 
IRQ_HANDLER 11
IRQ_HANDLER 12
IRQ_HANDLER 13
IRQ_HANDLER 14
IRQ_HANDLER 15
IRQ_HANDLER 16
IRQ_HANDLER 17
IRQ_HANDLER 18
IRQ_HANDLER 19

; =====================================================================================
; ISRs

; handler; forwards call to the registered handler via argument 0
extern _k_isr_handler

global _k_isr_switch_point:function

isr_handler_stub:
    
    ;  eax, ecx, edx, ebx, original esp, ebp, esi, and edi
    pushad    

    ; save caller's data segment (could be kernel, could be user)
    mov ax, ds
    push eax
    
    cld
    ;TODO: swap to kernel data segments + stack    
    call _k_isr_handler    
    
    ; we use this for our task switcher...
_k_isr_switch_point:
    pop eax
    popad
    ; from handler entry point (error code + isr id)
    add esp,+8    
    iret


; an isr/fault/trap that doesn't provide an error code
%macro ISR_HANDLER 1
global _k_isr%1:function
_k_isr%1:
    cli
    ; empty error code
    push 0
    ; isr id
    push dword %1
    jmp isr_handler_stub
%endmacro

; some faults (but not all) provide an error code
%macro ISR_HANDLER_ERROR_CODE 1
global _k_isr%1:function
_k_isr%1:
    cli     
    ; error code is already pushed
    ; isr id
    push dword %1
    jmp isr_handler_stub
%endmacro

; divide by zero
ISR_HANDLER 0
; debug
ISR_HANDLER 1
; NMI
ISR_HANDLER 2
; breakpoint
ISR_HANDLER 3
; overflow
ISR_HANDLER 4
; bound range exceeded
ISR_HANDLER 5
; invalid opcode
ISR_HANDLER 6
; device not available
ISR_HANDLER 7
; double fault
ISR_HANDLER_ERROR_CODE 8
; "coprocessor segment overrun"
ISR_HANDLER 9
; invalid TSS
ISR_HANDLER_ERROR_CODE 10
; segment not present
ISR_HANDLER_ERROR_CODE 11
; stack segment fault
ISR_HANDLER_ERROR_CODE 12
; genera protection fault
ISR_HANDLER_ERROR_CODE 13
; page fault
ISR_HANDLER_ERROR_CODE 14
; this one is reserved...
ISR_HANDLER 15
; x87 FP exception
ISR_HANDLER 16
; alignment check fault
ISR_HANDLER_ERROR_CODE 17
; machine check
ISR_HANDLER 18
; SIMD fp exception
ISR_HANDLER 19
; virtualization exception
ISR_HANDLER 20
; reserved
ISR_HANDLER 21
ISR_HANDLER 22
ISR_HANDLER 23
ISR_HANDLER 24
ISR_HANDLER 25
ISR_HANDLER 26
ISR_HANDLER 27
ISR_HANDLER 28
ISR_HANDLER 29
; security exception
ISR_HANDLER_ERROR_CODE 30
; "fpu error interrupt"
ISR_HANDLER 31
