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
struct _task_context 
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
} __attribute__((packed));
typedef struct _task_context task_context_t;

// initialise task system and switch to the root task
__attribute__((__noreturn__)) void k_tasks_init(task_func_t root, void* obj);

typedef struct _task_create_info
{
    unsigned int _pri;
    task_func_t _func;
    void*       _obj;
} task_create_info_t;

// create a task, return the id.
// this sets up the initial stack and context for the task.
unsigned int k_task_create(task_create_info_t* info);

void k_task_yield(void);

#endif // JOS_KERNEL_TASKS_H