[bits 32]

section .text

%include "arch/i386/macros.inc"

global _k_task_switch_point:function
; "unwind" the stack set up by a ISR_HANDLER_PREAMBLE and return to the cs:eip stored on that stack
_k_task_switch_point:
    ISR_HANDLER_POSTAMBLE    
    iret

global _k_task_yield:function
; _k_task_yield(task_context_t* ctx)
_k_task_yield:    

    ; load ctx before we start clobbering the stack
    mov esi, [esp+8]
    
    ; "fake" the iret structure
    pushf
    push 08h    ; JOS_KERNEL_CS_SELECTOR
    lea edi, [.yield_resume]
    push edi
    
    ; now push the rest 
    ISR_HANDLER_PREAMBLE
   
    ; NOTE: assumes task_context_t structure layout
    add esi, 3*4
    mov esp, [esi+0]
    mov ebp, [esi+4]    
    ;  switch to the new task
    jmp _k_task_switch_point
    
.yield_resume:
    ; when we next resume we'll re-appear here
    ret
