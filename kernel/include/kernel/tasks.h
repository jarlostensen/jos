#ifndef JOS_KERNEL_TASKS_H
#define JOS_KERNEL_TASKS_H

/*
    Task management and task state.    

    A task is state and a stack which is used to save and restore the register context and current eip.
    Switching between tasks uses the stack and the iret instruction for simplicity.
*/

typedef void (*task_func_t)(void* obj);
// a task is described by a context object which contains information about the entry point, stack,
// priority, etc.
typedef struct _task_context 
{
    unsigned int    _id;
    // priority
    unsigned int    _pri;
    // dedicated task stack top
    void*           _stack_top;
    // current esp
    void*           _esp;
    // current ebp
    void*           _ebp;
    // size of stack
    size_t          _stack_size;
    // the entry point for the task (NOTE: this is invoked by a task proxy to control shutdown behaviour etc.)
    task_func_t     _task_func;
    // optional argument for the task function
    void*           _obj;
} task_context_t;
// initialise task system
void k_tasks_init(task_func_t root, void* obj);
// create a task, return the id.
// this sets up the initial stack and context for the task.
unsigned int k_task_create(unsigned int pri, task_func_t func, void* obj);

#endif // JOS_KERNEL_TASKS_H