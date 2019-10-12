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

; handler; forwards call to the registered handler via argument 0
extern _k_irq_handler

irq_handler_stub:

    ; chain to handler, irq number is first arg
    call _k_irq_handler
    
    ; send EOI to the right PIC
    
    cmp [esp],dword 8
    jl .irq_handler_stub_1
    ; EOI to PIC2
    mov al, 0x20
    out 0xa0, al
    jmp .irq_handler_stub_2
.irq_handler_stub_1:
    ; EOI to PIC1
    mov al, 0x20
    out 0x20, al
.irq_handler_stub_2:
    add esp, 4
    iret

%macro IRQ_HANDLER 1
global _k_irq%1:function
_k_irq%1:
    push %1
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

isr_handler_stub:
    
    ;  eax, ecx, edx, ebx, original esp, ebp, esi, and edi
    pushad    

    ; save caller's data segment (could be kernel, could be user)
    mov ax, ds
    push eax
    
    ;TODO: swap to kernel data segments + stack    
    call _k_isr_handler
    
    pop eax
    popad

    ; from handler entry point (isr id)
    add esp,+4
    iret

; the frame/stub for an isr. 
%macro ISR_HANDLER 1
global _k_isr%1:function
_k_isr%1:
    cli
    ; isr id
    push %1
    jmp isr_handler_stub
%endmacro

ISR_HANDLER 0
ISR_HANDLER 1
ISR_HANDLER 2
ISR_HANDLER 3
ISR_HANDLER 4
ISR_HANDLER 5
ISR_HANDLER 6
ISR_HANDLER 7
ISR_HANDLER 8
ISR_HANDLER 9
ISR_HANDLER 10
ISR_HANDLER 11
ISR_HANDLER 12
ISR_HANDLER 13
ISR_HANDLER 14
ISR_HANDLER 15
ISR_HANDLER 16
ISR_HANDLER 17
ISR_HANDLER 18
ISR_HANDLER 19
ISR_HANDLER 20
ISR_HANDLER 21
ISR_HANDLER 22
ISR_HANDLER 23
ISR_HANDLER 24
ISR_HANDLER 25
ISR_HANDLER 26
ISR_HANDLER 27
ISR_HANDLER 28
ISR_HANDLER 29
ISR_HANDLER 30
ISR_HANDLER 31
