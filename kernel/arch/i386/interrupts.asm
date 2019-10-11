[bits 32]

section .text

; handler; forwards call to the registered handler via argument 0
extern _isr_handler
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

; trampoline to our per interrrupt handlers registered by the kernel
isr_handler_stub:
    
    ;  eax, ecx, edx, ebx, original esp, ebp, esi, and edi
    pushad    

    ; save caller's data segment (could be kernel, could be user)
    mov ax, ds
    push eax
    
    ;TODO: swap to kernel data segments + stack    
    call _isr_handler
    
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
