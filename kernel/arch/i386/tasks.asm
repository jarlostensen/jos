[bits 32]

section .text

%include "arch/i386/macros.inc"

global _k_task_switch_point:function
; "unwind" the stack set up by a ISR_HANDLER_PREAMBLE and return to the cs:eip stored on that stack
_k_task_switch_point:
    ; at this point we've switched to the target task's stack
    ISR_HANDLER_POSTAMBLE    
    iret

global _k_task_yield:function
; _k_task_yield(task_context_t* curr_ctx, task_context_t* new_ctx)
_k_task_yield:

    ; load ctx before we start clobbering the stack
    mov esi, [esp+4]    ; curr_ctx
    mov ebx, [esp+8]    ; new_ctx
    
    ; save current context 
    pushf
    push 08h    ; _JOS_KERNEL_CS_SELECTOR
    lea edi, [.yield_resume]
    push edi

    ; error code + id (only used with a real ISR stack frame)
    push 0
    push 0

    ISR_HANDLER_PREAMBLE
    ; NOTE: assumes task_context_t structure layout
    add esi, 3*4
    mov [esi+0], esp
    mov [esi+4], ebp
    
    ; switch to new context's stack
    add ebx, 3*4
    mov esp, [ebx+0]
    mov ebp, [ebx+4]
    ;  switch to the new task
    jmp _k_task_switch_point
    
.yield_resume:
    ; when we next resume we'll re-appear here
    ret
