[bits 32]

section .text

; array of handlers
extern _isr_handlers

; trampoline to our per interrrupt handlers registered by the kernel
isr_handler_stub:
    
    pushad    
    mov ebx, dword _isr_handlers
    ; handler id, pushed by caller
    mov eax, [esp+2*4+8*4]
    ; each entry is 32 bits!
    call [dword ebx+eax*4]
    
    popad
    ; from handler entry point 
    add esp,+4
    iret

%macro ISR_HANDLER 1
global isr%1:function
isr%1:
    ; isr handler id
    push %1
    jmp isr_handler_stub
%endmacro

ISR_HANDLER 0
ISR_HANDLER 1
ISR_HANDLER 2
ISR_HANDLER 3
ISR_HANDLER 4
ISR_HANDLER 5
