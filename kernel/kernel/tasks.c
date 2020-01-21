#include <stdint.h>
#include "kernel_detail.h"
#include "memory.h"
#include "interrupts.h"
#include <kernel/tasks.h>
#include <kernel/atomic.h>
#include <collections.h>
#include <stdio.h>

static const size_t kTaskDefaultStackSize = 4096;
static const size_t kTaskStackAlignment = 16;
// in tasks.asm
extern void _k_task_switch_point(void);
extern void _k_task_yield(task_context_t* ctx);

static vector_t _tasks;
static unsigned int _task_id = 0;
static atomic_int_t _current_task;

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

__attribute__((__noreturn__)) void _k_task_start(task_context_t* ctx)
{
    //TODO: if they're not already disabled... _k_disable_interrupts();

    __atomic_store_n(&_current_task._val, ctx->_id, __ATOMIC_RELAXED);    
    asm volatile("mov %0, %%esp" : : "r" (ctx->_esp));
    asm volatile("mov %0, %%ebp" : : "r" (ctx->_ebp));

    // see https://stackoverflow.com/a/3475763/2030688 for the syntax used here
    asm volatile("jmp %P0": : "i" (_k_task_switch_point));

    __builtin_unreachable();
}

void k_task_yield(void)
{
    int id = __atomic_load_n(&_current_task._val, __ATOMIC_RELAXED);
    _k_task_yield((task_context_t*)vector_at(&_tasks, id));
}

void k_tasks_init(task_func_t root, void* obj)
{
    vector_create(&_tasks, 32, sizeof(task_context_t)); 
    __atomic_store_n(&_current_task._val, 0, __ATOMIC_RELAXED);
    unsigned int id = k_task_create(0, root, obj);
    //NOTE: for now the task's ID matches the index into _tasks, but if we allow destroying tasks that could change!
    _k_task_start((task_context_t*)vector_at(&_tasks,id));
}

unsigned int k_task_create(unsigned int pri, task_func_t func, void* obj)
{   
    const size_t aligned_stack_size = (sizeof(isr_stack_t) + kTaskDefaultStackSize + kTaskStackAlignment-1); 
    task_context_t* ctx = (task_context_t*)k_mem_alloc(sizeof(task_context_t)+aligned_stack_size);

    ctx->_stack_size = kTaskDefaultStackSize;
    ctx->_obj = obj;
    ctx->_task_func = func;
    ctx->_pri = pri;
    ctx->_id = _task_id++;
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

    //TODO: this requires us to have interrupts disabled
    vector_push_back(&_tasks, (void*)ctx);
    
    return ctx->_id;
}