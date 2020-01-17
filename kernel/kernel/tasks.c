#include <stdint.h>
#include "kernel_detail.h"
#include "memory.h"
#include "interrupts.h"
#include <kernel/tasks.h>
#include <kernel/atomic.h>
#include <stdio.h>

static const size_t kTaskDefaultStackSize = 4096;
static const size_t kTaskStackAlignment = 16;
// in interupts.asm
extern void _k_isr_switch_point(void);

//TESTING:
static task_context_t* _tasks[2];
static unsigned int _task_id = 0;

static void _push_32(task_context_t* ctx, uint32_t val)
{
    ctx->_esp = (void*)((uintptr_t)ctx->_esp - 4);
    ((uint32_t*)ctx->_esp)[0] = val;    
}

void _task_handler(task_context_t* ctx)
{
    //TODO: task start preamble
    //DEBUG:printf("_task_handler 0x%x\n", ctx);

    // execute task body
    ctx->_task_func(ctx->_obj);

    //TODO: remove this task from task list
    // wait for this task never to be called again
    while(true)
    {
        k_pause();
    }
}

void _k_task_switch(task_context_t* ctx)
{
    //TODO: if they're not already disabled... _k_disable_interrupts();
    asm volatile("mov %0, %%esp" : : "r" (ctx->_esp));
    asm volatile("mov %0, %%ebp" : : "r" (ctx->_ebp));
    // https://stackoverflow.com/a/3475763/2030688
    asm volatile("jmp %P0": : "i" (_k_isr_switch_point));

    //NOTE: we never get here, ever...
}

void k_tasks_init(task_func_t root, void* obj)
{
    //TESTING:
    unsigned int id = k_task_create(0, root, obj);
    _k_task_switch(_tasks[id-1]);
}

unsigned int k_task_create(unsigned int pri, task_func_t func, void* obj)
{   
    const size_t aligned_stack_size = (sizeof(isr_stack_t) + kTaskDefaultStackSize + kTaskStackAlignment-1); 
    task_context_t* ctx = (task_context_t*)k_mem_alloc(sizeof(task_context_t)+aligned_stack_size);

    ctx->_stack_size = kTaskDefaultStackSize;
    ctx->_obj = obj;
    ctx->_task_func = func;
    ctx->_pri = pri;
    _tasks[_task_id++] = ctx;
    ctx->_id = _task_id;
    // aligned stack top    
    ctx->_esp = ctx->_stack_top = (void*)( ((uintptr_t)(ctx+1) + aligned_stack_size) & ~(kTaskStackAlignment-1));

    //DEBUG: printf("creating task context at 0x%x, stack size is %d bytes, top @ 0x%x\n", ctx, aligned_stack_size, ctx->_esp);

    // build the "stack" contents;
    // top: | ctx ptr                            |
    //      | dummy (return address)             |
    //      | isr_stack_struct fields, up until: |  
    //      | eip/cs/flags                       |
    //    
    _push_32(ctx, (uint32_t)ctx);
    _push_32(ctx, (uint32_t)0x0badc0de);
    
    ctx->_esp = (void*)((uintptr_t)ctx->_esp - sizeof(isr_stack_t));
    isr_stack_t* stack =  (isr_stack_t*)(ctx->_esp);
    stack->eip = (uintptr_t)_task_handler;
    stack->eflags = k_eflags();
    stack->cs = JOS_KERNEL_CS_SELECTOR;
    stack->ds = JOS_KERNEL_DS_SELECTOR;
    stack->error_code = 0;
    stack->handler_id = 0;
    stack->edi = 0;
    stack->esi = 0;
    stack->ebx = 0;
    stack->edx = 0;
    stack->ecx = 0;
    stack->eax = 0;
    ctx->_ebp = ctx->_esp; 
    stack->ebp = stack->esp = (uintptr_t)ctx->_esp;
    
    return ctx->_id;
}